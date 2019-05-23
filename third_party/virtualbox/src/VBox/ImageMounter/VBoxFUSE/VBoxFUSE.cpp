/* $Id: VBoxFUSE.cpp $ */
/** @file
 * VBoxFUSE - Disk Image Flattening FUSE Program.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT /** @todo log group */
#include <iprt/types.h>

#if defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD) || defined(RT_OS_LINUX)
# include <sys/param.h>
# undef PVM     /* Blasted old BSD mess still hanging around darwin. */
#endif
#define FUSE_USE_VERSION 27
#include <fuse.h>
#include <errno.h>
#include <fcntl.h>
#ifndef EDOOFUS
# ifdef EBADMACHO
#  define EDOOFUS EBADMACHO
# elif defined(EPROTO)
#  define EDOOFUS EPROTO                /* What a boring lot. */
//# elif defined(EXYZ)
//#  define EDOOFUS EXYZ
# else
#  error "Choose an unlikely and (if possible) fun error number for EDOOFUS."
# endif
#endif

#include <VBox/vd.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/critsect.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Node type.
 */
typedef enum VBOXFUSETYPE
{
    VBOXFUSETYPE_INVALID = 0,
    VBOXFUSETYPE_DIRECTORY,
    VBOXFUSETYPE_FLAT_IMAGE,
    VBOXFUSETYPE_CONTROL_PIPE
} VBOXFUSETYPE;

/**
 * Stuff common to both directories and files.
 */
typedef struct VBOXFUSENODE
{
    /** The directory name. */
    const char             *pszName;
    /** The name length. */
    size_t                  cchName;
    /** The node type. */
    VBOXFUSETYPE            enmType;
    /** The number of references.
     * The directory linking this node will always retain one. */
    int32_t volatile        cRefs;
    /** Critical section serializing access to the node data. */
    RTCRITSECT              CritSect;
    /** Pointer to the directory (parent). */
    struct VBOXFUSEDIR     *pDir;
    /** The mode mask. */
    RTFMODE                 fMode;
    /** The User ID of the directory owner. */
    RTUID                   Uid;
    /** The Group ID of the directory. */
    RTUID                   Gid;
    /** The link count. */
    uint32_t                cLinks;
    /** The inode number. */
    RTINODE                 Ino;
    /** The size of the primary stream. */
    RTFOFF                  cbPrimary;
} VBOXFUSENODE;
typedef VBOXFUSENODE *PVBOXFUSENODE;

/**
 * A flat image file.
 */
typedef struct VBOXFUSEFLATIMAGE
{
    /** The standard bits. */
    VBOXFUSENODE            Node;
    /** The virtual disk container. */
    PVBOXHDD                pDisk;
    /** The format name. */
    char                   *pszFormat;
    /** The number of readers.
     * Read only images will have this set to INT32_MAX/2 on creation. */
    int32_t                 cReaders;
    /** The number of writers. (Just 1 or 0 really.) */
    int32_t                 cWriters;
} VBOXFUSEFLATIMAGE;
typedef VBOXFUSEFLATIMAGE *PVBOXFUSEFLATIMAGE;

/**
 * A control pipe (file).
 */
typedef struct VBOXFUSECTRLPIPE
{
    /** The standard bits. */
    VBOXFUSENODE            Node;
} VBOXFUSECTRLPIPE;
typedef VBOXFUSECTRLPIPE *PVBOXFUSECTRLPIPE;


/**
 * A Directory.
 *
 * This is just a container of files and subdirectories, nothing special.
 */
typedef struct VBOXFUSEDIR
{
    /** The standard bits. */
    VBOXFUSENODE            Node;
    /** The number of directory entries. */
    uint32_t                cEntries;
    /** Array of pointers to directory entries.
     * Whether what's being pointed to is a file, directory or something else can be
     * determined by the enmType field. */
    PVBOXFUSENODE          *paEntries;
} VBOXFUSEDIR;
typedef VBOXFUSEDIR *PVBOXFUSEDIR;

/** The number of elements to grow VBOXFUSEDIR::paEntries by. */
#define VBOXFUSE_DIR_GROW_BY    2 /* 32 */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The root of the file hierarchy. */
static VBOXFUSEDIR     *g_pTreeRoot;
/** The next inode number. */
static RTINODE volatile g_NextIno = 1;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int vboxfuseTreeLookupParent(const char *pszPath, const char **ppszName, PVBOXFUSEDIR *ppDir);
static int vboxfuseTreeLookupParentForInsert(const char *pszPath, const char **ppszName, PVBOXFUSEDIR *ppDir);


/**
 * Node destructor.
 *
 * @returns true.
 * @param   pNode           The node.
 * @param   fLocked         Whether it's locked.
 */
static bool vboxfuseNodeDestroy(PVBOXFUSENODE pNode, bool fLocked)
{
    Assert(pNode->cRefs == 0);

    /*
     * Type specific cleanups.
     */
    switch (pNode->enmType)
    {
        case VBOXFUSETYPE_DIRECTORY:
        {
            PVBOXFUSEDIR pDir = (PVBOXFUSEDIR)pNode;
            RTMemFree(pDir->paEntries);
            pDir->paEntries = NULL;
            pDir->cEntries = 0;
            break;
        }

        case VBOXFUSETYPE_FLAT_IMAGE:
        {
            PVBOXFUSEFLATIMAGE pFlatImage = (PVBOXFUSEFLATIMAGE)pNode;
            if (pFlatImage->pDisk)
            {
                int rc2 = VDClose(pFlatImage->pDisk, false /* fDelete */); AssertRC(rc2);
                pFlatImage->pDisk = NULL;
            }
            RTStrFree(pFlatImage->pszFormat);
            pFlatImage->pszFormat = NULL;
            break;
        }

        case VBOXFUSETYPE_CONTROL_PIPE:
            break;

        default:
            AssertMsgFailed(("%d\n", pNode->enmType));
            break;
    }

    /*
     * Generic cleanup.
     */
    pNode->enmType = VBOXFUSETYPE_INVALID;
    pNode->pszName = NULL;

    /*
     * Unlock and destroy the lock, before we finally frees the node.
     */
    if (fLocked)
        RTCritSectLeave(&pNode->CritSect);
    RTCritSectDelete(&pNode->CritSect);

    RTMemFree(pNode);

    return true;
}


/**
 * Locks a FUSE node.
 *
 * @param   pNode   The node.
 */
static void vboxfuseNodeLock(PVBOXFUSENODE pNode)
{
    int rc = RTCritSectEnter(&pNode->CritSect);
    AssertRC(rc);
}


/**
 * Unlocks a FUSE node.
 *
 * @param   pNode   The node.
 */
static void vboxfuseNodeUnlock(PVBOXFUSENODE pNode)
{
    int rc = RTCritSectLeave(&pNode->CritSect);
    AssertRC(rc);
}


/**
 * Retain a VBoxFUSE node.
 *
 * @param   pNode   The node.
 */
static void vboxfuseNodeRetain(PVBOXFUSENODE pNode)
{
    int32_t cNewRefs = ASMAtomicIncS32(&pNode->cRefs);
    Assert(cNewRefs != 1);
}


/**
 * Releases a VBoxFUSE node reference.
 *
 * @returns true if deleted, false if not.
 * @param   pNode   The node.
 */
static bool vboxfuseNodeRelease(PVBOXFUSENODE pNode)
{
    if (ASMAtomicDecS32(&pNode->cRefs) == 0)
        return vboxfuseNodeDestroy(pNode, false /* fLocked */);
    return false;
}


/**
 * Locks and retains a VBoxFUSE node.
 *
 * @param   pNode   The node.
 */
static void vboxfuseNodeLockAndRetain(PVBOXFUSENODE pNode)
{
    vboxfuseNodeLock(pNode);
    vboxfuseNodeRetain(pNode);
}


/**
 * Releases a VBoxFUSE node reference and unlocks it.
 *
 * @returns true if deleted, false if not.
 * @param   pNode   The node.
 */
static bool vboxfuseNodeReleaseAndUnlock(PVBOXFUSENODE pNode)
{
    if (ASMAtomicDecS32(&pNode->cRefs) == 0)
        return vboxfuseNodeDestroy(pNode, true /* fLocked */);
    vboxfuseNodeUnlock(pNode);
    return false;
}


/**
 * Creates stat info for a locked node.
 *
 * @param   pNode   The node (locked).
 */
static void vboxfuseNodeFillStat(PVBOXFUSENODE pNode, struct stat *pStat)
{
    pStat->st_dev       = 0;                /* ignored */
    pStat->st_ino       = pNode->Ino;       /* maybe ignored */
    pStat->st_mode      = pNode->fMode;
    pStat->st_nlink     = pNode->cLinks;
    pStat->st_uid       = pNode->Uid;
    pStat->st_gid       = pNode->Gid;
    pStat->st_rdev      = 0;
    /** @todo file times  */
    pStat->st_atime     = 0;
//    pStat->st_atimensec = 0;
    pStat->st_mtime     = 0;
//    pStat->st_mtimensec = 0;
    pStat->st_ctime     = 0;
//    pStat->st_ctimensec = 0;
    pStat->st_size      = pNode->cbPrimary;
    pStat->st_blocks    = (pNode->cbPrimary + DEV_BSIZE - 1) / DEV_BSIZE;
    pStat->st_blksize   = 0x1000;           /* ignored */
#ifndef RT_OS_LINUX
    pStat->st_flags     = 0;
    pStat->st_gen       = 0;
#endif
}


/**
 * Allocates a new node and initialize the node part of it.
 *
 * The returned node has one reference.
 *
 * @returns VBox status code.
 *
 * @param   cbNode      The size of the node.
 * @param   pszName     The name of the node.
 * @param   enmType     The node type.
 * @param   pDir        The directory (parent).
 * @param   ppNode      Where to return the pointer to the node.
 */
static int vboxfuseNodeAlloc(size_t cbNode, const char *pszName, VBOXFUSETYPE enmType, PVBOXFUSEDIR pDir,
                             PVBOXFUSENODE *ppNode)
{
    Assert(cbNode >= sizeof(VBOXFUSENODE));

    /*
     * Allocate the memory for it and init the critical section.
     */
    size_t cchName = strlen(pszName);
    PVBOXFUSENODE pNode = (PVBOXFUSENODE)RTMemAlloc(cchName + 1 + RT_ALIGN_Z(cbNode, 8));
    if (!pNode)
        return VERR_NO_MEMORY;

    int rc = RTCritSectInit(&pNode->CritSect);
    if (RT_FAILURE(rc))
    {
        RTMemFree(pNode);
        return rc;
    }

    /*
     * Initialize the members.
     */
    pNode->pszName = (char *)memcpy((uint8_t *)pNode + RT_ALIGN_Z(cbNode, 8), pszName, cchName + 1);
    pNode->cchName = cchName;
    pNode->enmType = enmType;
    pNode->cRefs   = 1;
    pNode->pDir    = pDir;
#if 0
    pNode->fMode   = enmType == VBOXFUSETYPE_DIRECTORY ? S_IFDIR | 0755 : S_IFREG | 0644;
#else
    pNode->fMode   = enmType == VBOXFUSETYPE_DIRECTORY ? S_IFDIR | 0777 : S_IFREG | 0666;
#endif
    pNode->Uid     = 0;
    pNode->Gid     = 0;
    pNode->cLinks  = 0;
    pNode->Ino     = g_NextIno++; /** @todo make this safe! */
    pNode->cbPrimary = 0;

    *ppNode = pNode;
    return VINF_SUCCESS;
}


/**
 * Inserts a node into a directory
 *
 * The caller has locked and referenced the directory as well as checked that
 * the name doesn't already exist within it. On success both the reference and
 * and link counters will be incremented.
 *
 * @returns VBox status code.
 *
 * @param   pDir        The parent directory. Can be NULL when creating the root
 *                      directory.
 * @param   pNode       The node to insert.
 */
static int vboxfuseDirInsertChild(PVBOXFUSEDIR pDir, PVBOXFUSENODE pNode)
{
    if (!pDir)
    {
        /*
         * Special case: Root Directory.
         */
        AssertReturn(!g_pTreeRoot, VERR_ALREADY_EXISTS);
        AssertReturn(pNode->enmType == VBOXFUSETYPE_DIRECTORY, VERR_INTERNAL_ERROR);
        g_pTreeRoot = (PVBOXFUSEDIR)pNode;
    }
    else
    {
        /*
         * Common case.
         */
        if (!(pDir->cEntries % VBOXFUSE_DIR_GROW_BY))
        {
            void *pvNew = RTMemRealloc(pDir->paEntries, sizeof(*pDir->paEntries) * (pDir->cEntries + VBOXFUSE_DIR_GROW_BY));
            if (!pvNew)
                return VERR_NO_MEMORY;
            pDir->paEntries = (PVBOXFUSENODE *)pvNew;
        }
        pDir->paEntries[pDir->cEntries++] = pNode;
        pDir->Node.cLinks++;
    }

    vboxfuseNodeRetain(pNode);
    pNode->cLinks++;
    return VINF_SUCCESS;
}


/**
 * Create a directory node.
 *
 * @returns VBox status code.
 * @param   pszPath     The path to the directory.
 * @param   ppDir       Optional, where to return the new directory locked and
 *                      referenced (making cRefs == 2).
 */
static int vboxfuseDirCreate(const char *pszPath, PVBOXFUSEDIR *ppDir)
{
    /*
     * Figure out where the directory is going.
     */
    const char *pszName;
    PVBOXFUSEDIR pParent;
    int rc = vboxfuseTreeLookupParentForInsert(pszPath, &pszName, &pParent);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Allocate and initialize the new directory node.
     */
    PVBOXFUSEDIR pNewDir;
    rc = vboxfuseNodeAlloc(sizeof(*pNewDir), pszName, VBOXFUSETYPE_DIRECTORY, pParent, (PVBOXFUSENODE *)&pNewDir);
    if (RT_SUCCESS(rc))
    {
        pNewDir->cEntries  = 0;
        pNewDir->paEntries = NULL;

        /*
         * Insert it.
         */
        rc = vboxfuseDirInsertChild(pParent, &pNewDir->Node);
        if (    RT_SUCCESS(rc)
            &&  ppDir)
        {
            vboxfuseNodeLockAndRetain(&pNewDir->Node);
            *ppDir = pNewDir;
        }
        vboxfuseNodeRelease(&pNewDir->Node);
    }
    if (pParent)
        vboxfuseNodeReleaseAndUnlock(&pParent->Node);
    return rc;
}


/**
 * Creates a flattened image
 *
 * @returns VBox status code.
 * @param   pszPath         Where to create the flattened file in the FUSE file
 *                          system.
 * @param   pszImage        The image to flatten.
 * @param   ppFile          Where to return the pointer to the instance.
 *                          Optional.
 */
static int vboxfuseFlatImageCreate(const char *pszPath, const char *pszImage, PVBOXFUSEFLATIMAGE *ppFile)
{
    /*
     * Check that we can create this file.
     */
    const char *pszName;
    PVBOXFUSEDIR pParent;
    int rc = vboxfuseTreeLookupParentForInsert(pszPath, &pszName, &pParent);
    if (RT_FAILURE(rc))
        return rc;
    if (pParent)
        vboxfuseNodeReleaseAndUnlock(&pParent->Node);

    /*
     * Try open the image file (without holding any locks).
     */
    char *pszFormat;
    VDTYPE enmType;
    rc = VDGetFormat(NULL /* pVDIIfsDisk */, NULL /* pVDIIfsImage*/, pszImage, &pszFormat, &enmType);
    if (RT_FAILURE(rc))
    {
        LogRel(("VDGetFormat(%s,) failed, rc=%Rrc\n", pszImage, rc));
        return rc;
    }

    PVBOXHDD pDisk = NULL;
    rc = VDCreate(NULL /* pVDIIfsDisk */, enmType, &pDisk);
    if (RT_SUCCESS(rc))
    {
        rc = VDOpen(pDisk, pszFormat, pszImage, 0, NULL /* pVDIfsImage */);
        if (RT_FAILURE(rc))
        {
            LogRel(("VDCreate(,%s,%s,,,) failed, rc=%Rrc\n", pszFormat, pszImage, rc));
            VDClose(pDisk, false /* fDeletes */);
        }
    }
    else
        Log(("VDCreate failed, rc=%Rrc\n", rc));
    if (RT_FAILURE(rc))
    {
        RTStrFree(pszFormat);
        return rc;
    }

    /*
     * Allocate and initialize the new directory node.
     */
    rc = vboxfuseTreeLookupParentForInsert(pszPath, &pszName, &pParent);
    if (RT_SUCCESS(rc))
    {
        PVBOXFUSEFLATIMAGE pNewFlatImage;
        rc = vboxfuseNodeAlloc(sizeof(*pNewFlatImage), pszName, VBOXFUSETYPE_FLAT_IMAGE, pParent, (PVBOXFUSENODE *)&pNewFlatImage);
        if (RT_SUCCESS(rc))
        {
            pNewFlatImage->pDisk          = pDisk;
            pNewFlatImage->pszFormat      = pszFormat;
            pNewFlatImage->cReaders       = VDIsReadOnly(pNewFlatImage->pDisk) ? INT32_MAX / 2 : 0;
            pNewFlatImage->cWriters       = 0;
            pNewFlatImage->Node.cbPrimary = VDGetSize(pNewFlatImage->pDisk, 0 /* base */);

            /*
             * Insert it.
             */
            rc = vboxfuseDirInsertChild(pParent, &pNewFlatImage->Node);
            if (    RT_SUCCESS(rc)
                &&  ppFile)
            {
                vboxfuseNodeLockAndRetain(&pNewFlatImage->Node);
                *ppFile = pNewFlatImage;
            }
            vboxfuseNodeRelease(&pNewFlatImage->Node);
            pDisk = NULL;
        }
        if (pParent)
            vboxfuseNodeReleaseAndUnlock(&pParent->Node);
    }
    if (RT_FAILURE(rc) && pDisk != NULL)
        VDClose(pDisk, false /* fDelete */);
    return rc;
}


//static int vboxfuseTreeMkCtrlPipe(const char *pszPath, PVBOXFUSECTRLPIPE *ppPipe)
//{
//}


/**
 * Looks up a file in the tree.
 *
 * Upon successful return the returned node is both referenced and locked. The
 * call will have to release and unlock it.
 *
 * @returns VBox status code
 * @param   pszPath     The path to the file.
 * @param   ppNode      Where to return the node.
 */
static int vboxfuseTreeLookup(const char *pszPath, PVBOXFUSENODE *ppNode)
{
    /*
     * Root first.
     */
    const char *psz = pszPath;
    if (*psz != '/')
        return VERR_FILE_NOT_FOUND;

    PVBOXFUSEDIR pDir = g_pTreeRoot;
    vboxfuseNodeLockAndRetain(&pDir->Node);

    do psz++;
    while (*psz == '/');
    if (!*psz)
    {
        /* looking for the root. */
        *ppNode = &pDir->Node;
        return VINF_SUCCESS;
    }

    /*
     * Take it bit by bit from here on.
     */
    for (;;)
    {
        /*
         * Find the length of the current directory entry and check if it must be file.
         */
        const char * const pszName = psz;
        psz = strchr(psz, '/');
        if (!psz)
            psz = strchr(pszName, '\0');
        size_t cchName = psz - pszName;

        bool fMustBeDir = *psz == '/';
        while (*psz == '/')
            psz++;

        /*
         * Look it up.
         * This is safe as the directory will hold a reference to each node
         * so the nodes cannot possibly be destroyed while we're searching them.
         */
        PVBOXFUSENODE   pNode = NULL;
        uint32_t        i = pDir->cEntries;
        PVBOXFUSENODE  *paEntries = pDir->paEntries;
        while (i-- > 0)
        {
            PVBOXFUSENODE pCur = paEntries[i];
            if (    pCur->cchName == cchName
                &&  !memcmp(pCur->pszName, pszName, cchName))
            {
                pNode = pCur;
                vboxfuseNodeLockAndRetain(pNode);
                break;
            }
        }
        vboxfuseNodeReleaseAndUnlock(&pDir->Node);

        if (!pNode)
            return *psz ? VERR_PATH_NOT_FOUND : VERR_FILE_NOT_FOUND;
        if (    fMustBeDir
            &&  pNode->enmType != VBOXFUSETYPE_DIRECTORY)
        {
            vboxfuseNodeReleaseAndUnlock(pNode);
            return VERR_NOT_A_DIRECTORY;
        }

        /*
         * Are we done?
         */
        if (!*psz)
        {
            *ppNode = pNode;
            return VINF_SUCCESS;
        }

        /* advance */
        pDir = (PVBOXFUSEDIR)pNode;
    }
}


/**
 * Errno conversion wrapper around vboxfuseTreeLookup().
 *
 * @returns 0 on success, negated errno on failure.
 * @param   pszPath     The path to the file.
 * @param   ppNode      Where to return the node.
 */
static int vboxfuseTreeLookupErrno(const char *pszPath, PVBOXFUSENODE *ppNode)
{
    int rc = vboxfuseTreeLookup(pszPath, ppNode);
    if (RT_SUCCESS(rc))
        return 0;
    return -RTErrConvertToErrno(rc);
}


/**
 * Looks up a parent directory in the tree.
 *
 * Upon successful return the returned directory is both referenced and locked.
 * The call will have to release and unlock it.
 *
 * @returns VBox status code.
 *
 * @param   pszPath     The path to the file which parent we seek.
 * @param   ppszName    Where to return the pointer to the child's name within
 *                      pszPath.
 * @param   ppDir       Where to return the parent directory.
 */
static int vboxfuseTreeLookupParent(const char *pszPath, const char **ppszName, PVBOXFUSEDIR *ppDir)
{
    /*
     * Root first.
     */
    const char *psz = pszPath;
    if (*psz != '/')
        return VERR_INVALID_PARAMETER;
    do psz++;
    while (*psz == '/');
    if (!*psz)
    {
        /* looking for the root. */
        *ppszName = psz + 1;
        *ppDir = NULL;
        return VINF_SUCCESS;
    }

    /*
     * Take it bit by bit from here on.
     */
    PVBOXFUSEDIR pDir = g_pTreeRoot;
    AssertReturn(pDir, VERR_WRONG_ORDER);
    vboxfuseNodeLockAndRetain(&pDir->Node);
    for (;;)
    {
        /*
         * Find the length of the current directory entry and check if it must be file.
         */
        const char * const pszName = psz;
        psz = strchr(psz, '/');
        if (!psz)
        {
            /* that's all folks.*/
            *ppszName = pszName;
            *ppDir = pDir;
            return VINF_SUCCESS;
        }
        size_t cchName = psz - pszName;

        bool fMustBeDir = *psz == '/';
        while (*psz == '/')
            psz++;

        /* Trailing slashes are not allowed (because it's simpler without them). */
        if (!*psz)
            return VERR_INVALID_PARAMETER;

        /*
         * Look it up.
         * This is safe as the directory will hold a reference to each node
         * so the nodes cannot possibly be destroyed while we're searching them.
         */
        PVBOXFUSENODE   pNode = NULL;
        uint32_t        i = pDir->cEntries;
        PVBOXFUSENODE  *paEntries = pDir->paEntries;
        while (i-- > 0)
        {
            PVBOXFUSENODE pCur = paEntries[i];
            if (    pCur->cchName == cchName
                &&  !memcmp(pCur->pszName, pszName, cchName))
            {
                pNode = pCur;
                vboxfuseNodeLockAndRetain(pNode);
                break;
            }
        }
        vboxfuseNodeReleaseAndUnlock(&pDir->Node);

        if (!pNode)
            return VERR_FILE_NOT_FOUND;
        if (    fMustBeDir
            &&  pNode->enmType != VBOXFUSETYPE_DIRECTORY)
        {
            vboxfuseNodeReleaseAndUnlock(pNode);
            return VERR_PATH_NOT_FOUND;
        }

        /* advance */
        pDir = (PVBOXFUSEDIR)pNode;
    }
}


/**
 * Looks up a parent directory in the tree and checking that the specified child
 * doesn't already exist.
 *
 * Upon successful return the returned directory is both referenced and locked.
 * The call will have to release and unlock it.
 *
 * @returns VBox status code.
 *
 * @param   pszPath     The path to the file which parent we seek.
 * @param   ppszName    Where to return the pointer to the child's name within
 *                      pszPath.
 * @param   ppDir       Where to return the parent directory.
 */
static int vboxfuseTreeLookupParentForInsert(const char *pszPath, const char **ppszName, PVBOXFUSEDIR *ppDir)
{
    /*
     * Lookup the parent directory using vboxfuseTreeLookupParent first.
     */
    const char     *pszName;
    PVBOXFUSEDIR    pDir;
    int rc = vboxfuseTreeLookupParent(pszPath, &pszName, &pDir);
    if (RT_SUCCESS(rc))
    {
        /*
         * Check that it doesn't exist already
         */
        if (pDir)
        {
            size_t const    cchName   = strlen(pszName);
            uint32_t        i         = pDir->cEntries;
            PVBOXFUSENODE  *paEntries = pDir->paEntries;
            while (i-- > 0)
            {
                PVBOXFUSENODE pCur = paEntries[i];
                if (    pCur->cchName == cchName
                    &&  !memcmp(pCur->pszName, pszName, cchName))
                {
                    vboxfuseNodeReleaseAndUnlock(&pDir->Node);
                    rc = VERR_ALREADY_EXISTS;
                    break;
                }
            }
        }
        if (RT_SUCCESS(rc))
        {
            *ppDir = pDir;
            *ppszName = pszName;
        }
    }
    return rc;
}





/** @copydoc fuse_operations::getattr */
static int vboxfuseOp_getattr(const char *pszPath, struct stat *pStat)
{
    PVBOXFUSENODE pNode;
    int rc = vboxfuseTreeLookupErrno(pszPath, &pNode);
    if (!rc)
    {
        vboxfuseNodeFillStat(pNode, pStat);
        vboxfuseNodeReleaseAndUnlock(pNode);
    }
    LogFlow(("vboxfuseOp_getattr: rc=%d \"%s\"\n", rc, pszPath));
    return rc;
}


/** @copydoc fuse_operations::opendir */
static int vboxfuseOp_opendir(const char *pszPath, struct fuse_file_info *pInfo)
{
    PVBOXFUSENODE pNode;
    int rc = vboxfuseTreeLookupErrno(pszPath, &pNode);
    if (!rc)
    {
        /*
         * Check that it's a directory and that the caller should see it.
         */
        if (pNode->enmType != VBOXFUSETYPE_DIRECTORY)
            rc = -ENOTDIR;
        /** @todo access checks. */
        else
        {
            /** @todo update the accessed TS?    */

            /*
             * Put a reference to the node in the fuse_file_info::fh member so
             * we don't have to parse the path in readdir.
             */
            pInfo->fh = (uintptr_t)pNode;
            vboxfuseNodeUnlock(pNode);
        }

        /* cleanup */
        if (rc)
            vboxfuseNodeReleaseAndUnlock(pNode);
    }
    LogFlow(("vboxfuseOp_opendir: rc=%d \"%s\"\n", rc, pszPath));
    return rc;
}


/** @copydoc fuse_operations::readdir */
static int vboxfuseOp_readdir(const char *pszPath, void *pvBuf, fuse_fill_dir_t pfnFiller,
                              off_t offDir, struct fuse_file_info *pInfo)
{
    PVBOXFUSEDIR pDir = (PVBOXFUSEDIR)(uintptr_t)pInfo->fh;
    AssertPtr(pDir);
    Assert(pDir->Node.enmType == VBOXFUSETYPE_DIRECTORY);
    vboxfuseNodeLock(&pDir->Node);
    LogFlow(("vboxfuseOp_readdir: offDir=%llx \"%s\"\n", (uint64_t)offDir, pszPath));

#define VBOXFUSE_FAKE_DIRENT_SIZE   512

    /*
     * First the mandatory dot and dot-dot entries.
     */
    struct stat st;
    int rc = 0;
    if (!offDir)
    {
        offDir += VBOXFUSE_FAKE_DIRENT_SIZE;
        vboxfuseNodeFillStat(&pDir->Node, &st);
        rc = pfnFiller(pvBuf, ".", &st, offDir);
    }
    if (    offDir == VBOXFUSE_FAKE_DIRENT_SIZE
        &&  !rc)
    {
        offDir += VBOXFUSE_FAKE_DIRENT_SIZE;
        rc = pfnFiller(pvBuf, "..", NULL, offDir);
    }

    /*
     * Convert the offset to a directory index and start/continue filling the buffer.
     * The entries only needs locking since the directory already has a reference
     * to each of them.
     */
    Assert(offDir >= VBOXFUSE_FAKE_DIRENT_SIZE * 2 || rc);
    uint32_t i = offDir / VBOXFUSE_FAKE_DIRENT_SIZE - 2;
    while (    !rc
           &&  i < pDir->cEntries)
    {
        PVBOXFUSENODE pNode = pDir->paEntries[i];
        vboxfuseNodeLock(pNode);

        vboxfuseNodeFillStat(pNode, &st);
        offDir = (i + 3) * VBOXFUSE_FAKE_DIRENT_SIZE;
        rc = pfnFiller(pvBuf, pNode->pszName, &st, offDir);

        vboxfuseNodeUnlock(pNode);

        /* next */
        i++;
    }

    vboxfuseNodeUnlock(&pDir->Node);
    LogFlow(("vboxfuseOp_readdir: returns offDir=%llx\n", (uint64_t)offDir));
    return 0;
}


/** @copydoc fuse_operations::releasedir */
static int vboxfuseOp_releasedir(const char *pszPath, struct fuse_file_info *pInfo)
{
    PVBOXFUSEDIR pDir = (PVBOXFUSEDIR)(uintptr_t)pInfo->fh;
    AssertPtr(pDir);
    Assert(pDir->Node.enmType == VBOXFUSETYPE_DIRECTORY);
    pInfo->fh = 0;
    vboxfuseNodeRelease(&pDir->Node);
    LogFlow(("vboxfuseOp_releasedir: \"%s\"\n", pszPath));
    return 0;
}


/** @copydoc fuse_operations::symlink */
static int vboxfuseOp_symlink(const char *pszDst, const char *pszPath)
{
    /*
     * "Interface" for mounting a image.
     */
    int rc = vboxfuseFlatImageCreate(pszPath, pszDst, NULL);
    if (RT_SUCCESS(rc))
    {
        Log(("vboxfuseOp_symlink: \"%s\" => \"%s\" SUCCESS!\n", pszPath, pszDst));
        return 0;
    }

    LogFlow(("vboxfuseOp_symlink: \"%s\" => \"%s\" rc=%Rrc\n", pszPath, pszDst, rc));
    return -RTErrConvertToErrno(rc);
}


/** @copydoc fuse_operations::open */
static int vboxfuseOp_open(const char *pszPath, struct fuse_file_info *pInfo)
{
    LogFlow(("vboxfuseOp_open(\"%s\", .flags=%#x)\n", pszPath, pInfo->flags));

    /*
     * Validate incoming flags.
     */
#ifdef RT_OS_DARWIN
    if (pInfo->flags & (O_APPEND | O_NONBLOCK | O_SYMLINK | O_NOCTTY | O_SHLOCK | O_EXLOCK | O_ASYNC
                        | O_CREAT | O_TRUNC | O_EXCL | O_EVTONLY))
        return -EINVAL;
    if ((pInfo->flags & O_ACCMODE) == O_ACCMODE)
        return -EINVAL;
#elif defined(RT_OS_LINUX)
    if (pInfo->flags & (  O_APPEND | O_ASYNC | O_DIRECT /* | O_LARGEFILE ? */
                        | O_NOATIME | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK
                        /* | O_SYNC ? */))
        return -EINVAL;
    if ((pInfo->flags & O_ACCMODE) == O_ACCMODE)
        return -EINVAL;
#elif defined(RT_OS_FREEBSD)
    if (pInfo->flags & (  O_APPEND | O_ASYNC | O_DIRECT /* | O_LARGEFILE ? */
                        | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK
                        /* | O_SYNC ? */))
        return -EINVAL;
    if ((pInfo->flags & O_ACCMODE) == O_ACCMODE)
        return -EINVAL;
#else
# error "Port me"
#endif

    PVBOXFUSENODE pNode;
    int rc = vboxfuseTreeLookupErrno(pszPath, &pNode);
    if (!rc)
    {
        /*
         * Check flags and stuff.
         */
        switch (pNode->enmType)
        {
            /* not expected here? */
            case VBOXFUSETYPE_DIRECTORY:
                AssertFailed();
                rc = -EISDIR;
                break;

            case VBOXFUSETYPE_FLAT_IMAGE:
            {
                PVBOXFUSEFLATIMAGE pFlatImage = (PVBOXFUSEFLATIMAGE)pNode;
#ifdef O_DIRECTORY
                if (pInfo->flags & O_DIRECTORY)
                    rc = -ENOTDIR;
#endif
                if (    (pInfo->flags & O_ACCMODE) == O_WRONLY
                    ||  (pInfo->flags & O_ACCMODE) == O_RDWR)
                {
                    if (    pFlatImage->cWriters == 0
                        &&  pFlatImage->cReaders == 0)
                        pFlatImage->cWriters++;
                    else
                        rc = -ETXTBSY;
                }
                else if ((pInfo->flags & O_ACCMODE) == O_RDONLY)
                {
                    if (pFlatImage->cWriters == 0)
                    {
                        if (pFlatImage->cReaders + 1 < (  pFlatImage->cReaders < INT32_MAX / 2
                                                        ? INT32_MAX / 4
                                                        : INT32_MAX / 2 + INT32_MAX / 4) )
                            pFlatImage->cReaders++;
                        else
                            rc = -EMLINK;
                    }
                    else
                        rc = -ETXTBSY;
                }
                break;
            }

            case VBOXFUSETYPE_CONTROL_PIPE:
                rc = -ENOTSUP;
                break;

            default:
                rc = -EDOOFUS;
                break;
        }
        if (!rc)
        {
            /*
             * Put a reference to the node in the fuse_file_info::fh member so
             * we don't have to parse the path in the other file methods.
             */
            pInfo->fh = (uintptr_t)pNode;
            vboxfuseNodeUnlock(pNode);
        }
        else
        {
            /* cleanup */
            vboxfuseNodeReleaseAndUnlock(pNode);
        }
    }
    LogFlow(("vboxfuseOp_opendir: rc=%d \"%s\"\n", rc, pszPath));
    return rc;
}


/** @copydoc fuse_operations::release */
static int vboxfuseOp_release(const char *pszPath, struct fuse_file_info *pInfo)
{
    PVBOXFUSENODE pNode = (PVBOXFUSENODE)(uintptr_t)pInfo->fh;
    AssertPtr(pNode);
    pInfo->fh = 0;

    switch (pNode->enmType)
    {
        case VBOXFUSETYPE_DIRECTORY:
            /* nothing to do */
            vboxfuseNodeRelease(pNode);
            break;

        case VBOXFUSETYPE_FLAT_IMAGE:
        {
            PVBOXFUSEFLATIMAGE pFlatImage = (PVBOXFUSEFLATIMAGE)pNode;
            vboxfuseNodeLock(&pFlatImage->Node);

            if (    (pInfo->flags & O_ACCMODE) == O_WRONLY
                ||  (pInfo->flags & O_ACCMODE) == O_RDWR)
            {
                pFlatImage->cWriters--;
                Assert(pFlatImage->cWriters >= 0);
            }
            else if ((pInfo->flags & O_ACCMODE) == O_RDONLY)
            {
                pFlatImage->cReaders--;
                Assert(pFlatImage->cReaders >= 0);
            }
            else
                AssertFailed();

            vboxfuseNodeReleaseAndUnlock(&pFlatImage->Node);
            break;
        }

        case VBOXFUSETYPE_CONTROL_PIPE:
            /* nothing to do yet */
            vboxfuseNodeRelease(pNode);
            break;

        default:
            AssertMsgFailed(("%s\n", pszPath));
            return -EDOOFUS;
    }

    LogFlow(("vboxfuseOp_release: \"%s\"\n", pszPath));
    return 0;
}

/** The VDRead/VDWrite block granularity. */
#define VBOXFUSE_MIN_SIZE               512
/** Offset mask corresponding to VBOXFUSE_MIN_SIZE. */
#define VBOXFUSE_MIN_SIZE_MASK_OFF      (0x1ff)
/** Block mask corresponding to VBOXFUSE_MIN_SIZE. */
#define VBOXFUSE_MIN_SIZE_MASK_BLK      (~UINT64_C(0x1ff))

/** @copydoc fuse_operations::read */
static int vboxfuseOp_read(const char *pszPath, char *pbBuf, size_t cbBuf,
                           off_t offFile, struct fuse_file_info *pInfo)
{
    /* paranoia */
    AssertReturn((int)cbBuf >= 0, -EINVAL);
    AssertReturn((unsigned)cbBuf == cbBuf, -EINVAL);
    AssertReturn(offFile >= 0, -EINVAL);
    AssertReturn((off_t)(offFile + cbBuf) >= offFile, -EINVAL);

    PVBOXFUSENODE pNode = (PVBOXFUSENODE)(uintptr_t)pInfo->fh;
    AssertPtr(pNode);
    switch (pNode->enmType)
    {
        case VBOXFUSETYPE_DIRECTORY:
            return -ENOTSUP;

        case VBOXFUSETYPE_FLAT_IMAGE:
        {
            PVBOXFUSEFLATIMAGE pFlatImage = (PVBOXFUSEFLATIMAGE)(uintptr_t)pInfo->fh;
            LogFlow(("vboxfuseOp_read: offFile=%#llx cbBuf=%#zx pszPath=\"%s\"\n", (uint64_t)offFile, cbBuf, pszPath));
            vboxfuseNodeLock(&pFlatImage->Node);

            int rc;
            if ((off_t)(offFile + cbBuf) < offFile)
                rc = -EINVAL;
            else if (offFile >= pFlatImage->Node.cbPrimary)
                rc = 0;
            else if (!cbBuf)
                rc = 0;
            else
            {
                /* Adjust for EOF. */
                if ((off_t)(offFile + cbBuf) >= pFlatImage->Node.cbPrimary)
                    cbBuf = pFlatImage->Node.cbPrimary - offFile;

                /*
                 * Aligned read?
                 */
                int rc2;
                if (    !(offFile & VBOXFUSE_MIN_SIZE_MASK_OFF)
                    &&  !(cbBuf   & VBOXFUSE_MIN_SIZE_MASK_OFF))
                    rc2 = VDRead(pFlatImage->pDisk, offFile, pbBuf, cbBuf);
                else
                {
                    /*
                     * Unaligned read - lots of extra work.
                     */
                    uint8_t abBlock[VBOXFUSE_MIN_SIZE];
                    if (((offFile + cbBuf) & VBOXFUSE_MIN_SIZE_MASK_BLK) == (offFile & VBOXFUSE_MIN_SIZE_MASK_BLK))
                    {
                        /* a single partial block. */
                        rc2 = VDRead(pFlatImage->pDisk, offFile & VBOXFUSE_MIN_SIZE_MASK_BLK, abBlock, VBOXFUSE_MIN_SIZE);
                        if (RT_SUCCESS(rc2))
                            memcpy(pbBuf, &abBlock[offFile & VBOXFUSE_MIN_SIZE_MASK_OFF], cbBuf);
                    }
                    else
                    {
                        /* read unaligned head. */
                        rc2 = VINF_SUCCESS;
                        if (offFile & VBOXFUSE_MIN_SIZE_MASK_OFF)
                        {
                            rc2 = VDRead(pFlatImage->pDisk, offFile & VBOXFUSE_MIN_SIZE_MASK_BLK, abBlock, VBOXFUSE_MIN_SIZE);
                            if (RT_SUCCESS(rc2))
                            {
                                size_t cbCopy = VBOXFUSE_MIN_SIZE - (offFile & VBOXFUSE_MIN_SIZE_MASK_OFF);
                                memcpy(pbBuf, &abBlock[offFile & VBOXFUSE_MIN_SIZE_MASK_OFF], cbCopy);
                                pbBuf   += cbCopy;
                                offFile += cbCopy;
                                cbBuf   -= cbCopy;
                            }
                        }

                        /* read the middle. */
                        Assert(!(offFile & VBOXFUSE_MIN_SIZE_MASK_OFF));
                        if (cbBuf >= VBOXFUSE_MIN_SIZE && RT_SUCCESS(rc2))
                        {
                            size_t cbRead = cbBuf & VBOXFUSE_MIN_SIZE_MASK_BLK;
                            rc2 = VDRead(pFlatImage->pDisk, offFile, pbBuf, cbRead);
                            if (RT_SUCCESS(rc2))
                            {
                                pbBuf   += cbRead;
                                offFile += cbRead;
                                cbBuf   -= cbRead;
                            }
                        }

                        /* unaligned tail read. */
                        Assert(cbBuf < VBOXFUSE_MIN_SIZE);
                        Assert(!(offFile & VBOXFUSE_MIN_SIZE_MASK_OFF));
                        if (cbBuf && RT_SUCCESS(rc2))
                        {
                            rc2 = VDRead(pFlatImage->pDisk, offFile, abBlock, VBOXFUSE_MIN_SIZE);
                            if (RT_SUCCESS(rc2))
                                memcpy(pbBuf, &abBlock[0], cbBuf);
                        }
                    }
                }

                /* convert the return code */
                if (RT_SUCCESS(rc2))
                    rc = cbBuf;
                else
                    rc = -RTErrConvertToErrno(rc2);
            }

            vboxfuseNodeUnlock(&pFlatImage->Node);
            return rc;
        }

        case VBOXFUSETYPE_CONTROL_PIPE:
            return -ENOTSUP;

        default:
            AssertMsgFailed(("%s\n", pszPath));
            return -EDOOFUS;
    }
}


/** @copydoc fuse_operations::write */
static int vboxfuseOp_write(const char *pszPath, const char *pbBuf, size_t cbBuf,
                           off_t offFile, struct fuse_file_info *pInfo)
{
    /* paranoia */
    AssertReturn((int)cbBuf >= 0, -EINVAL);
    AssertReturn((unsigned)cbBuf == cbBuf, -EINVAL);
    AssertReturn(offFile >= 0, -EINVAL);
    AssertReturn((off_t)(offFile + cbBuf) >= offFile, -EINVAL);

    PVBOXFUSENODE pNode = (PVBOXFUSENODE)(uintptr_t)pInfo->fh;
    AssertPtr(pNode);
    switch (pNode->enmType)
    {
        case VBOXFUSETYPE_DIRECTORY:
            return -ENOTSUP;

        case VBOXFUSETYPE_FLAT_IMAGE:
        {
            PVBOXFUSEFLATIMAGE pFlatImage = (PVBOXFUSEFLATIMAGE)(uintptr_t)pInfo->fh;
            LogFlow(("vboxfuseOp_write: offFile=%#llx cbBuf=%#zx pszPath=\"%s\"\n", (uint64_t)offFile, cbBuf, pszPath));
            vboxfuseNodeLock(&pFlatImage->Node);

            int rc;
            if ((off_t)(offFile + cbBuf) < offFile)
                rc = -EINVAL;
            else if (offFile >= pFlatImage->Node.cbPrimary)
                rc = 0;
            else if (!cbBuf)
                rc = 0;
            else
            {
                /* Adjust for EOF. */
                if ((off_t)(offFile + cbBuf) >= pFlatImage->Node.cbPrimary)
                    cbBuf = pFlatImage->Node.cbPrimary - offFile;

                /*
                 * Aligned write?
                 */
                int rc2;
                if (    !(offFile & VBOXFUSE_MIN_SIZE_MASK_OFF)
                    &&  !(cbBuf   & VBOXFUSE_MIN_SIZE_MASK_OFF))
                    rc2 = VDWrite(pFlatImage->pDisk, offFile, pbBuf, cbBuf);
                else
                {
                    /*
                     * Unaligned write - lots of extra work.
                     */
                    uint8_t abBlock[VBOXFUSE_MIN_SIZE];
                    if (((offFile + cbBuf) & VBOXFUSE_MIN_SIZE_MASK_BLK) == (offFile & VBOXFUSE_MIN_SIZE_MASK_BLK))
                    {
                        /* a single partial block. */
                        rc2 = VDRead(pFlatImage->pDisk, offFile & VBOXFUSE_MIN_SIZE_MASK_BLK, abBlock, VBOXFUSE_MIN_SIZE);
                        if (RT_SUCCESS(rc2))
                        {
                            memcpy(&abBlock[offFile & VBOXFUSE_MIN_SIZE_MASK_OFF], pbBuf, cbBuf);
                            /* Update the block */
                            rc2 = VDWrite(pFlatImage->pDisk, offFile & VBOXFUSE_MIN_SIZE_MASK_BLK, abBlock, VBOXFUSE_MIN_SIZE);
                        }
                    }
                    else
                    {
                        /* read unaligned head. */
                        rc2 = VINF_SUCCESS;
                        if (offFile & VBOXFUSE_MIN_SIZE_MASK_OFF)
                        {
                            rc2 = VDRead(pFlatImage->pDisk, offFile & VBOXFUSE_MIN_SIZE_MASK_BLK, abBlock, VBOXFUSE_MIN_SIZE);
                            if (RT_SUCCESS(rc2))
                            {
                                size_t cbCopy = VBOXFUSE_MIN_SIZE - (offFile & VBOXFUSE_MIN_SIZE_MASK_OFF);
                                memcpy(&abBlock[offFile & VBOXFUSE_MIN_SIZE_MASK_OFF], pbBuf, cbCopy);
                                pbBuf   += cbCopy;
                                offFile += cbCopy;
                                cbBuf   -= cbCopy;
                                rc2 = VDWrite(pFlatImage->pDisk, offFile & VBOXFUSE_MIN_SIZE_MASK_BLK, abBlock, VBOXFUSE_MIN_SIZE);
                            }
                        }

                        /* write the middle. */
                        Assert(!(offFile & VBOXFUSE_MIN_SIZE_MASK_OFF));
                        if (cbBuf >= VBOXFUSE_MIN_SIZE && RT_SUCCESS(rc2))
                        {
                            size_t cbWrite = cbBuf & VBOXFUSE_MIN_SIZE_MASK_BLK;
                            rc2 = VDWrite(pFlatImage->pDisk, offFile, pbBuf, cbWrite);
                            if (RT_SUCCESS(rc2))
                            {
                                pbBuf   += cbWrite;
                                offFile += cbWrite;
                                cbBuf   -= cbWrite;
                            }
                        }

                        /* unaligned tail write. */
                        Assert(cbBuf < VBOXFUSE_MIN_SIZE);
                        Assert(!(offFile & VBOXFUSE_MIN_SIZE_MASK_OFF));
                        if (cbBuf && RT_SUCCESS(rc2))
                        {
                            rc2 = VDRead(pFlatImage->pDisk, offFile, abBlock, VBOXFUSE_MIN_SIZE);
                            if (RT_SUCCESS(rc2))
                            {
                                memcpy(&abBlock[0], pbBuf, cbBuf);
                                rc2 = VDWrite(pFlatImage->pDisk, offFile, abBlock, VBOXFUSE_MIN_SIZE);
                            }
                        }
                    }
                }

                /* convert the return code */
                if (RT_SUCCESS(rc2))
                    rc = cbBuf;
                else
                    rc = -RTErrConvertToErrno(rc2);
            }

            vboxfuseNodeUnlock(&pFlatImage->Node);
            return rc;
        }

        case VBOXFUSETYPE_CONTROL_PIPE:
            return -ENOTSUP;

        default:
            AssertMsgFailed(("%s\n", pszPath));
            return -EDOOFUS;
    }
}


/**
 * The FUSE operations.
 *
 * @remarks We'll initialize this manually since we cannot use C99 style
 *          initialzer designations in C++ (yet).
 */
static struct fuse_operations   g_vboxfuseOps;



int main(int argc, char **argv)
{
    /*
     * Initialize the runtime and VD.
     */
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
    {
        RTStrmPrintf(g_pStdErr, "VBoxFUSE: RTR3InitExe failed, rc=%Rrc\n", rc);
        return 1;
    }
    RTPrintf("VBoxFUSE: Hello...\n");
    rc = VDInit();
    if (RT_FAILURE(rc))
    {
        RTStrmPrintf(g_pStdErr, "VBoxFUSE: VDInit failed, rc=%Rrc\n", rc);
        return 1;
    }

    /*
     * Initializes the globals and populate the file hierarchy.
     */
    rc = vboxfuseDirCreate("/", NULL);
    if (RT_SUCCESS(rc))
        rc = vboxfuseDirCreate("/FlattenedImages", NULL);
    if (RT_FAILURE(rc))
    {
        RTStrmPrintf(g_pStdErr, "VBoxFUSE: vboxfuseDirCreate failed, rc=%Rrc\n", rc);
        return 1;
    }

    /*
     * Initialize the g_vboxfuseOps. (C++ sucks!)
     */
    memset(&g_vboxfuseOps, 0, sizeof(g_vboxfuseOps));
    g_vboxfuseOps.getattr    = vboxfuseOp_getattr;
    g_vboxfuseOps.opendir    = vboxfuseOp_opendir;
    g_vboxfuseOps.readdir    = vboxfuseOp_readdir;
    g_vboxfuseOps.releasedir = vboxfuseOp_releasedir;
    g_vboxfuseOps.symlink    = vboxfuseOp_symlink;
    g_vboxfuseOps.open       = vboxfuseOp_open;
    g_vboxfuseOps.read       = vboxfuseOp_read;
    g_vboxfuseOps.write      = vboxfuseOp_write;
    g_vboxfuseOps.release    = vboxfuseOp_release;

    /*
     * Hand control over to libfuse.
     */

#if 0
    /** @todo multithreaded fun. */
#else
    rc = fuse_main(argc, argv, &g_vboxfuseOps, NULL);
#endif
    RTPrintf("VBoxFUSE: fuse_main -> %d\n", rc);
    return rc;
}


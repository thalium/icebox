/* $Id: kRdrFile-iprt.cpp $ */
/** @file
 * IPRT - kRdr Backend.
 */

/*
 * Copyright (C) 2007-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kRdr.h>
#include <k/kRdrAll.h>
#include <k/kErrors.h>
#include <k/kMagics.h>

#include <iprt/file.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/mem.h>
#include <iprt/err.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Prepared stuff.
 */
typedef struct KRDRFILEPREP
{
    /** The address of the prepared region. */
    void           *pv;
    /** The size of the prepared region. */
    KSIZE           cb;
} KRDRFILEPREP, *PKRDRFILEPREP;

/**
 * The file provier instance for native files.
 */
typedef struct KRDRFILE
{
    /** The file reader vtable. */
    KRDR                Core;
    /** The file handle. */
    RTFILE              File;
    /** The current file offset. */
    KFOFF               off;
    /** The file size. */
    KFOFF               cb;
    /** Array where we stuff the mapping area data. */
    KRDRFILEPREP        aPreps[4];
    /** The number of current preps. */
    KU32                cPreps;
    /** Number of mapping references. */
    KI32                cMappings;
    /** The memory mapping. */
    void               *pvMapping;
    /** The filename. */
    char                szFilename[1];
} KRDRFILE, *PKRDRFILE;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void     krdrRTFileDone(PKRDR pRdr);
static int      krdrRTFileUnmap(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrRTFileGenericUnmap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrRTFileProtect(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect);
static int      krdrRTFileGenericProtect(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect);
static int      krdrRTFileRefresh(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrRTFileGenericRefresh(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrRTFileMap(PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed);
static int      krdrRTFileGenericMap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed);
static KSIZE   krdrRTFilePageSize(PKRDR pRdr);
static const char *krdrRTFileName(PKRDR pRdr);
static KIPTR    krdrRTFileNativeFH(PKRDR pRdr);
static KFOFF    krdrRTFileTell(PKRDR pRdr);
static KFOFF    krdrRTFileSize(PKRDR pRdr);
static int      krdrRTFileAllUnmap(PKRDR pRdr, const void *pvBits);
static int      krdrRTFileAllMap(PKRDR pRdr, const void **ppvBits);
static int      krdrRTFileRead(PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off);
static int      krdrRTFileDestroy(PKRDR pRdr);
static int      krdrRTFileCreate(PPKRDR ppRdr, const char *pszFilename);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
extern "C" const KRDROPS g_kRdrFileOps;

/** Native file provider operations. */
const KRDROPS g_kRdrFileOps =
{
    "IPRT file",
    NULL,
    krdrRTFileCreate,
    krdrRTFileDestroy,
    krdrRTFileRead,
    krdrRTFileAllMap,
    krdrRTFileAllUnmap,
    krdrRTFileSize,
    krdrRTFileTell,
    krdrRTFileName,
    krdrRTFileNativeFH,
    krdrRTFilePageSize,
    krdrRTFileMap,
    krdrRTFileRefresh,
    krdrRTFileProtect,
    krdrRTFileUnmap,
    krdrRTFileDone,
    42
};


/** @copydoc KRDROPS::pfnDone */
static void     krdrRTFileDone(PKRDR pRdr)
{
    K_NOREF(pRdr);
}


/**
 * Finds a prepared mapping region.
 *
 * @returns Pointer to the aPrep entry.
 * @param   pFile   The instance data.
 * @param   pv      The base of the region.
 */
static PKRDRFILEPREP krdrRTFileFindPrepExact(PKRDRFILE pFile, void *pv)
{
    KI32 i = pFile->cPreps;
    while (i-- > 0)
        if (pFile->aPreps[i].pv == pv)
            return &pFile->aPreps[i];
    return NULL;
}


/** @copydoc KRDROPS::pfnUnmap */
static int      krdrRTFileUnmap(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    PKRDRFILE        pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP       pPrep = krdrRTFileFindPrepExact(pRdrFile, pvBase);
    int                 rc;
    if (!pPrep)
        return KERR_INVALID_PARAMETER;

    rc = krdrRTFileGenericUnmap(pRdr, pPrep, cSegments, paSegments);

    /* remove the mapping data on success. */
    if (!rc)
    {
        pRdrFile->cPreps--;
        if (pPrep != &pRdrFile->aPreps[pRdrFile->cPreps])
            *pPrep = pRdrFile->aPreps[pRdrFile->cPreps];
    }
    return rc;
}


/** Generic implementation of krdrRTFileUnmap. */
static int krdrRTFileGenericUnmap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments)
{
    krdrRTFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 1 /* unprotect */);
    RTMemPageFree(pPrep->pv, pPrep->cb);
    return 0;
}


/** @copydoc KRDROPS::pfnProtect */
static int krdrRTFileProtect(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect)
{
    PKRDRFILE        pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP    pPrep = krdrRTFileFindPrepExact(pRdrFile, pvBase);
    if (!pPrep)
        return KERR_INVALID_PARAMETER;

    return krdrRTFileGenericProtect(pRdr, pPrep, cSegments, paSegments, fUnprotectOrProtect);
}


static unsigned krdrRTFileConvertProt(KPROT enmProt)
{
    switch (enmProt)
    {
        case KPROT_NOACCESS:                return RTMEM_PROT_NONE;
        case KPROT_READONLY:                return RTMEM_PROT_READ;
        case KPROT_READWRITE:               return RTMEM_PROT_READ | RTMEM_PROT_WRITE;
        case KPROT_WRITECOPY:               return RTMEM_PROT_READ | RTMEM_PROT_WRITE;
        case KPROT_EXECUTE:                 return RTMEM_PROT_EXEC;
        case KPROT_EXECUTE_READ:            return RTMEM_PROT_EXEC | RTMEM_PROT_READ;
        case KPROT_EXECUTE_READWRITE:       return RTMEM_PROT_EXEC | RTMEM_PROT_READ | RTMEM_PROT_WRITE;
        case KPROT_EXECUTE_WRITECOPY:       return RTMEM_PROT_EXEC | RTMEM_PROT_READ | RTMEM_PROT_WRITE;
        default:
            AssertFailed();
            return ~0U;
    }
}


/** Generic implementation of krdrRTFileProtect. */
static int krdrRTFileGenericProtect(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect)
{
    KU32 i;
    K_NOREF(pRdr);

    /*
     * Iterate the segments and apply memory protection changes.
     */
    for (i = 0; i < cSegments; i++)
    {
        int rc;
        void *pv;
        KPROT enmProt;

        if (paSegments[i].RVA == NIL_KLDRADDR)
            continue;

        /* calc new protection. */
        enmProt = (KPROT)paSegments[i].enmProt; /** @todo drop cast */
        if (fUnprotectOrProtect)
        {
            switch (enmProt)
            {
                case KPROT_NOACCESS:
                case KPROT_READONLY:
                case KPROT_READWRITE:
                case KPROT_WRITECOPY:
                    enmProt = KPROT_READWRITE;
                    break;
                case KPROT_EXECUTE:
                case KPROT_EXECUTE_READ:
                case KPROT_EXECUTE_READWRITE:
                case KPROT_EXECUTE_WRITECOPY:
                    enmProt = KPROT_EXECUTE_READWRITE;
                    break;
                default:
                    AssertFailed();
                    return -1;
            }
        }
        else
        {
            /* copy on write -> normal write. */
            if (enmProt == KPROT_EXECUTE_WRITECOPY)
                enmProt = KPROT_EXECUTE_READWRITE;
            else if (enmProt == KPROT_WRITECOPY)
                enmProt = KPROT_READWRITE;
        }

        pv = (KU8 *)pPrep->pv + paSegments[i].RVA;
        rc = RTMemProtect(pv, paSegments[i].cbMapped, krdrRTFileConvertProt(enmProt));
        if (rc)
            break;
    }

    return 0;
}


/** @copydoc KRDROPS::pfnRefresh */
static int      krdrRTFileRefresh(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    PKRDRFILE        pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP    pPrep = krdrRTFileFindPrepExact(pRdrFile, pvBase);
    if (!pPrep)
        return KERR_INVALID_PARAMETER;
    return krdrRTFileGenericRefresh(pRdr, pPrep, cSegments, paSegments);
}


/** Generic implementation of krdrRTFileRefresh. */
static int      krdrRTFileGenericRefresh(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments)
{
    int rc;
    int rc2;
    KU32 i;

    /*
     * Make everything writable again.
     */
    rc = krdrRTFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 1 /* unprotect */);
    if (rc)
    {
        krdrRTFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 0 /* protect */);
        return rc;
    }

    /*
     * Clear everything.
     */
    /** @todo only zero the areas not covered by raw file bits. */
    memset(pPrep->pv, 0, pPrep->cb);

    /*
     * Reload all the segments.
     * We could possibly skip some segments, but we currently have
     * no generic way of figuring out which at the moment.
     */
    for (i = 0; i < cSegments; i++)
    {
        void *pv;

        if (    paSegments[i].RVA == NIL_KLDRADDR
            ||  paSegments[i].cbFile <= 0)
            continue;

        pv = (KU8 *)pPrep->pv + paSegments[i].RVA;
        rc = pRdr->pOps->pfnRead(pRdr, pv, paSegments[i].cbFile, paSegments[i].offFile);
        if (rc)
            break;
    }

    /*
     * Protect the bits again.
     */
    rc2 = krdrRTFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 0 /* protect */);
    if (rc2 && rc)
        rc = rc2;

    return rc;
}


/** @copydoc KRDROPS::pfnMap */
static int      krdrRTFileMap(PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed)
{
    PKRDRFILE       pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP   pPrep = &pRdrFile->aPreps[pRdrFile->cPreps];
    KLDRSIZE        cbTotal;
    const KSIZE     cbPage = pRdr->pOps->pfnPageSize(pRdr);
    int             rc;
    KU32            i;

    if (pRdrFile->cPreps >= K_ELEMENTS(pRdrFile->aPreps))
        return KRDR_ERR_TOO_MANY_MAPPINGS;

    /*
     * Calc the total mapping space needed.
     */
    cbTotal = 0;
    for (i = 0; i < cSegments; i++)
    {
        KLDRSIZE uRVASegmentEnd;
        if (paSegments[i].RVA == NIL_KLDRADDR)
            continue;
        uRVASegmentEnd = paSegments[i].RVA + paSegments[i].cbMapped;
        if (cbTotal < uRVASegmentEnd)
            cbTotal = uRVASegmentEnd;
    }
    pPrep->cb = (KSIZE)cbTotal;
    if (pPrep->cb != cbTotal)
        return KLDR_ERR_ADDRESS_OVERFLOW;
    pPrep->cb = (pPrep->cb + (cbPage - 1)) & ~(cbPage- 1);

    /*
     * Use the generic map emulation.
     */
    pPrep->pv = fFixed ? *ppvBase : NULL;
    rc = krdrRTFileGenericMap(pRdr, pPrep, cSegments, paSegments, fFixed);
    if (!rc)
    {
        *ppvBase = pPrep->pv;
        pRdrFile->cPreps++;
    }

    return rc;
}


/** Generic implementation of krdrRTFileMap. */
static int  krdrRTFileGenericMap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed)
{
    int rc = 0;
    KU32 i;
    K_NOREF(fFixed);

    /*
     * Generic mapping code using kHlpPageAlloc(), kHlpPageFree() and kHlpPageProtect().
     */
    pPrep->pv = RTMemPageAlloc(pPrep->cb);
    if (!pPrep->pv)
        return KERR_NO_MEMORY;

    /*
     * Load the data.
     */
    for (i = 0; i < cSegments; i++)
    {
        void *pv;

        if (    paSegments[i].RVA == NIL_KLDRADDR
            ||  paSegments[i].cbFile <= 0)
            continue;

        pv = (KU8 *)pPrep->pv + paSegments[i].RVA;
        rc = pRdr->pOps->pfnRead(pRdr, pv, paSegments[i].cbFile, paSegments[i].offFile);
        if (rc)
            break;
    }

    /*
     * Set segment protection.
     */
    if (!rc)
    {
        rc = krdrRTFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 0 /* protect */);
        if (!rc)
            return 0;
        krdrRTFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 1 /* unprotect */);
    }

    /* bailout */
    RTMemPageFree(pPrep->pv, pPrep->cb);
    return rc;
}


/** @copydoc KRDROPS::pfnPageSize */
static KSIZE   krdrRTFilePageSize(PKRDR pRdr)
{
    K_NOREF(pRdr);
    return PAGE_SIZE;
}


/** @copydoc KRDROPS::pfnName */
static const char *krdrRTFileName(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
    return &pRdrFile->szFilename[0];
}


static KIPTR krdrRTFileNativeFH(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
    return (KIPTR)pRdrFile->File;
}


/** @copydoc KRDROPS::pfnTell */
static KFOFF krdrRTFileTell(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;

    /*
     * If the offset is undefined, try figure out what it is.
     */
    if (pRdrFile->off == -1)
    {
        pRdrFile->off = RTFileTell(pRdrFile->File);
        if (pRdrFile->off < 0)
            pRdrFile->off = -1;
    }
    return pRdrFile->off;
}


/** @copydoc KRDROPS::pfnSize */
static KFOFF krdrRTFileSize(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
    return pRdrFile->cb;
}


/** @copydoc KRDROPS::pfnAllUnmap */
static int krdrRTFileAllUnmap(PKRDR pRdr, const void *pvBits)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
    K_NOREF(pvBits);

    /* check for underflow */
    if (pRdrFile->cMappings <= 0)
        return KERR_INVALID_PARAMETER;

    /* decrement usage counter, free mapping if no longer in use. */
    if (!--pRdrFile->cMappings)
    {
        RTMemFree(pRdrFile->pvMapping);
        pRdrFile->pvMapping = NULL;
    }

    return 0;
}


/** @copydoc KRDROPS::pfnAllMap */
static int krdrRTFileAllMap(PKRDR pRdr, const void **ppvBits)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;

    /*
     * Do we need to map it?
     */
    if (!pRdrFile->pvMapping)
    {
        int rc;
        KFOFF cbFile = pRdrFile->Core.pOps->pfnSize(pRdr);
        KSIZE cb = (KSIZE)cbFile;
        if ((KFOFF)cb != cbFile)
            return KERR_NO_MEMORY;

        pRdrFile->pvMapping = RTMemAlloc(cb);
        if (!pRdrFile->pvMapping)
            return KERR_NO_MEMORY;
        rc = pRdrFile->Core.pOps->pfnRead(pRdr, pRdrFile->pvMapping, cb, 0);
        if (rc)
        {
            RTMemFree(pRdrFile->pvMapping);
            pRdrFile->pvMapping = NULL;
            return rc;
        }
        pRdrFile->cMappings = 0;
    }

    *ppvBits = pRdrFile->pvMapping;
    pRdrFile->cMappings++;
    return 0;
}


/** @copydoc KRDROPS::pfnRead */
static int krdrRTFileRead(PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
    int rc;

    /*
     * Do a seek if needed.
     */
    if (pRdrFile->off != off)
    {
        rc = RTFileSeek(pRdrFile->File, off, RTFILE_SEEK_BEGIN, NULL);
        if (RT_FAILURE(rc))
        {
            pRdrFile->off = -1;
            return rc;
        }
    }

    /*
     * Do the read.
     */
    rc = RTFileRead(pRdrFile->File, pvBuf, cb, NULL);
    if (RT_FAILURE(rc))
    {
        pRdrFile->off = -1;
        return rc;
    }

    pRdrFile->off = off + cb;
    return 0;
}


/** @copydoc KRDROPS::pfnDestroy */
static int krdrRTFileDestroy(PKRDR pRdr)
{
    PKRDRFILE    pRdrFile = (PKRDRFILE)pRdr;
    int          rc;

    rc = RTFileClose(pRdrFile->File);

    if (pRdrFile->pvMapping)
    {
        RTMemFree(pRdrFile->pvMapping);
        pRdrFile->pvMapping = NULL;
    }

    RTMemFree(pRdr);
    return rc;
}


/** @copydoc KRDROPS::pfnCreate */
static int krdrRTFileCreate(PPKRDR ppRdr, const char *pszFilename)
{
    KSIZE       cchFilename;
    PKRDRFILE   pRdrFile;
    RTFILE      File;
    uint64_t    cb;
    int         rc;
    char        szFilename[RTPATH_MAX];

    /*
     * Open the file, determin its size and correct filename.
     */
    rc = RTFileOpen(&File, pszFilename, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE);
    if (RT_FAILURE(rc))
        return rc;

    rc = RTFileGetSize(File, &cb);
    if (RT_SUCCESS(rc))
    {
        rc = RTPathReal(pszFilename, szFilename, sizeof(szFilename));
        if (RT_SUCCESS(rc))
        {
            /*
             * Allocate the reader instance.
             */
            cchFilename = strlen(szFilename);
            pRdrFile = (PKRDRFILE)RTMemAlloc(sizeof(*pRdrFile) + cchFilename);
            if (pRdrFile)
            {
                /*
                 * Initialize it and return successfully.
                 */
                pRdrFile->Core.u32Magic = KRDR_MAGIC;
                pRdrFile->Core.pOps = &g_kRdrFileOps;
                pRdrFile->File = File;
                pRdrFile->cb = cb;
                pRdrFile->off = 0;
                pRdrFile->cMappings = 0;
                pRdrFile->cPreps = 0;
                memcpy(&pRdrFile->szFilename[0], szFilename, cchFilename + 1);

                *ppRdr = &pRdrFile->Core;
                return 0;
            }

            rc = KERR_NO_MEMORY;
        }
    }

    RTFileClose(File);
    return rc;
}



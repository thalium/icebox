/* $Id: tar.cpp $ */
/** @file
 * IPRT - Tar archive I/O.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/iprt.h"
#include <iprt/tar.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/vfs.h>
#include <iprt/zip.h>


#include "internal/magics.h"
#include "tar.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** @name RTTARRECORD::h::linkflag
 * @{  */
#define LF_OLDNORMAL '\0' /**< Normal disk file, Unix compatible */
#define LF_NORMAL    '0'  /**< Normal disk file */
#define LF_LINK      '1'  /**< Link to previously dumped file */
#define LF_SYMLINK   '2'  /**< Symbolic link */
#define LF_CHR       '3'  /**< Character special file */
#define LF_BLK       '4'  /**< Block special file */
#define LF_DIR       '5'  /**< Directory */
#define LF_FIFO      '6'  /**< FIFO special file */
#define LF_CONTIG    '7'  /**< Contiguous file */
/** @} */

/**
 * A tar file header.
 */
typedef union RTTARRECORD
{
    char   d[512];
    struct h
    {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char chksum[8];
        char linkflag;
        char linkname[100];
        char magic[8];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
    } h;
} RTTARRECORD;
AssertCompileSize(RTTARRECORD, 512);
AssertCompileMemberOffset(RTTARRECORD, h.size, 100+8*3);
AssertCompileMemberSize(RTTARRECORD, h.name, RTTAR_NAME_MAX+1);
/** Pointer to a tar file header. */
typedef RTTARRECORD *PRTTARRECORD;

/** Pointer to a tar file handle. */
typedef struct RTTARFILEINTERNAL *PRTTARFILEINTERNAL;

/**
 * The internal data of a tar handle.
 */
typedef struct RTTARINTERNAL
{
    /** The magic (RTTAR_MAGIC). */
    uint32_t            u32Magic;
    /** The handle to the tar file. */
    RTFILE              hTarFile;
    /** The open mode for hTarFile. */
    uint32_t            fOpenMode;
    /** Whether a file within the archive is currently open for writing.
     * Only one can be open.  */
    bool                fFileOpenForWrite;
    /** The tar file VFS handle (for reading). */
    RTVFSFILE           hVfsFile;
    /** The tar file system VFS handle. */
    RTVFSFSSTREAM       hVfsFss;
    /** Set if hVfsFss is at the start of the stream and doesn't need rewinding. */
    bool                fFssAtStart;
} RTTARINTERNAL;
/** Pointer to a the internal data of a tar handle.  */
typedef RTTARINTERNAL* PRTTARINTERNAL;

/**
 * The internal data of a file within a tar file.
 */
typedef struct RTTARFILEINTERNAL
{
    /** The magic (RTTARFILE_MAGIC). */
    uint32_t        u32Magic;
    /** The open mode. */
    uint32_t        fOpenMode;
    /** Pointer to back to the tar file. */
    PRTTARINTERNAL  pTar;
    /** The name of the file. */
    char           *pszFilename;
    /** The offset into the archive where the file header starts. */
    uint64_t        offStart;
    /** The size of the file. */
    uint64_t        cbSize;
    /** The size set by RTTarFileSetSize(). */
    uint64_t        cbSetSize;
    /** The current offset within this file. */
    uint64_t        offCurrent;
    /** The VFS I/O stream (only for reading atm). */
    RTVFSIOSTREAM   hVfsIos;
} RTTARFILEINTERNAL;
/** Pointer to the internal data of a tar file.  */
typedef RTTARFILEINTERNAL *PRTTARFILEINTERNAL;



/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
/* RTTAR */
#define RTTAR_VALID_RETURN_RC(hHandle, rc) \
    do { \
        AssertPtrReturn((hHandle), (rc)); \
        AssertReturn((hHandle)->u32Magic == RTTAR_MAGIC, (rc)); \
    } while (0)
/* RTTARFILE */
#define RTTARFILE_VALID_RETURN_RC(hHandle, rc) \
    do { \
        AssertPtrReturn((hHandle), (rc)); \
        AssertReturn((hHandle)->u32Magic == RTTARFILE_MAGIC, (rc)); \
    } while (0)

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
/* RTTAR */
#define RTTAR_VALID_RETURN(hHandle) RTTAR_VALID_RETURN_RC((hHandle), VERR_INVALID_HANDLE)
/* RTTARFILE */
#define RTTARFILE_VALID_RETURN(hHandle) RTTARFILE_VALID_RETURN_RC((hHandle), VERR_INVALID_HANDLE)

/** Validates a handle and returns (void) if not valid. */
/* RTTAR */
#define RTTAR_VALID_RETURN_VOID(hHandle) \
    do { \
        AssertPtrReturnVoid(hHandle); \
        AssertReturnVoid((hHandle)->u32Magic == RTTAR_MAGIC); \
    } while (0)
/* RTTARFILE */
#define RTTARFILE_VALID_RETURN_VOID(hHandle) \
    do { \
        AssertPtrReturnVoid(hHandle); \
        AssertReturnVoid((hHandle)->u32Magic == RTTARFILE_MAGIC); \
    } while (0)


RTR3DECL(int) RTTarOpen(PRTTAR phTar, const char *pszTarname, uint32_t fMode)
{
    AssertReturn(fMode & RTFILE_O_WRITE, VERR_INVALID_PARAMETER);

    /*
     * Create a tar instance.
     */
    PRTTARINTERNAL pThis = (PRTTARINTERNAL)RTMemAllocZ(sizeof(RTTARINTERNAL));
    if (!pThis)
        return VERR_NO_MEMORY;

    pThis->u32Magic         = RTTAR_MAGIC;
    pThis->fOpenMode        = fMode;
    pThis->hVfsFile         = NIL_RTVFSFILE;
    pThis->hVfsFss          = NIL_RTVFSFSSTREAM;
    pThis->fFssAtStart      = false;

    /*
     * Open the tar file.
     */
    int rc = RTFileOpen(&pThis->hTarFile, pszTarname, fMode);
    if (RT_SUCCESS(rc))
    {
        *phTar = pThis;
        return VINF_SUCCESS;
    }

    RTMemFree(pThis);
    return rc;
}


RTR3DECL(int) RTTarClose(RTTAR hTar)
{
    if (hTar == NIL_RTTAR)
        return VINF_SUCCESS;

    PRTTARINTERNAL pInt = hTar;
    RTTAR_VALID_RETURN(pInt);

    int rc = VINF_SUCCESS;

    /* gtar gives a warning, but the documentation says EOF is indicated by a
     * zero block. Disabled for now. */
#if 0
    {
        /* Append the EOF record which is filled all by zeros */
        RTTARRECORD record;
        RT_ZERO(record);
        rc = RTFileWrite(pInt->hTarFile, &record, sizeof(record), NULL);
    }
#endif

    if (pInt->hVfsFss != NIL_RTVFSFSSTREAM)
    {
        uint32_t cRefs = RTVfsFsStrmRelease(pInt->hVfsFss); Assert(cRefs != UINT32_MAX); NOREF(cRefs);
        pInt->hVfsFss  = NIL_RTVFSFSSTREAM;
    }

    if (pInt->hVfsFile != NIL_RTVFSFILE)
    {
        uint32_t cRefs = RTVfsFileRelease(pInt->hVfsFile); Assert(cRefs != UINT32_MAX); NOREF(cRefs);
        pInt->hVfsFile = NIL_RTVFSFILE;
    }

    if (pInt->hTarFile != NIL_RTFILE)
    {
        rc = RTFileClose(pInt->hTarFile);
        pInt->hTarFile = NIL_RTFILE;
    }

    pInt->u32Magic = RTTAR_MAGIC_DEAD;

    RTMemFree(pInt);

    return rc;
}


/**
 * Creates a tar file handle for a read-only VFS stream object.
 *
 * @returns IPRT status code.
 * @param   pszName             The file name. Automatically freed on failure.
 * @param   hVfsIos             The VFS I/O stream we create the handle around.
 *                              The reference is NOT consumed.
 * @param   fOpen               The open flags.
 * @param   ppFile              Where to return the handle.
 */
static int rtTarFileCreateHandleForReadOnly(char *pszName, RTVFSIOSTREAM hVfsIos, uint32_t fOpen, PRTTARFILEINTERNAL *ppFile)
{
    int rc;
    PRTTARFILEINTERNAL pNewFile = (PRTTARFILEINTERNAL)RTMemAllocZ(sizeof(*pNewFile));
    if (pNewFile)
    {
        RTFSOBJINFO ObjInfo;
        rc = RTVfsIoStrmQueryInfo(hVfsIos, &ObjInfo, RTFSOBJATTRADD_UNIX);
        if (RT_SUCCESS(rc))
        {
            pNewFile->u32Magic      = RTTARFILE_MAGIC;
            pNewFile->pTar          = NULL;
            pNewFile->pszFilename   = pszName;
            pNewFile->offStart      = UINT64_MAX;
            pNewFile->cbSize        = ObjInfo.cbObject;
            pNewFile->cbSetSize     = 0;
            pNewFile->offCurrent    = 0;
            pNewFile->fOpenMode     = fOpen;
            pNewFile->hVfsIos       = hVfsIos;

            uint32_t cRefs = RTVfsIoStrmRetain(hVfsIos); Assert(cRefs != UINT32_MAX); NOREF(cRefs);

            *ppFile = pNewFile;
            return VINF_SUCCESS;
        }

        RTMemFree(pNewFile);
    }
    else
        rc = VERR_NO_MEMORY;
    RTStrFree(pszName);
    return rc;
}


/* Only used for write handles. */
static PRTTARFILEINTERNAL rtTarFileCreateForWrite(PRTTARINTERNAL pInt, const char *pszFilename, uint32_t fOpen)
{
    PRTTARFILEINTERNAL pFileInt = (PRTTARFILEINTERNAL)RTMemAllocZ(sizeof(RTTARFILEINTERNAL));
    if (!pFileInt)
        return NULL;

    pFileInt->u32Magic    = RTTARFILE_MAGIC;
    pFileInt->pTar        = pInt;
    pFileInt->fOpenMode   = fOpen;
    pFileInt->pszFilename = RTStrDup(pszFilename);
    pFileInt->hVfsIos     = NIL_RTVFSIOSTREAM;
    if (pFileInt->pszFilename)
        return pFileInt;

    RTMemFree(pFileInt);
    return NULL;

}


RTR3DECL(int) RTTarFileOpen(RTTAR hTar, PRTTARFILE phFile, const char *pszFilename, uint32_t fOpen)
{
    /* Write only interface now. */
    AssertReturn(fOpen & RTFILE_O_WRITE, VERR_INVALID_PARAMETER);

    PRTTARINTERNAL pInt = hTar;
    RTTAR_VALID_RETURN(pInt);

    if (!pInt->hTarFile)
        return VERR_INVALID_HANDLE;

    if (fOpen & RTFILE_O_WRITE)
    {
        if (!(pInt->fOpenMode & RTFILE_O_WRITE))
            return VERR_WRITE_PROTECT;
        if (pInt->fFileOpenForWrite)
            return VERR_TOO_MANY_OPEN_FILES;
    }

    int rc = VINF_SUCCESS;
    if (!(fOpen & RTFILE_O_WRITE))
    {
        /*
         * Rewind the stream if necessary.
         */
        if (!pInt->fFssAtStart)
        {
            if (pInt->hVfsFss != NIL_RTVFSFSSTREAM)
            {
                uint32_t cRefs = RTVfsFsStrmRelease(pInt->hVfsFss); Assert(cRefs != UINT32_MAX); NOREF(cRefs);
                pInt->hVfsFss  = NIL_RTVFSFSSTREAM;
            }

            if (pInt->hVfsFile == NIL_RTVFSFILE)
            {
                rc = RTVfsFileFromRTFile(pInt->hTarFile, RTFILE_O_READ, true /*fLeaveOpen*/, &pInt->hVfsFile);
                if (RT_FAILURE(rc))
                    return rc;
            }

            RTVFSIOSTREAM hVfsIos = RTVfsFileToIoStream(pInt->hVfsFile);
            rc = RTZipTarFsStreamFromIoStream(hVfsIos, 0 /*fFlags*/, &pInt->hVfsFss);
            RTVfsIoStrmRelease(hVfsIos);
            if (RT_FAILURE(rc))
                return rc;
        }

        /*
         * Search the file system stream.
         */
        pInt->fFssAtStart = false;
        for (;;)
        {
            char           *pszName;
            RTVFSOBJTYPE    enmType;
            RTVFSOBJ        hVfsObj;
            rc = RTVfsFsStrmNext(pInt->hVfsFss, &pszName, &enmType, &hVfsObj);
            if (rc == VERR_EOF)
                return VERR_FILE_NOT_FOUND;
            if (RT_FAILURE(rc))
                return rc;

            if (!RTStrCmp(pszName, pszFilename))
            {
                if (enmType == RTVFSOBJTYPE_FILE || enmType == RTVFSOBJTYPE_IO_STREAM)
                    rc = rtTarFileCreateHandleForReadOnly(pszName, RTVfsObjToIoStream(hVfsObj), fOpen, phFile);
                else
                {
                    rc = VERR_UNEXPECTED_FS_OBJ_TYPE;
                    RTStrFree(pszName);
                }
                RTVfsObjRelease(hVfsObj);
                break;
            }
            RTStrFree(pszName);
            RTVfsObjRelease(hVfsObj);
        } /* Search loop. */
    }
    else
    {
        PRTTARFILEINTERNAL pFileInt = rtTarFileCreateForWrite(pInt, pszFilename, fOpen);
        if (!pFileInt)
            return VERR_NO_MEMORY;

        pInt->fFileOpenForWrite = true;

        /* If we are in write mode, we also in append mode. Add an dummy
         * header at the end of the current file. It will be filled by the
         * close operation. */
        rc = RTFileSeek(pFileInt->pTar->hTarFile, 0, RTFILE_SEEK_END, &pFileInt->offStart);
        if (RT_SUCCESS(rc))
        {
            RTTARRECORD record;
            RT_ZERO(record);
            rc = RTFileWrite(pFileInt->pTar->hTarFile, &record, sizeof(RTTARRECORD), NULL);
        }

        if (RT_SUCCESS(rc))
            *phFile = (RTTARFILE)pFileInt;
        else
        {
            /* Cleanup on failure */
            if (pFileInt->pszFilename)
                RTStrFree(pFileInt->pszFilename);
            RTMemFree(pFileInt);
        }
    }

    return rc;
}


static void rtTarSizeToRec(PRTTARRECORD pRecord, uint64_t cbSize)
{
    /*
     * Small enough for the standard octal string encoding?
     *
     * Note! We could actually use the terminator character as well if we liked,
     *       but let not do that as it's easier to test this way.
     */
    if (cbSize < _4G * 2U)
        RTStrPrintf(pRecord->h.size, sizeof(pRecord->h.size), "%0.11llo", cbSize);
    else
    {
        /*
         * Base 256 extension. Set the highest bit of the left most character.
         * We don't deal with negatives here, cause the size have to be greater
         * than zero.
         *
         * Note! The base-256 extension are never used by gtar or libarchive
         *       with the "ustar  \0" format version, only the later
         *       "ustar\000" version.  However, this shouldn't cause much
         *       trouble as they are not picky about what they read.
         */
        size_t cchField = sizeof(pRecord->h.size) - 1;
        unsigned char *puchField = (unsigned char*)pRecord->h.size;
        puchField[0] = 0x80;
        do
        {
            puchField[cchField--] = cbSize & 0xff;
            cbSize >>= 8;
        } while (cchField);
    }
}


static int rtTarCreateHeaderRecord(PRTTARRECORD pRecord, const char *pszSrcName, uint64_t cbSize,
                                   RTUID uid, RTGID gid, RTFMODE fmode, int64_t mtime)
{
    /** @todo check for field overflows. */
    /* Fill the header record */
//    RT_ZERO(pRecord); - done by the caller.
    /** @todo use RTStrCopy */
    size_t cb = RTStrPrintf(pRecord->h.name,  sizeof(pRecord->h.name),  "%s", pszSrcName);
    if (cb < strlen(pszSrcName))
        return VERR_BUFFER_OVERFLOW;
    RTStrPrintf(pRecord->h.mode,  sizeof(pRecord->h.mode),  "%0.7o", fmode);
    RTStrPrintf(pRecord->h.uid,   sizeof(pRecord->h.uid),   "%0.7o", uid);
    RTStrPrintf(pRecord->h.gid,   sizeof(pRecord->h.gid),   "%0.7o", gid);
    rtTarSizeToRec(pRecord, cbSize);
    RTStrPrintf(pRecord->h.mtime, sizeof(pRecord->h.mtime), "%0.11llo", mtime);
    RTStrPrintf(pRecord->h.magic, sizeof(pRecord->h.magic), "ustar  ");
    RTStrPrintf(pRecord->h.uname, sizeof(pRecord->h.uname), "someone");
    RTStrPrintf(pRecord->h.gname, sizeof(pRecord->h.gname), "someone");
    pRecord->h.linkflag = LF_NORMAL;

    /* Create the checksum out of the new header */
    int32_t iUnsignedChksum, iSignedChksum;
    if (rtZipTarCalcChkSum((PCRTZIPTARHDR)pRecord, &iUnsignedChksum, &iSignedChksum))
        return VERR_TAR_END_OF_FILE;

    /* Format the checksum */
    RTStrPrintf(pRecord->h.chksum, sizeof(pRecord->h.chksum), "%0.7o", iUnsignedChksum);

    return VINF_SUCCESS;
}


DECLINLINE(void *) rtTarMemTmpAlloc(size_t *pcbSize)
{
    *pcbSize = 0;
    /* Allocate a reasonably large buffer, fall back on a tiny one.
     * Note: has to be 512 byte aligned and >= 512 byte. */
    size_t cbTmp = _1M;
    void *pvTmp = RTMemTmpAlloc(cbTmp);
    if (!pvTmp)
    {
        cbTmp = sizeof(RTTARRECORD);
        pvTmp = RTMemTmpAlloc(cbTmp);
    }
    *pcbSize = cbTmp;
    return pvTmp;
}


static int rtTarAppendZeros(PRTTARFILEINTERNAL pFileInt, uint64_t cbSize)
{
    /* Allocate a temporary buffer for copying the tar content in blocks. */
    size_t cbTmp = 0;
    void *pvTmp = rtTarMemTmpAlloc(&cbTmp);
    if (!pvTmp)
        return VERR_NO_MEMORY;
    RT_BZERO(pvTmp, cbTmp);

    int rc = VINF_SUCCESS;
    uint64_t cbAllWritten = 0;
    size_t cbWritten = 0;
    for (;;)
    {
        if (cbAllWritten >= cbSize)
            break;
        size_t cbToWrite = RT_MIN(cbSize - cbAllWritten, cbTmp);
        rc = RTTarFileWriteAt(pFileInt, pFileInt->offCurrent, pvTmp, cbToWrite, &cbWritten);
        if (RT_FAILURE(rc))
            break;
        cbAllWritten += cbWritten;
    }

    RTMemTmpFree(pvTmp);

    return rc;
}


RTR3DECL(int) RTTarFileClose(RTTARFILE hFile)
{
    /* Already closed? */
    if (hFile == NIL_RTTARFILE)
        return VINF_SUCCESS;

    PRTTARFILEINTERNAL pFileInt = hFile;
    RTTARFILE_VALID_RETURN(pFileInt);

    int rc = VINF_SUCCESS;

    /* In write mode: */
    if ((pFileInt->fOpenMode & (RTFILE_O_WRITE | RTFILE_O_READ)) == RTFILE_O_WRITE)
    {
        pFileInt->pTar->fFileOpenForWrite = false;
        do
        {
            /* If the user has called RTTarFileSetSize in the meantime, we have
               to make sure the file has the right size. */
            if (pFileInt->cbSetSize > pFileInt->cbSize)
            {
                rc = rtTarAppendZeros(pFileInt, pFileInt->cbSetSize - pFileInt->cbSize);
                if (RT_FAILURE(rc))
                    break;
            }

            /* If the written size isn't 512 byte aligned, we need to fix this. */
            RTTARRECORD record;
            RT_ZERO(record);
            uint64_t cbSizeAligned = RT_ALIGN(pFileInt->cbSize, sizeof(RTTARRECORD));
            if (cbSizeAligned != pFileInt->cbSize)
            {
                /* Note the RTFile method. We didn't increase the cbSize or cbCurrentPos here. */
                rc = RTFileWriteAt(pFileInt->pTar->hTarFile,
                                   pFileInt->offStart + sizeof(RTTARRECORD) + pFileInt->cbSize,
                                   &record,
                                   cbSizeAligned - pFileInt->cbSize,
                                   NULL);
                if (RT_FAILURE(rc))
                    break;
            }

            /* Create a header record for the file */
            /** @todo mode, gid, uid, mtime should be setable (or detected myself) */
            RTTIMESPEC time;
            RTTimeNow(&time);
            rc = rtTarCreateHeaderRecord(&record, pFileInt->pszFilename, pFileInt->cbSize,
                                         0, 0, 0600, RTTimeSpecGetSeconds(&time));
            if (RT_FAILURE(rc))
                break;

            /* Write this at the start of the file data */
            rc = RTFileWriteAt(pFileInt->pTar->hTarFile, pFileInt->offStart, &record, sizeof(RTTARRECORD), NULL);
            if (RT_FAILURE(rc))
                break;
        }
        while (0);
    }

    /*
     * Now cleanup and delete the handle.
     */
    if (pFileInt->pszFilename)
        RTStrFree(pFileInt->pszFilename);
    if (pFileInt->hVfsIos != NIL_RTVFSIOSTREAM)
    {
        RTVfsIoStrmRelease(pFileInt->hVfsIos);
        pFileInt->hVfsIos = NIL_RTVFSIOSTREAM;
    }
    pFileInt->u32Magic = RTTARFILE_MAGIC_DEAD;
    RTMemFree(pFileInt);

    return rc;
}


RTR3DECL(int) RTTarFileReadAt(RTTARFILE hFile, uint64_t off, void *pvBuf, size_t cbToRead, size_t *pcbRead)
{
    PRTTARFILEINTERNAL pFileInt = hFile;
    RTTARFILE_VALID_RETURN(pFileInt);

    size_t cbTmpRead = 0;
    int rc = RTVfsIoStrmReadAt(pFileInt->hVfsIos, off, pvBuf, cbToRead, true /*fBlocking*/, &cbTmpRead);
    if (RT_SUCCESS(rc))
    {
        pFileInt->offCurrent = off + cbTmpRead;
        if (pcbRead)
            *pcbRead = cbTmpRead;
        if (rc == VINF_EOF)
            rc = pcbRead ? VINF_SUCCESS : VERR_EOF;
    }
    else if (pcbRead)
        *pcbRead = 0;
    return rc;
}


RTR3DECL(int) RTTarFileWriteAt(RTTARFILE hFile, uint64_t off, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten)
{
    PRTTARFILEINTERNAL pFileInt = hFile;
    RTTARFILE_VALID_RETURN(pFileInt);

    if ((pFileInt->fOpenMode & RTFILE_O_WRITE) != RTFILE_O_WRITE)
        return VERR_ACCESS_DENIED;

    size_t cbTmpWritten = 0;
    int rc = RTFileWriteAt(pFileInt->pTar->hTarFile, pFileInt->offStart + 512 + off, pvBuf, cbToWrite, &cbTmpWritten);
    pFileInt->cbSize += cbTmpWritten;
    pFileInt->offCurrent = off + cbTmpWritten;
    if (pcbWritten)
        *pcbWritten = cbTmpWritten;

    return rc;
}


RTR3DECL(int) RTTarFileGetSize(RTTARFILE hFile, uint64_t *pcbSize)
{
    /* Validate input */
    AssertPtrReturn(pcbSize, VERR_INVALID_POINTER);

    PRTTARFILEINTERNAL pFileInt = hFile;
    RTTARFILE_VALID_RETURN(pFileInt);

    *pcbSize = RT_MAX(pFileInt->cbSetSize, pFileInt->cbSize);

    return VINF_SUCCESS;
}


RTR3DECL(int) RTTarFileSetSize(RTTARFILE hFile, uint64_t cbSize)
{
    PRTTARFILEINTERNAL pFileInt = hFile;
    RTTARFILE_VALID_RETURN(pFileInt);

    if ((pFileInt->fOpenMode & RTFILE_O_WRITE) != RTFILE_O_WRITE)
        return VERR_WRITE_ERROR;

    /** @todo If cbSize is smaller than pFileInt->cbSize we have to
     * truncate the current file. */
    pFileInt->cbSetSize = cbSize;

    return VINF_SUCCESS;
}


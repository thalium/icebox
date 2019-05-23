/** $Id: VDIoBackend.cpp $ */
/** @file
 *
 * VBox HDD container test utility, I/O backend API
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#define LOGGROUP LOGGROUP_DEFAULT /** @todo Log group */
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/file.h>
#include <iprt/aiomgr.h>
#include <iprt/string.h>

#include "VDIoBackend.h"
#include "VDMemDisk.h"
#include "VDIoBackendMem.h"

typedef struct VDIOBACKEND
{
    /** Memory I/O backend handle. */
    PVDIOBACKENDMEM   pIoMem;
    /** Async I/O manager. */
    RTAIOMGR          hAioMgr;
    /** Users of the memory backend. */
    volatile uint32_t cRefsIoMem;
    /** Users of the file backend. */
    volatile uint32_t cRefsFile;
} VDIOBACKEND;

typedef struct VDIOSTORAGE
{
    /** Pointer to the I/O backend parent. */
    PVDIOBACKEND      pIoBackend;
    /** Completion callback. */
    PFNVDIOCOMPLETE   pfnComplete;
    /** Flag whether this storage is backed by a file or memory.disk. */
    bool              fMemory;
    /** Type dependent data. */
    union
    {
        /** Memory disk handle. */
        PVDMEMDISK    pMemDisk;
        struct
        {
            /** file handle. */
            RTFILE        hFile;
            /** I/O manager file handle. */
            RTAIOMGRFILE  hAioMgrFile;
        } File;
    } u;
} VDIOSTORAGE;

static DECLCALLBACK(void) vdIoBackendFileIoComplete(RTAIOMGRFILE hAioMgrFile, int rcReq, void *pvUser)
{
    PVDIOSTORAGE pIoStorage = (PVDIOSTORAGE)RTAioMgrFileGetUser(hAioMgrFile);
    pIoStorage->pfnComplete(pvUser, rcReq);
}

int VDIoBackendCreate(PPVDIOBACKEND ppIoBackend)
{
    int rc = VINF_SUCCESS;
    PVDIOBACKEND pIoBackend;

    pIoBackend = (PVDIOBACKEND)RTMemAllocZ(sizeof(VDIOBACKEND));
    if (pIoBackend)
    {
        pIoBackend->hAioMgr = NIL_RTAIOMGR;
        *ppIoBackend = pIoBackend;
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

void VDIoBackendDestroy(PVDIOBACKEND pIoBackend)
{
    if (pIoBackend->pIoMem)
        VDIoBackendMemDestroy(pIoBackend->pIoMem);
    if (pIoBackend->hAioMgr)
        RTAioMgrRelease(pIoBackend->hAioMgr);
    RTMemFree(pIoBackend);
}

int VDIoBackendStorageCreate(PVDIOBACKEND pIoBackend, const char *pszBackend,
                             const char *pszName, PFNVDIOCOMPLETE pfnComplete,
                             PPVDIOSTORAGE ppIoStorage)
{
    int rc = VINF_SUCCESS;
    PVDIOSTORAGE pIoStorage = (PVDIOSTORAGE)RTMemAllocZ(sizeof(VDIOSTORAGE));

    if (pIoStorage)
    {
        pIoStorage->pIoBackend = pIoBackend;
        pIoStorage->pfnComplete = pfnComplete;
        if (!strcmp(pszBackend, "memory"))
        {
            pIoStorage->fMemory = true;
            rc = VDMemDiskCreate(&pIoStorage->u.pMemDisk, 0 /* Growing */);
            if (RT_SUCCESS(rc))
            {
                uint32_t cRefs = ASMAtomicIncU32(&pIoBackend->cRefsIoMem);
                if (   cRefs == 1
                    && !pIoBackend->pIoMem)
                {
                    rc = VDIoBackendMemCreate(&pIoBackend->pIoMem);
                    if (RT_FAILURE(rc))
                        VDMemDiskDestroy(pIoStorage->u.pMemDisk);
                }
            }
        }
        else if (!strcmp(pszBackend, "file"))
        {
            pIoStorage->fMemory = false;
            uint32_t cRefs = ASMAtomicIncU32(&pIoBackend->cRefsFile);
            if (   cRefs == 1
                && pIoBackend->hAioMgr == NIL_RTAIOMGR)
                rc = RTAioMgrCreate(&pIoBackend->hAioMgr, 1024);

            if (RT_SUCCESS(rc))
            {
                /* Create file. */
                rc = RTFileOpen(&pIoStorage->u.File.hFile, pszName,
                                RTFILE_O_READWRITE | RTFILE_O_CREATE | RTFILE_O_ASYNC_IO | RTFILE_O_NO_CACHE | RTFILE_O_DENY_NONE);
                if (RT_SUCCESS(rc))
                {
                    /* Create file handle for I/O manager. */
                    rc = RTAioMgrFileCreate(pIoBackend->hAioMgr, pIoStorage->u.File.hFile,
                                            vdIoBackendFileIoComplete, pIoStorage,
                                            &pIoStorage->u.File.hAioMgrFile);
                    if (RT_FAILURE(rc))
                        RTFileClose(pIoStorage->u.File.hFile);
                }
            }

            if (RT_FAILURE(rc))
                ASMAtomicDecU32(&pIoBackend->cRefsFile);
        }
        else
            rc = VERR_NOT_SUPPORTED;

        if (RT_FAILURE(rc))
            RTMemFree(pIoStorage);
        else
            *ppIoStorage = pIoStorage;
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

void VDIoBackendStorageDestroy(PVDIOSTORAGE pIoStorage)
{
    if (pIoStorage->fMemory)
    {
        VDMemDiskDestroy(pIoStorage->u.pMemDisk);
        ASMAtomicDecU32(&pIoStorage->pIoBackend->cRefsIoMem);
    }
    else
    {
        RTAioMgrFileRelease(pIoStorage->u.File.hAioMgrFile);
        RTFileClose(pIoStorage->u.File.hFile);
        ASMAtomicDecU32(&pIoStorage->pIoBackend->cRefsFile);
    }
    RTMemFree(pIoStorage);
}

int VDIoBackendTransfer(PVDIOSTORAGE pIoStorage, VDIOTXDIR enmTxDir, uint64_t off,
                        size_t cbTransfer, PRTSGBUF pSgBuf, void *pvUser, bool fSync)
{
    int rc = VINF_SUCCESS;

    if (pIoStorage->fMemory)
    {
        if (!fSync)
        {
            rc = VDIoBackendMemTransfer(pIoStorage->pIoBackend->pIoMem, pIoStorage->u.pMemDisk,
                                        enmTxDir, off, cbTransfer, pSgBuf, pIoStorage->pfnComplete,
                                        pvUser);
        }
        else
        {
            switch (enmTxDir)
            {
                case VDIOTXDIR_READ:
                    rc = VDMemDiskRead(pIoStorage->u.pMemDisk, off, cbTransfer, pSgBuf);
                    break;
                case VDIOTXDIR_WRITE:
                    rc = VDMemDiskWrite(pIoStorage->u.pMemDisk, off, cbTransfer, pSgBuf);
                    break;
                case VDIOTXDIR_FLUSH:
                    break;
                default:
                    AssertMsgFailed(("Invalid transfer type %d\n", enmTxDir));
            }
        }
    }
    else
    {
        if (!fSync)
        {
            switch (enmTxDir)
            {
                case VDIOTXDIR_READ:
                    rc = RTAioMgrFileRead(pIoStorage->u.File.hAioMgrFile, off, pSgBuf, cbTransfer, pvUser);
                    break;
                case VDIOTXDIR_WRITE:
                    rc = RTAioMgrFileWrite(pIoStorage->u.File.hAioMgrFile, off, pSgBuf, cbTransfer, pvUser);
                    break;
                case VDIOTXDIR_FLUSH:
                    rc = RTAioMgrFileFlush(pIoStorage->u.File.hAioMgrFile, pvUser);
                    break;
                default:
                    AssertMsgFailed(("Invalid transfer type %d\n", enmTxDir));
            }
            if (rc == VERR_FILE_AIO_IN_PROGRESS)
                rc = VINF_SUCCESS;
        }
        else
        {
            switch (enmTxDir)
            {
                case VDIOTXDIR_READ:
                    rc = RTFileSgReadAt(pIoStorage->u.File.hFile, off, pSgBuf, cbTransfer, NULL);
                    break;
                case VDIOTXDIR_WRITE:
                    rc = RTFileSgWriteAt(pIoStorage->u.File.hFile, off, pSgBuf, cbTransfer, NULL);
                    break;
                case VDIOTXDIR_FLUSH:
                    rc = RTFileFlush(pIoStorage->u.File.hFile);
                    break;
                default:
                    AssertMsgFailed(("Invalid transfer type %d\n", enmTxDir));
            }
        }
    }

    return rc;
}

int VDIoBackendStorageSetSize(PVDIOSTORAGE pIoStorage, uint64_t cbSize)
{
    int rc = VINF_SUCCESS;

    if (pIoStorage->fMemory)
    {
        rc = VDMemDiskSetSize(pIoStorage->u.pMemDisk, cbSize);
    }
    else
        rc = RTFileSetSize(pIoStorage->u.File.hFile, cbSize);

    return rc;
}

int VDIoBackendStorageGetSize(PVDIOSTORAGE pIoStorage, uint64_t *pcbSize)
{
    int rc = VINF_SUCCESS;

    if (pIoStorage->fMemory)
    {
        rc = VDMemDiskGetSize(pIoStorage->u.pMemDisk, pcbSize);
    }
    else
        rc = RTFileGetSize(pIoStorage->u.File.hFile, pcbSize);

    return rc;
}

DECLHIDDEN(int) VDIoBackendDumpToFile(PVDIOSTORAGE pIoStorage, const char *pszPath)
{
    int rc = VINF_SUCCESS;

    if (pIoStorage->fMemory)
        rc = VDMemDiskWriteToFile(pIoStorage->u.pMemDisk, pszPath);
    else
        rc = VERR_NOT_IMPLEMENTED;

    return rc;
}


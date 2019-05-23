/** $Id: VDIoBackend.h $ */
/** @file
 *
 * VBox HDD container test utility, async I/O backend
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
#ifndef __VDIoBackend_h__
#define __VDIoBackend_h__

#include <iprt/sg.h>

#include "VDDefs.h"

/** I/O backend handle. */
typedef struct VDIOBACKEND *PVDIOBACKEND;
/** Pointer to a I/O backend handle. */
typedef PVDIOBACKEND *PPVDIOBACKEND;

/** Storage handle. */
typedef struct VDIOSTORAGE *PVDIOSTORAGE;
/** Pointer to a storage handle. */
typedef PVDIOSTORAGE *PPVDIOSTORAGE;

/**
 * Completion handler.
 *
 * @returns nothing.
 * @param   pvUser    Opaque user data.
 * @param   rcReq     Completion code for the request.
 */
typedef DECLCALLBACK(int) FNVDIOCOMPLETE(void *pvUser, int rcReq);
/** Pointer to a completion handler. */
typedef FNVDIOCOMPLETE *PFNVDIOCOMPLETE;

/**
 * Creates a new memory I/O backend.
 *
 * @returns IPRT status code.
 *
 * @param ppIoBackend    Where to store the handle on success.
 */
int VDIoBackendCreate(PPVDIOBACKEND ppIoBackend);

/**
 * Destroys a memory I/O backend.
 *
 * @returns nothing.
 *
 * @param pIoBackend     The backend to destroy.
 */
void VDIoBackendDestroy(PVDIOBACKEND pIoBackend);

int VDIoBackendStorageCreate(PVDIOBACKEND pIoBackend, const char *pszBackend,
                             const char *pszName, PFNVDIOCOMPLETE pfnComplete,
                             PPVDIOSTORAGE ppIoStorage);

void VDIoBackendStorageDestroy(PVDIOSTORAGE pIoStorage);

int VDIoBackendStorageSetSize(PVDIOSTORAGE pIoStorage, uint64_t cbSize);

int VDIoBackendStorageGetSize(PVDIOSTORAGE pIoStorage, uint64_t *pcbSize);

DECLHIDDEN(int) VDIoBackendDumpToFile(PVDIOSTORAGE pIoStorage, const char *pszPath);

/**
 * Enqueues a new I/O request.
 *
 * @returns IPRT status code.
 *
 * @param pIoStorage     Storage handle.
 * @param enmTxDir       The transfer direction.
 * @param off            Start offset of the transfer.
 * @param cbTransfer     Size of the transfer.
 * @param pSgBuf         S/G buffer to use.
 * @param pvUser         Opaque user data.
 * @param fSync          Flag whether to wait for the operation to complete.
 */
int VDIoBackendTransfer(PVDIOSTORAGE pIoStorage, VDIOTXDIR enmTxDir, uint64_t off,
                        size_t cbTransfer, PRTSGBUF pSgBuf, void *pvUser, bool fSync);

#endif /* __VDIoBackendMem_h__ */

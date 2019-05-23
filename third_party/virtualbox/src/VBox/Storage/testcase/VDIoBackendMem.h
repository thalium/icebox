/** $Id: VDIoBackendMem.h $ */
/** @file
 *
 * VBox HDD container test utility, async I/O memory backend
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef __VDIoBackendMem_h__
#define __VDIoBackendMem_h__

#include <iprt/sg.h>

#include "VDDefs.h"

/** Memory backend handle. */
typedef struct VDIOBACKENDMEM *PVDIOBACKENDMEM;
/** Pointer to a memory backend handle. */
typedef PVDIOBACKENDMEM *PPVDIOBACKENDMEM;

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
int VDIoBackendMemCreate(PPVDIOBACKENDMEM ppIoBackend);

/**
 * Destroys a memory I/O backend.
 *
 * @returns IPRT status code.
 *
 * @param pIoBackend     The backend to destroy.
 */
int VDIoBackendMemDestroy(PVDIOBACKENDMEM pIoBackend);

/**
 * Enqueues a new I/O request.
 *
 * @returns IPRT status code.
 *
 * @param pIoBackend     The backend which should handle the
 *                       transfer.
 * @param pMemDisk       The memory disk the request is for.
 * @param enmTxDir       The transfer direction.
 * @param off            Start offset of the transfer.
 * @param cbTransfer     Size of the transfer.
 * @param pSgBuf         S/G buffer to use.
 * @param pfnComplete    Completion handler to call.
 * @param pvUser         Opaque user data.
 */
int VDIoBackendMemTransfer(PVDIOBACKENDMEM pIoBackend, PVDMEMDISK pMemDisk,
                           VDIOTXDIR enmTxDir, uint64_t off, size_t cbTransfer,
                           PRTSGBUF pSgBuf, PFNVDIOCOMPLETE pfnComplete, void *pvUser);

#endif /* __VDIoBackendMem_h__ */

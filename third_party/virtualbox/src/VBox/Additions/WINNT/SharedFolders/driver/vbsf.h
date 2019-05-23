/* $Id: vbsf.h $ */
/** @file
 * VirtualBox Windows Guest Shared Folders - File System Driver header file
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBSF_H
#define VBSF_H

/*
 * This must be defined before including RX headers.
 */
#define MINIRDR__NAME VBoxMRx
#define ___MINIRDR_IMPORTS_NAME (VBoxMRxDeviceObject->RdbssExports)

/*
 * System and RX headers.
 */
#include <iprt/nt/nt.h> /* includes ntifs.h + wdm.h */
#include <iprt/win/windef.h>
#ifndef INVALID_HANDLE_VALUE
# define INVALID_HANDLE_VALUE RTNT_INVALID_HANDLE_VALUE /* (The rx.h definition causes warnings for amd64)  */
#endif
#include <iprt/nt/rx.h>

/*
 * VBox shared folders.
 */
#include "vbsfhlp.h"
#include "vbsfshared.h"

extern PRDBSS_DEVICE_OBJECT VBoxMRxDeviceObject;

/*
 * Maximum drive letters (A - Z).
 */
#define _MRX_MAX_DRIVE_LETTERS 26

/*
 * The shared folders device extension.
 */
typedef struct _MRX_VBOX_DEVICE_EXTENSION
{
    /* The shared folders device object pointer. */
    PRDBSS_DEVICE_OBJECT pDeviceObject;

    /*
     * Keep a list of local connections used.
     * The size (_MRX_MAX_DRIVE_LETTERS = 26) of the array presents the available drive letters C: - Z: of Windows.
     */
    CHAR cLocalConnections[_MRX_MAX_DRIVE_LETTERS];
    PUNICODE_STRING wszLocalConnectionName[_MRX_MAX_DRIVE_LETTERS];
    FAST_MUTEX mtxLocalCon;

    /* The HGCM client information. */
    VBGLSFCLIENT hgcmClient;

    /* Saved pointer to the original IRP_MJ_DEVICE_CONTROL handler. */
    NTSTATUS (* pfnRDBSSDeviceControl) (PDEVICE_OBJECT pDevObj, PIRP pIrp);

} MRX_VBOX_DEVICE_EXTENSION, *PMRX_VBOX_DEVICE_EXTENSION;

/*
 * The shared folders NET_ROOT extension.
 */
typedef struct _MRX_VBOX_NETROOT_EXTENSION
{
    /* The pointert to HGCM client information in device extension. */
    VBGLSFCLIENT *phgcmClient;

    /* The shared folder map handle of this netroot. */
    VBGLSFMAP map;
} MRX_VBOX_NETROOT_EXTENSION, *PMRX_VBOX_NETROOT_EXTENSION;

#define VBOX_FOBX_F_INFO_CREATION_TIME   0x01
#define VBOX_FOBX_F_INFO_LASTACCESS_TIME 0x02
#define VBOX_FOBX_F_INFO_LASTWRITE_TIME  0x04
#define VBOX_FOBX_F_INFO_CHANGE_TIME     0x08
#define VBOX_FOBX_F_INFO_ATTRIBUTES      0x10

/*
 * The shared folders file extension.
 */
typedef struct _MRX_VBOX_FOBX_
{
    SHFLHANDLE hFile;
    PMRX_SRV_CALL pSrvCall;
    FILE_BASIC_INFORMATION FileBasicInfo;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    BOOLEAN fKeepCreationTime;
    BOOLEAN fKeepLastAccessTime;
    BOOLEAN fKeepLastWriteTime;
    BOOLEAN fKeepChangeTime;
    BYTE SetFileInfoOnCloseFlags;
} MRX_VBOX_FOBX, *PMRX_VBOX_FOBX;

#define VBoxMRxGetDeviceExtension(RxContext) \
        (PMRX_VBOX_DEVICE_EXTENSION)((PBYTE)(RxContext->RxDeviceObject) + sizeof(RDBSS_DEVICE_OBJECT))

#define VBoxMRxGetNetRootExtension(pNetRoot) \
        (((pNetRoot) == NULL) ? NULL : (PMRX_VBOX_NETROOT_EXTENSION)((pNetRoot)->Context))

#define VBoxMRxGetSrvOpenExtension(pSrvOpen)  \
        (((pSrvOpen) == NULL) ? NULL : (PMRX_VBOX_SRV_OPEN)((pSrvOpen)->Context))

#define VBoxMRxGetFileObjectExtension(pFobx)  \
        (((pFobx) == NULL) ? NULL : (PMRX_VBOX_FOBX)((pFobx)->Context))

/*
 * Prototypes for the dispatch table routines.
 */
NTSTATUS VBoxMRxStart(IN OUT struct _RX_CONTEXT * RxContext,
                      IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject);
NTSTATUS VBoxMRxStop(IN OUT struct _RX_CONTEXT * RxContext,
                     IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject);

NTSTATUS VBoxMRxCreate(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxCollapseOpen(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxShouldTryToCollapseThisOpen(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxFlush(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxTruncate(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxCleanupFobx(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxCloseSrvOpen(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxDeallocateForFcb(IN OUT PMRX_FCB pFcb);
NTSTATUS VBoxMRxDeallocateForFobx(IN OUT PMRX_FOBX pFobx);
NTSTATUS VBoxMRxForceClosed(IN OUT PMRX_SRV_OPEN SrvOpen);

NTSTATUS VBoxMRxQueryDirectory(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxQueryFileInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetFileInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetFileInfoAtCleanup(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxQueryEaInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetEaInfo(IN OUT struct _RX_CONTEXT * RxContext);
NTSTATUS VBoxMRxQuerySdInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetSdInfo(IN OUT struct _RX_CONTEXT * RxContext);
NTSTATUS VBoxMRxQueryVolumeInfo(IN OUT PRX_CONTEXT RxContext);

NTSTATUS VBoxMRxComputeNewBufferingState(IN OUT PMRX_SRV_OPEN pSrvOpen,
                                         IN PVOID pMRxContext,
                                         OUT ULONG *pNewBufferingState);

NTSTATUS VBoxMRxRead(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxWrite(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxLocks(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxFsCtl(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxIoCtl(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxNotifyChangeDirectory(IN OUT PRX_CONTEXT RxContext);

ULONG NTAPI VBoxMRxExtendStub(IN OUT struct _RX_CONTEXT * RxContext,
                              IN OUT PLARGE_INTEGER pNewFileSize,
                              OUT PLARGE_INTEGER pNewAllocationSize);
NTSTATUS VBoxMRxCompleteBufferingStateChangeRequest(IN OUT PRX_CONTEXT RxContext,
                                                    IN OUT PMRX_SRV_OPEN SrvOpen,
                                                    IN PVOID pContext);

NTSTATUS VBoxMRxCreateVNetRoot(IN OUT PMRX_CREATENETROOT_CONTEXT pContext);
NTSTATUS VBoxMRxFinalizeVNetRoot(IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
                                 IN PBOOLEAN ForceDisconnect);
NTSTATUS VBoxMRxFinalizeNetRoot(IN OUT PMRX_NET_ROOT pNetRoot,
                                IN PBOOLEAN ForceDisconnect);
NTSTATUS VBoxMRxUpdateNetRootState(IN PMRX_NET_ROOT pNetRoot);
VOID VBoxMRxExtractNetRootName(IN PUNICODE_STRING FilePathName,
                               IN PMRX_SRV_CALL SrvCall,
                               OUT PUNICODE_STRING NetRootName,
                               OUT PUNICODE_STRING RestOfName OPTIONAL);

NTSTATUS VBoxMRxCreateSrvCall(PMRX_SRV_CALL pSrvCall,
                              PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext);
NTSTATUS VBoxMRxSrvCallWinnerNotify(IN OUT PMRX_SRV_CALL pSrvCall,
                                    IN BOOLEAN ThisMinirdrIsTheWinner,
                                    IN OUT PVOID pSrvCallContext);
NTSTATUS VBoxMRxFinalizeSrvCall(PMRX_SRV_CALL pSrvCall,
                                BOOLEAN Force);

NTSTATUS VBoxMRxDevFcbXXXControlFile(IN OUT PRX_CONTEXT RxContext);

/*
 * Support functions.
 */
NTSTATUS vbsfDeleteConnection(IN PRX_CONTEXT RxContext,
                              OUT PBOOLEAN PostToFsp);
NTSTATUS vbsfCreateConnection(IN PRX_CONTEXT RxContext,
                              OUT PBOOLEAN PostToFsp);

NTSTATUS vbsfSetEndOfFile(IN OUT struct _RX_CONTEXT * RxContext,
                          IN OUT PLARGE_INTEGER pNewFileSize,
                          OUT PLARGE_INTEGER pNewAllocationSize);
NTSTATUS vbsfRename(IN PRX_CONTEXT RxContext,
                    IN FILE_INFORMATION_CLASS FileInformationClass,
                    IN PVOID pBuffer,
                    IN ULONG BufferLength);
NTSTATUS vbsfRemove(IN PRX_CONTEXT RxContext);
NTSTATUS vbsfCloseFileHandle(PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension,
                             PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension,
                             PMRX_VBOX_FOBX pVBoxFobx);

#endif /* VBSF_H */

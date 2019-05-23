/* $Id: file.c $ */
/** @file
 * VirtualBox Windows Guest Shared Folders - File System Driver file routines.
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

#include "vbsf.h"
#include <iprt/fs.h>
#include <iprt/mem.h>


/* How much data to transfer in one HGCM request. */
#define VBSF_MAX_READ_WRITE_PAGES 256


typedef int FNVBSFTRANSFERBUFFER(PVBGLSFCLIENT pClient, PVBGLSFMAP pMap, SHFLHANDLE hFile,
                                 uint64_t offset, uint32_t *pcbBuffer,
                                 uint8_t *pBuffer, bool fLocked);
typedef FNVBSFTRANSFERBUFFER *PFNVBSFTRANSFERBUFFER;

typedef int FNVBSFTRANSFERPAGES(PVBGLSFCLIENT pClient, PVBGLSFMAP pMap, SHFLHANDLE hFile,
                                uint64_t offset, uint32_t *pcbBuffer,
                                uint16_t offFirstPage, uint16_t cPages, RTGCPHYS64 *paPages);
typedef FNVBSFTRANSFERPAGES *PFNVBSFTRANSFERPAGES;


static int vbsfTransferBufferRead(PVBGLSFCLIENT pClient, PVBGLSFMAP pMap, SHFLHANDLE hFile,
                                  uint64_t offset, uint32_t *pcbBuffer,
                                  uint8_t *pBuffer, bool fLocked)
{
    return VbglR0SfRead(pClient, pMap, hFile, offset, pcbBuffer, pBuffer, fLocked);
}

static int vbsfTransferBufferWrite(PVBGLSFCLIENT pClient, PVBGLSFMAP pMap, SHFLHANDLE hFile,
                                   uint64_t offset, uint32_t *pcbBuffer,
                                   uint8_t *pBuffer, bool fLocked)
{
    return VbglR0SfWrite(pClient, pMap, hFile, offset, pcbBuffer, pBuffer, fLocked);
}

static int vbsfTransferPagesRead(PVBGLSFCLIENT pClient, PVBGLSFMAP pMap, SHFLHANDLE hFile,
                                 uint64_t offset, uint32_t *pcbBuffer,
                                 uint16_t offFirstPage, uint16_t cPages, RTGCPHYS64 *paPages)
{
    return VbglR0SfReadPageList(pClient, pMap, hFile, offset, pcbBuffer, offFirstPage, cPages, paPages);
}

static int vbsfTransferPagesWrite(PVBGLSFCLIENT pClient, PVBGLSFMAP pMap, SHFLHANDLE hFile,
                                  uint64_t offset, uint32_t *pcbBuffer,
                                  uint16_t offFirstPage, uint16_t cPages, RTGCPHYS64 *paPages)
{
    return VbglR0SfWritePageList(pClient, pMap, hFile, offset, pcbBuffer, offFirstPage, cPages, paPages);
}


typedef struct VBSFTRANSFERCTX
{
    PVBGLSFCLIENT pClient;
    PVBGLSFMAP pMap;
    SHFLHANDLE hFile;
    uint64_t offset;
    uint32_t cbData;

    PMDL pMdl;
    uint8_t *pBuffer;
    bool fLocked;

    PFNVBSFTRANSFERBUFFER pfnTransferBuffer;
    PFNVBSFTRANSFERPAGES pfnTransferPages;
} VBSFTRANSFERCTX;


static int vbsfTransferCommon(VBSFTRANSFERCTX *pCtx)
{
    int rc = VINF_SUCCESS;
    BOOLEAN fProcessed = FALSE;

    uint32_t cbTransferred = 0;

    uint32_t cbToTransfer;
    uint32_t cbIO;

    /** @todo Remove the test and the fall-back path.  VbglR0CanUsePhysPageList()
     *        returns true for any host version after 3.0, i.e. further back than
     *        we support. */
    if (VbglR0CanUsePhysPageList())
    {
        ULONG offFirstPage = MmGetMdlByteOffset(pCtx->pMdl);
        ULONG cPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(pCtx->pMdl), pCtx->cbData);
        ULONG cPagesToTransfer = RT_MIN(cPages, VBSF_MAX_READ_WRITE_PAGES);
        RTGCPHYS64 *paPages = (RTGCPHYS64 *)RTMemTmpAlloc(cPagesToTransfer * sizeof(RTGCPHYS64));

        Log(("VBOXSF: vbsfTransferCommon: using page list: %d pages, offset 0x%03X\n", cPages, offFirstPage));

        if (paPages)
        {
            PPFN_NUMBER paPfns = MmGetMdlPfnArray(pCtx->pMdl);
            ULONG cPagesTransferred = 0;
            cbTransferred = 0;

            while (cPagesToTransfer != 0)
            {
                ULONG iPage;
                cbToTransfer = cPagesToTransfer * PAGE_SIZE - offFirstPage;

                if (cbToTransfer > pCtx->cbData - cbTransferred)
                    cbToTransfer = pCtx->cbData - cbTransferred;

                if (cbToTransfer == 0)
                {
                    /* Nothing to transfer. */
                    break;
                }

                cbIO = cbToTransfer;

                Log(("VBOXSF: vbsfTransferCommon: transferring %d pages at %d; %d bytes at %d\n",
                     cPagesToTransfer, cPagesTransferred, cbToTransfer, cbTransferred));

                for (iPage = 0; iPage < cPagesToTransfer; iPage++)
                    paPages[iPage] = (RTGCPHYS64)paPfns[iPage + cPagesTransferred] << PAGE_SHIFT;

                rc = pCtx->pfnTransferPages(pCtx->pClient, pCtx->pMap, pCtx->hFile,
                                            pCtx->offset + cbTransferred, &cbIO,
                                            (uint16_t)offFirstPage, (uint16_t)cPagesToTransfer, paPages);
                if (RT_FAILURE(rc))
                {
                    Log(("VBOXSF: vbsfTransferCommon: pfnTransferPages %Rrc, cbTransferred %d\n", rc, cbTransferred));

                    /* If some data was transferred, then it is no error. */
                    if (cbTransferred > 0)
                        rc = VINF_SUCCESS;

                    break;
                }

                cbTransferred += cbIO;

                if (cbToTransfer < cbIO)
                {
                    /* Transferred less than requested, do not continue with the possibly remaining data. */
                    break;
                }

                cPagesTransferred += cPagesToTransfer;
                offFirstPage = 0;

                cPagesToTransfer = cPages - cPagesTransferred;
                if (cPagesToTransfer > VBSF_MAX_READ_WRITE_PAGES)
                    cPagesToTransfer = VBSF_MAX_READ_WRITE_PAGES;
            }

            RTMemTmpFree(paPages);

            fProcessed = TRUE;
        }
    }

    if (fProcessed != TRUE)
    {
        /* Split large transfers. */
        cbTransferred = 0;
        cbToTransfer = RT_MIN(pCtx->cbData, VBSF_MAX_READ_WRITE_PAGES * PAGE_SIZE);

        /* Page list not supported or a fallback. */
        Log(("VBOXSF: vbsfTransferCommon: using linear address\n"));

        while (cbToTransfer != 0)
        {
            cbIO = cbToTransfer;

            Log(("VBOXSF: vbsfTransferCommon: transferring %d bytes at %d\n",
                 cbToTransfer, cbTransferred));

            rc = pCtx->pfnTransferBuffer(pCtx->pClient, pCtx->pMap, pCtx->hFile,
                                         pCtx->offset + cbTransferred, &cbIO,
                                         pCtx->pBuffer + cbTransferred, true /* locked */);

            if (RT_FAILURE(rc))
            {
                Log(("VBOXSF: vbsfTransferCommon: pfnTransferBuffer %Rrc, cbTransferred %d\n", rc, cbTransferred));

                /* If some data was transferred, then it is no error. */
                if (cbTransferred > 0)
                    rc = VINF_SUCCESS;

                break;
            }

            cbTransferred += cbIO;

            if (cbToTransfer < cbIO)
            {
                /* Transferred less than requested, do not continue with the possibly remaining data. */
                break;
            }

            cbToTransfer = pCtx->cbData - cbTransferred;
            if (cbToTransfer > VBSF_MAX_READ_WRITE_PAGES * PAGE_SIZE)
                cbToTransfer = VBSF_MAX_READ_WRITE_PAGES * PAGE_SIZE;
        }
    }

    pCtx->cbData = cbTransferred;

    return rc;
}

static NTSTATUS vbsfReadInternal(IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    VBSFTRANSFERCTX ctx;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PMDL BufferMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
    uint32_t ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    RXVBO ByteOffset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

    PVOID pbUserBuffer = RxLowIoGetBufferAddress(RxContext);

    int vboxRC;

#ifdef LOG_ENABLED
    BOOLEAN AsyncIo = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
#endif
    LONGLONG FileSize;

    RxGetFileSizeWithLock((PFCB)capFcb, &FileSize);

    Log(("VBOXSF: vbsfReadInternal: AsyncIo = %d, Fcb->FileSize = 0x%RX64\n",
         AsyncIo, capFcb->Header.FileSize.QuadPart));
    Log(("VBOXSF: vbsfReadInternal: UserBuffer %p, BufferMdl %p\n",
         pbUserBuffer, BufferMdl));
    Log(("VBOXSF: vbsfReadInternal: ByteCount 0x%X, ByteOffset 0x%RX64, FileSize 0x%RX64\n",
         ByteCount, ByteOffset, FileSize));

    /** @todo check if this is necessary. */
#ifdef FCB_STATE_READCACHING_ENABLED    /* Correct spelling for Vista 6001 SDK. */
    if (!FlagOn(capFcb->FcbState, FCB_STATE_READCACHING_ENABLED))
#else
    if (!FlagOn(capFcb->FcbState, FCB_STATE_READCACHEING_ENABLED))
#endif
    {
        if (ByteOffset >= FileSize)
        {
            Log(("VBOXSF: vbsfReadInternal: EOF\n"));
            return STATUS_END_OF_FILE;
        }

        if (ByteCount > FileSize - ByteOffset)
            ByteCount = (ULONG)(FileSize - ByteOffset);
    }

    /** @todo read 0 bytes == always success? */
    if (   !BufferMdl
        || ByteCount == 0)
    {
        AssertFailed();
        return STATUS_INVALID_PARAMETER;
    }

    ctx.pClient = &pDeviceExtension->hgcmClient;
    ctx.pMap    = &pNetRootExtension->map;
    ctx.hFile   = pVBoxFobx->hFile;
    ctx.offset  = (uint64_t)ByteOffset;
    ctx.cbData  = ByteCount;
    ctx.pMdl    = BufferMdl;
    ctx.pBuffer = (uint8_t *)pbUserBuffer;
    ctx.fLocked = true;
    ctx.pfnTransferBuffer = vbsfTransferBufferRead;
    ctx.pfnTransferPages = vbsfTransferPagesRead;

    vboxRC = vbsfTransferCommon(&ctx);

    ByteCount = ctx.cbData;

    Status = VBoxErrorToNTStatus(vboxRC);

    if (Status != STATUS_SUCCESS)
    {
        /* Nothing read. */
        ByteCount = 0;
    }

    RxContext->InformationToReturn = ByteCount;

    Log(("VBOXSF: vbsfReadInternal: Status = 0x%08X, ByteCount = 0x%X\n",
         Status, ByteCount));

    return Status;
}


static VOID vbsfReadWorker(VOID *pv)
{
    PRX_CONTEXT RxContext = (PRX_CONTEXT)pv;

    Log(("VBOXSF: vbsfReadWorker: calling the worker\n"));

    RxContext->IoStatusBlock.Status = vbsfReadInternal(RxContext);

    Log(("VBOXSF: vbsfReadWorker: Status 0x%08X\n",
         RxContext->IoStatusBlock.Status));

    RxLowIoCompletion(RxContext);
}


NTSTATUS VBoxMRxRead(IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = RxDispatchToWorkerThread(VBoxMRxDeviceObject, DelayedWorkQueue,
                                               vbsfReadWorker,
                                               RxContext);

    Log(("VBOXSF: MRxRead: RxDispatchToWorkerThread: Status 0x%08X\n", Status));

    if (Status == STATUS_SUCCESS)
        Status = STATUS_PENDING;

    return Status;
}

static NTSTATUS vbsfWriteInternal(IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    VBSFTRANSFERCTX ctx;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);

    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PMDL BufferMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
    uint32_t ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    RXVBO ByteOffset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

    PVOID pbUserBuffer = RxLowIoGetBufferAddress(RxContext);

    int vboxRC;

#ifdef LOG_ENABLED
    BOOLEAN AsyncIo = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
#endif
    LONGLONG FileSize;

    RxGetFileSizeWithLock((PFCB)capFcb, &FileSize);

    Log(("VBOXSF: vbsfWriteInternal: AsyncIo = %d, Fcb->FileSize = 0x%RX64\n",
         AsyncIo, capFcb->Header.FileSize.QuadPart));
    Log(("VBOXSF: vbsfWriteInternal: UserBuffer %p, BufferMdl %p\n",
         pbUserBuffer, BufferMdl));
    Log(("VBOXSF: vbsfWriteInternal: ByteCount is 0x%X, ByteOffset is 0x%RX64, FileSize 0x%RX64\n",
         ByteCount, ByteOffset, FileSize));

    /** @todo allow to write 0 bytes. */
    if (   !BufferMdl
        || ByteCount == 0)
    {
        AssertFailed();
        return STATUS_INVALID_PARAMETER;
    }

    ctx.pClient = &pDeviceExtension->hgcmClient;
    ctx.pMap    = &pNetRootExtension->map;
    ctx.hFile   = pVBoxFobx->hFile;
    ctx.offset  = (uint64_t)ByteOffset;
    ctx.cbData  = ByteCount;
    ctx.pMdl    = BufferMdl;
    ctx.pBuffer = (uint8_t *)pbUserBuffer;
    ctx.fLocked = true;
    ctx.pfnTransferBuffer = vbsfTransferBufferWrite;
    ctx.pfnTransferPages = vbsfTransferPagesWrite;

    vboxRC = vbsfTransferCommon(&ctx);

    ByteCount = ctx.cbData;

    Status = VBoxErrorToNTStatus(vboxRC);

    if (Status != STATUS_SUCCESS)
    {
        /* Nothing written. */
        ByteCount = 0;
    }

    RxContext->InformationToReturn = ByteCount;

    Log(("VBOXSF: vbsfWriteInternal: Status = 0x%08X, ByteCount = 0x%X\n",
         Status, ByteCount));

    return Status;
}

static VOID vbsfWriteWorker(VOID *pv)
{
    PRX_CONTEXT RxContext = (PRX_CONTEXT)pv;

    Log(("VBOXSF: vbsfWriteWorker: calling the worker\n"));

    RxContext->IoStatusBlock.Status = vbsfWriteInternal(RxContext);

    Log(("VBOXSF: vbsfWriteWorker: Status 0x%08X\n",
         RxContext->IoStatusBlock.Status));

    RxLowIoCompletion(RxContext);
}


NTSTATUS VBoxMRxWrite(IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = RxDispatchToWorkerThread(VBoxMRxDeviceObject, DelayedWorkQueue,
                                               vbsfWriteWorker,
                                               RxContext);

    Log(("VBOXSF: MRxWrite: RxDispatchToWorkerThread: Status 0x%08X\n",
         Status));

    if (Status == STATUS_SUCCESS)
        Status = STATUS_PENDING;

    return Status;
}


NTSTATUS VBoxMRxLocks(IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    uint32_t fu32Lock = 0;
    int vboxRC;

    Log(("VBOXSF: MRxLocks: Operation %d\n",
         LowIoContext->Operation));

    switch (LowIoContext->Operation)
    {
        default:
            AssertMsgFailed(("VBOXSF: MRxLocks: Unsupported lock/unlock type %d detected!\n",
                             LowIoContext->Operation));
            return STATUS_NOT_IMPLEMENTED;

        case LOWIO_OP_UNLOCK_MULTIPLE:
            /** @todo Remove multiple locks listed in LowIoContext.ParamsFor.Locks.LockList. */
            Log(("VBOXSF: MRxLocks: Unsupported LOWIO_OP_UNLOCK_MULTIPLE!\n",
                 LowIoContext->Operation));
            return STATUS_NOT_IMPLEMENTED;

        case LOWIO_OP_SHAREDLOCK:
            fu32Lock = SHFL_LOCK_SHARED | SHFL_LOCK_PARTIAL;
            break;

        case LOWIO_OP_EXCLUSIVELOCK:
            fu32Lock = SHFL_LOCK_EXCLUSIVE | SHFL_LOCK_PARTIAL;
            break;

        case LOWIO_OP_UNLOCK:
            fu32Lock = SHFL_LOCK_CANCEL | SHFL_LOCK_PARTIAL;
            break;
    }

    if (LowIoContext->ParamsFor.Locks.Flags & LOWIO_LOCKSFLAG_FAIL_IMMEDIATELY)
        fu32Lock |= SHFL_LOCK_NOWAIT;
    else
        fu32Lock |= SHFL_LOCK_WAIT;

    vboxRC = VbglR0SfLock(&pDeviceExtension->hgcmClient, &pNetRootExtension->map, pVBoxFobx->hFile,
                          LowIoContext->ParamsFor.Locks.ByteOffset, LowIoContext->ParamsFor.Locks.Length, fu32Lock);

    Status = VBoxErrorToNTStatus(vboxRC);

    Log(("VBOXSF: MRxLocks: Returned 0x%08X\n", Status));
    return Status;
}

NTSTATUS VBoxMRxCompleteBufferingStateChangeRequest(IN OUT PRX_CONTEXT RxContext, IN OUT PMRX_SRV_OPEN SrvOpen,
                                                    IN PVOID pvContext)
{
    RT_NOREF(RxContext, SrvOpen, pvContext);
    Log(("VBOXSF: MRxCompleteBufferingStateChangeRequest: not implemented\n"));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS VBoxMRxFlush (IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);

    int vboxRC;

    Log(("VBOXSF: MRxFlush\n"));

    /* Do the actual flushing of file buffers */
    vboxRC = VbglR0SfFlush(&pDeviceExtension->hgcmClient, &pNetRootExtension->map, pVBoxFobx->hFile);

    Status = VBoxErrorToNTStatus(vboxRC);

    Log(("VBOXSF: MRxFlush: Returned 0x%08X\n", Status));
    return Status;
}

NTSTATUS vbsfSetEndOfFile(IN OUT struct _RX_CONTEXT * RxContext,
                          IN OUT PLARGE_INTEGER pNewFileSize,
                          OUT PLARGE_INTEGER pNewAllocationSize)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_VBOX_DEVICE_EXTENSION pDeviceExtension = VBoxMRxGetDeviceExtension(RxContext);
    PMRX_VBOX_NETROOT_EXTENSION pNetRootExtension = VBoxMRxGetNetRootExtension(capFcb->pNetRoot);
    PMRX_VBOX_FOBX pVBoxFobx = VBoxMRxGetFileObjectExtension(capFobx);

    PSHFLFSOBJINFO pObjInfo;
    uint32_t cbBuffer;
    int vboxRC;

    Log(("VBOXSF: vbsfSetEndOfFile: New size = %RX64 (%p), pNewAllocationSize = %p\n",
         pNewFileSize->QuadPart, pNewFileSize, pNewAllocationSize));

    Assert(pVBoxFobx && pNetRootExtension && pDeviceExtension);

    cbBuffer = sizeof(SHFLFSOBJINFO);
    pObjInfo = (SHFLFSOBJINFO *)vbsfAllocNonPagedMem(cbBuffer);
    if (!pObjInfo)
    {
        AssertFailed();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pObjInfo, cbBuffer);
    pObjInfo->cbObject = pNewFileSize->QuadPart;

    vboxRC = VbglR0SfFsInfo(&pDeviceExtension->hgcmClient, &pNetRootExtension->map, pVBoxFobx->hFile,
                            SHFL_INFO_SET | SHFL_INFO_SIZE, &cbBuffer, (PSHFLDIRINFO)pObjInfo);

    Log(("VBOXSF: vbsfSetEndOfFile: VbglR0SfFsInfo returned %Rrc\n", vboxRC));

    Status = VBoxErrorToNTStatus(vboxRC);
    if (Status == STATUS_SUCCESS)
    {
        Log(("VBOXSF: vbsfSetEndOfFile: VbglR0SfFsInfo new allocation size = %RX64\n",
             pObjInfo->cbAllocated));

        /* Return new allocation size */
        pNewAllocationSize->QuadPart = pObjInfo->cbAllocated;
    }

    if (pObjInfo)
        vbsfFreeNonPagedMem(pObjInfo);

    Log(("VBOXSF: vbsfSetEndOfFile: Returned 0x%08X\n", Status));
    return Status;
}

/** See PMRX_EXTENDFILE_CALLDOWN in ddk/mrx.h
 *
 * Documentation says it returns STATUS_SUCCESS on success and an error
 * status on failure, so the ULONG return type is probably just a typo that
 * stuck.
 */
ULONG NTAPI VBoxMRxExtendStub(IN OUT struct _RX_CONTEXT * RxContext, IN OUT PLARGE_INTEGER pNewFileSize,
                              OUT PLARGE_INTEGER pNewAllocationSize)
{
    RT_NOREF(RxContext);

    /* Note: On Windows hosts vbsfSetEndOfFile returns ACCESS_DENIED if the file has been
     *       opened in APPEND mode. Writes to a file will extend it anyway, therefore it is
     *       better to not call the host at all and tell the caller that the file was extended.
     */
    Log(("VBOXSF: MRxExtendStub: new size = %RX64\n",
         pNewFileSize->QuadPart));

    pNewAllocationSize->QuadPart = pNewFileSize->QuadPart;

    return STATUS_SUCCESS;
}


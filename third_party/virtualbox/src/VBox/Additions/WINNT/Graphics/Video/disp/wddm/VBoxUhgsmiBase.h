/* $Id: VBoxUhgsmiBase.h $ */
/** @file
 * VBoxVideo Display D3D User mode dll
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

#ifndef ___VBoxUhgsmiBase_h__
#define ___VBoxUhgsmiBase_h__

#include <VBoxUhgsmi.h>
#include <VBoxCrHgsmi.h>

#include <iprt/win/windows.h>
#include <D3dkmthk.h>
//#include <D3dumddi.h>
#include "common/wddm/VBoxMPIf.h"


#ifndef VBOX_WITH_CRHGSMI
#error "VBOX_WITH_CRHGSMI not defined!"
#endif

#if 0
typedef DECLCALLBACK(int) FNVBOXCRHGSMI_CTLCON_CALL(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, struct VBGLIOCHGCMCALL *pCallInfo, int cbCallInfo);
typedef FNVBOXCRHGSMI_CTLCON_CALL *PFNVBOXCRHGSMI_CTLCON_CALL;

#define vboxCrHgsmiPrivateCtlConCall(_pHgsmi, _pCallInfo, _cbCallInfo) (_pHgsmi->pfnCtlConCall((_pHgsmi), (_pCallInfo), (_cbCallInfo)))


typedef DECLCALLBACK(int) FNVBOXCRHGSMI_CTLCON_GETCLIENTID(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, uint32_t *pu32ClientID);
typedef FNVBOXCRHGSMI_CTLCON_GETCLIENTID *PFNVBOXCRHGSMI_CTLCON_GETCLIENTID;

#define vboxCrHgsmiPrivateCtlConGetClientID(_pHgsmi, _pu32ClientID) (_pHgsmi->pfnCtlConGetClientID((_pHgsmi), (_pu32ClientID)))
#else
int vboxCrHgsmiPrivateCtlConCall(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, struct VBGLIOCHGCMCALL *pCallInfo, int cbCallInfo);
int vboxCrHgsmiPrivateCtlConGetClientID(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, uint32_t *pu32ClientID);
int vboxCrHgsmiPrivateCtlConGetHostCaps(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, uint32_t *pu32HostCaps);
#endif

typedef DECLCALLBACK(int) FNVBOXCRHGSMI_ESCAPE(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, void *pvData, uint32_t cbData, BOOL fHwAccess);
typedef FNVBOXCRHGSMI_ESCAPE *PFNVBOXCRHGSMI_ESCAPE;

#define vboxCrHgsmiPrivateEscape(_pHgsmi, _pvData, _cbData, _fHwAccess) (_pHgsmi->pfnEscape((_pHgsmi), (_pvData), (_cbData), (_fHwAccess)))

typedef struct VBOXUHGSMI_PRIVATE_BASE
{
    VBOXUHGSMI Base;
    PFNVBOXCRHGSMI_ESCAPE pfnEscape;
} VBOXUHGSMI_PRIVATE_BASE, *PVBOXUHGSMI_PRIVATE_BASE;

typedef struct VBOXUHGSMI_BUFFER_PRIVATE_BASE
{
    VBOXUHGSMI_BUFFER Base;
    PVBOXUHGSMI_PRIVATE_BASE pHgsmi;
} VBOXUHGSMI_BUFFER_PRIVATE_BASE, *PVBOXUHGSMI_BUFFER_PRIVATE_BASE;

typedef struct VBOXUHGSMI_BUFFER_PRIVATE_ESC_BASE
{
    VBOXUHGSMI_BUFFER_PRIVATE_BASE BasePrivate;
    VBOXVIDEOCM_UM_ALLOC Alloc;
    HANDLE hSynch;
} VBOXUHGSMI_BUFFER_PRIVATE_ESC_BASE, *PVBOXUHGSMI_BUFFER_PRIVATE_ESC_BASE;

typedef struct VBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE
{
    VBOXUHGSMI_BUFFER_PRIVATE_BASE BasePrivate;
    D3DKMT_HANDLE hAllocation;
    UINT aLockPageIndices[1];
} VBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE, *PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE;

#define VBOXUHGSMIBASE_GET_PRIVATE(_p, _t) ((_t*)(((uint8_t*)_p) - RT_OFFSETOF(_t, Base)))
#define VBOXUHGSMIBASE_GET(_p) VBOXUHGSMIBASE_GET_PRIVATE(_p, VBOXUHGSMI_PRIVATE_BASE)
#define VBOXUHGSMIBASE_GET_BUFFER(_p) VBOXUHGSMIBASE_GET_PRIVATE(_p, VBOXUHGSMI_BUFFER_PRIVATE_BASE)

#define VBOXUHGSMIPRIVATEBASE_GET_PRIVATE(_p, _t) ((_t*)(((uint8_t*)_p) - RT_OFFSETOF(_t, BasePrivate.Base)))
#define VBOXUHGSMIESCBASE_GET_BUFFER(_p) VBOXUHGSMIPRIVATEBASE_GET_PRIVATE(_p, VBOXUHGSMI_BUFFER_PRIVATE_ESC_BASE)
#define VBOXUHGSMDXALLOCBASE_GET_BUFFER(_p) VBOXUHGSMIPRIVATEBASE_GET_PRIVATE(_p, VBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE)

DECLINLINE(int) vboxUhgsmiBaseDxLockData(PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pPrivate, uint32_t offLock, uint32_t cbLock,
                                         VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags, D3DDDICB_LOCKFLAGS *pfFlags, UINT *pNumPages)
{
    PVBOXUHGSMI_BUFFER pBuf = &pPrivate->BasePrivate.Base;
    D3DDDICB_LOCKFLAGS fLockFlags;
    fLockFlags.Value = 0;
    if (fFlags.bLockEntire)
    {
        Assert(!offLock);
        fLockFlags.LockEntire = 1;
    }
    else
    {
        AssertReturn(cbLock, VERR_INVALID_PARAMETER);
        AssertReturn(offLock + cbLock <= pBuf->cbBuffer, VERR_INVALID_PARAMETER);

        uint32_t iFirstPage = offLock >> 12;
        uint32_t iAfterLastPage = (cbLock + 0xfff) >> 12;
        uint32_t cPages = iAfterLastPage - iFirstPage;
        uint32_t cBufPages = pBuf->cbBuffer >> 12;
        Assert(cPages <= (cBufPages));

        if (cPages == cBufPages)
        {
            *pNumPages = 0;
            fLockFlags.LockEntire = 1;
        }
        else
        {
            *pNumPages = cPages;
            for (UINT i = 0, j = iFirstPage; i < cPages; ++i, ++j)
            {
                pPrivate->aLockPageIndices[i] = j;
            }
        }

    }

    fLockFlags.ReadOnly = fFlags.bReadOnly;
    fLockFlags.WriteOnly = fFlags.bWriteOnly;
    fLockFlags.DonotWait = fFlags.bDonotWait;
//    fLockFlags.Discard = fFlags.bDiscard;
    *pfFlags = fLockFlags;
    return VINF_SUCCESS;
}

DECLINLINE(void) vboxUhgsmiBaseDxAllocInfoFill(D3DDDI_ALLOCATIONINFO *pDdiAllocInfo, VBOXWDDM_ALLOCINFO *pAllocInfo,
                                               uint32_t cbBuffer, VBOXUHGSMI_BUFFER_TYPE_FLAGS fUhgsmiType)
{
    memset(pDdiAllocInfo, 0, sizeof (*pDdiAllocInfo));
    pDdiAllocInfo->pPrivateDriverData = pAllocInfo;
    pDdiAllocInfo->PrivateDriverDataSize = sizeof (*pAllocInfo);
    memset(pAllocInfo, 0, sizeof (*pAllocInfo));
    pAllocInfo->enmType = VBOXWDDM_ALLOC_TYPE_UMD_HGSMI_BUFFER;
    pAllocInfo->cbBuffer = cbBuffer;
    pAllocInfo->fUhgsmiType = fUhgsmiType;
}

DECLINLINE(int) vboxUhgsmiBaseDxDmaFill(PVBOXUHGSMI_BUFFER_SUBMIT aBuffers, uint32_t cBuffers,
                                        VOID *pCommandBuffer, UINT *pCommandBufferSize,
                                        D3DDDI_ALLOCATIONLIST *pAllocationList, UINT AllocationListSize,
                                        D3DDDI_PATCHLOCATIONLIST *pPatchLocationList, UINT PatchLocationListSize)
{
    const uint32_t cbDmaCmd = RT_OFFSETOF(VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD, aBufInfos[cBuffers]);
    RT_NOREF(pPatchLocationList, PatchLocationListSize);

    AssertReturn(*pCommandBufferSize >= cbDmaCmd, VERR_GENERAL_FAILURE);
    AssertReturn(AllocationListSize >= cBuffers, VERR_GENERAL_FAILURE);

    *pCommandBufferSize = cbDmaCmd;

    PVBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD pHdr = (PVBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD)pCommandBuffer;
    pHdr->Base.enmCmd = VBOXVDMACMD_TYPE_CHROMIUM_CMD;
    pHdr->Base.u32CmdReserved = 0;

    PVBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO pBufSubmInfo = pHdr->aBufInfos;

    for (uint32_t i = 0; i < cBuffers; ++i)
    {
        PVBOXUHGSMI_BUFFER_SUBMIT pBufInfo = &aBuffers[i];
        PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuffer = VBOXUHGSMDXALLOCBASE_GET_BUFFER(pBufInfo->pBuf);

        memset(pAllocationList, 0, sizeof (D3DDDI_ALLOCATIONLIST));
        pAllocationList->hAllocation = pBuffer->hAllocation;
        pAllocationList->Value = 0;
        pAllocationList->WriteOperation = !pBufInfo->fFlags.bHostReadOnly;
        pAllocationList->DoNotRetireInstance = pBufInfo->fFlags.bDoNotRetire;
        if (pBufInfo->fFlags.bEntireBuffer)
        {
            pBufSubmInfo->offData = 0;
            pBufSubmInfo->cbData = pBuffer->BasePrivate.Base.cbBuffer;
        }
        else
        {
            pBufSubmInfo->offData = pBufInfo->offData;
            pBufSubmInfo->cbData = pBufInfo->cbData;
        }

        ++pAllocationList;
        ++pBufSubmInfo;
    }

    return VINF_SUCCESS;
}

DECLCALLBACK(int) vboxUhgsmiBaseEscBufferLock(PVBOXUHGSMI_BUFFER pBuf, uint32_t offLock, uint32_t cbLock, VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags, void**pvLock);
DECLCALLBACK(int) vboxUhgsmiBaseEscBufferUnlock(PVBOXUHGSMI_BUFFER pBuf);
int vboxUhgsmiBaseBufferTerm(PVBOXUHGSMI_BUFFER_PRIVATE_ESC_BASE pBuffer);
static int vboxUhgsmiBaseEventChkCreate(VBOXUHGSMI_BUFFER_TYPE_FLAGS fUhgsmiType, HANDLE *phSynch);
int vboxUhgsmiKmtEscBufferInit(PVBOXUHGSMI_PRIVATE_BASE pPrivate, PVBOXUHGSMI_BUFFER_PRIVATE_ESC_BASE pBuffer, uint32_t cbBuf, VBOXUHGSMI_BUFFER_TYPE_FLAGS fUhgsmiType, PFNVBOXUHGSMI_BUFFER_DESTROY pfnDestroy);
DECLCALLBACK(int) vboxUhgsmiBaseEscBufferSubmit(PVBOXUHGSMI pHgsmi, PVBOXUHGSMI_BUFFER_SUBMIT aBuffers, uint32_t cBuffers);
DECLCALLBACK(int) vboxUhgsmiBaseEscBufferDestroy(PVBOXUHGSMI_BUFFER pBuf);
DECLCALLBACK(int) vboxUhgsmiBaseEscBufferCreate(PVBOXUHGSMI pHgsmi, uint32_t cbBuf, VBOXUHGSMI_BUFFER_TYPE_FLAGS fUhgsmiType, PVBOXUHGSMI_BUFFER* ppBuf);

DECLINLINE(void) vboxUhgsmiBaseInit(PVBOXUHGSMI_PRIVATE_BASE pHgsmi, PFNVBOXCRHGSMI_ESCAPE pfnEscape)
{
    pHgsmi->Base.pfnBufferCreate = vboxUhgsmiBaseEscBufferCreate;
    pHgsmi->Base.pfnBufferSubmit = vboxUhgsmiBaseEscBufferSubmit;
    pHgsmi->pfnEscape = pfnEscape;
}

#endif /* #ifndef ___VBoxUhgsmiBase_h__ */

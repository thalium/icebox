/* $Id: VBoxUhgsmiDisp.cpp $ */
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

#include "VBoxDispD3DCmn.h"

#define VBOXUHGSMID3D_GET_PRIVATE(_p, _t) ((_t*)(((uint8_t*)_p) - RT_OFFSETOF(_t, BasePrivate.Base)))
#define VBOXUHGSMID3D_GET(_p) VBOXUHGSMID3D_GET_PRIVATE(_p, VBOXUHGSMI_PRIVATE_D3D)

#include <iprt/mem.h>
#include <iprt/err.h>

DECLCALLBACK(int) vboxUhgsmiD3DBufferDestroy(PVBOXUHGSMI_BUFFER pBuf)
{
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuffer = VBOXUHGSMDXALLOCBASE_GET_BUFFER(pBuf);
    struct VBOXWDDMDISP_DEVICE *pDevice = VBOXUHGSMID3D_GET(pBuffer->BasePrivate.pHgsmi)->pDevice;
    D3DDDICB_DEALLOCATE DdiDealloc;
    DdiDealloc.hResource = 0;
    DdiDealloc.NumAllocations = 1;
    DdiDealloc.HandleList = &pBuffer->hAllocation;
    HRESULT hr = pDevice->RtCallbacks.pfnDeallocateCb(pDevice->hDevice, &DdiDealloc);
    if (hr == S_OK)
    {
#ifdef DEBUG_misha
        memset(pBuffer, 0, sizeof (*pBuffer));
#endif
        RTMemFree(pBuffer);
        return VINF_SUCCESS;
    }

    WARN(("pfnDeallocateCb failed, hr %#x", hr));
    return VERR_GENERAL_FAILURE;
}

/* typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_LOCK(PVBOXUHGSMI_BUFFER pBuf, uint32_t offLock, uint32_t cbLock, VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags, void**pvLock); */
DECLCALLBACK(int) vboxUhgsmiD3DBufferLock(PVBOXUHGSMI_BUFFER pBuf, uint32_t offLock, uint32_t cbLock, VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags, void**pvLock)
{
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuffer = VBOXUHGSMDXALLOCBASE_GET_BUFFER(pBuf);
    struct VBOXWDDMDISP_DEVICE *pDevice = VBOXUHGSMID3D_GET(pBuffer->BasePrivate.pHgsmi)->pDevice;
    D3DDDICB_LOCK DdiLock = {0};
    DdiLock.hAllocation = pBuffer->hAllocation;
    DdiLock.PrivateDriverData = 0;

    int rc = vboxUhgsmiBaseDxLockData(pBuffer, offLock, cbLock, fFlags,
                                         &DdiLock.Flags, &DdiLock.NumPages);
    if (!RT_SUCCESS(rc))
    {
        WARN(("vboxUhgsmiBaseDxLockData failed rc %d", rc));
        return rc;
    }

    if (DdiLock.NumPages)
        DdiLock.pPages = pBuffer->aLockPageIndices;
    else
        DdiLock.pPages = NULL;

    HRESULT hr = pDevice->RtCallbacks.pfnLockCb(pDevice->hDevice, &DdiLock);
    if (hr == S_OK)
    {
        *pvLock = (void*)(((uint8_t*)DdiLock.pData) + (offLock & 0xfff));
        return VINF_SUCCESS;
    }

    WARN(("pfnLockCb failed, hr %#x", hr));
    return VERR_GENERAL_FAILURE;
}

DECLCALLBACK(int) vboxUhgsmiD3DBufferUnlock(PVBOXUHGSMI_BUFFER pBuf)
{
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuffer = VBOXUHGSMDXALLOCBASE_GET_BUFFER(pBuf);
    struct VBOXWDDMDISP_DEVICE *pDevice = VBOXUHGSMID3D_GET(pBuffer->BasePrivate.pHgsmi)->pDevice;
    D3DDDICB_UNLOCK DdiUnlock;
    DdiUnlock.NumAllocations = 1;
    DdiUnlock.phAllocations = &pBuffer->hAllocation;
    HRESULT hr = pDevice->RtCallbacks.pfnUnlockCb(pDevice->hDevice, &DdiUnlock);
    if (hr == S_OK)
        return VINF_SUCCESS;

    WARN(("pfnUnlockCb failed, hr %#x", hr));
    return VERR_GENERAL_FAILURE;
}

/*typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_CREATE(PVBOXUHGSMI pHgsmi, uint32_t cbBuf, VBOXUHGSMI_BUFFER_TYPE_FLAGS fType, PVBOXUHGSMI_BUFFER* ppBuf);*/
DECLCALLBACK(int) vboxUhgsmiD3DBufferCreate(PVBOXUHGSMI pHgsmi, uint32_t cbBuf, VBOXUHGSMI_BUFFER_TYPE_FLAGS fType, PVBOXUHGSMI_BUFFER* ppBuf)
{
    if (!cbBuf)
        return VERR_INVALID_PARAMETER;

    int rc = VINF_SUCCESS;

    cbBuf = VBOXWDDM_ROUNDBOUND(cbBuf, 0x1000);
    Assert(cbBuf);
    uint32_t cPages = cbBuf >> 12;
    Assert(cPages);

    PVBOXUHGSMI_PRIVATE_D3D pPrivate = VBOXUHGSMID3D_GET(pHgsmi);
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuf = (PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE)RTMemAllocZ(RT_OFFSETOF(VBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE, aLockPageIndices[cPages]));
    if (pBuf)
    {
        D3DDDICB_ALLOCATE DdiAlloc;
        D3DDDI_ALLOCATIONINFO DdiAllocInfo;
        VBOXWDDM_ALLOCINFO AllocInfo;

        memset(&DdiAlloc, 0, sizeof (DdiAlloc));
        DdiAlloc.hResource = NULL;
        DdiAlloc.hKMResource = NULL;
        DdiAlloc.NumAllocations = 1;
        DdiAlloc.pAllocationInfo = &DdiAllocInfo;
        vboxUhgsmiBaseDxAllocInfoFill(&DdiAllocInfo, &AllocInfo, cbBuf, fType);

        HRESULT hr = pPrivate->pDevice->RtCallbacks.pfnAllocateCb(pPrivate->pDevice->hDevice, &DdiAlloc);
        if (hr == S_OK)
        {
            Assert(DdiAllocInfo.hAllocation);
            pBuf->BasePrivate.Base.pfnLock = vboxUhgsmiD3DBufferLock;
            pBuf->BasePrivate.Base.pfnUnlock = vboxUhgsmiD3DBufferUnlock;
            pBuf->BasePrivate.Base.pfnDestroy = vboxUhgsmiD3DBufferDestroy;

            pBuf->BasePrivate.Base.fType = fType;
            pBuf->BasePrivate.Base.cbBuffer = cbBuf;

            pBuf->BasePrivate.pHgsmi = &pPrivate->BasePrivate;

            pBuf->hAllocation = DdiAllocInfo.hAllocation;

            *ppBuf = &pBuf->BasePrivate.Base;

            return VINF_SUCCESS;
        }
        else
        {
            WARN(("pfnAllocateCb failed hr %#x"));
            rc = VERR_GENERAL_FAILURE;
        }

        RTMemFree(pBuf);
    }
    else
    {
        WARN(("RTMemAllocZ failed"));
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

/* typedef DECLCALLBACK(int) FNVBOXUHGSMI_BUFFER_SUBMIT(PVBOXUHGSMI pHgsmi, PVBOXUHGSMI_BUFFER_SUBMIT aBuffers, uint32_t cBuffers); */
DECLCALLBACK(int) vboxUhgsmiD3DBufferSubmit(PVBOXUHGSMI pHgsmi, PVBOXUHGSMI_BUFFER_SUBMIT aBuffers, uint32_t cBuffers)
{
    PVBOXUHGSMI_PRIVATE_D3D pHg = VBOXUHGSMID3D_GET(pHgsmi);
    PVBOXWDDMDISP_DEVICE pDevice = pHg->pDevice;
    UINT cbDmaCmd = pDevice->DefaultContext.ContextInfo.CommandBufferSize;
    int rc = vboxUhgsmiBaseDxDmaFill(aBuffers, cBuffers,
            pDevice->DefaultContext.ContextInfo.pCommandBuffer, &cbDmaCmd,
            pDevice->DefaultContext.ContextInfo.pAllocationList, pDevice->DefaultContext.ContextInfo.AllocationListSize,
            pDevice->DefaultContext.ContextInfo.pPatchLocationList, pDevice->DefaultContext.ContextInfo.PatchLocationListSize);
    if (RT_FAILURE(rc))
    {
        WARN(("vboxUhgsmiBaseDxDmaFill failed, rc %d", rc));
        return rc;
    }

    D3DDDICB_RENDER DdiRender = {0};
    DdiRender.CommandLength = cbDmaCmd;
    Assert(DdiRender.CommandLength);
    Assert(DdiRender.CommandLength < UINT32_MAX/2);
    DdiRender.CommandOffset = 0;
    DdiRender.NumAllocations = cBuffers;
    DdiRender.NumPatchLocations = 0;
//    DdiRender.NewCommandBufferSize = sizeof (VBOXVDMACMD) + 4 * (100);
//    DdiRender.NewAllocationListSize = 100;
//    DdiRender.NewPatchLocationListSize = 100;
    DdiRender.hContext = pDevice->DefaultContext.ContextInfo.hContext;

    HRESULT hr = pDevice->RtCallbacks.pfnRenderCb(pDevice->hDevice, &DdiRender);
    if (hr == S_OK)
    {
        pDevice->DefaultContext.ContextInfo.CommandBufferSize = DdiRender.NewCommandBufferSize;
        pDevice->DefaultContext.ContextInfo.pCommandBuffer = DdiRender.pNewCommandBuffer;
        pDevice->DefaultContext.ContextInfo.AllocationListSize = DdiRender.NewAllocationListSize;
        pDevice->DefaultContext.ContextInfo.pAllocationList = DdiRender.pNewAllocationList;
        pDevice->DefaultContext.ContextInfo.PatchLocationListSize = DdiRender.NewPatchLocationListSize;
        pDevice->DefaultContext.ContextInfo.pPatchLocationList = DdiRender.pNewPatchLocationList;

        return VINF_SUCCESS;
    }

    WARN(("pfnRenderCb failed, hr %#x", hr));
    return VERR_GENERAL_FAILURE;
}

static DECLCALLBACK(int) vboxCrHhgsmiDispEscape(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, void *pvData, uint32_t cbData, BOOL fHwAccess)
{
    PVBOXUHGSMI_PRIVATE_D3D pPrivate = VBOXUHGSMID3D_GET(pHgsmi);
    PVBOXWDDMDISP_DEVICE pDevice = pPrivate->pDevice;
    D3DDDICB_ESCAPE DdiEscape = {0};
    DdiEscape.hContext = pDevice->DefaultContext.ContextInfo.hContext;
    DdiEscape.hDevice = pDevice->hDevice;
    DdiEscape.Flags.HardwareAccess = !!fHwAccess;
    DdiEscape.pPrivateDriverData = pvData;
    DdiEscape.PrivateDriverDataSize = cbData;
    HRESULT hr = pDevice->RtCallbacks.pfnEscapeCb(pDevice->pAdapter->hAdapter, &DdiEscape);
    if (SUCCEEDED(hr))
    {
        return VINF_SUCCESS;
    }

    WARN(("pfnEscapeCb failed, hr 0x%x", hr));
    return VERR_GENERAL_FAILURE;
}

void vboxUhgsmiD3DInit(PVBOXUHGSMI_PRIVATE_D3D pHgsmi, PVBOXWDDMDISP_DEVICE pDevice)
{
    pHgsmi->BasePrivate.Base.pfnBufferCreate = vboxUhgsmiD3DBufferCreate;
    pHgsmi->BasePrivate.Base.pfnBufferSubmit = vboxUhgsmiD3DBufferSubmit;
    /* escape is still needed, since Ugfsmi uses it e.g. to query connection id */
    pHgsmi->BasePrivate.pfnEscape = vboxCrHhgsmiDispEscape;
    pHgsmi->pDevice = pDevice;
}

void vboxUhgsmiD3DEscInit(PVBOXUHGSMI_PRIVATE_D3D pHgsmi, struct VBOXWDDMDISP_DEVICE *pDevice)
{
    vboxUhgsmiBaseInit(&pHgsmi->BasePrivate, vboxCrHhgsmiDispEscape);
    pHgsmi->pDevice = pDevice;
}

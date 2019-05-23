/* $Id: VBoxUhgsmiKmt.cpp $ */
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

#include <iprt/mem.h>
#include <iprt/err.h>

#include <cr_protocol.h>

#ifndef NT_SUCCESS
# define NT_SUCCESS(_Status) (((NTSTATUS)(_Status)) >= 0)
#endif


DECLCALLBACK(int) vboxUhgsmiKmtBufferDestroy(PVBOXUHGSMI_BUFFER pBuf)
{
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuffer = VBOXUHGSMDXALLOCBASE_GET_BUFFER(pBuf);
    PVBOXUHGSMI_PRIVATE_KMT pPrivate = VBOXUHGSMIKMT_GET(pBuffer->BasePrivate.pHgsmi);

    D3DKMT_DESTROYALLOCATION DdiDealloc;
    DdiDealloc.hDevice = pPrivate->Device.hDevice;
    DdiDealloc.hResource = NULL;
    DdiDealloc.phAllocationList = &pBuffer->hAllocation;
    DdiDealloc.AllocationCount = 1;
    NTSTATUS Status = pPrivate->Callbacks.pfnD3DKMTDestroyAllocation(&DdiDealloc);
    if (NT_SUCCESS(Status))
    {
#ifdef DEBUG_misha
        memset(pBuffer, 0, sizeof(*pBuffer));
#endif
        RTMemFree(pBuffer);
        return VINF_SUCCESS;
    }
    else
    {
        WARN(("pfnD3DKMTDestroyAllocation failed, Status (0x%x)", Status));
    }
    return VERR_GENERAL_FAILURE;
}

DECLCALLBACK(int) vboxUhgsmiKmtBufferLock(PVBOXUHGSMI_BUFFER pBuf, uint32_t offLock, uint32_t cbLock, VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags, void**pvLock)
{
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuffer = VBOXUHGSMDXALLOCBASE_GET_BUFFER(pBuf);
    PVBOXUHGSMI_PRIVATE_KMT pPrivate = VBOXUHGSMIKMT_GET(pBuffer->BasePrivate.pHgsmi);
    D3DKMT_LOCK DdiLock = {0};
    DdiLock.hDevice = pPrivate->Device.hDevice;
    DdiLock.hAllocation = pBuffer->hAllocation;
    DdiLock.PrivateDriverData = NULL;

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

    NTSTATUS Status = pPrivate->Callbacks.pfnD3DKMTLock(&DdiLock);
    if (NT_SUCCESS(Status))
    {
        *pvLock = (void*)(((uint8_t*)DdiLock.pData) + (offLock & 0xfff));
        return VINF_SUCCESS;
    }
    else
    {
        WARN(("pfnD3DKMTLock failed, Status (0x%x)", Status));
    }

    return VERR_GENERAL_FAILURE;
}

DECLCALLBACK(int) vboxUhgsmiKmtBufferUnlock(PVBOXUHGSMI_BUFFER pBuf)
{
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuffer = VBOXUHGSMDXALLOCBASE_GET_BUFFER(pBuf);
    D3DKMT_UNLOCK DdiUnlock;

    PVBOXUHGSMI_PRIVATE_KMT pPrivate = VBOXUHGSMIKMT_GET(pBuffer->BasePrivate.pHgsmi);
    DdiUnlock.hDevice = pPrivate->Device.hDevice;
    DdiUnlock.NumAllocations = 1;
    DdiUnlock.phAllocations = &pBuffer->hAllocation;
    NTSTATUS Status = pPrivate->Callbacks.pfnD3DKMTUnlock(&DdiUnlock);
    if (NT_SUCCESS(Status))
        return VINF_SUCCESS;
    else
        WARN(("pfnD3DKMTUnlock failed, Status (0x%x)", Status));

    return VERR_GENERAL_FAILURE;
}

DECLCALLBACK(int) vboxUhgsmiKmtBufferCreate(PVBOXUHGSMI pHgsmi, uint32_t cbBuf, VBOXUHGSMI_BUFFER_TYPE_FLAGS fType, PVBOXUHGSMI_BUFFER* ppBuf)
{
    if (!cbBuf)
        return VERR_INVALID_PARAMETER;

    int rc = VINF_SUCCESS;

    cbBuf = VBOXWDDM_ROUNDBOUND(cbBuf, 0x1000);
    Assert(cbBuf);
    uint32_t cPages = cbBuf >> 12;
    Assert(cPages);

    PVBOXUHGSMI_PRIVATE_KMT pPrivate = VBOXUHGSMIKMT_GET(pHgsmi);
    PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE pBuf = (PVBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE)RTMemAllocZ(RT_OFFSETOF(VBOXUHGSMI_BUFFER_PRIVATE_DX_ALLOC_BASE, aLockPageIndices[cPages]));
    if (!pBuf)
    {
        WARN(("RTMemAllocZ failed"));
        return VERR_NO_MEMORY;
    }

    D3DKMT_CREATEALLOCATION DdiAlloc;
    D3DDDI_ALLOCATIONINFO DdiAllocInfo;
    VBOXWDDM_ALLOCINFO AllocInfo;

    memset(&DdiAlloc, 0, sizeof (DdiAlloc));
    DdiAlloc.hDevice = pPrivate->Device.hDevice;
    DdiAlloc.NumAllocations = 1;
    DdiAlloc.pAllocationInfo = &DdiAllocInfo;

    vboxUhgsmiBaseDxAllocInfoFill(&DdiAllocInfo, &AllocInfo, cbBuf, fType);

    NTSTATUS Status = pPrivate->Callbacks.pfnD3DKMTCreateAllocation(&DdiAlloc);
    if (NT_SUCCESS(Status))
    {
        Assert(DdiAllocInfo.hAllocation);
        pBuf->BasePrivate.Base.pfnLock = vboxUhgsmiKmtBufferLock;
        pBuf->BasePrivate.Base.pfnUnlock = vboxUhgsmiKmtBufferUnlock;
        pBuf->BasePrivate.Base.pfnDestroy = vboxUhgsmiKmtBufferDestroy;

        pBuf->BasePrivate.Base.fType = fType;
        pBuf->BasePrivate.Base.cbBuffer = cbBuf;

        pBuf->BasePrivate.pHgsmi = &pPrivate->BasePrivate;

        pBuf->hAllocation = DdiAllocInfo.hAllocation;


        *ppBuf = &pBuf->BasePrivate.Base;

        return VINF_SUCCESS;
    }
    else
    {
        WARN(("pfnD3DKMTCreateAllocation failes, Status(0x%x)", Status));
        rc = VERR_OUT_OF_RESOURCES;
    }

    RTMemFree(pBuf);

    return rc;
}

DECLCALLBACK(int) vboxUhgsmiKmtBufferSubmit(PVBOXUHGSMI pHgsmi, PVBOXUHGSMI_BUFFER_SUBMIT aBuffers, uint32_t cBuffers)
{
    PVBOXUHGSMI_PRIVATE_KMT pHg = VBOXUHGSMIKMT_GET(pHgsmi);
    UINT cbDmaCmd = pHg->Context.CommandBufferSize;
    int rc = vboxUhgsmiBaseDxDmaFill(aBuffers, cBuffers,
            pHg->Context.pCommandBuffer, &cbDmaCmd,
            pHg->Context.pAllocationList, pHg->Context.AllocationListSize,
            pHg->Context.pPatchLocationList, pHg->Context.PatchLocationListSize);
    if (RT_FAILURE(rc))
    {
        WARN(("vboxUhgsmiBaseDxDmaFill failed, rc %d", rc));
        return rc;
    }

    D3DKMT_RENDER DdiRender = {0};
    DdiRender.hContext = pHg->Context.hContext;
    DdiRender.CommandLength = cbDmaCmd;
    DdiRender.AllocationCount = cBuffers;
    Assert(DdiRender.CommandLength);
    Assert(DdiRender.CommandLength < UINT32_MAX/2);

    NTSTATUS Status = pHg->Callbacks.pfnD3DKMTRender(&DdiRender);
    if (NT_SUCCESS(Status))
    {
        pHg->Context.CommandBufferSize = DdiRender.NewCommandBufferSize;
        pHg->Context.pCommandBuffer = DdiRender.pNewCommandBuffer;
        pHg->Context.AllocationListSize = DdiRender.NewAllocationListSize;
        pHg->Context.pAllocationList = DdiRender.pNewAllocationList;
        pHg->Context.PatchLocationListSize = DdiRender.NewPatchLocationListSize;
        pHg->Context.pPatchLocationList = DdiRender.pNewPatchLocationList;

        return VINF_SUCCESS;
    }
    else
    {
        WARN(("pfnD3DKMTRender failed, Status (0x%x)", Status));
    }

    return VERR_GENERAL_FAILURE;
}


static HRESULT vboxUhgsmiKmtEngineCreate(PVBOXUHGSMI_PRIVATE_KMT pHgsmi, BOOL bD3D)
{
    HRESULT hr = vboxDispKmtCallbacksInit(&pHgsmi->Callbacks);
    if (hr == S_OK)
    {
        hr = vboxDispKmtOpenAdapter(&pHgsmi->Callbacks, &pHgsmi->Adapter);
        if (hr == S_OK)
        {
            hr = vboxDispKmtCreateDevice(&pHgsmi->Adapter, &pHgsmi->Device);
            if (hr == S_OK)
            {
                hr = vboxDispKmtCreateContext(&pHgsmi->Device, &pHgsmi->Context,
                        bD3D ? VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_3D : VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_GL,
                                CR_PROTOCOL_VERSION_MAJOR, CR_PROTOCOL_VERSION_MINOR,
                                NULL, 0);
                if (hr == S_OK)
                {
                    return S_OK;
                }
                else
                {
                    WARN(("vboxDispKmtCreateContext failed, hr(0x%x)", hr));
                }
                vboxDispKmtDestroyDevice(&pHgsmi->Device);
            }
            else
            {
                WARN(("vboxDispKmtCreateDevice failed, hr(0x%x)", hr));
            }
            vboxDispKmtCloseAdapter(&pHgsmi->Adapter);
        }
        else
        {
//            WARN(("vboxDispKmtOpenAdapter failed, hr(0x%x)", hr));
        }

        vboxDispKmtCallbacksTerm(&pHgsmi->Callbacks);
    }
    else
    {
        WARN(("vboxDispKmtCallbacksInit failed, hr(0x%x)", hr));
    }
    return hr;
}

static DECLCALLBACK(int) vboxCrHhgsmiKmtEscape(struct VBOXUHGSMI_PRIVATE_BASE *pHgsmi, void *pvData, uint32_t cbData, BOOL fHwAccess)
{
    PVBOXUHGSMI_PRIVATE_KMT pPrivate = VBOXUHGSMIKMT_GET(pHgsmi);
    D3DKMT_ESCAPE DdiEscape = {0};
    DdiEscape.hAdapter = pPrivate->Adapter.hAdapter;
    DdiEscape.hDevice = pPrivate->Device.hDevice;
    DdiEscape.Type = D3DKMT_ESCAPE_DRIVERPRIVATE;
    DdiEscape.Flags.HardwareAccess = !!fHwAccess;
    DdiEscape.pPrivateDriverData = pvData;
    DdiEscape.PrivateDriverDataSize = cbData;
    DdiEscape.hContext = pPrivate->Context.hContext;

    NTSTATUS Status = pPrivate->Callbacks.pfnD3DKMTEscape(&DdiEscape);
    if (NT_SUCCESS(Status))
    {
        return VINF_SUCCESS;
    }

    WARN(("pfnD3DKMTEscape failed, Status (0x%x)", Status));
    return VERR_GENERAL_FAILURE;
}

static void vboxUhgsmiKmtSetupCallbacks(PVBOXUHGSMI_PRIVATE_KMT pHgsmi)
{
    pHgsmi->BasePrivate.Base.pfnBufferCreate = vboxUhgsmiKmtBufferCreate;
    pHgsmi->BasePrivate.Base.pfnBufferSubmit = vboxUhgsmiKmtBufferSubmit;
    /* escape is still needed, since Ugfsmi uses it e.g. to query connection id */
    pHgsmi->BasePrivate.pfnEscape = vboxCrHhgsmiKmtEscape;
}

static void vboxUhgsmiKmtEscSetupCallbacks(PVBOXUHGSMI_PRIVATE_KMT pHgsmi)
{
    vboxUhgsmiBaseInit(&pHgsmi->BasePrivate, vboxCrHhgsmiKmtEscape);
}

#if 0
HRESULT vboxUhgsmiKmtCreate(PVBOXUHGSMI_PRIVATE_KMT pHgsmi, BOOL bD3D)
{
    vboxUhgsmiKmtSetupCallbacks(pHgsmi);
    return vboxUhgsmiKmtEngineCreate(pHgsmi, bD3D);
}

HRESULT vboxUhgsmiKmtEscCreate(PVBOXUHGSMI_PRIVATE_KMT pHgsmi, BOOL bD3D)
{
    vboxUhgsmiKmtEscSetupCallbacks(pHgsmi);
    return vboxUhgsmiKmtEngineCreate(pHgsmi, bD3D);
}
#endif

static HRESULT vboxUhgsmiKmtQueryCaps(PVBOXUHGSMI_PRIVATE_KMT pHgsmi, uint32_t *pu32Caps)
{
    VBOXWDDM_QI Query;
    D3DKMT_QUERYADAPTERINFO Info;
    Info.hAdapter = pHgsmi->Adapter.hAdapter;
    Info.Type = KMTQAITYPE_UMDRIVERPRIVATE;
    Info.pPrivateDriverData = &Query;
    Info.PrivateDriverDataSize = sizeof (Query);

    NTSTATUS Status = pHgsmi->Callbacks.pfnD3DKMTQueryAdapterInfo(&Info);
    if (!NT_SUCCESS(Status))
    {
        WARN(("pfnD3DKMTQueryAdapterInfo failed, Status %#x", Status));
        return Status;
    }

    if (Query.u32Version != VBOXVIDEOIF_VERSION)
    {
        WARN(("Version mismatch"));
        return E_FAIL;
    }

    *pu32Caps = Query.u32VBox3DCaps;

    return S_OK;
}

HRESULT vboxUhgsmiKmtCreate(PVBOXUHGSMI_PRIVATE_KMT pHgsmi, BOOL bD3D)
{
    HRESULT hr = vboxUhgsmiKmtEngineCreate(pHgsmi, bD3D);
    if (!SUCCEEDED(hr))
        return hr;

    uint32_t u32Caps = 0;
    hr = vboxUhgsmiKmtQueryCaps(pHgsmi, &u32Caps);
    if (!SUCCEEDED(hr))
    {
        WARN(("vboxUhgsmiKmtQueryCaps failed hr %#x", hr));
        return hr;
    }

    if (u32Caps & CR_VBOX_CAP_CMDVBVA)
        vboxUhgsmiKmtSetupCallbacks(pHgsmi);
    else
        vboxUhgsmiKmtEscSetupCallbacks(pHgsmi);

    return S_OK;
}

HRESULT vboxUhgsmiKmtDestroy(PVBOXUHGSMI_PRIVATE_KMT pHgsmi)
{
    HRESULT hr = vboxDispKmtDestroyContext(&pHgsmi->Context);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        hr = vboxDispKmtDestroyDevice(&pHgsmi->Device);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            hr = vboxDispKmtCloseAdapter(&pHgsmi->Adapter);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                hr = vboxDispKmtCallbacksTerm(&pHgsmi->Callbacks);
                Assert(hr == S_OK);
                if (hr == S_OK)
                {
#ifdef DEBUG_misha
                    memset(pHgsmi, 0, sizeof (*pHgsmi));
#endif
                    return S_OK;
                }
            }
        }
    }
    return hr;
}

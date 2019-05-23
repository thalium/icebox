/* $Id: VBoxMPWddm.cpp $ */
/** @file
 * VBox WDDM Miniport driver
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

#include "VBoxMPWddm.h"
#include "common/VBoxMPCommon.h"
#include "common/VBoxMPHGSMI.h"
#include "VBoxMPVhwa.h"
#include "VBoxMPVidPn.h"

#include <iprt/asm.h>
#include <iprt/param.h>

#include <VBox/VBoxGuestLib.h>
#include <VBox/VMMDev.h> /* for VMMDevVideoSetVisibleRegion */
#include <VBoxVideo.h>
#include <wingdi.h> /* needed for RGNDATA definition */
#include <VBoxDisplay.h> /* this is from Additions/WINNT/include/ to include escape codes */
#include <VBoxVideoVBE.h>
#include <VBox/Version.h>

#include <stdio.h>

/* Uncomment this in order to enable dumping regions guest wants to display on DxgkDdiPresentNew(). */
//#define VBOX_WDDM_DUMP_REGIONS_ON_PRESENT

#define VBOXWDDM_DUMMY_DMABUFFER_SIZE (sizeof(VBOXCMDVBVA_HDR) / 2)

DWORD g_VBoxLogUm = 0;
#ifdef VBOX_WDDM_WIN8
DWORD g_VBoxDisplayOnly = 0;
#endif

#define VBOXWDDM_MEMTAG 'MDBV'
PVOID vboxWddmMemAlloc(IN SIZE_T cbSize)
{
#ifdef VBOX_WDDM_WIN8
    POOL_TYPE enmPoolType = NonPagedPoolNx;
#else
    POOL_TYPE enmPoolType = NonPagedPool;
#endif
    return ExAllocatePoolWithTag(enmPoolType, cbSize, VBOXWDDM_MEMTAG);
}

PVOID vboxWddmMemAllocZero(IN SIZE_T cbSize)
{
    PVOID pvMem = vboxWddmMemAlloc(cbSize);
    if (pvMem)
        memset(pvMem, 0, cbSize);
    return pvMem;
}


VOID vboxWddmMemFree(PVOID pvMem)
{
    ExFreePool(pvMem);
}

DECLINLINE(void) VBoxWddmOaHostIDReleaseLocked(PVBOXWDDM_OPENALLOCATION pOa)
{
    Assert(pOa->cHostIDRefs);
    PVBOXWDDM_ALLOCATION pAllocation = pOa->pAllocation;
    Assert(pAllocation->AllocData.cHostIDRefs >= pOa->cHostIDRefs);
    Assert(pAllocation->AllocData.hostID);
    --pOa->cHostIDRefs;
    --pAllocation->AllocData.cHostIDRefs;
    if (!pAllocation->AllocData.cHostIDRefs)
        pAllocation->AllocData.hostID = 0;
}

DECLINLINE(void) VBoxWddmOaHostIDCheckReleaseLocked(PVBOXWDDM_OPENALLOCATION pOa)
{
    if (pOa->cHostIDRefs)
        VBoxWddmOaHostIDReleaseLocked(pOa);
}

DECLINLINE(void) VBoxWddmOaRelease(PVBOXWDDM_OPENALLOCATION pOa)
{
    PVBOXWDDM_ALLOCATION pAllocation = pOa->pAllocation;
    KIRQL OldIrql;
    KeAcquireSpinLock(&pAllocation->OpenLock, &OldIrql);
    Assert(pAllocation->cOpens);
    VBoxWddmOaHostIDCheckReleaseLocked(pOa);
    --pAllocation->cOpens;
    uint32_t cOpens = --pOa->cOpens;
    Assert(cOpens < UINT32_MAX/2);
    if (!cOpens)
    {
        RemoveEntryList(&pOa->ListEntry);
        KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);
        vboxWddmMemFree(pOa);
    }
    else
        KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);
}

DECLINLINE(PVBOXWDDM_OPENALLOCATION) VBoxWddmOaSearchLocked(PVBOXWDDM_DEVICE pDevice, PVBOXWDDM_ALLOCATION pAllocation)
{
    for (PLIST_ENTRY pCur = pAllocation->OpenList.Flink; pCur != &pAllocation->OpenList; pCur = pCur->Flink)
    {
        PVBOXWDDM_OPENALLOCATION pCurOa = CONTAINING_RECORD(pCur, VBOXWDDM_OPENALLOCATION, ListEntry);
        if (pCurOa->pDevice == pDevice)
        {
            return pCurOa;
        }
    }
    return NULL;
}

DECLINLINE(PVBOXWDDM_OPENALLOCATION) VBoxWddmOaSearch(PVBOXWDDM_DEVICE pDevice, PVBOXWDDM_ALLOCATION pAllocation)
{
    PVBOXWDDM_OPENALLOCATION pOa;
    KIRQL OldIrql;
    KeAcquireSpinLock(&pAllocation->OpenLock, &OldIrql);
    pOa = VBoxWddmOaSearchLocked(pDevice, pAllocation);
    KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);
    return pOa;
}

DECLINLINE(int) VBoxWddmOaSetHostID(PVBOXWDDM_DEVICE pDevice, PVBOXWDDM_ALLOCATION pAllocation, uint32_t hostID, uint32_t *pHostID)
{
    PVBOXWDDM_OPENALLOCATION pOa;
    KIRQL OldIrql;
    int rc = VINF_SUCCESS;
    KeAcquireSpinLock(&pAllocation->OpenLock, &OldIrql);
    pOa = VBoxWddmOaSearchLocked(pDevice, pAllocation);
    if (!pOa)
    {
        KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);;
        WARN(("no open allocation!"));
        return VERR_INVALID_STATE;
    }

    if (hostID)
    {
        if (pAllocation->AllocData.hostID == 0)
        {
            pAllocation->AllocData.hostID = hostID;
        }
        else if (pAllocation->AllocData.hostID != hostID)
        {
            WARN(("hostID differ: alloc(%d), trying to assign(%d)", pAllocation->AllocData.hostID, hostID));
            hostID = pAllocation->AllocData.hostID;
            rc = VERR_NOT_EQUAL;
        }

        ++pAllocation->AllocData.cHostIDRefs;
        ++pOa->cHostIDRefs;
    }
    else
        VBoxWddmOaHostIDCheckReleaseLocked(pOa);

    KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);

    if (pHostID)
        *pHostID = hostID;

    return rc;
}

DECLINLINE(PVBOXWDDM_ALLOCATION) vboxWddmGetAllocationFromHandle(PVBOXMP_DEVEXT pDevExt, D3DKMT_HANDLE hAllocation)
{
    DXGKARGCB_GETHANDLEDATA GhData;
    GhData.hObject = hAllocation;
    GhData.Type = DXGK_HANDLE_ALLOCATION;
    GhData.Flags.Value = 0;
    return (PVBOXWDDM_ALLOCATION)pDevExt->u.primary.DxgkInterface.DxgkCbGetHandleData(&GhData);
}

DECLINLINE(PVBOXWDDM_ALLOCATION) vboxWddmGetAllocationFromAllocList(DXGK_ALLOCATIONLIST *pAllocList)
{
    PVBOXWDDM_OPENALLOCATION pOa = (PVBOXWDDM_OPENALLOCATION)pAllocList->hDeviceSpecificAllocation;
    return pOa->pAllocation;
}

static void vboxWddmPopulateDmaAllocInfo(PVBOXWDDM_DMA_ALLOCINFO pInfo, PVBOXWDDM_ALLOCATION pAlloc, DXGK_ALLOCATIONLIST *pDmaAlloc)
{
    pInfo->pAlloc = pAlloc;
    if (pDmaAlloc->SegmentId)
    {
        pInfo->offAlloc = (VBOXVIDEOOFFSET)pDmaAlloc->PhysicalAddress.QuadPart;
        pInfo->segmentIdAlloc = pDmaAlloc->SegmentId;
    }
    else
        pInfo->segmentIdAlloc = 0;
    pInfo->srcId = pAlloc->AllocData.SurfDesc.VidPnSourceId;
}

static void vboxWddmPopulateDmaAllocInfoWithOffset(PVBOXWDDM_DMA_ALLOCINFO pInfo, PVBOXWDDM_ALLOCATION pAlloc, DXGK_ALLOCATIONLIST *pDmaAlloc, uint32_t offStart)
{
    pInfo->pAlloc = pAlloc;
    if (pDmaAlloc->SegmentId)
    {
        pInfo->offAlloc = (VBOXVIDEOOFFSET)pDmaAlloc->PhysicalAddress.QuadPart + offStart;
        pInfo->segmentIdAlloc = pDmaAlloc->SegmentId;
    }
    else
        pInfo->segmentIdAlloc = 0;
    pInfo->srcId = pAlloc->AllocData.SurfDesc.VidPnSourceId;
}

int vboxWddmGhDisplayPostInfoScreen(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData, const POINT * pVScreenPos, uint16_t fFlags)
{
    VBVAINFOSCREEN RT_UNTRUSTED_VOLATILE_HOST *pScreen =
        (VBVAINFOSCREEN RT_UNTRUSTED_VOLATILE_HOST *)VBoxHGSMIBufferAlloc(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx,
                                                                          sizeof(VBVAINFOSCREEN),
                                                                          HGSMI_CH_VBVA,
                                                                          VBVA_INFO_SCREEN);
    if (!pScreen != NULL)
    {
        WARN(("VBoxHGSMIBufferAlloc failed"));
        return VERR_OUT_OF_RESOURCES;
    }

    int rc = vboxWddmScreenInfoInit(pScreen, pAllocData, pVScreenPos, fFlags);
    if (RT_SUCCESS(rc))
    {
        pScreen->u32StartOffset = 0; //(uint32_t)offVram; /* we pretend the view is located at the start of each framebuffer */

        rc = VBoxHGSMIBufferSubmit(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx, pScreen);
        if (RT_FAILURE(rc))
            WARN(("VBoxHGSMIBufferSubmit failed %d", rc));
    }
    else
        WARN(("VBoxHGSMIBufferSubmit failed %d", rc));

    VBoxHGSMIBufferFree(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx, pScreen);

    return rc;
}

int vboxWddmGhDisplayPostInfoView(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData)
{
    VBOXVIDEOOFFSET offVram = vboxWddmAddrFramOffset(&pAllocData->Addr);
    if (offVram == VBOXVIDEOOFFSET_VOID)
    {
        WARN(("offVram == VBOXVIDEOOFFSET_VOID"));
        return VERR_INVALID_PARAMETER;
    }

    /* Issue the screen info command. */
    VBVAINFOVIEW RT_UNTRUSTED_VOLATILE_HOST *pView =
        (VBVAINFOVIEW RT_UNTRUSTED_VOLATILE_HOST *)VBoxHGSMIBufferAlloc(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx,
                                                                        sizeof(VBVAINFOVIEW), HGSMI_CH_VBVA, VBVA_INFO_VIEW);
    if (!pView)
    {
        WARN(("VBoxHGSMIBufferAlloc failed"));
        return VERR_OUT_OF_RESOURCES;
    }
    pView->u32ViewIndex     = pAllocData->SurfDesc.VidPnSourceId;
    pView->u32ViewOffset    = (uint32_t)offVram; /* we pretend the view is located at the start of each framebuffer */
    pView->u32ViewSize      = vboxWddmVramCpuVisibleSegmentSize(pDevExt)/VBoxCommonFromDeviceExt(pDevExt)->cDisplays;
    pView->u32MaxScreenSize = pView->u32ViewSize;

    int rc = VBoxHGSMIBufferSubmit (&VBoxCommonFromDeviceExt(pDevExt)->guestCtx, pView);
    if (RT_FAILURE(rc))
        WARN(("VBoxHGSMIBufferSubmit failed %d", rc));

    VBoxHGSMIBufferFree(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx, pView);
    return rc;
}

NTSTATUS vboxWddmGhDisplayPostResizeLegacy(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData, const POINT * pVScreenPos, uint16_t fFlags)
{
    int rc;

    if (!(fFlags & (VBVA_SCREEN_F_DISABLED | VBVA_SCREEN_F_BLANK2)))
    {
        rc = vboxWddmGhDisplayPostInfoView(pDevExt, pAllocData);
        if (RT_FAILURE(rc))
        {
            WARN(("vboxWddmGhDisplayPostInfoView failed %d", rc));
            return STATUS_UNSUCCESSFUL;
        }
    }

    rc = vboxWddmGhDisplayPostInfoScreen(pDevExt, pAllocData, pVScreenPos, fFlags);
    if (RT_FAILURE(rc))
    {
        WARN(("vboxWddmGhDisplayPostInfoScreen failed %d", rc));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS vboxWddmGhDisplayPostResizeNew(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData, const uint32_t *pTargetMap, const POINT * pVScreenPos, uint16_t fFlags)
{
    int rc = VBoxCmdVbvaConCmdResize(pDevExt, pAllocData, pTargetMap, pVScreenPos, fFlags);
    if (RT_SUCCESS(rc))
        return STATUS_SUCCESS;

    WARN(("VBoxCmdVbvaConCmdResize failed %d", rc));
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS vboxWddmGhDisplaySetMode(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData)
{
    RT_NOREF(pDevExt);
    VBOXVIDEOOFFSET offVram = vboxWddmAddrFramOffset(&pAllocData->Addr);;
    if (offVram == VBOXVIDEOOFFSET_VOID)
    {
        WARN(("offVram == VBOXVIDEOOFFSET_VOID"));
        return STATUS_UNSUCCESSFUL;
    }

    USHORT width  = pAllocData->SurfDesc.width;
    USHORT height = pAllocData->SurfDesc.height;
    USHORT bpp    = pAllocData->SurfDesc.bpp;
    ULONG cbLine  = VBOXWDDM_ROUNDBOUND(((width * bpp) + 7) / 8, 4);
    ULONG yOffset = (ULONG)offVram / cbLine;
    ULONG xOffset = (ULONG)offVram % cbLine;

    if (bpp == 4)
    {
        xOffset <<= 1;
    }
    else
    {
        Assert(!(xOffset%((bpp + 7) >> 3)));
        xOffset /= ((bpp + 7) >> 3);
    }
    Assert(xOffset <= 0xffff);
    Assert(yOffset <= 0xffff);

    VBoxVideoSetModeRegisters(width, height, width, bpp, 0, (uint16_t)xOffset, (uint16_t)yOffset);
    /** @todo read back from port to check if mode switch was successful */

    return STATUS_SUCCESS;
}

static uint16_t vboxWddmCalcScreenFlags(PVBOXMP_DEVEXT pDevExt, bool fValidAlloc, bool fPowerOff, bool fDisabled)
{
    uint16_t u16Flags;

    if (fValidAlloc)
    {
        u16Flags = VBVA_SCREEN_F_ACTIVE;
    }
    else
    {
        if (   !fDisabled
            && fPowerOff
            && RT_BOOL(VBoxCommonFromDeviceExt(pDevExt)->u16SupportedScreenFlags & VBVA_SCREEN_F_BLANK2))
        {
            u16Flags = VBVA_SCREEN_F_ACTIVE | VBVA_SCREEN_F_BLANK2;
        }
        else
        {
            u16Flags = VBVA_SCREEN_F_DISABLED;
        }
    }

    return u16Flags;
}

NTSTATUS vboxWddmGhDisplaySetInfoLegacy(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData, const POINT * pVScreenPos, uint8_t u8CurCyncState, bool fPowerOff, bool fDisabled)
{
    NTSTATUS Status = STATUS_SUCCESS;
    bool fValidAlloc = pAllocData->SurfDesc.width > 0 && pAllocData->SurfDesc.height > 0;
    uint16_t fu16Flags = vboxWddmCalcScreenFlags(pDevExt, fValidAlloc, fPowerOff, fDisabled);

    if (fValidAlloc)
    {
#ifdef VBOX_WITH_CROGL
        if ((u8CurCyncState & VBOXWDDM_HGSYNC_F_CHANGED_LOCATION_ONLY) == VBOXWDDM_HGSYNC_F_CHANGED_LOCATION_ONLY
                && pAllocData->hostID)
        {
            Status = vboxVdmaTexPresentSetAlloc(pDevExt, pAllocData);
            if (!NT_SUCCESS(Status))
                WARN(("vboxVdmaTexPresentSetAlloc failed, Status 0x%x", Status));
            return Status;
        }
#endif

        if (pAllocData->SurfDesc.VidPnSourceId == 0)
            Status = vboxWddmGhDisplaySetMode(pDevExt, pAllocData);
    }

    if (NT_SUCCESS(Status))
    {
        Status = vboxWddmGhDisplayPostResizeLegacy(pDevExt, pAllocData, pVScreenPos,
                fu16Flags);
        if (NT_SUCCESS(Status))
        {
#ifdef VBOX_WITH_CROGL
            if (fValidAlloc && pDevExt->f3DEnabled)
            {
                Status = vboxVdmaTexPresentSetAlloc(pDevExt, pAllocData);
                if (NT_SUCCESS(Status))
                    return STATUS_SUCCESS;
                else
                    WARN(("vboxVdmaTexPresentSetAlloc failed, Status 0x%x", Status));
            }
#else
            return STATUS_SUCCESS;
#endif
        }
        else
            WARN(("vboxWddmGhDisplayPostResize failed, Status 0x%x", Status));
    }
    else
        WARN(("vboxWddmGhDisplaySetMode failed, Status 0x%x", Status));

    return Status;
}

NTSTATUS vboxWddmGhDisplaySetInfoNew(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData, const uint32_t *pTargetMap, const POINT * pVScreenPos, uint8_t u8CurCyncState, bool fPowerOff, bool fDisabled)
{
    NTSTATUS Status = STATUS_SUCCESS;
    bool fValidAlloc = pAllocData->SurfDesc.width > 0 && pAllocData->SurfDesc.height > 0;
    uint16_t fu16Flags = vboxWddmCalcScreenFlags(pDevExt, fValidAlloc, fPowerOff, fDisabled);

    if (fValidAlloc)
    {
#ifdef VBOX_WITH_CROGL
        if ((u8CurCyncState & VBOXWDDM_HGSYNC_F_CHANGED_LOCATION_ONLY) == VBOXWDDM_HGSYNC_F_CHANGED_LOCATION_ONLY
                && pAllocData->hostID)
        {
            Status = vboxVdmaTexPresentSetAlloc(pDevExt, pAllocData);
            if (!NT_SUCCESS(Status))
                WARN(("vboxVdmaTexPresentSetAlloc failed, Status 0x%x", Status));
            return Status;
        }
#endif

        if (ASMBitTest(pTargetMap, 0))
            Status = vboxWddmGhDisplaySetMode(pDevExt, pAllocData);
    }

    if (NT_SUCCESS(Status))
    {
        Status = vboxWddmGhDisplayPostResizeNew(pDevExt, pAllocData, pTargetMap, pVScreenPos, fu16Flags);
        if (NT_SUCCESS(Status))
        {
#ifdef VBOX_WITH_CROGL
            if (fValidAlloc && pDevExt->f3DEnabled)
            {
                Status = vboxVdmaTexPresentSetAlloc(pDevExt, pAllocData);
                if (NT_SUCCESS(Status))
                    return STATUS_SUCCESS;
                else
                    WARN(("vboxVdmaTexPresentSetAlloc failed, Status 0x%x", Status));
            }
#else
            return STATUS_SUCCESS;
#endif
        }
        else
            WARN(("vboxWddmGhDisplayPostResizeNew failed, Status 0x%x", Status));
    }
    else
        WARN(("vboxWddmGhDisplaySetMode failed, Status 0x%x", Status));

    return Status;
}

bool vboxWddmGhDisplayCheckSetInfoFromSourceNew(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_SOURCE pSource, bool fReportTargets)
{
    if (pSource->u8SyncState == VBOXWDDM_HGSYNC_F_SYNCED_ALL)
    {
        if (!pSource->fTargetsReported && fReportTargets)
            pSource->u8SyncState &= ~VBOXWDDM_HGSYNC_F_SYNCED_TOPOLOGY;
        else
            return false;
    }

    if (!pSource->AllocData.Addr.SegmentId && pSource->AllocData.SurfDesc.width)
        return false;

    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);
    uint32_t *pTargetMap;
    if (fReportTargets)
        pTargetMap = pSource->aTargetMap;
    else
    {
        memset(aTargetMap, 0, sizeof (aTargetMap));
        pTargetMap = aTargetMap;
    }

    NTSTATUS Status = vboxWddmGhDisplaySetInfoNew(pDevExt, &pSource->AllocData, pTargetMap, &pSource->VScreenPos, pSource->u8SyncState, RT_BOOL(pSource->bBlankedByPowerOff), false);
    if (NT_SUCCESS(Status))
    {
        if (fReportTargets && (pSource->u8SyncState & VBOXWDDM_HGSYNC_F_CHANGED_LOCATION_ONLY) != VBOXWDDM_HGSYNC_F_CHANGED_LOCATION_ONLY)
        {
            VBOXWDDM_TARGET_ITER Iter;
            VBoxVidPnStTIterInit(pSource, pDevExt->aTargets, VBoxCommonFromDeviceExt(pDevExt)->cDisplays, &Iter);

            for (PVBOXWDDM_TARGET pTarget = VBoxVidPnStTIterNext(&Iter);
                    pTarget;
                    pTarget = VBoxVidPnStTIterNext(&Iter))
            {
                pTarget->u8SyncState = VBOXWDDM_HGSYNC_F_SYNCED_ALL;
            }
        }

        pSource->u8SyncState = VBOXWDDM_HGSYNC_F_SYNCED_ALL;
        pSource->fTargetsReported = !!fReportTargets;
        return true;
    }

    WARN(("vboxWddmGhDisplaySetInfoNew failed, Status (0x%x)", Status));
    return false;
}

bool vboxWddmGhDisplayCheckSetInfoFromSourceLegacy(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_SOURCE pSource, bool fReportTargets)
{
    if (!fReportTargets)
        return false;

    if (pSource->u8SyncState == VBOXWDDM_HGSYNC_F_SYNCED_ALL)
        return false;

    if (!pSource->AllocData.Addr.SegmentId)
        return false;

    VBOXWDDM_TARGET_ITER Iter;
    VBoxVidPnStTIterInit(pSource, pDevExt->aTargets, VBoxCommonFromDeviceExt(pDevExt)->cDisplays, &Iter);
    uint8_t u8SyncState = VBOXWDDM_HGSYNC_F_SYNCED_ALL;
    VBOXWDDM_ALLOC_DATA AllocData = pSource->AllocData;

    for (PVBOXWDDM_TARGET pTarget = VBoxVidPnStTIterNext(&Iter);
            pTarget;
            pTarget = VBoxVidPnStTIterNext(&Iter))
    {
        AllocData.SurfDesc.VidPnSourceId = pTarget->u32Id;
        NTSTATUS Status = vboxWddmGhDisplaySetInfoLegacy(pDevExt, &AllocData, &pSource->VScreenPos, pSource->u8SyncState | pTarget->u8SyncState, pTarget->fBlankedByPowerOff, pTarget->fDisabled);
        if (NT_SUCCESS(Status))
            pTarget->u8SyncState = VBOXWDDM_HGSYNC_F_SYNCED_ALL;
        else
        {
            WARN(("vboxWddmGhDisplaySetInfoLegacy failed, Status (0x%x)", Status));
            u8SyncState = 0;
        }
    }

    pSource->u8SyncState |= u8SyncState;

    return true;
}

bool vboxWddmGhDisplayCheckSetInfoFromSourceEx(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_SOURCE pSource, bool fReportTargets)
{
    if (pDevExt->fCmdVbvaEnabled)
        return vboxWddmGhDisplayCheckSetInfoFromSourceNew(pDevExt, pSource, fReportTargets);
    return vboxWddmGhDisplayCheckSetInfoFromSourceLegacy(pDevExt, pSource, fReportTargets);
}

bool vboxWddmGhDisplayCheckSetInfoFromSource(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_SOURCE pSource)
{
    bool fReportTargets = !pDevExt->fDisableTargetUpdate;
    return vboxWddmGhDisplayCheckSetInfoFromSourceEx(pDevExt, pSource, fReportTargets);
}

bool vboxWddmGhDisplayCheckSetInfoForDisabledTargetsNew(PVBOXMP_DEVEXT pDevExt)
{
    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);

    memset(aTargetMap, 0, sizeof (aTargetMap));

    bool fFound = false;
    for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
    {
        VBOXWDDM_TARGET *pTarget = &pDevExt->aTargets[i];
        Assert(pTarget->u32Id == i);
        if (pTarget->VidPnSourceId != D3DDDI_ID_UNINITIALIZED)
        {
            Assert(pTarget->VidPnSourceId < (D3DDDI_VIDEO_PRESENT_SOURCE_ID)VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
            continue;
        }

        /* Explicitely disabled targets must not be skipped. */
        if (pTarget->fBlankedByPowerOff && !pTarget->fDisabled)
        {
            LOG(("Skip doing DISABLED request for PowerOff tgt %d", pTarget->u32Id));
            continue;
        }

        if (pTarget->u8SyncState != VBOXWDDM_HGSYNC_F_SYNCED_ALL)
        {
            ASMBitSet(aTargetMap, i);
            fFound = true;
        }
    }

    if (!fFound)
        return false;

    POINT VScreenPos = {0};
    VBOXWDDM_ALLOC_DATA AllocData;
    VBoxVidPnAllocDataInit(&AllocData, D3DDDI_ID_UNINITIALIZED);
    NTSTATUS Status = vboxWddmGhDisplaySetInfoNew(pDevExt, &AllocData, aTargetMap, &VScreenPos, 0, false, true);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxWddmGhDisplaySetInfoNew failed %#x", Status));
        return false;
    }

    for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
    {
        VBOXWDDM_TARGET *pTarget = &pDevExt->aTargets[i];
        if (pTarget->VidPnSourceId != D3DDDI_ID_UNINITIALIZED)
        {
            Assert(pTarget->VidPnSourceId < (D3DDDI_VIDEO_PRESENT_SOURCE_ID)VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
            continue;
        }

        pTarget->u8SyncState = VBOXWDDM_HGSYNC_F_SYNCED_ALL;
    }

    return true;
}

bool vboxWddmGhDisplayCheckSetInfoForDisabledTargetsLegacy(PVBOXMP_DEVEXT pDevExt)
{
    POINT VScreenPos = {0};
    bool fFound = false;
    VBOXWDDM_ALLOC_DATA AllocData;
    VBoxVidPnAllocDataInit(&AllocData, D3DDDI_ID_UNINITIALIZED);

    for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
    {
        VBOXWDDM_TARGET *pTarget = &pDevExt->aTargets[i];
        Assert(pTarget->u32Id == i);
        if (pTarget->VidPnSourceId != D3DDDI_ID_UNINITIALIZED)
        {
            Assert(pTarget->VidPnSourceId < (D3DDDI_VIDEO_PRESENT_SOURCE_ID)VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
            continue;
        }

        if (pTarget->u8SyncState == VBOXWDDM_HGSYNC_F_SYNCED_ALL)
            continue;

        fFound = true;
        AllocData.SurfDesc.VidPnSourceId = i;
        NTSTATUS Status = vboxWddmGhDisplaySetInfoLegacy(pDevExt, &AllocData, &VScreenPos, 0, pTarget->fBlankedByPowerOff, pTarget->fDisabled);
        if (NT_SUCCESS(Status))
            pTarget->u8SyncState = VBOXWDDM_HGSYNC_F_SYNCED_ALL;
        else
            WARN(("vboxWddmGhDisplaySetInfoLegacy failed, Status (0x%x)", Status));
    }

    return fFound;
}

void vboxWddmGhDisplayCheckSetInfoForDisabledTargets(PVBOXMP_DEVEXT pDevExt)
{
    if (pDevExt->fCmdVbvaEnabled)
        vboxWddmGhDisplayCheckSetInfoForDisabledTargetsNew(pDevExt);
    else
        vboxWddmGhDisplayCheckSetInfoForDisabledTargetsLegacy(pDevExt);
}

void vboxWddmGhDisplayCheckSetInfoForDisabledTargetsCheck(PVBOXMP_DEVEXT pDevExt)
{
    bool fReportTargets = !pDevExt->fDisableTargetUpdate;

    if (fReportTargets)
        vboxWddmGhDisplayCheckSetInfoForDisabledTargets(pDevExt);
}

void vboxWddmGhDisplayCheckSetInfoEx(PVBOXMP_DEVEXT pDevExt, bool fReportTargets)
{
    for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
    {
        VBOXWDDM_SOURCE *pSource = &pDevExt->aSources[i];
        vboxWddmGhDisplayCheckSetInfoFromSourceEx(pDevExt, pSource, fReportTargets);
    }

    if (fReportTargets)
        vboxWddmGhDisplayCheckSetInfoForDisabledTargets(pDevExt);
}

void vboxWddmGhDisplayCheckSetInfo(PVBOXMP_DEVEXT pDevExt)
{
    bool fReportTargets = !pDevExt->fDisableTargetUpdate;
    vboxWddmGhDisplayCheckSetInfoEx(pDevExt, fReportTargets);
}

PVBOXSHGSMI vboxWddmHgsmiGetHeapFromCmdOffset(PVBOXMP_DEVEXT pDevExt, HGSMIOFFSET offCmd)
{
#ifdef VBOX_WITH_VDMA
    if(HGSMIAreaContainsOffset(&pDevExt->u.primary.Vdma.CmdHeap.Heap.area, offCmd))
        return &pDevExt->u.primary.Vdma.CmdHeap;
#endif
    if (HGSMIAreaContainsOffset(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx.Heap.area, offCmd))
        return &VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx;
    return NULL;
}

typedef enum
{
    VBOXWDDM_HGSMICMD_TYPE_UNDEFINED = 0,
    VBOXWDDM_HGSMICMD_TYPE_CTL       = 1,
#ifdef VBOX_WITH_VDMA
    VBOXWDDM_HGSMICMD_TYPE_DMACMD    = 2
#endif
} VBOXWDDM_HGSMICMD_TYPE;

VBOXWDDM_HGSMICMD_TYPE vboxWddmHgsmiGetCmdTypeFromOffset(PVBOXMP_DEVEXT pDevExt, HGSMIOFFSET offCmd)
{
#ifdef VBOX_WITH_VDMA
    if(HGSMIAreaContainsOffset(&pDevExt->u.primary.Vdma.CmdHeap.Heap.area, offCmd))
        return VBOXWDDM_HGSMICMD_TYPE_DMACMD;
#endif
    if (HGSMIAreaContainsOffset(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx.Heap.area, offCmd))
        return VBOXWDDM_HGSMICMD_TYPE_CTL;
    return VBOXWDDM_HGSMICMD_TYPE_UNDEFINED;
}

typedef struct VBOXWDDM_HWRESOURCES
{
    PHYSICAL_ADDRESS phVRAM;
    ULONG cbVRAM;
    ULONG ulApertureSize;
} VBOXWDDM_HWRESOURCES, *PVBOXWDDM_HWRESOURCES;

NTSTATUS vboxWddmPickResources(PVBOXMP_DEVEXT pDevExt, PDXGK_DEVICE_INFO pDeviceInfo, PVBOXWDDM_HWRESOURCES pHwResources)
{
    RT_NOREF(pDevExt);
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT DispiId;
    memset(pHwResources, 0, sizeof (*pHwResources));
    pHwResources->cbVRAM = VBE_DISPI_TOTAL_VIDEO_MEMORY_BYTES;

    VBVO_PORT_WRITE_U16(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ID);
    VBVO_PORT_WRITE_U16(VBE_DISPI_IOPORT_DATA, VBE_DISPI_ID2);
    DispiId = VBVO_PORT_READ_U16(VBE_DISPI_IOPORT_DATA);
    if (DispiId == VBE_DISPI_ID2)
    {
       LOGREL(("found the VBE card"));
       /*
        * Write some hardware information to registry, so that
        * it's visible in Windows property dialog.
        */

       /*
        * Query the adapter's memory size. It's a bit of a hack, we just read
        * an ULONG from the data port without setting an index before.
        */
       pHwResources->cbVRAM = VBVO_PORT_READ_U32(VBE_DISPI_IOPORT_DATA);
       if (VBoxHGSMIIsSupported ())
       {
           PCM_RESOURCE_LIST pRcList = pDeviceInfo->TranslatedResourceList;
           /** @todo verify resources */
           for (ULONG i = 0; i < pRcList->Count; ++i)
           {
               PCM_FULL_RESOURCE_DESCRIPTOR pFRc = &pRcList->List[i];
               for (ULONG j = 0; j < pFRc->PartialResourceList.Count; ++j)
               {
                   PCM_PARTIAL_RESOURCE_DESCRIPTOR pPRc = &pFRc->PartialResourceList.PartialDescriptors[j];
                   switch (pPRc->Type)
                   {
                       case CmResourceTypePort:
                           break;
                       case CmResourceTypeInterrupt:
                           break;
                       case CmResourceTypeMemory:
                           /* we assume there is one memory segment */
                           Assert(pHwResources->phVRAM.QuadPart == 0);
                           pHwResources->phVRAM = pPRc->u.Memory.Start;
                           Assert(pHwResources->phVRAM.QuadPart != 0);
                           pHwResources->ulApertureSize = pPRc->u.Memory.Length;
                           Assert(pHwResources->cbVRAM <= pHwResources->ulApertureSize);
                           break;
                       case CmResourceTypeDma:
                           break;
                       case CmResourceTypeDeviceSpecific:
                           break;
                       case CmResourceTypeBusNumber:
                           break;
                       default:
                           break;
                   }
               }
           }
       }
       else
       {
           LOGREL(("HGSMI unsupported, returning err"));
           /** @todo report a better status */
           Status = STATUS_UNSUCCESSFUL;
       }
    }
    else
    {
        LOGREL(("VBE card not found, returning err"));
        Status = STATUS_UNSUCCESSFUL;
    }


    return Status;
}

static void vboxWddmDevExtZeroinit(PVBOXMP_DEVEXT pDevExt, CONST PDEVICE_OBJECT pPDO)
{
    memset(pDevExt, 0, sizeof (VBOXMP_DEVEXT));
    pDevExt->pPDO = pPDO;
    PWCHAR pName = (PWCHAR)(((uint8_t*)pDevExt) + VBOXWDDM_ROUNDBOUND(sizeof(VBOXMP_DEVEXT), 8));
    RtlInitUnicodeString(&pDevExt->RegKeyName, pName);

    VBoxVidPnSourcesInit(pDevExt->aSources, RT_ELEMENTS(pDevExt->aSources), 0);

    VBoxVidPnTargetsInit(pDevExt->aTargets, RT_ELEMENTS(pDevExt->aTargets), 0);
}

static void vboxWddmSetupDisplaysLegacy(PVBOXMP_DEVEXT pDevExt)
{
    /* For WDDM, we simply store the number of monitors as we will deal with
     * VidPN stuff later */
    int rc = STATUS_SUCCESS;

    if (VBoxCommonFromDeviceExt(pDevExt)->bHGSMI)
    {
        ULONG ulAvailable = VBoxCommonFromDeviceExt(pDevExt)->cbVRAM
                            - VBoxCommonFromDeviceExt(pDevExt)->cbMiniportHeap
                            - VBVA_ADAPTER_INFORMATION_SIZE;

        ULONG ulSize;
        ULONG offset;
#ifdef VBOX_WITH_VDMA
        ulSize = ulAvailable / 2;
        if (ulSize > VBOXWDDM_C_VDMA_BUFFER_SIZE)
            ulSize = VBOXWDDM_C_VDMA_BUFFER_SIZE;

        /* Align down to 4096 bytes. */
        ulSize &= ~0xFFF;
        offset = ulAvailable - ulSize;

        Assert(!(offset & 0xFFF));
#else
        offset = ulAvailable;
#endif
        rc = vboxVdmaCreate (pDevExt, &pDevExt->u.primary.Vdma
#ifdef VBOX_WITH_VDMA
                , offset, ulSize
#endif
                );
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
#ifdef VBOX_VDMA_WITH_WATCHDOG
            vboxWddmWdInit(pDevExt);
#endif
            /* can enable it right away since the host does not need any screen/FB info
             * for basic DMA functionality */
            rc = vboxVdmaEnable(pDevExt, &pDevExt->u.primary.Vdma);
            AssertRC(rc);
            if (RT_FAILURE(rc))
                vboxVdmaDestroy(pDevExt, &pDevExt->u.primary.Vdma);
        }

        ulAvailable = offset;
        ulSize = ulAvailable/2;
        offset = ulAvailable - ulSize;

        NTSTATUS Status = vboxVideoAMgrCreate(pDevExt, &pDevExt->AllocMgr, offset, ulSize);
        AssertNtStatusSuccess(Status);
        if (Status != STATUS_SUCCESS)
        {
            offset = ulAvailable;
        }

#ifdef VBOXWDDM_RENDER_FROM_SHADOW
        if (RT_SUCCESS(rc))
        {
            ulAvailable = offset;
            ulSize = ulAvailable / 2;
            ulSize /= VBoxCommonFromDeviceExt(pDevExt)->cDisplays;
            Assert(ulSize > VBVA_MIN_BUFFER_SIZE);
            if (ulSize > VBVA_MIN_BUFFER_SIZE)
            {
                ULONG ulRatio = ulSize/VBVA_MIN_BUFFER_SIZE;
                ulRatio >>= 4; /* /= 16; */
                if (ulRatio)
                    ulSize = VBVA_MIN_BUFFER_SIZE * ulRatio;
                else
                    ulSize = VBVA_MIN_BUFFER_SIZE;
            }
            else
            {
                /** @todo ?? */
            }

            ulSize &= ~0xFFF;
            Assert(ulSize);

            Assert(ulSize * VBoxCommonFromDeviceExt(pDevExt)->cDisplays < ulAvailable);

            for (int i = VBoxCommonFromDeviceExt(pDevExt)->cDisplays-1; i >= 0; --i)
            {
                offset -= ulSize;
                rc = vboxVbvaCreate(pDevExt, &pDevExt->aSources[i].Vbva, offset, ulSize, i);
                AssertRC(rc);
                if (RT_SUCCESS(rc))
                {
                    rc = vboxVbvaEnable(pDevExt, &pDevExt->aSources[i].Vbva);
                    AssertRC(rc);
                    if (RT_FAILURE(rc))
                    {
                        /** @todo de-initialize */
                    }
                }
            }
        }
#endif

        rc = VBoxMPCmnMapAdapterMemory(VBoxCommonFromDeviceExt(pDevExt), (void**)&pDevExt->pvVisibleVram,
                                       0, vboxWddmVramCpuVisibleSize(pDevExt));
        Assert(rc == VINF_SUCCESS);
        if (rc != VINF_SUCCESS)
            pDevExt->pvVisibleVram = NULL;

        if (RT_FAILURE(rc))
            VBoxCommonFromDeviceExt(pDevExt)->bHGSMI = FALSE;
    }
}
#ifdef VBOX_WITH_CROGL
static NTSTATUS vboxWddmSetupDisplaysNew(PVBOXMP_DEVEXT pDevExt)
{
    if (!VBoxCommonFromDeviceExt(pDevExt)->bHGSMI)
        return STATUS_UNSUCCESSFUL;

    ULONG cbAvailable = VBoxCommonFromDeviceExt(pDevExt)->cbVRAM
                            - VBoxCommonFromDeviceExt(pDevExt)->cbMiniportHeap
                            - VBVA_ADAPTER_INFORMATION_SIZE;

    /* Size of the VBVA buffer which is used to pass VBOXCMDVBVA_* commands to the host.
     * Estimate max 4KB per command.
     */
    ULONG cbCmdVbva = VBOXCMDVBVA_BUFFERSIZE(4096);

    if (cbCmdVbva >= cbAvailable)
    {
        WARN(("too few VRAM memory fatal, %d, requested for CmdVbva %d", cbAvailable, cbCmdVbva));
        return STATUS_UNSUCCESSFUL;
    }


    ULONG offCmdVbva = cbAvailable - cbCmdVbva;

    int rc = VBoxCmdVbvaCreate(pDevExt, &pDevExt->CmdVbva, offCmdVbva, cbCmdVbva);
    if (RT_SUCCESS(rc))
    {
        rc = VBoxCmdVbvaEnable(pDevExt, &pDevExt->CmdVbva);
        if (RT_SUCCESS(rc))
        {
            rc = VBoxMPCmnMapAdapterMemory(VBoxCommonFromDeviceExt(pDevExt), (void**)&pDevExt->pvVisibleVram,
                                           0, vboxWddmVramCpuVisibleSize(pDevExt));
            if (RT_SUCCESS(rc))
                return STATUS_SUCCESS;
            else
                WARN(("VBoxMPCmnMapAdapterMemory failed, rc %d", rc));

            VBoxCmdVbvaDisable(pDevExt, &pDevExt->CmdVbva);
        }
        else
            WARN(("VBoxCmdVbvaEnable failed, rc %d", rc));

        VBoxCmdVbvaDestroy(pDevExt, &pDevExt->CmdVbva);
    }
    else
        WARN(("VBoxCmdVbvaCreate failed, rc %d", rc));

    return STATUS_UNSUCCESSFUL;
}
#endif
static NTSTATUS vboxWddmSetupDisplays(PVBOXMP_DEVEXT pDevExt)
{
#ifdef VBOX_WITH_CROGL
    if (pDevExt->fCmdVbvaEnabled)
    {
        NTSTATUS Status = vboxWddmSetupDisplaysNew(pDevExt);
        if (!NT_SUCCESS(Status))
            VBoxCommonFromDeviceExt(pDevExt)->bHGSMI = FALSE;
        return Status;
    }
#endif

    vboxWddmSetupDisplaysLegacy(pDevExt);
    return VBoxCommonFromDeviceExt(pDevExt)->bHGSMI ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static int vboxWddmFreeDisplays(PVBOXMP_DEVEXT pDevExt)
{
    int rc = VINF_SUCCESS;

    Assert(pDevExt->pvVisibleVram);
    if (pDevExt->pvVisibleVram)
        VBoxMPCmnUnmapAdapterMemory(VBoxCommonFromDeviceExt(pDevExt), (void**)&pDevExt->pvVisibleVram);

    if (pDevExt->fCmdVbvaEnabled)
    {
        rc = VBoxCmdVbvaDisable(pDevExt, &pDevExt->CmdVbva);
        if (RT_SUCCESS(rc))
        {
            rc = VBoxCmdVbvaDestroy(pDevExt, &pDevExt->CmdVbva);
            if (RT_FAILURE(rc))
                WARN(("VBoxCmdVbvaDestroy failed %d", rc));
        }
        else
            WARN(("VBoxCmdVbvaDestroy failed %d", rc));

    }
    else
    {
        for (int i = VBoxCommonFromDeviceExt(pDevExt)->cDisplays-1; i >= 0; --i)
        {
            rc = vboxVbvaDisable(pDevExt, &pDevExt->aSources[i].Vbva);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                rc = vboxVbvaDestroy(pDevExt, &pDevExt->aSources[i].Vbva);
                AssertRC(rc);
                if (RT_FAILURE(rc))
                {
                    /** @todo */
                }
            }
        }

        vboxVideoAMgrDestroy(pDevExt, &pDevExt->AllocMgr);

        rc = vboxVdmaDisable(pDevExt, &pDevExt->u.primary.Vdma);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
#ifdef VBOX_VDMA_WITH_WATCHDOG
            vboxWddmWdTerm(pDevExt);
#endif
            rc = vboxVdmaDestroy(pDevExt, &pDevExt->u.primary.Vdma);
            AssertRC(rc);
        }
    }

    return rc;
}


/* driver callbacks */
NTSTATUS DxgkDdiAddDevice(
    IN CONST PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PVOID *MiniportDeviceContext
    )
{
    /* The DxgkDdiAddDevice function should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, pdo(0x%x)", PhysicalDeviceObject));

    vboxVDbgBreakFv();

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXMP_DEVEXT pDevExt = NULL;

    WCHAR RegKeyBuf[512];
    ULONG cbRegKeyBuf = sizeof (RegKeyBuf);

    Status = IoGetDeviceProperty (PhysicalDeviceObject,
                                  DevicePropertyDriverKeyName,
                                  cbRegKeyBuf,
                                  RegKeyBuf,
                                  &cbRegKeyBuf);
    AssertNtStatusSuccess(Status);
    if (Status == STATUS_SUCCESS)
    {
        pDevExt = (PVBOXMP_DEVEXT)vboxWddmMemAllocZero(VBOXWDDM_ROUNDBOUND(sizeof(VBOXMP_DEVEXT), 8) + cbRegKeyBuf);
        if (pDevExt)
        {
            PWCHAR pName = (PWCHAR)(((uint8_t*)pDevExt) + VBOXWDDM_ROUNDBOUND(sizeof(VBOXMP_DEVEXT), 8));
            memcpy(pName, RegKeyBuf, cbRegKeyBuf);
            vboxWddmDevExtZeroinit(pDevExt, PhysicalDeviceObject);
            *MiniportDeviceContext = pDevExt;
        }
        else
        {
            Status  = STATUS_NO_MEMORY;
            LOGREL(("ERROR, failed to create context"));
        }
    }

    LOGF(("LEAVE, Status(0x%x), pDevExt(0x%x)", Status, pDevExt));

    return Status;
}

NTSTATUS DxgkDdiStartDevice(
    IN CONST PVOID  MiniportDeviceContext,
    IN PDXGK_START_INFO  DxgkStartInfo,
    IN PDXGKRNL_INTERFACE  DxgkInterface,
    OUT PULONG  NumberOfVideoPresentSources,
    OUT PULONG  NumberOfChildren
    )
{
    /* The DxgkDdiStartDevice function should be made pageable. */
    PAGED_CODE();

    NTSTATUS Status;

    LOGF(("ENTER, context(0x%x)", MiniportDeviceContext));

    vboxVDbgBreakFv();

    if ( ARGUMENT_PRESENT(MiniportDeviceContext) &&
        ARGUMENT_PRESENT(DxgkInterface) &&
        ARGUMENT_PRESENT(DxgkStartInfo) &&
        ARGUMENT_PRESENT(NumberOfVideoPresentSources) &&
        ARGUMENT_PRESENT(NumberOfChildren)
        )
    {
        PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;

        vboxWddmVGuidGet(pDevExt);

        /* Save DeviceHandle and function pointers supplied by the DXGKRNL_INTERFACE structure passed to DxgkInterface. */
        memcpy(&pDevExt->u.primary.DxgkInterface, DxgkInterface, sizeof (DXGKRNL_INTERFACE));

        /* Allocate a DXGK_DEVICE_INFO structure, and call DxgkCbGetDeviceInformation to fill in the members of that structure, which include the registry path, the PDO, and a list of translated resources for the display adapter represented by MiniportDeviceContext. Save selected members (ones that the display miniport driver will need later)
         * of the DXGK_DEVICE_INFO structure in the context block represented by MiniportDeviceContext. */
        DXGK_DEVICE_INFO DeviceInfo;
        Status = pDevExt->u.primary.DxgkInterface.DxgkCbGetDeviceInformation (pDevExt->u.primary.DxgkInterface.DeviceHandle, &DeviceInfo);
        if (Status == STATUS_SUCCESS)
        {
            VBOXWDDM_HWRESOURCES HwRc;
            Status = vboxWddmPickResources(pDevExt, &DeviceInfo, &HwRc);
            if (Status == STATUS_SUCCESS)
            {
#ifdef VBOX_WITH_CROGL
                pDevExt->f3DEnabled = VBoxMpCrCtlConIs3DSupported();

                if (pDevExt->f3DEnabled)
                {
                    pDevExt->fTexPresentEnabled = !!(VBoxMpCrGetHostCaps() & CR_VBOX_CAP_TEX_PRESENT);
                    pDevExt->fCmdVbvaEnabled = !!(VBoxMpCrGetHostCaps() & CR_VBOX_CAP_CMDVBVA);
# if 0
                    pDevExt->fComplexTopologiesEnabled = pDevExt->fCmdVbvaEnabled;
# else
                    pDevExt->fComplexTopologiesEnabled = FALSE;
# endif
                }
                else
                {
                    pDevExt->fTexPresentEnabled = FALSE;
                    pDevExt->fCmdVbvaEnabled = FALSE;
                    pDevExt->fComplexTopologiesEnabled = FALSE;
                }
#endif

                /* Guest supports only HGSMI, the old VBVA via VMMDev is not supported.
                 * The host will however support both old and new interface to keep compatibility
                 * with old guest additions.
                 */
                VBoxSetupDisplaysHGSMI(VBoxCommonFromDeviceExt(pDevExt),
                                       HwRc.phVRAM, HwRc.ulApertureSize, HwRc.cbVRAM,
                                       VBVACAPS_COMPLETEGCMD_BY_IOREAD | VBVACAPS_IRQ);
                if (VBoxCommonFromDeviceExt(pDevExt)->bHGSMI)
                {
                    vboxWddmSetupDisplays(pDevExt);
                    if (!VBoxCommonFromDeviceExt(pDevExt)->bHGSMI)
                        VBoxFreeDisplaysHGSMI(VBoxCommonFromDeviceExt(pDevExt));
                }
                if (VBoxCommonFromDeviceExt(pDevExt)->bHGSMI)
                {
                    LOGREL(("using HGSMI"));
                    *NumberOfVideoPresentSources = VBoxCommonFromDeviceExt(pDevExt)->cDisplays;
                    *NumberOfChildren = VBoxCommonFromDeviceExt(pDevExt)->cDisplays;
                    LOG(("sources(%d), children(%d)", *NumberOfVideoPresentSources, *NumberOfChildren));

                    vboxVdmaDdiNodesInit(pDevExt);
                    vboxVideoCmInit(&pDevExt->CmMgr);
                    vboxVideoCmInit(&pDevExt->SeamlessCtxMgr);
                    InitializeListHead(&pDevExt->SwapchainList3D);
                    pDevExt->cContexts3D = 0;
                    pDevExt->cContexts2D = 0;
                    pDevExt->cContextsDispIfResize = 0;
                    pDevExt->cUnlockedVBVADisabled = 0;
                    pDevExt->fDisableTargetUpdate = 0;
                    VBOXWDDM_CTXLOCK_INIT(pDevExt);
                    KeInitializeSpinLock(&pDevExt->SynchLock);

                    VBoxCommonFromDeviceExt(pDevExt)->fAnyX = VBoxVideoAnyWidthAllowed();
#if 0
                    vboxShRcTreeInit(pDevExt);
#endif

#ifdef VBOX_WITH_VIDEOHWACCEL
                    vboxVhwaInit(pDevExt);
#endif
                    VBoxWddmSlInit(pDevExt);

#ifdef VBOX_WITH_CROGL
                    VBoxMpCrShgsmiTransportCreate(&pDevExt->CrHgsmiTransport, pDevExt);
#endif

                    for (UINT i = 0; i < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
                    {
                        PVBOXWDDM_SOURCE pSource = &pDevExt->aSources[i];
                        KeInitializeSpinLock(&pSource->AllocationLock);
#ifdef VBOX_WITH_CROGL
                        VBoxVrListInit(&pSource->VrList);
#endif
                    }

                    DWORD dwVal = VBOXWDDM_CFG_DRV_DEFAULT;
                    HANDLE hKey = NULL;
                    WCHAR aNameBuf[100];

                    Status = IoOpenDeviceRegistryKey(pDevExt->pPDO, PLUGPLAY_REGKEY_DRIVER, GENERIC_READ, &hKey);
                    if (!NT_SUCCESS(Status))
                    {
                        WARN(("IoOpenDeviceRegistryKey failed, Status = 0x%x", Status));
                        hKey = NULL;
                    }


                    if (hKey)
                    {
                        Status = vboxWddmRegQueryValueDword(hKey, VBOXWDDM_REG_DRV_FLAGS_NAME, &dwVal);
                        if (!NT_SUCCESS(Status))
                        {
                            LOG(("vboxWddmRegQueryValueDword failed, Status = 0x%x", Status));
                            dwVal = VBOXWDDM_CFG_DRV_DEFAULT;
                        }
                    }

                    pDevExt->dwDrvCfgFlags = dwVal;

                    for (UINT i = 0; i < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
                    {
                        PVBOXWDDM_TARGET pTarget = &pDevExt->aTargets[i];
                        if (i == 0 || (pDevExt->dwDrvCfgFlags & VBOXWDDM_CFG_DRV_SECONDARY_TARGETS_CONNECTED) || !hKey)
                        {
                            pTarget->fConnected = true;
                            pTarget->fConfigured = true;
                        }
                        else if (hKey)
                        {
                            swprintf(aNameBuf, L"%s%d", VBOXWDDM_REG_DRV_DISPFLAGS_PREFIX, i);
                            Status = vboxWddmRegQueryValueDword(hKey, aNameBuf, &dwVal);
                            if (NT_SUCCESS(Status))
                            {
                                pTarget->fConnected = !!(dwVal & VBOXWDDM_CFG_DRVTARGET_CONNECTED);
                                pTarget->fConfigured = true;
                            }
                            else
                            {
                                WARN(("vboxWddmRegQueryValueDword failed, Status = 0x%x", Status));
                                pTarget->fConnected = false;
                                pTarget->fConfigured = false;
                            }
                        }
                    }

                    if (hKey)
                    {
                        NTSTATUS rcNt2 = ZwClose(hKey);
                        Assert(rcNt2 == STATUS_SUCCESS); NOREF(rcNt2);
                    }

                    Status = STATUS_SUCCESS;
#ifdef VBOX_WDDM_WIN8
                    DXGK_DISPLAY_INFORMATION DisplayInfo;
                    Status = pDevExt->u.primary.DxgkInterface.DxgkCbAcquirePostDisplayOwnership(pDevExt->u.primary.DxgkInterface.DeviceHandle,
                            &DisplayInfo);
                    if (NT_SUCCESS(Status))
                    {
                        PVBOXWDDM_SOURCE pSource = &pDevExt->aSources[0];
                        PHYSICAL_ADDRESS PhAddr;
                        /* display info may sometimes not be valid, e.g. on from-full-graphics wddm driver update
                         * ensure we have something meaningful here */
                        if (!DisplayInfo.Width)
                        {
                            PhAddr = VBoxCommonFromDeviceExt(pDevExt)->phVRAM;
                            vboxWddmDiInitDefault(&DisplayInfo, PhAddr, 0);
                        }
                        else
                        {
                            PhAddr = DisplayInfo.PhysicAddress;
                            DisplayInfo.TargetId = 0;
                        }

                        vboxWddmDiToAllocData(pDevExt, &DisplayInfo, &pSource->AllocData);

                        /* init the rest source infos with some default values */
                        for (UINT i = 1; i < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
                        {
                            PhAddr.QuadPart += pSource->AllocData.SurfDesc.cbSize;
                            PhAddr.QuadPart = ROUND_TO_PAGES(PhAddr.QuadPart);
                            vboxWddmDiInitDefault(&DisplayInfo, PhAddr, i);
                            pSource = &pDevExt->aSources[i];
                            vboxWddmDiToAllocData(pDevExt, &DisplayInfo, &pSource->AllocData);
                        }
                    }
                    else
                    {
                        WARN(("DxgkCbAcquirePostDisplayOwnership failed, Status 0x%x", Status));
                    }
#endif

                    VBoxWddmVModesInit(pDevExt);
                }
                else
                {
                    LOGREL(("HGSMI failed to initialize, returning err"));

                    /** @todo report a better status */
                    Status = STATUS_UNSUCCESSFUL;
                }
            }
            else
            {
                LOGREL(("vboxWddmPickResources failed Status(0x%x), returning err", Status));
                Status = STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            LOGREL(("DxgkCbGetDeviceInformation failed Status(0x%x), returning err", Status));
        }
    }
    else
    {
        LOGREL(("invalid parameter, returning err"));
        Status = STATUS_INVALID_PARAMETER;
    }

    LOGF(("LEAVE, status(0x%x)", Status));

    return Status;
}

NTSTATUS DxgkDdiStopDevice(
    IN CONST PVOID MiniportDeviceContext
    )
{
    /* The DxgkDdiStopDevice function should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%p)", MiniportDeviceContext));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef VBOX_WITH_CROGL
    if (pDevExt->u32CrConDefaultClientID)
        VBoxMpCrCtlConDisconnect(pDevExt, &pDevExt->CrCtlCon, pDevExt->u32CrConDefaultClientID);

    VBoxMpCrShgsmiTransportTerm(&pDevExt->CrHgsmiTransport);
#endif

    VBoxWddmSlTerm(pDevExt);

    vboxVideoCmTerm(&pDevExt->CmMgr);

    vboxVideoCmTerm(&pDevExt->SeamlessCtxMgr);

    /* do everything we did on DxgkDdiStartDevice in the reverse order */
#ifdef VBOX_WITH_VIDEOHWACCEL
    vboxVhwaFree(pDevExt);
#endif
#if 0
    vboxShRcTreeTerm(pDevExt);
#endif

    int rc = vboxWddmFreeDisplays(pDevExt);
    if (RT_SUCCESS(rc))
        VBoxFreeDisplaysHGSMI(VBoxCommonFromDeviceExt(pDevExt));
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        vboxWddmVGuidFree(pDevExt);

        VBoxWddmVModesCleanup();
        /* revert back to the state we were right after the DxgkDdiAddDevice */
        vboxWddmDevExtZeroinit(pDevExt, pDevExt->pPDO);
    }
    else
        Status = STATUS_UNSUCCESSFUL;

    return Status;
}

NTSTATUS DxgkDdiRemoveDevice(
    IN CONST PVOID MiniportDeviceContext
    )
{
    /* DxgkDdiRemoveDevice should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%p)", MiniportDeviceContext));

    vboxVDbgBreakFv();

    vboxWddmMemFree(MiniportDeviceContext);

    LOGF(("LEAVE, context(0x%p)", MiniportDeviceContext));

    return STATUS_SUCCESS;
}

NTSTATUS DxgkDdiDispatchIoRequest(
    IN CONST PVOID MiniportDeviceContext,
    IN ULONG VidPnSourceId,
    IN PVIDEO_REQUEST_PACKET VideoRequestPacket
    )
{
    LOGF(("ENTER, context(0x%p), ctl(0x%x)", MiniportDeviceContext, VideoRequestPacket->IoControlCode));

    AssertBreakpoint();
#if 0
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;

    switch (VideoRequestPacket->IoControlCode)
    {
        case IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES:
        {
            if (VideoRequestPacket->OutputBufferLength < sizeof(VIDEO_COLOR_CAPABILITIES))
            {
                AssertBreakpoint();
                VideoRequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            VIDEO_COLOR_CAPABILITIES *pCaps = (VIDEO_COLOR_CAPABILITIES*)VideoRequestPacket->OutputBuffer;

            pCaps->Length = sizeof (VIDEO_COLOR_CAPABILITIES);
            pCaps->AttributeFlags = VIDEO_DEVICE_COLOR;
            pCaps->RedPhosphoreDecay = 0;
            pCaps->GreenPhosphoreDecay = 0;
            pCaps->BluePhosphoreDecay = 0;
            pCaps->WhiteChromaticity_x = 3127;
            pCaps->WhiteChromaticity_y = 3290;
            pCaps->WhiteChromaticity_Y = 0;
            pCaps->RedChromaticity_x = 6700;
            pCaps->RedChromaticity_y = 3300;
            pCaps->GreenChromaticity_x = 2100;
            pCaps->GreenChromaticity_y = 7100;
            pCaps->BlueChromaticity_x = 1400;
            pCaps->BlueChromaticity_y = 800;
            pCaps->WhiteGamma = 0;
            pCaps->RedGamma = 20000;
            pCaps->GreenGamma = 20000;
            pCaps->BlueGamma = 20000;

            VideoRequestPacket->StatusBlock->Status = NO_ERROR;
            VideoRequestPacket->StatusBlock->Information = sizeof (VIDEO_COLOR_CAPABILITIES);
            break;
        }
#if 0
        case IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS:
        {
            if (VideoRequestPacket->OutputBufferLength < sizeof(VIDEOPARAMETERS)
                    || VideoRequestPacket->InputBufferLength < sizeof(VIDEOPARAMETERS))
            {
                AssertBreakpoint();
                VideoRequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }

            Result = VBoxVideoResetDevice((PVBOXMP_DEVEXT)HwDeviceExtension,
                                          RequestPacket->StatusBlock);
            break;
        }
#endif
        default:
            AssertBreakpoint();
            VideoRequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;
            VideoRequestPacket->StatusBlock->Information = 0;
    }
#else
    RT_NOREF(MiniportDeviceContext, VidPnSourceId, VideoRequestPacket);
#endif
    LOGF(("LEAVE, context(0x%p), ctl(0x%x)", MiniportDeviceContext, VideoRequestPacket->IoControlCode));

    return STATUS_SUCCESS;
}

#ifdef VBOX_WITH_CROGL
BOOLEAN DxgkDdiInterruptRoutineNew(
    IN CONST PVOID MiniportDeviceContext,
    IN ULONG MessageNumber
    )
{
    RT_NOREF(MessageNumber);
//    LOGF(("ENTER, context(0x%p), msg(0x%x)", MiniportDeviceContext, MessageNumber));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;
    BOOLEAN bOur = FALSE;
    bool bNeedDpc = FALSE;
    if (!VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags) /* If HGSMI is enabled at all. */
    {
        WARN(("ISR called with hgsmi disabled!"));
        return FALSE;
    }

    VBOXVTLIST CtlList;
    vboxVtListInit(&CtlList);
#ifdef VBOX_WITH_VIDEOHWACCEL
    VBOXVTLIST VhwaCmdList;
    vboxVtListInit(&VhwaCmdList);
#endif

    uint32_t flags = VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags->u32HostFlags;
    bOur = (flags & HGSMIHOSTFLAGS_IRQ);

    if (bOur)
        VBoxHGSMIClearIrq(&VBoxCommonFromDeviceExt(pDevExt)->hostCtx);

    bNeedDpc |= VBoxCmdVbvaCheckCompletedIrq(pDevExt, &pDevExt->CmdVbva);

    do {
        /* re-read flags right here to avoid host-guest racing,
         * i.e. the situation:
         * 1. guest reads flags ant it is HGSMIHOSTFLAGS_IRQ, i.e. HGSMIHOSTFLAGS_GCOMMAND_COMPLETED no set
         * 2. host completes guest command, sets the HGSMIHOSTFLAGS_GCOMMAND_COMPLETED and raises IRQ
         * 3. guest clleans IRQ and exits  */
        flags = VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags->u32HostFlags;

        if (flags & HGSMIHOSTFLAGS_GCOMMAND_COMPLETED)
        {
            /* read the command offset */
            HGSMIOFFSET offCmd = VBVO_PORT_READ_U32(VBoxCommonFromDeviceExt(pDevExt)->guestCtx.port);
            if (offCmd == HGSMIOFFSET_VOID)
            {
                WARN(("void command offset!"));
                continue;
            }

            uint16_t chInfo;
            uint8_t RT_UNTRUSTED_VOLATILE_HOST *pvCmd =
                HGSMIBufferDataAndChInfoFromOffset(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx.Heap.area, offCmd, &chInfo);
            if (!pvCmd)
            {
                WARN(("zero cmd"));
                continue;
            }

            switch (chInfo)
            {
                case VBVA_CMDVBVA_CTL:
                {
                    int rc = VBoxSHGSMICommandProcessCompletion(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx,
                                                                (VBOXSHGSMIHEADER *)pvCmd, TRUE /*bool bIrq*/ , &CtlList);
                    AssertRC(rc);
                    break;
                }
#ifdef VBOX_WITH_VIDEOHWACCEL
                case VBVA_VHWA_CMD:
                {
                    vboxVhwaPutList(&VhwaCmdList, (VBOXVHWACMD*)pvCmd);
                    break;
                }
#endif /* # ifdef VBOX_WITH_VIDEOHWACCEL */
                default:
                    AssertBreakpoint();
            }
        }
        else if (flags & HGSMIHOSTFLAGS_COMMANDS_PENDING)
        {
            AssertBreakpoint();
            /** @todo FIXME: implement !!! */
        }
        else
            break;
    } while (1);

    if (!vboxVtListIsEmpty(&CtlList))
    {
        vboxVtListCat(&pDevExt->CtlList, &CtlList);
        bNeedDpc = TRUE;
        ASMAtomicWriteU32(&pDevExt->fCompletingCommands, 1);
    }

    if (!vboxVtListIsEmpty(&VhwaCmdList))
    {
        vboxVtListCat(&pDevExt->VhwaCmdList, &VhwaCmdList);
        bNeedDpc = TRUE;
        ASMAtomicWriteU32(&pDevExt->fCompletingCommands, 1);
    }

    bNeedDpc |= !vboxVdmaDdiCmdIsCompletedListEmptyIsr(pDevExt);

    if (bOur)
    {
#ifdef VBOX_VDMA_WITH_WATCHDOG
        if (flags & HGSMIHOSTFLAGS_WATCHDOG)
        {
            Assert(0);
        }
#endif
        if (flags & HGSMIHOSTFLAGS_VSYNC)
        {
            Assert(0);
            DXGKARGCB_NOTIFY_INTERRUPT_DATA notify;
            for (UINT i = 0; i < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
            {
                PVBOXWDDM_TARGET pTarget = &pDevExt->aTargets[i];
                if (pTarget->fConnected)
                {
                    memset(&notify, 0, sizeof(DXGKARGCB_NOTIFY_INTERRUPT_DATA));
                    notify.InterruptType = DXGK_INTERRUPT_CRTC_VSYNC;
                    notify.CrtcVsync.VidPnTargetId = i;
                    pDevExt->u.primary.DxgkInterface.DxgkCbNotifyInterrupt(pDevExt->u.primary.DxgkInterface.DeviceHandle, &notify);
                    bNeedDpc = TRUE;
                }
            }
        }
    }

    if (pDevExt->bNotifyDxDpc)
        bNeedDpc = TRUE;

    if (bNeedDpc)
        pDevExt->u.primary.DxgkInterface.DxgkCbQueueDpc(pDevExt->u.primary.DxgkInterface.DeviceHandle);

    return bOur;
}
#endif

static BOOLEAN DxgkDdiInterruptRoutineLegacy(
    IN CONST PVOID MiniportDeviceContext,
    IN ULONG MessageNumber
    )
{
    RT_NOREF(MessageNumber);
//    LOGF(("ENTER, context(0x%p), msg(0x%x)", MiniportDeviceContext, MessageNumber));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;
    BOOLEAN bOur = FALSE;
    BOOLEAN bNeedDpc = FALSE;
    if (VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags) /* If HGSMI is enabled at all. */
    {
        VBOXVTLIST CtlList;
#ifdef VBOX_WITH_VDMA
        VBOXVTLIST DmaCmdList;
#endif
        vboxVtListInit(&CtlList);
#ifdef VBOX_WITH_VDMA
        vboxVtListInit(&DmaCmdList);
#endif

#ifdef VBOX_WITH_VIDEOHWACCEL
        VBOXVTLIST VhwaCmdList;
        vboxVtListInit(&VhwaCmdList);
#endif

        uint32_t flags = VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags->u32HostFlags;
        bOur = (flags & HGSMIHOSTFLAGS_IRQ);

        if (bOur)
            VBoxHGSMIClearIrq(&VBoxCommonFromDeviceExt(pDevExt)->hostCtx);

        do
        {
            if (flags & HGSMIHOSTFLAGS_GCOMMAND_COMPLETED)
            {
                /* read the command offset */
                HGSMIOFFSET offCmd = VBVO_PORT_READ_U32(VBoxCommonFromDeviceExt(pDevExt)->guestCtx.port);
                Assert(offCmd != HGSMIOFFSET_VOID);
                if (offCmd != HGSMIOFFSET_VOID)
                {
                    VBOXWDDM_HGSMICMD_TYPE enmType = vboxWddmHgsmiGetCmdTypeFromOffset(pDevExt, offCmd);
                    PVBOXVTLIST pList;
                    PVBOXSHGSMI pHeap;
                    switch (enmType)
                    {
#ifdef VBOX_WITH_VDMA
                        case VBOXWDDM_HGSMICMD_TYPE_DMACMD:
                            pList = &DmaCmdList;
                            pHeap = &pDevExt->u.primary.Vdma.CmdHeap;
                            break;
#endif
                        case VBOXWDDM_HGSMICMD_TYPE_CTL:
                            pList = &CtlList;
                            pHeap = &VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx;
                            break;
                        default:
                            AssertBreakpoint();
                            pList = NULL;
                            pHeap = NULL;
                            break;
                    }

                    if (pHeap)
                    {
                        uint16_t chInfo;
                        uint8_t RT_UNTRUSTED_VOLATILE_GUEST *pvCmd =
                            HGSMIBufferDataAndChInfoFromOffset(&pHeap->Heap.area, offCmd, &chInfo);
                        Assert(pvCmd);
                        if (pvCmd)
                        {
                            switch (chInfo)
                            {
#ifdef VBOX_WITH_VDMA
                                case VBVA_VDMA_CMD:
                                case VBVA_VDMA_CTL:
                                {
                                    int rc = VBoxSHGSMICommandProcessCompletion(pHeap, (VBOXSHGSMIHEADER*)pvCmd,
                                                                                TRUE /*bool bIrq*/ , pList);
                                    AssertRC(rc);
                                    break;
                                }
#endif
#ifdef VBOX_WITH_VIDEOHWACCEL
                                case VBVA_VHWA_CMD:
                                {
                                    vboxVhwaPutList(&VhwaCmdList, (VBOXVHWACMD*)pvCmd);
                                    break;
                                }
#endif /* # ifdef VBOX_WITH_VIDEOHWACCEL */
                                default:
                                    AssertBreakpoint();
                            }
                        }
                    }
                }
            }
            else if (flags & HGSMIHOSTFLAGS_COMMANDS_PENDING)
            {
                AssertBreakpoint();
                /** @todo FIXME: implement !!! */
            }
            else
                break;

            flags = VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags->u32HostFlags;
        } while (1);

        if (!vboxVtListIsEmpty(&CtlList))
        {
            vboxVtListCat(&pDevExt->CtlList, &CtlList);
            bNeedDpc = TRUE;
        }
#ifdef VBOX_WITH_VDMA
        if (!vboxVtListIsEmpty(&DmaCmdList))
        {
            vboxVtListCat(&pDevExt->DmaCmdList, &DmaCmdList);
            bNeedDpc = TRUE;
        }
#endif
        if (!vboxVtListIsEmpty(&VhwaCmdList))
        {
            vboxVtListCat(&pDevExt->VhwaCmdList, &VhwaCmdList);
            bNeedDpc = TRUE;
        }

        bNeedDpc |= !vboxVdmaDdiCmdIsCompletedListEmptyIsr(pDevExt);

        if (pDevExt->bNotifyDxDpc)
        {
            bNeedDpc = TRUE;
        }

        if (bOur)
        {
#ifdef VBOX_VDMA_WITH_WATCHDOG
            if (flags & HGSMIHOSTFLAGS_WATCHDOG)
            {
                Assert(0);
            }
#endif
            if (flags & HGSMIHOSTFLAGS_VSYNC)
            {
                Assert(0);
                DXGKARGCB_NOTIFY_INTERRUPT_DATA notify;
                for (UINT i = 0; i < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
                {
                    PVBOXWDDM_TARGET pTarget = &pDevExt->aTargets[i];
                    if (pTarget->fConnected)
                    {
                        memset(&notify, 0, sizeof(DXGKARGCB_NOTIFY_INTERRUPT_DATA));
                        notify.InterruptType = DXGK_INTERRUPT_CRTC_VSYNC;
                        notify.CrtcVsync.VidPnTargetId = i;
                        pDevExt->u.primary.DxgkInterface.DxgkCbNotifyInterrupt(pDevExt->u.primary.DxgkInterface.DeviceHandle, &notify);
                        bNeedDpc = TRUE;
                    }
                }
            }

            if (pDevExt->bNotifyDxDpc)
            {
                bNeedDpc = TRUE;
            }

#if 0 //def DEBUG_misha
            /* this is not entirely correct since host may concurrently complete some commands and raise a new IRQ while we are here,
             * still this allows to check that the host flags are correctly cleared after the ISR */
            Assert(VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags);
            uint32_t flags = VBoxCommonFromDeviceExt(pDevExt)->hostCtx.pfHostFlags->u32HostFlags;
            Assert(flags == 0);
#endif
        }

        if (bNeedDpc)
        {
            pDevExt->u.primary.DxgkInterface.DxgkCbQueueDpc(pDevExt->u.primary.DxgkInterface.DeviceHandle);
        }
    }

//    LOGF(("LEAVE, context(0x%p), bOur(0x%x)", MiniportDeviceContext, (ULONG)bOur));

    return bOur;
}


typedef struct VBOXWDDM_DPCDATA
{
    VBOXVTLIST CtlList;
#ifdef VBOX_WITH_VDMA
    VBOXVTLIST DmaCmdList;
#endif
#ifdef VBOX_WITH_VIDEOHWACCEL
    VBOXVTLIST VhwaCmdList;
#endif
    LIST_ENTRY CompletedDdiCmdQueue;
    BOOL bNotifyDpc;
} VBOXWDDM_DPCDATA, *PVBOXWDDM_DPCDATA;

typedef struct VBOXWDDM_GETDPCDATA_CONTEXT
{
    PVBOXMP_DEVEXT pDevExt;
    VBOXWDDM_DPCDATA data;
} VBOXWDDM_GETDPCDATA_CONTEXT, *PVBOXWDDM_GETDPCDATA_CONTEXT;

BOOLEAN vboxWddmGetDPCDataCallback(PVOID Context)
{
    PVBOXWDDM_GETDPCDATA_CONTEXT pdc = (PVBOXWDDM_GETDPCDATA_CONTEXT)Context;
    PVBOXMP_DEVEXT pDevExt = pdc->pDevExt;
    vboxVtListDetach2List(&pDevExt->CtlList, &pdc->data.CtlList);
#ifdef VBOX_WITH_VDMA
    vboxVtListDetach2List(&pDevExt->DmaCmdList, &pdc->data.DmaCmdList);
#endif
#ifdef VBOX_WITH_VIDEOHWACCEL
    vboxVtListDetach2List(&pDevExt->VhwaCmdList, &pdc->data.VhwaCmdList);
#endif
#ifdef VBOX_WITH_CROGL
    if (!pDevExt->fCmdVbvaEnabled)
#endif
    {
        vboxVdmaDdiCmdGetCompletedListIsr(pDevExt, &pdc->data.CompletedDdiCmdQueue);
    }

    pdc->data.bNotifyDpc = pDevExt->bNotifyDxDpc;
    pDevExt->bNotifyDxDpc = FALSE;

    ASMAtomicWriteU32(&pDevExt->fCompletingCommands, 0);

    return TRUE;
}

#ifdef VBOX_WITH_CROGL
static VOID DxgkDdiDpcRoutineNew(
    IN CONST PVOID  MiniportDeviceContext
    )
{
//    LOGF(("ENTER, context(0x%p)", MiniportDeviceContext));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;

    pDevExt->u.primary.DxgkInterface.DxgkCbNotifyDpc(pDevExt->u.primary.DxgkInterface.DeviceHandle);

    if (ASMAtomicReadU32(&pDevExt->fCompletingCommands))
    {
        VBOXWDDM_GETDPCDATA_CONTEXT context = {0};
        BOOLEAN bRet;

        context.pDevExt = pDevExt;

        /* get DPC data at IRQL */
        NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbSynchronizeExecution(
                pDevExt->u.primary.DxgkInterface.DeviceHandle,
                vboxWddmGetDPCDataCallback,
                &context,
                0, /* IN ULONG MessageNumber */
                &bRet);
        AssertNtStatusSuccess(Status); NOREF(Status);

    //    if (context.data.bNotifyDpc)
        pDevExt->u.primary.DxgkInterface.DxgkCbNotifyDpc(pDevExt->u.primary.DxgkInterface.DeviceHandle);

        if (!vboxVtListIsEmpty(&context.data.CtlList))
        {
            int rc = VBoxSHGSMICommandPostprocessCompletion (&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, &context.data.CtlList);
            AssertRC(rc);
        }
#ifdef VBOX_WITH_VIDEOHWACCEL
        if (!vboxVtListIsEmpty(&context.data.VhwaCmdList))
        {
            vboxVhwaCompletionListProcess(pDevExt, &context.data.VhwaCmdList);
        }
#endif
    }
//    LOGF(("LEAVE, context(0x%p)", MiniportDeviceContext));
}
#endif

static VOID DxgkDdiDpcRoutineLegacy(
    IN CONST PVOID  MiniportDeviceContext
    )
{
//    LOGF(("ENTER, context(0x%p)", MiniportDeviceContext));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;

    VBOXWDDM_GETDPCDATA_CONTEXT context = {0};
    BOOLEAN bRet;

    context.pDevExt = pDevExt;

    /* get DPC data at IRQL */
    NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbSynchronizeExecution(
            pDevExt->u.primary.DxgkInterface.DeviceHandle,
            vboxWddmGetDPCDataCallback,
            &context,
            0, /* IN ULONG MessageNumber */
            &bRet);
    AssertNtStatusSuccess(Status); NOREF(Status);

//    if (context.data.bNotifyDpc)
    pDevExt->u.primary.DxgkInterface.DxgkCbNotifyDpc(pDevExt->u.primary.DxgkInterface.DeviceHandle);

    if (!vboxVtListIsEmpty(&context.data.CtlList))
    {
        int rc = VBoxSHGSMICommandPostprocessCompletion (&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, &context.data.CtlList);
        AssertRC(rc);
    }
#ifdef VBOX_WITH_VDMA
    if (!vboxVtListIsEmpty(&context.data.DmaCmdList))
    {
        int rc = VBoxSHGSMICommandPostprocessCompletion (&pDevExt->u.primary.Vdma.CmdHeap, &context.data.DmaCmdList);
        AssertRC(rc);
    }
#endif
#ifdef VBOX_WITH_VIDEOHWACCEL
    if (!vboxVtListIsEmpty(&context.data.VhwaCmdList))
    {
        vboxVhwaCompletionListProcess(pDevExt, &context.data.VhwaCmdList);
    }
#endif

    vboxVdmaDdiCmdHandleCompletedList(pDevExt, &context.data.CompletedDdiCmdQueue);

//    LOGF(("LEAVE, context(0x%p)", MiniportDeviceContext));
}

NTSTATUS DxgkDdiQueryChildRelations(
    IN CONST PVOID MiniportDeviceContext,
    IN OUT PDXGK_CHILD_DESCRIPTOR ChildRelations,
    IN ULONG ChildRelationsSize
    )
{
    RT_NOREF(ChildRelationsSize);
    /* The DxgkDdiQueryChildRelations function should be made pageable. */
    PAGED_CODE();

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;

    LOGF(("ENTER, context(0x%x)", MiniportDeviceContext));
    Assert(ChildRelationsSize == (VBoxCommonFromDeviceExt(pDevExt)->cDisplays + 1)*sizeof(DXGK_CHILD_DESCRIPTOR));
    for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
    {
        ChildRelations[i].ChildDeviceType = TypeVideoOutput;
        ChildRelations[i].ChildCapabilities.Type.VideoOutput.InterfaceTechnology = D3DKMDT_VOT_HD15; /* VGA */
        ChildRelations[i].ChildCapabilities.Type.VideoOutput.MonitorOrientationAwareness = D3DKMDT_MOA_NONE; //D3DKMDT_MOA_INTERRUPTIBLE; /* ?? D3DKMDT_MOA_NONE*/
        ChildRelations[i].ChildCapabilities.Type.VideoOutput.SupportsSdtvModes = FALSE;
        ChildRelations[i].ChildCapabilities.HpdAwareness = HpdAwarenessInterruptible; /* ?? HpdAwarenessAlwaysConnected; */
        ChildRelations[i].AcpiUid =  0; /* */
        ChildRelations[i].ChildUid = i; /* should be == target id */
    }
    LOGF(("LEAVE, context(0x%x)", MiniportDeviceContext));
    return STATUS_SUCCESS;
}

NTSTATUS DxgkDdiQueryChildStatus(
    IN CONST PVOID  MiniportDeviceContext,
    IN PDXGK_CHILD_STATUS  ChildStatus,
    IN BOOLEAN  NonDestructiveOnly
    )
{
    RT_NOREF(NonDestructiveOnly);
    /* The DxgkDdiQueryChildStatus should be made pageable. */
    PAGED_CODE();

    vboxVDbgBreakFv();

    LOGF(("ENTER, context(0x%x)", MiniportDeviceContext));

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)MiniportDeviceContext;

    if (ChildStatus->ChildUid >= (uint32_t)VBoxCommonFromDeviceExt(pDevExt)->cDisplays)
    {
        WARN(("Invalid child id %d", ChildStatus->ChildUid));
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    switch (ChildStatus->Type)
    {
        case StatusConnection:
        {
            LOGF(("StatusConnection"));
            VBOXWDDM_TARGET *pTarget = &pDevExt->aTargets[ChildStatus->ChildUid];
            BOOLEAN Connected = !!pTarget->fConnected;
            if (!Connected)
                LOGREL(("Tgt[%d] DISCONNECTED!!", ChildStatus->ChildUid));
            ChildStatus->HotPlug.Connected = !!pTarget->fConnected;
            break;
        }
        case StatusRotation:
            LOGF(("StatusRotation"));
            ChildStatus->Rotation.Angle = 0;
            break;
        default:
            WARN(("ERROR: status type: %d", ChildStatus->Type));
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    LOGF(("LEAVE, context(0x%x)", MiniportDeviceContext));

    return Status;
}

NTSTATUS DxgkDdiQueryDeviceDescriptor(
    IN CONST PVOID MiniportDeviceContext,
    IN ULONG ChildUid,
    IN OUT PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor
    )
{
    RT_NOREF(MiniportDeviceContext, ChildUid, DeviceDescriptor);
    /* The DxgkDdiQueryDeviceDescriptor should be made pageable. */
    PAGED_CODE();

    vboxVDbgBreakFv();

    LOGF(("ENTER, context(0x%x)", MiniportDeviceContext));

    LOGF(("LEAVE, context(0x%x)", MiniportDeviceContext));

    /* we do not support EDID */
    return STATUS_MONITOR_NO_DESCRIPTOR;
}

NTSTATUS DxgkDdiSetPowerState(
    IN CONST PVOID MiniportDeviceContext,
    IN ULONG DeviceUid,
    IN DEVICE_POWER_STATE DevicePowerState,
    IN POWER_ACTION ActionType
    )
{
    RT_NOREF(MiniportDeviceContext, DeviceUid, DevicePowerState, ActionType);
    /* The DxgkDdiSetPowerState function should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", MiniportDeviceContext));

    vboxVDbgBreakFv();

    LOGF(("LEAVE, context(0x%x)", MiniportDeviceContext));

    return STATUS_SUCCESS;
}

NTSTATUS DxgkDdiNotifyAcpiEvent(
    IN CONST PVOID  MiniportDeviceContext,
    IN DXGK_EVENT_TYPE  EventType,
    IN ULONG  Event,
    IN PVOID  Argument,
    OUT PULONG  AcpiFlags
    )
{
    RT_NOREF(MiniportDeviceContext, EventType, Event, Argument, AcpiFlags);
    LOGF(("ENTER, MiniportDeviceContext(0x%x)", MiniportDeviceContext));

    vboxVDbgBreakF();

    LOGF(("LEAVE, MiniportDeviceContext(0x%x)", MiniportDeviceContext));

    return STATUS_SUCCESS;
}

VOID DxgkDdiResetDevice(
    IN CONST PVOID MiniportDeviceContext
    )
{
    RT_NOREF(MiniportDeviceContext);
    /* DxgkDdiResetDevice can be called at any IRQL, so it must be in nonpageable memory.  */
    vboxVDbgBreakF();



    LOGF(("ENTER, context(0x%x)", MiniportDeviceContext));
    LOGF(("LEAVE, context(0x%x)", MiniportDeviceContext));
}

VOID DxgkDdiUnload(
    VOID
    )
{
    /* DxgkDdiUnload should be made pageable. */
    PAGED_CODE();
    LOGF((": unloading"));

    vboxVDbgBreakFv();

    VbglR0TerminateClient();

#ifdef VBOX_WITH_CROGL
    VBoxVrTerm();
#endif

    PRTLOGGER pLogger = RTLogRelSetDefaultInstance(NULL);
    if (pLogger)
    {
        RTLogDestroy(pLogger);
    }
    pLogger = RTLogSetDefaultInstance(NULL);
    if (pLogger)
    {
        RTLogDestroy(pLogger);
    }
}

NTSTATUS DxgkDdiQueryInterface(
    IN CONST PVOID MiniportDeviceContext,
    IN PQUERY_INTERFACE QueryInterface
    )
{
    RT_NOREF(MiniportDeviceContext, QueryInterface);
    LOGF(("ENTER, MiniportDeviceContext(0x%x)", MiniportDeviceContext));

    vboxVDbgBreakFv();

    LOGF(("LEAVE, MiniportDeviceContext(0x%x)", MiniportDeviceContext));

    return STATUS_NOT_SUPPORTED;
}

VOID DxgkDdiControlEtwLogging(
    IN BOOLEAN  Enable,
    IN ULONG  Flags,
    IN UCHAR  Level
    )
{
    RT_NOREF(Enable, Flags, Level);
    LOGF(("ENTER"));

    vboxVDbgBreakF();

    LOGF(("LEAVE"));
}

/**
 * DxgkDdiQueryAdapterInfo
 */
NTSTATUS APIENTRY DxgkDdiQueryAdapterInfo(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_QUERYADAPTERINFO*  pQueryAdapterInfo)
{
    /* The DxgkDdiQueryAdapterInfo should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x), Query type (%d)", hAdapter, pQueryAdapterInfo->Type));
    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    vboxVDbgBreakFv();

    switch (pQueryAdapterInfo->Type)
    {
        case DXGKQAITYPE_DRIVERCAPS:
        {
            DXGK_DRIVERCAPS *pCaps = (DXGK_DRIVERCAPS*)pQueryAdapterInfo->pOutputData;

#ifdef VBOX_WDDM_WIN8
            memset(pCaps, 0, sizeof (*pCaps));
#endif
            pCaps->HighestAcceptableAddress.LowPart = ~0UL;
#ifdef RT_ARCH_AMD64
            /* driver talks to host in terms of page numbers when reffering to RAM
             * we use uint32_t field to pass page index to host, so max would be (~0UL) << PAGE_OFFSET,
             * which seems quite enough */
            pCaps->HighestAcceptableAddress.HighPart = PAGE_OFFSET_MASK;
#endif
            pCaps->MaxPointerWidth  = VBOXWDDM_C_POINTER_MAX_WIDTH;
            pCaps->MaxPointerHeight = VBOXWDDM_C_POINTER_MAX_HEIGHT;
            pCaps->PointerCaps.Value = 3; /* Monochrome , Color*/ /* MaskedColor == Value | 4, disable for now */
#ifdef VBOX_WDDM_WIN8
            if (!g_VBoxDisplayOnly)
#endif
            {
            pCaps->MaxAllocationListSlotId = 16;
            pCaps->ApertureSegmentCommitLimit = 0;
            pCaps->InterruptMessageNumber = 0;
            pCaps->NumberOfSwizzlingRanges = 0;
            pCaps->MaxOverlays = 0;
#ifdef VBOX_WITH_VIDEOHWACCEL
            for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
            {
                if ( pDevExt->aSources[i].Vhwa.Settings.fFlags & VBOXVHWA_F_ENABLED)
                    pCaps->MaxOverlays += pDevExt->aSources[i].Vhwa.Settings.cOverlaysSupported;
            }
#endif
            pCaps->GammaRampCaps.Value = 0;
            pCaps->PresentationCaps.Value = 0;
            pCaps->PresentationCaps.NoScreenToScreenBlt = 1;
            pCaps->PresentationCaps.NoOverlapScreenBlt = 1;
            pCaps->MaxQueuedFlipOnVSync = 0; /* do we need it? */
            pCaps->FlipCaps.Value = 0;
            /* ? pCaps->FlipCaps.FlipOnVSyncWithNoWait = 1; */
            pCaps->SchedulingCaps.Value = 0;
            /* we might need it for Aero.
             * Setting this flag means we support DeviceContext, i.e.
             *  DxgkDdiCreateContext and DxgkDdiDestroyContext
             */
            pCaps->SchedulingCaps.MultiEngineAware = 1;
            pCaps->MemoryManagementCaps.Value = 0;
            /** @todo this correlates with pCaps->SchedulingCaps.MultiEngineAware */
            pCaps->MemoryManagementCaps.PagingNode = 0;
            /** @todo this correlates with pCaps->SchedulingCaps.MultiEngineAware */
            pCaps->GpuEngineTopology.NbAsymetricProcessingNodes = VBOXWDDM_NUM_NODES;
#ifdef VBOX_WDDM_WIN8
            pCaps->WDDMVersion = DXGKDDI_WDDMv1;
#endif
            }
#ifdef VBOX_WDDM_WIN8
            else
            {
                pCaps->WDDMVersion = DXGKDDI_WDDMv1_2;
            }
#endif
            break;
        }
        case DXGKQAITYPE_QUERYSEGMENT:
        {
#ifdef VBOX_WDDM_WIN8
            if (!g_VBoxDisplayOnly)
#endif
            {
            /* no need for DXGK_QUERYSEGMENTIN as it contains AGP aperture info, which (AGP aperture) we do not support
             * DXGK_QUERYSEGMENTIN *pQsIn = (DXGK_QUERYSEGMENTIN*)pQueryAdapterInfo->pInputData; */
            DXGK_QUERYSEGMENTOUT *pQsOut = (DXGK_QUERYSEGMENTOUT*)pQueryAdapterInfo->pOutputData;
# define VBOXWDDM_SEGMENTS_COUNT 2
            if (!pQsOut->pSegmentDescriptor)
            {
                /* we are requested to provide the number of segments we support */
                pQsOut->NbSegment = VBOXWDDM_SEGMENTS_COUNT;
            }
            else if (pQsOut->NbSegment != VBOXWDDM_SEGMENTS_COUNT)
            {
                WARN(("NbSegment (%d) != 1", pQsOut->NbSegment));
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                DXGK_SEGMENTDESCRIPTOR* pDr = pQsOut->pSegmentDescriptor;
                /* we are requested to provide segment information */
                pDr->BaseAddress.QuadPart = 0;
                pDr->CpuTranslatedAddress = VBoxCommonFromDeviceExt(pDevExt)->phVRAM;
                /* make sure the size is page aligned */
                /** @todo need to setup VBVA buffers and adjust the mem size here */
                pDr->Size = vboxWddmVramCpuVisibleSegmentSize(pDevExt);
                pDr->NbOfBanks = 0;
                pDr->pBankRangeTable = 0;
                pDr->CommitLimit = pDr->Size;
                pDr->Flags.Value = 0;
                pDr->Flags.CpuVisible = 1;

                ++pDr;
                /* create cpu-invisible segment of the same size */
                pDr->BaseAddress.QuadPart = 0;
                pDr->CpuTranslatedAddress.QuadPart = 0;
                /* make sure the size is page aligned */
                /** @todo need to setup VBVA buffers and adjust the mem size here */
                pDr->Size = vboxWddmVramCpuInvisibleSegmentSize(pDevExt);
                pDr->NbOfBanks = 0;
                pDr->pBankRangeTable = 0;
                pDr->CommitLimit = pDr->Size;
                pDr->Flags.Value = 0;

                pQsOut->PagingBufferSegmentId = 0;
                pQsOut->PagingBufferSize = PAGE_SIZE;
                pQsOut->PagingBufferPrivateDataSize = PAGE_SIZE;
            }
            }
#ifdef VBOX_WDDM_WIN8
            else
            {
                WARN(("unsupported Type (%d)", pQueryAdapterInfo->Type));
                Status = STATUS_NOT_SUPPORTED;
            }
#endif

            break;
        }
        case DXGKQAITYPE_UMDRIVERPRIVATE:
#ifdef VBOX_WDDM_WIN8
            if (!g_VBoxDisplayOnly)
#endif
            {
                if (pQueryAdapterInfo->OutputDataSize == sizeof (VBOXWDDM_QI))
                {
                    VBOXWDDM_QI * pQi = (VBOXWDDM_QI*)pQueryAdapterInfo->pOutputData;
                    memset (pQi, 0, sizeof (VBOXWDDM_QI));
                    pQi->u32Version = VBOXVIDEOIF_VERSION;
#ifdef VBOX_WITH_CROGL
                    pQi->u32VBox3DCaps = VBoxMpCrGetHostCaps();
#endif
                    pQi->cInfos = VBoxCommonFromDeviceExt(pDevExt)->cDisplays;
#ifdef VBOX_WITH_VIDEOHWACCEL
                    for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
                    {
                        pQi->aInfos[i] = pDevExt->aSources[i].Vhwa.Settings;
                    }
#endif
                }
                else
                {
                    WARN(("incorrect buffer size %d, expected %d", pQueryAdapterInfo->OutputDataSize, sizeof (VBOXWDDM_QI)));
                    Status = STATUS_BUFFER_TOO_SMALL;
                }
            }
#ifdef VBOX_WDDM_WIN8
            else
            {
                WARN(("unsupported Type (%d)", pQueryAdapterInfo->Type));
                Status = STATUS_NOT_SUPPORTED;
            }
#endif
            break;
#ifdef VBOX_WDDM_WIN8
        case DXGKQAITYPE_QUERYSEGMENT3:
            LOGREL(("DXGKQAITYPE_QUERYSEGMENT3 treating as unsupported!"));
            Status = STATUS_NOT_SUPPORTED;
            break;
#endif
        default:
            WARN(("unsupported Type (%d)", pQueryAdapterInfo->Type));
            Status = STATUS_NOT_SUPPORTED;
            break;
    }
    LOGF(("LEAVE, context(0x%x), Status(0x%x)", hAdapter, Status));
    return Status;
}

/**
 * DxgkDdiCreateDevice
 */
NTSTATUS APIENTRY DxgkDdiCreateDevice(
    CONST HANDLE  hAdapter,
    DXGKARG_CREATEDEVICE*  pCreateDevice)
{
    /* DxgkDdiCreateDevice should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));
    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    vboxVDbgBreakFv();

    PVBOXWDDM_DEVICE pDevice = (PVBOXWDDM_DEVICE)vboxWddmMemAllocZero(sizeof (VBOXWDDM_DEVICE));
    if (!pDevice)
    {
        WARN(("vboxWddmMemAllocZero failed for WDDM device structure"));
        return STATUS_NO_MEMORY;
    }

    pDevice->pAdapter = pDevExt;
    pDevice->hDevice = pCreateDevice->hDevice;

    pCreateDevice->hDevice = pDevice;
    if (pCreateDevice->Flags.SystemDevice)
        pDevice->enmType = VBOXWDDM_DEVICE_TYPE_SYSTEM;

    pCreateDevice->pInfo = NULL;

    LOGF(("LEAVE, context(0x%x), Status(0x%x)", hAdapter, Status));

    return Status;
}

PVBOXWDDM_RESOURCE vboxWddmResourceCreate(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_RCINFO pRcInfo)
{
    RT_NOREF(pDevExt);
    PVBOXWDDM_RESOURCE pResource = (PVBOXWDDM_RESOURCE)vboxWddmMemAllocZero(RT_OFFSETOF(VBOXWDDM_RESOURCE, aAllocations[pRcInfo->cAllocInfos]));
    if (!pResource)
    {
        AssertFailed();
        return NULL;
    }
    pResource->cRefs = 1;
    pResource->cAllocations = pRcInfo->cAllocInfos;
    pResource->fFlags = pRcInfo->fFlags;
    pResource->RcDesc = pRcInfo->RcDesc;
    return pResource;
}

VOID vboxWddmResourceRetain(PVBOXWDDM_RESOURCE pResource)
{
    ASMAtomicIncU32(&pResource->cRefs);
}

static VOID vboxWddmResourceDestroy(PVBOXWDDM_RESOURCE pResource)
{
    vboxWddmMemFree(pResource);
}

VOID vboxWddmResourceWaitDereference(PVBOXWDDM_RESOURCE pResource)
{
    vboxWddmCounterU32Wait(&pResource->cRefs, 1);
}

VOID vboxWddmResourceRelease(PVBOXWDDM_RESOURCE pResource)
{
    uint32_t cRefs = ASMAtomicDecU32(&pResource->cRefs);
    Assert(cRefs < UINT32_MAX/2);
    if (!cRefs)
    {
        vboxWddmResourceDestroy(pResource);
    }
}

void vboxWddmAllocationDeleteFromResource(PVBOXWDDM_RESOURCE pResource, PVBOXWDDM_ALLOCATION pAllocation)
{
    Assert(pAllocation->pResource == pResource);
    if (pResource)
    {
        Assert(&pResource->aAllocations[pAllocation->iIndex] == pAllocation);
        vboxWddmResourceRelease(pResource);
    }
    else
    {
        vboxWddmMemFree(pAllocation);
    }
}

VOID vboxWddmAllocationCleanupAssignment(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_ALLOCATION pAllocation)
{
    switch (pAllocation->enmType)
    {
        case VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE:
        case VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC:
        {
            if (pAllocation->bAssigned)
            {
                /** @todo do we need to notify host? */
                vboxWddmAssignPrimary(&pDevExt->aSources[pAllocation->AllocData.SurfDesc.VidPnSourceId], NULL, pAllocation->AllocData.SurfDesc.VidPnSourceId);
            }
            break;
        }
        default:
            break;
    }
}

VOID vboxWddmAllocationCleanup(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_ALLOCATION pAllocation)
{
    switch (pAllocation->enmType)
    {
        case VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE:
        case VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC:
        {
#if 0
            if (pAllocation->enmType == VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC)
            {
                if (pAllocation->hSharedHandle)
                {
                    vboxShRcTreeRemove(pDevExt, pAllocation);
                }
            }
#endif
            break;
        }
        case VBOXWDDM_ALLOC_TYPE_UMD_HGSMI_BUFFER:
        {
            break;
        }
        default:
            break;
    }
#ifdef VBOX_WITH_CROGL
    PVBOXWDDM_SWAPCHAIN pSwapchain = vboxWddmSwapchainRetainByAlloc(pDevExt, pAllocation);
    if (pSwapchain)
    {
        vboxWddmSwapchainAllocRemove(pDevExt, pSwapchain, pAllocation);
        vboxWddmSwapchainRelease(pSwapchain);
    }
#endif
}

VOID vboxWddmAllocationDestroy(PVBOXWDDM_ALLOCATION pAllocation)
{
    PAGED_CODE();

    vboxWddmAllocationDeleteFromResource(pAllocation->pResource, pAllocation);
}

PVBOXWDDM_ALLOCATION vboxWddmAllocationCreateFromResource(PVBOXWDDM_RESOURCE pResource, uint32_t iIndex)
{
    PVBOXWDDM_ALLOCATION pAllocation = NULL;
    if (pResource)
    {
        Assert(iIndex < pResource->cAllocations);
        if (iIndex < pResource->cAllocations)
        {
            pAllocation = &pResource->aAllocations[iIndex];
            memset(pAllocation, 0, sizeof (VBOXWDDM_ALLOCATION));
        }
        vboxWddmResourceRetain(pResource);
    }
    else
        pAllocation = (PVBOXWDDM_ALLOCATION)vboxWddmMemAllocZero(sizeof (VBOXWDDM_ALLOCATION));

    if (pAllocation)
    {
        if (pResource)
        {
            pAllocation->pResource = pResource;
            pAllocation->iIndex = iIndex;
        }
    }

    return pAllocation;
}

NTSTATUS vboxWddmAllocationCreate(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_RESOURCE pResource, uint32_t iIndex, DXGK_ALLOCATIONINFO* pAllocationInfo)
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pAllocationInfo->PrivateDriverDataSize == sizeof (VBOXWDDM_ALLOCINFO));
    if (pAllocationInfo->PrivateDriverDataSize >= sizeof (VBOXWDDM_ALLOCINFO))
    {
        PVBOXWDDM_ALLOCINFO pAllocInfo = (PVBOXWDDM_ALLOCINFO)pAllocationInfo->pPrivateDriverData;
        PVBOXWDDM_ALLOCATION pAllocation = vboxWddmAllocationCreateFromResource(pResource, iIndex);
        Assert(pAllocation);
        if (pAllocation)
        {
            pAllocationInfo->pPrivateDriverData = NULL;
            pAllocationInfo->PrivateDriverDataSize = 0;
            pAllocationInfo->Alignment = 0;
            pAllocationInfo->PitchAlignedSize = 0;
            pAllocationInfo->HintedBank.Value = 0;
            pAllocationInfo->PreferredSegment.Value = 0;
            pAllocationInfo->SupportedReadSegmentSet = 1;
            pAllocationInfo->SupportedWriteSegmentSet = 1;
            pAllocationInfo->EvictionSegmentSet = 0;
            pAllocationInfo->MaximumRenamingListLength = 0;
            pAllocationInfo->hAllocation = pAllocation;
            pAllocationInfo->Flags.Value = 0;
            pAllocationInfo->pAllocationUsageHint = NULL;
            pAllocationInfo->AllocationPriority = D3DDDI_ALLOCATIONPRIORITY_NORMAL;

            pAllocation->enmType = pAllocInfo->enmType;
            pAllocation->AllocData.Addr.SegmentId = 0;
            pAllocation->AllocData.Addr.offVram = VBOXVIDEOOFFSET_VOID;
            pAllocation->bVisible = FALSE;
            pAllocation->bAssigned = FALSE;
            KeInitializeSpinLock(&pAllocation->OpenLock);
            InitializeListHead(&pAllocation->OpenList);

            switch (pAllocInfo->enmType)
            {
                case VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE:
                case VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC:
                case VBOXWDDM_ALLOC_TYPE_STD_SHADOWSURFACE:
                case VBOXWDDM_ALLOC_TYPE_STD_STAGINGSURFACE:
                {
                    pAllocation->fRcFlags = pAllocInfo->fFlags;
                    pAllocation->AllocData.SurfDesc = pAllocInfo->SurfDesc;
                    pAllocation->AllocData.hostID = pAllocInfo->hostID;

                    pAllocationInfo->Size = pAllocInfo->SurfDesc.cbSize;

                    switch (pAllocInfo->enmType)
                    {
                        case VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE:
                            break;
                        case VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC:
#ifdef VBOX_WITH_VIDEOHWACCEL
                            if (pAllocInfo->fFlags.Overlay)
                            {
                                /* actually we can not "properly" issue create overlay commands to the host here
                                 * because we do not know source VidPn id here, i.e.
                                 * the primary which is supposed to be overlayed,
                                 * however we need to get some info like pitch & size from the host here */
                                int rc = vboxVhwaHlpGetSurfInfo(pDevExt, pAllocation);
                                AssertRC(rc);
                                if (RT_SUCCESS(rc))
                                {
                                    pAllocationInfo->Flags.Overlay = 1;
                                    pAllocationInfo->Flags.CpuVisible = 1;
                                    pAllocationInfo->Size = pAllocation->AllocData.SurfDesc.cbSize;

                                    pAllocationInfo->AllocationPriority = D3DDDI_ALLOCATIONPRIORITY_HIGH;
                                }
                                else
                                    Status = STATUS_UNSUCCESSFUL;
                            }
                            else
#endif
                            {
                                Assert(pAllocation->AllocData.SurfDesc.bpp);
                                Assert(pAllocation->AllocData.SurfDesc.pitch);
                                Assert(pAllocation->AllocData.SurfDesc.cbSize);

                                /*
                                 * Mark the allocation as visible to the CPU so we can
                                 * lock it in the user mode driver for SYSTEM pool allocations.
                                 * See @bugref{8040} for further information.
                                 */
                                if (!pAllocInfo->fFlags.SharedResource && !pAllocInfo->hostID)
                                    pAllocationInfo->Flags.CpuVisible = 1;

                                if (pAllocInfo->fFlags.SharedResource)
                                {
                                    pAllocation->hSharedHandle = (HANDLE)pAllocInfo->hSharedHandle;
#if 0
                                    if (pAllocation->hSharedHandle)
                                    {
                                        vboxShRcTreePut(pDevExt, pAllocation);
                                    }
#endif
                                }

#if 0
                                /* Allocation from the CPU invisible second segment does not
                                 * work apparently and actually fails on Vista.
                                 *
                                 * @todo Find out what exactly is wrong.
                                 */
//                                if (pAllocInfo->hostID)
                                {
                                    pAllocationInfo->SupportedReadSegmentSet = 2;
                                    pAllocationInfo->SupportedWriteSegmentSet = 2;
                                }
#endif
                            }
                            break;
                        case VBOXWDDM_ALLOC_TYPE_STD_SHADOWSURFACE:
                        case VBOXWDDM_ALLOC_TYPE_STD_STAGINGSURFACE:
                            pAllocationInfo->Flags.CpuVisible = 1;
                            break;
                        default: AssertFailedBreak(); /* Shut up MSC.*/
                    }

                    if (Status == STATUS_SUCCESS)
                    {
                        pAllocation->UsageHint.Version = 0;
                        pAllocation->UsageHint.v1.Flags.Value = 0;
                        pAllocation->UsageHint.v1.Format = pAllocInfo->SurfDesc.format;
                        pAllocation->UsageHint.v1.SwizzledFormat = 0;
                        pAllocation->UsageHint.v1.ByteOffset = 0;
                        pAllocation->UsageHint.v1.Width = pAllocation->AllocData.SurfDesc.width;
                        pAllocation->UsageHint.v1.Height = pAllocation->AllocData.SurfDesc.height;
                        pAllocation->UsageHint.v1.Pitch = pAllocation->AllocData.SurfDesc.pitch;
                        pAllocation->UsageHint.v1.Depth = 0;
                        pAllocation->UsageHint.v1.SlicePitch = 0;

                        Assert(!pAllocationInfo->pAllocationUsageHint);
                        pAllocationInfo->pAllocationUsageHint = &pAllocation->UsageHint;
                    }

                    break;
                }
                case VBOXWDDM_ALLOC_TYPE_UMD_HGSMI_BUFFER:
                {
                    pAllocationInfo->Size = pAllocInfo->cbBuffer;
                    pAllocation->fUhgsmiType = pAllocInfo->fUhgsmiType;
                    pAllocation->AllocData.SurfDesc.cbSize = pAllocInfo->cbBuffer;
                    pAllocationInfo->Flags.CpuVisible = 1;
//                    pAllocationInfo->Flags.SynchronousPaging = 1;
                    pAllocationInfo->AllocationPriority = D3DDDI_ALLOCATIONPRIORITY_MAXIMUM;
                    break;
                }

                default:
                    LOGREL(("ERROR: invalid alloc info type(%d)", pAllocInfo->enmType));
                    AssertBreakpoint();
                    Status = STATUS_INVALID_PARAMETER;
                    break;

            }

            if (Status != STATUS_SUCCESS)
                vboxWddmAllocationDeleteFromResource(pResource, pAllocation);
        }
        else
        {
            LOGREL(("ERROR: failed to create allocation description"));
            Status = STATUS_NO_MEMORY;
        }

    }
    else
    {
        LOGREL(("ERROR: PrivateDriverDataSize(%d) less than header size(%d)", pAllocationInfo->PrivateDriverDataSize, sizeof (VBOXWDDM_ALLOCINFO)));
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS APIENTRY DxgkDdiCreateAllocation(
    CONST HANDLE  hAdapter,
    DXGKARG_CREATEALLOCATION*  pCreateAllocation)
{
    /* DxgkDdiCreateAllocation should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_RESOURCE pResource = NULL;

    if (pCreateAllocation->PrivateDriverDataSize)
    {
        Assert(pCreateAllocation->PrivateDriverDataSize == sizeof (VBOXWDDM_RCINFO));
        Assert(pCreateAllocation->pPrivateDriverData);
        if (pCreateAllocation->PrivateDriverDataSize < sizeof (VBOXWDDM_RCINFO))
        {
            WARN(("invalid private data size (%d)", pCreateAllocation->PrivateDriverDataSize));
            return STATUS_INVALID_PARAMETER;
        }

        PVBOXWDDM_RCINFO pRcInfo = (PVBOXWDDM_RCINFO)pCreateAllocation->pPrivateDriverData;
//            Assert(pRcInfo->RcDesc.VidPnSourceId < VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
        if (pRcInfo->cAllocInfos != pCreateAllocation->NumAllocations)
        {
            WARN(("invalid number of allocations passed in, (%d), expected (%d)", pRcInfo->cAllocInfos, pCreateAllocation->NumAllocations));
            return STATUS_INVALID_PARAMETER;
        }

        /* a check to ensure we do not get the allocation size which is too big to overflow the 32bit value */
        if (VBOXWDDM_TRAILARRAY_MAXELEMENTSU32(VBOXWDDM_RESOURCE, aAllocations) < pRcInfo->cAllocInfos)
        {
            WARN(("number of allocations passed too big (%d), max is (%d)", pRcInfo->cAllocInfos, VBOXWDDM_TRAILARRAY_MAXELEMENTSU32(VBOXWDDM_RESOURCE, aAllocations)));
            return STATUS_INVALID_PARAMETER;
        }

        pResource = (PVBOXWDDM_RESOURCE)vboxWddmMemAllocZero(RT_OFFSETOF(VBOXWDDM_RESOURCE, aAllocations[pRcInfo->cAllocInfos]));
        if (!pResource)
        {
            WARN(("vboxWddmMemAllocZero failed for (%d) allocations", pRcInfo->cAllocInfos));
            return STATUS_NO_MEMORY;
        }

        pResource->cRefs = 1;
        pResource->cAllocations = pRcInfo->cAllocInfos;
        pResource->fFlags = pRcInfo->fFlags;
        pResource->RcDesc = pRcInfo->RcDesc;
    }


    for (UINT i = 0; i < pCreateAllocation->NumAllocations; ++i)
    {
        Status = vboxWddmAllocationCreate(pDevExt, pResource, i, &pCreateAllocation->pAllocationInfo[i]);
        if (Status != STATUS_SUCCESS)
        {
            WARN(("vboxWddmAllocationCreate(%d) failed, Status(0x%x)", i, Status));
            /* note: i-th allocation is expected to be cleared in a fail handling code above */
            for (UINT j = 0; j < i; ++j)
            {
                PVBOXWDDM_ALLOCATION pAllocation = (PVBOXWDDM_ALLOCATION)pCreateAllocation->pAllocationInfo[j].hAllocation;
                vboxWddmAllocationCleanup(pDevExt, pAllocation);
                vboxWddmAllocationDestroy(pAllocation);
            }
            break;
        }
    }

    if (Status == STATUS_SUCCESS)
    {
        pCreateAllocation->hResource = pResource;
    }
    else
    {
        if (pResource)
            vboxWddmResourceRelease(pResource);
    }

    LOGF(("LEAVE, status(0x%x), context(0x%x)", Status, hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiDestroyAllocation(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_DESTROYALLOCATION*  pDestroyAllocation)
{
    /* DxgkDdiDestroyAllocation should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    NTSTATUS Status = STATUS_SUCCESS;

    PVBOXWDDM_RESOURCE pRc = (PVBOXWDDM_RESOURCE)pDestroyAllocation->hResource;
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    if (pRc)
    {
        Assert(pRc->cAllocations == pDestroyAllocation->NumAllocations);
    }

    for (UINT i = 0; i < pDestroyAllocation->NumAllocations; ++i)
    {
        PVBOXWDDM_ALLOCATION pAlloc = (PVBOXWDDM_ALLOCATION)pDestroyAllocation->pAllocationList[i];
        Assert(pAlloc->pResource == pRc);
        vboxWddmAllocationCleanupAssignment(pDevExt, pAlloc);
        /* wait for all current allocation-related ops are completed */
        vboxWddmAllocationCleanup(pDevExt, pAlloc);
        if (pAlloc->hSharedHandle && pAlloc->AllocData.hostID)
            VBoxVdmaChromiumParameteriCRSubmit(pDevExt, GL_PIN_TEXTURE_CLEAR_CR, pAlloc->AllocData.hostID);
        vboxWddmAllocationDestroy(pAlloc);
    }

    if (pRc)
    {
        /* wait for all current resource-related ops are completed */
        vboxWddmResourceWaitDereference(pRc);
        vboxWddmResourceRelease(pRc);
    }

    LOGF(("LEAVE, status(0x%x), context(0x%x)", Status, hAdapter));

    return Status;
}

/**
 * DxgkDdiDescribeAllocation
 */
NTSTATUS
APIENTRY
DxgkDdiDescribeAllocation(
    CONST HANDLE  hAdapter,
    DXGKARG_DESCRIBEALLOCATION*  pDescribeAllocation)
{
    RT_NOREF(hAdapter);
//    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXWDDM_ALLOCATION pAllocation = (PVBOXWDDM_ALLOCATION)pDescribeAllocation->hAllocation;
    pDescribeAllocation->Width = pAllocation->AllocData.SurfDesc.width;
    pDescribeAllocation->Height = pAllocation->AllocData.SurfDesc.height;
    pDescribeAllocation->Format = pAllocation->AllocData.SurfDesc.format;
    memset (&pDescribeAllocation->MultisampleMethod, 0, sizeof (pDescribeAllocation->MultisampleMethod));
    pDescribeAllocation->RefreshRate.Numerator = 60000;
    pDescribeAllocation->RefreshRate.Denominator = 1000;
    pDescribeAllocation->PrivateDriverFormatAttribute = 0;

//    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

/**
 * DxgkDdiGetStandardAllocationDriverData
 */
NTSTATUS
APIENTRY
DxgkDdiGetStandardAllocationDriverData(
    CONST HANDLE  hAdapter,
    DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA*  pGetStandardAllocationDriverData)
{
    RT_NOREF(hAdapter);
    /* DxgkDdiGetStandardAllocationDriverData should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_ALLOCINFO pAllocInfo = NULL;

    switch (pGetStandardAllocationDriverData->StandardAllocationType)
    {
        case D3DKMDT_STANDARDALLOCATION_SHAREDPRIMARYSURFACE:
        {
            LOGF(("D3DKMDT_STANDARDALLOCATION_SHAREDPRIMARYSURFACE"));
            if(pGetStandardAllocationDriverData->pAllocationPrivateDriverData)
            {
                pAllocInfo = (PVBOXWDDM_ALLOCINFO)pGetStandardAllocationDriverData->pAllocationPrivateDriverData;
                memset (pAllocInfo, 0, sizeof (VBOXWDDM_ALLOCINFO));
                pAllocInfo->enmType = VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE;
                pAllocInfo->SurfDesc.width = pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData->Width;
                pAllocInfo->SurfDesc.height = pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData->Height;
                pAllocInfo->SurfDesc.format = pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData->Format;
                pAllocInfo->SurfDesc.bpp = vboxWddmCalcBitsPerPixel(pAllocInfo->SurfDesc.format);
                pAllocInfo->SurfDesc.pitch = vboxWddmCalcPitch(pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData->Width, pAllocInfo->SurfDesc.format);
                pAllocInfo->SurfDesc.cbSize = vboxWddmCalcSize(pAllocInfo->SurfDesc.pitch, pAllocInfo->SurfDesc.height, pAllocInfo->SurfDesc.format);
                pAllocInfo->SurfDesc.depth = 0;
                pAllocInfo->SurfDesc.slicePitch = 0;
                pAllocInfo->SurfDesc.RefreshRate = pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData->RefreshRate;
                pAllocInfo->SurfDesc.VidPnSourceId = pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData->VidPnSourceId;
            }
            pGetStandardAllocationDriverData->AllocationPrivateDriverDataSize = sizeof (VBOXWDDM_ALLOCINFO);

            pGetStandardAllocationDriverData->ResourcePrivateDriverDataSize = 0;
            break;
        }
        case D3DKMDT_STANDARDALLOCATION_SHADOWSURFACE:
        {
            LOGF(("D3DKMDT_STANDARDALLOCATION_SHADOWSURFACE"));
            UINT bpp = vboxWddmCalcBitsPerPixel(pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Format);
            Assert(bpp);
            if (bpp != 0)
            {
                UINT Pitch = vboxWddmCalcPitch(pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Width, pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Format);
                pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Pitch = Pitch;

                /** @todo need [d/q]word align?? */

                if (pGetStandardAllocationDriverData->pAllocationPrivateDriverData)
                {
                    pAllocInfo = (PVBOXWDDM_ALLOCINFO)pGetStandardAllocationDriverData->pAllocationPrivateDriverData;
                    pAllocInfo->enmType = VBOXWDDM_ALLOC_TYPE_STD_SHADOWSURFACE;
                    pAllocInfo->SurfDesc.width = pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Width;
                    pAllocInfo->SurfDesc.height = pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Height;
                    pAllocInfo->SurfDesc.format = pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Format;
                    pAllocInfo->SurfDesc.bpp = vboxWddmCalcBitsPerPixel(pAllocInfo->SurfDesc.format);
                    pAllocInfo->SurfDesc.pitch = vboxWddmCalcPitch(pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Width, pAllocInfo->SurfDesc.format);
                    pAllocInfo->SurfDesc.cbSize = vboxWddmCalcSize(pAllocInfo->SurfDesc.pitch, pAllocInfo->SurfDesc.height, pAllocInfo->SurfDesc.format);
                    pAllocInfo->SurfDesc.depth = 0;
                    pAllocInfo->SurfDesc.slicePitch = 0;
                    pAllocInfo->SurfDesc.RefreshRate.Numerator = 0;
                    pAllocInfo->SurfDesc.RefreshRate.Denominator = 1000;
                    pAllocInfo->SurfDesc.VidPnSourceId = D3DDDI_ID_UNINITIALIZED;

                    pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Pitch = pAllocInfo->SurfDesc.pitch;
                }
                pGetStandardAllocationDriverData->AllocationPrivateDriverDataSize = sizeof (VBOXWDDM_ALLOCINFO);

                pGetStandardAllocationDriverData->ResourcePrivateDriverDataSize = 0;
            }
            else
            {
                LOGREL(("Invalid format (%d)", pGetStandardAllocationDriverData->pCreateShadowSurfaceData->Format));
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }
        case D3DKMDT_STANDARDALLOCATION_STAGINGSURFACE:
        {
            LOGF(("D3DKMDT_STANDARDALLOCATION_STAGINGSURFACE"));
            if(pGetStandardAllocationDriverData->pAllocationPrivateDriverData)
            {
                pAllocInfo = (PVBOXWDDM_ALLOCINFO)pGetStandardAllocationDriverData->pAllocationPrivateDriverData;
                pAllocInfo->enmType = VBOXWDDM_ALLOC_TYPE_STD_STAGINGSURFACE;
                pAllocInfo->SurfDesc.width = pGetStandardAllocationDriverData->pCreateStagingSurfaceData->Width;
                pAllocInfo->SurfDesc.height = pGetStandardAllocationDriverData->pCreateStagingSurfaceData->Height;
                pAllocInfo->SurfDesc.format = D3DDDIFMT_X8R8G8B8; /* staging has always always D3DDDIFMT_X8R8G8B8 */
                pAllocInfo->SurfDesc.bpp = vboxWddmCalcBitsPerPixel(pAllocInfo->SurfDesc.format);
                pAllocInfo->SurfDesc.pitch = vboxWddmCalcPitch(pGetStandardAllocationDriverData->pCreateStagingSurfaceData->Width, pAllocInfo->SurfDesc.format);
                pAllocInfo->SurfDesc.cbSize = vboxWddmCalcSize(pAllocInfo->SurfDesc.pitch, pAllocInfo->SurfDesc.height, pAllocInfo->SurfDesc.format);
                pAllocInfo->SurfDesc.depth = 0;
                pAllocInfo->SurfDesc.slicePitch = 0;
                pAllocInfo->SurfDesc.RefreshRate.Numerator = 0;
                pAllocInfo->SurfDesc.RefreshRate.Denominator = 1000;
                pAllocInfo->SurfDesc.VidPnSourceId = D3DDDI_ID_UNINITIALIZED;

                pGetStandardAllocationDriverData->pCreateStagingSurfaceData->Pitch = pAllocInfo->SurfDesc.pitch;
            }
            pGetStandardAllocationDriverData->AllocationPrivateDriverDataSize = sizeof (VBOXWDDM_ALLOCINFO);

            pGetStandardAllocationDriverData->ResourcePrivateDriverDataSize = 0;
            break;
        }
//#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
//        case D3DKMDT_STANDARDALLOCATION_GDISURFACE:
//# error port to Win7 DDI
//              break;
//#endif
        default:
            LOGREL(("Invalid allocation type (%d)", pGetStandardAllocationDriverData->StandardAllocationType));
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    LOGF(("LEAVE, status(0x%x), context(0x%x)", Status, hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiAcquireSwizzlingRange(
    CONST HANDLE  hAdapter,
    DXGKARG_ACQUIRESWIZZLINGRANGE*  pAcquireSwizzlingRange)
{
    RT_NOREF(hAdapter, pAcquireSwizzlingRange);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertBreakpoint();

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiReleaseSwizzlingRange(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_RELEASESWIZZLINGRANGE*  pReleaseSwizzlingRange)
{
    RT_NOREF(hAdapter, pReleaseSwizzlingRange);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertBreakpoint();

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

#ifdef VBOX_WITH_CROGL
static NTSTATUS
APIENTRY
DxgkDdiPatchNew(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_PATCH*  pPatch)
{
    RT_NOREF(hAdapter);
    /* DxgkDdiPatch should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    uint8_t * pPrivateBuf = (uint8_t*)((uint8_t*)pPatch->pDmaBufferPrivateData + pPatch->DmaBufferPrivateDataSubmissionStartOffset);
    UINT cbPatchBuff = pPatch->DmaBufferPrivateDataSubmissionEndOffset - pPatch->DmaBufferPrivateDataSubmissionStartOffset;

    for (UINT i = pPatch->PatchLocationListSubmissionStart; i < pPatch->PatchLocationListSubmissionLength; ++i)
    {
        const D3DDDI_PATCHLOCATIONLIST* pPatchList = &pPatch->pPatchLocationList[i];
        Assert(pPatchList->AllocationIndex < pPatch->AllocationListSize);
        const DXGK_ALLOCATIONLIST *pAllocationList = &pPatch->pAllocationList[pPatchList->AllocationIndex];
        if (!pAllocationList->SegmentId)
        {
            WARN(("no segment id specified"));
            continue;
        }

        if (pPatchList->PatchOffset == ~0UL)
        {
            /* this is a dummy patch request, ignore */
            continue;
        }

        if (pPatchList->PatchOffset >= cbPatchBuff)
        {
            WARN(("pPatchList->PatchOffset(%d) >= cbPatchBuff(%d)", pPatchList->PatchOffset, cbPatchBuff));
            return STATUS_INVALID_PARAMETER;
        }

        VBOXCMDVBVAOFFSET *poffVram = (VBOXCMDVBVAOFFSET*)(pPrivateBuf + pPatchList->PatchOffset);
        Assert(pAllocationList->SegmentId);
        Assert(!pAllocationList->PhysicalAddress.HighPart);
        Assert(!(pAllocationList->PhysicalAddress.QuadPart & 0xfffUL)); /* <- just a check to ensure allocation offset does not go here */
        *poffVram = pAllocationList->PhysicalAddress.LowPart + pPatchList->AllocationOffset;
    }

    return STATUS_SUCCESS;
}
#endif

static NTSTATUS
APIENTRY
DxgkDdiPatchLegacy(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_PATCH*  pPatch)
{
    RT_NOREF(hAdapter);
    /* DxgkDdiPatch should be made pageable. */
    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    /* Value == 2 is Present
     * Value == 4 is RedirectedPresent
     * we do not expect any other flags to be set here */
//    Assert(pPatch->Flags.Value == 2 || pPatch->Flags.Value == 4);
    if (pPatch->DmaBufferPrivateDataSubmissionEndOffset - pPatch->DmaBufferPrivateDataSubmissionStartOffset >= sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR))
    {
        Assert(pPatch->DmaBufferPrivateDataSize >= sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR));
        VBOXWDDM_DMA_PRIVATEDATA_BASEHDR *pPrivateDataBase = (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR*)((uint8_t*)pPatch->pDmaBufferPrivateData + pPatch->DmaBufferPrivateDataSubmissionStartOffset);
        switch (pPrivateDataBase->enmCmd)
        {
            case VBOXVDMACMD_TYPE_DMA_PRESENT_BLT:
            {
                PVBOXWDDM_DMA_PRIVATEDATA_BLT pBlt = (PVBOXWDDM_DMA_PRIVATEDATA_BLT)pPrivateDataBase;
                Assert(pPatch->PatchLocationListSubmissionLength == 2);
                const D3DDDI_PATCHLOCATIONLIST* pPatchList = &pPatch->pPatchLocationList[pPatch->PatchLocationListSubmissionStart];
                Assert(pPatchList->AllocationIndex == DXGK_PRESENT_SOURCE_INDEX);
                Assert(pPatchList->PatchOffset == 0);
                const DXGK_ALLOCATIONLIST *pSrcAllocationList = &pPatch->pAllocationList[pPatchList->AllocationIndex];
                Assert(pSrcAllocationList->SegmentId);
                pBlt->Blt.SrcAlloc.segmentIdAlloc = pSrcAllocationList->SegmentId;
                pBlt->Blt.SrcAlloc.offAlloc = (VBOXVIDEOOFFSET)pSrcAllocationList->PhysicalAddress.QuadPart;

                pPatchList = &pPatch->pPatchLocationList[pPatch->PatchLocationListSubmissionStart + 1];
                Assert(pPatchList->AllocationIndex == DXGK_PRESENT_DESTINATION_INDEX);
                Assert(pPatchList->PatchOffset == 4);
                const DXGK_ALLOCATIONLIST *pDstAllocationList = &pPatch->pAllocationList[pPatchList->AllocationIndex];
                Assert(pDstAllocationList->SegmentId);
                pBlt->Blt.DstAlloc.segmentIdAlloc = pDstAllocationList->SegmentId;
                pBlt->Blt.DstAlloc.offAlloc = (VBOXVIDEOOFFSET)pDstAllocationList->PhysicalAddress.QuadPart;
                break;
            }
            case VBOXVDMACMD_TYPE_DMA_PRESENT_FLIP:
            {
                PVBOXWDDM_DMA_PRIVATEDATA_FLIP pFlip = (PVBOXWDDM_DMA_PRIVATEDATA_FLIP)pPrivateDataBase;
                Assert(pPatch->PatchLocationListSubmissionLength == 1);
                const D3DDDI_PATCHLOCATIONLIST* pPatchList = &pPatch->pPatchLocationList[pPatch->PatchLocationListSubmissionStart];
                Assert(pPatchList->AllocationIndex == DXGK_PRESENT_SOURCE_INDEX);
                Assert(pPatchList->PatchOffset == 0);
                const DXGK_ALLOCATIONLIST *pSrcAllocationList = &pPatch->pAllocationList[pPatchList->AllocationIndex];
                Assert(pSrcAllocationList->SegmentId);
                pFlip->Flip.Alloc.segmentIdAlloc = pSrcAllocationList->SegmentId;
                pFlip->Flip.Alloc.offAlloc = (VBOXVIDEOOFFSET)pSrcAllocationList->PhysicalAddress.QuadPart;
                break;
            }
            case VBOXVDMACMD_TYPE_DMA_PRESENT_CLRFILL:
            {
                PVBOXWDDM_DMA_PRIVATEDATA_CLRFILL pCF = (PVBOXWDDM_DMA_PRIVATEDATA_CLRFILL)pPrivateDataBase;
                Assert(pPatch->PatchLocationListSubmissionLength == 1);
                const D3DDDI_PATCHLOCATIONLIST* pPatchList = &pPatch->pPatchLocationList[pPatch->PatchLocationListSubmissionStart];
                Assert(pPatchList->AllocationIndex == DXGK_PRESENT_DESTINATION_INDEX);
                Assert(pPatchList->PatchOffset == 0);
                const DXGK_ALLOCATIONLIST *pDstAllocationList = &pPatch->pAllocationList[pPatchList->AllocationIndex];
                Assert(pDstAllocationList->SegmentId);
                pCF->ClrFill.Alloc.segmentIdAlloc = pDstAllocationList->SegmentId;
                pCF->ClrFill.Alloc.offAlloc = (VBOXVIDEOOFFSET)pDstAllocationList->PhysicalAddress.QuadPart;
                break;
            }
            case VBOXVDMACMD_TYPE_DMA_NOP:
                break;
            case VBOXVDMACMD_TYPE_CHROMIUM_CMD:
            {
                uint8_t * pPrivateBuf = (uint8_t*)pPrivateDataBase;
                for (UINT i = pPatch->PatchLocationListSubmissionStart; i < pPatch->PatchLocationListSubmissionLength; ++i)
                {
                    const D3DDDI_PATCHLOCATIONLIST* pPatchList = &pPatch->pPatchLocationList[i];
                    Assert(pPatchList->AllocationIndex < pPatch->AllocationListSize);
                    const DXGK_ALLOCATIONLIST *pAllocationList = &pPatch->pAllocationList[pPatchList->AllocationIndex];
                    Assert(pAllocationList->SegmentId);
                    if (pAllocationList->SegmentId)
                    {
                        DXGK_ALLOCATIONLIST *pAllocation2Patch = (DXGK_ALLOCATIONLIST*)(pPrivateBuf + pPatchList->PatchOffset);
                        pAllocation2Patch->SegmentId = pAllocationList->SegmentId;
                        pAllocation2Patch->PhysicalAddress.QuadPart = pAllocationList->PhysicalAddress.QuadPart + pPatchList->AllocationOffset;
                        Assert(!(pAllocationList->PhysicalAddress.QuadPart & 0xfffUL)); /* <- just a check to ensure allocation offset does not go here */
                    }
                }
                break;
            }
            default:
            {
                AssertBreakpoint();
                uint8_t *pBuf = ((uint8_t *)pPatch->pDmaBuffer) + pPatch->DmaBufferSubmissionStartOffset;
                for (UINT i = pPatch->PatchLocationListSubmissionStart; i < pPatch->PatchLocationListSubmissionLength; ++i)
                {
                    const D3DDDI_PATCHLOCATIONLIST* pPatchList = &pPatch->pPatchLocationList[i];
                    Assert(pPatchList->AllocationIndex < pPatch->AllocationListSize);
                    const DXGK_ALLOCATIONLIST *pAllocationList = &pPatch->pAllocationList[pPatchList->AllocationIndex];
                    if (pAllocationList->SegmentId)
                    {
                        Assert(pPatchList->PatchOffset < (pPatch->DmaBufferSubmissionEndOffset - pPatch->DmaBufferSubmissionStartOffset));
                        *((VBOXVIDEOOFFSET*)(pBuf+pPatchList->PatchOffset)) = (VBOXVIDEOOFFSET)pAllocationList->PhysicalAddress.QuadPart;
                    }
                    else
                    {
                        /* sanity */
                        if (pPatch->Flags.Value == 2 || pPatch->Flags.Value == 4)
                            Assert(i == 0);
                    }
                }
                break;
            }
        }
    }
    else if (pPatch->DmaBufferPrivateDataSubmissionEndOffset == pPatch->DmaBufferPrivateDataSubmissionStartOffset)
    {
        /* this is a NOP, just return success */
//        LOG(("null data size, treating as NOP"));
        return STATUS_SUCCESS;
    }
    else
    {
        WARN(("DmaBufferPrivateDataSubmissionEndOffset (%d) - DmaBufferPrivateDataSubmissionStartOffset (%d) < sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR) (%d)",
                pPatch->DmaBufferPrivateDataSubmissionEndOffset,
                pPatch->DmaBufferPrivateDataSubmissionStartOffset,
                sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR)));
        return STATUS_INVALID_PARAMETER;
    }

    LOGF(("LEAVE, context(0x%x)", hAdapter));

    return Status;
}

typedef struct VBOXWDDM_CALL_ISR
{
    PVBOXMP_DEVEXT pDevExt;
    ULONG MessageNumber;
} VBOXWDDM_CALL_ISR, *PVBOXWDDM_CALL_ISR;

static BOOLEAN vboxWddmCallIsrCb(PVOID Context)
{
    PVBOXWDDM_CALL_ISR pdc = (PVBOXWDDM_CALL_ISR)Context;
    PVBOXMP_DEVEXT pDevExt = pdc->pDevExt;
#ifdef VBOX_WITH_CROGL
    if (pDevExt->fCmdVbvaEnabled)
        return DxgkDdiInterruptRoutineNew(pDevExt, pdc->MessageNumber);
#endif
    return DxgkDdiInterruptRoutineLegacy(pDevExt, pdc->MessageNumber);
}

NTSTATUS vboxWddmCallIsr(PVBOXMP_DEVEXT pDevExt)
{
    VBOXWDDM_CALL_ISR context;
    context.pDevExt = pDevExt;
    context.MessageNumber = 0;
    BOOLEAN bRet;
    NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbSynchronizeExecution(
            pDevExt->u.primary.DxgkInterface.DeviceHandle,
            vboxWddmCallIsrCb,
            &context,
            0, /* IN ULONG MessageNumber */
            &bRet);
    AssertNtStatusSuccess(Status);
    return Status;
}

#ifdef VBOX_WITH_CRHGSMI
DECLCALLBACK(VOID) vboxWddmDmaCompleteChromiumCmd(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, PVOID pvContext)
{
    RT_NOREF(pCmd);
    PVBOXVDMACBUF_DR pDr = (PVBOXVDMACBUF_DR)pvContext;
    vboxVdmaCBufDrFree(&pDevExt->u.primary.Vdma, pDr);
}
#endif

#ifdef VBOX_WITH_CROGL
static NTSTATUS
APIENTRY
DxgkDdiSubmitCommandNew(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_SUBMITCOMMAND*  pSubmitCommand)
{
    /* DxgkDdiSubmitCommand runs at dispatch, should not be pageable. */

//    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
#ifdef VBOX_STRICT
    PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pSubmitCommand->hContext;
    Assert(pContext);
    Assert(pContext->pDevice);
    Assert(pContext->pDevice->pAdapter == pDevExt);
    Assert(!pSubmitCommand->DmaBufferSegmentId);
#endif

    uint32_t cbCmd = pSubmitCommand->DmaBufferPrivateDataSubmissionEndOffset - pSubmitCommand->DmaBufferPrivateDataSubmissionStartOffset;
    uint32_t cbDma = pSubmitCommand->DmaBufferSubmissionEndOffset - pSubmitCommand->DmaBufferSubmissionStartOffset;
    VBOXCMDVBVA_HDR *pHdr;
    VBOXCMDVBVA_HDR NopCmd;
    uint32_t cbCurCmd, cbCurDma;
    if (cbCmd < sizeof (VBOXCMDVBVA_HDR))
    {
        if (cbCmd || cbDma)
        {
            WARN(("invalid command data"));
            return STATUS_INVALID_PARAMETER;
        }
        Assert(!cbDma);
        NopCmd.u8OpCode = VBOXCMDVBVA_OPTYPE_NOPCMD;
        NopCmd.u8Flags = 0;
        NopCmd.u8State = VBOXCMDVBVA_STATE_SUBMITTED;
        NopCmd.u2.complexCmdEl.u16CbCmdHost = sizeof (VBOXCMDVBVA_HDR);
        NopCmd.u2.complexCmdEl.u16CbCmdGuest = 0;
        cbCmd = sizeof (VBOXCMDVBVA_HDR);
        pHdr = &NopCmd;
        cbCurCmd = sizeof (VBOXCMDVBVA_HDR);
        cbCurDma = 0;
    }
    else
    {
        pHdr = (VBOXCMDVBVA_HDR*)(((uint8_t*)pSubmitCommand->pDmaBufferPrivateData) + pSubmitCommand->DmaBufferPrivateDataSubmissionStartOffset);
        cbCurCmd = pHdr->u2.complexCmdEl.u16CbCmdHost;
        cbCurDma = pHdr->u2.complexCmdEl.u16CbCmdGuest;
    }


    VBOXCMDVBVA_HDR *pDstHdr, *pCurDstCmd;
    if (cbCmd != cbCurCmd || cbCurDma != cbDma)
    {
        if (cbCmd < cbCurCmd || cbDma < cbCurDma)
        {
            WARN(("incorrect buffer size"));
            return STATUS_INVALID_PARAMETER;
        }

        pDstHdr = VBoxCmdVbvaSubmitLock(pDevExt, &pDevExt->CmdVbva, cbCmd + sizeof (VBOXCMDVBVA_HDR));
        if (!pDstHdr)
        {
            WARN(("VBoxCmdVbvaSubmitLock failed"));
            return STATUS_UNSUCCESSFUL;
        }

        pDstHdr->u8OpCode = VBOXCMDVBVA_OPTYPE_COMPLEXCMD;
        pDstHdr->u8Flags = 0;
        pDstHdr->u.u8PrimaryID = 0;

        pCurDstCmd = pDstHdr + 1;
    }
    else
    {
        pDstHdr = VBoxCmdVbvaSubmitLock(pDevExt, &pDevExt->CmdVbva, cbCmd);
        if (!pDstHdr)
        {
            WARN(("VBoxCmdVbvaSubmitLock failed"));
            return STATUS_UNSUCCESSFUL;
        }
        pCurDstCmd = pDstHdr;
    }

    PHYSICAL_ADDRESS phAddr;
    phAddr.QuadPart = pSubmitCommand->DmaBufferPhysicalAddress.QuadPart + pSubmitCommand->DmaBufferSubmissionStartOffset;
    NTSTATUS Status = STATUS_SUCCESS;
    for (;;)
    {
        switch (pHdr->u8OpCode)
        {
            case VBOXCMDVBVA_OPTYPE_SYSMEMCMD:
            {
                VBOXCMDVBVA_SYSMEMCMD *pSysMem = (VBOXCMDVBVA_SYSMEMCMD*)pHdr;
                if (pSubmitCommand->DmaBufferPhysicalAddress.QuadPart & PAGE_OFFSET_MASK)
                {
                    WARN(("command should be page aligned for now"));
                    return STATUS_INVALID_PARAMETER;
                }
                pSysMem->phCmd = (VBOXCMDVBVAPHADDR)(pSubmitCommand->DmaBufferPhysicalAddress.QuadPart + pSubmitCommand->DmaBufferSubmissionStartOffset);
#ifdef DEBUG
                {
                    uint32_t cbRealDmaCmd = (pSysMem->Hdr.u8Flags | (pSysMem->Hdr.u.u8PrimaryID << 8));
                    Assert(cbRealDmaCmd >= cbDma);
                    if (cbDma < cbRealDmaCmd)
                        WARN(("parrtial sysmem transfer"));
                }
#endif
                break;
            }
            default:
                break;
        }

        memcpy(pCurDstCmd, pHdr, cbCurCmd);
        pCurDstCmd->u2.complexCmdEl.u16CbCmdGuest = 0;

        phAddr.QuadPart += cbCurDma;
        pHdr = (VBOXCMDVBVA_HDR*)(((uint8_t*)pHdr) + cbCurCmd);
        pCurDstCmd = (VBOXCMDVBVA_HDR*)(((uint8_t*)pCurDstCmd) + cbCurCmd);
        cbCmd -= cbCurCmd;
        cbDma -= cbCurDma;
        if (!cbCmd)
        {
            if (cbDma)
            {
                WARN(("invalid param"));
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }

        if (cbCmd < sizeof (VBOXCMDVBVA_HDR))
        {
            WARN(("invalid param"));
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        cbCurCmd = pHdr->u2.complexCmdEl.u16CbCmdHost;
        cbCurDma = pHdr->u2.complexCmdEl.u16CbCmdGuest;

        if (cbCmd < cbCurCmd)
        {
            WARN(("invalid param"));
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (cbDma < cbCurDma)
        {
            WARN(("invalid param"));
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    uint32_t u32FenceId = pSubmitCommand->SubmissionFenceId;

    if (!NT_SUCCESS(Status))
    {
        /* nop the entire command on failure */
        pDstHdr->u8OpCode = VBOXCMDVBVA_OPTYPE_NOPCMD;
        pDstHdr->u8Flags = 0;
        pDstHdr->u.i8Result = 0;
        u32FenceId = 0;
    }

    VBoxCmdVbvaSubmitUnlock(pDevExt, &pDevExt->CmdVbva, pDstHdr, u32FenceId);

    return Status;
}
#endif

static NTSTATUS
APIENTRY
DxgkDdiSubmitCommandLegacy(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_SUBMITCOMMAND*  pSubmitCommand)
{
    /* DxgkDdiSubmitCommand runs at dispatch, should not be pageable. */
    NTSTATUS Status = STATUS_SUCCESS;

//    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pSubmitCommand->hContext;
    PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR pPrivateDataBase = NULL;
    VBOXVDMACMD_TYPE enmCmd = VBOXVDMACMD_TYPE_UNDEFINED;
    Assert(pContext);
    Assert(pContext->pDevice);
    Assert(pContext->pDevice->pAdapter == pDevExt);
    Assert(!pSubmitCommand->DmaBufferSegmentId);

    /* the DMA command buffer is located in system RAM, the host will need to pick it from there */
    //BufInfo.fFlags = 0; /* see VBOXVDMACBUF_FLAG_xx */
    if (pSubmitCommand->DmaBufferPrivateDataSubmissionEndOffset - pSubmitCommand->DmaBufferPrivateDataSubmissionStartOffset >= sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR))
    {
        pPrivateDataBase = (PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR)((uint8_t*)pSubmitCommand->pDmaBufferPrivateData + pSubmitCommand->DmaBufferPrivateDataSubmissionStartOffset);
        Assert(pPrivateDataBase);
        enmCmd = pPrivateDataBase->enmCmd;
    }
    else if (pSubmitCommand->DmaBufferPrivateDataSubmissionEndOffset == pSubmitCommand->DmaBufferPrivateDataSubmissionStartOffset)
    {
        enmCmd = VBOXVDMACMD_TYPE_DMA_NOP;
    }
    else
    {
        WARN(("DmaBufferPrivateDataSubmissionEndOffset (%d) - DmaBufferPrivateDataSubmissionStartOffset (%d) < sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR) (%d)",
                pSubmitCommand->DmaBufferPrivateDataSubmissionEndOffset,
                pSubmitCommand->DmaBufferPrivateDataSubmissionStartOffset,
                sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR)));
        return STATUS_INVALID_PARAMETER;
    }

    switch (enmCmd)
    {
        case VBOXVDMACMD_TYPE_DMA_PRESENT_BLT:
        {
            VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR *pPrivateData = (VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR*)pPrivateDataBase;
            PVBOXWDDM_DMA_PRIVATEDATA_BLT pBlt = (PVBOXWDDM_DMA_PRIVATEDATA_BLT)pPrivateData;
            PVBOXWDDM_ALLOCATION pDstAlloc = pBlt->Blt.DstAlloc.pAlloc;
            PVBOXWDDM_ALLOCATION pSrcAlloc = pBlt->Blt.SrcAlloc.pAlloc;
            BOOLEAN fSrcChanged;
            BOOLEAN fDstChanged;

            fDstChanged = vboxWddmAddrSetVram(&pDstAlloc->AllocData.Addr, pBlt->Blt.DstAlloc.segmentIdAlloc, pBlt->Blt.DstAlloc.offAlloc);
            fSrcChanged = vboxWddmAddrSetVram(&pSrcAlloc->AllocData.Addr, pBlt->Blt.SrcAlloc.segmentIdAlloc, pBlt->Blt.SrcAlloc.offAlloc);

            if (VBOXWDDM_IS_FB_ALLOCATION(pDevExt, pDstAlloc))
            {
                Assert(pDstAlloc->AllocData.SurfDesc.VidPnSourceId < VBOX_VIDEO_MAX_SCREENS);
#if 0
                VBOXWDDM_SOURCE *pSource = &pDevExt->aSources[pDstAlloc->AllocData.SurfDesc.VidPnSourceId];
                if (VBOXWDDM_IS_FB_ALLOCATION(pDevExt, pDstAlloc) && pDstAlloc->AllocData.hostID)
                {
                    if (pSource->AllocData.hostID != pDstAlloc->AllocData.hostID)
                    {
                        pSource->AllocData.hostID = pDstAlloc->AllocData.hostID;
                        fDstChanged = TRUE;
                    }

                    if (fDstChanged)
                        pSource->u8SyncState &= ~VBOXWDDM_HGSYNC_F_SYNCED_LOCATION;
                }
#endif
            }

            Status = vboxVdmaProcessBltCmd(pDevExt, pContext, pBlt);
            if (!NT_SUCCESS(Status))
                WARN(("vboxVdmaProcessBltCmd failed, Status 0x%x", Status));

            Status = vboxVdmaDdiCmdFenceComplete(pDevExt, pContext->NodeOrdinal, pSubmitCommand->SubmissionFenceId,
                    NT_SUCCESS(Status) ? DXGK_INTERRUPT_DMA_COMPLETED : DXGK_INTERRUPT_DMA_FAULTED);
            break;
        }
        case VBOXVDMACMD_TYPE_DMA_PRESENT_FLIP:
        {
            VBOXWDDM_DMA_PRIVATEDATA_FLIP *pFlip = (VBOXWDDM_DMA_PRIVATEDATA_FLIP*)pPrivateDataBase;
            PVBOXWDDM_ALLOCATION pAlloc = pFlip->Flip.Alloc.pAlloc;
            VBOXWDDM_SOURCE *pSource = &pDevExt->aSources[pAlloc->AllocData.SurfDesc.VidPnSourceId];
            vboxWddmAddrSetVram(&pAlloc->AllocData.Addr, pFlip->Flip.Alloc.segmentIdAlloc, pFlip->Flip.Alloc.offAlloc);
            vboxWddmAssignPrimary(pSource, pAlloc, pAlloc->AllocData.SurfDesc.VidPnSourceId);
            vboxWddmGhDisplayCheckSetInfoFromSource(pDevExt, pSource);

            Status = vboxVdmaDdiCmdFenceComplete(pDevExt, pContext->NodeOrdinal, pSubmitCommand->SubmissionFenceId,
                    NT_SUCCESS(Status) ? DXGK_INTERRUPT_DMA_COMPLETED : DXGK_INTERRUPT_DMA_FAULTED);
            break;
        }
        case VBOXVDMACMD_TYPE_DMA_PRESENT_CLRFILL:
        {
            PVBOXWDDM_DMA_PRIVATEDATA_CLRFILL pCF = (PVBOXWDDM_DMA_PRIVATEDATA_CLRFILL)pPrivateDataBase;
            vboxWddmAddrSetVram(&pCF->ClrFill.Alloc.pAlloc->AllocData.Addr, pCF->ClrFill.Alloc.segmentIdAlloc, pCF->ClrFill.Alloc.offAlloc);

            Status = vboxVdmaProcessClrFillCmd(pDevExt, pContext, pCF);
            if (!NT_SUCCESS(Status))
                WARN(("vboxVdmaProcessClrFillCmd failed, Status 0x%x", Status));

            Status = vboxVdmaDdiCmdFenceComplete(pDevExt, pContext->NodeOrdinal, pSubmitCommand->SubmissionFenceId,
                    NT_SUCCESS(Status) ? DXGK_INTERRUPT_DMA_COMPLETED : DXGK_INTERRUPT_DMA_FAULTED);
            break;
        }
        case VBOXVDMACMD_TYPE_DMA_NOP:
        {
            Status = vboxVdmaDdiCmdFenceComplete(pDevExt, pContext->NodeOrdinal, pSubmitCommand->SubmissionFenceId, DXGK_INTERRUPT_DMA_COMPLETED);
            AssertNtStatusSuccess(Status);
            break;
        }
        default:
        {
            WARN(("unexpected command %d", enmCmd));
#if 0 //def VBOX_WITH_VDMA
            VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR *pPrivateData = (VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR*)pPrivateDataBase;
            PVBOXVDMACBUF_DR pDr = vboxVdmaCBufDrCreate (&pDevExt->u.primary.Vdma, 0);
            if (!pDr)
            {
                /** @todo try flushing.. */
                LOGREL(("vboxVdmaCBufDrCreate returned NULL"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            // vboxVdmaCBufDrCreate zero initializes the pDr
            //pDr->fFlags = 0;
            pDr->cbBuf = pSubmitCommand->DmaBufferSubmissionEndOffset - pSubmitCommand->DmaBufferSubmissionStartOffset;
            pDr->u32FenceId = pSubmitCommand->SubmissionFenceId;
            pDr->rc = VERR_NOT_IMPLEMENTED;
            if (pPrivateData)
                pDr->u64GuestContext = (uint64_t)pPrivateData->pContext;
        //    else    // vboxVdmaCBufDrCreate zero initializes the pDr
        //        pDr->u64GuestContext = NULL;
            pDr->Location.phBuf = pSubmitCommand->DmaBufferPhysicalAddress.QuadPart + pSubmitCommand->DmaBufferSubmissionStartOffset;

            vboxVdmaCBufDrSubmit(pDevExt, &pDevExt->u.primary.Vdma, pDr);
#endif
            break;
        }
    }
//    LOGF(("LEAVE, context(0x%x)", hAdapter));

    return Status;
}

#ifdef VBOX_WITH_CROGL
static NTSTATUS
APIENTRY
DxgkDdiPreemptCommandNew(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_PREEMPTCOMMAND*  pPreemptCommand)
{
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    vboxVDbgBreakF();

    VBoxCmdVbvaPreempt(pDevExt, &pDevExt->CmdVbva, pPreemptCommand->PreemptionFenceId);

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}
#endif

static NTSTATUS
APIENTRY
DxgkDdiPreemptCommandLegacy(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_PREEMPTCOMMAND*  pPreemptCommand)
{
    RT_NOREF(hAdapter, pPreemptCommand);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertFailed();
    /** @todo fixme: implement */

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

#ifdef VBOX_WITH_CROGL
/*
 * DxgkDdiBuildPagingBuffer
 */
static NTSTATUS
APIENTRY
DxgkDdiBuildPagingBufferNew(
    CONST HANDLE  hAdapter,
    DXGKARG_BUILDPAGINGBUFFER*  pBuildPagingBuffer)
{
    RT_NOREF(hAdapter);
    /* DxgkDdiBuildPagingBuffer should be made pageable. */
    PAGED_CODE();

    vboxVDbgBreakFv();

    uint32_t cbBuffer = 0, cbPrivateData = 0;

    LOGF(("ENTER context(0x%X), operation(0x%X) MultipassOffset(0x%X) DmaSizes(0x%X 0x%X)",
        hAdapter, pBuildPagingBuffer->Operation, pBuildPagingBuffer->MultipassOffset,
        pBuildPagingBuffer->DmaSize, pBuildPagingBuffer->DmaBufferPrivateDataSize));

    /* Checking for bare minimum of DMA buffer sizes*/
    if (pBuildPagingBuffer->DmaBufferPrivateDataSize < sizeof (VBOXCMDVBVA_HDR))
    {
        WARN(("pBuildPagingBuffer->DmaBufferPrivateDataSize(%d) < sizeof (VBOXCMDVBVA_HDR)", pBuildPagingBuffer->DmaBufferPrivateDataSize));
        return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
    }

    if (pBuildPagingBuffer->DmaSize < VBOXWDDM_DUMMY_DMABUFFER_SIZE)
    {
        WARN(("pBuildPagingBuffer->DmaSize(%d) < VBOXWDDM_DUMMY_DMABUFFER_SIZE", pBuildPagingBuffer->DmaSize));
        return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
    }

    VBOXCMDVBVA_HDR *pHdr = (VBOXCMDVBVA_HDR*)pBuildPagingBuffer->pDmaBufferPrivateData;

    switch (pBuildPagingBuffer->Operation)
    {
        case DXGK_OPERATION_TRANSFER:
        {
#if 0
            if (!pBuildPagingBuffer->Transfer.Flags.AllocationIsIdle)
            {
                WARN(("allocation is not idle"));
                return STATUS_GRAPHICS_ALLOCATION_BUSY;
            }
#endif

            if (pBuildPagingBuffer->DmaBufferPrivateDataSize < sizeof (VBOXCMDVBVA_SYSMEMCMD))
            {
                WARN(("private data too small"));
                return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
            }

            Assert(!pBuildPagingBuffer->Transfer.MdlOffset);

            if ((!pBuildPagingBuffer->Transfer.Source.SegmentId) == (!pBuildPagingBuffer->Transfer.Destination.SegmentId))
            {
                WARN(("we only support RAM <-> VRAM moves, Src Seg(%d), Dst Seg(%d)", pBuildPagingBuffer->Transfer.Source.SegmentId, pBuildPagingBuffer->Transfer.Destination.SegmentId));
                return STATUS_INVALID_PARAMETER;
            }

            PVBOXWDDM_ALLOCATION pAlloc = (PVBOXWDDM_ALLOCATION)pBuildPagingBuffer->Transfer.hAllocation;
            if (!pAlloc)
            {
                WARN(("allocation is null"));
                return STATUS_INVALID_PARAMETER;
            }

            if (pAlloc->AllocData.hostID)
            {
                cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
                cbPrivateData = sizeof (*pHdr);

                pHdr->u8OpCode = VBOXCMDVBVA_OPTYPE_NOPCMD;
                pHdr->u8Flags = 0;
                pHdr->u.u8PrimaryID = 0;
                pHdr->u8State = VBOXCMDVBVA_STATE_SUBMITTED;
                break;
            }

            if (pBuildPagingBuffer->DmaSize < sizeof (VBOXCMDVBVA_PAGING_TRANSFER))
            {
                WARN(("pBuildPagingBuffer->DmaSize(%d) < sizeof VBOXCMDVBVA_PAGING_TRANSFER (%d)", pBuildPagingBuffer->DmaSize , sizeof (VBOXCMDVBVA_PAGING_TRANSFER)));
                /** @todo can this actually happen? what status to return? */
                return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
            }

            VBOXCMDVBVA_PAGING_TRANSFER *pPaging = (VBOXCMDVBVA_PAGING_TRANSFER*)pBuildPagingBuffer->pDmaBuffer;
            pPaging->Hdr.u8OpCode = VBOXCMDVBVA_OPTYPE_PAGING_TRANSFER;
            /* sanity */
            pPaging->Hdr.u8Flags = 0;
            pPaging->Hdr.u8State = VBOXCMDVBVA_STATE_SUBMITTED;

            PMDL pMdl;
            uint32_t offVRAM;
            BOOLEAN fIn;
            UINT SegmentId;

            if (pBuildPagingBuffer->Transfer.Source.SegmentId)
            {
                SegmentId = pBuildPagingBuffer->Transfer.Source.SegmentId;
                Assert(!pBuildPagingBuffer->Transfer.Destination.SegmentId);
                Assert(!pBuildPagingBuffer->Transfer.Source.SegmentAddress.HighPart);
                offVRAM = pBuildPagingBuffer->Transfer.Source.SegmentAddress.LowPart;
                pMdl = pBuildPagingBuffer->Transfer.Destination.pMdl;
                fIn = FALSE;
            }
            else
            {
                SegmentId = pBuildPagingBuffer->Transfer.Destination.SegmentId;
                Assert(pBuildPagingBuffer->Transfer.Destination.SegmentId);
                Assert(!pBuildPagingBuffer->Transfer.Source.SegmentId);
                Assert(!pBuildPagingBuffer->Transfer.Destination.SegmentAddress.HighPart);
                offVRAM = pBuildPagingBuffer->Transfer.Destination.SegmentAddress.LowPart;
                pMdl = pBuildPagingBuffer->Transfer.Source.pMdl;
                fIn = TRUE;
            }

            if (SegmentId != 1)
            {
                WARN(("SegmentId"));
                cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
                break;
            }

            Assert(!(pBuildPagingBuffer->Transfer.TransferSize & PAGE_OFFSET_MASK));
            Assert(!(offVRAM & PAGE_OFFSET_MASK));
            uint32_t cPages = (uint32_t)(pBuildPagingBuffer->Transfer.TransferSize >> PAGE_SHIFT);
            Assert(cPages > pBuildPagingBuffer->MultipassOffset);
            cPages -= pBuildPagingBuffer->MultipassOffset;
            uint32_t iFirstPage = pBuildPagingBuffer->MultipassOffset;
            uint32_t cPagesWritten;
            offVRAM += pBuildPagingBuffer->Transfer.TransferOffset + (pBuildPagingBuffer->MultipassOffset << PAGE_SHIFT);

            pPaging->Data.Alloc.u.offVRAM = offVRAM;
            if (fIn)
                pPaging->Hdr.u8Flags |= VBOXCMDVBVA_OPF_PAGING_TRANSFER_IN;
            cbBuffer = VBoxCVDdiPTransferVRamSysBuildEls(pPaging, pMdl, iFirstPage, cPages, pBuildPagingBuffer->DmaSize, &cPagesWritten);
            if (cPagesWritten != cPages)
                pBuildPagingBuffer->MultipassOffset += cPagesWritten;
            else
                pBuildPagingBuffer->MultipassOffset = 0;

            VBOXCMDVBVA_SYSMEMCMD *pSysMemCmd = (VBOXCMDVBVA_SYSMEMCMD*)pBuildPagingBuffer->pDmaBufferPrivateData;

            cbPrivateData = sizeof (*pSysMemCmd);

            pSysMemCmd->Hdr.u8OpCode = VBOXCMDVBVA_OPTYPE_SYSMEMCMD;
            pSysMemCmd->Hdr.u8Flags = cbBuffer & 0xff;
            pSysMemCmd->Hdr.u.u8PrimaryID = (cbBuffer >> 8) & 0xff;
            pSysMemCmd->Hdr.u8State = VBOXCMDVBVA_STATE_SUBMITTED;
            pSysMemCmd->phCmd = 0;

            break;
        }
        case DXGK_OPERATION_FILL:
        {
            Assert(pBuildPagingBuffer->Fill.FillPattern == 0);
            PVBOXWDDM_ALLOCATION pAlloc = (PVBOXWDDM_ALLOCATION)pBuildPagingBuffer->Fill.hAllocation;
            if (!pAlloc)
            {
                WARN(("allocation is null"));
                return STATUS_INVALID_PARAMETER;
            }

            if (pAlloc->AllocData.hostID || pBuildPagingBuffer->Fill.Destination.SegmentId != 1)
            {
                if (!pAlloc->AllocData.hostID)
                {
                    WARN(("unexpected segment id"));
                }

                cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
                cbPrivateData = sizeof (*pHdr);

                pHdr->u8OpCode = VBOXCMDVBVA_OPTYPE_NOPCMD;
                pHdr->u8Flags = 0;
                pHdr->u.u8PrimaryID = 0;
                pHdr->u8State = VBOXCMDVBVA_STATE_SUBMITTED;
                break;
            }

            if (pBuildPagingBuffer->DmaBufferPrivateDataSize < sizeof (VBOXCMDVBVA_PAGING_FILL))
            {
                WARN(("private data too small"));
                return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
            }

            VBOXCMDVBVA_PAGING_FILL *pFill = (VBOXCMDVBVA_PAGING_FILL*)pBuildPagingBuffer->pDmaBufferPrivateData;
            pFill->Hdr.u8OpCode = VBOXCMDVBVA_OPTYPE_PAGING_FILL;
            pFill->Hdr.u8Flags = 0;
            pFill->Hdr.u.u8PrimaryID = 0;
            pFill->Hdr.u8State = VBOXCMDVBVA_STATE_SUBMITTED;
            pFill->u32CbFill = (uint32_t)pBuildPagingBuffer->Fill.FillSize;
            pFill->u32Pattern = pBuildPagingBuffer->Fill.FillPattern;
            Assert(!pBuildPagingBuffer->Fill.Destination.SegmentAddress.HighPart);
            pFill->offVRAM = pBuildPagingBuffer->Fill.Destination.SegmentAddress.LowPart;

            cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
            cbPrivateData = sizeof (*pFill);

            break;
        }
        case DXGK_OPERATION_DISCARD_CONTENT:
        {
            PVBOXWDDM_ALLOCATION pAlloc = (PVBOXWDDM_ALLOCATION)pBuildPagingBuffer->DiscardContent.hAllocation;
            if (!pAlloc)
            {
                WARN(("allocation is null"));
                return STATUS_INVALID_PARAMETER;
            }
//            WARN(("Do we need to do anything here?"));
            break;
        }
        default:
        {
            WARN(("unsupported op (%d)", pBuildPagingBuffer->Operation));
            break;
        }
    }

    Assert(cbPrivateData >= sizeof (VBOXCMDVBVA_HDR) || pBuildPagingBuffer->Operation == DXGK_OPERATION_DISCARD_CONTENT);
    Assert(pBuildPagingBuffer->Operation == DXGK_OPERATION_DISCARD_CONTENT || cbBuffer);
    Assert(cbBuffer <= pBuildPagingBuffer->DmaSize);
    Assert(cbBuffer == 0 || cbBuffer >= sizeof (VBOXCMDVBVA_PAGING_TRANSFER) || cbBuffer == VBOXWDDM_DUMMY_DMABUFFER_SIZE);
    AssertCompile(VBOXWDDM_DUMMY_DMABUFFER_SIZE < 8);

    pHdr->u2.complexCmdEl.u16CbCmdHost = cbPrivateData;
    pHdr->u2.complexCmdEl.u16CbCmdGuest = cbBuffer;

    pBuildPagingBuffer->pDmaBuffer = ((uint8_t*)pBuildPagingBuffer->pDmaBuffer) + cbBuffer;
    pBuildPagingBuffer->pDmaBufferPrivateData = ((uint8_t*)pBuildPagingBuffer->pDmaBufferPrivateData) + cbPrivateData;

    LOGF(("LEAVE context(0x%X), MultipassOffset(0x%X) cbBuffer(0x%X) cbPrivateData(0x%X)",
        hAdapter, pBuildPagingBuffer->MultipassOffset, cbBuffer, cbPrivateData));

    if (pBuildPagingBuffer->MultipassOffset)
        return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
    return STATUS_SUCCESS;
}
#endif

static NTSTATUS
APIENTRY
DxgkDdiBuildPagingBufferLegacy(
    CONST HANDLE  hAdapter,
    DXGKARG_BUILDPAGINGBUFFER*  pBuildPagingBuffer)
{
    /* DxgkDdiBuildPagingBuffer should be made pageable. */
    PAGED_CODE();

    vboxVDbgBreakFv();

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    LOGF(("ENTER, context(0x%x)", hAdapter));

    uint32_t cbCmdDma = 0;

    /** @todo */
    switch (pBuildPagingBuffer->Operation)
    {
        case DXGK_OPERATION_TRANSFER:
        {
            cbCmdDma = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
#ifdef VBOX_WITH_VDMA
#if 0
            if ((!pBuildPagingBuffer->Transfer.Source.SegmentId) != (!pBuildPagingBuffer->Transfer.Destination.SegmentId))
            {
                PVBOXVDMACMD pCmd = (PVBOXVDMACMD)pBuildPagingBuffer->pDmaBuffer;
                pCmd->enmType = VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER_VRAMSYS;
                pCmd->u32CmdSpecific = 0;
                PVBOXVDMACMD_DMA_BPB_TRANSFER_VRAMSYS pBody = VBOXVDMACMD_BODY(pCmd, VBOXVDMACMD_DMA_BPB_TRANSFER_VRAMSYS);
                PMDL pMdl;
                uint32_t cPages = (pBuildPagingBuffer->Transfer.TransferSize + 0xfff) >> 12;
                cPages -= pBuildPagingBuffer->MultipassOffset;
                uint32_t iFirstPage = pBuildPagingBuffer->Transfer.MdlOffset + pBuildPagingBuffer->MultipassOffset;
                uint32_t cPagesRemaining;
                if (pBuildPagingBuffer->Transfer.Source.SegmentId)
                {
                    uint64_t off = pBuildPagingBuffer->Transfer.Source.SegmentAddress.QuadPart;
                    off += pBuildPagingBuffer->Transfer.TransferOffset + (pBuildPagingBuffer->MultipassOffset << PAGE_SHIFT);
                    pBody->offVramBuf = off;
                    pMdl = pBuildPagingBuffer->Transfer.Source.pMdl;
                    pBody->fFlags = 0;//VBOXVDMACMD_DMA_BPB_TRANSFER_VRAMSYS_SYS2VRAM
                }
                else
                {
                    uint64_t off = pBuildPagingBuffer->Transfer.Destination.SegmentAddress.QuadPart;
                    off += pBuildPagingBuffer->Transfer.TransferOffset + (pBuildPagingBuffer->MultipassOffset << PAGE_SHIFT);
                    pBody->offVramBuf = off;
                    pMdl = pBuildPagingBuffer->Transfer.Destination.pMdl;
                    pBody->fFlags = VBOXVDMACMD_DMA_BPB_TRANSFER_VRAMSYS_SYS2VRAM;
                }

                uint32_t sbBufferUsed = vboxWddmBpbTransferVRamSysBuildEls(pBody, pMdl, iFirstPage, cPages, pBuildPagingBuffer->DmaSize, &cPagesRemaining);
                Assert(sbBufferUsed);
            }

#else
            PVBOXWDDM_ALLOCATION pAlloc = (PVBOXWDDM_ALLOCATION)pBuildPagingBuffer->Transfer.hAllocation;
            Assert(pAlloc);
            if (pAlloc
                    && !pAlloc->fRcFlags.Overlay /* overlay surfaces actually contain a valid data */
                    && pAlloc->enmType != VBOXWDDM_ALLOC_TYPE_STD_SHADOWSURFACE  /* shadow primary - also */
                    && pAlloc->enmType != VBOXWDDM_ALLOC_TYPE_UMD_HGSMI_BUFFER /* hgsmi buffer - also */
                    )
            {
                /* we do not care about the others for now */
                Status = STATUS_SUCCESS;
                break;
            }
            UINT cbCmd = VBOXVDMACMD_SIZE(VBOXVDMACMD_DMA_BPB_TRANSFER);
            VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr = vboxVdmaCBufDrCreate(&pDevExt->u.primary.Vdma, cbCmd);
            Assert(pDr);
            if (pDr)
            {
                SIZE_T cbTransfered = 0;
                SIZE_T cbTransferSize = pBuildPagingBuffer->Transfer.TransferSize;
                VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
                do
                {
                    // vboxVdmaCBufDrCreate zero initializes the pDr
                    pDr->fFlags = VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR;
                    pDr->cbBuf = cbCmd;
                    pDr->rc = VERR_NOT_IMPLEMENTED;

                    pHdr->enmType = VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER;
                    pHdr->u32CmdSpecific = 0;
                    VBOXVDMACMD_DMA_BPB_TRANSFER RT_UNTRUSTED_VOLATILE_HOST *pBody
                        = VBOXVDMACMD_BODY(pHdr, VBOXVDMACMD_DMA_BPB_TRANSFER);
//                    pBody->cbTransferSize = (uint32_t)pBuildPagingBuffer->Transfer.TransferSize;
                    pBody->fFlags = 0;
                    SIZE_T cSrcPages = (cbTransferSize + 0xfff ) >> 12;
                    SIZE_T cDstPages = cSrcPages;

                    if (pBuildPagingBuffer->Transfer.Source.SegmentId)
                    {
                        uint64_t off = pBuildPagingBuffer->Transfer.Source.SegmentAddress.QuadPart;
                        off += pBuildPagingBuffer->Transfer.TransferOffset + cbTransfered;
                        pBody->Src.offVramBuf = off;
                        pBody->fFlags |= VBOXVDMACMD_DMA_BPB_TRANSFER_F_SRC_VRAMOFFSET;
                    }
                    else
                    {
                        UINT index = pBuildPagingBuffer->Transfer.MdlOffset + (UINT)(cbTransfered>>12);
                        pBody->Src.phBuf = MmGetMdlPfnArray(pBuildPagingBuffer->Transfer.Source.pMdl)[index] << PAGE_SHIFT;
                        PFN_NUMBER num = MmGetMdlPfnArray(pBuildPagingBuffer->Transfer.Source.pMdl)[index];
                        cSrcPages = 1;
                        for (UINT i = 1; i < ((cbTransferSize - cbTransfered + 0xfff) >> 12); ++i)
                        {
                            PFN_NUMBER cur = MmGetMdlPfnArray(pBuildPagingBuffer->Transfer.Source.pMdl)[index+i];
                            if(cur != ++num)
                            {
                                cSrcPages+= i-1;
                                break;
                            }
                        }
                    }

                    if (pBuildPagingBuffer->Transfer.Destination.SegmentId)
                    {
                        uint64_t off = pBuildPagingBuffer->Transfer.Destination.SegmentAddress.QuadPart;
                        off += pBuildPagingBuffer->Transfer.TransferOffset;
                        pBody->Dst.offVramBuf = off + cbTransfered;
                        pBody->fFlags |= VBOXVDMACMD_DMA_BPB_TRANSFER_F_DST_VRAMOFFSET;
                    }
                    else
                    {
                        UINT index = pBuildPagingBuffer->Transfer.MdlOffset + (UINT)(cbTransfered>>12);
                        pBody->Dst.phBuf = MmGetMdlPfnArray(pBuildPagingBuffer->Transfer.Destination.pMdl)[index] << PAGE_SHIFT;
                        PFN_NUMBER num = MmGetMdlPfnArray(pBuildPagingBuffer->Transfer.Destination.pMdl)[index];
                        cDstPages = 1;
                        for (UINT i = 1; i < ((cbTransferSize - cbTransfered + 0xfff) >> 12); ++i)
                        {
                            PFN_NUMBER cur = MmGetMdlPfnArray(pBuildPagingBuffer->Transfer.Destination.pMdl)[index+i];
                            if(cur != ++num)
                            {
                                cDstPages+= i-1;
                                break;
                            }
                        }
                    }

                    SIZE_T cbCurTransfer;
                    cbCurTransfer = RT_MIN(cbTransferSize - cbTransfered, (SIZE_T)cSrcPages << PAGE_SHIFT);
                    cbCurTransfer = RT_MIN(cbCurTransfer, (SIZE_T)cDstPages << PAGE_SHIFT);

                    pBody->cbTransferSize = (UINT)cbCurTransfer;
                    Assert(!(cbCurTransfer & 0xfff));

                    int rc = vboxVdmaCBufDrSubmitSynch(pDevExt, &pDevExt->u.primary.Vdma, pDr);
                    AssertRC(rc);
                    if (RT_SUCCESS(rc))
                    {
                        Status = STATUS_SUCCESS;
                        cbTransfered += cbCurTransfer;
                    }
                    else
                        Status = STATUS_UNSUCCESSFUL;
                } while (cbTransfered < cbTransferSize);
                Assert(cbTransfered == cbTransferSize);
                vboxVdmaCBufDrFree(&pDevExt->u.primary.Vdma, pDr);
            }
            else
            {
                /** @todo try flushing.. */
                LOGREL(("vboxVdmaCBufDrCreate returned NULL"));
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
#endif
#endif /* #ifdef VBOX_WITH_VDMA */
            break;
        }
        case DXGK_OPERATION_FILL:
        {
            cbCmdDma = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
            Assert(pBuildPagingBuffer->Fill.FillPattern == 0);
            /*PVBOXWDDM_ALLOCATION pAlloc = (PVBOXWDDM_ALLOCATION)pBuildPagingBuffer->Fill.hAllocation; - unused. Incomplete code? */
//            pBuildPagingBuffer->pDmaBuffer = (uint8_t*)pBuildPagingBuffer->pDmaBuffer + VBOXVDMACMD_SIZE(VBOXVDMACMD_DMA_BPB_FILL);
            break;
        }
        case DXGK_OPERATION_DISCARD_CONTENT:
        {
//            AssertBreakpoint();
            break;
        }
        default:
        {
            WARN(("unsupported op (%d)", pBuildPagingBuffer->Operation));
            break;
        }
    }

    if (cbCmdDma)
    {
        pBuildPagingBuffer->pDmaBuffer = ((uint8_t*)pBuildPagingBuffer->pDmaBuffer) + cbCmdDma;
    }

    LOGF(("LEAVE, context(0x%x)", hAdapter));

    return Status;

}

NTSTATUS
APIENTRY
DxgkDdiSetPalette(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_SETPALETTE*  pSetPalette
    )
{
    RT_NOREF(hAdapter, pSetPalette);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertBreakpoint();
    /** @todo fixme: implement */

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

BOOL vboxWddmPointerCopyColorData(CONST DXGKARG_SETPOINTERSHAPE* pSetPointerShape, PVIDEO_POINTER_ATTRIBUTES pPointerAttributes)
{
    ULONG srcMaskW, srcMaskH;
    ULONG dstBytesPerLine;
    ULONG x, y;
    BYTE *pSrc, *pDst, bit;

    srcMaskW = pSetPointerShape->Width;
    srcMaskH = pSetPointerShape->Height;

    /* truncate masks if we exceed supported size */
    pPointerAttributes->Width = min(srcMaskW, VBOXWDDM_C_POINTER_MAX_WIDTH);
    pPointerAttributes->Height = min(srcMaskH, VBOXWDDM_C_POINTER_MAX_HEIGHT);
    pPointerAttributes->WidthInBytes = pPointerAttributes->Width * 4;

    /* cnstruct and mask from alpha color channel */
    pSrc = (PBYTE)pSetPointerShape->pPixels;
    pDst = pPointerAttributes->Pixels;
    dstBytesPerLine = (pPointerAttributes->Width+7)/8;

    /* sanity check */
    uint32_t cbData = RT_ALIGN_T(dstBytesPerLine*pPointerAttributes->Height, 4, ULONG)+
                      pPointerAttributes->Height*pPointerAttributes->WidthInBytes;
    uint32_t cbPointerAttributes = RT_OFFSETOF(VIDEO_POINTER_ATTRIBUTES, Pixels[cbData]);
    Assert(VBOXWDDM_POINTER_ATTRIBUTES_SIZE >= cbPointerAttributes);
    if (VBOXWDDM_POINTER_ATTRIBUTES_SIZE < cbPointerAttributes)
    {
        LOGREL(("VBOXWDDM_POINTER_ATTRIBUTES_SIZE(%d) < cbPointerAttributes(%d)", VBOXWDDM_POINTER_ATTRIBUTES_SIZE, cbPointerAttributes));
        return FALSE;
    }

    memset(pDst, 0xFF, dstBytesPerLine*pPointerAttributes->Height);
    for (y=0; y<pPointerAttributes->Height; ++y)
    {
        for (x=0, bit=7; x<pPointerAttributes->Width; ++x, --bit)
        {
            if (0xFF==bit) bit=7;

            if (pSrc[y*pSetPointerShape->Pitch + x*4 + 3] > 0x7F)
            {
                pDst[y*dstBytesPerLine + x/8] &= ~RT_BIT(bit);
            }
        }
    }

    /* copy 32bpp to XOR DIB, it start in pPointerAttributes->Pixels should be 4bytes aligned */
    pSrc = (BYTE*)pSetPointerShape->pPixels;
    pDst = pPointerAttributes->Pixels + RT_ALIGN_T(dstBytesPerLine*pPointerAttributes->Height, 4, ULONG);
    dstBytesPerLine = pPointerAttributes->Width * 4;

    for (y=0; y<pPointerAttributes->Height; ++y)
    {
        memcpy(pDst+y*dstBytesPerLine, pSrc+y*pSetPointerShape->Pitch, dstBytesPerLine);
    }

    return TRUE;
}

BOOL vboxWddmPointerCopyMonoData(CONST DXGKARG_SETPOINTERSHAPE* pSetPointerShape, PVIDEO_POINTER_ATTRIBUTES pPointerAttributes)
{
    ULONG srcMaskW, srcMaskH;
    ULONG dstBytesPerLine;
    ULONG x, y;
    BYTE *pSrc, *pDst, bit;

    srcMaskW = pSetPointerShape->Width;
    srcMaskH = pSetPointerShape->Height;

    /* truncate masks if we exceed supported size */
    pPointerAttributes->Width = min(srcMaskW, VBOXWDDM_C_POINTER_MAX_WIDTH);
    pPointerAttributes->Height = min(srcMaskH, VBOXWDDM_C_POINTER_MAX_HEIGHT);
    pPointerAttributes->WidthInBytes = pPointerAttributes->Width * 4;

    /* copy AND mask */
    pSrc = (PBYTE)pSetPointerShape->pPixels;
    pDst = pPointerAttributes->Pixels;
    dstBytesPerLine = (pPointerAttributes->Width+7)/8;

    for (y=0; y<pPointerAttributes->Height; ++y)
    {
        memcpy(pDst+y*dstBytesPerLine, pSrc+y*pSetPointerShape->Pitch, dstBytesPerLine);
    }

    /* convert XOR mask to RGB0 DIB, it start in pPointerAttributes->Pixels should be 4bytes aligned */
    pSrc = (BYTE*)pSetPointerShape->pPixels + srcMaskH*pSetPointerShape->Pitch;
    pDst = pPointerAttributes->Pixels + RT_ALIGN_T(dstBytesPerLine*pPointerAttributes->Height, 4, ULONG);
    dstBytesPerLine = pPointerAttributes->Width * 4;

    for (y=0; y<pPointerAttributes->Height; ++y)
    {
        for (x=0, bit=7; x<pPointerAttributes->Width; ++x, --bit)
        {
            if (0xFF==bit) bit=7;

            *(ULONG*)&pDst[y*dstBytesPerLine+x*4] = (pSrc[y*pSetPointerShape->Pitch+x/8] & RT_BIT(bit)) ? 0x00FFFFFF : 0;
        }
    }

    return TRUE;
}

static BOOLEAN vboxVddmPointerShapeToAttributes(CONST DXGKARG_SETPOINTERSHAPE* pSetPointerShape, PVBOXWDDM_POINTER_INFO pPointerInfo)
{
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes = &pPointerInfo->Attributes.data;
    /* pPointerAttributes maintains the visibility state, clear all except visibility */
    pPointerAttributes->Enable &= VBOX_MOUSE_POINTER_VISIBLE;

    Assert(pSetPointerShape->Flags.Value == 1 || pSetPointerShape->Flags.Value == 2);
    if (pSetPointerShape->Flags.Color)
    {
        if (vboxWddmPointerCopyColorData(pSetPointerShape, pPointerAttributes))
        {
            pPointerAttributes->Flags = VIDEO_MODE_COLOR_POINTER;
            pPointerAttributes->Enable |= VBOX_MOUSE_POINTER_ALPHA;
        }
        else
        {
            LOGREL(("vboxWddmPointerCopyColorData failed"));
            AssertBreakpoint();
            return FALSE;
        }

    }
    else if (pSetPointerShape->Flags.Monochrome)
    {
        if (vboxWddmPointerCopyMonoData(pSetPointerShape, pPointerAttributes))
        {
            pPointerAttributes->Flags = VIDEO_MODE_MONO_POINTER;
        }
        else
        {
            LOGREL(("vboxWddmPointerCopyMonoData failed"));
            AssertBreakpoint();
            return FALSE;
        }
    }
    else
    {
        LOGREL(("unsupported pointer type Flags.Value(0x%x)", pSetPointerShape->Flags.Value));
        AssertBreakpoint();
        return FALSE;
    }

    pPointerAttributes->Enable |= VBOX_MOUSE_POINTER_SHAPE;

    /*
     * The hot spot coordinates and alpha flag will be encoded in the pPointerAttributes::Enable field.
     * High word will contain hot spot info and low word - flags.
     */
    pPointerAttributes->Enable |= (pSetPointerShape->YHot & 0xFF) << 24;
    pPointerAttributes->Enable |= (pSetPointerShape->XHot & 0xFF) << 16;

    return TRUE;
}

static void vboxWddmHostPointerEnable(PVBOXMP_DEVEXT pDevExt, BOOLEAN fEnable)
{
    VIDEO_POINTER_ATTRIBUTES PointerAttributes;
    RT_ZERO(PointerAttributes);
    if (fEnable)
    {
        PointerAttributes.Enable = VBOX_MOUSE_POINTER_VISIBLE;
    }
    VBoxMPCmnUpdatePointerShape(VBoxCommonFromDeviceExt(pDevExt), &PointerAttributes, sizeof(PointerAttributes));
}

NTSTATUS
APIENTRY
DxgkDdiSetPointerPosition(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_SETPOINTERPOSITION*  pSetPointerPosition)
{
//    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    /* mouse integration is ON */
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    PVBOXWDDM_POINTER_INFO pPointerInfo = &pDevExt->aSources[pSetPointerPosition->VidPnSourceId].PointerInfo;
    PVBOXWDDM_GLOBAL_POINTER_INFO pGlobalPointerInfo = &pDevExt->PointerInfo;
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes = &pPointerInfo->Attributes.data;
    BOOLEAN fScreenVisState = !!(pPointerAttributes->Enable & VBOX_MOUSE_POINTER_VISIBLE);
    BOOLEAN fVisStateChanged = FALSE;
    BOOLEAN fScreenChanged = pGlobalPointerInfo->iLastReportedScreen != pSetPointerPosition->VidPnSourceId;

    if (pSetPointerPosition->Flags.Visible)
    {
        pPointerAttributes->Enable |= VBOX_MOUSE_POINTER_VISIBLE;
        if (!fScreenVisState)
        {
            fVisStateChanged = !!pGlobalPointerInfo->cVisible;
            ++pGlobalPointerInfo->cVisible;
        }
    }
    else
    {
        pPointerAttributes->Enable &= ~VBOX_MOUSE_POINTER_VISIBLE;
        if (fScreenVisState)
        {
            --pGlobalPointerInfo->cVisible;
            fVisStateChanged = !!pGlobalPointerInfo->cVisible;
        }
    }

    pGlobalPointerInfo->iLastReportedScreen = pSetPointerPosition->VidPnSourceId;

    if ((fVisStateChanged || fScreenChanged) && VBoxQueryHostWantsAbsolute())
    {
        if (fScreenChanged)
        {
            BOOLEAN bResult = VBoxMPCmnUpdatePointerShape(VBoxCommonFromDeviceExt(pDevExt), &pPointerInfo->Attributes.data, VBOXWDDM_POINTER_ATTRIBUTES_SIZE);
            if (!bResult)
            {
                vboxWddmHostPointerEnable(pDevExt, FALSE);
            }
        }
        else
        {
            // tell the host to use the guest's pointer
            vboxWddmHostPointerEnable(pDevExt, pSetPointerPosition->Flags.Visible);
        }
    }

//    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiSetPointerShape(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_SETPOINTERSHAPE*  pSetPointerShape)
{
//    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    if (VBoxQueryHostWantsAbsolute())
    {
        /* mouse integration is ON */
        PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
        PVBOXWDDM_POINTER_INFO pPointerInfo = &pDevExt->aSources[pSetPointerShape->VidPnSourceId].PointerInfo;
        /** @todo to avoid extra data copy and extra heap allocation,
         *  need to maintain the pre-allocated HGSMI buffer and convert the data directly to it */
        if (vboxVddmPointerShapeToAttributes(pSetPointerShape, pPointerInfo))
        {
            pDevExt->PointerInfo.iLastReportedScreen = pSetPointerShape->VidPnSourceId;
            if (VBoxMPCmnUpdatePointerShape(VBoxCommonFromDeviceExt(pDevExt), &pPointerInfo->Attributes.data, VBOXWDDM_POINTER_ATTRIBUTES_SIZE))
                Status = STATUS_SUCCESS;
            else
            {
                // tell the host to use the guest's pointer
                vboxWddmHostPointerEnable(pDevExt, FALSE);
            }
        }
    }

//    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return Status;
}

NTSTATUS
APIENTRY CALLBACK
DxgkDdiResetFromTimeout(
    CONST HANDLE  hAdapter)
{
    RT_NOREF(hAdapter);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertBreakpoint();
    /** @todo fixme: implement */

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}


/* the lpRgnData->Buffer comes to us as RECT
 * to avoid extra memcpy we cast it to PRTRECT assuming
 * they are identical */
AssertCompile(sizeof(RECT) == sizeof(RTRECT));
AssertCompile(RT_OFFSETOF(RECT, left) == RT_OFFSETOF(RTRECT, xLeft));
AssertCompile(RT_OFFSETOF(RECT, bottom) == RT_OFFSETOF(RTRECT, yBottom));
AssertCompile(RT_OFFSETOF(RECT, right) == RT_OFFSETOF(RTRECT, xRight));
AssertCompile(RT_OFFSETOF(RECT, top) == RT_OFFSETOF(RTRECT, yTop));

NTSTATUS
APIENTRY
DxgkDdiEscape(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_ESCAPE*  pEscape)
{
    PAGED_CODE();

//    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    Assert(pEscape->PrivateDriverDataSize >= sizeof (VBOXDISPIFESCAPE));
    if (pEscape->PrivateDriverDataSize >= sizeof (VBOXDISPIFESCAPE))
    {
        PVBOXDISPIFESCAPE pEscapeHdr = (PVBOXDISPIFESCAPE)pEscape->pPrivateDriverData;
        switch (pEscapeHdr->escapeCode)
        {
#ifdef VBOX_WITH_CRHGSMI
            case VBOXESC_UHGSMI_SUBMIT:
            {
                if (pDevExt->fCmdVbvaEnabled)
                {
                    WARN(("VBOXESC_UHGSMI_SUBMIT not supported for CmdVbva mode"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                /* submit VBOXUHGSMI command */
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                PVBOXDISPIFESCAPE_UHGSMI_SUBMIT pSubmit = (PVBOXDISPIFESCAPE_UHGSMI_SUBMIT)pEscapeHdr;
                Assert(pEscape->PrivateDriverDataSize >= sizeof (VBOXDISPIFESCAPE_UHGSMI_SUBMIT)
                        && pEscape->PrivateDriverDataSize == RT_OFFSETOF(VBOXDISPIFESCAPE_UHGSMI_SUBMIT, aBuffers[pEscapeHdr->u32CmdSpecific]));
                if (pEscape->PrivateDriverDataSize >= sizeof (VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD)
                        && pEscape->PrivateDriverDataSize == RT_OFFSETOF(VBOXDISPIFESCAPE_UHGSMI_SUBMIT, aBuffers[pEscapeHdr->u32CmdSpecific]))
                {
                    Status = vboxVideoAMgrCtxAllocSubmit(pDevExt, &pContext->AllocContext, pEscapeHdr->u32CmdSpecific, pSubmit->aBuffers);
                    AssertNtStatusSuccess(Status);
                }
                else
                    Status = STATUS_BUFFER_TOO_SMALL;

                break;
            }

            case VBOXESC_UHGSMI_ALLOCATE:
            {
                /* allocate VBOXUHGSMI buffer */
                if (pDevExt->fCmdVbvaEnabled)
                {
                    WARN(("VBOXESC_UHGSMI_ALLOCATE not supported for CmdVbva mode"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                PVBOXDISPIFESCAPE_UHGSMI_ALLOCATE pAlocate = (PVBOXDISPIFESCAPE_UHGSMI_ALLOCATE)pEscapeHdr;
                Assert(pEscape->PrivateDriverDataSize == sizeof (VBOXDISPIFESCAPE_UHGSMI_ALLOCATE));
                if (pEscape->PrivateDriverDataSize == sizeof (VBOXDISPIFESCAPE_UHGSMI_ALLOCATE))
                {
                    Status = vboxVideoAMgrCtxAllocCreate(&pContext->AllocContext, &pAlocate->Alloc);
                    AssertNtStatusSuccess(Status);
                }
                else
                    Status = STATUS_BUFFER_TOO_SMALL;

                break;
            }

            case VBOXESC_UHGSMI_DEALLOCATE:
            {
                if (pDevExt->fCmdVbvaEnabled)
                {
                    WARN(("VBOXESC_UHGSMI_DEALLOCATE not supported for CmdVbva mode"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                /* deallocate VBOXUHGSMI buffer */
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                PVBOXDISPIFESCAPE_UHGSMI_DEALLOCATE pDealocate = (PVBOXDISPIFESCAPE_UHGSMI_DEALLOCATE)pEscapeHdr;
                Assert(pEscape->PrivateDriverDataSize == sizeof (VBOXDISPIFESCAPE_UHGSMI_DEALLOCATE));
                if (pEscape->PrivateDriverDataSize == sizeof (VBOXDISPIFESCAPE_UHGSMI_DEALLOCATE))
                {
                    Status = vboxVideoAMgrCtxAllocDestroy(&pContext->AllocContext, pDealocate->hAlloc);
                    AssertNtStatusSuccess(Status);
                }
                else
                    Status = STATUS_BUFFER_TOO_SMALL;

                break;
            }

            case VBOXESC_CRHGSMICTLCON_CALL:
            {
                if (pDevExt->fCmdVbvaEnabled)
                {
                    WARN(("VBOXESC_CRHGSMICTLCON_CALL not supported for CmdVbva mode"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PVBOXDISPIFESCAPE_CRHGSMICTLCON_CALL pCall = (PVBOXDISPIFESCAPE_CRHGSMICTLCON_CALL)pEscapeHdr;
                if (pEscape->PrivateDriverDataSize >= sizeof (*pCall))
                {
                    /* this is true due to the above condition */
                    Assert(pEscape->PrivateDriverDataSize > RT_OFFSETOF(VBOXDISPIFESCAPE_CRHGSMICTLCON_CALL, CallInfo));
                    int rc = VBoxMpCrCtlConCallUserData(&pDevExt->CrCtlCon, &pCall->CallInfo, pEscape->PrivateDriverDataSize - RT_OFFSETOF(VBOXDISPIFESCAPE_CRHGSMICTLCON_CALL, CallInfo));
                    pEscapeHdr->u32CmdSpecific = (uint32_t)rc;
                    Status = STATUS_SUCCESS; /* <- always return success here, otherwise the private data buffer modifications
                                              * i.e. rc status stored in u32CmdSpecific will not be copied to user mode */
                    if (!RT_SUCCESS(rc))
                        WARN(("VBoxMpCrUmCtlConCall failed, rc(%d)", rc));
                }
                else
                {
                    WARN(("buffer too small!"));
                    Status = STATUS_BUFFER_TOO_SMALL;
                }

                break;
            }

            case VBOXESC_CRHGSMICTLCON_GETCLIENTID:
            {
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                if (!pContext)
                {
                    WARN(("context not specified"));
                    return STATUS_INVALID_PARAMETER;
                }
                if (pEscape->PrivateDriverDataSize == sizeof (*pEscapeHdr))
                {
                    pEscapeHdr->u32CmdSpecific = pContext->u32CrConClientID;
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    WARN(("unexpected buffer size!"));
                    Status = STATUS_INVALID_PARAMETER;
                }

                break;
            }

            case VBOXESC_CRHGSMICTLCON_GETHOSTCAPS:
            {
                if (pEscape->PrivateDriverDataSize == sizeof (*pEscapeHdr))
                {
                    pEscapeHdr->u32CmdSpecific = VBoxMpCrGetHostCaps();
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    WARN(("unexpected buffer size!"));
                    Status = STATUS_INVALID_PARAMETER;
                }

                break;
            }
#endif

            case VBOXESC_SETVISIBLEREGION:
            {
#ifdef VBOX_DISPIF_WITH_OPCONTEXT
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                if (!pContext)
                {
                    WARN(("VBOXESC_SETVISIBLEREGION no context supplied!"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pContext->enmType != VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_SEAMLESS)
                {
                    WARN(("VBOXESC_SETVISIBLEREGION invalid context supplied %d!", pContext->enmType));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
#endif
                /* visible regions for seamless */
                LPRGNDATA lpRgnData = VBOXDISPIFESCAPE_DATA(pEscapeHdr, RGNDATA);
                uint32_t cbData = VBOXDISPIFESCAPE_DATA_SIZE(pEscape->PrivateDriverDataSize);
                uint32_t cbRects = cbData - RT_OFFSETOF(RGNDATA, Buffer);
                /* the lpRgnData->Buffer comes to us as RECT
                 * to avoid extra memcpy we cast it to PRTRECT assuming
                 * they are identical
                 * see AssertCompile's above */

                RTRECT   *pRect = (RTRECT *)&lpRgnData->Buffer;

                uint32_t cRects = cbRects/sizeof(RTRECT);
                int      rc;

                LOG(("IOCTL_VIDEO_VBOX_SETVISIBLEREGION cRects=%d", cRects));
                Assert(cbRects >= sizeof(RTRECT)
                    &&  cbRects == cRects*sizeof(RTRECT)
                    &&  cRects == lpRgnData->rdh.nCount);
                if (    cbRects >= sizeof(RTRECT)
                    &&  cbRects == cRects*sizeof(RTRECT)
                    &&  cRects == lpRgnData->rdh.nCount)
                {
                    /*
                     * Inform the host about the visible region
                     */
                    VMMDevVideoSetVisibleRegion *req = NULL;

                    rc = VbglR0GRAlloc ((VMMDevRequestHeader **)&req,
                                      sizeof (VMMDevVideoSetVisibleRegion) + (cRects-1)*sizeof(RTRECT),
                                      VMMDevReq_VideoSetVisibleRegion);
                    AssertRC(rc);
                    if (RT_SUCCESS(rc))
                    {
                        req->cRect = cRects;
                        memcpy(&req->Rect, pRect, cRects*sizeof(RTRECT));

                        rc = VbglR0GRPerform (&req->header);
                        AssertRC(rc);
                        if (RT_SUCCESS(rc))
                            Status = STATUS_SUCCESS;
                        else
                        {
                            WARN(("VbglR0GRPerform failed rc (%d)", rc));
                            Status = STATUS_UNSUCCESSFUL;
                        }
                    }
                    else
                    {
                        WARN(("VbglR0GRAlloc failed rc (%d)", rc));
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }
                else
                {
                    WARN(("VBOXESC_SETVISIBLEREGION: incorrect buffer size (%d), reported count (%d)", cbRects, lpRgnData->rdh.nCount));
                    Status = STATUS_INVALID_PARAMETER;
                }
                break;
            }
            case VBOXESC_ISVRDPACTIVE:
                /** @todo implement */
                Status = STATUS_SUCCESS;
                break;
#ifdef VBOX_WITH_CROGL
            case VBOXESC_SETCTXHOSTID:
            {
                /* set swapchain information */
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                if (!pContext)
                {
                    WARN(("VBOXESC_SETCTXHOSTID: no context specified"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pEscape->PrivateDriverDataSize != sizeof (VBOXDISPIFESCAPE))
                {
                    WARN(("VBOXESC_SETCTXHOSTID: invalid data size %d", pEscape->PrivateDriverDataSize));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                int32_t hostID = (int32_t)pEscapeHdr->u32CmdSpecific;
                if (hostID <= 0)
                {
                    WARN(("VBOXESC_SETCTXHOSTID: invalid hostID %d", hostID));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pContext->hostID)
                {
                    WARN(("VBOXESC_SETCTXHOSTID: context already has hostID specified"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                pContext->hostID = hostID;
                Status = STATUS_SUCCESS;
                break;
            }
            case VBOXESC_SWAPCHAININFO:
            {
                /* set swapchain information */
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                Status = vboxWddmSwapchainCtxEscape(pDevExt, pContext, (PVBOXDISPIFESCAPE_SWAPCHAININFO)pEscapeHdr, pEscape->PrivateDriverDataSize);
                AssertNtStatusSuccess(Status);
                break;
            }
#endif
            case VBOXESC_CONFIGURETARGETS:
            {
                LOG(("=> VBOXESC_CONFIGURETARGETS"));

                if (!pEscape->Flags.HardwareAccess)
                {
                    WARN(("VBOXESC_CONFIGURETARGETS called without HardwareAccess flag set, failing"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

#ifdef VBOX_DISPIF_WITH_OPCONTEXT
                /* win8.1 does not allow context-based escapes for display-only mode */
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                if (!pContext)
                {
                    WARN(("VBOXESC_CONFIGURETARGETS no context supplied!"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pContext->enmType != VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_RESIZE)
                {
                    WARN(("VBOXESC_CONFIGURETARGETS invalid context supplied %d!", pContext->enmType));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
#endif

                if (pEscape->PrivateDriverDataSize != sizeof (*pEscapeHdr))
                {
                    WARN(("VBOXESC_CONFIGURETARGETS invalid private driver size %d", pEscape->PrivateDriverDataSize));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pEscapeHdr->u32CmdSpecific)
                {
                    WARN(("VBOXESC_CONFIGURETARGETS invalid command %d", pEscapeHdr->u32CmdSpecific));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                HANDLE hKey = NULL;
                WCHAR aNameBuf[100];
                uint32_t cAdjusted = 0;

                for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
                {
                    VBOXWDDM_TARGET *pTarget = &pDevExt->aTargets[i];
                    if (pTarget->fConfigured)
                        continue;

                    pTarget->fConfigured = true;

                    if (!pTarget->fConnected)
                    {
                        Status = VBoxWddmChildStatusConnect(pDevExt, (uint32_t)i, TRUE);
                        if (NT_SUCCESS(Status))
                            ++cAdjusted;
                        else
                            WARN(("VBOXESC_CONFIGURETARGETS vboxWddmChildStatusConnectSecondaries failed Status 0x%x\n", Status));
                    }

                    if (!hKey)
                    {
                        Status = IoOpenDeviceRegistryKey(pDevExt->pPDO, PLUGPLAY_REGKEY_DRIVER, GENERIC_WRITE, &hKey);
                        if (!NT_SUCCESS(Status))
                        {
                            WARN(("VBOXESC_CONFIGURETARGETS IoOpenDeviceRegistryKey failed, Status = 0x%x", Status));
                            hKey = NULL;
                            continue;
                        }
                    }

                    Assert(hKey);

                    swprintf(aNameBuf, L"%s%d", VBOXWDDM_REG_DRV_DISPFLAGS_PREFIX, i);
                    Status = vboxWddmRegSetValueDword(hKey, aNameBuf, VBOXWDDM_CFG_DRVTARGET_CONNECTED);
                    if (!NT_SUCCESS(Status))
                        WARN(("VBOXESC_CONFIGURETARGETS vboxWddmRegSetValueDword (%d) failed Status 0x%x\n", aNameBuf, Status));

                }

                if (hKey)
                {
                    NTSTATUS rcNt2 = ZwClose(hKey);
                    Assert(rcNt2 == STATUS_SUCCESS); NOREF(rcNt2);
                }

                pEscapeHdr->u32CmdSpecific = cAdjusted;

                Status = STATUS_SUCCESS;

                LOG(("<= VBOXESC_CONFIGURETARGETS"));
                break;
            }
            case VBOXESC_SETALLOCHOSTID:
            {
                PVBOXWDDM_DEVICE pDevice = (PVBOXWDDM_DEVICE)pEscape->hDevice;
                if (!pDevice)
                {
                    WARN(("VBOXESC_SETALLOCHOSTID called without no device specified, failing"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pEscape->PrivateDriverDataSize != sizeof (VBOXDISPIFESCAPE_SETALLOCHOSTID))
                {
                    WARN(("invalid buffer size for VBOXDISPIFESCAPE_SHRC_REF, was(%d), but expected (%d)",
                            pEscape->PrivateDriverDataSize, sizeof (VBOXDISPIFESCAPE_SHRC_REF)));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (!pEscape->Flags.HardwareAccess)
                {
                    WARN(("VBOXESC_SETALLOCHOSTID not HardwareAccess"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PVBOXDISPIFESCAPE_SETALLOCHOSTID pSetHostID = (PVBOXDISPIFESCAPE_SETALLOCHOSTID)pEscapeHdr;
                PVBOXWDDM_ALLOCATION pAlloc = vboxWddmGetAllocationFromHandle(pDevExt, (D3DKMT_HANDLE)pSetHostID->hAlloc);
                if (!pAlloc)
                {
                    WARN(("failed to get allocation from handle"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pAlloc->enmType != VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE)
                {
                    WARN(("setHostID: invalid allocation type: %d", pAlloc->enmType));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                pSetHostID->rc = VBoxWddmOaSetHostID(pDevice, pAlloc, pSetHostID->hostID, &pSetHostID->EscapeHdr.u32CmdSpecific);

                if (pAlloc->bAssigned)
                {
                    PVBOXMP_DEVEXT pDevExt = pDevice->pAdapter;
                    Assert(pAlloc->AllocData.SurfDesc.VidPnSourceId < (D3DDDI_VIDEO_PRESENT_SOURCE_ID)VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
                    PVBOXWDDM_SOURCE pSource = &pDevExt->aSources[pAlloc->AllocData.SurfDesc.VidPnSourceId];
                    if (pSource->AllocData.hostID != pAlloc->AllocData.hostID)
                    {
                        pSource->AllocData.hostID = pAlloc->AllocData.hostID;
                        pSource->u8SyncState &= ~VBOXWDDM_HGSYNC_F_SYNCED_LOCATION;

                        vboxWddmGhDisplayCheckSetInfo(pDevExt);
                    }
                }

                Status = STATUS_SUCCESS;
                break;
            }
            case VBOXESC_SHRC_ADDREF:
            case VBOXESC_SHRC_RELEASE:
            {
                PVBOXWDDM_DEVICE pDevice = (PVBOXWDDM_DEVICE)pEscape->hDevice;
                if (!pDevice)
                {
                    WARN(("VBOXESC_SHRC_ADDREF|VBOXESC_SHRC_RELEASE called without no device specified, failing"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                /* query whether the allocation represanted by the given [wine-generated] shared resource handle still exists */
                if (pEscape->PrivateDriverDataSize != sizeof (VBOXDISPIFESCAPE_SHRC_REF))
                {
                    WARN(("invalid buffer size for VBOXDISPIFESCAPE_SHRC_REF, was(%d), but expected (%d)",
                            pEscape->PrivateDriverDataSize, sizeof (VBOXDISPIFESCAPE_SHRC_REF)));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PVBOXDISPIFESCAPE_SHRC_REF pShRcRef = (PVBOXDISPIFESCAPE_SHRC_REF)pEscapeHdr;
                PVBOXWDDM_ALLOCATION pAlloc = vboxWddmGetAllocationFromHandle(pDevExt, (D3DKMT_HANDLE)pShRcRef->hAlloc);
                if (!pAlloc)
                {
                    WARN(("failed to get allocation from handle"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PVBOXWDDM_OPENALLOCATION pOa = VBoxWddmOaSearch(pDevice, pAlloc);
                if (!pOa)
                {
                    WARN(("failed to get open allocation from alloc"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                Assert(pAlloc->cShRcRefs >= pOa->cShRcRefs);

                if (pEscapeHdr->escapeCode == VBOXESC_SHRC_ADDREF)
                {
#ifdef DEBUG
                    Assert(!pAlloc->fAssumedDeletion);
#endif
                    ++pAlloc->cShRcRefs;
                    ++pOa->cShRcRefs;
                }
                else
                {
                    Assert(pAlloc->cShRcRefs);
                    Assert(pOa->cShRcRefs);
                    --pAlloc->cShRcRefs;
                    --pOa->cShRcRefs;
#ifdef DEBUG
                    Assert(!pAlloc->fAssumedDeletion);
                    if (!pAlloc->cShRcRefs)
                    {
                        pAlloc->fAssumedDeletion = TRUE;
                    }
#endif
                }

                pShRcRef->EscapeHdr.u32CmdSpecific = pAlloc->cShRcRefs;
                Status = STATUS_SUCCESS;
                break;
            }
            case VBOXESC_ISANYX:
            {
                if (pEscape->PrivateDriverDataSize != sizeof (VBOXDISPIFESCAPE_ISANYX))
                {
                    WARN(("invalid private driver size %d", pEscape->PrivateDriverDataSize));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PVBOXDISPIFESCAPE_ISANYX pIsAnyX = (PVBOXDISPIFESCAPE_ISANYX)pEscapeHdr;
                pIsAnyX->u32IsAnyX = VBoxCommonFromDeviceExt(pDevExt)->fAnyX;
                Status = STATUS_SUCCESS;
                break;
            }
            case VBOXESC_UPDATEMODES:
            {
                LOG(("=> VBOXESC_UPDATEMODES"));

                if (!pEscape->Flags.HardwareAccess)
                {
                    WARN(("VBOXESC_UPDATEMODES called without HardwareAccess flag set, failing"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

#ifdef VBOX_DISPIF_WITH_OPCONTEXT
                /* win8.1 does not allow context-based escapes for display-only mode */
                PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)pEscape->hContext;
                if (!pContext)
                {
                    WARN(("VBOXESC_UPDATEMODES no context supplied!"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pContext->enmType != VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_RESIZE)
                {
                    WARN(("VBOXESC_UPDATEMODES invalid context supplied %d!", pContext->enmType));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
#endif

                if (pEscape->PrivateDriverDataSize != sizeof (VBOXDISPIFESCAPE_UPDATEMODES))
                {
                    WARN(("VBOXESC_UPDATEMODES invalid private driver size %d", pEscape->PrivateDriverDataSize));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                VBOXDISPIFESCAPE_UPDATEMODES *pData = (VBOXDISPIFESCAPE_UPDATEMODES*)pEscapeHdr;
                Status = VBoxVidPnUpdateModes(pDevExt, pData->u32TargetId, &pData->Size);
                if (!NT_SUCCESS(Status))
                {
                    WARN(("VBoxVidPnUpdateModes failed Status(%#x)\n", Status));
                    return Status;
                }

                break;
            }
            case VBOXESC_TARGET_CONNECTIVITY:
            {
                if (!pEscape->Flags.HardwareAccess)
                {
                    WARN(("VBOXESC_TARGET_CONNECTIVITY called without HardwareAccess flag set, failing"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (pEscape->PrivateDriverDataSize != sizeof(VBOXDISPIFESCAPE_TARGETCONNECTIVITY))
                {
                    WARN(("VBOXESC_TARGET_CONNECTIVITY invalid private driver size %d", pEscape->PrivateDriverDataSize));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                VBOXDISPIFESCAPE_TARGETCONNECTIVITY *pData = (VBOXDISPIFESCAPE_TARGETCONNECTIVITY *)pEscapeHdr;
                LOG(("=> VBOXESC_TARGET_CONNECTIVITY[%d] 0x%08X", pData->u32TargetId, pData->fu32Connect));

                if (pData->u32TargetId >= (uint32_t)VBoxCommonFromDeviceExt(pDevExt)->cDisplays)
                {
                    WARN(("VBOXESC_TARGET_CONNECTIVITY invalid screen index 0x%x", pData->u32TargetId));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PVBOXWDDM_TARGET pTarget = &pDevExt->aTargets[pData->u32TargetId];
                pTarget->fDisabled = !RT_BOOL(pData->fu32Connect & 1);
                pTarget->u8SyncState &= ~VBOXWDDM_HGSYNC_F_SYNCED_TOPOLOGY;

                break;
            }
            case VBOXESC_DBGPRINT:
            {
                /* use RT_OFFSETOF instead of sizeof since sizeof will give an aligned size that might
                 * be bigger than the VBOXDISPIFESCAPE_DBGPRINT with a data containing just a few chars */
                Assert(pEscape->PrivateDriverDataSize >= RT_OFFSETOF(VBOXDISPIFESCAPE_DBGPRINT, aStringBuf[1]));
                /* only do DbgPrint when pEscape->PrivateDriverDataSize > RT_OFFSETOF(VBOXDISPIFESCAPE_DBGPRINT, aStringBuf[1])
                 * since == RT_OFFSETOF(VBOXDISPIFESCAPE_DBGPRINT, aStringBuf[1]) means the buffer contains just \0,
                 * i.e. no need to print it */
                if (pEscape->PrivateDriverDataSize > RT_OFFSETOF(VBOXDISPIFESCAPE_DBGPRINT, aStringBuf[1]))
                {
                    PVBOXDISPIFESCAPE_DBGPRINT pDbgPrint = (PVBOXDISPIFESCAPE_DBGPRINT)pEscapeHdr;
                    /* ensure the last char is \0*/
                    if (*((uint8_t*)pDbgPrint + pEscape->PrivateDriverDataSize - 1) == '\0')
                    {
                        if (g_VBoxLogUm & VBOXWDDM_CFG_LOG_UM_DBGPRINT)
                            DbgPrint("%s\n", pDbgPrint->aStringBuf);
                        if (g_VBoxLogUm & VBOXWDDM_CFG_LOG_UM_BACKDOOR)
                            LOGREL_EXACT(("%s\n", pDbgPrint->aStringBuf));
                    }
                }
                Status = STATUS_SUCCESS;
                break;
            }
            case VBOXESC_DBGDUMPBUF:
            {
                Status = vboxUmdDumpBuf((PVBOXDISPIFESCAPE_DBGDUMPBUF)pEscapeHdr, pEscape->PrivateDriverDataSize);
                break;
            }
            case VBOXESC_GUEST_DISPLAYCHANGED:
            {
                LOG(("=> VBOXESC_GUEST_DISPLAYCHANGED"));

                for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
                {
                    vboxWddmDisplaySettingsCheckPos(pDevExt, i);
                }
                break;
            }
            default:
                WARN(("unsupported escape code (0x%x)", pEscapeHdr->escapeCode));
                break;
        }
    }
    else
    {
        WARN(("pEscape->PrivateDriverDataSize(%d) < (%d)", pEscape->PrivateDriverDataSize, sizeof (VBOXDISPIFESCAPE)));
        Status = STATUS_BUFFER_TOO_SMALL;
    }

//    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiCollectDbgInfo(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_COLLECTDBGINFO*  pCollectDbgInfo
    )
{
    RT_NOREF(hAdapter, pCollectDbgInfo);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertBreakpoint();

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

typedef struct VBOXWDDM_QUERYCURFENCE_CB
{
    PVBOXMP_DEVEXT pDevExt;
    ULONG MessageNumber;
    ULONG uLastCompletedCmdFenceId;
} VBOXWDDM_QUERYCURFENCE_CB, *PVBOXWDDM_QUERYCURFENCE_CB;

static BOOLEAN vboxWddmQueryCurrentFenceCb(PVOID Context)
{
    PVBOXWDDM_QUERYCURFENCE_CB pdc = (PVBOXWDDM_QUERYCURFENCE_CB)Context;
    PVBOXMP_DEVEXT pDevExt = pdc->pDevExt;
    BOOL bRc = DxgkDdiInterruptRoutineLegacy(pDevExt, pdc->MessageNumber);
    pdc->uLastCompletedCmdFenceId = pDevExt->u.primary.Vdma.uLastCompletedPagingBufferCmdFenceId;
    return bRc;
}

#ifdef VBOX_WITH_CROGL
static NTSTATUS
APIENTRY
DxgkDdiQueryCurrentFenceNew(
    CONST HANDLE  hAdapter,
    DXGKARG_QUERYCURRENTFENCE*  pCurrentFence)
{
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    vboxVDbgBreakF();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    WARN(("=>DxgkDdiQueryCurrentFenceNew"));

    uint32_t u32FenceSubmitted = 0;
    uint32_t u32FenceCompleted = 0;
    uint32_t u32FenceProcessed = 0;

    LARGE_INTEGER DelayInterval;
    DelayInterval.QuadPart = -10LL * 1000LL * 1000LL;

    for (;;)
    {
        u32FenceCompleted = VBoxCmdVbvaCheckCompleted(pDevExt, &pDevExt->CmdVbva, false, &u32FenceSubmitted, &u32FenceProcessed);
        if (!u32FenceCompleted)
        {
            WARN(("VBoxCmdVbvaCheckCompleted failed"));
            return STATUS_UNSUCCESSFUL;
        }

        if (u32FenceSubmitted == u32FenceProcessed)
            break;

        WARN(("uncompleted fences, u32FenceSubmitted(%d), u32FenceCompleted(%d) u32FenceProcessed(%d)", u32FenceSubmitted, u32FenceCompleted, u32FenceProcessed));

        NTSTATUS Status = KeDelayExecutionThread(KernelMode, FALSE, &DelayInterval);
        if (Status != STATUS_SUCCESS)
            WARN(("KeDelayExecutionThread failed %#x", Status));
    }

    pCurrentFence->CurrentFence = u32FenceCompleted;

    WARN(("<=DxgkDdiQueryCurrentFenceNew"));

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}
#endif

static NTSTATUS
APIENTRY
DxgkDdiQueryCurrentFenceLegacy(
    CONST HANDLE  hAdapter,
    DXGKARG_QUERYCURRENTFENCE*  pCurrentFence)
{
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    vboxVDbgBreakF();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    VBOXWDDM_QUERYCURFENCE_CB context = {0};
    context.pDevExt = pDevExt;
    BOOLEAN bRet;
    NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbSynchronizeExecution(
            pDevExt->u.primary.DxgkInterface.DeviceHandle,
            vboxWddmQueryCurrentFenceCb,
            &context,
            0, /* IN ULONG MessageNumber */
            &bRet);
    AssertNtStatusSuccess(Status);
    if (Status == STATUS_SUCCESS)
    {
        pCurrentFence->CurrentFence = context.uLastCompletedCmdFenceId;
    }

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiIsSupportedVidPn(
    CONST HANDLE  hAdapter,
    OUT DXGKARG_ISSUPPORTEDVIDPN*  pIsSupportedVidPnArg
    )
{
    /* The DxgkDdiIsSupportedVidPn should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    NTSTATUS Status = VBoxVidPnIsSupported(pDevExt, pIsSupportedVidPnArg->hDesiredVidPn, &pIsSupportedVidPnArg->IsVidPnSupported);
    if (!NT_SUCCESS(Status))
    {
        WARN(("VBoxVidPnIsSupported failed Status(%#x)\n", Status));
        return Status;
    }

    LOGF(("LEAVE, isSupported(%d), context(0x%x)", pIsSupportedVidPnArg->IsVidPnSupported, hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiRecommendFunctionalVidPn(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_RECOMMENDFUNCTIONALVIDPN* CONST  pRecommendFunctionalVidPnArg
    )
{
    /* The DxgkDdiRecommendFunctionalVidPn should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    if (pRecommendFunctionalVidPnArg->PrivateDriverDataSize != sizeof (VBOXWDDM_RECOMMENDVIDPN))
    {
        WARN(("invalid size"));
        return STATUS_INVALID_PARAMETER;
    }

    VBOXWDDM_RECOMMENDVIDPN *pData = (VBOXWDDM_RECOMMENDVIDPN*)pRecommendFunctionalVidPnArg->pPrivateDriverData;
    Assert(pData);

    NTSTATUS Status = VBoxVidPnRecommendFunctional(pDevExt, pRecommendFunctionalVidPnArg->hRecommendedFunctionalVidPn, pData);
    if (!NT_SUCCESS(Status))
    {
        WARN(("VBoxVidPnRecommendFunctional failed %#x", Status));
        return Status;
    }

    LOGF(("LEAVE, status(0x%x), context(0x%x)", Status, hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiEnumVidPnCofuncModality(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_ENUMVIDPNCOFUNCMODALITY* CONST  pEnumCofuncModalityArg
    )
{
    /* The DxgkDdiEnumVidPnCofuncModality function should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    NTSTATUS Status = VBoxVidPnCofuncModality(pDevExt, pEnumCofuncModalityArg->hConstrainingVidPn, pEnumCofuncModalityArg->EnumPivotType, &pEnumCofuncModalityArg->EnumPivot);
    if (!NT_SUCCESS(Status))
    {
        WARN(("VBoxVidPnCofuncModality failed Status(%#x)\n", Status));
        return Status;
    }

    LOGF(("LEAVE, status(0x%x), context(0x%x)", Status, hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiSetVidPnSourceAddress(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_SETVIDPNSOURCEADDRESS*  pSetVidPnSourceAddress
    )
{
    /* The DxgkDdiSetVidPnSourceAddress function should be made pageable. */
    PAGED_CODE();

    vboxVDbgBreakFv();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    if ((UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays <= pSetVidPnSourceAddress->VidPnSourceId)
    {
        WARN(("invalid VidPnSourceId (%d), for displays(%d)", pSetVidPnSourceAddress->VidPnSourceId, VBoxCommonFromDeviceExt(pDevExt)->cDisplays));
        return STATUS_INVALID_PARAMETER;
    }

    vboxWddmDisplaySettingsCheckPos(pDevExt, pSetVidPnSourceAddress->VidPnSourceId);

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_SOURCE pSource = &pDevExt->aSources[pSetVidPnSourceAddress->VidPnSourceId];
    PVBOXWDDM_ALLOCATION pAllocation;
    Assert(pSetVidPnSourceAddress->hAllocation);
    Assert(pSetVidPnSourceAddress->hAllocation || pSource->pPrimaryAllocation);
    Assert (pSetVidPnSourceAddress->Flags.Value < 2); /* i.e. 0 or 1 (ModeChange) */

    if (pSetVidPnSourceAddress->hAllocation)
    {
        pAllocation = (PVBOXWDDM_ALLOCATION)pSetVidPnSourceAddress->hAllocation;
        vboxWddmAssignPrimary(pSource, pAllocation, pSetVidPnSourceAddress->VidPnSourceId);
    }
    else
        pAllocation = pSource->pPrimaryAllocation;

    if (pAllocation)
    {
        vboxWddmAddrSetVram(&pAllocation->AllocData.Addr, pSetVidPnSourceAddress->PrimarySegment, (VBOXVIDEOOFFSET)pSetVidPnSourceAddress->PrimaryAddress.QuadPart);
    }

#ifdef VBOX_WDDM_WIN8
    if (g_VBoxDisplayOnly && !pAllocation)
    {
        /* the VRAM here is an absolute address, nto an offset!
         * convert to offset since all internal VBox functionality is offset-based */
        vboxWddmAddrSetVram(&pSource->AllocData.Addr, pSetVidPnSourceAddress->PrimarySegment,
                vboxWddmVramAddrToOffset(pDevExt, pSetVidPnSourceAddress->PrimaryAddress));
    }
    else
#endif
    {
#ifdef VBOX_WDDM_WIN8
        Assert(!g_VBoxDisplayOnly);
#endif
        vboxWddmAddrSetVram(&pSource->AllocData.Addr, pSetVidPnSourceAddress->PrimarySegment,
                                                    pSetVidPnSourceAddress->PrimaryAddress.QuadPart);
    }

    pSource->u8SyncState &= ~VBOXWDDM_HGSYNC_F_SYNCED_LOCATION;

    vboxWddmGhDisplayCheckSetInfoFromSource(pDevExt, pSource);

    LOGF(("LEAVE, status(0x%x), context(0x%x)", Status, hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiSetVidPnSourceVisibility(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_SETVIDPNSOURCEVISIBILITY* pSetVidPnSourceVisibility
    )
{
    /* DxgkDdiSetVidPnSourceVisibility should be made pageable. */
    PAGED_CODE();

    vboxVDbgBreakFv();

    LOGF(("ENTER, context(0x%x)", hAdapter));

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    if ((UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays <= pSetVidPnSourceVisibility->VidPnSourceId)
    {
        WARN(("invalid VidPnSourceId (%d), for displays(%d)", pSetVidPnSourceVisibility->VidPnSourceId, VBoxCommonFromDeviceExt(pDevExt)->cDisplays));
        return STATUS_INVALID_PARAMETER;
    }

    vboxWddmDisplaySettingsCheckPos(pDevExt, pSetVidPnSourceVisibility->VidPnSourceId);

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_SOURCE pSource = &pDevExt->aSources[pSetVidPnSourceVisibility->VidPnSourceId];
    PVBOXWDDM_ALLOCATION pAllocation = pSource->pPrimaryAllocation;
    if (pAllocation)
    {
        Assert(pAllocation->bVisible == pSource->bVisible);
        pAllocation->bVisible = pSetVidPnSourceVisibility->Visible;
    }

    if (pSource->bVisible != pSetVidPnSourceVisibility->Visible)
    {
        pSource->bVisible = pSetVidPnSourceVisibility->Visible;
//        pSource->u8SyncState &= ~VBOXWDDM_HGSYNC_F_SYNCED_VISIBILITY;
//        if (pDevExt->fCmdVbvaEnabled || pSource->bVisible)
//        {
//            vboxWddmGhDisplayCheckSetInfoFromSource(pDevExt, pSource);
//        }
    }

    LOGF(("LEAVE, status(0x%x), context(0x%x)", Status, hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiCommitVidPn(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_COMMITVIDPN* CONST  pCommitVidPnArg
    )
{
    LOG(("ENTER AffectedVidPnSourceId(%d) hAdapter(0x%x)", pCommitVidPnArg->AffectedVidPnSourceId, hAdapter));

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    NTSTATUS Status;

    vboxVDbgBreakFv();

    VBOXWDDM_SOURCE *paSources = (VBOXWDDM_SOURCE*)RTMemAlloc(sizeof (VBOXWDDM_SOURCE) * VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
    if (!paSources)
    {
        WARN(("RTMemAlloc failed"));
        return STATUS_NO_MEMORY;
    }

    VBOXWDDM_TARGET *paTargets = (VBOXWDDM_TARGET*)RTMemAlloc(sizeof (VBOXWDDM_TARGET) * VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
    if (!paTargets)
    {
        WARN(("RTMemAlloc failed"));
        RTMemFree(paSources);
        return STATUS_NO_MEMORY;
    }

    VBoxVidPnSourcesInit(paSources, VBoxCommonFromDeviceExt(pDevExt)->cDisplays, VBOXWDDM_HGSYNC_F_SYNCED_ALL);

    VBoxVidPnTargetsInit(paTargets, VBoxCommonFromDeviceExt(pDevExt)->cDisplays, VBOXWDDM_HGSYNC_F_SYNCED_ALL);

    VBoxVidPnSourcesCopy(paSources, pDevExt->aSources, VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
    VBoxVidPnTargetsCopy(paTargets, pDevExt->aTargets, VBoxCommonFromDeviceExt(pDevExt)->cDisplays);

    do {
        const DXGK_VIDPN_INTERFACE* pVidPnInterface = NULL;
        Status = pDevExt->u.primary.DxgkInterface.DxgkCbQueryVidPnInterface(pCommitVidPnArg->hFunctionalVidPn, DXGK_VIDPN_INTERFACE_VERSION_V1, &pVidPnInterface);
        if (!NT_SUCCESS(Status))
        {
            WARN(("DxgkCbQueryVidPnInterface failed Status 0x%x", Status));
            break;
        }

#ifdef VBOXWDDM_DEBUG_VIDPN
        vboxVidPnDumpVidPn("\n>>>>COMMIT VidPN: >>>>", pDevExt, pCommitVidPnArg->hFunctionalVidPn, pVidPnInterface, "<<<<<<<<<<<<<<<<<<<<\n");
#endif

        if (pCommitVidPnArg->AffectedVidPnSourceId != D3DDDI_ID_ALL)
        {
            Status = VBoxVidPnCommitSourceModeForSrcId(
                    pDevExt,
                    pCommitVidPnArg->hFunctionalVidPn, pVidPnInterface,
                    (PVBOXWDDM_ALLOCATION)pCommitVidPnArg->hPrimaryAllocation,
                    pCommitVidPnArg->AffectedVidPnSourceId, paSources, paTargets, pCommitVidPnArg->Flags.PathPowerTransition);
            if (!NT_SUCCESS(Status))
            {
                WARN(("VBoxVidPnCommitSourceModeForSrcId for current VidPn failed Status 0x%x", Status));
                break;
            }
        }
        else
        {
            Status = VBoxVidPnCommitAll(pDevExt, pCommitVidPnArg->hFunctionalVidPn, pVidPnInterface,
                    (PVBOXWDDM_ALLOCATION)pCommitVidPnArg->hPrimaryAllocation,
                    paSources, paTargets);
            if (!NT_SUCCESS(Status))
            {
                WARN(("VBoxVidPnCommitAll for current VidPn failed Status 0x%x", Status));
                break;
            }
        }

        Assert(NT_SUCCESS(Status));
        pDevExt->u.primary.hCommittedVidPn = pCommitVidPnArg->hFunctionalVidPn;
        VBoxVidPnSourcesCopy(pDevExt->aSources, paSources, VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
        VBoxVidPnTargetsCopy(pDevExt->aTargets, paTargets, VBoxCommonFromDeviceExt(pDevExt)->cDisplays);

        VBoxDumpSourceTargetArrays(paSources, paTargets, VBoxCommonFromDeviceExt(pDevExt)->cDisplays);

        vboxWddmGhDisplayCheckSetInfo(pDevExt);
    } while (0);

    RTMemFree(paSources);
    RTMemFree(paTargets);

    LOG(("LEAVE, status(0x%x), hAdapter(0x%x)", Status, hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiUpdateActiveVidPnPresentPath(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH* CONST  pUpdateActiveVidPnPresentPathArg
    )
{
    RT_NOREF(hAdapter, pUpdateActiveVidPnPresentPathArg);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertBreakpoint();

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiRecommendMonitorModes(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_RECOMMENDMONITORMODES* CONST  pRecommendMonitorModesArg
    )
{
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    NTSTATUS Status = VBoxVidPnRecommendMonitorModes(pDevExt, pRecommendMonitorModesArg->VideoPresentTargetId,
            pRecommendMonitorModesArg->hMonitorSourceModeSet, pRecommendMonitorModesArg->pMonitorSourceModeSetInterface);
    if (!NT_SUCCESS(Status))
    {
        WARN(("VBoxVidPnRecommendMonitorModes failed %#x", Status));
        return Status;
    }

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiRecommendVidPnTopology(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_RECOMMENDVIDPNTOPOLOGY* CONST  pRecommendVidPnTopologyArg
    )
{
    RT_NOREF(hAdapter, pRecommendVidPnTopologyArg);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    vboxVDbgBreakFv();

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_GRAPHICS_NO_RECOMMENDED_VIDPN_TOPOLOGY;
}

NTSTATUS
APIENTRY
DxgkDdiGetScanLine(
    CONST HANDLE  hAdapter,
    DXGKARG_GETSCANLINE*  pGetScanLine)
{
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

#ifdef DEBUG_misha
//    RT_BREAKPOINT();
#endif

    NTSTATUS Status = VBoxWddmSlGetScanLine(pDevExt, pGetScanLine);

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiStopCapture(
    CONST HANDLE  hAdapter,
    CONST DXGKARG_STOPCAPTURE*  pStopCapture)
{
    RT_NOREF(hAdapter, pStopCapture);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    AssertBreakpoint();

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
DxgkDdiControlInterrupt(
    CONST HANDLE hAdapter,
    CONST DXGK_INTERRUPT_TYPE InterruptType,
    BOOLEAN Enable
    )
{
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));

    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;

    switch (InterruptType)
    {
#ifdef VBOX_WDDM_WIN8
        case DXGK_INTERRUPT_DISPLAYONLY_VSYNC:
#endif
        case DXGK_INTERRUPT_CRTC_VSYNC:
        {
            Status = VBoxWddmSlEnableVSyncNotification(pDevExt, Enable);
            if (NT_SUCCESS(Status))
                Status = STATUS_SUCCESS; /* <- sanity */
            else
                WARN(("VSYNC Interrupt control failed Enable(%d), Status(0x%x)", Enable, Status));
            break;
        }
        case DXGK_INTERRUPT_DMA_COMPLETED:
        case DXGK_INTERRUPT_DMA_PREEMPTED:
        case DXGK_INTERRUPT_DMA_FAULTED:
            WARN(("Unexpected interrupt type! %d", InterruptType));
            break;
        default:
            WARN(("UNSUPPORTED interrupt type! %d", InterruptType));
            break;
    }

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiCreateOverlay(
    CONST HANDLE  hAdapter,
    DXGKARG_CREATEOVERLAY  *pCreateOverlay)
{
    LOGF(("ENTER, hAdapter(0x%p)", hAdapter));

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    PVBOXWDDM_OVERLAY pOverlay = (PVBOXWDDM_OVERLAY)vboxWddmMemAllocZero(sizeof (VBOXWDDM_OVERLAY));
    Assert(pOverlay);
    if (pOverlay)
    {
        int rc = vboxVhwaHlpOverlayCreate(pDevExt, pCreateOverlay->VidPnSourceId, &pCreateOverlay->OverlayInfo, pOverlay);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            pCreateOverlay->hOverlay = pOverlay;
        }
        else
        {
            vboxWddmMemFree(pOverlay);
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
        Status = STATUS_NO_MEMORY;

    LOGF(("LEAVE, hAdapter(0x%p)", hAdapter));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiDestroyDevice(
    CONST HANDLE  hDevice)
{
    /* DxgkDdiDestroyDevice should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, hDevice(0x%x)", hDevice));

    vboxVDbgBreakFv();

    vboxWddmMemFree(hDevice);

    LOGF(("LEAVE, "));

    return STATUS_SUCCESS;
}



/*
 * DxgkDdiOpenAllocation
 */
NTSTATUS
APIENTRY
DxgkDdiOpenAllocation(
    CONST HANDLE  hDevice,
    CONST DXGKARG_OPENALLOCATION  *pOpenAllocation)
{
    /* DxgkDdiOpenAllocation should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, hDevice(0x%x)", hDevice));

    vboxVDbgBreakFv();

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_DEVICE pDevice = (PVBOXWDDM_DEVICE)hDevice;
    PVBOXMP_DEVEXT pDevExt = pDevice->pAdapter;
    PVBOXWDDM_RCINFO pRcInfo = NULL;
    if (pOpenAllocation->PrivateDriverSize)
    {
        Assert(pOpenAllocation->pPrivateDriverData);
        if (pOpenAllocation->PrivateDriverSize == sizeof (VBOXWDDM_RCINFO))
        {
            pRcInfo = (PVBOXWDDM_RCINFO)pOpenAllocation->pPrivateDriverData;
            Assert(pRcInfo->cAllocInfos == pOpenAllocation->NumAllocations);
        }
        else
        {
            WARN(("Invalid PrivateDriverSize %d", pOpenAllocation->PrivateDriverSize));
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if (Status == STATUS_SUCCESS)
    {
        UINT i = 0;
        for (; i < pOpenAllocation->NumAllocations; ++i)
        {
            DXGK_OPENALLOCATIONINFO* pInfo = &pOpenAllocation->pOpenAllocation[i];
            Assert(pInfo->PrivateDriverDataSize == sizeof (VBOXWDDM_ALLOCINFO));
            Assert(pInfo->pPrivateDriverData);
            PVBOXWDDM_ALLOCATION pAllocation = vboxWddmGetAllocationFromHandle(pDevExt, pInfo->hAllocation);
            if (!pAllocation)
            {
                WARN(("invalid handle"));
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

#ifdef DEBUG
            Assert(!pAllocation->fAssumedDeletion);
#endif
            if (pRcInfo)
            {
                Assert(pAllocation->enmType == VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC);

                if (pInfo->PrivateDriverDataSize != sizeof (VBOXWDDM_ALLOCINFO)
                        || !pInfo->pPrivateDriverData)
                {
                    WARN(("invalid data size"));
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                PVBOXWDDM_ALLOCINFO pAllocInfo = (PVBOXWDDM_ALLOCINFO)pInfo->pPrivateDriverData;

#ifdef VBOX_WITH_VIDEOHWACCEL
                if (pRcInfo->RcDesc.fFlags.Overlay)
                {
                    /* we have queried host for some surface info, like pitch & size,
                     * need to return it back to the UMD (User Mode Drive) */
                    pAllocInfo->SurfDesc = pAllocation->AllocData.SurfDesc;
                    /* success, just continue */
                }
#endif
            }

            KIRQL OldIrql;
            PVBOXWDDM_OPENALLOCATION pOa;
            KeAcquireSpinLock(&pAllocation->OpenLock, &OldIrql);
            pOa = VBoxWddmOaSearchLocked(pDevice, pAllocation);
            if (pOa)
            {
                ++pOa->cOpens;
                ++pAllocation->cOpens;
                KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);
            }
            else
            {
                KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);
                pOa = (PVBOXWDDM_OPENALLOCATION)vboxWddmMemAllocZero(sizeof (VBOXWDDM_OPENALLOCATION));
                if (!pOa)
                {
                    WARN(("failed to allocation alloc info"));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                pOa->hAllocation = pInfo->hAllocation;
                pOa->pAllocation = pAllocation;
                pOa->pDevice = pDevice;
                pOa->cOpens = 1;

                PVBOXWDDM_OPENALLOCATION pConcurrentOa;
                KeAcquireSpinLock(&pAllocation->OpenLock, &OldIrql);
                pConcurrentOa = VBoxWddmOaSearchLocked(pDevice, pAllocation);
                if (!pConcurrentOa)
                    InsertHeadList(&pAllocation->OpenList, &pOa->ListEntry);
                else
                    ++pConcurrentOa->cOpens;
                ++pAllocation->cOpens;
                KeReleaseSpinLock(&pAllocation->OpenLock, OldIrql);
                if (pConcurrentOa)
                {
                    vboxWddmMemFree(pOa);
                    pOa = pConcurrentOa;
                }
            }

            pInfo->hDeviceSpecificAllocation = pOa;
        }

        if (Status != STATUS_SUCCESS)
        {
            for (UINT j = 0; j < i; ++j)
            {
                DXGK_OPENALLOCATIONINFO* pInfo2Free = &pOpenAllocation->pOpenAllocation[j];
                PVBOXWDDM_OPENALLOCATION pOa2Free = (PVBOXWDDM_OPENALLOCATION)pInfo2Free->hDeviceSpecificAllocation;
                VBoxWddmOaRelease(pOa2Free);
            }
        }
    }
    LOGF(("LEAVE, hDevice(0x%x)", hDevice));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiCloseAllocation(
    CONST HANDLE  hDevice,
    CONST DXGKARG_CLOSEALLOCATION*  pCloseAllocation)
{
    RT_NOREF(hDevice);
    /* DxgkDdiCloseAllocation should be made pageable. */
    PAGED_CODE();

    LOGF(("ENTER, hDevice(0x%x)", hDevice));

    vboxVDbgBreakFv();

    for (UINT i = 0; i < pCloseAllocation->NumAllocations; ++i)
    {
        PVBOXWDDM_OPENALLOCATION pOa2Free = (PVBOXWDDM_OPENALLOCATION)pCloseAllocation->pOpenHandleList[i];
        PVBOXWDDM_ALLOCATION pAllocation = pOa2Free->pAllocation;
        Assert(pAllocation->cShRcRefs >= pOa2Free->cShRcRefs);
        pAllocation->cShRcRefs -= pOa2Free->cShRcRefs;
        VBoxWddmOaRelease(pOa2Free);
    }

    LOGF(("LEAVE, hDevice(0x%x)", hDevice));

    return STATUS_SUCCESS;
}

#ifdef VBOX_WITH_CROGL
static NTSTATUS
APIENTRY
DxgkDdiRenderNew(
    CONST HANDLE  hContext,
    DXGKARG_RENDER  *pRender)
{
    RT_NOREF(hContext);
//    LOGF(("ENTER, hContext(0x%x)", hContext));
    vboxVDbgBreakF();

    if (pRender->DmaBufferPrivateDataSize < sizeof (VBOXCMDVBVA_HDR))
    {
        WARN(("pRender->DmaBufferPrivateDataSize(%d) < sizeof VBOXCMDVBVA_HDR (%d)",
                pRender->DmaBufferPrivateDataSize , sizeof (VBOXCMDVBVA_HDR)));
        /** @todo can this actually happen? what status to return? */
        return STATUS_INVALID_PARAMETER;
    }
    if (pRender->CommandLength < sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR))
    {
        WARN(("Present->DmaBufferPrivateDataSize(%d) < sizeof VBOXWDDM_DMA_PRIVATEDATA_BASEHDR (%d)",
                pRender->DmaBufferPrivateDataSize , sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR)));
        return STATUS_INVALID_PARAMETER;
    }
    if (pRender->PatchLocationListOutSize < pRender->AllocationListSize)
    {
        WARN(("pRender->PatchLocationListOutSize(%d) < pRender->AllocationListSize(%d)",
                pRender->PatchLocationListOutSize, pRender->AllocationListSize));
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS Status = STATUS_SUCCESS;

    __try
    {
        PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR pInputHdr = (PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR)pRender->pCommand;

        uint32_t cbBuffer = 0;
        uint32_t cbPrivateData = 0;
        uint32_t cbCmd = 0;
        VBOXCMDVBVA_HDR* pCmd = (VBOXCMDVBVA_HDR*)pRender->pDmaBufferPrivateData;

        switch (pInputHdr->enmCmd)
        {
            case VBOXVDMACMD_TYPE_CHROMIUM_CMD:
            {
                if (pRender->AllocationListSize >= (UINT32_MAX - RT_OFFSETOF(VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD, aBufInfos))/ RT_SIZEOFMEMB(VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD, aBufInfos[0]))
                {
                    WARN(("Invalid AllocationListSize %d", pRender->AllocationListSize));
                    return STATUS_INVALID_PARAMETER;
                }

                if (pRender->CommandLength != RT_OFFSETOF(VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD, aBufInfos[pRender->AllocationListSize]))
                {
                    WARN(("pRender->CommandLength (%d) != RT_OFFSETOF(VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD, aBufInfos[pRender->AllocationListSize](%d)",
                            pRender->CommandLength, RT_OFFSETOF(VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD, aBufInfos[pRender->AllocationListSize])));
                    return STATUS_INVALID_PARAMETER;
                }

                if (pRender->AllocationListSize >= (UINT32_MAX - RT_OFFSETOF(VBOXCMDVBVA_CRCMD, Cmd.aBuffers))/ RT_SIZEOFMEMB(VBOXCMDVBVA_CRCMD, Cmd.aBuffers[0]))
                {
                    WARN(("Invalid AllocationListSize %d", pRender->AllocationListSize));
                    return STATUS_INVALID_PARAMETER;
                }

                cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
                cbPrivateData = RT_OFFSETOF(VBOXCMDVBVA_CRCMD, Cmd.aBuffers[pRender->AllocationListSize]);

                if (pRender->DmaBufferPrivateDataSize < cbPrivateData)
                {
                    WARN(("pRender->DmaBufferPrivateDataSize too small %d, requested %d", pRender->DmaBufferPrivateDataSize, cbPrivateData));
                    return STATUS_INVALID_PARAMETER;
                }

                if (pRender->DmaSize < cbBuffer)
                {
                    WARN(("dma buffer %d too small", pRender->DmaSize));
                    return STATUS_INVALID_PARAMETER;
                }

    //            Assert(pRender->PatchLocationListOutSize == pRender->AllocationListSize);

                if (pRender->PatchLocationListOutSize < pRender->AllocationListSize)
                {
                    WARN(("pRender->PatchLocationListOutSize too small %d, requested %d", pRender->PatchLocationListOutSize, pRender->AllocationListSize));
                    return STATUS_INVALID_PARAMETER;
                }

                PVBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD pUmCmd = (PVBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD)pInputHdr;
                VBOXCMDVBVA_CRCMD* pChromiumCmd = (VBOXCMDVBVA_CRCMD*)pRender->pDmaBufferPrivateData;

                pChromiumCmd->Hdr.u8OpCode = VBOXCMDVBVA_OPTYPE_CRCMD;
                pChromiumCmd->Hdr.u8Flags = 0;
                pChromiumCmd->Cmd.cBuffers = pRender->AllocationListSize;

                DXGK_ALLOCATIONLIST *pAllocationList = pRender->pAllocationList;
                VBOXCMDVBVA_CRCMD_BUFFER *pSubmInfo = pChromiumCmd->Cmd.aBuffers;
                PVBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO pSubmUmInfo = pUmCmd->aBufInfos;

                for (UINT i = 0; i < pRender->AllocationListSize; ++i, ++pRender->pPatchLocationListOut, ++pAllocationList, ++pSubmInfo, ++pSubmUmInfo)
                {
                    VBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO SubmUmInfo = *pSubmUmInfo;
                    D3DDDI_PATCHLOCATIONLIST* pPLL = pRender->pPatchLocationListOut;
                    PVBOXWDDM_ALLOCATION pAlloc = vboxWddmGetAllocationFromAllocList(pAllocationList);
                    if (SubmUmInfo.offData >= pAlloc->AllocData.SurfDesc.cbSize
                            || SubmUmInfo.cbData > pAlloc->AllocData.SurfDesc.cbSize
                            || SubmUmInfo.offData + SubmUmInfo.cbData > pAlloc->AllocData.SurfDesc.cbSize)
                    {
                        WARN(("invalid data"));
                        return STATUS_INVALID_PARAMETER;
                    }

                    memset(pPLL, 0, sizeof (*pPLL));

                    if (pAllocationList->SegmentId)
                        pSubmInfo->offBuffer = pAllocationList->PhysicalAddress.LowPart + SubmUmInfo.offData;

                    pSubmInfo->cbBuffer = SubmUmInfo.cbData;

                    pPLL->AllocationIndex = i;
                    pPLL->PatchOffset = RT_OFFSETOF(VBOXCMDVBVA_CRCMD, Cmd.aBuffers[i].offBuffer);
                    pPLL->AllocationOffset = SubmUmInfo.offData;
                }

                cbCmd = cbPrivateData;

                break;
            }
            case VBOXVDMACMD_TYPE_DMA_NOP:
            {
                cbPrivateData = sizeof (VBOXCMDVBVA_HDR);
                cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;

                if (pRender->DmaBufferPrivateDataSize < cbPrivateData)
                {
                    WARN(("pRender->DmaBufferPrivateDataSize too small %d, requested %d", pRender->DmaBufferPrivateDataSize, cbPrivateData));
                    return STATUS_INVALID_PARAMETER;
                }

                if (pRender->DmaSize < cbBuffer)
                {
                    WARN(("dma buffer %d too small", pRender->DmaSize));
                    return STATUS_INVALID_PARAMETER;
                }

                pCmd->u8OpCode = VBOXCMDVBVA_OPTYPE_NOPCMD;
                pCmd->u8Flags = 0;

                for (UINT i = 0; i < pRender->AllocationListSize; ++i, ++pRender->pPatchLocationListOut)
                {
                    D3DDDI_PATCHLOCATIONLIST* pPLL = pRender->pPatchLocationListOut;
                    memset(pPLL, 0, sizeof (*pPLL));
                    pPLL->AllocationIndex = i;
                    pPLL->PatchOffset = ~0UL;
                    pPLL->AllocationOffset = 0;
                }

                cbCmd = cbPrivateData;

                break;
            }
            default:
             {
                 WARN(("unsupported render command %d", pInputHdr->enmCmd));
                 return STATUS_INVALID_PARAMETER;
             }
        }

        Assert(cbPrivateData >= sizeof (VBOXCMDVBVA_HDR));
        pRender->pDmaBufferPrivateData = ((uint8_t*)pRender->pDmaBufferPrivateData) + cbPrivateData;
        pRender->pDmaBuffer = ((uint8_t*)pRender->pDmaBuffer) + cbBuffer;

        pCmd->u8State = VBOXCMDVBVA_STATE_SUBMITTED;
        pCmd->u2.complexCmdEl.u16CbCmdHost = cbPrivateData;
        pCmd->u2.complexCmdEl.u16CbCmdGuest = cbBuffer;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INVALID_PARAMETER;
        WARN(("invalid parameter"));
    }
//    LOGF(("LEAVE, hContext(0x%x)", hContext));

    return Status;
}
#endif

static void vboxWddmPatchLocationInit(D3DDDI_PATCHLOCATIONLIST *pPatchLocationListOut, UINT idx, UINT offPatch)
{
    memset(pPatchLocationListOut, 0, sizeof (*pPatchLocationListOut));
    pPatchLocationListOut->AllocationIndex = idx;
    pPatchLocationListOut->PatchOffset = offPatch;
}

static NTSTATUS
APIENTRY
DxgkDdiRenderLegacy(
    CONST HANDLE  hContext,
    DXGKARG_RENDER  *pRender)
{
    RT_NOREF(hContext);
//    LOGF(("ENTER, hContext(0x%x)", hContext));

    if (pRender->DmaBufferPrivateDataSize < sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR))
    {
        WARN(("Present->DmaBufferPrivateDataSize(%d) < sizeof VBOXWDDM_DMA_PRIVATEDATA_BASEHDR (%d)",
                pRender->DmaBufferPrivateDataSize , sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR)));
        return STATUS_INVALID_PARAMETER;
    }
    if (pRender->CommandLength < sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR))
    {
        WARN(("Present->DmaBufferPrivateDataSize(%d) < sizeof VBOXWDDM_DMA_PRIVATEDATA_BASEHDR (%d)",
                pRender->DmaBufferPrivateDataSize , sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR)));
        return STATUS_INVALID_PARAMETER;
    }
    if (pRender->DmaSize < pRender->CommandLength)
    {
        WARN(("pRender->DmaSize(%d) < pRender->CommandLength(%d)",
                pRender->DmaSize, pRender->CommandLength));
        return STATUS_INVALID_PARAMETER;
    }
    if (pRender->PatchLocationListOutSize < pRender->PatchLocationListInSize)
    {
        WARN(("pRender->PatchLocationListOutSize(%d) < pRender->PatchLocationListInSize(%d)",
                pRender->PatchLocationListOutSize, pRender->PatchLocationListInSize));
        return STATUS_INVALID_PARAMETER;
    }
    if (pRender->AllocationListSize != pRender->PatchLocationListInSize)
    {
        WARN(("pRender->AllocationListSize(%d) != pRender->PatchLocationListInSize(%d)",
                pRender->AllocationListSize, pRender->PatchLocationListInSize));
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS Status = STATUS_SUCCESS;

    __try
    {
        PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR pInputHdr = (PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR)pRender->pCommand;
        switch (pInputHdr->enmCmd)
        {
            case VBOXVDMACMD_TYPE_DMA_NOP:
            {
                PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR pPrivateData = (PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR)pRender->pDmaBufferPrivateData;
                pPrivateData->enmCmd = VBOXVDMACMD_TYPE_DMA_NOP;
                pRender->pDmaBufferPrivateData = (uint8_t*)pRender->pDmaBufferPrivateData + sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR);
                pRender->pDmaBuffer = ((uint8_t*)pRender->pDmaBuffer) + pRender->CommandLength;
                for (UINT i = 0; i < pRender->PatchLocationListInSize; ++i)
                {
                    UINT offPatch = i * 4;
                    if (offPatch + 4 > pRender->CommandLength)
                    {
                        WARN(("wrong offPatch"));
                        return STATUS_INVALID_PARAMETER;
                    }
                    if (offPatch != pRender->pPatchLocationListIn[i].PatchOffset)
                    {
                        WARN(("wrong PatchOffset"));
                        return STATUS_INVALID_PARAMETER;
                    }
                    if (i != pRender->pPatchLocationListIn[i].AllocationIndex)
                    {
                        WARN(("wrong AllocationIndex"));
                        return STATUS_INVALID_PARAMETER;
                    }
                    vboxWddmPatchLocationInit(&pRender->pPatchLocationListOut[i], i, offPatch);
                }
                break;
            }
            default:
            {
                WARN(("unsupported command %d", pInputHdr->enmCmd));
                return STATUS_INVALID_PARAMETER;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INVALID_PARAMETER;
        WARN(("invalid parameter"));
    }
//    LOGF(("LEAVE, hContext(0x%x)", hContext));

    return Status;
}

#define VBOXVDMACMD_DMA_PRESENT_BLT_MINSIZE() (VBOXVDMACMD_SIZE(VBOXVDMACMD_DMA_PRESENT_BLT))
#define VBOXVDMACMD_DMA_PRESENT_BLT_SIZE(_c) (VBOXVDMACMD_BODY_FIELD_OFFSET(UINT, VBOXVDMACMD_DMA_PRESENT_BLT, aDstSubRects[_c]))

#ifdef VBOX_WITH_VDMA
DECLINLINE(VOID) vboxWddmRectlFromRect(const RECT *pRect, PVBOXVDMA_RECTL pRectl)
{
    pRectl->left = (int16_t)pRect->left;
    pRectl->width = (uint16_t)(pRect->right - pRect->left);
    pRectl->top = (int16_t)pRect->top;
    pRectl->height = (uint16_t)(pRect->bottom - pRect->top);
}

DECLINLINE(VBOXVDMA_PIXEL_FORMAT) vboxWddmFromPixFormat(D3DDDIFORMAT format)
{
    return (VBOXVDMA_PIXEL_FORMAT)format;
}

DECLINLINE(VOID) vboxWddmSurfDescFromAllocation(PVBOXWDDM_ALLOCATION pAllocation, PVBOXVDMA_SURF_DESC pDesc)
{
    pDesc->width = pAllocation->AllocData.SurfDesc.width;
    pDesc->height = pAllocation->AllocData.SurfDesc.height;
    pDesc->format = vboxWddmFromPixFormat(pAllocation->AllocData.SurfDesc.format);
    pDesc->bpp = pAllocation->AllocData.SurfDesc.bpp;
    pDesc->pitch = pAllocation->AllocData.SurfDesc.pitch;
    pDesc->fFlags = 0;
}
#endif

DECLINLINE(BOOLEAN) vboxWddmPixFormatConversionSupported(D3DDDIFORMAT From, D3DDDIFORMAT To)
{
    Assert(From != D3DDDIFMT_UNKNOWN);
    Assert(To != D3DDDIFMT_UNKNOWN);
    Assert(From == To);
    return From == To;
}

#ifdef VBOX_WITH_CROGL

DECLINLINE(void) VBoxCVDdiFillAllocDescHostID(VBOXCMDVBVA_ALLOCDESC *pDesc, const VBOXWDDM_ALLOCATION *pAlloc)
{
    pDesc->Info.u.id = pAlloc->AllocData.hostID;
    /* we do not care about wdth and height, zero them up though */
    pDesc->u16Width = 0;
    pDesc->u16Height = 0;
}

DECLINLINE(void) VBoxCVDdiFillAllocInfoOffVRAM(VBOXCMDVBVA_ALLOCINFO *pInfo, const DXGK_ALLOCATIONLIST *pList)
{
    Assert(!pList->PhysicalAddress.HighPart);
    pInfo->u.offVRAM = pList->PhysicalAddress.LowPart;
}

DECLINLINE(void) VBoxCVDdiFillAllocDescOffVRAM(VBOXCMDVBVA_ALLOCDESC *pDesc, const VBOXWDDM_ALLOCATION *pAlloc, const DXGK_ALLOCATIONLIST *pList)
{
    VBoxCVDdiFillAllocInfoOffVRAM(&pDesc->Info, pList);
    pDesc->u16Width = (uint16_t)pAlloc->AllocData.SurfDesc.width;
    pDesc->u16Height = (uint16_t)pAlloc->AllocData.SurfDesc.height;
}

static NTSTATUS vboxWddmCmCmdBltIdNotIdFill(VBOXCMDVBVA_BLT_HDR *pBltHdr, const VBOXWDDM_ALLOCATION *pIdAlloc, const VBOXWDDM_ALLOCATION *pAlloc, const DXGK_ALLOCATIONLIST *pList,
                            BOOLEAN fToId, uint32_t *poffPatch, uint32_t *poffRects)
{
    uint8_t fFlags;
    Assert(pIdAlloc->AllocData.hostID);
    Assert(!pAlloc->AllocData.hostID);

    D3DDDIFORMAT enmFormat = vboxWddmFmtNoAlphaFormat(pAlloc->AllocData.SurfDesc.format);
    if (enmFormat != D3DDDIFMT_X8R8G8B8)
    {
        WARN(("unsupported format"));
        return STATUS_INVALID_PARAMETER;
    }

    if (pIdAlloc->AllocData.SurfDesc.width == pAlloc->AllocData.SurfDesc.width
            && pIdAlloc->AllocData.SurfDesc.height == pAlloc->AllocData.SurfDesc.height)
    {
        fFlags = VBOXCMDVBVA_OPF_BLT_TYPE_OFFPRIMSZFMT_OR_ID | VBOXCMDVBVA_OPF_OPERAND2_ISID;
        VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID *pBlt = (VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID*)pBltHdr;
        VBoxCVDdiFillAllocInfoOffVRAM(&pBlt->alloc, pList);
        pBlt->id = pIdAlloc->AllocData.hostID;
        *poffPatch = RT_OFFSETOF(VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID, alloc.u.offVRAM);
        *poffRects = RT_OFFSETOF(VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID, aRects);
    }
    else
    {
        fFlags = VBOXCMDVBVA_OPF_BLT_TYPE_SAMEDIM_A8R8G8B8 | VBOXCMDVBVA_OPF_OPERAND2_ISID;
        VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8 *pBlt = (VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8*)pBltHdr;
        VBoxCVDdiFillAllocDescOffVRAM(&pBlt->alloc1, pAlloc, pList);
        pBlt->info2.u.id = pIdAlloc->AllocData.hostID;
        *poffPatch = RT_OFFSETOF(VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8, alloc1.Info.u.offVRAM);
        *poffRects = RT_OFFSETOF(VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8, aRects);
    }

    if (fToId)
        fFlags |= VBOXCMDVBVA_OPF_BLT_DIR_IN_2;

    pBltHdr->Hdr.u8Flags |= fFlags;
    return STATUS_SUCCESS;
}

static NTSTATUS vboxWddmCmCmdBltNotIdNotIdFill(VBOXCMDVBVA_BLT_HDR *pBltHdr, const VBOXWDDM_ALLOCATION *pSrcAlloc, const DXGK_ALLOCATIONLIST *pSrcList,
        const VBOXWDDM_ALLOCATION *pDstAlloc, const DXGK_ALLOCATIONLIST *pDstList,
                            uint32_t *poffSrcPatch, uint32_t *poffDstPatch, uint32_t *poffRects)
{
    if (pDstAlloc->AllocData.SurfDesc.width == pSrcAlloc->AllocData.SurfDesc.width
            && pDstAlloc->AllocData.SurfDesc.height == pSrcAlloc->AllocData.SurfDesc.height)
    {
        pBltHdr->Hdr.u8Flags |= VBOXCMDVBVA_OPF_BLT_TYPE_SAMEDIM_A8R8G8B8;
        VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8 *pBlt = (VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8*)pBltHdr;
        VBoxCVDdiFillAllocDescOffVRAM(&pBlt->alloc1, pDstAlloc, pDstList);
        VBoxCVDdiFillAllocInfoOffVRAM(&pBlt->info2, pSrcList);
        *poffDstPatch = RT_OFFSETOF(VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8, alloc1.Info.u.offVRAM);
        *poffSrcPatch = RT_OFFSETOF(VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8, info2.u.offVRAM);
        *poffRects = RT_OFFSETOF(VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8, aRects);
    }
    else
    {
        pBltHdr->Hdr.u8Flags |= VBOXCMDVBVA_OPF_BLT_TYPE_GENERIC_A8R8G8B8;
        VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8 *pBlt = (VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8*)pBltHdr;
        VBoxCVDdiFillAllocDescOffVRAM(&pBlt->alloc1, pDstAlloc, pDstList);
        VBoxCVDdiFillAllocDescOffVRAM(&pBlt->alloc2, pSrcAlloc, pSrcList);
        *poffDstPatch = RT_OFFSETOF(VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8, alloc1.Info.u.offVRAM);
        *poffSrcPatch = RT_OFFSETOF(VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8, alloc2.Info.u.offVRAM);
        *poffRects = RT_OFFSETOF(VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8, aRects);
    }
    return STATUS_SUCCESS;
}
/**
 * DxgkDdiPresent
 */
static NTSTATUS
APIENTRY
DxgkDdiPresentNew(
    CONST HANDLE  hContext,
    DXGKARG_PRESENT  *pPresent)
{
    RT_NOREF(hContext);
    PAGED_CODE();

//    LOGF(("ENTER, hContext(0x%x)", hContext));

    vboxVDbgBreakFv();

#ifdef VBOX_STRICT
    PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)hContext;
    PVBOXWDDM_DEVICE pDevice = pContext->pDevice;
    PVBOXMP_DEVEXT pDevExt = pDevice->pAdapter;
#endif
    uint32_t cbBuffer = 0;
    uint32_t cbPrivateData = 0;

    if (pPresent->DmaBufferPrivateDataSize < sizeof (VBOXCMDVBVA_HDR))
    {
        WARN(("Present->DmaBufferPrivateDataSize(%d) < sizeof VBOXCMDVBVA_HDR (%d)", pPresent->DmaBufferPrivateDataSize , sizeof (VBOXCMDVBVA_HDR)));
        /** @todo can this actually happen? what status to return? */
        return STATUS_INVALID_PARAMETER;
    }

    VBOXCMDVBVA_HDR* pHdr = (VBOXCMDVBVA_HDR*)pPresent->pDmaBufferPrivateData;

    UINT u32SrcPatch = ~0UL;
    UINT u32DstPatch = ~0UL;
    BOOLEAN fPatchSrc = false;
    BOOLEAN fPatchDst = false;
    VBOXCMDVBVA_RECT *paRects = NULL;

    if (pPresent->DmaSize < VBOXWDDM_DUMMY_DMABUFFER_SIZE)
    {
        WARN(("Present->DmaSize(%d) < VBOXWDDM_DUMMY_DMABUFFER_SIZE (%d)", pPresent->DmaSize , VBOXWDDM_DUMMY_DMABUFFER_SIZE));
        /** @todo can this actually happen? what status to return? */
        return STATUS_INVALID_PARAMETER;
    }

#ifdef VBOX_WDDM_DUMP_REGIONS_ON_PRESENT
    LogRel(("%s: [%ld, %ld, %ld, %ld] -> [%ld, %ld, %ld, %ld] (SubRectCnt=%u)\n",
        pPresent->Flags.Blt ? "Blt" : (pPresent->Flags.Flip ? "Flip" : (pPresent->Flags.ColorFill ? "ColorFill" : "Unknown OP")),
        pPresent->SrcRect.left, pPresent->SrcRect.top, pPresent->SrcRect.right, pPresent->SrcRect.bottom,
        pPresent->DstRect.left, pPresent->DstRect.top, pPresent->DstRect.right, pPresent->DstRect.bottom,
        pPresent->SubRectCnt));
    for (unsigned int i = 0; i < pPresent->SubRectCnt; i++)
        LogRel(("\tsub#%u = [%ld, %ld, %ld, %ld]\n", i, pPresent->pDstSubRects[i].left, pPresent->pDstSubRects[i].top, pPresent->pDstSubRects[i].right, pPresent->pDstSubRects[i].bottom));
#endif

    if (pPresent->Flags.Blt)
    {
        Assert(pPresent->Flags.Value == 1); /* only Blt is set, we do not support anything else for now */
        DXGK_ALLOCATIONLIST *pSrc =  &pPresent->pAllocationList[DXGK_PRESENT_SOURCE_INDEX];
        DXGK_ALLOCATIONLIST *pDst =  &pPresent->pAllocationList[DXGK_PRESENT_DESTINATION_INDEX];
        PVBOXWDDM_ALLOCATION pSrcAlloc = vboxWddmGetAllocationFromAllocList(pSrc);
        if (!pSrcAlloc)
        {
            /* this should not happen actually */
            WARN(("failed to get Src Allocation info for hDeviceSpecificAllocation(0x%x)",pSrc->hDeviceSpecificAllocation));
            return STATUS_INVALID_HANDLE;
        }

        PVBOXWDDM_ALLOCATION pDstAlloc = vboxWddmGetAllocationFromAllocList(pDst);
        if (!pDstAlloc)
        {
            /* this should not happen actually */
            WARN(("failed to get Dst Allocation info for hDeviceSpecificAllocation(0x%x)",pDst->hDeviceSpecificAllocation));
            return STATUS_INVALID_HANDLE;
        }

        fPatchSrc = TRUE;
        fPatchDst = TRUE;

        BOOLEAN fDstPrimary = (!pDstAlloc->AllocData.hostID
                && pDstAlloc->enmType == VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE
                && pDstAlloc->bAssigned);
        BOOLEAN fSrcPrimary = (!pSrcAlloc->AllocData.hostID
                && pSrcAlloc->enmType == VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE
                && pSrcAlloc->bAssigned);

        pHdr->u8OpCode = VBOXCMDVBVA_OPTYPE_BLT;
        pHdr->u8Flags = 0;

        VBOXCMDVBVA_BLT_HDR *pBltHdr = (VBOXCMDVBVA_BLT_HDR*)pHdr;
        pBltHdr->Pos.x = (int16_t)(pPresent->DstRect.left - pPresent->SrcRect.left);
        pBltHdr->Pos.y = (int16_t)(pPresent->DstRect.top - pPresent->SrcRect.top);

        if (pPresent->DmaBufferPrivateDataSize < VBOXCMDVBVA_SIZEOF_BLTSTRUCT_MAX)
        {
            WARN(("Present->DmaBufferPrivateDataSize(%d) < (%d)", pPresent->DmaBufferPrivateDataSize , VBOXCMDVBVA_SIZEOF_BLTSTRUCT_MAX));
            /** @todo can this actually happen? what status to return? */
            return STATUS_INVALID_PARAMETER;
        }

        if (pSrcAlloc->AllocData.hostID)
        {
            if (pDstAlloc->AllocData.hostID)
            {
                VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID *pBlt = (VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID*)pBltHdr;
                pHdr->u8Flags |= VBOXCMDVBVA_OPF_BLT_TYPE_OFFPRIMSZFMT_OR_ID | VBOXCMDVBVA_OPF_OPERAND1_ISID | VBOXCMDVBVA_OPF_OPERAND2_ISID;
                pBlt->id = pDstAlloc->AllocData.hostID;
                pBlt->alloc.u.id = pSrcAlloc->AllocData.hostID;
                cbPrivateData = RT_OFFSETOF(VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID, aRects);
            }
            else
            {
                NTSTATUS Status = vboxWddmCmCmdBltIdNotIdFill(pBltHdr, pSrcAlloc, pDstAlloc, pDst, FALSE, &u32DstPatch, &cbPrivateData);
                if (!NT_SUCCESS(Status))
                {
                    WARN(("vboxWddmCmCmdBltIdNotIdFill failed, %#x", Status));
                    return Status;
                }
            }
        }
        else
        {
            if (pDstAlloc->AllocData.hostID)
            {
                NTSTATUS Status = vboxWddmCmCmdBltIdNotIdFill(pBltHdr, pDstAlloc, pSrcAlloc, pSrc, TRUE, &u32SrcPatch, &cbPrivateData);
                if (!NT_SUCCESS(Status))
                {
                    WARN(("vboxWddmCmCmdBltIdNotIdFill failed, %#x", Status));
                    return Status;
                }
            }
            else
            {
                vboxWddmCmCmdBltNotIdNotIdFill(pBltHdr, pSrcAlloc, pSrc, pDstAlloc, pDst, &u32SrcPatch, &u32DstPatch, &cbPrivateData);
            }
        }

        if (fDstPrimary)
            pBltHdr->Hdr.u.u8PrimaryID = (uint8_t)pDstAlloc->AllocData.SurfDesc.VidPnSourceId;
        else if (fSrcPrimary)
        {
            pBltHdr->Hdr.u8Flags |= VBOXCMDVBVA_OPF_PRIMARY_HINT_SRC;
            pBltHdr->Hdr.u.u8PrimaryID = (uint8_t)pSrcAlloc->AllocData.SurfDesc.VidPnSourceId;
        }
        else
            pBltHdr->Hdr.u.u8PrimaryID = 0xff;

        paRects = (VBOXCMDVBVA_RECT*)(((uint8_t*)pPresent->pDmaBufferPrivateData) + cbPrivateData);
        cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
    }
    else if (pPresent->Flags.Flip)
    {
        if (pPresent->DmaBufferPrivateDataSize < sizeof (VBOXCMDVBVA_FLIP))
        {
            WARN(("Present->DmaBufferPrivateDataSize(%d) < sizeof VBOXCMDVBVA_FLIP (%d)", pPresent->DmaBufferPrivateDataSize , sizeof (VBOXCMDVBVA_FLIP)));
            /** @todo can this actually happen? what status to return? */
            return STATUS_INVALID_PARAMETER;
        }

        fPatchSrc = TRUE;

        Assert(pPresent->Flags.Value == 4); /* only Blt is set, we do not support anything else for now */
        Assert(pContext->enmType == VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D);
        DXGK_ALLOCATIONLIST *pSrc =  &pPresent->pAllocationList[DXGK_PRESENT_SOURCE_INDEX];
        PVBOXWDDM_ALLOCATION pSrcAlloc = vboxWddmGetAllocationFromAllocList(pSrc);

        if (!pSrcAlloc)
        {
            /* this should not happen actually */
            WARN(("failed to get pSrc Allocation info for hDeviceSpecificAllocation(0x%x)",pSrc->hDeviceSpecificAllocation));
            return STATUS_INVALID_HANDLE;
        }

        Assert(pDevExt->cContexts3D);
        pHdr->u8OpCode = VBOXCMDVBVA_OPTYPE_FLIP;
        Assert((UINT)pSrcAlloc->AllocData.SurfDesc.VidPnSourceId < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
        pHdr->u.u8PrimaryID = pSrcAlloc->AllocData.SurfDesc.VidPnSourceId;
        VBOXCMDVBVA_FLIP *pFlip = (VBOXCMDVBVA_FLIP*)pHdr;

        if (pSrcAlloc->AllocData.hostID)
        {
            pHdr->u8Flags = VBOXCMDVBVA_OPF_OPERAND1_ISID;
            pFlip->src.u.id = pSrcAlloc->AllocData.hostID;
        }
        else
        {
            WARN(("VBoxCVDdiFillAllocInfo reported no host id for flip!"));
            pHdr->u8Flags = 0;
            VBoxCVDdiFillAllocInfoOffVRAM(&pFlip->src, pSrc);
            u32SrcPatch = RT_OFFSETOF(VBOXCMDVBVA_FLIP, src.u.offVRAM);
        }

        cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
        paRects = pFlip->aRects;
        cbPrivateData = VBOXCMDVBVA_SIZEOF_FLIPSTRUCT_MIN;
    }
    else if (pPresent->Flags.ColorFill)
    {
#ifdef DEBUG_misha
        WARN(("test color fill!"));
#endif

        fPatchDst = TRUE;

        Assert(pContext->enmType == VBOXWDDM_CONTEXT_TYPE_CUSTOM_2D);
        Assert(pPresent->Flags.Value == 2); /* only ColorFill is set, we do not support anything else for now */
        DXGK_ALLOCATIONLIST *pDst =  &pPresent->pAllocationList[DXGK_PRESENT_DESTINATION_INDEX];
        PVBOXWDDM_ALLOCATION pDstAlloc = vboxWddmGetAllocationFromAllocList(pDst);
        if (!pDstAlloc)
        {
            /* this should not happen actually */
            WARN(("failed to get pDst Allocation info for hDeviceSpecificAllocation(0x%x)",pDst->hDeviceSpecificAllocation));
            return STATUS_INVALID_HANDLE;
        }

        if (pDstAlloc->AllocData.hostID)
        {
            WARN(("color fill present for texid not supported"));
            pHdr->u8OpCode = VBOXCMDVBVA_OPTYPE_NOPCMD;
            pHdr->u8Flags = 0;
            pHdr->u8State = VBOXCMDVBVA_STATE_SUBMITTED;
            cbPrivateData = sizeof (VBOXCMDVBVA_HDR);
            cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
        }
        else
        {
            BOOLEAN fDstPrimary = (!pDstAlloc->AllocData.hostID
                    && pDstAlloc->enmType == VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE
                    && pDstAlloc->bAssigned);

            if (pPresent->DmaBufferPrivateDataSize < VBOXCMDVBVA_SIZEOF_CLRFILLSTRUCT_MAX)
            {
                WARN(("Present->DmaBufferPrivateDataSize(%d) < VBOXCMDVBVA_SIZEOF_CLRFILLSTRUCT_MAX (%d)", pPresent->DmaBufferPrivateDataSize , VBOXCMDVBVA_SIZEOF_CLRFILLSTRUCT_MAX));
                /** @todo can this actually happen? what status to return? */
                return STATUS_INVALID_PARAMETER;
            }

            VBOXCMDVBVA_CLRFILL_HDR *pClrFillHdr = (VBOXCMDVBVA_CLRFILL_HDR*)pHdr;

            pClrFillHdr->Hdr.u8OpCode = VBOXCMDVBVA_OPTYPE_CLRFILL;
            pClrFillHdr->u32Color = pPresent->Color;

            pHdr->u8Flags = VBOXCMDVBVA_OPF_CLRFILL_TYPE_GENERIC_A8R8G8B8;
            pHdr->u.u8PrimaryID = 0;

            VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8 *pCFill = (VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8*)pHdr;
            VBoxCVDdiFillAllocInfoOffVRAM(&pCFill->dst.Info, pDst);
            pCFill->dst.u16Width = (uint16_t)pDstAlloc->AllocData.SurfDesc.width;
            pCFill->dst.u16Height = (uint16_t)pDstAlloc->AllocData.SurfDesc.height;
            u32DstPatch = RT_OFFSETOF(VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8, dst.Info.u.offVRAM);
            paRects = pCFill->aRects;
            cbPrivateData = RT_OFFSETOF(VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8, aRects);

            if (fDstPrimary)
                pCFill->Hdr.Hdr.u.u8PrimaryID = (uint8_t)pDstAlloc->AllocData.SurfDesc.VidPnSourceId;
            else
                pCFill->Hdr.Hdr.u.u8PrimaryID = 0xff;

            cbBuffer = VBOXWDDM_DUMMY_DMABUFFER_SIZE;
        }
    }
    else
    {
        WARN(("cmd NOT IMPLEMENTED!! Flags(0x%x)", pPresent->Flags.Value));
        return STATUS_NOT_SUPPORTED;
    }

    if (paRects)
    {
        uint32_t cbMaxRects = pPresent->DmaBufferPrivateDataSize - cbPrivateData;
        UINT iStartRect = pPresent->MultipassOffset;
        UINT cMaxRects = cbMaxRects / sizeof (VBOXCMDVBVA_RECT);
        Assert(pPresent->SubRectCnt > iStartRect);
        UINT cRects = pPresent->SubRectCnt - iStartRect;
        if (cRects > cMaxRects)
        {
            pPresent->MultipassOffset += cMaxRects;
            cRects = cMaxRects;
        }
        else
            pPresent->MultipassOffset = 0;

        Assert(cRects);
        const RECT *paDstSubRects = &pPresent->pDstSubRects[iStartRect];
        VBoxCVDdiPackRects(paRects, paDstSubRects, cRects);
        cbPrivateData += (cRects * sizeof (VBOXCMDVBVA_RECT));
    }

    if (fPatchSrc)
    {
        memset(pPresent->pPatchLocationListOut, 0, sizeof (D3DDDI_PATCHLOCATIONLIST));
        pPresent->pPatchLocationListOut->PatchOffset = u32SrcPatch;
        pPresent->pPatchLocationListOut->AllocationIndex = DXGK_PRESENT_SOURCE_INDEX;
        ++pPresent->pPatchLocationListOut;
    }

    if (fPatchDst)
    {
        memset(pPresent->pPatchLocationListOut, 0, sizeof (D3DDDI_PATCHLOCATIONLIST));
        pPresent->pPatchLocationListOut->PatchOffset = u32DstPatch;
        pPresent->pPatchLocationListOut->AllocationIndex = DXGK_PRESENT_DESTINATION_INDEX;
        ++pPresent->pPatchLocationListOut;
    }

    pHdr->u8State = VBOXCMDVBVA_STATE_SUBMITTED;

    pHdr->u2.complexCmdEl.u16CbCmdHost = cbPrivateData;
    pHdr->u2.complexCmdEl.u16CbCmdGuest = cbBuffer;

    Assert(cbBuffer);
    Assert(cbPrivateData);
    Assert(cbPrivateData >= sizeof (VBOXCMDVBVA_HDR));
    pPresent->pDmaBuffer = ((uint8_t*)pPresent->pDmaBuffer) + cbBuffer;
    pPresent->pDmaBufferPrivateData = ((uint8_t*)pPresent->pDmaBufferPrivateData) + cbPrivateData;

    if (pPresent->MultipassOffset)
        return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
    return STATUS_SUCCESS;
}
#endif

/**
 * DxgkDdiPresent
 */
static NTSTATUS
APIENTRY
DxgkDdiPresentLegacy(
    CONST HANDLE  hContext,
    DXGKARG_PRESENT  *pPresent)
{
    RT_NOREF(hContext);
    PAGED_CODE();

//    LOGF(("ENTER, hContext(0x%x)", hContext));

    vboxVDbgBreakFv();

    NTSTATUS Status = STATUS_SUCCESS;
#ifdef VBOX_STRICT
    PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)hContext;
    PVBOXWDDM_DEVICE pDevice = pContext->pDevice;
    PVBOXMP_DEVEXT pDevExt = pDevice->pAdapter;
#endif

    Assert(pPresent->DmaBufferPrivateDataSize >= sizeof (VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR));
    if (pPresent->DmaBufferPrivateDataSize < sizeof (VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR))
    {
        LOGREL(("Present->DmaBufferPrivateDataSize(%d) < sizeof VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR (%d)", pPresent->DmaBufferPrivateDataSize , sizeof (VBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR)));
        /** @todo can this actually happen? what status tu return? */
        return STATUS_INVALID_PARAMETER;
    }

    PVBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR pPrivateData = (PVBOXWDDM_DMA_PRIVATEDATA_PRESENTHDR)pPresent->pDmaBufferPrivateData;
    pPrivateData->BaseHdr.fFlags.Value = 0;
    /*uint32_t cContexts2D = ASMAtomicReadU32(&pDevExt->cContexts2D); - unused */

    if (pPresent->Flags.Blt)
    {
        Assert(pPresent->Flags.Value == 1); /* only Blt is set, we do not support anything else for now */
        DXGK_ALLOCATIONLIST *pSrc =  &pPresent->pAllocationList[DXGK_PRESENT_SOURCE_INDEX];
        DXGK_ALLOCATIONLIST *pDst =  &pPresent->pAllocationList[DXGK_PRESENT_DESTINATION_INDEX];
        PVBOXWDDM_ALLOCATION pSrcAlloc = vboxWddmGetAllocationFromAllocList(pSrc);
        if (!pSrcAlloc)
        {
            /* this should not happen actually */
            WARN(("failed to get Src Allocation info for hDeviceSpecificAllocation(0x%x)",pSrc->hDeviceSpecificAllocation));
            Status = STATUS_INVALID_HANDLE;
            goto done;
        }

        PVBOXWDDM_ALLOCATION pDstAlloc = vboxWddmGetAllocationFromAllocList(pDst);
        if (!pDstAlloc)
        {
            /* this should not happen actually */
            WARN(("failed to get Dst Allocation info for hDeviceSpecificAllocation(0x%x)",pDst->hDeviceSpecificAllocation));
            Status = STATUS_INVALID_HANDLE;
            goto done;
        }


        UINT cbCmd = pPresent->DmaBufferPrivateDataSize;
        pPrivateData->BaseHdr.enmCmd = VBOXVDMACMD_TYPE_DMA_PRESENT_BLT;

        PVBOXWDDM_DMA_PRIVATEDATA_BLT pBlt = (PVBOXWDDM_DMA_PRIVATEDATA_BLT)pPrivateData;

        vboxWddmPopulateDmaAllocInfo(&pBlt->Blt.SrcAlloc, pSrcAlloc, pSrc);
        vboxWddmPopulateDmaAllocInfo(&pBlt->Blt.DstAlloc, pDstAlloc, pDst);

        ASSERT_WARN(!pSrcAlloc->fRcFlags.SharedResource, ("Shared Allocatoin used in Present!"));

        pBlt->Blt.SrcRect = pPresent->SrcRect;
        pBlt->Blt.DstRects.ContextRect = pPresent->DstRect;
        pBlt->Blt.DstRects.UpdateRects.cRects = 0;
        UINT cbHead = RT_OFFSETOF(VBOXWDDM_DMA_PRIVATEDATA_BLT, Blt.DstRects.UpdateRects.aRects[0]);
        Assert(pPresent->SubRectCnt > pPresent->MultipassOffset);
        UINT cbRects = (pPresent->SubRectCnt - pPresent->MultipassOffset) * sizeof (RECT);
        pPresent->pDmaBuffer = ((uint8_t*)pPresent->pDmaBuffer) + VBOXWDDM_DUMMY_DMABUFFER_SIZE;
        Assert(pPresent->DmaSize >= VBOXWDDM_DUMMY_DMABUFFER_SIZE);
        cbCmd -= cbHead;
        Assert(cbCmd < UINT32_MAX/2);
        Assert(cbCmd > sizeof (RECT));
        if (cbCmd >= cbRects)
        {
            cbCmd -= cbRects;
            memcpy(&pBlt->Blt.DstRects.UpdateRects.aRects[0], &pPresent->pDstSubRects[pPresent->MultipassOffset], cbRects);
            pBlt->Blt.DstRects.UpdateRects.cRects += cbRects/sizeof (RECT);

            pPresent->pDmaBufferPrivateData = (uint8_t*)pPresent->pDmaBufferPrivateData + cbHead + cbRects;
        }
        else
        {
            UINT cbFitingRects = (cbCmd/sizeof (RECT)) * sizeof (RECT);
            Assert(cbFitingRects);
            memcpy(&pBlt->Blt.DstRects.UpdateRects.aRects[0], &pPresent->pDstSubRects[pPresent->MultipassOffset], cbFitingRects);
            cbCmd -= cbFitingRects;
            pPresent->MultipassOffset += cbFitingRects/sizeof (RECT);
            pBlt->Blt.DstRects.UpdateRects.cRects += cbFitingRects/sizeof (RECT);
            Assert(pPresent->SubRectCnt > pPresent->MultipassOffset);

            pPresent->pDmaBufferPrivateData = (uint8_t*)pPresent->pDmaBufferPrivateData + cbHead + cbFitingRects;
            Status = STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
        }

        memset(pPresent->pPatchLocationListOut, 0, 2*sizeof (D3DDDI_PATCHLOCATIONLIST));
        pPresent->pPatchLocationListOut->PatchOffset = 0;
        pPresent->pPatchLocationListOut->AllocationIndex = DXGK_PRESENT_SOURCE_INDEX;
        ++pPresent->pPatchLocationListOut;
        pPresent->pPatchLocationListOut->PatchOffset = 4;
        pPresent->pPatchLocationListOut->AllocationIndex = DXGK_PRESENT_DESTINATION_INDEX;
        ++pPresent->pPatchLocationListOut;
    }
    else if (pPresent->Flags.Flip)
    {
        Assert(pPresent->Flags.Value == 4); /* only Blt is set, we do not support anything else for now */
        Assert(pContext->enmType == VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D);
        DXGK_ALLOCATIONLIST *pSrc =  &pPresent->pAllocationList[DXGK_PRESENT_SOURCE_INDEX];
        PVBOXWDDM_ALLOCATION pSrcAlloc = vboxWddmGetAllocationFromAllocList(pSrc);

        if (!pSrcAlloc)
        {
            /* this should not happen actually */
            WARN(("failed to get pSrc Allocation info for hDeviceSpecificAllocation(0x%x)",pSrc->hDeviceSpecificAllocation));
            Status = STATUS_INVALID_HANDLE;
            goto done;
        }

        Assert(pDevExt->cContexts3D);
        pPrivateData->BaseHdr.enmCmd = VBOXVDMACMD_TYPE_DMA_PRESENT_FLIP;
        PVBOXWDDM_DMA_PRIVATEDATA_FLIP pFlip = (PVBOXWDDM_DMA_PRIVATEDATA_FLIP)pPrivateData;

        vboxWddmPopulateDmaAllocInfo(&pFlip->Flip.Alloc, pSrcAlloc, pSrc);

        UINT cbCmd = sizeof (VBOXWDDM_DMA_PRIVATEDATA_FLIP);
        pPresent->pDmaBufferPrivateData = (uint8_t*)pPresent->pDmaBufferPrivateData + cbCmd;
        pPresent->pDmaBuffer = ((uint8_t*)pPresent->pDmaBuffer) + VBOXWDDM_DUMMY_DMABUFFER_SIZE;
        Assert(pPresent->DmaSize >= VBOXWDDM_DUMMY_DMABUFFER_SIZE);

        memset(pPresent->pPatchLocationListOut, 0, sizeof (D3DDDI_PATCHLOCATIONLIST));
        pPresent->pPatchLocationListOut->PatchOffset = 0;
        pPresent->pPatchLocationListOut->AllocationIndex = DXGK_PRESENT_SOURCE_INDEX;
        ++pPresent->pPatchLocationListOut;
    }
    else if (pPresent->Flags.ColorFill)
    {
        Assert(pContext->enmType == VBOXWDDM_CONTEXT_TYPE_CUSTOM_2D);
        Assert(pPresent->Flags.Value == 2); /* only ColorFill is set, we do not support anything else for now */
        DXGK_ALLOCATIONLIST *pDst =  &pPresent->pAllocationList[DXGK_PRESENT_DESTINATION_INDEX];
        PVBOXWDDM_ALLOCATION pDstAlloc = vboxWddmGetAllocationFromAllocList(pDst);
        if (!pDstAlloc)
        {

            /* this should not happen actually */
            WARN(("failed to get pDst Allocation info for hDeviceSpecificAllocation(0x%x)",pDst->hDeviceSpecificAllocation));
            Status = STATUS_INVALID_HANDLE;
            goto done;
        }

        UINT cbCmd = pPresent->DmaBufferPrivateDataSize;
        pPrivateData->BaseHdr.enmCmd = VBOXVDMACMD_TYPE_DMA_PRESENT_CLRFILL;
        PVBOXWDDM_DMA_PRIVATEDATA_CLRFILL pCF = (PVBOXWDDM_DMA_PRIVATEDATA_CLRFILL)pPrivateData;

        vboxWddmPopulateDmaAllocInfo(&pCF->ClrFill.Alloc, pDstAlloc, pDst);

        pCF->ClrFill.Color = pPresent->Color;
        pCF->ClrFill.Rects.cRects = 0;
        UINT cbHead = RT_OFFSETOF(VBOXWDDM_DMA_PRIVATEDATA_CLRFILL, ClrFill.Rects.aRects[0]);
        Assert(pPresent->SubRectCnt > pPresent->MultipassOffset);
        UINT cbRects = (pPresent->SubRectCnt - pPresent->MultipassOffset) * sizeof (RECT);
        pPresent->pDmaBuffer = ((uint8_t*)pPresent->pDmaBuffer) + VBOXWDDM_DUMMY_DMABUFFER_SIZE;
        Assert(pPresent->DmaSize >= VBOXWDDM_DUMMY_DMABUFFER_SIZE);
        cbCmd -= cbHead;
        Assert(cbCmd < UINT32_MAX/2);
        Assert(cbCmd > sizeof (RECT));
        if (cbCmd >= cbRects)
        {
            cbCmd -= cbRects;
            memcpy(&pCF->ClrFill.Rects.aRects[pPresent->MultipassOffset], pPresent->pDstSubRects, cbRects);
            pCF->ClrFill.Rects.cRects += cbRects/sizeof (RECT);

            pPresent->pDmaBufferPrivateData = (uint8_t*)pPresent->pDmaBufferPrivateData + cbHead + cbRects;
        }
        else
        {
            UINT cbFitingRects = (cbCmd/sizeof (RECT)) * sizeof (RECT);
            Assert(cbFitingRects);
            memcpy(&pCF->ClrFill.Rects.aRects[0], pPresent->pDstSubRects, cbFitingRects);
            cbCmd -= cbFitingRects;
            pPresent->MultipassOffset += cbFitingRects/sizeof (RECT);
            pCF->ClrFill.Rects.cRects += cbFitingRects/sizeof (RECT);
            Assert(pPresent->SubRectCnt > pPresent->MultipassOffset);

            pPresent->pDmaBufferPrivateData = (uint8_t*)pPresent->pDmaBufferPrivateData + cbHead + cbFitingRects;
            Status = STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
        }

        memset(pPresent->pPatchLocationListOut, 0, sizeof (D3DDDI_PATCHLOCATIONLIST));
        pPresent->pPatchLocationListOut->PatchOffset = 0;
        pPresent->pPatchLocationListOut->AllocationIndex = DXGK_PRESENT_DESTINATION_INDEX;
        ++pPresent->pPatchLocationListOut;
    }
    else
    {
        WARN(("cmd NOT IMPLEMENTED!! Flags(0x%x)", pPresent->Flags.Value));
        Status = STATUS_NOT_SUPPORTED;
    }

done:
//    LOGF(("LEAVE, hContext(0x%x), Status(0x%x)", hContext, Status));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiUpdateOverlay(
    CONST HANDLE  hOverlay,
    CONST DXGKARG_UPDATEOVERLAY  *pUpdateOverlay)
{
    LOGF(("ENTER, hOverlay(0x%p)", hOverlay));

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_OVERLAY pOverlay = (PVBOXWDDM_OVERLAY)hOverlay;
    Assert(pOverlay);
    int rc = vboxVhwaHlpOverlayUpdate(pOverlay, &pUpdateOverlay->OverlayInfo);
    AssertRC(rc);
    if (RT_FAILURE(rc))
        Status = STATUS_UNSUCCESSFUL;

    LOGF(("LEAVE, hOverlay(0x%p)", hOverlay));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiFlipOverlay(
    CONST HANDLE  hOverlay,
    CONST DXGKARG_FLIPOVERLAY  *pFlipOverlay)
{
    LOGF(("ENTER, hOverlay(0x%p)", hOverlay));

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_OVERLAY pOverlay = (PVBOXWDDM_OVERLAY)hOverlay;
    Assert(pOverlay);
    int rc = vboxVhwaHlpOverlayFlip(pOverlay, pFlipOverlay);
    AssertRC(rc);
    if (RT_FAILURE(rc))
        Status = STATUS_UNSUCCESSFUL;

    LOGF(("LEAVE, hOverlay(0x%p)", hOverlay));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiDestroyOverlay(
    CONST HANDLE  hOverlay)
{
    LOGF(("ENTER, hOverlay(0x%p)", hOverlay));

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_OVERLAY pOverlay = (PVBOXWDDM_OVERLAY)hOverlay;
    Assert(pOverlay);
    int rc = vboxVhwaHlpOverlayDestroy(pOverlay);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
        vboxWddmMemFree(pOverlay);
    else
        Status = STATUS_UNSUCCESSFUL;

    LOGF(("LEAVE, hOverlay(0x%p)", hOverlay));

    return Status;
}

/**
 * DxgkDdiCreateContext
 */
NTSTATUS
APIENTRY
DxgkDdiCreateContext(
    CONST HANDLE  hDevice,
    DXGKARG_CREATECONTEXT  *pCreateContext)
{
    /* DxgkDdiCreateContext should be made pageable */
    PAGED_CODE();

    LOGF(("ENTER, hDevice(0x%x)", hDevice));

    vboxVDbgBreakFv();

    if (pCreateContext->NodeOrdinal >= VBOXWDDM_NUM_NODES)
    {
        WARN(("Invalid NodeOrdinal (%d), expected to be less that (%d)\n", pCreateContext->NodeOrdinal, VBOXWDDM_NUM_NODES));
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_DEVICE pDevice = (PVBOXWDDM_DEVICE)hDevice;
    PVBOXMP_DEVEXT pDevExt = pDevice->pAdapter;
    PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)vboxWddmMemAllocZero(sizeof (VBOXWDDM_CONTEXT));
    Assert(pContext);
    if (pContext)
    {
        pContext->pDevice = pDevice;
        pContext->hContext = pCreateContext->hContext;
        pContext->EngineAffinity = pCreateContext->EngineAffinity;
        pContext->NodeOrdinal = pCreateContext->NodeOrdinal;
        vboxVideoCmCtxInitEmpty(&pContext->CmContext);
        if (pCreateContext->Flags.SystemContext || pCreateContext->PrivateDriverDataSize == 0)
        {
            Assert(pCreateContext->PrivateDriverDataSize == 0);
            Assert(!pCreateContext->pPrivateDriverData);
            Assert(pCreateContext->Flags.Value <= 2); /* 2 is a GDI context in Win7 */
            pContext->enmType = VBOXWDDM_CONTEXT_TYPE_SYSTEM;
            for (int i = 0; i < VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++i)
            {
                vboxWddmDisplaySettingsCheckPos(pDevExt, i);
            }

#ifdef VBOX_WITH_CROGL
            if (!VBOXWDDM_IS_DISPLAYONLY() && pDevExt->f3DEnabled)
            {
                VBoxMpCrPackerInit(&pContext->CrPacker);
                int rc = VBoxMpCrCtlConConnect(pDevExt, &pDevExt->CrCtlCon, CR_PROTOCOL_VERSION_MAJOR, CR_PROTOCOL_VERSION_MINOR, &pContext->u32CrConClientID);
                if (!RT_SUCCESS(rc))
                    WARN(("VBoxMpCrCtlConConnect failed rc (%d), ignoring for system context", rc));
            }
#endif
            Status = STATUS_SUCCESS;
        }
        else
        {
            Assert(pCreateContext->Flags.Value == 0);
            Assert(pCreateContext->PrivateDriverDataSize == sizeof (VBOXWDDM_CREATECONTEXT_INFO));
            Assert(pCreateContext->pPrivateDriverData);
            if (pCreateContext->PrivateDriverDataSize == sizeof (VBOXWDDM_CREATECONTEXT_INFO))
            {
                PVBOXWDDM_CREATECONTEXT_INFO pInfo = (PVBOXWDDM_CREATECONTEXT_INFO)pCreateContext->pPrivateDriverData;
                switch (pInfo->enmType)
                {
#ifdef VBOX_WITH_CROGL
                    case VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D:
                    {
                        if (!pDevExt->fCmdVbvaEnabled)
                        {
                            Status = vboxVideoAMgrCtxCreate(&pDevExt->AllocMgr, &pContext->AllocContext);
                            if (!NT_SUCCESS(Status))
                                WARN(("vboxVideoAMgrCtxCreate failed %#x", Status));
                        }
                        else
                            Status = STATUS_SUCCESS;

                        if (Status == STATUS_SUCCESS)
                        {
                            Status = vboxWddmSwapchainCtxInit(pDevExt, pContext);
                            AssertNtStatusSuccess(Status);
                            if (Status == STATUS_SUCCESS)
                            {
                                pContext->enmType = VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D;
                                Status = vboxVideoCmCtxAdd(&pDevice->pAdapter->CmMgr, &pContext->CmContext, (HANDLE)pInfo->hUmEvent, pInfo->u64UmInfo);
                                AssertNtStatusSuccess(Status);
                                if (Status == STATUS_SUCCESS)
                                {
                                    if (pInfo->crVersionMajor || pInfo->crVersionMinor)
                                    {
                                        if (pDevExt->f3DEnabled)
                                        {
                                            int rc = VBoxMpCrCtlConConnect(pDevExt, &pDevExt->CrCtlCon,
                                                pInfo->crVersionMajor, pInfo->crVersionMinor,
                                                &pContext->u32CrConClientID);
                                            if (RT_SUCCESS(rc))
                                            {
                                                VBoxMpCrPackerInit(&pContext->CrPacker);
                                            }
                                            else
                                            {
                                                WARN(("VBoxMpCrCtlConConnect failed rc (%d)", rc));
                                                Status = STATUS_UNSUCCESSFUL;
                                            }
                                        }
                                        else
                                        {
                                            LOG(("3D Not Enabled, failing 3D context creation"));
                                            Status = STATUS_UNSUCCESSFUL;
                                        }
                                    }

                                    if (NT_SUCCESS(Status))
                                    {
                                        ASMAtomicIncU32(&pDevExt->cContexts3D);
                                        break;
                                    }
                                }

                                vboxWddmSwapchainCtxTerm(pDevExt, pContext);
                            }
                            vboxVideoAMgrCtxDestroy(&pContext->AllocContext);
                        }
                        break;
                    }
                    case VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_3D:
                    case VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_GL:
                    {
                        pContext->enmType = pInfo->enmType;
                        if (!pDevExt->fCmdVbvaEnabled)
                        {
                            Status = vboxVideoAMgrCtxCreate(&pDevExt->AllocMgr, &pContext->AllocContext);
                            if (!NT_SUCCESS(Status))
                                WARN(("vboxVideoAMgrCtxCreate failed %#x", Status));
                        }
                        else
                            Status = STATUS_SUCCESS;

                        if (Status == STATUS_SUCCESS)
                        {
                            if (pInfo->crVersionMajor || pInfo->crVersionMinor)
                            {
                                if (pDevExt->f3DEnabled)
                                {
                                    int rc = VBoxMpCrCtlConConnect(pDevExt, &pDevExt->CrCtlCon,
                                        pInfo->crVersionMajor, pInfo->crVersionMinor,
                                        &pContext->u32CrConClientID);
                                    if (!RT_SUCCESS(rc))
                                    {
                                        WARN(("VBoxMpCrCtlConConnect failed rc (%d)", rc));
                                        Status = STATUS_UNSUCCESSFUL;
                                    }
                                }
                                else
                                {
                                    LOG(("3D Not Enabled, failing 3D (hgsmi) context creation"));
                                    Status = STATUS_UNSUCCESSFUL;
                                }
                            }

                            if (NT_SUCCESS(Status))
                            {
                                ASMAtomicIncU32(&pDevExt->cContexts3D);
                                break;
                            }
                            vboxVideoAMgrCtxDestroy(&pContext->AllocContext);
                        }
                        break;
                    }
#endif
                    case VBOXWDDM_CONTEXT_TYPE_CUSTOM_2D:
                    {
                        pContext->enmType = pInfo->enmType;
                        ASMAtomicIncU32(&pDevExt->cContexts2D);
                        break;
                    }
                    case VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_RESIZE:
                    {
                        pContext->enmType = pInfo->enmType;
                        ASMAtomicIncU32(&pDevExt->cContextsDispIfResize);
                        break;
                    }
                    case VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_SEAMLESS:
                    {
                        pContext->enmType = pInfo->enmType;
                        Status = vboxVideoCmCtxAdd(&pDevice->pAdapter->SeamlessCtxMgr, &pContext->CmContext, (HANDLE)pInfo->hUmEvent, pInfo->u64UmInfo);
                        if (!NT_SUCCESS(Status))
                        {
                            WARN(("vboxVideoCmCtxAdd failed, Status 0x%x", Status));
                        }
                        break;
                    }
                    default:
                    {
                        WARN(("unsupported context type %d", pInfo->enmType));
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }
        }

        if (Status == STATUS_SUCCESS)
        {
            pCreateContext->hContext = pContext;
            pCreateContext->ContextInfo.DmaBufferSize = VBOXWDDM_C_DMA_BUFFER_SIZE;
            pCreateContext->ContextInfo.DmaBufferSegmentSet = 0;
            pCreateContext->ContextInfo.DmaBufferPrivateDataSize = VBOXWDDM_C_DMA_PRIVATEDATA_SIZE;
            pCreateContext->ContextInfo.AllocationListSize = VBOXWDDM_C_ALLOC_LIST_SIZE;
            pCreateContext->ContextInfo.PatchLocationListSize = VBOXWDDM_C_PATH_LOCATION_LIST_SIZE;
        //#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
        //# error port to Win7 DDI
        //    //pCreateContext->ContextInfo.DmaBufferAllocationGroup = ???;
        //#endif // DXGKDDI_INTERFACE_VERSION
        }
        else
            vboxWddmMemFree(pContext);
    }
    else
        Status = STATUS_NO_MEMORY;

    LOGF(("LEAVE, hDevice(0x%x)", hDevice));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiDestroyContext(
    CONST HANDLE  hContext)
{
    LOGF(("ENTER, hContext(0x%x)", hContext));
    vboxVDbgBreakFv();
    PVBOXWDDM_CONTEXT pContext = (PVBOXWDDM_CONTEXT)hContext;
    PVBOXMP_DEVEXT pDevExt = pContext->pDevice->pAdapter;
    NTSTATUS Status = STATUS_SUCCESS;

    switch(pContext->enmType)
    {
        case VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D:
        case VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_3D:
        case VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_GL:
        {
            uint32_t cContexts = ASMAtomicDecU32(&pDevExt->cContexts3D);
            Assert(cContexts < UINT32_MAX/2); NOREF(cContexts);
            break;
        }
        case VBOXWDDM_CONTEXT_TYPE_CUSTOM_2D:
        {
            uint32_t cContexts = ASMAtomicDecU32(&pDevExt->cContexts2D);
            Assert(cContexts < UINT32_MAX/2); NOREF(cContexts);
            break;
        }
        case VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_RESIZE:
        {
            uint32_t cContexts = ASMAtomicDecU32(&pDevExt->cContextsDispIfResize);
            Assert(cContexts < UINT32_MAX/2);
            if (!cContexts)
            {
                if (pDevExt->fDisableTargetUpdate)
                {
                    pDevExt->fDisableTargetUpdate = FALSE;
                    vboxWddmGhDisplayCheckSetInfoEx(pDevExt, true);
                }
            }
            break;
        }
        case VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_SEAMLESS:
        {
            Status = vboxVideoCmCtxRemove(&pContext->pDevice->pAdapter->SeamlessCtxMgr, &pContext->CmContext);
            if (!NT_SUCCESS(Status))
                WARN(("vboxVideoCmCtxRemove failed, Status 0x%x", Status));

            Assert(pContext->CmContext.pSession == NULL);
            break;
        }
        default:
            break;
    }

#ifdef VBOX_WITH_CROGL
    if (pContext->u32CrConClientID)
    {
        VBoxMpCrCtlConDisconnect(pDevExt, &pDevExt->CrCtlCon, pContext->u32CrConClientID);
    }
#endif

#ifdef VBOX_WITH_CROGL
    /* first terminate the swapchain, this will also ensure
     * all currently pending driver->user Cm commands
     * (i.e. visible regions commands) are completed */
    vboxWddmSwapchainCtxTerm(pDevExt, pContext);
#endif

    Status = vboxVideoAMgrCtxDestroy(&pContext->AllocContext);
    if (NT_SUCCESS(Status))
    {
        Status = vboxVideoCmCtxRemove(&pContext->pDevice->pAdapter->CmMgr, &pContext->CmContext);
        if (NT_SUCCESS(Status))
            vboxWddmMemFree(pContext);
        else
            WARN(("vboxVideoCmCtxRemove failed, Status 0x%x", Status));
    }
    else
        WARN(("vboxVideoAMgrCtxDestroy failed, Status 0x%x", Status));

    LOGF(("LEAVE, hContext(0x%x)", hContext));

    return Status;
}

NTSTATUS
APIENTRY
DxgkDdiLinkDevice(
    __in CONST PDEVICE_OBJECT  PhysicalDeviceObject,
    __in CONST PVOID  MiniportDeviceContext,
    __inout PLINKED_DEVICE  LinkedDevice
    )
{
    RT_NOREF(PhysicalDeviceObject, MiniportDeviceContext, LinkedDevice);
    LOGF(("ENTER, MiniportDeviceContext(0x%x)", MiniportDeviceContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, MiniportDeviceContext(0x%x)", MiniportDeviceContext));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
APIENTRY
DxgkDdiSetDisplayPrivateDriverFormat(
    CONST HANDLE  hAdapter,
    /*CONST*/ DXGKARG_SETDISPLAYPRIVATEDRIVERFORMAT*  pSetDisplayPrivateDriverFormat
    )
{
    RT_NOREF(hAdapter, pSetDisplayPrivateDriverFormat);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));
    return STATUS_SUCCESS;
}

NTSTATUS APIENTRY CALLBACK DxgkDdiRestartFromTimeout(IN_CONST_HANDLE hAdapter)
{
    RT_NOREF(hAdapter);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));
    return STATUS_SUCCESS;
}

#ifdef VBOX_WDDM_WIN8

static NTSTATUS APIENTRY DxgkDdiQueryVidPnHWCapability(
        __in     const HANDLE hAdapter,
        __inout  DXGKARG_QUERYVIDPNHWCAPABILITY *pVidPnHWCaps
      )
{
    RT_NOREF(hAdapter);
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));
    vboxVDbgBreakF();
    pVidPnHWCaps->VidPnHWCaps.DriverRotation = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverScaling = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverCloning = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverColorConvert = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverLinkedAdapaterOutput = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverRemoteDisplay = 0;
    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY DxgkDdiPresentDisplayOnly(
        _In_  const HANDLE hAdapter,
        _In_  const DXGKARG_PRESENT_DISPLAYONLY *pPresentDisplayOnly
      )
{
    LOGF(("ENTER, hAdapter(0x%x)", hAdapter));
    vboxVDbgBreakFv();

    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)hAdapter;
    PVBOXWDDM_SOURCE pSource = &pDevExt->aSources[pPresentDisplayOnly->VidPnSourceId];
    Assert(pSource->AllocData.Addr.SegmentId == 1);
    VBOXWDDM_ALLOC_DATA SrcAllocData;
    SrcAllocData.SurfDesc.width = pPresentDisplayOnly->Pitch * pPresentDisplayOnly->BytesPerPixel;
    SrcAllocData.SurfDesc.height = ~0UL;
    switch (pPresentDisplayOnly->BytesPerPixel)
    {
        case 4:
            SrcAllocData.SurfDesc.format = D3DDDIFMT_A8R8G8B8;
            break;
        case 3:
            SrcAllocData.SurfDesc.format = D3DDDIFMT_R8G8B8;
            break;
        case 2:
            SrcAllocData.SurfDesc.format = D3DDDIFMT_R5G6B5;
            break;
        case 1:
            SrcAllocData.SurfDesc.format = D3DDDIFMT_P8;
            break;
        default:
            WARN(("Unknown format"));
            SrcAllocData.SurfDesc.format = D3DDDIFMT_UNKNOWN;
            break;
    }
    SrcAllocData.SurfDesc.bpp = pPresentDisplayOnly->BytesPerPixel >> 3;
    SrcAllocData.SurfDesc.pitch = pPresentDisplayOnly->Pitch;
    SrcAllocData.SurfDesc.depth = 1;
    SrcAllocData.SurfDesc.slicePitch = pPresentDisplayOnly->Pitch;
    SrcAllocData.SurfDesc.cbSize =  ~0UL;
    SrcAllocData.Addr.SegmentId = 0;
    SrcAllocData.Addr.pvMem = pPresentDisplayOnly->pSource;
    SrcAllocData.hostID = 0;
    SrcAllocData.pSwapchain = NULL;

    RECT UpdateRect;
    BOOLEAN bUpdateRectInited = FALSE;

    for (UINT i = 0; i < pPresentDisplayOnly->NumMoves; ++i)
    {
        if (!bUpdateRectInited)
        {
            UpdateRect = pPresentDisplayOnly->pMoves[i].DestRect;
            bUpdateRectInited = TRUE;
        }
        else
            vboxWddmRectUnite(&UpdateRect, &pPresentDisplayOnly->pMoves[i].DestRect);
        vboxVdmaGgDmaBltPerform(pDevExt, &SrcAllocData, &pPresentDisplayOnly->pMoves[i].DestRect, &pSource->AllocData, &pPresentDisplayOnly->pMoves[i].DestRect);
    }

    for (UINT i = 0; i < pPresentDisplayOnly->NumDirtyRects; ++i)
    {
        RECT *pDirtyRect = &pPresentDisplayOnly->pDirtyRect[i];

        if (pDirtyRect->left >= pDirtyRect->right || pDirtyRect->top >= pDirtyRect->bottom)
        {
            WARN(("Wrong dirty rect (%d, %d)-(%d, %d)",
                pDirtyRect->left, pDirtyRect->top, pDirtyRect->right, pDirtyRect->bottom));
            continue;
        }

        vboxVdmaGgDmaBltPerform(pDevExt, &SrcAllocData, pDirtyRect, &pSource->AllocData, pDirtyRect);

        if (!bUpdateRectInited)
        {
            UpdateRect = *pDirtyRect;
            bUpdateRectInited = TRUE;
        }
        else
            vboxWddmRectUnite(&UpdateRect, pDirtyRect);
    }

    if (bUpdateRectInited && pSource->bVisible)
    {
        VBOXVBVA_OP_WITHLOCK(ReportDirtyRect, pDevExt, pSource, &UpdateRect);
    }

    LOGF(("LEAVE, hAdapter(0x%x)", hAdapter));
    return STATUS_SUCCESS;
}

static NTSTATUS DxgkDdiStopDeviceAndReleasePostDisplayOwnership(
  _In_   PVOID MiniportDeviceContext,
  _In_   D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
  _Out_  PDXGK_DISPLAY_INFORMATION DisplayInfo
)
{
    RT_NOREF(MiniportDeviceContext, TargetId, DisplayInfo);
    LOGF(("ENTER, hAdapter(0x%x)", MiniportDeviceContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, hAdapter(0x%x)", MiniportDeviceContext));
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS DxgkDdiSystemDisplayEnable(
        _In_   PVOID MiniportDeviceContext,
        _In_   D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
        _In_   PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
        _Out_  UINT *Width,
        _Out_  UINT *Height,
        _Out_  D3DDDIFORMAT *ColorFormat
      )
{
    RT_NOREF(MiniportDeviceContext, TargetId, Flags, Width, Height, ColorFormat);
    LOGF(("ENTER, hAdapter(0x%x)", MiniportDeviceContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, hAdapter(0x%x)", MiniportDeviceContext));
    return STATUS_NOT_SUPPORTED;
}

static VOID DxgkDdiSystemDisplayWrite(
  _In_  PVOID MiniportDeviceContext,
  _In_  PVOID Source,
  _In_  UINT SourceWidth,
  _In_  UINT SourceHeight,
  _In_  UINT SourceStride,
  _In_  UINT PositionX,
  _In_  UINT PositionY
)
{
    RT_NOREF(MiniportDeviceContext, Source, SourceWidth, SourceHeight, SourceStride, PositionX, PositionY);
    LOGF(("ENTER, hAdapter(0x%x)", MiniportDeviceContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, hAdapter(0x%x)", MiniportDeviceContext));
}

static NTSTATUS DxgkDdiGetChildContainerId(
  _In_     PVOID MiniportDeviceContext,
  _In_     ULONG ChildUid,
  _Inout_  PDXGK_CHILD_CONTAINER_ID ContainerId
)
{
    RT_NOREF(MiniportDeviceContext, ChildUid, ContainerId);
    LOGF(("ENTER, hAdapter(0x%x)", MiniportDeviceContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, hAdapter(0x%x)", MiniportDeviceContext));
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY DxgkDdiSetPowerComponentFState(
  _In_  const HANDLE DriverContext,
  UINT ComponentIndex,
  UINT FState
)
{
    RT_NOREF(DriverContext, ComponentIndex, FState);
    LOGF(("ENTER, DriverContext(0x%x)", DriverContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, DriverContext(0x%x)", DriverContext));
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY DxgkDdiPowerRuntimeControlRequest(
  _In_       const HANDLE DriverContext,
  _In_       LPCGUID PowerControlCode,
  _In_opt_   PVOID InBuffer,
  _In_       SIZE_T InBufferSize,
  _Out_opt_  PVOID OutBuffer,
  _In_       SIZE_T OutBufferSize,
  _Out_opt_  PSIZE_T BytesReturned
)
{
    RT_NOREF(DriverContext, PowerControlCode, InBuffer, InBufferSize, OutBuffer, OutBufferSize, BytesReturned);
    LOGF(("ENTER, DriverContext(0x%x)", DriverContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, DriverContext(0x%x)", DriverContext));
    return STATUS_SUCCESS;
}

static NTSTATUS DxgkDdiNotifySurpriseRemoval(
        _In_  PVOID MiniportDeviceContext,
        _In_  DXGK_SURPRISE_REMOVAL_TYPE RemovalType
      )
{
    RT_NOREF(MiniportDeviceContext, RemovalType);
    LOGF(("ENTER, hAdapter(0x%x)", MiniportDeviceContext));
    vboxVDbgBreakFv();
    AssertBreakpoint();
    LOGF(("LEAVE, hAdapter(0x%x)", MiniportDeviceContext));
    return STATUS_SUCCESS;
}

static NTSTATUS vboxWddmInitDisplayOnlyDriver(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
    KMDDOD_INITIALIZATION_DATA DriverInitializationData = {'\0'};

    DriverInitializationData.Version = DXGKDDI_INTERFACE_VERSION_WIN8;

    DriverInitializationData.DxgkDdiAddDevice = DxgkDdiAddDevice;
    DriverInitializationData.DxgkDdiStartDevice = DxgkDdiStartDevice;
    DriverInitializationData.DxgkDdiStopDevice = DxgkDdiStopDevice;
    DriverInitializationData.DxgkDdiRemoveDevice = DxgkDdiRemoveDevice;
    DriverInitializationData.DxgkDdiDispatchIoRequest = DxgkDdiDispatchIoRequest;
    DriverInitializationData.DxgkDdiInterruptRoutine = DxgkDdiInterruptRoutineLegacy;
    DriverInitializationData.DxgkDdiDpcRoutine = DxgkDdiDpcRoutineLegacy;
    DriverInitializationData.DxgkDdiQueryChildRelations = DxgkDdiQueryChildRelations;
    DriverInitializationData.DxgkDdiQueryChildStatus = DxgkDdiQueryChildStatus;
    DriverInitializationData.DxgkDdiQueryDeviceDescriptor = DxgkDdiQueryDeviceDescriptor;
    DriverInitializationData.DxgkDdiSetPowerState = DxgkDdiSetPowerState;
    DriverInitializationData.DxgkDdiNotifyAcpiEvent = DxgkDdiNotifyAcpiEvent;
    DriverInitializationData.DxgkDdiResetDevice = DxgkDdiResetDevice;
    DriverInitializationData.DxgkDdiUnload = DxgkDdiUnload;
    DriverInitializationData.DxgkDdiQueryInterface = DxgkDdiQueryInterface;
    DriverInitializationData.DxgkDdiControlEtwLogging = DxgkDdiControlEtwLogging;
    DriverInitializationData.DxgkDdiQueryAdapterInfo = DxgkDdiQueryAdapterInfo;
    DriverInitializationData.DxgkDdiSetPalette = DxgkDdiSetPalette;
    DriverInitializationData.DxgkDdiSetPointerPosition = DxgkDdiSetPointerPosition;
    DriverInitializationData.DxgkDdiSetPointerShape = DxgkDdiSetPointerShape;
    DriverInitializationData.DxgkDdiEscape = DxgkDdiEscape;
    DriverInitializationData.DxgkDdiCollectDbgInfo = DxgkDdiCollectDbgInfo;
    DriverInitializationData.DxgkDdiIsSupportedVidPn = DxgkDdiIsSupportedVidPn;
    DriverInitializationData.DxgkDdiRecommendFunctionalVidPn = DxgkDdiRecommendFunctionalVidPn;
    DriverInitializationData.DxgkDdiEnumVidPnCofuncModality = DxgkDdiEnumVidPnCofuncModality;
    DriverInitializationData.DxgkDdiSetVidPnSourceVisibility = DxgkDdiSetVidPnSourceVisibility;
    DriverInitializationData.DxgkDdiCommitVidPn = DxgkDdiCommitVidPn;
    DriverInitializationData.DxgkDdiUpdateActiveVidPnPresentPath = DxgkDdiUpdateActiveVidPnPresentPath;
    DriverInitializationData.DxgkDdiRecommendMonitorModes = DxgkDdiRecommendMonitorModes;
    DriverInitializationData.DxgkDdiQueryVidPnHWCapability = DxgkDdiQueryVidPnHWCapability;
    DriverInitializationData.DxgkDdiPresentDisplayOnly = DxgkDdiPresentDisplayOnly;
    DriverInitializationData.DxgkDdiStopDeviceAndReleasePostDisplayOwnership = DxgkDdiStopDeviceAndReleasePostDisplayOwnership;
    DriverInitializationData.DxgkDdiSystemDisplayEnable = DxgkDdiSystemDisplayEnable;
    DriverInitializationData.DxgkDdiSystemDisplayWrite = DxgkDdiSystemDisplayWrite;
//    DriverInitializationData.DxgkDdiGetChildContainerId = DxgkDdiGetChildContainerId;
//    DriverInitializationData.DxgkDdiSetPowerComponentFState = DxgkDdiSetPowerComponentFState;
//    DriverInitializationData.DxgkDdiPowerRuntimeControlRequest = DxgkDdiPowerRuntimeControlRequest;
//    DriverInitializationData.DxgkDdiNotifySurpriseRemoval = DxgkDdiNotifySurpriseRemoval;

    /* Display-only driver is not required to report VSYNC.
     * The Microsoft KMDOD driver sample does not implement DxgkDdiControlInterrupt and DxgkDdiGetScanLine.
     * The functions must be either both implemented or none implemented.
     * Windows 10 10586 guests had problems with VSYNC in display-only driver (#8228).
     * Therefore the driver does not implement DxgkDdiControlInterrupt and DxgkDdiGetScanLine.
     */

    NTSTATUS Status = DxgkInitializeDisplayOnlyDriver(pDriverObject,
                          pRegistryPath,
                          &DriverInitializationData);
    if (!NT_SUCCESS(Status))
    {
        WARN(("DxgkInitializeDisplayOnlyDriver failed! Status 0x%x", Status));
    }
    return Status;
}
#endif

static NTSTATUS vboxWddmInitFullGraphicsDriver(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath, BOOLEAN fCmdVbva)
{
#ifdef VBOX_WITH_CROGL
#define VBOXWDDM_CALLBACK_NAME(_base, _fCmdVbva) ((_fCmdVbva) ? _base##New : _base##Legacy)
#else
#define VBOXWDDM_CALLBACK_NAME(_base, _fCmdVbva) (_base##Legacy)
#endif

    DRIVER_INITIALIZATION_DATA DriverInitializationData = {'\0'};

    // Fill in the DriverInitializationData structure and call DxgkInitialize()
    DriverInitializationData.Version = DXGKDDI_INTERFACE_VERSION;

    DriverInitializationData.DxgkDdiAddDevice = DxgkDdiAddDevice;
    DriverInitializationData.DxgkDdiStartDevice = DxgkDdiStartDevice;
    DriverInitializationData.DxgkDdiStopDevice = DxgkDdiStopDevice;
    DriverInitializationData.DxgkDdiRemoveDevice = DxgkDdiRemoveDevice;
    DriverInitializationData.DxgkDdiDispatchIoRequest = DxgkDdiDispatchIoRequest;
    DriverInitializationData.DxgkDdiInterruptRoutine = VBOXWDDM_CALLBACK_NAME(DxgkDdiInterruptRoutine, fCmdVbva);
    DriverInitializationData.DxgkDdiDpcRoutine = VBOXWDDM_CALLBACK_NAME(DxgkDdiDpcRoutine, fCmdVbva);
    DriverInitializationData.DxgkDdiQueryChildRelations = DxgkDdiQueryChildRelations;
    DriverInitializationData.DxgkDdiQueryChildStatus = DxgkDdiQueryChildStatus;
    DriverInitializationData.DxgkDdiQueryDeviceDescriptor = DxgkDdiQueryDeviceDescriptor;
    DriverInitializationData.DxgkDdiSetPowerState = DxgkDdiSetPowerState;
    DriverInitializationData.DxgkDdiNotifyAcpiEvent = DxgkDdiNotifyAcpiEvent;
    DriverInitializationData.DxgkDdiResetDevice = DxgkDdiResetDevice;
    DriverInitializationData.DxgkDdiUnload = DxgkDdiUnload;
    DriverInitializationData.DxgkDdiQueryInterface = DxgkDdiQueryInterface;
    DriverInitializationData.DxgkDdiControlEtwLogging = DxgkDdiControlEtwLogging;

    DriverInitializationData.DxgkDdiQueryAdapterInfo = DxgkDdiQueryAdapterInfo;
    DriverInitializationData.DxgkDdiCreateDevice = DxgkDdiCreateDevice;
    DriverInitializationData.DxgkDdiCreateAllocation = DxgkDdiCreateAllocation;
    DriverInitializationData.DxgkDdiDestroyAllocation = DxgkDdiDestroyAllocation;
    DriverInitializationData.DxgkDdiDescribeAllocation = DxgkDdiDescribeAllocation;
    DriverInitializationData.DxgkDdiGetStandardAllocationDriverData = DxgkDdiGetStandardAllocationDriverData;
    DriverInitializationData.DxgkDdiAcquireSwizzlingRange = DxgkDdiAcquireSwizzlingRange;
    DriverInitializationData.DxgkDdiReleaseSwizzlingRange = DxgkDdiReleaseSwizzlingRange;
    DriverInitializationData.DxgkDdiPatch = VBOXWDDM_CALLBACK_NAME(DxgkDdiPatch, fCmdVbva);
    DriverInitializationData.DxgkDdiSubmitCommand = VBOXWDDM_CALLBACK_NAME(DxgkDdiSubmitCommand, fCmdVbva);
    DriverInitializationData.DxgkDdiPreemptCommand = VBOXWDDM_CALLBACK_NAME(DxgkDdiPreemptCommand, fCmdVbva);
    DriverInitializationData.DxgkDdiBuildPagingBuffer = VBOXWDDM_CALLBACK_NAME(DxgkDdiBuildPagingBuffer, fCmdVbva);
    DriverInitializationData.DxgkDdiSetPalette = DxgkDdiSetPalette;
    DriverInitializationData.DxgkDdiSetPointerPosition = DxgkDdiSetPointerPosition;
    DriverInitializationData.DxgkDdiSetPointerShape = DxgkDdiSetPointerShape;
    DriverInitializationData.DxgkDdiResetFromTimeout = DxgkDdiResetFromTimeout;
    DriverInitializationData.DxgkDdiRestartFromTimeout = DxgkDdiRestartFromTimeout;
    DriverInitializationData.DxgkDdiEscape = DxgkDdiEscape;
    DriverInitializationData.DxgkDdiCollectDbgInfo = DxgkDdiCollectDbgInfo;
    DriverInitializationData.DxgkDdiQueryCurrentFence = VBOXWDDM_CALLBACK_NAME(DxgkDdiQueryCurrentFence, fCmdVbva);
    DriverInitializationData.DxgkDdiIsSupportedVidPn = DxgkDdiIsSupportedVidPn;
    DriverInitializationData.DxgkDdiRecommendFunctionalVidPn = DxgkDdiRecommendFunctionalVidPn;
    DriverInitializationData.DxgkDdiEnumVidPnCofuncModality = DxgkDdiEnumVidPnCofuncModality;
    DriverInitializationData.DxgkDdiSetVidPnSourceAddress = DxgkDdiSetVidPnSourceAddress;
    DriverInitializationData.DxgkDdiSetVidPnSourceVisibility = DxgkDdiSetVidPnSourceVisibility;
    DriverInitializationData.DxgkDdiCommitVidPn = DxgkDdiCommitVidPn;
    DriverInitializationData.DxgkDdiUpdateActiveVidPnPresentPath = DxgkDdiUpdateActiveVidPnPresentPath;
    DriverInitializationData.DxgkDdiRecommendMonitorModes = DxgkDdiRecommendMonitorModes;
    DriverInitializationData.DxgkDdiRecommendVidPnTopology = DxgkDdiRecommendVidPnTopology;
    DriverInitializationData.DxgkDdiGetScanLine = DxgkDdiGetScanLine;
    DriverInitializationData.DxgkDdiStopCapture = DxgkDdiStopCapture;
    DriverInitializationData.DxgkDdiControlInterrupt = DxgkDdiControlInterrupt;
    DriverInitializationData.DxgkDdiCreateOverlay = DxgkDdiCreateOverlay;

    DriverInitializationData.DxgkDdiDestroyDevice = DxgkDdiDestroyDevice;
    DriverInitializationData.DxgkDdiOpenAllocation = DxgkDdiOpenAllocation;
    DriverInitializationData.DxgkDdiCloseAllocation = DxgkDdiCloseAllocation;
    DriverInitializationData.DxgkDdiRender = VBOXWDDM_CALLBACK_NAME(DxgkDdiRender, fCmdVbva);
    DriverInitializationData.DxgkDdiPresent = VBOXWDDM_CALLBACK_NAME(DxgkDdiPresent, fCmdVbva);

    DriverInitializationData.DxgkDdiUpdateOverlay = DxgkDdiUpdateOverlay;
    DriverInitializationData.DxgkDdiFlipOverlay = DxgkDdiFlipOverlay;
    DriverInitializationData.DxgkDdiDestroyOverlay = DxgkDdiDestroyOverlay;

    DriverInitializationData.DxgkDdiCreateContext = DxgkDdiCreateContext;
    DriverInitializationData.DxgkDdiDestroyContext = DxgkDdiDestroyContext;

    DriverInitializationData.DxgkDdiLinkDevice = NULL; //DxgkDdiLinkDevice;
    DriverInitializationData.DxgkDdiSetDisplayPrivateDriverFormat = DxgkDdiSetDisplayPrivateDriverFormat;
//#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
//# error port to Win7 DDI
//            DriverInitializationData.DxgkDdiRenderKm  = DxgkDdiRenderKm;
//            DriverInitializationData.DxgkDdiRestartFromTimeout  = DxgkDdiRestartFromTimeout;
//            DriverInitializationData.DxgkDdiSetVidPnSourceVisibility  = DxgkDdiSetVidPnSourceVisibility;
//            DriverInitializationData.DxgkDdiUpdateActiveVidPnPresentPath  = DxgkDdiUpdateActiveVidPnPresentPath;
//            DriverInitializationData.DxgkDdiQueryVidPnHWCapability  = DxgkDdiQueryVidPnHWCapability;
//#endif

    NTSTATUS Status = DxgkInitialize(pDriverObject,
                          pRegistryPath,
                          &DriverInitializationData);
    if (!NT_SUCCESS(Status))
    {
        WARN(("DxgkInitialize failed! Status 0x%x", Status));
    }
    return Status;
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    PAGED_CODE();

    vboxVDbgBreakFv();

#if 0//def DEBUG_misha
    RTLogGroupSettings(0, "+default.e.l.f.l2.l3");
#endif

#ifdef VBOX_WDDM_WIN8
    LOGREL(("VBox WDDM Driver for Windows 8+ version %d.%d.%dr%d, %d bit; Built %s %s",
            VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD, VBOX_SVN_REV,
            (sizeof (void*) << 3), __DATE__, __TIME__));
#else
    LOGREL(("VBox WDDM Driver for Windows Vista and 7 version %d.%d.%dr%d, %d bit; Built %s %s",
            VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD, VBOX_SVN_REV,
            (sizeof (void*) << 3), __DATE__, __TIME__));
#endif

    if (   !ARGUMENT_PRESENT(DriverObject)
        || !ARGUMENT_PRESENT(RegistryPath))
        return STATUS_INVALID_PARAMETER;

    vboxWddmDrvCfgInit(RegistryPath);

    ULONG major, minor, build;
    BOOLEAN fCheckedBuild = PsGetVersion(&major, &minor, &build, NULL); NOREF(fCheckedBuild);
    BOOLEAN f3DRequired = FALSE;

    LOGREL(("OsVersion(%d, %d, %d)", major, minor, build));

    NTSTATUS Status = STATUS_SUCCESS;
    /* Initialize VBoxGuest library, which is used for requests which go through VMMDev. */
    int rc = VbglR0InitClient();
    if (RT_SUCCESS(rc))
    {
        if (major > 6)
        {
            /* Windows 10 and newer. */
            f3DRequired = TRUE;
        }
        else if (major == 6)
        {
            if (minor >= 2)
            {
                /* Windows 8, 8.1 and 10 preview. */
                f3DRequired = TRUE;
            }
            else
            {
                f3DRequired = FALSE;
            }
        }
        else
        {
            WARN(("Unsupported OLDER win version, ignore and assume 3D is NOT required"));
            f3DRequired = FALSE;
        }

        LOG(("3D is %srequired!", f3DRequired? "": "NOT "));

        Status = STATUS_SUCCESS;
#ifdef VBOX_WITH_CROGL
        VBoxMpCrCtlConInit();

        /* always need to do the check to request host caps */
        LOG(("Doing the 3D check.."));
        if (!VBoxMpCrCtlConIs3DSupported())
#endif
        {
#ifdef VBOX_WDDM_WIN8
            Assert(f3DRequired);
            g_VBoxDisplayOnly = 1;

            /* Black list some builds. */
            if (major == 6 && minor == 4 && build == 9841)
            {
                /* W10 Technical preview crashes with display-only driver. */
                LOGREL(("3D is NOT supported by the host, fallback to the system video driver."));
                Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                LOGREL(("3D is NOT supported by the host, falling back to display-only mode.."));
            }
#else
            if (f3DRequired)
            {
                LOGREL(("3D is NOT supported by the host, but is required for the current guest version using this driver.."));
                Status = STATUS_UNSUCCESSFUL;
            }
            else
                LOGREL(("3D is NOT supported by the host, but is NOT required for the current guest version using this driver, continuing with Disabled 3D.."));
#endif
        }

#if 0 //defined(DEBUG_misha) && defined(VBOX_WDDM_WIN8)
        /* force g_VBoxDisplayOnly for debugging purposes */
        LOGREL(("Current win8 video driver only supports display-only mode no matter whether or not host 3D is enabled!"));
        g_VBoxDisplayOnly = 1;
#endif

        if (NT_SUCCESS(Status))
        {
#ifdef VBOX_WITH_CROGL
            rc = VBoxVrInit();
            if (RT_SUCCESS(rc))
#endif
            {
#ifdef VBOX_WDDM_WIN8
                if (g_VBoxDisplayOnly)
                {
                    Status = vboxWddmInitDisplayOnlyDriver(DriverObject, RegistryPath);
                }
                else
#endif
                {
                    Status = vboxWddmInitFullGraphicsDriver(DriverObject, RegistryPath,
#ifdef VBOX_WITH_CROGL
                            !!(VBoxMpCrGetHostCaps() & CR_VBOX_CAP_CMDVBVA)
#else
                            FALSE
#endif
                            );
                }

                if (NT_SUCCESS(Status))
                    return Status;
#ifdef VBOX_WITH_CROGL
                VBoxVrTerm();
#endif
            }
#ifdef VBOX_WITH_CROGL
            else
            {
                WARN(("VBoxVrInit failed, rc(%d)", rc));
                Status = STATUS_UNSUCCESSFUL;
            }
#endif
        }
        else
            LOGREL(("Aborting the video driver load due to 3D support missing"));

        VbglR0TerminateClient();
    }
    else
    {
        WARN(("VbglR0InitClient failed, rc(%d)", rc));
        Status = STATUS_UNSUCCESSFUL;
    }

    AssertRelease(!NT_SUCCESS(Status));

    PRTLOGGER pLogger = RTLogRelSetDefaultInstance(NULL);
    if (pLogger)
    {
        RTLogDestroy(pLogger);
    }
    pLogger = RTLogSetDefaultInstance(NULL);
    if (pLogger)
    {
        RTLogDestroy(pLogger);
    }

    return Status;
}


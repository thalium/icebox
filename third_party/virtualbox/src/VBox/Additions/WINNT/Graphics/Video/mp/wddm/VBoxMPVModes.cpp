/* $Id: VBoxMPVModes.cpp $ */
/** @file
 * VBox WDDM Miniport driver
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#include <iprt/param.h> /* PAGE_OFFSET_MASK */

#include <stdio.h> /* for swprintf */


int VBoxVModesInit(VBOX_VMODES *pModes, uint32_t cTargets)
{
    if (cTargets >= VBOX_VIDEO_MAX_SCREENS)
    {
        WARN(("invalid target"));
        return VERR_INVALID_PARAMETER;
    }

    pModes->cTargets = cTargets;
    for (uint32_t i = 0; i < cTargets; ++i)
    {
        int rc = CrSaInit(&pModes->aTargets[i], 16);
        if (RT_FAILURE(rc))
        {
            WARN(("CrSaInit failed"));

            for (uint32_t j = 0; j < i; ++j)
            {
                CrSaCleanup(&pModes->aTargets[j]);
            }
            return rc;
        }
    }

    return VINF_SUCCESS;
}

void VBoxVModesCleanup(VBOX_VMODES *pModes)
{
    for (uint32_t i = 0; i < pModes->cTargets; ++i)
    {
        CrSaCleanup(&pModes->aTargets[i]);
    }
}

int VBoxVModesAdd(VBOX_VMODES *pModes, uint32_t u32Target, uint64_t u64)
{
    if (u32Target >= pModes->cTargets)
    {
        WARN(("invalid target id"));
        return VERR_INVALID_PARAMETER;
    }

    return CrSaAdd(&pModes->aTargets[u32Target], u64);
}

int VBoxVModesRemove(VBOX_VMODES *pModes, uint32_t u32Target, uint64_t u64)
{
    if (u32Target >= pModes->cTargets)
    {
        WARN(("invalid target id"));
        return VERR_INVALID_PARAMETER;
    }

    return CrSaRemove(&pModes->aTargets[u32Target], u64);
}

static void vboxWddmVModesInit(VBOXWDDM_VMODES *pModes, uint32_t cTargets)
{
    VBoxVModesInit(&pModes->Modes, cTargets);
    memset(pModes->aTransientResolutions, 0, cTargets * sizeof (pModes->aTransientResolutions[0]));
    memset(pModes->aPendingRemoveCurResolutions, 0, cTargets * sizeof (pModes->aPendingRemoveCurResolutions[0]));
}

static void vboxWddmVModesCleanup(VBOXWDDM_VMODES *pModes)
{
    VBoxVModesCleanup(&pModes->Modes);
    memset(pModes->aTransientResolutions, 0, sizeof (pModes->aTransientResolutions));
    memset(pModes->aPendingRemoveCurResolutions, 0, sizeof (pModes->aPendingRemoveCurResolutions));
}

/*
static void vboxWddmVModesClone(const VBOXWDDM_VMODES *pModes, VBOXWDDM_VMODES *pDst)
{
    VBoxVModesClone(&pModes->Modes, pDst->Modes);
    memcpy(pDst->aTransientResolutions, pModes->aTransientResolutions, pModes->Modes.cTargets * sizeof (pModes->aTransientResolutions[0]));
    memcpy(pDst->aPendingRemoveCurResolutions, pModes->aPendingRemoveCurResolutions, pModes->Modes.cTargets * sizeof (pModes->aPendingRemoveCurResolutions[0]));
}
*/

static const RTRECTSIZE g_VBoxBuiltinResolutions[] =
{
    /* standard modes */
    { 640,   480 },
    { 800,   600 },
    { 1024,  768 },
    { 1152,  864 },
    { 1280,  960 },
    { 1280, 1024 },
    { 1400, 1050 },
    { 1600, 1200 },
    { 1920, 1440 },
};

DECLINLINE(bool) vboxVModesRMatch(const RTRECTSIZE *pResolution1, const RTRECTSIZE *pResolution2)
{
    return !memcmp(pResolution1, pResolution2, sizeof (*pResolution1));
}

int vboxWddmVModesRemove(PVBOXMP_DEVEXT pExt, VBOXWDDM_VMODES *pModes, uint32_t u32Target, const RTRECTSIZE *pResolution)
{
    if (!pResolution->cx || !pResolution->cy)
    {
        WARN(("invalid resolution data"));
        return VERR_INVALID_PARAMETER;
    }

    if (u32Target >= pModes->Modes.cTargets)
    {
        WARN(("invalid target id"));
        return VERR_INVALID_PARAMETER;
    }

    if (CR_RSIZE2U64(*pResolution) == pModes->aTransientResolutions[u32Target])
        pModes->aTransientResolutions[u32Target] = 0;

    if (vboxVModesRMatch(pResolution, &pExt->aTargets[u32Target].Size))
    {
        if (CR_RSIZE2U64(*pResolution) == pModes->aPendingRemoveCurResolutions[u32Target])
            return VINF_ALREADY_INITIALIZED;

        if (pModes->aPendingRemoveCurResolutions[u32Target])
        {
            VBoxVModesRemove(&pModes->Modes, u32Target, pModes->aPendingRemoveCurResolutions[u32Target]);
            pModes->aPendingRemoveCurResolutions[u32Target] = 0;
        }

        pModes->aPendingRemoveCurResolutions[u32Target] = CR_RSIZE2U64(*pResolution);
        return VINF_ALREADY_INITIALIZED;
    }
    else if (CR_RSIZE2U64(*pResolution) == pModes->aPendingRemoveCurResolutions[u32Target])
        pModes->aPendingRemoveCurResolutions[u32Target] = 0;

    int rc = VBoxVModesRemove(&pModes->Modes, u32Target, CR_RSIZE2U64(*pResolution));
    if (RT_FAILURE(rc))
    {
        WARN(("VBoxVModesRemove failed %d, can never happen", rc));
        return rc;
    }

    if (rc == VINF_ALREADY_INITIALIZED)
        return rc;

    return VINF_SUCCESS;
}

static void vboxWddmVModesSaveTransient(PVBOXMP_DEVEXT pExt, uint32_t u32Target, const RTRECTSIZE *pResolution)
{
    VBOXMPCMNREGISTRY Registry;
    VP_STATUS rc;

    rc = VBoxMPCmnRegInit(pExt, &Registry);
    VBOXMP_WARN_VPS(rc);

    if (u32Target==0)
    {
        /*First name without a suffix*/
        rc = VBoxMPCmnRegSetDword(Registry, L"CustomXRes", pResolution->cx);
        VBOXMP_WARN_VPS(rc);
        rc = VBoxMPCmnRegSetDword(Registry, L"CustomYRes", pResolution->cy);
        VBOXMP_WARN_VPS(rc);
        rc = VBoxMPCmnRegSetDword(Registry, L"CustomBPP", 32); /* <- just in case for older driver usage */
        VBOXMP_WARN_VPS(rc);
    }
    else
    {
        wchar_t keyname[32];
        swprintf(keyname, L"CustomXRes%d", u32Target);
        rc = VBoxMPCmnRegSetDword(Registry, keyname, pResolution->cx);
        VBOXMP_WARN_VPS(rc);
        swprintf(keyname, L"CustomYRes%d", u32Target);
        rc = VBoxMPCmnRegSetDword(Registry, keyname, pResolution->cy);
        VBOXMP_WARN_VPS(rc);
        swprintf(keyname, L"CustomBPP%d", u32Target);
        rc = VBoxMPCmnRegSetDword(Registry, keyname, 32); /* <- just in case for older driver usage */
        VBOXMP_WARN_VPS(rc);
    }

    rc = VBoxMPCmnRegFini(Registry);
    VBOXMP_WARN_VPS(rc);
}

int vboxWddmVModesAdd(PVBOXMP_DEVEXT pExt, VBOXWDDM_VMODES *pModes, uint32_t u32Target, const RTRECTSIZE *pResolution, BOOLEAN fTransient)
{
    if (!pResolution->cx || !pResolution->cy)
    {
        WARN(("invalid resolution data"));
        return VERR_INVALID_PARAMETER;
    }

    if (u32Target >= pModes->Modes.cTargets)
    {
        WARN(("invalid target id"));
        return VERR_INVALID_PARAMETER;
    }

    ULONG vramSize = vboxWddmVramCpuVisibleSegmentSize(pExt);
    vramSize /= pExt->u.primary.commonInfo.cDisplays;
# ifdef VBOX_WDDM_WIN8
    if (!g_VBoxDisplayOnly)
# endif
    {
        /* at least two surfaces will be needed: primary & shadow */
        vramSize /= 2;
    }
    vramSize &= ~PAGE_OFFSET_MASK;

    /* prevent potensial overflow */
    if (   pResolution->cx > 0x7fff
        || pResolution->cy > 0x7fff)
    {
        WARN(("resolution %dx%d insane", pResolution->cx, pResolution->cy));
        return VERR_INVALID_PARAMETER;
    }

    uint32_t cbSurfMem = pResolution->cx * pResolution->cy * 4;
    if (cbSurfMem > vramSize)
    {
        WARN(("resolution %dx%d too big for available VRAM (%d bytes)\n", pResolution->cx, pResolution->cy, vramSize));
        return VERR_NOT_SUPPORTED;
    }

    if (!VBoxLikesVideoMode(u32Target, pResolution->cx, pResolution->cy, 32))
    {
        WARN(("resolution %dx%d not accepted by the frontend\n", pResolution->cx, pResolution->cy));
        return VERR_NOT_SUPPORTED;
    }

    if (pModes->aTransientResolutions[u32Target] == CR_RSIZE2U64(*pResolution))
    {
        if (!fTransient) /* if the mode is not transient anymore, remove it from transient */
            pModes->aTransientResolutions[u32Target] = 0;
        return VINF_ALREADY_INITIALIZED;
    }

    int rc;
    bool fTransientIfExists = false;
    if (pModes->aPendingRemoveCurResolutions[u32Target] == CR_RSIZE2U64(*pResolution))
    {
        /* no need to remove it anymore */
        pModes->aPendingRemoveCurResolutions[u32Target] = 0;
        rc = VINF_ALREADY_INITIALIZED;
        fTransientIfExists = true;
    }
    else
    {
        rc = VBoxVModesAdd(&pModes->Modes, u32Target, CR_RSIZE2U64(*pResolution));
        if (RT_FAILURE(rc))
        {
            WARN(("VBoxVModesAdd failed %d", rc));
            return rc;
        }
    }

    if (rc == VINF_ALREADY_INITIALIZED && !fTransientIfExists)
        return rc;

    if (fTransient)
    {
        if (pModes->aTransientResolutions[u32Target])
        {
            /* note that we can not overwrite rc here, because it holds the "existed" status, which we need to return */
            RTRECTSIZE size = CR_U642RSIZE(pModes->aTransientResolutions[u32Target]);
            int tmpRc = vboxWddmVModesRemove(pExt, pModes, u32Target, &size);
            if (RT_FAILURE(tmpRc))
            {
                WARN(("vboxWddmVModesRemove failed %d, can never happen", tmpRc));
                return tmpRc;
            }
        }
        Assert(!pModes->aTransientResolutions[u32Target]);

        pModes->aTransientResolutions[u32Target] = CR_RSIZE2U64(*pResolution);
        vboxWddmVModesSaveTransient(pExt, u32Target, pResolution);
    }

    return rc;
}

int voxWddmVModesInitForTarget(PVBOXMP_DEVEXT pExt, VBOXWDDM_VMODES *pModes, uint32_t u32Target)
{
    for (uint32_t i = 0; i < RT_ELEMENTS(g_VBoxBuiltinResolutions); ++i)
    {
        vboxWddmVModesAdd(pExt, pModes, u32Target, &g_VBoxBuiltinResolutions[i], FALSE);
    }

    if (pExt->aTargets[u32Target].Size.cx)
    {
        vboxWddmVModesAdd(pExt, pModes, u32Target, &pExt->aTargets[u32Target].Size, TRUE);
    }

    /* Check registry for manually added modes, up to 128 entries is supported
     * Give up on the first error encountered.
     */
    VBOXMPCMNREGISTRY Registry;
    VP_STATUS vpRc;

    vpRc = VBoxMPCmnRegInit(pExt, &Registry);
    if (vpRc != NO_ERROR)
    {
        WARN(("VBoxMPCmnRegInit failed %d, ignore", vpRc));
        return VINF_SUCCESS;
    }

    uint32_t CustomXRes = 0, CustomYRes = 0;

    if (u32Target == 0)
    {
        /*First name without a suffix*/
        vpRc = VBoxMPCmnRegQueryDword(Registry, L"CustomXRes", &CustomXRes);
        VBOXMP_WARN_VPS_NOBP(vpRc);
        vpRc = VBoxMPCmnRegQueryDword(Registry, L"CustomYRes", &CustomYRes);
        VBOXMP_WARN_VPS_NOBP(vpRc);
    }
    else
    {
        wchar_t keyname[32];
        swprintf(keyname, L"CustomXRes%d", u32Target);
        vpRc = VBoxMPCmnRegQueryDword(Registry, keyname, &CustomXRes);
        VBOXMP_WARN_VPS_NOBP(vpRc);
        swprintf(keyname, L"CustomYRes%d", u32Target);
        vpRc = VBoxMPCmnRegQueryDword(Registry, keyname, &CustomYRes);
        VBOXMP_WARN_VPS_NOBP(vpRc);
    }

    LOG(("got stored custom resolution[%d] %dx%dx", u32Target, CustomXRes, CustomYRes));

    if (CustomXRes || CustomYRes)
    {
        if (CustomXRes == 0)
            CustomXRes = pExt->aTargets[u32Target].Size.cx ? pExt->aTargets[u32Target].Size.cx : 800;
        if (CustomYRes == 0)
            CustomYRes = pExt->aTargets[u32Target].Size.cy ? pExt->aTargets[u32Target].Size.cy : 600;

        RTRECTSIZE Resolution = {CustomXRes, CustomYRes};
        vboxWddmVModesAdd(pExt, pModes, u32Target, &Resolution, TRUE);
    }


    for (int curKey=0; curKey<128; curKey++)
    {
        wchar_t keyname[24];

        swprintf(keyname, L"CustomMode%dWidth", curKey);
        vpRc = VBoxMPCmnRegQueryDword(Registry, keyname, &CustomXRes);
        VBOXMP_CHECK_VPS_BREAK(vpRc);

        swprintf(keyname, L"CustomMode%dHeight", curKey);
        vpRc = VBoxMPCmnRegQueryDword(Registry, keyname, &CustomYRes);
        VBOXMP_CHECK_VPS_BREAK(vpRc);

        LOG(("got custom mode[%u]=%ux%u", curKey, CustomXRes, CustomYRes));

        /* round down width to be a multiple of 8 if necessary */
        if (!VBoxCommonFromDeviceExt(pExt)->fAnyX)
        {
            CustomXRes &= 0xFFF8;
        }

        LOG(("adding video mode from registry."));

        RTRECTSIZE Resolution = {CustomXRes, CustomYRes};

        vboxWddmVModesAdd(pExt, pModes, u32Target, &Resolution, FALSE);
    }

    vpRc = VBoxMPCmnRegFini(Registry);
    VBOXMP_WARN_VPS(vpRc);

    return VINF_SUCCESS;
}

static VBOXWDDM_VMODES g_VBoxWddmVModes;

void VBoxWddmVModesCleanup()
{
    VBOXWDDM_VMODES *pModes = &g_VBoxWddmVModes;
    vboxWddmVModesCleanup(pModes);
}

int VBoxWddmVModesInit(PVBOXMP_DEVEXT pExt)
{
    VBOXWDDM_VMODES *pModes = &g_VBoxWddmVModes;

    vboxWddmVModesInit(pModes, VBoxCommonFromDeviceExt(pExt)->cDisplays);

    int rc;

    for (int i = 0; i < VBoxCommonFromDeviceExt(pExt)->cDisplays; ++i)
    {
        rc = voxWddmVModesInitForTarget(pExt, pModes, (uint32_t)i);
        if (RT_FAILURE(rc))
        {
            WARN(("voxWddmVModesInitForTarget failed %d", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}

const CR_SORTARRAY* VBoxWddmVModesGet(PVBOXMP_DEVEXT pExt, uint32_t u32Target)
{
    if (u32Target >= (uint32_t)VBoxCommonFromDeviceExt(pExt)->cDisplays)
    {
        WARN(("invalid target"));
        return NULL;
    }

    return &g_VBoxWddmVModes.Modes.aTargets[u32Target];
}

int VBoxWddmVModesRemove(PVBOXMP_DEVEXT pExt, uint32_t u32Target, const RTRECTSIZE *pResolution)
{
    return vboxWddmVModesRemove(pExt, &g_VBoxWddmVModes, u32Target, pResolution);
}

int VBoxWddmVModesAdd(PVBOXMP_DEVEXT pExt, uint32_t u32Target, const RTRECTSIZE *pResolution, BOOLEAN fTrancient)
{
    return vboxWddmVModesAdd(pExt, &g_VBoxWddmVModes, u32Target, pResolution, fTrancient);
}


static NTSTATUS vboxWddmChildStatusReportPerform(PVBOXMP_DEVEXT pDevExt, PVBOXVDMA_CHILD_STATUS pChildStatus, D3DDDI_VIDEO_PRESENT_TARGET_ID iChild)
{
    DXGK_CHILD_STATUS DdiChildStatus;

    Assert(iChild < UINT32_MAX/2);
    Assert(iChild < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays);

    PVBOXWDDM_TARGET pTarget = &pDevExt->aTargets[iChild];

    if ((pChildStatus->fFlags & VBOXVDMA_CHILD_STATUS_F_DISCONNECTED)
            && pTarget->fConnected)
    {
        /* report disconnected */
        memset(&DdiChildStatus, 0, sizeof (DdiChildStatus));
        DdiChildStatus.Type = StatusConnection;
        DdiChildStatus.ChildUid = iChild;
        DdiChildStatus.HotPlug.Connected = FALSE;

        LOG(("Reporting DISCONNECT to child %d", DdiChildStatus.ChildUid));

        NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbIndicateChildStatus(pDevExt->u.primary.DxgkInterface.DeviceHandle, &DdiChildStatus);
        if (!NT_SUCCESS(Status))
        {
            WARN(("DxgkCbIndicateChildStatus failed with Status (0x%x)", Status));
            return Status;
        }
        pTarget->fConnected = FALSE;
    }

    if ((pChildStatus->fFlags & VBOXVDMA_CHILD_STATUS_F_CONNECTED)
            && !pTarget->fConnected)
    {
        /* report disconnected */
        memset(&DdiChildStatus, 0, sizeof (DdiChildStatus));
        DdiChildStatus.Type = StatusConnection;
        DdiChildStatus.ChildUid = iChild;
        DdiChildStatus.HotPlug.Connected = TRUE;

        LOG(("Reporting CONNECT to child %d", DdiChildStatus.ChildUid));

        NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbIndicateChildStatus(pDevExt->u.primary.DxgkInterface.DeviceHandle, &DdiChildStatus);
        if (!NT_SUCCESS(Status))
        {
            WARN(("DxgkCbIndicateChildStatus failed with Status (0x%x)", Status));
            return Status;
        }
        pTarget->fConnected = TRUE;
    }

    if (pChildStatus->fFlags & VBOXVDMA_CHILD_STATUS_F_ROTATED)
    {
        /* report disconnected */
        memset(&DdiChildStatus, 0, sizeof (DdiChildStatus));
        DdiChildStatus.Type = StatusRotation;
        DdiChildStatus.ChildUid = iChild;
        DdiChildStatus.Rotation.Angle = pChildStatus->u8RotationAngle;

        LOG(("Reporting ROTATED to child %d", DdiChildStatus.ChildUid));

        NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbIndicateChildStatus(pDevExt->u.primary.DxgkInterface.DeviceHandle, &DdiChildStatus);
        if (!NT_SUCCESS(Status))
        {
            WARN(("DxgkCbIndicateChildStatus failed with Status (0x%x)", Status));
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS vboxWddmChildStatusHandleRequest(PVBOXMP_DEVEXT pDevExt, VBOXVDMACMD_CHILD_STATUS_IRQ *pBody)
{
    NTSTATUS Status = STATUS_SUCCESS;

    for (UINT i = 0; i < pBody->cInfos; ++i)
    {
        PVBOXVDMA_CHILD_STATUS pInfo = &pBody->aInfos[i];
        if (pBody->fFlags & VBOXVDMACMD_CHILD_STATUS_IRQ_F_APPLY_TO_ALL)
        {
            for (D3DDDI_VIDEO_PRESENT_TARGET_ID iChild = 0; iChild < (UINT)VBoxCommonFromDeviceExt(pDevExt)->cDisplays; ++iChild)
            {
                Status = vboxWddmChildStatusReportPerform(pDevExt, pInfo, iChild);
                if (!NT_SUCCESS(Status))
                {
                    WARN(("vboxWddmChildStatusReportPerform failed with Status (0x%x)", Status));
                    break;
                }
            }
        }
        else
        {
            Status = vboxWddmChildStatusReportPerform(pDevExt, pInfo, pInfo->iChild);
            if (!NT_SUCCESS(Status))
            {
                WARN(("vboxWddmChildStatusReportPerform failed with Status (0x%x)", Status));
                break;
            }
        }
    }

    return Status;
}

#ifdef VBOX_WDDM_MONITOR_REPLUG_IRQ
typedef struct VBOXWDDMCHILDSTATUSCB
{
    PVBOXVDMACBUF_DR pDr;
    PKEVENT pEvent;
} VBOXWDDMCHILDSTATUSCB, *PVBOXWDDMCHILDSTATUSCB;

static DECLCALLBACK(VOID) vboxWddmChildStatusReportCompletion(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, PVOID pvContext)
{
    /* we should be called from our DPC routine */
    Assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    PVBOXWDDMCHILDSTATUSCB pCtx = (PVBOXWDDMCHILDSTATUSCB)pvContext;
    PVBOXVDMACBUF_DR pDr = pCtx->pDr;
    VBOXVDMACMD                  RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
    VBOXVDMACMD_CHILD_STATUS_IRQ RT_UNTRUSTED_VOLATILE_HOST *pBody = VBOXVDMACMD_BODY(pHdr, VBOXVDMACMD_CHILD_STATUS_IRQ);

    vboxWddmChildStatusHandleRequest(pDevExt, pBody);

    vboxVdmaCBufDrFree(&pDevExt->u.primary.Vdma, pDr);

    if (pCtx->pEvent)
    {
        KeSetEvent(pCtx->pEvent, 0, FALSE);
    }
}
#endif

NTSTATUS VBoxWddmChildStatusReportReconnected(PVBOXMP_DEVEXT pDevExt, uint32_t iChild)
{
#ifdef VBOX_WDDM_MONITOR_REPLUG_IRQ
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UINT cbCmd = VBOXVDMACMD_SIZE_FROMBODYSIZE(sizeof (VBOXVDMACMD_CHILD_STATUS_IRQ));

    PVBOXVDMACBUF_DR pDr = vboxVdmaCBufDrCreate(&pDevExt->u.primary.Vdma, cbCmd);
    if (pDr)
    {
        // vboxVdmaCBufDrCreate zero initializes the pDr
        /* the command data follows the descriptor */
        pDr->fFlags = VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR;
        pDr->cbBuf = cbCmd;
        pDr->rc = VERR_NOT_IMPLEMENTED;

        VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
        pHdr->enmType = VBOXVDMACMD_TYPE_CHILD_STATUS_IRQ;
        pHdr->u32CmdSpecific = 0;

        VBOXVDMACMD_CHILD_STATUS_IRQ RT_UNTRUSTED_VOLATILE_HOST *pBody = VBOXVDMACMD_BODY(pHdr, VBOXVDMACMD_CHILD_STATUS_IRQ);
        pBody->cInfos = 1;
        if (iChild == D3DDDI_ID_ALL)
        {
            pBody->fFlags |= VBOXVDMACMD_CHILD_STATUS_IRQ_F_APPLY_TO_ALL;
        }
        pBody->aInfos[0].iChild = iChild;
        pBody->aInfos[0].fFlags = VBOXVDMA_CHILD_STATUS_F_DISCONNECTED | VBOXVDMA_CHILD_STATUS_F_CONNECTED;
        /* we're going to KeWaitForSingleObject */
        Assert(KeGetCurrentIrql() < DISPATCH_LEVEL);

        PVBOXVDMADDI_CMD pDdiCmd = VBOXVDMADDI_CMD_FROM_BUF_DR(pDr);
        VBOXWDDMCHILDSTATUSCB Ctx;
        KEVENT Event;
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Ctx.pDr = pDr;
        Ctx.pEvent = &Event;
        vboxVdmaDdiCmdInit(pDdiCmd, 0, 0, vboxWddmChildStatusReportCompletion, &Ctx);
        /* mark command as submitted & invisible for the dx runtime since dx did not originate it */
        vboxVdmaDdiCmdSubmittedNotDx(pDdiCmd);
        int rc = vboxVdmaCBufDrSubmit(pDevExt, &pDevExt->u.primary.Vdma, pDr);
        Assert(rc == VINF_SUCCESS);
        if (RT_SUCCESS(rc))
        {
            Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            AssertNtStatusSuccess(Status);
            return STATUS_SUCCESS;
        }

        Status = STATUS_UNSUCCESSFUL;

        vboxVdmaCBufDrFree(&pDevExt->u.primary.Vdma, pDr);
    }
    else
    {
        /** @todo try flushing.. */
        WARN(("vboxVdmaCBufDrCreate returned NULL"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
#else
    VBOXVDMACMD_CHILD_STATUS_IRQ Body = {0};
    Body.cInfos = 1;
    if (iChild == D3DDDI_ID_ALL)
    {
        Body.fFlags |= VBOXVDMACMD_CHILD_STATUS_IRQ_F_APPLY_TO_ALL;
    }
    Body.aInfos[0].iChild = iChild;
    Body.aInfos[0].fFlags = VBOXVDMA_CHILD_STATUS_F_DISCONNECTED | VBOXVDMA_CHILD_STATUS_F_CONNECTED;
    Assert(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    return vboxWddmChildStatusHandleRequest(pDevExt, &Body);
#endif
}

NTSTATUS VBoxWddmChildStatusConnect(PVBOXMP_DEVEXT pDevExt, uint32_t iChild, BOOLEAN fConnect)
{
#ifdef VBOX_WDDM_MONITOR_REPLUG_IRQ
# error "port me!"
#else
    Assert(iChild < (uint32_t)VBoxCommonFromDeviceExt(pDevExt)->cDisplays);
    NTSTATUS Status = STATUS_SUCCESS;
    VBOXVDMACMD_CHILD_STATUS_IRQ Body = {0};
    Body.cInfos = 1;
    Body.aInfos[0].iChild = iChild;
    Body.aInfos[0].fFlags = fConnect ? VBOXVDMA_CHILD_STATUS_F_CONNECTED : VBOXVDMA_CHILD_STATUS_F_DISCONNECTED;
    Assert(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    Status = vboxWddmChildStatusHandleRequest(pDevExt, &Body);
    if (!NT_SUCCESS(Status))
        WARN(("vboxWddmChildStatusHandleRequest failed Status 0x%x", Status));

    return Status;
#endif
}

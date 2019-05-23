/* $Id: VBoxMPVdma.cpp $ */
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
#include "VBoxMPVdma.h"
#include "VBoxMPVhwa.h"
#include <iprt/asm.h>
#include "VBoxMPCr.h"
#include <iprt/mem.h>

#ifdef VBOX_WITH_CROGL
# include <packer.h>
#endif

static NTSTATUS vboxVdmaCrCtlGetDefaultClientId(PVBOXMP_DEVEXT pDevExt, uint32_t *pu32ClienID);

NTSTATUS vboxVdmaPipeConstruct(PVBOXVDMAPIPE pPipe)
{
    KeInitializeSpinLock(&pPipe->SinchLock);
    KeInitializeEvent(&pPipe->Event, SynchronizationEvent, FALSE);
    InitializeListHead(&pPipe->CmdListHead);
    pPipe->enmState = VBOXVDMAPIPE_STATE_CREATED;
    pPipe->bNeedNotify = true;
    return STATUS_SUCCESS;
}

NTSTATUS vboxVdmaPipeSvrOpen(PVBOXVDMAPIPE pPipe)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;
    KeAcquireSpinLock(&pPipe->SinchLock, &OldIrql);
    Assert(pPipe->enmState == VBOXVDMAPIPE_STATE_CREATED);
    switch (pPipe->enmState)
    {
        case VBOXVDMAPIPE_STATE_CREATED:
            pPipe->enmState = VBOXVDMAPIPE_STATE_OPENNED;
            pPipe->bNeedNotify = false;
            break;
        case VBOXVDMAPIPE_STATE_OPENNED:
            pPipe->bNeedNotify = false;
            break;
        default:
            AssertBreakpoint();
            Status = STATUS_INVALID_PIPE_STATE;
            break;
    }

    KeReleaseSpinLock(&pPipe->SinchLock, OldIrql);
    return Status;
}

NTSTATUS vboxVdmaPipeSvrClose(PVBOXVDMAPIPE pPipe)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;
    KeAcquireSpinLock(&pPipe->SinchLock, &OldIrql);
    Assert(pPipe->enmState == VBOXVDMAPIPE_STATE_CLOSED
            || pPipe->enmState == VBOXVDMAPIPE_STATE_CLOSING);
    switch (pPipe->enmState)
    {
        case VBOXVDMAPIPE_STATE_CLOSING:
            pPipe->enmState = VBOXVDMAPIPE_STATE_CLOSED;
            break;
        case VBOXVDMAPIPE_STATE_CLOSED:
            break;
        default:
            AssertBreakpoint();
            Status = STATUS_INVALID_PIPE_STATE;
            break;
    }

    KeReleaseSpinLock(&pPipe->SinchLock, OldIrql);
    return Status;
}

NTSTATUS vboxVdmaPipeCltClose(PVBOXVDMAPIPE pPipe)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;
    KeAcquireSpinLock(&pPipe->SinchLock, &OldIrql);
    bool bNeedNotify = false;
    Assert(pPipe->enmState == VBOXVDMAPIPE_STATE_OPENNED
                || pPipe->enmState == VBOXVDMAPIPE_STATE_CREATED
                ||  pPipe->enmState == VBOXVDMAPIPE_STATE_CLOSED);
    switch (pPipe->enmState)
    {
        case VBOXVDMAPIPE_STATE_OPENNED:
            pPipe->enmState = VBOXVDMAPIPE_STATE_CLOSING;
            bNeedNotify = pPipe->bNeedNotify;
            pPipe->bNeedNotify = false;
            break;
        case VBOXVDMAPIPE_STATE_CREATED:
            pPipe->enmState = VBOXVDMAPIPE_STATE_CLOSED;
            pPipe->bNeedNotify = false;
            break;
        case VBOXVDMAPIPE_STATE_CLOSED:
            break;
        default:
            AssertBreakpoint();
            Status = STATUS_INVALID_PIPE_STATE;
            break;
    }

    KeReleaseSpinLock(&pPipe->SinchLock, OldIrql);

    if (bNeedNotify)
    {
        KeSetEvent(&pPipe->Event, 0, FALSE);
    }
    return Status;
}

NTSTATUS vboxVdmaPipeDestruct(PVBOXVDMAPIPE pPipe)
{
    Assert(pPipe->enmState == VBOXVDMAPIPE_STATE_CLOSED
            || pPipe->enmState == VBOXVDMAPIPE_STATE_CREATED);
    /* ensure the pipe is closed */
    NTSTATUS Status = vboxVdmaPipeCltClose(pPipe);
    AssertNtStatusSuccess(Status);

    Assert(pPipe->enmState == VBOXVDMAPIPE_STATE_CLOSED);

    return Status;
}

NTSTATUS vboxVdmaPipeSvrCmdGetList(PVBOXVDMAPIPE pPipe, PLIST_ENTRY pDetachHead)
{
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_SUCCESS;
    VBOXVDMAPIPE_STATE enmState = VBOXVDMAPIPE_STATE_CLOSED;
    do
    {
        bool bListEmpty = true;
        KeAcquireSpinLock(&pPipe->SinchLock, &OldIrql);
        Assert(pPipe->enmState == VBOXVDMAPIPE_STATE_OPENNED
                || pPipe->enmState == VBOXVDMAPIPE_STATE_CLOSING);
        Assert(pPipe->enmState >= VBOXVDMAPIPE_STATE_OPENNED);
        enmState = pPipe->enmState;
        if (enmState >= VBOXVDMAPIPE_STATE_OPENNED)
        {
            vboxVideoLeDetach(&pPipe->CmdListHead, pDetachHead);
            bListEmpty = !!(IsListEmpty(pDetachHead));
            pPipe->bNeedNotify = bListEmpty;
        }
        else
        {
            KeReleaseSpinLock(&pPipe->SinchLock, OldIrql);
            Status = STATUS_INVALID_PIPE_STATE;
            break;
        }

        KeReleaseSpinLock(&pPipe->SinchLock, OldIrql);

        if (!bListEmpty)
        {
            AssertNtStatusSuccess(Status);
            break;
        }

        if (enmState == VBOXVDMAPIPE_STATE_OPENNED)
        {
            Status = KeWaitForSingleObject(&pPipe->Event, Executive, KernelMode, FALSE, NULL /* PLARGE_INTEGER Timeout */);
            AssertNtStatusSuccess(Status);
            if (Status != STATUS_SUCCESS)
                break;
        }
        else
        {
            Assert(enmState == VBOXVDMAPIPE_STATE_CLOSING);
            Status = STATUS_PIPE_CLOSING;
            break;
        }
    } while (1);

    return Status;
}

NTSTATUS vboxVdmaPipeCltCmdPut(PVBOXVDMAPIPE pPipe, PVBOXVDMAPIPE_CMD_HDR pCmd)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;
    bool bNeedNotify = false;

    KeAcquireSpinLock(&pPipe->SinchLock, &OldIrql);

    Assert(pPipe->enmState == VBOXVDMAPIPE_STATE_OPENNED);
    if (pPipe->enmState == VBOXVDMAPIPE_STATE_OPENNED)
    {
        bNeedNotify = pPipe->bNeedNotify;
        InsertHeadList(&pPipe->CmdListHead, &pCmd->ListEntry);
        pPipe->bNeedNotify = false;
    }
    else
        Status = STATUS_INVALID_PIPE_STATE;

    KeReleaseSpinLock(&pPipe->SinchLock, OldIrql);

    if (bNeedNotify)
    {
        KeSetEvent(&pPipe->Event, 0, FALSE);
    }

    return Status;
}

DECLINLINE(void) vboxVdmaDirtyRectsCalcIntersection(const RECT *pArea, const VBOXWDDM_RECTS_INFO *pRects, PVBOXWDDM_RECTS_INFO pResult)
{
    uint32_t cRects = 0;
    for (uint32_t i = 0; i < pRects->cRects; ++i)
    {
        if (vboxWddmRectIntersection(pArea, &pRects->aRects[i], &pResult->aRects[cRects]))
        {
            ++cRects;
        }
    }

    pResult->cRects = cRects;
}

DECLINLINE(bool) vboxVdmaDirtyRectsHasIntersections(const RECT *paRects1, uint32_t cRects1, const RECT *paRects2, uint32_t cRects2)
{
    RECT tmpRect;
    for (uint32_t i = 0; i < cRects1; ++i)
    {
        const RECT * pRect1 = &paRects1[i];
        for (uint32_t j = 0; j < cRects2; ++j)
        {
            const RECT * pRect2 = &paRects2[j];
            if (vboxWddmRectIntersection(pRect1, pRect2, &tmpRect))
                return true;
        }
    }
    return false;
}

DECLINLINE(bool) vboxVdmaDirtyRectsIsCover(const RECT *paRects, uint32_t cRects, const RECT *paRectsCovered, uint32_t cRectsCovered)
{
    for (uint32_t i = 0; i < cRectsCovered; ++i)
    {
        const RECT * pRectCovered = &paRectsCovered[i];
        uint32_t j = 0;
        for (; j < cRects; ++j)
        {
            const RECT * pRect = &paRects[j];
            if (vboxWddmRectIsCoveres(pRect, pRectCovered))
                break;
        }
        if (j == cRects)
            return false;
    }
    return true;
}
#ifdef VBOX_WITH_CROGL
NTSTATUS vboxVdmaPostHideSwapchain(PVBOXWDDM_SWAPCHAIN pSwapchain)
{
    Assert(KeGetCurrentIrql() < DISPATCH_LEVEL);
    uint32_t cbCmdInternal = VBOXVIDEOCM_CMD_RECTS_INTERNAL_SIZE4CRECTS(0);
    PVBOXVIDEOCM_CMD_RECTS_INTERNAL pCmdInternal =
            (PVBOXVIDEOCM_CMD_RECTS_INTERNAL)vboxVideoCmCmdCreate(&pSwapchain->pContext->CmContext, cbCmdInternal);
    Assert(pCmdInternal);
    if (pCmdInternal)
    {
        pCmdInternal->hSwapchainUm = pSwapchain->hSwapchainUm;
        pCmdInternal->Cmd.fFlags.Value = 0;
        pCmdInternal->Cmd.fFlags.bAddHiddenRects = 1;
        pCmdInternal->Cmd.fFlags.bHide = 1;
        pCmdInternal->Cmd.RectsInfo.cRects = 0;
        vboxVideoCmCmdSubmit(pCmdInternal, VBOXVIDEOCM_SUBMITSIZE_DEFAULT);
        return STATUS_SUCCESS;
    }
    return STATUS_NO_MEMORY;
}
#endif
static VOID vboxWddmBltPipeRectsTranslate(VBOXVDMAPIPE_RECTS *pRects, int x, int y)
{
    vboxWddmRectTranslate(&pRects->ContextRect, x, y);

    for (UINT i = 0; i < pRects->UpdateRects.cRects; ++i)
    {
        vboxWddmRectTranslate(&pRects->UpdateRects.aRects[i], x, y);
    }
}

static VBOXVDMAPIPE_RECTS * vboxWddmBltPipeRectsDup(const VBOXVDMAPIPE_RECTS *pRects)
{
    const size_t cbDup = RT_OFFSETOF(VBOXVDMAPIPE_RECTS, UpdateRects.aRects[pRects->UpdateRects.cRects]);
    VBOXVDMAPIPE_RECTS *pDup = (VBOXVDMAPIPE_RECTS*)vboxWddmMemAllocZero(cbDup);
    if (!pDup)
    {
        WARN(("vboxWddmMemAllocZero failed"));
        return NULL;
    }
    memcpy(pDup, pRects, cbDup);
    return pDup;
}
#ifdef VBOX_WITH_CROGL
typedef struct VBOXMP_VDMACR_WRITECOMPLETION
{
    void *pvBufferToFree;
} VBOXMP_VDMACR_WRITECOMPLETION, *PVBOXMP_VDMACR_WRITECOMPLETION;

static DECLCALLBACK(void) vboxVdmaCrWriteCompletion(PVBOXMP_CRSHGSMITRANSPORT pCon, int rc, void RT_UNTRUSTED_VOLATILE_HOST *pvCtx)
{
    RT_NOREF(rc);
    PVBOXMP_VDMACR_WRITECOMPLETION pData = (PVBOXMP_VDMACR_WRITECOMPLETION)pvCtx;
    void *pvBufferToFree = pData->pvBufferToFree;
    if (pvBufferToFree)
        VBoxMpCrShgsmiTransportBufFree(pCon, pvBufferToFree);

    VBoxMpCrShgsmiTransportCmdTermWriteAsync(pCon, pvCtx);
}

typedef struct VBOXMP_VDMACR_WRITEREADCOMPLETION
{
    void RT_UNTRUSTED_VOLATILE_HOST *pvBufferToFree;
    void RT_UNTRUSTED_VOLATILE_HOST *pvContext;
} VBOXMP_VDMACR_WRITEREADCOMPLETION, *PVBOXMP_VDMACR_WRITEREADCOMPLETION;

void vboxVdmaCrSubmitWriteReadAsyncGenericCompletion(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvCtx)
{
    VBOXMP_VDMACR_WRITEREADCOMPLETION RT_UNTRUSTED_VOLATILE_HOST *pData
        = (VBOXMP_VDMACR_WRITEREADCOMPLETION RT_UNTRUSTED_VOLATILE_HOST *)pvCtx;
    void RT_UNTRUSTED_VOLATILE_HOST *pvBufferToFree = pData->pvBufferToFree;
    if (pvBufferToFree)
        VBoxMpCrShgsmiTransportBufFree(pCon, pvBufferToFree);

    VBoxMpCrShgsmiTransportCmdTermWriteReadAsync(pCon, pvCtx);
}

static NTSTATUS vboxVdmaCrSubmitWriteReadAsync(PVBOXMP_DEVEXT pDevExt, VBOXMP_CRPACKER *pCrPacker, uint32_t u32CrConClientID,
                                               PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEREADASYNC_COMPLETION pfnCompletion,
                                               void RT_UNTRUSTED_VOLATILE_HOST *pvCompletion)
{
    Assert(u32CrConClientID);
    NTSTATUS Status = STATUS_SUCCESS;
    uint32_t cbBuffer;
    void * pvPackBuffer;
    void * pvBuffer = VBoxMpCrPackerTxBufferComplete(pCrPacker, &cbBuffer, &pvPackBuffer);
    if (pvBuffer)
    {
        VBOXMP_VDMACR_WRITEREADCOMPLETION RT_UNTRUSTED_VOLATILE_HOST *pCompletionData
            = (VBOXMP_VDMACR_WRITEREADCOMPLETION RT_UNTRUSTED_VOLATILE_HOST  *)
              VBoxMpCrShgsmiTransportCmdCreateWriteReadAsync(&pDevExt->CrHgsmiTransport, u32CrConClientID, pvBuffer, cbBuffer,
                                                             pfnCompletion, sizeof(*pCompletionData));
        if (pCompletionData)
        {
            pCompletionData->pvBufferToFree = pvPackBuffer;
            pCompletionData->pvContext = pvCompletion;
            int rc = VBoxMpCrShgsmiTransportCmdSubmitWriteReadAsync(&pDevExt->CrHgsmiTransport, pCompletionData);
            if (RT_SUCCESS(rc))
            {
                return STATUS_SUCCESS;
            }
            WARN(("VBoxMpCrShgsmiTransportCmdSubmitWriteReadAsync failed, rc %d", rc));
            Status = STATUS_UNSUCCESSFUL;
            VBoxMpCrShgsmiTransportCmdTermWriteReadAsync(&pDevExt->CrHgsmiTransport, pCompletionData);
        }
        else
        {
            WARN(("VBoxMpCrShgsmiTransportCmdCreateWriteReadAsync failed"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}

NTSTATUS vboxVdmaCrSubmitWriteAsync(PVBOXMP_DEVEXT pDevExt, VBOXMP_CRPACKER *pCrPacker, uint32_t u32CrConClientID)
{
    Assert(u32CrConClientID);
    NTSTATUS Status = STATUS_SUCCESS;
    uint32_t cbBuffer;
    void * pvPackBuffer;
    void * pvBuffer = VBoxMpCrPackerTxBufferComplete(pCrPacker, &cbBuffer, &pvPackBuffer);
    if (pvBuffer)
    {
        VBOXMP_VDMACR_WRITECOMPLETION RT_UNTRUSTED_VOLATILE_HOST *pCompletionData
            = (VBOXMP_VDMACR_WRITECOMPLETION RT_UNTRUSTED_VOLATILE_HOST *)
              VBoxMpCrShgsmiTransportCmdCreateWriteAsync(&pDevExt->CrHgsmiTransport, u32CrConClientID, pvBuffer, cbBuffer,
                                                         vboxVdmaCrWriteCompletion, sizeof(*pCompletionData));
        if (pCompletionData)
        {
            pCompletionData->pvBufferToFree = pvPackBuffer;
            int rc = VBoxMpCrShgsmiTransportCmdSubmitWriteAsync(&pDevExt->CrHgsmiTransport, pCompletionData);
            if (RT_SUCCESS(rc))
            {
                return STATUS_SUCCESS;
            }
            WARN(("VBoxMpCrShgsmiTransportCmdSubmitWriteAsync failed, rc %d", rc));
            Status = STATUS_UNSUCCESSFUL;
            VBoxMpCrShgsmiTransportCmdTermWriteAsync(&pDevExt->CrHgsmiTransport, pCompletionData);
        }
        else
        {
            WARN(("VBoxMpCrShgsmiTransportCmdCreateWriteAsync failed"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}

static NTSTATUS vboxVdmaVRegGet(PVBOXWDDM_SWAPCHAIN pSwapchain, const RTRECT *pCtxRect, uint32_t *pcVRects, RTRECT **ppVRectsBuff, uint32_t *pcVRectsBuff)
{
    RTRECT *pVRectsBuff = *ppVRectsBuff;
    uint32_t cVRectsBuff = *pcVRectsBuff;
    uint32_t cVRects = VBoxVrListRectsCount(&pSwapchain->VisibleRegions);
    if (cVRectsBuff < cVRects)
    {
        if (pVRectsBuff)
            vboxWddmMemFree(pVRectsBuff);
        pVRectsBuff = (RTRECT*)vboxWddmMemAlloc(cVRects * sizeof (pVRectsBuff[0]));
        if (!pVRectsBuff)
        {
            WARN(("vboxWddmMemAlloc failed"));
            *pcVRectsBuff = 0;
            *ppVRectsBuff = NULL;
            *pcVRects = NULL;
            return STATUS_NO_MEMORY;
        }

        cVRectsBuff = cVRects;
        *pcVRectsBuff = cVRectsBuff;
        *ppVRectsBuff = pVRectsBuff;
    }

    int rc = VBoxVrListRectsGet(&pSwapchain->VisibleRegions, cVRects, pVRectsBuff);
    AssertRC(rc);
    if (pCtxRect->xLeft || pCtxRect->yTop)
    {
        for (UINT i = 0; i < cVRects; ++i)
        {
            VBoxRectTranslate(&pVRectsBuff[i], -pCtxRect->xLeft, -pCtxRect->yTop);
        }
    }

    *pcVRects = cVRects;
    return STATUS_SUCCESS;
}


/**
 * @param pDevExt
 */
static NTSTATUS vboxVdmaProcessVRegCmdLegacy(PVBOXMP_DEVEXT pDevExt,
        VBOXMP_CRPACKER *pCrPacker,
        uint32_t u32CrConClientID,
        PVBOXWDDM_SOURCE pSource,
        PVBOXWDDM_SWAPCHAIN pSwapchain,
        const RECT *pSrcRect,
        const VBOXVDMAPIPE_RECTS *pContextRects)
{
    VBOXVDMAPIPE_RECTS *pRectsToFree = NULL;
    POINT pos = pSource->VScreenPos;
    if (pos.x || pos.y)
    {
        pRectsToFree = vboxWddmBltPipeRectsDup(pContextRects);
        /* note: do NOT translate the src rect since it is used for screen pos calculation */
        vboxWddmBltPipeRectsTranslate(pRectsToFree, pos.x, pos.y);
        pContextRects = pRectsToFree;
    }
    const VBOXWDDM_RECTS_INFO *pRects = &pContextRects->UpdateRects;
    NTSTATUS Status = STATUS_SUCCESS;
    int rc;
    bool fCurChanged = FALSE;
    bool fCurRectChanged = FALSE;
    POINT CurPos = { 0, 0 }; /* MSC is confused (me too), maybe used uninitialized. */
    RTRECT *pVRectsBuff = NULL;
    uint32_t cVRectsBuff = 0;
    VBOXWDDM_CTXLOCK_DATA

    VBOXWDDM_CTXLOCK_LOCK(pDevExt);

    if (pSwapchain)
    {
        CurPos.x = pContextRects->ContextRect.left - pSrcRect->left;
        CurPos.y = pContextRects->ContextRect.top - pSrcRect->top;

        if (CurPos.x != pSwapchain->Pos.x || CurPos.y != pSwapchain->Pos.y)
        {
#if 0
            if (pSwapchain->Pos.x != VBOXWDDM_INVALID_COORD)
                VBoxVrListTranslate(&pSwapchain->VisibleRegions, pSwapchain->Pos.x - CurPos.x, pSwapchain->Pos.y - CurPos.y);
            else
#endif
                VBoxVrListClear(&pSwapchain->VisibleRegions);
            fCurRectChanged = TRUE;
            pSwapchain->Pos = CurPos;
        }

        rc = VBoxVrListRectsAdd(&pSwapchain->VisibleRegions, pRects->cRects, (const RTRECT*)pRects->aRects, &fCurChanged);
        if (!RT_SUCCESS(rc))
        {
            WARN(("VBoxWddmVrListRectsAdd failed, rc %d!", rc));
            Status = STATUS_UNSUCCESSFUL;
            goto done;
        }


        /* visible rects of different windows do not intersect,
         * so if the given window visible rects did not increase, others have not changed either */
        if (!fCurChanged && !fCurRectChanged)
            goto done;
    }

    /* before posting the add visible rects diff, we need to first hide rects for other windows */

    for (PLIST_ENTRY pCur = pDevExt->SwapchainList3D.Flink; pCur != &pDevExt->SwapchainList3D; pCur = pCur->Flink)
    {
        if (pCur != &pSwapchain->DevExtListEntry)
        {
            PVBOXWDDM_SWAPCHAIN pCurSwapchain = VBOXWDDMENTRY_2_SWAPCHAIN(pCur);
            PVBOXWDDM_CONTEXT pCurContext = pCurSwapchain->pContext;
            PVBOXMP_CRPACKER pCurPacker = &pCurContext->CrPacker;
            bool fChanged = FALSE;

            rc = VBoxVrListRectsSubst(&pCurSwapchain->VisibleRegions, pRects->cRects, (const RTRECT*)pRects->aRects, &fChanged);
            if (!RT_SUCCESS(rc))
            {
                WARN(("VBoxWddmVrListRectsAdd failed, rc %d!", rc));
                Status = STATUS_UNSUCCESSFUL;
                goto done;
            }

            if (!fChanged)
                continue;

            uint32_t cVRects;
            RTRECT CurCtxRect;
            CurCtxRect.xLeft = pCurSwapchain->Pos.x;
            CurCtxRect.yTop = pCurSwapchain->Pos.y;
            CurCtxRect.xRight = CurCtxRect.xLeft + pCurSwapchain->width;
            CurCtxRect.yBottom = CurCtxRect.yTop + pCurSwapchain->height;
            Status = vboxVdmaVRegGet(pCurSwapchain, &CurCtxRect, &cVRects, &pVRectsBuff, &cVRectsBuff);
            if (!NT_SUCCESS(Status))
            {
                WARN(("vboxVdmaVRegGet Status 0x%x", Status));
                goto done;
            }

            ;
            uint32_t cbCommandBuffer = VBOXMP_CRCMD_HEADER_SIZE
                                     + VBOXMP_CRCMD_SIZE_WINDOWVISIBLEREGIONS(cVRects);
            void RT_UNTRUSTED_VOLATILE_HOST *pvCommandBuffer = VBoxMpCrShgsmiTransportBufAlloc(&pDevExt->CrHgsmiTransport,
                                                                                               cbCommandBuffer);
            if (!pvCommandBuffer)
            {
                WARN(("VBoxMpCrShgsmiTransportBufAlloc failed!"));
                Status = VERR_OUT_OF_RESOURCES;
                goto done;
            }

            VBoxMpCrPackerTxBufferInit(pCurPacker, pvCommandBuffer, cbCommandBuffer, 1);

            Assert(pCurSwapchain->winHostID);
            crPackWindowVisibleRegion(&pCurPacker->CrPacker, pCurSwapchain->winHostID, cVRects, (GLint*)pVRectsBuff);

            Status = vboxVdmaCrSubmitWriteAsync(pDevExt, pCurPacker, pCurContext->u32CrConClientID);
            if (!NT_SUCCESS(Status))
            {
                WARN(("vboxVdmaCrSubmitWriteAsync failed Status 0x%x", Status));
                VBoxMpCrShgsmiTransportBufFree(&pDevExt->CrHgsmiTransport, pvCommandBuffer);
            }
        }
    }

    if (!pSwapchain)
        goto done;

    uint32_t cbCommandBuffer = VBOXMP_CRCMD_HEADER_SIZE, cCommands = 0;

    uint32_t cVRects;
    Status = vboxVdmaVRegGet(pSwapchain, (const RTRECT *)&pContextRects->ContextRect, &cVRects, &pVRectsBuff, &cVRectsBuff);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaVRegGet Status 0x%x", Status));
        goto done;
    }

    ++cCommands;
    cbCommandBuffer += VBOXMP_CRCMD_SIZE_WINDOWVISIBLEREGIONS(cVRects);

    if (fCurRectChanged && fCurChanged)
    {
        ++cCommands;
        cbCommandBuffer += VBOXMP_CRCMD_SIZE_WINDOWPOSITION;
    }

    if (!pSwapchain->fExposed)
    {
        ++cCommands;
        cbCommandBuffer += VBOXMP_CRCMD_SIZE_WINDOWSHOW;
        ++cCommands;
        cbCommandBuffer += VBOXMP_CRCMD_SIZE_WINDOWSIZE;
    }

    void RT_UNTRUSTED_VOLATILE_HOST *pvCommandBuffer = VBoxMpCrShgsmiTransportBufAlloc(&pDevExt->CrHgsmiTransport, cbCommandBuffer);
    if (!pvCommandBuffer)
    {
        WARN(("VBoxMpCrShgsmiTransportBufAlloc failed!"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    VBoxMpCrPackerTxBufferInit(pCrPacker, pvCommandBuffer, cbCommandBuffer, cCommands);

    Assert(pSwapchain->winHostID);

    if (fCurRectChanged && fCurChanged)
        crPackWindowPosition(&pCrPacker->CrPacker, pSwapchain->winHostID, CurPos.x, CurPos.y);

    if (!pSwapchain->fExposed)
    {
        crPackWindowSize(&pCrPacker->CrPacker, pSwapchain->winHostID, pSwapchain->width, pSwapchain->height);
        crPackWindowShow(&pCrPacker->CrPacker, pSwapchain->winHostID, TRUE);
        pSwapchain->fExposed = TRUE;
    }

    crPackWindowVisibleRegion(&pCrPacker->CrPacker, pSwapchain->winHostID, cVRects, (GLint*)pVRectsBuff);

    Status = vboxVdmaCrSubmitWriteAsync(pDevExt, pCrPacker, u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrSubmitWriteAsync failed Status 0x%x", Status));
        VBoxMpCrShgsmiTransportBufFree(&pDevExt->CrHgsmiTransport, pvCommandBuffer);
    }

done:
    VBOXWDDM_CTXLOCK_UNLOCK(pDevExt);

    if (pRectsToFree)
        vboxWddmMemFree(pRectsToFree);

    if (pVRectsBuff)
        vboxWddmMemFree(pVRectsBuff);

    return Status;
}
#endif

static NTSTATUS vboxVdmaGgDmaColorFill(PVBOXMP_DEVEXT pDevExt, VBOXVDMA_CLRFILL *pCF)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    Assert (pDevExt->pvVisibleVram);
    if (pDevExt->pvVisibleVram)
    {
        PVBOXWDDM_ALLOCATION pAlloc = pCF->Alloc.pAlloc;
        if (pAlloc->AllocData.Addr.SegmentId && pAlloc->AllocData.Addr.SegmentId != 1)
        {
            WARN(("request to collor fill invalid allocation"));
            return STATUS_INVALID_PARAMETER;
        }

        VBOXVIDEOOFFSET offVram = vboxWddmAddrFramOffset(&pAlloc->AllocData.Addr);
        if (offVram != VBOXVIDEOOFFSET_VOID)
        {
            RECT UnionRect = {0};
            uint8_t *pvMem = pDevExt->pvVisibleVram + offVram;
            UINT bpp = pAlloc->AllocData.SurfDesc.bpp;
            Assert(bpp);
            Assert(((bpp * pAlloc->AllocData.SurfDesc.width) >> 3) == pAlloc->AllocData.SurfDesc.pitch);
            switch (bpp)
            {
                case 32:
                {
                    uint8_t bytestPP = bpp >> 3;
                    for (UINT i = 0; i < pCF->Rects.cRects; ++i)
                    {
                        RECT *pRect = &pCF->Rects.aRects[i];
                        for (LONG ir = pRect->top; ir < pRect->bottom; ++ir)
                        {
                            uint32_t * pvU32Mem = (uint32_t*)(pvMem + (ir * pAlloc->AllocData.SurfDesc.pitch) + (pRect->left * bytestPP));
                            uint32_t cRaw = pRect->right - pRect->left;
                            Assert(pRect->left >= 0);
                            Assert(pRect->right <= (LONG)pAlloc->AllocData.SurfDesc.width);
                            Assert(pRect->top >= 0);
                            Assert(pRect->bottom <= (LONG)pAlloc->AllocData.SurfDesc.height);
                            for (UINT j = 0; j < cRaw; ++j)
                            {
                                *pvU32Mem = pCF->Color;
                                ++pvU32Mem;
                            }
                        }
                        vboxWddmRectUnited(&UnionRect, &UnionRect, pRect);
                    }
                    Status = STATUS_SUCCESS;
                    break;
                }
                case 16:
                case 8:
                default:
                    AssertBreakpoint();
                    break;
            }

            if (Status == STATUS_SUCCESS)
            {
                if (pAlloc->AllocData.SurfDesc.VidPnSourceId != D3DDDI_ID_UNINITIALIZED
                        && VBOXWDDM_IS_FB_ALLOCATION(pDevExt, pAlloc)
                        && pAlloc->bVisible
                        )
                {
                    if (!vboxWddmRectIsEmpty(&UnionRect))
                    {
                        PVBOXWDDM_SOURCE pSource = &pDevExt->aSources[pCF->Alloc.pAlloc->AllocData.SurfDesc.VidPnSourceId];
                        uint32_t cUnlockedVBVADisabled = ASMAtomicReadU32(&pDevExt->cUnlockedVBVADisabled);
                        if (!cUnlockedVBVADisabled)
                        {
                            VBOXVBVA_OP(ReportDirtyRect, pDevExt, pSource, &UnionRect);
                        }
                        else
                        {
                            VBOXVBVA_OP_WITHLOCK(ReportDirtyRect, pDevExt, pSource, &UnionRect);
                        }
                    }
                }
                else
                {
                    AssertBreakpoint();
                }
            }
        }
        else
            WARN(("invalid offVram"));
    }

    return Status;
}

NTSTATUS vboxVdmaGgDmaBltPerform(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_ALLOC_DATA pSrcAlloc, RECT* pSrcRect,
        PVBOXWDDM_ALLOC_DATA pDstAlloc, RECT* pDstRect)
{
    uint8_t* pvVramBase = pDevExt->pvVisibleVram;
    /* we do not support stretching */
    uint32_t srcWidth = pSrcRect->right - pSrcRect->left;
    uint32_t srcHeight = pSrcRect->bottom - pSrcRect->top;
    uint32_t dstWidth = pDstRect->right - pDstRect->left;
    uint32_t dstHeight = pDstRect->bottom - pDstRect->top;
    Assert(srcHeight == dstHeight);
    Assert(dstWidth == srcWidth);
    Assert(pDstAlloc->Addr.offVram != VBOXVIDEOOFFSET_VOID);
    Assert(pSrcAlloc->Addr.offVram != VBOXVIDEOOFFSET_VOID);
    D3DDDIFORMAT enmSrcFormat, enmDstFormat;

    enmSrcFormat = pSrcAlloc->SurfDesc.format;
    enmDstFormat = pDstAlloc->SurfDesc.format;

    if (pDstAlloc->Addr.SegmentId && pDstAlloc->Addr.SegmentId != 1)
    {
        WARN(("request to collor blit invalid allocation"));
        return STATUS_INVALID_PARAMETER;
    }

    if (pSrcAlloc->Addr.SegmentId && pSrcAlloc->Addr.SegmentId != 1)
    {
        WARN(("request to collor blit invalid allocation"));
        return STATUS_INVALID_PARAMETER;
    }

    if (enmSrcFormat != enmDstFormat)
    {
        /* just ignore the alpha component
         * this is ok since our software-based stuff can not handle alpha channel in any way */
        enmSrcFormat = vboxWddmFmtNoAlphaFormat(enmSrcFormat);
        enmDstFormat = vboxWddmFmtNoAlphaFormat(enmDstFormat);
        if (enmSrcFormat != enmDstFormat)
        {
            WARN(("color conversion src(%d), dst(%d) not supported!", pSrcAlloc->SurfDesc.format, pDstAlloc->SurfDesc.format));
            return STATUS_INVALID_PARAMETER;
        }
    }
    if (srcHeight != dstHeight)
            return STATUS_INVALID_PARAMETER;
    if (srcWidth != dstWidth)
            return STATUS_INVALID_PARAMETER;
    if (pDstAlloc->Addr.offVram == VBOXVIDEOOFFSET_VOID)
        return STATUS_INVALID_PARAMETER;
    if (pSrcAlloc->Addr.offVram == VBOXVIDEOOFFSET_VOID)
        return STATUS_INVALID_PARAMETER;

    uint8_t *pvDstSurf = pDstAlloc->Addr.SegmentId ? pvVramBase + pDstAlloc->Addr.offVram : (uint8_t*)pDstAlloc->Addr.pvMem;
    uint8_t *pvSrcSurf = pSrcAlloc->Addr.SegmentId ? pvVramBase + pSrcAlloc->Addr.offVram : (uint8_t*)pSrcAlloc->Addr.pvMem;

    if (pDstAlloc->SurfDesc.width == dstWidth
            && pSrcAlloc->SurfDesc.width == srcWidth
            && pSrcAlloc->SurfDesc.width == pDstAlloc->SurfDesc.width)
    {
        Assert(!pDstRect->left);
        Assert(!pSrcRect->left);
        uint32_t cbDstOff = vboxWddmCalcOffXYrd(0 /* x */, pDstRect->top, pDstAlloc->SurfDesc.pitch, pDstAlloc->SurfDesc.format);
        uint32_t cbSrcOff = vboxWddmCalcOffXYrd(0 /* x */, pSrcRect->top, pSrcAlloc->SurfDesc.pitch, pSrcAlloc->SurfDesc.format);
        uint32_t cbSize = vboxWddmCalcSize(pDstAlloc->SurfDesc.pitch, dstHeight, pDstAlloc->SurfDesc.format);
        memcpy(pvDstSurf + cbDstOff, pvSrcSurf + cbSrcOff, cbSize);
    }
    else
    {
        uint32_t cbDstLine =  vboxWddmCalcRowSize(pDstRect->left, pDstRect->right, pDstAlloc->SurfDesc.format);
        uint32_t offDstStart = vboxWddmCalcOffXYrd(pDstRect->left, pDstRect->top, pDstAlloc->SurfDesc.pitch, pDstAlloc->SurfDesc.format);
        Assert(cbDstLine <= pDstAlloc->SurfDesc.pitch);
        uint32_t cbDstSkip = pDstAlloc->SurfDesc.pitch;
        uint8_t * pvDstStart = pvDstSurf + offDstStart;

        uint32_t cbSrcLine = vboxWddmCalcRowSize(pSrcRect->left, pSrcRect->right, pSrcAlloc->SurfDesc.format);
        uint32_t offSrcStart = vboxWddmCalcOffXYrd(pSrcRect->left, pSrcRect->top, pSrcAlloc->SurfDesc.pitch, pSrcAlloc->SurfDesc.format);
        Assert(cbSrcLine <= pSrcAlloc->SurfDesc.pitch); NOREF(cbSrcLine);
        uint32_t cbSrcSkip = pSrcAlloc->SurfDesc.pitch;
        const uint8_t * pvSrcStart = pvSrcSurf + offSrcStart;

        uint32_t cRows = vboxWddmCalcNumRows(pDstRect->top, pDstRect->bottom, pDstAlloc->SurfDesc.format);

        Assert(cbDstLine == cbSrcLine);

        for (uint32_t i = 0; i < cRows; ++i)
        {
            memcpy(pvDstStart, pvSrcStart, cbDstLine);
            pvDstStart += cbDstSkip;
            pvSrcStart += cbSrcSkip;
        }
    }
    return STATUS_SUCCESS;
}

/*
 * @return on success the number of bytes the command contained, otherwise - VERR_xxx error code
 */
static NTSTATUS vboxVdmaGgDmaBlt(PVBOXMP_DEVEXT pDevExt, PVBOXVDMA_BLT pBlt)
{
    /* we do not support stretching for now */
    Assert(pBlt->SrcRect.right - pBlt->SrcRect.left == pBlt->DstRects.ContextRect.right - pBlt->DstRects.ContextRect.left);
    Assert(pBlt->SrcRect.bottom - pBlt->SrcRect.top == pBlt->DstRects.ContextRect.bottom - pBlt->DstRects.ContextRect.top);
    if (pBlt->SrcRect.right - pBlt->SrcRect.left != pBlt->DstRects.ContextRect.right - pBlt->DstRects.ContextRect.left)
        return STATUS_INVALID_PARAMETER;
    if (pBlt->SrcRect.bottom - pBlt->SrcRect.top != pBlt->DstRects.ContextRect.bottom - pBlt->DstRects.ContextRect.top)
        return STATUS_INVALID_PARAMETER;
    Assert(pBlt->DstRects.UpdateRects.cRects);

    NTSTATUS Status = STATUS_SUCCESS;

    if (pBlt->DstRects.UpdateRects.cRects)
    {
        for (uint32_t i = 0; i < pBlt->DstRects.UpdateRects.cRects; ++i)
        {
            RECT SrcRect;
            vboxWddmRectTranslated(&SrcRect, &pBlt->DstRects.UpdateRects.aRects[i], -pBlt->DstRects.ContextRect.left, -pBlt->DstRects.ContextRect.top);

            Status = vboxVdmaGgDmaBltPerform(pDevExt, &pBlt->SrcAlloc.pAlloc->AllocData, &SrcRect,
                    &pBlt->DstAlloc.pAlloc->AllocData, &pBlt->DstRects.UpdateRects.aRects[i]);
            AssertNtStatusSuccess(Status);
            if (Status != STATUS_SUCCESS)
                return Status;
        }
    }
    else
    {
        Status = vboxVdmaGgDmaBltPerform(pDevExt, &pBlt->SrcAlloc.pAlloc->AllocData, &pBlt->SrcRect,
                &pBlt->DstAlloc.pAlloc->AllocData, &pBlt->DstRects.ContextRect);
        AssertNtStatusSuccess(Status);
        if (Status != STATUS_SUCCESS)
            return Status;
    }

    return Status;
}

#ifdef VBOX_WITH_CROGL
typedef struct VBOXVDMA_CRRXGENERICSYNC
{
    int rc;
    KEVENT Event;
} VBOXVDMA_CRRXGENERICSYNC, *PVBOXVDMA_CRRXGENERICSYNC;

/** @callback_method_impl{PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEREADASYNC_COMPLETION}   */
static DECLCALLBACK(void) vboxVdmaCrRxGenericSyncCompletion(PVBOXMP_CRSHGSMITRANSPORT pCon, int rc,
                                                            void RT_UNTRUSTED_VOLATILE_HOST *pvRx, uint32_t cbRx,
                                                            void RT_UNTRUSTED_VOLATILE_HOST *pvCtx)
{
    VBOXMP_VDMACR_WRITEREADCOMPLETION RT_UNTRUSTED_VOLATILE_HOST *pCompletionData
        = (VBOXMP_VDMACR_WRITEREADCOMPLETION RT_UNTRUSTED_VOLATILE_HOST *)pvCtx;
    PVBOXVDMA_CRRXGENERICSYNC pData = (PVBOXVDMA_CRRXGENERICSYNC)pCompletionData->pvContext;
    if (RT_SUCCESS(rc))
    {
        rc = VBoxMpCrCmdRxHandler((CRMessageHeader *)pvRx, cbRx);
        if (!RT_SUCCESS(rc))
        {
            WARN(("VBoxMpCrCmdRxHandler failed %d", rc));
        }
    }
    else
    {
        WARN(("rx failure %d", rc));
    }

    pData->rc = rc;

    KeSetEvent(&pData->Event, 0, FALSE);

    vboxVdmaCrSubmitWriteReadAsyncGenericCompletion(pCon, pvCtx);
}

NTSTATUS vboxVdmaCrRxGenericSync(PVBOXMP_DEVEXT pDevExt, VBOXMP_CRPACKER *pCrPacker, uint32_t u32CrConClientID)
{
    VBOXVDMA_CRRXGENERICSYNC Data;
    Data.rc = VERR_NOT_SUPPORTED;
    KeInitializeEvent(&Data.Event, SynchronizationEvent, FALSE);
    NTSTATUS Status = vboxVdmaCrSubmitWriteReadAsync(pDevExt, pCrPacker, u32CrConClientID, vboxVdmaCrRxGenericSyncCompletion, &Data);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrSubmitWriteAsync failed Status 0x%x", Status));
        return Status;
    }

     Status = KeWaitForSingleObject(&Data.Event, Executive, KernelMode, FALSE, NULL /* PLARGE_INTEGER Timeout */);
     if (!NT_SUCCESS(Status))
     {
         WARN(("KeWaitForSingleObject failed Status 0x%x", Status));
         return Status;
     }

     return STATUS_SUCCESS;
}

typedef struct VBOXMP_CRHGSMIMGR
{
    VBOXMP_CRPACKER CrPacker;
    void RT_UNTRUSTED_VOLATILE_HOST *pvCommandBuffer;
} VBOXMP_CRHGSMIMGR;

DECLINLINE(CRPackContext*) vboxVdmaCrHmGetPackContext(VBOXMP_CRHGSMIMGR *pMgr)
{
    return &pMgr->CrPacker.CrPacker;
}

NTSTATUS vboxVdmaCrHmCreate(PVBOXMP_DEVEXT pDevExt, VBOXMP_CRHGSMIMGR *pMgr, uint32_t cbCommandBuffer, uint32_t cCommands)
{
    pMgr->pvCommandBuffer = VBoxMpCrShgsmiTransportBufAlloc(&pDevExt->CrHgsmiTransport, cbCommandBuffer);
    if (!pMgr->pvCommandBuffer)
    {
        WARN(("VBoxMpCrShgsmiTransportBufAlloc failed!"));
        return VERR_OUT_OF_RESOURCES;
    }

    VBoxMpCrPackerInit(&pMgr->CrPacker);

    VBoxMpCrPackerTxBufferInit(&pMgr->CrPacker, pMgr->pvCommandBuffer, cbCommandBuffer, cCommands);

    return STATUS_SUCCESS;
}

NTSTATUS vboxVdmaCrHmSubmitWrSync(PVBOXMP_DEVEXT pDevExt, VBOXMP_CRHGSMIMGR *pMgr, uint32_t u32CrConClientID)
{
    NTSTATUS Status = vboxVdmaCrRxGenericSync(pDevExt, &pMgr->CrPacker, u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrRxGenericSync failed Status 0x%x", Status));
        VBoxMpCrShgsmiTransportBufFree(&pDevExt->CrHgsmiTransport, pMgr->pvCommandBuffer);
        return Status;
    }

    return STATUS_SUCCESS;
}
#if 0
NTSTATUS vboxVdmaCrCmdGetChromiumParametervCR(PVBOXMP_DEVEXT pDevExt, uint32_t u32CrConClientID, GLenum target, GLuint index, GLenum type, GLsizei count, GLvoid * values)
{
    uint32_t cbCommandBuffer = VBOXMP_CRCMD_HEADER_SIZE + VBOXMP_CRCMD_SIZE_GETCHROMIUMPARAMETERVCR;
    uint32_t cCommands = 1;

    VBOXMP_CRHGSMIMGR Mgr;
    NTSTATUS Status = vboxVdmaCrHmCreate(pDevExt, &Mgr, cbCommandBuffer, cCommands);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrHmCreate failed Status 0x%x", Status));
        return Status;
    }

    int dummy = 1;

    crPackGetChromiumParametervCR(vboxVdmaCrHmGetPackContext(&Mgr), target, index, type, count, values, &dummy);

    Status = vboxVdmaCrHmSubmitWrSync(pDevExt, &Mgr, u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrHmSubmitWrSync failed Status 0x%x", Status));
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS vboxVdmaCrCmdCreateContext(PVBOXMP_DEVEXT pDevExt, int32_t visualBits, int32_t *pi32CtxID)
{
    uint32_t cbCommandBuffer = VBOXMP_CRCMD_HEADER_SIZE + VBOXMP_CRCMD_SIZE_CREATECONTEXT;
    uint32_t cCommands = 1;

    VBOXMP_CRHGSMIMGR Mgr;
    NTSTATUS Status = vboxVdmaCrHmCreate(pDevExt, &Mgr, cbCommandBuffer, cCommands);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrHmCreate failed Status 0x%x", Status));
        return Status;
    }

    int dummy = 1;

    crPackCreateContext(&CrPacker.CrPacker, "", visualBits, 0, &pi32CtxID, &dummy);

    Status = vboxVdmaCrHmSubmitWrSync(pDevExt, &Mgr, u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrHmSubmitWrSync failed Status 0x%x", Status));
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS vboxVdmaCrCmdWindowCreate(PVBOXMP_DEVEXT pDevExt, int32_t visualBits, int32_t *pi32WinID)
{
    uint32_t cbCommandBuffer = VBOXMP_CRCMD_HEADER_SIZE + VBOXMP_CRCMD_SIZE_WINDOWCREATE;
    uint32_t cCommands = 1;

    VBOXMP_CRHGSMIMGR Mgr;
    NTSTATUS Status = vboxVdmaCrHmCreate(pDevExt, &Mgr, cbCommandBuffer, cCommands);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrHmCreate failed Status 0x%x", Status));
        return Status;
    }

    int dummy = 1;

    crPackWindowCreate(&CrPacker.CrPacker, "", visualBits, 0, &pi32CtxID, &dummy);

    Status = vboxVdmaCrHmSubmitWrSync(pDevExt, &Mgr, u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrHmSubmitWrSync failed Status 0x%x", Status));
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS vboxVdmaCrCtlGetDefaultCtxId(PVBOXMP_DEVEXT pDevExt, int32_t *pi32CtxID)
{
    if (!pDevExt->i32CrConDefaultCtxID)
    {
        if (!pDevExt->f3DEnabled)
        {
            WARN(("3D disabled, should not be here!"));
            return STATUS_UNSUCCESSFUL;
        }

        uint32_t u32ClienID;
        NTSTATUS Status = vboxVdmaCrCtlGetDefaultClientId(pDevExt, &u32ClienID);
        if (!NT_SUCCESS(Status))
        {
            WARN(("vboxVdmaCrCtlGetDefaultClientId failed, Status %#x", Status));
            return Status;
        }

        Status = vboxVdmaCrCmdWindowCreate(PVBOXMP_DEVEXT pDevExt, int32_t visualBits, int32_t *pi32WinID)

        VBOXMP_CRPACKER CrPacker;
        VBoxMpCrPackerInit(&CrPacker);

        int rc = VBoxMpCrCtlConConnect(pDevExt, &pDevExt->CrCtlCon, CR_PROTOCOL_VERSION_MAJOR, CR_PROTOCOL_VERSION_MINOR, &pDevExt->u32CrConDefaultClientID);
        if (!RT_SUCCESS(rc))
        {
            WARN(("VBoxMpCrCtlConConnect failed, rc %d", rc));
            return STATUS_UNSUCCESSFUL;
        }
    }

    *pi32CtxID = pDevExt->i32CrConDefaultCtxID;
    return STATUS_SUCCESS;
}
#endif

static NTSTATUS vboxVdmaTexPresentSubmit(PVBOXMP_DEVEXT pDevExt,
        VBOXMP_CRPACKER *pCrPacker,
        uint32_t u32CrConClientID,
        uint32_t hostID,
        uint32_t cfg,
        int32_t posX,
        int32_t posY,
        uint32_t cRects,
        const RTRECT*paRects)
{
    Assert(pDevExt->fTexPresentEnabled);

    uint32_t cbCommandBuffer = VBOXMP_CRCMD_HEADER_SIZE + VBOXMP_CRCMD_SIZE_VBOXTEXPRESENT(cRects);
    uint32_t cCommands = 1;
    void RT_UNTRUSTED_VOLATILE_HOST *pvCommandBuffer = VBoxMpCrShgsmiTransportBufAlloc(&pDevExt->CrHgsmiTransport, cbCommandBuffer);
    if (!pvCommandBuffer)
    {
        WARN(("VBoxMpCrShgsmiTransportBufAlloc failed!"));
        return VERR_OUT_OF_RESOURCES;
    }

    VBoxMpCrPackerTxBufferInit(pCrPacker, pvCommandBuffer, cbCommandBuffer, cCommands);

    crPackVBoxTexPresent(&pCrPacker->CrPacker, hostID, cfg, posX, posY, cRects, (int32_t*)paRects);

    NTSTATUS Status = vboxVdmaCrSubmitWriteAsync(pDevExt, pCrPacker, u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrSubmitWriteAsync failed Status 0x%x", Status));
        VBoxMpCrShgsmiTransportBufFree(&pDevExt->CrHgsmiTransport, pvCommandBuffer);
    }

    return Status;
}

static NTSTATUS vboxVdmaChromiumParameteriCRSubmit(PVBOXMP_DEVEXT pDevExt,
        VBOXMP_CRPACKER *pCrPacker,
        uint32_t u32CrConClientID,
        uint32_t target, uint32_t value)
{
    Assert(pDevExt->fTexPresentEnabled);

    uint32_t cbCommandBuffer = VBOXMP_CRCMD_HEADER_SIZE + VBOXMP_CRCMD_SIZE_CHROMIUMPARAMETERICR;
    uint32_t cCommands = 1;
    void RT_UNTRUSTED_VOLATILE_HOST *pvCommandBuffer = VBoxMpCrShgsmiTransportBufAlloc(&pDevExt->CrHgsmiTransport, cbCommandBuffer);
    if (!pvCommandBuffer)
    {
        WARN(("VBoxMpCrShgsmiTransportBufAlloc failed!"));
        return VERR_OUT_OF_RESOURCES;
    }

    VBoxMpCrPackerTxBufferInit(pCrPacker, pvCommandBuffer, cbCommandBuffer, cCommands);

    crPackChromiumParameteriCR(&pCrPacker->CrPacker, target, value);

    NTSTATUS Status = vboxVdmaCrSubmitWriteAsync(pDevExt, pCrPacker, u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrSubmitWriteAsync failed Status 0x%x", Status));
        VBoxMpCrShgsmiTransportBufFree(&pDevExt->CrHgsmiTransport, pvCommandBuffer);
    }

    return Status;
}

NTSTATUS VBoxVdmaChromiumParameteriCRSubmit(PVBOXMP_DEVEXT pDevExt, uint32_t target, uint32_t value)
{
    uint32_t u32CrConClientID;
    NTSTATUS Status = vboxVdmaCrCtlGetDefaultClientId(pDevExt, &u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrCtlGetDefaultClientId failed Status 0x%x", Status));
        return Status;
    }

    VBOXMP_CRPACKER *pCrPacker = (VBOXMP_CRPACKER *)RTMemTmpAlloc(sizeof(*pCrPacker));
    if (!pCrPacker)
        return STATUS_NO_MEMORY;
    VBoxMpCrPackerInit(pCrPacker);

    Status = vboxVdmaChromiumParameteriCRSubmit(pDevExt, pCrPacker, u32CrConClientID, target, value);
    if (!NT_SUCCESS(Status))
        WARN(("vboxVdmaChromiumParameteriCRSubmit failed Status 0x%x", Status));

    RTMemTmpFree(pCrPacker);
    return Status;
}

static NTSTATUS vboxVdmaCrCtlGetDefaultClientId(PVBOXMP_DEVEXT pDevExt, uint32_t *pu32ClienID)
{
    if (!pDevExt->u32CrConDefaultClientID)
    {
        if (!pDevExt->f3DEnabled)
        {
            WARN(("3D disabled, should not be here!"));
            return STATUS_UNSUCCESSFUL;
        }

        int rc = VBoxMpCrCtlConConnect(pDevExt, &pDevExt->CrCtlCon, CR_PROTOCOL_VERSION_MAJOR, CR_PROTOCOL_VERSION_MINOR, &pDevExt->u32CrConDefaultClientID);
        if (!RT_SUCCESS(rc))
        {
            WARN(("VBoxMpCrCtlConConnect failed, rc %d", rc));
            return STATUS_UNSUCCESSFUL;
        }
    }

    *pu32ClienID = pDevExt->u32CrConDefaultClientID;
    return STATUS_SUCCESS;
}


static NTSTATUS vboxVdmaProcessVRegTexPresent(PVBOXMP_DEVEXT pDevExt,
        VBOXMP_CRPACKER *pCrPacker,
        uint32_t u32CrConClientID,
        const VBOXWDDM_ALLOC_DATA *pSrcAllocData,
        const VBOXWDDM_ALLOC_DATA *pDstAllocData,
        const RECT *pSrcRect, const VBOXVDMAPIPE_RECTS *pDstRects)
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId = pDstAllocData->SurfDesc.VidPnSourceId;
    if (srcId >= (D3DDDI_VIDEO_PRESENT_SOURCE_ID)VBoxCommonFromDeviceExt(pDevExt)->cDisplays)
    {
        WARN(("invalid srcId"));
        return STATUS_NOT_SUPPORTED;
    }

    VBOXWDDM_SOURCE *pSource = &pDevExt->aSources[srcId];

    bool fPrimary = vboxWddmAddrVramEqual(&pDstAllocData->Addr, &pSource->AllocData.Addr);
    /* we care only about screen regions */
    if (!fPrimary)
    {
        WARN(("non-primary allocation passed to vboxWddmSubmitBltCmd!"));
        return STATUS_NOT_SUPPORTED;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    uint32_t hostID = pSrcAllocData->hostID;
    int rc;
    if (hostID)
    {
//            Assert(pContext->enmType == VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D);
        int32_t posX = pDstRects->ContextRect.left - pSrcRect->left;
        int32_t posY = pDstRects->ContextRect.top - pSrcRect->top;

        Status = vboxVdmaTexPresentSubmit(pDevExt, pCrPacker, u32CrConClientID, hostID, srcId, posX, posY, pDstRects->UpdateRects.cRects, (const RTRECT*)pDstRects->UpdateRects.aRects);
        if (NT_SUCCESS(Status))
        {
            rc = VBoxVrListRectsSubst(&pSource->VrList, pDstRects->UpdateRects.cRects, (const RTRECT*)pDstRects->UpdateRects.aRects, NULL);
            if (RT_SUCCESS(rc))
                pSource->fHas3DVrs = TRUE;
            else
                WARN(("VBoxVrListRectsSubst failed rc %d, ignoring..", rc));
        }
        else
            WARN(("vboxVdmaTexPresentSubmit failed Status 0x%x", Status));
    }
    else
    {
        Assert(fPrimary);

        bool fChanged = false;
        Assert(pSource->pPrimaryAllocation->bVisible);
        rc = VBoxVrListRectsAdd(&pSource->VrList, pDstRects->UpdateRects.cRects, (const RTRECT*)pDstRects->UpdateRects.aRects, &fChanged);
        if (RT_SUCCESS(rc))
        {
            if (fChanged)
            {
                Status = vboxVdmaTexPresentSubmit(pDevExt, pCrPacker, u32CrConClientID, hostID, srcId, 0, 0, pDstRects->UpdateRects.cRects, (const RTRECT*)pDstRects->UpdateRects.aRects);
                if (NT_SUCCESS(Status))
                {
                    if (pSource->fHas3DVrs)
                    {
                        if (VBoxVrListRectsCount(&pSource->VrList) == 1)
                        {
                            RTRECT Rect;
                            VBoxVrListRectsGet(&pSource->VrList, 1, &Rect);
                            if (   Rect.xLeft   == 0
                                && Rect.yTop    == 0
                                && Rect.xRight  == (int32_t)pDstAllocData->SurfDesc.width
                                && Rect.yBottom == (int32_t)pDstAllocData->SurfDesc.height)
                            {
                                pSource->fHas3DVrs = FALSE;
                            }
                        }
                    }
                }
                else
                    WARN(("vboxVdmaTexPresentSubmit failed Status 0x%x", Status));
            }
        }
        else
            WARN(("VBoxVrListRectsAdd failed rc %d, ignoring..", rc));
    }

    return Status;
}

static NTSTATUS vboxVdmaProcessVReg(PVBOXMP_DEVEXT pDevExt,
        VBOXMP_CRPACKER *pCrPacker,
        uint32_t u32CrConClientID,
        const VBOXWDDM_ALLOCATION *pSrcAlloc,
        const VBOXWDDM_ALLOCATION *pDstAlloc,
        const RECT *pSrcRect, const VBOXVDMAPIPE_RECTS *pDstRects)
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId = pDstAlloc->AllocData.SurfDesc.VidPnSourceId;
    VBOXWDDM_SOURCE *pSource = &pDevExt->aSources[srcId];
    NTSTATUS Status = STATUS_SUCCESS;

    if (pDevExt->fTexPresentEnabled)
    {
        Status = vboxVdmaProcessVRegTexPresent(pDevExt, pCrPacker, u32CrConClientID,
                &pSrcAlloc->AllocData, &pDstAlloc->AllocData,
                pSrcRect, pDstRects);
    }
    else
    {
        PVBOXWDDM_SWAPCHAIN pSwapchain = vboxWddmSwapchainRetainByAlloc(pDevExt, pSrcAlloc);

        if (pSwapchain)
        {
            Assert(pSrcAlloc->AllocData.SurfDesc.width == pSwapchain->width);
            Assert(pSrcAlloc->AllocData.SurfDesc.height == pSwapchain->height);
        }

        Status = vboxVdmaProcessVRegCmdLegacy(pDevExt, pCrPacker, u32CrConClientID, pSource, pSwapchain, pSrcRect, pDstRects);
        if (!NT_SUCCESS(Status))
            WARN(("vboxVdmaProcessVRegCmdLegacy failed Status 0x%x", Status));

        if (pSwapchain)
            vboxWddmSwapchainRelease(pSwapchain);
    }

    return Status;
}

NTSTATUS vboxVdmaTexPresentSetAlloc(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData)
{
    uint32_t u32CrConClientID;
    NTSTATUS Status = vboxVdmaCrCtlGetDefaultClientId(pDevExt, &u32CrConClientID);
    if (!NT_SUCCESS(Status))
    {
        WARN(("vboxVdmaCrCtlGetDefaultClientId failed Status 0x%x", Status));
        return Status;
    }

    VBOXMP_CRPACKER *pCrPacker = (VBOXMP_CRPACKER *)RTMemTmpAlloc(sizeof(*pCrPacker));
    if (!pCrPacker)
        return STATUS_NO_MEMORY;
    VBoxMpCrPackerInit(pCrPacker);

    RECT Rect;
    Rect.left = 0;
    Rect.top = 0;
    Rect.right = pAllocData->SurfDesc.width;
    Rect.bottom = pAllocData->SurfDesc.height;

    if (pDevExt->fCmdVbvaEnabled)
        Status = vboxVdmaTexPresentSubmit(pDevExt, pCrPacker, u32CrConClientID, pAllocData->hostID, pAllocData->SurfDesc.VidPnSourceId, 0, 0, 1, (RTRECT*)&Rect);
    else if (pDevExt->fTexPresentEnabled)
    {
        VBOXVDMAPIPE_RECTS RectInfo;
        RectInfo.ContextRect = Rect;
        RectInfo.UpdateRects.cRects = 1;
        RectInfo.UpdateRects.aRects[0] = Rect;

        Status = vboxVdmaProcessVRegTexPresent(pDevExt, pCrPacker, u32CrConClientID,
                pAllocData, pAllocData,
                &Rect, &RectInfo);
    }
    else
        Status = STATUS_NOT_IMPLEMENTED;

    RTMemTmpFree(pCrPacker);
    return Status;
}

static NTSTATUS vboxVdmaProcessVRegCmd(PVBOXMP_DEVEXT pDevExt, VBOXWDDM_CONTEXT *pContext,
        const VBOXWDDM_DMA_ALLOCINFO *pSrcAllocInfo,
        const VBOXWDDM_DMA_ALLOCINFO *pDstAllocInfo,
        const RECT *pSrcRect, const VBOXVDMAPIPE_RECTS *pDstRects)
{
    return vboxVdmaProcessVReg(pDevExt, &pContext->CrPacker, pContext->u32CrConClientID,
            pSrcAllocInfo->pAlloc, pDstAllocInfo->pAlloc,
            pSrcRect, pDstRects);
}

static void vboxVdmaBltDirtyRectsUpdate(PVBOXMP_DEVEXT pDevExt, VBOXWDDM_SOURCE *pSource, uint32_t cRects, const RECT *paRects)
{
    if (!cRects)
    {
        WARN(("vboxVdmaBltDirtyRectsUpdate: no rects specified"));
        return;
    }

    RECT rect;
    rect = paRects[0];
    for (UINT i = 1; i < cRects; ++i)
    {
        vboxWddmRectUnited(&rect, &rect, &paRects[i]);
    }

    uint32_t cUnlockedVBVADisabled = ASMAtomicReadU32(&pDevExt->cUnlockedVBVADisabled);
    if (!cUnlockedVBVADisabled)
    {
        VBOXVBVA_OP(ReportDirtyRect, pDevExt, pSource, &rect);
    }
    else
    {
        VBOXVBVA_OP_WITHLOCK_ATDPC(ReportDirtyRect, pDevExt, pSource, &rect);
    }
}
#endif

NTSTATUS vboxVdmaProcessBltCmd(PVBOXMP_DEVEXT pDevExt, VBOXWDDM_CONTEXT *pContext, VBOXWDDM_DMA_PRIVATEDATA_BLT *pBlt)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_ALLOCATION pDstAlloc = pBlt->Blt.DstAlloc.pAlloc;
    PVBOXWDDM_ALLOCATION pSrcAlloc = pBlt->Blt.SrcAlloc.pAlloc;
    BOOLEAN fVRAMUpdated = FALSE;
#ifdef VBOX_WITH_CROGL
    if (!pDstAlloc->AllocData.hostID && !pSrcAlloc->AllocData.hostID)
#endif
    {
        /* the allocations contain a real data in VRAM, do blitting */
        vboxVdmaGgDmaBlt(pDevExt, &pBlt->Blt);
        fVRAMUpdated = TRUE;
    }

#ifdef VBOX_WITH_CROGL
    if (VBOXWDDM_IS_FB_ALLOCATION(pDevExt, pDstAlloc)
            && pDstAlloc->bVisible)
    {
        VBOXWDDM_SOURCE *pSource = &pDevExt->aSources[pDstAlloc->AllocData.SurfDesc.VidPnSourceId];
        Assert(pDstAlloc->AllocData.SurfDesc.VidPnSourceId < VBOX_VIDEO_MAX_SCREENS);
        Assert(pSource->pPrimaryAllocation == pDstAlloc);

        if (fVRAMUpdated)
            vboxVdmaBltDirtyRectsUpdate(pDevExt, pSource, pBlt->Blt.DstRects.UpdateRects.cRects, pBlt->Blt.DstRects.UpdateRects.aRects);

        if (pSrcAlloc->AllocData.hostID || (pDevExt->fTexPresentEnabled ? pSource->fHas3DVrs : !!pDevExt->cContexts3D))
        {
            Status = vboxVdmaProcessVRegCmd(pDevExt, pContext, &pBlt->Blt.SrcAlloc, &pBlt->Blt.DstAlloc, &pBlt->Blt.SrcRect, &pBlt->Blt.DstRects);
            if (!NT_SUCCESS(Status))
                WARN(("vboxVdmaProcessVRegCmd failed Status 0x%x", Status));
        }
    }
#endif
    return Status;
}

NTSTATUS vboxVdmaProcessFlipCmd(PVBOXMP_DEVEXT pDevExt, VBOXWDDM_CONTEXT *pContext, VBOXWDDM_DMA_PRIVATEDATA_FLIP *pFlip)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_ALLOCATION pAlloc = pFlip->Flip.Alloc.pAlloc;
    VBOXWDDM_SOURCE *pSource = &pDevExt->aSources[pAlloc->AllocData.SurfDesc.VidPnSourceId];
    vboxWddmAssignPrimary(pSource, pAlloc, pAlloc->AllocData.SurfDesc.VidPnSourceId);
#ifdef VBOX_WITH_CROGL
    if (pAlloc->AllocData.hostID)
    {
        RECT SrcRect;
        VBOXVDMAPIPE_RECTS Rects;
        SrcRect.left = 0;
        SrcRect.top = 0;
        SrcRect.right = pAlloc->AllocData.SurfDesc.width;
        SrcRect.bottom = pAlloc->AllocData.SurfDesc.height;
        Rects.ContextRect.left = 0;
        Rects.ContextRect.top = 0;
        Rects.ContextRect.right = pAlloc->AllocData.SurfDesc.width;
        Rects.ContextRect.bottom = pAlloc->AllocData.SurfDesc.height;
        Rects.UpdateRects.cRects = 1;
        Rects.UpdateRects.aRects[0] = Rects.ContextRect;

        Status = vboxVdmaProcessVRegCmd(pDevExt, pContext, &pFlip->Flip.Alloc, &pFlip->Flip.Alloc, &SrcRect, &Rects);
        if (!NT_SUCCESS(Status))
            WARN(("vboxVdmaProcessVRegCmd failed Status 0x%x", Status));
    }
    else
#endif
    {
        WARN(("unexpected flip request"));
    }

    return Status;
}

NTSTATUS vboxVdmaProcessClrFillCmd(PVBOXMP_DEVEXT pDevExt, VBOXWDDM_CONTEXT *pContext, VBOXWDDM_DMA_PRIVATEDATA_CLRFILL *pCF)
{
    RT_NOREF(pContext);
    NTSTATUS Status = STATUS_SUCCESS;
    PVBOXWDDM_ALLOCATION pAlloc = pCF->ClrFill.Alloc.pAlloc;
#ifdef VBOX_WITH_CROGL
    if (!pAlloc->AllocData.hostID)
#endif
    {
        Status = vboxVdmaGgDmaColorFill(pDevExt, &pCF->ClrFill);
        if (!NT_SUCCESS(Status))
            WARN(("vboxVdmaGgDmaColorFill failed Status 0x%x", Status));
    }
#ifdef VBOX_WITH_CROGL
    else
        WARN(("unexpected clrfill request"));
#endif

    return Status;
}


#ifdef VBOX_WITH_VDMA
/*
 * This is currently used by VDMA. It is invisible for Vdma API clients since
 * Vdma transport may change if we choose to use another (e.g. more light-weight)
 * transport for DMA commands submission
 */

#ifdef VBOXVDMA_WITH_VBVA
static int vboxWddmVdmaSubmitVbva(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo, HGSMIOFFSET offDr)
{
    int rc;
    if (vboxVbvaBufferBeginUpdate (pDevExt, &pDevExt->u.primary.Vbva))
    {
        rc = vboxVbvaReportCmdOffset(pDevExt, &pDevExt->u.primary.Vbva, offDr);
        vboxVbvaBufferEndUpdate (pDevExt, &pDevExt->u.primary.Vbva);
    }
    else
    {
        AssertBreakpoint();
        rc = VERR_INVALID_STATE;
    }
    return rc;
}
# define vboxWddmVdmaSubmit vboxWddmVdmaSubmitVbva
#else
static int vboxWddmVdmaSubmitHgsmi(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo, HGSMIOFFSET offDr)
{
    RT_NOREF(pInfo);
    VBVO_PORT_WRITE_U32(VBoxCommonFromDeviceExt(pDevExt)->guestCtx.port, offDr);
    /* Make the compiler aware that the host has changed memory. */
    ASMCompilerBarrier();
    return VINF_SUCCESS;
}
# define vboxWddmVdmaSubmit vboxWddmVdmaSubmitHgsmi
#endif

static int vboxVdmaInformHost(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo, VBOXVDMA_CTL_TYPE enmCtl)
{
    int rc = VINF_SUCCESS;

    VBOXVDMA_CTL RT_UNTRUSTED_VOLATILE_HOST *pCmd =
        (VBOXVDMA_CTL RT_UNTRUSTED_VOLATILE_HOST *)VBoxSHGSMICommandAlloc(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx,
                                                                           sizeof(VBOXVDMA_CTL), HGSMI_CH_VBVA, VBVA_VDMA_CTL);
    if (pCmd)
    {
        pCmd->enmCtl = enmCtl;
        pCmd->u32Offset = pInfo->CmdHeap.Heap.area.offBase;
        pCmd->i32Result = VERR_NOT_SUPPORTED;

        const VBOXSHGSMIHEADER RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBoxSHGSMICommandPrepSynch(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx,
                                                                                             pCmd);
        Assert(pHdr);
        if (pHdr)
        {
            do
            {
                HGSMIOFFSET offCmd = VBoxSHGSMICommandOffset(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, pHdr);
                Assert(offCmd != HGSMIOFFSET_VOID);
                if (offCmd != HGSMIOFFSET_VOID)
                {
                    rc = vboxWddmVdmaSubmit(pDevExt, pInfo, offCmd);
                    AssertRC(rc);
                    if (RT_SUCCESS(rc))
                    {
                        rc = VBoxSHGSMICommandDoneSynch(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, pHdr);
                        AssertRC(rc);
                        if (RT_SUCCESS(rc))
                        {
                            rc = pCmd->i32Result;
                            AssertRC(rc);
                        }
                        break;
                    }
                }
                else
                    rc = VERR_INVALID_PARAMETER;
                /* fail to submit, cancel it */
                VBoxSHGSMICommandCancelSynch(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, pHdr);
            } while (0);
        }

        VBoxSHGSMICommandFree(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, pCmd);
    }
    else
    {
        LOGREL(("HGSMIHeapAlloc failed"));
        rc = VERR_OUT_OF_RESOURCES;
    }

    return rc;
}
#endif

static DECLCALLBACK(void *) hgsmiEnvAlloc(void *pvEnv, HGSMISIZE cb)
{
    NOREF(pvEnv);
    return RTMemAlloc(cb);
}

static DECLCALLBACK(void) hgsmiEnvFree(void *pvEnv, void *pv)
{
    NOREF(pvEnv);
    RTMemFree(pv);
}

static HGSMIENV g_hgsmiEnvVdma =
{
    NULL,
    hgsmiEnvAlloc,
    hgsmiEnvFree
};

/* create a DMACommand buffer */
int vboxVdmaCreate(PVBOXMP_DEVEXT pDevExt, VBOXVDMAINFO *pInfo
#ifdef VBOX_WITH_VDMA
        , ULONG offBuffer, ULONG cbBuffer
#endif
        )
{
    pInfo->fEnabled           = FALSE;

#ifdef VBOX_WITH_VDMA
    int rc;
    Assert((offBuffer & 0xfff) == 0);
    Assert((cbBuffer & 0xfff) == 0);
    Assert(offBuffer);
    Assert(cbBuffer);

    if((offBuffer & 0xfff)
            || (cbBuffer & 0xfff)
            || !offBuffer
            || !cbBuffer)
    {
        LOGREL(("invalid parameters: offBuffer(0x%x), cbBuffer(0x%x)", offBuffer, cbBuffer));
        return VERR_INVALID_PARAMETER;
    }
    PVOID pvBuffer;

    rc = VBoxMPCmnMapAdapterMemory(VBoxCommonFromDeviceExt(pDevExt),
                                   &pvBuffer,
                                   offBuffer,
                                   cbBuffer);
    Assert(RT_SUCCESS(rc));
    if (RT_SUCCESS(rc))
    {
        /* Setup a HGSMI heap within the adapter information area. */
        rc = VBoxSHGSMIInit(&pInfo->CmdHeap,
                             pvBuffer,
                             cbBuffer,
                             offBuffer,
                             &g_hgsmiEnvVdma);
        Assert(RT_SUCCESS(rc));
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;
        else
            LOGREL(("HGSMIHeapSetup failed rc = 0x%x", rc));

        VBoxMPCmnUnmapAdapterMemory(VBoxCommonFromDeviceExt(pDevExt), &pvBuffer);
    }
    else
        LOGREL(("VBoxMapAdapterMemory failed rc = 0x%x\n", rc));
    return rc;
#else
    return VINF_SUCCESS;
#endif
}

int vboxVdmaDisable (PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo)
{
    Assert(pInfo->fEnabled);
    if (!pInfo->fEnabled)
        return VINF_ALREADY_INITIALIZED;

    /* ensure nothing else is submitted */
    pInfo->fEnabled        = FALSE;
#ifdef VBOX_WITH_VDMA
    int rc = vboxVdmaInformHost (pDevExt, pInfo, VBOXVDMA_CTL_TYPE_DISABLE);
    AssertRC(rc);
    return rc;
#else
    return VINF_SUCCESS;
#endif
}

int vboxVdmaEnable (PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo)
{
    Assert(!pInfo->fEnabled);
    if (pInfo->fEnabled)
        return VINF_ALREADY_INITIALIZED;
#ifdef VBOX_WITH_VDMA
    int rc = vboxVdmaInformHost (pDevExt, pInfo, VBOXVDMA_CTL_TYPE_ENABLE);
    Assert(RT_SUCCESS(rc));
    if (RT_SUCCESS(rc))
        pInfo->fEnabled        = TRUE;

    return rc;
#else
    return VINF_SUCCESS;
#endif
}

#ifdef VBOX_WITH_VDMA
int vboxVdmaFlush (PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo)
{
    Assert(pInfo->fEnabled);
    if (!pInfo->fEnabled)
        return VINF_ALREADY_INITIALIZED;

    int rc = vboxVdmaInformHost (pDevExt, pInfo, VBOXVDMA_CTL_TYPE_FLUSH);
    Assert(RT_SUCCESS(rc));

    return rc;
}
#endif

int vboxVdmaDestroy (PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo)
{
    int rc = VINF_SUCCESS;
    Assert(!pInfo->fEnabled);
    if (pInfo->fEnabled)
        rc = vboxVdmaDisable (pDevExt, pInfo);
#ifdef VBOX_WITH_VDMA
    VBoxSHGSMITerm(&pInfo->CmdHeap);
    VBoxMPCmnUnmapAdapterMemory(VBoxCommonFromDeviceExt(pDevExt), (void**)&pInfo->CmdHeap.Heap.area.pu8Base);
#endif
    return rc;
}

#ifdef VBOX_WITH_VDMA
void vboxVdmaCBufDrFree(PVBOXVDMAINFO pInfo, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr)
{
    VBoxSHGSMICommandFree(&pInfo->CmdHeap, pDr);
}

VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *vboxVdmaCBufDrCreate(PVBOXVDMAINFO pInfo, uint32_t cbTrailingData)
{
    uint32_t cbDr = VBOXVDMACBUF_DR_SIZE(cbTrailingData);
    VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr =
        (VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *)VBoxSHGSMICommandAlloc(&pInfo->CmdHeap, cbDr, HGSMI_CH_VBVA, VBVA_VDMA_CMD);
    Assert(pDr);
    if (pDr)
        memset((void *)pDr, 0, cbDr);
    else
        LOGREL(("VBoxSHGSMICommandAlloc returned NULL"));

    return pDr;
}

static DECLCALLBACK(void) vboxVdmaCBufDrCompletion(PVBOXSHGSMI pHeap, void *pvCmd, void *pvContext)
{
    RT_NOREF(pHeap);
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)pvContext;
    PVBOXVDMAINFO pInfo = &pDevExt->u.primary.Vdma;

    vboxVdmaCBufDrFree(pInfo, (PVBOXVDMACBUF_DR)pvCmd);
}

/** @callback_method_impl{FNVBOXSHGSMICMDCOMPLETION_IRQ} */
static DECLCALLBACK(PFNVBOXSHGSMICMDCOMPLETION)
vboxVdmaCBufDrCompletionIrq(PVBOXSHGSMI pHeap, void RT_UNTRUSTED_VOLATILE_HOST *pvCmd, void *pvContext, void **ppvCompletion)
{
    RT_NOREF(pHeap, ppvCompletion);
    PVBOXMP_DEVEXT pDevExt = (PVBOXMP_DEVEXT)pvContext;
    VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr = (VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *)pvCmd;

    DXGK_INTERRUPT_TYPE enmComplType;

    if (RT_SUCCESS(pDr->rc))
    {
        enmComplType = DXGK_INTERRUPT_DMA_COMPLETED;
    }
    else if (pDr->rc == VERR_INTERRUPTED)
    {
        AssertFailed();
        enmComplType = DXGK_INTERRUPT_DMA_PREEMPTED;
    }
    else
    {
        AssertFailed();
        enmComplType = DXGK_INTERRUPT_DMA_FAULTED;
    }

    if (vboxVdmaDdiCmdCompletedIrq(pDevExt, VBOXVDMADDI_CMD_FROM_BUF_DR(pDr), enmComplType))
    {
        pDevExt->bNotifyDxDpc = TRUE;
    }

    /* inform SHGSMI we DO NOT want to be called at DPC later */
    return NULL;
//    *ppvCompletion = pvContext;
}

int vboxVdmaCBufDrSubmit(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr)
{
    const VBOXSHGSMIHEADER RT_UNTRUSTED_VOLATILE_HOST *pHdr =
        VBoxSHGSMICommandPrepAsynchIrq(&pInfo->CmdHeap, pDr, vboxVdmaCBufDrCompletionIrq, pDevExt, VBOXSHGSMI_FLAG_GH_ASYNCH_FORCE);
    Assert(pHdr);
    int rc = VERR_GENERAL_FAILURE;
    if (pHdr)
    {
        do
        {
            HGSMIOFFSET offCmd = VBoxSHGSMICommandOffset(&pInfo->CmdHeap, pHdr);
            Assert(offCmd != HGSMIOFFSET_VOID);
            if (offCmd != HGSMIOFFSET_VOID)
            {
                rc = vboxWddmVdmaSubmit(pDevExt, pInfo, offCmd);
                AssertRC(rc);
                if (RT_SUCCESS(rc))
                {
                    VBoxSHGSMICommandDoneAsynch(&pInfo->CmdHeap, pHdr);
                    AssertRC(rc);
                    break;
                }
            }
            else
                rc = VERR_INVALID_PARAMETER;
            /* fail to submit, cancel it */
            VBoxSHGSMICommandCancelAsynch(&pInfo->CmdHeap, pHdr);
        } while (0);
    }
    else
        rc = VERR_INVALID_PARAMETER;
    return rc;
}

int vboxVdmaCBufDrSubmitSynch(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr)
{
    const VBOXSHGSMIHEADER RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBoxSHGSMICommandPrepAsynch(&pInfo->CmdHeap, pDr, NULL,
                                                                                          NULL, VBOXSHGSMI_FLAG_GH_SYNCH);
    Assert(pHdr);
    int rc = VERR_GENERAL_FAILURE;
    if (pHdr)
    {
        do
        {
            HGSMIOFFSET offCmd = VBoxSHGSMICommandOffset(&pInfo->CmdHeap, pHdr);
            Assert(offCmd != HGSMIOFFSET_VOID);
            if (offCmd != HGSMIOFFSET_VOID)
            {
                rc = vboxWddmVdmaSubmit(pDevExt, pInfo, offCmd);
                AssertRC(rc);
                if (RT_SUCCESS(rc))
                {
                    VBoxSHGSMICommandDoneAsynch(&pInfo->CmdHeap, pHdr);
                    AssertRC(rc);
                    break;
                }
            }
            else
                rc = VERR_INVALID_PARAMETER;
            /* fail to submit, cancel it */
            VBoxSHGSMICommandCancelAsynch(&pInfo->CmdHeap, pHdr);
        } while (0);
    }
    else
        rc = VERR_INVALID_PARAMETER;
    return rc;
}
#endif


/* ddi dma command queue */

VOID vboxVdmaDdiCmdGetCompletedListIsr(PVBOXMP_DEVEXT pDevExt, LIST_ENTRY *pList)
{
    vboxVideoLeDetach(&pDevExt->DpcCmdQueue, pList);
}

BOOLEAN vboxVdmaDdiCmdIsCompletedListEmptyIsr(PVBOXMP_DEVEXT pDevExt)
{
    return IsListEmpty(&pDevExt->DpcCmdQueue);
}

DECLINLINE(BOOLEAN) vboxVdmaDdiCmdCanComplete(PVBOXMP_DEVEXT pDevExt, UINT u32NodeOrdinal)
{
    PVBOXVDMADDI_CMD_QUEUE pQueue = &pDevExt->aNodes[u32NodeOrdinal].CmdQueue;
    return ASMAtomicUoReadU32(&pQueue->cQueuedCmds) == 0;
}

DECLCALLBACK(VOID) vboxVdmaDdiCmdCompletionCbFree(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, PVOID pvContext)
{
    RT_NOREF(pDevExt, pvContext);
    vboxWddmMemFree(pCmd);
}

static VOID vboxVdmaDdiCmdNotifyCompletedIrq(PVBOXMP_DEVEXT pDevExt, UINT u32NodeOrdinal, UINT u32FenceId, DXGK_INTERRUPT_TYPE enmComplType)
{
    PVBOXVDMADDI_NODE pNode = &pDevExt->aNodes[u32NodeOrdinal];
    DXGKARGCB_NOTIFY_INTERRUPT_DATA notify;
    memset(&notify, 0, sizeof(DXGKARGCB_NOTIFY_INTERRUPT_DATA));
    switch (enmComplType)
    {
        case DXGK_INTERRUPT_DMA_COMPLETED:
            notify.InterruptType = DXGK_INTERRUPT_DMA_COMPLETED;
            notify.DmaCompleted.SubmissionFenceId = u32FenceId;
            notify.DmaCompleted.NodeOrdinal = u32NodeOrdinal;
            pNode->uLastCompletedFenceId = u32FenceId;
            break;

        case DXGK_INTERRUPT_DMA_PREEMPTED:
            Assert(0);
            notify.InterruptType = DXGK_INTERRUPT_DMA_PREEMPTED;
            notify.DmaPreempted.PreemptionFenceId = u32FenceId;
            notify.DmaPreempted.NodeOrdinal = u32NodeOrdinal;
            notify.DmaPreempted.LastCompletedFenceId = pNode->uLastCompletedFenceId;
            break;

        case DXGK_INTERRUPT_DMA_FAULTED:
            Assert(0);
            notify.InterruptType = DXGK_INTERRUPT_DMA_FAULTED;
            notify.DmaFaulted.FaultedFenceId = u32FenceId;
            notify.DmaFaulted.Status = STATUS_UNSUCCESSFUL; /** @todo better status ? */
            notify.DmaFaulted.NodeOrdinal = u32NodeOrdinal;
            break;

        default:
            Assert(0);
            break;
    }

    pDevExt->u.primary.DxgkInterface.DxgkCbNotifyInterrupt(pDevExt->u.primary.DxgkInterface.DeviceHandle, &notify);
}

static VOID vboxVdmaDdiCmdProcessCompletedIrq(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, DXGK_INTERRUPT_TYPE enmComplType)
{
    vboxVdmaDdiCmdNotifyCompletedIrq(pDevExt, pCmd->u32NodeOrdinal, pCmd->u32FenceId, enmComplType);
    switch (enmComplType)
    {
        case DXGK_INTERRUPT_DMA_COMPLETED:
            InsertTailList(&pDevExt->DpcCmdQueue, &pCmd->QueueEntry);
            break;
        default:
            AssertFailed();
            break;
    }
}

DECLINLINE(VOID) vboxVdmaDdiCmdDequeueIrq(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd)
{
    PVBOXVDMADDI_CMD_QUEUE pQueue = &pDevExt->aNodes[pCmd->u32NodeOrdinal].CmdQueue;
    ASMAtomicDecU32(&pQueue->cQueuedCmds);
    RemoveEntryList(&pCmd->QueueEntry);
}

DECLINLINE(VOID) vboxVdmaDdiCmdEnqueueIrq(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd)
{
    PVBOXVDMADDI_CMD_QUEUE pQueue = &pDevExt->aNodes[pCmd->u32NodeOrdinal].CmdQueue;
    ASMAtomicIncU32(&pQueue->cQueuedCmds);
    InsertTailList(&pQueue->CmdQueue, &pCmd->QueueEntry);
}

VOID vboxVdmaDdiNodesInit(PVBOXMP_DEVEXT pDevExt)
{
    for (UINT i = 0; i < RT_ELEMENTS(pDevExt->aNodes); ++i)
    {
        pDevExt->aNodes[i].uLastCompletedFenceId = 0;
        PVBOXVDMADDI_CMD_QUEUE pQueue = &pDevExt->aNodes[i].CmdQueue;
        pQueue->cQueuedCmds = 0;
        InitializeListHead(&pQueue->CmdQueue);
    }
    InitializeListHead(&pDevExt->DpcCmdQueue);
}

BOOLEAN vboxVdmaDdiCmdCompletedIrq(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, DXGK_INTERRUPT_TYPE enmComplType)
{
    if (VBOXVDMADDI_STATE_NOT_DX_CMD == pCmd->enmState)
    {
        InsertTailList(&pDevExt->DpcCmdQueue, &pCmd->QueueEntry);
        return FALSE;
    }

    PVBOXVDMADDI_CMD_QUEUE pQueue = &pDevExt->aNodes[pCmd->u32NodeOrdinal].CmdQueue;
    BOOLEAN bQueued = pCmd->enmState > VBOXVDMADDI_STATE_NOT_QUEUED;
    BOOLEAN bComplete = FALSE;
    Assert(!bQueued || pQueue->cQueuedCmds);
    Assert(!bQueued || !IsListEmpty(&pQueue->CmdQueue));
    pCmd->enmState = VBOXVDMADDI_STATE_COMPLETED;
    if (bQueued)
    {
        if (pQueue->CmdQueue.Flink == &pCmd->QueueEntry)
        {
            vboxVdmaDdiCmdDequeueIrq(pDevExt, pCmd);
            bComplete = TRUE;
        }
    }
    else if (IsListEmpty(&pQueue->CmdQueue))
    {
        bComplete = TRUE;
    }
    else
    {
        vboxVdmaDdiCmdEnqueueIrq(pDevExt, pCmd);
    }

    if (bComplete)
    {
        vboxVdmaDdiCmdProcessCompletedIrq(pDevExt, pCmd, enmComplType);

        while (!IsListEmpty(&pQueue->CmdQueue))
        {
            pCmd = VBOXVDMADDI_CMD_FROM_ENTRY(pQueue->CmdQueue.Flink);
            if (pCmd->enmState == VBOXVDMADDI_STATE_COMPLETED)
            {
                vboxVdmaDdiCmdDequeueIrq(pDevExt, pCmd);
                vboxVdmaDdiCmdProcessCompletedIrq(pDevExt, pCmd, pCmd->enmComplType);
            }
            else
                break;
        }
    }
    else
    {
        pCmd->enmState = VBOXVDMADDI_STATE_COMPLETED;
        pCmd->enmComplType = enmComplType;
    }

    return bComplete;
}

VOID vboxVdmaDdiCmdSubmittedIrq(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd)
{
    BOOLEAN bQueued = pCmd->enmState >= VBOXVDMADDI_STATE_PENDING;
    Assert(pCmd->enmState < VBOXVDMADDI_STATE_SUBMITTED);
    pCmd->enmState = VBOXVDMADDI_STATE_SUBMITTED;
    if (!bQueued)
        vboxVdmaDdiCmdEnqueueIrq(pDevExt, pCmd);
}

typedef struct VBOXVDMADDI_CMD_COMPLETED_CB
{
    PVBOXMP_DEVEXT pDevExt;
    PVBOXVDMADDI_CMD pCmd;
    DXGK_INTERRUPT_TYPE enmComplType;
} VBOXVDMADDI_CMD_COMPLETED_CB, *PVBOXVDMADDI_CMD_COMPLETED_CB;

static BOOLEAN vboxVdmaDdiCmdCompletedCb(PVOID Context)
{
    PVBOXVDMADDI_CMD_COMPLETED_CB pdc = (PVBOXVDMADDI_CMD_COMPLETED_CB)Context;
    PVBOXMP_DEVEXT pDevExt = pdc->pDevExt;
    BOOLEAN bNeedDpc = vboxVdmaDdiCmdCompletedIrq(pDevExt, pdc->pCmd, pdc->enmComplType);
    pDevExt->bNotifyDxDpc |= bNeedDpc;

    if (bNeedDpc)
    {
        pDevExt->u.primary.DxgkInterface.DxgkCbQueueDpc(pDevExt->u.primary.DxgkInterface.DeviceHandle);
    }

    return bNeedDpc;
}

NTSTATUS vboxVdmaDdiCmdCompleted(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, DXGK_INTERRUPT_TYPE enmComplType)
{
    VBOXVDMADDI_CMD_COMPLETED_CB context;
    context.pDevExt = pDevExt;
    context.pCmd = pCmd;
    context.enmComplType = enmComplType;
    BOOLEAN bNeedDps;
    NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbSynchronizeExecution(
            pDevExt->u.primary.DxgkInterface.DeviceHandle,
            vboxVdmaDdiCmdCompletedCb,
            &context,
            0, /* IN ULONG MessageNumber */
            &bNeedDps);
    AssertNtStatusSuccess(Status);
    return Status;
}

typedef struct VBOXVDMADDI_CMD_SUBMITTED_CB
{
    PVBOXMP_DEVEXT pDevExt;
    PVBOXVDMADDI_CMD pCmd;
} VBOXVDMADDI_CMD_SUBMITTED_CB, *PVBOXVDMADDI_CMD_SUBMITTED_CB;

static BOOLEAN vboxVdmaDdiCmdSubmittedCb(PVOID Context)
{
    PVBOXVDMADDI_CMD_SUBMITTED_CB pdc = (PVBOXVDMADDI_CMD_SUBMITTED_CB)Context;
    vboxVdmaDdiCmdSubmittedIrq(pdc->pDevExt, pdc->pCmd);

    return FALSE;
}

NTSTATUS vboxVdmaDdiCmdSubmitted(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd)
{
    VBOXVDMADDI_CMD_SUBMITTED_CB context;
    context.pDevExt = pDevExt;
    context.pCmd = pCmd;
    BOOLEAN bRc;
    NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbSynchronizeExecution(
            pDevExt->u.primary.DxgkInterface.DeviceHandle,
            vboxVdmaDdiCmdSubmittedCb,
            &context,
            0, /* IN ULONG MessageNumber */
            &bRc);
    AssertNtStatusSuccess(Status);
    return Status;
}

typedef struct VBOXVDMADDI_CMD_COMPLETE_CB
{
    PVBOXMP_DEVEXT pDevExt;
    UINT u32NodeOrdinal;
    uint32_t u32FenceId;
} VBOXVDMADDI_CMD_COMPLETE_CB, *PVBOXVDMADDI_CMD_COMPLETE_CB;

static BOOLEAN vboxVdmaDdiCmdFenceCompleteCb(PVOID Context)
{
    PVBOXVDMADDI_CMD_COMPLETE_CB pdc = (PVBOXVDMADDI_CMD_COMPLETE_CB)Context;
    PVBOXMP_DEVEXT pDevExt = pdc->pDevExt;

    vboxVdmaDdiCmdNotifyCompletedIrq(pDevExt, pdc->u32NodeOrdinal, pdc->u32FenceId, DXGK_INTERRUPT_DMA_COMPLETED);

    pDevExt->bNotifyDxDpc = TRUE;
    pDevExt->u.primary.DxgkInterface.DxgkCbQueueDpc(pDevExt->u.primary.DxgkInterface.DeviceHandle);

    return TRUE;
}

static NTSTATUS vboxVdmaDdiCmdFenceNotifyComplete(PVBOXMP_DEVEXT pDevExt, uint32_t u32NodeOrdinal, uint32_t u32FenceId)
{
    VBOXVDMADDI_CMD_COMPLETE_CB context;
    context.pDevExt = pDevExt;
    context.u32NodeOrdinal = u32NodeOrdinal;
    context.u32FenceId = u32FenceId;
    BOOLEAN bRet;
    NTSTATUS Status = pDevExt->u.primary.DxgkInterface.DxgkCbSynchronizeExecution(
            pDevExt->u.primary.DxgkInterface.DeviceHandle,
            vboxVdmaDdiCmdFenceCompleteCb,
            &context,
            0, /* IN ULONG MessageNumber */
            &bRet);
    AssertNtStatusSuccess(Status);
    return Status;
}

NTSTATUS vboxVdmaDdiCmdFenceComplete(PVBOXMP_DEVEXT pDevExt, uint32_t u32NodeOrdinal, uint32_t u32FenceId, DXGK_INTERRUPT_TYPE enmComplType)
{
    if (vboxVdmaDdiCmdCanComplete(pDevExt, u32NodeOrdinal))
        return vboxVdmaDdiCmdFenceNotifyComplete(pDevExt, u32NodeOrdinal, u32FenceId);

    PVBOXVDMADDI_CMD pCmd = (PVBOXVDMADDI_CMD)vboxWddmMemAlloc(sizeof (VBOXVDMADDI_CMD));
    Assert(pCmd);
    if (pCmd)
    {
        vboxVdmaDdiCmdInit(pCmd, u32NodeOrdinal, u32FenceId, vboxVdmaDdiCmdCompletionCbFree, NULL);
        NTSTATUS Status = vboxVdmaDdiCmdCompleted(pDevExt, pCmd, enmComplType);
        AssertNtStatusSuccess(Status);
        if (Status == STATUS_SUCCESS)
            return STATUS_SUCCESS;
        vboxWddmMemFree(pCmd);
        return Status;
    }
    return STATUS_NO_MEMORY;
}

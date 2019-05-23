/* $Id: VBoxMPVhwa.h $ */
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

#ifndef ___VBoxMPVhwa_h___
#define ___VBoxMPVhwa_h___

#include <iprt/cdefs.h>

#include "VBoxMPShgsmi.h"

VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST * vboxVhwaCommandCreate(PVBOXMP_DEVEXT pDevExt,
                                                               D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId,
                                                               VBOXVHWACMD_TYPE enmCmd,
                                                               VBOXVHWACMD_LENGTH cbCmd);

void vboxVhwaCommandFree(PVBOXMP_DEVEXT pDevExt, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST *pCmd);
int  vboxVhwaCommandSubmit(PVBOXMP_DEVEXT pDevExt, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST * pCmd);
void vboxVhwaCommandSubmitAsynchAndComplete(PVBOXMP_DEVEXT pDevExt, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST *pCmd);

#ifndef VBOXVHWA_WITH_SHGSMI
typedef DECLCALLBACK(void) FNVBOXVHWACMDCOMPLETION(PVBOXMP_DEVEXT pDevExt, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST * pCmd,
                                                   void *pvContext);
typedef FNVBOXVHWACMDCOMPLETION *PFNVBOXVHWACMDCOMPLETION;

void vboxVhwaCommandSubmitAsynch(PVBOXMP_DEVEXT pDevExt, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST *pCmd,
                                 PFNVBOXVHWACMDCOMPLETION pfnCompletion, void * pContext);
void vboxVhwaCommandSubmitAsynchByEvent(PVBOXMP_DEVEXT pDevExt, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST *pCmd, RTSEMEVENT hEvent);

#define VBOXVHWA_CMD2LISTENTRY(_pCmd)   ((PVBOXVTLIST_ENTRY)&(_pCmd)->u.pNext)
#define VBOXVHWA_LISTENTRY2CMD(_pEntry) ( (VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST *)((uint8_t *)(_pEntry) - RT_OFFSETOF(VBOXVHWACMD, u.pNext)) )

DECLINLINE(void) vboxVhwaPutList(VBOXVTLIST *pList, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_HOST *pCmd)
{
    vboxVtListPut(pList, VBOXVHWA_CMD2LISTENTRY(pCmd), VBOXVHWA_CMD2LISTENTRY(pCmd));
}

void vboxVhwaCompletionListProcess(PVBOXMP_DEVEXT pDevExt, VBOXVTLIST *pList);
#endif

int vboxVhwaEnable(PVBOXMP_DEVEXT pDevExt, D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId);
int vboxVhwaDisable(PVBOXMP_DEVEXT pDevExt, D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId);
void vboxVhwaInit(PVBOXMP_DEVEXT pDevExt);
void vboxVhwaFree(PVBOXMP_DEVEXT pDevExt);

int vboxVhwaHlpOverlayFlip(PVBOXWDDM_OVERLAY pOverlay, const DXGKARG_FLIPOVERLAY *pFlipInfo);
int vboxVhwaHlpOverlayUpdate(PVBOXWDDM_OVERLAY pOverlay, const DXGK_OVERLAYINFO *pOverlayInfo);
int vboxVhwaHlpOverlayDestroy(PVBOXWDDM_OVERLAY pOverlay);
int vboxVhwaHlpOverlayCreate(PVBOXMP_DEVEXT pDevExt, D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, DXGK_OVERLAYINFO *pOverlayInfo, /* OUT */ PVBOXWDDM_OVERLAY pOverlay);

int vboxVhwaHlpColorFill(PVBOXWDDM_OVERLAY pOverlay, PVBOXWDDM_DMA_PRIVATEDATA_CLRFILL pCF);

int vboxVhwaHlpGetSurfInfo(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_ALLOCATION pSurf);

BOOLEAN vboxVhwaHlpOverlayListIsEmpty(PVBOXMP_DEVEXT pDevExt, D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId);
//void vboxVhwaHlpOverlayListAdd(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_OVERLAY pOverlay);
//void vboxVhwaHlpOverlayListRemove(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_OVERLAY pOverlay);
void vboxVhwaHlpOverlayDstRectUnion(PVBOXMP_DEVEXT pDevExt, D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, RECT *pRect);
void vboxVhwaHlpOverlayDstRectGet(PVBOXMP_DEVEXT pDevExt, PVBOXWDDM_OVERLAY pOverlay, RECT *pRect);

#endif /* #ifndef ___VBoxMPVhwa_h___ */

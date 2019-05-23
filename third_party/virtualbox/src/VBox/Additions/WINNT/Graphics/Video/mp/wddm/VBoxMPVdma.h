/* $Id: VBoxMPVdma.h $ */
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

#ifndef ___VBoxMPVdma_h___
#define ___VBoxMPVdma_h___

#include <iprt/cdefs.h>
#include <iprt/asm.h>
#include <VBoxVideo.h>
#include <HGSMI.h>

typedef struct _VBOXMP_DEVEXT *PVBOXMP_DEVEXT;

/* ddi dma command queue handling */
typedef enum
{
    VBOXVDMADDI_STATE_UNCKNOWN = 0,
    VBOXVDMADDI_STATE_NOT_DX_CMD,
    VBOXVDMADDI_STATE_NOT_QUEUED,
    VBOXVDMADDI_STATE_PENDING,
    VBOXVDMADDI_STATE_SUBMITTED,
    VBOXVDMADDI_STATE_COMPLETED
} VBOXVDMADDI_STATE;

typedef struct VBOXVDMADDI_CMD *PVBOXVDMADDI_CMD;
typedef DECLCALLBACK(VOID) FNVBOXVDMADDICMDCOMPLETE_DPC(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, PVOID pvContext);
typedef FNVBOXVDMADDICMDCOMPLETE_DPC *PFNVBOXVDMADDICMDCOMPLETE_DPC;

typedef struct VBOXVDMADDI_CMD
{
    LIST_ENTRY QueueEntry;
    VBOXVDMADDI_STATE enmState;
    uint32_t u32NodeOrdinal;
    uint32_t u32FenceId;
    DXGK_INTERRUPT_TYPE enmComplType;
    PFNVBOXVDMADDICMDCOMPLETE_DPC pfnComplete;
    PVOID pvComplete;
} VBOXVDMADDI_CMD, *PVBOXVDMADDI_CMD;

typedef struct VBOXVDMADDI_CMD_QUEUE
{
    volatile uint32_t cQueuedCmds;
    LIST_ENTRY CmdQueue;
} VBOXVDMADDI_CMD_QUEUE, *PVBOXVDMADDI_CMD_QUEUE;

typedef struct VBOXVDMADDI_NODE
{
    VBOXVDMADDI_CMD_QUEUE CmdQueue;
    UINT uLastCompletedFenceId;
} VBOXVDMADDI_NODE, *PVBOXVDMADDI_NODE;

VOID vboxVdmaDdiNodesInit(PVBOXMP_DEVEXT pDevExt);
BOOLEAN vboxVdmaDdiCmdCompletedIrq(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, DXGK_INTERRUPT_TYPE enmComplType);
VOID vboxVdmaDdiCmdSubmittedIrq(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd);

NTSTATUS vboxVdmaDdiCmdCompleted(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, DXGK_INTERRUPT_TYPE enmComplType);
NTSTATUS vboxVdmaDdiCmdSubmitted(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd);

DECLINLINE(VOID) vboxVdmaDdiCmdInit(PVBOXVDMADDI_CMD pCmd, uint32_t u32NodeOrdinal, uint32_t u32FenceId,
                                    PFNVBOXVDMADDICMDCOMPLETE_DPC pfnComplete, PVOID pvComplete)
{
    pCmd->QueueEntry.Blink = NULL;
    pCmd->QueueEntry.Flink = NULL;
    pCmd->enmState = VBOXVDMADDI_STATE_NOT_QUEUED;
    pCmd->u32NodeOrdinal = u32NodeOrdinal;
    pCmd->u32FenceId = u32FenceId;
    pCmd->pfnComplete = pfnComplete;
    pCmd->pvComplete = pvComplete;
}

/* marks the command a submitted in a way that it is invisible for dx runtime,
 * i.e. the dx runtime won't be notified about the command completion
 * this is used to submit commands initiated by the driver, but not by the dx runtime */
DECLINLINE(VOID) vboxVdmaDdiCmdSubmittedNotDx(PVBOXVDMADDI_CMD pCmd)
{
    Assert(pCmd->enmState == VBOXVDMADDI_STATE_NOT_QUEUED);
    pCmd->enmState = VBOXVDMADDI_STATE_NOT_DX_CMD;
}

NTSTATUS vboxVdmaDdiCmdFenceComplete(PVBOXMP_DEVEXT pDevExt, uint32_t u32NodeOrdinal, uint32_t u32FenceId, DXGK_INTERRUPT_TYPE enmComplType);

DECLCALLBACK(VOID) vboxVdmaDdiCmdCompletionCbFree(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pCmd, PVOID pvContext);

VOID vboxVdmaDdiCmdGetCompletedListIsr(PVBOXMP_DEVEXT pDevExt, LIST_ENTRY *pList);

BOOLEAN vboxVdmaDdiCmdIsCompletedListEmptyIsr(PVBOXMP_DEVEXT pDevExt);

#define VBOXVDMADDI_CMD_FROM_ENTRY(_pEntry) ((PVBOXVDMADDI_CMD)(((uint8_t*)(_pEntry)) - RT_OFFSETOF(VBOXVDMADDI_CMD, QueueEntry)))

DECLINLINE(VOID) vboxVdmaDdiCmdHandleCompletedList(PVBOXMP_DEVEXT pDevExt, LIST_ENTRY *pList)
{
    LIST_ENTRY *pEntry = pList->Flink;
    while (pEntry != pList)
    {
        PVBOXVDMADDI_CMD pCmd = VBOXVDMADDI_CMD_FROM_ENTRY(pEntry);
        pEntry = pEntry->Flink;
        if (pCmd->pfnComplete)
            pCmd->pfnComplete(pDevExt, pCmd, pCmd->pvComplete);
    }
}

#if 0
typedef DECLCALLBACK(int) FNVBOXVDMASUBMIT(struct _DEVICE_EXTENSION* pDevExt, struct VBOXVDMAINFO * pInfo, HGSMIOFFSET offDr, PVOID pvContext);
typedef FNVBOXVDMASUBMIT *PFNVBOXVDMASUBMIT;

typedef struct VBOXVDMASUBMIT
{
    PFNVBOXVDMASUBMIT pfnSubmit;
    PVOID pvContext;
} VBOXVDMASUBMIT, *PVBOXVDMASUBMIT;
#endif

/* start */
typedef enum
{
    VBOXVDMAPIPE_STATE_CLOSED    = 0,
    VBOXVDMAPIPE_STATE_CREATED   = 1,
    VBOXVDMAPIPE_STATE_OPENNED   = 2,
    VBOXVDMAPIPE_STATE_CLOSING   = 3
} VBOXVDMAPIPE_STATE;

typedef struct VBOXVDMAPIPE
{
    KSPIN_LOCK SinchLock;
    KEVENT Event;
    LIST_ENTRY CmdListHead;
    VBOXVDMAPIPE_STATE enmState;
    /* true iff the other end needs Event notification */
    bool bNeedNotify;
} VBOXVDMAPIPE, *PVBOXVDMAPIPE;

typedef struct VBOXVDMAPIPE_CMD_HDR
{
    LIST_ENTRY ListEntry;
} VBOXVDMAPIPE_CMD_HDR, *PVBOXVDMAPIPE_CMD_HDR;

#define VBOXVDMAPIPE_CMD_HDR_FROM_ENTRY(_pE)  ( (PVBOXVDMAPIPE_CMD_HDR)((uint8_t *)(_pE) - RT_OFFSETOF(VBOXVDMAPIPE_CMD_HDR, ListEntry)) )

typedef enum
{
    VBOXVDMAPIPE_CMD_TYPE_UNDEFINED = 0,
    VBOXVDMAPIPE_CMD_TYPE_RECTSINFO,
    VBOXVDMAPIPE_CMD_TYPE_DMACMD,
    VBOXVDMAPIPE_CMD_TYPE_FINISH, /* ensures all previously submitted commands are completed */
    VBOXVDMAPIPE_CMD_TYPE_CANCEL
} VBOXVDMAPIPE_CMD_TYPE;

typedef struct VBOXVDMAPIPE_CMD_DR
{
    VBOXVDMAPIPE_CMD_HDR PipeHdr;
    VBOXVDMAPIPE_CMD_TYPE enmType;
    volatile uint32_t cRefs;
} VBOXVDMAPIPE_CMD_DR, *PVBOXVDMAPIPE_CMD_DR;

#define VBOXVDMAPIPE_CMD_DR_FROM_ENTRY(_pE)  ( (PVBOXVDMAPIPE_CMD_DR)VBOXVDMAPIPE_CMD_HDR_FROM_ENTRY(_pE) )

typedef struct VBOXWDDM_DMA_ALLOCINFO
{
    PVBOXWDDM_ALLOCATION pAlloc;
    VBOXVIDEOOFFSET offAlloc;
    UINT segmentIdAlloc : 31;
    UINT fWriteOp : 1;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId;
} VBOXWDDM_DMA_ALLOCINFO, *PVBOXWDDM_DMA_ALLOCINFO;

typedef struct VBOXVDMAPIPE_RECTS
{
    RECT ContextRect;
    VBOXWDDM_RECTS_INFO UpdateRects;
} VBOXVDMAPIPE_RECTS, *PVBOXVDMAPIPE_RECTS;

typedef struct VBOXVDMAPIPE_CMD_RECTSINFO
{
    VBOXVDMAPIPE_CMD_DR Hdr;
    PVBOXWDDM_CONTEXT pContext;
    struct VBOXWDDM_SWAPCHAIN *pSwapchain;
    VBOXVDMAPIPE_RECTS ContextsRects;
} VBOXVDMAPIPE_CMD_RECTSINFO, *PVBOXVDMAPIPE_CMD_RECTSINFO;

typedef struct VBOXVDMAPIPE_CMD_DMACMD
{
    VBOXVDMAPIPE_CMD_DR Hdr;
#ifndef VBOX_WDDM_IRQ_COMPLETION
    VBOXVDMADDI_CMD DdiCmd;
#endif
    PVBOXWDDM_CONTEXT pContext;
    VBOXVDMACMD_TYPE enmCmd;
//    VBOXVDMAPIPE_FLAGS_DMACMD fFlags;
} VBOXVDMAPIPE_CMD_DMACMD, *PVBOXVDMAPIPE_CMD_DMACMD;

typedef struct VBOXVDMA_CLRFILL
{
    VBOXWDDM_DMA_ALLOCINFO Alloc;
    UINT Color;
    VBOXWDDM_RECTS_INFO Rects;
} VBOXVDMA_CLRFILL, *PVBOXVDMA_CLRFILL;

typedef struct VBOXVDMAPIPE_CMD_DMACMD_CLRFILL
{
    VBOXVDMAPIPE_CMD_DMACMD Hdr;
    VBOXVDMA_CLRFILL ClrFill;
} VBOXVDMAPIPE_CMD_DMACMD_CLRFILL, *PVBOXVDMAPIPE_CMD_DMACMD_CLRFILL;

typedef struct VBOXVDMA_BLT
{
    VBOXWDDM_DMA_ALLOCINFO SrcAlloc;
    VBOXWDDM_DMA_ALLOCINFO DstAlloc;
    RECT SrcRect;
    VBOXVDMAPIPE_RECTS DstRects;
} VBOXVDMA_BLT, *PVBOXVDMA_BLT;

typedef struct VBOXVDMAPIPE_CMD_DMACMD_BLT
{
    VBOXVDMAPIPE_CMD_DMACMD Hdr;
    VBOXVDMA_BLT Blt;
} VBOXVDMAPIPE_CMD_DMACMD_BLT, *PVBOXVDMAPIPE_CMD_DMACMD_BLT;

typedef struct VBOXVDMA_FLIP
{
    VBOXWDDM_DMA_ALLOCINFO Alloc;
} VBOXVDMA_FLIP, *PVBOXVDMA_FLIP;

typedef struct VBOXVDMAPIPE_CMD_DMACMD_FLIP
{
    VBOXVDMAPIPE_CMD_DMACMD Hdr;
    VBOXVDMA_FLIP Flip;
} VBOXVDMAPIPE_CMD_DMACMD_FLIP, *PVBOXVDMAPIPE_CMD_DMACMD_FLIP;

typedef struct VBOXVDMA_SHADOW2PRIMARY
{
    VBOXWDDM_DMA_ALLOCINFO ShadowAlloc;
    RECT SrcRect;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} VBOXVDMA_SHADOW2PRIMARY, *PVBOXVDMA_SHADOW2PRIMARY;

typedef struct VBOXVDMAGG
{
    VBOXVDMAPIPE CmdPipe;
    PKTHREAD pThread;
} VBOXVDMAGG, *PVBOXVDMAGG;

/* DMA commands are currently submitted over HGSMI */
typedef struct VBOXVDMAINFO
{
#ifdef VBOX_WITH_VDMA
    VBOXSHGSMI CmdHeap;
#endif
    UINT      uLastCompletedPagingBufferCmdFenceId;
    BOOL      fEnabled;
} VBOXVDMAINFO, *PVBOXVDMAINFO;

int vboxVdmaCreate (PVBOXMP_DEVEXT pDevExt, VBOXVDMAINFO *pInfo
#ifdef VBOX_WITH_VDMA
        , ULONG offBuffer, ULONG cbBuffer
#endif
#if 0
        , PFNVBOXVDMASUBMIT pfnSubmit, PVOID pvContext
#endif
        );
int vboxVdmaDisable(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo);
int vboxVdmaEnable(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo);
int vboxVdmaDestroy(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo);

#ifdef VBOX_WITH_VDMA
int vboxVdmaFlush(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo);
DECLINLINE(HGSMIOFFSET) vboxVdmaCBufDrPtrOffset(const PVBOXVDMAINFO pInfo, const void RT_UNTRUSTED_VOLATILE_HOST *pvPtr)
{
    return VBoxSHGSMICommandPtrOffset(&pInfo->CmdHeap, pvPtr);
}
int vboxVdmaCBufDrSubmit(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr);
int vboxVdmaCBufDrSubmitSynch(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAINFO pInfo, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr);
struct VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *vboxVdmaCBufDrCreate(PVBOXVDMAINFO pInfo, uint32_t cbTrailingData);
void vboxVdmaCBufDrFree(PVBOXVDMAINFO pInfo, struct VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr);

#define VBOXVDMACBUF_DR_DATA_OFFSET() (sizeof (VBOXVDMACBUF_DR))
#define VBOXVDMACBUF_DR_SIZE(_cbData) (VBOXVDMACBUF_DR_DATA_OFFSET() + (_cbData))
#define VBOXVDMACBUF_DR_DATA(_pDr) ( ((uint8_t*)(_pDr)) + VBOXVDMACBUF_DR_DATA_OFFSET() )

AssertCompile(sizeof (VBOXVDMADDI_CMD) <= RT_SIZEOFMEMB(VBOXVDMACBUF_DR, aGuestData));
#define VBOXVDMADDI_CMD_FROM_BUF_DR(_pDr) ((PVBOXVDMADDI_CMD)(_pDr)->aGuestData)
#define VBOXVDMACBUF_DR_FROM_DDI_CMD(_pCmd) ((PVBOXVDMACBUF_DR)(((uint8_t*)(_pCmd)) - RT_OFFSETOF(VBOXVDMACBUF_DR, aGuestData)))

#endif
#ifdef VBOX_WITH_CROGL
NTSTATUS vboxVdmaPostHideSwapchain(PVBOXWDDM_SWAPCHAIN pSwapchain);
#endif

NTSTATUS vboxVdmaGgCmdDmaNotifyCompleted(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAPIPE_CMD_DMACMD pCmd, DXGK_INTERRUPT_TYPE enmComplType);
NTSTATUS vboxVdmaGgCmdDmaNotifySubmitted(PVBOXMP_DEVEXT pDevExt, PVBOXVDMAPIPE_CMD_DMACMD pCmd);
VOID vboxVdmaGgCmdDmaNotifyInit(PVBOXVDMAPIPE_CMD_DMACMD pCmd,
        uint32_t u32NodeOrdinal, uint32_t u32FenceId,
        PFNVBOXVDMADDICMDCOMPLETE_DPC pfnComplete, PVOID pvComplete);

NTSTATUS vboxVdmaGgDmaBltPerform(PVBOXMP_DEVEXT pDevExt, struct VBOXWDDM_ALLOC_DATA * pSrcAlloc, RECT* pSrcRect,
        struct VBOXWDDM_ALLOC_DATA *pDstAlloc, RECT* pDstRect);

#define VBOXVDMAPIPE_CMD_DR_FROM_DDI_CMD(_pCmd) ((PVBOXVDMAPIPE_CMD_DR)(((uint8_t*)(_pCmd)) - RT_OFFSETOF(VBOXVDMAPIPE_CMD_DR, DdiCmd)))

NTSTATUS vboxVdmaProcessBltCmd(PVBOXMP_DEVEXT pDevExt, struct VBOXWDDM_CONTEXT *pContext, struct VBOXWDDM_DMA_PRIVATEDATA_BLT *pBlt);
NTSTATUS vboxVdmaProcessFlipCmd(PVBOXMP_DEVEXT pDevExt, struct VBOXWDDM_CONTEXT *pContext, struct VBOXWDDM_DMA_PRIVATEDATA_FLIP *pFlip);
NTSTATUS vboxVdmaProcessClrFillCmd(PVBOXMP_DEVEXT pDevExt, struct VBOXWDDM_CONTEXT *pContext, struct VBOXWDDM_DMA_PRIVATEDATA_CLRFILL *pCF);
#ifdef VBOX_WITH_CROGL
NTSTATUS vboxVdmaTexPresentSetAlloc(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData);
NTSTATUS VBoxVdmaChromiumParameteriCRSubmit(PVBOXMP_DEVEXT pDevExt, uint32_t target, uint32_t value);
#endif

#endif /* #ifndef ___VBoxMPVdma_h___ */

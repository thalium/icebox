/* $Id: VBoxMPVbva.h $ */
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

#ifndef ___VBoxMPVbva_h___
#define ___VBoxMPVbva_h___

#include <VBox/cdefs.h>  /* for VBOXCALL */

typedef struct VBOXVBVAINFO
{
    VBVABUFFERCONTEXT Vbva;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId;
    KSPIN_LOCK Lock;
} VBOXVBVAINFO;

int vboxVbvaEnable(PVBOXMP_DEVEXT pDevExt, VBOXVBVAINFO *pVbva);
int vboxVbvaDisable(PVBOXMP_DEVEXT pDevExt, VBOXVBVAINFO *pVbva);
int vboxVbvaDestroy(PVBOXMP_DEVEXT pDevExt, VBOXVBVAINFO *pVbva);
int vboxVbvaCreate(PVBOXMP_DEVEXT pDevExt, VBOXVBVAINFO *pVbva, ULONG offBuffer, ULONG cbBuffer, D3DDDI_VIDEO_PRESENT_SOURCE_ID srcId);
int vboxVbvaReportDirtyRect(PVBOXMP_DEVEXT pDevExt, struct VBOXWDDM_SOURCE *pSrc, RECT *pRectOrig);

#define VBOXVBVA_OP(_op, _pdext, _psrc, _arg) \
        do { \
            if (VBoxVBVABufferBeginUpdate(&(_psrc)->Vbva.Vbva, &VBoxCommonFromDeviceExt(_pdext)->guestCtx)) \
            { \
                vboxVbva##_op(_pdext, _psrc, _arg); \
                VBoxVBVABufferEndUpdate(&(_psrc)->Vbva.Vbva); \
            } \
        } while (0)

#define VBOXVBVA_OP_WITHLOCK_ATDPC(_op, _pdext, _psrc, _arg) \
        do { \
            Assert(KeGetCurrentIrql() == DISPATCH_LEVEL); \
            KeAcquireSpinLockAtDpcLevel(&(_psrc)->Vbva.Lock);  \
            VBOXVBVA_OP(_op, _pdext, _psrc, _arg);        \
            KeReleaseSpinLockFromDpcLevel(&(_psrc)->Vbva.Lock);\
        } while (0)

#define VBOXVBVA_OP_WITHLOCK(_op, _pdext, _psrc, _arg) \
        do { \
            KIRQL OldIrql; \
            KeAcquireSpinLock(&(_psrc)->Vbva.Lock, &OldIrql);  \
            VBOXVBVA_OP(_op, _pdext, _psrc, _arg);        \
            KeReleaseSpinLock(&(_psrc)->Vbva.Lock, OldIrql);   \
        } while (0)


#ifdef VBOX_WITH_CROGL
/* customized VBVA implementation */
struct VBVAEXBUFFERCONTEXT;

typedef DECLCALLBACKPTR(void, PFNVBVAEXBUFFERFLUSH) (struct VBVAEXBUFFERCONTEXT *pCtx, PHGSMIGUESTCOMMANDCONTEXT pHGSMICtx, void *pvFlush);

/**
 * Structure grouping the context needed for sending graphics acceleration
 * information to the host via VBVA.  Each screen has its own VBVA buffer.
 */
typedef struct VBVAEXBUFFERCONTEXT
{
    /** Offset of the buffer in the VRAM section for the screen */
    uint32_t    offVRAMBuffer;
    /** Length of the buffer in bytes */
    uint32_t    cbBuffer;
    /** This flag is set if we wrote to the buffer faster than the host could
     * read it. */
    bool        fHwBufferOverflow;
    /* the window between indexRecordFirstUncompleted and pVBVA->::indexRecordFirst represents
     * command records processed by the host, but not completed by the guest yet */
    volatile uint32_t    indexRecordFirstUncompleted;
    /* the window between off32DataUncompleted and pVBVA->::off32Data represents
     * command data processed by the host, but not completed by the guest yet */
    uint32_t    off32DataUncompleted;
    /* flush function */
    PFNVBVAEXBUFFERFLUSH pfnFlush;
    void *pvFlush;
    /** The VBVA record that we are currently preparing for the host, NULL if
     * none. */
    struct VBVARECORD *pRecord;
    /** Pointer to the VBVA buffer mapped into the current address space.  Will
     * be NULL if VBVA is not enabled. */
    struct VBVABUFFER *pVBVA;
} VBVAEXBUFFERCONTEXT, *PVBVAEXBUFFERCONTEXT;

typedef struct VBVAEXBUFFERITERBASE
{
    struct VBVAEXBUFFERCONTEXT *pCtx;
    /* index of the current record */
    uint32_t iCurRecord;
    /* offset of the current command */
    uint32_t off32CurCmd;
} VBVAEXBUFFERITERBASE, *PVBVAEXBUFFERITERBASE;

typedef struct VBVAEXBUFFERFORWARDITER
{
    VBVAEXBUFFERITERBASE Base;
} VBVAEXBUFFERFORWARDITER, *PVBVAEXBUFFERFORWARDITER;

typedef struct VBVAEXBUFFERBACKWARDITER
{
    VBVAEXBUFFERITERBASE Base;
} VBVAEXBUFFERBACKWARDITER, *PVBVAEXBUFFERBACKWARDITER;

#define VBOXCMDVBVA_BUFFERSIZE(_cbCmdApprox) (RT_OFFSETOF(VBVABUFFER, au8Data) + ((RT_SIZEOFMEMB(VBVABUFFER, aRecords)/RT_SIZEOFMEMB(VBVABUFFER, aRecords[0])) * (_cbCmdApprox)))

typedef struct VBOXCMDVBVA_PREEMPT_EL
{
    uint32_t u32SubmitFence;
    uint32_t u32PreemptFence;
} VBOXCMDVBVA_PREEMPT_EL;

#define VBOXCMDVBVA_PREEMPT_EL_SIZE 16

typedef struct VBOXCMDVBVA
{
    VBVAEXBUFFERCONTEXT Vbva;

    /* last completted fence id */
    uint32_t u32FenceCompleted;
    /* last submitted fence id */
    uint32_t u32FenceSubmitted;
    /* last processed fence id (i.e. either completed or cancelled) */
    uint32_t u32FenceProcessed;

    /* node ordinal */
    uint32_t idNode;

    uint32_t cPreempt;
    uint32_t iCurPreempt;
    VBOXCMDVBVA_PREEMPT_EL aPreempt[VBOXCMDVBVA_PREEMPT_EL_SIZE];
} VBOXCMDVBVA;

/** @name VBVAEx APIs
 * @{ */
#define VBVAEX_DECL(type) type VBOXCALL
VBVAEX_DECL(int) VBoxVBVAExEnable(PVBVAEXBUFFERCONTEXT pCtx, PHGSMIGUESTCOMMANDCONTEXT pHGSMICtx, struct VBVABUFFER *pVBVA);
VBVAEX_DECL(void) VBoxVBVAExDisable(PVBVAEXBUFFERCONTEXT pCtx, PHGSMIGUESTCOMMANDCONTEXT pHGSMICtx);
VBVAEX_DECL(bool) VBoxVBVAExBufferBeginUpdate(PVBVAEXBUFFERCONTEXT pCtx, PHGSMIGUESTCOMMANDCONTEXT pHGSMICtx);
VBVAEX_DECL(void) VBoxVBVAExBufferEndUpdate(PVBVAEXBUFFERCONTEXT pCtx);
VBVAEX_DECL(bool) VBoxVBVAExWrite(PVBVAEXBUFFERCONTEXT pCtx, PHGSMIGUESTCOMMANDCONTEXT pHGSMICtx, const void *pv, uint32_t cb);

VBVAEX_DECL(bool) VBoxVBVAExOrderSupported(PVBVAEXBUFFERCONTEXT pCtx, unsigned code);

VBVAEX_DECL(void) VBoxVBVAExSetupBufferContext(PVBVAEXBUFFERCONTEXT pCtx, uint32_t offVRAMBuffer, uint32_t cbBuffer,
                                        PFNVBVAEXBUFFERFLUSH pfnFlush, void *pvFlush);

DECLINLINE(uint32_t) VBoxVBVAExGetSize(PVBVAEXBUFFERCONTEXT pCtx)
{
    return pCtx->pVBVA->cbData;
}

/** can be used to ensure the command will not cross the ring buffer boundary,
 * and thus will not be splitted */
VBVAEX_DECL(uint32_t) VBoxVBVAExGetFreeTail(PVBVAEXBUFFERCONTEXT pCtx);
/** allocates a contiguous buffer of a given size, i.e. the one that is not splitted across ringbuffer boundaries */
VBVAEX_DECL(void *) VBoxVBVAExAllocContiguous(PVBVAEXBUFFERCONTEXT pCtx, PHGSMIGUESTCOMMANDCONTEXT pHGSMICtx, uint32_t cb);
/** answers whether host is in "processing" state now,
 * i.e. if "processing" is true after the command is submitted, no notification is required to be posted to host to make the commandbe processed,
 * otherwise, host should be notified about the command */
VBVAEX_DECL(bool) VBoxVBVAExIsProcessing(PVBVAEXBUFFERCONTEXT pCtx);

/** initializes iterator that starts with free record,
 * i.e. VBoxVBVAExIterNext would return the first uncompleted record.
 *
 * can be used by submitter only */
VBVAEX_DECL(void) VBoxVBVAExBIterInit(PVBVAEXBUFFERCONTEXT pCtx, PVBVAEXBUFFERBACKWARDITER pIter);
/** can be used by submitter only */
VBVAEX_DECL(void *) VBoxVBVAExBIterNext(PVBVAEXBUFFERBACKWARDITER pIter, uint32_t *pcbBuffer, bool *pfProcessed);

/* completer functions
 * completer can only use below ones, and submitter is NOT allowed to use them.
 * Completter functions are prefixed with VBoxVBVAExC as opposed to submitter ones,
 * that do not have the last "C" in the prefix */
/** initializes iterator that starts with completed record,
 * i.e. VBoxVBVAExIterPrev would return the first uncompleted record.
 * note that we can not have iterator that starts at processed record
 * (i.e. the one processed by host, but not completed by guest, since host modifies
 * VBVABUFFER::off32Data and VBVABUFFER::indexRecordFirst concurrently,
 * and so we may end up with inconsistent index-offData pair
 *
 * can be used by completer only */
VBVAEX_DECL(void) VBoxVBVAExCFIterInit(PVBVAEXBUFFERCONTEXT pCtx, PVBVAEXBUFFERFORWARDITER pIter);
/** can be used by completer only */
VBVAEX_DECL(void *) VBoxVBVAExCFIterNext(PVBVAEXBUFFERFORWARDITER pIter, uint32_t *pcbBuffer, bool *pfProcessed);

VBVAEX_DECL(void) VBoxVBVAExCBufferCompleted(PVBVAEXBUFFERCONTEXT pCtx);
/** @}  */

struct VBOXCMDVBVA_HDR;

int VBoxCmdVbvaEnable(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva);
int VBoxCmdVbvaDisable(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva);
int VBoxCmdVbvaDestroy(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva);
int VBoxCmdVbvaCreate(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva, ULONG offBuffer, ULONG cbBuffer);
int VBoxCmdVbvaSubmit(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva, struct VBOXCMDVBVA_HDR *pCmd, uint32_t u32FenceID, uint32_t cbCmd);
void VBoxCmdVbvaSubmitUnlock(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva, VBOXCMDVBVA_HDR* pCmd, uint32_t u32FenceID);
VBOXCMDVBVA_HDR* VBoxCmdVbvaSubmitLock(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva, uint32_t cbCmd);
bool VBoxCmdVbvaPreempt(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva, uint32_t u32FenceID);
uint32_t VBoxCmdVbvaCheckCompleted(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva, bool fPingHost, uint32_t *pu32FenceSubmitted, uint32_t *pu32FenceProcessed);
bool VBoxCmdVbvaCheckCompletedIrq(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva);

/*helper functions for filling vbva commands */
DECLINLINE(void) VBoxCVDdiPackRect(VBOXCMDVBVA_RECT *pVbvaRect, const RECT *pRect)
{
    pVbvaRect->xLeft = (int16_t)pRect->left;
    pVbvaRect->yTop = (int16_t)pRect->top;
    pVbvaRect->xRight = (int16_t)pRect->right;
    pVbvaRect->yBottom = (int16_t)pRect->bottom;
}

DECLINLINE(void) VBoxCVDdiPackRects(VBOXCMDVBVA_RECT *paVbvaRects, const RECT *paRects, uint32_t cRects)
{
    for (uint32_t i = 0; i < cRects; ++i)
    {
        VBoxCVDdiPackRect(&paVbvaRects[i], &paRects[i]);
    }

}

uint32_t VBoxCVDdiPTransferVRamSysBuildEls(VBOXCMDVBVA_PAGING_TRANSFER *pCmd, PMDL pMdl, uint32_t iPfn, uint32_t cPages, uint32_t cbBuffer, uint32_t *pcPagesWritten);

int VBoxCmdVbvaConConnect(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva,
        uint32_t crVersionMajor, uint32_t crVersionMinor,
        uint32_t *pu32ClientID);
int VBoxCmdVbvaConDisconnect(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA *pVbva, uint32_t u32ClientID);
VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST *VBoxCmdVbvaConCmdAlloc(PVBOXMP_DEVEXT pDevExt, uint32_t cbCmd);
void VBoxCmdVbvaConCmdFree(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST *pCmd);
int VBoxCmdVbvaConCmdSubmitAsync(PVBOXMP_DEVEXT pDevExt, VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST *pCmd,
                                 PFNVBOXSHGSMICMDCOMPLETION pfnCompletion, void RT_UNTRUSTED_VOLATILE_HOST *pvCompletion);
int VBoxCmdVbvaConCmdCompletionData(void RT_UNTRUSTED_VOLATILE_HOST *pvCmd, VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST **ppCmd);
int VBoxCmdVbvaConCmdResize(PVBOXMP_DEVEXT pDevExt, const VBOXWDDM_ALLOC_DATA *pAllocData, const uint32_t *pTargetMap, const POINT * pVScreenPos, uint16_t fFlags);
#endif /* #ifdef VBOX_WITH_CROGL */

#endif /* #ifndef ___VBoxMPVbva_h___ */

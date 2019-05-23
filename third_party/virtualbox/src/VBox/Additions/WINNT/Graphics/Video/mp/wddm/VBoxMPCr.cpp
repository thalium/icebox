/* $Id: VBoxMPCr.cpp $ */
/** @file
 * VBox WDDM Miniport driver
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

#ifdef VBOX_WITH_CROGL

#include "VBoxMPWddm.h"
#include "VBoxMPCr.h"

#include <VBox/HostServices/VBoxCrOpenGLSvc.h>

#include <cr_protocol.h>

CR_CAPS_INFO g_VBoxMpCrHostCapsInfo;
static uint32_t g_VBoxMpCr3DSupported = 0;

uint32_t VBoxMpCrGetHostCaps(void)
{
    return g_VBoxMpCrHostCapsInfo.u32Caps;
}

bool VBoxMpCrCtlConIs3DSupported(void)
{
    return !!g_VBoxMpCr3DSupported;
}

static void RT_UNTRUSTED_VOLATILE_HOST *vboxMpCrShgsmiBufferAlloc(PVBOXMP_DEVEXT pDevExt, HGSMISIZE cbData)
{
    return VBoxSHGSMIHeapBufferAlloc(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, cbData);
}

static VBOXVIDEOOFFSET vboxMpCrShgsmiBufferOffset(PVBOXMP_DEVEXT pDevExt, void RT_UNTRUSTED_VOLATILE_HOST *pvBuffer)
{
    return (VBOXVIDEOOFFSET)HGSMIPointerToOffset(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx.Heap.area, pvBuffer);
}

static void RT_UNTRUSTED_VOLATILE_HOST *vboxMpCrShgsmiBufferFromOffset(PVBOXMP_DEVEXT pDevExt, VBOXVIDEOOFFSET offBuffer)
{
    return HGSMIOffsetToPointer(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx.Heap.area, (HGSMIOFFSET)offBuffer);
}

static void vboxMpCrShgsmiBufferFree(PVBOXMP_DEVEXT pDevExt, void RT_UNTRUSTED_VOLATILE_HOST *pvBuffer)
{
    VBoxSHGSMIHeapBufferFree(&VBoxCommonFromDeviceExt(pDevExt)->guestCtx.heapCtx, pvBuffer);
}

static VBOXVIDEOOFFSET vboxMpCrShgsmiTransportBufOffset(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvBuffer)
{
    return vboxMpCrShgsmiBufferOffset(pCon->pDevExt, pvBuffer);
}

static void RT_UNTRUSTED_VOLATILE_HOST *vboxMpCrShgsmiTransportBufFromOffset(PVBOXMP_CRSHGSMITRANSPORT pCon, VBOXVIDEOOFFSET offBuffer)
{
    return vboxMpCrShgsmiBufferFromOffset(pCon->pDevExt, offBuffer);
}

void RT_UNTRUSTED_VOLATILE_HOST *VBoxMpCrShgsmiTransportBufAlloc(PVBOXMP_CRSHGSMITRANSPORT pCon, uint32_t cbBuffer)
{
    return vboxMpCrShgsmiBufferAlloc(pCon->pDevExt, cbBuffer);
}

void VBoxMpCrShgsmiTransportBufFree(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvBuffer)
{
    vboxMpCrShgsmiBufferFree(pCon->pDevExt, pvBuffer);
}

static int vboxMpCrShgsmiBufCacheBufReinit(PVBOXMP_CRSHGSMITRANSPORT pCon, PVBOXMP_CRSHGSMICON_BUFDR_CACHE pCache,
                                           PVBOXMP_CRSHGSMICON_BUFDR pDr, uint32_t cbRequested)
{
    RT_NOREF(pCache);
    if (pDr->cbBuf >= cbRequested)
        return VINF_SUCCESS;

    if (pDr->pvBuf)
        VBoxMpCrShgsmiTransportBufFree(pCon, pDr->pvBuf);

    pDr->pvBuf = VBoxMpCrShgsmiTransportBufAlloc(pCon, cbRequested);
    if (!pDr->pvBuf)
    {
        WARN(("VBoxMpCrShgsmiTransportBufAlloc failed"));
        pDr->cbBuf = 0;
        return VERR_NO_MEMORY;
    }

    pDr->cbBuf = cbRequested;
    return VINF_SUCCESS;
}

static void vboxMpCrShgsmiBufCacheFree(PVBOXMP_CRSHGSMITRANSPORT pCon, PVBOXMP_CRSHGSMICON_BUFDR_CACHE pCache,
                                       PVBOXMP_CRSHGSMICON_BUFDR pDr)
{
    if (ASMAtomicCmpXchgPtr(&pCache->pBufDr, pDr, NULL))
        return;

    /* the value is already cached, free the current one */
    VBoxMpCrShgsmiTransportBufFree(pCon, pDr->pvBuf);
    vboxWddmMemFree(pDr);
}

static PVBOXMP_CRSHGSMICON_BUFDR vboxMpCrShgsmiBufCacheGetAllocDr(PVBOXMP_CRSHGSMICON_BUFDR_CACHE pCache)
{
    PVBOXMP_CRSHGSMICON_BUFDR pBufDr = (PVBOXMP_CRSHGSMICON_BUFDR)ASMAtomicXchgPtr((void * volatile *)&pCache->pBufDr, NULL);
    if (!pBufDr)
    {
        pBufDr = (PVBOXMP_CRSHGSMICON_BUFDR)vboxWddmMemAllocZero(sizeof (*pBufDr));
        if (!pBufDr)
        {
            WARN(("vboxWddmMemAllocZero failed!"));
            return NULL;
        }
    }
    return pBufDr;
}

static PVBOXMP_CRSHGSMICON_BUFDR vboxMpCrShgsmiBufCacheAlloc(PVBOXMP_CRSHGSMITRANSPORT pCon,
                                                             PVBOXMP_CRSHGSMICON_BUFDR_CACHE pCache, uint32_t cbBuffer)
{
    PVBOXMP_CRSHGSMICON_BUFDR pBufDr = vboxMpCrShgsmiBufCacheGetAllocDr(pCache);
    int rc = vboxMpCrShgsmiBufCacheBufReinit(pCon, pCache, pBufDr, cbBuffer);
    if (RT_SUCCESS(rc))
        return pBufDr;

    WARN(("vboxMpCrShgsmiBufCacheBufReinit failed, rc %d", rc));

    vboxMpCrShgsmiBufCacheFree(pCon, pCache, pBufDr);
    return NULL;
}

static PVBOXMP_CRSHGSMICON_BUFDR vboxMpCrShgsmiBufCacheAllocAny(PVBOXMP_CRSHGSMITRANSPORT pCon,
                                                                PVBOXMP_CRSHGSMICON_BUFDR_CACHE pCache, uint32_t cbBuffer)
{
    PVBOXMP_CRSHGSMICON_BUFDR pBufDr = vboxMpCrShgsmiBufCacheGetAllocDr(pCache);

    if (pBufDr->cbBuf)
        return pBufDr;

    int rc = vboxMpCrShgsmiBufCacheBufReinit(pCon, pCache, pBufDr, cbBuffer);
    if (RT_SUCCESS(rc))
        return pBufDr;

    WARN(("vboxMpCrShgsmiBufCacheBufReinit failed, rc %d", rc));

    vboxMpCrShgsmiBufCacheFree(pCon, pCache, pBufDr);
    return NULL;
}


static int vboxMpCrShgsmiBufCacheInit(PVBOXMP_CRSHGSMITRANSPORT pCon, PVBOXMP_CRSHGSMICON_BUFDR_CACHE pCache)
{
    RT_NOREF(pCon);
    memset(pCache, 0, sizeof (*pCache));
    return VINF_SUCCESS;
}

static void vboxMpCrShgsmiBufCacheTerm(PVBOXMP_CRSHGSMITRANSPORT pCon, PVBOXMP_CRSHGSMICON_BUFDR_CACHE pCache)
{
    if (pCache->pBufDr)
        vboxMpCrShgsmiBufCacheFree(pCon, pCache, pCache->pBufDr);
}

int VBoxMpCrShgsmiTransportCreate(PVBOXMP_CRSHGSMITRANSPORT pCon, PVBOXMP_DEVEXT pDevExt)
{
    memset(pCon, 0, sizeof (*pCon));
    pCon->pDevExt = pDevExt;
    return VINF_SUCCESS;
#if 0 /** @todo should this be unreachable? */
    int rc;
//    int rc = vboxMpCrShgsmiBufCacheInit(pCon, &pCon->CmdDrCache);
//    if (RT_SUCCESS(rc))
    {
        rc = vboxMpCrShgsmiBufCacheInit(pCon, &pCon->WbDrCache);
        if (RT_SUCCESS(rc))
        {
        }
        else
        {
            WARN(("vboxMpCrShgsmiBufCacheInit2 failed rc %d", rc));
        }
//        vboxMpCrShgsmiBufCacheTerm(pCon, &pCon->CmdDrCache);
    }
//    else
//    {
//        WARN(("vboxMpCrShgsmiBufCacheInit1 failed rc %d", rc));
//    }

    return rc;
#endif
}

void VBoxMpCrShgsmiTransportTerm(PVBOXMP_CRSHGSMITRANSPORT pCon)
{
    vboxMpCrShgsmiBufCacheTerm(pCon, &pCon->WbDrCache);
//    vboxMpCrShgsmiBufCacheTerm(pCon, &pCon->CmdDrCache);
}

typedef struct VBOXMP_CRHGSMICMD_BASE
{
//    VBOXMP_CRHGSMICMD_HDR Hdr;
    CRVBOXHGSMIHDR CmdHdr;
} VBOXMP_CRHGSMICMD_BASE, *PVBOXMP_CRHGSMICMD_BASE;

typedef struct VBOXMP_CRHGSMICMD_WRITEREAD
{
//    VBOXMP_CRHGSMICMD_HDR Hdr;
    CRVBOXHGSMIWRITEREAD Cmd;
} VBOXMP_CRHGSMICMD_WRITEREAD, *PVBOXMP_CRHGSMICMD_WRITEREAD;

typedef struct VBOXMP_CRHGSMICMD_READ
{
//    VBOXMP_CRHGSMICMD_HDR Hdr;
    CRVBOXHGSMIREAD Cmd;
} VBOXMP_CRHGSMICMD_READ, *PVBOXMP_CRHGSMICMD_READ;

typedef struct VBOXMP_CRHGSMICMD_WRITE
{
//    VBOXMP_CRHGSMICMD_HDR Hdr;
    CRVBOXHGSMIWRITE Cmd;
} VBOXMP_CRHGSMICMD_WRITE, *PVBOXMP_CRHGSMICMD_WRITE;


#define VBOXMP_CRSHGSMICON_CMD_CMDBUF_OFFSET(_cBuffers) \
    VBOXWDDM_ROUNDBOUND(RT_OFFSETOF(VBOXVDMACMD_CHROMIUM_CMD, aBuffers[_cBuffers]), 8)
#define VBOXMP_CRSHGSMICON_CMD_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf)  \
    ( VBOXMP_CRSHGSMICON_CMD_CMDBUF_OFFSET(_cBuffers) + VBOXWDDM_ROUNDBOUND(_cbCmdBuf, 8))
#define VBOXMP_CRSHGSMICON_CMD_GET_CMDBUF(_pCmd, _cBuffers, _type)  \
    ((_type*)(((uint8_t*)(_pCmd)) + VBOXMP_CRSHGSMICON_CMD_CMDBUF_OFFSET(_cBuffers)))
#define VBOXMP_CRSHGSMICON_CMD_GET_CMDCTX(_pCmd, _cBuffers, _cbCmdBuf, _type)  \
    ((_type RT_UNTRUSTED_VOLATILE_HOST *)(((uint8_t RT_UNTRUSTED_VOLATILE_HOST *)(_pCmd)) +  VBOXMP_CRSHGSMICON_CMD_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf)))
#define VBOXMP_CRSHGSMICON_CMD_GET_FROM_CMDCTX(_pCtx, _cBuffers, _cbCmdBuf, _type)  \
    ((_type RT_UNTRUSTED_VOLATILE_HOST  *)(((uint8_t RT_UNTRUSTED_VOLATILE_HOST  *)(_pCtx)) -  VBOXMP_CRSHGSMICON_CMD_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf)))
#define VBOXMP_CRSHGSMICON_CMD_SIZE(_cBuffers, _cbCmdBuf, _cbCtx)  \
    (VBOXMP_CRSHGSMICON_CMD_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf) + (_cbCtx))


#define VBOXMP_CRSHGSMICON_DR_CMDBUF_OFFSET(_cBuffers)  \
    VBOXWDDM_ROUNDBOUND((VBOXVDMACMD_SIZE_FROMBODYSIZE(RT_OFFSETOF(VBOXVDMACMD_CHROMIUM_CMD, aBuffers[_cBuffers]))), 8)
#define VBOXMP_CRSHGSMICON_DR_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf)  \
    ( VBOXMP_CRSHGSMICON_DR_CMDBUF_OFFSET(_cBuffers) + VBOXWDDM_ROUNDBOUND(_cbCmdBuf, 8))
#define VBOXMP_CRSHGSMICON_DR_GET_CRCMD(_pDr)  \
    (VBOXVDMACMD_BODY((_pDr), VBOXVDMACMD_CHROMIUM_CMD))
#define VBOXMP_CRSHGSMICON_DR_GET_CMDBUF(_pDr, _cBuffers, _type)  \
    ((_type RT_UNTRUSTED_VOLATILE_HOST *)(((uint8_t RT_UNTRUSTED_VOLATILE_HOST *)(_pDr)) + VBOXMP_CRSHGSMICON_DR_CMDBUF_OFFSET(_cBuffers)))
#define VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(_pDr, _cBuffers, _cbCmdBuf, _type)  \
    ((_type RT_UNTRUSTED_VOLATILE_HOST *)(((uint8_t RT_UNTRUSTED_VOLATILE_HOST *)(_pDr)) +  VBOXMP_CRSHGSMICON_DR_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf)))
#define VBOXMP_CRSHGSMICON_DR_GET_FROM_CMDCTX(_pCtx, _cBuffers, _cbCmdBuf)  \
    ((VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *)(((uint8_t RT_UNTRUSTED_VOLATILE_HOST *)(_pCtx)) -  VBOXMP_CRSHGSMICON_DR_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf)))
#define VBOXMP_CRSHGSMICON_DR_SIZE(_cBuffers, _cbCmdBuf, _cbCtx)  \
    (VBOXMP_CRSHGSMICON_DR_CMDCTX_OFFSET(_cBuffers, _cbCmdBuf) + (_cbCtx))


static int vboxMpCrShgsmiTransportCmdSubmitDr(PVBOXMP_CRSHGSMITRANSPORT pCon, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr,
                                              PFNVBOXVDMADDICMDCOMPLETE_DPC pfnComplete)
{

    PVBOXVDMADDI_CMD pDdiCmd = VBOXVDMADDI_CMD_FROM_BUF_DR(pDr);
    PVBOXMP_DEVEXT pDevExt = pCon->pDevExt;
    vboxVdmaDdiCmdInit(pDdiCmd, 0, 0, pfnComplete, pCon);
    /* mark command as submitted & invisible for the dx runtime since dx did not originate it */
    vboxVdmaDdiCmdSubmittedNotDx(pDdiCmd);
    int rc = vboxVdmaCBufDrSubmit(pDevExt, &pDevExt->u.primary.Vdma, pDr);
    if (RT_SUCCESS(rc))
    {
        return VINF_SUCCESS;
    }

    WARN(("vboxVdmaCBufDrSubmit failed rc %d", rc));
    return rc;
}

static int vboxMpCrShgsmiTransportCmdSubmitDmaCmd(PVBOXMP_CRSHGSMITRANSPORT pCon, VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr,
                                                  PFNVBOXVDMADDICMDCOMPLETE_DPC pfnComplete)
{
    VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr = VBOXVDMACBUF_DR_FROM_TAIL(pHdr);
    return vboxMpCrShgsmiTransportCmdSubmitDr(pCon, pDr, pfnComplete);
}

static void vboxMpCrShgsmiTransportCmdTermDmaCmd(PVBOXMP_CRSHGSMITRANSPORT pCon, VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr)
{
    VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr = VBOXVDMACBUF_DR_FROM_TAIL(pHdr);
    PVBOXMP_DEVEXT pDevExt = pCon->pDevExt;
    vboxVdmaCBufDrFree(&pDevExt->u.primary.Vdma, pDr);
}


typedef DECLCALLBACK(void) FNVBOXMP_CRSHGSMITRANSPORT_SENDREADASYNC_COMPLETION(PVBOXMP_CRSHGSMITRANSPORT pCon, int rc,
                                                                               void RT_UNTRUSTED_VOLATILE_HOST *pvRx,
                                                                               uint32_t cbRx,
                                                                               void RT_UNTRUSTED_VOLATILE_HOST *pvCtx);
typedef FNVBOXMP_CRSHGSMITRANSPORT_SENDREADASYNC_COMPLETION *PFNVBOXMP_CRSHGSMITRANSPORT_SENDREADASYNC_COMPLETION;

static DECLCALLBACK(VOID) vboxMpCrShgsmiTransportSendReadAsyncCompletion(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pDdiCmd,
                                                                         PVOID pvContext)
{
    RT_NOREF(pDevExt);
    /* we should be called from our DPC routine */
    Assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    PVBOXMP_CRSHGSMITRANSPORT                               pCon  = (PVBOXMP_CRSHGSMITRANSPORT)pvContext;
    PVBOXVDMACBUF_DR                                        pDr   = VBOXVDMACBUF_DR_FROM_DDI_CMD(pDdiCmd);
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST                 *pHdr  = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
    VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_HOST    *pBody = VBOXMP_CRSHGSMICON_DR_GET_CRCMD(pHdr);
    const UINT                                              cBuffers = 2;
    Assert(pBody->cBuffers == cBuffers);

    VBOXMP_CRHGSMICMD_READ RT_UNTRUSTED_VOLATILE_HOST      *pWrData = VBOXMP_CRSHGSMICON_DR_GET_CMDBUF(pHdr, cBuffers, VBOXMP_CRHGSMICMD_READ);
    CRVBOXHGSMIREAD RT_UNTRUSTED_VOLATILE_HOST             *pCmd    = &pWrData->Cmd;
    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pBufCmd = &pBody->aBuffers[0];
    Assert(pBufCmd->cbBuffer == sizeof (CRVBOXHGSMIREAD));

    CRVBOXHGSMIREAD RT_UNTRUSTED_VOLATILE_HOST             *pWr = (CRVBOXHGSMIREAD*)vboxMpCrShgsmiTransportBufFromOffset(pCon, pBufCmd->offBuffer);
    PFNVBOXMP_CRSHGSMITRANSPORT_SENDREADASYNC_COMPLETION    pfnCompletion = (PFNVBOXMP_CRSHGSMITRANSPORT_SENDREADASYNC_COMPLETION)pBufCmd->u64GuestData;
    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pRxBuf = &pBody->aBuffers[1];
    VBOXMP_CRSHGSMICON_BUFDR                               *pWbDr = (PVBOXMP_CRSHGSMICON_BUFDR)pRxBuf->u64GuestData;

    void RT_UNTRUSTED_VOLATILE_HOST                        *pvRx = NULL;
    uint32_t                                                cbRx = 0;

    int rc = pDr->rc;
    if (RT_SUCCESS(rc))
    {
        rc = pWr->hdr.result;
        if (RT_SUCCESS(rc))
        {
            cbRx = pCmd->cbBuffer;
            if (cbRx)
                pvRx = pWbDr->pvBuf;
        }
        else
        {
            WARN(("CRVBOXHGSMIREAD failed, rc %d", rc));
        }
    }
    else
    {
        WARN(("dma command buffer failed rc %d!", rc));
    }

    if (pfnCompletion)
    {
        void RT_UNTRUSTED_VOLATILE_HOST *pvCtx = VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(pHdr, cBuffers,
                                                                                  sizeof(VBOXMP_CRHGSMICMD_READ), void);
        pfnCompletion(pCon, rc, pvRx, cbRx, pvCtx);
    }

    vboxMpCrShgsmiBufCacheFree(pCon, &pCon->WbDrCache, pWbDr);
}

static void RT_UNTRUSTED_VOLATILE_HOST *
vboxMpCrShgsmiTransportCmdCreateReadAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, uint32_t u32ClientID, PVBOXVDMACBUF_DR pDr,
                                          uint32_t cbDrData, PVBOXMP_CRSHGSMICON_BUFDR pWbDr,
                                          PFNVBOXMP_CRSHGSMITRANSPORT_SENDREADASYNC_COMPLETION pfnCompletion,
                                          uint32_t cbContextData)
{
    RT_NOREF(cbDrData);
    const uint32_t cBuffers = 2;
    const uint32_t cbCmd = VBOXMP_CRSHGSMICON_DR_SIZE(cBuffers, sizeof (VBOXMP_CRHGSMICMD_READ), cbContextData);
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST              *pHdr    = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
    VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_HOST *pBody   = VBOXMP_CRSHGSMICON_DR_GET_CRCMD(pHdr);
    VBOXMP_CRHGSMICMD_READ RT_UNTRUSTED_VOLATILE_HOST   *pWrData = VBOXMP_CRSHGSMICON_DR_GET_CMDBUF(pHdr, cBuffers, VBOXMP_CRHGSMICMD_READ);
    CRVBOXHGSMIREAD RT_UNTRUSTED_VOLATILE_HOST          *pCmd    = &pWrData->Cmd;

    if (cbCmd > cbContextData)
    {
        ERR(("the passed descriptor is less than needed!"));
        return NULL;
    }

    memset(pDr, 0, VBOXVDMACBUF_DR_SIZE(cbCmd));

    pDr->fFlags = VBOXVDMACBUF_FLAG_BUF_VRAM_OFFSET;
    pDr->cbBuf = cbCmd;
    pDr->rc = VERR_NOT_IMPLEMENTED;
    pDr->Location.offVramBuf = vboxMpCrShgsmiTransportBufOffset(pCon, pCmd);

    pHdr->enmType = VBOXVDMACMD_TYPE_CHROMIUM_CMD;
    pHdr->u32CmdSpecific = 0;

    pBody->cBuffers = cBuffers;

    pCmd->hdr.result      = VERR_WRONG_ORDER;
    pCmd->hdr.u32ClientID = u32ClientID;
    pCmd->hdr.u32Function = SHCRGL_GUEST_FN_WRITE_READ;
    //    pCmd->hdr.u32Reserved = 0;
    pCmd->iBuffer = 1;

    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pBufCmd = &pBody->aBuffers[0];
    pBufCmd->offBuffer = vboxMpCrShgsmiTransportBufOffset(pCon, pCmd);
    pBufCmd->cbBuffer = sizeof (*pCmd);
    pBufCmd->u32GuestData = 0;
    pBufCmd->u64GuestData = (uintptr_t)pfnCompletion;

    pBufCmd = &pBody->aBuffers[1];
    pBufCmd->offBuffer = vboxMpCrShgsmiTransportBufOffset(pCon, pWbDr->pvBuf);
    pBufCmd->cbBuffer = pWbDr->cbBuf;
    pBufCmd->u32GuestData = 0;
    pBufCmd->u64GuestData = (uintptr_t)pWbDr;

    return VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(pHdr, cBuffers, sizeof(VBOXMP_CRHGSMICMD_READ), void);
}

static int vboxMpCrShgsmiTransportCmdSubmitReadAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, void *pvContext)
{
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXMP_CRSHGSMICON_DR_GET_FROM_CMDCTX(pvContext, 2, sizeof(VBOXMP_CRHGSMICMD_READ));
    return vboxMpCrShgsmiTransportCmdSubmitDmaCmd(pCon, pHdr, vboxMpCrShgsmiTransportSendReadAsyncCompletion);
}

typedef struct VBOXMP_CRHGSMICON_WRR_COMPLETION_CTX
{
    PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEREADASYNC_COMPLETION pfnCompletion;
    void RT_UNTRUSTED_VOLATILE_HOST *pvContext;

} VBOXMP_CRHGSMICON_WRR_COMPLETION_CTX, *PVBOXMP_CRHGSMICON_WRR_COMPLETION_CTX;

/** @callback_method_impl{FNVBOXMP_CRSHGSMITRANSPORT_SENDREADASYNC_COMPLETION} */
static DECLCALLBACK(void) vboxMpCrShgsmiTransportSendWriteReadReadRepostCompletion(PVBOXMP_CRSHGSMITRANSPORT pCon, int rc,
                                                                                   void RT_UNTRUSTED_VOLATILE_HOST *pvRx,
                                                                                   uint32_t cbRx,
                                                                                   void RT_UNTRUSTED_VOLATILE_HOST *pvCtx)
{
    PVBOXMP_CRHGSMICON_WRR_COMPLETION_CTX pData = (PVBOXMP_CRHGSMICON_WRR_COMPLETION_CTX)pvCtx;
    PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEREADASYNC_COMPLETION pfnCompletion = pData->pfnCompletion;
    if (pfnCompletion)
        pfnCompletion(pCon, rc, pvRx, cbRx, pData->pvContext);
}

static DECLCALLBACK(VOID) vboxMpCrShgsmiTransportSendWriteReadAsyncCompletion(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pDdiCmd,
                                                                              PVOID pvContext)
{
    RT_NOREF(pDevExt);
    /* we should be called from our DPC routine */
    Assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    PVBOXMP_CRSHGSMITRANSPORT                               pCon = (PVBOXMP_CRSHGSMITRANSPORT)pvContext;
    PVBOXVDMACBUF_DR                                        pDr = VBOXVDMACBUF_DR_FROM_DDI_CMD(pDdiCmd);
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST                 *pHdr = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
    VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_HOST    *pBody = VBOXMP_CRSHGSMICON_DR_GET_CRCMD(pHdr);
    const UINT cBuffers = 3;
    Assert(pBody->cBuffers == cBuffers);

    VBOXMP_CRHGSMICMD_WRITEREAD RT_UNTRUSTED_VOLATILE_HOST *pWrData = VBOXMP_CRSHGSMICON_DR_GET_CMDBUF(pHdr, cBuffers, VBOXMP_CRHGSMICMD_WRITEREAD);
    CRVBOXHGSMIWRITEREAD RT_UNTRUSTED_VOLATILE_HOST        *pCmd = &pWrData->Cmd;
    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pBufCmd = &pBody->aBuffers[0];
    Assert(pBufCmd->cbBuffer == sizeof(CRVBOXHGSMIWRITEREAD));

    CRVBOXHGSMIWRITEREAD RT_UNTRUSTED_VOLATILE_HOST        *pWr = (CRVBOXHGSMIWRITEREAD*)vboxMpCrShgsmiTransportBufFromOffset(pCon, pBufCmd->offBuffer);
    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pRxBuf = &pBody->aBuffers[2];
    VBOXMP_CRSHGSMICON_BUFDR                               *pWbDr = (PVBOXMP_CRSHGSMICON_BUFDR)pRxBuf->u64GuestData;
    PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEREADASYNC_COMPLETION pfnCompletion = (PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEREADASYNC_COMPLETION)pBufCmd->u64GuestData;

    void RT_UNTRUSTED_VOLATILE_HOST                        *pvRx = NULL;
    uint32_t                                                cbRx = 0;

    int rc = pDr->rc;
    if (RT_SUCCESS(rc))
    {
        rc = pWr->hdr.result;
        if (RT_SUCCESS(rc))
        {
            cbRx = pCmd->cbWriteback;
            if (cbRx)
                pvRx = pWbDr->pvBuf;
        }
        else if (rc == VERR_BUFFER_OVERFLOW)
        {
            /* issue read */
            void RT_UNTRUSTED_VOLATILE_HOST *pvCtx = VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(pHdr, cBuffers,
                                                                                      sizeof(VBOXMP_CRHGSMICMD_WRITEREAD), void);
            vboxMpCrShgsmiBufCacheFree(pCon, &pCon->WbDrCache, pWbDr);
            pWbDr = vboxMpCrShgsmiBufCacheAlloc(pCon, &pCon->WbDrCache, pCmd->cbWriteback);
            if (pWbDr)
            {
                /* the Read Command is shorter than WriteRead, so just reuse the Write-Read descriptor here */
                PVBOXMP_CRHGSMICON_WRR_COMPLETION_CTX pReadCtx;
                pReadCtx = (PVBOXMP_CRHGSMICON_WRR_COMPLETION_CTX)vboxMpCrShgsmiTransportCmdCreateReadAsync(pCon,
                                                    pCmd->hdr.u32ClientID,
                                                    pDr,
                                                    VBOXMP_CRSHGSMICON_DR_SIZE(cBuffers, sizeof(VBOXMP_CRHGSMICMD_WRITEREAD), 0),
                                                    pWbDr,
                                                    vboxMpCrShgsmiTransportSendWriteReadReadRepostCompletion,
                                                    sizeof(*pReadCtx));
                pReadCtx->pfnCompletion = pfnCompletion;
                pReadCtx->pvContext = pvCtx;
                vboxMpCrShgsmiTransportCmdSubmitReadAsync(pCon, pReadCtx);
                /* don't do completion here, the completion will be called from the read completion we issue here */
                pfnCompletion = NULL;
                /* the current pWbDr was already freed, and we'll free the Read dr in the Read Completion */
                pWbDr = NULL;
            }
            else
            {
                WARN(("vboxMpCrShgsmiBufCacheAlloc failed for %d", pCmd->cbWriteback));
                rc = VERR_NO_MEMORY;
            }
        }
        else
        {
            WARN(("CRVBOXHGSMIWRITEREAD failed, rc %d", rc));
        }
    }
    else
    {
        WARN(("dma command buffer failed rc %d!", rc));
    }

    if (pfnCompletion)
    {
        void RT_UNTRUSTED_VOLATILE_HOST *pvCtx = VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(pHdr, cBuffers,
                                                                                  sizeof(VBOXMP_CRHGSMICMD_WRITEREAD), void);
        pfnCompletion(pCon, rc, pvRx, cbRx, pvCtx);
    }

    if (pWbDr)
        vboxMpCrShgsmiBufCacheFree(pCon, &pCon->WbDrCache, pWbDr);
}

static DECLCALLBACK(VOID) vboxMpCrShgsmiTransportVdmaSendWriteAsyncCompletion(PVBOXMP_DEVEXT pDevExt, PVBOXVDMADDI_CMD pDdiCmd,
                                                                              PVOID pvContext)
{
    RT_NOREF(pDevExt);
    /* we should be called from our DPC routine */
    Assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    PVBOXMP_CRSHGSMITRANSPORT                               pCon = (PVBOXMP_CRSHGSMITRANSPORT)pvContext;
    PVBOXVDMACBUF_DR                                        pDr = VBOXVDMACBUF_DR_FROM_DDI_CMD(pDdiCmd);
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST                 *pHdr = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
    VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_HOST    *pBody = VBOXMP_CRSHGSMICON_DR_GET_CRCMD(pHdr);
    const UINT                                              cBuffers = 2;
    Assert(pBody->cBuffers == cBuffers);

    VBOXMP_CRHGSMICMD_WRITE RT_UNTRUSTED_VOLATILE_HOST     *pWrData = VBOXMP_CRSHGSMICON_DR_GET_CMDBUF(pHdr, cBuffers, VBOXMP_CRHGSMICMD_WRITE);
    CRVBOXHGSMIWRITE RT_UNTRUSTED_VOLATILE_HOST            *pCmd = &pWrData->Cmd;
    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pBufCmd = &pBody->aBuffers[0];
    Assert(pBufCmd->cbBuffer == sizeof (CRVBOXHGSMIWRITE));
    PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEASYNC_COMPLETION pfnCompletion = (PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEASYNC_COMPLETION)pBufCmd->u64GuestData;

    int rc = pDr->rc;
    if (RT_SUCCESS(rc))
    {
        rc = pCmd->hdr.result;
        if (!RT_SUCCESS(rc))
        {
            WARN(("CRVBOXHGSMIWRITE failed, rc %d", rc));
        }
    }
    else
    {
        WARN(("dma command buffer failed rc %d!", rc));
    }

    if (pfnCompletion)
    {
        void RT_UNTRUSTED_VOLATILE_HOST *pvCtx = VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(pHdr, cBuffers, sizeof(VBOXMP_CRHGSMICMD_WRITE), void);
        pfnCompletion(pCon, rc, pvCtx);
    }
}

/** @callback_method_impl{FNVBOXSHGSMICMDCOMPLETION}   */
static DECLCALLBACK(VOID)
vboxMpCrShgsmiTransportVbvaSendWriteAsyncCompletion(PVBOXSHGSMI pHeap, void RT_UNTRUSTED_VOLATILE_HOST *pvCmd, void *pvContext)
{
    RT_NOREF(pHeap);
    /* we should be called from our DPC routine */
    Assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    PVBOXMP_CRSHGSMITRANSPORT pCon = (PVBOXMP_CRSHGSMITRANSPORT)pvContext;
    VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST *pCmd = NULL;
    int rc = VBoxCmdVbvaConCmdCompletionData(pvCmd, &pCmd);
    const UINT cBuffers = 2;
    Assert(pCmd->cBuffers == cBuffers);
    uint64_t RT_UNTRUSTED_VOLATILE_HOST *pu64Completion = VBOXMP_CRSHGSMICON_CMD_GET_CMDCTX(pCmd, cBuffers,
                                                                                            sizeof(VBOXMP_CRHGSMICMD_WRITE),
                                                                                            uint64_t);
    PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEASYNC_COMPLETION pfnCompletion
        = (PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEASYNC_COMPLETION)*pu64Completion;

    if (!RT_SUCCESS(rc))
        WARN(("CRVBOXHGSMIWRITE failed, rc %d", rc));

    if (pfnCompletion)
        pfnCompletion(pCon, rc, (void RT_UNTRUSTED_VOLATILE_HOST *)(pu64Completion + 1));
}

void RT_UNTRUSTED_VOLATILE_HOST *
VBoxMpCrShgsmiTransportCmdCreateWriteReadAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, uint32_t u32ClientID, void *pvBuffer,
                                               uint32_t cbBuffer,
                                               PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEREADASYNC_COMPLETION pfnCompletion,
                                               uint32_t cbContextData)
{
    const uint32_t cBuffers = 3;
    const uint32_t cbCmd = VBOXMP_CRSHGSMICON_DR_SIZE(cBuffers, sizeof (VBOXMP_CRHGSMICMD_WRITEREAD), cbContextData);
    PVBOXMP_DEVEXT pDevExt = pCon->pDevExt;
    VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr = vboxVdmaCBufDrCreate(&pDevExt->u.primary.Vdma, cbCmd);
    if (!pDr)
    {
        WARN(("vboxVdmaCBufDrCreate failed"));
        return NULL;
    }

    PVBOXMP_CRSHGSMICON_BUFDR pWbDr = vboxMpCrShgsmiBufCacheAllocAny(pCon, &pCon->WbDrCache, 1000);
    if (!pWbDr)
    {
        WARN(("vboxMpCrShgsmiBufCacheAlloc for wb dr failed"));
        vboxVdmaCBufDrFree(&pDevExt->u.primary.Vdma, pDr);
        return NULL;
    }

    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST                 *pHdr = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
    VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_HOST    *pBody = VBOXMP_CRSHGSMICON_DR_GET_CRCMD(pHdr);
    VBOXMP_CRHGSMICMD_WRITEREAD RT_UNTRUSTED_VOLATILE_HOST *pWrData = VBOXMP_CRSHGSMICON_DR_GET_CMDBUF(pHdr, cBuffers, VBOXMP_CRHGSMICMD_WRITEREAD);
    CRVBOXHGSMIWRITEREAD RT_UNTRUSTED_VOLATILE_HOST        *pCmd = &pWrData->Cmd;

    pDr->fFlags = VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR;
    pDr->cbBuf = cbCmd;
    pDr->rc = VERR_NOT_IMPLEMENTED;
//    pDr->Location.offVramBuf = vboxMpCrShgsmiTransportBufOffset(pCon, pCmd);


    pHdr->enmType = VBOXVDMACMD_TYPE_CHROMIUM_CMD;
    pHdr->u32CmdSpecific = 0;

    pBody->cBuffers = cBuffers;

    pCmd->hdr.result      = VERR_WRONG_ORDER;
    pCmd->hdr.u32ClientID = u32ClientID;
    pCmd->hdr.u32Function = SHCRGL_GUEST_FN_WRITE_READ;
    //    pCmd->hdr.u32Reserved = 0;
    pCmd->iBuffer = 1;
    pCmd->iWriteback = 2;
    pCmd->cbWriteback = 0;

    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pBufCmd = &pBody->aBuffers[0];
    pBufCmd->offBuffer = vboxVdmaCBufDrPtrOffset(&pDevExt->u.primary.Vdma, pCmd);
    pBufCmd->cbBuffer = sizeof (*pCmd);
    pBufCmd->u32GuestData = 0;
    pBufCmd->u64GuestData = (uintptr_t)pfnCompletion;

    pBufCmd = &pBody->aBuffers[1];
    pBufCmd->offBuffer = vboxMpCrShgsmiTransportBufOffset(pCon, pvBuffer);
    pBufCmd->cbBuffer = cbBuffer;
    pBufCmd->u32GuestData = 0;
    pBufCmd->u64GuestData = 0;

    pBufCmd = &pBody->aBuffers[2];
    pBufCmd->offBuffer = vboxMpCrShgsmiTransportBufOffset(pCon, pWbDr->pvBuf);
    pBufCmd->cbBuffer = pWbDr->cbBuf;
    pBufCmd->u32GuestData = 0;
    pBufCmd->u64GuestData = (uintptr_t)pWbDr;

    return VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(pHdr, cBuffers, sizeof(VBOXMP_CRHGSMICMD_WRITEREAD), void);
}

static void RT_UNTRUSTED_VOLATILE_HOST *
vboxMpCrShgsmiTransportCmdVbvaCreateWriteAsync(PVBOXMP_DEVEXT pDevExt, uint32_t u32ClientID, void *pvBuffer,
                                               uint32_t cbBuffer,
                                               PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEASYNC_COMPLETION pfnCompletion,
                                               uint32_t cbContextData)
{
    const uint32_t cBuffers = 2;
    const uint32_t cbCmd = VBOXMP_CRSHGSMICON_CMD_SIZE(cBuffers, sizeof (VBOXMP_CRHGSMICMD_WRITE) + 8, cbContextData);
    VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST *pCmd = VBoxCmdVbvaConCmdAlloc(pDevExt, cbCmd);
    if (!pCmd)
    {
        WARN(("VBoxCmdVbvaConCmdAlloc failed"));
        return NULL;
    }

    pCmd->cBuffers = cBuffers;

    VBOXMP_CRHGSMICMD_WRITE RT_UNTRUSTED_VOLATILE_HOST *pWrData
        = VBOXMP_CRSHGSMICON_CMD_GET_CMDBUF(pCmd, cBuffers, VBOXMP_CRHGSMICMD_WRITE);
    CRVBOXHGSMIWRITE RT_UNTRUSTED_VOLATILE_HOST *pCmdWrite = &pWrData->Cmd;

    pCmdWrite->hdr.result      = VERR_WRONG_ORDER;
    pCmdWrite->hdr.u32ClientID = u32ClientID;
    pCmdWrite->hdr.u32Function = SHCRGL_GUEST_FN_WRITE;
    //    pCmdWrite->hdr.u32Reserved = 0;
    pCmdWrite->iBuffer = 1;

    VBOXCMDVBVA_CRCMD_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pBufCmd = &pCmd->aBuffers[0];
    pBufCmd->offBuffer = (VBOXCMDVBVAOFFSET)vboxMpCrShgsmiBufferOffset(pDevExt, pCmdWrite);
    pBufCmd->cbBuffer = sizeof (*pCmdWrite);

    pBufCmd = &pCmd->aBuffers[1];
    pBufCmd->offBuffer = (VBOXCMDVBVAOFFSET)vboxMpCrShgsmiBufferOffset(pDevExt, pvBuffer);
    pBufCmd->cbBuffer = cbBuffer;

    uint64_t RT_UNTRUSTED_VOLATILE_HOST *pu64Completion = VBOXMP_CRSHGSMICON_CMD_GET_CMDCTX(pCmd, cBuffers,
                                                                                            sizeof(VBOXMP_CRHGSMICMD_WRITE),
                                                                                            uint64_t);
    *pu64Completion = (uintptr_t)pfnCompletion;
    return pu64Completion + 1;
}

void RT_UNTRUSTED_VOLATILE_HOST *
vboxMpCrShgsmiTransportCmdVdmaCreateWriteAsync(PVBOXMP_DEVEXT pDevExt, uint32_t u32ClientID, void *pvBuffer,
                                               uint32_t cbBuffer,
                                               PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEASYNC_COMPLETION pfnCompletion,
                                               uint32_t cbContextData)
{
    const uint32_t cBuffers = 2;
    const uint32_t cbCmd = VBOXMP_CRSHGSMICON_DR_SIZE(cBuffers, sizeof (VBOXMP_CRHGSMICMD_WRITE), cbContextData);
    VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_HOST *pDr = vboxVdmaCBufDrCreate(&pDevExt->u.primary.Vdma, cbCmd);
    if (!pDr)
    {
        WARN(("vboxVdmaCBufDrCreate failed"));
        return NULL;
    }

    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST              *pHdr = VBOXVDMACBUF_DR_TAIL(pDr, VBOXVDMACMD);
    VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_HOST *pBody = VBOXMP_CRSHGSMICON_DR_GET_CRCMD(pHdr);
    VBOXMP_CRHGSMICMD_WRITE RT_UNTRUSTED_VOLATILE_HOST  *pWrData = VBOXMP_CRSHGSMICON_DR_GET_CMDBUF(pHdr, cBuffers, VBOXMP_CRHGSMICMD_WRITE);
    CRVBOXHGSMIWRITE RT_UNTRUSTED_VOLATILE_HOST         *pCmd = &pWrData->Cmd;

    pDr->fFlags = VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR;
    pDr->cbBuf = cbCmd;
    pDr->rc = VERR_NOT_IMPLEMENTED;
//    pDr->Location.offVramBuf = vboxMpCrShgsmiTransportBufOffset(pCon, pCmd);

    pHdr->enmType = VBOXVDMACMD_TYPE_CHROMIUM_CMD;
    pHdr->u32CmdSpecific = 0;

    pBody->cBuffers = cBuffers;

    pCmd->hdr.result      = VERR_WRONG_ORDER;
    pCmd->hdr.u32ClientID = u32ClientID;
    pCmd->hdr.u32Function = SHCRGL_GUEST_FN_WRITE;
    //    pCmd->hdr.u32Reserved = 0;
    pCmd->iBuffer = 1;

    VBOXVDMACMD_CHROMIUM_BUFFER RT_UNTRUSTED_VOLATILE_HOST *pBufCmd = &pBody->aBuffers[0];
    pBufCmd->offBuffer = vboxVdmaCBufDrPtrOffset(&pDevExt->u.primary.Vdma, pCmd);
    pBufCmd->cbBuffer = sizeof (*pCmd);
    pBufCmd->u32GuestData = 0;
    pBufCmd->u64GuestData = (uintptr_t)pfnCompletion;

    pBufCmd = &pBody->aBuffers[1];
    pBufCmd->offBuffer = vboxMpCrShgsmiBufferOffset(pDevExt, pvBuffer);
    pBufCmd->cbBuffer = cbBuffer;
    pBufCmd->u32GuestData = 0;
    pBufCmd->u64GuestData = 0;

    return VBOXMP_CRSHGSMICON_DR_GET_CMDCTX(pHdr, cBuffers, sizeof(VBOXMP_CRHGSMICMD_WRITE), void);
}

void RT_UNTRUSTED_VOLATILE_HOST *
VBoxMpCrShgsmiTransportCmdCreateWriteAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, uint32_t u32ClientID, void *pvBuffer, uint32_t cbBuffer,
                                           PFNVBOXMP_CRSHGSMITRANSPORT_SENDWRITEASYNC_COMPLETION pfnCompletion,
                                           uint32_t cbContextData)
{
    PVBOXMP_DEVEXT pDevExt = pCon->pDevExt;
    if (pDevExt->fCmdVbvaEnabled)
        return vboxMpCrShgsmiTransportCmdVbvaCreateWriteAsync(pDevExt, u32ClientID, pvBuffer, cbBuffer, pfnCompletion,
                                                              cbContextData);
    return vboxMpCrShgsmiTransportCmdVdmaCreateWriteAsync(pDevExt, u32ClientID, pvBuffer, cbBuffer, pfnCompletion, cbContextData);
}

int VBoxMpCrShgsmiTransportCmdSubmitWriteReadAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXMP_CRSHGSMICON_DR_GET_FROM_CMDCTX(pvContext, 3,
                                                                                         sizeof(VBOXMP_CRHGSMICMD_WRITEREAD));
    return vboxMpCrShgsmiTransportCmdSubmitDmaCmd(pCon, pHdr, vboxMpCrShgsmiTransportSendWriteReadAsyncCompletion);
}

static int vboxMpCrShgsmiTransportCmdVdmaSubmitWriteAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXMP_CRSHGSMICON_DR_GET_FROM_CMDCTX(pvContext, 2, sizeof(VBOXMP_CRHGSMICMD_WRITE));
    return vboxMpCrShgsmiTransportCmdSubmitDmaCmd(pCon, pHdr, vboxMpCrShgsmiTransportVdmaSendWriteAsyncCompletion);
}

static int vboxMpCrShgsmiTransportCmdVbvaSubmitWriteAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    PVBOXMP_DEVEXT pDevExt = pCon->pDevExt;
    VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST *pCmd
        = VBOXMP_CRSHGSMICON_CMD_GET_FROM_CMDCTX(pvContext, 2, sizeof (VBOXMP_CRHGSMICMD_WRITE) + 8, VBOXCMDVBVA_CRCMD_CMD);
    return VBoxCmdVbvaConCmdSubmitAsync(pDevExt, pCmd, vboxMpCrShgsmiTransportVbvaSendWriteAsyncCompletion, pCon);
}

int VBoxMpCrShgsmiTransportCmdSubmitWriteAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    if (pCon->pDevExt->fCmdVbvaEnabled)
        return vboxMpCrShgsmiTransportCmdVbvaSubmitWriteAsync(pCon, pvContext);
    return vboxMpCrShgsmiTransportCmdVdmaSubmitWriteAsync(pCon, pvContext);
}

void VBoxMpCrShgsmiTransportCmdTermWriteReadAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXMP_CRSHGSMICON_DR_GET_FROM_CMDCTX(pvContext, 3,
                                                                                         sizeof(VBOXMP_CRHGSMICMD_WRITEREAD));
    vboxMpCrShgsmiTransportCmdTermDmaCmd(pCon, pHdr);
}

static void vboxMpCrShgsmiTransportCmdVbvaTermWriteAsync(PVBOXMP_CRSHGSMITRANSPORT pCon,
                                                         void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    VBOXCMDVBVA_CRCMD_CMD RT_UNTRUSTED_VOLATILE_HOST *pCmd
        = VBOXMP_CRSHGSMICON_CMD_GET_FROM_CMDCTX(pvContext, 2, sizeof(VBOXMP_CRHGSMICMD_WRITE) + 8, VBOXCMDVBVA_CRCMD_CMD);
    VBoxCmdVbvaConCmdFree(pCon->pDevExt, pCmd);
}

static void vboxMpCrShgsmiTransportCmdVdmaTermWriteAsync(PVBOXMP_CRSHGSMITRANSPORT pCon,
                                                         void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_HOST *pHdr = VBOXMP_CRSHGSMICON_DR_GET_FROM_CMDCTX(pvContext, 2, sizeof(VBOXMP_CRHGSMICMD_WRITE));
    vboxMpCrShgsmiTransportCmdTermDmaCmd(pCon, pHdr);
}

void VBoxMpCrShgsmiTransportCmdTermWriteAsync(PVBOXMP_CRSHGSMITRANSPORT pCon, void RT_UNTRUSTED_VOLATILE_HOST *pvContext)
{
    if (pCon->pDevExt->fCmdVbvaEnabled)
        vboxMpCrShgsmiTransportCmdVbvaTermWriteAsync(pCon, pvContext);
    else
        vboxMpCrShgsmiTransportCmdVdmaTermWriteAsync(pCon, pvContext);
}

static int vboxMpCrCtlAddRef(PVBOXMP_CRCTLCON pCrCtlCon)
{
    if (pCrCtlCon->cCrCtlRefs++)
        return VINF_ALREADY_INITIALIZED;

    int rc = VbglR0CrCtlCreate(&pCrCtlCon->hCrCtl);
    if (RT_SUCCESS(rc))
    {
        Assert(pCrCtlCon->hCrCtl);
        return VINF_SUCCESS;
    }

    WARN(("vboxCrCtlCreate failed, rc (%d)", rc));

    --pCrCtlCon->cCrCtlRefs;
    return rc;
}

static int vboxMpCrCtlRelease(PVBOXMP_CRCTLCON pCrCtlCon)
{
    Assert(pCrCtlCon->cCrCtlRefs);
    if (--pCrCtlCon->cCrCtlRefs)
    {
        return VINF_SUCCESS;
    }

    int rc = VbglR0CrCtlDestroy(pCrCtlCon->hCrCtl);
    if (RT_SUCCESS(rc))
    {
        pCrCtlCon->hCrCtl = NULL;
        return VINF_SUCCESS;
    }

    WARN(("vboxCrCtlDestroy failed, rc (%d)", rc));

    ++pCrCtlCon->cCrCtlRefs;
    return rc;
}

static int vboxMpCrCtlConSetVersion(PVBOXMP_CRCTLCON pCrCtlCon, uint32_t u32ClientID, uint32_t vMajor, uint32_t vMinor)
{
    CRVBOXHGCMSETVERSION parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, u32ClientID, SHCRGL_GUEST_FN_SET_VERSION, SHCRGL_CPARMS_SET_VERSION);

    parms.vMajor.type      = VMMDevHGCMParmType_32bit;
    parms.vMajor.u.value32 = vMajor;
    parms.vMinor.type      = VMMDevHGCMParmType_32bit;
    parms.vMinor.u.value32 = vMinor;

    rc = VbglR0CrCtlConCall(pCrCtlCon->hCrCtl, &parms.hdr, sizeof (parms));
    if (RT_FAILURE(rc))
        WARN(("vboxCrCtlConCall/SET_VERSION failed, rc = %Rrc", rc));
    return rc;
}

static int vboxMpCrCtlConGetCapsLegacy(PVBOXMP_CRCTLCON pCrCtlCon, uint32_t u32ClientID, uint32_t *pu32Caps)
{
    CRVBOXHGCMGETCAPS parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, u32ClientID, SHCRGL_GUEST_FN_GET_CAPS_LEGACY, SHCRGL_CPARMS_GET_CAPS_LEGACY);

    parms.Caps.type      = VMMDevHGCMParmType_32bit;
    parms.Caps.u.value32 = 0;

    *pu32Caps = 0;

    rc = VbglR0CrCtlConCall(pCrCtlCon->hCrCtl, &parms.hdr, sizeof (parms));
    if (RT_FAILURE(rc))
    {
        WARN(("vboxCrCtlConCall/GET_CAPS_LEAGCY failed, rc=%Rrc", rc));
        return rc;
    }

    /* if host reports it supports CR_VBOX_CAP_CMDVBVA, clean it up,
     * we only support CR_VBOX_CAP_CMDVBVA of the proper version reported by SHCRGL_GUEST_FN_GET_CAPS_NEW */
    parms.Caps.u.value32 &= ~CR_VBOX_CAP_CMDVBVA;

    *pu32Caps = parms.Caps.u.value32;

    return VINF_SUCCESS;
}

static int vboxMpCrCtlConGetCapsNew(PVBOXMP_CRCTLCON pCrCtlCon, uint32_t u32ClientID, CR_CAPS_INFO *pCapsInfo)
{
    pCapsInfo->u32Caps = CR_VBOX_CAPS_ALL;
    pCapsInfo->u32CmdVbvaVersion = CR_CMDVBVA_VERSION;

    CRVBOXHGCMGETCAPS parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, u32ClientID, SHCRGL_GUEST_FN_GET_CAPS_NEW, SHCRGL_CPARMS_GET_CAPS_NEW);

    parms.Caps.type      = VMMDevHGCMParmType_LinAddr;
    parms.Caps.u.Pointer.u.linearAddr = (uintptr_t)pCapsInfo;
    parms.Caps.u.Pointer.size = sizeof (*pCapsInfo);

    rc = VbglR0CrCtlConCall(pCrCtlCon->hCrCtl, &parms.hdr, sizeof (parms));
    if (RT_FAILURE(rc))
    {
        WARN(("vboxCrCtlConCall/GET_CAPS_NEW failed, rc=%Rrc", rc));
        return rc;
    }

    if (pCapsInfo->u32CmdVbvaVersion != CR_CMDVBVA_VERSION)
    {
        WARN(("CmdVbva version mismatch (%d), expected(%d)", pCapsInfo->u32CmdVbvaVersion, CR_CMDVBVA_VERSION));
        pCapsInfo->u32Caps &= ~CR_VBOX_CAP_CMDVBVA;
    }

    pCapsInfo->u32Caps &= CR_VBOX_CAPS_ALL;

    return VINF_SUCCESS;
}

static int vboxMpCrCtlConSetPID(PVBOXMP_CRCTLCON pCrCtlCon, uint32_t u32ClientID)
{
    CRVBOXHGCMSETPID parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, u32ClientID, SHCRGL_GUEST_FN_SET_PID, SHCRGL_CPARMS_SET_PID);

    parms.u64PID.type     = VMMDevHGCMParmType_64bit;
    parms.u64PID.u.value64 = (uintptr_t)PsGetCurrentProcessId();

    Assert(parms.u64PID.u.value64);

    rc = VbglR0CrCtlConCall(pCrCtlCon->hCrCtl, &parms.hdr, sizeof (parms));
    if (RT_FAILURE(rc))
        WARN(("vboxCrCtlConCall/SET_PIDfailed, rc=%Rrc", rc));
    return rc;
}

int VBoxMpCrCtlConConnectHgcm(PVBOXMP_CRCTLCON pCrCtlCon,
        uint32_t crVersionMajor, uint32_t crVersionMinor,
        uint32_t *pu32ClientID)
{
    uint32_t u32ClientID;
    int rc = vboxMpCrCtlAddRef(pCrCtlCon);
    if (RT_SUCCESS(rc))
    {
        rc = VbglR0CrCtlConConnect(pCrCtlCon->hCrCtl, &u32ClientID);
        if (RT_SUCCESS(rc))
        {
            rc = vboxMpCrCtlConSetVersion(pCrCtlCon, u32ClientID, crVersionMajor, crVersionMinor);
            if (RT_SUCCESS(rc))
            {
                rc = vboxMpCrCtlConSetPID(pCrCtlCon, u32ClientID);
                if (RT_SUCCESS(rc))
                {
                    *pu32ClientID = u32ClientID;
                    return VINF_SUCCESS;
                }
                else
                {
                    WARN(("vboxMpCrCtlConSetPID failed, rc (%d)", rc));
                }
            }
            else
            {
                WARN(("vboxMpCrCtlConSetVersion failed, rc (%d)", rc));
            }
            VbglR0CrCtlConDisconnect(pCrCtlCon->hCrCtl, u32ClientID);
        }
        else
        {
            WARN(("vboxCrCtlConConnect failed, rc (%d)", rc));
        }
        vboxMpCrCtlRelease(pCrCtlCon);
    }
    else
    {
        WARN(("vboxMpCrCtlAddRef failed, rc (%d)", rc));
    }

    *pu32ClientID = 0;
    Assert(RT_FAILURE(rc));
    return rc;
}

int VBoxMpCrCtlConConnectVbva(PVBOXMP_DEVEXT pDevExt, PVBOXMP_CRCTLCON pCrCtlCon,
        uint32_t crVersionMajor, uint32_t crVersionMinor,
        uint32_t *pu32ClientID)
{
    if (pCrCtlCon->hCrCtl)
    {
        WARN(("pCrCtlCon is HGCM connection"));
        return VERR_INVALID_STATE;
    }

    Assert(!pCrCtlCon->cCrCtlRefs);
    return VBoxCmdVbvaConConnect(pDevExt, &pDevExt->CmdVbva,
            crVersionMajor, crVersionMinor,
            pu32ClientID);
}

int VBoxMpCrCtlConConnect(PVBOXMP_DEVEXT pDevExt, PVBOXMP_CRCTLCON pCrCtlCon,
        uint32_t crVersionMajor, uint32_t crVersionMinor,
        uint32_t *pu32ClientID)
{
    if (pDevExt->fCmdVbvaEnabled)
    {
        return VBoxMpCrCtlConConnectVbva(pDevExt, pCrCtlCon,
                crVersionMajor, crVersionMinor,
                pu32ClientID);
    }
    return VBoxMpCrCtlConConnectHgcm(pCrCtlCon,
            crVersionMajor, crVersionMinor,
            pu32ClientID);
}

int VBoxMpCrCtlConDisconnectHgcm(PVBOXMP_CRCTLCON pCrCtlCon, uint32_t u32ClientID)
{
    int rc = VbglR0CrCtlConDisconnect(pCrCtlCon->hCrCtl, u32ClientID);
    if (RT_SUCCESS(rc))
    {
        vboxMpCrCtlRelease(pCrCtlCon);
        return VINF_SUCCESS;
    }
    WARN(("vboxCrCtlConDisconnect failed, rc (%d)", rc));
    return rc;
}

int VBoxMpCrCtlConDisconnectVbva(PVBOXMP_DEVEXT pDevExt, PVBOXMP_CRCTLCON pCrCtlCon, uint32_t u32ClientID)
{
    RT_NOREF(pDevExt, pCrCtlCon);
    Assert(!pCrCtlCon->hCrCtl);
    Assert(!pCrCtlCon->cCrCtlRefs);
    return VBoxCmdVbvaConDisconnect(pDevExt, &pDevExt->CmdVbva, u32ClientID);
}

int VBoxMpCrCtlConDisconnect(PVBOXMP_DEVEXT pDevExt, PVBOXMP_CRCTLCON pCrCtlCon, uint32_t u32ClientID)
{
    if (!pCrCtlCon->hCrCtl)
        return VBoxMpCrCtlConDisconnectVbva(pDevExt, pCrCtlCon, u32ClientID);
    return VBoxMpCrCtlConDisconnectHgcm(pCrCtlCon, u32ClientID);
}

int VBoxMpCrCtlConCall(PVBOXMP_CRCTLCON pCrCtlCon, PVBGLIOCHGCMCALL pData, uint32_t cbData)
{
    int rc = VbglR0CrCtlConCallRaw(pCrCtlCon->hCrCtl, pData, cbData);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vboxCrCtlConCallUserData failed, rc(%d)", rc));
    return rc;
}

int VBoxMpCrCtlConCallUserData(PVBOXMP_CRCTLCON pCrCtlCon, PVBGLIOCHGCMCALL pData, uint32_t cbData)
{
    int rc = VbglR0CrCtlConCallUserDataRaw(pCrCtlCon->hCrCtl, pData, cbData);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vboxCrCtlConCallUserData failed, rc(%d)", rc));
    return rc;
}

void VBoxMpCrCtlConInit(void)
{
    g_VBoxMpCr3DSupported = 0;
    memset(&g_VBoxMpCrHostCapsInfo, 0, sizeof (g_VBoxMpCrHostCapsInfo));

    VBOXMP_CRCTLCON CrCtlCon = {0};
    uint32_t u32ClientID = 0;
    int rc = VBoxMpCrCtlConConnectHgcm(&CrCtlCon, CR_PROTOCOL_VERSION_MAJOR, CR_PROTOCOL_VERSION_MINOR, &u32ClientID);
    if (RT_FAILURE(rc))
    {
        LOGREL(("VBoxMpCrCtlConConnectHgcm failed with rc(%d), 3D not supported!", rc));
        return;
    }

    rc = vboxMpCrCtlConGetCapsNew(&CrCtlCon, u32ClientID, &g_VBoxMpCrHostCapsInfo);
    if (RT_FAILURE(rc))
    {
        WARN(("vboxMpCrCtlConGetCapsNew failed rc (%d), ignoring..", rc));
        g_VBoxMpCrHostCapsInfo.u32CmdVbvaVersion = 0;
        rc = vboxMpCrCtlConGetCapsLegacy(&CrCtlCon, u32ClientID, &g_VBoxMpCrHostCapsInfo.u32Caps);
        if (RT_FAILURE(rc))
        {
            WARN(("vboxMpCrCtlConGetCapsLegacy failed rc (%d), ignoring..", rc));
            g_VBoxMpCrHostCapsInfo.u32Caps = 0;
        }
    }

    if (g_VBoxMpCrHostCapsInfo.u32Caps & CR_VBOX_CAP_HOST_CAPS_NOT_SUFFICIENT)
    {
        LOGREL(("Insufficient host 3D capabilities"));
        g_VBoxMpCr3DSupported = 0;
        memset(&g_VBoxMpCrHostCapsInfo, 0, sizeof (g_VBoxMpCrHostCapsInfo));
    }
    else
    {
        g_VBoxMpCr3DSupported = 1;
    }

#if 0 //ndef DEBUG_misha
    g_VBoxMpCrHostCapsInfo.u32Caps &= ~CR_VBOX_CAP_CMDVBVA;
    g_VBoxMpCrHostCapsInfo.u32CmdVbvaVersion = 0;
#endif

    rc = VBoxMpCrCtlConDisconnectHgcm(&CrCtlCon, u32ClientID);
    if (RT_FAILURE(rc))
        WARN(("VBoxMpCrCtlConDisconnectHgcm failed rc (%d), ignoring..", rc));
}

int VBoxMpCrCmdRxReadbackHandler(CRMessageReadback RT_UNTRUSTED_VOLATILE_HOST *pRx, uint32_t cbRx)
{
    if (cbRx < sizeof(*pRx))
    {
        WARN(("invalid rx size %d", cbRx));
        return VERR_INVALID_PARAMETER;
    }
    void    *pvSrc  = VBoxMpCrCmdRxReadbackData(pRx);
    uint32_t cbData = VBoxMpCrCmdRxReadbackDataSize(pRx, cbRx);
    void    *pvDst  = *((void**)&pRx->readback_ptr);
    memcpy(pvDst, pvSrc, cbData);
    return VINF_SUCCESS;
}

int VBoxMpCrCmdRxHandler(CRMessageHeader RT_UNTRUSTED_VOLATILE_HOST *pRx, uint32_t cbRx)
{
    if (cbRx < sizeof(*pRx))
    {
        WARN(("invalid rx size %d", cbRx));
        return VERR_INVALID_PARAMETER;
    }
    CRMessageType enmType = pRx->type;
    switch (enmType)
    {
        case CR_MESSAGE_READBACK:
            return VBoxMpCrCmdRxReadbackHandler((CRMessageReadback RT_UNTRUSTED_VOLATILE_HOST *)pRx, cbRx);
        default:
            WARN(("unsupported rx message type: %d", enmType));
            return VERR_INVALID_PARAMETER;
    }
}

#endif /* VBOX_WITH_CROGL */

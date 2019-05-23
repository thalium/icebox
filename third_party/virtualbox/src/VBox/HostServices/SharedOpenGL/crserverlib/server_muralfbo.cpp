/* $Id: server_muralfbo.cpp $ */

/** @file
 * VBox crOpenGL: Window to FBO redirect support.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "server.h"
#include "cr_string.h"
#include "cr_mem.h"
#include "cr_vreg.h"
#include "render/renderspu.h"

static void crServerRedirMuralFbSync(CRMuralInfo *mural);

void crServerCheckMuralGeometry(CRMuralInfo *mural)
{
    if (!mural->CreateInfo.externalID)
        return;

    CRASSERT(mural->spuWindow);
    CRASSERT(mural->spuWindow != CR_RENDER_DEFAULT_WINDOW_ID);

    if (!mural->width || !mural->height
            || mural->fboWidth != mural->width
            || mural->fboHeight != mural->height)
    {
        crServerRedirMuralFbClear(mural);
        crServerRedirMuralFBO(mural, false);
        crServerDeleteMuralFBO(mural);
    }

    if (!mural->width || !mural->height)
        return;

    crServerRedirMuralFBO(mural, true);
    crServerRedirMuralFbSync(mural);
}

static void crServerCheckMuralGeometryCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo *pMI = (CRMuralInfo*) data1;

    if (!pMI->fRedirected || pMI == data2)
        return;

    crServerCheckMuralGeometry(pMI);
}


void crServerCheckAllMuralGeometry(CRMuralInfo *pMI)
{
    CR_FBMAP Map;
    int rc = CrPMgrHlpGlblUpdateBegin(&Map);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrPMgrHlpGlblUpdateBegin failed %d", rc));
        return;
    }

    crHashtableWalk(cr_server.muralTable, crServerCheckMuralGeometryCB, pMI);

    if (pMI)
        crServerCheckMuralGeometry(pMI);

    CrPMgrHlpGlblUpdateEnd(&Map);
}

GLboolean crServerSupportRedirMuralFBO(void)
{
    static GLboolean fInited = GL_FALSE;
    static GLboolean fSupported = GL_FALSE;
    if (!fInited)
    {
        const GLubyte* pExt = cr_server.head_spu->dispatch_table.GetString(GL_REAL_EXTENSIONS);

        fSupported = ( NULL!=crStrstr((const char*)pExt, "GL_ARB_framebuffer_object")
                 || NULL!=crStrstr((const char*)pExt, "GL_EXT_framebuffer_object"))
               && NULL!=crStrstr((const char*)pExt, "GL_ARB_texture_non_power_of_two");
        fInited = GL_TRUE;
    }
    return fSupported;
}

static void crServerCreateMuralFBO(CRMuralInfo *mural);

void crServerRedirMuralFbClear(CRMuralInfo *mural)
{
    uint32_t i;
    for (i = 0; i < mural->cUsedFBDatas; ++i)
    {
        CR_FBDATA *pData = mural->apUsedFBDatas[i];
        int rc = CrFbUpdateBegin(pData->hFb);
        if (RT_SUCCESS(rc))
        {
            CrFbEntryRegionsSet(pData->hFb, pData->hFbEntry, NULL, 0, NULL, false);
            CrFbUpdateEnd(pData->hFb);
        }
        else
            WARN(("CrFbUpdateBegin failed rc %d", rc));
    }
    mural->cUsedFBDatas = 0;

    for (i = 0; i < (uint32_t)cr_server.screenCount; ++i)
    {
        GLuint j;
        CR_FBDATA *pData = &mural->aFBDatas[i];
        if (!pData->hFb)
            continue;

        if (pData->hFbEntry != NULL)
        {
            CrFbEntryRelease(pData->hFb, pData->hFbEntry);
            pData->hFbEntry = NULL;
        }

        /* Release all valid texture data structures in the array.
         * Do not rely on mural->cBuffers because it might be already
         * set to zero in crServerDeleteMuralFBO.
         */
        for (j = 0; j < RT_ELEMENTS(pData->apTexDatas); ++j)
        {
            if (pData->apTexDatas[j] != NULL)
            {
                CrTdRelease(pData->apTexDatas[j]);
                pData->apTexDatas[j] = NULL;
            }
        }

        pData->hFb = NULL;
    }
}

static int crServerRedirMuralDbSyncFb(CRMuralInfo *mural, HCR_FRAMEBUFFER hFb, CR_FBDATA **ppData)
{
    CR_FBDATA *pData;
    const struct VBVAINFOSCREEN* pScreenInfo = CrFbGetScreenInfo(hFb);
    const struct VBOXVR_SCR_COMPOSITOR* pCompositor = CrFbGetCompositor(hFb);
    RTRECT FbRect = *CrVrScrCompositorRectGet(pCompositor);
    RTRECT DefaultRegionsRect;
    const RTRECT * pRegions;
    uint32_t cRegions;
    RTPOINT Pos;
    RTRECT MuralRect;
    int rc;

    CRASSERT(mural->fRedirected);

    *ppData = NULL;

    if (!mural->bVisible)
        return VINF_SUCCESS;

    MuralRect.xLeft = mural->gX;
    MuralRect.yTop = mural->gY;
    MuralRect.xRight = MuralRect.xLeft + mural->width;
    MuralRect.yBottom = MuralRect.yTop + mural->height;

    Pos.x = mural->gX - pScreenInfo->i32OriginX;
    Pos.y = mural->gY - pScreenInfo->i32OriginY;

    VBoxRectTranslate(&FbRect, pScreenInfo->i32OriginX, pScreenInfo->i32OriginY);

    VBoxRectIntersect(&FbRect, &MuralRect);

    if (VBoxRectIsZero(&FbRect))
        return VINF_SUCCESS;

    if (mural->bReceivedRects)
    {
        pRegions = (const RTRECT*)mural->pVisibleRects;
        cRegions = mural->cVisibleRects;
    }
    else
    {
        DefaultRegionsRect.xLeft = 0;
        DefaultRegionsRect.yTop = 0;
        DefaultRegionsRect.xRight = mural->width;
        DefaultRegionsRect.yBottom = mural->height;
        pRegions = &DefaultRegionsRect;
        cRegions = 1;
    }

    if (!cRegions)
        return VINF_SUCCESS;

    pData = &mural->aFBDatas[pScreenInfo->u32ViewIndex];

    if (!pData->hFb)
    {
        /* Guard against modulo-by-zero when calling CrFbEntryCreateForTexData
           below. Observed when failing to load atig6pxx.dll and similar. */
        if (RT_UNLIKELY(mural->cBuffers == 0))
        {
            WARN(("crServerRedirMuralDbSyncFb: cBuffers == 0 (crServerSupportRedirMuralFBO=%d)", crServerSupportRedirMuralFBO()));
            return VERR_NOT_SUPPORTED;
        }

        pData->hFb = hFb;

        RT_ZERO(pData->apTexDatas);
        for (uint32_t i = 0; i < mural->cBuffers; ++i)
        {
            VBOXVR_TEXTURE Tex;
            Tex.width = mural->width;
            Tex.height = mural->height;
            Tex.hwid = mural->aidColorTexs[i];
            Tex.target = GL_TEXTURE_2D;

            pData->apTexDatas[i] = CrFbTexDataCreate(&Tex);
        }

        rc = CrFbEntryCreateForTexData(hFb, pData->apTexDatas[CR_SERVER_FBO_FB_IDX(mural)], 0, &pData->hFbEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrFbEntryCreateForTexData failed rc %d", rc));
        }
    }
    else
    {
        CRASSERT(pData->hFb == hFb);
    }

    rc = CrFbUpdateBegin(hFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbUpdateBegin failed rc %d", rc));
        return rc;
    }

    rc = CrFbEntryRegionsSet(hFb, pData->hFbEntry, &Pos, cRegions, pRegions, true);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbEntryRegionsSet failed rc %d", rc));
    }

    CrFbUpdateEnd(hFb);

    const struct VBOXVR_SCR_COMPOSITOR_ENTRY* pCEntry = CrFbEntryGetCompositorEntry(pData->hFbEntry);
    if (CrVrScrCompositorEntryIsUsed(pCEntry))
        *ppData = pData;

    return rc;
}

static void crServerRedirMuralFbSync(CRMuralInfo *mural)
{
    uint32_t i;
    uint32_t cUsedFBs = 0;
    HCR_FRAMEBUFFER ahUsedFbs[CR_MAX_GUEST_MONITORS];
    HCR_FRAMEBUFFER hFb;

    for (i = 0; i < mural->cUsedFBDatas; ++i)
    {
        CR_FBDATA *pData = mural->apUsedFBDatas[i];
        int rc = CrFbUpdateBegin(pData->hFb);
        if (RT_SUCCESS(rc))
        {
            ahUsedFbs[cUsedFBs] = pData->hFb;
            CrFbEntryRegionsSet(pData->hFb, pData->hFbEntry, NULL, 0, NULL, false);
            ++cUsedFBs;
        }
        else
            WARN(("CrFbUpdateBegin failed rc %d", rc));
    }
    mural->cUsedFBDatas = 0;

    if (!mural->width
            || !mural->height
            || !mural->bVisible
            )
        goto end;

    CRASSERT(mural->fRedirected);

    for (hFb = CrPMgrFbGetFirstEnabled();
            hFb;
            hFb = CrPMgrFbGetNextEnabled(hFb))
    {
        CR_FBDATA *pData = NULL;
        int rc = crServerRedirMuralDbSyncFb(mural, hFb, &pData);
        if (!RT_SUCCESS(rc))
        {
            WARN(("crServerRedirMuralDbSyncFb failed %d", rc));
            continue;
        }

        if (!pData)
            continue;

        mural->apUsedFBDatas[mural->cUsedFBDatas] = pData;
        ++mural->cUsedFBDatas;
    }

end:

    for (i = 0; i < cUsedFBs; ++i)
    {
        CrFbUpdateEnd(ahUsedFbs[i]);
    }
}

static void crVBoxServerMuralFbCleanCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo *pMI = (CRMuralInfo*) data1;
    HCR_FRAMEBUFFER hFb = (HCR_FRAMEBUFFER)data2;
    uint32_t i;
    for (i = 0; i < pMI->cUsedFBDatas; ++i)
    {
        CR_FBDATA *pData = pMI->apUsedFBDatas[i];
        if (hFb != pData->hFb)
            continue;

        CrFbEntryRegionsSet(pData->hFb, pData->hFbEntry, NULL, 0, NULL, false);
        break;
    }
}

static void crVBoxServerMuralFbSetCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo *pMI = (CRMuralInfo*) data1;
    HCR_FRAMEBUFFER hFb = (HCR_FRAMEBUFFER)data2;
    uint32_t i;
    CR_FBDATA *pData = NULL;
    bool fFbWasUsed = false;

    Assert(hFb);

    if (!pMI->fRedirected)
    {
        Assert(!pMI->cUsedFBDatas);
        return;
    }

    for (i = 0; i < pMI->cUsedFBDatas; ++i)
    {
        CR_FBDATA *pData = pMI->apUsedFBDatas[i];
        if (hFb != pData->hFb)
            continue;

        fFbWasUsed = true;
        break;
    }

    if (CrFbIsEnabled(hFb))
    {
        int rc = crServerRedirMuralDbSyncFb(pMI, hFb, &pData);
        if (!RT_SUCCESS(rc))
        {
            WARN(("crServerRedirMuralDbSyncFb failed %d", rc));
            pData = NULL;
        }
    }

    if (pData)
    {
        if (!fFbWasUsed)
        {
            uint32_t idScreen = CrFbGetScreenInfo(hFb)->u32ViewIndex;
            for (i = 0; i < pMI->cUsedFBDatas; ++i)
            {
                CR_FBDATA *pData = pMI->apUsedFBDatas[i];
                uint32_t idCurScreen = CrFbGetScreenInfo(pData->hFb)->u32ViewIndex;
                if (idCurScreen > idScreen)
                    break;

                Assert(idCurScreen != idScreen);
            }

            for (uint32_t j = pMI->cUsedFBDatas; j > i; --j)
            {
                pMI->apUsedFBDatas[j] = pMI->apUsedFBDatas[j-1];
            }

            pMI->apUsedFBDatas[i] = pData;
            ++pMI->cUsedFBDatas;
        }
        /* else - nothing to do */
    }
    else
    {
        if (fFbWasUsed)
        {
            for (uint32_t j = i; j < pMI->cUsedFBDatas - 1; ++j)
            {
                pMI->apUsedFBDatas[j] = pMI->apUsedFBDatas[j+1];
            }
            --pMI->cUsedFBDatas;
        }
        /* else - nothing to do */
    }
}

void crVBoxServerMuralFbResizeEnd(HCR_FRAMEBUFFER hFb)
{
    crHashtableWalk(cr_server.muralTable, crVBoxServerMuralFbSetCB, hFb);
}

void crVBoxServerMuralFbResizeBegin(HCR_FRAMEBUFFER hFb)
{
    crHashtableWalk(cr_server.muralTable, crVBoxServerMuralFbCleanCB, hFb);
}

DECLEXPORT(int) crVBoxServerNotifyResize(const struct VBVAINFOSCREEN *pScreen, void *pvVRAM)
{
    if (cr_server.fCrCmdEnabled)
    {
        WARN(("crVBoxServerNotifyResize for enabled CrCmd"));
        return VERR_INVALID_STATE;
    }

    if (pScreen->u32ViewIndex >= (uint32_t)cr_server.screenCount)
    {
        WARN(("invalid view index"));
        return VERR_INVALID_PARAMETER;
    }

    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);

    memset(aTargetMap, 0, sizeof (aTargetMap));

    ASMBitSet(aTargetMap, pScreen->u32ViewIndex);

    int rc = CrPMgrResize(pScreen, pvVRAM, aTargetMap);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return VINF_SUCCESS;
}

void crServerRedirMuralFBO(CRMuralInfo *mural, bool fEnabled)
{
    if (!mural->fRedirected == !fEnabled)
    {
        return;
    }

    if (!mural->CreateInfo.externalID)
    {
        WARN(("trying to change redir setting for internal mural %d", mural->spuWindow));
        return;
    }

    if (fEnabled)
    {
        if (!crServerSupportRedirMuralFBO())
        {
            WARN(("FBO not supported, can't redirect window output"));
            return;
        }

        if (mural->aidFBOs[0]==0)
        {
            crServerCreateMuralFBO(mural);
        }

        if (cr_server.curClient && cr_server.curClient->currentMural == mural)
        {
            if (!crStateGetCurrent()->framebufferobject.drawFB)
            {
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, CR_SERVER_FBO_FOR_IDX(mural, mural->iCurDrawBuffer));
            }
            if (!crStateGetCurrent()->framebufferobject.readFB)
            {
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_READ_FRAMEBUFFER, CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer));
            }

            crStateGetCurrent()->buffer.width = 0;
            crStateGetCurrent()->buffer.height = 0;
        }
    }
    else
    {
        if (cr_server.curClient && cr_server.curClient->currentMural == mural)
        {
            if (!crStateGetCurrent()->framebufferobject.drawFB)
            {
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
            }
            if (!crStateGetCurrent()->framebufferobject.readFB)
            {
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);
            }

            crStateGetCurrent()->buffer.width = mural->width;
            crStateGetCurrent()->buffer.height = mural->height;
        }
    }

    mural->fRedirected = !!fEnabled;
}

static void crServerCreateMuralFBO(CRMuralInfo *mural)
{
    CRContext *ctx = crStateGetCurrent();
    GLuint uid, i;
    GLenum status;
    SPUDispatchTable *gl = &cr_server.head_spu->dispatch_table;
    CRContextInfo *pMuralContextInfo;

    CRASSERT(mural->aidFBOs[0]==0);
    CRASSERT(mural->aidFBOs[1]==0);

    pMuralContextInfo = cr_server.currentCtxInfo;
    if (!pMuralContextInfo)
    {
        /* happens on saved state load */
        CRASSERT(cr_server.MainContextInfo.SpuContext);
        pMuralContextInfo = &cr_server.MainContextInfo;
        cr_server.head_spu->dispatch_table.MakeCurrent(mural->spuWindow, 0, cr_server.MainContextInfo.SpuContext);
    }

    if (pMuralContextInfo->CreateInfo.realVisualBits != mural->CreateInfo.realVisualBits)
    {
        WARN(("mural visual bits do not match with current context visual bits!"));
    }

    mural->cBuffers = 2;
    mural->iBbBuffer = 0;
    /*Color texture*/

    if (crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        gl->BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }

    for (i = 0; i < mural->cBuffers; ++i)
    {
        gl->GenTextures(1, &mural->aidColorTexs[i]);
        gl->BindTexture(GL_TEXTURE_2D, mural->aidColorTexs[i]);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mural->width, mural->height,
                       0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    }

    /* Depth & Stencil. */
    gl->GenRenderbuffersEXT(1, &mural->idDepthStencilRB);
    gl->BindRenderbufferEXT(GL_RENDERBUFFER_EXT, mural->idDepthStencilRB);
    gl->RenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT,
                           mural->width, mural->height);

    /*FBO*/
    for (i = 0; i < mural->cBuffers; ++i)
    {
        gl->GenFramebuffersEXT(1, &mural->aidFBOs[i]);
        gl->BindFramebufferEXT(GL_FRAMEBUFFER_EXT, mural->aidFBOs[i]);

        gl->FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                    GL_TEXTURE_2D, mural->aidColorTexs[i], 0);
        gl->FramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                       GL_RENDERBUFFER_EXT, mural->idDepthStencilRB);
        gl->FramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                       GL_RENDERBUFFER_EXT, mural->idDepthStencilRB);

        status = gl->CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (status!=GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            WARN(("FBO status(0x%x) isn't complete", status));
        }
    }

    mural->iCurDrawBuffer = crServerMuralFBOIdxFromBufferName(mural, ctx->buffer.drawBuffer);
    mural->iCurReadBuffer = crServerMuralFBOIdxFromBufferName(mural, ctx->buffer.readBuffer);

    mural->fboWidth = mural->width;
    mural->fboHeight = mural->height;

    mural->iCurDrawBuffer = crServerMuralFBOIdxFromBufferName(mural, ctx->buffer.drawBuffer);
    mural->iCurReadBuffer = crServerMuralFBOIdxFromBufferName(mural, ctx->buffer.readBuffer);

    /*Restore gl state*/
    uid = ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D->hwid;
    gl->BindTexture(GL_TEXTURE_2D, uid);

    uid = ctx->framebufferobject.renderbuffer ? ctx->framebufferobject.renderbuffer->hwid:0;
    gl->BindRenderbufferEXT(GL_RENDERBUFFER_EXT, uid);

    uid = ctx->framebufferobject.drawFB ? ctx->framebufferobject.drawFB->hwid:0;
    gl->BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, uid);

    uid = ctx->framebufferobject.readFB ? ctx->framebufferobject.readFB->hwid:0;
    gl->BindFramebufferEXT(GL_READ_FRAMEBUFFER, uid);

    if (crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        gl->BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->bufferobject.unpackBuffer->hwid);
    }

    if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
    {
        gl->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, ctx->bufferobject.packBuffer->hwid);
    }
    else
    {
        gl->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }

    CRASSERT(mural->aidColorTexs[CR_SERVER_FBO_FB_IDX(mural)]);
}

void crServerDeleteMuralFBO(CRMuralInfo *mural)
{
    if (mural->aidFBOs[0]!=0)
    {
        GLuint i;
        for (i = 0; i < mural->cBuffers; ++i)
        {
            cr_server.head_spu->dispatch_table.DeleteTextures(1, &mural->aidColorTexs[i]);
            mural->aidColorTexs[i] = 0;
        }

        cr_server.head_spu->dispatch_table.DeleteRenderbuffersEXT(1, &mural->idDepthStencilRB);
        mural->idDepthStencilRB = 0;

        for (i = 0; i < mural->cBuffers; ++i)
        {
            cr_server.head_spu->dispatch_table.DeleteFramebuffersEXT(1, &mural->aidFBOs[i]);
            mural->aidFBOs[i] = 0;
        }
    }

    mural->cBuffers = 0;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static GLboolean crServerIntersectRect(CRrecti *a, CRrecti *b, CRrecti *rect)
{
    CRASSERT(a && b && rect);

    rect->x1 = MAX(a->x1, b->x1);
    rect->x2 = MIN(a->x2, b->x2);
    rect->y1 = MAX(a->y1, b->y1);
    rect->y2 = MIN(a->y2, b->y2);

    return (rect->x2>rect->x1) && (rect->y2>rect->y1);
}

DECLEXPORT(void) crServerVBoxCompositionSetEnableStateGlobal(GLboolean fEnable)
{
}

DECLEXPORT(void) crServerVBoxScreenshotRelease(CR_SCREENSHOT *pScreenshot)
{
    if (pScreenshot->fDataAllocated)
    {
        RTMemFree(pScreenshot->Img.pvData);
        pScreenshot->fDataAllocated = 0;
    }
}

DECLEXPORT(int) crServerVBoxScreenshotGet(uint32_t u32Screen, uint32_t width, uint32_t height, uint32_t pitch, void *pvBuffer, CR_SCREENSHOT *pScreenshot)
{
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabledForScreen(u32Screen);
    if (!hFb)
        return VERR_INVALID_STATE;

    const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hFb);

    if (!width)
        width = pScreen->u32Width;
    if (!height)
        height = pScreen->u32Height;
    if (!pitch)
        pitch = pScreen->u32LineSize;

    if (CrFbHas3DData(hFb)
            || pScreen->u32Width != width
            || pScreen->u32Height != height
            || pScreen->u32LineSize != pitch
            || pScreen->u16BitsPerPixel != 32)
    {
        RTRECTSIZE SrcRectSize;
        RTRECT DstRect;

        pScreenshot->Img.cbData = pScreen->u32LineSize * pScreen->u32Height;
        if (!pvBuffer)
        {
            pScreenshot->Img.pvData = RTMemAlloc(pScreenshot->Img.cbData);
            if (!pScreenshot->Img.pvData)
            {
                WARN(("RTMemAlloc failed"));
                return VERR_NO_MEMORY;
            }
            pScreenshot->fDataAllocated = 1;
        }
        else
        {
            pScreenshot->Img.pvData = pvBuffer;
            pScreenshot->fDataAllocated = 0;
        }

        pScreenshot->Img.enmFormat = GL_BGRA;
        pScreenshot->Img.width = width;
        pScreenshot->Img.height = height;
        pScreenshot->Img.bpp = 32;
        pScreenshot->Img.pitch = pitch;
        SrcRectSize.cx = pScreen->u32Width;
        SrcRectSize.cy = pScreen->u32Height;
        DstRect.xLeft = 0;
        DstRect.yTop = 0;
        DstRect.xRight = width;
        DstRect.yBottom = height;
        int rc = CrFbBltGetContentsEx(hFb, &SrcRectSize, &DstRect, 1, &DstRect, &pScreenshot->Img);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrFbBltGetContents failed %d", rc));
            crServerVBoxScreenshotRelease(pScreenshot);
            return rc;
        }
    }
    else
    {
        pScreenshot->Img.cbData = pScreen->u32LineSize * pScreen->u32Height;
        if (!pvBuffer)
            pScreenshot->Img.pvData = CrFbGetVRAM(hFb);
        else
        {
            pScreenshot->Img.pvData = pvBuffer;
            memcpy(pvBuffer, CrFbGetVRAM(hFb), pScreenshot->Img.cbData);
        }
        pScreenshot->Img.enmFormat = GL_BGRA;
        pScreenshot->Img.width = pScreen->u32Width;
        pScreenshot->Img.height = pScreen->u32Height;
        pScreenshot->Img.bpp = pScreen->u16BitsPerPixel;
        pScreenshot->Img.pitch = pScreen->u32LineSize;

        pScreenshot->fDataAllocated = 0;
    }

    pScreenshot->u32Screen = u32Screen;

    return VINF_SUCCESS;
}

extern DECLEXPORT(int) crServerVBoxWindowsShow(bool fShow)
{
    return CrPMgrModeWinVisible(fShow);
}

void crServerPresentFBO(CRMuralInfo *mural)
{
    uint32_t i;
    for (i = 0; i < mural->cUsedFBDatas; ++i)
    {
        CR_FBDATA *pData = mural->apUsedFBDatas[i];
        int rc = CrFbUpdateBegin(pData->hFb);
        if (RT_SUCCESS(rc))
        {
            CrFbEntryTexDataUpdate(pData->hFb, pData->hFbEntry, pData->apTexDatas[CR_SERVER_FBO_FB_IDX(mural)]);
            CrFbUpdateEnd(pData->hFb);
        }
        else
            WARN(("CrFbUpdateBegin failed rc %d", rc));
    }
}

GLboolean crServerIsRedirectedToFBO()
{
#ifdef DEBUG_misha
    Assert(cr_server.curClient);
    if (cr_server.curClient)
    {
        Assert(cr_server.curClient->currentMural == cr_server.currentMural);
        Assert(cr_server.curClient->currentCtxInfo == cr_server.currentCtxInfo);
    }
#endif
    return cr_server.curClient
           && cr_server.curClient->currentMural
           && cr_server.curClient->currentMural->fRedirected;
}

GLint crServerMuralFBOIdxFromBufferName(CRMuralInfo *mural, GLenum buffer)
{
    switch (buffer)
    {
        case GL_FRONT:
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
            return CR_SERVER_FBO_FB_IDX(mural);
        case GL_BACK:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
            return CR_SERVER_FBO_BB_IDX(mural);
        case GL_NONE:
        case GL_AUX0:
        case GL_AUX1:
        case GL_AUX2:
        case GL_AUX3:
        case GL_LEFT:
        case GL_RIGHT:
        case GL_FRONT_AND_BACK:
            return -1;
        default:
            WARN(("crServerMuralFBOIdxFromBufferName: invalid buffer passed 0x%x", buffer));
            return -2;
    }
}

void crServerMuralFBOSwapBuffers(CRMuralInfo *mural)
{
    CRContext *ctx = crStateGetCurrent();
    GLuint iOldCurDrawBuffer = mural->iCurDrawBuffer;
    GLuint iOldCurReadBuffer = mural->iCurReadBuffer;
    mural->iBbBuffer = ((mural->iBbBuffer + 1) % (mural->cBuffers));
    if (mural->iCurDrawBuffer >= 0)
        mural->iCurDrawBuffer = ((mural->iCurDrawBuffer + 1) % (mural->cBuffers));
    if (mural->iCurReadBuffer >= 0)
        mural->iCurReadBuffer = ((mural->iCurReadBuffer + 1) % (mural->cBuffers));
    Assert(iOldCurDrawBuffer != mural->iCurDrawBuffer || mural->cBuffers == 1 || mural->iCurDrawBuffer < 0);
    Assert(iOldCurReadBuffer != mural->iCurReadBuffer || mural->cBuffers == 1 || mural->iCurReadBuffer < 0);
    if (!ctx->framebufferobject.drawFB && iOldCurDrawBuffer != mural->iCurDrawBuffer)
    {
        cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, CR_SERVER_FBO_FOR_IDX(mural, mural->iCurDrawBuffer));
    }
    if (!ctx->framebufferobject.readFB && iOldCurReadBuffer != mural->iCurReadBuffer)
    {
        cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_READ_FRAMEBUFFER, CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer));
    }
    Assert(mural->aidColorTexs[CR_SERVER_FBO_FB_IDX(mural)]);
}


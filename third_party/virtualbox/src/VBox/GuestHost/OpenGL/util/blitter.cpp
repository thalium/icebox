/* $Id: blitter.cpp $ */
/** @file
 * Blitter API implementation
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifdef IN_VMSVGA3D
# include <OpenGL/OpenGL.h>
# include <OpenGL/gl3.h>
# include "../include/cr_blitter.h"
# include <iprt/assert.h>
# define WARN       AssertMsgFailed
# define CRASSERT   Assert
DECLINLINE(void) crWarning(const char *format, ... ) {}
#else
# include "cr_blitter.h"
# include "cr_spu.h"
# include "chromium.h"
# include "cr_error.h"
# include "cr_net.h"
# include "cr_rand.h"
# include "cr_mem.h"
# include "cr_string.h"
# include "cr_bmpscale.h"
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/mem.h>



static void crMClrFillMem(uint32_t *pu32Dst, int32_t cbDstPitch, uint32_t width, uint32_t height, uint32_t u32Color)
{
    for (uint32_t i = 0; i < height; ++i)
    {
        for (uint32_t j = 0; j < width; ++j)
        {
            pu32Dst[j] = u32Color;
        }

        pu32Dst = (uint32_t*)(((uint8_t*)pu32Dst) + cbDstPitch);
    }
}

void CrMClrFillImgRect(CR_BLITTER_IMG *pDst, const RTRECT *pCopyRect, uint32_t u32Color)
{
    int32_t x = pCopyRect->xLeft;
    int32_t y = pCopyRect->yTop;
    int32_t width = pCopyRect->xRight - pCopyRect->xLeft;
    int32_t height = pCopyRect->yBottom - pCopyRect->yTop;
    Assert(x >= 0);
    Assert(y >= 0);
    uint8_t *pu8Dst = ((uint8_t*)pDst->pvData) + pDst->pitch * y + x * 4;

    crMClrFillMem((uint32_t*)pu8Dst, pDst->pitch, width, height, u32Color);
}

void CrMClrFillImg(CR_BLITTER_IMG *pImg, uint32_t cRects, const RTRECT *pRects, uint32_t u32Color)
{
    RTRECT Rect;
    Rect.xLeft = 0;
    Rect.yTop = 0;
    Rect.xRight = pImg->width;
    Rect.yBottom = pImg->height;


    RTRECT Intersection;
    /*const RTPOINT ZeroPoint = {0, 0}; - unused */

    for (uint32_t i = 0; i < cRects; ++i)
    {
        const RTRECT * pRect = &pRects[i];
        VBoxRectIntersected(pRect, &Rect, &Intersection);

        if (VBoxRectIsZero(&Intersection))
            continue;

        CrMClrFillImgRect(pImg, &Intersection, u32Color);
    }
}

static void crMBltMem(const uint8_t *pu8Src, int32_t cbSrcPitch, uint8_t *pu8Dst, int32_t cbDstPitch, uint32_t width, uint32_t height)
{
    uint32_t cbCopyRow = width * 4;

    for (uint32_t i = 0; i < height; ++i)
    {
        memcpy(pu8Dst, pu8Src, cbCopyRow);

        pu8Src += cbSrcPitch;
        pu8Dst += cbDstPitch;
    }
}

void CrMBltImgRect(const CR_BLITTER_IMG *pSrc, const RTPOINT *pSrcDataPoint, bool fSrcInvert, const RTRECT *pCopyRect, CR_BLITTER_IMG *pDst)
{
    int32_t srcX = pCopyRect->xLeft - pSrcDataPoint->x;
    int32_t srcY = pCopyRect->yTop - pSrcDataPoint->y;
    Assert(srcX >= 0);
    Assert(srcY >= 0);
    Assert(srcX < (int32_t)pSrc->width);
    Assert(srcY < (int32_t)pSrc->height);

    int32_t dstX = pCopyRect->xLeft;
    int32_t dstY = pCopyRect->yTop;
    Assert(dstX >= 0);
    Assert(dstY >= 0);
    Assert(dstX < (int32_t)pDst->width);
    Assert(dstY < (int32_t)pDst->height);

    uint8_t *pu8Src = ((uint8_t*)pSrc->pvData) + pSrc->pitch * (!fSrcInvert ? srcY : pSrc->height - srcY - 1) + srcX * 4;
    uint8_t *pu8Dst = ((uint8_t*)pDst->pvData) + pDst->pitch * dstY + dstX * 4;

    crMBltMem(pu8Src, fSrcInvert ? -((int32_t)pSrc->pitch) : (int32_t)pSrc->pitch, pu8Dst, pDst->pitch, pCopyRect->xRight - pCopyRect->xLeft, pCopyRect->yBottom - pCopyRect->yTop);
}

void CrMBltImg(const CR_BLITTER_IMG *pSrc, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pDst)
{
    RTRECT Intersection;
    RTRECT RestrictSrcRect;
    RestrictSrcRect.xLeft = 0;
    RestrictSrcRect.yTop = 0;
    RestrictSrcRect.xRight = pSrc->width;
    RestrictSrcRect.yBottom = pSrc->height;
    RTRECT RestrictDstRect;
    RestrictDstRect.xLeft = 0;
    RestrictDstRect.yTop = 0;
    RestrictDstRect.xRight = pDst->width;
    RestrictDstRect.yBottom = pDst->height;

    for (uint32_t i = 0; i < cRects; ++i)
    {
        const RTRECT * pRect = &pRects[i];
        VBoxRectIntersected(pRect, &RestrictDstRect, &Intersection);

        RTRECT TranslatedSrc;
        VBoxRectTranslated(&RestrictSrcRect, pPos->x, pPos->y, &TranslatedSrc);

        VBoxRectIntersect(&Intersection, &TranslatedSrc);

        if (VBoxRectIsZero(&Intersection))
            continue;

        CrMBltImgRect(pSrc, pPos, false, &Intersection, pDst);
    }
}

#ifndef IN_VMSVGA3D

void CrMBltImgRectScaled(const CR_BLITTER_IMG *pSrc, const RTPOINT *pPos, bool fSrcInvert, const RTRECT *pCopyRect, float strX, float strY, CR_BLITTER_IMG *pDst)
{
    RTPOINT UnscaledPos;
    UnscaledPos.x = CR_FLOAT_RCAST(int32_t, pPos->x / strX);
    UnscaledPos.y = CR_FLOAT_RCAST(int32_t, pPos->y / strY);

    RTRECT UnscaledCopyRect;

    VBoxRectUnscaled(pCopyRect, strX, strY, &UnscaledCopyRect);

    if (VBoxRectIsZero(&UnscaledCopyRect))
    {
        WARN(("ups"));
        return;
    }

    int32_t srcX = UnscaledCopyRect.xLeft - UnscaledPos.x;
    int32_t srcY = UnscaledCopyRect.yTop - UnscaledPos.y;
    if (srcX < 0)
    {
        WARN(("ups"));
        srcX = 0;
    }
    if (srcY < 0)
    {
        WARN(("ups"));
        srcY = 0;
    }

    if ((GLuint)srcX >= pSrc->width)
    {
        WARN(("ups"));
        return;
    }

    if ((GLuint)srcY >= pSrc->height)
    {
        WARN(("ups"));
        return;
    }

    Assert(srcX >= 0);
    Assert(srcY >= 0);
    Assert(srcX < (int32_t)pSrc->width);
    Assert(srcY < (int32_t)pSrc->height);

    int32_t dstX = pCopyRect->xLeft;
    int32_t dstY = pCopyRect->yTop;
    Assert(dstX >= 0);
    Assert(dstY >= 0);

    int32_t UnscaledSrcWidth = UnscaledCopyRect.xRight - UnscaledCopyRect.xLeft;
    int32_t UnscaledSrcHeight = UnscaledCopyRect.yBottom - UnscaledCopyRect.yTop;

    if (UnscaledSrcWidth + srcX > (GLint)pSrc->width)
        UnscaledSrcWidth = pSrc->width - srcX;

    if (UnscaledSrcHeight + srcY > (GLint)pSrc->height)
        UnscaledSrcHeight = pSrc->height - srcY;

    uint8_t *pu8Src = ((uint8_t*)pSrc->pvData) + pSrc->pitch * (!fSrcInvert ? srcY : pSrc->height - srcY - 1) + srcX * 4;
    uint8_t *pu8Dst = ((uint8_t*)pDst->pvData) + pDst->pitch * dstY + dstX * 4;

    CrBmpScale32(pu8Dst, pDst->pitch,
                        pCopyRect->xRight - pCopyRect->xLeft, pCopyRect->yBottom - pCopyRect->yTop,
                        pu8Src,
                        fSrcInvert ? -((int32_t)pSrc->pitch) : (int32_t)pSrc->pitch,
                        UnscaledSrcWidth, UnscaledSrcHeight);
}


void CrMBltImgScaled(const CR_BLITTER_IMG *pSrc, const RTRECTSIZE *pSrcRectSize, const RTRECT *pDstRect, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pDst)
{
    int32_t srcWidth = pSrcRectSize->cx;
    int32_t srcHeight = pSrcRectSize->cy;
    int32_t dstWidth = pDstRect->xRight - pDstRect->xLeft;
    int32_t dstHeight = pDstRect->yBottom - pDstRect->yTop;

    float strX = ((float)dstWidth) / srcWidth;
    float strY = ((float)dstHeight) / srcHeight;
    Assert(dstWidth != srcWidth || dstHeight != srcHeight);

    RTRECT Intersection;
    RTRECT ScaledRestrictSrcRect;
    ScaledRestrictSrcRect.xLeft = 0;
    ScaledRestrictSrcRect.yTop = 0;
    ScaledRestrictSrcRect.xRight = CR_FLOAT_RCAST(int32_t, pSrc->width * strX);
    ScaledRestrictSrcRect.yBottom = CR_FLOAT_RCAST(int32_t, pSrc->height * strY);
    RTRECT RestrictDstRect;
    RestrictDstRect.xLeft = 0;
    RestrictDstRect.yTop = 0;
    RestrictDstRect.xRight = pDst->width;
    RestrictDstRect.yBottom = pDst->height;

    RTPOINT Pos = {pDstRect->xLeft, pDstRect->yTop};

    for (uint32_t i = 0; i < cRects; ++i)
    {
        const RTRECT * pRect = &pRects[i];
        VBoxRectIntersected(pRect, &RestrictDstRect, &Intersection);

        RTRECT TranslatedSrc;
        VBoxRectTranslated(&ScaledRestrictSrcRect, Pos.x, Pos.y, &TranslatedSrc);

        VBoxRectIntersect(&Intersection, &TranslatedSrc);

        if (VBoxRectIsZero(&Intersection))
            continue;

        CrMBltImgRectScaled(pSrc, &Pos, false, &Intersection, strX, strY, pDst);
    }
}

#endif /* !IN_VMSVGA3D */


/**
 *
 * @param   pBlitter        The blitter to initialize.
 * @param   pCtxBase        Contains the blitter context info. Its value is
 *                          treated differently depending on the fCreateNewCtx
 *                          value.
 * @param   fCreateNewCtx   If true, then @a pCtxBase must NOT be NULL. Its
 *                          visualBits is used as a visual bits info for the new
 *                          context, its id field is used to specified the
 *                          shared context id to be used for blitter context.
 *                          The id can be null to specify no shared context is
 *                          needed
 *
 *                          If false and @a pCtxBase is NOT null AND its id
 *                          field is NOT null, then specified the blitter
 *                          context to be used blitter treats it as if it has
 *                          default ogl state.
 *
 *                          Otherwise, the blitter works in a "no-context" mode,
 *                          i.e. the caller is responsible for making a proper
 *                          context current before calling the blitter. Note
 *                          that BltEnter/Leave MUST still be called, but the
 *                          proper context must be set before doing BltEnter,
 *                          and ResoreContext info is ignored in that case. Also
 *                          note that the blitter caches the current window
 *                          info, and assumes the current context's values are
 *                          preserved wrt that window before the calls, so if
 *                          one uses different contexts for one blitter, the
 *                          blitter current window values must be explicitly
 *                          reset by doing CrBltMuralSetCurrentInfo(pBlitter,
 *                          NULL).
 * @param   fForceDrawBlt   If true this forces the blitter to always use
 *                          glDrawXxx-based blits even if
 *                          GL_EXT_framebuffer_blit.  This is needed because
 *                          BlitFramebufferEXT is often known to be buggy, and
 *                          glDrawXxx-based blits appear to be more reliable.
 * @param   pShaders
 * @param   pDispatch
 */
VBOXBLITTERDECL(int) CrBltInit(PCR_BLITTER pBlitter, const CR_BLITTER_CONTEXT *pCtxBase,
                               bool fCreateNewCtx, bool fForceDrawBlt, const CR_GLSL_CACHE *pShaders,
                               SPUDispatchTable *pDispatch)
{
    if (pCtxBase && pCtxBase->Base.id < 0)
    {
        crWarning("Default share context not initialized!");
        return VERR_INVALID_PARAMETER;
    }

    if (!pCtxBase && fCreateNewCtx)
    {
        crWarning("pCtxBase is zero while fCreateNewCtx is set!");
        return VERR_INVALID_PARAMETER;
    }

    RT_ZERO(*pBlitter);

    pBlitter->pDispatch = pDispatch;
    if (pCtxBase)
        pBlitter->CtxInfo = *pCtxBase;

    pBlitter->Flags.ForceDrawBlit = fForceDrawBlt;

    if (fCreateNewCtx)
    {
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
        pBlitter->CtxInfo.Base.id = 0;
#else
        pBlitter->CtxInfo.Base.id = pDispatch->CreateContext("", pCtxBase->Base.visualBits, pCtxBase->Base.id);
#endif
        if (!pBlitter->CtxInfo.Base.id)
        {
            RT_ZERO(*pBlitter);
            crWarning("CreateContext failed!");
            return VERR_GENERAL_FAILURE;
        }
        pBlitter->Flags.CtxCreated = 1;
    }

    if (pShaders)
    {
        pBlitter->pGlslCache = pShaders;
        pBlitter->Flags.ShadersGloal = 1;
    }
    else
    {
        CrGlslInit(&pBlitter->LocalGlslCache, pDispatch);
        pBlitter->pGlslCache = &pBlitter->LocalGlslCache;
    }

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(int) CrBltCleanup(PCR_BLITTER pBlitter)
{
    if (CrBltIsEntered(pBlitter))
    {
        WARN(("CrBltBlitTexTex: blitter is entered"));
        return VERR_INVALID_STATE;
    }

    if (pBlitter->Flags.ShadersGloal || !CrGlslNeedsCleanup(&pBlitter->LocalGlslCache))
        return VINF_SUCCESS;

    int rc = CrBltEnter(pBlitter);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrBltEnter failed, rc %d", rc));
        return rc;
    }

    CrGlslCleanup(&pBlitter->LocalGlslCache);

    CrBltLeave(pBlitter);

    return VINF_SUCCESS;
}

void CrBltTerm(PCR_BLITTER pBlitter)
{
#ifdef IN_VMSVGA3D
    /** @todo IN_VMSVGA3D */
#else
    if (pBlitter->Flags.CtxCreated)
        pBlitter->pDispatch->DestroyContext(pBlitter->CtxInfo.Base.id);
#endif
    memset(pBlitter, 0, sizeof (*pBlitter));
}

int CrBltMuralSetCurrentInfo(PCR_BLITTER pBlitter, const CR_BLITTER_WINDOW *pMural)
{
    if (pMural)
    {
        if (!memcmp(&pBlitter->CurrentMural, pMural, sizeof (pBlitter->CurrentMural)))
            return VINF_SUCCESS;
        memcpy(&pBlitter->CurrentMural, pMural, sizeof (pBlitter->CurrentMural));
    }
    else
    {
        if (CrBltIsEntered(pBlitter))
        {
            WARN(("can not set null mural for entered bleater"));
            return VERR_INVALID_STATE;
        }
        if (!pBlitter->CurrentMural.Base.id)
            return VINF_SUCCESS;
        pBlitter->CurrentMural.Base.id = 0;
    }

    pBlitter->Flags.CurrentMuralChanged = 1;

    if (!CrBltIsEntered(pBlitter))
        return VINF_SUCCESS;

    if (!pBlitter->CtxInfo.Base.id)
    {
        WARN(("setting current mural for entered no-context blitter"));
        return VERR_INVALID_STATE;
    }

    WARN(("changing mural for entered blitter, is is somewhat expected?"));

#ifdef IN_VMSVGA3D
    /** @todo IN_VMSVGA3D */
#else
    pBlitter->pDispatch->Flush();

    pBlitter->pDispatch->MakeCurrent(pMural->Base.id, pBlitter->i32MakeCurrentUserData, pBlitter->CtxInfo.Base.id);
#endif

    return VINF_SUCCESS;
}


#ifndef IN_VMSVGA3D

static DECLCALLBACK(int) crBltBlitTexBufImplFbo(PCR_BLITTER pBlitter, const VBOXVR_TEXTURE *pSrc, const RTRECT *paSrcRect, const RTRECTSIZE *pDstSize, const RTRECT *paDstRect, uint32_t cRects, uint32_t fFlags)
{
    GLenum filter = CRBLT_FILTER_FROM_FLAGS(fFlags);
    pBlitter->pDispatch->BindFramebufferEXT(GL_READ_FRAMEBUFFER, pBlitter->idFBO);
    pBlitter->pDispatch->FramebufferTexture2DEXT(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pSrc->target, pSrc->hwid, 0);
    pBlitter->pDispatch->ReadBuffer(GL_COLOR_ATTACHMENT0);

    for (uint32_t i = 0; i < cRects; ++i)
    {
        const RTRECT * pSrcRect = &paSrcRect[i];
        const RTRECT * pDstRect = &paDstRect[i];
        int32_t srcY1;
        int32_t srcY2;
        int32_t dstY1;
        int32_t dstY2;
        int32_t srcX1 = pSrcRect->xLeft;
        int32_t srcX2 = pSrcRect->xRight;
        int32_t dstX1 = pDstRect->xLeft;
        int32_t dstX2 = pDstRect->xRight;

        if (CRBLT_F_INVERT_SRC_YCOORDS & fFlags)
        {
            srcY1 = pSrc->height - pSrcRect->yTop;
            srcY2 = pSrc->height - pSrcRect->yBottom;
        }
        else
        {
            srcY1 = pSrcRect->yTop;
            srcY2 = pSrcRect->yBottom;
        }

        if (CRBLT_F_INVERT_DST_YCOORDS & fFlags)
        {
            dstY1 = pDstSize->cy - pDstRect->yTop;
            dstY2 = pDstSize->cy - pDstRect->yBottom;
        }
        else
        {
            dstY1 = pDstRect->yTop;
            dstY2 = pDstRect->yBottom;
        }

        if (srcY1 > srcY2)
        {
            if (dstY1 > dstY2)
            {
                /* use srcY1 < srcY2 && dstY1 < dstY2 whenever possible to avoid GPU driver bugs */
                int32_t tmp = srcY1;
                srcY1 = srcY2;
                srcY2 = tmp;
                tmp = dstY1;
                dstY1 = dstY2;
                dstY2 = tmp;
            }
        }

        if (srcX1 > srcX2)
        {
            if (dstX1 > dstX2)
            {
                /* use srcX1 < srcX2 && dstX1 < dstX2 whenever possible to avoid GPU driver bugs */
                int32_t tmp = srcX1;
                srcX1 = srcX2;
                srcX2 = tmp;
                tmp = dstX1;
                dstX1 = dstX2;
                dstX2 = tmp;
            }
        }

        pBlitter->pDispatch->BlitFramebufferEXT(srcX1, srcY1, srcX2, srcY2,
                    dstX1, dstY1, dstX2, dstY2,
                    GL_COLOR_BUFFER_BIT, filter);
    }

    return VINF_SUCCESS;
}

/* GL_TRIANGLE_FAN */
DECLINLINE(GLfloat*) crBltVtRectTFNormalized(const RTRECT *pRect, uint32_t normalX, uint32_t normalY, GLfloat* pBuff, uint32_t height)
{
    /* going ccw:
     * 1. (left;top)        4. (right;top)
     *        |                    ^
     *        >                    |
     * 2. (left;bottom)  -> 3. (right;bottom) */
    /* xLeft yTop */
    pBuff[0] = ((float)pRect->xLeft)/((float)normalX);
    pBuff[1] = ((float)(height ? height - pRect->yTop : pRect->yTop))/((float)normalY);

    /* xLeft yBottom */
    pBuff[2] = pBuff[0];
    pBuff[3] = ((float)(height ? height - pRect->yBottom : pRect->yBottom))/((float)normalY);

    /* xRight yBottom */
    pBuff[4] = ((float)pRect->xRight)/((float)normalX);
    pBuff[5] = pBuff[3];

    /* xRight yTop */
    pBuff[6] = pBuff[4];
    pBuff[7] = pBuff[1];
    return &pBuff[8];
}

DECLINLINE(GLfloat*) crBltVtRectsTFNormalized(const RTRECT *paRects, uint32_t cRects, uint32_t normalX, uint32_t normalY, GLfloat* pBuff, uint32_t height)
{
    for (uint32_t i = 0; i < cRects; ++i)
    {
        pBuff = crBltVtRectTFNormalized(&paRects[i], normalX, normalY, pBuff, height);
    }
    return pBuff;
}

DECLINLINE(GLint*) crBltVtRectTF(const RTRECT *pRect, uint32_t normalX, uint32_t normalY, GLint* pBuff, uint32_t height)
{
    (void)normalX; (void)normalY;

    /* xLeft yTop */
    pBuff[0] = pRect->xLeft;
    pBuff[1] = height ? height - pRect->yTop : pRect->yTop;

    /* xLeft yBottom */
    pBuff[2] = pBuff[0];
    pBuff[3] = height ? height - pRect->yBottom : pRect->yBottom;

    /* xRight yBottom */
    pBuff[4] = pRect->xRight;
    pBuff[5] = pBuff[3];

    /* xRight yTop */
    pBuff[6] = pBuff[4];
    pBuff[7] = pBuff[1];
    return &pBuff[8];
}

DECLINLINE(GLubyte*) crBltVtFillRectIndicies(GLubyte *pIndex, GLubyte *piBase)
{
    GLubyte iBase = *piBase;
    /* triangle 1 */
    pIndex[0] = iBase;
    pIndex[1] = iBase + 1;
    pIndex[2] = iBase + 2;

    /* triangle 2 */
    pIndex[3] = iBase;
    pIndex[4] = iBase + 2;
    pIndex[5] = iBase + 3;
    *piBase = iBase + 4;
    return pIndex + 6;
}

/* Indexed GL_TRIANGLES */
DECLINLINE(GLfloat*) crBltVtRectITNormalized(const RTRECT *pRect, uint32_t normalX, uint32_t normalY, GLfloat* pBuff, uint32_t height)
{
    GLfloat* ret = crBltVtRectTFNormalized(pRect, normalX, normalY, pBuff, height);
    return ret;
}

DECLINLINE(GLint*) crBltVtRectIT(RTRECT *pRect, uint32_t normalX, uint32_t normalY, GLint* pBuff, GLubyte **ppIndex, GLubyte *piBase, uint32_t height)
{
    GLint* ret = crBltVtRectTF(pRect, normalX, normalY, pBuff, height);

    if (ppIndex)
        *ppIndex = crBltVtFillRectIndicies(*ppIndex, piBase);

    return ret;
}

DECLINLINE(GLuint) crBltVtGetNumVerticiesTF(GLuint cRects)
{
    return cRects * 4;
}

#define crBltVtGetNumVerticiesIT crBltVtGetNumVerticiesTF

DECLINLINE(GLuint) crBltVtGetNumIndiciesIT(GLuint cRects)
{
    return 6 * cRects;
}


static GLfloat* crBltVtRectsITNormalized(const RTRECT *paRects, uint32_t cRects, uint32_t normalX, uint32_t normalY, GLfloat* pBuff, GLubyte **ppIndex, GLubyte *piBase, uint32_t height)
{
    uint32_t i;
    for (i = 0; i < cRects; ++i)
    {
        pBuff = crBltVtRectITNormalized(&paRects[i], normalX, normalY, pBuff, height);
    }


    if (ppIndex)
    {
        GLubyte *pIndex = (GLubyte*)pBuff;
        *ppIndex = pIndex;
        for (i = 0; i < cRects; ++i)
        {
            pIndex = crBltVtFillRectIndicies(pIndex, piBase);
        }
        pBuff = (GLfloat*)pIndex;
    }

    return pBuff;
}

static void *crBltBufGet(PCR_BLITTER_BUFFER pBuffer, GLuint cbBuffer)
{
    if (pBuffer->cbBuffer < cbBuffer)
    {
        if (pBuffer->pvBuffer)
        {
            RTMemFree(pBuffer->pvBuffer);
        }

#ifndef DEBUG_misha
        /* debugging: ensure we calculate proper buffer size */
        cbBuffer += 16;
#endif

        pBuffer->pvBuffer = RTMemAlloc(cbBuffer);
        if (pBuffer->pvBuffer)
            pBuffer->cbBuffer = cbBuffer;
        else
        {
            crWarning("failed to allocate buffer of size %d", cbBuffer);
            pBuffer->cbBuffer = 0;
        }
    }
    return pBuffer->pvBuffer;
}

#endif /* !IN_VMSVGA3D */


static void crBltCheckSetupViewport(PCR_BLITTER pBlitter, const RTRECTSIZE *pDstSize, bool fFBODraw)
{
    bool fUpdateViewport = pBlitter->Flags.CurrentMuralChanged;
    if (   pBlitter->CurrentSetSize.cx != pDstSize->cx
        || pBlitter->CurrentSetSize.cy != pDstSize->cy)
    {
        pBlitter->CurrentSetSize = *pDstSize;
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
#else
        pBlitter->pDispatch->MatrixMode(GL_PROJECTION);
        pBlitter->pDispatch->LoadIdentity();
        pBlitter->pDispatch->Ortho(0, pDstSize->cx, 0, pDstSize->cy, -1, 1);
#endif
        fUpdateViewport = true;
    }

    if (fUpdateViewport)
    {
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
#else
        pBlitter->pDispatch->Viewport(0, 0, pBlitter->CurrentSetSize.cx, pBlitter->CurrentSetSize.cy);
#endif
        pBlitter->Flags.CurrentMuralChanged = 0;
    }

    pBlitter->Flags.LastWasFBODraw = fFBODraw;
}


#ifndef IN_VMSVGA3D

static DECLCALLBACK(int) crBltBlitTexBufImplDraw2D(PCR_BLITTER pBlitter, const VBOXVR_TEXTURE *pSrc, const RTRECT *paSrcRect, const RTRECTSIZE *pDstSize, const RTRECT *paDstRect, uint32_t cRects, uint32_t fFlags)
{
    GLuint normalX, normalY;
    uint32_t srcHeight = (fFlags & CRBLT_F_INVERT_SRC_YCOORDS) ? pSrc->height : 0;
    uint32_t dstHeight = (fFlags & CRBLT_F_INVERT_DST_YCOORDS) ? pDstSize->cy : 0;

    switch (pSrc->target)
    {
        case GL_TEXTURE_2D:
        {
            normalX = pSrc->width;
            normalY = pSrc->height;
            break;
        }

        case GL_TEXTURE_RECTANGLE_ARB:
        {
            normalX = 1;
            normalY = 1;
            break;
        }

        default:
        {
            crWarning("Unsupported texture target 0x%x", pSrc->target);
            return VERR_INVALID_PARAMETER;
        }
    }

    Assert(pSrc->hwid);

    pBlitter->pDispatch->BindTexture(pSrc->target, pSrc->hwid);

    if (cRects == 1)
    {
        /* just optimization to draw a single rect with GL_TRIANGLE_FAN */
        GLfloat *pVerticies;
        GLfloat *pTexCoords;
        GLuint cElements = crBltVtGetNumVerticiesTF(cRects);

        pVerticies = (GLfloat*)crBltBufGet(&pBlitter->Verticies, cElements * 2 * 2 * sizeof (*pVerticies));
        pTexCoords = crBltVtRectsTFNormalized(paDstRect, cRects, 1, 1, pVerticies, dstHeight);
        crBltVtRectsTFNormalized(paSrcRect, cRects, normalX, normalY, pTexCoords, srcHeight);

        pBlitter->pDispatch->EnableClientState(GL_VERTEX_ARRAY);
        pBlitter->pDispatch->VertexPointer(2, GL_FLOAT, 0, pVerticies);

        pBlitter->pDispatch->EnableClientState(GL_TEXTURE_COORD_ARRAY);
        pBlitter->pDispatch->TexCoordPointer(2, GL_FLOAT, 0, pTexCoords);

        pBlitter->pDispatch->Enable(pSrc->target);

        pBlitter->pDispatch->DrawArrays(GL_TRIANGLE_FAN, 0, cElements);

        pBlitter->pDispatch->Disable(pSrc->target);

        pBlitter->pDispatch->DisableClientState(GL_TEXTURE_COORD_ARRAY);
        pBlitter->pDispatch->DisableClientState(GL_VERTEX_ARRAY);
    }
    else
    {
        GLfloat *pVerticies;
        GLfloat *pTexCoords;
        GLubyte *pIndicies;
        GLuint cElements = crBltVtGetNumVerticiesIT(cRects);
        GLuint cIndicies = crBltVtGetNumIndiciesIT(cRects);
        GLubyte iIdxBase = 0;

        pVerticies = (GLfloat*)crBltBufGet(&pBlitter->Verticies, cElements * 2 * 2 * sizeof (*pVerticies) + cIndicies * sizeof (*pIndicies));
        pTexCoords = crBltVtRectsITNormalized(paDstRect, cRects, 1, 1, pVerticies, &pIndicies, &iIdxBase, dstHeight);
        crBltVtRectsITNormalized(paSrcRect, cRects, normalX, normalY, pTexCoords, NULL, NULL, srcHeight);

        pBlitter->pDispatch->EnableClientState(GL_VERTEX_ARRAY);
        pBlitter->pDispatch->VertexPointer(2, GL_FLOAT, 0, pVerticies);

        pBlitter->pDispatch->EnableClientState(GL_TEXTURE_COORD_ARRAY);
        pBlitter->pDispatch->TexCoordPointer(2, GL_FLOAT, 0, pTexCoords);

        pBlitter->pDispatch->Enable(pSrc->target);

        pBlitter->pDispatch->DrawElements(GL_TRIANGLES, cIndicies, GL_UNSIGNED_BYTE, pIndicies);

        pBlitter->pDispatch->Disable(pSrc->target);

        pBlitter->pDispatch->DisableClientState(GL_TEXTURE_COORD_ARRAY);
        pBlitter->pDispatch->DisableClientState(GL_VERTEX_ARRAY);
    }

    pBlitter->pDispatch->BindTexture(pSrc->target, 0);

    return VINF_SUCCESS;
}

static int crBltInitOnMakeCurent(PCR_BLITTER pBlitter)
{
    const char * pszExtension = (const char*)pBlitter->pDispatch->GetString(GL_EXTENSIONS);
    if (crStrstr(pszExtension, "GL_EXT_framebuffer_object"))
    {
        pBlitter->Flags.SupportsFBO = 1;
        pBlitter->pDispatch->GenFramebuffersEXT(1, &pBlitter->idFBO);
        Assert(pBlitter->idFBO);
    }
    else
        crWarning("GL_EXT_framebuffer_object not supported, blitter can only blit to window");

    if (crStrstr(pszExtension, "GL_ARB_pixel_buffer_object"))
        pBlitter->Flags.SupportsPBO = 1;
    else
        crWarning("GL_ARB_pixel_buffer_object not supported");

    /* BlitFramebuffer seems to be buggy on Intel,
     * try always glDrawXxx for now */
    if (!pBlitter->Flags.ForceDrawBlit && crStrstr(pszExtension, "GL_EXT_framebuffer_blit"))
    {
        pBlitter->pfnBlt = crBltBlitTexBufImplFbo;
    }
    else
    {
//        crWarning("GL_EXT_framebuffer_blit not supported, will use Draw functions for blitting, which might be less efficient");
        pBlitter->pfnBlt = crBltBlitTexBufImplDraw2D;
    }

    /* defaults. but just in case */
    pBlitter->pDispatch->MatrixMode(GL_TEXTURE);
    pBlitter->pDispatch->LoadIdentity();
    pBlitter->pDispatch->MatrixMode(GL_MODELVIEW);
    pBlitter->pDispatch->LoadIdentity();

    return VINF_SUCCESS;
}

#endif /* !IN_VMSVGA3D */


void CrBltLeave(PCR_BLITTER pBlitter)
{
    if (!pBlitter->cEnters)
    {
        WARN(("blitter not entered!"));
        return;
    }

    if (--pBlitter->cEnters)
        return;

#ifdef IN_VMSVGA3D
    /** @todo IN_VMSVGA3D */
#else
    if (pBlitter->Flags.SupportsFBO)
    {
        pBlitter->pDispatch->BindFramebufferEXT(GL_FRAMEBUFFER, 0);
        pBlitter->pDispatch->DrawBuffer(GL_BACK);
        pBlitter->pDispatch->ReadBuffer(GL_BACK);
    }

    pBlitter->pDispatch->Flush();

    if (pBlitter->CtxInfo.Base.id)
        pBlitter->pDispatch->MakeCurrent(0, 0, 0);
#endif
}

int CrBltEnter(PCR_BLITTER pBlitter)
{
    if (!pBlitter->CurrentMural.Base.id && pBlitter->CtxInfo.Base.id)
    {
        WARN(("current mural not initialized!"));
        return VERR_INVALID_STATE;
    }

    if (pBlitter->cEnters++)
        return VINF_SUCCESS;

    if (pBlitter->CurrentMural.Base.id) /* <- pBlitter->CurrentMural.Base.id can be null if the blitter is in a "no-context" mode (see comments to BltInit for detail)*/
    {
#ifdef IN_VMSVGA3D
    /** @todo IN_VMSVGA3D */
#else
        pBlitter->pDispatch->MakeCurrent(pBlitter->CurrentMural.Base.id, pBlitter->i32MakeCurrentUserData, pBlitter->CtxInfo.Base.id);
#endif
    }

    if (pBlitter->Flags.Initialized)
        return VINF_SUCCESS;

#ifdef IN_VMSVGA3D
    /** @todo IN_VMSVGA3D */
    int rc = VINF_SUCCESS;
#else
    int rc = crBltInitOnMakeCurent(pBlitter);
#endif
    if (RT_SUCCESS(rc))
    {
        pBlitter->Flags.Initialized = 1;
        return VINF_SUCCESS;
    }

    WARN(("crBltInitOnMakeCurent failed, rc %d", rc));
    CrBltLeave(pBlitter);
    return rc;
}


static void crBltBlitTexBuf(PCR_BLITTER pBlitter, const VBOXVR_TEXTURE *pSrc, const RTRECT *paSrcRects, GLenum enmDstBuff, const RTRECTSIZE *pDstSize, const RTRECT *paDstRects, uint32_t cRects, uint32_t fFlags)
{
#ifdef IN_VMSVGA3D
    /** @todo IN_VMSVGA3D */
#else
    pBlitter->pDispatch->DrawBuffer(enmDstBuff);

    crBltCheckSetupViewport(pBlitter, pDstSize, enmDstBuff == GL_DRAW_FRAMEBUFFER);

    if (!(fFlags & CRBLT_F_NOALPHA))
        pBlitter->pfnBlt(pBlitter, pSrc, paSrcRects, pDstSize, paDstRects, cRects, fFlags);
    else
    {
        int rc = pBlitter->Flags.ShadersGloal
               ? CrGlslProgUseNoAlpha(pBlitter->pGlslCache, pSrc->target)
               : CrGlslProgUseGenNoAlpha(&pBlitter->LocalGlslCache, pSrc->target);

        if (!RT_SUCCESS(rc))
        {
            crWarning("Failed to use no-alpha program rc %d!, falling back to default blit", rc);
            pBlitter->pfnBlt(pBlitter, pSrc, paSrcRects, pDstSize, paDstRects, cRects, fFlags);
            return;
        }

        /* since we use shaders, we need to use draw commands rather than framebuffer blits.
         * force using draw-based blitting */
        crBltBlitTexBufImplDraw2D(pBlitter, pSrc, paSrcRects, pDstSize, paDstRects, cRects, fFlags);

        Assert(pBlitter->Flags.ShadersGloal || &pBlitter->LocalGlslCache == pBlitter->pGlslCache);

        CrGlslProgClear(pBlitter->pGlslCache);
    }
#endif
}

void CrBltCheckUpdateViewport(PCR_BLITTER pBlitter)
{
    RTRECTSIZE DstSize = {pBlitter->CurrentMural.width, pBlitter->CurrentMural.height};
    crBltCheckSetupViewport(pBlitter, &DstSize, false);
}

void CrBltBlitTexMural(PCR_BLITTER pBlitter, bool fBb, const VBOXVR_TEXTURE *pSrc, const RTRECT *paSrcRects, const RTRECT *paDstRects, uint32_t cRects, uint32_t fFlags)
{
    if (!CrBltIsEntered(pBlitter))
    {
        WARN(("CrBltBlitTexMural: blitter not entered"));
        return;
    }

    RTRECTSIZE DstSize = {pBlitter->CurrentMural.width, pBlitter->CurrentMural.height};

#ifdef IN_VMSVGA3D
    /** @todo IN_VMSVGA3D */
#else
    pBlitter->pDispatch->BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
#endif

    crBltBlitTexBuf(pBlitter, pSrc, paSrcRects, fBb ? GL_BACK : GL_FRONT, &DstSize, paDstRects, cRects, fFlags);
}


#ifndef IN_VMSVGA3D

void CrBltBlitTexTex(PCR_BLITTER pBlitter, const VBOXVR_TEXTURE *pSrc, const RTRECT *pSrcRect, const VBOXVR_TEXTURE *pDst, const RTRECT *pDstRect, uint32_t cRects, uint32_t fFlags)
{
    if (!CrBltIsEntered(pBlitter))
    {
        WARN(("CrBltBlitTexTex: blitter not entered"));
        return;
    }

    RTRECTSIZE DstSize = {(uint32_t)pDst->width, (uint32_t)pDst->height};

    pBlitter->pDispatch->BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, pBlitter->idFBO);

    /** @todo mag/min filters ? */

    pBlitter->pDispatch->FramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pDst->target, pDst->hwid, 0);

//    pBlitter->pDispatch->FramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
//    pBlitter->pDispatch->FramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

    crBltBlitTexBuf(pBlitter, pSrc, pSrcRect, GL_DRAW_FRAMEBUFFER, &DstSize, pDstRect, cRects, fFlags);

    pBlitter->pDispatch->FramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pDst->target, 0, 0);
}

void CrBltPresent(PCR_BLITTER pBlitter)
{
    if (!CrBltIsEntered(pBlitter))
    {
        WARN(("CrBltPresent: blitter not entered"));
        return;
    }

    if (pBlitter->CtxInfo.Base.visualBits & CR_DOUBLE_BIT)
        pBlitter->pDispatch->SwapBuffers(pBlitter->CurrentMural.Base.id, 0);
    else
        pBlitter->pDispatch->Flush();
}

static int crBltImgInitBaseForTex(const VBOXVR_TEXTURE *pSrc, CR_BLITTER_IMG *pDst, GLenum enmFormat)
{
    memset(pDst, 0, sizeof (*pDst));
    if (enmFormat != GL_RGBA
            && enmFormat != GL_BGRA)
    {
        WARN(("unsupported format 0x%x", enmFormat));
        return VERR_NOT_IMPLEMENTED;
    }

    uint32_t bpp = 32;

    uint32_t pitch = ((bpp * pSrc->width) + 7) >> 3;
    uint32_t cbData = pitch * pSrc->height;
    pDst->cbData = cbData;
    pDst->enmFormat = enmFormat;
    pDst->width = pSrc->width;
    pDst->height = pSrc->height;
    pDst->bpp = bpp;
    pDst->pitch = pitch;
    return VINF_SUCCESS;
}

static int crBltImgCreateForTex(const VBOXVR_TEXTURE *pSrc, CR_BLITTER_IMG *pDst, GLenum enmFormat)
{
    int rc = crBltImgInitBaseForTex(pSrc, pDst, enmFormat);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crBltImgInitBaseForTex failed rc %d", rc);
        return rc;
    }

    uint32_t cbData = pDst->cbData;
    pDst->pvData = RTMemAllocZ(cbData);
    if (!pDst->pvData)
    {
        crWarning("RTMemAlloc failed");
        return VERR_NO_MEMORY;
    }

#ifdef DEBUG_misha
    {
        char *pTmp = (char*)pDst->pvData;
        for (uint32_t i = 0; i < cbData; ++i)
        {
            pTmp[i] = (char)((1 << i) % 255);
        }
    }
#endif
    return VINF_SUCCESS;
}

VBOXBLITTERDECL(int) CrBltImgGetTex(PCR_BLITTER pBlitter, const VBOXVR_TEXTURE *pSrc, GLenum enmFormat, CR_BLITTER_IMG *pDst)
{
    if (!CrBltIsEntered(pBlitter))
    {
        WARN(("CrBltImgGetTex: blitter not entered"));
        return VERR_INVALID_STATE;
    }

    int rc = crBltImgCreateForTex(pSrc, pDst, enmFormat);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crBltImgCreateForTex failed, rc %d", rc);
        return rc;
    }
    pBlitter->pDispatch->BindTexture(pSrc->target, pSrc->hwid);

#ifdef DEBUG_misha
    {
        GLint width = 0, height = 0, depth = 0;
        pBlitter->pDispatch->GetTexLevelParameteriv(pSrc->target, 0, GL_TEXTURE_WIDTH, &width);
        pBlitter->pDispatch->GetTexLevelParameteriv(pSrc->target, 0, GL_TEXTURE_HEIGHT, &height);
        pBlitter->pDispatch->GetTexLevelParameteriv(pSrc->target, 0, GL_TEXTURE_DEPTH, &depth);

        Assert(width == pSrc->width);
        Assert(height == pSrc->height);
//        Assert(depth == pSrc->depth);
    }
#endif

    pBlitter->pDispatch->GetTexImage(pSrc->target, 0, enmFormat, GL_UNSIGNED_BYTE, pDst->pvData);

    pBlitter->pDispatch->BindTexture(pSrc->target, 0);
    return VINF_SUCCESS;
}

VBOXBLITTERDECL(int) CrBltImgGetMural(PCR_BLITTER pBlitter, bool fBb, CR_BLITTER_IMG *pDst)
{
    (void)fBb; (void)pDst;
    if (!CrBltIsEntered(pBlitter))
    {
        WARN(("CrBltImgGetMural: blitter not entered"));
        return VERR_INVALID_STATE;
    }

    WARN(("NOT IMPLEMENTED"));
    return VERR_NOT_IMPLEMENTED;
}

VBOXBLITTERDECL(void) CrBltImgFree(PCR_BLITTER pBlitter, CR_BLITTER_IMG *pDst)
{
    if (!CrBltIsEntered(pBlitter))
    {
        WARN(("CrBltImgFree: blitter not entered"));
        return;
    }

    if (pDst->pvData)
    {
        RTMemFree(pDst->pvData);
        pDst->pvData = NULL;
    }
}


VBOXBLITTERDECL(bool) CrGlslIsSupported(CR_GLSL_CACHE *pCache)
{
    if (pCache->iGlVersion == 0)
    {
        const char * pszStr = (const char*)pCache->pDispatch->GetString(GL_VERSION);
        pCache->iGlVersion = crStrParseGlVersion(pszStr);
        if (pCache->iGlVersion <= 0)
        {
            crWarning("crStrParseGlVersion returned %d", pCache->iGlVersion);
            pCache->iGlVersion = -1;
        }
    }

    if (pCache->iGlVersion >= CR_GLVERSION_COMPOSE(2, 0, 0))
        return true;

    crWarning("GLSL unsuported, gl version %d", pCache->iGlVersion);

    /** @todo we could also check for GL_ARB_shader_objects and GL_ARB_fragment_shader,
     * but seems like chromium does not support properly gl*Object versions of shader functions used with those extensions */
    return false;
}

#define CR_GLSL_STR_V_120 "#version 120\n"
#define CR_GLSL_STR_EXT_TR "#extension GL_ARB_texture_rectangle : enable\n"
#define CR_GLSL_STR_2D "2D"
#define CR_GLSL_STR_2DRECT "2DRect"

#define CR_GLSL_PATTERN_FS_NOALPHA(_ver, _ext, _tex) \
        _ver \
        _ext \
        "uniform sampler" _tex " sampler0;\n" \
        "void main()\n" \
        "{\n" \
            "vec2 srcCoord = vec2(gl_TexCoord[0]);\n" \
            "gl_FragData[0].xyz = (texture" _tex "(sampler0, srcCoord).xyz);\n" \
            "gl_FragData[0].w = 1.0;\n" \
        "}\n"

static const char* crGlslGetFsStringNoAlpha(CR_GLSL_CACHE *pCache, GLenum enmTexTarget)
{
    if (!CrGlslIsSupported(pCache))
    {
        crWarning("CrGlslIsSupported is false");
        return NULL;
    }

    if (pCache->iGlVersion >= CR_GLVERSION_COMPOSE(2, 1, 0))
    {
        if (enmTexTarget == GL_TEXTURE_2D)
            return CR_GLSL_PATTERN_FS_NOALPHA(CR_GLSL_STR_V_120, "", CR_GLSL_STR_2D);
        else if (enmTexTarget == GL_TEXTURE_RECTANGLE_ARB)
            return CR_GLSL_PATTERN_FS_NOALPHA(CR_GLSL_STR_V_120, CR_GLSL_STR_EXT_TR, CR_GLSL_STR_2DRECT);

        crWarning("invalid enmTexTarget %#x", enmTexTarget);
        return NULL;
    }
    else if (pCache->iGlVersion >= CR_GLVERSION_COMPOSE(2, 0, 0))
    {
        if (enmTexTarget == GL_TEXTURE_2D)
            return CR_GLSL_PATTERN_FS_NOALPHA("", "", CR_GLSL_STR_2D);
        else if (enmTexTarget == GL_TEXTURE_RECTANGLE_ARB)
            return CR_GLSL_PATTERN_FS_NOALPHA("", CR_GLSL_STR_EXT_TR, CR_GLSL_STR_2DRECT);

        crWarning("invalid enmTexTarget %#x", enmTexTarget);
        return NULL;
    }

    crError("crGlslGetFsStringNoAlpha: we should not be here!");
    return NULL;
}

static int crGlslProgGenNoAlpha(CR_GLSL_CACHE *pCache, GLenum enmTexTarget, GLuint *puiProgram)
{
    *puiProgram = 0;

    const char*pStrFsShader = crGlslGetFsStringNoAlpha(pCache, enmTexTarget);
    if (!pStrFsShader)
    {
        crWarning("crGlslGetFsStringNoAlpha failed");
        return VERR_NOT_SUPPORTED;
    }

    int rc = VINF_SUCCESS;
    GLchar * pBuf = NULL;
    GLuint uiProgram = 0;
    GLint iUniform = -1;
    GLuint uiShader = pCache->pDispatch->CreateShader(GL_FRAGMENT_SHADER);
    if (!uiShader)
    {
        crWarning("CreateShader failed");
        return VERR_NOT_SUPPORTED;
    }

    pCache->pDispatch->ShaderSource(uiShader, 1, &pStrFsShader, NULL);

    pCache->pDispatch->CompileShader(uiShader);

    GLint compiled = 0;
    pCache->pDispatch->GetShaderiv(uiShader, GL_COMPILE_STATUS, &compiled);

#ifndef DEBUG_misha
    if(!compiled)
#endif
    {
        if (!pBuf)
            pBuf = (GLchar *)RTMemAlloc(16300);
        pCache->pDispatch->GetShaderInfoLog(uiShader, 16300, NULL, pBuf);
#ifdef DEBUG_misha
        if (compiled)
            crDebug("compile success:\n-------------------\n%s\n--------\n", pBuf);
        else
#endif
        {
            crWarning("compile FAILURE:\n-------------------\n%s\n--------\n", pBuf);
            rc = VERR_NOT_SUPPORTED;
            goto end;
        }
    }

    Assert(compiled);

    uiProgram = pCache->pDispatch->CreateProgram();
    if (!uiProgram)
    {
        rc = VERR_NOT_SUPPORTED;
        goto end;
    }

    pCache->pDispatch->AttachShader(uiProgram, uiShader);

    pCache->pDispatch->LinkProgram(uiProgram);

    GLint linked;
    pCache->pDispatch->GetProgramiv(uiProgram, GL_LINK_STATUS, &linked);
#ifndef DEBUG_misha
    if(!linked)
#endif
    {
        if (!pBuf)
            pBuf = (GLchar *)RTMemAlloc(16300);
        pCache->pDispatch->GetProgramInfoLog(uiProgram, 16300, NULL, pBuf);
#ifdef DEBUG_misha
        if (linked)
            crDebug("link success:\n-------------------\n%s\n--------\n", pBuf);
        else
#endif
        {
            crWarning("link FAILURE:\n-------------------\n%s\n--------\n", pBuf);
            rc = VERR_NOT_SUPPORTED;
            goto end;
        }
    }

    Assert(linked);

    iUniform = pCache->pDispatch->GetUniformLocation(uiProgram, "sampler0");
    if (iUniform == -1)
    {
        crWarning("GetUniformLocation failed for sampler0");
    }
    else
    {
        pCache->pDispatch->Uniform1i(iUniform, 0);
    }

    *puiProgram = uiProgram;

    /* avoid end finalizer from cleaning it */
    uiProgram = 0;

    end:
    if (uiShader)
        pCache->pDispatch->DeleteShader(uiShader);
    if (uiProgram)
        pCache->pDispatch->DeleteProgram(uiProgram);
    if (pBuf)
        RTMemFree(pBuf);
    return rc;
}

DECLINLINE(GLuint) crGlslProgGetNoAlpha(const CR_GLSL_CACHE *pCache, GLenum enmTexTarget)
{
    switch (enmTexTarget)
    {
        case GL_TEXTURE_2D:
            return  pCache->uNoAlpha2DProg;
        case GL_TEXTURE_RECTANGLE_ARB:
            return pCache->uNoAlpha2DRectProg;
        default:
            crWarning("invalid tex enmTexTarget %#x", enmTexTarget);
            return 0;
    }
}

DECLINLINE(GLuint*) crGlslProgGetNoAlphaPtr(CR_GLSL_CACHE *pCache, GLenum enmTexTarget)
{
    switch (enmTexTarget)
    {
        case GL_TEXTURE_2D:
            return  &pCache->uNoAlpha2DProg;
        case GL_TEXTURE_RECTANGLE_ARB:
            return &pCache->uNoAlpha2DRectProg;
        default:
            crWarning("invalid tex enmTexTarget %#x", enmTexTarget);
            return NULL;
    }
}

VBOXBLITTERDECL(int) CrGlslProgGenNoAlpha(CR_GLSL_CACHE *pCache, GLenum enmTexTarget)
{
    GLuint*puiProgram = crGlslProgGetNoAlphaPtr(pCache, enmTexTarget);
    if (!puiProgram)
        return VERR_INVALID_PARAMETER;

    if (*puiProgram)
        return VINF_SUCCESS;

    return crGlslProgGenNoAlpha(pCache, enmTexTarget, puiProgram);
}

VBOXBLITTERDECL(int) CrGlslProgGenAllNoAlpha(CR_GLSL_CACHE *pCache)
{
    int rc = CrGlslProgGenNoAlpha(pCache, GL_TEXTURE_2D);
    if (!RT_SUCCESS(rc))
    {
        crWarning("CrGlslProgGenNoAlpha GL_TEXTURE_2D failed rc %d", rc);
        return rc;
    }

    rc = CrGlslProgGenNoAlpha(pCache, GL_TEXTURE_RECTANGLE_ARB);
    if (!RT_SUCCESS(rc))
    {
        crWarning("CrGlslProgGenNoAlpha GL_TEXTURE_RECTANGLE failed rc %d", rc);
        return rc;
    }

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(void) CrGlslProgClear(const CR_GLSL_CACHE *pCache)
{
    pCache->pDispatch->UseProgram(0);
}

VBOXBLITTERDECL(int) CrGlslProgUseNoAlpha(const CR_GLSL_CACHE *pCache, GLenum enmTexTarget)
{
    GLuint uiProg = crGlslProgGetNoAlpha(pCache, enmTexTarget);
    if (!uiProg)
    {
        crWarning("request to use inexistent program!");
        return VERR_INVALID_STATE;
    }

    Assert(uiProg);

    pCache->pDispatch->UseProgram(uiProg);

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(int) CrGlslProgUseGenNoAlpha(CR_GLSL_CACHE *pCache, GLenum enmTexTarget)
{
    GLuint uiProg = crGlslProgGetNoAlpha(pCache, enmTexTarget);
    if (!uiProg)
    {
        int rc = CrGlslProgGenNoAlpha(pCache, enmTexTarget);
        if (!RT_SUCCESS(rc))
        {
            crWarning("CrGlslProgGenNoAlpha failed, rc %d", rc);
            return rc;
        }

        uiProg = crGlslProgGetNoAlpha(pCache, enmTexTarget);
        CRASSERT(uiProg);
    }

    Assert(uiProg);

    pCache->pDispatch->UseProgram(uiProg);

    return VINF_SUCCESS;
}

#endif /* !IN_VMSVGA3D */

VBOXBLITTERDECL(bool) CrGlslNeedsCleanup(const CR_GLSL_CACHE *pCache)
{
    return pCache->uNoAlpha2DProg || pCache->uNoAlpha2DRectProg;
}

VBOXBLITTERDECL(void) CrGlslCleanup(CR_GLSL_CACHE *pCache)
{
    if (pCache->uNoAlpha2DProg)
    {
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
#else
        pCache->pDispatch->DeleteProgram(pCache->uNoAlpha2DProg);
#endif
        pCache->uNoAlpha2DProg = 0;
    }

    if (pCache->uNoAlpha2DRectProg)
    {
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
#else
        pCache->pDispatch->DeleteProgram(pCache->uNoAlpha2DRectProg);
#endif
        pCache->uNoAlpha2DRectProg = 0;
    }
}

VBOXBLITTERDECL(void) CrGlslTerm(CR_GLSL_CACHE *pCache)
{
    CRASSERT(!CrGlslNeedsCleanup(pCache));

    CrGlslCleanup(pCache);

    /* sanity */
    memset(pCache, 0, sizeof (*pCache));
}

#ifndef IN_VMSVGA3D

/*TdBlt*/
static void crTdBltCheckPBO(PCR_TEXDATA pTex)
{
    if (pTex->idPBO)
        return;

    PCR_BLITTER pBlitter = pTex->pBlitter;

    if (!pBlitter->Flags.SupportsPBO)
        return;

    pBlitter->pDispatch->GenBuffersARB(1, &pTex->idPBO);
    if (!pTex->idPBO)
    {
        crWarning("PBO create failed");
        return;
    }

    pBlitter->pDispatch->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pTex->idPBO);
    pBlitter->pDispatch->BufferDataARB(GL_PIXEL_PACK_BUFFER_ARB,
                pTex->Tex.width*pTex->Tex.height*4,
                0, GL_STREAM_READ_ARB);
    pBlitter->pDispatch->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
}

static uint32_t crTdBltTexCreate(PCR_BLITTER pBlitter, uint32_t width, uint32_t height, GLenum enmTarget)
{
    uint32_t tex = 0;
    pBlitter->pDispatch->GenTextures(1, &tex);
    if (!tex)
    {
        crWarning("Tex create failed");
        return 0;
    }

    pBlitter->pDispatch->BindTexture(enmTarget, tex);
    pBlitter->pDispatch->TexParameteri(enmTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    pBlitter->pDispatch->TexParameteri(enmTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    pBlitter->pDispatch->TexParameteri(enmTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    pBlitter->pDispatch->TexParameteri(enmTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    pBlitter->pDispatch->TexImage2D(enmTarget, 0, GL_RGBA8,
            width, height,
            0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);


    /*Restore gl state*/
    pBlitter->pDispatch->BindTexture(enmTarget, 0);

    return tex;
}

int crTdBltCheckInvertTex(PCR_TEXDATA pTex)
{
    if (pTex->idInvertTex)
        return VINF_SUCCESS;

    pTex->idInvertTex = crTdBltTexCreate(pTex->pBlitter, pTex->Tex.width, pTex->Tex.height, pTex->Tex.target);
    if (!pTex->idInvertTex)
    {
        crWarning("Invert Tex create failed");
        return VERR_GENERAL_FAILURE;
    }
    return VINF_SUCCESS;
}

#endif /* !IN_VMSVGA3D */


void crTdBltImgRelease(PCR_TEXDATA pTex)
{
    pTex->Flags.DataValid = 0;
}

void crTdBltImgFree(PCR_TEXDATA pTex)
{
    if (!pTex->Img.pvData)
    {
        Assert(!pTex->Flags.DataValid);
        return;
    }

    crTdBltImgRelease(pTex);

    Assert(!pTex->Flags.DataValid);


    if (pTex->idPBO)
    {
        PCR_BLITTER pBlitter = pTex->pBlitter;

        Assert(CrBltIsEntered(pBlitter));
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
#else
        pBlitter->pDispatch->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pTex->idPBO);
        pBlitter->pDispatch->UnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
        pBlitter->pDispatch->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
#endif
    }
    else
    {
        Assert(pTex->Img.pvData);
        RTMemFree(pTex->Img.pvData);
    }

    pTex->Img.pvData = NULL;
}


#ifndef IN_VMSVGA3D

int crTdBltImgAcquire(PCR_TEXDATA pTex, GLenum enmFormat, bool fInverted)
{
    void *pvData = pTex->Img.pvData;
    Assert(!pTex->Flags.DataValid);
    int rc = crBltImgInitBaseForTex(&pTex->Tex, &pTex->Img, enmFormat);
    if (!RT_SUCCESS(rc))
    {
        WARN(("crBltImgInitBaseForTex failed rc %d", rc));
        return rc;
    }

    PCR_BLITTER pBlitter = pTex->pBlitter;
    Assert(CrBltIsEntered(pBlitter));
    pBlitter->pDispatch->BindTexture(pTex->Tex.target, fInverted ? pTex->idInvertTex : pTex->Tex.hwid);

    pBlitter->pDispatch->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pTex->idPBO);

    if (pvData)
    {
        if (pTex->idPBO)
        {
            pBlitter->pDispatch->UnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
            pvData = NULL;

        }
    }
    else
    {
        if (!pTex->idPBO)
        {
            pvData = RTMemAlloc(4*pTex->Tex.width*pTex->Tex.height);
            if (!pvData)
            {
                WARN(("Out of memory in crTdBltImgAcquire"));
                pBlitter->pDispatch->BindTexture(pTex->Tex.target, 0);
                return VERR_NO_MEMORY;
            }
        }
    }

    Assert(!pvData == !!pTex->idPBO);

    /*read the texture, note pixels are NULL for PBO case as it's offset in the buffer*/
    pBlitter->pDispatch->GetTexImage(GL_TEXTURE_2D, 0, enmFormat, GL_UNSIGNED_BYTE, pvData);

    /*restore gl state*/
    pBlitter->pDispatch->BindTexture(pTex->Tex.target, 0);

    if (pTex->idPBO)
    {
        pvData = pBlitter->pDispatch->MapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
        if (!pvData)
        {
            WARN(("Failed to MapBuffer in CrHlpGetTexImage"));
            return VERR_GENERAL_FAILURE;
        }

        pBlitter->pDispatch->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }

    Assert(pvData);
    pTex->Img.pvData = pvData;
    pTex->Flags.DataValid = 1;
    pTex->Flags.DataInverted = fInverted;
    return VINF_SUCCESS;
}

#endif /* !IN_VMSVGA3D */


/* release the texture data, the data remains cached in the CR_TEXDATA object until it is discarded with CrTdBltDataInvalidateNe or CrTdBltDataCleanup */
VBOXBLITTERDECL(int) CrTdBltDataRelease(PCR_TEXDATA pTex)
{
    if (!pTex->Flags.Entered)
    {
        WARN(("tex not entered"));
        return VERR_INVALID_STATE;
    }

    if (!pTex->Flags.DataAcquired)
    {
        WARN(("Data NOT acquired"));
        return VERR_INVALID_STATE;
    }

    Assert(pTex->Img.pvData);
    Assert(pTex->Flags.DataValid);

    pTex->Flags.DataAcquired = 0;

    return VINF_SUCCESS;
}

static void crTdBltDataFree(PCR_TEXDATA pTex)
{
    crTdBltImgFree(pTex);

    if (pTex->pScaledCache)
        CrTdBltDataFreeNe(pTex->pScaledCache);
}

/* discard the texture data cached with previous CrTdBltDataAcquire.
 * Must be called wit data released (CrTdBltDataRelease) */
VBOXBLITTERDECL(int) CrTdBltDataFree(PCR_TEXDATA pTex)
{
    if (!pTex->Flags.Entered)
    {
        WARN(("tex not entered"));
        return VERR_INVALID_STATE;
    }

    crTdBltDataFree(pTex);

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(void) CrTdBltDataInvalidateNe(PCR_TEXDATA pTex)
{
    crTdBltImgRelease(pTex);

    if (pTex->pScaledCache)
        CrTdBltDataInvalidateNe(pTex->pScaledCache);
}

VBOXBLITTERDECL(int) CrTdBltDataFreeNe(PCR_TEXDATA pTex)
{
    if (!pTex->Img.pvData)
        return VINF_SUCCESS;

    bool fEntered = false;
    if (pTex->idPBO)
    {
        int rc = CrTdBltEnter(pTex);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }

        fEntered = true;
    }

    crTdBltDataFree(pTex);

    if (fEntered)
        CrTdBltLeave(pTex);

    return VINF_SUCCESS;
}

static void crTdBltSdCleanupCacheNe(PCR_TEXDATA pTex)
{
    if (pTex->pScaledCache)
    {
        CrTdBltDataCleanupNe(pTex->pScaledCache);
        CrTdRelease(pTex->pScaledCache);
        pTex->pScaledCache = NULL;
    }
}

static void crTdBltDataCleanup(PCR_TEXDATA pTex)
{
    crTdBltImgFree(pTex);

    PCR_BLITTER pBlitter = pTex->pBlitter;

    if (pTex->idPBO)
    {
        Assert(CrBltIsEntered(pBlitter));
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
#else
        pBlitter->pDispatch->DeleteBuffersARB(1, &pTex->idPBO);
#endif
        pTex->idPBO = 0;
    }

    if (pTex->idInvertTex)
    {
        Assert(CrBltIsEntered(pBlitter));
#ifdef IN_VMSVGA3D
        /** @todo IN_VMSVGA3D */
#else
        pBlitter->pDispatch->DeleteTextures(1, &pTex->idInvertTex);
#endif
        pTex->idInvertTex = 0;
    }

    crTdBltSdCleanupCacheNe(pTex);
}

/* does same as CrTdBltDataFree, and in addition cleans up */
VBOXBLITTERDECL(int) CrTdBltDataCleanup(PCR_TEXDATA pTex)
{
    if (!pTex->Flags.Entered)
    {
        WARN(("tex not entered"));
        return VERR_INVALID_STATE;
    }

    crTdBltDataCleanup(pTex);

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(int) CrTdBltDataCleanupNe(PCR_TEXDATA pTex)
{
    bool fEntered = false;
    if (pTex->idPBO || pTex->idInvertTex)
    {
        int rc = CrTdBltEnter(pTex);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }

        fEntered = true;
    }

    crTdBltDataCleanup(pTex);

    if (fEntered)
        CrTdBltLeave(pTex);

    return VINF_SUCCESS;
}


#ifndef IN_VMSVGA3D

/* acquire the texture data, returns the cached data in case it is cached.
 * the data remains cached in the CR_TEXDATA object until it is discarded with CrTdBltDataFree or CrTdBltDataCleanup.
 * */
VBOXBLITTERDECL(int) CrTdBltDataAcquire(PCR_TEXDATA pTex, GLenum enmFormat, bool fInverted, const CR_BLITTER_IMG**ppImg)
{
    if (!pTex->Flags.Entered)
    {
        WARN(("tex not entered"));
        return VERR_INVALID_STATE;
    }

    if (pTex->Flags.DataAcquired)
    {
        WARN(("Data acquired already"));
        return VERR_INVALID_STATE;
    }

    if (pTex->Flags.DataValid && pTex->Img.enmFormat == enmFormat && !pTex->Flags.DataInverted == !fInverted)
    {
        Assert(pTex->Img.pvData);
        *ppImg = &pTex->Img;
        pTex->Flags.DataAcquired = 1;
        return VINF_SUCCESS;
    }

    crTdBltImgRelease(pTex);

    crTdBltCheckPBO(pTex);

    int rc;

    if (fInverted)
    {
        rc = crTdBltCheckInvertTex(pTex);
        if (!RT_SUCCESS(rc))
        {
            WARN(("crTdBltCheckInvertTex failed rc %d", rc));
            return rc;
        }

        RTRECT SrcRect, DstRect;
        VBOXVR_TEXTURE InvertTex;

        InvertTex = pTex->Tex;
        InvertTex.hwid = pTex->idInvertTex;

        SrcRect.xLeft = 0;
        SrcRect.yTop = InvertTex.height;
        SrcRect.xRight = InvertTex.width;
        SrcRect.yBottom = 0;

        DstRect.xLeft = 0;
        DstRect.yTop = 0;
        DstRect.xRight = InvertTex.width;
        DstRect.yBottom = InvertTex.height;

        CrBltBlitTexTex(pTex->pBlitter, &pTex->Tex, &SrcRect, &InvertTex, &DstRect, 1, 0);
    }

    rc = crTdBltImgAcquire(pTex, enmFormat, fInverted);
    if (!RT_SUCCESS(rc))
    {
        WARN(("crTdBltImgAcquire failed rc %d", rc));
        return rc;
    }

    Assert(pTex->Img.pvData);
    *ppImg = &pTex->Img;
    pTex->Flags.DataAcquired = 1;

    return VINF_SUCCESS;
}

DECLINLINE(void) crTdResize(PCR_TEXDATA pTex, const VBOXVR_TEXTURE *pVrTex)
{
    crTdBltDataCleanup(pTex);

    pTex->Tex = *pVrTex;
}

static DECLCALLBACK(void) ctTdBltSdReleased(struct CR_TEXDATA *pTexture)
{
    PCR_BLITTER pBlitter = pTexture->pBlitter;

    int rc = CrBltEnter(pBlitter);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrBltEnter failed, rc %d", rc));
        return;
    }

    CrTdBltDataCleanupNe(pTexture);

    pBlitter->pDispatch->DeleteTextures(1, &pTexture->Tex.hwid);

    CrBltLeave(pBlitter);

    RTMemFree(pTexture);
}

static int ctTdBltSdCreate(PCR_BLITTER pBlitter, uint32_t width, uint32_t height, GLenum enmTarget, PCR_TEXDATA *ppScaledCache)
{
    PCR_TEXDATA pScaledCache;

    Assert(CrBltIsEntered(pBlitter));

    *ppScaledCache = NULL;

    pScaledCache = (PCR_TEXDATA)RTMemAlloc(sizeof (*pScaledCache));
    if (!pScaledCache)
    {
        WARN(("RTMemAlloc failed"));
        return VERR_NO_MEMORY;
    }

    VBOXVR_TEXTURE Tex;
    Tex.width = width;
    Tex.height = height;
    Tex.target = enmTarget;
    Tex.hwid = crTdBltTexCreate(pBlitter, width, height, enmTarget);
    if (!Tex.hwid)
    {
        WARN(("Tex create failed"));
        RTMemFree(pScaledCache);
        return VERR_GENERAL_FAILURE;
    }

    CrTdInit(pScaledCache, &Tex, pBlitter, ctTdBltSdReleased);

    *ppScaledCache = pScaledCache;

    return VINF_SUCCESS;
}

static int ctTdBltSdGet(PCR_TEXDATA pTex, uint32_t width, uint32_t height, PCR_TEXDATA *ppScaledCache)
{
    Assert(CrBltIsEntered(pTex->pBlitter));

    PCR_TEXDATA pScaledCache;

    *ppScaledCache = NULL;

    if (!pTex->pScaledCache)
    {
        int rc = ctTdBltSdCreate(pTex->pBlitter, width, height, pTex->Tex.target, &pScaledCache);
        if (!RT_SUCCESS(rc))
        {
            WARN(("ctTdBltSdCreate failed %d", rc));
            return rc;
        }

        pTex->pScaledCache = pScaledCache;
    }
    else
    {
        int cmp = pTex->pScaledCache->Tex.width - width;
        if (cmp <= 0)
            cmp = pTex->pScaledCache->Tex.height - height;

        if (!cmp)
            pScaledCache = pTex->pScaledCache;
        else if (cmp < 0) /* current cache is "less" than the requested */
        {
            int rc = ctTdBltSdCreate(pTex->pBlitter, width, height, pTex->Tex.target, &pScaledCache);
            if (!RT_SUCCESS(rc))
            {
                WARN(("ctTdBltSdCreate failed %d", rc));
                return rc;
            }

            pScaledCache->pScaledCache = pTex->pScaledCache;
            pTex->pScaledCache = pScaledCache;
        }
        else /* cmp > 0 */
        {
            int rc = ctTdBltSdGet(pTex->pScaledCache, width, height, &pScaledCache);
            if (!RT_SUCCESS(rc))
            {
                WARN(("ctTdBltSdGet failed %d", rc));
                return rc;
            }
        }
    }

    Assert(pScaledCache);

#if 0
    {
        VBOXVR_TEXTURE Tex;
        Tex.width = width;
        Tex.height = height;
        Tex.target = pTex->Tex.target;
        Tex.hwid = crTdBltTexCreate(pTex, width, height);
        if (!Tex.hwid)
        {
            WARN(("Tex create failed"));
            return VERR_GENERAL_FAILURE;
        }

        pTex->pBlitter->pDispatch->DeleteTextures(1, &pTex->pScaledCache->Tex.hwid);

        crTdResize(pTex->pScaledCache, &Tex);
    }
#endif

    *ppScaledCache = pScaledCache;
    return VINF_SUCCESS;
}

static int ctTdBltSdGetUpdated(PCR_TEXDATA pTex, uint32_t width, uint32_t height, PCR_TEXDATA *ppScaledCache)
{
    PCR_TEXDATA pScaledCache;

    *ppScaledCache = NULL;
    int rc = ctTdBltSdGet(pTex, width, height, &pScaledCache);
    if (!RT_SUCCESS(rc))
    {
        WARN(("ctTdBltSdGet failed %d", rc));
        return rc;
    }

    Assert(width == (uint32_t)pScaledCache->Tex.width);
    Assert(height == (uint32_t)pScaledCache->Tex.height);

    if (!pScaledCache->Flags.DataValid)
    {
        RTRECT SrcRect, DstRect;

        SrcRect.xLeft = 0;
        SrcRect.yTop = 0;
        SrcRect.xRight = pTex->Tex.width;
        SrcRect.yBottom = pTex->Tex.height;

        DstRect.xLeft = 0;
        DstRect.yTop = 0;
        DstRect.xRight = width;
        DstRect.yBottom = height;

        CrBltBlitTexTex(pTex->pBlitter, &pTex->Tex, &SrcRect, &pScaledCache->Tex, &DstRect, 1, 0);
    }

    *ppScaledCache = pScaledCache;

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(int) CrTdBltDataAcquireScaled(PCR_TEXDATA pTex, GLenum enmFormat, bool fInverted, uint32_t width, uint32_t height, const CR_BLITTER_IMG**ppImg)
{
    if ((uint32_t)pTex->Tex.width == width && (uint32_t)pTex->Tex.height == height)
        return CrTdBltDataAcquire(pTex, enmFormat, fInverted, ppImg);

    if (!pTex->Flags.Entered)
    {
        WARN(("tex not entered"));
        return VERR_INVALID_STATE;
    }

    PCR_TEXDATA pScaledCache;

    int rc = ctTdBltSdGetUpdated(pTex, width, height, &pScaledCache);
    if (!RT_SUCCESS(rc))
    {
        WARN(("ctTdBltSdGetUpdated failed rc %d", rc));
        return rc;
    }

    rc = CrTdBltEnter(pScaledCache);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrTdBltEnter failed rc %d", rc));
        return rc;
    }

    rc = CrTdBltDataAcquire(pScaledCache, enmFormat, fInverted, ppImg);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrTdBltDataAcquire failed rc %d", rc));
        CrTdBltLeave(pTex->pScaledCache);
        return rc;
    }

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(int) CrTdBltDataReleaseScaled(PCR_TEXDATA pTex, const CR_BLITTER_IMG *pImg)
{
    PCR_TEXDATA pScaledCache = RT_FROM_MEMBER(pImg, CR_TEXDATA, Img);
    int rc = CrTdBltDataRelease(pScaledCache);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrTdBltDataRelease failed rc %d", rc));
        return rc;
    }

    if (pScaledCache != pTex)
        CrTdBltLeave(pScaledCache);

    return VINF_SUCCESS;
}

VBOXBLITTERDECL(void) CrTdBltScaleCacheMoveTo(PCR_TEXDATA pTex, PCR_TEXDATA pDstTex)
{
    if (!pTex->pScaledCache)
        return;

    crTdBltSdCleanupCacheNe(pDstTex);

    pDstTex->pScaledCache = pTex->pScaledCache;
    pTex->pScaledCache = NULL;
}

#endif /* !IN_VMSVGA3D */


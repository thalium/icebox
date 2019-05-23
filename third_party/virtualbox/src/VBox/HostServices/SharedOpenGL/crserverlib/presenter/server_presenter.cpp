/* $Id: server_presenter.cpp $ */

/** @file
 * Presenter API
 */

/*
 * Copyright (C) 2012-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef DEBUG_misha
# define VBOXVDBG_MEMCACHE_DISABLE
#endif

#ifndef VBOXVDBG_MEMCACHE_DISABLE
# include <iprt/memcache.h>
#endif

#include "server_presenter.h"

//#define CR_SERVER_WITH_CLIENT_CALLOUTS

#define PCR_FBTEX_FROM_TEX(_pTex) ((CR_FBTEX*)((uint8_t*)(_pTex) - RT_OFFSETOF(CR_FBTEX, Tex)))
#define PCR_FRAMEBUFFER_FROM_COMPOSITOR(_pCompositor) ((CR_FRAMEBUFFER*)((uint8_t*)(_pCompositor) - RT_OFFSETOF(CR_FRAMEBUFFER, Compositor)))
#define PCR_FBENTRY_FROM_ENTRY(_pEntry) ((CR_FRAMEBUFFER_ENTRY*)((uint8_t*)(_pEntry) - RT_OFFSETOF(CR_FRAMEBUFFER_ENTRY, Entry)))


static int crPMgrFbConnectTargetDisplays(HCR_FRAMEBUFFER hFb, CR_FBDISPLAY_INFO *pDpInfo, uint32_t u32ModeAdd);

CR_PRESENTER_GLOBALS g_CrPresenter;

/* FRAMEBUFFER */

void CrFbInit(CR_FRAMEBUFFER *pFb, uint32_t idFb)
{
    RTRECT Rect;
    Rect.xLeft = 0;
    Rect.yTop = 0;
    Rect.xRight = 1;
    Rect.yBottom = 1;
    memset(pFb, 0, sizeof (*pFb));
    pFb->ScreenInfo.u16Flags = VBVA_SCREEN_F_DISABLED;
    pFb->ScreenInfo.u32ViewIndex = idFb;
    CrVrScrCompositorInit(&pFb->Compositor, &Rect);
    RTListInit(&pFb->EntriesList);
    CrHTableCreate(&pFb->SlotTable, 0);
}


bool CrFbIsEnabled(CR_FRAMEBUFFER *pFb)
{
    return !(pFb->ScreenInfo.u16Flags & VBVA_SCREEN_F_DISABLED);
}


const struct VBOXVR_SCR_COMPOSITOR* CrFbGetCompositor(CR_FRAMEBUFFER *pFb)
{
    return &pFb->Compositor;
}

DECLINLINE(CR_FRAMEBUFFER*) CrFbFromCompositor(const struct VBOXVR_SCR_COMPOSITOR* pCompositor)
{
    return RT_FROM_MEMBER(pCompositor, CR_FRAMEBUFFER, Compositor);
}

const struct VBVAINFOSCREEN* CrFbGetScreenInfo(HCR_FRAMEBUFFER hFb)
{
    return &hFb->ScreenInfo;
}

void* CrFbGetVRAM(HCR_FRAMEBUFFER hFb)
{
    return hFb->pvVram;
}

int CrFbUpdateBegin(CR_FRAMEBUFFER *pFb)
{
    ++pFb->cUpdating;

    if (pFb->cUpdating == 1)
    {
        if (pFb->pDisplay)
            pFb->pDisplay->UpdateBegin(pFb);
    }

    return VINF_SUCCESS;
}

void CrFbUpdateEnd(CR_FRAMEBUFFER *pFb)
{
    if (!pFb->cUpdating)
    {
        WARN(("invalid UpdateEnd call!"));
        return;
    }

    --pFb->cUpdating;

    if (!pFb->cUpdating)
    {
        if (pFb->pDisplay)
            pFb->pDisplay->UpdateEnd(pFb);
    }
}

bool CrFbIsUpdating(const CR_FRAMEBUFFER *pFb)
{
    return !!pFb->cUpdating;
}

bool CrFbHas3DData(HCR_FRAMEBUFFER hFb)
{
    return !CrVrScrCompositorIsEmpty(&hFb->Compositor);
}

static void crFbImgFromScreenVram(const VBVAINFOSCREEN *pScreen, void *pvVram, CR_BLITTER_IMG *pImg)
{
    pImg->pvData = pvVram;
    pImg->cbData = pScreen->u32LineSize * pScreen->u32Height;
    pImg->enmFormat = GL_BGRA;
    pImg->width = pScreen->u32Width;
    pImg->height = pScreen->u32Height;
    pImg->bpp = pScreen->u16BitsPerPixel;
    pImg->pitch = pScreen->u32LineSize;
}

static void crFbImgFromDimPtrBGRA(void *pvVram, uint32_t width, uint32_t height, CR_BLITTER_IMG *pImg)
{
    pImg->pvData = pvVram;
    pImg->cbData = width * height * 4;
    pImg->enmFormat = GL_BGRA;
    pImg->width = width;
    pImg->height = height;
    pImg->bpp = 32;
    pImg->pitch = width * 4;
}

static int8_t crFbImgFromDimOffVramBGRA(VBOXCMDVBVAOFFSET offVRAM, uint32_t width, uint32_t height, CR_BLITTER_IMG *pImg)
{
    uint32_t cbBuff = width * height * 4;
    if (offVRAM >= g_cbVRam
            || offVRAM + cbBuff >= g_cbVRam)
    {
        WARN(("invalid param"));
        return -1;
    }

    uint8_t *pu8Buf = g_pvVRamBase + offVRAM;
    crFbImgFromDimPtrBGRA(pu8Buf, width, height, pImg);

    return 0;
}

static int8_t crFbImgFromDescBGRA(const VBOXCMDVBVA_ALLOCDESC *pDesc, CR_BLITTER_IMG *pImg)
{
    return crFbImgFromDimOffVramBGRA(pDesc->Info.u.offVRAM, pDesc->u16Width, pDesc->u16Height, pImg);
}

static void crFbImgFromFb(HCR_FRAMEBUFFER hFb, CR_BLITTER_IMG *pImg)
{
    const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hFb);
    void *pvVram = CrFbGetVRAM(hFb);
    crFbImgFromScreenVram(pScreen, pvVram, pImg);
}

static int crFbTexDataGetContents(CR_TEXDATA *pTex, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pDst)
{
    const CR_BLITTER_IMG *pSrcImg;
    int rc = CrTdBltDataAcquire(pTex, GL_BGRA, false, &pSrcImg);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrTdBltDataAcquire failed rc %d", rc));
        return rc;
    }

    CrMBltImg(pSrcImg, pPos, cRects, pRects, pDst);

    CrTdBltDataRelease(pTex);

    return VINF_SUCCESS;
}

static int crFbBltGetContentsScaledDirect(HCR_FRAMEBUFFER hFb, const RTRECTSIZE *pSrcRectSize, const RTRECT *pDstRect, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pDst)
{
    VBOXVR_LIST List;
    uint32_t c2DRects = 0;
    CR_TEXDATA *pEnteredTex = NULL;
    PCR_BLITTER pEnteredBlitter = NULL;

    /* Scaled texture size and rect calculated for every new "entered" texture. */
    uint32_t width = 0, height = 0;
    RTRECT ScaledSrcRect = {0};

    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR Iter;
    int32_t srcWidth = pSrcRectSize->cx;
    int32_t srcHeight = pSrcRectSize->cy;
    int32_t dstWidth = pDstRect->xRight - pDstRect->xLeft;
    int32_t dstHeight = pDstRect->yBottom - pDstRect->yTop;

    RTPOINT DstPoint = {pDstRect->xLeft, pDstRect->yTop};
    float strX = ((float)dstWidth) / srcWidth;
    float strY = ((float)dstHeight) / srcHeight;
    bool fScale = (dstWidth != srcWidth || dstHeight != srcHeight);
    Assert(fScale);

    /* 'List' contains the destination rectangles to be updated (in pDst coords). */
    VBoxVrListInit(&List);
    int rc = VBoxVrListRectsAdd(&List, cRects, pRects, NULL);
    if (!RT_SUCCESS(rc))
    {
        WARN(("VBoxVrListRectsAdd failed rc %d", rc));
        goto end;
    }

    CrVrScrCompositorConstIterInit(&hFb->Compositor, &Iter);

    for(const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry = CrVrScrCompositorConstIterNext(&Iter);
            pEntry;
            pEntry = CrVrScrCompositorConstIterNext(&Iter))
    {
        /* Where the entry would be located in pDst coords, i.e. convert pEntry hFb coord to pDst coord. */
        RTPOINT ScaledEntryPoint;
        ScaledEntryPoint.x = CR_FLOAT_RCAST(int32_t, strX * CrVrScrCompositorEntryRectGet(pEntry)->xLeft) + pDstRect->xLeft;
        ScaledEntryPoint.y = CR_FLOAT_RCAST(int32_t, strY * CrVrScrCompositorEntryRectGet(pEntry)->yTop) + pDstRect->yTop;

        CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(pEntry);

        /* Optimization to avoid entering/leaving the same texture and its blitter. */
        if (pEnteredTex != pTex)
        {
            if (!pEnteredBlitter)
            {
                pEnteredBlitter = CrTdBlitterGet(pTex);
                rc = CrBltEnter(pEnteredBlitter);
                if (!RT_SUCCESS(rc))
                {
                    WARN(("CrBltEnter failed %d", rc));
                    pEnteredBlitter = NULL;
                    goto end;
                }
            }

            if (pEnteredTex)
            {
                CrTdBltLeave(pEnteredTex);

                pEnteredTex = NULL;

                if (pEnteredBlitter != CrTdBlitterGet(pTex))
                {
                    WARN(("blitters not equal!"));
                    CrBltLeave(pEnteredBlitter);

                    pEnteredBlitter = CrTdBlitterGet(pTex);
                    rc = CrBltEnter(pEnteredBlitter);
                     if (!RT_SUCCESS(rc))
                     {
                         WARN(("CrBltEnter failed %d", rc));
                         pEnteredBlitter = NULL;
                         goto end;
                     }
                }
            }

            rc = CrTdBltEnter(pTex);
            if (!RT_SUCCESS(rc))
            {
                WARN(("CrTdBltEnter failed %d", rc));
                goto end;
            }

            pEnteredTex = pTex;

            const VBOXVR_TEXTURE *pVrTex = CrTdTexGet(pTex);

            width = CR_FLOAT_RCAST(uint32_t, strX * pVrTex->width);
            height = CR_FLOAT_RCAST(uint32_t, strY * pVrTex->height);
            ScaledSrcRect.xLeft = ScaledEntryPoint.x;
            ScaledSrcRect.yTop = ScaledEntryPoint.y;
            ScaledSrcRect.xRight = width + ScaledEntryPoint.x;
            ScaledSrcRect.yBottom = height + ScaledEntryPoint.y;
        }

        bool fInvert = !(CrVrScrCompositorEntryFlagsGet(pEntry) & CRBLT_F_INVERT_SRC_YCOORDS);

        /* pRegions is where the pEntry was drawn in hFb coords. */
        uint32_t cRegions;
        const RTRECT *pRegions;
        rc = CrVrScrCompositorEntryRegionsGet(&hFb->Compositor, pEntry, &cRegions, NULL, NULL, &pRegions);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrVrScrCompositorEntryRegionsGet failed rc %d", rc));
            goto end;
        }

        /* CrTdBltDataAcquireScaled/CrTdBltDataReleaseScaled can use cached data,
         * so it is not necessary to optimize and Aquire only when Tex changes.
         */
        const CR_BLITTER_IMG *pSrcImg;
        rc = CrTdBltDataAcquireScaled(pTex, GL_BGRA, false, width, height, &pSrcImg);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrTdBltDataAcquire failed rc %d", rc));
            goto end;
        }

        for (uint32_t j = 0; j < cRegions; ++j)
        {
            /* rects are in dst coordinates,
             * while the pReg is in source coords
             * convert */
            const RTRECT * pReg = &pRegions[j];
            RTRECT ScaledReg;
            /* scale */
            VBoxRectScaled(pReg, strX, strY, &ScaledReg);
            /* translate */
            VBoxRectTranslate(&ScaledReg, pDstRect->xLeft, pDstRect->yTop);

            /* Exclude the pEntry rectangle, because it will be updated now in pDst.
             * List uses dst coords and pRegions use hFb coords, therefore use
             * ScaledReg which is already translated to dst.
             */
            rc = VBoxVrListRectsSubst(&List, 1, &ScaledReg, NULL);
            if (!RT_SUCCESS(rc))
            {
                WARN(("VBoxVrListRectsSubst failed rc %d", rc));
                goto end;
            }

            for (uint32_t i = 0; i < cRects; ++i)
            {
                const RTRECT * pRect = &pRects[i];

                RTRECT Intersection;
                VBoxRectIntersected(pRect, &ScaledReg, &Intersection);
                if (VBoxRectIsZero(&Intersection))
                    continue;

                VBoxRectIntersect(&Intersection, &ScaledSrcRect);
                if (VBoxRectIsZero(&Intersection))
                    continue;

                CrMBltImgRect(pSrcImg, &ScaledEntryPoint, fInvert, &Intersection, pDst);
            }
        }

        CrTdBltDataReleaseScaled(pTex, pSrcImg);
    }

    /* Blit still not updated dst rects, i.e. not covered by 3D entries. */
    c2DRects = VBoxVrListRectsCount(&List);
    if (c2DRects)
    {
        if (g_CrPresenter.cbTmpBuf2 < c2DRects * sizeof (RTRECT))
        {
            if (g_CrPresenter.pvTmpBuf2)
                RTMemFree(g_CrPresenter.pvTmpBuf2);

            g_CrPresenter.cbTmpBuf2 = (c2DRects + 10) * sizeof (RTRECT);
            g_CrPresenter.pvTmpBuf2 = RTMemAlloc(g_CrPresenter.cbTmpBuf2);
            if (!g_CrPresenter.pvTmpBuf2)
            {
                WARN(("RTMemAlloc failed!"));
                g_CrPresenter.cbTmpBuf2 = 0;
                rc = VERR_NO_MEMORY;
                goto end;
            }
        }

        RTRECT *p2DRects  = (RTRECT *)g_CrPresenter.pvTmpBuf2;

        rc = VBoxVrListRectsGet(&List, c2DRects, p2DRects);
        if (!RT_SUCCESS(rc))
        {
            WARN(("VBoxVrListRectsGet failed, rc %d", rc));
            goto end;
        }

        /* p2DRects are in pDst coords and already scaled. */

        CR_BLITTER_IMG FbImg;

        crFbImgFromFb(hFb, &FbImg);

        CrMBltImgScaled(&FbImg, pSrcRectSize, pDstRect, c2DRects, p2DRects, pDst);
    }

end:

    if (pEnteredTex)
        CrTdBltLeave(pEnteredTex);

    if (pEnteredBlitter)
        CrBltLeave(pEnteredBlitter);

    VBoxVrListClear(&List);

    return rc;
}

static int crFbBltGetContentsScaledCPU(HCR_FRAMEBUFFER hFb, const RTRECTSIZE *pSrcRectSize, const RTRECT *pDstRect, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pImg)
{
    WARN(("not implemented!"));
    return VERR_NOT_IMPLEMENTED;
#if 0
    int32_t srcWidth = pSrcRectSize->cx;
    int32_t srcHeight = pSrcRectSize->cy;
    int32_t dstWidth = pDstRect->xRight - pDstRect->xLeft;
    int32_t dstHeight = pDstRect->yBottom - pDstRect->yTop;

    RTPOINT DstPoint = {pDstRect->xLeft, pDstRect->yTop};
    float strX = ((float)dstWidth) / srcWidth;
    float strY = ((float)dstHeight) / srcHeight;

    RTPOINT UnscaledPos;
    UnscaledPos.x = CR_FLOAT_RCAST(int32_t, pDstRect->xLeft / strX);
    UnscaledPos.y = CR_FLOAT_RCAST(int32_t, pDstRect->yTop / strY);

    /* destination is bigger than the source, do 3D data stretching with CPU */
    CR_BLITTER_IMG Img;
    Img.cbData = srcWidth * srcHeight * 4;
    Img.pvData = RTMemAlloc(Img.cbData);
    if (!Img.pvData)
    {
        WARN(("RTMemAlloc Failed"));
        return VERR_NO_MEMORY;
    }
    Img.enmFormat = pImg->enmFormat;
    Img.width = srcWidth;
    Img.height = srcHeight;
    Img.bpp = pImg->bpp;
    Img.pitch = Img.width * 4;

    int rc = CrFbBltGetContents(hFb, &UnscaledPos, cRects, pRects, &Img);
    if (RT_SUCCESS(rc))
    {
        CrBmpScale32((uint8_t *)pImg->pvData,
                            pImg->pitch,
                            pImg->width, pImg->height,
                            (const uint8_t *)Img.pvData,
                            Img.pitch,
                            Img.width, Img.height);
    }
    else
        WARN(("CrFbBltGetContents failed %d", rc));

    RTMemFree(Img.pvData);

    return rc;
#endif
}

static int CrFbBltGetContents(HCR_FRAMEBUFFER hFb, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pDst)
{
    VBOXVR_LIST List;
    uint32_t c2DRects = 0;
    CR_TEXDATA *pEnteredTex = NULL;
    PCR_BLITTER pEnteredBlitter = NULL;

    /* 'List' contains the destination rectangles to be updated (in pDst coords). */
    VBoxVrListInit(&List);
    int rc = VBoxVrListRectsAdd(&List, cRects, pRects, NULL);
    if (!RT_SUCCESS(rc))
    {
        WARN(("VBoxVrListRectsAdd failed rc %d", rc));
        goto end;
    }

    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR Iter;
    CrVrScrCompositorConstIterInit(&hFb->Compositor, &Iter);

    for(const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry = CrVrScrCompositorConstIterNext(&Iter);
            pEntry;
            pEntry = CrVrScrCompositorConstIterNext(&Iter))
    {
        /* Where the entry would be located in pDst coords (pPos = pDst_coord - hFb_coord). */
        RTPOINT EntryPoint;
        EntryPoint.x = CrVrScrCompositorEntryRectGet(pEntry)->xLeft + pPos->x;
        EntryPoint.y = CrVrScrCompositorEntryRectGet(pEntry)->yTop + pPos->y;

        CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(pEntry);

        /* Optimization to avoid entering/leaving the same texture and its blitter. */
        if (pEnteredTex != pTex)
        {
            if (!pEnteredBlitter)
            {
                pEnteredBlitter = CrTdBlitterGet(pTex);
                rc = CrBltEnter(pEnteredBlitter);
                if (!RT_SUCCESS(rc))
                {
                    WARN(("CrBltEnter failed %d", rc));
                    pEnteredBlitter = NULL;
                    goto end;
                }
            }

            if (pEnteredTex)
            {
                CrTdBltLeave(pEnteredTex);

                pEnteredTex = NULL;

                if (pEnteredBlitter != CrTdBlitterGet(pTex))
                {
                    WARN(("blitters not equal!"));
                    CrBltLeave(pEnteredBlitter);

                    pEnteredBlitter = CrTdBlitterGet(pTex);
                    rc = CrBltEnter(pEnteredBlitter);
                     if (!RT_SUCCESS(rc))
                     {
                         WARN(("CrBltEnter failed %d", rc));
                         pEnteredBlitter = NULL;
                         goto end;
                     }
                }
            }

            rc = CrTdBltEnter(pTex);
            if (!RT_SUCCESS(rc))
            {
                WARN(("CrTdBltEnter failed %d", rc));
                goto end;
            }

            pEnteredTex = pTex;
        }

        bool fInvert = !(CrVrScrCompositorEntryFlagsGet(pEntry) & CRBLT_F_INVERT_SRC_YCOORDS);

        /* pRegions is where the pEntry was drawn in hFb coords. */
        uint32_t cRegions;
        const RTRECT *pRegions;
        rc = CrVrScrCompositorEntryRegionsGet(&hFb->Compositor, pEntry, &cRegions, NULL, NULL, &pRegions);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrVrScrCompositorEntryRegionsGet failed rc %d", rc));
            goto end;
        }

        /* CrTdBltDataAcquire/CrTdBltDataRelease can use cached data,
         * so it is not necessary to optimize and Aquire only when Tex changes.
         */
        const CR_BLITTER_IMG *pSrcImg;
        rc = CrTdBltDataAcquire(pTex, GL_BGRA, false, &pSrcImg);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrTdBltDataAcquire failed rc %d", rc));
            goto end;
        }

        for (uint32_t j = 0; j < cRegions; ++j)
        {
            /* rects are in dst coordinates,
             * while the pReg is in source coords
             * convert */
            const RTRECT * pReg = &pRegions[j];
            RTRECT SrcReg;
            /* translate */
            VBoxRectTranslated(pReg, pPos->x, pPos->y, &SrcReg);

            /* Exclude the pEntry rectangle, because it will be updated now in pDst.
             * List uses dst coords and pRegions use hFb coords, therefore use
             * SrcReg which is already translated to dst.
             */
            rc = VBoxVrListRectsSubst(&List, 1, &SrcReg, NULL);
            if (!RT_SUCCESS(rc))
            {
                WARN(("VBoxVrListRectsSubst failed rc %d", rc));
                goto end;
            }

            for (uint32_t i = 0; i < cRects; ++i)
            {
                const RTRECT * pRect = &pRects[i];

                RTRECT Intersection;
                VBoxRectIntersected(pRect, &SrcReg, &Intersection);
                if (VBoxRectIsZero(&Intersection))
                    continue;

                CrMBltImgRect(pSrcImg, &EntryPoint, fInvert, &Intersection, pDst);
            }
        }

        CrTdBltDataRelease(pTex);
    }

    /* Blit still not updated dst rects, i.e. not covered by 3D entries. */
    c2DRects = VBoxVrListRectsCount(&List);
    if (c2DRects)
    {
        if (g_CrPresenter.cbTmpBuf2 < c2DRects * sizeof (RTRECT))
        {
            if (g_CrPresenter.pvTmpBuf2)
                RTMemFree(g_CrPresenter.pvTmpBuf2);

            g_CrPresenter.cbTmpBuf2 = (c2DRects + 10) * sizeof (RTRECT);
            g_CrPresenter.pvTmpBuf2 = RTMemAlloc(g_CrPresenter.cbTmpBuf2);
            if (!g_CrPresenter.pvTmpBuf2)
            {
                WARN(("RTMemAlloc failed!"));
                g_CrPresenter.cbTmpBuf2 = 0;
                rc = VERR_NO_MEMORY;
                goto end;
            }
        }

        RTRECT *p2DRects  = (RTRECT *)g_CrPresenter.pvTmpBuf2;

        rc = VBoxVrListRectsGet(&List, c2DRects, p2DRects);
        if (!RT_SUCCESS(rc))
        {
            WARN(("VBoxVrListRectsGet failed, rc %d", rc));
            goto end;
        }

        CR_BLITTER_IMG FbImg;

        crFbImgFromFb(hFb, &FbImg);

        CrMBltImg(&FbImg, pPos, c2DRects, p2DRects, pDst);
    }

end:

    if (pEnteredTex)
        CrTdBltLeave(pEnteredTex);

    if (pEnteredBlitter)
        CrBltLeave(pEnteredBlitter);

    VBoxVrListClear(&List);

    return rc;
}

int CrFbBltGetContentsEx(HCR_FRAMEBUFFER hFb, const RTRECTSIZE *pSrcRectSize, const RTRECT *pDstRect, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pImg)
{
    uint32_t srcWidth = pSrcRectSize->cx;
    uint32_t srcHeight = pSrcRectSize->cy;
    uint32_t dstWidth = pDstRect->xRight - pDstRect->xLeft;
    uint32_t dstHeight = pDstRect->yBottom - pDstRect->yTop;
    if (srcWidth == dstWidth
            && srcHeight == dstHeight)
    {
        RTPOINT Pos = {pDstRect->xLeft, pDstRect->yTop};
        return CrFbBltGetContents(hFb, &Pos, cRects, pRects, pImg);
    }
    if (!CrFbHas3DData(hFb)
            || (srcWidth * srcHeight > dstWidth * dstHeight))
        return crFbBltGetContentsScaledDirect(hFb, pSrcRectSize, pDstRect, cRects, pRects, pImg);

    return crFbBltGetContentsScaledCPU(hFb, pSrcRectSize, pDstRect, cRects, pRects, pImg);
}

static void crFbBltPutContentsFbVram(HCR_FRAMEBUFFER hFb, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pSrc)
{
    const RTRECT *pCompRect = CrVrScrCompositorRectGet(&hFb->Compositor);

    CR_BLITTER_IMG FbImg;

    crFbImgFromFb(hFb, &FbImg);

    CrMBltImg(pSrc, pPos, cRects, pRects, &FbImg);
}

static void crFbClrFillFbVram(HCR_FRAMEBUFFER hFb, uint32_t cRects, const RTRECT *pRects, uint32_t u32Color)
{
    CR_BLITTER_IMG FbImg;

    crFbImgFromFb(hFb, &FbImg);

    CrMClrFillImg(&FbImg, cRects, pRects, u32Color);
}

int CrFbClrFill(HCR_FRAMEBUFFER hFb, uint32_t cRects, const RTRECT *pRects, uint32_t u32Color)
{
    if (!hFb->cUpdating)
    {
        WARN(("framebuffer not updating"));
        return VERR_INVALID_STATE;
    }

    crFbClrFillFbVram(hFb, cRects, pRects, u32Color);

    RTPOINT DstPoint = {0, 0};

    int rc = CrFbEntryRegionsAdd(hFb, NULL, &DstPoint, cRects, pRects, false);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbEntryRegionsAdd failed %d", rc));
        return rc;
    }

    return VINF_SUCCESS;
}

static int crFbBltPutContents(HCR_FRAMEBUFFER hFb, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pImg)
{
    crFbBltPutContentsFbVram(hFb, pPos, cRects, pRects, pImg);

    int rc = CrFbEntryRegionsAdd(hFb, NULL, pPos, cRects, pRects, false);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbEntryRegionsAdd failed %d", rc));
        return rc;
    }

    return VINF_SUCCESS;
}

int CrFbBltPutContents(HCR_FRAMEBUFFER hFb, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pImg)
{
    if (!hFb->cUpdating)
    {
        WARN(("framebuffer not updating"));
        return VERR_INVALID_STATE;
    }

    return crFbBltPutContents(hFb, pPos, cRects, pRects, pImg);
}

static int crFbRegionsIsIntersectRects(HCR_FRAMEBUFFER hFb, uint32_t cRects, const RTRECT *pRects, bool *pfRegChanged)
{
    uint32_t cCompRects;
    const RTRECT *pCompRects;
    int rc = CrVrScrCompositorRegionsGet(&hFb->Compositor, &cCompRects, NULL, NULL, &pCompRects);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrVrScrCompositorRegionsGet failed rc %d", rc));
        return rc;
    }

    bool fRegChanged = false;
    for (uint32_t i = 0; i < cCompRects; ++i)
    {
        const RTRECT *pCompRect = &pCompRects[i];
        for (uint32_t j = 0; j < cRects; ++j)
        {
            const RTRECT *pRect = &pRects[j];
            if (VBoxRectIsIntersect(pCompRect, pRect))
            {
                *pfRegChanged = true;
                return VINF_SUCCESS;
            }
        }
    }

    *pfRegChanged = false;
    return VINF_SUCCESS;
}

int CrFbBltPutContentsNe(HCR_FRAMEBUFFER hFb, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, CR_BLITTER_IMG *pImg)
{
    bool fRegChanged = false;
    int rc = crFbRegionsIsIntersectRects(hFb, cRects, pRects, &fRegChanged);
    if (!RT_SUCCESS(rc))
    {
        WARN(("crFbRegionsIsIntersectRects failed rc %d", rc));
        return rc;
    }

    if (fRegChanged)
    {
        rc = CrFbUpdateBegin(hFb);
        if (RT_SUCCESS(rc))
        {
            rc = CrFbBltPutContents(hFb, pPos, cRects, pRects, pImg);
            if (!RT_SUCCESS(rc))
                WARN(("CrFbBltPutContents failed rc %d", rc));
            CrFbUpdateEnd(hFb);
        }
        else
            WARN(("CrFbUpdateBegin failed rc %d", rc));

        return rc;
    }

    crFbBltPutContentsFbVram(hFb, pPos, cRects, pRects, pImg);
    return VINF_SUCCESS;
}

int CrFbClrFillNe(HCR_FRAMEBUFFER hFb, uint32_t cRects, const RTRECT *pRects, uint32_t u32Color)
{
    bool fRegChanged = false;
    int rc = crFbRegionsIsIntersectRects(hFb, cRects, pRects, &fRegChanged);
    if (!RT_SUCCESS(rc))
    {
        WARN(("crFbRegionsIsIntersectRects failed rc %d", rc));
        return rc;
    }

    if (fRegChanged)
    {
        rc = CrFbUpdateBegin(hFb);
        if (RT_SUCCESS(rc))
        {
            rc = CrFbClrFill(hFb, cRects, pRects, u32Color);
            if (!RT_SUCCESS(rc))
                WARN(("CrFbClrFill failed rc %d", rc));
            CrFbUpdateEnd(hFb);
        }
        else
            WARN(("CrFbUpdateBegin failed rc %d", rc));

        return rc;
    }

    crFbClrFillFbVram(hFb, cRects, pRects, u32Color);
    return VINF_SUCCESS;
}

int CrFbResize(CR_FRAMEBUFFER *pFb, const struct VBVAINFOSCREEN * pScreen, void *pvVRAM)
{
    if (!pFb->cUpdating)
    {
        WARN(("no update in progress"));
        return VERR_INVALID_STATE;
    }

    int rc = VINF_SUCCESS;
    if (CrFbIsEnabled(pFb))
    {
        rc = CrFbRegionsClear(pFb);
        if (RT_FAILURE(rc))
        {
            WARN(("CrFbRegionsClear failed %d", rc));
            return rc;
        }
    }

    RTRECT Rect;
    Rect.xLeft = 0;
    Rect.yTop = 0;
    Rect.xRight = pScreen->u32Width;
    Rect.yBottom = pScreen->u32Height;
    rc = CrVrScrCompositorRectSet(&pFb->Compositor, &Rect, NULL);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrVrScrCompositorRectSet failed rc %d", rc));
        return rc;
    }

    pFb->ScreenInfo = *pScreen;
    pFb->pvVram = pvVRAM ? pvVRAM : g_pvVRamBase + pScreen->u32StartOffset;

    if (pFb->pDisplay)
        pFb->pDisplay->FramebufferChanged(pFb);

    return VINF_SUCCESS;
}

void CrFbTerm(CR_FRAMEBUFFER *pFb)
{
    if (pFb->cUpdating)
    {
        WARN(("update in progress"));
        return;
    }
    uint32_t idFb = pFb->ScreenInfo.u32ViewIndex;

    CrVrScrCompositorClear(&pFb->Compositor);
    CrHTableDestroy(&pFb->SlotTable);

    Assert(RTListIsEmpty(&pFb->EntriesList));
    Assert(!pFb->cEntries);

    memset(pFb, 0, sizeof (*pFb));

    pFb->ScreenInfo.u16Flags = VBVA_SCREEN_F_DISABLED;
    pFb->ScreenInfo.u32ViewIndex = idFb;
}

ICrFbDisplay* CrFbDisplayGet(CR_FRAMEBUFFER *pFb)
{
    return pFb->pDisplay;
}

int CrFbDisplaySet(CR_FRAMEBUFFER *pFb, ICrFbDisplay *pDisplay)
{
    if (pFb->cUpdating)
    {
        WARN(("update in progress"));
        return VERR_INVALID_STATE;
    }

    if (pFb->pDisplay == pDisplay)
        return VINF_SUCCESS;

    pFb->pDisplay = pDisplay;

    return VINF_SUCCESS;
}

#define CR_PMGR_MODE_WINDOW 0x1
/* mutually exclusive with CR_PMGR_MODE_WINDOW */
#define CR_PMGR_MODE_ROOTVR 0x2
#define CR_PMGR_MODE_VRDP   0x4
#define CR_PMGR_MODE_ALL    0x7

static int crPMgrModeModifyGlobal(uint32_t u32ModeAdd, uint32_t u32ModeRemove);
static void crPMgrCleanUnusedDisplays();

static CR_FBTEX* crFbTexAlloc()
{
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    return (CR_FBTEX*)RTMemCacheAlloc(g_CrPresenter.FbTexLookasideList);
#else
    return (CR_FBTEX*)RTMemAlloc(sizeof (CR_FBTEX));
#endif
}

static void crFbTexFree(CR_FBTEX *pTex)
{
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMemCacheFree(g_CrPresenter.FbTexLookasideList, pTex);
#else
    RTMemFree(pTex);
#endif
}

static CR_FRAMEBUFFER_ENTRY* crFbEntryAlloc()
{
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    return (CR_FRAMEBUFFER_ENTRY*)RTMemCacheAlloc(g_CrPresenter.FbEntryLookasideList);
#else
    return (CR_FRAMEBUFFER_ENTRY*)RTMemAlloc(sizeof (CR_FRAMEBUFFER_ENTRY));
#endif
}

static void crFbEntryFree(CR_FRAMEBUFFER_ENTRY *pEntry)
{
    Assert(!CrVrScrCompositorEntryIsUsed(&pEntry->Entry));
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMemCacheFree(g_CrPresenter.FbEntryLookasideList, pEntry);
#else
    RTMemFree(pEntry);
#endif
}

DECLCALLBACK(void) crFbTexRelease(CR_TEXDATA *pTex)
{
    CR_FBTEX *pFbTex = PCR_FBTEX_FROM_TEX(pTex);
    CRTextureObj *pTobj = pFbTex->pTobj;

    CrTdBltDataCleanupNe(pTex);

    if (pTobj)
    {
        crHashtableDelete(g_CrPresenter.pFbTexMap, pTobj->id, NULL);

        crStateReleaseTexture(cr_server.MainContextInfo.pContext, pTobj);


        crStateGlobalSharedRelease();
    }

    crFbTexFree(pFbTex);
}

void CrFbTexDataInit(CR_TEXDATA* pFbTex, const VBOXVR_TEXTURE *pTex, PFNCRTEXDATA_RELEASED pfnTextureReleased)
{
    PCR_BLITTER pBlitter = crServerVBoxBlitterGet();

    CrTdInit(pFbTex, pTex, pBlitter, pfnTextureReleased);
}

static CR_FBTEX* crFbTexCreate(const VBOXVR_TEXTURE *pTex)
{
    CR_FBTEX *pFbTex = crFbTexAlloc();
    if (!pFbTex)
    {
        WARN(("crFbTexAlloc failed!"));
        return NULL;
    }

    CrFbTexDataInit(&pFbTex->Tex, pTex, crFbTexRelease);
    pFbTex->pTobj = NULL;

    return pFbTex;
}

CR_TEXDATA* CrFbTexDataCreate(const VBOXVR_TEXTURE *pTex)
{
    CR_FBTEX *pFbTex = crFbTexCreate(pTex);
    if (!pFbTex)
    {
        WARN(("crFbTexCreate failed!"));
        return NULL;
    }

    return &pFbTex->Tex;
}

static CR_FBTEX* crFbTexAcquire(GLuint idTexture)
{
    CR_FBTEX *pFbTex = (CR_FBTEX *)crHashtableSearch(g_CrPresenter.pFbTexMap, idTexture);
    if (pFbTex)
    {
        CrTdAddRef(&pFbTex->Tex);
        return pFbTex;
    }

    CRSharedState *pShared = crStateGlobalSharedAcquire();
    if (!pShared)
    {
        WARN(("pShared is null!"));
        return NULL;
    }

    CRTextureObj *pTobj = (CRTextureObj*)crHashtableSearch(pShared->textureTable, idTexture);
    if (!pTobj)
    {
        LOG(("pTobj is null!"));
        crStateGlobalSharedRelease();
        return NULL;
    }

    Assert(pTobj->id == idTexture);

    GLuint hwid = crStateGetTextureObjHWID(pTobj);
    if (!hwid)
    {
        WARN(("hwId is null!"));
        crStateGlobalSharedRelease();
        return NULL;
    }

    VBOXVR_TEXTURE Tex;
    Tex.width = pTobj->level[0]->width;
    Tex.height = pTobj->level[0]->height;
    Tex.hwid = hwid;
    Tex.target = pTobj->target;

    pFbTex = crFbTexCreate(&Tex);
    if (!pFbTex)
    {
        WARN(("crFbTexCreate failed!"));
        crStateGlobalSharedRelease();
        return NULL;
    }

    CR_STATE_SHAREDOBJ_USAGE_SET(pTobj, cr_server.MainContextInfo.pContext);

    pFbTex->pTobj = pTobj;

    crHashtableAdd(g_CrPresenter.pFbTexMap, idTexture, pFbTex);

    return pFbTex;
}

static CR_TEXDATA* CrFbTexDataAcquire(GLuint idTexture)
{
    CR_FBTEX* pTex = crFbTexAcquire(idTexture);
    if (!pTex)
    {
        WARN(("crFbTexAcquire failed for %d", idTexture));
        return NULL;
    }

    return &pTex->Tex;
}

static void crFbEntryMarkDestroyed(CR_FRAMEBUFFER *pFb, CR_FRAMEBUFFER_ENTRY* pEntry)
{
    if (pEntry->Flags.fCreateNotified)
    {
        pEntry->Flags.fCreateNotified = 0;
        if (pFb->pDisplay)
            pFb->pDisplay->EntryDestroyed(pFb, pEntry);

        CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(&pEntry->Entry);
        if (pTex)
            CrTdBltDataInvalidateNe(pTex);
    }
}

static void crFbEntryDestroy(CR_FRAMEBUFFER *pFb, CR_FRAMEBUFFER_ENTRY* pEntry)
{
    crFbEntryMarkDestroyed(pFb, pEntry);
    CrVrScrCompositorEntryCleanup(&pEntry->Entry);
    CrHTableDestroy(&pEntry->HTable);
    Assert(pFb->cEntries);
    RTListNodeRemove(&pEntry->Node);
    --pFb->cEntries;
    crFbEntryFree(pEntry);
}

DECLINLINE(uint32_t) crFbEntryAddRef(CR_FRAMEBUFFER_ENTRY* pEntry)
{
    return ++pEntry->cRefs;
}

DECLINLINE(uint32_t) crFbEntryRelease(CR_FRAMEBUFFER *pFb, CR_FRAMEBUFFER_ENTRY* pEntry)
{
    uint32_t cRefs = --pEntry->cRefs;
    if (!cRefs)
        crFbEntryDestroy(pFb, pEntry);
    return cRefs;
}

static DECLCALLBACK(void) crFbEntryReleased(const struct VBOXVR_SCR_COMPOSITOR *pCompositor, struct VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry, struct VBOXVR_SCR_COMPOSITOR_ENTRY *pReplacingEntry)
{
    CR_FRAMEBUFFER *pFb = PCR_FRAMEBUFFER_FROM_COMPOSITOR(pCompositor);
    CR_FRAMEBUFFER_ENTRY *pFbEntry = PCR_FBENTRY_FROM_ENTRY(pEntry);
    CR_FRAMEBUFFER_ENTRY *pFbReplacingEntry = pReplacingEntry ? PCR_FBENTRY_FROM_ENTRY(pReplacingEntry) : NULL;
    if (pFbReplacingEntry)
    {
        /*replace operation implies the replaced entry gets auto-destroyed,
         * while all its data gets moved to the *clean* replacing entry
         * 1. ensure the replacing entry is cleaned up */
        crFbEntryMarkDestroyed(pFb, pFbReplacingEntry);

        CrHTableMoveTo(&pFbEntry->HTable, &pFbReplacingEntry->HTable);

        CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(&pFbEntry->Entry);
        CR_TEXDATA *pReplacingTex = CrVrScrCompositorEntryTexGet(&pFbReplacingEntry->Entry);

        CrTdBltScaleCacheMoveTo(pTex, pReplacingTex);

        if (pFb->pDisplay)
            pFb->pDisplay->EntryReplaced(pFb, pFbReplacingEntry, pFbEntry);

        CrTdBltDataInvalidateNe(pTex);

        /* 2. mark the replaced entry is destroyed */
        Assert(pFbEntry->Flags.fCreateNotified);
        Assert(pFbEntry->Flags.fInList);
        pFbEntry->Flags.fCreateNotified = 0;
        pFbEntry->Flags.fInList = 0;
        pFbReplacingEntry->Flags.fCreateNotified = 1;
        pFbReplacingEntry->Flags.fInList = 1;
    }
    else
    {
        if (pFbEntry->Flags.fInList)
        {
            pFbEntry->Flags.fInList = 0;
            if (pFb->pDisplay)
                pFb->pDisplay->EntryRemoved(pFb, pFbEntry);

            CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(&pFbEntry->Entry);
            if (pTex)
                CrTdBltDataInvalidateNe(pTex);
        }
    }

    crFbEntryRelease(pFb, pFbEntry);
}

static CR_FRAMEBUFFER_ENTRY* crFbEntryCreate(CR_FRAMEBUFFER *pFb, CR_TEXDATA* pTex, const RTRECT *pRect, uint32_t fFlags)
{
    CR_FRAMEBUFFER_ENTRY *pEntry = crFbEntryAlloc();
    if (!pEntry)
    {
        WARN(("crFbEntryAlloc failed!"));
        return NULL;
    }

    CrVrScrCompositorEntryInit(&pEntry->Entry, pRect, pTex, crFbEntryReleased);
    CrVrScrCompositorEntryFlagsSet(&pEntry->Entry, fFlags);
    pEntry->cRefs = 1;
    pEntry->Flags.Value = 0;
    CrHTableCreate(&pEntry->HTable, 0);

    RTListAppend(&pFb->EntriesList, &pEntry->Node);
    ++pFb->cEntries;

    return pEntry;
}

int CrFbEntryCreateForTexData(CR_FRAMEBUFFER *pFb, struct CR_TEXDATA *pTex, uint32_t fFlags, HCR_FRAMEBUFFER_ENTRY *phEntry)
{
    if (pTex == NULL)
    {
        WARN(("pTex is NULL"));
        return VERR_INVALID_PARAMETER;
    }

    RTRECT Rect;
    Rect.xLeft = 0;
    Rect.yTop = 0;
    Rect.xRight = pTex->Tex.width;
    Rect.yBottom = pTex->Tex.height;
    CR_FRAMEBUFFER_ENTRY* pEntry = crFbEntryCreate(pFb, pTex, &Rect, fFlags);
    if (!pEntry)
    {
        WARN(("crFbEntryCreate failed"));
        return VERR_NO_MEMORY;
    }

    *phEntry = pEntry;
    return VINF_SUCCESS;
}

int CrFbEntryTexDataUpdate(CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY pEntry, struct CR_TEXDATA *pTex)
{
    if (!pFb->cUpdating)
    {
        WARN(("framebuffer not updating"));
        return VERR_INVALID_STATE;
    }

    if (pTex)
        CrVrScrCompositorEntryTexSet(&pEntry->Entry, pTex);

    if (CrVrScrCompositorEntryIsUsed(&pEntry->Entry))
    {
        if (pFb->pDisplay)
            pFb->pDisplay->EntryTexChanged(pFb, pEntry);

        CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(&pEntry->Entry);
        if (pTex)
            CrTdBltDataInvalidateNe(pTex);
    }

    return VINF_SUCCESS;
}


int CrFbEntryCreateForTexId(CR_FRAMEBUFFER *pFb, GLuint idTexture, uint32_t fFlags, HCR_FRAMEBUFFER_ENTRY *phEntry)
{
    CR_FBTEX* pFbTex = crFbTexAcquire(idTexture);
    if (!pFbTex)
    {
        LOG(("crFbTexAcquire failed"));
        return VERR_INVALID_PARAMETER;
    }

    CR_TEXDATA* pTex = &pFbTex->Tex;
    int rc = CrFbEntryCreateForTexData(pFb, pTex, fFlags, phEntry);
    if (!RT_SUCCESS(rc))
    {
    	WARN(("CrFbEntryCreateForTexData failed rc %d", rc));
    }

    /*always release the tex, the CrFbEntryCreateForTexData will do incref as necessary */
    CrTdRelease(pTex);
    return rc;
}

void CrFbEntryAddRef(CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    ++hEntry->cRefs;
}

void CrFbEntryRelease(CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    crFbEntryRelease(pFb, hEntry);
}

static int8_t crVBoxServerCrCmdBltPrimaryVramGenericProcess(uint32_t u32PrimaryID, VBOXCMDVBVAOFFSET offVRAM, uint32_t width, uint32_t height, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, bool fToPrimary);

int CrFbRegionsClear(HCR_FRAMEBUFFER hFb)
{
    if (!hFb->cUpdating)
    {
        WARN(("framebuffer not updating"));
        return VERR_INVALID_STATE;
    }

    uint32_t cRegions;
    const RTRECT *pRegions;
    int rc = CrVrScrCompositorRegionsGet(&hFb->Compositor, &cRegions, NULL, NULL, &pRegions);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrVrScrCompositorEntryRegionsGet failed rc %d", rc));
        return rc;
    }

    const struct VBVAINFOSCREEN* pScreen = CrFbGetScreenInfo(hFb);
    VBOXCMDVBVAOFFSET offVRAM = (VBOXCMDVBVAOFFSET)(((uintptr_t)CrFbGetVRAM(hFb)) - ((uintptr_t)g_pvVRamBase));
    RTPOINT Pos = {0,0};
    int8_t i8Result = crVBoxServerCrCmdBltPrimaryVramGenericProcess(pScreen->u32ViewIndex, offVRAM, pScreen->u32Width, pScreen->u32Height, &Pos, cRegions, pRegions, true);
    if (i8Result)
    {
        WARN(("crVBoxServerCrCmdBltPrimaryVramGenericProcess failed"));
        return VERR_INTERNAL_ERROR;
    }

#ifdef DEBUG
    {
        uint32_t cTmpRegions;
        const RTRECT *pTmpRegions;
        int tmpRc = CrVrScrCompositorRegionsGet(&hFb->Compositor, &cTmpRegions, NULL, NULL, &pTmpRegions);
        if (!RT_SUCCESS(tmpRc))
        {
            WARN(("CrVrScrCompositorEntryRegionsGet failed rc %d", tmpRc));
        }
        Assert(!cTmpRegions);
    }
#endif

    /* just in case */
    bool fChanged = false;
    CrVrScrCompositorRegionsClear(&hFb->Compositor, &fChanged);
    Assert(!fChanged);

    if (cRegions)
    {
        if (hFb->pDisplay)
            hFb->pDisplay->RegionsChanged(hFb);
    }

    return VINF_SUCCESS;
}

int CrFbEntryRegionsAdd(CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry, const RTPOINT *pPos, uint32_t cRegions, const RTRECT *paRegions, bool fPosRelated)
{
    if (!pFb->cUpdating)
    {
        WARN(("framebuffer not updating"));
        return VERR_INVALID_STATE;
    }

    uint32_t fChangeFlags = 0;
    VBOXVR_SCR_COMPOSITOR_ENTRY *pReplacedScrEntry = NULL;
    VBOXVR_SCR_COMPOSITOR_ENTRY *pNewEntry;
    bool fEntryWasInList;

    if (hEntry)
    {
        crFbEntryAddRef(hEntry);
        pNewEntry = &hEntry->Entry;
        fEntryWasInList = CrVrScrCompositorEntryIsUsed(pNewEntry);

        Assert(!hEntry->Flags.fInList == !fEntryWasInList);
    }
    else
    {
        pNewEntry = NULL;
        fEntryWasInList = false;
    }

    int rc = CrVrScrCompositorEntryRegionsAdd(&pFb->Compositor, hEntry ? &hEntry->Entry : NULL, pPos, cRegions, paRegions, fPosRelated, &pReplacedScrEntry, &fChangeFlags);
    if (RT_SUCCESS(rc))
    {
        if (fChangeFlags & VBOXVR_COMPOSITOR_CF_REGIONS_CHANGED)
        {
            if (!fEntryWasInList && pNewEntry)
            {
                Assert(CrVrScrCompositorEntryIsUsed(pNewEntry));
                if (!hEntry->Flags.fCreateNotified)
                {
                    hEntry->Flags.fCreateNotified = 1;
                    if (pFb->pDisplay)
                        pFb->pDisplay->EntryCreated(pFb, hEntry);
                }

#ifdef DEBUG_misha
                /* in theory hEntry->Flags.fInList can be set if entry is replaced,
                 * but then modified to fit the compositor rects,
                 * and so we get the regions changed notification as a result
                 * this should not generally happen though, so put an assertion to debug that situation */
                Assert(!hEntry->Flags.fInList);
#endif
                if (!hEntry->Flags.fInList)
                {
                    hEntry->Flags.fInList = 1;

                    if (pFb->pDisplay)
                        pFb->pDisplay->EntryAdded(pFb, hEntry);
                }
            }
            if (pFb->pDisplay)
                pFb->pDisplay->RegionsChanged(pFb);

            Assert(!pReplacedScrEntry);
        }
        else if (fChangeFlags & VBOXVR_COMPOSITOR_CF_ENTRY_REPLACED)
        {
            Assert(pReplacedScrEntry);
            /* we have already processed that in a "release" callback */
            Assert(hEntry);
        }
        else
        {
            Assert(!fChangeFlags);
            Assert(!pReplacedScrEntry);
        }

        if (hEntry)
        {
            if (CrVrScrCompositorEntryIsUsed(&hEntry->Entry))
            {
                if (pFb->pDisplay)
                    pFb->pDisplay->EntryTexChanged(pFb, hEntry);

                CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(&hEntry->Entry);
                if (pTex)
                    CrTdBltDataInvalidateNe(pTex);
            }
        }
    }
    else
        WARN(("CrVrScrCompositorEntryRegionsAdd failed, rc %d", rc));

    return rc;
}

int CrFbEntryRegionsSet(CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry, const RTPOINT *pPos, uint32_t cRegions, const RTRECT *paRegions, bool fPosRelated)
{
    if (!pFb->cUpdating)
    {
        WARN(("framebuffer not updating"));
        return VERR_INVALID_STATE;
    }

    bool fChanged = 0;
    VBOXVR_SCR_COMPOSITOR_ENTRY *pReplacedScrEntry = NULL;
    VBOXVR_SCR_COMPOSITOR_ENTRY *pNewEntry;
    bool fEntryWasInList;

    if (hEntry)
    {
        crFbEntryAddRef(hEntry);
        pNewEntry = &hEntry->Entry;
        fEntryWasInList = CrVrScrCompositorEntryIsUsed(pNewEntry);
        Assert(!hEntry->Flags.fInList == !fEntryWasInList);
    }
    else
    {
        pNewEntry = NULL;
        fEntryWasInList = false;
    }

    int rc = CrVrScrCompositorEntryRegionsSet(&pFb->Compositor, pNewEntry, pPos, cRegions, paRegions, fPosRelated, &fChanged);
    if (RT_SUCCESS(rc))
    {
        if (fChanged)
        {
            if (!fEntryWasInList && pNewEntry)
            {
                if (CrVrScrCompositorEntryIsUsed(pNewEntry))
                {
                    if (!hEntry->Flags.fCreateNotified)
                    {
                        hEntry->Flags.fCreateNotified = 1;

                        if (pFb->pDisplay)
                            pFb->pDisplay->EntryCreated(pFb, hEntry);
                    }

                    Assert(!hEntry->Flags.fInList);
                    hEntry->Flags.fInList = 1;

                    if (pFb->pDisplay)
                        pFb->pDisplay->EntryAdded(pFb, hEntry);
                }
            }

            if (pFb->pDisplay)
                pFb->pDisplay->RegionsChanged(pFb);
        }

        if (hEntry)
        {
            if (CrVrScrCompositorEntryIsUsed(&hEntry->Entry))
            {
                if (pFb->pDisplay)
                    pFb->pDisplay->EntryTexChanged(pFb, hEntry);

                CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(&hEntry->Entry);
                if (pTex)
                    CrTdBltDataInvalidateNe(pTex);
            }
        }
    }
    else
        WARN(("CrVrScrCompositorEntryRegionsSet failed, rc %d", rc));

    return rc;
}

const struct VBOXVR_SCR_COMPOSITOR_ENTRY* CrFbEntryGetCompositorEntry(HCR_FRAMEBUFFER_ENTRY hEntry)
{
    return &hEntry->Entry;
}

HCR_FRAMEBUFFER_ENTRY CrFbEntryFromCompositorEntry(const struct VBOXVR_SCR_COMPOSITOR_ENTRY* pCEntry)
{
    return RT_FROM_MEMBER(pCEntry, CR_FRAMEBUFFER_ENTRY, Entry);
}

void CrFbVisitCreatedEntries(HCR_FRAMEBUFFER hFb, PFNCR_FRAMEBUFFER_ENTRIES_VISITOR_CB pfnVisitorCb, void *pvContext)
{
    HCR_FRAMEBUFFER_ENTRY hEntry, hNext;
    RTListForEachSafe(&hFb->EntriesList, hEntry, hNext, CR_FRAMEBUFFER_ENTRY, Node)
    {
        if (hEntry->Flags.fCreateNotified)
        {
            if (!pfnVisitorCb(hFb, hEntry, pvContext))
                return;
        }
    }
}


CRHTABLE_HANDLE CrFbDDataAllocSlot(CR_FRAMEBUFFER *pFb)
{
    return CrHTablePut(&pFb->SlotTable, (void*)1);
}

void CrFbDDataReleaseSlot(CR_FRAMEBUFFER *pFb, CRHTABLE_HANDLE hSlot, PFNCR_FRAMEBUFFER_SLOT_RELEASE_CB pfnReleaseCb, void *pvContext)
{
    HCR_FRAMEBUFFER_ENTRY hEntry, hNext;
    RTListForEachSafe(&pFb->EntriesList, hEntry, hNext, CR_FRAMEBUFFER_ENTRY, Node)
    {
        if (CrFbDDataEntryGet(hEntry, hSlot))
        {
            if (pfnReleaseCb)
                pfnReleaseCb(pFb, hEntry, pvContext);

            CrFbDDataEntryClear(hEntry, hSlot);
        }
    }

    CrHTableRemove(&pFb->SlotTable, hSlot);
}

int CrFbDDataEntryPut(HCR_FRAMEBUFFER_ENTRY hEntry, CRHTABLE_HANDLE hSlot, void *pvData)
{
    return CrHTablePutToSlot(&hEntry->HTable, hSlot, pvData);
}

void* CrFbDDataEntryClear(HCR_FRAMEBUFFER_ENTRY hEntry, CRHTABLE_HANDLE hSlot)
{
    return CrHTableRemove(&hEntry->HTable, hSlot);
}

void* CrFbDDataEntryGet(HCR_FRAMEBUFFER_ENTRY hEntry, CRHTABLE_HANDLE hSlot)
{
    return CrHTableGet(&hEntry->HTable, hSlot);
}

int CrPMgrDisable()
{
    if (!g_CrPresenter.fEnabled)
        return VINF_SUCCESS;

    g_CrPresenter.u32DisabledDisplayMode = g_CrPresenter.u32DisplayMode;

    int rc = crPMgrModeModifyGlobal(0, CR_PMGR_MODE_WINDOW);
    if (RT_FAILURE(rc))
    {
        WARN(("crPMgrModeModifyGlobal failed %d", rc));
        return rc;
    }

    crPMgrCleanUnusedDisplays();

    g_CrPresenter.fEnabled = false;

    return VINF_SUCCESS;
}

int CrPMgrEnable()
{
    if (g_CrPresenter.fEnabled)
        return VINF_SUCCESS;

    g_CrPresenter.fEnabled = true;

    int rc = crPMgrModeModifyGlobal(g_CrPresenter.u32DisabledDisplayMode, 0);
    if (RT_FAILURE(rc))
    {
        WARN(("crPMgrModeModifyGlobal failed %d", rc));
        g_CrPresenter.fEnabled = false;
        return rc;
    }

    g_CrPresenter.u32DisabledDisplayMode = 0;

    return VINF_SUCCESS;
}

int CrPMgrInit()
{
    int rc = VINF_SUCCESS;
    memset(&g_CrPresenter, 0, sizeof (g_CrPresenter));
    g_CrPresenter.fEnabled = true;
    for (int i = 0; i < RT_ELEMENTS(g_CrPresenter.aDisplayInfos); ++i)
    {
        g_CrPresenter.aDisplayInfos[i].u32Id = i;
        g_CrPresenter.aDisplayInfos[i].iFb = -1;

        g_CrPresenter.aFbInfos[i].u32Id = i;
    }

    g_CrPresenter.pFbTexMap = crAllocHashtable();
    if (g_CrPresenter.pFbTexMap)
    {
#ifndef VBOXVDBG_MEMCACHE_DISABLE
        rc = RTMemCacheCreate(&g_CrPresenter.FbEntryLookasideList, sizeof (CR_FRAMEBUFFER_ENTRY),
                                0, /* size_t cbAlignment */
                                UINT32_MAX, /* uint32_t cMaxObjects */
                                NULL, /* PFNMEMCACHECTOR pfnCtor*/
                                NULL, /* PFNMEMCACHEDTOR pfnDtor*/
                                NULL, /* void *pvUser*/
                                0 /* uint32_t fFlags*/
                                );
        if (RT_SUCCESS(rc))
        {
            rc = RTMemCacheCreate(&g_CrPresenter.FbTexLookasideList, sizeof (CR_FBTEX),
                                        0, /* size_t cbAlignment */
                                        UINT32_MAX, /* uint32_t cMaxObjects */
                                        NULL, /* PFNMEMCACHECTOR pfnCtor*/
                                        NULL, /* PFNMEMCACHEDTOR pfnDtor*/
                                        NULL, /* void *pvUser*/
                                        0 /* uint32_t fFlags*/
                                        );
            if (RT_SUCCESS(rc))
            {
                rc = RTMemCacheCreate(&g_CrPresenter.CEntryLookasideList, sizeof (VBOXVR_SCR_COMPOSITOR_ENTRY),
                                            0, /* size_t cbAlignment */
                                            UINT32_MAX, /* uint32_t cMaxObjects */
                                            NULL, /* PFNMEMCACHECTOR pfnCtor*/
                                            NULL, /* PFNMEMCACHEDTOR pfnDtor*/
                                            NULL, /* void *pvUser*/
                                            0 /* uint32_t fFlags*/
                                            );
                if (RT_SUCCESS(rc))
                {
#endif
                    rc = crPMgrModeModifyGlobal(CR_PMGR_MODE_WINDOW, 0);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    else
                        WARN(("crPMgrModeModifyGlobal failed rc %d", rc));
#ifndef VBOXVDBG_MEMCACHE_DISABLE
                    RTMemCacheDestroy(g_CrPresenter.CEntryLookasideList);
                }
                else
                    WARN(("RTMemCacheCreate failed rc %d", rc));

                RTMemCacheDestroy(g_CrPresenter.FbTexLookasideList);
            }
            else
                WARN(("RTMemCacheCreate failed rc %d", rc));

            RTMemCacheDestroy(g_CrPresenter.FbEntryLookasideList);
        }
        else
            WARN(("RTMemCacheCreate failed rc %d", rc));
#endif
    }
    else
    {
        WARN(("crAllocHashtable failed"));
        rc = VERR_NO_MEMORY;
    }
    return rc;
}

void CrPMgrTerm()
{
    crPMgrModeModifyGlobal(0, CR_PMGR_MODE_ALL);

    HCR_FRAMEBUFFER hFb;

    for (hFb = CrPMgrFbGetFirstInitialized();
            hFb;
            hFb = CrPMgrFbGetNextInitialized(hFb))
    {
        uint32_t iFb = CrFbGetScreenInfo(hFb)->u32ViewIndex;
        CrFbDisplaySet(hFb, NULL);
        CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[iFb];
        if (pFbInfo->pDpComposite)
        {
            delete pFbInfo->pDpComposite;
            pFbInfo->pDpComposite = NULL;
        }

        CrFbTerm(hFb);
    }

    crPMgrCleanUnusedDisplays();

#ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMemCacheDestroy(g_CrPresenter.FbEntryLookasideList);
    RTMemCacheDestroy(g_CrPresenter.FbTexLookasideList);
    RTMemCacheDestroy(g_CrPresenter.CEntryLookasideList);
#endif
    crFreeHashtable(g_CrPresenter.pFbTexMap, NULL);

    if (g_CrPresenter.pvTmpBuf)
        RTMemFree(g_CrPresenter.pvTmpBuf);

    if (g_CrPresenter.pvTmpBuf2)
        RTMemFree(g_CrPresenter.pvTmpBuf2);

    memset(&g_CrPresenter, 0, sizeof (g_CrPresenter));
}

HCR_FRAMEBUFFER CrPMgrFbGet(uint32_t idFb)
{
    if (idFb >= CR_MAX_GUEST_MONITORS)
    {
        WARN(("invalid idFb %d", idFb));
        return NULL;
    }

    if (!CrFBmIsSet(&g_CrPresenter.FramebufferInitMap, idFb))
    {
        CrFbInit(&g_CrPresenter.aFramebuffers[idFb], idFb);
        CrFBmSetAtomic(&g_CrPresenter.FramebufferInitMap, idFb);
    }
    else
        Assert(g_CrPresenter.aFramebuffers[idFb].ScreenInfo.u32ViewIndex == idFb);

    return &g_CrPresenter.aFramebuffers[idFb];
}

HCR_FRAMEBUFFER CrPMgrFbGetInitialized(uint32_t idFb)
{
    if (idFb >= CR_MAX_GUEST_MONITORS)
    {
        WARN(("invalid idFb %d", idFb));
        return NULL;
    }

    if (!CrFBmIsSet(&g_CrPresenter.FramebufferInitMap, idFb))
    {
        return NULL;
    }
    else
        Assert(g_CrPresenter.aFramebuffers[idFb].ScreenInfo.u32ViewIndex == idFb);

    return &g_CrPresenter.aFramebuffers[idFb];
}

HCR_FRAMEBUFFER CrPMgrFbGetEnabled(uint32_t idFb)
{
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetInitialized(idFb);

    if(hFb && CrFbIsEnabled(hFb))
        return hFb;

    return NULL;
}

HCR_FRAMEBUFFER CrPMgrFbGetEnabledForScreen(uint32_t idScreen)
{
    if (idScreen >= (uint32_t)cr_server.screenCount)
    {
        WARN(("invalid target id"));
        return NULL;
    }

    const CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[idScreen];
    if (pDpInfo->iFb < 0)
        return NULL;

    return CrPMgrFbGetEnabled(pDpInfo->iFb);
}

static HCR_FRAMEBUFFER crPMgrFbGetNextEnabled(uint32_t i)
{
    for (;i < (uint32_t)cr_server.screenCount; ++i)
    {
        HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabled(i);
        if (hFb)
            return hFb;
    }

    return NULL;
}

static HCR_FRAMEBUFFER crPMgrFbGetNextInitialized(uint32_t i)
{
    for (;i < (uint32_t)cr_server.screenCount; ++i)
    {
        HCR_FRAMEBUFFER hFb = CrPMgrFbGetInitialized(i);
        if (hFb)
            return hFb;
    }

    return NULL;
}

HCR_FRAMEBUFFER CrPMgrFbGetFirstEnabled()
{
    HCR_FRAMEBUFFER hFb = crPMgrFbGetNextEnabled(0);
//    if (!hFb)
//        WARN(("no enabled framebuffer found"));
    return hFb;
}

HCR_FRAMEBUFFER CrPMgrFbGetNextEnabled(HCR_FRAMEBUFFER hFb)
{
    return crPMgrFbGetNextEnabled(hFb->ScreenInfo.u32ViewIndex+1);
}

HCR_FRAMEBUFFER CrPMgrFbGetFirstInitialized()
{
    HCR_FRAMEBUFFER hFb = crPMgrFbGetNextInitialized(0);
//    if (!hFb)
//        WARN(("no initialized framebuffer found"));
    return hFb;
}

HCR_FRAMEBUFFER CrPMgrFbGetNextInitialized(HCR_FRAMEBUFFER hFb)
{
    return crPMgrFbGetNextInitialized(hFb->ScreenInfo.u32ViewIndex+1);
}

HCR_FRAMEBUFFER CrPMgrFbGetEnabledByVramStart(VBOXCMDVBVAOFFSET offVRAM)
{
    for (HCR_FRAMEBUFFER hFb = CrPMgrFbGetFirstEnabled();
            hFb;
            hFb = CrPMgrFbGetNextEnabled(hFb))
    {
        const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hFb);
        if (pScreen->u32StartOffset == offVRAM)
            return hFb;
    }

    return NULL;
}


static uint32_t crPMgrModeAdjustVal(uint32_t u32Mode)
{
    u32Mode = CR_PMGR_MODE_ALL & u32Mode;
    if (CR_PMGR_MODE_ROOTVR & u32Mode)
        u32Mode &= ~CR_PMGR_MODE_WINDOW;
    return u32Mode;
}

static int crPMgrCheckInitWindowDisplays(uint32_t idScreen)
{
#ifdef CR_SERVER_WITH_CLIENT_CALLOUTS
    CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[idScreen];
    if (pDpInfo->iFb >= 0)
    {
        uint32_t u32ModeAdd = g_CrPresenter.u32DisplayMode & (CR_PMGR_MODE_WINDOW | CR_PMGR_MODE_ROOTVR);
        int rc = crPMgrFbConnectTargetDisplays(&g_CrPresenter.aFramebuffers[pDpInfo->iFb], pDpInfo, u32ModeAdd);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbConnectTargetDisplays failed %d", rc));
            return rc;
        }
    }
#endif
    return VINF_SUCCESS;
}

extern "C" DECLEXPORT(int) VBoxOglSetScaleFactor(uint32_t idScreen, double dScaleFactorW, double dScaleFactorH)
{
    if (idScreen >= CR_MAX_GUEST_MONITORS)
    {
        crDebug("Can't set scale factor because specified screen ID (%u) is out of range (max=%d).", idScreen, CR_MAX_GUEST_MONITORS);
        return VERR_INVALID_PARAMETER;
    }

    CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[idScreen];
    if (pDpInfo->pDpWin)
    {
        CrFbWindow *pWin = pDpInfo->pDpWin->getWindow();
        if (pWin)
        {
            bool rc;
            crDebug("Set scale factor for initialized display.");
            rc = pWin->SetScaleFactor((GLdouble)dScaleFactorW, (GLdouble)dScaleFactorH);
            return rc ? 0 : VERR_LOCK_FAILED;
        }
        else
            crDebug("Can't apply scale factor at the moment bacause overlay window obgect not yet created. Will be chached.");
    }
    else
        crDebug("Can't apply scale factor at the moment bacause display not yet initialized. Will be chached.");

    /* Display output not yet initialized. Let's cache values. */
    pDpInfo->dInitialScaleFactorW = dScaleFactorW;
    pDpInfo->dInitialScaleFactorH = dScaleFactorH;

    return 0;
}

int CrPMgrScreenChanged(uint32_t idScreen)
{
    if (idScreen >= CR_MAX_GUEST_MONITORS)
    {
        WARN(("invalid idScreen %d", idScreen));
        return VERR_INVALID_PARAMETER;
    }

    int rc = VINF_SUCCESS;

    CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[idScreen];
    HCR_FRAMEBUFFER    hFb     = pDpInfo->iFb >= 0 ? CrPMgrFbGet(pDpInfo->iFb) : NULL;

    if (hFb && CrFbIsUpdating(hFb))
    {
        WARN(("trying to update viewport while framebuffer is being updated"));
        return VERR_INVALID_STATE;
    }

    if (pDpInfo->pDpWin)
    {
        CRASSERT(pDpInfo->pDpWin->getWindow());

        rc = pDpInfo->pDpWin->UpdateBegin(hFb);
        if (RT_SUCCESS(rc))
        {
            pDpInfo->pDpWin->reparent(cr_server.screen[idScreen].winID);
            pDpInfo->pDpWin->UpdateEnd(hFb);
        }
    }
    else
    {
        if (pDpInfo->pWindow)
        {
            rc = pDpInfo->pWindow->UpdateBegin();
            if (RT_SUCCESS(rc))
            {
                rc = pDpInfo->pWindow->SetVisible(false);
                if (RT_SUCCESS(rc))
                    rc = pDpInfo->pWindow->Reparent(cr_server.screen[idScreen].winID);

                pDpInfo->pWindow->UpdateEnd();
            }
        }

        if (RT_SUCCESS(rc))
            rc = crPMgrCheckInitWindowDisplays(idScreen);
    }

    CRASSERT(!rc);

    return rc;
}

int CrPMgrViewportUpdate(uint32_t idScreen)
{
    if (idScreen >= CR_MAX_GUEST_MONITORS)
    {
        WARN(("invalid idScreen %d", idScreen));
        return VERR_INVALID_PARAMETER;
    }

    CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[idScreen];
    if (pDpInfo->iFb >= 0)
    {
        HCR_FRAMEBUFFER hFb = CrPMgrFbGet(pDpInfo->iFb);
        if (CrFbIsUpdating(hFb))
        {
            WARN(("trying to update viewport while framebuffer is being updated"));
            return VERR_INVALID_STATE;
        }

        if (pDpInfo->pDpWin)
        {
            CRASSERT(pDpInfo->pDpWin->getWindow());
            int rc = pDpInfo->pDpWin->UpdateBegin(hFb);
            if (RT_SUCCESS(rc))
            {
                pDpInfo->pDpWin->setViewportRect(&cr_server.screenVieport[idScreen].Rect);
                pDpInfo->pDpWin->UpdateEnd(hFb);
            }
            else
                WARN(("UpdateBegin failed %d", rc));
        }
    }

    return VINF_SUCCESS;
}

static int crPMgrFbDisconnectDisplay(HCR_FRAMEBUFFER hFb, CrFbDisplayBase *pDp)
{
    if (pDp->getFramebuffer() != hFb)
        return VINF_SUCCESS;

    CrFbDisplayBase * pCurDp = (CrFbDisplayBase*)CrFbDisplayGet(hFb);
    if (!pCurDp)
    {
        WARN(("no display set, unexpected"));
        return VERR_INTERNAL_ERROR;
    }

    if (pCurDp == pDp)
    {
        pDp->setFramebuffer(NULL);
        CrFbDisplaySet(hFb, NULL);
        return VINF_SUCCESS;
    }

    uint32_t idFb = CrFbGetScreenInfo(hFb)->u32ViewIndex;
    CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[idFb];
    if (pFbInfo->pDpComposite != pCurDp)
    {
        WARN(("misconfig, expectig the curret framebuffer to be present, and thus composite is expected"));
        return VERR_INTERNAL_ERROR;
    }

    if (pDp->getContainer() == pFbInfo->pDpComposite)
    {
        pFbInfo->pDpComposite->remove(pDp);
        uint32_t cDisplays = pFbInfo->pDpComposite->getDisplayCount();
        if (cDisplays <= 1)
        {
            Assert(cDisplays == 1);
            CrFbDisplayBase *pDpFirst = pFbInfo->pDpComposite->first();
            if (pDpFirst)
                pFbInfo->pDpComposite->remove(pDpFirst, false);
            CrFbDisplaySet(hFb, pDpFirst);
        }
        return VINF_SUCCESS;
    }

    WARN(("misconfig"));
    return VERR_INTERNAL_ERROR;
}

static int crPMgrFbConnectDisplay(HCR_FRAMEBUFFER hFb, CrFbDisplayBase *pDp)
{
    if (pDp->getFramebuffer() == hFb)
        return VINF_SUCCESS;

    CrFbDisplayBase * pCurDp = (CrFbDisplayBase*)CrFbDisplayGet(hFb);
    if (!pCurDp)
    {
        pDp->setFramebuffer(hFb);
        CrFbDisplaySet(hFb, pDp);
        return VINF_SUCCESS;
    }

    if (pCurDp == pDp)
    {
        WARN(("misconfig, current framebuffer is not expected to be set"));
        return VERR_INTERNAL_ERROR;
    }

    uint32_t idFb = CrFbGetScreenInfo(hFb)->u32ViewIndex;
    CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[idFb];
    if (pFbInfo->pDpComposite != pCurDp)
    {
        if (!pFbInfo->pDpComposite)
        {
            pFbInfo->pDpComposite = new CrFbDisplayComposite();
            pFbInfo->pDpComposite->setFramebuffer(hFb);
        }

        pFbInfo->pDpComposite->add(pCurDp);
        CrFbDisplaySet(hFb, pFbInfo->pDpComposite);
    }

    pFbInfo->pDpComposite->add(pDp);
    return VINF_SUCCESS;
}

static int crPMgrFbDisconnectTarget(HCR_FRAMEBUFFER hFb, uint32_t i)
{
    uint32_t idFb = CrFbGetScreenInfo(hFb)->u32ViewIndex;
    CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[idFb];
    CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[i];
    if (pDpInfo->iFb != idFb)
    {
        WARN(("target not connected"));
        Assert(!ASMBitTest(pFbInfo->aTargetMap, i));
        return VINF_SUCCESS;
    }

    Assert(ASMBitTest(pFbInfo->aTargetMap, i));

    int rc = VINF_SUCCESS;
    if (pDpInfo->pDpVrdp)
    {
        rc = crPMgrFbDisconnectDisplay(hFb, pDpInfo->pDpVrdp);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbDisconnectDisplay failed %d", rc));
            return rc;
        }
    }

    if (pDpInfo->pDpWinRootVr)
    {
#ifdef CR_SERVER_WITH_CLIENT_CALLOUTS
        CrFbWindow *pWindow = pDpInfo->pDpWinRootVr->windowDetach(false);
        Assert(pWindow == pDpInfo->pWindow);
#endif
        rc = crPMgrFbDisconnectDisplay(hFb, pDpInfo->pDpWinRootVr);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbDisconnectDisplay failed %d", rc));
            return rc;
        }
    }
    else if (pDpInfo->pDpWin)
    {
#ifdef CR_SERVER_WITH_CLIENT_CALLOUTS
        CrFbWindow *pWindow = pDpInfo->pDpWin->windowDetach(false);
        Assert(pWindow == pDpInfo->pWindow);
#endif
        rc = crPMgrFbDisconnectDisplay(hFb, pDpInfo->pDpWin);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbDisconnectDisplay failed %d", rc));
            return rc;
        }
    }

    ASMBitClear(pFbInfo->aTargetMap, i);
    pDpInfo->iFb = -1;

    return VINF_SUCCESS;
}

static void crPMgrDpWinRootVrCreate(CR_FBDISPLAY_INFO *pDpInfo)
{
    if (!pDpInfo->pDpWinRootVr)
    {
        if (pDpInfo->pDpWin)
        {
            CrFbWindow *pWin = pDpInfo->pDpWin->windowDetach();
            CRASSERT(pWin);
            Assert(pWin == pDpInfo->pWindow);
            delete pDpInfo->pDpWin;
            pDpInfo->pDpWin = NULL;
        }
        else if (!pDpInfo->pWindow)
        {
            pDpInfo->pWindow = new CrFbWindow(0);
        }

        pDpInfo->pDpWinRootVr = new CrFbDisplayWindowRootVr(&cr_server.screenVieport[pDpInfo->u32Id].Rect, cr_server.screen[pDpInfo->u32Id].winID);
        pDpInfo->pDpWin = pDpInfo->pDpWinRootVr;
        pDpInfo->pDpWinRootVr->windowAttach(pDpInfo->pWindow);

        /* Set scale factor once it was previously cached when display output was not yet initialized. */
        if (pDpInfo->dInitialScaleFactorW || pDpInfo->dInitialScaleFactorH)
        {
            crDebug("Set cached scale factor for seamless mode.");
            pDpInfo->pWindow->SetScaleFactor((GLdouble)pDpInfo->dInitialScaleFactorW, (GLdouble)pDpInfo->dInitialScaleFactorH);
            /* Invalidate cache. */
            pDpInfo->dInitialScaleFactorW = pDpInfo->dInitialScaleFactorH = 0;
        }
    }
}

static void crPMgrDpWinCreate(CR_FBDISPLAY_INFO *pDpInfo)
{
    if (pDpInfo->pDpWinRootVr)
    {
        CRASSERT(pDpInfo->pDpWinRootVr == pDpInfo->pDpWin);
        CrFbWindow *pWin = pDpInfo->pDpWin->windowDetach();
        CRASSERT(pWin);
        Assert(pWin == pDpInfo->pWindow);
        delete pDpInfo->pDpWinRootVr;
        pDpInfo->pDpWinRootVr = NULL;
        pDpInfo->pDpWin = NULL;
    }

    if (!pDpInfo->pDpWin)
    {
        if (!pDpInfo->pWindow)
            pDpInfo->pWindow = new CrFbWindow(0);

        pDpInfo->pDpWin = new CrFbDisplayWindow(&cr_server.screenVieport[pDpInfo->u32Id].Rect, cr_server.screen[pDpInfo->u32Id].winID);
        pDpInfo->pDpWin->windowAttach(pDpInfo->pWindow);

        /* Set scale factor once it was previously cached when display output was not yet initialized. */
        if (pDpInfo->dInitialScaleFactorW || pDpInfo->dInitialScaleFactorH)
        {
            crDebug("Set cached scale factor for host window.");
            pDpInfo->pWindow->SetScaleFactor((GLdouble)pDpInfo->dInitialScaleFactorW, (GLdouble)pDpInfo->dInitialScaleFactorH);
            /* Invalidate cache. */
            pDpInfo->dInitialScaleFactorW = pDpInfo->dInitialScaleFactorH = 0;
        }
    }
}

static int crPMgrFbDisconnectTargetDisplays(HCR_FRAMEBUFFER hFb, CR_FBDISPLAY_INFO *pDpInfo, uint32_t u32ModeRemove)
{
    int rc = VINF_SUCCESS;
    if (u32ModeRemove & CR_PMGR_MODE_ROOTVR)
    {
        if (pDpInfo->pDpWinRootVr)
        {
#ifdef CR_SERVER_WITH_CLIENT_CALLOUTS
            CrFbWindow *pWindow = pDpInfo->pDpWinRootVr->windowDetach(false);
            Assert(pWindow == pDpInfo->pWindow);
#endif
            CRASSERT(pDpInfo->pDpWin == pDpInfo->pDpWinRootVr);
            rc = crPMgrFbDisconnectDisplay(hFb, pDpInfo->pDpWinRootVr);
            if (RT_FAILURE(rc))
            {
                WARN(("crPMgrFbDisconnectDisplay pDpWinRootVr failed %d", rc));
                return rc;
            }
        }
    }
    else if (u32ModeRemove & CR_PMGR_MODE_WINDOW)
    {
        CRASSERT(!pDpInfo->pDpWinRootVr);
        if (pDpInfo->pDpWin)
        {
#ifdef CR_SERVER_WITH_CLIENT_CALLOUTS
            CrFbWindow *pWindow = pDpInfo->pDpWin->windowDetach(false);
            Assert(pWindow == pDpInfo->pWindow);
#endif
            rc = crPMgrFbDisconnectDisplay(hFb, pDpInfo->pDpWin);
            if (RT_FAILURE(rc))
            {
                WARN(("crPMgrFbDisconnectDisplay pDpWin failed %d", rc));
                return rc;
            }
        }
    }

    if (u32ModeRemove & CR_PMGR_MODE_VRDP)
    {
        if (pDpInfo->pDpVrdp)
        {
            rc = crPMgrFbDisconnectDisplay(hFb, pDpInfo->pDpVrdp);
            if (RT_FAILURE(rc))
            {
                WARN(("crPMgrFbDisconnectDisplay pDpVrdp failed %d", rc));
                return rc;
            }
        }
    }

    pDpInfo->u32DisplayMode &= ~u32ModeRemove;

    return VINF_SUCCESS;
}

static int crPMgrFbConnectTargetDisplays(HCR_FRAMEBUFFER hFb, CR_FBDISPLAY_INFO *pDpInfo, uint32_t u32ModeAdd)
{
    int rc = VINF_SUCCESS;

    if (u32ModeAdd & CR_PMGR_MODE_ROOTVR)
    {
        crPMgrDpWinRootVrCreate(pDpInfo);

        rc = crPMgrFbConnectDisplay(hFb, pDpInfo->pDpWinRootVr);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbConnectDisplay pDpWinRootVr failed %d", rc));
            return rc;
        }
    }
    else if (u32ModeAdd & CR_PMGR_MODE_WINDOW)
    {
        crPMgrDpWinCreate(pDpInfo);

        rc = crPMgrFbConnectDisplay(hFb, pDpInfo->pDpWin);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbConnectDisplay pDpWin failed %d", rc));
            return rc;
        }
    }

    if (u32ModeAdd & CR_PMGR_MODE_VRDP)
    {
        if (!pDpInfo->pDpVrdp)
            pDpInfo->pDpVrdp = new CrFbDisplayVrdp();

        rc = crPMgrFbConnectDisplay(hFb, pDpInfo->pDpVrdp);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbConnectDisplay pDpVrdp failed %d", rc));
            return rc;
        }
    }

    pDpInfo->u32DisplayMode |= u32ModeAdd;

    return VINF_SUCCESS;
}

static int crPMgrFbConnectTarget(HCR_FRAMEBUFFER hFb, uint32_t i)
{
    uint32_t idFb = CrFbGetScreenInfo(hFb)->u32ViewIndex;
    CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[idFb];
    CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[i];
    if (pDpInfo->iFb == idFb)
    {
        WARN(("target not connected"));
        Assert(ASMBitTest(pFbInfo->aTargetMap, i));
        return VINF_SUCCESS;
    }

    Assert(!ASMBitTest(pFbInfo->aTargetMap, i));

    int rc = VINF_SUCCESS;

    if (pDpInfo->iFb != -1)
    {
        Assert(pDpInfo->iFb < cr_server.screenCount);
        HCR_FRAMEBUFFER hAssignedFb = CrPMgrFbGet(pDpInfo->iFb);
        Assert(hAssignedFb);
        rc = crPMgrFbDisconnectTarget(hAssignedFb, i);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbDisconnectTarget failed %d", rc));
            return rc;
        }
    }

    rc = crPMgrFbConnectTargetDisplays(hFb, pDpInfo, g_CrPresenter.u32DisplayMode
#ifdef CR_SERVER_WITH_CLIENT_CALLOUTS
            & ~(CR_PMGR_MODE_WINDOW | CR_PMGR_MODE_ROOTVR)
#endif
            );
    if (RT_FAILURE(rc))
    {
        WARN(("crPMgrFbConnectTargetDisplays failed %d", rc));
        return rc;
    }

    ASMBitSet(pFbInfo->aTargetMap, i);
    pDpInfo->iFb = idFb;

    return VINF_SUCCESS;
}

static int crPMgrFbDisconnect(HCR_FRAMEBUFFER hFb, const uint32_t *pTargetMap)
{
    int rc = VINF_SUCCESS;
    for (int i = ASMBitFirstSet(pTargetMap, cr_server.screenCount);
            i >= 0;
            i = ASMBitNextSet(pTargetMap, cr_server.screenCount, i))
    {
        rc = crPMgrFbDisconnectTarget(hFb, (uint32_t)i);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbDisconnectTarget failed %d", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}

static int crPMgrFbConnect(HCR_FRAMEBUFFER hFb, const uint32_t *pTargetMap)
{
    int rc = VINF_SUCCESS;
    for (int i = ASMBitFirstSet(pTargetMap, cr_server.screenCount);
            i >= 0;
            i = ASMBitNextSet(pTargetMap, cr_server.screenCount, i))
    {
        rc = crPMgrFbConnectTarget(hFb, (uint32_t)i);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbConnectTarget failed %d", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}

static int crPMgrModeModifyTarget(HCR_FRAMEBUFFER hFb, uint32_t iDisplay, uint32_t u32ModeAdd, uint32_t u32ModeRemove)
{
    CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[iDisplay];
    int rc = crPMgrFbDisconnectTargetDisplays(hFb, pDpInfo, u32ModeRemove);
    if (RT_FAILURE(rc))
    {
        WARN(("crPMgrFbDisconnectTargetDisplays failed %d", rc));
        return rc;
    }

    rc = crPMgrFbConnectTargetDisplays(hFb, pDpInfo, u32ModeAdd);
    if (RT_FAILURE(rc))
    {
        WARN(("crPMgrFbConnectTargetDisplays failed %d", rc));
        return rc;
    }

    return VINF_SUCCESS;
}

static int crPMgrModeModify(HCR_FRAMEBUFFER hFb, uint32_t u32ModeAdd, uint32_t u32ModeRemove)
{
    int rc = VINF_SUCCESS;
    uint32_t idFb = CrFbGetScreenInfo(hFb)->u32ViewIndex;
    CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[idFb];
    for (int i = ASMBitFirstSet(pFbInfo->aTargetMap, cr_server.screenCount);
            i >= 0;
            i = ASMBitNextSet(pFbInfo->aTargetMap, cr_server.screenCount, i))
    {
        rc = crPMgrModeModifyTarget(hFb, (uint32_t)i, u32ModeAdd, u32ModeRemove);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrModeModifyTarget failed %d", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}

static void crPMgrCleanUnusedDisplays()
{
    for (int i = 0; i < cr_server.screenCount; ++i)
    {
        CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[i];

        if (pDpInfo->pDpWinRootVr)
        {
            if (!pDpInfo->pDpWinRootVr->getFramebuffer())
            {
                pDpInfo->pDpWinRootVr->windowDetach(false);
                delete pDpInfo->pDpWinRootVr;
                pDpInfo->pDpWinRootVr = NULL;
                pDpInfo->pDpWin = NULL;
                if (pDpInfo->pWindow)
                {
                    delete pDpInfo->pWindow;
                    pDpInfo->pWindow = NULL;
                }
            }
            else
                WARN(("pDpWinRootVr is used"));
        }
        else if (pDpInfo->pDpWin)
        {
            if (!pDpInfo->pDpWin->getFramebuffer())
            {
                pDpInfo->pDpWin->windowDetach(false);
                delete pDpInfo->pDpWin;
                pDpInfo->pDpWin = NULL;
                if (pDpInfo->pWindow)
                {
                    delete pDpInfo->pWindow;
                    pDpInfo->pWindow = NULL;
                }
            }
            else
                WARN(("pDpWin is used"));
        }

        if (pDpInfo->pDpVrdp)
        {
            if (!pDpInfo->pDpVrdp->getFramebuffer())
            {
                delete pDpInfo->pDpVrdp;
                pDpInfo->pDpVrdp = NULL;
            }
            else
                WARN(("pDpVrdp is used"));
        }
    }
}

static int crPMgrModeModifyGlobal(uint32_t u32ModeAdd, uint32_t u32ModeRemove)
{
    uint32_t u32InternalMode = g_CrPresenter.fEnabled ? g_CrPresenter.u32DisplayMode : g_CrPresenter.u32DisabledDisplayMode;

    u32ModeRemove = ((u32ModeRemove | crPMgrModeAdjustVal(u32ModeRemove)) & CR_PMGR_MODE_ALL);
    u32ModeAdd = crPMgrModeAdjustVal(u32ModeAdd);
    u32ModeRemove &= u32InternalMode;
    u32ModeAdd &= ~(u32ModeRemove | u32InternalMode);
    uint32_t u32ModeResulting = ((u32InternalMode | u32ModeAdd) & ~u32ModeRemove);
    uint32_t u32Tmp = crPMgrModeAdjustVal(u32ModeResulting);
    if (u32Tmp != u32ModeResulting)
    {
        u32ModeAdd |= (u32Tmp & ~u32ModeResulting);
        u32ModeRemove |= (~u32Tmp & u32ModeResulting);
        u32ModeResulting = u32Tmp;
        Assert(u32ModeResulting == ((u32InternalMode | u32ModeAdd) & ~u32ModeRemove));
    }
    if (!u32ModeRemove && !u32ModeAdd)
        return VINF_SUCCESS;

    uint32_t u32DisplayMode = (g_CrPresenter.u32DisplayMode | u32ModeAdd) & ~u32ModeRemove;
    if (!g_CrPresenter.fEnabled)
    {
        Assert(g_CrPresenter.u32DisplayMode == 0);
        g_CrPresenter.u32DisabledDisplayMode = u32DisplayMode;
        return VINF_SUCCESS;
    }

    g_CrPresenter.u32DisplayMode = u32DisplayMode;

    /* disabled framebuffers may still have displays attached */
    for (HCR_FRAMEBUFFER hFb = CrPMgrFbGetFirstInitialized();
            hFb;
            hFb = CrPMgrFbGetNextInitialized(hFb))
    {
        crPMgrModeModify(hFb, u32ModeAdd, u32ModeRemove);
    }

    return VINF_SUCCESS;
}

int CrPMgrClearRegionsGlobal()
{
    for (HCR_FRAMEBUFFER hFb = CrPMgrFbGetFirstEnabled();
            hFb;
            hFb = CrPMgrFbGetNextEnabled(hFb))
    {
        int rc = CrFbUpdateBegin(hFb);
        if (RT_SUCCESS(rc))
        {
            rc = CrFbRegionsClear(hFb);
            if (RT_FAILURE(rc))
            {
                WARN(("CrFbRegionsClear failed %d", rc));
            }

            CrFbUpdateEnd(hFb);
        }
    }

    return VINF_SUCCESS;
}

int CrPMgrModeVrdp(bool fEnable)
{
    uint32_t u32ModeAdd, u32ModeRemove;
    if (fEnable)
    {
        u32ModeAdd = CR_PMGR_MODE_VRDP;
        u32ModeRemove = 0;
    }
    else
    {
        u32ModeAdd = 0;
        u32ModeRemove = CR_PMGR_MODE_VRDP;
    }
    return crPMgrModeModifyGlobal(u32ModeAdd, u32ModeRemove);
}

int CrPMgrModeRootVr(bool fEnable)
{
    uint32_t u32ModeAdd, u32ModeRemove;
    if (fEnable)
    {
        u32ModeAdd = CR_PMGR_MODE_ROOTVR;
        u32ModeRemove = CR_PMGR_MODE_WINDOW;
    }
    else
    {
        u32ModeAdd = CR_PMGR_MODE_WINDOW;
        u32ModeRemove = CR_PMGR_MODE_ROOTVR;
    }

    return crPMgrModeModifyGlobal(u32ModeAdd, u32ModeRemove);
}

int CrPMgrModeWinVisible(bool fEnable)
{
    if (!g_CrPresenter.fWindowsForceHidden == !!fEnable)
        return VINF_SUCCESS;

    g_CrPresenter.fWindowsForceHidden = !fEnable;

    for (int i = 0; i < cr_server.screenCount; ++i)
    {
        CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[i];

        if (pDpInfo->pDpWin)
            pDpInfo->pDpWin->winVisibilityChanged();
    }

    return VINF_SUCCESS;
}

int CrPMgrRootVrUpdate()
{
    for (HCR_FRAMEBUFFER hFb = CrPMgrFbGetFirstEnabled();
            hFb;
            hFb = CrPMgrFbGetNextEnabled(hFb))
    {
        if (!CrFbHas3DData(hFb))
            continue;

        uint32_t idFb = CrFbGetScreenInfo(hFb)->u32ViewIndex;
        CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[idFb];
        int rc = CrFbUpdateBegin(hFb);
        if (RT_SUCCESS(rc))
        {
            for (int i = ASMBitFirstSet(pFbInfo->aTargetMap, cr_server.screenCount);
                    i >= 0;
                    i = ASMBitNextSet(pFbInfo->aTargetMap, cr_server.screenCount, i))
            {
                CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[i];
                Assert(pDpInfo->iFb == (int32_t)idFb);

                pDpInfo->pDpWinRootVr->RegionsChanged(hFb);
            }

            CrFbUpdateEnd(hFb);
        }
        else
            WARN(("CrFbUpdateBegin failed %d", rc));
    }

    return VINF_SUCCESS;
}

/*helper function that calls CrFbUpdateBegin for all enabled framebuffers */
int CrPMgrHlpGlblUpdateBegin(CR_FBMAP *pMap)
{
    CrFBmInit(pMap);
    for (HCR_FRAMEBUFFER hFb = CrPMgrFbGetFirstEnabled();
            hFb;
            hFb = CrPMgrFbGetNextEnabled(hFb))
    {
        int rc = CrFbUpdateBegin(hFb);
        if (!RT_SUCCESS(rc))
        {
            WARN(("UpdateBegin failed, rc %d", rc));
            for (HCR_FRAMEBUFFER hTmpFb = CrPMgrFbGetFirstEnabled();
                        hFb != hTmpFb;
                        hTmpFb = CrPMgrFbGetNextEnabled(hTmpFb))
            {
                CrFbUpdateEnd(hTmpFb);
                CrFBmClear(pMap, CrFbGetScreenInfo(hFb)->u32ViewIndex);
            }
            return rc;
        }

        CrFBmSet(pMap, CrFbGetScreenInfo(hFb)->u32ViewIndex);
    }

    return VINF_SUCCESS;
}

/*helper function that calls CrFbUpdateEnd for all framebuffers being updated */
void CrPMgrHlpGlblUpdateEnd(CR_FBMAP *pMap)
{
    for (uint32_t i = 0; i < (uint32_t)cr_server.screenCount; ++i)
    {
        if (!CrFBmIsSet(pMap, i))
            continue;

        HCR_FRAMEBUFFER hFb = CrPMgrFbGetInitialized(i);
        CRASSERT(hFb);
        CrFbUpdateEnd(hFb);
    }
}

int CrPMgrResize(const struct VBVAINFOSCREEN *pScreen, void *pvVRAM, const uint32_t *pTargetMap)
{
    int rc = VINF_SUCCESS;

    if (pScreen->u32ViewIndex == 0xffffffff)
    {
        /* this is just a request to disable targets, search and disable */
        for (int i = ASMBitFirstSet(pTargetMap, cr_server.screenCount);
                i >= 0;
                i = ASMBitNextSet(pTargetMap, cr_server.screenCount, i))
        {
            CR_FBDISPLAY_INFO *pDpInfo = &g_CrPresenter.aDisplayInfos[i];
            if (pDpInfo->iFb < 0)
                continue;

            Assert(pDpInfo->iFb < cr_server.screenCount);
            HCR_FRAMEBUFFER hAssignedFb = CrPMgrFbGet(pDpInfo->iFb);

            rc = crPMgrFbDisconnectTarget(hAssignedFb, (uint32_t)i);
            if (RT_FAILURE(rc))
            {
                WARN(("crPMgrFbDisconnectTarget failed %d", rc));
                return rc;
            }
        }

        return VINF_SUCCESS;
    }

    HCR_FRAMEBUFFER hFb = CrPMgrFbGet(pScreen->u32ViewIndex);
    if (!hFb)
    {
        WARN(("CrPMgrFbGet failed"));
        return VERR_INVALID_PARAMETER;
    }

    const VBVAINFOSCREEN *pFbScreen = CrFbGetScreenInfo(hFb);
    bool fFbInfoChanged = true;

    if (!memcmp(pFbScreen, pScreen, sizeof (*pScreen)))
    {
        if (!pvVRAM || pvVRAM == CrFbGetVRAM(hFb))
            fFbInfoChanged = false;
    }

    CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[pScreen->u32ViewIndex];

    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aRemovedTargetMap);
    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aAddedTargetMap);

    bool fDisplaysAdded = false, fDisplaysRemoved = false;

    memcpy(aRemovedTargetMap, pFbInfo->aTargetMap, sizeof (aRemovedTargetMap));

    if (pScreen->u16Flags & VBVA_SCREEN_F_DISABLED)
    {
        /* so far there is no need in keeping displays attached to disabled Framebffer,
         * just disconnect everything */
        for (int i = 0; i < RT_ELEMENTS(aRemovedTargetMap); ++i)
        {
            if (aRemovedTargetMap[i])
            {
                fDisplaysRemoved = true;
                break;
            }
        }

        memset(aAddedTargetMap, 0, sizeof (aAddedTargetMap));
    }
    else
    {
        for (int i = 0; i < RT_ELEMENTS(aRemovedTargetMap); ++i)
        {
            aRemovedTargetMap[i] = (aRemovedTargetMap[i] & ~pTargetMap[i]);
            if (aRemovedTargetMap[i])
                fDisplaysRemoved = true;
        }

        memcpy(aAddedTargetMap, pFbInfo->aTargetMap, sizeof (aAddedTargetMap));
        for (int i = 0; i < RT_ELEMENTS(aAddedTargetMap); ++i)
        {
            aAddedTargetMap[i] = (pTargetMap[i] & ~aAddedTargetMap[i]);
            if (aAddedTargetMap[i])
                fDisplaysAdded = true;
        }
    }

    if (!fFbInfoChanged && !fDisplaysRemoved && !fDisplaysAdded)
    {
        crDebug("resize: no changes");
        return VINF_SUCCESS;
    }

    if (fDisplaysRemoved)
    {
        rc = crPMgrFbDisconnect(hFb, aRemovedTargetMap);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbDisconnect failed %d", rc));
            return rc;
        }
    }

    if (fFbInfoChanged)
    {
#ifdef CR_SERVER_WITH_CLIENT_CALLOUTS
        rc = crPMgrModeModify(hFb, 0, CR_PMGR_MODE_WINDOW | CR_PMGR_MODE_ROOTVR);
        if (!RT_SUCCESS(rc))
        {
            WARN(("crPMgrModeModifyTarget failed %d", rc));
            return rc;
        }
#endif
        rc = CrFbUpdateBegin(hFb);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrFbUpdateBegin failed %d", rc));
            return rc;
        }

        crVBoxServerMuralFbResizeBegin(hFb);

        rc = CrFbResize(hFb, pScreen, pvVRAM);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrFbResize failed %d", rc));
        }

        crVBoxServerMuralFbResizeEnd(hFb);

        CrFbUpdateEnd(hFb);
    }

    if (fDisplaysAdded)
    {
        rc = crPMgrFbConnect(hFb, aAddedTargetMap);
        if (RT_FAILURE(rc))
        {
            WARN(("crPMgrFbConnect failed %d", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}

int CrFbEntrySaveState(CR_FRAMEBUFFER *pFb, CR_FRAMEBUFFER_ENTRY *hEntry, PSSMHANDLE pSSM)
{
    const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry = CrFbEntryGetCompositorEntry(hEntry);
    CR_TEXDATA *pTexData = CrVrScrCompositorEntryTexGet(pEntry);
    CR_FBTEX *pFbTex = PCR_FBTEX_FROM_TEX(pTexData);
    int rc = SSMR3PutU32(pSSM, pFbTex->pTobj->id);
    AssertRCReturn(rc, rc);
    uint32_t u32 = 0;

    u32 = CrVrScrCompositorEntryFlagsGet(pEntry);
    rc = SSMR3PutU32(pSSM, u32);
    AssertRCReturn(rc, rc);

    const RTRECT *pRect = CrVrScrCompositorEntryRectGet(pEntry);

    rc = SSMR3PutS32(pSSM, pRect->xLeft);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutS32(pSSM, pRect->yTop);
    AssertRCReturn(rc, rc);
#if 0
    rc = SSMR3PutS32(pSSM, pRect->xRight);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutS32(pSSM, pRect->yBottom);
    AssertRCReturn(rc, rc);
#endif

    rc = CrVrScrCompositorEntryRegionsGet(&pFb->Compositor, pEntry, &u32, NULL, NULL, &pRect);
    AssertRCReturn(rc, rc);

    rc = SSMR3PutU32(pSSM, u32);
    AssertRCReturn(rc, rc);

    if (u32)
    {
        rc = SSMR3PutMem(pSSM, pRect, u32 * sizeof (*pRect));
        AssertRCReturn(rc, rc);
    }
    return rc;
}

int CrFbSaveState(CR_FRAMEBUFFER *pFb, PSSMHANDLE pSSM)
{
    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR Iter;
    CrVrScrCompositorConstIterInit(&pFb->Compositor, &Iter);
    const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry;
    uint32_t u32 = 0;
    while ((pEntry = CrVrScrCompositorConstIterNext(&Iter)) != NULL)
    {
        CR_TEXDATA *pTexData = CrVrScrCompositorEntryTexGet(pEntry);
        CRASSERT(pTexData);
        CR_FBTEX *pFbTex = PCR_FBTEX_FROM_TEX(pTexData);
        if (pFbTex->pTobj)
            ++u32;
    }

    int rc = SSMR3PutU32(pSSM, u32);
    AssertRCReturn(rc, rc);

    CrVrScrCompositorConstIterInit(&pFb->Compositor, &Iter);

    while ((pEntry = CrVrScrCompositorConstIterNext(&Iter)) != NULL)
    {
        CR_TEXDATA *pTexData = CrVrScrCompositorEntryTexGet(pEntry);
        CR_FBTEX *pFbTex = PCR_FBTEX_FROM_TEX(pTexData);
        if (pFbTex->pTobj)
        {
            HCR_FRAMEBUFFER_ENTRY hEntry = CrFbEntryFromCompositorEntry(pEntry);
            rc = CrFbEntrySaveState(pFb, hEntry, pSSM);
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}

int CrPMgrSaveState(PSSMHANDLE pSSM)
{
    int rc;
    int cDisplays = 0, i;

    for (i = 0; i < cr_server.screenCount; ++i)
    {
        if (CrPMgrFbGetEnabled(i))
            ++cDisplays;
    }

    rc = SSMR3PutS32(pSSM, cDisplays);
    AssertRCReturn(rc, rc);

    if (!cDisplays)
        return VINF_SUCCESS;

    rc = SSMR3PutS32(pSSM, cr_server.screenCount);
    AssertRCReturn(rc, rc);

    for (i = 0; i < cr_server.screenCount; ++i)
    {
        CR_FRAMEBUFFER *hFb = CrPMgrFbGetEnabled(i);
        if (hFb)
        {
            Assert(hFb->ScreenInfo.u32ViewIndex == i);
            rc = SSMR3PutU32(pSSM, hFb->ScreenInfo.u32ViewIndex);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutS32(pSSM, hFb->ScreenInfo.i32OriginX);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutS32(pSSM, hFb->ScreenInfo.i32OriginY);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, hFb->ScreenInfo.u32StartOffset);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, hFb->ScreenInfo.u32LineSize);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, hFb->ScreenInfo.u32Width);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, hFb->ScreenInfo.u32Height);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU16(pSSM, hFb->ScreenInfo.u16BitsPerPixel);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU16(pSSM, hFb->ScreenInfo.u16Flags);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, hFb->ScreenInfo.u32StartOffset);
            AssertRCReturn(rc, rc);

            CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[hFb->ScreenInfo.u32ViewIndex];
            rc = SSMR3PutMem(pSSM, pFbInfo->aTargetMap, sizeof (pFbInfo->aTargetMap));
            AssertRCReturn(rc, rc);

            rc = CrFbSaveState(hFb, pSSM);
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}

int CrFbEntryLoadState(CR_FRAMEBUFFER *pFb, PSSMHANDLE pSSM, uint32_t version)
{
    uint32_t texture;
    int  rc = SSMR3GetU32(pSSM, &texture);
    AssertRCReturn(rc, rc);

    uint32_t fFlags;
    rc = SSMR3GetU32(pSSM, &fFlags);
    AssertRCReturn(rc, rc);


    HCR_FRAMEBUFFER_ENTRY hEntry;

    rc = CrFbEntryCreateForTexId(pFb, texture, fFlags, &hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbEntryCreateForTexId Failed"));
        return rc;
    }

    Assert(hEntry);

    const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry = CrFbEntryGetCompositorEntry(hEntry);
    CR_TEXDATA *pTexData = CrVrScrCompositorEntryTexGet(pEntry);
    CR_FBTEX *pFbTex = PCR_FBTEX_FROM_TEX(pTexData);

    RTPOINT Point;
    rc = SSMR3GetS32(pSSM, &Point.x);
    AssertRCReturn(rc, rc);

    rc = SSMR3GetS32(pSSM, &Point.y);
    AssertRCReturn(rc, rc);

    uint32_t cRects;
    rc = SSMR3GetU32(pSSM, &cRects);
    AssertRCReturn(rc, rc);

    RTRECT * pRects = NULL;
    if (cRects)
    {
        pRects = (RTRECT *)crAlloc(cRects * sizeof (*pRects));
        AssertReturn(pRects, VERR_NO_MEMORY);

        rc = SSMR3GetMem(pSSM, pRects, cRects * sizeof (*pRects));
        AssertRCReturn(rc, rc);
    }

    rc = CrFbEntryRegionsSet(pFb, hEntry, &Point, cRects, pRects, false);
    AssertRCReturn(rc, rc);

    if (pRects)
        crFree(pRects);

    CrFbEntryRelease(pFb, hEntry);

    return VINF_SUCCESS;
}

int CrFbLoadState(CR_FRAMEBUFFER *pFb, PSSMHANDLE pSSM, uint32_t version)
{
    uint32_t u32 = 0;
    int rc = SSMR3GetU32(pSSM, &u32);
    AssertRCReturn(rc, rc);

    if (!u32)
        return VINF_SUCCESS;

    rc = CrFbUpdateBegin(pFb);
    AssertRCReturn(rc, rc);

    for (uint32_t i = 0; i < u32; ++i)
    {
        rc = CrFbEntryLoadState(pFb, pSSM, version);
        AssertRCReturn(rc, rc);
    }

    CrFbUpdateEnd(pFb);

    return VINF_SUCCESS;
}

int CrPMgrLoadState(PSSMHANDLE pSSM, uint32_t version)
{
    int rc;
    int cDisplays, screenCount, i;

    rc = SSMR3GetS32(pSSM, &cDisplays);
    AssertRCReturn(rc, rc);

    if (!cDisplays)
        return VINF_SUCCESS;

    rc = SSMR3GetS32(pSSM, &screenCount);
    AssertRCReturn(rc, rc);

    CRASSERT(screenCount == cr_server.screenCount);

    CRScreenInfo screen[CR_MAX_GUEST_MONITORS];

    if (version < SHCROGL_SSM_VERSION_WITH_FB_INFO)
    {
        for (i = 0; i < cr_server.screenCount; ++i)
        {
            rc = SSMR3GetS32(pSSM, &screen[i].x);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetS32(pSSM, &screen[i].y);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &screen[i].w);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &screen[i].h);
            AssertRCReturn(rc, rc);
        }
    }

    for (i = 0; i < cDisplays; ++i)
    {
        int iScreen;

        rc = SSMR3GetS32(pSSM, &iScreen);
        AssertRCReturn(rc, rc);

        CR_FRAMEBUFFER *pFb = CrPMgrFbGet(iScreen);
        Assert(pFb);

        VBVAINFOSCREEN Screen;

        Screen.u32ViewIndex = iScreen;

        VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);

        memset(aTargetMap, 0, sizeof (aTargetMap));
        ASMBitSet(aTargetMap, iScreen);

        if (version < SHCROGL_SSM_VERSION_WITH_FB_INFO)
        {
            memset(&Screen, 0, sizeof (Screen));
            Screen.u32LineSize = 4 * screen[iScreen].w;
            Screen.u32Width = screen[iScreen].w;
            Screen.u32Height = screen[iScreen].h;
            Screen.u16BitsPerPixel = 4;
            Screen.u16Flags = VBVA_SCREEN_F_ACTIVE;
        }
        else
        {
            rc = SSMR3GetS32(pSSM, &Screen.i32OriginX);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetS32(pSSM, &Screen.i32OriginY);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &Screen.u32StartOffset);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &Screen.u32LineSize);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &Screen.u32Width);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &Screen.u32Height);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU16(pSSM, &Screen.u16BitsPerPixel);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU16(pSSM, &Screen.u16Flags);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &Screen.u32StartOffset);
            AssertRCReturn(rc, rc);
            if (Screen.u32StartOffset == 0xffffffff)
            {
                WARN(("not expected offVram"));
                Screen.u32StartOffset = 0;
            }

            if (version >= SHCROGL_SSM_VERSION_WITH_SCREEN_MAP_REORDERED)
            {
                rc = SSMR3GetMem(pSSM, aTargetMap, sizeof (aTargetMap));
                AssertRCReturn(rc, rc);
            }

            if (version == SHCROGL_SSM_VERSION_WITH_SCREEN_MAP)
            {
                VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aEmptyTargetMap);

                memset(aEmptyTargetMap, 0, sizeof (aEmptyTargetMap));

                rc = CrPMgrResize(&Screen, cr_server.fCrCmdEnabled ? NULL : CrFbGetVRAM(pFb), aEmptyTargetMap);
                AssertRCReturn(rc, rc);

                rc = CrFbLoadState(pFb, pSSM, version);
                AssertRCReturn(rc, rc);

                rc = SSMR3GetMem(pSSM, aTargetMap, sizeof (aTargetMap));
                AssertRCReturn(rc, rc);
            }
        }

        rc = CrPMgrResize(&Screen, cr_server.fCrCmdEnabled ? NULL : CrFbGetVRAM(pFb), aTargetMap);
        AssertRCReturn(rc, rc);

        if (version >= SHCROGL_SSM_VERSION_WITH_FB_INFO && version != SHCROGL_SSM_VERSION_WITH_SCREEN_MAP)
        {
            rc = CrFbLoadState(pFb, pSSM, version);
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchVBoxTexPresent(GLuint texture, GLuint cfg, GLint xPos, GLint yPos, GLint cRects, const GLint *pRects)
{
    uint32_t idFb = CR_PRESENT_GET_SCREEN(cfg);
    if (idFb >= CR_MAX_GUEST_MONITORS)
    {
        WARN(("Invalid guest screen"));
        return;
    }

    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabled(idFb);
    if (!hFb)
    {
        WARN(("request to present on disabled framebuffer, ignore"));
        return;
    }

    HCR_FRAMEBUFFER_ENTRY hEntry;
    int rc;
    if (texture)
    {
        rc = CrFbEntryCreateForTexId(hFb, texture, (cfg & CR_PRESENT_FLAG_TEX_NONINVERT_YCOORD) ? 0 : CRBLT_F_INVERT_SRC_YCOORDS, &hEntry);
        if (!RT_SUCCESS(rc))
        {
            LOG(("CrFbEntryCreateForTexId Failed"));
            return;
        }

        Assert(hEntry);

#if 0
        if (!(cfg & CR_PRESENT_FLAG_CLEAR_RECTS))
        {
            CR_SERVER_DUMP_TEXPRESENT(&pEntry->CEntry.Tex);
        }
#endif
    }
    else
        hEntry = NULL;

    rc = CrFbUpdateBegin(hFb);
    if (RT_SUCCESS(rc))
    {
        if (!(cfg & CR_PRESENT_FLAG_CLEAR_RECTS))
        {
            RTPOINT Point = {xPos, yPos};
            rc = CrFbEntryRegionsAdd(hFb, hEntry, &Point, (uint32_t)cRects, (const RTRECT*)pRects, false);
        }
        else
        {
            CrFbRegionsClear(hFb);
        }

        CrFbUpdateEnd(hFb);
    }
    else
    {
        WARN(("CrFbUpdateBegin Failed"));
    }

    if (hEntry)
        CrFbEntryRelease(hFb, hEntry);
}

DECLINLINE(void) crVBoxPRectUnpack(const VBOXCMDVBVA_RECT *pVbvaRect, RTRECT *pRect)
{
    pRect->xLeft = pVbvaRect->xLeft;
    pRect->yTop = pVbvaRect->yTop;
    pRect->xRight = pVbvaRect->xRight;
    pRect->yBottom = pVbvaRect->yBottom;
}

DECLINLINE(void) crVBoxPRectUnpacks(const VBOXCMDVBVA_RECT *paVbvaRects, RTRECT *paRects, uint32_t cRects)
{
    uint32_t i = 0;
    for (; i < cRects; ++i)
    {
        crVBoxPRectUnpack(&paVbvaRects[i], &paRects[i]);
    }
}

static RTRECT * crVBoxServerCrCmdBltRecsUnpack(const VBOXCMDVBVA_RECT *pPRects, uint32_t cRects)
{
    if (g_CrPresenter.cbTmpBuf < cRects * sizeof (RTRECT))
    {
        if (g_CrPresenter.pvTmpBuf)
            RTMemFree(g_CrPresenter.pvTmpBuf);

        g_CrPresenter.cbTmpBuf = (cRects + 10) * sizeof (RTRECT);
        g_CrPresenter.pvTmpBuf = RTMemAlloc(g_CrPresenter.cbTmpBuf);
        if (!g_CrPresenter.pvTmpBuf)
        {
            WARN(("RTMemAlloc failed!"));
            g_CrPresenter.cbTmpBuf = 0;
            return NULL;
        }
    }

    RTRECT *pRects = (RTRECT *)g_CrPresenter.pvTmpBuf;
    crVBoxPRectUnpacks(pPRects, pRects, cRects);

    return pRects;
}

static void crPMgrPrimaryUpdateScreen(HCR_FRAMEBUFFER hFb, uint32_t idScreen, uint32_t cRects, const RTRECT *pRects)
{
    const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hFb);

    bool fDirtyEmpty = true;
    RTRECT dirtyRect = {0};
    cr_server.CrCmdClientInfo.pfnCltScrUpdateBegin(cr_server.CrCmdClientInfo.hCltScr, idScreen);

    VBVACMDHDR hdr;
    for (uint32_t i = 0; i < cRects; ++i)
    {
        hdr.x = pRects[i].xLeft;
        hdr.y = pRects[i].yTop;
        hdr.w = hdr.x + pRects[i].xRight;
        hdr.h = hdr.y + pRects[i].yBottom;

        cr_server.CrCmdClientInfo.pfnCltScrUpdateProcess(cr_server.CrCmdClientInfo.hCltScr, idScreen, &hdr, sizeof (hdr));

        if (fDirtyEmpty)
        {
            /* This is the first rectangle to be added. */
            dirtyRect.xLeft   = pRects[i].xLeft;
            dirtyRect.yTop    = pRects[i].yTop;
            dirtyRect.xRight  = pRects[i].xRight;
            dirtyRect.yBottom = pRects[i].yBottom;
            fDirtyEmpty       = false;
        }
        else
        {
            /* Adjust region coordinates. */
            if (dirtyRect.xLeft > pRects[i].xLeft)
            {
                dirtyRect.xLeft = pRects[i].xLeft;
            }

            if (dirtyRect.yTop > pRects[i].yTop)
            {
                dirtyRect.yTop = pRects[i].yTop;
            }

            if (dirtyRect.xRight < pRects[i].xRight)
            {
                dirtyRect.xRight = pRects[i].xRight;
            }

            if (dirtyRect.yBottom < pRects[i].yBottom)
            {
                dirtyRect.yBottom = pRects[i].yBottom;
            }
        }
    }

    if (dirtyRect.xRight - dirtyRect.xLeft)
    {
        cr_server.CrCmdClientInfo.pfnCltScrUpdateEnd(cr_server.CrCmdClientInfo.hCltScr, idScreen, pScreen->i32OriginX + dirtyRect.xLeft, pScreen->i32OriginY + dirtyRect.yTop,
                                           dirtyRect.xRight - dirtyRect.xLeft, dirtyRect.yBottom - dirtyRect.yTop);
    }
    else
    {
        cr_server.CrCmdClientInfo.pfnCltScrUpdateEnd(cr_server.CrCmdClientInfo.hCltScr, idScreen, 0, 0, 0, 0);
    }

}

static void crPMgrPrimaryUpdate(HCR_FRAMEBUFFER hFb, uint32_t cRects, const RTRECT *pRects)
{
    if (!cRects)
        return;

    const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hFb);

    uint32_t idFb = pScreen->u32ViewIndex;
    CR_FB_INFO *pFbInfo = &g_CrPresenter.aFbInfos[idFb];

    for (int i = ASMBitFirstSet(pFbInfo->aTargetMap, cr_server.screenCount);
            i >= 0;
            i = ASMBitNextSet(pFbInfo->aTargetMap, cr_server.screenCount, i))
    {
        crPMgrPrimaryUpdateScreen(hFb, i, cRects, pRects);
    }
}

static int8_t crVBoxServerCrCmdBltPrimaryVramGenericProcess(uint32_t u32PrimaryID, VBOXCMDVBVAOFFSET offVRAM, uint32_t width, uint32_t height, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects, bool fToPrimary)
{
    CR_BLITTER_IMG Img;
    int8_t i8Result = crFbImgFromDimOffVramBGRA(offVRAM, width, height, &Img);
    if (i8Result)
    {
        WARN(("invalid param"));
        return -1;
    }

    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabled(u32PrimaryID);
    if (!hFb)
    {
        WARN(("request to present on disabled framebuffer"));
        return -1;
    }

    if (!fToPrimary)
    {
        int rc = CrFbBltGetContents(hFb, pPos, cRects, pRects, &Img);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrFbBltGetContents failed %d", rc));
            return -1;
        }

        return 0;
    }

    int rc = CrFbBltPutContentsNe(hFb, pPos, cRects, pRects, &Img);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbBltPutContentsNe failed %d", rc));
        return -1;
    }

    return 0;
}

static int8_t crVBoxServerCrCmdBltPrimaryProcess(const VBOXCMDVBVA_BLT_PRIMARY *pCmd, uint32_t cbCmd)
{
    uint32_t u32PrimaryID = (uint32_t)pCmd->Hdr.Hdr.u.u8PrimaryID;
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabled(u32PrimaryID);
    if (!hFb)
    {
        WARN(("request to present on disabled framebuffer, ignore"));
        return 0;
    }

    uint32_t cRects;
    const VBOXCMDVBVA_RECT *pPRects = pCmd->aRects;
    if ((cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_PRIMARY, aRects)) % sizeof (VBOXCMDVBVA_RECT))
    {
        WARN(("invalid argument size"));
        return -1;
    }

    cRects = (cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_PRIMARY, aRects)) / sizeof (VBOXCMDVBVA_RECT);

    RTRECT *pRects = crVBoxServerCrCmdBltRecsUnpack(pPRects, cRects);
    if (!pRects)
    {
        WARN(("crVBoxServerCrCmdBltRecsUnpack failed"));
        return -1;
    }

    uint8_t u8Flags = pCmd->Hdr.Hdr.u8Flags;

    if (u8Flags & VBOXCMDVBVA_OPF_OPERAND2_ISID)
    {
        uint32_t texId = pCmd->alloc.u.id;
        if (!texId)
        {
            WARN(("texId is NULL!\n"));
            return -1;
        }

        if (u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2)
        {
            WARN(("blit from primary to texture not implemented"));
            return -1;
        }

        crServerDispatchVBoxTexPresent(texId, u32PrimaryID, pCmd->Hdr.Pos.x, pCmd->Hdr.Pos.y, cRects, (const GLint*)pRects);

        return 0;
    }
    else
    {
        const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hFb);
        uint32_t width = pScreen->u32Width, height = pScreen->u32Height;
        VBOXCMDVBVAOFFSET offVRAM = pCmd->alloc.u.offVRAM;

        bool fToPrymary = !(u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2);
        RTPOINT Pos = {pCmd->Hdr.Pos.x, pCmd->Hdr.Pos.y};
        int8_t i8Result = crVBoxServerCrCmdBltPrimaryVramGenericProcess(u32PrimaryID, offVRAM, width, height, &Pos, cRects, pRects, fToPrymary);
        if (i8Result < 0)
        {
            WARN(("crVBoxServerCrCmdBltPrimaryVramGenericProcess failed"));
            return i8Result;
        }

        if (!fToPrymary)
            return 0;
    }

    crPMgrPrimaryUpdate(hFb, cRects, pRects);

    return 0;
}

static int8_t crVBoxServerCrCmdBltIdToVramMem(uint32_t hostId, VBOXCMDVBVAOFFSET offVRAM, uint32_t width, uint32_t height, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects)
{
    CR_TEXDATA* pTex = CrFbTexDataAcquire(hostId);
    if (!pTex)
    {
        WARN(("pTex failed for %d", hostId));
        return -1;
    }

    const VBOXVR_TEXTURE *pVrTex = CrTdTexGet(pTex);
    if (!width)
    {
        width = pVrTex->width;
        height = pVrTex->height;
    }

    CR_BLITTER_IMG Img;
    int8_t i8Result = crFbImgFromDimOffVramBGRA(offVRAM, width, height, &Img);
    if (i8Result)
    {
        WARN(("invalid param"));
        return -1;
    }

    int rc = CrTdBltEnter(pTex);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrTdBltEnter failed %d", rc));
        return -1;
    }

    rc = crFbTexDataGetContents(pTex, pPos, cRects, pRects, &Img);

    CrTdBltLeave(pTex);

    CrTdRelease(pTex);

    if (!RT_SUCCESS(rc))
    {
        WARN(("crFbTexDataGetContents failed %d", rc));
        return -1;
    }

    return 0;
}

static int8_t crVBoxServerCrCmdBltIdToVram(uint32_t hostId, VBOXCMDVBVAOFFSET offVRAM, uint32_t width, uint32_t height, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects)
{
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabledByVramStart(offVRAM);
    if (hFb)
    {
        const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hFb);
        Assert(!width || pScreen->u32Width == width);
        Assert(!height || pScreen->u32Height == height);

        crServerDispatchVBoxTexPresent(hostId, pScreen->u32ViewIndex, pPos->x, pPos->y, cRects, (const GLint*)pRects);
        return 0;
    }

    return crVBoxServerCrCmdBltIdToVramMem(hostId, offVRAM, width, height, pPos, cRects, pRects);
}

static int8_t crVBoxServerCrCmdBltVramToVramMem(VBOXCMDVBVAOFFSET offSrcVRAM, uint32_t srcWidth, uint32_t srcHeight, VBOXCMDVBVAOFFSET offDstVRAM, uint32_t dstWidth, uint32_t dstHeight, const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects)
{
    CR_BLITTER_IMG srcImg, dstImg;
    int8_t i8Result = crFbImgFromDimOffVramBGRA(offSrcVRAM, srcWidth, srcHeight, &srcImg);
    if (i8Result)
    {
        WARN(("invalid param"));
        return -1;
    }

    i8Result = crFbImgFromDimOffVramBGRA(offDstVRAM, dstWidth, dstHeight, &dstImg);
    if (i8Result)
    {
        WARN(("invalid param"));
        return -1;
    }

    CrMBltImg(&srcImg, pPos, cRects, pRects, &dstImg);

    return 0;
}

static int8_t crVBoxServerCrCmdBltVramToVram(VBOXCMDVBVAOFFSET offSrcVRAM, uint32_t srcWidth, uint32_t srcHeight,
        VBOXCMDVBVAOFFSET offDstVRAM, uint32_t dstWidth, uint32_t dstHeight,
        const RTPOINT *pPos, uint32_t cRects, const RTRECT *pRects)
{
    HCR_FRAMEBUFFER hSrcFb = CrPMgrFbGetEnabledByVramStart(offSrcVRAM);
    HCR_FRAMEBUFFER hDstFb = CrPMgrFbGetEnabledByVramStart(offDstVRAM);

    if (hDstFb)
    {
        if (hSrcFb)
        {
            LOG(("blit from one framebuffer, wow"));

            int rc = CrFbUpdateBegin(hSrcFb);
            if (RT_SUCCESS(rc))
            {
                CrFbRegionsClear(hSrcFb);

                CrFbUpdateEnd(hSrcFb);
            }
            else
                WARN(("CrFbUpdateBegin failed %d", rc));
        }

        CR_BLITTER_IMG Img;
        int8_t i8Result = crFbImgFromDimOffVramBGRA(offSrcVRAM, srcWidth, srcHeight, &Img);
        if (i8Result)
        {
            WARN(("invalid param"));
            return -1;
        }

        const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hDstFb);
        if (pScreen->u32Width == dstWidth && pScreen->u32Height == dstHeight)
        {
            int rc = CrFbBltPutContentsNe(hDstFb, pPos, cRects, pRects, &Img);
            if (RT_FAILURE(rc))
            {
                WARN(("CrFbBltPutContentsNe failed %d", rc));
                return -1;
            }
        }
        else
        {
            int rc = CrFbUpdateBegin(hDstFb);
            if (RT_SUCCESS(rc))
            {
                CrFbRegionsClear(hDstFb);

                CrFbUpdateEnd(hDstFb);
            }
            else
                WARN(("CrFbUpdateBegin failed %d", rc));

            rc = crVBoxServerCrCmdBltVramToVramMem(offSrcVRAM, srcWidth, srcHeight, offDstVRAM, dstWidth, dstHeight, pPos, cRects, pRects);
            if (RT_FAILURE(rc))
            {
                WARN(("crVBoxServerCrCmdBltVramToVramMem failed, %d", rc));
                return -1;
            }
        }

        crPMgrPrimaryUpdate(hDstFb, cRects, pRects);

        return 0;
    }
    else if (hSrcFb)
    {
        CR_BLITTER_IMG Img;
        int8_t i8Result = crFbImgFromDimOffVramBGRA(offDstVRAM, dstWidth, dstHeight, &Img);
        if (i8Result)
        {
            WARN(("invalid param"));
            return -1;
        }

        const VBVAINFOSCREEN *pScreen = CrFbGetScreenInfo(hSrcFb);
        if (pScreen->u32Width == srcWidth && pScreen->u32Height == srcHeight)
        {
            int rc = CrFbBltGetContents(hSrcFb, pPos, cRects, pRects, &Img);
            if (RT_FAILURE(rc))
            {
                WARN(("CrFbBltGetContents failed %d", rc));
                return -1;
            }
        }
        else
        {
            int rc = CrFbUpdateBegin(hSrcFb);
            if (RT_SUCCESS(rc))
            {
                CrFbRegionsClear(hSrcFb);

                CrFbUpdateEnd(hSrcFb);
            }
            else
                WARN(("CrFbUpdateBegin failed %d", rc));

            rc = crVBoxServerCrCmdBltVramToVramMem(offSrcVRAM, srcWidth, srcHeight, offDstVRAM, dstWidth, dstHeight, pPos, cRects, pRects);
            if (RT_FAILURE(rc))
            {
                WARN(("crVBoxServerCrCmdBltVramToVramMem failed, %d", rc));
                return -1;
            }
        }

        return 0;
    }

    return crVBoxServerCrCmdBltVramToVramMem(offSrcVRAM, srcWidth, srcHeight, offDstVRAM, dstWidth, dstHeight, pPos, cRects, pRects);
}


static int8_t crVBoxServerCrCmdBltOffIdProcess(const VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID *pCmd, uint32_t cbCmd)
{
    uint32_t cRects;
    const VBOXCMDVBVA_RECT *pPRects = pCmd->aRects;
    if ((cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID, aRects)) % sizeof (VBOXCMDVBVA_RECT))
    {
        WARN(("invalid argument size"));
        return -1;
    }

    cRects = (cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID, aRects)) / sizeof (VBOXCMDVBVA_RECT);

    RTRECT *pRects = crVBoxServerCrCmdBltRecsUnpack(pPRects, cRects);
    if (!pRects)
    {
        WARN(("crVBoxServerCrCmdBltRecsUnpack failed"));
        return -1;
    }

    uint8_t u8Flags = pCmd->Hdr.Hdr.u8Flags;
    uint32_t hostId = pCmd->id;

    Assert(u8Flags & VBOXCMDVBVA_OPF_OPERAND2_ISID);

    if (!hostId)
    {
        WARN(("zero host id"));
        return -1;
    }

    if (u8Flags & VBOXCMDVBVA_OPF_OPERAND1_ISID)
    {
        WARN(("blit from texture to texture not implemented"));
        return -1;
    }

    if (u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2)
    {
        WARN(("blit to texture not implemented"));
        return -1;
    }

    VBOXCMDVBVAOFFSET offVRAM = pCmd->alloc.u.offVRAM;

    RTPOINT Pos = {pCmd->Hdr.Pos.x, pCmd->Hdr.Pos.y};
    return crVBoxServerCrCmdBltIdToVram(hostId, offVRAM, 0, 0, &Pos, cRects, pRects);
}

static int8_t crVBoxServerCrCmdBltSameDimOrId(const VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8 *pCmd, uint32_t cbCmd)
{
    uint32_t cRects;
    const VBOXCMDVBVA_RECT *pPRects = pCmd->aRects;
    if ((cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8, aRects)) % sizeof (VBOXCMDVBVA_RECT))
    {
        WARN(("invalid argument size"));
        return -1;
    }

    cRects = (cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8, aRects)) / sizeof (VBOXCMDVBVA_RECT);

    RTRECT *pRects = crVBoxServerCrCmdBltRecsUnpack(pPRects, cRects);
    if (!pRects)
    {
        WARN(("crVBoxServerCrCmdBltRecsUnpack failed"));
        return -1;
    }

    uint8_t u8Flags = pCmd->Hdr.Hdr.u8Flags;
    VBOXCMDVBVAOFFSET offVRAM = pCmd->alloc1.Info.u.offVRAM;
    uint32_t width = pCmd->alloc1.u16Width;
    uint32_t height = pCmd->alloc1.u16Height;
    RTPOINT Pos = {pCmd->Hdr.Pos.x, pCmd->Hdr.Pos.y};

    if (u8Flags & VBOXCMDVBVA_OPF_OPERAND2_ISID)
    {
        uint32_t hostId = pCmd->info2.u.id;

        if (!hostId)
        {
            WARN(("zero host id"));
            return -1;
        }

        if (u8Flags & VBOXCMDVBVA_OPF_OPERAND1_ISID)
        {
            WARN(("blit from texture to texture not implemented"));
            return -1;
        }

        if (u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2)
        {
            WARN(("blit to texture not implemented"));
            return -1;
        }

        return crVBoxServerCrCmdBltIdToVram(hostId, offVRAM, width, height, &Pos, cRects, pRects);
    }

    if (u8Flags & VBOXCMDVBVA_OPF_OPERAND1_ISID)
    {
        if (!(u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2))
        {
            WARN(("blit to texture not implemented"));
            return -1;
        }

        return crVBoxServerCrCmdBltIdToVram(pCmd->alloc1.Info.u.id, pCmd->info2.u.offVRAM, width, height, &Pos, cRects, pRects);
    }

    if (u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2)
        crVBoxServerCrCmdBltVramToVram(offVRAM, width, height, pCmd->info2.u.offVRAM, width, height, &Pos, cRects, pRects);
    else
        crVBoxServerCrCmdBltVramToVram(pCmd->info2.u.offVRAM, width, height, offVRAM, width, height, &Pos, cRects, pRects);

    return 0;
}

static int8_t crVBoxServerCrCmdBltGenericBGRAProcess(const VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8 *pCmd, uint32_t cbCmd)
{
    uint32_t cRects;
    const VBOXCMDVBVA_RECT *pPRects = pCmd->aRects;
    if ((cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8, aRects)) % sizeof (VBOXCMDVBVA_RECT))
    {
        WARN(("invalid argument size"));
        return -1;
    }

    cRects = (cbCmd - RT_OFFSETOF(VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8, aRects)) / sizeof (VBOXCMDVBVA_RECT);

    RTRECT *pRects = crVBoxServerCrCmdBltRecsUnpack(pPRects, cRects);
    if (!pRects)
    {
        WARN(("crVBoxServerCrCmdBltRecsUnpack failed"));
        return -1;
    }

    uint8_t u8Flags = pCmd->Hdr.Hdr.u8Flags;
    RTPOINT Pos = {pCmd->Hdr.Pos.x, pCmd->Hdr.Pos.y};

    if (u8Flags & VBOXCMDVBVA_OPF_OPERAND2_ISID)
    {
        if (u8Flags & VBOXCMDVBVA_OPF_OPERAND1_ISID)
        {
            WARN(("blit from texture to texture not implemented"));
            return -1;
        }

        if (u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2)
        {
            WARN(("blit to texture not implemented"));
            return -1;
        }

        return crVBoxServerCrCmdBltIdToVram(pCmd->alloc2.Info.u.id, pCmd->alloc1.Info.u.offVRAM, pCmd->alloc1.u16Width, pCmd->alloc1.u16Height, &Pos, cRects, pRects);
    }
    else
    {
        if (u8Flags & VBOXCMDVBVA_OPF_OPERAND1_ISID)
        {
            if (!(u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2))
            {
                WARN(("blit to texture not implemented"));
                return -1;
            }

            RTPOINT Pos = {pCmd->Hdr.Pos.x, pCmd->Hdr.Pos.y};
            return crVBoxServerCrCmdBltIdToVram(pCmd->alloc1.Info.u.id, pCmd->alloc2.Info.u.offVRAM, pCmd->alloc2.u16Width, pCmd->alloc2.u16Height, &Pos, cRects, pRects);
        }

        if (u8Flags & VBOXCMDVBVA_OPF_BLT_DIR_IN_2)
            crVBoxServerCrCmdBltVramToVram(pCmd->alloc1.Info.u.offVRAM, pCmd->alloc1.u16Width, pCmd->alloc1.u16Height, pCmd->alloc2.Info.u.offVRAM, pCmd->alloc2.u16Width, pCmd->alloc2.u16Height, &Pos, cRects, pRects);
        else
            crVBoxServerCrCmdBltVramToVram(pCmd->alloc2.Info.u.offVRAM, pCmd->alloc2.u16Width, pCmd->alloc2.u16Height, pCmd->alloc1.Info.u.offVRAM, pCmd->alloc1.u16Width, pCmd->alloc1.u16Height, &Pos, cRects, pRects);

        return 0;
    }
}

static int8_t crVBoxServerCrCmdClrFillPrimaryGenericProcess(uint32_t u32PrimaryID, const RTRECT *pRects, uint32_t cRects, uint32_t u32Color)
{
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabled(u32PrimaryID);
    if (!hFb)
    {
        WARN(("request to present on disabled framebuffer, ignore"));
        return 0;
    }

    int rc = CrFbClrFillNe(hFb, cRects, pRects, u32Color);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbClrFillNe failed %d", rc));
        return -1;
    }

    return 0;
}

static int8_t crVBoxServerCrCmdClrFillVramGenericProcess(VBOXCMDVBVAOFFSET offVRAM, uint32_t width, uint32_t height, const RTRECT *pRects, uint32_t cRects, uint32_t u32Color)
{
    CR_BLITTER_IMG Img;
    int8_t i8Result = crFbImgFromDimOffVramBGRA(offVRAM, width, height, &Img);
    if (i8Result)
    {
        WARN(("invalid param"));
        return -1;
    }

    CrMClrFillImg(&Img, cRects, pRects, u32Color);

    return 0;
}

static int8_t crVBoxServerCrCmdClrFillGenericBGRAProcess(const VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8 *pCmd, uint32_t cbCmd)
{
    uint32_t cRects;
    const VBOXCMDVBVA_RECT *pPRects = pCmd->aRects;
    if ((cbCmd - RT_OFFSETOF(VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8, aRects)) % sizeof (VBOXCMDVBVA_RECT))
    {
        WARN(("invalid argument size"));
        return -1;
    }

    cRects = (cbCmd - RT_OFFSETOF(VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8, aRects)) / sizeof (VBOXCMDVBVA_RECT);

    RTRECT *pRects = crVBoxServerCrCmdBltRecsUnpack(pPRects, cRects);
    if (!pRects)
    {
        WARN(("crVBoxServerCrCmdBltRecsUnpack failed"));
        return -1;
    }

//    uint8_t u8Flags = pCmd->Hdr.Hdr.u8Flags;
    int8_t i8Result = crVBoxServerCrCmdClrFillVramGenericProcess(pCmd->dst.Info.u.offVRAM, pCmd->dst.u16Width, pCmd->dst.u16Height, pRects, cRects, pCmd->Hdr.u32Color);
    if (i8Result < 0)
    {
        WARN(("crVBoxServerCrCmdClrFillVramGenericProcess failed"));
        return i8Result;
    }

    return 0;
}

int8_t crVBoxServerCrCmdClrFillProcess(const VBOXCMDVBVA_CLRFILL_HDR *pCmd, uint32_t cbCmd)
{
    uint8_t u8Flags = pCmd->Hdr.u8Flags;
    uint8_t u8Cmd = (VBOXCMDVBVA_OPF_CLRFILL_TYPE_MASK & u8Flags);

    switch (u8Cmd)
    {
        case VBOXCMDVBVA_OPF_CLRFILL_TYPE_GENERIC_A8R8G8B8:
        {
            if (cbCmd < sizeof (VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8))
            {
                WARN(("VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8: invalid command size"));
                return -1;
            }

            return crVBoxServerCrCmdClrFillGenericBGRAProcess((const VBOXCMDVBVA_CLRFILL_GENERIC_A8R8G8B8*)pCmd, cbCmd);
        }
        default:
            WARN(("unsupported command"));
            return -1;
    }

}

int8_t crVBoxServerCrCmdBltProcess(const VBOXCMDVBVA_BLT_HDR *pCmd, uint32_t cbCmd)
{
    uint8_t u8Flags = pCmd->Hdr.u8Flags;
    uint8_t u8Cmd = (VBOXCMDVBVA_OPF_BLT_TYPE_MASK & u8Flags);

    switch (u8Cmd)
    {
        case VBOXCMDVBVA_OPF_BLT_TYPE_SAMEDIM_A8R8G8B8:
        {
            if (cbCmd < sizeof (VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8))
            {
                WARN(("VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8: invalid command size"));
                return -1;
            }

            return crVBoxServerCrCmdBltSameDimOrId((const VBOXCMDVBVA_BLT_SAMEDIM_A8R8G8B8 *)pCmd, cbCmd);
        }
        case VBOXCMDVBVA_OPF_BLT_TYPE_OFFPRIMSZFMT_OR_ID:
        {
            if (cbCmd < sizeof (VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID))
            {
                WARN(("VBOXCMDVBVA_OPF_BLT_TYPE_OFFPRIMSZFMT_OR_ID: invalid command size"));
                return -1;
            }

            return crVBoxServerCrCmdBltOffIdProcess((const VBOXCMDVBVA_BLT_OFFPRIMSZFMT_OR_ID *)pCmd, cbCmd);
        }
        case VBOXCMDVBVA_OPF_BLT_TYPE_GENERIC_A8R8G8B8:
        {
            if (cbCmd < sizeof (VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8))
            {
                WARN(("VBOXCMDVBVA_OPF_BLT_TYPE_GENERIC_A8R8G8B8: invalid command size"));
                return -1;
            }

            return crVBoxServerCrCmdBltGenericBGRAProcess((const VBOXCMDVBVA_BLT_GENERIC_A8R8G8B8 *)pCmd, cbCmd);
        }
        default:
            WARN(("unsupported command"));
            return -1;
    }
}

int8_t crVBoxServerCrCmdFlipProcess(const VBOXCMDVBVA_FLIP *pFlip, uint32_t cbCmd)
{
    uint32_t hostId;
    const VBOXCMDVBVA_RECT *pPRects = pFlip->aRects;
    uint32_t cRects;

    if (pFlip->Hdr.u8Flags & VBOXCMDVBVA_OPF_OPERAND1_ISID)
    {
        hostId = pFlip->src.u.id;
        if (!hostId)
        {
            WARN(("hostId is NULL"));
            return -1;
        }
    }
    else
    {
        WARN(("VBOXCMDVBVA_OPF_ALLOC_SRCID not specified"));
        hostId = 0;
    }

    uint32_t idFb = pFlip->Hdr.u.u8PrimaryID;
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabled(idFb);
    if (!hFb)
    {
        WARN(("request to present on disabled framebuffer, ignore"));
        return 0;
    }

    cRects = (cbCmd - VBOXCMDVBVA_SIZEOF_FLIPSTRUCT_MIN) / sizeof (VBOXCMDVBVA_RECT);
    if (cRects > 0)
    {
        RTRECT *pRects = crVBoxServerCrCmdBltRecsUnpack(pPRects, cRects);
        if (pRects)
        {
            crServerDispatchVBoxTexPresent(hostId, idFb, 0, 0, cRects, (const GLint*)pRects);
            return 0;
        }
    }
    else
    {
        /* Prior to r100476 guest WDDM driver was not supplying us with sub-rectangles
         * data obtained in DxgkDdiPresentNew() callback. Therefore, in order to support backward compatibility,
         * lets play in old way if no rectangles were supplied. */
        const RTRECT *pRect = CrVrScrCompositorRectGet(&hFb->Compositor);
        crServerDispatchVBoxTexPresent(hostId, idFb, 0, 0, 1, (const GLint*)pRect);
    }

    return -1;
}

typedef struct CRSERVER_CLIENT_CALLOUT
{
    VBOXCRCMDCTL_CALLOUT_LISTENTRY Entry;
    PFNVCRSERVER_CLIENT_CALLOUT_CB pfnCb;
    void*pvCb;
} CRSERVER_CLIENT_CALLOUT;

static DECLCALLBACK(void) crServerClientCalloutCb(struct VBOXCRCMDCTL_CALLOUT_LISTENTRY *pEntry)
{
    CRSERVER_CLIENT_CALLOUT *pCallout = RT_FROM_MEMBER(pEntry, CRSERVER_CLIENT_CALLOUT, Entry);
    pCallout->pfnCb(pCallout->pvCb);
    int rc = RTSemEventSignal(cr_server.hCalloutCompletionEvent);
    if (RT_FAILURE(rc))
        WARN(("RTSemEventSignal failed rc %d", rc));
}

static DECLCALLBACK(void) crServerClientCallout(PFNVCRSERVER_CLIENT_CALLOUT_CB pfnCb, void*pvCb)
{
    Assert(cr_server.pCurrentCalloutCtl);
    CRSERVER_CLIENT_CALLOUT Callout;
    Callout.pfnCb = pfnCb;
    Callout.pvCb = pvCb;
    cr_server.ClientInfo.pfnCallout(cr_server.ClientInfo.hClient, cr_server.pCurrentCalloutCtl, &Callout.Entry, crServerClientCalloutCb);

    int rc = RTSemEventWait(cr_server.hCalloutCompletionEvent, RT_INDEFINITE_WAIT);
    if (RT_FAILURE(rc))
        WARN(("RTSemEventWait failed %d", rc));
}


DECLEXPORT(void) crVBoxServerCalloutEnable(VBOXCRCMDCTL *pCtl)
{
#if 1 //def CR_SERVER_WITH_CLIENT_CALLOUTS
    Assert(!cr_server.pCurrentCalloutCtl);
    cr_server.pCurrentCalloutCtl = pCtl;

    cr_server.head_spu->dispatch_table.ChromiumParametervCR(GL_HH_SET_CLIENT_CALLOUT, 0, 0, (void*)crServerClientCallout);
#endif
}

extern DECLEXPORT(void) crVBoxServerCalloutDisable()
{
#if 1 //def CR_SERVER_WITH_CLIENT_CALLOUTS
    Assert(cr_server.pCurrentCalloutCtl);

    cr_server.head_spu->dispatch_table.ChromiumParametervCR(GL_HH_SET_CLIENT_CALLOUT, 0, 0, NULL);

    cr_server.pCurrentCalloutCtl = NULL;
#endif
}

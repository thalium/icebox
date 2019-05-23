/* $Id: display_vrdp.cpp $ */

/** @file
 * Presenter API: CrFbDisplayVrdp class implementation -- display content over VRDP.
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

#include "server_presenter.h"


CrFbDisplayVrdp::CrFbDisplayVrdp()
{
    memset(&mPos, 0, sizeof (mPos));
}


int CrFbDisplayVrdp::EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryCreated(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("EntryAdded failed rc %d", rc));
        return rc;
    }

    Assert(!CrFbDDataEntryGet(hEntry, slotGet()));
    rc = vrdpCreate(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("vrdpCreate failed rc %d", rc));
        return rc;
    }

    return VINF_SUCCESS;
}


int CrFbDisplayVrdp::EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry)
{
    int rc = CrFbDisplayBase::EntryReplaced(pFb, hNewEntry, hReplacedEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pReplacedEntry = CrFbEntryGetCompositorEntry(hReplacedEntry);
    CR_TEXDATA *pReplacedTex = CrVrScrCompositorEntryTexGet(pReplacedEntry);
    const VBOXVR_SCR_COMPOSITOR_ENTRY* pNewEntry = CrFbEntryGetCompositorEntry(hNewEntry);
    CR_TEXDATA *pNewTex = CrVrScrCompositorEntryTexGet(pNewEntry);

    CrTdBltDataInvalidateNe(pReplacedTex);

    rc = CrTdBltEnter(pNewTex);
    if (RT_SUCCESS(rc))
    {
        rc = vrdpFrame(hNewEntry);
        CrTdBltLeave(pNewTex);
    }
    else
        WARN(("CrTdBltEnter failed %d", rc));

    return rc;
}


int CrFbDisplayVrdp::EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryTexChanged(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pEntry = CrFbEntryGetCompositorEntry(hEntry);
    CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(pEntry);

    rc = CrTdBltEnter(pTex);
    if (RT_SUCCESS(rc))
    {
        rc = vrdpFrame(hEntry);
        CrTdBltLeave(pTex);
    }
    else
        WARN(("CrTdBltEnter failed %d", rc));

    return rc;
}


int CrFbDisplayVrdp::EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryRemoved(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pEntry = CrFbEntryGetCompositorEntry(hEntry);
    CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(pEntry);
    CrTdBltDataInvalidateNe(pTex);

    return vrdpRegions(pFb, hEntry);
}


int CrFbDisplayVrdp::EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryDestroyed(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    vrdpDestroy(hEntry);
    return VINF_SUCCESS;
}


int CrFbDisplayVrdp::EntryPosChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryPosChanged(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    vrdpGeometry(hEntry);

    return VINF_SUCCESS;
}


int CrFbDisplayVrdp::RegionsChanged(struct CR_FRAMEBUFFER *pFb)
{
    int rc = CrFbDisplayBase::RegionsChanged(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return vrdpRegionsAll(pFb);
}


int CrFbDisplayVrdp::FramebufferChanged(struct CR_FRAMEBUFFER *pFb)
{
    int rc = CrFbDisplayBase::FramebufferChanged(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    syncPos();

    rc = vrdpSyncEntryAll(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return vrdpRegionsAll(pFb);
}


void CrFbDisplayVrdp::syncPos()
{
    const struct VBVAINFOSCREEN* pScreenInfo = CrFbGetScreenInfo(getFramebuffer());
    mPos.x = pScreenInfo->i32OriginX;
    mPos.y = pScreenInfo->i32OriginY;
}

int CrFbDisplayVrdp::fbCleanup()
{
    int rc = fbCleanupRemoveAllEntries();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return CrFbDisplayBase::fbCleanup();
}


int CrFbDisplayVrdp::fbSync()
{
    syncPos();

    int rc = fbSynchAddAllEntries();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return CrFbDisplayBase::fbSync();
}


void CrFbDisplayVrdp::vrdpDestroy(HCR_FRAMEBUFFER_ENTRY hEntry)
{
    void *pVrdp = CrFbDDataEntryGet(hEntry, slotGet());
    cr_server.outputRedirect.CROREnd(pVrdp);
}


void CrFbDisplayVrdp::vrdpGeometry(HCR_FRAMEBUFFER_ENTRY hEntry)
{
    void *pVrdp = CrFbDDataEntryGet(hEntry, slotGet());
    const VBOXVR_SCR_COMPOSITOR_ENTRY* pEntry = CrFbEntryGetCompositorEntry(hEntry);

    cr_server.outputRedirect.CRORGeometry(
        pVrdp,
        mPos.x + CrVrScrCompositorEntryRectGet(pEntry)->xLeft,
        mPos.y + CrVrScrCompositorEntryRectGet(pEntry)->yTop,
        CrVrScrCompositorEntryTexGet(pEntry)->Tex.width,
        CrVrScrCompositorEntryTexGet(pEntry)->Tex.height);
}


int CrFbDisplayVrdp::vrdpRegions(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    void *pVrdp = CrFbDDataEntryGet(hEntry, slotGet());
    const struct VBOXVR_SCR_COMPOSITOR* pCompositor = CrFbGetCompositor(pFb);
    const VBOXVR_SCR_COMPOSITOR_ENTRY* pEntry = CrFbEntryGetCompositorEntry(hEntry);
    uint32_t cRects;
    const RTRECT *pRects;

    int rc = CrVrScrCompositorEntryRegionsGet(pCompositor, pEntry, &cRects, &pRects, NULL, NULL);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrVrScrCompositorEntryRegionsGet failed, rc %d", rc));
        return rc;
    }

    cr_server.outputRedirect.CRORVisibleRegion(pVrdp, cRects, pRects);
    return VINF_SUCCESS;
}


int CrFbDisplayVrdp::vrdpFrame(HCR_FRAMEBUFFER_ENTRY hEntry)
{
    void *pVrdp = CrFbDDataEntryGet(hEntry, slotGet());
    const VBOXVR_SCR_COMPOSITOR_ENTRY* pEntry = CrFbEntryGetCompositorEntry(hEntry);
    CR_TEXDATA *pTex = CrVrScrCompositorEntryTexGet(pEntry);
    const CR_BLITTER_IMG *pImg;
    CrTdBltDataInvalidateNe(pTex);

    int rc = CrTdBltDataAcquire(pTex, GL_BGRA, !!(CrVrScrCompositorEntryFlagsGet(pEntry) & CRBLT_F_INVERT_SRC_YCOORDS), &pImg);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrTdBltDataAcquire failed rc %d", rc));
        return rc;
    }

    cr_server.outputRedirect.CRORFrame(pVrdp, pImg->pvData, pImg->cbData);
    CrTdBltDataRelease(pTex);
    return VINF_SUCCESS;
}


int CrFbDisplayVrdp::vrdpRegionsAll(struct CR_FRAMEBUFFER *pFb)
{
    const struct VBOXVR_SCR_COMPOSITOR* pCompositor = CrFbGetCompositor(pFb);
    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR Iter;
    CrVrScrCompositorConstIterInit(pCompositor, &Iter);
    const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry;
    while ((pEntry = CrVrScrCompositorConstIterNext(&Iter)) != NULL)
    {
        HCR_FRAMEBUFFER_ENTRY hEntry = CrFbEntryFromCompositorEntry(pEntry);
        vrdpRegions(pFb, hEntry);
    }

    return VINF_SUCCESS;
}


int CrFbDisplayVrdp::vrdpSynchEntry(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    vrdpGeometry(hEntry);

    return vrdpRegions(pFb, hEntry);;
}


int CrFbDisplayVrdp::vrdpSyncEntryAll(struct CR_FRAMEBUFFER *pFb)
{
    const struct VBOXVR_SCR_COMPOSITOR* pCompositor = CrFbGetCompositor(pFb);
    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR Iter;
    CrVrScrCompositorConstIterInit(pCompositor, &Iter);
    const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry;
    while ((pEntry = CrVrScrCompositorConstIterNext(&Iter)) != NULL)
    {
        HCR_FRAMEBUFFER_ENTRY hEntry = CrFbEntryFromCompositorEntry(pEntry);
        int rc = vrdpSynchEntry(pFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("vrdpSynchEntry failed rc %d", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int CrFbDisplayVrdp::vrdpCreate(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    void *pVrdp;

    /* Query supported formats. */
    uint32_t cbFormats = 4096;
    char *pachFormats = (char *)crAlloc(cbFormats);

    if (!pachFormats)
    {
        WARN(("crAlloc failed"));
        return VERR_NO_MEMORY;
    }

    int rc = cr_server.outputRedirect.CRORContextProperty(cr_server.outputRedirect.pvContext,
                                                          0 /* H3DOR_PROP_FORMATS */, /// @todo from a header
                                                          pachFormats, cbFormats, &cbFormats);
    if (RT_SUCCESS(rc))
    {
        if (RTStrStr(pachFormats, "H3DOR_FMT_RGBA_TOPDOWN"))
        {
            cr_server.outputRedirect.CRORBegin(
                cr_server.outputRedirect.pvContext,
                &pVrdp,
                "H3DOR_FMT_RGBA_TOPDOWN"); /// @todo from a header

            if (pVrdp)
            {
                rc = CrFbDDataEntryPut(hEntry, slotGet(), pVrdp);
                if (RT_SUCCESS(rc))
                {
                    vrdpGeometry(hEntry);
                    vrdpRegions(hFb, hEntry);
                    //vrdpFrame(hEntry);
                    return VINF_SUCCESS;
                }
                else
                    WARN(("CrFbDDataEntryPut failed rc %d", rc));

                cr_server.outputRedirect.CROREnd(pVrdp);
            }
            else
            {
                WARN(("CRORBegin failed"));
                rc = VERR_GENERAL_FAILURE;
            }
        }
    }
    else
        WARN(("CRORContextProperty failed rc %d", rc));

    crFree(pachFormats);

    return rc;
}


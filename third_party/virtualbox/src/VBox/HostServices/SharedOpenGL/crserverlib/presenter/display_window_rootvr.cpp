/* $Id: display_window_rootvr.cpp $ */

/** @file
 * Presenter API: CrFbDisplayWindowRootVr class implementation -- display seamless content (visible regions).
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


CrFbDisplayWindowRootVr::CrFbDisplayWindowRootVr(const RTRECT *pViewportRect, uint64_t parentId) :
    CrFbDisplayWindow(pViewportRect, parentId)
{
    CrVrScrCompositorInit(&mCompositor, NULL);
}


int CrFbDisplayWindowRootVr::EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayWindow::EntryCreated(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    Assert(!CrFbDDataEntryGet(hEntry, slotGet()));

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pSrcEntry = CrFbEntryGetCompositorEntry(hEntry);
    VBOXVR_SCR_COMPOSITOR_ENTRY *pMyEntry = entryAlloc();
    CrVrScrCompositorEntryInit(pMyEntry, CrVrScrCompositorEntryRectGet(pSrcEntry), CrVrScrCompositorEntryTexGet(pSrcEntry), NULL);
    CrVrScrCompositorEntryFlagsSet(pMyEntry, CrVrScrCompositorEntryFlagsGet(pSrcEntry));
    rc = CrFbDDataEntryPut(hEntry, slotGet(), pMyEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrFbDDataEntryPut failed rc %d", rc));
        entryFree(pMyEntry);
        return rc;
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::EntryAdded(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayWindow::EntryAdded(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pSrcEntry = CrFbEntryGetCompositorEntry(hEntry);
    VBOXVR_SCR_COMPOSITOR_ENTRY *pMyEntry = (VBOXVR_SCR_COMPOSITOR_ENTRY*)CrFbDDataEntryGet(hEntry, slotGet());
    Assert(pMyEntry);
    CrVrScrCompositorEntryTexSet(pMyEntry, CrVrScrCompositorEntryTexGet(pSrcEntry));

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry)
{
    int rc = CrFbDisplayWindow::EntryReplaced(pFb, hNewEntry, hReplacedEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pSrcNewEntry = CrFbEntryGetCompositorEntry(hNewEntry);
    VBOXVR_SCR_COMPOSITOR_ENTRY *pMyEntry = (VBOXVR_SCR_COMPOSITOR_ENTRY*)CrFbDDataEntryGet(hNewEntry, slotGet());
    CrVrScrCompositorEntryTexSet(pMyEntry, CrVrScrCompositorEntryTexGet(pSrcNewEntry));

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayWindow::EntryTexChanged(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pSrcEntry = CrFbEntryGetCompositorEntry(hEntry);
    VBOXVR_SCR_COMPOSITOR_ENTRY *pMyEntry = (VBOXVR_SCR_COMPOSITOR_ENTRY*)CrFbDDataEntryGet(hEntry, slotGet());
    CrVrScrCompositorEntryTexSet(pMyEntry, CrVrScrCompositorEntryTexGet(pSrcEntry));

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayWindow::EntryRemoved(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    VBOXVR_SCR_COMPOSITOR_ENTRY *pMyEntry = (VBOXVR_SCR_COMPOSITOR_ENTRY*)CrFbDDataEntryGet(hEntry, slotGet());
    rc = CrVrScrCompositorEntryRegionsSet(&mCompositor, pMyEntry, NULL, 0, NULL, false, NULL);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayWindow::EntryDestroyed(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    const VBOXVR_SCR_COMPOSITOR_ENTRY* pSrcEntry = CrFbEntryGetCompositorEntry(hEntry);
    VBOXVR_SCR_COMPOSITOR_ENTRY *pMyEntry = (VBOXVR_SCR_COMPOSITOR_ENTRY*)CrFbDDataEntryGet(hEntry, slotGet());
    CrVrScrCompositorEntryCleanup(pMyEntry);
    entryFree(pMyEntry);

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::setViewportRect(const RTRECT *pViewportRect)
{
    int rc = CrFbDisplayWindow::setViewportRect(pViewportRect);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    rc = setRegionsChanged();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::windowSetCompositor(bool fSet)
{
    if (fSet)
        return getWindow()->SetCompositor(&mCompositor);
    return getWindow()->SetCompositor(NULL);
}


void CrFbDisplayWindowRootVr::ueRegions()
{
    synchCompositorRegions();
}


int CrFbDisplayWindowRootVr::compositorMarkUpdated()
{
    CrVrScrCompositorClear(&mCompositor);

    int rc = CrVrScrCompositorRectSet(&mCompositor, CrVrScrCompositorRectGet(CrFbGetCompositor(getFramebuffer())), NULL);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    rc = setRegionsChanged();
    if (!RT_SUCCESS(rc))
    {
        WARN(("screenChanged failed %d", rc));
        return rc;
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindowRootVr::screenChanged()
{
    int rc = compositorMarkUpdated();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    rc = CrFbDisplayWindow::screenChanged();
    if (!RT_SUCCESS(rc))
    {
        WARN(("screenChanged failed %d", rc));
        return rc;
    }

    return VINF_SUCCESS;
}


const struct RTRECT* CrFbDisplayWindowRootVr::getRect()
{
    return CrVrScrCompositorRectGet(&mCompositor);
}

int CrFbDisplayWindowRootVr::fbCleanup()
{
    int rc = clearCompositor();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return CrFbDisplayWindow::fbCleanup();
}


int CrFbDisplayWindowRootVr::fbSync()
{
    int rc = synchCompositor();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return CrFbDisplayWindow::fbSync();
}


VBOXVR_SCR_COMPOSITOR_ENTRY* CrFbDisplayWindowRootVr::entryAlloc()
{
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    return (VBOXVR_SCR_COMPOSITOR_ENTRY*)RTMemCacheAlloc(g_CrPresenter.CEntryLookasideList);
#else
    return (VBOXVR_SCR_COMPOSITOR_ENTRY*)RTMemAlloc(sizeof (VBOXVR_SCR_COMPOSITOR_ENTRY));
#endif
}


void CrFbDisplayWindowRootVr::entryFree(VBOXVR_SCR_COMPOSITOR_ENTRY* pEntry)
{
    Assert(!CrVrScrCompositorEntryIsUsed(pEntry));
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMemCacheFree(g_CrPresenter.CEntryLookasideList, pEntry);
#else
    RTMemFree(pEntry);
#endif
}


int CrFbDisplayWindowRootVr::synchCompositorRegions()
{
    int rc;

    rootVrTranslateForPos();

    /* ensure the rootvr compositor does not hold any data,
     * i.e. cleanup all rootvr entries data */
    CrVrScrCompositorClear(&mCompositor);

    rc = CrVrScrCompositorIntersectedList(CrFbGetCompositor(getFramebuffer()), &cr_server.RootVr, &mCompositor, rootVrGetCEntry, this, NULL);
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrVrScrCompositorIntersectedList failed, rc %d", rc));
        return rc;
    }

    return getWindow()->SetVisibleRegionsChanged();
}


int CrFbDisplayWindowRootVr::synchCompositor()
{
    int rc = compositorMarkUpdated();
    if (!RT_SUCCESS(rc))
    {
        WARN(("compositorMarkUpdated failed, rc %d", rc));
        return rc;
    }

    rc = fbSynchAddAllEntries();
    if (!RT_SUCCESS(rc))
    {
        WARN(("fbSynchAddAllEntries failed, rc %d", rc));
        return rc;
    }

    return rc;
}


int CrFbDisplayWindowRootVr::clearCompositor()
{
    return fbCleanupRemoveAllEntries();
}


void CrFbDisplayWindowRootVr::rootVrTranslateForPos()
{
    const RTRECT *pRect = getViewportRect();
    const struct VBVAINFOSCREEN* pScreen = CrFbGetScreenInfo(getFramebuffer());
    int32_t x = pScreen->i32OriginX;
    int32_t y = pScreen->i32OriginY;
    int32_t dx = cr_server.RootVrCurPoint.x - x;
    int32_t dy = cr_server.RootVrCurPoint.y - y;

    cr_server.RootVrCurPoint.x = x;
    cr_server.RootVrCurPoint.y = y;

    VBoxVrListTranslate(&cr_server.RootVr, dx, dy);
}


DECLCALLBACK(VBOXVR_SCR_COMPOSITOR_ENTRY*) CrFbDisplayWindowRootVr::rootVrGetCEntry(const VBOXVR_SCR_COMPOSITOR_ENTRY*pEntry, void *pvContext)
{
    CrFbDisplayWindowRootVr *pThis = (CrFbDisplayWindowRootVr*)pvContext;
    HCR_FRAMEBUFFER_ENTRY hEntry = CrFbEntryFromCompositorEntry(pEntry);
    VBOXVR_SCR_COMPOSITOR_ENTRY *pMyEntry = (VBOXVR_SCR_COMPOSITOR_ENTRY*)CrFbDDataEntryGet(hEntry, pThis->slotGet());
    Assert(!CrVrScrCompositorEntryIsUsed(pMyEntry));
    CrVrScrCompositorEntryRectSet(&pThis->mCompositor, pMyEntry, CrVrScrCompositorEntryRectGet(pEntry));
    return pMyEntry;
}


/* $Id: display_window.cpp $ */

/** @file
 * Presenter API: CrFbDisplayWindow class implementation -- display content into host GUI window.
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

CrFbDisplayWindow::CrFbDisplayWindow(const RTRECT *pViewportRect, uint64_t parentId) :
    mpWindow(NULL),
    mViewportRect(*pViewportRect),
    mu32Screen(~0),
    mParentId(parentId)
{
    mFlags.u32Value = 0;
}


CrFbDisplayWindow::~CrFbDisplayWindow()
{
    if (mpWindow)
        delete mpWindow;
}


int CrFbDisplayWindow::UpdateBegin(struct CR_FRAMEBUFFER *pFb)
{
    int rc = mpWindow ? mpWindow->UpdateBegin() : VINF_SUCCESS;
    if (RT_SUCCESS(rc))
    {
        rc = CrFbDisplayBase::UpdateBegin(pFb);
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;
        else
        {
            WARN(("err"));
            if (mpWindow)
                mpWindow->UpdateEnd();
        }
    }
    else
        WARN(("err"));

    return rc;
}


void CrFbDisplayWindow::UpdateEnd(struct CR_FRAMEBUFFER *pFb)
{
    CrFbDisplayBase::UpdateEnd(pFb);

    if (mpWindow)
        mpWindow->UpdateEnd();
}


int CrFbDisplayWindow::RegionsChanged(struct CR_FRAMEBUFFER *pFb)
{
    int rc = CrFbDisplayBase::RegionsChanged(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    if (mpWindow && mpWindow->GetParentId())
    {
        rc = mpWindow->Create();
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindow::EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryCreated(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    if (mpWindow && mpWindow->GetParentId())
    {
        rc = mpWindow->Create();
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindow::EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry)
{
    int rc = CrFbDisplayBase::EntryReplaced(pFb, hNewEntry, hReplacedEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    if (mpWindow && mpWindow->GetParentId())
    {
        rc = mpWindow->Create();
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindow::EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryTexChanged(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    if (mpWindow && mpWindow->GetParentId())
    {
        rc = mpWindow->Create();
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindow::FramebufferChanged(struct CR_FRAMEBUFFER *pFb)
{
    int rc = CrFbDisplayBase::FramebufferChanged(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    return screenChanged();
}


const RTRECT* CrFbDisplayWindow::getViewportRect()
{
    return &mViewportRect;
}


int CrFbDisplayWindow::setViewportRect(const RTRECT *pViewportRect)
{
    if (!isUpdating())
    {
        WARN(("not updating!"));
        return VERR_INVALID_STATE;
    }

    // always call SetPosition to ensure window is adjustep properly
    //        if (pViewportRect->xLeft != mViewportRect.xLeft || pViewportRect->yTop != mViewportRect.yTop)
    if (mpWindow)
    {
        const RTRECT* pRect = getRect();
        int rc = mpWindow->SetPosition(pRect->xLeft - pViewportRect->xLeft, pRect->yTop - pViewportRect->yTop);
        if (!RT_SUCCESS(rc))
        {
            WARN(("SetPosition failed"));
            return rc;
        }
    }

    mViewportRect = *pViewportRect;

    return VINF_SUCCESS;
}


CrFbWindow * CrFbDisplayWindow::windowDetach(bool fCleanup)
{
    if (isUpdating())
    {
        WARN(("updating!"));
        return NULL;
    }

    CrFbWindow * pWindow = mpWindow;
    if (mpWindow)
    {
        if (fCleanup)
            windowCleanup();
        mpWindow = NULL;
    }
    return pWindow;
}


CrFbWindow * CrFbDisplayWindow::windowAttach(CrFbWindow * pNewWindow)
{
    if (isUpdating())
    {
        WARN(("updating!"));
        return NULL;
    }

    CrFbWindow * pOld = mpWindow;
    if (mpWindow)
        windowDetach();

    mpWindow = pNewWindow;
    if (pNewWindow)
        windowSync();

    return mpWindow;
}


int CrFbDisplayWindow::reparent(uint64_t parentId)
{
    if (!isUpdating())
    {
        WARN(("not updating!"));
        return VERR_INVALID_STATE;
    }

    crDebug("CrFbDisplayWindow: change parent from %p to %p.", mParentId, parentId);

    mParentId = parentId;
    int rc = VINF_SUCCESS;

    /* Force notify Render SPU about parent window ID change in order to prevent
     * crashes when it tries to access already deallocated parent window.
     * Previously, we also used isActive() here, however it might become FALSE for the case
     * when VM Window goes fullscreen mode and back. */
    if ( /* isActive() && */ mpWindow)
    {
        rc = mpWindow->Reparent(parentId);
        if (!RT_SUCCESS(rc))
            WARN(("window reparent failed"));

        mFlags.fNeForce = 1;
    }

    return rc;
}


bool CrFbDisplayWindow::isVisible()
{
    HCR_FRAMEBUFFER hFb = getFramebuffer();
    if (!hFb)
        return false;
    const struct VBOXVR_SCR_COMPOSITOR* pCompositor = CrFbGetCompositor(hFb);
    return !CrVrScrCompositorIsEmpty(pCompositor);
}


int CrFbDisplayWindow::winVisibilityChanged()
{
    HCR_FRAMEBUFFER hFb = getFramebuffer();
    if (!hFb || !CrFbIsEnabled(hFb))
    {
        Assert(!mpWindow || !mpWindow->IsVisivle());
        return VINF_SUCCESS;
    }

    int rc = VINF_SUCCESS;

    if (mpWindow)
    {
        rc = mpWindow->UpdateBegin();
        if (RT_SUCCESS(rc))
        {
            rc = mpWindow->SetVisible(!g_CrPresenter.fWindowsForceHidden);
            if (!RT_SUCCESS(rc))
                WARN(("SetVisible failed, rc %d", rc));

            mpWindow->UpdateEnd();
        }
        else
            WARN(("UpdateBegin failed, rc %d", rc));
    }

    return rc;
}


CrFbWindow* CrFbDisplayWindow::getWindow()
{
    return mpWindow;
}


void CrFbDisplayWindow::onUpdateEnd()
{
    CrFbDisplayBase::onUpdateEnd();
    bool fVisible = isVisible();
    if (mFlags.fNeVisible != fVisible || mFlags.fNeForce)
    {
        crVBoxServerNotifyEvent(mu32Screen,
                                fVisible? VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_VISIBLE:
                                          VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_HIDDEN,
                                NULL, 0);
        mFlags.fNeVisible = fVisible;
        mFlags.fNeForce = 0;
    }
}


void CrFbDisplayWindow::ueRegions()
{
    if (mpWindow)
        mpWindow->SetVisibleRegionsChanged();
}


int CrFbDisplayWindow::screenChanged()
{
    if (!isUpdating())
    {
        WARN(("not updating!"));
            return VERR_INVALID_STATE;
    }

    int rc = windowDimensionsSync();
    if (!RT_SUCCESS(rc))
    {
        WARN(("windowDimensionsSync failed rc %d", rc));
        return rc;
    }

    return VINF_SUCCESS;
}


int CrFbDisplayWindow::windowSetCompositor(bool fSet)
{
    if (!mpWindow)
        return VINF_SUCCESS;

    if (fSet)
    {
        const struct VBOXVR_SCR_COMPOSITOR* pCompositor = CrFbGetCompositor(getFramebuffer());
        return mpWindow->SetCompositor(pCompositor);
    }
    return mpWindow->SetCompositor(NULL);
}


int CrFbDisplayWindow::windowCleanup()
{
    if (!mpWindow)
        return VINF_SUCCESS;

    int rc = mpWindow->UpdateBegin();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    rc = windowDimensionsSync(true);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        mpWindow->UpdateEnd();
        return rc;
    }

    rc = windowSetCompositor(false);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        mpWindow->UpdateEnd();
        return rc;
    }

    mpWindow->UpdateEnd();

    return VINF_SUCCESS;
}


int CrFbDisplayWindow::fbCleanup()
{
    int rc = windowCleanup();
    if (!RT_SUCCESS(rc))
    {
        WARN(("windowCleanup failed"));
        return rc;
    }
    return CrFbDisplayBase::fbCleanup();
}


bool CrFbDisplayWindow::isActive()
{
    HCR_FRAMEBUFFER hFb = getFramebuffer();
    return hFb && CrFbIsEnabled(hFb);
}


int CrFbDisplayWindow::windowDimensionsSync(bool fForceCleanup)
{
    int rc = VINF_SUCCESS;

    if (!mpWindow)
        return VINF_SUCCESS;

    //HCR_FRAMEBUFFER hFb = getFramebuffer();
    if (!fForceCleanup && isActive())
    {
        const RTRECT* pRect = getRect();

        if (mpWindow->GetParentId() != mParentId)
        {
            rc = mpWindow->Reparent(mParentId);
            if (!RT_SUCCESS(rc))
            {
                WARN(("err"));
                return rc;
            }
        }

        rc = mpWindow->SetPosition(pRect->xLeft - mViewportRect.xLeft, pRect->yTop - mViewportRect.yTop);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }

        setRegionsChanged();

        rc = mpWindow->SetSize((uint32_t)(pRect->xRight - pRect->xLeft), (uint32_t)(pRect->yBottom - pRect->yTop));
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }

        rc = mpWindow->SetVisible(!g_CrPresenter.fWindowsForceHidden);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }
    else
    {
        rc = mpWindow->SetVisible(false);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
#if 0
        rc = mpWindow->Reparent(mDefaultParentId);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
#endif
    }

    return rc;
}


int CrFbDisplayWindow::windowSync()
{
    if (!mpWindow)
        return VINF_SUCCESS;

    int rc = mpWindow->UpdateBegin();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    rc = windowSetCompositor(true);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        mpWindow->UpdateEnd();
        return rc;
    }

    rc = windowDimensionsSync();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        mpWindow->UpdateEnd();
        return rc;
    }

    mpWindow->UpdateEnd();

    return rc;
}


int CrFbDisplayWindow::fbSync()
{
    int rc = CrFbDisplayBase::fbSync();
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    HCR_FRAMEBUFFER hFb = getFramebuffer();

    mu32Screen = CrFbGetScreenInfo(hFb)->u32ViewIndex;

    rc = windowSync();
    if (!RT_SUCCESS(rc))
    {
        WARN(("windowSync failed %d", rc));
        return rc;
    }

    if (CrFbHas3DData(hFb))
    {
        if (mpWindow && mpWindow->GetParentId())
        {
            rc = mpWindow->Create();
            if (!RT_SUCCESS(rc))
            {
                WARN(("err"));
                return rc;
            }
        }
    }

    return VINF_SUCCESS;
}


const struct RTRECT* CrFbDisplayWindow::getRect()
{
    const struct VBOXVR_SCR_COMPOSITOR* pCompositor = CrFbGetCompositor(getFramebuffer());
    return CrVrScrCompositorRectGet(pCompositor);
}


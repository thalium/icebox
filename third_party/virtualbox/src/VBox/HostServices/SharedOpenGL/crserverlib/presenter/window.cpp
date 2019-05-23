/* $Id: window.cpp $ */

/** @file
 * Presenter API: window class implementation.
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
#include <VBox/VBoxOGL.h>

CrFbWindow::CrFbWindow(uint64_t parentId) :
    mSpuWindow(0),
    mpCompositor(NULL),
    mcUpdates(0),
    mxPos(0),
    myPos(0),
    mWidth(0),
    mHeight(0),
    mParentId(parentId),
    mScaleFactorWStorage(1.0),
    mScaleFactorHStorage(1.0)
{
    int rc;

    mFlags.Value = 0;

    rc = RTSemRWCreate(&scaleFactorLock);
    if (!RT_SUCCESS(rc))
        WARN(("Unable to initialize scaling factor data lock."));
}


bool CrFbWindow::IsCreated() const
{
    return !!mSpuWindow;
}

bool CrFbWindow::IsVisivle() const
{
    return mFlags.fVisible;
}


void CrFbWindow::Destroy()
{
    CRASSERT(!mcUpdates);

    if (!mSpuWindow)
        return;

    cr_server.head_spu->dispatch_table.WindowDestroy(mSpuWindow);

    mSpuWindow = 0;
    mFlags.fDataPresented = 0;
}


int CrFbWindow::Reparent(uint64_t parentId)
{
    if (!checkInitedUpdating())
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }

    crDebug("CrFbWindow: reparent to %p (current mxPos=%d, myPos=%d, mWidth=%u, mHeight=%u)",
        parentId, mxPos, myPos, mWidth, mHeight);

    uint64_t oldParentId = mParentId;

    mParentId = parentId;

    if (mSpuWindow)
    {
        if (oldParentId && !parentId && mFlags.fVisible)
            cr_server.head_spu->dispatch_table.WindowShow(mSpuWindow, false);

        renderspuSetWindowId(mParentId);
        renderspuReparentWindow(mSpuWindow);
        renderspuSetWindowId(cr_server.screen[0].winID);

        if (parentId)
        {
            if (mFlags.fVisible)
                cr_server.head_spu->dispatch_table.WindowPosition(mSpuWindow, mxPos, myPos);
            cr_server.head_spu->dispatch_table.WindowShow(mSpuWindow, mFlags.fVisible);
        }
    }

    return VINF_SUCCESS;
}


int CrFbWindow::SetVisible(bool fVisible)
{
    if (!checkInitedUpdating())
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }

    LOG(("CrWIN: Visible [%d]", fVisible));

    if (!fVisible != !mFlags.fVisible)
    {
        mFlags.fVisible = fVisible;
        if (mSpuWindow && mParentId)
        {
            if (fVisible)
                cr_server.head_spu->dispatch_table.WindowPosition(mSpuWindow, mxPos, myPos);
            cr_server.head_spu->dispatch_table.WindowShow(mSpuWindow, fVisible);
        }
    }

    return VINF_SUCCESS;
}


int CrFbWindow::SetSize(uint32_t width, uint32_t height, bool fForced)
{
    if (!fForced && !checkInitedUpdating())
    {
        crDebug("CrFbWindow: SetSize request dropped because window is currently updating"
                "(width=%d, height=%d, mWidth=%d, mHeight=%d).", width, height, mWidth, mHeight);
        return VERR_INVALID_STATE;
    }

    if (mWidth != width || mHeight != height || fForced)
    {
        GLdouble scaleFactorW, scaleFactorH;
        uint32_t scaledWidth, scaledHeight;

        /* Reset to default values if operation was unsuccessfull. */
        if (!GetScaleFactor(&scaleFactorW, &scaleFactorH))
            scaleFactorW = scaleFactorH = 1.0;

        mFlags.fCompositoEntriesModified = 1;

        /* Keep mWidth and mHeight unchanged (not multiplied by scale factor scalar). */
        mWidth  = width;
        mHeight = height;

        scaledWidth  = (uint32_t)((GLdouble)width  * scaleFactorW);
        scaledHeight = (uint32_t)((GLdouble)height * scaleFactorH);

        if (mSpuWindow)
        {
            cr_server.head_spu->dispatch_table.WindowSize(mSpuWindow, scaledWidth, scaledHeight);
            crDebug("CrFbWindow: SetSize request performed successfully "
                    "(width=%d, height=%d, scaledWidth=%d, scaledHeight=%d).", width, height, scaledWidth, scaledHeight);
        }
        else
            crDebug("CrFbWindow: SetSize request skipped because mSpuWindow not yet constructed "
                    "(width=%d, height=%d, scaledWidth=%d, scaledHeight=%d).", width, height, scaledWidth, scaledHeight);
    }
    else
        crDebug("CrFbWindow: SetSize request skipped because window arleady has requested size "
                "(width=%d, height=%d, mWidth=%d, mHeight=%d).", width, height, mWidth, mHeight);

    return VINF_SUCCESS;
}


int CrFbWindow::SetPosition(int32_t x, int32_t y, bool fForced)
{
    if (!fForced && !checkInitedUpdating())
    {
        crDebug("CrFbWindow: SetPosition request dropped because window is currently updating (x=%d, y=%d).", x, y);
        return VERR_INVALID_STATE;
    }

    LOG(("CrWIN: Pos [%d ; %d]", x, y));
//      always do WindowPosition to ensure window is adjusted properly
//        if (x != mxPos || y != myPos)
    {
        mxPos = x;
        myPos = y;
        if (mSpuWindow)
            cr_server.head_spu->dispatch_table.WindowPosition(mSpuWindow, x, y);
        crDebug("CrFbWindow: SetPosition performed successfully (x=%d, y=%d).", x, y);
    }

    return VINF_SUCCESS;
}


int CrFbWindow::SetVisibleRegionsChanged()
{
    if (!checkInitedUpdating())
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }

    mFlags.fCompositoEntriesModified = 1;
    return VINF_SUCCESS;
}


int CrFbWindow::SetCompositor(const struct VBOXVR_SCR_COMPOSITOR * pCompositor)
{
    if (!checkInitedUpdating())
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }

    mpCompositor = pCompositor;
    mFlags.fCompositoEntriesModified = 1;

    return VINF_SUCCESS;
}


bool CrFbWindow::SetScaleFactor(GLdouble scaleFactorW, GLdouble scaleFactorH)
{
    int rc;

    /* Simple check for input values. */
    if ( !(  (scaleFactorW >= VBOX_OGL_SCALE_FACTOR_MIN && scaleFactorW <= VBOX_OGL_SCALE_FACTOR_MAX)
          && (scaleFactorH >= VBOX_OGL_SCALE_FACTOR_MIN && scaleFactorH <= VBOX_OGL_SCALE_FACTOR_MAX)))
    {
        crDebug("CrFbWindow: attempt to set scale factor out of valid values range: scaleFactorW=%d, scaleFactorH=%d, multiplier=%d.",
            (int)(scaleFactorW * VBOX_OGL_SCALE_FACTOR_MULTIPLIER), (int)(scaleFactorH * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
            (int)VBOX_OGL_SCALE_FACTOR_MULTIPLIER);

        return false;
    }

    rc = RTSemRWRequestWrite(scaleFactorLock, RT_INDEFINITE_WAIT);
    if (RT_SUCCESS(rc))
    {
        mScaleFactorWStorage = scaleFactorW;
        mScaleFactorHStorage = scaleFactorH;
        RTSemRWReleaseWrite(scaleFactorLock);

        crDebug("CrFbWindow: set scale factor: scaleFactorW=%d, scaleFactorH=%d, multiplier=%d.",
            (int)(scaleFactorW * VBOX_OGL_SCALE_FACTOR_MULTIPLIER), (int)(scaleFactorH * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
            (int)VBOX_OGL_SCALE_FACTOR_MULTIPLIER);

        /* Update window geometry. Do not wait for GAs to send SetSize() and SetPosition()
         * events since they might not be running or installed at all. */
        SetSize(mWidth, mHeight, true);
        SetPosition(mxPos, myPos, true);

        return true;
    }

    crDebug("CrFbWindow: unable to set scale factor because RW lock cannot be aquired: scaleFactorW=%d, scaleFactorH=%d, multiplier=%d.",
            (int)(scaleFactorW * VBOX_OGL_SCALE_FACTOR_MULTIPLIER), (int)(scaleFactorH * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
            (int)VBOX_OGL_SCALE_FACTOR_MULTIPLIER);

    return false;
}


bool CrFbWindow::GetScaleFactor(GLdouble *scaleFactorW, GLdouble *scaleFactorH)
{
    int rc;

    rc = RTSemRWRequestRead(scaleFactorLock, RT_INDEFINITE_WAIT);
    if (RT_SUCCESS(rc))
    {
        *scaleFactorW = mScaleFactorWStorage;
        *scaleFactorH = mScaleFactorHStorage;
        RTSemRWReleaseRead(scaleFactorLock);
        return true;
    }

    return false;
}


int CrFbWindow::UpdateBegin()
{
    ++mcUpdates;
    if (mcUpdates > 1)
        return VINF_SUCCESS;

    Assert(!mFlags.fForcePresentOnReenable);

    if (mFlags.fDataPresented)
    {
        Assert(mSpuWindow);
        cr_server.head_spu->dispatch_table.VBoxPresentComposition(mSpuWindow, NULL, NULL);
        mFlags.fForcePresentOnReenable = isPresentNeeded();
    }

    return VINF_SUCCESS;
}


void CrFbWindow::UpdateEnd()
{
    --mcUpdates;
    Assert(mcUpdates < UINT32_MAX/2);
    if (mcUpdates)
        return;

    checkRegions();

    if (mSpuWindow)
    {
        bool fPresentNeeded = isPresentNeeded();
        if (fPresentNeeded || mFlags.fForcePresentOnReenable)
        {
            GLdouble scaleFactorW, scaleFactorH;
            /* Reset to default values if operation was unseccessfull. */
            if (!GetScaleFactor(&scaleFactorW, &scaleFactorH))
                scaleFactorW = scaleFactorH = 1.0;

            mFlags.fForcePresentOnReenable = false;
            if (mpCompositor)
            {
                CrVrScrCompositorSetStretching((VBOXVR_SCR_COMPOSITOR *)mpCompositor, scaleFactorW, scaleFactorH);
                cr_server.head_spu->dispatch_table.VBoxPresentComposition(mSpuWindow, mpCompositor, NULL);
            }
            else
            {
                VBOXVR_SCR_COMPOSITOR TmpCompositor;
                RTRECT Rect;
                Rect.xLeft   = 0;
                Rect.yTop    = 0;
                Rect.xRight  = (uint32_t)((GLdouble)mWidth  * scaleFactorW);
                Rect.yBottom = (uint32_t)((GLdouble)mHeight * scaleFactorH);
                CrVrScrCompositorInit(&TmpCompositor, &Rect);
                CrVrScrCompositorSetStretching((VBOXVR_SCR_COMPOSITOR *)&TmpCompositor, scaleFactorW, scaleFactorH);
                /* this is a cleanup operation
                 * empty compositor is guarantid to be released on VBoxPresentComposition return */
                cr_server.head_spu->dispatch_table.VBoxPresentComposition(mSpuWindow, &TmpCompositor, NULL);
            }
            g_pLed->Asserted.s.fWriting = 1;
        }

        /* even if the above branch is entered due to mFlags.fForcePresentOnReenable,
         * the backend should clean up the compositor as soon as presentation is performed */
        mFlags.fDataPresented = fPresentNeeded;
    }
    else
    {
        Assert(!mFlags.fDataPresented);
        Assert(!mFlags.fForcePresentOnReenable);
    }
}


uint64_t CrFbWindow::GetParentId()
{
    return mParentId;
}


int CrFbWindow::Create()
{
    if (mSpuWindow)
    {
        //WARN(("window already created"));
        return VINF_ALREADY_INITIALIZED;
    }

    CRASSERT(cr_server.fVisualBitsDefault);
    renderspuSetWindowId(mParentId);
    mSpuWindow = cr_server.head_spu->dispatch_table.WindowCreate("", cr_server.fVisualBitsDefault);
    renderspuSetWindowId(cr_server.screen[0].winID);
    if (mSpuWindow < 0) {
        WARN(("WindowCreate failed"));
        return VERR_GENERAL_FAILURE;
    }

    GLdouble scaleFactorW, scaleFactorH;
    /* Reset to default values if operation was unseccessfull. */
    if (!GetScaleFactor(&scaleFactorW, &scaleFactorH))
        scaleFactorW = scaleFactorH = 1.0;

    uint32_t scaledWidth, scaledHeight;

    scaledWidth  = (uint32_t)((GLdouble)mWidth  * scaleFactorW);
    scaledHeight = (uint32_t)((GLdouble)mHeight * scaleFactorH);

    cr_server.head_spu->dispatch_table.WindowSize(mSpuWindow, scaledWidth, scaledHeight);
    cr_server.head_spu->dispatch_table.WindowPosition(mSpuWindow, mxPos, myPos);

    checkRegions();

    if (mParentId && mFlags.fVisible)
        cr_server.head_spu->dispatch_table.WindowShow(mSpuWindow, true);

    return VINF_SUCCESS;
}


CrFbWindow::~CrFbWindow()
{
    int rc;

    Destroy();

    rc = RTSemRWDestroy(scaleFactorLock);
    if (!RT_SUCCESS(rc))
        WARN(("Unable to release scaling factor data lock."));
}


void CrFbWindow::checkRegions()
{
    if (!mSpuWindow)
        return;

    if (!mFlags.fCompositoEntriesModified)
        return;

    uint32_t cRects;
    const RTRECT *pRects;
    if (mpCompositor)
    {
        int rc = CrVrScrCompositorRegionsGet(mpCompositor, &cRects, NULL, &pRects, NULL);
        if (!RT_SUCCESS(rc))
        {
            WARN(("CrVrScrCompositorRegionsGet failed rc %d", rc));
            cRects = 0;
            pRects = NULL;
        }
    }
    else
    {
        cRects = 0;
        pRects = NULL;
    }

    cr_server.head_spu->dispatch_table.WindowVisibleRegion(mSpuWindow, cRects, (const GLint*)pRects);

    mFlags.fCompositoEntriesModified = 0;
}


bool CrFbWindow::isPresentNeeded()
{
    return mFlags.fVisible && mWidth && mHeight && mpCompositor && !CrVrScrCompositorIsEmpty(mpCompositor);
}


bool CrFbWindow::checkInitedUpdating()
{
    if (!mcUpdates)
    {
        WARN(("not updating"));
        return false;
    }

    return true;
}


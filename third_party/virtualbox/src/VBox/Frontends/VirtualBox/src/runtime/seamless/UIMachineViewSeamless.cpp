/* $Id: UIMachineViewSeamless.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineViewSeamless class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QApplication>
# include <QMainWindow>
# include <QTimer>
# ifdef VBOX_WS_MAC
#  include <QMenuBar>
# endif /* VBOX_WS_MAC */

/* GUI includes: */
# include "UISession.h"
# include "UIMachineLogicSeamless.h"
# include "UIMachineWindow.h"
# include "UIMachineViewSeamless.h"
# include "UIFrameBuffer.h"
# include "UIExtraDataManager.h"
# include "UIDesktopWidgetWatchdog.h"

/* COM includes: */
# include "CConsole.h"
# include "CDisplay.h"

/* Other VBox includes: */
# include "VBox/log.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* External includes: */
#ifdef VBOX_WS_X11
# include <limits.h>
#endif /* VBOX_WS_X11 */



UIMachineViewSeamless::UIMachineViewSeamless(  UIMachineWindow *pMachineWindow
                                             , ulong uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                             , bool bAccelerate2DVideo
#endif
                                             )
    : UIMachineView(  pMachineWindow
                    , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                    , bAccelerate2DVideo
#endif
                    )
{
    /* Prepare seamless view: */
    prepareSeamless();
}

void UIMachineViewSeamless::sltAdditionsStateChanged()
{
    adjustGuestScreenSize();
}

void UIMachineViewSeamless::sltHandleSetVisibleRegion(QRegion region)
{
    /* Apply new seamless-region: */
    m_pFrameBuffer->handleSetVisibleRegion(region);
}

bool UIMachineViewSeamless::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    if (pWatched != 0 && pWatched == machineWindow())
    {
        switch (pEvent->type())
        {
            case QEvent::Resize:
            {
                /* Send guest-resize hint only if top window resizing to required dimension: */
                QResizeEvent *pResizeEvent = static_cast<QResizeEvent*>(pEvent);
                if (pResizeEvent->size() != workingArea().size())
                    break;

                /* Recalculate max guest size: */
                setMaxGuestSize();

                break;
            }
            default:
                break;
        }
    }

    return UIMachineView::eventFilter(pWatched, pEvent);
}

void UIMachineViewSeamless::prepareCommon()
{
    /* Base class common settings: */
    UIMachineView::prepareCommon();

    /* Setup size-policy: */
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    /* Maximum size to sizehint: */
    setMaximumSize(sizeHint());
    /* Minimum size is ignored: */
    setMinimumSize(0, 0);
    /* No scrollbars: */
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void UIMachineViewSeamless::prepareFilters()
{
    /* Base class filters: */
    UIMachineView::prepareFilters();
}

void UIMachineViewSeamless::prepareConsoleConnections()
{
    /* Base class connections: */
    UIMachineView::prepareConsoleConnections();

    /* Guest additions state-change updater: */
    connect(uisession(), SIGNAL(sigAdditionsStateActualChange()), this, SLOT(sltAdditionsStateChanged()));
}

void UIMachineViewSeamless::prepareSeamless()
{
    /* Set seamless feature flag to the guest: */
    display().SetSeamlessMode(true);
}

void UIMachineViewSeamless::cleanupSeamless()
{
    /* Reset seamless feature flag if possible: */
    if (uisession()->isRunning())
        display().SetSeamlessMode(false);
}

void UIMachineViewSeamless::adjustGuestScreenSize()
{
    /* Should we adjust guest-screen size? Logging paranoia is required here to reveal the truth. */
    LogRel(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: Adjust guest-screen size if necessary.\n"));
    bool fAdjust = false;

    /* Step 1: Was the guest-screen enabled automatically? */
    if (!fAdjust)
    {
        if (frameBuffer()->isAutoEnabled())
        {
            LogRel2(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: Guest-screen was enabled automatically, adjustment is required.\n"));
            fAdjust = true;
        }
    }
    /* Step 2: Is the guest-screen of another size than necessary? */
    if (!fAdjust)
    {
        /* Acquire frame-buffer size: */
        QSize frameBufferSize(frameBuffer()->width(), frameBuffer()->height());
        /* Take the scale-factor(s) into account: */
        frameBufferSize = scaledForward(frameBufferSize);

        /* Acquire working-area size: */
        const QSize workingAreaSize = workingArea().size();

        if (frameBufferSize != workingAreaSize)
        {
            LogRel2(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: Guest-screen is of another size than necessary, adjustment is required.\n"));
            fAdjust = true;
        }
    }

    /* Step 3: Is guest-additions supports graphics? */
    if (fAdjust)
    {
        if (!uisession()->isGuestSupportsGraphics())
        {
            LogRel2(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: Guest-additions are not supporting graphics, adjustment is omitted.\n"));
            fAdjust = false;
        }
    }
    /* Step 4: Is guest-screen visible? */
    if (fAdjust)
    {
        if (!uisession()->isScreenVisible(screenId()))
        {
            LogRel2(("GUI: UIMachineViewSeamless::adjustGuestScreenSize: Guest-screen is not visible, adjustment is omitted.\n"));
            fAdjust = false;
        }
    }

    /* Final step: Adjust if requested/allowed. */
    if (fAdjust)
    {
        frameBuffer()->setAutoEnabled(false);
        sltPerformGuestResize(workingArea().size());
        /* And remember the size to know what we are resizing out of when we exit: */
        uisession()->setLastFullScreenSize(screenId(), scaledForward(scaledBackward(workingArea().size())));
    }
}

QRect UIMachineViewSeamless::workingArea() const
{
    /* Get corresponding screen: */
    int iScreen = static_cast<UIMachineLogicSeamless*>(machineLogic())->hostScreenForGuestScreen(screenId());
    /* Return available geometry for that screen: */
    return gpDesktop->availableGeometry(iScreen);
}

QSize UIMachineViewSeamless::calculateMaxGuestSize() const
{
    return workingArea().size();
}


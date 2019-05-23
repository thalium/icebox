/* $Id: UIMachineWindowScale.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineWindowScale class implementation.
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
# include <QMenu>
# include <QTimer>
# include <QSpacerItem>
# include <QResizeEvent>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIDesktopWidgetWatchdog.h"
# include "UIExtraDataManager.h"
# include "UISession.h"
# include "UIMachineLogic.h"
# include "UIMachineWindowScale.h"
# include "UIMachineView.h"
# ifdef VBOX_WS_MAC
#  include "VBoxUtils.h"
#  include "UIImageTools.h"
#  include "UICocoaApplication.h"
# endif /* VBOX_WS_MAC */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIMachineWindowScale::UIMachineWindowScale(UIMachineLogic *pMachineLogic, ulong uScreenId)
    : UIMachineWindow(pMachineLogic, uScreenId)
{
}

void UIMachineWindowScale::prepareMainLayout()
{
    /* Call to base-class: */
    UIMachineWindow::prepareMainLayout();

    /* Strict spacers to hide them, they are not necessary for scale-mode: */
    m_pTopSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pBottomSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pLeftSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pRightSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
}

#ifdef VBOX_WS_MAC
void UIMachineWindowScale::prepareVisualState()
{
    /* Call to base-class: */
    UIMachineWindow::prepareVisualState();

    /* Beta label? */
    if (vboxGlobal().isBeta())
    {
        QPixmap betaLabel = ::betaLabel(QSize(100, 16));
        ::darwinLabelWindow(this, &betaLabel, true);
    }

    /* For 'Yosemite' and above: */
    if (vboxGlobal().osRelease() >= MacOSXRelease_Yosemite)
    {
        /* Enable fullscreen support for every screen which requires it: */
        if (darwinScreensHaveSeparateSpaces() || m_uScreenId == 0)
            darwinEnableFullscreenSupport(this);
        /* Register 'Zoom' button to use our full-screen: */
        UICocoaApplication::instance()->registerCallbackForStandardWindowButton(this, StandardWindowButtonType_Zoom,
                                                                                UIMachineWindow::handleStandardWindowButtonCallback);
    }
}
#endif /* VBOX_WS_MAC */

void UIMachineWindowScale::loadSettings()
{
    /* Call to base-class: */
    UIMachineWindow::loadSettings();

    /* Load extra-data settings: */
    {
        /* Load extra-data: */
        QRect geo = gEDataManager->machineWindowGeometry(machineLogic()->visualStateType(),
                                                         m_uScreenId, vboxGlobal().managedVMUuid());

        /* If we do have proper geometry: */
        if (!geo.isNull())
        {
            /* Restore window geometry: */
            m_normalGeometry = geo;
            VBoxGlobal::setTopLevelGeometry(this, m_normalGeometry);

            /* Maximize (if necessary): */
            if (gEDataManager->machineWindowShouldBeMaximized(machineLogic()->visualStateType(),
                                                              m_uScreenId, vboxGlobal().managedVMUuid()))
                setWindowState(windowState() | Qt::WindowMaximized);
        }
        /* If we do NOT have proper geometry: */
        else
        {
            /* Get available geometry, for screen with (x,y) coords if possible: */
            QRect availableGeo = !geo.isNull() ? gpDesktop->availableGeometry(QPoint(geo.x(), geo.y())) :
                                                 gpDesktop->availableGeometry(this);

            /* Resize to default size: */
            resize(640, 480);
            /* Move newly created window to the screen-center: */
            m_normalGeometry = geometry();
            m_normalGeometry.moveCenter(availableGeo.center());
            VBoxGlobal::setTopLevelGeometry(this, m_normalGeometry);
        }

        /* Normalize to the optimal size: */
#ifdef VBOX_WS_X11
        QTimer::singleShot(0, this, SLOT(sltNormalizeGeometry()));
#else /* !VBOX_WS_X11 */
        normalizeGeometry(true /* adjust position */);
#endif /* !VBOX_WS_X11 */
    }
}

void UIMachineWindowScale::saveSettings()
{
    /* Save window geometry: */
    {
        gEDataManager->setMachineWindowGeometry(machineLogic()->visualStateType(),
                                                m_uScreenId, m_normalGeometry,
                                                isMaximizedChecked(), vboxGlobal().managedVMUuid());
    }

    /* Call to base-class: */
    UIMachineWindow::saveSettings();
}

#ifdef VBOX_WS_MAC
void UIMachineWindowScale::cleanupVisualState()
{
    /* Unregister 'Zoom' button from using our full-screen since Yosemite: */
    if (vboxGlobal().osRelease() >= MacOSXRelease_Yosemite)
        UICocoaApplication::instance()->unregisterCallbackForStandardWindowButton(this, StandardWindowButtonType_Zoom);
}
#endif /* VBOX_WS_MAC */

void UIMachineWindowScale::showInNecessaryMode()
{
    /* Make sure this window should be shown at all: */
    if (!uisession()->isScreenVisible(m_uScreenId))
        return hide();

    /* Make sure this window is not minimized: */
    if (isMinimized())
        return;

    /* Show in normal mode: */
    show();

    /* Make sure machine-view have focus: */
    m_pMachineView->setFocus();
}

void UIMachineWindowScale::restoreCachedGeometry()
{
    /* Restore the geometry cached by the window: */
    resize(m_normalGeometry.size());
    move(m_normalGeometry.topLeft());

    /* Adjust machine-view accordingly: */
    adjustMachineViewSize();
}

void UIMachineWindowScale::normalizeGeometry(bool fAdjustPosition)
{
#ifndef VBOX_GUI_WITH_CUSTOMIZATIONS1
    /* Skip if maximized: */
    if (isMaximized())
        return;

    /* Calculate client window offsets: */
    QRect frGeo = frameGeometry();
    const QRect geo = geometry();
    const int dl = geo.left() - frGeo.left();
    const int dt = geo.top() - frGeo.top();
    const int dr = frGeo.right() - geo.right();
    const int db = frGeo.bottom() - geo.bottom();

    /* Adjust position if necessary: */
    if (fAdjustPosition)
        frGeo = VBoxGlobal::normalizeGeometry(frGeo, gpDesktop->overallAvailableRegion());

    /* Finally, set the frame geometry: */
    VBoxGlobal::setTopLevelGeometry(this, frGeo.left() + dl, frGeo.top() + dt,
                                    frGeo.width() - dl - dr, frGeo.height() - dt - db);
#else /* VBOX_GUI_WITH_CUSTOMIZATIONS1 */
    /* Customer request: There should no be
     * machine-window resize/move on machine-view resize: */
    Q_UNUSED(fAdjustPosition);
#endif /* VBOX_GUI_WITH_CUSTOMIZATIONS1 */
}

bool UIMachineWindowScale::event(QEvent *pEvent)
{
    switch (pEvent->type())
    {
        case QEvent::Resize:
        {
#ifdef VBOX_WS_X11
            /* Prevent handling if fake screen detected: */
            if (gpDesktop->isFakeScreenDetected())
                break;
#endif /* VBOX_WS_X11 */

            QResizeEvent *pResizeEvent = static_cast<QResizeEvent*>(pEvent);
            if (!isMaximizedChecked())
            {
                m_normalGeometry.setSize(pResizeEvent->size());
#ifdef VBOX_WITH_DEBUGGER_GUI
                /* Update debugger window position: */
                updateDbgWindows();
#endif /* VBOX_WITH_DEBUGGER_GUI */
            }
            break;
        }
        case QEvent::Move:
        {
#ifdef VBOX_WS_X11
            /* Prevent handling if fake screen detected: */
            if (gpDesktop->isFakeScreenDetected())
                break;
#endif /* VBOX_WS_X11 */

            if (!isMaximizedChecked())
            {
                m_normalGeometry.moveTo(geometry().x(), geometry().y());
#ifdef VBOX_WITH_DEBUGGER_GUI
                /* Update debugger window position: */
                updateDbgWindows();
#endif /* VBOX_WITH_DEBUGGER_GUI */
            }
            break;
        }
        default:
            break;
    }
    return UIMachineWindow::event(pEvent);
}

bool UIMachineWindowScale::isMaximizedChecked()
{
#ifdef VBOX_WS_MAC
    /* On the Mac the WindowStateChange signal doesn't seems to be delivered
     * when the user get out of the maximized state. So check this ourself. */
    return ::darwinIsWindowMaximized(this);
#else /* VBOX_WS_MAC */
    return isMaximized();
#endif /* !VBOX_WS_MAC */
}


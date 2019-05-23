/* $Id: UIMachineWindowNormal.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineWindowNormal class implementation.
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
# include <QMenuBar>
# include <QTimer>
# include <QContextMenuEvent>
# include <QResizeEvent>
# include <QScrollBar>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIDesktopWidgetWatchdog.h"
# include "UIMachineWindowNormal.h"
# include "UIActionPoolRuntime.h"
# include "UIExtraDataManager.h"
# include "UIIndicatorsPool.h"
# include "UIKeyboardHandler.h"
# include "UIMouseHandler.h"
# include "UIMachineLogic.h"
# include "UIMachineView.h"
# include "UIIconPool.h"
# include "UISession.h"
# include "QIStatusBar.h"
# include "QIStatusBarIndicator.h"
# ifndef VBOX_WS_MAC
#  include "UIMenuBar.h"
# else  /* VBOX_WS_MAC */
#  include "VBoxUtils.h"
#  include "UIImageTools.h"
#  include "UICocoaApplication.h"
# endif /* VBOX_WS_MAC */

/* COM includes: */
# include "CConsole.h"
# include "CMediumAttachment.h"
# include "CUSBController.h"
# include "CUSBDeviceFilters.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIMachineWindowNormal::UIMachineWindowNormal(UIMachineLogic *pMachineLogic, ulong uScreenId)
    : UIMachineWindow(pMachineLogic, uScreenId)
    , m_pIndicatorsPool(0)
{
}

void UIMachineWindowNormal::sltMachineStateChanged()
{
    /* Call to base-class: */
    UIMachineWindow::sltMachineStateChanged();

    /* Update indicator-pool and virtualization stuff: */
    updateAppearanceOf(UIVisualElement_IndicatorPoolStuff | UIVisualElement_VideoCapture | UIVisualElement_FeaturesStuff);
}

void UIMachineWindowNormal::sltMediumChange(const CMediumAttachment &attachment)
{
    /* Update corresponding medium stuff: */
    KDeviceType type = attachment.GetType();
    if (type == KDeviceType_HardDisk)
        updateAppearanceOf(UIVisualElement_HDStuff);
    if (type == KDeviceType_DVD)
        updateAppearanceOf(UIVisualElement_CDStuff);
    if (type == KDeviceType_Floppy)
        updateAppearanceOf(UIVisualElement_FDStuff);
}

void UIMachineWindowNormal::sltUSBControllerChange()
{
    /* Update USB stuff: */
    updateAppearanceOf(UIVisualElement_USBStuff);
}

void UIMachineWindowNormal::sltUSBDeviceStateChange()
{
    /* Update USB stuff: */
    updateAppearanceOf(UIVisualElement_USBStuff);
}

void UIMachineWindowNormal::sltAudioAdapterChange()
{
    /* Update audio stuff: */
    updateAppearanceOf(UIVisualElement_AudioStuff);
}

void UIMachineWindowNormal::sltNetworkAdapterChange()
{
    /* Update network stuff: */
    updateAppearanceOf(UIVisualElement_NetworkStuff);
}

void UIMachineWindowNormal::sltSharedFolderChange()
{
    /* Update shared-folders stuff: */
    updateAppearanceOf(UIVisualElement_SharedFolderStuff);
}

void UIMachineWindowNormal::sltVideoCaptureChange()
{
    /* Update video-capture stuff: */
    updateAppearanceOf(UIVisualElement_VideoCapture);
}

void UIMachineWindowNormal::sltCPUExecutionCapChange()
{
    /* Update virtualization stuff: */
    updateAppearanceOf(UIVisualElement_FeaturesStuff);
}

void UIMachineWindowNormal::sltHandleSessionInitialized()
{
    /* Update virtualization stuff: */
    updateAppearanceOf(  UIVisualElement_FeaturesStuff
                       | UIVisualElement_HDStuff
                       | UIVisualElement_CDStuff
                       | UIVisualElement_FDStuff);
}

#ifndef RT_OS_DARWIN
void UIMachineWindowNormal::sltHandleMenuBarConfigurationChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (vboxGlobal().managedVMUuid() != strMachineID)
        return;

    /* Check whether menu-bar is enabled: */
    const bool fEnabled = gEDataManager->menuBarEnabled(vboxGlobal().managedVMUuid());
    /* Update settings action 'enable' state: */
    QAction *pActionMenuBarSettings = actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_S_Settings);
    pActionMenuBarSettings->setEnabled(fEnabled);
    /* Update switch action 'checked' state: */
    QAction *pActionMenuBarSwitch = actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility);
    pActionMenuBarSwitch->blockSignals(true);
    pActionMenuBarSwitch->setChecked(fEnabled);
    pActionMenuBarSwitch->blockSignals(false);

    /* Update menu-bar visibility: */
    menuBar()->setVisible(pActionMenuBarSwitch->isChecked());
    /* Update menu-bar: */
    updateMenu();

    /* Normalize geometry without moving: */
    normalizeGeometry(false /* adjust position */);
}

void UIMachineWindowNormal::sltHandleMenuBarContextMenuRequest(const QPoint &position)
{
    /* Raise action's context-menu: */
    if (gEDataManager->menuBarContextMenuEnabled(vboxGlobal().managedVMUuid()))
        actionPool()->action(UIActionIndexRT_M_View_M_MenuBar)->menu()->exec(menuBar()->mapToGlobal(position));
}
#endif /* !RT_OS_DARWIN */

void UIMachineWindowNormal::sltHandleStatusBarConfigurationChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (vboxGlobal().managedVMUuid() != strMachineID)
        return;

    /* Check whether status-bar is enabled: */
    const bool fEnabled = gEDataManager->statusBarEnabled(vboxGlobal().managedVMUuid());
    /* Update settings action 'enable' state: */
    QAction *pActionStatusBarSettings = actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_S_Settings);
    pActionStatusBarSettings->setEnabled(fEnabled);
    /* Update switch action 'checked' state: */
    QAction *pActionStatusBarSwitch = actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility);
    pActionStatusBarSwitch->blockSignals(true);
    pActionStatusBarSwitch->setChecked(fEnabled);
    pActionStatusBarSwitch->blockSignals(false);

    /* Update status-bar visibility: */
    statusBar()->setVisible(pActionStatusBarSwitch->isChecked());
    /* Update status-bar indicators-pool: */
    m_pIndicatorsPool->setAutoUpdateIndicatorStates(statusBar()->isVisible() && uisession()->isRunning());

    /* Normalize geometry without moving: */
    normalizeGeometry(false /* adjust position */);
}

void UIMachineWindowNormal::sltHandleStatusBarContextMenuRequest(const QPoint &position)
{
    /* Raise action's context-menu: */
    if (gEDataManager->statusBarContextMenuEnabled(vboxGlobal().managedVMUuid()))
        actionPool()->action(UIActionIndexRT_M_View_M_StatusBar)->menu()->exec(statusBar()->mapToGlobal(position));
}

void UIMachineWindowNormal::sltHandleIndicatorContextMenuRequest(IndicatorType indicatorType, const QPoint &position)
{
    /* Determine action depending on indicator-type: */
    UIAction *pAction = 0;
    switch (indicatorType)
    {
        case IndicatorType_HardDisks:     pAction = actionPool()->action(UIActionIndexRT_M_Devices_M_HardDrives);     break;
        case IndicatorType_OpticalDisks:  pAction = actionPool()->action(UIActionIndexRT_M_Devices_M_OpticalDevices); break;
        case IndicatorType_FloppyDisks:   pAction = actionPool()->action(UIActionIndexRT_M_Devices_M_FloppyDevices);  break;
        case IndicatorType_Audio:         pAction = actionPool()->action(UIActionIndexRT_M_Devices_M_Audio);          break;
        case IndicatorType_Network:       pAction = actionPool()->action(UIActionIndexRT_M_Devices_M_Network);        break;
        case IndicatorType_USB:           pAction = actionPool()->action(UIActionIndexRT_M_Devices_M_USBDevices);     break;
        case IndicatorType_SharedFolders: pAction = actionPool()->action(UIActionIndexRT_M_Devices_M_SharedFolders);  break;
        case IndicatorType_Display:       pAction = actionPool()->action(UIActionIndexRT_M_ViewPopup);                break;
        case IndicatorType_VideoCapture:  pAction = actionPool()->action(UIActionIndexRT_M_View_M_VideoCapture);      break;
        case IndicatorType_Mouse:         pAction = actionPool()->action(UIActionIndexRT_M_Input_M_Mouse);            break;
        case IndicatorType_Keyboard:      pAction = actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard);         break;
        default: break;
    }
    /* Raise action's context-menu: */
    if (pAction && pAction->isEnabled())
        pAction->menu()->exec(position);
}

#ifdef VBOX_WS_MAC
void UIMachineWindowNormal::sltActionHovered(UIAction *pAction)
{
    /* Show the action message for a ten seconds: */
    statusBar()->showMessage(pAction->statusTip(), 10000);
}
#endif /* VBOX_WS_MAC */

void UIMachineWindowNormal::prepareSessionConnections()
{
    /* Call to base-class: */
    UIMachineWindow::prepareSessionConnections();

    /* We should watch for console events: */
    connect(machineLogic()->uisession(), SIGNAL(sigMediumChange(const CMediumAttachment &)),
            this, SLOT(sltMediumChange(const CMediumAttachment &)));
    connect(machineLogic()->uisession(), SIGNAL(sigUSBControllerChange()),
            this, SLOT(sltUSBControllerChange()));
    connect(machineLogic()->uisession(), SIGNAL(sigUSBDeviceStateChange(const CUSBDevice &, bool, const CVirtualBoxErrorInfo &)),
            this, SLOT(sltUSBDeviceStateChange()));
    connect(machineLogic()->uisession(), &UISession::sigAudioAdapterChange,
            this, &UIMachineWindowNormal::sltAudioAdapterChange);
    connect(machineLogic()->uisession(), SIGNAL(sigNetworkAdapterChange(const CNetworkAdapter &)),
            this, SLOT(sltNetworkAdapterChange()));
    connect(machineLogic()->uisession(), SIGNAL(sigSharedFolderChange()),
            this, SLOT(sltSharedFolderChange()));
    connect(machineLogic()->uisession(), SIGNAL(sigVideoCaptureChange()),
            this, SLOT(sltVideoCaptureChange()));
    connect(machineLogic()->uisession(), SIGNAL(sigCPUExecutionCapChange()),
            this, SLOT(sltCPUExecutionCapChange()));
    connect(machineLogic()->uisession(), SIGNAL(sigInitialized()),
            this, SLOT(sltHandleSessionInitialized()));
}

#ifndef VBOX_WS_MAC
void UIMachineWindowNormal::prepareMenu()
{
    /* Create menu-bar: */
    setMenuBar(new UIMenuBar);
    AssertPtrReturnVoid(menuBar());
    {
        /* Configure menu-bar: */
        menuBar()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(menuBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
                this, SLOT(sltHandleMenuBarContextMenuRequest(const QPoint&)));
        connect(gEDataManager, SIGNAL(sigMenuBarConfigurationChange(const QString&)),
                this, SLOT(sltHandleMenuBarConfigurationChange(const QString&)));
        /* Update menu-bar: */
        updateMenu();
    }
}
#endif /* !VBOX_WS_MAC */

void UIMachineWindowNormal::prepareStatusBar()
{
    /* Call to base-class: */
    UIMachineWindow::prepareStatusBar();

    /* Create status-bar: */
    setStatusBar(new QIStatusBar);
    AssertPtrReturnVoid(statusBar());
    {
        /* Configure status-bar: */
        statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(statusBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
                this, SLOT(sltHandleStatusBarContextMenuRequest(const QPoint&)));
        /* Create indicator-pool: */
        m_pIndicatorsPool = new UIIndicatorsPool(machineLogic()->uisession());
        AssertPtrReturnVoid(m_pIndicatorsPool);
        {
            /* Configure indicator-pool: */
            connect(m_pIndicatorsPool, SIGNAL(sigContextMenuRequest(IndicatorType, const QPoint&)),
                    this, SLOT(sltHandleIndicatorContextMenuRequest(IndicatorType, const QPoint&)));
            /* Add indicator-pool into status-bar: */
            statusBar()->addPermanentWidget(m_pIndicatorsPool, 0);
        }
        /* Post-configure status-bar: */
        connect(gEDataManager, SIGNAL(sigStatusBarConfigurationChange(const QString&)),
                this, SLOT(sltHandleStatusBarConfigurationChange(const QString&)));
#ifdef VBOX_WS_MAC
        /* Make sure the status-bar is aware of action hovering: */
        connect(actionPool(), SIGNAL(sigActionHovered(UIAction *)),
                this, SLOT(sltActionHovered(UIAction *)));
#endif /* VBOX_WS_MAC */
    }

#ifdef VBOX_WS_MAC
    /* For the status-bar on Cocoa: */
    setUnifiedTitleAndToolBarOnMac(true);
#endif /* VBOX_WS_MAC */
}

void UIMachineWindowNormal::prepareVisualState()
{
    /* Call to base-class: */
    UIMachineWindow::prepareVisualState();

#ifdef VBOX_GUI_WITH_CUSTOMIZATIONS1
    /* Customer request: The background has to go black: */
    QPalette palette(centralWidget()->palette());
    palette.setColor(centralWidget()->backgroundRole(), Qt::black);
    centralWidget()->setPalette(palette);
    centralWidget()->setAutoFillBackground(true);
    setAutoFillBackground(true);
#endif /* VBOX_GUI_WITH_CUSTOMIZATIONS1 */

#ifdef VBOX_WS_MAC
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
#endif /* VBOX_WS_MAC */
}

void UIMachineWindowNormal::loadSettings()
{
    /* Call to base-class: */
    UIMachineWindow::loadSettings();

    /* Load GUI customizations: */
    {
#ifndef VBOX_WS_MAC
        /* Update menu-bar visibility: */
        menuBar()->setVisible(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility)->isChecked());
#endif /* !VBOX_WS_MAC */
        /* Update status-bar visibility: */
        statusBar()->setVisible(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility)->isChecked());
        m_pIndicatorsPool->setAutoUpdateIndicatorStates(statusBar()->isVisible() && uisession()->isRunning());
    }

    /* Load window geometry: */
    {
        /* Load extra-data: */
        QRect geo = gEDataManager->machineWindowGeometry(machineLogic()->visualStateType(),
                                                         m_uScreenId, vboxGlobal().managedVMUuid());

        /* If we do have proper geometry: */
        if (!geo.isNull())
        {
            /* If previous machine-state was SAVED: */
            if (machine().GetState() == KMachineState_Saved)
            {
                /* Restore window geometry: */
                m_normalGeometry = geo;
                VBoxGlobal::setTopLevelGeometry(this, m_normalGeometry);
            }
            /* If previous machine-state was NOT SAVED: */
            else
            {
                /* Restore only window position: */
                m_normalGeometry = QRect(geo.x(), geo.y(), width(), height());
                VBoxGlobal::setTopLevelGeometry(this, m_normalGeometry);
                /* And normalize to the optimal-size: */
                normalizeGeometry(false /* adjust position */);
            }

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

            /* Normalize to the optimal size: */
            normalizeGeometry(true /* adjust position */);
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

void UIMachineWindowNormal::saveSettings()
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

void UIMachineWindowNormal::cleanupVisualState()
{
#ifdef VBOX_WS_MAC
    /* Unregister 'Zoom' button from using our full-screen since Yosemite: */
    if (vboxGlobal().osRelease() >= MacOSXRelease_Yosemite)
        UICocoaApplication::instance()->unregisterCallbackForStandardWindowButton(this, StandardWindowButtonType_Zoom);
#endif /* VBOX_WS_MAC */
}

void UIMachineWindowNormal::cleanupSessionConnections()
{
    /* We should stop watching for console events: */
    disconnect(machineLogic()->uisession(), SIGNAL(sigMediumChange(const CMediumAttachment &)),
               this, SLOT(sltMediumChange(const CMediumAttachment &)));
    disconnect(machineLogic()->uisession(), SIGNAL(sigUSBControllerChange()),
               this, SLOT(sltUSBControllerChange()));
    disconnect(machineLogic()->uisession(), SIGNAL(sigUSBDeviceStateChange(const CUSBDevice &, bool, const CVirtualBoxErrorInfo &)),
               this, SLOT(sltUSBDeviceStateChange()));
    disconnect(machineLogic()->uisession(), SIGNAL(sigNetworkAdapterChange(const CNetworkAdapter &)),
               this, SLOT(sltNetworkAdapterChange()));
    disconnect(machineLogic()->uisession(), &UISession::sigAudioAdapterChange,
               this, &UIMachineWindowNormal::sltAudioAdapterChange);
    disconnect(machineLogic()->uisession(), SIGNAL(sigSharedFolderChange()),
               this, SLOT(sltSharedFolderChange()));
    disconnect(machineLogic()->uisession(), SIGNAL(sigVideoCaptureChange()),
               this, SLOT(sltVideoCaptureChange()));
    disconnect(machineLogic()->uisession(), SIGNAL(sigCPUExecutionCapChange()),
               this, SLOT(sltCPUExecutionCapChange()));

    /* Call to base-class: */
    UIMachineWindow::cleanupSessionConnections();
}

bool UIMachineWindowNormal::event(QEvent *pEvent)
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
            emit sigGeometryChange(geometry());
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
            emit sigGeometryChange(geometry());
            break;
        }
        case QEvent::WindowActivate:
            emit sigGeometryChange(geometry());
            break;
        default:
            break;
    }
    return UIMachineWindow::event(pEvent);
}

void UIMachineWindowNormal::showInNecessaryMode()
{
    /* Make sure this window should be shown at all: */
    if (!uisession()->isScreenVisible(m_uScreenId))
        return hide();

    /* Make sure this window is not minimized: */
    if (isMinimized())
        return;

    /* Show in normal mode: */
    show();

    /* Normalize machine-window geometry: */
    normalizeGeometry(true /* adjust position */);

    /* Make sure machine-view have focus: */
    m_pMachineView->setFocus();
}

void UIMachineWindowNormal::restoreCachedGeometry()
{
    /* Restore the geometry cached by the window: */
    resize(m_normalGeometry.size());
    move(m_normalGeometry.topLeft());

    /* Adjust machine-view accordingly: */
    adjustMachineViewSize();
}

void UIMachineWindowNormal::normalizeGeometry(bool fAdjustPosition)
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

    /* Get the best size w/o scroll-bars: */
    QSize sh = sizeHint();

    /* If guest-screen auto-resize is not enabled
     * or the guest-additions doesn't support graphics
     * we should take scroll-bars size-hints into account: */
    if (!machineView()->isGuestAutoresizeEnabled() || !uisession()->isGuestSupportsGraphics())
    {
        if (machineView()->verticalScrollBar()->isVisible())
            sh -= QSize(machineView()->verticalScrollBar()->sizeHint().width(), 0);
        if (machineView()->horizontalScrollBar()->isVisible())
            sh -= QSize(0, machineView()->horizontalScrollBar()->sizeHint().height());
    }

    /* Resize the frame to fit the contents: */
    sh -= size();
    frGeo.setRight(frGeo.right() + sh.width());
    frGeo.setBottom(frGeo.bottom() + sh.height());

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

void UIMachineWindowNormal::updateAppearanceOf(int iElement)
{
    /* Call to base-class: */
    UIMachineWindow::updateAppearanceOf(iElement);

    /* Set status-bar indicator-pool auto update timer: */
    if (iElement & UIVisualElement_IndicatorPoolStuff)
        m_pIndicatorsPool->setAutoUpdateIndicatorStates(statusBar()->isVisible() && uisession()->isRunning());
    /* Update status-bar indicator-pool appearance only when status-bar is visible: */
    if (statusBar()->isVisible())
    {
        /* If VM is running: */
        if (uisession()->isRunning())
        {
            if (iElement & UIVisualElement_HDStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_HardDisks);
            if (iElement & UIVisualElement_CDStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_OpticalDisks);
            if (iElement & UIVisualElement_FDStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_FloppyDisks);
            if (iElement & UIVisualElement_AudioStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_Audio);
            if (iElement & UIVisualElement_NetworkStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_Network);
            if (iElement & UIVisualElement_USBStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_USB);
            if (iElement & UIVisualElement_SharedFolderStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_SharedFolders);
            if (iElement & UIVisualElement_Display)
                m_pIndicatorsPool->updateAppearance(IndicatorType_Display);
            if (iElement & UIVisualElement_FeaturesStuff)
                m_pIndicatorsPool->updateAppearance(IndicatorType_Features);
        }
        /* If VM is running or paused: */
        if (uisession()->isRunning() || uisession()->isPaused())
        {
            if (iElement & UIVisualElement_VideoCapture)
                m_pIndicatorsPool->updateAppearance(IndicatorType_VideoCapture);
        }
    }
}

#ifndef VBOX_WS_MAC
void UIMachineWindowNormal::updateMenu()
{
    /* Rebuild menu-bar: */
    menuBar()->clear();
    foreach (QMenu *pMenu, actionPool()->menus())
        menuBar()->addMenu(pMenu);
}
#endif /* !VBOX_WS_MAC */

bool UIMachineWindowNormal::isMaximizedChecked()
{
#ifdef VBOX_WS_MAC
    /* On the Mac the WindowStateChange signal doesn't seems to be delivered
     * when the user get out of the maximized state. So check this ourself. */
    return ::darwinIsWindowMaximized(this);
#else /* VBOX_WS_MAC */
    return isMaximized();
#endif /* !VBOX_WS_MAC */
}


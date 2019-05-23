/* $Id: UIMachineLogicSeamless.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineLogicSeamless class implementation.
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
# ifndef VBOX_WS_MAC
#  include <QTimer>
# endif /* !VBOX_WS_MAC */

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIPopupCenter.h"
# include "UISession.h"
# include "UIActionPoolRuntime.h"
# include "UIMachineLogicSeamless.h"
# include "UIMachineWindowSeamless.h"
# include "UIMultiScreenLayout.h"
# include "UIShortcutPool.h"
# ifndef VBOX_WS_MAC
#  include "QIMenu.h"
# else  /* VBOX_WS_MAC */
#  include "VBoxUtils.h"
# endif /* VBOX_WS_MAC */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIMachineLogicSeamless::UIMachineLogicSeamless(QObject *pParent, UISession *pSession)
    : UIMachineLogic(pParent, pSession, UIVisualStateType_Seamless)
#ifndef VBOX_WS_MAC
    , m_pPopupMenu(0)
#endif /* !VBOX_WS_MAC */
{
    /* Create multiscreen layout: */
    m_pScreenLayout = new UIMultiScreenLayout(this);
    actionPool()->toRuntime()->setMultiScreenLayout(m_pScreenLayout);
}

UIMachineLogicSeamless::~UIMachineLogicSeamless()
{
    /* Delete multiscreen layout: */
    actionPool()->toRuntime()->unsetMultiScreenLayout(m_pScreenLayout);
    delete m_pScreenLayout;
}

bool UIMachineLogicSeamless::checkAvailability()
{
    /* Check if there is enough physical memory to enter seamless: */
    if (uisession()->isGuestSupportsSeamless())
    {
        quint64 availBits = machine().GetVRAMSize() /* VRAM */ * _1M /* MiB to bytes */ * 8 /* to bits */;
        quint64 usedBits = m_pScreenLayout->memoryRequirements();
        if (availBits < usedBits)
        {
            msgCenter().cannotEnterSeamlessMode(0, 0, 0,
                                                (((usedBits + 7) / 8 + _1M - 1) / _1M) * _1M);
            return false;
        }
    }

    /* Show the info message. */
    const UIShortcut &shortcut =
            gShortcutPool->shortcut(actionPool()->shortcutsExtraDataID(),
                                    actionPool()->action(UIActionIndexRT_M_View_T_Seamless)->shortcutExtraDataID());
    const QString strHotKey = QString("Host+%1").arg(shortcut.toString());
    if (!msgCenter().confirmGoingSeamless(strHotKey))
        return false;

    return true;
}

void UIMachineLogicSeamless::adjustMachineWindowsGeometry()
{
    LogRel(("GUI: UIMachineLogicSeamless::adjustMachineWindowsGeometry\n"));

    /* Rebuild multi-screen layout: */
    m_pScreenLayout->rebuild();

    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();
}

int UIMachineLogicSeamless::hostScreenForGuestScreen(int iScreenId) const
{
    return m_pScreenLayout->hostScreenForGuestScreen(iScreenId);
}

bool UIMachineLogicSeamless::hasHostScreenForGuestScreen(int iScreenId) const
{
    return m_pScreenLayout->hasHostScreenForGuestScreen(iScreenId);
}

void UIMachineLogicSeamless::notifyAbout3DOverlayVisibilityChange(bool)
{
    /* If active machine-window is defined now: */
    if (activeMachineWindow())
    {
        /* Reinstall corresponding popup-stack and make sure it has proper type: */
        popupCenter().hidePopupStack(activeMachineWindow());
        popupCenter().setPopupStackType(activeMachineWindow(), UIPopupStackType_Separate);
        popupCenter().showPopupStack(activeMachineWindow());
    }
}

void UIMachineLogicSeamless::sltCheckForRequestedVisualStateType()
{
    LogRel(("GUI: UIMachineLogicSeamless::sltCheckForRequestedVisualStateType: Requested-state=%d, Machine-state=%d\n",
            uisession()->requestedVisualState(), uisession()->machineState()));

    /* Do not try to change visual-state type if machine was not started yet: */
    if (!uisession()->isRunning() && !uisession()->isPaused())
        return;

    /* Do not try to change visual-state type in 'manual override' mode: */
    if (isManualOverrideMode())
        return;

    /* If 'seamless' visual-state type is no more supported: */
    if (!uisession()->isGuestSupportsSeamless())
    {
        LogRel(("GUI: UIMachineLogicSeamless::sltCheckForRequestedVisualStateType: "
                "Leaving 'seamless' as it is no more supported...\n"));
        uisession()->setRequestedVisualState(UIVisualStateType_Seamless);
        uisession()->changeVisualState(UIVisualStateType_Normal);
    }
}

void UIMachineLogicSeamless::sltMachineStateChanged()
{
    /* Call to base-class: */
    UIMachineLogic::sltMachineStateChanged();

    /* If machine-state changed from 'paused' to 'running': */
    if (uisession()->isRunning() && uisession()->wasPaused())
    {
        LogRel(("GUI: UIMachineLogicSeamless::sltMachineStateChanged:"
                "Machine-state changed from 'paused' to 'running': "
                "Adjust machine-window geometry...\n"));

        /* Make sure further code will be called just once: */
        uisession()->forgetPreviousMachineState();
        /* Adjust machine-window geometry if necessary: */
        adjustMachineWindowsGeometry();
    }
}

#ifndef VBOX_WS_MAC
void UIMachineLogicSeamless::sltInvokePopupMenu()
{
    /* Popup main-menu if present: */
    if (m_pPopupMenu && !m_pPopupMenu->isEmpty())
    {
        m_pPopupMenu->popup(activeMachineWindow()->geometry().center());
        QTimer::singleShot(0, m_pPopupMenu, SLOT(sltHighlightFirstAction()));
    }
}
#endif /* !VBOX_WS_MAC */

void UIMachineLogicSeamless::sltScreenLayoutChanged()
{
    LogRel(("GUI: UIMachineLogicSeamless::sltScreenLayoutChanged: Multi-screen layout changed.\n"));

    /* Make sure all machine-window(s) have proper geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->showInNecessaryMode();
}

void UIMachineLogicSeamless::sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo)
{
    LogRel(("GUI: UIMachineLogicSeamless: Guest-screen count changed.\n"));

    /* Rebuild multi-screen layout: */
    m_pScreenLayout->rebuild();

    /* Call to base-class: */
    UIMachineLogic::sltGuestMonitorChange(changeType, uScreenId, screenGeo);
}

void UIMachineLogicSeamless::sltHostScreenCountChange()
{
    LogRel(("GUI: UIMachineLogicSeamless: Host-screen count changed.\n"));

    /* Rebuild multi-screen layout: */
    m_pScreenLayout->rebuild();

    /* Call to base-class: */
    UIMachineLogic::sltHostScreenCountChange();
}

void UIMachineLogicSeamless::sltAdditionsStateChanged()
{
    /* Call to base-class: */
    UIMachineLogic::sltAdditionsStateChanged();

    LogRel(("GUI: UIMachineLogicSeamless: Additions-state actual-change event, rebuild multi-screen layout\n"));
    /* Rebuild multi-screen layout: */
    m_pScreenLayout->rebuild();
}

void UIMachineLogicSeamless::prepareActionGroups()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionGroups();

    /* Restrict 'Adjust Window', 'Guest Autoresize', 'Status Bar' and 'Resize' actions for 'View' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuView(UIActionRestrictionLevel_Logic,
                                                         (UIExtraDataMetaDefs::RuntimeMenuViewActionType)
                                                         (UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow |
                                                          UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize |
                                                          UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar |
                                                          UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar |
                                                          UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize));
#ifdef VBOX_WS_MAC
    /* Restrict 'Window' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuBar(UIActionRestrictionLevel_Logic,
                                                        UIExtraDataMetaDefs::MenuType_Window);
#endif /* VBOX_WS_MAC */

    /* Take care of view-action toggle state: */
    UIAction *pActionSeamless = actionPool()->action(UIActionIndexRT_M_View_T_Seamless);
    if (!pActionSeamless->isChecked())
    {
        pActionSeamless->blockSignals(true);
        pActionSeamless->setChecked(true);
        pActionSeamless->blockSignals(false);
    }
}

void UIMachineLogicSeamless::prepareActionConnections()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionConnections();

    /* Prepare 'View' actions connections: */
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToNormal()));
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToFullscreen()));
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToScale()));
}

void UIMachineLogicSeamless::prepareMachineWindows()
{
    /* Do not create machine-window(s) if they created already: */
    if (isMachineWindowsCreated())
        return;

#ifdef VBOX_WS_MAC
    /* We have to make sure that we are getting the front most process.
     * This is necessary for Qt versions > 4.3.3: */
    darwinSetFrontMostProcess();
#endif /* VBOX_WS_MAC */

    /* Update the multi-screen layout: */
    m_pScreenLayout->update();

    /* Create machine-window(s): */
    for (uint cScreenId = 0; cScreenId < machine().GetMonitorCount(); ++cScreenId)
        addMachineWindow(UIMachineWindow::create(this, cScreenId));

    /* Listen for frame-buffer resize: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        connect(pMachineWindow, SIGNAL(sigFrameBufferResize()),
                this, SIGNAL(sigFrameBufferResize()));
    emit sigFrameBufferResize();

    /* Connect multi-screen layout change handler: */
    connect(m_pScreenLayout, SIGNAL(sigScreenLayoutChange()),
            this, SLOT(sltScreenLayoutChanged()));

    /* Mark machine-window(s) created: */
    setMachineWindowsCreated(true);

#ifdef VBOX_WS_X11
    switch (vboxGlobal().typeOfWindowManager())
    {
        case X11WMType_GNOMEShell:
        case X11WMType_Mutter:
        {
            // WORKAROUND:
            // Under certain WMs we can loose machine-window activation due to any Qt::Tool
            // overlay asynchronously shown above it. Qt is not become aware of such event.
            // We are going to ask to return machine-window activation in let's say 100ms.
            QTimer::singleShot(100, machineWindows().first(), SLOT(sltActivateWindow()));
            break;
        }
        default:
            break;
    }
#endif /* VBOX_WS_X11 */
}

#ifndef VBOX_WS_MAC
void UIMachineLogicSeamless::prepareMenu()
{
    /* Prepare popup-menu: */
    m_pPopupMenu = new QIMenu;
    AssertPtrReturnVoid(m_pPopupMenu);
    {
        /* Prepare popup-menu: */
        foreach (QMenu *pMenu, actionPool()->menus())
            m_pPopupMenu->addMenu(pMenu);
    }
}
#endif /* !VBOX_WS_MAC */

#ifndef VBOX_WS_MAC
void UIMachineLogicSeamless::cleanupMenu()
{
    /* Cleanup popup-menu: */
    delete m_pPopupMenu;
    m_pPopupMenu = 0;
}
#endif /* !VBOX_WS_MAC */

void UIMachineLogicSeamless::cleanupMachineWindows()
{
    /* Do not destroy machine-window(s) if they destroyed already: */
    if (!isMachineWindowsCreated())
        return;

    /* Mark machine-window(s) destroyed: */
    setMachineWindowsCreated(false);

    /* Destroy machine-window(s): */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        UIMachineWindow::destroy(pMachineWindow);
}

void UIMachineLogicSeamless::cleanupActionConnections()
{
    /* "View" actions disconnections: */
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToNormal()));
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToFullscreen()));
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToScale()));

    /* Call to base-class: */
    UIMachineLogic::cleanupActionConnections();
}

void UIMachineLogicSeamless::cleanupActionGroups()
{
    /* Take care of view-action toggle state: */
    UIAction *pActionSeamless = actionPool()->action(UIActionIndexRT_M_View_T_Seamless);
    if (pActionSeamless->isChecked())
    {
        pActionSeamless->blockSignals(true);
        pActionSeamless->setChecked(false);
        pActionSeamless->blockSignals(false);
    }

    /* Allow 'Adjust Window', 'Guest Autoresize', 'Status Bar' and 'Resize' actions for 'View' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuView(UIActionRestrictionLevel_Logic,
                                                         UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid);
#ifdef VBOX_WS_MAC
    /* Allow 'Window' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuBar(UIActionRestrictionLevel_Logic,
                                                        UIExtraDataMetaDefs::MenuType_Invalid);
#endif /* VBOX_WS_MAC */

    /* Call to base-class: */
    UIMachineLogic::cleanupActionGroups();
}


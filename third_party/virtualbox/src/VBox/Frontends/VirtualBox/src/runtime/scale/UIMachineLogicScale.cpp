/* $Id: UIMachineLogicScale.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineLogicScale class implementation.
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
# include "UIDesktopWidgetWatchdog.h"
# include "UIMessageCenter.h"
# include "UISession.h"
# include "UIActionPoolRuntime.h"
# include "UIMachineLogicScale.h"
# include "UIMachineWindow.h"
# include "UIShortcutPool.h"
# ifndef VBOX_WS_MAC
#  include "QIMenu.h"
# else  /* VBOX_WS_MAC */
#  include "VBoxUtils.h"
# endif /* VBOX_WS_MAC */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIMachineLogicScale::UIMachineLogicScale(QObject *pParent, UISession *pSession)
    : UIMachineLogic(pParent, pSession, UIVisualStateType_Scale)
#ifndef VBOX_WS_MAC
    , m_pPopupMenu(0)
#endif /* !VBOX_WS_MAC */
{
}

bool UIMachineLogicScale::checkAvailability()
{
    /* Show the info message. */
    const UIShortcut &shortcut =
            gShortcutPool->shortcut(actionPool()->shortcutsExtraDataID(),
                                    actionPool()->action(UIActionIndexRT_M_View_T_Scale)->shortcutExtraDataID());
    const QString strHotKey = QString("Host+%1").arg(shortcut.toString());
    if (!msgCenter().confirmGoingScale(strHotKey))
        return false;

    return true;
}

#ifndef VBOX_WS_MAC
void UIMachineLogicScale::sltInvokePopupMenu()
{
    /* Popup main-menu if present: */
    if (m_pPopupMenu && !m_pPopupMenu->isEmpty())
    {
        m_pPopupMenu->popup(activeMachineWindow()->geometry().center());
        QTimer::singleShot(0, m_pPopupMenu, SLOT(sltHighlightFirstAction()));
    }
}
#endif /* !VBOX_WS_MAC */

void UIMachineLogicScale::sltHostScreenAvailableAreaChange()
{
#ifdef VBOX_WS_X11
    /* Prevent handling if fake screen detected: */
    if (gpDesktop->isFakeScreenDetected())
        return;

    /* Make sure all machine-window(s) have previous but normalized geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        pMachineWindow->restoreCachedGeometry();
#endif /* VBOX_WS_X11 */

    /* Call to base-class: */
    UIMachineLogic::sltHostScreenAvailableAreaChange();
}

void UIMachineLogicScale::prepareActionGroups()
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
                                                          UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize |
                                                          UIExtraDataMetaDefs::RuntimeMenuViewActionType_ScaleFactor));

    /* Take care of view-action toggle state: */
    UIAction *pActionScale = actionPool()->action(UIActionIndexRT_M_View_T_Scale);
    if (!pActionScale->isChecked())
    {
        pActionScale->blockSignals(true);
        pActionScale->setChecked(true);
        pActionScale->blockSignals(false);
    }
}

void UIMachineLogicScale::prepareActionConnections()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionConnections();

    /* Prepare 'View' actions connections: */
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToNormal()));
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToFullscreen()));
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToSeamless()));
}

void UIMachineLogicScale::prepareMachineWindows()
{
    /* Do not create machine-window(s) if they created already: */
    if (isMachineWindowsCreated())
        return;

#ifdef VBOX_WS_MAC /// @todo Is that really need here?
    /* We have to make sure that we are getting the front most process.
     * This is necessary for Qt versions > 4.3.3: */
    ::darwinSetFrontMostProcess();
#endif /* VBOX_WS_MAC */

    /* Get monitors count: */
    ulong uMonitorCount = machine().GetMonitorCount();
    /* Create machine window(s): */
    for (ulong uScreenId = 0; uScreenId < uMonitorCount; ++ uScreenId)
        addMachineWindow(UIMachineWindow::create(this, uScreenId));
    /* Order machine window(s): */
    for (ulong uScreenId = uMonitorCount; uScreenId > 0; -- uScreenId)
        machineWindows()[uScreenId - 1]->raise();

    /* Listen for frame-buffer resize: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        connect(pMachineWindow, SIGNAL(sigFrameBufferResize()),
                this, SIGNAL(sigFrameBufferResize()));
    emit sigFrameBufferResize();

    /* Mark machine-window(s) created: */
    setMachineWindowsCreated(true);
}

#ifndef VBOX_WS_MAC
void UIMachineLogicScale::prepareMenu()
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
void UIMachineLogicScale::cleanupMenu()
{
    /* Cleanup popup-menu: */
    delete m_pPopupMenu;
    m_pPopupMenu = 0;
}
#endif /* !VBOX_WS_MAC */

void UIMachineLogicScale::cleanupMachineWindows()
{
    /* Do not destroy machine-window(s) if they destroyed already: */
    if (!isMachineWindowsCreated())
        return;

    /* Mark machine-window(s) destroyed: */
    setMachineWindowsCreated(false);

    /* Cleanup machine-window(s): */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        UIMachineWindow::destroy(pMachineWindow);
}

void UIMachineLogicScale::cleanupActionConnections()
{
    /* "View" actions disconnections: */
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToNormal()));
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToFullscreen()));
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToSeamless()));

    /* Call to base-class: */
    UIMachineLogic::cleanupActionConnections();

}

void UIMachineLogicScale::cleanupActionGroups()
{
    /* Take care of view-action toggle state: */
    UIAction *pActionScale = actionPool()->action(UIActionIndexRT_M_View_T_Scale);
    if (pActionScale->isChecked())
    {
        pActionScale->blockSignals(true);
        pActionScale->setChecked(false);
        pActionScale->blockSignals(false);
    }

    /* Allow 'Adjust Window', 'Guest Autoresize', 'Status Bar' and 'Resize' actions for 'View' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuView(UIActionRestrictionLevel_Logic,
                                                         UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid);

    /* Call to base-class: */
    UIMachineLogic::cleanupActionGroups();
}


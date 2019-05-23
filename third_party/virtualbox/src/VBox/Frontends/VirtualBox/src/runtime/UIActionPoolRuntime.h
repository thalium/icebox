/* $Id: UIActionPoolRuntime.h $ */
/** @file
 * VBox Qt GUI - UIActionPoolRuntime class declaration.
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

#ifndef ___UIActionPoolRuntime_h___
#define ___UIActionPoolRuntime_h___

/* Qt includes: */
#include <QMap>
#include <QList>

/* GUI includes: */
#include "UIActionPool.h"
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class UISession;
class UIMultiScreenLayout;

/** Runtime action-pool index enum.
  * Naming convention is following:
  * 1. Every menu index prepended with 'M',
  * 2. Every simple-action index prepended with 'S',
  * 3. Every toggle-action index presended with 'T',
  * 4. Every polymorphic-action index presended with 'P',
  * 5. Every sub-index contains full parent-index name. */
enum UIActionIndexRT
{
    /* 'Machine' menu actions: */
    UIActionIndexRT_M_Machine = UIActionIndex_Max + 1,
    UIActionIndexRT_M_Machine_S_Settings,
    UIActionIndexRT_M_Machine_S_TakeSnapshot,
    UIActionIndexRT_M_Machine_S_ShowInformation,
    UIActionIndexRT_M_Machine_T_Pause,
    UIActionIndexRT_M_Machine_S_Reset,
    UIActionIndexRT_M_Machine_S_Detach,
    UIActionIndexRT_M_Machine_S_SaveState,
    UIActionIndexRT_M_Machine_S_Shutdown,
    UIActionIndexRT_M_Machine_S_PowerOff,

    /* 'View' menu actions: */
    UIActionIndexRT_M_View,
    UIActionIndexRT_M_ViewPopup,
    UIActionIndexRT_M_View_T_Fullscreen,
    UIActionIndexRT_M_View_T_Seamless,
    UIActionIndexRT_M_View_T_Scale,
#ifndef RT_OS_DARWIN
    UIActionIndexRT_M_View_S_MinimizeWindow,
#endif /* !RT_OS_DARWIN */
    UIActionIndexRT_M_View_S_AdjustWindow,
    UIActionIndexRT_M_View_T_GuestAutoresize,
    UIActionIndexRT_M_View_S_TakeScreenshot,
    UIActionIndexRT_M_View_M_VideoCapture,
    UIActionIndexRT_M_View_M_VideoCapture_S_Settings,
    UIActionIndexRT_M_View_M_VideoCapture_T_Start,
    UIActionIndexRT_M_View_T_VRDEServer,
    UIActionIndexRT_M_View_M_MenuBar,
    UIActionIndexRT_M_View_M_MenuBar_S_Settings,
#ifndef RT_OS_DARWIN
    UIActionIndexRT_M_View_M_MenuBar_T_Visibility,
#endif /* !RT_OS_DARWIN */
    UIActionIndexRT_M_View_M_StatusBar,
    UIActionIndexRT_M_View_M_StatusBar_S_Settings,
    UIActionIndexRT_M_View_M_StatusBar_T_Visibility,
    UIActionIndexRT_M_View_M_ScaleFactor,

    /* 'Input' menu actions: */
    UIActionIndexRT_M_Input,
    UIActionIndexRT_M_Input_M_Keyboard,
    UIActionIndexRT_M_Input_M_Keyboard_S_Settings,
    UIActionIndexRT_M_Input_M_Keyboard_S_TypeCAD,
#ifdef VBOX_WS_X11
    UIActionIndexRT_M_Input_M_Keyboard_S_TypeCABS,
#endif /* VBOX_WS_X11 */
    UIActionIndexRT_M_Input_M_Keyboard_S_TypeCtrlBreak,
    UIActionIndexRT_M_Input_M_Keyboard_S_TypeInsert,
    UIActionIndexRT_M_Input_M_Keyboard_S_TypePrintScreen,
    UIActionIndexRT_M_Input_M_Keyboard_S_TypeAltPrintScreen,
    UIActionIndexRT_M_Input_M_Mouse,
    UIActionIndexRT_M_Input_M_Mouse_T_Integration,

    /* 'Devices' menu actions: */
    UIActionIndexRT_M_Devices,
    UIActionIndexRT_M_Devices_M_HardDrives,
    UIActionIndexRT_M_Devices_M_HardDrives_S_Settings,
    UIActionIndexRT_M_Devices_M_OpticalDevices,
    UIActionIndexRT_M_Devices_M_FloppyDevices,
    UIActionIndexRT_M_Devices_M_Audio,
    UIActionIndexRT_M_Devices_M_Audio_T_Output,
    UIActionIndexRT_M_Devices_M_Audio_T_Input,
    UIActionIndexRT_M_Devices_M_Network,
    UIActionIndexRT_M_Devices_M_Network_S_Settings,
    UIActionIndexRT_M_Devices_M_USBDevices,
    UIActionIndexRT_M_Devices_M_USBDevices_S_Settings,
    UIActionIndexRT_M_Devices_M_WebCams,
    UIActionIndexRT_M_Devices_M_SharedClipboard,
    UIActionIndexRT_M_Devices_M_DragAndDrop,
    UIActionIndexRT_M_Devices_M_SharedFolders,
    UIActionIndexRT_M_Devices_M_SharedFolders_S_Settings,
    UIActionIndexRT_M_Devices_S_InstallGuestTools,

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* 'Debugger' menu actions: */
    UIActionIndexRT_M_Debug,
    UIActionIndexRT_M_Debug_S_ShowStatistics,
    UIActionIndexRT_M_Debug_S_ShowCommandLine,
    UIActionIndexRT_M_Debug_T_Logging,
    UIActionIndexRT_M_Debug_S_ShowLogDialog,
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
    /* 'Dock' menu actions: */
    UIActionIndexRT_M_Dock,
    UIActionIndexRT_M_Dock_M_DockSettings,
    UIActionIndexRT_M_Dock_M_DockSettings_T_PreviewMonitor,
    UIActionIndexRT_M_Dock_M_DockSettings_T_DisableMonitor,
    UIActionIndexRT_M_Dock_M_DockSettings_T_DisableOverlay,
#endif /* VBOX_WS_MAC */

    /* Maximum index: */
    UIActionIndexRT_Max
};

/** UIActionPool extension
  * representing action-pool singleton for Runtime UI. */
class UIActionPoolRuntime : public UIActionPool
{
    Q_OBJECT;

signals:

    /** Notifies about 'View' : 'Virtual Screen #' menu : 'Toggle' action trigger. */
    void sigNotifyAboutTriggeringViewScreenToggle(int iGuestScreenIndex, bool fEnabled);
    /** Notifies about 'View' : 'Virtual Screen #' menu : 'Resize' action trigger. */
    void sigNotifyAboutTriggeringViewScreenResize(int iGuestScreenIndex, const QSize &size);
    /** Notifies about 'View' : 'Virtual Screen #' menu : 'Remap' action trigger. */
    void sigNotifyAboutTriggeringViewScreenRemap(int iGuestScreenIndex, int iHostScreenIndex);

public:

    /** Defines UI session object reference.
      * @note For menus which uses it to build contents. */
    void setSession(UISession *pSession);
    /** Returns UI session object reference. */
    UISession* uisession() const { return m_pSession; }

    /** Defines UI multi-screen layout object reference.
      * @note For menus which uses it to build contents. */
    void setMultiScreenLayout(UIMultiScreenLayout *pMultiScreenLayout);
    /** Undefines UI multi-screen layout object reference.
      * @note For menus which uses it to build contents. */
    void unsetMultiScreenLayout(UIMultiScreenLayout *pMultiScreenLayout);
    /** Returns UI multi-screen layout object reference. */
    UIMultiScreenLayout* multiScreenLayout() const { return m_pMultiScreenLayout; }

    /** Returns whether the action with passed @a type is allowed in the 'Machine' menu. */
    bool isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType type) const;
    /** Defines 'Machine' menu @a restriction for passed @a level. */
    void setRestrictionForMenuMachine(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuMachineActionType restriction);

    /** Returns whether the action with passed @a type is allowed in the 'View' menu. */
    bool isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType type) const;
    /** Defines 'View' menu @a restriction for passed @a level. */
    void setRestrictionForMenuView(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuViewActionType restriction);

    /** Returns whether the action with passed @a type is allowed in the 'Input' menu. */
    bool isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType type) const;
    /** Defines 'Input' menu @a restriction for passed @a level. */
    void setRestrictionForMenuInput(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuInputActionType restriction);

    /** Returns whether the action with passed @a type is allowed in the 'Devices' menu. */
    bool isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType type) const;
    /** Defines 'Devices' menu @a restriction for passed @a level. */
    void setRestrictionForMenuDevices(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuDevicesActionType restriction);

#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Returns whether the action with passed @a type is allowed in the 'Debug' menu. */
    bool isAllowedInMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType type) const;
    /** Defines 'Debug' menu @a restriction for passed @a level. */
    void setRestrictionForMenuDebugger(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType restriction);
#endif /* VBOX_WITH_DEBUGGER_GUI */

protected slots:

    /** Handles configuration-change. */
    void sltHandleConfigurationChange(const QString &strMachineID);

    /** Handles 'View' : 'Scale Factor' menu : @a pAction trigger. */
    void sltHandleActionTriggerViewScaleFactor(QAction *pAction);

    /** Prepare 'View' : 'Virtual Screen #' menu routine (Normal, Scale). */
    void sltPrepareMenuViewScreen();
    /** Prepare 'View' : 'Virtual Screen #' menu routine (Fullscreen, Seamless). */
    void sltPrepareMenuViewMultiscreen();

    /** Handles 'View' : 'Virtual Screen #' menu : 'Toggle' action trigger. */
    void sltHandleActionTriggerViewScreenToggle();
    /** Handles 'View' : 'Virtual Screen #' menu : 'Resize' @a pAction trigger. */
    void sltHandleActionTriggerViewScreenResize(QAction *pAction);
    /** Handles 'View' : 'Virtual Screen #' menu : 'Remap' @a pAction trigger. */
    void sltHandleActionTriggerViewScreenRemap(QAction *pAction);

    /** Handles screen-layout update. */
    void sltHandleScreenLayoutUpdate();

protected:

    /** Constructor,
      * @param fTemporary is used to determine whether this action-pool is temporary,
      *                   which can be created to re-initialize shortcuts-pool. */
    UIActionPoolRuntime(bool fTemporary = false);

    /** Prepare pool routine. */
    virtual void preparePool();
    /** Prepare connections routine. */
    virtual void prepareConnections();

    /** Update configuration routine. */
    virtual void updateConfiguration();

    /** Update menu routine. */
    void updateMenu(int iIndex);
    /** Update menus routine. */
    void updateMenus();
    /** Update 'Machine' menu routine. */
    void updateMenuMachine();
    /** Update 'View' menu routine. */
    void updateMenuView();
    /** Update 'View' : 'Popup' menu routine. */
    void updateMenuViewPopup();
    /** Update 'View' : 'Video Capture' menu routine. */
    void updateMenuViewVideoCapture();
    /** Update 'View' : 'Menu Bar' menu routine. */
    void updateMenuViewMenuBar();
    /** Update 'View' : 'Status Bar' menu routine. */
    void updateMenuViewStatusBar();
    /** Update 'View' : 'Scale Factor' menu routine. */
    void updateMenuViewScaleFactor();
    /** Update 'View' : 'Virtual Screen #' @a pMenu routine (Normal, Scale). */
    void updateMenuViewScreen(QMenu *pMenu);
    /** Update 'View' : 'Virtual Screen #' @a pMenu routine (Fullscreen, Seamless). */
    void updateMenuViewMultiscreen(QMenu *pMenu);
    /** Update 'Input' menu routine. */
    void updateMenuInput();
    /** Update 'Input' : 'Keyboard' menu routine. */
    void updateMenuInputKeyboard();
    /** Update 'Input' : 'Mouse' menu routine. */
    void updateMenuInputMouse();
    /** Update 'Devices' menu routine. */
    void updateMenuDevices();
    /** Update 'Devices' : 'Hard Drives' menu routine. */
    void updateMenuDevicesHardDrives();
    /** Update 'Devices' : 'Audio' menu routine. */
    void updateMenuDevicesAudio();
    /** Update 'Devices' : 'Network' menu routine. */
    void updateMenuDevicesNetwork();
    /** Update 'Devices' : 'USB' menu routine. */
    void updateMenuDevicesUSBDevices();
    /** Update 'Devices' : 'Shared Folders' menu routine. */
    void updateMenuDevicesSharedFolders();
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Update 'Debug' menu routine. */
    void updateMenuDebug();
#endif /* VBOX_WITH_DEBUGGER_GUI */

    /** Update shortcuts. */
    virtual void updateShortcuts();

    /** Returns extra-data ID to save keyboard shortcuts under. */
    virtual QString shortcutsExtraDataID() const;

    /** Returns the list of Runtime UI main menus. */
    virtual QList<QMenu*> menus() const { return m_mainMenus; }

private:

    /** Holds the UI session object reference. */
    UISession *m_pSession;
    /** Holds the UI multi-screen layout object reference. */
    UIMultiScreenLayout *m_pMultiScreenLayout;

    /** Holds the list of Runtime UI main menus. */
    QList<QMenu*> m_mainMenus;

    /** Holds restricted action types of the Machine menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::RuntimeMenuMachineActionType> m_restrictedActionsMenuMachine;
    /** Holds restricted action types of the View menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::RuntimeMenuViewActionType> m_restrictedActionsMenuView;
    /** Holds restricted action types of the Input menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::RuntimeMenuInputActionType> m_restrictedActionsMenuInput;
    /** Holds restricted action types of the Devices menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::RuntimeMenuDevicesActionType> m_restrictedActionsMenuDevices;
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Holds restricted action types of the Debugger menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType> m_restrictedActionsMenuDebug;
#endif /* VBOX_WITH_DEBUGGER_GUI */

    /* Enable factory in base-class: */
    friend class UIActionPool;
};

#endif /* !___UIActionPoolRuntime_h___ */

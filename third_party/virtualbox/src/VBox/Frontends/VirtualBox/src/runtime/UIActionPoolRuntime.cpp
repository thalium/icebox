/* $Id: UIActionPoolRuntime.cpp $ */
/** @file
 * VBox Qt GUI - UIActionPoolRuntime class implementation.
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

/* GUI includes: */
# include "UIActionPoolRuntime.h"
# include "UIMultiScreenLayout.h"
# include "UIExtraDataManager.h"
# include "UIShortcutPool.h"
# include "UIFrameBuffer.h"
# include "UIConverter.h"
# include "UIIconPool.h"
# include "UISession.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CExtPack.h"
# include "CExtPackManager.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* Namespaces: */
using namespace UIExtraDataDefs;


class UIActionMenuMachineRuntime : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuMachineRuntime(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::MenuType_Machine; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_Machine); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_Machine); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Machine"));
    }
};

class UIActionSimpleShowSettingsDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowSettingsDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_settings_16px.png", ":/vm_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog); }

    QString shortcutExtraDataID() const
    {
        return QString("SettingsDialog");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("S");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display the virtual machine settings window"));
    }
};

class UIActionSimplePerformTakeSnapshot : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTakeSnapshot(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/snapshot_take_16px.png", ":/snapshot_take_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot); }

    QString shortcutExtraDataID() const
    {
        return QString("TakeSnapshot");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("T");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Take Sn&apshot..."));
        setStatusTip(QApplication::translate("UIActionPool", "Take a snapshot of the virtual machine"));
    }
};

class UIActionSimpleShowInformationDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowInformationDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/session_info_16px.png", ":/session_info_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_InformationDialog; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_InformationDialog); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_InformationDialog); }

    QString shortcutExtraDataID() const
    {
        return QString("InformationDialog");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("N");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Session I&nformation..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display the virtual machine session information window"));
    }
};

class UIActionTogglePause : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionTogglePause(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/vm_pause_on_16px.png", ":/vm_pause_16px.png",
                         ":/vm_pause_on_disabled_16px.png", ":/vm_pause_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Pause; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Pause); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Pause); }

    QString shortcutExtraDataID() const
    {
        return QString("Pause");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("P");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Pause"));
        setStatusTip(QApplication::translate("UIActionPool", "Suspend the execution of the virtual machine"));
    }
};

class UIActionSimplePerformReset : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformReset(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_reset_16px.png", ":/vm_reset_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Reset; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Reset); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Reset); }

    QString shortcutExtraDataID() const
    {
        return QString("Reset");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("R");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Reset"));
        setStatusTip(QApplication::translate("UIActionPool", "Reset the virtual machine"));
    }
};

class UIActionSimplePerformDetach : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformDetach(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_create_shortcut_16px.png", ":/vm_create_shortcut_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach); }

    QString shortcutExtraDataID() const
    {
        return QString("DetachUI");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Detach GUI"));
        setStatusTip(QApplication::translate("UIActionPool", "Detach the GUI from headless VM"));
    }
};

class UIActionSimplePerformSaveState : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformSaveState(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_save_state_16px.png", ":/vm_save_state_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SaveState; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SaveState); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SaveState); }

    QString shortcutExtraDataID() const
    {
        return QString("SaveState");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Save State"));
        setStatusTip(QApplication::translate("UIActionPool", "Save the state of the virtual machine"));
    }
};

class UIActionSimplePerformShutdown : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformShutdown(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_shutdown_16px.png", ":/vm_shutdown_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Shutdown; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Shutdown); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Shutdown); }

    QString shortcutExtraDataID() const
    {
        return QString("Shutdown");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
#ifdef VBOX_WS_MAC
        return QKeySequence("U");
#else /* VBOX_WS_MAC */
        return QKeySequence("H");
#endif /* !VBOX_WS_MAC */
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "ACPI Sh&utdown"));
        setStatusTip(QApplication::translate("UIActionPool", "Send the ACPI Shutdown signal to the virtual machine"));
    }
};

class UIActionSimplePerformPowerOff : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformPowerOff(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_poweroff_16px.png", ":/vm_poweroff_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_PowerOff; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_PowerOff); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_PowerOff); }

    QString shortcutExtraDataID() const
    {
        return QString("PowerOff");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Po&wer Off"));
        setStatusTip(QApplication::translate("UIActionPool", "Power off the virtual machine"));
    }
};

class UIActionMenuView : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuView(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::MenuType_View; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_View); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_View); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&View"));
    }
};

class UIActionMenuViewPopup : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuViewPopup(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::MenuType_View; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_View); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_View); }

    void retranslateUi() {}
};

class UIActionToggleFullscreenMode : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleFullscreenMode(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/fullscreen_on_16px.png", ":/fullscreen_16px.png",
                         ":/fullscreen_on_disabled_16px.png", ":/fullscreen_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen); }

    QString shortcutExtraDataID() const
    {
        return QString("FullscreenMode");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("F");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Full-screen Mode"));
        setStatusTip(QApplication::translate("UIActionPool", "Switch between normal and full-screen mode"));
    }
};

class UIActionToggleSeamlessMode : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleSeamlessMode(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/seamless_on_16px.png", ":/seamless_16px.png",
                         ":/seamless_on_disabled_16px.png", ":/seamless_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless); }

    QString shortcutExtraDataID() const
    {
        return QString("SeamlessMode");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("L");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Seam&less Mode"));
        setStatusTip(QApplication::translate("UIActionPool", "Switch between normal and seamless desktop integration mode"));
    }
};

class UIActionToggleScaleMode : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleScaleMode(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/scale_on_16px.png", ":/scale_16px.png",
                         ":/scale_on_disabled_16px.png", ":/scale_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale); }

    QString shortcutExtraDataID() const
    {
        return QString("ScaleMode");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("C");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "S&caled Mode"));
        setStatusTip(QApplication::translate("UIActionPool", "Switch between normal and scaled mode"));
    }
};

#ifndef RT_OS_DARWIN
class UIActionSimplePerformMinimizeWindow : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformMinimizeWindow(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/minimize_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_MinimizeWindow; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_MinimizeWindow); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_MinimizeWindow); }

    QString shortcutExtraDataID() const
    {
        return QString("WindowMinimize");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("M");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Minimize Window"));
        setStatusTip(QApplication::translate("UIActionPool", "Minimize active window"));
    }
};
#endif /* !RT_OS_DARWIN */

class UIActionSimplePerformWindowAdjust : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformWindowAdjust(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/adjust_win_size_16px.png", ":/adjust_win_size_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow); }

    QString shortcutExtraDataID() const
    {
        return QString("WindowAdjust");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("A");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Adjust Window Size"));
        setStatusTip(QApplication::translate("UIActionPool", "Adjust window size and position to best fit the guest display"));
    }
};

class UIActionToggleGuestAutoresize : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleGuestAutoresize(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/auto_resize_on_on_16px.png", ":/auto_resize_on_16px.png",
                         ":/auto_resize_on_on_disabled_16px.png", ":/auto_resize_on_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize); }

    QString shortcutExtraDataID() const
    {
        return QString("GuestAutoresize");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Auto-resize &Guest Display"));
        setStatusTip(QApplication::translate("UIActionPool", "Automatically resize the guest display when the window is resized"));
    }
};

class UIActionSimplePerformTakeScreenshot : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTakeScreenshot(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/screenshot_take_16px.png", ":/screenshot_take_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_TakeScreenshot; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_TakeScreenshot); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_TakeScreenshot); }

    QString shortcutExtraDataID() const
    {
        return QString("TakeScreenshot");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("E");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Take Screensh&ot..."));
        setStatusTip(QApplication::translate("UIActionPool", "Take guest display screenshot"));
    }
};

class UIActionMenuVideoCapture : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuVideoCapture(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCapture; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCapture); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCapture); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Video Capture"));
    }
};

class UIActionSimpleShowVideoCaptureSettingsDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowVideoCaptureSettingsDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/video_capture_settings_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCaptureSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCaptureSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCaptureSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("VideoCaptureSettingsDialog");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Video Capture Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display virtual machine settings window to configure video capture"));
    }
};

class UIActionToggleVideoCapture : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleVideoCapture(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/video_capture_on_16px.png", ":/video_capture_16px.png",
                         ":/video_capture_on_disabled_16px.png", ":/video_capture_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_StartVideoCapture; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_StartVideoCapture); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_StartVideoCapture); }

    QString shortcutExtraDataID() const
    {
        return QString("VideoCapture");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Video Capture"));
        setStatusTip(QApplication::translate("UIActionPool", "Enable guest display video capture"));
    }
};

class UIActionToggleVRDEServer : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleVRDEServer(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/vrdp_on_16px.png", ":/vrdp_16px.png",
                         ":/vrdp_on_disabled_16px.png", ":/vrdp_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer); }

    QString shortcutExtraDataID() const
    {
        return QString("VRDPServer");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "R&emote Display"));
        setStatusTip(QApplication::translate("UIActionPool", "Allow remote desktop (RDP) connections to this machine"));
    }
};

class UIActionMenuMenuBar : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuMenuBar(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/menubar_16px.png", ":/menubar_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Menu Bar"));
    }
};

class UIActionSimpleShowMenuBarSettingsWindow : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowMenuBarSettingsWindow(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/menubar_settings_16px.png", ":/menubar_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBarSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBarSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBarSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("MenuBarSettings");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Menu Bar Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display window to configure menu-bar"));
    }
};

#ifndef RT_OS_DARWIN
class UIActionToggleMenuBar : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleMenuBar(UIActionPool *pParent)
        : UIActionToggle(pParent, ":/menubar_on_16px.png", ":/menubar_16px.png",
                                  ":/menubar_on_disabled_16px.png", ":/menubar_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleMenuBar; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleMenuBar); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleMenuBar); }

    QString shortcutExtraDataID() const
    {
        return QString("ToggleMenuBar");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Show Menu &Bar"));
        setStatusTip(QApplication::translate("UIActionPool", "Enable menu-bar"));
    }
};
#endif /* !RT_OS_DARWIN */

class UIActionMenuStatusBar : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuStatusBar(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/statusbar_16px.png", ":/statusbar_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Status Bar"));
    }
};

class UIActionSimpleShowStatusBarSettingsWindow : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowStatusBarSettingsWindow(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/statusbar_settings_16px.png", ":/statusbar_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBarSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBarSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBarSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("StatusBarSettings");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Status Bar Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display window to configure status-bar"));
    }
};

class UIActionToggleStatusBar : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleStatusBar(UIActionPool *pParent)
        : UIActionToggle(pParent, ":/statusbar_on_16px.png", ":/statusbar_16px.png",
                                  ":/statusbar_on_disabled_16px.png", ":/statusbar_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleStatusBar; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleStatusBar); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleStatusBar); }

    QString shortcutExtraDataID() const
    {
        return QString("ToggleStatusBar");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Show Status &Bar"));
        setStatusTip(QApplication::translate("UIActionPool", "Enable status-bar"));
    }
};

class UIActionMenuScaleFactor : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuScaleFactor(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/scale_factor_16px.png", ":/scale_factor_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuViewActionType_ScaleFactor; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_ScaleFactor); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_ScaleFactor); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "S&cale Factor"));
    }
};

class UIActionMenuInput : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuInput(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::MenuType_Input; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_Input); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_Input); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Input"));
    }
};

class UIActionMenuKeyboard : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuKeyboard(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/keyboard_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_Keyboard; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_Keyboard); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_Keyboard); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Keyboard"));
    }
};

class UIActionSimpleKeyboardSettings : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleKeyboardSettings(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/keyboard_settings_16px.png", ":/keyboard_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("KeyboardSettings");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Keyboard Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display global preferences window to configure keyboard shortcuts"));
    }
};

class UIActionSimplePerformTypeCAD : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTypeCAD(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/hostkey_16px.png", ":/hostkey_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCAD; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCAD); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCAD); }

    QString shortcutExtraDataID() const
    {
        return QString("TypeCAD");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Del");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Insert %1", "that means send the %1 key sequence to the virtual machine").arg("Ctrl-Alt-Del"));
        setStatusTip(QApplication::translate("UIActionPool", "Send the %1 sequence to the virtual machine").arg("Ctrl-Alt-Del"));
    }
};

#ifdef VBOX_WS_X11
class UIActionSimplePerformTypeCABS : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTypeCABS(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/hostkey_16px.png", ":/hostkey_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCABS; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCABS); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCABS); }

    QString shortcutExtraDataID() const
    {
        return QString("TypeCABS");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Backspace");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Insert %1", "that means send the %1 key sequence to the virtual machine").arg("Ctrl-Alt-Backspace"));
        setStatusTip(QApplication::translate("UIActionPool", "Send the %1 sequence to the virtual machine").arg("Ctrl-Alt-Backspace"));
    }
};
#endif /* VBOX_WS_X11 */

class UIActionSimplePerformTypeCtrlBreak : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTypeCtrlBreak(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/hostkey_16px.png", ":/hostkey_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCtrlBreak; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCtrlBreak); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCtrlBreak); }

    QString shortcutExtraDataID() const
    {
        return QString("TypeCtrlBreak");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Insert %1", "that means send the %1 key sequence to the virtual machine").arg("Ctrl-Break"));
        setStatusTip(QApplication::translate("UIActionPool", "Send the %1 sequence to the virtual machine").arg("Ctrl-Break"));
    }
};

class UIActionSimplePerformTypeInsert : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTypeInsert(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/hostkey_16px.png", ":/hostkey_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeInsert; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeInsert); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeInsert); }

    QString shortcutExtraDataID() const
    {
        return QString("TypeInsert");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Insert %1", "that means send the %1 key sequence to the virtual machine").arg("Insert"));
        setStatusTip(QApplication::translate("UIActionPool", "Send the %1 sequence to the virtual machine").arg("Insert"));
    }
};

class UIActionSimplePerformTypePrintScreen : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTypePrintScreen(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/hostkey_16px.png", ":/hostkey_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypePrintScreen; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypePrintScreen); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypePrintScreen); }

    QString shortcutExtraDataID() const
    {
        return QString("TypePrintScreen");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Insert %1", "that means send the %1 key sequence to the virtual machine").arg("Print Screen"));
        setStatusTip(QApplication::translate("UIActionPool", "Send the %1 sequence to the virtual machine").arg("Print Screen"));
    }
};

class UIActionSimplePerformTypeAltPrintScreen : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformTypeAltPrintScreen(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/hostkey_16px.png", ":/hostkey_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeAltPrintScreen; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeAltPrintScreen); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeAltPrintScreen); }

    QString shortcutExtraDataID() const
    {
        return QString("TypeAltPrintScreen");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Insert %1", "that means send the %1 key sequence to the virtual machine").arg("Alt Print Screen"));
        setStatusTip(QApplication::translate("UIActionPool", "Send the %1 sequence to the virtual machine").arg("Alt Print Screen"));
    }
};

class UIActionMenuMouse : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuMouse(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_Mouse; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_Mouse); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_Mouse); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Mouse"));
    }
};

class UIActionToggleMouseIntegration : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleMouseIntegration(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/mouse_can_seamless_on_16px.png", ":/mouse_can_seamless_16px.png",
                         ":/mouse_can_seamless_on_disabled_16px.png", ":/mouse_can_seamless_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuInputActionType_MouseIntegration; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuInputActionType_MouseIntegration); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_MouseIntegration); }

    QString shortcutExtraDataID() const
    {
        return QString("MouseIntegration");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Mouse Integration"));
        setStatusTip(QApplication::translate("UIActionPool", "Enable host mouse pointer integration"));
    }
};

class UIActionMenuDevices : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuDevices(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::MenuType_Devices; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_Devices); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_Devices); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Devices"));
    }
};

class UIActionMenuHardDrives : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuHardDrives(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/hd_16px.png", ":/hd_disabled_16px.png")
    {
        setShowToolTip(true);
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrives; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrives); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrives); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Hard Disks"));
    }
};

class UIActionSimpleShowHardDrivesSettingsDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowHardDrivesSettingsDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/hd_settings_16px.png", ":/hd_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("HardDriveSettingsDialog");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Hard Disk Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display virtual machine settings window to configure hard disks"));
    }
};

class UIActionMenuOpticalDevices : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuOpticalDevices(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/cd_16px.png", ":/cd_disabled_16px.png")
    {
        setShowToolTip(true);
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Optical Drives"));
    }
};

class UIActionMenuFloppyDevices : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuFloppyDevices(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/fd_16px.png", ":/fd_disabled_16px.png")
    {
        setShowToolTip(true);
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Floppy Drives"));
    }
};

class UIActionMenuAudio : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuAudio(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/audio_16px.png", ":/audio_all_off_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Audio"));
    }
};

class UIActionToggleAudioOutput : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleAudioOutput(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/audio_output_on_16px.png", ":/audio_output_16px.png",
                         ":/audio_output_on_16px.png", ":/audio_output_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioOutput; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioOutput); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioOutput); }

    QString shortcutExtraDataID() const
    {
        return QString("ToggleAudioOutput");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Audio Output"));
        setStatusTip(QApplication::translate("UIActionPool", "Enable audio output"));
    }
};

class UIActionToggleAudioInput : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleAudioInput(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/audio_input_on_16px.png", ":/audio_input_16px.png",
                         ":/audio_input_on_16px.png", ":/audio_input_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioInput; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioInput); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioInput); }

    QString shortcutExtraDataID() const
    {
        return QString("ToggleAudioInput");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Audio Input"));
        setStatusTip(QApplication::translate("UIActionPool", "Enable audio input"));
    }
};

class UIActionMenuNetworkAdapters : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuNetworkAdapters(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/nw_16px.png", ":/nw_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Network"));
    }
};

class UIActionSimpleShowNetworkSettingsDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowNetworkSettingsDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/nw_settings_16px.png", ":/nw_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("NetworkSettingsDialog");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Network Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display virtual machine settings window to configure network adapters"));
    }
};

class UIActionMenuUSBDevices : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuUSBDevices(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/usb_16px.png", ":/usb_disabled_16px.png")
    {
        setShowToolTip(true);
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&USB"));
    }
};

class UIActionSimpleShowUSBDevicesSettingsDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowUSBDevicesSettingsDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/usb_settings_16px.png", ":/usb_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("USBDevicesSettingsDialog");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&USB Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display virtual machine settings window to configure USB devices"));
    }
};

class UIActionMenuWebCams : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuWebCams(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/web_camera_16px.png", ":/web_camera_disabled_16px.png")
    {
        setShowToolTip(true);
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Webcams"));
    }
};

class UIActionMenuSharedClipboard : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuSharedClipboard(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/shared_clipboard_16px.png", ":/shared_clipboard_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedClipboard; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedClipboard); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedClipboard); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Shared &Clipboard"));
    }
};

class UIActionMenuDragAndDrop : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuDragAndDrop(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/drag_drop_16px.png", ":/drag_drop_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_DragAndDrop; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_DragAndDrop); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_DragAndDrop); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Drag and Drop"));
    }
};

class UIActionMenuSharedFolders : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuSharedFolders(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/sf_16px.png", ":/sf_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFolders; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFolders); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFolders); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Shared Folders"));
    }
};

class UIActionSimpleShowSharedFoldersSettingsDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowSharedFoldersSettingsDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/sf_settings_16px.png", ":/sf_settings_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings); }

    QString shortcutExtraDataID() const
    {
        return QString("SharedFoldersSettingsDialog");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Shared Folders Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display virtual machine settings window to configure shared folders"));
    }
};

class UIActionSimplePerformInstallGuestTools : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePerformInstallGuestTools(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/guesttools_16px.png", ":/guesttools_disabled_16px.png") {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_InstallGuestTools; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_InstallGuestTools); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_InstallGuestTools); }

    QString shortcutExtraDataID() const
    {
        return QString("InstallGuestAdditions");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Insert Guest Additions CD image..."));
        setStatusTip(QApplication::translate("UIActionPool", "Insert the Guest Additions disk file into the virtual optical drive"));
    }
};

#ifdef VBOX_WITH_DEBUGGER_GUI
class UIActionMenuDebug : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuDebug(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::MenuType_Debug; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_Debug); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_Debug); }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "De&bug"));
    }
};

class UIActionSimpleShowStatistics : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowStatistics(UIActionPool *pParent)
        : UIActionSimple(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Statistics; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Statistics); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Statistics); }

    QString shortcutExtraDataID() const
    {
        return QString("StatisticWindow");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Statistics...", "debug action"));
    }
};

class UIActionSimpleShowCommandLine : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowCommandLine(UIActionPool *pParent)
        : UIActionSimple(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_CommandLine; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_CommandLine); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_CommandLine); }

    QString shortcutExtraDataID() const
    {
        return QString("CommandLineWindow");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Command Line...", "debug action"));
    }
};

class UIActionToggleLogging : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleLogging(UIActionPool *pParent)
        : UIActionToggle(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Logging; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Logging); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Logging); }

    QString shortcutExtraDataID() const
    {
        return QString("Logging");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Logging", "debug action"));
    }
};

class UIActionSimpleShowLogDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleShowLogDialog(UIActionPool *pParent)
        : UIActionSimple(pParent) {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_LogDialog; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_LogDialog); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return actionPool()->toRuntime()->isAllowedInMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_LogDialog); }

    QString shortcutExtraDataID() const
    {
        return QString("LogWindow");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Show &Log...", "debug action"));
    }
};
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef RT_OS_DARWIN
class UIActionMenuDock : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuDock(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    void retranslateUi() {}
};

class UIActionMenuDockSettings : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuDockSettings(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Dock Icon"));
    }
};

class UIActionToggleDockPreviewMonitor : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleDockPreviewMonitor(UIActionPool *pParent)
        : UIActionToggle(pParent) {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("DockPreviewMonitor");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Show Monitor Preview"));
    }
};

class UIActionToggleDockDisableMonitor : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleDockDisableMonitor(UIActionPool *pParent)
        : UIActionToggle(pParent) {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("DockDisableMonitor");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Show Application Icon"));
    }
};

class UIActionToggleDockIconDisableOverlay : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleDockIconDisableOverlay(UIActionPool *pParent)
        : UIActionToggle(pParent) {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("DockOverlayDisable");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Disable Dock Icon Overlay"));
    }
};
#endif /* VBOX_WS_MAC */

UIActionPoolRuntime::UIActionPoolRuntime(bool fTemporary /* = false */)
    : UIActionPool(UIActionPoolType_Runtime, fTemporary)
    , m_pSession(0)
    , m_pMultiScreenLayout(0)
{
}

void UIActionPoolRuntime::setSession(UISession *pSession)
{
    m_pSession = pSession;
    m_invalidations << UIActionIndexRT_M_View << UIActionIndexRT_M_ViewPopup;
}

void UIActionPoolRuntime::setMultiScreenLayout(UIMultiScreenLayout *pMultiScreenLayout)
{
    /* Do not allow NULL pointers: */
    AssertPtrReturnVoid(pMultiScreenLayout);

    /* Assign new multi-screen layout: */
    m_pMultiScreenLayout = pMultiScreenLayout;

    /* Connect new stuff: */
    connect(this, SIGNAL(sigNotifyAboutTriggeringViewScreenRemap(int, int)),
            m_pMultiScreenLayout, SLOT(sltHandleScreenLayoutChange(int, int)));
    connect(m_pMultiScreenLayout, SIGNAL(sigScreenLayoutUpdate()),
            this, SLOT(sltHandleScreenLayoutUpdate()));

    /* Invalidate View menu: */
    m_invalidations << UIActionIndexRT_M_View;
}

void UIActionPoolRuntime::unsetMultiScreenLayout(UIMultiScreenLayout *pMultiScreenLayout)
{
    /* Do not allow NULL pointers: */
    AssertPtrReturnVoid(pMultiScreenLayout);

    /* Disconnect old stuff: */
    disconnect(this, SIGNAL(sigNotifyAboutTriggeringViewScreenRemap(int, int)),
               pMultiScreenLayout, SLOT(sltHandleScreenLayoutChange(int, int)));
    disconnect(pMultiScreenLayout, SIGNAL(sigScreenLayoutUpdate()),
               this, SLOT(sltHandleScreenLayoutUpdate()));

    /* Unset old multi-screen layout: */
    if (m_pMultiScreenLayout == pMultiScreenLayout)
        m_pMultiScreenLayout = 0;

    /* Invalidate View menu: */
    m_invalidations << UIActionIndexRT_M_View;
}

bool UIActionPoolRuntime::isAllowedInMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType type) const
{
    foreach (const UIExtraDataMetaDefs::RuntimeMenuMachineActionType &restriction, m_restrictedActionsMenuMachine.values())
        if (restriction & type)
            return false;
    return true;
}

void UIActionPoolRuntime::setRestrictionForMenuMachine(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuMachineActionType restriction)
{
    m_restrictedActionsMenuMachine[level] = restriction;
    m_invalidations << UIActionIndexRT_M_Machine;
}

bool UIActionPoolRuntime::isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType type) const
{
    foreach (const UIExtraDataMetaDefs::RuntimeMenuViewActionType &restriction, m_restrictedActionsMenuView.values())
        if (restriction & type)
            return false;
    return true;
}

void UIActionPoolRuntime::setRestrictionForMenuView(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuViewActionType restriction)
{
    m_restrictedActionsMenuView[level] = restriction;
    m_invalidations << UIActionIndexRT_M_View << UIActionIndexRT_M_ViewPopup;
}

bool UIActionPoolRuntime::isAllowedInMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType type) const
{
    foreach (const UIExtraDataMetaDefs::RuntimeMenuInputActionType &restriction, m_restrictedActionsMenuInput.values())
        if (restriction & type)
            return false;
    return true;
}

void UIActionPoolRuntime::setRestrictionForMenuInput(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuInputActionType restriction)
{
    m_restrictedActionsMenuInput[level] = restriction;
    m_invalidations << UIActionIndexRT_M_Input;
}

bool UIActionPoolRuntime::isAllowedInMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType type) const
{
    foreach (const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType &restriction, m_restrictedActionsMenuDevices.values())
        if (restriction & type)
            return false;
    return true;
}

void UIActionPoolRuntime::setRestrictionForMenuDevices(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuDevicesActionType restriction)
{
    m_restrictedActionsMenuDevices[level] = restriction;
    m_invalidations << UIActionIndexRT_M_Devices;
}

#ifdef VBOX_WITH_DEBUGGER_GUI
bool UIActionPoolRuntime::isAllowedInMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType type) const
{
    foreach (const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType &restriction, m_restrictedActionsMenuDebug.values())
        if (restriction & type)
            return false;
    return true;
}

void UIActionPoolRuntime::setRestrictionForMenuDebugger(UIActionRestrictionLevel level, UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType restriction)
{
    m_restrictedActionsMenuDebug[level] = restriction;
    m_invalidations << UIActionIndexRT_M_Debug;
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

void UIActionPoolRuntime::sltHandleConfigurationChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (vboxGlobal().managedVMUuid() != strMachineID)
        return;

    /* Update configuration: */
    updateConfiguration();
}

void UIActionPoolRuntime::sltHandleActionTriggerViewScaleFactor(QAction *pAction)
{
    /* Make sure sender is valid: */
    AssertPtrReturnVoid(pAction);

    /* Change scale-factor directly: */
    const double dScaleFactor = pAction->property("Requested Scale Factor").toDouble();
    gEDataManager->setScaleFactor(dScaleFactor, vboxGlobal().managedVMUuid());
}

void UIActionPoolRuntime::sltPrepareMenuViewScreen()
{
    /* Make sure sender is valid: */
    QMenu *pMenu = qobject_cast<QMenu*>(sender());
    AssertPtrReturnVoid(pMenu);

    /* Call to corresponding handler: */
    updateMenuViewScreen(pMenu);
}

void UIActionPoolRuntime::sltPrepareMenuViewMultiscreen()
{
    /* Make sure sender is valid: */
    QMenu *pMenu = qobject_cast<QMenu*>(sender());
    AssertPtrReturnVoid(pMenu);

    /* Call to corresponding handler: */
    updateMenuViewMultiscreen(pMenu);
}

void UIActionPoolRuntime::sltHandleActionTriggerViewScreenToggle()
{
    /* Make sure sender is valid: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertPtrReturnVoid(pAction);

    /* Send request to enable/disable guest-screen: */
    const int iGuestScreenIndex = pAction->property("Guest Screen Index").toInt();
    const bool fScreenEnabled = pAction->isChecked();
    emit sigNotifyAboutTriggeringViewScreenToggle(iGuestScreenIndex, fScreenEnabled);
}

void UIActionPoolRuntime::sltHandleActionTriggerViewScreenResize(QAction *pAction)
{
    /* Make sure sender is valid: */
    AssertPtrReturnVoid(pAction);

    /* Send request to resize guest-screen to required size: */
    const int iGuestScreenIndex = pAction->property("Guest Screen Index").toInt();
    const QSize size = pAction->property("Requested Size").toSize();
    emit sigNotifyAboutTriggeringViewScreenResize(iGuestScreenIndex, size);
}

void UIActionPoolRuntime::sltHandleActionTriggerViewScreenRemap(QAction *pAction)
{
    /* Make sure sender is valid: */
    AssertPtrReturnVoid(pAction);

    /* Send request to remap guest-screen to required host-screen: */
    const int iGuestScreenIndex = pAction->property("Guest Screen Index").toInt();
    const int iHostScreenIndex = pAction->property("Host Screen Index").toInt();
    emit sigNotifyAboutTriggeringViewScreenRemap(iGuestScreenIndex, iHostScreenIndex);
}

void UIActionPoolRuntime::sltHandleScreenLayoutUpdate()
{
    /* Invalidate View menu: */
    m_invalidations << UIActionIndexRT_M_View;
}

void UIActionPoolRuntime::preparePool()
{
    /* 'Machine' actions: */
    m_pool[UIActionIndexRT_M_Machine] = new UIActionMenuMachineRuntime(this);
    m_pool[UIActionIndexRT_M_Machine_S_Settings] = new UIActionSimpleShowSettingsDialog(this);
    m_pool[UIActionIndexRT_M_Machine_S_TakeSnapshot] = new UIActionSimplePerformTakeSnapshot(this);
    m_pool[UIActionIndexRT_M_Machine_S_ShowInformation] = new UIActionSimpleShowInformationDialog(this);
    m_pool[UIActionIndexRT_M_Machine_T_Pause] = new UIActionTogglePause(this);
    m_pool[UIActionIndexRT_M_Machine_S_Reset] = new UIActionSimplePerformReset(this);
    m_pool[UIActionIndexRT_M_Machine_S_Detach] = new UIActionSimplePerformDetach(this);
    m_pool[UIActionIndexRT_M_Machine_S_SaveState] = new UIActionSimplePerformSaveState(this);
    m_pool[UIActionIndexRT_M_Machine_S_Shutdown] = new UIActionSimplePerformShutdown(this);
    m_pool[UIActionIndexRT_M_Machine_S_PowerOff] = new UIActionSimplePerformPowerOff(this);

    /* 'View' actions: */
    m_pool[UIActionIndexRT_M_View] = new UIActionMenuView(this);
    m_pool[UIActionIndexRT_M_ViewPopup] = new UIActionMenuViewPopup(this);
    m_pool[UIActionIndexRT_M_View_T_Fullscreen] = new UIActionToggleFullscreenMode(this);
    m_pool[UIActionIndexRT_M_View_T_Seamless] = new UIActionToggleSeamlessMode(this);
    m_pool[UIActionIndexRT_M_View_T_Scale] = new UIActionToggleScaleMode(this);
#ifndef RT_OS_DARWIN
    m_pool[UIActionIndexRT_M_View_S_MinimizeWindow] = new UIActionSimplePerformMinimizeWindow(this);
#endif /* !RT_OS_DARWIN */
    m_pool[UIActionIndexRT_M_View_S_AdjustWindow] = new UIActionSimplePerformWindowAdjust(this);
    m_pool[UIActionIndexRT_M_View_T_GuestAutoresize] = new UIActionToggleGuestAutoresize(this);
    m_pool[UIActionIndexRT_M_View_S_TakeScreenshot] = new UIActionSimplePerformTakeScreenshot(this);
    m_pool[UIActionIndexRT_M_View_M_VideoCapture] = new UIActionMenuVideoCapture(this);
    m_pool[UIActionIndexRT_M_View_M_VideoCapture_S_Settings] = new UIActionSimpleShowVideoCaptureSettingsDialog(this);
    m_pool[UIActionIndexRT_M_View_M_VideoCapture_T_Start] = new UIActionToggleVideoCapture(this);
    m_pool[UIActionIndexRT_M_View_T_VRDEServer] = new UIActionToggleVRDEServer(this);
    m_pool[UIActionIndexRT_M_View_M_MenuBar] = new UIActionMenuMenuBar(this);
    m_pool[UIActionIndexRT_M_View_M_MenuBar_S_Settings] = new UIActionSimpleShowMenuBarSettingsWindow(this);
#ifndef RT_OS_DARWIN
    m_pool[UIActionIndexRT_M_View_M_MenuBar_T_Visibility] = new UIActionToggleMenuBar(this);
#endif /* !RT_OS_DARWIN */
    m_pool[UIActionIndexRT_M_View_M_StatusBar] = new UIActionMenuStatusBar(this);
    m_pool[UIActionIndexRT_M_View_M_StatusBar_S_Settings] = new UIActionSimpleShowStatusBarSettingsWindow(this);
    m_pool[UIActionIndexRT_M_View_M_StatusBar_T_Visibility] = new UIActionToggleStatusBar(this);
    m_pool[UIActionIndexRT_M_View_M_ScaleFactor] = new UIActionMenuScaleFactor(this);

    /* 'Input' actions: */
    m_pool[UIActionIndexRT_M_Input] = new UIActionMenuInput(this);
    m_pool[UIActionIndexRT_M_Input_M_Keyboard] = new UIActionMenuKeyboard(this);
    m_pool[UIActionIndexRT_M_Input_M_Keyboard_S_Settings] = new UIActionSimpleKeyboardSettings(this);
    m_pool[UIActionIndexRT_M_Input_M_Keyboard_S_TypeCAD] = new UIActionSimplePerformTypeCAD(this);
#ifdef VBOX_WS_X11
    m_pool[UIActionIndexRT_M_Input_M_Keyboard_S_TypeCABS] = new UIActionSimplePerformTypeCABS(this);
#endif /* VBOX_WS_X11 */
    m_pool[UIActionIndexRT_M_Input_M_Keyboard_S_TypeCtrlBreak] = new UIActionSimplePerformTypeCtrlBreak(this);
    m_pool[UIActionIndexRT_M_Input_M_Keyboard_S_TypeInsert] = new UIActionSimplePerformTypeInsert(this);
    m_pool[UIActionIndexRT_M_Input_M_Keyboard_S_TypePrintScreen] = new UIActionSimplePerformTypePrintScreen(this);
    m_pool[UIActionIndexRT_M_Input_M_Keyboard_S_TypeAltPrintScreen] = new UIActionSimplePerformTypeAltPrintScreen(this);
    m_pool[UIActionIndexRT_M_Input_M_Mouse] = new UIActionMenuMouse(this);
    m_pool[UIActionIndexRT_M_Input_M_Mouse_T_Integration] = new UIActionToggleMouseIntegration(this);

    /* 'Devices' actions: */
    m_pool[UIActionIndexRT_M_Devices] = new UIActionMenuDevices(this);
    m_pool[UIActionIndexRT_M_Devices_M_HardDrives] = new UIActionMenuHardDrives(this);
    m_pool[UIActionIndexRT_M_Devices_M_HardDrives_S_Settings] = new UIActionSimpleShowHardDrivesSettingsDialog(this);
    m_pool[UIActionIndexRT_M_Devices_M_OpticalDevices] = new UIActionMenuOpticalDevices(this);
    m_pool[UIActionIndexRT_M_Devices_M_FloppyDevices] = new UIActionMenuFloppyDevices(this);
    m_pool[UIActionIndexRT_M_Devices_M_Audio] = new UIActionMenuAudio(this);
    m_pool[UIActionIndexRT_M_Devices_M_Audio_T_Output] = new UIActionToggleAudioOutput(this);
    m_pool[UIActionIndexRT_M_Devices_M_Audio_T_Input] = new UIActionToggleAudioInput(this);
    m_pool[UIActionIndexRT_M_Devices_M_Network] = new UIActionMenuNetworkAdapters(this);
    m_pool[UIActionIndexRT_M_Devices_M_Network_S_Settings] = new UIActionSimpleShowNetworkSettingsDialog(this);
    m_pool[UIActionIndexRT_M_Devices_M_USBDevices] = new UIActionMenuUSBDevices(this);
    m_pool[UIActionIndexRT_M_Devices_M_USBDevices_S_Settings] = new UIActionSimpleShowUSBDevicesSettingsDialog(this);
    m_pool[UIActionIndexRT_M_Devices_M_WebCams] = new UIActionMenuWebCams(this);
    m_pool[UIActionIndexRT_M_Devices_M_SharedClipboard] = new UIActionMenuSharedClipboard(this);
    m_pool[UIActionIndexRT_M_Devices_M_DragAndDrop] = new UIActionMenuDragAndDrop(this);
    m_pool[UIActionIndexRT_M_Devices_M_SharedFolders] = new UIActionMenuSharedFolders(this);
    m_pool[UIActionIndexRT_M_Devices_M_SharedFolders_S_Settings] = new UIActionSimpleShowSharedFoldersSettingsDialog(this);
    m_pool[UIActionIndexRT_M_Devices_S_InstallGuestTools] = new UIActionSimplePerformInstallGuestTools(this);

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* 'Debug' actions: */
    m_pool[UIActionIndexRT_M_Debug] = new UIActionMenuDebug(this);
    m_pool[UIActionIndexRT_M_Debug_S_ShowStatistics] = new UIActionSimpleShowStatistics(this);
    m_pool[UIActionIndexRT_M_Debug_S_ShowCommandLine] = new UIActionSimpleShowCommandLine(this);
    m_pool[UIActionIndexRT_M_Debug_T_Logging] = new UIActionToggleLogging(this);
    m_pool[UIActionIndexRT_M_Debug_S_ShowLogDialog] = new UIActionSimpleShowLogDialog(this);
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
    /* 'Dock' actions: */
    m_pool[UIActionIndexRT_M_Dock] = new UIActionMenuDock(this);
    m_pool[UIActionIndexRT_M_Dock_M_DockSettings] = new UIActionMenuDockSettings(this);
    m_pool[UIActionIndexRT_M_Dock_M_DockSettings_T_PreviewMonitor] = new UIActionToggleDockPreviewMonitor(this);
    m_pool[UIActionIndexRT_M_Dock_M_DockSettings_T_DisableMonitor] = new UIActionToggleDockDisableMonitor(this);
    m_pool[UIActionIndexRT_M_Dock_M_DockSettings_T_DisableOverlay] = new UIActionToggleDockIconDisableOverlay(this);
#endif /* VBOX_WS_MAC */

    /* Prepare update-handlers for known menus: */
    m_menuUpdateHandlers[UIActionIndexRT_M_Machine].ptfr =                 &UIActionPoolRuntime::updateMenuMachine;
    m_menuUpdateHandlers[UIActionIndexRT_M_View].ptfr =                    &UIActionPoolRuntime::updateMenuView;
    m_menuUpdateHandlers[UIActionIndexRT_M_ViewPopup].ptfr =               &UIActionPoolRuntime::updateMenuViewPopup;
    m_menuUpdateHandlers[UIActionIndexRT_M_View_M_VideoCapture].ptfr =     &UIActionPoolRuntime::updateMenuViewVideoCapture;
    m_menuUpdateHandlers[UIActionIndexRT_M_View_M_MenuBar].ptfr =          &UIActionPoolRuntime::updateMenuViewMenuBar;
    m_menuUpdateHandlers[UIActionIndexRT_M_View_M_StatusBar].ptfr =        &UIActionPoolRuntime::updateMenuViewStatusBar;
    m_menuUpdateHandlers[UIActionIndexRT_M_View_M_ScaleFactor].ptfr =      &UIActionPoolRuntime::updateMenuViewScaleFactor;
    m_menuUpdateHandlers[UIActionIndexRT_M_Input].ptfr =                   &UIActionPoolRuntime::updateMenuInput;
    m_menuUpdateHandlers[UIActionIndexRT_M_Input_M_Keyboard].ptfr =        &UIActionPoolRuntime::updateMenuInputKeyboard;
    m_menuUpdateHandlers[UIActionIndexRT_M_Input_M_Mouse].ptfr =           &UIActionPoolRuntime::updateMenuInputMouse;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices].ptfr =                 &UIActionPoolRuntime::updateMenuDevices;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_HardDrives].ptfr =    &UIActionPoolRuntime::updateMenuDevicesHardDrives;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_Audio].ptfr =         &UIActionPoolRuntime::updateMenuDevicesAudio;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_Network].ptfr =       &UIActionPoolRuntime::updateMenuDevicesNetwork;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_USBDevices].ptfr =    &UIActionPoolRuntime::updateMenuDevicesUSBDevices;
    m_menuUpdateHandlers[UIActionIndexRT_M_Devices_M_SharedFolders].ptfr = &UIActionPoolRuntime::updateMenuDevicesSharedFolders;
#ifdef VBOX_WITH_DEBUGGER_GUI
    m_menuUpdateHandlers[UIActionIndexRT_M_Debug].ptfr =                   &UIActionPoolRuntime::updateMenuDebug;
#endif /* VBOX_WITH_DEBUGGER_GUI */

    /* Call to base-class: */
    UIActionPool::preparePool();
}

void UIActionPoolRuntime::prepareConnections()
{
    /* Prepare connections: */
    connect(gShortcutPool, SIGNAL(sigSelectorShortcutsReloaded()), this, SLOT(sltApplyShortcuts()));
    connect(gShortcutPool, SIGNAL(sigMachineShortcutsReloaded()), this, SLOT(sltApplyShortcuts()));
    connect(gEDataManager, SIGNAL(sigMenuBarConfigurationChange(const QString&)),
            this, SLOT(sltHandleConfigurationChange(const QString&)));

    /* Call to base-class: */
    UIActionPool::prepareConnections();
}

void UIActionPoolRuntime::updateConfiguration()
{
    /* Get machine ID: */
    const QString strMachineID = vboxGlobal().managedVMUuid();
    if (strMachineID.isNull())
        return;

    /* Recache common action restrictions: */
    m_restrictedMenus[UIActionRestrictionLevel_Base] =                  gEDataManager->restrictedRuntimeMenuTypes(strMachineID);
    m_restrictedActionsMenuApplication[UIActionRestrictionLevel_Base] = gEDataManager->restrictedRuntimeMenuApplicationActionTypes(strMachineID);
    m_restrictedActionsMenuMachine[UIActionRestrictionLevel_Base] =     gEDataManager->restrictedRuntimeMenuMachineActionTypes(strMachineID);
    m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] =        gEDataManager->restrictedRuntimeMenuViewActionTypes(strMachineID);
    m_restrictedActionsMenuInput[UIActionRestrictionLevel_Base] =       gEDataManager->restrictedRuntimeMenuInputActionTypes(strMachineID);
    m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] =     gEDataManager->restrictedRuntimeMenuDevicesActionTypes(strMachineID);
#ifdef VBOX_WITH_DEBUGGER_GUI
    m_restrictedActionsMenuDebug[UIActionRestrictionLevel_Base] =       gEDataManager->restrictedRuntimeMenuDebuggerActionTypes(strMachineID);
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    m_restrictedActionsMenuWindow[UIActionRestrictionLevel_Base] =      gEDataManager->restrictedRuntimeMenuWindowActionTypes(strMachineID);
#endif /* VBOX_WS_MAC */
    m_restrictedActionsMenuHelp[UIActionRestrictionLevel_Base] =        gEDataManager->restrictedRuntimeMenuHelpActionTypes(strMachineID);

    /* Recache visual state action restrictions: */
    UIVisualStateType restrictedVisualStates = gEDataManager->restrictedVisualStates(strMachineID);
    {
        if (restrictedVisualStates & UIVisualStateType_Fullscreen)
            m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuViewActionType)
                (m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen);
        if (restrictedVisualStates & UIVisualStateType_Seamless)
            m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuViewActionType)
                (m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless);
        if (restrictedVisualStates & UIVisualStateType_Scale)
            m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuViewActionType)
                (m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale);
    }

    /* Recache reconfiguration action restrictions: */
    bool fReconfigurationAllowed = gEDataManager->machineReconfigurationEnabled(strMachineID);
    if (!fReconfigurationAllowed)
    {
        m_restrictedActionsMenuMachine[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuMachineActionType)
            (m_restrictedActionsMenuMachine[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog);
        m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuViewActionType)
            (m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCaptureSettings);
        m_restrictedActionsMenuInput[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuInputActionType)
            (m_restrictedActionsMenuInput[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings);
        m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)
            (m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings);
        m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)
            (m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings);
        m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)
            (m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings);
        m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)
            (m_restrictedActionsMenuDevices[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings);
    }

    /* Recache snapshot related action restrictions: */
    bool fSnapshotOperationsAllowed = gEDataManager->machineSnapshotOperationsEnabled(strMachineID);
    if (!fSnapshotOperationsAllowed)
    {
        m_restrictedActionsMenuMachine[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuMachineActionType)
            (m_restrictedActionsMenuMachine[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot);
    }

    /* Recache extension-pack related action restrictions: */
#if VBOX_WITH_EXTPACK
    CExtPack extPack = vboxGlobal().virtualBox().GetExtensionPackManager().Find(GUI_ExtPackName);
#else
    CExtPack extPack;
#endif
    bool fExtensionPackOperationsAllowed = !extPack.isNull() && extPack.GetUsable();
    if (!fExtensionPackOperationsAllowed)
    {
        m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::RuntimeMenuViewActionType)
            (m_restrictedActionsMenuView[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer);
    }

    /* Recache close related action restrictions: */
    MachineCloseAction restrictedCloseActions = gEDataManager->restrictedMachineCloseActions(strMachineID);
    bool fAllCloseActionsRestricted =    (!vboxGlobal().isSeparateProcess() || (restrictedCloseActions & MachineCloseAction_Detach))
                                      && (restrictedCloseActions & MachineCloseAction_SaveState)
                                      && (restrictedCloseActions & MachineCloseAction_Shutdown)
                                      && (restrictedCloseActions & MachineCloseAction_PowerOff);
                                      // Close VM Dialog hides PowerOff_RestoringSnapshot implicitly if PowerOff is hidden..
                                      // && (m_restrictedCloseActions & MachineCloseAction_PowerOff_RestoringSnapshot);
    if (fAllCloseActionsRestricted)
    {
        m_restrictedActionsMenuApplication[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::MenuApplicationActionType)
            (m_restrictedActionsMenuApplication[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::MenuApplicationActionType_Close);
    }

    /* Call to base-class: */
    UIActionPool::updateConfiguration();
}

void UIActionPoolRuntime::updateMenu(int iIndex)
{
    /* Call to base-class: */
    if (iIndex < UIActionIndex_Max)
        UIActionPool::updateMenu(iIndex);

    /* If menu with such index is invalidated and there is update-handler: */
    if (m_invalidations.contains(iIndex) && m_menuUpdateHandlers.contains(iIndex))
        (this->*(m_menuUpdateHandlers.value(iIndex).ptfr))();
}

void UIActionPoolRuntime::updateMenus()
{
    /* Clear menu list: */
    m_mainMenus.clear();

    /* 'Application' menu: */
    addMenu(m_mainMenus, action(UIActionIndex_M_Application));
    updateMenuApplication();

    /* 'Machine' menu: */
    addMenu(m_mainMenus, action(UIActionIndexRT_M_Machine));
    updateMenuMachine();

    /* 'View' menu: */
    addMenu(m_mainMenus, action(UIActionIndexRT_M_View));
    updateMenuView();
    /* 'View' popup menu: */
    addMenu(m_mainMenus, action(UIActionIndexRT_M_ViewPopup), false);
    updateMenuViewPopup();

    /* 'Input' menu: */
    addMenu(m_mainMenus, action(UIActionIndexRT_M_Input));
    updateMenuInput();

    /* 'Devices' menu: */
    addMenu(m_mainMenus, action(UIActionIndexRT_M_Devices));
    updateMenuDevices();

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* 'Debug' menu: */
    addMenu(m_mainMenus, action(UIActionIndexRT_M_Debug), vboxGlobal().isDebuggerEnabled());
    updateMenuDebug();
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef RT_OS_DARWIN
    /* 'Window' menu: */
    addMenu(m_mainMenus, action(UIActionIndex_M_Window));
    updateMenuWindow();
#endif /* RT_OS_DARWIN */

    /* 'Help' menu: */
    addMenu(m_mainMenus, action(UIActionIndex_Menu_Help));
    updateMenuHelp();
}

void UIActionPoolRuntime::updateMenuMachine()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Machine)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Settings Dialog' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_Settings)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Take Snapshot' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_TakeSnapshot)) || fSeparator;
    /* 'Information Dialog' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_ShowInformation)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Pause' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_T_Pause)) || fSeparator;
    /* 'Reset' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_Reset)) || fSeparator;
    /* 'Detach' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_Detach)) || fSeparator;
    /* 'SaveState' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_SaveState)) || fSeparator;
    /* 'Shutdown' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_Shutdown)) || fSeparator;
    /* 'PowerOff' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Machine_S_PowerOff)) || fSeparator;

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Machine);
}

void UIActionPoolRuntime::updateMenuView()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_View)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Fullscreen' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_T_Fullscreen)) || fSeparator;
    /* 'Seamless' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_T_Seamless)) || fSeparator;
    /* 'Scale' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_T_Scale)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Adjust Window' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_S_AdjustWindow)) || fSeparator;
    /* 'Guest Autoresize' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_T_GuestAutoresize)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Take Screenshot' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_S_TakeScreenshot)) || fSeparator;
    /* 'Video Capture' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_VideoCapture), false) || fSeparator;
    updateMenuViewVideoCapture();
    /* 'Video Capture Start' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_VideoCapture_T_Start)) || fSeparator;
    /* 'VRDE Server' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_T_VRDEServer)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Menu Bar' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_MenuBar)) || fSeparator;
    updateMenuViewMenuBar();
    /* 'Status Bar' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_StatusBar)) || fSeparator;
    updateMenuViewStatusBar();

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Scale Factor' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_ScaleFactor)) || fSeparator;
    updateMenuViewScaleFactor();

    /* Do we have to show resize or multiscreen menu? */
    const bool fAllowToShowActionResize = isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize);
    const bool fAllowToShowActionMultiscreen = isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Multiscreen);
    if (fAllowToShowActionResize && uisession())
    {
        for (int iGuestScreenIndex = 0; iGuestScreenIndex < uisession()->frameBuffers().size(); ++iGuestScreenIndex)
        {
            /* Add 'Virtual Screen %1' menu: */
            QMenu *pSubMenu = pMenu->addMenu(UIIconPool::iconSet(":/virtual_screen_16px.png",
                                                                 ":/virtual_screen_disabled_16px.png"),
                                             QApplication::translate("UIMultiScreenLayout", "Virtual Screen %1").arg(iGuestScreenIndex + 1));
            pSubMenu->setProperty("Guest Screen Index", iGuestScreenIndex);
            connect(pSubMenu, SIGNAL(aboutToShow()), this, SLOT(sltPrepareMenuViewScreen()));
        }
    }
    else if (fAllowToShowActionMultiscreen && multiScreenLayout())
    {
        /* Only for multi-screen host case: */
        if (uisession()->hostScreens().size() > 1)
        {
            for (int iGuestScreenIndex = 0; iGuestScreenIndex < uisession()->frameBuffers().size(); ++iGuestScreenIndex)
            {
                /* Add 'Virtual Screen %1' menu: */
                QMenu *pSubMenu = pMenu->addMenu(UIIconPool::iconSet(":/virtual_screen_16px.png",
                                                                     ":/virtual_screen_disabled_16px.png"),
                                                 QApplication::translate("UIMultiScreenLayout", "Virtual Screen %1").arg(iGuestScreenIndex + 1));
                pSubMenu->setProperty("Guest Screen Index", iGuestScreenIndex);
                connect(pSubMenu, SIGNAL(aboutToShow()), this, SLOT(sltPrepareMenuViewMultiscreen()));
            }
        }
    }

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_View);
}

void UIActionPoolRuntime::updateMenuViewPopup()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_ViewPopup)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Adjust Window' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_S_AdjustWindow)) || fSeparator;
    /* 'Guest Autoresize' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_T_GuestAutoresize)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Scale Factor' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_ScaleFactor)) || fSeparator;
    updateMenuViewScaleFactor();

    /* Do we have to show resize menu? */
    const bool fAllowToShowActionResize = isAllowedInMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize);
    if (fAllowToShowActionResize && uisession())
    {
        for (int iGuestScreenIndex = 0; iGuestScreenIndex < uisession()->frameBuffers().size(); ++iGuestScreenIndex)
        {
            /* Add 'Virtual Screen %1' menu: */
            QMenu *pSubMenu = pMenu->addMenu(UIIconPool::iconSet(":/virtual_screen_16px.png",
                                                                 ":/virtual_screen_disabled_16px.png"),
                                             QApplication::translate("UIMultiScreenLayout", "Virtual Screen %1").arg(iGuestScreenIndex + 1));
            pSubMenu->setProperty("Guest Screen Index", iGuestScreenIndex);
            connect(pSubMenu, SIGNAL(aboutToShow()), this, SLOT(sltPrepareMenuViewScreen()));
        }
    }

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_ViewPopup);
}

void UIActionPoolRuntime::updateMenuViewVideoCapture()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_View_M_VideoCapture)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Video Capture Settings' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_VideoCapture_S_Settings)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Start Video Capture' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_View_M_VideoCapture_T_Start)) || fSeparator;

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_View_M_VideoCapture);
}

void UIActionPoolRuntime::updateMenuViewMenuBar()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_View_M_MenuBar)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Menu Bar Settings' action: */
    addAction(pMenu, action(UIActionIndexRT_M_View_M_MenuBar_S_Settings));
#ifndef VBOX_WS_MAC
    /* 'Toggle Menu Bar' action: */
    addAction(pMenu, action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility));
#endif /* !VBOX_WS_MAC */

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_View_M_MenuBar);
}

void UIActionPoolRuntime::updateMenuViewStatusBar()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_View_M_StatusBar)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Status Bar Settings' action: */
    addAction(pMenu, action(UIActionIndexRT_M_View_M_StatusBar_S_Settings));
    /* 'Toggle Status Bar' action: */
    addAction(pMenu, action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility));

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_View_M_StatusBar);
}

void UIActionPoolRuntime::updateMenuViewScaleFactor()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_View_M_ScaleFactor)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Get corresponding scale-factor: */
    const double dCurrentScaleFactor = gEDataManager->scaleFactor(vboxGlobal().managedVMUuid());

    /* Prepare new contents: */
    const QList<double> factors = QList<double>()
                                  << 1.0
                                  << 1.1
                                  << 1.25
                                  << 1.5
                                  << 1.75
                                  << 2.0;

    /* Create exclusive 'scale-factor' action-group: */
    QActionGroup *pActionGroup = new QActionGroup(pMenu);
    AssertPtrReturnVoid(pActionGroup);
    {
        /* Configure exclusive 'scale-factor' action-group: */
        pActionGroup->setExclusive(true);
        /* For every available scale-factor: */
        foreach (const double &dScaleFactor, factors)
        {
            /* Create exclusive 'scale-factor' action: */
            QAction *pAction = pActionGroup->addAction(QApplication::translate("UIActionPool", "%1%", "scale-factor")
                                                                               .arg(dScaleFactor * 100));
            AssertPtrReturnVoid(pAction);
            {
                /* Configure exclusive 'scale-factor' action: */
                pAction->setProperty("Requested Scale Factor", dScaleFactor);
                pAction->setCheckable(true);
                if (dScaleFactor == dCurrentScaleFactor)
                    pAction->setChecked(true);
            }
        }
        /* Insert group actions into menu: */
        pMenu->addActions(pActionGroup->actions());
        /* Install listener for exclusive action-group: */
        connect(pActionGroup, SIGNAL(triggered(QAction*)),
                this, SLOT(sltHandleActionTriggerViewScaleFactor(QAction*)));
    }
}

void UIActionPoolRuntime::updateMenuViewScreen(QMenu *pMenu)
{
    /* Make sure UI session defined: */
    AssertPtrReturnVoid(uisession());

    /* Clear contents: */
    pMenu->clear();

    /* Prepare new contents: */
    const QList<QSize> sizes = QList<QSize>()
                               << QSize(640, 480)
                               << QSize(800, 600)
                               << QSize(1024, 768)
                               << QSize(1152, 864)
                               << QSize(1280, 720)
                               << QSize(1280, 800)
                               << QSize(1366, 768)
                               << QSize(1440, 900)
                               << QSize(1600, 900)
                               << QSize(1680, 1050)
                               << QSize(1920, 1080)
                               << QSize(1920, 1200);

    /* Get corresponding screen index and frame-buffer size: */
    const int iGuestScreenIndex = pMenu->property("Guest Screen Index").toInt();
    const UIFrameBuffer* pFrameBuffer = uisession()->frameBuffer(iGuestScreenIndex);
    const QSize screenSize = QSize(pFrameBuffer->width(), pFrameBuffer->height());
    const bool fScreenEnabled = uisession()->isScreenVisible(iGuestScreenIndex);

    /* For non-primary screens: */
    if (iGuestScreenIndex > 0)
    {
        /* Create 'toggle' action: */
        QAction *pToggleAction = pMenu->addAction(QApplication::translate("UIActionPool", "Enable", "Virtual Screen"),
                                                  this, SLOT(sltHandleActionTriggerViewScreenToggle()));
        AssertPtrReturnVoid(pToggleAction);
        {
            /* Configure 'toggle' action: */
            pToggleAction->setEnabled(uisession()->isGuestSupportsGraphics());
            pToggleAction->setProperty("Guest Screen Index", iGuestScreenIndex);
            pToggleAction->setCheckable(true);
            pToggleAction->setChecked(fScreenEnabled);
            /* Add separator: */
            pMenu->addSeparator();
        }
    }

    /* Create exclusive 'resize' action-group: */
    QActionGroup *pActionGroup = new QActionGroup(pMenu);
    AssertPtrReturnVoid(pActionGroup);
    {
        /* Configure exclusive 'resize' action-group: */
        pActionGroup->setExclusive(true);
        /* For every available size: */
        foreach (const QSize &size, sizes)
        {
            /* Create exclusive 'resize' action: */
            QAction *pAction = pActionGroup->addAction(QApplication::translate("UIActionPool", "Resize to %1x%2", "Virtual Screen")
                                                                               .arg(size.width()).arg(size.height()));
            AssertPtrReturnVoid(pAction);
            {
                /* Configure exclusive 'resize' action: */
                pAction->setEnabled(uisession()->isGuestSupportsGraphics() && fScreenEnabled);
                pAction->setProperty("Guest Screen Index", iGuestScreenIndex);
                pAction->setProperty("Requested Size", size);
                pAction->setCheckable(true);
                if (screenSize.width() == size.width() &&
                    screenSize.height() == size.height())
                    pAction->setChecked(true);
            }
        }
        /* Insert group actions into menu: */
        pMenu->addActions(pActionGroup->actions());
        /* Install listener for exclusive action-group: */
        connect(pActionGroup, SIGNAL(triggered(QAction*)),
                this, SLOT(sltHandleActionTriggerViewScreenResize(QAction*)));
    }
}

void UIActionPoolRuntime::updateMenuViewMultiscreen(QMenu *pMenu)
{
    /* Make sure UI session defined: */
    AssertPtrReturnVoid(multiScreenLayout());

    /* Clear contents: */
    pMenu->clear();

    /* Get corresponding screen index and size: */
    const int iGuestScreenIndex = pMenu->property("Guest Screen Index").toInt();

    /* Create exclusive action-group: */
    QActionGroup *pActionGroup = new QActionGroup(pMenu);
    AssertPtrReturnVoid(pActionGroup);
    {
        /* Configure exclusive action-group: */
        pActionGroup->setExclusive(true);
        for (int iHostScreenIndex = 0; iHostScreenIndex < uisession()->hostScreens().size(); ++iHostScreenIndex)
        {
            QAction *pAction = pActionGroup->addAction(UIMultiScreenLayout::tr("Use Host Screen %1")
                                                                               .arg(iHostScreenIndex + 1));
            AssertPtrReturnVoid(pAction);
            {
                pAction->setCheckable(true);
                pAction->setProperty("Guest Screen Index", iGuestScreenIndex);
                pAction->setProperty("Host Screen Index", iHostScreenIndex);
                if (multiScreenLayout()->hasHostScreenForGuestScreen(iGuestScreenIndex) &&
                    multiScreenLayout()->hostScreenForGuestScreen(iGuestScreenIndex) == iHostScreenIndex)
                    pAction->setChecked(true);
            }
        }
        /* Insert group actions into menu: */
        pMenu->addActions(pActionGroup->actions());
        /* Install listener for exclusive action-group: */
        connect(pActionGroup, SIGNAL(triggered(QAction*)),
                this, SLOT(sltHandleActionTriggerViewScreenRemap(QAction*)));
    }
}

void UIActionPoolRuntime::updateMenuInput()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Input)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Keyboard' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard)) || fSeparator;
    updateMenuInputKeyboard();
    /* 'Mouse' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Mouse), false) || fSeparator;
    updateMenuInputMouse();

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Mouse Integration' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Mouse_T_Integration)) || fSeparator;

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Input);
}

void UIActionPoolRuntime::updateMenuInputKeyboard()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Input_M_Keyboard)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Keyboard Settings' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard_S_Settings)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Type CAD' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCAD)) || fSeparator;
#ifdef VBOX_WS_X11
    /* 'Type CABS' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCABS)) || fSeparator;
#endif /* VBOX_WS_X11 */
    /* 'Type Ctrl-Break' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeCtrlBreak)) || fSeparator;
    /* 'Type Insert' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeInsert)) || fSeparator;
    /* 'Type Print Screen' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard_S_TypePrintScreen)) || fSeparator;
    /* 'Type Alt Print Screen' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Input_M_Keyboard_S_TypeAltPrintScreen)) || fSeparator;

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Input_M_Keyboard);
}

void UIActionPoolRuntime::updateMenuInputMouse()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Input_M_Mouse)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Machine Integration' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Input_M_Mouse_T_Integration));

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Input_M_Mouse);
}

void UIActionPoolRuntime::updateMenuDevices()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Devices)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Hard Drives' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_HardDrives)) || fSeparator;
    updateMenuDevicesHardDrives();
    /* 'Optical Devices' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_OpticalDevices)) || fSeparator;
    /* 'Floppy Devices' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_FloppyDevices)) || fSeparator;
    /* 'Audio' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_Audio)) || fSeparator;
    updateMenuDevicesAudio();
    /* 'Network' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_Network)) || fSeparator;
    updateMenuDevicesNetwork();
    /* 'USB Devices' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_USBDevices)) || fSeparator;
    updateMenuDevicesUSBDevices();
    /* 'Web Cams' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_WebCams)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Shared Folders' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_SharedFolders)) || fSeparator;
    updateMenuDevicesSharedFolders();
    /* 'Shared Clipboard' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_SharedClipboard)) || fSeparator;
    /* 'Drag&Drop' submenu: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_DragAndDrop)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* Install Guest Tools action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_S_InstallGuestTools)) || fSeparator;

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Devices);
}

void UIActionPoolRuntime::updateMenuDevicesHardDrives()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Devices_M_HardDrives)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Hard Drives Settings' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Devices_M_HardDrives_S_Settings));

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Devices_M_HardDrives);
}

void UIActionPoolRuntime::updateMenuDevicesAudio()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Devices_M_Audio)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Output' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Devices_M_Audio_T_Output));
    /* 'Input' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Devices_M_Audio_T_Input));

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Devices_M_Audio);
}

void UIActionPoolRuntime::updateMenuDevicesNetwork()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Devices_M_Network)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Network Settings' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_Network_S_Settings)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* This menu always remains invalid.. */
}

void UIActionPoolRuntime::updateMenuDevicesUSBDevices()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Devices_M_USBDevices)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'USB Devices Settings' action: */
    fSeparator = addAction(pMenu, action(UIActionIndexRT_M_Devices_M_USBDevices_S_Settings)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* This menu always remains invalid.. */
}

void UIActionPoolRuntime::updateMenuDevicesSharedFolders()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Devices_M_SharedFolders)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Shared Folders Settings' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Devices_M_SharedFolders_S_Settings));

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Devices_M_SharedFolders);
}

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIActionPoolRuntime::updateMenuDebug()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndexRT_M_Debug)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Statistics' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Debug_S_ShowStatistics));
    /* 'Command Line' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Debug_S_ShowCommandLine));
    /* 'Logging' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Debug_T_Logging));
    /* 'Log Dialog' action: */
    addAction(pMenu, action(UIActionIndexRT_M_Debug_S_ShowLogDialog));

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndexRT_M_Debug);
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

void UIActionPoolRuntime::updateShortcuts()
{
    /* Call to base-class: */
    UIActionPool::updateShortcuts();
    /* Create temporary Selector UI pool to do the same: */
    if (!m_fTemporary)
        UIActionPool::createTemporary(UIActionPoolType_Selector);
}

QString UIActionPoolRuntime::shortcutsExtraDataID() const
{
    return GUI_Input_MachineShortcuts;
}

#include "UIActionPoolRuntime.moc"


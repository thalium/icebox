/* $Id: UIMachineLogic.h $ */
/** @file
 * VBox Qt GUI - UIMachineLogic class declaration.
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

#ifndef ___UIMachineLogic_h___
#define ___UIMachineLogic_h___

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include <QIWithRetranslateUI.h>

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QAction;
class QActionGroup;
class UISession;
class UIActionPool;
class UIKeyboardHandler;
class UIMouseHandler;
class UIMachineWindow;
class UIMachineView;
class UIDockIconPreview;
class CSession;
class CMachine;
class CConsole;
class CDisplay;
class CGuest;
class CMouse;
class CKeyboard;
class CMachineDebugger;
class CSnapshot;
class CUSBDevice;
class CVirtualBoxErrorInfo;

#ifdef VBOX_WITH_DEBUGGER_GUI
typedef struct DBGGUIVT const *PCDBGGUIVT;
typedef struct DBGGUI *PDBGGUI;
#endif /* VBOX_WITH_DEBUGGER_GUI */

/* Machine logic interface: */
class UIMachineLogic : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

    /** Pointer to menu update-handler for this class: */
    typedef void (UIMachineLogic::*MenuUpdateHandler)(QMenu *pMenu);

signals:

    /** Notifies about frame-buffer resize. */
    void sigFrameBufferResize();

    /** Notifies listeners about 3D overlay visibility change. */
    void sigNotifyAbout3DOverlayVisibilityChange(bool fVisible);

public:

    /* Factory functions to create/destroy required logic sub-child: */
    static UIMachineLogic* create(QObject *pParent, UISession *pSession, UIVisualStateType visualStateType);
    static void destroy(UIMachineLogic *pWhichLogic);

    /* Check if this logic is available: */
    virtual bool checkAvailability() = 0;

    /** Returns machine-window flags for current machine-logic and passed @a uScreenId. */
    virtual Qt::WindowFlags windowFlags(ulong uScreenId) const = 0;

    /* Prepare/cleanup the logic: */
    virtual void prepare();
    virtual void cleanup();

    void initializePostPowerUp();

    /* Main getters/setters: */
    UISession* uisession() const { return m_pSession; }
    UIActionPool* actionPool() const;

    /** Returns the session reference. */
    CSession& session() const;
    /** Returns the session's machine reference. */
    CMachine& machine() const;
    /** Returns the session's console reference. */
    CConsole& console() const;
    /** Returns the console's display reference. */
    CDisplay& display() const;
    /** Returns the console's guest reference. */
    CGuest& guest() const;
    /** Returns the console's mouse reference. */
    CMouse& mouse() const;
    /** Returns the console's keyboard reference. */
    CKeyboard& keyboard() const;
    /** Returns the console's debugger reference. */
    CMachineDebugger& debugger() const;

    /** Returns the machine name. */
    const QString& machineName() const;

    UIVisualStateType visualStateType() const { return m_visualStateType; }
    const QList<UIMachineWindow*>& machineWindows() const { return m_machineWindowsList; }
    UIKeyboardHandler* keyboardHandler() const { return m_pKeyboardHandler; }
    UIMouseHandler* mouseHandler() const { return m_pMouseHandler; }
    UIMachineWindow* mainMachineWindow() const;
    UIMachineWindow* activeMachineWindow() const;

    /** Returns whether VM is in 'manual-override' mode.
      * @note S.a. #m_fIsManualOverride description for more information. */
    bool isManualOverrideMode() const { return m_fIsManualOverride; }
    /** Defines whether VM is in 'manual-override' mode.
      * @note S.a. #m_fIsManualOverride description for more information. */
    void setManualOverrideMode(bool fIsManualOverride) { m_fIsManualOverride = fIsManualOverride; }

    /** Adjusts machine-window(s) geometry if necessary. */
    virtual void adjustMachineWindowsGeometry();

    /** Send machine-window(s) size-hint(s) to the guest. */
    virtual void sendMachineWindowsSizeHints();

    /* Wrapper to open Machine settings / Network page: */
    void openNetworkSettingsDialog() { sltOpenNetworkSettingsDialog(); }

#ifdef VBOX_WS_MAC
    void updateDockIcon();
    void updateDockIconSize(int screenId, int width, int height);
    UIMachineView* dockPreviewView() const;
#endif /* VBOX_WS_MAC */

    /** Detach and close Runtime UI. */
    void detach();
    /** Save VM state, then close Runtime UI. */
    void saveState();
    /** Call for guest shutdown to close Runtime UI. */
    void shutdown();
    /** Power off VM, then close Runtime UI. */
    void powerOff(bool fDiscardingState);
    /** Close Runtime UI. */
    void closeRuntimeUI();

    /* API: 3D overlay visibility stuff: */
    virtual void notifyAbout3DOverlayVisibilityChange(bool fVisible);

    /** Returns whether VM should perform HID LEDs synchronization. */
    bool isHidLedsSyncEnabled() const { return m_fIsHidLedsSyncEnabled; }

protected slots:

    /** Handles the VBoxSVC availability change. */
    void sltHandleVBoxSVCAvailabilityChange();

    /** Checks if some visual-state type was requested. */
    virtual void sltCheckForRequestedVisualStateType() {}

    /** Requests visual-state change to 'normal' (window). */
    virtual void sltChangeVisualStateToNormal();
    /** Requests visual-state change to 'fullscreen'. */
    virtual void sltChangeVisualStateToFullscreen();
    /** Requests visual-state change to 'seamless'. */
    virtual void sltChangeVisualStateToSeamless();
    /** Requests visual-state change to 'scale'. */
    virtual void sltChangeVisualStateToScale();

    /* Console callback handlers: */
    virtual void sltMachineStateChanged();
    virtual void sltAdditionsStateChanged();
    virtual void sltMouseCapabilityChanged();
    virtual void sltKeyboardLedsChanged();
    virtual void sltUSBDeviceStateChange(const CUSBDevice &device, bool fIsAttached, const CVirtualBoxErrorInfo &error);
    virtual void sltRuntimeError(bool fIsFatal, const QString &strErrorId, const QString &strMessage);
#ifdef RT_OS_DARWIN
    virtual void sltShowWindows();
#endif /* RT_OS_DARWIN */
    /** Handles guest-screen count change. */
    virtual void sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);

    /** Handles host-screen count change. */
    virtual void sltHostScreenCountChange();
    /** Handles host-screen geometry change. */
    virtual void sltHostScreenGeometryChange();
    /** Handles host-screen available-area change. */
    virtual void sltHostScreenAvailableAreaChange();

protected:

    /* Constructor: */
    UIMachineLogic(QObject *pParent, UISession *pSession, UIVisualStateType visualStateType);

    /* Protected getters/setters: */
    bool isMachineWindowsCreated() const { return m_fIsWindowsCreated; }
    void setMachineWindowsCreated(bool fIsWindowsCreated);

    /* Protected members: */
    void setKeyboardHandler(UIKeyboardHandler *pKeyboardHandler);
    void setMouseHandler(UIMouseHandler *pMouseHandler);
    void addMachineWindow(UIMachineWindow *pMachineWindow);
    void retranslateUi();
#ifdef VBOX_WS_MAC
    bool isDockIconPreviewEnabled() const { return m_fIsDockIconEnabled; }
    void setDockIconPreviewEnabled(bool fIsDockIconPreviewEnabled) { m_fIsDockIconEnabled = fIsDockIconPreviewEnabled; }
    void updateDockOverlay();
#endif /* VBOX_WS_MAC */

    /* Prepare helpers: */
    virtual void prepareRequiredFeatures();
    virtual void prepareSessionConnections();
    virtual void prepareActionGroups();
    virtual void prepareActionConnections();
    virtual void prepareOtherConnections() {}
    virtual void prepareHandlers();
    virtual void prepareMachineWindows() = 0;
    virtual void prepareMenu() {}
#ifdef VBOX_WS_MAC
    virtual void prepareDock();
#endif /* VBOX_WS_MAC */
#ifdef VBOX_WITH_DEBUGGER_GUI
    virtual void prepareDebugger();
#endif /* VBOX_WITH_DEBUGGER_GUI */
    virtual void loadSettings();

    /* Cleanup helpers: */
    virtual void saveSettings();
#ifdef VBOX_WITH_DEBUGGER_GUI
    virtual void cleanupDebugger();
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    virtual void cleanupDock();
#endif /* VBOX_WS_MAC */
    virtual void cleanupMenu() {}
    virtual void cleanupMachineWindows() = 0;
    virtual void cleanupHandlers();
    //virtual void cleanupOtherConnections() {}
    virtual void cleanupActionConnections() {}
    virtual void cleanupActionGroups() {}
    virtual void cleanupSessionConnections();
    //virtual void cleanupRequiredFeatures() {}

    /* Handler: Event-filter stuff: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

private slots:

    /** Handle menu prepare. */
    void sltHandleMenuPrepare(int iIndex, QMenu *pMenu);

    /* "Machine" menu functionality: */
    void sltShowKeyboardSettings();
    void sltToggleMouseIntegration(bool fEnabled);
    void sltTypeCAD();
#ifdef VBOX_WS_X11
    void sltTypeCABS();
#endif /* VBOX_WS_X11 */
    void sltTypeCtrlBreak();
    void sltTypeInsert();
    void sltTypePrintScreen();
    void sltTypeAltPrintScreen();
    void sltTakeSnapshot();
    void sltShowInformationDialog();
    void sltReset();
    void sltPause(bool fOn);
    void sltDetach();
    void sltSaveState();
    void sltShutdown();
    void sltPowerOff();
    void sltClose();

    /* "View" menu functionality: */
    void sltMinimizeActiveMachineWindow();
    void sltAdjustMachineWindows();
    void sltToggleGuestAutoresize(bool fEnabled);
    void sltTakeScreenshot();
    void sltOpenVideoCaptureOptions();
    void sltToggleVideoCapture(bool fEnabled);
    void sltToggleVRDE(bool fEnabled);

    /* "Device" menu functionality: */
    void sltOpenVMSettingsDialog(const QString &strCategory = QString(), const QString &strControl = QString());
    void sltOpenStorageSettingsDialog();
    void sltToggleAudioOutput(bool fEnabled);
    void sltToggleAudioInput(bool fEnabled);
    void sltOpenNetworkSettingsDialog();
    void sltOpenUSBDevicesSettingsDialog();
    void sltOpenSharedFoldersSettingsDialog();
    void sltMountStorageMedium();
    void sltAttachUSBDevice();
    void sltAttachWebCamDevice();
    void sltChangeSharedClipboardType(QAction *pAction);
    void sltToggleNetworkAdapterConnection();
    void sltChangeDragAndDropType(QAction *pAction);
    void sltInstallGuestAdditions();

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* "Debug" menu functionality: */
    void sltShowDebugStatistics();
    void sltShowDebugCommandLine();
    void sltLoggingToggled(bool);
    void sltShowLogDialog();
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef RT_OS_DARWIN /* Something is *really* broken in regards of the moc here */
    /* "Window" menu functionality: */
    void sltSwitchToMachineWindow();

    /* "Dock" menu functionality: */
    void sltDockPreviewModeChanged(QAction *pAction);
    void sltDockPreviewMonitorChanged(QAction *pAction);
    void sltChangeDockIconUpdate(bool fEnabled);
    /** Handles dock icon overlay change event. */
    void sltChangeDockIconOverlayAppearance(bool fDisabled);
    /** Handles dock icon overlay disable action triggering. */
    void sltDockIconDisableOverlayChanged(bool fDisabled);
#endif /* RT_OS_DARWIN */

    /* Handlers: Keyboard LEDs sync logic: */
    void sltHidLedsSyncStateChanged(bool fEnabled);
    void sltSwitchKeyboardLedsToGuestLeds();
    void sltSwitchKeyboardLedsToPreviousLeds();

    /** Show Global Preferences. */
    void sltShowGlobalPreferences();

    /** Close Runtime UI. */
    void sltCloseRuntimeUI() { closeRuntimeUI(); }

private:

    /** Update 'Devices' : 'Optical/Floppy Devices' menu routine. */
    void updateMenuDevicesStorage(QMenu *pMenu);
    /** Update 'Devices' : 'Network' menu routine. */
    void updateMenuDevicesNetwork(QMenu *pMenu);
    /** Update 'Devices' : 'USB Devices' menu routine. */
    void updateMenuDevicesUSB(QMenu *pMenu);
    /** Update 'Devices' : 'Web Cams' menu routine. */
    void updateMenuDevicesWebCams(QMenu *pMenu);
    /** Update 'Devices' : 'Shared Clipboard' menu routine. */
    void updateMenuDevicesSharedClipboard(QMenu *pMenu);
    /** Update 'Devices' : 'Drag and Drop' menu routine. */
    void updateMenuDevicesDragAndDrop(QMenu *pMenu);
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Update 'Debug' menu routine. */
    void updateMenuDebug(QMenu *pMenu);
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    /** Update 'Window' menu routine. */
    void updateMenuWindow(QMenu *pMenu);
#endif /* VBOX_WS_MAC */

    /** Show Global Preferences on the page defined by @a strCategory and tab defined by @a strControl. */
    void showGlobalPreferences(const QString &strCategory = QString(), const QString &strControl = QString());

    /** Asks user for the disks encryption passwords. */
    void askUserForTheDiskEncryptionPasswords();

    /* Helpers: */
    static int searchMaxSnapshotIndex(const CMachine &machine, const CSnapshot &snapshot, const QString &strNameTemplate);
    void takeScreenshot(const QString &strFile, const QString &strFormat /* = "png" */) const;

    /* Private variables: */
    UISession *m_pSession;
    UIVisualStateType m_visualStateType;
    UIKeyboardHandler *m_pKeyboardHandler;
    UIMouseHandler *m_pMouseHandler;
    QList<UIMachineWindow*> m_machineWindowsList;

    QActionGroup *m_pRunningActions;
    QActionGroup *m_pRunningOrPausedActions;
    QActionGroup *m_pRunningOrPausedOrStackedActions;
    QActionGroup *m_pSharedClipboardActions;
    QActionGroup *m_pDragAndDropActions;

    /** Holds the map of menu update-handlers. */
    QMap<int, MenuUpdateHandler> m_menuUpdateHandlers;

    bool m_fIsWindowsCreated : 1;

    /** Holds whether VM is in 'manual-override' mode
      * which means there will be no automatic UI shutdowns,
      * visual representation mode changes and other similar routines. */
    bool m_fIsManualOverride : 1;

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* Debugger functionality: */
    bool dbgCreated();
    void dbgDestroy();
    void dbgAdjustRelativePos();
    /* The handle to the debugger GUI: */
    PDBGGUI m_pDbgGui;
    /* The virtual method table for the debugger GUI: */
    PCDBGGUIVT m_pDbgGuiVT;
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
    bool m_fIsDockIconEnabled;
    UIDockIconPreview *m_pDockIconPreview;
    QActionGroup *m_pDockPreviewSelectMonitorGroup;
    int m_DockIconPreviewMonitor;
#endif /* VBOX_WS_MAC */

    void *m_pHostLedsState;

    /** Holds whether VM should perform HID LEDs synchronization. */
    bool m_fIsHidLedsSyncEnabled;

    /* Friend classes: */
    friend class UIMachineWindow;
};

#endif /* !___UIMachineLogic_h___ */


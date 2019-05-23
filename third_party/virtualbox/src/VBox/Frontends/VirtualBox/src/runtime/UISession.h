/* $Id: UISession.h $ */
/** @file
 * VBox Qt GUI - UISession class declaration.
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

#ifndef ___UISession_h___
#define ___UISession_h___

/* Qt includes: */
#include <QObject>
#include <QCursor>
#include <QEvent>
#include <QMap>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UIMediumDefs.h"

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"
#include "CMachine.h"
#include "CConsole.h"
#include "CDisplay.h"
#include "CGuest.h"
#include "CMouse.h"
#include "CKeyboard.h"
#include "CMachineDebugger.h"

/* Forward declarations: */
class QMenu;
class UIFrameBuffer;
class UIMachine;
class UIMachineLogic;
class UIActionPool;
class CUSBDevice;
class CNetworkAdapter;
class CMediumAttachment;
#ifdef VBOX_WS_MAC
class QMenuBar;
#else /* !VBOX_WS_MAC */
class QIcon;
#endif /* !VBOX_WS_MAC */

/* CConsole callback event types: */
enum UIConsoleEventType
{
    UIConsoleEventType_MousePointerShapeChange = QEvent::User + 1,
    UIConsoleEventType_MouseCapabilityChange,
    UIConsoleEventType_KeyboardLedsChange,
    UIConsoleEventType_StateChange,
    UIConsoleEventType_AdditionsStateChange,
    UIConsoleEventType_NetworkAdapterChange,
    /* Not used: UIConsoleEventType_SerialPortChange, */
    /* Not used: UIConsoleEventType_ParallelPortChange, */
    /* Not used: UIConsoleEventType_StorageControllerChange, */
    UIConsoleEventType_MediumChange,
    /* Not used: UIConsoleEventType_CPUChange, */
    UIConsoleEventType_VRDEServerChange,
    UIConsoleEventType_VRDEServerInfoChange,
    UIConsoleEventType_USBControllerChange,
    UIConsoleEventType_USBDeviceStateChange,
    UIConsoleEventType_SharedFolderChange,
    UIConsoleEventType_RuntimeError,
    UIConsoleEventType_CanShowWindow,
    UIConsoleEventType_ShowWindow,
    UIConsoleEventType_MAX
};

class UISession : public QObject
{
    Q_OBJECT;

public:

    /** Factory constructor. */
    static bool create(UISession *&pSession, UIMachine *pMachine);
    /** Factory destructor. */
    static void destroy(UISession *&pSession);

    /* API: Runtime UI stuff: */
    bool initialize();
    bool powerUp();
    bool detach();
    bool saveState();
    bool shutdown();
    bool powerOff(bool fIncludingDiscard, bool &fServerCrashed);
    bool restoreCurrentSnapshot();

    /** Returns the session instance. */
    CSession& session() { return m_session; }
    /** Returns the session's machine instance. */
    CMachine& machine() { return m_machine; }
    /** Returns the session's console instance. */
    CConsole& console() { return m_console; }
    /** Returns the console's display instance. */
    CDisplay& display() { return m_display; }
    /** Returns the console's guest instance. */
    CGuest& guest() { return m_guest; }
    /** Returns the console's mouse instance. */
    CMouse& mouse() { return m_mouse; }
    /** Returns the console's keyboard instance. */
    CKeyboard& keyboard() { return m_keyboard; }
    /** Returns the console's debugger instance. */
    CMachineDebugger& debugger() { return m_debugger; }

    /** Returns the machine name. */
    const QString& machineName() const { return m_strMachineName; }

    UIActionPool* actionPool() const { return m_pActionPool; }
    KMachineState machineStatePrevious() const { return m_machineStatePrevious; }
    KMachineState machineState() const { return m_machineState; }
    UIMachineLogic* machineLogic() const;
    QWidget* mainMachineWindow() const;
    WId mainMachineWindowId() const;
    QCursor cursor() const { return m_cursor; }

    /** @name Branding stuff.
     ** @{ */
    /** Returns the cached machine-window icon. */
    QIcon *machineWindowIcon() const { return m_pMachineWindowIcon; }
#ifndef VBOX_WS_MAC
    /** Returns redefined machine-window name postfix. */
    QString machineWindowNamePostfix() const { return m_strMachineWindowNamePostfix; }
#endif
    /** @} */

    /** @name Host-screen configuration variables.
     ** @{ */
    /** Returns the list of host-screen geometries we currently have. */
    QList<QRect> hostScreens() const { return m_hostScreens; }
    /** @} */

    /** @name Application Close configuration stuff.
     * @{ */
    /** Defines @a defaultCloseAction. */
    void setDefaultCloseAction(MachineCloseAction defaultCloseAction) { m_defaultCloseAction = defaultCloseAction; }
    /** Returns default close action. */
    MachineCloseAction defaultCloseAction() const { return m_defaultCloseAction; }
    /** Returns merged restricted close actions. */
    MachineCloseAction restrictedCloseActions() const { return m_restrictedCloseActions; }
    /** Returns whether all the close actions are restricted. */
    bool isAllCloseActionsRestricted() const { return m_fAllCloseActionsRestricted; }
    /** @} */

    /** Returns whether visual @a state is allowed. */
    bool isVisualStateAllowed(UIVisualStateType state) const;
    /** Requests visual-state change. */
    void changeVisualState(UIVisualStateType visualStateType);
    /** Requests visual-state to be entered when possible. */
    void setRequestedVisualState(UIVisualStateType visualStateType) { m_requestedVisualStateType = visualStateType; }
    /** Returns requested visual-state to be entered when possible. */
    UIVisualStateType requestedVisualState() const { return m_requestedVisualStateType; }

    bool isSaved() const { return machineState() == KMachineState_Saved; }
    bool isTurnedOff() const { return machineState() == KMachineState_PoweredOff ||
                                      machineState() == KMachineState_Saved ||
                                      machineState() == KMachineState_Teleported ||
                                      machineState() == KMachineState_Aborted; }
    bool isPaused() const { return machineState() == KMachineState_Paused ||
                                   machineState() == KMachineState_TeleportingPausedVM; }
    bool isRunning() const { return machineState() == KMachineState_Running ||
                                    machineState() == KMachineState_Teleporting ||
                                    machineState() == KMachineState_LiveSnapshotting; }
    bool isStuck() const { return machineState() == KMachineState_Stuck; }
    bool wasPaused() const { return machineStatePrevious() == KMachineState_Paused ||
                                    machineStatePrevious() == KMachineState_TeleportingPausedVM; }
    bool isInitialized() const { return m_fInitialized; }
    bool isFirstTimeStarted() const { return m_fIsFirstTimeStarted; }
    bool isGuestResizeIgnored() const { return m_fIsGuestResizeIgnored; }
    bool isAutoCaptureDisabled() const { return m_fIsAutoCaptureDisabled; }

    /* Guest additions state getters: */
    bool isGuestAdditionsActive() const { return (m_ulGuestAdditionsRunLevel > KAdditionsRunLevelType_None); }
    bool isGuestSupportsGraphics() const { return m_fIsGuestSupportsGraphics; }
    /* The double check below is correct, even though it is an implementation
     * detail of the Additions which the GUI should not ideally have to know. */
    bool isGuestSupportsSeamless() const { return isGuestSupportsGraphics() && m_fIsGuestSupportsSeamless; }

    /* Keyboard getters: */
    /** Returns keyboard-state. */
    int keyboardState() const { return m_iKeyboardState; }
    bool isNumLock() const { return m_fNumLock; }
    bool isCapsLock() const { return m_fCapsLock; }
    bool isScrollLock() const { return m_fScrollLock; }
    uint numLockAdaptionCnt() const { return m_uNumLockAdaptionCnt; }
    uint capsLockAdaptionCnt() const { return m_uCapsLockAdaptionCnt; }

    /* Mouse getters: */
    /** Returns mouse-state. */
    int mouseState() const { return m_iMouseState; }
    bool isMouseSupportsAbsolute() const { return m_fIsMouseSupportsAbsolute; }
    bool isMouseSupportsRelative() const { return m_fIsMouseSupportsRelative; }
    bool isMouseSupportsMultiTouch() const { return m_fIsMouseSupportsMultiTouch; }
    bool isMouseHostCursorNeeded() const { return m_fIsMouseHostCursorNeeded; }
    bool isMouseCaptured() const { return m_fIsMouseCaptured; }
    bool isMouseIntegrated() const { return m_fIsMouseIntegrated; }
    bool isValidPointerShapePresent() const { return m_fIsValidPointerShapePresent; }
    bool isHidingHostPointer() const { return m_fIsHidingHostPointer; }

    /* Common setters: */
    bool pause() { return setPause(true); }
    bool unpause() { return setPause(false); }
    bool setPause(bool fOn);
    void setGuestResizeIgnored(bool fIsGuestResizeIgnored) { m_fIsGuestResizeIgnored = fIsGuestResizeIgnored; }
    void setAutoCaptureDisabled(bool fIsAutoCaptureDisabled) { m_fIsAutoCaptureDisabled = fIsAutoCaptureDisabled; }
    void forgetPreviousMachineState() { m_machineStatePrevious = m_machineState; }

    /* Keyboard setters: */
    void setNumLockAdaptionCnt(uint uNumLockAdaptionCnt) { m_uNumLockAdaptionCnt = uNumLockAdaptionCnt; }
    void setCapsLockAdaptionCnt(uint uCapsLockAdaptionCnt) { m_uCapsLockAdaptionCnt = uCapsLockAdaptionCnt; }

    /* Mouse setters: */
    void setMouseCaptured(bool fIsMouseCaptured) { m_fIsMouseCaptured = fIsMouseCaptured; }
    void setMouseIntegrated(bool fIsMouseIntegrated) { m_fIsMouseIntegrated = fIsMouseIntegrated; }

    /* Screen visibility status for host-desires: */
    bool isScreenVisibleHostDesires(ulong uScreenId) const;
    void setScreenVisibleHostDesires(ulong uScreenId, bool fIsMonitorVisible);

    /* Screen visibility status: */
    bool isScreenVisible(ulong uScreenId) const;
    void setScreenVisible(ulong uScreenId, bool fIsMonitorVisible);

    /* Last screen full-screen size: */
    QSize lastFullScreenSize(ulong uScreenId) const;
    void setLastFullScreenSize(ulong uScreenId, QSize size);

    /** Returns whether guest-screen is undrawable.
     *  @todo: extend this method to all the states when guest-screen is undrawable. */
    bool isGuestScreenUnDrawable() const { return machineState() == KMachineState_Stopping ||
                                                  machineState() == KMachineState_Saving; }

    /* Returns existing framebuffer for the given screen-number;
     * Returns 0 (asserts) if screen-number attribute is out of bounds: */
    UIFrameBuffer* frameBuffer(ulong uScreenId) const;
    /* Sets framebuffer for the given screen-number;
     * Ignores (asserts) if screen-number attribute is out of bounds: */
    void setFrameBuffer(ulong uScreenId, UIFrameBuffer* pFrameBuffer);
    /** Returns existing frame-buffer vector. */
    const QVector<UIFrameBuffer*>& frameBuffers() const { return m_frameBufferVector; }

    /** Updates VRDE Server action state. */
    void updateStatusVRDE() { sltVRDEChange(); }
    /** Updates Video Capture action state. */
    void updateStatusVideoCapture() { sltVideoCaptureChange(); }
    /** Updates Audio output action state. */
    void updateAudioOutput() { sltAudioAdapterChange(); }
    /** Updates Audio input action state. */
    void updateAudioInput() { sltAudioAdapterChange(); }

    /** @name CPU hardware virtualization features for VM.
     ** @{ */
    /** Returns whether CPU hardware virtualization extension is enabled. */
    bool isHWVirtExEnabled() const { return m_fIsHWVirtExEnabled; }
    /** Returns whether nested-paging CPU hardware virtualization extension is enabled. */
    bool isHWVirtExNestedPagingEnabled() const { return m_fIsHWVirtExNestedPagingEnabled; }
    /** Returns whether the VM is currently making use of the unrestricted execution feature of VT-x. */
    bool isHWVirtExUXEnabled() const { return m_fIsHWVirtExUXEnabled; }
    /** @} */

    /** Returns VM's effective paravirtualization provider. */
    KParavirtProvider paraVirtProvider() const { return m_paraVirtProvider; }

signals:

    /** Notifies about frame-buffer resize. */
    void sigFrameBufferResize();

    /* Console callback signals: */
    /** Notifies listeners about keyboard state-change. */
    void sigKeyboardStateChange(int iState);
    /** Notifies listeners about mouse state-change. */
    void sigMouseStateChange(int iState);
    void sigMousePointerShapeChange();
    void sigMouseCapabilityChange();
    void sigKeyboardLedsChange();
    void sigMachineStateChange();
    void sigAdditionsStateChange();
    void sigAdditionsStateActualChange();
    void sigNetworkAdapterChange(const CNetworkAdapter &networkAdapter);
    /** Notifies about storage device change for @a attachment, which was @a fRemoved and it was @a fSilent for guest. */
    void sigStorageDeviceChange(const CMediumAttachment &attachment, bool fRemoved, bool fSilent);
    void sigMediumChange(const CMediumAttachment &mediumAttachment);
    void sigVRDEChange();
    void sigVideoCaptureChange();
    void sigUSBControllerChange();
    void sigUSBDeviceStateChange(const CUSBDevice &device, bool bIsAttached, const CVirtualBoxErrorInfo &error);
    void sigSharedFolderChange();
    void sigRuntimeError(bool bIsFatal, const QString &strErrorId, const QString &strMessage);
#ifdef RT_OS_DARWIN
    void sigShowWindows();
#endif /* RT_OS_DARWIN */
    void sigCPUExecutionCapChange();
    void sigGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);
    void sigAudioAdapterChange();

    /** Notifies about host-screen count change. */
    void sigHostScreenCountChange();
    /** Notifies about host-screen geometry change. */
    void sigHostScreenGeometryChange();
    /** Notifies about host-screen available-area change. */
    void sigHostScreenAvailableAreaChange();

    /* Session signals: */
    void sigInitialized();

public slots:

    void sltInstallGuestAdditionsFrom(const QString &strSource);

    /** Defines @a iKeyboardState. */
    void setKeyboardState(int iKeyboardState) { m_iKeyboardState = iKeyboardState; emit sigKeyboardStateChange(m_iKeyboardState); }

    /** Defines @a iMouseState. */
    void setMouseState(int iMouseState) { m_iMouseState = iMouseState; emit sigMouseStateChange(m_iMouseState); }

private slots:

    /** Marks machine started. */
    void sltMarkInitialized() { m_fInitialized = true; }

    /** Close Runtime UI. */
    void sltCloseRuntimeUI();

#ifdef RT_OS_DARWIN
    /** Mac OS X: Handles menu-bar configuration-change. */
    void sltHandleMenuBarConfigurationChange(const QString &strMachineID);
#endif /* RT_OS_DARWIN */

    /* Console events slots */
    void sltMousePointerShapeChange(bool fVisible, bool fAlpha, QPoint hotCorner, QSize size, QVector<uint8_t> shape);
    void sltMouseCapabilityChange(bool fSupportsAbsolute, bool fSupportsRelative, bool fSupportsMultiTouch, bool fNeedsHostCursor);
    void sltKeyboardLedsChangeEvent(bool fNumLock, bool fCapsLock, bool fScrollLock);
    void sltStateChange(KMachineState state);
    void sltAdditionsChange();
    void sltVRDEChange();
    void sltVideoCaptureChange();
    void sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);
    /** Handles storage device change for @a attachment, which was @a fRemoved and it was @a fSilent for guest. */
    void sltHandleStorageDeviceChange(const CMediumAttachment &attachment, bool fRemoved, bool fSilent);
    /** Handles audio adapter change. */
    void sltAudioAdapterChange();

    /* Handlers: Display reconfiguration stuff: */
#ifdef RT_OS_DARWIN
    void sltHandleHostDisplayAboutToChange();
    void sltCheckIfHostDisplayChanged();
#endif /* RT_OS_DARWIN */

    /** Handles host-screen count change. */
    void sltHandleHostScreenCountChange();
    /** Handles host-screen geometry change. */
    void sltHandleHostScreenGeometryChange();
    /** Handles host-screen available-area change. */
    void sltHandleHostScreenAvailableAreaChange();

private:

    /** Constructor. */
    UISession(UIMachine *pMachine);
    /** Destructor. */
    ~UISession();

    /* Private getters: */
    UIMachine* uimachine() const { return m_pMachine; }

    /* Prepare helpers: */
    bool prepare();
    bool prepareSession();
    void prepareActions();
    void prepareConnections();
    void prepareConsoleEventHandlers();
    void prepareScreens();
    void prepareFramebuffers();
    void loadSessionSettings();

    /* Cleanup helpers: */
    void saveSessionSettings();
    void cleanupFramebuffers();
    //void cleanupScreens() {}
    void cleanupConsoleEventHandlers();
    void cleanupConnections();
    void cleanupActions();
    void cleanupSession();
    void cleanup();

#ifdef VBOX_WS_MAC
    /** Mac OS X: Updates menu-bar content. */
    void updateMenu();
#endif /* VBOX_WS_MAC */

    /* Common helpers: */
    void setPointerShape(const uchar *pShapeData, bool fHasAlpha, uint uXHot, uint uYHot, uint uWidth, uint uHeight);
    bool preprocessInitialization();
    bool mountAdHocImage(KDeviceType enmDeviceType, UIMediumType enmMediumType, const QString &strMediumName);
    bool postprocessInitialization();
    int countOfVisibleWindows();
    /** Loads VM settings. */
    void loadVMSettings();

    /** Update host-screen data. */
    void updateHostScreenData();

    /** Updates action restrictions. */
    void updateActionRestrictions();

    /* Private variables: */
    UIMachine *m_pMachine;

    /** Holds the session instance. */
    CSession m_session;
    /** Holds the session's machine instance. */
    CMachine m_machine;
    /** Holds the session's console instance. */
    CConsole m_console;
    /** Holds the console's display instance. */
    CDisplay m_display;
    /** Holds the console's guest instance. */
    CGuest m_guest;
    /** Holds the console's mouse instance. */
    CMouse m_mouse;
    /** Holds the console's keyboard instance. */
    CKeyboard m_keyboard;
    /** Holds the console's debugger instance. */
    CMachineDebugger m_debugger;

    /** Holds the machine name. */
    QString m_strMachineName;

    /** Holds the action-pool instance. */
    UIActionPool *m_pActionPool;

#ifdef VBOX_WS_MAC
    /** Holds the menu-bar instance. */
    QMenuBar *m_pMenuBar;
#endif /* VBOX_WS_MAC */

    /* Screen visibility vector: */
    QVector<bool> m_monitorVisibilityVector;

    /* Screen visibility vector for host-desires: */
    QVector<bool> m_monitorVisibilityVectorHostDesires;

    /* Screen last full-screen size vector: */
    QVector<QSize> m_monitorLastFullScreenSizeVector;

    /* Frame-buffers vector: */
    QVector<UIFrameBuffer*> m_frameBufferVector;

    /* Common variables: */
    KMachineState m_machineStatePrevious;
    KMachineState m_machineState;
    QCursor m_cursor;

    /** @name Branding variables.
     ** @{ */
    /** Holds the cached machine-window icon. */
    QIcon *m_pMachineWindowIcon;
#ifndef VBOX_WS_MAC
    /** Holds redefined machine-window name postfix. */
    QString m_strMachineWindowNamePostfix;
#endif
    /** @} */

    /** @name Visual-state configuration variables.
     ** @{ */
    /** Determines which visual-state should be entered when possible. */
    UIVisualStateType m_requestedVisualStateType;
    /** @} */

#if defined(VBOX_WS_WIN)
    HCURSOR m_alphaCursor;
#endif

    /** @name Host-screen configuration variables.
     * @{ */
    /** Holds the list of host-screen geometries we currently have. */
    QList<QRect> m_hostScreens;
#ifdef VBOX_WS_MAC
    /** Mac OS X: Watchdog timer looking for display reconfiguration. */
    QTimer *m_pWatchdogDisplayChange;
#endif /* VBOX_WS_MAC */
    /** @} */

    /** @name Application Close configuration variables.
     * @{ */
    /** Default close action. */
    MachineCloseAction m_defaultCloseAction;
    /** Merged restricted close actions. */
    MachineCloseAction m_restrictedCloseActions;
    /** Determines whether all the close actions are restricted. */
    bool m_fAllCloseActionsRestricted;
    /** @} */

    /* Common flags: */
    bool m_fInitialized : 1;
    bool m_fIsFirstTimeStarted : 1;
    bool m_fIsGuestResizeIgnored : 1;
    bool m_fIsAutoCaptureDisabled : 1;

    /* Guest additions flags: */
    ULONG m_ulGuestAdditionsRunLevel;
    bool  m_fIsGuestSupportsGraphics : 1;
    bool  m_fIsGuestSupportsSeamless : 1;

    /* Keyboard flags: */
    /** Holds the keyboard-state. */
    int m_iKeyboardState;
    bool m_fNumLock : 1;
    bool m_fCapsLock : 1;
    bool m_fScrollLock : 1;
    uint m_uNumLockAdaptionCnt;
    uint m_uCapsLockAdaptionCnt;

    /* Mouse flags: */
    /** Holds the mouse-state. */
    int m_iMouseState;
    bool m_fIsMouseSupportsAbsolute : 1;
    bool m_fIsMouseSupportsRelative : 1;
    bool m_fIsMouseSupportsMultiTouch: 1;
    bool m_fIsMouseHostCursorNeeded : 1;
    bool m_fIsMouseCaptured : 1;
    bool m_fIsMouseIntegrated : 1;
    bool m_fIsValidPointerShapePresent : 1;
    bool m_fIsHidingHostPointer : 1;

    /** @name CPU hardware virtualization features for VM.
     ** @{ */
    /** Holds whether CPU hardware virtualization extension is enabled. */
    bool m_fIsHWVirtExEnabled;
    /** Holds whether nested-paging CPU hardware virtualization extension is enabled. */
    bool m_fIsHWVirtExNestedPagingEnabled;
    /** Holds whether the VM is currently making use of the unrestricted execution feature of VT-x. */
    bool m_fIsHWVirtExUXEnabled;
    /** @} */

    /** Holds VM's effective paravirtualization provider. */
    KParavirtProvider m_paraVirtProvider;
};

#endif /* !___UISession_h___ */


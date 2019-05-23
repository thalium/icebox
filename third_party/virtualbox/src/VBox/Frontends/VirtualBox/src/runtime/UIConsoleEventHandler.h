/* $Id: UIConsoleEventHandler.h $ */
/** @file
 * VBox Qt GUI - UIConsoleEventHandler class declaration.
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

#ifndef ___UIConsoleEventHandler_h___
#define ___UIConsoleEventHandler_h___

/* Qt includes: */
#include <QObject>

/* COM includes: */
#include "COMEnums.h"
#include "CVirtualBoxErrorInfo.h"
#include "CMediumAttachment.h"
#include "CNetworkAdapter.h"
#include "CUSBDevice.h"

/* Forward declarations: */
class UIConsoleEventHandlerProxy;
class UISession;


/** Singleton QObject extension
  * providing GUI with the CConsole event-source. */
class UIConsoleEventHandler : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies about mouse pointer become @a fVisible and his shape changed to @a fAlpha, @a hotCorner, @a size and @a shape. */
    void sigMousePointerShapeChange(bool fVisible, bool fAlpha, QPoint hotCorner, QSize size, QVector<uint8_t> shape);
    /** Notifies about mouse capability change to @a fSupportsAbsolute, @a fSupportsRelative, @a fSupportsMultiTouch and @a fNeedsHostCursor. */
    void sigMouseCapabilityChange(bool fSupportsAbsolute, bool fSupportsRelative, bool fSupportsMultiTouch, bool fNeedsHostCursor);
    /** Notifies about keyboard LEDs change for @a fNumLock, @a fCapsLock and @a fScrollLock. */
    void sigKeyboardLedsChangeEvent(bool fNumLock, bool fCapsLock, bool fScrollLock);
    /** Notifies about machine @a state change. */
    void sigStateChange(KMachineState state);
    /** Notifies about guest additions state change. */
    void sigAdditionsChange();
    /** Notifies about network @a adapter state change. */
    void sigNetworkAdapterChange(CNetworkAdapter adapter);
    /** Notifies about storage device change for @a attachment, which was @a fRemoved and it was @a fSilent for guest. */
    void sigStorageDeviceChange(CMediumAttachment attachment, bool fRemoved, bool fSilent);
    /** Notifies about storage medium @a attachment state change. */
    void sigMediumChange(CMediumAttachment attachment);
    /** Notifies about VRDE device state change. */
    void sigVRDEChange();
    /** Notifies about Video Capture device state change. */
    void sigVideoCaptureChange();
    /** Notifies about USB controller state change. */
    void sigUSBControllerChange();
    /** Notifies about USB @a device state change to @a fAttached, holding additional @a error information. */
    void sigUSBDeviceStateChange(CUSBDevice device, bool fAttached, CVirtualBoxErrorInfo error);
    /** Notifies about shared folder state change. */
    void sigSharedFolderChange();
    /** Notifies about CPU execution-cap change. */
    void sigCPUExecutionCapChange();
    /** Notifies about guest-screen configuration change of @a type for @a uScreenId with @a screenGeo. */
    void sigGuestMonitorChange(KGuestMonitorChangedEventType type, ulong uScreenId, QRect screenGeo);
    /** Notifies about Runtime error with @a strErrorId which is @a fFatal and have @a strMessage. */
    void sigRuntimeError(bool fFatal, QString strErrorId, QString strMessage);
#ifdef RT_OS_DARWIN
    /** Notifies about VM window should be shown. */
    void sigShowWindow();
#endif /* RT_OS_DARWIN */
    /** Notifies about audio adapter state change. */
    void sigAudioAdapterChange();

public:

    /** Returns singleton instance created by the factory. */
    static UIConsoleEventHandler* instance() { return m_spInstance; }
    /** Creates singleton instance created by the factory. */
    static void create(UISession *pSession);
    /** Destroys singleton instance created by the factory. */
    static void destroy();

protected:

    /** Constructs console event handler for passed @a pSession. */
    UIConsoleEventHandler(UISession *pSession);

    /** @name Prepare cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares connections. */
        void prepareConnections();
    /** @} */

private:

    /** Holds the singleton static console event handler instance. */
    static UIConsoleEventHandler *m_spInstance;

    /** Holds the console event proxy instance. */
    UIConsoleEventHandlerProxy *m_pProxy;
};

/** Defines the globally known name for the console event handler instance. */
#define gConsoleEvents UIConsoleEventHandler::instance()

#endif /* !___UIConsoleEventHandler_h___ */


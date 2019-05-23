/* $Id: UIMainEventListener.h $ */
/** @file
 * VBox Qt GUI - UIMainEventListener class declaration.
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

#ifndef ___UIMainEventListener_h___
#define ___UIMainEventListener_h___

/* Qt includes: */
#include <QObject>
#include <QList>

/* COM includes: */
#include "COMEnums.h"
#include "CVirtualBoxErrorInfo.h"
#include "CMediumAttachment.h"
#include "CNetworkAdapter.h"
#include "CUSBDevice.h"

/* Other VBox includes: */
#include <VBox/com/listeners.h>

/* Forward declarations: */
class UIMainEventListeningThread;
class CEventListener;
class CEventSource;


/* Note: On a first look this may seems a little bit complicated.
 * There are two reasons to use a separate class here which handles the events
 * and forward them to the public class as signals. The first one is that on
 * some platforms (e.g. Win32) this events not arrive in the main GUI thread.
 * So there we have to make sure they are first delivered to the main GUI
 * thread and later executed there. The second reason is, that the initiator
 * method may hold a lock on a object which has to be manipulated in the event
 * consumer. Doing this without being asynchronous would lead to a dead lock. To
 * avoid both problems we send signals as a queued connection to the event
 * consumer. Qt will create a event for us, place it in the main GUI event
 * queue and deliver it later on. */

/** Main event listener. */
class UIMainEventListener: public QObject
{
    Q_OBJECT;

signals:

    /** Notifies about the VBoxSVC become @a fAvailable. */
    void sigVBoxSVCAvailabilityChange(bool fAvailable);

    /** Notifies about @a state change event for the machine with @a strId. */
    void sigMachineStateChange(QString strId, KMachineState state);
    /** Notifies about data change event for the machine with @a strId. */
    void sigMachineDataChange(QString strId);
    /** Notifies about machine with @a strId was @a fRegistered. */
    void sigMachineRegistered(QString strId, bool fRegistered);
    /** Notifies about @a state change event for the session of the machine with @a strId. */
    void sigSessionStateChange(QString strId, KSessionState state);
    /** Notifies about snapshot with @a strSnapshotId was taken for the machine with @a strId. */
    void sigSnapshotTake(QString strId, QString strSnapshotId);
    /** Notifies about snapshot with @a strSnapshotId was deleted for the machine with @a strId. */
    void sigSnapshotDelete(QString strId, QString strSnapshotId);
    /** Notifies about snapshot with @a strSnapshotId was changed for the machine with @a strId. */
    void sigSnapshotChange(QString strId, QString strSnapshotId);
    /** Notifies about snapshot with @a strSnapshotId was restored for the machine with @a strId. */
    void sigSnapshotRestore(QString strId, QString strSnapshotId);

    /** Notifies about extra-data of the machine with @a strId can be changed for the key @a strKey to value @a strValue. */
    void sigExtraDataCanChange(QString strId, QString strKey, QString strValue, bool &fVeto, QString &strVetoReason); /* use Qt::DirectConnection */
    /** Notifies about extra-data of the machine with @a strId changed for the key @a strKey to value @a strValue. */
    void sigExtraDataChange(QString strId, QString strKey, QString strValue);

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
    void sigGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);
    /** Notifies about Runtime error with @a strErrorId which is @a fFatal and have @a strMessage. */
    void sigRuntimeError(bool fFatal, QString strErrorId, QString strMessage);
    /** Notifies about VM window can be shown, allowing to prevent it by @a fVeto with @a strReason. */
    void sigCanShowWindow(bool &fVeto, QString &strReason); /* use Qt::DirectConnection */
    /** Notifies about VM window with specified @a winId should be shown. */
    void sigShowWindow(qint64 &winId); /* use Qt::DirectConnection */
    /** Notifies about audio adapter state change. */
    void sigAudioAdapterChange();

    /** Notifies about @a iPercent change for progress with @a strProgressId. */
    void sigProgressPercentageChange(QString strProgressId, int iPercent);
    /** Notifies about task complete for progress with @a strProgressId. */
    void sigProgressTaskComplete(QString strProgressId);

public:

    /** Constructs main event listener. */
    UIMainEventListener();

    /** Initialization routine. */
    HRESULT init(QObject *pParent) { Q_UNUSED(pParent); return S_OK; }
    /** Deinitialization routine. */
    void uninit() {}

    /** Registers event @a source for passive event @a listener. */
    void registerSource(const CEventSource &source, const CEventListener &listener);
    /** Unregisters event sources. */
    void unregisterSources();

    /** Main event handler routine. */
    STDMETHOD(HandleEvent)(VBoxEventType_T enmType, IEvent *pEvent);

    /** Holds the list of threads handling passive event listening. */
    QList<UIMainEventListeningThread*> m_threads;
};

/* Wrap the IListener interface around our implementation class. */
typedef ListenerImpl<UIMainEventListener, QObject*> UIMainEventListenerImpl;

#endif /* !___UIMainEventListener_h___ */


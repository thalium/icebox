/* $Id: UIConsoleEventHandler.cpp $ */
/** @file
 * VBox Qt GUI - UIConsoleEventHandler class implementation.
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
# include "UIConsoleEventHandler.h"
# include "UIMainEventListener.h"
# include "UIExtraDataManager.h"
# include "VBoxGlobal.h"
# include "UISession.h"
# ifdef VBOX_WS_MAC
#  include "VBoxUtils.h"
# endif /* VBOX_WS_MAC */

/* COM includes: */
# include "CEventListener.h"
# include "CEventSource.h"
# include "CConsole.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Private QObject extension
  * providing UIConsoleEventHandler with the CConsole event-source. */
class UIConsoleEventHandlerProxy : public QObject
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

    /** Constructs event proxy object on the basis of passed @a pParent and @a pSession. */
    UIConsoleEventHandlerProxy(QObject *pParent, UISession *pSession);
    /** Destructs event proxy object. */
    ~UIConsoleEventHandlerProxy();

protected:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares listener. */
        void prepareListener();
        /** Prepares connections. */
        void prepareConnections();

        /** Cleanups connections. */
        void cleanupConnections();
        /** Cleanups listener. */
        void cleanupListener();
        /** Cleanups all. */
        void cleanup();
    /** @} */

private slots:

    /** @name Slots for waitable signals.
      * @{ */
        /** Returns whether VM window can be shown. */
        void sltCanShowWindow(bool &fVeto, QString &strReason);
        /** Shows VM window if possible. */
        void sltShowWindow(qint64 &winId);
    /** @} */

private:

    /** Holds the UI session reference. */
    UISession *m_pSession;

    /** Holds the Qt event listener instance. */
    ComObjPtr<UIMainEventListenerImpl> m_pQtListener;
    /** Holds the COM event listener instance. */
    CEventListener m_comEventListener;
};


/*********************************************************************************************************************************
*   Class UIConsoleEventHandlerProxy implementation.                                                                             *
*********************************************************************************************************************************/

UIConsoleEventHandlerProxy::UIConsoleEventHandlerProxy(QObject *pParent, UISession *pSession)
    : QObject(pParent)
    , m_pSession(pSession)
{
    /* Prepare: */
    prepare();
}

UIConsoleEventHandlerProxy::~UIConsoleEventHandlerProxy()
{
    /* Cleanup: */
    cleanup();
}

void UIConsoleEventHandlerProxy::prepare()
{
    /* Prepare: */
    prepareListener();
    prepareConnections();
}

void UIConsoleEventHandlerProxy::prepareListener()
{
    /* Make sure session is passed: */
    AssertPtrReturnVoid(m_pSession);

    /* Create event listener instance: */
    m_pQtListener.createObject();
    m_pQtListener->init(new UIMainEventListener, this);
    m_comEventListener = CEventListener(m_pQtListener);

    /* Get console: */
    const CConsole comConsole = m_pSession->session().GetConsole();
    AssertReturnVoid(!comConsole.isNull() && comConsole.isOk());
    /* Get console event source: */
    CEventSource comEventSourceConsole = comConsole.GetEventSource();
    AssertReturnVoid(!comEventSourceConsole.isNull() && comEventSourceConsole.isOk());

    /* Enumerate all the required event-types: */
    QVector<KVBoxEventType> eventTypes;
    eventTypes
        << KVBoxEventType_OnMousePointerShapeChanged
        << KVBoxEventType_OnMouseCapabilityChanged
        << KVBoxEventType_OnKeyboardLedsChanged
        << KVBoxEventType_OnStateChanged
        << KVBoxEventType_OnAdditionsStateChanged
        << KVBoxEventType_OnNetworkAdapterChanged
        << KVBoxEventType_OnStorageDeviceChanged
        << KVBoxEventType_OnMediumChanged
        << KVBoxEventType_OnVRDEServerChanged
        << KVBoxEventType_OnVRDEServerInfoChanged
        << KVBoxEventType_OnVideoCaptureChanged
        << KVBoxEventType_OnUSBControllerChanged
        << KVBoxEventType_OnUSBDeviceStateChanged
        << KVBoxEventType_OnSharedFolderChanged
        << KVBoxEventType_OnCPUExecutionCapChanged
        << KVBoxEventType_OnGuestMonitorChanged
        << KVBoxEventType_OnRuntimeError
        << KVBoxEventType_OnCanShowWindow
        << KVBoxEventType_OnShowWindow
        << KVBoxEventType_OnAudioAdapterChanged;

    /* Register event listener for console event source: */
    comEventSourceConsole.RegisterListener(m_comEventListener, eventTypes,
        gEDataManager->eventHandlingType() == EventHandlingType_Active ? TRUE : FALSE);
    AssertWrapperOk(comEventSourceConsole);

    /* If event listener registered as passive one: */
    if (gEDataManager->eventHandlingType() == EventHandlingType_Passive)
    {
        /* Register event sources in their listeners as well: */
        m_pQtListener->getWrapped()->registerSource(comEventSourceConsole, m_comEventListener);
    }
}

void UIConsoleEventHandlerProxy::prepareConnections()
{
    /* Create direct (sync) connections for signals of main listener: */
    connect(m_pQtListener->getWrapped(), SIGNAL(sigMousePointerShapeChange(bool, bool, QPoint, QSize, QVector<uint8_t>)),
            this, SIGNAL(sigMousePointerShapeChange(bool, bool, QPoint, QSize, QVector<uint8_t>)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigMouseCapabilityChange(bool, bool, bool, bool)),
            this, SIGNAL(sigMouseCapabilityChange(bool, bool, bool, bool)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigKeyboardLedsChangeEvent(bool, bool, bool)),
            this, SIGNAL(sigKeyboardLedsChangeEvent(bool, bool, bool)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigStateChange(KMachineState)),
            this, SIGNAL(sigStateChange(KMachineState)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigAdditionsChange()),
            this, SIGNAL(sigAdditionsChange()),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigNetworkAdapterChange(CNetworkAdapter)),
            this, SIGNAL(sigNetworkAdapterChange(CNetworkAdapter)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigStorageDeviceChange(CMediumAttachment, bool, bool)),
            this, SIGNAL(sigStorageDeviceChange(CMediumAttachment, bool, bool)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigMediumChange(CMediumAttachment)),
            this, SIGNAL(sigMediumChange(CMediumAttachment)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigVRDEChange()),
            this, SIGNAL(sigVRDEChange()),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigVideoCaptureChange()),
            this, SIGNAL(sigVideoCaptureChange()),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigUSBControllerChange()),
            this, SIGNAL(sigUSBControllerChange()),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigUSBDeviceStateChange(CUSBDevice, bool, CVirtualBoxErrorInfo)),
            this, SIGNAL(sigUSBDeviceStateChange(CUSBDevice, bool, CVirtualBoxErrorInfo)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigSharedFolderChange()),
            this, SIGNAL(sigSharedFolderChange()),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigCPUExecutionCapChange()),
            this, SIGNAL(sigCPUExecutionCapChange()),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigGuestMonitorChange(KGuestMonitorChangedEventType, ulong, QRect)),
            this, SIGNAL(sigGuestMonitorChange(KGuestMonitorChangedEventType, ulong, QRect)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigRuntimeError(bool, QString, QString)),
            this, SIGNAL(sigRuntimeError(bool, QString, QString)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigCanShowWindow(bool &, QString &)),
            this, SLOT(sltCanShowWindow(bool &, QString &)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigShowWindow(qint64 &)),
            this, SLOT(sltShowWindow(qint64 &)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigAudioAdapterChange()),
            this, SIGNAL(sigAudioAdapterChange()),
            Qt::DirectConnection);
}

void UIConsoleEventHandlerProxy::cleanupConnections()
{
    /* Nothing for now. */
}

void UIConsoleEventHandlerProxy::cleanupListener()
{
    /* Make sure session is passed: */
    AssertPtrReturnVoid(m_pSession);

    /* If event listener registered as passive one: */
    if (gEDataManager->eventHandlingType() == EventHandlingType_Passive)
    {
        /* Unregister everything: */
        m_pQtListener->getWrapped()->unregisterSources();
    }

    /* Get console: */
    const CConsole comConsole = m_pSession->session().GetConsole();
    if (comConsole.isNull() || !comConsole.isOk())
        return;
    /* Get console event source: */
    CEventSource comEventSourceConsole = comConsole.GetEventSource();
    AssertWrapperOk(comEventSourceConsole);

    /* Unregister event listener for console event source: */
    comEventSourceConsole.UnregisterListener(m_comEventListener);
}

void UIConsoleEventHandlerProxy::cleanup()
{
    /* Cleanup: */
    cleanupConnections();
    cleanupListener();
}

void UIConsoleEventHandlerProxy::sltCanShowWindow(bool & /* fVeto */, QString & /* strReason */)
{
    /* Nothing for now. */
}

void UIConsoleEventHandlerProxy::sltShowWindow(qint64 &winId)
{
#ifdef VBOX_WS_MAC
    /* First of all, just ask the GUI thread to show the machine-window: */
    winId = 0;
    if (::darwinSetFrontMostProcess())
        emit sigShowWindow();
    else
    {
        /* If it's failed for some reason, send the other process our PSN so it can try: */
        winId = ::darwinGetCurrentProcessId();
    }
#else /* !VBOX_WS_MAC */
    /* Return the ID of the top-level machine-window. */
    winId = (ULONG64)m_pSession->mainMachineWindowId();
#endif /* !VBOX_WS_MAC */
}


/*********************************************************************************************************************************
*   Class UIConsoleEventHandler implementation.                                                                                  *
*********************************************************************************************************************************/

/* static */
UIConsoleEventHandler *UIConsoleEventHandler::m_spInstance = 0;

/* static */
void UIConsoleEventHandler::create(UISession *pSession)
{
    if (!m_spInstance)
        m_spInstance = new UIConsoleEventHandler(pSession);
}

/* static */
void UIConsoleEventHandler::destroy()
{
    if (m_spInstance)
    {
        delete m_spInstance;
        m_spInstance = 0;
    }
}

UIConsoleEventHandler::UIConsoleEventHandler(UISession *pSession)
    : m_pProxy(new UIConsoleEventHandlerProxy(this, pSession))
{
    /* Prepare: */
    prepare();
}

void UIConsoleEventHandler::prepare()
{
    /* Prepare: */
    prepareConnections();
}

void UIConsoleEventHandler::prepareConnections()
{
    /* Create queued (async) connections for signals of event proxy object: */
    connect(m_pProxy, SIGNAL(sigMousePointerShapeChange(bool, bool, QPoint, QSize, QVector<uint8_t>)),
            this, SIGNAL(sigMousePointerShapeChange(bool, bool, QPoint, QSize, QVector<uint8_t>)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigMouseCapabilityChange(bool, bool, bool, bool)),
            this, SIGNAL(sigMouseCapabilityChange(bool, bool, bool, bool)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigKeyboardLedsChangeEvent(bool, bool, bool)),
            this, SIGNAL(sigKeyboardLedsChangeEvent(bool, bool, bool)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigStateChange(KMachineState)),
            this, SIGNAL(sigStateChange(KMachineState)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigAdditionsChange()),
            this, SIGNAL(sigAdditionsChange()),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigNetworkAdapterChange(CNetworkAdapter)),
            this, SIGNAL(sigNetworkAdapterChange(CNetworkAdapter)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigStorageDeviceChange(CMediumAttachment, bool, bool)),
            this, SIGNAL(sigStorageDeviceChange(CMediumAttachment, bool, bool)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigMediumChange(CMediumAttachment)),
            this, SIGNAL(sigMediumChange(CMediumAttachment)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigVRDEChange()),
            this, SIGNAL(sigVRDEChange()),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigVideoCaptureChange()),
            this, SIGNAL(sigVideoCaptureChange()),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigUSBControllerChange()),
            this, SIGNAL(sigUSBControllerChange()),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigUSBDeviceStateChange(CUSBDevice, bool, CVirtualBoxErrorInfo)),
            this, SIGNAL(sigUSBDeviceStateChange(CUSBDevice, bool, CVirtualBoxErrorInfo)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigSharedFolderChange()),
            this, SIGNAL(sigSharedFolderChange()),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigCPUExecutionCapChange()),
            this, SIGNAL(sigCPUExecutionCapChange()),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigGuestMonitorChange(KGuestMonitorChangedEventType, ulong, QRect)),
            this, SIGNAL(sigGuestMonitorChange(KGuestMonitorChangedEventType, ulong, QRect)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigRuntimeError(bool, QString, QString)),
            this, SIGNAL(sigRuntimeError(bool, QString, QString)),
            Qt::QueuedConnection);
#ifdef RT_OS_DARWIN
    connect(m_pProxy, SIGNAL(sigShowWindow(qint64 &)),
            this, SIGNAL(sigShowWindow(qint64 &)),
            Qt::QueuedConnection);
#endif /* RT_OS_DARWIN */
    connect(m_pProxy, SIGNAL(sigAudioAdapterChange()),
            this, SIGNAL(sigAudioAdapterChange()),
            Qt::QueuedConnection);
}

#include "UIConsoleEventHandler.moc"


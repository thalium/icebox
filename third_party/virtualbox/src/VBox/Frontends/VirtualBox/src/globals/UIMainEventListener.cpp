/* $Id: UIMainEventListener.cpp $ */
/** @file
 * VBox Qt GUI - UIMainEventListener class implementation.
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
# include <QThread>
# include <QMutex>

/* GUI includes: */
# include "UIMainEventListener.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "COMEnums.h"
# include "CEvent.h"
# include "CEventSource.h"
# include "CEventListener.h"
# include "CVBoxSVCAvailabilityChangedEvent.h"
# include "CVirtualBoxErrorInfo.h"
# include "CMachineStateChangedEvent.h"
# include "CMachineDataChangedEvent.h"
# include "CMachineRegisteredEvent.h"
# include "CSessionStateChangedEvent.h"
# include "CSnapshotTakenEvent.h"
# include "CSnapshotDeletedEvent.h"
# include "CSnapshotChangedEvent.h"
# include "CSnapshotRestoredEvent.h"
# include "CExtraDataCanChangeEvent.h"
# include "CExtraDataChangedEvent.h"
# include "CMousePointerShapeChangedEvent.h"
# include "CMouseCapabilityChangedEvent.h"
# include "CKeyboardLedsChangedEvent.h"
# include "CStateChangedEvent.h"
# include "CNetworkAdapterChangedEvent.h"
# include "CStorageDeviceChangedEvent.h"
# include "CMediumChangedEvent.h"
# include "CUSBDevice.h"
# include "CUSBDeviceStateChangedEvent.h"
# include "CGuestMonitorChangedEvent.h"
# include "CRuntimeErrorEvent.h"
# include "CCanShowWindowEvent.h"
# include "CShowWindowEvent.h"
# include "CProgressPercentageChangedEvent.h"
# include "CProgressTaskCompletedEvent.h"
#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Private QThread extension allowing to listen for Main events in separate thread.
  * This thread listens for a Main events infinitely unless creator calls for #setShutdown. */
class UIMainEventListeningThread : public QThread
{
    Q_OBJECT;

public:

    /** Constructs Main events listener thread redirecting events from @a source to @a listener. */
    UIMainEventListeningThread(const CEventSource &source, const CEventListener &listener);
    /** Destructs Main events listener thread. */
    ~UIMainEventListeningThread();

protected:

    /** Contains the thread excution body. */
    virtual void run() /* override */;

    /** Returns whether the thread asked to shutdown prematurely. */
    bool isShutdown() const;
    /** Defines whether the thread asked to @a fShutdown prematurely. */
    void setShutdown(bool fShutdown);

private:

    /** Holds the Main event source reference. */
    CEventSource m_source;
    /** Holds the Main event listener reference. */
    CEventListener m_listener;

    /** Holds the mutex instance which protects thread access. */
    mutable QMutex m_mutex;
    /** Holds whether the thread asked to shutdown prematurely. */
    bool m_fShutdown;
};


/*********************************************************************************************************************************
*   Class UIMainEventListeningThread implementation.                                                                             *
*********************************************************************************************************************************/

UIMainEventListeningThread::UIMainEventListeningThread(const CEventSource &source, const CEventListener &listener)
    : m_source(source)
    , m_listener(listener)
    , m_fShutdown(false)
{
}

UIMainEventListeningThread::~UIMainEventListeningThread()
{
    /* Make a request to shutdown: */
    setShutdown(true);

    /* And wait 30 seconds for run() to finish: */
    wait(30000);
}

void UIMainEventListeningThread::run()
{
    /* Initialize COM: */
    COMBase::InitializeCOM(false);

    /* Copy source wrapper to this thread: */
    CEventSource source = m_source;
    /* Copy listener wrapper to this thread: */
    CEventListener listener = m_listener;

    /* While we are not in shutdown: */
    while (!isShutdown())
    {
        /* Fetch the event from the queue: */
        CEvent event = source.GetEvent(listener, 500);
        if (!event.isNull())
        {
            /* Process the event and tell the listener: */
            listener.HandleEvent(event);
            if (event.GetWaitable())
                source.EventProcessed(listener, event);
        }
    }

    /* Cleanup COM: */
    COMBase::CleanupCOM();
}

bool UIMainEventListeningThread::isShutdown() const
{
    m_mutex.lock();
    bool fShutdown = m_fShutdown;
    m_mutex.unlock();
    return fShutdown;
}

void UIMainEventListeningThread::setShutdown(bool fShutdown)
{
    m_mutex.lock();
    m_fShutdown = fShutdown;
    m_mutex.unlock();
}


/*********************************************************************************************************************************
*   Class UIMainEventListener implementation.                                                                                    *
*********************************************************************************************************************************/

UIMainEventListener::UIMainEventListener()
{
    /* Register meta-types for required enums. */
    qRegisterMetaType<KMachineState>("KMachineState");
    qRegisterMetaType<KSessionState>("KSessionState");
    qRegisterMetaType< QVector<uint8_t> >("QVector<uint8_t>");
    qRegisterMetaType<CNetworkAdapter>("CNetworkAdapter");
    qRegisterMetaType<CMediumAttachment>("CMediumAttachment");
    qRegisterMetaType<CUSBDevice>("CUSBDevice");
    qRegisterMetaType<CVirtualBoxErrorInfo>("CVirtualBoxErrorInfo");
    qRegisterMetaType<KGuestMonitorChangedEventType>("KGuestMonitorChangedEventType");
}

void UIMainEventListener::registerSource(const CEventSource &source, const CEventListener &listener)
{
    /* Make sure source and listener are valid: */
    AssertReturnVoid(!source.isNull());
    AssertReturnVoid(!listener.isNull());

    /* Create thread for passed source: */
    m_threads << new UIMainEventListeningThread(source, listener);
    /* And start it: */
    m_threads.last()->start();
}

void UIMainEventListener::unregisterSources()
{
    /* Wipe out the threads: */
    qDeleteAll(m_threads);
}

STDMETHODIMP UIMainEventListener::HandleEvent(VBoxEventType_T /* type */, IEvent *pEvent)
{
    /* Try to acquire COM cleanup protection token first: */
    if (!vboxGlobal().comTokenTryLockForRead())
        return S_OK;

    CEvent event(pEvent);
    // printf("Event received: %d\n", event.GetType());
    switch (event.GetType())
    {
        case KVBoxEventType_OnVBoxSVCAvailabilityChanged:
        {
            CVBoxSVCAvailabilityChangedEvent es(pEvent);
            emit sigVBoxSVCAvailabilityChange(es.GetAvailable());
            break;
        }

        case KVBoxEventType_OnMachineStateChanged:
        {
            CMachineStateChangedEvent es(pEvent);
            emit sigMachineStateChange(es.GetMachineId(), es.GetState());
            break;
        }
        case KVBoxEventType_OnMachineDataChanged:
        {
            CMachineDataChangedEvent es(pEvent);
            emit sigMachineDataChange(es.GetMachineId());
            break;
        }
        case KVBoxEventType_OnMachineRegistered:
        {
            CMachineRegisteredEvent es(pEvent);
            emit sigMachineRegistered(es.GetMachineId(), es.GetRegistered());
            break;
        }
        case KVBoxEventType_OnSessionStateChanged:
        {
            CSessionStateChangedEvent es(pEvent);
            emit sigSessionStateChange(es.GetMachineId(), es.GetState());
            break;
        }
        case KVBoxEventType_OnSnapshotTaken:
        {
            CSnapshotTakenEvent es(pEvent);
            emit sigSnapshotTake(es.GetMachineId(), es.GetSnapshotId());
            break;
        }
        case KVBoxEventType_OnSnapshotDeleted:
        {
            CSnapshotDeletedEvent es(pEvent);
            emit sigSnapshotDelete(es.GetMachineId(), es.GetSnapshotId());
            break;
        }
        case KVBoxEventType_OnSnapshotChanged:
        {
            CSnapshotChangedEvent es(pEvent);
            emit sigSnapshotChange(es.GetMachineId(), es.GetSnapshotId());
            break;
        }
        case KVBoxEventType_OnSnapshotRestored:
        {
            CSnapshotRestoredEvent es(pEvent);
            emit sigSnapshotRestore(es.GetMachineId(), es.GetSnapshotId());
            break;
        }
//        case KVBoxEventType_OnMediumRegistered:
//        case KVBoxEventType_OnGuestPropertyChange:

        case KVBoxEventType_OnExtraDataCanChange:
        {
            CExtraDataCanChangeEvent es(pEvent);
            /* Has to be done in place to give an answer: */
            bool fVeto = false;
            QString strReason;
            emit sigExtraDataCanChange(es.GetMachineId(), es.GetKey(), es.GetValue(), fVeto, strReason);
            if (fVeto)
                es.AddVeto(strReason);
            break;
        }
        case KVBoxEventType_OnExtraDataChanged:
        {
            CExtraDataChangedEvent es(pEvent);
            emit sigExtraDataChange(es.GetMachineId(), es.GetKey(), es.GetValue());
            break;
        }

        case KVBoxEventType_OnMousePointerShapeChanged:
        {
            CMousePointerShapeChangedEvent es(pEvent);
            emit sigMousePointerShapeChange(es.GetVisible(), es.GetAlpha(), QPoint(es.GetXhot(), es.GetYhot()), QSize(es.GetWidth(), es.GetHeight()), es.GetShape());
            break;
        }
        case KVBoxEventType_OnMouseCapabilityChanged:
        {
            CMouseCapabilityChangedEvent es(pEvent);
            emit sigMouseCapabilityChange(es.GetSupportsAbsolute(), es.GetSupportsRelative(), es.GetSupportsMultiTouch(), es.GetNeedsHostCursor());
            break;
        }
        case KVBoxEventType_OnKeyboardLedsChanged:
        {
            CKeyboardLedsChangedEvent es(pEvent);
            emit sigKeyboardLedsChangeEvent(es.GetNumLock(), es.GetCapsLock(), es.GetScrollLock());
            break;
        }
        case KVBoxEventType_OnStateChanged:
        {
            CStateChangedEvent es(pEvent);
            emit sigStateChange(es.GetState());
            break;
        }
        case KVBoxEventType_OnAdditionsStateChanged:
        {
            emit sigAdditionsChange();
            break;
        }
        case KVBoxEventType_OnNetworkAdapterChanged:
        {
            CNetworkAdapterChangedEvent es(pEvent);
            emit sigNetworkAdapterChange(es.GetNetworkAdapter());
            break;
        }
        case KVBoxEventType_OnStorageDeviceChanged:
        {
            CStorageDeviceChangedEvent es(pEvent);
            emit sigStorageDeviceChange(es.GetStorageDevice(), es.GetRemoved(), es.GetSilent());
            break;
        }
        case KVBoxEventType_OnMediumChanged:
        {
            CMediumChangedEvent es(pEvent);
            emit sigMediumChange(es.GetMediumAttachment());
            break;
        }
        case KVBoxEventType_OnVRDEServerChanged:
        case KVBoxEventType_OnVRDEServerInfoChanged:
        {
            emit sigVRDEChange();
            break;
        }
        case KVBoxEventType_OnVideoCaptureChanged:
        {
            emit sigVideoCaptureChange();
            break;
        }
        case KVBoxEventType_OnUSBControllerChanged:
        {
            emit sigUSBControllerChange();
            break;
        }
        case KVBoxEventType_OnUSBDeviceStateChanged:
        {
            CUSBDeviceStateChangedEvent es(pEvent);
            emit sigUSBDeviceStateChange(es.GetDevice(), es.GetAttached(), es.GetError());
            break;
        }
        case KVBoxEventType_OnSharedFolderChanged:
        {
            emit sigSharedFolderChange();
            break;
        }
        case KVBoxEventType_OnCPUExecutionCapChanged:
        {
            emit sigCPUExecutionCapChange();
            break;
        }
        case KVBoxEventType_OnGuestMonitorChanged:
        {
            CGuestMonitorChangedEvent es(pEvent);
            emit sigGuestMonitorChange(es.GetChangeType(), es.GetScreenId(),
                                       QRect(es.GetOriginX(), es.GetOriginY(), es.GetWidth(), es.GetHeight()));
            break;
        }
        case KVBoxEventType_OnRuntimeError:
        {
            CRuntimeErrorEvent es(pEvent);
            emit sigRuntimeError(es.GetFatal(), es.GetId(), es.GetMessage());
            break;
        }
        case KVBoxEventType_OnCanShowWindow:
        {
            CCanShowWindowEvent es(pEvent);
            /* Has to be done in place to give an answer: */
            bool fVeto = false;
            QString strReason;
            emit sigCanShowWindow(fVeto, strReason);
            if (fVeto)
                es.AddVeto(strReason);
            else
                es.AddApproval(strReason);
            break;
        }
        case KVBoxEventType_OnShowWindow:
        {
            CShowWindowEvent es(pEvent);
            /* Has to be done in place to give an answer: */
            qint64 winId = es.GetWinId();
            if (winId != 0)
                break; /* Already set by some listener. */
            emit sigShowWindow(winId);
            es.SetWinId(winId);
            break;
        }
        case KVBoxEventType_OnAudioAdapterChanged:
        {
            emit sigAudioAdapterChange();
            break;
        }
        case KVBoxEventType_OnProgressPercentageChanged:
        {
            CProgressPercentageChangedEvent es(pEvent);
            emit sigProgressPercentageChange(es.GetProgressId(), (int)es.GetPercent());
            break;
        }
        case KVBoxEventType_OnProgressTaskCompleted:
        {
            CProgressTaskCompletedEvent es(pEvent);
            emit sigProgressTaskComplete(es.GetProgressId());
            break;
        }

        default: break;
    }

    /* Unlock COM cleanup protection token: */
    vboxGlobal().comTokenUnlock();

    return S_OK;
}

#include "UIMainEventListener.moc"


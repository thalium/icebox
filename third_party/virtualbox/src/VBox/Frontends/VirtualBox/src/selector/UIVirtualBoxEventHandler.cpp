/* $Id: UIVirtualBoxEventHandler.cpp $ */
/** @file
 * VBox Qt GUI - UIVirtualBoxEventHandler class implementation.
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
# include "UIVirtualBoxEventHandler.h"
# include "UIMainEventListener.h"
# include "UIExtraDataManager.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CEventListener.h"
# include "CEventSource.h"
# include "CVirtualBox.h"
# include "CVirtualBoxClient.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Private QObject extension
  * providing UIVirtualBoxEventHandler with the CVirtualBoxClient and CVirtualBox event-sources. */
class UIVirtualBoxEventHandlerProxy : public QObject
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

public:

    /** Constructs event proxy object on the basis of passed @a pParent. */
    UIVirtualBoxEventHandlerProxy(QObject *pParent);
    /** Destructs event proxy object. */
    ~UIVirtualBoxEventHandlerProxy();

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

private:

    /** Holds the COM event source instance. */
    CEventSource m_comEventSource;

    /** Holds the Qt event listener instance. */
    ComObjPtr<UIMainEventListenerImpl> m_pQtListener;
    /** Holds the COM event listener instance. */
    CEventListener m_comEventListener;
};


/*********************************************************************************************************************************
*   Class UIVirtualBoxEventHandlerProxy implementation.                                                                          *
*********************************************************************************************************************************/

UIVirtualBoxEventHandlerProxy::UIVirtualBoxEventHandlerProxy(QObject *pParent)
    : QObject(pParent)
{
    /* Prepare: */
    prepare();
}

UIVirtualBoxEventHandlerProxy::~UIVirtualBoxEventHandlerProxy()
{
    /* Cleanup: */
    cleanup();
}

void UIVirtualBoxEventHandlerProxy::prepare()
{
    /* Prepare: */
    prepareListener();
    prepareConnections();
}

void UIVirtualBoxEventHandlerProxy::prepareListener()
{
    /* Create Main event listener instance: */
    m_pQtListener.createObject();
    m_pQtListener->init(new UIMainEventListener, this);
    m_comEventListener = CEventListener(m_pQtListener);

    /* Get VirtualBoxClient: */
    const CVirtualBoxClient comVBoxClient = vboxGlobal().virtualBoxClient();
    AssertWrapperOk(comVBoxClient);
    /* Get VirtualBoxClient event source: */
    CEventSource comEventSourceVBoxClient = comVBoxClient.GetEventSource();
    AssertWrapperOk(comEventSourceVBoxClient);

    /* Get VirtualBox: */
    const CVirtualBox comVBox = vboxGlobal().virtualBox();
    AssertWrapperOk(comVBox);
    /* Get VirtualBox event source: */
    CEventSource comEventSourceVBox = comVBox.GetEventSource();
    AssertWrapperOk(comEventSourceVBox);

    /* Create event source aggregator: */
    m_comEventSource = comEventSourceVBoxClient.CreateAggregator(QVector<CEventSource>()
                                                                 << comEventSourceVBoxClient
                                                                 << comEventSourceVBox);

    /* Enumerate all the required event-types: */
    QVector<KVBoxEventType> eventTypes;
    eventTypes
        /* For VirtualBoxClient: */
        << KVBoxEventType_OnVBoxSVCAvailabilityChanged
        /* For VirtualBox: */
        << KVBoxEventType_OnMachineStateChanged
        << KVBoxEventType_OnMachineDataChanged
        << KVBoxEventType_OnMachineRegistered
        << KVBoxEventType_OnSessionStateChanged
        << KVBoxEventType_OnSnapshotTaken
        << KVBoxEventType_OnSnapshotDeleted
        << KVBoxEventType_OnSnapshotChanged
        << KVBoxEventType_OnSnapshotRestored;

    /* Register event listener for event source aggregator: */
    m_comEventSource.RegisterListener(m_comEventListener, eventTypes,
        gEDataManager->eventHandlingType() == EventHandlingType_Active ? TRUE : FALSE);
    AssertWrapperOk(m_comEventSource);

    /* If event listener registered as passive one: */
    if (gEDataManager->eventHandlingType() == EventHandlingType_Passive)
    {
        /* Register event sources in their listeners as well: */
        m_pQtListener->getWrapped()->registerSource(m_comEventSource, m_comEventListener);
    }
}

void UIVirtualBoxEventHandlerProxy::prepareConnections()
{
    /* Create direct (sync) connections for signals of main listener: */
    connect(m_pQtListener->getWrapped(), SIGNAL(sigVBoxSVCAvailabilityChange(bool)),
            this, SIGNAL(sigVBoxSVCAvailabilityChange(bool)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigMachineStateChange(QString, KMachineState)),
            this, SIGNAL(sigMachineStateChange(QString, KMachineState)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigMachineDataChange(QString)),
            this, SIGNAL(sigMachineDataChange(QString)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigMachineRegistered(QString, bool)),
            this, SIGNAL(sigMachineRegistered(QString, bool)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigSessionStateChange(QString, KSessionState)),
            this, SIGNAL(sigSessionStateChange(QString, KSessionState)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigSnapshotTake(QString, QString)),
            this, SIGNAL(sigSnapshotTake(QString, QString)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigSnapshotDelete(QString, QString)),
            this, SIGNAL(sigSnapshotDelete(QString, QString)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigSnapshotChange(QString, QString)),
            this, SIGNAL(sigSnapshotChange(QString, QString)),
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), SIGNAL(sigSnapshotRestore(QString, QString)),
            this, SIGNAL(sigSnapshotRestore(QString, QString)),
            Qt::DirectConnection);
}

void UIVirtualBoxEventHandlerProxy::cleanupConnections()
{
    /* Nothing for now. */
}

void UIVirtualBoxEventHandlerProxy::cleanupListener()
{
    /* If event listener registered as passive one: */
    if (gEDataManager->eventHandlingType() == EventHandlingType_Passive)
    {
        /* Unregister everything: */
        m_pQtListener->getWrapped()->unregisterSources();
    }

    /* Unregister event listener for event source aggregator: */
    m_comEventSource.UnregisterListener(m_comEventListener);
    m_comEventSource.detach();
}

void UIVirtualBoxEventHandlerProxy::cleanup()
{
    /* Cleanup: */
    cleanupConnections();
    cleanupListener();
}


/*********************************************************************************************************************************
*   Class UIVirtualBoxEventHandler implementation.                                                                               *
*********************************************************************************************************************************/

/* static */
UIVirtualBoxEventHandler *UIVirtualBoxEventHandler::m_spInstance = 0;

/* static */
UIVirtualBoxEventHandler* UIVirtualBoxEventHandler::instance()
{
    if (!m_spInstance)
        m_spInstance = new UIVirtualBoxEventHandler;
    return m_spInstance;
}

/* static */
void UIVirtualBoxEventHandler::destroy()
{
    if (m_spInstance)
    {
        delete m_spInstance;
        m_spInstance = 0;
    }
}

UIVirtualBoxEventHandler::UIVirtualBoxEventHandler()
    : m_pProxy(new UIVirtualBoxEventHandlerProxy(this))
{
    /* Prepare: */
    prepare();
}

void UIVirtualBoxEventHandler::prepare()
{
    /* Prepare: */
    prepareConnections();
}

void UIVirtualBoxEventHandler::prepareConnections()
{
    /* Create queued (async) connections for signals of event proxy object: */
    connect(m_pProxy, SIGNAL(sigVBoxSVCAvailabilityChange(bool)),
            this, SIGNAL(sigVBoxSVCAvailabilityChange(bool)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigMachineStateChange(QString, KMachineState)),
            this, SIGNAL(sigMachineStateChange(QString, KMachineState)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigMachineDataChange(QString)),
            this, SIGNAL(sigMachineDataChange(QString)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigMachineRegistered(QString, bool)),
            this, SIGNAL(sigMachineRegistered(QString, bool)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigSessionStateChange(QString, KSessionState)),
            this, SIGNAL(sigSessionStateChange(QString, KSessionState)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigSnapshotTake(QString, QString)),
            this, SIGNAL(sigSnapshotTake(QString, QString)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigSnapshotDelete(QString, QString)),
            this, SIGNAL(sigSnapshotDelete(QString, QString)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigSnapshotChange(QString, QString)),
            this, SIGNAL(sigSnapshotChange(QString, QString)),
            Qt::QueuedConnection);
    connect(m_pProxy, SIGNAL(sigSnapshotRestore(QString, QString)),
            this, SIGNAL(sigSnapshotRestore(QString, QString)),
            Qt::QueuedConnection);
}

#include "UIVirtualBoxEventHandler.moc"


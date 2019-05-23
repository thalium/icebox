/* $Id: UINetworkManager.h $ */
/** @file
 * VBox Qt GUI - UINetworkManager stuff declaration.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UINetworkManager_h__
#define __UINetworkManager_h__

/* Qt includes: */
#include <QObject>
#include <QUuid>

/* Local inludes: */
#include "UINetworkDefs.h"

/* Forward declarations: */
class QUrl;
class QWidget;
class UINetworkRequest;
class UINetworkCustomer;
class UINetworkManagerDialog;
class UINetworkManagerIndicator;

/* QObject class reimplementation.
 * Providing network access for VirtualBox application purposes. */
class UINetworkManager : public QObject
{
    Q_OBJECT;

signals:

    /* Ask listeners (network-requests) to cancel: */
    void sigCancelNetworkRequests();

    /** Requests to add @a pNetworkRequest to network-manager state-indicators. */
    void sigAddNetworkManagerIndicatorDescription(UINetworkRequest *pNetworkRequest);
    /** Requests to remove network-request with @a uuid from network-manager state-indicators. */
    void sigRemoveNetworkManagerIndicatorDescription(const QUuid &uuid);

public:

    /* Instance: */
    static UINetworkManager* instance() { return m_pInstance; }

    /* Create/destroy singleton: */
    static void create();
    static void destroy();

    /* Pointer to network-manager dialog: */
    UINetworkManagerDialog* window() const;

    /** Creates network-manager state-indicator.
      * @remarks To be cleaned up by the caller. */
    UINetworkManagerIndicator* createIndicator() const;

    /** Registers @a pNetworkRequest in network-manager. */
    void registerNetworkRequest(UINetworkRequest *pNetworkRequest);
    /** Unregisters network-request with @a uuid from network-manager. */
    void unregisterNetworkRequest(const QUuid &uuid);

public slots:

    /* Show network-manager dialog: */
    void show();

protected:

    /* Allow UINetworkCustomer to create network-request: */
    friend class UINetworkCustomer;
    /** Creates network-request of the passed @a type
      * on the basis of the passed @a urls and the @a requestHeaders for the @a pCustomer specified. */
    void createNetworkRequest(UINetworkRequestType type, const QList<QUrl> &urls,
                              const UserDictionary &requestHeaders, UINetworkCustomer *pCustomer);

private:

    /* Constructor/destructor: */
    UINetworkManager();
    ~UINetworkManager();

    /* Prepare/cleanup: */
    void prepare();
    void cleanup();

    /* Network-request prepare helper: */
    void prepareNetworkRequest(UINetworkRequest *pNetworkRequest);
    /* Network-request cleanup helper: */
    void cleanupNetworkRequest(QUuid uuid);
    /* Network-requests cleanup helper: */
    void cleanupNetworkRequests();

private slots:

    /* Slot to handle network-request progress: */
    void sltHandleNetworkRequestProgress(const QUuid &uuid, qint64 iReceived, qint64 iTotal);
    /* Slot to handle network-request cancel: */
    void sltHandleNetworkRequestCancel(const QUuid &uuid);
    /* Slot to handle network-request finish: */
    void sltHandleNetworkRequestFinish(const QUuid &uuid);
    /* Slot to handle network-request failure: */
    void sltHandleNetworkRequestFailure(const QUuid &uuid, const QString &strError);

private:

    /* Instance: */
    static UINetworkManager *m_pInstance;

    /* Network-request map: */
    QMap<QUuid, UINetworkRequest*> m_requests;

    /* Network-manager dialog: */
    UINetworkManagerDialog *m_pNetworkManagerDialog;
};
#define gNetworkManager UINetworkManager::instance()

#endif // __UINetworkManager_h__


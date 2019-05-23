/* $Id: UINetworkRequest.h $ */
/** @file
 * VBox Qt GUI - UINetworkRequest class declaration.
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

#ifndef ___UINetworkRequest_h___
#define ___UINetworkRequest_h___

/* Qt includes: */
#include <QUuid>
#include <QPointer>

/* GUI inludes: */
#include "UINetworkDefs.h"
#include "UINetworkReply.h"

/* Forward declarations: */
class UINetworkManager;
class UINetworkManagerDialog;
class UINetworkManagerIndicator;
class UINetworkRequestWidget;
class UINetworkCustomer;

/** QObject extension used as network-request container. */
class UINetworkRequest : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies common UINetworkManager about progress with @a uuid changed.
      * @param  iReceived  Holds the amount of bytes received.
      * @param  iTotal     Holds the amount of total bytes to receive. */
    void sigProgress(const QUuid &uuid, qint64 iReceived, qint64 iTotal);
    /** Notifies UINetworkManager about progress with @a uuid started. */
    void sigStarted(const QUuid &uuid);
    /** Notifies UINetworkManager about progress with @a uuid canceled. */
    void sigCanceled(const QUuid &uuid);
    /** Notifies UINetworkManager about progress with @a uuid finished. */
    void sigFinished(const QUuid &uuid);
    /** Notifies UINetworkManager about progress with @a uuid failed with @a strError. */
    void sigFailed(const QUuid &uuid, const QString &strError);

    /** Notifies own UINetworkRequestWidget about progress changed.
      * @param  iReceived  Holds the amount of bytes received.
      * @param  iTotal     Holds the amount of total bytes to receive. */
    void sigProgress(qint64 iReceived, qint64 iTotal);
    /** Notifies own UINetworkRequestWidget about progress started. */
    void sigStarted();
    /** Notifies own UINetworkRequestWidget about progress finished. */
    void sigFinished();
    /** Notifies own UINetworkRequestWidget about progress failed with @a strError. */
    void sigFailed(const QString &strError);

public:

    /** Constructs network-request of the passed @a type
      * on the basis of the passed @a urls and the @a requestHeaders
      * for the @a pCustomer and @a pNetworkManager specified. */
    UINetworkRequest(UINetworkRequestType type,
                     const QList<QUrl> &urls,
                     const UserDictionary &requestHeaders,
                     UINetworkCustomer *pCustomer,
                     UINetworkManager *pNetworkManager);
    /** Destructs network-request. */
    ~UINetworkRequest();

    /** Returns the request description. */
    const QString description() const;
    /** Returns the request customer. */
    UINetworkCustomer* customer() { return m_pCustomer; }
    /** Returns the request manager. */
    UINetworkManager* manager() const { return m_pNetworkManager; }
    /** Returns unique request QUuid. */
    const QUuid& uuid() const { return m_uuid; }
    /** Returns the request reply. */
    UINetworkReply* reply() { return m_pReply; }

public slots:

    /** Initiates request retrying. */
    void sltRetry();
    /** Initiates request cancelling. */
    void sltCancel();

private slots:

    /** Handles reply about progress changed.
      * @param  iReceived  Brings the amount of bytes received.
      * @param  iTotal     Brings the amount of total bytes to receive. */
    void sltHandleNetworkReplyProgress(qint64 iReceived, qint64 iTotal);
    /** Handles reply about progress finished. */
    void sltHandleNetworkReplyFinish();

private:

    /** Prepares request. */
    void prepare();
    /** Prepares request's reply. */
    void prepareNetworkReply();

    /** Cleanups request's reply. */
    void cleanupNetworkReply();
    /** Cleanups request. */
    void cleanup();

    /** Holds the request type. */
    const UINetworkRequestType m_type;
    /** Holds the request urls. */
    const QList<QUrl> m_urls;
    /** Holds the request headers. */
    const UserDictionary m_requestHeaders;
    /** Holds the request customer. */
    UINetworkCustomer *m_pCustomer;
    /** Holds the request manager. */
    UINetworkManager *m_pNetworkManager;
    /** Holds unique request QUuid. */
    const QUuid m_uuid;

    /** Holds current request url. */
    QUrl m_url;
    /** Holds index of current request url. */
    int m_iUrlIndex;
    /** Holds whether current request url is in progress. */
    bool m_fRunning;

    /** Holds the request reply. */
    QPointer<UINetworkReply> m_pReply;
};

#endif /* !___UINetworkRequest_h___ */


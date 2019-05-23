/* $Id: UINetworkCustomer.h $ */
/** @file
 * VBox Qt GUI - UINetworkCustomer class declaration.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UINetworkCustomer_h__
#define __UINetworkCustomer_h__

/* Global includes: */
#include <QObject>

/* Local includes: */
#include "UINetworkDefs.h"

/* Forward declarations: */
class UINetworkReply;
class QUrl;

/* Interface to access UINetworkManager protected functionality: */
class UINetworkCustomer : public QObject
{
    Q_OBJECT;

public:

    /* Constructors: */
    UINetworkCustomer();
    UINetworkCustomer(QObject *pParent, bool fForceCall);

    /* Getters: */
    bool isItForceCall() const { return m_fForceCall; }

    /* Network-reply progress handler: */
    virtual void processNetworkReplyProgress(qint64 iReceived, qint64 iTotal) = 0;
    /* Network-reply cancel handler: */
    virtual void processNetworkReplyCanceled(UINetworkReply *pReply) = 0;
    /* Network-reply finish handler: */
    virtual void processNetworkReplyFinished(UINetworkReply *pReply) = 0;

    /** Returns description of the current network operation. */
    virtual const QString description() const { return QString(); }

protected:

    /** Creates network-request of the passed @a type on the basis of the passed @a urls and the @a requestHeaders. */
    void createNetworkRequest(UINetworkRequestType type, const QList<QUrl> urls,
                              const UserDictionary requestHeaders = UserDictionary());

private:

    /* Variables: */
    bool m_fForceCall;
};

#endif // __UINetworkCustomer_h__


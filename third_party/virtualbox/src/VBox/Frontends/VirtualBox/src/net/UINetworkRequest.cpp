/* $Id: UINetworkRequest.cpp $ */
/** @file
 * VBox Qt GUI - UINetworkRequest class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "UINetworkRequest.h"
# include "UINetworkRequestWidget.h"
# include "UINetworkManager.h"
# include "UINetworkManagerDialog.h"
# include "UINetworkManagerIndicator.h"
# include "UINetworkCustomer.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

UINetworkRequest::UINetworkRequest(UINetworkRequestType type,
                                   const QList<QUrl> &urls,
                                   const UserDictionary &requestHeaders,
                                   UINetworkCustomer *pCustomer,
                                   UINetworkManager *pNetworkManager)
    : QObject(pNetworkManager)
    , m_type(type)
    , m_urls(urls)
    , m_requestHeaders(requestHeaders)
    , m_pCustomer(pCustomer)
    , m_pNetworkManager(pNetworkManager)
    , m_uuid(QUuid::createUuid())
    , m_iUrlIndex(-1)
    , m_fRunning(false)
{
    /* Prepare: */
    prepare();
}

UINetworkRequest::~UINetworkRequest()
{
    /* Cleanup: */
    cleanup();
}

const QString UINetworkRequest::description() const
{
    return m_pCustomer->description();
}

void UINetworkRequest::sltHandleNetworkReplyProgress(qint64 iReceived, qint64 iTotal)
{
    /* Notify common network-request listeners: */
    emit sigProgress(m_uuid, iReceived, iTotal);
    /* Notify own network-request listeners: */
    emit sigProgress(iReceived, iTotal);
}

void UINetworkRequest::sltHandleNetworkReplyFinish()
{
    /* Mark network-reply as non-running: */
    m_fRunning = false;

    /* Make sure network-reply still valid: */
    if (!m_pReply)
        return;

    /* If network-reply has no errors: */
    if (m_pReply->error() == UINetworkReply::NoError)
    {
        /* Notify own network-request listeners: */
        emit sigFinished();
        /* Notify common network-request listeners: */
        emit sigFinished(m_uuid);
    }
    /* If network-request was canceled: */
    else if (m_pReply->error() == UINetworkReply::OperationCanceledError)
    {
        /* Notify network-manager: */
        emit sigCanceled(m_uuid);
    }
    /* If some other error occured: */
    else
    {
        /* Check if we are able to handle error: */
        bool fErrorHandled = false;

        /* Handle redirection: */
        switch (m_pReply->error())
        {
            case UINetworkReply::ContentReSendError:
            {
                /* Check whether redirection link was acquired: */
                const QString strRedirect = m_pReply->header(UINetworkReply::LocationHeader).toString();
                if (!strRedirect.isEmpty())
                {
                    /* Cleanup current network-reply first: */
                    cleanupNetworkReply();

                    /* Choose redirect-source as current url: */
                    m_url = strRedirect;

                    /* Create new network-reply finally: */
                    prepareNetworkReply();

                    /* Mark this error handled: */
                    fErrorHandled = true;
                }
                break;
            }
            default:
                break;
        }

        /* If error still unhandled: */
        if (!fErrorHandled)
        {
            /* Check if we have other urls in queue: */
            if (m_iUrlIndex < m_urls.size() - 1)
            {
                /* Cleanup current network-reply first: */
                cleanupNetworkReply();

                /* Choose next url as current: */
                ++m_iUrlIndex;
                m_url = m_urls.at(m_iUrlIndex);

                /* Create new network-reply finally: */
                prepareNetworkReply();
            }
            else
            {
                /* Notify own network-request listeners: */
                emit sigFailed(m_pReply->errorString());
                /* Notify common network-request listeners: */
                emit sigFailed(m_uuid, m_pReply->errorString());
            }
        }
    }
}

void UINetworkRequest::sltRetry()
{
    /* Cleanup current network-reply first: */
    cleanupNetworkReply();

    /* Choose first url as current: */
    m_iUrlIndex = 0;
    m_url = m_urls.at(m_iUrlIndex);

    /* Create new network-reply finally: */
    prepareNetworkReply();
}

void UINetworkRequest::sltCancel()
{
    /* Abort network-reply if present: */
    if (m_pReply)
    {
        if (m_fRunning)
            m_pReply->abort();
        else
            emit sigCanceled(m_uuid);
    }
}

void UINetworkRequest::prepare()
{
    /* Prepare listeners for network-manager: */
    connect(manager(), &UINetworkManager::sigCancelNetworkRequests,
            this, &UINetworkRequest::sltCancel, Qt::QueuedConnection);

    /* Choose first url as current: */
    m_iUrlIndex = 0;
    m_url = m_urls.at(m_iUrlIndex);

    /* Register network-request in network-manager: */
    manager()->registerNetworkRequest(this);

    /* Prepare network-reply: */
    prepareNetworkReply();
}

void UINetworkRequest::prepareNetworkReply()
{
    /* Create network-reply: */
    m_pReply = new UINetworkReply(m_type, m_url, m_requestHeaders);
    AssertPtrReturnVoid(m_pReply.data());
    {
        /* Prepare network-reply: */
        connect(m_pReply.data(), &UINetworkReply::downloadProgress,
                this, &UINetworkRequest::sltHandleNetworkReplyProgress);
        connect(m_pReply.data(), &UINetworkReply::finished,
                this, &UINetworkRequest::sltHandleNetworkReplyFinish);

        /* Mark network-reply as running: */
        m_fRunning = true;

        /* Notify common network-request listeners: */
        emit sigStarted(m_uuid);
        /* Notify own network-request listeners: */
        emit sigStarted();
    }
}

void UINetworkRequest::cleanupNetworkReply()
{
    /* Destroy network-reply: */
    AssertPtrReturnVoid(m_pReply.data());
    m_pReply->disconnect();
    m_pReply->deleteLater();
    m_pReply = 0;
}

void UINetworkRequest::cleanup()
{
    /* Cleanup network-reply: */
    cleanupNetworkReply();

    /* Unregister network-request from network-manager: */
    manager()->unregisterNetworkRequest(m_uuid);
}


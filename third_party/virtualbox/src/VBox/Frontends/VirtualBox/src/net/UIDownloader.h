/* $Id: UIDownloader.h $ */
/** @file
 * VBox Qt GUI - UIDownloader class declaration.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/// This class requires proper doxy.
/// For now I'm keeping the old style consistent.

#ifndef __UIDownloader_h__
#define __UIDownloader_h__

/* Global includes: */
#include <QUrl>
#include <QList>

/* Local includes: */
#include "UINetworkDefs.h"
#include "UINetworkCustomer.h"

/* Forward declarations: */
class UINetworkReply;

/* Downloader interface.
 * UINetworkCustomer class extension which allows background http downloading. */
class UIDownloader : public UINetworkCustomer
{
    Q_OBJECT;

signals:

    /* Signal to start acknowledging: */
    void sigToStartAcknowledging();
    /* Signal to start downloading: */
    void sigToStartDownloading();
    /* Signal to start verifying: */
    void sigToStartVerifying();

public:

    /* Starting routine: */
    void start();

protected slots:

    /* Acknowledging part: */
    void sltStartAcknowledging();
    /* Downloading part: */
    void sltStartDownloading();
    /* Verifying part: */
    void sltStartVerifying();

protected:

    /* UIDownloader states: */
    enum UIDownloaderState
    {
        UIDownloaderState_Null,
        UIDownloaderState_Acknowledging,
        UIDownloaderState_Downloading,
        UIDownloaderState_Verifying,
    };

    /* Constructor: */
    UIDownloader();

    /* Source stuff,
     * allows to set/get one or more sources to try to download from: */
    void addSource(const QString &strSource) { m_sources << QUrl(strSource); }
    void setSource(const QString &strSource) { m_sources.clear(); addSource(strSource); }
    const QList<QUrl>& sources() const { return m_sources; }
    const QUrl& source() const { return m_source; }

    /* Target stuff,
     * allows to set/get downloaded file destination: */
    void setTarget(const QString &strTarget) { m_strTarget = strTarget; }
    const QString& target() const { return m_strTarget; }

    /* SHA-256 stuff,
     * allows to set/get SHA-256 sum dictionary file for downloaded source. */
    QString pathSHA256SumsFile() const { return m_strPathSHA256SumsFile; }
    void setPathSHA256SumsFile(const QString &strPathSHA256SumsFile) { m_strPathSHA256SumsFile = strPathSHA256SumsFile; }

    /** Returns description of the current network operation. */
    virtual const QString description() const;

    /* Start delayed acknowledging: */
    void startDelayedAcknowledging() { emit sigToStartAcknowledging(); }
    /* Start delayed downloading: */
    void startDelayedDownloading() { emit sigToStartDownloading(); }
    /* Start delayed verifying: */
    void startDelayedVerifying() { emit sigToStartVerifying(); }

    /* Network-reply progress handler: */
    void processNetworkReplyProgress(qint64 iReceived, qint64 iTotal);
    /* Network-reply cancel handler: */
    void processNetworkReplyCanceled(UINetworkReply *pNetworkReply);
    /* Network-reply finish handler: */
    void processNetworkReplyFinished(UINetworkReply *pNetworkReply);

    /* Handle acknowledging result: */
    virtual void handleAcknowledgingResult(UINetworkReply *pNetworkReply);
    /* Handle downloading result: */
    virtual void handleDownloadingResult(UINetworkReply *pNetworkReply);
    /* Handle verifying result: */
    virtual void handleVerifyingResult(UINetworkReply *pNetworkReply);

    /* Pure virtual function to ask user about downloading confirmation: */
    virtual bool askForDownloadingConfirmation(UINetworkReply *pNetworkReply) = 0;
    /* Pure virtual function to handle downloaded object: */
    virtual void handleDownloadedObject(UINetworkReply *pNetworkReply) = 0;
    /* Base virtual function to handle verified object: */
    virtual void handleVerifiedObject(UINetworkReply * /* pNetworkReply */) {}

private:

    /* Private variables: */
    UIDownloaderState m_state;
    QList<QUrl> m_sources;
    QUrl m_source;
    QString m_strTarget;
    QString m_strPathSHA256SumsFile;
};

#endif // __UIDownloader_h__


/* $Id: UIDownloaderAdditions.cpp $ */
/** @file
 * VBox Qt GUI - UIDownloaderAdditions class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Global includes: */
# include <QDir>
# include <QFile>

/* Local includes: */
# include "UIDownloaderAdditions.h"
# include "UINetworkReply.h"
# include "QIFileDialog.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIModalWindowManager.h"
# include "UIVersion.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Other VBox includes: */
#include <iprt/sha.h>


/* static */
UIDownloaderAdditions* UIDownloaderAdditions::m_spInstance = 0;

/* static */
UIDownloaderAdditions* UIDownloaderAdditions::create()
{
    if (!m_spInstance)
        m_spInstance = new UIDownloaderAdditions;
    return m_spInstance;
}

/* static */
UIDownloaderAdditions* UIDownloaderAdditions::current()
{
    return m_spInstance;
}

UIDownloaderAdditions::UIDownloaderAdditions()
{
    /* Prepare instance: */
    if (!m_spInstance)
        m_spInstance = this;

    /* Get version number and adjust it for test and trunk builds. The server only has official releases. */
    const QString strVersion = UIVersion(vboxGlobal().vboxVersionStringNormalized()).effectiveRelasedVersion().toString();

    /* Prepare source/target: */
    const QString strSourceName = QString("%1_%2.iso").arg(GUI_GuestAdditionsName, strVersion);
    const QString strSourcePath = QString("https://download.virtualbox.org/virtualbox/%1/").arg(strVersion);
    const QString strSource = strSourcePath + strSourceName;
    const QString strPathSHA256SumsFile = QString("https://www.virtualbox.org/download/hashes/%1/SHA256SUMS").arg(strVersion);
    const QString strTarget = QDir(vboxGlobal().homeFolder()).absoluteFilePath(strSourceName);

    /* Set source/target: */
    setSource(strSource);
    setTarget(strTarget);
    setPathSHA256SumsFile(strPathSHA256SumsFile);
}

UIDownloaderAdditions::~UIDownloaderAdditions()
{
    /* Cleanup instance: */
    if (m_spInstance == this)
        m_spInstance = 0;
}

/* virtual override */
const QString UIDownloaderAdditions::description() const
{
    return UIDownloader::description().arg(tr("VirtualBox Guest Additions"));
}

bool UIDownloaderAdditions::askForDownloadingConfirmation(UINetworkReply *pReply)
{
    return msgCenter().confirmDownloadGuestAdditions(source().toString(), pReply->header(UINetworkReply::ContentLengthHeader).toInt());
}

void UIDownloaderAdditions::handleDownloadedObject(UINetworkReply *pReply)
{
    /* Read received data into the buffer: */
    m_receivedData = pReply->readAll();
}

void UIDownloaderAdditions::handleVerifiedObject(UINetworkReply *pReply)
{
    /* Try to verify the SHA-256 checksum: */
    QString strCalculatedSumm;
    bool fSuccess = false;
    do
    {
        /* Read received data into the buffer: */
        const QByteArray receivedData(pReply->readAll());
        /* Make sure it's not empty: */
        if (receivedData.isEmpty())
            break;

        /* Parse buffer contents to dictionary: */
        const QStringList dictionary(QString(receivedData).split("\n", QString::SkipEmptyParts));
        /* Make sure it's not empty: */
        if (dictionary.isEmpty())
            break;

        /* Parse each record to tags, look for the required one: */
        foreach (const QString &strRecord, dictionary)
        {
            const QString strFileName = strRecord.section(" *", 1);
            const QString strDownloadedSumm = strRecord.section(" *", 0, 0);
            if (strFileName == source().fileName())
            {
                /* Calc the SHA-256 on the bytes, creating a string: */
                uint8_t abHash[RTSHA256_HASH_SIZE];
                RTSha256(m_receivedData.constData(), m_receivedData.length(), abHash);
                char szDigest[RTSHA256_DIGEST_LEN + 1];
                int rc = RTSha256ToString(abHash, szDigest, sizeof(szDigest));
                if (RT_FAILURE(rc))
                {
                    AssertRC(rc);
                    szDigest[0] = '\0';
                }
                strCalculatedSumm = szDigest;
                //printf("Downloaded SHA-256 summ: [%s]\n", strDownloadedSumm.toUtf8().constData());
                //printf("Calculated SHA-256 summ: [%s]\n", strCalculatedSumm.toUtf8().constData());
                /* Make sure checksum is valid: */
                fSuccess = strDownloadedSumm == strCalculatedSumm;
                break;
            }
        }
    }
    while (false);

    /* If SHA-256 checksum verification failed: */
    if (!fSuccess)
    {
        /* Warn the user about additions-image was downloaded and saved but checksum is invalid: */
        msgCenter().cannotValidateGuestAdditionsSHA256Sum(source().toString(), QDir::toNativeSeparators(target()));
        return;
    }

    /* Serialize that buffer into the file: */
    while (true)
    {
        /* Try to open file for writing: */
        QFile file(target());
        if (file.open(QIODevice::WriteOnly))
        {
            /* Write buffer into the file: */
            file.write(m_receivedData);
            file.close();

            /* Warn the user about additions-image loaded and saved, propose to mount it: */
            if (msgCenter().proposeMountGuestAdditions(source().toString(), QDir::toNativeSeparators(target())))
                emit sigDownloadFinished(target());
            break;
        }

        /* Warn the user about additions-image was downloaded but was NOT saved: */
        msgCenter().cannotSaveGuestAdditions(source().toString(), QDir::toNativeSeparators(target()));

        /* Ask the user for another location for the additions-image file: */
        QString strTarget = QIFileDialog::getExistingDirectory(QFileInfo(target()).absolutePath(),
                                                               windowManager().networkManagerOrMainWindowShown(),
                                                               tr("Select folder to save Guest Additions image to"), true);

        /* Check if user had really set a new target: */
        if (!strTarget.isNull())
            setTarget(QDir(strTarget).absoluteFilePath(QFileInfo(target()).fileName()));
        else
            break;
    }
}


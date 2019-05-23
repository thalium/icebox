/* $Id: UIDownloaderUserManual.cpp $ */
/** @file
 * VBox Qt GUI - UIDownloaderUserManual class implementation.
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
# include "UIDownloaderUserManual.h"
# include "UINetworkReply.h"
# include "QIFileDialog.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIModalWindowManager.h"
# include "UIVersion.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static */
UIDownloaderUserManual* UIDownloaderUserManual::m_spInstance = 0;

/* static */
UIDownloaderUserManual* UIDownloaderUserManual::create()
{
    if (!m_spInstance)
        m_spInstance = new UIDownloaderUserManual;
    return m_spInstance;
}

/* static */
UIDownloaderUserManual* UIDownloaderUserManual::current()
{
    return m_spInstance;
}

UIDownloaderUserManual::UIDownloaderUserManual()
{
    /* Prepare instance: */
    if (!m_spInstance)
        m_spInstance = this;

    /* Get version number and adjust it for test and trunk builds. The server only has official releases. */
    const QString strVersion = UIVersion(vboxGlobal().vboxVersionStringNormalized()).effectiveRelasedVersion().toString();

    /* Compose User Manual filename: */
    QString strUserManualFullFileName = vboxGlobal().helpFile();
    QString strUserManualShortFileName = QFileInfo(strUserManualFullFileName).fileName();

    /* Add sources: */
    QString strSource1 = QString("https://download.virtualbox.org/virtualbox/%1/").arg(strVersion) + strUserManualShortFileName;
    QString strSource2 = QString("https://download.virtualbox.org/virtualbox/") + strUserManualShortFileName;
    addSource(strSource1);
    addSource(strSource2);

    /* Set target: */
    QString strUserManualDestination = QDir(vboxGlobal().homeFolder()).absoluteFilePath(strUserManualShortFileName);
    setTarget(strUserManualDestination);
}

UIDownloaderUserManual::~UIDownloaderUserManual()
{
    /* Cleanup instance: */
    if (m_spInstance == this)
        m_spInstance = 0;
}

/* virtual override */
const QString UIDownloaderUserManual::description() const
{
    return UIDownloader::description().arg(tr("VirtualBox User Manual"));
}

bool UIDownloaderUserManual::askForDownloadingConfirmation(UINetworkReply *pReply)
{
    return msgCenter().confirmDownloadUserManual(source().toString(), pReply->header(UINetworkReply::ContentLengthHeader).toInt());
}

void UIDownloaderUserManual::handleDownloadedObject(UINetworkReply *pReply)
{
    /* Read received data into the buffer: */
    QByteArray receivedData(pReply->readAll());
    /* Serialize that buffer into the file: */
    while (true)
    {
        /* Try to open file for writing: */
        QFile file(target());
        if (file.open(QIODevice::WriteOnly))
        {
            /* Write buffer into the file: */
            file.write(receivedData);
            file.close();

            /* Warn the user about user-manual loaded and saved: */
            msgCenter().warnAboutUserManualDownloaded(source().toString(), QDir::toNativeSeparators(target()));
            /* Warn the listener about user-manual was downloaded: */
            emit sigDownloadFinished(target());
            break;
        }

        /* Warn user about user-manual was downloaded but was NOT saved: */
        msgCenter().cannotSaveUserManual(source().toString(), QDir::toNativeSeparators(target()));

        /* Ask the user for another location for the user-manual file: */
        QString strTarget = QIFileDialog::getExistingDirectory(QFileInfo(target()).absolutePath(),
                                                               windowManager().networkManagerOrMainWindowShown(),
                                                               tr("Select folder to save User Manual to"), true);

        /* Check if user had really set a new target: */
        if (!strTarget.isNull())
            setTarget(QDir(strTarget).absoluteFilePath(QFileInfo(target()).fileName()));
        else
            break;
    }
}


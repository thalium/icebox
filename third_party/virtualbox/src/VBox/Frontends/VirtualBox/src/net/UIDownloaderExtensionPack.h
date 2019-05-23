/* $Id: UIDownloaderExtensionPack.h $ */
/** @file
 * VBox Qt GUI - UIDownloaderExtensionPack class declaration.
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

#ifndef __UIDownloaderExtensionPack_h__
#define __UIDownloaderExtensionPack_h__

/* Local includes: */
#include "UIDownloader.h"

/* UIDownloader extension for background extension-pack downloading. */
class UIDownloaderExtensionPack : public UIDownloader
{
    Q_OBJECT;

signals:

    /* Notifies listeners about downloading finished: */
    void sigDownloadFinished(const QString &strSource, const QString &strTarget, QString strHash);

public:

    /* Static stuff: */
    static UIDownloaderExtensionPack* create();
    static UIDownloaderExtensionPack* current();

private:

    /* Constructor/destructor: */
    UIDownloaderExtensionPack();
    ~UIDownloaderExtensionPack();

    /** Returns description of the current network operation. */
    virtual const QString description() const;

    /* Virtual stuff reimplementations: */
    bool askForDownloadingConfirmation(UINetworkReply *pReply);
    void handleDownloadedObject(UINetworkReply *pReply);
    void handleVerifiedObject(UINetworkReply *pReply);

    /* Variables: */
    static UIDownloaderExtensionPack *m_spInstance;
    QByteArray m_receivedData;
};

#endif // __UIDownloaderExtensionPack_h__


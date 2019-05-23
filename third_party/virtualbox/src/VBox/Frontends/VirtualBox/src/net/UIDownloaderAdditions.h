/* $Id: UIDownloaderAdditions.h $ */
/** @file
 * VBox Qt GUI - UIDownloaderAdditions class declaration.
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

#ifndef __UIDownloaderAdditions_h__
#define __UIDownloaderAdditions_h__

/* Local includes: */
#include "UIDownloader.h"

/* UIDownloader extension for background additions downloading. */
class UIDownloaderAdditions : public UIDownloader
{
    Q_OBJECT;

signals:

    /* Notifies listeners about downloading finished: */
    void sigDownloadFinished(const QString &strFile);

public:

    /* Static stuff: */
    static UIDownloaderAdditions* create();
    static UIDownloaderAdditions* current();

private:

    /* Constructor/destructor: */
    UIDownloaderAdditions();
    ~UIDownloaderAdditions();

    /** Returns description of the current network operation. */
    virtual const QString description() const;

    /* Virtual stuff reimplementations: */
    bool askForDownloadingConfirmation(UINetworkReply *pReply);
    void handleDownloadedObject(UINetworkReply *pReply);
    void handleVerifiedObject(UINetworkReply *pReply);

    /* Variables: */
    static UIDownloaderAdditions *m_spInstance;
    QByteArray m_receivedData;
};

#endif // __UIDownloaderAdditions_h__


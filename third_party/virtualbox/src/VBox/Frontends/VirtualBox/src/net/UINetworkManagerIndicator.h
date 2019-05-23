/* $Id: UINetworkManagerIndicator.h $ */
/** @file
 * VBox Qt GUI - UINetworkManagerIndicator stuff declaration.
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

#ifndef __UINetworkManagerIndicator_h__
#define __UINetworkManagerIndicator_h__

/* Global includes: */
#include <QVector>
#include <QUuid>

/* Local includes: */
#include "QIStatusBarIndicator.h"

/* Forward declarations: */
class UINetworkRequest;

/* Network-manager status-bar indicator states: */
enum UINetworkManagerIndicatorState
{
    UINetworkManagerIndicatorState_Idle,
    UINetworkManagerIndicatorState_Loading,
    UINetworkManagerIndicatorState_Error
};

/* Network-manager status-bar indicator: */
class UINetworkManagerIndicator : public QIStateStatusBarIndicator
{
    Q_OBJECT;

public:

    /* Constructor: */
    UINetworkManagerIndicator();

    /** Update routine. */
    void updateAppearance();

public slots:

    /** Adds @a pNetworkRequest to network-manager state-indicators. */
    void sltAddNetworkManagerIndicatorDescription(UINetworkRequest *pNetworkRequest);
    /** Removes network-request with @a uuid from network-manager state-indicators. */
    void sldRemoveNetworkManagerIndicatorDescription(const QUuid &uuid);

private slots:

    /* Set particular network-request progress to 'started': */
    void sltSetProgressToStarted(const QUuid &uuid);
    /* Set particular network-request progress to 'canceled': */
    void sltSetProgressToCanceled(const QUuid &uuid);
    /* Set particular network-request progress to 'failed': */
    void sltSetProgressToFailed(const QUuid &uuid, const QString &strError);
    /* Set particular network-request progress to 'finished': */
    void sltSetProgressToFinished(const QUuid &uuid);
    /* Update particular network-request progress: */
    void sltSetProgress(const QUuid &uuid, qint64 iReceived, qint64 iTotal);

private:

    struct UINetworkRequestData
    {
        UINetworkRequestData()
            : bytesReceived(0), bytesTotal(0), failed(false) {}
        UINetworkRequestData(const QString &strDescription, int iBytesReceived, int iBytesTotal)
            : description(strDescription), bytesReceived(iBytesReceived), bytesTotal(iBytesTotal), failed(false) {}
        QString description;
        int bytesReceived;
        int bytesTotal;
        bool failed;
    };

    /* Translate stuff: */
    void retranslateUi();

    /* Update stuff: */
    void recalculateIndicatorState();

    /* Variables: */
    QVector<QUuid> m_ids;
    QVector<UINetworkRequestData> m_data;
};

#endif // __UINetworkManagerIndicator_h__


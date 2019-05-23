/* $Id: UINetworkRequestWidget.h $ */
/** @file
 * VBox Qt GUI - UINetworkRequestWidget stuff declaration.
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

#ifndef __UINetworkRequestWidget_h__
#define __UINetworkRequestWidget_h__

/* Local inludes: */
#include "QIWithRetranslateUI.h"
#include "UIPopupBox.h"

/* Forward declarations: */
class UINetworkManagerDialog;
class QWidget;
class QGridLayout;
class QProgressBar;
class QIToolButton;
class QIRichTextLabel;
class UINetworkRequest;
class QTimer;

/* UIPopupBox reimplementation to reflect network-request status: */
class UINetworkRequestWidget : public QIWithRetranslateUI<UIPopupBox>
{
    Q_OBJECT;

signals:

    /* Signal to retry network-request: */
    void sigRetry();
    /* Signal to cancel network-request: */
    void sigCancel();

protected:

    /* Allow creation of UINetworkRequestWidget to UINetworkManagerDialog only: */
    friend class UINetworkManagerDialog;
    /* Constructor: */
    UINetworkRequestWidget(UINetworkManagerDialog *pParent, UINetworkRequest *pNetworkRequest);

private slots:

    /* Updates current network-request progess: */
    void sltSetProgress(qint64 iReceived, qint64 iTotal);

    /* Set current network-request progress to 'started': */
    void sltSetProgressToStarted();
    /* Set current network-request progress to 'finished': */
    void sltSetProgressToFinished();
    /* Set current network-request progress to 'failed': */
    void sltSetProgressToFailed(const QString &strError);

    /* Handle frozen progress: */
    void sltTimeIsOut();

private:

    /* Translation stuff: */
    void retranslateUi();

    /** Composes error text on the basis of the passed @a strErrorText. */
    static const QString composeErrorText(QString strErrorText);

    /* Widgets: */
    QWidget *m_pContentWidget;
    QGridLayout *m_pMainLayout;
    QProgressBar *m_pProgressBar;
    QIToolButton *m_pRetryButton;
    QIToolButton *m_pCancelButton;
    QIRichTextLabel *m_pErrorPane;

    /* Objects: */
    UINetworkRequest *m_pNetworkRequest;
    QTimer *m_pTimer;
};

#endif // __UINetworkRequestWidget_h__


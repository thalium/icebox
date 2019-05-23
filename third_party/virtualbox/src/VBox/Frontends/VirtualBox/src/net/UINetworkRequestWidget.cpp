/* $Id: UINetworkRequestWidget.cpp $ */
/** @file
 * VBox Qt GUI - UINetworkRequestWidget stuff implementation.
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

/* Global includes: */
# include <QTimer>
# include <QStyle>
# include <QGridLayout>
# include <QProgressBar>

/* Local includes: */
# include "UINetworkRequestWidget.h"
# include "UINetworkRequest.h"
# include "UINetworkManager.h"
# include "UINetworkManagerDialog.h"
# include "UIIconPool.h"
# include "QIToolButton.h"
# include "QIRichTextLabel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UINetworkRequestWidget::UINetworkRequestWidget(UINetworkManagerDialog *pParent, UINetworkRequest *pNetworkRequest)
    : QIWithRetranslateUI<UIPopupBox>(pParent)
    , m_pContentWidget(new QWidget(this))
    , m_pMainLayout(new QGridLayout(m_pContentWidget))
    , m_pProgressBar(new QProgressBar(m_pContentWidget))
    , m_pRetryButton(new QIToolButton(m_pContentWidget))
    , m_pCancelButton(new QIToolButton(m_pContentWidget))
    , m_pErrorPane(new QIRichTextLabel(m_pContentWidget))
    , m_pNetworkRequest(pNetworkRequest)
    , m_pTimer(new QTimer(this))
{
    /* Setup self: */
    setTitleIcon(UIIconPool::iconSet(":/download_manager_16px.png"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setContentWidget(m_pContentWidget);
    setOpen(true);

    /* Prepare listeners for m_pNetworkRequest: */
    connect(m_pNetworkRequest, static_cast<void(UINetworkRequest::*)(qint64, qint64)>(&UINetworkRequest::sigProgress),
            this, &UINetworkRequestWidget::sltSetProgress);
    connect(m_pNetworkRequest, static_cast<void(UINetworkRequest::*)()>(&UINetworkRequest::sigStarted),
            this, &UINetworkRequestWidget::sltSetProgressToStarted);
    connect(m_pNetworkRequest, static_cast<void(UINetworkRequest::*)()>(&UINetworkRequest::sigFinished),
            this, &UINetworkRequestWidget::sltSetProgressToFinished);
    connect(m_pNetworkRequest, static_cast<void(UINetworkRequest::*)(const QString&)>(&UINetworkRequest::sigFailed),
            this, &UINetworkRequestWidget::sltSetProgressToFailed);

    /* Setup timer: */
    m_pTimer->setInterval(5000);
    connect(m_pTimer, &QTimer::timeout, this, &UINetworkRequestWidget::sltTimeIsOut);

    /* Setup main-layout: */
    const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2;
    const int iT = qApp->style()->pixelMetric(QStyle::PM_LayoutTopMargin) / 2;
    const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin) / 2;
    const int iB = qApp->style()->pixelMetric(QStyle::PM_LayoutBottomMargin) / 2;
    m_pMainLayout->setContentsMargins(iL, iT, iR, iB);

    /* Setup progress-bar: */
    m_pProgressBar->setRange(0, 0);
    m_pProgressBar->setMaximumHeight(16);

    /* Setup retry-button: */
    m_pRetryButton->setHidden(true);
    m_pRetryButton->removeBorder();
    m_pRetryButton->setFocusPolicy(Qt::NoFocus);
    m_pRetryButton->setIcon(UIIconPool::iconSet(":/refresh_16px.png"));
    connect(m_pRetryButton, &QIToolButton::clicked, this, &UINetworkRequestWidget::sigRetry);

    /* Setup cancel-button: */
    m_pCancelButton->removeBorder();
    m_pCancelButton->setFocusPolicy(Qt::NoFocus);
    m_pCancelButton->setIcon(UIIconPool::iconSet(":/cancel_16px.png"));
    connect(m_pCancelButton, &QIToolButton::clicked, this, &UINetworkRequestWidget::sigCancel);

    /* Setup error-label: */
    m_pErrorPane->setHidden(true);
    m_pErrorPane->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    /* Calculate required width: */
    int iMinimumWidth = pParent->minimumWidth();
    int iLeft, iTop, iRight, iBottom;
    /* Take into account content-widget layout margins: */
    m_pMainLayout->getContentsMargins(&iLeft, &iTop, &iRight, &iBottom);
    iMinimumWidth -= iLeft;
    iMinimumWidth -= iRight;
    /* Take into account this layout margins: */
    layout()->getContentsMargins(&iLeft, &iTop, &iRight, &iBottom);
    iMinimumWidth -= iLeft;
    iMinimumWidth -= iRight;
    /* Take into account parent layout margins: */
    QLayout *pParentLayout = qobject_cast<QMainWindow*>(parent())->centralWidget()->layout();
    pParentLayout->getContentsMargins(&iLeft, &iTop, &iRight, &iBottom);
    iMinimumWidth -= iLeft;
    iMinimumWidth -= iRight;
    /* Set minimum text width: */
    m_pErrorPane->setMinimumTextWidth(iMinimumWidth);

    /* Layout content: */
    m_pMainLayout->addWidget(m_pProgressBar, 0, 0);
    m_pMainLayout->addWidget(m_pRetryButton, 0, 1);
    m_pMainLayout->addWidget(m_pCancelButton, 0, 2);
    m_pMainLayout->addWidget(m_pErrorPane, 1, 0, 1, 3);

    /* Retranslate UI: */
    retranslateUi();
}

void UINetworkRequestWidget::sltSetProgress(qint64 iReceived, qint64 iTotal)
{
    /* Restart timer: */
    m_pTimer->start();

    /* Set current progress to passed: */
    m_pProgressBar->setRange(0, iTotal);
    m_pProgressBar->setValue(iReceived);
}

void UINetworkRequestWidget::sltSetProgressToStarted()
{
    /* Start timer: */
    m_pTimer->start();

    /* Set current progress to 'started': */
    m_pProgressBar->setRange(0, 1);
    m_pProgressBar->setValue(0);

    /* Hide 'retry' button: */
    m_pRetryButton->setHidden(true);

    /* Hide error label: */
    m_pErrorPane->setHidden(true);
    m_pErrorPane->setText(QString());
}

void UINetworkRequestWidget::sltSetProgressToFinished()
{
    /* Stop timer: */
    m_pTimer->stop();

    /* Set current progress to 'started': */
    m_pProgressBar->setRange(0, 1);
    m_pProgressBar->setValue(1);
}

void UINetworkRequestWidget::sltSetProgressToFailed(const QString &strError)
{
    /* Stop timer: */
    m_pTimer->stop();

    /* Set current progress to 'failed': */
    m_pProgressBar->setRange(0, 1);
    m_pProgressBar->setValue(1);

    /* Show 'retry' button: */
    m_pRetryButton->setHidden(false);

    /* Show error label: */
    m_pErrorPane->setHidden(false);
    m_pErrorPane->setText(composeErrorText(strError));
}

void UINetworkRequestWidget::sltTimeIsOut()
{
    /* Stop timer: */
    m_pTimer->stop();

    /* Set current progress to unknown: */
    m_pProgressBar->setRange(0, 0);
}

void UINetworkRequestWidget::retranslateUi()
{
    /* Get corresponding title: */
    const QString &strTitle = m_pNetworkRequest->description();

    /* Set popup title (default if missed): */
    setTitle(strTitle.isEmpty() ? UINetworkManagerDialog::tr("Network Operation") : strTitle);

    /* Translate retry button: */
    m_pRetryButton->setStatusTip(UINetworkManagerDialog::tr("Restart network operation"));

    /* Translate cancel button: */
    m_pCancelButton->setStatusTip(UINetworkManagerDialog::tr("Cancel network operation"));

    /* Translate error label: */
    if (m_pNetworkRequest->reply())
        m_pErrorPane->setText(composeErrorText(m_pNetworkRequest->reply()->errorString()));
}

/* static */
const QString UINetworkRequestWidget::composeErrorText(QString strErrorText)
{
    /* Null-string for null-string: */
    if (strErrorText.isEmpty())
        return QString();

    /* Try to find all the links in the error-message,
     * replace them with %increment if present: */
    QRegExp linkRegExp("[\\S]+[\\./][\\S]+");
    QStringList links;
    for (int i = 1; linkRegExp.indexIn(strErrorText) != -1; ++i)
    {
        links << linkRegExp.cap();
        strErrorText.replace(linkRegExp.cap(), QString("%%1").arg(i));
    }
    /* Return back all the links, just in bold: */
    if (!links.isEmpty())
        for (int i = 0; i < links.size(); ++i)
            strErrorText = strErrorText.arg(QString("<b>%1</b>").arg(links[i]));

    /// @todo NLS: Embed <br> directly into error header text.
    /* Prepend the error-message with <br> symbol: */
    strErrorText.prepend("<br>");

    /* Return final result: */
    return UINetworkManagerDialog::tr("The network operation failed with the following error: %1.").arg(strErrorText);
}


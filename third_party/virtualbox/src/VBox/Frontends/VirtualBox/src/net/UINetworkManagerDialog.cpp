/* $Id: UINetworkManagerDialog.cpp $ */
/** @file
 * VBox Qt GUI - UINetworkManagerDialog stuff implementation.
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
# include <QVBoxLayout>
# include <QLabel>
# include <QPushButton>
# include <QStatusBar>
# include <QKeyEvent>

/* Local includes: */
# include "UINetworkManagerDialog.h"
# include "UINetworkManager.h"
# include "UINetworkRequest.h"
# include "UINetworkRequestWidget.h"
# include "UINetworkCustomer.h"
# include "UIIconPool.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIModalWindowManager.h"
# include "QIDialogButtonBox.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


void UINetworkManagerDialog::showNormal()
{
    /* Show (restore if necessary): */
    QMainWindow::showNormal();

    /* Raise above the others: */
    raise();

    /* Activate: */
    activateWindow();
}

UINetworkManagerDialog::UINetworkManagerDialog()
{
    /* Apply window icons: */
    setWindowIcon(UIIconPool::iconSetFull(":/download_manager_32px.png", ":/download_manager_16px.png"));

    /* Do not count that window as important for application,
     * it will NOT be taken into account when other top-level windows will be closed: */
    setAttribute(Qt::WA_QuitOnClose, false);

    /* Set minimum width: */
    setMinimumWidth(500);

    /* Prepare central-widget: */
    setCentralWidget(new QWidget);

    /* Create main-layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(centralWidget());

    /* Create description-label: */
    m_pLabel = new QLabel(centralWidget());
    m_pLabel->setAlignment(Qt::AlignCenter);
    m_pLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    /* Create layout for network-request widgets: */
    m_pWidgetsLayout = new QVBoxLayout;

    /* Create button-box: */
    m_pButtonBox = new QIDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, centralWidget());
    connect(m_pButtonBox, &QIDialogButtonBox::rejected, this, &UINetworkManagerDialog::sltHandleCancelAllButtonPress);
    m_pButtonBox->setHidden(true);

    /* Layout content: */
    pMainLayout->addWidget(m_pLabel);
    pMainLayout->addLayout(m_pWidgetsLayout);
    pMainLayout->addStretch();
    pMainLayout->addWidget(m_pButtonBox);

    /* Create status-bar: */
    setStatusBar(new QStatusBar);

    /* Translate dialog: */
    retranslateUi();
}

void UINetworkManagerDialog::addNetworkRequestWidget(UINetworkRequest *pNetworkRequest)
{
    /* Make sure network-request is really exists: */
    AssertMsg(pNetworkRequest, ("Network-request doesn't exists!\n"));

    /* Create new network-request widget: */
    UINetworkRequestWidget *pNetworkRequestWidget = new UINetworkRequestWidget(this, pNetworkRequest);
    m_pWidgetsLayout->addWidget(pNetworkRequestWidget);
    m_widgets.insert(pNetworkRequest->uuid(), pNetworkRequestWidget);

    /* Hide label: */
    m_pLabel->hide();
    /* Show button-box: */
    m_pButtonBox->show();
    /* If customer made a force-call: */
    if (pNetworkRequest->customer()->isItForceCall())
    {
        /* Show dialog: */
        showNormal();
    }

    /* Prepare network-request widget's notifications for network-request: */
    connect(pNetworkRequestWidget, &UINetworkRequestWidget::sigRetry,  pNetworkRequest, &UINetworkRequest::sltRetry,  Qt::QueuedConnection);
    connect(pNetworkRequestWidget, &UINetworkRequestWidget::sigCancel, pNetworkRequest, &UINetworkRequest::sltCancel, Qt::QueuedConnection);
}

void UINetworkManagerDialog::removeNetworkRequestWidget(const QUuid &uuid)
{
    /* Make sure network-request widget still present: */
    AssertMsg(m_widgets.contains(uuid), ("Network-request widget already removed!\n"));

    /* Delete corresponding network-request widget: */
    delete m_widgets.value(uuid);
    m_widgets.remove(uuid);

    /* Check if dialog is empty: */
    if (m_widgets.isEmpty())
    {
        /* Show label: */
        m_pLabel->show();
        /* Hide button-box: */
        m_pButtonBox->hide();
        /* Let central-widget update its layout before being hidden: */
        QCoreApplication::sendPostedEvents(centralWidget(), QEvent::LayoutRequest);
        /* Hide dialog: */
        hide();
    }
}

void UINetworkManagerDialog::sltHandleCancelAllButtonPress()
{
    /* Ask if user wants to cancel all current network-requests: */
    if (msgCenter().confirmCancelingAllNetworkRequests())
        emit sigCancelNetworkRequests();
}

void UINetworkManagerDialog::retranslateUi()
{
    /* Set window caption: */
    setWindowTitle(tr("Network Operations Manager"));

    /* Set description-label text: */
    m_pLabel->setText(tr("There are no active network operations."));

    /* Set buttons-box text: */
    m_pButtonBox->button(QDialogButtonBox::Cancel)->setText(tr("&Cancel All"));
    m_pButtonBox->button(QDialogButtonBox::Cancel)->setStatusTip(tr("Cancel all active network operations"));
}

void UINetworkManagerDialog::showEvent(QShowEvent *pShowEvent)
{
    /* Resize to minimum size-hint: */
    resize(minimumSizeHint());

    /* Center according current main application window: */
    vboxGlobal().centerWidget(this, windowManager().mainWindowShown(), false);

    /* Pass event to the base-class: */
    QMainWindow::showEvent(pShowEvent);
}

void UINetworkManagerDialog::keyPressEvent(QKeyEvent *pKeyPressEvent)
{
    /* 'Escape' key used to close the dialog: */
    if (pKeyPressEvent->key() == Qt::Key_Escape)
    {
        close();
        return;
    }

    /* Pass event to the base-class: */
    QMainWindow::keyPressEvent(pKeyPressEvent);
}


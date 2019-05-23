/* $Id: UIGlobalSettingsPortForwardingDlg.cpp $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsPortForwardingDlg class implementation.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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

/* Qt includes: */
# include <QVBoxLayout>
# include <QPushButton>
# include <QTabWidget>

/* GUI includes: */
# include "UIGlobalSettingsPortForwardingDlg.h"
# include "UIIconPool.h"
# include "UIMessageCenter.h"
# include "QIDialogButtonBox.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGlobalSettingsPortForwardingDlg::UIGlobalSettingsPortForwardingDlg(QWidget *pParent,
                                                                     const UIPortForwardingDataList &ipv4rules,
                                                                     const UIPortForwardingDataList &ipv6rules)
    : QIWithRetranslateUI<QIDialog>(pParent)
    , m_pIPv4Table(0)
    , m_pIPv6Table(0)
    , m_pButtonBox(0)
{
    /* Set dialog icon: */
    setWindowIcon(UIIconPool::iconSetFull(":/nw_32px.png", ":/nw_16px.png"));

    /* Create layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        /* Create tab-widget: */
        m_pTabWidget = new QTabWidget;
        {
            /* Create table tabs: */
            m_pIPv4Table = new UIPortForwardingTable(ipv4rules, false, false);
            m_pIPv6Table = new UIPortForwardingTable(ipv6rules, true, false);
            /* Add widgets into tab-widget: */
            m_pTabWidget->addTab(m_pIPv4Table, QString());
            m_pTabWidget->addTab(m_pIPv6Table, QString());
        }
        /* Create button-box: */
        m_pButtonBox = new QIDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
        {
            /* Configure button-box: */
            connect(m_pButtonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
            connect(m_pButtonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
        }
        /* Add widgets into layout: */
        pMainLayout->addWidget(m_pTabWidget);
        pMainLayout->addWidget(m_pButtonBox);
    }

    /* Retranslate dialog: */
    retranslateUi();
}

const UIPortForwardingDataList UIGlobalSettingsPortForwardingDlg::ipv4rules() const
{
    return m_pIPv4Table->rules();
}

const UIPortForwardingDataList UIGlobalSettingsPortForwardingDlg::ipv6rules() const
{
    return m_pIPv6Table->rules();
}

void UIGlobalSettingsPortForwardingDlg::accept()
{
    /* Make sure both tables have their data committed: */
    m_pIPv4Table->makeSureEditorDataCommitted();
    m_pIPv6Table->makeSureEditorDataCommitted();
    /* Validate table: */
    bool fPassed = m_pIPv4Table->validate() && m_pIPv6Table->validate();
    if (!fPassed)
        return;
    /* Call to base-class: */
    QIWithRetranslateUI<QIDialog>::accept();
}

void UIGlobalSettingsPortForwardingDlg::reject()
{
    /* Ask user to discard table changes if necessary: */
    if (   (m_pIPv4Table->isChanged() || m_pIPv6Table->isChanged())
        && !msgCenter().confirmCancelingPortForwardingDialog(window()))
        return;
    /* Call to base-class: */
    QIWithRetranslateUI<QIDialog>::reject();
}

void UIGlobalSettingsPortForwardingDlg::retranslateUi()
{
    /* Set window title: */
    setWindowTitle(tr("Port Forwarding Rules"));
    /* Set tab-widget tab names: */
    m_pTabWidget->setTabText(0, tr("IPv4"));
    m_pTabWidget->setTabText(1, tr("IPv6"));
}


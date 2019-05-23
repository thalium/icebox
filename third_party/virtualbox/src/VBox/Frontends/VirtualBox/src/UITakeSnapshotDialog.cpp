/* $Id: UITakeSnapshotDialog.cpp $ */
/** @file
 * VBox Qt GUI - UITakeSnapshotDialog class implementation.
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

/* Qt includes: */
# include <QGridLayout>
# include <QLabel>
# include <QLineEdit>
# include <QPushButton>
# include <QStyle>

/* GUI includes: */
# include "QIDialogButtonBox.h"
# include "QILabel.h"
# include "UIDesktopWidgetWatchdog.h"
# include "UIMessageCenter.h"
# include "UITakeSnapshotDialog.h"
# include "VBoxUtils.h"

/* COM includes: */
# include "COMEnums.h"
# include "CMachine.h"
# include "CMedium.h"
# include "CMediumAttachment.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UITakeSnapshotDialog::UITakeSnapshotDialog(QWidget *pParent, const CMachine &comMachine)
    : QIWithRetranslateUI<QIDialog>(pParent)
    , m_comMachine(comMachine)
    , m_cImmutableMediums(0)
    , m_pLabelIcon(0)
    , m_pLabelName(0), m_pEditorName(0)
    , m_pLabelDescription(0), m_pEditorDescription(0)
    , m_pLabelInfo(0)
    , m_pButtonBox(0)
{
    /* Prepare: */
    prepare();
}

void UITakeSnapshotDialog::setPixmap(const QPixmap &pixmap)
{
    m_pLabelIcon->setPixmap(pixmap);
}

void UITakeSnapshotDialog::setName(const QString &strName)
{
    m_pEditorName->setText(strName);
}

QString UITakeSnapshotDialog::name() const
{
    return m_pEditorName->text();
}

QString UITakeSnapshotDialog::description() const
{
    return m_pEditorDescription->toPlainText();
}

void UITakeSnapshotDialog::retranslateUi()
{
    /* Translate: */
    setWindowTitle(tr("Take Snapshot of Virtual Machine"));
    m_pLabelName->setText(tr("Snapshot &Name"));
    m_pLabelDescription->setText(tr("Snapshot &Description"));
    m_pLabelInfo->setText(tr("Warning: You are taking a snapshot of a running machine which has %n immutable image(s) "
                             "attached to it. As long as you are working from this snapshot the immutable image(s) "
                             "will not be reset to avoid loss of data.", "", m_cImmutableMediums));
}

void UITakeSnapshotDialog::sltHandleNameChanged(const QString &strName)
{
    /* Update button state depending on snapshot name value: */
    m_pButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!strName.trimmed().isEmpty());
}

void UITakeSnapshotDialog::prepare()
{
    /* Create layout: */
    QGridLayout *pLayout = new QGridLayout(this);
    AssertPtrReturnVoid(pLayout);
    {
        /* Configure layout: */
#ifdef VBOX_WS_MAC
        pLayout->setSpacing(20);
        pLayout->setContentsMargins(40, 20, 40, 20);
#else
        pLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) * 2);
#endif

        /* Create sub-layout: */
        QVBoxLayout *pSubLayout1 = new QVBoxLayout;
        AssertPtrReturnVoid(pSubLayout1);
        {
            /* Create icon label: */
            m_pLabelIcon = new QLabel;
            AssertPtrReturnVoid(m_pLabelIcon);
            {
                /* Configure label: */
                m_pLabelIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                /* Add into layout: */
                pSubLayout1->addWidget(m_pLabelIcon);
            }

            /* Add stretch: */
            pSubLayout1->addStretch();

            /* Add into layout: */
            pLayout->addLayout(pSubLayout1, 0, 0, 2, 1);
        }

        /* Create sub-layout 2: */
        QVBoxLayout *pSubLayout2 = new QVBoxLayout;
        AssertPtrReturnVoid(pSubLayout2);
        {
            /* Configure layout: */
#ifdef VBOX_WS_MAC
            pSubLayout2->setSpacing(5);
#else
            pSubLayout2->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 2);
#endif

            /* Create name label: */
            m_pLabelName = new QLabel;
            AssertPtrReturnVoid(m_pLabelName);
            {
                /* Add into layout: */
                pSubLayout2->addWidget(m_pLabelName);
            }

            /* Create name editor: */
            m_pEditorName = new QLineEdit;
            AssertPtrReturnVoid(m_pEditorName);
            {
                /* Configure editor: */
                m_pLabelName->setBuddy(m_pEditorName);
                connect(m_pEditorName, &QLineEdit::textChanged,
                        this, &UITakeSnapshotDialog::sltHandleNameChanged);

                /* Add into layout: */
                pSubLayout2->addWidget(m_pEditorName);
            }

            /* Add into layout: */
            pLayout->addLayout(pSubLayout2, 0, 1);
        }

        /* Create sub-layout 3: */
        QVBoxLayout *pSubLayout3 = new QVBoxLayout;
        AssertPtrReturnVoid(pSubLayout3);
        {
            /* Configure layout: */
#ifdef VBOX_WS_MAC
            pSubLayout3->setSpacing(5);
#else
            pSubLayout3->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 2);
#endif

            /* Create description label: */
            m_pLabelDescription = new QLabel;
            AssertPtrReturnVoid(m_pLabelDescription);
            {
                /* Add into layout: */
                pSubLayout3->addWidget(m_pLabelDescription);
            }

            /* Create description editor: */
            m_pEditorDescription = new QTextEdit;
            AssertPtrReturnVoid(m_pEditorDescription);
            {
                /* Configure editor: */
                m_pLabelDescription->setBuddy(m_pEditorDescription);

                /* Add into layout: */
                pSubLayout3->addWidget(m_pEditorDescription);
            }

            /* Add into layout: */
            pLayout->addLayout(pSubLayout3, 1, 1);
        }

        /* Create information label: */
        m_pLabelInfo = new QILabel;
        AssertPtrReturnVoid(m_pLabelInfo);
        {
            /* Configure label: */
            m_pLabelInfo->setWordWrap(true);
            m_pLabelInfo->useSizeHintForWidth(400);

            /* Calculate the amount of immutable attachments: */
            if (m_comMachine.GetState() == KMachineState_Paused)
            {
                foreach (const CMediumAttachment &comAttachment, m_comMachine.GetMediumAttachments())
                {
                    CMedium comMedium = comAttachment.GetMedium();
                    if (   !comMedium.isNull()
                        && !comMedium.GetParent().isNull()
                        && comMedium.GetBase().GetType() == KMediumType_Immutable)
                        ++m_cImmutableMediums;
                }
            }
            /* Hide if machine have no immutable attachments: */
            if (!m_cImmutableMediums)
                m_pLabelInfo->setHidden(true);

            /* Add into layout: */
            pLayout->addWidget(m_pLabelInfo, 2, 0, 1, 2);
        }

        /* Create button-box: */
        m_pButtonBox = new QIDialogButtonBox;
        AssertPtrReturnVoid(m_pButtonBox);
        {
            /* Configure button-box: */
            m_pButtonBox->setStandardButtons(  QDialogButtonBox::Ok
                                             | QDialogButtonBox::Cancel
                                             | QDialogButtonBox::Help);
            connect(m_pButtonBox, &QIDialogButtonBox::accepted,
                    this, &UITakeSnapshotDialog::accept);
            connect(m_pButtonBox, &QIDialogButtonBox::rejected,
                    this, &UITakeSnapshotDialog::reject);
            connect(m_pButtonBox, &QIDialogButtonBox::helpRequested,
                    &msgCenter(), &UIMessageCenter::sltShowHelpHelpDialog);

            /* Add into layout: */
            pLayout->addWidget(m_pButtonBox, 3, 0, 1, 2);
        }
    }

    /* Apply language settings: */
    retranslateUi();

    /* Invent minimum size: */
    QSize minimumSize;
    const int iHostScreen = gpDesktop->screenNumber(parentWidget());
    if (iHostScreen >= 0 && iHostScreen < gpDesktop->screenCount())
    {
        /* On the basis of current host-screen geometry if possible: */
        const QRect screenGeometry = gpDesktop->screenGeometry(iHostScreen);
        if (screenGeometry.isValid())
            minimumSize = screenGeometry.size() / 4;
    }
    /* Fallback to default size if we failed: */
    if (minimumSize.isNull())
        minimumSize = QSize(800, 600);
    /* Resize to initial size: */
    setMinimumSize(minimumSize);
}


/* $Id: UIWizardCloneVDPageBasic1.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic1 class implementation.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
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
# include <QHBoxLayout>

/* Local includes: */
# include "UIWizardCloneVDPageBasic1.h"
# include "UIWizardCloneVD.h"
# include "UIIconPool.h"
# include "QIRichTextLabel.h"
# include "VBoxMediaComboBox.h"
# include "QIToolButton.h"
# include "UIMedium.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVDPage1::UIWizardCloneVDPage1()
{
}

void UIWizardCloneVDPage1::onHandleOpenSourceDiskClick()
{
    /* Get source virtual-disk using file-open dialog: */
    QString strMediumId = vboxGlobal().openMediumWithFileOpenDialog(UIMediumType_HardDisk, thisImp());
    if (!strMediumId.isNull())
    {
        /* Update medium-combo if necessary: */
        m_pSourceDiskSelector->setCurrentItem(strMediumId);
        /* Focus on virtual-disk combo: */
        m_pSourceDiskSelector->setFocus();
    }
}

CMedium UIWizardCloneVDPage1::sourceVirtualDisk() const
{
    return vboxGlobal().medium(m_pSourceDiskSelector->id()).medium();
}

void UIWizardCloneVDPage1::setSourceVirtualDisk(const CMedium &sourceVirtualDisk)
{
    m_pSourceDiskSelector->setCurrentItem(sourceVirtualDisk.GetId());
}

UIWizardCloneVDPageBasic1::UIWizardCloneVDPageBasic1(const CMedium &sourceVirtualDisk)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QHBoxLayout *pSourceDiskLayout = new QHBoxLayout;
        {
            m_pSourceDiskSelector = new VBoxMediaComboBox(this);
            {
                m_pSourceDiskSelector->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
                m_pSourceDiskSelector->setType(UIMediumType_HardDisk);
                m_pSourceDiskSelector->setCurrentItem(sourceVirtualDisk.GetId());
                m_pSourceDiskSelector->repopulate();
            }
            m_pSourceDiskOpenButton = new QIToolButton(this);
            {
                m_pSourceDiskOpenButton->setAutoRaise(true);
                m_pSourceDiskOpenButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", ":/select_file_disabled_16px.png"));
            }
            pSourceDiskLayout->addWidget(m_pSourceDiskSelector);
            pSourceDiskLayout->addWidget(m_pSourceDiskOpenButton);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pSourceDiskLayout);
        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pSourceDiskSelector, SIGNAL(currentIndexChanged(int)), this, SIGNAL(completeChanged()));
    connect(m_pSourceDiskOpenButton, SIGNAL(clicked()), this, SLOT(sltHandleOpenSourceDiskClick()));

    /* Register classes: */
    qRegisterMetaType<CMedium>();
    /* Register fields: */
    registerField("sourceVirtualDisk", this, "sourceVirtualDisk");
}

void UIWizardCloneVDPageBasic1::sltHandleOpenSourceDiskClick()
{
    /* Call to base-class: */
    onHandleOpenSourceDiskClick();

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardCloneVDPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("Hard disk to copy"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardCloneVD::tr("<p>Please select the virtual hard disk file that you would like to copy "
                                          "if it is not already selected. You can either choose one from the list "
                                          "or use the folder icon beside the list to select one.</p>"));
    m_pSourceDiskOpenButton->setToolTip(UIWizardCloneVD::tr("Choose a virtual hard disk file to copy..."));
}

void UIWizardCloneVDPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardCloneVDPageBasic1::isComplete() const
{
    /* Make sure source virtual-disk feats the rules: */
    return !sourceVirtualDisk().isNull();
}


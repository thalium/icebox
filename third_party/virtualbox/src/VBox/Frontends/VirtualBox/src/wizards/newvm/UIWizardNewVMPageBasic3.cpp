/* $Id: UIWizardNewVMPageBasic3.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageBasic3 class implementation.
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
# include <QMetaType>
# include <QVBoxLayout>
# include <QGridLayout>
# include <QRadioButton>

/* Local includes: */
# include "UIWizardNewVMPageBasic3.h"
# include "UIWizardNewVM.h"
# include "UIMessageCenter.h"
# include "UIIconPool.h"
# include "VBoxMediaComboBox.h"
# include "QIToolButton.h"
# include "UIWizardNewVD.h"
# include "QIRichTextLabel.h"
# include "UIMedium.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

UIWizardNewVMPage3::UIWizardNewVMPage3()
    : m_fRecommendedNoDisk(false)
{
}

void UIWizardNewVMPage3::updateVirtualDiskSource()
{
    /* Enable/disable controls: */
    m_pDiskSelector->setEnabled(m_pDiskPresent->isChecked());
    m_pVMMButton->setEnabled(m_pDiskPresent->isChecked());

    /* Fetch filed values: */
    if (m_pDiskSkip->isChecked())
    {
        m_strVirtualDiskId = QString();
        m_strVirtualDiskName = QString();
        m_strVirtualDiskLocation = QString();
    }
    else if (m_pDiskPresent->isChecked())
    {
        m_strVirtualDiskId = m_pDiskSelector->id();
        m_strVirtualDiskName = m_pDiskSelector->currentText();
        m_strVirtualDiskLocation = m_pDiskSelector->location();
    }
}

void UIWizardNewVMPage3::getWithFileOpenDialog()
{
    /* Get opened medium id: */
    QString strMediumId = vboxGlobal().openMediumWithFileOpenDialog(UIMediumType_HardDisk, thisImp());
    if (!strMediumId.isNull())
    {
        /* Update medium-combo if necessary: */
        m_pDiskSelector->setCurrentItem(strMediumId);
        /* Update hard disk source: */
        updateVirtualDiskSource();
        /* Focus on hard disk combo: */
        m_pDiskSelector->setFocus();
    }
}

bool UIWizardNewVMPage3::getWithNewVirtualDiskWizard()
{
    /* Create New Virtual Hard Drive wizard: */
    UISafePointerWizardNewVD pWizard = new UIWizardNewVD(thisImp(),
                                                         fieldImp("machineBaseName").toString(),
                                                         fieldImp("machineFolder").toString(),
                                                         fieldImp("type").value<CGuestOSType>().GetRecommendedHDD(),
                                                         wizardImp()->mode());
    pWizard->prepare();
    bool fResult = false;
    if (pWizard->exec() == QDialog::Accepted)
    {
        fResult = true;
        m_virtualDisk = pWizard->virtualDisk();
        m_pDiskSelector->setCurrentItem(m_virtualDisk.GetId());
        m_pDiskPresent->click();
    }
    if (pWizard)
        delete pWizard;
    return fResult;
}

void UIWizardNewVMPage3::ensureNewVirtualDiskDeleted()
{
    /* Make sure virtual-disk valid: */
    if (m_virtualDisk.isNull())
        return;

    /* Remember virtual-disk attributes: */
    QString strMediumID = m_virtualDisk.GetId();
    QString strLocation = m_virtualDisk.GetLocation();
    /* Prepare delete storage progress: */
    CProgress progress = m_virtualDisk.DeleteStorage();
    if (m_virtualDisk.isOk())
    {
        /* Show delete storage progress: */
        msgCenter().showModalProgressDialog(progress, thisImp()->windowTitle(), ":/progress_media_delete_90px.png", thisImp());
        if (!progress.isOk() || progress.GetResultCode() != 0)
            msgCenter().cannotDeleteHardDiskStorage(progress, strLocation, thisImp());
    }
    else
        msgCenter().cannotDeleteHardDiskStorage(m_virtualDisk, strLocation, thisImp());

    /* Inform VBoxGlobal about it: */
    vboxGlobal().deleteMedium(strMediumID);

    /* Detach virtual-disk anyway: */
    m_virtualDisk.detach();
}

UIWizardNewVMPageBasic3::UIWizardNewVMPageBasic3()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QGridLayout *pDiskLayout = new QGridLayout;
        {
            m_pDiskSkip = new QRadioButton(this);
            m_pDiskCreate = new QRadioButton(this);
            m_pDiskPresent = new QRadioButton(this);
            QStyleOptionButton options;
            options.initFrom(m_pDiskPresent);
            int iWidth = m_pDiskPresent->style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &options, m_pDiskPresent);
            pDiskLayout->setColumnMinimumWidth(0, iWidth);
            m_pDiskSelector = new VBoxMediaComboBox(this);
            {
                m_pDiskSelector->setType(UIMediumType_HardDisk);
                m_pDiskSelector->repopulate();
            }
            m_pVMMButton = new QIToolButton(this);
            {
                m_pVMMButton->setAutoRaise(true);
                m_pVMMButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", ":/select_file_disabled_16px.png"));
            }
            pDiskLayout->addWidget(m_pDiskSkip, 0, 0, 1, 3);
            pDiskLayout->addWidget(m_pDiskCreate, 1, 0, 1, 3);
            pDiskLayout->addWidget(m_pDiskPresent, 2, 0, 1, 3);
            pDiskLayout->addWidget(m_pDiskSelector, 3, 1);
            pDiskLayout->addWidget(m_pVMMButton, 3, 2);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pDiskLayout);
        pMainLayout->addStretch();
        updateVirtualDiskSource();
    }

    /* Setup connections: */
    connect(m_pDiskSkip, SIGNAL(toggled(bool)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pDiskCreate, SIGNAL(toggled(bool)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pDiskPresent, SIGNAL(toggled(bool)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pDiskSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pVMMButton, SIGNAL(clicked()), this, SLOT(sltGetWithFileOpenDialog()));

    /* Register classes: */
    qRegisterMetaType<CMedium>();
    /* Register fields: */
    registerField("virtualDisk", this, "virtualDisk");
    registerField("virtualDiskId", this, "virtualDiskId");
    registerField("virtualDiskName", this, "virtualDiskName");
    registerField("virtualDiskLocation", this, "virtualDiskLocation");
}

void UIWizardNewVMPageBasic3::sltVirtualDiskSourceChanged()
{
    /* Call to base-class: */
    updateVirtualDiskSource();

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardNewVMPageBasic3::sltGetWithFileOpenDialog()
{
    /* Call to base-class: */
    getWithFileOpenDialog();
}

void UIWizardNewVMPageBasic3::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardNewVM::tr("Hard disk"));

    /* Translate widgets: */
    QString strRecommendedHDD = field("type").value<CGuestOSType>().isNull() ? QString() :
                                VBoxGlobal::formatSize(field("type").value<CGuestOSType>().GetRecommendedHDD());
    m_pLabel->setText(UIWizardNewVM::tr("<p>If you wish you can add a virtual hard disk to the new machine. "
                                        "You can either create a new hard disk file or select one from the list "
                                        "or from another location using the folder icon.</p>"
                                        "<p>If you need a more complex storage set-up you can skip this step "
                                        "and make the changes to the machine settings once the machine is created.</p>"
                                        "<p>The recommended size of the hard disk is <b>%1</b>.</p>")
                                        .arg(strRecommendedHDD));
    m_pDiskSkip->setText(UIWizardNewVM::tr("&Do not add a virtual hard disk"));
    m_pDiskCreate->setText(UIWizardNewVM::tr("&Create a virtual hard disk now"));
    m_pDiskPresent->setText(UIWizardNewVM::tr("&Use an existing virtual hard disk file"));
    m_pVMMButton->setToolTip(UIWizardNewVM::tr("Choose a virtual hard disk file..."));
}

void UIWizardNewVMPageBasic3::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Prepare initial choice: */
    if (field("type").value<CGuestOSType>().GetRecommendedHDD() != 0)
    {
        m_pDiskCreate->setFocus();
        m_pDiskCreate->setChecked(true);
        m_fRecommendedNoDisk = false;
    }
    else
    {
        m_pDiskSkip->setFocus();
        m_pDiskSkip->setChecked(true);
        m_fRecommendedNoDisk = true;
    }
    m_pDiskSelector->setCurrentIndex(0);
}

void UIWizardNewVMPageBasic3::cleanupPage()
{
    /* Call to base-class: */
    ensureNewVirtualDiskDeleted();
    UIWizardPage::cleanupPage();
}

bool UIWizardNewVMPageBasic3::isComplete() const
{
    /* Make sure 'virtualDisk' field feats the rules: */
    return m_pDiskSkip->isChecked() ||
           !m_pDiskPresent->isChecked() ||
           !vboxGlobal().medium(m_pDiskSelector->id()).isNull();
}

bool UIWizardNewVMPageBasic3::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Ensure unused virtual-disk is deleted: */
    if (m_pDiskSkip->isChecked() || m_pDiskCreate->isChecked() || (!m_virtualDisk.isNull() && m_strVirtualDiskId != m_virtualDisk.GetId()))
        ensureNewVirtualDiskDeleted();

    if (m_pDiskSkip->isChecked())
    {
        /* Ask user about disk-less machine unless that's the recommendation: */
        if (!m_fRecommendedNoDisk)
            fResult = msgCenter().confirmHardDisklessMachine(thisImp());
    }
    else if (m_pDiskCreate->isChecked())
    {
        /* Show the New Virtual Hard Drive wizard: */
        fResult = getWithNewVirtualDiskWizard();
    }

    if (fResult)
    {
        /* Lock finish button: */
        startProcessing();

        /* Try to create VM: */
        fResult = qobject_cast<UIWizardNewVM*>(wizard())->createVM();

        /* Unlock finish button: */
        endProcessing();
    }

    /* Return result: */
    return fResult;
}


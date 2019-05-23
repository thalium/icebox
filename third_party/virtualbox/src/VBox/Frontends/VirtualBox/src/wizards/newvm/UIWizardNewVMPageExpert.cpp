/* $Id: UIWizardNewVMPageExpert.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageExpert class implementation.
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
# include <QVBoxLayout>
# include <QHBoxLayout>
# include <QGroupBox>
# include <QGridLayout>
# include <QSpacerItem>
# include <QLineEdit>
# include <QLabel>
# include <QSpinBox>
# include <QRadioButton>

/* Local includes: */
# include "UIWizardNewVMPageExpert.h"
# include "UIWizardNewVM.h"
# include "UIIconPool.h"
# include "UINameAndSystemEditor.h"
# include "VBoxGuestRAMSlider.h"
# include "VBoxMediaComboBox.h"
# include "QIToolButton.h"
# include "UIMedium.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardNewVMPageExpert::UIWizardNewVMPageExpert(const QString &strGroup)
    : UIWizardNewVMPage1(strGroup)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pNameAndSystemCnt = new QGroupBox(this);
        {
            m_pNameAndSystemCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QHBoxLayout *pNameAndSystemCntLayout = new QHBoxLayout(m_pNameAndSystemCnt);
            {
                m_pNameAndSystemEditor = new UINameAndSystemEditor(m_pNameAndSystemCnt);
                pNameAndSystemCntLayout->addWidget(m_pNameAndSystemEditor);
            }
        }
        m_pMemoryCnt = new QGroupBox(this);
        {
            m_pMemoryCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QGridLayout *pMemoryCntLayout = new QGridLayout(m_pMemoryCnt);
            {
                m_pRamSlider = new VBoxGuestRAMSlider(m_pMemoryCnt);
                {
                    m_pRamSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                    m_pRamSlider->setOrientation(Qt::Horizontal);
                    m_pRamSlider->setValue(m_pNameAndSystemEditor->type().GetRecommendedRAM());
                }
                m_pRamEditor = new QSpinBox(m_pMemoryCnt);
                {
                    m_pRamEditor->setMinimum(m_pRamSlider->minRAM());
                    m_pRamEditor->setMaximum(m_pRamSlider->maxRAM());
                    vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pRamEditor, 5);
                }
                m_pRamUnits = new QLabel(m_pMemoryCnt);
                {
                    m_pRamUnits->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                }
                m_pRamMin = new QLabel(m_pMemoryCnt);
                {
                    m_pRamMin->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                }
                m_pRamMax = new QLabel(m_pMemoryCnt);
                {
                    m_pRamMax->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                }
                pMemoryCntLayout->addWidget(m_pRamSlider, 0, 0, 1, 3);
                pMemoryCntLayout->addWidget(m_pRamEditor, 0, 3);
                pMemoryCntLayout->addWidget(m_pRamUnits, 0, 4);
                pMemoryCntLayout->addWidget(m_pRamMin, 1, 0);
                pMemoryCntLayout->setColumnStretch(1, 1);
                pMemoryCntLayout->addWidget(m_pRamMax, 1, 2);
            }
        }
        m_pDiskCnt = new QGroupBox(this);
        {
            m_pDiskCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QGridLayout *pDiskCntLayout = new QGridLayout(m_pDiskCnt);
            {
                m_pDiskSkip = new QRadioButton(m_pDiskCnt);
                m_pDiskCreate = new QRadioButton(m_pDiskCnt);
                {
                    m_pDiskCreate->setChecked(true);
                }
                m_pDiskPresent = new QRadioButton(m_pDiskCnt);
                QStyleOptionButton options;
                options.initFrom(m_pDiskPresent);
                int iWidth = m_pDiskPresent->style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &options, m_pDiskPresent);
                pDiskCntLayout->setColumnMinimumWidth(0, iWidth);
                m_pDiskSelector = new VBoxMediaComboBox(m_pDiskCnt);
                {
                    m_pDiskSelector->setType(UIMediumType_HardDisk);
                    m_pDiskSelector->repopulate();
                }
                m_pVMMButton = new QIToolButton(m_pDiskCnt);
                {
                    m_pVMMButton->setAutoRaise(true);
                    m_pVMMButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", ":/select_file_disabled_16px.png"));
                }
                pDiskCntLayout->addWidget(m_pDiskSkip, 0, 0, 1, 3);
                pDiskCntLayout->addWidget(m_pDiskCreate, 1, 0, 1, 3);
                pDiskCntLayout->addWidget(m_pDiskPresent, 2, 0, 1, 3);
                pDiskCntLayout->addWidget(m_pDiskSelector, 3, 1);
                pDiskCntLayout->addWidget(m_pVMMButton, 3, 2);
            }
        }
        pMainLayout->addWidget(m_pNameAndSystemCnt);
        pMainLayout->addWidget(m_pMemoryCnt);
        pMainLayout->addWidget(m_pDiskCnt);
        pMainLayout->addStretch();
        updateVirtualDiskSource();
    }

    /* Setup connections: */
    connect(m_pNameAndSystemEditor, SIGNAL(sigNameChanged(const QString &)), this, SLOT(sltNameChanged(const QString &)));
    connect(m_pNameAndSystemEditor, SIGNAL(sigOsTypeChanged()), this, SLOT(sltOsTypeChanged()));
    connect(m_pRamSlider, SIGNAL(valueChanged(int)), this, SLOT(sltRamSliderValueChanged()));
    connect(m_pRamEditor, SIGNAL(valueChanged(int)), this, SLOT(sltRamEditorValueChanged()));
    connect(m_pDiskSkip, SIGNAL(toggled(bool)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pDiskCreate, SIGNAL(toggled(bool)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pDiskPresent, SIGNAL(toggled(bool)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pDiskSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(sltVirtualDiskSourceChanged()));
    connect(m_pVMMButton, SIGNAL(clicked()), this, SLOT(sltGetWithFileOpenDialog()));

    /* Register classes: */
    qRegisterMetaType<CMedium>();
    /* Register fields: */
    registerField("name*", m_pNameAndSystemEditor, "name", SIGNAL(sigNameChanged(const QString &)));
    registerField("type", m_pNameAndSystemEditor, "type", SIGNAL(sigOsTypeChanged()));
    registerField("machineFolder", this, "machineFolder");
    registerField("machineBaseName", this, "machineBaseName");
    registerField("ram", m_pRamSlider, "value", SIGNAL(valueChanged(int)));
    registerField("virtualDisk", this, "virtualDisk");
    registerField("virtualDiskId", this, "virtualDiskId");
    registerField("virtualDiskLocation", this, "virtualDiskLocation");
}

void UIWizardNewVMPageExpert::sltNameChanged(const QString &strNewText)
{
    /* Call to base-class: */
    onNameChanged(strNewText);

    /* Fetch recommended RAM value: */
    CGuestOSType type = m_pNameAndSystemEditor->type();
    m_pRamSlider->setValue(type.GetRecommendedRAM());
    m_pRamEditor->setValue(type.GetRecommendedRAM());

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardNewVMPageExpert::sltOsTypeChanged()
{
    /* Call to base-class: */
    onOsTypeChanged();

    /* Fetch recommended RAM value: */
    CGuestOSType type = m_pNameAndSystemEditor->type();
    m_pRamSlider->setValue(type.GetRecommendedRAM());
    m_pRamEditor->setValue(type.GetRecommendedRAM());

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardNewVMPageExpert::sltRamSliderValueChanged()
{
    /* Call to base-class: */
    onRamSliderValueChanged();

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardNewVMPageExpert::sltRamEditorValueChanged()
{
    /* Call to base-class: */
    onRamEditorValueChanged();

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardNewVMPageExpert::sltVirtualDiskSourceChanged()
{
    /* Call to base-class: */
    updateVirtualDiskSource();

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardNewVMPageExpert::sltGetWithFileOpenDialog()
{
    /* Call to base-class: */
    getWithFileOpenDialog();
}

void UIWizardNewVMPageExpert::retranslateUi()
{
    /* Translate widgets: */
    m_pNameAndSystemCnt->setTitle(UIWizardNewVM::tr("Name and operating system"));
    m_pMemoryCnt->setTitle(UIWizardNewVM::tr("&Memory size"));
    m_pRamUnits->setText(VBoxGlobal::tr("MB", "size suffix MBytes=1024 KBytes"));
    m_pRamMin->setText(QString("%1 %2").arg(m_pRamSlider->minRAM()).arg(VBoxGlobal::tr("MB", "size suffix MBytes=1024 KBytes")));
    m_pRamMax->setText(QString("%1 %2").arg(m_pRamSlider->maxRAM()).arg(VBoxGlobal::tr("MB", "size suffix MBytes=1024 KBytes")));
    m_pDiskCnt->setTitle(UIWizardNewVM::tr("Hard disk"));
    m_pDiskSkip->setText(UIWizardNewVM::tr("&Do not add a virtual hard disk"));
    m_pDiskCreate->setText(UIWizardNewVM::tr("&Create a virtual hard disk now"));
    m_pDiskPresent->setText(UIWizardNewVM::tr("&Use an existing virtual hard disk file"));
    m_pVMMButton->setToolTip(UIWizardNewVM::tr("Choose a virtual hard disk file..."));
}

void UIWizardNewVMPageExpert::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

void UIWizardNewVMPageExpert::cleanupPage()
{
    /* Call to base-class: */
    ensureNewVirtualDiskDeleted();
    cleanupMachineFolder();
}

bool UIWizardNewVMPageExpert::isComplete() const
{
    /* Make sure mandatory fields are complete,
     * 'ram' field feats the bounds,
     * 'virtualDisk' field feats the rules: */
    return UIWizardPage::isComplete() &&
           (m_pRamSlider->value() >= qMax(1, (int)m_pRamSlider->minRAM()) && m_pRamSlider->value() <= (int)m_pRamSlider->maxRAM()) &&
           (m_pDiskSkip->isChecked() || !m_pDiskPresent->isChecked() || !vboxGlobal().medium(m_pDiskSelector->id()).isNull());
}

bool UIWizardNewVMPageExpert::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to create machine folder: */
    if (fResult)
        fResult = createMachineFolder();

    /* Try to assign boot virtual-disk: */
    if (fResult)
    {
        /* Ensure there is no virtual-disk created yet: */
        Assert(m_virtualDisk.isNull());
        if (fResult)
        {
            if (m_pDiskCreate->isChecked())
            {
                /* Show the New Virtual Hard Drive wizard if necessary: */
                fResult = getWithNewVirtualDiskWizard();
            }
        }
    }

    /* Try to create VM: */
    if (fResult)
        fResult = qobject_cast<UIWizardNewVM*>(wizard())->createVM();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}


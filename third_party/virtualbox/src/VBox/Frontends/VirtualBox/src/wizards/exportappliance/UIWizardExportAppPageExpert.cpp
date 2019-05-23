/* $Id: UIWizardExportAppPageExpert.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageExpert class implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
# include <QGridLayout>
# include <QListWidget>
# include <QGroupBox>
# include <QRadioButton>
# include <QLineEdit>
# include <QLabel>
# include <QCheckBox>
# include <QGroupBox>

/* Local includes: */
# include "UIWizardExportAppPageExpert.h"
# include "UIWizardExportApp.h"
# include "UIWizardExportAppDefs.h"
# include "VBoxGlobal.h"
# include "UIEmptyFilePathSelector.h"
# include "UIApplianceExportEditorWidget.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardExportAppPageExpert::UIWizardExportAppPageExpert(const QStringList &selectedVMNames)
{
    /* Create widgets: */
    QGridLayout *pMainLayout = new QGridLayout(this);
    {
        m_pSelectorCnt = new QGroupBox(this);
        {
            QVBoxLayout *pSelectorCntLayout = new QVBoxLayout(m_pSelectorCnt);
            {
                m_pVMSelector = new QListWidget(m_pSelectorCnt);
                {
                    m_pVMSelector->setAlternatingRowColors(true);
                    m_pVMSelector->setSelectionMode(QAbstractItemView::ExtendedSelection);
                }
                pSelectorCntLayout->addWidget(m_pVMSelector);
            }
        }
        m_pApplianceCnt = new QGroupBox(this);
        {
            QVBoxLayout *pApplianceCntLayout = new QVBoxLayout(m_pApplianceCnt);
            {
                m_pApplianceWidget = new UIApplianceExportEditorWidget(m_pApplianceCnt);
                {
                    m_pApplianceWidget->setMinimumHeight(250);
                    m_pApplianceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
                }
                pApplianceCntLayout->addWidget(m_pApplianceWidget);
            }
        }
        m_pTypeCnt = new QGroupBox(this);
        {
            m_pTypeCnt->hide();
            QVBoxLayout *pTypeCntLayout = new QVBoxLayout(m_pTypeCnt);
            {
                m_pTypeLocalFilesystem = new QRadioButton(m_pTypeCnt);
                m_pTypeSunCloud = new QRadioButton(m_pTypeCnt);
                m_pTypeSimpleStorageSystem = new QRadioButton(m_pTypeCnt);
                pTypeCntLayout->addWidget(m_pTypeLocalFilesystem);
                pTypeCntLayout->addWidget(m_pTypeSunCloud);
                pTypeCntLayout->addWidget(m_pTypeSimpleStorageSystem);
            }
        }
        m_pSettingsCnt = new QGroupBox(this);
        {
            QGridLayout *pSettingsLayout = new QGridLayout(m_pSettingsCnt);
            {
                m_pUsernameEditor = new QLineEdit(m_pSettingsCnt);
                m_pUsernameLabel = new QLabel(m_pSettingsCnt);
                {
                    m_pUsernameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                    m_pUsernameLabel->setBuddy(m_pUsernameEditor);
                }
                m_pPasswordEditor = new QLineEdit(m_pSettingsCnt);
                {
                    m_pPasswordEditor->setEchoMode(QLineEdit::Password);
                }
                m_pPasswordLabel = new QLabel(m_pSettingsCnt);
                {
                    m_pPasswordLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                    m_pPasswordLabel->setBuddy(m_pPasswordEditor);
                }
                m_pHostnameEditor = new QLineEdit(m_pSettingsCnt);
                m_pHostnameLabel = new QLabel(m_pSettingsCnt);
                {
                    m_pHostnameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                    m_pHostnameLabel->setBuddy(m_pHostnameEditor);
                }
                m_pBucketEditor = new QLineEdit(m_pSettingsCnt);
                m_pBucketLabel = new QLabel(m_pSettingsCnt);
                {
                    m_pBucketLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                    m_pBucketLabel->setBuddy(m_pBucketEditor);
                }
                m_pFileSelector = new UIEmptyFilePathSelector(m_pSettingsCnt);
                {
                    m_pFileSelector->setMode(UIEmptyFilePathSelector::Mode_File_Save);
                    m_pFileSelector->setEditable(true);
                    m_pFileSelector->setButtonPosition(UIEmptyFilePathSelector::RightPosition);
                    m_pFileSelector->setDefaultSaveExt("ova");
                }
                m_pFileSelectorLabel = new QLabel(m_pSettingsCnt);
                {
                    m_pFileSelectorLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                    m_pFileSelectorLabel->setBuddy(m_pFileSelector);
                }
                m_pFormatComboBox = new QComboBox(m_pSettingsCnt);
                {
                    const QString strFormatOVF09("ovf-0.9");
                    const QString strFormatOVF10("ovf-1.0");
                    const QString strFormatOVF20("ovf-2.0");
                    const QString strFormatOPC10("opc-1.0");
                    m_pFormatComboBox->addItem(strFormatOVF09, strFormatOVF09);
                    m_pFormatComboBox->addItem(strFormatOVF10, strFormatOVF10);
                    m_pFormatComboBox->addItem(strFormatOVF20, strFormatOVF20);
                    m_pFormatComboBox->addItem(strFormatOPC10, strFormatOPC10);
                    connect(m_pFormatComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                            this, &UIWizardExportAppPageExpert::sltHandleFormatComboChange);
                }
                m_pFormatComboBoxLabel = new QLabel(m_pSettingsCnt);
                {
                    m_pFormatComboBoxLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                    m_pFormatComboBoxLabel->setBuddy(m_pFormatComboBox);
                }
                m_pManifestCheckbox = new QCheckBox(m_pSettingsCnt);
                pSettingsLayout->addWidget(m_pUsernameLabel, 0, 0);
                pSettingsLayout->addWidget(m_pUsernameEditor, 0, 1);
                pSettingsLayout->addWidget(m_pPasswordLabel, 1, 0);
                pSettingsLayout->addWidget(m_pPasswordEditor, 1, 1);
                pSettingsLayout->addWidget(m_pHostnameLabel, 2, 0);
                pSettingsLayout->addWidget(m_pHostnameEditor, 2, 1);
                pSettingsLayout->addWidget(m_pBucketLabel, 3, 0);
                pSettingsLayout->addWidget(m_pBucketEditor, 3, 1);
                pSettingsLayout->addWidget(m_pFileSelectorLabel, 4, 0);
                pSettingsLayout->addWidget(m_pFileSelector, 4, 1);
                pSettingsLayout->addWidget(m_pFormatComboBoxLabel, 5, 0);
                pSettingsLayout->addWidget(m_pFormatComboBox, 5, 1);
                pSettingsLayout->addWidget(m_pManifestCheckbox, 6, 0, 1, 2);
            }
        }
        pMainLayout->addWidget(m_pSelectorCnt, 0, 0);
        pMainLayout->addWidget(m_pApplianceCnt, 0, 1);
        pMainLayout->addWidget(m_pTypeCnt, 1, 0, 1, 2);
        pMainLayout->addWidget(m_pSettingsCnt, 2, 0, 1, 2);
        populateVMSelectorItems(selectedVMNames);
        chooseDefaultStorageType();
        chooseDefaultSettings();
    }

    /* Setup connections: */
    connect(m_pVMSelector, SIGNAL(itemSelectionChanged()), this, SLOT(sltVMSelectionChangeHandler()));
    connect(m_pTypeLocalFilesystem, SIGNAL(clicked()), this, SLOT(sltStorageTypeChangeHandler()));
    connect(m_pTypeSunCloud, SIGNAL(clicked()), this, SLOT(sltStorageTypeChangeHandler()));
    connect(m_pTypeSimpleStorageSystem, SIGNAL(clicked()), this, SLOT(sltStorageTypeChangeHandler()));
    connect(m_pUsernameEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pPasswordEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pHostnameEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pBucketEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pFileSelector, SIGNAL(pathChanged(const QString &)), this, SIGNAL(completeChanged()));

    /* Register classes: */
    qRegisterMetaType<StorageType>();
    qRegisterMetaType<ExportAppliancePointer>();
    /* Register fields: */
    registerField("machineNames", this, "machineNames");
    registerField("machineIDs", this, "machineIDs");
    registerField("storageType", this, "storageType");
    registerField("format", this, "format");
    registerField("manifestSelected", this, "manifestSelected");
    registerField("username", this, "username");
    registerField("password", this, "password");
    registerField("hostname", this, "hostname");
    registerField("bucket", this, "bucket");
    registerField("path", this, "path");
    registerField("applianceWidget", this, "applianceWidget");
}

void UIWizardExportAppPageExpert::sltVMSelectionChangeHandler()
{
    /* Call to base-class: */
    refreshCurrentSettings();
    refreshApplianceSettingsWidget();

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardExportAppPageExpert::sltStorageTypeChangeHandler()
{
    /* Call to base-class: */
    refreshCurrentSettings();

    /* Broadcast complete-change: */
    emit completeChanged();
}

void UIWizardExportAppPageExpert::sltHandleFormatComboChange()
{
    refreshCurrentSettings();
    updateFormatComboToolTip();
}

void UIWizardExportAppPageExpert::retranslateUi()
{
    /* Translate objects: */
    m_strDefaultApplianceName = UIWizardExportApp::tr("Appliance");
    /* Translate widgets: */
    m_pSelectorCnt->setTitle(UIWizardExportApp::tr("Virtual &machines to export"));
    m_pApplianceCnt->setTitle(UIWizardExportApp::tr("Appliance &settings"));
    m_pTypeCnt->setTitle(UIWizardExportApp::tr("&Destination"));
    m_pSettingsCnt->setTitle(UIWizardExportApp::tr("&Storage settings"));
    m_pTypeLocalFilesystem->setText(UIWizardExportApp::tr("&Local Filesystem "));
    m_pTypeSunCloud->setText(UIWizardExportApp::tr("Sun &Cloud"));
    m_pTypeSimpleStorageSystem->setText(UIWizardExportApp::tr("&Simple Storage System (S3)"));
    m_pUsernameLabel->setText(UIWizardExportApp::tr("&Username:"));
    m_pPasswordLabel->setText(UIWizardExportApp::tr("&Password:"));
    m_pHostnameLabel->setText(UIWizardExportApp::tr("&Hostname:"));
    m_pBucketLabel->setText(UIWizardExportApp::tr("&Bucket:"));
    m_pFileSelectorLabel->setText(UIWizardExportApp::tr("&File:"));
    m_pFileSelector->setChooseButtonToolTip(tr("Choose a file to export the virtual appliance to..."));
    m_pFileSelector->setFileDialogTitle(UIWizardExportApp::tr("Please choose a file to export the virtual appliance to"));
    m_pFormatComboBoxLabel->setText(UIWizardExportApp::tr("F&ormat:"));
    m_pFormatComboBox->setItemText(0, UIWizardExportApp::tr("Open Virtualization Format 0.9"));
    m_pFormatComboBox->setItemText(1, UIWizardExportApp::tr("Open Virtualization Format 1.0"));
    m_pFormatComboBox->setItemText(2, UIWizardExportApp::tr("Open Virtualization Format 2.0"));
    m_pFormatComboBox->setItemText(3, UIWizardExportApp::tr("Oracle Public Cloud Format 1.0"));
    m_pFormatComboBox->setItemData(0, UIWizardExportApp::tr("Write in legacy OVF 0.9 format for compatibility "
                                                            "with other virtualization products."), Qt::ToolTipRole);
    m_pFormatComboBox->setItemData(1, UIWizardExportApp::tr("Write in standard OVF 1.0 format."), Qt::ToolTipRole);
    m_pFormatComboBox->setItemData(2, UIWizardExportApp::tr("Write in new OVF 2.0 format."), Qt::ToolTipRole);
    m_pFormatComboBox->setItemData(3, UIWizardExportApp::tr("Write in Oracle Public Cloud 1.0 format."), Qt::ToolTipRole);
    m_pManifestCheckbox->setToolTip(UIWizardExportApp::tr("Create a Manifest file for automatic data integrity checks on import."));
    m_pManifestCheckbox->setText(UIWizardExportApp::tr("Write &Manifest file"));

    /* Refresh current settings: */
    refreshCurrentSettings();
    updateFormatComboToolTip();
}

void UIWizardExportAppPageExpert::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Call to base-class: */
    refreshCurrentSettings();
    refreshApplianceSettingsWidget();
}

bool UIWizardExportAppPageExpert::isComplete() const
{
    /* Initial result: */
    bool fResult = true;

    /* There should be at least one vm selected: */
    if (fResult)
        fResult = (m_pVMSelector->selectedItems().size() > 0);

    /* Check storage-type attributes: */
    if (fResult)
    {
        const QString &strFile = m_pFileSelector->path().toLower();
        bool fOVF =    field("format").toString() == "ovf-0.9"
                    || field("format").toString() == "ovf-1.0"
                    || field("format").toString() == "ovf-2.0";
        bool fOPC =    field("format").toString() == "opc-1.0";
        fResult =    (   fOVF
                      && VBoxGlobal::hasAllowedExtension(strFile, OVFFileExts))
                  || (   fOPC
                      && VBoxGlobal::hasAllowedExtension(strFile, OPCFileExts));
        if (fResult)
        {
            StorageType st = storageType();
            switch (st)
            {
                case Filesystem:
                    break;
                case SunCloud:
                    fResult &= !m_pUsernameEditor->text().isEmpty() &&
                               !m_pPasswordEditor->text().isEmpty() &&
                               !m_pBucketEditor->text().isEmpty();
                    break;
                case S3:
                    fResult &= !m_pUsernameEditor->text().isEmpty() &&
                               !m_pPasswordEditor->text().isEmpty() &&
                               !m_pHostnameEditor->text().isEmpty() &&
                               !m_pBucketEditor->text().isEmpty();
                    break;
            }
        }
    }

    /* Return result: */
    return fResult;
}

bool UIWizardExportAppPageExpert::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to export appliance: */
    fResult = qobject_cast<UIWizardExportApp*>(wizard())->exportAppliance();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}


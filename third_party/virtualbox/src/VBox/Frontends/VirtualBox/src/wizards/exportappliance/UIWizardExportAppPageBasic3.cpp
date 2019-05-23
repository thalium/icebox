/* $Id: UIWizardExportAppPageBasic3.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic3 class implementation.
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
# include <QDir>
# include <QVBoxLayout>
# include <QGridLayout>
# include <QLabel>
# include <QLineEdit>
# include <QComboBox>
# include <QCheckBox>

/* Local includes: */
# include "UIWizardExportAppPageBasic3.h"
# include "UIWizardExportApp.h"
# include "UIWizardExportAppDefs.h"
# include "VBoxGlobal.h"
# include "UIEmptyFilePathSelector.h"
# include "QIRichTextLabel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardExportAppPage3::UIWizardExportAppPage3()
{
}

void UIWizardExportAppPage3::chooseDefaultSettings()
{
    /* Choose default format: */
    setFormat("ovf-1.0");
}

void UIWizardExportAppPage3::refreshCurrentSettings()
{
    /* Setup components for chosen storage-type: */
    StorageType storageType = fieldImp("storageType").value<StorageType>();
    switch (storageType)
    {
        case Filesystem:
        {
            m_pUsernameLabel->setVisible(false);
            m_pUsernameEditor->setVisible(false);
            m_pPasswordLabel->setVisible(false);
            m_pPasswordEditor->setVisible(false);
            m_pHostnameLabel->setVisible(false);
            m_pHostnameEditor->setVisible(false);
            m_pBucketLabel->setVisible(false);
            m_pBucketEditor->setVisible(false);
            m_pFormatComboBox->setVisible(true);
            m_pFileSelector->setChooserVisible(true);
            break;
        }
        case SunCloud:
        {
            m_pUsernameLabel->setVisible(true);
            m_pUsernameEditor->setVisible(true);
            m_pPasswordLabel->setVisible(true);
            m_pPasswordEditor->setVisible(true);
            m_pHostnameLabel->setVisible(false);
            m_pHostnameEditor->setVisible(false);
            m_pBucketLabel->setVisible(true);
            m_pBucketEditor->setVisible(true);
            m_pFormatComboBox->setVisible(false);
            m_pFileSelector->setChooserVisible(false);
            break;
        }
        case S3:
        {
            m_pUsernameLabel->setVisible(true);
            m_pUsernameEditor->setVisible(true);
            m_pPasswordLabel->setVisible(true);
            m_pPasswordEditor->setVisible(true);
            m_pHostnameLabel->setVisible(true);
            m_pHostnameEditor->setVisible(true);
            m_pBucketLabel->setVisible(true);
            m_pBucketEditor->setVisible(true);
            m_pFormatComboBox->setVisible(true);
            m_pFileSelector->setChooserVisible(false);
            break;
        }
    }

    /* Compose file-name: */
    QString strName;

    /* If it's one VM only, we use the VM name as file-name: */
    if (fieldImp("machineNames").toStringList().size() == 1)
        strName = fieldImp("machineNames").toStringList()[0];
    /* Otherwise => we use the default file-name: */
    else
        strName = m_strDefaultApplianceName;

    /* If the format is set to OPC: */
    if (fieldImp("format").toString() == "opc-1.0")
    {
        /* We use .tar.gz extension: */
        strName += ".tar.gz";

        /* Update file chooser accordingly: */
        m_pFileSelector->setFileFilters(UIWizardExportApp::tr("Oracle Public Cloud Format Archive (%1)").arg("*.tar.gz"));

        /* Disable manifest file creation: */
        m_pManifestCheckbox->setChecked(false);
        m_pManifestCheckbox->setEnabled(false);
    }
    /* Otherwise: */
    else
    {
        /* We use the default (.ova) extension: */
        strName += ".ova";

        /* Update file chooser accordingly: */
        m_pFileSelector->setFileFilters(UIWizardExportApp::tr("Open Virtualization Format Archive (%1)").arg("*.ova") + ";;" +
                                        UIWizardExportApp::tr("Open Virtualization Format (%1)").arg("*.ovf"));

        /* Enable manifest file creation: */
        m_pManifestCheckbox->setEnabled(true);
    }

    /* Copose file-path for 'Filesystem' storage case: */
    if (storageType == Filesystem)
        strName = QDir::toNativeSeparators(QString("%1/%2").arg(vboxGlobal().documentsPath()).arg(strName));

    /* Assign file-path: */
    m_pFileSelector->setPath(strName);
}

void UIWizardExportAppPage3::updateFormatComboToolTip()
{
    int iCurrentIndex = m_pFormatComboBox->currentIndex();
    QString strCurrentIndexToolTip = m_pFormatComboBox->itemData(iCurrentIndex, Qt::ToolTipRole).toString();
    m_pFormatComboBox->setToolTip(strCurrentIndexToolTip);
}

QString UIWizardExportAppPage3::format() const
{
    int iIndex = m_pFormatComboBox->currentIndex();
    return m_pFormatComboBox->itemData(iIndex).toString();
}

void UIWizardExportAppPage3::setFormat(const QString &strFormat)
{
    int iIndex = m_pFormatComboBox->findData(strFormat);
    AssertMsg(iIndex != -1, ("Field not found!"));
    m_pFormatComboBox->setCurrentIndex(iIndex);
}

bool UIWizardExportAppPage3::isManifestSelected() const
{
    return m_pManifestCheckbox->isChecked();
}

void UIWizardExportAppPage3::setManifestSelected(bool fChecked)
{
    m_pManifestCheckbox->setChecked(fChecked);
}

QString UIWizardExportAppPage3::username() const
{
    return m_pUsernameEditor->text();
}

void UIWizardExportAppPage3::setUserName(const QString &strUserName)
{
    m_pUsernameEditor->setText(strUserName);
}

QString UIWizardExportAppPage3::password() const
{
    return m_pPasswordEditor->text();
}

void UIWizardExportAppPage3::setPassword(const QString &strPassword)
{
    m_pPasswordEditor->setText(strPassword);
}

QString UIWizardExportAppPage3::hostname() const
{
    return m_pHostnameEditor->text();
}

void UIWizardExportAppPage3::setHostname(const QString &strHostname)
{
    m_pHostnameEditor->setText(strHostname);
}

QString UIWizardExportAppPage3::bucket() const
{
    return m_pBucketEditor->text();
}

void UIWizardExportAppPage3::setBucket(const QString &strBucket)
{
    m_pBucketEditor->setText(strBucket);
}

QString UIWizardExportAppPage3::path() const
{
    return m_pFileSelector->path();
}

void UIWizardExportAppPage3::setPath(const QString &strPath)
{
    m_pFileSelector->setPath(strPath);
}

UIWizardExportAppPageBasic3::UIWizardExportAppPageBasic3()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QGridLayout *pSettingsLayout = new QGridLayout;
        {
            m_pUsernameEditor = new QLineEdit(this);
            m_pUsernameLabel = new QLabel(this);
            {
                m_pUsernameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                m_pUsernameLabel->setBuddy(m_pUsernameEditor);
            }
            m_pPasswordEditor = new QLineEdit(this);
            {
                m_pPasswordEditor->setEchoMode(QLineEdit::Password);
            }
            m_pPasswordLabel = new QLabel(this);
            {
                m_pPasswordLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                m_pPasswordLabel->setBuddy(m_pPasswordEditor);
            }
            m_pHostnameEditor = new QLineEdit(this);
            m_pHostnameLabel = new QLabel(this);
            {
                m_pHostnameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                m_pHostnameLabel->setBuddy(m_pHostnameEditor);
            }
            m_pBucketEditor = new QLineEdit(this);
            m_pBucketLabel = new QLabel(this);
            {
                m_pBucketLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                m_pBucketLabel->setBuddy(m_pBucketEditor);
            }
            m_pFileSelector = new UIEmptyFilePathSelector(this);
            {
                m_pFileSelector->setMode(UIEmptyFilePathSelector::Mode_File_Save);
                m_pFileSelector->setEditable(true);
                m_pFileSelector->setButtonPosition(UIEmptyFilePathSelector::RightPosition);
                m_pFileSelector->setDefaultSaveExt("ova");
            }
            m_pFileSelectorLabel = new QLabel(this);
            {
                m_pFileSelectorLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                m_pFileSelectorLabel->setBuddy(m_pFileSelector);
            }
            m_pFormatComboBox = new QComboBox(this);
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
                        this, &UIWizardExportAppPageBasic3::sltHandleFormatComboChange);
            }
            m_pFormatComboBoxLabel = new QLabel(this);
            {
                m_pFormatComboBoxLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
                m_pFormatComboBoxLabel->setBuddy(m_pFormatComboBox);
            }
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
        }
        m_pManifestCheckbox = new QCheckBox(this);
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pSettingsLayout);
        pMainLayout->addWidget(m_pManifestCheckbox);
        pMainLayout->addStretch();
        chooseDefaultSettings();
    }

    /* Setup connections: */
    connect(m_pUsernameEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pPasswordEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pHostnameEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pBucketEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pFileSelector, SIGNAL(pathChanged(const QString &)), this, SIGNAL(completeChanged()));

    /* Register fields: */
    registerField("format", this, "format");
    registerField("manifestSelected", this, "manifestSelected");
    registerField("username", this, "username");
    registerField("password", this, "password");
    registerField("hostname", this, "hostname");
    registerField("bucket", this, "bucket");
    registerField("path", this, "path");
}

void UIWizardExportAppPageBasic3::sltHandleFormatComboChange()
{
    refreshCurrentSettings();
    updateFormatComboToolTip();
}

void UIWizardExportAppPageBasic3::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardExportApp::tr("Storage settings"));

    /* Translate objects: */
    m_strDefaultApplianceName = UIWizardExportApp::tr("Appliance");
    /* Translate widgets: */
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

void UIWizardExportAppPageBasic3::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardExportAppPageBasic3::isComplete() const
{
    /* Initial result: */
    bool fResult = true;

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
            StorageType storageType = field("storageType").value<StorageType>();
            switch (storageType)
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

void UIWizardExportAppPageBasic3::refreshCurrentSettings()
{
    /* Refresh base-class settings: */
    UIWizardExportAppPage3::refreshCurrentSettings();

    /* Setup components for chosen storage-type: */
    StorageType storageType = field("storageType").value<StorageType>();
    switch (storageType)
    {
        case Filesystem:
        {
            m_pLabel->setText(tr("<p>Please choose a filename to export the virtual appliance to.</p>"
                                 "<p>The <b>Open Virtualization Format</b> supports only "
                                 "<b>ovf</b> or <b>ova</b> extensions. "
                                 "<br>If you use the <b>ovf</b> extension, "
                                 "several files will be written separately."
                                 "<br>If you use the <b>ova</b> extension, "
                                 "all the files will be combined into one Open Virtualization Format archive.</p>"
                                 "<p>The <b>Oracle Public Cloud Format</b> supports only the <b>tar.gz</b> extension."
                                 "<br>Each virtual disk file will be written separately.</p>"));
            m_pFileSelector->setFocus();
            break;
        }
        case SunCloud:
        {
            m_pLabel->setText(tr("Please complete the additional fields like the username, password "
                                 "and the bucket, and provide a filename for the OVF target."));
            m_pUsernameEditor->setFocus();
            break;
        }
        case S3:
        {
            m_pLabel->setText(tr("Please complete the additional fields like the username, password, "
                                 "hostname and the bucket, and provide a filename for the OVF target."));
            m_pUsernameEditor->setFocus();
            break;
        }
    }
}


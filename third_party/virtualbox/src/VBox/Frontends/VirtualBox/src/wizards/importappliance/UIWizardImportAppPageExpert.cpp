/* $Id: UIWizardImportAppPageExpert.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageExpert class implementation.
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
# include <QFileInfo>
# include <QVBoxLayout>
# include <QGroupBox>

/* Local includes: */
# include "UIWizardImportAppPageExpert.h"
# include "UIWizardImportApp.h"
# include "VBoxGlobal.h"
# include "UIEmptyFilePathSelector.h"
# include "UIApplianceImportEditorWidget.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardImportAppPageExpert::UIWizardImportAppPageExpert(const QString &strFileName)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pApplianceCnt = new QGroupBox(this);
        {
            QVBoxLayout *pApplianceCntLayout = new QVBoxLayout(m_pApplianceCnt);
            {
                m_pFileSelector = new UIEmptyFilePathSelector(m_pApplianceCnt);
                {
                    m_pFileSelector->setHomeDir(vboxGlobal().documentsPath());
                    m_pFileSelector->setMode(UIEmptyFilePathSelector::Mode_File_Open);
                    m_pFileSelector->setButtonPosition(UIEmptyFilePathSelector::RightPosition);
                    m_pFileSelector->setEditable(true);
                }
                pApplianceCntLayout->addWidget(m_pFileSelector);
            }
        }
        m_pSettingsCnt = new QGroupBox(this);
        {
            QVBoxLayout *pSettingsCntLayout = new QVBoxLayout(m_pSettingsCnt);
            {
                m_pApplianceWidget = new UIApplianceImportEditorWidget(m_pSettingsCnt);
                {
                    m_pApplianceWidget->setMinimumHeight(300);
                    m_pApplianceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
                    m_pApplianceWidget->setFile(strFileName);
                }
                pSettingsCntLayout->addWidget(m_pApplianceWidget);
            }
        }
        pMainLayout->addWidget(m_pApplianceCnt);
        pMainLayout->addWidget(m_pSettingsCnt);
        m_pFileSelector->setPath(strFileName);
    }

    /* Setup connections: */
    connect(m_pFileSelector, SIGNAL(pathChanged(const QString&)), this, SLOT(sltFilePathChangeHandler()));

    /* Register classes: */
    qRegisterMetaType<ImportAppliancePointer>();
    /* Register fields: */
    registerField("applianceWidget", this, "applianceWidget");
}

void UIWizardImportAppPageExpert::sltFilePathChangeHandler()
{
    /* Check if set file contains valid appliance: */
    if (QFile::exists(m_pFileSelector->path()) &&
        m_pApplianceWidget->setFile(m_pFileSelector->path()))
    {
        /* Reset the modified bit if file was correctly set: */
        m_pFileSelector->resetModified();
    }

    emit completeChanged();
}

void UIWizardImportAppPageExpert::retranslateUi()
{
    /* Translate widgets: */
    m_pApplianceCnt->setTitle(UIWizardImportApp::tr("Appliance to import"));
    m_pFileSelector->setChooseButtonToolTip(UIWizardImportApp::tr("Choose a virtual appliance file to import..."));
    m_pFileSelector->setFileDialogTitle(UIWizardImportApp::tr("Please choose a virtual appliance file to import"));
    m_pFileSelector->setFileFilters(UIWizardImportApp::tr("Open Virtualization Format (%1)").arg("*.ova *.ovf"));
    m_pSettingsCnt->setTitle(UIWizardImportApp::tr("Appliance settings"));
}

void UIWizardImportAppPageExpert::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardImportAppPageExpert::isComplete() const
{
    /* Make sure appliance file has allowed extension and exists and appliance widget is valid: */
    return VBoxGlobal::hasAllowedExtension(m_pFileSelector->path().toLower(), OVFFileExts) &&
           QFile::exists(m_pFileSelector->path()) &&
           m_pApplianceWidget->isValid();
}

bool UIWizardImportAppPageExpert::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to import appliance: */
    if (fResult)
        fResult = qobject_cast<UIWizardImportApp*>(wizard())->importAppliance();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}


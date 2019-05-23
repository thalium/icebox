/* $Id: UIWizardExportAppPageBasic2.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic2 class implementation.
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
# include <QGroupBox>
# include <QRadioButton>

/* Local includes: */
# include "UIWizardExportAppPageBasic2.h"
# include "UIWizardExportApp.h"
# include "QIRichTextLabel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardExportAppPage2::UIWizardExportAppPage2()
{
}

void UIWizardExportAppPage2::chooseDefaultStorageType()
{
    /* Just select first of types: */
    setStorageType(Filesystem);
}

StorageType UIWizardExportAppPage2::storageType() const
{
    if (m_pTypeSunCloud->isChecked())
        return SunCloud;
    else if (m_pTypeSimpleStorageSystem->isChecked())
        return S3;
    return Filesystem;
}

void UIWizardExportAppPage2::setStorageType(StorageType storageType)
{
    switch (storageType)
    {
        case Filesystem:
            m_pTypeLocalFilesystem->setChecked(true);
            m_pTypeLocalFilesystem->setFocus();
            break;
        case SunCloud:
            m_pTypeSunCloud->setChecked(true);
            m_pTypeSunCloud->setFocus();
            break;
        case S3:
            m_pTypeSimpleStorageSystem->setChecked(true);
            m_pTypeSimpleStorageSystem->setFocus();
            break;
    }
}

UIWizardExportAppPageBasic2::UIWizardExportAppPageBasic2()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pTypeCnt = new QGroupBox(this);
        {
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
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pTypeCnt);
        pMainLayout->addStretch();
        chooseDefaultStorageType();
    }

    /* Setup connections: */
    connect(m_pTypeLocalFilesystem, SIGNAL(clicked()), this, SIGNAL(completeChanged()));
    connect(m_pTypeSunCloud, SIGNAL(clicked()), this, SIGNAL(completeChanged()));
    connect(m_pTypeSimpleStorageSystem, SIGNAL(clicked()), this, SIGNAL(completeChanged()));

    /* Register classes: */
    qRegisterMetaType<StorageType>();
    /* Register fields: */
    registerField("storageType", this, "storageType");
}

void UIWizardExportAppPageBasic2::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardExportApp::tr("Appliance settings"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardExportApp::tr("Please choose where to create the virtual appliance. "
                                            "You can create it on your own computer, "
                                            "on the Sun Cloud service or on an S3 storage server."));
    m_pTypeCnt->setTitle(UIWizardExportApp::tr("Create on"));
    m_pTypeLocalFilesystem->setText(UIWizardExportApp::tr("&This computer"));
    m_pTypeSunCloud->setText(UIWizardExportApp::tr("Sun &Cloud"));
    m_pTypeSimpleStorageSystem->setText(UIWizardExportApp::tr("&Simple Storage System (S3)"));
}

void UIWizardExportAppPageBasic2::initializePage()
{
    /* Translate page: */
    retranslateUi();
}


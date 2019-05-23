/* $Id: UIWizardNewVDPageBasic2.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVDPageBasic2 class implementation.
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
# include <QVBoxLayout>
# include <QButtonGroup>
# include <QRadioButton>
# include <QCheckBox>

/* GUI includes: */
# include "UIWizardNewVDPageBasic2.h"
# include "UIWizardNewVD.h"
# include "QIRichTextLabel.h"

/* COM includes: */
# include "CMediumFormat.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardNewVDPage2::UIWizardNewVDPage2()
{
}

qulonglong UIWizardNewVDPage2::mediumVariant() const
{
    /* Initial value: */
    qulonglong uMediumVariant = (qulonglong)KMediumVariant_Max;

    /* Exclusive options: */
    if (m_pDynamicalButton->isChecked())
        uMediumVariant = (qulonglong)KMediumVariant_Standard;
    else if (m_pFixedButton->isChecked())
        uMediumVariant = (qulonglong)KMediumVariant_Fixed;

    /* Additional options: */
    if (m_pSplitBox->isChecked())
        uMediumVariant |= (qulonglong)KMediumVariant_VmdkSplit2G;

    /* Return options: */
    return uMediumVariant;
}

void UIWizardNewVDPage2::setMediumVariant(qulonglong uMediumVariant)
{
    /* Exclusive options: */
    if (uMediumVariant & (qulonglong)KMediumVariant_Fixed)
    {
        m_pFixedButton->click();
        m_pFixedButton->setFocus();
    }
    else
    {
        m_pDynamicalButton->click();
        m_pDynamicalButton->setFocus();
    }

    /* Additional options: */
    m_pSplitBox->setChecked(uMediumVariant & (qulonglong)KMediumVariant_VmdkSplit2G);
}

UIWizardNewVDPageBasic2::UIWizardNewVDPageBasic2()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pDescriptionLabel = new QIRichTextLabel(this);
        m_pDynamicLabel = new QIRichTextLabel(this);
        m_pFixedLabel = new QIRichTextLabel(this);
        m_pSplitLabel = new QIRichTextLabel(this);
        QVBoxLayout *pVariantLayout = new QVBoxLayout;
        {
            m_pVariantButtonGroup = new QButtonGroup(this);
            {
                m_pDynamicalButton = new QRadioButton(this);
                {
                    m_pDynamicalButton->click();
                    m_pDynamicalButton->setFocus();
                }
                m_pFixedButton = new QRadioButton(this);
                m_pVariantButtonGroup->addButton(m_pDynamicalButton, 0);
                m_pVariantButtonGroup->addButton(m_pFixedButton, 1);
            }
            m_pSplitBox = new QCheckBox(this);
            pVariantLayout->addWidget(m_pDynamicalButton);
            pVariantLayout->addWidget(m_pFixedButton);
            pVariantLayout->addWidget(m_pSplitBox);
        }
        pMainLayout->addWidget(m_pDescriptionLabel);
        pMainLayout->addWidget(m_pDynamicLabel);
        pMainLayout->addWidget(m_pFixedLabel);
        pMainLayout->addWidget(m_pSplitLabel);
        pMainLayout->addLayout(pVariantLayout);
        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pVariantButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SIGNAL(completeChanged()));
    connect(m_pSplitBox, SIGNAL(stateChanged(int)), this, SIGNAL(completeChanged()));

    /* Register fields: */
    registerField("mediumVariant", this, "mediumVariant");
}

void UIWizardNewVDPageBasic2::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardNewVD::tr("Storage on physical hard disk"));

    /* Translate widgets: */
    m_pDescriptionLabel->setText(UIWizardNewVD::tr("Please choose whether the new virtual hard disk file should grow as it is used "
                                                   "(dynamically allocated) or if it should be created at its maximum size (fixed size)."));
    m_pDynamicLabel->setText(UIWizardNewVD::tr("<p>A <b>dynamically allocated</b> hard disk file will only use space "
                                               "on your physical hard disk as it fills up (up to a maximum <b>fixed size</b>), "
                                               "although it will not shrink again automatically when space on it is freed.</p>"));
    m_pFixedLabel->setText(UIWizardNewVD::tr("<p>A <b>fixed size</b> hard disk file may take longer to create on some "
                                             "systems but is often faster to use.</p>"));
    m_pSplitLabel->setText(UIWizardNewVD::tr("<p>You can also choose to <b>split</b> the hard disk file into several files "
                                             "of up to two gigabytes each. This is mainly useful if you wish to store the "
                                             "virtual machine on removable USB devices or old systems, some of which cannot "
                                             "handle very large files."));
    m_pDynamicalButton->setText(UIWizardNewVD::tr("&Dynamically allocated"));
    m_pFixedButton->setText(UIWizardNewVD::tr("&Fixed size"));
    m_pSplitBox->setText(UIWizardNewVD::tr("&Split into files of less than 2GB"));
}

void UIWizardNewVDPageBasic2::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Setup visibility: */
    CMediumFormat mediumFormat = field("mediumFormat").value<CMediumFormat>();
    ULONG uCapabilities = 0;
    QVector<KMediumFormatCapabilities> capabilities;
    capabilities = mediumFormat.GetCapabilities();
    for (int i = 0; i < capabilities.size(); i++)
        uCapabilities |= capabilities[i];

    bool fIsCreateDynamicPossible = uCapabilities & KMediumFormatCapabilities_CreateDynamic;
    bool fIsCreateFixedPossible = uCapabilities & KMediumFormatCapabilities_CreateFixed;
    bool fIsCreateSplitPossible = uCapabilities & KMediumFormatCapabilities_CreateSplit2G;
    m_pDynamicLabel->setHidden(!fIsCreateDynamicPossible);
    m_pDynamicalButton->setHidden(!fIsCreateDynamicPossible);
    m_pFixedLabel->setHidden(!fIsCreateFixedPossible);
    m_pFixedButton->setHidden(!fIsCreateFixedPossible);
    m_pSplitLabel->setHidden(!fIsCreateSplitPossible);
    m_pSplitBox->setHidden(!fIsCreateSplitPossible);
}

bool UIWizardNewVDPageBasic2::isComplete() const
{
    /* Make sure medium variant is correct: */
    return mediumVariant() != (qulonglong)KMediumVariant_Max;
}


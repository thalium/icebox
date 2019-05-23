/* $Id: UIWizardCloneVMPageBasic2.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic2 class implementation.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
# include <QRadioButton>
# include <QButtonGroup>

/* GUI includes: */
# include "UIWizardCloneVMPageBasic2.h"
# include "UIWizardCloneVM.h"
# include "QIRichTextLabel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVMPage2::UIWizardCloneVMPage2(bool fAdditionalInfo)
    : m_fAdditionalInfo(fAdditionalInfo)
{
}

bool UIWizardCloneVMPage2::isLinkedClone() const
{
    return m_pLinkedCloneRadio->isChecked();
}

UIWizardCloneVMPageBasic2::UIWizardCloneVMPageBasic2(bool fAdditionalInfo)
    : UIWizardCloneVMPage2(fAdditionalInfo)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QVBoxLayout *pCloneTypeCntLayout = new QVBoxLayout;
        {
            m_pButtonGroup = new QButtonGroup(this);
            {
                m_pFullCloneRadio = new QRadioButton(this);
                {
                    m_pFullCloneRadio->setChecked(true);
                }
                m_pLinkedCloneRadio = new QRadioButton(this);
                m_pButtonGroup->addButton(m_pFullCloneRadio);
                m_pButtonGroup->addButton(m_pLinkedCloneRadio);
            }
            pCloneTypeCntLayout->addWidget(m_pFullCloneRadio);
            pCloneTypeCntLayout->addWidget(m_pLinkedCloneRadio);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pCloneTypeCntLayout);
        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(sltButtonClicked(QAbstractButton *)));

    /* Register fields: */
    registerField("linkedClone", this, "linkedClone");
}

void UIWizardCloneVMPageBasic2::sltButtonClicked(QAbstractButton *pButton)
{
    setFinalPage(pButton != m_pFullCloneRadio);
}

void UIWizardCloneVMPageBasic2::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVM::tr("Clone type"));

    /* Translate widgets: */
    QString strLabel = UIWizardCloneVM::tr("<p>Please choose the type of clone you wish to create.</p>"
                                           "<p>If you choose <b>Full clone</b>, "
                                           "an exact copy (including all virtual hard disk files) "
                                           "of the original virtual machine will be created.</p>"
                                           "<p>If you choose <b>Linked clone</b>, "
                                           "a new machine will be created, but the virtual hard disk files "
                                           "will be tied to the virtual hard disk files of original machine "
                                           "and you will not be able to move the new virtual machine "
                                           "to a different computer without moving the original as well.</p>");
    if (m_fAdditionalInfo)
        strLabel += UIWizardCloneVM::tr("<p>If you create a <b>Linked clone</b> then a new snapshot will be created "
                                        "in the original virtual machine as part of the cloning process.</p>");
    m_pLabel->setText(strLabel);

    m_pFullCloneRadio->setText(UIWizardCloneVM::tr("&Full clone"));
    m_pLinkedCloneRadio->setText(UIWizardCloneVM::tr("&Linked clone"));
}

void UIWizardCloneVMPageBasic2::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardCloneVMPageBasic2::validatePage()
{
    /* This page could be final: */
    if (isFinalPage())
    {
        /* Initial result: */
        bool fResult = true;

        /* Lock finish button: */
        startProcessing();

        /* Trying to clone VM: */
        if (fResult)
            fResult = qobject_cast<UIWizardCloneVM*>(wizard())->cloneVM();

        /* Unlock finish button: */
        endProcessing();

        /* Return result: */
        return fResult;
    }
    else
        return true;
}

int UIWizardCloneVMPageBasic2::nextId() const
{
    return m_pFullCloneRadio->isChecked() && wizard()->page(UIWizardCloneVM::Page3) ? UIWizardCloneVM::Page3 : -1;
}


/* $Id: UIWizardCloneVMPageBasic1.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic1 class implementation.
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
# include <QLineEdit>
# include <QCheckBox>

/* GUI includes: */
# include "UIWizardCloneVMPageBasic1.h"
# include "UIWizardCloneVM.h"
# include "QIRichTextLabel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVMPage1::UIWizardCloneVMPage1(const QString &strOriginalName)
    : m_strOriginalName(strOriginalName)
{
}

QString UIWizardCloneVMPage1::cloneName() const
{
    return m_pNameEditor->text();
}

void UIWizardCloneVMPage1::setCloneName(const QString &strName)
{
    m_pNameEditor->setText(strName);
}

bool UIWizardCloneVMPage1::isReinitMACsChecked() const
{
    return m_pReinitMACsCheckBox->isChecked();
}

UIWizardCloneVMPageBasic1::UIWizardCloneVMPageBasic1(const QString &strOriginalName)
    : UIWizardCloneVMPage1(strOriginalName)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pNameEditor = new QLineEdit(this);
        {
            m_pNameEditor->setText(UIWizardCloneVM::tr("%1 Clone").arg(m_strOriginalName));
        }
        m_pReinitMACsCheckBox = new QCheckBox(this);
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pNameEditor);
        pMainLayout->addWidget(m_pReinitMACsCheckBox);
        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pNameEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));

    /* Register fields: */
    registerField("cloneName", this, "cloneName");
    registerField("reinitMACs", this, "reinitMACs");
}

void UIWizardCloneVMPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVM::tr("New machine name"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardCloneVM::tr("<p>Please choose a name for the new virtual machine. "
                                          "The new machine will be a clone of the machine <b>%1</b>.</p>")
                                          .arg(m_strOriginalName));
    m_pReinitMACsCheckBox->setToolTip(UIWizardCloneVM::tr("When checked a new unique MAC address will be assigned to all configured network cards."));
    m_pReinitMACsCheckBox->setText(UIWizardCloneVM::tr("&Reinitialize the MAC address of all network cards"));
}

void UIWizardCloneVMPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardCloneVMPageBasic1::isComplete() const
{
    /* Make sure VM name feat the rules: */
    QString strName = m_pNameEditor->text().trimmed();
    return !strName.isEmpty() && strName != m_strOriginalName;
}


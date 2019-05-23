/* $Id: UIWizardCloneVMPageBasic3.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic3 class implementation.
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

/* Global includes: */
# include <QVBoxLayout>
# include <QRadioButton>

/* Local includes: */
# include "UIWizardCloneVMPageBasic3.h"
# include "UIWizardCloneVM.h"
# include "QIRichTextLabel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVMPage3::UIWizardCloneVMPage3(bool fShowChildsOption)
    : m_fShowChildsOption(fShowChildsOption)
{
}

KCloneMode UIWizardCloneVMPage3::cloneMode() const
{
    if (m_pMachineAndChildsRadio->isChecked())
        return KCloneMode_MachineAndChildStates;
    else if (m_pAllRadio->isChecked())
        return KCloneMode_AllStates;
    return KCloneMode_MachineState;
}

void UIWizardCloneVMPage3::setCloneMode(KCloneMode cloneMode)
{
    switch (cloneMode)
    {
        case KCloneMode_MachineState: m_pMachineRadio->setChecked(true); break;
        case KCloneMode_MachineAndChildStates: m_pMachineAndChildsRadio->setChecked(true); break;
        case KCloneMode_AllStates: m_pAllRadio->setChecked(true); break;
        case KCloneMode_Max: break; /* Shut up, MSC! */
    }
}

UIWizardCloneVMPageBasic3::UIWizardCloneVMPageBasic3(bool fShowChildsOption)
    : UIWizardCloneVMPage3(fShowChildsOption)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QVBoxLayout *pCloneModeCntLayout = new QVBoxLayout;
        {
            m_pMachineRadio = new QRadioButton(this);
            {
                m_pMachineRadio->setChecked(true);
            }
            m_pMachineAndChildsRadio = new QRadioButton(this);
            {
                if (!m_fShowChildsOption)
                   m_pMachineAndChildsRadio->hide();
            }
            m_pAllRadio = new QRadioButton(this);
            pCloneModeCntLayout->addWidget(m_pMachineRadio);
            pCloneModeCntLayout->addWidget(m_pMachineAndChildsRadio);
            pCloneModeCntLayout->addWidget(m_pAllRadio);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pCloneModeCntLayout);
        pMainLayout->addStretch();
    }

    /* Register classes: */
    qRegisterMetaType<KCloneMode>();
    /* Register fields: */
    registerField("cloneMode", this, "cloneMode");
}

void UIWizardCloneVMPageBasic3::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVM::tr("Snapshots"));

    /* Translate widgets: */
    const QString strGeneral = UIWizardCloneVM::tr("<p>Please choose which parts of the snapshot tree "
                                                   "should be cloned with the machine.</p>");
    const QString strOpt1    = UIWizardCloneVM::tr("<p>If you choose <b>Current machine state</b>, "
                                                   "the new machine will reflect the current state "
                                                   "of the original machine and will have no snapshots.</p>");
    const QString strOpt2    = UIWizardCloneVM::tr("<p>If you choose <b>Current snapshot tree branch</b>, "
                                                   "the new machine will reflect the current state "
                                                   "of the original machine and will have matching snapshots "
                                                   "for all snapshots in the tree branch "
                                                   "starting at the current state in the original machine.</p>");
    const QString strOpt3    = UIWizardCloneVM::tr("<p>If you choose <b>Everything</b>, "
                                                   "the new machine will reflect the current state "
                                                   "of the original machine and will have matching snapshots "
                                                   "for all snapshots in the original machine.</p>");
    if (m_fShowChildsOption)
        m_pLabel->setText(QString("<p>%1</p><p>%2 %3 %4</p>")
                          .arg(strGeneral)
                          .arg(strOpt1)
                          .arg(strOpt2)
                          .arg(strOpt3));
    else
        m_pLabel->setText(QString("<p>%1</p><p>%2 %3</p>")
                          .arg(strGeneral)
                          .arg(strOpt1)
                          .arg(strOpt3));

    m_pMachineRadio->setText(UIWizardCloneVM::tr("Current &machine state"));
    m_pMachineAndChildsRadio->setText(UIWizardCloneVM::tr("Current &snapshot tree branch"));
    m_pAllRadio->setText(UIWizardCloneVM::tr("&Everything"));
}

void UIWizardCloneVMPageBasic3::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardCloneVMPageBasic3::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to clone VM: */
    if (fResult)
        fResult = qobject_cast<UIWizardCloneVM*>(wizard())->cloneVM();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}


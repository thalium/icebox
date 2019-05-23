/* $Id: UIWizardCloneVMPageExpert.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageExpert class implementation.
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
# include <QGridLayout>
# include <QButtonGroup>
# include <QGroupBox>
# include <QLineEdit>
# include <QCheckBox>
# include <QRadioButton>

/* Local includes: */
# include "UIWizardCloneVMPageExpert.h"
# include "UIWizardCloneVM.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVMPageExpert::UIWizardCloneVMPageExpert(const QString &strOriginalName, bool fAdditionalInfo, bool fShowChildsOption)
    : UIWizardCloneVMPage1(strOriginalName)
    , UIWizardCloneVMPage2(fAdditionalInfo)
    , UIWizardCloneVMPage3(fShowChildsOption)
{
    /* Create widgets: */
    QGridLayout *pMainLayout = new QGridLayout(this);
    {
        m_pNameCnt = new QGroupBox(this);
        {
            QVBoxLayout *pNameCntLayout = new QVBoxLayout(m_pNameCnt);
            {
                m_pNameEditor = new QLineEdit(m_pNameCnt);
                {
                    m_pNameEditor->setText(UIWizardCloneVM::tr("%1 Clone").arg(m_strOriginalName));
                }
                pNameCntLayout->addWidget(m_pNameEditor);
            }
        }
        m_pCloneTypeCnt = new QGroupBox(this);
        {
            m_pButtonGroup = new QButtonGroup(m_pCloneTypeCnt);
            {
                QVBoxLayout *pCloneTypeCntLayout = new QVBoxLayout(m_pCloneTypeCnt);
                {
                    m_pFullCloneRadio = new QRadioButton(m_pCloneTypeCnt);
                    {
                        m_pFullCloneRadio->setChecked(true);
                    }
                    m_pLinkedCloneRadio = new QRadioButton(m_pCloneTypeCnt);
                    pCloneTypeCntLayout->addWidget(m_pFullCloneRadio);
                    pCloneTypeCntLayout->addWidget(m_pLinkedCloneRadio);
                }
                m_pButtonGroup->addButton(m_pFullCloneRadio);
                m_pButtonGroup->addButton(m_pLinkedCloneRadio);
            }
        }
        m_pCloneModeCnt = new QGroupBox(this);
        {
            QVBoxLayout *pCloneModeCntLayout = new QVBoxLayout(m_pCloneModeCnt);
            {
                m_pMachineRadio = new QRadioButton(m_pCloneModeCnt);
                {
                    m_pMachineRadio->setChecked(true);
                }
                m_pMachineAndChildsRadio = new QRadioButton(m_pCloneModeCnt);
                {
                    if (!m_fShowChildsOption)
                       m_pMachineAndChildsRadio->hide();
                }
                m_pAllRadio = new QRadioButton(m_pCloneModeCnt);
                pCloneModeCntLayout->addWidget(m_pMachineRadio);
                pCloneModeCntLayout->addWidget(m_pMachineAndChildsRadio);
                pCloneModeCntLayout->addWidget(m_pAllRadio);
            }
        }
        m_pReinitMACsCheckBox = new QCheckBox(this);
        pMainLayout->addWidget(m_pNameCnt, 0, 0, 1, 2);
        pMainLayout->addWidget(m_pCloneTypeCnt, 1, 0, Qt::AlignTop);
        pMainLayout->addWidget(m_pCloneModeCnt, 1, 1, Qt::AlignTop);
        pMainLayout->addWidget(m_pReinitMACsCheckBox, 2, 0, 1, 2);
        pMainLayout->setRowStretch(3, 1);
        sltButtonClicked(m_pFullCloneRadio);
    }

    /* Setup connections: */
    connect(m_pNameEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(sltButtonClicked(QAbstractButton *)));

    /* Register classes: */
    qRegisterMetaType<KCloneMode>();
    /* Register fields: */
    registerField("cloneName", this, "cloneName");
    registerField("reinitMACs", this, "reinitMACs");
    registerField("linkedClone", this, "linkedClone");
    registerField("cloneMode", this, "cloneMode");
}

void UIWizardCloneVMPageExpert::sltButtonClicked(QAbstractButton *pButton)
{
    /* Enabled/disable mode container: */
    m_pCloneModeCnt->setEnabled(pButton == m_pFullCloneRadio);
}

void UIWizardCloneVMPageExpert::retranslateUi()
{
    /* Translate widgets: */
    m_pNameCnt->setTitle(UIWizardCloneVM::tr("New machine &name"));
    m_pCloneTypeCnt->setTitle(UIWizardCloneVM::tr("Clone type"));
    m_pFullCloneRadio->setText(UIWizardCloneVM::tr("&Full Clone"));
    m_pLinkedCloneRadio->setText(UIWizardCloneVM::tr("&Linked Clone"));
    m_pCloneModeCnt->setTitle(UIWizardCloneVM::tr("Snapshots"));
    m_pMachineRadio->setText(UIWizardCloneVM::tr("Current &machine state"));
    m_pMachineAndChildsRadio->setText(UIWizardCloneVM::tr("Current &snapshot tree branch"));
    m_pAllRadio->setText(UIWizardCloneVM::tr("&Everything"));
    m_pReinitMACsCheckBox->setToolTip(UIWizardCloneVM::tr("When checked a new unique MAC address will be assigned to all configured network cards."));
    m_pReinitMACsCheckBox->setText(UIWizardCloneVM::tr("&Reinitialize the MAC address of all network cards"));
}

void UIWizardCloneVMPageExpert::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardCloneVMPageExpert::isComplete() const
{
    /* Make sure VM name feat the rules: */
    QString strName = m_pNameEditor->text().trimmed();
    return !strName.isEmpty() && strName != m_strOriginalName;
}

bool UIWizardCloneVMPageExpert::validatePage()
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


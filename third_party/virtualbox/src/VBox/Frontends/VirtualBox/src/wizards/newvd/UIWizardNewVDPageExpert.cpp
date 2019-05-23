/* $Id: UIWizardNewVDPageExpert.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVDPageExpert class implementation.
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
# include <QGridLayout>
# include <QVBoxLayout>
# include <QRegExpValidator>
# include <QGroupBox>
# include <QRadioButton>
# include <QCheckBox>
# include <QButtonGroup>
# include <QLineEdit>
# include <QSlider>
# include <QLabel>

/* GUI includes: */
# include "UIWizardNewVDPageExpert.h"
# include "UIWizardNewVD.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIIconPool.h"
# include "QIRichTextLabel.h"
# include "QIToolButton.h"
# include "QILineEdit.h"
# include "UIMediumSizeEditor.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardNewVDPageExpert::UIWizardNewVDPageExpert(const QString &strDefaultName, const QString &strDefaultPath, qulonglong uDefaultSize)
    : UIWizardNewVDPage3(strDefaultName, strDefaultPath)
{
    /* Create widgets: */
    QGridLayout *pMainLayout = new QGridLayout(this);
    {
        m_pLocationCnt = new QGroupBox(this);
        {
            m_pLocationCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QHBoxLayout *pLocationCntLayout = new QHBoxLayout(m_pLocationCnt);
            {
                m_pLocationEditor = new QLineEdit(m_pLocationCnt);
                {
                    m_pLocationEditor->setText(m_strDefaultName);
                }
                m_pLocationOpenButton = new QIToolButton(m_pLocationCnt);
                {
                    m_pLocationOpenButton->setAutoRaise(true);
                    m_pLocationOpenButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", "select_file_disabled_16px.png"));
                }
                pLocationCntLayout->addWidget(m_pLocationEditor);
                pLocationCntLayout->addWidget(m_pLocationOpenButton);
            }
        }
        m_pSizeCnt = new QGroupBox(this);
        {
            m_pSizeCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QVBoxLayout *pSizeCntLayout = new QVBoxLayout(m_pSizeCnt);
            {
                m_pEditorSize = new UIMediumSizeEditor;
                {
                    pSizeCntLayout->addWidget(m_pEditorSize);
                }
            }
        }
        m_pFormatCnt = new QGroupBox(this);
        {
            m_pFormatCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QVBoxLayout *pFormatCntLayout = new QVBoxLayout(m_pFormatCnt);
            {
                m_pFormatButtonGroup = new QButtonGroup(m_pFormatCnt);
                {
                    /* Enumerate medium formats in special order: */
                    CSystemProperties properties = vboxGlobal().virtualBox().GetSystemProperties();
                    const QVector<CMediumFormat> &formats = properties.GetMediumFormats();
                    QMap<QString, CMediumFormat> vdi, preferred, others;
                    foreach (const CMediumFormat &format, formats)
                    {
                        /* VDI goes first: */
                        if (format.GetName() == "VDI")
                            vdi[format.GetId()] = format;
                        else
                        {
                            const QVector<KMediumFormatCapabilities> &capabilities = format.GetCapabilities();
                            /* Then goes preferred: */
                            if (capabilities.contains(KMediumFormatCapabilities_Preferred))
                                preferred[format.GetId()] = format;
                            /* Then others: */
                            else
                                others[format.GetId()] = format;
                        }
                    }

                    /* Create buttons for VDI, preferred and others: */
                    foreach (const QString &strId, vdi.keys())
                        addFormatButton(this, pFormatCntLayout, vdi.value(strId), true);
                    foreach (const QString &strId, preferred.keys())
                        addFormatButton(this, pFormatCntLayout, preferred.value(strId), true);
                    foreach (const QString &strId, others.keys())
                        addFormatButton(this, pFormatCntLayout, others.value(strId));

                    if (!m_pFormatButtonGroup->buttons().isEmpty())
                    {
                        m_pFormatButtonGroup->button(0)->click();
                        m_pFormatButtonGroup->button(0)->setFocus();
                    }
                }
            }
        }
        m_pVariantCnt = new QGroupBox(this);
        {
            m_pVariantCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QVBoxLayout *pVariantCntLayout = new QVBoxLayout(m_pVariantCnt);
            {
                m_pVariantButtonGroup = new QButtonGroup(m_pVariantCnt);
                {
                    m_pDynamicalButton = new QRadioButton(m_pVariantCnt);
                    {
                        m_pDynamicalButton->click();
                        m_pDynamicalButton->setFocus();
                    }
                    m_pFixedButton = new QRadioButton(m_pVariantCnt);
                    m_pVariantButtonGroup->addButton(m_pDynamicalButton, 0);
                    m_pVariantButtonGroup->addButton(m_pFixedButton, 1);
                }
                m_pSplitBox = new QCheckBox(m_pVariantCnt);
                pVariantCntLayout->addWidget(m_pDynamicalButton);
                pVariantCntLayout->addWidget(m_pFixedButton);
                pVariantCntLayout->addWidget(m_pSplitBox);
            }
        }
        pMainLayout->addWidget(m_pLocationCnt, 0, 0, 1, 2);
        pMainLayout->addWidget(m_pSizeCnt, 1, 0, 1, 2);
        pMainLayout->addWidget(m_pFormatCnt, 2, 0, Qt::AlignTop);
        pMainLayout->addWidget(m_pVariantCnt, 2, 1, Qt::AlignTop);
        setMediumSize(uDefaultSize);
        sltMediumFormatChanged();
    }

    /* Setup connections: */
    connect(m_pFormatButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(sltMediumFormatChanged()));
    connect(m_pVariantButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SIGNAL(completeChanged()));
    connect(m_pSplitBox, SIGNAL(stateChanged(int)), this, SIGNAL(completeChanged()));
    connect(m_pLocationEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pLocationOpenButton, SIGNAL(clicked()), this, SLOT(sltSelectLocationButtonClicked()));
    connect(m_pEditorSize, &UIMediumSizeEditor::sigSizeChanged, this, &UIWizardNewVDPageExpert::completeChanged);

    /* Register classes: */
    qRegisterMetaType<CMediumFormat>();
    /* Register fields: */
    registerField("mediumFormat", this, "mediumFormat");
    registerField("mediumVariant", this, "mediumVariant");
    registerField("mediumPath", this, "mediumPath");
    registerField("mediumSize", this, "mediumSize");
}

void UIWizardNewVDPageExpert::sltMediumFormatChanged()
{
    /* Get medium format: */
    CMediumFormat mf = mediumFormat();
    if (mf.isNull())
    {
        AssertMsgFailed(("No medium format set!"));
        return;
    }

    /* Enable/disable widgets: */
    ULONG uCapabilities = 0;
    QVector<KMediumFormatCapabilities> capabilities;
    capabilities = mf.GetCapabilities();
    for (int i = 0; i < capabilities.size(); i++)
        uCapabilities |= capabilities[i];

    bool fIsCreateDynamicPossible = uCapabilities & KMediumFormatCapabilities_CreateDynamic;
    bool fIsCreateFixedPossible = uCapabilities & KMediumFormatCapabilities_CreateFixed;
    bool fIsCreateSplitPossible = uCapabilities & KMediumFormatCapabilities_CreateSplit2G;
    m_pDynamicalButton->setEnabled(fIsCreateDynamicPossible);
    m_pFixedButton->setEnabled(fIsCreateFixedPossible);
    m_pSplitBox->setEnabled(fIsCreateSplitPossible);

    /* Compose virtual-disk extension: */
    m_strDefaultExtension = defaultExtension(mf);

    /* Broadcast complete-change: */
    completeChanged();
}

void UIWizardNewVDPageExpert::sltSelectLocationButtonClicked()
{
    /* Call to base-class: */
    onSelectLocationButtonClicked();
}

void UIWizardNewVDPageExpert::retranslateUi()
{
    /* Translate widgets: */
    m_pLocationCnt->setTitle(UIWizardNewVD::tr("File &location"));
    m_pLocationOpenButton->setToolTip(UIWizardNewVD::tr("Choose a location for new virtual hard disk file..."));
    m_pSizeCnt->setTitle(UIWizardNewVD::tr("File &size"));
    m_pFormatCnt->setTitle(UIWizardNewVD::tr("Hard disk file &type"));
    QList<QAbstractButton*> buttons = m_pFormatButtonGroup->buttons();
    for (int i = 0; i < buttons.size(); ++i)
    {
        QAbstractButton *pButton = buttons[i];
        pButton->setText(VBoxGlobal::fullMediumFormatName(m_formatNames[m_pFormatButtonGroup->id(pButton)]));
    }
    m_pVariantCnt->setTitle(UIWizardNewVD::tr("Storage on physical hard disk"));
    m_pDynamicalButton->setText(UIWizardNewVD::tr("&Dynamically allocated"));
    m_pFixedButton->setText(UIWizardNewVD::tr("&Fixed size"));
    m_pSplitBox->setText(UIWizardNewVD::tr("&Split into files of less than 2GB"));
}

void UIWizardNewVDPageExpert::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardNewVDPageExpert::isComplete() const
{
    /* Make sure medium format/variant is correct,
     * current name is not empty and current size feats the bounds: */
    return !mediumFormat().isNull() &&
           mediumVariant() != (qulonglong)KMediumVariant_Max &&
           !m_pLocationEditor->text().trimmed().isEmpty() &&
           mediumSize() >= m_uMediumSizeMin && mediumSize() <= m_uMediumSizeMax;
}

bool UIWizardNewVDPageExpert::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Make sure such virtual-disk doesn't exists: */
    QString strMediumPath(mediumPath());
    fResult = !QFileInfo(strMediumPath).exists();
    if (!fResult)
        msgCenter().cannotOverwriteHardDiskStorage(strMediumPath, this);

    /* Try to create virtual-disk: */
    if (fResult)
        fResult = qobject_cast<UIWizardNewVD*>(wizard())->createVirtualDisk();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}


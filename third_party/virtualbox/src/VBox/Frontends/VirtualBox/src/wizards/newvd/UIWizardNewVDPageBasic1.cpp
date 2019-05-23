/* $Id: UIWizardNewVDPageBasic1.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVDPageBasic1 class implementation.
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

/* GUI includes: */
# include "UIWizardNewVDPageBasic1.h"
# include "UIWizardNewVD.h"
# include "VBoxGlobal.h"
# include "QIRichTextLabel.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardNewVDPage1::UIWizardNewVDPage1()
{
}

void UIWizardNewVDPage1::addFormatButton(QWidget *pParent, QVBoxLayout *pFormatLayout, CMediumFormat medFormat, bool fPreferred /* = false */)
{
    /* Check that medium format supports creation: */
    ULONG uFormatCapabilities = 0;
    QVector<KMediumFormatCapabilities> capabilities;
    capabilities = medFormat.GetCapabilities();
    for (int i = 0; i < capabilities.size(); i++)
        uFormatCapabilities |= capabilities[i];

    if (!(uFormatCapabilities & KMediumFormatCapabilities_CreateFixed ||
          uFormatCapabilities & KMediumFormatCapabilities_CreateDynamic))
        return;

    /* Check that medium format supports creation of virtual hard-disks: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    medFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    if (!deviceTypes.contains(KDeviceType_HardDisk))
        return;

    /* Create/add corresponding radio-button: */
    QRadioButton *pFormatButton = new QRadioButton(pParent);
    AssertPtrReturnVoid(pFormatButton);
    {
        /* Make the preferred button font bold: */
        if (fPreferred)
        {
            QFont font = pFormatButton->font();
            font.setBold(true);
            pFormatButton->setFont(font);
        }
        pFormatLayout->addWidget(pFormatButton);
        m_formats << medFormat;
        m_formatNames << medFormat.GetName();
        m_pFormatButtonGroup->addButton(pFormatButton, m_formatNames.size() - 1);
    }
}

CMediumFormat UIWizardNewVDPage1::mediumFormat() const
{
    return m_pFormatButtonGroup->checkedButton() ? m_formats[m_pFormatButtonGroup->checkedId()] : CMediumFormat();
}

void UIWizardNewVDPage1::setMediumFormat(const CMediumFormat &mediumFormat)
{
    int iPosition = m_formats.indexOf(mediumFormat);
    if (iPosition >= 0)
    {
        m_pFormatButtonGroup->button(iPosition)->click();
        m_pFormatButtonGroup->button(iPosition)->setFocus();
    }
}

UIWizardNewVDPageBasic1::UIWizardNewVDPageBasic1()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QVBoxLayout *pFormatLayout = new QVBoxLayout;
        {
            m_pFormatButtonGroup = new QButtonGroup(this);
            {
                /* Enumerate medium formats in special order: */
                CSystemProperties properties = vboxGlobal().virtualBox().GetSystemProperties();
                const QVector<CMediumFormat> &formats = properties.GetMediumFormats();
                QMap<QString, CMediumFormat> vdi, preferred;
                foreach (const CMediumFormat &format, formats)
                {
                    /* VDI goes first: */
                    if (format.GetName() == "VDI")
                        vdi[format.GetId()] = format;
                    else
                    {
                        const QVector<KMediumFormatCapabilities> &capabilities = format.GetCapabilities();
                        /* Then preferred: */
                        if (capabilities.contains(KMediumFormatCapabilities_Preferred))
                            preferred[format.GetId()] = format;
                    }
                }

                /* Create buttons for VDI and preferred: */
                foreach (const QString &strId, vdi.keys())
                    addFormatButton(this, pFormatLayout, vdi.value(strId));
                foreach (const QString &strId, preferred.keys())
                    addFormatButton(this, pFormatLayout, preferred.value(strId));

                if (!m_pFormatButtonGroup->buttons().isEmpty())
                {
                    m_pFormatButtonGroup->button(0)->click();
                    m_pFormatButtonGroup->button(0)->setFocus();
                }
            }
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pFormatLayout);
        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pFormatButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SIGNAL(completeChanged()));

    /* Register classes: */
    qRegisterMetaType<CMediumFormat>();
    /* Register fields: */
    registerField("mediumFormat", this, "mediumFormat");
}

void UIWizardNewVDPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardNewVD::tr("Hard disk file type"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardNewVD::tr("Please choose the type of file that you would like to use "
                                        "for the new virtual hard disk. If you do not need to use it "
                                        "with other virtualization software you can leave this setting unchanged."));
    QList<QAbstractButton*> buttons = m_pFormatButtonGroup->buttons();
    for (int i = 0; i < buttons.size(); ++i)
    {
        QAbstractButton *pButton = buttons[i];
        pButton->setText(VBoxGlobal::fullMediumFormatName(m_formatNames[m_pFormatButtonGroup->id(pButton)]));
    }
}

void UIWizardNewVDPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardNewVDPageBasic1::isComplete() const
{
    /* Make sure medium format is correct: */
    return !mediumFormat().isNull();
}

int UIWizardNewVDPageBasic1::nextId() const
{
    /* Show variant page only if there is something to show: */
    CMediumFormat mf = mediumFormat();
    if (mf.isNull())
    {
        AssertMsgFailed(("No medium format set!"));
    }
    else
    {
        ULONG uCapabilities = 0;
        QVector<KMediumFormatCapabilities> capabilities;
        capabilities = mf.GetCapabilities();
        for (int i = 0; i < capabilities.size(); i++)
            uCapabilities |= capabilities[i];

        int cTest = 0;
        if (uCapabilities & KMediumFormatCapabilities_CreateDynamic)
            ++cTest;
        if (uCapabilities & KMediumFormatCapabilities_CreateFixed)
            ++cTest;
        if (uCapabilities & KMediumFormatCapabilities_CreateSplit2G)
            ++cTest;
        if (cTest > 1)
            return UIWizardNewVD::Page2;
    }
    /* Skip otherwise: */
    return UIWizardNewVD::Page3;
}


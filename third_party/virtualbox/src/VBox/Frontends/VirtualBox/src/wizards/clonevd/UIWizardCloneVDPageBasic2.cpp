/* $Id: UIWizardCloneVDPageBasic2.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic2 class implementation.
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
# include "UIWizardCloneVDPageBasic2.h"
# include "UIWizardCloneVD.h"
# include "VBoxGlobal.h"
# include "QIRichTextLabel.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVDPage2::UIWizardCloneVDPage2()
{
}

void UIWizardCloneVDPage2::addFormatButton(QWidget *pParent, QVBoxLayout *pFormatLayout, KDeviceType enmDeviceType, CMediumFormat comMediumFormat, bool fPreferred /* = false */)
{
    /* Check that medium format supports creation: */
    ULONG uFormatCapabilities = 0;
    QVector<KMediumFormatCapabilities> capabilities;
    capabilities = comMediumFormat.GetCapabilities();
    for (int i = 0; i < capabilities.size(); i++)
        uFormatCapabilities |= capabilities[i];

    if (!(uFormatCapabilities & KMediumFormatCapabilities_CreateFixed ||
          uFormatCapabilities & KMediumFormatCapabilities_CreateDynamic))
        return;

    /* Check that medium format supports creation of virtual disk images: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    comMediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    if (!deviceTypes.contains(enmDeviceType))
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
        m_formats << comMediumFormat;
        m_formatNames << comMediumFormat.GetName();
        m_pFormatButtonGroup->addButton(pFormatButton, m_formatNames.size() - 1);
    }
}

CMediumFormat UIWizardCloneVDPage2::mediumFormat() const
{
    return m_pFormatButtonGroup->checkedButton() ? m_formats[m_pFormatButtonGroup->checkedId()] : CMediumFormat();
}

void UIWizardCloneVDPage2::setMediumFormat(const CMediumFormat &comMediumFormat)
{
    int iPosition = m_formats.indexOf(comMediumFormat);
    if (iPosition >= 0)
    {
        m_pFormatButtonGroup->button(iPosition)->click();
        m_pFormatButtonGroup->button(iPosition)->setFocus();
    }
}

UIWizardCloneVDPageBasic2::UIWizardCloneVDPageBasic2(KDeviceType enmDeviceType)
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
                QMap<QString, CMediumFormat> vdi, preferred, others;
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
                        /* Then others: */
                        else
                            others[format.GetId()] = format;
                    }
                }

                /* Create buttons for VDI and preferred: */
                foreach (const QString &strId, vdi.keys())
                    addFormatButton(this, pFormatLayout, enmDeviceType, vdi.value(strId));
                foreach (const QString &strId, preferred.keys())
                    addFormatButton(this, pFormatLayout, enmDeviceType, preferred.value(strId));
                if (enmDeviceType == KDeviceType_DVD || enmDeviceType == KDeviceType_Floppy)
                    foreach (const QString &strId, others.keys())
                        addFormatButton(this, pFormatLayout, enmDeviceType, others.value(strId));

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

void UIWizardCloneVDPageBasic2::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("Disk image file type"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardCloneVD::tr("Please choose the type of file that you would like to use "
                                          "for the new virtual disk image. If you do not need to use it "
                                          "with other virtualization software you can leave this setting unchanged."));
    QList<QAbstractButton*> buttons = m_pFormatButtonGroup->buttons();
    for (int i = 0; i < buttons.size(); ++i)
    {
        QAbstractButton *pButton = buttons[i];
        pButton->setText(VBoxGlobal::fullMediumFormatName(m_formatNames[m_pFormatButtonGroup->id(pButton)]));
    }
}

void UIWizardCloneVDPageBasic2::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardCloneVDPageBasic2::isComplete() const
{
    /* Make sure medium format is correct: */
    return !mediumFormat().isNull();
}

int UIWizardCloneVDPageBasic2::nextId() const
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
            return UIWizardCloneVD::Page3;
    }
    /* Skip otherwise: */
    return UIWizardCloneVD::Page4;
}


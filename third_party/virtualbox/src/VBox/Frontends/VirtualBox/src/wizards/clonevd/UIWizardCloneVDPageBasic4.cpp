/* $Id: UIWizardCloneVDPageBasic4.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic4 class implementation.
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
# include <QDir>
# include <QVBoxLayout>
# include <QHBoxLayout>
# include <QLineEdit>

/* GUI includes: */
# include "UIWizardCloneVDPageBasic4.h"
# include "UIWizardCloneVD.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIIconPool.h"
# include "QIFileDialog.h"
# include "QIRichTextLabel.h"
# include "QIToolButton.h"

/* COM includes: */
# include "CMediumFormat.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVDPage4::UIWizardCloneVDPage4()
{
}

void UIWizardCloneVDPage4::onSelectLocationButtonClicked()
{
    /* Get current folder and filename: */
    QFileInfo fullFilePath(mediumPath());
    QDir folder = fullFilePath.path();
    QString strFileName = fullFilePath.fileName();

    /* Set the first parent folder that exists as the current: */
    while (!folder.exists() && !folder.isRoot())
    {
        QFileInfo folderInfo(folder.absolutePath());
        if (folder == QDir(folderInfo.absolutePath()))
            break;
        folder = folderInfo.absolutePath();
    }

    /* But if it doesn't exists at all: */
    if (!folder.exists() || folder.isRoot())
    {
        /* Use recommended one folder: */
        QFileInfo defaultFilePath(absoluteFilePath(strFileName, m_strDefaultPath));
        folder = defaultFilePath.path();
    }

    /* Prepare backends list: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    CMediumFormat mediumFormat = fieldImp("mediumFormat").value<CMediumFormat>();
    mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    QStringList validExtensionList;
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (deviceTypes[i] == static_cast<UIWizardCloneVD*>(wizardImp())->sourceVirtualDiskDeviceType())
            validExtensionList << QString("*.%1").arg(fileExtensions[i]);
    /* Compose full filter list: */
    QString strBackendsList = QString("%1 (%2)").arg(mediumFormat.GetName()).arg(validExtensionList.join(" "));

    /* Open corresponding file-dialog: */
    QString strChosenFilePath = QIFileDialog::getSaveFileName(folder.absoluteFilePath(strFileName),
                                                              strBackendsList, thisImp(),
                                                              UIWizardCloneVD::tr("Please choose a location for new virtual disk image file"));

    /* If there was something really chosen: */
    if (!strChosenFilePath.isEmpty())
    {
        /* If valid file extension is missed, append it: */
        if (QFileInfo(strChosenFilePath).suffix().isEmpty())
            strChosenFilePath += QString(".%1").arg(m_strDefaultExtension);
        m_pDestinationDiskEditor->setText(QDir::toNativeSeparators(strChosenFilePath));
        m_pDestinationDiskEditor->selectAll();
        m_pDestinationDiskEditor->setFocus();
    }
}

/* static */
QString UIWizardCloneVDPage4::toFileName(const QString &strName, const QString &strExtension)
{
    /* Convert passed name to native separators (it can be full, actually): */
    QString strFileName = QDir::toNativeSeparators(strName);

    /* Remove all trailing dots to avoid multiple dots before extension: */
    int iLen;
    while (iLen = strFileName.length(), iLen > 0 && strFileName[iLen - 1] == '.')
        strFileName.truncate(iLen - 1);

    /* Add passed extension if its not done yet: */
    if (QFileInfo(strFileName).suffix().toLower() != strExtension)
        strFileName += QString(".%1").arg(strExtension);

    /* Return result: */
    return strFileName;
}

/* static */
QString UIWizardCloneVDPage4::absoluteFilePath(const QString &strFileName, const QString &strDefaultPath)
{
    /* Wrap file-info around received file name: */
    QFileInfo fileInfo(strFileName);
    /* If path-info is relative or there is no path-info at all: */
    if (fileInfo.fileName() == strFileName || fileInfo.isRelative())
    {
        /* Resolve path on the basis of default path we have: */
        fileInfo = QFileInfo(strDefaultPath, strFileName);
    }
    /* Return full absolute disk image file path: */
    return QDir::toNativeSeparators(fileInfo.absoluteFilePath());
}

/* static */
void UIWizardCloneVDPage4::acquireExtensions(const CMediumFormat &comMediumFormat, KDeviceType enmDeviceType,
                                             QStringList &aAllowedExtensions, QString &strDefaultExtension)
{
    /* Load extension / device list: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    CMediumFormat mediumFormat(comMediumFormat);
    mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (deviceTypes[i] == enmDeviceType)
            aAllowedExtensions << fileExtensions[i].toLower();
    AssertReturnVoid(!aAllowedExtensions.isEmpty());
    strDefaultExtension = aAllowedExtensions.first();
}

QString UIWizardCloneVDPage4::mediumPath() const
{
    /* Acquire chosen file path, and what is important user suffix: */
    const QString strChosenFilePath = m_pDestinationDiskEditor->text();
    QString strSuffix = QFileInfo(strChosenFilePath).suffix().toLower();
    /* If there is no suffix of it's not allowed: */
    if (   strSuffix.isEmpty()
        || !m_aAllowedExtensions.contains(strSuffix))
        strSuffix = m_strDefaultExtension;
    /* Compose full file path finally: */
    return absoluteFilePath(toFileName(m_pDestinationDiskEditor->text(), strSuffix), m_strDefaultPath);
}

qulonglong UIWizardCloneVDPage4::mediumSize() const
{
    const CMedium &sourceVirtualDisk = fieldImp("sourceVirtualDisk").value<CMedium>();
    return sourceVirtualDisk.isNull() ? 0 : sourceVirtualDisk.GetLogicalSize();
}

UIWizardCloneVDPageBasic4::UIWizardCloneVDPageBasic4()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QHBoxLayout *pLocationLayout = new QHBoxLayout;
        {
            m_pDestinationDiskEditor = new QLineEdit(this);
            m_pDestinationDiskOpenButton = new QIToolButton(this);
            {
                m_pDestinationDiskOpenButton->setAutoRaise(true);
                m_pDestinationDiskOpenButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", "select_file_disabled_16px.png"));
            }
            pLocationLayout->addWidget(m_pDestinationDiskEditor);
            pLocationLayout->addWidget(m_pDestinationDiskOpenButton);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pLocationLayout);
        pMainLayout->addStretch();
    }

    /* Setup page connections: */
    connect(m_pDestinationDiskEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pDestinationDiskOpenButton, SIGNAL(clicked()), this, SLOT(sltSelectLocationButtonClicked()));

    /* Register fields: */
    registerField("mediumPath", this, "mediumPath");
    registerField("mediumSize", this, "mediumSize");
}

void UIWizardCloneVDPageBasic4::sltSelectLocationButtonClicked()
{
    /* Call to base-class: */
    onSelectLocationButtonClicked();
}

void UIWizardCloneVDPageBasic4::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("New disk image to create"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardCloneVD::tr("Please type the name of the new virtual disk image file into the box below or "
                                          "click on the folder icon to select a different folder to create the file in."));
    m_pDestinationDiskOpenButton->setToolTip(UIWizardCloneVD::tr("Choose a location for new virtual disk image file..."));
}

void UIWizardCloneVDPageBasic4::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Get source virtual-disk file-information: */
    QFileInfo sourceFileInfo(field("sourceVirtualDisk").value<CMedium>().GetLocation());
    /* Get default path for virtual-disk copy: */
    m_strDefaultPath = sourceFileInfo.absolutePath();
    /* Get default extension for virtual-disk copy: */
    acquireExtensions(field("mediumFormat").value<CMediumFormat>(),
                      static_cast<UIWizardCloneVD*>(wizardImp())->sourceVirtualDiskDeviceType(),
                      m_aAllowedExtensions, m_strDefaultExtension);
    /* Compose default-name for virtual-disk copy: */
    QString strMediumName = UIWizardCloneVD::tr("%1_copy", "copied virtual disk image name").arg(sourceFileInfo.completeBaseName());
    /* Set default-name as text for location editor: */
    m_pDestinationDiskEditor->setText(strMediumName);
}

bool UIWizardCloneVDPageBasic4::isComplete() const
{
    /* Make sure current name is not empty: */
    return !m_pDestinationDiskEditor->text().trimmed().isEmpty();
}

bool UIWizardCloneVDPageBasic4::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Make sure such file doesn't exists already: */
    QString strMediumPath(mediumPath());
    fResult = !QFileInfo(strMediumPath).exists();
    if (!fResult)
        msgCenter().cannotOverwriteHardDiskStorage(strMediumPath, this);

    if (fResult)
    {
        /* Lock finish button: */
        startProcessing();

        /* Try to copy virtual disk image file: */
        fResult = qobject_cast<UIWizardCloneVD*>(wizard())->copyVirtualDisk();

        /* Unlock finish button: */
        endProcessing();
    }

    /* Return result: */
    return fResult;
}


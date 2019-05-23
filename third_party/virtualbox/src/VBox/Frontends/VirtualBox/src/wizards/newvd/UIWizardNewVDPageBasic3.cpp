/* $Id: UIWizardNewVDPageBasic3.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVDPageBasic3 class implementation.
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
# include <QRegExpValidator>
# include <QVBoxLayout>
# include <QHBoxLayout>
# include <QLineEdit>
# include <QSlider>
# include <QLabel>
# include <QSpacerItem>

/* GUI includes: */
# include "UIWizardNewVDPageBasic3.h"
# include "UIWizardNewVD.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIIconPool.h"
# include "QIFileDialog.h"
# include "QIRichTextLabel.h"
# include "QIToolButton.h"
# include "QILineEdit.h"
# include "UIMediumSizeEditor.h"

/* COM includes: */
# include "CSystemProperties.h"
# include "CMediumFormat.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardNewVDPage3::UIWizardNewVDPage3(const QString &strDefaultName, const QString &strDefaultPath)
    : m_strDefaultName(strDefaultName.isEmpty() ? QString("NewVirtualDisk1") : strDefaultName)
    , m_strDefaultPath(strDefaultPath)
    , m_uMediumSizeMin(_4M)
    , m_uMediumSizeMax(vboxGlobal().virtualBox().GetSystemProperties().GetInfoVDSize())
{
}

void UIWizardNewVDPage3::onSelectLocationButtonClicked()
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
        if (deviceTypes[i] == KDeviceType_HardDisk)
            validExtensionList << QString("*.%1").arg(fileExtensions[i]);
    /* Compose full filter list: */
    QString strBackendsList = QString("%1 (%2)").arg(mediumFormat.GetName()).arg(validExtensionList.join(" "));

    /* Open corresponding file-dialog: */
    QString strChosenFilePath = QIFileDialog::getSaveFileName(folder.absoluteFilePath(strFileName),
                                                              strBackendsList, thisImp(),
                                                              VBoxGlobal::tr("Please choose a location for new virtual hard disk file"));

    /* If there was something really chosen: */
    if (!strChosenFilePath.isEmpty())
    {
        /* If valid file extension is missed, append it: */
        if (QFileInfo(strChosenFilePath).suffix().isEmpty())
            strChosenFilePath += QString(".%1").arg(m_strDefaultExtension);
        m_pLocationEditor->setText(QDir::toNativeSeparators(strChosenFilePath));
        m_pLocationEditor->selectAll();
        m_pLocationEditor->setFocus();
    }
}

/* static */
QString UIWizardNewVDPage3::toFileName(const QString &strName, const QString &strExtension)
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
QString UIWizardNewVDPage3::absoluteFilePath(const QString &strFileName, const QString &strDefaultPath)
{
    /* Wrap file-info around received file name: */
    QFileInfo fileInfo(strFileName);
    /* If path-info is relative or there is no path-info at all: */
    if (fileInfo.fileName() == strFileName || fileInfo.isRelative())
    {
        /* Resolve path on the basis of default path we have: */
        fileInfo = QFileInfo(strDefaultPath, strFileName);
    }
    /* Return full absolute hard disk file path: */
    return QDir::toNativeSeparators(fileInfo.absoluteFilePath());
}

/* static */
QString UIWizardNewVDPage3::defaultExtension(const CMediumFormat &mediumFormatRef)
{
    /* Load extension / device list: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    CMediumFormat mediumFormat(mediumFormatRef);
    mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (deviceTypes[i] == KDeviceType_HardDisk)
            return fileExtensions[i].toLower();
    AssertMsgFailed(("Extension can't be NULL!\n"));
    return QString();
}

QString UIWizardNewVDPage3::mediumPath() const
{
    return absoluteFilePath(toFileName(m_pLocationEditor->text(), m_strDefaultExtension), m_strDefaultPath);
}

qulonglong UIWizardNewVDPage3::mediumSize() const
{
    return m_pEditorSize->mediumSize();
}

void UIWizardNewVDPage3::setMediumSize(qulonglong uMediumSize)
{
    m_pEditorSize->setMediumSize(uMediumSize);
}

UIWizardNewVDPageBasic3::UIWizardNewVDPageBasic3(const QString &strDefaultName, const QString &strDefaultPath, qulonglong uDefaultSize)
    : UIWizardNewVDPage3(strDefaultName, strDefaultPath)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLocationLabel = new QIRichTextLabel(this);
        QHBoxLayout *pLocationLayout = new QHBoxLayout;
        {
            m_pLocationEditor = new QLineEdit(this);
            m_pLocationOpenButton = new QIToolButton(this);
            {
                m_pLocationOpenButton->setAutoRaise(true);
                m_pLocationOpenButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", "select_file_disabled_16px.png"));
            }
            pLocationLayout->addWidget(m_pLocationEditor);
            pLocationLayout->addWidget(m_pLocationOpenButton);
        }
        m_pSizeLabel = new QIRichTextLabel(this);
        m_pEditorSize = new UIMediumSizeEditor;
        setMediumSize(uDefaultSize);
        pMainLayout->addWidget(m_pLocationLabel);
        pMainLayout->addLayout(pLocationLayout);
        pMainLayout->addWidget(m_pSizeLabel);
        pMainLayout->addWidget(m_pEditorSize);
        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pLocationEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pLocationOpenButton, SIGNAL(clicked()), this, SLOT(sltSelectLocationButtonClicked()));
    connect(m_pEditorSize, &UIMediumSizeEditor::sigSizeChanged, this, &UIWizardNewVDPageBasic3::completeChanged);

    /* Register fields: */
    registerField("mediumPath", this, "mediumPath");
    registerField("mediumSize", this, "mediumSize");
}

void UIWizardNewVDPageBasic3::sltSelectLocationButtonClicked()
{
    /* Call to base-class: */
    onSelectLocationButtonClicked();
}

void UIWizardNewVDPageBasic3::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardNewVD::tr("File location and size"));

    /* Translate widgets: */
    m_pLocationLabel->setText(UIWizardNewVD::tr("Please type the name of the new virtual hard disk file into the box below or "
                                                "click on the folder icon to select a different folder to create the file in."));
    m_pLocationOpenButton->setToolTip(UIWizardNewVD::tr("Choose a location for new virtual hard disk file..."));
    m_pSizeLabel->setText(UIWizardNewVD::tr("Select the size of the virtual hard disk in megabytes. "
                                            "This size is the limit on the amount of file data "
                                            "that a virtual machine will be able to store on the hard disk."));
}

void UIWizardNewVDPageBasic3::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Get default extension for new virtual-disk: */
    m_strDefaultExtension = defaultExtension(field("mediumFormat").value<CMediumFormat>());
    /* Set default name as text for location editor: */
    m_pLocationEditor->setText(m_strDefaultName);
}

bool UIWizardNewVDPageBasic3::isComplete() const
{
    /* Make sure current name is not empty and current size feats the bounds: */
    return !m_pLocationEditor->text().trimmed().isEmpty() &&
           mediumSize() >= m_uMediumSizeMin && mediumSize() <= m_uMediumSizeMax;
}

bool UIWizardNewVDPageBasic3::validatePage()
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

        /* Try to create virtual hard drive file: */
        fResult = qobject_cast<UIWizardNewVD*>(wizard())->createVirtualDisk();

        /* Unlock finish button: */
        endProcessing();
    }

    /* Return result: */
    return fResult;
}


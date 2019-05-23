/* $Id: UIMachineSettingsSFDetails.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsSFDetails class implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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

/* Qt includes */
# include <QDir>
# include <QPushButton>

/* Other includes */
# include "UIMachineSettingsSFDetails.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIMachineSettingsSFDetails::UIMachineSettingsSFDetails(DialogType type,
                                                       bool fEnableSelector, /* for "permanent" checkbox */
                                                       const QStringList &usedNames,
                                                       QWidget *pParent /* = 0 */)
   : QIWithRetranslateUI2<QIDialog>(pParent)
   , m_type(type)
   , m_fUsePermanent(fEnableSelector)
   , m_usedNames(usedNames)
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsSFDetails::setupUi(this);

    /* Setup widgets: */
    mPsPath->setResetEnabled(false);
    mPsPath->setHomeDir(QDir::homePath());
    mCbPermanent->setHidden(!fEnableSelector);

    /* Setup connections: */
    connect(mPsPath, SIGNAL(currentIndexChanged(int)), this, SLOT(sltSelectPath()));
    connect(mPsPath, SIGNAL(pathChanged(const QString &)), this, SLOT(sltSelectPath()));
    connect(mLeName, SIGNAL(textChanged(const QString &)), this, SLOT(sltValidate()));
    if (fEnableSelector)
        connect(mCbPermanent, SIGNAL(toggled(bool)), this, SLOT(sltValidate()));

     /* Applying language settings: */
    retranslateUi();

    /* Validate the initial field values: */
    sltValidate();

    /* Adjust dialog size: */
    adjustSize();

#ifdef VBOX_WS_MAC
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(minimumSize());
#endif /* VBOX_WS_MAC */
}

void UIMachineSettingsSFDetails::setPath(const QString &strPath)
{
    mPsPath->setPath(strPath);
}

QString UIMachineSettingsSFDetails::path() const
{
    return mPsPath->path();
}

void UIMachineSettingsSFDetails::setName(const QString &strName)
{
    mLeName->setText(strName);
}

QString UIMachineSettingsSFDetails::name() const
{
    return mLeName->text();
}

void UIMachineSettingsSFDetails::setWriteable(bool fWritable)
{
    mCbReadonly->setChecked(!fWritable);
}

bool UIMachineSettingsSFDetails::isWriteable() const
{
    return !mCbReadonly->isChecked();
}

void UIMachineSettingsSFDetails::setAutoMount(bool fAutoMount)
{
    mCbAutoMount->setChecked(fAutoMount);
}

bool UIMachineSettingsSFDetails::isAutoMounted() const
{
    return mCbAutoMount->isChecked();
}

void UIMachineSettingsSFDetails::setPermanent(bool fPermanent)
{
    mCbPermanent->setChecked(fPermanent);
}

bool UIMachineSettingsSFDetails::isPermanent() const
{
    return m_fUsePermanent ? mCbPermanent->isChecked() : true;
}

void UIMachineSettingsSFDetails::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIMachineSettingsSFDetails::retranslateUi(this);

    switch (m_type)
    {
        case AddType:
            setWindowTitle(tr("Add Share"));
            break;
        case EditType:
            setWindowTitle(tr("Edit Share"));
            break;
        default:
            AssertMsgFailed(("Incorrect shared-folders dialog type: %d\n", m_type));
    }
}

void UIMachineSettingsSFDetails::sltValidate()
{
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!mPsPath->path().isEmpty() &&
                                                         QDir(mPsPath->path()).exists() &&
                                                         !mLeName->text().trimmed().isEmpty() &&
                                                         !mLeName->text().contains(" ") &&
                                                         !m_usedNames.contains(mLeName->text()));
}

void UIMachineSettingsSFDetails::sltSelectPath()
{
    if (!mPsPath->isPathSelected())
        return;

    QString strFolderName(mPsPath->path());
#if defined (VBOX_WS_WIN) || defined (Q_OS_OS2)
    if (strFolderName[0].isLetter() && strFolderName[1] == ':' && strFolderName[2] == 0)
    {
        /* UIFilePathSelector returns root path as 'X:', which is invalid path.
         * Append the trailing backslash to get a valid root path 'X:\': */
        strFolderName += "\\";
        mPsPath->setPath(strFolderName);
    }
#endif /* VBOX_WS_WIN || Q_OS_OS2 */
    QDir folder(strFolderName);
    if (!folder.isRoot())
    {
        /* Processing non-root folder */
        mLeName->setText(folder.dirName().replace(' ', '_'));
    }
    else
    {
        /* Processing root folder: */
#if defined (VBOX_WS_WIN) || defined (Q_OS_OS2)
        mLeName->setText(strFolderName.toUpper()[0] + "_DRIVE");
#elif defined (VBOX_WS_X11)
        mLeName->setText("ROOT");
#endif
    }
    /* Validate the field values: */
    sltValidate();
}


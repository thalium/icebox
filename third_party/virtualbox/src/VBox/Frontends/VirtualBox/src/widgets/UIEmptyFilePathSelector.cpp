/* $Id: UIEmptyFilePathSelector.cpp $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: UIEmptyFilePathSelector class implementation.
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

/* Local includes */
# include "QIFileDialog.h"
# include "QIToolButton.h"
# include "QILabel.h"
# include "QILineEdit.h"
# include "UIIconPool.h"
# include "UIEmptyFilePathSelector.h"
# include "VBoxGlobal.h"

/* Global includes */
# include <iprt/assert.h>
# include <QAction>
# include <QApplication>
# include <QClipboard>
# include <QDir>
# include <QFocusEvent>
# include <QHBoxLayout>
# include <QLineEdit>
# include <QTimer>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIEmptyFilePathSelector::UIEmptyFilePathSelector (QWidget *aParent /* = NULL */)
    : QIWithRetranslateUI<QWidget> (aParent)
    , mPathWgt (NULL)
    , mLabel (NULL)
    , mMode (UIEmptyFilePathSelector::Mode_File_Open)
    , mLineEdit (NULL)
    , m_fButtonToolTipSet(false)
    , mHomeDir (QDir::current().absolutePath())
    , mIsModified (false)
{
    mMainLayout = new QHBoxLayout (this);
    mMainLayout->setMargin (0);

    mSelectButton = new QIToolButton(this);
    mSelectButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", ":/select_file_disabled_16px.png"));
    connect(mSelectButton, SIGNAL(clicked()), this, SLOT(choose()));
    mMainLayout->addWidget(mSelectButton);

    setEditable (false);

    retranslateUi();
}

void UIEmptyFilePathSelector::setMode (UIEmptyFilePathSelector::Mode aMode)
{
    mMode = aMode;
}

UIEmptyFilePathSelector::Mode UIEmptyFilePathSelector::mode() const
{
    return mMode;
}

void UIEmptyFilePathSelector::setButtonPosition (ButtonPosition aPos)
{
    if (aPos == LeftPosition)
    {
        mMainLayout->setDirection (QBoxLayout::LeftToRight);
        setTabOrder (mSelectButton, mPathWgt);
    }
    else
    {
        mMainLayout->setDirection (QBoxLayout::RightToLeft);
        setTabOrder (mPathWgt, mSelectButton);
    }
}

UIEmptyFilePathSelector::ButtonPosition UIEmptyFilePathSelector::buttonPosition() const
{
    return mMainLayout->direction() == QBoxLayout::LeftToRight ? LeftPosition : RightPosition;
}

void UIEmptyFilePathSelector::setEditable (bool aOn)
{
    if (mPathWgt)
    {
        delete mPathWgt;
        mLabel = NULL;
        mLineEdit = NULL;
    }

    if (aOn)
    {
        mPathWgt = mLineEdit = new QILineEdit (this);
        connect (mLineEdit, SIGNAL (textChanged (const QString&)),
                 this, SLOT (textChanged (const QString&)));
    }
    else
    {
        mPathWgt = mLabel = new QILabel (this);
        mLabel->setWordWrap (true);
    }
    mMainLayout->addWidget (mPathWgt, 2);
    setButtonPosition (buttonPosition());

    setPath (mPath);
}

bool UIEmptyFilePathSelector::isEditable() const
{
    return mLabel ? false : true;
}

void UIEmptyFilePathSelector::setChooserVisible (bool aOn)
{
    mSelectButton->setVisible (aOn);
}

bool UIEmptyFilePathSelector::isChooserVisible() const
{
    return mSelectButton->isVisible();
}

void UIEmptyFilePathSelector::setPath (const QString& aPath)
{
    QString tmpPath = QDir::toNativeSeparators (aPath);
    if (mLabel)
        mLabel->setText (QString ("<compact elipsis=\"start\">%1</compact>").arg (tmpPath));
    else if (mLineEdit)
        mLineEdit->setText (tmpPath);
    textChanged(tmpPath);
}

QString UIEmptyFilePathSelector::path() const
{
    return mPath;
}

void UIEmptyFilePathSelector::setDefaultSaveExt (const QString &aExt)
{
    mDefaultSaveExt = aExt;
}

QString UIEmptyFilePathSelector::defaultSaveExt() const
{
    return mDefaultSaveExt;
}

void UIEmptyFilePathSelector::setChooseButtonToolTip(const QString &strToolTip)
{
    m_fButtonToolTipSet = !strToolTip.isEmpty();
    mSelectButton->setToolTip(strToolTip);
}

QString UIEmptyFilePathSelector::chooseButtonToolTip() const
{
    return mSelectButton->toolTip();
}

void UIEmptyFilePathSelector::setFileDialogTitle (const QString& aTitle)
{
    mFileDialogTitle = aTitle;
}

QString UIEmptyFilePathSelector::fileDialogTitle() const
{
    return mFileDialogTitle;
}

void UIEmptyFilePathSelector::setFileFilters (const QString& aFilters)
{
    mFileFilters = aFilters;
}

QString UIEmptyFilePathSelector::fileFilters() const
{
    return mFileFilters;
}

void UIEmptyFilePathSelector::setHomeDir (const QString& aDir)
{
    mHomeDir = aDir;
}

QString UIEmptyFilePathSelector::homeDir() const
{
    return mHomeDir;
}

void UIEmptyFilePathSelector::retranslateUi()
{
    if (!m_fButtonToolTipSet)
        mSelectButton->setToolTip(tr("Choose..."));
}

void UIEmptyFilePathSelector::choose()
{
    QString path = mPath;

    /* Preparing initial directory. */
    QString initDir = path.isNull() ? mHomeDir :
        QIFileDialog::getFirstExistingDir (path);
    if (initDir.isNull())
        initDir = mHomeDir;

    switch (mMode)
    {
        case UIEmptyFilePathSelector::Mode_File_Open:
            path = QIFileDialog::getOpenFileName (initDir, mFileFilters, parentWidget(), mFileDialogTitle); break;
        case UIEmptyFilePathSelector::Mode_File_Save:
        {
            path = QIFileDialog::getSaveFileName (initDir, mFileFilters, parentWidget(), mFileDialogTitle);
            if (!path.isEmpty() && QFileInfo (path).suffix().isEmpty())
                path = QString ("%1.%2").arg (path).arg (mDefaultSaveExt);
            break;
        }
        case UIEmptyFilePathSelector::Mode_Folder:
            path = QIFileDialog::getExistingDirectory (initDir, parentWidget(), mFileDialogTitle); break;
    }
    if (path.isEmpty())
        return;

    path.remove (QRegExp ("[\\\\/]$"));
    setPath (path);
}

void UIEmptyFilePathSelector::textChanged (const QString& aPath)
{
    const QString oldPath = mPath;
    mPath = aPath;
    if (oldPath != mPath)
    {
        mIsModified = true;
        emit pathChanged (mPath);
    }
}


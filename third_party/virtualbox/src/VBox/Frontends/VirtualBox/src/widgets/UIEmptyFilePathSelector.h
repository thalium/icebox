/* $Id: UIEmptyFilePathSelector.h $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: UIEmptyFilePathSelector class declaration.
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

#ifndef __UIEmptyFilePathSelector_h__
#define __UIEmptyFilePathSelector_h__

/* VBox includes */
#include "QIWithRetranslateUI.h"

/* Qt includes */
#include <QComboBox>

/* VBox forward declarations */
class QILabel;
class QILineEdit;

/* Qt forward declarations */
class QHBoxLayout;
class QAction;
class QIToolButton;


class UIEmptyFilePathSelector: public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    enum Mode
    {
        Mode_Folder = 0,
        Mode_File_Open,
        Mode_File_Save
    };

    enum ButtonPosition
    {
        LeftPosition,
        RightPosition
    };

    UIEmptyFilePathSelector (QWidget *aParent = NULL);

    void setMode (UIEmptyFilePathSelector::Mode aMode);
    UIEmptyFilePathSelector::Mode mode() const;

    void setButtonPosition (ButtonPosition aPos);
    ButtonPosition buttonPosition() const;

    void setEditable (bool aOn);
    bool isEditable() const;

    void setChooserVisible (bool aOn);
    bool isChooserVisible() const;

    QString path() const;

    void setDefaultSaveExt (const QString &aExt);
    QString defaultSaveExt() const;

    bool isModified () const { return mIsModified; }
    void resetModified () { mIsModified = false; }

    void setChooseButtonToolTip(const QString &strToolTip);
    QString chooseButtonToolTip() const;

    void setFileDialogTitle (const QString& aTitle);
    QString fileDialogTitle() const;

    void setFileFilters (const QString& aFilters);
    QString fileFilters() const;

    void setHomeDir (const QString& aDir);
    QString homeDir() const;

signals:
    void pathChanged (QString);

public slots:
    void setPath (const QString& aPath);

protected:
    void retranslateUi();

private slots:
    void choose();
    void textChanged (const QString& aPath);

private:
    /* Private member vars */
    QHBoxLayout *mMainLayout;
    QWidget *mPathWgt;
    QILabel *mLabel;
    UIEmptyFilePathSelector::Mode mMode;
    QILineEdit *mLineEdit;
    QIToolButton *mSelectButton;
    bool m_fButtonToolTipSet;
    QString mFileDialogTitle;
    QString mFileFilters;
    QString mDefaultSaveExt;
    QString mHomeDir;
    bool mIsModified;
    QString mPath;
};

#endif /* !___UIEmptyFilePathSelector_h___ */


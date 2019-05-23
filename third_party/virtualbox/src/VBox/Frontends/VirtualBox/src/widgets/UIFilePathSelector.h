/* $Id: UIFilePathSelector.h $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: UIFilePathSelector class declaration.
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

#ifndef ___UIFilePathSelector_h___
#define ___UIFilePathSelector_h___

/* GUI includes: */
#include "QIComboBox.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QAction;
class QHBoxLayout;
class QILabel;
class QILineEdit;
class QIToolButton;


/** QIComboBox extension providing GUI with
  * possibility to choose/reflect file/folder path. */
class UIFilePathSelector: public QIWithRetranslateUI<QIComboBox>
{
    Q_OBJECT;

signals:

    /** Notify listeners about @a strPath changed. */
    void pathChanged(const QString &strPath);

public:

    /** Modes file-path selector operates in. */
    enum Mode
    {
        Mode_Folder = 0,
        Mode_File_Open,
        Mode_File_Save
    };

    /** Combo-box field IDs file-path selector uses. */
    enum
    {
        PathId = 0,
        SelectId,
        ResetId
    };

    /** Constructs file-path selector passing @a pParent to QIComboBox base-class. */
    UIFilePathSelector(QWidget *pParent = 0);

    /** Defines the @a enmMode to operate in. */
    void setMode(Mode enmMode);
    /** Returns the mode to operate in. */
    Mode mode() const { return m_enmMode; }

    /** Defines whether the path is @a fEditable. */
    void setEditable(bool fEditable);
    /** Returns whether the path is editable. */
    bool isEditable() const { return m_fEditable; }

    /** Defines whether the reseting to defauilt path is @a fEnabled. */
    void setResetEnabled(bool fEnabled);
    /** Returns whether the reseting to defauilt path is enabled. */
    bool isResetEnabled() const { return count() - 1  == ResetId; }

    /** Defines the file-dialog @a strTitle. */
    void setFileDialogTitle(const QString &strTitle) { m_strFileDialogTitle = strTitle; }
    /** Returns the file-dialog title. */
    QString fileDialogTitle() const { return m_strFileDialogTitle; }

    /** Defines the file-dialog @a strFilters. */
    void setFileDialogFilters(const QString &strFilters) { m_strFileDialogFilters = strFilters; }
    /** Returns the file-dialog filters. */
    QString fileDialogFilters() const { return m_strFileDialogFilters; }

    /** Defines the file-dialog @a strDefaultSaveExtension. */
    void setFileDialogDefaultSaveExtension(const QString &strDefaultSaveExtension) { m_strFileDialogDefaultSaveExtension = strDefaultSaveExtension; }
    /** Returns the file-dialog default save extension. */
    QString fileDialogDefaultSaveExtension() const { return m_strFileDialogDefaultSaveExtension; }

    /** Resets path modified state to false. */
    void resetModified() { m_fModified = false; }
    /** Returns whether the path is modified. */
    bool isModified() const { return m_fModified; }
    /** Returns whether the path is selected. */
    bool isPathSelected() const { return currentIndex() == PathId; }

    /** Returns the path. */
    QString path() const { return m_strPath; }

    /** Sets overriden widget's @a strToolTip.
      * @note If nothing set it's generated automatically. */
    void setToolTip(const QString &strToolTip);

public slots:

    /** Defines the @a strPath and @a fRefreshText after that. */
    void setPath(const QString &strPath, bool fRefreshText = true);

    /** Defines the @a strHomeDir. */
    void setHomeDir(const QString &strHomeDir) { m_strHomeDir = strHomeDir; }

protected:

    /** Preprocesses every @a pEvent sent to @a pObject. */
    bool eventFilter(QObject *pObject, QEvent *pEvent);

    /** Handles resize @a pEvent. */
    void resizeEvent(QResizeEvent *pEvent);

    /** Handles focus-in @a pEvent. */
    void focusInEvent(QFocusEvent *pEvent);
    /** Handles focus-out @a pEvent. */
    void focusOutEvent(QFocusEvent *pEvent);

    /** Handles translation event. */
    void retranslateUi();

private slots:

    /** Handles combo-box @a iIndex activation. */
    void onActivated(int iIndex);

    /** Handles combo-box @a strText editing. */
    void onTextEdited(const QString &strText);

    /** Handles combo-box text copying. */
    void copyToClipboard();

    /** Refreshes combo-box text according to chosen path. */
    void refreshText();

private:

    /** Provokes change to @a strPath and @a fRefreshText after that. */
    void changePath(const QString &strPath, bool fRefreshText = true);

    /** Call for file-dialog to choose path. */
    void selectPath();

    /** Returns default icon. */
    QIcon defaultIcon() const;

    /** Returns full path @a fAbsolute if necessary. */
    QString fullPath(bool fAbsolute = true) const;

    /** Shrinks the reflected text to @a iWidth pixels. */
    QString shrinkText(int iWidth) const;

    /** Holds the mode to operate in. */
    Mode     m_enmMode;

    /** Holds the path. */
    QString  m_strPath;
    /** Holds the home dir. */
    QString  m_strHomeDir;

    /** Holds the file-dialog title. */
    QString  m_strFileDialogTitle;
    /** Holds the file-dialog filters. */
    QString  m_strFileDialogFilters;
    /** Holds the file-dialog default save extension. */
    QString  m_strFileDialogDefaultSaveExtension;

    /** Holds the cached text for empty path. */
    QString  m_strNoneText;
    /** Holds the cached tool-tip for empty path. */
    QString  m_strNoneToolTip;
    /** Holds the cached tool-tip for empty path in focused case. */
    QString  m_strNoneToolTipFocused;

    /** Holds whether the path is editable. */
    bool     m_fEditable;
    /** Holds whether the path is modified. */
    bool     m_fModified;

    /** Holds whether we are in editable mode. */
    bool     m_fEditableMode;
    /** Holds whether we are expecting mouse events. */
    bool     m_fMouseAwaited;

    /** Holds whether the tool-tip overriden. */
    bool     m_fToolTipOverriden;

    /** Holds the copy action instance. */
    QAction *m_pCopyAction;
};

#endif /* !___UIFilePathSelector_h___ */


/* $Id: UIFilePathSelector.cpp $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: UIFilePathSelector class implementation.
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

/* Qt includes: */
# include <QAction>
# include <QApplication>
# include <QClipboard>
# include <QDir>
# include <QFocusEvent>
# include <QHBoxLayout>
# include <QLineEdit>
# ifdef VBOX_WS_WIN
#  include <QListView>
# endif /* VBOX_WS_WIN */

/* GUI includes: */
# include "QIFileDialog.h"
# include "QILabel.h"
# include "QILineEdit.h"
# include "QIToolButton.h"
# include "UIIconPool.h"
# include "UIFilePathSelector.h"
# include "VBoxGlobal.h"

/* Other VBox includes: */
# include <iprt/assert.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Returns first position of difference between passed strings. */
static int differFrom(const QString &str1, const QString &str2)
{
    if (str1 == str2)
        return -1;

    int iMinLength = qMin(str1.size(), str2.size());
    int iIndex = 0;
    for (iIndex = 0; iIndex < iMinLength; ++iIndex)
        if (str1[iIndex] != str2[iIndex])
            break;
    return iIndex;
}

UIFilePathSelector::UIFilePathSelector(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QIComboBox>(pParent)
    , m_enmMode(Mode_Folder)
    , m_strHomeDir(QDir::current().absolutePath())
    , m_fEditable(true)
    , m_fModified(false)
    , m_fEditableMode(false)
    , m_fMouseAwaited(false)
    , m_fToolTipOverriden(false)
    , m_pCopyAction(new QAction(this))
{
#ifdef VBOX_WS_WIN
    // WORKAROUND:
    // On at least Windows host there is a bug with
    // the QListView which doesn't take into account
    // the item size change caused by assigning item's
    // icon of another size or unassigning icon at all.
    if (view()->inherits("QListView"))
        qobject_cast<QListView*>(view())->setUniformItemSizes(true);
#endif /* VBOX_WS_WIN */

    /* Populate items: */
    insertItem(PathId, "");
    insertItem(SelectId, "");
    insertItem(ResetId, "");

    /* Attaching known icons: */
    setItemIcon(SelectId, UIIconPool::iconSet(":/select_file_16px.png"));
    setItemIcon(ResetId, UIIconPool::iconSet(":/eraser_16px.png"));

    /* Setup context menu: */
    addAction(m_pCopyAction);
    m_pCopyAction->setShortcut(QKeySequence(QKeySequence::Copy));
    m_pCopyAction->setShortcutContext(Qt::WidgetShortcut);

    /* Initial setup: */
    setInsertPolicy(QComboBox::NoInsert);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setMinimumWidth(200);

    /* Setup connections: */
    connect(this, SIGNAL(activated(int)), this, SLOT(onActivated(int)));
    connect(m_pCopyAction, SIGNAL(triggered(bool)), this, SLOT(copyToClipboard()));

    /* Editable by default: */
    setEditable(true);

    /* Applying language settings: */
    retranslateUi();
}

void UIFilePathSelector::setMode(Mode enmMode)
{
    m_enmMode = enmMode;

    retranslateUi();
}

void UIFilePathSelector::setEditable(bool fEditable)
{
    m_fEditable = fEditable;

    if (m_fEditable)
    {
        QIComboBox::setEditable(true);

        /* Install combo-box event-filter: */
        Assert(comboBox());
        comboBox()->installEventFilter(this);

        /* Install line-edit connection/event-filter: */
        Assert(lineEdit());
        connect(lineEdit(), SIGNAL(textEdited(const QString &)),
                this, SLOT(onTextEdited(const QString &)));
        lineEdit()->installEventFilter(this);
    }
    else
    {
        if (lineEdit())
        {
            /* Remove line-edit event-filter/connection: */
            lineEdit()->removeEventFilter(this);
            disconnect(lineEdit(), SIGNAL(textEdited(const QString &)),
                       this, SLOT(onTextEdited(const QString &)));
        }
        if (comboBox())
        {
            /* Remove combo-box event-filter: */
            comboBox()->removeEventFilter(this);
        }
        QIComboBox::setEditable(false);
    }
}

void UIFilePathSelector::setResetEnabled(bool fEnabled)
{
    if (!fEnabled && count() - 1 == ResetId)
        removeItem(ResetId);
    else if (fEnabled && count() - 1 == ResetId - 1)
    {
        insertItem(ResetId, "");
        setItemIcon(ResetId, UIIconPool::iconSet(":/eraser_16px.png"));
    }
    retranslateUi();
}

void UIFilePathSelector::setToolTip(const QString &strToolTip)
{
    /* Call to base-class: */
    QIComboBox::setToolTip(strToolTip);

    /* Remember if the tool-tip overriden: */
    m_fToolTipOverriden = !toolTip().isEmpty();
}

void UIFilePathSelector::setPath(const QString &strPath, bool fRefreshText /* = true */)
{
    m_strPath = strPath.isEmpty() ? QString::null :
            QDir::toNativeSeparators(strPath);
    if (fRefreshText)
        refreshText();
}

bool UIFilePathSelector::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* If the object is private combo-box: */
    if (pObject == comboBox())
    {
        /* Handle focus events related to private child: */
        switch (pEvent->type())
        {
            case QEvent::FocusIn:  focusInEvent(static_cast<QFocusEvent*>(pEvent)); break;
            case QEvent::FocusOut: focusOutEvent(static_cast<QFocusEvent*>(pEvent)); break;
            default: break;
        }
    }

    /* If the object is private line-edit: */
    if (pObject == lineEdit())
    {
        if (m_fMouseAwaited && (pEvent->type() == QEvent::MouseButtonPress))
            QMetaObject::invokeMethod(this, "refreshText", Qt::QueuedConnection);
    }

    /* Call to base-class: */
    return QIWithRetranslateUI<QIComboBox>::eventFilter(pObject, pEvent);
}

void UIFilePathSelector::resizeEvent(QResizeEvent *pEvent)
{
    QIWithRetranslateUI<QIComboBox>::resizeEvent(pEvent);
    refreshText();
}

void UIFilePathSelector::focusInEvent(QFocusEvent *pEvent)
{
    if (isPathSelected())
    {
        if (m_fEditable)
            m_fEditableMode = true;
        if (pEvent->reason() == Qt::MouseFocusReason)
            m_fMouseAwaited = true;
        else
            refreshText();
    }
    QIWithRetranslateUI<QIComboBox>::focusInEvent(pEvent);
}

void UIFilePathSelector::focusOutEvent(QFocusEvent *pEvent)
{
    if (isPathSelected())
    {
        m_fEditableMode = false;
        refreshText();
    }
    QIWithRetranslateUI<QIComboBox>::focusOutEvent(pEvent);
}

void UIFilePathSelector::retranslateUi()
{
    /* Retranslate copy action: */
    m_pCopyAction->setText(tr("&Copy"));

    /* Retranslate 'select' item: */
    setItemText(SelectId, tr("Other..."));

    /* Retranslate 'reset' item: */
    if (count() - 1 == ResetId)
        setItemText(ResetId, tr("Reset"));

    /* Set tool-tips of the above two items based on the mode: */
    switch (m_enmMode)
    {
        case Mode_Folder:
            setItemData(SelectId,
                        tr("Displays a window to select a different folder."),
                        Qt::ToolTipRole);
            setItemData(ResetId,
                        tr("Resets the folder path to the default value."),
                        Qt::ToolTipRole);
            break;
        case Mode_File_Open:
        case Mode_File_Save:
            setItemData(SelectId,
                        tr("Displays a window to select a different file."),
                        Qt::ToolTipRole);
            setItemData(ResetId,
                        tr("Resets the file path to the default value."),
                        Qt::ToolTipRole);
            break;
        default:
            AssertFailedBreak();
    }

    /* If selector is NOT focused => we interpret the "nothing selected"
     * item depending on "reset to default" feature state: */
    if (isResetEnabled())
    {
        /* If "reset to default" is enabled: */
        m_strNoneText = tr("<reset to default>");
        m_strNoneToolTip = tr("The actual default path value will be displayed after "
                              "accepting the changes and opening this window again.");
    }
    else
    {
        /* If "reset to default" is NOT enabled: */
        m_strNoneText = tr("<not selected>");
        m_strNoneToolTip = tr("Please use the <b>Other...</b> item from the drop-down "
                              "list to select a path.");
    }

    /* But if selector is focused => tool-tip depends on the mode only: */
    switch (m_enmMode)
    {
        case Mode_Folder:
            m_strNoneToolTipFocused = tr("Holds the folder path.");
            break;
        case Mode_File_Open:
        case Mode_File_Save:
            m_strNoneToolTipFocused = tr("Holds the file path.");
            break;
        default:
            AssertFailedBreak();
    }

    /* Finally, retranslate current item: */
    refreshText();
}

void UIFilePathSelector::onActivated(int iIndex)
{
    switch (iIndex)
    {
        case SelectId:
        {
            selectPath();
            break;
        }
        case ResetId:
        {
            changePath(QString::null);
            break;
        }
        default:
            break;
    }
    setCurrentIndex(PathId);
    setFocus();
}

void UIFilePathSelector::onTextEdited(const QString &strPath)
{
    changePath(strPath, false /* refresh text? */);
}

void UIFilePathSelector::copyToClipboard()
{
    QString text(fullPath());
    /* Copy the current text to the selection and global clipboard. */
    if (QApplication::clipboard()->supportsSelection())
        QApplication::clipboard()->setText(text, QClipboard::Selection);
    QApplication::clipboard()->setText(text, QClipboard::Clipboard);
}

void UIFilePathSelector::changePath(const QString &strPath,
                                    bool fRefreshText /* = true */)
{
    const QString strOldPath = m_strPath;
    setPath(strPath, fRefreshText);
    if (!m_fModified && m_strPath != strOldPath)
        m_fModified = true;
    emit pathChanged(strPath);
}

void UIFilePathSelector::selectPath()
{
    /* Prepare initial directory: */
    QString strInitDir;
    /* If something already chosen: */
    if (!m_strPath.isEmpty())
    {
        /* If that is just a single file/folder (object) name: */
        const QString strObjectName = QFileInfo(m_strPath).fileName();
        if (strObjectName == m_strPath)
        {
            /* Use the home directory: */
            strInitDir = m_strHomeDir;
        }
        /* If that is full file/folder (object) path: */
        else
        {
            /* Use the first existing dir of m_strPath: */
            strInitDir = QIFileDialog::getFirstExistingDir(m_strPath);
        }
        /* Finally, append object name itself: */
        strInitDir = QDir(strInitDir).absoluteFilePath(strObjectName);
    }
    /* Use the home directory by default: */
    if (strInitDir.isNull())
        strInitDir = m_strHomeDir;

    /* Open the choose-file/folder dialog: */
    QString strSelPath;
    switch (m_enmMode)
    {
        case Mode_File_Open:
            strSelPath = QIFileDialog::getOpenFileName(strInitDir, m_strFileDialogFilters, parentWidget(), m_strFileDialogTitle); break;
        case Mode_File_Save:
        {
            strSelPath = QIFileDialog::getSaveFileName(strInitDir, m_strFileDialogFilters, parentWidget(), m_strFileDialogTitle);
            if (!strSelPath.isEmpty() && QFileInfo(strSelPath).suffix().isEmpty())
            {
                if (m_strFileDialogDefaultSaveExtension.isEmpty())
                    strSelPath = QString("%1").arg(strSelPath);
                else
                    strSelPath = QString("%1.%2").arg(strSelPath).arg(m_strFileDialogDefaultSaveExtension);
            }
            break;
        }
        case Mode_Folder:
            strSelPath = QIFileDialog::getExistingDirectory(strInitDir, parentWidget(), m_strFileDialogTitle); break;
    }

    /* Do nothing if nothing chosen: */
    if (strSelPath.isNull())
        return;

    /* Wipe out excessive slashes: */
    strSelPath.remove(QRegExp("[\\\\/]$"));

    /* Apply chosen path: */
    changePath(strSelPath);
}

QIcon UIFilePathSelector::defaultIcon() const
{
    if (m_enmMode == Mode_Folder)
        return vboxGlobal().icon(QFileIconProvider::Folder);
    else
        return vboxGlobal().icon(QFileIconProvider::File);
}

QString UIFilePathSelector::fullPath(bool fAbsolute /* = true */) const
{
    if (m_strPath.isNull())
        return m_strPath;

    QString strResult;
    switch (m_enmMode)
    {
        case Mode_Folder:
            strResult = fAbsolute ? QDir(m_strPath).absolutePath() :
                                    QDir(m_strPath).path();
            break;
        case Mode_File_Open:
        case Mode_File_Save:
            strResult = fAbsolute ? QFileInfo(m_strPath).absoluteFilePath() :
                                    QFileInfo(m_strPath).filePath();
            break;
        default:
            AssertFailedBreak();
    }
    return QDir::toNativeSeparators(strResult);
}

QString UIFilePathSelector::shrinkText(int iWidth) const
{
    QString strFullText(fullPath(false));
    if (strFullText.isEmpty())
        return strFullText;

    int iOldSize = fontMetrics().width(strFullText);
    int iIndentSize = fontMetrics().width("x...x");

    /* Compress text: */
    int iStart = 0;
    int iFinish = 0;
    int iPosition = 0;
    int iTextWidth = 0;
    do {
        iTextWidth = fontMetrics().width(strFullText);
        if (iTextWidth + iIndentSize > iWidth)
        {
            iStart = 0;
            iFinish = strFullText.length();

            /* Selecting remove position: */
            QRegExp regExp("([\\\\/][^\\\\^/]+[\\\\/]?$)");
            int iNewFinish = regExp.indexIn(strFullText);
            if (iNewFinish != -1)
                iFinish = iNewFinish;
            iPosition = (iFinish - iStart) / 2;

            if (iPosition == iFinish)
               break;

            strFullText.remove(iPosition, 1);
        }
    } while (iTextWidth + iIndentSize > iWidth);

    strFullText.insert(iPosition, "...");
    int newSize = fontMetrics().width(strFullText);

    return newSize < iOldSize ? strFullText : fullPath(false);
}

void UIFilePathSelector::refreshText()
{
    if (m_fEditable && m_fEditableMode)
    {
        /* Cursor positioning variables: */
        int iCurPos = -1;
        int iDiffPos = -1;
        int iFromRight = -1;

        if (m_fMouseAwaited)
        {
            /* Store the cursor position: */
            iCurPos = lineEdit()->cursorPosition();
            iDiffPos = differFrom(lineEdit()->text(), m_strPath);
            iFromRight = lineEdit()->text().size() - iCurPos;
        }

        /* In editable mode there should be no any icon
         * and text have be corresponding real stored path
         * which can be absolute or relative. */
        if (lineEdit()->text() != m_strPath)
            setItemText(PathId, m_strPath);
        setItemIcon(PathId, QIcon());

        /* Set the tool-tip: */
        if (!m_fToolTipOverriden)
            QIComboBox::setToolTip(m_strNoneToolTipFocused);
        setItemData(PathId, toolTip(), Qt::ToolTipRole);

        if (m_fMouseAwaited)
        {
            m_fMouseAwaited = false;

            /* Restore the position to the right of dots: */
            if (iDiffPos != -1 && iCurPos >= iDiffPos + 3)
                lineEdit()->setCursorPosition(lineEdit()->text().size() -
                                              iFromRight);
            /* Restore the position to the center of text: */
            else if (iDiffPos != -1 && iCurPos > iDiffPos)
                lineEdit()->setCursorPosition(lineEdit()->text().size() / 2);
            /* Restore the position to the left of dots: */
            else
                lineEdit()->setCursorPosition(iCurPos);
        }
    }
    else if (m_strPath.isNull())
    {
        /* If we are not in editable mode and no path is
         * stored here - show the translated 'none' string. */
        if (itemText(PathId) != m_strNoneText)
        {
            setItemText(PathId, m_strNoneText);
            setItemIcon(PathId, QIcon());

            /* Set the tool-tip: */
            if (!m_fToolTipOverriden)
                QIComboBox::setToolTip(m_strNoneToolTip);
            setItemData(PathId, toolTip(), Qt::ToolTipRole);
        }
    }
    else
    {
        /* Compress text in combobox: */
        QStyleOptionComboBox options;
        options.initFrom(this);
        QRect rect = QApplication::style()->subControlRect(
            QStyle::CC_ComboBox, &options, QStyle::SC_ComboBoxEditField);
        setItemText(PathId, shrinkText(rect.width() - iconSize().width()));

        /* Attach corresponding icon: */
        setItemIcon(PathId, QFileInfo(m_strPath).exists() ?
                            vboxGlobal().icon(QFileInfo(m_strPath)) :
                            defaultIcon());

        /* Set the tool-tip: */
        if (!m_fToolTipOverriden)
            QIComboBox::setToolTip(fullPath());
        setItemData(PathId, toolTip(), Qt::ToolTipRole);
    }
}


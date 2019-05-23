/* $Id: QIFileDialog.cpp $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIFileDialog class implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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

/* VBox includes */
# include "VBoxGlobal.h"
# include "UIModalWindowManager.h"
# include "UIMessageCenter.h"
# include "QIFileDialog.h"

# ifdef VBOX_WS_WIN
/* Qt includes */
#  include <QEvent>
#  include <QEventLoop>
#  include <QThread>

/* WinAPI includes */
#  include <iprt/win/shlobj.h>
# endif /* !VBOX_WS_WIN */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


#ifdef VBOX_WS_WIN

static QString extractFilter (const QString &aRawFilter)
{
    static const char qt_file_dialog_filter_reg_exp[] =
        "([a-zA-Z0-9 ]*)\\(([a-zA-Z0-9_.*? +;#\\[\\]]*)\\)$";

    QString result = aRawFilter;
    QRegExp r (QString::fromLatin1 (qt_file_dialog_filter_reg_exp));
    int index = r.indexIn (result);
    if (index >= 0)
        result = r.cap (2);
    return result.replace (QChar (' '), QChar (';'));
}

/**
 * Converts QFileDialog filter list to Win32 API filter list.
 */
static QString winFilter (const QString &aFilter)
{
    QStringList filterLst;

    if (!aFilter.isEmpty())
    {
        int i = aFilter.indexOf (";;", 0);
        QString sep (";;");
        if (i == -1)
        {
            if (aFilter.indexOf ("\n", 0) != -1)
            {
                sep = "\n";
                i = aFilter.indexOf (sep, 0);
            }
        }

        filterLst = aFilter.split (sep);
    }

    QStringList::Iterator it = filterLst.begin();
    QString winfilters;
    for (; it != filterLst.end(); ++ it)
    {
        winfilters += *it;
        winfilters += QChar::Null;
        winfilters += extractFilter (*it);
        winfilters += QChar::Null;
    }
    winfilters += QChar::Null;
    return winfilters;
}

/*
 * Callback function to control the native Win32 API file dialog
 */
UINT_PTR CALLBACK OFNHookProc (HWND aHdlg, UINT aUiMsg, WPARAM aWParam, LPARAM aLParam)
{
    RT_NOREF(aWParam);
    if (aUiMsg == WM_NOTIFY)
    {
        OFNOTIFY *notif = (OFNOTIFY*) aLParam;
        if (notif->hdr.code == CDN_TYPECHANGE)
        {
            /* locate native dialog controls */
            HWND parent = GetParent (aHdlg);
            HWND button = GetDlgItem (parent, IDOK);
            HWND textfield = ::GetDlgItem (parent, cmb13);
            if (textfield == NULL)
                textfield = ::GetDlgItem (parent, edt1);
            if (textfield == NULL)
                return FALSE;
            HWND selector = ::GetDlgItem (parent, cmb1);

            /* simulate filter change by pressing apply-key */
            int    size = 256;
            TCHAR *buffer = (TCHAR*)malloc (size);
            SendMessage (textfield, WM_GETTEXT, size, (LPARAM)buffer);
            SendMessage (textfield, WM_SETTEXT, 0, (LPARAM)"\0");
            SendMessage (button, BM_CLICK, 0, 0);
            SendMessage (textfield, WM_SETTEXT, 0, (LPARAM)buffer);
            free (buffer);

            /* make request for focus moving to filter selector combo-box */
            HWND curFocus = GetFocus();
            PostMessage (curFocus, WM_KILLFOCUS, (WPARAM)selector, 0);
            PostMessage (selector, WM_SETFOCUS, (WPARAM)curFocus, 0);
            WPARAM wParam = MAKEWPARAM (WA_ACTIVE, 0);
            PostMessage (selector, WM_ACTIVATE, wParam, (LPARAM)curFocus);
        }
    }
    return FALSE;
}

/*
 * Callback function to control the native Win32 API folders dialog
 */
static int __stdcall winGetExistDirCallbackProc (HWND hwnd, UINT uMsg,
                                                 LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED && lpData != 0)
    {
        QString *initDir = (QString *)(lpData);
        if (!initDir->isEmpty())
        {
            SendMessage (hwnd, BFFM_SETSELECTION, TRUE, uintptr_t(
                initDir->isNull() ? 0 : initDir->utf16()));
        }
    }
    else if (uMsg == BFFM_SELCHANGED)
    {
        TCHAR path [MAX_PATH];
        SHGetPathFromIDList (LPITEMIDLIST (lParam), path);
        QString tmpStr = QString::fromUtf16 ((ushort*)path);
        if (!tmpStr.isEmpty())
            SendMessage (hwnd, BFFM_ENABLEOK, 1, 1);
        else
            SendMessage (hwnd, BFFM_ENABLEOK, 0, 0);
        SendMessage (hwnd, BFFM_SETSTATUSTEXT, 1, uintptr_t(path));
    }
    return 0;
}

/**
 *  QEvent class to carry Win32 API native dialog's result information
 */
class OpenNativeDialogEvent : public QEvent
{
public:

    OpenNativeDialogEvent (const QString &aResult, QEvent::Type aType)
        : QEvent (aType), mResult (aResult) {}

    const QString& result() { return mResult; }

private:

    QString mResult;
};

/**
 *  QObject class reimplementation which is the target for OpenNativeDialogEvent
 *  event. It receives OpenNativeDialogEvent event from another thread,
 *  stores result information and exits the given local event loop.
 */
class LoopObject : public QObject
{
    Q_OBJECT;

public:

    LoopObject (QEvent::Type aType, QEventLoop &aLoop)
        : mType (aType), mLoop (aLoop), mResult (QString::null) {}
    const QString& result() { return mResult; }

private:

    bool event (QEvent *aEvent)
    {
        if (aEvent->type() == mType)
        {
            OpenNativeDialogEvent *ev = (OpenNativeDialogEvent*) aEvent;
            mResult = ev->result();
            mLoop.quit();
            return true;
        }
        return QObject::event (aEvent);
    }

    QEvent::Type mType;
    QEventLoop &mLoop;
    QString mResult;
};

#endif /* VBOX_WS_WIN */

QIFileDialog::QIFileDialog (QWidget *aParent, Qt::WindowFlags aFlags)
    : QFileDialog (aParent, aFlags)
{
}

/**
 *  Reimplementation of QFileDialog::getExistingDirectory() that removes some
 *  oddities and limitations.
 *
 *  On Win32, this function makes sure a native dialog is launched in
 *  another thread to avoid dialog visualization errors occurring due to
 *  multi-threaded COM apartment initialization on the main UI thread while
 *  the appropriate native dialog function expects a single-threaded one.
 *
 *  On all other platforms, this function is equivalent to
 *  QFileDialog::getExistingDirectory().
 */
QString QIFileDialog::getExistingDirectory (const QString &aDir,
                                            QWidget *aParent,
                                            const QString &aCaption,
                                            bool aDirOnly,
                                            bool aResolveSymlinks)
{
#ifdef VBOX_WS_MAC

    /* After 4.5 exec ignores the Qt::Sheet flag.
     * See "New Ways of Using Dialogs" in http://doc.trolltech.com/qq/QtQuarterly30.pdf why.
     * We want the old behavior for file-save dialog. Unfortunately there is a bug in Qt 4.5.x
     * which result in showing the native & the Qt dialog at the same time. */
    QWidget *pParent = windowManager().realParentWindow(aParent);
    QFileDialog dlg(pParent);
    windowManager().registerNewParent(&dlg, pParent);
    dlg.setWindowTitle(aCaption);
    dlg.setDirectory(aDir);
    dlg.setResolveSymlinks(aResolveSymlinks);
    dlg.setFileMode(aDirOnly ? QFileDialog::DirectoryOnly : QFileDialog::Directory);

    QEventLoop eventLoop;
    QObject::connect(&dlg, &QFileDialog::finished,
                     &eventLoop, &QEventLoop::quit);
    dlg.open();
    eventLoop.exec();

    return dlg.result() == QDialog::Accepted ? dlg.selectedFiles().value(0, QString()) : QString();

#else

    QFileDialog::Options o;
# if defined (VBOX_WS_X11)
    /** @todo see http://bugs.kde.org/show_bug.cgi?id=210904, make it conditional
     *        when this bug is fixed (xtracker 5167).
     *        Apparently not necessary anymore (xtracker 5748)! */
//    if (vboxGlobal().isKWinManaged())
//      o |= QFileDialog::DontUseNativeDialog;
# endif
    if (aDirOnly)
        o |= QFileDialog::ShowDirsOnly;
    if (!aResolveSymlinks)
        o |= QFileDialog::DontResolveSymlinks;
    return QFileDialog::getExistingDirectory (aParent, aCaption, aDir, o);

#endif
}

/**
 *  Reimplementation of QFileDialog::getSaveFileName() that removes some
 *  oddities and limitations.
 *
 *  On Win32, this function makes sure a file filter is applied automatically
 *  right after it is selected from the drop-down list, to conform to common
 *  experience in other applications. Note that currently, @a selectedFilter
 *  is always set to null on return.
 *
 *  On all other platforms, this function is equivalent to
 *  QFileDialog::getSaveFileName().
 */
/* static */
QString QIFileDialog::getSaveFileName (const QString &aStartWith,
                                       const QString &aFilters,
                                       QWidget       *aParent,
                                       const QString &aCaption,
                                       QString       *aSelectedFilter /* = 0 */,
                                       bool           aResolveSymlinks /* = true */,
                                       bool           fConfirmOverwrite /* = false */)
{
#ifdef VBOX_WS_MAC

    /* After 4.5 exec ignores the Qt::Sheet flag.
     * See "New Ways of Using Dialogs" in http://doc.trolltech.com/qq/QtQuarterly30.pdf why.
     * We want the old behavior for file-save dialog. Unfortunately there is a bug in Qt 4.5.x
     * which result in showing the native & the Qt dialog at the same time. */
    QWidget *pParent = windowManager().realParentWindow(aParent);
    QFileDialog dlg(pParent);
    windowManager().registerNewParent(&dlg, pParent);
    dlg.setWindowTitle(aCaption);

    /* Some predictive algorithm which seems missed in native code. */
    QDir dir(aStartWith);
    while (!dir.isRoot() && !dir.exists())
        dir = QDir(QFileInfo(dir.absolutePath()).absolutePath());
    const QString strDirectory = dir.absolutePath();
    if (!strDirectory.isNull())
        dlg.setDirectory(strDirectory);
    if (strDirectory != aStartWith)
        dlg.selectFile(QFileInfo(aStartWith).absoluteFilePath());

    dlg.setNameFilter(aFilters);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    if (aSelectedFilter)
        dlg.selectNameFilter(*aSelectedFilter);
    dlg.setResolveSymlinks(aResolveSymlinks);
    dlg.setConfirmOverwrite(fConfirmOverwrite);

    QEventLoop eventLoop;
    QObject::connect(&dlg, &QFileDialog::finished,
                     &eventLoop, &QEventLoop::quit);
    dlg.open();
    eventLoop.exec();

    return dlg.result() == QDialog::Accepted ? dlg.selectedFiles().value(0, QString()) : QString();

#else

    QFileDialog::Options o;
# if defined (VBOX_WS_X11)
    /** @todo see http://bugs.kde.org/show_bug.cgi?id=210904, make it conditional
     *        when this bug is fixed (xtracker 5167)
     *        Apparently not necessary anymore (xtracker 5748)! */
//    if (vboxGlobal().isKWinManaged())
//      o |= QFileDialog::DontUseNativeDialog;
# endif
    if (!aResolveSymlinks)
        o |= QFileDialog::DontResolveSymlinks;
    if (!fConfirmOverwrite)
        o |= QFileDialog::DontConfirmOverwrite;
    return QFileDialog::getSaveFileName (aParent, aCaption, aStartWith,
                                         aFilters, aSelectedFilter, o);
#endif
}

/**
 *  Reimplementation of QFileDialog::getOpenFileName() that removes some
 *  oddities and limitations.
 *
 *  On Win32, this function makes sure a file filter is applied automatically
 *  right after it is selected from the drop-down list, to conform to common
 *  experience in other applications. Note that currently, @a selectedFilter
 *  is always set to null on return.
 *
 *  On all other platforms, this function is equivalent to
 *  QFileDialog::getOpenFileName().
 */
/* static */
QString QIFileDialog::getOpenFileName (const QString &aStartWith,
                                       const QString &aFilters,
                                       QWidget       *aParent,
                                       const QString &aCaption,
                                       QString       *aSelectedFilter /* = 0 */,
                                       bool           aResolveSymlinks /* = true */)
{
    return getOpenFileNames (aStartWith,
                             aFilters,
                             aParent,
                             aCaption,
                             aSelectedFilter,
                             aResolveSymlinks,
                             true /* aSingleFile */).value (0, "");
}

/**
 *  Reimplementation of QFileDialog::getOpenFileNames() that removes some
 *  oddities and limitations.
 *
 *  On Win32, this function makes sure a file filter is applied automatically
 *  right after it is selected from the drop-down list, to conform to common
 *  experience in other applications. Note that currently, @a selectedFilter
 *  is always set to null on return.
 *  @todo: implement the multiple file selection on win
 *  @todo: is this extra handling on win still necessary with Qt4?
 *
 *  On all other platforms, this function is equivalent to
 *  QFileDialog::getOpenFileNames().
 */
/* static */
QStringList QIFileDialog::getOpenFileNames (const QString &aStartWith,
                                            const QString &aFilters,
                                            QWidget       *aParent,
                                            const QString &aCaption,
                                            QString       *aSelectedFilter /* = 0 */,
                                            bool           aResolveSymlinks /* = true */,
                                            bool           aSingleFile /* = false */)
{
#ifdef VBOX_WS_MAC

    /* After 4.5 exec ignores the Qt::Sheet flag.
     * See "New Ways of Using Dialogs" in http://doc.trolltech.com/qq/QtQuarterly30.pdf why.
     * We want the old behavior for file-save dialog. Unfortunately there is a bug in Qt 4.5.x
     * which result in showing the native & the Qt dialog at the same time. */
    QWidget *pParent = windowManager().realParentWindow(aParent);
    QFileDialog dlg(pParent);
    windowManager().registerNewParent(&dlg, pParent);
    dlg.setWindowTitle(aCaption);

    /* Some predictive algorithm which seems missed in native code. */
    QDir dir(aStartWith);
    while (!dir.isRoot() && !dir.exists())
        dir = QDir(QFileInfo(dir.absolutePath()).absolutePath());
    const QString strDirectory = dir.absolutePath();
    if (!strDirectory.isNull())
        dlg.setDirectory(strDirectory);
    if (strDirectory != aStartWith)
        dlg.selectFile(QFileInfo(aStartWith).absoluteFilePath());

    dlg.setNameFilter(aFilters);
    if (aSingleFile)
        dlg.setFileMode(QFileDialog::ExistingFile);
    else
        dlg.setFileMode(QFileDialog::ExistingFiles);
    if (aSelectedFilter)
        dlg.selectNameFilter(*aSelectedFilter);
    dlg.setResolveSymlinks(aResolveSymlinks);

    QEventLoop eventLoop;
    QObject::connect(&dlg, &QFileDialog::finished,
                     &eventLoop, &QEventLoop::quit);
    dlg.open();
    eventLoop.exec();

    return dlg.result() == QDialog::Accepted ? dlg.selectedFiles() : QStringList() << QString();

#else

    QFileDialog::Options o;
    if (!aResolveSymlinks)
        o |= QFileDialog::DontResolveSymlinks;
# if defined (VBOX_WS_X11)
    /** @todo see http://bugs.kde.org/show_bug.cgi?id=210904, make it conditional
     *        when this bug is fixed (xtracker 5167)
     *        Apparently not necessary anymore (xtracker 5748)! */
//    if (vboxGlobal().isKWinManaged())
//      o |= QFileDialog::DontUseNativeDialog;
# endif

    if (aSingleFile)
        return QStringList() << QFileDialog::getOpenFileName (aParent, aCaption, aStartWith,
                                                              aFilters, aSelectedFilter, o);
    else
        return QFileDialog::getOpenFileNames (aParent, aCaption, aStartWith,
                                              aFilters, aSelectedFilter, o);
#endif
}

/**
 *  Search for the first directory that exists starting from the passed one
 *  and going up through its parents.  In case if none of the directories
 *  exist (except the root one), the function returns QString::null.
 */
/* static */
QString QIFileDialog::getFirstExistingDir (const QString &aStartDir)
{
    QString result = QString::null;
    QDir dir (aStartDir);
    while (!dir.exists() && !dir.isRoot())
    {
        QFileInfo dirInfo (dir.absolutePath());
        if (dir == QDir(dirInfo.absolutePath()))
            break;
        dir = dirInfo.absolutePath();
    }
    if (dir.exists() && !dir.isRoot())
        result = dir.absolutePath();
    return result;
}

#if defined VBOX_WS_WIN
#include "QIFileDialog.moc"
#endif


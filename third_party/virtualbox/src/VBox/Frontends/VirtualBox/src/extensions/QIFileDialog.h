/* $Id: QIFileDialog.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIFileDialog class declarations.
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

#ifndef __QIFileDialog_h__
#define __QIFileDialog_h__

/* Qt includes */
#include <QFileDialog>

class QIFileDialog : public QFileDialog
{
    Q_OBJECT;

public:

    QIFileDialog (QWidget *aParent, Qt::WindowFlags aFlags);

    static QString getExistingDirectory (const QString &aDir, QWidget *aParent,
                                         const QString &aCaption = QString::null,
                                         bool aDirOnly = true,
                                         bool resolveSymlinks = true);

    static QString getSaveFileName (const QString &aStartWith, const QString &aFilters, QWidget *aParent,
                                    const QString &aCaption, QString *aSelectedFilter = 0,
                                    bool aResolveSymLinks = true, bool fConfirmOverwrite = false);

    static QString getOpenFileName (const QString &aStartWith, const QString &aFilters, QWidget *aParent,
                                    const QString &aCaption, QString *aSelectedFilter = 0,
                                    bool aResolveSymLinks = true);

    static QStringList getOpenFileNames (const QString &aStartWith, const QString &aFilters, QWidget *aParent,
                                         const QString &aCaption, QString *aSelectedFilter = 0,
                                         bool aResolveSymLinks = true,
                                         bool aSingleFile = false);

    static QString getFirstExistingDir (const QString &aStartDir);
};

#endif /* __QIFileDialog_h__ */


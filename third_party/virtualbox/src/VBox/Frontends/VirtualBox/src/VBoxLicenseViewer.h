/* $Id: VBoxLicenseViewer.h $ */
/** @file
 * VBox Qt GUI - VBoxLicenseViewer class declaration.
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

#ifndef __VBoxLicenseViewer__
#define __VBoxLicenseViewer__

#include "QIWithRetranslateUI.h"

/* Qt includes */
#include <QDialog>

class QTextBrowser;
class QPushButton;

/**
 *  This class is used to show a user license under linux.
 */
class VBoxLicenseViewer : public QIWithRetranslateUI2<QDialog>
{
    Q_OBJECT;

public:

    VBoxLicenseViewer(QWidget *pParent = 0);

    int showLicenseFromFile(const QString &strLicenseFileName);
    int showLicenseFromString(const QString &strLicenseText);

protected:

    void retranslateUi();

private slots:

    int exec();

    void onScrollBarMoving (int aValue);

    void unlockButtons();

private:

    void showEvent (QShowEvent *aEvent);

    bool eventFilter (QObject *aObject, QEvent *aEvent);

    /* Private member vars */
    QTextBrowser *mLicenseText;
    QPushButton  *mAgreeButton;
    QPushButton  *mDisagreeButton;
};

#endif /* __VBoxLicenseViewer__ */


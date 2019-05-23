/* $Id: VBoxOSTypeSelectorButton.h $ */
/** @file
 * VBox Qt GUI - VBoxOSTypeSelectorButton class declaration.
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

#ifndef __VBoxOSTypeSelectorButton_h__
#define __VBoxOSTypeSelectorButton_h__

/* VBox includes */
#include "QIWithRetranslateUI.h"

/* Qt includes */
#include <QPushButton>

class QSignalMapper;

class VBoxOSTypeSelectorButton: public QIWithRetranslateUI<QPushButton>
{
    Q_OBJECT;

public:
    VBoxOSTypeSelectorButton (QWidget *aParent);
    QString osTypeId() const { return mOSTypeId; }

    bool isMenuShown() const;

    void retranslateUi();

public slots:
    void setOSTypeId (const QString& aOSTypeId);

private:
    void populateMenu();

    /* Private member vars */
    QString mOSTypeId;
    QMenu *mMainMenu;
    QSignalMapper *mSignalMapper;
};

#endif /* __VBoxOSTypeSelectorButton_h__ */


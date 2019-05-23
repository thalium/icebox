/* $Id: UIMachineSettingsUSBFilterDetails.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsUSBFilterDetails class declaration.
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

#ifndef __UIMachineSettingsUSBFilterDetails_h__
#define __UIMachineSettingsUSBFilterDetails_h__

#include "UIMachineSettingsUSBFilterDetails.gen.h"
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"
#include "UIMachineSettingsUSB.h"

class UIMachineSettingsUSBFilterDetails : public QIWithRetranslateUI2<QIDialog>,
                                          public Ui::UIMachineSettingsUSBFilterDetails
{
    Q_OBJECT;

public:

    UIMachineSettingsUSBFilterDetails(QWidget *pParent = 0);

private:

    void retranslateUi();
};

#endif /* __UIMachineSettingsUSBFilterDetails_h__ */


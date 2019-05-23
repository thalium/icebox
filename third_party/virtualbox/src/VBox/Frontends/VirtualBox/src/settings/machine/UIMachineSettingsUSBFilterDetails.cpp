/* $Id: UIMachineSettingsUSBFilterDetails.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsUSBFilterDetails class implementation.
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

/* GUI includes: */
# include "UIMachineSettingsUSBFilterDetails.h"
# include "UIConverter.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIMachineSettingsUSBFilterDetails::UIMachineSettingsUSBFilterDetails(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI2<QIDialog>(pParent, Qt::Sheet)
{
    /* Apply UI decorations */
    Ui::UIMachineSettingsUSBFilterDetails::setupUi (this);

    mCbRemote->insertItem (UIMachineSettingsUSB::ModeAny, ""); /* Any */
    mCbRemote->insertItem (UIMachineSettingsUSB::ModeOn,  ""); /* Yes */
    mCbRemote->insertItem (UIMachineSettingsUSB::ModeOff, ""); /* No */

    mLeName->setValidator (new QRegExpValidator (QRegExp (".+"), this));
    mLeVendorID->setValidator (new QRegExpValidator (QRegExp ("[0-9a-fA-F]{0,4}"), this));
    mLeProductID->setValidator (new QRegExpValidator (QRegExp ("[0-9a-fA-F]{0,4}"), this));
    mLeRevision->setValidator (new QRegExpValidator (QRegExp ("[0-9a-fA-F]{0,4}"), this));
    mLePort->setValidator (new QRegExpValidator (QRegExp ("[0-9]*"), this));

    /* Applying language settings */
    retranslateUi();

    resize (minimumSize());
    setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void UIMachineSettingsUSBFilterDetails::retranslateUi()
{
    /* Translate uic generated strings */
    Ui::UIMachineSettingsUSBFilterDetails::retranslateUi (this);

    mCbRemote->setItemText (UIMachineSettingsUSB::ModeAny, tr ("Any", "remote"));
    mCbRemote->setItemText (UIMachineSettingsUSB::ModeOn,  tr ("Yes", "remote"));
    mCbRemote->setItemText (UIMachineSettingsUSB::ModeOff, tr ("No",  "remote"));
}


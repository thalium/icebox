/* $Id: UIMachineSettingsPortForwardingDlg.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsPortForwardingDlg class declaration.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIMachineSettingsPortForwardingDlg_h__
#define __UIMachineSettingsPortForwardingDlg_h__

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"
#include "UIPortForwardingTable.h"

/* Forward declarations: */
class QIDialogButtonBox;

/* Machine settings / Network page / NAT attachment / Port forwarding dialog: */
class UIMachineSettingsPortForwardingDlg : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /* Constructor/destructor: */
    UIMachineSettingsPortForwardingDlg(QWidget *pParent, const UIPortForwardingDataList &rules);

    /* API: Rules stuff: */
    const UIPortForwardingDataList rules() const;

private slots:

    /* Handlers: Dialog stuff: */
    void accept();
    void reject();

private:

    /* Handler: Translation stuff: */
    void retranslateUi();

    /* Widgets: */
    UIPortForwardingTable *m_pTable;
    QIDialogButtonBox *m_pButtonBox;
};

#endif // __UIMachineSettingsPortForwardingDlg_h__

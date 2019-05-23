/* $Id: UIGlobalSettingsPortForwardingDlg.h $ */
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

#ifndef __UIGlobalSettingsPortForwardingDlg_h__
#define __UIGlobalSettingsPortForwardingDlg_h__

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"
#include "UIPortForwardingTable.h"

/* Forward declarations: */
class QTabWidget;
class QIDialogButtonBox;

/* Global settings / Network page / NAT network tab / Port forwarding dialog: */
class UIGlobalSettingsPortForwardingDlg : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /* Constructor/destructor: */
    UIGlobalSettingsPortForwardingDlg(QWidget *pParent,
                                      const UIPortForwardingDataList &ipv4rules,
                                      const UIPortForwardingDataList &ipv6rules);

    /* API: Rules stuff: */
    const UIPortForwardingDataList ipv4rules() const;
    const UIPortForwardingDataList ipv6rules() const;

private slots:

    /* Handlers: Dialog stuff: */
    void accept();
    void reject();

private:

    /* Handler: Translation stuff: */
    void retranslateUi();

    /* Widgets: */
    QTabWidget *m_pTabWidget;
    UIPortForwardingTable *m_pIPv4Table;
    UIPortForwardingTable *m_pIPv6Table;
    QIDialogButtonBox *m_pButtonBox;
};

#endif // __UIGlobalSettingsPortForwardingDlg_h__

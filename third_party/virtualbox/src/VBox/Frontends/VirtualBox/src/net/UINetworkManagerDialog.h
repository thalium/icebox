/* $Id: UINetworkManagerDialog.h $ */
/** @file
 * VBox Qt GUI - UINetworkManagerDialog stuff declaration.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UINetworkManagerDialog_h__
#define __UINetworkManagerDialog_h__

/* Global includes: */
#include <QMainWindow>
#include <QMap>
#include <QUuid>

/* Local includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UINetworkRequest;
class QVBoxLayout;
class QLabel;
class QIDialogButtonBox;
class UINetworkRequestWidget;

/* QMainWindow reimplementation to reflect network-requests: */
class UINetworkManagerDialog : public QIWithRetranslateUI<QMainWindow>
{
    Q_OBJECT;

signals:

    /* Ask listener (network-manager) to cancel all network-requests: */
    void sigCancelNetworkRequests();

public slots:

    /* Show the dialog, make sure its visible: */
    void showNormal();

protected:

    /* Allow creation of UINetworkManagerDialog to UINetworkManager: */
    friend class UINetworkManager;
    /* Constructor: */
    UINetworkManagerDialog();

    /* Allow adding/removing network-request widgets to UINetworkRequest: */
    friend class UINetworkRequest;
    /* Add network-request widget: */
    void addNetworkRequestWidget(UINetworkRequest *pNetworkRequest);
    /* Remove network-request widget: */
    void removeNetworkRequestWidget(const QUuid &uuid);

private slots:

    /* Handler for 'Cancel All' button-press: */
    void sltHandleCancelAllButtonPress();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Overloaded event-handlers: */
    void showEvent(QShowEvent *pShowEvent);
    void keyPressEvent(QKeyEvent *pKeyPressEvent);

    /* Widgets: */
    QLabel *m_pLabel;
    QVBoxLayout *m_pWidgetsLayout;
    QIDialogButtonBox *m_pButtonBox;
    QMap<QUuid, UINetworkRequestWidget*> m_widgets;
};

#endif // __UINetworkManagerDialog_h__


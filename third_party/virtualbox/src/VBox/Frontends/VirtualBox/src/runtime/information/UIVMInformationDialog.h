/* $Id: UIVMInformationDialog.h $ */
/** @file
 * VBox Qt GUI - UIVMInformationDialog class declaration.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIVMInformationDialog_h___
#define ___UIVMInformationDialog_h___

/* GUI includes: */
#include "QIMainWindow.h"
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"

/* Forward declarations: */
class QITabWidget;
class QIDialogButtonBox;
class UIMachineWindow;


/** QIMainWindow subclass providing user
  * with the dialog unifying VM details and statistics. */
class UIVMInformationDialog : public QIWithRetranslateUI<QIMainWindow>
{
    Q_OBJECT;

public:

    /** Shows (and creates if necessary)
      * information-dialog for passed @a pMachineWindow. */
    static void invoke(UIMachineWindow *pMachineWindow);

protected:

    /** Constructs information dialog for passed @a pMachineWindow. */
    UIVMInformationDialog(UIMachineWindow *pMachineWindow);
    /** Destructs information dialog. */
    ~UIVMInformationDialog();

    /** Returns whether the dialog should be maximized when geometry being restored. */
    virtual bool shouldBeMaximized() const /* override */;

    /** Handles translation event. */
    void retranslateUi();

    /** Handles any Qt @a pEvent. */
    bool event(QEvent *pEvent);

private slots:

    /** Destroys dialog immediately. */
    void suicide() { delete this; }
    /** Handles tab-widget page change. */
    void sltHandlePageChanged(int iIndex);

private:

    /** Prepares all. */
    void prepare();
    /** Prepares this. */
    void prepareThis();
    /** Prepares central-widget. */
    void prepareCentralWidget();
    /** Prepares tab-widget. */
    void prepareTabWidget();
    /** Prepares tab with @a iTabIndex. */
    void prepareTab(int iTabIndex);
    /** Prepares button-box. */
    void prepareButtonBox();
    /** Loads settings. */
    void loadSettings();

    /** Saves settings. */
    void saveSettings();
    /** Cleanups all. */
    void cleanup();

    /** @name General variables.
     * @{ */
    /** Holds the dialog instance. */
    static UIVMInformationDialog *s_pInstance;
    /** @} */

    /** @name Widget variables.
     * @{ */
    /** Holds the dialog tab-widget instance. */
    QITabWidget                  *m_pTabWidget;
    /** Holds the map of dialog tab instances. */
    QMap<int, QWidget*>           m_tabs;
    /** Holds the dialog button-box instance. */
    QIDialogButtonBox            *m_pButtonBox;
    /** Holds the machine-window reference. */
    UIMachineWindow              *m_pMachineWindow;
    /** @} */
};

#endif /* !___UIVMInformationDialog_h___ */


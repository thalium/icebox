/* $Id: UISettingsDialogSpecific.h $ */
/** @file
 * VBox Qt GUI - UISettingsDialogSpecific class declaration.
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

#ifndef __UISettingsDialogSpecific_h__
#define __UISettingsDialogSpecific_h__

/* GUI includes: */
#include "UISettingsDialog.h"

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"
#include "CConsole.h"
#include "CMachine.h"

/* Dialog which encapsulate all the specific functionalities of the Global Settings */
class UISettingsDialogGlobal : public UISettingsDialog
{
    Q_OBJECT;

public:

    UISettingsDialogGlobal(QWidget *pParent,
                           const QString &strCategory = QString(),
                           const QString &strControl = QString());

protected:

    /** Loads the data from the corresponding source. */
    void loadOwnData();
    /** Saves the data to the corresponding source. */
    void saveOwnData();

    void retranslateUi();

    /** Returns the dialog title extension. */
    QString titleExtension() const;
    /** Returns the dialog title. */
    QString title() const;

private:

    bool isPageAvailable(int iPageId);
};

/* Dialog which encapsulate all the specific functionalities of the Virtual Machine Settings */
class UISettingsDialogMachine : public UISettingsDialog
{
    Q_OBJECT;

public:

    UISettingsDialogMachine(QWidget *pParent, const QString &strMachineId,
                            const QString &strCategory, const QString &strControl);

protected:

    /** Loads the data from the corresponding source. */
    void loadOwnData();
    /** Saves the data to the corresponding source. */
    void saveOwnData();

    void retranslateUi();

    /** Returns the dialog title extension. */
    QString titleExtension() const;
    /** Returns the dialog title. */
    QString title() const;

    void recorrelate(UISettingsPage *pSettingsPage);

private slots:

    void sltMarkLoaded();
    void sltMarkSaved();
    void sltSessionStateChanged(QString strMachineId, KSessionState sessionState);
    void sltMachineStateChanged(QString strMachineId, KMachineState machineState);
    void sltMachineDataChanged(QString strMachineId);
    void sltCategoryChanged(int cId);
    void sltAllowResetFirstRunFlag();
    void sltResetFirstRunFlag();

private:

    bool isPageAvailable(int iPageId);
    bool isSettingsChanged();

    /* Recalculates configuration access level. */
    void updateConfigurationAccessLevel();

    QString m_strMachineId;
    KSessionState m_sessionState;
    KMachineState m_machineState;

    CSession m_session;
    CMachine m_machine;
    CConsole m_console;

    bool m_fAllowResetFirstRunFlag;
    bool m_fResetFirstRunFlag;
};

#endif // __UISettingsDialogSpecific_h__


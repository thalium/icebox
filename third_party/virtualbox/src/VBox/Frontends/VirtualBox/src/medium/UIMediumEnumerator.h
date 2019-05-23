/* $Id: UIMediumEnumerator.h $ */
/** @file
 * VBox Qt GUI - UIMediumEnumerator class declaration.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIMediumEnumerator_h___
#define ___UIMediumEnumerator_h___

/* Qt includes: */
#include <QObject>
#include <QSet>

/* GUI includes: */
#include "UIMedium.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIThreadPool;
class UITask;

/* Typedefs: */
typedef QMap<QString, CMedium> CMediumMap;

/* Medium-enumerator prototype.
 * Manages access to medium information using thread-pool interface. */
class UIMediumEnumerator : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

signals:

    /* Notifiers: Medium-operations stuff: */
    void sigMediumCreated(const QString &strMediumID);
    void sigMediumDeleted(const QString &strMediumID);

    /* Notifiers: Medium-enumeration stuff: */
    void sigMediumEnumerationStarted();
    void sigMediumEnumerated(const QString &strMediumID);
    void sigMediumEnumerationFinished();

public:

    /** Constructs medium-enumerator object. */
    UIMediumEnumerator();

    /* API: Medium-access stuff: */
    QList<QString> mediumIDs() const;
    UIMedium medium(const QString &strMediumID);
    void createMedium(const UIMedium &medium);
    void deleteMedium(const QString &strMediumID);

    /* API: Medium-enumeration stuff: */
    bool isMediumEnumerationInProgress() const { return m_fMediumEnumerationInProgress; }
    void enumerateMediums();

private slots:

    /** Handles machine-data-change and snapshot-change events. */
    void sltHandleMachineUpdate(QString strMachineID);
    /** Handles machine-[un]registration events. */
    void sltHandleMachineRegistration(QString strMachineID, bool fRegistered);
    /** Handles snapshot-deleted events. */
    void sltHandleSnapshotDeleted(QString strMachineID, QString strSnapshotID);

    /* Handler: Medium-enumeration stuff: */
    void sltHandleMediumEnumerationTaskComplete(UITask *pTask);

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /* Helpers: Medium-enumeration stuff: */
    void createMediumEnumerationTask(const UIMedium &medium);
    void addNullMediumToMap(UIMediumMap &mediums);
    void addMediumsToMap(const CMediumVector &inputMediums, UIMediumMap &outputMediums, UIMediumType mediumType);
    void addHardDisksToMap(const CMediumVector &inputMediums, UIMediumMap &outputMediums);

    /* Helpers: Medium re-caching stuff: */
    void calculateCachedUsage(const QString &strMachineID, QStringList &previousUIMediumIDs, bool fTakeIntoAccountCurrentStateOnly) const;
    void calculateActualUsage(const QString &strMachineID, CMediumMap &currentCMediums, QStringList &currentCMediumIDs, bool fTakeIntoAccountCurrentStateOnly) const;
    void calculateActualUsage(const CSnapshot &snapshot, CMediumMap &currentCMediums, QStringList &currentCMediumIDs) const;
    void calculateActualUsage(const CMachine &machine, CMediumMap &currentCMediums, QStringList &currentCMediumIDs) const;
    void recacheFromCachedUsage(const QStringList &previousUIMediumIDs);
    void recacheFromActualUsage(const CMediumMap &currentCMediums, const QStringList &currentCMediumIDs);

    /* Variables: */
    bool m_fMediumEnumerationInProgress;
    QSet<UITask*> m_tasks;
    UIMediumMap m_mediums;
};

#endif /* !___UIMediumEnumerator_h___ */


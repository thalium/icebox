/* $Id: UISnapshotPane.h $ */
/** @file
 * VBox Qt GUI - UISnapshotPane class declaration.
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

#ifndef ___UISnapshotPane_h___
#define ___UISnapshotPane_h___

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "VBoxGlobal.h"

/* COM includes: */
#include "CMachine.h"

/* Forward declarations: */
class QIcon;
class QReadWriteLock;
class QTimer;
class QTreeWidgetItem;
class QITreeWidgetItem;
class UISnapshotDetailsWidget;
class UISnapshotItem;
class UISnapshotTree;
class UIToolBar;


/** Snapshot age format. */
enum SnapshotAgeFormat
{
    SnapshotAgeFormat_InSeconds,
    SnapshotAgeFormat_InMinutes,
    SnapshotAgeFormat_InHours,
    SnapshotAgeFormat_InDays,
    SnapshotAgeFormat_Max
};


/** QWidget extension providing GUI with the pane to control snapshot related functionality. */
class UISnapshotPane : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs snapshot pane passing @a pParent to the base-class. */
    UISnapshotPane(QWidget *pParent = 0);
    /** Destructs snapshot pane. */
    virtual ~UISnapshotPane() /* override */;

    /** Defines the @a comMachine object to be parsed. */
    void setMachine(const CMachine &comMachine);

    /** Returns cached snapshot-item icon depending on @a fOnline flag. */
    const QIcon *snapshotItemIcon(bool fOnline) const;

protected:

    /** @name Qt event handlers.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;

        /** Handles resize @a pEvent. */
        virtual void resizeEvent(QResizeEvent *pEvent) /* override */;

        /** Handles show @a pEvent. */
        virtual void showEvent(QShowEvent *pEvent) /* override */;
    /** @} */

private slots:

    /** @name Main event handlers.
      * @{ */
        /** Handles machine data change for machine with @a strMachineId. */
        void sltHandleMachineDataChange(QString strMachineId);
        /** Handles machine @a enmState change for machine with @a strMachineId. */
        void sltHandleMachineStateChange(QString strMachineId, KMachineState enmState);

        /** Handles session @a enmState change for machine with @a strMachineId. */
        void sltHandleSessionStateChange(QString strMachineId, KSessionState enmState);

        /** Handles snapshot take event for machine with @a strMachineId. */
        void sltHandleSnapshotTake(QString strMachineId, QString strSnapshotId);
        /** Handles snapshot delete event for machine with @a strMachineId. */
        void sltHandleSnapshotDelete(QString strMachineId, QString strSnapshotId);
        /** Handles snapshot change event for machine with @a strMachineId. */
        void sltHandleSnapshotChange(QString strMachineId, QString strSnapshotId);
        /** Handles snapshot restore event for machine with @a strMachineId. */
        void sltHandleSnapshotRestore(QString strMachineId, QString strSnapshotId);
    /** @} */

    /** @name Timer event handlers.
      * @{ */
        /** Updates snapshots age. */
        void sltUpdateSnapshotsAge();
    /** @} */

    /** @name Toolbar handlers.
      * @{ */
        /** Handles command to take a snapshot. */
        void sltTakeSnapshot() { takeSnapshot(); }
        /** Handles command to restore the snapshot. */
        void sltRestoreSnapshot() { restoreSnapshot(); }
        /** Handles command to delete the snapshot. */
        void sltDeleteSnapshot() { deleteSnapshot(); }
        /** Handles command to make snapshot details @a fVisible. */
        void sltToggleSnapshotDetailsVisibility(bool fVisible);
        /** Handles command to apply snapshot details changes. */
        void sltApplySnapshotDetailsChanges();
        /** Proposes to clone the snapshot. */
        void sltCloneSnapshot() { cloneSnapshot(); }
    /** @} */

    /** @name Tree-widget handlers.
      * @{ */
        /** Handles tree-widget current item change. */
        void sltHandleCurrentItemChange();
        /** Handles context menu request for tree-widget @a position. */
        void sltHandleContextMenuRequest(const QPoint &position);
        /** Handles tree-widget @a pItem change. */
        void sltHandleItemChange(QTreeWidgetItem *pItem);
        /** Handles tree-widget @a pItem double-click. */
        void sltHandleItemDoubleClick(QTreeWidgetItem *pItem);
    /** @} */

private:

    /** @name Prepare/cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares widgets. */
        void prepareWidgets();
        /** Prepares toolbar. */
        void prepareToolbar();
        /** Prepares tree-widget. */
        void prepareTreeWidget();
        /** Prepares details-widget. */
        void prepareDetailsWidget();
        /** Load settings: */
        void loadSettings();

        /** Refreshes everything. */
        void refreshAll();
        /** Populates snapshot items for corresponding @a comSnapshot using @a pItem as parent. */
        void populateSnapshots(const CSnapshot &comSnapshot, QITreeWidgetItem *pItem);

        /** Cleanups all. */
        void cleanup();
    /** @} */

    /** @name Toolbar helpers.
      * @{ */
        /** Updates action states. */
        void updateActionStates();

        /** Proposes to take a snapshot. */
        bool takeSnapshot(bool fAutomatically = false);
        /** Proposes to delete the snapshot. */
        bool deleteSnapshot(bool fAutomatically = false);
        /** Proposes to restore the snapshot. */
        bool restoreSnapshot(bool fAutomatically = false);
        /** Proposes to clone the snapshot. */
        void cloneSnapshot();
    /** @} */

    /** @name Tree-widget helpers.
      * @{ */
        /** Handles command to adjust snapshot tree. */
        void adjustTreeWidget();

        /** Searches for an item with corresponding @a strSnapshotID. */
        UISnapshotItem *findItem(const QString &strSnapshotID) const;

        /** Searches for smallest snapshot age starting with @a pItem as parent. */
        SnapshotAgeFormat traverseSnapshotAge(QTreeWidgetItem *pItem) const;

        /** Expand all the children starting with @a pItem. */
        void expandItemChildren(QTreeWidgetItem *pItem);
    /** @} */

    /** @name General variables.
      * @{ */
        /** Holds the COM machine object. */
        CMachine       m_comMachine;
        /** Holds the machine object ID. */
        QString        m_strMachineId;
        /** Holds the cached session state. */
        KSessionState  m_enmSessionState;

        /** Holds whether the snapshot operations are allowed. */
        bool  m_fShapshotOperationsAllowed;

        /** Holds the snapshot item editing protector. */
        QReadWriteLock *m_pLockReadWrite;

        /** Holds the cached snapshot-item pixmap for 'offline' state. */
        QIcon *m_pIconSnapshotOffline;
        /** Holds the cached snapshot-item pixmap for 'online' state. */
        QIcon *m_pIconSnapshotOnline;

        /** Holds the snapshot age update timer. */
        QTimer *m_pTimerUpdateAge;
    /** @} */

    /** @name Widget variables.
      * @{ */
        /** Holds the toolbar instance. */
        UIToolBar *m_pToolBar;
        /** Holds the Take Snapshot action instance. */
        QAction   *m_pActionTakeSnapshot;
        /** Holds the Delete Snapshot action instance. */
        QAction   *m_pActionDeleteSnapshot;
        /** Holds the Restore Snapshot action instance. */
        QAction   *m_pActionRestoreSnapshot;
        /** Holds the Show Snapshot Details action instance. */
        QAction   *m_pActionShowSnapshotDetails;
        /** Holds the Clone Snapshot action instance. */
        QAction   *m_pActionCloneSnapshot;

        /** Holds the snapshot tree instance. */
        UISnapshotTree *m_pSnapshotTree;
        /** Holds the "current snapshot" item reference. */
        UISnapshotItem *m_pCurrentSnapshotItem;
        /** Holds the "current state" item reference. */
        UISnapshotItem *m_pCurrentStateItem;

        /** Holds the details-widget instance. */
        UISnapshotDetailsWidget *m_pDetailsWidget;
    /** @} */
};

#endif /* !___UISnapshotPane_h___ */


/* $Id: UISnapshotPane.cpp $ */
/** @file
 * VBox Qt GUI - UISnapshotPane class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QAccessibleWidget>
# include <QApplication>
# include <QDateTime>
# include <QHeaderView>
# include <QIcon>
# include <QMenu>
# include <QPointer>
# include <QReadWriteLock>
# include <QScrollBar>
# include <QTimer>
# include <QWriteLocker>

/* GUI includes: */
# include "QITreeWidget.h"
# include "UIConverter.h"
# include "UIExtraDataManager.h"
# include "UIIconPool.h"
# include "UIMessageCenter.h"
# include "UIModalWindowManager.h"
# include "UISnapshotDetailsWidget.h"
# include "UISnapshotPane.h"
# include "UITakeSnapshotDialog.h"
# include "UIToolBar.h"
# include "UIVirtualBoxEventHandler.h"
# include "UIWizardCloneVM.h"

/* COM includes: */
# include "CConsole.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Snapshot tree column tags. */
enum
{
    Column_Name,
    Column_Taken,
    Column_Max,
};


/** QITreeWidgetItem subclass for snapshots items. */
class UISnapshotItem : public QITreeWidgetItem, public UIDataSnapshot
{
    Q_OBJECT;

public:

    /** Casts QTreeWidgetItem* to UISnapshotItem* if possible. */
    static UISnapshotItem *toSnapshotItem(QTreeWidgetItem *pItem);
    /** Casts const QTreeWidgetItem* to const UISnapshotItem* if possible. */
    static const UISnapshotItem *toSnapshotItem(const QTreeWidgetItem *pItem);

    /** Constructs normal snapshot item (child of tree-widget). */
    UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidget *pTreeWidget, const CSnapshot &comSnapshot);
    /** Constructs normal snapshot item (child of tree-widget-item). */
    UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidgetItem *pRootItem, const CSnapshot &comSnapshot);

    /** Constructs "current state" item (child of tree-widget). */
    UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidget *pTreeWidget, const CMachine &comMachine);
    /** Constructs "current state" item (child of tree-widget-item). */
    UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidgetItem *pRootItem, const CMachine &comMachine);

    /** Returns item machine. */
    CMachine machine() const { return m_comMachine; }
    /** Returns item snapshot. */
    CSnapshot snapshot() const { return m_comSnapshot; }
    /** Returns item snapshot ID. */
    QString snapshotID() const { return m_strSnapshotID; }

    /** Returns whether this is the "current state" item. */
    bool isCurrentStateItem() const { return m_fCurrentStateItem; }

    /** Returns whether this is the "current snapshot" item. */
    bool isCurrentSnapshotItem() const { return m_fCurrentSnapshotItem; }
    /** Defines whether this is the @a fCurrent snapshot item. */
    void setCurrentSnapshotItem(bool fCurrent);

    /** Calculates and returns the current item level. */
    int level() const;

    /** Recaches the item's contents. */
    void recache();

    /** Returns current machine state. */
    KMachineState machineState() const;
    /** Defines current machine @a enmState. */
    void setMachineState(KMachineState enmState);

    /** Updates item age. */
    SnapshotAgeFormat updateAge();

protected:

    /** Recaches item tool-tip. */
    void recacheToolTip();

private:

    /** Holds the pointer to the snapshot-widget this item belongs to. */
    QPointer<UISnapshotPane>  m_pSnapshotWidget;

    /** Holds whether this is a "current state" item. */
    bool  m_fCurrentStateItem;
    /** Holds whether this is a "current snapshot" item. */
    bool  m_fCurrentSnapshotItem;

    /** Holds the snapshot COM wrapper. */
    CSnapshot  m_comSnapshot;
    /** Holds the machine COM wrapper. */
    CMachine   m_comMachine;

    /** Holds the "current snapshot" ID. */
    QString  m_strSnapshotID;
    /** Holds whether the "current snapshot" is online one. */
    bool     m_fOnline;

    /** Holds the item timestamp. */
    QDateTime  m_timestamp;

    /** Holds whether the current state is modified. */
    bool           m_fCurrentStateModified;
    /** Holds the cached machine state. */
    KMachineState  m_enmMachineState;
};


/** QITreeWidget subclass for snapshots items. */
class UISnapshotTree : public QITreeWidget
{
    Q_OBJECT;

public:

    /** Constructs snapshot tree passing @a pParent to the base-class. */
    UISnapshotTree(QWidget *pParent);
};


/*********************************************************************************************************************************
*   Class UISnapshotItem implementation.                                                                                         *
*********************************************************************************************************************************/

/* static */
UISnapshotItem *UISnapshotItem::toSnapshotItem(QTreeWidgetItem *pItem)
{
    /* Get QITreeWidgetItem item first: */
    QITreeWidgetItem *pIItem = QITreeWidgetItem::toItem(pItem);
    if (!pIItem)
        return 0;

    /* Return casted UISnapshotItem then: */
    return qobject_cast<UISnapshotItem*>(pIItem);
}

/* static */
const UISnapshotItem *UISnapshotItem::toSnapshotItem(const QTreeWidgetItem *pItem)
{
    /* Get QITreeWidgetItem item first: */
    const QITreeWidgetItem *pIItem = QITreeWidgetItem::toItem(pItem);
    if (!pIItem)
        return 0;

    /* Return casted UISnapshotItem then: */
    return qobject_cast<const UISnapshotItem*>(pIItem);
}

UISnapshotItem::UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidget *pTreeWidget, const CSnapshot &comSnapshot)
    : QITreeWidgetItem(pTreeWidget)
    , m_pSnapshotWidget(pSnapshotWidget)
    , m_fCurrentStateItem(false)
    , m_fCurrentSnapshotItem(false)
    , m_comSnapshot(comSnapshot)
{
}

UISnapshotItem::UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidgetItem *pRootItem, const CSnapshot &comSnapshot)
    : QITreeWidgetItem(pRootItem)
    , m_pSnapshotWidget(pSnapshotWidget)
    , m_fCurrentStateItem(false)
    , m_fCurrentSnapshotItem(false)
    , m_comSnapshot(comSnapshot)
{
}

UISnapshotItem::UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidget *pTreeWidget, const CMachine &comMachine)
    : QITreeWidgetItem(pTreeWidget)
    , m_pSnapshotWidget(pSnapshotWidget)
    , m_fCurrentStateItem(true)
    , m_fCurrentSnapshotItem(false)
    , m_comMachine(comMachine)
{
    /* Set the bold font state
     * for "current state" item: */
    QFont myFont = font(Column_Name);
    myFont.setBold(true);
    setFont(Column_Name, myFont);

    /* Fetch current machine state: */
    setMachineState(m_comMachine.GetState());
}

UISnapshotItem::UISnapshotItem(UISnapshotPane *pSnapshotWidget, QITreeWidgetItem *pRootItem, const CMachine &comMachine)
    : QITreeWidgetItem(pRootItem)
    , m_pSnapshotWidget(pSnapshotWidget)
    , m_fCurrentStateItem(true)
    , m_fCurrentSnapshotItem(false)
    , m_comMachine(comMachine)
{
    /* Set the bold font state
     * for "current state" item: */
    QFont myFont = font(Column_Name);
    myFont.setBold(true);
    setFont(Column_Name, myFont);

    /* Fetch current machine state: */
    setMachineState(m_comMachine.GetState());
}

int UISnapshotItem::level() const
{
    const QTreeWidgetItem *pItem = this;
    int iResult = 0;
    while (pItem->parent())
    {
        ++iResult;
        pItem = pItem->parent();
    }
    return iResult;
}

void UISnapshotItem::setCurrentSnapshotItem(bool fCurrent)
{
    /* Remember the state: */
    m_fCurrentSnapshotItem = fCurrent;

    /* Set/clear the bold font state
     * for "current snapshot" item: */
    QFont myFont = font(Column_Name);
    myFont.setBold(fCurrent);
    setFont(Column_Name, myFont);

    /* Update tool-tip: */
    recacheToolTip();
}

void UISnapshotItem::recache()
{
    /* For "current state" item: */
    if (m_fCurrentStateItem)
    {
        /* Fetch machine information: */
        AssertReturnVoid(m_comMachine.isNotNull());
        m_fCurrentStateModified = m_comMachine.GetCurrentStateModified();
        m_strName = m_fCurrentStateModified
                  ? UISnapshotPane::tr("Current State (changed)", "Current State (Modified)")
                  : UISnapshotPane::tr("Current State", "Current State (Unmodified)");
        setText(Column_Name, m_strName);
        m_strDescription = m_fCurrentStateModified
                         ? UISnapshotPane::tr("The current state differs from the state stored in the current snapshot")
                         : QTreeWidgetItem::parent() != 0
                         ? UISnapshotPane::tr("The current state is identical to the state stored in the current snapshot")
                         : QString();
    }
    /* For snapshot item: */
    else
    {
        /* Fetch snapshot information: */
        AssertReturnVoid(m_comSnapshot.isNotNull());
        m_strSnapshotID = m_comSnapshot.GetId();
        m_strName = m_comSnapshot.GetName();
        setText(Column_Name, m_strName);
        m_fOnline = m_comSnapshot.GetOnline();
        setIcon(Column_Name, *m_pSnapshotWidget->snapshotItemIcon(m_fOnline));
        m_strDescription = m_comSnapshot.GetDescription();
        m_timestamp.setTime_t(m_comSnapshot.GetTimeStamp() / 1000);
        m_fCurrentStateModified = false;
    }

    /* Update tool-tip: */
    recacheToolTip();
}

KMachineState UISnapshotItem::machineState() const
{
    /* Make sure machine is valid: */
    if (m_comMachine.isNull())
        return KMachineState_Null;

    /* Return cached state: */
    return m_enmMachineState;
}

void UISnapshotItem::setMachineState(KMachineState enmState)
{
    /* Make sure machine is valid: */
    if (m_comMachine.isNull())
        return;

    /* Cache new state: */
    m_enmMachineState = enmState;
    /* Set corresponding icon: */
    setIcon(Column_Name, gpConverter->toIcon(m_enmMachineState));
    /* Update timestamp: */
    m_timestamp.setTime_t(m_comMachine.GetLastStateChange() / 1000);
}

SnapshotAgeFormat UISnapshotItem::updateAge()
{
    /* Prepare age: */
    QString strAge;

    /* Age: [date time|%1d ago|%1h ago|%1min ago|%1sec ago] */
    SnapshotAgeFormat enmAgeFormat;
    const QDateTime now = QDateTime::currentDateTime();
    QDateTime then = m_timestamp;
    if (then > now)
        then = now; /* can happen if the host time is wrong */
    if (then.daysTo(now) > 30)
    {
        strAge = then.toString(Qt::LocalDate);
        enmAgeFormat = SnapshotAgeFormat_Max;
    }
    else if (then.secsTo(now) > 60 * 60 * 24)
    {
        strAge = UISnapshotPane::tr("%1 (%2 ago)", "date time (how long ago)")
                 .arg(then.toString(Qt::LocalDate), VBoxGlobal::daysToString(then.secsTo(now) / 60 / 60 / 24));
        enmAgeFormat = SnapshotAgeFormat_InDays;
    }
    else if (then.secsTo(now) > 60 * 60)
    {
        strAge = UISnapshotPane::tr("%1 (%2 ago)", "date time (how long ago)")
                 .arg(then.toString(Qt::LocalDate), VBoxGlobal::hoursToString(then.secsTo(now) / 60 / 60));
        enmAgeFormat = SnapshotAgeFormat_InHours;
    }
    else if (then.secsTo(now) > 60)
    {
        strAge = UISnapshotPane::tr("%1 (%2 ago)", "date time (how long ago)")
                 .arg(then.toString(Qt::LocalDate), VBoxGlobal::minutesToString(then.secsTo(now) / 60));
        enmAgeFormat = SnapshotAgeFormat_InMinutes;
    }
    else
    {
        strAge = UISnapshotPane::tr("%1 (%2 ago)", "date time (how long ago)")
                 .arg(then.toString(Qt::LocalDate), VBoxGlobal::secondsToString(then.secsTo(now)));
        enmAgeFormat = SnapshotAgeFormat_InSeconds;
    }

    /* Update data: */
    if (!m_fCurrentStateItem)
        setText(Column_Taken, strAge);

    /* Return age: */
    return enmAgeFormat;
}

void UISnapshotItem::recacheToolTip()
{
    /* Is the saved date today? */
    const bool fDateTimeToday = m_timestamp.date() == QDate::currentDate();

    /* Compose date time: */
    QString strDateTime = fDateTimeToday
                        ? m_timestamp.time().toString(Qt::LocalDate)
                        : m_timestamp.toString(Qt::LocalDate);

    /* Prepare details: */
    QString strDetails;

    /* For "current state" item: */
    if (m_fCurrentStateItem)
    {
        strDateTime = UISnapshotPane::tr("%1 since %2", "Current State (time or date + time)")
                      .arg(gpConverter->toString(m_enmMachineState)).arg(strDateTime);
    }
    /* For snapshot item: */
    else
    {
        /* Gather details: */
        QStringList details;
        if (isCurrentSnapshotItem())
            details << UISnapshotPane::tr("current", "snapshot");
        details << (m_fOnline ? UISnapshotPane::tr("online", "snapshot")
                              : UISnapshotPane::tr("offline", "snapshot"));
        strDetails = QString(" (%1)").arg(details.join(", "));

        /* Add date/time information: */
        if (fDateTimeToday)
            strDateTime = UISnapshotPane::tr("Taken at %1", "Snapshot (time)").arg(strDateTime);
        else
            strDateTime = UISnapshotPane::tr("Taken on %1", "Snapshot (date + time)").arg(strDateTime);
    }

    /* Prepare tool-tip: */
    QString strToolTip = QString("<nobr><b>%1</b>%2</nobr><br><nobr>%3</nobr>")
                             .arg(text(Column_Name)).arg(strDetails).arg(strDateTime);

    /* Append description if any: */
    if (!m_strDescription.isEmpty())
        strToolTip += "<hr>" + m_strDescription;

    /* Assign tool-tip finally: */
    setToolTip(Column_Name, strToolTip);
}


/*********************************************************************************************************************************
*   Class UISnapshotTree implementation.                                                                                         *
*********************************************************************************************************************************/

UISnapshotTree::UISnapshotTree(QWidget *pParent)
    : QITreeWidget(pParent)
{
    /* Configure snapshot tree: */
    setColumnCount(Column_Max);
    setAllColumnsShowFocus(true);
    setAlternatingRowColors(true);
    setExpandsOnDoubleClick(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(  QAbstractItemView::SelectedClicked
                    | QAbstractItemView::EditKeyPressed);
}


/*********************************************************************************************************************************
*   Class UISnapshotPane implementation.                                                                                         *
*********************************************************************************************************************************/

UISnapshotPane::UISnapshotPane(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmSessionState(KSessionState_Null)
    , m_fShapshotOperationsAllowed(false)
    , m_pLockReadWrite(0)
    , m_pIconSnapshotOffline(0)
    , m_pIconSnapshotOnline(0)
    , m_pTimerUpdateAge(0)
    , m_pToolBar(0)
    , m_pActionTakeSnapshot(0)
    , m_pActionDeleteSnapshot(0)
    , m_pActionRestoreSnapshot(0)
    , m_pActionShowSnapshotDetails(0)
    , m_pActionCloneSnapshot(0)
    , m_pSnapshotTree(0)
    , m_pCurrentSnapshotItem(0)
    , m_pCurrentStateItem(0)
    , m_pDetailsWidget(0)
{
    /* Prepare: */
    prepare();
}

UISnapshotPane::~UISnapshotPane()
{
    /* Cleanup: */
    cleanup();
}

void UISnapshotPane::setMachine(const CMachine &comMachine)
{
    /* Cache passed machine: */
    m_comMachine = comMachine;

    /* Cache machine details: */
    if (m_comMachine.isNull())
    {
        m_strMachineId = QString();
        m_enmSessionState = KSessionState_Null;
        m_fShapshotOperationsAllowed = false;
    }
    else
    {
        m_strMachineId = comMachine.GetId();
        m_enmSessionState = comMachine.GetSessionState();
        m_fShapshotOperationsAllowed = gEDataManager->machineSnapshotOperationsEnabled(m_strMachineId);
    }

    /* Refresh everything: */
    refreshAll();
}

const QIcon *UISnapshotPane::snapshotItemIcon(bool fOnline) const
{
    return !fOnline ? m_pIconSnapshotOffline : m_pIconSnapshotOnline;
}

void UISnapshotPane::retranslateUi()
{
    /* Translate snapshot tree: */
    m_pSnapshotTree->setWhatsThis(tr("Contains the snapshot tree of the current virtual machine"));

    /* Translate actions names: */
    m_pActionTakeSnapshot->setText(tr("&Take..."));
    m_pActionDeleteSnapshot->setText(tr("&Delete"));
    m_pActionRestoreSnapshot->setText(tr("&Restore"));
    m_pActionShowSnapshotDetails->setText(tr("&Properties..."));
    m_pActionCloneSnapshot->setText(tr("&Clone..."));
    /* Translate actions tool-tips: */
    m_pActionTakeSnapshot->setToolTip(tr("Take Snapshot (%1)")
                                      .arg(m_pActionTakeSnapshot->shortcut().toString()));
    m_pActionDeleteSnapshot->setToolTip(tr("Delete Snapshot (%1)")
                                        .arg(m_pActionDeleteSnapshot->shortcut().toString()));
    m_pActionRestoreSnapshot->setToolTip(tr("Restore Snapshot (%1)")
                                         .arg(m_pActionRestoreSnapshot->shortcut().toString()));
    m_pActionShowSnapshotDetails->setToolTip(tr("Open Snapshot Properties (%1)")
                                             .arg(m_pActionShowSnapshotDetails->shortcut().toString()));
    m_pActionCloneSnapshot->setToolTip(tr("Clone Virtual Machine (%1)")
                                       .arg(m_pActionCloneSnapshot->shortcut().toString()));
    /* Translate actions status-tips: */
    m_pActionTakeSnapshot->setStatusTip(tr("Take a snapshot of the current virtual machine state"));
    m_pActionDeleteSnapshot->setStatusTip(tr("Delete selected snapshot of the virtual machine"));
    m_pActionRestoreSnapshot->setStatusTip(tr("Restore selected snapshot of the virtual machine"));
    m_pActionShowSnapshotDetails->setStatusTip(tr("Open pane with the selected snapshot properties"));
    m_pActionCloneSnapshot->setStatusTip(tr("Clone selected virtual machine"));

    /* Translate toolbar: */
#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text. */
    if (m_pToolBar)
        m_pToolBar->updateLayout();
#endif

    /* Translate snapshot tree: */
    const QStringList fields = QStringList()
                               << tr("Name", "snapshot")
                               << tr("Taken", "snapshot");
    m_pSnapshotTree->setHeaderLabels(fields);

    /* Refresh whole the tree: */
    refreshAll();
}

void UISnapshotPane::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::resizeEvent(pEvent);

    /* Adjust snapshot tree: */
    adjustTreeWidget();
}

void UISnapshotPane::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::showEvent(pEvent);

    /* Adjust snapshot tree: */
    adjustTreeWidget();
}

void UISnapshotPane::sltHandleMachineDataChange(QString strMachineId)
{
    /* Make sure it's our VM: */
    if (strMachineId != m_strMachineId)
        return;

    /* Prevent snapshot editing in the meantime: */
    QWriteLocker locker(m_pLockReadWrite);

    /* Recache "current state" item data: */
    m_pCurrentStateItem->recache();

    /* Choose current item again (to update details-widget): */
    sltHandleCurrentItemChange();
}

void UISnapshotPane::sltHandleMachineStateChange(QString strMachineId, KMachineState enmState)
{
    /* Make sure it's our VM: */
    if (strMachineId != m_strMachineId)
        return;

    /* Prevent snapshot editing in the meantime: */
    QWriteLocker locker(m_pLockReadWrite);

    /* Recache "current state" item data and machine-state: */
    m_pCurrentStateItem->recache();
    m_pCurrentStateItem->setMachineState(enmState);
}

void UISnapshotPane::sltHandleSessionStateChange(QString strMachineId, KSessionState enmState)
{
    /* Make sure it's our VM: */
    if (strMachineId != m_strMachineId)
        return;

    /* Prevent snapshot editing in the meantime: */
    QWriteLocker locker(m_pLockReadWrite);

    /* Recache current session-state: */
    m_enmSessionState = enmState;

    /* Update action states: */
    updateActionStates();
}

void UISnapshotPane::sltHandleSnapshotTake(QString strMachineId, QString strSnapshotId)
{
    /* Make sure it's our VM: */
    if (strMachineId != m_strMachineId)
        return;

    LogRel(("GUI: Updating snapshot tree after TAKING snapshot with MachineID={%s}, SnapshotID={%s}...\n",
            strMachineId.toUtf8().constData(), strSnapshotId.toUtf8().constData()));

    /* Prepare result: */
    bool fSuccess = true;
    {
        /* Prevent snapshot editing in the meantime: */
        QWriteLocker locker(m_pLockReadWrite);

        /* Search for corresponding snapshot: */
        CSnapshot comSnapshot = m_comMachine.FindSnapshot(strSnapshotId);
        fSuccess = m_comMachine.isOk() && !comSnapshot.isNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            msgCenter().cannotFindSnapshotById(m_comMachine, strSnapshotId, this);
        else
        {
            /* Where will be the newly created item located? */
            UISnapshotItem *pParentItem = 0;

            /* Acquire snapshot parent: */
            CSnapshot comParentSnapshot = comSnapshot.GetParent();
            if (comParentSnapshot.isNotNull())
            {
                /* Acquire parent snapshot id: */
                const QString strParentSnapshotId = comParentSnapshot.GetId();
                fSuccess = comParentSnapshot.isOk();

                /* Show error message if necessary: */
                if (!fSuccess)
                    msgCenter().cannotAcquireSnapshotAttributes(comSnapshot, this);
                else
                {
                    /* Search for an existing parent-item with such id: */
                    pParentItem = findItem(strParentSnapshotId);
                    fSuccess = pParentItem;
                }
            }

            /* Make sure this parent-item is a parent of "current state" item as well: */
            if (fSuccess)
                fSuccess = qobject_cast<UISnapshotItem*>(m_pCurrentStateItem->parentItem()) == pParentItem;
            /* Make sure this parent-item is a "current snapshot" item as well: */
            if (fSuccess)
                fSuccess = m_pCurrentSnapshotItem == pParentItem;

            /* Create new item: */
            if (fSuccess)
            {
                /* Delete "current state" item first of all: */
                UISnapshotItem *pCurrentStateItem = m_pCurrentStateItem;
                m_pCurrentStateItem = 0;
                delete pCurrentStateItem;
                pCurrentStateItem = 0;

                /* Create "current snapshot" item for a newly taken snapshot: */
                if (m_pCurrentSnapshotItem)
                    m_pCurrentSnapshotItem->setCurrentSnapshotItem(false);
                m_pCurrentSnapshotItem = pParentItem
                                       ? new UISnapshotItem(this, pParentItem, comSnapshot)
                                       : new UISnapshotItem(this, m_pSnapshotTree, comSnapshot);
                /* Mark it as editable: */
                m_pCurrentSnapshotItem->setFlags(m_pCurrentSnapshotItem->flags() | Qt::ItemIsEditable);
                /* Mark it as current: */
                m_pCurrentSnapshotItem->setCurrentSnapshotItem(true);
                /* And recache it's content: */
                m_pCurrentSnapshotItem->recache();

                /* Create "current state" item as a child of "current snapshot" item: */
                m_pCurrentStateItem = new UISnapshotItem(this, m_pCurrentSnapshotItem, m_comMachine);
                /* Recache it's content: */
                m_pCurrentStateItem->recache();
                /* And choose is as current one: */
                m_pSnapshotTree->setCurrentItem(m_pCurrentStateItem);
                sltHandleCurrentItemChange();

                LogRel(("GUI: Snapshot tree update successful!\n"));
            }
        }
    }

    /* Just refresh everything as fallback: */
    if (!fSuccess)
    {
        LogRel(("GUI: Snapshot tree update failed! Rebuilding from scratch...\n"));
        refreshAll();
    }
}

void UISnapshotPane::sltHandleSnapshotDelete(QString strMachineId, QString strSnapshotId)
{
    /* Make sure it's our VM: */
    if (strMachineId != m_strMachineId)
        return;

    LogRel(("GUI: Updating snapshot tree after DELETING snapshot with MachineID={%s}, SnapshotID={%s}...\n",
            strMachineId.toUtf8().constData(), strSnapshotId.toUtf8().constData()));

    /* Prepare result: */
    bool fSuccess = false;
    {
        /* Prevent snapshot editing in the meantime: */
        QWriteLocker locker(m_pLockReadWrite);

        /* Search for an existing item with such id: */
        UISnapshotItem *pItem = findItem(strSnapshotId);
        fSuccess = pItem;

        /* Make sure item has no more than one child: */
        if (fSuccess)
            fSuccess = pItem->childCount() <= 1;

        /* Detach child before deleting item: */
        QTreeWidgetItem *pChild = 0;
        if (fSuccess && pItem->childCount() == 1)
            pChild = pItem->takeChild(0);

        /* Check whether item has parent: */
        QTreeWidgetItem *pParent = 0;
        if (fSuccess)
            pParent = pItem->QTreeWidgetItem::parent();

        /* If item has child: */
        if (pChild)
        {
            /* Determine where the item located: */
            int iIndexOfChild = -1;
            if (fSuccess)
            {
                if (pParent)
                    iIndexOfChild = pParent->indexOfChild(pItem);
                else
                    iIndexOfChild = m_pSnapshotTree->indexOfTopLevelItem(pItem);
                fSuccess = iIndexOfChild != -1;
            }

            /* And move child into this place: */
            if (fSuccess)
            {
                if (pParent)
                    pParent->insertChild(iIndexOfChild, pChild);
                else
                    m_pSnapshotTree->insertTopLevelItem(iIndexOfChild, pChild);
                expandItemChildren(pChild);
            }
        }

        /* Delete item finally: */
        if (fSuccess)
        {
            if (pItem == m_pCurrentSnapshotItem)
            {
                m_pCurrentSnapshotItem = UISnapshotItem::toSnapshotItem(pParent);
                if (m_pCurrentSnapshotItem)
                    m_pCurrentSnapshotItem->setCurrentSnapshotItem(true);
            }
            delete pItem;
            pItem = 0;

            LogRel(("GUI: Snapshot tree update successful!\n"));
        }
    }

    /* Just refresh everything as fallback: */
    if (!fSuccess)
    {
        LogRel(("GUI: Snapshot tree update failed! Rebuilding from scratch...\n"));
        refreshAll();
    }
}

void UISnapshotPane::sltHandleSnapshotChange(QString strMachineId, QString strSnapshotId)
{
    /* Make sure it's our VM: */
    if (strMachineId != m_strMachineId)
        return;

    LogRel(("GUI: Updating snapshot tree after CHANGING snapshot with MachineID={%s}, SnapshotID={%s}...\n",
            strMachineId.toUtf8().constData(), strSnapshotId.toUtf8().constData()));

    /* Prepare result: */
    bool fSuccess = true;
    {
        /* Prevent snapshot editing in the meantime: */
        QWriteLocker locker(m_pLockReadWrite);

        /* Search for an existing item with such id: */
        UISnapshotItem *pItem = findItem(strSnapshotId);
        fSuccess = pItem;

        /* Update the item: */
        if (fSuccess)
        {
            /* Recache it: */
            pItem->recache();
            /* And choose it again if it's current one (to update details-widget): */
            if (UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem()) == pItem)
                sltHandleCurrentItemChange();

            LogRel(("GUI: Snapshot tree update successful!\n"));
        }
    }

    /* Just refresh everything as fallback: */
    if (!fSuccess)
    {
        LogRel(("GUI: Snapshot tree update failed! Rebuilding from scratch...\n"));
        refreshAll();
    }
}

void UISnapshotPane::sltHandleSnapshotRestore(QString strMachineId, QString strSnapshotId)
{
    /* Make sure it's our VM: */
    if (strMachineId != m_strMachineId)
        return;

    LogRel(("GUI: Updating snapshot tree after RESTORING snapshot with MachineID={%s}, SnapshotID={%s}...\n",
            strMachineId.toUtf8().constData(), strSnapshotId.toUtf8().constData()));

    /* Prepare result: */
    bool fSuccess = true;
    {
        /* Prevent snapshot editing in the meantime: */
        QWriteLocker locker(m_pLockReadWrite);

        /* Search for an existing item with such id: */
        UISnapshotItem *pItem = findItem(strSnapshotId);
        fSuccess = pItem;

        /* Choose this item as new "current snapshot": */
        if (fSuccess)
        {
            /* Delete "current state" item first of all: */
            UISnapshotItem *pCurrentStateItem = m_pCurrentStateItem;
            m_pCurrentStateItem = 0;
            delete pCurrentStateItem;
            pCurrentStateItem = 0;

            /* Move the "current snapshot" token from one to another: */
            AssertPtrReturnVoid(m_pCurrentSnapshotItem);
            m_pCurrentSnapshotItem->setCurrentSnapshotItem(false);
            m_pCurrentSnapshotItem = pItem;
            m_pCurrentSnapshotItem->setCurrentSnapshotItem(true);

            /* Create "current state" item as a child of "current snapshot" item: */
            m_pCurrentStateItem = new UISnapshotItem(this, m_pCurrentSnapshotItem, m_comMachine);
            m_pCurrentStateItem->recache();
            /* And choose is as current one: */
            m_pSnapshotTree->setCurrentItem(m_pCurrentStateItem);
            sltHandleCurrentItemChange();

            LogRel(("GUI: Snapshot tree update successful!\n"));
        }
    }

    /* Just refresh everything as fallback: */
    if (!fSuccess)
    {
        LogRel(("GUI: Snapshot tree update failed! Rebuilding from scratch...\n"));
        refreshAll();
    }
}

void UISnapshotPane::sltUpdateSnapshotsAge()
{
    /* Stop timer if active: */
    if (m_pTimerUpdateAge->isActive())
        m_pTimerUpdateAge->stop();

    /* Search for smallest snapshot age to optimize timer timeout: */
    const SnapshotAgeFormat enmAge = traverseSnapshotAge(m_pSnapshotTree->invisibleRootItem());
    switch (enmAge)
    {
        case SnapshotAgeFormat_InSeconds: m_pTimerUpdateAge->setInterval(5 * 1000); break;
        case SnapshotAgeFormat_InMinutes: m_pTimerUpdateAge->setInterval(60 * 1000); break;
        case SnapshotAgeFormat_InHours:   m_pTimerUpdateAge->setInterval(60 * 60 * 1000); break;
        case SnapshotAgeFormat_InDays:    m_pTimerUpdateAge->setInterval(24 * 60 * 60 * 1000); break;
        default:                          m_pTimerUpdateAge->setInterval(0); break;
    }

    /* Restart timer if necessary: */
    if (m_pTimerUpdateAge->interval() > 0)
        m_pTimerUpdateAge->start();
}

void UISnapshotPane::sltToggleSnapshotDetailsVisibility(bool fVisible)
{
    /* Save the setting: */
    gEDataManager->setSnapshotManagerDetailsExpanded(fVisible);
    /* Show/hide details-widget: */
    m_pDetailsWidget->setVisible(fVisible);
    /* If details-widget is visible: */
    if (m_pDetailsWidget->isVisible())
    {
        /* Acquire selected item: */
        const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());
        AssertPtrReturnVoid(pSnapshotItem);
        /* Update details-widget: */
        if (pSnapshotItem->isCurrentStateItem())
            m_pDetailsWidget->setData(m_comMachine);
        else
            m_pDetailsWidget->setData(*pSnapshotItem, pSnapshotItem->snapshot());
    }
}

void UISnapshotPane::sltApplySnapshotDetailsChanges()
{
    /* Acquire selected item: */
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());
    AssertPtrReturnVoid(pSnapshotItem);

    /* For current state item: */
    if (pSnapshotItem->isCurrentStateItem())
    {
        /* Get item data: */
        UIDataSnapshot newData = m_pDetailsWidget->data();

        /* Open a session (this call will handle all errors): */
        CSession comSession;
        if (m_enmSessionState != KSessionState_Unlocked)
            comSession = vboxGlobal().openExistingSession(m_strMachineId);
        else
            comSession = vboxGlobal().openSession(m_strMachineId);
        if (comSession.isNotNull())
        {
            /* Get corresponding machine object: */
            CMachine comMachine = comSession.GetMachine();

            /* Perform separate linked steps: */
            do
            {
                /* Take snapshot: */
                QString strSnapshotId;
                CProgress comProgress = comMachine.TakeSnapshot(newData.m_strName,
                                                                newData.m_strDescription,
                                                                true, strSnapshotId);
                if (!comMachine.isOk())
                {
                    msgCenter().cannotTakeSnapshot(comMachine, comMachine.GetName());
                    break;
                }

                /* Show snapshot taking progress: */
                msgCenter().showModalProgressDialog(comProgress, comMachine.GetName(),
                                                    ":/progress_snapshot_create_90px.png");
                if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                {
                    msgCenter().cannotTakeSnapshot(comProgress, comMachine.GetName());
                    break;
                }
            }
            while (0);

            /* Cleanup session: */
            comSession.UnlockMachine();
        }
    }
    /* For snapshot items: */
    else
    {
        /* Make sure nothing being edited in the meantime: */
        if (!m_pLockReadWrite->tryLockForWrite())
            return;

        /* Make sure that's a snapshot item indeed: */
        CSnapshot comSnapshot = pSnapshotItem->snapshot();
        AssertReturnVoid(comSnapshot.isNotNull());

        /* Get item data: */
        UIDataSnapshot oldData = *pSnapshotItem;
        UIDataSnapshot newData = m_pDetailsWidget->data();
        AssertReturnVoid(newData != oldData);

        /* Open a session (this call will handle all errors): */
        CSession comSession;
        if (m_enmSessionState != KSessionState_Unlocked)
            comSession = vboxGlobal().openExistingSession(m_strMachineId);
        else
            comSession = vboxGlobal().openSession(m_strMachineId);
        if (comSession.isNotNull())
        {
            /* Get corresponding machine object: */
            CMachine comMachine = comSession.GetMachine();

            /* Perform separate independent steps: */
            do
            {
                /* Save snapshot name: */
                if (newData.m_strName != oldData.m_strName)
                {
                    comSnapshot.SetName(newData.m_strName);
                    if (!comSnapshot.isOk())
                    {
                        msgCenter().cannotChangeSnapshot(comSnapshot, oldData.m_strName, comMachine.GetName());
                        break;
                    }
                }

                /* Save snapshot description: */
                if (newData.m_strDescription != oldData.m_strDescription)
                {
                    comSnapshot.SetDescription(newData.m_strDescription);
                    if (!comSnapshot.isOk())
                    {
                        msgCenter().cannotChangeSnapshot(comSnapshot, oldData.m_strName, comMachine.GetName());
                        break;
                    }
                }
            }
            while (0);

            /* Cleanup session: */
            comSession.UnlockMachine();
        }

        /* Allows editing again: */
        m_pLockReadWrite->unlock();
    }

    /* Adjust snapshot tree: */
    adjustTreeWidget();
}

void UISnapshotPane::sltHandleCurrentItemChange()
{
    /* Acquire "current snapshot" item: */
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());

    /* Make the current item visible: */
    if (pSnapshotItem)
    {
        m_pSnapshotTree->horizontalScrollBar()->setValue(0);
        m_pSnapshotTree->scrollToItem(pSnapshotItem);
        m_pSnapshotTree->horizontalScrollBar()->setValue(m_pSnapshotTree->indentation() * pSnapshotItem->level());
    }

    /* Update action states: */
    updateActionStates();

    /* Update details-widget if it's visible: */
    if (pSnapshotItem && m_pDetailsWidget->isVisible())
    {
        if (pSnapshotItem->isCurrentStateItem())
            m_pDetailsWidget->setData(m_comMachine);
        else
            m_pDetailsWidget->setData(*pSnapshotItem, pSnapshotItem->snapshot());
    }
}

void UISnapshotPane::sltHandleContextMenuRequest(const QPoint &position)
{
    /* Search for corresponding item: */
    const QTreeWidgetItem *pItem = m_pSnapshotTree->itemAt(position);
    if (!pItem)
        return;

    /* Acquire corresponding snapshot item: */
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(pItem);
    AssertReturnVoid(pSnapshotItem);

    /* Prepare menu: */
    QMenu menu;
    /* For snapshot item: */
    if (m_pCurrentSnapshotItem && !pSnapshotItem->isCurrentStateItem())
    {
        menu.addAction(m_pActionDeleteSnapshot);
        menu.addSeparator();
        menu.addAction(m_pActionRestoreSnapshot);
        menu.addAction(m_pActionShowSnapshotDetails);
        menu.addSeparator();
        menu.addAction(m_pActionCloneSnapshot);
    }
    /* For "current state" item: */
    else
    {
        menu.addAction(m_pActionTakeSnapshot);
        menu.addSeparator();
        menu.addAction(m_pActionCloneSnapshot);
    }

    /* Show menu: */
    menu.exec(m_pSnapshotTree->viewport()->mapToGlobal(position));
}

void UISnapshotPane::sltHandleItemChange(QTreeWidgetItem *pItem)
{
    /* Make sure nothing being edited in the meantime: */
    if (!m_pLockReadWrite->tryLockForWrite())
        return;

    /* Acquire corresponding snapshot item: */
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(pItem);
    AssertPtr(pSnapshotItem);
    if (pSnapshotItem)
    {
        /* Make sure that's a snapshot item indeed: */
        CSnapshot comSnapshot = pSnapshotItem->snapshot();
        if (comSnapshot.isNotNull())
        {
            /* Rename corresponding snapshot if necessary: */
            if (comSnapshot.GetName() != pSnapshotItem->text(Column_Name))
            {
                /* We need to open a session when we manipulate the snapshot data of a machine: */
                CSession comSession = vboxGlobal().openExistingSession(comSnapshot.GetMachine().GetId());
                if (!comSession.isNull())
                {
                    /// @todo Add settings save validation.

                    /* Save snapshot name: */
                    comSnapshot.SetName(pSnapshotItem->text(Column_Name));

                    /* Close the session again: */
                    comSession.UnlockMachine();
                }
            }
        }
    }

    /* Allows editing again: */
    m_pLockReadWrite->unlock();

    /* Adjust snapshot tree: */
    adjustTreeWidget();
}

void UISnapshotPane::sltHandleItemDoubleClick(QTreeWidgetItem *pItem)
{
    /* Acquire corresponding snapshot item: */
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(pItem);
    AssertReturnVoid(pSnapshotItem);

    /* If this is a snapshot item: */
    if (pSnapshotItem)
    {
        /* Handle Ctrl+DoubleClick: */
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
        {
            /* As snapshot-restore procedure: */
            if (pSnapshotItem->isCurrentStateItem())
                takeSnapshot(true /* automatically */);
            else
                restoreSnapshot(true /* suppress non-critical warnings */);
        }
        /* Handle Ctrl+Shift+DoubleClick: */
        else if (QApplication::keyboardModifiers() == (Qt::KeyboardModifiers)(Qt::ControlModifier | Qt::ShiftModifier))
        {
            /* As snapshot-delete procedure: */
            if (!pSnapshotItem->isCurrentStateItem())
                deleteSnapshot(true /* automatically */);
        }
        /* Handle other kinds of DoubleClick: */
        else
        {
            /* As show details-widget procedure: */
            m_pActionShowSnapshotDetails->setChecked(true);
        }
    }
}

void UISnapshotPane::prepare()
{
    /* Configure Main event connections: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineDataChange,
            this, &UISnapshotPane::sltHandleMachineDataChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineStateChange,
            this, &UISnapshotPane::sltHandleMachineStateChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSessionStateChange,
            this, &UISnapshotPane::sltHandleSessionStateChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotTake,
            this, &UISnapshotPane::sltHandleSnapshotTake);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotDelete,
            this, &UISnapshotPane::sltHandleSnapshotDelete);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotChange,
            this, &UISnapshotPane::sltHandleSnapshotChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotRestore,
            this, &UISnapshotPane::sltHandleSnapshotRestore);

    /* Create read-write locker: */
    m_pLockReadWrite = new QReadWriteLock;

    /* Create pixmaps: */
    m_pIconSnapshotOffline = new QIcon(UIIconPool::iconSet(":/snapshot_offline_16px.png"));
    m_pIconSnapshotOnline = new QIcon(UIIconPool::iconSet(":/snapshot_online_16px.png"));

    /* Create timer: */
    m_pTimerUpdateAge = new QTimer;
    AssertPtrReturnVoid(m_pTimerUpdateAge);
    {
        /* Configure timer: */
        m_pTimerUpdateAge->setSingleShot(true);
        connect(m_pTimerUpdateAge, &QTimer::timeout, this, &UISnapshotPane::sltUpdateSnapshotsAge);
    }

    /* Prepare widgets: */
    prepareWidgets();

    /* Load settings: */
    loadSettings();

    /* Apply language settings: */
    retranslateUi();
}

void UISnapshotPane::prepareWidgets()
{
    /* Create layout: */
    new QVBoxLayout(this);
    AssertPtrReturnVoid(layout());
    {
        /* Configure layout: */
        layout()->setContentsMargins(0, 0, 0, 0);
#ifdef VBOX_WS_MAC
        layout()->setSpacing(10);
#else
        layout()->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 2);
#endif

        /* Prepare toolbar: */
        prepareToolbar();
        /* Prepare snapshot tree: */
        prepareTreeWidget();
        /* Prepare details-widget: */
        prepareDetailsWidget();
    }
}

void UISnapshotPane::prepareToolbar()
{
    /* Create snapshot toolbar: */
    m_pToolBar = new UIToolBar(this);
    AssertPtrReturnVoid(m_pToolBar);
    {
        /* Configure toolbar: */
        const int iIconMetric = (int)(QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * 1.375);
        m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        /* Add Take Snapshot action: */
        m_pActionTakeSnapshot = m_pToolBar->addAction(UIIconPool::iconSetFull(":/snapshot_take_22px.png",
                                                                              ":/snapshot_take_16px.png",
                                                                              ":/snapshot_take_disabled_22px.png",
                                                                              ":/snapshot_take_disabled_16px.png"),
                                                      QString(), this, &UISnapshotPane::sltTakeSnapshot);
        {
            m_pActionTakeSnapshot->setShortcut(QString("Ctrl+Shift+T"));
        }

        /* Add Delete Snapshot action: */
        m_pActionDeleteSnapshot = m_pToolBar->addAction(UIIconPool::iconSetFull(":/snapshot_delete_22px.png",
                                                                                ":/snapshot_delete_16px.png",
                                                                                ":/snapshot_delete_disabled_22px.png",
                                                                                ":/snapshot_delete_disabled_16px.png"),
                                                        QString(), this, &UISnapshotPane::sltDeleteSnapshot);
        {
            m_pActionDeleteSnapshot->setShortcut(QString("Ctrl+Shift+D"));
        }

        m_pToolBar->addSeparator();

        /* Add Restore Snapshot action: */
        m_pActionRestoreSnapshot = m_pToolBar->addAction(UIIconPool::iconSetFull(":/snapshot_restore_22px.png",
                                                                                 ":/snapshot_restore_16px.png",
                                                                                 ":/snapshot_restore_disabled_22px.png",
                                                                                 ":/snapshot_restore_disabled_16px.png"),
                                                         QString(), this, &UISnapshotPane::sltRestoreSnapshot);
        {
            m_pActionRestoreSnapshot->setShortcut(QString("Ctrl+Shift+R"));
        }

        /* Add Show Snapshot Details action: */
        m_pActionShowSnapshotDetails = m_pToolBar->addAction(UIIconPool::iconSetFull(":/snapshot_show_details_22px.png",
                                                                                     ":/snapshot_show_details_16px.png",
                                                                                     ":/snapshot_show_details_disabled_22px.png",
                                                                                     ":/snapshot_show_details_disabled_16px.png"),
                                                             QString());
        {
            connect(m_pActionShowSnapshotDetails, &QAction::toggled,
                    this, &UISnapshotPane::sltToggleSnapshotDetailsVisibility);
            m_pActionShowSnapshotDetails->setCheckable(true);
            m_pActionShowSnapshotDetails->setShortcut(QString("Ctrl+Space"));
        }

        m_pToolBar->addSeparator();

        /* Add Clone Snapshot action: */
        m_pActionCloneSnapshot = m_pToolBar->addAction(UIIconPool::iconSetFull(":/vm_clone_22px.png",
                                                                               ":/vm_clone_16px.png",
                                                                               ":/vm_clone_disabled_22px.png",
                                                                               ":/vm_clone_disabled_16px.png"),
                                                       QString(), this, &UISnapshotPane::sltCloneSnapshot);
        {
            m_pActionCloneSnapshot->setShortcut(QString("Ctrl+Shift+C"));
        }

        /* Add into layout: */
        layout()->addWidget(m_pToolBar);
    }
}

void UISnapshotPane::prepareTreeWidget()
{
    /* Create snapshot tree: */
    m_pSnapshotTree = new UISnapshotTree(this);
    AssertPtrReturnVoid(m_pSnapshotTree);
    {
        /* Configure tree: */
        connect(m_pSnapshotTree, &UISnapshotTree::currentItemChanged,
                this, &UISnapshotPane::sltHandleCurrentItemChange);
        connect(m_pSnapshotTree, &UISnapshotTree::customContextMenuRequested,
                this, &UISnapshotPane::sltHandleContextMenuRequest);
        connect(m_pSnapshotTree, &UISnapshotTree::itemChanged,
                this, &UISnapshotPane::sltHandleItemChange);
        connect(m_pSnapshotTree, &UISnapshotTree::itemDoubleClicked,
                this, &UISnapshotPane::sltHandleItemDoubleClick);

        /* Add into layout: */
        layout()->addWidget(m_pSnapshotTree);
    }
}

void UISnapshotPane::prepareDetailsWidget()
{
    /* Create details-widget: */
    m_pDetailsWidget = new UISnapshotDetailsWidget;
    AssertPtrReturnVoid(m_pDetailsWidget);
    {
        /* Configure details-widget: */
        m_pDetailsWidget->setVisible(false);
        connect(m_pDetailsWidget, &UISnapshotDetailsWidget::sigDataChangeAccepted,
                this, &UISnapshotPane::sltApplySnapshotDetailsChanges);

        /* Add into layout: */
        layout()->addWidget(m_pDetailsWidget);
    }
}

void UISnapshotPane::loadSettings()
{
    /* Details action/widget: */
    m_pActionShowSnapshotDetails->setChecked(gEDataManager->snapshotManagerDetailsExpanded());
}

void UISnapshotPane::refreshAll()
{
    /* Prevent snapshot editing in the meantime: */
    QWriteLocker locker(m_pLockReadWrite);

    /* If VM is null, just updated the current item: */
    if (m_comMachine.isNull())
    {
        sltHandleCurrentItemChange();
        return;
    }

    /* Remember the selected item and it's first child: */
    QString strSelectedItem, strFirstChildOfSelectedItem;
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());
    if (pSnapshotItem)
    {
        strSelectedItem = pSnapshotItem->snapshotID();
        if (pSnapshotItem->child(0))
            strFirstChildOfSelectedItem = UISnapshotItem::toSnapshotItem(pSnapshotItem->child(0))->snapshotID();
    }

    /* Clear the tree: */
    m_pSnapshotTree->clear();

    /* If machine has snapshots: */
    if (m_comMachine.GetSnapshotCount() > 0)
    {
        /* Get the first snapshot: */
        const CSnapshot comSnapshot = m_comMachine.FindSnapshot(QString());

        /* Populate snapshot tree: */
        populateSnapshots(comSnapshot, 0);
        /* And make sure it has "current snapshot" item: */
        Assert(m_pCurrentSnapshotItem);

        /* Add the "current state" item as a child to "current snapshot" item: */
        m_pCurrentStateItem = new UISnapshotItem(this, m_pCurrentSnapshotItem, m_comMachine);
        m_pCurrentStateItem->recache();

        /* Search for a previously selected item: */
        UISnapshotItem *pCurrentItem = findItem(strSelectedItem);
        if (pCurrentItem == 0)
            pCurrentItem = findItem(strFirstChildOfSelectedItem);
        if (pCurrentItem == 0)
            pCurrentItem = m_pCurrentStateItem;

        /* Choose current item: */
        m_pSnapshotTree->scrollToItem(pCurrentItem);
        m_pSnapshotTree->setCurrentItem(pCurrentItem);
        sltHandleCurrentItemChange();
    }
    /* If machine has no snapshots: */
    else
    {
        /* There is no "current snapshot" item: */
        m_pCurrentSnapshotItem = 0;

        /* Add the "current state" item as a child of snapshot tree: */
        m_pCurrentStateItem = new UISnapshotItem(this, m_pSnapshotTree, m_comMachine);
        m_pCurrentStateItem->recache();

        /* Choose current item: */
        m_pSnapshotTree->setCurrentItem(m_pCurrentStateItem);
        sltHandleCurrentItemChange();
    }

    /* Update age: */
    sltUpdateSnapshotsAge();

    /* Adjust snapshot tree: */
    adjustTreeWidget();
}

void UISnapshotPane::populateSnapshots(const CSnapshot &comSnapshot, QITreeWidgetItem *pItem)
{
    /* Create a child of passed item: */
    UISnapshotItem *pSnapshotItem = pItem ? new UISnapshotItem(this, pItem, comSnapshot) :
                                            new UISnapshotItem(this, m_pSnapshotTree, comSnapshot);
    /* And recache it's content: */
    pSnapshotItem->recache();

    /* Mark snapshot item as "current" and remember it: */
    CSnapshot comCurrentSnapshot = m_comMachine.GetCurrentSnapshot();
    if (!comCurrentSnapshot.isNull() && comCurrentSnapshot.GetId() == comSnapshot.GetId())
    {
        pSnapshotItem->setCurrentSnapshotItem(true);
        m_pCurrentSnapshotItem = pSnapshotItem;
    }

    /* Walk through the children recursively: */
    foreach (const CSnapshot &comIteratedSnapshot, comSnapshot.GetChildren())
        populateSnapshots(comIteratedSnapshot, pSnapshotItem);

    /* Expand the newly created item: */
    pSnapshotItem->setExpanded(true);
    /* And mark it as editable: */
    pSnapshotItem->setFlags(pSnapshotItem->flags() | Qt::ItemIsEditable);
}

void UISnapshotPane::cleanup()
{
    /* Stop timer if active: */
    if (m_pTimerUpdateAge->isActive())
        m_pTimerUpdateAge->stop();
    /* Destroy timer: */
    delete m_pTimerUpdateAge;
    m_pTimerUpdateAge = 0;

    /* Destroy icons: */
    delete m_pIconSnapshotOffline;
    delete m_pIconSnapshotOnline;
    m_pIconSnapshotOffline = 0;
    m_pIconSnapshotOnline = 0;

    /* Destroy read-write locker: */
    delete m_pLockReadWrite;
    m_pLockReadWrite = 0;
}

void UISnapshotPane::updateActionStates()
{
    /* Acquire "current snapshot" item: */
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());

    /* Check whether another direct session is opened: */
    const bool fBusy = m_enmSessionState != KSessionState_Unlocked;

    /* Acquire machine-state of the "current state" item: */
    KMachineState enmState = KMachineState_Null;
    if (m_pCurrentStateItem)
        enmState = m_pCurrentStateItem->machineState();

    /* Determine whether taking or deleting snapshots is possible: */
    const bool fCanTakeDeleteSnapshot =    !fBusy
                                        || enmState == KMachineState_PoweredOff
                                        || enmState == KMachineState_Saved
                                        || enmState == KMachineState_Aborted
                                        || enmState == KMachineState_Running
                                        || enmState == KMachineState_Paused;

    /* Update 'Take' action: */
    m_pActionTakeSnapshot->setEnabled(
           m_fShapshotOperationsAllowed
        && (   (   fCanTakeDeleteSnapshot
                && m_pCurrentSnapshotItem
                && pSnapshotItem
                && pSnapshotItem->isCurrentStateItem())
            || (   pSnapshotItem
                && !m_pCurrentSnapshotItem))
    );

    /* Update 'Delete' action: */
    m_pActionDeleteSnapshot->setEnabled(
           m_fShapshotOperationsAllowed
        && fCanTakeDeleteSnapshot
        && m_pCurrentSnapshotItem
        && pSnapshotItem
        && !pSnapshotItem->isCurrentStateItem()
    );

    /* Update 'Restore' action: */
    m_pActionRestoreSnapshot->setEnabled(
           !fBusy
        && m_pCurrentSnapshotItem
        && pSnapshotItem
        && !pSnapshotItem->isCurrentStateItem()
    );

    /* Update 'Show Details' action: */
    m_pActionShowSnapshotDetails->setEnabled(
        pSnapshotItem
    );

    /* Update 'Clone' action: */
    m_pActionCloneSnapshot->setEnabled(
           pSnapshotItem
        && (   !pSnapshotItem->isCurrentStateItem()
            || !fBusy)
    );
}

bool UISnapshotPane::takeSnapshot(bool fAutomatically /* = false */)
{
    /* Simulate try-catch block: */
    bool fSuccess = false;
    do
    {
        /* Open a session (this call will handle all errors): */
        CSession comSession;
        if (m_enmSessionState != KSessionState_Unlocked)
            comSession = vboxGlobal().openExistingSession(m_strMachineId);
        else
            comSession = vboxGlobal().openSession(m_strMachineId);
        if (comSession.isNull())
            break;

        /* Simulate try-catch block: */
        do
        {
            /* Get corresponding machine object: */
            CMachine comMachine = comSession.GetMachine();

            /* Search for a maximum available snapshot index: */
            int iMaximumIndex = 0;
            const QString strNameTemplate = tr("Snapshot %1");
            const QRegExp reName(QString("^") + strNameTemplate.arg("([0-9]+)") + QString("$"));
            QTreeWidgetItemIterator iterator(m_pSnapshotTree);
            while (*iterator)
            {
                const QString strName = static_cast<UISnapshotItem*>(*iterator)->text(Column_Name);
                const int iPosition = reName.indexIn(strName);
                if (iPosition != -1)
                    iMaximumIndex = reName.cap(1).toInt() > iMaximumIndex
                                  ? reName.cap(1).toInt()
                                  : iMaximumIndex;
                ++iterator;
            }

            /* Prepare final snapshot name/description: */
            QString strFinalName = strNameTemplate.arg(iMaximumIndex + 1);
            QString strFinalDescription;

            /* In manual mode we should show take snapshot dialog: */
            if (!fAutomatically)
            {
                /* Create take-snapshot dialog: */
                QWidget *pDlgParent = windowManager().realParentWindow(this);
                QPointer<UITakeSnapshotDialog> pDlg = new UITakeSnapshotDialog(pDlgParent, comMachine);
                windowManager().registerNewParent(pDlg, pDlgParent);

                /* Assign corresponding icon: */
                QPixmap pixmap = vboxGlobal().vmUserPixmapDefault(comMachine);
                if (pixmap.isNull())
                    pixmap = vboxGlobal().vmGuestOSTypePixmapDefault(comMachine.GetOSTypeId());
                pDlg->setPixmap(pixmap);

                /* Assign corresponding snapshot name: */
                pDlg->setName(strFinalName);

                /* Show Take Snapshot dialog: */
                if (pDlg->exec() != QDialog::Accepted)
                {
                    /* Cleanup dialog if it wasn't destroyed in own loop: */
                    if (pDlg)
                        delete pDlg;
                    break;
                }

                /* Acquire final snapshot name/description: */
                strFinalName = pDlg->name().trimmed();
                strFinalDescription = pDlg->description();

                /* Cleanup dialog: */
                delete pDlg;
            }

            /* Take snapshot: */
            QString strSnapshotId;
            CProgress comProgress = comMachine.TakeSnapshot(strFinalName, strFinalDescription, true, strSnapshotId);
            if (!comMachine.isOk())
            {
                msgCenter().cannotTakeSnapshot(comMachine, comMachine.GetName());
                break;
            }

            /* Show snapshot taking progress: */
            msgCenter().showModalProgressDialog(comProgress, comMachine.GetName(), ":/progress_snapshot_create_90px.png");
            if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
            {
                msgCenter().cannotTakeSnapshot(comProgress, comMachine.GetName());
                break;
            }

            /* Mark snapshot restoring successful: */
            fSuccess = true;
        }
        while (0);

        /* Cleanup try-catch block: */
        comSession.UnlockMachine();
    }
    while (0);

    /* Adjust snapshot tree: */
    adjustTreeWidget();

    /* Return result: */
    return fSuccess;
}

bool UISnapshotPane::deleteSnapshot(bool fAutomatically /* = false */)
{
    /* Simulate try-catch block: */
    bool fSuccess = false;
    do
    {
        /* Acquire "current snapshot" item: */
        const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());
        AssertPtr(pSnapshotItem);
        if (!pSnapshotItem)
            break;

        /* Get corresponding snapshot: */
        const CSnapshot comSnapshot = pSnapshotItem->snapshot();
        Assert(!comSnapshot.isNull());
        if (comSnapshot.isNull())
            break;

        /* In manual mode we should ask if user really wants to remove the selected snapshot: */
        if (!fAutomatically && !msgCenter().confirmSnapshotRemoval(comSnapshot.GetName()))
            break;

        /** @todo check available space on the target filesystem etc etc. */
#if 0
        if (!msgCenter().warnAboutSnapshotRemovalFreeSpace(comSnapshot.GetName(),
                                                           "/home/juser/.VirtualBox/Machines/SampleVM/Snapshots/{01020304-0102-0102-0102-010203040506}.vdi",
                                                           "59 GiB",
                                                           "15 GiB"))
            break;
#endif

        /* Open a session (this call will handle all errors): */
        CSession comSession;
        if (m_enmSessionState != KSessionState_Unlocked)
            comSession = vboxGlobal().openExistingSession(m_strMachineId);
        else
            comSession = vboxGlobal().openSession(m_strMachineId);
        if (comSession.isNull())
            break;

        /* Simulate try-catch block: */
        do
        {
            /* Remove chosen snapshot: */
            CMachine comMachine = comSession.GetMachine();
            CProgress comProgress = comMachine.DeleteSnapshot(pSnapshotItem->snapshotID());
            if (!comMachine.isOk())
            {
                msgCenter().cannotRemoveSnapshot(comMachine,  comSnapshot.GetName(), m_comMachine.GetName());
                break;
            }

            /* Show snapshot removing progress: */
            msgCenter().showModalProgressDialog(comProgress, m_comMachine.GetName(), ":/progress_snapshot_discard_90px.png");
            if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
            {
                msgCenter().cannotRemoveSnapshot(comProgress,  comSnapshot.GetName(), m_comMachine.GetName());
                break;
            }

            /* Mark snapshot removing successful: */
            fSuccess = true;
        }
        while (0);

        /* Cleanup try-catch block: */
        comSession.UnlockMachine();
    }
    while (0);

    /* Adjust snapshot tree: */
    adjustTreeWidget();

    /* Return result: */
    return fSuccess;
}

bool UISnapshotPane::restoreSnapshot(bool fAutomatically /* = false */)
{
    /* Simulate try-catch block: */
    bool fSuccess = false;
    do
    {
        /* Acquire "current snapshot" item: */
        const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());
        AssertPtr(pSnapshotItem);
        if (!pSnapshotItem)
            break;

        /* Get corresponding snapshot: */
        const CSnapshot comSnapshot = pSnapshotItem->snapshot();
        Assert(!comSnapshot.isNull());
        if (comSnapshot.isNull())
            break;

        /* In manual mode we should check whether current state is changed: */
        if (!fAutomatically && m_comMachine.GetCurrentStateModified())
        {
            /* Ask if user really wants to restore the selected snapshot: */
            int iResultCode = msgCenter().confirmSnapshotRestoring(comSnapshot.GetName(), m_comMachine.GetCurrentStateModified());
            if (iResultCode & AlertButton_Cancel)
                break;

            /* Ask if user also wants to create new snapshot of current state which is changed: */
            if (iResultCode & AlertOption_CheckBox)
            {
                /* Take snapshot of changed current state: */
                m_pSnapshotTree->setCurrentItem(m_pCurrentStateItem);
                if (!takeSnapshot())
                    break;
            }
        }

        /* Open a direct session (this call will handle all errors): */
        CSession comSession = vboxGlobal().openSession(m_strMachineId);
        if (comSession.isNull())
            break;

        /* Simulate try-catch block: */
        do
        {
            /* Restore chosen snapshot: */
            CMachine comMachine = comSession.GetMachine();
            CProgress comProgress = comMachine.RestoreSnapshot(comSnapshot);
            if (!comMachine.isOk())
            {
                msgCenter().cannotRestoreSnapshot(comMachine, comSnapshot.GetName(), m_comMachine.GetName());
                break;
            }

            /* Show snapshot restoring progress: */
            msgCenter().showModalProgressDialog(comProgress, m_comMachine.GetName(), ":/progress_snapshot_restore_90px.png");
            if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
            {
                msgCenter().cannotRestoreSnapshot(comProgress, comSnapshot.GetName(), m_comMachine.GetName());
                break;
            }

            /* Mark snapshot restoring successful: */
            fSuccess = true;
        }
        while (0);

        /* Cleanup try-catch block: */
        comSession.UnlockMachine();
    }
    while (0);

    /* Adjust snapshot tree: */
    adjustTreeWidget();

    /* Return result: */
    return fSuccess;
}

void UISnapshotPane::cloneSnapshot()
{
    /* Acquire "current snapshot" item: */
    const UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(m_pSnapshotTree->currentItem());
    AssertReturnVoid(pSnapshotItem);

    /* Get desired machine/snapshot: */
    CMachine comMachine;
    CSnapshot comSnapshot;
    if (pSnapshotItem->isCurrentStateItem())
        comMachine = pSnapshotItem->machine();
    else
    {
        comSnapshot = pSnapshotItem->snapshot();
        AssertReturnVoid(!comSnapshot.isNull());
        comMachine = comSnapshot.GetMachine();
    }
    AssertReturnVoid(!comMachine.isNull());

    /* Show Clone VM wizard: */
    UISafePointerWizard pWizard = new UIWizardCloneVM(this, comMachine, comSnapshot);
    pWizard->prepare();
    pWizard->exec();
    if (pWizard)
        delete pWizard;
}

void UISnapshotPane::adjustTreeWidget()
{
    /* Get the snapshot tree abstract interface: */
    QAbstractItemView *pItemView = m_pSnapshotTree;
    /* Get the snapshot tree header-view: */
    QHeaderView *pItemHeader = m_pSnapshotTree->header();

    /* Calculate the total snapshot tree width: */
    const int iTotal = m_pSnapshotTree->viewport()->width();
    /* Look for a minimum width hints for non-important columns: */
    const int iMinWidth1 = qMax(pItemView->sizeHintForColumn(Column_Taken), pItemHeader->sectionSizeHint(Column_Taken));
    /* Propose suitable width hints for non-important columns: */
    const int iWidth1 = iMinWidth1 < iTotal / Column_Max ? iMinWidth1 : iTotal / Column_Max;
    /* Apply the proposal: */
    m_pSnapshotTree->setColumnWidth(Column_Taken, iWidth1);
    m_pSnapshotTree->setColumnWidth(Column_Name, iTotal - iWidth1);
}

UISnapshotItem *UISnapshotPane::findItem(const QString &strSnapshotID) const
{
    /* Search for the first item with required ID: */
    QTreeWidgetItemIterator it(m_pSnapshotTree);
    while (*it)
    {
        UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(*it);
        if (pSnapshotItem->snapshotID() == strSnapshotID)
            return pSnapshotItem;
        ++it;
    }

    /* Null by default: */
    return 0;
}

SnapshotAgeFormat UISnapshotPane::traverseSnapshotAge(QTreeWidgetItem *pItem) const
{
    /* Acquire corresponding snapshot item: */
    UISnapshotItem *pSnapshotItem = UISnapshotItem::toSnapshotItem(pItem);

    /* Fetch the snapshot age of the root if it's valid: */
    SnapshotAgeFormat age = pSnapshotItem ? pSnapshotItem->updateAge() : SnapshotAgeFormat_Max;

    /* Walk through the children recursively: */
    for (int i = 0; i < pItem->childCount(); ++i)
    {
        /* Fetch the smallest snapshot age of the children: */
        const SnapshotAgeFormat newAge = traverseSnapshotAge(pItem->child(i));
        /* Remember the smallest snapshot age among existing: */
        age = newAge < age ? newAge : age;
    }

    /* Return result: */
    return age;
}

void UISnapshotPane::expandItemChildren(QTreeWidgetItem *pItem)
{
    pItem ->setExpanded(true);
    for (int i = 0; i < pItem->childCount(); ++i)
        expandItemChildren(pItem->child(i));
}

#include "UISnapshotPane.moc"


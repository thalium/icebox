/* $Id: UIGChooserModel.h $ */
/** @file
 * VBox Qt GUI - UIGChooserModel class declaration.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIGChooserModel_h__
#define __UIGChooserModel_h__

/* Qt includes: */
#include <QObject>
#include <QPointer>
#include <QTransform>
#include <QMap>
#include <QThread>

/* GUI includes: */
#include "UIGChooserItem.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declaration: */
class QGraphicsScene;
class QGraphicsItem;
class QDrag;
class QMenu;
class QAction;
class QGraphicsSceneContextMenuEvent;
class QTimer;
class QPaintDevice;
class UIVMItem;
class UIGChooser;
class UIActionPool;
class UIGChooserHandlerMouse;
class UIGChooserHandlerKeyboard;
class CMachine;

/* Context-menu type: */
enum UIGraphicsSelectorContextMenuType
{
    UIGraphicsSelectorContextMenuType_Group,
    UIGraphicsSelectorContextMenuType_Machine
};

/* Graphics chooser-model: */
class UIGChooserModel : public QObject
{
    Q_OBJECT;

signals:

    /* Notifier: Current-item stuff: */
    void sigSelectionChanged();

    /* Notifier: Focus-item stuff: */
    void sigFocusChanged(UIGChooserItem *pFocusItem);

    /* Notifiers: Root-item stuff: */
    void sigRootItemMinimumWidthHintChanged(int iRootItemMinimumWidthHint);
    void sigRootItemMinimumHeightHintChanged(int iRootItemMinimumHeightHint);
    void sigSlidingStarted();

    /* Notifiers: Group-item stuff: */
    void sigToggleStarted();
    void sigToggleFinished();

    /* Notifiers: Group-saving stuff: */
    void sigStartGroupSaving();
    void sigGroupSavingStateChanged();

public:

    /* Constructor/destructor: */
    UIGChooserModel(UIGChooser *pParent);
    ~UIGChooserModel();

    /* API: Prepare/cleanup stuff: */
    void prepare();
    void cleanup();

    /** Returns the chooser reference. */
    UIGChooser* chooser() const { return m_pChooser; }
    /** Returns the action-pool reference. */
    UIActionPool* actionPool() const;

    /* API: Scene stuff: */
    QGraphicsScene* scene() const;
    QPaintDevice* paintDevice() const;
    QGraphicsItem* itemAt(const QPointF &position, const QTransform &deviceTransform = QTransform()) const;

    /* API: Layout stuff: */
    void updateLayout();

    /* API: Navigation stuff: */
    const QList<UIGChooserItem*>& navigationList() const;
    void removeFromNavigationList(UIGChooserItem *pItem);
    void updateNavigation();

    /* API: Current-item stuff: */
    UIVMItem* currentMachineItem() const;
    QList<UIVMItem*> currentMachineItems() const;
    UIGChooserItem* currentItem() const;
    const QList<UIGChooserItem*>& currentItems() const;
    void setCurrentItems(const QList<UIGChooserItem*> &items);
    void setCurrentItem(UIGChooserItem *pItem);
    void setCurrentItem(const QString &strDefinition);
    void unsetCurrentItem();
    void addToCurrentItems(UIGChooserItem *pItem);
    void removeFromCurrentItems(UIGChooserItem *pItem);
    void makeSureSomeItemIsSelected();
    void notifyCurrentItemChanged();
    bool isSingleGroupSelected() const;
    bool isAllItemsOfOneGroupSelected() const;

    /* API: Focus-item stuff: */
    UIGChooserItem* focusItem() const;
    void setFocusItem(UIGChooserItem *pItem);

    /* API: Root-item stuff: */
    UIGChooserItem* mainRoot() const;
    UIGChooserItem* root() const;
    void indentRoot(UIGChooserItem *pNewRootItem);
    void unindentRoot();
    bool isSlidingInProgress() const;

    /* API: Group-item stuff: */
    void startEditingGroupItemName();
    void cleanupGroupTree();
    static QString uniqueGroupName(UIGChooserItem *pRoot);

    /* API: Machine-item stuff: */
    void activateMachineItem();

    /* API: Drag&drop stuff: */
    void setCurrentDragObject(QDrag *pDragObject);

    /* API: Item lookup stuff: */
    void lookFor(const QString &strLookupSymbol);
    bool isLookupInProgress() const;

    /* API: Saving stuff: */
    void saveGroupSettings();
    bool isGroupSavingInProgress() const;

private slots:

    /* Handlers: Global events: */
    void sltMachineStateChanged(QString strId, KMachineState state);
    void sltMachineDataChanged(QString strId);
    void sltMachineRegistered(QString strId, bool fRegistered);
    void sltSessionStateChanged(QString strId, KSessionState state);
    void sltSnapshotChanged(QString strId, QString strSnapshotId);

    /* Handler: Chooser-view stuff: */
    void sltHandleViewResized();

    /* Handler: Focus-item stuff: */
    void sltFocusItemDestroyed();

    /* Handlers: Root-item stuff: */
    void sltLeftRootSlidingProgress();
    void sltRightRootSlidingProgress();
    void sltSlidingComplete();

    /* Handlers: Group-item stuff: */
    void sltEditGroupName();
    void sltSortGroup();
    void sltUngroupSelectedGroup();

    /* Handlers: Machine-item stuff: */
    void sltCreateNewMachine();
    void sltGroupSelectedMachines();
    void sltReloadMachine(const QString &strId);
    void sltSortParentGroup();
    void sltPerformRefreshAction();
    void sltRemoveSelectedMachine();

    /* Handlers: Drag&drop stuff: */
    void sltStartScrolling();
    void sltCurrentDragObjectDestroyed();

    /* Handler: Item lookup stuff: */
    void sltEraseLookupTimer();

    /* Handlers: Saving stuff: */
    void sltGroupSavingStart();
    void sltGroupDefinitionsSaveComplete();
    void sltGroupOrdersSaveComplete();

private:

    /* Data enumerator: */
    enum ChooserModelData
    {
        /* Layout margin: */
        ChooserModelData_Margin
    };

    /* Data provider: */
    QVariant data(int iKey) const;

    /* Helpers: Prepare stuff: */
    void prepareScene();
    void prepareRoot();
    void prepareLookup();
    void prepareContextMenu();
    void prepareHandlers();
    void prepareConnections();
    void loadLastSelectedItem();

    /* Helpers: Cleanup stuff: */
    void saveLastSelectedItem();
    void cleanupHandlers();
    void cleanupContextMenu();
    void cleanupLookup();
    void cleanupRoot();
    void cleanupScene();

    /* Handler: Event-filter: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Helper: Navigation stuff: */
    QList<UIGChooserItem*> createNavigationList(UIGChooserItem *pItem);

    /* Helper: Focus-item stuff: */
    void clearRealFocus();

    /* Helper: Root-item stuff: */
    void slideRoot(bool fForward);

    /* Helper: Group-item stuff: */
    void cleanupGroupTree(UIGChooserItem *pGroupItem);

    /* Helpers: Machine-item stuff: */
    void removeItems(const QList<UIGChooserItem*> &itemsToRemove);
    void unregisterMachines(const QStringList &ids);

    /* Helpers: Context-menu stuff: */
    bool processContextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent);
    void popupContextMenu(UIGraphicsSelectorContextMenuType type, QPoint point);

    /* Handler: Drag&drop event: */
    bool processDragMoveEvent(QGraphicsSceneDragDropEvent *pEvent);
    bool processDragLeaveEvent(QGraphicsSceneDragDropEvent *pEvent);

    /* Helpers: Loading stuff: */
    void loadGroupTree();
    void addMachineIntoTheTree(const CMachine &machine, bool fMakeItVisible = false);
    UIGChooserItem* getGroupItem(const QString &strName, UIGChooserItem *pParentItem, bool fAllGroupsOpened);
    bool shouldBeGroupOpened(UIGChooserItem *pParentItem, const QString &strName);
    int getDesiredPosition(UIGChooserItem *pParentItem, UIGChooserItemType type, const QString &strName);
    int positionFromDefinitions(UIGChooserItem *pParentItem, UIGChooserItemType type, const QString &strName);
    void createMachineItem(const CMachine &machine, UIGChooserItem *pParentItem);

    /* Helpers: Saving stuff: */
    void saveGroupDefinitions();
    void saveGroupOrders();
    void gatherGroupDefinitions(QMap<QString, QStringList> &groups, UIGChooserItem *pParentGroup);
    void gatherGroupOrders(QMap<QString, QStringList> &groups, UIGChooserItem *pParentItem);
    void makeSureGroupDefinitionsSaveIsFinished();
    void makeSureGroupOrdersSaveIsFinished();

    /** Holds the chooser reference. */
    UIGChooser *m_pChooser;

    /* Variables: */
    QGraphicsScene *m_pScene;

    QList<UIGChooserItem*> m_rootStack;
    bool m_fSliding;
    UIGChooserItem *m_pLeftRoot;
    UIGChooserItem *m_pRightRoot;
    QPointer<UIGChooserItem> m_pAfterSlidingFocus;

    QMap<QString, QStringList> m_groups;
    QList<UIGChooserItem*> m_navigationList;
    QList<UIGChooserItem*> m_currentItems;
    UIGChooserHandlerMouse *m_pMouseHandler;
    UIGChooserHandlerKeyboard *m_pKeyboardHandler;
    QPointer<QDrag> m_pCurrentDragObject;
    int m_iScrollingTokenSize;
    bool m_fIsScrollingInProgress;
    QPointer<UIGChooserItem> m_pFocusItem;
    QMenu *m_pContextMenuGroup;
    QMenu *m_pContextMenuMachine;

    /* Variables: Lookup stuff: */
    QTimer *m_pLookupTimer;
    QString m_strLookupString;

    /** Holds the Id of last VM created from the GUI side. */
    QString m_strLastCreatedMachineId;
};

/* Allows to save group definitions asynchronously: */
class UIGroupDefinitionSaveThread : public QThread
{
    Q_OBJECT;

signals:

    /* Notifier: Reload stuff: */
    void sigReload(QString strId);

    /* Notifier: Complete stuff: */
    void sigComplete();

public:

    /* Singleton stuff: */
    static UIGroupDefinitionSaveThread* instance();
    static void prepare();
    static void cleanup();

    /* API: Configuring stuff: */
    void configure(QObject *pParent,
                   const QMap<QString, QStringList> &oldLists,
                   const QMap<QString, QStringList> &newLists);

private:

    /* Constructor/destructor: */
    UIGroupDefinitionSaveThread();
    ~UIGroupDefinitionSaveThread();

    /* Worker thread stuff: */
    void run();

    /* Variables: */
    static UIGroupDefinitionSaveThread *m_spInstance;
    QMap<QString, QStringList> m_oldLists;
    QMap<QString, QStringList> m_newLists;
};

/* Allows to save group order asynchronously: */
class UIGroupOrderSaveThread : public QThread
{
    Q_OBJECT;

signals:

    /* Notifier: Complete stuff: */
    void sigComplete();

public:

    /* Singleton stuff: */
    static UIGroupOrderSaveThread* instance();
    static void prepare();
    static void cleanup();

    /* API: Configuring stuff: */
    void configure(QObject *pParent, const QMap<QString, QStringList> &groups);

private:

    /* Constructor/destructor: */
    UIGroupOrderSaveThread();
    ~UIGroupOrderSaveThread();

    /* Worker thread stuff: */
    void run();

    /* Variables: */
    static UIGroupOrderSaveThread *m_spInstance;
    QMap<QString, QStringList> m_groups;
};

#endif /* __UIGChooserModel_h__ */


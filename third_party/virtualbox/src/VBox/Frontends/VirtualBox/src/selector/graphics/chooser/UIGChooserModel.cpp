/* $Id: UIGChooserModel.cpp $ */
/** @file
 * VBox Qt GUI - UIGChooserModel class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QGraphicsScene>
# include <QGraphicsView>
# include <QRegExp>
# include <QGraphicsSceneMouseEvent>
# include <QGraphicsSceneContextMenuEvent>
# include <QPropertyAnimation>
# include <QScrollBar>
# include <QTimer>
# include <QDrag>

/* GUI includes: */
# include "UIGChooser.h"
# include "UIGChooserModel.h"
# include "UIGChooserItemGroup.h"
# include "UIGChooserItemMachine.h"
# include "UIExtraDataDefs.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIExtraDataManager.h"
# include "UIActionPoolSelector.h"
# include "UIGChooserHandlerMouse.h"
# include "UIGChooserHandlerKeyboard.h"
# include "UIWizardNewVM.h"
# include "UISelectorWindow.h"
# include "UIVirtualBoxEventHandler.h"
# include "UIModalWindowManager.h"

/* COM includes: */
# include "CVirtualBox.h"
# include "CMachine.h"
# include "CMedium.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
#include <QParallelAnimationGroup>

/* Type defs: */
typedef QSet<QString> UIStringSet;


UIGChooserModel::UIGChooserModel(UIGChooser *pParent)
    : QObject(pParent)
    , m_pChooser(pParent)
    , m_pScene(0)
    , m_fSliding(false)
    , m_pLeftRoot(0)
    , m_pRightRoot(0)
    , m_pAfterSlidingFocus(0)
    , m_pMouseHandler(0)
    , m_pKeyboardHandler(0)
    , m_iScrollingTokenSize(30)
    , m_fIsScrollingInProgress(false)
    , m_pContextMenuGroup(0)
    , m_pContextMenuMachine(0)
    , m_pLookupTimer(0)
{
    /* Prepare scene: */
    prepareScene();

    /* Prepare root: */
    prepareRoot();

    /* Prepare lookup: */
    prepareLookup();

    /* Prepare context-menu: */
    prepareContextMenu();

    /* Prepare handlers: */
    prepareHandlers();

    /* Prepare connections: */
    prepareConnections();
}

UIGChooserModel::~UIGChooserModel()
{
    /* Cleanup handlers: */
    cleanupHandlers();

    /* Prepare context-menu: */
    cleanupContextMenu();

    /* Cleanup lookup: */
    cleanupLookup();

    /* Cleanup root: */
    cleanupRoot();

    /* Cleanup scene: */
    cleanupScene();
 }

void UIGChooserModel::prepare()
{
    /* Load group tree: */
    loadGroupTree();

    /* Update navigation: */
    updateNavigation();

    /* Update layout: */
    updateLayout();

    /* Load last selected item: */
    loadLastSelectedItem();
}

void UIGChooserModel::cleanup()
{
    /* Save last selected item: */
    saveLastSelectedItem();

    /* Currently we are not saving group descriptors
     * (which reflecting group toggle-state) on-the-fly
     * So, for now we are additionally save group orders
     * when exiting application: */
    saveGroupOrders();

    /* Make sure all saving steps complete: */
    makeSureGroupDefinitionsSaveIsFinished();
    makeSureGroupOrdersSaveIsFinished();
}

UIActionPool* UIGChooserModel::actionPool() const
{
    return chooser()->actionPool();
}

QGraphicsScene* UIGChooserModel::scene() const
{
    return m_pScene;
}

QPaintDevice* UIGChooserModel::paintDevice() const
{
    if (!m_pScene || m_pScene->views().isEmpty())
        return 0;
    return m_pScene->views().first();
}

QGraphicsItem* UIGChooserModel::itemAt(const QPointF &position, const QTransform &deviceTransform /* = QTransform() */) const
{
    return scene()->itemAt(position, deviceTransform);
}

void UIGChooserModel::updateLayout()
{
    /* No layout updates while sliding: */
    if (m_fSliding)
        return;

    /* Initialize variables: */
    int iSceneMargin = data(ChooserModelData_Margin).toInt();
    QSize viewportSize = scene()->views()[0]->viewport()->size();
    int iViewportWidth = viewportSize.width() - 2 * iSceneMargin;
    int iViewportHeight = viewportSize.height() - 2 * iSceneMargin;
    /* Set root-item position: */
    root()->setPos(iSceneMargin, iSceneMargin);
    /* Set root-item size: */
    root()->resize(iViewportWidth, iViewportHeight);
    /* Relayout root-item: */
    root()->updateLayout();
    /* Make sure root-item is shown: */
    root()->show();
}

const QList<UIGChooserItem*>& UIGChooserModel::navigationList() const
{
    return m_navigationList;
}

void UIGChooserModel::removeFromNavigationList(UIGChooserItem *pItem)
{
    AssertMsg(pItem, ("Passed item is invalid!"));
    m_navigationList.removeAll(pItem);
}

void UIGChooserModel::updateNavigation()
{
    m_navigationList.clear();
    m_navigationList = createNavigationList(root());
}

UIVMItem* UIGChooserModel::currentMachineItem() const
{
    /* Return first machine-item of the current-item: */
    return currentItem() ? currentItem()->firstMachineItem() : 0;
}

QList<UIVMItem*> UIGChooserModel::currentMachineItems() const
{
    /* Gather list of current unique machine-items: */
    QList<UIGChooserItemMachine*> currentMachineItemList;
    UIGChooserItemMachine::enumerateMachineItems(currentItems(), currentMachineItemList,
                                                 UIGChooserItemMachineEnumerationFlag_Unique);

    /* Reintegrate machine-items into valid format: */
    QList<UIVMItem*> currentMachineList;
    foreach (UIGChooserItemMachine *pItem, currentMachineItemList)
        currentMachineList << pItem;
    return currentMachineList;
}

UIGChooserItem* UIGChooserModel::currentItem() const
{
    /* Return first of current items, if any: */
    return currentItems().isEmpty() ? 0 : currentItems().first();
}

const QList<UIGChooserItem*>& UIGChooserModel::currentItems() const
{
    return m_currentItems;
}

void UIGChooserModel::setCurrentItems(const QList<UIGChooserItem*> &items)
{
    /* Is there something seems to be changed? */
    if (m_currentItems == items)
        return;

    /* Remember old current-item list: */
    QList<UIGChooserItem*> oldCurrentItems = m_currentItems;

    /* Clear current current-item list: */
    m_currentItems.clear();

    /* Iterate over all the passed items: */
    foreach (UIGChooserItem *pItem, items)
    {
        /* If navigation list contains iterated-item: */
        if (pItem && navigationList().contains(pItem))
        {
            /* Add that item to current: */
            m_currentItems << pItem;
        }
        else
            AssertMsgFailed(("Passed item not in navigation list!"));
    }

    /* Is there something really changed? */
    if (oldCurrentItems == m_currentItems)
        return;

    /* Update all the old items (they are no longer selected): */
    foreach (UIGChooserItem *pItem, oldCurrentItems)
        pItem->update();

    /* Update all the new items (they are selected): */
    foreach (UIGChooserItem *pItem, m_currentItems)
        pItem->update();

    /* Notify about selection changes: */
    notifyCurrentItemChanged();
}

void UIGChooserModel::setCurrentItem(UIGChooserItem *pItem)
{
    /* Call for wrapper above: */
    QList<UIGChooserItem*> items;
    if (pItem)
        items << pItem;
    setCurrentItems(items);

    /* Move focus to current-item: */
    setFocusItem(currentItem());
}

void UIGChooserModel::setCurrentItem(const QString &strDefinition)
{
    /* Ignore if empty definition passed: */
    if (strDefinition.isEmpty())
        return;

    /* Parse definition: */
    UIGChooserItem *pItem = 0;
    QString strItemType = strDefinition.section('=', 0, 0);
    QString strItemDescriptor = strDefinition.section('=', 1, -1);
    /* Its a group-item definition? */
    if (strItemType == "g")
    {
        /* Search for group-item with passed descriptor (name): */
        pItem = mainRoot()->searchForItem(strItemDescriptor,
                                          UIGChooserItemSearchFlag_Group |
                                          UIGChooserItemSearchFlag_ExactName);
    }
    /* Its a machine-item definition? */
    else if (strItemType == "m")
    {
        /* Check if machine-item with passed descriptor (name or id) registered: */
        CMachine machine = vboxGlobal().virtualBox().FindMachine(strItemDescriptor);
        if (!machine.isNull())
        {
            /* Search for machine-item with required name: */
            pItem = mainRoot()->searchForItem(machine.GetName(),
                                              UIGChooserItemSearchFlag_Machine |
                                              UIGChooserItemSearchFlag_ExactName);
        }
    }

    /* Make sure found item is in navigation list: */
    if (!pItem || !navigationList().contains(pItem))
        return;

    /* Call for wrapper above: */
    setCurrentItem(pItem);
}

void UIGChooserModel::unsetCurrentItem()
{
    /* Call for wrapper above: */
    setCurrentItem(0);
}

void UIGChooserModel::addToCurrentItems(UIGChooserItem *pItem)
{
    /* Call for wrapper above: */
    setCurrentItems(QList<UIGChooserItem*>(m_currentItems) << pItem);
}

void UIGChooserModel::removeFromCurrentItems(UIGChooserItem *pItem)
{
    /* Prepare filtered list: */
    QList<UIGChooserItem*> list(m_currentItems);
    list.removeAll(pItem);
    /* Call for wrapper above: */
    setCurrentItems(list);
}

void UIGChooserModel::makeSureSomeItemIsSelected()
{
    /* Make sure selection item list is never empty
     * if at least one item (for example 'focus') present: */
    if (!currentItem() && focusItem())
        setCurrentItem(focusItem());
}

void UIGChooserModel::notifyCurrentItemChanged()
{
    /* Notify listeners about selection change: */
    emit sigSelectionChanged();
}

bool UIGChooserModel::isSingleGroupSelected() const
{
    return currentItems().size() == 1 &&
           currentItem()->type() == UIGChooserItemType_Group;
}

bool UIGChooserModel::isAllItemsOfOneGroupSelected() const
{
    /* Make sure at least one item selected: */
    if (currentItems().isEmpty())
        return false;

    /* Determine the parent group of the first item: */
    UIGChooserItem *pFirstParent = currentItem()->parentItem();

    /* Make sure this parent is not main root-item: */
    if (pFirstParent == mainRoot())
        return false;

    /* Enumerate current-item set: */
    QSet<UIGChooserItem*> currentItemSet;
    foreach (UIGChooserItem *pCurrentItem, currentItems())
        currentItemSet << pCurrentItem;

    /* Enumerate first parent children set: */
    QSet<UIGChooserItem*> firstParentItemSet;
    foreach (UIGChooserItem *pFirstParentItem, pFirstParent->items())
        firstParentItemSet << pFirstParentItem;

    /* Check if both sets contains the same: */
    return currentItemSet == firstParentItemSet;
}

UIGChooserItem* UIGChooserModel::focusItem() const
{
    return m_pFocusItem;
}

void UIGChooserModel::setFocusItem(UIGChooserItem *pItem)
{
    /* Make sure real focus unset: */
    clearRealFocus();

    /* Is there something changed? */
    if (m_pFocusItem == pItem)
        return;

    /* Remember old focus-item: */
    UIGChooserItem *pOldFocusItem = m_pFocusItem;

    /* Set new focus-item: */
    m_pFocusItem = pItem;

    /* Disconnect old focus-item (if any): */
    if (pOldFocusItem)
        disconnect(pOldFocusItem, SIGNAL(destroyed(QObject*)), this, SLOT(sltFocusItemDestroyed()));
    /* Connect new focus-item (if any): */
    if (m_pFocusItem)
        connect(m_pFocusItem, SIGNAL(destroyed(QObject*)), this, SLOT(sltFocusItemDestroyed()));

    /* Notify listeners about focus change: */
    emit sigFocusChanged(m_pFocusItem);
}

UIGChooserItem* UIGChooserModel::mainRoot() const
{
    return m_rootStack.first();
}

UIGChooserItem* UIGChooserModel::root() const
{
    return m_rootStack.last();
}

void UIGChooserModel::indentRoot(UIGChooserItem *pNewRootItem)
{
    /* Do nothing if sliding already: */
    if (m_fSliding)
        return;

    /* We are sliding: */
    m_fSliding = true;
    emit sigSlidingStarted();

    /* Hiding root: */
    root()->hide();

    /* Create left root: */
    bool fLeftRootIsMain = root() == mainRoot();
    m_pLeftRoot = new UIGChooserItemGroup(scene(), root()->toGroupItem(), fLeftRootIsMain);
    m_pLeftRoot->setPos(0, 0);
    m_pLeftRoot->resize(root()->geometry().size());

    /* Create right root: */
    m_pRightRoot = new UIGChooserItemGroup(scene(), pNewRootItem->toGroupItem(), false);
    m_pRightRoot->setPos(root()->geometry().width(), 0);
    m_pRightRoot->resize(root()->geometry().size());

    /* Indent root: */
    root()->setRoot(false);
    m_rootStack << pNewRootItem;
    root()->setRoot(true);
    m_pAfterSlidingFocus = root()->items().first();

    /* Slide root: */
    slideRoot(true);
}

void UIGChooserModel::unindentRoot()
{
    /* Do nothing if sliding already: */
    if (m_fSliding)
        return;

    /* We are sliding: */
    m_fSliding = true;
    emit sigSlidingStarted();

    /* Hiding root: */
    root()->hide();
    root()->setRoot(false);

    /* Create left root: */
    bool fLeftRootIsMain = m_rootStack.at(m_rootStack.size() - 2) == mainRoot();
    m_pLeftRoot = new UIGChooserItemGroup(scene(), m_rootStack.at(m_rootStack.size() - 2)->toGroupItem(), fLeftRootIsMain);
    m_pLeftRoot->setPos(- root()->geometry().width(), 0);
    m_pLeftRoot->resize(root()->geometry().size());

    /* Create right root: */
    m_pRightRoot = new UIGChooserItemGroup(scene(), root()->toGroupItem(), false);
    m_pRightRoot->setPos(0, 0);
    m_pRightRoot->resize(root()->geometry().size());

    /* Unindent root: */
    m_pAfterSlidingFocus = root();
    m_rootStack.removeLast();
    root()->setRoot(true);

    /* Slide root: */
    slideRoot(false);
}

bool UIGChooserModel::isSlidingInProgress() const
{
    return m_fSliding;
}

void UIGChooserModel::startEditingGroupItemName()
{
    sltEditGroupName();
}

void UIGChooserModel::cleanupGroupTree()
{
    cleanupGroupTree(mainRoot());
}

/* static */
QString UIGChooserModel::uniqueGroupName(UIGChooserItem *pRoot)
{
    /* Enumerate all the group names: */
    QStringList groupNames;
    foreach (UIGChooserItem *pItem, pRoot->items(UIGChooserItemType_Group))
        groupNames << pItem->name();

    /* Prepare reg-exp: */
    QString strMinimumName = tr("New group");
    QString strShortTemplate = strMinimumName;
    QString strFullTemplate = strShortTemplate + QString(" (\\d+)");
    QRegExp shortRegExp(strShortTemplate);
    QRegExp fullRegExp(strFullTemplate);

    /* Search for the maximum index: */
    int iMinimumPossibleNumber = 0;
    foreach (const QString &strName, groupNames)
    {
        if (shortRegExp.exactMatch(strName))
            iMinimumPossibleNumber = qMax(iMinimumPossibleNumber, 2);
        else if (fullRegExp.exactMatch(strName))
            iMinimumPossibleNumber = qMax(iMinimumPossibleNumber, fullRegExp.cap(1).toInt() + 1);
    }

    /* Prepare result: */
    QString strResult = strMinimumName;
    if (iMinimumPossibleNumber)
        strResult += " " + QString::number(iMinimumPossibleNumber);
    return strResult;
}

void UIGChooserModel::activateMachineItem()
{
    actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)->activate(QAction::Trigger);
}

void UIGChooserModel::setCurrentDragObject(QDrag *pDragObject)
{
    /* Make sure real focus unset: */
    clearRealFocus();

    /* Remember new drag-object: */
    m_pCurrentDragObject = pDragObject;
    connect(m_pCurrentDragObject, SIGNAL(destroyed(QObject*)), this, SLOT(sltCurrentDragObjectDestroyed()));
}

void UIGChooserModel::lookFor(const QString &strLookupSymbol)
{
    /* Restart timer to reset lookup-string: */
    m_pLookupTimer->start();

    /* Prepare item: */
    UIGChooserItem *pItem = 0;

    /* We are starting to look from the current position: */
    int iCurrentIndex = navigationList().indexOf(currentItem());

    /* Are we looking for the 1. same symbol or for the 2. composed word? */
    const QString strLookupString = m_strLookupString.isEmpty() || m_strLookupString == strLookupSymbol ?
                                    strLookupSymbol : m_strLookupString + strLookupSymbol;
    /* Are we looking from the 1. subsequent position or from the 2. same one? */
    const int     iFirstIndex     = m_strLookupString.isEmpty() || m_strLookupString == strLookupSymbol ?
                                    iCurrentIndex + 1 : iCurrentIndex;

    /* If first position feats the bounds: */
    if (iFirstIndex < navigationList().size())
    {
        /* We have to look starting from the first position: */
        for (int iIndex = iFirstIndex; iIndex < navigationList().size(); ++iIndex)
        {
            UIGChooserItem *pIteratedItem = navigationList().at(iIndex);
            if (pIteratedItem->name().startsWith(strLookupString, Qt::CaseInsensitive))
            {
                pItem = pIteratedItem;
                break;
            }
        }
    }

    /* If the item was not found: */
    if (!pItem && iFirstIndex > 0)
    {
        /* We have to try to look from the beginning of the list: */
        for (int iIndex = 0; iIndex < iFirstIndex; ++iIndex)
        {
            UIGChooserItem *pIteratedItem = navigationList().at(iIndex);
            if (pIteratedItem->name().startsWith(strLookupString, Qt::CaseInsensitive))
            {
                pItem = pIteratedItem;
                break;
            }
        }
    }

    /* If that item was found: */
    if (pItem)
    {
        /* Choose it: */
        pItem->makeSureItsVisible();
        setCurrentItem(pItem);
        /* Append lookup symbol: */
        if (m_strLookupString != strLookupSymbol)
            m_strLookupString += strLookupSymbol;
    }
}

bool UIGChooserModel::isLookupInProgress() const
{
    return m_pLookupTimer->isActive();
}

void UIGChooserModel::saveGroupSettings()
{
    emit sigStartGroupSaving();
}

bool UIGChooserModel::isGroupSavingInProgress() const
{
    return UIGroupDefinitionSaveThread::instance() ||
           UIGroupOrderSaveThread::instance();
}

void UIGChooserModel::sltMachineStateChanged(QString strId, KMachineState)
{
    /* Update machine-items with passed id: */
    mainRoot()->updateAll(strId);
}

void UIGChooserModel::sltMachineDataChanged(QString strId)
{
    /* Update machine-items with passed id: */
    mainRoot()->updateAll(strId);
}

void UIGChooserModel::sltMachineRegistered(QString strId, bool fRegistered)
{
    /* New VM registered? */
    if (fRegistered)
    {
        /* Search for corresponding machine: */
        CMachine machine = vboxGlobal().virtualBox().FindMachine(strId);
        /* Should we show this machine? */
        if (gEDataManager->showMachineInSelectorChooser(strId))
        {
            /* Add new machine-item: */
            addMachineIntoTheTree(machine, true);
            /* And update model: */
            updateNavigation();
            updateLayout();
            /* Change current-item only if VM was created from the GUI side: */
            if (strId == m_strLastCreatedMachineId)
            {
                setCurrentItem(mainRoot()->searchForItem(machine.GetName(),
                                                         UIGChooserItemSearchFlag_Machine |
                                                         UIGChooserItemSearchFlag_ExactName));
            }
        }
    }
    /* Existing VM unregistered? */
    else
    {
        /* Remove machine-items with passed id: */
        mainRoot()->removeAll(strId);
        /* Update model: */
        cleanupGroupTree();
        updateNavigation();
        updateLayout();
        /* Make sure current-item present, if possible: */
        if (!currentItem() && !navigationList().isEmpty())
            setCurrentItem(navigationList().first());
        /* Make sure focus-item present, if possible: */
        else if (!focusItem() && currentItem())
            setFocusItem(currentItem());
        /* Notify about current-item change: */
        notifyCurrentItemChanged();
    }
}

void UIGChooserModel::sltSessionStateChanged(QString strId, KSessionState)
{
    /* Update machine-items with passed id: */
    mainRoot()->updateAll(strId);
}

void UIGChooserModel::sltSnapshotChanged(QString strId, QString)
{
    /* Update machine-items with passed id: */
    mainRoot()->updateAll(strId);
}

void UIGChooserModel::sltHandleViewResized()
{
    /* Relayout: */
    updateLayout();
}

void UIGChooserModel::sltFocusItemDestroyed()
{
    AssertMsgFailed(("Focus item destroyed!"));
}

void UIGChooserModel::sltLeftRootSlidingProgress()
{
    /* Update left root: */
    m_pLeftRoot->updateGeometry();
    m_pLeftRoot->updateLayout();
}

void UIGChooserModel::sltRightRootSlidingProgress()
{
    /* Update right root: */
    m_pRightRoot->updateGeometry();
    m_pRightRoot->updateLayout();
}

void UIGChooserModel::sltSlidingComplete()
{
    /* Delete temporary roots: */
    delete m_pLeftRoot;
    m_pLeftRoot = 0;
    delete m_pRightRoot;
    m_pRightRoot = 0;

    /* We are no more sliding: */
    m_fSliding = false;

    /* Update root geometry: */
    root()->updateGeometry();

    /* Update model: */
    cleanupGroupTree();
    updateNavigation();
    updateLayout();
    if (m_pAfterSlidingFocus)
    {
        setCurrentItem(m_pAfterSlidingFocus);
        m_pAfterSlidingFocus = 0;
    }
    else
    {
        if (!navigationList().isEmpty())
            setCurrentItem(navigationList().first());
        else
            unsetCurrentItem();
    }
}

void UIGChooserModel::sltEditGroupName()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Group_S_Rename)->isEnabled())
        return;

    /* Only for single selected group: */
    if (!isSingleGroupSelected())
        return;

    /* Start editing group name: */
    currentItem()->startEditing();
}

void UIGChooserModel::sltSortGroup()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Group_S_Sort)->isEnabled())
        return;

    /* Only for single selected group: */
    if (!isSingleGroupSelected())
        return;

    /* Sorting group: */
    currentItem()->sortItems();
}

void UIGChooserModel::sltUngroupSelectedGroup()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Group_S_Remove)->isEnabled())
        return;

    /* Make sure focus item is of group type! */
    AssertMsg(focusItem()->type() == UIGChooserItemType_Group, ("This is not group-item!"));

    /* Check if we have collisions with our siblings: */
    UIGChooserItem *pFocusItem = focusItem();
    UIGChooserItem *pParentItem = pFocusItem->parentItem();
    QList<UIGChooserItem*> siblings = pParentItem->items();
    QList<UIGChooserItem*> toBeRenamed;
    QList<UIGChooserItem*> toBeRemoved;
    foreach (UIGChooserItem *pItem, pFocusItem->items())
    {
        QString strItemName = pItem->name();
        UIGChooserItem *pCollisionSibling = 0;
        foreach (UIGChooserItem *pSibling, siblings)
            if (pSibling != pFocusItem && pSibling->name() == strItemName)
                pCollisionSibling = pSibling;
        if (pCollisionSibling)
        {
            if (pItem->type() == UIGChooserItemType_Machine)
            {
                if (pCollisionSibling->type() == UIGChooserItemType_Machine)
                    toBeRemoved << pItem;
                else if (pCollisionSibling->type() == UIGChooserItemType_Group)
                {
                    msgCenter().cannotResolveCollisionAutomatically(strItemName, pParentItem->name());
                    return;
                }
            }
            else if (pItem->type() == UIGChooserItemType_Group)
            {
                if (msgCenter().confirmAutomaticCollisionResolve(strItemName, pParentItem->name()))
                    toBeRenamed << pItem;
                else
                    return;
            }
        }
    }

    /* Copy all the children into our parent: */
    foreach (UIGChooserItem *pItem, pFocusItem->items())
    {
        if (toBeRemoved.contains(pItem))
            continue;
        switch (pItem->type())
        {
            case UIGChooserItemType_Group:
            {
                UIGChooserItemGroup *pGroupItem = new UIGChooserItemGroup(pParentItem, pItem->toGroupItem());
                if (toBeRenamed.contains(pItem))
                    pGroupItem->setName(uniqueGroupName(pParentItem));
                break;
            }
            case UIGChooserItemType_Machine:
            {
                new UIGChooserItemMachine(pParentItem, pItem->toMachineItem());
                break;
            }
        }
    }

    /* Delete focus group: */
    delete focusItem();

    /* And update model: */
    updateNavigation();
    updateLayout();
    setCurrentItem(navigationList().first());
    saveGroupSettings();
}

void UIGChooserModel::sltCreateNewMachine()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Machine_S_New)->isEnabled())
        return;

    /* Choose the parent: */
    UIGChooserItem *pGroup = 0;
    if (isSingleGroupSelected())
        pGroup = currentItem();
    else if (!currentItems().isEmpty())
        pGroup = currentItem()->parentItem();
    QString strGroupName;
    if (pGroup)
        strGroupName = pGroup->fullName();

    /* Lock the action preventing cascade calls: */
    actionPool()->action(UIActionIndexST_M_Machine_S_New)->setEnabled(false);
    actionPool()->action(UIActionIndexST_M_Group_S_New)->setEnabled(false);

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pWizardParent = windowManager().realParentWindow(m_pChooser->selector());
    UISafePointerWizardNewVM pWizard = new UIWizardNewVM(pWizardParent, strGroupName);
    windowManager().registerNewParent(pWizard, pWizardParent);
    pWizard->prepare();

    /* Execute wizard and store created VM Id
     * on success for current-item handling: */
    if (pWizard->exec() == QDialog::Accepted)
        m_strLastCreatedMachineId = pWizard->createdMachineId();

    if (pWizard)
        delete pWizard;

    /* Unlock the action allowing further calls: */
    actionPool()->action(UIActionIndexST_M_Machine_S_New)->setEnabled(true);
    actionPool()->action(UIActionIndexST_M_Group_S_New)->setEnabled(true);
}

void UIGChooserModel::sltGroupSelectedMachines()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Machine_S_AddGroup)->isEnabled())
        return;

    /* Create new group in the current root: */
    UIGChooserItemGroup *pNewGroupItem = new UIGChooserItemGroup(root(), uniqueGroupName(root()), true);
    /* Enumerate all the currently chosen items: */
    QStringList busyGroupNames;
    QStringList busyMachineNames;
    QList<UIGChooserItem*> selectedItems = currentItems();
    foreach (UIGChooserItem *pItem, selectedItems)
    {
        /* For each of known types: */
        switch (pItem->type())
        {
            case UIGChooserItemType_Group:
            {
                /* Avoid name collisions: */
                if (busyGroupNames.contains(pItem->name()))
                    break;
                /* Add name to busy: */
                busyGroupNames << pItem->name();
                /* Copy or move group-item: */
                new UIGChooserItemGroup(pNewGroupItem, pItem->toGroupItem());
                delete pItem;
                break;
            }
            case UIGChooserItemType_Machine:
            {
                /* Avoid name collisions: */
                if (busyMachineNames.contains(pItem->name()))
                    break;
                /* Add name to busy: */
                busyMachineNames << pItem->name();
                /* Copy or move machine item: */
                new UIGChooserItemMachine(pNewGroupItem, pItem->toMachineItem());
                delete pItem;
                break;
            }
        }
    }
    /* Update model: */
    cleanupGroupTree();
    updateNavigation();
    updateLayout();
    setCurrentItem(pNewGroupItem);
    saveGroupSettings();
}

void UIGChooserModel::sltReloadMachine(const QString &strId)
{
    /* Remove all the items first: */
    mainRoot()->removeAll(strId);
    /* Wipe out empty groups: */
    cleanupGroupTree();

    /* Show machine if we should: */
    CMachine machine = vboxGlobal().virtualBox().FindMachine(strId);
    if (gEDataManager->showMachineInSelectorChooser(strId))
        addMachineIntoTheTree(machine);

    /* And update model: */
    updateNavigation();
    updateLayout();

    /* Make sure at least one item selected after that: */
    if (!currentItem() && !navigationList().isEmpty())
        setCurrentItem(navigationList().first());

    /* Notify listeners about selection change: */
    emit sigSelectionChanged();
}

void UIGChooserModel::sltSortParentGroup()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Machine_S_SortParent)->isEnabled())
        return;

    /* Only if some item selected: */
    if (!currentItem())
        return;

    /* Sorting parent group: */
    currentItem()->parentItem()->sortItems();
}

void UIGChooserModel::sltPerformRefreshAction()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Group_S_Refresh)->isEnabled())
        return;

    /* Gather list of current unique inaccessible machine-items: */
    QList<UIGChooserItemMachine*> inaccessibleMachineItemList;
    UIGChooserItemMachine::enumerateMachineItems(currentItems(), inaccessibleMachineItemList,
                                                 UIGChooserItemMachineEnumerationFlag_Unique |
                                                 UIGChooserItemMachineEnumerationFlag_Inaccessible);

    /* For each machine-item: */
    UIGChooserItem *pSelectedItem = 0;
    foreach (UIGChooserItemMachine *pItem, inaccessibleMachineItemList)
    {
        /* Recache: */
        pItem->recache();
        /* Become accessible? */
        if (pItem->accessible())
        {
            /* Machine name: */
            QString strMachineName = pItem->name();
            /* We should reload this machine: */
            sltReloadMachine(pItem->id());
            /* Select first of reloaded items: */
            if (!pSelectedItem)
                pSelectedItem = mainRoot()->searchForItem(strMachineName,
                                                          UIGChooserItemSearchFlag_Machine |
                                                          UIGChooserItemSearchFlag_ExactName);
        }
    }

    /* Some item to be selected? */
    if (pSelectedItem)
    {
        pSelectedItem->makeSureItsVisible();
        setCurrentItem(pSelectedItem);
    }
}

void UIGChooserModel::sltRemoveSelectedMachine()
{
    /* Check if action is enabled: */
    if (!actionPool()->action(UIActionIndexST_M_Machine_S_Remove)->isEnabled())
        return;

    /* Enumerate all the selected machine-items: */
    QList<UIGChooserItemMachine*> selectedMachineItemList;
    UIGChooserItemMachine::enumerateMachineItems(currentItems(), selectedMachineItemList);
    /* Enumerate all the existing machine-items: */
    QList<UIGChooserItemMachine*> existingMachineItemList;
    UIGChooserItemMachine::enumerateMachineItems(mainRoot()->items(), existingMachineItemList);

    /* Prepare arrays: */
    QMap<QString, bool> verdicts;
    QList<UIGChooserItem*> itemsToRemove;
    QStringList machinesToUnregister;

    /* For each selected machine-item: */
    foreach (UIGChooserItem *pItem, selectedMachineItemList)
    {
        /* Get machine-item id: */
        QString strId = pItem->toMachineItem()->id();

        /* We already decided for that machine? */
        if (verdicts.contains(strId))
        {
            /* To remove similar machine items? */
            if (!verdicts[strId])
                itemsToRemove << pItem;
            continue;
        }

        /* Selected copy count: */
        int iSelectedCopyCount = 0;
        foreach (UIGChooserItem *pSelectedItem, selectedMachineItemList)
            if (pSelectedItem->toMachineItem()->id() == strId)
                ++iSelectedCopyCount;
        /* Existing copy count: */
        int iExistingCopyCount = 0;
        foreach (UIGChooserItem *pExistingItem, existingMachineItemList)
            if (pExistingItem->toMachineItem()->id() == strId)
                ++iExistingCopyCount;
        /* If selected copy count equal to existing copy count,
         * we will propose ro unregister machine fully else
         * we will just propose to remove selected items: */
        bool fVerdict = iSelectedCopyCount == iExistingCopyCount;
        verdicts.insert(strId, fVerdict);
        if (fVerdict)
            machinesToUnregister << strId;
        else
            itemsToRemove << pItem;
    }

    /* If we have something to remove: */
    if (!itemsToRemove.isEmpty())
        removeItems(itemsToRemove);
    /* If we have something to unregister: */
    if (!machinesToUnregister.isEmpty())
        unregisterMachines(machinesToUnregister);
}

void UIGChooserModel::sltStartScrolling()
{
    /* Should we scroll? */
    if (!m_fIsScrollingInProgress)
        return;

    /* Reset scrolling progress: */
    m_fIsScrollingInProgress = false;

    /* Get view/scrollbar: */
    QGraphicsView *pView = scene()->views()[0];
    QScrollBar *pVerticalScrollBar = pView->verticalScrollBar();

    /* Convert mouse position to view co-ordinates: */
    QPoint mousePos = pView->mapFromGlobal(QCursor::pos());
    /* Mouse position is at the top of view? */
    if (mousePos.y() < m_iScrollingTokenSize && mousePos.y() > 0)
    {
        int iValue = mousePos.y();
        if (!iValue) iValue = 1;
        int iDelta = m_iScrollingTokenSize / iValue;
        if (pVerticalScrollBar->value() > pVerticalScrollBar->minimum())
        {
            /* Backward scrolling: */
            pVerticalScrollBar->setValue(pVerticalScrollBar->value() - 2 * iDelta);
            m_fIsScrollingInProgress = true;
            QTimer::singleShot(10, this, SLOT(sltStartScrolling()));
        }
    }
    /* Mouse position is at the bottom of view? */
    else if (mousePos.y() > pView->height() - m_iScrollingTokenSize && mousePos.y() < pView->height())
    {
        int iValue = pView->height() - mousePos.y();
        if (!iValue) iValue = 1;
        int iDelta = m_iScrollingTokenSize / iValue;
        if (pVerticalScrollBar->value() < pVerticalScrollBar->maximum())
        {
            /* Forward scrolling: */
            pVerticalScrollBar->setValue(pVerticalScrollBar->value() + 2 * iDelta);
            m_fIsScrollingInProgress = true;
            QTimer::singleShot(10, this, SLOT(sltStartScrolling()));
        }
    }
}

void UIGChooserModel::sltCurrentDragObjectDestroyed()
{
    root()->resetDragToken();
}

void UIGChooserModel::sltEraseLookupTimer()
{
    m_pLookupTimer->stop();
    m_strLookupString = QString();
}

void UIGChooserModel::sltGroupSavingStart()
{
    saveGroupDefinitions();
    saveGroupOrders();
}

void UIGChooserModel::sltGroupDefinitionsSaveComplete()
{
    makeSureGroupDefinitionsSaveIsFinished();
    emit sigGroupSavingStateChanged();
}

void UIGChooserModel::sltGroupOrdersSaveComplete()
{
    makeSureGroupOrdersSaveIsFinished();
    emit sigGroupSavingStateChanged();
}

QVariant UIGChooserModel::data(int iKey) const
{
    switch (iKey)
    {
        case ChooserModelData_Margin: return 0;
        default: break;
    }
    return QVariant();
}

void UIGChooserModel::prepareScene()
{
    m_pScene = new QGraphicsScene(this);
    m_pScene->installEventFilter(this);
}

void UIGChooserModel::prepareRoot()
{
    m_rootStack << new UIGChooserItemGroup(scene());
}

void UIGChooserModel::prepareLookup()
{
    m_pLookupTimer = new QTimer(this);
    m_pLookupTimer->setInterval(1000);
    m_pLookupTimer->setSingleShot(true);
    connect(m_pLookupTimer, SIGNAL(timeout()), this, SLOT(sltEraseLookupTimer()));
}

void UIGChooserModel::prepareContextMenu()
{
    /* Context menu for group(s): */
    m_pContextMenuGroup = new QMenu;
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_New));
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Add));
    m_pContextMenuGroup->addSeparator();
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Rename));
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Remove));
    m_pContextMenuGroup->addSeparator();
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow));
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_T_Pause));
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Reset));
    m_pContextMenuGroup->addMenu(actionPool()->action(UIActionIndexST_M_Group_M_Close)->menu());
    m_pContextMenuGroup->addSeparator();
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Discard));
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_ShowLogDialog));
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Refresh));
    m_pContextMenuGroup->addSeparator();
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_ShowInFileManager));
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_CreateShortcut));
    m_pContextMenuGroup->addSeparator();
    m_pContextMenuGroup->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Sort));

    /* Context menu for machine(s): */
    m_pContextMenuMachine = new QMenu;
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Settings));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Clone));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Remove));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_AddGroup));
    m_pContextMenuMachine->addSeparator();
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_T_Pause));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Reset));
    m_pContextMenuMachine->addMenu(actionPool()->action(UIActionIndexST_M_Machine_M_Close)->menu());
    m_pContextMenuMachine->addSeparator();
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Discard));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_ShowLogDialog));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Refresh));
    m_pContextMenuMachine->addSeparator();
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_ShowInFileManager));
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_CreateShortcut));
    m_pContextMenuMachine->addSeparator();
    m_pContextMenuMachine->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_SortParent));

    connect(actionPool()->action(UIActionIndexST_M_Group_S_New), SIGNAL(triggered()),
            this, SLOT(sltCreateNewMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_New), SIGNAL(triggered()),
            this, SLOT(sltCreateNewMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_Rename), SIGNAL(triggered()),
            this, SLOT(sltEditGroupName()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_Remove), SIGNAL(triggered()),
            this, SLOT(sltUngroupSelectedGroup()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_Remove), SIGNAL(triggered()),
            this, SLOT(sltRemoveSelectedMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_AddGroup), SIGNAL(triggered()),
            this, SLOT(sltGroupSelectedMachines()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_Refresh), SIGNAL(triggered()),
            this, SLOT(sltPerformRefreshAction()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_Refresh), SIGNAL(triggered()),
            this, SLOT(sltPerformRefreshAction()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_SortParent), SIGNAL(triggered()),
            this, SLOT(sltSortParentGroup()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_Sort), SIGNAL(triggered()),
            this, SLOT(sltSortGroup()));

    connect(this, SIGNAL(sigStartGroupSaving()), this, SLOT(sltGroupSavingStart()), Qt::QueuedConnection);
}

void UIGChooserModel::prepareHandlers()
{
    m_pMouseHandler = new UIGChooserHandlerMouse(this);
    m_pKeyboardHandler = new UIGChooserHandlerKeyboard(this);
}

void UIGChooserModel::prepareConnections()
{
    /* Setup parent connections: */
    connect(this, SIGNAL(sigSelectionChanged()),
            parent(), SIGNAL(sigSelectionChanged()));
    connect(this, SIGNAL(sigSlidingStarted()),
            parent(), SIGNAL(sigSlidingStarted()));
    connect(this, SIGNAL(sigToggleStarted()),
            parent(), SIGNAL(sigToggleStarted()));
    connect(this, SIGNAL(sigToggleFinished()),
            parent(), SIGNAL(sigToggleFinished()));
    connect(this, SIGNAL(sigGroupSavingStateChanged()),
            parent(), SIGNAL(sigGroupSavingStateChanged()));

    /* Setup global connections: */
    connect(gVBoxEvents, SIGNAL(sigMachineStateChange(QString, KMachineState)),
            this, SLOT(sltMachineStateChanged(QString, KMachineState)));
    connect(gVBoxEvents, SIGNAL(sigMachineDataChange(QString)),
            this, SLOT(sltMachineDataChanged(QString)));
    connect(gVBoxEvents, SIGNAL(sigMachineRegistered(QString, bool)),
            this, SLOT(sltMachineRegistered(QString, bool)));
    connect(gVBoxEvents, SIGNAL(sigSessionStateChange(QString, KSessionState)),
            this, SLOT(sltSessionStateChanged(QString, KSessionState)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotTake(QString, QString)),
            this, SLOT(sltSnapshotChanged(QString, QString)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotDelete(QString, QString)),
            this, SLOT(sltSnapshotChanged(QString, QString)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotChange(QString, QString)),
            this, SLOT(sltSnapshotChanged(QString, QString)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotRestore(QString, QString)),
            this, SLOT(sltSnapshotChanged(QString, QString)));
}

void UIGChooserModel::loadLastSelectedItem()
{
    /* Load last selected item (choose first if unable to load): */
    setCurrentItem(gEDataManager->selectorWindowLastItemChosen());
    if (!currentItem() && !navigationList().isEmpty())
        setCurrentItem(navigationList().first());
}

void UIGChooserModel::saveLastSelectedItem()
{
    /* Save last selected item: */
    gEDataManager->setSelectorWindowLastItemChosen(currentItem() ? currentItem()->definition() : QString());
}

void UIGChooserModel::cleanupHandlers()
{
    delete m_pKeyboardHandler;
    m_pKeyboardHandler = 0;
    delete m_pMouseHandler;
    m_pMouseHandler = 0;
}

void UIGChooserModel::cleanupContextMenu()
{
    delete m_pContextMenuGroup;
    m_pContextMenuGroup = 0;
    delete m_pContextMenuMachine;
    m_pContextMenuMachine = 0;
}

void UIGChooserModel::cleanupLookup()
{
    delete m_pLookupTimer;
    m_pLookupTimer = 0;
}

void UIGChooserModel::cleanupRoot()
{
    delete mainRoot();
    m_rootStack.clear();
}

void UIGChooserModel::cleanupScene()
{
    delete m_pScene;
    m_pScene = 0;
}

bool UIGChooserModel::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* Process only scene events: */
    if (pWatched != m_pScene)
        return QObject::eventFilter(pWatched, pEvent);

    /* Process only item focused by model: */
    if (scene()->focusItem())
        return QObject::eventFilter(pWatched, pEvent);

    /* Checking event-type: */
    switch (pEvent->type())
    {
        /* Keyboard handler: */
        case QEvent::KeyPress:
            return m_pKeyboardHandler->handle(static_cast<QKeyEvent*>(pEvent), UIKeyboardEventType_Press);
        case QEvent::KeyRelease:
            return m_pKeyboardHandler->handle(static_cast<QKeyEvent*>(pEvent), UIKeyboardEventType_Release);
        /* Mouse handler: */
        case QEvent::GraphicsSceneMousePress:
            return m_pMouseHandler->handle(static_cast<QGraphicsSceneMouseEvent*>(pEvent), UIMouseEventType_Press);
        case QEvent::GraphicsSceneMouseRelease:
            return m_pMouseHandler->handle(static_cast<QGraphicsSceneMouseEvent*>(pEvent), UIMouseEventType_Release);
        case QEvent::GraphicsSceneMouseDoubleClick:
            return m_pMouseHandler->handle(static_cast<QGraphicsSceneMouseEvent*>(pEvent), UIMouseEventType_DoubleClick);
        /* Context-menu handler: */
        case QEvent::GraphicsSceneContextMenu:
            return processContextMenuEvent(static_cast<QGraphicsSceneContextMenuEvent*>(pEvent));
        /* Drag&drop scroll-event (drag-move) handler: */
        case QEvent::GraphicsSceneDragMove:
            return processDragMoveEvent(static_cast<QGraphicsSceneDragDropEvent*>(pEvent));
        /* Drag&drop scroll-event (drag-leave) handler: */
        case QEvent::GraphicsSceneDragLeave:
            return processDragLeaveEvent(static_cast<QGraphicsSceneDragDropEvent*>(pEvent));
        default: break; /* Shut up MSC */
    }

    /* Call to base-class: */
    return QObject::eventFilter(pWatched, pEvent);
}

QList<UIGChooserItem*> UIGChooserModel::createNavigationList(UIGChooserItem *pItem)
{
    /* Prepare navigation list: */
    QList<UIGChooserItem*> navigationItems;

    /* Iterate over all the group-items: */
    foreach (UIGChooserItem *pGroupItem, pItem->items(UIGChooserItemType_Group))
    {
        navigationItems << pGroupItem;
        if (pGroupItem->toGroupItem()->isOpened())
            navigationItems << createNavigationList(pGroupItem);
    }
    /* Iterate over all the machine-items: */
    foreach (UIGChooserItem *pMachineItem, pItem->items(UIGChooserItemType_Machine))
        navigationItems << pMachineItem;

    /* Return navigation list: */
    return navigationItems;
}

void UIGChooserModel::clearRealFocus()
{
    /* Set the real focus to null: */
    scene()->setFocusItem(0);
}

void UIGChooserModel::slideRoot(bool fForward)
{
    /* Animation group: */
    QParallelAnimationGroup *pAnimation = new QParallelAnimationGroup(this);
    connect(pAnimation, SIGNAL(finished()), this, SLOT(sltSlidingComplete()), Qt::QueuedConnection);

    /* Left root animation: */
    {
        QPropertyAnimation *pLeftAnimation = new QPropertyAnimation(m_pLeftRoot, "geometry", this);
        connect(pLeftAnimation, SIGNAL(valueChanged(const QVariant&)), this, SLOT(sltLeftRootSlidingProgress()));
        QRectF startGeo = m_pLeftRoot->geometry();
        QRectF endGeo = fForward ? startGeo.translated(- startGeo.width(), 0) :
                                   startGeo.translated(startGeo.width(), 0);
        pLeftAnimation->setEasingCurve(QEasingCurve::InCubic);
        pLeftAnimation->setDuration(500);
        pLeftAnimation->setStartValue(startGeo);
        pLeftAnimation->setEndValue(endGeo);
        pAnimation->addAnimation(pLeftAnimation);
    }

    /* Right root animation: */
    {
        QPropertyAnimation *pRightAnimation = new QPropertyAnimation(m_pRightRoot, "geometry", this);
        connect(pRightAnimation, SIGNAL(valueChanged(const QVariant&)), this, SLOT(sltRightRootSlidingProgress()));
        QRectF startGeo = m_pRightRoot->geometry();
        QRectF endGeo = fForward ? startGeo.translated(- startGeo.width(), 0) :
                                   startGeo.translated(startGeo.width(), 0);
        pRightAnimation->setEasingCurve(QEasingCurve::InCubic);
        pRightAnimation->setDuration(500);
        pRightAnimation->setStartValue(startGeo);
        pRightAnimation->setEndValue(endGeo);
        pAnimation->addAnimation(pRightAnimation);
    }

    /* Start animation: */
    pAnimation->start();
}

void UIGChooserModel::cleanupGroupTree(UIGChooserItem *pParent)
{
    /* Cleanup all the group-items recursively first: */
    foreach (UIGChooserItem *pItem, pParent->items(UIGChooserItemType_Group))
        cleanupGroupTree(pItem);
    /* If parent has no items: */
    if (!pParent->hasItems())
    {
        /* Cleanup if that is non-root item: */
        if (!pParent->isRoot())
            delete pParent;
        /* Unindent if that is root item: */
        else if (root() != mainRoot())
            unindentRoot();
    }
}

void UIGChooserModel::removeItems(const QList<UIGChooserItem*> &itemsToRemove)
{
    /* Confirm machine-items removal: */
    QStringList names;
    foreach (UIGChooserItem *pItem, itemsToRemove)
        names << pItem->name();
    if (!msgCenter().confirmMachineItemRemoval(names))
        return;

    /* Remove all the passed items: */
    foreach (UIGChooserItem *pItem, itemsToRemove)
        delete pItem;

    /* And update model: */
    cleanupGroupTree();
    updateNavigation();
    updateLayout();
    if (!navigationList().isEmpty())
        setCurrentItem(navigationList().first());
    else
        unsetCurrentItem();
    saveGroupSettings();
}

void UIGChooserModel::unregisterMachines(const QStringList &ids)
{
    /* Populate machine list: */
    QList<CMachine> machines;
    CVirtualBox vbox = vboxGlobal().virtualBox();
    foreach (const QString &strId, ids)
    {
        CMachine machine = vbox.FindMachine(strId);
        if (!machine.isNull())
            machines << machine;
    }

    /* Confirm machine removal: */
    int iResultCode = msgCenter().confirmMachineRemoval(machines);
    if (iResultCode == AlertButton_Cancel)
        return;

    /* Unset current item(s): */
    unsetCurrentItem();

    /* For every selected item: */
    for (int iMachineIndex = 0; iMachineIndex < machines.size(); ++iMachineIndex)
    {
        /* Get iterated machine: */
        CMachine &machine = machines[iMachineIndex];
        if (iResultCode == AlertButton_Choice1)
        {
            /* Unregister machine first: */
            CMediumVector mediums = machine.Unregister(KCleanupMode_DetachAllReturnHardDisksOnly);
            if (!machine.isOk())
            {
                msgCenter().cannotRemoveMachine(machine);
                continue;
            }
            /* Prepare cleanup progress: */
            CProgress progress = machine.DeleteConfig(mediums);
            if (!machine.isOk())
            {
                msgCenter().cannotRemoveMachine(machine);
                continue;
            }
            /* And show cleanup progress finally: */
            msgCenter().showModalProgressDialog(progress, machine.GetName(), ":/progress_delete_90px.png");
            if (!progress.isOk() || progress.GetResultCode() != 0)
            {
                msgCenter().cannotRemoveMachine(machine, progress);
                continue;
            }
        }
        else if (iResultCode == AlertButton_Choice2 || iResultCode == AlertButton_Ok)
        {
            /* Unregister machine first: */
            CMediumVector mediums = machine.Unregister(KCleanupMode_DetachAllReturnHardDisksOnly);
            if (!machine.isOk())
            {
                msgCenter().cannotRemoveMachine(machine);
                continue;
            }
            /* Finally close all media, deliberately ignoring errors: */
            foreach (CMedium medium, mediums)
            {
                if (!medium.isNull())
                    medium.Close();
            }
        }
    }
}

bool UIGChooserModel::processContextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent)
{
    /* Whats the reason? */
    switch (pEvent->reason())
    {
        case QGraphicsSceneContextMenuEvent::Mouse:
        {
            /* First of all we should look for an item under cursor: */
            if (QGraphicsItem *pItem = itemAt(pEvent->scenePos()))
            {
                /* If this item of known type? */
                switch (pItem->type())
                {
                    case UIGChooserItemType_Group:
                    {
                        /* Get group-item: */
                        UIGChooserItem *pGroupItem = qgraphicsitem_cast<UIGChooserItemGroup*>(pItem);
                        /* Make sure thats not root: */
                        if (pGroupItem->isRoot())
                            return false;
                        /* Is this group-item only the one selected? */
                        if (currentItems().contains(pGroupItem) && currentItems().size() == 1)
                        {
                            /* Group context menu in that case: */
                            popupContextMenu(UIGraphicsSelectorContextMenuType_Group, pEvent->screenPos());
                            return true;
                        }
                    }
                    RT_FALL_THRU();
                    case UIGChooserItemType_Machine:
                    {
                        /* Machine context menu for other Group/Machine cases: */
                        popupContextMenu(UIGraphicsSelectorContextMenuType_Machine, pEvent->screenPos());
                        return true;
                    }
                    default:
                        break;
                }
            }
            return true;
        }
        case QGraphicsSceneContextMenuEvent::Keyboard:
        {
            /* Get first selected item: */
            if (UIGChooserItem *pItem = currentItem())
            {
                /* If this item of known type? */
                switch (pItem->type())
                {
                    case UIGChooserItemType_Group:
                    {
                        /* Is this group-item only the one selected? */
                        if (currentItems().size() == 1)
                        {
                            /* Group context menu in that case: */
                            popupContextMenu(UIGraphicsSelectorContextMenuType_Group, pEvent->screenPos());
                            return true;
                        }
                    }
                    RT_FALL_THRU();
                    case UIGChooserItemType_Machine:
                    {
                        /* Machine context menu for other Group/Machine cases: */
                        popupContextMenu(UIGraphicsSelectorContextMenuType_Machine, pEvent->screenPos());
                        return true;
                    }
                    default:
                        break;
                }
            }
            return true;
        }
        default:
            break;
    }
    /* Pass others context menu events: */
    return false;
}

void UIGChooserModel::popupContextMenu(UIGraphicsSelectorContextMenuType type, QPoint point)
{
    /* Which type of context-menu requested? */
    switch (type)
    {
        /* For group? */
        case UIGraphicsSelectorContextMenuType_Group:
        {
            m_pContextMenuGroup->exec(point);
            break;
        }
        /* For machine(s)? */
        case UIGraphicsSelectorContextMenuType_Machine:
        {
            m_pContextMenuMachine->exec(point);
            break;
        }
    }
}

bool UIGChooserModel::processDragMoveEvent(QGraphicsSceneDragDropEvent *pEvent)
{
    /* Do we scrolling already? */
    if (m_fIsScrollingInProgress)
        return false;

    /* Get view: */
    QGraphicsView *pView = scene()->views()[0];

    /* Check scroll-area: */
    QPoint eventPoint = pView->mapFromGlobal(pEvent->screenPos());
    if ((eventPoint.y() < m_iScrollingTokenSize) ||
        (eventPoint.y() > pView->height() - m_iScrollingTokenSize))
    {
        /* Set scrolling in progress: */
        m_fIsScrollingInProgress = true;
        /* Start scrolling: */
        QTimer::singleShot(200, this, SLOT(sltStartScrolling()));
    }

    /* Pass event: */
    return false;
}

bool UIGChooserModel::processDragLeaveEvent(QGraphicsSceneDragDropEvent *pEvent)
{
    /* Event object is not required here: */
    Q_UNUSED(pEvent);

    /* Make sure to stop scrolling as drag-leave event happened: */
    if (m_fIsScrollingInProgress)
        m_fIsScrollingInProgress = false;

    /* Pass event: */
    return false;
}

void UIGChooserModel::loadGroupTree()
{
    /* Add all the approved machines we have into the group-tree: */
    LogRelFlow(("UIGChooserModel: Loading VMs...\n"));
    foreach (CMachine machine, vboxGlobal().virtualBox().GetMachines())
    {
        const QString strMachineID = machine.GetId();
        if (!strMachineID.isEmpty() && gEDataManager->showMachineInSelectorChooser(strMachineID))
            addMachineIntoTheTree(machine);
    }
    LogRelFlow(("UIGChooserModel: VMs loaded.\n"));
}

void UIGChooserModel::addMachineIntoTheTree(const CMachine &machine, bool fMakeItVisible /* = false */)
{
    /* Make sure passed VM is not NULL: */
    if (machine.isNull())
        LogRelFlow(("UIGChooserModel: ERROR: Passed VM is NULL!\n"));
    AssertReturnVoid(!machine.isNull());

    /* Which VM we are loading: */
    LogRelFlow(("UIGChooserModel: Loading VM with ID={%s}...\n", machine.GetId().toUtf8().constData()));
    /* Is that machine accessible? */
    if (machine.GetAccessible())
    {
        /* VM is accessible: */
        QString strName = machine.GetName();
        LogRelFlow(("UIGChooserModel:  VM {%s} is accessible.\n", strName.toUtf8().constData()));
        /* Which groups passed machine attached to? */
        QVector<QString> groups = machine.GetGroups();
        QStringList groupList = groups.toList();
        QString strGroups = groupList.join(", ");
        LogRelFlow(("UIGChooserModel:  VM {%s} has groups: {%s}.\n", strName.toUtf8().constData(),
                                                                     strGroups.toUtf8().constData()));
        foreach (QString strGroup, groups)
        {
            /* Remove last '/' if any: */
            if (strGroup.right(1) == "/")
                strGroup.truncate(strGroup.size() - 1);
            /* Create machine-item with found group-item as parent: */
            LogRelFlow(("UIGChooserModel:   Creating item for VM {%s} in group {%s}.\n", strName.toUtf8().constData(),
                                                                                         strGroup.toUtf8().constData()));
            createMachineItem(machine, getGroupItem(strGroup, mainRoot(), fMakeItVisible));
        }
        /* Update group definitions: */
        m_groups[machine.GetId()] = groupList;
    }
    /* Inaccessible machine: */
    else
    {
        /* VM is accessible: */
        LogRelFlow(("UIGChooserModel:  VM {%s} is inaccessible.\n", machine.GetId().toUtf8().constData()));
        /* Create machine-item with main-root group-item as parent: */
        createMachineItem(machine, mainRoot());
    }
}

UIGChooserItem* UIGChooserModel::getGroupItem(const QString &strName, UIGChooserItem *pParentItem, bool fAllGroupsOpened)
{
    /* Check passed stuff: */
    if (pParentItem->name() == strName)
        return pParentItem;

    /* Prepare variables: */
    QString strFirstSubName = strName.section('/', 0, 0);
    QString strFirstSuffix = strName.section('/', 1, -1);
    QString strSecondSubName = strFirstSuffix.section('/', 0, 0);
    QString strSecondSuffix = strFirstSuffix.section('/', 1, -1);

    /* Passed group name equal to first sub-name: */
    if (pParentItem->name() == strFirstSubName)
    {
        /* Make sure first-suffix is NOT empty: */
        AssertMsg(!strFirstSuffix.isEmpty(), ("Invalid group name!"));
        /* Trying to get group-item among our children: */
        foreach (UIGChooserItem *pGroupItem, pParentItem->items(UIGChooserItemType_Group))
        {
            if (pGroupItem->name() == strSecondSubName)
            {
                UIGChooserItem *pFoundItem = getGroupItem(strFirstSuffix, pGroupItem, fAllGroupsOpened);
                if (UIGChooserItemGroup *pFoundGroupItem = pFoundItem->toGroupItem())
                    if (fAllGroupsOpened && pFoundGroupItem->isClosed())
                        pFoundGroupItem->open(false);
                return pFoundItem;
            }
        }
    }

    /* Found nothing? Creating: */
    UIGChooserItemGroup *pNewGroupItem =
            new UIGChooserItemGroup(/* Parent item and desired group name: */
                                    pParentItem, strSecondSubName,
                                    /* Should be new group opened when created? */
                                    fAllGroupsOpened || shouldBeGroupOpened(pParentItem, strSecondSubName),
                                    /* Which position new group-item should be placed in? */
                                    getDesiredPosition(pParentItem, UIGChooserItemType_Group, strSecondSubName));
    return strSecondSuffix.isEmpty() ? pNewGroupItem : getGroupItem(strFirstSuffix, pNewGroupItem, fAllGroupsOpened);
}

bool UIGChooserModel::shouldBeGroupOpened(UIGChooserItem *pParentItem, const QString &strName)
{
    /* Read group definitions: */
    const QStringList definitions = gEDataManager->selectorWindowGroupsDefinitions(pParentItem->fullName());
    /* Return 'false' if no definitions found: */
    if (definitions.isEmpty())
        return false;

    /* Prepare required group definition reg-exp: */
    QString strDefinitionTemplate = QString("g(\\S)*=%1").arg(strName);
    QRegExp definitionRegExp(strDefinitionTemplate);
    /* For each the group definition: */
    foreach (const QString &strDefinition, definitions)
    {
        /* Check if this is required definition: */
        if (definitionRegExp.indexIn(strDefinition) == 0)
        {
            /* Get group descriptor: */
            QString strDescriptor(definitionRegExp.cap(1));
            if (strDescriptor.contains('o'))
                return true;
        }
    }

    /* Return 'false' by default: */
    return false;
}

int UIGChooserModel::getDesiredPosition(UIGChooserItem *pParentItem, UIGChooserItemType type, const QString &strName)
{
    /* End of list (by default)? */
    int iNewItemDesiredPosition = -1;
    /* Which position should be new item placed by definitions: */
    int iNewItemDefinitionPosition = positionFromDefinitions(pParentItem, type, strName);
    /* If some position wanted: */
    if (iNewItemDefinitionPosition != -1)
    {
        /* Start of list if some definition present: */
        iNewItemDesiredPosition = 0;
        /* We have to check all the existing item positions: */
        QList<UIGChooserItem*> items = pParentItem->items(type);
        for (int i = items.size() - 1; i >= 0; --i)
        {
            /* Get current item: */
            UIGChooserItem *pItem = items[i];
            /* Which position should be current item placed by definitions? */
            QString strDefinitionName = pItem->type() == UIGChooserItemType_Group ? pItem->name() :
                                        pItem->type() == UIGChooserItemType_Machine ? pItem->toMachineItem()->id() :
                                        QString();
            AssertMsg(!strDefinitionName.isEmpty(), ("Wrong definition name!"));
            int iItemDefinitionPosition = positionFromDefinitions(pParentItem, type, strDefinitionName);
            /* If some position wanted: */
            if (iItemDefinitionPosition != -1)
            {
                AssertMsg(iItemDefinitionPosition != iNewItemDefinitionPosition, ("Incorrect definitions!"));
                if (iItemDefinitionPosition < iNewItemDefinitionPosition)
                {
                    iNewItemDesiredPosition = i + 1;
                    break;
                }
            }
        }
    }
    /* Return desired item position: */
    return iNewItemDesiredPosition;
}

int UIGChooserModel::positionFromDefinitions(UIGChooserItem *pParentItem, UIGChooserItemType type, const QString &strName)
{
    /* Read group definitions: */
    const QStringList definitions = gEDataManager->selectorWindowGroupsDefinitions(pParentItem->fullName());
    /* Return 'false' if no definitions found: */
    if (definitions.isEmpty())
        return -1;

    /* Prepare definition reg-exp: */
    QString strDefinitionTemplateShort;
    QString strDefinitionTemplateFull;
    switch (type)
    {
        case UIGChooserItemType_Group:
            strDefinitionTemplateShort = QString("^g(\\S)*=");
            strDefinitionTemplateFull = QString("^g(\\S)*=%1$").arg(strName);
            break;
        case UIGChooserItemType_Machine:
            strDefinitionTemplateShort = QString("^m=");
            strDefinitionTemplateFull = QString("^m=%1$").arg(strName);
            break;
        default: return -1;
    }
    QRegExp definitionRegExpShort(strDefinitionTemplateShort);
    QRegExp definitionRegExpFull(strDefinitionTemplateFull);

    /* For each the definition: */
    int iDefinitionIndex = -1;
    foreach (const QString &strDefinition, definitions)
    {
        /* Check if this definition is of required type: */
        if (definitionRegExpShort.indexIn(strDefinition) == 0)
        {
            ++iDefinitionIndex;
            /* Check if this definition is exactly what we need: */
            if (definitionRegExpFull.indexIn(strDefinition) == 0)
                return iDefinitionIndex;
        }
    }

    /* Return result: */
    return -1;
}

void UIGChooserModel::createMachineItem(const CMachine &machine, UIGChooserItem *pParentItem)
{
    /* Create corresponding item: */
    new UIGChooserItemMachine(/* Parent item and corresponding machine: */
                              pParentItem, machine,
                              /* Which position new group-item should be placed in? */
                              getDesiredPosition(pParentItem, UIGChooserItemType_Machine, machine.GetId()));
}

void UIGChooserModel::saveGroupDefinitions()
{
    /* Make sure there is no group save activity: */
    if (UIGroupDefinitionSaveThread::instance())
        return;

    /* Prepare full group map: */
    QMap<QString, QStringList> groups;
    gatherGroupDefinitions(groups, mainRoot());

    /* Save information in other thread: */
    UIGroupDefinitionSaveThread::prepare();
    emit sigGroupSavingStateChanged();
    connect(UIGroupDefinitionSaveThread::instance(), SIGNAL(sigReload(QString)),
            this, SLOT(sltReloadMachine(QString)));
    UIGroupDefinitionSaveThread::instance()->configure(this, m_groups, groups);
    UIGroupDefinitionSaveThread::instance()->start();
    m_groups = groups;
}

void UIGChooserModel::saveGroupOrders()
{
    /* Make sure there is no group save activity: */
    if (UIGroupOrderSaveThread::instance())
        return;

    /* Prepare full group map: */
    QMap<QString, QStringList> groups;
    gatherGroupOrders(groups, mainRoot());

    /* Save information in other thread: */
    UIGroupOrderSaveThread::prepare();
    emit sigGroupSavingStateChanged();
    UIGroupOrderSaveThread::instance()->configure(this, groups);
    UIGroupOrderSaveThread::instance()->start();
}

void UIGChooserModel::gatherGroupDefinitions(QMap<QString, QStringList> &groups,
                                             UIGChooserItem *pParentGroup)
{
    /* Iterate over all the machine-items: */
    foreach (UIGChooserItem *pItem, pParentGroup->items(UIGChooserItemType_Machine))
        if (UIGChooserItemMachine *pMachineItem = pItem->toMachineItem())
            if (pMachineItem->accessible())
                groups[pMachineItem->id()] << pParentGroup->fullName();
    /* Iterate over all the group-items: */
    foreach (UIGChooserItem *pItem, pParentGroup->items(UIGChooserItemType_Group))
        gatherGroupDefinitions(groups, pItem);
}

void UIGChooserModel::gatherGroupOrders(QMap<QString, QStringList> &groups,
                                        UIGChooserItem *pParentItem)
{
    /* Prepare extra-data key for current group: */
    const QString strExtraDataKey = pParentItem->fullName();
    /* Iterate over all the group-items: */
    foreach (UIGChooserItem *pItem, pParentItem->items(UIGChooserItemType_Group))
    {
        QString strGroupDescriptor(pItem->toGroupItem()->isOpened() ? "go" : "gc");
        groups[strExtraDataKey] << QString("%1=%2").arg(strGroupDescriptor, pItem->name());
        gatherGroupOrders(groups, pItem);
    }
    /* Iterate over all the machine-items: */
    foreach (UIGChooserItem *pItem, pParentItem->items(UIGChooserItemType_Machine))
        groups[strExtraDataKey] << QString("m=%1").arg(pItem->toMachineItem()->id());
}

void UIGChooserModel::makeSureGroupDefinitionsSaveIsFinished()
{
    /* Cleanup if necessary: */
    if (UIGroupDefinitionSaveThread::instance())
        UIGroupDefinitionSaveThread::cleanup();
}

void UIGChooserModel::makeSureGroupOrdersSaveIsFinished()
{
    /* Cleanup if necessary: */
    if (UIGroupOrderSaveThread::instance())
        UIGroupOrderSaveThread::cleanup();
}

/* static */
UIGroupDefinitionSaveThread* UIGroupDefinitionSaveThread::m_spInstance = 0;

/* static */
UIGroupDefinitionSaveThread* UIGroupDefinitionSaveThread::instance()
{
    return m_spInstance;
}

/* static */
void UIGroupDefinitionSaveThread::prepare()
{
    /* Make sure instance not prepared: */
    if (m_spInstance)
        return;

    /* Crate instance: */
    new UIGroupDefinitionSaveThread;
}

/* static */
void UIGroupDefinitionSaveThread::cleanup()
{
    /* Make sure instance prepared: */
    if (!m_spInstance)
        return;

    /* Crate instance: */
    delete m_spInstance;
}

void UIGroupDefinitionSaveThread::configure(QObject *pParent,
                                            const QMap<QString, QStringList> &oldLists,
                                            const QMap<QString, QStringList> &newLists)
{
    m_oldLists = oldLists;
    m_newLists = newLists;
    connect(this, SIGNAL(sigComplete()), pParent, SLOT(sltGroupDefinitionsSaveComplete()));
}

UIGroupDefinitionSaveThread::UIGroupDefinitionSaveThread()
{
    /* Assign instance: */
    m_spInstance = this;
}

UIGroupDefinitionSaveThread::~UIGroupDefinitionSaveThread()
{
    /* Wait: */
    wait();

    /* Erase instance: */
    m_spInstance = 0;
}

void UIGroupDefinitionSaveThread::run()
{
    /* COM prepare: */
    COMBase::InitializeCOM(false);

    /* For every particular machine ID: */
    foreach (const QString &strId, m_newLists.keys())
    {
        /* Get new group list/set: */
        const QStringList &newGroupList = m_newLists.value(strId);
        const UIStringSet &newGroupSet = UIStringSet::fromList(newGroupList);
        /* Get old group list/set: */
        const QStringList &oldGroupList = m_oldLists.value(strId);
        const UIStringSet &oldGroupSet = UIStringSet::fromList(oldGroupList);
        /* Make sure group set changed: */
        if (newGroupSet == oldGroupSet)
            continue;

        /* The next steps are subsequent.
         * Every of them is mandatory in order to continue
         * with common cleanup in case of failure.
         * We have to simulate a try-catch block. */
        CSession session;
        CMachine machine;
        do
        {
            /* 1. Open session: */
            session = vboxGlobal().openSession(strId);
            if (session.isNull())
                break;

            /* 2. Get session machine: */
            machine = session.GetMachine();
            if (machine.isNull())
                break;

            /* 3. Set new groups: */
            machine.SetGroups(newGroupList.toVector());
            if (!machine.isOk())
            {
                msgCenter().cannotSetGroups(machine);
                break;
            }

            /* 4. Save settings: */
            machine.SaveSettings();
            if (!machine.isOk())
            {
                msgCenter().cannotSaveMachineSettings(machine);
                break;
            }
        } while (0);

        /* Cleanup if necessary: */
        if (machine.isNull() || !machine.isOk())
            emit sigReload(strId);
        if (!session.isNull())
            session.UnlockMachine();
    }

    /* Notify listeners about completeness: */
    emit sigComplete();

    /* COM cleanup: */
    COMBase::CleanupCOM();
}

/* static */
UIGroupOrderSaveThread* UIGroupOrderSaveThread::m_spInstance = 0;

/* static */
UIGroupOrderSaveThread* UIGroupOrderSaveThread::instance()
{
    return m_spInstance;
}

/* static */
void UIGroupOrderSaveThread::prepare()
{
    /* Make sure instance not prepared: */
    if (m_spInstance)
        return;

    /* Crate instance: */
    new UIGroupOrderSaveThread;
}

/* static */
void UIGroupOrderSaveThread::cleanup()
{
    /* Make sure instance prepared: */
    if (!m_spInstance)
        return;

    /* Crate instance: */
    delete m_spInstance;
}

void UIGroupOrderSaveThread::configure(QObject *pParent,
                                       const QMap<QString, QStringList> &groups)
{
    m_groups = groups;
    connect(this, SIGNAL(sigComplete()), pParent, SLOT(sltGroupOrdersSaveComplete()));
}

UIGroupOrderSaveThread::UIGroupOrderSaveThread()
{
    /* Assign instance: */
    m_spInstance = this;
}

UIGroupOrderSaveThread::~UIGroupOrderSaveThread()
{
    /* Wait: */
    wait();

    /* Erase instance: */
    m_spInstance = 0;
}

void UIGroupOrderSaveThread::run()
{
    /* COM prepare: */
    COMBase::InitializeCOM(false);

    /* Clear all the extra-data records related to group definitions: */
    gEDataManager->clearSelectorWindowGroupsDefinitions();
    /* For every particular group definition: */
    foreach (const QString &strId, m_groups.keys())
        gEDataManager->setSelectorWindowGroupsDefinitions(strId, m_groups[strId]);

    /* Notify listeners about completeness: */
    emit sigComplete();

    /* COM cleanup: */
    COMBase::CleanupCOM();
}


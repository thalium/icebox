/* $Id: UIGDetailsGroup.cpp $ */
/** @file
 * VBox Qt GUI - UIGDetailsGroup class implementation.
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

/* Qt include: */
# include <QGraphicsScene>

/* GUI includes: */
# include "UIGDetailsGroup.h"
# include "UIGDetailsSet.h"
# include "UIGDetailsModel.h"
# include "UIExtraDataManager.h"
# include "VBoxGlobal.h"
# include "UIVMItem.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGDetailsGroup::UIGDetailsGroup(QGraphicsScene *pParent)
    : UIGDetailsItem(0)
    , m_iPreviousMinimumWidthHint(0)
    , m_iPreviousMinimumHeightHint(0)
    , m_pBuildStep(0)
{
    /* Add group to the parent scene: */
    pParent->addItem(this);

    /* Prepare connections: */
    prepareConnections();
}

UIGDetailsGroup::~UIGDetailsGroup()
{
    /* Cleanup items: */
    clearItems();
}

void UIGDetailsGroup::buildGroup(const QList<UIVMItem*> &machineItems)
{
    /* Remember passed machine-items: */
    m_machineItems = machineItems;

    /* Cleanup superflous items: */
    bool fCleanupPerformed = m_items.size() > m_machineItems.size();
    while (m_items.size() > m_machineItems.size())
        delete m_items.last();
    if (fCleanupPerformed)
        updateGeometry();

    /* Start building group: */
    rebuildGroup();
}

void UIGDetailsGroup::rebuildGroup()
{
    /* Cleanup build-step: */
    delete m_pBuildStep;
    m_pBuildStep = 0;

    /* Generate new group-id: */
    m_strGroupId = QUuid::createUuid().toString();

    /* Request to build first step: */
    emit sigBuildStep(m_strGroupId, 0);
}

void UIGDetailsGroup::stopBuildingGroup()
{
    /* Generate new group-id: */
    m_strGroupId = QUuid::createUuid().toString();
}

void UIGDetailsGroup::sltBuildStep(QString strStepId, int iStepNumber)
{
    /* Cleanup build-step: */
    delete m_pBuildStep;
    m_pBuildStep = 0;

    /* Is step id valid? */
    if (strStepId != m_strGroupId)
        return;

    /* Step number feats the bounds: */
    if (iStepNumber >= 0 && iStepNumber < m_machineItems.size())
    {
        /* Should we create a new set for this step? */
        UIGDetailsSet *pSet = 0;
        if (iStepNumber > m_items.size() - 1)
            pSet = new UIGDetailsSet(this);
        /* Or use existing? */
        else
            pSet = m_items.at(iStepNumber)->toSet();

        /* Create next build-step: */
        m_pBuildStep = new UIBuildStep(this, pSet, strStepId, iStepNumber + 1);

        /* Build set: */
        pSet->buildSet(m_machineItems[iStepNumber], m_machineItems.size() == 1, model()->settings());
    }
    else
    {
        /* Notify listener about build done: */
        emit sigBuildDone();
    }
}

QVariant UIGDetailsGroup::data(int iKey) const
{
    /* Provide other members with required data: */
    switch (iKey)
    {
        /* Layout hints: */
        case GroupData_Margin: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 6;
        case GroupData_Spacing: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 2;
        /* Default: */
        default: break;
    }
    return QVariant();
}

void UIGDetailsGroup::addItem(UIGDetailsItem *pItem)
{
    switch (pItem->type())
    {
        case UIGDetailsItemType_Set: m_items.append(pItem); break;
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
}

void UIGDetailsGroup::removeItem(UIGDetailsItem *pItem)
{
    switch (pItem->type())
    {
        case UIGDetailsItemType_Set: m_items.removeAt(m_items.indexOf(pItem)); break;
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
}

QList<UIGDetailsItem*> UIGDetailsGroup::items(UIGDetailsItemType type /* = UIGDetailsItemType_Set */) const
{
    switch (type)
    {
        case UIGDetailsItemType_Set: return m_items;
        case UIGDetailsItemType_Any: return items(UIGDetailsItemType_Set);
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
    return QList<UIGDetailsItem*>();
}

bool UIGDetailsGroup::hasItems(UIGDetailsItemType type /* = UIGDetailsItemType_Set */) const
{
    switch (type)
    {
        case UIGDetailsItemType_Set: return !m_items.isEmpty();
        case UIGDetailsItemType_Any: return hasItems(UIGDetailsItemType_Set);
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
    return false;
}

void UIGDetailsGroup::clearItems(UIGDetailsItemType type /* = UIGDetailsItemType_Set */)
{
    switch (type)
    {
        case UIGDetailsItemType_Set: while (!m_items.isEmpty()) { delete m_items.last(); } break;
        case UIGDetailsItemType_Any: clearItems(UIGDetailsItemType_Set); break;
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
}

void UIGDetailsGroup::prepareConnections()
{
    /* Prepare group-item connections: */
    connect(this, SIGNAL(sigMinimumWidthHintChanged(int)),
            model(), SIGNAL(sigRootItemMinimumWidthHintChanged(int)));
    connect(this, SIGNAL(sigMinimumHeightHintChanged(int)),
            model(), SIGNAL(sigRootItemMinimumHeightHintChanged(int)));
}

void UIGDetailsGroup::updateGeometry()
{
    /* Call to base class: */
    UIGDetailsItem::updateGeometry();

    /* Group-item should notify details-view if minimum-width-hint was changed: */
    int iMinimumWidthHint = minimumWidthHint();
    if (m_iPreviousMinimumWidthHint != iMinimumWidthHint)
    {
        /* Save new minimum-width-hint, notify listener: */
        m_iPreviousMinimumWidthHint = iMinimumWidthHint;
        emit sigMinimumWidthHintChanged(m_iPreviousMinimumWidthHint);
    }
    /* Group-item should notify details-view if minimum-height-hint was changed: */
    int iMinimumHeightHint = minimumHeightHint();
    if (m_iPreviousMinimumHeightHint != iMinimumHeightHint)
    {
        /* Save new minimum-height-hint, notify listener: */
        m_iPreviousMinimumHeightHint = iMinimumHeightHint;
        emit sigMinimumHeightHintChanged(m_iPreviousMinimumHeightHint);
    }
}

int UIGDetailsGroup::minimumWidthHint() const
{
    /* Prepare variables: */
    int iMargin = data(GroupData_Margin).toInt();
    int iMinimumWidthHint = 0;

    /* For each the set we have: */
    bool fHasItems = false;
    foreach (UIGDetailsItem *pItem, items())
    {
        /* Ignore which are with no details: */
        if (UIGDetailsSet *pSetItem = pItem->toSet())
            if (!pSetItem->hasDetails())
                continue;
        /* And take into account all the others: */
        iMinimumWidthHint = qMax(iMinimumWidthHint, pItem->minimumWidthHint());
        if (!fHasItems)
            fHasItems = true;
    }

    /* Add two margins finally: */
    if (fHasItems)
        iMinimumWidthHint += 2 * iMargin;

    /* Return result: */
    return iMinimumWidthHint;
}

int UIGDetailsGroup::minimumHeightHint() const
{
    /* Prepare variables: */
    int iMargin = data(GroupData_Margin).toInt();
    int iSpacing = data(GroupData_Spacing).toInt();
    int iMinimumHeightHint = 0;

    /* For each the set we have: */
    bool fHasItems = false;
    foreach (UIGDetailsItem *pItem, items())
    {
        /* Ignore which are with no details: */
        if (UIGDetailsSet *pSetItem = pItem->toSet())
            if (!pSetItem->hasDetails())
                continue;
        /* And take into account all the others: */
        iMinimumHeightHint += (pItem->minimumHeightHint() + iSpacing);
        if (!fHasItems)
            fHasItems = true;
    }
    /* Minus last spacing: */
    if (fHasItems)
        iMinimumHeightHint -= iSpacing;

    /* Add two margins finally: */
    if (fHasItems)
        iMinimumHeightHint += 2 * iMargin;

    /* Return result: */
    return iMinimumHeightHint;
}

void UIGDetailsGroup::updateLayout()
{
    /* Prepare variables: */
    int iMargin = data(GroupData_Margin).toInt();
    int iSpacing = data(GroupData_Spacing).toInt();
    int iMaximumWidth = (int)geometry().width() - 2 * iMargin;
    int iVerticalIndent = iMargin;

    /* Layout all the sets: */
    foreach (UIGDetailsItem *pItem, items())
    {
        /* Ignore sets with no details: */
        if (UIGDetailsSet *pSetItem = pItem->toSet())
            if (!pSetItem->hasDetails())
                continue;
        /* Move set: */
        pItem->setPos(iMargin, iVerticalIndent);
        /* Resize set: */
        int iWidth = iMaximumWidth;
        pItem->resize(iWidth, pItem->minimumHeightHint());
        /* Layout set content: */
        pItem->updateLayout();
        /* Advance indent: */
        iVerticalIndent += (pItem->minimumHeightHint() + iSpacing);
    }
}


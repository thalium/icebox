/* $Id: UIGChooserItemMachine.cpp $ */
/** @file
 * VBox Qt GUI - UIGChooserItemMachine class implementation.
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
# include <QPainter>
# include <QStyleOptionGraphicsItem>
# include <QGraphicsSceneMouseEvent>

/* GUI includes: */
# include "UIGChooserItemMachine.h"
# include "UIGChooserItemGroup.h"
# include "UIGChooserModel.h"
# include "UIGraphicsToolBar.h"
# include "UIGraphicsZoomButton.h"
# include "VBoxGlobal.h"
# include "UIIconPool.h"
# include "UIActionPoolSelector.h"
# include "UIImageTools.h"

/* COM includes: */
# include "COMEnums.h"
# include "CMachine.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static */
QString UIGChooserItemMachine::className() { return "UIGChooserItemMachine"; }

UIGChooserItemMachine::UIGChooserItemMachine(UIGChooserItem *pParent,
                                             const CMachine &machine,
                                             int iPosition /* = -1 */)
    : UIGChooserItem(pParent, pParent->isTemporary())
    , UIVMItem(machine)
{
    /* Prepare: */
    prepare();

    /* Add item to the parent: */
    AssertMsg(parentItem(), ("No parent set for machine-item!"));
    parentItem()->addItem(this, iPosition);
    setZValue(parentItem()->zValue() + 1);

    /* Init: */
    updatePixmaps();
    updateName();
    updateSnapshotName();

    /* Translate finally: */
    retranslateUi();
}

UIGChooserItemMachine::UIGChooserItemMachine(UIGChooserItem *pParent,
                                             UIGChooserItemMachine *pCopyFrom,
                                             int iPosition /* = -1 */)
    : UIGChooserItem(pParent, pParent->isTemporary())
    , UIVMItem(pCopyFrom->machine())
{
    /* Prepare: */
    prepare();

    /* Add item to the parent: */
    AssertMsg(parentItem(), ("No parent set for machine-item!"));
    parentItem()->addItem(this, iPosition);
    setZValue(parentItem()->zValue() + 1);

    /* Init: */
    updatePixmaps();
    updateName();
    updateSnapshotName();

    /* Translate finally: */
    retranslateUi();
}

UIGChooserItemMachine::~UIGChooserItemMachine()
{
    /* If that item is focused: */
    if (model()->focusItem() == this)
    {
        /* Unset the focus: */
        model()->setFocusItem(0);
    }
    /* If that item is in selection list: */
    if (model()->currentItems().contains(this))
    {
        /* Remove item from the selection list: */
        model()->removeFromCurrentItems(this);
    }
    /* If that item is in navigation list: */
    if (model()->navigationList().contains(this))
    {
        /* Remove item from the navigation list: */
        model()->removeFromNavigationList(this);
    }

    /* Remove item from the parent: */
    AssertMsg(parentItem(), ("No parent set for machine-item!"));
    parentItem()->removeItem(this);
}

QString UIGChooserItemMachine::name() const
{
    return UIVMItem::name();
}

QString UIGChooserItemMachine::description() const
{
    return m_strDescription;
}

QString UIGChooserItemMachine::fullName() const
{
    /* Get full parent name, append with '/' if not yet appended: */
    AssertMsg(parentItem(), ("Incorrect parent set!"));
    QString strFullParentName = parentItem()->fullName();
    if (!strFullParentName.endsWith('/'))
        strFullParentName.append('/');
    /* Return full item name based on parent prefix: */
    return strFullParentName + name();
}

QString UIGChooserItemMachine::definition() const
{
    return QString("m=%1").arg(name());
}

bool UIGChooserItemMachine::isLockedMachine() const
{
    KMachineState state = machineState();
    return state != KMachineState_PoweredOff &&
           state != KMachineState_Saved &&
           state != KMachineState_Teleported &&
           state != KMachineState_Aborted;
}

/* static */
void UIGChooserItemMachine::enumerateMachineItems(const QList<UIGChooserItem*> &il,
                                                  QList<UIGChooserItemMachine*> &ol,
                                                  int iEnumerationFlags /* = 0 */)
{
    /* Enumerate all the passed items: */
    foreach (UIGChooserItem *pItem, il)
    {
        /* If that is machine-item: */
        if (pItem->type() == UIGChooserItemType_Machine)
        {
            /* Get the iterated machine-item: */
            UIGChooserItemMachine *pMachineItem = pItem->toMachineItem();
            /* Skip if exactly this item is already enumerated: */
            if (ol.contains(pMachineItem))
                continue;
            /* Skip if item with same ID is already enumerated but we need unique: */
            if ((iEnumerationFlags & UIGChooserItemMachineEnumerationFlag_Unique) &&
                contains(ol, pMachineItem))
                continue;
            /* Skip if this item is accessible and we no need it: */
            if ((iEnumerationFlags & UIGChooserItemMachineEnumerationFlag_Inaccessible) &&
                pMachineItem->accessible())
                continue;
            /* Add it: */
            ol << pMachineItem;
        }
        /* If that is group-item: */
        else if (pItem->type() == UIGChooserItemType_Group)
        {
            /* Enumerate all the machine-items recursively: */
            enumerateMachineItems(pItem->items(UIGChooserItemType_Machine), ol, iEnumerationFlags);
            /* Enumerate all the group-items recursively: */
            enumerateMachineItems(pItem->items(UIGChooserItemType_Group), ol, iEnumerationFlags);
        }
    }
}

QVariant UIGChooserItemMachine::data(int iKey) const
{
    /* Provide other members with required data: */
    switch (iKey)
    {
        /* Layout hints: */
        case MachineItemData_Margin: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;
        case MachineItemData_MajorSpacing: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 2;
        case MachineItemData_MinorSpacing: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;
        case MachineItemData_TextSpacing: return 0;

        /* Pixmaps: */
        case MachineItemData_SettingsButtonPixmap: return UIIconPool::iconSet(":/vm_settings_16px.png");
        case MachineItemData_StartButtonPixmap: return UIIconPool::iconSet(":/vm_start_16px.png");
        case MachineItemData_PauseButtonPixmap: return UIIconPool::iconSet(":/vm_pause_16px.png");
        case MachineItemData_CloseButtonPixmap: return UIIconPool::iconSet(":/exit_16px.png");

        /* Sizes: */
        case MachineItemData_ToolBarSize: return m_pToolBar ? m_pToolBar->minimumSizeHint().toSize() : QSize(0, 0);

        /* Default: */
        default: break;
    }
    return QVariant();
}

void UIGChooserItemMachine::updatePixmaps()
{
    /* Update pixmap: */
    updatePixmap();

    /* Update state-pixmap: */
    updateStatePixmap();
}

void UIGChooserItemMachine::updatePixmap()
{
    /* Get new pixmap and pixmap-size: */
    QSize pixmapSize;
    QPixmap pixmap = osPixmap(&pixmapSize);
    /* Update linked values: */
    if (m_pixmapSize != pixmapSize)
    {
        m_pixmapSize = pixmapSize;
        updateFirstRowMaximumWidth();
        updateGeometry();
    }
    if (m_pixmap.toImage() != pixmap.toImage())
    {
        m_pixmap = pixmap;
        update();
    }
}

void UIGChooserItemMachine::updateStatePixmap()
{
    /* Determine the icon metric: */
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
    /* Get new state-pixmap and state-pixmap size: */
    const QIcon stateIcon = machineStateIcon();
    AssertReturnVoid(!stateIcon.isNull());
    const QSize statePixmapSize = QSize(iIconMetric, iIconMetric);
    const QPixmap statePixmap = stateIcon.pixmap(statePixmapSize);
    /* Update linked values: */
    if (m_statePixmapSize != statePixmapSize)
    {
        m_statePixmapSize = statePixmapSize;
        updateGeometry();
    }
    if (m_statePixmap.toImage() != statePixmap.toImage())
    {
        m_statePixmap = statePixmap;
        update();
    }
}

void UIGChooserItemMachine::updateName()
{
    /* Get new name: */
    QString strName = name();

    /* Is there something changed? */
    if (m_strName == strName)
        return;

    /* Update linked values: */
    m_strName = strName;
    updateMinimumNameWidth();
    updateVisibleName();
}

void UIGChooserItemMachine::updateSnapshotName()
{
    /* Get new snapshot-name: */
    QString strSnapshotName = snapshotName();

    /* Is there something changed? */
    if (m_strSnapshotName == strSnapshotName)
        return;

    /* Update linked values: */
    m_strSnapshotName = strSnapshotName;
    updateMinimumSnapshotNameWidth();
    updateVisibleSnapshotName();
}

void UIGChooserItemMachine::updateFirstRowMaximumWidth()
{
    /* Prepare variables: */
    int iMargin = data(MachineItemData_Margin).toInt();
    int iMajorSpacing = data(MachineItemData_MajorSpacing).toInt();
    int iToolBarWidth = data(MachineItemData_ToolBarSize).toSize().width();

    /* Calculate new maximum width for the first row: */
    int iFirstRowMaximumWidth = (int)geometry().width();
    iFirstRowMaximumWidth -= iMargin; /* left margin */
    iFirstRowMaximumWidth -= m_pixmapSize.width(); /* pixmap width */
    iFirstRowMaximumWidth -= iMajorSpacing; /* spacing between pixmap and name(s) */
    if (m_pToolBar)
    {
        iFirstRowMaximumWidth -= iMajorSpacing; /* spacing before toolbar */
        iFirstRowMaximumWidth -= iToolBarWidth; /* toolbar width */
    }
    iFirstRowMaximumWidth -= iMargin; /* right margin */

    /* Is there something changed? */
    if (m_iFirstRowMaximumWidth == iFirstRowMaximumWidth)
        return;

    /* Update linked values: */
    m_iFirstRowMaximumWidth = iFirstRowMaximumWidth;
    updateMaximumNameWidth();
    updateMaximumSnapshotNameWidth();
}

void UIGChooserItemMachine::updateMinimumNameWidth()
{
    /* Calculate new minimum name width: */
    QPaintDevice *pPaintDevice = model()->paintDevice();
    QFontMetrics fm(m_nameFont, pPaintDevice);
    int iMinimumNameWidth = fm.width(compressText(m_nameFont, pPaintDevice, m_strName, textWidth(m_nameFont, pPaintDevice, 15)));

    /* Is there something changed? */
    if (m_iMinimumNameWidth == iMinimumNameWidth)
        return;

    /* Update linked values: */
    m_iMinimumNameWidth = iMinimumNameWidth;
    updateGeometry();
}

void UIGChooserItemMachine::updateMinimumSnapshotNameWidth()
{
    /* Calculate new minimum snapshot-name width: */
    int iMinimumSnapshotNameWidth = 0;
    /* Is there any snapshot exists? */
    if (!m_strSnapshotName.isEmpty())
    {
        QFontMetrics fm(m_snapshotNameFont, model()->paintDevice());
        int iBracketWidth = fm.width("()"); /* bracket width */
        int iActualTextWidth = fm.width(m_strSnapshotName); /* snapshot-name width */
        int iMinimumTextWidth = fm.width("..."); /* ellipsis width */
        iMinimumSnapshotNameWidth = iBracketWidth + qMin(iActualTextWidth, iMinimumTextWidth);
    }

    /* Is there something changed? */
    if (m_iMinimumSnapshotNameWidth == iMinimumSnapshotNameWidth)
        return;

    /* Update linked values: */
    m_iMinimumSnapshotNameWidth = iMinimumSnapshotNameWidth;
    updateMaximumNameWidth();
    updateGeometry();
}

void UIGChooserItemMachine::updateMaximumNameWidth()
{
    /* Calculate new maximum name width: */
    int iMaximumNameWidth = m_iFirstRowMaximumWidth;
    /* Do we have a minimum snapshot-name width? */
    if (m_iMinimumSnapshotNameWidth != 0)
    {
        /* Prepare variables: */
        int iMinorSpacing = data(MachineItemData_MinorSpacing).toInt();
        /* Take spacing and snapshot-name into account: */
        iMaximumNameWidth -= (iMinorSpacing + m_iMinimumSnapshotNameWidth);
    }

    /* Is there something changed? */
    if (m_iMaximumNameWidth == iMaximumNameWidth)
        return;

    /* Update linked values: */
    m_iMaximumNameWidth = iMaximumNameWidth;
    updateVisibleName();
}

void UIGChooserItemMachine::updateMaximumSnapshotNameWidth()
{
    /* Prepare variables: */
    int iMinorSpacing = data(MachineItemData_MinorSpacing).toInt();

    /* Calculate new maximum snapshot-name width: */
    int iMaximumSnapshotNameWidth = m_iFirstRowMaximumWidth;
    iMaximumSnapshotNameWidth -= (iMinorSpacing + m_visibleNameSize.width());

    /* Is there something changed? */
    if (m_iMaximumSnapshotNameWidth == iMaximumSnapshotNameWidth)
        return;

    /* Update linked values: */
    m_iMaximumSnapshotNameWidth = iMaximumSnapshotNameWidth;
    updateVisibleSnapshotName();
}

void UIGChooserItemMachine::updateVisibleName()
{
    /* Prepare variables: */
    QPaintDevice *pPaintDevice = model()->paintDevice();

    /* Calculate new visible name and name-size: */
    QString strVisibleName = compressText(m_nameFont, pPaintDevice, m_strName, m_iMaximumNameWidth);
    QSize visibleNameSize = textSize(m_nameFont, pPaintDevice, strVisibleName);

    /* Update linked values: */
    if (m_visibleNameSize != visibleNameSize)
    {
        m_visibleNameSize = visibleNameSize;
        updateMaximumSnapshotNameWidth();
        updateGeometry();
    }
    if (m_strVisibleName != strVisibleName)
    {
        m_strVisibleName = strVisibleName;
        update();
    }
}

void UIGChooserItemMachine::updateVisibleSnapshotName()
{
    /* Prepare variables: */
    QPaintDevice *pPaintDevice = model()->paintDevice();

    /* Calculate new visible snapshot-name: */
    int iBracketWidth = QFontMetrics(m_snapshotNameFont, pPaintDevice).width("()");
    QString strVisibleSnapshotName = compressText(m_snapshotNameFont, pPaintDevice, m_strSnapshotName,
                                                  m_iMaximumSnapshotNameWidth - iBracketWidth);
    strVisibleSnapshotName = QString("(%1)").arg(strVisibleSnapshotName);
    QSize visibleSnapshotNameSize = textSize(m_snapshotNameFont, pPaintDevice, strVisibleSnapshotName);

    /* Update linked values: */
    if (m_visibleSnapshotNameSize != visibleSnapshotNameSize)
    {
        m_visibleSnapshotNameSize = visibleSnapshotNameSize;
        updateGeometry();
    }
    if (m_strVisibleSnapshotName != strVisibleSnapshotName)
    {
        m_strVisibleSnapshotName = strVisibleSnapshotName;
        update();
    }
}

void UIGChooserItemMachine::updateStateText()
{
    /* Get new state-text and state-text size: */
    QString strStateText = machineStateName();
    QSize stateTextSize = textSize(m_stateTextFont, model()->paintDevice(), m_strStateText);

    /* Update linked values: */
    if (m_stateTextSize != stateTextSize)
    {
        m_stateTextSize = stateTextSize;
        updateGeometry();
    }
    if (m_strStateText != strStateText)
    {
        m_strStateText = strStateText;
        update();
    }
}

void UIGChooserItemMachine::retranslateUi()
{
    /* Update description: */
    m_strDescription = tr("Virtual Machine");

    /* Update state text: */
    updateStateText();

    /* Update machine tool-tip: */
    updateToolTip();
}

void UIGChooserItemMachine::startEditing()
{
    AssertMsgFailed(("Machine graphics item do NOT support editing yet!"));
}

void UIGChooserItemMachine::updateToolTip()
{
    setToolTip(toolTipText());
}

void UIGChooserItemMachine::addItem(UIGChooserItem*, int)
{
    AssertMsgFailed(("Machine graphics item do NOT support children!"));
}

void UIGChooserItemMachine::removeItem(UIGChooserItem*)
{
    AssertMsgFailed(("Machine graphics item do NOT support children!"));
}

void UIGChooserItemMachine::setItems(const QList<UIGChooserItem*>&, UIGChooserItemType)
{
    AssertMsgFailed(("Machine graphics item do NOT support children!"));
}

QList<UIGChooserItem*> UIGChooserItemMachine::items(UIGChooserItemType) const
{
    AssertMsgFailed(("Machine graphics item do NOT support children!"));
    return QList<UIGChooserItem*>();
}

bool UIGChooserItemMachine::hasItems(UIGChooserItemType) const
{
    AssertMsgFailed(("Machine graphics item do NOT support children!"));
    return false;
}

void UIGChooserItemMachine::clearItems(UIGChooserItemType)
{
    AssertMsgFailed(("Machine graphics item do NOT support children!"));
}

void UIGChooserItemMachine::updateAll(const QString &strId)
{
    /* Skip other ids: */
    if (id() != strId)
        return;

    /* Update this machine-item: */
    recache();
    updatePixmaps();
    updateName();
    updateSnapshotName();
    updateStateText();
    updateToolTip();

    /* Update parent group-item: */
    parentItem()->updateToolTip();
    parentItem()->update();
}

void UIGChooserItemMachine::removeAll(const QString &strId)
{
    /* Skip wrong id: */
    if (id() != strId)
        return;

    /* Exclude itself from the current items: */
    if (model()->currentItems().contains(this))
        model()->removeFromCurrentItems(this);
    /* Move the focus item to the first available current after that: */
    if (model()->focusItem() == this && !model()->currentItems().isEmpty())
        model()->setFocusItem(model()->currentItems().first());

    /* Remove item: */
    delete this;
}

UIGChooserItem* UIGChooserItemMachine::searchForItem(const QString &strSearchTag, int iItemSearchFlags)
{
    /* Ignoring if we are not searching for the machine-item? */
    if (!(iItemSearchFlags & UIGChooserItemSearchFlag_Machine))
        return 0;

    /* Are we searching by the exact name? */
    if (iItemSearchFlags & UIGChooserItemSearchFlag_ExactName)
    {
        /* Exact name doesn't match? */
        if (name() != strSearchTag)
            return 0;
    }
    /* Are we searching by the few first symbols? */
    else
    {
        /* Name doesn't start with passed symbols? */
        if (!name().startsWith(strSearchTag, Qt::CaseInsensitive))
            return 0;
    }

    /* Returning this: */
    return this;
}

UIGChooserItemMachine* UIGChooserItemMachine::firstMachineItem()
{
    return this;
}

void UIGChooserItemMachine::sortItems()
{
    AssertMsgFailed(("Machine graphics item do NOT support children!"));
}

void UIGChooserItemMachine::updateLayout()
{
    /* Update tool-bar: */
    if (m_pToolBar)
    {
        /* Prepare variables: */
        QSize size = geometry().size().toSize();

        /* Prepare variables: */
        int iMachineItemWidth = size.width();
        int iMachineItemHeight = size.height();
        int iToolBarHeight = data(MachineItemData_ToolBarSize).toSize().height();

        /* Configure tool-bar: */
        QSize toolBarSize = m_pToolBar->minimumSizeHint().toSize();
        int iToolBarX = iMachineItemWidth - 1 - toolBarSize.width();
        int iToolBarY = (iMachineItemHeight - iToolBarHeight) / 2;
        m_pToolBar->setPos(iToolBarX, iToolBarY);
        m_pToolBar->resize(toolBarSize);
        m_pToolBar->updateLayout();

        /* Configure buttons: */
        m_pStartButton->updateAnimation();
        m_pSettingsButton->updateAnimation();
        m_pCloseButton->updateAnimation();
        m_pPauseButton->updateAnimation();
    }
}

int UIGChooserItemMachine::minimumWidthHint() const
{
    /* Prepare variables: */
    int iMargin = data(MachineItemData_Margin).toInt();
    int iMajorSpacing = data(MachineItemData_MajorSpacing).toInt();
    int iMinorSpacing = data(MachineItemData_MinorSpacing).toInt();
    int iToolBarWidth = data(MachineItemData_ToolBarSize).toSize().width();

    /* Calculating proposed width: */
    int iProposedWidth = 0;

    /* Two margins: */
    iProposedWidth += 2 * iMargin;
    /* And machine-item content to take into account: */
    int iTopLineWidth = m_iMinimumNameWidth;
    if (!m_strSnapshotName.isEmpty())
        iTopLineWidth += (iMinorSpacing +
                          m_iMinimumSnapshotNameWidth);
    int iBottomLineWidth = m_statePixmapSize.width() +
                           iMinorSpacing +
                           m_stateTextSize.width();
    int iRightColumnWidth = qMax(iTopLineWidth, iBottomLineWidth);
    int iMachineItemWidth = m_pixmapSize.width() +
                            iMajorSpacing +
                            iRightColumnWidth;
    if (m_pToolBar)
        iMachineItemWidth += (iMajorSpacing + iToolBarWidth);
    iProposedWidth += iMachineItemWidth;

    /* Return result: */
    return iProposedWidth;
}

int UIGChooserItemMachine::minimumHeightHint() const
{
    /* Prepare variables: */
    int iMargin = data(MachineItemData_Margin).toInt();
    int iMachineItemTextSpacing = data(MachineItemData_TextSpacing).toInt();
    int iToolBarHeight = data(MachineItemData_ToolBarSize).toSize().height();

    /* Calculating proposed height: */
    int iProposedHeight = 0;

    /* Two margins: */
    iProposedHeight += 2 * iMargin;
    /* And machine-item content to take into account: */
    int iTopLineHeight = qMax(m_visibleNameSize.height(), m_visibleSnapshotNameSize.height());
    int iBottomLineHeight = qMax(m_statePixmapSize.height(), m_stateTextSize.height());
    int iRightColumnHeight = iTopLineHeight +
                             iMachineItemTextSpacing +
                             iBottomLineHeight;
    QList<int> heights;
    heights << m_pixmapSize.height() << iRightColumnHeight << iToolBarHeight;
    int iMaxHeight = 0;
    foreach (int iHeight, heights)
        iMaxHeight = qMax(iMaxHeight, iHeight);
    iProposedHeight += iMaxHeight;

    /* Return result: */
    return iProposedHeight;
}

QSizeF UIGChooserItemMachine::sizeHint(Qt::SizeHint which, const QSizeF &constraint /* = QSizeF() */) const
{
    /* If Qt::MinimumSize requested: */
    if (which == Qt::MinimumSize)
        return QSizeF(minimumWidthHint(), minimumHeightHint());
    /* Else call to base-class: */
    return UIGChooserItem::sizeHint(which, constraint);
}

QPixmap UIGChooserItemMachine::toPixmap()
{
    /* Ask item to paint itself into pixmap: */
    QSize minimumSize = minimumSizeHint().toSize();
    QPixmap pixmap(minimumSize);
    QPainter painter(&pixmap);
    QStyleOptionGraphicsItem options;
    options.rect = QRect(QPoint(0, 0), minimumSize);
    paint(&painter, &options);
    return pixmap;
}

bool UIGChooserItemMachine::isDropAllowed(QGraphicsSceneDragDropEvent *pEvent, DragToken where) const
{
    /* No drops while saving groups: */
    if (model()->isGroupSavingInProgress())
        return false;
    /* No drops for immutable item: */
    if (isLockedMachine())
        return false;
    /* Get mime: */
    const QMimeData *pMimeData = pEvent->mimeData();
    /* If drag token is shown, its up to parent to decide: */
    if (where != DragToken_Off)
        return parentItem()->isDropAllowed(pEvent);
    /* Else we should make sure machine is accessible: */
    if (!accessible())
        return false;
    /* Else we should try to cast mime to known classes: */
    if (pMimeData->hasFormat(UIGChooserItemMachine::className()))
    {
        /* Make sure passed item id is not ours: */
        const UIGChooserItemMimeData *pCastedMimeData = qobject_cast<const UIGChooserItemMimeData*>(pMimeData);
        AssertMsg(pCastedMimeData, ("Can't cast passed mime-data to UIGChooserItemMimeData!"));
        UIGChooserItem *pItem = pCastedMimeData->item();
        UIGChooserItemMachine *pMachineItem = pItem->toMachineItem();
        /* Make sure passed machine is mutable: */
        if (pMachineItem->isLockedMachine())
            return false;
        return pMachineItem->id() != id();
    }
    /* That was invalid mime: */
    return false;
}

void UIGChooserItemMachine::processDrop(QGraphicsSceneDragDropEvent *pEvent, UIGChooserItem *pFromWho, DragToken where)
{
    /* Get mime: */
    const QMimeData *pMime = pEvent->mimeData();
    /* Make sure this handler called by this item (not by children): */
    AssertMsg(!pFromWho && where == DragToken_Off, ("Machine graphics item do NOT support children!"));
    Q_UNUSED(pFromWho);
    Q_UNUSED(where);
    if (pMime->hasFormat(UIGChooserItemMachine::className()))
    {
        switch (pEvent->proposedAction())
        {
            case Qt::MoveAction:
            case Qt::CopyAction:
            {
                /* Remember scene: */
                UIGChooserModel *pModel = model();

                /* Get passed item: */
                const UIGChooserItemMimeData *pCastedMime = qobject_cast<const UIGChooserItemMimeData*>(pMime);
                AssertMsg(pCastedMime, ("Can't cast passed mime-data to UIGChooserItemMimeData!"));
                UIGChooserItem *pItem = pCastedMime->item();

                /* Group passed item with current item into the new group: */
                UIGChooserItemGroup *pNewGroupItem = new UIGChooserItemGroup(parentItem(),
                                                                             UIGChooserModel::uniqueGroupName(parentItem()),
                                                                             true);
                new UIGChooserItemMachine(pNewGroupItem, this);
                new UIGChooserItemMachine(pNewGroupItem, pItem->toMachineItem());

                /* If proposed action is 'move': */
                if (pEvent->proposedAction() == Qt::MoveAction)
                {
                    /* Delete passed item: */
                    delete pItem;
                }
                /* Delete this item: */
                delete this;

                /* Update model: */
                pModel->cleanupGroupTree();
                pModel->updateNavigation();
                pModel->updateLayout();
                pModel->setCurrentItem(pNewGroupItem);
                pModel->saveGroupSettings();
                break;
            }
            default:
                break;
        }
    }
}

void UIGChooserItemMachine::resetDragToken()
{
    /* Reset drag token for this item: */
    if (dragTokenPlace() != DragToken_Off)
    {
        setDragTokenPlace(DragToken_Off);
        update();
    }
}

QMimeData* UIGChooserItemMachine::createMimeData()
{
    return new UIGChooserItemMimeData(this);
}

void UIGChooserItemMachine::resizeEvent(QGraphicsSceneResizeEvent *pEvent)
{
    /* Call to base-class: */
    UIGChooserItem::resizeEvent(pEvent);

    /* What is the new geometry? */
    QRectF newGeometry = geometry();

    /* Should we update visible name? */
    if (previousGeometry().width() != newGeometry.width())
        updateFirstRowMaximumWidth();

    /* Remember the new geometry: */
    setPreviousGeometry(newGeometry);
}

void UIGChooserItemMachine::mousePressEvent(QGraphicsSceneMouseEvent *pEvent)
{
    /* Call to base-class: */
    UIGChooserItem::mousePressEvent(pEvent);
    /* No drag for inaccessible: */
    if (!accessible())
        pEvent->ignore();
}

void UIGChooserItemMachine::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget* /* pWidget = 0 */)
{
    /* Setup: */
    pPainter->setRenderHint(QPainter::Antialiasing);

    /* Paint decorations: */
    paintDecorations(pPainter, pOption);

    /* Paint machine info: */
    paintMachineInfo(pPainter, pOption);
}

void UIGChooserItemMachine::paintDecorations(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption)
{
    /* Prepare variables: */
    QRect fullRect = pOption->rect;

    /* Paint background: */
    paintBackground(pPainter, fullRect);

    /* Paint frame: */
    paintFrameRectangle(pPainter, fullRect);
}

void UIGChooserItemMachine::paintBackground(QPainter *pPainter, const QRect &rect)
{
    /* Save painter: */
    pPainter->save();

    /* Prepare color: */
    QPalette pal = palette();

    /* Selection background: */
    if (model()->currentItems().contains(this))
    {
        /* Prepare color: */
        QColor highlight = pal.color(QPalette::Active, QPalette::Highlight);
        /* Draw gradient: */
        QLinearGradient bgGrad(rect.topLeft(), rect.bottomLeft());
        bgGrad.setColorAt(0, highlight.lighter(m_iHighlightLightness));
        bgGrad.setColorAt(1, highlight);
        pPainter->fillRect(rect, bgGrad);
    }
    /* Hovering background: */
    else if (isHovered())
    {
        /* Prepare color: */
        QColor highlight = pal.color(QPalette::Active, QPalette::Highlight);
        /* Draw gradient: */
        QLinearGradient bgGrad(rect.topLeft(), rect.bottomLeft());
        bgGrad.setColorAt(0, highlight.lighter(m_iHoverHighlightLightness));
        bgGrad.setColorAt(1, highlight.lighter(m_iHoverLightness));
        pPainter->fillRect(rect, bgGrad);
    }

    /* Paint drag token UP? */
    if (dragTokenPlace() != DragToken_Off)
    {
        /* Window color: */
        QColor base = pal.color(QPalette::Active, model()->currentItems().contains(this) ?
                                QPalette::Highlight : QPalette::Window);
        QLinearGradient dragTokenGradient;
        QRect dragTokenRect = rect;
        if (dragTokenPlace() == DragToken_Up)
        {
            dragTokenRect.setHeight(5);
            dragTokenGradient.setStart(dragTokenRect.bottomLeft());
            dragTokenGradient.setFinalStop(dragTokenRect.topLeft());
        }
        else if (dragTokenPlace() == DragToken_Down)
        {
            dragTokenRect.setTopLeft(dragTokenRect.bottomLeft() - QPoint(0, 5));
            dragTokenGradient.setStart(dragTokenRect.topLeft());
            dragTokenGradient.setFinalStop(dragTokenRect.bottomLeft());
        }
        dragTokenGradient.setColorAt(0, base.darker(dragTokenDarkness()));
        dragTokenGradient.setColorAt(1, base.darker(dragTokenDarkness() + 40));
        pPainter->fillRect(dragTokenRect, dragTokenGradient);
    }

    /* Restore painter: */
    pPainter->restore();
}

void UIGChooserItemMachine::paintFrameRectangle(QPainter *pPainter, const QRect &rect)
{
    /* Only chosen and/or hovered item should have a frame: */
    if (!model()->currentItems().contains(this) && !isHovered())
        return;

    /* Simple frame: */
    pPainter->save();
    QPalette pal = palette();
    QColor strokeColor = pal.color(QPalette::Active,
                                   model()->currentItems().contains(this) ?
                                   QPalette::Mid : QPalette::Highlight);
    pPainter->setPen(strokeColor);
    pPainter->drawRect(rect);
    pPainter->restore();
}

void UIGChooserItemMachine::paintMachineInfo(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption)
{
    /* Prepare variables: */
    QRect fullRect = pOption->rect;
    int iFullHeight = fullRect.height();
    int iMargin = data(MachineItemData_Margin).toInt();
    int iMajorSpacing = data(MachineItemData_MajorSpacing).toInt();
    int iMinorSpacing = data(MachineItemData_MinorSpacing).toInt();
    int iMachineItemTextSpacing = data(MachineItemData_TextSpacing).toInt();

    /* Selected item foreground: */
    if (model()->currentItems().contains(this))
    {
        QPalette pal = palette();
        pPainter->setPen(pal.color(QPalette::HighlightedText));
    }
    /* Hovered item foreground: */
    else if (isHovered())
    {
        /* Prepare color: */
        QPalette pal = palette();
        QColor highlight = pal.color(QPalette::Active, QPalette::Highlight);
        QColor hhl = highlight.lighter(m_iHoverHighlightLightness);
        if (hhl.value() - hhl.saturation() > 0)
            pPainter->setPen(pal.color(QPalette::Active, QPalette::Text));
        else
            pPainter->setPen(pal.color(QPalette::Active, QPalette::HighlightedText));
    }

    /* Calculate indents: */
    int iLeftColumnIndent = iMargin;

    /* Paint left column: */
    {
        /* Prepare variables: */
        int iMachinePixmapX = iLeftColumnIndent;
        int iMachinePixmapY = (iFullHeight - m_pixmap.height() / m_pixmap.devicePixelRatio()) / 2;
        /* Paint pixmap: */
        paintPixmap(/* Painter: */
                    pPainter,
                    /* Point to paint in: */
                    QPoint(iMachinePixmapX, iMachinePixmapY),
                    /* Pixmap to paint: */
                    m_pixmap);
    }

    /* Calculate indents: */
    int iRightColumnIndent = iLeftColumnIndent +
                             m_pixmapSize.width() +
                             iMajorSpacing;

    /* Paint right column: */
    {
        /* Calculate indents: */
        int iTopLineHeight = qMax(m_visibleNameSize.height(), m_visibleSnapshotNameSize.height());
        int iBottomLineHeight = qMax(m_statePixmapSize.height(), m_stateTextSize.height());
        int iRightColumnHeight = iTopLineHeight + iMachineItemTextSpacing + iBottomLineHeight;
        int iTopLineIndent = (iFullHeight - iRightColumnHeight) / 2;

        /* Paint top line: */
        {
            /* Paint left element: */
            {
                /* Prepare variables: */
                int iNameX = iRightColumnIndent;
                int iNameY = iTopLineIndent;
                /* Paint name: */
                paintText(/* Painter: */
                          pPainter,
                          /* Point to paint in: */
                          QPoint(iNameX, iNameY),
                          /* Font to paint text: */
                          m_nameFont,
                          /* Paint device: */
                          model()->paintDevice(),
                          /* Text to paint: */
                          m_strVisibleName);
            }

            /* Calculate indents: */
            int iSnapshotNameIndent = iRightColumnIndent +
                                      m_visibleNameSize.width() +
                                      iMinorSpacing;

            /* Paint right element: */
            if (!snapshotName().isEmpty())
            {
                /* Prepare variables: */
                int iSnapshotNameX = iSnapshotNameIndent;
                int iSnapshotNameY = iTopLineIndent;
                /* Paint snapshot-name: */
                paintText(/* Painter: */
                          pPainter,
                          /* Point to paint in: */
                          QPoint(iSnapshotNameX, iSnapshotNameY),
                          /* Font to paint text: */
                          m_snapshotNameFont,
                          /* Paint device: */
                          model()->paintDevice(),
                          /* Text to paint: */
                          m_strVisibleSnapshotName);
            }
        }

        /* Calculate indents: */
        int iBottomLineIndent = iTopLineIndent + iTopLineHeight;

        /* Paint bottom line: */
        {
            /* Paint left element: */
            {
                /* Prepare variables: */
                int iMachineStatePixmapX = iRightColumnIndent;
                int iMachineStatePixmapY = iBottomLineIndent;
                /* Paint state pixmap: */
                paintPixmap(/* Painter: */
                            pPainter,
                            /* Point to paint in: */
                            QPoint(iMachineStatePixmapX, iMachineStatePixmapY),
                            /* Pixmap to paint: */
                            m_statePixmap);
            }

            /* Calculate indents: */
            int iMachineStateTextIndent = iRightColumnIndent +
                                          m_statePixmapSize.width() +
                                          iMinorSpacing;

            /* Paint right element: */
            {
                /* Prepare variables: */
                int iMachineStateTextX = iMachineStateTextIndent;
                int iMachineStateTextY = iBottomLineIndent;
                /* Paint state text: */
                paintText(/* Painter: */
                          pPainter,
                          /* Point to paint in: */
                          QPoint(iMachineStateTextX, iMachineStateTextY),
                          /* Font to paint text: */
                          m_stateTextFont,
                          /* Paint device: */
                          model()->paintDevice(),
                          /* Text to paint: */
                          m_strStateText);
            }
        }
    }

    /* Tool-bar: */
    if (m_pToolBar)
    {
        /* Show/hide tool-bar: */
        if (isHovered())
        {
            if (!m_pToolBar->isVisible())
                m_pToolBar->show();
        }
        else
        {
            if (m_pToolBar->isVisible())
                m_pToolBar->hide();
        }
    }
}

void UIGChooserItemMachine::prepare()
{
    /* Tool-bar/buttons: */
    m_pToolBar = 0;
    m_pSettingsButton = 0;
    m_pStartButton = 0;
    m_pPauseButton = 0;
    m_pCloseButton = 0;

    /* Colors: */
#ifdef VBOX_WS_MAC
    m_iHighlightLightness = 115;
    m_iHoverLightness = 110;
    m_iHoverHighlightLightness = 120;
#else /* VBOX_WS_MAC */
    m_iHighlightLightness = 130;
    m_iHoverLightness = 155;
    m_iHoverHighlightLightness = 175;
#endif /* !VBOX_WS_MAC */

    /* Fonts: */
    m_nameFont = font();
    m_nameFont.setWeight(QFont::Bold);
    m_snapshotNameFont = font();
    m_stateTextFont = font();

    /* Sizes: */
    m_iFirstRowMaximumWidth = 0;
    m_iMinimumNameWidth = 0;
    m_iMaximumNameWidth = 0;
    m_iMinimumSnapshotNameWidth = 0;
    m_iMaximumSnapshotNameWidth = 0;

    /* Other things disabled for now: */
    return;

#if 0 /* disabled for now */
    /* Create tool-bar: */
    m_pToolBar = new UIGraphicsToolBar(this, 2, 2);

    /* Create buttons: */
    m_pSettingsButton = new UIGraphicsZoomButton(m_pToolBar,
                                                 data(MachineItemData_SettingsButtonPixmap).value<QIcon>(),
                                                 UIGraphicsZoomDirection_Top | UIGraphicsZoomDirection_Left);
    m_pSettingsButton->setIndent(m_pToolBar->toolBarMargin() - 1);
    m_pToolBar->insertItem(m_pSettingsButton, 0, 0);

    m_pStartButton = new UIGraphicsZoomButton(m_pToolBar,
                                              data(MachineItemData_StartButtonPixmap).value<QIcon>(),
                                              UIGraphicsZoomDirection_Top | UIGraphicsZoomDirection_Right);
    m_pStartButton->setIndent(m_pToolBar->toolBarMargin() - 1);
    m_pToolBar->insertItem(m_pStartButton, 0, 1);

    m_pPauseButton = new UIGraphicsZoomButton(m_pToolBar,
                                              data(MachineItemData_PauseButtonPixmap).value<QIcon>(),
                                              UIGraphicsZoomDirection_Bottom | UIGraphicsZoomDirection_Left);
    m_pPauseButton->setIndent(m_pToolBar->toolBarMargin() - 1);
    m_pToolBar->insertItem(m_pPauseButton, 1, 0);

    m_pCloseButton = new UIGraphicsZoomButton(m_pToolBar,
                                              data(MachineItemData_CloseButtonPixmap).value<QIcon>(),
                                              UIGraphicsZoomDirection_Bottom | UIGraphicsZoomDirection_Right);
    m_pCloseButton->setIndent(m_pToolBar->toolBarMargin() - 1);
    m_pToolBar->insertItem(m_pCloseButton, 1, 1);

    connect(m_pSettingsButton, SIGNAL(sigButtonClicked()),
            actionPool()->action(UIActionIndexST_M_Machine_S_Settings), SLOT(trigger()),
            Qt::QueuedConnection);
    connect(m_pStartButton, SIGNAL(sigButtonClicked()),
            actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow), SLOT(trigger()),
            Qt::QueuedConnection);
    connect(m_pPauseButton, SIGNAL(sigButtonClicked()),
            actionPool()->action(UIActionIndexST_M_Machine_T_Pause), SLOT(trigger()),
            Qt::QueuedConnection);
    connect(m_pCloseButton, SIGNAL(sigButtonClicked()),
            actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_PowerOff), SLOT(trigger()),
            Qt::QueuedConnection);
#endif /* disabled for now */
}

/* static */
bool UIGChooserItemMachine::contains(const QList<UIGChooserItemMachine*> &list, UIGChooserItemMachine *pItem)
{
    /* Check if passed list contains passed machine-item id: */
    foreach (UIGChooserItemMachine *pIteratedItem, list)
        if (pIteratedItem->id() == pItem->id())
            return true;
    /* Found nothing? */
    return false;
}


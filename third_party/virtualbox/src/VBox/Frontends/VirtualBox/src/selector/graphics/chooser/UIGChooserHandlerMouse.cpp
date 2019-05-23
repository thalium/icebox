/* $Id: UIGChooserHandlerMouse.cpp $ */
/** @file
 * VBox Qt GUI - UIGChooserHandlerMouse class implementation.
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
# include <QGraphicsSceneMouseEvent>

/* GUI incluedes: */
# include "UIGChooserHandlerMouse.h"
# include "UIGChooserModel.h"
# include "UIGChooserItemGroup.h"
# include "UIGChooserItemMachine.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGChooserHandlerMouse::UIGChooserHandlerMouse(UIGChooserModel *pParent)
    : QObject(pParent)
    , m_pModel(pParent)
{
}

bool UIGChooserHandlerMouse::handle(QGraphicsSceneMouseEvent *pEvent, UIMouseEventType type) const
{
    /* Process passed event: */
    switch (type)
    {
        case UIMouseEventType_Press: return handleMousePress(pEvent);
        case UIMouseEventType_Release: return handleMouseRelease(pEvent);
        case UIMouseEventType_DoubleClick: return handleMouseDoubleClick(pEvent);
    }
    /* Pass event if unknown: */
    return false;
}

UIGChooserModel* UIGChooserHandlerMouse::model() const
{
    return m_pModel;
}

bool UIGChooserHandlerMouse::handleMousePress(QGraphicsSceneMouseEvent *pEvent) const
{
    /* Get item under mouse cursor: */
    QPointF scenePos = pEvent->scenePos();
    if (QGraphicsItem *pItemUnderMouse = model()->itemAt(scenePos))
    {
        /* Which button it was? */
        switch (pEvent->button())
        {
            /* Left one? */
            case Qt::LeftButton:
            {
                /* Which item we just clicked? */
                UIGChooserItem *pClickedItem = 0;
                /* Was that a group item? */
                if (UIGChooserItemGroup *pGroupItem = qgraphicsitem_cast<UIGChooserItemGroup*>(pItemUnderMouse))
                    pClickedItem = pGroupItem;
                /* Or a machine one? */
                else if (UIGChooserItemMachine *pMachineItem = qgraphicsitem_cast<UIGChooserItemMachine*>(pItemUnderMouse))
                    pClickedItem = pMachineItem;
                /* If we had clicked one of required item types: */
                if (pClickedItem && !pClickedItem->isRoot())
                {
                    /* Was 'shift' modifier pressed? */
                    if (pEvent->modifiers() == Qt::ShiftModifier)
                    {
                        /* Calculate positions: */
                        UIGChooserItem *pFirstItem = model()->currentItem();
                        int iFirstPosition = model()->navigationList().indexOf(pFirstItem);
                        int iClickedPosition = model()->navigationList().indexOf(pClickedItem);
                        /* Populate list of items from 'first' to 'clicked': */
                        QList<UIGChooserItem*> items;
                        if (iFirstPosition <= iClickedPosition)
                            for (int i = iFirstPosition; i <= iClickedPosition; ++i)
                                items << model()->navigationList().at(i);
                        else
                            for (int i = iFirstPosition; i >= iClickedPosition; --i)
                                items << model()->navigationList().at(i);
                        /* Set that list as current: */
                        model()->setCurrentItems(items);
                        /* Move focus to clicked item: */
                        model()->setFocusItem(pClickedItem);
                    }
                    /* Was 'control' modifier pressed? */
                    else if (pEvent->modifiers() == Qt::ControlModifier)
                    {
                        /* Invert selection state for clicked item: */
                        if (model()->currentItems().contains(pClickedItem))
                            model()->removeFromCurrentItems(pClickedItem);
                        else
                            model()->addToCurrentItems(pClickedItem);
                        /* Move focus to clicked item: */
                        model()->setFocusItem(pClickedItem);
                        model()->makeSureSomeItemIsSelected();
                    }
                    /* Was no modifiers pressed? */
                    else if (pEvent->modifiers() == Qt::NoModifier)
                    {
                        /* Make clicked item the current one: */
                        model()->setCurrentItem(pClickedItem);
                    }
                }
                break;
            }
            /* Right one? */
            case Qt::RightButton:
            {
                /* Which item we just clicked? */
                UIGChooserItem *pClickedItem = 0;
                /* Was that a group item? */
                if (UIGChooserItemGroup *pGroupItem = qgraphicsitem_cast<UIGChooserItemGroup*>(pItemUnderMouse))
                    pClickedItem = pGroupItem;
                /* Or a machine one? */
                else if (UIGChooserItemMachine *pMachineItem = qgraphicsitem_cast<UIGChooserItemMachine*>(pItemUnderMouse))
                    pClickedItem = pMachineItem;
                /* If we had clicked one of required item types: */
                if (pClickedItem && !pClickedItem->isRoot())
                {
                    /* Select clicked item if not selected yet: */
                    if (!model()->currentItems().contains(pClickedItem))
                        model()->setCurrentItem(pClickedItem);
                }
                break;
            }
            default:
                break;
        }
    }
    /* Pass all other events: */
    return false;
}

bool UIGChooserHandlerMouse::handleMouseRelease(QGraphicsSceneMouseEvent*) const
{
    /* Pass all events: */
    return false;
}

bool UIGChooserHandlerMouse::handleMouseDoubleClick(QGraphicsSceneMouseEvent *pEvent) const
{
    /* Get item under mouse cursor: */
    QPointF scenePos = pEvent->scenePos();
    if (QGraphicsItem *pItemUnderMouse = model()->itemAt(scenePos))
    {
        /* Which button it was? */
        switch (pEvent->button())
        {
            /* Left one? */
            case Qt::LeftButton:
            {
                /* Was that a group item? */
                if (UIGChooserItemGroup *pGroupItem = qgraphicsitem_cast<UIGChooserItemGroup*>(pItemUnderMouse))
                {
                    /* Prepare variables: */
                    int iGroupItemWidth = pGroupItem->geometry().toRect().width();
                    int iMouseDoubleClickX = pEvent->scenePos().toPoint().x();
                    /* If it was a root: */
                    if (pGroupItem->isRoot())
                    {
                        /* Do not allow for unhovered root: */
                        if (!pGroupItem->isHovered())
                            return false;
                        /* Unindent root if possible: */
                        if (model()->root() != model()->mainRoot())
                        {
                            pGroupItem->setHovered(false);
                            model()->unindentRoot();
                        }
                    }
                    /* If it was a simple group item: */
                    else
                    {
                        /* If click was at left part: */
                        if (iMouseDoubleClickX < iGroupItemWidth / 2)
                        {
                            /* Toggle it: */
                            if (pGroupItem->isClosed())
                                pGroupItem->open();
                            else if (pGroupItem->isOpened())
                                pGroupItem->close();
                        }
                        /* If click was at right part: */
                        else
                        {
                            /* Indent root with group item: */
                            pGroupItem->setHovered(false);
                            model()->indentRoot(pGroupItem);
                        }
                    }
                    /* Filter that event out: */
                    return true;
                }
                /* Or a machine one? */
                else if (pItemUnderMouse->type() == UIGChooserItemType_Machine)
                {
                    /* Activate machine item: */
                    model()->activateMachineItem();
                }
                break;
            }
            default:
                break;
        }
    }
    /* Pass all other events: */
    return false;
}


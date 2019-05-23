/* $Id: QITreeWidget.cpp $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: QITreeWidget class implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
# include <QPainter>
# include <QResizeEvent>

/* GUI includes: */
# include "QITreeWidget.h"

/* Other VBox includes: */
# include "iprt/assert.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QAccessibleObject extension used as an accessibility interface for QITreeWidgetItem. */
class QIAccessibilityInterfaceForQITreeWidgetItem : public QAccessibleObject
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating QITreeWidgetItem accessibility interface: */
        if (pObject && strClassname == QLatin1String("QITreeWidgetItem"))
            return new QIAccessibilityInterfaceForQITreeWidgetItem(pObject);

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pObject to the base-class. */
    QIAccessibilityInterfaceForQITreeWidgetItem(QObject *pObject)
        : QAccessibleObject(pObject)
    {}

    /** Returns the parent. */
    virtual QAccessibleInterface *parent() const /* override */;

    /** Returns the number of children. */
    virtual int childCount() const /* override */;
    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */;
    /** Returns the index of the passed @a pChild. */
    virtual int indexOfChild(const QAccessibleInterface *pChild) const /* override */;

    /** Returns the rect. */
    virtual QRect rect() const /* override */;
    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */;

    /** Returns the role. */
    virtual QAccessible::Role role() const /* override */;
    /** Returns the state. */
    virtual QAccessible::State state() const /* override */;

private:

    /** Returns corresponding QITreeWidgetItem. */
    QITreeWidgetItem *item() const { return qobject_cast<QITreeWidgetItem*>(object()); }
};


/** QAccessibleWidget extension used as an accessibility interface for QITreeWidget. */
class QIAccessibilityInterfaceForQITreeWidget : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating QITreeWidget accessibility interface: */
        if (pObject && strClassname == QLatin1String("QITreeWidget"))
            return new QIAccessibilityInterfaceForQITreeWidget(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    QIAccessibilityInterfaceForQITreeWidget(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::List)
    {}

    /** Returns the number of children. */
    virtual int childCount() const /* override */;
    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */;

    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */;

private:

    /** Returns corresponding QITreeWidget. */
    QITreeWidget *tree() const { return qobject_cast<QITreeWidget*>(widget()); }
};


/*********************************************************************************************************************************
*   Class QIAccessibilityInterfaceForQITreeWidgetItem implementation.                                                            *
*********************************************************************************************************************************/

QAccessibleInterface *QIAccessibilityInterfaceForQITreeWidgetItem::parent() const
{
    /* Make sure item still alive: */
    AssertPtrReturn(item(), 0);

    /* Return the parent: */
    return item()->parentItem() ?
           QAccessible::queryAccessibleInterface(item()->parentItem()) :
           QAccessible::queryAccessibleInterface(item()->parentTree());
}

int QIAccessibilityInterfaceForQITreeWidgetItem::childCount() const
{
    /* Make sure item still alive: */
    AssertPtrReturn(item(), 0);

    /* Return the number of children: */
    return item()->childCount();
}

QAccessibleInterface *QIAccessibilityInterfaceForQITreeWidgetItem::child(int iIndex) const
{
    /* Make sure item still alive: */
    AssertPtrReturn(item(), 0);
    /* Make sure index is valid: */
    AssertReturn(iIndex >= 0 && iIndex < childCount(), 0);

    /* Return the child with the passed iIndex: */
    return QAccessible::queryAccessibleInterface(item()->childItem(iIndex));
}

int QIAccessibilityInterfaceForQITreeWidgetItem::indexOfChild(const QAccessibleInterface *pChild) const
{
    /* Search for corresponding child: */
    for (int i = 0; i < childCount(); ++i)
        if (child(i) == pChild)
            return i;

    /* -1 by default: */
    return -1;
}

QRect QIAccessibilityInterfaceForQITreeWidgetItem::rect() const
{
    /* Make sure item still alive: */
    AssertPtrReturn(item(), QRect());

    /* Compose common region: */
    QRegion region;

    /* Append item rectangle: */
    const QRect  itemRectInViewport = item()->parentTree()->visualItemRect(item());
    const QSize  itemSize           = itemRectInViewport.size();
    const QPoint itemPosInViewport  = itemRectInViewport.topLeft();
    const QPoint itemPosInScreen    = item()->parentTree()->viewport()->mapToGlobal(itemPosInViewport);
    const QRect  itemRectInScreen   = QRect(itemPosInScreen, itemSize);
    region += itemRectInScreen;

    /* Append children rectangles: */
    for (int i = 0; i < childCount(); ++i)
        region += child(i)->rect();

    /* Return common region bounding rectangle: */
    return region.boundingRect();
}

QString QIAccessibilityInterfaceForQITreeWidgetItem::text(QAccessible::Text enmTextRole) const
{
    /* Make sure item still alive: */
    AssertPtrReturn(item(), QString());

    /* Return a text for the passed enmTextRole: */
    switch (enmTextRole)
    {
        case QAccessible::Name: return item()->defaultText();
        default: break;
    }

    /* Null-string by default: */
    return QString();
}

QAccessible::Role QIAccessibilityInterfaceForQITreeWidgetItem::role() const
{
    /* Return the role of item with children: */
    if (childCount() > 0)
        return QAccessible::List;

    /* ListItem by default: */
    return QAccessible::ListItem;
}

QAccessible::State QIAccessibilityInterfaceForQITreeWidgetItem::state() const
{
    /* Make sure item still alive: */
    AssertPtrReturn(item(), QAccessible::State());

    /* Compose the state: */
    QAccessible::State state;
    state.focusable = true;
    state.selectable = true;

    /* Compose the state of current item: */
    if (   item()
        && item() == QITreeWidgetItem::toItem(item()->treeWidget()->currentItem()))
    {
        state.active = true;
        state.focused = true;
        state.selected = true;
    }

    /* Compose the state of checked item: */
    if (   item()
        && item()->checkState(0) != Qt::Unchecked)
    {
        state.checked = true;
        if (item()->checkState(0) == Qt::PartiallyChecked)
            state.checkStateMixed = true;
    }

    /* Return the state: */
    return state;
}


/*********************************************************************************************************************************
*   Class QIAccessibilityInterfaceForQITreeWidget implementation.                                                                *
*********************************************************************************************************************************/

int QIAccessibilityInterfaceForQITreeWidget::childCount() const
{
    /* Make sure tree still alive: */
    AssertPtrReturn(tree(), 0);

    /* Return the number of children: */
    return tree()->childCount();
}

QAccessibleInterface *QIAccessibilityInterfaceForQITreeWidget::child(int iIndex) const
{
    /* Make sure tree still alive: */
    AssertPtrReturn(tree(), 0);
    /* Make sure index is valid: */
    AssertReturn(iIndex >= 0, 0);
    if (iIndex >= childCount())
    {
        // WORKAROUND:
        // Normally I would assert here, but Qt5 accessibility code has
        // a hard-coded architecture for a tree-widgets which we do not like
        // but have to live with and this architecture enumerates children
        // of all levels as children of level 0, so Qt5 can try to address
        // our interface with index which surely out of bounds by our laws.
        // So let's assume that's exactly such case and try to enumerate
        // visible children like they are a part of the list, not tree.
        // printf("Invalid index: %d\n", iIndex);

        // Take into account we also have header with 'column count' indexes,
        // so we should start enumerating tree indexes since 'column count'.
        const int iColumnCount = tree()->columnCount();
        int iCurrentIndex = iColumnCount;

        // Do some sanity check as well, enough?
        AssertReturn(iIndex >= iColumnCount, 0);

        // Search for sibling with corresponding index:
        QTreeWidgetItem *pItem = tree()->topLevelItem(0);
        while (pItem && iCurrentIndex < iIndex)
        {
            ++iCurrentIndex;
            if (iCurrentIndex % iColumnCount == 0)
                pItem = tree()->itemBelow(pItem);
        }

        // Return what we found:
        // if (pItem)
        //     printf("Item found: [%s]\n", pItem->text(0).toUtf8().constData());
        // else
        //     printf("Item not found\n");
        return pItem ? QAccessible::queryAccessibleInterface(QITreeWidgetItem::toItem(pItem)) : 0;
    }

    /* Return the child with the passed iIndex: */
    return QAccessible::queryAccessibleInterface(tree()->childItem(iIndex));
}

QString QIAccessibilityInterfaceForQITreeWidget::text(QAccessible::Text /* enmTextRole */) const
{
    /* Make sure tree still alive: */
    AssertPtrReturn(tree(), QString());

    /* Return tree whats-this: */
    return tree()->whatsThis();
}


/*********************************************************************************************************************************
*   Class QITreeWidgetItem implementation.                                                                                       *
*********************************************************************************************************************************/

/* static */
QITreeWidgetItem *QITreeWidgetItem::toItem(QTreeWidgetItem *pItem)
{
    /* Make sure alive QITreeWidgetItem passed: */
    if (!pItem || pItem->type() != QITreeWidgetItem::ItemType)
        return 0;

    /* Return casted QITreeWidgetItem: */
    return static_cast<QITreeWidgetItem*>(pItem);
}

/* static */
const QITreeWidgetItem *QITreeWidgetItem::toItem(const QTreeWidgetItem *pItem)
{
    /* Make sure alive QITreeWidgetItem passed: */
    if (!pItem || pItem->type() != QITreeWidgetItem::ItemType)
        return 0;

    /* Return casted QITreeWidgetItem: */
    return static_cast<const QITreeWidgetItem*>(pItem);
}

QITreeWidgetItem::QITreeWidgetItem()
    : QTreeWidgetItem(ItemType)
{
}

QITreeWidgetItem::QITreeWidgetItem(QITreeWidget *pTreeWidget)
    : QTreeWidgetItem(pTreeWidget, ItemType)
{
}

QITreeWidgetItem::QITreeWidgetItem(QITreeWidgetItem *pTreeWidgetItem)
    : QTreeWidgetItem(pTreeWidgetItem, ItemType)
{
}

QITreeWidgetItem::QITreeWidgetItem(QITreeWidget *pTreeWidget, const QStringList &strings)
    : QTreeWidgetItem(pTreeWidget, strings, ItemType)
{
}

QITreeWidgetItem::QITreeWidgetItem(QITreeWidgetItem *pTreeWidgetItem, const QStringList &strings)
    : QTreeWidgetItem(pTreeWidgetItem, strings, ItemType)
{
}

QITreeWidget *QITreeWidgetItem::parentTree() const
{
    /* Return the parent tree if any: */
    return treeWidget() ? qobject_cast<QITreeWidget*>(treeWidget()) : 0;
}

QITreeWidgetItem *QITreeWidgetItem::parentItem() const
{
    /* Return the parent item if any: */
    return QTreeWidgetItem::parent() ? toItem(QTreeWidgetItem::parent()) : 0;
}

QITreeWidgetItem *QITreeWidgetItem::childItem(int iIndex) const
{
    /* Return the child item with iIndex if any: */
    return QTreeWidgetItem::child(iIndex) ? toItem(QTreeWidgetItem::child(iIndex)) : 0;
}

QString QITreeWidgetItem::defaultText() const
{
    /* Return 1st cell text as default: */
    return text(0);
}


/*********************************************************************************************************************************
*   Class QITreeWidget implementation.                                                                                           *
*********************************************************************************************************************************/

QITreeWidget::QITreeWidget(QWidget *pParent)
    : QTreeWidget(pParent)
{
    /* Install QITreeWidget accessibility interface factory: */
    QAccessible::installFactory(QIAccessibilityInterfaceForQITreeWidget::pFactory);
    /* Install QITreeWidgetItem accessibility interface factory: */
    QAccessible::installFactory(QIAccessibilityInterfaceForQITreeWidgetItem::pFactory);

    // WORKAROUND:
    // Ok, what do we have here..
    // There is a bug in QAccessible framework which might be just treated like
    // a functionality flaw. It consist in fact that if an accessibility client
    // is enabled, base-class can request an accessibility interface in own
    // constructor before the sub-class registers own factory, so we have to
    // recreate interface after we finished with our own initialization.
    QAccessibleInterface *pInterface = QAccessible::queryAccessibleInterface(this);
    if (pInterface)
    {
        QAccessible::deleteAccessibleInterface(QAccessible::uniqueId(pInterface));
        QAccessible::queryAccessibleInterface(this); // <= new one, proper..
    }
}

void QITreeWidget::setSizeHintForItems(const QSize &sizeHint)
{
    /* Pass the sizeHint to all the top-level items: */
    for (int i = 0; i < topLevelItemCount(); ++i)
        topLevelItem(i)->setSizeHint(0, sizeHint);
}

int QITreeWidget::childCount() const
{
    /* Return the number of children: */
    return invisibleRootItem()->childCount();
}

QITreeWidgetItem *QITreeWidget::childItem(int iIndex) const
{
    /* Return the child item with iIndex if any: */
    return invisibleRootItem()->child(iIndex) ? QITreeWidgetItem::toItem(invisibleRootItem()->child(iIndex)) : 0;
}

void QITreeWidget::paintEvent(QPaintEvent *pEvent)
{
    /* Create item painter: */
    QPainter painter;
    painter.begin(viewport());

    /* Notify listeners about painting: */
    QTreeWidgetItemIterator it(this);
    while (*it)
    {
        emit painted(*it, &painter);
        ++it;
    }

    /* Close item painter: */
    painter.end();

    /* Call to base-class: */
    QTreeWidget::paintEvent(pEvent);
}

void QITreeWidget::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QTreeWidget::resizeEvent(pEvent);

    /* Notify listeners about resizing: */
    emit resized(pEvent->size(), pEvent->oldSize());
}


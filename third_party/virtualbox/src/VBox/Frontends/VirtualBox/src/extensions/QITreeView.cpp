/* $Id: QITreeView.cpp $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: QITreeView class implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
# include <QMouseEvent>
# include <QPainter>
# include <QSortFilterProxyModel>

/* GUI includes: */
# include "QITreeView.h"

/* Other VBox includes: */
# include "iprt/assert.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QAccessibleObject extension used as an accessibility interface for QITreeViewItem. */
class QIAccessibilityInterfaceForQITreeViewItem : public QAccessibleObject
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating QITreeViewItem accessibility interface: */
        if (pObject && strClassname == QLatin1String("QITreeViewItem"))
            return new QIAccessibilityInterfaceForQITreeViewItem(pObject);

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pObject to the base-class. */
    QIAccessibilityInterfaceForQITreeViewItem(QObject *pObject)
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

    /** Returns corresponding QITreeViewItem. */
    QITreeViewItem *item() const { return qobject_cast<QITreeViewItem*>(object()); }
};


/** QAccessibleWidget extension used as an accessibility interface for QITreeView. */
class QIAccessibilityInterfaceForQITreeView : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating QITreeView accessibility interface: */
        if (pObject && strClassname == QLatin1String("QITreeView"))
            return new QIAccessibilityInterfaceForQITreeView(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    QIAccessibilityInterfaceForQITreeView(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::List)
    {}

    /** Returns the number of children. */
    virtual int childCount() const /* override */;
    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */;
    /** Returns the index of the passed @a pChild. */
    virtual int indexOfChild(const QAccessibleInterface *pChild) const /* override */;

    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */;

private:

    /** Returns corresponding QITreeView. */
    QITreeView *tree() const { return qobject_cast<QITreeView*>(widget()); }
};


/*********************************************************************************************************************************
*   Class QIAccessibilityInterfaceForQITreeViewItem implementation.                                                              *
*********************************************************************************************************************************/

QAccessibleInterface *QIAccessibilityInterfaceForQITreeViewItem::parent() const
{
    /* Sanity check: */
    AssertPtrReturn(item(), 0);

    /* Return the parent: */
    return item()->parentItem() ?
           QAccessible::queryAccessibleInterface(item()->parentItem()) :
           QAccessible::queryAccessibleInterface(item()->parentTree());
}

int QIAccessibilityInterfaceForQITreeViewItem::childCount() const
{
    /* Sanity check: */
    AssertPtrReturn(item(), 0);
    AssertPtrReturn(item()->parentTree(), 0);
    AssertPtrReturn(item()->parentTree()->model(), 0);

    /* Acquire item model-index: */
    const QModelIndex itemIndex = item()->modelIndex();

    /* Return the number of children: */
    return item()->parentTree()->model()->rowCount(itemIndex);;
}

QAccessibleInterface *QIAccessibilityInterfaceForQITreeViewItem::child(int iIndex) const /* override */
{
    /* Sanity check: */
    AssertPtrReturn(item(), 0);
    AssertPtrReturn(item()->parentTree(), 0);
    AssertPtrReturn(item()->parentTree()->model(), 0);
    AssertReturn(iIndex >= 0 && iIndex < childCount(), 0);

    /* Acquire item model-index: */
    const QModelIndex itemIndex = item()->modelIndex();
    /* Acquire child model-index: */
    const QModelIndex childIndex = itemIndex.child(iIndex, 0);

    /* Check whether we have proxy model set or source one otherwise: */
    const QSortFilterProxyModel *pProxyModel = qobject_cast<const QSortFilterProxyModel*>(item()->parentTree()->model());
    /* Acquire source child model-index, which can be the same as child model-index: */
    const QModelIndex sourceChildIndex = pProxyModel ? pProxyModel->mapToSource(childIndex) : childIndex;
    /* Acquire source child item: */
    QITreeViewItem *pItem = reinterpret_cast<QITreeViewItem*>(sourceChildIndex.internalPointer());
    /* Return the child with the passed iIndex: */
    return QAccessible::queryAccessibleInterface(pItem);
}

int QIAccessibilityInterfaceForQITreeViewItem::indexOfChild(const QAccessibleInterface *pChild) const /* override */
{
    /* Search for corresponding child: */
    for (int i = 0; i < childCount(); ++i)
        if (child(i) == pChild)
            return i;

    /* -1 by default: */
    return -1;
}

QRect QIAccessibilityInterfaceForQITreeViewItem::rect() const
{
    /* Sanity check: */
    AssertPtrReturn(item(), QRect());
    AssertPtrReturn(item()->parentTree(), QRect());
    AssertPtrReturn(item()->parentTree()->viewport(), QRect());

    /* Get the local rect: */
    const QRect  itemRectInViewport = item()->rect();
    const QSize  itemSize           = itemRectInViewport.size();
    const QPoint itemPosInViewport  = itemRectInViewport.topLeft();
    const QPoint itemPosInScreen    = item()->parentTree()->viewport()->mapToGlobal(itemPosInViewport);
    const QRect  itemRectInScreen   = QRect(itemPosInScreen, itemSize);

    /* Return the rect: */
    return itemRectInScreen;
}

QString QIAccessibilityInterfaceForQITreeViewItem::text(QAccessible::Text enmTextRole) const
{
    /* Sanity check: */
    AssertPtrReturn(item(), QString());

    /* Return a text for the passed enmTextRole: */
    switch (enmTextRole)
    {
        case QAccessible::Name: return item()->text();
        default: break;
    }

    /* Null-string by default: */
    return QString();
}

QAccessible::Role QIAccessibilityInterfaceForQITreeViewItem::role() const
{
    /* List if there are children, ListItem by default: */
    return childCount() ? QAccessible::List : QAccessible::ListItem;
}

QAccessible::State QIAccessibilityInterfaceForQITreeViewItem::state() const
{
    /* Empty state by default: */
    return QAccessible::State();
}


/*********************************************************************************************************************************
*   Class QIAccessibilityInterfaceForQITreeView implementation.                                                                  *
*********************************************************************************************************************************/

int QIAccessibilityInterfaceForQITreeView::childCount() const
{
    /* Sanity check: */
    AssertPtrReturn(tree(), 0);
    AssertPtrReturn(tree()->model(), 0);

    /* Acquire root model-index: */
    const QModelIndex rootIndex = tree()->rootIndex();

    /* Return the number of children: */
    return tree()->model()->rowCount(rootIndex);
}

QAccessibleInterface *QIAccessibilityInterfaceForQITreeView::child(int iIndex) const
{
    /* Sanity check: */
    AssertPtrReturn(tree(), 0);
    AssertReturn(iIndex >= 0, 0);
    if (iIndex >= childCount())
    {
        // WORKAROUND:
        // Normally I would assert here, but Qt5 accessibility code has
        // a hard-coded architecture for a tree-views which we do not like
        // but have to live with and this architecture enumerates children
        // of all levels as children of level 0, so Qt5 can try to address
        // our interface with index which surely out of bounds by our laws.
        // So let's assume that's exactly such case and try to enumerate
        // visible children like they are a part of the list, not tree.
        // printf("Invalid index: %d\n", iIndex);

        // Sanity check:
        AssertPtrReturn(tree()->model(), 0);

        // Take into account we also have header with 'column count' indexes,
        // so we should start enumerating tree indexes since 'column count'.
        const int iColumnCount = tree()->model()->columnCount();
        int iCurrentIndex = iColumnCount;

        // Set iterator to root model-index initially:
        QModelIndex index = tree()->rootIndex();
        // But if it has child, go deeper:
        if (index.child(0, 0).isValid())
            index = index.child(0, 0);

        // Search for sibling with corresponding index:
        while (index.isValid() && iCurrentIndex < iIndex)
        {
            ++iCurrentIndex;
            if (iCurrentIndex % iColumnCount == 0)
                index = tree()->indexBelow(index);
        }

        // Check whether we have proxy model set or source one otherwise:
        const QSortFilterProxyModel *pProxyModel = qobject_cast<const QSortFilterProxyModel*>(tree()->model());
        // Acquire source model-index, which can be the same as model-index:
        const QModelIndex sourceIndex = pProxyModel ? pProxyModel->mapToSource(index) : index;

        // Return what we found:
        // if (sourceIndex.isValid())
        //     printf("Item found: [%s]\n", ((QITreeViewItem*)sourceIndex.internalPointer())->text().toUtf8().constData());
        // else
        //     printf("Item not found\n");
        return sourceIndex.isValid() ? QAccessible::queryAccessibleInterface((QITreeViewItem*)sourceIndex.internalPointer()) : 0;
    }

    /* Acquire root model-index: */
    const QModelIndex rootIndex = tree()->rootIndex();
    /* Acquire child model-index: */
    const QModelIndex childIndex = rootIndex.child(iIndex, 0);

    /* Check whether we have proxy model set or source one otherwise: */
    const QSortFilterProxyModel *pProxyModel = qobject_cast<const QSortFilterProxyModel*>(tree()->model());
    /* Acquire source child model-index, which can be the same as child model-index: */
    const QModelIndex sourceChildIndex = pProxyModel ? pProxyModel->mapToSource(childIndex) : childIndex;
    /* Acquire source child item: */
    QITreeViewItem *pItem = reinterpret_cast<QITreeViewItem*>(sourceChildIndex.internalPointer());
    /* Return the child with the passed iIndex: */
    return QAccessible::queryAccessibleInterface(pItem);
}

int QIAccessibilityInterfaceForQITreeView::indexOfChild(const QAccessibleInterface *pChild) const
{
    /* Search for corresponding child: */
    for (int i = 0; i < childCount(); ++i)
        if (child(i) == pChild)
            return i;

    /* -1 by default: */
    return -1;
}

QString QIAccessibilityInterfaceForQITreeView::text(QAccessible::Text /* enmTextRole */) const
{
    /* Sanity check: */
    AssertPtrReturn(tree(), QString());

    /* Return tree whats-this: */
    return tree()->whatsThis();
}


/*********************************************************************************************************************************
*   Class QITreeViewItem implementation.                                                                                         *
*********************************************************************************************************************************/

QRect QITreeViewItem::rect() const
{
    /* Redirect call to parent-tree: */
    return parentTree() ? parentTree()->visualRect(modelIndex()) : QRect();
}

QModelIndex QITreeViewItem::modelIndex() const
{
    /* Acquire model: */
    const QAbstractItemModel *pModel = parentTree()->model();
    /* Check whether we have proxy model set or source one otherwise: */
    const QSortFilterProxyModel *pProxyModel = qobject_cast<const QSortFilterProxyModel*>(pModel);

    /* Acquire root model-index: */
    const QModelIndex rootIndex = parentTree()->rootIndex();
    /* Acquire source root model-index, which can be the same as root model-index: */
    const QModelIndex sourceRootModelIndex = pProxyModel ? pProxyModel->mapToSource(rootIndex) : rootIndex;

    /* Check whether we have root model-index here: */
    if (   sourceRootModelIndex.internalPointer()
        && sourceRootModelIndex.internalPointer() == this)
        return rootIndex;

    /* Determine our parent model-index: */
    const QModelIndex parentIndex = parentItem() ? parentItem()->modelIndex() : rootIndex;

    /* Determine our position inside parent: */
    int iPositionInParent = -1;
    for (int i = 0; i < pModel->rowCount(parentIndex); ++i)
    {
        /* Acquire child model-index: */
        const QModelIndex childIndex = parentIndex.child(i, 0);
        /* Acquire source child model-index, which can be the same as child model-index: */
        const QModelIndex sourceChildModelIndex = pProxyModel ? pProxyModel->mapToSource(childIndex) : childIndex;

        /* Check whether we have child model-index here: */
        if (   sourceChildModelIndex.internalPointer()
            && sourceChildModelIndex.internalPointer() == this)
        {
            iPositionInParent = i;
            break;
        }
    }
    /* Make sure we found something: */
    if (iPositionInParent == -1)
        return QModelIndex();

    /* Return model-index as child of parent model-index: */
    return parentIndex.child(iPositionInParent, 0);
}


/*********************************************************************************************************************************
*   Class QITreeView implementation.                                                                                             *
*********************************************************************************************************************************/

QITreeView::QITreeView(QWidget *pParent)
    : QTreeView(pParent)
{
    /* Prepare all: */
    prepare();
}

void QITreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    /* Notify listeners about it: */
    emit currentItemChanged(current, previous);
    /* Call to base-class: */
    QTreeView::currentChanged(current, previous);
}

void QITreeView::drawBranches(QPainter *pPainter, const QRect &rect, const QModelIndex &index) const
{
    /* Notify listeners about it: */
    emit drawItemBranches(pPainter, rect, index);
    /* Call to base-class: */
    QTreeView::drawBranches(pPainter, rect, index);
}

void QITreeView::mouseMoveEvent(QMouseEvent *pEvent)
{
    /* Reject event initially: */
    pEvent->setAccepted(false);
    /* Notify listeners about event allowing them to handle it: */
    emit mouseMoved(pEvent);
    /* Call to base-class only if event was not yet accepted: */
    if (!pEvent->isAccepted())
        QTreeView::mouseMoveEvent(pEvent);
}

void QITreeView::mousePressEvent(QMouseEvent *pEvent)
{
    /* Reject event initially: */
    pEvent->setAccepted(false);
    /* Notify listeners about event allowing them to handle it: */
    emit mousePressed(pEvent);
    /* Call to base-class only if event was not yet accepted: */
    if (!pEvent->isAccepted())
        QTreeView::mousePressEvent(pEvent);
}

void QITreeView::mouseDoubleClickEvent(QMouseEvent *pEvent)
{
    /* Reject event initially: */
    pEvent->setAccepted(false);
    /* Notify listeners about event allowing them to handle it: */
    emit mouseDoubleClicked(pEvent);
    /* Call to base-class only if event was not yet accepted: */
    if (!pEvent->isAccepted())
        QTreeView::mouseDoubleClickEvent(pEvent);
}

void QITreeView::prepare()
{
    /* Install QITreeViewItem accessibility interface factory: */
    QAccessible::installFactory(QIAccessibilityInterfaceForQITreeViewItem::pFactory);
    /* Install QITreeView accessibility interface factory: */
    QAccessible::installFactory(QIAccessibilityInterfaceForQITreeView::pFactory);

    /* Mark header hidden: */
    setHeaderHidden(true);
    /* Mark root hidden: */
    setRootIsDecorated(false);
}


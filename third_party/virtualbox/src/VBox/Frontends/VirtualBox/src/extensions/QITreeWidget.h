/* $Id: QITreeWidget.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QITreeWidget class implementation.
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

#ifndef ___QITreeWidget_h___
#define ___QITreeWidget_h___

/* Qt includes: */
#include <QTreeWidget>
#include <QTreeWidgetItem>

/* Forward declarations: */
class QITreeWidget;


/** QTreeWidgetItem subclass extending standard functionality. */
class QITreeWidgetItem : public QObject, public QTreeWidgetItem
{
    Q_OBJECT;

public:

    /** Item type for QITreeWidgetItem. */
    enum { ItemType = QTreeWidgetItem::UserType + 1 };

    /** Casts QTreeWidgetItem* to QITreeWidgetItem* if possible. */
    static QITreeWidgetItem *toItem(QTreeWidgetItem *pItem);
    /** Casts const QTreeWidgetItem* to const QITreeWidgetItem* if possible. */
    static const QITreeWidgetItem *toItem(const QTreeWidgetItem *pItem);

    /** Constructs item. */
    QITreeWidgetItem();

    /** Constructs item passing @a pTreeWidget into the base-class. */
    QITreeWidgetItem(QITreeWidget *pTreeWidget);
    /** Constructs item passing @a pTreeWidgetItem into the base-class. */
    QITreeWidgetItem(QITreeWidgetItem *pTreeWidgetItem);

    /** Constructs item passing @a pTreeWidget and @a strings into the base-class. */
    QITreeWidgetItem(QITreeWidget *pTreeWidget, const QStringList &strings);
    /** Constructs item passing @a pTreeWidgetItem and @a strings into the base-class. */
    QITreeWidgetItem(QITreeWidgetItem *pTreeWidgetItem, const QStringList &strings);

    /** Returns the parent tree-widget. */
    QITreeWidget *parentTree() const;
    /** Returns the parent tree-widget item. */
    QITreeWidgetItem *parentItem() const;

    /** Returns the child tree-widget item with @a iIndex. */
    QITreeWidgetItem *childItem(int iIndex) const;

    /** Returns default text. */
    virtual QString defaultText() const;
};


/** QTreeWidget subclass extending standard functionality. */
class QITreeWidget : public QTreeWidget
{
    Q_OBJECT;

signals:

    /** Notifies about particular tree-widget @a pItem is painted with @a pPainter. */
    void painted(QTreeWidgetItem *pItem, QPainter *pPainter);
    /** Notifies about tree-widget being resized from @a oldSize to @a size. */
    void resized(const QSize &size, const QSize &oldSize);

public:

    /** Constructs tree-widget passing @a pParent to the base-class. */
    QITreeWidget(QWidget *pParent = 0);

    /** Defines @a sizeHint for tree-widget items. */
    void setSizeHintForItems(const QSize &sizeHint);

    /** Returns the number of children. */
    int childCount() const;
    /** Returns the child item with @a iIndex. */
    QITreeWidgetItem *childItem(int iIndex) const;

protected:

    /** Handles paint @a pEvent. */
    void paintEvent(QPaintEvent *pEvent);
    /** Handles resize @a pEvent. */
    void resizeEvent(QResizeEvent *pEvent);
};

#endif /* !___QITreeWidget_h___ */


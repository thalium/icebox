/* $Id: QITableView.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QITableView class declaration.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QITableView_h___
#define ___QITableView_h___

/* Qt includes: */
#include <QTableView>

/* Forward declarations: */
class QITableViewCell;
class QITableViewRow;
class QITableView;


/** OObject subclass used as cell for the QITableView. */
class QITableViewCell : public QObject
{
    Q_OBJECT;

public:

    /** Constructs table-view cell for passed @a pParent. */
    QITableViewCell(QITableViewRow *pParent)
        : m_pRow(pParent)
    {}

    /** Defines the parent @a pRow reference. */
    void setRow(QITableViewRow *pRow) { m_pRow = pRow; }
    /** Returns the parent row reference. */
    QITableViewRow *row() const { return m_pRow; }

    /** Returns the cell text. */
    virtual QString text() const = 0;

private:

    /** Holds the parent row reference. */
    QITableViewRow *m_pRow;
};


/** OObject subclass used as row for the QITableView. */
class QITableViewRow : public QObject
{
    Q_OBJECT;

public:

    /** Constructs table-view row for passed @a pParent. */
    QITableViewRow(QITableView *pParent)
        : m_pTable(pParent)
    {}

    /** Defines the parent @a pTable reference. */
    void setTable(QITableView *pTable) { m_pTable = pTable; }
    /** Returns the parent table reference. */
    QITableView *table() const { return m_pTable; }

    /** Returns the number of children. */
    virtual int childCount() const = 0;
    /** Returns the child item with @a iIndex. */
    virtual QITableViewCell *childItem(int iIndex) const = 0;

private:

    /** Holds the parent table reference. */
    QITableView *m_pTable;
};


/** QTableView subclass extending standard functionality. */
class QITableView : public QTableView
{
    Q_OBJECT;

signals:

    /** Notifies listeners about index changed from @a previous to @a current. */
    void sigCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

public:

    /** Constructs table-view passing @a pParent to the base-class. */
    QITableView(QWidget *pParent = 0);
    /** Destructs table-view. */
    virtual ~QITableView() /* override */;

    /** Returns the number of children. */
    virtual int childCount() const { return 0; }
    /** Returns the child item with @a iIndex. */
    virtual QITableViewRow *childItem(int /* iIndex */) const { return 0; }

    /** Makes sure current editor data committed. */
    void makeSureEditorDataCommitted();

protected slots:

    /** Stores the created @a pEditor for passed @a index in the map. */
    virtual void sltEditorCreated(QWidget *pEditor, const QModelIndex &index);
    /** Clears the destoyed @a pEditor from the map. */
    virtual void sltEditorDestroyed(QObject *pEditor);

protected:

    /** Handles index change from @a previous to @a current. */
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous) /* override */;

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Holds the map of editors stored for passed indexes. */
    QMap<QModelIndex, QObject*> m_editors;
};

#endif /* !___QITableView_h___ */


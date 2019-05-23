/* $Id: UIBootTable.h $ */
/** @file
 * VBox Qt GUI - UIBootTable class declaration.
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

#ifndef __UIBootTable_h__
#define __UIBootTable_h__

/* Qt includes: */
#include <QListWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "COMEnums.h"

class UIBootTableItem : public QListWidgetItem
{
public:

    UIBootTableItem(KDeviceType type);

    KDeviceType type() const;

    void retranslateUi();

private:

    /* Private member vars */
    KDeviceType m_type;

};

class UIBootTable : public QIWithRetranslateUI<QListWidget>
{
    Q_OBJECT;

public:

    UIBootTable(QWidget *pParent = 0);

    void adjustSizeToFitContent();

public slots:

    void sltMoveItemUp();
    void sltMoveItemDown();

signals:

    void sigRowChanged(int);

protected:

    void retranslateUi();
    void dropEvent(QDropEvent *pEvent);
    QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    QModelIndex moveItemTo(const QModelIndex &index, int row);
};

#endif // __UIBootTable_h__


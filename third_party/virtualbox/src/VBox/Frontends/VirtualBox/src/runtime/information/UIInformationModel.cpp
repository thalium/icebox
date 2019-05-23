/* $Id: UIInformationModel.cpp $ */
/** @file
 * VBox Qt GUI - UIInformationModel class implementation.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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

/* GUI includes: */
# include "UIInformationModel.h"
# include "UIInformationDataItem.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIInformationModel::UIInformationModel(QObject *pParent, const CMachine &machine, const CConsole &console)
    : QAbstractListModel(pParent)
    , m_machine(machine)
    , m_console(console)
{
    /* Prepare: */
    prepare();
}

UIInformationModel::~UIInformationModel()
{
    /* Cleanup: */
    cleanup();
}

int UIInformationModel::rowCount(const QModelIndex & /* parentIndex */) const
{
    /* Return row-count: */
    return m_list.count();
}

QVariant UIInformationModel::data(const QModelIndex &index, int role) const
{
    /* Get item at the row: */
    UIInformationDataItem *pItem = m_list.at(index.row());
    /* Return the data for the corresponding role: */
    return pItem->data(index, role);
}

void UIInformationModel::addItem(UIInformationDataItem *pItem)
{
    /* Make sure item is valid: */
    AssertPtrReturnVoid(pItem);
    /* Add item: */
    m_list.append(pItem);
}

void UIInformationModel::updateData(const QModelIndex &index)
{
    /* Emit data-changed signal: */
    emit dataChanged(index, index);
}

void UIInformationModel::updateData(UIInformationDataItem *pItem)
{
    /* Update data: */
    AssertPtrReturnVoid(pItem);
    int iRow = m_list.indexOf(pItem);
    QModelIndex index = createIndex(iRow, 0);
    emit dataChanged(index, index);
}

void UIInformationModel::prepare()
{
    /* Prepare role-names for model: */
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "";
    roleNames[Qt::DecorationRole] = "";
    roleNames[Qt::UserRole + 1] = "";
    roleNames[Qt::UserRole + 2] = "";

    /* Register meta-type: */
    qRegisterMetaType<InformationElementType>();
}

void UIInformationModel::cleanup()
{
    /* Destroy all data-items: */
    qDeleteAll(m_list);
    m_list.clear();
}

QHash<int, QByteArray> UIInformationModel::roleNames() const
{
    /* Add supported roles and return: */
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "";
    roleNames[Qt::DecorationRole] = "";
    roleNames[Qt::UserRole + 1] = "";
    roleNames[Qt::UserRole + 2] = "";
    return roleNames;
}


/* $Id: UIInformationView.cpp $ */
/** @file
 * VBox Qt GUI - UIInformationView class implementation.
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

/* Qt includes: */
# include <QClipboard>
# include <QTextEdit>

/* GUI includes: */
# include "UIInformationView.h"
# include "UIInformationItem.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIInformationView::UIInformationView(QWidget *pParent)
    : QListView(pParent)
{
    // WORKAROUND:
    // Create a dummy text-edit for copying rich-text
    // as manual copying to clipboard is not working:
    m_pTextEdit = new QTextEdit(this);
    m_pTextEdit->setVisible(false);
    /* Set selection mode: */
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    /* Set scrolling mode to per pixel: */
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void UIInformationView::updateData(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    /* Iterate through all indexes: */
    for (int iRowIndex = topLeft.row(); iRowIndex <= bottomRight.row(); ++iRowIndex)
    {
        /* Get the index for current row: */
        const QModelIndex index = topLeft.sibling(iRowIndex, topLeft.column());
        /* If index is valid: */
        if (index.isValid())
        {
            /* Get the row-count of data-table: */
            const int iCount = index.data(Qt::UserRole + 1).value<UITextTable>().count();
            /* If there is no data hide the item: */
            if (iCount == 0)
                setRowHidden(index.row(), true);
        }
    }
}

void UIInformationView::keyPressEvent(QKeyEvent *pEvent)
{
    /* Copy the text: */
    if (pEvent == QKeySequence::Copy)
    {
        QString strText;
        /* Get selection model: */
        QItemSelectionModel *pSelectionModel = selectionModel();
        if (pSelectionModel)
        {
            /* Check all the selected-indexes and copy the text: */
            foreach (const QModelIndex &index, pSelectionModel->selectedIndexes())
            {
                UIInformationItem *pItem = dynamic_cast<UIInformationItem*>(itemDelegate(index));
                if (pItem)
                {
                    /* Update the corresponding data: */
                    pItem->updateData(index);
                    /* Get and add the html-data of item: */
                    strText.append(pItem->htmlData());
                }
            }
        }
        /* Set the text to text-edit and copy from it: */
        m_pTextEdit->setText(strText);
        m_pTextEdit->selectAll();
        m_pTextEdit->copy();
        /* Accept/acknowledge event: */
        pEvent->accept();
    }
    /* Call to base-class: */
    else
        QListView::keyPressEvent(pEvent);
}


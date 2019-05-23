/* $Id: UIInformationItem.cpp $ */
/** @file
 * VBox Qt GUI - UIInformationItem class definition.
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
# include <QApplication>
# include <QPainter>
# include <QTextDocument>
# include <QUrl>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIIconPool.h"
# include "UIInformationItem.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIInformationItem::UIInformationItem(QObject *pParent)
    : QStyledItemDelegate(pParent)
{
    /* Create text-document: */
    m_pTextDocument = new QTextDocument(this);
    AssertPtrReturnVoid(m_pTextDocument);

    /* Dummy initialization of icon-string (to avoid assertion in icon-pool when model is empty): */
    m_strIcon = ":/machine_16px.png";
}

void UIInformationItem::setIcon(const QString &strIcon) const
{
    /* Cache icon: */
    m_strIcon = strIcon;
    /* Update text-layout: */
    updateTextLayout();
}

void UIInformationItem::setName(const QString &strName) const
{
    /* Cache name: */
    m_strName = strName;
    /* Update text-layout: */
    updateTextLayout();
}

const UITextTable &UIInformationItem::text() const
{
    /* Return text: */
    return m_text;
}

void UIInformationItem::setText(const UITextTable &text) const
{
    /* Clear text: */
    m_text.clear();

    /* For each line of the passed table: */
    foreach (const UITextTableLine &line, text)
    {
        /* Lines: */
        const QString strLeftLine = line.first;
        const QString strRightLine = line.second;

        /* If 2nd line is NOT empty: */
        if (!strRightLine.isEmpty())
        {
            /* Take both lines 'as is': */
            m_text << UITextTableLine(strLeftLine, strRightLine);
        }
        /* If 2nd line is empty: */
        else
        {
            /* Parse the 1st one to sub-lines: */
            QStringList subLines = strLeftLine.split(QRegExp("\\n"));
            /* Parse sub-lines: */
            foreach (const QString &strSubLine, subLines)
                m_text << UITextTableLine(strSubLine, QString());
        }
    }

    /* Update text-layout: */
    updateTextLayout();
}

void UIInformationItem::updateData(const QModelIndex &index) const
{
    /* Set name: */
    setName(index.data().toString());
    /* Set icon: */
    setIcon(index.data(Qt::DecorationRole).toString());
    /* Set text: */
    setText(index.data(Qt::UserRole + 1).value<UITextTable>());
    /* Get type of the item: */
    m_type = index.data(Qt::UserRole + 2).value<InformationElementType>();
}

QString UIInformationItem::htmlData() const
{
    /* Return html-data: */
    return m_pTextDocument->toHtml();
}

void UIInformationItem::paint(QPainter *pPainter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    /* Save the painter: */
    pPainter->save();
    /* Update data: */
    updateData(index);
    /* If there is something to paint: */
    if (!m_text.isEmpty())
    {
        /* Draw item as per application style: */
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, pPainter);
        pPainter->resetTransform();
        pPainter->translate(option.rect.topLeft());

        /* Draw the content of text-document: */
        m_pTextDocument->drawContents(pPainter);
    }
    /* Restore the painter: */
    pPainter->restore();
}

QSize UIInformationItem::sizeHint(const QStyleOptionViewItem & /* option */, const QModelIndex &index) const
{
    /* Update data: */
    updateData(index);
    if (m_text.isEmpty())
        return QSize(0, 0);
    /* Return size: */
    return m_pTextDocument->size().toSize();
}

void UIInformationItem::updateTextLayout() const
{
    /* Details templates: */
    static const char *sSectionBoldTpl =
        "<tr><td width=%1 rowspan=%2 align=left><img src=\"image://%3\" /></td>"
            "<td><b><nobr>%4</nobr></b></td></tr>"
            "%5";
    static const char *sSectionItemTpl2 =
        "<tr><td width=200><nobr>%1</nobr></td><td/><td>%2</td></tr>";
    const QString &sectionTpl = sSectionBoldTpl;

    /* Initialize icon tag: */
    const QString strIconTag = QString("image://%1").arg(m_strIcon);

    /* Get icon metric: */
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

    /* Compose details report: */
    QString report;
    {
        QString item;
        /* Parse lines from text and add it to text: */
        foreach (const UITextTableLine &line, m_text)
            item = item + QString(sSectionItemTpl2).arg(line.first, line.second);

        /* Format the entire item: */
        report = sectionTpl.arg(1.375 * iIconMetric)
                           .arg(m_text.count() + 1) /* rows */
                           .arg(m_strIcon, /* icon */
                                m_strName, /* title */
                                item); /* items */
    }

    /* Add pixmap to text-document as image resource: */
    m_pTextDocument->addResource(QTextDocument::ImageResource, QUrl(strIconTag),
                                 UIIconPool::iconSet(m_strIcon).pixmap(iIconMetric, iIconMetric));

    /* Set html-data: */
    m_pTextDocument->setHtml(report);
}


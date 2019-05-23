/* $Id: UIInformationItem.h $ */
/** @file
 * VBox Qt GUI - UIInformationItem class declaration.
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

#ifndef ___UIInformationItem_h___
#define ___UIInformationItem_h___

/* Qt includes: */
#include <QStyledItemDelegate>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIExtraDataDefs.h"
#include "UIGDetailsItem.h"

/* Forward declarations: */
class QStyleOptionViewItem;
class QTextDocument;

/* Type definitions: */
typedef QPair<QString, QString> UITextTableLine;
typedef QList<UITextTableLine> UITextTable;

Q_DECLARE_METATYPE(UITextTable);


/** QStyledItemDelegate extension
  * providing GUI with delegate implementation for information-view in session-information window. */
class UIInformationItem : public QStyledItemDelegate
{
    Q_OBJECT;

public:

    /** Constructs information-item passing @a pParent to the base-class. */
    UIInformationItem(QObject *pParent = 0);

    /** Defines the icon of information-item as @a icon. */
    void setIcon(const QString &icon) const;

    /** Defines the name of information-item as @a strName. */
    void setName(const QString &strName) const;

    /** Returns the text-data of information-item. */
    const UITextTable &text() const;
    /** Defines the text-data of information-item as @a text. */
    void setText(const UITextTable &text) const;

    /** Updates data for information-item with @a index. */
    void updateData(const QModelIndex &index) const;

    /** Returns html data. */
    QString htmlData() const;

protected:

    /** Performs painting for @a index using @a pPainter and @a option set. */
    virtual void paint(QPainter *pPainter, const QStyleOptionViewItem &option, const QModelIndex &index) const /* override */;

    /** Calculates size-hint for @a index using @a option set. */
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const /* override */;

private:

    /** Updates text-layout. */
    void updateTextLayout() const;

    /** Holds the pixmap of information-item. */
    mutable QString m_strIcon;

    /** Holds the name of information-item. */
    mutable QString m_strName;

    /** Holds the text-data of information-item. */
    mutable UITextTable m_text;

    /** Holds the text-data of information-item. */
    mutable InformationElementType m_type;

    /** Holds the instance of text-dcoument we create. */
    QTextDocument *m_pTextDocument;
};

#endif /* !___UIInformationItem_h___ */


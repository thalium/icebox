/* $Id: QIFlowLayout.h $ */
/** @file
 * VBox Qt GUI - QIFlowLayout class declaration.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QIFlowLayout_h___
#define ___QIFlowLayout_h___

/* Qt includes: */
#include <QLayout>
#include <QStyle>


/** QLayout extension providing GUI with the possibility to build flow-layout.
  * This kind of horizonal layout can wrap children down to the next line (row)
  * performing calculations on the basis of layout size and children size-hints.
  * It is also takes into account that some of the children can be expandable
  * horizontally allowing them to grow up to all the available width. */
class QIFlowLayout : public QLayout
{
    Q_OBJECT;

    /** Layout item expand policy. */
    enum ExpandPolicy
    {
        ExpandPolicy_Fixed,
        ExpandPolicy_Dynamic
    };

    /** Layout item data. */
    struct LayoutData
    {
        /** Constructs layout item data on the bases of passed @a pItem.
          * @param  enmPolicy  Brings the layout item expand policy.
          * @param  iWidth     Brings the layout item desired width. */
        LayoutData(QLayoutItem *pItem, ExpandPolicy enmPolicy, int iWidth)
            : item(pItem), policy(enmPolicy), width(iWidth)
        {}

        /** Holds the layout item. */
        QLayoutItem *item;
        /** Holds the layout item expand policy. */
        ExpandPolicy policy;
        /** Holds the layout item desired width. */
        int width;
    };
    /** Layout item data list. */
    typedef QList<LayoutData> LayoutDataList;
    /** Layout item data table. */
    typedef QList<LayoutDataList> LayoutDataTable;

public:

    /** Constructs flow-layout passing @a pParent to the base-class.
      * @param  iMargin    Brings the layout contents margin.
      * @param  iSpacingH  Brings the layout horizontal spacing.
      * @param  iSpacingV  Brings the layout vertical spacing. */
    QIFlowLayout(QWidget *pParent, int iMargin = -1, int iSpacingH = -1, int iSpacingV = -1);

    /** Constructs flow-layout.
      * @param  iMargin    Brings the layout contents margin.
      * @param  iSpacingH  Brings the layout horizontal spacing.
      * @param  iSpacingV  Brings the layout vertical spacing. */
    QIFlowLayout(int iMargin = -1, int iSpacingH = -1, int iSpacingV = -1);

    /** Destructs flow-layout. */
    virtual ~QIFlowLayout() /* override */;

    /** Returns the number of layout items. */
    virtual int count() const /* override */;
    /** Adds @a pItem into layout. */
    virtual void addItem(QLayoutItem *pItem) /* override */;
    /** Returns the layout item at passed @a iIndex. */
    virtual QLayoutItem *itemAt(int iIndex) const /* override */;
    /** Removes the layout item at passed @a iIndex and returns it. */
    virtual QLayoutItem *takeAt(int index) /* override */;

    /** Returns whether this layout can make use of more space than sizeHint().
      * A value of Qt::Vertical or Qt::Horizontal means that it wants to grow in only one dimension,
      * whereas Qt::Vertical | Qt::Horizontal means that it wants to grow in both dimensions. */
    virtual Qt::Orientations expandingDirections() const /* override */;

    /** Returns whether this layout's preferred height depends on its width. */
    virtual bool hasHeightForWidth() const /* override */;
    /** Returns the preferred height for this layout item, given the width. */
    virtual int heightForWidth(int) const /* override */;

    /** Returns the minimum layout size. */
    virtual QSize minimumSize() const /* override */;
    /** Returns this item's preferred size. */
    virtual QSize sizeHint() const /* override */;

    /** Defines this item's geometry to @a rect. */
    virtual void setGeometry(const QRect &rect) /* override */;

private:

    /** Recalculates layout on the basis of passed @a rect.
      * Adjusts layout items if @a fDoLayout is true.
      * @returns recalculated layout height. */
    int relayout(const QRect &rect, bool fDoLayout) const;

    /** Returns smart spacing based on parent if present. */
    int smartSpacing(QStyle::PixelMetric pm) const;
    /** Returns horizontal spacing. */
    int horizontalSpacing() const;
    /** Returns vertical spacing. */
    int verticalSpacing() const;

    /** Holds the layout item list. */
    QList<QLayoutItem *> m_items;

    /** Holds the horizontal spacing. */
    int m_iSpacingH;
    /** Holds the vertical spacing. */
    int m_iSpacingV;
};

#endif /* !___QIFlowLayout_h___ */


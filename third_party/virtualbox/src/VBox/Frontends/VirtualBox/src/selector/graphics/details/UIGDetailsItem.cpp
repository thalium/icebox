/* $Id: UIGDetailsItem.cpp $ */
/** @file
 * VBox Qt GUI - UIGDetailsItem class definition.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
# include <QAccessibleObject>
# include <QApplication>
# include <QPainter>
# include <QGraphicsScene>
# include <QStyleOptionGraphicsItem>

/* GUI includes: */
# include "UIGraphicsTextPane.h"
# include "UIGDetailsGroup.h"
# include "UIGDetailsSet.h"
# include "UIGDetailsElement.h"
# include "UIGDetailsModel.h"
# include "UIGDetailsView.h"
# include "UIGDetails.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QAccessibleObject extension used as an accessibility interface for Details-view items. */
class UIAccessibilityInterfaceForUIGDetailsItem : public QAccessibleObject
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating Details-view accessibility interface: */
        if (pObject && strClassname == QLatin1String("UIGDetailsItem"))
            return new UIAccessibilityInterfaceForUIGDetailsItem(pObject);

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pObject to the base-class. */
    UIAccessibilityInterfaceForUIGDetailsItem(QObject *pObject)
        : QAccessibleObject(pObject)
    {}

    /** Returns the parent. */
    virtual QAccessibleInterface *parent() const /* override */
    {
        /* Make sure item still alive: */
        AssertPtrReturn(item(), 0);

        /* Return the parent: */
        switch (item()->type())
        {
            /* For a set: */
            case UIGDetailsItemType_Set:
            {
                /* Always return parent view: */
                return QAccessible::queryAccessibleInterface(item()->model()->details()->view());
            }
            /* For an element: */
            case UIGDetailsItemType_Element:
            {
                /* What amount of children root has? */
                const int cChildCount = item()->model()->root()->items().size();

                /* Return our parent (if root has many of children): */
                if (cChildCount > 1)
                    return QAccessible::queryAccessibleInterface(item()->parentItem());

                /* Return parent view (otherwise): */
                return QAccessible::queryAccessibleInterface(item()->model()->details()->view());
            }
            default:
                break;
        }

        /* Null by default: */
        return 0;
    }

    /** Returns the number of children. */
    virtual int childCount() const /* override */
    {
        /* Make sure item still alive: */
        AssertPtrReturn(item(), 0);

        /* Return the number of children: */
        switch (item()->type())
        {
            case UIGDetailsItemType_Set:     return item()->items().size();
            case UIGDetailsItemType_Element: return item()->toElement()->text().size();
            default: break;
        }

        /* Zero by default: */
        return 0;
    }

    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */
    {
        /* Make sure item still alive: */
        AssertPtrReturn(item(), 0);
        /* Make sure index is valid: */
        AssertReturn(iIndex >= 0 && iIndex < childCount(), 0);

        /* Return the child with the iIndex: */
        switch (item()->type())
        {
            case UIGDetailsItemType_Set:     return QAccessible::queryAccessibleInterface(item()->items().at(iIndex));
            case UIGDetailsItemType_Element: return QAccessible::queryAccessibleInterface(&item()->toElement()->text()[iIndex]);
            default: break;
        }

        /* Null be default: */
        return 0;
    }

    /** Returns the index of the passed @a pChild. */
    virtual int indexOfChild(const QAccessibleInterface *pChild) const /* override */
    {
        /* Search for corresponding child: */
        for (int i = 0; i < childCount(); ++i)
            if (child(i) == pChild)
                return i;

        /* -1 by default: */
        return -1;
    }

    /** Returns the rect. */
    virtual QRect rect() const /* override */
    {
        /* Now goes the mapping: */
        const QSize   itemSize         = item()->size().toSize();
        const QPointF itemPosInScene   = item()->mapToScene(QPointF(0, 0));
        const QPoint  itemPosInView    = item()->model()->details()->view()->mapFromScene(itemPosInScene);
        const QPoint  itemPosInScreen  = item()->model()->details()->view()->mapToGlobal(itemPosInView);
        const QRect   itemRectInScreen = QRect(itemPosInScreen, itemSize);
        return itemRectInScreen;
    }

    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */
    {
        /* Make sure item still alive: */
        AssertPtrReturn(item(), QString());

        /* Return the description: */
        if (enmTextRole == QAccessible::Description)
            return item()->description();

        /* Null-string by default: */
        return QString();
    }

    /** Returns the role. */
    virtual QAccessible::Role role() const /* override */
    {
        /* Return the role: */
        return QAccessible::List;
    }

    /** Returns the state. */
    virtual QAccessible::State state() const /* override */
    {
        /* Return the state: */
        return QAccessible::State();
    }

private:

    /** Returns corresponding Details-view item. */
    UIGDetailsItem *item() const { return qobject_cast<UIGDetailsItem*>(object()); }
};


UIGDetailsItem::UIGDetailsItem(UIGDetailsItem *pParent)
    : QIWithRetranslateUI4<QIGraphicsWidget>(pParent)
    , m_pParent(pParent)
{
    /* Install Details-view item accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUIGDetailsItem::pFactory);

    /* Basic item setup: */
    setOwnedByLayout(false);
    setAcceptDrops(false);
    setFocusPolicy(Qt::NoFocus);
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    /* Non-root item? */
    if (parentItem())
    {
        /* Non-root item setup: */
        setAcceptHoverEvents(true);
    }

    /* Setup connections: */
    connect(this, SIGNAL(sigBuildStep(QString, int)),
            this, SLOT(sltBuildStep(QString, int)), Qt::QueuedConnection);
}

UIGDetailsGroup* UIGDetailsItem::toGroup()
{
    UIGDetailsGroup *pItem = qgraphicsitem_cast<UIGDetailsGroup*>(this);
    AssertMsg(pItem, ("Trying to cast invalid item type to UIGDetailsGroup!"));
    return pItem;
}

UIGDetailsSet* UIGDetailsItem::toSet()
{
    UIGDetailsSet *pItem = qgraphicsitem_cast<UIGDetailsSet*>(this);
    AssertMsg(pItem, ("Trying to cast invalid item type to UIGDetailsSet!"));
    return pItem;
}

UIGDetailsElement* UIGDetailsItem::toElement()
{
    UIGDetailsElement *pItem = qgraphicsitem_cast<UIGDetailsElement*>(this);
    AssertMsg(pItem, ("Trying to cast invalid item type to UIGDetailsElement!"));
    return pItem;
}

UIGDetailsModel* UIGDetailsItem::model() const
{
    UIGDetailsModel *pModel = qobject_cast<UIGDetailsModel*>(QIGraphicsWidget::scene()->parent());
    AssertMsg(pModel, ("Incorrect graphics scene parent set!"));
    return pModel;
}

UIGDetailsItem* UIGDetailsItem::parentItem() const
{
    return m_pParent;
}

void UIGDetailsItem::updateGeometry()
{
    /* Call to base-class: */
    QIGraphicsWidget::updateGeometry();

    /* Do the same for the parent: */
    if (parentItem())
        parentItem()->updateGeometry();
}

QSizeF UIGDetailsItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint /* = QSizeF() */) const
{
    /* If Qt::MinimumSize or Qt::PreferredSize requested: */
    if (which == Qt::MinimumSize || which == Qt::PreferredSize)
        /* Return wrappers: */
        return QSizeF(minimumWidthHint(), minimumHeightHint());
    /* Call to base-class: */
    return QIGraphicsWidget::sizeHint(which, constraint);
}

void UIGDetailsItem::sltBuildStep(QString, int)
{
    AssertMsgFailed(("This item doesn't support building!"));
}

/* static */
void UIGDetailsItem::configurePainterShape(QPainter *pPainter,
                                           const QStyleOptionGraphicsItem *pOption,
                                           int iRadius)
{
    /* Rounded corners? */
    if (iRadius)
    {
        /* Setup clipping: */
        QPainterPath roundedPath;
        roundedPath.addRoundedRect(pOption->rect, iRadius, iRadius);
        pPainter->setRenderHint(QPainter::Antialiasing);
        pPainter->setClipPath(roundedPath);
    }
}

/* static */
void UIGDetailsItem::paintFrameRect(QPainter *pPainter, const QRect &rect, int iRadius)
{
    pPainter->save();
    QPalette pal = QApplication::palette();
    QColor base = pal.color(QPalette::Active, QPalette::Window);
    pPainter->setPen(base.darker(160));
    if (iRadius)
        pPainter->drawRoundedRect(rect, iRadius, iRadius);
    else
        pPainter->drawRect(rect);
    pPainter->restore();
}

/* static */
void UIGDetailsItem::paintPixmap(QPainter *pPainter, const QRect &rect, const QPixmap &pixmap)
{
    pPainter->drawPixmap(rect, pixmap);
}

/* static */
void UIGDetailsItem::paintText(QPainter *pPainter, QPoint point,
                               const QFont &font, QPaintDevice *pPaintDevice,
                               const QString &strText, const QColor &color)
{
    /* Prepare variables: */
    QFontMetrics fm(font, pPaintDevice);
    point += QPoint(0, fm.ascent());

    /* Draw text: */
    pPainter->save();
    pPainter->setFont(font);
    pPainter->setPen(color);
    pPainter->drawText(point, strText);
    pPainter->restore();
}

UIBuildStep::UIBuildStep(QObject *pParent, QObject *pBuildObject, const QString &strStepId, int iStepNumber)
    : QObject(pParent)
    , m_strStepId(strStepId)
    , m_iStepNumber(iStepNumber)
{
    /* Prepare connections: */
    connect(pBuildObject, SIGNAL(sigBuildDone()), this, SLOT(sltStepDone()), Qt::QueuedConnection);
    connect(this, SIGNAL(sigStepDone(QString, int)), pParent, SLOT(sltBuildStep(QString, int)), Qt::QueuedConnection);
}

void UIBuildStep::sltStepDone()
{
    emit sigStepDone(m_strStepId, m_iStepNumber);
}


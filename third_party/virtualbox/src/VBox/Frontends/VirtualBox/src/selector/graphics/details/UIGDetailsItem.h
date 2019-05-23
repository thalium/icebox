/* $Id: UIGDetailsItem.h $ */
/** @file
 * VBox Qt GUI - UIGDetailsItem class declaration.
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

#ifndef __UIGDetailsItem_h__
#define __UIGDetailsItem_h__

/* GUI includes: */
#include "QIGraphicsWidget.h"
#include "QIWithRetranslateUI.h"

/* Forward declaration: */
class UIGDetailsModel;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class UIGDetailsGroup;
class UIGDetailsSet;
class UIGDetailsElement;

/* UIGDetailsItem types: */
enum UIGDetailsItemType
{
    UIGDetailsItemType_Any     = QGraphicsItem::UserType,
    UIGDetailsItemType_Group   = QGraphicsItem::UserType + 1,
    UIGDetailsItemType_Set     = QGraphicsItem::UserType + 2,
    UIGDetailsItemType_Element = QGraphicsItem::UserType + 3,
    UIGDetailsItemType_Preview = QGraphicsItem::UserType + 10
};

/* Details item interface
 * for graphics details model/view architecture: */
class UIGDetailsItem : public QIWithRetranslateUI4<QIGraphicsWidget>
{
    Q_OBJECT;

signals:

    /* Notifiers: Build stuff: */
    void sigBuildStep(QString strStepId, int iStepNumber);
    void sigBuildDone();

public:

    /* Constructor: */
    UIGDetailsItem(UIGDetailsItem *pParent);

    /* API: Cast stuff: */
    UIGDetailsGroup* toGroup();
    UIGDetailsSet* toSet();
    UIGDetailsElement* toElement();

    /* API: Model stuff: */
    UIGDetailsModel* model() const;

    /* API: Parent stuff: */
    UIGDetailsItem* parentItem() const;

    /** Returns the description of the item. */
    virtual QString description() const = 0;

    /* API: Children stuff: */
    virtual void addItem(UIGDetailsItem *pItem) = 0;
    virtual void removeItem(UIGDetailsItem *pItem) = 0;
    virtual QList<UIGDetailsItem*> items(UIGDetailsItemType type = UIGDetailsItemType_Any) const = 0;
    virtual bool hasItems(UIGDetailsItemType type = UIGDetailsItemType_Any) const = 0;
    virtual void clearItems(UIGDetailsItemType type = UIGDetailsItemType_Any) = 0;

    /* API: Layout stuff: */
    void updateGeometry();
    virtual int minimumWidthHint() const = 0;
    virtual int minimumHeightHint() const = 0;
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
    virtual void updateLayout() = 0;

protected slots:

    /* Handler: Build stuff: */
    virtual void sltBuildStep(QString strStepId, int iStepNumber);

protected:

    /* Helper: Translate stuff: */
    void retranslateUi() {}

    /* Helpers: Paint stuff: */
    static void configurePainterShape(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, int iRadius);
    static void paintFrameRect(QPainter *pPainter, const QRect &rect, int iRadius);
    static void paintPixmap(QPainter *pPainter, const QRect &rect, const QPixmap &pixmap);
    static void paintText(QPainter *pPainter, QPoint point,
                          const QFont &font, QPaintDevice *pPaintDevice,
                          const QString &strText, const QColor &color);

private:

    /* Variables: */
    UIGDetailsItem *m_pParent;
};

/* Allows to build item content synchronously: */
class UIBuildStep : public QObject
{
    Q_OBJECT;

signals:

    /* Notifier: Build stuff: */
    void sigStepDone(QString strStepId, int iStepNumber);

public:

    /* Constructor: */
    UIBuildStep(QObject *pParent, QObject *pBuildObject, const QString &strStepId, int iStepNumber);

private slots:

    /* Handler: Build stuff: */
    void sltStepDone();

private:

    /* Variables: */
    QString m_strStepId;
    int m_iStepNumber;
};

#endif /* __UIGDetailsItem_h__ */


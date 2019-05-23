/* $Id: UIGraphicsButton.cpp $ */
/** @file
 * VBox Qt GUI - UIGraphicsButton class definition.
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
# include <QApplication>
# include <QPainter>
# include <QStyle>
# include <QGraphicsSceneMouseEvent>

/* GUI includes: */
# include "UIGraphicsButton.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGraphicsButton::UIGraphicsButton(QIGraphicsWidget *pParent, const QIcon &icon)
    : QIGraphicsWidget(pParent)
    , m_icon(icon)
    , m_fParentSelected(false)
{
    /* Refresh finally: */
    refresh();
}

void UIGraphicsButton::setParentSelected(bool fParentSelected)
{
    if (m_fParentSelected == fParentSelected)
        return;
    m_fParentSelected = fParentSelected;
    update();
}

QVariant UIGraphicsButton::data(int iKey) const
{
    switch (iKey)
    {
        case GraphicsButton_Margin:
            return 0;
        case GraphicsButton_IconSize:
        {
            const int iMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
            return QSize(iMetric, iMetric);
        }
        case GraphicsButton_Icon:
            return m_icon;
        default: break;
    }
    return QVariant();
}

QSizeF UIGraphicsButton::sizeHint(Qt::SizeHint which, const QSizeF &constraint /* = QSizeF() */) const
{
    /* Calculations for minimum size: */
    if (which == Qt::MinimumSize)
    {
        /* Variables: */
        int iMargin = data(GraphicsButton_Margin).toInt();
        QSize iconSize = data(GraphicsButton_IconSize).toSize();
        /* Calculations: */
        int iWidth = 2 * iMargin + iconSize.width();
        int iHeight = 2 * iMargin + iconSize.height();
        return QSize(iWidth, iHeight);
    }
    /* Call to base-class: */
    return QIGraphicsWidget::sizeHint(which, constraint);
}

void UIGraphicsButton::paint(QPainter *pPainter, const QStyleOptionGraphicsItem* /* pOption */, QWidget* /* pWidget = 0 */)
{
    /* Prepare variables: */
    const int iMargin = data(GraphicsButton_Margin).toInt();
    const QIcon icon = data(GraphicsButton_Icon).value<QIcon>();
    const QSize expectedIconSize = data(GraphicsButton_IconSize).toSize();
    const QPixmap pixmap = icon.pixmap(expectedIconSize);
    const QSize actualIconSize = pixmap.size() / pixmap.devicePixelRatio();
    QPoint position = QPoint(iMargin, iMargin);
    if (actualIconSize != expectedIconSize)
    {
        const int iDx = (expectedIconSize.width() - actualIconSize.width()) / 2;
        const int iDy = (expectedIconSize.height() - actualIconSize.height()) / 2;
        position += QPoint(iDx, iDy);
    }

    /* Just draw the pixmap: */
    pPainter->drawPixmap(position, pixmap);
}

void UIGraphicsButton::mousePressEvent(QGraphicsSceneMouseEvent *pEvent)
{
    /* Accepting this event allows to get release-event: */
    pEvent->accept();
}

void UIGraphicsButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent)
{
    /* Call to base-class: */
    QIGraphicsWidget::mouseReleaseEvent(pEvent);
    /* Notify listeners about button click: */
    emit sigButtonClicked();
}

void UIGraphicsButton::refresh()
{
    /* Refresh geometry: */
    updateGeometry();
    /* Resize to minimum size: */
    resize(minimumSizeHint());
}


/* $Id: UIGraphicsButton.h $ */
/** @file
 * VBox Qt GUI - UIGraphicsButton class declaration.
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

#ifndef __UIGraphicsButton_h__
#define __UIGraphicsButton_h__

/* Qt includes: */
#include <QIcon>

/* GUI includes: */
#include "QIGraphicsWidget.h"

/* Forward declarations: */
class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;
class QPropertyAnimation;

/* Graphics-button representation: */
class UIGraphicsButton : public QIGraphicsWidget
{
    Q_OBJECT;

signals:

    /* Notify listeners about button was clicked: */
    void sigButtonClicked();

public:

    /* Constructor: */
    UIGraphicsButton(QIGraphicsWidget *pParent, const QIcon &icon);

    /* API: Parent stuff: */
    void setParentSelected(bool fParentSelected);

protected:

    /* Data enumerator: */
    enum GraphicsButton
    {
        GraphicsButton_Margin,
        GraphicsButton_IconSize,
        GraphicsButton_Icon
    };

    /* Data provider: */
    virtual QVariant data(int iKey) const;

    /* Size hint stuff: */
    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

    /* Paint stuff: */
    virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = 0);

    /* Mouse handlers: */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvent);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent);

    /* Helpers: Update stuff: */
    virtual void refresh();

private:

    /* Variables: */
    QIcon m_icon;
    bool m_fParentSelected;
};

#endif /* __UIGraphicsButton_h__ */


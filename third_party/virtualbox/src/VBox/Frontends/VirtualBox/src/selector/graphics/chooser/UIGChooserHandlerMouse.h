/* $Id: UIGChooserHandlerMouse.h $ */
/** @file
 * VBox Qt GUI - UIGChooserHandlerMouse class declaration.
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

#ifndef __UIGChooserHandlerMouse_h__
#define __UIGChooserHandlerMouse_h__

/* Qt includes: */
#include <QObject>

/* Forward declarations: */
class UIGChooserModel;
class QGraphicsSceneMouseEvent;
class UIGChooserItem;

/* Mouse event type: */
enum UIMouseEventType
{
    UIMouseEventType_Press,
    UIMouseEventType_Release,
    UIMouseEventType_DoubleClick
};

/* Mouse handler for graphics selector: */
class UIGChooserHandlerMouse : public QObject
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIGChooserHandlerMouse(UIGChooserModel *pParent);

    /* API: Model mouse-event handler delegate: */
    bool handle(QGraphicsSceneMouseEvent *pEvent, UIMouseEventType type) const;

private:

    /* API: Model wrapper: */
    UIGChooserModel* model() const;

    /* Helpers: Model mouse-event handler delegates: */
    bool handleMousePress(QGraphicsSceneMouseEvent *pEvent) const;
    bool handleMouseRelease(QGraphicsSceneMouseEvent *pEvent) const;
    bool handleMouseDoubleClick(QGraphicsSceneMouseEvent *pEvent) const;

    /* Variables: */
    UIGChooserModel *m_pModel;
};

#endif /* __UIGChooserHandlerMouse_h__ */


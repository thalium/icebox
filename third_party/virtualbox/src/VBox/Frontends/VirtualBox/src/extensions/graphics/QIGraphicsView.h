/* $Id: QIGraphicsView.h $ */
/** @file
 * VBox Qt GUI - QIGraphicsView class declaration.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QIGraphicsView_h___
#define ___QIGraphicsView_h___

/* Qt includes: */
#include <QGraphicsView>

/** General QGraphicsView extension. */
class QIGraphicsView : public QGraphicsView
{
    Q_OBJECT;

public:

    /** Constructor, passes @a pParent to the QGraphicsView constructor. */
    QIGraphicsView(QWidget *pParent = 0);

protected:

    /** General @a pEvent handler. */
    bool event(QEvent *pEvent);

private:

    /** Holds the vertical scroll-bar position. */
    int m_iVerticalScrollBarPosition;
};

#endif /* !___QIGraphicsView_h___ */


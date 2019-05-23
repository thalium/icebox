/* $Id: QIGraphicsWidget.h $ */
/** @file
 * VBox Qt GUI - QIGraphicsWidget class declaration.
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

#ifndef __QIGraphicsWidget_h__
#define __QIGraphicsWidget_h__

/* Qt includes: */
#include <QGraphicsWidget>

/* Graphics widget extension: */
class QIGraphicsWidget : public QGraphicsWidget
{
    Q_OBJECT;

public:

    /* Constructor: */
    QIGraphicsWidget(QGraphicsWidget *pParent = 0);

    /* API: Size-hint stuff: */
    virtual QSizeF minimumSizeHint() const;
};

#endif /* __QIGraphicsWidget_h__ */


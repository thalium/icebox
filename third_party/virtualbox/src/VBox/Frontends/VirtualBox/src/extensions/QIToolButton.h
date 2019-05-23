/* $Id: QIToolButton.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIToolButton class declaration.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __QIToolButton_h__
#define __QIToolButton_h__

/* Global includes: */
#include <QToolButton>

/* QToolButton reimplementation: */
class QIToolButton: public QToolButton
{
    Q_OBJECT;

public:

    QIToolButton(QWidget *pParent = 0)
        : QToolButton(pParent)
    {
#ifdef VBOX_WS_MAC
        /* Keep size-hint alive: */
        const QSize sh = sizeHint();
        setStyleSheet("QToolButton { border: 0px none black; margin: 0px 0px 0px 0px; } QToolButton::menu-indicator {image: none;}");
        setFixedSize(sh);
#else /* !VBOX_WS_MAC */
        setAutoRaise(true);
#endif /* !VBOX_WS_MAC */
    }

    /** Sets the auto-raise status. */
    virtual void setAutoRaise(bool fEnable)
    {
#ifdef VBOX_WS_MAC
        /* Ignore for Mac OS X: */
        Q_UNUSED(fEnable);
#else /* !VBOX_WS_MAC */
        /* Call to base-class: */
        QToolButton::setAutoRaise(fEnable);
#endif /* !VBOX_WS_MAC */
    }

    void removeBorder()
    {
        setStyleSheet("QToolButton { border: 0px }");
    }
};

#endif /* __QIToolButton_h__ */


/* $Id: QISplitter.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QISplitter class declaration.
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

#ifndef _QISplitter_h_
#define _QISplitter_h_

/* Global includes */
#include <QSplitter>

class QISplitter : public QSplitter
{
    Q_OBJECT;

public:

    enum Type { Native, Shade };

    QISplitter(QWidget *pParent = 0);
    QISplitter(Qt::Orientation orientation, QWidget *pParent = 0);

    void setHandleType(Type type) { m_type = type; }
    Type handleType() const { return m_type; }

    void configureColors(const QColor &color1, const QColor &color2) { m_color1 = color1; m_color2 = color2; }

protected:

    bool eventFilter(QObject *pWatched, QEvent *pEvent);
    void showEvent(QShowEvent *pEvent);

    QSplitterHandle* createHandle();

private:

    QByteArray m_baseState;

    bool m_fPolished;
    Type m_type;
#ifdef VBOX_WS_MAC
    bool m_fHandleGrabbed;
#endif /* VBOX_WS_MAC */

    QColor m_color1;
    QColor m_color2;
};

#endif /* _QISplitter_h_ */


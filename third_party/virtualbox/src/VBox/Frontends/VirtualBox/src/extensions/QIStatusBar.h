/* $Id: QIStatusBar.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIStatusBar class declaration.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QIStatusBar_h___
#define ___QIStatusBar_h___

/* Qt includes: */
#include <QStatusBar>


/** QStatusBar subclass extending standard functionality. */
class QIStatusBar : public QStatusBar
{
    Q_OBJECT;

public:

    /** Constructs status-bar passing @a pParent to the base-class. */
    QIStatusBar(QWidget *pParent = 0);

protected slots:

    /** Remembers the last status @a strMessage. */
    void sltRememberLastMessage(const QString &strMessage) { m_strMessage = strMessage; }

protected:

    /** Holds the last status message. */
    QString m_strMessage;
};

#endif /* !___QIStatusBar_h___ */


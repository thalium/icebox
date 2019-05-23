/* $Id: UIMenuBar.h $ */
/** @file
 * VBox Qt GUI - UIMenuBar class declaration.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIMenuBar_h___
#define ___UIMenuBar_h___

/* Qt includes: */
#include <QMenuBar>

/** QMenuBar extension
  * which reflects BETA label when necessary. */
class UIMenuBar: public QMenuBar
{
    Q_OBJECT;

public:

    /** Constructor, passes @a pParent to the QMenuBar constructor. */
    UIMenuBar(QWidget *pParent = 0);

protected:

    /** Paint event handler. */
    void paintEvent(QPaintEvent *pEvent);

private:

    /** Reflects whether we should show BETA label or not. */
    bool m_fShowBetaLabel;
};

#endif /* !___UIMenuBar_h___ */

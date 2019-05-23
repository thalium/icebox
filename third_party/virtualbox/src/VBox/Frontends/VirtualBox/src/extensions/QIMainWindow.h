/* $Id: QIMainWindow.h $ */
/** @file
 * VBox Qt GUI - QIMainWindow class declaration.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QIMainWindow_h___
#define ___QIMainWindow_h___

/* Qt includes: */
#include <QMainWindow>

/** QMainWindow extension providing GUI
  * with the extended geometry management support. */
class QIMainWindow : public QMainWindow
{
    Q_OBJECT;

public:

    /** Constructs main window passing @a pParent and @a enmFlags to base-class. */
    QIMainWindow(QWidget *pParent = 0, Qt::WindowFlags enmFlags = 0);

protected:

    /** Returns whether the window should be maximized when geometry being restored. */
    virtual bool shouldBeMaximized() const { return false; }

    /** Restores the cached window geometry. */
    void restoreGeometry();

    /** Holds the cached window geometry. */
    QRect m_geometry;
};

#endif /* ___QIMainWindow_h___ */


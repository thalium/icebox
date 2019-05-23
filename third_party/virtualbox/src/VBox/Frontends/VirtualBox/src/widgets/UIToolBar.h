/* $Id: UIToolBar.h $ */
/** @file
 * VBox Qt GUI - UIToolBar class declaration.
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

#ifndef ___UIToolBar_h___
#define ___UIToolBar_h___

/* Qt includes: */
#include <QToolBar>

/* Forward declarations: */
class QMainWindow;

/** QToolBar extension
  * with few settings presets. */
class UIToolBar : public QToolBar
{
    Q_OBJECT;

public:

    /** Constructor, passes @a pParent to the QToolBar constructor. */
    UIToolBar(QWidget *pParent = 0);

    /** Defines whether tool-bar should use text-labels.
      * Default value if @a false. */
    void setUseTextLabels(bool fEnable);

#ifdef VBOX_WS_MAC
    /** Mac OS X: Defines whether native tool-bar should be used. */
    void enableMacToolbar();
    /** Mac OS X: Defines whether native tool-bar button should be shown. */
    void setShowToolBarButton(bool fShow);
    /** Mac OS X: Updates native tool-bar layout. */
    void updateLayout();
#endif /* VBOX_WS_MAC */

private:

    /** Prepare routine. */
    void prepare();

    /** Holds the parent main-window isntance. */
    QMainWindow *m_pMainWindow;
};

#endif /* !___UIToolBar_h___ */

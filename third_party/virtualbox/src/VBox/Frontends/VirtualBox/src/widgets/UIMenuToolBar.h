/* $Id: UIMenuToolBar.h $ */
/** @file
 * VBox Qt GUI - UIMenuToolBar class declaration.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIMenuToolBar_h___
#define ___UIMenuToolBar_h___

/* Qt includes: */
#include <QWidget>

/* Forward declarations: */
class UIMenuToolBarPrivate;


/** QWidget wrapper for UIToolBar extension
  * holding single drop-down menu of actions. */
class UIMenuToolBar : public QWidget
{
    Q_OBJECT;

public:

    /** Menu toolbar alignment types. */
    enum AlignmentType
    {
        AlignmentType_TopLeft,
        AlignmentType_TopRight,
        AlignmentType_BottomLeft,
        AlignmentType_BottomRight,
    };

    /** Constructs menu-toolbar wrapper. */
    UIMenuToolBar(QWidget *pParent = 0);

    /** Defines toolbar alignment @a enmType. */
    void setAlignmentType(AlignmentType enmType);

    /** Defines toolbar icon @a size. */
    void setIconSize(const QSize &size);

    /** Defines toolbar menu action. */
    void setMenuAction(QAction *pAction);

    /** Defines toolbar tool button @a enmStyle. */
    void setToolButtonStyle(Qt::ToolButtonStyle enmStyle);

    /** Returns toolbar widget for passed @a pAction. */
    QWidget *widgetForAction(QAction *pAction) const;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the menu-toolbar instance. */
    UIMenuToolBarPrivate *m_pToolbar;
};

#endif /* !___UIMenuToolBar_h___ */


/* $Id: UIToolsToolbar.h $ */
/** @file
 * VBox Qt GUI - UIToolsToolbar class declaration.
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

#ifndef ___UIToolsToolbar_h___
#define ___UIToolsToolbar_h___

/* Qt includes: */
#include <QMap>
#include <QUuid>
#include <QWidget>

/* GUI includes: */
#include "UIToolsPaneGlobal.h"
#include "UIToolsPaneMachine.h"

/* Forward declarations: */
class QAction;
class QHBoxLayout;
class QUuid;
class QWidget;
class UIActionPool;
class UITabBar;
class UIToolBar;


/** Tools toolbar imlementation for Selector UI. */
class UIToolsToolbar : public QWidget
{
    Q_OBJECT;

signals:

    /** Notifies listeners about Machine tab-bar should be shown. */
    void sigShowTabBarMachine();
    /** Notifies listeners about Global tab-bar should be shown. */
    void sigShowTabBarGlobal();

    /** Notifies listeners about Machine tool of particular @a enmType opened. */
    void sigToolOpenedMachine(const ToolTypeMachine enmType);
    /** Notifies listeners about Global tool of particular @a enmType opened. */
    void sigToolOpenedGlobal(const ToolTypeGlobal enmType);

    /** Notifies listeners about Machine tool of particular @a enmType closed. */
    void sigToolClosedMachine(const ToolTypeMachine enmType);
    /** Notifies listeners about Global tool of particular @a enmType closed. */
    void sigToolClosedGlobal(const ToolTypeGlobal enmType);

public:

    /** Constructs Tools toolbar passing @a pParent to the base-class.
      * @param  pActionPool  Brings the action-pool to take corresponding actions from. */
    UIToolsToolbar(UIActionPool *pActionPool, QWidget *pParent = 0);

    /** Defines the tab-bars to control. */
    void setTabBars(UITabBar *pTabBarMachine, UITabBar *pTabBarGlobal);

    /** Defines toolbar tool button @a enmStyle. */
    void setToolButtonStyle(Qt::ToolButtonStyle enmStyle);

    /** Returns Machine tab-bar order. */
    QList<ToolTypeMachine> tabOrderMachine() const;
    /** Returns Global tab-bar order. */
    QList<ToolTypeGlobal> tabOrderGlobal() const;

private slots:

    /** Handles request to open Machine tool. */
    void sltHandleOpenToolMachine();
    /** Handles request to open Global tool. */
    void sltHandleOpenToolGlobal();

    /** Handles request to close Machine tool with passed @a uuid. */
    void sltHandleCloseToolMachine(const QUuid &uuid);
    /** Handles request to close Global tool with passed @a uuid. */
    void sltHandleCloseToolGlobal(const QUuid &uuid);

    /** Handles request to make Machine tool with passed @a uuid current one. */
    void sltHandleToolChosenMachine(const QUuid &uuid);
    /** Handles request to make Global tool with passed @a uuid current one. */
    void sltHandleToolChosenGlobal(const QUuid &uuid);

    /** Handles action toggle. */
    void sltHandleActionToggle();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares menu. */
    void prepareMenu();
    /** Prepares widgets. */
    void prepareWidgets();

    /** Holds the action pool reference. */
    UIActionPool *m_pActionPool;

    /** Holds the Machine tab-bar instance. */
    UITabBar *m_pTabBarMachine;
    /** Holds the Global tab-bar instance. */
    UITabBar *m_pTabBarGlobal;

    /** Holds the main layout instance. */
    QHBoxLayout *m_pLayoutMain;
    /** Holds the toolbar instance. */
    UIToolBar   *m_pToolBar;

    /** Holds the map of opened Machine tool IDs. */
    QMap<ToolTypeMachine, QUuid>  m_mapTabIdsMachine;
    /** Holds the map of opened Global tool IDs. */
    QMap<ToolTypeGlobal, QUuid>   m_mapTabIdsGlobal;
};

#endif /* !___UIToolsToolbar_h___ */


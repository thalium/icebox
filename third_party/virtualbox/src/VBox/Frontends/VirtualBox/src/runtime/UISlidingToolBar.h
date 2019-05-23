/* $Id: UISlidingToolBar.h $ */
/** @file
 * VBox Qt GUI - UISlidingToolBar class declaration.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UISlidingToolBar_h___
#define ___UISlidingToolBar_h___

/* Qt includes: */
#include <QWidget>

/* Forward declarations: */
class QHBoxLayout;
class UIAnimation;

/** QWidget reimplementation
  * providing GUI with slideable tool-bar. */
class UISlidingToolBar : public QWidget
{
    Q_OBJECT;
    Q_PROPERTY(QRect widgetGeometry READ widgetGeometry WRITE setWidgetGeometry);
    Q_PROPERTY(QRect startWidgetGeometry READ startWidgetGeometry);
    Q_PROPERTY(QRect finalWidgetGeometry READ finalWidgetGeometry);

signals:

    /** Notifies about window shown. */
    void sigShown();
    /** Commands window to expand. */
    void sigExpand();
    /** Commands window to collapse. */
    void sigCollapse();

public:

    /** Possible positions. */
    enum Position
    {
        Position_Top,
        Position_Bottom
    };

    /** Constructor, passes @a pParentWidget to the QWidget constructor.
      * @param pParentWidget is used to get parent-widget geoemtry,
      * @param pIndentWidget is used to get indent-widget geometry,
      * @param pChildWidget  brings child-widget to be injected into tool-bar. */
    UISlidingToolBar(QWidget *pParentWidget, QWidget *pIndentWidget, QWidget *pChildWidget, Position position);

private slots:

    /** Performs window activation. */
    void sltActivateWindow() { activateWindow(); }

    /** Marks window as expanded. */
    void sltMarkAsExpanded() { m_fExpanded = true; }
    /** Marks window as collapsed. */
    void sltMarkAsCollapsed() { close(); m_fExpanded = false; }

    /** Handles parent geometry change. */
    void sltParentGeometryChanged(const QRect &parentRect);

private:

    /** Prepare routine. */
    void prepare();
    /** Prepare contents routine. */
    void prepareContents();
    /** Prepare geometry routine. */
    void prepareGeometry();
    /** Prepare animation routine. */
    void prepareAnimation();

    /** Update geometry. */
    void adjustGeometry();
    /** Updates animation. */
    void updateAnimation();

    /** Show event handler. */
    virtual void showEvent(QShowEvent *pEvent);
    /** Close event handler. */
    virtual void closeEvent(QCloseEvent *pEvent);
#ifdef VBOX_WS_MAC
    /** Common event handler. */
    virtual bool event(QEvent *pEvent);
#endif /* VBOX_WS_MAC */

    /** Defines sub-window geometry. */
    void setWidgetGeometry(const QRect &rect);
    /** Returns sub-window geometry. */
    QRect widgetGeometry() const;
    /** Returns sub-window start-geometry. */
    QRect startWidgetGeometry() const { return m_startWidgetGeometry; }
    /** Returns sub-window final-geometry. */
    QRect finalWidgetGeometry() const { return m_finalWidgetGeometry; }

    /** @name Geometry
      * @{ */
        /** Holds the tool-bar position. */
        const Position m_position;
        /** Holds the cached parent-widget geometry. */
        QRect m_parentRect;
        /** Holds the cached indent-widget geometry. */
        QRect m_indentRect;
    /** @} */

    /** @name Geometry: Animation
      * @{ */
        /** Holds the expand/collapse animation instance. */
        UIAnimation *m_pAnimation;
        /** Holds whether window is expanded. */
        bool m_fExpanded;
        /** Holds sub-window start-geometry. */
        QRect m_startWidgetGeometry;
        /** Holds sub-window final-geometry. */
        QRect m_finalWidgetGeometry;
    /** @} */

    /** @name Contents
      * @{ */
        /** Holds the main-layout instance. */
        QHBoxLayout *m_pMainLayout;
        /** Holds the area instance. */
        QWidget *m_pArea;
        /** Holds the child-widget reference. */
        QWidget *m_pWidget;
    /** @} */
};

#endif /* !___UISlidingToolBar_h___ */

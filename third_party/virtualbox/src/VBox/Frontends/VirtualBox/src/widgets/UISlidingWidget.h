/* $Id: UISlidingWidget.h $ */
/** @file
 * VBox Qt GUI - UISlidingWidget class declaration.
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

#ifndef ___UISlidingWidget_h___
#define ___UISlidingWidget_h___

/* Qt includes: */
#include <QWidget>

/* Forward declarations: */
class QHBoxLayout;
class QRect;
class QWidget;
class UIAnimation;


/** Some kind of splitter which allows to switch between
  * two widgets using horizontal sliding animation. */
class UISlidingWidget : public QWidget
{
    Q_OBJECT;
    Q_PROPERTY(QRect widgetGeometry READ widgetGeometry WRITE setWidgetGeometry);
    Q_PROPERTY(QRect startWidgetGeometry READ startWidgetGeometry);
    Q_PROPERTY(QRect finalWidgetGeometry READ finalWidgetGeometry);

signals:

    /** Commands to move animation forward. */
    void sigForward();
    /** Commands to move animation backward. */
    void sigBackward();

public:

    /** Constructs sliding widget passing @a pParent to the base-class. */
    UISlidingWidget(QWidget *pParent = 0);

    /** Holds the minimum widget size. */
    virtual QSize minimumSizeHint() const /* pverride */;

    /** Defines @a pWidget1 and @a pWidget2. */
    void setWidgets(QWidget *pWidget1, QWidget *pWidget2);

    /** Moves animation forward. */
    void moveForward() { emit sigForward(); }
    /** Moves animation backward. */
    void moveBackward() { emit sigBackward(); }

protected:

    /** Handles any Qt @a pEvent. */
    virtual bool event(QEvent *pEvent) /* override */;

    /** Handles resize @a pEvent. */
    virtual void resizeEvent(QResizeEvent *pEvent) /* override */;

private slots:

    /** Marks state as start. */
    void sltSetStateToStart() { m_fStateIsFinal = false; }
    /** Marks state as final. */
    void sltSetStateToFinal() { m_fStateIsFinal = true; }

private:

    /** Prepares all. */
    void prepare();

    /** Updates animation. */
    void updateAnimation();

    /** Defines sub-window geometry. */
    void setWidgetGeometry(const QRect &rect);
    /** Returns sub-window geometry. */
    QRect widgetGeometry() const;
    /** Returns sub-window start-geometry. */
    QRect startWidgetGeometry() const { return m_startWidgetGeometry; }
    /** Returns sub-window final-geometry. */
    QRect finalWidgetGeometry() const { return m_finalWidgetGeometry; }

    /** Holds whether we are in animation final state. */
    bool         m_fStateIsFinal;
    /** Holds the shift left/right animation instance. */
    UIAnimation *m_pAnimation;
    /** Holds sub-window start-geometry. */
    QRect        m_startWidgetGeometry;
    /** Holds sub-window final-geometry. */
    QRect        m_finalWidgetGeometry;

    /** Holds the private sliding widget instance. */
    QWidget     *m_pWidget;
    /** Holds the widget layout instance. */
    QHBoxLayout *m_pLayout;
    /** Holds the 1st widget reference. */
    QWidget     *m_pWidget1;
    /** Holds the 2nd widget reference. */
    QWidget     *m_pWidget2;
};

#endif /* !___UISlidingWidget_h___ */


/* $Id: UISlidingToolBar.cpp $ */
/** @file
 * VBox Qt GUI - UISlidingToolBar class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QHBoxLayout>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UISlidingToolBar.h"
# include "UIAnimationFramework.h"
# include "UIMachineWindow.h"
# ifdef VBOX_WS_MAC
#  include "VBoxUtils-darwin.h"
# endif /* VBOX_WS_MAC */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UISlidingToolBar::UISlidingToolBar(QWidget *pParentWidget, QWidget *pIndentWidget, QWidget *pChildWidget, Position position)
    : QWidget(pParentWidget, Qt::Tool | Qt::FramelessWindowHint)
    , m_position(position)
    , m_parentRect(pParentWidget ? pParentWidget->geometry() : QRect())
    , m_indentRect(pIndentWidget ? pIndentWidget->geometry() : QRect())
    , m_pAnimation(0)
    , m_fExpanded(false)
    , m_pMainLayout(0)
    , m_pArea(0)
    , m_pWidget(pChildWidget)
{
    /* Prepare: */
    prepare();
}

void UISlidingToolBar::sltParentGeometryChanged(const QRect &parentRect)
{
    /* Update rectangle: */
    m_parentRect = parentRect;
    /* Adjust geometry: */
    adjustGeometry();
    /* Update animation: */
    updateAnimation();
}

void UISlidingToolBar::prepare()
{
    /* Do not count that window as important for application,
     * it will NOT be taken into account when other top-level windows will be closed: */
    setAttribute(Qt::WA_QuitOnClose, false);
    /* Delete window when closed: */
    setAttribute(Qt::WA_DeleteOnClose);

#if   defined(VBOX_WS_MAC) || defined(VBOX_WS_WIN)
    /* Make sure we have no background
     * until the first one paint-event: */
    setAttribute(Qt::WA_NoSystemBackground);
    /* Use Qt API to enable translucency: */
    setAttribute(Qt::WA_TranslucentBackground);
#elif defined(VBOX_WS_X11)
    if (vboxGlobal().isCompositingManagerRunning())
    {
        /* Use Qt API to enable translucency: */
        setAttribute(Qt::WA_TranslucentBackground);
    }
#endif /* VBOX_WS_X11 */

    /* Prepare contents: */
    prepareContents();
    /* Prepare geometry: */
    prepareGeometry();
    /* Prepare animation: */
    prepareAnimation();
}

void UISlidingToolBar::prepareContents()
{
    /* Create main-layout: */
    m_pMainLayout = new QHBoxLayout(this);
    AssertPtrReturnVoid(m_pMainLayout);
    {
        /* Configure main-layout: */
        m_pMainLayout->setContentsMargins(0, 0, 0, 0);
        m_pMainLayout->setSpacing(0);
        /* Create area: */
        m_pArea = new QWidget;
        AssertPtrReturnVoid(m_pArea);
        {
            /* Configure area: */
            m_pArea->setAcceptDrops(true);
            m_pArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            QPalette pal1 = m_pArea->palette();
            pal1.setColor(QPalette::Window, QColor(Qt::transparent));
            m_pArea->setPalette(pal1);
            /* Make sure valid child-widget passed: */
            AssertPtrReturnVoid(m_pWidget);
            {
                /* Configure child-widget: */
                QPalette pal2 = m_pWidget->palette();
                pal2.setColor(QPalette::Window, palette().color(QPalette::Window));
                m_pWidget->setPalette(pal2);
                connect(m_pWidget, SIGNAL(sigCancelClicked()), this, SLOT(close()));
                /* Add child-widget into area: */
                m_pWidget->setParent(m_pArea);
            }
            /* Add area into main-layout: */
            m_pMainLayout->addWidget(m_pArea);
        }
    }
}

void UISlidingToolBar::prepareGeometry()
{
    /* Prepare geometry based on parent and sub-window size-hints,
     * But move sub-window to initial position: */
    const QSize sh = m_pWidget->sizeHint();
    switch (m_position)
    {
        case Position_Top:
        {
            VBoxGlobal::setTopLevelGeometry(this, m_parentRect.x(), m_parentRect.y()                         + m_indentRect.height(),
                                                  qMax(m_parentRect.width(), sh.width()), sh.height());
            m_pWidget->setGeometry(0, -sh.height(), qMax(width(), sh.width()), sh.height());
            break;
        }
        case Position_Bottom:
        {
            VBoxGlobal::setTopLevelGeometry(this, m_parentRect.x(), m_parentRect.y() + m_parentRect.height() - m_indentRect.height() - sh.height(),
                                                  qMax(m_parentRect.width(), sh.width()), sh.height());
            m_pWidget->setGeometry(0,  sh.height(), qMax(width(), sh.width()), sh.height());
            break;
        }
    }

#ifdef VBOX_WS_X11
    if (!vboxGlobal().isCompositingManagerRunning())
    {
        /* Use Xshape otherwise: */
        setMask(m_pWidget->geometry());
    }
#endif /* VBOX_WS_X11 */

#ifdef VBOX_WS_WIN
    /* Raise tool-window for proper z-order. */
    raise();
#endif /* VBOX_WS_WIN */

    /* Activate window after it was shown: */
    connect(this, SIGNAL(sigShown()), this,
            SLOT(sltActivateWindow()), Qt::QueuedConnection);
    /* Update window geometry after parent geometry changed: */
    connect(parent(), SIGNAL(sigGeometryChange(const QRect&)),
            this, SLOT(sltParentGeometryChanged(const QRect&)));
}

void UISlidingToolBar::prepareAnimation()
{
    /* Prepare sub-window geometry animation itself: */
    connect(this, SIGNAL(sigShown()), this, SIGNAL(sigExpand()), Qt::QueuedConnection);
    m_pAnimation = UIAnimation::installPropertyAnimation(this,
                                                         "widgetGeometry",
                                                         "startWidgetGeometry", "finalWidgetGeometry",
                                                         SIGNAL(sigExpand()), SIGNAL(sigCollapse()));
    connect(m_pAnimation, SIGNAL(sigStateEnteredStart()), this, SLOT(sltMarkAsCollapsed()));
    connect(m_pAnimation, SIGNAL(sigStateEnteredFinal()), this, SLOT(sltMarkAsExpanded()));
    /* Update geometry animation: */
    updateAnimation();
}

void UISlidingToolBar::adjustGeometry()
{
    /* Adjust geometry based on parent and sub-window size-hints: */
    const QSize sh = m_pWidget->sizeHint();
    switch (m_position)
    {
        case Position_Top:
        {
            VBoxGlobal::setTopLevelGeometry(this, m_parentRect.x(), m_parentRect.y()                         + m_indentRect.height(),
                                                  qMax(m_parentRect.width(), sh.width()), sh.height());
            break;
        }
        case Position_Bottom:
        {
            VBoxGlobal::setTopLevelGeometry(this, m_parentRect.x(), m_parentRect.y() + m_parentRect.height() - m_indentRect.height() - sh.height(),
                                                  qMax(m_parentRect.width(), sh.width()), sh.height());
            break;
        }
    }
    /* And move sub-window to corresponding position: */
    m_pWidget->setGeometry(0, 0, qMax(width(), sh.width()), sh.height());

#ifdef VBOX_WS_X11
    if (!vboxGlobal().isCompositingManagerRunning())
    {
        /* Use Xshape otherwise: */
        setMask(m_pWidget->geometry());
    }
#endif /* VBOX_WS_X11 */

#ifdef VBOX_WS_WIN
    /* Raise tool-window for proper z-order. */
    raise();
#endif /* VBOX_WS_WIN */
}

void UISlidingToolBar::updateAnimation()
{
    /* Skip if no animation created: */
    if (!m_pAnimation)
        return;

    /* Recalculate sub-window geometry animation boundaries based on size-hint: */
    const QSize sh = m_pWidget->sizeHint();
    switch (m_position)
    {
        case Position_Top:    m_startWidgetGeometry = QRect(0, -sh.height(), qMax(width(), sh.width()), sh.height()); break;
        case Position_Bottom: m_startWidgetGeometry = QRect(0,  sh.height(), qMax(width(), sh.width()), sh.height()); break;
    }
    m_finalWidgetGeometry = QRect(0, 0, qMax(width(), sh.width()), sh.height());
    m_pAnimation->update();
}

void UISlidingToolBar::showEvent(QShowEvent*)
{
    /* If window isn't expanded: */
    if (!m_fExpanded)
    {
        /* Start expand animation: */
        emit sigShown();
    }
}

void UISlidingToolBar::closeEvent(QCloseEvent *pEvent)
{
    /* If window isn't expanded: */
    if (!m_fExpanded)
    {
        /* Ignore close-event: */
        pEvent->ignore();
        return;
    }

    /* If animation state is Final: */
    const QString strAnimationState = property("AnimationState").toString();
    bool fAnimationComplete = strAnimationState == "Final";
    if (fAnimationComplete)
    {
        /* Ignore close-event: */
        pEvent->ignore();
        /* And start collapse animation: */
        emit sigCollapse();
    }
}

#ifdef VBOX_WS_MAC
bool UISlidingToolBar::event(QEvent *pEvent)
{
    /* Depending on event-type: */
    switch (pEvent->type())
    {
        case QEvent::Resize:
        case QEvent::WindowActivate:
        {
            /* By some strange reason
             * cocoa resets NSWindow::setHasShadow option
             * for frameless windows on every window resize/activation.
             * So we have to make sure window still has no shadows. */
            darwinSetWindowHasShadow(this, false);
            break;
        }
        default:
            break;
    }
    /* Call to base-class: */
    return QWidget::event(pEvent);
}
#endif /* VBOX_WS_MAC */

void UISlidingToolBar::setWidgetGeometry(const QRect &rect)
{
    /* Apply sub-window geometry: */
    m_pWidget->setGeometry(rect);

#ifdef VBOX_WS_X11
    if (!vboxGlobal().isCompositingManagerRunning())
    {
        /* Use Xshape otherwise: */
        setMask(m_pWidget->geometry());
    }
#endif /* VBOX_WS_X11 */
}

QRect UISlidingToolBar::widgetGeometry() const
{
    /* Return sub-window geometry: */
    return m_pWidget->geometry();
}


/* $Id: QIArrowButtonSwitch.cpp $ */
/** @file
 * VBox Qt GUI - QIArrowButtonSwitch class implementation.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
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
# include <QKeyEvent>

/* GUI includes: */
# include "QIArrowButtonSwitch.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


QIArrowButtonSwitch::QIArrowButtonSwitch(QWidget *pParent /* = 0 */)
    : QIRichToolButton(pParent)
    , m_fExpanded(false)
{
    /* Update icon: */
    updateIcon();
}

void QIArrowButtonSwitch::setIcons(const QIcon &iconCollapsed, const QIcon &iconExpanded)
{
    /* Assign icons: */
    m_iconCollapsed = iconCollapsed;
    m_iconExpanded = iconExpanded;
    /* Update icon: */
    updateIcon();
}

void QIArrowButtonSwitch::setExpanded(bool fExpanded)
{
    /* Set button state: */
    m_fExpanded = fExpanded;
    /* Update icon: */
    updateIcon();
}

void QIArrowButtonSwitch::sltButtonClicked()
{
    /* Toggle button state: */
    m_fExpanded = !m_fExpanded;
    /* Update icon: */
    updateIcon();
}

void QIArrowButtonSwitch::keyPressEvent(QKeyEvent *pEvent)
{
    /* Handle different keys: */
    switch (pEvent->key())
    {
        /* Animate-click for the Space key: */
        case Qt::Key_Minus: if (m_fExpanded) return animateClick(); break;
        case Qt::Key_Plus: if (!m_fExpanded) return animateClick(); break;
        default: break;
    }
    /* Call to base-class: */
    QIRichToolButton::keyPressEvent(pEvent);
}


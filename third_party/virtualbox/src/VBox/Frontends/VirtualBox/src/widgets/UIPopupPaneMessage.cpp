/* $Id: UIPopupPaneMessage.cpp $ */
/** @file
 * VBox Qt GUI - UIPopupPaneMessage class implementation.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
# include <QLabel>
# include <QCheckBox>

/* GUI includes: */
# include "UIPopupPaneMessage.h"
# include "UIAnimationFramework.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIPopupPaneMessage::UIPopupPaneMessage(QWidget *pParent, const QString &strText, bool fFocused)
    : QWidget(pParent)
    , m_iLayoutMargin(0)
    , m_iLayoutSpacing(10)
    , m_strText(strText)
    , m_pLabel(0)
    , m_iDesiredLabelWidth(-1)
    , m_fFocused(fFocused)
    , m_pAnimation(0)
{
    /* Prepare: */
    prepare();
}

void UIPopupPaneMessage::setText(const QString &strText)
{
    /* Make sure the text has changed: */
    if (m_strText == strText)
        return;

    /* Fetch new text: */
    m_strText = strText;
    m_pLabel->setText(m_strText);

    /* Update size-hint: */
    updateSizeHint();
}

QSize UIPopupPaneMessage::minimumSizeHint() const
{
    /* Check if desired-width set: */
    if (m_iDesiredLabelWidth >= 0)
        /* Dependent size-hint: */
        return m_minimumSizeHint;
    /* Golden-rule size-hint by default: */
    return QWidget::minimumSizeHint();
}

void UIPopupPaneMessage::setMinimumSizeHint(const QSize &minimumSizeHint)
{
    /* Make sure the size-hint has changed: */
    if (m_minimumSizeHint == minimumSizeHint)
        return;

    /* Fetch new size-hint: */
    m_minimumSizeHint = minimumSizeHint;

    /* Notify parent popup-pane: */
    emit sigSizeHintChanged();
}

void UIPopupPaneMessage::layoutContent()
{
    /* Variables: */
    const int iWidth = width();
    const int iHeight = height();
    const int iLabelWidth = m_labelSizeHint.width();
    const int iLabelHeight = m_labelSizeHint.height();

    /* Label: */
    m_pLabel->move(m_iLayoutMargin, m_iLayoutMargin);
    m_pLabel->resize(qMin(iWidth, iLabelWidth), qMin(iHeight, iLabelHeight));
}

void UIPopupPaneMessage::sltHandleProposalForWidth(int iWidth)
{
    /* Make sure the desired-width has changed: */
    if (m_iDesiredLabelWidth == iWidth)
        return;

    /* Fetch new desired-width: */
    m_iDesiredLabelWidth = iWidth;

    /* Update size-hint: */
    updateSizeHint();
}

void UIPopupPaneMessage::sltFocusEnter()
{
    /* Ignore if already focused: */
    if (m_fFocused)
        return;

    /* Update focus state: */
    m_fFocused = true;

    /* Notify listeners: */
    emit sigFocusEnter();
}

void UIPopupPaneMessage::sltFocusLeave()
{
    /* Ignore if already unfocused: */
    if (!m_fFocused)
        return;

    /* Update focus state: */
    m_fFocused = false;

    /* Notify listeners: */
    emit sigFocusLeave();
}

void UIPopupPaneMessage::prepare()
{
    /* Prepare content: */
    prepareContent();
    /* Prepare animation: */
    prepareAnimation();

    /* Update size-hint: */
    updateSizeHint();
}

void UIPopupPaneMessage::prepareContent()
{
    /* Create label: */
    m_pLabel = new QLabel(this);
    {
        /* Configure label: */
        m_pLabel->setFont(tuneFont(m_pLabel->font()));
        m_pLabel->setWordWrap(true);
        m_pLabel->setFocusPolicy(Qt::NoFocus);
        m_pLabel->setText(m_strText);
    }
}

void UIPopupPaneMessage::prepareAnimation()
{
    /* Propagate parent signals: */
    connect(parent(), SIGNAL(sigFocusEnter()), this, SLOT(sltFocusEnter()));
    connect(parent(), SIGNAL(sigFocusLeave()), this, SLOT(sltFocusLeave()));
    /* Install geometry animation for 'minimumSizeHint' property: */
    m_pAnimation = UIAnimation::installPropertyAnimation(this, "minimumSizeHint", "collapsedSizeHint", "expandedSizeHint",
                                                         SIGNAL(sigFocusEnter()), SIGNAL(sigFocusLeave()), m_fFocused);
}

void UIPopupPaneMessage::updateSizeHint()
{
    /* Recalculate collapsed size-hint: */
    {
        /* Collapsed size-hint contains only one-text-line label: */
        QFontMetrics fm(m_pLabel->font(), m_pLabel);
        m_collapsedSizeHint = QSize(m_iDesiredLabelWidth, fm.height());
    }

    /* Recalculate expanded size-hint: */
    {
        /* Recalculate label size-hint: */
        m_labelSizeHint = QSize(m_iDesiredLabelWidth, m_pLabel->heightForWidth(m_iDesiredLabelWidth));
        /* Expanded size-hint contains full-size label: */
        m_expandedSizeHint = m_labelSizeHint;
    }

    /* Update current size-hint: */
    m_minimumSizeHint = m_fFocused ? m_expandedSizeHint : m_collapsedSizeHint;

    /* Update animation: */
    if (m_pAnimation)
        m_pAnimation->update();

    /* Notify parent popup-pane: */
    emit sigSizeHintChanged();
}

/* static */
QFont UIPopupPaneMessage::tuneFont(QFont font)
{
#if defined(VBOX_WS_MAC)
    font.setPointSize(font.pointSize() - 2);
#elif defined(VBOX_WS_X11)
    font.setPointSize(font.pointSize() - 1);
#endif
    return font;
}


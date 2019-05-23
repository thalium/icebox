/* $Id: UIPopupPaneDetails.cpp $ */
/** @file
 * VBox Qt GUI - UIPopupPaneDetails class implementation.
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
# include <QCheckBox>
# include <QTextDocument>
# include <QTextEdit>

/* GUI includes: */
# include "UIPopupPaneDetails.h"
# include "UIAnimationFramework.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIPopupPaneDetails::UIPopupPaneDetails(QWidget *pParent, const QString &strText, bool fFocused)
    : QWidget(pParent)
    , m_iLayoutMargin(5)
    , m_iLayoutSpacing(10)
    , m_strText(strText)
    , m_pTextEdit(0)
    , m_iDesiredTextEditWidth(-1)
    , m_iMaximumPaneHeight(-1)
    , m_iMaximumTextEditHeight(0)
    , m_iTextContentMargin(5)
    , m_fFocused(fFocused)
    , m_pAnimation(0)
{
    /* Prepare: */
    prepare();
}

void UIPopupPaneDetails::setText(const QString &strText)
{
    /* Make sure the text has changed: */
    if (m_strText == strText)
        return;

    /* Fetch new text: */
    m_strText = strText;
    m_pTextEdit->setText(m_strText);

    /* Update size-hint/visibility: */
    updateSizeHint();
    updateVisibility();
}

QSize UIPopupPaneDetails::minimumSizeHint() const
{
    /* Check if desired-width set: */
    if (m_iDesiredTextEditWidth >= 0)
        /* Dependent size-hint: */
        return m_minimumSizeHint;
    /* Golden-rule size-hint by default: */
    return QWidget::minimumSizeHint();
}

void UIPopupPaneDetails::setMinimumSizeHint(const QSize &minimumSizeHint)
{
    /* Make sure the size-hint has changed: */
    if (m_minimumSizeHint == minimumSizeHint)
        return;

    /* Fetch new size-hint: */
    m_minimumSizeHint = minimumSizeHint;

    /* Notify parent popup-pane: */
    emit sigSizeHintChanged();
}

void UIPopupPaneDetails::layoutContent()
{
    /* Variables: */
    const int iWidth = width();
    const int iHeight = height();
    const int iTextEditWidth = m_textEditSizeHint.width();
    const int iTextEditHeight = m_textEditSizeHint.height();

    /* TextEdit: */
    m_pTextEdit->move(m_iLayoutMargin, m_iLayoutMargin);
    m_pTextEdit->resize(qMin(iWidth, iTextEditWidth), qMin(iHeight, iTextEditHeight));
    /* Text-document: */
    QTextDocument *pTextDocument = m_pTextEdit->document();
    if (pTextDocument)
    {
        pTextDocument->adjustSize();
        pTextDocument->setTextWidth(m_pTextEdit->width() - m_iTextContentMargin);
    }
}

void UIPopupPaneDetails::updateVisibility()
{
    if (m_fFocused && !m_strText.isEmpty())
        show();
    else
        hide();
}

void UIPopupPaneDetails::sltHandleProposalForWidth(int iWidth)
{
    /* Make sure the desired-width has changed: */
    if (m_iDesiredTextEditWidth == iWidth)
        return;

    /* Fetch new desired-width: */
    m_iDesiredTextEditWidth = iWidth;

    /* Update size-hint: */
    updateSizeHint();
}

void UIPopupPaneDetails::sltHandleProposalForHeight(int iHeight)
{
    /* Make sure the desired-height has changed: */
    if (m_iMaximumPaneHeight == iHeight)
        return;

    /* Fetch new desired-height: */
    m_iMaximumPaneHeight = iHeight;
    m_iMaximumTextEditHeight = m_iMaximumPaneHeight - 2 * m_iLayoutMargin;

    /* Update size-hint: */
    updateSizeHint();
}

void UIPopupPaneDetails::sltFocusEnter()
{
    /* Ignore if already focused: */
    if (m_fFocused)
        return;

    /* Update focus state: */
    m_fFocused = true;

    /* Update visibility: */
    updateVisibility();

    /* Notify listeners: */
    emit sigFocusEnter();
}

void UIPopupPaneDetails::sltFocusLeave()
{
    /* Ignore if already unfocused: */
    if (!m_fFocused)
        return;

    /* Update focus state: */
    m_fFocused = false;

    /* Update visibility: */
    updateVisibility();

    /* Notify listeners: */
    emit sigFocusLeave();
}

void UIPopupPaneDetails::prepare()
{
    /* Prepare content: */
    prepareContent();
    /* Prepare animation: */
    prepareAnimation();

    /* Update size-hint/visibility: */
    updateSizeHint();
    updateVisibility();
}

void UIPopupPaneDetails::prepareContent()
{
    /* Create text-editor: */
    m_pTextEdit = new QTextEdit(this);
    {
        /* Configure text-editor: */
        m_pTextEdit->setFont(tuneFont(m_pTextEdit->font()));
        m_pTextEdit->setText(m_strText);
        m_pTextEdit->setFocusProxy(this);
    }
}

void UIPopupPaneDetails::prepareAnimation()
{
    /* Propagate parent signals: */
    connect(parent(), SIGNAL(sigFocusEnter()), this, SLOT(sltFocusEnter()));
    connect(parent(), SIGNAL(sigFocusLeave()), this, SLOT(sltFocusLeave()));
    /* Install geometry animation for 'minimumSizeHint' property: */
    m_pAnimation = UIAnimation::installPropertyAnimation(this, "minimumSizeHint", "collapsedSizeHint", "expandedSizeHint",
                                                         SIGNAL(sigFocusEnter()), SIGNAL(sigFocusLeave()), m_fFocused);
}

void UIPopupPaneDetails::updateSizeHint()
{
    /* Recalculate collapsed size-hint: */
    {
        /* Collapsed size-hint with 0 height: */
        m_collapsedSizeHint = QSize(m_iDesiredTextEditWidth, 0);
    }

    /* Recalculate expanded size-hint: */
    {
        int iNewHeight = m_iMaximumPaneHeight;
        QTextDocument *pTextDocument = m_pTextEdit->document();
        if(pTextDocument)
        {
            /* Adjust text-edit size: */
            pTextDocument->adjustSize();
            /* Get corresponding QTextDocument size: */
            QSize textSize = pTextDocument->size().toSize();
            /* Make sure the text edits height is no larger than that of container widget: */
            iNewHeight = qMin(m_iMaximumTextEditHeight, textSize.height() + 2 * m_iLayoutMargin);
        }
        /* Recalculate label size-hint: */
        m_textEditSizeHint = QSize(m_iDesiredTextEditWidth, iNewHeight);
        /* Expanded size-hint contains full-size label: */
        m_expandedSizeHint = m_textEditSizeHint;
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
QFont UIPopupPaneDetails::tuneFont(QFont font)
{
#if defined(VBOX_WS_MAC)
    font.setPointSize(font.pointSize() - 2);
#elif defined(VBOX_WS_X11)
    font.setPointSize(font.pointSize() - 1);
#endif
    return font;
}


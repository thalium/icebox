/* $Id: UIFilmContainer.cpp $ */
/** @file
 * VBox Qt GUI - UIFilmContainer class implementation.
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
# include <QHBoxLayout>
# include <QVBoxLayout>
# include <QScrollArea>
# include <QScrollBar>
# include <QStyle>
# include <QCheckBox>
# include <QPainter>

/* GUI includes: */
# include "UIFilmContainer.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIFilmContainer::UIFilmContainer(QWidget *pParent /* = 0*/)
    : QWidget(pParent)
    , m_pMainLayout(0)
    , m_pScroller(0)
{
    /* Prepare: */
    prepare();
}

QVector<BOOL> UIFilmContainer::value() const
{
    /* Enumerate all the existing widgets: */
    QVector<BOOL> value;
    foreach (UIFilm *pWidget, m_widgets)
        value << static_cast<BOOL>(pWidget->checked());

    /* Return value: */
    return value;
}

void UIFilmContainer::setValue(const QVector<BOOL> &value)
{
    /* Cleanup viewport/widget list: */
    delete m_pScroller->takeWidget();
    m_widgets.clear();

    /* Create widget: */
    if (QWidget *pWidget = new QWidget)
    {
        /* Create viewport layout: */
        if (QHBoxLayout *pWidgetLayout = new QHBoxLayout(pWidget))
        {
            /* Configure viewport layout: */
            pWidgetLayout->setMargin(0);
#ifdef VBOX_WS_MAC
            pWidgetLayout->setSpacing(5);
#else
            pWidgetLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2);
#endif
            /* Create new widgets according passed vector: */
            for (int iScreenIndex = 0; iScreenIndex < value.size(); ++iScreenIndex)
            {
                /* Create new widget: */
                UIFilm *pWidget = new UIFilm(iScreenIndex, value[iScreenIndex]);
                /* Add widget into the widget list: */
                m_widgets << pWidget;
                /* Add widget into the viewport layout: */
                pWidgetLayout->addWidget(pWidget);
            }
        }
        /* Assign scroller with widget: */
        m_pScroller->setWidget(pWidget);
        /* Reconfigure scroller widget: */
        m_pScroller->widget()->setAutoFillBackground(false);
        /* And adjust that widget geometry: */
        QSize msh = m_pScroller->widget()->minimumSizeHint();
        int iMinimumHeight = msh.height();
        m_pScroller->viewport()->setFixedHeight(iMinimumHeight);
    }
}

void UIFilmContainer::prepare()
{
    /* Prepare layout: */
    prepareLayout();

    /* Prepare scroller: */
    prepareScroller();

    /* Append with 'default' value: */
    setValue(QVector<BOOL>() << true);
}

void UIFilmContainer::prepareLayout()
{
    /* Create layout: */
    m_pMainLayout = new QVBoxLayout(this);

    /* Configure layout: */
    m_pMainLayout->setMargin(0);
    m_pMainLayout->setSpacing(0);
}

void UIFilmContainer::prepareScroller()
{
    /* Create scroller: */
    m_pScroller = new QScrollArea;

    /* Configure scroller: */
    m_pScroller->setFrameShape(QFrame::NoFrame);
    m_pScroller->viewport()->setAutoFillBackground(false);
    m_pScroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_pScroller->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_pMainLayout->addWidget(m_pScroller);
}

UIFilm::UIFilm(int iScreenIndex, BOOL fEnabled, QWidget *pParent /* = 0*/)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_iScreenIndex(iScreenIndex)
    , m_fWasEnabled(fEnabled)
    , m_pCheckBox(0)
{
    /* Prepare: */
    prepare();
}

bool UIFilm::checked() const
{
    /* Is the check-box currently checked? */
    return m_pCheckBox->isChecked();
}

void UIFilm::retranslateUi()
{
    /* Translate check-box: */
    m_pCheckBox->setText(QApplication::translate("UIMachineSettingsDisplay", "Screen %1").arg(m_iScreenIndex + 1));
    m_pCheckBox->setWhatsThis(QApplication::translate("UIMachineSettingsDisplay", "When checked, enables video recording for screen %1.").arg(m_iScreenIndex + 1));
}

void UIFilm::prepare()
{
    /* Prepare layout: */
    prepareLayout();

    /* Prepare check-box: */
    prepareCheckBox();

    /* Translate finally: */
    retranslateUi();
}

void UIFilm::prepareLayout()
{
    /* Create layout: */
    m_pMainLayout = new QVBoxLayout(this);

    /* Configure layout: */
#ifdef VBOX_WS_MAC
    m_pMainLayout->setContentsMargins(10, 10, 15, 10);
#endif /* VBOX_WS_MAC */

    /* Add strech: */
    m_pMainLayout->addStretch();
}

void UIFilm::prepareCheckBox()
{
    /* Create check-box: */
    m_pCheckBox = new QCheckBox;
    m_pCheckBox->setChecked(static_cast<bool>(m_fWasEnabled));

    /* Configure font: */
    QFont currentFont = m_pCheckBox->font();
#ifdef VBOX_WS_MAC
    currentFont.setPointSize(currentFont.pointSize() - 2);
#else /* VBOX_WS_MAC */
    currentFont.setPointSize(currentFont.pointSize() - 1);
#endif /* !VBOX_WS_MAC */
    m_pCheckBox->setFont(currentFont);

    /* Insert check-box into layout: */
    m_pMainLayout->insertWidget(0, m_pCheckBox);
}

QSize UIFilm::minimumSizeHint() const
{
    /* Return 16:9 aspect-ratio msh: */
    QSize msh = QWidget::minimumSizeHint();
    return QSize(msh.width(), (msh.width() * 9) / 16);
}

void UIFilm::paintEvent(QPaintEvent*)
{
    /* Compose painting rectangle: */
    const QRect rect(1, 1, width() - 2, height() - 2);

    /* Create painter: */
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    /* Configure painter clipping: */
    QPainterPath path;
    int iDiameter = 6;
    QSizeF arcSize(2 * iDiameter, 2 * iDiameter);
    path.moveTo(rect.x() + iDiameter, rect.y());
    path.arcTo(QRectF(path.currentPosition(), arcSize).translated(-iDiameter, 0), 90, 90);
    path.lineTo(path.currentPosition().x(), rect.height() - iDiameter);
    path.arcTo(QRectF(path.currentPosition(), arcSize).translated(0, -iDiameter), 180, 90);
    path.lineTo(rect.width() - iDiameter, path.currentPosition().y());
    path.arcTo(QRectF(path.currentPosition(), arcSize).translated(-iDiameter, -2 * iDiameter), 270, 90);
    path.lineTo(path.currentPosition().x(), rect.y() + iDiameter);
    path.arcTo(QRectF(path.currentPosition(), arcSize).translated(-2 * iDiameter, -iDiameter), 0, 90);
    path.closeSubpath();

    /* Get current background color: */
    QColor currentColor(palette().color(backgroundRole()));

    /* Fill with background: */
    painter.setClipPath(path);
    QColor newColor1 = currentColor;
    QColor newColor2 = currentColor.darker(125);
    QLinearGradient headerGradient(rect.topLeft(), rect.bottomRight());
    headerGradient.setColorAt(0, newColor1);
    headerGradient.setColorAt(1, newColor2);
    painter.fillRect(rect, headerGradient);

    /* Stroke with border: */
    QColor strokeColor = currentColor.darker(150);
    painter.setClipping(false);
    painter.strokePath(path, strokeColor);
}


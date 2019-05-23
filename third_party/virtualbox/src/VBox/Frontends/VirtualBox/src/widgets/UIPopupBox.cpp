/* $Id: UIPopupBox.cpp $ */
/** @file
 * VBox Qt GUI - UIPopupBox/UIPopupBoxGroup classes implementation.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
# include <QApplication>
# include <QStyle>
# include <QLabel>
# include <QPainter>
# include <QVBoxLayout>
# include <QPaintEvent>

/* GUI includes: */
# include "UIPopupBox.h"
# ifdef VBOX_WS_MAC
#  include "UIImageTools.h"
# endif /* VBOX_WS_MAC */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

UIPopupBox::UIPopupBox(QWidget *pParent)
  : QWidget(pParent)
  , m_pTitleIcon(new QLabel(this))
  , m_pWarningIcon(new QLabel(this))
  , m_pTitleLabel(new QLabel(this))
  , m_fLinkEnabled(false)
  , m_pContentWidget(0)
  , m_fOpen(true)
  , m_pLabelPath(0)
  , m_iArrowWidth(9)
  , m_fHeaderHover(false)
{
    /* Setup main-layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    /* Setup title-layout: */
    QHBoxLayout *pTitleLayout = new QHBoxLayout;
    pTitleLayout->addWidget(m_pTitleIcon);
    pTitleLayout->addWidget(m_pWarningIcon);
    pTitleLayout->addWidget(m_pTitleLabel, Qt::AlignLeft);
    /* Add title-layout into main-layout: */
    pMainLayout->addLayout(pTitleLayout);

    /* Configure widgets: */
    m_pWarningIcon->setHidden(true);
    m_arrowPath.lineTo(m_iArrowWidth / 2.0, m_iArrowWidth / 2.0);
    m_arrowPath.lineTo(m_iArrowWidth, 0);

    /* Setup connections: */
    connect(m_pTitleLabel, SIGNAL(linkActivated(const QString)), this, SIGNAL(sigTitleClicked(const QString)));

    /* Install local event-filters: */
    installEventFilter(this);
    m_pTitleIcon->installEventFilter(this);
    m_pWarningIcon->installEventFilter(this);
    m_pTitleLabel->installEventFilter(this);
}

UIPopupBox::~UIPopupBox()
{
    if (m_pLabelPath)
        delete m_pLabelPath;
}

void UIPopupBox::setTitleIcon(const QIcon &icon)
{
    /* Remember new title-icon: */
    m_titleIcon = icon;
    /* Update title-icon: */
    updateTitleIcon();
    /* Recalculate title-size: */
    recalc();
}

QIcon UIPopupBox::titleIcon() const
{
    return m_titleIcon;
}

void UIPopupBox::setWarningIcon(const QIcon &icon)
{
    /* Remember new warning-icon: */
    m_warningIcon = icon;
    /* Update warning-icon: */
    updateWarningIcon();
    /* Recalculate title-size: */
    recalc();
}

QIcon UIPopupBox::warningIcon() const
{
    return m_warningIcon;
}

void UIPopupBox::setTitle(const QString &strTitle)
{
    /* Remember new title: */
    m_strTitle = strTitle;
    /* Update title: */
    updateTitle();
    /* Recalculate title-size: */
    recalc();
}

QString UIPopupBox::title() const
{
    return m_strTitle;
}

void UIPopupBox::setTitleLink(const QString &strLink)
{
    /* Remember new title-link: */
    m_strLink = strLink;
    /* Update title: */
    updateTitle();
}

QString UIPopupBox::titleLink() const
{
    return m_strLink;
}

void UIPopupBox::setTitleLinkEnabled(bool fEnabled)
{
    /* Remember new title-link availability flag: */
    m_fLinkEnabled = fEnabled;
    /* Update title: */
    updateTitle();
}

bool UIPopupBox::isTitleLinkEnabled() const
{
    return m_fLinkEnabled;
}

void UIPopupBox::setContentWidget(QWidget *pWidget)
{
    /* Cleanup old content-widget if any: */
    if (m_pContentWidget)
    {
        m_pContentWidget->removeEventFilter(this);
        layout()->removeWidget(m_pContentWidget);
    }
    /* Prepare new content-wodget: */
    m_pContentWidget = pWidget;
    layout()->addWidget(m_pContentWidget);
    m_pContentWidget->installEventFilter(this);
    recalc();
}

QWidget* UIPopupBox::contentWidget() const
{
    return m_pContentWidget;
}

void UIPopupBox::setOpen(bool fOpen)
{
    /* Check if we should toggle popup-box: */
    if (m_fOpen == fOpen)
        return;

    /* Store new value: */
    m_fOpen = fOpen;

    /* Update content-widget if present or this itself: */
    if (m_pContentWidget)
        m_pContentWidget->setVisible(m_fOpen);
    else
        update();

    /* Notify listeners about content-widget visibility: */
    if (m_pContentWidget && m_pContentWidget->isVisible())
        emit sigUpdateContentWidget();
}

void UIPopupBox::toggleOpen()
{
    /* Switch 'opened' state: */
    setOpen(!m_fOpen);

    /* Notify listeners about toggling: */
    emit sigToggled(m_fOpen);
}

bool UIPopupBox::isOpen() const
{
    return m_fOpen;
}

bool UIPopupBox::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* Handle all mouse-event to update hover: */
    QEvent::Type type = pEvent->type();
    if (type == QEvent::Enter ||
        type == QEvent::Leave ||
        type == QEvent::MouseMove ||
        type == QEvent::Wheel)
        updateHover();
    /* Call to base-class: */
    return QWidget::eventFilter(pWatched, pEvent);
}

void UIPopupBox::resizeEvent(QResizeEvent *pEvent)
{
    /* Recalculate title-size: */
    recalc();
    /* Call to base-class: */
    QWidget::resizeEvent(pEvent);
}

void UIPopupBox::mouseDoubleClickEvent(QMouseEvent * /* pEvent */)
{
    /* Toggle popup-box: */
    toggleOpen();
}

void UIPopupBox::paintEvent(QPaintEvent *pEvent)
{
    /* Create painter: */
    QPainter painter(this);
    painter.setClipRect(pEvent->rect());

    QPalette pal = palette();
    painter.setClipPath(*m_pLabelPath);
    QColor base = pal.color(QPalette::Active, QPalette::Window);
    QRect rect = QRect(QPoint(0, 0), size()).adjusted(0, 0, -1, -1);
    /* Base background */
    painter.fillRect(QRect(QPoint(0, 0), size()), pal.brush(QPalette::Active, QPalette::Base));
    /* Top header background */
    const int iMaxHeightHint = qMax(m_pTitleLabel->sizeHint().height(),
                                    m_pTitleIcon->sizeHint().height());
    QLinearGradient lg(rect.x(), rect.y(), rect.x(), rect.y() + 2 * 5 + iMaxHeightHint);
    lg.setColorAt(0, base.darker(95));
    lg.setColorAt(1, base.darker(110));
    int theight = rect.height();
    if (m_fOpen)
        theight = 2 * 5 + iMaxHeightHint;
    painter.fillRect(QRect(rect.x(), rect.y(), rect.width(), theight), lg);
    /* Outer round rectangle line */
    painter.setClipping(false);
    painter.strokePath(*m_pLabelPath, base.darker(110));
    /* Arrow */
    if (m_fHeaderHover)
    {
        painter.setBrush(base.darker(106));
        painter.setPen(QPen(base.darker(128), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QSizeF s = m_arrowPath.boundingRect().size();
        if (m_fOpen)
        {
            painter.translate(rect.x() + rect.width() - s.width() - 10, rect.y() + theight / 2 + s.height() / 2);
            /* Flip */
            painter.scale(1, -1);
        }
        else
            painter.translate(rect.x() + rect.width() - s.width() - 10, rect.y() + theight / 2 - s.height() / 2 + 1);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPath(m_arrowPath);
    }
}

void UIPopupBox::updateTitleIcon()
{
    /* Assign title-icon: */
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
    m_pTitleIcon->setPixmap(m_titleIcon.pixmap(iIconMetric, iIconMetric));
}

void UIPopupBox::updateWarningIcon()
{
    /* Hide warning-icon if its null: */
    m_pWarningIcon->setHidden(m_warningIcon.isNull());
    /* Assign warning-icon: */
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
    m_pWarningIcon->setPixmap(m_warningIcon.pixmap(iIconMetric, iIconMetric));
}

void UIPopupBox::updateTitle()
{
    /* If title-link is disabled or not set: */
    if (!m_fLinkEnabled || m_strLink.isEmpty())
    {
        /* We should just set simple text title: */
        m_pTitleLabel->setText(QString("<b>%1</b>").arg(m_strTitle));
    }
    /* If title-link is enabled and set: */
    else if (m_fLinkEnabled && !m_strLink.isEmpty())
    {
        /* We should set html reference title: */
        QPalette pal = m_pTitleLabel->palette();
        m_pTitleLabel->setText(QString("<b><a style=\"text-decoration: none; color: %1\" href=\"%2\">%3</a></b>")
                               .arg(m_fHeaderHover ? pal.color(QPalette::Link).name() : pal.color(QPalette::WindowText).name())
                               .arg(m_strLink)
                               .arg(m_strTitle));
    }
}

void UIPopupBox::updateHover()
{
    /* Calculate new header-hover state: */
    bool fNewHeaderHover = m_fHeaderHover;
    if (m_pLabelPath && m_pLabelPath->contains(mapFromGlobal(QCursor::pos())))
        fNewHeaderHover = true;
    else
        fNewHeaderHover = false;

    /* Check if we should toggle hover: */
    if (m_fHeaderHover == fNewHeaderHover)
        return;

    /* If header-hover state switched from disabled to enabled: */
    if (!m_fHeaderHover && fNewHeaderHover)
        /* Notify listeners: */
        emit sigGotHover();

    /* Toggle hover: */
    toggleHover(fNewHeaderHover);
}

void UIPopupBox::revokeHover()
{
    /* Check if we should toggle hover: */
    if (m_fHeaderHover == false)
        return;

    /* Toggle hover off: */
    toggleHover(false);
}

void UIPopupBox::toggleHover(bool fHeaderHover)
{
    /* Remember header-hover state: */
    m_fHeaderHover = fHeaderHover;

    /* Update title: */
    updateTitle();

    /* Call for update: */
    update();
}

void UIPopupBox::recalc()
{
    if (m_pLabelPath)
        delete m_pLabelPath;
    QRect rect = QRect(QPoint(0, 0), size()).adjusted(0, 0, -1, -1);
    int d = 18; // 22
    m_pLabelPath = new QPainterPath(QPointF(rect.x() + rect.width() - d, rect.y()));
    m_pLabelPath->arcTo(QRectF(rect.x(), rect.y(), d, d), 90, 90);
    m_pLabelPath->arcTo(QRectF(rect.x(), rect.y() + rect.height() - d, d, d), 180, 90);
    m_pLabelPath->arcTo(QRectF(rect.x() + rect.width() - d, rect.y() + rect.height() - d, d, d), 270, 90);
    m_pLabelPath->arcTo(QRectF(rect.x() + rect.width() - d, rect.y(), d, d), 0, 90);
    m_pLabelPath->closeSubpath();
    update();
}

UIPopupBoxGroup::UIPopupBoxGroup(QObject *pParent)
    : QObject(pParent)
{
}

UIPopupBoxGroup::~UIPopupBoxGroup()
{
    /* Clear the list early: */
    m_list.clear();
}

void UIPopupBoxGroup::addPopupBox(UIPopupBox *pPopupBox)
{
    /* Add popup-box into list: */
    m_list << pPopupBox;

    /* Connect got-hover signal of the popup-box to hover-change slot of the popup-box group: */
    connect(pPopupBox, SIGNAL(sigGotHover()), this, SLOT(sltHoverChanged()));
}

void UIPopupBoxGroup::sltHoverChanged()
{
    /* Fetch the sender: */
    UIPopupBox *pPopupBox = qobject_cast<UIPopupBox*>(sender());

    /* Check if sender popup-box exists/registered: */
    if (!pPopupBox || !m_list.contains(pPopupBox))
        return;

    /* Filter the sender: */
    QList<UIPopupBox*> list(m_list);
    list.removeOne(pPopupBox);

    /* Notify all other popup-boxes: */
    for (int i = 0; i < list.size(); ++i)
        list[i]->revokeHover();
}


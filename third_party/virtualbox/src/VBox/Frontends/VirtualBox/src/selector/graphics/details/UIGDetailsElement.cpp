/* $Id: UIGDetailsElement.cpp $ */
/** @file
 * VBox Qt GUI - UIGDetailsElement class implementation.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
# include <QGraphicsView>
# include <QStateMachine>
# include <QPropertyAnimation>
# include <QSignalTransition>
# include <QStyleOptionGraphicsItem>
# include <QGraphicsSceneMouseEvent>

/* GUI includes: */
# include "UIGDetailsElement.h"
# include "UIGDetailsSet.h"
# include "UIGDetailsModel.h"
# include "UIGraphicsRotatorButton.h"
# include "UIGraphicsTextPane.h"
# include "UIActionPool.h"
# include "UIIconPool.h"
# include "UIConverter.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGDetailsElement::UIGDetailsElement(UIGDetailsSet *pParent, DetailsElementType type, bool fOpened)
    : UIGDetailsItem(pParent)
    , m_pSet(pParent)
    , m_type(type)
    , m_iCornerRadius(QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 2)
    , m_iMinimumHeaderWidth(0)
    , m_iMinimumHeaderHeight(0)
    , m_pButton(0)
    , m_fClosed(!fOpened)
    , m_iAdditionalHeight(0)
    , m_fAnimationRunning(false)
    , m_pTextPane(0)
    , m_fHovered(false)
    , m_fNameHovered(false)
    , m_pHighlightMachine(0)
    , m_pForwardAnimation(0)
    , m_pBackwardAnimation(0)
    , m_iAnimationDuration(400)
    , m_iDefaultDarkness(100)
    , m_iHighlightDarkness(90)
    , m_iAnimationDarkness(m_iDefaultDarkness)
{
    /* Prepare element: */
    prepareElement();
    /* Prepare button: */
    prepareButton();
    /* Prepare text-pane: */
    prepareTextPane();

    /* Setup size-policy: */
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    /* Add item to the parent: */
    AssertMsg(parentItem(), ("No parent set for details element!"));
    parentItem()->addItem(this);
}

UIGDetailsElement::~UIGDetailsElement()
{
    /* Remove item from the parent: */
    AssertMsg(parentItem(), ("No parent set for details element!"));
    parentItem()->removeItem(this);
}

void UIGDetailsElement::close(bool fAnimated /* = true */)
{
    m_pButton->setToggled(false, fAnimated);
}

void UIGDetailsElement::open(bool fAnimated /* = true */)
{
    m_pButton->setToggled(true, fAnimated);
}

void UIGDetailsElement::updateAppearance()
{
    /* Reset name hover state: */
    m_fNameHovered = false;
    updateNameHoverLink();

    /* Update anchor role restrictions: */
    ConfigurationAccessLevel cal = m_pSet->configurationAccessLevel();
    m_pTextPane->setAnchorRoleRestricted("#mount", cal == ConfigurationAccessLevel_Null);
    m_pTextPane->setAnchorRoleRestricted("#attach", cal != ConfigurationAccessLevel_Full);
}

void UIGDetailsElement::markAnimationFinished()
{
    /* Mark animation as non-running: */
    m_fAnimationRunning = false;

    /* Recursively update size-hint: */
    updateGeometry();
    /* Repaint: */
    update();
}

UITextTable &UIGDetailsElement::text() const
{
    /* Retrieve text from text-pane: */
    return m_pTextPane->text();
}

void UIGDetailsElement::setText(const UITextTable &text)
{
    /* Pass text to text-pane: */
    m_pTextPane->setText(text);
}

void UIGDetailsElement::sltToggleButtonClicked()
{
    emit sigToggleElement(m_type, closed());
}

void UIGDetailsElement::sltElementToggleStart()
{
    /* Mark animation running: */
    m_fAnimationRunning = true;

    /* Setup animation: */
    updateAnimationParameters();

    /* Invert toggle-state: */
    m_fClosed = !m_fClosed;
}

void UIGDetailsElement::sltElementToggleFinish(bool fToggled)
{
    /* Update toggle-state: */
    m_fClosed = !fToggled;

    /* Notify about finishing: */
    emit sigToggleElementFinished();
}

void UIGDetailsElement::sltHandleAnchorClicked(const QString &strAnchor)
{
    /* Current anchor role: */
    const QString strRole = strAnchor.section(',', 0, 0);
    const QString strData = strAnchor.section(',', 1);

    /* Handle known anchor roles: */
    if (   strRole == "#mount"  // Optical and floppy attachments..
        || strRole == "#attach" // Hard-drive attachments..
        )
    {
        /* Prepare storage-menu: */
        UIMenu menu;
        menu.setShowToolTip(true);

        /* Storage-controller name: */
        QString strControllerName = strData.section(',', 0, 0);
        /* Storage-slot: */
        StorageSlot storageSlot = gpConverter->fromString<StorageSlot>(strData.section(',', 1));

        /* Fill storage-menu: */
        vboxGlobal().prepareStorageMenu(menu, this, SLOT(sltMountStorageMedium()),
                                        machine(), strControllerName, storageSlot);

        /* Exec menu: */
        menu.exec(QCursor::pos());
    }
}

void UIGDetailsElement::sltMountStorageMedium()
{
    /* Sender action: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertMsgReturnVoid(pAction, ("This slot should only be called by menu action!\n"));

    /* Current mount-target: */
    const UIMediumTarget target = pAction->data().value<UIMediumTarget>();

    /* Update current machine mount-target: */
    vboxGlobal().updateMachineStorage(machine(), target);
}

void UIGDetailsElement::resizeEvent(QGraphicsSceneResizeEvent*)
{
    /* Update layout: */
    updateLayout();
}

QString UIGDetailsElement::description() const
{
    return tr("%1 details", "like 'General details' or 'Storage details'").arg(m_strName);
}

QVariant UIGDetailsElement::data(int iKey) const
{
    /* Provide other members with required data: */
    switch (iKey)
    {
        /* Hints: */
        case ElementData_Margin: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;
        case ElementData_Spacing: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 2;
        /* Default: */
        default: break;
    }
    return QVariant();
}

void UIGDetailsElement::updateMinimumHeaderWidth()
{
    /* Prepare variables: */
    int iSpacing = data(ElementData_Spacing).toInt();

    /* Update minimum-header-width: */
    m_iMinimumHeaderWidth = m_pixmapSize.width() +
                            iSpacing + m_nameSize.width() +
                            iSpacing + m_buttonSize.width();
}

void UIGDetailsElement::updateMinimumHeaderHeight()
{
    /* Update minimum-header-height: */
    m_iMinimumHeaderHeight = qMax(m_pixmapSize.height(), m_nameSize.height());
    m_iMinimumHeaderHeight = qMax(m_iMinimumHeaderHeight, m_buttonSize.height());
}

void UIGDetailsElement::setIcon(const QIcon &icon)
{
    /* Cache icon: */
    if (icon.isNull())
    {
        /* No icon provided: */
        m_pixmapSize = QSize();
        m_pixmap = QPixmap();
    }
    else
    {
        /* Determine default the icon size: */
        const QStyle *pStyle = QApplication::style();
        const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
        m_pixmapSize = QSize(iIconMetric, iIconMetric);
        /* Acquire the icon of the corresponding size: */
        m_pixmap = icon.pixmap(m_pixmapSize);
    }

    /* Update linked values: */
    updateMinimumHeaderWidth();
    updateMinimumHeaderHeight();
}

void UIGDetailsElement::setName(const QString &strName)
{
    /* Cache name: */
    m_strName = strName;
    QFontMetrics fm(m_nameFont, model()->paintDevice());
    m_nameSize = QSize(fm.width(m_strName), fm.height());

    /* Update linked values: */
    updateMinimumHeaderWidth();
    updateMinimumHeaderHeight();
}

const CMachine& UIGDetailsElement::machine()
{
    return m_pSet->machine();
}

int UIGDetailsElement::minimumWidthHint() const
{
    /* Prepare variables: */
    int iMargin = data(ElementData_Margin).toInt();
    int iMinimumWidthHint = 0;

    /* Maximum width: */
    iMinimumWidthHint = qMax(m_iMinimumHeaderWidth, (int)m_pTextPane->minimumSizeHint().width());

    /* And 4 margins: 2 left and 2 right: */
    iMinimumWidthHint += 4 * iMargin;

    /* Return result: */
    return iMinimumWidthHint;
}

int UIGDetailsElement::minimumHeightHint(bool fClosed) const
{
    /* Prepare variables: */
    int iMargin = data(ElementData_Margin).toInt();
    int iMinimumHeightHint = 0;

    /* Two margins: */
    iMinimumHeightHint += 2 * iMargin;

    /* Header height: */
    iMinimumHeightHint += m_iMinimumHeaderHeight;

    /* Element is opened? */
    if (!fClosed)
    {
        /* Add text height: */
        if (!m_pTextPane->isEmpty())
            iMinimumHeightHint += 2 * iMargin + (int)m_pTextPane->minimumSizeHint().height();
    }

    /* Additional height during animation: */
    if (m_fAnimationRunning)
        iMinimumHeightHint += m_iAdditionalHeight;

    /* Return value: */
    return iMinimumHeightHint;
}

int UIGDetailsElement::minimumHeightHint() const
{
    return minimumHeightHint(m_fClosed);
}

void UIGDetailsElement::updateLayout()
{
    /* Prepare variables: */
    QSize size = geometry().size().toSize();
    int iMargin = data(ElementData_Margin).toInt();

    /* Layout button: */
    int iButtonWidth = m_buttonSize.width();
    int iButtonHeight = m_buttonSize.height();
    int iButtonX = size.width() - 2 * iMargin - iButtonWidth;
    int iButtonY = iButtonHeight == m_iMinimumHeaderHeight ? iMargin :
                   iMargin + (m_iMinimumHeaderHeight - iButtonHeight) / 2;
    m_pButton->setPos(iButtonX, iButtonY);

    /* If closed: */
    if (closed())
    {
        /* Hide text-pane if still visible: */
        if (m_pTextPane->isVisible())
            m_pTextPane->hide();
    }
    /* If opened: */
    else
    {
        /* Layout text-pane: */
        int iTextPaneX = 2 * iMargin;
        int iTextPaneY = iMargin + m_iMinimumHeaderHeight + 2 * iMargin;
        m_pTextPane->setPos(iTextPaneX, iTextPaneY);
        m_pTextPane->resize(size.width() - 4 * iMargin,
                            size.height() - 4 * iMargin - m_iMinimumHeaderHeight);
        /* Show text-pane if still invisible and animation finished: */
        if (!m_pTextPane->isVisible() && !isAnimationRunning())
            m_pTextPane->show();
    }
}

void UIGDetailsElement::setAdditionalHeight(int iAdditionalHeight)
{
    /* Cache new value: */
    m_iAdditionalHeight = iAdditionalHeight;
    /* Update layout: */
    updateLayout();
    /* Repaint: */
    update();
}

void UIGDetailsElement::addItem(UIGDetailsItem*)
{
    AssertMsgFailed(("Details element do NOT support children!"));
}

void UIGDetailsElement::removeItem(UIGDetailsItem*)
{
    AssertMsgFailed(("Details element do NOT support children!"));
}

QList<UIGDetailsItem*> UIGDetailsElement::items(UIGDetailsItemType) const
{
    AssertMsgFailed(("Details element do NOT support children!"));
    return QList<UIGDetailsItem*>();
}

bool UIGDetailsElement::hasItems(UIGDetailsItemType) const
{
    AssertMsgFailed(("Details element do NOT support children!"));
    return false;
}

void UIGDetailsElement::clearItems(UIGDetailsItemType)
{
    AssertMsgFailed(("Details element do NOT support children!"));
}

void UIGDetailsElement::prepareElement()
{
    /* Initialization: */
    m_nameFont = font();
    m_nameFont.setWeight(QFont::Bold);
    m_textFont = font();

    /* Create highlight machine: */
    m_pHighlightMachine = new QStateMachine(this);
    /* Create 'default' state: */
    QState *pStateDefault = new QState(m_pHighlightMachine);
    pStateDefault->assignProperty(this, "animationDarkness", m_iDefaultDarkness);
    /* Create 'highlighted' state: */
    QState *pStateHighlighted = new QState(m_pHighlightMachine);
    pStateHighlighted->assignProperty(this, "animationDarkness", m_iHighlightDarkness);

    /* Forward animation: */
    m_pForwardAnimation = new QPropertyAnimation(this, "animationDarkness", this);
    m_pForwardAnimation->setDuration(m_iAnimationDuration);
    m_pForwardAnimation->setStartValue(m_iDefaultDarkness);
    m_pForwardAnimation->setEndValue(m_iHighlightDarkness);

    /* Backward animation: */
    m_pBackwardAnimation = new QPropertyAnimation(this, "animationDarkness", this);
    m_pBackwardAnimation->setDuration(m_iAnimationDuration);
    m_pBackwardAnimation->setStartValue(m_iHighlightDarkness);
    m_pBackwardAnimation->setEndValue(m_iDefaultDarkness);

    /* Add state transitions: */
    QSignalTransition *pDefaultToHighlighted = pStateDefault->addTransition(this, SIGNAL(sigHoverEnter()), pStateHighlighted);
    pDefaultToHighlighted->addAnimation(m_pForwardAnimation);
    QSignalTransition *pHighlightedToDefault = pStateHighlighted->addTransition(this, SIGNAL(sigHoverLeave()), pStateDefault);
    pHighlightedToDefault->addAnimation(m_pBackwardAnimation);

    /* Initial state is 'default': */
    m_pHighlightMachine->setInitialState(pStateDefault);
    /* Start state-machine: */
    m_pHighlightMachine->start();

    connect(this, SIGNAL(sigToggleElement(DetailsElementType, bool)), model(), SLOT(sltToggleElements(DetailsElementType, bool)));
    connect(this, SIGNAL(sigLinkClicked(const QString&, const QString&, const QString&)),
            model(), SIGNAL(sigLinkClicked(const QString&, const QString&, const QString&)));
}

void UIGDetailsElement::prepareButton()
{
    /* Setup toggle-button: */
    m_pButton = new UIGraphicsRotatorButton(this, "additionalHeight", !m_fClosed, true /* reflected */);
    m_pButton->setAutoHandleButtonClick(false);
    connect(m_pButton, SIGNAL(sigButtonClicked()), this, SLOT(sltToggleButtonClicked()));
    connect(m_pButton, SIGNAL(sigRotationStart()), this, SLOT(sltElementToggleStart()));
    connect(m_pButton, SIGNAL(sigRotationFinish(bool)), this, SLOT(sltElementToggleFinish(bool)));
    m_buttonSize = m_pButton->minimumSizeHint().toSize();
}

void UIGDetailsElement::prepareTextPane()
{
    /* Create text-pane: */
    m_pTextPane = new UIGraphicsTextPane(this, model()->paintDevice());
    connect(m_pTextPane, SIGNAL(sigGeometryChanged()), this, SLOT(sltUpdateGeometry()));
    connect(m_pTextPane, SIGNAL(sigAnchorClicked(const QString&)), this, SLOT(sltHandleAnchorClicked(const QString&)));
}

void UIGDetailsElement::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget*)
{
    /* Update button visibility: */
    updateButtonVisibility();

    /* Configure painter shape: */
    configurePainterShape(pPainter, pOption, m_iCornerRadius);

    /* Paint decorations: */
    paintDecorations(pPainter, pOption);

    /* Paint element info: */
    paintElementInfo(pPainter, pOption);
}

void UIGDetailsElement::paintDecorations(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption)
{
    /* Paint background: */
    paintBackground(pPainter, pOption);
}

void UIGDetailsElement::paintElementInfo(QPainter *pPainter, const QStyleOptionGraphicsItem*)
{
    /* Initialize some necessary variables: */
    int iMargin = data(ElementData_Margin).toInt();
    int iSpacing = data(ElementData_Spacing).toInt();

    /* Calculate attributes: */
    int iPixmapHeight = m_pixmapSize.height();
    int iNameHeight = m_nameSize.height();
    int iMaximumHeight = qMax(iPixmapHeight, iNameHeight);

    /* Prepare color: */
    QPalette pal = palette();
    QColor buttonTextColor = pal.color(QPalette::Active, QPalette::ButtonText);
    QColor linkTextColor = pal.color(QPalette::Active, QPalette::Link);

    /* Paint pixmap: */
    int iElementPixmapX = 2 * iMargin;
    int iElementPixmapY = iPixmapHeight == iMaximumHeight ?
                          iMargin : iMargin + (iMaximumHeight - iPixmapHeight) / 2;
    paintPixmap(/* Painter: */
                pPainter,
                /* Rectangle to paint in: */
                QRect(QPoint(iElementPixmapX, iElementPixmapY), m_pixmapSize),
                /* Pixmap to paint: */
                m_pixmap);

    /* Paint name: */
    int iMachineNameX = iElementPixmapX +
                        m_pixmapSize.width() +
                        iSpacing;
    int iMachineNameY = iNameHeight == iMaximumHeight ?
                        iMargin : iMargin + (iMaximumHeight - iNameHeight) / 2;
    paintText(/* Painter: */
              pPainter,
              /* Rectangle to paint in: */
              QPoint(iMachineNameX, iMachineNameY),
              /* Font to paint text: */
              m_nameFont,
              /* Paint device: */
              model()->paintDevice(),
              /* Text to paint: */
              m_strName,
              /* Name hovered? */
              m_fNameHovered ? linkTextColor : buttonTextColor);
}

void UIGDetailsElement::paintBackground(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption)
{
    /* Save painter: */
    pPainter->save();

    /* Prepare variables: */
    int iMargin = data(ElementData_Margin).toInt();
    int iHeaderHeight = 2 * iMargin + m_iMinimumHeaderHeight;
    QRect optionRect = pOption->rect;
    QRect fullRect = !m_fAnimationRunning ? optionRect :
                     QRect(optionRect.topLeft(), QSize(optionRect.width(), iHeaderHeight + m_iAdditionalHeight));
    int iFullHeight = fullRect.height();

    /* Prepare color: */
    QPalette pal = palette();
    QColor headerColor = pal.color(QPalette::Active, QPalette::Button);
    QColor strokeColor = pal.color(QPalette::Active, QPalette::Mid);
    QColor bodyColor = pal.color(QPalette::Active, QPalette::Base);

    /* Add clipping: */
    QPainterPath path;
    path.moveTo(m_iCornerRadius, 0);
    path.arcTo(QRectF(path.currentPosition(), QSizeF(2 * m_iCornerRadius, 2 * m_iCornerRadius)).translated(-m_iCornerRadius, 0), 90, 90);
    path.lineTo(path.currentPosition().x(), iFullHeight - m_iCornerRadius);
    path.arcTo(QRectF(path.currentPosition(), QSizeF(2 * m_iCornerRadius, 2 * m_iCornerRadius)).translated(0, -m_iCornerRadius), 180, 90);
    path.lineTo(fullRect.width() - m_iCornerRadius, path.currentPosition().y());
    path.arcTo(QRectF(path.currentPosition(), QSizeF(2 * m_iCornerRadius, 2 * m_iCornerRadius)).translated(-m_iCornerRadius, -2 * m_iCornerRadius), 270, 90);
    path.lineTo(path.currentPosition().x(), m_iCornerRadius);
    path.arcTo(QRectF(path.currentPosition(), QSizeF(2 * m_iCornerRadius, 2 * m_iCornerRadius)).translated(-2 * m_iCornerRadius, -m_iCornerRadius), 0, 90);
    path.closeSubpath();
    pPainter->setClipPath(path);

    /* Calculate top rectangle: */
    QRect tRect = fullRect;
    tRect.setBottom(tRect.top() + iHeaderHeight);
    /* Calculate bottom rectangle: */
    QRect bRect = fullRect;
    bRect.setTop(tRect.bottom());

    /* Prepare top gradient: */
    QLinearGradient tGradient(tRect.bottomLeft(), tRect.topLeft());
    tGradient.setColorAt(0, headerColor.darker(110));
    tGradient.setColorAt(1, headerColor.darker(animationDarkness()));

    /* Paint all the stuff: */
    pPainter->fillRect(tRect, tGradient);
    pPainter->fillRect(bRect, bodyColor);

    /* Stroke path: */
    pPainter->setClipping(false);
    pPainter->strokePath(path, strokeColor);

    /* Restore painter: */
    pPainter->restore();
}

void UIGDetailsElement::hoverMoveEvent(QGraphicsSceneHoverEvent *pEvent)
{
    /* Update hover state: */
    if (!m_fHovered)
    {
        m_fHovered = true;
        emit sigHoverEnter();
    }

    /* Update name-hover state: */
    handleHoverEvent(pEvent);
}

void UIGDetailsElement::hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent)
{
    /* Update hover state: */
    if (m_fHovered)
    {
        m_fHovered = false;
        emit sigHoverLeave();
    }

    /* Update name-hover state: */
    handleHoverEvent(pEvent);
}

void UIGDetailsElement::mousePressEvent(QGraphicsSceneMouseEvent *pEvent)
{
    /* Only for hovered header: */
    if (!m_fNameHovered)
        return;

    /* Process link click: */
    pEvent->accept();
    QString strCategory;
    if (m_type >= DetailsElementType_General &&
        m_type < DetailsElementType_Description)
        strCategory = QString("#%1").arg(gpConverter->toInternalString(m_type));
    else if (m_type == DetailsElementType_Description)
        strCategory = QString("#%1%%mTeDescription").arg(gpConverter->toInternalString(m_type));
    emit sigLinkClicked(strCategory, QString(), machine().GetId());
}

void UIGDetailsElement::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *pEvent)
{
    /* Only for left-button: */
    if (pEvent->button() != Qt::LeftButton)
        return;

    /* Process left-button double-click: */
    emit sigToggleElement(m_type, closed());
}

void UIGDetailsElement::updateButtonVisibility()
{
    if (m_fHovered && !m_pButton->isVisible())
        m_pButton->show();
    else if (!m_fHovered && m_pButton->isVisible())
        m_pButton->hide();
}

void UIGDetailsElement::handleHoverEvent(QGraphicsSceneHoverEvent *pEvent)
{
    /* Not for 'preview' element type: */
    if (m_type == DetailsElementType_Preview)
        return;

    /* Prepare variables: */
    int iMargin = data(ElementData_Margin).toInt();
    int iSpacing = data(ElementData_Spacing).toInt();
    int iNameHeight = m_nameSize.height();
    int iElementNameX = 2 * iMargin + m_pixmapSize.width() + iSpacing;
    int iElementNameY = iNameHeight == m_iMinimumHeaderHeight ?
                        iMargin : iMargin + (m_iMinimumHeaderHeight - iNameHeight) / 2;

    /* Simulate hyperlink hovering: */
    QPoint point = pEvent->pos().toPoint();
    bool fNameHovered = QRect(QPoint(iElementNameX, iElementNameY), m_nameSize).contains(point);
    if (   m_pSet->configurationAccessLevel() != ConfigurationAccessLevel_Null
        && m_fNameHovered != fNameHovered)
    {
        m_fNameHovered = fNameHovered;
        updateNameHoverLink();
    }
}

void UIGDetailsElement::updateNameHoverLink()
{
    if (m_fNameHovered)
        VBoxGlobal::setCursor(this, Qt::PointingHandCursor);
    else
        VBoxGlobal::unsetCursor(this);
    update();
}

void UIGDetailsElement::updateAnimationParameters()
{
    /* Recalculate animation parameters: */
    int iOpenedHeight = minimumHeightHint(false);
    int iClosedHeight = minimumHeightHint(true);
    int iAdditionalHeight = iOpenedHeight - iClosedHeight;
    if (m_fClosed)
        m_iAdditionalHeight = 0;
    else
        m_iAdditionalHeight = iAdditionalHeight;
    m_pButton->setAnimationRange(0, iAdditionalHeight);
}

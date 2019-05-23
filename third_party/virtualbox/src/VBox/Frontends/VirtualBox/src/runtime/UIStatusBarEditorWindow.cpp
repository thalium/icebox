/* $Id: UIStatusBarEditorWindow.cpp $ */
/** @file
 * VBox Qt GUI - UIStatusBarEditorWindow class implementation.
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
# include <QAccessibleWidget>
# include <QStylePainter>
# include <QStyleOption>
# include <QHBoxLayout>
# include <QPaintEvent>
# include <QMouseEvent>
# include <QStatusBar>
# include <QCheckBox>
# include <QMimeData>
# include <QPainter>
# include <QPixmap>
# include <QDrag>

/* GUI includes: */
# include "UIStatusBarEditorWindow.h"
# include "UIExtraDataManager.h"
# include "UIMachineWindow.h"
# include "UIConverter.h"
# include "UIIconPool.h"
# include "QIToolButton.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QWidget extension
  * used as status-bar editor button. */
class UIStatusBarEditorButton : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies about click. */
    void sigClick();

    /** Notifies about drag-object destruction. */
    void sigDragObjectDestroy();

public:

    /** Holds the mime-type for the D&D system. */
    static const QString MimeType;

    /** Constructor for the button of passed @a type. */
    UIStatusBarEditorButton(IndicatorType type);

    /** Returns button type. */
    IndicatorType type() const { return m_type; }

    /** Returns button size-hint. */
    QSize sizeHint() const { return m_size; }

    /** Returns whether button is checked. */
    bool isChecked() const;
    /** Defines whether button is @a fChecked. */
    void setChecked(bool fChecked);

private:

    /** Retranslation routine. */
    virtual void retranslateUi();

    /** Paint-event handler. */
    virtual void paintEvent(QPaintEvent *pEvent);

    /** Mouse-press event handler. */
    virtual void mousePressEvent(QMouseEvent *pEvent);
    /** Mouse-release event handler. */
    virtual void mouseReleaseEvent(QMouseEvent *pEvent);
    /** Mouse-enter event handler. */
    virtual void enterEvent(QEvent *pEvent);
    /** Mouse-leave event handler. */
    virtual void leaveEvent(QEvent *pEvent);
    /** Mouse-move event handler. */
    virtual void mouseMoveEvent(QMouseEvent *pEvent);

    /** Holds the button type. */
    IndicatorType m_type;
    /** Holds the button size. */
    QSize m_size;
    /** Holds the button pixmap. */
    QPixmap m_pixmap;
    /** Holds the button pixmap size. */
    QSize m_pixmapSize;
    /** Holds whether button is checked. */
    bool m_fChecked;
    /** Holds whether button is hovered. */
    bool m_fHovered;
    /** Holds the last mouse-press position. */
    QPoint m_mousePressPosition;
};


/** QAccessibleWidget extension used as an accessibility interface for UIStatusBarEditor buttons. */
class UIAccessibilityInterfaceForUIStatusBarEditorButton : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject);

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUIStatusBarEditorButton(QWidget *pWidget);

    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */;

    /** Returns the state. */
    virtual QAccessible::State state() const /* override */;

private:

    /** Returns corresponding toolbar button. */
    UIStatusBarEditorButton *button() const { return qobject_cast<UIStatusBarEditorButton*>(widget()); }
};


/*********************************************************************************************************************************
*   Class UIAccessibilityInterfaceForUIStatusBarEditorButton implementation.                                                     *
*********************************************************************************************************************************/

/* static */
QAccessibleInterface *UIAccessibilityInterfaceForUIStatusBarEditorButton::pFactory(const QString &strClassname, QObject *pObject)
{
    /* Creating toolbar button accessibility interface: */
    if (   pObject
        && strClassname == QLatin1String("UIStatusBarEditorButton"))
        return new UIAccessibilityInterfaceForUIStatusBarEditorButton(qobject_cast<QWidget*>(pObject));

    /* Null by default: */
    return 0;
}

UIAccessibilityInterfaceForUIStatusBarEditorButton::UIAccessibilityInterfaceForUIStatusBarEditorButton(QWidget *pWidget)
    : QAccessibleWidget(pWidget, QAccessible::CheckBox)
{
}

QString UIAccessibilityInterfaceForUIStatusBarEditorButton::text(QAccessible::Text /* enmTextRole */) const
{
    /* Make sure view still alive: */
    AssertPtrReturn(button(), QString());

    /* Return view tool-tip: */
    return gpConverter->toString(button()->type());
}

QAccessible::State UIAccessibilityInterfaceForUIStatusBarEditorButton::state() const /* override */
{
    /* Prepare the button state: */
    QAccessible::State state;

    /* Make sure button still alive: */
    AssertPtrReturn(button(), state);

    /* Compose the button state: */
    state.checkable = true;
    state.checked = button()->isChecked();

    /* Return the button state: */
    return state;
}



/*********************************************************************************************************************************
*   Class UIStatusBarEditorButton implementation.                                                                                *
*********************************************************************************************************************************/

/* static */
const QString UIStatusBarEditorButton::MimeType = QString("application/virtualbox;value=IndicatorType");

UIStatusBarEditorButton::UIStatusBarEditorButton(IndicatorType type)
    : m_type(type)
    , m_fChecked(false)
    , m_fHovered(false)
{
    /* Track mouse events: */
    setMouseTracking(true);

    /* Prepare icon for assigned type: */
    const QIcon icon = gpConverter->toIcon(m_type);
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
    m_pixmapSize = QSize(iIconMetric, iIconMetric);
    m_pixmap = icon.pixmap(m_pixmapSize);

    /* Cache button size-hint: */
    QStyleOptionButton option;
    option.initFrom(this);
    const QRect minRect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option);
    m_size = m_pixmapSize.expandedTo(minRect.size());

    /* Translate finally: */
    retranslateUi();
}

bool UIStatusBarEditorButton::isChecked() const
{
    return m_fChecked;
}

void UIStatusBarEditorButton::setChecked(bool fChecked)
{
    /* Update 'checked' state: */
    m_fChecked = fChecked;
    /* Update: */
    update();
}

void UIStatusBarEditorButton::retranslateUi()
{
    /* Translate tool-tip: */
    setToolTip(UIStatusBarEditorWidget::tr("<nobr><b>Click</b> to toggle indicator presence.</nobr><br>"
                                           "<nobr><b>Drag&Drop</b> to change indicator position.</nobr>"));
}

void UIStatusBarEditorButton::paintEvent(QPaintEvent*)
{
    /* Create style-painter: */
    QStylePainter painter(this);
    /* Prepare option set for check-box: */
    QStyleOptionButton option;
    option.initFrom(this);
    /* Use the size of 'this': */
    option.rect = QRect(0, 0, width(), height());
    /* But do not use hover bit of 'this' since
     * we already have another hovered-state representation: */
    if (option.state & QStyle::State_MouseOver)
        option.state &= ~QStyle::State_MouseOver;
    /* Remember checked-state: */
    if (m_fChecked)
        option.state |= QStyle::State_On;
    /* Draw check-box for hovered-state: */
    if (m_fHovered)
        painter.drawControl(QStyle::CE_CheckBox, option);
    /* Draw pixmap for unhovered-state: */
    else
    {
        QRect pixmapRect = QRect(QPoint(0, 0), m_pixmapSize);
        pixmapRect.moveCenter(option.rect.center());
        painter.drawItemPixmap(pixmapRect, Qt::AlignCenter, m_pixmap);
    }
}

void UIStatusBarEditorButton::mousePressEvent(QMouseEvent *pEvent)
{
    /* We are interested in left button only: */
    if (pEvent->button() != Qt::LeftButton)
        return;

    /* Remember mouse-press position: */
    m_mousePressPosition = pEvent->globalPos();
}

void UIStatusBarEditorButton::mouseReleaseEvent(QMouseEvent *pEvent)
{
    /* We are interested in left button only: */
    if (pEvent->button() != Qt::LeftButton)
        return;

    /* Forget mouse-press position: */
    m_mousePressPosition = QPoint();

    /* Notify about click: */
    emit sigClick();
}

void UIStatusBarEditorButton::enterEvent(QEvent*)
{
    /* Make sure button isn't hovered: */
    if (m_fHovered)
        return;

    /* Invert hovered state: */
    m_fHovered = true;
    /* Update: */
    update();
}

void UIStatusBarEditorButton::leaveEvent(QEvent*)
{
    /* Make sure button is hovered: */
    if (!m_fHovered)
        return;

    /* Invert hovered state: */
    m_fHovered = false;
    /* Update: */
    update();
}

void UIStatusBarEditorButton::mouseMoveEvent(QMouseEvent *pEvent)
{
    /* Make sure item isn't already dragged: */
    if (m_mousePressPosition.isNull())
        return QWidget::mouseMoveEvent(pEvent);

    /* Make sure item is really dragged: */
    if (QLineF(pEvent->globalPos(), m_mousePressPosition).length() <
        QApplication::startDragDistance())
        return QWidget::mouseMoveEvent(pEvent);

    /* Revoke hovered state: */
    m_fHovered = false;
    /* Update: */
    update();

    /* Initialize dragging: */
    m_mousePressPosition = QPoint();
    QDrag *pDrag = new QDrag(this);
    connect(pDrag, SIGNAL(destroyed(QObject*)), this, SIGNAL(sigDragObjectDestroy()));
    QMimeData *pMimeData = new QMimeData;
    pMimeData->setData(MimeType, gpConverter->toInternalString(m_type).toLatin1());
    pDrag->setMimeData(pMimeData);
    pDrag->setPixmap(m_pixmap);
    pDrag->exec();
}


UIStatusBarEditorWidget::UIStatusBarEditorWidget(QWidget *pParent,
                                                 bool fStartedFromVMSettings /* = true */,
                                                 const QString &strMachineID /* = QString() */)
    : QIWithRetranslateUI2<QWidget>(pParent)
    , m_fPrepared(false)
    , m_fStartedFromVMSettings(fStartedFromVMSettings)
    , m_strMachineID(strMachineID)
    , m_pMainLayout(0), m_pButtonLayout(0)
    , m_pButtonClose(0)
    , m_pCheckBoxEnable(0)
    , m_pButtonDropToken(0)
    , m_fDropAfterTokenButton(true)
{
    /* Prepare: */
    prepare();
}

void UIStatusBarEditorWidget::setMachineID(const QString &strMachineID)
{
    /* Remember new machine ID: */
    m_strMachineID = strMachineID;
    /* Prepare: */
    prepare();
}

bool UIStatusBarEditorWidget::isStatusBarEnabled() const
{
    /* For VM settings only: */
    AssertReturn(m_fStartedFromVMSettings, false);

    /* Acquire enable-checkbox if possible: */
    AssertPtrReturn(m_pCheckBoxEnable, false);
    return m_pCheckBoxEnable->isChecked();
}

void UIStatusBarEditorWidget::setStatusBarEnabled(bool fEnabled)
{
    /* For VM settings only: */
    AssertReturnVoid(m_fStartedFromVMSettings);

    /* Update enable-checkbox if possible: */
    AssertPtrReturnVoid(m_pCheckBoxEnable);
    m_pCheckBoxEnable->setChecked(fEnabled);
}

void UIStatusBarEditorWidget::setStatusBarConfiguration(const QList<IndicatorType> &restrictions,
                                                        const QList<IndicatorType> &order)
{
    /* Cache passed restrictions: */
    m_restrictions = restrictions;

    /* Cache passed order: */
    m_order = order;
    /* Append order with missed indicators: */
    for (int iType = IndicatorType_Invalid; iType < IndicatorType_Max; ++iType)
        if (iType != IndicatorType_Invalid && iType != IndicatorType_KeyboardExtension &&
            !m_order.contains((IndicatorType)iType))
            m_order << (IndicatorType)iType;

    /* Update configuration for all existing buttons: */
    foreach (const IndicatorType &type, m_order)
    {
        /* Get button: */
        UIStatusBarEditorButton *pButton = m_buttons.value(type);
        /* Make sure button exists: */
        if (!pButton)
            continue;
        /* Update button 'checked' state: */
        pButton->setChecked(!m_restrictions.contains(type));
        /* Make sure it have valid position: */
        const int iWantedIndex = position(type);
        const int iActualIndex = m_pButtonLayout->indexOf(pButton);
        if (iActualIndex != iWantedIndex)
        {
            /* Re-inject button into main-layout at proper position: */
            m_pButtonLayout->removeWidget(pButton);
            m_pButtonLayout->insertWidget(iWantedIndex, pButton);
        }
    }
}

void UIStatusBarEditorWidget::sltHandleConfigurationChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (machineID() != strMachineID)
        return;

    /* Recache status-bar configuration: */
    setStatusBarConfiguration(gEDataManager->restrictedStatusBarIndicators(machineID()),
                              gEDataManager->statusBarIndicatorOrder(machineID()));
}

void UIStatusBarEditorWidget::sltHandleButtonClick()
{
    /* Make sure sender is valid: */
    UIStatusBarEditorButton *pButton = qobject_cast<UIStatusBarEditorButton*>(sender());
    AssertPtrReturnVoid(pButton);

    /* Get sender type: */
    const IndicatorType type = pButton->type();

    /* Invert restriction for sender type: */
    if (m_restrictions.contains(type))
        m_restrictions.removeAll(type);
    else
        m_restrictions.append(type);

    if (m_fStartedFromVMSettings)
    {
        /* Reapply status-bar configuration from cache: */
        setStatusBarConfiguration(m_restrictions, m_order);
    }
    else
    {
        /* Save updated status-bar indicator restrictions: */
        gEDataManager->setRestrictedStatusBarIndicators(m_restrictions, machineID());
    }
}

void UIStatusBarEditorWidget::sltHandleDragObjectDestroy()
{
    /* Reset token: */
    m_pButtonDropToken = 0;
    m_fDropAfterTokenButton = true;
    /* Update: */
    update();
}

void UIStatusBarEditorWidget::prepare()
{
    /* Do nothing if already prepared: */
    if (m_fPrepared)
        return;

    /* Do not prepare if machine ID is not set: */
    if (m_strMachineID.isEmpty())
        return;

    /* Install tool-bar button accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUIStatusBarEditorButton::pFactory);

    /* Track D&D events: */
    setAcceptDrops(true);

    /* Create main-layout: */
    m_pMainLayout = new QHBoxLayout(this);
    AssertPtrReturnVoid(m_pMainLayout);
    {
        /* Configure main-layout: */
        int iLeft, iTop, iRight, iBottom;
        m_pMainLayout->getContentsMargins(&iLeft, &iTop, &iRight, &iBottom);
        /* Acquire metric: */
        const int iStandardMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 2;
        const int iMinimumMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;
        /* Standard margins should not be too small/large: */
        iLeft   = iStandardMetric;
        iTop    = iStandardMetric;
        iRight  = iStandardMetric;
        iBottom = iStandardMetric;
        /* Bottom margin should be smaller for the common case: */
        if (iBottom >= iMinimumMetric)
            iBottom -= iMinimumMetric;
        /* Left margin should be bigger for the settings case: */
        if (m_fStartedFromVMSettings)
            iLeft += iMinimumMetric;
        /* Apply margins/spacing finally: */
        m_pMainLayout->setContentsMargins(iLeft, iTop, iRight, iBottom);
        m_pMainLayout->setSpacing(0);
        /* Create close-button if necessary: */
        if (!m_fStartedFromVMSettings)
        {
            m_pButtonClose = new QIToolButton;
            AssertPtrReturnVoid(m_pButtonClose);
            {
                /* Configure close-button: */
                m_pButtonClose->setFocusPolicy(Qt::StrongFocus);
                m_pButtonClose->setShortcut(Qt::Key_Escape);
                m_pButtonClose->setIcon(UIIconPool::iconSet(":/ok_16px.png"));
                connect(m_pButtonClose, SIGNAL(clicked(bool)), this, SIGNAL(sigCancelClicked()));
                /* Add close-button into main-layout: */
                m_pMainLayout->addWidget(m_pButtonClose);
            }
        }
        /* Create enable-checkbox if necessary: */
        else
        {
            m_pCheckBoxEnable = new QCheckBox;
            AssertPtrReturnVoid(m_pCheckBoxEnable);
            {
                /* Configure enable-checkbox: */
                m_pCheckBoxEnable->setFocusPolicy(Qt::StrongFocus);
                /* Add enable-checkbox into main-layout: */
                m_pMainLayout->addWidget(m_pCheckBoxEnable);
            }
        }
        /* Insert stretch: */
        m_pMainLayout->addStretch();
        /* Create button-layout: */
        m_pButtonLayout = new QHBoxLayout;
        AssertPtrReturnVoid(m_pButtonLayout);
        {
            /* Configure button-layout: */
            m_pButtonLayout->setContentsMargins(0, 0, 0, 0);
#ifdef VBOX_WS_MAC
            m_pButtonLayout->setSpacing(5);
#else
            m_pButtonLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2);
#endif
            /* Add button-layout into main-layout: */
            m_pMainLayout->addLayout(m_pButtonLayout);
        }
        /* Prepare status buttons: */
        prepareStatusButtons();
    }

    /* Mark as prepared: */
    m_fPrepared = true;

    /* Translate contents: */
    retranslateUi();
}

void UIStatusBarEditorWidget::prepareStatusButtons()
{
    /* Create status buttons: */
    for (int i = IndicatorType_Invalid; i < IndicatorType_Max; ++i)
    {
        /* Get current type: */
        const IndicatorType type = (IndicatorType)i;
        /* Skip inappropriate types: */
        if (type == IndicatorType_Invalid || type == IndicatorType_KeyboardExtension)
            continue;
        /* Create status button: */
        prepareStatusButton(type);
    }

    if (!m_fStartedFromVMSettings)
    {
        /* Cache status-bar configuration: */
        setStatusBarConfiguration(gEDataManager->restrictedStatusBarIndicators(machineID()),
                                  gEDataManager->statusBarIndicatorOrder(machineID()));
        /* And listen for the status-bar configuration changes after that: */
        connect(gEDataManager, SIGNAL(sigStatusBarConfigurationChange(const QString&)),
                this, SLOT(sltHandleConfigurationChange(const QString&)));
    }
}

void UIStatusBarEditorWidget::prepareStatusButton(IndicatorType type)
{
    /* Create status button: */
    UIStatusBarEditorButton *pButton = new UIStatusBarEditorButton(type);
    AssertPtrReturnVoid(pButton);
    {
        /* Configure status button: */
        connect(pButton, SIGNAL(sigClick()), this, SLOT(sltHandleButtonClick()));
        connect(pButton, SIGNAL(sigDragObjectDestroy()), this, SLOT(sltHandleDragObjectDestroy()));
        /* Add status button into button-layout: */
        m_pButtonLayout->addWidget(pButton);
        /* Insert status button into map: */
        m_buttons.insert(type, pButton);
    }
}

void UIStatusBarEditorWidget::retranslateUi()
{
    /* Translate close-button if necessary: */
    if (!m_fStartedFromVMSettings && m_pButtonClose)
        m_pButtonClose->setToolTip(tr("Close"));
    /* Translate enable-checkbox if necessary: */
    if (m_fStartedFromVMSettings && m_pCheckBoxEnable)
        m_pCheckBoxEnable->setToolTip(tr("Enable Status Bar"));
}

void UIStatusBarEditorWidget::paintEvent(QPaintEvent*)
{
    /* Prepare painter: */
    QPainter painter(this);

    /* Prepare palette colors: */
    const QPalette pal = palette();
    QColor color0 = pal.color(QPalette::Window);
    QColor color1 = pal.color(QPalette::Window).lighter(110);
    color1.setAlpha(0);
    QColor color2 = pal.color(QPalette::Window).darker(200);
#if defined(VBOX_WS_WIN) || defined(VBOX_WS_X11)
    QColor color3 = pal.color(QPalette::Window).darker(120);
#endif /* VBOX_WS_WIN || VBOX_WS_X11 */

    /* Acquire metric: */
    const int iMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;

    /* Left corner: */
    QRadialGradient grad1(QPointF(iMetric, iMetric), iMetric);
    {
        grad1.setColorAt(0, color2);
        grad1.setColorAt(1, color1);
    }
    /* Right corner: */
    QRadialGradient grad2(QPointF(width() - iMetric, iMetric), iMetric);
    {
        grad2.setColorAt(0, color2);
        grad2.setColorAt(1, color1);
    }
    /* Top line: */
    QLinearGradient grad3(QPointF(iMetric, 0), QPointF(iMetric, iMetric));
    {
        grad3.setColorAt(0, color1);
        grad3.setColorAt(1, color2);
    }
    /* Left line: */
    QLinearGradient grad4(QPointF(0, iMetric), QPointF(iMetric, iMetric));
    {
        grad4.setColorAt(0, color1);
        grad4.setColorAt(1, color2);
    }
    /* Right line: */
    QLinearGradient grad5(QPointF(width(), iMetric), QPointF(width() - iMetric, iMetric));
    {
        grad5.setColorAt(0, color1);
        grad5.setColorAt(1, color2);
    }

    /* Paint shape/shadow: */
    painter.fillRect(QRect(iMetric, iMetric, width() - iMetric * 2, height() - iMetric), color0); // background
    painter.fillRect(QRect(0,                 0, iMetric, iMetric), grad1); // left corner
    painter.fillRect(QRect(width() - iMetric, 0, iMetric, iMetric), grad2); // right corner
    painter.fillRect(QRect(iMetric,           0, width() - iMetric * 2, iMetric), grad3); // bottom line
    painter.fillRect(QRect(0,                 iMetric, iMetric, height() - iMetric), grad4); // left line
    painter.fillRect(QRect(width() - iMetric, iMetric, iMetric, height() - iMetric), grad5); // right line

#if defined(VBOX_WS_WIN) || defined(VBOX_WS_X11)
    /* Paint frames: */
    painter.save();
    painter.setPen(color3);
    painter.drawLine(QLine(QPoint(iMetric + 1,               iMetric + 1),
                           QPoint(width() - 1 - iMetric - 1, iMetric + 1)));
    painter.drawLine(QLine(QPoint(width() - 1 - iMetric - 1, iMetric + 1),
                           QPoint(width() - 1 - iMetric - 1, height() - 1)));
    painter.drawLine(QLine(QPoint(width() - 1 - iMetric - 1, height() - 1),
                           QPoint(iMetric + 1,               height() - 1)));
    painter.drawLine(QLine(QPoint(iMetric + 1,               height() - 1),
                           QPoint(iMetric + 1,               iMetric + 1)));
    painter.restore();
#endif /* VBOX_WS_WIN || VBOX_WS_X11 */

    /* Paint drop token: */
    if (m_pButtonDropToken)
    {
        QStyleOption option;
        option.state |= QStyle::State_Horizontal;
        const QRect geo = m_pButtonDropToken->geometry();
        option.rect = !m_fDropAfterTokenButton ?
                      QRect(geo.topLeft() - QPoint(iMetric, iMetric),
                            geo.bottomLeft() + QPoint(0, iMetric)) :
                      QRect(geo.topRight() - QPoint(0, iMetric),
                            geo.bottomRight() + QPoint(iMetric, iMetric));
        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator,
                                             &option, &painter);
    }
}

void UIStatusBarEditorWidget::dragEnterEvent(QDragEnterEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);
    /* And mime-data is set: */
    const QMimeData *pMimeData = pEvent->mimeData();
    AssertPtrReturnVoid(pMimeData);
    /* Make sure mime-data format is valid: */
    if (!pMimeData->hasFormat(UIStatusBarEditorButton::MimeType))
        return;

    /* Accept drag-enter event: */
    pEvent->acceptProposedAction();
}

void UIStatusBarEditorWidget::dragMoveEvent(QDragMoveEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);
    /* And mime-data is set: */
    const QMimeData *pMimeData = pEvent->mimeData();
    AssertPtrReturnVoid(pMimeData);
    /* Make sure mime-data format is valid: */
    if (!pMimeData->hasFormat(UIStatusBarEditorButton::MimeType))
        return;

    /* Reset token: */
    m_pButtonDropToken = 0;
    m_fDropAfterTokenButton = true;

    /* Get event position: */
    const QPoint pos = pEvent->pos();
    /* Search for most suitable button: */
    foreach (const IndicatorType &type, m_order)
    {
        m_pButtonDropToken = m_buttons.value(type);
        const QRect geo = m_pButtonDropToken->geometry();
        if (pos.x() < geo.center().x())
        {
            m_fDropAfterTokenButton = false;
            break;
        }
    }
    /* Update: */
    update();
}

void UIStatusBarEditorWidget::dragLeaveEvent(QDragLeaveEvent*)
{
    /* Reset token: */
    m_pButtonDropToken = 0;
    m_fDropAfterTokenButton = true;
    /* Update: */
    update();
}

void UIStatusBarEditorWidget::dropEvent(QDropEvent *pEvent)
{
    /* Make sure event is valid: */
    AssertPtrReturnVoid(pEvent);
    /* And mime-data is set: */
    const QMimeData *pMimeData = pEvent->mimeData();
    AssertPtrReturnVoid(pMimeData);
    /* Make sure mime-data format is valid: */
    if (!pMimeData->hasFormat(UIStatusBarEditorButton::MimeType))
        return;

    /* Make sure token-button set: */
    if (!m_pButtonDropToken)
        return;

    /* Determine type of token-button: */
    const IndicatorType tokenType = m_pButtonDropToken->type();
    /* Determine type of dropped-button: */
    const QString strDroppedType =
        QString::fromLatin1(pMimeData->data(UIStatusBarEditorButton::MimeType));
    const IndicatorType droppedType =
        gpConverter->fromInternalString<IndicatorType>(strDroppedType);

    /* Make sure these types are different: */
    if (droppedType == tokenType)
        return;

    /* Remove type of dropped-button: */
    m_order.removeAll(droppedType);
    /* Insert type of dropped-button into position of token-button: */
    int iPosition = m_order.indexOf(tokenType);
    if (m_fDropAfterTokenButton)
        ++iPosition;
    m_order.insert(iPosition, droppedType);

    if (m_fStartedFromVMSettings)
    {
        /* Reapply status-bar configuration from cache: */
        setStatusBarConfiguration(m_restrictions, m_order);
    }
    else
    {
        /* Save updated status-bar indicator order: */
        gEDataManager->setStatusBarIndicatorOrder(m_order, machineID());
    }
}

int UIStatusBarEditorWidget::position(IndicatorType type) const
{
    int iPosition = 0;
    foreach (const IndicatorType &iteratedType, m_order)
        if (iteratedType == type)
            return iPosition;
        else
            ++iPosition;
    return iPosition;
}


UIStatusBarEditorWindow::UIStatusBarEditorWindow(UIMachineWindow *pParent)
    : UISlidingToolBar(pParent, pParent->statusBar(), new UIStatusBarEditorWidget(0, false, vboxGlobal().managedVMUuid()), UISlidingToolBar::Position_Bottom)
{
}

#include "UIStatusBarEditorWindow.moc"


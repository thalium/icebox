/* $Id: UIDesktopPane.cpp $ */
/** @file
 * VBox Qt GUI - UIDesktopPane class implementation.
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
# include <QAction>
# include <QGridLayout>
# include <QHBoxLayout>
# include <QLabel>
# include <QPainter>
# include <QScrollArea>
# include <QScrollBar>
# include <QStackedWidget>
# include <QStyle>
# include <QToolButton>
# include <QUuid>
# include <QVBoxLayout>

/* GUI includes */
# include "QIWithRetranslateUI.h"
# include "UIDesktopPane.h"
# include "VBoxUtils.h"

/* Other VBox includes: */
# include <iprt/assert.h>

/* Forward declarations: */
class QEvent;
class QGridLayout;
class QHBoxLayout;
class QIcon;
class QLabel;
class QPaintEvent;
class QResizeEvent;
class QScrollArea;
class QString;
class QUuid;
class QVBoxLayout;
class QWidget;

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QScrollArea extension to wrap a tools pane. */
class UIToolScrollArea : public QScrollArea
{
    Q_OBJECT;

public:

    /** Constructs scroll-area passing @a pParent to the base-class. */
    UIToolScrollArea(QWidget *pParent = 0);

protected:

    /** Holds the minimum widget size. */
    virtual QSize minimumSizeHint() const /* override */;
};


/** Wrappable QLabel extension for tools pane of the desktop widget.
  * The main idea behind this stuff is to allow dynamically calculate
  * [minimum] size hint for changeable one-the-fly widget width.
  * That's a "white unicorn" task for QLabel which never worked since
  * the beginning, because out-of-the-box version just uses static
  * hints calculation which is very stupid taking into account
  * QLayout "eats it raw" and tries to be dynamical on it's basis. */
class UILabel : public QLabel
{
    Q_OBJECT;

public:

    /** Constructs scroll-area passing @a pParent to the base-class. */
    UILabel(QWidget *pParent = 0);

protected:

    /** Handles resize @a pEvent. */
    virtual void resizeEvent(QResizeEvent *pEvent) /* override */;

    /** Returns whether the widget's preferred height depends on its width. */
    virtual bool hasHeightForWidth() const /* override */;

    /** Holds the minimum widget size. */
    virtual QSize minimumSizeHint() const /* override */;

    /** Holds the preferred widget size. */
    virtual QSize sizeHint() const /* override */;
};


/** Our own skinnable implementation of tool widget for UIDesktopPane. */
class UIToolWidget : public QWidget
{
    Q_OBJECT;

public:

    /** Constructs tool widget on the basis of passed @a pAction and @a strDescription. */
    UIToolWidget(QAction *pAction, const QString &strDescription);

protected:

    /** Handles any Qt @a pEvent. */
    virtual bool event(QEvent *pEvent) /* override */;

    /** Handles paint @a pEvent. */
    virtual void paintEvent(QPaintEvent *pEvent) /* override */;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the action reference. */
    QAction *m_pAction;
    /** Holds the widget description. */
    QString  m_strDescription;

    /** Holds whether the widget is hovered. */
    bool  m_fHovered;

    /** Holds the main layout instance. */
    QGridLayout *m_pLayout;
    /** Holds the icon label instance. */
    QLabel      *m_pLabelIcon;
    /** Holds the name label instance. */
    QLabel      *m_pLabelName;
    /** Holds the description label instance. */
    QLabel      *m_pLabelDescription;
};


/** QStackedWidget subclass representing container which have two panes:
  * 1. Text pane reflecting base information about VirtualBox,
  * 2. Error pane reflecting information about currently chosen
  *    inaccessible VM and allowing to operate over it. */
class UIDesktopPanePrivate : public QIWithRetranslateUI<QStackedWidget>
{
    Q_OBJECT;

public:

    /** Constructs private desktop pane passing @a pParent to the base-class.
      * @param  pRefreshAction  Brings the refresh action reference. */
    UIDesktopPanePrivate(QWidget *pParent, QAction *pRefreshAction);

    /** Assigns @a strError and switches to error pane. */
    void setError(const QString &strError);

    /** Defines a tools pane welcome @a strText. */
    void setToolsPaneText(const QString &strText);
    /** Defines a tools pane welcome @a icon. */
    void setToolsPaneIcon(const QIcon &icon);
    /** Add a tool element.
      * @param  pAction         Brings tool action reference.
      * @param  strDescription  Brings the tool description. */
    void addToolDescription(QAction *pAction, const QString &strDescription);
    /** Removes all tool elements. */
    void removeToolDescriptions();

protected:

    /** Handles translation event. */
    void retranslateUi();

    /** Prepares error pane. */
    void prepareErrorPane();

    /** Prepares tools pane. */
    void prepareToolsPane();

private:

    /** Holds the error pane container. */
    QWidget *m_pErrBox;
    /** Holds the error label instance. */
    QLabel *m_pErrLabel;
    /** Holds the error text-browser instance. */
    QTextBrowser *m_pErrText;
    /** Holds the VM refresh button instance. */
    QToolButton *m_pRefreshButton;
    /** Holds the VM refresh action reference. */
    QAction *m_pRefreshAction;

    /** Holds the tools pane scroll-area instance. */
    UIToolScrollArea *m_pScrollArea;
    /** Holds the tools pane instance. */
    QWidget          *m_pToolsPane;
    /** Holds the tools pane widget layout instance. */
    QVBoxLayout      *m_pLayoutWidget;
    /** Holds the tools pane text label instance. */
    QLabel           *m_pLabelToolsPaneText;
    /** Holds the tools pane icon label instance. */
    QLabel           *m_pLabelToolsPaneIcon;
};


/*********************************************************************************************************************************
*   Class UIToolScrollArea implementation.                                                                                       *
*********************************************************************************************************************************/

UIToolScrollArea::UIToolScrollArea(QWidget *pParent /* = 0 */)
    : QScrollArea(pParent)
{
}

QSize UIToolScrollArea::minimumSizeHint() const
{
    // WORKAROUND:
    // The idea is simple, hold the minimum size hint to
    // avoid horizontal scroll-bar, but allow vertical one.
    return   widget()
           ? QSize(  widget()->minimumSizeHint().width()
                   + verticalScrollBar()->height(),
                   200 /* what about something more dynamical? */)
           : QScrollArea::minimumSizeHint();
}


/*********************************************************************************************************************************
*   Class UILabel implementation.                                                                                                *
*********************************************************************************************************************************/

UILabel::UILabel(QWidget *pParent /* = 0 */)
    : QLabel(pParent)
{
}

void UILabel::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QLabel::resizeEvent(pEvent);

    // WORKAROUND:
    // That's not cheap procedure but we need it to
    // make sure geometry is updated after width changed.
    if (minimumWidth() > 0)
        updateGeometry();
}

bool UILabel::hasHeightForWidth() const
{
    // WORKAROUND:
    // No need to panic, we do it ourselves in resizeEvent() and
    // this 'false' here to prevent automatic layout fighting for it.
    return   minimumWidth() > 0
           ? false
           : QLabel::hasHeightForWidth();
}

QSize UILabel::minimumSizeHint() const /* override */
{
    // WORKAROUND:
    // We should calculate hint height on the basis of width,
    // keeping the hint width equal to minimum we have set.
    return   minimumWidth() > 0
           ? QSize(minimumWidth(), heightForWidth(width()))
           : QLabel::minimumSizeHint();
}

QSize UILabel::sizeHint() const /* override */
{
    // WORKAROUND:
    // Keep widget always minimal.
    return minimumSizeHint();
}


/*********************************************************************************************************************************
*   Class UIToolWidget implementation.                                                                                           *
*********************************************************************************************************************************/

UIToolWidget::UIToolWidget(QAction *pAction, const QString &strDescription)
    : m_pAction(pAction)
    , m_strDescription(strDescription)
    , m_fHovered(false)
    , m_pLayout(0)
    , m_pLabelIcon(0)
    , m_pLabelName(0)
    , m_pLabelDescription(0)
{
    /* Prepare: */
    prepare();
}

bool UIToolWidget::event(QEvent *pEvent)
{
    /* Handle known event types: */
    switch (pEvent->type())
    {
        /* Update the hovered state on/off: */
        case QEvent::Enter:
        {
            m_fHovered = true;
            update();
            break;
        }
        case QEvent::Leave:
        {
            m_fHovered = false;
            update();
            break;
        }

        /* Notify listeners about the item was clicked: */
        case QEvent::MouseButtonRelease:
        {
            m_pAction->trigger();
            break;
        }

        default:
            break;
    }
    /* Call to base-class: */
    return QWidget::event(pEvent);
}

void UIToolWidget::paintEvent(QPaintEvent * /* pEvent */)
{
    /* Prepare painter: */
    QPainter painter(this);

    /* Prepare palette colors: */
    const QPalette pal = palette();
#ifdef VBOX_WS_MAC
    const QColor color0 = pal.color(QPalette::Base);
    const QColor color11 = pal.color(QPalette::Highlight).lighter(120);
    const QColor color12 = pal.color(QPalette::Highlight);
#else /* !VBOX_WS_MAC */
    const QColor color0 = pal.color(QPalette::Base);
    const QColor color11 = pal.color(QPalette::Highlight).lighter(150);
    const QColor color12 = pal.color(QPalette::Highlight);
#endif /* !VBOX_WS_MAC */
    QColor color2 = pal.color(QPalette::Window).lighter(110);
    color2.setAlpha(0);
    QColor color3 = pal.color(QPalette::Window).darker(200);

    /* Invent pixel metric: */
    const int iMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * 1.375;
    const int iIconMargin = iIconMetric / 2;

    /* Background gradient: */
    QLinearGradient grad(QPointF(width(), 0), QPointF(width() - (2 * iIconMargin + iIconMetric), 0));
    {
        grad.setColorAt(0, color11);
        grad.setColorAt(0.95, color12);
        grad.setColorAt(1, color2);
    }

    /* Top-left corner: */
    QRadialGradient grad1(QPointF(iMetric, iMetric), iMetric);
    {
        grad1.setColorAt(0, color3);
        grad1.setColorAt(1, color2);
    }
    /* Top-right corner: */
    QRadialGradient grad2(QPointF(width() - iMetric, iMetric), iMetric);
    {
        grad2.setColorAt(0, color3);
        grad2.setColorAt(1, color2);
    }
    /* Bottom-left corner: */
    QRadialGradient grad3(QPointF(iMetric, height() - iMetric), iMetric);
    {
        grad3.setColorAt(0, color3);
        grad3.setColorAt(1, color2);
    }
    /* Botom-right corner: */
    QRadialGradient grad4(QPointF(width() - iMetric, height() - iMetric), iMetric);
    {
        grad4.setColorAt(0, color3);
        grad4.setColorAt(1, color2);
    }

    /* Top line: */
    QLinearGradient grad5(QPointF(iMetric, 0), QPointF(iMetric, iMetric));
    {
        grad5.setColorAt(0, color2);
        grad5.setColorAt(1, color3);
    }
    /* Bottom line: */
    QLinearGradient grad6(QPointF(iMetric, height()), QPointF(iMetric, height() - iMetric));
    {
        grad6.setColorAt(0, color2);
        grad6.setColorAt(1, color3);
    }
    /* Left line: */
    QLinearGradient grad7(QPointF(0, height() - iMetric), QPointF(iMetric, height() - iMetric));
    {
        grad7.setColorAt(0, color2);
        grad7.setColorAt(1, color3);
    }
    /* Right line: */
    QLinearGradient grad8(QPointF(width(), height() - iMetric), QPointF(width() - iMetric, height() - iMetric));
    {
        grad8.setColorAt(0, color2);
        grad8.setColorAt(1, color3);
    }

    /* Paint hovered item cursor: */
    if (m_fHovered)
    {
        painter.fillRect(QRect(iMetric, iMetric, width() - iMetric * 2, height() - iMetric * 2), color0);
        painter.fillRect(QRect(width() - (height() - iMetric), iMetric, height() - iMetric * 2, height() - iMetric * 2), grad);
    }

    /* Paint shape/shadow: */
    painter.fillRect(QRect(0,                 0,                  iMetric,               iMetric),                grad1);
    painter.fillRect(QRect(width() - iMetric, 0,                  iMetric,               iMetric),                grad2);
    painter.fillRect(QRect(0,                 height() - iMetric, iMetric,               iMetric),                grad3);
    painter.fillRect(QRect(width() - iMetric, height() - iMetric, iMetric,               iMetric),                grad4);
    painter.fillRect(QRect(iMetric,           0,                  width() - iMetric * 2, iMetric),                grad5);
    painter.fillRect(QRect(iMetric,           height() - iMetric, width() - iMetric * 2, iMetric),                grad6);
    painter.fillRect(QRect(0,                 iMetric,            iMetric,               height() - iMetric * 2), grad7);
    painter.fillRect(QRect(width() - iMetric, iMetric,            iMetric,               height() - iMetric * 2), grad8);
}

void UIToolWidget::prepare()
{
    /* Configure self: */
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    /* Create main layout: */
    m_pLayout = new QGridLayout(this);
    AssertPtrReturnVoid(m_pLayout);
    {
        /* Invent pixel metric: */
        const int iMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * 1.375;
        const int iMargin = iMetric / 2;
        const int iVerticalSpacing = iMargin / 2;
        const int iHorizontalSpacing = iMargin * 2;

        /* Configure layout: */
        m_pLayout->setContentsMargins(iMargin + iVerticalSpacing, iMargin, iMargin, iMargin);
        m_pLayout->setVerticalSpacing(iVerticalSpacing);
        m_pLayout->setHorizontalSpacing(iHorizontalSpacing);

        /* Create name label: */
        m_pLabelName = new QLabel;
        AssertPtrReturnVoid(m_pLabelName);
        {
            /* Configure label: */
            QFont fontLabel = m_pLabelName->font();
            fontLabel.setBold(true);
            m_pLabelName->setFont(fontLabel);
            m_pLabelName->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            m_pLabelName->setText(QString("%1     ").arg(m_pAction->text().remove('&')));

            /* Add into layout: */
            m_pLayout->addWidget(m_pLabelName, 0, 0);
        }

        /* Create description label: */
        m_pLabelDescription = new UILabel;
        AssertPtrReturnVoid(m_pLabelDescription);
        {
            /* Configure label: */
            m_pLabelDescription->setAttribute(Qt::WA_TransparentForMouseEvents);
            m_pLabelDescription->setWordWrap(true);
            m_pLabelDescription->setMinimumWidth(315);
            m_pLabelDescription->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            m_pLabelDescription->setText(m_strDescription);

            /* Add into layout: */
            m_pLayout->addWidget(m_pLabelDescription, 1, 0);
        }

        /* Create icon label: */
        m_pLabelIcon = new QLabel;
        AssertPtrReturnVoid(m_pLabelIcon);
        {
            /* Configure label: */
            m_pLabelIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            m_pLabelIcon->setPixmap(m_pAction->icon().pixmap(iMetric));

            /* Add into layout: */
            m_pLayout->addWidget(m_pLabelIcon, 0, 1, 2, 1);
            m_pLayout->setAlignment(m_pLabelIcon, Qt::AlignCenter);
        }
    }
}


/*********************************************************************************************************************************
*   Class UIDesktopPanePrivate implementation.                                                                                   *
*********************************************************************************************************************************/

UIDesktopPanePrivate::UIDesktopPanePrivate(QWidget *pParent, QAction *pRefreshAction)
    : QIWithRetranslateUI<QStackedWidget>(pParent)
    , m_pErrBox(0), m_pErrLabel(0), m_pErrText(0)
    , m_pRefreshButton(0), m_pRefreshAction(pRefreshAction)
    , m_pScrollArea(0), m_pToolsPane(0), m_pLayoutWidget(0), m_pLabelToolsPaneText(0), m_pLabelToolsPaneIcon(0)
{
    /* Translate finally: */
    retranslateUi();
}

void UIDesktopPanePrivate::setError(const QString &strError)
{
    /* Prepare error pane if necessary: */
    prepareErrorPane();

    /* Assign corresponding text: */
    m_pErrText->setText(strError);

    /* Raise corresponding widget: */
    setCurrentIndex(indexOf(m_pErrBox));
}

void UIDesktopPanePrivate::setToolsPaneText(const QString &strText)
{
    /* Prepare tools pane if necessary: */
    prepareToolsPane();

    /* Assign corresponding text: */
    m_pLabelToolsPaneText->setText(strText);

    /* Raise corresponding widget: */
    setCurrentWidget(m_pScrollArea);
}

void UIDesktopPanePrivate::setToolsPaneIcon(const QIcon &icon)
{
    /* Prepare tools pane if necessary: */
    prepareToolsPane();

    /* Assign corresponding icon: */
    const QList<QSize> aSizes = icon.availableSizes();
    const QSize firstOne = aSizes.isEmpty() ? QSize(200, 200) : aSizes.first();
    const double dRatio = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize) / 32;
    m_pLabelToolsPaneIcon->setPixmap(icon.pixmap(QSize(firstOne.width() * dRatio, firstOne.height() * dRatio)));

    /* Raise corresponding widget: */
    setCurrentWidget(m_pScrollArea);
}

void UIDesktopPanePrivate::addToolDescription(QAction *pAction, const QString &strDescription)
{
    /* Prepare tools pane if necessary: */
    prepareToolsPane();

    /* Add tool widget on the basis of passed description: */
    UIToolWidget *pWidget = new UIToolWidget(pAction, strDescription);
    AssertPtrReturnVoid(pWidget);
    {
        /* Add into layout: */
        m_pLayoutWidget->addWidget(pWidget);
    }

    /* Raise corresponding widget: */
    setCurrentWidget(m_pScrollArea);
}

void UIDesktopPanePrivate::removeToolDescriptions()
{
    /* Clear the layout: */
    QLayoutItem *pChild = 0;
    while ((pChild = m_pLayoutWidget->takeAt(0)) != 0)
    {
        /* Delete widget wrapped by the item first: */
        if (pChild->widget())
            delete pChild->widget();
        /* Then the item itself: */
        delete pChild;
    }
}

void UIDesktopPanePrivate::retranslateUi()
{
    /* Translate error-label text: */
    if (m_pErrLabel)
        m_pErrLabel->setText(QApplication::translate("UIDetailsPagePrivate",
                                 "The selected virtual machine is <i>inaccessible</i>. "
                                 "Please inspect the error message shown below and press the "
                                 "<b>Refresh</b> button if you want to repeat the accessibility check:"));

    /* Translate refresh button & action text: */
    if (m_pRefreshAction && m_pRefreshButton)
    {
        m_pRefreshButton->setText(m_pRefreshAction->text());
        m_pRefreshButton->setIcon(m_pRefreshAction->icon());
        m_pRefreshButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
}

void UIDesktopPanePrivate::prepareErrorPane()
{
    if (m_pErrBox)
        return;

    /* Create error pane: */
    m_pErrBox = new QWidget;

    /* Create main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(m_pErrBox);
#ifdef VBOX_WS_MAC
    pMainLayout->setContentsMargins(4, 5, 5, 5);
#else /* !VBOX_WS_MAC */
    const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 3;
    const int iT = qApp->style()->pixelMetric(QStyle::PM_LayoutTopMargin) / 3;
    const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin) / 3;
    pMainLayout->setContentsMargins(iL, iT, iR, 0);
#endif /* !VBOX_WS_MAC */

    /* Create error label: */
    m_pErrLabel = new QLabel(m_pErrBox);
    m_pErrLabel->setWordWrap(true);
    m_pErrLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pMainLayout->addWidget(m_pErrLabel);

    /* Create error text-browser: */
    m_pErrText = new QTextBrowser(m_pErrBox);
    m_pErrText->setFocusPolicy(Qt::StrongFocus);
    m_pErrText->document()->setDefaultStyleSheet("a { text-decoration: none; }");
    pMainLayout->addWidget(m_pErrText);

    /* If refresh action was set: */
    if (m_pRefreshAction)
    {
        /* Create refresh button: */
        m_pRefreshButton = new QToolButton(m_pErrBox);
        m_pRefreshButton->setFocusPolicy(Qt::StrongFocus);

        /* Create refresh button layout: */
        QHBoxLayout *pButtonLayout = new QHBoxLayout;
        pMainLayout->addLayout(pButtonLayout);
        pButtonLayout->addStretch();
        pButtonLayout->addWidget(m_pRefreshButton);

        /* Connect refresh button: */
        connect(m_pRefreshButton, SIGNAL(clicked()), m_pRefreshAction, SIGNAL(triggered()));
    }

    pMainLayout->addStretch();

    /* Add into the stack: */
    addWidget(m_pErrBox);

    /* Retranslate finally: */
    retranslateUi();
}

void UIDesktopPanePrivate::prepareToolsPane()
{
    /* Do nothing if already exists: */
    if (m_pScrollArea)
        return;

    /* Create scroll-area: */
    m_pScrollArea = new UIToolScrollArea;
    AssertPtrReturnVoid(m_pScrollArea);
    {
        /* Configure scroll-area: */
        m_pScrollArea->setFrameShape(QFrame::NoFrame);
        m_pScrollArea->setWidgetResizable(true);

        /* Create tool pane: */
        m_pToolsPane = new QWidget;
        AssertPtrReturnVoid(m_pToolsPane);
        {
            /* Create main layout: */
            QVBoxLayout *pMainLayout = new QVBoxLayout(m_pToolsPane);
            AssertPtrReturnVoid(pMainLayout);
            {
                /* Create welcome layout: */
                QHBoxLayout *pLayoutWelcome = new QHBoxLayout;
                AssertPtrReturnVoid(pLayoutWelcome);
                {
                    /* Configure layout: */
                    const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2;
                    pLayoutWelcome->setContentsMargins(iL, 0, 0, 0);

                    /* Create welcome text label: */
                    m_pLabelToolsPaneText = new UILabel;
                    AssertPtrReturnVoid(m_pLabelToolsPaneText);
                    {
                        /* Configure label: */
                        m_pLabelToolsPaneText->setWordWrap(true);
                        m_pLabelToolsPaneText->setMinimumWidth(160);
                        m_pLabelToolsPaneText->setAlignment(Qt::AlignLeading | Qt::AlignTop);
                        m_pLabelToolsPaneText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

                        /* Add into layout: */
                        pLayoutWelcome->addWidget(m_pLabelToolsPaneText);
                    }

                    /* Create welcome picture label: */
                    m_pLabelToolsPaneIcon = new QLabel;
                    AssertPtrReturnVoid(m_pLabelToolsPaneIcon);
                    {
                        /* Configure label: */
                        m_pLabelToolsPaneIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                        /* Add into layout: */
                        pLayoutWelcome->addWidget(m_pLabelToolsPaneIcon);
                        pLayoutWelcome->setAlignment(m_pLabelToolsPaneIcon, Qt::AlignHCenter | Qt::AlignTop);
                    }

                    /* Add into layout: */
                    pMainLayout->addLayout(pLayoutWelcome);
                }

                /* Create widget layout: */
                m_pLayoutWidget = new QVBoxLayout;
                AssertPtrReturnVoid(m_pLayoutWidget);
                {
                    /* Add into layout: */
                    pMainLayout->addLayout(m_pLayoutWidget);
                }

                /* Add stretch: */
                pMainLayout->addStretch();
            }

            /* Add into the scroll-area: */
            m_pScrollArea->setWidget(m_pToolsPane);
        }

        /* Add into the stack: */
        addWidget(m_pScrollArea);
    }
}


/*********************************************************************************************************************************
*   Class UIDesktopPane implementation.                                                                                          *
*********************************************************************************************************************************/

UIDesktopPane::UIDesktopPane(QAction *pRefreshAction /* = 0 */, QWidget *pParent /* = 0 */)
    : QWidget(pParent)
{
    /* Prepare main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(0, 0, 0, 0);

    /* Create desktop pane: */
    m_pDesktopPrivate = new UIDesktopPanePrivate(this, pRefreshAction);

    /* Add it to the layout: */
    pMainLayout->addWidget(m_pDesktopPrivate);
}

void UIDesktopPane::updateDetailsError(const QString &strError)
{
    m_pDesktopPrivate->setError(strError);
}

void UIDesktopPane::setToolsPaneText(const QString &strText)
{
    m_pDesktopPrivate->setToolsPaneText(strText);
}

void UIDesktopPane::setToolsPaneIcon(const QIcon &icon)
{
    m_pDesktopPrivate->setToolsPaneIcon(icon);
}

void UIDesktopPane::addToolDescription(QAction *pAction, const QString &strDescription)
{
    m_pDesktopPrivate->addToolDescription(pAction, strDescription);
}

void UIDesktopPane::removeToolDescriptions()
{
    m_pDesktopPrivate->removeToolDescriptions();
}

#include "UIDesktopPane.moc"


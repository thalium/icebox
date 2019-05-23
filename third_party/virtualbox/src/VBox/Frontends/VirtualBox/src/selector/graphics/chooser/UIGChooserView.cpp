/* $Id: UIGChooserView.cpp $ */
/** @file
 * VBox Qt GUI - UIGChooserView class implementation.
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
# include <QScrollBar>
# include <QAccessibleWidget>

/* GUI includes: */
# include "UIGChooser.h"
# include "UIGChooserModel.h"
# include "UIGChooserView.h"
# include "UIGChooserItem.h"

/* Other VBox includes: */
# include <iprt/assert.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QAccessibleWidget extension used as an accessibility interface for Chooser-view. */
class UIAccessibilityInterfaceForUIGChooserView : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating Chooser-view accessibility interface: */
        if (pObject && strClassname == QLatin1String("UIGChooserView"))
            return new UIAccessibilityInterfaceForUIGChooserView(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUIGChooserView(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::List)
    {}

    /** Returns the number of children. */
    virtual int childCount() const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), 0);

        /* Return the number of children: */
        return view()->chooser()->model()->root()->items().size();
    }

    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), 0);
        /* Make sure index is valid: */
        AssertReturn(iIndex >= 0 && iIndex < childCount(), 0);

        /* Return the child with the passed iIndex: */
        return QAccessible::queryAccessibleInterface(view()->chooser()->model()->root()->items().at(iIndex));
    }

    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), QString());

        /* Return view tool-tip: */
        Q_UNUSED(enmTextRole);
        return view()->toolTip();
    }

private:

    /** Returns corresponding Chooser-view. */
    UIGChooserView *view() const { return qobject_cast<UIGChooserView*>(widget()); }
};


UIGChooserView::UIGChooserView(UIGChooser *pParent)
    : QIWithRetranslateUI<QIGraphicsView>(pParent)
    , m_pChooser(pParent)
    , m_iMinimumWidthHint(0)
    , m_iMinimumHeightHint(0)
{
    /* Install Chooser-view accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUIGChooserView::pFactory);

    /* Setup frame: */
    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    /* Setup scroll-bars policy: */
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /* Update scene-rect: */
    updateSceneRect();

    /* Translate finally: */
    retranslateUi();
}

void UIGChooserView::sltMinimumWidthHintChanged(int iMinimumWidthHint)
{
    /* Is there something changed? */
    if (m_iMinimumWidthHint == iMinimumWidthHint)
        return;

    /* Remember new value: */
    m_iMinimumWidthHint = iMinimumWidthHint;

    /* Set minimum view width according passed width-hint: */
    setMinimumWidth(2 * frameWidth() + iMinimumWidthHint + verticalScrollBar()->sizeHint().width());

    /* Update scene-rect: */
    updateSceneRect();
}

void UIGChooserView::sltMinimumHeightHintChanged(int iMinimumHeightHint)
{
    /* Is there something changed? */
    if (m_iMinimumHeightHint == iMinimumHeightHint)
        return;

    /* Remember new value: */
    m_iMinimumHeightHint = iMinimumHeightHint;

    /* Update scene-rect: */
    updateSceneRect();
}

void UIGChooserView::sltFocusChanged(UIGChooserItem *pFocusItem)
{
    /* Make sure focus-item set: */
    if (!pFocusItem)
        return;

    QSize viewSize = viewport()->size();
    QRectF geo = pFocusItem->geometry();
    geo &= QRectF(geo.topLeft(), viewSize);
    ensureVisible(geo, 0, 0);
}

void UIGChooserView::retranslateUi()
{
    /* Translate this: */
    setToolTip(tr("Contains a tree of Virtual Machines and their groups"));
}

void UIGChooserView::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QIGraphicsView>::resizeEvent(pEvent);
    /* Notify listeners: */
    emit sigResized();
}

void UIGChooserView::updateSceneRect()
{
    setSceneRect(0, 0, m_iMinimumWidthHint, m_iMinimumHeightHint);
}


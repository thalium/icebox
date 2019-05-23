/* $Id: UIGDetailsElement.h $ */
/** @file
 * VBox Qt GUI - UIGDetailsElement class declaration.
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

#ifndef __UIGDetailsElement_h__
#define __UIGDetailsElement_h__

/* Qt includes: */
#include <QIcon>

/* GUI includes: */
#include "UIGDetailsItem.h"
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class UIGDetailsSet;
class CMachine;
class UIGraphicsRotatorButton;
class UIGraphicsTextPane;
class QTextLayout;
class QStateMachine;
class QPropertyAnimation;
class UITextTableLine;

/* Typedefs: */
typedef QList<UITextTableLine> UITextTable;


/* Details element
 * for graphics details model/view architecture: */
class UIGDetailsElement : public UIGDetailsItem
{
    Q_OBJECT;
    Q_PROPERTY(int animationDarkness READ animationDarkness WRITE setAnimationDarkness);
    Q_PROPERTY(int additionalHeight READ additionalHeight WRITE setAdditionalHeight);

signals:

    /* Notifiers: Hover stuff: */
    void sigHoverEnter();
    void sigHoverLeave();

    /* Notifiers: Toggle stuff: */
    void sigToggleElement(DetailsElementType type, bool fToggled);
    void sigToggleElementFinished();

    /* Notifier: Link-click stuff: */
    void sigLinkClicked(const QString &strCategory, const QString &strControl, const QString &strId);

public:

    /* Graphics-item type: */
    enum { Type = UIGDetailsItemType_Element };
    int type() const { return Type; }

    /* Constructor/destructor: */
    UIGDetailsElement(UIGDetailsSet *pParent, DetailsElementType type, bool fOpened);
    ~UIGDetailsElement();

    /* API: Element type: */
    DetailsElementType elementType() const { return m_type; }

    /* API: Open/close stuff: */
    bool closed() const { return m_fClosed; }
    bool opened() const { return !m_fClosed; }
    void close(bool fAnimated = true);
    void open(bool fAnimated = true);

    /* API: Update stuff: */
    virtual void updateAppearance();

    /* API: Animation stuff: */
    void markAnimationFinished();

    /** Returns the reference to the text table. */
    UITextTable &text() const;
    /** Defines the @a text table as the passed one. */
    void setText(const UITextTable &text);

protected slots:

    /* Handlers: Toggle stuff: */
    void sltToggleButtonClicked();
    void sltElementToggleStart();
    void sltElementToggleFinish(bool fToggled);

    /** Handles children geometry changes. */
    void sltUpdateGeometry() { updateGeometry(); }

    /** Handles children anchor clicks. */
    void sltHandleAnchorClicked(const QString &strAnchor);

    /** Handles mount storage medium requests. */
    void sltMountStorageMedium();

protected:

    /* Data enumerator: */
    enum ElementData
    {
        /* Hints: */
        ElementData_Margin,
        ElementData_Spacing
    };

    /** This event handler is delivered after the widget has been resized. */
    void resizeEvent(QGraphicsSceneResizeEvent *pEvent);

    /** Returns the description of the item. */
    virtual QString description() const /* override */;

    /* Data provider: */
    QVariant data(int iKey) const;

    /* Helpers: Update stuff: */
    void updateMinimumHeaderWidth();
    void updateMinimumHeaderHeight();

    /* API: Icon stuff: */
    void setIcon(const QIcon &icon);

    /* API: Name stuff: */
    void setName(const QString &strName);

    /* API: Machine stuff: */
    const CMachine& machine();

    /* Helpers: Layout stuff: */
    int minimumHeaderWidth() const { return m_iMinimumHeaderWidth; }
    int minimumHeaderHeight() const { return m_iMinimumHeaderHeight; }
    int minimumWidthHint() const;
    virtual int minimumHeightHint(bool fClosed) const;
    int minimumHeightHint() const;
    void updateLayout();

    /* Helpers: Hover stuff: */
    int animationDarkness() const { return m_iAnimationDarkness; }
    void setAnimationDarkness(int iAnimationDarkness) { m_iAnimationDarkness = iAnimationDarkness; update(); }

    /* Helpers: Animation stuff: */
    void setAdditionalHeight(int iAdditionalHeight);
    int additionalHeight() const { return m_iAdditionalHeight; }
    UIGraphicsRotatorButton* button() const { return m_pButton; }
    bool isAnimationRunning() const { return m_fAnimationRunning; }

private:

    /* API: Children stuff: */
    void addItem(UIGDetailsItem *pItem);
    void removeItem(UIGDetailsItem *pItem);
    QList<UIGDetailsItem*> items(UIGDetailsItemType type) const;
    bool hasItems(UIGDetailsItemType type) const;
    void clearItems(UIGDetailsItemType type);

    /* Helpers: Prepare stuff: */
    void prepareElement();
    void prepareButton();
    void prepareTextPane();

    /* Helpers: Paint stuff: */
    void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = 0);
    void paintDecorations(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption);
    void paintElementInfo(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption);
    void paintBackground(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption);

    /* Handlers: Mouse stuff: */
    void hoverMoveEvent(QGraphicsSceneHoverEvent *pEvent);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);
    void mousePressEvent(QGraphicsSceneMouseEvent *pEvent);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *pEvent);

    /* Helpers: Mouse stuff: */
    void updateButtonVisibility();
    void handleHoverEvent(QGraphicsSceneHoverEvent *pEvent);
    void updateNameHoverLink();

    /* Helper: Animation stuff: */
    void updateAnimationParameters();

    /* Variables: */
    UIGDetailsSet *m_pSet;
    DetailsElementType m_type;
    QPixmap m_pixmap;
    QString m_strName;
    int m_iCornerRadius;
    QFont m_nameFont;
    QFont m_textFont;
    QSize m_pixmapSize;
    QSize m_nameSize;
    QSize m_buttonSize;
    int m_iMinimumHeaderWidth;
    int m_iMinimumHeaderHeight;

    /* Variables: Toggle-button stuff: */
    UIGraphicsRotatorButton *m_pButton;
    bool m_fClosed;
    int m_iAdditionalHeight;
    bool m_fAnimationRunning;

    /* Variables: Text-pane stuff: */
    UIGraphicsTextPane *m_pTextPane;

    /* Variables: Hover stuff: */
    bool m_fHovered;
    bool m_fNameHovered;
    QStateMachine *m_pHighlightMachine;
    QPropertyAnimation *m_pForwardAnimation;
    QPropertyAnimation *m_pBackwardAnimation;
    int m_iAnimationDuration;
    int m_iDefaultDarkness;
    int m_iHighlightDarkness;
    int m_iAnimationDarkness;

    /* Friends: */
    friend class UIGDetailsSet;
};

#endif /* __UIGDetailsElement_h__ */


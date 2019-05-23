/* $Id: UIGDetailsModel.h $ */
/** @file
 * VBox Qt GUI - UIGDetailsModel class declaration.
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

#ifndef __UIGDetailsModel_h__
#define __UIGDetailsModel_h__

/* Qt includes: */
#include <QObject>
#include <QPointer>
#include <QMap>
#include <QSet>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declaration: */
class QGraphicsItem;
class QGraphicsScene;
class QGraphicsSceneContextMenuEvent;
class QGraphicsView;
class UIVMItem;
class UIGDetailsElementAnimationCallback;
class UIGDetailsGroup;
class UIGDetailsItem;
class UIGDetails;

/* Graphics details-model: */
class UIGDetailsModel : public QObject
{
    Q_OBJECT;

signals:

    /* Notifiers: Root-item stuff: */
    void sigRootItemMinimumWidthHintChanged(int iRootItemMinimumWidthHint);
    void sigRootItemMinimumHeightHintChanged(int iRootItemMinimumHeightHint);

    /* Notifier: Link processing stuff: */
    void sigLinkClicked(const QString &strCategory, const QString &strControl, const QString &strId);

public:

    /** Constructs a details-model passing @a pParent to the base-class.
      * @param  pParent  Brings the details container to embed into. */
    UIGDetailsModel(UIGDetails *pParent);
    /** Destructs a details-model. */
    ~UIGDetailsModel();

    /* API: Scene stuff: */
    QGraphicsScene* scene() const;
    QGraphicsView* paintDevice() const;
    QGraphicsItem* itemAt(const QPointF &position) const;

    /** Returns the details reference. */
    UIGDetails *details() const { return m_pDetails; }

    /** Returns the root item instance. */
    UIGDetailsItem *root() const;

    /* API: Layout stuff: */
    void updateLayout();

    /* API: Current-item(s) stuff: */
    void setItems(const QList<UIVMItem*> &items);

    /** Returns the details settings. */
    const QMap<DetailsElementType, bool>& settings() const { return m_settings; }

private slots:

    /* Handler: Details-view stuff: */
    void sltHandleViewResize();

    /* Handlers: Element-items stuff: */
    void sltToggleElements(DetailsElementType type, bool fToggled);
    void sltToggleAnimationFinished(DetailsElementType type, bool fToggled);
    void sltElementTypeToggled();

    /* Handlers: Chooser stuff: */
    void sltHandleSlidingStarted();
    void sltHandleToggleStarted();
    void sltHandleToggleFinished();

private:

    /* Data enumerator: */
    enum DetailsModelData
    {
        /* Layout hints: */
        DetailsModelData_Margin
    };

    /* Data provider: */
    QVariant data(int iKey) const;

    /* Helpers: Prepare stuff: */
    void prepareScene();
    void prepareRoot();
    void loadSettings();

    /* Helpers: Cleanup stuff: */
    void saveSettings();
    void cleanupRoot();
    void cleanupScene();

    /* Handler: Event-filter: */
    bool eventFilter(QObject *pObject, QEvent *pEvent);

    /* Handler: Context-menu stuff: */
    bool processContextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent);

    /** Holds the details reference. */
    UIGDetails *m_pDetails;

    /* Variables: */
    QGraphicsScene *m_pScene;
    UIGDetailsGroup *m_pRoot;
    UIGDetailsElementAnimationCallback *m_pAnimationCallback;
    /** Holds the details settings. */
    QMap<DetailsElementType, bool> m_settings;
};

/* Details-element animation callback: */
class UIGDetailsElementAnimationCallback : public QObject
{
    Q_OBJECT;

signals:

    /* Notifier: Complete stuff: */
    void sigAllAnimationFinished(DetailsElementType type, bool fToggled);

public:

    /* Constructor: */
    UIGDetailsElementAnimationCallback(QObject *pParent, DetailsElementType type, bool fToggled);

    /* API: Notifiers stuff: */
    void addNotifier(UIGDetailsItem *pItem);

private slots:

    /* Handler: Progress stuff: */
    void sltAnimationFinished();

private:

    /* Variables: */
    QList<UIGDetailsItem*> m_notifiers;
    DetailsElementType m_type;
    bool m_fToggled;
};

#endif /* __UIGDetailsModel_h__ */


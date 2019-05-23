/* $Id: UIGMachinePreview.h $ */
/** @file
 * VBox Qt GUI - UIGMachinePreview class declaration.
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

#ifndef __UIGMachinePreview_h__
#define __UIGMachinePreview_h__

/* Qt includes: */
#include <QHash>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIGDetailsItem.h"
#include "UIExtraDataDefs.h"

/* COM includes: */
#include "COMEnums.h"
#include "CSession.h"
#include "CMachine.h"

/* Forward declarations: */
class QAction;
class QImage;
class QPixmap;
class QMenu;
class QTimer;

/* Preview window class: */
class UIGMachinePreview : public QIWithRetranslateUI4<QIGraphicsWidget>
{
    Q_OBJECT;

signals:

    /** Notifies about size-hint changes. */
    void sigSizeHintChanged();

public:

    /* Graphics-item type: */
    enum { Type = UIGDetailsItemType_Preview };
    int type() const { return Type; }

    /* Constructor/destructor: */
    UIGMachinePreview(QIGraphicsWidget *pParent);
    ~UIGMachinePreview();

    /* API: Machine stuff: */
    void setMachine(const CMachine& machine);
    CMachine machine() const;

private slots:

    /* Handler: Global-event listener stuff: */
    void sltMachineStateChange(QString strId);

    /* Handler: Preview recreator: */
    void sltRecreatePreview();

private:

    /** Aspect ratio presets. */
    enum AspectRatioPreset
    {
        AspectRatioPreset_16x10,
        AspectRatioPreset_16x9,
        AspectRatioPreset_4x3,
    };

    /* Helpers: Event handlers: */
    void resizeEvent(QGraphicsSceneResizeEvent *pEvent);
    void showEvent(QShowEvent *pEvent);
    void hideEvent(QHideEvent *pEvent);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent);

    /* Helpers: Translate stuff; */
    void retranslateUi();

    /* Helpers: Layout stuff: */
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

    /* Helpers: Paint stuff: */
    void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = 0);

    /* Helpers: Update stuff: */
    void setUpdateInterval(PreviewUpdateIntervalType interval, bool fSave);
    void recalculatePreviewRectangle();
    void restart();
    void stop();

    /** Looks for the best aspect-ratio preset for the passed @a dAspectRatio among all the passed @a ratios. */
    static AspectRatioPreset bestAspectRatioPreset(const double dAspectRatio, const QMap<AspectRatioPreset, double> &ratios);
    /** Calculates image size suitable to passed @a hostSize and @a guestSize. */
    static QSize imageAspectRatioSize(const QSize &hostSize, const QSize &guestSize);

    /* Variables: */
    CSession m_session;
    CMachine m_machine;
    QTimer *m_pUpdateTimer;
    QMenu *m_pUpdateTimerMenu;
    QHash<PreviewUpdateIntervalType, QAction*> m_actions;
    double m_dRatio;
    const int m_iMargin;
    QRect m_vRect;
    AspectRatioPreset m_preset;
    QMap<AspectRatioPreset, QSize> m_sizes;
    QMap<AspectRatioPreset, double> m_ratios;
    QMap<AspectRatioPreset, QPixmap*> m_emptyPixmaps;
    QMap<AspectRatioPreset, QPixmap*> m_fullPixmaps;
    QImage *m_pPreviewImg;
    QString m_strPreviewName;
};

#endif /* !__UIGMachinePreview_h__ */


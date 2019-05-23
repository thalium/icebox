/* $Id: UIPopupStack.h $ */
/** @file
 * VBox Qt GUI - UIPopupStack class declaration.
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

#ifndef __UIPopupStack_h__
#define __UIPopupStack_h__

/* Qt includes: */
#include <QWidget>
#include <QMap>

/* GUI includes: */
#include "UIPopupCenter.h"

/* Forward declaration: */
class QVBoxLayout;
class QScrollArea;
class UIPopupStackViewport;

/* Popup-stack prototype class: */
class UIPopupStack : public QWidget
{
    Q_OBJECT;

signals:

    /* Notifier: Layout stuff: */
    void sigProposeStackViewportSize(QSize newSize);

    /* Notifier: Popup-pane stuff: */
    void sigPopupPaneDone(QString strPopupPaneID, int iResultCode);

    /* Notifier: Popup-stack stuff: */
    void sigRemove(QString strID);

public:

    /* Constructor: */
    UIPopupStack(const QString &strID, UIPopupStackOrientation orientation);

    /* API: Popup-pane stuff: */
    bool exists(const QString &strPopupPaneID) const;
    void createPopupPane(const QString &strPopupPaneID,
                         const QString &strMessage, const QString &strDetails,
                         const QMap<int, QString> &buttonDescriptions);
    void updatePopupPane(const QString &strPopupPaneID,
                         const QString &strMessage, const QString &strDetails);
    void recallPopupPane(const QString &strPopupPaneID);
    void setOrientation(UIPopupStackOrientation orientation);

    /* API: Parent stuff: */
    void setParent(QWidget *pParent);
    void setParent(QWidget *pParent, Qt::WindowFlags flags);

private slots:

    /* Handler: Layout stuff: */
    void sltAdjustGeometry();

    /* Handlers: Popup-pane stuff: */
    void sltPopupPaneRemoved(QString strPopupPaneID);
    void sltPopupPanesRemoved();

private:

    /* Helpers: Prepare stuff: */
    void prepare();
    void prepareContent();

    /* Handler: Event-filter stuff: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Handler: Event stuff: */
    void showEvent(QShowEvent *pEvent);

    /* Helper: Layout stuff: */
    void propagateSize();

    /* Static helpers: Prepare stuff: */
    static int parentMenuBarHeight(QWidget *pParent);
    static int parentStatusBarHeight(QWidget *pParent);

    /* Variable: General stuff: */
    QString m_strID;
    UIPopupStackOrientation m_orientation;

    /* Variables: Widget stuff: */
    QVBoxLayout *m_pMainLayout;
    QScrollArea *m_pScrollArea;
    UIPopupStackViewport *m_pScrollViewport;

    /* Variables: Layout stuff: */
    int m_iParentMenuBarHeight;
    int m_iParentStatusBarHeight;
};

#endif /* __UIPopupStack_h__ */

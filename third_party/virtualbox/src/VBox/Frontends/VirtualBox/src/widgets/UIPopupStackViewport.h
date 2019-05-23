/* $Id: UIPopupStackViewport.h $ */
/** @file
 * VBox Qt GUI - UIPopupStackViewport class declaration.
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

#ifndef __UIPopupStackViewport_h__
#define __UIPopupStackViewport_h__

/* Qt includes: */
#include <QWidget>
#include <QMap>

/* Forward declaration: */
class UIPopupPane;

/* Popup-stack viewport prototype class: */
class UIPopupStackViewport : public QWidget
{
    Q_OBJECT;

signals:

    /* Notifiers: Layout stuff: */
    void sigProposePopupPaneSize(QSize newSize);
    void sigSizeHintChanged();

    /* Notifiers: Popup-pane stuff: */
    void sigPopupPaneDone(QString strPopupPaneID, int iResultCode);
    void sigPopupPaneRemoved(QString strPopupPaneID);
    void sigPopupPanesRemoved();

public:

    /* Constructor: */
    UIPopupStackViewport();

    /* API: Popup-pane stuff: */
    bool exists(const QString &strPopupPaneID) const;
    void createPopupPane(const QString &strPopupPaneID,
                         const QString &strMessage, const QString &strDetails,
                         const QMap<int, QString> &buttonDescriptions);
    void updatePopupPane(const QString &strPopupPaneID,
                         const QString &strMessage, const QString &strDetails);
    void recallPopupPane(const QString &strPopupPaneID);

    /* API: Layout stuff: */
    QSize minimumSizeHint() const { return m_minimumSizeHint; }

public slots:

    /* Handler: Layout stuff: */
    void sltHandleProposalForSize(QSize newSize);

private slots:

    /* Handler: Layout stuff: */
    void sltAdjustGeometry();

    /* Handler: Popup-pane stuff: */
    void sltPopupPaneDone(int iButtonCode);

private:

    /* Helpers: Layout stuff: */
    void updateSizeHint();
    void layoutContent();

    /* Variables: Layout stuff: */
    const int m_iLayoutMargin;
    const int m_iLayoutSpacing;
    QSize m_minimumSizeHint;

    /* Variables: Children stuff: */
    QMap<QString, UIPopupPane*> m_panes;
};

#endif /* __UIPopupStackViewport_h__ */

/* $Id: UIPopupPaneButtonPane.h $ */
/** @file
 * VBox Qt GUI - UIPopupPaneButtonPane class declaration.
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

#ifndef __UIPopupPaneButtonPane_h__
#define __UIPopupPaneButtonPane_h__

/* Qt includes: */
#include <QWidget>
#include <QMap>

/* Forward declarations: */
class QHBoxLayout;
class QKeyEvent;
class QIToolButton;

/* Popup-pane button-pane prototype class: */
class UIPopupPaneButtonPane : public QWidget
{
    Q_OBJECT;

signals:

    /* Notifier: Button stuff: */
    void sigButtonClicked(int iButtonID);

public:

    /* Constructor: */
    UIPopupPaneButtonPane(QWidget *pParent = 0);

    /* API: Button stuff: */
    void setButtons(const QMap<int, QString> &buttonDescriptions);
    int defaultButton() const { return m_iDefaultButton; }
    int escapeButton() const { return m_iEscapeButton; }

private slots:

    /* Handler: Button stuff: */
    void sltButtonClicked();

private:

    /* Helpers: Prepare/cleanup stuff: */
    void prepare();
    void prepareLayouts();
    void prepareButtons();
    void cleanupButtons();

    /* Handler: Event stuff: */
    void keyPressEvent(QKeyEvent *pEvent);

    /* Static helpers: Button stuff: */
    static QIToolButton* addButton(int iButtonID, const QString &strToolTip);
    static QString defaultToolTip(int iButtonID);
    static QIcon defaultIcon(int iButtonID);

    /* Variables: Widget stuff: */
    QHBoxLayout *m_pButtonLayout;
    QMap<int, QString> m_buttonDescriptions;
    QMap<int, QIToolButton*> m_buttons;
    int m_iDefaultButton;
    int m_iEscapeButton;
};

#endif /* __UIPopupPaneButtonPane_h__ */

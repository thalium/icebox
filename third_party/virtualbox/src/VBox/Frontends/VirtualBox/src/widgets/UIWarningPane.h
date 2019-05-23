/* $Id: UIWarningPane.h $ */
/** @file
 * VBox Qt GUI - UIWarningPane class declaration.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIWarningPane_h__
#define __UIWarningPane_h__

/* Global includes */
#include <QWidget>

/* Forward declarations: */
class UIPageValidator;
class QHBoxLayout;
class QLabel;
class QTimer;

/* Warning-pane prototype: */
class UIWarningPane: public QWidget
{
    Q_OBJECT;

signals:

    /* Notifiers: Hover stuff: */
    void sigHoverEnter(UIPageValidator *pValidator);
    void sigHoverLeave(UIPageValidator *pValidator);

public:

    /* Constructor: */
    UIWarningPane(QWidget *pParent = 0);

    /* API: Warning stuff: */
    void setWarningLabel(const QString &strWarningLabel);

    /* API: Registry stuff: */
    void registerValidator(UIPageValidator *pValidator);

private slots:

    /* Handler: Hover stuff: */
    void sltHandleHoverTimer();

private:

    /* Helpers: Prepare stuff: */
    void prepare();
    void prepareContent();

    /* Handler: Event processing stuff: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Variables: */
    QHBoxLayout *m_pIconLayout;
    QLabel *m_pTextLabel;
    QList<UIPageValidator*> m_validators;
    QList<QLabel*> m_icons;
    QList<bool> m_hovered;
    QTimer *m_pHoverTimer;
    int m_iHoveredIconLabelPosition;
};

#endif /* __UIWarningPane_h__ */

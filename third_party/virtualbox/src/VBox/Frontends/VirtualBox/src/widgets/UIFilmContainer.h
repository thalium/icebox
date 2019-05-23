/* $Id: UIFilmContainer.h $ */
/** @file
 * VBox Qt GUI - UIFilmContainer class declaration.
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

#ifndef __UIFilmContainer_h__
#define __UIFilmContainer_h__

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Other VBox includes: */
#include <VBox/com/com.h>

/* Forward declarations: */
class QVBoxLayout;
class QScrollArea;
class UIFilm;
class QCheckBox;

/* Transparent QScrollArea container for UIFilm widgets: */
class UIFilmContainer : public QWidget
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIFilmContainer(QWidget *pParent = 0);

    /* API: Value stuff: */
    QVector<BOOL> value() const;
    void setValue(const QVector<BOOL> &value);

private:

    /* Helpers: Prepare stuff: */
    void prepare();
    void prepareLayout();
    void prepareScroller();

    /* Variables: */
    QVBoxLayout *m_pMainLayout;
    QScrollArea *m_pScroller;
    QList<UIFilm*> m_widgets;
};

/* QWidget item prototype for UIFilmContainer: */
class UIFilm : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIFilm(int iScreenIndex, BOOL fEnabled, QWidget *pParent = 0);

    /* API: Check-box stuff: */
    bool checked() const;

private:

    /* Handler: Translate stuff: */
    void retranslateUi();

    /* Helpers: Prepare stuff: */
    void prepare();
    void prepareLayout();
    void prepareCheckBox();

    /* Helper: Layout stuff: */
    QSize minimumSizeHint() const;

    /* Handler: Paint stuff: */
    void paintEvent(QPaintEvent *pEvent);

    /* Variables: */
    int m_iScreenIndex;
    BOOL m_fWasEnabled;
    QVBoxLayout *m_pMainLayout;
    QCheckBox *m_pCheckBox;
};

#endif /* __UIFilmContainer_h__ */

/* $Id: QIArrowSplitter.h $ */
/** @file
 * VBox Qt GUI - QIArrowSplitter class declaration.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QIArrowSplitter_h___
#define ___QIArrowSplitter_h___

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QVBoxLayout;
class QIArrowButtonSwitch;
class QIArrowButtonPress;
class QIDetailsBrowser;

/* Type definitions: */
typedef QPair<QString, QString> QStringPair;
typedef QList<QStringPair> QStringPairList;

/** QWidget extension
  * allowing to toggle visibility for any other child widget. */
class QIArrowSplitter : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about size-hint change: */
    void sigSizeHintChange();

public:

    /** Constructor, passes @a pParent to the QWidget constructor. */
    QIArrowSplitter(QWidget *pParent = 0);

    /** Returns minimum size-hint. */
    QSize minimumSizeHint() const;

    /** Defines the @a strName for the switch-button. */
    void setName(const QString &strName);

    /** Returns splitter details. */
    const QStringPairList& details() const { return m_details; }
    /** Defines splitter @a details. */
    void setDetails(const QStringPairList &details);

public slots:

    /** Updates size-hints. */
    void sltUpdateSizeHints();

    /** Updates navigation-buttons visibility. */
    void sltUpdateNavigationButtonsVisibility();
    /** Updates details-browser visibility. */
    void sltUpdateDetailsBrowserVisibility();

    /** Navigates through details-list backward. */
    void sltSwitchDetailsPageBack();
    /** Navigates through details-list forward. */
    void sltSwitchDetailsPageNext();

private:

    /** Prepare routine. */
    void prepare();

    /** Retranslation routine. */
    void retranslateUi();

    /** Updates details. */
    void updateDetails();

    /** Holds the main-layout. */
    QVBoxLayout *m_pMainLayout;

    /** Holds the switch-button. */
    QIArrowButtonSwitch *m_pSwitchButton;
    /** Holds the back-button. */
    QIArrowButtonPress *m_pBackButton;
    /** Holds the next-button. */
    QIArrowButtonPress *m_pNextButton;

    /** Holds the details-browser. */
    QIDetailsBrowser *m_pDetailsBrowser;
    /** Holds details-list. */
    QStringPairList m_details;
    /** Holds details-list index. */
    int m_iDetailsIndex;
};

#endif /* !___QIArrowSplitter_h___ */

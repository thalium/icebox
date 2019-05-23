/* $Id: UISpecialControls.h $ */
/** @file
 * VBox Qt GUI - VBoxSpecialButtons declarations.
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

#ifndef ___VBoxSpecialControls_h__
#define ___VBoxSpecialControls_h__

/* VBox includes */
#include "QIWithRetranslateUI.h"

/* Qt includes */
#include <QPushButton>

#ifdef VBOX_DARWIN_USE_NATIVE_CONTROLS

/* VBox includes */
#include "UICocoaSpecialControls.h"

/********************************************************************************
 *
 * A mini cancel button in the native Cocoa version.
 *
 ********************************************************************************/
class UIMiniCancelButton: public QAbstractButton
{
    Q_OBJECT;

public:
    UIMiniCancelButton(QWidget *pParent = 0);

    void setText(const QString &strText) { m_pButton->setText(strText); }
    void setToolTip(const QString &strTip) { m_pButton->setToolTip(strTip); }
    void removeBorder() {}

protected:
    void paintEvent(QPaintEvent * /* pEvent */) {}
    void resizeEvent(QResizeEvent *pEvent);

private:
    UICocoaButton *m_pButton;
};

/********************************************************************************
 *
 * A reset button in the native Cocoa version.
 *
 ********************************************************************************/
class UIResetButton: public QAbstractButton
{
    Q_OBJECT;

public:
    UIResetButton(QWidget *pParent = 0);

    void setText(const QString &strText) { m_pButton->setText(strText); }
    void setToolTip(const QString &strTip) { m_pButton->setToolTip(strTip); }
    void removeBorder() {}

protected:
    void paintEvent(QPaintEvent * /* pEvent */) {}
    void resizeEvent(QResizeEvent *pEvent);

private:
    UICocoaButton *m_pButton;
};

/********************************************************************************
 *
 * A help button in the native Cocoa version.
 *
 ********************************************************************************/
class UIHelpButton: public QPushButton
{
    Q_OBJECT;

public:
    UIHelpButton(QWidget *pParent = 0);

    void setToolTip(const QString &strTip) { m_pButton->setToolTip(strTip); }

    void initFrom(QPushButton * /* pOther */) {}

protected:
    void paintEvent(QPaintEvent * /* pEvent */) {}

private:
    UICocoaButton *m_pButton;
};

/********************************************************************************
 *
 * A segmented button in the native Cocoa version.
 *
 ********************************************************************************/
class UIRoundRectSegmentedButton: public UICocoaSegmentedButton
{
    Q_OBJECT;

public:
    UIRoundRectSegmentedButton(QWidget *pParent, int cCount);
};

class UITexturedSegmentedButton: public UICocoaSegmentedButton
{
    Q_OBJECT;

public:
    UITexturedSegmentedButton(QWidget *pParent, int cCount);
};

/********************************************************************************
 *
 * A search field in the native Cocoa version.
 *
 ********************************************************************************/
class UISearchField: public UICocoaSearchField
{
    Q_OBJECT;

public:
    UISearchField(QWidget *pParent);
};

#else /* VBOX_DARWIN_USE_NATIVE_CONTROLS */

/* VBox includes */
#include "QIToolButton.h"

/* Qt includes */
#include <QLineEdit>

/* Qt forward declarations */
class QSignalMapper;

/********************************************************************************
 *
 * A mini cancel button for the other OS's.
 *
 ********************************************************************************/
class UIMiniCancelButton: public QIWithRetranslateUI<QIToolButton>
{
    Q_OBJECT;

public:
    UIMiniCancelButton(QWidget *pParent = 0);

protected:
    void retranslateUi() {};
};

/********************************************************************************
 *
 * A reset button for the other OS's (same as the cancel button for now)
 *
 ********************************************************************************/
class UIResetButton: public UIMiniCancelButton
{
    Q_OBJECT;

public:
    UIResetButton(QWidget *pParent = 0)
      : UIMiniCancelButton(pParent) {}
};

/********************************************************************************
 *
 * A help button for the other OS's.
 *
 ********************************************************************************/
class UIHelpButton: public QIWithRetranslateUI<QPushButton>
{
    Q_OBJECT;

public:
    UIHelpButton(QWidget *pParent = 0);
#ifdef VBOX_WS_MAC
    ~UIHelpButton();
    QSize sizeHint() const;
#endif /* VBOX_WS_MAC */

    void initFrom(QPushButton *pOther);

protected:
    void retranslateUi();

#ifdef VBOX_WS_MAC
    void paintEvent(QPaintEvent *pEvent);

    bool hitButton(const QPoint &pos) const;

    void mousePressEvent(QMouseEvent *pEvent);
    void mouseReleaseEvent(QMouseEvent *pEvent);
    void leaveEvent(QEvent *pEvent);

private:
    /* Private member vars */
    bool m_pButtonPressed;

    QSize m_size;
    QPixmap *m_pNormalPixmap;
    QPixmap *m_pPressedPixmap;
    QImage *m_pMask;
    QRect m_BRect;
#endif /* VBOX_WS_MAC */
};

/********************************************************************************
 *
 * A segmented button for the other OS's.
 *
 ********************************************************************************/
class UIRoundRectSegmentedButton: public QWidget
{
    Q_OBJECT;

public:
    UIRoundRectSegmentedButton(QWidget *pParent, int aCount);
    ~UIRoundRectSegmentedButton();

    int count() const;

    void setTitle(int iSegment, const QString &aTitle);
    void setToolTip(int iSegment, const QString &strTip);
    void setIcon(int iSegment, const QIcon &icon);
    void setEnabled(int iSegment, bool fEnabled);

    void setSelected(int iSegment);
    void animateClick(int iSegment);

signals:
    void clicked(int iSegment);

protected:
    /* Protected member vars */
    QList<QIToolButton*> m_pButtons;
    QSignalMapper *m_pSignalMapper;
};

class UITexturedSegmentedButton: public UIRoundRectSegmentedButton
{
    Q_OBJECT;

public:
    UITexturedSegmentedButton(QWidget *pParent, int cCount);
};

/********************************************************************************
 *
 * A search field  for the other OS's.
 *
 ********************************************************************************/
class UISearchField: public QLineEdit
{
    Q_OBJECT;

public:
    UISearchField(QWidget *pParent);

    void markError();
    void unmarkError();

private:
    /* Private member vars */
    QBrush m_baseBrush;
};

#endif /* VBOX_DARWIN_USE_NATIVE_CONTROLS */

#endif /* ___VBoxSpecialControls_h__ */


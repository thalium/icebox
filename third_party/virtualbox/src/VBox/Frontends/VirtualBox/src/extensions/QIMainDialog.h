/* $Id: QIMainDialog.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIMainDialog class implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QIMainDialog_h___
#define ___QIMainDialog_h___

/* Qt includes: */
#include <QMainWindow>
#include <QPointer>
#include <QDialog>

/* Forward declarations: */
class QPushButton;
class QEventLoop;
class QSizeGrip;

/** QDialog analog based on QMainWindow. */
class QIMainDialog: public QMainWindow
{
    Q_OBJECT;

public:

    /** Constructor.
      * @param pParent           holds the parent widget passed to the base-class,
      * @param enmFlags          holds the cumulative window flags passed to the base-class,
      * @param fIsAutoCentering  defines whether this dialog should be centered according it's parent. */
    QIMainDialog(QWidget *pParent = 0,
                 Qt::WindowFlags enmFlags = Qt::Dialog,
                 bool fIsAutoCentering = true);

    /** Returns the dialog's result code. */
    QDialog::DialogCode result() const { return m_enmResult; }

    /** Executes the dialog, launching local event-loop.
      * @param fApplicationModal defines whether this dialog should be modal to application or window. */
    QDialog::DialogCode exec(bool fApplicationModal = true);

    /** Returns dialog's default-button. */
    QPushButton* defaultButton() const;
    /** Defines dialog's default-button. */
    void setDefaultButton(QPushButton *pButton);

    /** Returns whether size-grip was enabled for that dialog. */
    bool isSizeGripEnabled() const;
    /** Defines whether size-grip should be @a fEnabled for that dialog. */
    void setSizeGripEnabled(bool fEnabled);

public slots:

    /** Defines whether the dialog is @a fVisible. */
    virtual void setVisible(bool fVisible);

protected:

    /** General event handler. */
    virtual bool event(QEvent *pEvent);
    /** Show event handler. */
    virtual void showEvent(QShowEvent *pEvent);
    /** Our own polish event handler. */
    virtual void polishEvent(QShowEvent *pEvent);
    /** Resize event handler. */
    virtual void resizeEvent(QResizeEvent *pEvent);
    /** Key-press event handler. */
    virtual void keyPressEvent(QKeyEvent *pEvent);
    /** General event filter. */
    virtual bool eventFilter(QObject *aObject, QEvent *pEvent);

    /** Function to search for dialog's default-button. */
    QPushButton* searchDefaultButton() const;

protected slots:

    /** Sets the modal dialog's result code to @a enmResult. */
    void setResult(QDialog::DialogCode enmResult) { m_enmResult = enmResult; }

    /** Closes the modal dialog and sets its result code to @a enmResult.
      * If this dialog is shown with exec(), done() causes the local
      * event-loop to finish, and exec() to return @a enmResult. */
    virtual void done(QDialog::DialogCode enmResult);
    /** Hides the modal dialog and sets the result code to Accepted. */
    virtual void accept() { done(QDialog::Accepted); }
    /** Hides the modal dialog and sets the result code to Rejected. */
    virtual void reject() { done(QDialog::Rejected); }

private:

    /** Holds whether this dialog should be centered according it's parent. */
    const bool m_fIsAutoCentering;
    /** Holds whether this dialog is polished. */
    bool m_fPolished;

    /** Holds modal dialog's result code. */
    QDialog::DialogCode m_enmResult;
    /** Holds modal dialog's event-loop. */
    QPointer<QEventLoop> m_pEventLoop;

    /** Holds dialog's default-button. */
    QPointer<QPushButton> m_pDefaultButton;
    /** Holds dialog's size-grip. */
    QPointer<QSizeGrip> m_pSizeGrip;
};

#endif /* !___QIMainDialog_h___ */

/* $Id: QIDialog.h $ */
/** @file
 * VBox Qt GUI - VBox Qt extensions: QIDialog class declaration.
 */

/*
 * Copyright (C) 2008-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___QIDialog_h___
#define ___QIDialog_h___

/* Qt includes: */
#include <QDialog>
#include <QPointer>

/* Forward declarations: */
class QEventLoop;


/** QDialog extension providing the GUI with
  * the advanced capabilities like delayed show. */
class QIDialog : public QDialog
{
    Q_OBJECT;

public:

    /** Constructs the dialog passing @a pParent and @a flags to the base-class. */
    QIDialog(QWidget *pParent = 0, Qt::WindowFlags flags = 0);

    /** Defines whether the dialog is @a fVisible. */
    void setVisible(bool fVisible);

public slots:

    /** Shows the dialog as a modal one, blocking until the user closes it.
      * @param  fShow              detemines whether the dialog should be shown instantly.
      * @param  fApplicationModal  determine whether the dialog should be application-modal. */
    virtual int execute(bool fShow = true, bool fApplicationModal = false);

    /** Shows the dialog as a modal one, blocking until the user closes it.
      * @note  Provided for compatibility with various Qt versions. */
    virtual int exec() /* overload for Qt4, override for Qt5 */ { return execute(); }

protected:

    /** Handles show @a pEvent. */
    virtual void showEvent(QShowEvent *pEvent) /* override */;
    /** Handles show @a pEvent sent for the first time. */
    virtual void polishEvent(QShowEvent *pEvent);

private:

    /** Holds whether the dialog is polished. */
    bool m_fPolished;

    /** Holds the separate event-loop instance.
      * @note  This event-loop is only used when the dialog being executed via the execute()
      *        functionality, allowing for the delayed show and advanced modality flag. */
    QPointer<QEventLoop> m_pEventLoop;
};

/** Safe pointer to the QIDialog class. */
typedef QPointer<QIDialog> UISafePointerDialog;

#endif /* !___QIDialog_h___ */


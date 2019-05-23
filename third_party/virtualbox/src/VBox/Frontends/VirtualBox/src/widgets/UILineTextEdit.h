/* $Id: UILineTextEdit.h $ */
/** @file
 * VBox Qt GUI - UILineTextEdit class declaration.
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

#ifndef __UILineTextEdit_h__
#define __UILineTextEdit_h__

/* VBox includes */
#include "QIDialog.h"
#include "QIWithRetranslateUI.h"

/* Qt includes */
#include <QPushButton>

/* Qt forward declarations */
class QTextEdit;
class QDialogButtonBox;

////////////////////////////////////////////////////////////////////////////////
// UITextEditor

class UITextEditor: public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:
    UITextEditor(QWidget *pParent = NULL);

    void setText(const QString& strText);
    QString text() const;

protected:
    void retranslateUi();

private slots:
    void open();

private:
    /* Private member vars */
    QTextEdit        *m_pTextEdit;
    QDialogButtonBox *m_pButtonBox;
    QPushButton      *m_pOpenButton;
};

////////////////////////////////////////////////////////////////////////////////
// UILineTextEdit

class UILineTextEdit: public QIWithRetranslateUI<QPushButton>
{
    Q_OBJECT;

signals:

    /* Notifier: Editing stuff: */
    void sigFinished(QWidget *pThis);

public:
    UILineTextEdit(QWidget *pParent = NULL);

    void setText(const QString& strText) { m_strText = strText; }
    QString text() const { return m_strText; }

protected:
    void retranslateUi();

private slots:
    void edit();

private:
    /* Private member vars */
    QString m_strText;
};

#endif /* __UILineTextEdit_h__ */


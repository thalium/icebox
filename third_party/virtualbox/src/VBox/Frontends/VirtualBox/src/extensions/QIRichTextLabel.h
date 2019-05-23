/* $Id: QIRichTextLabel.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIRichTextLabel class declaration.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __QIRichTextLabel_h__
#define __QIRichTextLabel_h__

/* Global includes: */
#include <QTextEdit>

/* QLabel analog to reflect rich-text,
 * Based on private QTextEdit functionality: */
class QIRichTextLabel : public QWidget
{
    Q_OBJECT;
    Q_PROPERTY(QString text READ text WRITE setText);

public:

    /* Constructor: */
    QIRichTextLabel(QWidget *pParent = 0);

    /* Text getter: */
    QString text() const;

    /* Register image: */
    void registerImage(const QImage &image, const QString &strName);

    /* Word-wrap mode getter/setter: */
    QTextOption::WrapMode wordWrapMode() const;
    void setWordWrapMode(QTextOption::WrapMode policy);

    /* API: Event-filter stuff: */
    void installEventFilter(QObject *pFilterObj);

public slots:

    /* Minimum text-width setter: */
    void setMinimumTextWidth(int iMinimumTextWidth);

    /* Text setter: */
    void setText(const QString &strText);

private:

    /* QTextEdit private member: */
    QTextEdit *m_pTextEdit;

    /* Minimum text-width: */
    int m_iMinimumTextWidth;
};

#endif // __QIRichTextLabel_h__

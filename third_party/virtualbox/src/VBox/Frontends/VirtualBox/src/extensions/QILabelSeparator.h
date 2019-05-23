/* $Id: QILabelSeparator.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QILabelSeparator class declaration.
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

#ifndef __QILabelSeparator_h__
#define __QILabelSeparator_h__

/* Global includes */
#include <QWidget>

/* Global forwards */
class QLabel;

class QILabelSeparator: public QWidget
{
    Q_OBJECT;

public:

    QILabelSeparator (QWidget *aParent = 0, Qt::WindowFlags aFlags = 0);
    QILabelSeparator (const QString &aText, QWidget *aParent = 0, Qt::WindowFlags aFlags = 0);

    QString text() const;
    void setBuddy (QWidget *aBuddy);

public slots:

    void clear();
    void setText (const QString &aText);

protected:

    virtual void init();

    QLabel *mLabel;
};

#endif // __QILabelSeparator_h__


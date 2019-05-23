/* $Id: QILineEdit.h $ */
/** @file
 * VBox Qt GUI - QILineEdit class declarations.
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

#ifndef __QILineEdit_h__
#define __QILineEdit_h__

/* Qt includes */
#include <QLineEdit>

class QILineEdit: public QLineEdit
{
public:

    QILineEdit (QWidget *aParent = 0)
        :QLineEdit (aParent) {}
    QILineEdit (const QString &aContents, QWidget *aParent = 0)
        :QLineEdit (aContents, aParent) {}

    void setMinimumWidthByText (const QString &aText);
    void setFixedWidthByText (const QString &aText);

private:

    QSize featTextWidth (const QString &aText) const;
};

#endif /* __QILineEdit_h__ */


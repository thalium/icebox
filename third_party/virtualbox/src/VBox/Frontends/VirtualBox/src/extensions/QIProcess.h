/* $Id: QIProcess.h $ */
/** @file
 * VBox Qt GUI - QIProcess class declaration.
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

#ifndef __QIProcess_h__
#define __QIProcess_h__

/* Qt includes: */
#include <QProcess>

/* QProcess reimplementation for VBox GUI needs: */
class QIProcess : public QProcess
{
    Q_OBJECT;

public:

    /* Static single-shot method to execute some script: */
    static QByteArray singleShot(const QString &strProcessName,
                                 int iTimeout = 5000 /* wait for data maximum 5 seconds */);

protected:

    /* Constructor: */
    QIProcess(QObject *pParent = 0);
};

#endif /* __QIProcess_h__ */


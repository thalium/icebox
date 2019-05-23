/* $Id: QIProcess.cpp $ */
/** @file
 * VBox Qt GUI - QIProcess class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */
/* GUI includes: */
# include "QIProcess.h"
#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* External includes: */
#ifdef VBOX_WS_X11
# include <sys/wait.h>
#endif /* VBOX_WS_X11 */


/* static */
QByteArray QIProcess::singleShot(const QString &strProcessName, int iTimeout)
{
    /* Why is it really needed is because of Qt4.3 bug with QProcess.
     * This bug is about QProcess sometimes (~70%) do not receive
     * notification about process was finished, so this makes
     * 'bool QProcess::waitForFinished (int)' block the GUI thread and
     * never dismissed with 'true' result even if process was really
     * started&finished. So we just waiting for some information
     * on process output and destroy the process with force. Due to
     * QProcess::~QProcess() has the same 'waitForFinished (int)' blocker
     * we have to change process state to QProcess::NotRunning. */

    QByteArray result;
    QIProcess process;
    process.start(strProcessName);
    bool firstShotReady = process.waitForReadyRead(iTimeout);
    if (firstShotReady)
        result = process.readAllStandardOutput();
    process.setProcessState(QProcess::NotRunning);
#ifdef VBOX_WS_X11
    int iStatus;
    if (process.pid() > 0)
        waitpid(process.pid(), &iStatus, 0);
#endif /* VBOX_WS_X11 */
    return result;
}

QIProcess::QIProcess(QObject *pParent /* = 0 */)
    : QProcess(pParent)
{
}


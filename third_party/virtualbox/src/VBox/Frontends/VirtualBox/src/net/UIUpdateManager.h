/* $Id: UIUpdateManager.h $ */
/** @file
 * VBox Qt GUI - UIUpdateManager class declaration.
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

#ifndef __UIUpdateManager_h__
#define __UIUpdateManager_h__

/* Global includes: */
#include <QObject>

/* Forward declarations: */
class UIUpdateQueue;

/* Singleton to perform new version checks
 * and update of various VirtualBox parts. */
class UIUpdateManager : public QObject
{
    Q_OBJECT;

public:

    /* Schedule manager: */
    static void schedule();
    /* Shutdown manager: */
    static void shutdown();
    /* Manager instance: */
    static UIUpdateManager* instance() { return m_pInstance; }

public slots:

    /* Force call for new version check: */
    void sltForceCheck();

private slots:

    /* Slot to check if update is necessary: */
    void sltCheckIfUpdateIsNecessary(bool fForceCall = false);

    /* Slot to handle update finishing: */
    void sltHandleUpdateFinishing();

private:

    /* Constructor/destructor: */
    UIUpdateManager();
    ~UIUpdateManager();

    /* Variables: */
    static UIUpdateManager* m_pInstance;
    UIUpdateQueue *m_pQueue;
    bool m_fIsRunning;
    quint64 m_uTime;
};
#define gUpdateManager UIUpdateManager::instance()

#endif // __UIUpdateManager_h__


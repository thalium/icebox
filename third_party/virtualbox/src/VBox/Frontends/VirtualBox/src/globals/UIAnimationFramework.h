/* $Id: UIAnimationFramework.h $ */
/** @file
 * VBox Qt GUI - UIAnimationFramework class declaration.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIAnimationFramework_h__
#define __UIAnimationFramework_h__

/* Qt includes: */
#include <QObject>

/* Forward declaration: */
class QStateMachine;
class QState;
class QPropertyAnimation;

/* UIAnimation factory: */
class UIAnimation : public QObject
{
    Q_OBJECT;

signals:

    /* Notifiers: State-change stuff: */
    void sigStateEnteredStart();
    void sigStateEnteredFinal();

public:

    /* API: Factory stuff: */
    static UIAnimation* installPropertyAnimation(QWidget *pTarget, const char *pszPropertyName,
                                                 const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                                                 const char *pszSignalForward, const char *pszSignalReverse,
                                                 bool fReverse = false, int iAnimationDuration = 300);

    /* API: Update stuff: */
    void update();

protected:

    /* Constructor: */
    UIAnimation(QWidget *pParent, const char *pszPropertyName,
                const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                const char *pszSignalForward, const char *pszSignalReverse,
                bool fReverse, int iAnimationDuration);

private:

    /* Helper: Prepare stuff: */
    void prepare();

    /* Variables: General stuff: */
    const char *m_pszPropertyName;
    const char *m_pszValuePropertyNameStart;
    const char *m_pszValuePropertyNameFinal;
    const char *m_pszSignalForward;
    const char *m_pszSignalReverse;
    bool m_fReverse;
    int m_iAnimationDuration;

    /* Variables: Animation-machine stuff: */
    QStateMachine *m_pAnimationMachine;
    QState *m_pStateStart;
    QState *m_pStateFinal;
    QPropertyAnimation *m_pForwardAnimation;
    QPropertyAnimation *m_pReverseAnimation;
};

/* UIAnimationLoop factory: */
class UIAnimationLoop : public QObject
{
    Q_OBJECT;

public:

    /* API: Factory stuff: */
    static UIAnimationLoop* installAnimationLoop(QWidget *pTarget, const char *pszPropertyName,
                                                 const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                                                 int iAnimationDuration = 300);

    /* API: Update stuff: */
    void update();

    /* API: Loop stuff: */
    void start();
    void stop();

protected:

    /* Constructor: */
    UIAnimationLoop(QWidget *pParent, const char *pszPropertyName,
                    const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                    int iAnimationDuration);

private:

    /* Helper: Prepare stuff: */
    void prepare();

    /* Variables: */
    const char *m_pszPropertyName;
    const char *m_pszValuePropertyNameStart;
    const char *m_pszValuePropertyNameFinal;
    int m_iAnimationDuration;
    QPropertyAnimation *m_pAnimation;
};

#endif /* __UIAnimationFramework_h__ */

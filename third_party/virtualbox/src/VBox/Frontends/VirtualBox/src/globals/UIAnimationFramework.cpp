/* $Id: UIAnimationFramework.cpp $ */
/** @file
 * VBox Qt GUI - UIAnimationFramework class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QWidget>
# include <QStateMachine>
# include <QPropertyAnimation>
# include <QSignalTransition>

/* GUI includes: */
# include "UIAnimationFramework.h"

/* Other VBox includes: */
# include "iprt/assert.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static */
UIAnimation* UIAnimation::installPropertyAnimation(QWidget *pTarget, const char *pszPropertyName,
                                                   const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                                                   const char *pszSignalForward, const char *pszSignalReverse,
                                                   bool fReverse /* = false*/, int iAnimationDuration /* = 300*/)
{
    /* Return newly created animation-machine: */
    return new UIAnimation(pTarget, pszPropertyName,
                           pszValuePropertyNameStart, pszValuePropertyNameFinal,
                           pszSignalForward, pszSignalReverse,
                           fReverse, iAnimationDuration);
}

/* static */
UIAnimationLoop* UIAnimationLoop::installAnimationLoop(QWidget *pTarget, const char *pszPropertyName,
                                                       const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                                                       int iAnimationDuration /* = 300*/)
{
    /* Return newly created animation-loop: */
    return new UIAnimationLoop(pTarget, pszPropertyName,
                               pszValuePropertyNameStart, pszValuePropertyNameFinal,
                               iAnimationDuration);
}


UIAnimation::UIAnimation(QWidget *pParent, const char *pszPropertyName,
                         const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                         const char *pszSignalForward, const char *pszSignalReverse,
                         bool fReverse, int iAnimationDuration)
    : QObject(pParent)
    , m_pszPropertyName(pszPropertyName)
    , m_pszValuePropertyNameStart(pszValuePropertyNameStart), m_pszValuePropertyNameFinal(pszValuePropertyNameFinal)
    , m_pszSignalForward(pszSignalForward), m_pszSignalReverse(pszSignalReverse)
    , m_fReverse(fReverse), m_iAnimationDuration(iAnimationDuration)
    , m_pAnimationMachine(0), m_pForwardAnimation(0), m_pReverseAnimation(0)
{
    /* Prepare: */
    prepare();
}

void UIAnimation::prepare()
{
    /* Make sure parent asigned: */
    AssertPtrReturnVoid(parent());

    /* Prepare animation-machine: */
    m_pAnimationMachine = new QStateMachine(this);
    /* Create 'start' state: */
    m_pStateStart = new QState(m_pAnimationMachine);
    m_pStateStart->assignProperty(parent(), "AnimationState", QString("Start"));
    connect(m_pStateStart, &QState::propertiesAssigned, this, &UIAnimation::sigStateEnteredStart);
    /* Create 'final' state: */
    m_pStateFinal = new QState(m_pAnimationMachine);
    m_pStateFinal->assignProperty(parent(), "AnimationState", QString("Final"));
    connect(m_pStateFinal, &QState::propertiesAssigned, this, &UIAnimation::sigStateEnteredFinal);

    /* Prepare 'forward' animation: */
    m_pForwardAnimation = new QPropertyAnimation(parent(), m_pszPropertyName, m_pAnimationMachine);
    m_pForwardAnimation->setEasingCurve(QEasingCurve(QEasingCurve::InOutCubic));
    m_pForwardAnimation->setDuration(m_iAnimationDuration);
    /* Prepare 'reverse' animation: */
    m_pReverseAnimation = new QPropertyAnimation(parent(), m_pszPropertyName, m_pAnimationMachine);
    m_pReverseAnimation->setEasingCurve(QEasingCurve(QEasingCurve::InOutCubic));
    m_pReverseAnimation->setDuration(m_iAnimationDuration);

    /* Prepare state-transitions: */
    QSignalTransition *pStartToFinal = m_pStateStart->addTransition(parent(), m_pszSignalForward, m_pStateFinal);
    AssertPtrReturnVoid(pStartToFinal);
    pStartToFinal->addAnimation(m_pForwardAnimation);
    QSignalTransition *pFinalToStart = m_pStateFinal->addTransition(parent(), m_pszSignalReverse, m_pStateStart);
    AssertPtrReturnVoid(pFinalToStart);
    pFinalToStart->addAnimation(m_pReverseAnimation);

    /* Fetch animation-borders: */
    update();

    /* Choose initial state: */
    m_pAnimationMachine->setInitialState(!m_fReverse ? m_pStateStart : m_pStateFinal);
    /* Start animation-machine: */
    m_pAnimationMachine->start();
}

void UIAnimation::update()
{
    /* Update 'forward' animation: */
    m_pForwardAnimation->setStartValue(parent()->property(m_pszValuePropertyNameStart));
    m_pForwardAnimation->setEndValue(parent()->property(m_pszValuePropertyNameFinal));
    m_pStateStart->assignProperty(parent(), m_pszPropertyName, parent()->property(m_pszValuePropertyNameStart));
    /* Update 'reverse' animation: */
    m_pReverseAnimation->setStartValue(parent()->property(m_pszValuePropertyNameFinal));
    m_pReverseAnimation->setEndValue(parent()->property(m_pszValuePropertyNameStart));
    m_pStateFinal->assignProperty(parent(), m_pszPropertyName, parent()->property(m_pszValuePropertyNameFinal));
}


UIAnimationLoop::UIAnimationLoop(QWidget *pParent, const char *pszPropertyName,
                                 const char *pszValuePropertyNameStart, const char *pszValuePropertyNameFinal,
                                 int iAnimationDuration)
    : QObject(pParent)
    , m_pszPropertyName(pszPropertyName)
    , m_pszValuePropertyNameStart(pszValuePropertyNameStart), m_pszValuePropertyNameFinal(pszValuePropertyNameFinal)
    , m_iAnimationDuration(iAnimationDuration)
    , m_pAnimation(0)
{
    /* Prepare: */
    prepare();
}

void UIAnimationLoop::prepare()
{
    /* Prepare loop: */
    m_pAnimation = new QPropertyAnimation(parent(), m_pszPropertyName, this);
    m_pAnimation->setDuration(m_iAnimationDuration);
    m_pAnimation->setLoopCount(-1);

    /* Fetch animation-borders: */
    update();
}

void UIAnimationLoop::update()
{
    /* Update animation: */
    m_pAnimation->setStartValue(parent()->property(m_pszValuePropertyNameStart));
    m_pAnimation->setEndValue(parent()->property(m_pszValuePropertyNameFinal));
}

void UIAnimationLoop::start()
{
    /* Start animation: */
    m_pAnimation->start();
}

void UIAnimationLoop::stop()
{
    /* Stop animation: */
    m_pAnimation->stop();
}


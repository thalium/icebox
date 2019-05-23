/* $Id: UIGraphicsRotatorButton.h $ */
/** @file
 * VBox Qt GUI - UIGraphicsRotatorButton class declaration.
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

#ifndef __UIGraphicsRotatorButton_h__
#define __UIGraphicsRotatorButton_h__

/* GUI includes: */
#include "UIGraphicsButton.h"

/* Forward declarations: */
class QStateMachine;
class QPropertyAnimation;

/* Rotator graphics-button states: */
enum UIGraphicsRotatorButtonState
{
    UIGraphicsRotatorButtonState_Default,
    UIGraphicsRotatorButtonState_Animating,
    UIGraphicsRotatorButtonState_Rotated
};
Q_DECLARE_METATYPE(UIGraphicsRotatorButtonState);

/* Rotator graphics-button representation: */
class UIGraphicsRotatorButton : public UIGraphicsButton
{
    Q_OBJECT;
    Q_PROPERTY(UIGraphicsRotatorButtonState state READ state WRITE setState);

signals:

    /* Rotation internal stuff: */
    void sigToAnimating();
    void sigToRotated();
    void sigToDefault();

    /* Rotation external stuff: */
    void sigRotationStart();
    void sigRotationFinish(bool fRotated);

public:

    /* Constructor: */
    UIGraphicsRotatorButton(QIGraphicsWidget *pParent,
                            const QString &strPropertyName,
                            bool fToggled,
                            bool fReflected = false,
                            int iAnimationDuration = 300);

    /* API: Button-click stuff: */
    void setAutoHandleButtonClick(bool fEnabled);

    /* API: Toggle stuff: */
    void setToggled(bool fToggled, bool fAnimated = true);

    /* API: Subordinate animation stuff: */
    void setAnimationRange(int iStart, int iEnd);
    bool isAnimationRunning() const;

protected slots:

    /* Handler: Button-click stuff: */
    void sltButtonClicked();

protected:

    /* Helpers: Update stuff: */
    void refresh();

private:

    /* Helpers: Rotate stuff: */
    void updateRotationState();

    /* Property stiff: */
    UIGraphicsRotatorButtonState state() const;
    void setState(UIGraphicsRotatorButtonState state);

    /* Variables: */
    bool m_fReflected;
    UIGraphicsRotatorButtonState m_state;
    QStateMachine *m_pAnimationMachine;
    int m_iAnimationDuration;
    QPropertyAnimation *m_pForwardButtonAnimation;
    QPropertyAnimation *m_pBackwardButtonAnimation;
    QPropertyAnimation *m_pForwardSubordinateAnimation;
    QPropertyAnimation *m_pBackwardSubordinateAnimation;
};

#endif /* __UIGraphicsRotatorButton_h__ */


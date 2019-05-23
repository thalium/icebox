/* $Id: UIMachineWindowScale.h $ */
/** @file
 * VBox Qt GUI - UIMachineWindowScale class declaration.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIMachineWindowScale_h___
#define ___UIMachineWindowScale_h___

/* GUI includes: */
#include "UIMachineWindow.h"

/** UIMachineWindow reimplementation,
  * providing GUI with machine-window for the scale mode. */
class UIMachineWindowScale : public UIMachineWindow
{
    Q_OBJECT;

protected:

    /** Constructor, passes @a pMachineLogic and @a uScreenId to the UIMachineWindow constructor. */
    UIMachineWindowScale(UIMachineLogic *pMachineLogic, ulong uScreenId);

private:

    /** Prepare main-layout routine. */
    void prepareMainLayout();
#ifdef VBOX_WS_MAC
    /** Prepare visual-state routine. */
    void prepareVisualState();
#endif /* VBOX_WS_MAC */
    /** Load settings routine. */
    void loadSettings();

    /** Save settings routine. */
    void saveSettings();
#ifdef VBOX_WS_MAC
    /** Cleanup visual-state routine. */
    void cleanupVisualState();
#endif /* VBOX_WS_MAC */

    /** Updates visibility according to visual-state. */
    void showInNecessaryMode();

    /** Restores cached window geometry. */
    virtual void restoreCachedGeometry() /* override */;

    /** Performs window geometry normalization according to guest-size and host's available geometry.
      * @param  fAdjustPosition  Determines whether is it necessary to adjust position as well. */
    virtual void normalizeGeometry(bool fAdjustPosition) /* override */;

    /** Common @a pEvent handler. */
    bool event(QEvent *pEvent);

    /** Returns whether this window is maximized. */
    bool isMaximizedChecked();

    /** Holds the current window geometry. */
    QRect m_normalGeometry;

    /** Factory support. */
    friend class UIMachineWindow;
};

#endif /* !___UIMachineWindowScale_h___ */


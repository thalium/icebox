/* $Id: UIMachineViewFullscreen.h $ */
/** @file
 * VBox Qt GUI - UIMachineViewFullscreen class declaration.
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

#ifndef ___UIMachineViewFullscreen_h___
#define ___UIMachineViewFullscreen_h___

/* Local includes */
#include "UIMachineView.h"

class UIMachineViewFullscreen : public UIMachineView
{
    Q_OBJECT;

protected:

    /* Fullscreen machine-view constructor: */
    UIMachineViewFullscreen(  UIMachineWindow *pMachineWindow
                            , ulong uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                            , bool bAccelerate2DVideo
#endif
    );
    /* Fullscreen machine-view destructor: */
    virtual ~UIMachineViewFullscreen() {}

private slots:

    /* Handler: Console callback stuff: */
    void sltAdditionsStateChanged();

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Prepare routines: */
    void prepareCommon();
    void prepareFilters();
    void prepareConsoleConnections();

    /* Cleanup routines: */
    //void cleanupConsoleConnections() {}
    //void cleanupFilters() {}
    //void cleanupCommon() {}

    /** Returns whether the guest-screen auto-resize is enabled. */
    virtual bool isGuestAutoresizeEnabled() const /* override */ { return m_bIsGuestAutoresizeEnabled; }
    /** Defines whether the guest-screen auto-resize is @a fEnabled. */
    virtual void setGuestAutoresizeEnabled(bool bEnabled) /* override */;

    /** Adjusts guest-screen size to correspond current <i>working area</i> size. */
    void adjustGuestScreenSize();

    /* Helpers: Geometry stuff: */
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;

    /* Private variables: */
    bool m_bIsGuestAutoresizeEnabled : 1;

    /* Friend classes: */
    friend class UIMachineView;
};

#endif // !___UIMachineViewFullscreen_h___


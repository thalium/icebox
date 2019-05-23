/* $Id: UIMachineViewNormal.h $ */
/** @file
 * VBox Qt GUI - UIMachineViewNormal class declaration.
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

#ifndef ___UIMachineViewNormal_h___
#define ___UIMachineViewNormal_h___

/* Local includes */
#include "UIMachineView.h"

class UIMachineViewNormal : public UIMachineView
{
    Q_OBJECT;

protected:

    /* Normal machine-view constructor: */
    UIMachineViewNormal(  UIMachineWindow *pMachineWindow
                        , ulong uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                        , bool bAccelerate2DVideo
#endif
    );
    /* Normal machine-view destructor: */
    virtual ~UIMachineViewNormal() {}

private slots:

    /* Console callback handlers: */
    void sltAdditionsStateChanged();

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Prepare helpers: */
    void prepareCommon();
    void prepareFilters();
    void prepareConsoleConnections();

    /* Cleanup helpers: */
    //void cleanupConsoleConnections() {}
    //void cleanupFilters() {}
    //void cleanupCommon() {}

    /** Returns whether the guest-screen auto-resize is enabled. */
    virtual bool isGuestAutoresizeEnabled() const /* override */ { return m_bIsGuestAutoresizeEnabled; }
    /** Defines whether the guest-screen auto-resize is @a fEnabled. */
    virtual void setGuestAutoresizeEnabled(bool bEnabled) /* override */;

    /** Resends guest size-hint. */
    void resendSizeHint();

    /** Adjusts guest-screen size to correspond current <i>machine-window</i> size. */
    void adjustGuestScreenSize();

    /* Private helpers: */
    QSize sizeHint() const;
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;

    /* Private members: */
    bool m_bIsGuestAutoresizeEnabled : 1;

    /* Friend classes: */
    friend class UIMachineView;
};

#endif // !___UIMachineViewNormal_h___


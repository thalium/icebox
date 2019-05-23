/* $Id: VBoxManageGuestCtrl.h $ */
/** @file
 * VBoxManageGuestCtrl.h - Definitions for guest control.
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

#ifndef ___H_VBOXMANAGE_GUESTCTRL
#define ___H_VBOXMANAGE_GUESTCTRL

#ifndef VBOX_ONLY_DOCS

#include <VBox/com/com.h>
#include <VBox/com/listeners.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/time.h>

#include <map>

const char *gctlFileStatusToText(FileStatus_T enmStatus);
const char *gctlProcessStatusToText(ProcessStatus_T enmStatus);
const char *gctlGuestSessionStatusToText(GuestSessionStatus_T enmStatus);

using namespace com;

class GuestFileEventListener;
typedef ListenerImpl<GuestFileEventListener> GuestFileEventListenerImpl;

class GuestProcessEventListener;
typedef ListenerImpl<GuestProcessEventListener> GuestProcessEventListenerImpl;

class GuestSessionEventListener;
typedef ListenerImpl<GuestSessionEventListener> GuestSessionEventListenerImpl;

class GuestEventListener;
typedef ListenerImpl<GuestEventListener> GuestEventListenerImpl;

/** Simple statistics class for binding locally
 *  held data to a specific guest object. */
class GuestEventStats
{

public:

    GuestEventStats(void)
        : uLastUpdatedMS(RTTimeMilliTS())
    {
    }

    /** @todo Make this more a class than a structure. */
public:

    uint64_t uLastUpdatedMS;
};

class GuestFileStats : public GuestEventStats
{

public:

    GuestFileStats(void) { }

    GuestFileStats(ComObjPtr<GuestFileEventListenerImpl> pListenerImpl)
        : mListener(pListenerImpl)
    {
    }

public: /** @todo */

    ComObjPtr<GuestFileEventListenerImpl> mListener;
};

class GuestProcStats : public GuestEventStats
{

public:

    GuestProcStats(void) { }

    GuestProcStats(ComObjPtr<GuestProcessEventListenerImpl> pListenerImpl)
        : mListener(pListenerImpl)
    {
    }

public: /** @todo */

    ComObjPtr<GuestProcessEventListenerImpl> mListener;
};

class GuestSessionStats : public GuestEventStats
{

public:

    GuestSessionStats(void) { }

    GuestSessionStats(ComObjPtr<GuestSessionEventListenerImpl> pListenerImpl)
        : mListener(pListenerImpl)
    {
    }

public: /** @todo */

    ComObjPtr<GuestSessionEventListenerImpl> mListener;
};

/** Map containing all watched guest files. */
typedef std::map< ComPtr<IGuestFile>, GuestFileStats > GuestEventFiles;
/** Map containing all watched guest processes. */
typedef std::map< ComPtr<IGuestProcess>, GuestProcStats > GuestEventProcs;
/** Map containing all watched guest sessions. */
typedef std::map< ComPtr<IGuestSession>, GuestSessionStats > GuestEventSessions;

class GuestListenerBase
{
public:

    GuestListenerBase(void);

    virtual ~GuestListenerBase(void);

public:

    HRESULT init(bool fVerbose = false);

protected:

    /** Verbose flag. */
    bool mfVerbose;
};

/**
 *  Handler for guest process events.
 */
class GuestFileEventListener : public GuestListenerBase
{
public:

    GuestFileEventListener(void);

    virtual ~GuestFileEventListener(void);

public:

    void uninit(void);

    STDMETHOD(HandleEvent)(VBoxEventType_T aType, IEvent *aEvent);

protected:

};

/**
 *  Handler for guest process events.
 */
class GuestProcessEventListener : public GuestListenerBase
{
public:

    GuestProcessEventListener(void);

    virtual ~GuestProcessEventListener(void);

public:

    void uninit(void);

    STDMETHOD(HandleEvent)(VBoxEventType_T aType, IEvent *aEvent);

protected:

};

/**
 *  Handler for guest session events.
 */
class GuestSessionEventListener : public GuestListenerBase
{
public:

    GuestSessionEventListener(void);

    virtual ~GuestSessionEventListener(void);

public:

    void uninit(void);

    STDMETHOD(HandleEvent)(VBoxEventType_T aType, IEvent *aEvent);

protected:

    GuestEventFiles mFiles;
    GuestEventProcs mProcs;
};

/**
 *  Handler for guest events.
 */
class GuestEventListener : public GuestListenerBase
{

public:

    GuestEventListener(void);

    virtual ~GuestEventListener(void);

public:

    void uninit(void);

    STDMETHOD(HandleEvent)(VBoxEventType_T aType, IEvent *aEvent);

protected:

    GuestEventSessions mSessions;
};
#endif /* !VBOX_ONLY_DOCS */

#endif /* !___H_VBOXMANAGE_GUESTCTRL */


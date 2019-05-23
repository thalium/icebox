/* $Id: VBoxTray.h $ */
/** @file
 * VBoxTray - Guest Additions Tray, Internal Header.
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

#ifndef ___VBOXTRAY_H
#define ___VBOXTRAY_H

#       define _InterlockedExchange           _InterlockedExchange_StupidDDKVsCompilerCrap
#       define _InterlockedExchangeAdd        _InterlockedExchangeAdd_StupidDDKVsCompilerCrap
#       define _InterlockedCompareExchange    _InterlockedCompareExchange_StupidDDKVsCompilerCrap
#       define _InterlockedAddLargeStatistic  _InterlockedAddLargeStatistic_StupidDDKVsCompilerCrap
#       define _interlockedbittestandset      _interlockedbittestandset_StupidDDKVsCompilerCrap
#       define _interlockedbittestandreset    _interlockedbittestandreset_StupidDDKVsCompilerCrap
#       define _interlockedbittestandset64    _interlockedbittestandset64_StupidDDKVsCompilerCrap
#       define _interlockedbittestandreset64  _interlockedbittestandreset64_StupidDDKVsCompilerCrap
#       pragma warning(disable : 4163)
#include <iprt/win/windows.h>
#       pragma warning(default : 4163)
#       undef  _InterlockedExchange
#       undef  _InterlockedExchangeAdd
#       undef  _InterlockedCompareExchange
#       undef  _InterlockedAddLargeStatistic
#       undef  _interlockedbittestandset
#       undef  _interlockedbittestandreset
#       undef  _interlockedbittestandset64
#       undef  _interlockedbittestandreset64

#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <process.h>

#include <iprt/initterm.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#include <VBox/version.h>
#include <VBox/VBoxGuestLib.h>
#include <VBoxDisplay.h>

#include "VBoxDispIf.h"

/*
 * Windows messsages.
 */

/**
 * General VBoxTray messages.
 */
#define WM_VBOXTRAY_TRAY_ICON                   WM_APP + 40

/* The tray icon's ID. */
#define ID_TRAYICON                             2000

/*
 * Timer IDs.
 */
#define TIMERID_VBOXTRAY_CHECK_HOSTVERSION      1000
#define TIMERID_VBOXTRAY_CAPS_TIMER             1001
#define TIMERID_VBOXTRAY_DT_TIMER               1002
#define TIMERID_VBOXTRAY_ST_DELAYED_INIT_TIMER  1003

/**
 * The environment information for services.
 */
typedef struct _VBOXSERVICEENV
{
    /** hInstance of VBoxTray. */
    HINSTANCE hInstance;
    /* Display driver interface, XPDM - WDDM abstraction see VBOXDISPIF** definitions above */
    /** @todo r=andy Argh. Needed by the "display" + "seamless" services (which in turn get called
     *               by the VBoxCaps facility. See #8037. */
    VBOXDISPIF dispIf;
} VBOXSERVICEENV;
/** Pointer to a VBoxTray service env info structure.  */
typedef VBOXSERVICEENV *PVBOXSERVICEENV;
/** Pointer to a const VBoxTray service env info structure.  */
typedef VBOXSERVICEENV const *PCVBOXSERVICEENV;

/**
 * A service descriptor.
 */
typedef struct _VBOXSERVICEDESC
{
    /** The service's name. RTTHREAD_NAME_LEN maximum characters. */
    char           *pszName;
    /** The service description. */
    char           *pszDesc;

    /** Callbacks. */

    /**
     * Initializes a service.
     * @returns VBox status code.
     * @param   pEnv
     * @param   ppInstance      Where to return the thread-specific instance data.
     * @todo r=bird: The pEnv type is WRONG!  Please check all your const pointers.
     */
    DECLCALLBACKMEMBER(int,  pfnInit)   (const PVBOXSERVICEENV pEnv, void **ppInstance);

    /** Called from the worker thread.
     *
     * @returns VBox status code.
     * @retval  VINF_SUCCESS if exitting because *pfShutdown was set.
     * @param   pInstance       Pointer to thread-specific instance data.
     * @param   pfShutdown      Pointer to a per service termination flag to check
     *                          before and after blocking.
     */
    DECLCALLBACKMEMBER(int,  pfnWorker) (void *pInstance, bool volatile *pfShutdown);

    /**
     * Stops a service.
     */
    DECLCALLBACKMEMBER(int,  pfnStop)   (void *pInstance);

    /**
     * Does termination cleanups.
     *
     * @remarks This may be called even if pfnInit hasn't been called!
     */
    DECLCALLBACKMEMBER(void, pfnDestroy)(void *pInstance);
} VBOXSERVICEDESC, *PVBOXSERVICEDESC;

extern VBOXSERVICEDESC g_SvcDescDisplay;
extern VBOXSERVICEDESC g_SvcDescClipboard;
extern VBOXSERVICEDESC g_SvcDescSeamless;
extern VBOXSERVICEDESC g_SvcDescVRDP;
extern VBOXSERVICEDESC g_SvcDescIPC;
extern VBOXSERVICEDESC g_SvcDescLA;
#ifdef VBOX_WITH_DRAG_AND_DROP
extern VBOXSERVICEDESC g_SvcDescDnD;
#endif

/**
 * The service initialization info and runtime variables.
 */
typedef struct _VBOXSERVICEINFO
{
    /** Pointer to the service descriptor. */
    PVBOXSERVICEDESC pDesc;
    /** Thread handle. */
    RTTHREAD         hThread;
    /** Pointer to service-specific instance data.
     *  Must be free'd by the service itself. */
    void            *pInstance;
    /** Whether Pre-init was called. */
    bool             fPreInited;
    /** Shutdown indicator. */
    bool volatile    fShutdown;
    /** Indicator set by the service thread exiting. */
    bool volatile    fStopped;
    /** Whether the service was started or not. */
    bool             fStarted;
    /** Whether the service is enabled or not. */
    bool             fEnabled;
} VBOXSERVICEINFO, *PVBOXSERVICEINFO;

/* Globally unique (system wide) message registration. */
typedef struct _VBOXGLOBALMESSAGE
{
    /** Message name. */
    char    *pszName;
    /** Function pointer for handling the message. */
    int      (* pfnHandler)          (WPARAM wParam, LPARAM lParam);

    /* Variables. */

    /** Message ID;
     *  to be filled in when registering the actual message. */
    UINT     uMsgID;
} VBOXGLOBALMESSAGE, *PVBOXGLOBALMESSAGE;

extern HWND         g_hwndToolWindow;
extern HINSTANCE    g_hInstance;
extern uint32_t     g_fGuestDisplaysChanged;

#endif /* !___VBOXTRAY_H */


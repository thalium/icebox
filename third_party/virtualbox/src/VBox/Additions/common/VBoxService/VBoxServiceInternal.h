/* $Id: VBoxServiceInternal.h $ */
/** @file
 * VBoxService - Guest Additions Services.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxServiceInternal_h
#define ___VBoxServiceInternal_h

#include <stdio.h>
#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
# include <process.h> /* Needed for file version information. */
#endif

#include <iprt/list.h>
#include <iprt/critsect.h>
#include <iprt/path.h> /* RTPATH_MAX */
#include <iprt/stdarg.h>

#include <VBox/VBoxGuestLib.h>
#include <VBox/HostServices/GuestControlSvc.h>

/**
 * A service descriptor.
 */
typedef struct
{
    /** The short service name. */
    const char *pszName;
    /** The longer service name. */
    const char *pszDescription;
    /** The usage options stuff for the --help screen. */
    const char *pszUsage;
    /** The option descriptions for the --help screen. */
    const char *pszOptions;

    /**
     * Called before parsing arguments.
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnPreInit)(void);

    /**
     * Tries to parse the given command line option.
     *
     * @returns 0 if we parsed, -1 if it didn't and anything else means exit.
     * @param   ppszShort   If not NULL it points to the short option iterator. a short argument.
     *                      If NULL examine argv[*pi].
     * @param   argc        The argument count.
     * @param   argv        The argument vector.
     * @param   pi          The argument vector index. Update if any value(s) are eaten.
     */
    DECLCALLBACKMEMBER(int, pfnOption)(const char **ppszShort, int argc, char **argv, int *pi);

    /**
     * Called before parsing arguments.
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnInit)(void);

    /** Called from the worker thread.
     *
     * @returns VBox status code.
     * @retval  VINF_SUCCESS if exitting because *pfShutdown was set.
     * @param   pfShutdown      Pointer to a per service termination flag to check
     *                          before and after blocking.
     */
    DECLCALLBACKMEMBER(int, pfnWorker)(bool volatile *pfShutdown);

    /**
     * Stops a service.
     */
    DECLCALLBACKMEMBER(void, pfnStop)(void);

    /**
     * Does termination cleanups.
     *
     * @remarks This may be called even if pfnInit hasn't been called!
     */
    DECLCALLBACKMEMBER(void, pfnTerm)(void);
} VBOXSERVICE;
/** Pointer to a VBOXSERVICE. */
typedef VBOXSERVICE *PVBOXSERVICE;
/** Pointer to a const VBOXSERVICE. */
typedef VBOXSERVICE const *PCVBOXSERVICE;

/* Default call-backs for services which do not need special behaviour. */
DECLCALLBACK(int)  VGSvcDefaultPreInit(void);
DECLCALLBACK(int)  VGSvcDefaultOption(const char **ppszShort, int argc, char **argv, int *pi);
DECLCALLBACK(int)  VGSvcDefaultInit(void);
DECLCALLBACK(void) VGSvcDefaultTerm(void);

/** The service name.
 * @note Used on windows to name the service as well as the global mutex. */
#define VBOXSERVICE_NAME            "VBoxService"

#ifdef RT_OS_WINDOWS
/** The friendly service name. */
# define VBOXSERVICE_FRIENDLY_NAME  "VirtualBox Guest Additions Service"
/** The service description (only W2K+ atm) */
# define VBOXSERVICE_DESCRIPTION    "Manages VM runtime information, time synchronization, guest control execution and miscellaneous utilities for guest operating systems."
/** The following constant may be defined by including NtStatus.h. */
# define STATUS_SUCCESS             ((NTSTATUS)0x00000000L)
#endif /* RT_OS_WINDOWS */

#ifdef VBOX_WITH_GUEST_PROPS
/**
 * A guest property cache.
 */
typedef struct VBOXSERVICEVEPROPCACHE
{
    /** The client ID for HGCM communication. */
    uint32_t        uClientID;
    /** Head in a list of VBOXSERVICEVEPROPCACHEENTRY nodes. */
    RTLISTANCHOR    NodeHead;
    /** Critical section for thread-safe use. */
    RTCRITSECT      CritSect;
} VBOXSERVICEVEPROPCACHE;
/** Pointer to a guest property cache. */
typedef VBOXSERVICEVEPROPCACHE *PVBOXSERVICEVEPROPCACHE;

/**
 * An entry in the property cache (VBOXSERVICEVEPROPCACHE).
 */
typedef struct VBOXSERVICEVEPROPCACHEENTRY
{
    /** Node to successor.
     * @todo r=bird: This is not really the node to the successor, but
     *       rather the OUR node in the list.  If it helps, remember that
     *       its a doubly linked list. */
    RTLISTNODE  NodeSucc;
    /** Name (and full path) of guest property. */
    char       *pszName;
    /** The last value stored (for reference). */
    char       *pszValue;
    /** Reset value to write if property is temporary.  If NULL, it will be
     *  deleted. */
    char       *pszValueReset;
    /** Flags. */
    uint32_t    fFlags;
} VBOXSERVICEVEPROPCACHEENTRY;
/** Pointer to a cached guest property. */
typedef VBOXSERVICEVEPROPCACHEENTRY *PVBOXSERVICEVEPROPCACHEENTRY;

#endif /* VBOX_WITH_GUEST_PROPS */

RT_C_DECLS_BEGIN

extern char        *g_pszProgName;
extern unsigned     g_cVerbosity;
extern char         g_szLogFile[RTPATH_MAX + 128];
extern uint32_t     g_DefaultInterval;
extern VBOXSERVICE  g_TimeSync;
extern VBOXSERVICE  g_Clipboard;
extern VBOXSERVICE  g_Control;
extern VBOXSERVICE  g_VMInfo;
extern VBOXSERVICE  g_CpuHotPlug;
#ifdef VBOX_WITH_VBOXSERVICE_MANAGEMENT
extern VBOXSERVICE  g_MemBalloon;
extern VBOXSERVICE  g_VMStatistics;
#endif
#ifdef VBOX_WITH_VBOXSERVICE_PAGE_SHARING
extern VBOXSERVICE  g_PageSharing;
#endif
#ifdef VBOX_WITH_SHARED_FOLDERS
extern VBOXSERVICE  g_AutoMount;
#endif
#ifdef DEBUG
extern RTCRITSECT   g_csLog; /* For guest process stdout dumping. */
#endif

extern RTEXITCODE               VGSvcSyntax(const char *pszFormat, ...);
extern RTEXITCODE               VGSvcError(const char *pszFormat, ...);
extern void                     VGSvcVerbose(unsigned iLevel, const char *pszFormat, ...);
extern int                      VGSvcLogCreate(const char *pszLogFile);
extern void                     VGSvcLogV(const char *pszFormat, va_list va);
extern void                     VGSvcLogDestroy(void);
extern int                      VGSvcArgUInt32(int argc, char **argv, const char *psz, int *pi, uint32_t *pu32,
                                               uint32_t u32Min, uint32_t u32Max);

/* Exposing the following bits because of windows: */
extern int                      VGSvcStartServices(void);
extern int                      VGSvcStopServices(void);
extern void                     VGSvcMainWait(void);
extern int                      VGSvcReportStatus(VBoxGuestFacilityStatus enmStatus);
#ifdef RT_OS_WINDOWS
extern RTEXITCODE               VGSvcWinInstall(void);
extern RTEXITCODE               VGSvcWinUninstall(void);
extern RTEXITCODE               VGSvcWinEnterCtrlDispatcher(void);
extern void                     VGSvcWinSetStopPendingStatus(uint32_t uCheckPoint);
#endif

#ifdef RT_OS_WINDOWS
# ifdef VBOX_WITH_GUEST_PROPS
extern int                      VGSvcVMInfoWinWriteUsers(PVBOXSERVICEVEPROPCACHE pCache, char **ppszUserList, uint32_t *pcUsersInList);
extern int                      VGSvcVMInfoWinGetComponentVersions(uint32_t uClientID);
# endif /* VBOX_WITH_GUEST_PROPS */
#endif /* RT_OS_WINDOWS */

#ifdef VBOX_WITH_VBOXSERVICE_MANAGEMENT
extern uint32_t                 VGSvcBalloonQueryPages(uint32_t cbPage);
#endif
#if defined(VBOX_WITH_VBOXSERVICE_PAGE_SHARING)
extern RTEXITCODE               VGSvcPageSharingWorkerChild(void);
#endif
extern int                      VGSvcVMInfoSignal(void);

RT_C_DECLS_END

#endif


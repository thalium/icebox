/* $Id: error.c $ */
/** @file
 * VBox crOpenGL error logging
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_SHARED_CROPENGL

#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/initterm.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/buildconfig.h>

#include <VBox/log.h>

#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
# include "cr_environment.h"
# include "cr_error.h"
# include "VBox/VBoxGuestLib.h"
# include "iprt/initterm.h"
#else
# include "cr_error.h"
#endif

#include <signal.h>
#include <stdlib.h>


/*
 * Stuff that needs to be dragged into the link because other DLLs needs it.
 * See also VBoxDeps.cpp in iprt and xpcom.
 */
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4232) /* nonstandard extension used: 'g_VBoxRTDeps' : address of dllimport 'RTBldCfgRevision' is not static, identiy not guaranteed */
#endif
PFNRT g_VBoxRTDeps[] =
{
    (PFNRT)RTAssertShouldPanic,
    (PFNRT)ASMAtomicReadU64,
    (PFNRT)ASMAtomicCmpXchgU64,
    (PFNRT)ASMBitFirstSet,
    (PFNRT)RTBldCfgRevision,
};
#ifdef _MSC_VER
# pragma warning(pop)
#endif


static void logMessageV(const char *pszPrefix, const char *pszFormat, va_list va)
{
    va_list vaCopy;
    if (RTR3InitIsInitialized())
    {
        va_copy(vaCopy, va);
        LogRel(("%s%N\n", pszPrefix, pszFormat, &vaCopy));
        va_end(vaCopy);
    }

#ifdef IN_GUEST  /** @todo Could be subject to pre-iprt-init issues, but hopefully not... */
    va_copy(vaCopy, va);
    RTStrmPrintf(g_pStdErr, "%s%N\n", pszPrefix, pszFormat, &vaCopy);
    va_end(vaCopy);
#endif
}

#ifdef WINDOWS
static void logMessage(const char *pszPrefix, const char *pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    logMessageV(pszPrefix, pszFormat, va);
    va_end(va);
}
#endif

DECLEXPORT(void) crError(const char *pszFormat, ...)
{
    va_list va;
#ifdef WINDOWS
    DWORD dwLastErr;
#endif

#ifdef WINDOWS
    /* Log last error on windows. */
    dwLastErr = GetLastError();
    if (dwLastErr != 0 && crGetenv("CR_WINDOWS_ERRORS") != NULL)
    {
        LPTSTR pszWindowsMessage;

        SetLastError(0);
        FormatMessageA(  FORMAT_MESSAGE_ALLOCATE_BUFFER
                       | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       NULL, dwLastErr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR)&pszWindowsMessage, 0, NULL);
        if (pszWindowsMessage)
        {
            logMessage("OpenGL, Windows error: ", "%u\n%s", dwLastErr, pszWindowsMessage);
            LocalFree(pszWindowsMessage);
        }
        else
            logMessage("OpenGL, Windows error: ", "%u", dwLastErr);
    }
#endif

    /* The message. */
    va_start(va, pszFormat);
    logMessageV("OpenGL Error: ", pszFormat, va);
    va_end(va);

#ifdef DEBUG
    /* Let's interrupt App execution only on debug builds and return
     * bad status to upper level on release ones. */
# ifdef IN_GUEST
    /* Trigger debugger's breakpoint handler. */
    ASMBreakpoint();
# else
    /* Dump core or activate the debugger in debug builds. */
    AssertFailed();
# endif
#endif /* DEBUG */
}

DECLEXPORT(void) crWarning(const char *pszFormat, ...)
{
    if (RTR3InitIsInitialized())
    {
        va_list va;

        va_start(va, pszFormat);
        logMessageV("OpenGL Warning: ", pszFormat, va);
        va_end(va);
    }
}

DECLEXPORT(void) crInfo(const char *pszFormat, ...)
{
    if (RTR3InitIsInitialized())
    {
        va_list va;

        va_start(va, pszFormat);
        logMessageV("OpenGL Info: ", pszFormat, va);
        va_end(va);
    }
}

DECLEXPORT(void) crDebug(const char *pszFormat, ...)
{
    if (RTR3InitIsInitialized())
    {
        va_list va;

        va_start(va, pszFormat);
#if defined(DEBUG_vgalitsy) || defined(DEBUG_galitsyn)
        LogRel(("OpenGL Debug: %N\n", pszFormat, &va));
#else
        Log(("OpenGL Debug: %N\n", pszFormat, &va));
#endif
        va_end(va);
    }
}

#if defined(RT_OS_WINDOWS)
BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    (void) lpvReserved; (void) hDLLInst;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            int rc;
            rc = RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE); CRASSERT(rc==0);
# ifdef IN_GUEST
            rc = VbglR3Init();
# endif
            LogRel(("crUtil DLL loaded.\n"));
# if defined(DEBUG_misha)
            char aName[MAX_PATH];
            GetModuleFileNameA(hDLLInst, aName, RT_ELEMENTS(aName));
            crDbgCmdSymLoadPrint(aName, hDLLInst);
# endif
             break;
        }

        case DLL_PROCESS_DETACH:
        {
            LogRel(("crUtil DLL unloaded."));
# ifdef IN_GUEST
            VbglR3Term();
# endif
        }

        default:
            break;
    }

    return TRUE;
}
#endif


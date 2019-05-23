/* $Id: VBoxSupLib-win.cpp $ */
/** @file
 * IPRT - VBoxSupLib.dll, Windows.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/nt/nt-and-windows.h>

#include <iprt/path.h>


/**
 * The Dll main entry point.
 * @remarks The dllexport is for forcing the linker to generate an import
 *          library, so the build system doesn't get confused.
 */
extern "C" __declspec(dllexport)
BOOL __stdcall DllMainEntrypoint(HANDLE hModule, DWORD dwReason, PVOID pvReserved)
{
    RT_NOREF1(pvReserved);

    switch (dwReason)
    {
        /*
         * Make sure the DLL isn't ever unloaded.
         */
        case DLL_PROCESS_ATTACH:
        {
            WCHAR wszName[RTPATH_MAX];
            SetLastError(NO_ERROR);
            if (   GetModuleFileNameW((HMODULE)hModule, wszName, RT_ELEMENTS(wszName)) > 0
                && RtlGetLastWin32Error() == NO_ERROR)
            {
                int cExtraLoads = 2;
                while (cExtraLoads-- > 0)
                    LoadLibraryW(wszName);
            }
            break;
        }

        case DLL_THREAD_ATTACH:
        {
#ifdef VBOX_WITH_HARDENING
# ifndef VBOX_WITHOUT_DEBUGGER_CHECKS
            /*
             * Anti debugging hack that prevents most debug notifications from
             * ending up in the debugger.
             */
            NTSTATUS rcNt = NtSetInformationThread(GetCurrentThread(), ThreadHideFromDebugger, NULL, 0);
            if (!NT_SUCCESS(rcNt))
            {
                __debugbreak();
                return FALSE;
            }
# endif
#endif
            break;
        }

        case DLL_THREAD_DETACH:
            /* Nothing to do. */
            break;

        case DLL_PROCESS_DETACH:
            /* Nothing to do. */
            break;

        default:
            /* ignore */
            break;
    }
    return TRUE;
}


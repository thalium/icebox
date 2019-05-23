/* $Id: vcc100-ntdll-fakes.cpp $ */
/** @file
 * IPRT - Tricks to make the Visual C++ 2010 CRT work on NT4, W2K and XP - NTDLL.DLL.
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
#include <iprt/cdefs.h>
#include <iprt/types.h>

#ifndef RT_ARCH_X86
# error "This code is X86 only"
#endif

#include <iprt/win/windows.h>



/** Dynamically resolves an kernel32 API.   */
#define RESOLVE_ME(ApiNm) \
    static bool volatile    s_fInitialized = false; \
    static decltype(ApiNm) *s_pfnApi = NULL; \
    decltype(ApiNm)        *pfnApi; \
    if (!s_fInitialized) \
        pfnApi = s_pfnApi; \
    else \
    { \
        pfnApi = (decltype(pfnApi))GetProcAddress(GetModuleHandleW(L"ntdll.dll"), #ApiNm); \
        s_pfnApi = pfnApi; \
        s_fInitialized = true; \
    } do {} while (0) \


extern "C"
__declspec(dllexport)
ULONG WINAPI RtlGetLastWin32Error(VOID)
{
    RESOLVE_ME(RtlGetLastWin32Error);
    if (pfnApi)
        return pfnApi();
    return GetLastError();
}


/* Dummy to force dragging in this object in the link, so the linker
   won't accidentally use the symbols from kernel32. */
extern "C" int vcc100_ntdll_fakes_cpp(void)
{
    return 42;
}


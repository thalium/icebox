/* $Id: VBoxCommon.cpp $ */
/** @file
 * VBoxCommon - Misc helper routines for install helper.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/win/windows.h>
#include <tchar.h>
#include <stdio.h>

#include <msi.h>
#include <msiquery.h>


#if (_MSC_VER < 1400) /* Provide swprintf_s to VC < 8.0. */
int swprintf_s(WCHAR *buffer, size_t cbBuffer, const WCHAR *format, ...)
{
    int ret;
    va_list va;
    va_start(va, format);
    ret = _vsnwprintf(buffer, cbBuffer, format, va);
    va_end(va);
    return ret;
}
#endif

UINT VBoxGetProperty(MSIHANDLE a_hModule, WCHAR *a_pwszName, WCHAR *a_pwszValue, DWORD a_dwSize)
{
    DWORD dwBuffer = 0;
    UINT uiRet = MsiGetPropertyW(a_hModule, a_pwszName, L"", &dwBuffer);
    if (uiRet == ERROR_MORE_DATA)
    {
        ++dwBuffer;     /* On output does not include terminating null, so add 1. */

        if (dwBuffer > a_dwSize)
            return ERROR_MORE_DATA;

        ZeroMemory(a_pwszValue, a_dwSize);
        uiRet = MsiGetPropertyW(a_hModule, a_pwszName, a_pwszValue, &dwBuffer);
    }
    return uiRet;
}

UINT VBoxSetProperty(MSIHANDLE a_hModule, WCHAR *a_pwszName, WCHAR *a_pwszValue)
{
    return MsiSetPropertyW(a_hModule, a_pwszName, a_pwszValue);
}


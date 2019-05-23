/* $Id: sw_common.c $ */
/** @file
 * VBox D3D8/9 dll switcher
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/win/windows.h>
#include "switcher.h"

static char* gsBlackListExe[] = {"Dwm.exe", "java.exe", "javaw.exe", "javaws.exe"/*, "taskeng.exe"*/, NULL};
static char* gsBlackListDll[] = {"awt.dll", "wpfgfx_v0400.dll", "wpfgfx_v0300.dll", NULL};

/**
 * Loads a system DLL.
 *
 * @returns Module handle or NULL
 * @param   pszName             The DLL name.
 */
static HMODULE loadSystemDll(const char *pszName)
{
    char   szPath[MAX_PATH];
    UINT   cchPath = GetSystemDirectoryA(szPath, sizeof(szPath));
    size_t cbName  = strlen(pszName) + 1;
    if (cchPath + 1 + cbName > sizeof(szPath))
        return NULL;
    szPath[cchPath] = '\\';
    memcpy(&szPath[cchPath + 1], pszName, cbName);
    return LoadLibraryA(szPath);
}

/* Checks if 3D is enabled for VM and it works on host machine */
BOOL isVBox3DEnabled(void)
{
    DrvValidateVersionProc pDrvValidateVersion;
    HANDLE hDLL;
    BOOL result = FALSE;

#ifdef VBOX_WDDM_WOW64
    hDLL = loadSystemDll("VBoxOGL-x86.dll");
#else
    hDLL = loadSystemDll("VBoxOGL.dll");
#endif

    /* note: this isn't really needed as our library will refuse to load if it can't connect to host.
       so it's in case we'd change it one day.
    */
    pDrvValidateVersion = (DrvValidateVersionProc) GetProcAddress(hDLL, "DrvValidateVersion");
    if (pDrvValidateVersion)
    {
        result = pDrvValidateVersion(0);
    }
    FreeLibrary(hDLL);
    return result;
}

BOOL checkOptionsDll(void)
{
    int i;
    for (i=0; gsBlackListDll[i]; ++i)
    {
        if (GetModuleHandleA(gsBlackListDll[i]))
            return FALSE;
    }

    return TRUE;
}

BOOL checkOptionsExe(void)
{
    char name[1000];
    char *filename = name, *pName;
    int i;

        if (!GetModuleFileName(NULL, name, 1000))
                return TRUE;

    /*Extract filename*/
    for (pName=name; *pName; ++pName)
    {
        switch (*pName)
        {
            case ':':
            case '\\':
            case '/':
                filename = pName + 1;
                break;
        }
    }

    for (i=0; gsBlackListExe[i]; ++i)
    {
        if (!stricmp(filename, gsBlackListExe[i]))
            return FALSE;
    }

    return TRUE;
}

BOOL checkOptions(void)
{
    if (!checkOptionsDll())
        return FALSE;

    if (!checkOptionsExe())
        return FALSE;

    return TRUE;
}

void InitD3DExports(const char *vboxName, const char *msName)
{
    const char *dllName;
    HANDLE hDLL;

    if (isVBox3DEnabled() && checkOptions())
        dllName = vboxName;
    else
        dllName = msName;

    hDLL = loadSystemDll(dllName);
    FillD3DExports(hDLL);
}


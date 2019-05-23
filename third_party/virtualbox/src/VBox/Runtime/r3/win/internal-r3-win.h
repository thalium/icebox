/* $Id: internal-r3-win.h $ */
/** @file
 * IPRT - some Windows OS type constants.
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


#ifndef ___internal_r3_win_h
#define ___internal_r3_win_h

#include "internal/iprt.h"
#include <iprt/types.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Windows OS type as determined by rtSystemWinOSType().
 *
 * @note ASSUMPTIONS are made regarding ordering. Win 9x should come first, then
 *       NT. The Win9x and NT versions should internally be ordered in ascending
 *       version/code-base order.
 */
typedef enum RTWINOSTYPE
{
    kRTWinOSType_UNKNOWN    = 0,
    kRTWinOSType_9XFIRST    = 1,
    kRTWinOSType_95         = kRTWinOSType_9XFIRST,
    kRTWinOSType_95SP1,
    kRTWinOSType_95OSR2,
    kRTWinOSType_98,
    kRTWinOSType_98SP1,
    kRTWinOSType_98SE,
    kRTWinOSType_ME,
    kRTWinOSType_9XLAST     = 99,
    kRTWinOSType_NTFIRST    = 100,
    kRTWinOSType_NT31       = kRTWinOSType_NTFIRST,
    kRTWinOSType_NT351,
    kRTWinOSType_NT4,
    kRTWinOSType_2K,                        /* 5.0 */
    kRTWinOSType_XP,                        /* 5.1 */
    kRTWinOSType_XP64,                      /* 5.2, workstation */
    kRTWinOSType_2003,                      /* 5.2 */
    kRTWinOSType_VISTA,                     /* 6.0, workstation */
    kRTWinOSType_2008,                      /* 6.0, server */
    kRTWinOSType_7,                         /* 6.1, workstation */
    kRTWinOSType_2008R2,                    /* 6.1, server */
    kRTWinOSType_8,                         /* 6.2, workstation */
    kRTWinOSType_2012,                      /* 6.2, server */
    kRTWinOSType_81,                        /* 6.3, workstation */
    kRTWinOSType_2012R2,                    /* 6.3, server */
    kRTWinOSType_10,                        /* 10.0, workstation */
    kRTWinOSType_2016,                      /* 10.0, server */
    kRTWinOSType_NT_UNKNOWN = 199,
    kRTWinOSType_NT_LAST    = kRTWinOSType_UNKNOWN
} RTWINOSTYPE;

/**
 * Windows loader protection level.
 */
typedef enum RTR3WINLDRPROT
{
    RTR3WINLDRPROT_INVALID = 0,
    RTR3WINLDRPROT_NONE,
    RTR3WINLDRPROT_NO_CWD,
    RTR3WINLDRPROT_SAFE,
    RTR3WINLDRPROT_SAFER
} RTR3WINLDRPROT;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
extern DECLHIDDEN(RTR3WINLDRPROT)   g_enmWinLdrProt;
extern DECLHIDDEN(RTWINOSTYPE)      g_enmWinVer;
#ifdef _WINDEF_
extern DECLHIDDEN(HMODULE)          g_hModKernel32;
extern DECLHIDDEN(HMODULE)          g_hModNtDll;
extern DECLHIDDEN(OSVERSIONINFOEXW) g_WinOsInfoEx;
typedef UINT (WINAPI *PFNGETWINSYSDIR)(LPWSTR,UINT);
extern DECLHIDDEN(PFNGETWINSYSDIR)  g_pfnGetSystemWindowsDirectoryW;
#endif


#endif


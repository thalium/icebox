/* $Id: vcc100-msvcrt-fakes.cpp $ */
/** @file
 * IPRT - Tricks to make the Visual C++ 2010 CRT work on NT4, W2K and XP.
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
#include <iprt/assert.h>
#include <stdlib.h>


#ifndef RT_ARCH_X86
# error "This code is X86 only"
#endif



/* This one is static in libcmt, fortunately no rocket science.  */
extern "C" void __cdecl my_initterm(PFNRT *papfnStart, PFNRT *papfnEnd)
{
    for (; (uintptr_t)papfnStart < (uintptr_t)papfnEnd; papfnStart++)
        if (*papfnStart)
            (*papfnStart)();
}

extern "C" PFNRT __cdecl my_dllonexit(PFNRT pfnToAdd, PFNRT **ppapfnStart, PFNRT **ppapfnEnd)
{
    /* This is _very_ crude, but it'll probably do for our purposes... */
    size_t cItems = *ppapfnEnd - *ppapfnStart;
    *ppapfnStart = (PFNRT *)realloc(*ppapfnStart, (cItems + 1) * sizeof(**ppapfnStart));
    (*ppapfnStart)[cItems] = pfnToAdd;
    *ppapfnEnd = &(*ppapfnStart)[cItems + 1];
    return pfnToAdd;
}

extern "C" int _newmode;
extern "C" int __cdecl __setargv(void);
extern "C" int __cdecl _setargv(void);

extern "C" int __cdecl my_getmainargs(int *pcArgs, char ***ppapszArgs, char ***ppapszEnv, int fDoWildcardExp, int *pfNewMode)
{
    _newmode = *pfNewMode;

    Assert(!fDoWildcardExp);
    int rc = _setargv();
    if (rc >= 0)
    {
        *pcArgs     = __argc;
        *ppapszArgs = __argv;
        *ppapszEnv  = _environ;
    }
    return rc;
}

extern "C" void __cdecl my_setusermatherr(PFNRT pfnIgnore)
{
    /* pure stub. */
}


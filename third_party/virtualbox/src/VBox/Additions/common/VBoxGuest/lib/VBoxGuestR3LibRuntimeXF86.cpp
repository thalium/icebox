/* $Id: VBoxGuestR3LibRuntimeXF86.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions,
 *                  implements the minimum of runtime functions needed for
 *                  XFree86 driver code.
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
#include <iprt/log.h>
#include <iprt/mem.h>
#if defined(VBOX_VBGLR3_XFREE86)
extern "C" {
# define XFree86LOADER
# include <xf86_ansic.h>
# undef size_t
}
#else
# include <stdarg.h>
# include <stdlib.h>
# define xalloc malloc
# define xfree free
extern "C" void ErrorF(const char *f, ...);
#endif

RTDECL(void)    RTAssertMsg1Weak(const char *pszExpr, unsigned uLine, const char *pszFile, const char *pszFunction)
{
    ErrorF("Assertion failed!  Expression: %s at %s in\n", pszExpr,
           pszFunction);
    ErrorF("%s:%u\n", pszFile, uLine);
}

RTDECL(void)    RTAssertMsg2Weak(const char *pszFormat, ...)
{
    NOREF(pszFormat);
}

RTDECL(bool)    RTAssertShouldPanic(void)
{
    return false;
}

RTDECL(PRTLOGGER) RTLogDefaultInstanceEx(uint32_t fFlagsAndGroup)
{
    NOREF(fFlagsAndGroup);
    return NULL;
}

RTDECL(PRTLOGGER) RTLogRelGetDefaultInstance(void)
{
    return NULL;
}

RTDECL(PRTLOGGER) RTLogRelGetDefaultInstanceEx(uint32_t fFlagsAndGroup)
{
    NOREF(fFlagsAndGroup);
    return NULL;
}

RTDECL(void) RTLogLoggerEx(PRTLOGGER, unsigned, unsigned, const char *pszFormat, ...)
{
    NOREF(pszFormat);
}

RTDECL(void *)  RTMemTmpAllocTag(size_t cb, const char *pszTag)
{
    NOREF(pszTag);
    return xalloc(cb);
}

RTDECL(void)    RTMemTmpFree(void *pv)
{
    xfree(pv);
}

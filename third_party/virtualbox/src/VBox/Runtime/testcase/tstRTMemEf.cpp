/* $Id: tstRTMemEf.cpp $ */
/** @file
 * IPRT - Testcase for the RTMemEf* functions.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include <iprt/mem.h>

#include <iprt/asm.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static unsigned g_cErrors = 0;


static int tstMemAllocEfAccess()
{
    /* Trivial alloc fence test - allocate a single word and access both
     * the word after the allocated block and the word before. One of them
     * will crash no matter whether the fence is at the bottom or on top. */
    int32_t *p = (int32_t *)RTMemEfAllocNP(sizeof(int32_t), RTMEM_TAG);
    RTPrintf("tstRTMemAllocEfAccess: allocated int32_t at %#p\n", p);
    RTPrintf("tstRTMemAllocEfAccess: triggering buffer overrun...\n");
    ASMProbeReadByte(p + 1);
    RTPrintf("tstRTMemAllocEfAccess: triggering buffer underrun...\n");
    ASMProbeReadByte((char *)p - 1);

    /* Reaching this is a severe error. */
    return 0;
}

int main()
{
    RTR3InitExeNoArguments(0);
    RTPrintf("tstRTMemEf: TESTING...\n");

#define CHECK_EXPR(expr) \
    do { bool const f = !!(expr); if (!f) { RTPrintf("tstRTMemEf(%d): %s!\n", __LINE__, #expr); g_cErrors++; } } while (0)

    /*
     * Some simple stuff.
     */
    {
        CHECK_EXPR(tstMemAllocEfAccess());
    }

    /*
     * Summary.
     */
    if (!g_cErrors)
        RTPrintf("tstMemAutoPtr: SUCCESS\n");
    else
        RTPrintf("tstMemAutoPtr: FAILED - %d errors\n", g_cErrors);
    return !!g_cErrors;
}

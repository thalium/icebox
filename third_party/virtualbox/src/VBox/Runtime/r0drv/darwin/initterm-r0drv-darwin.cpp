/* $Id: initterm-r0drv-darwin.cpp $ */
/** @file
 * IPRT - Initialization & Termination, R0 Driver, Darwin.
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
#include "the-darwin-kernel.h"
#include "internal/iprt.h"

#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/dbg.h>
#include "internal/initterm.h"



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Pointer to the lock group used by IPRT. */
lck_grp_t                  *g_pDarwinLockGroup = NULL;
/** Pointer to the ast_pending function, if found. */
PFNR0DARWINASTPENDING       g_pfnR0DarwinAstPending = NULL;
/** Pointer to the cpu_interrupt function, if found. */
PFNR0DARWINCPUINTERRUPT     g_pfnR0DarwinCpuInterrupt = NULL;


DECLHIDDEN(int) rtR0InitNative(void)
{
    IPRT_DARWIN_SAVE_EFL_AC();

    /*
     * Create the lock group.
     */
    g_pDarwinLockGroup = lck_grp_alloc_init("IPRT", LCK_GRP_ATTR_NULL);
    AssertReturn(g_pDarwinLockGroup, VERR_NO_MEMORY);

    /*
     * Initialize the preemption hacks.
     */
    int rc = rtThreadPreemptDarwinInit();
    if (RT_SUCCESS(rc))
    {
        /*
         * Try resolve kernel symbols we need but apple don't wish to give us.
         */
        RTDBGKRNLINFO hKrnlInfo;
        rc = RTR0DbgKrnlInfoOpen(&hKrnlInfo, 0 /*fFlags*/);
        if (RT_SUCCESS(rc))
        {
            RTR0DbgKrnlInfoQuerySymbol(hKrnlInfo, NULL, "ast_pending",   (void **)&g_pfnR0DarwinAstPending);
            printf("ast_pending=%p\n", g_pfnR0DarwinAstPending);
            RTR0DbgKrnlInfoQuerySymbol(hKrnlInfo, NULL, "cpu_interrupt", (void **)&g_pfnR0DarwinCpuInterrupt);
            printf("cpu_interrupt=%p\n", g_pfnR0DarwinCpuInterrupt);
            RTR0DbgKrnlInfoRelease(hKrnlInfo);
        }
        if (RT_FAILURE(rc))
        {
            printf("rtR0InitNative: warning! failed to resolve special kernel symbols\n");
            rc = VINF_SUCCESS;
        }
    }
    if (RT_FAILURE(rc))
        rtR0TermNative();

    IPRT_DARWIN_RESTORE_EFL_AC();
    return rc;
}


DECLHIDDEN(void) rtR0TermNative(void)
{
    IPRT_DARWIN_SAVE_EFL_AC();

    /*
     * Preemption hacks before the lock group.
     */
    rtThreadPreemptDarwinTerm();

    /*
     * Free the lock group.
     */
    if (g_pDarwinLockGroup)
    {
        lck_grp_free(g_pDarwinLockGroup);
        g_pDarwinLockGroup = NULL;
    }

    IPRT_DARWIN_RESTORE_EFL_AC();
}


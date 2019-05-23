/* $Id: tstTime-2.cpp $ */
/** @file
 * IPRT Testcase - Simple RTTime test.
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
#include <iprt/time.h>
#include <iprt/stream.h>
#include <iprt/initterm.h>
#include <iprt/thread.h>
#include <VBox/sup.h>



int main()
{
    unsigned cErrors = 0;
    int i;
    RTR3InitExeNoArguments(RTR3INIT_FLAGS_SUPLIB);
    RTPrintf("tstTime-2: TESTING...\n");

    /*
     * RTNanoTimeTS() shall never return something which
     * is less or equal to the return of the previous call.
     */

    RTTimeSystemNanoTS(); RTTimeNanoTS(); RTThreadYield();
    uint64_t u64RTStartTS = RTTimeNanoTS();
    uint64_t u64OSStartTS = RTTimeSystemNanoTS();
#define NUMBER_OF_CALLS (100*_1M)

    for (i = 0; i < NUMBER_OF_CALLS; i++)
        RTTimeNanoTS();

    uint64_t u64RTElapsedTS = RTTimeNanoTS();
    uint64_t u64OSElapsedTS = RTTimeSystemNanoTS();
    u64RTElapsedTS -= u64RTStartTS;
    u64OSElapsedTS -= u64OSStartTS;
    int64_t i64Diff = u64OSElapsedTS >= u64RTElapsedTS ? u64OSElapsedTS - u64RTElapsedTS : u64RTElapsedTS - u64OSElapsedTS;
    if (i64Diff > (int64_t)(u64OSElapsedTS / 1000))
    {
        RTPrintf("tstTime-2: error: total time differs too much! u64OSElapsedTS=%#llx u64RTElapsedTS=%#llx delta=%lld\n",
                 u64OSElapsedTS, u64RTElapsedTS, u64OSElapsedTS - u64RTElapsedTS);
        cErrors++;
    }
    else
        RTPrintf("tstTime-2: total time difference: u64OSElapsedTS=%#llx u64RTElapsedTS=%#llx delta=%lld\n",
                 u64OSElapsedTS, u64RTElapsedTS, u64OSElapsedTS - u64RTElapsedTS);

    RTPrintf("tstTime-2: %u calls to RTTimeNanoTS\n", NUMBER_OF_CALLS);
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86) /** @todo This isn't really x86 or AMD64 specific... */
    RTPrintf("tstTime-2: RTTimeDbgSteps   -> %u (%d ppt)\n", RTTimeDbgSteps(),   ((uint64_t)RTTimeDbgSteps() * 1000) / NUMBER_OF_CALLS);
    RTPrintf("tstTime-2: RTTimeDbgExpired -> %u (%d ppt)\n", RTTimeDbgExpired(), ((uint64_t)RTTimeDbgExpired() * 1000) / NUMBER_OF_CALLS);
    RTPrintf("tstTime-2: RTTimeDbgBad     -> %u (%d ppt)\n", RTTimeDbgBad(),     ((uint64_t)RTTimeDbgBad() * 1000) / NUMBER_OF_CALLS);
    RTPrintf("tstTime-2: RTTimeDbgRaces   -> %u (%d ppt)\n", RTTimeDbgRaces(),   ((uint64_t)RTTimeDbgRaces() * 1000) / NUMBER_OF_CALLS);
#endif

    if (!cErrors)
        RTPrintf("tstTime-2: SUCCESS\n");
    else
        RTPrintf("tstTime-2: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}

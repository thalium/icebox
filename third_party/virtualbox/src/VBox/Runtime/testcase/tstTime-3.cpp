/* $Id: tstTime-3.cpp $ */
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
#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>

#else /* posix */
# include <sys/time.h>
#endif

#include <iprt/time.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/initterm.h>
#include <iprt/thread.h>
#include <iprt/err.h>


DECLINLINE(uint64_t) OSNanoTS(void)
{
#ifdef RT_OS_WINDOWS
    uint64_t u64; /* manual say larger integer, should be safe to assume it's the same. */
    GetSystemTimeAsFileTime((LPFILETIME)&u64);
    return u64 * 100;

#else /* posix */

    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec  * (uint64_t)(1000 * 1000 * 1000)
         + (uint64_t)(tv.tv_usec * 1000);
#endif
}



int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_SUPLIB);

    if (argc <= 1)
    {
        RTPrintf("tstTime-3: usage: tstTime-3 <seconds> [seconds2 [..]]\n");
        return 1;
    }

    RTPrintf("tstTime-3: Testing difference between RTTimeNanoTS() and OS time...\n");

    for (int i = 1; i < argc; i++)
    {
        uint64_t cSeconds = 0;
        int rc = RTStrToUInt64Ex(argv[i], NULL, 0, &cSeconds);
        if (RT_FAILURE(rc))
        {
            RTPrintf("tstTime-3: Invalid argument %d: %s\n", i, argv[i]);
            return 1;
        }
        RTPrintf("tstTime-3: %d - %RU64 seconds period...\n", i, cSeconds);

        RTTimeNanoTS(); OSNanoTS(); RTThreadSleep(1);
        uint64_t u64RTStartTS = RTTimeNanoTS();
        uint64_t u64OSStartTS = OSNanoTS();

        RTThreadSleep(cSeconds * 1000);

        uint64_t u64RTElapsedTS = RTTimeNanoTS();
        uint64_t u64OSElapsedTS = OSNanoTS();
        u64RTElapsedTS -= u64RTStartTS;
        u64OSElapsedTS -= u64OSStartTS;

        RTPrintf("tstTime-3: %d -   RT: %16RU64 ns\n", i, u64RTElapsedTS);
        RTPrintf("tstTime-3: %d -   OS: %16RU64 ns\n", i, u64OSElapsedTS);
        RTPrintf("tstTime-3: %d - diff: %16RI64 ns\n", i, u64RTElapsedTS - u64OSElapsedTS);
    }

    return 0;
}

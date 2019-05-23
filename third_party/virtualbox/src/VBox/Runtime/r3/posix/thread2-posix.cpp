/* $Id: thread2-posix.cpp $ */
/** @file
 * IPRT - Threads part 2, POSIX.
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
#define LOG_GROUP RTLOGGROUP_THREAD
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#if defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD)
# include <sched.h>
#endif

#include <iprt/thread.h>
#include <iprt/log.h>
#include <iprt/asm.h>
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
# include <iprt/asm-amd64-x86.h>
#endif
#include <iprt/err.h>
#include "internal/thread.h"


RTDECL(RTNATIVETHREAD) RTThreadNativeSelf(void)
{
    return (RTNATIVETHREAD)pthread_self();
}


RTDECL(int) RTThreadSleep(RTMSINTERVAL cMillies)
{
    LogFlow(("RTThreadSleep: cMillies=%d\n", cMillies));
    if (!cMillies)
    {
        /* pthread_yield() isn't part of SuS, thus this fun. */
#ifdef RT_OS_DARWIN
        pthread_yield_np();
#elif defined(RT_OS_SOLARIS) || defined(RT_OS_HAIKU) || defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD)
        sched_yield();
#else
        if (!pthread_yield())
#endif
        {
            LogFlow(("RTThreadSleep: returning %Rrc (cMillies=%d)\n", VINF_SUCCESS, cMillies));
            return VINF_SUCCESS;
        }
    }
    else
    {
        struct timespec ts;
        struct timespec tsrem = {0,0};

        ts.tv_nsec = (cMillies % 1000) * 1000000;
        ts.tv_sec  = cMillies / 1000;
        if (!nanosleep(&ts, &tsrem))
        {
            LogFlow(("RTThreadSleep: returning %Rrc (cMillies=%d)\n", VINF_SUCCESS, cMillies));
            return VINF_SUCCESS;
        }
    }

    int rc = RTErrConvertFromErrno(errno);
    LogFlow(("RTThreadSleep: returning %Rrc (cMillies=%d)\n", rc, cMillies));
    return rc;
}


RTDECL(int) RTThreadSleepNoLog(RTMSINTERVAL cMillies)
{
    if (!cMillies)
    {
        /* pthread_yield() isn't part of SuS, thus this fun. */
#ifdef RT_OS_DARWIN
        pthread_yield_np();
#elif defined(RT_OS_SOLARIS) || defined(RT_OS_HAIKU) || defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD)
        sched_yield();
#else
        if (!pthread_yield())
#endif
            return VINF_SUCCESS;
    }
    else
    {
        struct timespec ts;
        struct timespec tsrem = {0,0};

        ts.tv_nsec = (cMillies % 1000) * 1000000;
        ts.tv_sec  = cMillies / 1000;
        if (!nanosleep(&ts, &tsrem))
            return VINF_SUCCESS;
    }

    return RTErrConvertFromErrno(errno);
}


RTDECL(bool) RTThreadYield(void)
{
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    uint64_t u64TS = ASMReadTSC();
#endif
#ifdef RT_OS_DARWIN
    pthread_yield_np();
#elif defined(RT_OS_SOLARIS) || defined(RT_OS_HAIKU) || defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD)
    sched_yield();
#else
    pthread_yield();
#endif
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    u64TS = ASMReadTSC() - u64TS;
    bool fRc = u64TS > 1500;
    LogFlow(("RTThreadYield: returning %d (%llu ticks)\n", fRc, u64TS));
#else
    bool fRc = true; /* PORTME: Add heuristics for determining whether the cpus was yielded. */
#endif
    return fRc;
}


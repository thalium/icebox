/* $Id: thread-posix.cpp $ */
/** @file
 * IPRT - Threads, POSIX.
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
#include <signal.h>
#if defined(RT_OS_LINUX)
# include <unistd.h>
# include <sys/syscall.h>
#endif
#if defined(RT_OS_SOLARIS)
# include <sched.h>
# include <sys/resource.h>
#endif
#if defined(RT_OS_DARWIN)
# include <mach/thread_act.h>
# include <mach/thread_info.h>
# include <mach/host_info.h>
# include <mach/mach_init.h>
# include <mach/mach_host.h>
#endif
#if defined(RT_OS_DARWIN) /*|| defined(RT_OS_FREEBSD) - later */ \
 || (defined(RT_OS_LINUX) && !defined(IN_RT_STATIC) /* static + dlsym = trouble */) \
 || defined(IPRT_MAY_HAVE_PTHREAD_SET_NAME_NP)
# define IPRT_MAY_HAVE_PTHREAD_SET_NAME_NP
# include <dlfcn.h>
#endif
#if defined(RT_OS_HAIKU)
# include <OS.h>
#endif

#include <iprt/thread.h>
#include <iprt/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/string.h>
#include "internal/thread.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifndef IN_GUEST
/** Includes RTThreadPoke. */
# define RTTHREAD_POSIX_WITH_POKE
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The pthread key in which we store the pointer to our own PRTTHREAD structure. */
static pthread_key_t    g_SelfKey;
#ifdef RTTHREAD_POSIX_WITH_POKE
/** The signal we use for poking threads.
 * This is set to -1 if no available signal was found. */
static int              g_iSigPokeThread = -1;
#endif

#ifdef IPRT_MAY_HAVE_PTHREAD_SET_NAME_NP
# if defined(RT_OS_DARWIN)
/**
 * The Mac OS X (10.6 and later) variant of pthread_setname_np.
 *
 * @returns errno.h
 * @param   pszName         The new thread name.
 */
typedef int (*PFNPTHREADSETNAME)(const char *pszName);
# else
/**
 * The variant of pthread_setname_np most other unix-like systems implement.
 *
 * @returns errno.h
 * @param   hThread         The thread.
 * @param   pszName         The new thread name.
 */
typedef int (*PFNPTHREADSETNAME)(pthread_t hThread, const char *pszName);
# endif

/** Pointer to pthread_setname_np if found. */
static PFNPTHREADSETNAME g_pfnThreadSetName = NULL;
#endif /* IPRT_MAY_HAVE_PTHREAD_SET_NAME_NP */


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void *rtThreadNativeMain(void *pvArgs);
static void rtThreadKeyDestruct(void *pvValue);
#ifdef RTTHREAD_POSIX_WITH_POKE
static void rtThreadPosixPokeSignal(int iSignal);
#endif


#ifdef RTTHREAD_POSIX_WITH_POKE
/**
 * Try register the dummy signal handler for RTThreadPoke.
 */
static void rtThreadPosixSelectPokeSignal(void)
{
    /*
     * Note! Avoid SIGRTMIN thru SIGRTMIN+2 because of LinuxThreads.
     */
    static const int s_aiSigCandidates[] =
    {
# ifdef SIGRTMAX
        SIGRTMAX-3,
        SIGRTMAX-2,
        SIGRTMAX-1,
# endif
# ifndef RT_OS_SOLARIS
        SIGUSR2,
# endif
        SIGWINCH
    };

    g_iSigPokeThread = -1;
    if (!RTR3InitIsUnobtrusive())
    {
        for (unsigned iSig = 0; iSig < RT_ELEMENTS(s_aiSigCandidates); iSig++)
        {
            struct sigaction SigActOld;
            if (!sigaction(s_aiSigCandidates[iSig], NULL, &SigActOld))
            {
                if (   SigActOld.sa_handler == SIG_DFL
                    || SigActOld.sa_handler == rtThreadPosixPokeSignal)
                {
                    struct sigaction SigAct;
                    RT_ZERO(SigAct);
                    SigAct.sa_handler = rtThreadPosixPokeSignal;
                    SigAct.sa_flags   = 0;
                    sigfillset(&SigAct.sa_mask);

                    /* ASSUMES no sigaction race... (lazy bird) */
                    if (!sigaction(s_aiSigCandidates[iSig], &SigAct, NULL))
                    {
                        g_iSigPokeThread = s_aiSigCandidates[iSig];
                        break;
                    }
                    AssertMsgFailed(("rc=%Rrc errno=%d\n", RTErrConvertFromErrno(errno), errno));
                }
            }
            else
                AssertMsgFailed(("rc=%Rrc errno=%d\n", RTErrConvertFromErrno(errno), errno));
        }
    }
}
#endif /* RTTHREAD_POSIX_WITH_POKE */


DECLHIDDEN(int) rtThreadNativeInit(void)
{
    /*
     * Allocate the TLS (key in posix terms) where we store the pointer to
     * a threads RTTHREADINT structure.
     */
    int rc = pthread_key_create(&g_SelfKey, rtThreadKeyDestruct);
    if (rc)
        return VERR_NO_TLS_FOR_SELF;

#ifdef RTTHREAD_POSIX_WITH_POKE
    rtThreadPosixSelectPokeSignal();
#endif

#ifdef IPRT_MAY_HAVE_PTHREAD_SET_NAME_NP
    if (RT_SUCCESS(rc))
        g_pfnThreadSetName = (PFNPTHREADSETNAME)(uintptr_t)dlsym(RTLD_DEFAULT, "pthread_setname_np");
#endif
    return rc;
}

static void rtThreadPosixBlockSignals(void)
{
    /*
     * Block SIGALRM - required for timer-posix.cpp.
     * This is done to limit harm done by OSes which doesn't do special SIGALRM scheduling.
     * It will not help much if someone creates threads directly using pthread_create. :/
     */
    if (!RTR3InitIsUnobtrusive())
    {
        sigset_t SigSet;
        sigemptyset(&SigSet);
        sigaddset(&SigSet, SIGALRM);
        sigprocmask(SIG_BLOCK, &SigSet, NULL);
    }
#ifdef RTTHREAD_POSIX_WITH_POKE
    if (g_iSigPokeThread != -1)
        siginterrupt(g_iSigPokeThread, 1);
#endif
}

DECLHIDDEN(void) rtThreadNativeReInitObtrusive(void)
{
#ifdef RTTHREAD_POSIX_WITH_POKE
    Assert(!RTR3InitIsUnobtrusive());
    rtThreadPosixSelectPokeSignal();
#endif
    rtThreadPosixBlockSignals();
}


/**
 * Destructor called when a thread terminates.
 * @param   pvValue     The key value. PRTTHREAD in our case.
 */
static void rtThreadKeyDestruct(void *pvValue)
{
    /*
     * Deal with alien threads.
     */
    PRTTHREADINT pThread = (PRTTHREADINT)pvValue;
    if (pThread->fIntFlags & RTTHREADINT_FLAGS_ALIEN)
    {
        pthread_setspecific(g_SelfKey, pThread);
        rtThreadTerminate(pThread, 0);
        pthread_setspecific(g_SelfKey, NULL);
    }
}


#ifdef RTTHREAD_POSIX_WITH_POKE
/**
 * Dummy signal handler for the poke signal.
 *
 * @param   iSignal     The signal number.
 */
static void rtThreadPosixPokeSignal(int iSignal)
{
    Assert(iSignal == g_iSigPokeThread);
    NOREF(iSignal);
}
#endif


/**
 * Adopts a thread, this is called immediately after allocating the
 * thread structure.
 *
 * @param   pThread     Pointer to the thread structure.
 */
DECLHIDDEN(int) rtThreadNativeAdopt(PRTTHREADINT pThread)
{
    rtThreadPosixBlockSignals();

    int rc = pthread_setspecific(g_SelfKey, pThread);
    if (!rc)
        return VINF_SUCCESS;
    return VERR_FAILED_TO_SET_SELF_TLS;
}


DECLHIDDEN(void) rtThreadNativeDestroy(PRTTHREADINT pThread)
{
    if (pThread == (PRTTHREADINT)pthread_getspecific(g_SelfKey))
        pthread_setspecific(g_SelfKey, NULL);
}


/**
 * Wrapper which unpacks the params and calls thread function.
 */
static void *rtThreadNativeMain(void *pvArgs)
{
    PRTTHREADINT  pThread = (PRTTHREADINT)pvArgs;
    pthread_t     Self    = pthread_self();
    Assert((uintptr_t)Self != NIL_RTNATIVETHREAD);
    Assert(Self == (pthread_t)(RTNATIVETHREAD)Self);

#if defined(RT_OS_LINUX)
    /*
     * Set the TID.
     */
    pThread->tid = syscall(__NR_gettid);
    ASMMemoryFence();
#endif

    rtThreadPosixBlockSignals();

    /*
     * Set the TLS entry and, if possible, the thread name.
     */
    int rc = pthread_setspecific(g_SelfKey, pThread);
    AssertReleaseMsg(!rc, ("failed to set self TLS. rc=%d thread '%s'\n", rc, pThread->szName));

#ifdef IPRT_MAY_HAVE_PTHREAD_SET_NAME_NP
    if (g_pfnThreadSetName)
# ifdef RT_OS_DARWIN
        g_pfnThreadSetName(pThread->szName);
# else
        g_pfnThreadSetName(Self, pThread->szName);
# endif
#endif

    /*
     * Call common main.
     */
    rc = rtThreadMain(pThread, (uintptr_t)Self, &pThread->szName[0]);

    pthread_setspecific(g_SelfKey, NULL);
    pthread_exit((void *)(intptr_t)rc);
    return (void *)(intptr_t)rc;
}


DECLHIDDEN(int) rtThreadNativeCreate(PRTTHREADINT pThread, PRTNATIVETHREAD pNativeThread)
{
    /*
     * Set the default stack size.
     */
    if (!pThread->cbStack)
        pThread->cbStack = 512*1024;

#ifdef RT_OS_LINUX
    pThread->tid = -1;
#endif

    /*
     * Setup thread attributes.
     */
    pthread_attr_t  ThreadAttr;
    int rc = pthread_attr_init(&ThreadAttr);
    if (!rc)
    {
        rc = pthread_attr_setdetachstate(&ThreadAttr, PTHREAD_CREATE_DETACHED);
        if (!rc)
        {
            rc = pthread_attr_setstacksize(&ThreadAttr, pThread->cbStack);
            if (!rc)
            {
                /*
                 * Create the thread.
                 */
                pthread_t ThreadId;
                rc = pthread_create(&ThreadId, &ThreadAttr, rtThreadNativeMain, pThread);
                if (!rc)
                {
                    pthread_attr_destroy(&ThreadAttr);
                    *pNativeThread = (uintptr_t)ThreadId;
                    return VINF_SUCCESS;
                }
            }
        }
        pthread_attr_destroy(&ThreadAttr);
    }
    return RTErrConvertFromErrno(rc);
}


RTDECL(RTTHREAD) RTThreadSelf(void)
{
    PRTTHREADINT pThread = (PRTTHREADINT)pthread_getspecific(g_SelfKey);
    /** @todo import alien threads? */
    return pThread;
}


#ifdef RTTHREAD_POSIX_WITH_POKE
RTDECL(int) RTThreadPoke(RTTHREAD hThread)
{
    AssertReturn(hThread != RTThreadSelf(), VERR_INVALID_PARAMETER);
    PRTTHREADINT pThread = rtThreadGet(hThread);
    AssertReturn(pThread, VERR_INVALID_HANDLE);

    int rc;
    if (g_iSigPokeThread != -1)
    {
        rc = pthread_kill((pthread_t)(uintptr_t)pThread->Core.Key, g_iSigPokeThread);
        rc = RTErrConvertFromErrno(rc);
    }
    else
        rc = VERR_NOT_SUPPORTED;

    rtThreadRelease(pThread);
    return rc;
}
#endif

/** @todo move this into platform specific files. */
RTR3DECL(int) RTThreadGetExecutionTimeMilli(uint64_t *pKernelTime, uint64_t *pUserTime)
{
#if defined(RT_OS_SOLARIS)
    struct rusage ts;
    int rc = getrusage(RUSAGE_LWP, &ts);
    if (rc)
        return RTErrConvertFromErrno(rc);

    *pKernelTime = ts.ru_stime.tv_sec * 1000 + ts.ru_stime.tv_usec / 1000;
    *pUserTime   = ts.ru_utime.tv_sec * 1000 + ts.ru_utime.tv_usec / 1000;
    return VINF_SUCCESS;

#elif defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)
    /* on Linux, getrusage(RUSAGE_THREAD, ...) is available since 2.6.26 */
    struct timespec ts;
    int rc = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    if (rc)
        return RTErrConvertFromErrno(rc);

    *pKernelTime = 0;
    *pUserTime = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return VINF_SUCCESS;

#elif defined(RT_OS_DARWIN)
    thread_basic_info       ThreadInfo;
    mach_msg_type_number_t  Count = THREAD_BASIC_INFO_COUNT;
    kern_return_t krc = thread_info(mach_thread_self(), THREAD_BASIC_INFO, (thread_info_t)&ThreadInfo, &Count);
    AssertReturn(krc == KERN_SUCCESS, RTErrConvertFromDarwinKern(krc));

    *pKernelTime = ThreadInfo.system_time.seconds * 1000 + ThreadInfo.system_time.microseconds / 1000;
    *pUserTime   = ThreadInfo.user_time.seconds   * 1000 + ThreadInfo.user_time.microseconds   / 1000;

    return VINF_SUCCESS;
#elif defined(RT_OS_HAIKU)
    thread_info       ThreadInfo;
    status_t status = get_thread_info(find_thread(NULL), &ThreadInfo);
    AssertReturn(status == B_OK, RTErrConvertFromErrno(status));

    *pKernelTime = ThreadInfo.kernel_time / 1000;
    *pUserTime   = ThreadInfo.user_time / 1000;

    return VINF_SUCCESS;
#else
    return VERR_NOT_IMPLEMENTED;
#endif
}


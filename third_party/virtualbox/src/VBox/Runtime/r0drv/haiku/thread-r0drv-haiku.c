/* $Id: thread-r0drv-haiku.c $ */
/** @file
 * IPRT - Threads, Ring-0 Driver, Haiku.
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
#include "the-haiku-kernel.h"
#include "internal/iprt.h"
#include <iprt/thread.h>

#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
#include <iprt/asm-amd64-x86.h>
#endif
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mp.h>


RTDECL(RTNATIVETHREAD) RTThreadNativeSelf(void)
{
    return (RTNATIVETHREAD)find_thread(NULL);
}


RTDECL(int) RTThreadSleep(RTMSINTERVAL cMillies)
{
    RT_ASSERT_PREEMPTIBLE();
    snooze((bigtime_t)cMillies * 1000);
    return VINF_SUCCESS;
}


RTDECL(bool) RTThreadYield(void)
{
    RT_ASSERT_PREEMPTIBLE();
    //FIXME
    //snooze(0);
    thread_yield(true);
    return true; /* this is fishy */
}


RTDECL(bool) RTThreadPreemptIsEnabled(RTTHREAD hThread)
{
    Assert(hThread == NIL_RTTHREAD);

    //XXX: can't do this, it might actually be held by another cpu
    //return !B_SPINLOCK_IS_LOCKED(&gThreadSpinlock);
    return ASMIntAreEnabled(); /** @todo find a better way. */
}


RTDECL(bool) RTThreadPreemptIsPending(RTTHREAD hThread)
{
    Assert(hThread == NIL_RTTHREAD);
    /** @todo check if Thread::next_priority or
     *        cpu_ent::invoke_scheduler could do. */
    return false;
}


RTDECL(bool) RTThreadPreemptIsPendingTrusty(void)
{
    /* RTThreadPreemptIsPending is not reliable yet. */
    return false;
}


RTDECL(bool) RTThreadPreemptIsPossible(void)
{
    /* yes, kernel preemption is possible. */
    return true;
}


RTDECL(void) RTThreadPreemptDisable(PRTTHREADPREEMPTSTATE pState)
{
    AssertPtr(pState);
    Assert(pState->uOldCpuState == 0);

    pState->uOldCpuState = (uint32_t)disable_interrupts();
    RT_ASSERT_PREEMPT_CPUID_DISABLE(pState);
}


RTDECL(void) RTThreadPreemptRestore(PRTTHREADPREEMPTSTATE pState)
{
    AssertPtr(pState);

    RT_ASSERT_PREEMPT_CPUID_RESTORE(pState);
    restore_interrupts((cpu_status)pState->uOldCpuState);
    pState->uOldCpuState = 0;
}


RTDECL(bool) RTThreadIsInInterrupt(RTTHREAD hThread)
{
    Assert(hThread == NIL_RTTHREAD); NOREF(hThread);
    /** @todo Implement RTThreadIsInInterrupt. Required for guest
     *        additions! */
    return !ASMIntAreEnabled();
}


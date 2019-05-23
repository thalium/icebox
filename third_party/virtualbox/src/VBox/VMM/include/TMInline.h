/* $Id: TMInline.h $ */
/** @file
 * TM - Common Inlined functions.
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
 */

#ifndef ___TMInline_h
#define ___TMInline_h


/**
 * Used to unlink a timer from the active list.
 *
 * @param   pQueue      The timer queue.
 * @param   pTimer      The timer that needs linking.
 *
 * @remarks Called while owning the relevant queue lock.
 */
DECL_FORCE_INLINE(void) tmTimerQueueUnlinkActive(PTMTIMERQUEUE pQueue, PTMTIMER pTimer)
{
#ifdef VBOX_STRICT
    TMTIMERSTATE const enmState = pTimer->enmState;
    Assert(  pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC
           ? enmState == TMTIMERSTATE_ACTIVE
           : enmState == TMTIMERSTATE_PENDING_SCHEDULE || enmState == TMTIMERSTATE_PENDING_STOP_SCHEDULE);
#endif

    const PTMTIMER pPrev = TMTIMER_GET_PREV(pTimer);
    const PTMTIMER pNext = TMTIMER_GET_NEXT(pTimer);
    if (pPrev)
        TMTIMER_SET_NEXT(pPrev, pNext);
    else
    {
        TMTIMER_SET_HEAD(pQueue, pNext);
        pQueue->u64Expire = pNext ? pNext->u64Expire : INT64_MAX;
        DBGFTRACE_U64_TAG(pTimer->CTX_SUFF(pVM), pQueue->u64Expire, "tmTimerQueueUnlinkActive");
    }
    if (pNext)
        TMTIMER_SET_PREV(pNext, pPrev);
    pTimer->offNext = 0;
    pTimer->offPrev = 0;
}

#endif


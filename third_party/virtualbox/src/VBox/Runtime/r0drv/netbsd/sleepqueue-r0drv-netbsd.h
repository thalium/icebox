/* $Id: sleepqueue-r0drv-netbsd.h $ */
/** @file
 * IPRT - NetBSD Ring-0 Driver Helpers for Abstracting Sleep Queues,
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


#ifndef ___r0drv_netbsd_sleepqueue_r0drv_netbsd_h
#define ___r0drv_netbsd_sleepqueue_r0drv_netbsd_h

#include "the-netbsd-kernel.h"

#include <iprt/asm-math.h>
#include <iprt/err.h>
#include <iprt/time.h>

static syncobj_t vbox_syncobj = {
        SOBJ_SLEEPQ_FIFO,
        sleepq_unsleep,
        sleepq_changepri,
        sleepq_lendpri,
        syncobj_noowner,
};

/**
 * Kernel mode Linux wait state structure.
 */
typedef struct RTR0SEMBSDSLEEP
{
    /** The absolute timeout given as nano seconds since the start of the
     *  monotonic clock. */
    uint64_t        uNsAbsTimeout;
    /** The timeout in ticks. Updated after waiting. */
    int             iTimeout;
    /** Set if it's an indefinite wait. */
    bool            fIndefinite;
    /** Set if we've already timed out.
     * Set by rtR0SemBsdWaitDoIt and read by rtR0SemBsdWaitHasTimedOut. */
    bool            fTimedOut;
    /** Flag whether the wait was interrupted. */
    bool            fInterrupted;
    /** flag whether the wait is interruptible or not. */
    bool            fInterruptible;
    /** Opaque wait channel id. */
    wchan_t         wchan;
    sleepq_t *sq;
    kmutex_t *sq_lock;
} RTR0SEMBSDSLEEP;
/** Pointer to a NetBSD wait state. */
typedef RTR0SEMBSDSLEEP *PRTR0SEMBSDSLEEP;


/**
 * Updates the timeout of the NetBSD wait.
 *
 * @returns RTSEMWAIT_FLAGS_INDEFINITE if the timeout value is too big.
 *          0 otherwise
 * @param   pWait               The wait structure.
 * @param   uTimeout            The relative timeout in nanoseconds.
 */
DECLINLINE(void) rtR0SemBsdWaitUpdateTimeout(PRTR0SEMBSDSLEEP pWait)
{
    /* Convert absolute timeout to ticks */
    uint64_t now = RTTimeNanoTS();
    if (now >= pWait->uNsAbsTimeout) {
        pWait->iTimeout = 0;
    } else {
        uint64_t nanos = pWait->uNsAbsTimeout - now;
        pWait->iTimeout = hz * nanos / 1000000000;
        /* for 1ms wait, wait at least one tick ?? */
        if ((pWait->iTimeout == 0) && (nanos >= 1000000)) {
            pWait->iTimeout = 1;
        }
    }
}

/**
 * Initializes a wait.
 *
 * The caller MUST check the wait condition BEFORE calling this function or the
 * timeout logic will be flawed.
 *
 * @returns VINF_SUCCESS or VERR_TIMEOUT.
 * @param   pWait               The wait structure.
 * @param   fFlags              The wait flags.
 * @param   uTimeout            The timeout.
 * @param   pvWaitChan          The opaque wait channel.
 */
DECLINLINE(int) rtR0SemBsdWaitInit(PRTR0SEMBSDSLEEP pWait, uint32_t fFlags, uint64_t uTimeout,
                                   void *pvWaitChan)
{
    if (fFlags & RTSEMWAIT_FLAGS_INDEFINITE) {
        pWait->fIndefinite      = true;
        pWait->iTimeout = 0;
        pWait->uNsAbsTimeout = 0;
    } else {
        if (fFlags & RTSEMWAIT_FLAGS_RELATIVE) {
            if (fFlags & RTSEMWAIT_FLAGS_MILLISECS) {
                pWait->uNsAbsTimeout = uTimeout * 1000000 + RTTimeSystemNanoTS();
            } else {
                pWait->uNsAbsTimeout = uTimeout + RTTimeSystemNanoTS();
            }
        } else {
            if (fFlags & RTSEMWAIT_FLAGS_MILLISECS) {
                pWait->uNsAbsTimeout = uTimeout * 1000000;
            } else {
                pWait->uNsAbsTimeout = uTimeout;
            }
        }
        rtR0SemBsdWaitUpdateTimeout(pWait);
        if (pWait->iTimeout == 0) {
            return VERR_TIMEOUT;
        }
    }

    pWait->fTimedOut   = false;
    /*
     * Initialize the wait queue related bits.
     */
    pWait->fInterruptible = fFlags & RTSEMWAIT_FLAGS_INTERRUPTIBLE
                            ? true : false;
    pWait->fInterrupted   = false;
    pWait->wchan = pvWaitChan;
    pWait->sq = NULL;
    pWait->sq_lock = NULL;

    return VINF_SUCCESS;
}

/**
 * Prepares the next wait.
 *
 * This must be called before rtR0SemBsdWaitDoIt, and the caller should check
 * the exit conditions inbetween the two calls.
 *
 * @param   pWait               The wait structure.
 */
DECLINLINE(void) rtR0SemBsdWaitPrepare(PRTR0SEMBSDSLEEP pWait)
{
    pWait->sq = sleeptab_lookup(&sleeptab, pWait->wchan, &pWait->sq_lock);
}

/**
 * Do the actual wait.
 *
 * @param   pWait               The wait structure.
 */
DECLINLINE(void) rtR0SemBsdWaitDoIt(PRTR0SEMBSDSLEEP pWait)
{
    sleepq_enter(pWait->sq, curlwp, pWait->sq_lock);
    sleepq_enqueue(pWait->sq, pWait->wchan, "VBoxIS", &vbox_syncobj);

    pWait->sq = NULL;
    pWait->sq_lock = NULL;

    int error = sleepq_block(pWait->iTimeout, pWait->fInterruptible);
    if (error == EWOULDBLOCK) {
        if (!pWait->fIndefinite) {
            pWait->fTimedOut = true;
        }
    } else if (error == ERESTART) {
        if (pWait->fInterruptible) {
            pWait->fInterrupted = true;
        } else if (!pWait->fIndefinite) {
            rtR0SemBsdWaitUpdateTimeout(pWait);
            if (pWait->iTimeout == 0) {
                pWait->fTimedOut = true;
            }
        }
    } else if (error == EINTR) {
        if (pWait->fInterruptible) {
            pWait->fInterrupted = true;
        } else if (!pWait->fIndefinite) {
            rtR0SemBsdWaitUpdateTimeout(pWait);
            if (pWait->iTimeout == 0) {
                pWait->fTimedOut = true;
            }
        }
    } else if (error) {
        AssertMsgFailed(("sleepq_block -> %d\n", error));
    }
}


/**
 * Checks if a NetBSD wait was interrupted.
 *
 * @returns true / false
 * @param   pWait               The wait structure.
 * @remarks This shall be called before the first rtR0SemLnxWaitDoIt().
 */
DECLINLINE(bool) rtR0SemBsdWaitWasInterrupted(PRTR0SEMBSDSLEEP pWait)
{
    return pWait->fInterrupted;
}


/**
 * Checks if a NetBSD wait has timed out.
 *
 * @returns true / false
 * @param   pWait               The wait structure.
 */
DECLINLINE(bool) rtR0SemBsdWaitHasTimedOut(PRTR0SEMBSDSLEEP pWait)
{
    return pWait->fTimedOut;
}


/**
 * Deletes a NetBSD wait.
 *
 * @param   pWait               The wait structure.
 */
DECLINLINE(void) rtR0SemBsdWaitDelete(PRTR0SEMBSDSLEEP pWait)
{
    if (pWait->sq_lock != NULL) {
        mutex_spin_exit(pWait->sq_lock);
        pWait->sq = NULL;
        pWait->sq_lock = NULL;
    }
}


/**
 * Signals the wait channel.
 *
 * @param  pvWaitChan           The opaque wait channel handle.
 */
DECLINLINE(void) rtR0SemBsdSignal(void *pvWaitChan)
{
    kmutex_t *mp;
    sleepq_t *sq = sleeptab_lookup(&sleeptab, pvWaitChan, &mp);
    sleepq_wake(sq, pvWaitChan, 1, mp);
}

/**
 * Wakes up all waiters on the wait channel.
 *
 * @param  pvWaitChan           The opaque wait channel handle.
 */
DECLINLINE(void) rtR0SemBsdBroadcast(void *pvWaitChan)
{
    kmutex_t *mp;
    sleepq_t *sq = sleeptab_lookup(&sleeptab, pvWaitChan, &mp);
    sleepq_wake(sq, pvWaitChan, ~0u, mp);
}

/**
 * Gets the max resolution of the timeout machinery.
 *
 * @returns Resolution specified in nanoseconds.
 */
DECLINLINE(uint32_t) rtR0SemBsdWaitGetResolution(void)
{
    return 1000000000 / hz; /* ns */
}

#endif /* ___r0drv_netbsd_sleepqueue_r0drv_netbsd_h */

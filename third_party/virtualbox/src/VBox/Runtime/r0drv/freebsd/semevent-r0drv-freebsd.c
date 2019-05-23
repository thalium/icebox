/* $Id: semevent-r0drv-freebsd.c $ */
/** @file
 * IPRT - Single Release Event Semaphores, Ring-0 Driver, FreeBSD.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define RTSEMEVENT_WITHOUT_REMAPPING
#include "the-freebsd-kernel.h"
#include "internal/iprt.h"
#include <iprt/semaphore.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/lockvalidator.h>
#include <iprt/mem.h>

#include "sleepqueue-r0drv-freebsd.h"
#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * FreeBSD event semaphore.
 */
typedef struct RTSEMEVENTINTERNAL
{
    /** Magic value (RTSEMEVENT_MAGIC). */
    uint32_t volatile   u32Magic;
    /** The object status - !0 when signaled and 0 when reset. */
    uint32_t volatile   fState;
    /** Reference counter. */
    uint32_t volatile   cRefs;
} RTSEMEVENTINTERNAL, *PRTSEMEVENTINTERNAL;


RTDECL(int)  RTSemEventCreate(PRTSEMEVENT phEventSem)
{
    return RTSemEventCreateEx(phEventSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, NULL);
}


RTDECL(int)  RTSemEventCreateEx(PRTSEMEVENT phEventSem, uint32_t fFlags, RTLOCKVALCLASS hClass, const char *pszNameFmt, ...)
{
    AssertCompile(sizeof(RTSEMEVENTINTERNAL) > sizeof(void *));
    AssertReturn(!(fFlags & ~(RTSEMEVENT_FLAGS_NO_LOCK_VAL | RTSEMEVENT_FLAGS_BOOTSTRAP_HACK)), VERR_INVALID_PARAMETER);
    Assert(!(fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK) || (fFlags & RTSEMEVENT_FLAGS_NO_LOCK_VAL));
    AssertPtrReturn(phEventSem, VERR_INVALID_POINTER);

    PRTSEMEVENTINTERNAL pThis = (PRTSEMEVENTINTERNAL)RTMemAllocZ(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;

    pThis->u32Magic  = RTSEMEVENT_MAGIC;
    pThis->cRefs     = 1;
    pThis->fState    = 0;

    *phEventSem = pThis;
    return VINF_SUCCESS;
}


/**
 * Retains a reference to the event semaphore.
 *
 * @param   pThis       The event semaphore.
 */
DECLINLINE(void) rtR0SemEventBsdRetain(PRTSEMEVENTINTERNAL pThis)
{
    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    Assert(cRefs < 100000); NOREF(cRefs);
}


/**
 * Releases a reference to the event semaphore.
 *
 * @param   pThis       The event semaphore.
 */
DECLINLINE(void) rtR0SemEventBsdRelease(PRTSEMEVENTINTERNAL pThis)
{
    if (RT_UNLIKELY(ASMAtomicDecU32(&pThis->cRefs) == 0))
        RTMemFree(pThis);
}


RTDECL(int)  RTSemEventDestroy(RTSEMEVENT hEventSem)
{
    /*
     * Validate input.
     */
    PRTSEMEVENTINTERNAL pThis = hEventSem;
    if (pThis == NIL_RTSEMEVENT)
        return VINF_SUCCESS;
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("pThis->u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);
    Assert(pThis->cRefs > 0);

    /*
     * Invalidate it and signal the object just in case.
     */
    ASMAtomicWriteU32(&pThis->u32Magic, ~RTSEMEVENT_MAGIC);
    ASMAtomicWriteU32(&pThis->fState, 0);
    rtR0SemBsdBroadcast(pThis);
    rtR0SemEventBsdRelease(pThis);
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemEventSignal(RTSEMEVENT hEventSem)
{
    /*
     * Validate input.
     */
    PRTSEMEVENTINTERNAL pThis = (PRTSEMEVENTINTERNAL)hEventSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("pThis->u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);
    rtR0SemEventBsdRetain(pThis);

    /*
     * Signal the event object.
     */
    ASMAtomicWriteU32(&pThis->fState, 1);
    rtR0SemBsdSignal(pThis);
    rtR0SemEventBsdRelease(pThis);
    return VINF_SUCCESS;
}

/**
 * Worker for RTSemEventWaitEx and RTSemEventWaitExDebug.
 *
 * @returns VBox status code.
 * @param   pThis           The event semaphore.
 * @param   fFlags          See RTSemEventWaitEx.
 * @param   uTimeout        See RTSemEventWaitEx.
 * @param   pSrcPos         The source code position of the wait.
 */
static int rtR0SemEventWait(PRTSEMEVENTINTERNAL pThis, uint32_t fFlags, uint64_t uTimeout,
                            PCRTLOCKVALSRCPOS pSrcPos)
{
    int rc;

    /*
     * Validate the input.
     */
    AssertPtrReturn(pThis, VERR_INVALID_PARAMETER);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("%p u32Magic=%RX32\n", pThis, pThis->u32Magic), VERR_INVALID_PARAMETER);
    AssertReturn(RTSEMWAIT_FLAGS_ARE_VALID(fFlags), VERR_INVALID_PARAMETER);
    rtR0SemEventBsdRetain(pThis);

    /*
     * Try grab the event without setting up the wait.
     */
    if (ASMAtomicCmpXchgU32(&pThis->fState, 0, 1))
        rc = VINF_SUCCESS;
    else
    {
        /*
         * We have to wait.
         */
        RTR0SEMBSDSLEEP Wait;
        rc = rtR0SemBsdWaitInit(&Wait, fFlags, uTimeout, pThis);
        if (RT_SUCCESS(rc))
        {
            for (;;)
            {
                /* The destruction test. */
                if (RT_UNLIKELY(pThis->u32Magic != RTSEMEVENT_MAGIC))
                    rc = VERR_SEM_DESTROYED;
                else
                {
                    rtR0SemBsdWaitPrepare(&Wait);

                    /* Check the exit conditions. */
                    if (RT_UNLIKELY(pThis->u32Magic != RTSEMEVENT_MAGIC))
                        rc = VERR_SEM_DESTROYED;
                    else if (ASMAtomicCmpXchgU32(&pThis->fState, 0, 1))
                        rc = VINF_SUCCESS;
                    else if (rtR0SemBsdWaitHasTimedOut(&Wait))
                        rc = VERR_TIMEOUT;
                    else if (rtR0SemBsdWaitWasInterrupted(&Wait))
                        rc = VERR_INTERRUPTED;
                    else
                    {
                        /* Do the wait and then recheck the conditions. */
                        rtR0SemBsdWaitDoIt(&Wait);
                        continue;
                    }
                }
                break;
            }

            rtR0SemBsdWaitDelete(&Wait);
        }
    }

    rtR0SemEventBsdRelease(pThis);
    return rc;
}


RTDECL(int)  RTSemEventWaitEx(RTSEMEVENT hEventSem, uint32_t fFlags, uint64_t uTimeout)
{
#ifndef RTSEMEVENT_STRICT
    return rtR0SemEventWait(hEventSem, fFlags, uTimeout, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtR0SemEventWait(hEventSem, fFlags, uTimeout, &SrcPos);
#endif
}
RT_EXPORT_SYMBOL(RTSemEventWaitEx);


RTDECL(int)  RTSemEventWaitExDebug(RTSEMEVENT hEventSem, uint32_t fFlags, uint64_t uTimeout,
                                   RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtR0SemEventWait(hEventSem, fFlags, uTimeout, &SrcPos);
}
RT_EXPORT_SYMBOL(RTSemEventWaitExDebug);


RTDECL(uint32_t) RTSemEventGetResolution(void)
{
    return 1000000000 / hz;
}


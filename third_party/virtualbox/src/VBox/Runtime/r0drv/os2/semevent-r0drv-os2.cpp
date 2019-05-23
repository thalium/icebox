/* $Id: semevent-r0drv-os2.cpp $ */
/** @file
 * IPRT - Single Release Event Semaphores, Ring-0 Driver, OS/2.
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
#include "the-os2-kernel.h"
#include "internal/iprt.h"

#include <iprt/semaphore.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/lockvalidator.h>

#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * OS/2 event semaphore.
 */
typedef struct RTSEMEVENTINTERNAL
{
    /** Magic value (RTSEMEVENT_MAGIC). */
    uint32_t volatile   u32Magic;
    /** The number of waiting threads. */
    uint32_t volatile   cWaiters;
    /** Set if the event object is signaled. */
    uint8_t volatile    fSignaled;
    /** The number of threads in the process of waking up. */
    uint32_t volatile   cWaking;
    /** The OS/2 spinlock protecting this structure. */
    SpinLock_t          Spinlock;
} RTSEMEVENTINTERNAL, *PRTSEMEVENTINTERNAL;


RTDECL(int)  RTSemEventCreate(PRTSEMEVENT phEventSem)
{
    return RTSemEventCreateEx(phEventSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, NULL);
}


RTDECL(int)  RTSemEventCreateEx(PRTSEMEVENT phEventSem, uint32_t fFlags, RTLOCKVALCLASS hClass, const char *pszNameFmt, ...)
{
    AssertReturn(!(fFlags & ~(RTSEMEVENT_FLAGS_NO_LOCK_VAL | RTSEMEVENT_FLAGS_BOOTSTRAP_HACK)), VERR_INVALID_PARAMETER);
    Assert(!(fFlags & RTSEMEVENT_FLAGS_BOOTSTRAP_HACK) || (fFlags & RTSEMEVENT_FLAGS_NO_LOCK_VAL));
    AssertCompile(sizeof(RTSEMEVENTINTERNAL) > sizeof(void *));
    AssertPtrReturn(phEventSem, VERR_INVALID_POINTER);

    PRTSEMEVENTINTERNAL pThis = (PRTSEMEVENTINTERNAL)RTMemAlloc(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;

    pThis->u32Magic = RTSEMEVENT_MAGIC;
    pThis->cWaiters = 0;
    pThis->cWaking = 0;
    pThis->fSignaled = 0;
    KernAllocSpinLock(&pThis->Spinlock);

    *phEventSem = pThis;
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemEventDestroy(RTSEMEVENT hEventSem)
{
    PRTSEMEVENTINTERNAL pThis = hEventSem;
    if (pThis == NIL_RTSEMEVENT)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);

    KernAcquireSpinLock(&pThis->Spinlock);
    ASMAtomicIncU32(&pThis->u32Magic); /* make the handle invalid */
    if (pThis->cWaiters > 0)
    {
        /* abort waiting thread, last man cleans up. */
        ASMAtomicXchgU32(&pThis->cWaking, pThis->cWaking + pThis->cWaiters);
        ULONG cThreads;
        KernWakeup((ULONG)pThis, WAKEUP_DATA | WAKEUP_BOOST, &cThreads, (ULONG)VERR_SEM_DESTROYED);
        KernReleaseSpinLock(&pThis->Spinlock);
    }
    else if (pThis->cWaking)
        /* the last waking thread is gonna do the cleanup */
        KernReleaseSpinLock(&pThis->Spinlock);
    else
    {
        KernReleaseSpinLock(&pThis->Spinlock);
        KernFreeSpinLock(&pThis->Spinlock);
        RTMemFree(pThis);
    }

    return VINF_SUCCESS;
}


RTDECL(int)  RTSemEventSignal(RTSEMEVENT hEventSem)
{
    PRTSEMEVENTINTERNAL pThis = (PRTSEMEVENTINTERNAL)hEventSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);

    KernAcquireSpinLock(&pThis->Spinlock);

    if (pThis->cWaiters > 0)
    {
        ASMAtomicDecU32(&pThis->cWaiters);
        ASMAtomicIncU32(&pThis->cWaking);
        ULONG cThreads;
        KernWakeup((ULONG)pThis, WAKEUP_DATA | WAKEUP_ONE, &cThreads, VINF_SUCCESS);
        if (RT_UNLIKELY(!cThreads))
        {
            /* shouldn't ever happen on OS/2 */
            ASMAtomicXchgU8(&pThis->fSignaled, true);
            ASMAtomicDecU32(&pThis->cWaking);
            ASMAtomicIncU32(&pThis->cWaiters);
        }
    }
    else
        ASMAtomicXchgU8(&pThis->fSignaled, true);

    KernReleaseSpinLock(&pThis->Spinlock);
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
static int rtR0SemEventOs2Wait(PRTSEMEVENTINTERNAL pThis, uint32_t fFlags, uint64_t uTimeout,
                               PCRTLOCKVALSRCPOS pSrcPos)
{
    /*
     * Validate and convert input.
     */
    if (!pThis)
        return VERR_INVALID_HANDLE;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMEVENT_MAGIC, ("u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);
    AssertReturn(RTSEMWAIT_FLAGS_ARE_VALID(fFlags), VERR_INVALID_PARAMETER);

    ULONG cMsTimeout = rtR0SemWaitOs2ConvertTimeout(fFlags, uTimeout);
    ULONG fBlock     = BLOCK_SPINLOCK;
    if (!(fFlags & RTSEMWAIT_FLAGS_INTERRUPTIBLE))
        fBlock |= BLOCK_UNINTERRUPTABLE;

    /*
     * Do the job.
     */
    KernAcquireSpinLock(&pThis->Spinlock);

    int rc;
    if (pThis->fSignaled)
    {
        Assert(!pThis->cWaiters);
        ASMAtomicXchgU8(&pThis->fSignaled, false);
        rc = VINF_SUCCESS;
    }
    else
    {
        ASMAtomicIncU32(&pThis->cWaiters);

        ULONG ulData = (ULONG)VERR_INTERNAL_ERROR;
        rc = KernBlock((ULONG)pThis, cMsTimeout, fBlock,
                       &pThis->Spinlock,
                       &ulData);
        switch (rc)
        {
            case NO_ERROR:
                rc = (int)ulData;
                Assert(rc == VINF_SUCCESS || rc == VERR_SEM_DESTROYED);
                Assert(pThis->cWaking > 0);
                if (    !ASMAtomicDecU32(&pThis->cWaking)
                    &&  pThis->u32Magic != RTSEMEVENT_MAGIC)
                {
                    /* The event was destroyed (ulData == VINF_SUCCESS if it was after we awoke), as
                       the last thread do the cleanup. */
                    KernReleaseSpinLock(&pThis->Spinlock);
                    KernFreeSpinLock(&pThis->Spinlock);
                    RTMemFree(pThis);
                    return rc;
                }
                break;

            case ERROR_TIMEOUT:
                Assert(cMsTimeout != SEM_INDEFINITE_WAIT);
                ASMAtomicDecU32(&pThis->cWaiters);
                rc = VERR_TIMEOUT;
                break;

            case ERROR_INTERRUPT:
                Assert(fFlags & RTSEMWAIT_FLAGS_INTERRUPTIBLE);
                ASMAtomicDecU32(&pThis->cWaiters);
                rc = VERR_INTERRUPTED;
                break;

            default:
                AssertMsgFailed(("rc=%d\n", rc));
                rc = VERR_GENERAL_FAILURE;
                break;
        }
    }

    KernReleaseSpinLock(&pThis->Spinlock);
    return rc;
}


RTDECL(int)  RTSemEventWaitEx(RTSEMEVENT hEventSem, uint32_t fFlags, uint64_t uTimeout)
{
#ifndef RTSEMEVENT_STRICT
    return rtR0SemEventOs2Wait(hEventSem, fFlags, uTimeout, NULL);
#else
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_NORMAL_API();
    return rtR0SemEventOs2Wait(hEventSem, fFlags, uTimeout, &SrcPos);
#endif
}


RTDECL(int)  RTSemEventWaitExDebug(RTSEMEVENT hEventSem, uint32_t fFlags, uint64_t uTimeout,
                                   RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    RTLOCKVALSRCPOS SrcPos = RTLOCKVALSRCPOS_INIT_DEBUG_API();
    return rtR0SemEventOs2Wait(hEventSem, fFlags, uTimeout, &SrcPos);
}


RTDECL(uint32_t) RTSemEventGetResolution(void)
{
    return 32000000; /* 32ms */
}


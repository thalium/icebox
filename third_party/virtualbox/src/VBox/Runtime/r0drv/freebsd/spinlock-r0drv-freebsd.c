/* $Id: spinlock-r0drv-freebsd.c $ */
/** @file
 * IPRT - Spinlocks, Ring-0 Driver, FreeBSD.
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
#include "the-freebsd-kernel.h"
#include "internal/iprt.h"

#include <iprt/spinlock.h>
#include <iprt/err.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/thread.h>
#include <iprt/mp.h>

#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Wrapper for the struct mtx type.
 */
typedef struct RTSPINLOCKINTERNAL
{
    /** Spinlock magic value (RTSPINLOCK_MAGIC). */
    uint32_t volatile   u32Magic;
    /** The spinlock. */
    uint32_t volatile   fLocked;
    /** Saved interrupt flag. */
    uint32_t volatile   fIntSaved;
    /** The spinlock creation flags. */
    uint32_t            fFlags;
#ifdef RT_MORE_STRICT
    /** The idAssertCpu variable before acquring the lock for asserting after
     *  releasing the spinlock. */
    RTCPUID volatile    idAssertCpu;
    /** The CPU that owns the lock. */
    RTCPUID volatile    idCpuOwner;
#endif
} RTSPINLOCKINTERNAL, *PRTSPINLOCKINTERNAL;


RTDECL(int)  RTSpinlockCreate(PRTSPINLOCK pSpinlock, uint32_t fFlags, const char *pszName)
{
    RT_ASSERT_PREEMPTIBLE();
    AssertReturn(fFlags == RTSPINLOCK_FLAGS_INTERRUPT_SAFE || fFlags == RTSPINLOCK_FLAGS_INTERRUPT_UNSAFE, VERR_INVALID_PARAMETER);

    /*
     * Allocate.
     */
    AssertCompile(sizeof(RTSPINLOCKINTERNAL) > sizeof(void *));
    PRTSPINLOCKINTERNAL pThis = (PRTSPINLOCKINTERNAL)RTMemAllocZ(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;

    /*
     * Initialize & return.
     */
    pThis->u32Magic  = RTSPINLOCK_MAGIC;
    pThis->fLocked   = 0;
    pThis->fFlags    = fFlags;
    pThis->fIntSaved = 0;

    *pSpinlock = pThis;
    return VINF_SUCCESS;
}


RTDECL(int)  RTSpinlockDestroy(RTSPINLOCK Spinlock)
{
    /*
     * Validate input.
     */
    RT_ASSERT_INTS_ON();
    PRTSPINLOCKINTERNAL pThis = (PRTSPINLOCKINTERNAL)Spinlock;
    if (!pThis)
        return VERR_INVALID_PARAMETER;
    AssertMsgReturn(pThis->u32Magic == RTSPINLOCK_MAGIC,
                    ("Invalid spinlock %p magic=%#x\n", pThis, pThis->u32Magic),
                    VERR_INVALID_PARAMETER);

    /*
     * Make the lock invalid and release the memory.
     */
    ASMAtomicIncU32(&pThis->u32Magic);
    RTMemFree(pThis);
    return VINF_SUCCESS;
}


RTDECL(void) RTSpinlockAcquire(RTSPINLOCK Spinlock)
{
    PRTSPINLOCKINTERNAL pThis = (PRTSPINLOCKINTERNAL)Spinlock;
    RT_ASSERT_PREEMPT_CPUID_VAR();
    AssertPtr(pThis);
    Assert(pThis->u32Magic == RTSPINLOCK_MAGIC);

    if (pThis->fFlags & RTSPINLOCK_FLAGS_INTERRUPT_SAFE)
    {
        for (;;)
        {
            uint32_t fIntSaved = ASMIntDisableFlags();
            critical_enter();

            int c = 50;
            for (;;)
            {
                if (ASMAtomicCmpXchgU32(&pThis->fLocked, 1, 0))
                {
                    RT_ASSERT_PREEMPT_CPUID_SPIN_ACQUIRED(pThis);
                    pThis->fIntSaved = fIntSaved;
                    return;
                }
                if (--c <= 0)
                    break;
                cpu_spinwait();
            }

            /* Enable interrupts while we sleep. */
            ASMSetFlags(fIntSaved);
            critical_exit();
            DELAY(1);
        }
    }
    else
    {
        for (;;)
        {
            critical_enter();

            int c = 50;
            for (;;)
            {
                if (ASMAtomicCmpXchgU32(&pThis->fLocked, 1, 0))
                {
                    RT_ASSERT_PREEMPT_CPUID_SPIN_ACQUIRED(pThis);
                    return;
                }
                if (--c <= 0)
                    break;
                cpu_spinwait();
            }

            critical_exit();
            DELAY(1);
        }
    }
}


RTDECL(void) RTSpinlockRelease(RTSPINLOCK Spinlock)
{
    PRTSPINLOCKINTERNAL pThis = (PRTSPINLOCKINTERNAL)Spinlock;
    RT_ASSERT_PREEMPT_CPUID_SPIN_RELEASE_VARS();

    AssertPtr(pThis);
    Assert(pThis->u32Magic == RTSPINLOCK_MAGIC);
    RT_ASSERT_PREEMPT_CPUID_SPIN_RELEASE(pThis);

    if (pThis->fFlags & RTSPINLOCK_FLAGS_INTERRUPT_SAFE)
    {
        uint32_t fIntSaved = pThis->fIntSaved;
        pThis->fIntSaved = 0;
        if (ASMAtomicCmpXchgU32(&pThis->fLocked, 0, 1))
            ASMSetFlags(fIntSaved);
        else
            AssertMsgFailed(("Spinlock %p was not locked!\n", pThis));
    }
    else
    {
        if (!ASMAtomicCmpXchgU32(&pThis->fLocked, 0, 1))
            AssertMsgFailed(("Spinlock %p was not locked!\n", pThis));
    }

    critical_exit();
}


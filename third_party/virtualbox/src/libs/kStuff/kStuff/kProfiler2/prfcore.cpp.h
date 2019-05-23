/* $Id: prfcore.cpp.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kProfiler Mark 2 - Core Code Template.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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


/**
 * Gets a function, create a new one if necessary.
 */
static KPRF_TYPE(P,FUNC) KPRF_NAME(GetFunction)(KPRF_TYPE(P,HDR) pHdr, KPRF_TYPE(,UPTR) uPC)
{
    /*
     * Perform a binary search of the function lookup table.
     */
    KPRF_TYPE(P,FUNC) paFunctions = KPRF_OFF2PTR(P,FUNC, pHdr->offFunctions, pHdr);

    KPRF_FUNCS_READ_LOCK();
    KI32 iStart = 0;
    KI32 iLast  = pHdr->cFunctions - 1;
    KI32 i      = iLast / 2;
    for (;;)
    {
        KU32 iFunction = pHdr->aiFunctions[i];
        KPRF_TYPE(,IPTR) iDiff = uPC - paFunctions[iFunction].uEntryPtr;
        if (!iDiff)
        {
            KPRF_FUNCS_READ_UNLOCK();
            return &paFunctions[iFunction];
        }
        if (iLast == iStart)
            break;
        if (iDiff < 0)
            iLast = i - 1;
        else
            iStart = i + 1;
        if (iLast < iStart)
            break;
        i = iStart + (iLast - iStart) / 2;
    }
    KPRF_FUNCS_READ_UNLOCK();

    /*
     * It wasn't found, try add it.
     */
    if (pHdr->cFunctions < pHdr->cMaxFunctions)
        return KPRF_NAME(NewFunction)(pHdr, uPC);
    return NULL;
}


/**
 * Unwind one frame.
 */
static KU64* KPRF_NAME(UnwindOne)(KPRF_TYPE(P,HDR) pHdr, KPRF_TYPE(P,STACK) pStack, KPRF_TYPE(,UPTR) uPC, KU64 TS)
{
    /*
     * Pop off the frame and update the frame below / thread.
     */
    KPRF_TYPE(P,FRAME) pFrame = &pStack->aFrames[--pStack->cFrames];
    KU64 *pCurOverheadTicks;
    if (pStack->cFrames)
    {
        KPRF_TYPE(P,FRAME) pTopFrame = pFrame - 1;
        pTopFrame->OverheadTicks += pFrame->OverheadTicks + pFrame->CurOverheadTicks;
        pTopFrame->SleepTicks    += pFrame->SleepTicks;
        pTopFrame->OnTopOfStackStart = TS;
        pTopFrame->CurOverheadTicks  = 0;

        pCurOverheadTicks = &pTopFrame->CurOverheadTicks;
    }
    else
    {
        KPRF_TYPE(P,THREAD) pThread = KPRF_OFF2PTR(P,THREAD, pStack->offThread, pHdr);
        pThread->ProfiledTicks  += TS - pFrame->OnStackStart - pFrame->CurOverheadTicks - pFrame->OverheadTicks - pFrame->SleepTicks;
        pThread->OverheadTicks  += pFrame->OverheadTicks + pFrame->CurOverheadTicks;
        pThread->SleepTicks     += pFrame->SleepTicks;

        pCurOverheadTicks = &pThread->OverheadTicks;
    }

    /*
     * Update the function (if any).
     */
    if (pFrame->offFunction)
    {
        KPRF_TYPE(P,FUNC) pFunc = KPRF_OFF2PTR(P,FUNC, pFrame->offFunction, pHdr);

        /* Time on stack */
        KU64 Ticks = TS - pFrame->OnStackStart;
        Ticks -= pFrame->OverheadTicks + pFrame->CurOverheadTicks + pFrame->SleepTicks;
/** @todo adjust overhead */
KPRF_ASSERT(!(Ticks >> 63));
        if (pFunc->OnStack.MinTicks > Ticks)
            KPRF_ATOMIC_SET64(&pFunc->OnStack.MinTicks, Ticks);
        if (pFunc->OnStack.MaxTicks < Ticks)
            KPRF_ATOMIC_SET64(&pFunc->OnStack.MaxTicks, Ticks);
        KPRF_ATOMIC_ADD64(&pFunc->OnStack.SumTicks, Ticks);

        /* Time on top of stack */
        Ticks = TS - pFrame->OnTopOfStackStart;
        Ticks -= pFrame->CurOverheadTicks;
        Ticks += pFrame->OnTopOfStackTicks;
/** @todo adjust overhead */
KPRF_ASSERT(!(Ticks >> 63));
        if (pFunc->OnTopOfStack.MinTicks > Ticks)
            KPRF_ATOMIC_SET64(&pFunc->OnTopOfStack.MinTicks, Ticks);
        if (pFunc->OnTopOfStack.MaxTicks < Ticks)
            KPRF_ATOMIC_SET64(&pFunc->OnTopOfStack.MaxTicks, Ticks);
        KPRF_ATOMIC_ADD64(&pFunc->OnTopOfStack.SumTicks, Ticks);

        /* calls */
        if (pFrame->cCalls)
            KPRF_ATOMIC_ADD64(&pFunc->cCalls, pFrame->cCalls);
    }

    return pCurOverheadTicks;
}


/**
 * Unwinds the stack.
 *
 * On MSC+AMD64 we have to be very very careful here, because the uFramePtr cannot be trusted.
 */
static KU64* KPRF_NAME(UnwindInt)(KPRF_TYPE(P,HDR) pHdr, KPRF_TYPE(P,STACK) pStack, KPRF_TYPE(,UPTR) uPC, KPRF_TYPE(,UPTR) uFramePtr, KU64 TS)
{
    /** @todo need to deal with alternative stacks! */

    /*
     * Pop the stack until we're down below the current frame (uFramePtr).
     */
    KI32 iFrame = pStack->cFrames - 1;
    KPRF_TYPE(P,FRAME) pFrame = &pStack->aFrames[iFrame];

    /* the most frequent case first. */
#if K_OS == K_OS_WINDOWS && K_ARCH == K_ARCH_AMD64
    if (    uFramePtr == pFrame->uFramePtr
        ||  (   pFrame->uFramePtr < uFramePtr
             && iFrame > 0
             && pFrame[-1].uFramePtr > uFramePtr))
        return KPRF_NAME(UnwindOne)(pHdr, pStack, uPC, TS);
#else
    if (uFramePtr == pFrame->uFramePtr)
        return KPRF_NAME(UnwindOne)(pHdr, pStack, uPC, TS);
#endif

    /* none?  */
    if (pFrame->uFramePtr > uFramePtr)
        return &pFrame->CurOverheadTicks;

    /* one or more, possibly all */
    KU64 *pCurOverheadTicks = KPRF_NAME(UnwindOne)(pHdr, pStack, uPC, TS);
    pFrame--;
    if (    iFrame > 0
#if K_OS == K_OS_WINDOWS && K_ARCH == K_ARCH_AMD64
        &&  pFrame->uFramePtr <= uFramePtr
        &&  pFrame[-1].uFramePtr > uFramePtr)
#else
        &&  pFrame->uFramePtr <= uFramePtr)
#endif
    {
        KPRF_TYPE(P,THREAD) pThread = KPRF_OFF2PTR(P,THREAD, pStack->offThread, pHdr);
        pThread->cUnwinds++;        /* (This is the reason for what looks like a bad loop unrolling.) */

        pCurOverheadTicks = KPRF_NAME(UnwindOne)(pHdr, pStack, uPC, TS);
        iFrame -= 2;
        pFrame--;
#if K_OS == K_OS_WINDOWS && K_ARCH == K_ARCH_AMD64
        while (     iFrame > 0
               &&   pFrame->uFramePtr <= uFramePtr
               &&   pFrame[-1].uFramePtr > uFramePtr)
#else
        while (     iFrame >= 0
               &&   pFrame->uFramePtr <= uFramePtr)
#endif
        {
            pCurOverheadTicks = KPRF_NAME(UnwindOne)(pHdr, pStack, uPC, TS);
            iFrame--;
            pFrame--;
        }
    }

    return pCurOverheadTicks;
}



/**
 * Enter function.
 *
 * @returns Where to account overhead.
 * @returns NULL if profiling is inactive.
 *
 * @param   uPC         The program counter register. (not relative)
 * @param   uFramePtr   The stack frame address. This must match the one passed to kPrfLeave. (not relative)
 * @param   TS          The timestamp when we entered into the profiler.
 *                      This must not be modified touched!
 *
 * @internal ?
 */
KPRF_DECL_FUNC(KU64 *, Enter)(KPRF_TYPE(,UPTR) uPC, KPRF_TYPE(,UPTR) uFramePtr, const KU64 TS)
{
    /*
     * Is profiling active ?
     */
    if (!KPRF_IS_ACTIVE())
        return NULL;

    /*
     * Get the header and adjust input addresses.
     */
    KPRF_TYPE(P,HDR)    pHdr = KPRF_GET_HDR();
    if (!pHdr)
        return NULL;
    const KPRF_TYPE(,UPTR) uBasePtr = pHdr->uBasePtr;
    if (uBasePtr)
    {
        uFramePtr -= uBasePtr;
        uPC       -= uBasePtr;
    }

    /*
     * Get the current thread. Reject unknown, inactive (in whatever way),
     * and thread which has performed a stack switch.
     */
    KPRF_TYPE(P,THREAD) pThread = KPRF_GET_THREAD();
    if (!pThread)
        return NULL;
    KPRF_TYPE(,THREADSTATE) enmThreadState = pThread->enmState;
    if (    enmThreadState != KPRF_TYPE(,THREADSTATE_ACTIVE)
        &&  enmThreadState != KPRF_TYPE(,THREADSTATE_OVERFLOWED)
        )
        return NULL;
    if (pThread->uStackBasePtr < uFramePtr)                                         /* ASSUMES stack direction */
    {
        pThread->cStackSwitchRejects++;
        return NULL;
    }
    pThread->enmState = KPRF_TYPE(,THREADSTATE_SUSPENDED);


    /*
     * Update the thread statistics.
     */
    pThread->cCalls++;
    KPRF_TYPE(,UPTR) cbStack = pThread->uStackBasePtr - uFramePtr;                 /* ASSUMES stack direction */
    if (pThread->cbMaxStack < cbStack)
        pThread->cbMaxStack = cbStack;

    /*
     * Check if an longjmp or throw has taken place.
     * This check will not work if a stack switch has taken place (can fix that later).
     */
    KPRF_TYPE(P,STACK) pStack = KPRF_OFF2PTR(P,STACK, pThread->offStack, pHdr);
    KU32 iFrame = pStack->cFrames;
    KPRF_TYPE(P,FRAME) pFrame = &pStack->aFrames[iFrame];
    if (    iFrame
#if K_OS == K_OS_WINDOWS && K_ARCH == K_ARCH_AMD64
        &&  0) /* don't bother her yet because of _penter/_pexit frame problems. */
#else
        &&  pThread->uStackBasePtr >= uFramePtr                                     /* ASSUMES stack direction */
        &&  pFrame[-1].uFramePtr + (KPRF_BITS - 8) / 8 < uFramePtr)                 /* ASSUMES stack direction */
#endif
    {
        KPRF_NAME(UnwindInt)(pHdr, pStack, uPC, uFramePtr, TS);
        iFrame = pStack->cFrames;
    }

    /*
     * Allocate a new stack frame.
     */
    if (iFrame >= pHdr->cMaxStackFrames)
    {
        /* overflow */
        pThread->enmState = KPRF_TYPE(,THREADSTATE_OVERFLOWED);
        pThread->cOverflows += enmThreadState != KPRF_TYPE(,THREADSTATE_OVERFLOWED);
        return &pStack->aFrames[iFrame - 1].CurOverheadTicks;
    }
    pStack->cFrames++;

    /*
     * Update the old top frame if any.
     */
    if (iFrame)
    {
        KPRF_TYPE(P,FRAME) pOldFrame = pFrame - 1;
        pOldFrame->OnTopOfStackTicks += TS - pOldFrame->OnTopOfStackStart;
        pOldFrame->cCalls++;
    }

    /*
     * Fill in the new frame.
     */
    pFrame->CurOverheadTicks    = 0;
    pFrame->OverheadTicks       = 0;
    pFrame->SleepTicks          = 0;
    pFrame->OnStackStart        = TS;
    pFrame->OnTopOfStackStart   = TS;
    pFrame->OnTopOfStackTicks   = 0;
    pFrame->cCalls              = 0;
    pFrame->uFramePtr           = uFramePtr;

    /*
     * Find the relevant function.
     */
    KPRF_TYPE(P,FUNC) pFunc = KPRF_NAME(GetFunction)(pHdr, uPC);
    if (pFunc)
    {
        pFrame->offFunction = KPRF_PTR2OFF(pFunc, pHdr);
        pFunc->cOnStack++;
    }
    else
        pFrame->offFunction = 0;

    /*
     * Nearly done, We only have to reactivate the thread and account overhead.
     * The latter is delegated to the caller.
     */
    pThread->enmState = KPRF_TYPE(,THREADSTATE_ACTIVE);
    return &pFrame->CurOverheadTicks;
}


/**
 * Leave function.
 *
 * @returns Where to account overhead.
 * @returns NULL if profiling is inactive.
 *
 * @param   uPC         The program counter register.
 * @param   uFramePtr   The stack frame address. This must match the one passed to kPrfEnter.
 * @param   TS          The timestamp when we entered into the profiler.
 *                      This must not be modified because the caller could be using it!
 * @internal
 */
KPRF_DECL_FUNC(KU64 *, Leave)(KPRF_TYPE(,UPTR) uPC, KPRF_TYPE(,UPTR) uFramePtr, const KU64 TS)
{
    /*
     * Is profiling active ?
     */
    if (!KPRF_IS_ACTIVE())
        return NULL;

    /*
     * Get the header and adjust input addresses.
     */
    KPRF_TYPE(P,HDR)    pHdr = KPRF_GET_HDR();
    if (!pHdr)
        return NULL;
    const KPRF_TYPE(,UPTR) uBasePtr = pHdr->uBasePtr;
    if (uBasePtr)
    {
        uFramePtr -= uBasePtr;
        uPC       -= uBasePtr;
    }

    /*
     * Get the current thread and suspend profiling of the thread until we leave this function.
     * Also reject threads which aren't active in some way.
     */
    KPRF_TYPE(P,THREAD) pThread = KPRF_GET_THREAD();
    if (!pThread)
        return NULL;
    KPRF_TYPE(,THREADSTATE) enmThreadState = pThread->enmState;
    if (    enmThreadState != KPRF_TYPE(,THREADSTATE_ACTIVE)
        &&  enmThreadState != KPRF_TYPE(,THREADSTATE_OVERFLOWED)
        )
        return NULL;
    KPRF_TYPE(P,STACK) pStack = KPRF_OFF2PTR(P,STACK, pThread->offStack, pHdr);
    if (!pStack->cFrames)
        return NULL;
    pThread->enmState = KPRF_TYPE(,THREADSTATE_SUSPENDED);

    /*
     * Unwind the stack down to and including the entry indicated by uFramePtr.
     * Leave it to the caller to update the overhead.
     */
    KU64 *pCurOverheadTicks = KPRF_NAME(UnwindInt)(pHdr, pStack, uPC, uFramePtr, TS);

    pThread->enmState = enmThreadState;
    return pCurOverheadTicks;
}


/**
 * Register the current thread.
 *
 * A thread can only be profiled if it has been registered by a call to this function.
 *
 * @param   uPC             The program counter register.
 * @param   uStackBasePtr   The base of the stack.
 */
KPRF_DECL_FUNC(KPRF_TYPE(P,THREAD), RegisterThread)(KPRF_TYPE(,UPTR) uStackBasePtr, const char *pszName)
{
    /*
     * Get the header and adjust input address.
     * (It doesn't matter whether we're active or not.)
     */
    KPRF_TYPE(P,HDR)    pHdr = KPRF_GET_HDR();
    if (!pHdr)
        return NULL;
    const KPRF_TYPE(,UPTR) uBasePtr = pHdr->uBasePtr;
    if (uBasePtr)
        uStackBasePtr -= uBasePtr;


    /*
     * Allocate a thread and a stack.
     */
    KPRF_THREADS_LOCK();
    if (pHdr->cThreads < pHdr->cMaxThreads)
    {
        KPRF_TYPE(P,STACK) pStack = KPRF_OFF2PTR(P,STACK, pHdr->offStacks, pHdr);
        KU32               cLeft = pHdr->cMaxStacks;
        do
        {
            if (!pStack->offThread)
            {
                /* init the stack. */
                pStack->cFrames   = 0;
                pStack->offThread = pHdr->offThreads + pHdr->cbThread * pHdr->cThreads++;
                pHdr->cStacks++;

                /* init the thread */
                KPRF_TYPE(P,THREAD) pThread = KPRF_OFF2PTR(P,THREAD, pStack->offThread, pHdr);
                pThread->ThreadId       = KPRF_GET_THREADID();
                unsigned i = 0;
                if (pszName)
                    while (i < sizeof(pThread->szName) - 1 && *pszName)
                        pThread->szName[i++] = *pszName++;
                while (i < sizeof(pThread->szName))
                    pThread->szName[i++] = '\0';
                pThread->enmState       = KPRF_TYPE(,THREADSTATE_SUSPENDED);
                pThread->Reserved0      = KPRF_TYPE(,THREADSTATE_TERMINATED);
                pThread->uStackBasePtr  = uStackBasePtr;
                pThread->cbMaxStack     = 0;
                pThread->cCalls         = 0;
                pThread->cOverflows     = 0;
                pThread->cStackSwitchRejects = 0;
                pThread->cUnwinds       = 0;
                pThread->ProfiledTicks  = 0;
                pThread->OverheadTicks  = 0;
                pThread->SleepTicks     = 0;
                pThread->offStack       = KPRF_PTR2OFF(pStack, pHdr);


                /* set the thread and make it active. */
                KPRF_THREADS_UNLOCK();
                KPRF_SET_THREAD(pThread);
                pThread->enmState = KPRF_TYPE(,THREADSTATE_ACTIVE);
                return pThread;
            }

            /* next */
            pStack = KPRF_TYPE(P,STACK)(((KPRF_TYPE(,UPTR))pStack + pHdr->cbStack));
        } while (--cLeft > 0);
    }

    KPRF_THREADS_UNLOCK();
    return NULL;
}


/**
 * Terminates a thread.
 *
 * To terminate the current thread use DeregisterThread(), because that
 * cleans up the TLS entry too.
 *
 * @param   pHdr        The profiler data set header.
 * @param   pThread     The thread to terminate.
 * @param   TS          The timestamp to use when terminating the thread.
 */
KPRF_DECL_FUNC(void, TerminateThread)(KPRF_TYPE(P,HDR) pHdr, KPRF_TYPE(P,THREAD) pThread, KU64 TS)
{
    if (pThread->enmState == KPRF_TYPE(,THREADSTATE_TERMINATED))
        return;
    pThread->enmState = KPRF_TYPE(,THREADSTATE_TERMINATED);

    /*
     * Unwind the entire stack.
     */
    if (pThread->offStack)
    {
        KPRF_TYPE(P,STACK) pStack = KPRF_OFF2PTR(P,STACK, pThread->offStack, pHdr);
        for (KU32 cFrames = pStack->cFrames; cFrames > 0; cFrames--)
            KPRF_NAME(UnwindOne)(pHdr, pStack, 0, TS);

        /*
         * Free the stack.
         */
        pThread->offStack = 0;
        KPRF_THREADS_LOCK();
        pStack->offThread = 0;
        pHdr->cStacks--;
        KPRF_THREADS_UNLOCK();
    }
}


/**
 * Deregister (terminate) the current thread.
 */
KPRF_DECL_FUNC(void, DeregisterThread)(void)
{
    KU64 TS = KPRF_NOW();

    /*
     * Get the header, then get the thread and mark it terminated.
     * (It doesn't matter whether we're active or not.)
     */
    KPRF_TYPE(P,HDR)    pHdr = KPRF_GET_HDR();
    if (!pHdr)
        return;

    KPRF_TYPE(P,THREAD) pThread = KPRF_GET_THREAD();
    KPRF_SET_THREAD(NULL);
    if (!pThread)
        return;
    KPRF_NAME(TerminateThread)(pHdr, pThread, TS);
}


/**
 * Resumes / restarts a thread.
 *
 * @param   fReset  If set the stack is reset.
 */
KPRF_DECL_FUNC(void, ResumeThread)(int fReset)
{
    KU64 TS = KPRF_NOW();

    /*
     * Get the header, then get the thread and mark it terminated.
     * (It doesn't matter whether we're active or not.)
     */
    KPRF_TYPE(P,HDR)    pHdr = KPRF_GET_HDR();
    if (!pHdr)
        return;

    KPRF_TYPE(P,THREAD) pThread = KPRF_GET_THREAD();
    if (!pThread)
        return;
    if (pThread->enmState != KPRF_TYPE(,THREADSTATE_SUSPENDED))
        return;

    /*
     * Reset (unwind) the stack?
     */
    KPRF_TYPE(P,STACK) pStack = KPRF_OFF2PTR(P,STACK, pThread->offStack, pHdr);
    if (fReset)
    {
        KU32 cFrames = pStack->cFrames;
        while (cFrames-- > 0)
            KPRF_NAME(UnwindOne)(pHdr, pStack, 0, TS);
    }
    /*
     * If we've got any thing on the stack, we'll have to stop the sleeping period.
     */
    else if (pStack->cFrames > 0)
    {
        KPRF_TYPE(P,FRAME) pFrame = &pStack->aFrames[pStack->cFrames - 1];

        /* update the sleeping time and set the start of the new top-of-stack period. */
        pFrame->SleepTicks += TS - pFrame->OnTopOfStackStart;
        pFrame->OnTopOfStackStart = TS;
    }
    /** @todo we're not accounting overhead here! */

    /*
     * We're done, switch the thread to active state.
     */
    pThread->enmState = KPRF_TYPE(,THREADSTATE_ACTIVE);
}


/**
 * Suspend / completes a thread.
 *
 * The thread will be in a suspend state where the time will be accounted for as sleeping.
 *
 * @param   fUnwind     If set the stack is unwound and the thread statistics updated.
 */
KPRF_DECL_FUNC(void, SuspendThread)(int fUnwind)
{
    KU64 TS = KPRF_NOW();

    /*
     * Get the header, then get the thread and mark it terminated.
     * (It doesn't matter whether we're active or not.)
     */
    KPRF_TYPE(P,HDR)    pHdr = KPRF_GET_HDR();
    if (!pHdr)
        return;

    KPRF_TYPE(P,THREAD) pThread = KPRF_GET_THREAD();
    if (!pThread)
        return;
    if (    pThread->enmState != KPRF_TYPE(,THREADSTATE_ACTIVE)
        &&  pThread->enmState != KPRF_TYPE(,THREADSTATE_OVERFLOWED)
        &&  (pThread->enmState != KPRF_TYPE(,THREADSTATE_SUSPENDED) || fUnwind))
        return;

    pThread->enmState = KPRF_TYPE(,THREADSTATE_SUSPENDED);

    /*
     * Unwind the stack?
     */
    KPRF_TYPE(P,STACK) pStack = KPRF_OFF2PTR(P,STACK, pThread->offStack, pHdr);
    if (fUnwind)
    {
        KU32 cFrames = pStack->cFrames;
        while (cFrames-- > 0)
            KPRF_NAME(UnwindOne)(pHdr, pStack, 0, TS);
    }
    /*
     * If we've got any thing on the stack, we'll have to record the sleeping period
     * of the thread. If not we'll ignore it (for now at least).
     */
    else if (pStack->cFrames > 0)
    {
        KPRF_TYPE(P,FRAME) pFrame = &pStack->aFrames[pStack->cFrames - 1];

        /* update the top of stack time and set the start of the sleep period. */
        pFrame->OnTopOfStackTicks += TS - pFrame->OnTopOfStackStart;
        pFrame->OnTopOfStackStart = TS;
    }

    /** @todo we're not accounting overhead here! */
}



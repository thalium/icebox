/* $Id: IEMAllCImplStrInstr.cpp.h $ */
/** @file
 * IEM - String Instruction Implementation Code Template.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#if OP_SIZE == 8
# define OP_rAX     al
#elif OP_SIZE == 16
# define OP_rAX     ax
#elif OP_SIZE == 32
# define OP_rAX     eax
#elif OP_SIZE == 64
# define OP_rAX     rax
#else
# error "Bad OP_SIZE."
#endif
#define OP_TYPE                     RT_CONCAT3(uint,OP_SIZE,_t)

#if ADDR_SIZE == 16
# define ADDR_rDI   di
# define ADDR_rSI   si
# define ADDR_rCX   cx
# define ADDR2_TYPE uint32_t
#elif ADDR_SIZE == 32
# define ADDR_rDI   edi
# define ADDR_rSI   esi
# define ADDR_rCX   ecx
# define ADDR2_TYPE uint32_t
#elif ADDR_SIZE == 64
# define ADDR_rDI   rdi
# define ADDR_rSI   rsi
# define ADDR_rCX   rcx
# define ADDR2_TYPE uint64_t
# define IS_64_BIT_CODE(a_pVCpu)    (true)
#else
# error "Bad ADDR_SIZE."
#endif
#define ADDR_TYPE                   RT_CONCAT3(uint,ADDR_SIZE,_t)

#if ADDR_SIZE == 64 || OP_SIZE == 64
# define IS_64_BIT_CODE(a_pVCpu)    (true)
#elif ADDR_SIZE == 32
# define IS_64_BIT_CODE(a_pVCpu)    ((a_pVCpu)->iem.s.enmCpuMode == IEMMODE_64BIT)
#else
# define IS_64_BIT_CODE(a_pVCpu)    (false)
#endif

/** @def IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN
 * Used in the outer (page-by-page) loop to check for reasons for returnning
 * before completing the instruction.   In raw-mode we temporarily enable
 * interrupts to let the host interrupt us.  We cannot let big string operations
 * hog the CPU, especially not in raw-mode.
 */
#ifdef IN_RC
# define IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(a_pVM, a_pVCpu, a_fEflags) \
    do { \
        if (RT_LIKELY(   (   !VMCPU_FF_IS_PENDING(a_pVCpu, (a_fEflags) & X86_EFL_IF ? VMCPU_FF_YIELD_REPSTR_MASK \
                                                                                   : VMCPU_FF_YIELD_REPSTR_NOINT_MASK) \
                          && !VM_FF_IS_PENDING(a_pVM, VM_FF_YIELD_REPSTR_MASK) ) \
                      || IEM_VERIFICATION_ENABLED(a_pVCpu) )) \
        { \
            RTCCUINTREG fSavedFlags = ASMGetFlags(); \
            if (!(fSavedFlags & X86_EFL_IF)) \
            { \
                ASMSetFlags(fSavedFlags | X86_EFL_IF); \
                ASMNopPause(); \
                ASMSetFlags(fSavedFlags); \
            } \
        } \
        else \
        { \
            LogFlow(("%s: Leaving early (outer)! ffcpu=%#x ffvm=%#x\n", \
                     __FUNCTION__, (a_pVCpu)->fLocalForcedActions, (a_pVM)->fGlobalForcedActions)); \
            return VINF_SUCCESS; \
        } \
    } while (0)
#else
# define IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(a_pVM, a_pVCpu, a_fEflags) \
    do { \
        if (RT_LIKELY(   (   !VMCPU_FF_IS_PENDING(a_pVCpu, (a_fEflags) & X86_EFL_IF ? VMCPU_FF_YIELD_REPSTR_MASK \
                                                                                   : VMCPU_FF_YIELD_REPSTR_NOINT_MASK) \
                          && !VM_FF_IS_PENDING(a_pVM, VM_FF_YIELD_REPSTR_MASK) ) \
                      || IEM_VERIFICATION_ENABLED(a_pVCpu) )) \
        { /* probable */ } \
        else  \
        { \
            LogFlow(("%s: Leaving early (outer)! ffcpu=%#x ffvm=%#x\n", \
                     __FUNCTION__, (a_pVCpu)->fLocalForcedActions, (a_pVM)->fGlobalForcedActions)); \
            return VINF_SUCCESS; \
        } \
    } while (0)
#endif

/** @def IEM_CHECK_FF_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN
 * This is used in some of the inner loops to make sure we respond immediately
 * to VMCPU_FF_IOM as well as outside requests.  Use this for expensive
 * instructions. Use IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN for
 * ones that are typically cheap. */
#define IEM_CHECK_FF_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(a_pVM, a_pVCpu, a_fExitExpr) \
    do { \
        if (RT_LIKELY(   (   !VMCPU_FF_IS_PENDING(a_pVCpu, VMCPU_FF_HIGH_PRIORITY_POST_REPSTR_MASK) \
                          && !VM_FF_IS_PENDING(a_pVM,         VM_FF_HIGH_PRIORITY_POST_REPSTR_MASK)) \
                      || (a_fExitExpr) \
                      || IEM_VERIFICATION_ENABLED(a_pVCpu) )) \
        { /* very likely */ } \
        else \
        { \
            LogFlow(("%s: Leaving early (inner)! ffcpu=%#x ffvm=%#x\n", \
                     __FUNCTION__, (a_pVCpu)->fLocalForcedActions, (a_pVM)->fGlobalForcedActions)); \
            return VINF_SUCCESS; \
        } \
    } while (0)


/** @def IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN
 * This is used in the inner loops where
 * IEM_CHECK_FF_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN isn't used.  It only
 * checks the CPU FFs so that we respond immediately to the pending IOM FF
 * (status code is hidden in IEMCPU::rcPassUp by IEM memory commit code).
 */
#define IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(a_pVM, a_pVCpu, a_fExitExpr) \
    do { \
        if (RT_LIKELY(   !VMCPU_FF_IS_PENDING(a_pVCpu, VMCPU_FF_HIGH_PRIORITY_POST_REPSTR_MASK) \
                      || (a_fExitExpr) \
                      || IEM_VERIFICATION_ENABLED(a_pVCpu) )) \
        { /* very likely */ } \
        else \
        { \
            LogFlow(("%s: Leaving early (inner)! ffcpu=%#x (ffvm=%#x)\n", \
                     __FUNCTION__, (a_pVCpu)->fLocalForcedActions, (a_pVM)->fGlobalForcedActions)); \
            return VINF_SUCCESS; \
        } \
    } while (0)


/**
 * Implements 'REPE CMPS'.
 */
IEM_CIMPL_DEF_1(RT_CONCAT4(iemCImpl_repe_cmps_op,OP_SIZE,_addr,ADDR_SIZE), uint8_t, iEffSeg)
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    ADDR_TYPE       uCounterReg  = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    PCCPUMSELREGHID pSrc1Hid     = iemSRegGetHid(pVCpu, iEffSeg);
    uint64_t        uSrc1Base;
    VBOXSTRICTRC    rcStrict     = iemMemSegCheckReadAccessEx(pVCpu, pSrc1Hid, iEffSeg, &uSrc1Base);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    uint64_t        uSrc2Base;
    rcStrict = iemMemSegCheckReadAccessEx(pVCpu, iemSRegUpdateHid(pVCpu, &pCtx->es), X86_SREG_ES, &uSrc2Base);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr       = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    ADDR_TYPE       uSrc1AddrReg = pCtx->ADDR_rSI;
    ADDR_TYPE       uSrc2AddrReg = pCtx->ADDR_rDI;
    uint32_t        uEFlags      = pCtx->eflags.u;

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtSrc1Addr = uSrc1AddrReg + (ADDR2_TYPE)uSrc1Base;
        ADDR2_TYPE  uVirtSrc2Addr = uSrc2AddrReg + (ADDR2_TYPE)uSrc2Base;
        uint32_t    cLeftSrc1Page = (PAGE_SIZE - (uVirtSrc1Addr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftSrc1Page > uCounterReg)
            cLeftSrc1Page = uCounterReg;
        uint32_t    cLeftSrc2Page = (PAGE_SIZE - (uVirtSrc2Addr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        uint32_t    cLeftPage     = RT_MIN(cLeftSrc1Page, cLeftSrc2Page);

        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Optimize reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uSrc1AddrReg < pSrc1Hid->u32Limit
                    && uSrc1AddrReg + (cLeftPage * (OP_SIZE / 8)) <= pSrc1Hid->u32Limit
                    && uSrc2AddrReg < pCtx->es.u32Limit
                    && uSrc2AddrReg + (cLeftPage * (OP_SIZE / 8)) <= pCtx->es.u32Limit)
               )
           )
        {
            RTGCPHYS GCPhysSrc1Mem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtSrc1Addr, IEM_ACCESS_DATA_R, &GCPhysSrc1Mem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            RTGCPHYS GCPhysSrc2Mem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtSrc2Addr, IEM_ACCESS_DATA_R, &GCPhysSrc2Mem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, do a block processing
             * until the end of the current page.
             */
            PGMPAGEMAPLOCK PgLockSrc2Mem;
            OP_TYPE const *puSrc2Mem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysSrc2Mem, IEM_ACCESS_DATA_R, (void **)&puSrc2Mem, &PgLockSrc2Mem);
            if (rcStrict == VINF_SUCCESS)
            {
                PGMPAGEMAPLOCK PgLockSrc1Mem;
                OP_TYPE const *puSrc1Mem;
                rcStrict = iemMemPageMap(pVCpu, GCPhysSrc1Mem, IEM_ACCESS_DATA_R, (void **)&puSrc1Mem, &PgLockSrc1Mem);
                if (rcStrict == VINF_SUCCESS)
                {
                    if (!memcmp(puSrc2Mem, puSrc1Mem, cLeftPage * (OP_SIZE / 8)))
                    {
                        /* All matches, only compare the last itme to get the right eflags. */
                        RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&puSrc1Mem[cLeftPage-1], puSrc2Mem[cLeftPage-1], &uEFlags);
                        uSrc1AddrReg += cLeftPage * cbIncr;
                        uSrc2AddrReg += cLeftPage * cbIncr;
                        uCounterReg  -= cLeftPage;
                    }
                    else
                    {
                        /* Some mismatch, compare each item (and keep volatile
                           memory in mind). */
                        uint32_t off = 0;
                        do
                        {
                            RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&puSrc1Mem[off], puSrc2Mem[off], &uEFlags);
                            off++;
                        } while (   off < cLeftPage
                                 && (uEFlags & X86_EFL_ZF));
                        uSrc1AddrReg += cbIncr * off;
                        uSrc2AddrReg += cbIncr * off;
                        uCounterReg  -= off;
                    }

                    /* Update the registers before looping. */
                    pCtx->ADDR_rCX = uCounterReg;
                    pCtx->ADDR_rSI = uSrc1AddrReg;
                    pCtx->ADDR_rDI = uSrc2AddrReg;
                    pCtx->eflags.u = uEFlags;

                    iemMemPageUnmap(pVCpu, GCPhysSrc1Mem, IEM_ACCESS_DATA_R, puSrc1Mem, &PgLockSrc1Mem);
                    iemMemPageUnmap(pVCpu, GCPhysSrc2Mem, IEM_ACCESS_DATA_R, puSrc2Mem, &PgLockSrc2Mem);
                    if (   uCounterReg == 0
                        || !(uEFlags & X86_EFL_ZF))
                        break;
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
                    continue;
                }
                iemMemPageUnmap(pVCpu, GCPhysSrc2Mem, IEM_ACCESS_DATA_R, puSrc2Mem, &PgLockSrc2Mem);
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         */
        do
        {
            OP_TYPE uValue1;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uValue1, iEffSeg, uSrc1AddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            OP_TYPE uValue2;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uValue2, X86_SREG_ES, uSrc2AddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)(&uValue1, uValue2, &uEFlags);

            pCtx->ADDR_rSI = uSrc1AddrReg += cbIncr;
            pCtx->ADDR_rDI = uSrc2AddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;
            pCtx->eflags.u = uEFlags;
            cLeftPage--;
            IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0 || !(uEFlags & X86_EFL_ZF));
        } while (   (int32_t)cLeftPage > 0
                 && (uEFlags & X86_EFL_ZF));

        /*
         * Next page? Must check for interrupts and stuff here.
         */
        if (   uCounterReg == 0
            || !(uEFlags & X86_EFL_ZF))
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'REPNE CMPS'.
 */
IEM_CIMPL_DEF_1(RT_CONCAT4(iemCImpl_repne_cmps_op,OP_SIZE,_addr,ADDR_SIZE), uint8_t, iEffSeg)
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    PCCPUMSELREGHID pSrc1Hid = iemSRegGetHid(pVCpu, iEffSeg);
    uint64_t        uSrc1Base;
    VBOXSTRICTRC rcStrict = iemMemSegCheckReadAccessEx(pVCpu, pSrc1Hid, iEffSeg, &uSrc1Base);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    uint64_t        uSrc2Base;
    rcStrict = iemMemSegCheckReadAccessEx(pVCpu, iemSRegUpdateHid(pVCpu, &pCtx->es), X86_SREG_ES, &uSrc2Base);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr       = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    ADDR_TYPE       uSrc1AddrReg = pCtx->ADDR_rSI;
    ADDR_TYPE       uSrc2AddrReg = pCtx->ADDR_rDI;
    uint32_t        uEFlags      = pCtx->eflags.u;

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtSrc1Addr = uSrc1AddrReg + (ADDR2_TYPE)uSrc1Base;
        ADDR2_TYPE  uVirtSrc2Addr = uSrc2AddrReg + (ADDR2_TYPE)uSrc2Base;
        uint32_t    cLeftSrc1Page = (PAGE_SIZE - (uVirtSrc1Addr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftSrc1Page > uCounterReg)
            cLeftSrc1Page = uCounterReg;
        uint32_t    cLeftSrc2Page = (PAGE_SIZE - (uVirtSrc2Addr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        uint32_t    cLeftPage = RT_MIN(cLeftSrc1Page, cLeftSrc2Page);

        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Optimize reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uSrc1AddrReg < pSrc1Hid->u32Limit
                    && uSrc1AddrReg + (cLeftPage * (OP_SIZE / 8)) <= pSrc1Hid->u32Limit
                    && uSrc2AddrReg < pCtx->es.u32Limit
                    && uSrc2AddrReg + (cLeftPage * (OP_SIZE / 8)) <= pCtx->es.u32Limit)
                )
           )
        {
            RTGCPHYS GCPhysSrc1Mem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtSrc1Addr, IEM_ACCESS_DATA_R, &GCPhysSrc1Mem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            RTGCPHYS GCPhysSrc2Mem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtSrc2Addr, IEM_ACCESS_DATA_R, &GCPhysSrc2Mem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, do a block processing
             * until the end of the current page.
             */
            OP_TYPE const *puSrc2Mem;
            PGMPAGEMAPLOCK PgLockSrc2Mem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysSrc2Mem, IEM_ACCESS_DATA_R, (void **)&puSrc2Mem, &PgLockSrc2Mem);
            if (rcStrict == VINF_SUCCESS)
            {
                OP_TYPE const *puSrc1Mem;
                PGMPAGEMAPLOCK PgLockSrc1Mem;
                rcStrict = iemMemPageMap(pVCpu, GCPhysSrc1Mem, IEM_ACCESS_DATA_R, (void **)&puSrc1Mem, &PgLockSrc1Mem);
                if (rcStrict == VINF_SUCCESS)
                {
                    if (memcmp(puSrc2Mem, puSrc1Mem, cLeftPage * (OP_SIZE / 8)))
                    {
                        /* All matches, only compare the last item to get the right eflags. */
                        RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&puSrc1Mem[cLeftPage-1], puSrc2Mem[cLeftPage-1], &uEFlags);
                        uSrc1AddrReg += cLeftPage * cbIncr;
                        uSrc2AddrReg += cLeftPage * cbIncr;
                        uCounterReg  -= cLeftPage;
                    }
                    else
                    {
                        /* Some mismatch, compare each item (and keep volatile
                           memory in mind). */
                        uint32_t off = 0;
                        do
                        {
                            RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&puSrc1Mem[off], puSrc2Mem[off], &uEFlags);
                            off++;
                        } while (   off < cLeftPage
                                 && !(uEFlags & X86_EFL_ZF));
                        uSrc1AddrReg += cbIncr * off;
                        uSrc2AddrReg += cbIncr * off;
                        uCounterReg  -= off;
                    }

                    /* Update the registers before looping. */
                    pCtx->ADDR_rCX = uCounterReg;
                    pCtx->ADDR_rSI = uSrc1AddrReg;
                    pCtx->ADDR_rDI = uSrc2AddrReg;
                    pCtx->eflags.u = uEFlags;

                    iemMemPageUnmap(pVCpu, GCPhysSrc1Mem, IEM_ACCESS_DATA_R, puSrc1Mem, &PgLockSrc1Mem);
                    iemMemPageUnmap(pVCpu, GCPhysSrc2Mem, IEM_ACCESS_DATA_R, puSrc2Mem, &PgLockSrc2Mem);
                    if (   uCounterReg == 0
                        || (uEFlags & X86_EFL_ZF))
                        break;
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
                    continue;
                }
                iemMemPageUnmap(pVCpu, GCPhysSrc2Mem, IEM_ACCESS_DATA_R, puSrc2Mem, &PgLockSrc2Mem);
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         */
        do
        {
            OP_TYPE uValue1;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uValue1, iEffSeg, uSrc1AddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            OP_TYPE uValue2;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uValue2, X86_SREG_ES, uSrc2AddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)(&uValue1, uValue2, &uEFlags);

            pCtx->ADDR_rSI = uSrc1AddrReg += cbIncr;
            pCtx->ADDR_rDI = uSrc2AddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;
            pCtx->eflags.u = uEFlags;
            cLeftPage--;
            IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0 || (uEFlags & X86_EFL_ZF));
        } while (   (int32_t)cLeftPage > 0
                 && !(uEFlags & X86_EFL_ZF));

        /*
         * Next page? Must check for interrupts and stuff here.
         */
        if (   uCounterReg == 0
            || (uEFlags & X86_EFL_ZF))
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'REPE SCAS'.
 */
IEM_CIMPL_DEF_0(RT_CONCAT4(iemCImpl_repe_scas_,OP_rAX,_m,ADDR_SIZE))
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    uint64_t        uBaseAddr;
    VBOXSTRICTRC rcStrict = iemMemSegCheckReadAccessEx(pVCpu, iemSRegUpdateHid(pVCpu, &pCtx->es), X86_SREG_ES, &uBaseAddr);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr      = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    OP_TYPE const   uValueReg   = pCtx->OP_rAX;
    ADDR_TYPE       uAddrReg    = pCtx->ADDR_rDI;
    uint32_t        uEFlags     = pCtx->eflags.u;

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtAddr = uAddrReg + (ADDR2_TYPE)uBaseAddr;
        uint32_t    cLeftPage = (PAGE_SIZE - (uVirtAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftPage > uCounterReg)
            cLeftPage = uCounterReg;
        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Implement reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uAddrReg < pCtx->es.u32Limit
                    && uAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pCtx->es.u32Limit)
               )
           )
        {
            RTGCPHYS GCPhysMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtAddr, IEM_ACCESS_DATA_R, &GCPhysMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, do a block processing
             * until the end of the current page.
             */
            PGMPAGEMAPLOCK PgLockMem;
            OP_TYPE const *puMem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, (void **)&puMem, &PgLockMem);
            if (rcStrict == VINF_SUCCESS)
            {
                /* Search till we find a mismatching item. */
                OP_TYPE  uTmpValue;
                bool     fQuit;
                uint32_t i = 0;
                do
                {
                    uTmpValue = puMem[i++];
                    fQuit = uTmpValue != uValueReg;
                } while (i < cLeftPage && !fQuit);

                /* Update the regs. */
                RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&uValueReg, uTmpValue, &uEFlags);
                pCtx->ADDR_rCX = uCounterReg -= i;
                pCtx->ADDR_rDI = uAddrReg    += i * cbIncr;
                pCtx->eflags.u = uEFlags;
                Assert(!(uEFlags & X86_EFL_ZF) == fQuit);
                iemMemPageUnmap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, puMem, &PgLockMem);
                if (   fQuit
                    || uCounterReg == 0)
                    break;

                /* If unaligned, we drop thru and do the page crossing access
                   below. Otherwise, do the next page. */
                if (!(uVirtAddr & (OP_SIZE - 1)))
                {
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
                    continue;
                }
                cLeftPage = 0;
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         */
        do
        {
            OP_TYPE uTmpValue;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uTmpValue, X86_SREG_ES, uAddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&uValueReg, uTmpValue, &uEFlags);

            pCtx->ADDR_rDI = uAddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;
            pCtx->eflags.u = uEFlags;
            cLeftPage--;
            IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0 || !(uEFlags & X86_EFL_ZF));
        } while (   (int32_t)cLeftPage > 0
                 && (uEFlags & X86_EFL_ZF));

        /*
         * Next page? Must check for interrupts and stuff here.
         */
        if (   uCounterReg == 0
            || !(uEFlags & X86_EFL_ZF))
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'REPNE SCAS'.
 */
IEM_CIMPL_DEF_0(RT_CONCAT4(iemCImpl_repne_scas_,OP_rAX,_m,ADDR_SIZE))
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    uint64_t        uBaseAddr;
    VBOXSTRICTRC rcStrict = iemMemSegCheckReadAccessEx(pVCpu, iemSRegUpdateHid(pVCpu, &pCtx->es), X86_SREG_ES, &uBaseAddr);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr      = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    OP_TYPE const   uValueReg   = pCtx->OP_rAX;
    ADDR_TYPE       uAddrReg    = pCtx->ADDR_rDI;
    uint32_t        uEFlags     = pCtx->eflags.u;

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtAddr = uAddrReg + (ADDR2_TYPE)uBaseAddr;
        uint32_t    cLeftPage = (PAGE_SIZE - (uVirtAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftPage > uCounterReg)
            cLeftPage = uCounterReg;
        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Implement reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uAddrReg < pCtx->es.u32Limit
                    && uAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pCtx->es.u32Limit)
               )
           )
        {
            RTGCPHYS GCPhysMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtAddr, IEM_ACCESS_DATA_R, &GCPhysMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, do a block processing
             * until the end of the current page.
             */
            PGMPAGEMAPLOCK PgLockMem;
            OP_TYPE const *puMem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, (void **)&puMem, &PgLockMem);
            if (rcStrict == VINF_SUCCESS)
            {
                /* Search till we find a mismatching item. */
                OP_TYPE  uTmpValue;
                bool     fQuit;
                uint32_t i = 0;
                do
                {
                    uTmpValue = puMem[i++];
                    fQuit = uTmpValue == uValueReg;
                } while (i < cLeftPage && !fQuit);

                /* Update the regs. */
                RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&uValueReg, uTmpValue, &uEFlags);
                pCtx->ADDR_rCX = uCounterReg -= i;
                pCtx->ADDR_rDI = uAddrReg    += i * cbIncr;
                pCtx->eflags.u = uEFlags;
                Assert(!!(uEFlags & X86_EFL_ZF) == fQuit);
                iemMemPageUnmap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, puMem, &PgLockMem);
                if (   fQuit
                    || uCounterReg == 0)
                    break;

                /* If unaligned, we drop thru and do the page crossing access
                   below. Otherwise, do the next page. */
                if (!(uVirtAddr & (OP_SIZE - 1)))
                {
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
                    continue;
                }
                cLeftPage = 0;
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         */
        do
        {
            OP_TYPE uTmpValue;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uTmpValue, X86_SREG_ES, uAddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            RT_CONCAT(iemAImpl_cmp_u,OP_SIZE)((OP_TYPE *)&uValueReg, uTmpValue, &uEFlags);
            pCtx->ADDR_rDI = uAddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;
            pCtx->eflags.u = uEFlags;
            cLeftPage--;
            IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0 || (uEFlags & X86_EFL_ZF));
        } while (   (int32_t)cLeftPage > 0
                 && !(uEFlags & X86_EFL_ZF));

        /*
         * Next page? Must check for interrupts and stuff here.
         */
        if (   uCounterReg == 0
            || (uEFlags & X86_EFL_ZF))
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, uEFlags);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}




/**
 * Implements 'REP MOVS'.
 */
IEM_CIMPL_DEF_1(RT_CONCAT4(iemCImpl_rep_movs_op,OP_SIZE,_addr,ADDR_SIZE), uint8_t, iEffSeg)
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    PCCPUMSELREGHID pSrcHid = iemSRegGetHid(pVCpu, iEffSeg);
    uint64_t        uSrcBase;
    VBOXSTRICTRC rcStrict = iemMemSegCheckReadAccessEx(pVCpu, pSrcHid, iEffSeg, &uSrcBase);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    uint64_t        uDstBase;
    rcStrict = iemMemSegCheckWriteAccessEx(pVCpu, iemSRegUpdateHid(pVCpu, &pCtx->es), X86_SREG_ES, &uDstBase);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr      = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    ADDR_TYPE       uSrcAddrReg = pCtx->ADDR_rSI;
    ADDR_TYPE       uDstAddrReg = pCtx->ADDR_rDI;

    /*
     * Be careful with handle bypassing.
     */
    if (pVCpu->iem.s.fBypassHandlers)
    {
        Log(("%s: declining because we're bypassing handlers\n", __FUNCTION__));
        return VERR_IEM_ASPECT_NOT_IMPLEMENTED;
    }

    /*
     * If we're reading back what we write, we have to let the verfication code
     * to prevent a false positive.
     * Note! This doesn't take aliasing or wrapping into account - lazy bird.
     */
#ifdef IEM_VERIFICATION_MODE_FULL
    if (   IEM_VERIFICATION_ENABLED(pVCpu)
        && (cbIncr > 0
            ?    uSrcAddrReg <= uDstAddrReg
              && uSrcAddrReg + cbIncr * uCounterReg > uDstAddrReg
            :    uDstAddrReg <= uSrcAddrReg
              && uDstAddrReg + cbIncr * uCounterReg > uSrcAddrReg))
        pVCpu->iem.s.fOverlappingMovs = true;
#endif

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtSrcAddr = uSrcAddrReg + (ADDR2_TYPE)uSrcBase;
        ADDR2_TYPE  uVirtDstAddr = uDstAddrReg + (ADDR2_TYPE)uDstBase;
        uint32_t    cLeftSrcPage = (PAGE_SIZE - (uVirtSrcAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftSrcPage > uCounterReg)
            cLeftSrcPage = uCounterReg;
        uint32_t    cLeftDstPage = (PAGE_SIZE - (uVirtDstAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        uint32_t    cLeftPage = RT_MIN(cLeftSrcPage, cLeftDstPage);

        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Implement reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uSrcAddrReg < pSrcHid->u32Limit
                    && uSrcAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pSrcHid->u32Limit
                    && uDstAddrReg < pCtx->es.u32Limit
                    && uDstAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pCtx->es.u32Limit)
               )
           )
        {
            RTGCPHYS GCPhysSrcMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtSrcAddr, IEM_ACCESS_DATA_R, &GCPhysSrcMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            RTGCPHYS GCPhysDstMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtDstAddr, IEM_ACCESS_DATA_W, &GCPhysDstMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, do a block processing
             * until the end of the current page.
             */
            PGMPAGEMAPLOCK PgLockDstMem;
            OP_TYPE *puDstMem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysDstMem, IEM_ACCESS_DATA_W, (void **)&puDstMem, &PgLockDstMem);
            if (rcStrict == VINF_SUCCESS)
            {
                PGMPAGEMAPLOCK PgLockSrcMem;
                OP_TYPE const *puSrcMem;
                rcStrict = iemMemPageMap(pVCpu, GCPhysSrcMem, IEM_ACCESS_DATA_R, (void **)&puSrcMem, &PgLockSrcMem);
                if (rcStrict == VINF_SUCCESS)
                {
                    Assert(   (GCPhysSrcMem         >> PAGE_SHIFT) != (GCPhysDstMem         >> PAGE_SHIFT)
                           || ((uintptr_t)puSrcMem  >> PAGE_SHIFT) == ((uintptr_t)puDstMem  >> PAGE_SHIFT));

                    /* Perform the operation exactly (don't use memcpy to avoid
                       having to consider how its implementation would affect
                       any overlapping source and destination area). */
                    OP_TYPE const  *puSrcCur = puSrcMem;
                    OP_TYPE        *puDstCur = puDstMem;
                    uint32_t        cTodo    = cLeftPage;
                    while (cTodo-- > 0)
                        *puDstCur++ = *puSrcCur++;

                    /* Update the registers. */
                    pCtx->ADDR_rSI = uSrcAddrReg += cLeftPage * cbIncr;
                    pCtx->ADDR_rDI = uDstAddrReg += cLeftPage * cbIncr;
                    pCtx->ADDR_rCX = uCounterReg -= cLeftPage;

                    iemMemPageUnmap(pVCpu, GCPhysSrcMem, IEM_ACCESS_DATA_R, puSrcMem, &PgLockSrcMem);
                    iemMemPageUnmap(pVCpu, GCPhysDstMem, IEM_ACCESS_DATA_W, puDstMem, &PgLockDstMem);

                    if (uCounterReg == 0)
                        break;
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
                    continue;
                }
                iemMemPageUnmap(pVCpu, GCPhysDstMem, IEM_ACCESS_DATA_W, puDstMem, &PgLockDstMem);
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         */
        do
        {
            OP_TYPE uValue;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uValue, iEffSeg, uSrcAddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            rcStrict = RT_CONCAT(iemMemStoreDataU,OP_SIZE)(pVCpu, X86_SREG_ES, uDstAddrReg, uValue);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            pCtx->ADDR_rSI = uSrcAddrReg += cbIncr;
            pCtx->ADDR_rDI = uDstAddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;
            cLeftPage--;
            IEM_CHECK_FF_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0);
        } while ((int32_t)cLeftPage > 0);

        /*
         * Next page.  Must check for interrupts and stuff here.
         */
        if (uCounterReg == 0)
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'REP STOS'.
 */
IEM_CIMPL_DEF_0(RT_CONCAT4(iemCImpl_stos_,OP_rAX,_m,ADDR_SIZE))
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    uint64_t        uBaseAddr;
    VBOXSTRICTRC rcStrict = iemMemSegCheckWriteAccessEx(pVCpu, iemSRegUpdateHid(pVCpu, &pCtx->es), X86_SREG_ES, &uBaseAddr);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr      = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    OP_TYPE const   uValue      = pCtx->OP_rAX;
    ADDR_TYPE       uAddrReg    = pCtx->ADDR_rDI;

    /*
     * Be careful with handle bypassing.
     */
    /** @todo Permit doing a page if correctly aligned. */
    if (pVCpu->iem.s.fBypassHandlers)
    {
        Log(("%s: declining because we're bypassing handlers\n", __FUNCTION__));
        return VERR_IEM_ASPECT_NOT_IMPLEMENTED;
    }

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtAddr = uAddrReg + (ADDR2_TYPE)uBaseAddr;
        uint32_t    cLeftPage = (PAGE_SIZE - (uVirtAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftPage > uCounterReg)
            cLeftPage = uCounterReg;
        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Implement reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uAddrReg < pCtx->es.u32Limit
                    && uAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pCtx->es.u32Limit)
               )
           )
        {
            RTGCPHYS GCPhysMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtAddr, IEM_ACCESS_DATA_W, &GCPhysMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, do a block processing
             * until the end of the current page.
             */
            PGMPAGEMAPLOCK PgLockMem;
            OP_TYPE *puMem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_W, (void **)&puMem, &PgLockMem);
            if (rcStrict == VINF_SUCCESS)
            {
                /* Update the regs first so we can loop on cLeftPage. */
                pCtx->ADDR_rCX = uCounterReg -= cLeftPage;
                pCtx->ADDR_rDI = uAddrReg    += cLeftPage * cbIncr;

                /* Do the memsetting. */
#if OP_SIZE == 8
                memset(puMem, uValue, cLeftPage);
/*#elif OP_SIZE == 32
                ASMMemFill32(puMem, cLeftPage * (OP_SIZE / 8), uValue);*/
#else
                while (cLeftPage-- > 0)
                    *puMem++ = uValue;
#endif

                iemMemPageUnmap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_W, puMem, &PgLockMem);

                if (uCounterReg == 0)
                    break;

                /* If unaligned, we drop thru and do the page crossing access
                   below. Otherwise, do the next page. */
                if (!(uVirtAddr & (OP_SIZE - 1)))
                {
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
                    continue;
                }
                cLeftPage = 0;
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         */
        do
        {
            rcStrict = RT_CONCAT(iemMemStoreDataU,OP_SIZE)(pVCpu, X86_SREG_ES, uAddrReg, uValue);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            pCtx->ADDR_rDI = uAddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;
            cLeftPage--;
            IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0);
        } while ((int32_t)cLeftPage > 0);

        /*
         * Next page.  Must check for interrupts and stuff here.
         */
        if (uCounterReg == 0)
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'REP LODS'.
 */
IEM_CIMPL_DEF_1(RT_CONCAT4(iemCImpl_lods_,OP_rAX,_m,ADDR_SIZE), int8_t, iEffSeg)
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    PCCPUMSELREGHID pSrcHid = iemSRegGetHid(pVCpu, iEffSeg);
    uint64_t        uBaseAddr;
    VBOXSTRICTRC rcStrict = iemMemSegCheckReadAccessEx(pVCpu, pSrcHid, iEffSeg, &uBaseAddr);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr      = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    ADDR_TYPE       uAddrReg    = pCtx->ADDR_rSI;

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtAddr = uAddrReg + (ADDR2_TYPE)uBaseAddr;
        uint32_t    cLeftPage = (PAGE_SIZE - (uVirtAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftPage > uCounterReg)
            cLeftPage = uCounterReg;
        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Implement reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uAddrReg < pSrcHid->u32Limit
                    && uAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pSrcHid->u32Limit)
               )
           )
        {
            RTGCPHYS GCPhysMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtAddr, IEM_ACCESS_DATA_R, &GCPhysMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, we can get away with
             * just reading the last value on the page.
             */
            PGMPAGEMAPLOCK PgLockMem;
            OP_TYPE const *puMem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, (void **)&puMem, &PgLockMem);
            if (rcStrict == VINF_SUCCESS)
            {
                /* Only get the last byte, the rest doesn't matter in direct access mode. */
#if OP_SIZE == 32
                pCtx->rax      = puMem[cLeftPage - 1];
#else
                pCtx->OP_rAX   = puMem[cLeftPage - 1];
#endif
                pCtx->ADDR_rCX = uCounterReg -= cLeftPage;
                pCtx->ADDR_rSI = uAddrReg    += cLeftPage * cbIncr;
                iemMemPageUnmap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, puMem, &PgLockMem);

                if (uCounterReg == 0)
                    break;

                /* If unaligned, we drop thru and do the page crossing access
                   below. Otherwise, do the next page. */
                if (!(uVirtAddr & (OP_SIZE - 1)))
                {
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
                    continue;
                }
                cLeftPage = 0;
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         */
        do
        {
            OP_TYPE uTmpValue;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uTmpValue, iEffSeg, uAddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
#if OP_SIZE == 32
            pCtx->rax      = uTmpValue;
#else
            pCtx->OP_rAX   = uTmpValue;
#endif
            pCtx->ADDR_rSI = uAddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;
            cLeftPage--;
            IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0);
        } while ((int32_t)cLeftPage > 0);

        if (rcStrict != VINF_SUCCESS)
            break;

        /*
         * Next page.  Must check for interrupts and stuff here.
         */
        if (uCounterReg == 0)
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


#if OP_SIZE != 64

/**
 * Implements 'INS' (no rep)
 */
IEM_CIMPL_DEF_1(RT_CONCAT4(iemCImpl_ins_op,OP_SIZE,_addr,ADDR_SIZE), bool, fIoChecked)
{
    PVM             pVM  = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC    rcStrict;

    /*
     * Be careful with handle bypassing.
     */
    if (pVCpu->iem.s.fBypassHandlers)
    {
        Log(("%s: declining because we're bypassing handlers\n", __FUNCTION__));
        return VERR_IEM_ASPECT_NOT_IMPLEMENTED;
    }

    /*
     * ASSUMES the #GP for I/O permission is taken first, then any #GP for
     * segmentation and finally any #PF due to virtual address translation.
     * ASSUMES nothing is read from the I/O port before traps are taken.
     */
    if (!fIoChecked)
    {
        rcStrict = iemHlpCheckPortIOPermission(pVCpu, pCtx, pCtx->dx, OP_SIZE / 8);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }

#ifdef VBOX_WITH_NESTED_HWVIRT
    /*
     * Check SVM nested-guest IO intercept.
     */
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IOIO_PROT))
    {
        rcStrict = iemSvmHandleIOIntercept(pVCpu, pCtx->dx, SVMIOIOTYPE_IN, OP_SIZE / 8, ADDR_SIZE, X86_SREG_ES, false /* fRep */,
                                           true /* fStrIo */, cbInstr);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("iemCImpl_ins_op: iemSvmHandleIOIntercept failed (u16Port=%#x, cbReg=%u) rc=%Rrc\n", pCtx->dx, OP_SIZE / 8,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    OP_TYPE        *puMem;
    rcStrict = iemMemMap(pVCpu, (void **)&puMem, OP_SIZE / 8, X86_SREG_ES, pCtx->ADDR_rDI, IEM_ACCESS_DATA_W);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    uint32_t        u32Value = 0;
    if (!IEM_VERIFICATION_ENABLED(pVCpu))
        rcStrict = IOMIOPortRead(pVM, pVCpu, pCtx->dx, &u32Value, OP_SIZE / 8);
    else
        rcStrict = iemVerifyFakeIOPortRead(pVCpu, pCtx->dx, &u32Value, OP_SIZE / 8);
    if (IOM_SUCCESS(rcStrict))
    {
        *puMem = (OP_TYPE)u32Value;
# ifdef IN_RING3
        VBOXSTRICTRC rcStrict2 = iemMemCommitAndUnmap(pVCpu, puMem, IEM_ACCESS_DATA_W);
# else
        VBOXSTRICTRC rcStrict2 = iemMemCommitAndUnmapPostponeTroubleToR3(pVCpu, puMem, IEM_ACCESS_DATA_W);
# endif
        if (RT_LIKELY(rcStrict2 == VINF_SUCCESS))
        {
            if (!pCtx->eflags.Bits.u1DF)
                pCtx->ADDR_rDI += OP_SIZE / 8;
            else
                pCtx->ADDR_rDI -= OP_SIZE / 8;
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
        else
            AssertLogRelMsgFailedReturn(("rcStrict2=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict2)), RT_FAILURE_NP(rcStrict2) ? rcStrict2 : VERR_IEM_IPE_1);
    }
    return rcStrict;
}


/**
 * Implements 'REP INS'.
 */
IEM_CIMPL_DEF_1(RT_CONCAT4(iemCImpl_rep_ins_op,OP_SIZE,_addr,ADDR_SIZE), bool, fIoChecked)
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    uint16_t const  u16Port    = pCtx->dx;
    VBOXSTRICTRC    rcStrict;
    if (!fIoChecked)
    {
        rcStrict = iemHlpCheckPortIOPermission(pVCpu, pCtx, u16Port, OP_SIZE / 8);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }

#ifdef VBOX_WITH_NESTED_HWVIRT
    /*
     * Check SVM nested-guest IO intercept.
     */
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IOIO_PROT))
    {
        rcStrict = iemSvmHandleIOIntercept(pVCpu, u16Port, SVMIOIOTYPE_IN, OP_SIZE / 8, ADDR_SIZE, X86_SREG_ES, true /* fRep */,
                                           true /* fStrIo */, cbInstr);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("iemCImpl_rep_ins_op: iemSvmHandleIOIntercept failed (u16Port=%#x, cbReg=%u) rc=%Rrc\n", u16Port, OP_SIZE / 8,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    uint64_t        uBaseAddr;
    rcStrict = iemMemSegCheckWriteAccessEx(pVCpu, iemSRegUpdateHid(pVCpu, &pCtx->es), X86_SREG_ES, &uBaseAddr);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr      = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    ADDR_TYPE       uAddrReg    = pCtx->ADDR_rDI;

    /*
     * Be careful with handle bypassing.
     */
    if (pVCpu->iem.s.fBypassHandlers)
    {
        Log(("%s: declining because we're bypassing handlers\n", __FUNCTION__));
        return VERR_IEM_ASPECT_NOT_IMPLEMENTED;
    }

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtAddr = uAddrReg + (ADDR2_TYPE)uBaseAddr;
        uint32_t    cLeftPage = (PAGE_SIZE - (uVirtAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftPage > uCounterReg)
            cLeftPage = uCounterReg;
        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Implement reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uAddrReg < pCtx->es.u32Limit
                    && uAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pCtx->es.u32Limit)
               )
            && !IEM_VERIFICATION_ENABLED(pVCpu)
           )
        {
            RTGCPHYS GCPhysMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtAddr, IEM_ACCESS_DATA_W, &GCPhysMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, use the IOM
             * string I/O interface to do the work.
             */
            PGMPAGEMAPLOCK PgLockMem;
            OP_TYPE *puMem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_W, (void **)&puMem, &PgLockMem);
            if (rcStrict == VINF_SUCCESS)
            {
                uint32_t cTransfers = cLeftPage;
                rcStrict = IOMIOPortReadString(pVM, pVCpu, u16Port, puMem, &cTransfers, OP_SIZE / 8);

                uint32_t cActualTransfers = cLeftPage - cTransfers;
                Assert(cActualTransfers <= cLeftPage);
                pCtx->ADDR_rDI = uAddrReg    += cbIncr * cActualTransfers;
                pCtx->ADDR_rCX = uCounterReg -= cActualTransfers;
                puMem += cActualTransfers;

                iemMemPageUnmap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_W, puMem, &PgLockMem);

                if (rcStrict != VINF_SUCCESS)
                {
                    if (IOM_SUCCESS(rcStrict))
                    {
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                        if (uCounterReg == 0)
                            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
                    }
                    return rcStrict;
                }

                /* If unaligned, we drop thru and do the page crossing access
                   below. Otherwise, do the next page. */
                if (uCounterReg == 0)
                    break;
                if (!(uVirtAddr & (OP_SIZE - 1)))
                {
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
                    continue;
                }
                cLeftPage = 0;
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         *
         * Note! We ASSUME the CPU will raise #PF or #GP before access the
         *       I/O port, otherwise it wouldn't really be restartable.
         */
        /** @todo investigate what the CPU actually does with \#PF/\#GP
         *        during INS. */
        do
        {
            OP_TYPE *puMem;
            rcStrict = iemMemMap(pVCpu, (void **)&puMem, OP_SIZE / 8, X86_SREG_ES, uAddrReg, IEM_ACCESS_DATA_W);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            uint32_t u32Value = 0;
            if (!IEM_VERIFICATION_ENABLED(pVCpu))
                rcStrict = IOMIOPortRead(pVM, pVCpu, u16Port, &u32Value, OP_SIZE / 8);
            else
                rcStrict = iemVerifyFakeIOPortRead(pVCpu, u16Port, &u32Value, OP_SIZE / 8);
            if (!IOM_SUCCESS(rcStrict))
                return rcStrict;

            *puMem = (OP_TYPE)u32Value;
# ifdef IN_RING3
            VBOXSTRICTRC rcStrict2 = iemMemCommitAndUnmap(pVCpu, puMem, IEM_ACCESS_DATA_W);
# else
            VBOXSTRICTRC rcStrict2 = iemMemCommitAndUnmapPostponeTroubleToR3(pVCpu, puMem, IEM_ACCESS_DATA_W);
# endif
            if (rcStrict2 == VINF_SUCCESS)
            { /* likely */ }
            else
                AssertLogRelMsgFailedReturn(("rcStrict2=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict2)),
                                            RT_FAILURE(rcStrict2) ? rcStrict2 : VERR_IEM_IPE_1);

            pCtx->ADDR_rDI = uAddrReg += cbIncr;
            pCtx->ADDR_rCX = --uCounterReg;

            cLeftPage--;
            if (rcStrict != VINF_SUCCESS)
            {
                if (uCounterReg == 0)
                    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
                rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                return rcStrict;
            }

            IEM_CHECK_FF_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0);
        } while ((int32_t)cLeftPage > 0);


        /*
         * Next page.  Must check for interrupts and stuff here.
         */
        if (uCounterReg == 0)
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'OUTS' (no rep)
 */
IEM_CIMPL_DEF_2(RT_CONCAT4(iemCImpl_outs_op,OP_SIZE,_addr,ADDR_SIZE), uint8_t, iEffSeg, bool, fIoChecked)
{
    PVM             pVM  = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC    rcStrict;

    /*
     * ASSUMES the #GP for I/O permission is taken first, then any #GP for
     * segmentation and finally any #PF due to virtual address translation.
     * ASSUMES nothing is read from the I/O port before traps are taken.
     */
    if (!fIoChecked)
    {
        rcStrict = iemHlpCheckPortIOPermission(pVCpu, pCtx, pCtx->dx, OP_SIZE / 8);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }

#ifdef VBOX_WITH_NESTED_HWVIRT
    /*
     * Check SVM nested-guest IO intercept.
     */
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IOIO_PROT))
    {
        rcStrict = iemSvmHandleIOIntercept(pVCpu, pCtx->dx, SVMIOIOTYPE_OUT, OP_SIZE / 8, ADDR_SIZE, iEffSeg, false /* fRep */,
                                           true /* fStrIo */, cbInstr);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("iemCImpl_outs_op: iemSvmHandleIOIntercept failed (u16Port=%#x, cbReg=%u) rc=%Rrc\n", pCtx->dx, OP_SIZE / 8,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    OP_TYPE uValue;
    rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uValue, iEffSeg, pCtx->ADDR_rSI);
    if (rcStrict == VINF_SUCCESS)
    {
        if (!IEM_VERIFICATION_ENABLED(pVCpu))
            rcStrict = IOMIOPortWrite(pVM, pVCpu, pCtx->dx, uValue, OP_SIZE / 8);
        else
            rcStrict = iemVerifyFakeIOPortWrite(pVCpu, pCtx->dx, uValue, OP_SIZE / 8);
        if (IOM_SUCCESS(rcStrict))
        {
            if (!pCtx->eflags.Bits.u1DF)
                pCtx->ADDR_rSI += OP_SIZE / 8;
            else
                pCtx->ADDR_rSI -= OP_SIZE / 8;
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
            if (rcStrict != VINF_SUCCESS)
                rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        }
    }
    return rcStrict;
}


/**
 * Implements 'REP OUTS'.
 */
IEM_CIMPL_DEF_2(RT_CONCAT4(iemCImpl_rep_outs_op,OP_SIZE,_addr,ADDR_SIZE), uint8_t, iEffSeg, bool, fIoChecked)
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PCPUMCTX    pCtx  = IEM_GET_CTX(pVCpu);

    /*
     * Setup.
     */
    uint16_t const  u16Port    = pCtx->dx;
    VBOXSTRICTRC    rcStrict;
    if (!fIoChecked)
    {
        rcStrict = iemHlpCheckPortIOPermission(pVCpu, pCtx, u16Port, OP_SIZE / 8);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }

#ifdef VBOX_WITH_NESTED_HWVIRT
    /*
     * Check SVM nested-guest IO intercept.
     */
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IOIO_PROT))
    {
        rcStrict = iemSvmHandleIOIntercept(pVCpu, u16Port, SVMIOIOTYPE_OUT, OP_SIZE / 8, ADDR_SIZE, iEffSeg, true /* fRep */,
                                           true /* fStrIo */, cbInstr);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("iemCImpl_rep_outs_op: iemSvmHandleIOIntercept failed (u16Port=%#x, cbReg=%u) rc=%Rrc\n", u16Port, OP_SIZE / 8,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    ADDR_TYPE       uCounterReg = pCtx->ADDR_rCX;
    if (uCounterReg == 0)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    PCCPUMSELREGHID pHid = iemSRegGetHid(pVCpu, iEffSeg);
    uint64_t        uBaseAddr;
    rcStrict = iemMemSegCheckReadAccessEx(pVCpu, pHid, iEffSeg, &uBaseAddr);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    int8_t const    cbIncr      = pCtx->eflags.Bits.u1DF ? -(OP_SIZE / 8) : (OP_SIZE / 8);
    ADDR_TYPE       uAddrReg    = pCtx->ADDR_rSI;

    /*
     * The loop.
     */
    for (;;)
    {
        /*
         * Do segmentation and virtual page stuff.
         */
        ADDR2_TYPE  uVirtAddr = uAddrReg + (ADDR2_TYPE)uBaseAddr;
        uint32_t    cLeftPage = (PAGE_SIZE - (uVirtAddr & PAGE_OFFSET_MASK)) / (OP_SIZE / 8);
        if (cLeftPage > uCounterReg)
            cLeftPage = uCounterReg;
        if (   cLeftPage > 0 /* can be null if unaligned, do one fallback round. */
            && cbIncr > 0    /** @todo Implement reverse direction string ops. */
            && (   IS_64_BIT_CODE(pVCpu)
                || (   uAddrReg < pHid->u32Limit
                    && uAddrReg + (cLeftPage * (OP_SIZE / 8)) <= pHid->u32Limit)
               )
            && !IEM_VERIFICATION_ENABLED(pVCpu)
           )
        {
            RTGCPHYS GCPhysMem;
            rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, uVirtAddr, IEM_ACCESS_DATA_R, &GCPhysMem);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /*
             * If we can map the page without trouble, we use the IOM
             * string I/O interface to do the job.
             */
            PGMPAGEMAPLOCK PgLockMem;
            OP_TYPE const *puMem;
            rcStrict = iemMemPageMap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, (void **)&puMem, &PgLockMem);
            if (rcStrict == VINF_SUCCESS)
            {
                uint32_t cTransfers = cLeftPage;
                rcStrict = IOMIOPortWriteString(pVM, pVCpu, u16Port, puMem, &cTransfers, OP_SIZE / 8);

                uint32_t cActualTransfers = cLeftPage - cTransfers;
                Assert(cActualTransfers <= cLeftPage);
                pCtx->ADDR_rSI = uAddrReg    += cbIncr * cActualTransfers;
                pCtx->ADDR_rCX = uCounterReg -= cActualTransfers;
                puMem += cActualTransfers;

                iemMemPageUnmap(pVCpu, GCPhysMem, IEM_ACCESS_DATA_R, puMem, &PgLockMem);

                if (rcStrict != VINF_SUCCESS)
                {
                    if (IOM_SUCCESS(rcStrict))
                    {
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                        if (uCounterReg == 0)
                            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
                    }
                    return rcStrict;
                }

                if (uCounterReg == 0)
                    break;

                /* If unaligned, we drop thru and do the page crossing access
                   below. Otherwise, do the next page. */
                if (!(uVirtAddr & (OP_SIZE - 1)))
                {
                    IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
                    continue;
                }
                cLeftPage = 0;
            }
        }

        /*
         * Fallback - slow processing till the end of the current page.
         * In the cross page boundrary case we will end up here with cLeftPage
         * as 0, we execute one loop then.
         *
         * Note! We ASSUME the CPU will raise #PF or #GP before access the
         *       I/O port, otherwise it wouldn't really be restartable.
         */
        /** @todo investigate what the CPU actually does with \#PF/\#GP
         *        during INS. */
        do
        {
            OP_TYPE uValue;
            rcStrict = RT_CONCAT(iemMemFetchDataU,OP_SIZE)(pVCpu, &uValue, iEffSeg, uAddrReg);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            if (!IEM_VERIFICATION_ENABLED(pVCpu))
                rcStrict = IOMIOPortWrite(pVM, pVCpu, u16Port, uValue, OP_SIZE / 8);
            else
                rcStrict = iemVerifyFakeIOPortWrite(pVCpu, u16Port, uValue, OP_SIZE / 8);
            if (IOM_SUCCESS(rcStrict))
            {
                pCtx->ADDR_rSI = uAddrReg += cbIncr;
                pCtx->ADDR_rCX = --uCounterReg;
                cLeftPage--;
            }
            if (rcStrict != VINF_SUCCESS)
            {
                if (IOM_SUCCESS(rcStrict))
                {
                    if (uCounterReg == 0)
                        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                }
                return rcStrict;
            }
            IEM_CHECK_FF_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN(pVM, pVCpu, uCounterReg == 0);
        } while ((int32_t)cLeftPage > 0);


        /*
         * Next page.  Must check for interrupts and stuff here.
         */
        if (uCounterReg == 0)
            break;
        IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN(pVM, pVCpu, pCtx->eflags.u);
    }

    /*
     * Done.
     */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}

#endif /* OP_SIZE != 64-bit */


#undef OP_rAX
#undef OP_SIZE
#undef ADDR_SIZE
#undef ADDR_rDI
#undef ADDR_rSI
#undef ADDR_rCX
#undef ADDR_rIP
#undef ADDR2_TYPE
#undef ADDR_TYPE
#undef ADDR2_TYPE
#undef IS_64_BIT_CODE
#undef IEM_CHECK_FF_YIELD_REPSTR_MAYBE_RETURN
#undef IEM_CHECK_FF_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN
#undef IEM_CHECK_FF_CPU_HIGH_PRIORITY_POST_REPSTR_MAYBE_RETURN


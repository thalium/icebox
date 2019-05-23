/* $Id: IEMAllCImpl.cpp.h $ */
/** @file
 * IEM - Instruction Implementation in C/C++ (code include).
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

#ifdef VBOX_WITH_NESTED_HWVIRT
# include "IEMAllCImplSvmInstr.cpp.h"
#endif

/** @name Misc Helpers
 * @{
 */


/**
 * Worker function for iemHlpCheckPortIOPermission, don't call directly.
 *
 * @returns Strict VBox status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                The register context.
 * @param   u16Port             The port number.
 * @param   cbOperand           The operand size.
 */
static VBOXSTRICTRC iemHlpCheckPortIOPermissionBitmap(PVMCPU pVCpu, PCCPUMCTX pCtx, uint16_t u16Port, uint8_t cbOperand)
{
    /* The TSS bits we're interested in are the same on 386 and AMD64. */
    AssertCompile(AMD64_SEL_TYPE_SYS_TSS_BUSY  == X86_SEL_TYPE_SYS_386_TSS_BUSY);
    AssertCompile(AMD64_SEL_TYPE_SYS_TSS_AVAIL == X86_SEL_TYPE_SYS_386_TSS_AVAIL);
    AssertCompileMembersAtSameOffset(X86TSS32, offIoBitmap, X86TSS64, offIoBitmap);
    AssertCompile(sizeof(X86TSS32) == sizeof(X86TSS64));

    /*
     * Check the TSS type, 16-bit TSSes doesn't have any I/O permission bitmap.
     */
    Assert(!pCtx->tr.Attr.n.u1DescType);
    if (RT_UNLIKELY(   pCtx->tr.Attr.n.u4Type != AMD64_SEL_TYPE_SYS_TSS_BUSY
                    && pCtx->tr.Attr.n.u4Type != AMD64_SEL_TYPE_SYS_TSS_AVAIL))
    {
        Log(("iemHlpCheckPortIOPermissionBitmap: Port=%#x cb=%d - TSS type %#x (attr=%#x) has no I/O bitmap -> #GP(0)\n",
             u16Port, cbOperand, pCtx->tr.Attr.n.u4Type, pCtx->tr.Attr.u));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Read the bitmap offset (may #PF).
     */
    uint16_t offBitmap;
    VBOXSTRICTRC rcStrict = iemMemFetchSysU16(pVCpu, &offBitmap, UINT8_MAX,
                                              pCtx->tr.u64Base + RT_UOFFSETOF(X86TSS64, offIoBitmap));
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemHlpCheckPortIOPermissionBitmap: Error reading offIoBitmap (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /*
     * The bit range from u16Port to (u16Port + cbOperand - 1), however intel
     * describes the CPU actually reading two bytes regardless of whether the
     * bit range crosses a byte boundrary.  Thus the + 1 in the test below.
     */
    uint32_t offFirstBit = (uint32_t)u16Port / 8 + offBitmap;
    /** @todo check if real CPUs ensures that offBitmap has a minimum value of
     *        for instance sizeof(X86TSS32). */
    if (offFirstBit + 1 > pCtx->tr.u32Limit) /* the limit is inclusive */
    {
        Log(("iemHlpCheckPortIOPermissionBitmap: offFirstBit=%#x + 1 is beyond u32Limit=%#x -> #GP(0)\n",
             offFirstBit, pCtx->tr.u32Limit));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Read the necessary bits.
     */
    /** @todo Test the assertion in the intel manual that the CPU reads two
     *        bytes.  The question is how this works wrt to #PF and #GP on the
     *        2nd byte when it's not required. */
    uint16_t bmBytes = UINT16_MAX;
    rcStrict = iemMemFetchSysU16(pVCpu, &bmBytes, UINT8_MAX, pCtx->tr.u64Base + offFirstBit);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemHlpCheckPortIOPermissionBitmap: Error reading I/O bitmap @%#x (%Rrc)\n", offFirstBit, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /*
     * Perform the check.
     */
    uint16_t fPortMask = (1 << cbOperand) - 1;
    bmBytes >>= (u16Port & 7);
    if (bmBytes & fPortMask)
    {
        Log(("iemHlpCheckPortIOPermissionBitmap: u16Port=%#x LB %u - access denied (bm=%#x mask=%#x) -> #GP(0)\n",
             u16Port, cbOperand, bmBytes, fPortMask));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    return VINF_SUCCESS;
}


/**
 * Checks if we are allowed to access the given I/O port, raising the
 * appropriate exceptions if we aren't (or if the I/O bitmap is not
 * accessible).
 *
 * @returns Strict VBox status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                The register context.
 * @param   u16Port             The port number.
 * @param   cbOperand           The operand size.
 */
DECLINLINE(VBOXSTRICTRC) iemHlpCheckPortIOPermission(PVMCPU pVCpu, PCCPUMCTX pCtx, uint16_t u16Port, uint8_t cbOperand)
{
    X86EFLAGS Efl;
    Efl.u = IEMMISC_GET_EFL(pVCpu, pCtx);
    if (   (pCtx->cr0 & X86_CR0_PE)
        && (    pVCpu->iem.s.uCpl > Efl.Bits.u2IOPL
            ||  Efl.Bits.u1VM) )
        return iemHlpCheckPortIOPermissionBitmap(pVCpu, pCtx, u16Port, cbOperand);
    return VINF_SUCCESS;
}


#if 0
/**
 * Calculates the parity bit.
 *
 * @returns true if the bit is set, false if not.
 * @param   u8Result            The least significant byte of the result.
 */
static bool iemHlpCalcParityFlag(uint8_t u8Result)
{
    /*
     * Parity is set if the number of bits in the least significant byte of
     * the result is even.
     */
    uint8_t cBits;
    cBits  = u8Result & 1;              /* 0 */
    u8Result >>= 1;
    cBits += u8Result & 1;
    u8Result >>= 1;
    cBits += u8Result & 1;
    u8Result >>= 1;
    cBits += u8Result & 1;
    u8Result >>= 1;
    cBits += u8Result & 1;              /* 4 */
    u8Result >>= 1;
    cBits += u8Result & 1;
    u8Result >>= 1;
    cBits += u8Result & 1;
    u8Result >>= 1;
    cBits += u8Result & 1;
    return !(cBits & 1);
}
#endif /* not used */


/**
 * Updates the specified flags according to a 8-bit result.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u8Result            The result to set the flags according to.
 * @param   fToUpdate           The flags to update.
 * @param   fUndefined          The flags that are specified as undefined.
 */
static void iemHlpUpdateArithEFlagsU8(PVMCPU pVCpu, uint8_t u8Result, uint32_t fToUpdate, uint32_t fUndefined)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    uint32_t fEFlags = pCtx->eflags.u;
    iemAImpl_test_u8(&u8Result, u8Result, &fEFlags);
    pCtx->eflags.u &= ~(fToUpdate | fUndefined);
    pCtx->eflags.u |= (fToUpdate | fUndefined) & fEFlags;
#ifdef IEM_VERIFICATION_MODE_FULL
    pVCpu->iem.s.fUndefinedEFlags |= fUndefined;
#endif
}


/**
 * Updates the specified flags according to a 16-bit result.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16Result           The result to set the flags according to.
 * @param   fToUpdate           The flags to update.
 * @param   fUndefined          The flags that are specified as undefined.
 */
static void iemHlpUpdateArithEFlagsU16(PVMCPU pVCpu, uint16_t u16Result, uint32_t fToUpdate, uint32_t fUndefined)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    uint32_t fEFlags = pCtx->eflags.u;
    iemAImpl_test_u16(&u16Result, u16Result, &fEFlags);
    pCtx->eflags.u &= ~(fToUpdate | fUndefined);
    pCtx->eflags.u |= (fToUpdate | fUndefined) & fEFlags;
#ifdef IEM_VERIFICATION_MODE_FULL
    pVCpu->iem.s.fUndefinedEFlags |= fUndefined;
#endif
}


/**
 * Helper used by iret.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uCpl                The new CPL.
 * @param   pSReg               Pointer to the segment register.
 */
static void iemHlpAdjustSelectorForNewCpl(PVMCPU pVCpu, uint8_t uCpl, PCPUMSELREG pSReg)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg))
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, pSReg);
#else
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
#endif

    if (   uCpl > pSReg->Attr.n.u2Dpl
        && pSReg->Attr.n.u1DescType /* code or data, not system */
        &&    (pSReg->Attr.n.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
           !=                         (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF)) /* not conforming code */
        iemHlpLoadNullDataSelectorProt(pVCpu, pSReg, 0);
}


/**
 * Indicates that we have modified the FPU state.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemHlpUsedFpu(PVMCPU pVCpu)
{
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
}

/** @} */

/** @name C Implementations
 * @{
 */

/**
 * Implements a 16-bit popa.
 */
IEM_CIMPL_DEF_0(iemCImpl_popa_16)
{
    PCPUMCTX        pCtx        = IEM_GET_CTX(pVCpu);
    RTGCPTR         GCPtrStart  = iemRegGetEffRsp(pVCpu, pCtx);
    RTGCPTR         GCPtrLast   = GCPtrStart + 15;
    VBOXSTRICTRC    rcStrict;

    /*
     * The docs are a bit hard to comprehend here, but it looks like we wrap
     * around in real mode as long as none of the individual "popa" crosses the
     * end of the stack segment.  In protected mode we check the whole access
     * in one go.  For efficiency, only do the word-by-word thing if we're in
     * danger of wrapping around.
     */
    /** @todo do popa boundary / wrap-around checks.  */
    if (RT_UNLIKELY(   IEM_IS_REAL_OR_V86_MODE(pVCpu)
                    && (pCtx->cs.u32Limit < GCPtrLast)) ) /* ASSUMES 64-bit RTGCPTR */
    {
        /* word-by-word */
        RTUINT64U TmpRsp;
        TmpRsp.u = pCtx->rsp;
        rcStrict = iemMemStackPopU16Ex(pVCpu, &pCtx->di, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU16Ex(pVCpu, &pCtx->si, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU16Ex(pVCpu, &pCtx->bp, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
        {
            iemRegAddToRspEx(pVCpu, pCtx, &TmpRsp, 2); /* sp */
            rcStrict = iemMemStackPopU16Ex(pVCpu, &pCtx->bx, &TmpRsp);
        }
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU16Ex(pVCpu, &pCtx->dx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU16Ex(pVCpu, &pCtx->cx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU16Ex(pVCpu, &pCtx->ax, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
        {
            pCtx->rsp = TmpRsp.u;
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
    }
    else
    {
        uint16_t const *pa16Mem = NULL;
        rcStrict = iemMemMap(pVCpu, (void **)&pa16Mem, 16, X86_SREG_SS, GCPtrStart, IEM_ACCESS_STACK_R);
        if (rcStrict == VINF_SUCCESS)
        {
            pCtx->di = pa16Mem[7 - X86_GREG_xDI];
            pCtx->si = pa16Mem[7 - X86_GREG_xSI];
            pCtx->bp = pa16Mem[7 - X86_GREG_xBP];
            /* skip sp */
            pCtx->bx = pa16Mem[7 - X86_GREG_xBX];
            pCtx->dx = pa16Mem[7 - X86_GREG_xDX];
            pCtx->cx = pa16Mem[7 - X86_GREG_xCX];
            pCtx->ax = pa16Mem[7 - X86_GREG_xAX];
            rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)pa16Mem, IEM_ACCESS_STACK_R);
            if (rcStrict == VINF_SUCCESS)
            {
                iemRegAddToRsp(pVCpu, pCtx, 16);
                iemRegAddToRipAndClearRF(pVCpu, cbInstr);
            }
        }
    }
    return rcStrict;
}


/**
 * Implements a 32-bit popa.
 */
IEM_CIMPL_DEF_0(iemCImpl_popa_32)
{
    PCPUMCTX        pCtx        = IEM_GET_CTX(pVCpu);
    RTGCPTR         GCPtrStart  = iemRegGetEffRsp(pVCpu, pCtx);
    RTGCPTR         GCPtrLast   = GCPtrStart + 31;
    VBOXSTRICTRC    rcStrict;

    /*
     * The docs are a bit hard to comprehend here, but it looks like we wrap
     * around in real mode as long as none of the individual "popa" crosses the
     * end of the stack segment.  In protected mode we check the whole access
     * in one go.  For efficiency, only do the word-by-word thing if we're in
     * danger of wrapping around.
     */
    /** @todo do popa boundary / wrap-around checks.  */
    if (RT_UNLIKELY(   IEM_IS_REAL_OR_V86_MODE(pVCpu)
                    && (pCtx->cs.u32Limit < GCPtrLast)) ) /* ASSUMES 64-bit RTGCPTR */
    {
        /* word-by-word */
        RTUINT64U TmpRsp;
        TmpRsp.u = pCtx->rsp;
        rcStrict = iemMemStackPopU32Ex(pVCpu, &pCtx->edi, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU32Ex(pVCpu, &pCtx->esi, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU32Ex(pVCpu, &pCtx->ebp, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
        {
            iemRegAddToRspEx(pVCpu, pCtx, &TmpRsp, 2); /* sp */
            rcStrict = iemMemStackPopU32Ex(pVCpu, &pCtx->ebx, &TmpRsp);
        }
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU32Ex(pVCpu, &pCtx->edx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU32Ex(pVCpu, &pCtx->ecx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPopU32Ex(pVCpu, &pCtx->eax, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
        {
#if 1  /** @todo what actually happens with the high bits when we're in 16-bit mode? */
            pCtx->rdi &= UINT32_MAX;
            pCtx->rsi &= UINT32_MAX;
            pCtx->rbp &= UINT32_MAX;
            pCtx->rbx &= UINT32_MAX;
            pCtx->rdx &= UINT32_MAX;
            pCtx->rcx &= UINT32_MAX;
            pCtx->rax &= UINT32_MAX;
#endif
            pCtx->rsp = TmpRsp.u;
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
    }
    else
    {
        uint32_t const *pa32Mem;
        rcStrict = iemMemMap(pVCpu, (void **)&pa32Mem, 32, X86_SREG_SS, GCPtrStart, IEM_ACCESS_STACK_R);
        if (rcStrict == VINF_SUCCESS)
        {
            pCtx->rdi = pa32Mem[7 - X86_GREG_xDI];
            pCtx->rsi = pa32Mem[7 - X86_GREG_xSI];
            pCtx->rbp = pa32Mem[7 - X86_GREG_xBP];
            /* skip esp */
            pCtx->rbx = pa32Mem[7 - X86_GREG_xBX];
            pCtx->rdx = pa32Mem[7 - X86_GREG_xDX];
            pCtx->rcx = pa32Mem[7 - X86_GREG_xCX];
            pCtx->rax = pa32Mem[7 - X86_GREG_xAX];
            rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)pa32Mem, IEM_ACCESS_STACK_R);
            if (rcStrict == VINF_SUCCESS)
            {
                iemRegAddToRsp(pVCpu, pCtx, 32);
                iemRegAddToRipAndClearRF(pVCpu, cbInstr);
            }
        }
    }
    return rcStrict;
}


/**
 * Implements a 16-bit pusha.
 */
IEM_CIMPL_DEF_0(iemCImpl_pusha_16)
{
    PCPUMCTX        pCtx        = IEM_GET_CTX(pVCpu);
    RTGCPTR         GCPtrTop    = iemRegGetEffRsp(pVCpu, pCtx);
    RTGCPTR         GCPtrBottom = GCPtrTop - 15;
    VBOXSTRICTRC    rcStrict;

    /*
     * The docs are a bit hard to comprehend here, but it looks like we wrap
     * around in real mode as long as none of the individual "pushd" crosses the
     * end of the stack segment.  In protected mode we check the whole access
     * in one go.  For efficiency, only do the word-by-word thing if we're in
     * danger of wrapping around.
     */
    /** @todo do pusha boundary / wrap-around checks.  */
    if (RT_UNLIKELY(   GCPtrBottom > GCPtrTop
                    && IEM_IS_REAL_OR_V86_MODE(pVCpu) ) )
    {
        /* word-by-word */
        RTUINT64U TmpRsp;
        TmpRsp.u = pCtx->rsp;
        rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->ax, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->cx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->dx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->bx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->sp, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->bp, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->si, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU16Ex(pVCpu, pCtx->di, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
        {
            pCtx->rsp = TmpRsp.u;
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
    }
    else
    {
        GCPtrBottom--;
        uint16_t *pa16Mem = NULL;
        rcStrict = iemMemMap(pVCpu, (void **)&pa16Mem, 16, X86_SREG_SS, GCPtrBottom, IEM_ACCESS_STACK_W);
        if (rcStrict == VINF_SUCCESS)
        {
            pa16Mem[7 - X86_GREG_xDI] = pCtx->di;
            pa16Mem[7 - X86_GREG_xSI] = pCtx->si;
            pa16Mem[7 - X86_GREG_xBP] = pCtx->bp;
            pa16Mem[7 - X86_GREG_xSP] = pCtx->sp;
            pa16Mem[7 - X86_GREG_xBX] = pCtx->bx;
            pa16Mem[7 - X86_GREG_xDX] = pCtx->dx;
            pa16Mem[7 - X86_GREG_xCX] = pCtx->cx;
            pa16Mem[7 - X86_GREG_xAX] = pCtx->ax;
            rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)pa16Mem, IEM_ACCESS_STACK_W);
            if (rcStrict == VINF_SUCCESS)
            {
                iemRegSubFromRsp(pVCpu, pCtx, 16);
                iemRegAddToRipAndClearRF(pVCpu, cbInstr);
            }
        }
    }
    return rcStrict;
}


/**
 * Implements a 32-bit pusha.
 */
IEM_CIMPL_DEF_0(iemCImpl_pusha_32)
{
    PCPUMCTX        pCtx        = IEM_GET_CTX(pVCpu);
    RTGCPTR         GCPtrTop    = iemRegGetEffRsp(pVCpu, pCtx);
    RTGCPTR         GCPtrBottom = GCPtrTop - 31;
    VBOXSTRICTRC    rcStrict;

    /*
     * The docs are a bit hard to comprehend here, but it looks like we wrap
     * around in real mode as long as none of the individual "pusha" crosses the
     * end of the stack segment.  In protected mode we check the whole access
     * in one go.  For efficiency, only do the word-by-word thing if we're in
     * danger of wrapping around.
     */
    /** @todo do pusha boundary / wrap-around checks.  */
    if (RT_UNLIKELY(   GCPtrBottom > GCPtrTop
                    && IEM_IS_REAL_OR_V86_MODE(pVCpu) ) )
    {
        /* word-by-word */
        RTUINT64U TmpRsp;
        TmpRsp.u = pCtx->rsp;
        rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->eax, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->ecx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->edx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->ebx, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->esp, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->ebp, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->esi, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemStackPushU32Ex(pVCpu, pCtx->edi, &TmpRsp);
        if (rcStrict == VINF_SUCCESS)
        {
            pCtx->rsp = TmpRsp.u;
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
    }
    else
    {
        GCPtrBottom--;
        uint32_t *pa32Mem;
        rcStrict = iemMemMap(pVCpu, (void **)&pa32Mem, 32, X86_SREG_SS, GCPtrBottom, IEM_ACCESS_STACK_W);
        if (rcStrict == VINF_SUCCESS)
        {
            pa32Mem[7 - X86_GREG_xDI] = pCtx->edi;
            pa32Mem[7 - X86_GREG_xSI] = pCtx->esi;
            pa32Mem[7 - X86_GREG_xBP] = pCtx->ebp;
            pa32Mem[7 - X86_GREG_xSP] = pCtx->esp;
            pa32Mem[7 - X86_GREG_xBX] = pCtx->ebx;
            pa32Mem[7 - X86_GREG_xDX] = pCtx->edx;
            pa32Mem[7 - X86_GREG_xCX] = pCtx->ecx;
            pa32Mem[7 - X86_GREG_xAX] = pCtx->eax;
            rcStrict = iemMemCommitAndUnmap(pVCpu, pa32Mem, IEM_ACCESS_STACK_W);
            if (rcStrict == VINF_SUCCESS)
            {
                iemRegSubFromRsp(pVCpu, pCtx, 32);
                iemRegAddToRipAndClearRF(pVCpu, cbInstr);
            }
        }
    }
    return rcStrict;
}


/**
 * Implements pushf.
 *
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_pushf, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC rcStrict;

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_PUSHF))
    {
        Log2(("pushf: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_PUSHF, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * If we're in V8086 mode some care is required (which is why we're in
     * doing this in a C implementation).
     */
    uint32_t fEfl = IEMMISC_GET_EFL(pVCpu, pCtx);
    if (   (fEfl & X86_EFL_VM)
        && X86_EFL_GET_IOPL(fEfl) != 3 )
    {
        Assert(pCtx->cr0 & X86_CR0_PE);
        if (   enmEffOpSize != IEMMODE_16BIT
            || !(pCtx->cr4 & X86_CR4_VME))
            return iemRaiseGeneralProtectionFault0(pVCpu);
        fEfl &= ~X86_EFL_IF;          /* (RF and VM are out of range) */
        fEfl |= (fEfl & X86_EFL_VIF) >> (19 - 9);
        rcStrict = iemMemStackPushU16(pVCpu, (uint16_t)fEfl);
    }
    else
    {

        /*
         * Ok, clear RF and VM, adjust for ancient CPUs, and push the flags.
         */
        fEfl &= ~(X86_EFL_RF | X86_EFL_VM);

        switch (enmEffOpSize)
        {
            case IEMMODE_16BIT:
                AssertCompile(IEMTARGETCPU_8086 <= IEMTARGETCPU_186 && IEMTARGETCPU_V20 <= IEMTARGETCPU_186 && IEMTARGETCPU_286 > IEMTARGETCPU_186);
                if (IEM_GET_TARGET_CPU(pVCpu) <= IEMTARGETCPU_186)
                    fEfl |= UINT16_C(0xf000);
                rcStrict = iemMemStackPushU16(pVCpu, (uint16_t)fEfl);
                break;
            case IEMMODE_32BIT:
                rcStrict = iemMemStackPushU32(pVCpu, fEfl);
                break;
            case IEMMODE_64BIT:
                rcStrict = iemMemStackPushU64(pVCpu, fEfl);
                break;
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements popf.
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_popf, IEMMODE, enmEffOpSize)
{
    PCPUMCTX        pCtx    = IEM_GET_CTX(pVCpu);
    uint32_t const  fEflOld = IEMMISC_GET_EFL(pVCpu, pCtx);
    VBOXSTRICTRC    rcStrict;
    uint32_t        fEflNew;

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_POPF))
    {
        Log2(("popf: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_POPF, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * V8086 is special as usual.
     */
    if (fEflOld & X86_EFL_VM)
    {
        /*
         * Almost anything goes if IOPL is 3.
         */
        if (X86_EFL_GET_IOPL(fEflOld) == 3)
        {
            switch (enmEffOpSize)
            {
                case IEMMODE_16BIT:
                {
                    uint16_t u16Value;
                    rcStrict = iemMemStackPopU16(pVCpu, &u16Value);
                    if (rcStrict != VINF_SUCCESS)
                        return rcStrict;
                    fEflNew = u16Value | (fEflOld & UINT32_C(0xffff0000));
                    break;
                }
                case IEMMODE_32BIT:
                    rcStrict = iemMemStackPopU32(pVCpu, &fEflNew);
                    if (rcStrict != VINF_SUCCESS)
                        return rcStrict;
                    break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }

            const uint32_t fPopfBits = pVCpu->CTX_SUFF(pVM)->cpum.ro.GuestFeatures.enmMicroarch != kCpumMicroarch_Intel_80386
                                     ? X86_EFL_POPF_BITS : X86_EFL_POPF_BITS_386;
            fEflNew &=   fPopfBits & ~(X86_EFL_IOPL);
            fEflNew |= ~(fPopfBits & ~(X86_EFL_IOPL)) & fEflOld;
        }
        /*
         * Interrupt flag virtualization with CR4.VME=1.
         */
        else if (   enmEffOpSize == IEMMODE_16BIT
                 && (pCtx->cr4 & X86_CR4_VME) )
        {
            uint16_t    u16Value;
            RTUINT64U   TmpRsp;
            TmpRsp.u = pCtx->rsp;
            rcStrict = iemMemStackPopU16Ex(pVCpu, &u16Value, &TmpRsp);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /** @todo Is the popf VME #GP(0) delivered after updating RSP+RIP
             *        or before? */
            if (    (   (u16Value & X86_EFL_IF)
                     && (fEflOld  & X86_EFL_VIP))
                ||  (u16Value & X86_EFL_TF) )
                return iemRaiseGeneralProtectionFault0(pVCpu);

            fEflNew = u16Value | (fEflOld & UINT32_C(0xffff0000) & ~X86_EFL_VIF);
            fEflNew |= (fEflNew & X86_EFL_IF) << (19 - 9);
            fEflNew &=   X86_EFL_POPF_BITS & ~(X86_EFL_IOPL | X86_EFL_IF);
            fEflNew |= ~(X86_EFL_POPF_BITS & ~(X86_EFL_IOPL | X86_EFL_IF)) & fEflOld;

            pCtx->rsp = TmpRsp.u;
        }
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);

    }
    /*
     * Not in V8086 mode.
     */
    else
    {
        /* Pop the flags. */
        switch (enmEffOpSize)
        {
            case IEMMODE_16BIT:
            {
                uint16_t u16Value;
                rcStrict = iemMemStackPopU16(pVCpu, &u16Value);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;
                fEflNew = u16Value | (fEflOld & UINT32_C(0xffff0000));

                /*
                 * Ancient CPU adjustments:
                 *  - 8086, 80186, V20/30:
                 *    Fixed bits 15:12 bits are not kept correctly internally, mostly for
                 *    practical reasons (masking below).  We add them when pushing flags.
                 *  - 80286:
                 *    The NT and IOPL flags cannot be popped from real mode and are
                 *    therefore always zero (since a 286 can never exit from PM and
                 *    their initial value is zero).  This changed on a 386 and can
                 *    therefore be used to detect 286 or 386 CPU in real mode.
                 */
                if (   IEM_GET_TARGET_CPU(pVCpu) == IEMTARGETCPU_286
                    && !(pCtx->cr0 & X86_CR0_PE) )
                    fEflNew &= ~(X86_EFL_NT | X86_EFL_IOPL);
                break;
            }
            case IEMMODE_32BIT:
                rcStrict = iemMemStackPopU32(pVCpu, &fEflNew);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;
                break;
            case IEMMODE_64BIT:
            {
                uint64_t u64Value;
                rcStrict = iemMemStackPopU64(pVCpu, &u64Value);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;
                fEflNew = u64Value;  /** @todo testcase: Check exactly what happens if high bits are set. */
                break;
            }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }

        /* Merge them with the current flags. */
        const uint32_t fPopfBits = pVCpu->CTX_SUFF(pVM)->cpum.ro.GuestFeatures.enmMicroarch != kCpumMicroarch_Intel_80386
                                 ? X86_EFL_POPF_BITS : X86_EFL_POPF_BITS_386;
        if (   (fEflNew & (X86_EFL_IOPL | X86_EFL_IF)) == (fEflOld & (X86_EFL_IOPL | X86_EFL_IF))
            || pVCpu->iem.s.uCpl == 0)
        {
            fEflNew &=  fPopfBits;
            fEflNew |= ~fPopfBits & fEflOld;
        }
        else if (pVCpu->iem.s.uCpl <= X86_EFL_GET_IOPL(fEflOld))
        {
            fEflNew &=   fPopfBits & ~(X86_EFL_IOPL);
            fEflNew |= ~(fPopfBits & ~(X86_EFL_IOPL)) & fEflOld;
        }
        else
        {
            fEflNew &=   fPopfBits & ~(X86_EFL_IOPL | X86_EFL_IF);
            fEflNew |= ~(fPopfBits & ~(X86_EFL_IOPL | X86_EFL_IF)) & fEflOld;
        }
    }

    /*
     * Commit the flags.
     */
    Assert(fEflNew & RT_BIT_32(1));
    IEMMISC_SET_EFL(pVCpu, pCtx, fEflNew);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);

    return VINF_SUCCESS;
}


/**
 * Implements an indirect call.
 *
 * @param   uNewPC          The new program counter (RIP) value (loaded from the
 *                          operand).
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_call_16, uint16_t, uNewPC)
{
    PCPUMCTX pCtx   = IEM_GET_CTX(pVCpu);
    uint16_t uOldPC = pCtx->ip + cbInstr;
    if (uNewPC > pCtx->cs.u32Limit)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    VBOXSTRICTRC rcStrict = iemMemStackPushU16(pVCpu, uOldPC);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    pCtx->rip = uNewPC;
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif
    return VINF_SUCCESS;
}


/**
 * Implements a 16-bit relative call.
 *
 * @param   offDisp      The displacment offset.
 */
IEM_CIMPL_DEF_1(iemCImpl_call_rel_16, int16_t, offDisp)
{
    PCPUMCTX pCtx   = IEM_GET_CTX(pVCpu);
    uint16_t uOldPC = pCtx->ip + cbInstr;
    uint16_t uNewPC = uOldPC + offDisp;
    if (uNewPC > pCtx->cs.u32Limit)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    VBOXSTRICTRC rcStrict = iemMemStackPushU16(pVCpu, uOldPC);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    pCtx->rip = uNewPC;
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif
    return VINF_SUCCESS;
}


/**
 * Implements a 32-bit indirect call.
 *
 * @param   uNewPC          The new program counter (RIP) value (loaded from the
 *                          operand).
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_call_32, uint32_t, uNewPC)
{
    PCPUMCTX pCtx   = IEM_GET_CTX(pVCpu);
    uint32_t uOldPC = pCtx->eip + cbInstr;
    if (uNewPC > pCtx->cs.u32Limit)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    VBOXSTRICTRC rcStrict = iemMemStackPushU32(pVCpu, uOldPC);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

#if defined(IN_RING3) && defined(VBOX_WITH_RAW_MODE) && defined(VBOX_WITH_CALL_RECORD)
    /*
     * CASM hook for recording interesting indirect calls.
     */
    if (   !pCtx->eflags.Bits.u1IF
        && (pCtx->cr0 & X86_CR0_PG)
        && !CSAMIsEnabled(pVCpu->CTX_SUFF(pVM))
        && pVCpu->iem.s.uCpl == 0)
    {
        EMSTATE enmState = EMGetState(pVCpu);
        if (   enmState == EMSTATE_IEM_THEN_REM
            || enmState == EMSTATE_IEM
            || enmState == EMSTATE_REM)
            CSAMR3RecordCallAddress(pVCpu->CTX_SUFF(pVM), pCtx->eip);
    }
#endif

    pCtx->rip = uNewPC;
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif
    return VINF_SUCCESS;
}


/**
 * Implements a 32-bit relative call.
 *
 * @param   offDisp      The displacment offset.
 */
IEM_CIMPL_DEF_1(iemCImpl_call_rel_32, int32_t, offDisp)
{
    PCPUMCTX pCtx   = IEM_GET_CTX(pVCpu);
    uint32_t uOldPC = pCtx->eip + cbInstr;
    uint32_t uNewPC = uOldPC + offDisp;
    if (uNewPC > pCtx->cs.u32Limit)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    VBOXSTRICTRC rcStrict = iemMemStackPushU32(pVCpu, uOldPC);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    pCtx->rip = uNewPC;
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif
    return VINF_SUCCESS;
}


/**
 * Implements a 64-bit indirect call.
 *
 * @param   uNewPC          The new program counter (RIP) value (loaded from the
 *                          operand).
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_call_64, uint64_t, uNewPC)
{
    PCPUMCTX pCtx   = IEM_GET_CTX(pVCpu);
    uint64_t uOldPC = pCtx->rip + cbInstr;
    if (!IEM_IS_CANONICAL(uNewPC))
        return iemRaiseGeneralProtectionFault0(pVCpu);

    VBOXSTRICTRC rcStrict = iemMemStackPushU64(pVCpu, uOldPC);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    pCtx->rip = uNewPC;
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif
    return VINF_SUCCESS;
}


/**
 * Implements a 64-bit relative call.
 *
 * @param   offDisp      The displacment offset.
 */
IEM_CIMPL_DEF_1(iemCImpl_call_rel_64, int64_t, offDisp)
{
    PCPUMCTX pCtx   = IEM_GET_CTX(pVCpu);
    uint64_t uOldPC = pCtx->rip + cbInstr;
    uint64_t uNewPC = uOldPC + offDisp;
    if (!IEM_IS_CANONICAL(uNewPC))
        return iemRaiseNotCanonical(pVCpu);

    VBOXSTRICTRC rcStrict = iemMemStackPushU64(pVCpu, uOldPC);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    pCtx->rip = uNewPC;
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Implements far jumps and calls thru task segments (TSS).
 *
 * @param   uSel            The selector.
 * @param   enmBranch       The kind of branching we're performing.
 * @param   enmEffOpSize    The effective operand size.
 * @param   pDesc           The descriptor corresponding to @a uSel. The type is
 *                          task gate.
 */
IEM_CIMPL_DEF_4(iemCImpl_BranchTaskSegment, uint16_t, uSel, IEMBRANCH, enmBranch, IEMMODE, enmEffOpSize, PIEMSELDESC, pDesc)
{
#ifndef IEM_IMPLEMENTS_TASKSWITCH
    IEM_RETURN_ASPECT_NOT_IMPLEMENTED();
#else
    Assert(enmBranch == IEMBRANCH_JUMP || enmBranch == IEMBRANCH_CALL);
    Assert(   pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_TSS_AVAIL
           || pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_TSS_AVAIL);
    RT_NOREF_PV(enmEffOpSize);

    if (   pDesc->Legacy.Gate.u2Dpl < pVCpu->iem.s.uCpl
        || pDesc->Legacy.Gate.u2Dpl < (uSel & X86_SEL_RPL))
    {
        Log(("BranchTaskSegment invalid priv. uSel=%04x TSS DPL=%d CPL=%u Sel RPL=%u -> #GP\n", uSel, pDesc->Legacy.Gate.u2Dpl,
             pVCpu->iem.s.uCpl, (uSel & X86_SEL_RPL)));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    /** @todo This is checked earlier for far jumps (see iemCImpl_FarJmp) but not
     *        far calls (see iemCImpl_callf). Most likely in both cases it should be
     *        checked here, need testcases. */
    if (!pDesc->Legacy.Gen.u1Present)
    {
        Log(("BranchTaskSegment TSS not present uSel=%04x -> #NP\n", uSel));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    PCPUMCTX pCtx     = IEM_GET_CTX(pVCpu);
    uint32_t uNextEip = pCtx->eip + cbInstr;
    return iemTaskSwitch(pVCpu, pCtx, enmBranch == IEMBRANCH_JUMP ? IEMTASKSWITCH_JUMP : IEMTASKSWITCH_CALL,
                         uNextEip, 0 /* fFlags */, 0 /* uErr */, 0 /* uCr2 */, uSel, pDesc);
#endif
}


/**
 * Implements far jumps and calls thru task gates.
 *
 * @param   uSel            The selector.
 * @param   enmBranch       The kind of branching we're performing.
 * @param   enmEffOpSize    The effective operand size.
 * @param   pDesc           The descriptor corresponding to @a uSel. The type is
 *                          task gate.
 */
IEM_CIMPL_DEF_4(iemCImpl_BranchTaskGate, uint16_t, uSel, IEMBRANCH, enmBranch, IEMMODE, enmEffOpSize, PIEMSELDESC, pDesc)
{
#ifndef IEM_IMPLEMENTS_TASKSWITCH
    IEM_RETURN_ASPECT_NOT_IMPLEMENTED();
#else
    Assert(enmBranch == IEMBRANCH_JUMP || enmBranch == IEMBRANCH_CALL);
    RT_NOREF_PV(enmEffOpSize);

    if (   pDesc->Legacy.Gate.u2Dpl < pVCpu->iem.s.uCpl
        || pDesc->Legacy.Gate.u2Dpl < (uSel & X86_SEL_RPL))
    {
        Log(("BranchTaskGate invalid priv. uSel=%04x TSS DPL=%d CPL=%u Sel RPL=%u -> #GP\n", uSel, pDesc->Legacy.Gate.u2Dpl,
             pVCpu->iem.s.uCpl, (uSel & X86_SEL_RPL)));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    /** @todo This is checked earlier for far jumps (see iemCImpl_FarJmp) but not
     *        far calls (see iemCImpl_callf). Most likely in both cases it should be
     *        checked here, need testcases. */
    if (!pDesc->Legacy.Gen.u1Present)
    {
        Log(("BranchTaskSegment segment not present uSel=%04x -> #NP\n", uSel));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    /*
     * Fetch the new TSS descriptor from the GDT.
     */
    RTSEL uSelTss = pDesc->Legacy.Gate.u16Sel;
    if (uSelTss  & X86_SEL_LDT)
    {
        Log(("BranchTaskGate TSS is in LDT. uSel=%04x uSelTss=%04x -> #GP\n", uSel, uSelTss));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    IEMSELDESC TssDesc;
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, &TssDesc, uSelTss, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    if (TssDesc.Legacy.Gate.u4Type & X86_SEL_TYPE_SYS_TSS_BUSY_MASK)
    {
        Log(("BranchTaskGate TSS is busy. uSel=%04x uSelTss=%04x DescType=%#x -> #GP\n", uSel, uSelTss,
             TssDesc.Legacy.Gate.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    if (!TssDesc.Legacy.Gate.u1Present)
    {
        Log(("BranchTaskGate TSS is not present. uSel=%04x uSelTss=%04x -> #NP\n", uSel, uSelTss));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSelTss & X86_SEL_MASK_OFF_RPL);
    }

    PCPUMCTX pCtx     = IEM_GET_CTX(pVCpu);
    uint32_t uNextEip = pCtx->eip + cbInstr;
    return iemTaskSwitch(pVCpu, pCtx, enmBranch == IEMBRANCH_JUMP ? IEMTASKSWITCH_JUMP : IEMTASKSWITCH_CALL,
                         uNextEip, 0 /* fFlags */, 0 /* uErr */, 0 /* uCr2 */, uSelTss, &TssDesc);
#endif
}


/**
 * Implements far jumps and calls thru call gates.
 *
 * @param   uSel            The selector.
 * @param   enmBranch       The kind of branching we're performing.
 * @param   enmEffOpSize    The effective operand size.
 * @param   pDesc           The descriptor corresponding to @a uSel. The type is
 *                          call gate.
 */
IEM_CIMPL_DEF_4(iemCImpl_BranchCallGate, uint16_t, uSel, IEMBRANCH, enmBranch, IEMMODE, enmEffOpSize, PIEMSELDESC, pDesc)
{
#define IEM_IMPLEMENTS_CALLGATE
#ifndef IEM_IMPLEMENTS_CALLGATE
    IEM_RETURN_ASPECT_NOT_IMPLEMENTED();
#else
    RT_NOREF_PV(enmEffOpSize);

    /* NB: Far jumps can only do intra-privilege transfers. Far calls support
     * inter-privilege calls and are much more complex.
     *
     * NB: 64-bit call gate has the same type as a 32-bit call gate! If
     * EFER.LMA=1, the gate must be 64-bit. Conversely if EFER.LMA=0, the gate
     * must be 16-bit or 32-bit.
     */
    /** @todo: effective operand size is probably irrelevant here, only the
     *         call gate bitness matters??
     */
    VBOXSTRICTRC    rcStrict;
    RTPTRUNION      uPtrRet;
    uint64_t        uNewRsp;
    uint64_t        uNewRip;
    uint64_t        u64Base;
    uint32_t        cbLimit;
    RTSEL           uNewCS;
    IEMSELDESC      DescCS;

    AssertCompile(X86_SEL_TYPE_SYS_386_CALL_GATE == AMD64_SEL_TYPE_SYS_CALL_GATE);
    Assert(enmBranch == IEMBRANCH_JUMP || enmBranch == IEMBRANCH_CALL);
    Assert(   pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_CALL_GATE
           || pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_CALL_GATE);

    /* Determine the new instruction pointer from the gate descriptor. */
    uNewRip = pDesc->Legacy.Gate.u16OffsetLow
            | ((uint32_t)pDesc->Legacy.Gate.u16OffsetHigh << 16)
            | ((uint64_t)pDesc->Long.Gate.u32OffsetTop    << 32);

    /* Perform DPL checks on the gate descriptor. */
    if (   pDesc->Legacy.Gate.u2Dpl < pVCpu->iem.s.uCpl
        || pDesc->Legacy.Gate.u2Dpl < (uSel & X86_SEL_RPL))
    {
        Log(("BranchCallGate invalid priv. uSel=%04x Gate DPL=%d CPL=%u Sel RPL=%u -> #GP\n", uSel, pDesc->Legacy.Gate.u2Dpl,
             pVCpu->iem.s.uCpl, (uSel & X86_SEL_RPL)));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
    }

    /** @todo does this catch NULL selectors, too? */
    if (!pDesc->Legacy.Gen.u1Present)
    {
        Log(("BranchCallGate Gate not present uSel=%04x -> #NP\n", uSel));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSel);
    }

    /*
     * Fetch the target CS descriptor from the GDT or LDT.
     */
    uNewCS = pDesc->Legacy.Gate.u16Sel;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, uNewCS, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Target CS must be a code selector. */
    if (   !DescCS.Legacy.Gen.u1DescType
        || !(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE) )
    {
        Log(("BranchCallGate %04x:%08RX64 -> not a code selector (u1DescType=%u u4Type=%#x).\n",
             uNewCS, uNewRip, DescCS.Legacy.Gen.u1DescType, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCS);
    }

    /* Privilege checks on target CS. */
    if (enmBranch == IEMBRANCH_JUMP)
    {
        if (DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
        {
            if (DescCS.Legacy.Gen.u2Dpl > pVCpu->iem.s.uCpl)
            {
                Log(("BranchCallGate jump (conforming) bad DPL uNewCS=%04x Gate DPL=%d CPL=%u -> #GP\n",
                     uNewCS, DescCS.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCS);
            }
        }
        else
        {
            if (DescCS.Legacy.Gen.u2Dpl != pVCpu->iem.s.uCpl)
            {
                Log(("BranchCallGate jump (non-conforming) bad DPL uNewCS=%04x Gate DPL=%d CPL=%u -> #GP\n",
                     uNewCS, DescCS.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCS);
            }
        }
    }
    else
    {
        Assert(enmBranch == IEMBRANCH_CALL);
        if (DescCS.Legacy.Gen.u2Dpl > pVCpu->iem.s.uCpl)
        {
            Log(("BranchCallGate call invalid priv. uNewCS=%04x Gate DPL=%d CPL=%u -> #GP\n",
                 uNewCS, DescCS.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }
    }

    /* Additional long mode checks. */
    if (IEM_IS_LONG_MODE(pVCpu))
    {
        if (!DescCS.Legacy.Gen.u1Long)
        {
            Log(("BranchCallGate uNewCS %04x -> not a 64-bit code segment.\n", uNewCS));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCS);
        }

        /* L vs D. */
        if (   DescCS.Legacy.Gen.u1Long
            && DescCS.Legacy.Gen.u1DefBig)
        {
            Log(("BranchCallGate uNewCS %04x -> both L and D are set.\n", uNewCS));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCS);
        }
    }

    if (!DescCS.Legacy.Gate.u1Present)
    {
        Log(("BranchCallGate target CS is not present. uSel=%04x uNewCS=%04x -> #NP(CS)\n", uSel, uNewCS));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uNewCS);
    }

    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    if (enmBranch == IEMBRANCH_JUMP)
    {
        /** @todo: This is very similar to regular far jumps; merge! */
        /* Jumps are fairly simple... */

        /* Chop the high bits off if 16-bit gate (Intel says so). */
        if (pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_CALL_GATE)
            uNewRip = (uint16_t)uNewRip;

        /* Limit check for non-long segments. */
        cbLimit = X86DESC_LIMIT_G(&DescCS.Legacy);
        if (DescCS.Legacy.Gen.u1Long)
            u64Base = 0;
        else
        {
            if (uNewRip > cbLimit)
            {
                Log(("BranchCallGate jump %04x:%08RX64 -> out of bounds (%#x) -> #GP(0)\n", uNewCS, uNewRip, cbLimit));
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, 0);
            }
            u64Base = X86DESC_BASE(&DescCS.Legacy);
        }

        /* Canonical address check. */
        if (!IEM_IS_CANONICAL(uNewRip))
        {
            Log(("BranchCallGate jump %04x:%016RX64 - not canonical -> #GP\n", uNewCS, uNewRip));
            return iemRaiseNotCanonical(pVCpu);
        }

        /*
         * Ok, everything checked out fine.  Now set the accessed bit before
         * committing the result into CS, CSHID and RIP.
         */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            /** @todo check what VT-x and AMD-V does. */
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /* commit */
        pCtx->rip         = uNewRip;
        pCtx->cs.Sel      = uNewCS & X86_SEL_MASK_OFF_RPL;
        pCtx->cs.Sel     |= pVCpu->iem.s.uCpl; /** @todo is this right for conforming segs? or in general? */
        pCtx->cs.ValidSel = pCtx->cs.Sel;
        pCtx->cs.fFlags   = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.Attr.u   = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
        pCtx->cs.u32Limit = cbLimit;
        pCtx->cs.u64Base  = u64Base;
        pVCpu->iem.s.enmCpuMode = iemCalcCpuMode(pCtx);
    }
    else
    {
        Assert(enmBranch == IEMBRANCH_CALL);
        /* Calls are much more complicated. */

        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF) && (DescCS.Legacy.Gen.u2Dpl < pVCpu->iem.s.uCpl))
        {
            uint16_t    offNewStack;    /* Offset of new stack in TSS. */
            uint16_t    cbNewStack;     /* Number of bytes the stack information takes up in TSS. */
            uint8_t     uNewCSDpl;
            uint8_t     cbWords;
            RTSEL       uNewSS;
            RTSEL       uOldSS;
            uint64_t    uOldRsp;
            IEMSELDESC  DescSS;
            RTPTRUNION  uPtrTSS;
            RTGCPTR     GCPtrTSS;
            RTPTRUNION  uPtrParmWds;
            RTGCPTR     GCPtrParmWds;

            /* More privilege. This is the fun part. */
            Assert(!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF));    /* Filtered out above. */

            /*
             * Determine new SS:rSP from the TSS.
             */
            Assert(!pCtx->tr.Attr.n.u1DescType);

            /* Figure out where the new stack pointer is stored in the TSS. */
            uNewCSDpl = DescCS.Legacy.Gen.u2Dpl;
            if (!IEM_IS_LONG_MODE(pVCpu))
            {
                if (pCtx->tr.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY)
                {
                    offNewStack = RT_UOFFSETOF(X86TSS32, esp0) + uNewCSDpl * 8;
                    cbNewStack  = RT_SIZEOFMEMB(X86TSS32, esp0) + RT_SIZEOFMEMB(X86TSS32, ss0);
                }
                else
                {
                    Assert(pCtx->tr.Attr.n.u4Type == X86_SEL_TYPE_SYS_286_TSS_BUSY);
                    offNewStack = RT_UOFFSETOF(X86TSS16, sp0) + uNewCSDpl * 4;
                    cbNewStack  = RT_SIZEOFMEMB(X86TSS16, sp0) + RT_SIZEOFMEMB(X86TSS16, ss0);
                }
            }
            else
            {
                Assert(pCtx->tr.Attr.n.u4Type == AMD64_SEL_TYPE_SYS_TSS_BUSY);
                offNewStack = RT_UOFFSETOF(X86TSS64, rsp0) + uNewCSDpl * RT_SIZEOFMEMB(X86TSS64, rsp0);
                cbNewStack  = RT_SIZEOFMEMB(X86TSS64, rsp0);
            }

            /* Check against TSS limit. */
            if ((uint16_t)(offNewStack + cbNewStack - 1) > pCtx->tr.u32Limit)
            {
                Log(("BranchCallGate inner stack past TSS limit - %u > %u -> #TS(TSS)\n", offNewStack + cbNewStack - 1, pCtx->tr.u32Limit));
                return iemRaiseTaskSwitchFaultBySelector(pVCpu, pCtx->tr.Sel);
            }

            GCPtrTSS = pCtx->tr.u64Base + offNewStack;
            rcStrict = iemMemMap(pVCpu, &uPtrTSS.pv, cbNewStack, UINT8_MAX, GCPtrTSS, IEM_ACCESS_SYS_R);
            if (rcStrict != VINF_SUCCESS)
            {
                Log(("BranchCallGate: TSS mapping failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                return rcStrict;
            }

            if (!IEM_IS_LONG_MODE(pVCpu))
            {
                if (pCtx->tr.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY)
                {
                    uNewRsp = uPtrTSS.pu32[0];
                    uNewSS  = uPtrTSS.pu16[2];
                }
                else
                {
                    Assert(pCtx->tr.Attr.n.u4Type == X86_SEL_TYPE_SYS_286_TSS_BUSY);
                    uNewRsp = uPtrTSS.pu16[0];
                    uNewSS  = uPtrTSS.pu16[1];
                }
            }
            else
            {
                Assert(pCtx->tr.Attr.n.u4Type == AMD64_SEL_TYPE_SYS_TSS_BUSY);
                /* SS will be a NULL selector, but that's valid. */
                uNewRsp = uPtrTSS.pu64[0];
                uNewSS  = uNewCSDpl;
            }

            /* Done with the TSS now. */
            rcStrict = iemMemCommitAndUnmap(pVCpu, uPtrTSS.pv, IEM_ACCESS_SYS_R);
            if (rcStrict != VINF_SUCCESS)
            {
                Log(("BranchCallGate: TSS unmapping failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                return rcStrict;
            }

            /* Only used outside of long mode. */
            cbWords = pDesc->Legacy.Gate.u5ParmCount;

            /* If EFER.LMA is 0, there's extra work to do. */
            if (!IEM_IS_LONG_MODE(pVCpu))
            {
                if ((uNewSS & X86_SEL_MASK_OFF_RPL) == 0)
                {
                    Log(("BranchCallGate new SS NULL -> #TS(NewSS)\n"));
                    return iemRaiseTaskSwitchFaultBySelector(pVCpu, uNewSS);
                }

                /* Grab the new SS descriptor. */
                rcStrict = iemMemFetchSelDesc(pVCpu, &DescSS, uNewSS, X86_XCPT_SS);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;

                /* Ensure that CS.DPL == SS.RPL == SS.DPL. */
                if (   (DescCS.Legacy.Gen.u2Dpl != (uNewSS & X86_SEL_RPL))
                    || (DescCS.Legacy.Gen.u2Dpl != DescSS.Legacy.Gen.u2Dpl))
                {
                    Log(("BranchCallGate call bad RPL/DPL uNewSS=%04x SS DPL=%d CS DPL=%u -> #TS(NewSS)\n",
                         uNewSS, DescCS.Legacy.Gen.u2Dpl, DescCS.Legacy.Gen.u2Dpl));
                    return iemRaiseTaskSwitchFaultBySelector(pVCpu, uNewSS);
                }

                /* Ensure new SS is a writable data segment. */
                if ((DescSS.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE)) != X86_SEL_TYPE_WRITE)
                {
                    Log(("BranchCallGate call new SS -> not a writable data selector (u4Type=%#x)\n", DescSS.Legacy.Gen.u4Type));
                    return iemRaiseTaskSwitchFaultBySelector(pVCpu, uNewSS);
                }

                if (!DescSS.Legacy.Gen.u1Present)
                {
                    Log(("BranchCallGate New stack not present uSel=%04x -> #SS(NewSS)\n", uNewSS));
                    return iemRaiseStackSelectorNotPresentBySelector(pVCpu, uNewSS);
                }
                if (pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_CALL_GATE)
                    cbNewStack = (uint16_t)sizeof(uint32_t) * (4 + cbWords);
                else
                    cbNewStack = (uint16_t)sizeof(uint16_t) * (4 + cbWords);
            }
            else
            {
                /* Just grab the new (NULL) SS descriptor. */
                /** @todo testcase: Check whether the zero GDT entry is actually loaded here
                 *        like we do... */
                rcStrict = iemMemFetchSelDesc(pVCpu, &DescSS, uNewSS, X86_XCPT_SS);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;

                cbNewStack = sizeof(uint64_t) * 4;
            }

            /** @todo: According to Intel, new stack is checked for enough space first,
             *         then switched. According to AMD, the stack is switched first and
             *         then pushes might fault!
             *         NB: OS/2 Warp 3/4 actively relies on the fact that possible
             *         incoming stack #PF happens before actual stack switch. AMD is
             *         either lying or implicitly assumes that new state is committed
             *         only if and when an instruction doesn't fault.
             */

            /** @todo: According to AMD, CS is loaded first, then SS.
             *         According to Intel, it's the other way around!?
             */

            /** @todo: Intel and AMD disagree on when exactly the CPL changes! */

            /* Set the accessed bit before committing new SS. */
            if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
            {
                rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewSS);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;
                DescSS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
            }

            /* Remember the old SS:rSP and their linear address. */
            uOldSS  = pCtx->ss.Sel;
            uOldRsp = pCtx->ss.Attr.n.u1DefBig ? pCtx->rsp : pCtx->sp;

            GCPtrParmWds = pCtx->ss.u64Base + uOldRsp;

            /* HACK ALERT! Probe if the write to the new stack will succeed. May #SS(NewSS)
                           or #PF, the former is not implemented in this workaround. */
            /** @todo Proper fix callgate target stack exceptions. */
            /** @todo testcase: Cover callgates with partially or fully inaccessible
             *        target stacks. */
            void    *pvNewFrame;
            RTGCPTR  GCPtrNewStack = X86DESC_BASE(&DescSS.Legacy) + uNewRsp - cbNewStack;
            rcStrict = iemMemMap(pVCpu, &pvNewFrame, cbNewStack, UINT8_MAX, GCPtrNewStack, IEM_ACCESS_SYS_RW);
            if (rcStrict != VINF_SUCCESS)
            {
                Log(("BranchCallGate: Incoming stack (%04x:%08RX64) not accessible, rc=%Rrc\n", uNewSS, uNewRsp, VBOXSTRICTRC_VAL(rcStrict)));
                return rcStrict;
            }
            rcStrict = iemMemCommitAndUnmap(pVCpu, pvNewFrame, IEM_ACCESS_SYS_RW);
            if (rcStrict != VINF_SUCCESS)
            {
                Log(("BranchCallGate: New stack probe unmapping failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                return rcStrict;
            }

            /* Commit new SS:rSP. */
            pCtx->ss.Sel      = uNewSS;
            pCtx->ss.ValidSel = uNewSS;
            pCtx->ss.Attr.u   = X86DESC_GET_HID_ATTR(&DescSS.Legacy);
            pCtx->ss.u32Limit = X86DESC_LIMIT_G(&DescSS.Legacy);
            pCtx->ss.u64Base  = X86DESC_BASE(&DescSS.Legacy);
            pCtx->ss.fFlags   = CPUMSELREG_FLAGS_VALID;
            pCtx->rsp         = uNewRsp;
            pVCpu->iem.s.uCpl = uNewCSDpl;
            Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ss));
            CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);

            /* At this point the stack access must not fail because new state was already committed. */
            /** @todo this can still fail due to SS.LIMIT not check.   */
            rcStrict = iemMemStackPushBeginSpecial(pVCpu, cbNewStack,
                                                   &uPtrRet.pv, &uNewRsp);
            AssertMsgReturn(rcStrict == VINF_SUCCESS, ("BranchCallGate: New stack mapping failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)),
                            VERR_INTERNAL_ERROR_5);

            if (!IEM_IS_LONG_MODE(pVCpu))
            {
                if (pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_CALL_GATE)
                {
                    /* Push the old CS:rIP. */
                    uPtrRet.pu32[0] = pCtx->eip + cbInstr;
                    uPtrRet.pu32[1] = pCtx->cs.Sel; /** @todo Testcase: What is written to the high word when pushing CS? */

                    if (cbWords)
                    {
                        /* Map the relevant chunk of the old stack. */
                        rcStrict = iemMemMap(pVCpu, &uPtrParmWds.pv, cbWords * 4, UINT8_MAX, GCPtrParmWds, IEM_ACCESS_DATA_R);
                        if (rcStrict != VINF_SUCCESS)
                        {
                            Log(("BranchCallGate: Old stack mapping (32-bit) failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                            return rcStrict;
                        }

                        /* Copy the parameter (d)words. */
                        for (int i = 0; i < cbWords; ++i)
                            uPtrRet.pu32[2 + i] = uPtrParmWds.pu32[i];

                        /* Unmap the old stack. */
                        rcStrict = iemMemCommitAndUnmap(pVCpu, uPtrParmWds.pv, IEM_ACCESS_DATA_R);
                        if (rcStrict != VINF_SUCCESS)
                        {
                            Log(("BranchCallGate: Old stack unmapping (32-bit) failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                            return rcStrict;
                        }
                    }

                    /* Push the old SS:rSP. */
                    uPtrRet.pu32[2 + cbWords + 0] = uOldRsp;
                    uPtrRet.pu32[2 + cbWords + 1] = uOldSS;
                }
                else
                {
                    Assert(pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_CALL_GATE);

                    /* Push the old CS:rIP. */
                    uPtrRet.pu16[0] = pCtx->ip + cbInstr;
                    uPtrRet.pu16[1] = pCtx->cs.Sel;

                    if (cbWords)
                    {
                        /* Map the relevant chunk of the old stack. */
                        rcStrict = iemMemMap(pVCpu, &uPtrParmWds.pv, cbWords * 2, UINT8_MAX, GCPtrParmWds, IEM_ACCESS_DATA_R);
                        if (rcStrict != VINF_SUCCESS)
                        {
                            Log(("BranchCallGate: Old stack mapping (16-bit) failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                            return rcStrict;
                        }

                        /* Copy the parameter words. */
                        for (int i = 0; i < cbWords; ++i)
                            uPtrRet.pu16[2 + i] = uPtrParmWds.pu16[i];

                        /* Unmap the old stack. */
                        rcStrict = iemMemCommitAndUnmap(pVCpu, uPtrParmWds.pv, IEM_ACCESS_DATA_R);
                        if (rcStrict != VINF_SUCCESS)
                        {
                            Log(("BranchCallGate: Old stack unmapping (32-bit) failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                            return rcStrict;
                        }
                    }

                    /* Push the old SS:rSP. */
                    uPtrRet.pu16[2 + cbWords + 0] = uOldRsp;
                    uPtrRet.pu16[2 + cbWords + 1] = uOldSS;
                }
            }
            else
            {
                Assert(pDesc->Legacy.Gate.u4Type == AMD64_SEL_TYPE_SYS_CALL_GATE);

                /* For 64-bit gates, no parameters are copied. Just push old SS:rSP and CS:rIP. */
                uPtrRet.pu64[0] = pCtx->rip + cbInstr;
                uPtrRet.pu64[1] = pCtx->cs.Sel; /** @todo Testcase: What is written to the high words when pushing CS? */
                uPtrRet.pu64[2] = uOldRsp;
                uPtrRet.pu64[3] = uOldSS;       /** @todo Testcase: What is written to the high words when pushing SS? */
            }

            rcStrict = iemMemStackPushCommitSpecial(pVCpu, uPtrRet.pv, uNewRsp);
            if (rcStrict != VINF_SUCCESS)
            {
                Log(("BranchCallGate: New stack unmapping failed (%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict)));
                return rcStrict;
            }

            /* Chop the high bits off if 16-bit gate (Intel says so). */
            if (pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_CALL_GATE)
                uNewRip = (uint16_t)uNewRip;

            /* Limit / canonical check. */
            cbLimit = X86DESC_LIMIT_G(&DescCS.Legacy);
            if (!IEM_IS_LONG_MODE(pVCpu))
            {
                if (uNewRip > cbLimit)
                {
                    Log(("BranchCallGate %04x:%08RX64 -> out of bounds (%#x)\n", uNewCS, uNewRip, cbLimit));
                    return iemRaiseGeneralProtectionFaultBySelector(pVCpu, 0);
                }
                u64Base = X86DESC_BASE(&DescCS.Legacy);
            }
            else
            {
                Assert(pDesc->Legacy.Gate.u4Type == AMD64_SEL_TYPE_SYS_CALL_GATE);
                if (!IEM_IS_CANONICAL(uNewRip))
                {
                    Log(("BranchCallGate call %04x:%016RX64 - not canonical -> #GP\n", uNewCS, uNewRip));
                    return iemRaiseNotCanonical(pVCpu);
                }
                u64Base = 0;
            }

            /*
             * Now set the accessed bit before
             * writing the return address to the stack and committing the result into
             * CS, CSHID and RIP.
             */
            /** @todo Testcase: Need to check WHEN exactly the accessed bit is set. */
            if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
            {
                rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCS);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;
                /** @todo check what VT-x and AMD-V does. */
                DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
            }

            /* Commit new CS:rIP. */
            pCtx->rip         = uNewRip;
            pCtx->cs.Sel      = uNewCS & X86_SEL_MASK_OFF_RPL;
            pCtx->cs.Sel     |= pVCpu->iem.s.uCpl;
            pCtx->cs.ValidSel = pCtx->cs.Sel;
            pCtx->cs.fFlags   = CPUMSELREG_FLAGS_VALID;
            pCtx->cs.Attr.u   = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
            pCtx->cs.u32Limit = cbLimit;
            pCtx->cs.u64Base  = u64Base;
            pVCpu->iem.s.enmCpuMode = iemCalcCpuMode(pCtx);
        }
        else
        {
            /* Same privilege. */
            /** @todo: This is very similar to regular far calls; merge! */

            /* Check stack first - may #SS(0). */
            /** @todo check how gate size affects pushing of CS! Does callf 16:32 in
             *        16-bit code cause a two or four byte CS to be pushed? */
            rcStrict = iemMemStackPushBeginSpecial(pVCpu,
                                                   IEM_IS_LONG_MODE(pVCpu) ? 8+8
                                                   : pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_CALL_GATE ? 4+4 : 2+2,
                                                   &uPtrRet.pv, &uNewRsp);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /* Chop the high bits off if 16-bit gate (Intel says so). */
            if (pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_CALL_GATE)
                uNewRip = (uint16_t)uNewRip;

            /* Limit / canonical check. */
            cbLimit = X86DESC_LIMIT_G(&DescCS.Legacy);
            if (!IEM_IS_LONG_MODE(pVCpu))
            {
                if (uNewRip > cbLimit)
                {
                    Log(("BranchCallGate %04x:%08RX64 -> out of bounds (%#x)\n", uNewCS, uNewRip, cbLimit));
                    return iemRaiseGeneralProtectionFaultBySelector(pVCpu, 0);
                }
                u64Base = X86DESC_BASE(&DescCS.Legacy);
            }
            else
            {
                if (!IEM_IS_CANONICAL(uNewRip))
                {
                    Log(("BranchCallGate call %04x:%016RX64 - not canonical -> #GP\n", uNewCS, uNewRip));
                    return iemRaiseNotCanonical(pVCpu);
                }
                u64Base = 0;
            }

            /*
             * Now set the accessed bit before
             * writing the return address to the stack and committing the result into
             * CS, CSHID and RIP.
             */
            /** @todo Testcase: Need to check WHEN exactly the accessed bit is set. */
            if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
            {
                rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCS);
                if (rcStrict != VINF_SUCCESS)
                    return rcStrict;
                /** @todo check what VT-x and AMD-V does. */
                DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
            }

            /* stack */
            if (!IEM_IS_LONG_MODE(pVCpu))
            {
                if (pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_CALL_GATE)
                {
                    uPtrRet.pu32[0] = pCtx->eip + cbInstr;
                    uPtrRet.pu32[1] = pCtx->cs.Sel; /** @todo Testcase: What is written to the high word when pushing CS? */
                }
                else
                {
                    Assert(pDesc->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_CALL_GATE);
                    uPtrRet.pu16[0] = pCtx->ip + cbInstr;
                    uPtrRet.pu16[1] = pCtx->cs.Sel;
                }
            }
            else
            {
                Assert(pDesc->Legacy.Gate.u4Type == AMD64_SEL_TYPE_SYS_CALL_GATE);
                uPtrRet.pu64[0] = pCtx->rip + cbInstr;
                uPtrRet.pu64[1] = pCtx->cs.Sel; /** @todo Testcase: What is written to the high words when pushing CS? */
            }

            rcStrict = iemMemStackPushCommitSpecial(pVCpu, uPtrRet.pv, uNewRsp);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            /* commit */
            pCtx->rip         = uNewRip;
            pCtx->cs.Sel      = uNewCS & X86_SEL_MASK_OFF_RPL;
            pCtx->cs.Sel     |= pVCpu->iem.s.uCpl;
            pCtx->cs.ValidSel = pCtx->cs.Sel;
            pCtx->cs.fFlags   = CPUMSELREG_FLAGS_VALID;
            pCtx->cs.Attr.u   = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
            pCtx->cs.u32Limit = cbLimit;
            pCtx->cs.u64Base  = u64Base;
            pVCpu->iem.s.enmCpuMode  = iemCalcCpuMode(pCtx);
        }
    }
    pCtx->eflags.Bits.u1RF = 0;

    /* Flush the prefetch buffer. */
# ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
# else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
# endif
    return VINF_SUCCESS;
#endif
}


/**
 * Implements far jumps and calls thru system selectors.
 *
 * @param   uSel            The selector.
 * @param   enmBranch       The kind of branching we're performing.
 * @param   enmEffOpSize    The effective operand size.
 * @param   pDesc           The descriptor corresponding to @a uSel.
 */
IEM_CIMPL_DEF_4(iemCImpl_BranchSysSel, uint16_t, uSel, IEMBRANCH, enmBranch, IEMMODE, enmEffOpSize, PIEMSELDESC, pDesc)
{
    Assert(enmBranch == IEMBRANCH_JUMP || enmBranch == IEMBRANCH_CALL);
    Assert((uSel & X86_SEL_MASK_OFF_RPL));

    if (IEM_IS_LONG_MODE(pVCpu))
        switch (pDesc->Legacy.Gen.u4Type)
        {
            case AMD64_SEL_TYPE_SYS_CALL_GATE:
                return IEM_CIMPL_CALL_4(iemCImpl_BranchCallGate, uSel, enmBranch, enmEffOpSize, pDesc);

            default:
            case AMD64_SEL_TYPE_SYS_LDT:
            case AMD64_SEL_TYPE_SYS_TSS_BUSY:
            case AMD64_SEL_TYPE_SYS_TSS_AVAIL:
            case AMD64_SEL_TYPE_SYS_TRAP_GATE:
            case AMD64_SEL_TYPE_SYS_INT_GATE:
                Log(("branch %04x -> wrong sys selector (64-bit): %d\n", uSel, pDesc->Legacy.Gen.u4Type));
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }

    switch (pDesc->Legacy.Gen.u4Type)
    {
        case X86_SEL_TYPE_SYS_286_CALL_GATE:
        case X86_SEL_TYPE_SYS_386_CALL_GATE:
            return IEM_CIMPL_CALL_4(iemCImpl_BranchCallGate, uSel, enmBranch, enmEffOpSize, pDesc);

        case X86_SEL_TYPE_SYS_TASK_GATE:
            return IEM_CIMPL_CALL_4(iemCImpl_BranchTaskGate, uSel, enmBranch, enmEffOpSize, pDesc);

        case X86_SEL_TYPE_SYS_286_TSS_AVAIL:
        case X86_SEL_TYPE_SYS_386_TSS_AVAIL:
            return IEM_CIMPL_CALL_4(iemCImpl_BranchTaskSegment, uSel, enmBranch, enmEffOpSize, pDesc);

        case X86_SEL_TYPE_SYS_286_TSS_BUSY:
            Log(("branch %04x -> busy 286 TSS\n", uSel));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);

        case X86_SEL_TYPE_SYS_386_TSS_BUSY:
            Log(("branch %04x -> busy 386 TSS\n", uSel));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);

        default:
        case X86_SEL_TYPE_SYS_LDT:
        case X86_SEL_TYPE_SYS_286_INT_GATE:
        case X86_SEL_TYPE_SYS_286_TRAP_GATE:
        case X86_SEL_TYPE_SYS_386_INT_GATE:
        case X86_SEL_TYPE_SYS_386_TRAP_GATE:
            Log(("branch %04x -> wrong sys selector: %d\n", uSel, pDesc->Legacy.Gen.u4Type));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
    }
}


/**
 * Implements far jumps.
 *
 * @param   uSel            The selector.
 * @param   offSeg          The segment offset.
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_3(iemCImpl_FarJmp, uint16_t, uSel, uint64_t, offSeg, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    NOREF(cbInstr);
    Assert(offSeg <= UINT32_MAX);

    /*
     * Real mode and V8086 mode are easy.  The only snag seems to be that
     * CS.limit doesn't change and the limit check is done against the current
     * limit.
     */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        if (offSeg > pCtx->cs.u32Limit)
        {
            Log(("iemCImpl_FarJmp: 16-bit limit\n"));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        if (enmEffOpSize == IEMMODE_16BIT) /** @todo WRONG, must pass this. */
            pCtx->rip       = offSeg;
        else
            pCtx->rip       = offSeg & UINT16_MAX;
        pCtx->cs.Sel        = uSel;
        pCtx->cs.ValidSel   = uSel;
        pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.u64Base    = (uint32_t)uSel << 4;
        pCtx->eflags.Bits.u1RF = 0;
        return VINF_SUCCESS;
    }

    /*
     * Protected mode. Need to parse the specified descriptor...
     */
    if (!(uSel & X86_SEL_MASK_OFF_RPL))
    {
        Log(("jmpf %04x:%08RX64 -> invalid selector, #GP(0)\n", uSel, offSeg));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Fetch the descriptor. */
    IEMSELDESC Desc;
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, &Desc, uSel, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Is it there? */
    if (!Desc.Legacy.Gen.u1Present) /** @todo this is probably checked too early. Testcase! */
    {
        Log(("jmpf %04x:%08RX64 -> segment not present\n", uSel, offSeg));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSel);
    }

    /*
     * Deal with it according to its type.  We do the standard code selectors
     * here and dispatch the system selectors to worker functions.
     */
    if (!Desc.Legacy.Gen.u1DescType)
        return IEM_CIMPL_CALL_4(iemCImpl_BranchSysSel, uSel, IEMBRANCH_JUMP, enmEffOpSize, &Desc);

    /* Only code segments. */
    if (!(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
    {
        Log(("jmpf %04x:%08RX64 -> not a code selector (u4Type=%#x).\n", uSel, offSeg, Desc.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
    }

    /* L vs D. */
    if (   Desc.Legacy.Gen.u1Long
        && Desc.Legacy.Gen.u1DefBig
        && IEM_IS_LONG_MODE(pVCpu))
    {
        Log(("jmpf %04x:%08RX64 -> both L and D are set.\n", uSel, offSeg));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
    }

    /* DPL/RPL/CPL check, where conforming segments makes a difference. */
    if (Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
    {
        if (pVCpu->iem.s.uCpl < Desc.Legacy.Gen.u2Dpl)
        {
            Log(("jmpf %04x:%08RX64 -> DPL violation (conforming); DPL=%d CPL=%u\n",
                 uSel, offSeg, Desc.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
    }
    else
    {
        if (pVCpu->iem.s.uCpl != Desc.Legacy.Gen.u2Dpl)
        {
            Log(("jmpf %04x:%08RX64 -> CPL != DPL; DPL=%d CPL=%u\n", uSel, offSeg, Desc.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
        if ((uSel & X86_SEL_RPL) > pVCpu->iem.s.uCpl)
        {
            Log(("jmpf %04x:%08RX64 -> RPL > DPL; RPL=%d CPL=%u\n", uSel, offSeg, (uSel & X86_SEL_RPL), pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
    }

    /* Chop the high bits if 16-bit (Intel says so). */
    if (enmEffOpSize == IEMMODE_16BIT)
        offSeg &= UINT16_MAX;

    /* Limit check. (Should alternatively check for non-canonical addresses
       here, but that is ruled out by offSeg being 32-bit, right?) */
    uint64_t u64Base;
    uint32_t cbLimit = X86DESC_LIMIT_G(&Desc.Legacy);
    if (Desc.Legacy.Gen.u1Long)
        u64Base = 0;
    else
    {
        if (offSeg > cbLimit)
        {
            Log(("jmpf %04x:%08RX64 -> out of bounds (%#x)\n", uSel, offSeg, cbLimit));
            /** @todo: Intel says this is #GP(0)! */
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
        u64Base = X86DESC_BASE(&Desc.Legacy);
    }

    /*
     * Ok, everything checked out fine.  Now set the accessed bit before
     * committing the result into CS, CSHID and RIP.
     */
    if (!(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, uSel);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        /** @todo check what VT-x and AMD-V does. */
        Desc.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    /* commit */
    pCtx->rip = offSeg;
    pCtx->cs.Sel         = uSel & X86_SEL_MASK_OFF_RPL;
    pCtx->cs.Sel        |= pVCpu->iem.s.uCpl; /** @todo is this right for conforming segs? or in general? */
    pCtx->cs.ValidSel    = pCtx->cs.Sel;
    pCtx->cs.fFlags      = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.Attr.u      = X86DESC_GET_HID_ATTR(&Desc.Legacy);
    pCtx->cs.u32Limit    = cbLimit;
    pCtx->cs.u64Base     = u64Base;
    pVCpu->iem.s.enmCpuMode  = iemCalcCpuMode(pCtx);
    pCtx->eflags.Bits.u1RF = 0;
    /** @todo check if the hidden bits are loaded correctly for 64-bit
     *        mode.  */

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Implements far calls.
 *
 * This very similar to iemCImpl_FarJmp.
 *
 * @param   uSel            The selector.
 * @param   offSeg          The segment offset.
 * @param   enmEffOpSize    The operand size (in case we need it).
 */
IEM_CIMPL_DEF_3(iemCImpl_callf, uint16_t, uSel, uint64_t, offSeg, IEMMODE, enmEffOpSize)
{
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC    rcStrict;
    uint64_t        uNewRsp;
    RTPTRUNION      uPtrRet;

    /*
     * Real mode and V8086 mode are easy.  The only snag seems to be that
     * CS.limit doesn't change and the limit check is done against the current
     * limit.
     */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        Assert(enmEffOpSize == IEMMODE_16BIT || enmEffOpSize == IEMMODE_32BIT);

        /* Check stack first - may #SS(0). */
        rcStrict = iemMemStackPushBeginSpecial(pVCpu, enmEffOpSize == IEMMODE_32BIT ? 4+4 : 2+2,
                                               &uPtrRet.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /* Check the target address range. */
        if (offSeg > UINT32_MAX)
            return iemRaiseGeneralProtectionFault0(pVCpu);

        /* Everything is fine, push the return address. */
        if (enmEffOpSize == IEMMODE_16BIT)
        {
            uPtrRet.pu16[0] = pCtx->ip + cbInstr;
            uPtrRet.pu16[1] = pCtx->cs.Sel;
        }
        else
        {
            uPtrRet.pu32[0] = pCtx->eip + cbInstr;
            uPtrRet.pu16[2] = pCtx->cs.Sel;
        }
        rcStrict = iemMemStackPushCommitSpecial(pVCpu, uPtrRet.pv, uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /* Branch. */
        pCtx->rip           = offSeg;
        pCtx->cs.Sel        = uSel;
        pCtx->cs.ValidSel   = uSel;
        pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.u64Base    = (uint32_t)uSel << 4;
        pCtx->eflags.Bits.u1RF = 0;
        return VINF_SUCCESS;
    }

    /*
     * Protected mode. Need to parse the specified descriptor...
     */
    if (!(uSel & X86_SEL_MASK_OFF_RPL))
    {
        Log(("callf %04x:%08RX64 -> invalid selector, #GP(0)\n", uSel, offSeg));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Fetch the descriptor. */
    IEMSELDESC Desc;
    rcStrict = iemMemFetchSelDesc(pVCpu, &Desc, uSel, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Deal with it according to its type.  We do the standard code selectors
     * here and dispatch the system selectors to worker functions.
     */
    if (!Desc.Legacy.Gen.u1DescType)
        return IEM_CIMPL_CALL_4(iemCImpl_BranchSysSel, uSel, IEMBRANCH_CALL, enmEffOpSize, &Desc);

    /* Only code segments. */
    if (!(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
    {
        Log(("callf %04x:%08RX64 -> not a code selector (u4Type=%#x).\n", uSel, offSeg, Desc.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
    }

    /* L vs D. */
    if (   Desc.Legacy.Gen.u1Long
        && Desc.Legacy.Gen.u1DefBig
        && IEM_IS_LONG_MODE(pVCpu))
    {
        Log(("callf %04x:%08RX64 -> both L and D are set.\n", uSel, offSeg));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
    }

    /* DPL/RPL/CPL check, where conforming segments makes a difference. */
    if (Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
    {
        if (pVCpu->iem.s.uCpl < Desc.Legacy.Gen.u2Dpl)
        {
            Log(("callf %04x:%08RX64 -> DPL violation (conforming); DPL=%d CPL=%u\n",
                 uSel, offSeg, Desc.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
    }
    else
    {
        if (pVCpu->iem.s.uCpl != Desc.Legacy.Gen.u2Dpl)
        {
            Log(("callf %04x:%08RX64 -> CPL != DPL; DPL=%d CPL=%u\n", uSel, offSeg, Desc.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
        if ((uSel & X86_SEL_RPL) > pVCpu->iem.s.uCpl)
        {
            Log(("callf %04x:%08RX64 -> RPL > DPL; RPL=%d CPL=%u\n", uSel, offSeg, (uSel & X86_SEL_RPL), pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
    }

    /* Is it there? */
    if (!Desc.Legacy.Gen.u1Present)
    {
        Log(("callf %04x:%08RX64 -> segment not present\n", uSel, offSeg));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSel);
    }

    /* Check stack first - may #SS(0). */
    /** @todo check how operand prefix affects pushing of CS! Does callf 16:32 in
     *        16-bit code cause a two or four byte CS to be pushed? */
    rcStrict = iemMemStackPushBeginSpecial(pVCpu,
                                           enmEffOpSize == IEMMODE_64BIT   ? 8+8
                                           : enmEffOpSize == IEMMODE_32BIT ? 4+4 : 2+2,
                                           &uPtrRet.pv, &uNewRsp);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Chop the high bits if 16-bit (Intel says so). */
    if (enmEffOpSize == IEMMODE_16BIT)
        offSeg &= UINT16_MAX;

    /* Limit / canonical check. */
    uint64_t u64Base;
    uint32_t cbLimit = X86DESC_LIMIT_G(&Desc.Legacy);
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        if (!IEM_IS_CANONICAL(offSeg))
        {
            Log(("callf %04x:%016RX64 - not canonical -> #GP\n", uSel, offSeg));
            return iemRaiseNotCanonical(pVCpu);
        }
        u64Base = 0;
    }
    else
    {
        if (offSeg > cbLimit)
        {
            Log(("callf %04x:%08RX64 -> out of bounds (%#x)\n", uSel, offSeg, cbLimit));
            /** @todo: Intel says this is #GP(0)! */
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
        u64Base = X86DESC_BASE(&Desc.Legacy);
    }

    /*
     * Now set the accessed bit before
     * writing the return address to the stack and committing the result into
     * CS, CSHID and RIP.
     */
    /** @todo Testcase: Need to check WHEN exactly the accessed bit is set. */
    if (!(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, uSel);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        /** @todo check what VT-x and AMD-V does. */
        Desc.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    /* stack */
    if (enmEffOpSize == IEMMODE_16BIT)
    {
        uPtrRet.pu16[0] = pCtx->ip + cbInstr;
        uPtrRet.pu16[1] = pCtx->cs.Sel;
    }
    else if (enmEffOpSize == IEMMODE_32BIT)
    {
        uPtrRet.pu32[0] = pCtx->eip + cbInstr;
        uPtrRet.pu32[1] = pCtx->cs.Sel; /** @todo Testcase: What is written to the high word when callf is pushing CS? */
    }
    else
    {
        uPtrRet.pu64[0] = pCtx->rip + cbInstr;
        uPtrRet.pu64[1] = pCtx->cs.Sel; /** @todo Testcase: What is written to the high words when callf is pushing CS? */
    }
    rcStrict = iemMemStackPushCommitSpecial(pVCpu, uPtrRet.pv, uNewRsp);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* commit */
    pCtx->rip = offSeg;
    pCtx->cs.Sel         = uSel & X86_SEL_MASK_OFF_RPL;
    pCtx->cs.Sel        |= pVCpu->iem.s.uCpl;
    pCtx->cs.ValidSel    = pCtx->cs.Sel;
    pCtx->cs.fFlags      = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.Attr.u      = X86DESC_GET_HID_ATTR(&Desc.Legacy);
    pCtx->cs.u32Limit    = cbLimit;
    pCtx->cs.u64Base     = u64Base;
    pVCpu->iem.s.enmCpuMode  = iemCalcCpuMode(pCtx);
    pCtx->eflags.Bits.u1RF = 0;
    /** @todo check if the hidden bits are loaded correctly for 64-bit
     *        mode.  */

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif
    return VINF_SUCCESS;
}


/**
 * Implements retf.
 *
 * @param   enmEffOpSize    The effective operand size.
 * @param   cbPop           The amount of arguments to pop from the stack
 *                          (bytes).
 */
IEM_CIMPL_DEF_2(iemCImpl_retf, IEMMODE, enmEffOpSize, uint16_t, cbPop)
{
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC    rcStrict;
    RTCPTRUNION     uPtrFrame;
    uint64_t        uNewRsp;
    uint64_t        uNewRip;
    uint16_t        uNewCs;
    NOREF(cbInstr);

    /*
     * Read the stack values first.
     */
    uint32_t        cbRetPtr = enmEffOpSize == IEMMODE_16BIT ? 2+2
                             : enmEffOpSize == IEMMODE_32BIT ? 4+4 : 8+8;
    rcStrict = iemMemStackPopBeginSpecial(pVCpu, cbRetPtr, &uPtrFrame.pv, &uNewRsp);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    if (enmEffOpSize == IEMMODE_16BIT)
    {
        uNewRip = uPtrFrame.pu16[0];
        uNewCs  = uPtrFrame.pu16[1];
    }
    else if (enmEffOpSize == IEMMODE_32BIT)
    {
        uNewRip = uPtrFrame.pu32[0];
        uNewCs  = uPtrFrame.pu16[2];
    }
    else
    {
        uNewRip = uPtrFrame.pu64[0];
        uNewCs  = uPtrFrame.pu16[4];
    }
    rcStrict = iemMemStackPopDoneSpecial(pVCpu, uPtrFrame.pv);
    if (RT_LIKELY(rcStrict == VINF_SUCCESS))
    { /* extremely likely */ }
    else
        return rcStrict;

    /*
     * Real mode and V8086 mode are easy.
     */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        Assert(enmEffOpSize == IEMMODE_32BIT || enmEffOpSize == IEMMODE_16BIT);
        /** @todo check how this is supposed to work if sp=0xfffe. */

        /* Check the limit of the new EIP. */
        /** @todo Intel pseudo code only does the limit check for 16-bit
         *        operands, AMD does not make any distinction. What is right? */
        if (uNewRip > pCtx->cs.u32Limit)
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);

        /* commit the operation. */
        pCtx->rsp           = uNewRsp;
        pCtx->rip           = uNewRip;
        pCtx->cs.Sel        = uNewCs;
        pCtx->cs.ValidSel   = uNewCs;
        pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.u64Base    = (uint32_t)uNewCs << 4;
        pCtx->eflags.Bits.u1RF = 0;
        /** @todo do we load attribs and limit as well? */
        if (cbPop)
            iemRegAddToRsp(pVCpu, pCtx, cbPop);
        return VINF_SUCCESS;
    }

    /*
     * Protected mode is complicated, of course.
     */
    if (!(uNewCs & X86_SEL_MASK_OFF_RPL))
    {
        Log(("retf %04x:%08RX64 -> invalid selector, #GP(0)\n", uNewCs, uNewRip));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Fetch the descriptor. */
    IEMSELDESC DescCs;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCs, uNewCs, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Can only return to a code selector. */
    if (   !DescCs.Legacy.Gen.u1DescType
        || !(DescCs.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE) )
    {
        Log(("retf %04x:%08RX64 -> not a code selector (u1DescType=%u u4Type=%#x).\n",
             uNewCs, uNewRip, DescCs.Legacy.Gen.u1DescType, DescCs.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }

    /* L vs D. */
    if (   DescCs.Legacy.Gen.u1Long /** @todo Testcase: far return to a selector with both L and D set. */
        && DescCs.Legacy.Gen.u1DefBig
        && IEM_IS_LONG_MODE(pVCpu))
    {
        Log(("retf %04x:%08RX64 -> both L & D set.\n", uNewCs, uNewRip));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }

    /* DPL/RPL/CPL checks. */
    if ((uNewCs & X86_SEL_RPL) < pVCpu->iem.s.uCpl)
    {
        Log(("retf %04x:%08RX64 -> RPL < CPL(%d).\n", uNewCs, uNewRip, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }

    if (DescCs.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
    {
        if ((uNewCs & X86_SEL_RPL) < DescCs.Legacy.Gen.u2Dpl)
        {
            Log(("retf %04x:%08RX64 -> DPL violation (conforming); DPL=%u RPL=%u\n",
                 uNewCs, uNewRip, DescCs.Legacy.Gen.u2Dpl, (uNewCs & X86_SEL_RPL)));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
        }
    }
    else
    {
        if ((uNewCs & X86_SEL_RPL) != DescCs.Legacy.Gen.u2Dpl)
        {
            Log(("retf %04x:%08RX64 -> RPL != DPL; DPL=%u RPL=%u\n",
                 uNewCs, uNewRip, DescCs.Legacy.Gen.u2Dpl, (uNewCs & X86_SEL_RPL)));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
        }
    }

    /* Is it there? */
    if (!DescCs.Legacy.Gen.u1Present)
    {
        Log(("retf %04x:%08RX64 -> segment not present\n", uNewCs, uNewRip));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uNewCs);
    }

    /*
     * Return to outer privilege? (We'll typically have entered via a call gate.)
     */
    if ((uNewCs & X86_SEL_RPL) != pVCpu->iem.s.uCpl)
    {
        /* Read the outer stack pointer stored *after* the parameters. */
        rcStrict = iemMemStackPopContinueSpecial(pVCpu, cbPop + cbRetPtr, &uPtrFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        uPtrFrame.pu8 += cbPop; /* Skip the parameters. */

        uint16_t uNewOuterSs;
        uint64_t uNewOuterRsp;
        if (enmEffOpSize == IEMMODE_16BIT)
        {
            uNewOuterRsp = uPtrFrame.pu16[0];
            uNewOuterSs  = uPtrFrame.pu16[1];
        }
        else if (enmEffOpSize == IEMMODE_32BIT)
        {
            uNewOuterRsp = uPtrFrame.pu32[0];
            uNewOuterSs  = uPtrFrame.pu16[2];
        }
        else
        {
            uNewOuterRsp = uPtrFrame.pu64[0];
            uNewOuterSs  = uPtrFrame.pu16[4];
        }
        uPtrFrame.pu8 -= cbPop; /* Put uPtrFrame back the way it was. */
        rcStrict = iemMemStackPopDoneSpecial(pVCpu, uPtrFrame.pv);
        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        { /* extremely likely */ }
        else
            return rcStrict;

        /* Check for NULL stack selector (invalid in ring-3 and non-long mode)
           and read the selector. */
        IEMSELDESC DescSs;
        if (!(uNewOuterSs & X86_SEL_MASK_OFF_RPL))
        {
            if (   !DescCs.Legacy.Gen.u1Long
                || (uNewOuterSs & X86_SEL_RPL) == 3)
            {
                Log(("retf %04x:%08RX64 %04x:%08RX64 -> invalid stack selector, #GP\n",
                     uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }
            /** @todo Testcase: Return far to ring-1 or ring-2 with SS=0. */
            iemMemFakeStackSelDesc(&DescSs, (uNewOuterSs & X86_SEL_RPL));
        }
        else
        {
            /* Fetch the descriptor for the new stack segment. */
            rcStrict = iemMemFetchSelDesc(pVCpu, &DescSs, uNewOuterSs, X86_XCPT_GP);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }

        /* Check that RPL of stack and code selectors match. */
        if ((uNewCs & X86_SEL_RPL) != (uNewOuterSs & X86_SEL_RPL))
        {
            Log(("retf %04x:%08RX64 %04x:%08RX64 - SS.RPL != CS.RPL -> #GP(SS)\n", uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewOuterSs);
        }

        /* Must be a writable data segment. */
        if (   !DescSs.Legacy.Gen.u1DescType
            || (DescSs.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE)
            || !(DescSs.Legacy.Gen.u4Type & X86_SEL_TYPE_WRITE) )
        {
            Log(("retf %04x:%08RX64 %04x:%08RX64 - SS not a writable data segment (u1DescType=%u u4Type=%#x) -> #GP(SS).\n",
                 uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp, DescSs.Legacy.Gen.u1DescType, DescSs.Legacy.Gen.u4Type));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewOuterSs);
        }

        /* L vs D. (Not mentioned by intel.) */
        if (   DescSs.Legacy.Gen.u1Long /** @todo Testcase: far return to a stack selector with both L and D set. */
            && DescSs.Legacy.Gen.u1DefBig
            && IEM_IS_LONG_MODE(pVCpu))
        {
            Log(("retf %04x:%08RX64 %04x:%08RX64 - SS has both L & D set -> #GP(SS).\n",
                 uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewOuterSs);
        }

        /* DPL/RPL/CPL checks. */
        if (DescSs.Legacy.Gen.u2Dpl != (uNewCs & X86_SEL_RPL))
        {
            Log(("retf %04x:%08RX64 %04x:%08RX64 - SS.DPL(%u) != CS.RPL (%u) -> #GP(SS).\n",
                 uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp, DescSs.Legacy.Gen.u2Dpl, uNewCs & X86_SEL_RPL));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewOuterSs);
        }

        /* Is it there? */
        if (!DescSs.Legacy.Gen.u1Present)
        {
            Log(("retf %04x:%08RX64 %04x:%08RX64 - SS not present -> #NP(SS).\n", uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp));
            return iemRaiseSelectorNotPresentBySelector(pVCpu, uNewCs);
        }

        /* Calc SS limit.*/
        uint32_t cbLimitSs = X86DESC_LIMIT_G(&DescSs.Legacy);

        /* Is RIP canonical or within CS.limit? */
        uint64_t u64Base;
        uint32_t cbLimitCs = X86DESC_LIMIT_G(&DescCs.Legacy);

        /** @todo Testcase: Is this correct? */
        if (   DescCs.Legacy.Gen.u1Long
            && IEM_IS_LONG_MODE(pVCpu) )
        {
            if (!IEM_IS_CANONICAL(uNewRip))
            {
                Log(("retf %04x:%08RX64 %04x:%08RX64 - not canonical -> #GP.\n", uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp));
                return iemRaiseNotCanonical(pVCpu);
            }
            u64Base = 0;
        }
        else
        {
            if (uNewRip > cbLimitCs)
            {
                Log(("retf %04x:%08RX64 %04x:%08RX64 - out of bounds (%#x)-> #GP(CS).\n",
                     uNewCs, uNewRip, uNewOuterSs, uNewOuterRsp, cbLimitCs));
                /** @todo: Intel says this is #GP(0)! */
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
            }
            u64Base = X86DESC_BASE(&DescCs.Legacy);
        }

        /*
         * Now set the accessed bit before
         * writing the return address to the stack and committing the result into
         * CS, CSHID and RIP.
         */
        /** @todo Testcase: Need to check WHEN exactly the CS accessed bit is set. */
        if (!(DescCs.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCs);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            /** @todo check what VT-x and AMD-V does. */
            DescCs.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }
        /** @todo Testcase: Need to check WHEN exactly the SS accessed bit is set. */
        if (!(DescSs.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewOuterSs);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            /** @todo check what VT-x and AMD-V does. */
            DescSs.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /* commit */
        if (enmEffOpSize == IEMMODE_16BIT)
            pCtx->rip           = uNewRip & UINT16_MAX; /** @todo Testcase: When exactly does this occur? With call it happens prior to the limit check according to Intel... */
        else
            pCtx->rip           = uNewRip;
        pCtx->cs.Sel            = uNewCs;
        pCtx->cs.ValidSel       = uNewCs;
        pCtx->cs.fFlags         = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.Attr.u         = X86DESC_GET_HID_ATTR(&DescCs.Legacy);
        pCtx->cs.u32Limit       = cbLimitCs;
        pCtx->cs.u64Base        = u64Base;
        pVCpu->iem.s.enmCpuMode     = iemCalcCpuMode(pCtx);
        pCtx->ss.Sel            = uNewOuterSs;
        pCtx->ss.ValidSel       = uNewOuterSs;
        pCtx->ss.fFlags         = CPUMSELREG_FLAGS_VALID;
        pCtx->ss.Attr.u         = X86DESC_GET_HID_ATTR(&DescSs.Legacy);
        pCtx->ss.u32Limit       = cbLimitSs;
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
            pCtx->ss.u64Base    = 0;
        else
            pCtx->ss.u64Base    = X86DESC_BASE(&DescSs.Legacy);
        if (!pCtx->ss.Attr.n.u1DefBig)
            pCtx->sp            = (uint16_t)uNewOuterRsp;
        else
            pCtx->rsp           = uNewOuterRsp;

        pVCpu->iem.s.uCpl           = (uNewCs & X86_SEL_RPL);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->ds);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->es);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->fs);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->gs);

        /** @todo check if the hidden bits are loaded correctly for 64-bit
         *        mode. */

        if (cbPop)
            iemRegAddToRsp(pVCpu, pCtx, cbPop);
        pCtx->eflags.Bits.u1RF = 0;

        /* Done! */
    }
    /*
     * Return to the same privilege level
     */
    else
    {
        /* Limit / canonical check. */
        uint64_t u64Base;
        uint32_t cbLimitCs = X86DESC_LIMIT_G(&DescCs.Legacy);

        /** @todo Testcase: Is this correct? */
        if (   DescCs.Legacy.Gen.u1Long
            && IEM_IS_LONG_MODE(pVCpu) )
        {
            if (!IEM_IS_CANONICAL(uNewRip))
            {
                Log(("retf %04x:%08RX64 - not canonical -> #GP\n", uNewCs, uNewRip));
                return iemRaiseNotCanonical(pVCpu);
            }
            u64Base = 0;
        }
        else
        {
            if (uNewRip > cbLimitCs)
            {
                Log(("retf %04x:%08RX64 -> out of bounds (%#x)\n", uNewCs, uNewRip, cbLimitCs));
                /** @todo: Intel says this is #GP(0)! */
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
            }
            u64Base = X86DESC_BASE(&DescCs.Legacy);
        }

        /*
         * Now set the accessed bit before
         * writing the return address to the stack and committing the result into
         * CS, CSHID and RIP.
         */
        /** @todo Testcase: Need to check WHEN exactly the accessed bit is set. */
        if (!(DescCs.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCs);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            /** @todo check what VT-x and AMD-V does. */
            DescCs.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /* commit */
        if (!pCtx->ss.Attr.n.u1DefBig)
            pCtx->sp        = (uint16_t)uNewRsp;
        else
            pCtx->rsp       = uNewRsp;
        if (enmEffOpSize == IEMMODE_16BIT)
            pCtx->rip       = uNewRip & UINT16_MAX; /** @todo Testcase: When exactly does this occur? With call it happens prior to the limit check according to Intel... */
        else
            pCtx->rip       = uNewRip;
        pCtx->cs.Sel        = uNewCs;
        pCtx->cs.ValidSel   = uNewCs;
        pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.Attr.u     = X86DESC_GET_HID_ATTR(&DescCs.Legacy);
        pCtx->cs.u32Limit   = cbLimitCs;
        pCtx->cs.u64Base    = u64Base;
        /** @todo check if the hidden bits are loaded correctly for 64-bit
         *        mode.  */
        pVCpu->iem.s.enmCpuMode = iemCalcCpuMode(pCtx);
        if (cbPop)
            iemRegAddToRsp(pVCpu, pCtx, cbPop);
        pCtx->eflags.Bits.u1RF = 0;
    }

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif
    return VINF_SUCCESS;
}


/**
 * Implements retn.
 *
 * We're doing this in C because of the \#GP that might be raised if the popped
 * program counter is out of bounds.
 *
 * @param   enmEffOpSize    The effective operand size.
 * @param   cbPop           The amount of arguments to pop from the stack
 *                          (bytes).
 */
IEM_CIMPL_DEF_2(iemCImpl_retn, IEMMODE, enmEffOpSize, uint16_t, cbPop)
{
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);
    NOREF(cbInstr);

    /* Fetch the RSP from the stack. */
    VBOXSTRICTRC    rcStrict;
    RTUINT64U       NewRip;
    RTUINT64U       NewRsp;
    NewRsp.u = pCtx->rsp;

    switch (enmEffOpSize)
    {
        case IEMMODE_16BIT:
            NewRip.u = 0;
            rcStrict = iemMemStackPopU16Ex(pVCpu, &NewRip.Words.w0, &NewRsp);
            break;
        case IEMMODE_32BIT:
            NewRip.u = 0;
            rcStrict = iemMemStackPopU32Ex(pVCpu, &NewRip.DWords.dw0, &NewRsp);
            break;
        case IEMMODE_64BIT:
            rcStrict = iemMemStackPopU64Ex(pVCpu, &NewRip.u, &NewRsp);
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Check the new RSP before loading it. */
    /** @todo Should test this as the intel+amd pseudo code doesn't mention half
     *        of it.  The canonical test is performed here and for call. */
    if (enmEffOpSize != IEMMODE_64BIT)
    {
        if (NewRip.DWords.dw0 > pCtx->cs.u32Limit)
        {
            Log(("retn newrip=%llx - out of bounds (%x) -> #GP\n", NewRip.u, pCtx->cs.u32Limit));
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
        }
    }
    else
    {
        if (!IEM_IS_CANONICAL(NewRip.u))
        {
            Log(("retn newrip=%llx - not canonical -> #GP\n", NewRip.u));
            return iemRaiseNotCanonical(pVCpu);
        }
    }

    /* Apply cbPop */
    if (cbPop)
        iemRegAddToRspEx(pVCpu, pCtx, &NewRsp, cbPop);

    /* Commit it. */
    pCtx->rip = NewRip.u;
    pCtx->rsp = NewRsp.u;
    pCtx->eflags.Bits.u1RF = 0;

    /* Flush the prefetch buffer. */
#ifndef IEM_WITH_CODE_TLB
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Implements enter.
 *
 * We're doing this in C because the instruction is insane, even for the
 * u8NestingLevel=0 case dealing with the stack is tedious.
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_3(iemCImpl_enter, IEMMODE, enmEffOpSize, uint16_t, cbFrame, uint8_t, cParameters)
{
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);

    /* Push RBP, saving the old value in TmpRbp. */
    RTUINT64U       NewRsp; NewRsp.u = pCtx->rsp;
    RTUINT64U       TmpRbp; TmpRbp.u = pCtx->rbp;
    RTUINT64U       NewRbp;
    VBOXSTRICTRC    rcStrict;
    if (enmEffOpSize == IEMMODE_64BIT)
    {
        rcStrict = iemMemStackPushU64Ex(pVCpu, TmpRbp.u, &NewRsp);
        NewRbp = NewRsp;
    }
    else if (enmEffOpSize == IEMMODE_32BIT)
    {
        rcStrict = iemMemStackPushU32Ex(pVCpu, TmpRbp.DWords.dw0, &NewRsp);
        NewRbp = NewRsp;
    }
    else
    {
        rcStrict = iemMemStackPushU16Ex(pVCpu, TmpRbp.Words.w0, &NewRsp);
        NewRbp = TmpRbp;
        NewRbp.Words.w0 = NewRsp.Words.w0;
    }
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Copy the parameters (aka nesting levels by Intel). */
    cParameters &= 0x1f;
    if (cParameters > 0)
    {
        switch (enmEffOpSize)
        {
            case IEMMODE_16BIT:
                if (pCtx->ss.Attr.n.u1DefBig)
                    TmpRbp.DWords.dw0 -= 2;
                else
                    TmpRbp.Words.w0   -= 2;
                do
                {
                    uint16_t u16Tmp;
                    rcStrict = iemMemStackPopU16Ex(pVCpu, &u16Tmp, &TmpRbp);
                    if (rcStrict != VINF_SUCCESS)
                        break;
                    rcStrict = iemMemStackPushU16Ex(pVCpu, u16Tmp, &NewRsp);
                } while (--cParameters > 0 && rcStrict == VINF_SUCCESS);
                break;

            case IEMMODE_32BIT:
                if (pCtx->ss.Attr.n.u1DefBig)
                    TmpRbp.DWords.dw0 -= 4;
                else
                    TmpRbp.Words.w0   -= 4;
                do
                {
                    uint32_t u32Tmp;
                    rcStrict = iemMemStackPopU32Ex(pVCpu, &u32Tmp, &TmpRbp);
                    if (rcStrict != VINF_SUCCESS)
                        break;
                    rcStrict = iemMemStackPushU32Ex(pVCpu, u32Tmp, &NewRsp);
                } while (--cParameters > 0 && rcStrict == VINF_SUCCESS);
                break;

            case IEMMODE_64BIT:
                TmpRbp.u -= 8;
                do
                {
                    uint64_t u64Tmp;
                    rcStrict = iemMemStackPopU64Ex(pVCpu, &u64Tmp, &TmpRbp);
                    if (rcStrict != VINF_SUCCESS)
                        break;
                    rcStrict = iemMemStackPushU64Ex(pVCpu, u64Tmp, &NewRsp);
                } while (--cParameters > 0 && rcStrict == VINF_SUCCESS);
                break;

            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
        if (rcStrict != VINF_SUCCESS)
            return VINF_SUCCESS;

        /* Push the new RBP */
        if (enmEffOpSize == IEMMODE_64BIT)
            rcStrict = iemMemStackPushU64Ex(pVCpu, NewRbp.u, &NewRsp);
        else if (enmEffOpSize == IEMMODE_32BIT)
            rcStrict = iemMemStackPushU32Ex(pVCpu, NewRbp.DWords.dw0, &NewRsp);
        else
            rcStrict = iemMemStackPushU16Ex(pVCpu, NewRbp.Words.w0, &NewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

    }

    /* Recalc RSP. */
    iemRegSubFromRspEx(pVCpu, pCtx, &NewRsp, cbFrame);

    /** @todo Should probe write access at the new RSP according to AMD. */

    /* Commit it. */
    pCtx->rbp = NewRbp.u;
    pCtx->rsp = NewRsp.u;
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);

    return VINF_SUCCESS;
}



/**
 * Implements leave.
 *
 * We're doing this in C because messing with the stack registers is annoying
 * since they depends on SS attributes.
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_leave, IEMMODE, enmEffOpSize)
{
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);

    /* Calculate the intermediate RSP from RBP and the stack attributes. */
    RTUINT64U       NewRsp;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        NewRsp.u = pCtx->rbp;
    else if (pCtx->ss.Attr.n.u1DefBig)
        NewRsp.u = pCtx->ebp;
    else
    {
        /** @todo Check that LEAVE actually preserve the high EBP bits. */
        NewRsp.u = pCtx->rsp;
        NewRsp.Words.w0 = pCtx->bp;
    }

    /* Pop RBP according to the operand size. */
    VBOXSTRICTRC    rcStrict;
    RTUINT64U       NewRbp;
    switch (enmEffOpSize)
    {
        case IEMMODE_16BIT:
            NewRbp.u = pCtx->rbp;
            rcStrict = iemMemStackPopU16Ex(pVCpu, &NewRbp.Words.w0, &NewRsp);
            break;
        case IEMMODE_32BIT:
            NewRbp.u = 0;
            rcStrict = iemMemStackPopU32Ex(pVCpu, &NewRbp.DWords.dw0, &NewRsp);
            break;
        case IEMMODE_64BIT:
            rcStrict = iemMemStackPopU64Ex(pVCpu, &NewRbp.u, &NewRsp);
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;


    /* Commit it. */
    pCtx->rbp = NewRbp.u;
    pCtx->rsp = NewRsp.u;
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);

    return VINF_SUCCESS;
}


/**
 * Implements int3 and int XX.
 *
 * @param   u8Int       The interrupt vector number.
 * @param   enmInt      The int instruction type.
 */
IEM_CIMPL_DEF_2(iemCImpl_int, uint8_t, u8Int, IEMINT, enmInt)
{
    Assert(pVCpu->iem.s.cXcptRecursions == 0);
    return iemRaiseXcptOrInt(pVCpu,
                             cbInstr,
                             u8Int,
                             IEM_XCPT_FLAGS_T_SOFT_INT | enmInt,
                             0,
                             0);
}


/**
 * Implements iret for real mode and V8086 mode.
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_iret_real_v8086, IEMMODE, enmEffOpSize)
{
    PCPUMCTX  pCtx  = IEM_GET_CTX(pVCpu);
    X86EFLAGS Efl;
    Efl.u = IEMMISC_GET_EFL(pVCpu, pCtx);
    NOREF(cbInstr);

    /*
     * iret throws an exception if VME isn't enabled.
     */
    if (   Efl.Bits.u1VM
        && Efl.Bits.u2IOPL != 3
        && !(pCtx->cr4 & X86_CR4_VME))
        return iemRaiseGeneralProtectionFault0(pVCpu);

    /*
     * Do the stack bits, but don't commit RSP before everything checks
     * out right.
     */
    Assert(enmEffOpSize == IEMMODE_32BIT || enmEffOpSize == IEMMODE_16BIT);
    VBOXSTRICTRC    rcStrict;
    RTCPTRUNION     uFrame;
    uint16_t        uNewCs;
    uint32_t        uNewEip;
    uint32_t        uNewFlags;
    uint64_t        uNewRsp;
    if (enmEffOpSize == IEMMODE_32BIT)
    {
        rcStrict = iemMemStackPopBeginSpecial(pVCpu, 12, &uFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        uNewEip    = uFrame.pu32[0];
        if (uNewEip > UINT16_MAX)
            return iemRaiseGeneralProtectionFault0(pVCpu);

        uNewCs     = (uint16_t)uFrame.pu32[1];
        uNewFlags  = uFrame.pu32[2];
        uNewFlags &= X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF
                   | X86_EFL_TF | X86_EFL_IF | X86_EFL_DF | X86_EFL_OF | X86_EFL_IOPL | X86_EFL_NT
                   | X86_EFL_RF /*| X86_EFL_VM*/ | X86_EFL_AC /*|X86_EFL_VIF*/ /*|X86_EFL_VIP*/
                   | X86_EFL_ID;
        if (IEM_GET_TARGET_CPU(pVCpu) <= IEMTARGETCPU_386)
            uNewFlags &= ~(X86_EFL_AC | X86_EFL_ID | X86_EFL_VIF | X86_EFL_VIP);
        uNewFlags |= Efl.u & (X86_EFL_VM | X86_EFL_VIF | X86_EFL_VIP | X86_EFL_1);
    }
    else
    {
        rcStrict = iemMemStackPopBeginSpecial(pVCpu, 6, &uFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        uNewEip    = uFrame.pu16[0];
        uNewCs     = uFrame.pu16[1];
        uNewFlags  = uFrame.pu16[2];
        uNewFlags &= X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF
                   | X86_EFL_TF | X86_EFL_IF | X86_EFL_DF | X86_EFL_OF | X86_EFL_IOPL | X86_EFL_NT;
        uNewFlags |= Efl.u & ((UINT32_C(0xffff0000) | X86_EFL_1) & ~X86_EFL_RF);
        /** @todo The intel pseudo code does not indicate what happens to
         *        reserved flags. We just ignore them. */
        /* Ancient CPU adjustments: See iemCImpl_popf. */
        if (IEM_GET_TARGET_CPU(pVCpu) == IEMTARGETCPU_286)
            uNewFlags &= ~(X86_EFL_NT | X86_EFL_IOPL);
    }
    rcStrict = iemMemStackPopDoneSpecial(pVCpu, uFrame.pv);
    if (RT_LIKELY(rcStrict == VINF_SUCCESS))
    { /* extremely likely */ }
    else
        return rcStrict;

    /** @todo Check how this is supposed to work if sp=0xfffe. */
    Log7(("iemCImpl_iret_real_v8086: uNewCs=%#06x uNewRip=%#010x uNewFlags=%#x uNewRsp=%#18llx\n",
          uNewCs, uNewEip, uNewFlags, uNewRsp));

    /*
     * Check the limit of the new EIP.
     */
    /** @todo Only the AMD pseudo code check the limit here, what's
     *        right? */
    if (uNewEip > pCtx->cs.u32Limit)
        return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);

    /*
     * V8086 checks and flag adjustments
     */
    if (Efl.Bits.u1VM)
    {
        if (Efl.Bits.u2IOPL == 3)
        {
            /* Preserve IOPL and clear RF. */
            uNewFlags &=        ~(X86_EFL_IOPL | X86_EFL_RF);
            uNewFlags |= Efl.u & (X86_EFL_IOPL);
        }
        else if (   enmEffOpSize == IEMMODE_16BIT
                 && (   !(uNewFlags & X86_EFL_IF)
                     || !Efl.Bits.u1VIP )
                 && !(uNewFlags & X86_EFL_TF)   )
        {
            /* Move IF to VIF, clear RF and preserve IF and IOPL.*/
            uNewFlags &= ~X86_EFL_VIF;
            uNewFlags |= (uNewFlags & X86_EFL_IF) << (19 - 9);
            uNewFlags &=        ~(X86_EFL_IF | X86_EFL_IOPL | X86_EFL_RF);
            uNewFlags |= Efl.u & (X86_EFL_IF | X86_EFL_IOPL);
        }
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);
        Log7(("iemCImpl_iret_real_v8086: u1VM=1: adjusted uNewFlags=%#x\n", uNewFlags));
    }

    /*
     * Commit the operation.
     */
#ifdef DBGFTRACE_ENABLED
    RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "iret/rm %04x:%04x -> %04x:%04x %x %04llx",
                      pCtx->cs.Sel, pCtx->eip, uNewCs, uNewEip, uNewFlags, uNewRsp);
#endif
    pCtx->rsp           = uNewRsp;
    pCtx->rip           = uNewEip;
    pCtx->cs.Sel        = uNewCs;
    pCtx->cs.ValidSel   = uNewCs;
    pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.u64Base    = (uint32_t)uNewCs << 4;
    /** @todo do we load attribs and limit as well? */
    Assert(uNewFlags & X86_EFL_1);
    IEMMISC_SET_EFL(pVCpu, pCtx, uNewFlags);

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Loads a segment register when entering V8086 mode.
 *
 * @param   pSReg           The segment register.
 * @param   uSeg            The segment to load.
 */
static void iemCImplCommonV8086LoadSeg(PCPUMSELREG pSReg, uint16_t uSeg)
{
    pSReg->Sel        = uSeg;
    pSReg->ValidSel   = uSeg;
    pSReg->fFlags     = CPUMSELREG_FLAGS_VALID;
    pSReg->u64Base    = (uint32_t)uSeg << 4;
    pSReg->u32Limit   = 0xffff;
    pSReg->Attr.u     = X86_SEL_TYPE_RW_ACC | RT_BIT(4) /*!sys*/ | RT_BIT(7) /*P*/ | (3 /*DPL*/ << 5); /* VT-x wants 0xf3 */
    /** @todo Testcase: Check if VT-x really needs this and what it does itself when
     *        IRET'ing to V8086. */
}


/**
 * Implements iret for protected mode returning to V8086 mode.
 *
 * @param   pCtx            Pointer to the CPU context.
 * @param   uNewEip         The new EIP.
 * @param   uNewCs          The new CS.
 * @param   uNewFlags       The new EFLAGS.
 * @param   uNewRsp         The RSP after the initial IRET frame.
 *
 * @note    This can only be a 32-bit iret du to the X86_EFL_VM position.
 */
IEM_CIMPL_DEF_5(iemCImpl_iret_prot_v8086, PCPUMCTX, pCtx, uint32_t, uNewEip, uint16_t, uNewCs,
                uint32_t, uNewFlags, uint64_t, uNewRsp)
{
    RT_NOREF_PV(cbInstr);

    /*
     * Pop the V8086 specific frame bits off the stack.
     */
    VBOXSTRICTRC    rcStrict;
    RTCPTRUNION     uFrame;
    rcStrict = iemMemStackPopContinueSpecial(pVCpu, 24, &uFrame.pv, &uNewRsp);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    uint32_t uNewEsp = uFrame.pu32[0];
    uint16_t uNewSs  = uFrame.pu32[1];
    uint16_t uNewEs  = uFrame.pu32[2];
    uint16_t uNewDs  = uFrame.pu32[3];
    uint16_t uNewFs  = uFrame.pu32[4];
    uint16_t uNewGs  = uFrame.pu32[5];
    rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)uFrame.pv, IEM_ACCESS_STACK_R); /* don't use iemMemStackPopCommitSpecial here. */
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Commit the operation.
     */
    uNewFlags &= X86_EFL_LIVE_MASK;
    uNewFlags |= X86_EFL_RA1_MASK;
#ifdef DBGFTRACE_ENABLED
    RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "iret/p/v %04x:%08x -> %04x:%04x %x %04x:%04x",
                      pCtx->cs.Sel, pCtx->eip, uNewCs, uNewEip, uNewFlags, uNewSs, uNewEsp);
#endif
    Log7(("iemCImpl_iret_prot_v8086: %04x:%08x -> %04x:%04x %x %04x:%04x\n", pCtx->cs.Sel, pCtx->eip, uNewCs, uNewEip, uNewFlags, uNewSs, uNewEsp));

    IEMMISC_SET_EFL(pVCpu, pCtx, uNewFlags);
    iemCImplCommonV8086LoadSeg(&pCtx->cs, uNewCs);
    iemCImplCommonV8086LoadSeg(&pCtx->ss, uNewSs);
    iemCImplCommonV8086LoadSeg(&pCtx->es, uNewEs);
    iemCImplCommonV8086LoadSeg(&pCtx->ds, uNewDs);
    iemCImplCommonV8086LoadSeg(&pCtx->fs, uNewFs);
    iemCImplCommonV8086LoadSeg(&pCtx->gs, uNewGs);
    pCtx->rip      = (uint16_t)uNewEip;
    pCtx->rsp      = uNewEsp; /** @todo check this out! */
    pVCpu->iem.s.uCpl  = 3;

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Implements iret for protected mode returning via a nested task.
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_iret_prot_NestedTask, IEMMODE, enmEffOpSize)
{
    Log7(("iemCImpl_iret_prot_NestedTask:\n"));
#ifndef IEM_IMPLEMENTS_TASKSWITCH
    IEM_RETURN_ASPECT_NOT_IMPLEMENTED();
#else
    RT_NOREF_PV(enmEffOpSize);

    /*
     * Read the segment selector in the link-field of the current TSS.
     */
    RTSEL        uSelRet;
    PCPUMCTX     pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC rcStrict = iemMemFetchSysU16(pVCpu, &uSelRet, UINT8_MAX, pCtx->tr.u64Base);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Fetch the returning task's TSS descriptor from the GDT.
     */
    if (uSelRet & X86_SEL_LDT)
    {
        Log(("iret_prot_NestedTask TSS not in LDT. uSelRet=%04x -> #TS\n", uSelRet));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, uSelRet);
    }

    IEMSELDESC TssDesc;
    rcStrict = iemMemFetchSelDesc(pVCpu, &TssDesc, uSelRet, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    if (TssDesc.Legacy.Gate.u1DescType)
    {
        Log(("iret_prot_NestedTask Invalid TSS type. uSelRet=%04x -> #TS\n", uSelRet));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, uSelRet & X86_SEL_MASK_OFF_RPL);
    }

    if (   TssDesc.Legacy.Gate.u4Type != X86_SEL_TYPE_SYS_286_TSS_BUSY
        && TssDesc.Legacy.Gate.u4Type != X86_SEL_TYPE_SYS_386_TSS_BUSY)
    {
        Log(("iret_prot_NestedTask TSS is not busy. uSelRet=%04x DescType=%#x -> #TS\n", uSelRet, TssDesc.Legacy.Gate.u4Type));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, uSelRet & X86_SEL_MASK_OFF_RPL);
    }

    if (!TssDesc.Legacy.Gate.u1Present)
    {
        Log(("iret_prot_NestedTask TSS is not present. uSelRet=%04x -> #NP\n", uSelRet));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSelRet & X86_SEL_MASK_OFF_RPL);
    }

    uint32_t uNextEip = pCtx->eip + cbInstr;
    return iemTaskSwitch(pVCpu, pCtx, IEMTASKSWITCH_IRET, uNextEip, 0 /* fFlags */, 0 /* uErr */,
                         0 /* uCr2 */, uSelRet, &TssDesc);
#endif
}


/**
 * Implements iret for protected mode
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_iret_prot, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    NOREF(cbInstr);
    Assert(enmEffOpSize == IEMMODE_32BIT || enmEffOpSize == IEMMODE_16BIT);

    /*
     * Nested task return.
     */
    if (pCtx->eflags.Bits.u1NT)
        return IEM_CIMPL_CALL_1(iemCImpl_iret_prot_NestedTask, enmEffOpSize);

    /*
     * Normal return.
     *
     * Do the stack bits, but don't commit RSP before everything checks
     * out right.
     */
    Assert(enmEffOpSize == IEMMODE_32BIT || enmEffOpSize == IEMMODE_16BIT);
    VBOXSTRICTRC    rcStrict;
    RTCPTRUNION     uFrame;
    uint16_t        uNewCs;
    uint32_t        uNewEip;
    uint32_t        uNewFlags;
    uint64_t        uNewRsp;
    if (enmEffOpSize == IEMMODE_32BIT)
    {
        rcStrict = iemMemStackPopBeginSpecial(pVCpu, 12, &uFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        uNewEip    = uFrame.pu32[0];
        uNewCs     = (uint16_t)uFrame.pu32[1];
        uNewFlags  = uFrame.pu32[2];
    }
    else
    {
        rcStrict = iemMemStackPopBeginSpecial(pVCpu, 6, &uFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        uNewEip    = uFrame.pu16[0];
        uNewCs     = uFrame.pu16[1];
        uNewFlags  = uFrame.pu16[2];
    }
    rcStrict = iemMemStackPopDoneSpecial(pVCpu, (void *)uFrame.pv); /* don't use iemMemStackPopCommitSpecial here. */
    if (RT_LIKELY(rcStrict == VINF_SUCCESS))
    { /* extremely likely */ }
    else
        return rcStrict;
    Log7(("iemCImpl_iret_prot: uNewCs=%#06x uNewEip=%#010x uNewFlags=%#x uNewRsp=%#18llx uCpl=%u\n", uNewCs, uNewEip, uNewFlags, uNewRsp, pVCpu->iem.s.uCpl));

    /*
     * We're hopefully not returning to V8086 mode...
     */
    if (   (uNewFlags & X86_EFL_VM)
        && pVCpu->iem.s.uCpl == 0)
    {
        Assert(enmEffOpSize == IEMMODE_32BIT);
        return IEM_CIMPL_CALL_5(iemCImpl_iret_prot_v8086, pCtx, uNewEip, uNewCs, uNewFlags, uNewRsp);
    }

    /*
     * Protected mode.
     */
    /* Read the CS descriptor. */
    if (!(uNewCs & X86_SEL_MASK_OFF_RPL))
    {
        Log(("iret %04x:%08x -> invalid CS selector, #GP(0)\n", uNewCs, uNewEip));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    IEMSELDESC DescCS;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, uNewCs, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iret %04x:%08x - rcStrict=%Rrc when fetching CS\n", uNewCs, uNewEip, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a code descriptor. */
    if (!DescCS.Legacy.Gen.u1DescType)
    {
        Log(("iret %04x:%08x - CS is system segment (%#x) -> #GP\n", uNewCs, uNewEip, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
    {
        Log(("iret %04x:%08x - not code segment (%#x) -> #GP\n", uNewCs, uNewEip, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    /* Raw ring-0 and ring-1 compression adjustments for PATM performance tricks and other CS leaks. */
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    if (EMIsRawRing0Enabled(pVM) && !HMIsEnabled(pVM))
    {
        if ((uNewCs & X86_SEL_RPL) == 1)
        {
            if (   pVCpu->iem.s.uCpl == 0
                && (   !EMIsRawRing1Enabled(pVM)
                    || pCtx->cs.Sel == (uNewCs & X86_SEL_MASK_OFF_RPL)) )
            {
                Log(("iret: Ring-0 compression fix: uNewCS=%#x -> %#x\n", uNewCs, uNewCs & X86_SEL_MASK_OFF_RPL));
                uNewCs &= X86_SEL_MASK_OFF_RPL;
            }
# ifdef LOG_ENABLED
            else if (pVCpu->iem.s.uCpl <= 1 && EMIsRawRing1Enabled(pVM))
                Log(("iret: uNewCs=%#x genuine return to ring-1.\n", uNewCs));
# endif
        }
        else if (   (uNewCs & X86_SEL_RPL) == 2
                 && EMIsRawRing1Enabled(pVM)
                 && pVCpu->iem.s.uCpl <= 1)
        {
            Log(("iret: Ring-1 compression fix: uNewCS=%#x -> %#x\n", uNewCs, (uNewCs & X86_SEL_MASK_OFF_RPL) | 1));
            uNewCs = (uNewCs & X86_SEL_MASK_OFF_RPL) | 2;
        }
    }
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */


    /* Privilege checks. */
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF))
    {
        if ((uNewCs & X86_SEL_RPL) != DescCS.Legacy.Gen.u2Dpl)
        {
            Log(("iret %04x:%08x - RPL != DPL (%d) -> #GP\n", uNewCs, uNewEip, DescCS.Legacy.Gen.u2Dpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
        }
    }
    else if ((uNewCs & X86_SEL_RPL) < DescCS.Legacy.Gen.u2Dpl)
    {
        Log(("iret %04x:%08x - RPL < DPL (%d) -> #GP\n", uNewCs, uNewEip, DescCS.Legacy.Gen.u2Dpl));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }
    if ((uNewCs & X86_SEL_RPL) < pVCpu->iem.s.uCpl)
    {
        Log(("iret %04x:%08x - RPL < CPL (%d) -> #GP\n", uNewCs, uNewEip, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }

    /* Present? */
    if (!DescCS.Legacy.Gen.u1Present)
    {
        Log(("iret %04x:%08x - CS not present -> #NP\n", uNewCs, uNewEip));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uNewCs);
    }

    uint32_t cbLimitCS = X86DESC_LIMIT_G(&DescCS.Legacy);

    /*
     * Return to outer level?
     */
    if ((uNewCs & X86_SEL_RPL) != pVCpu->iem.s.uCpl)
    {
        uint16_t    uNewSS;
        uint32_t    uNewESP;
        if (enmEffOpSize == IEMMODE_32BIT)
        {
            rcStrict = iemMemStackPopContinueSpecial(pVCpu, 8, &uFrame.pv, &uNewRsp);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
/** @todo We might be popping a 32-bit ESP from the IRET frame, but whether
 *        16-bit or 32-bit are being loaded into SP depends on the D/B
 *        bit of the popped SS selector it turns out. */
            uNewESP = uFrame.pu32[0];
            uNewSS  = (uint16_t)uFrame.pu32[1];
        }
        else
        {
            rcStrict = iemMemStackPopContinueSpecial(pVCpu, 4, &uFrame.pv, &uNewRsp);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            uNewESP = uFrame.pu16[0];
            uNewSS  = uFrame.pu16[1];
        }
        rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)uFrame.pv, IEM_ACCESS_STACK_R);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        Log7(("iemCImpl_iret_prot: uNewSS=%#06x uNewESP=%#010x\n", uNewSS, uNewESP));

        /* Read the SS descriptor. */
        if (!(uNewSS & X86_SEL_MASK_OFF_RPL))
        {
            Log(("iret %04x:%08x/%04x:%08x -> invalid SS selector, #GP(0)\n", uNewCs, uNewEip, uNewSS, uNewESP));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        IEMSELDESC DescSS;
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescSS, uNewSS, X86_XCPT_GP); /** @todo Correct exception? */
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iret %04x:%08x/%04x:%08x - %Rrc when fetching SS\n",
                 uNewCs, uNewEip, uNewSS, uNewESP, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* Privilege checks. */
        if ((uNewSS & X86_SEL_RPL) != (uNewCs & X86_SEL_RPL))
        {
            Log(("iret %04x:%08x/%04x:%08x -> SS.RPL != CS.RPL -> #GP\n", uNewCs, uNewEip, uNewSS, uNewESP));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSS);
        }
        if (DescSS.Legacy.Gen.u2Dpl != (uNewCs & X86_SEL_RPL))
        {
            Log(("iret %04x:%08x/%04x:%08x -> SS.DPL (%d) != CS.RPL -> #GP\n",
                 uNewCs, uNewEip, uNewSS, uNewESP, DescSS.Legacy.Gen.u2Dpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSS);
        }

        /* Must be a writeable data segment descriptor. */
        if (!DescSS.Legacy.Gen.u1DescType)
        {
            Log(("iret %04x:%08x/%04x:%08x -> SS is system segment (%#x) -> #GP\n",
                 uNewCs, uNewEip, uNewSS, uNewESP, DescSS.Legacy.Gen.u4Type));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSS);
        }
        if ((DescSS.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE)) != X86_SEL_TYPE_WRITE)
        {
            Log(("iret %04x:%08x/%04x:%08x - not writable data segment (%#x) -> #GP\n",
                 uNewCs, uNewEip, uNewSS, uNewESP, DescSS.Legacy.Gen.u4Type));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSS);
        }

        /* Present? */
        if (!DescSS.Legacy.Gen.u1Present)
        {
            Log(("iret %04x:%08x/%04x:%08x -> SS not present -> #SS\n", uNewCs, uNewEip, uNewSS, uNewESP));
            return iemRaiseStackSelectorNotPresentBySelector(pVCpu, uNewSS);
        }

        uint32_t cbLimitSs = X86DESC_LIMIT_G(&DescSS.Legacy);

        /* Check EIP. */
        if (uNewEip > cbLimitCS)
        {
            Log(("iret %04x:%08x/%04x:%08x -> EIP is out of bounds (%#x) -> #GP(0)\n",
                 uNewCs, uNewEip, uNewSS, uNewESP, cbLimitCS));
            /** @todo: Which is it, #GP(0) or #GP(sel)? */
            return iemRaiseSelectorBoundsBySelector(pVCpu, uNewCs);
        }

        /*
         * Commit the changes, marking CS and SS accessed first since
         * that may fail.
         */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCs);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }
        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewSS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescSS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        uint32_t fEFlagsMask = X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF
                             | X86_EFL_TF | X86_EFL_DF | X86_EFL_OF | X86_EFL_NT;
        if (enmEffOpSize != IEMMODE_16BIT)
            fEFlagsMask |= X86_EFL_RF | X86_EFL_AC | X86_EFL_ID;
        if (pVCpu->iem.s.uCpl == 0)
            fEFlagsMask |= X86_EFL_IF | X86_EFL_IOPL | X86_EFL_VIF | X86_EFL_VIP; /* VM is 0 */
        else if (pVCpu->iem.s.uCpl <= pCtx->eflags.Bits.u2IOPL)
            fEFlagsMask |= X86_EFL_IF;
        if (IEM_GET_TARGET_CPU(pVCpu) <= IEMTARGETCPU_386)
            fEFlagsMask &= ~(X86_EFL_AC | X86_EFL_ID | X86_EFL_VIF | X86_EFL_VIP);
        uint32_t fEFlagsNew = IEMMISC_GET_EFL(pVCpu, pCtx);
        fEFlagsNew         &= ~fEFlagsMask;
        fEFlagsNew         |= uNewFlags & fEFlagsMask;
#ifdef DBGFTRACE_ENABLED
        RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "iret/%up%u %04x:%08x -> %04x:%04x %x %04x:%04x",
                          pVCpu->iem.s.uCpl, uNewCs & X86_SEL_RPL, pCtx->cs.Sel, pCtx->eip,
                          uNewCs, uNewEip, uNewFlags,  uNewSS, uNewESP);
#endif

        IEMMISC_SET_EFL(pVCpu, pCtx, fEFlagsNew);
        pCtx->rip           = uNewEip;
        pCtx->cs.Sel        = uNewCs;
        pCtx->cs.ValidSel   = uNewCs;
        pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.Attr.u     = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
        pCtx->cs.u32Limit   = cbLimitCS;
        pCtx->cs.u64Base    = X86DESC_BASE(&DescCS.Legacy);
        pVCpu->iem.s.enmCpuMode = iemCalcCpuMode(pCtx);

        pCtx->ss.Sel        = uNewSS;
        pCtx->ss.ValidSel   = uNewSS;
        pCtx->ss.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->ss.Attr.u     = X86DESC_GET_HID_ATTR(&DescSS.Legacy);
        pCtx->ss.u32Limit   = cbLimitSs;
        pCtx->ss.u64Base    = X86DESC_BASE(&DescSS.Legacy);
        if (!pCtx->ss.Attr.n.u1DefBig)
            pCtx->sp        = (uint16_t)uNewESP;
        else
            pCtx->rsp       = uNewESP;

        pVCpu->iem.s.uCpl       = uNewCs & X86_SEL_RPL;
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->ds);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->es);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->fs);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCs & X86_SEL_RPL, &pCtx->gs);

        /* Done! */

    }
    /*
     * Return to the same level.
     */
    else
    {
        /* Check EIP. */
        if (uNewEip > cbLimitCS)
        {
            Log(("iret %04x:%08x - EIP is out of bounds (%#x) -> #GP(0)\n", uNewCs, uNewEip, cbLimitCS));
            /** @todo: Which is it, #GP(0) or #GP(sel)? */
            return iemRaiseSelectorBoundsBySelector(pVCpu, uNewCs);
        }

        /*
         * Commit the changes, marking CS first since it may fail.
         */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCs);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        X86EFLAGS NewEfl;
        NewEfl.u = IEMMISC_GET_EFL(pVCpu, pCtx);
        uint32_t fEFlagsMask = X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF
                             | X86_EFL_TF | X86_EFL_DF | X86_EFL_OF | X86_EFL_NT;
        if (enmEffOpSize != IEMMODE_16BIT)
            fEFlagsMask |= X86_EFL_RF | X86_EFL_AC | X86_EFL_ID;
        if (pVCpu->iem.s.uCpl == 0)
            fEFlagsMask |= X86_EFL_IF | X86_EFL_IOPL | X86_EFL_VIF | X86_EFL_VIP; /* VM is 0 */
        else if (pVCpu->iem.s.uCpl <= NewEfl.Bits.u2IOPL)
            fEFlagsMask |= X86_EFL_IF;
        if (IEM_GET_TARGET_CPU(pVCpu) <= IEMTARGETCPU_386)
            fEFlagsMask &= ~(X86_EFL_AC | X86_EFL_ID | X86_EFL_VIF | X86_EFL_VIP);
        NewEfl.u           &= ~fEFlagsMask;
        NewEfl.u           |= fEFlagsMask & uNewFlags;
#ifdef DBGFTRACE_ENABLED
        RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "iret/%up %04x:%08x -> %04x:%04x %x %04x:%04llx",
                          pVCpu->iem.s.uCpl, pCtx->cs.Sel, pCtx->eip,
                          uNewCs, uNewEip, uNewFlags, pCtx->ss.Sel, uNewRsp);
#endif

        IEMMISC_SET_EFL(pVCpu, pCtx, NewEfl.u);
        pCtx->rip           = uNewEip;
        pCtx->cs.Sel        = uNewCs;
        pCtx->cs.ValidSel   = uNewCs;
        pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->cs.Attr.u     = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
        pCtx->cs.u32Limit   = cbLimitCS;
        pCtx->cs.u64Base    = X86DESC_BASE(&DescCS.Legacy);
        pVCpu->iem.s.enmCpuMode = iemCalcCpuMode(pCtx);
        if (!pCtx->ss.Attr.n.u1DefBig)
            pCtx->sp        = (uint16_t)uNewRsp;
        else
            pCtx->rsp       = uNewRsp;
        /* Done! */
    }

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Implements iret for long mode
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_iret_64bit, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    NOREF(cbInstr);

    /*
     * Nested task return is not supported in long mode.
     */
    if (pCtx->eflags.Bits.u1NT)
    {
        Log(("iretq with NT=1 (eflags=%#x) -> #GP(0)\n", pCtx->eflags.u));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Normal return.
     *
     * Do the stack bits, but don't commit RSP before everything checks
     * out right.
     */
    VBOXSTRICTRC    rcStrict;
    RTCPTRUNION     uFrame;
    uint64_t        uNewRip;
    uint16_t        uNewCs;
    uint16_t        uNewSs;
    uint32_t        uNewFlags;
    uint64_t        uNewRsp;
    if (enmEffOpSize == IEMMODE_64BIT)
    {
        rcStrict = iemMemStackPopBeginSpecial(pVCpu, 5*8, &uFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        uNewRip    = uFrame.pu64[0];
        uNewCs     = (uint16_t)uFrame.pu64[1];
        uNewFlags  = (uint32_t)uFrame.pu64[2];
        uNewRsp    = uFrame.pu64[3];
        uNewSs     = (uint16_t)uFrame.pu64[4];
    }
    else if (enmEffOpSize == IEMMODE_32BIT)
    {
        rcStrict = iemMemStackPopBeginSpecial(pVCpu, 5*4, &uFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        uNewRip    = uFrame.pu32[0];
        uNewCs     = (uint16_t)uFrame.pu32[1];
        uNewFlags  = uFrame.pu32[2];
        uNewRsp    = uFrame.pu32[3];
        uNewSs     = (uint16_t)uFrame.pu32[4];
    }
    else
    {
        Assert(enmEffOpSize == IEMMODE_16BIT);
        rcStrict = iemMemStackPopBeginSpecial(pVCpu, 5*2, &uFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        uNewRip    = uFrame.pu16[0];
        uNewCs     = uFrame.pu16[1];
        uNewFlags  = uFrame.pu16[2];
        uNewRsp    = uFrame.pu16[3];
        uNewSs     = uFrame.pu16[4];
    }
    rcStrict = iemMemStackPopDoneSpecial(pVCpu, (void *)uFrame.pv); /* don't use iemMemStackPopCommitSpecial here. */
    if (RT_LIKELY(rcStrict == VINF_SUCCESS))
    { /* extremely like */ }
    else
        return rcStrict;
    Log7(("iretq stack: cs:rip=%04x:%016RX64 rflags=%016RX64 ss:rsp=%04x:%016RX64\n", uNewCs, uNewRip, uNewFlags, uNewSs, uNewRsp));

    /*
     * Check stuff.
     */
    /* Read the CS descriptor. */
    if (!(uNewCs & X86_SEL_MASK_OFF_RPL))
    {
        Log(("iret %04x:%016RX64/%04x:%016RX64 -> invalid CS selector, #GP(0)\n", uNewCs, uNewRip, uNewSs, uNewRsp));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    IEMSELDESC DescCS;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, uNewCs, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iret %04x:%016RX64/%04x:%016RX64 - rcStrict=%Rrc when fetching CS\n",
             uNewCs, uNewRip, uNewSs, uNewRsp, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a code descriptor. */
    if (   !DescCS.Legacy.Gen.u1DescType
        || !(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
    {
        Log(("iret %04x:%016RX64/%04x:%016RX64 - CS is not a code segment T=%u T=%#xu -> #GP\n",
             uNewCs, uNewRip, uNewSs, uNewRsp, DescCS.Legacy.Gen.u1DescType, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }

    /* Privilege checks. */
    uint8_t const uNewCpl = uNewCs & X86_SEL_RPL;
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF))
    {
        if ((uNewCs & X86_SEL_RPL) != DescCS.Legacy.Gen.u2Dpl)
        {
            Log(("iret %04x:%016RX64 - RPL != DPL (%d) -> #GP\n", uNewCs, uNewRip, DescCS.Legacy.Gen.u2Dpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
        }
    }
    else if ((uNewCs & X86_SEL_RPL) < DescCS.Legacy.Gen.u2Dpl)
    {
        Log(("iret %04x:%016RX64 - RPL < DPL (%d) -> #GP\n", uNewCs, uNewRip, DescCS.Legacy.Gen.u2Dpl));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }
    if ((uNewCs & X86_SEL_RPL) < pVCpu->iem.s.uCpl)
    {
        Log(("iret %04x:%016RX64 - RPL < CPL (%d) -> #GP\n", uNewCs, uNewRip, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewCs);
    }

    /* Present? */
    if (!DescCS.Legacy.Gen.u1Present)
    {
        Log(("iret %04x:%016RX64/%04x:%016RX64 - CS not present -> #NP\n", uNewCs, uNewRip, uNewSs, uNewRsp));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uNewCs);
    }

    uint32_t cbLimitCS = X86DESC_LIMIT_G(&DescCS.Legacy);

    /* Read the SS descriptor. */
    IEMSELDESC DescSS;
    if (!(uNewSs & X86_SEL_MASK_OFF_RPL))
    {
        if (   !DescCS.Legacy.Gen.u1Long
            || DescCS.Legacy.Gen.u1DefBig /** @todo exactly how does iret (and others) behave with u1Long=1 and u1DefBig=1? \#GP(sel)? */
            || uNewCpl > 2) /** @todo verify SS=0 impossible for ring-3. */
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 -> invalid SS selector, #GP(0)\n", uNewCs, uNewRip, uNewSs, uNewRsp));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }
        DescSS.Legacy.u = 0;
    }
    else
    {
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescSS, uNewSs, X86_XCPT_GP); /** @todo Correct exception? */
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 - %Rrc when fetching SS\n",
                 uNewCs, uNewRip, uNewSs, uNewRsp, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }

    /* Privilege checks. */
    if ((uNewSs & X86_SEL_RPL) != (uNewCs & X86_SEL_RPL))
    {
        Log(("iret %04x:%016RX64/%04x:%016RX64 -> SS.RPL != CS.RPL -> #GP\n", uNewCs, uNewRip, uNewSs, uNewRsp));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSs);
    }

    uint32_t cbLimitSs;
    if (!(uNewSs & X86_SEL_MASK_OFF_RPL))
        cbLimitSs = UINT32_MAX;
    else
    {
        if (DescSS.Legacy.Gen.u2Dpl != (uNewCs & X86_SEL_RPL))
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 -> SS.DPL (%d) != CS.RPL -> #GP\n",
                 uNewCs, uNewRip, uNewSs, uNewRsp, DescSS.Legacy.Gen.u2Dpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSs);
        }

        /* Must be a writeable data segment descriptor. */
        if (!DescSS.Legacy.Gen.u1DescType)
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 -> SS is system segment (%#x) -> #GP\n",
                 uNewCs, uNewRip, uNewSs, uNewRsp, DescSS.Legacy.Gen.u4Type));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSs);
        }
        if ((DescSS.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE)) != X86_SEL_TYPE_WRITE)
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 - not writable data segment (%#x) -> #GP\n",
                 uNewCs, uNewRip, uNewSs, uNewRsp, DescSS.Legacy.Gen.u4Type));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewSs);
        }

        /* Present? */
        if (!DescSS.Legacy.Gen.u1Present)
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 -> SS not present -> #SS\n", uNewCs, uNewRip, uNewSs, uNewRsp));
            return iemRaiseStackSelectorNotPresentBySelector(pVCpu, uNewSs);
        }
        cbLimitSs = X86DESC_LIMIT_G(&DescSS.Legacy);
    }

    /* Check EIP. */
    if (DescCS.Legacy.Gen.u1Long)
    {
        if (!IEM_IS_CANONICAL(uNewRip))
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 -> RIP is not canonical -> #GP(0)\n",
                 uNewCs, uNewRip, uNewSs, uNewRsp));
            return iemRaiseSelectorBoundsBySelector(pVCpu, uNewCs);
        }
    }
    else
    {
        if (uNewRip > cbLimitCS)
        {
            Log(("iret %04x:%016RX64/%04x:%016RX64 -> EIP is out of bounds (%#x) -> #GP(0)\n",
                 uNewCs, uNewRip, uNewSs, uNewRsp, cbLimitCS));
            /** @todo: Which is it, #GP(0) or #GP(sel)? */
            return iemRaiseSelectorBoundsBySelector(pVCpu, uNewCs);
        }
    }

    /*
     * Commit the changes, marking CS and SS accessed first since
     * that may fail.
     */
    /** @todo where exactly are these actually marked accessed by a real CPU? */
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCs);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }
    if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewSs);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        DescSS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    uint32_t fEFlagsMask = X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF
                         | X86_EFL_TF | X86_EFL_DF | X86_EFL_OF | X86_EFL_NT;
    if (enmEffOpSize != IEMMODE_16BIT)
        fEFlagsMask |= X86_EFL_RF | X86_EFL_AC | X86_EFL_ID;
    if (pVCpu->iem.s.uCpl == 0)
        fEFlagsMask |= X86_EFL_IF | X86_EFL_IOPL | X86_EFL_VIF | X86_EFL_VIP; /* VM is ignored */
    else if (pVCpu->iem.s.uCpl <= pCtx->eflags.Bits.u2IOPL)
        fEFlagsMask |= X86_EFL_IF;
    uint32_t fEFlagsNew = IEMMISC_GET_EFL(pVCpu, pCtx);
    fEFlagsNew         &= ~fEFlagsMask;
    fEFlagsNew         |= uNewFlags & fEFlagsMask;
#ifdef DBGFTRACE_ENABLED
    RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "iret/%ul%u %08llx -> %04x:%04llx %llx %04x:%04llx",
                      pVCpu->iem.s.uCpl, uNewCpl, pCtx->rip, uNewCs, uNewRip, uNewFlags, uNewSs, uNewRsp);
#endif

    IEMMISC_SET_EFL(pVCpu, pCtx, fEFlagsNew);
    pCtx->rip           = uNewRip;
    pCtx->cs.Sel        = uNewCs;
    pCtx->cs.ValidSel   = uNewCs;
    pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.Attr.u     = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
    pCtx->cs.u32Limit   = cbLimitCS;
    pCtx->cs.u64Base    = X86DESC_BASE(&DescCS.Legacy);
    pVCpu->iem.s.enmCpuMode = iemCalcCpuMode(pCtx);
    if (pCtx->cs.Attr.n.u1Long || pCtx->cs.Attr.n.u1DefBig)
        pCtx->rsp       = uNewRsp;
    else
        pCtx->sp        = (uint16_t)uNewRsp;
    pCtx->ss.Sel        = uNewSs;
    pCtx->ss.ValidSel   = uNewSs;
    if (!(uNewSs & X86_SEL_MASK_OFF_RPL))
    {
        pCtx->ss.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->ss.Attr.u     = X86DESCATTR_UNUSABLE | (uNewCpl << X86DESCATTR_DPL_SHIFT);
        pCtx->ss.u32Limit   = UINT32_MAX;
        pCtx->ss.u64Base    = 0;
        Log2(("iretq new SS: NULL\n"));
    }
    else
    {
        pCtx->ss.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->ss.Attr.u     = X86DESC_GET_HID_ATTR(&DescSS.Legacy);
        pCtx->ss.u32Limit   = cbLimitSs;
        pCtx->ss.u64Base    = X86DESC_BASE(&DescSS.Legacy);
        Log2(("iretq new SS: base=%#RX64 lim=%#x attr=%#x\n", pCtx->ss.u64Base, pCtx->ss.u32Limit, pCtx->ss.Attr.u));
    }

    if (pVCpu->iem.s.uCpl != uNewCpl)
    {
        pVCpu->iem.s.uCpl = uNewCpl;
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCpl, &pCtx->ds);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCpl, &pCtx->es);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCpl, &pCtx->fs);
        iemHlpAdjustSelectorForNewCpl(pVCpu, uNewCpl, &pCtx->gs);
    }

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Implements iret.
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_1(iemCImpl_iret, IEMMODE, enmEffOpSize)
{
    /*
     * First, clear NMI blocking, if any, before causing any exceptions.
     */
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);

    /*
     * The SVM nested-guest intercept for iret takes priority over all exceptions,
     * see AMD spec. "15.9 Instruction Intercepts".
     */
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IRET))
    {
        Log(("iret: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_IRET, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Call a mode specific worker.
     */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
        return IEM_CIMPL_CALL_1(iemCImpl_iret_real_v8086, enmEffOpSize);
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        return IEM_CIMPL_CALL_1(iemCImpl_iret_64bit, enmEffOpSize);
    return     IEM_CIMPL_CALL_1(iemCImpl_iret_prot, enmEffOpSize);
}


/**
 * Implements SYSCALL (AMD and Intel64).
 *
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_0(iemCImpl_syscall)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     *
     * Note that CPUs described in the documentation may load a few odd values
     * into CS and SS than we allow here.  This has yet to be checked on real
     * hardware.
     */
    if (!(pCtx->msrEFER & MSR_K6_EFER_SCE))
    {
        Log(("syscall: Not enabled in EFER -> #UD\n"));
        return iemRaiseUndefinedOpcode(pVCpu);
    }
    if (!(pCtx->cr0 & X86_CR0_PE))
    {
        Log(("syscall: Protected mode is required -> #GP(0)\n"));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu) && !CPUMIsGuestInLongModeEx(pCtx))
    {
        Log(("syscall: Only available in long mode on intel -> #UD\n"));
        return iemRaiseUndefinedOpcode(pVCpu);
    }

    /** @todo verify RPL ignoring and CS=0xfff8 (i.e. SS == 0). */
    /** @todo what about LDT selectors? Shouldn't matter, really. */
    uint16_t uNewCs = (pCtx->msrSTAR >> MSR_K6_STAR_SYSCALL_CS_SS_SHIFT) & X86_SEL_MASK_OFF_RPL;
    uint16_t uNewSs = uNewCs + 8;
    if (uNewCs == 0 || uNewSs == 0)
    {
        Log(("syscall: msrSTAR.CS = 0 or SS = 0 -> #GP(0)\n"));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Long mode and legacy mode differs. */
    if (CPUMIsGuestInLongModeEx(pCtx))
    {
        uint64_t uNewRip = pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT ? pCtx->msrLSTAR : pCtx-> msrCSTAR;

        /* This test isn't in the docs, but I'm not trusting the guys writing
           the MSRs to have validated the values as canonical like they should. */
        if (!IEM_IS_CANONICAL(uNewRip))
        {
            Log(("syscall: Only available in long mode on intel -> #UD\n"));
            return iemRaiseUndefinedOpcode(pVCpu);
        }

        /*
         * Commit it.
         */
        Log(("syscall: %04x:%016RX64 [efl=%#llx] -> %04x:%016RX64\n", pCtx->cs, pCtx->rip, pCtx->rflags.u, uNewCs, uNewRip));
        pCtx->rcx           = pCtx->rip + cbInstr;
        pCtx->rip           = uNewRip;

        pCtx->rflags.u     &= ~X86_EFL_RF;
        pCtx->r11           = pCtx->rflags.u;
        pCtx->rflags.u     &= ~pCtx->msrSFMASK;
        pCtx->rflags.u     |= X86_EFL_1;

        pCtx->cs.Attr.u     = X86DESCATTR_P | X86DESCATTR_G | X86DESCATTR_L | X86DESCATTR_DT | X86_SEL_TYPE_ER_ACC;
        pCtx->ss.Attr.u     = X86DESCATTR_P | X86DESCATTR_G | X86DESCATTR_L | X86DESCATTR_DT | X86_SEL_TYPE_RW_ACC;
    }
    else
    {
        /*
         * Commit it.
         */
        Log(("syscall: %04x:%08RX32 [efl=%#x] -> %04x:%08RX32\n",
             pCtx->cs, pCtx->eip, pCtx->eflags.u, uNewCs, (uint32_t)(pCtx->msrSTAR & MSR_K6_STAR_SYSCALL_EIP_MASK)));
        pCtx->rcx           = pCtx->eip + cbInstr;
        pCtx->rip           = pCtx->msrSTAR & MSR_K6_STAR_SYSCALL_EIP_MASK;
        pCtx->rflags.u     &= ~(X86_EFL_VM | X86_EFL_IF | X86_EFL_RF);

        pCtx->cs.Attr.u     = X86DESCATTR_P | X86DESCATTR_G | X86DESCATTR_D | X86DESCATTR_DT | X86_SEL_TYPE_ER_ACC;
        pCtx->ss.Attr.u     = X86DESCATTR_P | X86DESCATTR_G | X86DESCATTR_D | X86DESCATTR_DT | X86_SEL_TYPE_RW_ACC;
    }
    pCtx->cs.Sel        = uNewCs;
    pCtx->cs.ValidSel   = uNewCs;
    pCtx->cs.u64Base    = 0;
    pCtx->cs.u32Limit   = UINT32_MAX;
    pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;

    pCtx->ss.Sel        = uNewSs;
    pCtx->ss.ValidSel   = uNewSs;
    pCtx->ss.u64Base    = 0;
    pCtx->ss.u32Limit   = UINT32_MAX;
    pCtx->ss.fFlags     = CPUMSELREG_FLAGS_VALID;

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Implements SYSRET (AMD and Intel64).
 */
IEM_CIMPL_DEF_0(iemCImpl_sysret)

{
    RT_NOREF_PV(cbInstr);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     *
     * Note that CPUs described in the documentation may load a few odd values
     * into CS and SS than we allow here.  This has yet to be checked on real
     * hardware.
     */
    if (!(pCtx->msrEFER & MSR_K6_EFER_SCE))
    {
        Log(("sysret: Not enabled in EFER -> #UD\n"));
        return iemRaiseUndefinedOpcode(pVCpu);
    }
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu) && !CPUMIsGuestInLongModeEx(pCtx))
    {
        Log(("sysret: Only available in long mode on intel -> #UD\n"));
        return iemRaiseUndefinedOpcode(pVCpu);
    }
    if (!(pCtx->cr0 & X86_CR0_PE))
    {
        Log(("sysret: Protected mode is required -> #GP(0)\n"));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    if (pVCpu->iem.s.uCpl != 0)
    {
        Log(("sysret: CPL must be 0 not %u -> #GP(0)\n", pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /** @todo Does SYSRET verify CS != 0 and SS != 0? Neither is valid in ring-3. */
    uint16_t uNewCs = (pCtx->msrSTAR >> MSR_K6_STAR_SYSRET_CS_SS_SHIFT) & X86_SEL_MASK_OFF_RPL;
    uint16_t uNewSs = uNewCs + 8;
    if (pVCpu->iem.s.enmEffOpSize == IEMMODE_64BIT)
        uNewCs += 16;
    if (uNewCs == 0 || uNewSs == 0)
    {
        Log(("sysret: msrSTAR.CS = 0 or SS = 0 -> #GP(0)\n"));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Commit it.
     */
    if (CPUMIsGuestInLongModeEx(pCtx))
    {
        if (pVCpu->iem.s.enmEffOpSize == IEMMODE_64BIT)
        {
            Log(("sysret: %04x:%016RX64 [efl=%#llx] -> %04x:%016RX64 [r11=%#llx]\n",
                 pCtx->cs, pCtx->rip, pCtx->rflags.u, uNewCs, pCtx->rcx, pCtx->r11));
            /* Note! We disregard intel manual regarding the RCX cananonical
                     check, ask intel+xen why AMD doesn't do it. */
            pCtx->rip       = pCtx->rcx;
            pCtx->cs.Attr.u = X86DESCATTR_P | X86DESCATTR_G | X86DESCATTR_L | X86DESCATTR_DT | X86_SEL_TYPE_ER_ACC
                            | (3 << X86DESCATTR_DPL_SHIFT);
        }
        else
        {
            Log(("sysret: %04x:%016RX64 [efl=%#llx] -> %04x:%08RX32 [r11=%#llx]\n",
                 pCtx->cs, pCtx->rip, pCtx->rflags.u, uNewCs, pCtx->ecx, pCtx->r11));
            pCtx->rip       = pCtx->ecx;
            pCtx->cs.Attr.u = X86DESCATTR_P | X86DESCATTR_G | X86DESCATTR_D | X86DESCATTR_DT | X86_SEL_TYPE_ER_ACC
                            | (3 << X86DESCATTR_DPL_SHIFT);
        }
        /** @todo testcase: See what kind of flags we can make SYSRET restore and
         *        what it really ignores. RF and VM are hinted at being zero, by AMD. */
        pCtx->rflags.u      = pCtx->r11 & (X86_EFL_POPF_BITS | X86_EFL_VIF | X86_EFL_VIP);
        pCtx->rflags.u     |= X86_EFL_1;
    }
    else
    {
        Log(("sysret: %04x:%08RX32 [efl=%#x] -> %04x:%08RX32\n", pCtx->cs, pCtx->eip, pCtx->eflags.u, uNewCs, pCtx->ecx));
        pCtx->rip           = pCtx->rcx;
        pCtx->rflags.u     |= X86_EFL_IF;
        pCtx->cs.Attr.u     = X86DESCATTR_P | X86DESCATTR_G | X86DESCATTR_D | X86DESCATTR_DT | X86_SEL_TYPE_ER_ACC
                            | (3 << X86DESCATTR_DPL_SHIFT);
    }
    pCtx->cs.Sel        = uNewCs | 3;
    pCtx->cs.ValidSel   = uNewCs | 3;
    pCtx->cs.u64Base    = 0;
    pCtx->cs.u32Limit   = UINT32_MAX;
    pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;

    pCtx->ss.Sel        = uNewSs | 3;
    pCtx->ss.ValidSel   = uNewSs | 3;
    pCtx->ss.fFlags     = CPUMSELREG_FLAGS_VALID;
    /* The SS hidden bits remains unchanged says AMD. To that I say "Yeah, right!". */
    pCtx->ss.Attr.u    |= (3 << X86DESCATTR_DPL_SHIFT);
    /** @todo Testcase: verify that SS.u1Long and SS.u1DefBig are left unchanged
     *        on sysret. */

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    return VINF_SUCCESS;
}


/**
 * Common worker for 'pop SReg', 'mov SReg, GReg' and 'lXs GReg, reg/mem'.
 *
 * @param   iSegReg     The segment register number (valid).
 * @param   uSel        The new selector value.
 */
IEM_CIMPL_DEF_2(iemCImpl_LoadSReg, uint8_t, iSegReg, uint16_t, uSel)
{
    /*PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);*/
    uint16_t       *pSel = iemSRegRef(pVCpu, iSegReg);
    PCPUMSELREGHID  pHid = iemSRegGetHid(pVCpu, iSegReg);

    Assert(iSegReg <= X86_SREG_GS && iSegReg != X86_SREG_CS);

    /*
     * Real mode and V8086 mode are easy.
     */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        *pSel           = uSel;
        pHid->u64Base   = (uint32_t)uSel << 4;
        pHid->ValidSel  = uSel;
        pHid->fFlags    = CPUMSELREG_FLAGS_VALID;
#if 0 /* AMD Volume 2, chapter 4.1 - "real mode segmentation" - states that limit and attributes are untouched. */
        /** @todo Does the CPU actually load limits and attributes in the
         *        real/V8086 mode segment load case?  It doesn't for CS in far
         *        jumps...  Affects unreal mode.  */
        pHid->u32Limit          = 0xffff;
        pHid->Attr.u = 0;
        pHid->Attr.n.u1Present  = 1;
        pHid->Attr.n.u1DescType = 1;
        pHid->Attr.n.u4Type     = iSegReg != X86_SREG_CS
                                ? X86_SEL_TYPE_RW
                                : X86_SEL_TYPE_READ | X86_SEL_TYPE_CODE;
#endif
        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    /*
     * Protected mode.
     *
     * Check if it's a null segment selector value first, that's OK for DS, ES,
     * FS and GS.  If not null, then we have to load and parse the descriptor.
     */
    if (!(uSel & X86_SEL_MASK_OFF_RPL))
    {
        Assert(iSegReg != X86_SREG_CS); /** @todo testcase for \#UD on MOV CS, ax! */
        if (iSegReg == X86_SREG_SS)
        {
            /* In 64-bit kernel mode, the stack can be 0 because of the way
               interrupts are dispatched. AMD seems to have a slighly more
               relaxed relationship to SS.RPL than intel does. */
            /** @todo We cannot 'mov ss, 3' in 64-bit kernel mode, can we? There is a testcase (bs-cpu-xcpt-1), but double check this! */
            if (   pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT
                || pVCpu->iem.s.uCpl > 2
                || (   uSel != pVCpu->iem.s.uCpl
                    && !IEM_IS_GUEST_CPU_AMD(pVCpu)) )
            {
                Log(("load sreg %#x -> invalid stack selector, #GP(0)\n", uSel));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }
        }

        *pSel = uSel;   /* Not RPL, remember :-) */
        iemHlpLoadNullDataSelectorProt(pVCpu, pHid, uSel);
        if (iSegReg == X86_SREG_SS)
            pHid->Attr.u |= pVCpu->iem.s.uCpl << X86DESCATTR_DPL_SHIFT;

        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pHid));
        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);

        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    /* Fetch the descriptor. */
    IEMSELDESC Desc;
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, &Desc, uSel, X86_XCPT_GP); /** @todo Correct exception? */
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Check GPs first. */
    if (!Desc.Legacy.Gen.u1DescType)
    {
        Log(("load sreg %d (=%#x) - system selector (%#x) -> #GP\n", iSegReg, uSel, Desc.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
    }
    if (iSegReg == X86_SREG_SS) /* SS gets different treatment */
    {
        if (    (Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE)
            || !(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_WRITE) )
        {
            Log(("load sreg SS, %#x - code or read only (%#x) -> #GP\n", uSel, Desc.Legacy.Gen.u4Type));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
        if ((uSel & X86_SEL_RPL) != pVCpu->iem.s.uCpl)
        {
            Log(("load sreg SS, %#x - RPL and CPL (%d) differs -> #GP\n", uSel, pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
        if (Desc.Legacy.Gen.u2Dpl != pVCpu->iem.s.uCpl)
        {
            Log(("load sreg SS, %#x - DPL (%d) and CPL (%d) differs -> #GP\n", uSel, Desc.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
    }
    else
    {
        if ((Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ)) == X86_SEL_TYPE_CODE)
        {
            Log(("load sreg%u, %#x - execute only segment -> #GP\n", iSegReg, uSel));
            return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
        }
        if (   (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
            != (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
        {
#if 0 /* this is what intel says. */
            if (   (uSel & X86_SEL_RPL) > Desc.Legacy.Gen.u2Dpl
                && pVCpu->iem.s.uCpl        > Desc.Legacy.Gen.u2Dpl)
            {
                Log(("load sreg%u, %#x - both RPL (%d) and CPL (%d) are greater than DPL (%d) -> #GP\n",
                     iSegReg, uSel, (uSel & X86_SEL_RPL), pVCpu->iem.s.uCpl, Desc.Legacy.Gen.u2Dpl));
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
            }
#else /* this is what makes more sense. */
            if ((unsigned)(uSel & X86_SEL_RPL) > Desc.Legacy.Gen.u2Dpl)
            {
                Log(("load sreg%u, %#x - RPL (%d) is greater than DPL (%d) -> #GP\n",
                     iSegReg, uSel, (uSel & X86_SEL_RPL), Desc.Legacy.Gen.u2Dpl));
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
            }
            if (pVCpu->iem.s.uCpl > Desc.Legacy.Gen.u2Dpl)
            {
                Log(("load sreg%u, %#x - CPL (%d) is greater than DPL (%d) -> #GP\n",
                     iSegReg, uSel, pVCpu->iem.s.uCpl, Desc.Legacy.Gen.u2Dpl));
                return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uSel);
            }
#endif
        }
    }

    /* Is it there? */
    if (!Desc.Legacy.Gen.u1Present)
    {
        Log(("load sreg%d,%#x - segment not present -> #NP\n", iSegReg, uSel));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uSel);
    }

    /* The base and limit. */
    uint32_t cbLimit = X86DESC_LIMIT_G(&Desc.Legacy);
    uint64_t u64Base = X86DESC_BASE(&Desc.Legacy);

    /*
     * Ok, everything checked out fine.  Now set the accessed bit before
     * committing the result into the registers.
     */
    if (!(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, uSel);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        Desc.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    /* commit */
    *pSel = uSel;
    pHid->Attr.u   = X86DESC_GET_HID_ATTR(&Desc.Legacy);
    pHid->u32Limit = cbLimit;
    pHid->u64Base  = u64Base;
    pHid->ValidSel = uSel;
    pHid->fFlags   = CPUMSELREG_FLAGS_VALID;

    /** @todo check if the hidden bits are loaded correctly for 64-bit
     *        mode.  */
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pHid));

    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'mov SReg, r/m'.
 *
 * @param   iSegReg     The segment register number (valid).
 * @param   uSel        The new selector value.
 */
IEM_CIMPL_DEF_2(iemCImpl_load_SReg, uint8_t, iSegReg, uint16_t, uSel)
{
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_LoadSReg, iSegReg, uSel);
    if (rcStrict == VINF_SUCCESS)
    {
        if (iSegReg == X86_SREG_SS)
        {
            PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
            EMSetInhibitInterruptsPC(pVCpu, pCtx->rip);
        }
    }
    return rcStrict;
}


/**
 * Implements 'pop SReg'.
 *
 * @param   iSegReg         The segment register number (valid).
 * @param   enmEffOpSize    The efficient operand size (valid).
 */
IEM_CIMPL_DEF_2(iemCImpl_pop_Sreg, uint8_t, iSegReg, IEMMODE, enmEffOpSize)
{
    PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC    rcStrict;

    /*
     * Read the selector off the stack and join paths with mov ss, reg.
     */
    RTUINT64U TmpRsp;
    TmpRsp.u = pCtx->rsp;
    switch (enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t uSel;
            rcStrict = iemMemStackPopU16Ex(pVCpu, &uSel, &TmpRsp);
            if (rcStrict == VINF_SUCCESS)
                rcStrict = IEM_CIMPL_CALL_2(iemCImpl_LoadSReg, iSegReg, uSel);
            break;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Value;
            rcStrict = iemMemStackPopU32Ex(pVCpu, &u32Value, &TmpRsp);
            if (rcStrict == VINF_SUCCESS)
                rcStrict = IEM_CIMPL_CALL_2(iemCImpl_LoadSReg, iSegReg, (uint16_t)u32Value);
            break;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Value;
            rcStrict = iemMemStackPopU64Ex(pVCpu, &u64Value, &TmpRsp);
            if (rcStrict == VINF_SUCCESS)
                rcStrict = IEM_CIMPL_CALL_2(iemCImpl_LoadSReg, iSegReg, (uint16_t)u64Value);
            break;
        }
        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    /*
     * Commit the stack on success.
     */
    if (rcStrict == VINF_SUCCESS)
    {
        pCtx->rsp = TmpRsp.u;
        if (iSegReg == X86_SREG_SS)
            EMSetInhibitInterruptsPC(pVCpu, pCtx->rip);
    }
    return rcStrict;
}


/**
 * Implements lgs, lfs, les, lds & lss.
 */
IEM_CIMPL_DEF_5(iemCImpl_load_SReg_Greg,
                uint16_t, uSel,
                uint64_t, offSeg,
                uint8_t,  iSegReg,
                uint8_t,  iGReg,
                IEMMODE,  enmEffOpSize)
{
    /*PCPUMCTX        pCtx = IEM_GET_CTX(pVCpu);*/
    VBOXSTRICTRC    rcStrict;

    /*
     * Use iemCImpl_LoadSReg to do the tricky segment register loading.
     */
    /** @todo verify and test that mov, pop and lXs works the segment
     *        register loading in the exact same way. */
    rcStrict = IEM_CIMPL_CALL_2(iemCImpl_LoadSReg, iSegReg, uSel);
    if (rcStrict == VINF_SUCCESS)
    {
        switch (enmEffOpSize)
        {
            case IEMMODE_16BIT:
                *(uint16_t *)iemGRegRef(pVCpu, iGReg) = offSeg;
                break;
            case IEMMODE_32BIT:
                *(uint64_t *)iemGRegRef(pVCpu, iGReg) = offSeg;
                break;
            case IEMMODE_64BIT:
                *(uint64_t *)iemGRegRef(pVCpu, iGReg) = offSeg;
                break;
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
    }

    return rcStrict;
}


/**
 * Helper for VERR, VERW, LAR, and LSL and loads the descriptor into memory.
 *
 * @retval VINF_SUCCESS on success.
 * @retval VINF_IEM_SELECTOR_NOT_OK if the selector isn't ok.
 * @retval iemMemFetchSysU64 return value.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uSel                The selector value.
 * @param   fAllowSysDesc       Whether system descriptors are OK or not.
 * @param   pDesc               Where to return the descriptor on success.
 */
static VBOXSTRICTRC iemCImpl_LoadDescHelper(PVMCPU pVCpu, uint16_t uSel, bool fAllowSysDesc, PIEMSELDESC pDesc)
{
    pDesc->Long.au64[0] = 0;
    pDesc->Long.au64[1] = 0;

    if (!(uSel & X86_SEL_MASK_OFF_RPL)) /** @todo test this on 64-bit. */
        return VINF_IEM_SELECTOR_NOT_OK;

    /* Within the table limits? */
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    RTGCPTR GCPtrBase;
    if (uSel & X86_SEL_LDT)
    {
        if (   !pCtx->ldtr.Attr.n.u1Present
            || (uSel | X86_SEL_RPL_LDT) > pCtx->ldtr.u32Limit )
            return VINF_IEM_SELECTOR_NOT_OK;
        GCPtrBase = pCtx->ldtr.u64Base;
    }
    else
    {
        if ((uSel | X86_SEL_RPL_LDT) > pCtx->gdtr.cbGdt)
            return VINF_IEM_SELECTOR_NOT_OK;
        GCPtrBase = pCtx->gdtr.pGdt;
    }

    /* Fetch the descriptor. */
    VBOXSTRICTRC rcStrict = iemMemFetchSysU64(pVCpu, &pDesc->Legacy.u, UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK));
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    if (!pDesc->Legacy.Gen.u1DescType)
    {
        if (!fAllowSysDesc)
            return VINF_IEM_SELECTOR_NOT_OK;
        if (CPUMIsGuestInLongModeEx(pCtx))
        {
            rcStrict = iemMemFetchSysU64(pVCpu, &pDesc->Long.au64[1], UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK) + 8);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }

    }

    return VINF_SUCCESS;
}


/**
 * Implements verr (fWrite = false) and verw (fWrite = true).
 */
IEM_CIMPL_DEF_2(iemCImpl_VerX, uint16_t, uSel, bool, fWrite)
{
    Assert(!IEM_IS_REAL_OR_V86_MODE(pVCpu));

    /** @todo figure whether the accessed bit is set or not. */

    bool         fAccessible = true;
    IEMSELDESC   Desc;
    VBOXSTRICTRC rcStrict = iemCImpl_LoadDescHelper(pVCpu, uSel, false /*fAllowSysDesc*/, &Desc);
    if (rcStrict == VINF_SUCCESS)
    {
        /* Check the descriptor, order doesn't matter much here. */
        if (   !Desc.Legacy.Gen.u1DescType
            || !Desc.Legacy.Gen.u1Present)
            fAccessible = false;
        else
        {
            if (  fWrite
                ? (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE)) != X86_SEL_TYPE_WRITE
                : (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ))  == X86_SEL_TYPE_CODE)
                fAccessible = false;

            /** @todo testcase for the conforming behavior. */
            if (   (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
                != (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
            {
                if ((unsigned)(uSel & X86_SEL_RPL) > Desc.Legacy.Gen.u2Dpl)
                    fAccessible = false;
                else if (pVCpu->iem.s.uCpl > Desc.Legacy.Gen.u2Dpl)
                    fAccessible = false;
            }
        }

    }
    else if (rcStrict == VINF_IEM_SELECTOR_NOT_OK)
        fAccessible = false;
    else
        return rcStrict;

    /* commit */
    IEM_GET_CTX(pVCpu)->eflags.Bits.u1ZF = fAccessible;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements LAR and LSL with 64-bit operand size.
 *
 * @returns VINF_SUCCESS.
 * @param   pu16Dst         Pointer to the destination register.
 * @param   uSel            The selector to load details for.
 * @param   fIsLar          true = LAR, false = LSL.
 */
IEM_CIMPL_DEF_3(iemCImpl_LarLsl_u64, uint64_t *, pu64Dst, uint16_t, uSel, bool, fIsLar)
{
    Assert(!IEM_IS_REAL_OR_V86_MODE(pVCpu));

    /** @todo figure whether the accessed bit is set or not. */

    bool         fDescOk = true;
    IEMSELDESC   Desc;
    VBOXSTRICTRC rcStrict = iemCImpl_LoadDescHelper(pVCpu, uSel, true /*fAllowSysDesc*/, &Desc);
    if (rcStrict == VINF_SUCCESS)
    {
        /*
         * Check the descriptor type.
         */
        if (!Desc.Legacy.Gen.u1DescType)
        {
            if (CPUMIsGuestInLongModeEx(IEM_GET_CTX(pVCpu)))
            {
                if (Desc.Long.Gen.u5Zeros)
                    fDescOk = false;
                else
                    switch (Desc.Long.Gen.u4Type)
                    {
                        /** @todo Intel lists 0 as valid for LSL, verify whether that's correct */
                        case AMD64_SEL_TYPE_SYS_TSS_AVAIL:
                        case AMD64_SEL_TYPE_SYS_TSS_BUSY:
                        case AMD64_SEL_TYPE_SYS_LDT: /** @todo Intel lists this as invalid for LAR, AMD and 32-bit does otherwise. */
                            break;
                        case AMD64_SEL_TYPE_SYS_CALL_GATE:
                            fDescOk = fIsLar;
                            break;
                        default:
                            fDescOk = false;
                            break;
                    }
            }
            else
            {
                switch (Desc.Long.Gen.u4Type)
                {
                    case X86_SEL_TYPE_SYS_286_TSS_AVAIL:
                    case X86_SEL_TYPE_SYS_286_TSS_BUSY:
                    case X86_SEL_TYPE_SYS_386_TSS_AVAIL:
                    case X86_SEL_TYPE_SYS_386_TSS_BUSY:
                    case X86_SEL_TYPE_SYS_LDT:
                        break;
                    case X86_SEL_TYPE_SYS_286_CALL_GATE:
                    case X86_SEL_TYPE_SYS_TASK_GATE:
                    case X86_SEL_TYPE_SYS_386_CALL_GATE:
                        fDescOk = fIsLar;
                        break;
                    default:
                        fDescOk = false;
                        break;
                }
            }
        }
        if (fDescOk)
        {
            /*
             * Check the RPL/DPL/CPL interaction..
             */
            /** @todo testcase for the conforming behavior. */
            if (   (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF)) != (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF)
                || !Desc.Legacy.Gen.u1DescType)
            {
                if ((unsigned)(uSel & X86_SEL_RPL) > Desc.Legacy.Gen.u2Dpl)
                    fDescOk = false;
                else if (pVCpu->iem.s.uCpl > Desc.Legacy.Gen.u2Dpl)
                    fDescOk = false;
            }
        }

        if (fDescOk)
        {
            /*
             * All fine, start committing the result.
             */
            if (fIsLar)
                *pu64Dst = Desc.Legacy.au32[1] & UINT32_C(0x00ffff00);
            else
                *pu64Dst = X86DESC_LIMIT_G(&Desc.Legacy);
        }

    }
    else if (rcStrict == VINF_IEM_SELECTOR_NOT_OK)
        fDescOk = false;
    else
        return rcStrict;

    /* commit flags value and advance rip. */
    IEM_GET_CTX(pVCpu)->eflags.Bits.u1ZF = fDescOk;
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);

    return VINF_SUCCESS;
}


/**
 * Implements LAR and LSL with 16-bit operand size.
 *
 * @returns VINF_SUCCESS.
 * @param   pu16Dst         Pointer to the destination register.
 * @param   u16Sel          The selector to load details for.
 * @param   fIsLar          true = LAR, false = LSL.
 */
IEM_CIMPL_DEF_3(iemCImpl_LarLsl_u16, uint16_t *, pu16Dst, uint16_t, uSel, bool, fIsLar)
{
    uint64_t u64TmpDst = *pu16Dst;
    IEM_CIMPL_CALL_3(iemCImpl_LarLsl_u64, &u64TmpDst, uSel, fIsLar);
    *pu16Dst = u64TmpDst;
    return VINF_SUCCESS;
}


/**
 * Implements lgdt.
 *
 * @param   iEffSeg         The segment of the new gdtr contents
 * @param   GCPtrEffSrc     The address of the new gdtr contents.
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_3(iemCImpl_lgdt, uint8_t, iEffSeg, RTGCPTR, GCPtrEffSrc, IEMMODE, enmEffOpSize)
{
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!IEM_GET_CTX(pVCpu)->eflags.Bits.u1VM);

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_GDTR_WRITES))
    {
        Log(("lgdt: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_GDTR_WRITE, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Fetch the limit and base address.
     */
    uint16_t cbLimit;
    RTGCPTR  GCPtrBase;
    VBOXSTRICTRC rcStrict = iemMemFetchDataXdtr(pVCpu, &cbLimit, &GCPtrBase, iEffSeg, GCPtrEffSrc, enmEffOpSize);
    if (rcStrict == VINF_SUCCESS)
    {
        if (   pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT
            || X86_IS_CANONICAL(GCPtrBase))
        {
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
                rcStrict = CPUMSetGuestGDTR(pVCpu, GCPtrBase, cbLimit);
            else
            {
                PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
                pCtx->gdtr.cbGdt = cbLimit;
                pCtx->gdtr.pGdt  = GCPtrBase;
            }
            if (rcStrict == VINF_SUCCESS)
                iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
        else
        {
            Log(("iemCImpl_lgdt: Non-canonical base %04x:%RGv\n", cbLimit, GCPtrBase));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }
    }
    return rcStrict;
}


/**
 * Implements sgdt.
 *
 * @param   iEffSeg         The segment where to store the gdtr content.
 * @param   GCPtrEffDst     The address where to store the gdtr content.
 */
IEM_CIMPL_DEF_2(iemCImpl_sgdt, uint8_t, iEffSeg, RTGCPTR, GCPtrEffDst)
{
    /*
     * Join paths with sidt.
     * Note! No CPL or V8086 checks here, it's a really sad story, ask Intel if
     *       you really must know.
     */
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC rcStrict = iemMemStoreDataXdtr(pVCpu, pCtx->gdtr.cbGdt, pCtx->gdtr.pGdt, iEffSeg, GCPtrEffDst);
    if (rcStrict == VINF_SUCCESS)
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return rcStrict;
}


/**
 * Implements lidt.
 *
 * @param   iEffSeg         The segment of the new idtr contents
 * @param   GCPtrEffSrc     The address of the new idtr contents.
 * @param   enmEffOpSize    The effective operand size.
 */
IEM_CIMPL_DEF_3(iemCImpl_lidt, uint8_t, iEffSeg, RTGCPTR, GCPtrEffSrc, IEMMODE, enmEffOpSize)
{
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!IEM_GET_CTX(pVCpu)->eflags.Bits.u1VM);

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IDTR_WRITES))
    {
        Log(("lidt: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_IDTR_WRITE, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Fetch the limit and base address.
     */
    uint16_t cbLimit;
    RTGCPTR  GCPtrBase;
    VBOXSTRICTRC rcStrict = iemMemFetchDataXdtr(pVCpu, &cbLimit, &GCPtrBase, iEffSeg, GCPtrEffSrc, enmEffOpSize);
    if (rcStrict == VINF_SUCCESS)
    {
        if (   pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT
            || X86_IS_CANONICAL(GCPtrBase))
        {
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
                CPUMSetGuestIDTR(pVCpu, GCPtrBase, cbLimit);
            else
            {
                PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
                pCtx->idtr.cbIdt = cbLimit;
                pCtx->idtr.pIdt  = GCPtrBase;
            }
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
        else
        {
            Log(("iemCImpl_lidt: Non-canonical base %04x:%RGv\n", cbLimit, GCPtrBase));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }
    }
    return rcStrict;
}


/**
 * Implements sidt.
 *
 * @param   iEffSeg         The segment where to store the idtr content.
 * @param   GCPtrEffDst     The address where to store the idtr content.
 */
IEM_CIMPL_DEF_2(iemCImpl_sidt, uint8_t, iEffSeg, RTGCPTR, GCPtrEffDst)
{
    /*
     * Join paths with sgdt.
     * Note! No CPL or V8086 checks here, it's a really sad story, ask Intel if
     *       you really must know.
     */
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC rcStrict = iemMemStoreDataXdtr(pVCpu, pCtx->idtr.cbIdt, pCtx->idtr.pIdt, iEffSeg, GCPtrEffDst);
    if (rcStrict == VINF_SUCCESS)
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return rcStrict;
}


/**
 * Implements lldt.
 *
 * @param   uNewLdt     The new LDT selector value.
 */
IEM_CIMPL_DEF_1(iemCImpl_lldt, uint16_t, uNewLdt)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        Log(("lldt %04x - real or v8086 mode -> #GP(0)\n", uNewLdt));
        return iemRaiseUndefinedOpcode(pVCpu);
    }
    if (pVCpu->iem.s.uCpl != 0)
    {
        Log(("lldt %04x - CPL is %d -> #GP(0)\n", uNewLdt, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    if (uNewLdt & X86_SEL_LDT)
    {
        Log(("lldt %04x - LDT selector -> #GP\n", uNewLdt));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewLdt);
    }

    /*
     * Now, loading a NULL selector is easy.
     */
    if (!(uNewLdt & X86_SEL_MASK_OFF_RPL))
    {
        /* Nested-guest SVM intercept. */
        if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_LDTR_WRITES))
        {
            Log(("lldt: Guest intercept -> #VMEXIT\n"));
            IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_LDTR_WRITE, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        Log(("lldt %04x: Loading NULL selector.\n",  uNewLdt));
        if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
            CPUMSetGuestLDTR(pVCpu, uNewLdt);
        else
            pCtx->ldtr.Sel = uNewLdt;
        pCtx->ldtr.ValidSel = uNewLdt;
        pCtx->ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
        if (IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
        {
            pCtx->ldtr.Attr.u = X86DESCATTR_UNUSABLE;
            pCtx->ldtr.u64Base = pCtx->ldtr.u32Limit = 0; /* For verfication against REM. */
        }
        else if (IEM_IS_GUEST_CPU_AMD(pVCpu))
        {
            /* AMD-V seems to leave the base and limit alone. */
            pCtx->ldtr.Attr.u = X86DESCATTR_UNUSABLE;
        }
        else if (!IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
        {
            /* VT-x (Intel 3960x) seems to be doing the following. */
            pCtx->ldtr.Attr.u   = X86DESCATTR_UNUSABLE | X86DESCATTR_G | X86DESCATTR_D;
            pCtx->ldtr.u64Base  = 0;
            pCtx->ldtr.u32Limit = UINT32_MAX;
        }

        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    /*
     * Read the descriptor.
     */
    IEMSELDESC Desc;
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, &Desc, uNewLdt, X86_XCPT_GP); /** @todo Correct exception? */
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Check GPs first. */
    if (Desc.Legacy.Gen.u1DescType)
    {
        Log(("lldt %#x - not system selector (type %x) -> #GP\n", uNewLdt, Desc.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, uNewLdt & X86_SEL_MASK_OFF_RPL);
    }
    if (Desc.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_LDT)
    {
        Log(("lldt %#x - not LDT selector (type %x) -> #GP\n", uNewLdt, Desc.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, uNewLdt & X86_SEL_MASK_OFF_RPL);
    }
    uint64_t u64Base;
    if (!IEM_IS_LONG_MODE(pVCpu))
        u64Base = X86DESC_BASE(&Desc.Legacy);
    else
    {
        if (Desc.Long.Gen.u5Zeros)
        {
            Log(("lldt %#x - u5Zeros=%#x -> #GP\n", uNewLdt, Desc.Long.Gen.u5Zeros));
            return iemRaiseGeneralProtectionFault(pVCpu, uNewLdt & X86_SEL_MASK_OFF_RPL);
        }

        u64Base = X86DESC64_BASE(&Desc.Long);
        if (!IEM_IS_CANONICAL(u64Base))
        {
            Log(("lldt %#x - non-canonical base address %#llx -> #GP\n", uNewLdt, u64Base));
            return iemRaiseGeneralProtectionFault(pVCpu, uNewLdt & X86_SEL_MASK_OFF_RPL);
        }
    }

    /* NP */
    if (!Desc.Legacy.Gen.u1Present)
    {
        Log(("lldt %#x - segment not present -> #NP\n", uNewLdt));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uNewLdt);
    }

    /* Nested-guest SVM intercept. */
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_LDTR_WRITES))
    {
        Log(("lldt: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_LDTR_WRITE, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * It checks out alright, update the registers.
     */
/** @todo check if the actual value is loaded or if the RPL is dropped */
    if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
        CPUMSetGuestLDTR(pVCpu, uNewLdt & X86_SEL_MASK_OFF_RPL);
    else
        pCtx->ldtr.Sel  = uNewLdt & X86_SEL_MASK_OFF_RPL;
    pCtx->ldtr.ValidSel = uNewLdt & X86_SEL_MASK_OFF_RPL;
    pCtx->ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
    pCtx->ldtr.Attr.u   = X86DESC_GET_HID_ATTR(&Desc.Legacy);
    pCtx->ldtr.u32Limit = X86DESC_LIMIT_G(&Desc.Legacy);
    pCtx->ldtr.u64Base  = u64Base;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements lldt.
 *
 * @param   uNewLdt     The new LDT selector value.
 */
IEM_CIMPL_DEF_1(iemCImpl_ltr, uint16_t, uNewTr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        Log(("ltr %04x - real or v8086 mode -> #GP(0)\n", uNewTr));
        return iemRaiseUndefinedOpcode(pVCpu);
    }
    if (pVCpu->iem.s.uCpl != 0)
    {
        Log(("ltr %04x - CPL is %d -> #GP(0)\n", uNewTr, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    if (uNewTr & X86_SEL_LDT)
    {
        Log(("ltr %04x - LDT selector -> #GP\n", uNewTr));
        return iemRaiseGeneralProtectionFaultBySelector(pVCpu, uNewTr);
    }
    if (!(uNewTr & X86_SEL_MASK_OFF_RPL))
    {
        Log(("ltr %04x - NULL selector -> #GP(0)\n", uNewTr));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_TR_WRITES))
    {
        Log(("ltr: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_TR_WRITE, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Read the descriptor.
     */
    IEMSELDESC Desc;
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, &Desc, uNewTr, X86_XCPT_GP); /** @todo Correct exception? */
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Check GPs first. */
    if (Desc.Legacy.Gen.u1DescType)
    {
        Log(("ltr %#x - not system selector (type %x) -> #GP\n", uNewTr, Desc.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, uNewTr & X86_SEL_MASK_OFF_RPL);
    }
    if (   Desc.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_386_TSS_AVAIL /* same as AMD64_SEL_TYPE_SYS_TSS_AVAIL */
        && (   Desc.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_286_TSS_AVAIL
            || IEM_IS_LONG_MODE(pVCpu)) )
    {
        Log(("ltr %#x - not an available TSS selector (type %x) -> #GP\n", uNewTr, Desc.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, uNewTr & X86_SEL_MASK_OFF_RPL);
    }
    uint64_t u64Base;
    if (!IEM_IS_LONG_MODE(pVCpu))
        u64Base = X86DESC_BASE(&Desc.Legacy);
    else
    {
        if (Desc.Long.Gen.u5Zeros)
        {
            Log(("ltr %#x - u5Zeros=%#x -> #GP\n", uNewTr, Desc.Long.Gen.u5Zeros));
            return iemRaiseGeneralProtectionFault(pVCpu, uNewTr & X86_SEL_MASK_OFF_RPL);
        }

        u64Base = X86DESC64_BASE(&Desc.Long);
        if (!IEM_IS_CANONICAL(u64Base))
        {
            Log(("ltr %#x - non-canonical base address %#llx -> #GP\n", uNewTr, u64Base));
            return iemRaiseGeneralProtectionFault(pVCpu, uNewTr & X86_SEL_MASK_OFF_RPL);
        }
    }

    /* NP */
    if (!Desc.Legacy.Gen.u1Present)
    {
        Log(("ltr %#x - segment not present -> #NP\n", uNewTr));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, uNewTr);
    }

    /*
     * Set it busy.
     * Note! Intel says this should lock down the whole descriptor, but we'll
     *       restrict our selves to 32-bit for now due to lack of inline
     *       assembly and such.
     */
    void *pvDesc;
    rcStrict = iemMemMap(pVCpu, &pvDesc, 8, UINT8_MAX, pCtx->gdtr.pGdt + (uNewTr & X86_SEL_MASK_OFF_RPL), IEM_ACCESS_DATA_RW);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    switch ((uintptr_t)pvDesc & 3)
    {
        case 0: ASMAtomicBitSet(pvDesc, 40 + 1); break;
        case 1: ASMAtomicBitSet((uint8_t *)pvDesc + 3, 40 + 1 - 24); break;
        case 2: ASMAtomicBitSet((uint8_t *)pvDesc + 2, 40 + 1 - 16); break;
        case 3: ASMAtomicBitSet((uint8_t *)pvDesc + 1, 40 + 1 -  8); break;
    }
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvDesc, IEM_ACCESS_DATA_RW);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    Desc.Legacy.Gen.u4Type |= X86_SEL_TYPE_SYS_TSS_BUSY_MASK;

    /*
     * It checks out alright, update the registers.
     */
/** @todo check if the actual value is loaded or if the RPL is dropped */
    if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
        CPUMSetGuestTR(pVCpu, uNewTr & X86_SEL_MASK_OFF_RPL);
    else
        pCtx->tr.Sel  = uNewTr & X86_SEL_MASK_OFF_RPL;
    pCtx->tr.ValidSel = uNewTr & X86_SEL_MASK_OFF_RPL;
    pCtx->tr.fFlags   = CPUMSELREG_FLAGS_VALID;
    pCtx->tr.Attr.u   = X86DESC_GET_HID_ATTR(&Desc.Legacy);
    pCtx->tr.u32Limit = X86DESC_LIMIT_G(&Desc.Legacy);
    pCtx->tr.u64Base  = u64Base;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements mov GReg,CRx.
 *
 * @param   iGReg           The general register to store the CRx value in.
 * @param   iCrReg          The CRx register to read (valid).
 */
IEM_CIMPL_DEF_2(iemCImpl_mov_Rd_Cd, uint8_t, iGReg, uint8_t, iCrReg)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!pCtx->eflags.Bits.u1VM);

    if (IEM_IS_SVM_READ_CR_INTERCEPT_SET(pVCpu, iCrReg))
    {
        Log(("iemCImpl_mov_Rd_Cd: Guest intercept CR%u -> #VMEXIT\n", iCrReg));
        IEM_RETURN_SVM_CRX_VMEXIT(pVCpu, SVM_EXIT_READ_CR0 + iCrReg, IEMACCESSCRX_MOV_CRX, iGReg);
    }

    /* read it */
    uint64_t crX;
    switch (iCrReg)
    {
        case 0:
            crX = pCtx->cr0;
            if (IEM_GET_TARGET_CPU(pVCpu) <= IEMTARGETCPU_386)
                crX |= UINT32_C(0x7fffffe0); /* All reserved CR0 flags are set on a 386, just like MSW on 286. */
            break;
        case 2: crX = pCtx->cr2; break;
        case 3: crX = pCtx->cr3; break;
        case 4: crX = pCtx->cr4; break;
        case 8:
        {
#ifdef VBOX_WITH_NESTED_HWVIRT
            PCSVMVMCBCTRL pVmcbCtrl = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
            if (pVmcbCtrl->IntCtrl.n.u1VIntrMasking)
            {
                crX = pVmcbCtrl->IntCtrl.n.u8VTPR;
                break;
            }
#endif
            uint8_t uTpr;
            int rc = APICGetTpr(pVCpu, &uTpr, NULL, NULL);
            if (RT_SUCCESS(rc))
                crX = uTpr >> 4;
            else
                crX = 0;
            break;
        }
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* call checks */
    }

    /* store it */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        *(uint64_t *)iemGRegRef(pVCpu, iGReg) = crX;
    else
        *(uint64_t *)iemGRegRef(pVCpu, iGReg) = (uint32_t)crX;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Used to implemented 'mov CRx,GReg' and 'lmsw r/m16'.
 *
 * @param   iCrReg          The CRx register to write (valid).
 * @param   uNewCrX         The new value.
 * @param   enmAccessCrx    The instruction that caused the CrX load.
 * @param   iGReg           The general register in case of a 'mov CRx,GReg'
 *                          instruction.
 */
IEM_CIMPL_DEF_4(iemCImpl_load_CrX, uint8_t, iCrReg, uint64_t, uNewCrX, IEMACCESSCRX, enmAccessCrX, uint8_t, iGReg)
{
    PCPUMCTX        pCtx  = IEM_GET_CTX(pVCpu);
    VBOXSTRICTRC    rcStrict;
    int             rc;
#ifndef VBOX_WITH_NESTED_HWVIRT
    RT_NOREF2(iGReg, enmAccessCrX);
#endif

    /*
     * Try store it.
     * Unfortunately, CPUM only does a tiny bit of the work.
     */
    switch (iCrReg)
    {
        case 0:
        {
            /*
             * Perform checks.
             */
            uint64_t const uOldCrX = pCtx->cr0;
            uint32_t const fValid  = X86_CR0_PE | X86_CR0_MP | X86_CR0_EM | X86_CR0_TS
                                   | X86_CR0_ET | X86_CR0_NE | X86_CR0_WP | X86_CR0_AM
                                   | X86_CR0_NW | X86_CR0_CD | X86_CR0_PG;

            /* ET is hardcoded on 486 and later. */
            if (IEM_GET_TARGET_CPU(pVCpu) > IEMTARGETCPU_486)
                uNewCrX |= X86_CR0_ET;
            /* The 386 and 486 didn't #GP(0) on attempting to set reserved CR0 bits. ET was settable on 386. */
            else if (IEM_GET_TARGET_CPU(pVCpu) == IEMTARGETCPU_486)
            {
                uNewCrX &= fValid;
                uNewCrX |= X86_CR0_ET;
            }
            else
                uNewCrX &= X86_CR0_PE | X86_CR0_MP | X86_CR0_EM | X86_CR0_TS | X86_CR0_PG | X86_CR0_ET;

            /* Check for reserved bits. */
            if (uNewCrX & ~(uint64_t)fValid)
            {
                Log(("Trying to set reserved CR0 bits: NewCR0=%#llx InvalidBits=%#llx\n", uNewCrX, uNewCrX & ~(uint64_t)fValid));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            /* Check for invalid combinations. */
            if (    (uNewCrX & X86_CR0_PG)
                && !(uNewCrX & X86_CR0_PE) )
            {
                Log(("Trying to set CR0.PG without CR0.PE\n"));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            if (   !(uNewCrX & X86_CR0_CD)
                && (uNewCrX & X86_CR0_NW) )
            {
                Log(("Trying to clear CR0.CD while leaving CR0.NW set\n"));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            if (   !(uNewCrX & X86_CR0_PG)
                && (pCtx->cr4 & X86_CR4_PCIDE))
            {
                Log(("Trying to clear CR0.PG while leaving CR4.PCID set\n"));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            /* Long mode consistency checks. */
            if (    (uNewCrX & X86_CR0_PG)
                && !(uOldCrX & X86_CR0_PG)
                &&  (pCtx->msrEFER & MSR_K6_EFER_LME) )
            {
                if (!(pCtx->cr4 & X86_CR4_PAE))
                {
                    Log(("Trying to enabled long mode paging without CR4.PAE set\n"));
                    return iemRaiseGeneralProtectionFault0(pVCpu);
                }
                if (pCtx->cs.Attr.n.u1Long)
                {
                    Log(("Trying to enabled long mode paging with a long CS descriptor loaded.\n"));
                    return iemRaiseGeneralProtectionFault0(pVCpu);
                }
            }

            /** @todo check reserved PDPTR bits as AMD states. */

            /*
             * SVM nested-guest CR0 write intercepts.
             */
            if (IEM_IS_SVM_WRITE_CR_INTERCEPT_SET(pVCpu, iCrReg))
            {
                Log(("iemCImpl_load_Cr%#x: Guest intercept -> #VMEXIT\n", iCrReg));
                IEM_RETURN_SVM_CRX_VMEXIT(pVCpu, SVM_EXIT_WRITE_CR0, enmAccessCrX, iGReg);
            }
            if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_CR0_SEL_WRITES))
            {
                /* 'lmsw' intercepts regardless of whether the TS/MP bits are actually toggled. */
                if (   enmAccessCrX == IEMACCESSCRX_LMSW
                    || (uNewCrX & ~(X86_CR0_TS | X86_CR0_MP)) != (uOldCrX & ~(X86_CR0_TS | X86_CR0_MP)))
                {
                    Assert(enmAccessCrX != IEMACCESSCRX_CLTS);
                    Log(("iemCImpl_load_Cr%#x: TS/MP bit changed or lmsw instr: Guest intercept -> #VMEXIT\n", iCrReg));
                    IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_CR0_SEL_WRITE, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
                }
            }

            /*
             * Change CR0.
             */
            if (!IEM_VERIFICATION_ENABLED(pVCpu))
                CPUMSetGuestCR0(pVCpu, uNewCrX);
            else
                pCtx->cr0 = uNewCrX;
            Assert(pCtx->cr0 == uNewCrX);

            /*
             * Change EFER.LMA if entering or leaving long mode.
             */
            if (   (uNewCrX & X86_CR0_PG) != (uOldCrX & X86_CR0_PG)
                && (pCtx->msrEFER & MSR_K6_EFER_LME) )
            {
                uint64_t NewEFER = pCtx->msrEFER;
                if (uNewCrX & X86_CR0_PG)
                    NewEFER |= MSR_K6_EFER_LMA;
                else
                    NewEFER &= ~MSR_K6_EFER_LMA;

                if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
                    CPUMSetGuestEFER(pVCpu, NewEFER);
                else
                    pCtx->msrEFER = NewEFER;
                Assert(pCtx->msrEFER == NewEFER);
            }

            /*
             * Inform PGM.
             */
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
            {
                if (    (uNewCrX & (X86_CR0_PG | X86_CR0_WP | X86_CR0_PE))
                    !=  (uOldCrX & (X86_CR0_PG | X86_CR0_WP | X86_CR0_PE)) )
                {
                    rc = PGMFlushTLB(pVCpu, pCtx->cr3, true /* global */);
                    AssertRCReturn(rc, rc);
                    /* ignore informational status codes */
                }
                rcStrict = PGMChangeMode(pVCpu, pCtx->cr0, pCtx->cr4, pCtx->msrEFER);
            }
            else
                rcStrict = VINF_SUCCESS;

#ifdef IN_RC
            /* Return to ring-3 for rescheduling if WP or AM changes. */
            if (   rcStrict == VINF_SUCCESS
                && (   (uNewCrX & (X86_CR0_WP | X86_CR0_AM))
                    != (uOldCrX & (X86_CR0_WP | X86_CR0_AM))) )
                rcStrict = VINF_EM_RESCHEDULE;
#endif
            break;
        }

        /*
         * CR2 can be changed without any restrictions.
         */
        case 2:
        {
            if (IEM_IS_SVM_WRITE_CR_INTERCEPT_SET(pVCpu, /*cr*/ 2))
            {
                Log(("iemCImpl_load_Cr%#x: Guest intercept -> #VMEXIT\n", iCrReg));
                IEM_RETURN_SVM_CRX_VMEXIT(pVCpu, SVM_EXIT_WRITE_CR2, enmAccessCrX, iGReg);
            }
            pCtx->cr2 = uNewCrX;
            rcStrict  = VINF_SUCCESS;
            break;
        }

        /*
         * CR3 is relatively simple, although AMD and Intel have different
         * accounts of how setting reserved bits are handled.  We take intel's
         * word for the lower bits and AMD's for the high bits (63:52).  The
         * lower reserved bits are ignored and left alone; OpenBSD 5.8 relies
         * on this.
         */
        /** @todo Testcase: Setting reserved bits in CR3, especially before
         *        enabling paging. */
        case 3:
        {
            /* clear bit 63 from the source operand and indicate no invalidations are required. */
            if (   (pCtx->cr4 & X86_CR4_PCIDE)
                && (uNewCrX & RT_BIT_64(63)))
            {
                /** @todo r=ramshankar: avoiding a TLB flush altogether here causes Windows 10
                 *        SMP(w/o nested-paging) to hang during bootup on Skylake systems, see
                 *        Intel spec. 4.10.4.1 "Operations that Invalidate TLBs and
                 *        Paging-Structure Caches". */
                uNewCrX &= ~RT_BIT_64(63);
            }

            /* check / mask the value. */
            if (uNewCrX & UINT64_C(0xfff0000000000000))
            {
                Log(("Trying to load CR3 with invalid high bits set: %#llx\n", uNewCrX));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            uint64_t fValid;
            if (   (pCtx->cr4 & X86_CR4_PAE)
                && (pCtx->msrEFER & MSR_K6_EFER_LME))
                fValid = UINT64_C(0x000fffffffffffff);
            else
                fValid = UINT64_C(0xffffffff);
            if (uNewCrX & ~fValid)
            {
                Log(("Automatically clearing reserved MBZ bits in CR3 load: NewCR3=%#llx ClearedBits=%#llx\n",
                     uNewCrX, uNewCrX & ~fValid));
                uNewCrX &= fValid;
            }

            if (IEM_IS_SVM_WRITE_CR_INTERCEPT_SET(pVCpu, /*cr*/ 3))
            {
                Log(("iemCImpl_load_Cr%#x: Guest intercept -> #VMEXIT\n", iCrReg));
                IEM_RETURN_SVM_CRX_VMEXIT(pVCpu, SVM_EXIT_WRITE_CR3, enmAccessCrX, iGReg);
            }

            /** @todo If we're in PAE mode we should check the PDPTRs for
             *        invalid bits. */

            /* Make the change. */
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
            {
                rc = CPUMSetGuestCR3(pVCpu, uNewCrX);
                AssertRCSuccessReturn(rc, rc);
            }
            else
                pCtx->cr3 = uNewCrX;

            /* Inform PGM. */
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
            {
                if (pCtx->cr0 & X86_CR0_PG)
                {
                    rc = PGMFlushTLB(pVCpu, pCtx->cr3, !(pCtx->cr4 & X86_CR4_PGE));
                    AssertRCReturn(rc, rc);
                    /* ignore informational status codes */
                }
            }
            rcStrict = VINF_SUCCESS;
            break;
        }

        /*
         * CR4 is a bit more tedious as there are bits which cannot be cleared
         * under some circumstances and such.
         */
        case 4:
        {
            uint64_t const uOldCrX = pCtx->cr4;

            /** @todo Shouldn't this look at the guest CPUID bits to determine
             *        valid bits? e.g. if guest CPUID doesn't allow X86_CR4_OSXMMEEXCPT, we
             *        should #GP(0). */
            /* reserved bits */
            uint32_t fValid = X86_CR4_VME | X86_CR4_PVI
                            | X86_CR4_TSD | X86_CR4_DE
                            | X86_CR4_PSE | X86_CR4_PAE
                            | X86_CR4_MCE | X86_CR4_PGE
                            | X86_CR4_PCE | X86_CR4_OSFXSR
                            | X86_CR4_OSXMMEEXCPT;
            //if (xxx)
            //    fValid |= X86_CR4_VMXE;
            if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fXSaveRstor)
                fValid |= X86_CR4_OSXSAVE;
            if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fPcid)
                fValid |= X86_CR4_PCIDE;
            if (IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fFsGsBase)
                fValid |= X86_CR4_FSGSBASE;
            if (uNewCrX & ~(uint64_t)fValid)
            {
                Log(("Trying to set reserved CR4 bits: NewCR4=%#llx InvalidBits=%#llx\n", uNewCrX, uNewCrX & ~(uint64_t)fValid));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            bool const fPcide    = ((uNewCrX ^ uOldCrX) & X86_CR4_PCIDE) && (uNewCrX & X86_CR4_PCIDE);
            bool const fLongMode = CPUMIsGuestInLongModeEx(pCtx);

            /* PCIDE check. */
            if (   fPcide
                && (   !fLongMode
                    || (pCtx->cr3 & UINT64_C(0xfff))))
            {
                Log(("Trying to set PCIDE with invalid PCID or outside long mode. Pcid=%#x\n", (pCtx->cr3 & UINT64_C(0xfff))));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            /* PAE check. */
            if (   fLongMode
                && (uOldCrX & X86_CR4_PAE)
                && !(uNewCrX & X86_CR4_PAE))
            {
                Log(("Trying to set clear CR4.PAE while long mode is active\n"));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            if (IEM_IS_SVM_WRITE_CR_INTERCEPT_SET(pVCpu, /*cr*/ 4))
            {
                Log(("iemCImpl_load_Cr%#x: Guest intercept -> #VMEXIT\n", iCrReg));
                IEM_RETURN_SVM_CRX_VMEXIT(pVCpu, SVM_EXIT_WRITE_CR4, enmAccessCrX, iGReg);
            }

            /*
             * Change it.
             */
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
            {
                rc = CPUMSetGuestCR4(pVCpu, uNewCrX);
                AssertRCSuccessReturn(rc, rc);
            }
            else
                pCtx->cr4 = uNewCrX;
            Assert(pCtx->cr4 == uNewCrX);

            /*
             * Notify SELM and PGM.
             */
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
            {
                /* SELM - VME may change things wrt to the TSS shadowing. */
                if ((uNewCrX ^ uOldCrX) & X86_CR4_VME)
                {
                    Log(("iemCImpl_load_CrX: VME %d -> %d => Setting VMCPU_FF_SELM_SYNC_TSS\n",
                         RT_BOOL(uOldCrX & X86_CR4_VME), RT_BOOL(uNewCrX & X86_CR4_VME) ));
#ifdef VBOX_WITH_RAW_MODE
                    if (!HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
                        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
                }

                /* PGM - flushing and mode. */
                if ((uNewCrX ^ uOldCrX) & (X86_CR4_PSE | X86_CR4_PAE | X86_CR4_PGE | X86_CR4_PCIDE /* | X86_CR4_SMEP */))
                {
                    rc = PGMFlushTLB(pVCpu, pCtx->cr3, true /* global */);
                    AssertRCReturn(rc, rc);
                    /* ignore informational status codes */
                }
                rcStrict = PGMChangeMode(pVCpu, pCtx->cr0, pCtx->cr4, pCtx->msrEFER);
            }
            else
                rcStrict = VINF_SUCCESS;
            break;
        }

        /*
         * CR8 maps to the APIC TPR.
         */
        case 8:
        {
            if (uNewCrX & ~(uint64_t)0xf)
            {
                Log(("Trying to set reserved CR8 bits (%#RX64)\n", uNewCrX));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }

            uint8_t const u8Tpr = (uint8_t)uNewCrX << 4;
#ifdef VBOX_WITH_NESTED_HWVIRT
            if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
            {
                if (IEM_IS_SVM_WRITE_CR_INTERCEPT_SET(pVCpu, /*cr*/ 8))
                {
                    Log(("iemCImpl_load_Cr%#x: Guest intercept -> #VMEXIT\n", iCrReg));
                    IEM_RETURN_SVM_CRX_VMEXIT(pVCpu, SVM_EXIT_WRITE_CR8, enmAccessCrX, iGReg);
                }

                PSVMVMCBCTRL pVmcbCtrl = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
                pVmcbCtrl->IntCtrl.n.u8VTPR = u8Tpr;
                if (pVmcbCtrl->IntCtrl.n.u1VIntrMasking)
                {
                    rcStrict = VINF_SUCCESS;
                    break;
                }
            }
#endif
            if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
                APICSetTpr(pVCpu, u8Tpr);
            rcStrict = VINF_SUCCESS;
            break;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* call checks */
    }

    /*
     * Advance the RIP on success.
     */
    if (RT_SUCCESS(rcStrict))
    {
        if (rcStrict != VINF_SUCCESS)
            rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    }

    return rcStrict;
}


/**
 * Implements mov CRx,GReg.
 *
 * @param   iCrReg          The CRx register to write (valid).
 * @param   iGReg           The general register to load the DRx value from.
 */
IEM_CIMPL_DEF_2(iemCImpl_mov_Cd_Rd, uint8_t, iCrReg, uint8_t, iGReg)
{
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!IEM_GET_CTX(pVCpu)->eflags.Bits.u1VM);

    /*
     * Read the new value from the source register and call common worker.
     */
    uint64_t uNewCrX;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        uNewCrX = iemGRegFetchU64(pVCpu, iGReg);
    else
        uNewCrX = iemGRegFetchU32(pVCpu, iGReg);
    return IEM_CIMPL_CALL_4(iemCImpl_load_CrX, iCrReg, uNewCrX, IEMACCESSCRX_MOV_CRX, iGReg);
}


/**
 * Implements 'LMSW r/m16'
 *
 * @param   u16NewMsw       The new value.
 */
IEM_CIMPL_DEF_1(iemCImpl_lmsw, uint16_t, u16NewMsw)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!pCtx->eflags.Bits.u1VM);

    /*
     * Compose the new CR0 value and call common worker.
     */
    uint64_t uNewCr0 = pCtx->cr0     & ~(X86_CR0_MP | X86_CR0_EM | X86_CR0_TS);
    uNewCr0 |= u16NewMsw & (X86_CR0_PE | X86_CR0_MP | X86_CR0_EM | X86_CR0_TS);
    return IEM_CIMPL_CALL_4(iemCImpl_load_CrX, /*cr*/ 0, uNewCr0, IEMACCESSCRX_LMSW, UINT8_MAX /* iGReg */);
}


/**
 * Implements 'CLTS'.
 */
IEM_CIMPL_DEF_0(iemCImpl_clts)
{
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    uint64_t uNewCr0 = pCtx->cr0;
    uNewCr0 &= ~X86_CR0_TS;
    return IEM_CIMPL_CALL_4(iemCImpl_load_CrX, /*cr*/ 0, uNewCr0, IEMACCESSCRX_CLTS, UINT8_MAX /* iGReg */);
}


/**
 * Implements mov GReg,DRx.
 *
 * @param   iGReg           The general register to store the DRx value in.
 * @param   iDrReg          The DRx register to read (0-7).
 */
IEM_CIMPL_DEF_2(iemCImpl_mov_Rd_Dd, uint8_t, iGReg, uint8_t, iDrReg)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */

    /* Raise GPs. */
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!pCtx->eflags.Bits.u1VM);

    if (   (iDrReg == 4 || iDrReg == 5)
        && (pCtx->cr4 & X86_CR4_DE) )
    {
        Log(("mov r%u,dr%u: CR4.DE=1 -> #GP(0)\n", iGReg, iDrReg));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Raise #DB if general access detect is enabled. */
    if (pCtx->dr[7] & X86_DR7_GD)
    {
        Log(("mov r%u,dr%u: DR7.GD=1 -> #DB\n", iGReg, iDrReg));
        return iemRaiseDebugException(pVCpu);
    }

    /*
     * Read the debug register and store it in the specified general register.
     */
    uint64_t drX;
    switch (iDrReg)
    {
        case 0: drX = pCtx->dr[0]; break;
        case 1: drX = pCtx->dr[1]; break;
        case 2: drX = pCtx->dr[2]; break;
        case 3: drX = pCtx->dr[3]; break;
        case 6:
        case 4:
            drX = pCtx->dr[6];
            drX |= X86_DR6_RA1_MASK;
            drX &= ~X86_DR6_RAZ_MASK;
            break;
        case 7:
        case 5:
            drX = pCtx->dr[7];
            drX |=X86_DR7_RA1_MASK;
            drX &= ~X86_DR7_RAZ_MASK;
            break;
        IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* call checks */
    }

    /** @todo SVM nested-guest intercept for DR8-DR15? */
    /*
     * Check for any SVM nested-guest intercepts for the DRx read.
     */
    if (IEM_IS_SVM_READ_DR_INTERCEPT_SET(pVCpu, iDrReg))
    {
        Log(("mov r%u,dr%u: Guest intercept -> #VMEXIT\n", iGReg, iDrReg));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_READ_DR0 + (iDrReg & 0xf),
                              IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSvmDecodeAssist ? (iGReg & 7) : 0, 0 /* uExitInfo2 */);
    }

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        *(uint64_t *)iemGRegRef(pVCpu, iGReg) = drX;
    else
        *(uint64_t *)iemGRegRef(pVCpu, iGReg) = (uint32_t)drX;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements mov DRx,GReg.
 *
 * @param   iDrReg          The DRx register to write (valid).
 * @param   iGReg           The general register to load the DRx value from.
 */
IEM_CIMPL_DEF_2(iemCImpl_mov_Dd_Rd, uint8_t, iDrReg, uint8_t, iGReg)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!pCtx->eflags.Bits.u1VM);

    if (iDrReg == 4 || iDrReg == 5)
    {
        if (pCtx->cr4 & X86_CR4_DE)
        {
            Log(("mov dr%u,r%u: CR4.DE=1 -> #GP(0)\n", iDrReg, iGReg));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }
        iDrReg += 2;
    }

    /* Raise #DB if general access detect is enabled. */
    /** @todo is \#DB/DR7.GD raised before any reserved high bits in DR7/DR6
     *        \#GP? */
    if (pCtx->dr[7] & X86_DR7_GD)
    {
        Log(("mov dr%u,r%u: DR7.GD=1 -> #DB\n", iDrReg, iGReg));
        return iemRaiseDebugException(pVCpu);
    }

    /*
     * Read the new value from the source register.
     */
    uint64_t uNewDrX;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        uNewDrX = iemGRegFetchU64(pVCpu, iGReg);
    else
        uNewDrX = iemGRegFetchU32(pVCpu, iGReg);

    /*
     * Adjust it.
     */
    switch (iDrReg)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            /* nothing to adjust */
            break;

        case 6:
            if (uNewDrX & X86_DR6_MBZ_MASK)
            {
                Log(("mov dr%u,%#llx: DR6 high bits are not zero -> #GP(0)\n", iDrReg, uNewDrX));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }
            uNewDrX |= X86_DR6_RA1_MASK;
            uNewDrX &= ~X86_DR6_RAZ_MASK;
            break;

        case 7:
            if (uNewDrX & X86_DR7_MBZ_MASK)
            {
                Log(("mov dr%u,%#llx: DR7 high bits are not zero -> #GP(0)\n", iDrReg, uNewDrX));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }
            uNewDrX |= X86_DR7_RA1_MASK;
            uNewDrX &= ~X86_DR7_RAZ_MASK;
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    /** @todo SVM nested-guest intercept for DR8-DR15? */
    /*
     * Check for any SVM nested-guest intercepts for the DRx write.
     */
    if (IEM_IS_SVM_WRITE_DR_INTERCEPT_SET(pVCpu, iDrReg))
    {
        Log2(("mov dr%u,r%u: Guest intercept -> #VMEXIT\n", iDrReg, iGReg));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_WRITE_DR0 + (iDrReg & 0xf),
                              IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSvmDecodeAssist ? (iGReg & 7) : 0, 0 /* uExitInfo2 */);
    }

    /*
     * Do the actual setting.
     */
    if (!IEM_VERIFICATION_ENABLED(pVCpu))
    {
        int rc = CPUMSetGuestDRx(pVCpu, iDrReg, uNewDrX);
        AssertRCSuccessReturn(rc, RT_SUCCESS_NP(rc) ? VERR_IEM_IPE_1 : rc);
    }
    else
        pCtx->dr[iDrReg] = uNewDrX;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'INVLPG m'.
 *
 * @param   GCPtrPage       The effective address of the page to invalidate.
 * @remarks Updates the RIP.
 */
IEM_CIMPL_DEF_1(iemCImpl_invlpg, RTGCPTR, GCPtrPage)
{
    /* ring-0 only. */
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);
    Assert(!IEM_GET_CTX(pVCpu)->eflags.Bits.u1VM);

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_INVLPG))
    {
        Log(("invlpg: Guest intercept (%RGp) -> #VMEXIT\n", GCPtrPage));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_INVLPG,
                              IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSvmDecodeAssist ? GCPtrPage : 0, 0 /* uExitInfo2 */);
    }

    int rc = PGMInvalidatePage(pVCpu, GCPtrPage);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);

    if (rc == VINF_SUCCESS)
        return VINF_SUCCESS;
    if (rc == VINF_PGM_SYNC_CR3)
        return iemSetPassUpStatus(pVCpu, rc);

    AssertMsg(rc == VINF_EM_RAW_EMULATE_INSTR || RT_FAILURE_NP(rc), ("%Rrc\n", rc));
    Log(("PGMInvalidatePage(%RGv) -> %Rrc\n", GCPtrPage, rc));
    return rc;
}


/**
 * Implements INVPCID.
 *
 * @param   uInvpcidType         The invalidation type.
 * @param   GCPtrInvpcidDesc     The effective address of invpcid descriptor.
 * @remarks Updates the RIP.
 */
IEM_CIMPL_DEF_2(iemCImpl_invpcid, uint64_t, uInvpcidType, RTGCPTR, GCPtrInvpcidDesc)
{
    /*
     * Check preconditions.
     */
    if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fInvpcid)
        return iemRaiseUndefinedOpcode(pVCpu);
    if (pVCpu->iem.s.uCpl != 0)
    {
        Log(("invpcid: CPL != 0 -> #GP(0)\n"));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    if (IEM_IS_V86_MODE(pVCpu))
    {
        Log(("invpcid: v8086 mode -> #GP(0)\n"));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    if (uInvpcidType > X86_INVPCID_TYPE_MAX_VALID)
    {
        Log(("invpcid: invalid/unrecognized invpcid type %#x -> #GP(0)\n", uInvpcidType));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Fetch the invpcid descriptor from guest memory.
     */
    RTUINT128U uDesc;
    VBOXSTRICTRC rcStrict = iemMemFetchDataU128(pVCpu, &uDesc, pVCpu->iem.s.iEffSeg, GCPtrInvpcidDesc);
    if (rcStrict == VINF_SUCCESS)
    {
        /*
         * Validate the descriptor.
         */
        if (uDesc.s.Lo > 0xfff)
        {
            Log(("invpcid: reserved bits set in invpcid descriptor %#RX64 -> #GP(0)\n", uDesc.s.Lo));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        RTGCUINTPTR64 const GCPtrInvAddr = uDesc.s.Hi;
        uint8_t       const uPcid        = uDesc.s.Lo & UINT64_C(0xfff);
        uint32_t      const uCr4         = IEM_GET_CTX(pVCpu)->cr4;
        uint64_t      const uCr3         = IEM_GET_CTX(pVCpu)->cr3;
        switch (uInvpcidType)
        {
            case X86_INVPCID_TYPE_INDV_ADDR:
            {
                if (!IEM_IS_CANONICAL(GCPtrInvAddr))
                {
                    Log(("invpcid: invalidation address %#RGP is not canonical -> #GP(0)\n", GCPtrInvAddr));
                    return iemRaiseGeneralProtectionFault0(pVCpu);
                }
                if (  !(uCr4 & X86_CR4_PCIDE)
                    && uPcid != 0)
                {
                    Log(("invpcid: invalid pcid %#x\n", uPcid));
                    return iemRaiseGeneralProtectionFault0(pVCpu);
                }

                /* Invalidate mappings for the linear address tagged with PCID except global translations. */
                PGMFlushTLB(pVCpu, uCr3, false /* fGlobal */);
                break;
            }

            case X86_INVPCID_TYPE_SINGLE_CONTEXT:
            {
                if (  !(uCr4 & X86_CR4_PCIDE)
                    && uPcid != 0)
                {
                    Log(("invpcid: invalid pcid %#x\n", uPcid));
                    return iemRaiseGeneralProtectionFault0(pVCpu);
                }
                /* Invalidate all mappings associated with PCID except global translations. */
                PGMFlushTLB(pVCpu, uCr3, false /* fGlobal */);
                break;
            }

            case X86_INVPCID_TYPE_ALL_CONTEXT_INCL_GLOBAL:
            {
                PGMFlushTLB(pVCpu, uCr3, true /* fGlobal */);
                break;
            }

            case X86_INVPCID_TYPE_ALL_CONTEXT_EXCL_GLOBAL:
            {
                PGMFlushTLB(pVCpu, uCr3, false /* fGlobal */);
                break;
            }
            IEM_NOT_REACHED_DEFAULT_CASE_RET();
        }
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    }
    return rcStrict;
}


/**
 * Implements RDTSC.
 */
IEM_CIMPL_DEF_0(iemCImpl_rdtsc)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */
    if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fTsc)
        return iemRaiseUndefinedOpcode(pVCpu);

    if (   (pCtx->cr4 & X86_CR4_TSD)
        && pVCpu->iem.s.uCpl != 0)
    {
        Log(("rdtsc: CR4.TSD and CPL=%u -> #GP(0)\n", pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_RDTSC))
    {
        Log(("rdtsc: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_RDTSC, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Do the job.
     */
    uint64_t uTicks = TMCpuTickGet(pVCpu);
    pCtx->rax = RT_LO_U32(uTicks);
    pCtx->rdx = RT_HI_U32(uTicks);
#ifdef IEM_VERIFICATION_MODE_FULL
    pVCpu->iem.s.fIgnoreRaxRdx = true;
#endif

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements RDTSC.
 */
IEM_CIMPL_DEF_0(iemCImpl_rdtscp)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */
    if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fRdTscP)
        return iemRaiseUndefinedOpcode(pVCpu);

    if (   (pCtx->cr4 & X86_CR4_TSD)
        && pVCpu->iem.s.uCpl != 0)
    {
        Log(("rdtscp: CR4.TSD and CPL=%u -> #GP(0)\n", pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_RDTSCP))
    {
        Log(("rdtscp: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_RDTSCP, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Do the job.
     * Query the MSR first in case of trips to ring-3.
     */
    VBOXSTRICTRC rcStrict = CPUMQueryGuestMsr(pVCpu, MSR_K8_TSC_AUX, &pCtx->rcx);
    if (rcStrict == VINF_SUCCESS)
    {
        /* Low dword of the TSC_AUX msr only. */
        pCtx->rcx &= UINT32_C(0xffffffff);

        uint64_t uTicks = TMCpuTickGet(pVCpu);
        pCtx->rax = RT_LO_U32(uTicks);
        pCtx->rdx = RT_HI_U32(uTicks);
#ifdef IEM_VERIFICATION_MODE_FULL
        pVCpu->iem.s.fIgnoreRaxRdx = true;
#endif
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    }
    return rcStrict;
}


/**
 * Implements RDPMC.
 */
IEM_CIMPL_DEF_0(iemCImpl_rdpmc)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    if (   pVCpu->iem.s.uCpl != 0
        && !(pCtx->cr4 & X86_CR4_PCE))
        return iemRaiseGeneralProtectionFault0(pVCpu);

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_RDPMC))
    {
        Log(("rdpmc: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_RDPMC, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /** @todo Implement RDPMC for the regular guest execution case (the above only
     *        handles nested-guest intercepts). */
    RT_NOREF(cbInstr);
    return VERR_IEM_INSTR_NOT_IMPLEMENTED;
}


/**
 * Implements RDMSR.
 */
IEM_CIMPL_DEF_0(iemCImpl_rdmsr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */
    if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fMsr)
        return iemRaiseUndefinedOpcode(pVCpu);
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    /*
     * Do the job.
     */
    RTUINT64U uValue;
    VBOXSTRICTRC rcStrict;
#ifdef VBOX_WITH_NESTED_HWVIRT
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_MSR_PROT))
    {
        rcStrict = iemSvmHandleMsrIntercept(pVCpu, pCtx, pCtx->ecx, false /* fWrite */);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("IEM: SVM intercepted rdmsr(%#x) failed. rc=%Rrc\n", pCtx->ecx, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    rcStrict = CPUMQueryGuestMsr(pVCpu, pCtx->ecx, &uValue.u);
    if (rcStrict == VINF_SUCCESS)
    {
        pCtx->rax = uValue.s.Lo;
        pCtx->rdx = uValue.s.Hi;

        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

#ifndef IN_RING3
    /* Deferred to ring-3. */
    if (rcStrict == VINF_CPUM_R3_MSR_READ)
    {
        Log(("IEM: rdmsr(%#x) -> ring-3\n", pCtx->ecx));
        return rcStrict;
    }
#else /* IN_RING3 */
    /* Often a unimplemented MSR or MSR bit, so worth logging. */
    static uint32_t s_cTimes = 0;
    if (s_cTimes++ < 10)
        LogRel(("IEM: rdmsr(%#x) -> #GP(0)\n", pCtx->ecx));
    else
#endif
        Log(("IEM: rdmsr(%#x) -> #GP(0)\n", pCtx->ecx));
    AssertMsgReturn(rcStrict == VERR_CPUM_RAISE_GP_0, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)), VERR_IPE_UNEXPECTED_STATUS);
    return iemRaiseGeneralProtectionFault0(pVCpu);
}


/**
 * Implements WRMSR.
 */
IEM_CIMPL_DEF_0(iemCImpl_wrmsr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Check preconditions.
     */
    if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fMsr)
        return iemRaiseUndefinedOpcode(pVCpu);
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    /*
     * Do the job.
     */
    RTUINT64U uValue;
    uValue.s.Lo = pCtx->eax;
    uValue.s.Hi = pCtx->edx;

    VBOXSTRICTRC rcStrict;
#ifdef VBOX_WITH_NESTED_HWVIRT
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_MSR_PROT))
    {
        rcStrict = iemSvmHandleMsrIntercept(pVCpu, pCtx, pCtx->ecx, true /* fWrite */);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("IEM: SVM intercepted rdmsr(%#x) failed. rc=%Rrc\n", pCtx->ecx, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    if (!IEM_VERIFICATION_ENABLED(pVCpu))
        rcStrict = CPUMSetGuestMsr(pVCpu, pCtx->ecx, uValue.u);
    else
    {
#ifdef IN_RING3
        CPUMCTX CtxTmp = *pCtx;
        rcStrict = CPUMSetGuestMsr(pVCpu, pCtx->ecx, uValue.u);
        PCPUMCTX pCtx2 = CPUMQueryGuestCtxPtr(pVCpu);
        *pCtx = *pCtx2;
        *pCtx2 = CtxTmp;
#else
        AssertReleaseFailedReturn(VERR_IEM_IPE_2);
#endif
    }
    if (rcStrict == VINF_SUCCESS)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

#ifndef IN_RING3
    /* Deferred to ring-3. */
    if (rcStrict == VINF_CPUM_R3_MSR_WRITE)
    {
        Log(("IEM: wrmsr(%#x) -> ring-3\n", pCtx->ecx));
        return rcStrict;
    }
#else /* IN_RING3 */
    /* Often a unimplemented MSR or MSR bit, so worth logging. */
    static uint32_t s_cTimes = 0;
    if (s_cTimes++ < 10)
        LogRel(("IEM: wrmsr(%#x,%#x`%08x) -> #GP(0)\n", pCtx->ecx, uValue.s.Hi, uValue.s.Lo));
    else
#endif
        Log(("IEM: wrmsr(%#x,%#x`%08x) -> #GP(0)\n", pCtx->ecx, uValue.s.Hi, uValue.s.Lo));
    AssertMsgReturn(rcStrict == VERR_CPUM_RAISE_GP_0, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)), VERR_IPE_UNEXPECTED_STATUS);
    return iemRaiseGeneralProtectionFault0(pVCpu);
}


/**
 * Implements 'IN eAX, port'.
 *
 * @param   u16Port     The source port.
 * @param   cbReg       The register size.
 */
IEM_CIMPL_DEF_2(iemCImpl_in, uint16_t, u16Port, uint8_t, cbReg)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * CPL check
     */
    VBOXSTRICTRC rcStrict = iemHlpCheckPortIOPermission(pVCpu, pCtx, u16Port, cbReg);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Check SVM nested-guest IO intercept.
     */
#ifdef VBOX_WITH_NESTED_HWVIRT
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IOIO_PROT))
    {
        rcStrict = iemSvmHandleIOIntercept(pVCpu, u16Port, SVMIOIOTYPE_IN, cbReg, 0 /* N/A - cAddrSizeBits */,
                                           0 /* N/A - iEffSeg */, false /* fRep */, false /* fStrIo */, cbInstr);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("iemCImpl_in: iemSvmHandleIOIntercept failed (u16Port=%#x, cbReg=%u) rc=%Rrc\n", u16Port, cbReg,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    /*
     * Perform the I/O.
     */
    uint32_t u32Value;
    if (!IEM_VERIFICATION_ENABLED(pVCpu))
        rcStrict = IOMIOPortRead(pVCpu->CTX_SUFF(pVM), pVCpu, u16Port, &u32Value, cbReg);
    else
        rcStrict = iemVerifyFakeIOPortRead(pVCpu, u16Port, &u32Value, cbReg);
    if (IOM_SUCCESS(rcStrict))
    {
        switch (cbReg)
        {
            case 1: pCtx->al  = (uint8_t)u32Value;  break;
            case 2: pCtx->ax  = (uint16_t)u32Value; break;
            case 4: pCtx->rax = u32Value;           break;
            default: AssertFailedReturn(VERR_IEM_IPE_3);
        }
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        pVCpu->iem.s.cPotentialExits++;
        if (rcStrict != VINF_SUCCESS)
            rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        Assert(rcStrict == VINF_SUCCESS); /* assumed below */

        /*
         * Check for I/O breakpoints.
         */
        uint32_t const uDr7 = pCtx->dr[7];
        if (RT_UNLIKELY(   (   (uDr7 & X86_DR7_ENABLED_MASK)
                            && X86_DR7_ANY_RW_IO(uDr7)
                            && (pCtx->cr4 & X86_CR4_DE))
                        || DBGFBpIsHwIoArmed(pVCpu->CTX_SUFF(pVM))))
        {
            rcStrict = DBGFBpCheckIo(pVCpu->CTX_SUFF(pVM), pVCpu, pCtx, u16Port, cbReg);
            if (rcStrict == VINF_EM_RAW_GUEST_TRAP)
                rcStrict = iemRaiseDebugException(pVCpu);
        }
    }

    return rcStrict;
}


/**
 * Implements 'IN eAX, DX'.
 *
 * @param   cbReg       The register size.
 */
IEM_CIMPL_DEF_1(iemCImpl_in_eAX_DX, uint8_t, cbReg)
{
    return IEM_CIMPL_CALL_2(iemCImpl_in, IEM_GET_CTX(pVCpu)->dx, cbReg);
}


/**
 * Implements 'OUT port, eAX'.
 *
 * @param   u16Port     The destination port.
 * @param   cbReg       The register size.
 */
IEM_CIMPL_DEF_2(iemCImpl_out, uint16_t, u16Port, uint8_t, cbReg)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * CPL check
     */
    VBOXSTRICTRC rcStrict = iemHlpCheckPortIOPermission(pVCpu, pCtx, u16Port, cbReg);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Check SVM nested-guest IO intercept.
     */
#ifdef VBOX_WITH_NESTED_HWVIRT
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IOIO_PROT))
    {
        rcStrict = iemSvmHandleIOIntercept(pVCpu, u16Port, SVMIOIOTYPE_OUT, cbReg, 0 /* N/A - cAddrSizeBits */,
                                           0 /* N/A - iEffSeg */, false /* fRep */, false /* fStrIo */, cbInstr);
        if (rcStrict == VINF_SVM_VMEXIT)
            return VINF_SUCCESS;
        if (rcStrict != VINF_HM_INTERCEPT_NOT_ACTIVE)
        {
            Log(("iemCImpl_out: iemSvmHandleIOIntercept failed (u16Port=%#x, cbReg=%u) rc=%Rrc\n", u16Port, cbReg,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
#endif

    /*
     * Perform the I/O.
     */
    uint32_t u32Value;
    switch (cbReg)
    {
        case 1: u32Value = pCtx->al;  break;
        case 2: u32Value = pCtx->ax;  break;
        case 4: u32Value = pCtx->eax; break;
        default: AssertFailedReturn(VERR_IEM_IPE_4);
    }
    if (!IEM_VERIFICATION_ENABLED(pVCpu))
        rcStrict = IOMIOPortWrite(pVCpu->CTX_SUFF(pVM), pVCpu, u16Port, u32Value, cbReg);
    else
        rcStrict = iemVerifyFakeIOPortWrite(pVCpu, u16Port, u32Value, cbReg);
    if (IOM_SUCCESS(rcStrict))
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        pVCpu->iem.s.cPotentialExits++;
        if (rcStrict != VINF_SUCCESS)
            rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        Assert(rcStrict == VINF_SUCCESS); /* assumed below */

        /*
         * Check for I/O breakpoints.
         */
        uint32_t const uDr7 = pCtx->dr[7];
        if (RT_UNLIKELY(   (   (uDr7 & X86_DR7_ENABLED_MASK)
                            && X86_DR7_ANY_RW_IO(uDr7)
                            && (pCtx->cr4 & X86_CR4_DE))
                        || DBGFBpIsHwIoArmed(pVCpu->CTX_SUFF(pVM))))
        {
            rcStrict = DBGFBpCheckIo(pVCpu->CTX_SUFF(pVM), pVCpu, pCtx, u16Port, cbReg);
            if (rcStrict == VINF_EM_RAW_GUEST_TRAP)
                rcStrict = iemRaiseDebugException(pVCpu);
        }
    }
    return rcStrict;
}


/**
 * Implements 'OUT DX, eAX'.
 *
 * @param   cbReg       The register size.
 */
IEM_CIMPL_DEF_1(iemCImpl_out_DX_eAX, uint8_t, cbReg)
{
    return IEM_CIMPL_CALL_2(iemCImpl_out, IEM_GET_CTX(pVCpu)->dx, cbReg);
}


/**
 * Implements 'CLI'.
 */
IEM_CIMPL_DEF_0(iemCImpl_cli)
{
    PCPUMCTX        pCtx    = IEM_GET_CTX(pVCpu);
    uint32_t        fEfl    = IEMMISC_GET_EFL(pVCpu, pCtx);
    uint32_t const  fEflOld = fEfl;
    if (pCtx->cr0 & X86_CR0_PE)
    {
        uint8_t const uIopl = X86_EFL_GET_IOPL(fEfl);
        if (!(fEfl & X86_EFL_VM))
        {
            if (pVCpu->iem.s.uCpl <= uIopl)
                fEfl &= ~X86_EFL_IF;
            else if (   pVCpu->iem.s.uCpl == 3
                     && (pCtx->cr4 & X86_CR4_PVI) )
                fEfl &= ~X86_EFL_VIF;
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
        }
        /* V8086 */
        else if (uIopl == 3)
            fEfl &= ~X86_EFL_IF;
        else if (   uIopl < 3
                 && (pCtx->cr4 & X86_CR4_VME) )
            fEfl &= ~X86_EFL_VIF;
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    /* real mode */
    else
        fEfl &= ~X86_EFL_IF;

    /* Commit. */
    IEMMISC_SET_EFL(pVCpu, pCtx, fEfl);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    Log2(("CLI: %#x -> %#x\n", fEflOld, fEfl)); NOREF(fEflOld);
    return VINF_SUCCESS;
}


/**
 * Implements 'STI'.
 */
IEM_CIMPL_DEF_0(iemCImpl_sti)
{
    PCPUMCTX        pCtx    = IEM_GET_CTX(pVCpu);
    uint32_t        fEfl    = IEMMISC_GET_EFL(pVCpu, pCtx);
    uint32_t const  fEflOld = fEfl;

    if (pCtx->cr0 & X86_CR0_PE)
    {
        uint8_t const uIopl = X86_EFL_GET_IOPL(fEfl);
        if (!(fEfl & X86_EFL_VM))
        {
            if (pVCpu->iem.s.uCpl <= uIopl)
                fEfl |= X86_EFL_IF;
            else if (   pVCpu->iem.s.uCpl == 3
                     && (pCtx->cr4 & X86_CR4_PVI)
                     && !(fEfl & X86_EFL_VIP) )
                fEfl |= X86_EFL_VIF;
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
        }
        /* V8086 */
        else if (uIopl == 3)
            fEfl |= X86_EFL_IF;
        else if (   uIopl < 3
                 && (pCtx->cr4 & X86_CR4_VME)
                 && !(fEfl & X86_EFL_VIP) )
            fEfl |= X86_EFL_VIF;
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    /* real mode */
    else
        fEfl |= X86_EFL_IF;

    /* Commit. */
    IEMMISC_SET_EFL(pVCpu, pCtx, fEfl);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    if ((!(fEflOld & X86_EFL_IF) && (fEfl & X86_EFL_IF)) || IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
        EMSetInhibitInterruptsPC(pVCpu, pCtx->rip);
    Log2(("STI: %#x -> %#x\n", fEflOld, fEfl));
    return VINF_SUCCESS;
}


/**
 * Implements 'HLT'.
 */
IEM_CIMPL_DEF_0(iemCImpl_hlt)
{
    if (pVCpu->iem.s.uCpl != 0)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_HLT))
    {
        Log2(("hlt: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_HLT, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_EM_HALT;
}


/**
 * Implements 'MONITOR'.
 */
IEM_CIMPL_DEF_1(iemCImpl_monitor, uint8_t, iEffSeg)
{
    /*
     * Permission checks.
     */
    if (pVCpu->iem.s.uCpl != 0)
    {
        Log2(("monitor: CPL != 0\n"));
        return iemRaiseUndefinedOpcode(pVCpu); /** @todo MSR[0xC0010015].MonMwaitUserEn if we care. */
    }
    if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fMonitorMWait)
    {
        Log2(("monitor: Not in CPUID\n"));
        return iemRaiseUndefinedOpcode(pVCpu);
    }

    /*
     * Gather the operands and validate them.
     */
    PCPUMCTX pCtx       = IEM_GET_CTX(pVCpu);
    RTGCPTR  GCPtrMem   = pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT ? pCtx->rax : pCtx->eax;
    uint32_t uEcx       = pCtx->ecx;
    uint32_t uEdx       = pCtx->edx;
/** @todo Test whether EAX or ECX is processed first, i.e. do we get \#PF or
 *        \#GP first. */
    if (uEcx != 0)
    {
        Log2(("monitor rax=%RX64, ecx=%RX32, edx=%RX32; ECX != 0 -> #GP(0)\n", GCPtrMem, uEcx, uEdx)); NOREF(uEdx);
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    VBOXSTRICTRC rcStrict = iemMemApplySegment(pVCpu, IEM_ACCESS_TYPE_READ | IEM_ACCESS_WHAT_DATA, iEffSeg, 1, &GCPtrMem);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    RTGCPHYS GCPhysMem;
    rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrMem, IEM_ACCESS_TYPE_READ | IEM_ACCESS_WHAT_DATA, &GCPhysMem);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_MONITOR))
    {
        Log2(("monitor: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_MONITOR, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Call EM to prepare the monitor/wait.
     */
    rcStrict = EMMonitorWaitPrepare(pVCpu, pCtx->rax, pCtx->rcx, pCtx->rdx, GCPhysMem);
    Assert(rcStrict == VINF_SUCCESS);

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return rcStrict;
}


/**
 * Implements 'MWAIT'.
 */
IEM_CIMPL_DEF_0(iemCImpl_mwait)
{
    /*
     * Permission checks.
     */
    if (pVCpu->iem.s.uCpl != 0)
    {
        Log2(("mwait: CPL != 0\n"));
        /** @todo MSR[0xC0010015].MonMwaitUserEn if we care. (Remember to check
         *        EFLAGS.VM then.) */
        return iemRaiseUndefinedOpcode(pVCpu);
    }
    if (!IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fMonitorMWait)
    {
        Log2(("mwait: Not in CPUID\n"));
        return iemRaiseUndefinedOpcode(pVCpu);
    }

    /*
     * Gather the operands and validate them.
     */
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    uint32_t uEax = pCtx->eax;
    uint32_t uEcx = pCtx->ecx;
    if (uEcx != 0)
    {
        /* Only supported extension is break on IRQ when IF=0. */
        if (uEcx > 1)
        {
            Log2(("mwait eax=%RX32, ecx=%RX32; ECX > 1 -> #GP(0)\n", uEax, uEcx));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }
        uint32_t fMWaitFeatures = 0;
        uint32_t uIgnore = 0;
        CPUMGetGuestCpuId(pVCpu, 5, 0, &uIgnore, &uIgnore, &fMWaitFeatures, &uIgnore);
        if (    (fMWaitFeatures & (X86_CPUID_MWAIT_ECX_EXT | X86_CPUID_MWAIT_ECX_BREAKIRQIF0))
            !=                    (X86_CPUID_MWAIT_ECX_EXT | X86_CPUID_MWAIT_ECX_BREAKIRQIF0))
        {
            Log2(("mwait eax=%RX32, ecx=%RX32; break-on-IRQ-IF=0 extension not enabled -> #GP(0)\n", uEax, uEcx));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }
    }

    /*
     * Check SVM nested-guest mwait intercepts.
     */
    if (   IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_MWAIT_ARMED)
        && EMMonitorIsArmed(pVCpu))
    {
        Log2(("mwait: Guest intercept (monitor hardware armed) -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_MWAIT_ARMED, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_MWAIT))
    {
        Log2(("mwait: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_MWAIT, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * Call EM to prepare the monitor/wait.
     */
    VBOXSTRICTRC rcStrict = EMMonitorWaitPerform(pVCpu, uEax, uEcx);

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return rcStrict;
}


/**
 * Implements 'SWAPGS'.
 */
IEM_CIMPL_DEF_0(iemCImpl_swapgs)
{
    Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT); /* Caller checks this. */

    /*
     * Permission checks.
     */
    if (pVCpu->iem.s.uCpl != 0)
    {
        Log2(("swapgs: CPL != 0\n"));
        return iemRaiseUndefinedOpcode(pVCpu);
    }

    /*
     * Do the job.
     */
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    uint64_t uOtherGsBase = pCtx->msrKERNELGSBASE;
    pCtx->msrKERNELGSBASE = pCtx->gs.u64Base;
    pCtx->gs.u64Base = uOtherGsBase;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'CPUID'.
 */
IEM_CIMPL_DEF_0(iemCImpl_cpuid)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_CPUID))
    {
        Log2(("cpuid: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_CPUID, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    CPUMGetGuestCpuId(pVCpu, pCtx->eax, pCtx->ecx, &pCtx->eax, &pCtx->ebx, &pCtx->ecx, &pCtx->edx);
    pCtx->rax &= UINT32_C(0xffffffff);
    pCtx->rbx &= UINT32_C(0xffffffff);
    pCtx->rcx &= UINT32_C(0xffffffff);
    pCtx->rdx &= UINT32_C(0xffffffff);

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'AAD'.
 *
 * @param   bImm            The immediate operand.
 */
IEM_CIMPL_DEF_1(iemCImpl_aad, uint8_t, bImm)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    uint16_t const ax = pCtx->ax;
    uint8_t const  al = (uint8_t)ax + (uint8_t)(ax >> 8) * bImm;
    pCtx->ax = al;
    iemHlpUpdateArithEFlagsU8(pVCpu, al,
                              X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF,
                              X86_EFL_OF | X86_EFL_AF | X86_EFL_CF);

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'AAM'.
 *
 * @param   bImm            The immediate operand. Cannot be 0.
 */
IEM_CIMPL_DEF_1(iemCImpl_aam, uint8_t, bImm)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    Assert(bImm != 0); /* #DE on 0 is handled in the decoder. */

    uint16_t const ax = pCtx->ax;
    uint8_t const  al = (uint8_t)ax % bImm;
    uint8_t const  ah = (uint8_t)ax / bImm;
    pCtx->ax = (ah << 8) + al;
    iemHlpUpdateArithEFlagsU8(pVCpu, al,
                              X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF,
                              X86_EFL_OF | X86_EFL_AF | X86_EFL_CF);

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'DAA'.
 */
IEM_CIMPL_DEF_0(iemCImpl_daa)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    uint8_t const  al       = pCtx->al;
    bool const     fCarry   = pCtx->eflags.Bits.u1CF;

    if (   pCtx->eflags.Bits.u1AF
        || (al & 0xf) >= 10)
    {
        pCtx->al = al + 6;
        pCtx->eflags.Bits.u1AF = 1;
    }
    else
        pCtx->eflags.Bits.u1AF = 0;

    if (al >= 0x9a || fCarry)
    {
        pCtx->al += 0x60;
        pCtx->eflags.Bits.u1CF = 1;
    }
    else
        pCtx->eflags.Bits.u1CF = 0;

    iemHlpUpdateArithEFlagsU8(pVCpu, pCtx->al, X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'DAS'.
 */
IEM_CIMPL_DEF_0(iemCImpl_das)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    uint8_t const  uInputAL = pCtx->al;
    bool const     fCarry   = pCtx->eflags.Bits.u1CF;

    if (   pCtx->eflags.Bits.u1AF
        || (uInputAL & 0xf) >= 10)
    {
        pCtx->eflags.Bits.u1AF = 1;
        if (uInputAL < 6)
            pCtx->eflags.Bits.u1CF = 1;
        pCtx->al = uInputAL - 6;
    }
    else
    {
        pCtx->eflags.Bits.u1AF = 0;
        pCtx->eflags.Bits.u1CF = 0;
    }

    if (uInputAL >= 0x9a || fCarry)
    {
        pCtx->al -= 0x60;
        pCtx->eflags.Bits.u1CF = 1;
    }

    iemHlpUpdateArithEFlagsU8(pVCpu, pCtx->al, X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'AAA'.
 */
IEM_CIMPL_DEF_0(iemCImpl_aaa)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    if (IEM_IS_GUEST_CPU_AMD(pVCpu))
    {
        if (   pCtx->eflags.Bits.u1AF
            || (pCtx->ax & 0xf) >= 10)
        {
            iemAImpl_add_u16(&pCtx->ax, 0x106, &pCtx->eflags.u32);
            pCtx->eflags.Bits.u1AF = 1;
            pCtx->eflags.Bits.u1CF = 1;
#ifdef IEM_VERIFICATION_MODE_FULL
            pVCpu->iem.s.fUndefinedEFlags |= X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF;
#endif
        }
        else
        {
            iemHlpUpdateArithEFlagsU16(pVCpu, pCtx->ax, X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF);
            pCtx->eflags.Bits.u1AF = 0;
            pCtx->eflags.Bits.u1CF = 0;
        }
        pCtx->ax &= UINT16_C(0xff0f);
    }
    else
    {
        if (   pCtx->eflags.Bits.u1AF
            || (pCtx->ax & 0xf) >= 10)
        {
            pCtx->ax += UINT16_C(0x106);
            pCtx->eflags.Bits.u1AF = 1;
            pCtx->eflags.Bits.u1CF = 1;
        }
        else
        {
            pCtx->eflags.Bits.u1AF = 0;
            pCtx->eflags.Bits.u1CF = 0;
        }
        pCtx->ax &= UINT16_C(0xff0f);
        iemHlpUpdateArithEFlagsU8(pVCpu, pCtx->al, X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF);
    }

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'AAS'.
 */
IEM_CIMPL_DEF_0(iemCImpl_aas)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    if (IEM_IS_GUEST_CPU_AMD(pVCpu))
    {
        if (   pCtx->eflags.Bits.u1AF
            || (pCtx->ax & 0xf) >= 10)
        {
            iemAImpl_sub_u16(&pCtx->ax, 0x106, &pCtx->eflags.u32);
            pCtx->eflags.Bits.u1AF = 1;
            pCtx->eflags.Bits.u1CF = 1;
#ifdef IEM_VERIFICATION_MODE_FULL
            pVCpu->iem.s.fUndefinedEFlags |= X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF;
#endif
        }
        else
        {
            iemHlpUpdateArithEFlagsU16(pVCpu, pCtx->ax, X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF);
            pCtx->eflags.Bits.u1AF = 0;
            pCtx->eflags.Bits.u1CF = 0;
        }
        pCtx->ax &= UINT16_C(0xff0f);
    }
    else
    {
        if (   pCtx->eflags.Bits.u1AF
            || (pCtx->ax & 0xf) >= 10)
        {
            pCtx->ax -= UINT16_C(0x106);
            pCtx->eflags.Bits.u1AF = 1;
            pCtx->eflags.Bits.u1CF = 1;
        }
        else
        {
            pCtx->eflags.Bits.u1AF = 0;
            pCtx->eflags.Bits.u1CF = 0;
        }
        pCtx->ax &= UINT16_C(0xff0f);
        iemHlpUpdateArithEFlagsU8(pVCpu, pCtx->al, X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF, X86_EFL_OF);
    }

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements the 16-bit version of 'BOUND'.
 *
 * @note    We have separate 16-bit and 32-bit variants of this function due to
 *          the decoder using unsigned parameters, whereas we want signed one to
 *          do the job.  This is significant for a recompiler.
 */
IEM_CIMPL_DEF_3(iemCImpl_bound_16, int16_t, idxArray, int16_t, idxLowerBound, int16_t, idxUpperBound)
{
    /*
     * Check if the index is inside the bounds, otherwise raise #BR.
     */
    if (   idxArray >= idxLowerBound
        && idxArray <= idxUpperBound)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    return iemRaiseBoundRangeExceeded(pVCpu);
}


/**
 * Implements the 32-bit version of 'BOUND'.
 */
IEM_CIMPL_DEF_3(iemCImpl_bound_32, int32_t, idxArray, int32_t, idxLowerBound, int32_t, idxUpperBound)
{
    /*
     * Check if the index is inside the bounds, otherwise raise #BR.
     */
    if (   idxArray >= idxLowerBound
        && idxArray <= idxUpperBound)
    {
        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }

    return iemRaiseBoundRangeExceeded(pVCpu);
}



/*
 * Instantiate the various string operation combinations.
 */
#define OP_SIZE     8
#define ADDR_SIZE   16
#include "IEMAllCImplStrInstr.cpp.h"
#define OP_SIZE     8
#define ADDR_SIZE   32
#include "IEMAllCImplStrInstr.cpp.h"
#define OP_SIZE     8
#define ADDR_SIZE   64
#include "IEMAllCImplStrInstr.cpp.h"

#define OP_SIZE     16
#define ADDR_SIZE   16
#include "IEMAllCImplStrInstr.cpp.h"
#define OP_SIZE     16
#define ADDR_SIZE   32
#include "IEMAllCImplStrInstr.cpp.h"
#define OP_SIZE     16
#define ADDR_SIZE   64
#include "IEMAllCImplStrInstr.cpp.h"

#define OP_SIZE     32
#define ADDR_SIZE   16
#include "IEMAllCImplStrInstr.cpp.h"
#define OP_SIZE     32
#define ADDR_SIZE   32
#include "IEMAllCImplStrInstr.cpp.h"
#define OP_SIZE     32
#define ADDR_SIZE   64
#include "IEMAllCImplStrInstr.cpp.h"

#define OP_SIZE     64
#define ADDR_SIZE   32
#include "IEMAllCImplStrInstr.cpp.h"
#define OP_SIZE     64
#define ADDR_SIZE   64
#include "IEMAllCImplStrInstr.cpp.h"


/**
 * Implements 'XGETBV'.
 */
IEM_CIMPL_DEF_0(iemCImpl_xgetbv)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    if (pCtx->cr4 & X86_CR4_OSXSAVE)
    {
        uint32_t uEcx = pCtx->ecx;
        switch (uEcx)
        {
            case 0:
                break;

            case 1: /** @todo Implement XCR1 support. */
            default:
                Log(("xgetbv ecx=%RX32 -> #GP(0)\n", uEcx));
                return iemRaiseGeneralProtectionFault0(pVCpu);

        }
        pCtx->rax = RT_LO_U32(pCtx->aXcr[uEcx]);
        pCtx->rdx = RT_HI_U32(pCtx->aXcr[uEcx]);

        iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        return VINF_SUCCESS;
    }
    Log(("xgetbv CR4.OSXSAVE=0 -> UD\n"));
    return iemRaiseUndefinedOpcode(pVCpu);
}


/**
 * Implements 'XSETBV'.
 */
IEM_CIMPL_DEF_0(iemCImpl_xsetbv)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    if (pCtx->cr4 & X86_CR4_OSXSAVE)
    {
        if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_XSETBV))
        {
            Log2(("xsetbv: Guest intercept -> #VMEXIT\n"));
            IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_XSETBV, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }

        if (pVCpu->iem.s.uCpl == 0)
        {
            uint32_t uEcx = pCtx->ecx;
            uint64_t uNewValue = RT_MAKE_U64(pCtx->eax, pCtx->edx);
            switch (uEcx)
            {
                case 0:
                {
                    int rc = CPUMSetGuestXcr0(pVCpu, uNewValue);
                    if (rc == VINF_SUCCESS)
                        break;
                    Assert(rc == VERR_CPUM_RAISE_GP_0);
                    Log(("xsetbv ecx=%RX32 (newvalue=%RX64) -> #GP(0)\n", uEcx, uNewValue));
                    return iemRaiseGeneralProtectionFault0(pVCpu);
                }

                case 1: /** @todo Implement XCR1 support. */
                default:
                    Log(("xsetbv ecx=%RX32 (newvalue=%RX64) -> #GP(0)\n", uEcx, uNewValue));
                    return iemRaiseGeneralProtectionFault0(pVCpu);

            }

            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
            return VINF_SUCCESS;
        }

        Log(("xsetbv cpl=%u -> GP(0)\n", pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    Log(("xsetbv CR4.OSXSAVE=0 -> UD\n"));
    return iemRaiseUndefinedOpcode(pVCpu);
}

#ifdef IN_RING3

/** Argument package for iemCImpl_cmpxchg16b_fallback_rendezvous_callback. */
struct IEMCIMPLCX16ARGS
{
    PRTUINT128U     pu128Dst;
    PRTUINT128U     pu128RaxRdx;
    PRTUINT128U     pu128RbxRcx;
    uint32_t       *pEFlags;
# ifdef VBOX_STRICT
    uint32_t        cCalls;
# endif
};

/**
 * @callback_method_impl{FNVMMEMTRENDEZVOUS,
 *                       Worker for iemCImpl_cmpxchg16b_fallback_rendezvous}
 */
static DECLCALLBACK(VBOXSTRICTRC) iemCImpl_cmpxchg16b_fallback_rendezvous_callback(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    RT_NOREF(pVM, pVCpu);
    struct IEMCIMPLCX16ARGS *pArgs = (struct IEMCIMPLCX16ARGS *)pvUser;
# ifdef VBOX_STRICT
    Assert(pArgs->cCalls == 0);
    pArgs->cCalls++;
# endif

    iemAImpl_cmpxchg16b_fallback(pArgs->pu128Dst, pArgs->pu128RaxRdx, pArgs->pu128RbxRcx, pArgs->pEFlags);
    return VINF_SUCCESS;
}

#endif /* IN_RING3 */

/**
 * Implements 'CMPXCHG16B' fallback using rendezvous.
 */
IEM_CIMPL_DEF_4(iemCImpl_cmpxchg16b_fallback_rendezvous, PRTUINT128U, pu128Dst, PRTUINT128U, pu128RaxRdx,
                PRTUINT128U, pu128RbxRcx, uint32_t *, pEFlags)
{
#ifdef IN_RING3
    struct IEMCIMPLCX16ARGS Args;
    Args.pu128Dst       = pu128Dst;
    Args.pu128RaxRdx    = pu128RaxRdx;
    Args.pu128RbxRcx    = pu128RbxRcx;
    Args.pEFlags        = pEFlags;
# ifdef VBOX_STRICT
    Args.cCalls         = 0;
# endif
    VBOXSTRICTRC rcStrict = VMMR3EmtRendezvous(pVCpu->CTX_SUFF(pVM), VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE,
                                               iemCImpl_cmpxchg16b_fallback_rendezvous_callback, &Args);
    Assert(Args.cCalls == 1);
    if (rcStrict == VINF_SUCCESS)
    {
        /* Duplicated tail code. */
        rcStrict = iemMemCommitAndUnmap(pVCpu, pu128Dst, IEM_ACCESS_DATA_RW);
        if (rcStrict == VINF_SUCCESS)
        {
            PCPUMCTX pCtx = pVCpu->iem.s.CTX_SUFF(pCtx);
            pCtx->eflags.u = *pEFlags; /* IEM_MC_COMMIT_EFLAGS */
            if (!(*pEFlags & X86_EFL_ZF))
            {
                pCtx->rax = pu128RaxRdx->s.Lo;
                pCtx->rdx = pu128RaxRdx->s.Hi;
            }
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
        }
    }
    return rcStrict;
#else
    RT_NOREF(pVCpu, cbInstr, pu128Dst, pu128RaxRdx, pu128RbxRcx, pEFlags);
    return VERR_IEM_ASPECT_NOT_IMPLEMENTED; /* This should get us to ring-3 for now.  Should perhaps be replaced later. */
#endif
}


/**
 * Implements 'CLFLUSH' and 'CLFLUSHOPT'.
 *
 * This is implemented in C because it triggers a load like behviour without
 * actually reading anything.  Since that's not so common, it's implemented
 * here.
 *
 * @param   iEffSeg         The effective segment.
 * @param   GCPtrEff        The address of the image.
 */
IEM_CIMPL_DEF_2(iemCImpl_clflush_clflushopt, uint8_t, iEffSeg, RTGCPTR, GCPtrEff)
{
    /*
     * Pretend to do a load w/o reading (see also iemCImpl_monitor and iemMemMap).
     */
    VBOXSTRICTRC rcStrict = iemMemApplySegment(pVCpu, IEM_ACCESS_TYPE_READ | IEM_ACCESS_WHAT_DATA, iEffSeg, 1, &GCPtrEff);
    if (rcStrict == VINF_SUCCESS)
    {
        RTGCPHYS GCPhysMem;
        rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrEff, IEM_ACCESS_TYPE_READ | IEM_ACCESS_WHAT_DATA, &GCPhysMem);
        if (rcStrict == VINF_SUCCESS)
        {
            iemRegAddToRipAndClearRF(pVCpu, cbInstr);
            return VINF_SUCCESS;
        }
    }

    return rcStrict;
}


/**
 * Implements 'FINIT' and 'FNINIT'.
 *
 * @param   fCheckXcpts     Whether to check for umasked pending exceptions or
 *                          not.
 */
IEM_CIMPL_DEF_1(iemCImpl_finit, bool, fCheckXcpts)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    if (pCtx->cr0 & (X86_CR0_EM | X86_CR0_TS))
        return iemRaiseDeviceNotAvailable(pVCpu);

    NOREF(fCheckXcpts); /** @todo trigger pending exceptions:
        if (fCheckXcpts && TODO )
        return iemRaiseMathFault(pVCpu);
     */

    PX86XSAVEAREA pXState = pCtx->CTX_SUFF(pXState);
    pXState->x87.FCW   = 0x37f;
    pXState->x87.FSW   = 0;
    pXState->x87.FTW   = 0x00;         /* 0 - empty. */
    pXState->x87.FPUDP = 0;
    pXState->x87.DS    = 0; //??
    pXState->x87.Rsrvd2= 0;
    pXState->x87.FPUIP = 0;
    pXState->x87.CS    = 0; //??
    pXState->x87.Rsrvd1= 0;
    pXState->x87.FOP   = 0;

    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'FXSAVE'.
 *
 * @param   iEffSeg         The effective segment.
 * @param   GCPtrEff        The address of the image.
 * @param   enmEffOpSize    The operand size (only REX.W really matters).
 */
IEM_CIMPL_DEF_3(iemCImpl_fxsave, uint8_t, iEffSeg, RTGCPTR, GCPtrEff, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Raise exceptions.
     */
    if (pCtx->cr0 & X86_CR0_EM)
        return iemRaiseUndefinedOpcode(pVCpu);
    if (pCtx->cr0 & (X86_CR0_TS | X86_CR0_EM))
        return iemRaiseDeviceNotAvailable(pVCpu);
    if (GCPtrEff & 15)
    {
        /** @todo CPU/VM detection possible! \#AC might not be signal for
         * all/any misalignment sizes, intel says its an implementation detail. */
        if (   (pCtx->cr0 & X86_CR0_AM)
            && pCtx->eflags.Bits.u1AC
            && pVCpu->iem.s.uCpl == 3)
            return iemRaiseAlignmentCheckException(pVCpu);
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Access the memory.
     */
    void *pvMem512;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &pvMem512, 512, iEffSeg, GCPtrEff, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    PX86FXSTATE  pDst = (PX86FXSTATE)pvMem512;
    PCX86FXSTATE pSrc = &pCtx->CTX_SUFF(pXState)->x87;

    /*
     * Store the registers.
     */
    /** @todo CPU/VM detection possible! If CR4.OSFXSR=0 MXCSR it's
     * implementation specific whether MXCSR and XMM0-XMM7 are saved. */

    /* common for all formats */
    pDst->FCW           = pSrc->FCW;
    pDst->FSW           = pSrc->FSW;
    pDst->FTW           = pSrc->FTW & UINT16_C(0xff);
    pDst->FOP           = pSrc->FOP;
    pDst->MXCSR         = pSrc->MXCSR;
    pDst->MXCSR_MASK    = CPUMGetGuestMxCsrMask(pVCpu->CTX_SUFF(pVM));
    for (uint32_t i = 0; i < RT_ELEMENTS(pDst->aRegs); i++)
    {
        /** @todo Testcase: What actually happens to the 6 reserved bytes? I'm clearing
         *        them for now... */
        pDst->aRegs[i].au32[0] = pSrc->aRegs[i].au32[0];
        pDst->aRegs[i].au32[1] = pSrc->aRegs[i].au32[1];
        pDst->aRegs[i].au32[2] = pSrc->aRegs[i].au32[2] & UINT32_C(0xffff);
        pDst->aRegs[i].au32[3] = 0;
    }

    /* FPU IP, CS, DP and DS. */
    pDst->FPUIP  = pSrc->FPUIP;
    pDst->CS     = pSrc->CS;
    pDst->FPUDP  = pSrc->FPUDP;
    pDst->DS     = pSrc->DS;
    if (enmEffOpSize == IEMMODE_64BIT)
    {
        /* Save upper 16-bits of FPUIP (IP:CS:Rsvd1) and FPUDP (DP:DS:Rsvd2). */
        pDst->Rsrvd1 = pSrc->Rsrvd1;
        pDst->Rsrvd2 = pSrc->Rsrvd2;
        pDst->au32RsrvdForSoftware[0] = 0;
    }
    else
    {
        pDst->Rsrvd1 = 0;
        pDst->Rsrvd2 = 0;
        pDst->au32RsrvdForSoftware[0] = X86_FXSTATE_RSVD_32BIT_MAGIC;
    }

    /* XMM registers. */
    if (   !(pCtx->msrEFER & MSR_K6_EFER_FFXSR)
        || pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT
        || pVCpu->iem.s.uCpl != 0)
    {
        uint32_t cXmmRegs = enmEffOpSize == IEMMODE_64BIT ? 16 : 8;
        for (uint32_t i = 0; i < cXmmRegs; i++)
            pDst->aXMM[i] = pSrc->aXMM[i];
        /** @todo Testcase: What happens to the reserved XMM registers? Untouched,
         *        right? */
    }

    /*
     * Commit the memory.
     */
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvMem512, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'FXRSTOR'.
 *
 * @param   GCPtrEff        The address of the image.
 * @param   enmEffOpSize    The operand size (only REX.W really matters).
 */
IEM_CIMPL_DEF_3(iemCImpl_fxrstor, uint8_t, iEffSeg, RTGCPTR, GCPtrEff, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Raise exceptions.
     */
    if (pCtx->cr0 & X86_CR0_EM)
        return iemRaiseUndefinedOpcode(pVCpu);
    if (pCtx->cr0 & (X86_CR0_TS | X86_CR0_EM))
        return iemRaiseDeviceNotAvailable(pVCpu);
    if (GCPtrEff & 15)
    {
        /** @todo CPU/VM detection possible! \#AC might not be signal for
         * all/any misalignment sizes, intel says its an implementation detail. */
        if (   (pCtx->cr0 & X86_CR0_AM)
            && pCtx->eflags.Bits.u1AC
            && pVCpu->iem.s.uCpl == 3)
            return iemRaiseAlignmentCheckException(pVCpu);
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Access the memory.
     */
    void *pvMem512;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &pvMem512, 512, iEffSeg, GCPtrEff, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    PCX86FXSTATE pSrc = (PCX86FXSTATE)pvMem512;
    PX86FXSTATE  pDst = &pCtx->CTX_SUFF(pXState)->x87;

    /*
     * Check the state for stuff which will #GP(0).
     */
    uint32_t const fMXCSR      = pSrc->MXCSR;
    uint32_t const fMXCSR_MASK = CPUMGetGuestMxCsrMask(pVCpu->CTX_SUFF(pVM));
    if (fMXCSR & ~fMXCSR_MASK)
    {
        Log(("fxrstor: MXCSR=%#x (MXCSR_MASK=%#x) -> #GP(0)\n", fMXCSR, fMXCSR_MASK));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Load the registers.
     */
    /** @todo CPU/VM detection possible! If CR4.OSFXSR=0 MXCSR it's
     * implementation specific whether MXCSR and XMM0-XMM7 are restored. */

    /* common for all formats */
    pDst->FCW       = pSrc->FCW;
    pDst->FSW       = pSrc->FSW;
    pDst->FTW       = pSrc->FTW & UINT16_C(0xff);
    pDst->FOP       = pSrc->FOP;
    pDst->MXCSR     = fMXCSR;
    /* (MXCSR_MASK is read-only) */
    for (uint32_t i = 0; i < RT_ELEMENTS(pSrc->aRegs); i++)
    {
        pDst->aRegs[i].au32[0] = pSrc->aRegs[i].au32[0];
        pDst->aRegs[i].au32[1] = pSrc->aRegs[i].au32[1];
        pDst->aRegs[i].au32[2] = pSrc->aRegs[i].au32[2] & UINT32_C(0xffff);
        pDst->aRegs[i].au32[3] = 0;
    }

    /* FPU IP, CS, DP and DS. */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        pDst->FPUIP  = pSrc->FPUIP;
        pDst->CS     = pSrc->CS;
        pDst->Rsrvd1 = pSrc->Rsrvd1;
        pDst->FPUDP  = pSrc->FPUDP;
        pDst->DS     = pSrc->DS;
        pDst->Rsrvd2 = pSrc->Rsrvd2;
    }
    else
    {
        pDst->FPUIP  = pSrc->FPUIP;
        pDst->CS     = pSrc->CS;
        pDst->Rsrvd1 = 0;
        pDst->FPUDP  = pSrc->FPUDP;
        pDst->DS     = pSrc->DS;
        pDst->Rsrvd2 = 0;
    }

    /* XMM registers. */
    if (   !(pCtx->msrEFER & MSR_K6_EFER_FFXSR)
        || pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT
        || pVCpu->iem.s.uCpl != 0)
    {
        uint32_t cXmmRegs = enmEffOpSize == IEMMODE_64BIT ? 16 : 8;
        for (uint32_t i = 0; i < cXmmRegs; i++)
            pDst->aXMM[i] = pSrc->aXMM[i];
    }

    /*
     * Commit the memory.
     */
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvMem512, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'XSAVE'.
 *
 * @param   iEffSeg         The effective segment.
 * @param   GCPtrEff        The address of the image.
 * @param   enmEffOpSize    The operand size (only REX.W really matters).
 */
IEM_CIMPL_DEF_3(iemCImpl_xsave, uint8_t, iEffSeg, RTGCPTR, GCPtrEff, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Raise exceptions.
     */
    if (!(pCtx->cr4 & X86_CR4_OSXSAVE))
        return iemRaiseUndefinedOpcode(pVCpu);
    if (pCtx->cr0 & X86_CR0_TS)
        return iemRaiseDeviceNotAvailable(pVCpu);
    if (GCPtrEff & 63)
    {
        /** @todo CPU/VM detection possible! \#AC might not be signal for
         * all/any misalignment sizes, intel says its an implementation detail. */
        if (   (pCtx->cr0 & X86_CR0_AM)
            && pCtx->eflags.Bits.u1AC
            && pVCpu->iem.s.uCpl == 3)
            return iemRaiseAlignmentCheckException(pVCpu);
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * Calc the requested mask
     */
    uint64_t const fReqComponents = RT_MAKE_U64(pCtx->eax, pCtx->edx) & pCtx->aXcr[0];
    AssertLogRelReturn(!(fReqComponents & ~(XSAVE_C_X87 | XSAVE_C_SSE | XSAVE_C_YMM)), VERR_IEM_ASPECT_NOT_IMPLEMENTED);
    uint64_t const fXInUse        = pCtx->aXcr[0];

/** @todo figure out the exact protocol for the memory access.  Currently we
 *        just need this crap to work halfways to make it possible to test
 *        AVX instructions. */
/** @todo figure out the XINUSE and XMODIFIED   */

    /*
     * Access the x87 memory state.
     */
    /* The x87+SSE state.  */
    void *pvMem512;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &pvMem512, 512, iEffSeg, GCPtrEff, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    PX86FXSTATE  pDst = (PX86FXSTATE)pvMem512;
    PCX86FXSTATE pSrc = &pCtx->CTX_SUFF(pXState)->x87;

    /* The header.  */
    PX86XSAVEHDR pHdr;
    rcStrict = iemMemMap(pVCpu, (void **)&pHdr, sizeof(&pHdr), iEffSeg, GCPtrEff + 512, IEM_ACCESS_DATA_RW);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Store the X87 state.
     */
    if (fReqComponents & XSAVE_C_X87)
    {
        /* common for all formats */
        pDst->FCW    = pSrc->FCW;
        pDst->FSW    = pSrc->FSW;
        pDst->FTW    = pSrc->FTW & UINT16_C(0xff);
        pDst->FOP    = pSrc->FOP;
        pDst->FPUIP  = pSrc->FPUIP;
        pDst->CS     = pSrc->CS;
        pDst->FPUDP  = pSrc->FPUDP;
        pDst->DS     = pSrc->DS;
        if (enmEffOpSize == IEMMODE_64BIT)
        {
            /* Save upper 16-bits of FPUIP (IP:CS:Rsvd1) and FPUDP (DP:DS:Rsvd2). */
            pDst->Rsrvd1 = pSrc->Rsrvd1;
            pDst->Rsrvd2 = pSrc->Rsrvd2;
            pDst->au32RsrvdForSoftware[0] = 0;
        }
        else
        {
            pDst->Rsrvd1 = 0;
            pDst->Rsrvd2 = 0;
            pDst->au32RsrvdForSoftware[0] = X86_FXSTATE_RSVD_32BIT_MAGIC;
        }
        for (uint32_t i = 0; i < RT_ELEMENTS(pDst->aRegs); i++)
        {
            /** @todo Testcase: What actually happens to the 6 reserved bytes? I'm clearing
             *        them for now... */
            pDst->aRegs[i].au32[0] = pSrc->aRegs[i].au32[0];
            pDst->aRegs[i].au32[1] = pSrc->aRegs[i].au32[1];
            pDst->aRegs[i].au32[2] = pSrc->aRegs[i].au32[2] & UINT32_C(0xffff);
            pDst->aRegs[i].au32[3] = 0;
        }

    }

    if (fReqComponents & (XSAVE_C_SSE | XSAVE_C_YMM))
    {
        pDst->MXCSR         = pSrc->MXCSR;
        pDst->MXCSR_MASK    = CPUMGetGuestMxCsrMask(pVCpu->CTX_SUFF(pVM));
    }

    if (fReqComponents & XSAVE_C_SSE)
    {
        /* XMM registers. */
        uint32_t cXmmRegs = enmEffOpSize == IEMMODE_64BIT ? 16 : 8;
        for (uint32_t i = 0; i < cXmmRegs; i++)
            pDst->aXMM[i] = pSrc->aXMM[i];
        /** @todo Testcase: What happens to the reserved XMM registers? Untouched,
         *        right? */
    }

    /* Commit the x87 state bits. (probably wrong) */
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvMem512, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Store AVX state.
     */
    if (fReqComponents & XSAVE_C_YMM)
    {
        /** @todo testcase: xsave64 vs xsave32 wrt XSAVE_C_YMM. */
        AssertLogRelReturn(pCtx->aoffXState[XSAVE_C_YMM_BIT] != UINT16_MAX, VERR_IEM_IPE_9);
        PCX86XSAVEYMMHI pCompSrc = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_YMM_BIT, PCX86XSAVEYMMHI);
        PX86XSAVEYMMHI  pCompDst;
        rcStrict = iemMemMap(pVCpu, (void **)&pCompDst, sizeof(*pCompDst), iEffSeg, GCPtrEff + pCtx->aoffXState[XSAVE_C_YMM_BIT],
                             IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        uint32_t cXmmRegs = enmEffOpSize == IEMMODE_64BIT ? 16 : 8;
        for (uint32_t i = 0; i < cXmmRegs; i++)
            pCompDst->aYmmHi[i] = pCompSrc->aYmmHi[i];

        rcStrict = iemMemCommitAndUnmap(pVCpu, pCompDst, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }

    /*
     * Update the header.
     */
    pHdr->bmXState = (pHdr->bmXState & ~fReqComponents)
                   | (fReqComponents & fXInUse);

    rcStrict = iemMemCommitAndUnmap(pVCpu, pHdr, IEM_ACCESS_DATA_RW);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'XRSTOR'.
 *
 * @param   iEffSeg         The effective segment.
 * @param   GCPtrEff        The address of the image.
 * @param   enmEffOpSize    The operand size (only REX.W really matters).
 */
IEM_CIMPL_DEF_3(iemCImpl_xrstor, uint8_t, iEffSeg, RTGCPTR, GCPtrEff, IEMMODE, enmEffOpSize)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Raise exceptions.
     */
    if (!(pCtx->cr4 & X86_CR4_OSXSAVE))
        return iemRaiseUndefinedOpcode(pVCpu);
    if (pCtx->cr0 & X86_CR0_TS)
        return iemRaiseDeviceNotAvailable(pVCpu);
    if (GCPtrEff & 63)
    {
        /** @todo CPU/VM detection possible! \#AC might not be signal for
         * all/any misalignment sizes, intel says its an implementation detail. */
        if (   (pCtx->cr0 & X86_CR0_AM)
            && pCtx->eflags.Bits.u1AC
            && pVCpu->iem.s.uCpl == 3)
            return iemRaiseAlignmentCheckException(pVCpu);
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

/** @todo figure out the exact protocol for the memory access.  Currently we
 *        just need this crap to work halfways to make it possible to test
 *        AVX instructions. */
/** @todo figure out the XINUSE and XMODIFIED   */

    /*
     * Access the x87 memory state.
     */
    /* The x87+SSE state.  */
    void *pvMem512;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &pvMem512, 512, iEffSeg, GCPtrEff, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    PCX86FXSTATE pSrc = (PCX86FXSTATE)pvMem512;
    PX86FXSTATE  pDst = &pCtx->CTX_SUFF(pXState)->x87;

    /*
     * Calc the requested mask
     */
    PX86XSAVEHDR  pHdrDst = &pCtx->CTX_SUFF(pXState)->Hdr;
    PCX86XSAVEHDR pHdrSrc;
    rcStrict = iemMemMap(pVCpu, (void **)&pHdrSrc, sizeof(&pHdrSrc), iEffSeg, GCPtrEff + 512, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    uint64_t const fReqComponents = RT_MAKE_U64(pCtx->eax, pCtx->edx) & pCtx->aXcr[0];
    AssertLogRelReturn(!(fReqComponents & ~(XSAVE_C_X87 | XSAVE_C_SSE | XSAVE_C_YMM)), VERR_IEM_ASPECT_NOT_IMPLEMENTED);
    //uint64_t const fXInUse        = pCtx->aXcr[0];
    uint64_t const fRstorMask     = pHdrSrc->bmXState;
    uint64_t const fCompMask      = pHdrSrc->bmXComp;

    AssertLogRelReturn(!(fCompMask & XSAVE_C_X), VERR_IEM_ASPECT_NOT_IMPLEMENTED);

    uint32_t const cXmmRegs = enmEffOpSize == IEMMODE_64BIT ? 16 : 8;

    /* We won't need this any longer. */
    rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)pHdrSrc, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Store the X87 state.
     */
    if (fReqComponents & XSAVE_C_X87)
    {
        if (fRstorMask & XSAVE_C_X87)
        {
            pDst->FCW    = pSrc->FCW;
            pDst->FSW    = pSrc->FSW;
            pDst->FTW    = pSrc->FTW & UINT16_C(0xff);
            pDst->FOP    = pSrc->FOP;
            pDst->FPUIP  = pSrc->FPUIP;
            pDst->CS     = pSrc->CS;
            pDst->FPUDP  = pSrc->FPUDP;
            pDst->DS     = pSrc->DS;
            if (enmEffOpSize == IEMMODE_64BIT)
            {
                /* Save upper 16-bits of FPUIP (IP:CS:Rsvd1) and FPUDP (DP:DS:Rsvd2). */
                pDst->Rsrvd1 = pSrc->Rsrvd1;
                pDst->Rsrvd2 = pSrc->Rsrvd2;
            }
            else
            {
                pDst->Rsrvd1 = 0;
                pDst->Rsrvd2 = 0;
            }
            for (uint32_t i = 0; i < RT_ELEMENTS(pDst->aRegs); i++)
            {
                pDst->aRegs[i].au32[0] = pSrc->aRegs[i].au32[0];
                pDst->aRegs[i].au32[1] = pSrc->aRegs[i].au32[1];
                pDst->aRegs[i].au32[2] = pSrc->aRegs[i].au32[2] & UINT32_C(0xffff);
                pDst->aRegs[i].au32[3] = 0;
            }
        }
        else
        {
            pDst->FCW   = 0x37f;
            pDst->FSW   = 0;
            pDst->FTW   = 0x00;         /* 0 - empty. */
            pDst->FPUDP = 0;
            pDst->DS    = 0; //??
            pDst->Rsrvd2= 0;
            pDst->FPUIP = 0;
            pDst->CS    = 0; //??
            pDst->Rsrvd1= 0;
            pDst->FOP   = 0;
            for (uint32_t i = 0; i < RT_ELEMENTS(pSrc->aRegs); i++)
            {
                pDst->aRegs[i].au32[0] = 0;
                pDst->aRegs[i].au32[1] = 0;
                pDst->aRegs[i].au32[2] = 0;
                pDst->aRegs[i].au32[3] = 0;
            }
        }
        pHdrDst->bmXState |= XSAVE_C_X87; /* playing safe for now */
    }

    /* MXCSR */
    if (fReqComponents & (XSAVE_C_SSE | XSAVE_C_YMM))
    {
        if (fRstorMask & (XSAVE_C_SSE | XSAVE_C_YMM))
            pDst->MXCSR = pSrc->MXCSR;
        else
            pDst->MXCSR = 0x1f80;
    }

    /* XMM registers. */
    if (fReqComponents & XSAVE_C_SSE)
    {
        if (fRstorMask & XSAVE_C_SSE)
        {
            for (uint32_t i = 0; i < cXmmRegs; i++)
                pDst->aXMM[i] = pSrc->aXMM[i];
            /** @todo Testcase: What happens to the reserved XMM registers? Untouched,
             *        right? */
        }
        else
        {
            for (uint32_t i = 0; i < cXmmRegs; i++)
            {
                pDst->aXMM[i].au64[0] = 0;
                pDst->aXMM[i].au64[1] = 0;
            }
        }
        pHdrDst->bmXState |= XSAVE_C_SSE; /* playing safe for now */
    }

    /* Unmap the x87 state bits (so we've don't run out of mapping). */
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvMem512, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Restore AVX state.
     */
    if (fReqComponents & XSAVE_C_YMM)
    {
        AssertLogRelReturn(pCtx->aoffXState[XSAVE_C_YMM_BIT] != UINT16_MAX, VERR_IEM_IPE_9);
        PX86XSAVEYMMHI  pCompDst = CPUMCTX_XSAVE_C_PTR(pCtx, XSAVE_C_YMM_BIT, PX86XSAVEYMMHI);

        if (fRstorMask & XSAVE_C_YMM)
        {
            /** @todo testcase: xsave64 vs xsave32 wrt XSAVE_C_YMM. */
            PCX86XSAVEYMMHI pCompSrc;
            rcStrict = iemMemMap(pVCpu, (void **)&pCompSrc, sizeof(*pCompDst),
                                 iEffSeg, GCPtrEff + pCtx->aoffXState[XSAVE_C_YMM_BIT], IEM_ACCESS_DATA_R);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;

            for (uint32_t i = 0; i < cXmmRegs; i++)
            {
                pCompDst->aYmmHi[i].au64[0] = pCompSrc->aYmmHi[i].au64[0];
                pCompDst->aYmmHi[i].au64[1] = pCompSrc->aYmmHi[i].au64[1];
            }

            rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)pCompSrc, IEM_ACCESS_DATA_R);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
        else
        {
            for (uint32_t i = 0; i < cXmmRegs; i++)
            {
                pCompDst->aYmmHi[i].au64[0] = 0;
                pCompDst->aYmmHi[i].au64[1] = 0;
            }
        }
        pHdrDst->bmXState |= XSAVE_C_YMM; /* playing safe for now */
    }

    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}




/**
 * Implements 'STMXCSR'.
 *
 * @param   GCPtrEff        The address of the image.
 */
IEM_CIMPL_DEF_2(iemCImpl_stmxcsr, uint8_t, iEffSeg, RTGCPTR, GCPtrEff)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Raise exceptions.
     */
    if (   !(pCtx->cr0 & X86_CR0_EM)
        && (pCtx->cr4 & X86_CR4_OSFXSR))
    {
        if (!(pCtx->cr0 & X86_CR0_TS))
        {
            /*
             * Do the job.
             */
            VBOXSTRICTRC rcStrict = iemMemStoreDataU32(pVCpu, iEffSeg, GCPtrEff, pCtx->CTX_SUFF(pXState)->x87.MXCSR);
            if (rcStrict == VINF_SUCCESS)
            {
                iemRegAddToRipAndClearRF(pVCpu, cbInstr);
                return VINF_SUCCESS;
            }
            return rcStrict;
        }
        return iemRaiseDeviceNotAvailable(pVCpu);
    }
    return iemRaiseUndefinedOpcode(pVCpu);
}


/**
 * Implements 'VSTMXCSR'.
 *
 * @param   GCPtrEff        The address of the image.
 */
IEM_CIMPL_DEF_2(iemCImpl_vstmxcsr, uint8_t, iEffSeg, RTGCPTR, GCPtrEff)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Raise exceptions.
     */
    if (   (   !IEM_IS_GUEST_CPU_AMD(pVCpu)
            ? (pCtx->aXcr[0] & (XSAVE_C_SSE | XSAVE_C_YMM)) == (XSAVE_C_SSE | XSAVE_C_YMM)
            : !(pCtx->cr0 & X86_CR0_EM)) /* AMD Jaguar CPU (f0x16,m0,s1) behaviour */
        && (pCtx->cr4 & X86_CR4_OSXSAVE))
    {
        if (!(pCtx->cr0 & X86_CR0_TS))
        {
            /*
             * Do the job.
             */
            VBOXSTRICTRC rcStrict = iemMemStoreDataU32(pVCpu, iEffSeg, GCPtrEff, pCtx->CTX_SUFF(pXState)->x87.MXCSR);
            if (rcStrict == VINF_SUCCESS)
            {
                iemRegAddToRipAndClearRF(pVCpu, cbInstr);
                return VINF_SUCCESS;
            }
            return rcStrict;
        }
        return iemRaiseDeviceNotAvailable(pVCpu);
    }
    return iemRaiseUndefinedOpcode(pVCpu);
}


/**
 * Implements 'LDMXCSR'.
 *
 * @param   GCPtrEff        The address of the image.
 */
IEM_CIMPL_DEF_2(iemCImpl_ldmxcsr, uint8_t, iEffSeg, RTGCPTR, GCPtrEff)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Raise exceptions.
     */
    /** @todo testcase - order of LDMXCSR faults.  Does \#PF, \#GP and \#SS
     *        happen after or before \#UD and \#EM? */
    if (   !(pCtx->cr0 & X86_CR0_EM)
        && (pCtx->cr4 & X86_CR4_OSFXSR))
    {
        if (!(pCtx->cr0 & X86_CR0_TS))
        {
            /*
             * Do the job.
             */
            uint32_t fNewMxCsr;
            VBOXSTRICTRC rcStrict = iemMemFetchDataU32(pVCpu, &fNewMxCsr, iEffSeg, GCPtrEff);
            if (rcStrict == VINF_SUCCESS)
            {
                uint32_t const fMxCsrMask = CPUMGetGuestMxCsrMask(pVCpu->CTX_SUFF(pVM));
                if (!(fNewMxCsr & ~fMxCsrMask))
                {
                    pCtx->CTX_SUFF(pXState)->x87.MXCSR = fNewMxCsr;
                    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
                    return VINF_SUCCESS;
                }
                Log(("lddmxcsr: New MXCSR=%#RX32 & ~MASK=%#RX32 = %#RX32 -> #GP(0)\n",
                     fNewMxCsr, fMxCsrMask, fNewMxCsr & ~fMxCsrMask));
                return iemRaiseGeneralProtectionFault0(pVCpu);
            }
            return rcStrict;
        }
        return iemRaiseDeviceNotAvailable(pVCpu);
    }
    return iemRaiseUndefinedOpcode(pVCpu);
}


/**
 * Commmon routine for fnstenv and fnsave.
 *
 * @param   uPtr                Where to store the state.
 * @param   pCtx                The CPU context.
 */
static void iemCImplCommonFpuStoreEnv(PVMCPU pVCpu, IEMMODE enmEffOpSize, RTPTRUNION uPtr, PCCPUMCTX pCtx)
{
    PCX86FXSTATE pSrcX87 = &pCtx->CTX_SUFF(pXState)->x87;
    if (enmEffOpSize == IEMMODE_16BIT)
    {
        uPtr.pu16[0] = pSrcX87->FCW;
        uPtr.pu16[1] = pSrcX87->FSW;
        uPtr.pu16[2] = iemFpuCalcFullFtw(pSrcX87);
        if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
        {
            /** @todo Testcase: How does this work when the FPUIP/CS was saved in
             *        protected mode or long mode and we save it in real mode?  And vice
             *        versa?  And with 32-bit operand size?  I think CPU is storing the
             *        effective address ((CS << 4) + IP) in the offset register and not
             *        doing any address calculations here. */
            uPtr.pu16[3] = (uint16_t)pSrcX87->FPUIP;
            uPtr.pu16[4] = ((pSrcX87->FPUIP >> 4) & UINT16_C(0xf000)) | pSrcX87->FOP;
            uPtr.pu16[5] = (uint16_t)pSrcX87->FPUDP;
            uPtr.pu16[6] = (pSrcX87->FPUDP  >> 4) & UINT16_C(0xf000);
        }
        else
        {
            uPtr.pu16[3] = pSrcX87->FPUIP;
            uPtr.pu16[4] = pSrcX87->CS;
            uPtr.pu16[5] = pSrcX87->FPUDP;
            uPtr.pu16[6] = pSrcX87->DS;
        }
    }
    else
    {
        /** @todo Testcase: what is stored in the "gray" areas? (figure 8-9 and 8-10) */
        uPtr.pu16[0*2]   = pSrcX87->FCW;
        uPtr.pu16[0*2+1] = 0xffff;  /* (0xffff observed on intel skylake.) */
        uPtr.pu16[1*2]   = pSrcX87->FSW;
        uPtr.pu16[1*2+1] = 0xffff;
        uPtr.pu16[2*2]   = iemFpuCalcFullFtw(pSrcX87);
        uPtr.pu16[2*2+1] = 0xffff;
        if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
        {
            uPtr.pu16[3*2]   = (uint16_t)pSrcX87->FPUIP;
            uPtr.pu32[4]     = ((pSrcX87->FPUIP & UINT32_C(0xffff0000)) >> 4) | pSrcX87->FOP;
            uPtr.pu16[5*2]   = (uint16_t)pSrcX87->FPUDP;
            uPtr.pu32[6]     = (pSrcX87->FPUDP  & UINT32_C(0xffff0000)) >> 4;
        }
        else
        {
            uPtr.pu32[3]     = pSrcX87->FPUIP;
            uPtr.pu16[4*2]   = pSrcX87->CS;
            uPtr.pu16[4*2+1] = pSrcX87->FOP;
            uPtr.pu32[5]     = pSrcX87->FPUDP;
            uPtr.pu16[6*2]   = pSrcX87->DS;
            uPtr.pu16[6*2+1] = 0xffff;
        }
    }
}


/**
 * Commmon routine for fldenv and frstor
 *
 * @param   uPtr                Where to store the state.
 * @param   pCtx                The CPU context.
 */
static void iemCImplCommonFpuRestoreEnv(PVMCPU pVCpu, IEMMODE enmEffOpSize, RTCPTRUNION uPtr, PCPUMCTX pCtx)
{
    PX86FXSTATE pDstX87 = &pCtx->CTX_SUFF(pXState)->x87;
    if (enmEffOpSize == IEMMODE_16BIT)
    {
        pDstX87->FCW = uPtr.pu16[0];
        pDstX87->FSW = uPtr.pu16[1];
        pDstX87->FTW = uPtr.pu16[2];
        if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
        {
            pDstX87->FPUIP = uPtr.pu16[3] | ((uint32_t)(uPtr.pu16[4] & UINT16_C(0xf000)) << 4);
            pDstX87->FPUDP = uPtr.pu16[5] | ((uint32_t)(uPtr.pu16[6] & UINT16_C(0xf000)) << 4);
            pDstX87->FOP   = uPtr.pu16[4] & UINT16_C(0x07ff);
            pDstX87->CS    = 0;
            pDstX87->Rsrvd1= 0;
            pDstX87->DS    = 0;
            pDstX87->Rsrvd2= 0;
        }
        else
        {
            pDstX87->FPUIP = uPtr.pu16[3];
            pDstX87->CS    = uPtr.pu16[4];
            pDstX87->Rsrvd1= 0;
            pDstX87->FPUDP = uPtr.pu16[5];
            pDstX87->DS    = uPtr.pu16[6];
            pDstX87->Rsrvd2= 0;
            /** @todo Testcase: Is FOP cleared when doing 16-bit protected mode fldenv? */
        }
    }
    else
    {
        pDstX87->FCW = uPtr.pu16[0*2];
        pDstX87->FSW = uPtr.pu16[1*2];
        pDstX87->FTW = uPtr.pu16[2*2];
        if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
        {
            pDstX87->FPUIP = uPtr.pu16[3*2] | ((uPtr.pu32[4] & UINT32_C(0x0ffff000)) << 4);
            pDstX87->FOP   = uPtr.pu32[4] & UINT16_C(0x07ff);
            pDstX87->FPUDP = uPtr.pu16[5*2] | ((uPtr.pu32[6] & UINT32_C(0x0ffff000)) << 4);
            pDstX87->CS    = 0;
            pDstX87->Rsrvd1= 0;
            pDstX87->DS    = 0;
            pDstX87->Rsrvd2= 0;
        }
        else
        {
            pDstX87->FPUIP = uPtr.pu32[3];
            pDstX87->CS    = uPtr.pu16[4*2];
            pDstX87->Rsrvd1= 0;
            pDstX87->FOP   = uPtr.pu16[4*2+1];
            pDstX87->FPUDP = uPtr.pu32[5];
            pDstX87->DS    = uPtr.pu16[6*2];
            pDstX87->Rsrvd2= 0;
        }
    }

    /* Make adjustments. */
    pDstX87->FTW = iemFpuCompressFtw(pDstX87->FTW);
    pDstX87->FCW &= ~X86_FCW_ZERO_MASK;
    iemFpuRecalcExceptionStatus(pDstX87);
    /** @todo Testcase: Check if ES and/or B are automatically cleared if no
     *        exceptions are pending after loading the saved state? */
}


/**
 * Implements 'FNSTENV'.
 *
 * @param   enmEffOpSize    The operand size (only REX.W really matters).
 * @param   iEffSeg         The effective segment register for @a GCPtrEff.
 * @param   GCPtrEffDst     The address of the image.
 */
IEM_CIMPL_DEF_3(iemCImpl_fnstenv, IEMMODE, enmEffOpSize, uint8_t, iEffSeg, RTGCPTR, GCPtrEffDst)
{
    PCPUMCTX     pCtx = IEM_GET_CTX(pVCpu);
    RTPTRUNION   uPtr;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &uPtr.pv, enmEffOpSize == IEMMODE_16BIT ? 14 : 28,
                                      iEffSeg, GCPtrEffDst, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemCImplCommonFpuStoreEnv(pVCpu, enmEffOpSize, uPtr, pCtx);

    rcStrict = iemMemCommitAndUnmap(pVCpu, uPtr.pv, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Note: C0, C1, C2 and C3 are documented as undefined, we leave them untouched! */
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'FNSAVE'.
 *
 * @param   GCPtrEffDst     The address of the image.
 * @param   enmEffOpSize    The operand size.
 */
IEM_CIMPL_DEF_3(iemCImpl_fnsave, IEMMODE, enmEffOpSize, uint8_t, iEffSeg, RTGCPTR, GCPtrEffDst)
{
    PCPUMCTX     pCtx = IEM_GET_CTX(pVCpu);
    RTPTRUNION   uPtr;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &uPtr.pv, enmEffOpSize == IEMMODE_16BIT ? 94 : 108,
                                      iEffSeg, GCPtrEffDst, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemCImplCommonFpuStoreEnv(pVCpu, enmEffOpSize, uPtr, pCtx);
    PRTFLOAT80U paRegs = (PRTFLOAT80U)(uPtr.pu8 + (enmEffOpSize == IEMMODE_16BIT ? 14 : 28));
    for (uint32_t i = 0; i < RT_ELEMENTS(pFpuCtx->aRegs); i++)
    {
        paRegs[i].au32[0] = pFpuCtx->aRegs[i].au32[0];
        paRegs[i].au32[1] = pFpuCtx->aRegs[i].au32[1];
        paRegs[i].au16[4] = pFpuCtx->aRegs[i].au16[4];
    }

    rcStrict = iemMemCommitAndUnmap(pVCpu, uPtr.pv, IEM_ACCESS_DATA_W | IEM_ACCESS_PARTIAL_WRITE);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Re-initialize the FPU context.
     */
    pFpuCtx->FCW   = 0x37f;
    pFpuCtx->FSW   = 0;
    pFpuCtx->FTW   = 0x00;       /* 0 - empty */
    pFpuCtx->FPUDP = 0;
    pFpuCtx->DS    = 0;
    pFpuCtx->Rsrvd2= 0;
    pFpuCtx->FPUIP = 0;
    pFpuCtx->CS    = 0;
    pFpuCtx->Rsrvd1= 0;
    pFpuCtx->FOP   = 0;

    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}



/**
 * Implements 'FLDENV'.
 *
 * @param   enmEffOpSize    The operand size (only REX.W really matters).
 * @param   iEffSeg         The effective segment register for @a GCPtrEff.
 * @param   GCPtrEffSrc     The address of the image.
 */
IEM_CIMPL_DEF_3(iemCImpl_fldenv, IEMMODE, enmEffOpSize, uint8_t, iEffSeg, RTGCPTR, GCPtrEffSrc)
{
    PCPUMCTX     pCtx = IEM_GET_CTX(pVCpu);
    RTCPTRUNION  uPtr;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, (void **)&uPtr.pv, enmEffOpSize == IEMMODE_16BIT ? 14 : 28,
                                      iEffSeg, GCPtrEffSrc, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemCImplCommonFpuRestoreEnv(pVCpu, enmEffOpSize, uPtr, pCtx);

    rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)uPtr.pv, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'FRSTOR'.
 *
 * @param   GCPtrEffSrc     The address of the image.
 * @param   enmEffOpSize    The operand size.
 */
IEM_CIMPL_DEF_3(iemCImpl_frstor, IEMMODE, enmEffOpSize, uint8_t, iEffSeg, RTGCPTR, GCPtrEffSrc)
{
    PCPUMCTX     pCtx = IEM_GET_CTX(pVCpu);
    RTCPTRUNION  uPtr;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, (void **)&uPtr.pv, enmEffOpSize == IEMMODE_16BIT ? 94 : 108,
                                      iEffSeg, GCPtrEffSrc, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemCImplCommonFpuRestoreEnv(pVCpu, enmEffOpSize, uPtr, pCtx);
    PCRTFLOAT80U paRegs = (PCRTFLOAT80U)(uPtr.pu8 + (enmEffOpSize == IEMMODE_16BIT ? 14 : 28));
    for (uint32_t i = 0; i < RT_ELEMENTS(pFpuCtx->aRegs); i++)
    {
        pFpuCtx->aRegs[i].au32[0] = paRegs[i].au32[0];
        pFpuCtx->aRegs[i].au32[1] = paRegs[i].au32[1];
        pFpuCtx->aRegs[i].au32[2] = paRegs[i].au16[4];
        pFpuCtx->aRegs[i].au32[3] = 0;
    }

    rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)uPtr.pv, IEM_ACCESS_DATA_R);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'FLDCW'.
 *
 * @param   u16Fcw          The new FCW.
 */
IEM_CIMPL_DEF_1(iemCImpl_fldcw, uint16_t, u16Fcw)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /** @todo Testcase: Check what happens when trying to load X86_FCW_PC_RSVD. */
    /** @todo Testcase: Try see what happens when trying to set undefined bits
     *        (other than 6 and 7).  Currently ignoring them. */
    /** @todo Testcase: Test that it raises and loweres the FPU exception bits
     *        according to FSW. (This is was is currently implemented.) */
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    pFpuCtx->FCW = u16Fcw & ~X86_FCW_ZERO_MASK;
    iemFpuRecalcExceptionStatus(pFpuCtx);

    /* Note: C0, C1, C2 and C3 are documented as undefined, we leave them untouched! */
    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}



/**
 * Implements the underflow case of fxch.
 *
 * @param   iStReg              The other stack register.
 */
IEM_CIMPL_DEF_1(iemCImpl_fxch_underflow, uint8_t, iStReg)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    unsigned const iReg1 = X86_FSW_TOP_GET(pFpuCtx->FSW);
    unsigned const iReg2 = (iReg1 + iStReg) & X86_FSW_TOP_SMASK;
    Assert(!(RT_BIT(iReg1) & pFpuCtx->FTW) || !(RT_BIT(iReg2) & pFpuCtx->FTW));

    /** @todo Testcase: fxch underflow. Making assumptions that underflowed
     *        registers are read as QNaN and then exchanged. This could be
     *        wrong... */
    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        if (RT_BIT(iReg1) & pFpuCtx->FTW)
        {
            if (RT_BIT(iReg2) & pFpuCtx->FTW)
                iemFpuStoreQNan(&pFpuCtx->aRegs[0].r80);
            else
                pFpuCtx->aRegs[0].r80 = pFpuCtx->aRegs[iStReg].r80;
            iemFpuStoreQNan(&pFpuCtx->aRegs[iStReg].r80);
        }
        else
        {
            pFpuCtx->aRegs[iStReg].r80 = pFpuCtx->aRegs[0].r80;
            iemFpuStoreQNan(&pFpuCtx->aRegs[0].r80);
        }
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_C1 | X86_FSW_IE | X86_FSW_SF;
    }
    else
    {
        /* raise underflow exception, don't change anything. */
        pFpuCtx->FSW &= ~(X86_FSW_TOP_MASK | X86_FSW_XCPT_MASK);
        pFpuCtx->FSW |= X86_FSW_C1 | X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
    }

    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}


/**
 * Implements 'FCOMI', 'FCOMIP', 'FUCOMI', and 'FUCOMIP'.
 *
 * @param   cToAdd              1 or 7.
 */
IEM_CIMPL_DEF_3(iemCImpl_fcomi_fucomi, uint8_t, iStReg, PFNIEMAIMPLFPUR80EFL, pfnAImpl, bool, fPop)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    Assert(iStReg < 8);

    /*
     * Raise exceptions.
     */
    if (pCtx->cr0 & (X86_CR0_EM | X86_CR0_TS))
        return iemRaiseDeviceNotAvailable(pVCpu);

    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    uint16_t u16Fsw = pFpuCtx->FSW;
    if (u16Fsw & X86_FSW_ES)
        return iemRaiseMathFault(pVCpu);

    /*
     * Check if any of the register accesses causes #SF + #IA.
     */
    unsigned const iReg1 = X86_FSW_TOP_GET(u16Fsw);
    unsigned const iReg2 = (iReg1 + iStReg) & X86_FSW_TOP_SMASK;
    if ((pFpuCtx->FTW & (RT_BIT(iReg1) | RT_BIT(iReg2))) == (RT_BIT(iReg1) | RT_BIT(iReg2)))
    {
        uint32_t u32Eflags = pfnAImpl(pFpuCtx, &u16Fsw, &pFpuCtx->aRegs[0].r80, &pFpuCtx->aRegs[iStReg].r80);
        NOREF(u32Eflags);

        pFpuCtx->FSW &= ~X86_FSW_C1;
        pFpuCtx->FSW |= u16Fsw & ~X86_FSW_TOP_MASK;
        if (   !(u16Fsw & X86_FSW_IE)
            || (pFpuCtx->FCW & X86_FCW_IM) )
        {
            pCtx->eflags.u &= ~(X86_EFL_OF | X86_EFL_SF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF);
            pCtx->eflags.u |= pCtx->eflags.u & (X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF);
        }
    }
    else if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked underflow. */
        pFpuCtx->FSW &= ~X86_FSW_C1;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF;
        pCtx->eflags.u &= ~(X86_EFL_OF | X86_EFL_SF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF);
        pCtx->eflags.u |= X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF;
    }
    else
    {
        /* Raise underflow - don't touch EFLAGS or TOP. */
        pFpuCtx->FSW &= ~X86_FSW_C1;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
        fPop = false;
    }

    /*
     * Pop if necessary.
     */
    if (fPop)
    {
        pFpuCtx->FTW &= ~RT_BIT(iReg1);
        pFpuCtx->FSW &= X86_FSW_TOP_MASK;
        pFpuCtx->FSW |= ((iReg1 + 7) & X86_FSW_TOP_SMASK) << X86_FSW_TOP_SHIFT;
    }

    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemHlpUsedFpu(pVCpu);
    iemRegAddToRipAndClearRF(pVCpu, cbInstr);
    return VINF_SUCCESS;
}

/** @} */


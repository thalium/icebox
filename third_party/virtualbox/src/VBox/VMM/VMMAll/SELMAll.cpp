/* $Id: SELMAll.cpp $ */
/** @file
 * SELM All contexts.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SELM
#include <VBox/vmm/selm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/hm.h>
#include "SELMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <iprt/assert.h>
#include <VBox/vmm/vmm.h>
#include <iprt/x86.h>
#include <iprt/string.h>

#include "SELMInline.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if defined(LOG_ENABLED) && defined(VBOX_WITH_RAW_MODE_NOT_R0)
/** Segment register names. */
static char const g_aszSRegNms[X86_SREG_COUNT][4] = { "ES", "CS", "SS", "DS", "FS", "GS" };
#endif


#ifndef IN_RING0

# ifdef SELM_TRACK_GUEST_GDT_CHANGES
/**
 * @callback_method_impl{FNPGMVIRTHANDLER}
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC)
selmGuestGDTWriteHandler(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvPtr, void *pvBuf, size_t cbBuf,
                         PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    Assert(enmAccessType == PGMACCESSTYPE_WRITE); NOREF(enmAccessType);
    Log(("selmGuestGDTWriteHandler: write to %RGv size %d\n", GCPtr, cbBuf)); NOREF(GCPtr); NOREF(cbBuf);
    NOREF(pvPtr); NOREF(pvBuf); NOREF(enmOrigin); NOREF(pvUser);

#  ifdef IN_RING3
    RT_NOREF_PV(pVM);

    VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
    return VINF_PGM_HANDLER_DO_DEFAULT;

#  else  /* IN_RC: */
    /*
     * Execute the write, doing necessary pre and post shadow GDT checks.
     */
    PCPUMCTX pCtx        = CPUMQueryGuestCtxPtr(pVCpu);
    uint32_t offGuestGdt = pCtx->gdtr.pGdt - GCPtr;
    selmRCGuestGdtPreWriteCheck(pVM, pVCpu, offGuestGdt, cbBuf, pCtx);
    memcpy(pvBuf, pvPtr, cbBuf);
    VBOXSTRICTRC rcStrict = selmRCGuestGdtPostWriteCheck(pVM, pVCpu, offGuestGdt, cbBuf, pCtx);
    if (!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_GDT))
        STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestGDTHandled);
    else
        STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestGDTUnhandled);
    return rcStrict;
#  endif
}
# endif


# ifdef SELM_TRACK_GUEST_LDT_CHANGES
/**
 * @callback_method_impl{FNPGMVIRTHANDLER}
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC)
selmGuestLDTWriteHandler(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvPtr, void *pvBuf, size_t cbBuf,
                         PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    Assert(enmAccessType == PGMACCESSTYPE_WRITE); NOREF(enmAccessType);
    Log(("selmGuestLDTWriteHandler: write to %RGv size %d\n", GCPtr, cbBuf)); NOREF(GCPtr); NOREF(cbBuf);
    NOREF(pvPtr); NOREF(pvBuf); NOREF(enmOrigin); NOREF(pvUser); RT_NOREF_PV(pVM);

    VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
#  ifdef IN_RING3
    return VINF_PGM_HANDLER_DO_DEFAULT;
#  else
    STAM_COUNTER_INC(&pVM->selm.s.StatRCWriteGuestLDT);
    return VINF_EM_RAW_EMULATE_INSTR_LDT_FAULT;
#  endif
}
# endif


# ifdef SELM_TRACK_GUEST_TSS_CHANGES
/**
 * @callback_method_impl{FNPGMVIRTHANDLER}
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC)
selmGuestTSSWriteHandler(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvPtr, void *pvBuf, size_t cbBuf,
                         PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    Assert(enmAccessType == PGMACCESSTYPE_WRITE); NOREF(enmAccessType);
    Log(("selmGuestTSSWriteHandler: write %.*Rhxs to %RGv size %d\n", RT_MIN(8, cbBuf), pvBuf, GCPtr, cbBuf));
    NOREF(pvBuf); NOREF(GCPtr); NOREF(cbBuf); NOREF(enmOrigin); NOREF(pvUser); NOREF(pvPtr);

#  ifdef IN_RING3
    RT_NOREF_PV(pVM);

    /** @todo This can be optimized by checking for the ESP0 offset and tracking TR
     *        reloads in REM (setting VM_FF_SELM_SYNC_TSS if TR is reloaded). We
     *        should probably also deregister the virtual handler if TR.base/size
     *        changes while we're in REM.  May also share
     *        selmRCGuestTssPostWriteCheck code. */
    VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
    return VINF_PGM_HANDLER_DO_DEFAULT;

#  else  /* IN_RC */
    /*
     * Do the write and check if anything relevant changed.
     */
    Assert(pVM->selm.s.GCPtrGuestTss != (uintptr_t)RTRCPTR_MAX);
    memcpy(pvPtr, pvBuf, cbBuf);
    return selmRCGuestTssPostWriteCheck(pVM, pVCpu, GCPtr - pVM->selm.s.GCPtrGuestTss, cbBuf);
#  endif
}
# endif

#endif /* IN_RING0 */


#ifdef VBOX_WITH_RAW_MODE_NOT_R0
/**
 * Converts a GC selector based address to a flat address.
 *
 * No limit checks are done. Use the SELMToFlat*() or SELMValidate*() functions
 * for that.
 *
 * @returns Flat address.
 * @param   pVM     The cross context VM structure.
 * @param   Sel     Selector part.
 * @param   Addr    Address part.
 * @remarks Don't use when in long mode.
 */
VMMDECL(RTGCPTR) SELMToFlatBySel(PVM pVM, RTSEL Sel, RTGCPTR Addr)
{
    Assert(pVM->cCpus == 1 && !CPUMIsGuestInLongMode(VMMGetCpu(pVM)));    /* DON'T USE! */
    Assert(!HMIsEnabled(pVM));

    /** @todo check the limit. */
    X86DESC    Desc;
    if (!(Sel & X86_SEL_LDT))
        Desc = pVM->selm.s.CTX_SUFF(paGdt)[Sel >> X86_SEL_SHIFT];
    else
    {
        /** @todo handle LDT pages not present! */
        PX86DESC    paLDT = (PX86DESC)((char *)pVM->selm.s.CTX_SUFF(pvLdt) + pVM->selm.s.offLdtHyper);
        Desc = paLDT[Sel >> X86_SEL_SHIFT];
    }

    return (RTGCPTR)(((RTGCUINTPTR)Addr + X86DESC_BASE(&Desc)) & 0xffffffff);
}
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */


/**
 * Converts a GC selector based address to a flat address.
 *
 * No limit checks are done. Use the SELMToFlat*() or SELMValidate*() functions
 * for that.
 *
 * @returns Flat address.
 * @param   pVM         The cross context VM structure.
 * @param   SelReg      Selector register
 * @param   pCtxCore    CPU context
 * @param   Addr        Address part.
 */
VMMDECL(RTGCPTR) SELMToFlat(PVM pVM, DISSELREG SelReg, PCPUMCTXCORE pCtxCore, RTGCPTR Addr)
{
    PCPUMSELREG    pSReg;
    PVMCPU         pVCpu = VMMGetCpu(pVM);

    int rc = DISFetchRegSegEx(pCtxCore, SelReg, &pSReg); AssertRC(rc);

    /*
     * Deal with real & v86 mode first.
     */
    if (    pCtxCore->eflags.Bits.u1VM
        ||  CPUMIsGuestInRealMode(pVCpu))
    {
        uint32_t uFlat = (uint32_t)Addr & 0xffff;
        if (CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg))
            uFlat += (uint32_t)pSReg->u64Base;
        else
            uFlat += (uint32_t)pSReg->Sel << 4;
        return (RTGCPTR)uFlat;
    }

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    /** @todo when we're in 16 bits mode, we should cut off the address as well?? */
    if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg))
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, pSReg);
    if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtxCore->cs))
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, &pCtxCore->cs);
#else
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtxCore->cs));
#endif

    /* 64 bits mode: CS, DS, ES and SS are treated as if each segment base is 0
       (Intel® 64 and IA-32 Architectures Software Developer's Manual: 3.4.2.1). */
    if (    pCtxCore->cs.Attr.n.u1Long
        &&  CPUMIsGuestInLongMode(pVCpu))
    {
        switch (SelReg)
        {
            case DISSELREG_FS:
            case DISSELREG_GS:
                return (RTGCPTR)(pSReg->u64Base + Addr);

            default:
                return Addr;    /* base 0 */
        }
    }

    /* AMD64 manual: compatibility mode ignores the high 32 bits when calculating an effective address. */
    Assert(pSReg->u64Base <= 0xffffffff);
    return (uint32_t)pSReg->u64Base + (uint32_t)Addr;
}


/**
 * Converts a GC selector based address to a flat address.
 *
 * Some basic checking is done, but not all kinds yet.
 *
 * @returns VBox status
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   SelReg      Selector register.
 * @param   pCtxCore    CPU context.
 * @param   Addr        Address part.
 * @param   fFlags      SELMTOFLAT_FLAGS_*
 *                      GDT entires are valid.
 * @param   ppvGC       Where to store the GC flat address.
 */
VMMDECL(int) SELMToFlatEx(PVMCPU pVCpu, DISSELREG SelReg, PCPUMCTXCORE pCtxCore, RTGCPTR Addr, uint32_t fFlags, PRTGCPTR ppvGC)
{
    /*
     * Fetch the selector first.
     */
    PCPUMSELREG pSReg;
    int rc = DISFetchRegSegEx(pCtxCore, SelReg, &pSReg);
    AssertRCReturn(rc, rc); AssertPtr(pSReg);

    /*
     * Deal with real & v86 mode first.
     */
    if (    pCtxCore->eflags.Bits.u1VM
        ||  CPUMIsGuestInRealMode(pVCpu))
    {
        if (ppvGC)
        {
            uint32_t uFlat = (uint32_t)Addr & 0xffff;
            if (CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg))
                *ppvGC = (uint32_t)pSReg->u64Base + uFlat;
            else
                *ppvGC = ((uint32_t)pSReg->Sel << 4) + uFlat;
        }
        return VINF_SUCCESS;
    }

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg))
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, pSReg);
    if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtxCore->cs))
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, &pCtxCore->cs);
#else
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtxCore->cs));
#endif

    /* 64 bits mode: CS, DS, ES and SS are treated as if each segment base is 0
       (Intel® 64 and IA-32 Architectures Software Developer's Manual: 3.4.2.1). */
    RTGCPTR  pvFlat;
    bool     fCheckLimit   = true;
    if (    pCtxCore->cs.Attr.n.u1Long
        &&  CPUMIsGuestInLongMode(pVCpu))
    {
        fCheckLimit = false;
        switch (SelReg)
        {
            case DISSELREG_FS:
            case DISSELREG_GS:
                pvFlat = pSReg->u64Base + Addr;
                break;

            default:
                pvFlat = Addr;
                break;
        }
    }
    else
    {
        /* AMD64 manual: compatibility mode ignores the high 32 bits when calculating an effective address. */
        Assert(pSReg->u64Base <= UINT32_C(0xffffffff));
        pvFlat  = (uint32_t)pSReg->u64Base + (uint32_t)Addr;
        Assert(pvFlat <= UINT32_MAX);
    }

    /*
     * Check type if present.
     */
    if (pSReg->Attr.n.u1Present)
    {
        switch (pSReg->Attr.n.u4Type)
        {
            /* Read only selector type. */
            case X86_SEL_TYPE_RO:
            case X86_SEL_TYPE_RO_ACC:
            case X86_SEL_TYPE_RW:
            case X86_SEL_TYPE_RW_ACC:
            case X86_SEL_TYPE_EO:
            case X86_SEL_TYPE_EO_ACC:
            case X86_SEL_TYPE_ER:
            case X86_SEL_TYPE_ER_ACC:
                if (!(fFlags & SELMTOFLAT_FLAGS_NO_PL))
                {
                    /** @todo fix this mess */
                }
                /* check limit. */
                if (fCheckLimit && Addr > pSReg->u32Limit)
                    return VERR_OUT_OF_SELECTOR_BOUNDS;
                /* ok */
                if (ppvGC)
                    *ppvGC = pvFlat;
                return VINF_SUCCESS;

            case X86_SEL_TYPE_EO_CONF:
            case X86_SEL_TYPE_EO_CONF_ACC:
            case X86_SEL_TYPE_ER_CONF:
            case X86_SEL_TYPE_ER_CONF_ACC:
                if (!(fFlags & SELMTOFLAT_FLAGS_NO_PL))
                {
                    /** @todo fix this mess */
                }
                /* check limit. */
                if (fCheckLimit && Addr > pSReg->u32Limit)
                    return VERR_OUT_OF_SELECTOR_BOUNDS;
                /* ok */
                if (ppvGC)
                    *ppvGC = pvFlat;
                return VINF_SUCCESS;

            case X86_SEL_TYPE_RO_DOWN:
            case X86_SEL_TYPE_RO_DOWN_ACC:
            case X86_SEL_TYPE_RW_DOWN:
            case X86_SEL_TYPE_RW_DOWN_ACC:
                if (!(fFlags & SELMTOFLAT_FLAGS_NO_PL))
                {
                    /** @todo fix this mess */
                }
                /* check limit. */
                if (fCheckLimit)
                {
                    if (!pSReg->Attr.n.u1Granularity && Addr > UINT32_C(0xffff))
                        return VERR_OUT_OF_SELECTOR_BOUNDS;
                    if (Addr <= pSReg->u32Limit)
                        return VERR_OUT_OF_SELECTOR_BOUNDS;
                }
                /* ok */
                if (ppvGC)
                    *ppvGC = pvFlat;
                return VINF_SUCCESS;

            default:
                return VERR_INVALID_SELECTOR;

        }
    }
    return VERR_SELECTOR_NOT_PRESENT;
}


#ifdef VBOX_WITH_RAW_MODE_NOT_R0
/**
 * Converts a GC selector based address to a flat address.
 *
 * Some basic checking is done, but not all kinds yet.
 *
 * @returns VBox status
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   eflags      Current eflags
 * @param   Sel         Selector part.
 * @param   Addr        Address part.
 * @param   fFlags      SELMTOFLAT_FLAGS_*
 *                      GDT entires are valid.
 * @param   ppvGC       Where to store the GC flat address.
 * @param   pcb         Where to store the bytes from *ppvGC which can be accessed according to
 *                      the selector. NULL is allowed.
 * @remarks Don't use when in long mode.
 */
VMMDECL(int) SELMToFlatBySelEx(PVMCPU pVCpu, X86EFLAGS eflags, RTSEL Sel, RTGCPTR Addr,
                               uint32_t fFlags, PRTGCPTR ppvGC, uint32_t *pcb)
{
    Assert(!CPUMIsGuestInLongMode(pVCpu));    /* DON'T USE! (Accessing shadow GDT/LDT.) */
    Assert(!HMIsEnabled(pVCpu->CTX_SUFF(pVM)));

    /*
     * Deal with real & v86 mode first.
     */
    if (    eflags.Bits.u1VM
        ||  CPUMIsGuestInRealMode(pVCpu))
    {
        RTGCUINTPTR uFlat = (RTGCUINTPTR)Addr & 0xffff;
        if (ppvGC)
            *ppvGC = ((RTGCUINTPTR)Sel << 4) + uFlat;
        if (pcb)
            *pcb = 0x10000 - uFlat;
        return VINF_SUCCESS;
    }

    /** @todo when we're in 16 bits mode, we should cut off the address as well?? */
    X86DESC Desc;
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    if (!(Sel & X86_SEL_LDT))
    {
        if (   !(fFlags & SELMTOFLAT_FLAGS_HYPER)
            && (Sel | X86_SEL_RPL_LDT) > pVM->selm.s.GuestGdtr.cbGdt)
            return VERR_INVALID_SELECTOR;
        Desc = pVM->selm.s.CTX_SUFF(paGdt)[Sel >> X86_SEL_SHIFT];
    }
    else
    {
        if ((Sel | X86_SEL_RPL_LDT) > pVM->selm.s.cbLdtLimit)
            return VERR_INVALID_SELECTOR;

        /** @todo handle LDT page(s) not present! */
        PX86DESC paLDT = (PX86DESC)((char *)pVM->selm.s.CTX_SUFF(pvLdt) + pVM->selm.s.offLdtHyper);
        Desc = paLDT[Sel >> X86_SEL_SHIFT];
    }

    /* calc limit. */
    uint32_t u32Limit = X86DESC_LIMIT_G(&Desc);

    /* calc address assuming straight stuff. */
    RTGCPTR pvFlat = Addr + X86DESC_BASE(&Desc);

    /* Cut the address to 32 bits. */
    Assert(!CPUMIsGuestInLongMode(pVCpu));
    pvFlat &= 0xffffffff;

    uint8_t u1Present     = Desc.Gen.u1Present;
    uint8_t u1Granularity = Desc.Gen.u1Granularity;
    uint8_t u1DescType    = Desc.Gen.u1DescType;
    uint8_t u4Type        = Desc.Gen.u4Type;

    /*
     * Check if present.
     */
    if (u1Present)
    {
        /*
         * Type check.
         */
#define BOTH(a, b) ((a << 16) | b)
        switch (BOTH(u1DescType, u4Type))
        {

            /** Read only selector type. */
            case BOTH(1,X86_SEL_TYPE_RO):
            case BOTH(1,X86_SEL_TYPE_RO_ACC):
            case BOTH(1,X86_SEL_TYPE_RW):
            case BOTH(1,X86_SEL_TYPE_RW_ACC):
            case BOTH(1,X86_SEL_TYPE_EO):
            case BOTH(1,X86_SEL_TYPE_EO_ACC):
            case BOTH(1,X86_SEL_TYPE_ER):
            case BOTH(1,X86_SEL_TYPE_ER_ACC):
                if (!(fFlags & SELMTOFLAT_FLAGS_NO_PL))
                {
                    /** @todo fix this mess */
                }
                /* check limit. */
                if ((RTGCUINTPTR)Addr > u32Limit)
                    return VERR_OUT_OF_SELECTOR_BOUNDS;
                /* ok */
                if (ppvGC)
                    *ppvGC = pvFlat;
                if (pcb)
                    *pcb = u32Limit - (uint32_t)Addr + 1;
                return VINF_SUCCESS;

            case BOTH(1,X86_SEL_TYPE_EO_CONF):
            case BOTH(1,X86_SEL_TYPE_EO_CONF_ACC):
            case BOTH(1,X86_SEL_TYPE_ER_CONF):
            case BOTH(1,X86_SEL_TYPE_ER_CONF_ACC):
                if (!(fFlags & SELMTOFLAT_FLAGS_NO_PL))
                {
                    /** @todo fix this mess */
                }
                /* check limit. */
                if ((RTGCUINTPTR)Addr > u32Limit)
                    return VERR_OUT_OF_SELECTOR_BOUNDS;
                /* ok */
                if (ppvGC)
                    *ppvGC = pvFlat;
                if (pcb)
                    *pcb = u32Limit - (uint32_t)Addr + 1;
                return VINF_SUCCESS;

            case BOTH(1,X86_SEL_TYPE_RO_DOWN):
            case BOTH(1,X86_SEL_TYPE_RO_DOWN_ACC):
            case BOTH(1,X86_SEL_TYPE_RW_DOWN):
            case BOTH(1,X86_SEL_TYPE_RW_DOWN_ACC):
                if (!(fFlags & SELMTOFLAT_FLAGS_NO_PL))
                {
                    /** @todo fix this mess */
                }
                /* check limit. */
                if (!u1Granularity && (RTGCUINTPTR)Addr > (RTGCUINTPTR)0xffff)
                    return VERR_OUT_OF_SELECTOR_BOUNDS;
                if ((RTGCUINTPTR)Addr <= u32Limit)
                    return VERR_OUT_OF_SELECTOR_BOUNDS;

                /* ok */
                if (ppvGC)
                    *ppvGC = pvFlat;
                if (pcb)
                    *pcb = (RTGCUINTPTR)(u1Granularity ? 0xffffffff : 0xffff) - (RTGCUINTPTR)Addr + 1;
                return VINF_SUCCESS;

            case BOTH(0,X86_SEL_TYPE_SYS_286_TSS_AVAIL):
            case BOTH(0,X86_SEL_TYPE_SYS_LDT):
            case BOTH(0,X86_SEL_TYPE_SYS_286_TSS_BUSY):
            case BOTH(0,X86_SEL_TYPE_SYS_286_CALL_GATE):
            case BOTH(0,X86_SEL_TYPE_SYS_TASK_GATE):
            case BOTH(0,X86_SEL_TYPE_SYS_286_INT_GATE):
            case BOTH(0,X86_SEL_TYPE_SYS_286_TRAP_GATE):
            case BOTH(0,X86_SEL_TYPE_SYS_386_TSS_AVAIL):
            case BOTH(0,X86_SEL_TYPE_SYS_386_TSS_BUSY):
            case BOTH(0,X86_SEL_TYPE_SYS_386_CALL_GATE):
            case BOTH(0,X86_SEL_TYPE_SYS_386_INT_GATE):
            case BOTH(0,X86_SEL_TYPE_SYS_386_TRAP_GATE):
                if (!(fFlags & SELMTOFLAT_FLAGS_NO_PL))
                {
                    /** @todo fix this mess */
                }
                /* check limit. */
                if ((RTGCUINTPTR)Addr > u32Limit)
                    return VERR_OUT_OF_SELECTOR_BOUNDS;
                /* ok */
                if (ppvGC)
                    *ppvGC = pvFlat;
                if (pcb)
                    *pcb = 0xffffffff - (RTGCUINTPTR)pvFlat + 1; /* Depends on the type.. fixme if we care. */
                return VINF_SUCCESS;

            default:
                return VERR_INVALID_SELECTOR;

        }
#undef BOTH
    }
    return VERR_SELECTOR_NOT_PRESENT;
}
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */


#ifdef VBOX_WITH_RAW_MODE_NOT_R0

static void selLoadHiddenSelectorRegFromGuestTable(PVMCPU pVCpu, PCCPUMCTX pCtx, PCPUMSELREG pSReg,
                                                   RTGCPTR GCPtrDesc, RTSEL const Sel, uint32_t const iSReg)
{
    Assert(!HMIsEnabled(pVCpu->CTX_SUFF(pVM)));
    RT_NOREF_PV(pCtx); RT_NOREF_PV(Sel);

    /*
     * Try read the entry.
     */
    X86DESC GstDesc;
    VBOXSTRICTRC rcStrict = PGMPhysReadGCPtr(pVCpu, &GstDesc, GCPtrDesc, sizeof(GstDesc), PGMACCESSORIGIN_SELM);
    if (rcStrict == VINF_SUCCESS)
    {
        /*
         * Validate it and load it.
         */
        if (selmIsGstDescGoodForSReg(pVCpu, pSReg, &GstDesc, iSReg, CPUMGetGuestCPL(pVCpu)))
        {
            selmLoadHiddenSRegFromGuestDesc(pVCpu, pSReg, &GstDesc);
            Log(("SELMLoadHiddenSelectorReg: loaded %s=%#x:{b=%llx, l=%x, a=%x, vs=%x} (gst)\n",
                 g_aszSRegNms[iSReg], Sel, pSReg->u64Base, pSReg->u32Limit, pSReg->Attr.u, pSReg->ValidSel));
            STAM_COUNTER_INC(&pVCpu->CTX_SUFF(pVM)->selm.s.StatLoadHidSelGst);
        }
        else
        {
            Log(("SELMLoadHiddenSelectorReg: Guest table entry is no good (%s=%#x): %.8Rhxs\n", g_aszSRegNms[iSReg], Sel, &GstDesc));
            STAM_REL_COUNTER_INC(&pVCpu->CTX_SUFF(pVM)->selm.s.StatLoadHidSelGstNoGood);
        }
    }
    else
    {
        AssertMsg(RT_FAILURE_NP(rcStrict), ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
        Log(("SELMLoadHiddenSelectorReg: Error reading descriptor %s=%#x: %Rrc\n",
             g_aszSRegNms[iSReg], Sel, VBOXSTRICTRC_VAL(rcStrict) ));
        STAM_REL_COUNTER_INC(&pVCpu->CTX_SUFF(pVM)->selm.s.StatLoadHidSelReadErrors);
    }
}


/**
 * CPUM helper that loads the hidden selector register from the descriptor table
 * when executing with raw-mode.
 *
 * @remarks This is only used when in legacy protected mode!
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtx        The guest CPU context.
 * @param   pSReg       The selector register.
 *
 * @todo    Deal 100% correctly with stale selectors.  What's more evil is
 *          invalid page table entries, which isn't impossible to imagine for
 *          LDT entries for instance, though unlikely.  Currently, we turn a
 *          blind eye to these issues and return the old hidden registers,
 *          though we don't set the valid flag, so that we'll try loading them
 *          over and over again till we succeed loading something.
 */
VMM_INT_DECL(void) SELMLoadHiddenSelectorReg(PVMCPU pVCpu, PCCPUMCTX pCtx, PCPUMSELREG pSReg)
{
    Assert(pCtx->cr0 & X86_CR0_PE);
    Assert(!(pCtx->msrEFER & MSR_K6_EFER_LMA));

    PVM pVM = pVCpu->CTX_SUFF(pVM);
    Assert(pVM->cCpus == 1);
    Assert(!HMIsEnabled(pVM));


    /*
     * Get the shadow descriptor table entry and validate it.
     * Should something go amiss, try the guest table.
     */
    RTSEL const     Sel   = pSReg->Sel;
    uint32_t const  iSReg = pSReg - CPUMCTX_FIRST_SREG(pCtx); Assert(iSReg < X86_SREG_COUNT);
    PCX86DESC       pShwDesc;
    if (!(Sel & X86_SEL_LDT))
    {
        /** @todo this shall not happen, we shall check for these things when executing
         *        LGDT */
        AssertReturnVoid((Sel | X86_SEL_RPL | X86_SEL_LDT) <= pCtx->gdtr.cbGdt);

        pShwDesc = &pVM->selm.s.CTX_SUFF(paGdt)[Sel >> X86_SEL_SHIFT];
        if (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT)
            || !selmIsShwDescGoodForSReg(pSReg, pShwDesc, iSReg, CPUMGetGuestCPL(pVCpu)))
        {
            selLoadHiddenSelectorRegFromGuestTable(pVCpu, pCtx, pSReg, pCtx->gdtr.pGdt + (Sel & X86_SEL_MASK), Sel, iSReg);
            return;
        }
    }
    else
    {
        /** @todo this shall not happen, we shall check for these things when executing
         *        LLDT */
        AssertReturnVoid((Sel | X86_SEL_RPL | X86_SEL_LDT) <= pCtx->ldtr.u32Limit);

        pShwDesc = (PCX86DESC)((uintptr_t)pVM->selm.s.CTX_SUFF(pvLdt) + pVM->selm.s.offLdtHyper + (Sel & X86_SEL_MASK));
        if (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT)
            || !selmIsShwDescGoodForSReg(pSReg, pShwDesc, iSReg, CPUMGetGuestCPL(pVCpu)))
        {
            selLoadHiddenSelectorRegFromGuestTable(pVCpu, pCtx, pSReg, pCtx->ldtr.u64Base + (Sel & X86_SEL_MASK), Sel, iSReg);
            return;
        }
    }

    /*
     * All fine, load it.
     */
    selmLoadHiddenSRegFromShadowDesc(pSReg, pShwDesc);
    STAM_COUNTER_INC(&pVCpu->CTX_SUFF(pVM)->selm.s.StatLoadHidSelShw);
    Log(("SELMLoadHiddenSelectorReg: loaded %s=%#x:{b=%llx, l=%x, a=%x, vs=%x} (shw)\n",
         g_aszSRegNms[iSReg], Sel, pSReg->u64Base, pSReg->u32Limit, pSReg->Attr.u, pSReg->ValidSel));
}

#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */

/**
 * Validates and converts a GC selector based code address to a flat
 * address when in real or v8086 mode.
 *
 * @returns VINF_SUCCESS.
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   SelCS   Selector part.
 * @param   pSReg   The hidden CS register part. Optional.
 * @param   Addr    Address part.
 * @param   ppvFlat Where to store the flat address.
 */
DECLINLINE(int) selmValidateAndConvertCSAddrRealMode(PVMCPU pVCpu, RTSEL SelCS, PCCPUMSELREGHID pSReg, RTGCPTR Addr,
                                                     PRTGCPTR ppvFlat)
{
    NOREF(pVCpu);
    uint32_t uFlat = Addr & 0xffff;
    if (!pSReg || !CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg))
        uFlat += (uint32_t)SelCS << 4;
    else
        uFlat += (uint32_t)pSReg->u64Base;
    *ppvFlat = uFlat;
    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_RAW_MODE_NOT_R0
/**
 * Validates and converts a GC selector based code address to a flat address
 * when in protected/long mode using the raw-mode algorithm.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   SelCPL      Current privilege level. Get this from SS - CS might be
 *                      conforming! A full selector can be passed, we'll only
 *                      use the RPL part.
 * @param   SelCS       Selector part.
 * @param   Addr        Address part.
 * @param   ppvFlat     Where to store the flat address.
 * @param   pcBits      Where to store the segment bitness (16/32/64). Optional.
 */
DECLINLINE(int) selmValidateAndConvertCSAddrRawMode(PVM pVM, PVMCPU pVCpu, RTSEL SelCPL, RTSEL SelCS, RTGCPTR Addr,
                                                    PRTGCPTR ppvFlat, uint32_t *pcBits)
{
    NOREF(pVCpu);
    Assert(!HMIsEnabled(pVM));

    /** @todo validate limit! */
    X86DESC    Desc;
    if (!(SelCS & X86_SEL_LDT))
        Desc = pVM->selm.s.CTX_SUFF(paGdt)[SelCS >> X86_SEL_SHIFT];
    else
    {
        /** @todo handle LDT page(s) not present! */
        PX86DESC    paLDT = (PX86DESC)((char *)pVM->selm.s.CTX_SUFF(pvLdt) + pVM->selm.s.offLdtHyper);
        Desc = paLDT[SelCS >> X86_SEL_SHIFT];
    }

    /*
     * Check if present.
     */
    if (Desc.Gen.u1Present)
    {
        /*
         * Type check.
         */
        if (    Desc.Gen.u1DescType == 1
            &&  (Desc.Gen.u4Type & X86_SEL_TYPE_CODE))
        {
            /*
             * Check level.
             */
            unsigned uLevel = RT_MAX(SelCPL & X86_SEL_RPL, SelCS & X86_SEL_RPL);
            if (    !(Desc.Gen.u4Type & X86_SEL_TYPE_CONF)
                ?   uLevel <= Desc.Gen.u2Dpl
                :   uLevel >= Desc.Gen.u2Dpl /* hope I got this right now... */
                    )
            {
                /*
                 * Limit check.
                 */
                uint32_t    u32Limit = X86DESC_LIMIT_G(&Desc);
                if ((RTGCUINTPTR)Addr <= u32Limit)
                {
                    *ppvFlat = (RTGCPTR)((RTGCUINTPTR)Addr + X86DESC_BASE(&Desc));
                    /* Cut the address to 32 bits. */
                    *ppvFlat &= 0xffffffff;

                    if (pcBits)
                        *pcBits = Desc.Gen.u1DefBig ? 32 : 16; /** @todo GUEST64 */
                    return VINF_SUCCESS;
                }
                return VERR_OUT_OF_SELECTOR_BOUNDS;
            }
            return VERR_INVALID_RPL;
        }
        return VERR_NOT_CODE_SELECTOR;
    }
    return VERR_SELECTOR_NOT_PRESENT;
}
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */


/**
 * Validates and converts a GC selector based code address to a flat address
 * when in protected/long mode using the standard hidden selector registers
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   SelCPL      Current privilege level.  Get this from SS - CS might be
 *                      conforming!  A full selector can be passed, we'll only
 *                      use the RPL part.
 * @param   SelCS       Selector part.
 * @param   pSRegCS     The full CS selector register.
 * @param   Addr        The address (think IP/EIP/RIP).
 * @param   ppvFlat     Where to store the flat address upon successful return.
 */
DECLINLINE(int) selmValidateAndConvertCSAddrHidden(PVMCPU pVCpu, RTSEL SelCPL, RTSEL SelCS, PCCPUMSELREGHID pSRegCS,
                                                   RTGCPTR Addr, PRTGCPTR ppvFlat)
{
    NOREF(SelCPL); NOREF(SelCS);

    /*
     * Check if present.
     */
    if (pSRegCS->Attr.n.u1Present)
    {
        /*
         * Type check.
         */
        if (     pSRegCS->Attr.n.u1DescType == 1
            &&  (pSRegCS->Attr.n.u4Type & X86_SEL_TYPE_CODE))
        {
            /* 64 bits mode: CS, DS, ES and SS are treated as if each segment base is 0
               (Intel® 64 and IA-32 Architectures Software Developer's Manual: 3.4.2.1). */
            if (    pSRegCS->Attr.n.u1Long
                &&  CPUMIsGuestInLongMode(pVCpu))
            {
                *ppvFlat = Addr;
                return VINF_SUCCESS;
            }

            /*
             * Limit check. Note that the limit in the hidden register is the
             * final value. The granularity bit was included in its calculation.
             */
            uint32_t u32Limit = pSRegCS->u32Limit;
            if ((uint32_t)Addr <= u32Limit)
            {
                *ppvFlat = (uint32_t)Addr + (uint32_t)pSRegCS->u64Base;
                return VINF_SUCCESS;
            }

            return VERR_OUT_OF_SELECTOR_BOUNDS;
        }
        return VERR_NOT_CODE_SELECTOR;
    }
    return VERR_SELECTOR_NOT_PRESENT;
}


/**
 * Validates and converts a GC selector based code address to a flat address.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   Efl         Current EFLAGS.
 * @param   SelCPL      Current privilege level.  Get this from SS - CS might be
 *                      conforming!  A full selector can be passed, we'll only
 *                      use the RPL part.
 * @param   SelCS       Selector part.
 * @param   pSRegCS     The full CS selector register.
 * @param   Addr        The address (think IP/EIP/RIP).
 * @param   ppvFlat     Where to store the flat address upon successful return.
 */
VMMDECL(int) SELMValidateAndConvertCSAddr(PVMCPU pVCpu, X86EFLAGS Efl, RTSEL SelCPL, RTSEL SelCS, PCPUMSELREG pSRegCS,
                                          RTGCPTR Addr, PRTGCPTR ppvFlat)
{
    if (    Efl.Bits.u1VM
        ||  CPUMIsGuestInRealMode(pVCpu))
        return selmValidateAndConvertCSAddrRealMode(pVCpu, SelCS, pSRegCS, Addr, ppvFlat);

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    /* Use the hidden registers when possible, updating them if outdate. */
    if (!pSRegCS)
        return selmValidateAndConvertCSAddrRawMode(pVCpu->CTX_SUFF(pVM), pVCpu, SelCPL, SelCS, Addr, ppvFlat, NULL);

    if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSRegCS))
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, pSRegCS);

    /* Undo ring compression. */
    if ((SelCPL & X86_SEL_RPL) == 1 && !HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        SelCPL &= ~X86_SEL_RPL;
    Assert(pSRegCS->Sel == SelCS);
    if ((SelCS  & X86_SEL_RPL) == 1 && !HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        SelCS  &= ~X86_SEL_RPL;
#else
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSRegCS));
    Assert(pSRegCS->Sel == SelCS);
#endif

    return selmValidateAndConvertCSAddrHidden(pVCpu, SelCPL, SelCS, pSRegCS, Addr, ppvFlat);
}


/**
 * Returns Hypervisor's Trap 08 (\#DF) selector.
 *
 * @returns Hypervisor's Trap 08 (\#DF) selector.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(RTSEL) SELMGetTrap8Selector(PVM pVM)
{
    return pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08];
}


/**
 * Sets EIP of Hypervisor's Trap 08 (\#DF) TSS.
 *
 * @param   pVM     The cross context VM structure.
 * @param   u32EIP  EIP of Trap 08 handler.
 */
VMMDECL(void) SELMSetTrap8EIP(PVM pVM, uint32_t u32EIP)
{
    pVM->selm.s.TssTrap08.eip = u32EIP;
}


/**
 * Sets ss:esp for ring1 in main Hypervisor's TSS.
 *
 * @param   pVM     The cross context VM structure.
 * @param   ss      Ring1 SS register value. Pass 0 if invalid.
 * @param   esp     Ring1 ESP register value.
 */
void selmSetRing1Stack(PVM pVM, uint32_t ss, RTGCPTR32 esp)
{
    Assert(!HMIsEnabled(pVM));
    Assert((ss & 1) || esp == 0);
    pVM->selm.s.Tss.ss1  = ss;
    pVM->selm.s.Tss.esp1 = (uint32_t)esp;
}


#ifdef VBOX_WITH_RAW_RING1
/**
 * Sets ss:esp for ring1 in main Hypervisor's TSS.
 *
 * @param   pVM     The cross context VM structure.
 * @param   ss      Ring2 SS register value. Pass 0 if invalid.
 * @param   esp     Ring2 ESP register value.
 */
void selmSetRing2Stack(PVM pVM, uint32_t ss, RTGCPTR32 esp)
{
    Assert(!HMIsEnabled(pVM));
    Assert((ss & 3) == 2 || esp == 0);
    pVM->selm.s.Tss.ss2  = ss;
    pVM->selm.s.Tss.esp2 = (uint32_t)esp;
}
#endif


#ifdef VBOX_WITH_RAW_MODE_NOT_R0
/**
 * Gets ss:esp for ring1 in main Hypervisor's TSS.
 *
 * Returns SS=0 if the ring-1 stack isn't valid.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 * @param   pSS     Ring1 SS register value.
 * @param   pEsp    Ring1 ESP register value.
 */
VMMDECL(int) SELMGetRing1Stack(PVM pVM, uint32_t *pSS, PRTGCPTR32 pEsp)
{
    Assert(!HMIsEnabled(pVM));
    Assert(pVM->cCpus == 1);
    PVMCPU pVCpu = &pVM->aCpus[0];

#ifdef SELM_TRACK_GUEST_TSS_CHANGES
    if (pVM->selm.s.fSyncTSSRing0Stack)
    {
#endif
        RTGCPTR GCPtrTss = pVM->selm.s.GCPtrGuestTss;
        int     rc;
        VBOXTSS tss;

        Assert(pVM->selm.s.GCPtrGuestTss && pVM->selm.s.cbMonitoredGuestTss);

# ifdef IN_RC
        bool    fTriedAlready = false;

l_tryagain:
        PVBOXTSS pTss = (PVBOXTSS)(uintptr_t)GCPtrTss;
        rc  = MMGCRamRead(pVM, &tss.ss0,  &pTss->ss0,  sizeof(tss.ss0));
        rc |= MMGCRamRead(pVM, &tss.esp0, &pTss->esp0, sizeof(tss.esp0));
#  ifdef DEBUG
        rc |= MMGCRamRead(pVM, &tss.offIoBitmap, &pTss->offIoBitmap, sizeof(tss.offIoBitmap));
#  endif

        if (RT_FAILURE(rc))
        {
            if (!fTriedAlready)
            {
                /* Shadow page might be out of sync. Sync and try again */
                /** @todo might cross page boundary */
                fTriedAlready = true;
                rc = PGMPrefetchPage(pVCpu, (RTGCPTR)GCPtrTss);
                if (rc != VINF_SUCCESS)
                    return rc;
                goto l_tryagain;
            }
            AssertMsgFailed(("Unable to read TSS structure at %08X\n", GCPtrTss));
            return rc;
        }

# else /* !IN_RC */
        /* Reading too much. Could be cheaper than two separate calls though. */
        rc = PGMPhysSimpleReadGCPtr(pVCpu, &tss, GCPtrTss, sizeof(VBOXTSS));
        if (RT_FAILURE(rc))
        {
            AssertReleaseMsgFailed(("Unable to read TSS structure at %08X\n", GCPtrTss));
            return rc;
        }
# endif /* !IN_RC */

# ifdef LOG_ENABLED
        uint32_t ssr0  = pVM->selm.s.Tss.ss1;
        uint32_t espr0 = pVM->selm.s.Tss.esp1;
        ssr0 &= ~1;

        if (ssr0 != tss.ss0 || espr0 != tss.esp0)
            Log(("SELMGetRing1Stack: Updating TSS ring 0 stack to %04X:%08X\n", tss.ss0, tss.esp0));

        Log(("offIoBitmap=%#x\n", tss.offIoBitmap));
# endif
        /* Update our TSS structure for the guest's ring 1 stack */
        selmSetRing1Stack(pVM, tss.ss0 | 1, (RTGCPTR32)tss.esp0);
        pVM->selm.s.fSyncTSSRing0Stack = false;
#ifdef SELM_TRACK_GUEST_TSS_CHANGES
    }
#endif

    *pSS  = pVM->selm.s.Tss.ss1;
    *pEsp = (RTGCPTR32)pVM->selm.s.Tss.esp1;

    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */


#if defined(VBOX_WITH_RAW_MODE) || (HC_ARCH_BITS != 64 && defined(VBOX_WITH_64_BITS_GUESTS))

/**
 * Gets the hypervisor code selector (CS).
 * @returns CS selector.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(RTSEL) SELMGetHyperCS(PVM pVM)
{
    return pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS];
}


/**
 * Gets the 64-mode hypervisor code selector (CS64).
 * @returns CS selector.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(RTSEL) SELMGetHyperCS64(PVM pVM)
{
    return pVM->selm.s.aHyperSel[SELM_HYPER_SEL_CS64];
}


/**
 * Gets the hypervisor data selector (DS).
 * @returns DS selector.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(RTSEL) SELMGetHyperDS(PVM pVM)
{
    return pVM->selm.s.aHyperSel[SELM_HYPER_SEL_DS];
}


/**
 * Gets the hypervisor TSS selector.
 * @returns TSS selector.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(RTSEL) SELMGetHyperTSS(PVM pVM)
{
    return pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS];
}


/**
 * Gets the hypervisor TSS Trap 8 selector.
 * @returns TSS Trap 8 selector.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(RTSEL) SELMGetHyperTSSTrap08(PVM pVM)
{
    return pVM->selm.s.aHyperSel[SELM_HYPER_SEL_TSS_TRAP08];
}

/**
 * Gets the address for the hypervisor GDT.
 *
 * @returns The GDT address.
 * @param   pVM     The cross context VM structure.
 * @remark  This is intended only for very special use, like in the world
 *          switchers. Don't exploit this API!
 */
VMMDECL(RTRCPTR) SELMGetHyperGDT(PVM pVM)
{
    /*
     * Always convert this from the HC pointer since we can be
     * called before the first relocation and have to work correctly
     * without having dependencies on the relocation order.
     */
    return (RTRCPTR)MMHyperR3ToRC(pVM, pVM->selm.s.paGdtR3);
}

#endif /* defined(VBOX_WITH_RAW_MODE) || (HC_ARCH_BITS != 64 && defined(VBOX_WITH_64_BITS_GUESTS)) */

/**
 * Gets info about the current TSS.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if we've got a TSS loaded.
 * @retval  VERR_SELM_NO_TSS if we haven't got a TSS (rather unlikely).
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   pGCPtrTss           Where to store the TSS address.
 * @param   pcbTss              Where to store the TSS size limit.
 * @param   pfCanHaveIOBitmap   Where to store the can-have-I/O-bitmap indicator. (optional)
 */
VMMDECL(int) SELMGetTSSInfo(PVM pVM, PVMCPU pVCpu, PRTGCUINTPTR pGCPtrTss, PRTGCUINTPTR pcbTss, bool *pfCanHaveIOBitmap)
{
    NOREF(pVM);

    /*
     * The TR hidden register is always valid.
     */
    CPUMSELREGHID trHid;
    RTSEL tr = CPUMGetGuestTR(pVCpu, &trHid);
    if (!(tr & X86_SEL_MASK_OFF_RPL))
        return VERR_SELM_NO_TSS;

    *pGCPtrTss = trHid.u64Base;
    *pcbTss    = trHid.u32Limit + (trHid.u32Limit != UINT32_MAX); /* be careful. */
    if (pfCanHaveIOBitmap)
        *pfCanHaveIOBitmap = trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_AVAIL
                          || trHid.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_BUSY;
    return VINF_SUCCESS;
}



/**
 * Notification callback which is called whenever there is a chance that a CR3
 * value might have changed.
 * This is called by PGM.
 *
 * @param   pVM       The cross context VM structure.
 * @param   pVCpu     The cross context virtual CPU structure.
 */
VMMDECL(void) SELMShadowCR3Changed(PVM pVM, PVMCPU pVCpu)
{
    /** @todo SMP support!! (64-bit guest scenario, primarily) */
    pVM->selm.s.Tss.cr3       = PGMGetHyperCR3(pVCpu);
    pVM->selm.s.TssTrap08.cr3 = PGMGetInterRCCR3(pVM, pVCpu);
}


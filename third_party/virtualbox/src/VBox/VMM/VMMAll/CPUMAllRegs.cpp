/* $Id: CPUMAllRegs.cpp $ */
/** @file
 * CPUM - CPU Monitor(/Manager) - Getters and Setters.
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
#define LOG_GROUP LOG_GROUP_CPUM
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#if defined(VBOX_WITH_RAW_MODE) && !defined(IN_RING0)
# include <VBox/vmm/selm.h>
#endif
#include "CPUMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/dis.h>
#include <VBox/log.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/tm.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#ifdef IN_RING3
#include <iprt/thread.h>
#endif

/** Disable stack frame pointer generation here. */
#if defined(_MSC_VER) && !defined(DEBUG) && defined(RT_ARCH_X86)
# pragma optimize("y", off)
#endif

AssertCompile2MemberOffsets(VM, cpum.s.HostFeatures,  cpum.ro.HostFeatures);
AssertCompile2MemberOffsets(VM, cpum.s.GuestFeatures, cpum.ro.GuestFeatures);


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/**
 * Converts a CPUMCPU::Guest pointer into a VMCPU pointer.
 *
 * @returns Pointer to the Virtual CPU.
 * @param   a_pGuestCtx     Pointer to the guest context.
 */
#define CPUM_GUEST_CTX_TO_VMCPU(a_pGuestCtx) RT_FROM_MEMBER(a_pGuestCtx, VMCPU, cpum.s.Guest)

/**
 * Lazily loads the hidden parts of a selector register when using raw-mode.
 */
#if defined(VBOX_WITH_RAW_MODE) && !defined(IN_RING0)
# define CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(a_pVCpu, a_pSReg) \
    do \
    { \
        if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(a_pVCpu, a_pSReg)) \
            cpumGuestLazyLoadHiddenSelectorReg(a_pVCpu, a_pSReg); \
    } while (0)
#else
# define CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(a_pVCpu, a_pSReg) \
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(a_pVCpu, a_pSReg));
#endif



#ifdef VBOX_WITH_RAW_MODE_NOT_R0

/**
 * Does the lazy hidden selector register loading.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pSReg       The selector register to lazily load hidden parts of.
 */
static void cpumGuestLazyLoadHiddenSelectorReg(PVMCPU pVCpu, PCPUMSELREG pSReg)
{
    Assert(!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    Assert(!HMIsEnabled(pVCpu->CTX_SUFF(pVM)));
    Assert((uintptr_t)(pSReg - &pVCpu->cpum.s.Guest.es) < X86_SREG_COUNT);

    if (pVCpu->cpum.s.Guest.eflags.Bits.u1VM)
    {
        /* V8086 mode - Tightly controlled environment, no question about the limit or flags. */
        pSReg->Attr.u               = 0;
        pSReg->Attr.n.u4Type        = pSReg == &pVCpu->cpum.s.Guest.cs ? X86_SEL_TYPE_ER_ACC : X86_SEL_TYPE_RW_ACC;
        pSReg->Attr.n.u1DescType    = 1; /* code/data segment */
        pSReg->Attr.n.u2Dpl         = 3;
        pSReg->Attr.n.u1Present     = 1;
        pSReg->u32Limit             = 0x0000ffff;
        pSReg->u64Base              = (uint32_t)pSReg->Sel << 4;
        pSReg->ValidSel             = pSReg->Sel;
        pSReg->fFlags               = CPUMSELREG_FLAGS_VALID;
        /** @todo Check what the accessed bit should be (VT-x and AMD-V). */
    }
    else if (!(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE))
    {
        /* Real mode - leave the limit and flags alone here, at least for now. */
        pSReg->u64Base              = (uint32_t)pSReg->Sel << 4;
        pSReg->ValidSel             = pSReg->Sel;
        pSReg->fFlags               = CPUMSELREG_FLAGS_VALID;
    }
    else
    {
        /* Protected mode - get it from the selector descriptor tables. */
        if (!(pSReg->Sel & X86_SEL_MASK_OFF_RPL))
        {
            Assert(!CPUMIsGuestInLongMode(pVCpu));
            pSReg->Sel              = 0;
            pSReg->u64Base          = 0;
            pSReg->u32Limit         = 0;
            pSReg->Attr.u           = 0;
            pSReg->ValidSel         = 0;
            pSReg->fFlags           = CPUMSELREG_FLAGS_VALID;
            /** @todo see todo in iemHlpLoadNullDataSelectorProt. */
        }
        else
            SELMLoadHiddenSelectorReg(pVCpu, &pVCpu->cpum.s.Guest, pSReg);
    }
}


/**
 * Makes sure the hidden CS and SS selector registers are valid, loading them if
 * necessary.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
VMM_INT_DECL(void) CPUMGuestLazyLoadHiddenCsAndSs(PVMCPU pVCpu)
{
    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, &pVCpu->cpum.s.Guest.cs);
    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, &pVCpu->cpum.s.Guest.ss);
}


/**
 * Loads a the hidden parts of a selector register.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pSReg       The selector register to lazily load hidden parts of.
 */
VMM_INT_DECL(void) CPUMGuestLazyLoadHiddenSelectorReg(PVMCPU pVCpu, PCPUMSELREG pSReg)
{
    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, pSReg);
}

#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */


/**
 * Obsolete.
 *
 * We don't support nested hypervisor context interrupts or traps.  Life is much
 * simpler when we don't.  It's also slightly faster at times.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(PCCPUMCTXCORE) CPUMGetHyperCtxCore(PVMCPU pVCpu)
{
    return CPUMCTX2CORE(&pVCpu->cpum.s.Hyper);
}


/**
 * Gets the pointer to the hypervisor CPU context structure of a virtual CPU.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(PCPUMCTX) CPUMGetHyperCtxPtr(PVMCPU pVCpu)
{
    return &pVCpu->cpum.s.Hyper;
}


VMMDECL(void) CPUMSetHyperGDTR(PVMCPU pVCpu, uint32_t addr, uint16_t limit)
{
    pVCpu->cpum.s.Hyper.gdtr.cbGdt = limit;
    pVCpu->cpum.s.Hyper.gdtr.pGdt  = addr;
}


VMMDECL(void) CPUMSetHyperIDTR(PVMCPU pVCpu, uint32_t addr, uint16_t limit)
{
    pVCpu->cpum.s.Hyper.idtr.cbIdt = limit;
    pVCpu->cpum.s.Hyper.idtr.pIdt  = addr;
}


VMMDECL(void) CPUMSetHyperCR3(PVMCPU pVCpu, uint32_t cr3)
{
    pVCpu->cpum.s.Hyper.cr3 = cr3;

#ifdef IN_RC
    /* Update the current CR3. */
    ASMSetCR3(cr3);
#endif
}

VMMDECL(uint32_t) CPUMGetHyperCR3(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.cr3;
}


VMMDECL(void) CPUMSetHyperCS(PVMCPU pVCpu, RTSEL SelCS)
{
    pVCpu->cpum.s.Hyper.cs.Sel = SelCS;
}


VMMDECL(void) CPUMSetHyperDS(PVMCPU pVCpu, RTSEL SelDS)
{
    pVCpu->cpum.s.Hyper.ds.Sel = SelDS;
}


VMMDECL(void) CPUMSetHyperES(PVMCPU pVCpu, RTSEL SelES)
{
    pVCpu->cpum.s.Hyper.es.Sel = SelES;
}


VMMDECL(void) CPUMSetHyperFS(PVMCPU pVCpu, RTSEL SelFS)
{
    pVCpu->cpum.s.Hyper.fs.Sel = SelFS;
}


VMMDECL(void) CPUMSetHyperGS(PVMCPU pVCpu, RTSEL SelGS)
{
    pVCpu->cpum.s.Hyper.gs.Sel = SelGS;
}


VMMDECL(void) CPUMSetHyperSS(PVMCPU pVCpu, RTSEL SelSS)
{
    pVCpu->cpum.s.Hyper.ss.Sel = SelSS;
}


VMMDECL(void) CPUMSetHyperESP(PVMCPU pVCpu, uint32_t u32ESP)
{
    pVCpu->cpum.s.Hyper.esp = u32ESP;
}


VMMDECL(void) CPUMSetHyperEDX(PVMCPU pVCpu, uint32_t u32ESP)
{
    pVCpu->cpum.s.Hyper.esp = u32ESP;
}


VMMDECL(int) CPUMSetHyperEFlags(PVMCPU pVCpu, uint32_t Efl)
{
    pVCpu->cpum.s.Hyper.eflags.u32 = Efl;
    return VINF_SUCCESS;
}


VMMDECL(void) CPUMSetHyperEIP(PVMCPU pVCpu, uint32_t u32EIP)
{
    pVCpu->cpum.s.Hyper.eip = u32EIP;
}


/**
 * Used by VMMR3RawRunGC to reinitialize the general raw-mode context registers,
 * EFLAGS and EIP prior to resuming guest execution.
 *
 * All general register not given as a parameter will be set to 0.  The EFLAGS
 * register will be set to sane values for C/C++ code execution with interrupts
 * disabled and IOPL 0.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   u32EIP              The EIP value.
 * @param   u32ESP              The ESP value.
 * @param   u32EAX              The EAX value.
 * @param   u32EDX              The EDX value.
 */
VMM_INT_DECL(void) CPUMSetHyperState(PVMCPU pVCpu, uint32_t u32EIP, uint32_t u32ESP, uint32_t u32EAX, uint32_t u32EDX)
{
    pVCpu->cpum.s.Hyper.eip      = u32EIP;
    pVCpu->cpum.s.Hyper.esp      = u32ESP;
    pVCpu->cpum.s.Hyper.eax      = u32EAX;
    pVCpu->cpum.s.Hyper.edx      = u32EDX;
    pVCpu->cpum.s.Hyper.ecx      = 0;
    pVCpu->cpum.s.Hyper.ebx      = 0;
    pVCpu->cpum.s.Hyper.ebp      = 0;
    pVCpu->cpum.s.Hyper.esi      = 0;
    pVCpu->cpum.s.Hyper.edi      = 0;
    pVCpu->cpum.s.Hyper.eflags.u = X86_EFL_1;
}


VMMDECL(void) CPUMSetHyperTR(PVMCPU pVCpu, RTSEL SelTR)
{
    pVCpu->cpum.s.Hyper.tr.Sel = SelTR;
}


VMMDECL(void) CPUMSetHyperLDTR(PVMCPU pVCpu, RTSEL SelLDTR)
{
    pVCpu->cpum.s.Hyper.ldtr.Sel = SelLDTR;
}


/** @def MAYBE_LOAD_DRx
 * Macro for updating DRx values in raw-mode and ring-0 contexts.
 */
#ifdef IN_RING0
# if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
#  define MAYBE_LOAD_DRx(a_pVCpu, a_fnLoad, a_uValue) \
    do { \
        if (!CPUMIsGuestInLongModeEx(&(a_pVCpu)->cpum.s.Guest)) \
            a_fnLoad(a_uValue); \
        else \
            (a_pVCpu)->cpum.s.fUseFlags |= CPUM_SYNC_DEBUG_REGS_HYPER; \
    } while (0)
# else
#  define MAYBE_LOAD_DRx(a_pVCpu, a_fnLoad, a_uValue) \
    do { \
        a_fnLoad(a_uValue); \
    } while (0)
# endif

#elif defined(IN_RC)
# define MAYBE_LOAD_DRx(a_pVCpu, a_fnLoad, a_uValue) \
    do { \
        if ((a_pVCpu)->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_HYPER) \
        { a_fnLoad(a_uValue); } \
    } while (0)

#else
# define MAYBE_LOAD_DRx(a_pVCpu, a_fnLoad, a_uValue) do { } while (0)
#endif

VMMDECL(void) CPUMSetHyperDR0(PVMCPU pVCpu, RTGCUINTREG uDr0)
{
    pVCpu->cpum.s.Hyper.dr[0] = uDr0;
    MAYBE_LOAD_DRx(pVCpu, ASMSetDR0, uDr0);
}


VMMDECL(void) CPUMSetHyperDR1(PVMCPU pVCpu, RTGCUINTREG uDr1)
{
    pVCpu->cpum.s.Hyper.dr[1] = uDr1;
    MAYBE_LOAD_DRx(pVCpu, ASMSetDR1, uDr1);
}


VMMDECL(void) CPUMSetHyperDR2(PVMCPU pVCpu, RTGCUINTREG uDr2)
{
    pVCpu->cpum.s.Hyper.dr[2] = uDr2;
    MAYBE_LOAD_DRx(pVCpu, ASMSetDR2, uDr2);
}


VMMDECL(void) CPUMSetHyperDR3(PVMCPU pVCpu, RTGCUINTREG uDr3)
{
    pVCpu->cpum.s.Hyper.dr[3] = uDr3;
    MAYBE_LOAD_DRx(pVCpu, ASMSetDR3, uDr3);
}


VMMDECL(void) CPUMSetHyperDR6(PVMCPU pVCpu, RTGCUINTREG uDr6)
{
    pVCpu->cpum.s.Hyper.dr[6] = uDr6;
}


VMMDECL(void) CPUMSetHyperDR7(PVMCPU pVCpu, RTGCUINTREG uDr7)
{
    pVCpu->cpum.s.Hyper.dr[7] = uDr7;
#ifdef IN_RC
    MAYBE_LOAD_DRx(pVCpu, ASMSetDR7, uDr7);
#endif
}


VMMDECL(RTSEL) CPUMGetHyperCS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.cs.Sel;
}


VMMDECL(RTSEL) CPUMGetHyperDS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.ds.Sel;
}


VMMDECL(RTSEL) CPUMGetHyperES(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.es.Sel;
}


VMMDECL(RTSEL) CPUMGetHyperFS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.fs.Sel;
}


VMMDECL(RTSEL) CPUMGetHyperGS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.gs.Sel;
}


VMMDECL(RTSEL) CPUMGetHyperSS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.ss.Sel;
}


VMMDECL(uint32_t) CPUMGetHyperEAX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.eax;
}


VMMDECL(uint32_t) CPUMGetHyperEBX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.ebx;
}


VMMDECL(uint32_t) CPUMGetHyperECX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.ecx;
}


VMMDECL(uint32_t) CPUMGetHyperEDX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.edx;
}


VMMDECL(uint32_t) CPUMGetHyperESI(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.esi;
}


VMMDECL(uint32_t) CPUMGetHyperEDI(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.edi;
}


VMMDECL(uint32_t) CPUMGetHyperEBP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.ebp;
}


VMMDECL(uint32_t) CPUMGetHyperESP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.esp;
}


VMMDECL(uint32_t) CPUMGetHyperEFlags(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.eflags.u32;
}


VMMDECL(uint32_t) CPUMGetHyperEIP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.eip;
}


VMMDECL(uint64_t) CPUMGetHyperRIP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.rip;
}


VMMDECL(uint32_t) CPUMGetHyperIDTR(PVMCPU pVCpu, uint16_t *pcbLimit)
{
    if (pcbLimit)
        *pcbLimit = pVCpu->cpum.s.Hyper.idtr.cbIdt;
    return pVCpu->cpum.s.Hyper.idtr.pIdt;
}


VMMDECL(uint32_t) CPUMGetHyperGDTR(PVMCPU pVCpu, uint16_t *pcbLimit)
{
    if (pcbLimit)
        *pcbLimit = pVCpu->cpum.s.Hyper.gdtr.cbGdt;
    return pVCpu->cpum.s.Hyper.gdtr.pGdt;
}


VMMDECL(RTSEL) CPUMGetHyperLDTR(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.ldtr.Sel;
}


VMMDECL(RTGCUINTREG) CPUMGetHyperDR0(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.dr[0];
}


VMMDECL(RTGCUINTREG) CPUMGetHyperDR1(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.dr[1];
}


VMMDECL(RTGCUINTREG) CPUMGetHyperDR2(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.dr[2];
}


VMMDECL(RTGCUINTREG) CPUMGetHyperDR3(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.dr[3];
}


VMMDECL(RTGCUINTREG) CPUMGetHyperDR6(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.dr[6];
}


VMMDECL(RTGCUINTREG) CPUMGetHyperDR7(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Hyper.dr[7];
}


/**
 * Gets the pointer to the internal CPUMCTXCORE structure.
 * This is only for reading in order to save a few calls.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(PCCPUMCTXCORE) CPUMGetGuestCtxCore(PVMCPU pVCpu)
{
    return CPUMCTX2CORE(&pVCpu->cpum.s.Guest);
}


/**
 * Queries the pointer to the internal CPUMCTX structure.
 *
 * @returns The CPUMCTX pointer.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(PCPUMCTX) CPUMQueryGuestCtxPtr(PVMCPU pVCpu)
{
    return &pVCpu->cpum.s.Guest;
}

VMMDECL(int) CPUMSetGuestGDTR(PVMCPU pVCpu, uint64_t GCPtrBase, uint16_t cbLimit)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_GDT);
#endif
    pVCpu->cpum.s.Guest.gdtr.cbGdt = cbLimit;
    pVCpu->cpum.s.Guest.gdtr.pGdt  = GCPtrBase;
    pVCpu->cpum.s.fChanged |= CPUM_CHANGED_GDTR;
    return VINF_SUCCESS; /* formality, consider it void. */
}

VMMDECL(int) CPUMSetGuestIDTR(PVMCPU pVCpu, uint64_t GCPtrBase, uint16_t cbLimit)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        VMCPU_FF_SET(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
#endif
    pVCpu->cpum.s.Guest.idtr.cbIdt = cbLimit;
    pVCpu->cpum.s.Guest.idtr.pIdt  = GCPtrBase;
    pVCpu->cpum.s.fChanged |= CPUM_CHANGED_IDTR;
    return VINF_SUCCESS; /* formality, consider it void. */
}

VMMDECL(int) CPUMSetGuestTR(PVMCPU pVCpu, uint16_t tr)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_TSS);
#endif
    pVCpu->cpum.s.Guest.tr.Sel  = tr;
    pVCpu->cpum.s.fChanged |= CPUM_CHANGED_TR;
    return VINF_SUCCESS; /* formality, consider it void. */
}

VMMDECL(int) CPUMSetGuestLDTR(PVMCPU pVCpu, uint16_t ldtr)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (   (   ldtr != 0
            || pVCpu->cpum.s.Guest.ldtr.Sel != 0)
        && !HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        VMCPU_FF_SET(pVCpu, VMCPU_FF_SELM_SYNC_LDT);
#endif
    pVCpu->cpum.s.Guest.ldtr.Sel      = ldtr;
    /* The caller will set more hidden bits if it has them. */
    pVCpu->cpum.s.Guest.ldtr.ValidSel = 0;
    pVCpu->cpum.s.Guest.ldtr.fFlags   = 0;
    pVCpu->cpum.s.fChanged  |= CPUM_CHANGED_LDTR;
    return VINF_SUCCESS; /* formality, consider it void. */
}


/**
 * Set the guest CR0.
 *
 * When called in GC, the hyper CR0 may be updated if that is
 * required. The caller only has to take special action if AM,
 * WP, PG or PE changes.
 *
 * @returns VINF_SUCCESS (consider it void).
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   cr0     The new CR0 value.
 */
VMMDECL(int) CPUMSetGuestCR0(PVMCPU pVCpu, uint64_t cr0)
{
#ifdef IN_RC
    /*
     * Check if we need to change hypervisor CR0 because
     * of math stuff.
     */
    if (    (cr0                     & (X86_CR0_TS | X86_CR0_EM | X86_CR0_MP))
        !=  (pVCpu->cpum.s.Guest.cr0 & (X86_CR0_TS | X86_CR0_EM | X86_CR0_MP)))
    {
        if (!(pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_GUEST))
        {
            /*
             * We haven't loaded the guest FPU state yet, so TS and MT are both set
             * and EM should be reflecting the guest EM (it always does this).
             */
            if ((cr0 & X86_CR0_EM) != (pVCpu->cpum.s.Guest.cr0 & X86_CR0_EM))
            {
                uint32_t HyperCR0 = ASMGetCR0();
                AssertMsg((HyperCR0 & (X86_CR0_TS | X86_CR0_MP)) == (X86_CR0_TS | X86_CR0_MP), ("%#x\n", HyperCR0));
                AssertMsg((HyperCR0 & X86_CR0_EM) == (pVCpu->cpum.s.Guest.cr0 & X86_CR0_EM), ("%#x\n", HyperCR0));
                HyperCR0 &= ~X86_CR0_EM;
                HyperCR0 |= cr0 & X86_CR0_EM;
                Log(("CPUM: New HyperCR0=%#x\n", HyperCR0));
                ASMSetCR0(HyperCR0);
            }
# ifdef VBOX_STRICT
            else
            {
                uint32_t HyperCR0 = ASMGetCR0();
                AssertMsg((HyperCR0 & (X86_CR0_TS | X86_CR0_MP)) == (X86_CR0_TS | X86_CR0_MP), ("%#x\n", HyperCR0));
                AssertMsg((HyperCR0 & X86_CR0_EM) == (pVCpu->cpum.s.Guest.cr0 & X86_CR0_EM), ("%#x\n", HyperCR0));
            }
# endif
        }
        else
        {
            /*
             * Already loaded the guest FPU state, so we're just mirroring
             * the guest flags.
             */
            uint32_t HyperCR0 = ASMGetCR0();
            AssertMsg(     (HyperCR0                 & (X86_CR0_TS | X86_CR0_EM | X86_CR0_MP))
                      ==   (pVCpu->cpum.s.Guest.cr0  & (X86_CR0_TS | X86_CR0_EM | X86_CR0_MP)),
                      ("%#x %#x\n", HyperCR0, pVCpu->cpum.s.Guest.cr0));
            HyperCR0 &= ~(X86_CR0_TS | X86_CR0_EM | X86_CR0_MP);
            HyperCR0 |= cr0 & (X86_CR0_TS | X86_CR0_EM | X86_CR0_MP);
            Log(("CPUM: New HyperCR0=%#x\n", HyperCR0));
            ASMSetCR0(HyperCR0);
        }
    }
#endif /* IN_RC */

    /*
     * Check for changes causing TLB flushes (for REM).
     * The caller is responsible for calling PGM when appropriate.
     */
    if (    (cr0                     & (X86_CR0_PG | X86_CR0_WP | X86_CR0_PE))
        !=  (pVCpu->cpum.s.Guest.cr0 & (X86_CR0_PG | X86_CR0_WP | X86_CR0_PE)))
        pVCpu->cpum.s.fChanged |= CPUM_CHANGED_GLOBAL_TLB_FLUSH;
    pVCpu->cpum.s.fChanged |= CPUM_CHANGED_CR0;

    /*
     * Let PGM know if the WP goes from 0 to 1 (netware WP0+RO+US hack)
     */
    if (((cr0 ^ pVCpu->cpum.s.Guest.cr0) & X86_CR0_WP) && (cr0 & X86_CR0_WP))
        PGMCr0WpEnabled(pVCpu);

    /* The ET flag is settable on a 386 and hardwired on 486+. */
    if (   !(cr0 & X86_CR0_ET)
        && pVCpu->CTX_SUFF(pVM)->cpum.s.GuestFeatures.enmMicroarch != kCpumMicroarch_Intel_80386)
        cr0 |= X86_CR0_ET;

    pVCpu->cpum.s.Guest.cr0 = cr0;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestCR2(PVMCPU pVCpu, uint64_t cr2)
{
    pVCpu->cpum.s.Guest.cr2 = cr2;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestCR3(PVMCPU pVCpu, uint64_t cr3)
{
    pVCpu->cpum.s.Guest.cr3 = cr3;
    pVCpu->cpum.s.fChanged |= CPUM_CHANGED_CR3;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestCR4(PVMCPU pVCpu, uint64_t cr4)
{
    /* Note! We don't bother with OSXSAVE and legacy CPUID patches. */

    if (   (cr4                     & (X86_CR4_PGE | X86_CR4_PAE | X86_CR4_PSE))
        != (pVCpu->cpum.s.Guest.cr4 & (X86_CR4_PGE | X86_CR4_PAE | X86_CR4_PSE)))
        pVCpu->cpum.s.fChanged |= CPUM_CHANGED_GLOBAL_TLB_FLUSH;

    pVCpu->cpum.s.fChanged |= CPUM_CHANGED_CR4;
    pVCpu->cpum.s.Guest.cr4 = cr4;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestEFlags(PVMCPU pVCpu, uint32_t eflags)
{
    pVCpu->cpum.s.Guest.eflags.u32 = eflags;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestEIP(PVMCPU pVCpu, uint32_t eip)
{
    pVCpu->cpum.s.Guest.eip = eip;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestEAX(PVMCPU pVCpu, uint32_t eax)
{
    pVCpu->cpum.s.Guest.eax = eax;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestEBX(PVMCPU pVCpu, uint32_t ebx)
{
    pVCpu->cpum.s.Guest.ebx = ebx;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestECX(PVMCPU pVCpu, uint32_t ecx)
{
    pVCpu->cpum.s.Guest.ecx = ecx;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestEDX(PVMCPU pVCpu, uint32_t edx)
{
    pVCpu->cpum.s.Guest.edx = edx;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestESP(PVMCPU pVCpu, uint32_t esp)
{
    pVCpu->cpum.s.Guest.esp = esp;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestEBP(PVMCPU pVCpu, uint32_t ebp)
{
    pVCpu->cpum.s.Guest.ebp = ebp;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestESI(PVMCPU pVCpu, uint32_t esi)
{
    pVCpu->cpum.s.Guest.esi = esi;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestEDI(PVMCPU pVCpu, uint32_t edi)
{
    pVCpu->cpum.s.Guest.edi = edi;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestSS(PVMCPU pVCpu, uint16_t ss)
{
    pVCpu->cpum.s.Guest.ss.Sel = ss;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestCS(PVMCPU pVCpu, uint16_t cs)
{
    pVCpu->cpum.s.Guest.cs.Sel = cs;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestDS(PVMCPU pVCpu, uint16_t ds)
{
    pVCpu->cpum.s.Guest.ds.Sel = ds;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestES(PVMCPU pVCpu, uint16_t es)
{
    pVCpu->cpum.s.Guest.es.Sel = es;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestFS(PVMCPU pVCpu, uint16_t fs)
{
    pVCpu->cpum.s.Guest.fs.Sel = fs;
    return VINF_SUCCESS;
}


VMMDECL(int) CPUMSetGuestGS(PVMCPU pVCpu, uint16_t gs)
{
    pVCpu->cpum.s.Guest.gs.Sel = gs;
    return VINF_SUCCESS;
}


VMMDECL(void) CPUMSetGuestEFER(PVMCPU pVCpu, uint64_t val)
{
    pVCpu->cpum.s.Guest.msrEFER = val;
}


VMMDECL(RTGCPTR) CPUMGetGuestIDTR(PVMCPU pVCpu, uint16_t *pcbLimit)
{
    if (pcbLimit)
        *pcbLimit = pVCpu->cpum.s.Guest.idtr.cbIdt;
    return pVCpu->cpum.s.Guest.idtr.pIdt;
}


VMMDECL(RTSEL) CPUMGetGuestTR(PVMCPU pVCpu, PCPUMSELREGHID pHidden)
{
    if (pHidden)
        *pHidden = pVCpu->cpum.s.Guest.tr;
    return pVCpu->cpum.s.Guest.tr.Sel;
}


VMMDECL(RTSEL) CPUMGetGuestCS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.cs.Sel;
}


VMMDECL(RTSEL) CPUMGetGuestDS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.ds.Sel;
}


VMMDECL(RTSEL) CPUMGetGuestES(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.es.Sel;
}


VMMDECL(RTSEL) CPUMGetGuestFS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.fs.Sel;
}


VMMDECL(RTSEL) CPUMGetGuestGS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.gs.Sel;
}


VMMDECL(RTSEL) CPUMGetGuestSS(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.ss.Sel;
}


VMMDECL(uint64_t)   CPUMGetGuestFlatPC(PVMCPU pVCpu)
{
    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, &pVCpu->cpum.s.Guest.cs);
    if (   !CPUMIsGuestInLongMode(pVCpu)
        || pVCpu->cpum.s.Guest.cs.Attr.n.u1Long)
        return pVCpu->cpum.s.Guest.eip + (uint32_t)pVCpu->cpum.s.Guest.cs.u64Base;
    return pVCpu->cpum.s.Guest.rip + pVCpu->cpum.s.Guest.cs.u64Base;
}


VMMDECL(uint64_t)   CPUMGetGuestFlatSP(PVMCPU pVCpu)
{
    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, &pVCpu->cpum.s.Guest.ss);
    if (   !CPUMIsGuestInLongMode(pVCpu)
        || pVCpu->cpum.s.Guest.ss.Attr.n.u1Long)
        return pVCpu->cpum.s.Guest.eip + (uint32_t)pVCpu->cpum.s.Guest.ss.u64Base;
    return pVCpu->cpum.s.Guest.rip + pVCpu->cpum.s.Guest.ss.u64Base;
}


VMMDECL(RTSEL) CPUMGetGuestLDTR(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.ldtr.Sel;
}


VMMDECL(RTSEL) CPUMGetGuestLdtrEx(PVMCPU pVCpu, uint64_t *pGCPtrBase, uint32_t *pcbLimit)
{
    *pGCPtrBase = pVCpu->cpum.s.Guest.ldtr.u64Base;
    *pcbLimit   = pVCpu->cpum.s.Guest.ldtr.u32Limit;
    return pVCpu->cpum.s.Guest.ldtr.Sel;
}


VMMDECL(uint64_t) CPUMGetGuestCR0(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.cr0;
}


VMMDECL(uint64_t) CPUMGetGuestCR2(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.cr2;
}


VMMDECL(uint64_t) CPUMGetGuestCR3(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.cr3;
}


VMMDECL(uint64_t) CPUMGetGuestCR4(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.cr4;
}


VMMDECL(uint64_t) CPUMGetGuestCR8(PVMCPU pVCpu)
{
    uint64_t u64;
    int rc = CPUMGetGuestCRx(pVCpu, DISCREG_CR8, &u64);
    if (RT_FAILURE(rc))
        u64 = 0;
    return u64;
}


VMMDECL(void) CPUMGetGuestGDTR(PVMCPU pVCpu, PVBOXGDTR pGDTR)
{
    *pGDTR = pVCpu->cpum.s.Guest.gdtr;
}


VMMDECL(uint32_t) CPUMGetGuestEIP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.eip;
}


VMMDECL(uint64_t) CPUMGetGuestRIP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.rip;
}


VMMDECL(uint32_t) CPUMGetGuestEAX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.eax;
}


VMMDECL(uint32_t) CPUMGetGuestEBX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.ebx;
}


VMMDECL(uint32_t) CPUMGetGuestECX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.ecx;
}


VMMDECL(uint32_t) CPUMGetGuestEDX(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.edx;
}


VMMDECL(uint32_t) CPUMGetGuestESI(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.esi;
}


VMMDECL(uint32_t) CPUMGetGuestEDI(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.edi;
}


VMMDECL(uint32_t) CPUMGetGuestESP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.esp;
}


VMMDECL(uint32_t) CPUMGetGuestEBP(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.ebp;
}


VMMDECL(uint32_t) CPUMGetGuestEFlags(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.eflags.u32;
}


VMMDECL(int) CPUMGetGuestCRx(PVMCPU pVCpu, unsigned iReg, uint64_t *pValue)
{
    switch (iReg)
    {
        case DISCREG_CR0:
            *pValue = pVCpu->cpum.s.Guest.cr0;
            break;

        case DISCREG_CR2:
            *pValue = pVCpu->cpum.s.Guest.cr2;
            break;

        case DISCREG_CR3:
            *pValue = pVCpu->cpum.s.Guest.cr3;
            break;

        case DISCREG_CR4:
            *pValue = pVCpu->cpum.s.Guest.cr4;
            break;

        case DISCREG_CR8:
        {
            uint8_t u8Tpr;
            int rc = APICGetTpr(pVCpu, &u8Tpr, NULL /* pfPending */, NULL /* pu8PendingIrq */);
            if (RT_FAILURE(rc))
            {
                AssertMsg(rc == VERR_PDM_NO_APIC_INSTANCE, ("%Rrc\n", rc));
                *pValue = 0;
                return rc;
            }
            *pValue = u8Tpr >> 4; /* bits 7-4 contain the task priority that go in cr8, bits 3-0 */
            break;
        }

        default:
            return VERR_INVALID_PARAMETER;
    }
    return VINF_SUCCESS;
}


VMMDECL(uint64_t) CPUMGetGuestDR0(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.dr[0];
}


VMMDECL(uint64_t) CPUMGetGuestDR1(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.dr[1];
}


VMMDECL(uint64_t) CPUMGetGuestDR2(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.dr[2];
}


VMMDECL(uint64_t) CPUMGetGuestDR3(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.dr[3];
}


VMMDECL(uint64_t) CPUMGetGuestDR6(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.dr[6];
}


VMMDECL(uint64_t) CPUMGetGuestDR7(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.dr[7];
}


VMMDECL(int) CPUMGetGuestDRx(PVMCPU pVCpu, uint32_t iReg, uint64_t *pValue)
{
    AssertReturn(iReg <= DISDREG_DR7, VERR_INVALID_PARAMETER);
    /* DR4 is an alias for DR6, and DR5 is an alias for DR7. */
    if (iReg == 4 || iReg == 5)
        iReg += 2;
    *pValue = pVCpu->cpum.s.Guest.dr[iReg];
    return VINF_SUCCESS;
}


VMMDECL(uint64_t) CPUMGetGuestEFER(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.Guest.msrEFER;
}


/**
 * Looks up a CPUID leaf in the CPUID leaf array, no subleaf.
 *
 * @returns Pointer to the leaf if found, NULL if not.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   uLeaf               The leaf to get.
 */
PCPUMCPUIDLEAF cpumCpuIdGetLeaf(PVM pVM, uint32_t uLeaf)
{
    unsigned            iEnd     = pVM->cpum.s.GuestInfo.cCpuIdLeaves;
    if (iEnd)
    {
        unsigned        iStart   = 0;
        PCPUMCPUIDLEAF  paLeaves = pVM->cpum.s.GuestInfo.CTX_SUFF(paCpuIdLeaves);
        for (;;)
        {
            unsigned i = iStart + (iEnd - iStart) / 2U;
            if (uLeaf < paLeaves[i].uLeaf)
            {
                if (i <= iStart)
                    return NULL;
                iEnd = i;
            }
            else if (uLeaf > paLeaves[i].uLeaf)
            {
                i += 1;
                if (i >= iEnd)
                    return NULL;
                iStart = i;
            }
            else
            {
                if (RT_LIKELY(paLeaves[i].fSubLeafMask == 0 && paLeaves[i].uSubLeaf == 0))
                    return &paLeaves[i];

                /* This shouldn't normally happen. But in case the it does due
                   to user configuration overrids or something, just return the
                   first sub-leaf. */
                AssertMsgFailed(("uLeaf=%#x fSubLeafMask=%#x uSubLeaf=%#x\n",
                                 uLeaf, paLeaves[i].fSubLeafMask, paLeaves[i].uSubLeaf));
                while (   paLeaves[i].uSubLeaf != 0
                       && i > 0
                       && uLeaf == paLeaves[i - 1].uLeaf)
                    i--;
                return &paLeaves[i];
            }
        }
    }

    return NULL;
}


/**
 * Looks up a CPUID leaf in the CPUID leaf array.
 *
 * @returns Pointer to the leaf if found, NULL if not.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   uLeaf               The leaf to get.
 * @param   uSubLeaf            The subleaf, if applicable.  Just pass 0 if it
 *                              isn't.
 * @param   pfExactSubLeafHit   Whether we've got an exact subleaf hit or not.
 */
PCPUMCPUIDLEAF cpumCpuIdGetLeafEx(PVM pVM, uint32_t uLeaf, uint32_t uSubLeaf, bool *pfExactSubLeafHit)
{
    unsigned            iEnd     = pVM->cpum.s.GuestInfo.cCpuIdLeaves;
    if (iEnd)
    {
        unsigned        iStart   = 0;
        PCPUMCPUIDLEAF  paLeaves = pVM->cpum.s.GuestInfo.CTX_SUFF(paCpuIdLeaves);
        for (;;)
        {
            unsigned i = iStart + (iEnd - iStart) / 2U;
            if (uLeaf < paLeaves[i].uLeaf)
            {
                if (i <= iStart)
                    return NULL;
                iEnd = i;
            }
            else if (uLeaf > paLeaves[i].uLeaf)
            {
                i += 1;
                if (i >= iEnd)
                    return NULL;
                iStart = i;
            }
            else
            {
                uSubLeaf &= paLeaves[i].fSubLeafMask;
                if (uSubLeaf == paLeaves[i].uSubLeaf)
                    *pfExactSubLeafHit = true;
                else
                {
                    /* Find the right subleaf.  We return the last one before
                       uSubLeaf if we don't find an exact match. */
                    if (uSubLeaf < paLeaves[i].uSubLeaf)
                        while (   i > 0
                               && uLeaf    == paLeaves[i - 1].uLeaf
                               && uSubLeaf <= paLeaves[i - 1].uSubLeaf)
                            i--;
                    else
                        while (   i + 1 < pVM->cpum.s.GuestInfo.cCpuIdLeaves
                               && uLeaf    == paLeaves[i + 1].uLeaf
                               && uSubLeaf >= paLeaves[i + 1].uSubLeaf)
                            i++;
                    *pfExactSubLeafHit = uSubLeaf == paLeaves[i].uSubLeaf;
                }
                return &paLeaves[i];
            }
        }
    }

    *pfExactSubLeafHit = false;
    return NULL;
}


/**
 * Gets a CPUID leaf.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   uLeaf       The CPUID leaf to get.
 * @param   uSubLeaf    The CPUID sub-leaf to get, if applicable.
 * @param   pEax        Where to store the EAX value.
 * @param   pEbx        Where to store the EBX value.
 * @param   pEcx        Where to store the ECX value.
 * @param   pEdx        Where to store the EDX value.
 */
VMMDECL(void) CPUMGetGuestCpuId(PVMCPU pVCpu, uint32_t uLeaf, uint32_t uSubLeaf,
                                uint32_t *pEax, uint32_t *pEbx, uint32_t *pEcx, uint32_t *pEdx)
{
    bool            fExactSubLeafHit;
    PVM             pVM   = pVCpu->CTX_SUFF(pVM);
    PCCPUMCPUIDLEAF pLeaf = cpumCpuIdGetLeafEx(pVM, uLeaf, uSubLeaf, &fExactSubLeafHit);
    if (pLeaf)
    {
        AssertMsg(pLeaf->uLeaf == uLeaf, ("%#x %#x\n", pLeaf->uLeaf, uLeaf));
        if (fExactSubLeafHit)
        {
            *pEax = pLeaf->uEax;
            *pEbx = pLeaf->uEbx;
            *pEcx = pLeaf->uEcx;
            *pEdx = pLeaf->uEdx;

            /*
             * Deal with CPU specific information.
             */
            if (pLeaf->fFlags & (  CPUMCPUIDLEAF_F_CONTAINS_APIC_ID
                                 | CPUMCPUIDLEAF_F_CONTAINS_OSXSAVE
                                 | CPUMCPUIDLEAF_F_CONTAINS_APIC ))
            {
                if (uLeaf == 1)
                {
                    /* EBX: Bits 31-24: Initial APIC ID. */
                    Assert(pVCpu->idCpu <= 255);
                    AssertMsg((pLeaf->uEbx >> 24) == 0, ("%#x\n", pLeaf->uEbx)); /* raw-mode assumption */
                    *pEbx = (pLeaf->uEbx & UINT32_C(0x00ffffff)) | (pVCpu->idCpu << 24);

                    /* EDX: Bit 9: AND with APICBASE.EN. */
                    if (!pVCpu->cpum.s.fCpuIdApicFeatureVisible && (pLeaf->fFlags & CPUMCPUIDLEAF_F_CONTAINS_APIC))
                        *pEdx &= ~X86_CPUID_FEATURE_EDX_APIC;

                    /* ECX: Bit 27: CR4.OSXSAVE mirror. */
                    *pEcx = (pLeaf->uEcx & ~X86_CPUID_FEATURE_ECX_OSXSAVE)
                          | (pVCpu->cpum.s.Guest.cr4 & X86_CR4_OSXSAVE ? X86_CPUID_FEATURE_ECX_OSXSAVE : 0);
                }
                else if (uLeaf == 0xb)
                {
                    /* EDX: Initial extended APIC ID. */
                    AssertMsg(pLeaf->uEdx == 0, ("%#x\n", pLeaf->uEdx)); /* raw-mode assumption */
                    *pEdx = pVCpu->idCpu;
                    Assert(!(pLeaf->fFlags & ~(CPUMCPUIDLEAF_F_CONTAINS_APIC_ID | CPUMCPUIDLEAF_F_INTEL_TOPOLOGY_SUBLEAVES)));
                }
                else if (uLeaf == UINT32_C(0x8000001e))
                {
                    /* EAX: Initial extended APIC ID. */
                    AssertMsg(pLeaf->uEax == 0, ("%#x\n", pLeaf->uEax)); /* raw-mode assumption */
                    *pEax = pVCpu->idCpu;
                    Assert(!(pLeaf->fFlags & ~CPUMCPUIDLEAF_F_CONTAINS_APIC_ID));
                }
                else if (uLeaf == UINT32_C(0x80000001))
                {
                    /* EDX: Bit 9: AND with APICBASE.EN. */
                    if (!pVCpu->cpum.s.fCpuIdApicFeatureVisible)
                        *pEdx &= ~X86_CPUID_AMD_FEATURE_EDX_APIC;
                    Assert(!(pLeaf->fFlags & ~CPUMCPUIDLEAF_F_CONTAINS_APIC));
                }
                else
                    AssertMsgFailed(("uLeaf=%#x\n", uLeaf));
            }
        }
        /*
         * Out of range sub-leaves aren't quite as easy and pretty as we emulate
         * them here, but we do the best we can here...
         */
        else
        {
            *pEax = *pEbx = *pEcx = *pEdx = 0;
            if (pLeaf->fFlags & CPUMCPUIDLEAF_F_INTEL_TOPOLOGY_SUBLEAVES)
            {
                *pEcx = uSubLeaf & 0xff;
                *pEdx = pVCpu->idCpu;
            }
        }
    }
    else
    {
        /*
         * Different CPUs have different ways of dealing with unknown CPUID leaves.
         */
        switch (pVM->cpum.s.GuestInfo.enmUnknownCpuIdMethod)
        {
            default:
                AssertFailed();
                RT_FALL_THRU();
            case CPUMUNKNOWNCPUID_DEFAULTS:
            case CPUMUNKNOWNCPUID_LAST_STD_LEAF: /* ASSUME this is executed */
            case CPUMUNKNOWNCPUID_LAST_STD_LEAF_WITH_ECX: /** @todo Implement CPUMUNKNOWNCPUID_LAST_STD_LEAF_WITH_ECX */
                *pEax = pVM->cpum.s.GuestInfo.DefCpuId.uEax;
                *pEbx = pVM->cpum.s.GuestInfo.DefCpuId.uEbx;
                *pEcx = pVM->cpum.s.GuestInfo.DefCpuId.uEcx;
                *pEdx = pVM->cpum.s.GuestInfo.DefCpuId.uEdx;
                break;
            case CPUMUNKNOWNCPUID_PASSTHRU:
                *pEax = uLeaf;
                *pEbx = 0;
                *pEcx = uSubLeaf;
                *pEdx = 0;
                break;
        }
    }
    Log2(("CPUMGetGuestCpuId: uLeaf=%#010x/%#010x %RX32 %RX32 %RX32 %RX32\n", uLeaf, uSubLeaf, *pEax, *pEbx, *pEcx, *pEdx));
}


/**
 * Sets the visibility of the X86_CPUID_FEATURE_EDX_APIC and
 * X86_CPUID_AMD_FEATURE_EDX_APIC CPUID bits.
 *
 * @returns Previous value.
 * @param   pVCpu       The cross context virtual CPU structure to make the
 *                      change on.  Usually the calling EMT.
 * @param   fVisible    Whether to make it visible (true) or hide it (false).
 *
 * @remarks This is "VMMDECL" so that it still links with
 *          the old APIC code which is in VBoxDD2 and not in
 *          the VMM module.
 */
VMMDECL(bool) CPUMSetGuestCpuIdPerCpuApicFeature(PVMCPU pVCpu, bool fVisible)
{
    bool fOld = pVCpu->cpum.s.fCpuIdApicFeatureVisible;
    pVCpu->cpum.s.fCpuIdApicFeatureVisible = fVisible;

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    /*
     * Patch manager saved state legacy pain.
     */
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    PCPUMCPUIDLEAF pLeaf = cpumCpuIdGetLeaf(pVM, UINT32_C(0x00000001));
    if (pLeaf)
    {
        if (fVisible || (pLeaf->fFlags & CPUMCPUIDLEAF_F_CONTAINS_APIC))
            pVM->cpum.s.aGuestCpuIdPatmStd[1].uEdx = pLeaf->uEdx;
        else
            pVM->cpum.s.aGuestCpuIdPatmStd[1].uEdx = pLeaf->uEdx & ~X86_CPUID_FEATURE_EDX_APIC;
    }

    pLeaf = cpumCpuIdGetLeaf(pVM, UINT32_C(0x80000001));
    if (pLeaf)
    {
        if (fVisible || (pLeaf->fFlags & CPUMCPUIDLEAF_F_CONTAINS_APIC))
            pVM->cpum.s.aGuestCpuIdPatmExt[1].uEdx = pLeaf->uEdx;
        else
            pVM->cpum.s.aGuestCpuIdPatmExt[1].uEdx = pLeaf->uEdx & ~X86_CPUID_AMD_FEATURE_EDX_APIC;
    }
#endif

    return fOld;
}


/**
 * Gets the host CPU vendor.
 *
 * @returns CPU vendor.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(CPUMCPUVENDOR) CPUMGetHostCpuVendor(PVM pVM)
{
    return (CPUMCPUVENDOR)pVM->cpum.s.HostFeatures.enmCpuVendor;
}


/**
 * Gets the CPU vendor.
 *
 * @returns CPU vendor.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(CPUMCPUVENDOR) CPUMGetGuestCpuVendor(PVM pVM)
{
    return (CPUMCPUVENDOR)pVM->cpum.s.GuestFeatures.enmCpuVendor;
}


VMMDECL(int) CPUMSetGuestDR0(PVMCPU pVCpu, uint64_t uDr0)
{
    pVCpu->cpum.s.Guest.dr[0] = uDr0;
    return CPUMRecalcHyperDRx(pVCpu, 0, false);
}


VMMDECL(int) CPUMSetGuestDR1(PVMCPU pVCpu, uint64_t uDr1)
{
    pVCpu->cpum.s.Guest.dr[1] = uDr1;
    return CPUMRecalcHyperDRx(pVCpu, 1, false);
}


VMMDECL(int) CPUMSetGuestDR2(PVMCPU pVCpu, uint64_t uDr2)
{
    pVCpu->cpum.s.Guest.dr[2] = uDr2;
    return CPUMRecalcHyperDRx(pVCpu, 2, false);
}


VMMDECL(int) CPUMSetGuestDR3(PVMCPU pVCpu, uint64_t uDr3)
{
    pVCpu->cpum.s.Guest.dr[3] = uDr3;
    return CPUMRecalcHyperDRx(pVCpu, 3, false);
}


VMMDECL(int) CPUMSetGuestDR6(PVMCPU pVCpu, uint64_t uDr6)
{
    pVCpu->cpum.s.Guest.dr[6] = uDr6;
    return VINF_SUCCESS; /* No need to recalc. */
}


VMMDECL(int) CPUMSetGuestDR7(PVMCPU pVCpu, uint64_t uDr7)
{
    pVCpu->cpum.s.Guest.dr[7] = uDr7;
    return CPUMRecalcHyperDRx(pVCpu, 7, false);
}


VMMDECL(int) CPUMSetGuestDRx(PVMCPU pVCpu, uint32_t iReg, uint64_t Value)
{
    AssertReturn(iReg <= DISDREG_DR7, VERR_INVALID_PARAMETER);
    /* DR4 is an alias for DR6, and DR5 is an alias for DR7. */
    if (iReg == 4 || iReg == 5)
        iReg += 2;
    pVCpu->cpum.s.Guest.dr[iReg] = Value;
    return CPUMRecalcHyperDRx(pVCpu, iReg, false);
}


/**
 * Recalculates the hypervisor DRx register values based on current guest
 * registers and DBGF breakpoints, updating changed registers depending on the
 * context.
 *
 * This is called whenever a guest DRx register is modified (any context) and
 * when DBGF sets a hardware breakpoint (ring-3 only, rendezvous).
 *
 * In raw-mode context this function will reload any (hyper) DRx registers which
 * comes out with a different value.  It may also have to save the host debug
 * registers if that haven't been done already.  In this context though, we'll
 * be intercepting and emulating all DRx accesses, so the hypervisor DRx values
 * are only important when breakpoints are actually enabled.
 *
 * In ring-0 (HM) context DR0-3 will be relocated by us, while DR7 will be
 * reloaded by the HM code if it changes.  Further more, we will only use the
 * combined register set when the VBox debugger is actually using hardware BPs,
 * when it isn't we'll keep the guest DR0-3 + (maybe) DR6 loaded (DR6 doesn't
 * concern us here).
 *
 * In ring-3 we won't be loading anything, so well calculate hypervisor values
 * all the time.
 *
 * @returns VINF_SUCCESS.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   iGstReg     The guest debug register number that was modified.
 *                      UINT8_MAX if not guest register.
 * @param   fForceHyper Used in HM to force hyper registers because of single
 *                      stepping.
 */
VMMDECL(int) CPUMRecalcHyperDRx(PVMCPU pVCpu, uint8_t iGstReg, bool fForceHyper)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);
#ifndef IN_RING0
    RT_NOREF_PV(iGstReg);
#endif

    /*
     * Compare the DR7s first.
     *
     * We only care about the enabled flags.  GD is virtualized when we
     * dispatch the #DB, we never enable it.  The DBGF DR7 value is will
     * always have the LE and GE bits set, so no need to check and disable
     * stuff if they're cleared like we have to for the guest DR7.
     */
    RTGCUINTREG uGstDr7 = CPUMGetGuestDR7(pVCpu);
    if (!(uGstDr7 & (X86_DR7_LE | X86_DR7_GE)))
        uGstDr7 = 0;
    else if (!(uGstDr7 & X86_DR7_LE))
        uGstDr7 &= ~X86_DR7_LE_ALL;
    else if (!(uGstDr7 & X86_DR7_GE))
        uGstDr7 &= ~X86_DR7_GE_ALL;

    const RTGCUINTREG uDbgfDr7 = DBGFBpGetDR7(pVM);

#ifdef IN_RING0
    if (!fForceHyper && (pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_HYPER))
        fForceHyper = true;
#endif
    if (( HMIsEnabled(pVCpu->CTX_SUFF(pVM)) && !fForceHyper ? uDbgfDr7 : (uGstDr7 | uDbgfDr7)) & X86_DR7_ENABLED_MASK)
    {
        Assert(!CPUMIsGuestDebugStateActive(pVCpu));
#ifdef IN_RC
        bool const fHmEnabled = false;
#elif defined(IN_RING3)
        bool const fHmEnabled = HMIsEnabled(pVM);
#endif

        /*
         * Ok, something is enabled.  Recalc each of the breakpoints, taking
         * the VM debugger ones of the guest ones.  In raw-mode context we will
         * not allow breakpoints with values inside the hypervisor area.
         */
        RTGCUINTREG uNewDr7 = X86_DR7_GE | X86_DR7_LE | X86_DR7_RA1_MASK;

        /* bp 0 */
        RTGCUINTREG uNewDr0;
        if (uDbgfDr7 & (X86_DR7_L0 | X86_DR7_G0))
        {
            uNewDr7 |= uDbgfDr7 & (X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW0_MASK | X86_DR7_LEN0_MASK);
            uNewDr0 = DBGFBpGetDR0(pVM);
        }
        else if (uGstDr7 & (X86_DR7_L0 | X86_DR7_G0))
        {
            uNewDr0 = CPUMGetGuestDR0(pVCpu);
#ifndef IN_RING0
            if (fHmEnabled && MMHyperIsInsideArea(pVM, uNewDr0))
                uNewDr0 = 0;
            else
#endif
                uNewDr7 |= uGstDr7 & (X86_DR7_L0 | X86_DR7_G0 | X86_DR7_RW0_MASK | X86_DR7_LEN0_MASK);
        }
        else
            uNewDr0 = 0;

        /* bp 1 */
        RTGCUINTREG uNewDr1;
        if (uDbgfDr7 & (X86_DR7_L1 | X86_DR7_G1))
        {
            uNewDr7 |= uDbgfDr7 & (X86_DR7_L1 | X86_DR7_G1 | X86_DR7_RW1_MASK | X86_DR7_LEN1_MASK);
            uNewDr1 = DBGFBpGetDR1(pVM);
        }
        else if (uGstDr7 & (X86_DR7_L1 | X86_DR7_G1))
        {
            uNewDr1 = CPUMGetGuestDR1(pVCpu);
#ifndef IN_RING0
            if (fHmEnabled && MMHyperIsInsideArea(pVM, uNewDr1))
                uNewDr1 = 0;
            else
#endif
                uNewDr7 |= uGstDr7 & (X86_DR7_L1 | X86_DR7_G1 | X86_DR7_RW1_MASK | X86_DR7_LEN1_MASK);
        }
        else
            uNewDr1 = 0;

        /* bp 2 */
        RTGCUINTREG uNewDr2;
        if (uDbgfDr7 & (X86_DR7_L2 | X86_DR7_G2))
        {
            uNewDr7 |= uDbgfDr7 & (X86_DR7_L2 | X86_DR7_G2 | X86_DR7_RW2_MASK | X86_DR7_LEN2_MASK);
            uNewDr2 = DBGFBpGetDR2(pVM);
        }
        else if (uGstDr7 & (X86_DR7_L2 | X86_DR7_G2))
        {
            uNewDr2 = CPUMGetGuestDR2(pVCpu);
#ifndef IN_RING0
            if (fHmEnabled && MMHyperIsInsideArea(pVM, uNewDr2))
                uNewDr2 = 0;
            else
#endif
                uNewDr7 |= uGstDr7 & (X86_DR7_L2 | X86_DR7_G2 | X86_DR7_RW2_MASK | X86_DR7_LEN2_MASK);
        }
        else
            uNewDr2 = 0;

        /* bp 3 */
        RTGCUINTREG uNewDr3;
        if (uDbgfDr7 & (X86_DR7_L3 | X86_DR7_G3))
        {
            uNewDr7 |= uDbgfDr7 & (X86_DR7_L3 | X86_DR7_G3 | X86_DR7_RW3_MASK | X86_DR7_LEN3_MASK);
            uNewDr3 = DBGFBpGetDR3(pVM);
        }
        else if (uGstDr7 & (X86_DR7_L3 | X86_DR7_G3))
        {
            uNewDr3 = CPUMGetGuestDR3(pVCpu);
#ifndef IN_RING0
            if (fHmEnabled && MMHyperIsInsideArea(pVM, uNewDr3))
                uNewDr3 = 0;
            else
#endif
                uNewDr7 |= uGstDr7 & (X86_DR7_L3 | X86_DR7_G3 | X86_DR7_RW3_MASK | X86_DR7_LEN3_MASK);
        }
        else
            uNewDr3 = 0;

        /*
         * Apply the updates.
         */
#ifdef IN_RC
        /* Make sure to save host registers first. */
        if (!(pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_HOST))
        {
            if (!(pVCpu->cpum.s.fUseFlags & CPUM_USE_DEBUG_REGS_HOST))
            {
                pVCpu->cpum.s.Host.dr6 = ASMGetDR6();
                pVCpu->cpum.s.Host.dr7 = ASMGetDR7();
            }
            pVCpu->cpum.s.Host.dr0 = ASMGetDR0();
            pVCpu->cpum.s.Host.dr1 = ASMGetDR1();
            pVCpu->cpum.s.Host.dr2 = ASMGetDR2();
            pVCpu->cpum.s.Host.dr3 = ASMGetDR3();
            pVCpu->cpum.s.fUseFlags |= CPUM_USED_DEBUG_REGS_HOST | CPUM_USE_DEBUG_REGS_HYPER | CPUM_USED_DEBUG_REGS_HYPER;

            /* We haven't loaded any hyper DRxes yet, so we'll have to load them all now. */
            pVCpu->cpum.s.Hyper.dr[0] = uNewDr0;
            ASMSetDR0(uNewDr0);
            pVCpu->cpum.s.Hyper.dr[1] = uNewDr1;
            ASMSetDR1(uNewDr1);
            pVCpu->cpum.s.Hyper.dr[2] = uNewDr2;
            ASMSetDR2(uNewDr2);
            pVCpu->cpum.s.Hyper.dr[3] = uNewDr3;
            ASMSetDR3(uNewDr3);
            ASMSetDR6(X86_DR6_INIT_VAL);
            pVCpu->cpum.s.Hyper.dr[7] = uNewDr7;
            ASMSetDR7(uNewDr7);
        }
        else
#endif
        {
            pVCpu->cpum.s.fUseFlags |= CPUM_USE_DEBUG_REGS_HYPER;
            if (uNewDr3 != pVCpu->cpum.s.Hyper.dr[3])
                CPUMSetHyperDR3(pVCpu, uNewDr3);
            if (uNewDr2 != pVCpu->cpum.s.Hyper.dr[2])
                CPUMSetHyperDR2(pVCpu, uNewDr2);
            if (uNewDr1 != pVCpu->cpum.s.Hyper.dr[1])
                CPUMSetHyperDR1(pVCpu, uNewDr1);
            if (uNewDr0 != pVCpu->cpum.s.Hyper.dr[0])
                CPUMSetHyperDR0(pVCpu, uNewDr0);
            if (uNewDr7 != pVCpu->cpum.s.Hyper.dr[7])
                CPUMSetHyperDR7(pVCpu, uNewDr7);
        }
    }
#ifdef IN_RING0
    else if (CPUMIsGuestDebugStateActive(pVCpu))
    {
        /*
         * Reload the register that was modified.  Normally this won't happen
         * as we won't intercept DRx writes when not having the hyper debug
         * state loaded, but in case we do for some reason we'll simply deal
         * with it.
         */
        switch (iGstReg)
        {
            case 0: ASMSetDR0(CPUMGetGuestDR0(pVCpu)); break;
            case 1: ASMSetDR1(CPUMGetGuestDR1(pVCpu)); break;
            case 2: ASMSetDR2(CPUMGetGuestDR2(pVCpu)); break;
            case 3: ASMSetDR3(CPUMGetGuestDR3(pVCpu)); break;
            default:
                AssertReturn(iGstReg != UINT8_MAX, VERR_INTERNAL_ERROR_3);
        }
    }
#endif
    else
    {
        /*
         * No active debug state any more.  In raw-mode this means we have to
         * make sure DR7 has everything disabled now, if we armed it already.
         * In ring-0 we might end up here when just single stepping.
         */
#if defined(IN_RC) || defined(IN_RING0)
        if (pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_HYPER)
        {
# ifdef IN_RC
            ASMSetDR7(X86_DR7_INIT_VAL);
# endif
            if (pVCpu->cpum.s.Hyper.dr[0])
                ASMSetDR0(0);
            if (pVCpu->cpum.s.Hyper.dr[1])
                ASMSetDR1(0);
            if (pVCpu->cpum.s.Hyper.dr[2])
                ASMSetDR2(0);
            if (pVCpu->cpum.s.Hyper.dr[3])
                ASMSetDR3(0);
            pVCpu->cpum.s.fUseFlags &= ~CPUM_USED_DEBUG_REGS_HYPER;
        }
#endif
        pVCpu->cpum.s.fUseFlags &= ~CPUM_USE_DEBUG_REGS_HYPER;

        /* Clear all the registers. */
        pVCpu->cpum.s.Hyper.dr[7] = X86_DR7_RA1_MASK;
        pVCpu->cpum.s.Hyper.dr[3] = 0;
        pVCpu->cpum.s.Hyper.dr[2] = 0;
        pVCpu->cpum.s.Hyper.dr[1] = 0;
        pVCpu->cpum.s.Hyper.dr[0] = 0;

    }
    Log2(("CPUMRecalcHyperDRx: fUseFlags=%#x %RGr %RGr %RGr %RGr  %RGr %RGr\n",
          pVCpu->cpum.s.fUseFlags, pVCpu->cpum.s.Hyper.dr[0], pVCpu->cpum.s.Hyper.dr[1],
          pVCpu->cpum.s.Hyper.dr[2], pVCpu->cpum.s.Hyper.dr[3], pVCpu->cpum.s.Hyper.dr[6],
          pVCpu->cpum.s.Hyper.dr[7]));

    return VINF_SUCCESS;
}


/**
 * Set the guest XCR0 register.
 *
 * Will load additional state if the FPU state is already loaded (in ring-0 &
 * raw-mode context).
 *
 * @returns VINF_SUCCESS on success, VERR_CPUM_RAISE_GP_0 on invalid input
 *          value.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   uNewValue   The new value.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(int)   CPUMSetGuestXcr0(PVMCPU pVCpu, uint64_t uNewValue)
{
    if (   (uNewValue & ~pVCpu->CTX_SUFF(pVM)->cpum.s.fXStateGuestMask) == 0
        /* The X87 bit cannot be cleared. */
        && (uNewValue & XSAVE_C_X87)
        /* AVX requires SSE. */
        && (uNewValue & (XSAVE_C_SSE | XSAVE_C_YMM)) != XSAVE_C_YMM
        /* AVX-512 requires YMM, SSE and all of its three components to be enabled. */
        && (   (uNewValue & (XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI)) == 0
            ||    (uNewValue & (XSAVE_C_SSE | XSAVE_C_YMM | XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI))
               ==              (XSAVE_C_SSE | XSAVE_C_YMM | XSAVE_C_OPMASK | XSAVE_C_ZMM_HI256 | XSAVE_C_ZMM_16HI) )
       )
    {
        pVCpu->cpum.s.Guest.aXcr[0] = uNewValue;

        /* If more state components are enabled, we need to take care to load
           them if the FPU/SSE state is already loaded.  May otherwise leak
           host state to the guest. */
        uint64_t fNewComponents = ~pVCpu->cpum.s.Guest.fXStateMask & uNewValue;
        if (fNewComponents)
        {
#if defined(IN_RING0) || defined(IN_RC)
            if (pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_GUEST)
            {
                if (pVCpu->cpum.s.Guest.fXStateMask != 0)
                    /* Adding more components. */
                    ASMXRstor(pVCpu->cpum.s.Guest.CTX_SUFF(pXState), fNewComponents);
                else
                {
                    /* We're switching from FXSAVE/FXRSTOR to XSAVE/XRSTOR. */
                    pVCpu->cpum.s.Guest.fXStateMask |= XSAVE_C_X87 | XSAVE_C_SSE;
                    if (uNewValue & ~(XSAVE_C_X87 | XSAVE_C_SSE))
                        ASMXRstor(pVCpu->cpum.s.Guest.CTX_SUFF(pXState), uNewValue & ~(XSAVE_C_X87 | XSAVE_C_SSE));
                }
            }
#endif
            pVCpu->cpum.s.Guest.fXStateMask |= uNewValue;
        }
        return VINF_SUCCESS;
    }
    return VERR_CPUM_RAISE_GP_0;
}


/**
 * Tests if the guest has No-Execute Page Protection Enabled (NXE).
 *
 * @returns true if in real mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestNXEnabled(PVMCPU pVCpu)
{
    return !!(pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_NXE);
}


/**
 * Tests if the guest has the Page Size Extension enabled (PSE).
 *
 * @returns true if in real mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestPageSizeExtEnabled(PVMCPU pVCpu)
{
    /* PAE or AMD64 implies support for big pages regardless of CR4.PSE */
    return !!(pVCpu->cpum.s.Guest.cr4 & (X86_CR4_PSE | X86_CR4_PAE));
}


/**
 * Tests if the guest has the paging enabled (PG).
 *
 * @returns true if in real mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestPagingEnabled(PVMCPU pVCpu)
{
    return !!(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PG);
}


/**
 * Tests if the guest has the paging enabled (PG).
 *
 * @returns true if in real mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestR0WriteProtEnabled(PVMCPU pVCpu)
{
    return !!(pVCpu->cpum.s.Guest.cr0 & X86_CR0_WP);
}


/**
 * Tests if the guest is running in real mode or not.
 *
 * @returns true if in real mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestInRealMode(PVMCPU pVCpu)
{
    return !(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE);
}


/**
 * Tests if the guest is running in real or virtual 8086 mode.
 *
 * @returns @c true if it is, @c false if not.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestInRealOrV86Mode(PVMCPU pVCpu)
{
    return !(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE)
        || pVCpu->cpum.s.Guest.eflags.Bits.u1VM; /** @todo verify that this cannot be set in long mode. */
}


/**
 * Tests if the guest is running in protected or not.
 *
 * @returns true if in protected mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestInProtectedMode(PVMCPU pVCpu)
{
    return !!(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE);
}


/**
 * Tests if the guest is running in paged protected or not.
 *
 * @returns true if in paged protected mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestInPagedProtectedMode(PVMCPU pVCpu)
{
    return (pVCpu->cpum.s.Guest.cr0 & (X86_CR0_PE | X86_CR0_PG)) == (X86_CR0_PE | X86_CR0_PG);
}


/**
 * Tests if the guest is running in long mode or not.
 *
 * @returns true if in long mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestInLongMode(PVMCPU pVCpu)
{
    return (pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_LMA) == MSR_K6_EFER_LMA;
}


/**
 * Tests if the guest is running in PAE mode or not.
 *
 * @returns true if in PAE mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestInPAEMode(PVMCPU pVCpu)
{
    /* Intel mentions EFER.LMA and EFER.LME in different parts of their spec. We shall use EFER.LMA rather
       than EFER.LME as it reflects if the CPU has entered paging with EFER.LME set.  */
    return (pVCpu->cpum.s.Guest.cr4 & X86_CR4_PAE)
        && (pVCpu->cpum.s.Guest.cr0 & X86_CR0_PG)
        && !(pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_LMA);
}


/**
 * Tests if the guest is running in 64 bits mode or not.
 *
 * @returns true if in 64 bits protected mode, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(bool) CPUMIsGuestIn64BitCode(PVMCPU pVCpu)
{
    if (!CPUMIsGuestInLongMode(pVCpu))
        return false;
    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, &pVCpu->cpum.s.Guest.cs);
    return pVCpu->cpum.s.Guest.cs.Attr.n.u1Long;
}


/**
 * Helper for CPUMIsGuestIn64BitCodeEx that handles lazy resolving of hidden CS
 * registers.
 *
 * @returns true if in 64 bits protected mode, otherwise false.
 * @param   pCtx        Pointer to the current guest CPU context.
 */
VMM_INT_DECL(bool) CPUMIsGuestIn64BitCodeSlow(PCPUMCTX pCtx)
{
    return CPUMIsGuestIn64BitCode(CPUM_GUEST_CTX_TO_VMCPU(pCtx));
}

#ifdef VBOX_WITH_RAW_MODE_NOT_R0

/**
 *
 * @returns @c true if we've entered raw-mode and selectors with RPL=1 are
 *          really RPL=0, @c false if we've not (RPL=1 really is RPL=1).
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMM_INT_DECL(bool) CPUMIsGuestInRawMode(PVMCPU pVCpu)
{
    return pVCpu->cpum.s.fRawEntered;
}

/**
 * Transforms the guest CPU state to raw-ring mode.
 *
 * This function will change the any of the cs and ss register with DPL=0 to DPL=1.
 *
 * @returns VBox status code. (recompiler failure)
 * @param   pVCpu       The cross context virtual CPU structure.
 * @see     @ref pg_raw
 */
VMM_INT_DECL(int) CPUMRawEnter(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    Assert(!pVCpu->cpum.s.fRawEntered);
    Assert(!pVCpu->cpum.s.fRemEntered);
    PCPUMCTX pCtx = &pVCpu->cpum.s.Guest;

    /*
     * Are we in Ring-0?
     */
    if (    pCtx->ss.Sel
        &&  (pCtx->ss.Sel & X86_SEL_RPL) == 0
        &&  !pCtx->eflags.Bits.u1VM)
    {
        /*
         * Enter execution mode.
         */
        PATMRawEnter(pVM, pCtx);

        /*
         * Set CPL to Ring-1.
         */
        pCtx->ss.Sel |= 1;
        if (    pCtx->cs.Sel
            &&  (pCtx->cs.Sel & X86_SEL_RPL) == 0)
            pCtx->cs.Sel |= 1;
    }
    else
    {
# ifdef VBOX_WITH_RAW_RING1
        if (    EMIsRawRing1Enabled(pVM)
            &&  !pCtx->eflags.Bits.u1VM
            &&  (pCtx->ss.Sel & X86_SEL_RPL) == 1)
        {
            /* Set CPL to Ring-2. */
            pCtx->ss.Sel = (pCtx->ss.Sel & ~X86_SEL_RPL) | 2;
            if (pCtx->cs.Sel && (pCtx->cs.Sel & X86_SEL_RPL) == 1)
                pCtx->cs.Sel = (pCtx->cs.Sel & ~X86_SEL_RPL) | 2;
        }
# else
        AssertMsg((pCtx->ss.Sel & X86_SEL_RPL) >= 2 || pCtx->eflags.Bits.u1VM,
                  ("ring-1 code not supported\n"));
# endif
        /*
         * PATM takes care of IOPL and IF flags for Ring-3 and Ring-2 code as well.
         */
        PATMRawEnter(pVM, pCtx);
    }

    /*
     * Assert sanity.
     */
    AssertMsg((pCtx->eflags.u32 & X86_EFL_IF), ("X86_EFL_IF is clear\n"));
    AssertReleaseMsg(pCtx->eflags.Bits.u2IOPL == 0,
                     ("X86_EFL_IOPL=%d CPL=%d\n", pCtx->eflags.Bits.u2IOPL, pCtx->ss.Sel & X86_SEL_RPL));
    Assert((pVCpu->cpum.s.Guest.cr0 & (X86_CR0_PG | X86_CR0_PE)) == (X86_CR0_PG | X86_CR0_PE));

    pCtx->eflags.u32        |= X86_EFL_IF; /* paranoia */

    pVCpu->cpum.s.fRawEntered = true;
    return VINF_SUCCESS;
}


/**
 * Transforms the guest CPU state from raw-ring mode to correct values.
 *
 * This function will change any selector registers with DPL=1 to DPL=0.
 *
 * @returns Adjusted rc.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   rc          Raw mode return code
 * @see     @ref pg_raw
 */
VMM_INT_DECL(int) CPUMRawLeave(PVMCPU pVCpu, int rc)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    /*
     * Don't leave if we've already left (in RC).
     */
    Assert(!pVCpu->cpum.s.fRemEntered);
    if (!pVCpu->cpum.s.fRawEntered)
        return rc;
    pVCpu->cpum.s.fRawEntered = false;

    PCPUMCTX pCtx = &pVCpu->cpum.s.Guest;
    Assert(pCtx->eflags.Bits.u1VM || (pCtx->ss.Sel & X86_SEL_RPL));
    AssertMsg(pCtx->eflags.Bits.u1VM || pCtx->eflags.Bits.u2IOPL < (unsigned)(pCtx->ss.Sel & X86_SEL_RPL),
              ("X86_EFL_IOPL=%d CPL=%d\n", pCtx->eflags.Bits.u2IOPL, pCtx->ss.Sel & X86_SEL_RPL));

    /*
     * Are we executing in raw ring-1?
     */
    if (    (pCtx->ss.Sel & X86_SEL_RPL) == 1
        &&  !pCtx->eflags.Bits.u1VM)
    {
        /*
         * Leave execution mode.
         */
        PATMRawLeave(pVM, pCtx, rc);
        /* Not quite sure if this is really required, but shouldn't harm (too much anyways). */
        /** @todo See what happens if we remove this. */
        if ((pCtx->ds.Sel & X86_SEL_RPL) == 1)
            pCtx->ds.Sel &= ~X86_SEL_RPL;
        if ((pCtx->es.Sel & X86_SEL_RPL) == 1)
            pCtx->es.Sel &= ~X86_SEL_RPL;
        if ((pCtx->fs.Sel & X86_SEL_RPL) == 1)
            pCtx->fs.Sel &= ~X86_SEL_RPL;
        if ((pCtx->gs.Sel & X86_SEL_RPL) == 1)
            pCtx->gs.Sel &= ~X86_SEL_RPL;

        /*
         * Ring-1 selector => Ring-0.
         */
        pCtx->ss.Sel &= ~X86_SEL_RPL;
        if ((pCtx->cs.Sel & X86_SEL_RPL) == 1)
            pCtx->cs.Sel &= ~X86_SEL_RPL;
    }
    else
    {
        /*
         * PATM is taking care of the IOPL and IF flags for us.
         */
        PATMRawLeave(pVM, pCtx, rc);
        if (!pCtx->eflags.Bits.u1VM)
        {
# ifdef VBOX_WITH_RAW_RING1
            if (    EMIsRawRing1Enabled(pVM)
                &&  (pCtx->ss.Sel & X86_SEL_RPL) == 2)
            {
                /* Not quite sure if this is really required, but shouldn't harm (too much anyways). */
                /** @todo See what happens if we remove this. */
                if ((pCtx->ds.Sel & X86_SEL_RPL) == 2)
                    pCtx->ds.Sel = (pCtx->ds.Sel & ~X86_SEL_RPL) | 1;
                if ((pCtx->es.Sel & X86_SEL_RPL) == 2)
                    pCtx->es.Sel = (pCtx->es.Sel & ~X86_SEL_RPL) | 1;
                if ((pCtx->fs.Sel & X86_SEL_RPL) == 2)
                    pCtx->fs.Sel = (pCtx->fs.Sel & ~X86_SEL_RPL) | 1;
                if ((pCtx->gs.Sel & X86_SEL_RPL) == 2)
                    pCtx->gs.Sel = (pCtx->gs.Sel & ~X86_SEL_RPL) | 1;

                /*
                 * Ring-2 selector => Ring-1.
                 */
                pCtx->ss.Sel = (pCtx->ss.Sel & ~X86_SEL_RPL) | 1;
                if ((pCtx->cs.Sel & X86_SEL_RPL) == 2)
                    pCtx->cs.Sel = (pCtx->cs.Sel & ~X86_SEL_RPL) | 1;
            }
            else
            {
# endif
                /** @todo See what happens if we remove this. */
                if ((pCtx->ds.Sel & X86_SEL_RPL) == 1)
                    pCtx->ds.Sel &= ~X86_SEL_RPL;
                if ((pCtx->es.Sel & X86_SEL_RPL) == 1)
                    pCtx->es.Sel &= ~X86_SEL_RPL;
                if ((pCtx->fs.Sel & X86_SEL_RPL) == 1)
                    pCtx->fs.Sel &= ~X86_SEL_RPL;
                if ((pCtx->gs.Sel & X86_SEL_RPL) == 1)
                    pCtx->gs.Sel &= ~X86_SEL_RPL;
# ifdef VBOX_WITH_RAW_RING1
            }
# endif
        }
    }

    return rc;
}

#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */

/**
 * Updates the EFLAGS while we're in raw-mode.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   fEfl        The new EFLAGS value.
 */
VMMDECL(void) CPUMRawSetEFlags(PVMCPU pVCpu, uint32_t fEfl)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (pVCpu->cpum.s.fRawEntered)
        PATMRawSetEFlags(pVCpu->CTX_SUFF(pVM), &pVCpu->cpum.s.Guest, fEfl);
    else
#endif
        pVCpu->cpum.s.Guest.eflags.u32 = fEfl;
}


/**
 * Gets the EFLAGS while we're in raw-mode.
 *
 * @returns The eflags.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(uint32_t) CPUMRawGetEFlags(PVMCPU pVCpu)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (pVCpu->cpum.s.fRawEntered)
        return PATMRawGetEFlags(pVCpu->CTX_SUFF(pVM), &pVCpu->cpum.s.Guest);
#endif
    return pVCpu->cpum.s.Guest.eflags.u32;
}


/**
 * Sets the specified changed flags (CPUM_CHANGED_*).
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   fChangedAdd The changed flags to add.
 */
VMMDECL(void) CPUMSetChangedFlags(PVMCPU pVCpu, uint32_t fChangedAdd)
{
    pVCpu->cpum.s.fChanged |= fChangedAdd;
}


/**
 * Checks if the CPU supports the XSAVE and XRSTOR instruction.
 *
 * @returns true if supported.
 * @returns false if not supported.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(bool) CPUMSupportsXSave(PVM pVM)
{
    return pVM->cpum.s.HostFeatures.fXSaveRstor != 0;
}


/**
 * Checks if the host OS uses the SYSENTER / SYSEXIT instructions.
 * @returns true if used.
 * @returns false if not used.
 * @param   pVM       The cross context VM structure.
 */
VMMDECL(bool) CPUMIsHostUsingSysEnter(PVM pVM)
{
    return RT_BOOL(pVM->cpum.s.fHostUseFlags & CPUM_USE_SYSENTER);
}


/**
 * Checks if the host OS uses the SYSCALL / SYSRET instructions.
 * @returns true if used.
 * @returns false if not used.
 * @param   pVM       The cross context VM structure.
 */
VMMDECL(bool) CPUMIsHostUsingSysCall(PVM pVM)
{
    return RT_BOOL(pVM->cpum.s.fHostUseFlags & CPUM_USE_SYSCALL);
}

#ifdef IN_RC

/**
 * Lazily sync in the FPU/XMM state.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(int) CPUMHandleLazyFPU(PVMCPU pVCpu)
{
    return cpumHandleLazyFPUAsm(&pVCpu->cpum.s);
}

#endif /* !IN_RC */

/**
 * Checks if we activated the FPU/XMM state of the guest OS.
 *
 * This differs from CPUMIsGuestFPUStateLoaded() in that it refers to the next
 * time we'll be executing guest code, so it may return true for 64-on-32 when
 * we still haven't actually loaded the FPU status, just scheduled it to be
 * loaded the next time we go thru the world switcher (CPUM_SYNC_FPU_STATE).
 *
 * @returns true / false.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestFPUStateActive(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->cpum.s.fUseFlags & (CPUM_USED_FPU_GUEST | CPUM_SYNC_FPU_STATE));
}


/**
 * Checks if we've really loaded the FPU/XMM state of the guest OS.
 *
 * @returns true / false.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsGuestFPUStateLoaded(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_GUEST);
}


/**
 * Checks if we saved the FPU/XMM state of the host OS.
 *
 * @returns true / false.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
VMMDECL(bool) CPUMIsHostFPUStateSaved(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_HOST);
}


/**
 * Checks if the guest debug state is active.
 *
 * @returns boolean
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(bool) CPUMIsGuestDebugStateActive(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_GUEST);
}


/**
 * Checks if the guest debug state is to be made active during the world-switch
 * (currently only used for the 32->64 switcher case).
 *
 * @returns boolean
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(bool) CPUMIsGuestDebugStateActivePending(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->cpum.s.fUseFlags & CPUM_SYNC_DEBUG_REGS_GUEST);
}


/**
 * Checks if the hyper debug state is active.
 *
 * @returns boolean
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(bool) CPUMIsHyperDebugStateActive(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_HYPER);
}


/**
 * Checks if the hyper debug state is to be made active during the world-switch
 * (currently only used for the 32->64 switcher case).
 *
 * @returns boolean
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(bool) CPUMIsHyperDebugStateActivePending(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->cpum.s.fUseFlags & CPUM_SYNC_DEBUG_REGS_HYPER);
}


/**
 * Mark the guest's debug state as inactive.
 *
 * @returns boolean
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @todo    This API doesn't make sense any more.
 */
VMMDECL(void) CPUMDeactivateGuestDebugState(PVMCPU pVCpu)
{
    Assert(!(pVCpu->cpum.s.fUseFlags & (CPUM_USED_DEBUG_REGS_GUEST | CPUM_USED_DEBUG_REGS_HYPER | CPUM_USED_DEBUG_REGS_HOST)));
    NOREF(pVCpu);
}


/**
 * Get the current privilege level of the guest.
 *
 * @returns CPL
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(uint32_t) CPUMGetGuestCPL(PVMCPU pVCpu)
{
    /*
     * CPL can reliably be found in SS.DPL (hidden regs valid) or SS if not.
     *
     * Note! We used to check CS.DPL here, assuming it was always equal to
     * CPL even if a conforming segment was loaded.  But this turned out to
     * only apply to older AMD-V.  With VT-x we had an ACP2 regression
     * during install after a far call to ring 2 with VT-x.  Then on newer
     * AMD-V CPUs we have to move the VMCB.guest.u8CPL into cs.Attr.n.u2Dpl
     * as well as ss.Attr.n.u2Dpl to make this (and other) code work right.
     *
     * So, forget CS.DPL, always use SS.DPL.
     *
     * Note! The SS RPL is always equal to the CPL, while the CS RPL
     * isn't necessarily equal if the segment is conforming.
     * See section 4.11.1 in the AMD manual.
     *
     * Update: Where the heck does it say CS.RPL can differ from CPL other than
     *         right after real->prot mode switch and when in V8086 mode?  That
     *         section says the RPL specified in a direct transfere (call, jmp,
     *         ret) is not the one loaded into CS. Besides, if CS.RPL != CPL
     *         it would be impossible for an exception handle or the iret
     *         instruction to figure out whether SS:ESP are part of the frame
     *         or not.  VBox or qemu bug must've lead to this misconception.
     *
     * Update2: On an AMD bulldozer system here, I've no trouble loading a null
     *         selector into SS with an RPL other than the CPL when CPL != 3 and
     *         we're in 64-bit mode.  The intel dev box doesn't allow this, on
     *         RPL = CPL.  Weird.
     */
    uint32_t uCpl;
    if (pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE)
    {
        if (!pVCpu->cpum.s.Guest.eflags.Bits.u1VM)
        {
            if (CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pVCpu->cpum.s.Guest.ss))
                uCpl = pVCpu->cpum.s.Guest.ss.Attr.n.u2Dpl;
            else
            {
                uCpl = (pVCpu->cpum.s.Guest.ss.Sel & X86_SEL_RPL);
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
# ifdef VBOX_WITH_RAW_RING1
                if (pVCpu->cpum.s.fRawEntered)
                {
                    if (   uCpl == 2
                        && EMIsRawRing1Enabled(pVCpu->CTX_SUFF(pVM)))
                        uCpl = 1;
                    else if (uCpl == 1)
                        uCpl = 0;
                }
                Assert(uCpl != 2);  /* ring 2 support not allowed anymore. */
# else
                if (uCpl == 1)
                    uCpl = 0;
# endif
#endif
            }
        }
        else
            uCpl = 3; /* V86 has CPL=3; REM doesn't set DPL=3 in V8086 mode. See @bugref{5130}. */
    }
    else
        uCpl = 0;     /* Real mode is zero; CPL set to 3 for VT-x real-mode emulation. */
    return uCpl;
}


/**
 * Gets the current guest CPU mode.
 *
 * If paging mode is what you need, check out PGMGetGuestMode().
 *
 * @returns The CPU mode.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMDECL(CPUMMODE) CPUMGetGuestMode(PVMCPU pVCpu)
{
    CPUMMODE enmMode;
    if (!(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE))
        enmMode = CPUMMODE_REAL;
    else if (!(pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_LMA))
        enmMode = CPUMMODE_PROTECTED;
    else
        enmMode = CPUMMODE_LONG;

    return enmMode;
}


/**
 * Figure whether the CPU is currently executing 16, 32 or 64 bit code.
 *
 * @returns 16, 32 or 64.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(uint32_t)       CPUMGetGuestCodeBits(PVMCPU pVCpu)
{
    if (!(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE))
        return 16;

    if (pVCpu->cpum.s.Guest.eflags.Bits.u1VM)
    {
        Assert(!(pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_LMA));
        return 16;
    }

    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, &pVCpu->cpum.s.Guest.cs);
    if (   pVCpu->cpum.s.Guest.cs.Attr.n.u1Long
        && (pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_LMA))
        return 64;

    if (pVCpu->cpum.s.Guest.cs.Attr.n.u1DefBig)
        return 32;

    return 16;
}


VMMDECL(DISCPUMODE)     CPUMGetGuestDisMode(PVMCPU pVCpu)
{
    if (!(pVCpu->cpum.s.Guest.cr0 & X86_CR0_PE))
        return DISCPUMODE_16BIT;

    if (pVCpu->cpum.s.Guest.eflags.Bits.u1VM)
    {
        Assert(!(pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_LMA));
        return DISCPUMODE_16BIT;
    }

    CPUMSELREG_LAZY_LOAD_HIDDEN_PARTS(pVCpu, &pVCpu->cpum.s.Guest.cs);
    if (   pVCpu->cpum.s.Guest.cs.Attr.n.u1Long
        && (pVCpu->cpum.s.Guest.msrEFER & MSR_K6_EFER_LMA))
        return DISCPUMODE_64BIT;

    if (pVCpu->cpum.s.Guest.cs.Attr.n.u1DefBig)
        return DISCPUMODE_32BIT;

    return DISCPUMODE_16BIT;
}


/**
 * Gets the guest MXCSR_MASK value.
 *
 * This does not access the x87 state, but the value we determined at VM
 * initialization.
 *
 * @returns MXCSR mask.
 * @param   pVM                 The cross context VM structure.
 */
VMMDECL(uint32_t) CPUMGetGuestMxCsrMask(PVM pVM)
{
    return pVM->cpum.s.GuestInfo.fMxCsrMask;
}


/**
 * Checks whether the SVM nested-guest is in a state to receive physical (APIC)
 * interrupts.
 *
 * @returns VBox status code.
 * @retval  true if it's ready, false otherwise.
 *
 * @param   pCtx        The guest-CPU context.
 */
VMM_INT_DECL(bool) CPUMCanSvmNstGstTakePhysIntr(PCCPUMCTX pCtx)
{
#ifdef IN_RC
    RT_NOREF(pCtx);
    AssertReleaseFailedReturn(false);
#else
    Assert(CPUMIsGuestInSvmNestedHwVirtMode(pCtx));
    Assert(pCtx->hwvirt.svm.fGif);
    Assert(!pCtx->hwvirt.svm.fHMCachedVmcb);

    PCSVMVMCBCTRL pVmcbCtrl = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
    X86EFLAGS fEFlags;
    if (pVmcbCtrl->IntCtrl.n.u1VIntrMasking)
        fEFlags.u = pCtx->hwvirt.svm.HostState.rflags.u;
    else
        fEFlags.u = pCtx->eflags.u;

    return fEFlags.Bits.u1IF;
#endif
}


/**
 * Checks whether the SVM nested-guest is in a state to receive virtual
 * (injected by VMRUN) interrupts.
 *
 * @returns VBox status code.
 * @retval  true if it's ready, false otherwise.
 *
 * @param   pCtx        The guest-CPU context.
 */
VMM_INT_DECL(bool) CPUMCanSvmNstGstTakeVirtIntr(PCCPUMCTX pCtx)
{
#ifdef IN_RC
    RT_NOREF(pCtx);
    AssertReleaseFailedReturn(false);
#else
    Assert(CPUMIsGuestInSvmNestedHwVirtMode(pCtx));

    PCSVMVMCBCTRL pVmcbCtrl = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
    if (   !pVmcbCtrl->IntCtrl.n.u1IgnoreTPR
        &&  pVmcbCtrl->IntCtrl.n.u4VIntrPrio <= pVmcbCtrl->IntCtrl.n.u8VTPR)
        return false;

    if (!pCtx->rflags.Bits.u1IF)
        return false;

    if (!pCtx->hwvirt.svm.fGif)
        return false;

    return true;
#endif
}


/**
 * Gets the pending SVM nested-guest interrupt.
 *
 * @returns The nested-guest interrupt to inject.
 * @param   pCtx            The guest-CPU context.
 */
VMM_INT_DECL(uint8_t) CPUMGetSvmNstGstInterrupt(PCCPUMCTX pCtx)
{
#ifdef IN_RC
    RT_NOREF(pCtx);
    AssertReleaseFailedReturn(0);
#else
    PCSVMVMCBCTRL pVmcbCtrl = &pCtx->hwvirt.svm.CTX_SUFF(pVmcb)->ctrl;
    return pVmcbCtrl->IntCtrl.n.u8VIntrVector;
#endif
}


/**
 * Restores the host-state from the host-state save area as part of a \#VMEXIT.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtx        The guest-CPU context.
 */
VMM_INT_DECL(void) CPUMSvmVmExitRestoreHostState(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * Reload the guest's "host state".
     */
    PSVMHOSTSTATE pHostState = &pCtx->hwvirt.svm.HostState;
    pCtx->es         = pHostState->es;
    pCtx->cs         = pHostState->cs;
    pCtx->ss         = pHostState->ss;
    pCtx->ds         = pHostState->ds;
    pCtx->gdtr       = pHostState->gdtr;
    pCtx->idtr       = pHostState->idtr;
    CPUMSetGuestMsrEferNoCheck(pVCpu, pCtx->msrEFER, pHostState->uEferMsr);
    CPUMSetGuestCR0(pVCpu, pHostState->uCr0 | X86_CR0_PE);
    pCtx->cr3        = pHostState->uCr3;
    CPUMSetGuestCR4(pVCpu, pHostState->uCr4);
    pCtx->rflags     = pHostState->rflags;
    pCtx->rflags.Bits.u1VM = 0;
    pCtx->rip        = pHostState->uRip;
    pCtx->rsp        = pHostState->uRsp;
    pCtx->rax        = pHostState->uRax;
    pCtx->dr[7]     &= ~(X86_DR7_ENABLED_MASK | X86_DR7_RAZ_MASK | X86_DR7_MBZ_MASK);
    pCtx->dr[7]     |= X86_DR7_RA1_MASK;
    Assert(pCtx->ss.Attr.n.u2Dpl == 0);

    /** @todo if RIP is not canonical or outside the CS segment limit, we need to
     *        raise \#GP(0) in the guest. */

    /** @todo check the loaded host-state for consistency. Figure out what
     *        exactly this involves? */
}


/**
 * Saves the host-state to the host-state save area as part of a VMRUN.
 *
 * @param   pCtx        The guest-CPU context.
 * @param   cbInstr     The length of the VMRUN instruction in bytes.
 */
VMM_INT_DECL(void) CPUMSvmVmRunSaveHostState(PCPUMCTX pCtx, uint8_t cbInstr)
{
    PSVMHOSTSTATE pHostState = &pCtx->hwvirt.svm.HostState;
    pHostState->es       = pCtx->es;
    pHostState->cs       = pCtx->cs;
    pHostState->ss       = pCtx->ss;
    pHostState->ds       = pCtx->ds;
    pHostState->gdtr     = pCtx->gdtr;
    pHostState->idtr     = pCtx->idtr;
    pHostState->uEferMsr = pCtx->msrEFER;
    pHostState->uCr0     = pCtx->cr0;
    pHostState->uCr3     = pCtx->cr3;
    pHostState->uCr4     = pCtx->cr4;
    pHostState->rflags   = pCtx->rflags;
    pHostState->uRip     = pCtx->rip + cbInstr;
    pHostState->uRsp     = pCtx->rsp;
    pHostState->uRax     = pCtx->rax;
}


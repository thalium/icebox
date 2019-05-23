/* $Id: GIMAllKvm.cpp $ */
/** @file
 * GIM - Guest Interface Manager, KVM, All Contexts.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_GIM
#include <VBox/vmm/gim.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmapi.h>
#include "GIMKvmInternal.h"
#include "GIMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/dis.h>
#include <VBox/err.h>
#include <VBox/sup.h>

#include <iprt/asm-amd64-x86.h>
#include <iprt/time.h>


/**
 * Handles the KVM hypercall.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_SUCCESS if the hypercall succeeded (even if its operation
 *          failed).
 * @retval  VINF_GIM_R3_HYPERCALL re-start the hypercall from ring-3.
 * @retval  VERR_GIM_HYPERCALL_ACCESS_DENIED CPL is insufficient.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 *
 * @thread  EMT(pVCpu).
 */
VMM_INT_DECL(VBOXSTRICTRC) gimKvmHypercall(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    VMCPU_ASSERT_EMT(pVCpu);

    PVM pVM = pVCpu->CTX_SUFF(pVM);
    STAM_REL_COUNTER_INC(&pVM->gim.s.StatHypercalls);

    /*
     * Get the hypercall operation and arguments.
     */
    bool const fIs64BitMode = CPUMIsGuestIn64BitCodeEx(pCtx);
    uint64_t uHyperOp       = pCtx->rax;
    uint64_t uHyperArg0     = pCtx->rbx;
    uint64_t uHyperArg1     = pCtx->rcx;
    uint64_t uHyperArg2     = pCtx->rdi;
    uint64_t uHyperArg3     = pCtx->rsi;
    uint64_t uHyperRet      = KVM_HYPERCALL_RET_ENOSYS;
    uint64_t uAndMask       = UINT64_C(0xffffffffffffffff);
    if (!fIs64BitMode)
    {
        uAndMask    = UINT64_C(0xffffffff);
        uHyperOp   &= UINT64_C(0xffffffff);
        uHyperArg0 &= UINT64_C(0xffffffff);
        uHyperArg1 &= UINT64_C(0xffffffff);
        uHyperArg2 &= UINT64_C(0xffffffff);
        uHyperArg3 &= UINT64_C(0xffffffff);
        uHyperRet  &= UINT64_C(0xffffffff);
    }

    /*
     * Verify that guest ring-0 is the one making the hypercall.
     */
    uint32_t uCpl = CPUMGetGuestCPL(pVCpu);
    if (RT_UNLIKELY(uCpl))
    {
        pCtx->rax = KVM_HYPERCALL_RET_EPERM & uAndMask;
        return VERR_GIM_HYPERCALL_ACCESS_DENIED;
    }

    /*
     * Do the work.
     */
    int rc = VINF_SUCCESS;
    switch (uHyperOp)
    {
        case KVM_HYPERCALL_OP_KICK_CPU:
        {
            if (uHyperArg1 < pVM->cCpus)
            {
                PVMCPU pVCpuDst = &pVM->aCpus[uHyperArg1];   /* ASSUMES pVCpu index == ApicId of the VCPU. */
                EMUnhaltAndWakeUp(pVM, pVCpuDst);
                uHyperRet = KVM_HYPERCALL_RET_SUCCESS;
            }
            else
            {
                /* Shouldn't ever happen! If it does, throw a guru, as otherwise it'll lead to deadlocks in the guest anyway! */
                rc = VERR_GIM_HYPERCALL_FAILED;
            }
            break;
        }

        case KVM_HYPERCALL_OP_VAPIC_POLL_IRQ:
            uHyperRet = KVM_HYPERCALL_RET_SUCCESS;
            break;

        default:
            break;
    }

    /*
     * Place the result in rax/eax.
     */
    pCtx->rax = uHyperRet & uAndMask;
    return rc;
}


/**
 * Returns whether the guest has configured and enabled the use of KVM's
 * hypercall interface.
 *
 * @returns true if hypercalls are enabled, false otherwise.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(bool) gimKvmAreHypercallsEnabled(PVMCPU pVCpu)
{
    NOREF(pVCpu);
    /* KVM paravirt interface doesn't have hypercall control bits (like Hyper-V does)
       that guests can control, i.e. hypercalls are always enabled. */
    return true;
}


/**
 * Returns whether the guest has configured and enabled the use of KVM's
 * paravirtualized TSC.
 *
 * @returns true if paravirt. TSC is enabled, false otherwise.
 * @param   pVM     The cross context VM structure.
 */
VMM_INT_DECL(bool) gimKvmIsParavirtTscEnabled(PVM pVM)
{
    uint32_t cCpus = pVM->cCpus;
    for (uint32_t i = 0; i < cCpus; i++)
    {
        PVMCPU     pVCpu      = &pVM->aCpus[i];
        PGIMKVMCPU pGimKvmCpu = &pVCpu->gim.s.u.KvmCpu;
        if (MSR_GIM_KVM_SYSTEM_TIME_IS_ENABLED(pGimKvmCpu->u64SystemTimeMsr))
            return true;
    }
    return false;
}


/**
 * MSR read handler for KVM.
 *
 * @returns Strict VBox status code like CPUMQueryGuestMsr().
 * @retval  VINF_CPUM_R3_MSR_READ
 * @retval  VERR_CPUM_RAISE_GP_0
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   idMsr       The MSR being read.
 * @param   pRange      The range this MSR belongs to.
 * @param   puValue     Where to store the MSR value read.
 */
VMM_INT_DECL(VBOXSTRICTRC) gimKvmReadMsr(PVMCPU pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t *puValue)
{
    NOREF(pRange);
    PVM     pVM        = pVCpu->CTX_SUFF(pVM);
    PGIMKVM pKvm       = &pVM->gim.s.u.Kvm;
    PGIMKVMCPU pKvmCpu = &pVCpu->gim.s.u.KvmCpu;

    switch (idMsr)
    {
        case MSR_GIM_KVM_SYSTEM_TIME:
        case MSR_GIM_KVM_SYSTEM_TIME_OLD:
            *puValue = pKvmCpu->u64SystemTimeMsr;
            return VINF_SUCCESS;

        case MSR_GIM_KVM_WALL_CLOCK:
        case MSR_GIM_KVM_WALL_CLOCK_OLD:
            *puValue = pKvm->u64WallClockMsr;
            return VINF_SUCCESS;

        default:
        {
#ifdef IN_RING3
            static uint32_t s_cTimes = 0;
            if (s_cTimes++ < 20)
                LogRel(("GIM: KVM: Unknown/invalid RdMsr (%#x) -> #GP(0)\n", idMsr));
#endif
            LogFunc(("Unknown/invalid RdMsr (%#RX32) -> #GP(0)\n", idMsr));
            break;
        }
    }

    return VERR_CPUM_RAISE_GP_0;
}


/**
 * MSR write handler for KVM.
 *
 * @returns Strict VBox status code like CPUMSetGuestMsr().
 * @retval  VINF_CPUM_R3_MSR_WRITE
 * @retval  VERR_CPUM_RAISE_GP_0
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   idMsr       The MSR being written.
 * @param   pRange      The range this MSR belongs to.
 * @param   uRawValue   The raw value with the ignored bits not masked.
 */
VMM_INT_DECL(VBOXSTRICTRC) gimKvmWriteMsr(PVMCPU pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t uRawValue)
{
    NOREF(pRange);
    PVM        pVM  = pVCpu->CTX_SUFF(pVM);
    PGIMKVMCPU pKvmCpu = &pVCpu->gim.s.u.KvmCpu;

    switch (idMsr)
    {
        case MSR_GIM_KVM_SYSTEM_TIME:
        case MSR_GIM_KVM_SYSTEM_TIME_OLD:
        {
            bool fEnable = RT_BOOL(uRawValue & MSR_GIM_KVM_SYSTEM_TIME_ENABLE_BIT);
#ifdef IN_RING0
            NOREF(fEnable); NOREF(pKvmCpu);
            gimR0KvmUpdateSystemTime(pVM, pVCpu);
            return VINF_CPUM_R3_MSR_WRITE;
#elif defined(IN_RC)
            Assert(pVM->cCpus == 1);
            if (fEnable)
            {
                RTCCUINTREG fEFlags  = ASMIntDisableFlags();
                pKvmCpu->uTsc        = TMCpuTickGetNoCheck(pVCpu) | UINT64_C(1);
                pKvmCpu->uVirtNanoTS = TMVirtualGetNoCheck(pVM)   | UINT64_C(1);
                ASMSetFlags(fEFlags);
            }
            return VINF_CPUM_R3_MSR_WRITE;
#else /* IN_RING3 */
            if (!fEnable)
            {
                gimR3KvmDisableSystemTime(pVM);
                pKvmCpu->u64SystemTimeMsr = uRawValue;
                return VINF_SUCCESS;
            }

            /* Is the system-time struct. already enabled? If so, get flags that need preserving. */
            GIMKVMSYSTEMTIME SystemTime;
            RT_ZERO(SystemTime);
            if (   MSR_GIM_KVM_SYSTEM_TIME_IS_ENABLED(pKvmCpu->u64SystemTimeMsr)
                && MSR_GIM_KVM_SYSTEM_TIME_GUEST_GPA(uRawValue) == pKvmCpu->GCPhysSystemTime)
            {
                int rc2 = PGMPhysSimpleReadGCPhys(pVM, &SystemTime, pKvmCpu->GCPhysSystemTime, sizeof(GIMKVMSYSTEMTIME));
                if (RT_SUCCESS(rc2))
                    pKvmCpu->fSystemTimeFlags = (SystemTime.fFlags & GIM_KVM_SYSTEM_TIME_FLAGS_GUEST_PAUSED);
            }

            /* Enable and populate the system-time struct. */
            pKvmCpu->u64SystemTimeMsr      = uRawValue;
            pKvmCpu->GCPhysSystemTime      = MSR_GIM_KVM_SYSTEM_TIME_GUEST_GPA(uRawValue);
            pKvmCpu->u32SystemTimeVersion += 2;
            int rc = gimR3KvmEnableSystemTime(pVM, pVCpu);
            if (RT_FAILURE(rc))
            {
                pKvmCpu->u64SystemTimeMsr = 0;
                /* We shouldn't throw a #GP(0) here for buggy guests (neither does KVM apparently), see @bugref{8627}. */
            }
            return VINF_SUCCESS;
#endif
        }

        case MSR_GIM_KVM_WALL_CLOCK:
        case MSR_GIM_KVM_WALL_CLOCK_OLD:
        {
#ifndef IN_RING3
            return VINF_CPUM_R3_MSR_WRITE;
#else
            /* Enable the wall-clock struct. */
            RTGCPHYS GCPhysWallClock = MSR_GIM_KVM_WALL_CLOCK_GUEST_GPA(uRawValue);
            if (RT_LIKELY(RT_ALIGN_64(GCPhysWallClock, 4) == GCPhysWallClock))
            {
                int rc = gimR3KvmEnableWallClock(pVM, GCPhysWallClock);
                if (RT_SUCCESS(rc))
                {
                    PGIMKVM pKvm = &pVM->gim.s.u.Kvm;
                    pKvm->u64WallClockMsr = uRawValue;
                    return VINF_SUCCESS;
                }
            }
            return VERR_CPUM_RAISE_GP_0;
#endif /* IN_RING3 */
        }

        default:
        {
#ifdef IN_RING3
            static uint32_t s_cTimes = 0;
            if (s_cTimes++ < 20)
                LogRel(("GIM: KVM: Unknown/invalid WrMsr (%#x,%#x`%08x) -> #GP(0)\n", idMsr,
                        uRawValue & UINT64_C(0xffffffff00000000), uRawValue & UINT64_C(0xffffffff)));
#endif
            LogFunc(("Unknown/invalid WrMsr (%#RX32,%#RX64) -> #GP(0)\n", idMsr, uRawValue));
            break;
        }
    }

    return VERR_CPUM_RAISE_GP_0;
}


/**
 * Whether we need to trap \#UD exceptions in the guest.
 *
 * On AMD-V we need to trap them because paravirtualized Linux/KVM guests use
 * the Intel VMCALL instruction to make hypercalls and we need to trap and
 * optionally patch them to the AMD-V VMMCALL instruction and handle the
 * hypercall.
 *
 * I guess this was done so that guest teleporation between an AMD and an Intel
 * machine would working without any changes at the time of teleporation.
 * However, this also means we -always- need to intercept \#UD exceptions on one
 * of the two CPU models (Intel or AMD). Hyper-V solves this problem more
 * elegantly by letting the hypervisor supply an opaque hypercall page.
 *
 * For raw-mode VMs, this function will always return true. See gimR3KvmInit().
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(bool) gimKvmShouldTrapXcptUD(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    return pVM->gim.s.u.Kvm.fTrapXcptUD;
}


/**
 * Checks the currently disassembled instruction and executes the hypercall if
 * it's a hypercall instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pDis        Pointer to the disassembled instruction state at RIP.
 *
 * @thread  EMT(pVCpu).
 *
 * @todo    Make this function static when @bugref{7270#c168} is addressed.
 */
VMM_INT_DECL(VBOXSTRICTRC) gimKvmExecHypercallInstr(PVMCPU pVCpu, PCPUMCTX pCtx, PDISCPUSTATE pDis)
{
    Assert(pVCpu);
    Assert(pCtx);
    Assert(pDis);
    VMCPU_ASSERT_EMT(pVCpu);

    /*
     * If the instruction at RIP is the Intel VMCALL instruction or
     * the AMD VMMCALL instruction handle it as a hypercall.
     *
     * Linux/KVM guests always uses the Intel VMCALL instruction but we patch
     * it to the host-native one whenever we encounter it so subsequent calls
     * will not require disassembly (when coming from HM).
     */
    if (   pDis->pCurInstr->uOpcode == OP_VMCALL
        || pDis->pCurInstr->uOpcode == OP_VMMCALL)
    {
        /*
         * Perform the hypercall.
         *
         * For HM, we can simply resume guest execution without performing the hypercall now and
         * do it on the next VMCALL/VMMCALL exit handler on the patched instruction.
         *
         * For raw-mode we need to do this now anyway. So we do it here regardless with an added
         * advantage is that it saves one world-switch for the HM case.
         */
        VBOXSTRICTRC rcStrict = gimKvmHypercall(pVCpu, pCtx);
        if (rcStrict == VINF_SUCCESS)
        {
            /*
             * Patch the instruction to so we don't have to spend time disassembling it each time.
             * Makes sense only for HM as with raw-mode we will be getting a #UD regardless.
             */
            PVM      pVM  = pVCpu->CTX_SUFF(pVM);
            PCGIMKVM pKvm = &pVM->gim.s.u.Kvm;
            if (   pDis->pCurInstr->uOpcode != pKvm->uOpCodeNative
                && HMIsEnabled(pVM))
            {
                /** @todo r=ramshankar: we probably should be doing this in an
                 *        EMT rendezvous. */
                uint8_t abHypercall[3];
                size_t  cbWritten = 0;
                int rc = VMMPatchHypercall(pVM, &abHypercall, sizeof(abHypercall), &cbWritten);
                AssertRC(rc);
                Assert(sizeof(abHypercall) == pDis->cbInstr);
                Assert(sizeof(abHypercall) == cbWritten);

                rc = PGMPhysSimpleWriteGCPtr(pVCpu, pCtx->rip, &abHypercall, sizeof(abHypercall));
                AssertRC(rc);

                /** @todo Add stats for patching. */
            }
        }
        else
        {
            /* The KVM provider doesn't have any concept of continuing hypercalls. */
            Assert(rcStrict != VINF_GIM_HYPERCALL_CONTINUING);
#ifdef IN_RING3
            Assert(rcStrict != VINF_GIM_R3_HYPERCALL);
#endif
        }
        return rcStrict;
    }

    return VERR_GIM_INVALID_HYPERCALL_INSTR;
}


/**
 * Exception handler for \#UD.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_SUCCESS if the hypercall succeeded (even if its operation
 *          failed).
 * @retval  VINF_GIM_R3_HYPERCALL re-start the hypercall from ring-3.
 * @retval  VERR_GIM_HYPERCALL_ACCESS_DENIED CPL is insufficient.
 * @retval  VERR_GIM_INVALID_HYPERCALL_INSTR instruction at RIP is not a valid
 *          hypercall instruction.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pDis        Pointer to the disassembled instruction state at RIP.
 *                      Optional, can be NULL.
 * @param   pcbInstr    Where to store the instruction length of the hypercall
 *                      instruction. Optional, can be NULL.
 *
 * @thread  EMT(pVCpu).
 */
VMM_INT_DECL(VBOXSTRICTRC) gimKvmXcptUD(PVMCPU pVCpu, PCPUMCTX pCtx, PDISCPUSTATE pDis, uint8_t *pcbInstr)
{
    VMCPU_ASSERT_EMT(pVCpu);

    /*
     * If we didn't ask for #UD to be trapped, bail.
     */
    PVM      pVM  = pVCpu->CTX_SUFF(pVM);
    PCGIMKVM pKvm = &pVM->gim.s.u.Kvm;
    if (RT_UNLIKELY(!pKvm->fTrapXcptUD))
        return VERR_GIM_IPE_3;

    if (!pDis)
    {
        unsigned    cbInstr;
        DISCPUSTATE Dis;
        int rc = EMInterpretDisasCurrent(pVM, pVCpu, &Dis, &cbInstr);
        if (RT_SUCCESS(rc))
        {
            if (pcbInstr)
                *pcbInstr = (uint8_t)cbInstr;
            return gimKvmExecHypercallInstr(pVCpu, pCtx, &Dis);
        }

        Log(("GIM: KVM: Failed to disassemble instruction at CS:RIP=%04x:%08RX64. rc=%Rrc\n", pCtx->cs.Sel, pCtx->rip, rc));
        return rc;
    }

    return gimKvmExecHypercallInstr(pVCpu, pCtx, pDis);
}


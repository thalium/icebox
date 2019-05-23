/* $Id: EMRaw.cpp $ */
/** @file
 * EM - Execution Monitor / Manager - software virtualization
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
#define LOG_GROUP LOG_GROUP_EM
#include <VBox/vmm/em.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/pgm.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/tm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/patm.h>
#include "EMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/gim.h>
#include <VBox/vmm/cpumdis.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <VBox/vmm/dbgf.h>
#include "VMMTracing.h"

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/stream.h>



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int      emR3RawForcedActions(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx);
DECLINLINE(int) emR3RawExecuteInstruction(PVM pVM, PVMCPU pVCpu, const char *pszPrefix, int rcGC = VINF_SUCCESS);
static int      emR3RawGuestTrap(PVM pVM, PVMCPU pVCpu);
static int      emR3RawPatchTrap(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int gcret);
static int      emR3RawPrivileged(PVM pVM, PVMCPU pVCpu);
static int      emR3RawExecuteIOInstruction(PVM pVM, PVMCPU pVCpu);
static int      emR3RawRingSwitch(PVM pVM, PVMCPU pVCpu);

#define EMHANDLERC_WITH_PATM
#define emR3ExecuteInstruction   emR3RawExecuteInstruction
#define emR3ExecuteIOInstruction emR3RawExecuteIOInstruction
#include "EMHandleRCTmpl.h"



#ifdef VBOX_WITH_STATISTICS
/**
 * Just a braindead function to keep track of cli addresses.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtrInstr  The EIP of the cli instruction.
 */
static void emR3RecordCli(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtrInstr)
{
    PCLISTAT pRec;

    pRec = (PCLISTAT)RTAvlGCPtrGet(&pVCpu->em.s.pCliStatTree, GCPtrInstr);
    if (!pRec)
    {
        /* New cli instruction; insert into the tree. */
        pRec = (PCLISTAT)MMR3HeapAllocZ(pVM, MM_TAG_EM, sizeof(*pRec));
        Assert(pRec);
        if (!pRec)
            return;
        pRec->Core.Key = GCPtrInstr;

        char szCliStatName[32];
        RTStrPrintf(szCliStatName, sizeof(szCliStatName), "/EM/Cli/0x%RGv", GCPtrInstr);
        STAM_REG(pVM, &pRec->Counter, STAMTYPE_COUNTER, szCliStatName, STAMUNIT_OCCURENCES, "Number of times cli was executed.");

        bool fRc = RTAvlGCPtrInsert(&pVCpu->em.s.pCliStatTree, &pRec->Core);
        Assert(fRc); NOREF(fRc);
    }
    STAM_COUNTER_INC(&pRec->Counter);
    STAM_COUNTER_INC(&pVCpu->em.s.StatTotalClis);
}
#endif /* VBOX_WITH_STATISTICS */



/**
 * Resumes executing hypervisor after a debug event.
 *
 * This is kind of special since our current guest state is
 * potentially out of sync.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
int emR3RawResumeHyper(PVM pVM, PVMCPU pVCpu)
{
    int         rc;
    PCPUMCTX    pCtx = pVCpu->em.s.pCtx;
    Assert(pVCpu->em.s.enmState == EMSTATE_DEBUG_HYPER);
    Log(("emR3RawResumeHyper: cs:eip=%RTsel:%RGr efl=%RGr\n", pCtx->cs.Sel, pCtx->eip, pCtx->eflags));

    /*
     * Resume execution.
     */
    CPUMRawEnter(pVCpu);
    CPUMSetHyperEFlags(pVCpu, CPUMGetHyperEFlags(pVCpu) | X86_EFL_RF);
    rc = VMMR3ResumeHyper(pVM, pVCpu);
    Log(("emR3RawResumeHyper: cs:eip=%RTsel:%RGr efl=%RGr - returned from GC with rc=%Rrc\n", pCtx->cs.Sel, pCtx->eip, pCtx->eflags, rc));
    rc = CPUMRawLeave(pVCpu, rc);
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_RESUME_GUEST_MASK);

    /*
     * Deal with the return code.
     */
    rc = emR3HighPriorityPostForcedActions(pVM, pVCpu, rc);
    rc = emR3RawHandleRC(pVM, pVCpu, pCtx, rc);
    rc = emR3RawUpdateForceFlag(pVM, pVCpu, pCtx, rc);
    return rc;
}


/**
 * Steps rawmode.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
int emR3RawStep(PVM pVM, PVMCPU pVCpu)
{
    Assert(   pVCpu->em.s.enmState == EMSTATE_DEBUG_HYPER
           || pVCpu->em.s.enmState == EMSTATE_DEBUG_GUEST_RAW
           || pVCpu->em.s.enmState == EMSTATE_DEBUG_GUEST_REM);
    int         rc;
    PCPUMCTX    pCtx   = pVCpu->em.s.pCtx;
    bool        fGuest = pVCpu->em.s.enmState != EMSTATE_DEBUG_HYPER;
#ifndef DEBUG_sander
    Log(("emR3RawStep: cs:eip=%RTsel:%RGr efl=%RGr\n", fGuest ? CPUMGetGuestCS(pVCpu) : CPUMGetHyperCS(pVCpu),
         fGuest ? CPUMGetGuestEIP(pVCpu) : CPUMGetHyperEIP(pVCpu), fGuest ? CPUMGetGuestEFlags(pVCpu) : CPUMGetHyperEFlags(pVCpu)));
#endif
    if (fGuest)
    {
        /*
         * Check vital forced actions, but ignore pending interrupts and timers.
         */
        if (    VM_FF_IS_PENDING(pVM, VM_FF_HIGH_PRIORITY_PRE_RAW_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK))
        {
            rc = emR3RawForcedActions(pVM, pVCpu, pCtx);
            VBOXVMM_EM_FF_RAW_RET(pVCpu, rc);
            if (rc != VINF_SUCCESS)
                return rc;
        }

        /*
         * Set flags for single stepping.
         */
        CPUMSetGuestEFlags(pVCpu, CPUMGetGuestEFlags(pVCpu) | X86_EFL_TF | X86_EFL_RF);
    }
    else
        CPUMSetHyperEFlags(pVCpu, CPUMGetHyperEFlags(pVCpu) | X86_EFL_TF | X86_EFL_RF);

    /*
     * Single step.
     * We do not start time or anything, if anything we should just do a few nanoseconds.
     */
    CPUMRawEnter(pVCpu);
    do
    {
        if (pVCpu->em.s.enmState == EMSTATE_DEBUG_HYPER)
            rc = VMMR3ResumeHyper(pVM, pVCpu);
        else
            rc = VMMR3RawRunGC(pVM, pVCpu);
#ifndef DEBUG_sander
        Log(("emR3RawStep: cs:eip=%RTsel:%RGr efl=%RGr - GC rc %Rrc\n", fGuest ? CPUMGetGuestCS(pVCpu) : CPUMGetHyperCS(pVCpu),
             fGuest ? CPUMGetGuestEIP(pVCpu) : CPUMGetHyperEIP(pVCpu), fGuest ? CPUMGetGuestEFlags(pVCpu) : CPUMGetHyperEFlags(pVCpu), rc));
#endif
    } while (   rc == VINF_SUCCESS
             || rc == VINF_EM_RAW_INTERRUPT);
    rc = CPUMRawLeave(pVCpu, rc);
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_RESUME_GUEST_MASK);

    /*
     * Make sure the trap flag is cleared.
     * (Too bad if the guest is trying to single step too.)
     */
    if (fGuest)
        CPUMSetGuestEFlags(pVCpu, CPUMGetGuestEFlags(pVCpu) & ~X86_EFL_TF);
    else
        CPUMSetHyperEFlags(pVCpu, CPUMGetHyperEFlags(pVCpu) & ~X86_EFL_TF);

    /*
     * Deal with the return codes.
     */
    rc = emR3HighPriorityPostForcedActions(pVM, pVCpu, rc);
    rc = emR3RawHandleRC(pVM, pVCpu, pCtx, rc);
    rc = emR3RawUpdateForceFlag(pVM, pVCpu, pCtx, rc);
    return rc;
}


#ifdef DEBUG


int emR3SingleStepExecRaw(PVM pVM, PVMCPU pVCpu, uint32_t cIterations)
{
    int     rc          = VINF_SUCCESS;
    EMSTATE enmOldState = pVCpu->em.s.enmState;
    pVCpu->em.s.enmState  = EMSTATE_DEBUG_GUEST_RAW;

    Log(("Single step BEGIN:\n"));
    for (uint32_t i = 0; i < cIterations; i++)
    {
        DBGFR3PrgStep(pVCpu);
        DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "RSS");
        rc = emR3RawStep(pVM, pVCpu);
        if (   rc != VINF_SUCCESS
            && rc != VINF_EM_DBG_STEPPED)
            break;
    }
    Log(("Single step END: rc=%Rrc\n", rc));
    CPUMSetGuestEFlags(pVCpu, CPUMGetGuestEFlags(pVCpu) & ~X86_EFL_TF);
    pVCpu->em.s.enmState = enmOldState;
    return rc;
}

#endif /* DEBUG */


/**
 * Executes one (or perhaps a few more) instruction(s).
 *
 * @returns VBox status code suitable for EM.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   rcGC        GC return code
 * @param   pszPrefix   Disassembly prefix. If not NULL we'll disassemble the
 *                      instruction and prefix the log output with this text.
 */
#if defined(LOG_ENABLED) || defined(DOXYGEN_RUNNING)
static int emR3RawExecuteInstructionWorker(PVM pVM, PVMCPU pVCpu, int rcGC, const char *pszPrefix)
#else
static int emR3RawExecuteInstructionWorker(PVM pVM, PVMCPU pVCpu, int rcGC)
#endif
{
    PCPUMCTX pCtx = pVCpu->em.s.pCtx;
    int      rc;

#ifdef LOG_ENABLED
    /*
     * Disassemble the instruction if requested.
     */
    if (pszPrefix)
    {
        DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", pszPrefix);
        DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, pszPrefix);
    }
#endif /* LOG_ENABLED */

    /*
     * PATM is making life more interesting.
     * We cannot hand anything to REM which has an EIP inside patch code. So, we'll
     * tell PATM there is a trap in this code and have it take the appropriate actions
     * to allow us execute the code in REM.
     */
    if (PATMIsPatchGCAddr(pVM, pCtx->eip))
    {
        Log(("emR3RawExecuteInstruction: In patch block. eip=%RRv\n", (RTRCPTR)pCtx->eip));

        RTGCPTR uNewEip;
        rc = PATMR3HandleTrap(pVM, pCtx, pCtx->eip, &uNewEip);
        switch (rc)
        {
            /*
             * It's not very useful to emulate a single instruction and then go back to raw
             * mode; just execute the whole block until IF is set again.
             */
            case VINF_SUCCESS:
                Log(("emR3RawExecuteInstruction: Executing instruction starting at new address %RGv IF=%d VMIF=%x\n",
                     uNewEip, pCtx->eflags.Bits.u1IF, pVCpu->em.s.pPatmGCState->uVMFlags));
                pCtx->eip = uNewEip;
                Assert(pCtx->eip);

                if (pCtx->eflags.Bits.u1IF)
                {
                    /*
                     * The last instruction in the patch block needs to be executed!! (sti/sysexit for example)
                     */
                    Log(("PATCH: IF=1 -> emulate last instruction as it can't be interrupted!!\n"));
                    return emR3RawExecuteInstruction(pVM, pVCpu, "PATCHIR");
                }
                else if (rcGC == VINF_PATM_PENDING_IRQ_AFTER_IRET)
                {
                    /* special case: iret, that sets IF,  detected a pending irq/event */
                    return emR3RawExecuteInstruction(pVM, pVCpu, "PATCHIRET");
                }
                return VINF_EM_RESCHEDULE_REM;

            /*
             * One instruction.
             */
            case VINF_PATCH_EMULATE_INSTR:
                Log(("emR3RawExecuteInstruction: Emulate patched instruction at %RGv IF=%d VMIF=%x\n",
                     uNewEip, pCtx->eflags.Bits.u1IF, pVCpu->em.s.pPatmGCState->uVMFlags));
                pCtx->eip = uNewEip;
                return emR3RawExecuteInstruction(pVM, pVCpu, "PATCHIR");

            /*
             * The patch was disabled, hand it to the REM.
             */
            case VERR_PATCH_DISABLED:
                Log(("emR3RawExecuteInstruction: Disabled patch -> new eip %RGv IF=%d VMIF=%x\n",
                     uNewEip, pCtx->eflags.Bits.u1IF, pVCpu->em.s.pPatmGCState->uVMFlags));
                pCtx->eip = uNewEip;
                if (pCtx->eflags.Bits.u1IF)
                {
                    /*
                     * The last instruction in the patch block needs to be executed!! (sti/sysexit for example)
                     */
                    Log(("PATCH: IF=1 -> emulate last instruction as it can't be interrupted!!\n"));
                    return emR3RawExecuteInstruction(pVM, pVCpu, "PATCHIR");
                }
                return VINF_EM_RESCHEDULE_REM;

            /* Force continued patch exection; usually due to write monitored stack. */
            case VINF_PATCH_CONTINUE:
                return VINF_SUCCESS;

            default:
                AssertReleaseMsgFailed(("Unknown return code %Rrc from PATMR3HandleTrap\n", rc));
                return VERR_IPE_UNEXPECTED_STATUS;
        }
    }


    /*
     * Use IEM and fallback on REM if the functionality is missing.
     * Once IEM gets mature enough, nothing should ever fall back.
     */
#define VBOX_WITH_FIRST_IEM_STEP_B
#if defined(VBOX_WITH_FIRST_IEM_STEP_B) || !defined(VBOX_WITH_REM)
    Log(("EMINS: %04x:%RGv RSP=%RGv\n", pCtx->cs.Sel, (RTGCPTR)pCtx->rip, (RTGCPTR)pCtx->rsp));
    STAM_PROFILE_START(&pVCpu->em.s.StatIEMEmu, a);
    rc = VBOXSTRICTRC_TODO(IEMExecOne(pVCpu));
    STAM_PROFILE_STOP(&pVCpu->em.s.StatIEMEmu, a);
    if (RT_SUCCESS(rc))
    {
        if (rc == VINF_SUCCESS || rc == VINF_EM_RESCHEDULE)
            rc = VINF_EM_RESCHEDULE;
    }
    else if (   rc == VERR_IEM_ASPECT_NOT_IMPLEMENTED
             || rc == VERR_IEM_INSTR_NOT_IMPLEMENTED)
#endif
    {
#ifdef VBOX_WITH_REM
        STAM_PROFILE_START(&pVCpu->em.s.StatREMEmu, b);
# ifndef VBOX_WITH_FIRST_IEM_STEP_B
        Log(("EMINS[rem]: %04x:%RGv RSP=%RGv\n", pCtx->cs.Sel, (RTGCPTR)pCtx->rip, (RTGCPTR)pCtx->rsp));
//# elif defined(DEBUG_bird)
//        AssertFailed();
# endif
        EMRemLock(pVM);
        /* Flush the recompiler TLB if the VCPU has changed. */
        if (pVM->em.s.idLastRemCpu != pVCpu->idCpu)
            CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_ALL);
        pVM->em.s.idLastRemCpu = pVCpu->idCpu;

        rc = REMR3EmulateInstruction(pVM, pVCpu);
        EMRemUnlock(pVM);
        STAM_PROFILE_STOP(&pVCpu->em.s.StatREMEmu, b);
#else  /* !VBOX_WITH_REM */
        NOREF(pVM);
#endif /* !VBOX_WITH_REM */
    }
    return rc;
}


/**
 * Executes one (or perhaps a few more) instruction(s).
 * This is just a wrapper for discarding pszPrefix in non-logging builds.
 *
 * @returns VBox status code suitable for EM.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pszPrefix   Disassembly prefix. If not NULL we'll disassemble the
 *                      instruction and prefix the log output with this text.
 * @param   rcGC        GC return code
 */
DECLINLINE(int) emR3RawExecuteInstruction(PVM pVM, PVMCPU pVCpu, const char *pszPrefix, int rcGC)
{
#ifdef LOG_ENABLED
    return emR3RawExecuteInstructionWorker(pVM, pVCpu, rcGC, pszPrefix);
#else
    RT_NOREF_PV(pszPrefix);
    return emR3RawExecuteInstructionWorker(pVM, pVCpu, rcGC);
#endif
}

/**
 * Executes one (or perhaps a few more) IO instruction(s).
 *
 * @returns VBox status code suitable for EM.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
static int emR3RawExecuteIOInstruction(PVM pVM, PVMCPU pVCpu)
{
    STAM_PROFILE_START(&pVCpu->em.s.StatIOEmu, a);
    RT_NOREF_PV(pVM);

    /* Hand it over to the interpreter. */
    VBOXSTRICTRC rcStrict = IEMExecOne(pVCpu);
    LogFlow(("emR3RawExecuteIOInstruction: %Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
    STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->StatIoIem);
    STAM_PROFILE_STOP(&pVCpu->em.s.StatIOEmu, a);
    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * Handle a guest context trap.
 *
 * @returns VBox status code suitable for EM.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
static int emR3RawGuestTrap(PVM pVM, PVMCPU pVCpu)
{
    PCPUMCTX pCtx = pVCpu->em.s.pCtx;

    /*
     * Get the trap info.
     */
    uint8_t         u8TrapNo;
    TRPMEVENT       enmType;
    RTGCUINT        uErrorCode;
    RTGCUINTPTR     uCR2;
    int rc = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrorCode, &uCR2, NULL /* pu8InstrLen */);
    if (RT_FAILURE(rc))
    {
        AssertReleaseMsgFailed(("No trap! (rc=%Rrc)\n", rc));
        return rc;
    }


#if 1 /* Experimental: Review, disable if it causes trouble. */
    /*
     * Handle traps in patch code first.
     *
     * We catch a few of these cases in RC before returning to R3 (#PF, #GP, #BP)
     * but several traps isn't handled specially by TRPM in RC and we end up here
     * instead. One example is #DE.
     */
    uint32_t uCpl = CPUMGetGuestCPL(pVCpu);
    if (    uCpl == 0
        &&  PATMIsPatchGCAddr(pVM, pCtx->eip))
    {
        LogFlow(("emR3RawGuestTrap: trap %#x in patch code; eip=%08x\n", u8TrapNo, pCtx->eip));
        return emR3RawPatchTrap(pVM, pVCpu, pCtx, rc);
    }
#endif

    /*
     * If the guest gate is marked unpatched, then we will check again if we can patch it.
     * (This assumes that we've already tried and failed to dispatch the trap in
     * RC for the gates that already has been patched. Which is true for most high
     * volume traps, because these are handled specially, but not for odd ones like #DE.)
     */
    if (TRPMR3GetGuestTrapHandler(pVM, u8TrapNo) == TRPM_INVALID_HANDLER)
    {
        CSAMR3CheckGates(pVM, u8TrapNo, 1);
        Log(("emR3RawHandleRC: recheck gate %x -> valid=%d\n", u8TrapNo, TRPMR3GetGuestTrapHandler(pVM, u8TrapNo) != TRPM_INVALID_HANDLER));

        /* If it was successful, then we could go back to raw mode. */
        if (TRPMR3GetGuestTrapHandler(pVM, u8TrapNo) != TRPM_INVALID_HANDLER)
        {
            /* Must check pending forced actions as our IDT or GDT might be out of sync. */
            rc = EMR3CheckRawForcedActions(pVM, pVCpu);
            AssertRCReturn(rc, rc);

            TRPMERRORCODE enmError = uErrorCode != ~0U
                                   ? TRPM_TRAP_HAS_ERRORCODE
                                   : TRPM_TRAP_NO_ERRORCODE;
            rc = TRPMForwardTrap(pVCpu, CPUMCTX2CORE(pCtx), u8TrapNo, uErrorCode, enmError, TRPM_TRAP, -1);
            if (rc == VINF_SUCCESS /* Don't use RT_SUCCESS */)
            {
                TRPMResetTrap(pVCpu);
                return VINF_EM_RESCHEDULE_RAW;
            }
            AssertMsg(rc == VINF_EM_RAW_GUEST_TRAP, ("%Rrc\n", rc));
        }
    }

    /*
     * Scan kernel code that traps; we might not get another chance.
     */
    /** @todo move this up before the dispatching? */
    if (    (pCtx->ss.Sel & X86_SEL_RPL) <= 1
        &&  !pCtx->eflags.Bits.u1VM)
    {
        Assert(!PATMIsPatchGCAddr(pVM, pCtx->eip));
        CSAMR3CheckCodeEx(pVM, pCtx, pCtx->eip);
    }

    /*
     * Trap specific handling.
     */
    if (u8TrapNo == 6) /* (#UD) Invalid opcode. */
    {
        /*
         * If MONITOR & MWAIT are supported, then interpret them here.
         */
        DISCPUSTATE cpu;
        rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pCtx->rip, &cpu, "Guest Trap (#UD): ");
        if (    RT_SUCCESS(rc)
            && (cpu.pCurInstr->uOpcode == OP_MONITOR || cpu.pCurInstr->uOpcode == OP_MWAIT))
        {
            uint32_t u32Dummy, u32Features, u32ExtFeatures;
            CPUMGetGuestCpuId(pVCpu, 1, 0, &u32Dummy, &u32Dummy, &u32ExtFeatures, &u32Features);
            if (u32ExtFeatures & X86_CPUID_FEATURE_ECX_MONITOR)
            {
                rc = TRPMResetTrap(pVCpu);
                AssertRC(rc);

                rc = VBOXSTRICTRC_TODO(EMInterpretInstructionDisasState(pVCpu, &cpu, CPUMCTX2CORE(pCtx), 0, EMCODETYPE_SUPERVISOR));
                if (RT_SUCCESS(rc))
                    return rc;
                return emR3RawExecuteInstruction(pVM, pVCpu, "Monitor: ");
            }
        }
    }
    else if (u8TrapNo == 13) /* (#GP) Privileged exception */
    {
        /*
         * Handle I/O bitmap?
         */
        /** @todo We're not supposed to be here with a false guest trap concerning
         *        I/O access. We can easily handle those in RC.  */
        DISCPUSTATE cpu;
        rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pCtx->rip, &cpu, "Guest Trap: ");
        if (    RT_SUCCESS(rc)
            &&  (cpu.pCurInstr->fOpType & DISOPTYPE_PORTIO))
        {
            /*
             * We should really check the TSS for the IO bitmap, but it's not like this
             * lazy approach really makes things worse.
             */
            rc = TRPMResetTrap(pVCpu);
            AssertRC(rc);
            return emR3RawExecuteInstruction(pVM, pVCpu, "IO Guest Trap: ");
        }
    }

#ifdef LOG_ENABLED
    DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", "Guest trap");
    DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Guest trap");

    /* Get guest page information. */
    uint64_t    fFlags = 0;
    RTGCPHYS    GCPhys = 0;
    int rc2 = PGMGstGetPage(pVCpu, uCR2, &fFlags, &GCPhys);
    Log(("emR3RawGuestTrap: cs:eip=%04x:%08x: trap=%02x err=%08x cr2=%08x cr0=%08x%s: Phys=%RGp fFlags=%08llx %s %s %s%s rc2=%d\n",
         pCtx->cs.Sel, pCtx->eip, u8TrapNo, uErrorCode, uCR2, (uint32_t)pCtx->cr0,
         (enmType == TRPM_SOFTWARE_INT) ? " software" : "",  GCPhys, fFlags,
         fFlags & X86_PTE_P  ? "P " : "NP", fFlags & X86_PTE_US ? "U"  : "S",
         fFlags & X86_PTE_RW ? "RW" : "R0", fFlags & X86_PTE_G  ? " G" : "", rc2));
#endif

    /*
     * #PG has CR2.
     * (Because of stuff like above we must set CR2 in a delayed fashion.)
     */
    if (u8TrapNo == 14 /* #PG */)
        pCtx->cr2 = uCR2;

    return VINF_EM_RESCHEDULE_REM;
}


/**
 * Handle a ring switch trap.
 * Need to do statistics and to install patches. The result is going to REM.
 *
 * @returns VBox status code suitable for EM.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
static int emR3RawRingSwitch(PVM pVM, PVMCPU pVCpu)
{
    int         rc;
    DISCPUSTATE Cpu;
    PCPUMCTX    pCtx = pVCpu->em.s.pCtx;

    /*
     * sysenter, syscall & callgate
     */
    rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pCtx->rip, &Cpu, "RSWITCH: ");
    if (RT_SUCCESS(rc))
    {
        if (Cpu.pCurInstr->uOpcode == OP_SYSENTER)
        {
            if (pCtx->SysEnter.cs != 0)
            {
                rc = PATMR3InstallPatch(pVM, SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), pCtx->eip),
                                        CPUMGetGuestCodeBits(pVCpu) == 32 ? PATMFL_CODE32 : 0);
                if (RT_SUCCESS(rc))
                {
                    DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Patched sysenter instruction");
                    return VINF_EM_RESCHEDULE_RAW;
                }
            }
        }

#ifdef VBOX_WITH_STATISTICS
        switch (Cpu.pCurInstr->uOpcode)
        {
            case OP_SYSENTER:
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->StatSysEnter);
                break;
            case OP_SYSEXIT:
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->StatSysExit);
                break;
            case OP_SYSCALL:
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->StatSysCall);
                break;
            case OP_SYSRET:
                STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->StatSysRet);
                break;
        }
#endif
    }
    else
        AssertRC(rc);

    /* go to the REM to emulate a single instruction */
    return emR3RawExecuteInstruction(pVM, pVCpu, "RSWITCH: ");
}


/**
 * Handle a trap (\#PF or \#GP) in patch code
 *
 * @returns VBox status code suitable for EM.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   pCtx    Pointer to the guest CPU context.
 * @param   gcret   GC return code.
 */
static int emR3RawPatchTrap(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int gcret)
{
    uint8_t         u8TrapNo;
    int             rc;
    TRPMEVENT       enmType;
    RTGCUINT        uErrorCode;
    RTGCUINTPTR     uCR2;

    Assert(PATMIsPatchGCAddr(pVM, pCtx->eip));

    if (gcret == VINF_PATM_PATCH_INT3)
    {
        u8TrapNo   = 3;
        uCR2       = 0;
        uErrorCode = 0;
    }
    else if (gcret == VINF_PATM_PATCH_TRAP_GP)
    {
        /* No active trap in this case. Kind of ugly. */
        u8TrapNo   = X86_XCPT_GP;
        uCR2       = 0;
        uErrorCode = 0;
    }
    else
    {
        rc = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrorCode, &uCR2, NULL /* pu8InstrLen */);
        if (RT_FAILURE(rc))
        {
            AssertReleaseMsgFailed(("emR3RawPatchTrap: no trap! (rc=%Rrc) gcret=%Rrc\n", rc, gcret));
            return rc;
        }
        /* Reset the trap as we'll execute the original instruction again. */
        TRPMResetTrap(pVCpu);
    }

    /*
     * Deal with traps inside patch code.
     * (This code won't run outside GC.)
     */
    if (u8TrapNo != 1)
    {
#ifdef LOG_ENABLED
        DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", "Trap in patch code");
        DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Patch code");

        DISCPUSTATE Cpu;
        rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pCtx->eip, &Cpu, "Patch code: ");
        if (    RT_SUCCESS(rc)
            &&  Cpu.pCurInstr->uOpcode == OP_IRET)
        {
            uint32_t eip, selCS, uEFlags;

            /* Iret crashes are bad as we have already changed the flags on the stack */
            rc  = PGMPhysSimpleReadGCPtr(pVCpu, &eip,     pCtx->esp, 4);
            rc |= PGMPhysSimpleReadGCPtr(pVCpu, &selCS,   pCtx->esp+4, 4);
            rc |= PGMPhysSimpleReadGCPtr(pVCpu, &uEFlags, pCtx->esp+8, 4);
            if (rc == VINF_SUCCESS)
            {
                if (    (uEFlags & X86_EFL_VM)
                    ||  (selCS & X86_SEL_RPL) == 3)
                {
                    uint32_t selSS, esp;

                    rc |= PGMPhysSimpleReadGCPtr(pVCpu, &esp,     pCtx->esp + 12, 4);
                    rc |= PGMPhysSimpleReadGCPtr(pVCpu, &selSS,   pCtx->esp + 16, 4);

                    if (uEFlags & X86_EFL_VM)
                    {
                        uint32_t selDS, selES, selFS, selGS;
                        rc  = PGMPhysSimpleReadGCPtr(pVCpu, &selES,   pCtx->esp + 20, 4);
                        rc |= PGMPhysSimpleReadGCPtr(pVCpu, &selDS,   pCtx->esp + 24, 4);
                        rc |= PGMPhysSimpleReadGCPtr(pVCpu, &selFS,   pCtx->esp + 28, 4);
                        rc |= PGMPhysSimpleReadGCPtr(pVCpu, &selGS,   pCtx->esp + 32, 4);
                        if (rc == VINF_SUCCESS)
                        {
                            Log(("Patch code: IRET->VM stack frame: return address %04X:%08RX32 eflags=%08x ss:esp=%04X:%08RX32\n", selCS, eip, uEFlags, selSS, esp));
                            Log(("Patch code: IRET->VM stack frame: DS=%04X ES=%04X FS=%04X GS=%04X\n", selDS, selES, selFS, selGS));
                        }
                    }
                    else
                        Log(("Patch code: IRET stack frame: return address %04X:%08RX32 eflags=%08x ss:esp=%04X:%08RX32\n", selCS, eip, uEFlags, selSS, esp));
                }
                else
                    Log(("Patch code: IRET stack frame: return address %04X:%08RX32 eflags=%08x\n", selCS, eip, uEFlags));
            }
        }
#endif /* LOG_ENABLED */
        Log(("emR3RawPatchTrap: in patch: eip=%08x: trap=%02x err=%08x cr2=%08x cr0=%08x\n",
             pCtx->eip, u8TrapNo, uErrorCode, uCR2, (uint32_t)pCtx->cr0));

        RTGCPTR uNewEip;
        rc = PATMR3HandleTrap(pVM, pCtx, pCtx->eip, &uNewEip);
        switch (rc)
        {
            /*
             * Execute the faulting instruction.
             */
            case VINF_SUCCESS:
            {
                /** @todo execute a whole block */
                Log(("emR3RawPatchTrap: Executing faulting instruction at new address %RGv\n", uNewEip));
                if (!(pVCpu->em.s.pPatmGCState->uVMFlags & X86_EFL_IF))
                    Log(("emR3RawPatchTrap: Virtual IF flag disabled!!\n"));

                pCtx->eip = uNewEip;
                AssertRelease(pCtx->eip);

                if (pCtx->eflags.Bits.u1IF)
                {
                    /* Windows XP lets irets fault intentionally and then takes action based on the opcode; an
                     * int3 patch overwrites it and leads to blue screens. Remove the patch in this case.
                     */
                    if (    u8TrapNo == X86_XCPT_GP
                        &&  PATMIsInt3Patch(pVM, pCtx->eip, NULL, NULL))
                    {
                        /** @todo move to PATMR3HandleTrap */
                        Log(("Possible Windows XP iret fault at %08RX32\n", pCtx->eip));
                        PATMR3RemovePatch(pVM, pCtx->eip);
                    }

                    /** @todo Knoppix 5 regression when returning VINF_SUCCESS here and going back to raw mode. */
                    /* Note: possibly because a reschedule is required (e.g. iret to V86 code) */

                    return emR3RawExecuteInstruction(pVM, pVCpu, "PATCHIR");
                    /* Interrupts are enabled; just go back to the original instruction.
                    return VINF_SUCCESS; */
                }
                return VINF_EM_RESCHEDULE_REM;
            }

            /*
             * One instruction.
             */
            case VINF_PATCH_EMULATE_INSTR:
                Log(("emR3RawPatchTrap: Emulate patched instruction at %RGv IF=%d VMIF=%x\n",
                     uNewEip, pCtx->eflags.Bits.u1IF, pVCpu->em.s.pPatmGCState->uVMFlags));
                pCtx->eip = uNewEip;
                AssertRelease(pCtx->eip);
                return emR3RawExecuteInstruction(pVM, pVCpu, "PATCHEMUL: ");

            /*
             * The patch was disabled, hand it to the REM.
             */
            case VERR_PATCH_DISABLED:
                if (!(pVCpu->em.s.pPatmGCState->uVMFlags & X86_EFL_IF))
                    Log(("emR3RawPatchTrap: Virtual IF flag disabled!!\n"));
                pCtx->eip = uNewEip;
                AssertRelease(pCtx->eip);

                if (pCtx->eflags.Bits.u1IF)
                {
                    /*
                     * The last instruction in the patch block needs to be executed!! (sti/sysexit for example)
                     */
                    Log(("PATCH: IF=1 -> emulate last instruction as it can't be interrupted!!\n"));
                    return emR3RawExecuteInstruction(pVM, pVCpu, "PATCHIR");
                }
                return VINF_EM_RESCHEDULE_REM;

            /* Force continued patch exection; usually due to write monitored stack. */
            case VINF_PATCH_CONTINUE:
                return VINF_SUCCESS;

            /*
             * Anything else is *fatal*.
             */
            default:
                AssertReleaseMsgFailed(("Unknown return code %Rrc from PATMR3HandleTrap!\n", rc));
                return VERR_IPE_UNEXPECTED_STATUS;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Handle a privileged instruction.
 *
 * @returns VBox status code suitable for EM.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
static int emR3RawPrivileged(PVM pVM, PVMCPU pVCpu)
{
    PCPUMCTX    pCtx = pVCpu->em.s.pCtx;

    Assert(!pCtx->eflags.Bits.u1VM);

    if (PATMIsEnabled(pVM))
    {
        /*
         * Check if in patch code.
         */
        if (PATMR3IsInsidePatchJump(pVM, pCtx->eip, NULL))
        {
#ifdef LOG_ENABLED
            DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", "PRIV");
#endif
            AssertMsgFailed(("FATAL ERROR: executing random instruction inside generated patch jump %08x\n", pCtx->eip));
            return VERR_EM_RAW_PATCH_CONFLICT;
        }
        if (   (pCtx->ss.Sel & X86_SEL_RPL) == 0
            && !pCtx->eflags.Bits.u1VM
            && !PATMIsPatchGCAddr(pVM, pCtx->eip))
        {
            int rc = PATMR3InstallPatch(pVM, SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), pCtx->eip),
                                        CPUMGetGuestCodeBits(pVCpu) == 32 ? PATMFL_CODE32 : 0);
            if (RT_SUCCESS(rc))
            {
#ifdef LOG_ENABLED
                DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", "PRIV");
#endif
                DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Patched privileged instruction");
                return VINF_SUCCESS;
            }
        }
    }

#ifdef LOG_ENABLED
    if (!PATMIsPatchGCAddr(pVM, pCtx->eip))
    {
        DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", "PRIV");
        DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Privileged instr");
    }
#endif

    /*
     * Instruction statistics and logging.
     */
    DISCPUSTATE Cpu;
    int         rc;

    rc = CPUMR3DisasmInstrCPU(pVM, pVCpu, pCtx, pCtx->rip, &Cpu, "PRIV: ");
    if (RT_SUCCESS(rc))
    {
#ifdef VBOX_WITH_STATISTICS
        PEMSTATS pStats = pVCpu->em.s.CTX_SUFF(pStats);
        switch (Cpu.pCurInstr->uOpcode)
        {
            case OP_INVLPG:
                STAM_COUNTER_INC(&pStats->StatInvlpg);
                break;
            case OP_IRET:
                STAM_COUNTER_INC(&pStats->StatIret);
                break;
            case OP_CLI:
                STAM_COUNTER_INC(&pStats->StatCli);
                emR3RecordCli(pVM, pVCpu, pCtx->rip);
                break;
            case OP_STI:
                STAM_COUNTER_INC(&pStats->StatSti);
                break;
            case OP_INSB:
            case OP_INSWD:
            case OP_IN:
            case OP_OUTSB:
            case OP_OUTSWD:
            case OP_OUT:
                AssertMsgFailed(("Unexpected privileged exception due to port IO\n"));
                break;

            case OP_MOV_CR:
                if (Cpu.Param1.fUse & DISUSE_REG_GEN32)
                {
                    //read
                    Assert(Cpu.Param2.fUse & DISUSE_REG_CR);
                    Assert(Cpu.Param2.Base.idxCtrlReg <= DISCREG_CR4);
                    STAM_COUNTER_INC(&pStats->StatMovReadCR[Cpu.Param2.Base.idxCtrlReg]);
                }
                else
                {
                    //write
                    Assert(Cpu.Param1.fUse & DISUSE_REG_CR);
                    Assert(Cpu.Param1.Base.idxCtrlReg <= DISCREG_CR4);
                    STAM_COUNTER_INC(&pStats->StatMovWriteCR[Cpu.Param1.Base.idxCtrlReg]);
                }
                break;

            case OP_MOV_DR:
                STAM_COUNTER_INC(&pStats->StatMovDRx);
                break;
            case OP_LLDT:
                STAM_COUNTER_INC(&pStats->StatMovLldt);
                break;
            case OP_LIDT:
                STAM_COUNTER_INC(&pStats->StatMovLidt);
                break;
            case OP_LGDT:
                STAM_COUNTER_INC(&pStats->StatMovLgdt);
                break;
            case OP_SYSENTER:
                STAM_COUNTER_INC(&pStats->StatSysEnter);
                break;
            case OP_SYSEXIT:
                STAM_COUNTER_INC(&pStats->StatSysExit);
                break;
            case OP_SYSCALL:
                STAM_COUNTER_INC(&pStats->StatSysCall);
                break;
            case OP_SYSRET:
                STAM_COUNTER_INC(&pStats->StatSysRet);
                break;
            case OP_HLT:
                STAM_COUNTER_INC(&pStats->StatHlt);
                break;
            default:
                STAM_COUNTER_INC(&pStats->StatMisc);
                Log4(("emR3RawPrivileged: opcode=%d\n", Cpu.pCurInstr->uOpcode));
                break;
        }
#endif /* VBOX_WITH_STATISTICS */
        if (    (pCtx->ss.Sel & X86_SEL_RPL) == 0
            &&  !pCtx->eflags.Bits.u1VM
            &&  CPUMGetGuestCodeBits(pVCpu) == 32)
        {
            STAM_PROFILE_START(&pVCpu->em.s.StatPrivEmu, a);
            switch (Cpu.pCurInstr->uOpcode)
            {
                case OP_CLI:
                    pCtx->eflags.u32 &= ~X86_EFL_IF;
                    Assert(Cpu.cbInstr == 1);
                    pCtx->rip += Cpu.cbInstr;
                    STAM_PROFILE_STOP(&pVCpu->em.s.StatPrivEmu, a);
                    return VINF_EM_RESCHEDULE_REM; /* must go to the recompiler now! */

                case OP_STI:
                    pCtx->eflags.u32 |= X86_EFL_IF;
                    EMSetInhibitInterruptsPC(pVCpu, pCtx->rip + Cpu.cbInstr);
                    Assert(Cpu.cbInstr == 1);
                    pCtx->rip += Cpu.cbInstr;
                    STAM_PROFILE_STOP(&pVCpu->em.s.StatPrivEmu, a);
                    return VINF_SUCCESS;

                case OP_HLT:
                    if (PATMIsPatchGCAddr(pVM, pCtx->eip))
                    {
                        PATMTRANSSTATE  enmState;
                        RTGCPTR         pOrgInstrGC = PATMR3PatchToGCPtr(pVM, pCtx->eip, &enmState);

                        if (enmState == PATMTRANS_OVERWRITTEN)
                        {
                            rc = PATMR3DetectConflict(pVM, pOrgInstrGC, pOrgInstrGC);
                            Assert(rc == VERR_PATCH_DISABLED);
                            /* Conflict detected, patch disabled */
                            Log(("emR3RawPrivileged: detected conflict -> disabled patch at %08RX32\n", pCtx->eip));

                            enmState = PATMTRANS_SAFE;
                        }

                        /* The translation had better be successful. Otherwise we can't recover. */
                        AssertReleaseMsg(pOrgInstrGC && enmState != PATMTRANS_OVERWRITTEN, ("Unable to translate instruction address at %08RX32\n", pCtx->eip));
                        if (enmState != PATMTRANS_OVERWRITTEN)
                            pCtx->eip = pOrgInstrGC;
                    }
                    /* no break; we could just return VINF_EM_HALT here */
                    RT_FALL_THRU();

                case OP_MOV_CR:
                case OP_MOV_DR:
#ifdef LOG_ENABLED
                    if (PATMIsPatchGCAddr(pVM, pCtx->eip))
                    {
                        DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", "PRIV");
                        DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Privileged instr");
                    }
#endif

                    rc = VBOXSTRICTRC_TODO(EMInterpretInstructionDisasState(pVCpu, &Cpu, CPUMCTX2CORE(pCtx), 0, EMCODETYPE_SUPERVISOR));
                    if (RT_SUCCESS(rc))
                    {
                        STAM_PROFILE_STOP(&pVCpu->em.s.StatPrivEmu, a);

                        if (    Cpu.pCurInstr->uOpcode == OP_MOV_CR
                            &&  Cpu.Param1.fUse == DISUSE_REG_CR /* write */
                           )
                        {
                            /* Deal with CR0 updates inside patch code that force
                             * us to go to the recompiler.
                             */
                            if (   PATMIsPatchGCAddr(pVM, pCtx->rip)
                                && (pCtx->cr0 & (X86_CR0_WP|X86_CR0_PG|X86_CR0_PE)) != (X86_CR0_WP|X86_CR0_PG|X86_CR0_PE))
                            {
                                PATMTRANSSTATE  enmState;
                                RTGCPTR         pOrgInstrGC = PATMR3PatchToGCPtr(pVM, pCtx->rip, &enmState);

                                Log(("Force recompiler switch due to cr0 (%RGp) update rip=%RGv -> %RGv (enmState=%d)\n", pCtx->cr0, pCtx->rip, pOrgInstrGC, enmState));
                                if (enmState == PATMTRANS_OVERWRITTEN)
                                {
                                    rc = PATMR3DetectConflict(pVM, pOrgInstrGC, pOrgInstrGC);
                                    Assert(rc == VERR_PATCH_DISABLED);
                                    /* Conflict detected, patch disabled */
                                    Log(("emR3RawPrivileged: detected conflict -> disabled patch at %RGv\n", (RTGCPTR)pCtx->rip));
                                    enmState = PATMTRANS_SAFE;
                                }
                                /* The translation had better be successful. Otherwise we can't recover. */
                                AssertReleaseMsg(pOrgInstrGC && enmState != PATMTRANS_OVERWRITTEN, ("Unable to translate instruction address at %RGv\n", (RTGCPTR)pCtx->rip));
                                if (enmState != PATMTRANS_OVERWRITTEN)
                                    pCtx->rip = pOrgInstrGC;
                            }

                            /* Reschedule is necessary as the execution/paging mode might have changed. */
                            return VINF_EM_RESCHEDULE;
                        }
                        return rc; /* can return VINF_EM_HALT as well. */
                    }
                    AssertMsgReturn(rc == VERR_EM_INTERPRETER, ("%Rrc\n", rc), rc);
                    break; /* fall back to the recompiler */
            }
            STAM_PROFILE_STOP(&pVCpu->em.s.StatPrivEmu, a);
        }
    }

    if (PATMIsPatchGCAddr(pVM, pCtx->eip))
        return emR3RawPatchTrap(pVM, pVCpu, pCtx, VINF_PATM_PATCH_TRAP_GP);

    return emR3RawExecuteInstruction(pVM, pVCpu, "PRIV");
}


/**
 * Update the forced rawmode execution modifier.
 *
 * This function is called when we're returning from the raw-mode loop(s). If we're
 * in patch code, it will set a flag forcing execution to be resumed in raw-mode,
 * if not in patch code, the flag will be cleared.
 *
 * We should never interrupt patch code while it's being executed. Cli patches can
 * contain big code blocks, but they are always executed with IF=0. Other patches
 * replace single instructions and should be atomic.
 *
 * @returns Updated rc.
 *
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   pCtx    Pointer to the guest CPU context.
 * @param   rc      The result code.
 */
int emR3RawUpdateForceFlag(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int rc)
{
    if (PATMIsPatchGCAddr(pVM, pCtx->eip)) /** @todo check cs selector base/type */
    {
        /* ignore reschedule attempts. */
        switch (rc)
        {
            case VINF_EM_RESCHEDULE:
            case VINF_EM_RESCHEDULE_REM:
                LogFlow(("emR3RawUpdateForceFlag: patch address -> force raw reschedule\n"));
                rc = VINF_SUCCESS;
                break;
        }
        pVCpu->em.s.fForceRAW = true;
    }
    else
        pVCpu->em.s.fForceRAW = false;
    return rc;
}


/**
 * Check for pending raw actions
 *
 * @returns VBox status code. May return VINF_EM_NO_MEMORY but none of the other
 *          EM statuses.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3_INT_DECL(int) EMR3CheckRawForcedActions(PVM pVM, PVMCPU pVCpu)
{
    int rc = emR3RawForcedActions(pVM, pVCpu, pVCpu->em.s.pCtx);
    VBOXVMM_EM_FF_RAW_RET(pVCpu, rc);
    return rc;
}


/**
 * Process raw-mode specific forced actions.
 *
 * This function is called when any FFs in the VM_FF_HIGH_PRIORITY_PRE_RAW_MASK is pending.
 *
 * @returns VBox status code. May return VINF_EM_NO_MEMORY but none of the other
 *          EM statuses.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest CPU context.
 */
static int emR3RawForcedActions(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * Note that the order is *vitally* important!
     * Also note that SELMR3UpdateFromCPUM may trigger VM_FF_SELM_SYNC_TSS.
     */
    VBOXVMM_EM_FF_RAW(pVCpu, pVM->fGlobalForcedActions, pVCpu->fLocalForcedActions);

    /*
     * Sync selector tables.
     */
    if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_LDT))
    {
        VBOXSTRICTRC rcStrict = SELMR3UpdateFromCPUM(pVM, pVCpu);
        if (rcStrict != VINF_SUCCESS)
            return VBOXSTRICTRC_TODO(rcStrict);
    }

    /*
     * Sync IDT.
     *
     * The CSAMR3CheckGates call in TRPMR3SyncIDT may call PGMPrefetchPage
     * and PGMShwModifyPage, so we're in for trouble if for instance a
     * PGMSyncCR3+pgmR3PoolClearAll is pending.
     */
    if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_TRPM_SYNC_IDT))
    {
        if (   VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_PGM_SYNC_CR3)
            && EMIsRawRing0Enabled(pVM)
            && CSAMIsEnabled(pVM))
        {
            int rc = PGMSyncCR3(pVCpu, pCtx->cr0, pCtx->cr3, pCtx->cr4, VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
            if (RT_FAILURE(rc))
                return rc;
        }

        int rc = TRPMR3SyncIDT(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Sync TSS.
     */
    if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_TSS))
    {
        int rc = SELMR3SyncTSS(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Sync page directory.
     */
    if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL))
    {
        Assert(pVCpu->em.s.enmState != EMSTATE_WAIT_SIPI);
        int rc = PGMSyncCR3(pVCpu, pCtx->cr0, pCtx->cr3, pCtx->cr4, VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
        if (RT_FAILURE(rc))
            return rc == VERR_PGM_NO_HYPERVISOR_ADDRESS ? VINF_EM_RESCHEDULE_REM : rc;

        Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_LDT));

        /* Prefetch pages for EIP and ESP. */
        /** @todo This is rather expensive. Should investigate if it really helps at all. */
        rc = PGMPrefetchPage(pVCpu, SELMToFlat(pVM, DISSELREG_CS, CPUMCTX2CORE(pCtx), pCtx->rip));
        if (rc == VINF_SUCCESS)
            rc = PGMPrefetchPage(pVCpu, SELMToFlat(pVM, DISSELREG_SS, CPUMCTX2CORE(pCtx), pCtx->rsp));
        if (rc != VINF_SUCCESS)
        {
            if (rc != VINF_PGM_SYNC_CR3)
            {
                AssertLogRelMsgReturn(RT_FAILURE(rc), ("%Rrc\n", rc), VERR_IPE_UNEXPECTED_INFO_STATUS);
                return rc;
            }
            rc = PGMSyncCR3(pVCpu, pCtx->cr0, pCtx->cr3, pCtx->cr4, VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
            if (RT_FAILURE(rc))
                return rc;
        }
        /** @todo maybe prefetch the supervisor stack page as well */
        Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_LDT));
    }

    /*
     * Allocate handy pages (just in case the above actions have consumed some pages).
     */
    if (VM_FF_IS_PENDING_EXCEPT(pVM, VM_FF_PGM_NEED_HANDY_PAGES, VM_FF_PGM_NO_MEMORY))
    {
        int rc = PGMR3PhysAllocateHandyPages(pVM);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Check whether we're out of memory now.
     *
     * This may stem from some of the above actions or operations that has been executed
     * since we ran FFs. The allocate handy pages must for instance always be followed by
     * this check.
     */
    if (VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY))
        return VINF_EM_NO_MEMORY;

    return VINF_SUCCESS;
}


/**
 * Executes raw code.
 *
 * This function contains the raw-mode version of the inner
 * execution loop (the outer loop being in EMR3ExecuteVM()).
 *
 * @returns VBox status code. The most important ones are: VINF_EM_RESCHEDULE,
 *          VINF_EM_RESCHEDULE_REM, VINF_EM_SUSPEND, VINF_EM_RESET and VINF_EM_TERMINATE.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pfFFDone    Where to store an indicator telling whether or not
 *                      FFs were done before returning.
 */
int emR3RawExecute(PVM pVM, PVMCPU pVCpu, bool *pfFFDone)
{
    STAM_REL_PROFILE_ADV_START(&pVCpu->em.s.StatRAWTotal, a);

    int      rc = VERR_IPE_UNINITIALIZED_STATUS;
    PCPUMCTX pCtx = pVCpu->em.s.pCtx;
    LogFlow(("emR3RawExecute: (cs:eip=%04x:%08x)\n", pCtx->cs.Sel, pCtx->eip));
    pVCpu->em.s.fForceRAW = false;
    *pfFFDone = false;


    /*
     *
     * Spin till we get a forced action or raw mode status code resulting in
     * in anything but VINF_SUCCESS or VINF_EM_RESCHEDULE_RAW.
     *
     */
    for (;;)
    {
        STAM_PROFILE_ADV_START(&pVCpu->em.s.StatRAWEntry, b);

        /*
         * Check various preconditions.
         */
#ifdef VBOX_STRICT
        Assert(pCtx->eflags.Bits.u1VM || (pCtx->ss.Sel & X86_SEL_RPL) == 3 || (pCtx->ss.Sel & X86_SEL_RPL) == 0
               || (EMIsRawRing1Enabled(pVM) && (pCtx->ss.Sel & X86_SEL_RPL) == 1));
        AssertMsg(   (pCtx->eflags.u32 & X86_EFL_IF)
                  || PATMShouldUseRawMode(pVM, (RTGCPTR)pCtx->eip),
                  ("Tried to execute code with IF at EIP=%08x!\n", pCtx->eip));
        if (    !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL)
            &&  PGMMapHasConflicts(pVM))
        {
            PGMMapCheck(pVM);
            AssertMsgFailed(("We should not get conflicts any longer!!!\n"));
            return VERR_EM_UNEXPECTED_MAPPING_CONFLICT;
        }
#endif /* VBOX_STRICT */

        /*
         * Process high priority pre-execution raw-mode FFs.
         */
        if (    VM_FF_IS_PENDING(pVM, VM_FF_HIGH_PRIORITY_PRE_RAW_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK))
        {
            rc = emR3RawForcedActions(pVM, pVCpu, pCtx);
            VBOXVMM_EM_FF_RAW_RET(pVCpu, rc);
            if (rc != VINF_SUCCESS)
                break;
        }

        /*
         * If we're going to execute ring-0 code, the guest state needs to
         * be modified a bit and some of the state components (IF, SS/CS RPL,
         * and perhaps EIP) needs to be stored with PATM.
         */
        rc = CPUMRawEnter(pVCpu);
        if (rc != VINF_SUCCESS)
        {
            STAM_PROFILE_ADV_STOP(&pVCpu->em.s.StatRAWEntry, b);
            break;
        }

        /*
         * Scan code before executing it. Don't bother with user mode or V86 code
         */
        if (    (pCtx->ss.Sel & X86_SEL_RPL) <= 1
            &&  !pCtx->eflags.Bits.u1VM
            && !PATMIsPatchGCAddr(pVM, pCtx->eip))
        {
            STAM_PROFILE_ADV_SUSPEND(&pVCpu->em.s.StatRAWEntry, b);
            CSAMR3CheckCodeEx(pVM, pCtx, pCtx->eip);
            STAM_PROFILE_ADV_RESUME(&pVCpu->em.s.StatRAWEntry, b);
            if (    VM_FF_IS_PENDING(pVM, VM_FF_HIGH_PRIORITY_PRE_RAW_MASK)
                ||  VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK))
            {
                rc = emR3RawForcedActions(pVM, pVCpu, pCtx);
                VBOXVMM_EM_FF_RAW_RET(pVCpu, rc);
                if (rc != VINF_SUCCESS)
                {
                    rc = CPUMRawLeave(pVCpu, rc);
                    break;
                }
            }
        }

#ifdef LOG_ENABLED
        /*
         * Log important stuff before entering GC.
         */
        PPATMGCSTATE pGCState = PATMR3QueryGCStateHC(pVM);
        if (pCtx->eflags.Bits.u1VM)
            Log(("RV86: %04x:%08x IF=%d VMFlags=%x\n", pCtx->cs.Sel, pCtx->eip, pCtx->eflags.Bits.u1IF, pGCState->uVMFlags));
        else if ((pCtx->ss.Sel & X86_SEL_RPL) == 1)
            Log(("RR0: %x:%08x ESP=%x:%08x EFL=%x IF=%d/%d VMFlags=%x PIF=%d CPL=%d (Scanned=%d)\n",
                 pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, CPUMRawGetEFlags(pVCpu), !!(pGCState->uVMFlags & X86_EFL_IF), pCtx->eflags.Bits.u1IF,
                 pGCState->uVMFlags, pGCState->fPIF, (pCtx->ss.Sel & X86_SEL_RPL), CSAMIsPageScanned(pVM, (RTGCPTR)pCtx->eip)));
# ifdef VBOX_WITH_RAW_RING1
        else if ((pCtx->ss.Sel & X86_SEL_RPL) == 2)
            Log(("RR1: %x:%08x ESP=%x:%08x IF=%d VMFlags=%x CPL=%x\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, pCtx->eflags.Bits.u1IF, pGCState->uVMFlags, (pCtx->ss.Sel & X86_SEL_RPL)));
# endif
        else if ((pCtx->ss.Sel & X86_SEL_RPL) == 3)
            Log(("RR3: %x:%08x ESP=%x:%08x IF=%d VMFlags=%x\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel, pCtx->esp, pCtx->eflags.Bits.u1IF, pGCState->uVMFlags));
#endif /* LOG_ENABLED */



        /*
         * Execute the code.
         */
        STAM_PROFILE_ADV_STOP(&pVCpu->em.s.StatRAWEntry, b);
        if (RT_LIKELY(emR3IsExecutionAllowed(pVM, pVCpu)))
        {
            STAM_PROFILE_START(&pVCpu->em.s.StatRAWExec, c);
            VBOXVMM_EM_RAW_RUN_PRE(pVCpu, pCtx);
            rc = VMMR3RawRunGC(pVM, pVCpu);
            VBOXVMM_EM_RAW_RUN_RET(pVCpu, pCtx, rc);
            STAM_PROFILE_STOP(&pVCpu->em.s.StatRAWExec, c);
        }
        else
        {
            /* Give up this time slice; virtual time continues */
            STAM_REL_PROFILE_ADV_START(&pVCpu->em.s.StatCapped, u);
            RTThreadSleep(5);
            STAM_REL_PROFILE_ADV_STOP(&pVCpu->em.s.StatCapped, u);
            rc = VINF_SUCCESS;
        }
        STAM_PROFILE_ADV_START(&pVCpu->em.s.StatRAWTail, d);

        LogFlow(("RR%u-E: %08x ESP=%08x EFL=%x IF=%d/%d VMFlags=%x PIF=%d\n",
                 (pCtx->ss.Sel & X86_SEL_RPL), pCtx->eip, pCtx->esp, CPUMRawGetEFlags(pVCpu),
                 !!(pGCState->uVMFlags & X86_EFL_IF), pCtx->eflags.Bits.u1IF, pGCState->uVMFlags, pGCState->fPIF));
        LogFlow(("VMMR3RawRunGC returned %Rrc\n", rc));



        /*
         * Restore the real CPU state and deal with high priority post
         * execution FFs before doing anything else.
         */
        rc = CPUMRawLeave(pVCpu, rc);
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_RESUME_GUEST_MASK);
        if (    VM_FF_IS_PENDING(pVM, VM_FF_HIGH_PRIORITY_POST_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HIGH_PRIORITY_POST_MASK))
            rc = emR3HighPriorityPostForcedActions(pVM, pVCpu, rc);

#ifdef VBOX_STRICT
        /*
         * Assert TSS consistency & rc vs patch code.
         */
        if (   !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_TSS | VMCPU_FF_SELM_SYNC_GDT) /* GDT implies TSS at the moment. */
            &&  EMIsRawRing0Enabled(pVM))
            SELMR3CheckTSS(pVM);
        switch (rc)
        {
            case VINF_SUCCESS:
            case VINF_EM_RAW_INTERRUPT:
            case VINF_PATM_PATCH_TRAP_PF:
            case VINF_PATM_PATCH_TRAP_GP:
            case VINF_PATM_PATCH_INT3:
            case VINF_PATM_CHECK_PATCH_PAGE:
            case VINF_EM_RAW_EXCEPTION_PRIVILEGED:
            case VINF_EM_RAW_GUEST_TRAP:
            case VINF_EM_RESCHEDULE_RAW:
                break;

            default:
                if (PATMIsPatchGCAddr(pVM, pCtx->eip) && !(pCtx->eflags.u32 & X86_EFL_TF))
                    LogIt(0, LOG_GROUP_PATM, ("Patch code interrupted at %RRv for reason %Rrc\n", (RTRCPTR)CPUMGetGuestEIP(pVCpu), rc));
                break;
        }
        /*
         * Let's go paranoid!
         */
        if (    !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL)
            &&  PGMMapHasConflicts(pVM))
        {
            PGMMapCheck(pVM);
            AssertMsgFailed(("We should not get conflicts any longer!!! rc=%Rrc\n", rc));
            return VERR_EM_UNEXPECTED_MAPPING_CONFLICT;
        }
#endif /* VBOX_STRICT */

        /*
         * Process the returned status code.
         */
        if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
        {
            STAM_PROFILE_ADV_STOP(&pVCpu->em.s.StatRAWTail, d);
            break;
        }
        rc = emR3RawHandleRC(pVM, pVCpu, pCtx, rc);
        if (rc != VINF_SUCCESS)
        {
            rc = emR3RawUpdateForceFlag(pVM, pVCpu, pCtx, rc);
            if (rc != VINF_SUCCESS)
            {
                STAM_PROFILE_ADV_STOP(&pVCpu->em.s.StatRAWTail, d);
                break;
            }
        }

        /*
         * Check and execute forced actions.
         */
#ifdef VBOX_HIGH_RES_TIMERS_HACK
        TMTimerPollVoid(pVM, pVCpu);
#endif
        STAM_PROFILE_ADV_STOP(&pVCpu->em.s.StatRAWTail, d);
        if (    VM_FF_IS_PENDING(pVM, ~VM_FF_HIGH_PRIORITY_PRE_RAW_MASK | VM_FF_PGM_NO_MEMORY)
            ||  VMCPU_FF_IS_PENDING(pVCpu, ~VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK))
        {
            Assert(pCtx->eflags.Bits.u1VM || (pCtx->ss.Sel & X86_SEL_RPL) != (EMIsRawRing1Enabled(pVM) ? 2U : 1U));

            STAM_REL_PROFILE_ADV_SUSPEND(&pVCpu->em.s.StatRAWTotal, a);
            rc = emR3ForcedActions(pVM, pVCpu, rc);
            VBOXVMM_EM_FF_ALL_RET(pVCpu, rc);
            STAM_REL_PROFILE_ADV_RESUME(&pVCpu->em.s.StatRAWTotal, a);
            if (    rc != VINF_SUCCESS
                &&  rc != VINF_EM_RESCHEDULE_RAW)
            {
                rc = emR3RawUpdateForceFlag(pVM, pVCpu, pCtx, rc);
                if (rc != VINF_SUCCESS)
                {
                    *pfFFDone = true;
                    break;
                }
            }
        }
    }

    /*
     * Return to outer loop.
     */
#if defined(LOG_ENABLED) && defined(DEBUG)
    RTLogFlush(NULL);
#endif
    STAM_REL_PROFILE_ADV_STOP(&pVCpu->em.s.StatRAWTotal, a);
    return rc;
}


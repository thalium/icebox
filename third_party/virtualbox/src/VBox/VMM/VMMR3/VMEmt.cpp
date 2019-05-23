/* $Id: VMEmt.cpp $ */
/** @file
 * VM - Virtual Machine, The Emulation Thread.
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
#define LOG_GROUP LOG_GROUP_VM
#include <VBox/vmm/tm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/pdmapi.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/tm.h>
#include "VMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>

#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-math.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/time.h>

/*MYCODE*/
#include <VBox/vmm/iem.h>
#include <FDP/include/FDP.h>
#include <FDP/include/FDP_structs.h>
#include <iprt/spinlock.h>
/*ENDMYCODE*/

/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
int vmR3EmulationThreadWithId(RTTHREAD hThreadSelf, PUVMCPU pUVCpu, VMCPUID idCpu);


/**
 * The emulation thread main function.
 *
 * @returns Thread exit code.
 * @param   hThreadSelf The handle to the executing thread.
 * @param   pvArgs      Pointer to the user mode per-VCpu structure (UVMPCU).
 */
DECLCALLBACK(int) vmR3EmulationThread(RTTHREAD hThreadSelf, void *pvArgs)
{
    PUVMCPU pUVCpu = (PUVMCPU)pvArgs;
    return vmR3EmulationThreadWithId(hThreadSelf, pUVCpu, pUVCpu->idCpu);
}


/**
 * The emulation thread main function, with Virtual CPU ID for debugging.
 *
 * @returns Thread exit code.
 * @param   hThreadSelf The handle to the executing thread.
 * @param   pUVCpu      Pointer to the user mode per-VCpu structure.
 * @param   idCpu       The virtual CPU ID, for backtrace purposes.
 */
int vmR3EmulationThreadWithId(RTTHREAD hThreadSelf, PUVMCPU pUVCpu, VMCPUID idCpu)
{
    PUVM    pUVM = pUVCpu->pUVM;
    int     rc;
    RT_NOREF_PV(hThreadSelf);

    AssertReleaseMsg(VALID_PTR(pUVM) && pUVM->u32Magic == UVM_MAGIC,
                     ("Invalid arguments to the emulation thread!\n"));

    rc = RTTlsSet(pUVM->vm.s.idxTLS, pUVCpu);
    AssertReleaseMsgRCReturn(rc, ("RTTlsSet %x failed with %Rrc\n", pUVM->vm.s.idxTLS, rc), rc);

    if (   pUVM->pVmm2UserMethods
        && pUVM->pVmm2UserMethods->pfnNotifyEmtInit)
        pUVM->pVmm2UserMethods->pfnNotifyEmtInit(pUVM->pVmm2UserMethods, pUVM, pUVCpu);

    /*
     * The request loop.
     */
    rc = VINF_SUCCESS;
    Log(("vmR3EmulationThread: Emulation thread starting the days work... Thread=%#x pUVM=%p\n", hThreadSelf, pUVM));
    VMSTATE enmBefore = VMSTATE_CREATED; /* (only used for logging atm.) */
    ASMAtomicIncU32(&pUVM->vm.s.cActiveEmts);
    for (;;)
    {
        /*
         * During early init there is no pVM and/or pVCpu, so make a special path
         * for that to keep things clearly separate.
         */
        PVM    pVM   = pUVM->pVM;
        PVMCPU pVCpu = pUVCpu->pVCpu;
        if (!pVCpu || !pVM)
        {
            /*
             * Check for termination first.
             */
            if (pUVM->vm.s.fTerminateEMT)
            {
                rc = VINF_EM_TERMINATE;
                break;
            }

            /*
             * Only the first VCPU may initialize the VM during early init
             * and must therefore service all VMCPUID_ANY requests.
             * See also VMR3Create
             */
            if (    (pUVM->vm.s.pNormalReqs || pUVM->vm.s.pPriorityReqs)
                &&  pUVCpu->idCpu == 0)
            {
                /*
                 * Service execute in any EMT request.
                 */
                rc = VMR3ReqProcessU(pUVM, VMCPUID_ANY, false /*fPriorityOnly*/);
                Log(("vmR3EmulationThread: Req rc=%Rrc, VM state %s -> %s\n", rc, VMR3GetStateName(enmBefore), pUVM->pVM ? VMR3GetStateName(pUVM->pVM->enmVMState) : "CREATING"));
            }
            else if (pUVCpu->vm.s.pNormalReqs || pUVCpu->vm.s.pPriorityReqs)
            {
                /*
                 * Service execute in specific EMT request.
                 */
                rc = VMR3ReqProcessU(pUVM, pUVCpu->idCpu, false /*fPriorityOnly*/);
                Log(("vmR3EmulationThread: Req (cpu=%u) rc=%Rrc, VM state %s -> %s\n", pUVCpu->idCpu, rc, VMR3GetStateName(enmBefore), pUVM->pVM ? VMR3GetStateName(pUVM->pVM->enmVMState) : "CREATING"));
            }
            else
            {
                /*
                 * Nothing important is pending, so wait for something.
                 */
                rc = VMR3WaitU(pUVCpu);
                if (RT_FAILURE(rc))
                {
                    AssertLogRelMsgFailed(("VMR3WaitU failed with %Rrc\n", rc));
                    break;
                }
            }
        }
        else
        {
            /*
             * Pending requests which needs servicing?
             *
             * We check for state changes in addition to status codes when
             * servicing requests. (Look after the ifs.)
             */
            enmBefore = pVM->enmVMState;
            if (pUVM->vm.s.fTerminateEMT)
            {
                rc = VINF_EM_TERMINATE;
                break;
            }

            if (VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS))
            {
                rc = VMMR3EmtRendezvousFF(pVM, &pVM->aCpus[idCpu]);
                Log(("vmR3EmulationThread: Rendezvous rc=%Rrc, VM state %s -> %s\n", rc, VMR3GetStateName(enmBefore), VMR3GetStateName(pVM->enmVMState)));
            }
            else if (pUVM->vm.s.pNormalReqs || pUVM->vm.s.pPriorityReqs)
            {
                /*
                 * Service execute in any EMT request.
                 */
                rc = VMR3ReqProcessU(pUVM, VMCPUID_ANY, false /*fPriorityOnly*/);
                Log(("vmR3EmulationThread: Req rc=%Rrc, VM state %s -> %s\n", rc, VMR3GetStateName(enmBefore), VMR3GetStateName(pVM->enmVMState)));
            }
            else if (pUVCpu->vm.s.pNormalReqs || pUVCpu->vm.s.pPriorityReqs)
            {
                /*
                 * Service execute in specific EMT request.
                 */
                rc = VMR3ReqProcessU(pUVM, pUVCpu->idCpu, false /*fPriorityOnly*/);
                Log(("vmR3EmulationThread: Req (cpu=%u) rc=%Rrc, VM state %s -> %s\n", pUVCpu->idCpu, rc, VMR3GetStateName(enmBefore), VMR3GetStateName(pVM->enmVMState)));
            }
            else if (   VM_FF_IS_SET(pVM, VM_FF_DBGF)
                     || VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_DBGF))
            {
                /*
                 * Service the debugger request.
                 */
                rc = DBGFR3VMMForcedAction(pVM, pVCpu);
                Log(("vmR3EmulationThread: Dbg rc=%Rrc, VM state %s -> %s\n", rc, VMR3GetStateName(enmBefore), VMR3GetStateName(pVM->enmVMState)));
            }
            else if (VM_FF_TEST_AND_CLEAR(pVM, VM_FF_RESET))
            {
                /*
                 * Service a delayed reset request.
                 */
                rc = VBOXSTRICTRC_VAL(VMR3ResetFF(pVM));
                VM_FF_CLEAR(pVM, VM_FF_RESET);
                Log(("vmR3EmulationThread: Reset rc=%Rrc, VM state %s -> %s\n", rc, VMR3GetStateName(enmBefore), VMR3GetStateName(pVM->enmVMState)));
            }
            else
            {
                /*
                 * Nothing important is pending, so wait for something.
                 */
                rc = VMR3WaitU(pUVCpu);
                if (RT_FAILURE(rc))
                {
                    AssertLogRelMsgFailed(("VMR3WaitU failed with %Rrc\n", rc));
                    break;
                }
            }

            /*
             * Check for termination requests, these have extremely high priority.
             */
            if (    rc == VINF_EM_TERMINATE
                ||  pUVM->vm.s.fTerminateEMT)
                break;
        }

        /*
         * Some requests (both VMR3Req* and the DBGF) can potentially resume
         * or start the VM, in that case we'll get a change in VM status
         * indicating that we're now running.
         */
        if (RT_SUCCESS(rc))
        {
            pVM = pUVM->pVM;
            if (pVM)
            {
                pVCpu = &pVM->aCpus[idCpu];
                if (   pVM->enmVMState == VMSTATE_RUNNING
                    && VMCPUSTATE_IS_STARTED(VMCPU_GET_STATE(pVCpu)))
                {
                    rc = EMR3ExecuteVM(pVM, pVCpu);
                    Log(("vmR3EmulationThread: EMR3ExecuteVM() -> rc=%Rrc, enmVMState=%d\n", rc, pVM->enmVMState));
                }
            }
        }

    } /* forever */


    /*
     * Decrement the active EMT count if we haven't done it yet in vmR3Destroy.
     */
    if (!pUVCpu->vm.s.fBeenThruVmDestroy)
        ASMAtomicDecU32(&pUVM->vm.s.cActiveEmts);


    /*
     * Cleanup and exit.
     * EMT0 does the VM destruction after all other EMTs have deregistered and terminated.
     */
    Log(("vmR3EmulationThread: Terminating emulation thread! Thread=%#x pUVM=%p rc=%Rrc enmBefore=%d enmVMState=%d\n",
         hThreadSelf, pUVM, rc, enmBefore, pUVM->pVM ? pUVM->pVM->enmVMState : VMSTATE_TERMINATED));
    PVM pVM;
    if (   idCpu == 0
        && (pVM = pUVM->pVM) != NULL)
    {
        /* Wait for any other EMTs to terminate before we destroy the VM (see vmR3DestroyVM). */
        for (VMCPUID iCpu = 1; iCpu < pUVM->cCpus; iCpu++)
        {
            RTTHREAD hThread;
            ASMAtomicXchgHandle(&pUVM->aCpus[iCpu].vm.s.ThreadEMT, NIL_RTTHREAD, &hThread);
            if (hThread != NIL_RTTHREAD)
            {
                int rc2 = RTThreadWait(hThread, 5 * RT_MS_1SEC, NULL);
                AssertLogRelMsgRC(rc2, ("iCpu=%u rc=%Rrc\n", iCpu, rc2));
                if (RT_FAILURE(rc2))
                    pUVM->aCpus[iCpu].vm.s.ThreadEMT = hThread;
            }
        }

        /* Switch to the terminated state, clearing the VM pointer and finally destroy the VM. */
        vmR3SetTerminated(pVM);

        pUVM->pVM = NULL;

        int rc2 = SUPR3CallVMMR0Ex(pVM->pVMR0, 0 /*idCpu*/, VMMR0_DO_GVMM_DESTROY_VM, 0, NULL);
        AssertLogRelRC(rc2);
    }
    /* Deregister the EMT with VMMR0. */
    else if (   idCpu != 0
             && (pVM = pUVM->pVM) != NULL)
    {
        int rc2 = SUPR3CallVMMR0Ex(pVM->pVMR0, idCpu, VMMR0_DO_GVMM_DEREGISTER_VMCPU, 0, NULL);
        AssertLogRelRC(rc2);
    }

    if (   pUVM->pVmm2UserMethods
        && pUVM->pVmm2UserMethods->pfnNotifyEmtTerm)
        pUVM->pVmm2UserMethods->pfnNotifyEmtTerm(pUVM->pVmm2UserMethods, pUVM, pUVCpu);

    pUVCpu->vm.s.NativeThreadEMT = NIL_RTNATIVETHREAD;
    Log(("vmR3EmulationThread: EMT is terminated.\n"));
    return rc;
}


/**
 * Gets the name of a halt method.
 *
 * @returns Pointer to a read only string.
 * @param   enmMethod   The method.
 */
static const char *vmR3GetHaltMethodName(VMHALTMETHOD enmMethod)
{
    switch (enmMethod)
    {
        case VMHALTMETHOD_BOOTSTRAP:    return "bootstrap";
        case VMHALTMETHOD_DEFAULT:      return "default";
        case VMHALTMETHOD_OLD:          return "old";
        case VMHALTMETHOD_1:            return "method1";
        //case VMHALTMETHOD_2:            return "method2";
        case VMHALTMETHOD_GLOBAL_1:     return "global1";
        default:                        return "unknown";
    }
}


/**
 * Signal a fatal wait error.
 *
 * @returns Fatal error code to be propagated up the call stack.
 * @param   pUVCpu              The user mode per CPU structure of the calling
 *                              EMT.
 * @param   pszFmt              The error format with a single %Rrc in it.
 * @param   rcFmt               The status code to format.
 */
static int vmR3FatalWaitError(PUVMCPU pUVCpu, const char *pszFmt, int rcFmt)
{
    /** @todo This is wrong ... raise a fatal error / guru meditation
     *        instead. */
    AssertLogRelMsgFailed((pszFmt, rcFmt));
    ASMAtomicUoWriteBool(&pUVCpu->pUVM->vm.s.fTerminateEMT, true);
    if (pUVCpu->pVM)
        VM_FF_SET(pUVCpu->pVM, VM_FF_CHECK_VM_STATE);
    return VERR_VM_FATAL_WAIT_ERROR;
}


/**
 * The old halt loop.
 */
static DECLCALLBACK(int) vmR3HaltOldDoHalt(PUVMCPU pUVCpu, const uint32_t fMask, uint64_t /* u64Now*/)
{
    /*
     * Halt loop.
     */
    PVM    pVM   = pUVCpu->pVM;
    PVMCPU pVCpu = pUVCpu->pVCpu;

    int rc = VINF_SUCCESS;
    ASMAtomicWriteBool(&pUVCpu->vm.s.fWait, true);
    //unsigned cLoops = 0;
    for (;;)
    {
        /*
         * Work the timers and check if we can exit.
         * The poll call gives us the ticks left to the next event in
         * addition to perhaps set an FF.
         */
        uint64_t const u64StartTimers   = RTTimeNanoTS();
        TMR3TimerQueuesDo(pVM);
        uint64_t const cNsElapsedTimers = RTTimeNanoTS() - u64StartTimers;
        STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltTimers, cNsElapsedTimers);
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
            break;

        /*MYCODE*/
        if(pVCpu->mystate.s.bPauseRequired == true)
            break;
        /*ENDMYCODE*/

        uint64_t u64NanoTS;
        TMTimerPollGIP(pVM, pVCpu, &u64NanoTS);
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
            break;

        /*MYCODE*/
        if(pVCpu->mystate.s.bPauseRequired == true)
            break;
        /*ENDMYCODE*/

        /*
         * Wait for a while. Someone will wake us up or interrupt the call if
         * anything needs our attention.
         */
        if (u64NanoTS < 50000)
        {
            //RTLogPrintf("u64NanoTS=%RI64 cLoops=%d spin\n", u64NanoTS, cLoops++);
            /* spin */;
        }
        else
        {
            VMMR3YieldStop(pVM);
            //uint64_t u64Start = RTTimeNanoTS();
            if (u64NanoTS <  870000) /* this is a bit speculative... works fine on linux. */
            {
                //RTLogPrintf("u64NanoTS=%RI64 cLoops=%d yield", u64NanoTS, cLoops++);
                uint64_t const u64StartSchedYield   = RTTimeNanoTS();
                RTThreadYield(); /* this is the best we can do here */
                uint64_t const cNsElapsedSchedYield = RTTimeNanoTS() - u64StartSchedYield;
                STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltYield, cNsElapsedSchedYield);
            }
            else if (u64NanoTS < 2000000)
            {
                //RTLogPrintf("u64NanoTS=%RI64 cLoops=%d sleep 1ms", u64NanoTS, cLoops++);
                uint64_t const u64StartSchedHalt   = RTTimeNanoTS();
                rc = RTSemEventWait(pUVCpu->vm.s.EventSemWait, 1);
                uint64_t const cNsElapsedSchedHalt = RTTimeNanoTS() - u64StartSchedHalt;
                STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltBlock, cNsElapsedSchedHalt);
            }
            else
            {
                //RTLogPrintf("u64NanoTS=%RI64 cLoops=%d sleep %dms", u64NanoTS, cLoops++, (uint32_t)RT_MIN((u64NanoTS - 500000) / 1000000, 15));
                uint64_t const u64StartSchedHalt   = RTTimeNanoTS();
                rc = RTSemEventWait(pUVCpu->vm.s.EventSemWait, RT_MIN((u64NanoTS - 1000000) / 1000000, 15));
                uint64_t const cNsElapsedSchedHalt = RTTimeNanoTS() - u64StartSchedHalt;
                STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltBlock, cNsElapsedSchedHalt);
            }
            //uint64_t u64Slept = RTTimeNanoTS() - u64Start;
            //RTLogPrintf(" -> rc=%Rrc in %RU64 ns / %RI64 ns delta\n", rc, u64Slept, u64NanoTS - u64Slept);
        }
        if (rc == VERR_TIMEOUT)
            rc = VINF_SUCCESS;
        else if (RT_FAILURE(rc))
        {
            rc = vmR3FatalWaitError(pUVCpu, "RTSemEventWait->%Rrc\n", rc);
            break;
        }
    }

    ASMAtomicUoWriteBool(&pUVCpu->vm.s.fWait, false);
    return rc;
}


/**
 * Initialize the configuration of halt method 1 & 2.
 *
 * @return VBox status code. Failure on invalid CFGM data.
 * @param   pUVM        The user mode VM structure.
 */
static int vmR3HaltMethod12ReadConfigU(PUVM pUVM)
{
    /*
     * The defaults.
     */
#if 1 /* DEBUGGING STUFF - REMOVE LATER */
    pUVM->vm.s.Halt.Method12.u32LagBlockIntervalDivisorCfg = 4;
    pUVM->vm.s.Halt.Method12.u32MinBlockIntervalCfg =   2*1000000;
    pUVM->vm.s.Halt.Method12.u32MaxBlockIntervalCfg =  75*1000000;
    pUVM->vm.s.Halt.Method12.u32StartSpinningCfg    =  30*1000000;
    pUVM->vm.s.Halt.Method12.u32StopSpinningCfg     =  20*1000000;
#else
    pUVM->vm.s.Halt.Method12.u32LagBlockIntervalDivisorCfg = 4;
    pUVM->vm.s.Halt.Method12.u32MinBlockIntervalCfg =   5*1000000;
    pUVM->vm.s.Halt.Method12.u32MaxBlockIntervalCfg = 200*1000000;
    pUVM->vm.s.Halt.Method12.u32StartSpinningCfg    =  20*1000000;
    pUVM->vm.s.Halt.Method12.u32StopSpinningCfg     =   2*1000000;
#endif

    /*
     * Query overrides.
     *
     * I don't have time to bother with niceties such as invalid value checks
     * here right now. sorry.
     */
    PCFGMNODE pCfg = CFGMR3GetChild(CFGMR3GetRoot(pUVM->pVM), "/VMM/HaltedMethod1");
    if (pCfg)
    {
        uint32_t u32;
        if (RT_SUCCESS(CFGMR3QueryU32(pCfg, "LagBlockIntervalDivisor", &u32)))
            pUVM->vm.s.Halt.Method12.u32LagBlockIntervalDivisorCfg = u32;
        if (RT_SUCCESS(CFGMR3QueryU32(pCfg, "MinBlockInterval", &u32)))
            pUVM->vm.s.Halt.Method12.u32MinBlockIntervalCfg = u32;
        if (RT_SUCCESS(CFGMR3QueryU32(pCfg, "MaxBlockInterval", &u32)))
            pUVM->vm.s.Halt.Method12.u32MaxBlockIntervalCfg = u32;
        if (RT_SUCCESS(CFGMR3QueryU32(pCfg, "StartSpinning", &u32)))
            pUVM->vm.s.Halt.Method12.u32StartSpinningCfg = u32;
        if (RT_SUCCESS(CFGMR3QueryU32(pCfg, "StopSpinning", &u32)))
            pUVM->vm.s.Halt.Method12.u32StopSpinningCfg = u32;
        LogRel(("VMEmt: HaltedMethod1 config: %d/%d/%d/%d/%d\n",
                pUVM->vm.s.Halt.Method12.u32LagBlockIntervalDivisorCfg,
                pUVM->vm.s.Halt.Method12.u32MinBlockIntervalCfg,
                pUVM->vm.s.Halt.Method12.u32MaxBlockIntervalCfg,
                pUVM->vm.s.Halt.Method12.u32StartSpinningCfg,
                pUVM->vm.s.Halt.Method12.u32StopSpinningCfg));
    }

    return VINF_SUCCESS;
}


/**
 * Initialize halt method 1.
 *
 * @return VBox status code.
 * @param   pUVM            Pointer to the user mode VM structure.
 */
static DECLCALLBACK(int) vmR3HaltMethod1Init(PUVM pUVM)
{
    return vmR3HaltMethod12ReadConfigU(pUVM);
}


/**
 * Method 1 - Block whenever possible, and when lagging behind
 * switch to spinning for 10-30ms with occasional blocking until
 * the lag has been eliminated.
 */
static DECLCALLBACK(int) vmR3HaltMethod1Halt(PUVMCPU pUVCpu, const uint32_t fMask, uint64_t u64Now)
{
    PUVM    pUVM    = pUVCpu->pUVM;
    PVMCPU  pVCpu   = pUVCpu->pVCpu;
    PVM     pVM     = pUVCpu->pVM;

    /*
     * To simplify things, we decide up-front whether we should switch to spinning or
     * not. This makes some ASSUMPTIONS about the cause of the spinning (PIT/RTC/PCNet)
     * and that it will generate interrupts or other events that will cause us to exit
     * the halt loop.
     */
    bool fBlockOnce = false;
    bool fSpinning = false;
    uint32_t u32CatchUpPct = TMVirtualSyncGetCatchUpPct(pVM);
    if (u32CatchUpPct /* non-zero if catching up */)
    {
        if (pUVCpu->vm.s.Halt.Method12.u64StartSpinTS)
        {
            fSpinning = TMVirtualSyncGetLag(pVM) >= pUVM->vm.s.Halt.Method12.u32StopSpinningCfg;
            if (fSpinning)
            {
                uint64_t u64Lag = TMVirtualSyncGetLag(pVM);
                fBlockOnce = u64Now - pUVCpu->vm.s.Halt.Method12.u64LastBlockTS
                           > RT_MAX(pUVM->vm.s.Halt.Method12.u32MinBlockIntervalCfg,
                                    RT_MIN(u64Lag / pUVM->vm.s.Halt.Method12.u32LagBlockIntervalDivisorCfg,
                                           pUVM->vm.s.Halt.Method12.u32MaxBlockIntervalCfg));
            }
            else
            {
                //RTLogRelPrintf("Stopped spinning (%u ms)\n", (u64Now - pUVCpu->vm.s.Halt.Method12.u64StartSpinTS) / 1000000);
                pUVCpu->vm.s.Halt.Method12.u64StartSpinTS = 0;
            }
        }
        else
        {
            fSpinning = TMVirtualSyncGetLag(pVM) >= pUVM->vm.s.Halt.Method12.u32StartSpinningCfg;
            if (fSpinning)
                pUVCpu->vm.s.Halt.Method12.u64StartSpinTS = u64Now;
        }
    }
    else if (pUVCpu->vm.s.Halt.Method12.u64StartSpinTS)
    {
        //RTLogRelPrintf("Stopped spinning (%u ms)\n", (u64Now - pUVCpu->vm.s.Halt.Method12.u64StartSpinTS) / 1000000);
        pUVCpu->vm.s.Halt.Method12.u64StartSpinTS = 0;
    }

    /*
     * Halt loop.
     */
    int rc = VINF_SUCCESS;
    ASMAtomicWriteBool(&pUVCpu->vm.s.fWait, true);
    unsigned cLoops = 0;
    for (;; cLoops++)
    {
        /*
         * Work the timers and check if we can exit.
         */
        uint64_t const u64StartTimers   = RTTimeNanoTS();
        TMR3TimerQueuesDo(pVM);
        uint64_t const cNsElapsedTimers = RTTimeNanoTS() - u64StartTimers;
        STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltTimers, cNsElapsedTimers);
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
            break;

        /*
         * Estimate time left to the next event.
         */
        uint64_t u64NanoTS;
        TMTimerPollGIP(pVM, pVCpu, &u64NanoTS);
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
            break;

        /*
         * Block if we're not spinning and the interval isn't all that small.
         */
        if (    (   !fSpinning
                 || fBlockOnce)
#if 1 /* DEBUGGING STUFF - REMOVE LATER */
            &&  u64NanoTS >= 100000) /* 0.100 ms */
#else
            &&  u64NanoTS >= 250000) /* 0.250 ms */
#endif
        {
            const uint64_t Start = pUVCpu->vm.s.Halt.Method12.u64LastBlockTS = RTTimeNanoTS();
            VMMR3YieldStop(pVM);

            uint32_t cMilliSecs = RT_MIN(u64NanoTS / 1000000, 15);
            if (cMilliSecs <= pUVCpu->vm.s.Halt.Method12.cNSBlockedTooLongAvg)
                cMilliSecs = 1;
            else
                cMilliSecs -= pUVCpu->vm.s.Halt.Method12.cNSBlockedTooLongAvg;

            //RTLogRelPrintf("u64NanoTS=%RI64 cLoops=%3d sleep %02dms (%7RU64) ", u64NanoTS, cLoops, cMilliSecs, u64NanoTS);
            uint64_t const u64StartSchedHalt   = RTTimeNanoTS();
            rc = RTSemEventWait(pUVCpu->vm.s.EventSemWait, cMilliSecs);
            uint64_t const cNsElapsedSchedHalt = RTTimeNanoTS() - u64StartSchedHalt;
            STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltBlock, cNsElapsedSchedHalt);

            if (rc == VERR_TIMEOUT)
                rc = VINF_SUCCESS;
            else if (RT_FAILURE(rc))
            {
                rc = vmR3FatalWaitError(pUVCpu, "RTSemEventWait->%Rrc\n", rc);
                break;
            }

            /*
             * Calc the statistics.
             * Update averages every 16th time, and flush parts of the history every 64th time.
             */
            const uint64_t Elapsed = RTTimeNanoTS() - Start;
            pUVCpu->vm.s.Halt.Method12.cNSBlocked += Elapsed;
            if (Elapsed > u64NanoTS)
                pUVCpu->vm.s.Halt.Method12.cNSBlockedTooLong += Elapsed - u64NanoTS;
            pUVCpu->vm.s.Halt.Method12.cBlocks++;
            if (!(pUVCpu->vm.s.Halt.Method12.cBlocks & 0xf))
            {
                pUVCpu->vm.s.Halt.Method12.cNSBlockedTooLongAvg = pUVCpu->vm.s.Halt.Method12.cNSBlockedTooLong / pUVCpu->vm.s.Halt.Method12.cBlocks;
                if (!(pUVCpu->vm.s.Halt.Method12.cBlocks & 0x3f))
                {
                    pUVCpu->vm.s.Halt.Method12.cNSBlockedTooLong = pUVCpu->vm.s.Halt.Method12.cNSBlockedTooLongAvg * 0x40;
                    pUVCpu->vm.s.Halt.Method12.cBlocks = 0x40;
                }
            }
            //RTLogRelPrintf(" -> %7RU64 ns / %7RI64 ns delta%s\n", Elapsed, Elapsed - u64NanoTS, fBlockOnce ? " (block once)" : "");

            /*
             * Clear the block once flag if we actually blocked.
             */
            if (    fBlockOnce
                &&  Elapsed > 100000 /* 0.1 ms */)
                fBlockOnce = false;
        }
    }
    //if (fSpinning) RTLogRelPrintf("spun for %RU64 ns %u loops; lag=%RU64 pct=%d\n", RTTimeNanoTS() - u64Now, cLoops, TMVirtualSyncGetLag(pVM), u32CatchUpPct);

    ASMAtomicUoWriteBool(&pUVCpu->vm.s.fWait, false);
    return rc;
}


/**
 * Initialize the global 1 halt method.
 *
 * @return VBox status code.
 * @param   pUVM            Pointer to the user mode VM structure.
 */
static DECLCALLBACK(int) vmR3HaltGlobal1Init(PUVM pUVM)
{
    /*
     * The defaults.
     */
    uint32_t cNsResolution = SUPSemEventMultiGetResolution(pUVM->vm.s.pSession);
    if (cNsResolution > 5*RT_NS_100US)
        pUVM->vm.s.Halt.Global1.cNsSpinBlockThresholdCfg = 50000;
    else if (cNsResolution > RT_NS_100US)
        pUVM->vm.s.Halt.Global1.cNsSpinBlockThresholdCfg = cNsResolution / 4;
    else
        pUVM->vm.s.Halt.Global1.cNsSpinBlockThresholdCfg = 2000;

    /*
     * Query overrides.
     *
     * I don't have time to bother with niceties such as invalid value checks
     * here right now. sorry.
     */
    PCFGMNODE pCfg = CFGMR3GetChild(CFGMR3GetRoot(pUVM->pVM), "/VMM/HaltedGlobal1");
    if (pCfg)
    {
        uint32_t u32;
        if (RT_SUCCESS(CFGMR3QueryU32(pCfg, "SpinBlockThreshold", &u32)))
            pUVM->vm.s.Halt.Global1.cNsSpinBlockThresholdCfg = u32;
    }
    LogRel(("VMEmt: HaltedGlobal1 config: cNsSpinBlockThresholdCfg=%u\n",
            pUVM->vm.s.Halt.Global1.cNsSpinBlockThresholdCfg));
    return VINF_SUCCESS;
}


/**
 * The global 1 halt method - Block in GMM (ring-0) and let it
 * try take care of the global scheduling of EMT threads.
 */
static DECLCALLBACK(int) vmR3HaltGlobal1Halt(PUVMCPU pUVCpu, const uint32_t fMask, uint64_t u64Now)
{
    PUVM    pUVM  = pUVCpu->pUVM;
    PVMCPU  pVCpu = pUVCpu->pVCpu;
    PVM     pVM   = pUVCpu->pVM;
    Assert(VMMGetCpu(pVM) == pVCpu);
    NOREF(u64Now);

    /*
     * Halt loop.
     */
    //uint64_t u64NowLog, u64Start;
    //u64Start = u64NowLog = RTTimeNanoTS();
    int rc = VINF_SUCCESS;
    ASMAtomicWriteBool(&pUVCpu->vm.s.fWait, true);
    unsigned cLoops = 0;
    for (;; cLoops++)
    {
        /*
         * Work the timers and check if we can exit.
         */
        uint64_t const u64StartTimers   = RTTimeNanoTS();
        TMR3TimerQueuesDo(pVM);
        uint64_t const cNsElapsedTimers = RTTimeNanoTS() - u64StartTimers;
        STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltTimers, cNsElapsedTimers);
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
            break;

        /*MYCODE*/
        if(pVCpu->mystate.s.bPauseRequired == true)
            break;
        /*ENDMYCODE*/

        /*
         * Estimate time left to the next event.
         */
        //u64NowLog = RTTimeNanoTS();
        uint64_t u64Delta;
        uint64_t u64GipTime = TMTimerPollGIP(pVM, pVCpu, &u64Delta);
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
            break;

        /*MYCODE*/
        if(pVCpu->mystate.s.bPauseRequired == true)
            break;
        /*ENDMYCODE*/

        /*
         * Block if we're not spinning and the interval isn't all that small.
         */
        if (u64Delta >= pUVM->vm.s.Halt.Global1.cNsSpinBlockThresholdCfg)
        {
            VMMR3YieldStop(pVM);
            if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
                ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
                break;

            //RTLogPrintf("loop=%-3d  u64GipTime=%'llu / %'llu   now=%'llu / %'llu\n", cLoops, u64GipTime, u64Delta, u64NowLog, u64GipTime - u64NowLog);
            uint64_t const u64StartSchedHalt   = RTTimeNanoTS();
            rc = SUPR3CallVMMR0Ex(pVM->pVMR0, pVCpu->idCpu, VMMR0_DO_GVMM_SCHED_HALT, u64GipTime, NULL);
            uint64_t const u64EndSchedHalt     = RTTimeNanoTS();
            uint64_t const cNsElapsedSchedHalt = u64EndSchedHalt - u64StartSchedHalt;
            STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltBlock, cNsElapsedSchedHalt);

            if (rc == VERR_INTERRUPTED)
                rc = VINF_SUCCESS;
            else if (RT_FAILURE(rc))
            {
                rc = vmR3FatalWaitError(pUVCpu, "vmR3HaltGlobal1Halt: VMMR0_DO_GVMM_SCHED_HALT->%Rrc\n", rc);
                break;
            }
            else
            {
                int64_t const cNsOverslept = u64EndSchedHalt - u64GipTime;
                if (cNsOverslept > 50000)
                    STAM_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltBlockOverslept, cNsOverslept);
                else if (cNsOverslept < -50000)
                    STAM_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltBlockInsomnia,  cNsElapsedSchedHalt);
                else
                    STAM_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltBlockOnTime,    cNsElapsedSchedHalt);
            }
        }
        /*
         * When spinning call upon the GVMM and do some wakups once
         * in a while, it's not like we're actually busy or anything.
         */
        else if (!(cLoops & 0x1fff))
        {
            uint64_t const u64StartSchedYield   = RTTimeNanoTS();
            rc = SUPR3CallVMMR0Ex(pVM->pVMR0, pVCpu->idCpu, VMMR0_DO_GVMM_SCHED_POLL, false /* don't yield */, NULL);
            uint64_t const cNsElapsedSchedYield = RTTimeNanoTS() - u64StartSchedYield;
            STAM_REL_PROFILE_ADD_PERIOD(&pUVCpu->vm.s.StatHaltYield, cNsElapsedSchedYield);
        }
    }
    //RTLogPrintf("*** %u loops %'llu;  lag=%RU64\n", cLoops, u64NowLog - u64Start, TMVirtualSyncGetLag(pVM));

    ASMAtomicUoWriteBool(&pUVCpu->vm.s.fWait, false);
    return rc;
}


/**
 * The global 1 halt method - VMR3Wait() worker.
 *
 * @returns VBox status code.
 * @param   pUVCpu            Pointer to the user mode VMCPU structure.
 */
static DECLCALLBACK(int) vmR3HaltGlobal1Wait(PUVMCPU pUVCpu)
{
    ASMAtomicWriteBool(&pUVCpu->vm.s.fWait, true);

    PVM    pVM   = pUVCpu->pUVM->pVM;
    PVMCPU pVCpu = VMMGetCpu(pVM);
    Assert(pVCpu->idCpu == pUVCpu->idCpu);

    int rc = VINF_SUCCESS;
    for (;;)
    {
        /*
         * Check Relevant FFs.
         */
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_SUSPENDED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_EXTERNAL_SUSPENDED_MASK))
            break;

        /*
         * Wait for a while. Someone will wake us up or interrupt the call if
         * anything needs our attention.
         */
        rc = SUPR3CallVMMR0Ex(pVM->pVMR0, pVCpu->idCpu, VMMR0_DO_GVMM_SCHED_HALT, RTTimeNanoTS() + 1000000000 /* +1s */, NULL);
        if (rc == VERR_INTERRUPTED)
            rc = VINF_SUCCESS;
        else if (RT_FAILURE(rc))
        {
            rc = vmR3FatalWaitError(pUVCpu, "vmR3HaltGlobal1Wait: VMMR0_DO_GVMM_SCHED_HALT->%Rrc\n", rc);
            break;
        }
    }

    ASMAtomicUoWriteBool(&pUVCpu->vm.s.fWait, false);
    return rc;
}


/**
 * The global 1 halt method - VMR3NotifyFF() worker.
 *
 * @param   pUVCpu          Pointer to the user mode VMCPU structure.
 * @param   fFlags          Notification flags, VMNOTIFYFF_FLAGS_*.
 */
static DECLCALLBACK(void) vmR3HaltGlobal1NotifyCpuFF(PUVMCPU pUVCpu, uint32_t fFlags)
{
    if (pUVCpu->vm.s.fWait)
    {
        int rc = SUPR3CallVMMR0Ex(pUVCpu->pVM->pVMR0, pUVCpu->idCpu, VMMR0_DO_GVMM_SCHED_WAKE_UP, 0, NULL);
        AssertRC(rc);
    }
    else if (   (   (fFlags & VMNOTIFYFF_FLAGS_POKE)
                 || !(fFlags & VMNOTIFYFF_FLAGS_DONE_REM))
             && pUVCpu->pVCpu)
    {
        VMCPUSTATE enmState = VMCPU_GET_STATE(pUVCpu->pVCpu);
        if (enmState == VMCPUSTATE_STARTED_EXEC)
        {
            if (fFlags & VMNOTIFYFF_FLAGS_POKE)
            {
                int rc = SUPR3CallVMMR0Ex(pUVCpu->pVM->pVMR0, pUVCpu->idCpu, VMMR0_DO_GVMM_SCHED_POKE, 0, NULL);
                AssertRC(rc);
            }
        }
#ifdef VBOX_WITH_REM
        else if (enmState == VMCPUSTATE_STARTED_EXEC_REM)
        {
            if (!(fFlags & VMNOTIFYFF_FLAGS_DONE_REM))
                REMR3NotifyFF(pUVCpu->pVM);
        }
#endif
    }
}


/**
 * Bootstrap VMR3Wait() worker.
 *
 * @returns VBox status code.
 * @param   pUVCpu      Pointer to the user mode VMCPU structure.
 */
static DECLCALLBACK(int) vmR3BootstrapWait(PUVMCPU pUVCpu)
{
    PUVM pUVM = pUVCpu->pUVM;

    ASMAtomicWriteBool(&pUVCpu->vm.s.fWait, true);

    int rc = VINF_SUCCESS;
    for (;;)
    {
        /*
         * Check Relevant FFs.
         */
        if (pUVM->vm.s.pNormalReqs   || pUVM->vm.s.pPriorityReqs)   /* global requests pending? */
            break;
        if (pUVCpu->vm.s.pNormalReqs || pUVCpu->vm.s.pPriorityReqs) /* local requests pending? */
            break;

        if (    pUVCpu->pVM
            &&  (   VM_FF_IS_PENDING(pUVCpu->pVM, VM_FF_EXTERNAL_SUSPENDED_MASK)
                 || VMCPU_FF_IS_PENDING(VMMGetCpu(pUVCpu->pVM), VMCPU_FF_EXTERNAL_SUSPENDED_MASK)
                )
            )
            break;
        if (pUVM->vm.s.fTerminateEMT)
            break;

        /*
         * Wait for a while. Someone will wake us up or interrupt the call if
         * anything needs our attention.
         */
        rc = RTSemEventWait(pUVCpu->vm.s.EventSemWait, 1000);
        if (rc == VERR_TIMEOUT)
            rc = VINF_SUCCESS;
        else if (RT_FAILURE(rc))
        {
            rc = vmR3FatalWaitError(pUVCpu, "RTSemEventWait->%Rrc\n", rc);
            break;
        }
    }

    ASMAtomicUoWriteBool(&pUVCpu->vm.s.fWait, false);
    return rc;
}


/**
 * Bootstrap VMR3NotifyFF() worker.
 *
 * @param   pUVCpu          Pointer to the user mode VMCPU structure.
 * @param   fFlags          Notification flags, VMNOTIFYFF_FLAGS_*.
 */
static DECLCALLBACK(void) vmR3BootstrapNotifyCpuFF(PUVMCPU pUVCpu, uint32_t fFlags)
{
    if (pUVCpu->vm.s.fWait)
    {
        int rc = RTSemEventSignal(pUVCpu->vm.s.EventSemWait);
        AssertRC(rc);
    }
    NOREF(fFlags);
}


/**
 * Default VMR3Wait() worker.
 *
 * @returns VBox status code.
 * @param   pUVCpu          Pointer to the user mode VMCPU structure.
 */
static DECLCALLBACK(int) vmR3DefaultWait(PUVMCPU pUVCpu)
{
    ASMAtomicWriteBool(&pUVCpu->vm.s.fWait, true);

    PVM    pVM   = pUVCpu->pVM;
    PVMCPU pVCpu = pUVCpu->pVCpu;
    int    rc    = VINF_SUCCESS;
    for (;;)
    {
        /*
         * Check Relevant FFs.
         */
        if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_SUSPENDED_MASK)
            ||  VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_EXTERNAL_SUSPENDED_MASK))
            break;

        /*
         * Wait for a while. Someone will wake us up or interrupt the call if
         * anything needs our attention.
         */
        rc = RTSemEventWait(pUVCpu->vm.s.EventSemWait, 1000);
        if (rc == VERR_TIMEOUT)
            rc = VINF_SUCCESS;
        else if (RT_FAILURE(rc))
        {
            rc = vmR3FatalWaitError(pUVCpu, "RTSemEventWait->%Rrc", rc);
            break;
        }
    }

    ASMAtomicUoWriteBool(&pUVCpu->vm.s.fWait, false);
    return rc;
}


/**
 * Default VMR3NotifyFF() worker.
 *
 * @param   pUVCpu          Pointer to the user mode VMCPU structure.
 * @param   fFlags          Notification flags, VMNOTIFYFF_FLAGS_*.
 */
static DECLCALLBACK(void) vmR3DefaultNotifyCpuFF(PUVMCPU pUVCpu, uint32_t fFlags)
{
    if (pUVCpu->vm.s.fWait)
    {
        int rc = RTSemEventSignal(pUVCpu->vm.s.EventSemWait);
        AssertRC(rc);
    }
#ifdef VBOX_WITH_REM
    else if (   !(fFlags & VMNOTIFYFF_FLAGS_DONE_REM)
             && pUVCpu->pVCpu
             && pUVCpu->pVCpu->enmState == VMCPUSTATE_STARTED_EXEC_REM)
        REMR3NotifyFF(pUVCpu->pVM);
#else
    RT_NOREF(fFlags);
#endif
}


/**
 * Array with halt method descriptors.
 * VMINT::iHaltMethod contains an index into this array.
 */
static const struct VMHALTMETHODDESC
{
    /** The halt method id. */
    VMHALTMETHOD enmHaltMethod;
    /** The init function for loading config and initialize variables. */
    DECLR3CALLBACKMEMBER(int,  pfnInit,(PUVM pUVM));
    /** The term function. */
    DECLR3CALLBACKMEMBER(void, pfnTerm,(PUVM pUVM));
    /** The VMR3WaitHaltedU function. */
    DECLR3CALLBACKMEMBER(int,  pfnHalt,(PUVMCPU pUVCpu, const uint32_t fMask, uint64_t u64Now));
    /** The VMR3WaitU function. */
    DECLR3CALLBACKMEMBER(int,  pfnWait,(PUVMCPU pUVCpu));
    /** The VMR3NotifyCpuFFU function. */
    DECLR3CALLBACKMEMBER(void, pfnNotifyCpuFF,(PUVMCPU pUVCpu, uint32_t fFlags));
    /** The VMR3NotifyGlobalFFU function. */
    DECLR3CALLBACKMEMBER(void, pfnNotifyGlobalFF,(PUVM pUVM, uint32_t fFlags));
} g_aHaltMethods[] =
{
    { VMHALTMETHOD_BOOTSTRAP, NULL,                NULL,   NULL,                vmR3BootstrapWait,   vmR3BootstrapNotifyCpuFF,   NULL },
    { VMHALTMETHOD_OLD,       NULL,                NULL,   vmR3HaltOldDoHalt,   vmR3DefaultWait,     vmR3DefaultNotifyCpuFF,     NULL },
    { VMHALTMETHOD_1,         vmR3HaltMethod1Init, NULL,   vmR3HaltMethod1Halt, vmR3DefaultWait,     vmR3DefaultNotifyCpuFF,     NULL },
    { VMHALTMETHOD_GLOBAL_1,  vmR3HaltGlobal1Init, NULL,   vmR3HaltGlobal1Halt, vmR3HaltGlobal1Wait, vmR3HaltGlobal1NotifyCpuFF, NULL },
};


/**
 * Notify the emulation thread (EMT) about pending Forced Action (FF).
 *
 * This function is called by thread other than EMT to make
 * sure EMT wakes up and promptly service an FF request.
 *
 * @param   pUVM            Pointer to the user mode VM structure.
 * @param   fFlags          Notification flags, VMNOTIFYFF_FLAGS_*.
 * @internal
 */
VMMR3_INT_DECL(void) VMR3NotifyGlobalFFU(PUVM pUVM, uint32_t fFlags)
{
    LogFlow(("VMR3NotifyGlobalFFU:\n"));
    uint32_t iHaldMethod = pUVM->vm.s.iHaltMethod;

    if (g_aHaltMethods[iHaldMethod].pfnNotifyGlobalFF) /** @todo make mandatory. */
        g_aHaltMethods[iHaldMethod].pfnNotifyGlobalFF(pUVM, fFlags);
    else
        for (VMCPUID iCpu = 0; iCpu < pUVM->cCpus; iCpu++)
            g_aHaltMethods[iHaldMethod].pfnNotifyCpuFF(&pUVM->aCpus[iCpu], fFlags);
}


/**
 * Notify the emulation thread (EMT) about pending Forced Action (FF).
 *
 * This function is called by thread other than EMT to make
 * sure EMT wakes up and promptly service an FF request.
 *
 * @param   pUVCpu          Pointer to the user mode per CPU VM structure.
 * @param   fFlags          Notification flags, VMNOTIFYFF_FLAGS_*.
 * @internal
 */
VMMR3_INT_DECL(void) VMR3NotifyCpuFFU(PUVMCPU pUVCpu, uint32_t fFlags)
{
    PUVM pUVM = pUVCpu->pUVM;

    LogFlow(("VMR3NotifyCpuFFU:\n"));
    g_aHaltMethods[pUVM->vm.s.iHaltMethod].pfnNotifyCpuFF(pUVCpu, fFlags);
}

/*MYCODE*/
//TODO: include with DBGTcp.cpp
#define DEBUG_LEVEL 0

#if DEBUG_LEVEL > 0
#define LogRelDebug(x) LogRel(x)
#else
#define LogRelDebug(x)
#endif


//TODO: Add Cs, Ds, Es, Fs, Gs, SS, ...
void VMR3UpdateFdpCpuCtx(PVMCPU pVCpu)
{
    PCCPUMCTXCORE pCtxCore = CPUMGetGuestCtxCore(pVCpu);
    FDP_CPU_CTX* pFdpCpuCtx = (FDP_CPU_CTX *)pVCpu->mystate.s.pCpuShm;

    pFdpCpuCtx->rip = pCtxCore->rip;
    pFdpCpuCtx->rax = pCtxCore->rax;
    pFdpCpuCtx->rcx = pCtxCore->rcx;
    pFdpCpuCtx->rdx = pCtxCore->rdx;
    pFdpCpuCtx->rbx = pCtxCore->rbx;
    pFdpCpuCtx->rsp = pCtxCore->rsp;
    pFdpCpuCtx->rbp = pCtxCore->rbp;
    pFdpCpuCtx->rsi = pCtxCore->rsi;
    pFdpCpuCtx->rdi = pCtxCore->rdi;
    pFdpCpuCtx->r8 = pCtxCore->r8;
    pFdpCpuCtx->r9 = pCtxCore->r9;
    pFdpCpuCtx->r10 = pCtxCore->r10;
    pFdpCpuCtx->r11 = pCtxCore->r11;
    pFdpCpuCtx->r12 = pCtxCore->r12;
    pFdpCpuCtx->r13 = pCtxCore->r13;
    pFdpCpuCtx->r14 = pCtxCore->r14;
    pFdpCpuCtx->r15 = pCtxCore->r15;
    pFdpCpuCtx->cr0 = CPUMGetGuestCR0(pVCpu);
    pFdpCpuCtx->cr2 = CPUMGetGuestCR2(pVCpu);
    pFdpCpuCtx->cr3 = CPUMGetGuestCR3(pVCpu);
    pFdpCpuCtx->cr4 = CPUMGetGuestCR4(pVCpu);

}

HardwarePage_t* VMR3GetAllocatedHardwarePage(PUVM pUVM, uint64_t GCPhys)
{
    //Look for a breakpoint already using a convient page
    int BreakpointId = VMMGetBreakpointIdFromPage(pUVM->pVM, GCPhys, FDP_SOFTHBP);
    if(BreakpointId >= 0
    && BreakpointId < MAX_BREAKPOINT_ID){
        //A breakpoint using a convient page exists
        pUVM->pVM->bp.l[BreakpointId].breakpointHardwarePage->ReferenceCount++;
        return pUVM->pVM->bp.l[BreakpointId].breakpointHardwarePage;
    }

    //Look in the Free HardwarePage table
    for(uint32_t i=0; i<pUVM->pVM->mystate.s.u32HardwarePageTableCount; i++){
        if(pUVM->pVM->mystate.s.aHardwarePageTable[i].ReferenceCount == 0
        && pUVM->pVM->mystate.s.aHardwarePageTable[i].HCPhys != 0
        && pUVM->pVM->mystate.s.aHardwarePageTable[i].R3Ptr != NULL){
            pUVM->pVM->mystate.s.aHardwarePageTable[i].ReferenceCount = 1;
            return &pUVM->pVM->mystate.s.aHardwarePageTable[i];
        }
    }

    LogRelDebug(("Allocate a new HardwarePage for %p !\n", GCPhys));
    //None are free, allocate a new one !
    ALLOCPAGEREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq    = sizeof(Req);
    Req.newPageSize = _4K;
    int rc = SUPR3CallVMMR0Ex(pUVM->pVM->pVMR0, NIL_VMCPUID, VMMR0_DO_ALLOC_HCPHYS, 0, &Req.Hdr);
    if(rc != 0){
        LogRelDebug(("Failed to allocate a new HardwarePage\n"));
        return NULL;
    }

    HardwarePage_t* TmpHardwarePage = &pUVM->pVM->mystate.s.aHardwarePageTable[pUVM->pVM->mystate.s.u32HardwarePageTableCount];

    TmpHardwarePage->PageSize = _4K;
    TmpHardwarePage->HCPhys = Req.newPageHCPHys;
    TmpHardwarePage->R3Ptr = Req.newPageR3Ptr;
    TmpHardwarePage->ReferenceCount = 1;

    pUVM->pVM->mystate.s.u32HardwarePageTableCount++;

    return TmpHardwarePage;
}


/*
 * @brief: Restore all Original HCPhys for SoftHyperBreakpointed GCPhys
 */
VMMR3_INT_DECL(int)    VMR3RestoreAllOriginalPage(PUVM pUVM, bool bIsRead, bool bIsWrite, bool bIsExecute)
{
    PVM pVM = pUVM->pVM;
    PVMCPU pVCpu = &pVM->aCpus[0];
    BreakpointEntrie_t *pTempBreakpointEntrie = NULL;
    for(uint8_t BreakpointId=4*pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        //Check if Breakpoint is Activated and if it is a SoftWareBreakpoint
        if(pTempBreakpointEntrie->breakpointActivated == true &&
           pTempBreakpointEntrie->breakpointType == FDP_SOFTHBP &&
           pTempBreakpointEntrie->breakpointOrigHCPhys != 0x0 &&
           pTempBreakpointEntrie->breakpointGCPhysAreaTable[0].Start != 0x0){
            //Set Original Page as Read and Write
            uint64_t GCPhys = pTempBreakpointEntrie->breakpointGCPhysAreaTable[0].Start;
            PGMShwSetHCPage(pVCpu, GCPhys, pTempBreakpointEntrie->breakpointOrigHCPhys);
            if(bIsRead == true){
                PGMShwPresent(pVCpu, GCPhys);
            }else{
                PGMShwNoPresent(pVCpu, GCPhys);
            }
            if(bIsWrite == true){
                PGMShwWrite(pVCpu, GCPhys);
            }else{
                PGMShwNoWrite(pVCpu, GCPhys);
            }
            if(bIsExecute == true){
                PGMShwExecute(pVCpu, GCPhys);
            }else{
                PGMShwNoExecute(pVCpu, GCPhys);
            }
            //Set page as breakable !
            PGMShwSetBreakable(pVCpu, GCPhys, true);
            //Invalidate the page !
            PGMShwInvalidate(pVCpu, GCPhys);
        }
    }
    return 0;
}

VMMR3_INT_DECL(int)    VMR3AddMsrBreakpoint(PUVM pUVM, uint8_t BreakpointAccessType, uint64_t BreakpointAddress)
{
    PVM pVM = pUVM->pVM;
    //Look for a free breakpoint
    for(int BreakpointId=4*pUVM->pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == false){
            pTempBreakpointEntrie->breakpointActivated = true;
            pTempBreakpointEntrie->breakpointGCPtr = BreakpointAddress;
            pTempBreakpointEntrie->breakpointOrigHCPhys = 0x0;
            pTempBreakpointEntrie->breakpointType = FDP_MSRHBP;
            pTempBreakpointEntrie->breakpointLength = 1;
            pTempBreakpointEntrie->breakpointAccessType = BreakpointAccessType;
            pTempBreakpointEntrie->breakpointPageSize = 0x0;
            pTempBreakpointEntrie->breakpointHardwarePage = NULL;
            pTempBreakpointEntrie->breakpointGCPhysAreaCount = 0;
            pTempBreakpointEntrie->breakpointGCPhysAreaTable = NULL;

            return BreakpointId;
        }
    }
    return -1;
}

VMMR3_INT_DECL(int)    VMR3AddCrBreakpoint(PUVM pUVM, uint8_t BreakpointAccessType, uint64_t BreakpointAddress)
{
    PVM pVM = pUVM->pVM;
    //Look for a free breakpoint
    for(int BreakpointId=4*pUVM->pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == false){
            pTempBreakpointEntrie->breakpointActivated = true;
            pTempBreakpointEntrie->breakpointGCPtr = BreakpointAddress;
            pTempBreakpointEntrie->breakpointOrigHCPhys = 0x0;
            pTempBreakpointEntrie->breakpointType = FDP_CRHBP;
            pTempBreakpointEntrie->breakpointLength = 1;
            pTempBreakpointEntrie->breakpointAccessType = BreakpointAccessType;
            pTempBreakpointEntrie->breakpointPageSize = 0x0;
            pTempBreakpointEntrie->breakpointHardwarePage = NULL;
            pTempBreakpointEntrie->breakpointGCPhysAreaCount = 0;
            pTempBreakpointEntrie->breakpointGCPhysAreaTable = NULL;

            return BreakpointId;
        }
    }
    return -1;
}

VMMR3_INT_DECL(int)    VMR3AddSoftBreakpoint(PUVM pUVM, PVMCPU pVCpu, uint8_t BreakpointAddressType, uint64_t BreakpointAddress, uint64_t BreakpointCr3)
{ //TODO: Move it to VMMAll !

    VMR3RestoreAllOriginalPage(pUVM, true, true, false);

    PVM pVM = pUVM->pVM;

    //Convert GCPtr to GCPhys if needed
    uint64_t GCPhys;
    uint64_t GCPtr = 0;
    if(BreakpointAddressType == 0x1){//Virtual
        GCPtr = BreakpointAddress;
        PGMPhysGCPtr2GCPhys(pVCpu, BreakpointAddress, &GCPhys);
    }else{ //Physical
        GCPhys = BreakpointAddress;
    }

    //Look for an already existing SoftHyperBreakpoint with same GCPhys
    for(int BreakpointId=4*pUVM->pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == true &&
           pTempBreakpointEntrie->breakpointType == FDP_SOFTHBP &&
           pTempBreakpointEntrie->breakpointGCPhysAreaTable[0].Start == GCPhys){
               //We found one !
               return BreakpointId;
        }
    }

    for(int BreakpointId=4*pUVM->pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];

        //Find a free breakpoint
        if(pTempBreakpointEntrie->breakpointActivated == false){
            //Get Original HCPhys
            uint64_t origHCPhys;
            //It is the OriginalPage because we called VMR3RestoreAllOriginalPage(pUVM);
            PGMShwGetHCPage(pVCpu, GCPhys, &origHCPhys);
            //After restore EPTPTE are not initilized
            if(origHCPhys < 0x100){
                return -1;
            }

            //Get a HardwarePage
            HardwarePage_t* pTempHardwarePage = VMR3GetAllocatedHardwarePage(pUVM, GCPhys);
            if(pTempHardwarePage != NULL){
                if(pTempHardwarePage->ReferenceCount == 1){
                    //This is the first breakpoint using this hardware page
                    //Copy original page content to new page only if the page is new
                    LogRelDebug(("[WDEBUG] Copy OrignalPage to ModificatedPage\n"));
                    PGMPhysSimpleReadGCPhys(pUVM->pVM, (void*)pTempHardwarePage->R3Ptr, (RTGCPHYS)(GCPhys & ~(pTempHardwarePage->PageSize-1)), pTempHardwarePage->PageSize);
                }

                LogRelDebug(("[WDEBUG] SoftHyperBreakpoint installation : \n"));
                LogRelDebug(("[WDEBUG] Original page HCPhys: 0x%p\n", origHCPhys));
                LogRelDebug(("[WDEBUG] GCPhys: 0x%p\n", GCPhys));
                LogRelDebug(("[WDEBUG] pTempHardwarePage: %p\n", pTempHardwarePage));
                LogRelDebug(("[WDEBUG] Modificated page HCPhys:  0x%p\n", pTempHardwarePage->HCPhys));
                LogRelDebug(("[WDEBUG] Modificated page R3Ptr:  0x%p\n", pTempHardwarePage->R3Ptr));
                LogRelDebug(("[WDEBUG] HardwarePage Reference Count: %d\n", pTempHardwarePage->ReferenceCount));
                LogRelDebug(("[WDEBUG] \n"));

                pTempBreakpointEntrie->breakpointActivated = true;
                pTempBreakpointEntrie->breakpointGCPtr = GCPtr;
                pTempBreakpointEntrie->breakpointOrigHCPhys = origHCPhys;
                pTempBreakpointEntrie->breakpointType = FDP_SOFTHBP;
                pTempBreakpointEntrie->breakpointLength = 1;
                pTempBreakpointEntrie->breakpointCr3 = BreakpointCr3;
                pTempBreakpointEntrie->breakpointAccessType = FDP_EXECUTE_BP;
                pTempBreakpointEntrie->breakpointPageSize = pTempHardwarePage->PageSize;
                pTempBreakpointEntrie->breakpointHardwarePage = pTempHardwarePage;
                //Save the original byte
                pTempBreakpointEntrie->breakpointOriginalByte = pTempHardwarePage->R3Ptr[(GCPhys & (pTempHardwarePage->PageSize-1))]; //TODO change % to &
                //Install a HLT in the new page
                pTempHardwarePage->R3Ptr[(GCPhys & (pTempHardwarePage->PageSize-1))] = 0xCC;
                pTempBreakpointEntrie->breakpointGCPhysAreaCount = 1;
                pTempBreakpointEntrie->breakpointGCPhysAreaTable = (GCPhysArea_t*)malloc(1 * sizeof(GCPhysArea_t));
                pTempBreakpointEntrie->breakpointGCPhysAreaTable[0].Start = GCPhys;
                pTempBreakpointEntrie->breakpointGCPhysAreaTable[0].End = GCPhys+1;

                //Set page as original and read/write
                PGMShwSetHCPage(pVCpu, GCPhys, origHCPhys);
                PGMShwPresent(pVCpu, GCPhys);
                PGMShwWrite(pVCpu, GCPhys);
                PGMShwNoExecute(pVCpu, GCPhys);
                //Set page as breakable !
                PGMShwSetBreakable(pVCpu, GCPhys, true);
                //Invalidate the page !
                PGMShwInvalidate(pVCpu, GCPhys);

                return BreakpointId;
            }else{
                return -1;
            }
        }
    }
    return -1;
}

void ApplyBreakpointOnPage(PVM pVM, PVMCPU pVCpu, uint64_t GCPhys, uint8_t BreakpointAccessType)
{
    //No access at all for FDP_READ_BP
    if(BreakpointAccessType & FDP_READ_BP){
        PGMShwNoPresent(pVCpu, GCPhys);
        PGMShwNoWrite(pVCpu, GCPhys);
        PGMShwNoExecute(pVCpu, GCPhys);
    }
    if(BreakpointAccessType & FDP_WRITE_BP)
        PGMShwNoWrite(pVCpu, GCPhys);
    if(BreakpointAccessType & FDP_EXECUTE_BP)
        PGMShwNoExecute(pVCpu, GCPhys);

    //Set the page as Breakable page
    PGMShwSetBreakable(pVCpu, GCPhys, true);
    //Save the final page rights
    PGMShwSaveRights(pVCpu, GCPhys);
    //Invalidate the page !
    PGMShwInvalidate(pVCpu, GCPhys);
}

void DisableBreakpointOnPage(PVM pVM, PVMCPU pVCpu, uint64_t GCPhys)
{
    PGMShwPresent(pVCpu, GCPhys);
    PGMShwWrite(pVCpu, GCPhys);
    PGMShwExecute(pVCpu, GCPhys);

    //Set the page as Standard page
    PGMShwSetBreakable(pVCpu, GCPhys, false);
    //Save the final page rights
    PGMShwSaveRights(pVCpu, GCPhys);
    //Invalidate the page !
    PGMShwInvalidate(pVCpu, GCPhys);
}

#define MIN(a,b) (((a)<(b))?(a):(b))


void AddGCPhysAreaInBreakpoint(BreakpointEntrie_t *pTempBreakpointEntrie, uint64_t Start, uint64_t End)
{
    if(pTempBreakpointEntrie){
        int CurrentGCPhysAreaIndex = pTempBreakpointEntrie->breakpointGCPhysAreaCount;
        //LogRel(("[WDEBUG] %d. %p->%p\n", CurrentGCPhysAreaIndex, Start, End));
        pTempBreakpointEntrie->breakpointGCPhysAreaTable[CurrentGCPhysAreaIndex].Start = Start;
        pTempBreakpointEntrie->breakpointGCPhysAreaTable[CurrentGCPhysAreaIndex].End = End;
        pTempBreakpointEntrie->breakpointGCPhysAreaCount++;
    }
}

void DisableAllPageBreakpoint(PVM pVM, PVMCPU pVCpu)
{
    //Restore all rights for pages in breakpoint
    for(int BreakpointId=0; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == true
        && pTempBreakpointEntrie->breakpointType == FDP_PAGEHBP
        && pTempBreakpointEntrie->breakpointGCPhysAreaTable){
            for(int j=0; j<pTempBreakpointEntrie->breakpointGCPhysAreaCount; j++){
                DisableBreakpointOnPage(pVM, pVCpu, pTempBreakpointEntrie->breakpointGCPhysAreaTable[j].Start);
            }
        }
    }
}

void EnableAllPageBreakpoint(PVM pVM, PVMCPU pVCpu)
{
    //Restore all rights for pages in breakpoint
    for(int BreakpointId=0; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == true
        && pTempBreakpointEntrie->breakpointType == FDP_PAGEHBP
        && pTempBreakpointEntrie->breakpointGCPhysAreaTable){
            //Apply on pages
            for(int j=0; j<pTempBreakpointEntrie->breakpointGCPhysAreaCount; j++){
                ApplyBreakpointOnPage(pVM, pVCpu, pTempBreakpointEntrie->breakpointGCPhysAreaTable[j].Start, pTempBreakpointEntrie->breakpointAccessType);
            }
        }
    }
}

void InstallAllPageBreakpoint(PVM pVM, PVMCPU pVCpu)
{
    //Restore all rights for pages in breakpoint
    for(int BreakpointId=0; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == true
        && pTempBreakpointEntrie->breakpointType == FDP_PAGEHBP
        && pTempBreakpointEntrie->breakpointGCPhysAreaTable){
            for(int j=0; j<pTempBreakpointEntrie->breakpointGCPhysAreaCount; j++){
                DisableBreakpointOnPage(pVM, pVCpu, pTempBreakpointEntrie->breakpointGCPhysAreaTable[j].Start);
            }
            if(pTempBreakpointEntrie->breakpointGCPhysAreaTable)
                free(pTempBreakpointEntrie->breakpointGCPhysAreaTable);
            pTempBreakpointEntrie->breakpointGCPhysAreaCount = 0;
            pTempBreakpointEntrie->breakpointGCPhysAreaTable = NULL;
        }
    }

    //Remove rights for pages in breakpoint
    for(int BreakpointId=0; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == true
        && pTempBreakpointEntrie->breakpointType == FDP_PAGEHBP){
            uint64_t BreakpointLength = pTempBreakpointEntrie->breakpointLength;
            if(pTempBreakpointEntrie->breakpointGCPtr > 0){ //VirtualAddress Breakpoint
                uint64_t GCPhys;
                uint64_t GCPtr = pTempBreakpointEntrie->breakpointGCPtr;
                int MaxGCPhysAreaCount = (BreakpointLength/_4K) + 1;
                pTempBreakpointEntrie->breakpointGCPhysAreaTable = (GCPhysArea_t*)malloc(MaxGCPhysAreaCount * sizeof(GCPhysArea_t));

                //First chunk Page
                int rc = PGMPhysGCPtr2GCPhys(pVCpu, GCPtr, &GCPhys);
                uint64_t GCPhysPageEnd = (GCPhys & 0xFFFFFFFFFFFFF000) + _4K;
                uint64_t AlreadyBreakpointSize = MIN(GCPhysPageEnd - GCPhys, BreakpointLength);
                if(RT_SUCCESS(rc)){
                    AddGCPhysAreaInBreakpoint(pTempBreakpointEntrie, GCPhys, GCPhys+AlreadyBreakpointSize);
                }
                //Intermediate complete page
                int64_t LeftToBreakpoint = BreakpointLength - AlreadyBreakpointSize;
                while (LeftToBreakpoint >= _4K){ //More than 1 page to breakpoint !
                    rc = PGMPhysGCPtr2GCPhys(pVCpu, GCPtr + AlreadyBreakpointSize, &GCPhys);
                    if(RT_SUCCESS(rc)){
                        AddGCPhysAreaInBreakpoint(pTempBreakpointEntrie, GCPhys, GCPhys+_4K);
                    }
                    LeftToBreakpoint = LeftToBreakpoint - _4K;
                    AlreadyBreakpointSize = AlreadyBreakpointSize + _4K;
                }

                //Last chunk page
                if (LeftToBreakpoint > 0){ //Left breakpoint bytes
                    rc = PGMPhysGCPtr2GCPhys(pVCpu, GCPtr + AlreadyBreakpointSize, &GCPhys);
                    if(RT_SUCCESS(rc)){
                        AddGCPhysAreaInBreakpoint(pTempBreakpointEntrie, GCPhys, GCPhys+LeftToBreakpoint);
                    }
                }
            }else{ //PhysicalAddress Breakpoint
                uint64_t LeftToBreakpoint = 0;
                int MaxGCPhysAreaCount = (BreakpointLength/_4K) + 1;
                pTempBreakpointEntrie->breakpointGCPhysAreaTable = (GCPhysArea_t*)malloc(MaxGCPhysAreaCount * sizeof(GCPhysArea_t));
                uint64_t GCPhysPageEnd = (pTempBreakpointEntrie->breakpointGCPhys & ~(_4K-1)) + _4K;
                uint64_t LastPageEnd = MIN(GCPhysPageEnd, pTempBreakpointEntrie->breakpointGCPhys+BreakpointLength);
                LeftToBreakpoint = BreakpointLength - (LastPageEnd - pTempBreakpointEntrie->breakpointGCPhys);
                AddGCPhysAreaInBreakpoint(pTempBreakpointEntrie, pTempBreakpointEntrie->breakpointGCPhys, LastPageEnd);
                while(LeftToBreakpoint >= _4K){
                    AddGCPhysAreaInBreakpoint(pTempBreakpointEntrie, LastPageEnd, LastPageEnd+_4K);
                    LeftToBreakpoint -= _4K;
                    LastPageEnd += _4K;
                }
                if(LeftToBreakpoint > 0){
                    AddGCPhysAreaInBreakpoint(pTempBreakpointEntrie, LastPageEnd, LastPageEnd+LeftToBreakpoint);
                }
            }


            //Apply on pages
            for(int j=0; j<pTempBreakpointEntrie->breakpointGCPhysAreaCount; j++){
                ApplyBreakpointOnPage(pVM, pVCpu, pTempBreakpointEntrie->breakpointGCPhysAreaTable[j].Start, pTempBreakpointEntrie->breakpointAccessType);
            }
        }
    }

    return;
}


bool IsOneCPURunning(PUVM pUVM)
{
    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        if(!(pUVM->pVM->aCpus[i].mystate.s.u8StateBitmap & FDP_STATE_PAUSED)){
            return true;
        }
    }
    return false;
}




VMMR3_INT_DECL(int)    VMR3AddPageBreakpoint(PUVM pUVM, PVMCPU pVCpu, uint8_t BreakpointId, uint8_t BreakpointAccessType, uint8_t BreakpointAddressType, uint64_t BreakpointAddress, uint64_t BreakpointLength)
{ //TODO: Move it to VMMAll !
    if(IsOneCPURunning(pUVM) == true){
        //NO WAY !!!!!!
        return -1;
    }

    PVM pVM = pUVM->pVM;

    BreakpointEntrie_t *pTempBreakpointEntrie = NULL;
    //If not a reserved to the guest breakpoint
    if(BreakpointId < 0 || BreakpointId > 3){
        bool BreakpointIdFound = false;
        for(BreakpointId=4*pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
            pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
            //Find a free breakpoint
            if(pTempBreakpointEntrie->breakpointActivated == false){
                break;
            }
        }
    }else{
        pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
    }

    if(pTempBreakpointEntrie != NULL
    && pTempBreakpointEntrie->breakpointActivated == false){
        uint64_t GCPhys;
        uint64_t GCPtr = 0;
        if(BreakpointAddressType == FDP_VIRTUAL_ADDRESS){//Virtual
            GCPtr = BreakpointAddress;
            int rc = PGMPhysGCPtr2GCPhys(pVCpu, BreakpointAddress, &GCPhys);
            if(RT_FAILURE(rc)){
                //LogRel(("Fail to convert GCPtr(%p) -> GCphys\n", BreakpointAddress));
                return -1;
            }
        }else{ //Physical
            GCPhys = BreakpointAddress;
        }

        pVM->bp.l[BreakpointId].breakpointActivated = true;
        pVM->bp.l[BreakpointId].breakpointGCPtr = GCPtr;
        pVM->bp.l[BreakpointId].breakpointGCPhys = GCPhys;
        pVM->bp.l[BreakpointId].breakpointType = FDP_PAGEHBP;
        pVM->bp.l[BreakpointId].breakpointLength = BreakpointLength;
        pVM->bp.l[BreakpointId].breakpointAccessType = BreakpointAccessType;
        pVM->bp.l[BreakpointId].breakpointPageSize = _4K;

        InstallAllPageBreakpoint(pVM, pVCpu);
        return BreakpointId;
    }
    return -1;
}

//TODO: Move it to VMMAll !
VMMR3_INT_DECL(bool) VMR3RemoveBreakpoint(PUVM pUVM, int BreakpointId)
{

    if(BreakpointId < 0 || BreakpointId > MAX_BREAKPOINT_ID){
        return false;
    }

    //If one Cpu is running, we can't remove a breakpoint !
    if(IsOneCPURunning(pUVM) == true){
        return false;
    }

    PVM pVM = pUVM->pVM;
    PVMCPU pVCpu = &pVM->aCpus[0];

    //Restore OriginalPage for all SoftHyperBreakpoint
    VMR3RestoreAllOriginalPage(pVM->pUVM, true, true, false);

    BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];

    if(pTempBreakpointEntrie->breakpointActivated == true){
        //Set the breakpoint as disabled
        pTempBreakpointEntrie->breakpointActivated = false;

        switch(pTempBreakpointEntrie->breakpointType)
        {
            case FDP_PAGEHBP:
            {
                //Enable all rights for page in this breakpoint
                for(int j=0; j<pTempBreakpointEntrie->breakpointGCPhysAreaCount; j++){
                    DisableBreakpointOnPage(pVM, pVCpu, pTempBreakpointEntrie->breakpointGCPhysAreaTable[j].Start);
                }
                //Enable all other page Breakpoint
                InstallAllPageBreakpoint(pVM, pVCpu);
                break;
            }
            case FDP_SOFTHBP:
            {
                pTempBreakpointEntrie->breakpointHardwarePage->ReferenceCount--;
                if(pTempBreakpointEntrie->breakpointHardwarePage->ReferenceCount == 0){
                    uint64_t GCPhys = pTempBreakpointEntrie->breakpointGCPhysAreaTable[0].Start;
                    LogRelDebug(("[WDEBUG] HardwarePage->ReferenceCount == 0\n"));
                    //No more breakpoint use this HardwarePage !
                    //Restore Original page
                    PGMShwSetHCPage(pVCpu, GCPhys, pTempBreakpointEntrie->breakpointOrigHCPhys);
                    PGMShwPresent(pVCpu, GCPhys);
                    PGMShwWrite(pVCpu, GCPhys);
                    PGMShwExecute(pVCpu, GCPhys);

                    PGMShwSetBreakable(pVCpu, GCPhys, false);

                    PGMShwInvalidate(pVCpu, GCPhys);
                }
                break;
            }
            default:
                break;
        }

        pTempBreakpointEntrie->breakpointTag = 0;
        pTempBreakpointEntrie->breakpointGCPtr = 0;
        pTempBreakpointEntrie->breakpointType = 0;
        pTempBreakpointEntrie->breakpointLength = 0;
        pTempBreakpointEntrie->breakpointAccessType = 0x0;
        pTempBreakpointEntrie->breakpointOrigHCPhys = 0x0;
        pTempBreakpointEntrie->breakpointOriginalByte = 0x0;
        pTempBreakpointEntrie->breakpointHardwarePage = NULL;
        pTempBreakpointEntrie->breakpointPageSize = 0x0;

        if(pTempBreakpointEntrie->breakpointGCPhysAreaTable){
            free(pTempBreakpointEntrie->breakpointGCPhysAreaTable);
        }

        pTempBreakpointEntrie->breakpointGCPhysAreaCount = 0;
        pTempBreakpointEntrie->breakpointGCPhysAreaTable = NULL;
        return true;
    }
    return false;
}

VMMDECL(int) VMR3PhysSimpleReadGCPhysU(PUVM pUVM, void *pvDst, RTGCPHYS GCPhysSrc, size_t cb)
{
    return PGMPhysSimpleReadGCPhys(pUVM->pVM, pvDst, GCPhysSrc, cb);
}

VMMDECL(int) VMR3PhysSimpleWriteGCPhysU(PUVM pUVM, const void *pvBuf, RTGCPHYS GCPhys, size_t cbWrite)
{
    return PGMPhysSimpleWriteGCPhys(pUVM->pVM, GCPhys, pvBuf, cbWrite);
}

VMMDECL(int) VMR3SingleStep(PUVM pUVM, PVMCPU pVCpu)
{
    //Dont try to single step on a running
    if(pVCpu->mystate.s.u8StateBitmap & FDP_STATE_PAUSED){
        pVCpu->mystate.s.bSingleStepRequired = true;
        while(pVCpu->mystate.s.bSingleStepRequired){
            //Yield
            RTThreadSleep(0);
        }
        return 0;
    }
    return -1;
}

VMMDECL(int) VMR3BreakNoLock(PUVM pUVM)
{
    //LogRel(("[WDEBUG] BREAK !\n"));

    //Wait for all cpu paused
    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        pUVM->pVM->aCpus[i].mystate.s.bPauseRequired = true;

        //Inject a IPI
        SUPR3CallVMMR0Ex(pUVM->pVM->pVMR0, pUVM->pVM->aCpus[i].idCpu, VMMR0_DO_GVMM_SCHED_WAKE_UP, 0, NULL);
        SUPR3CallVMMR0Ex(pUVM->pVM->pVMR0, pUVM->pVM->aCpus[i].idCpu, VMMR0_DO_GVMM_SCHED_POKE, 0, NULL);

        do{
            //LogRel(("[WDEBUG] Waiting for CPU[%d] to pause state %02x\n", i, pUVM->pVM->aCpus[i].mystate.s.u8StateBitmap));
            //RTThreadSleep(10);
        }while(!(pUVM->pVM->aCpus[i].mystate.s.u8StateBitmap & FDP_STATE_PAUSED));
        //LogRel(("[WDEBUG] CPU[%d] is paused !\n", i));
    }

    //LogRel(("[WDEBUG] All Cpus are PAUSED !\n"));
    return 0;
}

VMMDECL(int) VMR3Break(PUVM pUVM)
{
    RTSpinlockAcquire(pUVM->pVM->mystate.s.CpuLock);

    VMR3BreakNoLock(pUVM);

    RTSpinlockRelease(pUVM->pVM->mystate.s.CpuLock);
    return 0;
}

VMMDECL(int) VMR3ContinueNoWaitNoLock(PUVM pUVM)
{
    //LogRel(("[WDEBUG] VMR3ContinueNoWaitNoLock !\n"));

    pUVM->pVM->mystate.s.u8StateBitmap &= ~FDP_STATE_DEBUGGER_ALERTED;

    //Wait for all CPUs resumed
    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        PVMCPU pVCpu = &pUVM->pVM->aCpus[i];
        uint64_t oldu64TickCount = pVCpu->mystate.s.u64TickCount;
        pVCpu->mystate.s.bPauseRequired = false;
        VMCPU_FF_SET(pVCpu, VMCPU_FF_EXTERNAL_SUSPENDED_MASK);
    }

    return 0;
}

VMMDECL(int) VMR3ContinueWaitNoLock(PUVM pUVM)
{
    //LogRel(("[WDEBUG] VMR3ContinueWaitNoLock !\n"));

    pUVM->pVM->mystate.s.u8StateBitmap &= ~FDP_STATE_DEBUGGER_ALERTED;

    //Wait for all CPUs resumed
    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        PVMCPU pVCpu = &pUVM->pVM->aCpus[i];
        uint64_t oldu64TickCount = pVCpu->mystate.s.u64TickCount;
        pVCpu->mystate.s.bPauseRequired = false;
        VMCPU_FF_SET(pVCpu, VMCPU_FF_EXTERNAL_SUSPENDED_MASK);
        while(oldu64TickCount == pVCpu->mystate.s.u64TickCount){
            //Yield
            RTThreadSleep(0);
        }
    }

    return 0;
}

VMMDECL(int) VMR3Continue(PUVM pUVM)
{
    RTSpinlockAcquire(pUVM->pVM->mystate.s.CpuLock);

    VMR3ContinueWaitNoLock(pUVM);

    RTSpinlockRelease(pUVM->pVM->mystate.s.CpuLock);
    return 0;
}

VMMDECL(uint8_t) VMR3GetFDPState(PUVM pUVM)
{
    uint8_t u8OldState = 0;
    bool bIsPaused = true;
    bool bIsBreakpointHitted = false;
    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        //If one CPU is Running not in pause
        if(!(pUVM->pVM->aCpus[i].mystate.s.u8StateBitmap & FDP_STATE_PAUSED)){
            bIsPaused = false;
        }
        //If one CPU hit a breakpoint
        if(pUVM->pVM->aCpus[i].mystate.s.u8StateBitmap & FDP_STATE_BREAKPOINT_HIT){
            bIsBreakpointHitted = true;
        }
    }

    if(bIsPaused){
        u8OldState |= FDP_STATE_PAUSED;
        if(bIsBreakpointHitted){
            u8OldState |= FDP_STATE_BREAKPOINT_HIT;
        }
    }

    if(pUVM->pVM->mystate.s.u8StateBitmap & FDP_STATE_DEBUGGER_ALERTED){
        u8OldState |= FDP_STATE_DEBUGGER_ALERTED;
    }
    if(u8OldState & FDP_STATE_BREAKPOINT_HIT){
        pUVM->pVM->mystate.s.u8StateBitmap |= FDP_STATE_DEBUGGER_ALERTED;
    }
    return u8OldState;
}

VMMDECL(bool) VMR3DisableAllMsrBreakpoint(PVM pVM)
{
    for(int BreakpointId=4*pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointActivated == true
            && pTempBreakpointEntrie->breakpointType == FDP_MSRHBP){
                pTempBreakpointEntrie->breakpointActivated = false;
        }
    }
    return 0;
}

VMMDECL(bool) VMR3EnableAllMsrBreakpoint(PVM pVM)
{
    for(int BreakpointId=4*pVM->cCpus; BreakpointId<MAX_BREAKPOINT_ID; BreakpointId++){
        BreakpointEntrie_t *pTempBreakpointEntrie = &pVM->bp.l[BreakpointId];
        if(pTempBreakpointEntrie->breakpointType == FDP_MSRHBP){
                pTempBreakpointEntrie->breakpointActivated = true;
        }
    }
    return 0;
}

VMMDECL(bool) VMR3HandleSingleStep(PVM pVM, PVMCPU pVCpu)
{
    //Check if Single step is required !
    if(pVCpu->mystate.s.bSingleStepRequired){

        TMR3NotifyResume(pVM, pVCpu);

        LogRelDebug(("[WDEBUG] CPU[%d] bSingleStepRequired !\n", pVCpu->idCpu));

        //Restore Original Page with Execute right, avoid Breakpoint in SingleStep
        VMR3RestoreAllOriginalPage(pVM->pUVM, true, true, true);
        //Disable All Msr Breakpoint, avoid Breakpoint in Breakpoint
        VMR3DisableAllMsrBreakpoint(pVM);
        //Disable PageHyperBreapoint
        DisableAllPageBreakpoint(pVM, pVCpu);
        //Disable Debug Register
        uint64_t OldDr7 = CPUMGetGuestDR7(pVCpu);
        CPUMSetGuestDR7(pVCpu, 0x400);

        int rc = 0;
        //First call is for instruction that jump on self "jmp -2 (ebfe)"
        rc = EMR3HmSingleInstruction(pVM, pVCpu, 0);

        LogRelDebug(("[WDEBUG] CPU[%d] Single Step => %d!\n", pVCpu->idCpu, rc));

        if(VM_FF_IS_PENDING(pVM, VM_FF_ALL_REM_MASK)
        || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_ALL_REM_MASK)){
            EMR3ProcessForcedAction(pVM, pVCpu, rc);
        }

        //If rc == 0 then it failed, we have to call SingleInstruction whith RIP_CHANGE
        if(rc == 0){
            rc = EMR3HmSingleInstruction(pVM, pVCpu, EM_ONE_INS_FLAGS_RIP_CHANGE);

            LogRelDebug(("[WDEBUG] CPU[%d] Single Step => %d!\n", pVCpu->idCpu, rc));

            if(VM_FF_IS_PENDING(pVM, VM_FF_ALL_REM_MASK)
            || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_ALL_REM_MASK)){
                EMR3ProcessForcedAction(pVM, pVCpu, rc);
            }
        }

        //Restore Original Page, avoid VirtualBox being crazy with unknown HCPhys on SoftHyperBreakpoint
        VMR3RestoreAllOriginalPage(pVM->pUVM, true, true, false);
        //Enable All Msr Breakpoint
        VMR3EnableAllMsrBreakpoint(pVM);
        //Enable All PageHyperBreakpoint
        EnableAllPageBreakpoint(pVM, pVCpu);
        //Enable Debug Register
        CPUMSetGuestDR7(pVCpu, OldDr7);

        TMR3NotifySuspend(pVM, pVCpu);

        pVCpu->mystate.s.bSingleStepRequired = false; //Single step no more required !
        return true;
    }
    return false;
}

VMMDECL(int) VMR3InjectInterrupt(PVM pVM, PVMCPU pVCpu, uint32_t enmXcpt, uint32_t uErr, uint64_t Cr2)
{
    return TRPMRaiseXcptErrCR2(pVCpu, NULL, (X86XCPT)enmXcpt, uErr, Cr2);
}

#include <VBox/vmm/pdmusb.h>

VMMDECL(int) VMR3ClearInterrupt(PUVM pUVM, PVMCPU pVCpu)
{
    //return PDMR3UsbHasHub(pUVM);
    PDMR3PowerOn(pUVM->pVM);
    return 0;
}

VMMDECL(void) VMR3SetFDPShm(PUVM pUVM, void *pFdpShm)
{
    pUVM->pVM->mystate.s.pFdpShm = pFdpShm;
}

VMMDECL(bool) VMR3EnterPause(PVM pVM, PVMCPU pVCpu)
{
    if(pVCpu->idCpu == 0){
        //Update FDP_CPU_CTX
        VMR3UpdateFdpCpuCtx(pVCpu);
        TMR3NotifySuspend(pVM, pVCpu);

        //Active wait
        uint32_t u32WaitCount = 0;
        while(pVCpu->mystate.s.bPauseRequired == true){
            if(VMR3HandleSingleStep(pVM, pVCpu) == true){
                //Update FDP_CPU_CTX
                VMR3UpdateFdpCpuCtx(pVCpu);
                u32WaitCount = 0;
            }
            //Powersaving :)
            if((u32WaitCount & 0xFFFFFF) == 0xFFFFFF){
                RTThreadSleep(5);
            }else{
                u32WaitCount++;
            }
        }

        TMR3NotifyResume(pVM, pVCpu);

        //ProcessForcedAction avoid freeze in CLI...BP...SAVE...STI
        if(VM_FF_IS_PENDING(pVM, VM_FF_ALL_REM_MASK)
            || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_ALL_REM_MASK)){
            EMR3ProcessForcedAction(pVM, pVCpu, 0);
        }


        //Update FDP_CPU_CTX
        VMR3UpdateFdpCpuCtx(pVCpu);
    }
    return true;
}

#include "VMMInternal.h"


#define DR0_ENABLED 0x3
#define DR1_ENABLED 0xC
#define DR2_ENABLED 0x30
#define DR3_ENABLED 0xC0

#define DR_READ 0x03
#define DR_WRITE 0x01
#define DR_EXECUTE 0x00

/*
* @brief Convert a Debug Register Breakpoint Type to FDP Breakpoint Type
*
*/
int GetDrType(uint64_t DrType)
{
    switch(DrType){
        case DR_READ:
            return FDP_READ_BP;
        case DR_WRITE:
            return FDP_WRITE_BP;
        case DR_EXECUTE:
            return FDP_EXECUTE_BP;
    }
    return 0;
}

/*
 * @brief Get the Breakpoint Lenght from Debug Register Breakpoint Lenght
 */
uint64_t GetDrLength(uint64_t DrLength)
{
    switch(DrLength){
        case 0:
            return 1;
        case 1:
            return 2;
        case 2:
            return 8;
        case 3:
            return 4;
    }
    return 1;
}

VMMR3DECL(uint32_t) VMR3GetCPUCount(PUVM pUVM)
{
    return pUVM->pVM->cCpus;
}


/**
 * Halted VM Wait.
 * Any external event will unblock the thread.
 *
 * @returns VINF_SUCCESS unless a fatal error occurred. In the latter
 *          case an appropriate status code is returned.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   fIgnoreInterrupts   If set the VM_FF_INTERRUPT flags is ignored.
 * @thread  The emulation thread.
 * @remarks Made visible for implementing vmsvga sync register.
 * @internal
 */
VMMR3_INT_DECL(int) VMR3WaitHalted(PVM pVM, PVMCPU pVCpu, bool fIgnoreInterrupts)
{

    /*MYCODE*/
    if(pVCpu->mystate.s.bInstallDrBreakpointRequired){
        LogRelDebug(("[WDEBUG] CPU[%d] Entering bInstallDrBreakpointRequired !!\n", pVCpu->idCpu));
        pVCpu->mystate.s.u8StateBitmap |= FDP_STATE_PAUSED;

        //Break all CPUs
        VMR3Break(pVM->pUVM);

        //Remove all breakpoint
        int BreakpointId = 0;
        for(int BreakpointId = (0+(pVCpu->idCpu*4)); BreakpointId<(int)(4+(pVCpu->idCpu*4)); BreakpointId++){
            VMR3RemoveBreakpoint(pVM->pUVM, BreakpointId);
        }

        //Install all breakpoint
        LogRelDebug(("[WDEBUG] CPU[%d] aGuestDr[0] %p\n", pVCpu->idCpu, pVCpu->mystate.s.aGuestDr[0]));
        LogRelDebug(("[WDEBUG] CPU[%d] aGuestDr[1] %p\n", pVCpu->idCpu, pVCpu->mystate.s.aGuestDr[1]));
        LogRelDebug(("[WDEBUG] CPU[%d] aGuestDr[2] %p\n", pVCpu->idCpu, pVCpu->mystate.s.aGuestDr[2]));
        LogRelDebug(("[WDEBUG] CPU[%d] aGuestDr[3] %p\n", pVCpu->idCpu, pVCpu->mystate.s.aGuestDr[3]));
        LogRelDebug(("[WDEBUG] CPU[%d] aGuestDr[7] %p\n", pVCpu->idCpu, pVCpu->mystate.s.aGuestDr[7]));

        //Update Guest Breakpoint
        for(uint8_t i=0; i<4; i++){
            if(pVCpu->mystate.s.aGuestDr[7] & (0x3<< (i*2))){
                uint8_t TEMP_DRX_LENGTH = (pVCpu->mystate.s.aGuestDr[7] & (0x3 << (18+i*4))) >> (18+i*4);
                uint8_t DRX_LENGTH = GetDrLength(TEMP_DRX_LENGTH);
                uint8_t TEMP_DRX_TYPE = (pVCpu->mystate.s.aGuestDr[7] & (0x3 << (16+i*4))) >> (16+i*4);
                int DRX_TYPE = GetDrType(TEMP_DRX_TYPE);

                int BreakpointId = -1;
                if(DRX_TYPE > 0){
                    BreakpointId = VMR3AddPageBreakpoint(pVM->pUVM, pVCpu, i+(pVCpu->idCpu*4), DRX_TYPE, FDP_VIRTUAL_ADDRESS, pVCpu->mystate.s.aGuestDr[i], DRX_LENGTH);
                }
                //LogRel(("INSTALL DR[%d] %d\n", i, BreakpointId));
            }
        }


        pVCpu->mystate.s.u8StateBitmap &= ~FDP_STATE_PAUSED;
        pVCpu->mystate.s.bPauseRequired = false;
        pVCpu->mystate.s.bInstallDrBreakpointRequired = false;

        //Continue all CPUs
        VMR3ContinueNoWaitNoLock(pVM->pUVM);

        LogRelDebug(("[WDEBUG] CPU[%d] Leaving bInstallDrBreakpointRequired !!\n", pVCpu->idCpu));
        return VINF_EM_RESCHEDULE;
    }

    if(pVCpu->mystate.s.bHardHyperBreakPointHitted
    || pVCpu->mystate.s.bPageHyperBreakPointHitted
    || pVCpu->mystate.s.bSoftHyperBreakPointHitted
    || pVCpu->mystate.s.bMsrHyperBreakPointHitted
    || pVCpu->mystate.s.bCrHyperBreakPointHitted){

        //Update FDP_CPU_CTX
        VMR3UpdateFdpCpuCtx(pVCpu);

        if(pVCpu->mystate.s.bPageHyperBreakPointHitted){
            LogRelDebug(("[WDEBUG] CPU[%d] bPageHyperBreakPointHitted !!\n", pVCpu->idCpu));
        }
        if(pVCpu->mystate.s.bSoftHyperBreakPointHitted){
            LogRelDebug(("[WDEBUG] CPU[%d] bSoftHyperBreakPointHitted !!\n", pVCpu->idCpu));
        }
        if(pVCpu->mystate.s.bHardHyperBreakPointHitted){
            LogRelDebug(("[WDEBUG] CPU[%d] bHardHyperBreakPointHitted !!\n", pVCpu->idCpu));
            pVCpu->mystate.s.u8StateBitmap |= FDP_STATE_HARD_BREAKPOINT_HIT;
        }
        if(pVCpu->mystate.s.bMsrHyperBreakPointHitted){
            LogRelDebug(("[WDEBUG] CPU[%d] bMsrHyperBreakPointHitted !!\n", pVCpu->idCpu));
        }
        if(pVCpu->mystate.s.bCrHyperBreakPointHitted){
            LogRelDebug(("[WDEBUG] CPU[%d] bCrHyperBreakPointHitted !!\n", pVCpu->idCpu));
        }


        //Restore OriginalPage for all SoftHyperBreakpoint
        VMR3RestoreAllOriginalPage(pVM->pUVM, true, true, false);

        //Set the CPU as PAUSED and BREAKPOINT_HITTED
        pVCpu->mystate.s.u8StateBitmap |= FDP_STATE_BREAKPOINT_HIT;
        pVCpu->mystate.s.u8StateBitmap |= FDP_STATE_PAUSED;

        //Break all CPUs
        VMR3Break(pVM->pUVM);

        //TODO: Protect this !
        FDP_SHM *pFdpShm = (FDP_SHM *)pVM->mystate.s.pFdpShm;
        FDP_SetStateChanged(pFdpShm);

        //Waiting for debugger resume !
        VMR3EnterPause(pVM, pVCpu);

        bool bMsrHyperBreakpointHitted = pVCpu->mystate.s.bMsrHyperBreakPointHitted;

        //We are ready to go !
        pVCpu->mystate.s.bHardHyperBreakPointHitted = false;
        pVCpu->mystate.s.bPageHyperBreakPointHitted = false;
        pVCpu->mystate.s.bSoftHyperBreakPointHitted = false;
        pVCpu->mystate.s.bMsrHyperBreakPointHitted = false;
        pVCpu->mystate.s.bCrHyperBreakPointHitted = false;
        pVCpu->mystate.s.u8StateBitmap = 0;

        //Single step for MsrBreakpoint... Maybe this stuff should be done in Winbagility...
        if(bMsrHyperBreakpointHitted == true){
            pVCpu->mystate.s.bSingleStepRequired = true;
            VMR3HandleSingleStep(pVM, pVCpu);
            pVCpu->mystate.s.bSingleStepRequired = false;
        }

        LogRelDebug(("[WDEBUG] CPU[%d] Leaving Breakpoint !\n", pVCpu->idCpu));
        return VINF_EM_RESCHEDULE;
    }
    /*ENDMYCODE*/

    LogFlow(("VMR3WaitHalted: fIgnoreInterrupts=%d\n", fIgnoreInterrupts));

    /*
     * Check Relevant FFs.
     */
    const uint32_t fMask = !fIgnoreInterrupts
        ? VMCPU_FF_EXTERNAL_HALTED_MASK
        : VMCPU_FF_EXTERNAL_HALTED_MASK & ~(VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC);
    if (    VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_HALTED_MASK)
        ||  VMCPU_FF_IS_PENDING(pVCpu, fMask))
    {
        LogFlow(("VMR3WaitHalted: returns VINF_SUCCESS (FF %#x FFCPU %#x)\n", pVM->fGlobalForcedActions, pVCpu->fLocalForcedActions));
        return VINF_SUCCESS;
    }

    /*
     * The yielder is suspended while we're halting, while TM might have clock(s) running
     * only at certain times and need to be notified..
     */
    if (pVCpu->idCpu == 0)
        VMMR3YieldSuspend(pVM);
    TMNotifyStartOfHalt(pVCpu);

    /*
     * Record halt averages for the last second.
     */
    PUVMCPU pUVCpu = pVCpu->pUVCpu;
    uint64_t u64Now = RTTimeNanoTS();
    int64_t off = u64Now - pUVCpu->vm.s.u64HaltsStartTS;
    if (off > 1000000000)
    {
        if (off > _4G || !pUVCpu->vm.s.cHalts)
        {
            pUVCpu->vm.s.HaltInterval = 1000000000 /* 1 sec */;
            pUVCpu->vm.s.HaltFrequency = 1;
        }
        else
        {
            pUVCpu->vm.s.HaltInterval = (uint32_t)off / pUVCpu->vm.s.cHalts;
            pUVCpu->vm.s.HaltFrequency = ASMMultU64ByU32DivByU32(pUVCpu->vm.s.cHalts, 1000000000, (uint32_t)off);
        }
        pUVCpu->vm.s.u64HaltsStartTS = u64Now;
        pUVCpu->vm.s.cHalts = 0;
    }
    pUVCpu->vm.s.cHalts++;

    /*
     * Do the halt.
     */
    VMCPU_ASSERT_STATE(pVCpu, VMCPUSTATE_STARTED);
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_HALTED);
    PUVM pUVM = pUVCpu->pUVM;
    int rc = g_aHaltMethods[pUVM->vm.s.iHaltMethod].pfnHalt(pUVCpu, fMask, u64Now);
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED);

    /*
     * Notify TM and resume the yielder
     */
    TMNotifyEndOfHalt(pVCpu);
    if (pVCpu->idCpu == 0)
        VMMR3YieldResume(pVM);

    LogFlow(("VMR3WaitHalted: returns %Rrc (FF %#x)\n", rc, pVM->fGlobalForcedActions));
    return rc;
}


/**
 * Suspended VM Wait.
 * Only a handful of forced actions will cause the function to
 * return to the caller.
 *
 * @returns VINF_SUCCESS unless a fatal error occurred. In the latter
 *          case an appropriate status code is returned.
 * @param   pUVCpu          Pointer to the user mode VMCPU structure.
 * @thread  The emulation thread.
 * @internal
 */
VMMR3_INT_DECL(int) VMR3WaitU(PUVMCPU pUVCpu)
{
    LogFlow(("VMR3WaitU:\n"));

    /*
     * Check Relevant FFs.
     */
    PVM    pVM   = pUVCpu->pVM;
    PVMCPU pVCpu = pUVCpu->pVCpu;

    if (    pVM
        &&  (   VM_FF_IS_PENDING(pVM, VM_FF_EXTERNAL_SUSPENDED_MASK)
             || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_EXTERNAL_SUSPENDED_MASK)
            )
        )
    {
        LogFlow(("VMR3Wait: returns VINF_SUCCESS (FF %#x)\n", pVM->fGlobalForcedActions));
        return VINF_SUCCESS;
    }

    /*
     * Do waiting according to the halt method (so VMR3NotifyFF
     * doesn't have to special case anything).
     */
    PUVM pUVM = pUVCpu->pUVM;
    int rc = g_aHaltMethods[pUVM->vm.s.iHaltMethod].pfnWait(pUVCpu);
    LogFlow(("VMR3WaitU: returns %Rrc (FF %#x)\n", rc, pUVM->pVM ? pUVM->pVM->fGlobalForcedActions : 0));
    return rc;
}


/**
 * Interface that PDMR3Suspend, PDMR3PowerOff and PDMR3Reset uses when they wait
 * for the handling of asynchronous notifications to complete.
 *
 * @returns VINF_SUCCESS unless a fatal error occurred. In the latter
 *          case an appropriate status code is returned.
 * @param   pUVCpu              Pointer to the user mode VMCPU structure.
 * @thread  The emulation thread.
 */
VMMR3_INT_DECL(int) VMR3AsyncPdmNotificationWaitU(PUVMCPU pUVCpu)
{
    LogFlow(("VMR3AsyncPdmNotificationWaitU:\n"));
    return VMR3WaitU(pUVCpu);
}


/**
 * Interface that PDM the helper asynchronous notification completed methods
 * uses for EMT0 when it is waiting inside VMR3AsyncPdmNotificationWaitU().
 *
 * @param   pUVM                Pointer to the user mode VM structure.
 */
VMMR3_INT_DECL(void) VMR3AsyncPdmNotificationWakeupU(PUVM pUVM)
{
    LogFlow(("VMR3AsyncPdmNotificationWakeupU:\n"));
    VM_FF_SET(pUVM->pVM, VM_FF_REQUEST); /* this will have to do for now. */
    g_aHaltMethods[pUVM->vm.s.iHaltMethod].pfnNotifyCpuFF(&pUVM->aCpus[0], 0 /*fFlags*/);
}


/**
 * Rendezvous callback that will be called once.
 *
 * @returns VBox strict status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   pvUser              The new g_aHaltMethods index.
 */
static DECLCALLBACK(VBOXSTRICTRC) vmR3SetHaltMethodCallback(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    PUVM        pUVM = pVM->pUVM;
    uintptr_t   i    = (uintptr_t)pvUser;
    Assert(i < RT_ELEMENTS(g_aHaltMethods));
    NOREF(pVCpu);

    /*
     * Terminate the old one.
     */
    if (    pUVM->vm.s.enmHaltMethod != VMHALTMETHOD_INVALID
        &&  g_aHaltMethods[pUVM->vm.s.iHaltMethod].pfnTerm)
    {
        g_aHaltMethods[pUVM->vm.s.iHaltMethod].pfnTerm(pUVM);
        pUVM->vm.s.enmHaltMethod = VMHALTMETHOD_INVALID;
    }

    /* Assert that the failure fallback is where we expect. */
    Assert(g_aHaltMethods[0].enmHaltMethod == VMHALTMETHOD_BOOTSTRAP);
    Assert(!g_aHaltMethods[0].pfnTerm && !g_aHaltMethods[0].pfnInit);

    /*
     * Init the new one.
     */
    int rc = VINF_SUCCESS;
    memset(&pUVM->vm.s.Halt, 0, sizeof(pUVM->vm.s.Halt));
    if (g_aHaltMethods[i].pfnInit)
    {
        rc = g_aHaltMethods[i].pfnInit(pUVM);
        if (RT_FAILURE(rc))
        {
            /* Fall back on the bootstrap method. This requires no
               init/term (see assertion above), and will always work. */
            AssertLogRelRC(rc);
            i = 0;
        }
    }

    /*
     * Commit it.
     */
    pUVM->vm.s.enmHaltMethod = g_aHaltMethods[i].enmHaltMethod;
    ASMAtomicWriteU32(&pUVM->vm.s.iHaltMethod, i);

    return rc;
}


/**
 * Changes the halt method.
 *
 * @returns VBox status code.
 * @param   pUVM            Pointer to the user mode VM structure.
 * @param   enmHaltMethod   The new halt method.
 * @thread  EMT.
 */
int vmR3SetHaltMethodU(PUVM pUVM, VMHALTMETHOD enmHaltMethod)
{
    PVM pVM = pUVM->pVM; Assert(pVM);
    VM_ASSERT_EMT(pVM);
    AssertReturn(enmHaltMethod > VMHALTMETHOD_INVALID && enmHaltMethod < VMHALTMETHOD_END, VERR_INVALID_PARAMETER);

    /*
     * Resolve default (can be overridden in the configuration).
     */
    if (enmHaltMethod == VMHALTMETHOD_DEFAULT)
    {
        uint32_t u32;
        int rc = CFGMR3QueryU32(CFGMR3GetChild(CFGMR3GetRoot(pVM), "VM"), "HaltMethod", &u32);
        if (RT_SUCCESS(rc))
        {
            enmHaltMethod = (VMHALTMETHOD)u32;
            if (enmHaltMethod <= VMHALTMETHOD_INVALID || enmHaltMethod >= VMHALTMETHOD_END)
                return VMSetError(pVM, VERR_INVALID_PARAMETER, RT_SRC_POS, N_("Invalid VM/HaltMethod value %d"), enmHaltMethod);
        }
        else if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_CHILD_NOT_FOUND)
            return VMSetError(pVM, rc, RT_SRC_POS, N_("Failed to Query VM/HaltMethod as uint32_t"));
        else
            enmHaltMethod = VMHALTMETHOD_GLOBAL_1;
            //enmHaltMethod = VMHALTMETHOD_1;
            //enmHaltMethod = VMHALTMETHOD_OLD;
    }
    LogRel(("VMEmt: Halt method %s (%d)\n", vmR3GetHaltMethodName(enmHaltMethod), enmHaltMethod));

    /*
     * Find the descriptor.
     */
    unsigned i = 0;
    while (     i < RT_ELEMENTS(g_aHaltMethods)
           &&   g_aHaltMethods[i].enmHaltMethod != enmHaltMethod)
        i++;
    AssertReturn(i < RT_ELEMENTS(g_aHaltMethods), VERR_INVALID_PARAMETER);

    /*
     * This needs to be done while the other EMTs are not sleeping or otherwise messing around.
     */
    return VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, vmR3SetHaltMethodCallback, (void *)(uintptr_t)i);
}


/**
 * Special interface for implementing a HLT-like port on a device.
 *
 * This can be called directly from device code, provide the device is trusted
 * to access the VMM directly.  Since we may not have an accurate register set
 * and the caller certainly shouldn't (device code does not access CPU
 * registers), this function will return when interrupts are pending regardless
 * of the actual EFLAGS.IF state.
 *
 * @returns VBox error status (never informational statuses).
 * @param   pVM                 The cross context VM structure.
 * @param   idCpu               The id of the calling EMT.
 */
VMMR3DECL(int) VMR3WaitForDeviceReady(PVM pVM, VMCPUID idCpu)
{
    /*
     * Validate caller and resolve the CPU ID.
     */
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(idCpu < pVM->cCpus, VERR_INVALID_CPU_ID);
    PVMCPU pVCpu = &pVM->aCpus[idCpu];
    VMCPU_ASSERT_EMT_RETURN(pVCpu, VERR_VM_THREAD_NOT_EMT);

    /*
     * Tag along with the HLT mechanics for now.
     */
    int rc = VMR3WaitHalted(pVM, pVCpu, false /*fIgnoreInterrupts*/);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;
    return rc;
}


/**
 * Wakes up a CPU that has called VMR3WaitForDeviceReady.
 *
 * @returns VBox error status (never informational statuses).
 * @param   pVM                 The cross context VM structure.
 * @param   idCpu               The id of the calling EMT.
 */
VMMR3DECL(int) VMR3NotifyCpuDeviceReady(PVM pVM, VMCPUID idCpu)
{
    /*
     * Validate caller and resolve the CPU ID.
     */
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(idCpu < pVM->cCpus, VERR_INVALID_CPU_ID);
    PVMCPU pVCpu = &pVM->aCpus[idCpu];

    /*
     * Pretend it was an FF that got set since we've got logic for that already.
     */
    VMR3NotifyCpuFFU(pVCpu->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM);
    return VINF_SUCCESS;
}


/**
 * Returns the number of active EMTs.
 *
 * This is used by the rendezvous code during VM destruction to avoid waiting
 * for EMTs that aren't around any more.
 *
 * @returns Number of active EMTs.  0 if invalid parameter.
 * @param   pUVM                The user mode VM structure.
 */
VMMR3_INT_DECL(uint32_t) VMR3GetActiveEmts(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, 0);
    return pUVM->vm.s.cActiveEmts;
}


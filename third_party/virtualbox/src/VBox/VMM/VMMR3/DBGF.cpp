/* $Id: DBGF.cpp $ */
/** @file
 * DBGF - Debugger Facility.
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


/** @page   pg_dbgf     DBGF - The Debugger Facility
 *
 * The purpose of the DBGF is to provide an interface for debuggers to
 * manipulate the VMM without having to mess up the source code for each of
 * them. The DBGF is always built in and will always work when a debugger
 * attaches to the VM. The DBGF provides the basic debugger features, such as
 * halting execution, handling breakpoints, single step execution, instruction
 * disassembly, info querying, OS specific diggers, symbol and module
 * management.
 *
 * The interface is working in a manner similar to the win32, linux and os2
 * debugger interfaces. The interface has an asynchronous nature. This comes
 * from the fact that the VMM and the Debugger are running in different threads.
 * They are referred to as the "emulation thread" and the "debugger thread", or
 * as the "ping thread" and the "pong thread, respectivly. (The last set of
 * names comes from the use of the Ping-Pong synchronization construct from the
 * RTSem API.)
 *
 * @see grp_dbgf
 *
 *
 * @section sec_dbgf_scenario   Usage Scenario
 *
 * The debugger starts by attaching to the VM. For practical reasons we limit the
 * number of concurrently attached debuggers to 1 per VM. The action of
 * attaching to the VM causes the VM to check and generate debug events.
 *
 * The debugger then will wait/poll for debug events and issue commands.
 *
 * The waiting and polling is done by the DBGFEventWait() function. It will wait
 * for the emulation thread to send a ping, thus indicating that there is an
 * event waiting to be processed.
 *
 * An event can be a response to a command issued previously, the hitting of a
 * breakpoint, or running into a bad/fatal VMM condition. The debugger now has
 * the ping and must respond to the event at hand - the VMM is waiting. This
 * usually means that the user of the debugger must do something, but it doesn't
 * have to. The debugger is free to call any DBGF function (nearly at least)
 * while processing the event.
 *
 * Typically the user will issue a request for the execution to be resumed, so
 * the debugger calls DBGFResume() and goes back to waiting/polling for events.
 *
 * When the user eventually terminates the debugging session or selects another
 * VM, the debugger detaches from the VM. This means that breakpoints are
 * disabled and that the emulation thread no longer polls for debugger commands.
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGF
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/selm.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include "DBGFInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/asm.h>
#include <iprt/time.h>
#include <iprt/assert.h>
#include <iprt/stream.h>
#include <iprt/env.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Instruction type returned by dbgfStepGetCurInstrType.
 */
typedef enum DBGFSTEPINSTRTYPE
{
    DBGFSTEPINSTRTYPE_INVALID = 0,
    DBGFSTEPINSTRTYPE_OTHER,
    DBGFSTEPINSTRTYPE_RET,
    DBGFSTEPINSTRTYPE_CALL,
    DBGFSTEPINSTRTYPE_END,
    DBGFSTEPINSTRTYPE_32BIT_HACK = 0x7fffffff
} DBGFSTEPINSTRTYPE;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int dbgfR3VMMWait(PVM pVM);
static int dbgfR3VMMCmd(PVM pVM, DBGFCMD enmCmd, PDBGFCMDDATA pCmdData, bool *pfResumeExecution);
static DECLCALLBACK(int) dbgfR3Attach(PVM pVM);
static DBGFSTEPINSTRTYPE dbgfStepGetCurInstrType(PVM pVM, PVMCPU pVCpu);
static bool dbgfStepAreWeThereYet(PVM pVM,  PVMCPU pVCpu);


/**
 * Sets the VMM Debug Command variable.
 *
 * @returns Previous command.
 * @param   pVM     The cross context VM structure.
 * @param   enmCmd  The command.
 */
DECLINLINE(DBGFCMD) dbgfR3SetCmd(PVM pVM, DBGFCMD enmCmd)
{
    DBGFCMD rc;
    if (enmCmd == DBGFCMD_NO_COMMAND)
    {
        Log2(("DBGF: Setting command to %d (DBGFCMD_NO_COMMAND)\n", enmCmd));
        rc = (DBGFCMD)ASMAtomicXchgU32((uint32_t volatile *)(void *)&pVM->dbgf.s.enmVMMCmd, enmCmd);
        VM_FF_CLEAR(pVM, VM_FF_DBGF);
    }
    else
    {
        Log2(("DBGF: Setting command to %d\n", enmCmd));
        AssertMsg(pVM->dbgf.s.enmVMMCmd == DBGFCMD_NO_COMMAND, ("enmCmd=%d enmVMMCmd=%d\n", enmCmd, pVM->dbgf.s.enmVMMCmd));
        rc = (DBGFCMD)ASMAtomicXchgU32((uint32_t volatile *)(void *)&pVM->dbgf.s.enmVMMCmd, enmCmd);
        VM_FF_SET(pVM, VM_FF_DBGF);
        VMR3NotifyGlobalFFU(pVM->pUVM, 0 /* didn't notify REM */);
    }
    return rc;
}


/**
 * Initializes the DBGF.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) DBGFR3Init(PVM pVM)
{
    PUVM pUVM = pVM->pUVM;
    AssertCompile(sizeof(pUVM->dbgf.s)          <= sizeof(pUVM->dbgf.padding));
    AssertCompile(sizeof(pUVM->aCpus[0].dbgf.s) <= sizeof(pUVM->aCpus[0].dbgf.padding));

    pVM->dbgf.s.SteppingFilter.idCpu = NIL_VMCPUID;

    /*
     * The usual sideways mountain climbing style of init:
     */
    int rc = dbgfR3InfoInit(pUVM); /* (First, initalizes the shared critical section.) */
    if (RT_SUCCESS(rc))
    {
        rc = dbgfR3TraceInit(pVM);
        if (RT_SUCCESS(rc))
        {
            rc = dbgfR3RegInit(pUVM);
            if (RT_SUCCESS(rc))
            {
                rc = dbgfR3AsInit(pUVM);
                if (RT_SUCCESS(rc))
                {
                    rc = dbgfR3BpInit(pVM);
                    if (RT_SUCCESS(rc))
                    {
                        rc = dbgfR3OSInit(pUVM);
                        if (RT_SUCCESS(rc))
                        {
                            rc = dbgfR3PlugInInit(pUVM);
                            if (RT_SUCCESS(rc))
                            {
                                return VINF_SUCCESS;
                            }
                            dbgfR3OSTerm(pUVM);
                        }
                    }
                    dbgfR3AsTerm(pUVM);
                }
                dbgfR3RegTerm(pUVM);
            }
            dbgfR3TraceTerm(pVM);
        }
        dbgfR3InfoTerm(pUVM);
    }
    return rc;
}


/**
 * Terminates and cleans up resources allocated by the DBGF.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) DBGFR3Term(PVM pVM)
{
    PUVM pUVM = pVM->pUVM;

    dbgfR3PlugInTerm(pUVM);
    dbgfR3OSTerm(pUVM);
    dbgfR3AsTerm(pUVM);
    dbgfR3RegTerm(pUVM);
    dbgfR3TraceTerm(pVM);
    dbgfR3InfoTerm(pUVM);

    return VINF_SUCCESS;
}


/**
 * Called when the VM is powered off to detach debuggers.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(void) DBGFR3PowerOff(PVM pVM)
{

    /*
     * Send a termination event to any attached debugger.
     */
    /* wait to become the speaker (we should already be that). */
    if (    pVM->dbgf.s.fAttached
        &&  RTSemPingShouldWait(&pVM->dbgf.s.PingPong))
        RTSemPingWait(&pVM->dbgf.s.PingPong, 5000);

    if (pVM->dbgf.s.fAttached)
    {
        /* Just mark it as detached if we're not in a position to send a power
           off event.  It should fail later on. */
        if (!RTSemPingIsSpeaker(&pVM->dbgf.s.PingPong))
        {
            ASMAtomicWriteBool(&pVM->dbgf.s.fAttached, false);
            if (RTSemPingIsSpeaker(&pVM->dbgf.s.PingPong))
                ASMAtomicWriteBool(&pVM->dbgf.s.fAttached, true);
        }

        if (RTSemPingIsSpeaker(&pVM->dbgf.s.PingPong))
        {
            /* Try send the power off event. */
            int rc;
            DBGFCMD enmCmd = dbgfR3SetCmd(pVM, DBGFCMD_NO_COMMAND);
            if (enmCmd == DBGFCMD_DETACH_DEBUGGER)
                /* the debugger beat us to initiating the detaching. */
                rc = VINF_SUCCESS;
            else
            {
                /* ignore the command (if any). */
                enmCmd = DBGFCMD_NO_COMMAND;
                pVM->dbgf.s.DbgEvent.enmType = DBGFEVENT_POWERING_OFF;
                pVM->dbgf.s.DbgEvent.enmCtx  = DBGFEVENTCTX_OTHER;
                rc = RTSemPing(&pVM->dbgf.s.PingPong);
            }

            /*
             * Process commands and priority requests until we get a command
             * indicating that the debugger has detached.
             */
            uint32_t cPollHack = 1;
            PVMCPU   pVCpu     = VMMGetCpu(pVM);
            while (RT_SUCCESS(rc))
            {
                if (enmCmd != DBGFCMD_NO_COMMAND)
                {
                    /* process command */
                    bool fResumeExecution;
                    DBGFCMDDATA CmdData = pVM->dbgf.s.VMMCmdData;
                    rc = dbgfR3VMMCmd(pVM, enmCmd, &CmdData, &fResumeExecution);
                    if (enmCmd == DBGFCMD_DETACHED_DEBUGGER)
                        break;
                    enmCmd = DBGFCMD_NO_COMMAND;
                }
                else
                {
                    /* Wait for new command, processing pending priority requests
                       first.  The request processing is a bit crazy, but
                       unfortunately required by plugin unloading. */
                    if (   VM_FF_IS_PENDING(pVM, VM_FF_REQUEST)
                        || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_REQUEST))
                    {
                        LogFlow(("DBGFR3PowerOff: Processes priority requests...\n"));
                        rc = VMR3ReqProcessU(pVM->pUVM, VMCPUID_ANY, true /*fPriorityOnly*/);
                        if (rc == VINF_SUCCESS)
                            rc = VMR3ReqProcessU(pVM->pUVM, pVCpu->idCpu, true /*fPriorityOnly*/);
                        LogFlow(("DBGFR3PowerOff: VMR3ReqProcess -> %Rrc\n", rc));
                        cPollHack = 1;
                    }
                    /* Need to handle rendezvous too, for generic debug event management. */
                    else if (VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS))
                    {
                        rc = VMMR3EmtRendezvousFF(pVM, pVCpu);
                        AssertLogRel(rc == VINF_SUCCESS);
                        cPollHack = 1;
                    }
                    else if (cPollHack < 120)
                        cPollHack++;

                    rc = RTSemPingWait(&pVM->dbgf.s.PingPong, cPollHack);
                    if (RT_SUCCESS(rc))
                        enmCmd = dbgfR3SetCmd(pVM, DBGFCMD_NO_COMMAND);
                    else if (rc == VERR_TIMEOUT)
                        rc = VINF_SUCCESS;
                }
            }

            /*
             * Clear the FF so we won't get confused later on.
             */
            VM_FF_CLEAR(pVM, VM_FF_DBGF);
        }
    }
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * @param   pVM         The cross context VM structure.
 * @param   offDelta    Relocation delta relative to old location.
 */
VMMR3_INT_DECL(void) DBGFR3Relocate(PVM pVM, RTGCINTPTR offDelta)
{
    dbgfR3TraceRelocate(pVM);
    dbgfR3AsRelocate(pVM->pUVM, offDelta);
}


/**
 * Waits a little while for a debuggger to attach.
 *
 * @returns True is a debugger have attached.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context per CPU structure.
 * @param   enmEvent    Event.
 *
 * @thread  EMT(pVCpu)
 */
bool dbgfR3WaitForAttach(PVM pVM, PVMCPU pVCpu, DBGFEVENTTYPE enmEvent)
{
    /*
     * First a message.
     */
#ifndef RT_OS_L4

# if !defined(DEBUG) || defined(DEBUG_sandervl) || defined(DEBUG_frank) || defined(IEM_VERIFICATION_MODE)
    int cWait = 10;
# else
    int cWait = HMIsEnabled(pVM)
             && (   enmEvent == DBGFEVENT_ASSERTION_HYPER
                 || enmEvent == DBGFEVENT_FATAL_ERROR)
             && !RTEnvExist("VBOX_DBGF_WAIT_FOR_ATTACH")
              ? 10
              : 150;
# endif
    RTStrmPrintf(g_pStdErr, "DBGF: No debugger attached, waiting %d second%s for one to attach (event=%d)\n",
                 cWait / 10, cWait != 10 ? "s" : "", enmEvent);
    RTStrmFlush(g_pStdErr);
    while (cWait > 0)
    {
        RTThreadSleep(100);
        if (pVM->dbgf.s.fAttached)
        {
            RTStrmPrintf(g_pStdErr, "Attached!\n");
            RTStrmFlush(g_pStdErr);
            return true;
        }

        /* Process priority stuff. */
        if (   VM_FF_IS_PENDING(pVM, VM_FF_REQUEST)
            || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_REQUEST))
        {
            int rc = VMR3ReqProcessU(pVM->pUVM, VMCPUID_ANY, true /*fPriorityOnly*/);
            if (rc == VINF_SUCCESS)
                rc = VMR3ReqProcessU(pVM->pUVM, pVCpu->idCpu, true /*fPriorityOnly*/);
            if (rc != VINF_SUCCESS)
            {
                RTStrmPrintf(g_pStdErr, "[rcReq=%Rrc, ignored!]", rc);
                RTStrmFlush(g_pStdErr);
            }
        }

        /* next */
        if (!(cWait % 10))
        {
            RTStrmPrintf(g_pStdErr, "%d.", cWait / 10);
            RTStrmFlush(g_pStdErr);
        }
        cWait--;
    }
#endif

    RTStrmPrintf(g_pStdErr, "Stopping the VM!\n");
    RTStrmFlush(g_pStdErr);
    return false;
}


/**
 * Forced action callback.
 *
 * The VMM will call this from it's main loop when either VM_FF_DBGF or
 * VMCPU_FF_DBGF are set.
 *
 * The function checks for and executes pending commands from the debugger.
 * Then it checks for pending debug events and serves these.
 *
 * @returns VINF_SUCCESS normally.
 * @returns VERR_DBGF_RAISE_FATAL_ERROR to pretend a fatal error happened.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context per CPU structure.
 */
VMMR3_INT_DECL(int) DBGFR3VMMForcedAction(PVM pVM, PVMCPU pVCpu)
{
    VBOXSTRICTRC rcStrict = VINF_SUCCESS;

    if (VM_FF_TEST_AND_CLEAR(pVM, VM_FF_DBGF))
    {
        /*
         * Command pending? Process it.
         */
        if (pVM->dbgf.s.enmVMMCmd != DBGFCMD_NO_COMMAND)
        {
            bool            fResumeExecution;
            DBGFCMDDATA     CmdData = pVM->dbgf.s.VMMCmdData;
            DBGFCMD         enmCmd = dbgfR3SetCmd(pVM, DBGFCMD_NO_COMMAND);
            rcStrict = dbgfR3VMMCmd(pVM, enmCmd, &CmdData, &fResumeExecution);
            if (!fResumeExecution)
                rcStrict = dbgfR3VMMWait(pVM);
        }
    }

    /*
     * Dispatch pending events.
     */
    if (VMCPU_FF_TEST_AND_CLEAR(pVCpu, VMCPU_FF_DBGF))
    {
        if (   pVCpu->dbgf.s.cEvents > 0
            && pVCpu->dbgf.s.aEvents[pVCpu->dbgf.s.cEvents - 1].enmState == DBGFEVENTSTATE_CURRENT)
        {
            VBOXSTRICTRC rcStrict2 = DBGFR3EventHandlePending(pVM, pVCpu);
            if (   rcStrict2 != VINF_SUCCESS
                && (   rcStrict == VINF_SUCCESS
                    || RT_FAILURE(rcStrict2)
                    || rcStrict2 < rcStrict) ) /** @todo oversimplified? */
                rcStrict = rcStrict2;
        }
    }

    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * Flag whether the event implies that we're stopped in the hypervisor code
 * and have to block certain operations.
 *
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    The event.
 */
static void dbgfR3EventSetStoppedInHyperFlag(PVM pVM, DBGFEVENTTYPE enmEvent)
{
    switch (enmEvent)
    {
        case DBGFEVENT_STEPPED_HYPER:
        case DBGFEVENT_ASSERTION_HYPER:
        case DBGFEVENT_BREAKPOINT_HYPER:
            pVM->dbgf.s.fStoppedInHyper = true;
            break;
        default:
            pVM->dbgf.s.fStoppedInHyper = false;
            break;
    }
}


/**
 * Try to determine the event context.
 *
 * @returns debug event context.
 * @param   pVM         The cross context VM structure.
 */
static DBGFEVENTCTX dbgfR3FigureEventCtx(PVM pVM)
{
    /** @todo SMP support! */
    PVMCPU pVCpu = &pVM->aCpus[0];

    switch (EMGetState(pVCpu))
    {
        case EMSTATE_RAW:
        case EMSTATE_DEBUG_GUEST_RAW:
            return DBGFEVENTCTX_RAW;

        case EMSTATE_REM:
        case EMSTATE_DEBUG_GUEST_REM:
            return DBGFEVENTCTX_REM;

        case EMSTATE_DEBUG_HYPER:
        case EMSTATE_GURU_MEDITATION:
            return DBGFEVENTCTX_HYPER;

        default:
            return DBGFEVENTCTX_OTHER;
    }
}

/**
 * The common event prologue code.
 * It will set the 'stopped-in-hyper' flag, make sure someone is attached,
 * and perhaps process any high priority pending actions (none yet).
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    The event to be sent.
 */
static int dbgfR3EventPrologue(PVM pVM, DBGFEVENTTYPE enmEvent)
{
    /** @todo SMP */
    PVMCPU pVCpu = VMMGetCpu(pVM);

    /*
     * Check if a debugger is attached.
     */
    if (    !pVM->dbgf.s.fAttached
        &&  !dbgfR3WaitForAttach(pVM, pVCpu, enmEvent))
    {
        Log(("DBGFR3VMMEventSrc: enmEvent=%d - debugger not attached\n", enmEvent));
        return VERR_DBGF_NOT_ATTACHED;
    }

    /*
     * Sync back the state from the REM.
     */
    dbgfR3EventSetStoppedInHyperFlag(pVM, enmEvent);
#ifdef VBOX_WITH_REM
    if (!pVM->dbgf.s.fStoppedInHyper)
        REMR3StateUpdate(pVM, pVCpu);
#endif

    /*
     * Look thru pending commands and finish those which make sense now.
     */
    /** @todo Process/purge pending commands. */
    //int rc = DBGFR3VMMForcedAction(pVM);
    return VINF_SUCCESS;
}


/**
 * Sends the event in the event buffer.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
static int dbgfR3SendEvent(PVM pVM)
{
    pVM->dbgf.s.SteppingFilter.idCpu = NIL_VMCPUID;

    int rc = RTSemPing(&pVM->dbgf.s.PingPong);
    if (RT_SUCCESS(rc))
        rc = dbgfR3VMMWait(pVM);

    pVM->dbgf.s.fStoppedInHyper = false;
    /** @todo sync VMM -> REM after exitting the debugger. everything may change while in the debugger! */
    return rc;
}


/**
 * Processes a pending event on the current CPU.
 *
 * This is called by EM in response to VINF_EM_DBG_EVENT.
 *
 * @returns Strict VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context per CPU structure.
 *
 * @thread  EMT(pVCpu)
 */
VMMR3_INT_DECL(VBOXSTRICTRC) DBGFR3EventHandlePending(PVM pVM, PVMCPU pVCpu)
{
    VMCPU_ASSERT_EMT(pVCpu);
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_DBGF);

    /*
     * Check that we've got an event first.
     */
    AssertReturn(pVCpu->dbgf.s.cEvents > 0, VINF_SUCCESS);
    AssertReturn(pVCpu->dbgf.s.aEvents[pVCpu->dbgf.s.cEvents - 1].enmState == DBGFEVENTSTATE_CURRENT, VINF_SUCCESS);
    PDBGFEVENT pEvent = &pVCpu->dbgf.s.aEvents[pVCpu->dbgf.s.cEvents - 1].Event;

    /*
     * Make sure we've got a debugger and is allowed to speak to it.
     */
    int rc = dbgfR3EventPrologue(pVM, pEvent->enmType);
    if (RT_FAILURE(rc))
    {
        /** @todo drop them events?   */
        return rc;
    }

/** @todo SMP + debugger speaker logic  */
    /*
     * Copy the event over and mark it as ignore.
     */
    pVM->dbgf.s.DbgEvent = *pEvent;
    pVCpu->dbgf.s.aEvents[pVCpu->dbgf.s.cEvents - 1].enmState = DBGFEVENTSTATE_IGNORE;
    return dbgfR3SendEvent(pVM);
}


/**
 * Send a generic debugger event which takes no data.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    The event to send.
 * @internal
 */
VMMR3DECL(int) DBGFR3Event(PVM pVM, DBGFEVENTTYPE enmEvent)
{
    /*
     * Do stepping filtering.
     */
    /** @todo Would be better if we did some of this inside the execution
     *        engines. */
    if (   enmEvent == DBGFEVENT_STEPPED
        || enmEvent == DBGFEVENT_STEPPED_HYPER)
    {
        if (!dbgfStepAreWeThereYet(pVM, VMMGetCpu(pVM)))
            return VINF_EM_DBG_STEP;
    }

    int rc = dbgfR3EventPrologue(pVM, enmEvent);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Send the event and process the reply communication.
     */
    pVM->dbgf.s.DbgEvent.enmType = enmEvent;
    pVM->dbgf.s.DbgEvent.enmCtx  = dbgfR3FigureEventCtx(pVM);
    return dbgfR3SendEvent(pVM);
}


/**
 * Send a debugger event which takes the full source file location.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    The event to send.
 * @param   pszFile     Source file.
 * @param   uLine       Line number in source file.
 * @param   pszFunction Function name.
 * @param   pszFormat   Message which accompanies the event.
 * @param   ...         Message arguments.
 * @internal
 */
VMMR3DECL(int) DBGFR3EventSrc(PVM pVM, DBGFEVENTTYPE enmEvent, const char *pszFile, unsigned uLine, const char *pszFunction, const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    int rc = DBGFR3EventSrcV(pVM, enmEvent, pszFile, uLine, pszFunction, pszFormat, args);
    va_end(args);
    return rc;
}


/**
 * Send a debugger event which takes the full source file location.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    The event to send.
 * @param   pszFile     Source file.
 * @param   uLine       Line number in source file.
 * @param   pszFunction Function name.
 * @param   pszFormat   Message which accompanies the event.
 * @param   args        Message arguments.
 * @internal
 */
VMMR3DECL(int) DBGFR3EventSrcV(PVM pVM, DBGFEVENTTYPE enmEvent, const char *pszFile, unsigned uLine, const char *pszFunction, const char *pszFormat, va_list args)
{
    int rc = dbgfR3EventPrologue(pVM, enmEvent);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Format the message.
     */
    char   *pszMessage = NULL;
    char    szMessage[8192];
    if (pszFormat && *pszFormat)
    {
        pszMessage = &szMessage[0];
        RTStrPrintfV(szMessage, sizeof(szMessage), pszFormat, args);
    }

    /*
     * Send the event and process the reply communication.
     */
    pVM->dbgf.s.DbgEvent.enmType = enmEvent;
    pVM->dbgf.s.DbgEvent.enmCtx  = dbgfR3FigureEventCtx(pVM);
    pVM->dbgf.s.DbgEvent.u.Src.pszFile      = pszFile;
    pVM->dbgf.s.DbgEvent.u.Src.uLine        = uLine;
    pVM->dbgf.s.DbgEvent.u.Src.pszFunction  = pszFunction;
    pVM->dbgf.s.DbgEvent.u.Src.pszMessage   = pszMessage;
    return dbgfR3SendEvent(pVM);
}


/**
 * Send a debugger event which takes the two assertion messages.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    The event to send.
 * @param   pszMsg1     First assertion message.
 * @param   pszMsg2     Second assertion message.
 */
VMMR3_INT_DECL(int) DBGFR3EventAssertion(PVM pVM, DBGFEVENTTYPE enmEvent, const char *pszMsg1, const char *pszMsg2)
{
    int rc = dbgfR3EventPrologue(pVM, enmEvent);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Send the event and process the reply communication.
     */
    pVM->dbgf.s.DbgEvent.enmType = enmEvent;
    pVM->dbgf.s.DbgEvent.enmCtx  = dbgfR3FigureEventCtx(pVM);
    pVM->dbgf.s.DbgEvent.u.Assert.pszMsg1 = pszMsg1;
    pVM->dbgf.s.DbgEvent.u.Assert.pszMsg2 = pszMsg2;
    return dbgfR3SendEvent(pVM);
}


/**
 * Breakpoint was hit somewhere.
 * Figure out which breakpoint it is and notify the debugger.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    DBGFEVENT_BREAKPOINT_HYPER or DBGFEVENT_BREAKPOINT.
 */
VMMR3_INT_DECL(int) DBGFR3EventBreakpoint(PVM pVM, DBGFEVENTTYPE enmEvent)
{
    int rc = dbgfR3EventPrologue(pVM, enmEvent);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Send the event and process the reply communication.
     */
    /** @todo SMP */
    PVMCPU pVCpu = VMMGetCpu0(pVM);

    pVM->dbgf.s.DbgEvent.enmType = enmEvent;
    RTUINT iBp = pVM->dbgf.s.DbgEvent.u.Bp.iBp = pVCpu->dbgf.s.iActiveBp;
    pVCpu->dbgf.s.iActiveBp = ~0U;
    if (iBp != ~0U)
        pVM->dbgf.s.DbgEvent.enmCtx = DBGFEVENTCTX_RAW;
    else
    {
        /* REM breakpoints has be been searched for. */
#if 0   /** @todo get flat PC api! */
        uint32_t eip = CPUMGetGuestEIP(pVM);
#else
        /** @todo SMP support!! */
        PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(VMMGetCpu(pVM));
        RTGCPTR  eip = pCtx->rip + pCtx->cs.u64Base;
#endif
        for (size_t i = 0; i < RT_ELEMENTS(pVM->dbgf.s.aBreakpoints); i++)
            if (    pVM->dbgf.s.aBreakpoints[i].enmType == DBGFBPTYPE_REM
                &&  pVM->dbgf.s.aBreakpoints[i].u.Rem.GCPtr == eip)
            {
                pVM->dbgf.s.DbgEvent.u.Bp.iBp = pVM->dbgf.s.aBreakpoints[i].iBp;
                break;
            }
        AssertMsg(pVM->dbgf.s.DbgEvent.u.Bp.iBp != ~0U, ("eip=%08x\n", eip));
        pVM->dbgf.s.DbgEvent.enmCtx = DBGFEVENTCTX_REM;
    }
    return dbgfR3SendEvent(pVM);
}


/**
 * Waits for the debugger to respond.
 *
 * @returns VBox status code. (clearify)
 * @param   pVM     The cross context VM structure.
 */
static int dbgfR3VMMWait(PVM pVM)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);

    LogFlow(("dbgfR3VMMWait:\n"));
    int rcRet = VINF_SUCCESS;

    /*
     * Waits for the debugger to reply (i.e. issue an command).
     */
    for (;;)
    {
        /*
         * Wait.
         */
        uint32_t cPollHack = 1; /** @todo this interface is horrible now that we're using lots of VMR3ReqCall stuff all over DBGF. */
        for (;;)
        {
            int rc;
            if (    !VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS | VM_FF_REQUEST)
                &&  !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_REQUEST))
            {
                rc = RTSemPingWait(&pVM->dbgf.s.PingPong, cPollHack);
                if (RT_SUCCESS(rc))
                    break;
                if (rc != VERR_TIMEOUT)
                {
                    LogFlow(("dbgfR3VMMWait: returns %Rrc\n", rc));
                    return rc;
                }
            }

            if (VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS))
            {
                rc = VMMR3EmtRendezvousFF(pVM, pVCpu);
                cPollHack = 1;
            }
            else if (   VM_FF_IS_PENDING(pVM, VM_FF_REQUEST)
                     || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_REQUEST))
            {
                LogFlow(("dbgfR3VMMWait: Processes requests...\n"));
                rc = VMR3ReqProcessU(pVM->pUVM, VMCPUID_ANY, false /*fPriorityOnly*/);
                if (rc == VINF_SUCCESS)
                    rc = VMR3ReqProcessU(pVM->pUVM, pVCpu->idCpu, false /*fPriorityOnly*/);
                LogFlow(("dbgfR3VMMWait: VMR3ReqProcess -> %Rrc rcRet=%Rrc\n", rc, rcRet));
                cPollHack = 1;
            }
            else
            {
                rc = VINF_SUCCESS;
                if (cPollHack < 120)
                    cPollHack++;
            }

            if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
            {
                switch (rc)
                {
                    case VINF_EM_DBG_BREAKPOINT:
                    case VINF_EM_DBG_STEPPED:
                    case VINF_EM_DBG_STEP:
                    case VINF_EM_DBG_STOP:
                    case VINF_EM_DBG_EVENT:
                        AssertMsgFailed(("rc=%Rrc\n", rc));
                        break;

                    /* return straight away */
                    case VINF_EM_TERMINATE:
                    case VINF_EM_OFF:
                        LogFlow(("dbgfR3VMMWait: returns %Rrc\n", rc));
                        return rc;

                    /* remember return code. */
                    default:
                        AssertReleaseMsgFailed(("rc=%Rrc is not in the switch!\n", rc));
                        RT_FALL_THRU();
                    case VINF_EM_RESET:
                    case VINF_EM_SUSPEND:
                    case VINF_EM_HALT:
                    case VINF_EM_RESUME:
                    case VINF_EM_RESCHEDULE:
                    case VINF_EM_RESCHEDULE_REM:
                    case VINF_EM_RESCHEDULE_RAW:
                        if (rc < rcRet || rcRet == VINF_SUCCESS)
                            rcRet = rc;
                        break;
                }
            }
            else if (RT_FAILURE(rc))
            {
                LogFlow(("dbgfR3VMMWait: returns %Rrc\n", rc));
                return rc;
            }
        }

        /*
         * Process the command.
         */
        bool            fResumeExecution;
        DBGFCMDDATA     CmdData = pVM->dbgf.s.VMMCmdData;
        DBGFCMD         enmCmd = dbgfR3SetCmd(pVM, DBGFCMD_NO_COMMAND);
        int rc = dbgfR3VMMCmd(pVM, enmCmd, &CmdData, &fResumeExecution);
        if (fResumeExecution)
        {
            if (RT_FAILURE(rc))
                rcRet = rc;
            else if (    rc >= VINF_EM_FIRST
                     &&  rc <= VINF_EM_LAST
                     &&  (rc < rcRet || rcRet == VINF_SUCCESS))
                rcRet = rc;
            LogFlow(("dbgfR3VMMWait: returns %Rrc\n", rcRet));
            return rcRet;
        }
    }
}


/**
 * Executes command from debugger.
 *
 * The caller is responsible for waiting or resuming execution based on the
 * value returned in the *pfResumeExecution indicator.
 *
 * @returns VBox status code. (clearify!)
 * @param   pVM                 The cross context VM structure.
 * @param   enmCmd              The command in question.
 * @param   pCmdData            Pointer to the command data.
 * @param   pfResumeExecution   Where to store the resume execution / continue waiting indicator.
 */
static int dbgfR3VMMCmd(PVM pVM, DBGFCMD enmCmd, PDBGFCMDDATA pCmdData, bool *pfResumeExecution)
{
    bool    fSendEvent;
    bool    fResume;
    int     rc = VINF_SUCCESS;

    NOREF(pCmdData); /* for later */

    switch (enmCmd)
    {
        /*
         * Halt is answered by an event say that we've halted.
         */
        case DBGFCMD_HALT:
        {
            pVM->dbgf.s.DbgEvent.enmType = DBGFEVENT_HALT_DONE;
            pVM->dbgf.s.DbgEvent.enmCtx  = dbgfR3FigureEventCtx(pVM);
            fSendEvent = true;
            fResume = false;
            break;
        }


        /*
         * Resume is not answered we'll just resume execution.
         */
        case DBGFCMD_GO:
        {
            /** @todo SMP */
            PVMCPU pVCpu = VMMGetCpu0(pVM);
            pVCpu->dbgf.s.fSingleSteppingRaw = false;
            fSendEvent = false;
            fResume = true;
            break;
        }

        /** @todo implement (and define) the rest of the commands. */

        /*
         * Disable breakpoints and stuff.
         * Send an everythings cool event to the debugger thread and resume execution.
         */
        case DBGFCMD_DETACH_DEBUGGER:
        {
            ASMAtomicWriteBool(&pVM->dbgf.s.fAttached, false);
            pVM->dbgf.s.DbgEvent.enmType = DBGFEVENT_DETACH_DONE;
            pVM->dbgf.s.DbgEvent.enmCtx  = DBGFEVENTCTX_OTHER;
            pVM->dbgf.s.SteppingFilter.idCpu = NIL_VMCPUID;
            fSendEvent = true;
            fResume = true;
            break;
        }

        /*
         * The debugger has detached successfully.
         * There is no reply to this event.
         */
        case DBGFCMD_DETACHED_DEBUGGER:
        {
            fSendEvent = false;
            fResume = true;
            break;
        }

        /*
         * Single step, with trace into.
         */
        case DBGFCMD_SINGLE_STEP:
        {
            Log2(("Single step\n"));
            /** @todo SMP */
            PVMCPU pVCpu = VMMGetCpu0(pVM);
            if (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_OVER)
            {
                if (dbgfStepGetCurInstrType(pVM, pVCpu) == DBGFSTEPINSTRTYPE_CALL)
                    pVM->dbgf.s.SteppingFilter.uCallDepth++;
            }
            if (pVM->dbgf.s.SteppingFilter.cMaxSteps > 0)
            {
                pVCpu->dbgf.s.fSingleSteppingRaw = true;
                fSendEvent = false;
                fResume = true;
                rc = VINF_EM_DBG_STEP;
            }
            else
            {
                /* Stop after zero steps. Nonsense, but whatever. */
                pVM->dbgf.s.SteppingFilter.idCpu = NIL_VMCPUID;
                pVM->dbgf.s.DbgEvent.enmCtx  = dbgfR3FigureEventCtx(pVM);
                pVM->dbgf.s.DbgEvent.enmType = pVM->dbgf.s.DbgEvent.enmCtx != DBGFEVENTCTX_HYPER
                                             ? DBGFEVENT_STEPPED : DBGFEVENT_STEPPED_HYPER;
                fSendEvent = false;
                fResume = false;
            }
            break;
        }

        /*
         * Default is to send an invalid command event.
         */
        default:
        {
            pVM->dbgf.s.DbgEvent.enmType = DBGFEVENT_INVALID_COMMAND;
            pVM->dbgf.s.DbgEvent.enmCtx  = dbgfR3FigureEventCtx(pVM);
            fSendEvent = true;
            fResume = false;
            break;
        }
    }

    /*
     * Send pending event.
     */
    if (fSendEvent)
    {
        Log2(("DBGF: Emulation thread: sending event %d\n", pVM->dbgf.s.DbgEvent.enmType));
        int rc2 = RTSemPing(&pVM->dbgf.s.PingPong);
        if (RT_FAILURE(rc2))
        {
            AssertRC(rc2);
            *pfResumeExecution = true;
            return rc2;
        }
    }

    /*
     * Return.
     */
    *pfResumeExecution = fResume;
    return rc;
}


/**
 * Attaches a debugger to the specified VM.
 *
 * Only one debugger at a time.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(int) DBGFR3Attach(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    /*
     * Call the VM, use EMT for serialization.
     *
     * Using a priority call here so we can actually attach a debugger during
     * the countdown in dbgfR3WaitForAttach.
     */
    /** @todo SMP */
    return VMR3ReqPriorityCallWait(pVM, VMCPUID_ANY, (PFNRT)dbgfR3Attach, 1, pVM);
}


/**
 * EMT worker for DBGFR3Attach.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
static DECLCALLBACK(int) dbgfR3Attach(PVM pVM)
{
    if (pVM->dbgf.s.fAttached)
    {
        Log(("dbgR3Attach: Debugger already attached\n"));
        return VERR_DBGF_ALREADY_ATTACHED;
    }

    /*
     * Create the Ping-Pong structure.
     */
    int rc = RTSemPingPongInit(&pVM->dbgf.s.PingPong);
    AssertRCReturn(rc, rc);

    /*
     * Set the attached flag.
     */
    ASMAtomicWriteBool(&pVM->dbgf.s.fAttached, true);
    return VINF_SUCCESS;
}


/**
 * Detaches a debugger from the specified VM.
 *
 * Caller must be attached to the VM.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(int) DBGFR3Detach(PUVM pUVM)
{
    LogFlow(("DBGFR3Detach:\n"));
    int rc;

    /*
     * Validate input. The UVM handle shall be valid, the VM handle might be
     * in the processes of being destroyed already, so deal quietly with that.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    if (!VM_IS_VALID_EXT(pVM))
        return VERR_INVALID_VM_HANDLE;

    /*
     * Check if attached.
     */
    if (!pVM->dbgf.s.fAttached)
        return VERR_DBGF_NOT_ATTACHED;

    /*
     * Try send the detach command.
     * Keep in mind that we might be racing EMT, so, be extra careful.
     */
    DBGFCMD enmCmd = dbgfR3SetCmd(pVM, DBGFCMD_DETACH_DEBUGGER);
    if (RTSemPongIsSpeaker(&pVM->dbgf.s.PingPong))
    {
        rc = RTSemPong(&pVM->dbgf.s.PingPong);
        AssertMsgRCReturn(rc, ("Failed to signal emulation thread. rc=%Rrc\n", rc), rc);
        LogRel(("DBGFR3Detach: enmCmd=%d (pong -> ping)\n", enmCmd));
    }

    /*
     * Wait for the OK event.
     */
    rc = RTSemPongWait(&pVM->dbgf.s.PingPong, RT_INDEFINITE_WAIT);
    AssertLogRelMsgRCReturn(rc, ("Wait on detach command failed, rc=%Rrc\n", rc), rc);

    /*
     * Send the notification command indicating that we're really done.
     */
    enmCmd = dbgfR3SetCmd(pVM, DBGFCMD_DETACHED_DEBUGGER);
    rc = RTSemPong(&pVM->dbgf.s.PingPong);
    AssertMsgRCReturn(rc, ("Failed to signal emulation thread. rc=%Rrc\n", rc), rc);

    LogFlowFunc(("returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * Wait for a debug event.
 *
 * @returns VBox status code. Will not return VBOX_INTERRUPTED.
 * @param   pUVM        The user mode VM handle.
 * @param   cMillies    Number of millis to wait.
 * @param   ppEvent     Where to store the event pointer.
 */
VMMR3DECL(int) DBGFR3EventWait(PUVM pUVM, RTMSINTERVAL cMillies, PCDBGFEVENT *ppEvent)
{
    /*
     * Check state.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(pVM->dbgf.s.fAttached, VERR_DBGF_NOT_ATTACHED);
    *ppEvent = NULL;

    /*
     * Wait.
     */
    int rc = RTSemPongWait(&pVM->dbgf.s.PingPong, cMillies);
    if (RT_SUCCESS(rc))
    {
        *ppEvent = &pVM->dbgf.s.DbgEvent;
        Log2(("DBGF: Debugger thread: receiving event %d\n", (*ppEvent)->enmType));
        return VINF_SUCCESS;
    }

    return rc;
}


/**
 * Halts VM execution.
 *
 * After calling this the VM isn't actually halted till an DBGFEVENT_HALT_DONE
 * arrives. Until that time it's not possible to issue any new commands.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(int) DBGFR3Halt(PUVM pUVM)
{
    /*
     * Check state.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(pVM->dbgf.s.fAttached, VERR_DBGF_NOT_ATTACHED);
    RTPINGPONGSPEAKER enmSpeaker = pVM->dbgf.s.PingPong.enmSpeaker;
    if (   enmSpeaker == RTPINGPONGSPEAKER_PONG
        || enmSpeaker == RTPINGPONGSPEAKER_PONG_SIGNALED)
        return VWRN_DBGF_ALREADY_HALTED;

    /*
     * Send command.
     */
    dbgfR3SetCmd(pVM, DBGFCMD_HALT);

    return VINF_SUCCESS;
}


/**
 * Checks if the VM is halted by the debugger.
 *
 * @returns True if halted.
 * @returns False if not halted.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(bool) DBGFR3IsHalted(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    AssertReturn(pVM->dbgf.s.fAttached, false);

    RTPINGPONGSPEAKER enmSpeaker = pVM->dbgf.s.PingPong.enmSpeaker;
    return enmSpeaker == RTPINGPONGSPEAKER_PONG_SIGNALED
        || enmSpeaker == RTPINGPONGSPEAKER_PONG;
}


/**
 * Checks if the debugger can wait for events or not.
 *
 * This function is only used by lazy, multiplexing debuggers. :-)
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if waitable.
 * @retval  VERR_SEM_OUT_OF_TURN if not waitable.
 * @retval  VERR_INVALID_VM_HANDLE if the VM is being (/ has been) destroyed
 *          (not asserted) or if the handle is invalid (asserted).
 * @retval  VERR_DBGF_NOT_ATTACHED if not attached.
 *
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(int) DBGFR3QueryWaitable(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);

    /* Note! There is a slight race here, unfortunately. */
    PVM pVM = pUVM->pVM;
    if (!RT_VALID_PTR(pVM))
        return VERR_INVALID_VM_HANDLE;
    if (pVM->enmVMState >= VMSTATE_DESTROYING)
        return VERR_INVALID_VM_HANDLE;
    if (!pVM->dbgf.s.fAttached)
        return VERR_DBGF_NOT_ATTACHED;

    if (!RTSemPongShouldWait(&pVM->dbgf.s.PingPong))
        return VERR_SEM_OUT_OF_TURN;

    return VINF_SUCCESS;
}


/**
 * Resumes VM execution.
 *
 * There is no receipt event on this command.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(int) DBGFR3Resume(PUVM pUVM)
{
    /*
     * Check state.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(pVM->dbgf.s.fAttached, VERR_DBGF_NOT_ATTACHED);
    if (RT_LIKELY(RTSemPongIsSpeaker(&pVM->dbgf.s.PingPong)))
    { /* likely */ }
    else
        return VERR_SEM_OUT_OF_TURN;

    /*
     * Send the ping back to the emulation thread telling it to run.
     */
    dbgfR3SetCmd(pVM, DBGFCMD_GO);
    int rc = RTSemPong(&pVM->dbgf.s.PingPong);
    AssertRC(rc);

    return rc;
}


/**
 * Classifies the current instruction.
 *
 * @returns Type of instruction.
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The current CPU.
 * @thread  EMT(pVCpu)
 */
static DBGFSTEPINSTRTYPE dbgfStepGetCurInstrType(PVM pVM, PVMCPU pVCpu)
{
    /*
     * Read the instruction.
     */
    bool     fIsHyper = dbgfR3FigureEventCtx(pVM) == DBGFEVENTCTX_HYPER;
    size_t   cbRead   = 0;
    uint8_t  abOpcode[16] = { 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0 };
    int rc = PGMR3DbgReadGCPtr(pVM, abOpcode, !fIsHyper ? CPUMGetGuestFlatPC(pVCpu) : CPUMGetHyperRIP(pVCpu),
                               sizeof(abOpcode) - 1, 0 /*fFlags*/, &cbRead);
    if (RT_SUCCESS(rc))
    {
        /*
         * Do minimal parsing.  No real need to involve the disassembler here.
         */
        uint8_t *pb = abOpcode;
        for (;;)
        {
            switch (*pb++)
            {
                default:
                    return DBGFSTEPINSTRTYPE_OTHER;

                case 0xe8: /* call rel16/32 */
                case 0x9a: /* call farptr */
                case 0xcc: /* int3 */
                case 0xcd: /* int xx */
                // case 0xce: /* into */
                    return DBGFSTEPINSTRTYPE_CALL;

                case 0xc2: /* ret xx */
                case 0xc3: /* ret */
                case 0xca: /* retf xx */
                case 0xcb: /* retf */
                case 0xcf: /* iret */
                    return DBGFSTEPINSTRTYPE_RET;

                case 0xff:
                    if (   ((*pb >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) == 2  /* call indir */
                        || ((*pb >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) == 3) /* call indir-farptr */
                        return DBGFSTEPINSTRTYPE_CALL;
                    return DBGFSTEPINSTRTYPE_OTHER;

                case 0x0f:
                    switch (*pb++)
                    {
                        case 0x05: /* syscall */
                        case 0x34: /* sysenter */
                            return DBGFSTEPINSTRTYPE_CALL;
                        case 0x07: /* sysret */
                        case 0x35: /* sysexit */
                            return DBGFSTEPINSTRTYPE_RET;
                    }
                    break;

                /* Must handle some REX prefixes. So we do all normal prefixes. */
                case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
                case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
                    if (fIsHyper) /* ASSUMES 32-bit raw-mode! */
                        return DBGFSTEPINSTRTYPE_OTHER;
                    if (!CPUMIsGuestIn64BitCode(pVCpu))
                        return DBGFSTEPINSTRTYPE_OTHER;
                    break;

                case 0x2e: /* CS */
                case 0x36: /* SS */
                case 0x3e: /* DS */
                case 0x26: /* ES */
                case 0x64: /* FS */
                case 0x65: /* GS */
                case 0x66: /* op size */
                case 0x67: /* addr size */
                case 0xf0: /* lock */
                case 0xf2: /* REPNZ */
                case 0xf3: /* REPZ */
                    break;
            }
        }
    }

    return DBGFSTEPINSTRTYPE_INVALID;
}


/**
 * Checks if the stepping has reached a stop point.
 *
 * Called when raising a stepped event.
 *
 * @returns true if the event should be raised, false if we should take one more
 *          step first.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context per CPU structure of the calling EMT.
 * @thread  EMT(pVCpu)
 */
static bool dbgfStepAreWeThereYet(PVM pVM, PVMCPU pVCpu)
{
    /*
     * Check valid pVCpu and that it matches the CPU one stepping.
     */
    if (pVCpu)
    {
        if (pVCpu->idCpu == pVM->dbgf.s.SteppingFilter.idCpu)
        {
            /*
             * Increase the number of steps and see if we've reached the max.
             */
            pVM->dbgf.s.SteppingFilter.cSteps++;
            if (pVM->dbgf.s.SteppingFilter.cSteps < pVM->dbgf.s.SteppingFilter.cMaxSteps)
            {
                /*
                 * Check PC and SP address filtering.
                 */
                if (pVM->dbgf.s.SteppingFilter.fFlags & (DBGF_STEP_F_STOP_ON_ADDRESS | DBGF_STEP_F_STOP_ON_STACK_POP))
                {
                    bool fIsHyper = dbgfR3FigureEventCtx(pVM) == DBGFEVENTCTX_HYPER;
                    if (   (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_ON_ADDRESS)
                        && pVM->dbgf.s.SteppingFilter.AddrPc == (!fIsHyper ? CPUMGetGuestFlatPC(pVCpu) : CPUMGetHyperRIP(pVCpu)))
                        return true;
                    if (   (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_ON_STACK_POP)
                        &&     (!fIsHyper ? CPUMGetGuestFlatSP(pVCpu) : (uint64_t)CPUMGetHyperESP(pVCpu))
                             - pVM->dbgf.s.SteppingFilter.AddrStackPop
                           < pVM->dbgf.s.SteppingFilter.cbStackPop)
                        return true;
                }

                /*
                 * Do step-over filtering separate from the step-into one.
                 */
                if (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_OVER)
                {
                    DBGFSTEPINSTRTYPE enmType = dbgfStepGetCurInstrType(pVM, pVCpu);
                    switch (enmType)
                    {
                        default:
                            if (   pVM->dbgf.s.SteppingFilter.uCallDepth != 0
                                || (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_FILTER_MASK))
                                break;
                            return true;
                        case DBGFSTEPINSTRTYPE_CALL:
                            if (   (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_ON_CALL)
                                && pVM->dbgf.s.SteppingFilter.uCallDepth == 0)
                                return true;
                            pVM->dbgf.s.SteppingFilter.uCallDepth++;
                            break;
                        case DBGFSTEPINSTRTYPE_RET:
                            if (pVM->dbgf.s.SteppingFilter.uCallDepth == 0)
                            {
                                if (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_ON_RET)
                                    return true;
                                /* If after return, we use the cMaxStep limit to stop the next time. */
                                if (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_AFTER_RET)
                                    pVM->dbgf.s.SteppingFilter.cMaxSteps = pVM->dbgf.s.SteppingFilter.cSteps + 1;
                            }
                            else if (pVM->dbgf.s.SteppingFilter.uCallDepth > 0)
                                pVM->dbgf.s.SteppingFilter.uCallDepth--;
                            break;
                    }
                    return false;
                }
                /*
                 * Filtered step-into.
                 */
                else if (  pVM->dbgf.s.SteppingFilter.fFlags
                         & (DBGF_STEP_F_STOP_ON_CALL | DBGF_STEP_F_STOP_ON_RET | DBGF_STEP_F_STOP_AFTER_RET))
                {
                    DBGFSTEPINSTRTYPE enmType = dbgfStepGetCurInstrType(pVM, pVCpu);
                    switch (enmType)
                    {
                        default:
                            break;
                        case DBGFSTEPINSTRTYPE_CALL:
                            if (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_ON_CALL)
                                return true;
                            break;
                        case DBGFSTEPINSTRTYPE_RET:
                            if (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_ON_RET)
                                return true;
                            /* If after return, we use the cMaxStep limit to stop the next time. */
                            if (pVM->dbgf.s.SteppingFilter.fFlags & DBGF_STEP_F_STOP_AFTER_RET)
                                pVM->dbgf.s.SteppingFilter.cMaxSteps = pVM->dbgf.s.SteppingFilter.cSteps + 1;
                            break;
                    }
                    return false;
                }
            }
        }
    }

    return true;
}


/**
 * Step Into.
 *
 * A single step event is generated from this command.
 * The current implementation is not reliable, so don't rely on the event coming.
 *
 * @returns VBox status code.
 * @param   pUVM    The user mode VM handle.
 * @param   idCpu   The ID of the CPU to single step on.
 */
VMMR3DECL(int) DBGFR3Step(PUVM pUVM, VMCPUID idCpu)
{
    return DBGFR3StepEx(pUVM, idCpu, DBGF_STEP_F_INTO, NULL, NULL, 0, 1);
}


/**
 * Full fleged step.
 *
 * This extended stepping API allows for doing multiple steps before raising an
 * event, helping implementing step over, step out and other more advanced
 * features.
 *
 * Like the DBGFR3Step() API, this will normally generate a DBGFEVENT_STEPPED or
 * DBGFEVENT_STEPPED_EVENT.  However the stepping may be interrupted by other
 * events, which will abort the stepping.
 *
 * The stop on pop area feature is for safeguarding step out.
 *
 * Please note though, that it will always use stepping and never breakpoints.
 * While this allows for a much greater flexibility it can at times be rather
 * slow.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   idCpu           The ID of the CPU to single step on.
 * @param   fFlags          Flags controlling the stepping, DBGF_STEP_F_XXX.
 *                          Either DBGF_STEP_F_INTO or DBGF_STEP_F_OVER must
 *                          always be specified.
 * @param   pStopPcAddr     Address to stop executing at.  Completely ignored
 *                          unless DBGF_STEP_F_STOP_ON_ADDRESS is specified.
 * @param   pStopPopAddr    Stack address that SP must be lower than when
 *                          performing DBGF_STEP_F_STOP_ON_STACK_POP filtering.
 * @param   cbStopPop       The range starting at @a pStopPopAddr which is
 *                          considered to be within the same thread stack. Note
 *                          that the API allows @a pStopPopAddr and @a cbStopPop
 *                          to form an area that wraps around and it will
 *                          consider the part starting at 0 as included.
 * @param   cMaxSteps       The maximum number of steps to take.  This is to
 *                          prevent stepping for ever, so passing UINT32_MAX is
 *                          not recommended.
 *
 * @remarks The two address arguments must be guest context virtual addresses,
 *          or HMA.  The code doesn't make much of a point of out HMA, though.
 */
VMMR3DECL(int) DBGFR3StepEx(PUVM pUVM, VMCPUID idCpu, uint32_t fFlags, PCDBGFADDRESS pStopPcAddr,
                            PCDBGFADDRESS pStopPopAddr, RTGCUINTPTR cbStopPop, uint32_t cMaxSteps)
{
    /*
     * Check state.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(idCpu < pVM->cCpus, VERR_INVALID_PARAMETER);
    AssertReturn(!(fFlags & ~DBGF_STEP_F_VALID_MASK), VERR_INVALID_FLAGS);
    AssertReturn(RT_BOOL(fFlags & DBGF_STEP_F_INTO) != RT_BOOL(fFlags & DBGF_STEP_F_OVER), VERR_INVALID_FLAGS);
    if (fFlags & DBGF_STEP_F_STOP_ON_ADDRESS)
    {
        AssertReturn(RT_VALID_PTR(pStopPcAddr), VERR_INVALID_POINTER);
        AssertReturn(DBGFADDRESS_IS_VALID(pStopPcAddr), VERR_INVALID_PARAMETER);
        AssertReturn(DBGFADDRESS_IS_VIRT_GC(pStopPcAddr), VERR_INVALID_PARAMETER);
    }
    AssertReturn(!(fFlags & DBGF_STEP_F_STOP_ON_STACK_POP) || RT_VALID_PTR(pStopPopAddr), VERR_INVALID_POINTER);
    if (fFlags & DBGF_STEP_F_STOP_ON_STACK_POP)
    {
        AssertReturn(RT_VALID_PTR(pStopPopAddr), VERR_INVALID_POINTER);
        AssertReturn(DBGFADDRESS_IS_VALID(pStopPopAddr), VERR_INVALID_PARAMETER);
        AssertReturn(DBGFADDRESS_IS_VIRT_GC(pStopPopAddr), VERR_INVALID_PARAMETER);
        AssertReturn(cbStopPop > 0, VERR_INVALID_PARAMETER);
    }

    AssertReturn(pVM->dbgf.s.fAttached, VERR_DBGF_NOT_ATTACHED);
    if (RT_LIKELY(RTSemPongIsSpeaker(&pVM->dbgf.s.PingPong)))
    { /* likely */ }
    else
        return VERR_SEM_OUT_OF_TURN;
    Assert(pVM->dbgf.s.SteppingFilter.idCpu == NIL_VMCPUID);

    /*
     * Send the ping back to the emulation thread telling it to run.
     */
    if (fFlags == DBGF_STEP_F_INTO)
        pVM->dbgf.s.SteppingFilter.idCpu = NIL_VMCPUID;
    else
        pVM->dbgf.s.SteppingFilter.idCpu = idCpu;
    pVM->dbgf.s.SteppingFilter.fFlags = fFlags;
    if (fFlags & DBGF_STEP_F_STOP_ON_ADDRESS)
        pVM->dbgf.s.SteppingFilter.AddrPc = pStopPcAddr->FlatPtr;
    else
        pVM->dbgf.s.SteppingFilter.AddrPc = 0;
    if (fFlags & DBGF_STEP_F_STOP_ON_STACK_POP)
    {
        pVM->dbgf.s.SteppingFilter.AddrStackPop = pStopPopAddr->FlatPtr;
        pVM->dbgf.s.SteppingFilter.cbStackPop   = cbStopPop;
    }
    else
    {
        pVM->dbgf.s.SteppingFilter.AddrStackPop = 0;
        pVM->dbgf.s.SteppingFilter.cbStackPop   = RTGCPTR_MAX;
    }

    pVM->dbgf.s.SteppingFilter.cMaxSteps    = cMaxSteps;
    pVM->dbgf.s.SteppingFilter.cSteps       = 0;
    pVM->dbgf.s.SteppingFilter.uCallDepth   = 0;

/** @todo SMP (idCpu) */
    dbgfR3SetCmd(pVM, DBGFCMD_SINGLE_STEP);
    int rc = RTSemPong(&pVM->dbgf.s.PingPong);
    AssertRC(rc);
    return rc;
}



/**
 * dbgfR3EventConfigEx argument packet.
 */
typedef struct DBGFR3EVENTCONFIGEXARGS
{
    PCDBGFEVENTCONFIG   paConfigs;
    size_t              cConfigs;
    int                 rc;
} DBGFR3EVENTCONFIGEXARGS;
/** Pointer to a dbgfR3EventConfigEx argument packet. */
typedef DBGFR3EVENTCONFIGEXARGS *PDBGFR3EVENTCONFIGEXARGS;


/**
 * @callback_method_impl{FNVMMEMTRENDEZVOUS, Worker for DBGFR3EventConfigEx.}
 */
static DECLCALLBACK(VBOXSTRICTRC) dbgfR3EventConfigEx(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    if (pVCpu->idCpu == 0)
    {
        PDBGFR3EVENTCONFIGEXARGS        pArgs = (PDBGFR3EVENTCONFIGEXARGS)pvUser;
        DBGFEVENTCONFIG volatile const *paConfigs = pArgs->paConfigs;
        size_t                          cConfigs  = pArgs->cConfigs;

        /*
         * Apply the changes.
         */
        unsigned cChanges = 0;
        for (uint32_t i = 0; i < cConfigs; i++)
        {
            DBGFEVENTTYPE enmType = paConfigs[i].enmType;
            AssertReturn(enmType >= DBGFEVENT_FIRST_SELECTABLE && enmType < DBGFEVENT_END, VERR_INVALID_PARAMETER);
            if (paConfigs[i].fEnabled)
                cChanges += ASMAtomicBitTestAndSet(&pVM->dbgf.s.bmSelectedEvents, enmType) == false;
            else
                cChanges += ASMAtomicBitTestAndClear(&pVM->dbgf.s.bmSelectedEvents, enmType) == true;
        }

        /*
         * Inform HM about changes.
         */
        if (cChanges > 0 && HMIsEnabled(pVM))
        {
            HMR3NotifyDebugEventChanged(pVM);
            HMR3NotifyDebugEventChangedPerCpu(pVM, pVCpu);
        }
    }
    else if (HMIsEnabled(pVM))
        HMR3NotifyDebugEventChangedPerCpu(pVM, pVCpu);

    return VINF_SUCCESS;
}


/**
 * Configures (enables/disables) multiple selectable debug events.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   paConfigs   The event to configure and their new state.
 * @param   cConfigs    Number of entries in @a paConfigs.
 */
VMMR3DECL(int) DBGFR3EventConfigEx(PUVM pUVM, PCDBGFEVENTCONFIG paConfigs, size_t cConfigs)
{
    /*
     * Validate input.
     */
    size_t i = cConfigs;
    while (i-- > 0)
    {
        AssertReturn(paConfigs[i].enmType >= DBGFEVENT_FIRST_SELECTABLE, VERR_INVALID_PARAMETER);
        AssertReturn(paConfigs[i].enmType <  DBGFEVENT_END, VERR_INVALID_PARAMETER);
    }
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    /*
     * Apply the changes in EMT(0) and rendezvous with the other CPUs so they
     * can sync their data and execution with new debug state.
     */
    DBGFR3EVENTCONFIGEXARGS Args = { paConfigs, cConfigs, VINF_SUCCESS };
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING | VMMEMTRENDEZVOUS_FLAGS_PRIORITY,
                                dbgfR3EventConfigEx, &Args);
    if (RT_SUCCESS(rc))
        rc = Args.rc;
    return rc;
}


/**
 * Enables or disables a selectable debug event.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   enmEvent    The selectable debug event.
 * @param   fEnabled    The new state.
 */
VMMR3DECL(int) DBGFR3EventConfig(PUVM pUVM, DBGFEVENTTYPE enmEvent, bool fEnabled)
{
    /*
     * Convert to an array call.
     */
    DBGFEVENTCONFIG EvtCfg = { enmEvent, fEnabled };
    return DBGFR3EventConfigEx(pUVM, &EvtCfg, 1);
}


/**
 * Checks if the given selectable event is enabled.
 *
 * @returns true if enabled, false if not or invalid input.
 * @param   pUVM        The user mode VM handle.
 * @param   enmEvent    The selectable debug event.
 * @sa      DBGFR3EventQuery
 */
VMMR3DECL(bool) DBGFR3EventIsEnabled(PUVM pUVM, DBGFEVENTTYPE enmEvent)
{
    /*
     * Validate input.
     */
    AssertReturn(   enmEvent >= DBGFEVENT_HALT_DONE
                 && enmEvent <  DBGFEVENT_END, false);
    Assert(   enmEvent >= DBGFEVENT_FIRST_SELECTABLE
           || enmEvent == DBGFEVENT_BREAKPOINT
           || enmEvent == DBGFEVENT_BREAKPOINT_IO
           || enmEvent == DBGFEVENT_BREAKPOINT_MMIO);

    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);

    /*
     * Check the event status.
     */
    return ASMBitTest(&pVM->dbgf.s.bmSelectedEvents, enmEvent);
}


/**
 * Queries the status of a set of events.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   paConfigs   The events to query and where to return the state.
 * @param   cConfigs    The number of elements in @a paConfigs.
 * @sa      DBGFR3EventIsEnabled, DBGF_IS_EVENT_ENABLED
 */
VMMR3DECL(int) DBGFR3EventQuery(PUVM pUVM, PDBGFEVENTCONFIG paConfigs, size_t cConfigs)
{
    /*
     * Validate input.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    for (size_t i = 0; i < cConfigs; i++)
    {
        DBGFEVENTTYPE enmType = paConfigs[i].enmType;
        AssertReturn(   enmType >= DBGFEVENT_HALT_DONE
                     && enmType <  DBGFEVENT_END, VERR_INVALID_PARAMETER);
        Assert(   enmType >= DBGFEVENT_FIRST_SELECTABLE
               || enmType == DBGFEVENT_BREAKPOINT
               || enmType == DBGFEVENT_BREAKPOINT_IO
               || enmType == DBGFEVENT_BREAKPOINT_MMIO);
        paConfigs[i].fEnabled = ASMBitTest(&pVM->dbgf.s.bmSelectedEvents, paConfigs[i].enmType);
    }

    return VINF_SUCCESS;
}


/**
 * dbgfR3InterruptConfigEx argument packet.
 */
typedef struct DBGFR3INTERRUPTCONFIGEXARGS
{
    PCDBGFINTERRUPTCONFIG   paConfigs;
    size_t                  cConfigs;
    int                     rc;
} DBGFR3INTERRUPTCONFIGEXARGS;
/** Pointer to a dbgfR3InterruptConfigEx argument packet. */
typedef DBGFR3INTERRUPTCONFIGEXARGS *PDBGFR3INTERRUPTCONFIGEXARGS;

/**
 * @callback_method_impl{FNVMMEMTRENDEZVOUS,
 *      Worker for DBGFR3InterruptConfigEx.}
 */
static DECLCALLBACK(VBOXSTRICTRC) dbgfR3InterruptConfigEx(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    if (pVCpu->idCpu == 0)
    {
        PDBGFR3INTERRUPTCONFIGEXARGS    pArgs = (PDBGFR3INTERRUPTCONFIGEXARGS)pvUser;
        PCDBGFINTERRUPTCONFIG           paConfigs = pArgs->paConfigs;
        size_t                          cConfigs  = pArgs->cConfigs;

        /*
         * Apply the changes.
         */
        bool fChanged = false;
        bool fThis;
        for (uint32_t i = 0; i < cConfigs; i++)
        {
            /*
             * Hardware interrupts.
             */
            if (paConfigs[i].enmHardState == DBGFINTERRUPTSTATE_ENABLED)
            {
                fChanged |= fThis = ASMAtomicBitTestAndSet(&pVM->dbgf.s.bmHardIntBreakpoints, paConfigs[i].iInterrupt) == false;
                if (fThis)
                {
                    Assert(pVM->dbgf.s.cHardIntBreakpoints < 256);
                    pVM->dbgf.s.cHardIntBreakpoints++;
                }
            }
            else if (paConfigs[i].enmHardState == DBGFINTERRUPTSTATE_DISABLED)
            {
                fChanged |= fThis = ASMAtomicBitTestAndClear(&pVM->dbgf.s.bmHardIntBreakpoints, paConfigs[i].iInterrupt) == true;
                if (fThis)
                {
                    Assert(pVM->dbgf.s.cHardIntBreakpoints > 0);
                    pVM->dbgf.s.cHardIntBreakpoints--;
                }
            }

            /*
             * Software interrupts.
             */
            if (paConfigs[i].enmHardState == DBGFINTERRUPTSTATE_ENABLED)
            {
                fChanged |= fThis = ASMAtomicBitTestAndSet(&pVM->dbgf.s.bmSoftIntBreakpoints, paConfigs[i].iInterrupt) == false;
                if (fThis)
                {
                    Assert(pVM->dbgf.s.cSoftIntBreakpoints < 256);
                    pVM->dbgf.s.cSoftIntBreakpoints++;
                }
            }
            else if (paConfigs[i].enmSoftState == DBGFINTERRUPTSTATE_DISABLED)
            {
                fChanged |= fThis = ASMAtomicBitTestAndClear(&pVM->dbgf.s.bmSoftIntBreakpoints, paConfigs[i].iInterrupt) == true;
                if (fThis)
                {
                    Assert(pVM->dbgf.s.cSoftIntBreakpoints > 0);
                    pVM->dbgf.s.cSoftIntBreakpoints--;
                }
            }
        }

        /*
         * Update the event bitmap entries.
         */
        if (pVM->dbgf.s.cHardIntBreakpoints > 0)
            fChanged |= ASMAtomicBitTestAndSet(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_INTERRUPT_HARDWARE) == false;
        else
            fChanged |= ASMAtomicBitTestAndClear(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_INTERRUPT_HARDWARE) == true;

        if (pVM->dbgf.s.cSoftIntBreakpoints > 0)
            fChanged |= ASMAtomicBitTestAndSet(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_INTERRUPT_SOFTWARE) == false;
        else
            fChanged |= ASMAtomicBitTestAndClear(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_INTERRUPT_SOFTWARE) == true;

        /*
         * Inform HM about changes.
         */
        if (fChanged && HMIsEnabled(pVM))
        {
            HMR3NotifyDebugEventChanged(pVM);
            HMR3NotifyDebugEventChangedPerCpu(pVM, pVCpu);
        }
    }
    else if (HMIsEnabled(pVM))
        HMR3NotifyDebugEventChangedPerCpu(pVM, pVCpu);

    return VINF_SUCCESS;
}


/**
 * Changes
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   paConfigs   The events to query and where to return the state.
 * @param   cConfigs    The number of elements in @a paConfigs.
 * @sa      DBGFR3InterruptConfigHardware, DBGFR3InterruptConfigSoftware
 */
VMMR3DECL(int) DBGFR3InterruptConfigEx(PUVM pUVM, PCDBGFINTERRUPTCONFIG paConfigs, size_t cConfigs)
{
    /*
     * Validate input.
     */
    size_t i = cConfigs;
    while (i-- > 0)
    {
        AssertReturn(paConfigs[i].enmHardState <= DBGFINTERRUPTSTATE_DONT_TOUCH, VERR_INVALID_PARAMETER);
        AssertReturn(paConfigs[i].enmSoftState <= DBGFINTERRUPTSTATE_DONT_TOUCH, VERR_INVALID_PARAMETER);
    }

    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    /*
     * Apply the changes in EMT(0) and rendezvous with the other CPUs so they
     * can sync their data and execution with new debug state.
     */
    DBGFR3INTERRUPTCONFIGEXARGS Args = { paConfigs, cConfigs, VINF_SUCCESS };
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING | VMMEMTRENDEZVOUS_FLAGS_PRIORITY,
                                dbgfR3InterruptConfigEx, &Args);
    if (RT_SUCCESS(rc))
        rc = Args.rc;
    return rc;
}


/**
 * Configures interception of a hardware interrupt.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iInterrupt  The interrupt number.
 * @param   fEnabled    Whether interception is enabled or not.
 * @sa      DBGFR3InterruptSoftwareConfig, DBGFR3InterruptConfigEx
 */
VMMR3DECL(int) DBGFR3InterruptHardwareConfig(PUVM pUVM, uint8_t iInterrupt, bool fEnabled)
{
    /*
     * Convert to DBGFR3InterruptConfigEx call.
     */
    DBGFINTERRUPTCONFIG IntCfg = { iInterrupt, (uint8_t)fEnabled, DBGFINTERRUPTSTATE_DONT_TOUCH };
    return DBGFR3InterruptConfigEx(pUVM, &IntCfg, 1);
}


/**
 * Configures interception of a software interrupt.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iInterrupt  The interrupt number.
 * @param   fEnabled    Whether interception is enabled or not.
 * @sa      DBGFR3InterruptHardwareConfig, DBGFR3InterruptConfigEx
 */
VMMR3DECL(int) DBGFR3InterruptSoftwareConfig(PUVM pUVM, uint8_t iInterrupt, bool fEnabled)
{
    /*
     * Convert to DBGFR3InterruptConfigEx call.
     */
    DBGFINTERRUPTCONFIG IntCfg = { iInterrupt, DBGFINTERRUPTSTATE_DONT_TOUCH, (uint8_t)fEnabled };
    return DBGFR3InterruptConfigEx(pUVM, &IntCfg, 1);
}


/**
 * Checks whether interception is enabled for a hardware interrupt.
 *
 * @returns true if enabled, false if not or invalid input.
 * @param   pUVM        The user mode VM handle.
 * @param   iInterrupt  The interrupt number.
 * @sa      DBGFR3InterruptSoftwareIsEnabled, DBGF_IS_HARDWARE_INT_ENABLED,
 *          DBGF_IS_SOFTWARE_INT_ENABLED
 */
VMMR3DECL(int) DBGFR3InterruptHardwareIsEnabled(PUVM pUVM, uint8_t iInterrupt)
{
    /*
     * Validate input.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);

    /*
     * Check it.
     */
    return ASMBitTest(&pVM->dbgf.s.bmHardIntBreakpoints, iInterrupt);
}


/**
 * Checks whether interception is enabled for a software interrupt.
 *
 * @returns true if enabled, false if not or invalid input.
 * @param   pUVM        The user mode VM handle.
 * @param   iInterrupt  The interrupt number.
 * @sa      DBGFR3InterruptHardwareIsEnabled, DBGF_IS_SOFTWARE_INT_ENABLED,
 *          DBGF_IS_HARDWARE_INT_ENABLED,
 */
VMMR3DECL(int) DBGFR3InterruptSoftwareIsEnabled(PUVM pUVM, uint8_t iInterrupt)
{
    /*
     * Validate input.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);

    /*
     * Check it.
     */
    return ASMBitTest(&pVM->dbgf.s.bmSoftIntBreakpoints, iInterrupt);
}



/**
 * Call this to single step programmatically.
 *
 * You must pass down the return code to the EM loop! That's
 * where the actual single stepping take place (at least in the
 * current implementation).
 *
 * @returns VINF_EM_DBG_STEP
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 * @thread  VCpu EMT
 * @internal
 */
VMMR3_INT_DECL(int) DBGFR3PrgStep(PVMCPU pVCpu)
{
    VMCPU_ASSERT_EMT(pVCpu);

    pVCpu->dbgf.s.fSingleSteppingRaw = true;
    return VINF_EM_DBG_STEP;
}


/**
 * Inject an NMI into a running VM (only VCPU 0!)
 *
 * @returns VBox status code.
 * @param   pUVM    The user mode VM structure.
 * @param   idCpu   The ID of the CPU to inject the NMI on.
 */
VMMR3DECL(int) DBGFR3InjectNMI(PUVM pUVM, VMCPUID idCpu)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(idCpu < pVM->cCpus, VERR_INVALID_CPU_ID);

    /** @todo Implement generic NMI injection. */
    if (!HMIsEnabled(pVM))
        return VERR_NOT_SUP_IN_RAW_MODE;

    VMCPU_FF_SET(&pVM->aCpus[idCpu], VMCPU_FF_INTERRUPT_NMI);
    return VINF_SUCCESS;
}


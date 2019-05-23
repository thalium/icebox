/* $Id: VMMRC.cpp $ */
/** @file
 * VMM - Raw-mode Context.
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
#define LOG_GROUP LOG_GROUP_VMM
#include <iprt/asm-amd64-x86.h> /* for SUPGetCpuHzFromGIP */
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/pgm.h>
#include "VMMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/sup.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Default logger instance. */
extern "C" DECLIMPORT(RTLOGGERRC)   g_Logger;
extern "C" DECLIMPORT(RTLOGGERRC)   g_RelLogger;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int vmmGCTest(PVM pVM, unsigned uOperation, unsigned uArg);
static DECLCALLBACK(int) vmmGCTestTmpPFHandler(PVM pVM, PCPUMCTXCORE pRegFrame);
static DECLCALLBACK(int) vmmGCTestTmpPFHandlerCorruptFS(PVM pVM, PCPUMCTXCORE pRegFrame);
DECLASM(bool)   vmmRCSafeMsrRead(uint32_t uMsr, uint64_t *pu64Value);
DECLASM(bool)   vmmRCSafeMsrWrite(uint32_t uMsr, uint64_t u64Value);



/**
 * The GC entry point.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   uOperation  Which operation to execute (VMMRCOPERATION).
 * @param   uArg        Argument to that operation.
 */
VMMRCDECL(int) VMMRCEntry(PVM pVM, unsigned uOperation, unsigned uArg, ...)
{
    /** @todo */
    switch (uOperation)
    {
        /*
         * Init RC modules.
         */
        case VMMRC_DO_VMMRC_INIT:
        {
            /*
             * Validate the svn revision (uArg) and build type (ellipsis).
             */
            if (uArg != VMMGetSvnRev())
                return VERR_VMM_RC_VERSION_MISMATCH;

            va_list va;
            va_start(va, uArg);

            uint32_t uBuildType = va_arg(va, uint32_t);
            if (uBuildType != vmmGetBuildType())
            {
                va_end(va);
                return VERR_VMM_RC_VERSION_MISMATCH;
            }

            /*
             * Initialize the runtime.
             */
            uint64_t u64TS = va_arg(va, uint64_t);

            va_end(va);

            int rc = RTRCInit(u64TS);
            Log(("VMMRCEntry: VMMRC_DO_VMMRC_INIT - uArg=%u (svn revision) u64TS=%RX64; rc=%Rrc\n", uArg, u64TS, rc));
            AssertRCReturn(rc, rc);

            rc = PGMRegisterStringFormatTypes();
            AssertRCReturn(rc, rc);

            rc = PGMRCDynMapInit(pVM);
            AssertRCReturn(rc, rc);
            return VINF_SUCCESS;
        }

        /*
         * Testcase which is used to test interrupt forwarding.
         * It spins for a while with interrupts enabled.
         */
        case VMMRC_DO_TESTCASE_HYPER_INTERRUPT:
        {
            uint32_t volatile i = 0;
            ASMIntEnable();
            while (i < _2G32)
                i++;
            ASMIntDisable();
            return 0;
        }

        /*
         * Testcase which simply returns, this is used for
         * profiling of the switcher.
         */
        case VMMRC_DO_TESTCASE_NOP:
            return 0;

        /*
         * Testcase executes a privileged instruction to force a world switch. (in both SVM & VMX)
         */
        case VMMRC_DO_TESTCASE_HM_NOP:
            ASMRdMsr_Low(MSR_IA32_SYSENTER_CS);
            return 0;

        /*
         * Delay for ~100us.
         */
        case VMMRC_DO_TESTCASE_INTERRUPT_MASKING:
        {
            uint64_t u64MaxTicks = (SUPGetCpuHzFromGip(g_pSUPGlobalInfoPage) != ~(uint64_t)0
                                    ? SUPGetCpuHzFromGip(g_pSUPGlobalInfoPage)
                                    : _2G)
                                   / 10000;
            uint64_t u64StartTSC = ASMReadTSC();
            uint64_t u64TicksNow;
            uint32_t volatile i = 0;

            do
            {
                /* waste some time and protect against getting stuck. */
                for (uint32_t volatile j = 0; j < 1000; j++, i++)
                    if (i > _2G32)
                        return VERR_GENERAL_FAILURE;

                /* check if we're done.*/
                u64TicksNow = ASMReadTSC() - u64StartTSC;
            } while (u64TicksNow < u64MaxTicks);

            return VINF_SUCCESS;
        }

        /*
         * Trap testcases and unknown operations.
         */
        default:
            if (    uOperation >= VMMRC_DO_TESTCASE_TRAP_FIRST
                &&  uOperation < VMMRC_DO_TESTCASE_TRAP_LAST)
                return vmmGCTest(pVM, uOperation, uArg);
            return VERR_INVALID_PARAMETER;
    }
}


/**
 * Internal RC logger worker: Flush logger.
 *
 * @returns VINF_SUCCESS.
 * @param   pLogger     The logger instance to flush.
 * @remark  This function must be exported!
 */
VMMRCDECL(int) vmmGCLoggerFlush(PRTLOGGERRC pLogger)
{
    PVM pVM = &g_VM;
    NOREF(pLogger);
    if (pVM->vmm.s.fRCLoggerFlushingDisabled)
        return VINF_SUCCESS; /* fail quietly. */
    return VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_VMM_LOGGER_FLUSH, 0);
}


/**
 * Flush logger if almost full.
 *
 * @param   pVM             The cross context VM structure.
 */
VMMRCDECL(void) VMMRCLogFlushIfFull(PVM pVM)
{
    if (    pVM->vmm.s.pRCLoggerRC
        &&  pVM->vmm.s.pRCLoggerRC->offScratch >= (sizeof(pVM->vmm.s.pRCLoggerRC->achScratch)*3/4))
    {
        if (pVM->vmm.s.fRCLoggerFlushingDisabled)
            return; /* fail quietly. */
        VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_VMM_LOGGER_FLUSH, 0);
    }
}


/**
 * Switches from guest context to host context.
 *
 * @param   pVM         The cross context VM structure.
 * @param   rc          The status code.
 */
VMMRCDECL(void) VMMRCGuestToHost(PVM pVM, int rc)
{
    pVM->vmm.s.pfnRCToHost(rc);
}


/**
 * Calls the ring-0 host code.
 *
 * @param   pVM             The cross context VM structure.
 */
DECLASM(void) vmmRCProbeFireHelper(PVM pVM)
{
    pVM->vmm.s.pfnRCToHost(VINF_VMM_CALL_TRACER);
}



/**
 * Execute the trap testcase.
 *
 * There is some common code here, that's why we're collecting them
 * like this. Odd numbered variation (uArg) are executed with write
 * protection (WP) enabled.
 *
 * @returns VINF_SUCCESS if it was a testcase setup up to continue and did so successfully.
 * @returns VERR_NOT_IMPLEMENTED if the testcase wasn't implemented.
 * @returns VERR_GENERAL_FAILURE if the testcase continued when it shouldn't.
 *
 * @param   pVM         The cross context VM structure.
 * @param   uOperation  The testcase.
 * @param   uArg        The variation. See function description for odd / even details.
 *
 * @remark  Careful with the trap 08 testcase and WP, it will triple
 *          fault the box if the TSS, the Trap8 TSS and the fault TSS
 *          GDTE are in pages which are read-only.
 *          See bottom of SELMR3Init().
 */
static int vmmGCTest(PVM pVM, unsigned uOperation, unsigned uArg)
{
    /*
     * Set up the testcase.
     */
#if 0
    switch (uOperation)
    {
        default:
            break;
    }
#endif

    /*
     * Enable WP if odd variation.
     */
    if (uArg & 1)
        vmmGCEnableWP();

    /*
     * Execute the testcase.
     */
    int rc = VERR_NOT_IMPLEMENTED;
    switch (uOperation)
    {
        //case VMMRC_DO_TESTCASE_TRAP_0:
        //case VMMRC_DO_TESTCASE_TRAP_1:
        //case VMMRC_DO_TESTCASE_TRAP_2:

        case VMMRC_DO_TESTCASE_TRAP_3:
        {
            if (uArg <= 1)
                rc = vmmGCTestTrap3();
            break;
        }

        //case VMMRC_DO_TESTCASE_TRAP_4:
        //case VMMRC_DO_TESTCASE_TRAP_5:
        //case VMMRC_DO_TESTCASE_TRAP_6:
        //case VMMRC_DO_TESTCASE_TRAP_7:

        case VMMRC_DO_TESTCASE_TRAP_8:
        {
#ifndef DEBUG_bird /** @todo dynamic check that this won't triple fault... */
            if (uArg & 1)
                break;
#endif
            if (uArg <= 1)
                rc = vmmGCTestTrap8();
            break;
        }

        //VMMRC_DO_TESTCASE_TRAP_9,
        //VMMRC_DO_TESTCASE_TRAP_0A,
        //VMMRC_DO_TESTCASE_TRAP_0B,
        //VMMRC_DO_TESTCASE_TRAP_0C,

        case VMMRC_DO_TESTCASE_TRAP_0D:
        {
            if (uArg <= 1)
                rc = vmmGCTestTrap0d();
            break;
        }

        case VMMRC_DO_TESTCASE_TRAP_0E:
        {
            if (uArg <= 1)
                rc = vmmGCTestTrap0e();
            else if (uArg == 2 || uArg == 4)
            {
                /*
                 * Test the use of a temporary #PF handler.
                 */
                rc = TRPMGCSetTempHandler(pVM, X86_XCPT_PF, uArg != 4 ? vmmGCTestTmpPFHandler : vmmGCTestTmpPFHandlerCorruptFS);
                if (RT_SUCCESS(rc))
                {
                    rc = vmmGCTestTrap0e();

                    /* in case it didn't fire. */
                    int rc2 = TRPMGCSetTempHandler(pVM, X86_XCPT_PF, NULL);
                    if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                        rc = rc2;
                }
            }
            break;
        }
    }

    /*
     * Re-enable WP.
     */
    if (uArg & 1)
        vmmGCDisableWP();

    return rc;
}



/**
 * Reads a range of MSRs.
 *
 * This is called directly via VMMR3CallRC.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   uMsr            The MSR to start at.
 * @param   cMsrs           The number of MSRs to read.
 * @param   paResults       Where to store the results.  This must be large
 *                          enough to hold at least @a cMsrs result values.
 */
extern "C" VMMRCDECL(int)
VMMRCTestReadMsrs(PVM pVM, uint32_t uMsr, uint32_t cMsrs, PVMMTESTMSRENTRY paResults)
{
    AssertReturn(cMsrs <= 16384, VERR_INVALID_PARAMETER);
    AssertPtrReturn(paResults, VERR_INVALID_POINTER);
    ASMIntEnable(); /* Run with interrupts enabled, so we can query more MSRs in one block. */
    RT_NOREF_PV(pVM);

    for (uint32_t i = 0; i < cMsrs; i++, uMsr++)
    {
        if (vmmRCSafeMsrRead(uMsr, &paResults[i].uValue))
            paResults[i].uMsr = uMsr;
        else
            paResults[i].uMsr = UINT64_MAX;
    }

    ASMIntDisable();
    return VINF_SUCCESS;
}


/**
 * Tries to write the given value to an MSR, returns the effect and restors the
 * original value.
 *
 * This is called directly via VMMR3CallRC.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   uMsr            The MSR to start at.
 * @param   u32ValueLow     The low part of the value to write.
 * @param   u32ValueHi      The high part of the value to write.
 * @param   puValueBefore   The value before writing.
 * @param   puValueAfter    The value read back after writing.
 */
extern "C" VMMRCDECL(int)
VMMRCTestTestWriteMsr(PVM pVM, uint32_t uMsr, uint32_t u32ValueLow, uint32_t u32ValueHi,
                      uint64_t *puValueBefore, uint64_t *puValueAfter)
{
    AssertPtrReturn(puValueBefore, VERR_INVALID_POINTER);
    AssertPtrReturn(puValueAfter, VERR_INVALID_POINTER);
    ASMIntDisable();
    RT_NOREF_PV(pVM);

    int      rc           = VINF_SUCCESS;
    uint64_t uValueBefore = UINT64_MAX;
    uint64_t uValueAfter  = UINT64_MAX;
    if (vmmRCSafeMsrRead(uMsr, &uValueBefore))
    {
        if (!vmmRCSafeMsrWrite(uMsr, RT_MAKE_U64(u32ValueLow, u32ValueHi)))
            rc = VERR_WRITE_PROTECT;
        if (!vmmRCSafeMsrRead(uMsr, &uValueAfter) && RT_SUCCESS(rc))
            rc = VERR_READ_ERROR;
        vmmRCSafeMsrWrite(uMsr, uValueBefore);
    }
    else
        rc = VERR_ACCESS_DENIED;

    *puValueBefore = uValueBefore;
    *puValueAfter  = uValueAfter;
    return rc;
}



/**
 * Temporary \#PF trap handler for the \#PF test case.
 *
 * @returns VBox status code (appropriate for GC return).
 *          In this context RT_SUCCESS means to restart the instruction.
 * @param   pVM         The cross context VM structure.
 * @param   pRegFrame   Trap register frame.
 */
static DECLCALLBACK(int) vmmGCTestTmpPFHandler(PVM pVM, PCPUMCTXCORE pRegFrame)
{
    if (pRegFrame->eip == (uintptr_t)vmmGCTestTrap0e_FaultEIP)
    {
        pRegFrame->eip = (uintptr_t)vmmGCTestTrap0e_ResumeEIP;
        return VINF_SUCCESS;
    }
    NOREF(pVM);
    return VERR_INTERNAL_ERROR;
}


/**
 * Temporary \#PF trap handler for the \#PF test case, this one messes up the fs
 * selector.
 *
 * @returns VBox status code (appropriate for GC return).
 *          In this context RT_SUCCESS means to restart the instruction.
 * @param   pVM         The cross context VM structure.
 * @param   pRegFrame   Trap register frame.
 */
static DECLCALLBACK(int) vmmGCTestTmpPFHandlerCorruptFS(PVM pVM, PCPUMCTXCORE pRegFrame)
{
    int rc = vmmGCTestTmpPFHandler(pVM, pRegFrame);
    pRegFrame->fs.Sel = 0x30;
    return rc;
}


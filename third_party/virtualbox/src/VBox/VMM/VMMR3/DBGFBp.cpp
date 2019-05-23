/* $Id: DBGFBp.cpp $ */
/** @file
 * DBGF - Debugger Facility, Breakpoint Management.
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
#define LOG_GROUP LOG_GROUP_DBGF
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/selm.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#else
# include <VBox/vmm/iem.h>
#endif
#include <VBox/vmm/mm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/hm.h>
#include "DBGFInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>

#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * DBGF INT3-breakpoint set callback arguments.
 */
typedef struct DBGFBPINT3ARGS
{
    /** The source virtual CPU ID (used for breakpoint address resolution). */
    VMCPUID         idSrcCpu;
    /** The breakpoint address. */
    PCDBGFADDRESS   pAddress;
    /** The hit count at which the breakpoint starts triggering. */
    uint64_t        iHitTrigger;
    /** The hit count at which disables the breakpoint. */
    uint64_t        iHitDisable;
    /** Where to store the breakpoint Id (optional). */
    uint32_t       *piBp;
} DBGFBPINT3ARGS;
/** Pointer to a DBGF INT3 breakpoint set callback argument. */
typedef DBGFBPINT3ARGS *PDBGFBPINT3ARGS;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
static int dbgfR3BpRegArm(PVM pVM, PDBGFBP pBp);
static int dbgfR3BpInt3Arm(PVM pVM, PDBGFBP pBp);
RT_C_DECLS_END



/**
 * Initialize the breakpoint stuff.
 *
 * @returns VINF_SUCCESS
 * @param   pVM     The cross context VM structure.
 */
int dbgfR3BpInit(PVM pVM)
{
    /*
     * Init structures.
     */
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints); i++)
    {
        pVM->dbgf.s.aHwBreakpoints[i].iBp = i;
        pVM->dbgf.s.aHwBreakpoints[i].enmType = DBGFBPTYPE_FREE;
        pVM->dbgf.s.aHwBreakpoints[i].u.Reg.iReg = i;
    }

    for (i = 0; i < RT_ELEMENTS(pVM->dbgf.s.aBreakpoints); i++)
    {
        pVM->dbgf.s.aBreakpoints[i].iBp = i + RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints);
        pVM->dbgf.s.aBreakpoints[i].enmType = DBGFBPTYPE_FREE;
    }

    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];
        pVCpu->dbgf.s.iActiveBp = ~0U;
    }

    /*
     * Register saved state.
     */
    /** @todo */

    return VINF_SUCCESS;
}



/**
 * Allocate a breakpoint.
 *
 * @returns Pointer to the allocated breakpoint.
 * @returns NULL if we're out of breakpoints.
 * @param   pVM     The cross context VM structure.
 * @param   enmType The type to allocate.
 */
static PDBGFBP dbgfR3BpAlloc(PVM pVM, DBGFBPTYPE enmType)
{
    /*
     * Determine which array to search and where in the array to start
     * searching (latter for grouping similar BPs, reducing runtime overhead).
     */
    unsigned    iStart;
    unsigned    cBps;
    PDBGFBP     paBps;
    switch (enmType)
    {
        case DBGFBPTYPE_REG:
            cBps = RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints);
            paBps = &pVM->dbgf.s.aHwBreakpoints[0];
            iStart = 0;
            break;

        case DBGFBPTYPE_INT3:
        case DBGFBPTYPE_REM:
        case DBGFBPTYPE_PORT_IO:
        case DBGFBPTYPE_MMIO:
            cBps = RT_ELEMENTS(pVM->dbgf.s.aBreakpoints);
            paBps = &pVM->dbgf.s.aBreakpoints[0];
            if (enmType == DBGFBPTYPE_PORT_IO)
                iStart = cBps / 4 * 2;
            else if (enmType == DBGFBPTYPE_MMIO)
                iStart = cBps / 4 * 1;
            else if (enmType == DBGFBPTYPE_REM)
                iStart = cBps / 4 * 3;
            else
                iStart = 0;
            break;

        default:
            AssertMsgFailed(("enmType=%d\n", enmType));
            return NULL;
    }

    /*
     * Search for a free breakpoint entry.
     */
    unsigned iBp;
    for (iBp = iStart; iBp < cBps; iBp++)
        if (paBps[iBp].enmType == DBGFBPTYPE_FREE)
            break;
    if (iBp >= cBps && iStart != 0)
        for (iBp = 0; iBp < cBps; iBp++)
            if (paBps[iBp].enmType == DBGFBPTYPE_FREE)
                break;
    if (iBp < cBps)
    {
        /*
         * Return what we found.
         */
        paBps[iBp].fEnabled = false;
        paBps[iBp].cHits    = 0;
        paBps[iBp].enmType  = enmType;
        return &paBps[iBp];
    }

    LogFlow(("dbgfR3BpAlloc: returns NULL - we're out of breakpoint slots! cBps=%u\n", cBps));
    return NULL;
}


/**
 * Updates the search optimization structure for enabled breakpoints of the
 * specified type.
 *
 * @returns VINF_SUCCESS.
 * @param   pVM         The cross context VM structure.
 * @param   enmType     The breakpoint type.
 * @param   pOpt        The breakpoint optimization structure to update.
 */
static int dbgfR3BpUpdateSearchOptimizations(PVM pVM, DBGFBPTYPE enmType, PDBGFBPSEARCHOPT pOpt)
{
    DBGFBPSEARCHOPT Opt  = { UINT32_MAX, 0 };

    for (uint32_t iBp = 0; iBp < RT_ELEMENTS(pVM->dbgf.s.aBreakpoints); iBp++)
        if (   pVM->dbgf.s.aBreakpoints[iBp].enmType == enmType
            && pVM->dbgf.s.aBreakpoints[iBp].fEnabled)
        {
            if (Opt.iStartSearch > iBp)
                Opt.iStartSearch = iBp;
            Opt.cToSearch = iBp - Opt.iStartSearch + 1;
        }

    *pOpt = Opt;
    return VINF_SUCCESS;
}


/**
 * Get a breakpoint give by breakpoint id.
 *
 * @returns Pointer to the allocated breakpoint.
 * @returns NULL if the breakpoint is invalid.
 * @param   pVM     The cross context VM structure.
 * @param   iBp     The breakpoint id.
 */
static PDBGFBP dbgfR3BpGet(PVM pVM, uint32_t iBp)
{
    /* Find it. */
    PDBGFBP pBp;
    if (iBp < RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints))
        pBp = &pVM->dbgf.s.aHwBreakpoints[iBp];
    else
    {
        iBp -= RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints);
        if (iBp >= RT_ELEMENTS(pVM->dbgf.s.aBreakpoints))
            return NULL;
        pBp = &pVM->dbgf.s.aBreakpoints[iBp];
    }

    /* check if it's valid. */
    switch (pBp->enmType)
    {
        case DBGFBPTYPE_FREE:
            return NULL;

        case DBGFBPTYPE_REG:
        case DBGFBPTYPE_INT3:
        case DBGFBPTYPE_REM:
        case DBGFBPTYPE_PORT_IO:
        case DBGFBPTYPE_MMIO:
            break;

        default:
            AssertMsgFailed(("Invalid enmType=%d!\n", pBp->enmType));
            return NULL;
    }

    return pBp;
}


/**
 * Get a breakpoint give by address.
 *
 * @returns Pointer to the allocated breakpoint.
 * @returns NULL if the breakpoint is invalid.
 * @param   pVM     The cross context VM structure.
 * @param   enmType The breakpoint type.
 * @param   GCPtr   The breakpoint address.
 */
static PDBGFBP dbgfR3BpGetByAddr(PVM pVM, DBGFBPTYPE enmType, RTGCUINTPTR GCPtr)
{
    /*
     * Determine which array to search.
     */
    unsigned cBps;
    PDBGFBP  paBps;
    switch (enmType)
    {
        case DBGFBPTYPE_REG:
            cBps = RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints);
            paBps = &pVM->dbgf.s.aHwBreakpoints[0];
            break;

        case DBGFBPTYPE_INT3:
        case DBGFBPTYPE_REM:
            cBps = RT_ELEMENTS(pVM->dbgf.s.aBreakpoints);
            paBps = &pVM->dbgf.s.aBreakpoints[0];
            break;

        default:
            AssertMsgFailed(("enmType=%d\n", enmType));
            return NULL;
    }

    /*
     * Search.
     */
    for (unsigned iBp = 0; iBp < cBps; iBp++)
        if (   paBps[iBp].enmType == enmType
            && paBps[iBp].u.GCPtr == GCPtr)
            return &paBps[iBp];

    return NULL;
}


/**
 * Frees a breakpoint.
 *
 * @param   pVM     The cross context VM structure.
 * @param   pBp     The breakpoint to free.
 */
static void dbgfR3BpFree(PVM pVM, PDBGFBP pBp)
{
    switch (pBp->enmType)
    {
        case DBGFBPTYPE_FREE:
            AssertMsgFailed(("Already freed!\n"));
            return;

        case DBGFBPTYPE_REG:
            Assert((uintptr_t)(pBp - &pVM->dbgf.s.aHwBreakpoints[0]) < RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints));
            break;

        case DBGFBPTYPE_INT3:
        case DBGFBPTYPE_REM:
        case DBGFBPTYPE_PORT_IO:
        case DBGFBPTYPE_MMIO:
            Assert((uintptr_t)(pBp - &pVM->dbgf.s.aBreakpoints[0]) < RT_ELEMENTS(pVM->dbgf.s.aBreakpoints));
            break;

        default:
            AssertMsgFailed(("Invalid enmType=%d!\n", pBp->enmType));
            return;

    }
    pBp->enmType = DBGFBPTYPE_FREE;
    NOREF(pVM);
}


/**
 * @callback_method_impl{FNVMMEMTRENDEZVOUS}
 */
static DECLCALLBACK(VBOXSTRICTRC) dbgfR3BpEnableInt3OnCpu(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    /*
     * Validate input.
     */
    PDBGFBP pBp = (PDBGFBP)pvUser;
    AssertReturn(pBp, VERR_INVALID_PARAMETER);
    Assert(pBp->enmType == DBGFBPTYPE_INT3);
    VMCPU_ASSERT_EMT(pVCpu); RT_NOREF(pVCpu);
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    /*
     * Arm the breakpoint.
     */
    return dbgfR3BpInt3Arm(pVM, pBp);
}


/**
 * @callback_method_impl{FNVMMEMTRENDEZVOUS}
 */
static DECLCALLBACK(VBOXSTRICTRC) dbgfR3BpSetInt3OnCpu(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    /*
     * Validate input.
     */
    PDBGFBPINT3ARGS pBpArgs = (PDBGFBPINT3ARGS)pvUser;
    AssertReturn(pBpArgs, VERR_INVALID_PARAMETER);
    VMCPU_ASSERT_EMT(pVCpu);
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    AssertMsgReturn(!pBpArgs->piBp || VALID_PTR(pBpArgs->piBp), ("piBp=%p\n", pBpArgs->piBp), VERR_INVALID_POINTER);
    PCDBGFADDRESS pAddress = pBpArgs->pAddress;
    if (!DBGFR3AddrIsValid(pVM->pUVM, pAddress))
        return VERR_INVALID_PARAMETER;

    if (pBpArgs->iHitTrigger > pBpArgs->iHitDisable)
        return VERR_INVALID_PARAMETER;

    /*
     * Check if we're on the source CPU where we can resolve the breakpoint address.
     */
    if (pVCpu->idCpu == pBpArgs->idSrcCpu)
    {
        if (pBpArgs->piBp)
            *pBpArgs->piBp = UINT32_MAX;

        /*
         * Check if the breakpoint already exists.
         */
        PDBGFBP pBp = dbgfR3BpGetByAddr(pVM, DBGFBPTYPE_INT3, pAddress->FlatPtr);
        if (pBp)
        {
            int rc = VINF_SUCCESS;
            if (!pBp->fEnabled)
                rc = dbgfR3BpInt3Arm(pVM, pBp);
            if (RT_SUCCESS(rc))
            {
                if (pBpArgs->piBp)
                    *pBpArgs->piBp = pBp->iBp;

                /*
                 * Returning VINF_DBGF_BP_ALREADY_EXIST here causes a VBOXSTRICTRC out-of-range assertion
                 * in VMMR3EmtRendezvous(). Re-setting of an existing breakpoint shouldn't cause an assertion
                 * killing the VM (and debugging session), so for now we'll pretend success.
                 */
#if 0
                rc = VINF_DBGF_BP_ALREADY_EXIST;
#endif
            }
            else
                dbgfR3BpFree(pVM, pBp);
            return rc;
        }

        /*
         * Allocate the breakpoint.
         */
        pBp = dbgfR3BpAlloc(pVM, DBGFBPTYPE_INT3);
        if (!pBp)
            return VERR_DBGF_NO_MORE_BP_SLOTS;

        /*
         * Translate & save the breakpoint address into a guest-physical address.
         */
        int rc = DBGFR3AddrToPhys(pVM->pUVM, pBpArgs->idSrcCpu, pAddress, &pBp->u.Int3.PhysAddr);
        if (RT_SUCCESS(rc))
        {
            /* The physical address from DBGFR3AddrToPhys() is the start of the page,
               we need the exact byte offset into the page while writing to it in dbgfR3BpInt3Arm(). */
            pBp->u.Int3.PhysAddr |= (pAddress->FlatPtr & X86_PAGE_OFFSET_MASK);
            pBp->u.Int3.GCPtr = pAddress->FlatPtr;
            pBp->iHitTrigger  = pBpArgs->iHitTrigger;
            pBp->iHitDisable  = pBpArgs->iHitDisable;

            /*
             * Now set the breakpoint in guest memory.
             */
            rc = dbgfR3BpInt3Arm(pVM, pBp);
            if (RT_SUCCESS(rc))
            {
                if (pBpArgs->piBp)
                    *pBpArgs->piBp = pBp->iBp;
                return VINF_SUCCESS;
            }
        }

        dbgfR3BpFree(pVM, pBp);
        return rc;
    }

    return VINF_SUCCESS;
}


/**
 * Sets a breakpoint (int 3 based).
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   idSrcCpu    The ID of the virtual CPU used for the
 *                      breakpoint address resolution.
 * @param   pAddress    The address of the breakpoint.
 * @param   iHitTrigger The hit count at which the breakpoint start triggering.
 *                      Use 0 (or 1) if it's gonna trigger at once.
 * @param   iHitDisable The hit count which disables the breakpoint.
 *                      Use ~(uint64_t) if it's never gonna be disabled.
 * @param   piBp        Where to store the breakpoint id. (optional)
 * @thread  Any thread.
 */
VMMR3DECL(int) DBGFR3BpSetInt3(PUVM pUVM, VMCPUID idSrcCpu, PCDBGFADDRESS pAddress, uint64_t iHitTrigger, uint64_t iHitDisable,
                               uint32_t *piBp)
{
    AssertReturn(idSrcCpu <= pUVM->cCpus, VERR_INVALID_CPU_ID);

    DBGFBPINT3ARGS BpArgs;
    RT_ZERO(BpArgs);
    BpArgs.idSrcCpu    = idSrcCpu;
    BpArgs.iHitTrigger = iHitTrigger;
    BpArgs.iHitDisable = iHitDisable;
    BpArgs.pAddress    = pAddress;
    BpArgs.piBp        = piBp;

    int rc = VMMR3EmtRendezvous(pUVM->pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE, dbgfR3BpSetInt3OnCpu, &BpArgs);
    LogFlow(("DBGFR3BpSet: returns %Rrc\n", rc));
    return rc;
}


/**
 * Arms an int 3 breakpoint.
 *
 * This is used to implement both DBGFR3BpSetInt3() and
 * DBGFR3BpEnable().
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pBp         The breakpoint.
 */
static int dbgfR3BpInt3Arm(PVM pVM, PDBGFBP pBp)
{
    VM_ASSERT_EMT(pVM);

    /*
     * Save current byte and write the int3 instruction byte.
     */
    int rc = PGMPhysSimpleReadGCPhys(pVM, &pBp->u.Int3.bOrg, pBp->u.Int3.PhysAddr, sizeof(pBp->u.Int3.bOrg));
    if (RT_SUCCESS(rc))
    {
        static const uint8_t s_bInt3 = 0xcc;
        rc = PGMPhysSimpleWriteGCPhys(pVM, pBp->u.Int3.PhysAddr, &s_bInt3, sizeof(s_bInt3));
        if (RT_SUCCESS(rc))
        {
            pBp->fEnabled = true;
            dbgfR3BpUpdateSearchOptimizations(pVM, DBGFBPTYPE_INT3, &pVM->dbgf.s.Int3);
            pVM->dbgf.s.cEnabledInt3Breakpoints = pVM->dbgf.s.Int3.cToSearch;
            Log(("DBGF: Set breakpoint at %RGv (Phys %RGp) cEnabledInt3Breakpoints=%u\n", pBp->u.Int3.GCPtr,
                 pBp->u.Int3.PhysAddr, pVM->dbgf.s.cEnabledInt3Breakpoints));
        }
    }
    return rc;
}


/**
 * Disarms an int 3 breakpoint.
 *
 * This is used to implement both DBGFR3BpClear() and DBGFR3BpDisable().
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pBp         The breakpoint.
 */
static int dbgfR3BpInt3Disarm(PVM pVM, PDBGFBP pBp)
{
    VM_ASSERT_EMT(pVM);

    /*
     * Check that the current byte is the int3 instruction, and restore the original one.
     * We currently ignore invalid bytes.
     */
    uint8_t bCurrent = 0;
    int rc = PGMPhysSimpleReadGCPhys(pVM, &bCurrent, pBp->u.Int3.PhysAddr, sizeof(bCurrent));
    if (   RT_SUCCESS(rc)
        && bCurrent == 0xcc)
    {
        rc = PGMPhysSimpleWriteGCPhys(pVM, pBp->u.Int3.PhysAddr, &pBp->u.Int3.bOrg, sizeof(pBp->u.Int3.bOrg));
        if (RT_SUCCESS(rc))
        {
            pBp->fEnabled = false;
            dbgfR3BpUpdateSearchOptimizations(pVM, DBGFBPTYPE_INT3, &pVM->dbgf.s.Int3);
            pVM->dbgf.s.cEnabledInt3Breakpoints = pVM->dbgf.s.Int3.cToSearch;
            Log(("DBGF: Removed breakpoint at %RGv (Phys %RGp) cEnabledInt3Breakpoints=%u\n", pBp->u.Int3.GCPtr,
                 pBp->u.Int3.PhysAddr, pVM->dbgf.s.cEnabledInt3Breakpoints));
        }
    }
    return rc;
}


/**
 * @callback_method_impl{FNVMMEMTRENDEZVOUS}
 */
static DECLCALLBACK(VBOXSTRICTRC) dbgfR3BpDisableInt3OnCpu(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    /*
     * Validate input.
     */
    PDBGFBP pBp = (PDBGFBP)pvUser;
    AssertReturn(pBp, VERR_INVALID_PARAMETER);
    Assert(pBp->enmType == DBGFBPTYPE_INT3);
    VMCPU_ASSERT_EMT(pVCpu); RT_NOREF(pVCpu);
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    /*
     * Disarm the breakpoint.
     */
    return dbgfR3BpInt3Disarm(pVM, pBp);
}


/**
 * Sets a register breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pAddress        The address of the breakpoint.
 * @param   piHitTrigger    The hit count at which the breakpoint start triggering.
 *                          Use 0 (or 1) if it's gonna trigger at once.
 * @param   piHitDisable    The hit count which disables the breakpoint.
 *                          Use ~(uint64_t) if it's never gonna be disabled.
 * @param   fType           The access type (one of the X86_DR7_RW_* defines).
 * @param   cb              The access size - 1,2,4 or 8 (the latter is AMD64 long mode only.
 *                          Must be 1 if fType is X86_DR7_RW_EO.
 * @param   piBp            Where to store the breakpoint id. (optional)
 * @thread  EMT
 * @internal
 */
static DECLCALLBACK(int) dbgfR3BpSetReg(PUVM pUVM, PCDBGFADDRESS pAddress, uint64_t *piHitTrigger, uint64_t *piHitDisable,
                                        uint8_t fType, uint8_t cb, uint32_t *piBp)
{
    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    if (!DBGFR3AddrIsValid(pUVM, pAddress))
        return VERR_INVALID_PARAMETER;
    if (*piHitTrigger > *piHitDisable)
        return VERR_INVALID_PARAMETER;
    AssertMsgReturn(!piBp || VALID_PTR(piBp), ("piBp=%p\n", piBp), VERR_INVALID_POINTER);
    if (piBp)
        *piBp = UINT32_MAX;
    switch (fType)
    {
        case X86_DR7_RW_EO:
            if (cb == 1)
                break;
            AssertMsgFailed(("fType=%#x cb=%d != 1\n", fType, cb));
            return VERR_INVALID_PARAMETER;
        case X86_DR7_RW_IO:
        case X86_DR7_RW_RW:
        case X86_DR7_RW_WO:
            break;
        default:
            AssertMsgFailed(("fType=%#x\n", fType));
            return VERR_INVALID_PARAMETER;
    }
    switch (cb)
    {
        case 1:
        case 2:
        case 4:
            break;
        default:
            AssertMsgFailed(("cb=%#x\n", cb));
            return VERR_INVALID_PARAMETER;
    }

    /*
     * Check if the breakpoint already exists.
     */
    PDBGFBP pBp = dbgfR3BpGetByAddr(pVM, DBGFBPTYPE_REG, pAddress->FlatPtr);
    if (    pBp
        &&  pBp->u.Reg.cb == cb
        &&  pBp->u.Reg.fType == fType)
    {
        int rc = VINF_SUCCESS;
        if (!pBp->fEnabled)
            rc = dbgfR3BpRegArm(pVM, pBp);
        if (RT_SUCCESS(rc))
        {
            rc = VINF_DBGF_BP_ALREADY_EXIST;
            if (piBp)
                *piBp = pBp->iBp;
        }
        return rc;
    }

    /*
     * Allocate and initialize the bp.
     */
    pBp = dbgfR3BpAlloc(pVM, DBGFBPTYPE_REG);
    if (!pBp)
        return VERR_DBGF_NO_MORE_BP_SLOTS;
    pBp->iHitTrigger = *piHitTrigger;
    pBp->iHitDisable = *piHitDisable;
    Assert(pBp->iBp == pBp->u.Reg.iReg);
    pBp->u.Reg.GCPtr = pAddress->FlatPtr;
    pBp->u.Reg.fType = fType;
    pBp->u.Reg.cb    = cb;
    ASMCompilerBarrier();
    pBp->fEnabled    = true;

    /*
     * Arm the breakpoint.
     */
    int rc = dbgfR3BpRegArm(pVM, pBp);
    if (RT_SUCCESS(rc))
    {
        if (piBp)
            *piBp = pBp->iBp;
    }
    else
        dbgfR3BpFree(pVM, pBp);

    return rc;
}


/**
 * Sets a register breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pAddress    The address of the breakpoint.
 * @param   iHitTrigger The hit count at which the breakpoint start triggering.
 *                      Use 0 (or 1) if it's gonna trigger at once.
 * @param   iHitDisable The hit count which disables the breakpoint.
 *                      Use ~(uint64_t) if it's never gonna be disabled.
 * @param   fType       The access type (one of the X86_DR7_RW_* defines).
 * @param   cb          The access size - 1,2,4 or 8 (the latter is AMD64 long mode only.
 *                      Must be 1 if fType is X86_DR7_RW_EO.
 * @param   piBp        Where to store the breakpoint id. (optional)
 * @thread  Any thread.
 */
VMMR3DECL(int) DBGFR3BpSetReg(PUVM pUVM, PCDBGFADDRESS pAddress, uint64_t iHitTrigger, uint64_t iHitDisable,
                              uint8_t fType, uint8_t cb, uint32_t *piBp)
{
    /*
     * This must be done on EMT.
     */
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpSetReg, 7,
                              pUVM, pAddress, &iHitTrigger, &iHitDisable, fType, cb, piBp);
    LogFlow(("DBGFR3BpSetReg: returns %Rrc\n", rc));
    return rc;

}


/**
 * @callback_method_impl{FNVMMEMTRENDEZVOUS}
 */
static DECLCALLBACK(VBOXSTRICTRC) dbgfR3BpRegRecalcOnCpu(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    NOREF(pVM); NOREF(pvUser);

    /*
     * CPU 0 updates the enabled hardware breakpoint counts.
     */
    if (pVCpu->idCpu == 0)
    {
        pVM->dbgf.s.cEnabledHwBreakpoints   = 0;
        pVM->dbgf.s.cEnabledHwIoBreakpoints = 0;

        for (uint32_t iBp = 0; iBp < RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints); iBp++)
            if (   pVM->dbgf.s.aHwBreakpoints[iBp].fEnabled
                && pVM->dbgf.s.aHwBreakpoints[iBp].enmType == DBGFBPTYPE_REG)
            {
                pVM->dbgf.s.cEnabledHwBreakpoints   += 1;
                pVM->dbgf.s.cEnabledHwIoBreakpoints += pVM->dbgf.s.aHwBreakpoints[iBp].u.Reg.fType == X86_DR7_RW_IO;
            }
    }

    return CPUMRecalcHyperDRx(pVCpu, UINT8_MAX, false);
}


/**
 * Arms a debug register breakpoint.
 *
 * This is used to implement both DBGFR3BpSetReg() and DBGFR3BpEnable().
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pBp         The breakpoint.
 * @thread  EMT(0)
 */
static int dbgfR3BpRegArm(PVM pVM, PDBGFBP pBp)
{
    RT_NOREF_PV(pBp);
    Assert(pBp->fEnabled);
    return VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE, dbgfR3BpRegRecalcOnCpu, NULL);
}


/**
 * Disarms a debug register breakpoint.
 *
 * This is used to implement both DBGFR3BpClear() and DBGFR3BpDisable().
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pBp         The breakpoint.
 * @thread  EMT(0)
 */
static int dbgfR3BpRegDisarm(PVM pVM, PDBGFBP pBp)
{
    RT_NOREF_PV(pBp);
    Assert(!pBp->fEnabled);
    return VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE, dbgfR3BpRegRecalcOnCpu, NULL);
}


/**
 * EMT worker for DBGFR3BpSetREM().
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pAddress        The address of the breakpoint.
 * @param   piHitTrigger    The hit count at which the breakpoint start triggering.
 *                          Use 0 (or 1) if it's gonna trigger at once.
 * @param   piHitDisable    The hit count which disables the breakpoint.
 *                          Use ~(uint64_t) if it's never gonna be disabled.
 * @param   piBp            Where to store the breakpoint id. (optional)
 * @thread  EMT(0)
 * @internal
 */
static DECLCALLBACK(int) dbgfR3BpSetREM(PUVM pUVM, PCDBGFADDRESS pAddress, uint64_t *piHitTrigger,
                                        uint64_t *piHitDisable, uint32_t *piBp)
{
    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    if (!DBGFR3AddrIsValid(pUVM, pAddress))
        return VERR_INVALID_PARAMETER;
    if (*piHitTrigger > *piHitDisable)
        return VERR_INVALID_PARAMETER;
    AssertMsgReturn(!piBp || VALID_PTR(piBp), ("piBp=%p\n", piBp), VERR_INVALID_POINTER);
    if (piBp)
        *piBp = UINT32_MAX;

    /*
     * Check if the breakpoint already exists.
     */
    PDBGFBP pBp = dbgfR3BpGetByAddr(pVM, DBGFBPTYPE_REM, pAddress->FlatPtr);
    if (pBp)
    {
        int rc = VINF_SUCCESS;
        if (!pBp->fEnabled)
#ifdef VBOX_WITH_REM
            rc = REMR3BreakpointSet(pVM, pBp->u.Rem.GCPtr);
#else
            rc = IEMBreakpointSet(pVM, pBp->u.Rem.GCPtr);
#endif
        if (RT_SUCCESS(rc))
        {
            rc = VINF_DBGF_BP_ALREADY_EXIST;
            if (piBp)
                *piBp = pBp->iBp;
        }
        return rc;
    }

    /*
     * Allocate and initialize the bp.
     */
    pBp = dbgfR3BpAlloc(pVM, DBGFBPTYPE_REM);
    if (!pBp)
        return VERR_DBGF_NO_MORE_BP_SLOTS;
    pBp->u.Rem.GCPtr = pAddress->FlatPtr;
    pBp->iHitTrigger = *piHitTrigger;
    pBp->iHitDisable = *piHitDisable;
    ASMCompilerBarrier();
    pBp->fEnabled    = true;

    /*
     * Now ask REM to set the breakpoint.
     */
#ifdef VBOX_WITH_REM
    int rc = REMR3BreakpointSet(pVM, pAddress->FlatPtr);
#else
    int rc = IEMBreakpointSet(pVM, pAddress->FlatPtr);
#endif
    if (RT_SUCCESS(rc))
    {
        if (piBp)
            *piBp = pBp->iBp;
    }
    else
        dbgfR3BpFree(pVM, pBp);

    return rc;
}


/**
 * Sets a recompiler breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pAddress    The address of the breakpoint.
 * @param   iHitTrigger The hit count at which the breakpoint start triggering.
 *                      Use 0 (or 1) if it's gonna trigger at once.
 * @param   iHitDisable The hit count which disables the breakpoint.
 *                      Use ~(uint64_t) if it's never gonna be disabled.
 * @param   piBp        Where to store the breakpoint id. (optional)
 * @thread  Any thread.
 */
VMMR3DECL(int) DBGFR3BpSetREM(PUVM pUVM, PCDBGFADDRESS pAddress, uint64_t iHitTrigger, uint64_t iHitDisable, uint32_t *piBp)
{
    /*
     * This must be done on EMT.
     */
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpSetREM, 5,
                              pUVM, pAddress, &iHitTrigger, &iHitDisable, piBp);
    LogFlow(("DBGFR3BpSetREM: returns %Rrc\n", rc));
    return rc;
}


/**
 * Updates IOM on whether we've got any armed I/O port or MMIO breakpoints.
 *
 * @returns VINF_SUCCESS
 * @param   pVM         The cross context VM structure.
 * @thread  EMT(0)
 */
static int dbgfR3BpUpdateIom(PVM pVM)
{
    dbgfR3BpUpdateSearchOptimizations(pVM, DBGFBPTYPE_PORT_IO, &pVM->dbgf.s.PortIo);
    if (pVM->dbgf.s.PortIo.cToSearch)
        ASMAtomicBitSet(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_BREAKPOINT_IO);
    else
        ASMAtomicBitClear(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_BREAKPOINT_IO);

    dbgfR3BpUpdateSearchOptimizations(pVM, DBGFBPTYPE_MMIO, &pVM->dbgf.s.Mmio);
    if (pVM->dbgf.s.Mmio.cToSearch)
        ASMAtomicBitSet(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_BREAKPOINT_MMIO);
    else
        ASMAtomicBitClear(&pVM->dbgf.s.bmSelectedEvents, DBGFEVENT_BREAKPOINT_MMIO);

    IOMR3NotifyBreakpointCountChange(pVM, pVM->dbgf.s.PortIo.cToSearch != 0, pVM->dbgf.s.Mmio.cToSearch != 0);
    return VINF_SUCCESS;
}


/**
 * EMT worker for DBGFR3BpSetPortIo.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   uPort           The first I/O port.
 * @param   cPorts          The number of I/O ports.
 * @param   fAccess         The access we want to break on.
 * @param   piHitTrigger    The hit count at which the breakpoint start triggering.
 *                          Use 0 (or 1) if it's gonna trigger at once.
 * @param   piHitDisable    The hit count which disables the breakpoint.
 *                          Use ~(uint64_t) if it's never gonna be disabled.
 * @param   piBp            Where to store the breakpoint ID.
 * @thread  EMT(0)
 */
static DECLCALLBACK(int) dbgfR3BpSetPortIo(PUVM pUVM, RTIOPORT uPort, RTIOPORT cPorts, uint32_t fAccess,
                                           uint64_t const *piHitTrigger, uint64_t const *piHitDisable, uint32_t *piBp)
{
    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    *piBp = UINT32_MAX;

    /*
     * Check if the breakpoint already exists.
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(pVM->dbgf.s.aBreakpoints); i++)
        if (   pVM->dbgf.s.aBreakpoints[i].enmType == DBGFBPTYPE_PORT_IO
            && pVM->dbgf.s.aBreakpoints[i].u.PortIo.uPort == uPort
            && pVM->dbgf.s.aBreakpoints[i].u.PortIo.cPorts == cPorts
            && pVM->dbgf.s.aBreakpoints[i].u.PortIo.fAccess == fAccess)
        {
            if (!pVM->dbgf.s.aBreakpoints[i].fEnabled)
            {
                pVM->dbgf.s.aBreakpoints[i].fEnabled = true;
                dbgfR3BpUpdateIom(pVM);
            }
            *piBp = pVM->dbgf.s.aBreakpoints[i].iBp;
            return VINF_DBGF_BP_ALREADY_EXIST;
        }

    /*
     * Allocate and initialize the breakpoint.
     */
    PDBGFBP pBp = dbgfR3BpAlloc(pVM, DBGFBPTYPE_PORT_IO);
    if (!pBp)
        return VERR_DBGF_NO_MORE_BP_SLOTS;
    pBp->iHitTrigger        = *piHitTrigger;
    pBp->iHitDisable        = *piHitDisable;
    pBp->u.PortIo.uPort     = uPort;
    pBp->u.PortIo.cPorts    = cPorts;
    pBp->u.PortIo.fAccess   = fAccess;
    ASMCompilerBarrier();
    pBp->fEnabled           = true;

    /*
     * Tell IOM.
     */
    dbgfR3BpUpdateIom(pVM);
    *piBp = pBp->iBp;
    return VINF_SUCCESS;
}


/**
 * Sets an I/O port breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   uPort           The first I/O port.
 * @param   cPorts          The number of I/O ports, see DBGFBPIOACCESS_XXX.
 * @param   fAccess         The access we want to break on.
 * @param   iHitTrigger     The hit count at which the breakpoint start
 *                          triggering. Use 0 (or 1) if it's gonna trigger at
 *                          once.
 * @param   iHitDisable     The hit count which disables the breakpoint.
 *                          Use ~(uint64_t) if it's never gonna be disabled.
 * @param   piBp            Where to store the breakpoint ID. Optional.
 * @thread  Any thread.
 */
VMMR3DECL(int)  DBGFR3BpSetPortIo(PUVM pUVM, RTIOPORT uPort, RTIOPORT cPorts, uint32_t fAccess,
                                  uint64_t iHitTrigger, uint64_t iHitDisable, uint32_t *piBp)
{
    AssertReturn(!(fAccess & ~DBGFBPIOACCESS_VALID_MASK_PORT_IO), VERR_INVALID_FLAGS);
    AssertReturn(fAccess, VERR_INVALID_FLAGS);
    if (iHitTrigger > iHitDisable)
        return VERR_INVALID_PARAMETER;
    AssertPtrNullReturn(piBp, VERR_INVALID_POINTER);
    AssertReturn(cPorts > 0, VERR_OUT_OF_RANGE);
    AssertReturn((RTIOPORT)(uPort + cPorts) < uPort, VERR_OUT_OF_RANGE);

    /*
     * This must be done on EMT.
     */
    uint32_t iBp = UINT32_MAX;
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpSetPortIo, 7,
                              pUVM, uPort, cPorts, fAccess, &iHitTrigger, &iHitDisable, piBp);
    if (piBp)
        *piBp = iBp;
    LogFlow(("DBGFR3BpSetPortIo: returns %Rrc iBp=%d\n", rc, iBp));
    return rc;
}


/**
 * EMT worker for DBGFR3BpSetMmio.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pGCPhys         The start of the MMIO range to break on.
 * @param   cb              The size of the MMIO range.
 * @param   fAccess         The access we want to break on.
 * @param   piHitTrigger    The hit count at which the breakpoint start triggering.
 *                          Use 0 (or 1) if it's gonna trigger at once.
 * @param   piHitDisable    The hit count which disables the breakpoint.
 *                          Use ~(uint64_t) if it's never gonna be disabled.
 * @param   piBp            Where to store the breakpoint ID.
 * @thread  EMT(0)
 */
static DECLCALLBACK(int) dbgfR3BpSetMmio(PUVM pUVM, PCRTGCPHYS pGCPhys, uint32_t cb, uint32_t fAccess,
                                         uint64_t const *piHitTrigger, uint64_t const *piHitDisable, uint32_t *piBp)
{
    RTGCPHYS const GCPhys = *pGCPhys;

    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    *piBp = UINT32_MAX;

    /*
     * Check if the breakpoint already exists.
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(pVM->dbgf.s.aBreakpoints); i++)
        if (   pVM->dbgf.s.aBreakpoints[i].enmType == DBGFBPTYPE_MMIO
            && pVM->dbgf.s.aBreakpoints[i].u.Mmio.PhysAddr == GCPhys
            && pVM->dbgf.s.aBreakpoints[i].u.Mmio.cb == cb
            && pVM->dbgf.s.aBreakpoints[i].u.Mmio.fAccess == fAccess)
        {
            if (!pVM->dbgf.s.aBreakpoints[i].fEnabled)
            {
                pVM->dbgf.s.aBreakpoints[i].fEnabled = true;
                dbgfR3BpUpdateIom(pVM);
            }
            *piBp = pVM->dbgf.s.aBreakpoints[i].iBp;
            return VINF_DBGF_BP_ALREADY_EXIST;
        }

    /*
     * Allocate and initialize the breakpoint.
     */
    PDBGFBP pBp = dbgfR3BpAlloc(pVM, DBGFBPTYPE_MMIO);
    if (!pBp)
        return VERR_DBGF_NO_MORE_BP_SLOTS;
    pBp->iHitTrigger        = *piHitTrigger;
    pBp->iHitDisable        = *piHitDisable;
    pBp->u.Mmio.PhysAddr    = GCPhys;
    pBp->u.Mmio.cb          = cb;
    pBp->u.Mmio.fAccess     = fAccess;
    ASMCompilerBarrier();
    pBp->fEnabled           = true;

    /*
     * Tell IOM.
     */
    dbgfR3BpUpdateIom(pVM);
    *piBp = pBp->iBp;
    return VINF_SUCCESS;
}


/**
 * Sets a memory mapped I/O breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   GCPhys          The first MMIO address.
 * @param   cb              The size of the MMIO range to break on.
 * @param   fAccess         The access we want to break on.
 * @param   iHitTrigger     The hit count at which the breakpoint start
 *                          triggering. Use 0 (or 1) if it's gonna trigger at
 *                          once.
 * @param   iHitDisable     The hit count which disables the breakpoint.
 *                          Use ~(uint64_t) if it's never gonna be disabled.
 * @param   piBp            Where to store the breakpoint ID. Optional.
 * @thread  Any thread.
 */
VMMR3DECL(int)  DBGFR3BpSetMmio(PUVM pUVM, RTGCPHYS GCPhys, uint32_t cb, uint32_t fAccess,
                                uint64_t iHitTrigger, uint64_t iHitDisable, uint32_t *piBp)
{
    AssertReturn(!(fAccess & ~DBGFBPIOACCESS_VALID_MASK_MMIO), VERR_INVALID_FLAGS);
    AssertReturn(fAccess, VERR_INVALID_FLAGS);
    if (iHitTrigger > iHitDisable)
        return VERR_INVALID_PARAMETER;
    AssertPtrNullReturn(piBp, VERR_INVALID_POINTER);
    AssertReturn(cb, VERR_OUT_OF_RANGE);
    AssertReturn(GCPhys + cb < GCPhys, VERR_OUT_OF_RANGE);

    /*
     * This must be done on EMT.
     */
    uint32_t iBp = UINT32_MAX;
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpSetMmio, 7,
                              pUVM, &GCPhys, cb, fAccess, &iHitTrigger, &iHitDisable, piBp);
    if (piBp)
        *piBp = iBp;
    LogFlow(("DBGFR3BpSetMmio: returns %Rrc iBp=%d\n", rc, iBp));
    return rc;
}


/**
 * EMT worker for DBGFR3BpClear().
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iBp         The id of the breakpoint which should be removed (cleared).
 * @thread  EMT(0)
 * @internal
 */
static DECLCALLBACK(int) dbgfR3BpClear(PUVM pUVM, uint32_t iBp)
{
    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_EMT(pVM);
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    PDBGFBP pBp = dbgfR3BpGet(pVM, iBp);
    if (!pBp)
        return VERR_DBGF_BP_NOT_FOUND;

    /*
     * Disarm the breakpoint if it's enabled.
     */
    if (pBp->fEnabled)
    {
        pBp->fEnabled = false;
        int rc;
        switch (pBp->enmType)
        {
            case DBGFBPTYPE_REG:
                rc = dbgfR3BpRegDisarm(pVM, pBp);
                break;

            case DBGFBPTYPE_INT3:
                rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, dbgfR3BpDisableInt3OnCpu, pBp);
                break;

            case DBGFBPTYPE_REM:
#ifdef VBOX_WITH_REM
                rc = REMR3BreakpointClear(pVM, pBp->u.Rem.GCPtr);
#else
                rc = IEMBreakpointClear(pVM, pBp->u.Rem.GCPtr);
#endif
                break;

            case DBGFBPTYPE_PORT_IO:
            case DBGFBPTYPE_MMIO:
                rc = dbgfR3BpUpdateIom(pVM);
                break;

            default:
                AssertMsgFailedReturn(("Invalid enmType=%d!\n", pBp->enmType), VERR_IPE_NOT_REACHED_DEFAULT_CASE);
        }
        AssertRCReturn(rc, rc);
    }

    /*
     * Free the breakpoint.
     */
    dbgfR3BpFree(pVM, pBp);
    return VINF_SUCCESS;
}


/**
 * Clears a breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iBp         The id of the breakpoint which should be removed (cleared).
 * @thread  Any thread.
 */
VMMR3DECL(int) DBGFR3BpClear(PUVM pUVM, uint32_t iBp)
{
    /*
     * This must be done on EMT.
     */
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpClear, 2, pUVM, iBp);
    LogFlow(("DBGFR3BpClear: returns %Rrc\n", rc));
    return rc;
}


/**
 * EMT worker for DBGFR3BpEnable().
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iBp         The id of the breakpoint which should be enabled.
 * @thread  EMT(0)
 * @internal
 */
static DECLCALLBACK(int) dbgfR3BpEnable(PUVM pUVM, uint32_t iBp)
{
    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    PDBGFBP pBp = dbgfR3BpGet(pVM, iBp);
    if (!pBp)
        return VERR_DBGF_BP_NOT_FOUND;

    /*
     * Already enabled?
     */
    if (pBp->fEnabled)
        return VINF_DBGF_BP_ALREADY_ENABLED;

    /*
     * Arm the breakpoint.
     */
    int rc;
    pBp->fEnabled = true;
    switch (pBp->enmType)
    {
        case DBGFBPTYPE_REG:
            rc = dbgfR3BpRegArm(pVM, pBp);
            break;

        case DBGFBPTYPE_INT3:
            rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, dbgfR3BpEnableInt3OnCpu, pBp);
            break;

        case DBGFBPTYPE_REM:
#ifdef VBOX_WITH_REM
            rc = REMR3BreakpointSet(pVM, pBp->u.Rem.GCPtr);
#else
            rc = IEMBreakpointSet(pVM, pBp->u.Rem.GCPtr);
#endif
            break;

        case DBGFBPTYPE_PORT_IO:
        case DBGFBPTYPE_MMIO:
            rc = dbgfR3BpUpdateIom(pVM);
            break;

        default:
            AssertMsgFailedReturn(("Invalid enmType=%d!\n", pBp->enmType), VERR_IPE_NOT_REACHED_DEFAULT_CASE);
    }
    if (RT_FAILURE(rc))
        pBp->fEnabled = false;

    return rc;
}


/**
 * Enables a breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iBp         The id of the breakpoint which should be enabled.
 * @thread  Any thread.
 */
VMMR3DECL(int) DBGFR3BpEnable(PUVM pUVM, uint32_t iBp)
{
    /*
     * This must be done on EMT.
     */
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpEnable, 2, pUVM, iBp);
    LogFlow(("DBGFR3BpEnable: returns %Rrc\n", rc));
    return rc;
}


/**
 * EMT worker for DBGFR3BpDisable().
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iBp         The id of the breakpoint which should be disabled.
 * @thread  EMT(0)
 * @internal
 */
static DECLCALLBACK(int) dbgfR3BpDisable(PUVM pUVM, uint32_t iBp)
{
    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    PDBGFBP pBp = dbgfR3BpGet(pVM, iBp);
    if (!pBp)
        return VERR_DBGF_BP_NOT_FOUND;

    /*
     * Already enabled?
     */
    if (!pBp->fEnabled)
        return VINF_DBGF_BP_ALREADY_DISABLED;

    /*
     * Remove the breakpoint.
     */
    pBp->fEnabled = false;
    int rc;
    switch (pBp->enmType)
    {
        case DBGFBPTYPE_REG:
            rc = dbgfR3BpRegDisarm(pVM, pBp);
            break;

        case DBGFBPTYPE_INT3:
            rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, dbgfR3BpDisableInt3OnCpu, pBp);
            break;

        case DBGFBPTYPE_REM:
#ifdef VBOX_WITH_REM
            rc = REMR3BreakpointClear(pVM, pBp->u.Rem.GCPtr);
#else
            rc = IEMBreakpointClear(pVM, pBp->u.Rem.GCPtr);
#endif
            break;

        case DBGFBPTYPE_PORT_IO:
        case DBGFBPTYPE_MMIO:
            rc = dbgfR3BpUpdateIom(pVM);
            break;

        default:
            AssertMsgFailedReturn(("Invalid enmType=%d!\n", pBp->enmType), VERR_IPE_NOT_REACHED_DEFAULT_CASE);
    }

    return rc;
}


/**
 * Disables a breakpoint.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   iBp         The id of the breakpoint which should be disabled.
 * @thread  Any thread.
 */
VMMR3DECL(int) DBGFR3BpDisable(PUVM pUVM, uint32_t iBp)
{
    /*
     * This must be done on EMT.
     */
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpDisable, 2, pUVM, iBp);
    LogFlow(("DBGFR3BpDisable: returns %Rrc\n", rc));
    return rc;
}


/**
 * EMT worker for DBGFR3BpEnum().
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pfnCallback The callback function.
 * @param   pvUser      The user argument to pass to the callback.
 * @thread  EMT
 * @internal
 */
static DECLCALLBACK(int) dbgfR3BpEnum(PUVM pUVM, PFNDBGFBPENUM pfnCallback, void *pvUser)
{
    /*
     * Validate input.
     */
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertPtrReturn(pfnCallback, VERR_INVALID_POINTER);

    /*
     * Enumerate the hardware breakpoints.
     */
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(pVM->dbgf.s.aHwBreakpoints); i++)
        if (pVM->dbgf.s.aHwBreakpoints[i].enmType != DBGFBPTYPE_FREE)
        {
            int rc = pfnCallback(pUVM, pvUser, &pVM->dbgf.s.aHwBreakpoints[i]);
            if (RT_FAILURE(rc) || rc == VINF_CALLBACK_RETURN)
                return rc;
        }

    /*
     * Enumerate the other breakpoints.
     */
    for (i = 0; i < RT_ELEMENTS(pVM->dbgf.s.aBreakpoints); i++)
        if (pVM->dbgf.s.aBreakpoints[i].enmType != DBGFBPTYPE_FREE)
        {
            int rc = pfnCallback(pUVM, pvUser, &pVM->dbgf.s.aBreakpoints[i]);
            if (RT_FAILURE(rc) || rc == VINF_CALLBACK_RETURN)
                return rc;
        }

    return VINF_SUCCESS;
}


/**
 * Enumerate the breakpoints.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pfnCallback The callback function.
 * @param   pvUser      The user argument to pass to the callback.
 * @thread  Any thread but the callback will be called from EMT.
 */
VMMR3DECL(int) DBGFR3BpEnum(PUVM pUVM, PFNDBGFBPENUM pfnCallback, void *pvUser)
{
    /*
     * This must be done on EMT.
     */
    int rc = VMR3ReqPriorityCallWaitU(pUVM, 0 /*idDstCpu*/, (PFNRT)dbgfR3BpEnum, 3, pUVM, pfnCallback, pvUser);
    LogFlow(("DBGFR3BpEnum: returns %Rrc\n", rc));
    return rc;
}


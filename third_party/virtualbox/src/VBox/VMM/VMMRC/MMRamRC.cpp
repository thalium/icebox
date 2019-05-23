/* $Id: MMRamRC.cpp $ */
/** @file
 * MMRamGC - Guest Context Ram access Routines, pair for MMRamGCA.asm.
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
#define LOG_GROUP LOG_GROUP_MM
#include <VBox/vmm/mm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/em.h>
#include "MMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/pgm.h>

#include <iprt/assert.h>
#include <VBox/param.h>
#include <VBox/err.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int) mmGCRamTrap0eHandler(PVM pVM, PCPUMCTXCORE pRegFrame);

DECLASM(void) MMGCRamReadNoTrapHandler_EndProc(void);
DECLASM(void) MMGCRamWriteNoTrapHandler_EndProc(void);
DECLASM(void) MMGCRamRead_Error(void);
DECLASM(void) MMGCRamWrite_Error(void);


/**
 * Install MMGCRam Hypervisor page fault handler for normal working
 * of MMGCRamRead and MMGCRamWrite calls.
 * This handler will be automatically removed at page fault.
 * In other case it must be removed by MMGCRamDeregisterTrapHandler call.
 *
 * @param   pVM         The cross context VM structure.
 */
VMMRCDECL(void) MMGCRamRegisterTrapHandler(PVM pVM)
{
    TRPMGCSetTempHandler(pVM, 0xe, mmGCRamTrap0eHandler);
}


/**
 * Remove MMGCRam Hypervisor page fault handler.
 * See description of MMGCRamRegisterTrapHandler call.
 *
 * @param   pVM         The cross context VM structure.
 */
VMMRCDECL(void) MMGCRamDeregisterTrapHandler(PVM pVM)
{
    TRPMGCSetTempHandler(pVM, 0xe, NULL);
}


/**
 * Read data in guest context with \#PF control.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDst        Where to store the read data.
 * @param   pSrc        Pointer to the data to read.
 * @param   cb          Size of data to read.
 */
VMMRCDECL(int) MMGCRamRead(PVM pVM, void *pDst, void *pSrc, size_t cb)
{
    int    rc;
    PVMCPU pVCpu = VMMGetCpu0(pVM);

    /*
     * Save the current trap info, because it will get trashed if our access failed.
     */
    TRPMSaveTrap(pVCpu);

    /*
     * Need to serve the request in a silly loop because the assembly code wasn't
     * written for abrbitrary sizes, only 1/2/4/8.
     */
    MMGCRamRegisterTrapHandler(pVM);
    for (;;)
    {
        size_t cbThisRead;
        switch (cb)
        {
            case 1: cbThisRead = 1; break;
            case 2: cbThisRead = 2; break;
            case 3: cbThisRead = 2; break;
            case 4: cbThisRead = 4; break;
            case 5: cbThisRead = 4; break;
            case 6: cbThisRead = 4; break;
            case 7: cbThisRead = 4; break;
            default:
            case 8: cbThisRead = 8; break;
        }
        rc = MMGCRamReadNoTrapHandler(pDst, pSrc, cbThisRead);
        if (RT_FAILURE(rc) || cbThisRead == cb)
            break;

        /* advance */
        cb   -= cbThisRead;
        pDst  = (uint8_t *)pDst + cbThisRead;
        pSrc  = (uint8_t *)pSrc + cbThisRead;
    }
    MMGCRamDeregisterTrapHandler(pVM);

    if (RT_FAILURE(rc))
        TRPMRestoreTrap(pVCpu);

    return rc;
}


/**
 * Write data in guest context with \#PF control.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDst        Where to write the data.
 * @param   pSrc        Pointer to the data to write.
 * @param   cb          Size of data to write, only 1/2/4 is valid.
 *
 * @deprecated Don't use this as it doesn't check the page state.
 */
VMMRCDECL(int) MMGCRamWrite(PVM pVM, void *pDst, void *pSrc, size_t cb)
{
    PVMCPU pVCpu = VMMGetCpu0(pVM);
    TRPMSaveTrap(pVCpu);  /* save the current trap info, because it will get trashed if our access failed. */

    MMGCRamRegisterTrapHandler(pVM);
    int rc = MMGCRamWriteNoTrapHandler(pDst, pSrc, cb);
    MMGCRamDeregisterTrapHandler(pVM);
    if (RT_FAILURE(rc))
        TRPMRestoreTrap(pVCpu);

    /*
     * And mark the relevant guest page as accessed and dirty.
     */
    PGMGstModifyPage(VMMGetCpu0(pVM), (RTGCPTR)(RTRCUINTPTR)pDst, cb, X86_PTE_A | X86_PTE_D, ~(uint64_t)(X86_PTE_A | X86_PTE_D));

    return rc;
}


/**
 * \#PF Handler for servicing traps inside MMGCRamReadNoTrapHandler and MMGCRamWriteNoTrapHandler functions.
 *
 * @internal
 */
DECLCALLBACK(int) mmGCRamTrap0eHandler(PVM pVM, PCPUMCTXCORE pRegFrame)
{
    /*
     * Page fault inside MMGCRamRead()? Resume at *_Error.
     */
    if (    (uintptr_t)&MMGCRamReadNoTrapHandler < (uintptr_t)pRegFrame->eip
        &&  (uintptr_t)pRegFrame->eip < (uintptr_t)&MMGCRamReadNoTrapHandler_EndProc)
    {
        /* Must be a read violation. */
        AssertReturn(!(TRPMGetErrorCode(VMMGetCpu0(pVM)) & X86_TRAP_PF_RW), VERR_MM_BAD_TRAP_TYPE_IPE);
        pRegFrame->eip = (uintptr_t)&MMGCRamRead_Error;
        return VINF_SUCCESS;
    }

    /*
     * Page fault inside MMGCRamWrite()? Resume at _Error.
     */
    if (    (uintptr_t)&MMGCRamWriteNoTrapHandler < (uintptr_t)pRegFrame->eip
        &&  (uintptr_t)pRegFrame->eip < (uintptr_t)&MMGCRamWriteNoTrapHandler_EndProc)
    {
        /* Must be a write violation. */
        AssertReturn(TRPMGetErrorCode(VMMGetCpu0(pVM)) & X86_TRAP_PF_RW, VERR_MM_BAD_TRAP_TYPE_IPE);
        pRegFrame->eip = (uintptr_t)&MMGCRamWrite_Error;
        return VINF_SUCCESS;
    }

    /*
     * #PF is not handled - cause guru meditation.
     */
    return VERR_INTERNAL_ERROR;
}


/* $Id: PGMR0.cpp $ */
/** @file
 * PGM - Page Manager and Monitor, Ring-0.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_PGM
#include <VBox/rawpci.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/gmm.h>
#include <VBox/vmm/gvm.h>
#include "PGMInternal.h"
#include <VBox/vmm/vm.h>
#include "PGMInline.h"
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/mem.h>


/*
 * Instantiate the ring-0 header/code templates.
 */
#define PGM_BTH_NAME(name)          PGM_BTH_NAME_32BIT_PROT(name)
#include "PGMR0Bth.h"
#undef PGM_BTH_NAME

#define PGM_BTH_NAME(name)          PGM_BTH_NAME_PAE_PROT(name)
#include "PGMR0Bth.h"
#undef PGM_BTH_NAME

#define PGM_BTH_NAME(name)          PGM_BTH_NAME_AMD64_PROT(name)
#include "PGMR0Bth.h"
#undef PGM_BTH_NAME

#define PGM_BTH_NAME(name)          PGM_BTH_NAME_EPT_PROT(name)
#include "PGMR0Bth.h"
#undef PGM_BTH_NAME


/**
 * Worker function for PGMR3PhysAllocateHandyPages and pgmPhysEnsureHandyPage.
 *
 * @returns The following VBox status codes.
 * @retval  VINF_SUCCESS on success. FF cleared.
 * @retval  VINF_EM_NO_MEMORY if we're out of memory. The FF is set in this case.
 *
 * @param   pGVM        The global (ring-0) VM structure.
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       The ID of the calling EMT.
 *
 * @thread  EMT(idCpu)
 *
 * @remarks Must be called from within the PGM critical section. The caller
 *          must clear the new pages.
 */
VMMR0_INT_DECL(int) PGMR0PhysAllocateHandyPages(PGVM pGVM, PVM pVM, VMCPUID idCpu)
{
    /*
     * Validate inputs.
     */
    AssertReturn(idCpu < pGVM->cCpus, VERR_INVALID_CPU_ID); /* caller already checked this, but just to be sure. */
    AssertReturn(pGVM->aCpus[idCpu].hEMT == RTThreadNativeSelf(), VERR_NOT_OWNER);
    PGM_LOCK_ASSERT_OWNER_EX(pVM, &pVM->aCpus[idCpu]);

    /*
     * Check for error injection.
     */
    if (RT_UNLIKELY(pVM->pgm.s.fErrInjHandyPages))
        return VERR_NO_MEMORY;

    /*
     * Try allocate a full set of handy pages.
     */
    uint32_t iFirst = pVM->pgm.s.cHandyPages;
    AssertReturn(iFirst <= RT_ELEMENTS(pVM->pgm.s.aHandyPages), VERR_PGM_HANDY_PAGE_IPE);
    uint32_t cPages = RT_ELEMENTS(pVM->pgm.s.aHandyPages) - iFirst;
    if (!cPages)
        return VINF_SUCCESS;
    int rc = GMMR0AllocateHandyPages(pGVM, pVM, idCpu, cPages, cPages, &pVM->pgm.s.aHandyPages[iFirst]);
    if (RT_SUCCESS(rc))
    {
#ifdef VBOX_STRICT
        for (uint32_t i = 0; i < RT_ELEMENTS(pVM->pgm.s.aHandyPages); i++)
        {
            Assert(pVM->pgm.s.aHandyPages[i].idPage != NIL_GMM_PAGEID);
            Assert(pVM->pgm.s.aHandyPages[i].idPage <= GMM_PAGEID_LAST);
            Assert(pVM->pgm.s.aHandyPages[i].idSharedPage == NIL_GMM_PAGEID);
            Assert(pVM->pgm.s.aHandyPages[i].HCPhysGCPhys != NIL_RTHCPHYS);
            Assert(!(pVM->pgm.s.aHandyPages[i].HCPhysGCPhys & ~X86_PTE_PAE_PG_MASK));
        }
#endif

        pVM->pgm.s.cHandyPages = RT_ELEMENTS(pVM->pgm.s.aHandyPages);
    }
    else if (rc != VERR_GMM_SEED_ME)
    {
        if (    (   rc == VERR_GMM_HIT_GLOBAL_LIMIT
                 || rc == VERR_GMM_HIT_VM_ACCOUNT_LIMIT)
            &&  iFirst < PGM_HANDY_PAGES_MIN)
        {

#ifdef VBOX_STRICT
            /* We're ASSUMING that GMM has updated all the entires before failing us. */
            uint32_t i;
            for (i = iFirst; i < RT_ELEMENTS(pVM->pgm.s.aHandyPages); i++)
            {
                Assert(pVM->pgm.s.aHandyPages[i].idPage == NIL_GMM_PAGEID);
                Assert(pVM->pgm.s.aHandyPages[i].idSharedPage == NIL_GMM_PAGEID);
                Assert(pVM->pgm.s.aHandyPages[i].HCPhysGCPhys == NIL_RTHCPHYS);
            }
#endif

            /*
             * Reduce the number of pages until we hit the minimum limit.
             */
            do
            {
                cPages >>= 1;
                if (cPages + iFirst < PGM_HANDY_PAGES_MIN)
                    cPages = PGM_HANDY_PAGES_MIN - iFirst;
                rc = GMMR0AllocateHandyPages(pGVM, pVM, idCpu, 0, cPages, &pVM->pgm.s.aHandyPages[iFirst]);
            } while (   (   rc == VERR_GMM_HIT_GLOBAL_LIMIT
                         || rc == VERR_GMM_HIT_VM_ACCOUNT_LIMIT)
                     && cPages + iFirst > PGM_HANDY_PAGES_MIN);
            if (RT_SUCCESS(rc))
            {
#ifdef VBOX_STRICT
                i = iFirst + cPages;
                while (i-- > 0)
                {
                    Assert(pVM->pgm.s.aHandyPages[i].idPage != NIL_GMM_PAGEID);
                    Assert(pVM->pgm.s.aHandyPages[i].idPage <= GMM_PAGEID_LAST);
                    Assert(pVM->pgm.s.aHandyPages[i].idSharedPage == NIL_GMM_PAGEID);
                    Assert(pVM->pgm.s.aHandyPages[i].HCPhysGCPhys != NIL_RTHCPHYS);
                    Assert(!(pVM->pgm.s.aHandyPages[i].HCPhysGCPhys & ~X86_PTE_PAE_PG_MASK));
                }

                for (i = cPages + iFirst; i < RT_ELEMENTS(pVM->pgm.s.aHandyPages); i++)
                {
                    Assert(pVM->pgm.s.aHandyPages[i].idPage == NIL_GMM_PAGEID);
                    Assert(pVM->pgm.s.aHandyPages[i].idSharedPage == NIL_GMM_PAGEID);
                    Assert(pVM->pgm.s.aHandyPages[i].HCPhysGCPhys == NIL_RTHCPHYS);
                }
#endif

                pVM->pgm.s.cHandyPages = iFirst + cPages;
            }
        }

        if (RT_FAILURE(rc) && rc != VERR_GMM_SEED_ME)
        {
            LogRel(("PGMR0PhysAllocateHandyPages: rc=%Rrc iFirst=%d cPages=%d\n", rc, iFirst, cPages));
            VM_FF_SET(pVM, VM_FF_PGM_NO_MEMORY);
        }
    }


    LogFlow(("PGMR0PhysAllocateHandyPages: cPages=%d rc=%Rrc\n", cPages, rc));
    return rc;
}


/**
 * Flushes any changes pending in the handy page array.
 *
 * It is very important that this gets done when page sharing is enabled.
 *
 * @returns The following VBox status codes.
 * @retval  VINF_SUCCESS on success. FF cleared.
 *
 * @param   pGVM        The global (ring-0) VM structure.
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       The ID of the calling EMT.
 *
 * @thread  EMT(idCpu)
 *
 * @remarks Must be called from within the PGM critical section.
 */
VMMR0_INT_DECL(int) PGMR0PhysFlushHandyPages(PGVM pGVM, PVM pVM, VMCPUID idCpu)
{
    /*
     * Validate inputs.
     */
    AssertReturn(idCpu < pGVM->cCpus, VERR_INVALID_CPU_ID); /* caller already checked this, but just to be sure. */
    AssertReturn(pGVM->aCpus[idCpu].hEMT == RTThreadNativeSelf(), VERR_NOT_OWNER);
    PGM_LOCK_ASSERT_OWNER_EX(pVM, &pVM->aCpus[idCpu]);

    /*
     * Try allocate a full set of handy pages.
     */
    uint32_t iFirst = pVM->pgm.s.cHandyPages;
    AssertReturn(iFirst <= RT_ELEMENTS(pVM->pgm.s.aHandyPages), VERR_PGM_HANDY_PAGE_IPE);
    uint32_t cPages = RT_ELEMENTS(pVM->pgm.s.aHandyPages) - iFirst;
    if (!cPages)
        return VINF_SUCCESS;
    int rc = GMMR0AllocateHandyPages(pGVM, pVM, idCpu, cPages, 0, &pVM->pgm.s.aHandyPages[iFirst]);

    LogFlow(("PGMR0PhysFlushHandyPages: cPages=%d rc=%Rrc\n", cPages, rc));
    return rc;
}


/**
 * Worker function for PGMR3PhysAllocateLargeHandyPage
 *
 * @returns The following VBox status codes.
 * @retval  VINF_SUCCESS on success.
 * @retval  VINF_EM_NO_MEMORY if we're out of memory.
 *
 * @param   pGVM        The global (ring-0) VM structure.
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       The ID of the calling EMT.
 *
 * @thread  EMT(idCpu)
 *
 * @remarks Must be called from within the PGM critical section. The caller
 *          must clear the new pages.
 */
VMMR0_INT_DECL(int) PGMR0PhysAllocateLargeHandyPage(PGVM pGVM, PVM pVM, VMCPUID idCpu)
{
    /*
     * Validate inputs.
     */
    AssertReturn(idCpu < pGVM->cCpus, VERR_INVALID_CPU_ID); /* caller already checked this, but just to be sure. */
    AssertReturn(pGVM->aCpus[idCpu].hEMT == RTThreadNativeSelf(), VERR_NOT_OWNER);
    PGM_LOCK_ASSERT_OWNER_EX(pVM, &pVM->aCpus[idCpu]);
    Assert(!pVM->pgm.s.cLargeHandyPages);

    /*
     * Do the job.
     */
    int rc = GMMR0AllocateLargePage(pGVM, pVM, idCpu, _2M,
                                    &pVM->pgm.s.aLargeHandyPage[0].idPage,
                                    &pVM->pgm.s.aLargeHandyPage[0].HCPhysGCPhys);
    if (RT_SUCCESS(rc))
        pVM->pgm.s.cLargeHandyPages = 1;

    return rc;
}


#ifdef VBOX_WITH_PCI_PASSTHROUGH
/* Interface sketch.  The interface belongs to a global PCI pass-through
   manager.  It shall use the global VM handle, not the user VM handle to
   store the per-VM info (domain) since that is all ring-0 stuff, thus
   passing pGVM here.  I've tentitively prefixed the functions 'GPciRawR0',
   we can discuss the PciRaw code re-organtization when I'm back from
   vacation.

   I've implemented the initial IOMMU set up below.  For things to work
   reliably, we will probably need add a whole bunch of checks and
   GPciRawR0GuestPageUpdate call to the PGM code.  For the present,
   assuming nested paging (enforced) and prealloc (enforced), no
   ballooning (check missing), page sharing (check missing) or live
   migration (check missing), it might work fine.  At least if some
   VM power-off hook is present and can tear down the IOMMU page tables. */

/**
 * Tells the global PCI pass-through manager that we are about to set up the
 * guest page to host page mappings for the specfied VM.
 *
 * @returns VBox status code.
 *
 * @param   pGVM                The ring-0 VM structure.
 */
VMMR0_INT_DECL(int) GPciRawR0GuestPageBeginAssignments(PGVM pGVM)
{
    NOREF(pGVM);
    return VINF_SUCCESS;
}


/**
 * Assigns a host page mapping for a guest page.
 *
 * This is only used when setting up the mappings, i.e. between
 * GPciRawR0GuestPageBeginAssignments and GPciRawR0GuestPageEndAssignments.
 *
 * @returns VBox status code.
 * @param   pGVM                The ring-0 VM structure.
 * @param   GCPhys              The address of the guest page (page aligned).
 * @param   HCPhys              The address of the host page (page aligned).
 */
VMMR0_INT_DECL(int) GPciRawR0GuestPageAssign(PGVM pGVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys)
{
    AssertReturn(!(GCPhys & PAGE_OFFSET_MASK), VERR_INTERNAL_ERROR_3);
    AssertReturn(!(HCPhys & PAGE_OFFSET_MASK), VERR_INTERNAL_ERROR_3);

    if (pGVM->rawpci.s.pfnContigMemInfo)
        /** @todo what do we do on failure? */
        pGVM->rawpci.s.pfnContigMemInfo(&pGVM->rawpci.s, HCPhys, GCPhys, PAGE_SIZE, PCIRAW_MEMINFO_MAP);

    return VINF_SUCCESS;
}


/**
 * Indicates that the specified guest page doesn't exists but doesn't have host
 * page mapping we trust PCI pass-through with.
 *
 * This is only used when setting up the mappings, i.e. between
 * GPciRawR0GuestPageBeginAssignments and GPciRawR0GuestPageEndAssignments.
 *
 * @returns VBox status code.
 * @param   pGVM                The ring-0 VM structure.
 * @param   GCPhys              The address of the guest page (page aligned).
 * @param   HCPhys              The address of the host page (page aligned).
 */
VMMR0_INT_DECL(int) GPciRawR0GuestPageUnassign(PGVM pGVM, RTGCPHYS GCPhys)
{
    AssertReturn(!(GCPhys & PAGE_OFFSET_MASK), VERR_INTERNAL_ERROR_3);

    if (pGVM->rawpci.s.pfnContigMemInfo)
        /** @todo what do we do on failure? */
        pGVM->rawpci.s.pfnContigMemInfo(&pGVM->rawpci.s, 0, GCPhys, PAGE_SIZE, PCIRAW_MEMINFO_UNMAP);

    return VINF_SUCCESS;
}


/**
 * Tells the global PCI pass-through manager that we have completed setting up
 * the guest page to host page mappings for the specfied VM.
 *
 * This complements GPciRawR0GuestPageBeginAssignments and will be called even
 * if some page assignment failed.
 *
 * @returns VBox status code.
 *
 * @param   pGVM                The ring-0 VM structure.
 */
VMMR0_INT_DECL(int) GPciRawR0GuestPageEndAssignments(PGVM pGVM)
{
    NOREF(pGVM);
    return VINF_SUCCESS;
}


/**
 * Tells the global PCI pass-through manager that a guest page mapping has
 * changed after the initial setup.
 *
 * @returns VBox status code.
 * @param   pGVM                The ring-0 VM structure.
 * @param   GCPhys              The address of the guest page (page aligned).
 * @param   HCPhys              The new host page address or NIL_RTHCPHYS if
 *                              now unassigned.
 */
VMMR0_INT_DECL(int) GPciRawR0GuestPageUpdate(PGVM pGVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys)
{
    AssertReturn(!(GCPhys & PAGE_OFFSET_MASK), VERR_INTERNAL_ERROR_4);
    AssertReturn(!(HCPhys & PAGE_OFFSET_MASK) || HCPhys == NIL_RTHCPHYS, VERR_INTERNAL_ERROR_4);
    NOREF(pGVM);
    return VINF_SUCCESS;
}

#endif /* VBOX_WITH_PCI_PASSTHROUGH */


/**
 * Sets up the IOMMU when raw PCI device is enabled.
 *
 * @note    This is a hack that will probably be remodelled and refined later!
 *
 * @returns VBox status code.
 *
 * @param   pGVM                The global (ring-0) VM structure.
 * @param   pVM                 The cross context VM structure.
 */
VMMR0_INT_DECL(int) PGMR0PhysSetupIoMmu(PGVM pGVM, PVM pVM)
{
    int rc = GVMMR0ValidateGVMandVM(pGVM, pVM);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_PCI_PASSTHROUGH
    if (pVM->pgm.s.fPciPassthrough)
    {
        /*
         * The Simplistic Approach - Enumerate all the pages and call tell the
         * IOMMU about each of them.
         */
        pgmLock(pVM);
        rc = GPciRawR0GuestPageBeginAssignments(pGVM);
        if (RT_SUCCESS(rc))
        {
            for (PPGMRAMRANGE pRam = pVM->pgm.s.pRamRangesXR0; RT_SUCCESS(rc) && pRam; pRam = pRam->pNextR0)
            {
                PPGMPAGE    pPage  = &pRam->aPages[0];
                RTGCPHYS    GCPhys = pRam->GCPhys;
                uint32_t    cLeft  = pRam->cb >> PAGE_SHIFT;
                while (cLeft-- > 0)
                {
                    /* Only expose pages that are 100% safe for now. */
                    if (   PGM_PAGE_GET_TYPE(pPage) == PGMPAGETYPE_RAM
                        && PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED
                        && !PGM_PAGE_HAS_ANY_HANDLERS(pPage))
                        rc = GPciRawR0GuestPageAssign(pGVM, GCPhys, PGM_PAGE_GET_HCPHYS(pPage));
                    else
                        rc = GPciRawR0GuestPageUnassign(pGVM, GCPhys);

                    /* next */
                    pPage++;
                    GCPhys += PAGE_SIZE;
                }
            }

            int rc2 = GPciRawR0GuestPageEndAssignments(pGVM);
            if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                rc = rc2;
        }
        pgmUnlock(pVM);
    }
    else
#endif
        rc = VERR_NOT_SUPPORTED;
    return rc;
}


/**
 * \#PF Handler for nested paging.
 *
 * @returns VBox status code (appropriate for trap handling and GC return).
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   enmShwPagingMode    Paging mode for the nested page tables.
 * @param   uErr                The trap error code.
 * @param   pRegFrame           Trap register frame.
 * @param   GCPhysFault         The fault address.
 */
VMMR0DECL(int) PGMR0Trap0eHandlerNestedPaging(PVM pVM, PVMCPU pVCpu, PGMMODE enmShwPagingMode, RTGCUINT uErr,
                                              PCPUMCTXCORE pRegFrame, RTGCPHYS GCPhysFault)
{
    int rc;

    LogFlow(("PGMTrap0eHandler: uErr=%RGx GCPhysFault=%RGp eip=%RGv\n", uErr, GCPhysFault, (RTGCPTR)pRegFrame->rip));
    STAM_PROFILE_START(&pVCpu->pgm.s.StatRZTrap0e, a);
    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = NULL; } );

    /* AMD uses the host's paging mode; Intel has a single mode (EPT). */
    AssertMsg(   enmShwPagingMode == PGMMODE_32_BIT || enmShwPagingMode == PGMMODE_PAE      || enmShwPagingMode == PGMMODE_PAE_NX
              || enmShwPagingMode == PGMMODE_AMD64  || enmShwPagingMode == PGMMODE_AMD64_NX || enmShwPagingMode == PGMMODE_EPT,
              ("enmShwPagingMode=%d\n", enmShwPagingMode));

    /* Reserved shouldn't end up here. */
    Assert(!(uErr & X86_TRAP_PF_RSVD));

#ifdef VBOX_WITH_STATISTICS
    /*
     * Error code stats.
     */
    if (uErr & X86_TRAP_PF_US)
    {
        if (!(uErr & X86_TRAP_PF_P))
        {
            if (uErr & X86_TRAP_PF_RW)
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eUSNotPresentWrite);
            else
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eUSNotPresentRead);
        }
        else if (uErr & X86_TRAP_PF_RW)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eUSWrite);
        else if (uErr & X86_TRAP_PF_RSVD)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eUSReserved);
        else if (uErr & X86_TRAP_PF_ID)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eUSNXE);
        else
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eUSRead);
    }
    else
    {   /* Supervisor */
        if (!(uErr & X86_TRAP_PF_P))
        {
            if (uErr & X86_TRAP_PF_RW)
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eSVNotPresentWrite);
            else
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eSVNotPresentRead);
        }
        else if (uErr & X86_TRAP_PF_RW)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eSVWrite);
        else if (uErr & X86_TRAP_PF_ID)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eSNXE);
        else if (uErr & X86_TRAP_PF_RSVD)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eSVReserved);
    }
#endif

    /*
     * Call the worker.
     *
     * Note! We pretend the guest is in protected mode without paging, so we
     *       can use existing code to build the nested page tables.
     */
    bool fLockTaken = false;
    switch (enmShwPagingMode)
    {
        case PGMMODE_32_BIT:
            rc = PGM_BTH_NAME_32BIT_PROT(Trap0eHandler)(pVCpu, uErr, pRegFrame, GCPhysFault, &fLockTaken);
            break;
        case PGMMODE_PAE:
        case PGMMODE_PAE_NX:
            rc = PGM_BTH_NAME_PAE_PROT(Trap0eHandler)(pVCpu, uErr, pRegFrame, GCPhysFault, &fLockTaken);
            break;
        case PGMMODE_AMD64:
        case PGMMODE_AMD64_NX:
            rc = PGM_BTH_NAME_AMD64_PROT(Trap0eHandler)(pVCpu, uErr, pRegFrame, GCPhysFault, &fLockTaken);
            break;
        case PGMMODE_EPT:
            rc = PGM_BTH_NAME_EPT_PROT(Trap0eHandler)(pVCpu, uErr, pRegFrame, GCPhysFault, &fLockTaken);
            break;
        default:
            AssertFailed();
            rc = VERR_INVALID_PARAMETER;
            break;
    }
    if (fLockTaken)
    {
        PGM_LOCK_ASSERT_OWNER(pVM);
        pgmUnlock(pVM);
    }

    if (rc == VINF_PGM_SYNCPAGE_MODIFIED_PDE)
        rc = VINF_SUCCESS;
    /*
     * Handle the case where we cannot interpret the instruction because we cannot get the guest physical address
     * via its page tables, see @bugref{6043}.
     */
    else if (   rc == VERR_PAGE_NOT_PRESENT                 /* SMP only ; disassembly might fail. */
             || rc == VERR_PAGE_TABLE_NOT_PRESENT           /* seen with UNI & SMP */
             || rc == VERR_PAGE_DIRECTORY_PTR_NOT_PRESENT   /* seen with SMP */
             || rc == VERR_PAGE_MAP_LEVEL4_NOT_PRESENT)     /* precaution */
    {
        Log(("WARNING: Unexpected VERR_PAGE_TABLE_NOT_PRESENT (%d) for page fault at %RGp error code %x (rip=%RGv)\n", rc, GCPhysFault, uErr, pRegFrame->rip));
        /* Some kind of inconsistency in the SMP case; it's safe to just execute the instruction again; not sure about
           single VCPU VMs though. */
        rc = VINF_SUCCESS;
    }

    STAM_STATS({ if (!pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution))
                    pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2Misc; });
    STAM_PROFILE_STOP_EX(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0e, pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution), a);
    return rc;
}


/**
 * \#PF Handler for deliberate nested paging misconfiguration (/reserved bit)
 * employed for MMIO pages.
 *
 * @returns VBox status code (appropriate for trap handling and GC return).
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   enmShwPagingMode    Paging mode for the nested page tables.
 * @param   pRegFrame           Trap register frame.
 * @param   GCPhysFault         The fault address.
 * @param   uErr                The error code, UINT32_MAX if not available
 *                              (VT-x).
 */
VMMR0DECL(VBOXSTRICTRC) PGMR0Trap0eHandlerNPMisconfig(PVM pVM, PVMCPU pVCpu, PGMMODE enmShwPagingMode,
                                                      PCPUMCTXCORE pRegFrame, RTGCPHYS GCPhysFault, uint32_t uErr)
{
#ifdef PGM_WITH_MMIO_OPTIMIZATIONS
    STAM_PROFILE_START(&pVCpu->CTX_SUFF(pStats)->StatR0NpMiscfg, a);
    VBOXSTRICTRC rc;

    /*
     * Try lookup the all access physical handler for the address.
     */
    pgmLock(pVM);
    PPGMPHYSHANDLER         pHandler     = pgmHandlerPhysicalLookup(pVM, GCPhysFault);
    PPGMPHYSHANDLERTYPEINT  pHandlerType = RT_LIKELY(pHandler) ? PGMPHYSHANDLER_GET_TYPE(pVM, pHandler) : NULL;
    if (RT_LIKELY(pHandler && pHandlerType->enmKind != PGMPHYSHANDLERKIND_WRITE))
    {
        /*
         * If the handle has aliases page or pages that have been temporarily
         * disabled, we'll have to take a detour to make sure we resync them
         * to avoid lots of unnecessary exits.
         */
        PPGMPAGE pPage;
        if (   (   pHandler->cAliasedPages
                || pHandler->cTmpOffPages)
            && (   (pPage = pgmPhysGetPage(pVM, GCPhysFault)) == NULL
                || PGM_PAGE_GET_HNDL_PHYS_STATE(pPage) == PGM_PAGE_HNDL_PHYS_STATE_DISABLED)
           )
        {
            Log(("PGMR0Trap0eHandlerNPMisconfig: Resyncing aliases / tmp-off page at %RGp (uErr=%#x) %R[pgmpage]\n", GCPhysFault, uErr, pPage));
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatR0NpMiscfgSyncPage);
            rc = pgmShwSyncNestedPageLocked(pVCpu, GCPhysFault, 1 /*cPages*/, enmShwPagingMode);
            pgmUnlock(pVM);
        }
        else
        {
            if (pHandlerType->CTX_SUFF(pfnPfHandler))
            {
                void *pvUser = pHandler->CTX_SUFF(pvUser);
                STAM_PROFILE_START(&pHandler->Stat, h);
                pgmUnlock(pVM);

                Log6(("PGMR0Trap0eHandlerNPMisconfig: calling %p(,%#x,,%RGp,%p)\n", pHandlerType->CTX_SUFF(pfnPfHandler), uErr, GCPhysFault, pvUser));
                rc = pHandlerType->CTX_SUFF(pfnPfHandler)(pVM, pVCpu, uErr == UINT32_MAX ? RTGCPTR_MAX : uErr, pRegFrame,
                                                          GCPhysFault, GCPhysFault, pvUser);

#ifdef VBOX_WITH_STATISTICS
                pgmLock(pVM);
                pHandler = pgmHandlerPhysicalLookup(pVM, GCPhysFault);
                if (pHandler)
                    STAM_PROFILE_STOP(&pHandler->Stat, h);
                pgmUnlock(pVM);
#endif
            }
            else
            {
                pgmUnlock(pVM);
                Log(("PGMR0Trap0eHandlerNPMisconfig: %RGp (uErr=%#x) -> R3\n", GCPhysFault, uErr));
                rc = VINF_EM_RAW_EMULATE_INSTR;
            }
        }
    }
    else
    {
        /*
         * Must be out of sync, so do a SyncPage and restart the instruction.
         *
         * ASSUMES that ALL handlers are page aligned and covers whole pages
         * (assumption asserted in PGMHandlerPhysicalRegisterEx).
         */
        Log(("PGMR0Trap0eHandlerNPMisconfig: Out of sync page at %RGp (uErr=%#x)\n", GCPhysFault, uErr));
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatR0NpMiscfgSyncPage);
        rc = pgmShwSyncNestedPageLocked(pVCpu, GCPhysFault, 1 /*cPages*/, enmShwPagingMode);
        pgmUnlock(pVM);
    }

    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatR0NpMiscfg, a);
    return rc;

#else
    AssertLogRelFailed();
    return VERR_PGM_NOT_USED_IN_MODE;
#endif
}


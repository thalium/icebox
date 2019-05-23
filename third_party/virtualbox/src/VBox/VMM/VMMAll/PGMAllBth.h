/* $Id: PGMAllBth.h $ */
/** @file
 * VBox - Page Manager, Shadow+Guest Paging Template - All context code.
 *
 * @remarks The nested page tables on AMD makes use of PGM_SHW_TYPE in
 *          {PGM_TYPE_AMD64, PGM_TYPE_PAE and PGM_TYPE_32BIT} and PGM_GST_TYPE
 *          set to PGM_TYPE_PROT.  Half of the code in this file is not
 *          exercised with PGM_SHW_TYPE set to PGM_TYPE_NESTED.
 *
 * @remarks Extended page tables (intel) are built with PGM_GST_TYPE set to
 *          PGM_TYPE_PROT (and PGM_SHW_TYPE set to PGM_TYPE_EPT).
 *
 * @remarks This file is one big \#ifdef-orgy!
 *
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

#ifdef _MSC_VER
/** @todo we're generating unnecessary code in nested/ept shadow mode and for
 *        real/prot-guest+RC mode. */
# pragma warning(disable: 4505)
#endif

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
RT_C_DECLS_BEGIN
PGM_BTH_DECL(int, Trap0eHandler)(PVMCPU pVCpu, RTGCUINT uErr, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, bool *pfLockTaken);
PGM_BTH_DECL(int, InvalidatePage)(PVMCPU pVCpu, RTGCPTR GCPtrPage);
static int PGM_BTH_NAME(SyncPage)(PVMCPU pVCpu, GSTPDE PdeSrc, RTGCPTR GCPtrPage, unsigned cPages, unsigned uErr);
static int PGM_BTH_NAME(CheckDirtyPageFault)(PVMCPU pVCpu, uint32_t uErr, PSHWPDE pPdeDst, GSTPDE const *pPdeSrc, RTGCPTR GCPtrPage);
static int PGM_BTH_NAME(SyncPT)(PVMCPU pVCpu, unsigned iPD, PGSTPD pPDSrc, RTGCPTR GCPtrPage);
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
static void PGM_BTH_NAME(SyncPageWorker)(PVMCPU pVCpu, PSHWPTE pPteDst, GSTPDE PdeSrc, GSTPTE PteSrc, PPGMPOOLPAGE pShwPage, unsigned iPTDst);
# else
static void PGM_BTH_NAME(SyncPageWorker)(PVMCPU pVCpu, PSHWPTE pPteDst, RTGCPHYS GCPhysPage, PPGMPOOLPAGE pShwPage, unsigned iPTDst);
#endif
PGM_BTH_DECL(int, VerifyAccessSyncPage)(PVMCPU pVCpu, RTGCPTR Addr, unsigned fPage, unsigned uErr);
PGM_BTH_DECL(int, PrefetchPage)(PVMCPU pVCpu, RTGCPTR GCPtrPage);
PGM_BTH_DECL(int, SyncCR3)(PVMCPU pVCpu, uint64_t cr0, uint64_t cr3, uint64_t cr4, bool fGlobal);
#ifdef VBOX_STRICT
PGM_BTH_DECL(unsigned, AssertCR3)(PVMCPU pVCpu, uint64_t cr3, uint64_t cr4, RTGCPTR GCPtr = 0, RTGCPTR cb = ~(RTGCPTR)0);
#endif
PGM_BTH_DECL(int, MapCR3)(PVMCPU pVCpu, RTGCPHYS GCPhysCR3);
PGM_BTH_DECL(int, UnmapCR3)(PVMCPU pVCpu);
RT_C_DECLS_END


/*
 * Filter out some illegal combinations of guest and shadow paging, so we can
 * remove redundant checks inside functions.
 */
#if      PGM_GST_TYPE == PGM_TYPE_PAE && PGM_SHW_TYPE != PGM_TYPE_PAE && PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT
# error "Invalid combination; PAE guest implies PAE shadow"
#endif

#if     (PGM_GST_TYPE == PGM_TYPE_REAL || PGM_GST_TYPE == PGM_TYPE_PROT) \
    && !(PGM_SHW_TYPE == PGM_TYPE_32BIT || PGM_SHW_TYPE == PGM_TYPE_PAE || PGM_SHW_TYPE == PGM_TYPE_AMD64 || PGM_SHW_TYPE == PGM_TYPE_NESTED || PGM_SHW_TYPE == PGM_TYPE_EPT)
# error "Invalid combination; real or protected mode without paging implies 32 bits or PAE shadow paging."
#endif

#if     (PGM_GST_TYPE == PGM_TYPE_32BIT || PGM_GST_TYPE == PGM_TYPE_PAE) \
    && !(PGM_SHW_TYPE == PGM_TYPE_32BIT || PGM_SHW_TYPE == PGM_TYPE_PAE || PGM_SHW_TYPE == PGM_TYPE_NESTED || PGM_SHW_TYPE == PGM_TYPE_EPT)
# error "Invalid combination; 32 bits guest paging or PAE implies 32 bits or PAE shadow paging."
#endif

#if    (PGM_GST_TYPE == PGM_TYPE_AMD64 && PGM_SHW_TYPE != PGM_TYPE_AMD64 && PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT) \
    || (PGM_SHW_TYPE == PGM_TYPE_AMD64 && PGM_GST_TYPE != PGM_TYPE_AMD64 && PGM_GST_TYPE != PGM_TYPE_PROT)
# error "Invalid combination; AMD64 guest implies AMD64 shadow and vice versa"
#endif

#ifndef IN_RING3

# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
/**
 * Deal with a guest page fault.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_EM_RAW_GUEST_TRAP
 * @retval  VINF_EM_RAW_EMULATE_INSTR
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   pGstWalk        The guest page table walk result.
 * @param   uErr            The error code.
 */
PGM_BTH_DECL(VBOXSTRICTRC, Trap0eHandlerGuestFault)(PVMCPU pVCpu, PGSTPTWALK pGstWalk, RTGCUINT uErr)
{
#  if !defined(PGM_WITHOUT_MAPPINGS) && (PGM_GST_TYPE == PGM_TYPE_32BIT || PGM_GST_TYPE == PGM_TYPE_PAE)
    /*
     * Check for write conflicts with our hypervisor mapping.
     *
     * If the  guest happens to access a non-present page, where our hypervisor
     * is currently mapped, then we'll create a #PF storm in the guest.
     */
    if (   (uErr & (X86_TRAP_PF_P | X86_TRAP_PF_RW)) == (X86_TRAP_PF_P | X86_TRAP_PF_RW)
        && pgmMapAreMappingsEnabled(pVCpu->CTX_SUFF(pVM))
        && MMHyperIsInsideArea(pVCpu->CTX_SUFF(pVM), pGstWalk->Core.GCPtr))
    {
        /* Force a CR3 sync to check for conflicts and emulate the instruction. */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
        STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2GuestTrap; });
        return VINF_EM_RAW_EMULATE_INSTR;
    }
#  endif

    /*
     * Calc the error code for the guest trap.
     */
    uint32_t uNewErr = GST_IS_NX_ACTIVE(pVCpu)
                     ? uErr & (X86_TRAP_PF_RW | X86_TRAP_PF_US | X86_TRAP_PF_ID)
                     : uErr & (X86_TRAP_PF_RW | X86_TRAP_PF_US);
    if (   pGstWalk->Core.fRsvdError
        || pGstWalk->Core.fBadPhysAddr)
    {
        uNewErr |= X86_TRAP_PF_RSVD | X86_TRAP_PF_P;
        Assert(!pGstWalk->Core.fNotPresent);
    }
    else if (!pGstWalk->Core.fNotPresent)
        uNewErr |= X86_TRAP_PF_P;
    TRPMSetErrorCode(pVCpu, uNewErr);

    LogFlow(("Guest trap; cr2=%RGv uErr=%RGv lvl=%d\n", pGstWalk->Core.GCPtr, uErr, pGstWalk->Core.uLevel));
    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2GuestTrap; });
    return VINF_EM_RAW_GUEST_TRAP;
}
# endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */


/**
 * Deal with a guest page fault.
 *
 * The caller has taken the PGM lock.
 *
 * @returns Strict VBox status code.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   uErr            The error code.
 * @param   pRegFrame       The register frame.
 * @param   pvFault         The fault address.
 * @param   pPage           The guest page at @a pvFault.
 * @param   pGstWalk        The guest page table walk result.
 * @param   pfLockTaken     PGM lock taken here or not (out).  This is true
 *                          when we're called.
 */
static VBOXSTRICTRC PGM_BTH_NAME(Trap0eHandlerDoAccessHandlers)(PVMCPU pVCpu, RTGCUINT uErr, PCPUMCTXCORE pRegFrame,
                                                                RTGCPTR pvFault, PPGMPAGE pPage, bool *pfLockTaken
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) || defined(DOXYGEN_RUNNING)
                                                                , PGSTPTWALK pGstWalk
# endif
                                                                )
{
# if !PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    GSTPDE const    PdeSrcDummy = { X86_PDE_P | X86_PDE_US | X86_PDE_RW | X86_PDE_A };
# endif
    PVM             pVM         = pVCpu->CTX_SUFF(pVM);
    VBOXSTRICTRC    rcStrict;

    if (PGM_PAGE_HAS_ANY_PHYSICAL_HANDLERS(pPage))
    {
        /*
         * Physical page access handler.
         */
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        const RTGCPHYS  GCPhysFault = pGstWalk->Core.GCPhys;
# else
        const RTGCPHYS  GCPhysFault = PGM_A20_APPLY(pVCpu, (RTGCPHYS)pvFault);
# endif
        PPGMPHYSHANDLER pCur = pgmHandlerPhysicalLookup(pVM, GCPhysFault);
        if (pCur)
        {
            PPGMPHYSHANDLERTYPEINT pCurType = PGMPHYSHANDLER_GET_TYPE(pVM, pCur);

#  ifdef PGM_SYNC_N_PAGES
            /*
             * If the region is write protected and we got a page not present fault, then sync
             * the pages. If the fault was caused by a read, then restart the instruction.
             * In case of write access continue to the GC write handler.
             *
             * ASSUMES that there is only one handler per page or that they have similar write properties.
             */
            if (   !(uErr & X86_TRAP_PF_P)
                &&  pCurType->enmKind == PGMPHYSHANDLERKIND_WRITE)
            {
#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
                rcStrict = PGM_BTH_NAME(SyncPage)(pVCpu, pGstWalk->Pde, pvFault, PGM_SYNC_NR_PAGES, uErr);
#   else
                rcStrict = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrcDummy, pvFault, PGM_SYNC_NR_PAGES, uErr);
#   endif
                if (    RT_FAILURE(rcStrict)
                    || !(uErr & X86_TRAP_PF_RW)
                    || rcStrict == VINF_PGM_SYNCPAGE_MODIFIED_PDE)
                {
                    AssertMsgRC(rcStrict, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersOutOfSync);
                    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2OutOfSyncHndPhys; });
                    return rcStrict;
                }
            }
#  endif
#  ifdef PGM_WITH_MMIO_OPTIMIZATIONS
            /*
             * If the access was not thru a #PF(RSVD|...) resync the page.
             */
            if (   !(uErr & X86_TRAP_PF_RSVD)
                && pCurType->enmKind != PGMPHYSHANDLERKIND_WRITE
#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
                && pGstWalk->Core.fEffectiveRW
                && !pGstWalk->Core.fEffectiveUS /** @todo Remove pGstWalk->Core.fEffectiveUS and X86_PTE_US further down in the sync code. */
#   endif
               )
            {
#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
                rcStrict = PGM_BTH_NAME(SyncPage)(pVCpu, pGstWalk->Pde, pvFault, PGM_SYNC_NR_PAGES, uErr);
#   else
                rcStrict = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrcDummy, pvFault, PGM_SYNC_NR_PAGES, uErr);
#   endif
                if (    RT_FAILURE(rcStrict)
                    || rcStrict == VINF_PGM_SYNCPAGE_MODIFIED_PDE)
                {
                    AssertMsgRC(rcStrict, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersOutOfSync);
                    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2OutOfSyncHndPhys; });
                    return rcStrict;
                }
            }
#  endif

            AssertMsg(   pCurType->enmKind != PGMPHYSHANDLERKIND_WRITE
                      || (pCurType->enmKind == PGMPHYSHANDLERKIND_WRITE && (uErr & X86_TRAP_PF_RW)),
                      ("Unexpected trap for physical handler: %08X (phys=%08x) pPage=%R[pgmpage] uErr=%X, enmKind=%d\n",
                       pvFault, GCPhysFault, pPage, uErr, pCurType->enmKind));
            if (pCurType->enmKind == PGMPHYSHANDLERKIND_WRITE)
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersPhysWrite);
            else
            {
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersPhysAll);
                if (uErr & X86_TRAP_PF_RSVD) STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersPhysAllOpt);
            }

            if (pCurType->CTX_SUFF(pfnPfHandler))
            {
                PPGMPOOL    pPool  = pVM->pgm.s.CTX_SUFF(pPool);
                void       *pvUser = pCur->CTX_SUFF(pvUser);

                STAM_PROFILE_START(&pCur->Stat, h);
                if (pCur->hType != pPool->hAccessHandlerType)
                {
                    pgmUnlock(pVM);
                    *pfLockTaken = false;
                }

                rcStrict = pCurType->CTX_SUFF(pfnPfHandler)(pVM, pVCpu, uErr, pRegFrame, pvFault, GCPhysFault, pvUser);

#  ifdef VBOX_WITH_STATISTICS
                pgmLock(pVM);
                pCur = pgmHandlerPhysicalLookup(pVM, GCPhysFault);
                if (pCur)
                    STAM_PROFILE_STOP(&pCur->Stat, h);
                pgmUnlock(pVM);
#  endif
            }
            else
                rcStrict = VINF_EM_RAW_EMULATE_INSTR;

            STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2HndPhys; });
            return rcStrict;
        }
    }
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) && !defined(IN_RING0)
    else
    {
#  ifdef PGM_SYNC_N_PAGES
        /*
         * If the region is write protected and we got a page not present fault, then sync
         * the pages. If the fault was caused by a read, then restart the instruction.
         * In case of write access continue to the GC write handler.
         */
        if (    PGM_PAGE_GET_HNDL_VIRT_STATE(pPage) < PGM_PAGE_HNDL_PHYS_STATE_ALL
            && !(uErr & X86_TRAP_PF_P))
        {
            rcStrict = PGM_BTH_NAME(SyncPage)(pVCpu, pGstWalk->Pde, pvFault, PGM_SYNC_NR_PAGES, uErr);
            if (    RT_FAILURE(rcStrict)
                ||  rcStrict == VINF_PGM_SYNCPAGE_MODIFIED_PDE
                ||  !(uErr & X86_TRAP_PF_RW))
            {
                AssertRC(rcStrict);
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersOutOfSync);
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2OutOfSyncHndVirt; });
                return rcStrict;
            }
        }
#  endif
        /*
         * Ok, it's an virtual page access handler.
         *
         * Since it's faster to search by address, we'll do that first
         * and then retry by GCPhys if that fails.
         */
        /** @todo r=bird: perhaps we should consider looking up by physical address directly now?
         * r=svl: true, but lookup on virtual address should remain as a fallback as phys & virt trees might be
         *        out of sync, because the page was changed without us noticing it (not-present -> present
         *        without invlpg or mov cr3, xxx).
         */
        PPGMVIRTHANDLER pCur = (PPGMVIRTHANDLER)RTAvlroGCPtrRangeGet(&pVM->pgm.s.CTX_SUFF(pTrees)->VirtHandlers, pvFault);
        if (pCur)
        {
            PPGMVIRTHANDLERTYPEINT pCurType = PGMVIRTANDLER_GET_TYPE(pVM, pCur);
            AssertMsg(!(pvFault - pCur->Core.Key < pCur->cb)
                      || (     pCurType->enmKind != PGMVIRTHANDLERKIND_WRITE
                           || !(uErr & X86_TRAP_PF_P)
                           || (pCurType->enmKind == PGMVIRTHANDLERKIND_WRITE && (uErr & X86_TRAP_PF_RW))),
                      ("Unexpected trap for virtual handler: %RGv (phys=%RGp) pPage=%R[pgmpage] uErr=%X, enumKind=%d\n",
                       pvFault, pGstWalk->Core.GCPhys, pPage, uErr, pCurType->enmKind));

            if (    pvFault - pCur->Core.Key < pCur->cb
                &&  (    uErr & X86_TRAP_PF_RW
                     ||  pCurType->enmKind != PGMVIRTHANDLERKIND_WRITE ) )
            {
#   ifdef IN_RC
                STAM_PROFILE_START(&pCur->Stat, h);
                RTGCPTR GCPtrStart = pCur->Core.Key;
                void *pvUser = pCur->CTX_SUFF(pvUser);
                pgmUnlock(pVM);
                *pfLockTaken = false;

                rcStrict = pCurType->CTX_SUFF(pfnPfHandler)(pVM, pVCpu, uErr, pRegFrame, pvFault, GCPtrStart,
                                                            pvFault - GCPtrStart, pvUser);

#    ifdef VBOX_WITH_STATISTICS
                pgmLock(pVM);
                pCur = (PPGMVIRTHANDLER)RTAvlroGCPtrRangeGet(&pVM->pgm.s.CTX_SUFF(pTrees)->VirtHandlers, pvFault);
                if (pCur)
                    STAM_PROFILE_STOP(&pCur->Stat, h);
                pgmUnlock(pVM);
#    endif
#   else
                rcStrict = VINF_EM_RAW_EMULATE_INSTR; /** @todo for VMX */
#   endif
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersVirtual);
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2HndVirt; });
                return rcStrict;
            }
            /* Unhandled part of a monitored page */
            Log(("Unhandled part of monitored page %RGv\n", pvFault));
        }
        else
        {
           /* Check by physical address. */
            unsigned iPage;
            pCur = pgmHandlerVirtualFindByPhysAddr(pVM, pGstWalk->Core.GCPhys, &iPage);
            if (pCur)
            {
                PPGMVIRTHANDLERTYPEINT pCurType = PGMVIRTANDLER_GET_TYPE(pVM, pCur);
                if  (   uErr & X86_TRAP_PF_RW
                     || pCurType->enmKind != PGMVIRTHANDLERKIND_WRITE )
                {
                    Assert(   (pCur->aPhysToVirt[iPage].Core.Key & X86_PTE_PAE_PG_MASK)
                           == (pGstWalk->Core.GCPhys & X86_PTE_PAE_PG_MASK));
#   ifdef IN_RC
                    STAM_PROFILE_START(&pCur->Stat, h);
                    RTGCPTR GCPtrStart = pCur->Core.Key;
                    void *pvUser = pCur->CTX_SUFF(pvUser);
                    pgmUnlock(pVM);
                    *pfLockTaken = false;

                    RTGCPTR off = (iPage << PAGE_SHIFT)
                                + (pvFault    & PAGE_OFFSET_MASK)
                                - (GCPtrStart & PAGE_OFFSET_MASK);
                    Assert(off < pCur->cb);
                    rcStrict = pCurType->CTX_SUFF(pfnPfHandler)(pVM, pVCpu, uErr, pRegFrame, pvFault, GCPtrStart, off, pvUser);

#    ifdef VBOX_WITH_STATISTICS
                    pgmLock(pVM);
                    pCur = (PPGMVIRTHANDLER)RTAvlroGCPtrRangeGet(&pVM->pgm.s.CTX_SUFF(pTrees)->VirtHandlers, GCPtrStart);
                    if (pCur)
                        STAM_PROFILE_STOP(&pCur->Stat, h);
                    pgmUnlock(pVM);
#    endif
#   else
                    rcStrict = VINF_EM_RAW_EMULATE_INSTR; /** @todo for VMX */
#   endif
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersVirtualByPhys);
                    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2HndVirt; });
                    return rcStrict;
                }
            }
        }
    }
#  endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */

    /*
     * There is a handled area of the page, but this fault doesn't belong to it.
     * We must emulate the instruction.
     *
     * To avoid crashing (non-fatal) in the interpreter and go back to the recompiler
     * we first check if this was a page-not-present fault for a page with only
     * write access handlers. Restart the instruction if it wasn't a write access.
     */
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersUnhandled);

    if (    !PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage)
        &&  !(uErr & X86_TRAP_PF_P))
    {
#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        rcStrict = PGM_BTH_NAME(SyncPage)(pVCpu, pGstWalk->Pde, pvFault, PGM_SYNC_NR_PAGES, uErr);
#  else
        rcStrict = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrcDummy, pvFault, PGM_SYNC_NR_PAGES, uErr);
#  endif
        if (    RT_FAILURE(rcStrict)
            ||  rcStrict == VINF_PGM_SYNCPAGE_MODIFIED_PDE
            ||  !(uErr & X86_TRAP_PF_RW))
        {
            AssertMsgRC(rcStrict, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersOutOfSync);
            STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2OutOfSyncHndPhys; });
            return rcStrict;
        }
    }

    /** @todo This particular case can cause quite a lot of overhead. E.g. early stage of kernel booting in Ubuntu 6.06
     *        It's writing to an unhandled part of the LDT page several million times.
     */
    rcStrict = PGMInterpretInstruction(pVM, pVCpu, pRegFrame, pvFault);
    LogFlow(("PGM: PGMInterpretInstruction -> rcStrict=%d pPage=%R[pgmpage]\n", VBOXSTRICTRC_VAL(rcStrict), pPage));
    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2HndUnhandled; });
    return rcStrict;
} /* if any kind of handler */


/**
 * \#PF Handler for raw-mode guest execution.
 *
 * @returns VBox status code (appropriate for trap handling and GC return).
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   uErr        The trap error code.
 * @param   pRegFrame   Trap register frame.
 * @param   pvFault     The fault address.
 * @param   pfLockTaken PGM lock taken here or not (out)
 */
PGM_BTH_DECL(int, Trap0eHandler)(PVMCPU pVCpu, RTGCUINT uErr, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, bool *pfLockTaken)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM); NOREF(pVM);

    *pfLockTaken = false;

# if  (   PGM_GST_TYPE == PGM_TYPE_32BIT || PGM_GST_TYPE == PGM_TYPE_REAL || PGM_GST_TYPE == PGM_TYPE_PROT \
       || PGM_GST_TYPE == PGM_TYPE_PAE   || PGM_GST_TYPE == PGM_TYPE_AMD64) \
    && PGM_SHW_TYPE != PGM_TYPE_NESTED \
    && (PGM_SHW_TYPE != PGM_TYPE_EPT || PGM_GST_TYPE == PGM_TYPE_PROT)
    int rc;

#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    /*
     * Walk the guest page translation tables and check if it's a guest fault.
     */
    GSTPTWALK GstWalk;
    rc = PGM_GST_NAME(Walk)(pVCpu, pvFault, &GstWalk);
    if (RT_FAILURE_NP(rc))
        return VBOXSTRICTRC_TODO(PGM_BTH_NAME(Trap0eHandlerGuestFault)(pVCpu, &GstWalk, uErr));

    /* assert some GstWalk sanity. */
#   if PGM_GST_TYPE == PGM_TYPE_AMD64
    /*AssertMsg(GstWalk.Pml4e.u == GstWalk.pPml4e->u, ("%RX64 %RX64\n", (uint64_t)GstWalk.Pml4e.u, (uint64_t)GstWalk.pPml4e->u));  - not always true with SMP guests. */
#   endif
#   if PGM_GST_TYPE == PGM_TYPE_AMD64 || PGM_GST_TYPE == PGM_TYPE_PAE
    /*AssertMsg(GstWalk.Pdpe.u == GstWalk.pPdpe->u, ("%RX64 %RX64\n", (uint64_t)GstWalk.Pdpe.u, (uint64_t)GstWalk.pPdpe->u)); - ditto */
#   endif
    /*AssertMsg(GstWalk.Pde.u == GstWalk.pPde->u, ("%RX64 %RX64\n", (uint64_t)GstWalk.Pde.u, (uint64_t)GstWalk.pPde->u)); - ditto */
    /*AssertMsg(GstWalk.Core.fBigPage || GstWalk.Pte.u == GstWalk.pPte->u, ("%RX64 %RX64\n", (uint64_t)GstWalk.Pte.u, (uint64_t)GstWalk.pPte->u)); - ditto */
    Assert(GstWalk.Core.fSucceeded);

    if (uErr & (X86_TRAP_PF_RW | X86_TRAP_PF_US | X86_TRAP_PF_ID))
    {
        if (    (   (uErr & X86_TRAP_PF_RW)
                 && !GstWalk.Core.fEffectiveRW
                 && (   (uErr & X86_TRAP_PF_US)
                     || CPUMIsGuestR0WriteProtEnabled(pVCpu)) )
            ||  ((uErr & X86_TRAP_PF_US) && !GstWalk.Core.fEffectiveUS)
            ||  ((uErr & X86_TRAP_PF_ID) && GstWalk.Core.fEffectiveNX)
           )
            return VBOXSTRICTRC_TODO(PGM_BTH_NAME(Trap0eHandlerGuestFault)(pVCpu, &GstWalk, uErr));
    }

    /*
     * Set the accessed and dirty flags.
     */
#   if PGM_GST_TYPE == PGM_TYPE_AMD64
    GstWalk.Pml4e.u     |= X86_PML4E_A;
    GstWalk.pPml4e->u   |= X86_PML4E_A;
    GstWalk.Pdpe.u      |= X86_PDPE_A;
    GstWalk.pPdpe->u    |= X86_PDPE_A;
#   endif
    if (GstWalk.Core.fBigPage)
    {
        Assert(GstWalk.Pde.b.u1Size);
        if (uErr & X86_TRAP_PF_RW)
        {
            GstWalk.Pde.u   |= X86_PDE4M_A | X86_PDE4M_D;
            GstWalk.pPde->u |= X86_PDE4M_A | X86_PDE4M_D;
        }
        else
        {
            GstWalk.Pde.u   |= X86_PDE4M_A;
            GstWalk.pPde->u |= X86_PDE4M_A;
        }
    }
    else
    {
        Assert(!GstWalk.Pde.b.u1Size);
        GstWalk.Pde.u   |= X86_PDE_A;
        GstWalk.pPde->u |= X86_PDE_A;
        if (uErr & X86_TRAP_PF_RW)
        {
#   ifdef VBOX_WITH_STATISTICS
            if (!GstWalk.Pte.n.u1Dirty)
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtiedPage));
            else
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageAlreadyDirty));
#   endif
            GstWalk.Pte.u   |= X86_PTE_A | X86_PTE_D;
            GstWalk.pPte->u |= X86_PTE_A | X86_PTE_D;
        }
        else
        {
            GstWalk.Pte.u   |= X86_PTE_A;
            GstWalk.pPte->u |= X86_PTE_A;
        }
        Assert(GstWalk.Pte.u == GstWalk.pPte->u);
    }
    AssertMsg(GstWalk.Pde.u == GstWalk.pPde->u || GstWalk.pPte->u == GstWalk.pPde->u,
              ("%RX64 %RX64 pPte=%p pPde=%p Pte=%RX64\n", (uint64_t)GstWalk.Pde.u, (uint64_t)GstWalk.pPde->u, GstWalk.pPte, GstWalk.pPde, (uint64_t)GstWalk.pPte->u));
#  else  /* !PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */
    GSTPDE const PdeSrcDummy = { X86_PDE_P | X86_PDE_US | X86_PDE_RW | X86_PDE_A}; /** @todo eliminate this */
#  endif /* !PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */

    /* Take the big lock now. */
    *pfLockTaken = true;
    pgmLock(pVM);

#  ifdef PGM_WITH_MMIO_OPTIMIZATIONS
    /*
     * If it is a reserved bit fault we know that it is an MMIO (access
     * handler) related fault and can skip some 200 lines of code.
     */
    if (uErr & X86_TRAP_PF_RSVD)
    {
        Assert(uErr & X86_TRAP_PF_P);
        PPGMPAGE pPage;
#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        rc = pgmPhysGetPageEx(pVM, GstWalk.Core.GCPhys, &pPage);
        if (RT_SUCCESS(rc) && PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage))
            return VBOXSTRICTRC_TODO(PGM_BTH_NAME(Trap0eHandlerDoAccessHandlers)(pVCpu, uErr, pRegFrame, pvFault, pPage,
                                                                                 pfLockTaken, &GstWalk));
        rc = PGM_BTH_NAME(SyncPage)(pVCpu, GstWalk.Pde, pvFault, 1, uErr);
#   else
        rc = pgmPhysGetPageEx(pVM, PGM_A20_APPLY(pVCpu, (RTGCPHYS)pvFault), &pPage);
        if (RT_SUCCESS(rc) && PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage))
            return VBOXSTRICTRC_TODO(PGM_BTH_NAME(Trap0eHandlerDoAccessHandlers)(pVCpu, uErr, pRegFrame, pvFault, pPage,
                                                                                 pfLockTaken));
        rc = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrcDummy, pvFault, 1, uErr);
#   endif
        AssertRC(rc);
        PGM_INVL_PG(pVCpu, pvFault);
        return rc; /* Restart with the corrected entry. */
    }
#  endif /* PGM_WITH_MMIO_OPTIMIZATIONS */

    /*
     * Fetch the guest PDE, PDPE and PML4E.
     */
#  if PGM_SHW_TYPE == PGM_TYPE_32BIT
    const unsigned  iPDDst = pvFault >> SHW_PD_SHIFT;
    PX86PD          pPDDst = pgmShwGet32BitPDPtr(pVCpu);

#  elif PGM_SHW_TYPE == PGM_TYPE_PAE
    const unsigned  iPDDst = (pvFault >> SHW_PD_SHIFT) & SHW_PD_MASK;   /* pPDDst index, not used with the pool. */
    PX86PDPAE       pPDDst;
#   if PGM_GST_TYPE == PGM_TYPE_PAE
    rc = pgmShwSyncPaePDPtr(pVCpu, pvFault, GstWalk.Pdpe.u, &pPDDst);
#   else
    rc = pgmShwSyncPaePDPtr(pVCpu, pvFault, X86_PDPE_P, &pPDDst);       /* RW, US and A are reserved in PAE mode. */
#   endif
    AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS);

#  elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    const unsigned  iPDDst = ((pvFault >> SHW_PD_SHIFT) & SHW_PD_MASK);
    PX86PDPAE       pPDDst;
#   if PGM_GST_TYPE == PGM_TYPE_PROT  /* (AMD-V nested paging) */
    rc = pgmShwSyncLongModePDPtr(pVCpu, pvFault, X86_PML4E_P | X86_PML4E_RW | X86_PML4E_US | X86_PML4E_A,
                                 X86_PDPE_P | X86_PDPE_RW | X86_PDPE_US | X86_PDPE_A, &pPDDst);
#   else
    rc = pgmShwSyncLongModePDPtr(pVCpu, pvFault, GstWalk.Pml4e.u, GstWalk.Pdpe.u, &pPDDst);
#   endif
    AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS);

#  elif PGM_SHW_TYPE == PGM_TYPE_EPT
    const unsigned  iPDDst = ((pvFault >> SHW_PD_SHIFT) & SHW_PD_MASK);
    PEPTPD          pPDDst;
    rc = pgmShwGetEPTPDPtr(pVCpu, pvFault, NULL, &pPDDst);
    AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS);
#  endif
    Assert(pPDDst);

#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    /*
     * Dirty page handling.
     *
     * If we successfully correct the write protection fault due to dirty bit
     * tracking, then return immediately.
     */
    if (uErr & X86_TRAP_PF_RW)  /* write fault? */
    {
        STAM_PROFILE_START(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyBitTracking), a);
        rc = PGM_BTH_NAME(CheckDirtyPageFault)(pVCpu, uErr, &pPDDst->a[iPDDst], GstWalk.pPde, pvFault);
        STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyBitTracking), a);
        if (rc == VINF_PGM_HANDLED_DIRTY_BIT_FAULT)
        {
            STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution)
                        = rc == VINF_PGM_HANDLED_DIRTY_BIT_FAULT
                          ? &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2DirtyAndAccessed
                          : &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2GuestTrap; });
            Log8(("Trap0eHandler: returns VINF_SUCCESS\n"));
            return VINF_SUCCESS;
        }
#ifdef DEBUG_bird
        AssertMsg(GstWalk.Pde.u == GstWalk.pPde->u || GstWalk.pPte->u == GstWalk.pPde->u || pVM->cCpus > 1, ("%RX64 %RX64\n", (uint64_t)GstWalk.Pde.u, (uint64_t)GstWalk.pPde->u)); // - triggers with smp w7 guests.
        AssertMsg(GstWalk.Core.fBigPage || GstWalk.Pte.u == GstWalk.pPte->u || pVM->cCpus > 1, ("%RX64 %RX64\n", (uint64_t)GstWalk.Pte.u, (uint64_t)GstWalk.pPte->u)); // - ditto.
#endif
    }

#   if 0 /* rarely useful; leave for debugging. */
    STAM_COUNTER_INC(&pVCpu->pgm.s.StatRZTrap0ePD[iPDSrc]);
#   endif
#  endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */

    /*
     * A common case is the not-present error caused by lazy page table syncing.
     *
     * It is IMPORTANT that we weed out any access to non-present shadow PDEs
     * here so we can safely assume that the shadow PT is present when calling
     * SyncPage later.
     *
     * On failure, we ASSUME that SyncPT is out of memory or detected some kind
     * of mapping conflict and defer to SyncCR3 in R3.
     * (Again, we do NOT support access handlers for non-present guest pages.)
     *
     */
#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    Assert(GstWalk.Pde.n.u1Present);
#  endif
    if (    !(uErr & X86_TRAP_PF_P) /* not set means page not present instead of page protection violation */
        &&  !pPDDst->a[iPDDst].n.u1Present)
    {
        STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2SyncPT; });
#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        LogFlow(("=>SyncPT %04x = %08RX64\n", (pvFault >> GST_PD_SHIFT) & GST_PD_MASK, (uint64_t)GstWalk.Pde.u));
        rc = PGM_BTH_NAME(SyncPT)(pVCpu, (pvFault >> GST_PD_SHIFT) & GST_PD_MASK, GstWalk.pPd, pvFault);
#  else
        LogFlow(("=>SyncPT pvFault=%RGv\n", pvFault));
        rc = PGM_BTH_NAME(SyncPT)(pVCpu, 0, NULL, pvFault);
#  endif
        if (RT_SUCCESS(rc))
            return rc;
        Log(("SyncPT: %RGv failed!! rc=%Rrc\n", pvFault, rc));
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3); /** @todo no need to do global sync, right? */
        return VINF_PGM_SYNC_CR3;
    }

#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) && !defined(PGM_WITHOUT_MAPPINGS)
    /*
     * Check if this address is within any of our mappings.
     *
     * This is *very* fast and it's gonna save us a bit of effort below and prevent
     * us from screwing ourself with MMIO2 pages which have a GC Mapping (VRam).
     * (BTW, it's impossible to have physical access handlers in a mapping.)
     */
    if (pgmMapAreMappingsEnabled(pVM))
    {
        PPGMMAPPING pMapping = pVM->pgm.s.CTX_SUFF(pMappings);
        for ( ; pMapping; pMapping = pMapping->CTX_SUFF(pNext))
        {
            if (pvFault < pMapping->GCPtr)
                break;
            if (pvFault - pMapping->GCPtr < pMapping->cb)
            {
                /*
                 * The first thing we check is if we've got an undetected conflict.
                 */
                if (pgmMapAreMappingsFloating(pVM))
                {
                    unsigned iPT = pMapping->cb >> GST_PD_SHIFT;
                    while (iPT-- > 0)
                        if (GstWalk.pPde[iPT].n.u1Present)
                        {
                            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eConflicts);
                            Log(("Trap0e: Detected Conflict %RGv-%RGv\n", pMapping->GCPtr, pMapping->GCPtrLast));
                            VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3); /** @todo no need to do global sync,right? */
                            STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2Mapping; });
                            return VINF_PGM_SYNC_CR3;
                        }
                }

                /*
                 * Check if the fault address is in a virtual page access handler range.
                 */
                PPGMVIRTHANDLER pCur = (PPGMVIRTHANDLER)RTAvlroGCPtrRangeGet(&pVM->pgm.s.CTX_SUFF(pTrees)->HyperVirtHandlers,
                                                                             pvFault);
                if (    pCur
                    &&  pvFault - pCur->Core.Key < pCur->cb
                    &&  uErr & X86_TRAP_PF_RW)
                {
                    VBOXSTRICTRC rcStrict;
#   ifdef IN_RC
                    STAM_PROFILE_START(&pCur->Stat, h);
                    PPGMVIRTHANDLERTYPEINT pCurType = PGMVIRTANDLER_GET_TYPE(pVM, pCur);
                    void *pvUser = pCur->CTX_SUFF(pvUser);
                    pgmUnlock(pVM);
                    rcStrict = pCurType->CTX_SUFF(pfnPfHandler)(pVM, pVCpu, uErr, pRegFrame, pvFault, pCur->Core.Key,
                                                                pvFault - pCur->Core.Key, pvUser);
                    pgmLock(pVM);
                    STAM_PROFILE_STOP(&pCur->Stat, h);
#   else
                    AssertFailed();
                    rcStrict = VINF_EM_RAW_EMULATE_INSTR; /* can't happen with VMX */
#   endif
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersMapping);
                    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2Mapping; });
                    return VBOXSTRICTRC_TODO(rcStrict);
                }

                /*
                 * Pretend we're not here and let the guest handle the trap.
                 */
                TRPMSetErrorCode(pVCpu, uErr & ~X86_TRAP_PF_P);
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eGuestPFMapping);
                LogFlow(("PGM: Mapping access -> route trap to recompiler!\n"));
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2Mapping; });
                return VINF_EM_RAW_GUEST_TRAP;
            }
        }
    } /* pgmAreMappingsEnabled(&pVM->pgm.s) */
#  endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */

    /*
     * Check if this fault address is flagged for special treatment,
     * which means we'll have to figure out the physical address and
     * check flags associated with it.
     *
     * ASSUME that we can limit any special access handling to pages
     * in page tables which the guest believes to be present.
     */
#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    RTGCPHYS GCPhys = GstWalk.Core.GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK;
#  else
    RTGCPHYS GCPhys = PGM_A20_APPLY(pVCpu, (RTGCPHYS)pvFault & ~(RTGCPHYS)PAGE_OFFSET_MASK);
#  endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */
    PPGMPAGE pPage;
    rc = pgmPhysGetPageEx(pVM, GCPhys, &pPage);
    if (RT_FAILURE(rc))
    {
        /*
         * When the guest accesses invalid physical memory (e.g. probing
         * of RAM or accessing a remapped MMIO range), then we'll fall
         * back to the recompiler to emulate the instruction.
         */
        LogFlow(("PGM #PF: pgmPhysGetPageEx(%RGp) failed with %Rrc\n", GCPhys, rc));
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eHandlersInvalid);
        STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2InvalidPhys; });
        return VINF_EM_RAW_EMULATE_INSTR;
    }

    /*
     * Any handlers for this page?
     */
    if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        return VBOXSTRICTRC_TODO(PGM_BTH_NAME(Trap0eHandlerDoAccessHandlers)(pVCpu, uErr, pRegFrame, pvFault, pPage, pfLockTaken,
                                                                             &GstWalk));
# else
        return VBOXSTRICTRC_TODO(PGM_BTH_NAME(Trap0eHandlerDoAccessHandlers)(pVCpu, uErr, pRegFrame, pvFault, pPage, pfLockTaken));
# endif

#  if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) && !defined(IN_RING0)
    if (uErr & X86_TRAP_PF_P)
    {
        /*
         * The page isn't marked, but it might still be monitored by a virtual page access handler.
         * (ASSUMES no temporary disabling of virtual handlers.)
         */
        /** @todo r=bird: Since the purpose is to catch out of sync pages with virtual handler(s) here,
         * we should correct both the shadow page table and physical memory flags, and not only check for
         * accesses within the handler region but for access to pages with virtual handlers. */
        PPGMVIRTHANDLER pCur = (PPGMVIRTHANDLER)RTAvlroGCPtrRangeGet(&pVM->pgm.s.CTX_SUFF(pTrees)->VirtHandlers, pvFault);
        if (pCur)
        {
            PPGMVIRTHANDLERTYPEINT pCurType = PGMVIRTANDLER_GET_TYPE(pVM, pCur);
            AssertMsg(   !(pvFault - pCur->Core.Key < pCur->cb)
                      || (    pCurType->enmKind != PGMVIRTHANDLERKIND_WRITE
                           || !(uErr & X86_TRAP_PF_P)
                           || (pCurType->enmKind == PGMVIRTHANDLERKIND_WRITE && (uErr & X86_TRAP_PF_RW))),
                      ("Unexpected trap for virtual handler: %08X (phys=%08x) %R[pgmpage] uErr=%X, enumKind=%d\n",
                       pvFault, GCPhys, pPage, uErr, pCurType->enmKind));

            if (    pvFault - pCur->Core.Key < pCur->cb
                &&  (    uErr & X86_TRAP_PF_RW
                     ||  pCurType->enmKind != PGMVIRTHANDLERKIND_WRITE ) )
            {
                VBOXSTRICTRC rcStrict;
#   ifdef IN_RC
                STAM_PROFILE_START(&pCur->Stat, h);
                void *pvUser = pCur->CTX_SUFF(pvUser);
                pgmUnlock(pVM);
                rcStrict = pCurType->CTX_SUFF(pfnPfHandler)(pVM, pVCpu, uErr, pRegFrame, pvFault, pCur->Core.Key,
                                                            pvFault - pCur->Core.Key, pvUser);
                pgmLock(pVM);
                STAM_PROFILE_STOP(&pCur->Stat, h);
#   else
                rcStrict = VINF_EM_RAW_EMULATE_INSTR; /** @todo for VMX */
#   endif
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2HndVirt; });
                return VBOXSTRICTRC_TODO(rcStrict);
            }
        }
    }
#  endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */

    /*
     * We are here only if page is present in Guest page tables and
     * trap is not handled by our handlers.
     *
     * Check it for page out-of-sync situation.
     */
    if (!(uErr & X86_TRAP_PF_P))
    {
        /*
         * Page is not present in our page tables. Try to sync it!
         */
        if (uErr & X86_TRAP_PF_US)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageOutOfSyncUser));
        else /* supervisor */
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageOutOfSyncSupervisor));

        if (PGM_PAGE_IS_BALLOONED(pPage))
        {
            /* Emulate reads from ballooned pages as they are not present in
               our shadow page tables. (Required for e.g. Solaris guests; soft
               ecc, random nr generator.) */
            rc = VBOXSTRICTRC_TODO(PGMInterpretInstruction(pVM, pVCpu, pRegFrame, pvFault));
            LogFlow(("PGM: PGMInterpretInstruction balloon -> rc=%d pPage=%R[pgmpage]\n", rc, pPage));
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageOutOfSyncBallloon));
            STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2Ballooned; });
            return rc;
        }

#   if defined(LOG_ENABLED) && !defined(IN_RING0)
        RTGCPHYS   GCPhys2;
        uint64_t   fPageGst2;
        PGMGstGetPage(pVCpu, pvFault, &fPageGst2, &GCPhys2);
#    if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        Log(("Page out of sync: %RGv eip=%08x PdeSrc.US=%d fPageGst2=%08llx GCPhys2=%RGp scan=%d\n",
             pvFault, pRegFrame->eip, GstWalk.Pde.n.u1User, fPageGst2, GCPhys2, CSAMDoesPageNeedScanning(pVM, pRegFrame->eip)));
#    else
        Log(("Page out of sync: %RGv eip=%08x fPageGst2=%08llx GCPhys2=%RGp scan=%d\n",
             pvFault, pRegFrame->eip, fPageGst2, GCPhys2, CSAMDoesPageNeedScanning(pVM, pRegFrame->eip)));
#    endif
#   endif /* LOG_ENABLED */

#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) && !defined(IN_RING0)
        if (   !GstWalk.Core.fEffectiveUS
            && CSAMIsEnabled(pVM)
            && CPUMGetGuestCPL(pVCpu) == 0)
        {
            /* Note: Can't check for X86_TRAP_ID bit, because that requires execute disable support on the CPU. */
            if (    pvFault == (RTGCPTR)pRegFrame->eip
                ||  pvFault - pRegFrame->eip < 8    /* instruction crossing a page boundary */
#    ifdef CSAM_DETECT_NEW_CODE_PAGES
                ||  (   !PATMIsPatchGCAddr(pVM, pRegFrame->eip)
                     && CSAMDoesPageNeedScanning(pVM, pRegFrame->eip))   /* any new code we encounter here */
#    endif /* CSAM_DETECT_NEW_CODE_PAGES */
               )
            {
                LogFlow(("CSAMExecFault %RX32\n", pRegFrame->eip));
                rc = CSAMExecFault(pVM, (RTRCPTR)pRegFrame->eip);
                if (rc != VINF_SUCCESS)
                {
                    /*
                     * CSAM needs to perform a job in ring 3.
                     *
                     * Sync the page before going to the host context; otherwise we'll end up in a loop if
                     * CSAM fails (e.g. instruction crosses a page boundary and the next page is not present)
                     */
                    LogFlow(("CSAM ring 3 job\n"));
                    int rc2 = PGM_BTH_NAME(SyncPage)(pVCpu, GstWalk.Pde, pvFault, 1, uErr);
                    AssertRC(rc2);

                    STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2CSAM; });
                    return rc;
                }
            }
#    ifdef CSAM_DETECT_NEW_CODE_PAGES
            else if (    uErr == X86_TRAP_PF_RW
                     &&  pRegFrame->ecx >= 0x100         /* early check for movswd count */
                     &&  pRegFrame->ecx < 0x10000)
            {
                /* In case of a write to a non-present supervisor shadow page, we'll take special precautions
                 * to detect loading of new code pages.
                 */

                /*
                 * Decode the instruction.
                 */
                PDISCPUSTATE pDis = &pVCpu->pgm.s.DisState;
                uint32_t     cbOp;
                rc = EMInterpretDisasCurrent(pVM, pVCpu, pDis, &cbOp);

                /* For now we'll restrict this to rep movsw/d instructions */
                if (    rc == VINF_SUCCESS
                    &&  pDis->pCurInstr->opcode == OP_MOVSWD
                    &&  (pDis->prefix & DISPREFIX_REP))
                {
                    CSAMMarkPossibleCodePage(pVM, pvFault);
                }
            }
#    endif  /* CSAM_DETECT_NEW_CODE_PAGES */

            /*
             * Mark this page as safe.
             */
            /** @todo not correct for pages that contain both code and data!! */
            Log2(("CSAMMarkPage %RGv; scanned=%d\n", pvFault, true));
            CSAMMarkPage(pVM, pvFault, true);
        }
#   endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) && !defined(IN_RING0) */
#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        rc = PGM_BTH_NAME(SyncPage)(pVCpu, GstWalk.Pde, pvFault, PGM_SYNC_NR_PAGES, uErr);
#   else
        rc = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrcDummy, pvFault, PGM_SYNC_NR_PAGES, uErr);
#   endif
        if (RT_SUCCESS(rc))
        {
            /* The page was successfully synced, return to the guest. */
            STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2OutOfSync; });
            return VINF_SUCCESS;
        }
    }
    else /* uErr & X86_TRAP_PF_P: */
    {
        /*
         * Write protected pages are made writable when the guest makes the
         * first write to it.  This happens for pages that are shared, write
         * monitored or not yet allocated.
         *
         * We may also end up here when CR0.WP=0 in the guest.
         *
         * Also, a side effect of not flushing global PDEs are out of sync
         * pages due to physical monitored regions, that are no longer valid.
         * Assume for now it only applies to the read/write flag.
         */
        if (uErr & X86_TRAP_PF_RW)
        {
            /*
             * Check if it is a read-only page.
             */
            if (PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED)
            {
                Log(("PGM #PF: Make writable: %RGp %R[pgmpage] pvFault=%RGp uErr=%#x\n", GCPhys, pPage, pvFault, uErr));
                Assert(!PGM_PAGE_IS_ZERO(pPage));
                AssertFatalMsg(!PGM_PAGE_IS_BALLOONED(pPage), ("Unexpected ballooned page at %RGp\n", GCPhys));
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2MakeWritable; });

                rc = pgmPhysPageMakeWritable(pVM, pPage, GCPhys);
                if (rc != VINF_SUCCESS)
                {
                    AssertMsg(rc == VINF_PGM_SYNC_CR3 || RT_FAILURE(rc), ("%Rrc\n", rc));
                    return rc;
                }
                if (RT_UNLIKELY(VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY)))
                    return VINF_EM_NO_MEMORY;
            }

#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
            /*
             * Check to see if we need to emulate the instruction if CR0.WP=0.
             */
            if (    !GstWalk.Core.fEffectiveRW
                &&  (CPUMGetGuestCR0(pVCpu) & (X86_CR0_WP | X86_CR0_PG)) == X86_CR0_PG
                &&  CPUMGetGuestCPL(pVCpu) < 3)
            {
                Assert((uErr & (X86_TRAP_PF_RW | X86_TRAP_PF_P)) == (X86_TRAP_PF_RW | X86_TRAP_PF_P));

                /*
                 * The Netware WP0+RO+US hack.
                 *
                 * Netware sometimes(/always?) runs with WP0.  It has been observed doing
                 * excessive write accesses to pages which are mapped with US=1 and RW=0
                 * while WP=0.  This causes a lot of exits and extremely slow execution.
                 * To avoid trapping and emulating every write here, we change the shadow
                 * page table entry to map it as US=0 and RW=1 until user mode tries to
                 * access it again (see further below).  We count these shadow page table
                 * changes so we can avoid having to clear the page pool every time the WP
                 * bit changes to 1 (see PGMCr0WpEnabled()).
                 */
#    if (PGM_GST_TYPE == PGM_TYPE_32BIT || PGM_GST_TYPE == PGM_TYPE_PAE) && 1
                if (   GstWalk.Core.fEffectiveUS
                    && !GstWalk.Core.fEffectiveRW
                    && (GstWalk.Core.fBigPage || GstWalk.Pde.n.u1Write)
                    && pVM->cCpus == 1 /* Sorry, no go on SMP. Add CFGM option? */)
                {
                    Log(("PGM #PF: Netware WP0+RO+US hack: pvFault=%RGp uErr=%#x (big=%d)\n", pvFault, uErr, GstWalk.Core.fBigPage));
                    rc = pgmShwMakePageSupervisorAndWritable(pVCpu, pvFault, GstWalk.Core.fBigPage, PGM_MK_PG_IS_WRITE_FAULT);
                    if (rc == VINF_SUCCESS || rc == VINF_PGM_SYNC_CR3)
                    {
                        PGM_INVL_PG(pVCpu, pvFault);
                        pVCpu->pgm.s.cNetwareWp0Hacks++;
                        STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2Wp0RoUsHack; });
                        return rc;
                    }
                    AssertMsg(RT_FAILURE_NP(rc), ("%Rrc\n", rc));
                    Log(("pgmShwMakePageSupervisorAndWritable(%RGv) failed with rc=%Rrc - ignored\n", pvFault, rc));
                }
#    endif

                /* Interpret the access. */
                rc = VBOXSTRICTRC_TODO(PGMInterpretInstruction(pVM, pVCpu, pRegFrame, pvFault));
                Log(("PGM #PF: WP0 emulation (pvFault=%RGp uErr=%#x cpl=%d fBig=%d fEffUs=%d)\n", pvFault, uErr, CPUMGetGuestCPL(pVCpu), GstWalk.Core.fBigPage, GstWalk.Core.fEffectiveUS));
                if (RT_SUCCESS(rc))
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eWPEmulInRZ);
                else
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eWPEmulToR3);
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2WPEmulation; });
                return rc;
            }
#   endif
            /// @todo count the above case; else
            if (uErr & X86_TRAP_PF_US)
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageOutOfSyncUserWrite));
            else /* supervisor */
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageOutOfSyncSupervisorWrite));

            /*
             * Sync the page.
             *
             * Note: Do NOT use PGM_SYNC_NR_PAGES here. That only works if the
             *       page is not present, which is not true in this case.
             */
#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
            rc = PGM_BTH_NAME(SyncPage)(pVCpu, GstWalk.Pde, pvFault, 1, uErr);
#   else
            rc = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrcDummy, pvFault, 1, uErr);
#   endif
            if (RT_SUCCESS(rc))
            {
               /*
                * Page was successfully synced, return to guest but invalidate
                * the TLB first as the page is very likely to be in it.
                */
#   if PGM_SHW_TYPE == PGM_TYPE_EPT
                HMInvalidatePhysPage(pVM, (RTGCPHYS)pvFault);
#   else
                PGM_INVL_PG(pVCpu, pvFault);
#   endif
#   ifdef VBOX_STRICT
                RTGCPHYS GCPhys2 = RTGCPHYS_MAX;
                uint64_t fPageGst = UINT64_MAX;
                if (!pVM->pgm.s.fNestedPaging)
                {
                    rc = PGMGstGetPage(pVCpu, pvFault, &fPageGst, &GCPhys2);
                    AssertMsg(RT_SUCCESS(rc) && ((fPageGst & X86_PTE_RW) || ((CPUMGetGuestCR0(pVCpu) & (X86_CR0_WP | X86_CR0_PG)) == X86_CR0_PG && CPUMGetGuestCPL(pVCpu) < 3)), ("rc=%Rrc fPageGst=%RX64\n", rc, fPageGst));
                    LogFlow(("Obsolete physical monitor page out of sync %RGv - phys %RGp flags=%08llx\n", pvFault, GCPhys2, (uint64_t)fPageGst));
                }
#    if 0 /* Bogus! Triggers incorrectly with w7-64 and later for the SyncPage case: "Pde at %RGv changed behind our back?" */
                uint64_t fPageShw = 0;
                rc = PGMShwGetPage(pVCpu, pvFault, &fPageShw, NULL);
                AssertMsg((RT_SUCCESS(rc) && (fPageShw & X86_PTE_RW)) || pVM->cCpus > 1 /* new monitor can be installed/page table flushed between the trap exit and PGMTrap0eHandler */,
                          ("rc=%Rrc fPageShw=%RX64 GCPhys2=%RGp fPageGst=%RX64 pvFault=%RGv\n", rc, fPageShw, GCPhys2, fPageGst, pvFault));
#    endif
#   endif /* VBOX_STRICT */
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2OutOfSyncHndObs; });
                return VINF_SUCCESS;
            }
        }
#    if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        /*
         * Check for Netware WP0+RO+US hack from above and undo it when user
         * mode accesses the page again.
         */
        else if (    GstWalk.Core.fEffectiveUS
                 && !GstWalk.Core.fEffectiveRW
                 && (GstWalk.Core.fBigPage || GstWalk.Pde.n.u1Write)
                 &&  pVCpu->pgm.s.cNetwareWp0Hacks > 0
                 &&  (CPUMGetGuestCR0(pVCpu) & (X86_CR0_WP | X86_CR0_PG)) == X86_CR0_PG
                 &&  CPUMGetGuestCPL(pVCpu) == 3
                 &&  pVM->cCpus == 1
                )
        {
            Log(("PGM #PF: Undo netware WP0+RO+US hack: pvFault=%RGp uErr=%#x\n", pvFault, uErr));
            rc = PGM_BTH_NAME(SyncPage)(pVCpu, GstWalk.Pde, pvFault, 1, uErr);
            if (RT_SUCCESS(rc))
            {
                PGM_INVL_PG(pVCpu, pvFault);
                pVCpu->pgm.s.cNetwareWp0Hacks--;
                STAM_STATS({ pVCpu->pgm.s.CTX_SUFF(pStatTrap0eAttribution) = &pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZTrap0eTime2Wp0RoUsUnhack; });
                return VINF_SUCCESS;
            }
        }
#    endif /* PGM_WITH_PAGING */

        /** @todo else: why are we here? */

#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) && defined(VBOX_STRICT)
        /*
         * Check for VMM page flags vs. Guest page flags consistency.
         * Currently only for debug purposes.
         */
        if (RT_SUCCESS(rc))
        {
            /* Get guest page flags. */
            uint64_t fPageGst;
            int rc2 = PGMGstGetPage(pVCpu, pvFault, &fPageGst, NULL);
            if (RT_SUCCESS(rc2))
            {
                uint64_t fPageShw = 0;
                rc2 = PGMShwGetPage(pVCpu, pvFault, &fPageShw, NULL);

#if 0
                /*
                 * Compare page flags.
                 * Note: we have AVL, A, D bits desynced.
                 */
                AssertMsg(      (fPageShw & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK))
                             == (fPageGst & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK))
                          || (   pVCpu->pgm.s.cNetwareWp0Hacks > 0
                              &&    (fPageShw & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK | X86_PTE_RW | X86_PTE_US))
                                 == (fPageGst & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK | X86_PTE_RW | X86_PTE_US))
                              && (fPageShw & (X86_PTE_RW | X86_PTE_US)) == X86_PTE_RW
                              && (fPageGst & (X86_PTE_RW | X86_PTE_US)) == X86_PTE_US),
                          ("Page flags mismatch! pvFault=%RGv uErr=%x GCPhys=%RGp fPageShw=%RX64 fPageGst=%RX64 rc=%d\n",
                           pvFault, (uint32_t)uErr, GCPhys, fPageShw, fPageGst, rc));
01:01:15.623511 00:08:43.266063 Expression: (fPageShw & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK)) == (fPageGst & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK)) || ( pVCpu->pgm.s.cNetwareWp0Hacks > 0 && (fPageShw & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK | X86_PTE_RW | X86_PTE_US)) == (fPageGst & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK | X86_PTE_RW | X86_PTE_US)) && (fPageShw & (X86_PTE_RW | X86_PTE_US)) == X86_PTE_RW && (fPageGst & (X86_PTE_RW | X86_PTE_US)) == X86_PTE_US)
01:01:15.623511 00:08:43.266064 Location  : e:\vbox\svn\trunk\srcPage flags mismatch! pvFault=fffff801b0d7b000 uErr=11 GCPhys=0000000019b52000 fPageShw=0 fPageGst=77b0000000000121 rc=0

01:01:15.625516 00:08:43.268051 Expression: (fPageShw & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK)) == (fPageGst & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK)) || ( pVCpu->pgm.s.cNetwareWp0Hacks > 0 && (fPageShw & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK | X86_PTE_RW | X86_PTE_US)) == (fPageGst & ~(X86_PTE_A | X86_PTE_D | X86_PTE_AVL_MASK | X86_PTE_RW | X86_PTE_US)) && (fPageShw & (X86_PTE_RW | X86_PTE_US)) == X86_PTE_RW && (fPageGst & (X86_PTE_RW | X86_PTE_US)) == X86_PTE_US)
01:01:15.625516 00:08:43.268051 Location  :
e:\vbox\svn\trunk\srcPage flags mismatch!
pvFault=fffff801b0d7b000
                uErr=11     X86_TRAP_PF_ID | X86_TRAP_PF_P
GCPhys=0000000019b52000
fPageShw=0
fPageGst=77b0000000000121
rc=0
#endif

            }
            else
                AssertMsgFailed(("PGMGstGetPage rc=%Rrc\n", rc));
        }
        else
            AssertMsgFailed(("PGMGCGetPage rc=%Rrc\n", rc));
#   endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) && VBOX_STRICT */
    }


    /*
     * If we get here it is because something failed above, i.e. most like guru
     * meditiation time.
     */
    LogRel(("%s: returns rc=%Rrc pvFault=%RGv uErr=%RX64 cs:rip=%04x:%08RX64\n",
            __PRETTY_FUNCTION__, rc, pvFault, (uint64_t)uErr, pRegFrame->cs.Sel, pRegFrame->rip));
    return rc;

# else  /* Nested paging, EPT except PGM_GST_TYPE = PROT   */
    NOREF(uErr); NOREF(pRegFrame); NOREF(pvFault);
    AssertReleaseMsgFailed(("Shw=%d Gst=%d is not implemented!\n", PGM_SHW_TYPE, PGM_GST_TYPE));
    return VERR_PGM_NOT_USED_IN_MODE;
# endif
}
#endif /* !IN_RING3 */


/**
 * Emulation of the invlpg instruction.
 *
 *
 * @returns VBox status code.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtrPage   Page to invalidate.
 *
 * @remark  ASSUMES that the guest is updating before invalidating. This order
 *          isn't required by the CPU, so this is speculative and could cause
 *          trouble.
 * @remark  No TLB shootdown is done on any other VCPU as we assume that
 *          invlpg emulation is the *only* reason for calling this function.
 *          (The guest has to shoot down TLB entries on other CPUs itself)
 *          Currently true, but keep in mind!
 *
 * @todo    Clean this up! Most of it is (or should be) no longer necessary as we catch all page table accesses.
 *          Should only be required when PGMPOOL_WITH_OPTIMIZED_DIRTY_PT is active (PAE or AMD64 (for now))
 */
PGM_BTH_DECL(int, InvalidatePage)(PVMCPU pVCpu, RTGCPTR GCPtrPage)
{
#if    PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)   \
    && PGM_SHW_TYPE != PGM_TYPE_NESTED \
    && PGM_SHW_TYPE != PGM_TYPE_EPT
    int rc;
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

    PGM_LOCK_ASSERT_OWNER(pVM);

    LogFlow(("InvalidatePage %RGv\n", GCPtrPage));

    /*
     * Get the shadow PD entry and skip out if this PD isn't present.
     * (Guessing that it is frequent for a shadow PDE to not be present, do this first.)
     */
# if PGM_SHW_TYPE == PGM_TYPE_32BIT
    const unsigned  iPDDst    = (uint32_t)GCPtrPage >> SHW_PD_SHIFT;
    PX86PDE         pPdeDst   = pgmShwGet32BitPDEPtr(pVCpu, GCPtrPage);

    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE    pShwPde = pVCpu->pgm.s.CTX_SUFF(pShwPageCR3);
#  ifdef IN_RING3 /* Possible we didn't resync yet when called from REM. */
    if (!pShwPde)
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePageSkipped));
        return VINF_SUCCESS;
    }
#  else
    Assert(pShwPde);
#  endif

# elif PGM_SHW_TYPE == PGM_TYPE_PAE
    const unsigned  iPdpt     = (uint32_t)GCPtrPage >> X86_PDPT_SHIFT;
    PX86PDPT        pPdptDst  = pgmShwGetPaePDPTPtr(pVCpu);

    /* If the shadow PDPE isn't present, then skip the invalidate. */
#  ifdef IN_RING3 /* Possible we didn't resync yet when called from REM. */
    if (!pPdptDst || !pPdptDst->a[iPdpt].n.u1Present)
#  else
    if (!pPdptDst->a[iPdpt].n.u1Present)
#  endif
    {
        Assert(!pPdptDst || !(pPdptDst->a[iPdpt].u & PGM_PLXFLAGS_MAPPING));
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePageSkipped));
        PGM_INVL_PG(pVCpu, GCPtrPage);
        return VINF_SUCCESS;
    }

    const unsigned  iPDDst  = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PPGMPOOLPAGE    pShwPde = NULL;
    PX86PDPAE       pPDDst;

    /* Fetch the pgm pool shadow descriptor. */
    rc = pgmShwGetPaePoolPagePD(pVCpu, GCPtrPage, &pShwPde);
    AssertRCSuccessReturn(rc, rc);
    Assert(pShwPde);

    pPDDst             = (PX86PDPAE)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPde);
    PX86PDEPAE pPdeDst = &pPDDst->a[iPDDst];

# else /* PGM_SHW_TYPE == PGM_TYPE_AMD64 */
    /* PML4 */
    /*const unsigned  iPml4     = (GCPtrPage >> X86_PML4_SHIFT) & X86_PML4_MASK;*/
    const unsigned  iPdpt     = (GCPtrPage >> X86_PDPT_SHIFT) & X86_PDPT_MASK_AMD64;
    const unsigned  iPDDst    = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PX86PDPAE       pPDDst;
    PX86PDPT        pPdptDst;
    PX86PML4E       pPml4eDst;
    rc = pgmShwGetLongModePDPtr(pVCpu, GCPtrPage, &pPml4eDst, &pPdptDst, &pPDDst);
    if (rc != VINF_SUCCESS)
    {
        AssertMsg(rc == VERR_PAGE_DIRECTORY_PTR_NOT_PRESENT || rc == VERR_PAGE_MAP_LEVEL4_NOT_PRESENT, ("Unexpected rc=%Rrc\n", rc));
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePageSkipped));
        PGM_INVL_PG(pVCpu, GCPtrPage);
        return VINF_SUCCESS;
    }
    Assert(pPDDst);

    PX86PDEPAE  pPdeDst  = &pPDDst->a[iPDDst];
    PX86PDPE    pPdpeDst = &pPdptDst->a[iPdpt];

    if (!pPdpeDst->n.u1Present)
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePageSkipped));
        PGM_INVL_PG(pVCpu, GCPtrPage);
        return VINF_SUCCESS;
    }

    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE pShwPde = pgmPoolGetPage(pPool, pPdptDst->a[iPdpt].u & SHW_PDPE_PG_MASK);
    Assert(pShwPde);

# endif /* PGM_SHW_TYPE == PGM_TYPE_AMD64 */

    const SHWPDE PdeDst = *pPdeDst;
    if (!PdeDst.n.u1Present)
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePageSkipped));
        PGM_INVL_PG(pVCpu, GCPtrPage);
        return VINF_SUCCESS;
    }

    /*
     * Get the guest PD entry and calc big page.
     */
# if PGM_GST_TYPE == PGM_TYPE_32BIT
    PGSTPD          pPDSrc      = pgmGstGet32bitPDPtr(pVCpu);
    const unsigned  iPDSrc      = (uint32_t)GCPtrPage >> GST_PD_SHIFT;
    GSTPDE          PdeSrc      = pPDSrc->a[iPDSrc];
# else /* PGM_GST_TYPE != PGM_TYPE_32BIT */
    unsigned        iPDSrc = 0;
#  if PGM_GST_TYPE == PGM_TYPE_PAE
    X86PDPE         PdpeSrcIgn;
    PX86PDPAE       pPDSrc      = pgmGstGetPaePDPtr(pVCpu, GCPtrPage, &iPDSrc, &PdpeSrcIgn);
#  else /* AMD64 */
    PX86PML4E       pPml4eSrcIgn;
    X86PDPE         PdpeSrcIgn;
    PX86PDPAE       pPDSrc      = pgmGstGetLongModePDPtr(pVCpu, GCPtrPage, &pPml4eSrcIgn, &PdpeSrcIgn, &iPDSrc);
#  endif
    GSTPDE          PdeSrc;

    if (pPDSrc)
        PdeSrc = pPDSrc->a[iPDSrc];
    else
        PdeSrc.u = 0;
# endif /* PGM_GST_TYPE != PGM_TYPE_32BIT */
    const bool      fWasBigPage = RT_BOOL(PdeDst.u & PGM_PDFLAGS_BIG_PAGE);
    const bool      fIsBigPage  = PdeSrc.b.u1Size && GST_IS_PSE_ACTIVE(pVCpu);
    if (fWasBigPage != fIsBigPage)
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePageSkipped));

# ifdef IN_RING3
    /*
     * If a CR3 Sync is pending we may ignore the invalidate page operation
     * depending on the kind of sync and if it's a global page or not.
     * This doesn't make sense in GC/R0 so we'll skip it entirely there.
     */
#  ifdef PGM_SKIP_GLOBAL_PAGEDIRS_ON_NONGLOBAL_FLUSH
    if (    VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3)
        || (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL)
            && fIsBigPage
            && PdeSrc.b.u1Global
           )
       )
#  else
    if (VM_FF_IS_PENDING(pVM, VM_FF_PGM_SYNC_CR3 | VM_FF_PGM_SYNC_CR3_NON_GLOBAL) )
#  endif
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePageSkipped));
        return VINF_SUCCESS;
    }
# endif /* IN_RING3 */

    /*
     * Deal with the Guest PDE.
     */
    rc = VINF_SUCCESS;
    if (PdeSrc.n.u1Present)
    {
        Assert(     PdeSrc.n.u1User == PdeDst.n.u1User
               &&   (PdeSrc.n.u1Write || !PdeDst.n.u1Write || pVCpu->pgm.s.cNetwareWp0Hacks > 0));
# ifndef PGM_WITHOUT_MAPPING
        if (PdeDst.u & PGM_PDFLAGS_MAPPING)
        {
            /*
             * Conflict - Let SyncPT deal with it to avoid duplicate code.
             */
            Assert(pgmMapAreMappingsEnabled(pVM));
            Assert(PGMGetGuestMode(pVCpu) <= PGMMODE_PAE);
            rc = PGM_BTH_NAME(SyncPT)(pVCpu, iPDSrc, pPDSrc, GCPtrPage);
        }
        else
# endif /* !PGM_WITHOUT_MAPPING */
        if (!fIsBigPage)
        {
            /*
             * 4KB - page.
             */
            PPGMPOOLPAGE    pShwPage = pgmPoolGetPage(pPool, PdeDst.u & SHW_PDE_PG_MASK);
            RTGCPHYS        GCPhys   = GST_GET_PDE_GCPHYS(PdeSrc);

# if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
            /* Select the right PDE as we're emulating a 4kb page table with 2 shadow page tables. */
            GCPhys = PGM_A20_APPLY(pVCpu, GCPhys | ((iPDDst & 1) * (PAGE_SIZE / 2)));
# endif
            if (pShwPage->GCPhys == GCPhys)
            {
                /* Syncing it here isn't 100% safe and it's probably not worth spending time syncing it. */
                PSHWPT pPTDst = (PSHWPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPage);

                PGSTPT pPTSrc;
                rc = PGM_GCPHYS_2_PTR_V2(pVM, pVCpu, GST_GET_PDE_GCPHYS(PdeSrc), &pPTSrc);
                if (RT_SUCCESS(rc))
                {
                    const unsigned iPTSrc = (GCPtrPage >> GST_PT_SHIFT) & GST_PT_MASK;
                    GSTPTE PteSrc = pPTSrc->a[iPTSrc];
                    const unsigned iPTDst = (GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK;
                    PGM_BTH_NAME(SyncPageWorker)(pVCpu, &pPTDst->a[iPTDst], PdeSrc, PteSrc, pShwPage, iPTDst);
                    Log2(("SyncPage: 4K  %RGv PteSrc:{P=%d RW=%d U=%d raw=%08llx} PteDst=%08llx %s\n",
                            GCPtrPage, PteSrc.n.u1Present,
                            PteSrc.n.u1Write & PdeSrc.n.u1Write,
                            PteSrc.n.u1User  & PdeSrc.n.u1User,
                            (uint64_t)PteSrc.u,
                            SHW_PTE_LOG64(pPTDst->a[iPTDst]),
                            SHW_PTE_IS_TRACK_DIRTY(pPTDst->a[iPTDst]) ? " Track-Dirty" : ""));
                }
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePage4KBPages));
                PGM_INVL_PG(pVCpu, GCPtrPage);
            }
            else
            {
                /*
                 * The page table address changed.
                 */
                LogFlow(("InvalidatePage: Out-of-sync at %RGp PdeSrc=%RX64 PdeDst=%RX64 ShwGCPhys=%RGp iPDDst=%#x\n",
                         GCPtrPage, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u, pShwPage->GCPhys, iPDDst));
                pgmPoolFree(pVM, PdeDst.u & SHW_PDE_PG_MASK, pShwPde->idx, iPDDst);
                ASMAtomicWriteSize(pPdeDst, 0);
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePagePDOutOfSync));
                PGM_INVL_VCPU_TLBS(pVCpu);
            }
        }
        else
        {
            /*
             * 2/4MB - page.
             */
            /* Before freeing the page, check if anything really changed. */
            PPGMPOOLPAGE    pShwPage = pgmPoolGetPage(pPool, PdeDst.u & SHW_PDE_PG_MASK);
            RTGCPHYS        GCPhys   = GST_GET_BIG_PDE_GCPHYS(pVM, PdeSrc);
# if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
            /* Select the right PDE as we're emulating a 4MB page directory with two 2 MB shadow PDEs.*/
            GCPhys = PGM_A20_APPLY(pVCpu, GCPhys | (GCPtrPage & (1 << X86_PD_PAE_SHIFT)));
# endif
            if (    pShwPage->GCPhys == GCPhys
                &&  pShwPage->enmKind == BTH_PGMPOOLKIND_PT_FOR_BIG)
            {
                /* ASSUMES a the given bits are identical for 4M and normal PDEs */
                /** @todo This test is wrong as it cannot check the G bit!
                 *        FIXME */
                if (        (PdeSrc.u & (X86_PDE_P | X86_PDE_RW | X86_PDE_US))
                        ==  (PdeDst.u & (X86_PDE_P | X86_PDE_RW | X86_PDE_US))
                    &&  (   PdeSrc.b.u1Dirty /** @todo rainy day: What about read-only 4M pages? not very common, but still... */
                         || (PdeDst.u & PGM_PDFLAGS_TRACK_DIRTY)))
                {
                    LogFlow(("Skipping flush for big page containing %RGv (PD=%X .u=%RX64)-> nothing has changed!\n", GCPtrPage, iPDSrc, PdeSrc.u));
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePage4MBPagesSkip));
                    return VINF_SUCCESS;
                }
            }

            /*
             * Ok, the page table is present and it's been changed in the guest.
             * If we're in host context, we'll just mark it as not present taking the lazy approach.
             * We could do this for some flushes in GC too, but we need an algorithm for
             * deciding which 4MB pages containing code likely to be executed very soon.
             */
            LogFlow(("InvalidatePage: Out-of-sync PD at %RGp PdeSrc=%RX64 PdeDst=%RX64\n",
                     GCPtrPage, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
            pgmPoolFree(pVM, PdeDst.u & SHW_PDE_PG_MASK, pShwPde->idx, iPDDst);
            ASMAtomicWriteSize(pPdeDst, 0);
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePage4MBPages));
            PGM_INVL_BIG_PG(pVCpu, GCPtrPage);
        }
    }
    else
    {
        /*
         * Page directory is not present, mark shadow PDE not present.
         */
        if (!(PdeDst.u & PGM_PDFLAGS_MAPPING))
        {
            pgmPoolFree(pVM, PdeDst.u & SHW_PDE_PG_MASK, pShwPde->idx, iPDDst);
            ASMAtomicWriteSize(pPdeDst, 0);
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePagePDNPs));
            PGM_INVL_PG(pVCpu, GCPtrPage);
        }
        else
        {
            Assert(pgmMapAreMappingsEnabled(pVM));
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,InvalidatePagePDMappings));
        }
    }
    return rc;

#else /* guest real and protected mode */
    /* There's no such thing as InvalidatePage when paging is disabled, so just ignore. */
    NOREF(pVCpu); NOREF(GCPtrPage);
    return VINF_SUCCESS;
#endif
}


/**
 * Update the tracking of shadowed pages.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pShwPage    The shadow page.
 * @param   HCPhys      The physical page we is being dereferenced.
 * @param   iPte        Shadow PTE index
 * @param   GCPhysPage  Guest physical address (only valid if pShwPage->fDirty is set)
 */
DECLINLINE(void) PGM_BTH_NAME(SyncPageWorkerTrackDeref)(PVMCPU pVCpu, PPGMPOOLPAGE pShwPage, RTHCPHYS HCPhys, uint16_t iPte,
                                                        RTGCPHYS GCPhysPage)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

# if    defined(PGMPOOL_WITH_OPTIMIZED_DIRTY_PT) \
     && PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) \
     && (PGM_GST_TYPE == PGM_TYPE_PAE || PGM_GST_TYPE == PGM_TYPE_AMD64 || PGM_SHW_TYPE == PGM_TYPE_PAE /* pae/32bit combo */)

    /* Use the hint we retrieved from the cached guest PT. */
    if (pShwPage->fDirty)
    {
        PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

        Assert(pShwPage->cPresent);
        Assert(pPool->cPresent);
        pShwPage->cPresent--;
        pPool->cPresent--;

        PPGMPAGE pPhysPage = pgmPhysGetPage(pVM, GCPhysPage);
        AssertRelease(pPhysPage);
        pgmTrackDerefGCPhys(pPool, pShwPage, pPhysPage, iPte);
        return;
    }
# else
  NOREF(GCPhysPage);
# endif

    STAM_PROFILE_START(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackDeref, a);
    LogFlow(("SyncPageWorkerTrackDeref: Damn HCPhys=%RHp pShwPage->idx=%#x!!!\n", HCPhys, pShwPage->idx));

    /** @todo If this turns out to be a bottle neck (*very* likely) two things can be done:
     *      1. have a medium sized HCPhys -> GCPhys TLB (hash?)
     *      2. write protect all shadowed pages. I.e. implement caching.
     */
    /** @todo duplicated in the 2nd half of pgmPoolTracDerefGCPhysHint */

    /*
     * Find the guest address.
     */
    for (PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(pRamRangesX);
         pRam;
         pRam = pRam->CTX_SUFF(pNext))
    {
        unsigned iPage = pRam->cb >> PAGE_SHIFT;
        while (iPage-- > 0)
        {
            if (PGM_PAGE_GET_HCPHYS(&pRam->aPages[iPage]) == HCPhys)
            {
                PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

                Assert(pShwPage->cPresent);
                Assert(pPool->cPresent);
                pShwPage->cPresent--;
                pPool->cPresent--;

                pgmTrackDerefGCPhys(pPool, pShwPage, &pRam->aPages[iPage], iPte);
                STAM_PROFILE_STOP(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackDeref, a);
                return;
            }
        }
    }

    for (;;)
        AssertReleaseMsgFailed(("HCPhys=%RHp wasn't found!\n", HCPhys));
}


/**
 * Update the tracking of shadowed pages.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pShwPage    The shadow page.
 * @param   u16         The top 16-bit of the pPage->HCPhys.
 * @param   pPage       Pointer to the guest page. this will be modified.
 * @param   iPTDst      The index into the shadow table.
 */
DECLINLINE(void) PGM_BTH_NAME(SyncPageWorkerTrackAddref)(PVMCPU pVCpu, PPGMPOOLPAGE pShwPage, uint16_t u16, PPGMPAGE pPage, const unsigned iPTDst)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    /*
     * Just deal with the simple first time here.
     */
    if (!u16)
    {
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackVirgin);
        u16 = PGMPOOL_TD_MAKE(1, pShwPage->idx);
        /* Save the page table index. */
        PGM_PAGE_SET_PTE_INDEX(pVM, pPage, iPTDst);
    }
    else
        u16 = pgmPoolTrackPhysExtAddref(pVM, pPage, u16, pShwPage->idx, iPTDst);

    /* write back */
    Log2(("SyncPageWorkerTrackAddRef: u16=%#x->%#x  iPTDst=%#x\n", u16, PGM_PAGE_GET_TRACKING(pPage), iPTDst));
    PGM_PAGE_SET_TRACKING(pVM, pPage, u16);

    /* update statistics. */
    pVM->pgm.s.CTX_SUFF(pPool)->cPresent++;
    pShwPage->cPresent++;
    if (pShwPage->iFirstPresent > iPTDst)
        pShwPage->iFirstPresent = iPTDst;
}


/**
 * Modifies a shadow PTE to account for access handlers.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPage       The page in question.
 * @param   fPteSrc     The shadowed flags of the source PTE.  Must include the
 *                      A (accessed) bit so it can be emulated correctly.
 * @param   pPteDst     The shadow PTE (output).  This is temporary storage and
 *                      does not need to be set atomically.
 */
DECLINLINE(void) PGM_BTH_NAME(SyncHandlerPte)(PVM pVM, PCPGMPAGE pPage, uint64_t fPteSrc, PSHWPTE pPteDst)
{
    NOREF(pVM); RT_NOREF_PV(fPteSrc);

    /** @todo r=bird: Are we actually handling dirty and access bits for pages with access handlers correctly? No.
     *  Update: \#PF should deal with this before or after calling the handlers. It has all the info to do the job efficiently. */
    if (!PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage))
    {
        LogFlow(("SyncHandlerPte: monitored page (%R[pgmpage]) -> mark read-only\n", pPage));
#if PGM_SHW_TYPE == PGM_TYPE_EPT
        pPteDst->u             = PGM_PAGE_GET_HCPHYS(pPage);
        pPteDst->n.u1Present   = 1;
        pPteDst->n.u1Execute   = 1;
        pPteDst->n.u1IgnorePAT = 1;
        pPteDst->n.u3EMT       = VMX_EPT_MEMTYPE_WB;
        /* PteDst.n.u1Write = 0 && PteDst.n.u1Size = 0 */
#else
        if (fPteSrc & X86_PTE_A)
        {
            SHW_PTE_SET(*pPteDst, fPteSrc | PGM_PAGE_GET_HCPHYS(pPage));
            SHW_PTE_SET_RO(*pPteDst);
        }
        else
            SHW_PTE_SET(*pPteDst, 0);
#endif
    }
#ifdef PGM_WITH_MMIO_OPTIMIZATIONS
# if PGM_SHW_TYPE == PGM_TYPE_EPT || PGM_SHW_TYPE == PGM_TYPE_PAE || PGM_SHW_TYPE == PGM_TYPE_AMD64
    else if (   PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage)
             && (   BTH_IS_NP_ACTIVE(pVM)
                 || (fPteSrc & (X86_PTE_RW | X86_PTE_US)) == X86_PTE_RW) /** @todo Remove X86_PTE_US here and pGstWalk->Core.fEffectiveUS before the sync page test. */
#  if PGM_SHW_TYPE == PGM_TYPE_AMD64
             && pVM->pgm.s.fLessThan52PhysicalAddressBits
#  endif
            )
    {
        LogFlow(("SyncHandlerPte: MMIO page -> invalid \n"));
#  if PGM_SHW_TYPE == PGM_TYPE_EPT
        /* 25.2.3.1: Reserved physical address bit -> EPT Misconfiguration (exit 49) */
        pPteDst->u = pVM->pgm.s.HCPhysInvMmioPg;
        /* 25.2.3.1: bits 2:0 = 010b -> EPT Misconfiguration (exit 49) */
        pPteDst->n.u1Present = 0;
        pPteDst->n.u1Write   = 1;
        pPteDst->n.u1Execute = 0;
        /* 25.2.3.1: leaf && 2:0 != 0 && u3Emt in {2, 3, 7} -> EPT Misconfiguration */
        pPteDst->n.u3EMT     = 7;
#  else
        /* Set high page frame bits that MBZ (bankers on PAE, CPU dependent on AMD64).  */
        SHW_PTE_SET(*pPteDst, pVM->pgm.s.HCPhysInvMmioPg | X86_PTE_PAE_MBZ_MASK_NO_NX | X86_PTE_P);
#  endif
    }
# endif
#endif /* PGM_WITH_MMIO_OPTIMIZATIONS */
    else
    {
        LogFlow(("SyncHandlerPte: monitored page (%R[pgmpage]) -> mark not present\n", pPage));
        SHW_PTE_SET(*pPteDst, 0);
    }
    /** @todo count these kinds of entries. */
}


/**
 * Creates a 4K shadow page for a guest page.
 *
 * For 4M pages the caller must convert the PDE4M to a PTE, this includes adjusting the
 * physical address.  The PdeSrc argument only the flags are used.  No page
 * structured will be mapped in this function.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pPteDst     Destination page table entry.
 * @param   PdeSrc      Source page directory entry (i.e. Guest OS page directory entry).
 *                      Can safely assume that only the flags are being used.
 * @param   PteSrc      Source page table entry (i.e. Guest OS page table entry).
 * @param   pShwPage    Pointer to the shadow page.
 * @param   iPTDst      The index into the shadow table.
 *
 * @remark  Not used for 2/4MB pages!
 */
#if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) || defined(DOXYGEN_RUNNING)
static void PGM_BTH_NAME(SyncPageWorker)(PVMCPU pVCpu, PSHWPTE pPteDst, GSTPDE PdeSrc, GSTPTE PteSrc,
                                         PPGMPOOLPAGE pShwPage, unsigned iPTDst)
#else
static void PGM_BTH_NAME(SyncPageWorker)(PVMCPU pVCpu, PSHWPTE pPteDst, RTGCPHYS GCPhysPage,
                                         PPGMPOOLPAGE pShwPage, unsigned iPTDst)
#endif
{
    PVM      pVM = pVCpu->CTX_SUFF(pVM);
    RTGCPHYS GCPhysOldPage = NIL_RTGCPHYS;

#if    defined(PGMPOOL_WITH_OPTIMIZED_DIRTY_PT) \
     && PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) \
     && (PGM_GST_TYPE == PGM_TYPE_PAE || PGM_GST_TYPE == PGM_TYPE_AMD64 || PGM_SHW_TYPE == PGM_TYPE_PAE /* pae/32bit combo */)

    if (pShwPage->fDirty)
    {
        PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
        PGSTPT pGstPT;

        /* Note that iPTDst can be used to index the guest PT even in the pae/32bit combo as we copy only half the table; see pgmPoolAddDirtyPage. */
        pGstPT = (PGSTPT)&pPool->aDirtyPages[pShwPage->idxDirtyEntry].aPage[0];
        GCPhysOldPage = GST_GET_PTE_GCPHYS(pGstPT->a[iPTDst]);
        pGstPT->a[iPTDst].u = PteSrc.u;
    }
#else
    Assert(!pShwPage->fDirty);
#endif

#if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    if (   PteSrc.n.u1Present
        && GST_IS_PTE_VALID(pVCpu, PteSrc))
#endif
    {
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        RTGCPHYS GCPhysPage = GST_GET_PTE_GCPHYS(PteSrc);
# endif
        PGM_A20_ASSERT_MASKED(pVCpu, GCPhysPage);

        /*
         * Find the ram range.
         */
        PPGMPAGE pPage;
        int rc = pgmPhysGetPageEx(pVM, GCPhysPage, &pPage);
        if (RT_SUCCESS(rc))
        {
            /* Ignore ballooned pages.
               Don't return errors or use a fatal assert here as part of a
               shadow sync range might included ballooned pages. */
            if (PGM_PAGE_IS_BALLOONED(pPage))
            {
                Assert(!SHW_PTE_IS_P(*pPteDst)); /** @todo user tracking needs updating if this triggers. */
                return;
            }

#ifndef VBOX_WITH_NEW_LAZY_PAGE_ALLOC
            /* Make the page writable if necessary. */
            if (    PGM_PAGE_GET_TYPE(pPage)  == PGMPAGETYPE_RAM
                &&  (   PGM_PAGE_IS_ZERO(pPage)
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
                     || (   PteSrc.n.u1Write
# else
                     || (   1
# endif
                         && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED
# ifdef VBOX_WITH_REAL_WRITE_MONITORED_PAGES
                         && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_WRITE_MONITORED
# endif
# ifdef VBOX_WITH_PAGE_SHARING
                         && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_SHARED
# endif
                        )
                     )
               )
            {
                rc = pgmPhysPageMakeWritable(pVM, pPage, GCPhysPage);
                AssertRC(rc);
            }
#endif

            /*
             * Make page table entry.
             */
            SHWPTE PteDst;
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
            uint64_t fGstShwPteFlags = GST_GET_PTE_SHW_FLAGS(pVCpu, PteSrc);
# else
            uint64_t fGstShwPteFlags = X86_PTE_P | X86_PTE_RW | X86_PTE_US | X86_PTE_A | X86_PTE_D;
# endif
            if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
                PGM_BTH_NAME(SyncHandlerPte)(pVM, pPage, fGstShwPteFlags, &PteDst);
            else
            {
#if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
                /*
                 * If the page or page directory entry is not marked accessed,
                 * we mark the page not present.
                 */
                if (!PteSrc.n.u1Accessed || !PdeSrc.n.u1Accessed)
                {
                    LogFlow(("SyncPageWorker: page and or page directory not accessed -> mark not present\n"));
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,AccessedPage));
                    SHW_PTE_SET(PteDst, 0);
                }
                /*
                 * If the page is not flagged as dirty and is writable, then make it read-only, so we can set the dirty bit
                 * when the page is modified.
                 */
                else if (!PteSrc.n.u1Dirty && (PdeSrc.n.u1Write & PteSrc.n.u1Write))
                {
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPage));
                    SHW_PTE_SET(PteDst,
                                  fGstShwPteFlags
                                | PGM_PAGE_GET_HCPHYS(pPage)
                                | PGM_PTFLAGS_TRACK_DIRTY);
                    SHW_PTE_SET_RO(PteDst);
                }
                else
#endif
                {
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageSkipped));
#if PGM_SHW_TYPE == PGM_TYPE_EPT
                    PteDst.u             = PGM_PAGE_GET_HCPHYS(pPage);
                    PteDst.n.u1Present   = 1;
                    PteDst.n.u1Write     = 1;
                    PteDst.n.u1Execute   = 1;
                    PteDst.n.u1IgnorePAT = 1;
                    PteDst.n.u3EMT       = VMX_EPT_MEMTYPE_WB;
                    /* PteDst.n.u1Size = 0 */
#else
                    SHW_PTE_SET(PteDst, fGstShwPteFlags | PGM_PAGE_GET_HCPHYS(pPage));
#endif
                }

                /*
                 * Make sure only allocated pages are mapped writable.
                 */
                if (    SHW_PTE_IS_P_RW(PteDst)
                    &&  PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED)
                {
                    /* Still applies to shared pages. */
                    Assert(!PGM_PAGE_IS_ZERO(pPage));
                    SHW_PTE_SET_RO(PteDst);   /** @todo this isn't quite working yet. Why, isn't it? */
                    Log3(("SyncPageWorker: write-protecting %RGp pPage=%R[pgmpage]at iPTDst=%d\n", GCPhysPage, pPage, iPTDst));
                }
            }

            /*
             * Keep user track up to date.
             */
            if (SHW_PTE_IS_P(PteDst))
            {
                if (!SHW_PTE_IS_P(*pPteDst))
                    PGM_BTH_NAME(SyncPageWorkerTrackAddref)(pVCpu, pShwPage, PGM_PAGE_GET_TRACKING(pPage), pPage, iPTDst);
                else if (SHW_PTE_GET_HCPHYS(*pPteDst) != SHW_PTE_GET_HCPHYS(PteDst))
                {
                    Log2(("SyncPageWorker: deref! *pPteDst=%RX64 PteDst=%RX64\n", SHW_PTE_LOG64(*pPteDst), SHW_PTE_LOG64(PteDst)));
                    PGM_BTH_NAME(SyncPageWorkerTrackDeref)(pVCpu, pShwPage, SHW_PTE_GET_HCPHYS(*pPteDst), iPTDst, GCPhysOldPage);
                    PGM_BTH_NAME(SyncPageWorkerTrackAddref)(pVCpu, pShwPage, PGM_PAGE_GET_TRACKING(pPage), pPage, iPTDst);
                }
            }
            else if (SHW_PTE_IS_P(*pPteDst))
            {
                Log2(("SyncPageWorker: deref! *pPteDst=%RX64\n", SHW_PTE_LOG64(*pPteDst)));
                PGM_BTH_NAME(SyncPageWorkerTrackDeref)(pVCpu, pShwPage, SHW_PTE_GET_HCPHYS(*pPteDst), iPTDst, GCPhysOldPage);
            }

            /*
             * Update statistics and commit the entry.
             */
#if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
            if (!PteSrc.n.u1Global)
                pShwPage->fSeenNonGlobal = true;
#endif
            SHW_PTE_ATOMIC_SET2(*pPteDst, PteDst);
            return;
        }

/** @todo count these three different kinds. */
        Log2(("SyncPageWorker: invalid address in Pte\n"));
    }
#if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    else if (!PteSrc.n.u1Present)
        Log2(("SyncPageWorker: page not present in Pte\n"));
    else
        Log2(("SyncPageWorker: invalid Pte\n"));
#endif

    /*
     * The page is not present or the PTE is bad. Replace the shadow PTE by
     * an empty entry, making sure to keep the user tracking up to date.
     */
    if (SHW_PTE_IS_P(*pPteDst))
    {
        Log2(("SyncPageWorker: deref! *pPteDst=%RX64\n", SHW_PTE_LOG64(*pPteDst)));
        PGM_BTH_NAME(SyncPageWorkerTrackDeref)(pVCpu, pShwPage, SHW_PTE_GET_HCPHYS(*pPteDst), iPTDst, GCPhysOldPage);
    }
    SHW_PTE_ATOMIC_SET(*pPteDst, 0);
}


/**
 * Syncs a guest OS page.
 *
 * There are no conflicts at this point, neither is there any need for
 * page table allocations.
 *
 * When called in PAE or AMD64 guest mode, the guest PDPE shall be valid.
 * When called in AMD64 guest mode, the guest PML4E shall be valid.
 *
 * @returns VBox status code.
 * @returns VINF_PGM_SYNCPAGE_MODIFIED_PDE if it modifies the PDE in any way.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   PdeSrc      Page directory entry of the guest.
 * @param   GCPtrPage   Guest context page address.
 * @param   cPages      Number of pages to sync (PGM_SYNC_N_PAGES) (default=1).
 * @param   uErr        Fault error (X86_TRAP_PF_*).
 */
static int PGM_BTH_NAME(SyncPage)(PVMCPU pVCpu, GSTPDE PdeSrc, RTGCPTR GCPtrPage, unsigned cPages, unsigned uErr)
{
    PVM      pVM   = pVCpu->CTX_SUFF(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool); NOREF(pPool);
    LogFlow(("SyncPage: GCPtrPage=%RGv cPages=%u uErr=%#x\n", GCPtrPage, cPages, uErr));
    RT_NOREF_PV(uErr); RT_NOREF_PV(cPages); RT_NOREF_PV(GCPtrPage);

    PGM_LOCK_ASSERT_OWNER(pVM);

#if    (   PGM_GST_TYPE == PGM_TYPE_32BIT  \
        || PGM_GST_TYPE == PGM_TYPE_PAE    \
        || PGM_GST_TYPE == PGM_TYPE_AMD64) \
    && PGM_SHW_TYPE != PGM_TYPE_NESTED     \
    && PGM_SHW_TYPE != PGM_TYPE_EPT

    /*
     * Assert preconditions.
     */
    Assert(PdeSrc.n.u1Present);
    Assert(cPages);
# if 0 /* rarely useful; leave for debugging. */
    STAM_COUNTER_INC(&pVCpu->pgm.s.StatSyncPagePD[(GCPtrPage >> GST_PD_SHIFT) & GST_PD_MASK]);
# endif

    /*
     * Get the shadow PDE, find the shadow page table in the pool.
     */
# if PGM_SHW_TYPE == PGM_TYPE_32BIT
    const unsigned  iPDDst   = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PX86PDE         pPdeDst  = pgmShwGet32BitPDEPtr(pVCpu, GCPtrPage);

    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE    pShwPde = pVCpu->pgm.s.CTX_SUFF(pShwPageCR3);
    Assert(pShwPde);

# elif PGM_SHW_TYPE == PGM_TYPE_PAE
    const unsigned  iPDDst  = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PPGMPOOLPAGE    pShwPde = NULL;
    PX86PDPAE       pPDDst;

    /* Fetch the pgm pool shadow descriptor. */
    int rc2 = pgmShwGetPaePoolPagePD(pVCpu, GCPtrPage, &pShwPde);
    AssertRCSuccessReturn(rc2, rc2);
    Assert(pShwPde);

    pPDDst             = (PX86PDPAE)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPde);
    PX86PDEPAE pPdeDst = &pPDDst->a[iPDDst];

# elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    const unsigned  iPDDst   = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    const unsigned  iPdpt    = (GCPtrPage >> X86_PDPT_SHIFT) & X86_PDPT_MASK_AMD64;
    PX86PDPAE       pPDDst   = NULL;            /* initialized to shut up gcc */
    PX86PDPT        pPdptDst = NULL;            /* initialized to shut up gcc */

    int rc2 = pgmShwGetLongModePDPtr(pVCpu, GCPtrPage, NULL, &pPdptDst, &pPDDst);
    AssertRCSuccessReturn(rc2, rc2);
    Assert(pPDDst && pPdptDst);
    PX86PDEPAE      pPdeDst = &pPDDst->a[iPDDst];
# endif
    SHWPDE          PdeDst   = *pPdeDst;

    /*
     * - In the guest SMP case we could have blocked while another VCPU reused
     *   this page table.
     * - With W7-64 we may also take this path when the A bit is cleared on
     *   higher level tables (PDPE/PML4E).  The guest does not invalidate the
     *   relevant TLB entries.  If we're write monitoring any page mapped by
     *   the modified entry, we may end up here with a "stale" TLB entry.
     */
    if (!PdeDst.n.u1Present)
    {
        Log(("CPU%u: SyncPage: Pde at %RGv changed behind our back? (pPdeDst=%p/%RX64) uErr=%#x\n", pVCpu->idCpu, GCPtrPage, pPdeDst, (uint64_t)PdeDst.u, (uint32_t)uErr));
        AssertMsg(pVM->cCpus > 1 || (uErr & (X86_TRAP_PF_P | X86_TRAP_PF_RW)) == (X86_TRAP_PF_P | X86_TRAP_PF_RW),
                  ("Unexpected missing PDE p=%p/%RX64 uErr=%#x\n", pPdeDst, (uint64_t)PdeDst.u, (uint32_t)uErr));
        if (uErr & X86_TRAP_PF_P)
            PGM_INVL_PG(pVCpu, GCPtrPage);
        return VINF_SUCCESS;    /* force the instruction to be executed again. */
    }

    PPGMPOOLPAGE    pShwPage = pgmPoolGetPage(pPool, PdeDst.u & SHW_PDE_PG_MASK);
    Assert(pShwPage);

# if PGM_GST_TYPE == PGM_TYPE_AMD64
    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE    pShwPde  = pgmPoolGetPage(pPool, pPdptDst->a[iPdpt].u & X86_PDPE_PG_MASK);
    Assert(pShwPde);
# endif

    /*
     * Check that the page is present and that the shadow PDE isn't out of sync.
     */
    const bool      fBigPage  = PdeSrc.b.u1Size && GST_IS_PSE_ACTIVE(pVCpu);
    const bool      fPdeValid = !fBigPage ? GST_IS_PDE_VALID(pVCpu, PdeSrc) : GST_IS_BIG_PDE_VALID(pVCpu, PdeSrc);
    RTGCPHYS        GCPhys;
    if (!fBigPage)
    {
        GCPhys = GST_GET_PDE_GCPHYS(PdeSrc);
# if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
        /* Select the right PDE as we're emulating a 4kb page table with 2 shadow page tables. */
        GCPhys = PGM_A20_APPLY(pVCpu, GCPhys | ((iPDDst & 1) * (PAGE_SIZE / 2)));
# endif
    }
    else
    {
        GCPhys = GST_GET_BIG_PDE_GCPHYS(pVM, PdeSrc);
# if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
        /* Select the right PDE as we're emulating a 4MB page directory with two 2 MB shadow PDEs.*/
        GCPhys = PGM_A20_APPLY(pVCpu, GCPhys | (GCPtrPage & (1 << X86_PD_PAE_SHIFT)));
# endif
    }
    /** @todo This doesn't check the G bit of 2/4MB pages. FIXME */
    if (    fPdeValid
        &&  pShwPage->GCPhys == GCPhys
        &&  PdeSrc.n.u1Present
        &&  PdeSrc.n.u1User == PdeDst.n.u1User
        &&  (PdeSrc.n.u1Write == PdeDst.n.u1Write || !PdeDst.n.u1Write)
# if PGM_WITH_NX(PGM_GST_TYPE, PGM_SHW_TYPE)
        &&  (PdeSrc.n.u1NoExecute == PdeDst.n.u1NoExecute || !GST_IS_NX_ACTIVE(pVCpu))
# endif
       )
    {
        /*
         * Check that the PDE is marked accessed already.
         * Since we set the accessed bit *before* getting here on a #PF, this
         * check is only meant for dealing with non-#PF'ing paths.
         */
        if (PdeSrc.n.u1Accessed)
        {
            PSHWPT pPTDst = (PSHWPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPage);
            if (!fBigPage)
            {
                /*
                 * 4KB Page - Map the guest page table.
                 */
                PGSTPT pPTSrc;
                int rc = PGM_GCPHYS_2_PTR_V2(pVM, pVCpu, GST_GET_PDE_GCPHYS(PdeSrc), &pPTSrc);
                if (RT_SUCCESS(rc))
                {
# ifdef PGM_SYNC_N_PAGES
                    Assert(cPages == 1 || !(uErr & X86_TRAP_PF_P));
                    if (    cPages > 1
                        &&  !(uErr & X86_TRAP_PF_P)
                        &&  !VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY))
                    {
                        /*
                         * This code path is currently only taken when the caller is PGMTrap0eHandler
                         * for non-present pages!
                         *
                         * We're setting PGM_SYNC_NR_PAGES pages around the faulting page to sync it and
                         * deal with locality.
                         */
                        unsigned        iPTDst      = (GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK;
#  if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
                        /* Select the right PDE as we're emulating a 4kb page table with 2 shadow page tables. */
                        const unsigned  offPTSrc    = ((GCPtrPage >> SHW_PD_SHIFT) & 1) * 512;
#  else
                        const unsigned  offPTSrc    = 0;
#  endif
                        const unsigned  iPTDstEnd   = RT_MIN(iPTDst + PGM_SYNC_NR_PAGES / 2, RT_ELEMENTS(pPTDst->a));
                        if (iPTDst < PGM_SYNC_NR_PAGES / 2)
                            iPTDst = 0;
                        else
                            iPTDst -= PGM_SYNC_NR_PAGES / 2;

                        for (; iPTDst < iPTDstEnd; iPTDst++)
                        {
                            const PGSTPTE pPteSrc = &pPTSrc->a[offPTSrc + iPTDst];

                            if (   pPteSrc->n.u1Present
                                && !SHW_PTE_IS_P(pPTDst->a[iPTDst]))
                            {
                                RTGCPTR GCPtrCurPage = (GCPtrPage & ~(RTGCPTR)(GST_PT_MASK << GST_PT_SHIFT)) | ((offPTSrc + iPTDst) << PAGE_SHIFT);
                                NOREF(GCPtrCurPage);
#  ifdef VBOX_WITH_RAW_MODE_NOT_R0
                                /*
                                 * Assuming kernel code will be marked as supervisor - and not as user level
                                 * and executed using a conforming code selector - And marked as readonly.
                                 * Also assume that if we're monitoring a page, it's of no interest to CSAM.
                                 */
                                PPGMPAGE pPage;
                                if (    ((PdeSrc.u & pPteSrc->u) & (X86_PTE_RW | X86_PTE_US))
                                    ||  iPTDst == ((GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK)   /* always sync GCPtrPage */
                                    ||  !CSAMDoesPageNeedScanning(pVM, GCPtrCurPage)
                                    ||  (   (pPage = pgmPhysGetPage(pVM, pPteSrc->u & GST_PTE_PG_MASK))
                                         && PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
                                   )
#  endif /* else: CSAM not active */
                                   PGM_BTH_NAME(SyncPageWorker)(pVCpu, &pPTDst->a[iPTDst], PdeSrc, *pPteSrc, pShwPage, iPTDst);
                                Log2(("SyncPage: 4K+ %RGv PteSrc:{P=%d RW=%d U=%d raw=%08llx} PteDst=%08llx%s\n",
                                      GCPtrCurPage, pPteSrc->n.u1Present,
                                      pPteSrc->n.u1Write & PdeSrc.n.u1Write,
                                      pPteSrc->n.u1User  & PdeSrc.n.u1User,
                                      (uint64_t)pPteSrc->u,
                                      SHW_PTE_LOG64(pPTDst->a[iPTDst]),
                                      SHW_PTE_IS_TRACK_DIRTY(pPTDst->a[iPTDst]) ? " Track-Dirty" : ""));
                            }
                        }
                    }
                    else
# endif /* PGM_SYNC_N_PAGES */
                    {
                        const unsigned iPTSrc = (GCPtrPage >> GST_PT_SHIFT) & GST_PT_MASK;
                        GSTPTE PteSrc = pPTSrc->a[iPTSrc];
                        const unsigned iPTDst = (GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK;
                        PGM_BTH_NAME(SyncPageWorker)(pVCpu, &pPTDst->a[iPTDst], PdeSrc, PteSrc, pShwPage, iPTDst);
                        Log2(("SyncPage: 4K  %RGv PteSrc:{P=%d RW=%d U=%d raw=%08llx} PteDst=%08llx %s\n",
                              GCPtrPage, PteSrc.n.u1Present,
                              PteSrc.n.u1Write & PdeSrc.n.u1Write,
                              PteSrc.n.u1User  & PdeSrc.n.u1User,
                              (uint64_t)PteSrc.u,
                              SHW_PTE_LOG64(pPTDst->a[iPTDst]),
                              SHW_PTE_IS_TRACK_DIRTY(pPTDst->a[iPTDst]) ? " Track-Dirty" : ""));
                    }
                }
                else /* MMIO or invalid page: emulated in #PF handler. */
                {
                    LogFlow(("PGM_GCPHYS_2_PTR %RGp failed with %Rrc\n", GCPhys, rc));
                    Assert(!SHW_PTE_IS_P(pPTDst->a[(GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK]));
                }
            }
            else
            {
                /*
                 * 4/2MB page - lazy syncing shadow 4K pages.
                 * (There are many causes of getting here, it's no longer only CSAM.)
                 */
                /* Calculate the GC physical address of this 4KB shadow page. */
                GCPhys = PGM_A20_APPLY(pVCpu, GST_GET_BIG_PDE_GCPHYS(pVM, PdeSrc) | (GCPtrPage & GST_BIG_PAGE_OFFSET_MASK));
                /* Find ram range. */
                PPGMPAGE pPage;
                int rc = pgmPhysGetPageEx(pVM, GCPhys, &pPage);
                if (RT_SUCCESS(rc))
                {
                    AssertFatalMsg(!PGM_PAGE_IS_BALLOONED(pPage), ("Unexpected ballooned page at %RGp\n", GCPhys));

# ifndef VBOX_WITH_NEW_LAZY_PAGE_ALLOC
                    /* Try to make the page writable if necessary. */
                    if (    PGM_PAGE_GET_TYPE(pPage)  == PGMPAGETYPE_RAM
                        &&  (   PGM_PAGE_IS_ZERO(pPage)
                             || (   PdeSrc.n.u1Write
                                 && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED
#  ifdef VBOX_WITH_REAL_WRITE_MONITORED_PAGES
                                 && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_WRITE_MONITORED
#  endif
#  ifdef VBOX_WITH_PAGE_SHARING
                                 && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_SHARED
#  endif
                                 )
                             )
                       )
                    {
                        rc = pgmPhysPageMakeWritable(pVM, pPage, GCPhys);
                        AssertRC(rc);
                    }
# endif

                    /*
                     * Make shadow PTE entry.
                     */
                    SHWPTE PteDst;
                    if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
                        PGM_BTH_NAME(SyncHandlerPte)(pVM, pPage, GST_GET_BIG_PDE_SHW_FLAGS_4_PTE(pVCpu, PdeSrc), &PteDst);
                    else
                        SHW_PTE_SET(PteDst, GST_GET_BIG_PDE_SHW_FLAGS_4_PTE(pVCpu, PdeSrc) | PGM_PAGE_GET_HCPHYS(pPage));

                    const unsigned iPTDst = (GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK;
                    if (    SHW_PTE_IS_P(PteDst)
                        &&  !SHW_PTE_IS_P(pPTDst->a[iPTDst]))
                        PGM_BTH_NAME(SyncPageWorkerTrackAddref)(pVCpu, pShwPage, PGM_PAGE_GET_TRACKING(pPage), pPage, iPTDst);

                    /* Make sure only allocated pages are mapped writable. */
                    if (    SHW_PTE_IS_P_RW(PteDst)
                        &&  PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED)
                    {
                        /* Still applies to shared pages. */
                        Assert(!PGM_PAGE_IS_ZERO(pPage));
                        SHW_PTE_SET_RO(PteDst);   /** @todo this isn't quite working yet... */
                        Log3(("SyncPage: write-protecting %RGp pPage=%R[pgmpage] at %RGv\n", GCPhys, pPage, GCPtrPage));
                    }

                    SHW_PTE_ATOMIC_SET2(pPTDst->a[iPTDst], PteDst);

                    /*
                     * If the page is not flagged as dirty and is writable, then make it read-only
                     * at PD level, so we can set the dirty bit when the page is modified.
                     *
                     * ASSUMES that page access handlers are implemented on page table entry level.
                     *      Thus we will first catch the dirty access and set PDE.D and restart. If
                     *      there is an access handler, we'll trap again and let it work on the problem.
                     */
                    /** @todo r=bird: figure out why we need this here, SyncPT should've taken care of this already.
                     * As for invlpg, it simply frees the whole shadow PT.
                     * ...It's possibly because the guest clears it and the guest doesn't really tell us... */
                    if (    !PdeSrc.b.u1Dirty
                        &&  PdeSrc.b.u1Write)
                    {
                        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageBig));
                        PdeDst.u |= PGM_PDFLAGS_TRACK_DIRTY;
                        PdeDst.n.u1Write = 0;
                    }
                    else
                    {
                        PdeDst.au32[0] &= ~PGM_PDFLAGS_TRACK_DIRTY;
                        PdeDst.n.u1Write = PdeSrc.n.u1Write;
                    }
                    ASMAtomicWriteSize(pPdeDst, PdeDst.u);
                    Log2(("SyncPage: BIG %RGv PdeSrc:{P=%d RW=%d U=%d raw=%08llx} GCPhys=%RGp%s\n",
                          GCPtrPage, PdeSrc.n.u1Present, PdeSrc.n.u1Write, PdeSrc.n.u1User, (uint64_t)PdeSrc.u, GCPhys,
                          PdeDst.u & PGM_PDFLAGS_TRACK_DIRTY ? " Track-Dirty" : ""));
                }
                else
                {
                    LogFlow(("PGM_GCPHYS_2_PTR %RGp (big) failed with %Rrc\n", GCPhys, rc));
                    /** @todo must wipe the shadow page table entry in this
                     *        case. */
                }
            }
            PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);
            return VINF_SUCCESS;
        }

        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPagePDNAs));
    }
    else if (fPdeValid)
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPagePDOutOfSync));
        Log2(("SyncPage: Out-Of-Sync PDE at %RGp PdeSrc=%RX64 PdeDst=%RX64 (GCPhys %RGp vs %RGp)\n",
              GCPtrPage, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u, pShwPage->GCPhys, GCPhys));
    }
    else
    {
/// @todo        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_MID_Z(Stat,SyncPagePDOutOfSyncAndInvalid));
        Log2(("SyncPage: Bad PDE at %RGp PdeSrc=%RX64 PdeDst=%RX64 (GCPhys %RGp vs %RGp)\n",
              GCPtrPage, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u, pShwPage->GCPhys, GCPhys));
    }

    /*
     * Mark the PDE not present.  Restart the instruction and let #PF call SyncPT.
     * Yea, I'm lazy.
     */
    pgmPoolFreeByPage(pPool, pShwPage, pShwPde->idx, iPDDst);
    ASMAtomicWriteSize(pPdeDst, 0);

    PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);
    PGM_INVL_VCPU_TLBS(pVCpu);
    return VINF_PGM_SYNCPAGE_MODIFIED_PDE;


#elif (PGM_GST_TYPE == PGM_TYPE_REAL || PGM_GST_TYPE == PGM_TYPE_PROT) \
    && PGM_SHW_TYPE != PGM_TYPE_NESTED \
    && (PGM_SHW_TYPE != PGM_TYPE_EPT || PGM_GST_TYPE == PGM_TYPE_PROT) \
    && !defined(IN_RC)
    NOREF(PdeSrc);

# ifdef PGM_SYNC_N_PAGES
    /*
     * Get the shadow PDE, find the shadow page table in the pool.
     */
#  if PGM_SHW_TYPE == PGM_TYPE_32BIT
    X86PDE          PdeDst = pgmShwGet32BitPDE(pVCpu, GCPtrPage);

#  elif PGM_SHW_TYPE == PGM_TYPE_PAE
    X86PDEPAE       PdeDst = pgmShwGetPaePDE(pVCpu, GCPtrPage);

#  elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    const unsigned  iPDDst   = ((GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK);
    const unsigned  iPdpt    = (GCPtrPage >> X86_PDPT_SHIFT) & X86_PDPT_MASK_AMD64; NOREF(iPdpt);
    PX86PDPAE       pPDDst   = NULL;            /* initialized to shut up gcc */
    X86PDEPAE       PdeDst;
    PX86PDPT        pPdptDst = NULL;            /* initialized to shut up gcc */

    int rc = pgmShwGetLongModePDPtr(pVCpu, GCPtrPage, NULL, &pPdptDst, &pPDDst);
    AssertRCSuccessReturn(rc, rc);
    Assert(pPDDst && pPdptDst);
    PdeDst = pPDDst->a[iPDDst];
#  elif PGM_SHW_TYPE == PGM_TYPE_EPT
    const unsigned  iPDDst = ((GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK);
    PEPTPD          pPDDst;
    EPTPDE          PdeDst;

    int rc = pgmShwGetEPTPDPtr(pVCpu, GCPtrPage, NULL, &pPDDst);
    if (rc != VINF_SUCCESS)
    {
        AssertRC(rc);
        return rc;
    }
    Assert(pPDDst);
    PdeDst = pPDDst->a[iPDDst];
#  endif
    /* In the guest SMP case we could have blocked while another VCPU reused this page table. */
    if (!PdeDst.n.u1Present)
    {
        AssertMsg(pVM->cCpus > 1, ("Unexpected missing PDE %RX64\n", (uint64_t)PdeDst.u));
        Log(("CPU%d: SyncPage: Pde at %RGv changed behind our back!\n", pVCpu->idCpu, GCPtrPage));
        return VINF_SUCCESS;    /* force the instruction to be executed again. */
    }

    /* Can happen in the guest SMP case; other VCPU activated this PDE while we were blocking to handle the page fault. */
    if (PdeDst.n.u1Size)
    {
        Assert(pVM->pgm.s.fNestedPaging);
        Log(("CPU%d: SyncPage: Pde (big:%RX64) at %RGv changed behind our back!\n", pVCpu->idCpu, PdeDst.u, GCPtrPage));
        return VINF_SUCCESS;
    }

    /* Mask away the page offset. */
    GCPtrPage &= ~((RTGCPTR)0xfff);

    PPGMPOOLPAGE  pShwPage = pgmPoolGetPage(pPool, PdeDst.u & SHW_PDE_PG_MASK);
    PSHWPT        pPTDst   = (PSHWPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPage);

    Assert(cPages == 1 || !(uErr & X86_TRAP_PF_P));
    if (    cPages > 1
        &&  !(uErr & X86_TRAP_PF_P)
        &&  !VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY))
    {
        /*
         * This code path is currently only taken when the caller is PGMTrap0eHandler
         * for non-present pages!
         *
         * We're setting PGM_SYNC_NR_PAGES pages around the faulting page to sync it and
         * deal with locality.
         */
        unsigned        iPTDst    = (GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK;
        const unsigned  iPTDstEnd = RT_MIN(iPTDst + PGM_SYNC_NR_PAGES / 2, RT_ELEMENTS(pPTDst->a));
        if (iPTDst < PGM_SYNC_NR_PAGES / 2)
            iPTDst = 0;
        else
            iPTDst -= PGM_SYNC_NR_PAGES / 2;
        for (; iPTDst < iPTDstEnd; iPTDst++)
        {
            if (!SHW_PTE_IS_P(pPTDst->a[iPTDst]))
            {
                RTGCPTR GCPtrCurPage = PGM_A20_APPLY(pVCpu, (GCPtrPage & ~(RTGCPTR)(SHW_PT_MASK << SHW_PT_SHIFT))
                                                          | (iPTDst << PAGE_SHIFT));

                PGM_BTH_NAME(SyncPageWorker)(pVCpu, &pPTDst->a[iPTDst], GCPtrCurPage, pShwPage, iPTDst);
                Log2(("SyncPage: 4K+ %RGv PteSrc:{P=1 RW=1 U=1} PteDst=%08llx%s\n",
                      GCPtrCurPage,
                      SHW_PTE_LOG64(pPTDst->a[iPTDst]),
                      SHW_PTE_IS_TRACK_DIRTY(pPTDst->a[iPTDst]) ? " Track-Dirty" : ""));

                if (RT_UNLIKELY(VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY)))
                    break;
            }
            else
                Log4(("%RGv iPTDst=%x pPTDst->a[iPTDst] %RX64\n", (GCPtrPage & ~(RTGCPTR)(SHW_PT_MASK << SHW_PT_SHIFT)) | (iPTDst << PAGE_SHIFT), iPTDst, SHW_PTE_LOG64(pPTDst->a[iPTDst]) ));
        }
    }
    else
# endif /* PGM_SYNC_N_PAGES */
    {
        const unsigned  iPTDst       = (GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK;
        RTGCPTR         GCPtrCurPage = PGM_A20_APPLY(pVCpu, (GCPtrPage & ~(RTGCPTR)(SHW_PT_MASK << SHW_PT_SHIFT))
                                                          | (iPTDst << PAGE_SHIFT));

        PGM_BTH_NAME(SyncPageWorker)(pVCpu, &pPTDst->a[iPTDst], GCPtrCurPage, pShwPage, iPTDst);

        Log2(("SyncPage: 4K  %RGv PteSrc:{P=1 RW=1 U=1}PteDst=%08llx%s\n",
              GCPtrPage,
              SHW_PTE_LOG64(pPTDst->a[iPTDst]),
              SHW_PTE_IS_TRACK_DIRTY(pPTDst->a[iPTDst]) ? " Track-Dirty" : ""));
    }
    return VINF_SUCCESS;

#else
    NOREF(PdeSrc);
    AssertReleaseMsgFailed(("Shw=%d Gst=%d is not implemented!\n", PGM_GST_TYPE, PGM_SHW_TYPE));
    return VERR_PGM_NOT_USED_IN_MODE;
#endif
}


#if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)

/**
 * CheckPageFault helper for returning a page fault indicating a non-present
 * (NP) entry in the page translation structures.
 *
 * @returns VINF_EM_RAW_GUEST_TRAP.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   uErr            The error code of the shadow fault.  Corrections to
 *                          TRPM's copy will be made if necessary.
 * @param   GCPtrPage       For logging.
 * @param   uPageFaultLevel For logging.
 */
DECLINLINE(int) PGM_BTH_NAME(CheckPageFaultReturnNP)(PVMCPU pVCpu, uint32_t uErr, RTGCPTR GCPtrPage, unsigned uPageFaultLevel)
{
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyTrackRealPF));
    AssertMsg(!(uErr & X86_TRAP_PF_P), ("%#x\n", uErr));
    AssertMsg(!(uErr & X86_TRAP_PF_RSVD), ("%#x\n", uErr));
    if (uErr & (X86_TRAP_PF_RSVD | X86_TRAP_PF_P))
        TRPMSetErrorCode(pVCpu, uErr & ~(X86_TRAP_PF_RSVD | X86_TRAP_PF_P));

    Log(("CheckPageFault: real page fault (notp) at %RGv (%d)\n", GCPtrPage, uPageFaultLevel));
    RT_NOREF_PV(GCPtrPage); RT_NOREF_PV(uPageFaultLevel);
    return VINF_EM_RAW_GUEST_TRAP;
}


/**
 * CheckPageFault helper for returning a page fault indicating a reserved bit
 * (RSVD) error in the page translation structures.
 *
 * @returns VINF_EM_RAW_GUEST_TRAP.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   uErr            The error code of the shadow fault.  Corrections to
 *                          TRPM's copy will be made if necessary.
 * @param   GCPtrPage       For logging.
 * @param   uPageFaultLevel For logging.
 */
DECLINLINE(int) PGM_BTH_NAME(CheckPageFaultReturnRSVD)(PVMCPU pVCpu, uint32_t uErr, RTGCPTR GCPtrPage, unsigned uPageFaultLevel)
{
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyTrackRealPF));
    if ((uErr & (X86_TRAP_PF_RSVD | X86_TRAP_PF_P)) != (X86_TRAP_PF_RSVD | X86_TRAP_PF_P))
        TRPMSetErrorCode(pVCpu, uErr | X86_TRAP_PF_RSVD | X86_TRAP_PF_P);

    Log(("CheckPageFault: real page fault (rsvd) at %RGv (%d)\n", GCPtrPage, uPageFaultLevel));
    RT_NOREF_PV(GCPtrPage); RT_NOREF_PV(uPageFaultLevel);
    return VINF_EM_RAW_GUEST_TRAP;
}


/**
 * CheckPageFault helper for returning a page protection fault (P).
 *
 * @returns VINF_EM_RAW_GUEST_TRAP.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   uErr            The error code of the shadow fault.  Corrections to
 *                          TRPM's copy will be made if necessary.
 * @param   GCPtrPage       For logging.
 * @param   uPageFaultLevel For logging.
 */
DECLINLINE(int) PGM_BTH_NAME(CheckPageFaultReturnProt)(PVMCPU pVCpu, uint32_t uErr, RTGCPTR GCPtrPage, unsigned uPageFaultLevel)
{
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyTrackRealPF));
    AssertMsg(uErr & (X86_TRAP_PF_RW | X86_TRAP_PF_US | X86_TRAP_PF_ID), ("%#x\n", uErr));
    if ((uErr & (X86_TRAP_PF_P | X86_TRAP_PF_RSVD)) != X86_TRAP_PF_P)
        TRPMSetErrorCode(pVCpu, (uErr & ~X86_TRAP_PF_RSVD) | X86_TRAP_PF_P);

    Log(("CheckPageFault: real page fault (prot) at %RGv (%d)\n", GCPtrPage, uPageFaultLevel));
    RT_NOREF_PV(GCPtrPage); RT_NOREF_PV(uPageFaultLevel);
    return VINF_EM_RAW_GUEST_TRAP;
}


/**
 * Handle dirty bit tracking faults.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   uErr        Page fault error code.
 * @param   pPdeSrc     Guest page directory entry.
 * @param   pPdeDst     Shadow page directory entry.
 * @param   GCPtrPage   Guest context page address.
 */
static int PGM_BTH_NAME(CheckDirtyPageFault)(PVMCPU pVCpu, uint32_t uErr, PSHWPDE pPdeDst, GSTPDE const *pPdeSrc,
                                             RTGCPTR GCPtrPage)
{
    PVM         pVM   = pVCpu->CTX_SUFF(pVM);
    PPGMPOOL    pPool = pVM->pgm.s.CTX_SUFF(pPool);
    NOREF(uErr);

    PGM_LOCK_ASSERT_OWNER(pVM);

    /*
     * Handle big page.
     */
    if (pPdeSrc->b.u1Size && GST_IS_PSE_ACTIVE(pVCpu))
    {
        if (    pPdeDst->n.u1Present
            &&  (pPdeDst->u & PGM_PDFLAGS_TRACK_DIRTY))
        {
            SHWPDE PdeDst = *pPdeDst;

            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageTrap));
            Assert(pPdeSrc->b.u1Write);

            /* Note: No need to invalidate this entry on other VCPUs as a stale TLB entry will not harm; write access will simply
             *       fault again and take this path to only invalidate the entry (see below).
             */
            PdeDst.n.u1Write      = 1;
            PdeDst.n.u1Accessed   = 1;
            PdeDst.au32[0]       &= ~PGM_PDFLAGS_TRACK_DIRTY;
            ASMAtomicWriteSize(pPdeDst, PdeDst.u);
            PGM_INVL_BIG_PG(pVCpu, GCPtrPage);
            return VINF_PGM_HANDLED_DIRTY_BIT_FAULT;    /* restarts the instruction. */
        }

# ifdef IN_RING0
        /* Check for stale TLB entry; only applies to the SMP guest case. */
        if (    pVM->cCpus > 1
            &&  pPdeDst->n.u1Write
            &&  pPdeDst->n.u1Accessed)
        {
            PPGMPOOLPAGE    pShwPage = pgmPoolGetPage(pPool, pPdeDst->u & SHW_PDE_PG_MASK);
            if (pShwPage)
            {
                PSHWPT      pPTDst   = (PSHWPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPage);
                PSHWPTE     pPteDst  = &pPTDst->a[(GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK];
                if (SHW_PTE_IS_P_RW(*pPteDst))
                {
                    /* Stale TLB entry. */
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageStale));
                    PGM_INVL_PG(pVCpu, GCPtrPage);
                    return VINF_PGM_HANDLED_DIRTY_BIT_FAULT;    /* restarts the instruction. */
                }
            }
        }
# endif /* IN_RING0 */
        return VINF_PGM_NO_DIRTY_BIT_TRACKING;
    }

    /*
     * Map the guest page table.
     */
    PGSTPT pPTSrc;
    int rc = PGM_GCPHYS_2_PTR_V2(pVM, pVCpu, GST_GET_PDE_GCPHYS(*pPdeSrc), &pPTSrc);
    if (RT_FAILURE(rc))
    {
        AssertRC(rc);
        return rc;
    }

    if (pPdeDst->n.u1Present)
    {
        GSTPTE const  *pPteSrc = &pPTSrc->a[(GCPtrPage >> GST_PT_SHIFT) & GST_PT_MASK];
        const GSTPTE   PteSrc  = *pPteSrc;

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
        /* Bail out here as pgmPoolGetPage will return NULL and we'll crash below.
         * Our individual shadow handlers will provide more information and force a fatal exit.
         */
        if (   !HMIsEnabled(pVM)
            && MMHyperIsInsideArea(pVM, (RTGCPTR)GCPtrPage))
        {
            LogRel(("CheckPageFault: write to hypervisor region %RGv\n", GCPtrPage));
            return VINF_PGM_NO_DIRTY_BIT_TRACKING;
        }
#endif
        /*
         * Map shadow page table.
         */
        PPGMPOOLPAGE    pShwPage = pgmPoolGetPage(pPool, pPdeDst->u & SHW_PDE_PG_MASK);
        if (pShwPage)
        {
            PSHWPT      pPTDst   = (PSHWPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPage);
            PSHWPTE     pPteDst  = &pPTDst->a[(GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK];
            if (SHW_PTE_IS_P(*pPteDst))    /** @todo Optimize accessed bit emulation? */
            {
                if (SHW_PTE_IS_TRACK_DIRTY(*pPteDst))
                {
                    PPGMPAGE pPage  = pgmPhysGetPage(pVM, GST_GET_PTE_GCPHYS(PteSrc));
                    SHWPTE   PteDst = *pPteDst;

                    LogFlow(("DIRTY page trap addr=%RGv\n", GCPtrPage));
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageTrap));

                    Assert(PteSrc.n.u1Write);

                    /* Note: No need to invalidate this entry on other VCPUs as a stale TLB
                     *       entry will not harm; write access will simply fault again and
                     *       take this path to only invalidate the entry.
                     */
                    if (RT_LIKELY(pPage))
                    {
                        if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
                        {
                            //AssertMsgFailed(("%R[pgmpage] - we don't set PGM_PTFLAGS_TRACK_DIRTY for these pages\n", pPage));
                            Assert(!PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPage));
                            /* Assuming write handlers here as the PTE is present (otherwise we wouldn't be here). */
                            SHW_PTE_SET_RO(PteDst);
                        }
                        else
                        {
                            if (   PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_WRITE_MONITORED
                                && PGM_PAGE_GET_TYPE(pPage)  == PGMPAGETYPE_RAM)
                            {
                                rc = pgmPhysPageMakeWritable(pVM, pPage, GST_GET_PTE_GCPHYS(PteSrc));
                                AssertRC(rc);
                            }
                            if (PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED)
                                SHW_PTE_SET_RW(PteDst);
                            else
                            {
                                /* Still applies to shared pages. */
                                Assert(!PGM_PAGE_IS_ZERO(pPage));
                                SHW_PTE_SET_RO(PteDst);
                            }
                        }
                    }
                    else
                        SHW_PTE_SET_RW(PteDst);  /** @todo r=bird: This doesn't make sense to me. */

                    SHW_PTE_SET(PteDst, (SHW_PTE_GET_U(PteDst) | X86_PTE_D | X86_PTE_A) & ~(uint64_t)PGM_PTFLAGS_TRACK_DIRTY);
                    SHW_PTE_ATOMIC_SET2(*pPteDst, PteDst);
                    PGM_INVL_PG(pVCpu, GCPtrPage);
                    return VINF_PGM_HANDLED_DIRTY_BIT_FAULT;    /* restarts the instruction. */
                }

# ifdef IN_RING0
                /* Check for stale TLB entry; only applies to the SMP guest case. */
                if (    pVM->cCpus > 1
                    &&  SHW_PTE_IS_RW(*pPteDst)
                    &&  SHW_PTE_IS_A(*pPteDst))
                {
                    /* Stale TLB entry. */
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageStale));
                    PGM_INVL_PG(pVCpu, GCPtrPage);
                    return VINF_PGM_HANDLED_DIRTY_BIT_FAULT;    /* restarts the instruction. */
                }
# endif
            }
        }
        else
            AssertMsgFailed(("pgmPoolGetPageByHCPhys %RGp failed!\n", pPdeDst->u & SHW_PDE_PG_MASK));
    }

    return VINF_PGM_NO_DIRTY_BIT_TRACKING;
}

#endif /* PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE) */


/**
 * Sync a shadow page table.
 *
 * The shadow page table is not present in the shadow PDE.
 *
 * Handles mapping conflicts.
 *
 * This is called by VerifyAccessSyncPage, PrefetchPage, InvalidatePage (on
 * conflict), and Trap0eHandler.
 *
 * A precondition for this method is that the shadow PDE is not present.  The
 * caller must take the PGM lock before checking this and continue to hold it
 * when calling this method.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   iPDSrc      Page directory index.
 * @param   pPDSrc      Source page directory (i.e. Guest OS page directory).
 *                      Assume this is a temporary mapping.
 * @param   GCPtrPage   GC Pointer of the page that caused the fault
 */
static int PGM_BTH_NAME(SyncPT)(PVMCPU pVCpu, unsigned iPDSrc, PGSTPD pPDSrc, RTGCPTR GCPtrPage)
{
    PVM             pVM      = pVCpu->CTX_SUFF(pVM);
    PPGMPOOL        pPool    = pVM->pgm.s.CTX_SUFF(pPool); NOREF(pPool);

#if 0 /* rarely useful; leave for debugging. */
    STAM_COUNTER_INC(&pVCpu->pgm.s.StatSyncPtPD[iPDSrc]);
#endif
    LogFlow(("SyncPT: GCPtrPage=%RGv\n", GCPtrPage)); RT_NOREF_PV(GCPtrPage);

    PGM_LOCK_ASSERT_OWNER(pVM);

#if (   PGM_GST_TYPE == PGM_TYPE_32BIT \
     || PGM_GST_TYPE == PGM_TYPE_PAE \
     || PGM_GST_TYPE == PGM_TYPE_AMD64) \
 && PGM_SHW_TYPE != PGM_TYPE_NESTED \
 && PGM_SHW_TYPE != PGM_TYPE_EPT

    int             rc       = VINF_SUCCESS;

    STAM_PROFILE_START(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);

    /*
     * Some input validation first.
     */
    AssertMsg(iPDSrc == ((GCPtrPage >> GST_PD_SHIFT) & GST_PD_MASK), ("iPDSrc=%x GCPtrPage=%RGv\n", iPDSrc, GCPtrPage));

    /*
     * Get the relevant shadow PDE entry.
     */
# if PGM_SHW_TYPE == PGM_TYPE_32BIT
    const unsigned  iPDDst   = GCPtrPage >> SHW_PD_SHIFT;
    PSHWPDE         pPdeDst  = pgmShwGet32BitPDEPtr(pVCpu, GCPtrPage);

    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE    pShwPde  = pVCpu->pgm.s.CTX_SUFF(pShwPageCR3);
    Assert(pShwPde);

# elif PGM_SHW_TYPE == PGM_TYPE_PAE
    const unsigned  iPDDst   = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PPGMPOOLPAGE    pShwPde  = NULL;
    PX86PDPAE       pPDDst;
    PSHWPDE         pPdeDst;

    /* Fetch the pgm pool shadow descriptor. */
    rc = pgmShwGetPaePoolPagePD(pVCpu, GCPtrPage, &pShwPde);
    AssertRCSuccessReturn(rc, rc);
    Assert(pShwPde);

    pPDDst  = (PX86PDPAE)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPde);
    pPdeDst = &pPDDst->a[iPDDst];

# elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    const unsigned  iPdpt    = (GCPtrPage >> X86_PDPT_SHIFT) & X86_PDPT_MASK_AMD64;
    const unsigned  iPDDst   = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PX86PDPAE       pPDDst   = NULL;            /* initialized to shut up gcc */
    PX86PDPT        pPdptDst = NULL;            /* initialized to shut up gcc */
    rc = pgmShwGetLongModePDPtr(pVCpu, GCPtrPage, NULL, &pPdptDst, &pPDDst);
    AssertRCSuccessReturn(rc, rc);
    Assert(pPDDst);
    PSHWPDE         pPdeDst  = &pPDDst->a[iPDDst];
# endif
    SHWPDE          PdeDst   = *pPdeDst;

# if PGM_GST_TYPE == PGM_TYPE_AMD64
    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE    pShwPde  = pgmPoolGetPage(pPool, pPdptDst->a[iPdpt].u & X86_PDPE_PG_MASK);
    Assert(pShwPde);
# endif

# ifndef PGM_WITHOUT_MAPPINGS
    /*
     * Check for conflicts.
     * RC: In case of a conflict we'll go to Ring-3 and do a full SyncCR3.
     * R3: Simply resolve the conflict.
     */
    if (PdeDst.u & PGM_PDFLAGS_MAPPING)
    {
        Assert(pgmMapAreMappingsEnabled(pVM));
#  ifndef IN_RING3
        Log(("SyncPT: Conflict at %RGv\n", GCPtrPage));
        STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);
        return VERR_ADDRESS_CONFLICT;

#  else  /* IN_RING3 */
        PPGMMAPPING pMapping = pgmGetMapping(pVM, (RTGCPTR)GCPtrPage);
        Assert(pMapping);
#   if PGM_GST_TYPE == PGM_TYPE_32BIT
        rc = pgmR3SyncPTResolveConflict(pVM, pMapping, pPDSrc, GCPtrPage & (GST_PD_MASK << GST_PD_SHIFT));
#   elif PGM_GST_TYPE == PGM_TYPE_PAE
        rc = pgmR3SyncPTResolveConflictPAE(pVM, pMapping, GCPtrPage & (GST_PD_MASK << GST_PD_SHIFT));
#   else
        AssertFailed(); NOREF(pMapping); /* can't happen for amd64 */
#   endif
        if (RT_FAILURE(rc))
        {
            STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);
            return rc;
        }
        PdeDst = *pPdeDst;
#  endif /* IN_RING3 */
    }
# endif /* !PGM_WITHOUT_MAPPINGS */
    Assert(!PdeDst.n.u1Present); /* We're only supposed to call SyncPT on PDE!P and conflicts.*/

    /*
     * Sync the page directory entry.
     */
    GSTPDE      PdeSrc = pPDSrc->a[iPDSrc];
    const bool  fPageTable = !PdeSrc.b.u1Size || !GST_IS_PSE_ACTIVE(pVCpu);
    if (   PdeSrc.n.u1Present
        && (fPageTable ? GST_IS_PDE_VALID(pVCpu, PdeSrc) : GST_IS_BIG_PDE_VALID(pVCpu, PdeSrc)) )
    {
        /*
         * Allocate & map the page table.
         */
        PSHWPT          pPTDst;
        PPGMPOOLPAGE    pShwPage;
        RTGCPHYS        GCPhys;
        if (fPageTable)
        {
            GCPhys = GST_GET_PDE_GCPHYS(PdeSrc);
# if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
            /* Select the right PDE as we're emulating a 4kb page table with 2 shadow page tables. */
            GCPhys = PGM_A20_APPLY(pVCpu, GCPhys | ((iPDDst & 1) * (PAGE_SIZE / 2)));
# endif
            rc = pgmPoolAlloc(pVM, GCPhys, BTH_PGMPOOLKIND_PT_FOR_PT, PGMPOOLACCESS_DONTCARE, PGM_A20_IS_ENABLED(pVCpu),
                              pShwPde->idx, iPDDst, false /*fLockPage*/,
                              &pShwPage);
        }
        else
        {
            PGMPOOLACCESS enmAccess;
# if PGM_WITH_NX(PGM_GST_TYPE, PGM_SHW_TYPE)
            const bool  fNoExecute = PdeSrc.n.u1NoExecute && GST_IS_NX_ACTIVE(pVCpu);
# else
            const bool  fNoExecute = false;
# endif

            GCPhys = GST_GET_BIG_PDE_GCPHYS(pVM, PdeSrc);
# if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
            /* Select the right PDE as we're emulating a 4MB page directory with two 2 MB shadow PDEs.*/
            GCPhys = PGM_A20_APPLY(pVCpu, GCPhys | (GCPtrPage & (1 << X86_PD_PAE_SHIFT)));
# endif
            /* Determine the right kind of large page to avoid incorrect cached entry reuse. */
            if (PdeSrc.n.u1User)
            {
                if (PdeSrc.n.u1Write)
                    enmAccess = (fNoExecute) ? PGMPOOLACCESS_USER_RW_NX : PGMPOOLACCESS_USER_RW;
                else
                    enmAccess = (fNoExecute) ? PGMPOOLACCESS_USER_R_NX  : PGMPOOLACCESS_USER_R;
            }
            else
            {
                if (PdeSrc.n.u1Write)
                    enmAccess = (fNoExecute) ? PGMPOOLACCESS_SUPERVISOR_RW_NX : PGMPOOLACCESS_SUPERVISOR_RW;
                else
                    enmAccess = (fNoExecute) ? PGMPOOLACCESS_SUPERVISOR_R_NX  : PGMPOOLACCESS_SUPERVISOR_R;
            }
            rc = pgmPoolAlloc(pVM, GCPhys, BTH_PGMPOOLKIND_PT_FOR_BIG, enmAccess, PGM_A20_IS_ENABLED(pVCpu),
                              pShwPde->idx, iPDDst, false /*fLockPage*/,
                              &pShwPage);
        }
        if (rc == VINF_SUCCESS)
            pPTDst = (PSHWPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPage);
        else if (rc == VINF_PGM_CACHED_PAGE)
        {
            /*
             * The PT was cached, just hook it up.
             */
            if (fPageTable)
                PdeDst.u = pShwPage->Core.Key | GST_GET_PDE_SHW_FLAGS(pVCpu, PdeSrc);
            else
            {
                PdeDst.u = pShwPage->Core.Key | GST_GET_BIG_PDE_SHW_FLAGS(pVCpu, PdeSrc);
                /* (see explanation and assumptions further down.) */
                if (    !PdeSrc.b.u1Dirty
                    &&  PdeSrc.b.u1Write)
                {
                    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageBig));
                    PdeDst.u |= PGM_PDFLAGS_TRACK_DIRTY;
                    PdeDst.b.u1Write = 0;
                }
            }
            ASMAtomicWriteSize(pPdeDst, PdeDst.u);
            PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);
            return VINF_SUCCESS;
        }
        else if (rc == VERR_PGM_POOL_FLUSHED)
        {
            VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
            PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);
            return VINF_PGM_SYNC_CR3;
        }
        else
            AssertMsgFailedReturn(("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS);
        /** @todo Why do we bother preserving X86_PDE_AVL_MASK here?
         * Both PGM_PDFLAGS_MAPPING and PGM_PDFLAGS_TRACK_DIRTY should be
         * irrelevant at this point. */
        PdeDst.u &= X86_PDE_AVL_MASK;
        PdeDst.u |= pShwPage->Core.Key;

        /*
         * Page directory has been accessed (this is a fault situation, remember).
         */
        /** @todo
         * Well, when the caller is PrefetchPage or InvalidatePage is isn't a
         * fault situation.  What's more, the Trap0eHandler has already set the
         * accessed bit.  So, it's actually just VerifyAccessSyncPage which
         * might need setting the accessed flag.
         *
         * The best idea is to leave this change to the caller and add an
         * assertion that it's set already. */
        pPDSrc->a[iPDSrc].n.u1Accessed = 1;
        if (fPageTable)
        {
            /*
             * Page table - 4KB.
             *
             * Sync all or just a few entries depending on PGM_SYNC_N_PAGES.
             */
            Log2(("SyncPT:   4K  %RGv PdeSrc:{P=%d RW=%d U=%d raw=%08llx}\n",
                  GCPtrPage, PdeSrc.b.u1Present, PdeSrc.b.u1Write, PdeSrc.b.u1User, (uint64_t)PdeSrc.u));
            PGSTPT pPTSrc;
            rc = PGM_GCPHYS_2_PTR(pVM, GST_GET_PDE_GCPHYS(PdeSrc), &pPTSrc);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Start by syncing the page directory entry so CSAM's TLB trick works.
                 */
                PdeDst.u = (PdeDst.u & (SHW_PDE_PG_MASK | X86_PDE_AVL_MASK))
                         | GST_GET_PDE_SHW_FLAGS(pVCpu, PdeSrc);
                ASMAtomicWriteSize(pPdeDst, PdeDst.u);
                PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);

                /*
                 * Directory/page user or supervisor privilege: (same goes for read/write)
                 *
                 * Directory    Page    Combined
                 * U/S          U/S     U/S
                 *  0            0       0
                 *  0            1       0
                 *  1            0       0
                 *  1            1       1
                 *
                 * Simple AND operation. Table listed for completeness.
                 *
                 */
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT4K));
# ifdef PGM_SYNC_N_PAGES
                unsigned        iPTBase   = (GCPtrPage >> SHW_PT_SHIFT) & SHW_PT_MASK;
                unsigned        iPTDst    = iPTBase;
                const unsigned  iPTDstEnd = RT_MIN(iPTDst + PGM_SYNC_NR_PAGES / 2, RT_ELEMENTS(pPTDst->a));
                if (iPTDst <= PGM_SYNC_NR_PAGES / 2)
                    iPTDst = 0;
                else
                    iPTDst -= PGM_SYNC_NR_PAGES / 2;
# else /* !PGM_SYNC_N_PAGES */
                unsigned        iPTDst    = 0;
                const unsigned  iPTDstEnd = RT_ELEMENTS(pPTDst->a);
# endif /* !PGM_SYNC_N_PAGES */
                RTGCPTR         GCPtrCur  = (GCPtrPage & ~(RTGCPTR)((1 << SHW_PD_SHIFT) - 1))
                                          | ((RTGCPTR)iPTDst << PAGE_SHIFT);
# if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
                /* Select the right PDE as we're emulating a 4kb page table with 2 shadow page tables. */
                const unsigned  offPTSrc  = ((GCPtrPage >> SHW_PD_SHIFT) & 1) * 512;
# else
                const unsigned  offPTSrc  = 0;
# endif
                for (; iPTDst < iPTDstEnd; iPTDst++, GCPtrCur += PAGE_SIZE)
                {
                    const unsigned iPTSrc = iPTDst + offPTSrc;
                    const GSTPTE   PteSrc = pPTSrc->a[iPTSrc];

                    if (PteSrc.n.u1Present)
                    {
# ifdef VBOX_WITH_RAW_MODE_NOT_R0
                        /*
                         * Assuming kernel code will be marked as supervisor - and not as user level
                         * and executed using a conforming code selector - And marked as readonly.
                         * Also assume that if we're monitoring a page, it's of no interest to CSAM.
                         */
                        PPGMPAGE pPage;
                        if (    ((PdeSrc.u & pPTSrc->a[iPTSrc].u) & (X86_PTE_RW | X86_PTE_US))
                            ||  !CSAMDoesPageNeedScanning(pVM, GCPtrCur)
                            ||  (   (pPage = pgmPhysGetPage(pVM, GST_GET_PTE_GCPHYS(PteSrc)))
                                 &&  PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
                           )
# endif
                            PGM_BTH_NAME(SyncPageWorker)(pVCpu, &pPTDst->a[iPTDst], PdeSrc, PteSrc, pShwPage, iPTDst);
                        Log2(("SyncPT:   4K+ %RGv PteSrc:{P=%d RW=%d U=%d raw=%08llx}%s dst.raw=%08llx iPTSrc=%x PdeSrc.u=%x physpte=%RGp\n",
                              GCPtrCur,
                              PteSrc.n.u1Present,
                              PteSrc.n.u1Write & PdeSrc.n.u1Write,
                              PteSrc.n.u1User  & PdeSrc.n.u1User,
                              (uint64_t)PteSrc.u,
                              SHW_PTE_IS_TRACK_DIRTY(pPTDst->a[iPTDst]) ? " Track-Dirty" : "", SHW_PTE_LOG64(pPTDst->a[iPTDst]), iPTSrc, PdeSrc.au32[0],
                              (RTGCPHYS)(GST_GET_PDE_GCPHYS(PdeSrc) + iPTSrc*sizeof(PteSrc)) ));
                    }
                    /* else: the page table was cleared by the pool */
                } /* for PTEs */
            }
        }
        else
        {
            /*
             * Big page - 2/4MB.
             *
             * We'll walk the ram range list in parallel and optimize lookups.
             * We will only sync one shadow page table at a time.
             */
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT4M));

            /**
             * @todo It might be more efficient to sync only a part of the 4MB
             *       page (similar to what we do for 4KB PDs).
             */

            /*
             * Start by syncing the page directory entry.
             */
            PdeDst.u = (PdeDst.u & (SHW_PDE_PG_MASK | (X86_PDE_AVL_MASK & ~PGM_PDFLAGS_TRACK_DIRTY)))
                     | GST_GET_BIG_PDE_SHW_FLAGS(pVCpu, PdeSrc);

            /*
             * If the page is not flagged as dirty and is writable, then make it read-only
             * at PD level, so we can set the dirty bit when the page is modified.
             *
             * ASSUMES that page access handlers are implemented on page table entry level.
             *      Thus we will first catch the dirty access and set PDE.D and restart. If
             *      there is an access handler, we'll trap again and let it work on the problem.
             */
            /** @todo move the above stuff to a section in the PGM documentation. */
            Assert(!(PdeDst.u & PGM_PDFLAGS_TRACK_DIRTY));
            if (    !PdeSrc.b.u1Dirty
                &&  PdeSrc.b.u1Write)
            {
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,DirtyPageBig));
                PdeDst.u |= PGM_PDFLAGS_TRACK_DIRTY;
                PdeDst.b.u1Write = 0;
            }
            ASMAtomicWriteSize(pPdeDst, PdeDst.u);
            PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);

            /*
             * Fill the shadow page table.
             */
            /* Get address and flags from the source PDE. */
            SHWPTE PteDstBase;
            SHW_PTE_SET(PteDstBase, GST_GET_BIG_PDE_SHW_FLAGS_4_PTE(pVCpu, PdeSrc));

            /* Loop thru the entries in the shadow PT. */
            const RTGCPTR   GCPtr  = (GCPtrPage >> SHW_PD_SHIFT) << SHW_PD_SHIFT; NOREF(GCPtr);
            Log2(("SyncPT:   BIG %RGv PdeSrc:{P=%d RW=%d U=%d raw=%08llx} Shw=%RGv GCPhys=%RGp %s\n",
                  GCPtrPage, PdeSrc.b.u1Present, PdeSrc.b.u1Write, PdeSrc.b.u1User, (uint64_t)PdeSrc.u, GCPtr,
                  GCPhys, PdeDst.u & PGM_PDFLAGS_TRACK_DIRTY ? " Track-Dirty" : ""));
            PPGMRAMRANGE    pRam   = pgmPhysGetRangeAtOrAbove(pVM, GCPhys);
            unsigned        iPTDst = 0;
            while (     iPTDst < RT_ELEMENTS(pPTDst->a)
                   &&   !VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY))
            {
                if (pRam && GCPhys >= pRam->GCPhys)
                {
# ifndef PGM_WITH_A20
                    unsigned iHCPage = (GCPhys - pRam->GCPhys) >> PAGE_SHIFT;
# endif
                    do
                    {
                        /* Make shadow PTE. */
# ifdef PGM_WITH_A20
                        PPGMPAGE    pPage = &pRam->aPages[(GCPhys - pRam->GCPhys) >> PAGE_SHIFT];
# else
                        PPGMPAGE    pPage = &pRam->aPages[iHCPage];
# endif
                        SHWPTE      PteDst;

# ifndef VBOX_WITH_NEW_LAZY_PAGE_ALLOC
                        /* Try to make the page writable if necessary. */
                        if (    PGM_PAGE_GET_TYPE(pPage)  == PGMPAGETYPE_RAM
                            &&  (   PGM_PAGE_IS_ZERO(pPage)
                                 || (   SHW_PTE_IS_RW(PteDstBase)
                                     && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED
#  ifdef VBOX_WITH_REAL_WRITE_MONITORED_PAGES
                                     && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_WRITE_MONITORED
#  endif
#  ifdef VBOX_WITH_PAGE_SHARING
                                     && PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_SHARED
#  endif
                                     && !PGM_PAGE_IS_BALLOONED(pPage))
                                 )
                           )
                        {
                            rc = pgmPhysPageMakeWritable(pVM, pPage, GCPhys);
                            AssertRCReturn(rc, rc);
                            if (VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY))
                                break;
                        }
# endif

                        if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPage))
                            PGM_BTH_NAME(SyncHandlerPte)(pVM, pPage, SHW_PTE_GET_U(PteDstBase), &PteDst);
                        else if (PGM_PAGE_IS_BALLOONED(pPage))
                            SHW_PTE_SET(PteDst, 0); /* Handle ballooned pages at #PF time. */
# ifdef VBOX_WITH_RAW_MODE_NOT_R0
                        /*
                         * Assuming kernel code will be marked as supervisor and not as user level and executed
                         * using a conforming code selector. Don't check for readonly, as that implies the whole
                         * 4MB can be code or readonly data. Linux enables write access for its large pages.
                         */
                        else if (    !PdeSrc.n.u1User
                                 &&  CSAMDoesPageNeedScanning(pVM, GCPtr | (iPTDst << SHW_PT_SHIFT)))
                            SHW_PTE_SET(PteDst, 0);
# endif
                        else
                            SHW_PTE_SET(PteDst, PGM_PAGE_GET_HCPHYS(pPage) | SHW_PTE_GET_U(PteDstBase));

                        /* Only map writable pages writable. */
                        if (    SHW_PTE_IS_P_RW(PteDst)
                            &&  PGM_PAGE_GET_STATE(pPage) != PGM_PAGE_STATE_ALLOCATED)
                        {
                            /* Still applies to shared pages. */
                            Assert(!PGM_PAGE_IS_ZERO(pPage));
                            SHW_PTE_SET_RO(PteDst);   /** @todo this isn't quite working yet... */
                            Log3(("SyncPT: write-protecting %RGp pPage=%R[pgmpage] at %RGv\n", GCPhys, pPage, (RTGCPTR)(GCPtr | (iPTDst << SHW_PT_SHIFT))));
                        }

                        if (SHW_PTE_IS_P(PteDst))
                            PGM_BTH_NAME(SyncPageWorkerTrackAddref)(pVCpu, pShwPage, PGM_PAGE_GET_TRACKING(pPage), pPage, iPTDst);

                        /* commit it (not atomic, new table) */
                        pPTDst->a[iPTDst] = PteDst;
                        Log4(("SyncPT: BIG %RGv PteDst:{P=%d RW=%d U=%d raw=%08llx}%s\n",
                              (RTGCPTR)(GCPtr | (iPTDst << SHW_PT_SHIFT)), SHW_PTE_IS_P(PteDst), SHW_PTE_IS_RW(PteDst), SHW_PTE_IS_US(PteDst), SHW_PTE_LOG64(PteDst),
                              SHW_PTE_IS_TRACK_DIRTY(PteDst) ? " Track-Dirty" : ""));

                        /* advance */
                        GCPhys += PAGE_SIZE;
                        PGM_A20_APPLY_TO_VAR(pVCpu, GCPhys);
# ifndef PGM_WITH_A20
                        iHCPage++;
# endif
                        iPTDst++;
                    } while (   iPTDst < RT_ELEMENTS(pPTDst->a)
                             && GCPhys <= pRam->GCPhysLast);

                    /* Advance ram range list. */
                    while (pRam && GCPhys > pRam->GCPhysLast)
                        pRam = pRam->CTX_SUFF(pNext);
                }
                else if (pRam)
                {
                    Log(("Invalid pages at %RGp\n", GCPhys));
                    do
                    {
                        SHW_PTE_SET(pPTDst->a[iPTDst], 0); /* Invalid page, we must handle them manually. */
                        GCPhys += PAGE_SIZE;
                        iPTDst++;
                    } while (   iPTDst < RT_ELEMENTS(pPTDst->a)
                             && GCPhys < pRam->GCPhys);
                    PGM_A20_APPLY_TO_VAR(pVCpu,GCPhys);
                }
                else
                {
                    Log(("Invalid pages at %RGp (2)\n", GCPhys));
                    for ( ; iPTDst < RT_ELEMENTS(pPTDst->a); iPTDst++)
                        SHW_PTE_SET(pPTDst->a[iPTDst], 0); /* Invalid page, we must handle them manually. */
                }
            } /* while more PTEs */
        } /* 4KB / 4MB */
    }
    else
        AssertRelease(!PdeDst.n.u1Present);

    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);
    if (RT_FAILURE(rc))
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPTFailed));
    return rc;

#elif (PGM_GST_TYPE == PGM_TYPE_REAL || PGM_GST_TYPE == PGM_TYPE_PROT) \
    && PGM_SHW_TYPE != PGM_TYPE_NESTED \
    && (PGM_SHW_TYPE != PGM_TYPE_EPT || PGM_GST_TYPE == PGM_TYPE_PROT) \
    && !defined(IN_RC)
    NOREF(iPDSrc); NOREF(pPDSrc);

    STAM_PROFILE_START(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);

    /*
     * Validate input a little bit.
     */
    int             rc = VINF_SUCCESS;
# if PGM_SHW_TYPE == PGM_TYPE_32BIT
    const unsigned  iPDDst  = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PSHWPDE         pPdeDst = pgmShwGet32BitPDEPtr(pVCpu, GCPtrPage);

    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE    pShwPde = pVCpu->pgm.s.CTX_SUFF(pShwPageCR3);
    Assert(pShwPde);

# elif PGM_SHW_TYPE == PGM_TYPE_PAE
    const unsigned  iPDDst  = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PPGMPOOLPAGE    pShwPde = NULL;             /* initialized to shut up gcc */
    PX86PDPAE       pPDDst;
    PSHWPDE         pPdeDst;

    /* Fetch the pgm pool shadow descriptor. */
    rc = pgmShwGetPaePoolPagePD(pVCpu, GCPtrPage, &pShwPde);
    AssertRCSuccessReturn(rc, rc);
    Assert(pShwPde);

    pPDDst  = (PX86PDPAE)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPde);
    pPdeDst = &pPDDst->a[iPDDst];

# elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    const unsigned  iPdpt   = (GCPtrPage >> X86_PDPT_SHIFT) & X86_PDPT_MASK_AMD64;
    const unsigned  iPDDst  = (GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK;
    PX86PDPAE       pPDDst  = NULL;             /* initialized to shut up gcc */
    PX86PDPT        pPdptDst= NULL;             /* initialized to shut up gcc */
    rc = pgmShwGetLongModePDPtr(pVCpu, GCPtrPage, NULL, &pPdptDst, &pPDDst);
    AssertRCSuccessReturn(rc, rc);
    Assert(pPDDst);
    PSHWPDE         pPdeDst = &pPDDst->a[iPDDst];

    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE    pShwPde = pgmPoolGetPage(pPool, pPdptDst->a[iPdpt].u & X86_PDPE_PG_MASK);
    Assert(pShwPde);

# elif PGM_SHW_TYPE == PGM_TYPE_EPT
    const unsigned  iPdpt   = (GCPtrPage >> EPT_PDPT_SHIFT) & EPT_PDPT_MASK;
    const unsigned  iPDDst  = ((GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK);
    PEPTPD          pPDDst;
    PEPTPDPT        pPdptDst;

    rc = pgmShwGetEPTPDPtr(pVCpu, GCPtrPage, &pPdptDst, &pPDDst);
    if (rc != VINF_SUCCESS)
    {
        STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);
        AssertRC(rc);
        return rc;
    }
    Assert(pPDDst);
    PSHWPDE         pPdeDst = &pPDDst->a[iPDDst];

    /* Fetch the pgm pool shadow descriptor. */
    PPGMPOOLPAGE pShwPde = pgmPoolGetPage(pPool, pPdptDst->a[iPdpt].u & EPT_PDPTE_PG_MASK);
    Assert(pShwPde);
# endif
    SHWPDE          PdeDst = *pPdeDst;

    Assert(!(PdeDst.u & PGM_PDFLAGS_MAPPING));
    Assert(!PdeDst.n.u1Present); /* We're only supposed to call SyncPT on PDE!P and conflicts.*/

# if defined(PGM_WITH_LARGE_PAGES) && PGM_SHW_TYPE != PGM_TYPE_32BIT && PGM_SHW_TYPE != PGM_TYPE_PAE
    if (BTH_IS_NP_ACTIVE(pVM))
    {
        /* Check if we allocated a big page before for this 2 MB range. */
        PPGMPAGE pPage;
        rc = pgmPhysGetPageEx(pVM, PGM_A20_APPLY(pVCpu, GCPtrPage & X86_PDE2M_PAE_PG_MASK), &pPage);
        if (RT_SUCCESS(rc))
        {
            RTHCPHYS HCPhys = NIL_RTHCPHYS;
            if (PGM_PAGE_GET_PDE_TYPE(pPage) == PGM_PAGE_PDE_TYPE_PDE)
            {
                if (PGM_A20_IS_ENABLED(pVCpu))
                {
                    STAM_REL_COUNTER_INC(&pVM->pgm.s.StatLargePageReused);
                    AssertRelease(PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED);
                    HCPhys = PGM_PAGE_GET_HCPHYS(pPage);
                }
                else
                {
                    PGM_PAGE_SET_PDE_TYPE(pVM, pPage, PGM_PAGE_PDE_TYPE_PDE_DISABLED);
                    pVM->pgm.s.cLargePagesDisabled++;
                }
            }
            else if (   PGM_PAGE_GET_PDE_TYPE(pPage) == PGM_PAGE_PDE_TYPE_PDE_DISABLED
                     && PGM_A20_IS_ENABLED(pVCpu))
            {
                /* Recheck the entire 2 MB range to see if we can use it again as a large page. */
                rc = pgmPhysRecheckLargePage(pVM, GCPtrPage, pPage);
                if (RT_SUCCESS(rc))
                {
                    Assert(PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED);
                    Assert(PGM_PAGE_GET_PDE_TYPE(pPage) == PGM_PAGE_PDE_TYPE_PDE);
                    HCPhys = PGM_PAGE_GET_HCPHYS(pPage);
                }
            }
            else if (   PGMIsUsingLargePages(pVM)
                     && PGM_A20_IS_ENABLED(pVCpu))
            {
                rc = pgmPhysAllocLargePage(pVM, GCPtrPage);
                if (RT_SUCCESS(rc))
                {
                    Assert(PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED);
                    Assert(PGM_PAGE_GET_PDE_TYPE(pPage) == PGM_PAGE_PDE_TYPE_PDE);
                    HCPhys = PGM_PAGE_GET_HCPHYS(pPage);
                }
                else
                    LogFlow(("pgmPhysAllocLargePage failed with %Rrc\n", rc));
            }

            if (HCPhys != NIL_RTHCPHYS)
            {
                PdeDst.u &= X86_PDE_AVL_MASK;
                PdeDst.u |= HCPhys;
                PdeDst.n.u1Present   = 1;
                PdeDst.n.u1Write     = 1;
                PdeDst.b.u1Size      = 1;
#  if  PGM_SHW_TYPE == PGM_TYPE_EPT
                PdeDst.n.u1Execute   = 1;
                PdeDst.b.u1IgnorePAT = 1;
                PdeDst.b.u3EMT       = VMX_EPT_MEMTYPE_WB;
#  else
                PdeDst.n.u1User      = 1;
#  endif
                ASMAtomicWriteSize(pPdeDst, PdeDst.u);

                Log(("SyncPT: Use large page at %RGp PDE=%RX64\n", GCPtrPage, PdeDst.u));
                /* Add a reference to the first page only. */
                PGM_BTH_NAME(SyncPageWorkerTrackAddref)(pVCpu, pShwPde, PGM_PAGE_GET_TRACKING(pPage), pPage, iPDDst);

                STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);
                return VINF_SUCCESS;
            }
        }
    }
# endif /* HC_ARCH_BITS == 64 */

    /*
     * Allocate & map the page table.
     */
    PSHWPT          pPTDst;
    PPGMPOOLPAGE    pShwPage;
    RTGCPHYS        GCPhys;

    /* Virtual address = physical address */
    GCPhys = PGM_A20_APPLY(pVCpu, GCPtrPage & X86_PAGE_4K_BASE_MASK);
    rc = pgmPoolAlloc(pVM, GCPhys & ~(RT_BIT_64(SHW_PD_SHIFT) - 1), BTH_PGMPOOLKIND_PT_FOR_PT, PGMPOOLACCESS_DONTCARE,
                      PGM_A20_IS_ENABLED(pVCpu), pShwPde->idx, iPDDst, false /*fLockPage*/,
                      &pShwPage);
    if (    rc == VINF_SUCCESS
        ||  rc == VINF_PGM_CACHED_PAGE)
        pPTDst = (PSHWPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPage);
    else
    {
       STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);
       AssertMsgFailedReturn(("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS);
    }

    if (rc == VINF_SUCCESS)
    {
        /* New page table; fully set it up. */
        Assert(pPTDst);

        /* Mask away the page offset. */
        GCPtrPage &= ~(RTGCPTR)PAGE_OFFSET_MASK;

        for (unsigned iPTDst = 0; iPTDst < RT_ELEMENTS(pPTDst->a); iPTDst++)
        {
            RTGCPTR GCPtrCurPage = PGM_A20_APPLY(pVCpu, (GCPtrPage & ~(RTGCPTR)(SHW_PT_MASK << SHW_PT_SHIFT))
                                                      | (iPTDst << PAGE_SHIFT));

            PGM_BTH_NAME(SyncPageWorker)(pVCpu, &pPTDst->a[iPTDst], GCPtrCurPage, pShwPage, iPTDst);
            Log2(("SyncPage: 4K+ %RGv PteSrc:{P=1 RW=1 U=1} PteDst=%08llx%s\n",
                  GCPtrCurPage,
                  SHW_PTE_LOG64(pPTDst->a[iPTDst]),
                  SHW_PTE_IS_TRACK_DIRTY(pPTDst->a[iPTDst]) ? " Track-Dirty" : ""));

            if (RT_UNLIKELY(VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY)))
                break;
        }
    }
    else
        rc = VINF_SUCCESS; /* Cached entry; assume it's still fully valid. */

    /* Save the new PDE. */
    PdeDst.u &= X86_PDE_AVL_MASK;
    PdeDst.u |= pShwPage->Core.Key;
    PdeDst.n.u1Present  = 1;
    PdeDst.n.u1Write    = 1;
# if PGM_SHW_TYPE == PGM_TYPE_EPT
    PdeDst.n.u1Execute  = 1;
# else
    PdeDst.n.u1User     = 1;
    PdeDst.n.u1Accessed = 1;
# endif
    ASMAtomicWriteSize(pPdeDst, PdeDst.u);

    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPT), a);
    if (RT_FAILURE(rc))
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncPTFailed));
    return rc;

#else
    NOREF(iPDSrc); NOREF(pPDSrc);
    AssertReleaseMsgFailed(("Shw=%d Gst=%d is not implemented!\n", PGM_SHW_TYPE, PGM_GST_TYPE));
    return VERR_PGM_NOT_USED_IN_MODE;
#endif
}



/**
 * Prefetch a page/set of pages.
 *
 * Typically used to sync commonly used pages before entering raw mode
 * after a CR3 reload.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtrPage   Page to invalidate.
 */
PGM_BTH_DECL(int, PrefetchPage)(PVMCPU pVCpu, RTGCPTR GCPtrPage)
{
#if (   PGM_GST_TYPE == PGM_TYPE_32BIT \
     || PGM_GST_TYPE == PGM_TYPE_REAL \
     || PGM_GST_TYPE == PGM_TYPE_PROT \
     || PGM_GST_TYPE == PGM_TYPE_PAE \
     || PGM_GST_TYPE == PGM_TYPE_AMD64 ) \
 && PGM_SHW_TYPE != PGM_TYPE_NESTED \
 && PGM_SHW_TYPE != PGM_TYPE_EPT

    /*
     * Check that all Guest levels thru the PDE are present, getting the
     * PD and PDE in the processes.
     */
    int             rc     = VINF_SUCCESS;
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
#  if PGM_GST_TYPE == PGM_TYPE_32BIT
    const unsigned  iPDSrc = (uint32_t)GCPtrPage >> GST_PD_SHIFT;
    PGSTPD          pPDSrc = pgmGstGet32bitPDPtr(pVCpu);
#  elif PGM_GST_TYPE == PGM_TYPE_PAE
    unsigned        iPDSrc;
    X86PDPE         PdpeSrc;
    PGSTPD          pPDSrc = pgmGstGetPaePDPtr(pVCpu, GCPtrPage, &iPDSrc, &PdpeSrc);
    if (!pPDSrc)
        return VINF_SUCCESS; /* not present */
#  elif PGM_GST_TYPE == PGM_TYPE_AMD64
    unsigned        iPDSrc;
    PX86PML4E       pPml4eSrc;
    X86PDPE         PdpeSrc;
    PGSTPD          pPDSrc = pgmGstGetLongModePDPtr(pVCpu, GCPtrPage, &pPml4eSrc, &PdpeSrc, &iPDSrc);
    if (!pPDSrc)
        return VINF_SUCCESS; /* not present */
#  endif
    const GSTPDE    PdeSrc = pPDSrc->a[iPDSrc];
# else
    PGSTPD          pPDSrc = NULL;
    const unsigned  iPDSrc = 0;
    GSTPDE          PdeSrc;

    PdeSrc.u            = 0; /* faked so we don't have to #ifdef everything */
    PdeSrc.n.u1Present  = 1;
    PdeSrc.n.u1Write    = 1;
    PdeSrc.n.u1Accessed = 1;
    PdeSrc.n.u1User     = 1;
# endif

    if (PdeSrc.n.u1Present && PdeSrc.n.u1Accessed)
    {
        PVM pVM = pVCpu->CTX_SUFF(pVM);
        pgmLock(pVM);

# if PGM_SHW_TYPE == PGM_TYPE_32BIT
        const X86PDE    PdeDst = pgmShwGet32BitPDE(pVCpu, GCPtrPage);
# elif PGM_SHW_TYPE == PGM_TYPE_PAE
        const unsigned  iPDDst = ((GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK);
        PX86PDPAE       pPDDst;
        X86PDEPAE       PdeDst;
#   if PGM_GST_TYPE != PGM_TYPE_PAE
        X86PDPE         PdpeSrc;

        /* Fake PDPT entry; access control handled on the page table level, so allow everything. */
        PdpeSrc.u  = X86_PDPE_P;   /* rw/us are reserved for PAE pdpte's; accessed bit causes invalid VT-x guest state errors */
#   endif
        rc = pgmShwSyncPaePDPtr(pVCpu, GCPtrPage, PdpeSrc.u, &pPDDst);
        if (rc != VINF_SUCCESS)
        {
            pgmUnlock(pVM);
            AssertRC(rc);
            return rc;
        }
        Assert(pPDDst);
        PdeDst = pPDDst->a[iPDDst];

# elif PGM_SHW_TYPE == PGM_TYPE_AMD64
        const unsigned  iPDDst = ((GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK);
        PX86PDPAE       pPDDst;
        X86PDEPAE       PdeDst;

#  if PGM_GST_TYPE == PGM_TYPE_PROT
        /* AMD-V nested paging */
        X86PML4E        Pml4eSrc;
        X86PDPE         PdpeSrc;
        PX86PML4E       pPml4eSrc = &Pml4eSrc;

        /* Fake PML4 & PDPT entry; access control handled on the page table level, so allow everything. */
        Pml4eSrc.u = X86_PML4E_P | X86_PML4E_RW | X86_PML4E_US | X86_PML4E_A;
        PdpeSrc.u  = X86_PDPE_P | X86_PDPE_RW | X86_PDPE_US | X86_PDPE_A;
#  endif

        rc = pgmShwSyncLongModePDPtr(pVCpu, GCPtrPage, pPml4eSrc->u, PdpeSrc.u, &pPDDst);
        if (rc != VINF_SUCCESS)
        {
            pgmUnlock(pVM);
            AssertRC(rc);
            return rc;
        }
        Assert(pPDDst);
        PdeDst = pPDDst->a[iPDDst];
# endif
        if (!(PdeDst.u & PGM_PDFLAGS_MAPPING))
        {
            if (!PdeDst.n.u1Present)
            {
                /** @todo r=bird: This guy will set the A bit on the PDE,
                 *    probably harmless. */
                rc = PGM_BTH_NAME(SyncPT)(pVCpu, iPDSrc, pPDSrc, GCPtrPage);
            }
            else
            {
                /* Note! We used to sync PGM_SYNC_NR_PAGES pages, which triggered assertions in CSAM, because
                 *       R/W attributes of nearby pages were reset. Not sure how that could happen. Anyway, it
                 *       makes no sense to prefetch more than one page.
                 */
                rc = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrc, GCPtrPage, 1, 0);
                if (RT_SUCCESS(rc))
                    rc = VINF_SUCCESS;
            }
        }
        pgmUnlock(pVM);
    }
    return rc;

#elif PGM_SHW_TYPE == PGM_TYPE_NESTED || PGM_SHW_TYPE == PGM_TYPE_EPT
    NOREF(pVCpu); NOREF(GCPtrPage);
    return VINF_SUCCESS; /* ignore */
#else
    AssertCompile(0);
#endif
}




/**
 * Syncs a page during a PGMVerifyAccess() call.
 *
 * @returns VBox status code (informational included).
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtrPage   The address of the page to sync.
 * @param   fPage       The effective guest page flags.
 * @param   uErr        The trap error code.
 * @remarks This will normally never be called on invalid guest page
 *          translation entries.
 */
PGM_BTH_DECL(int, VerifyAccessSyncPage)(PVMCPU pVCpu, RTGCPTR GCPtrPage, unsigned fPage, unsigned uErr)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM); NOREF(pVM);

    LogFlow(("VerifyAccessSyncPage: GCPtrPage=%RGv fPage=%#x uErr=%#x\n", GCPtrPage, fPage, uErr));
    RT_NOREF_PV(GCPtrPage); RT_NOREF_PV(fPage); RT_NOREF_PV(uErr);

    Assert(!pVM->pgm.s.fNestedPaging);
#if   (   PGM_GST_TYPE == PGM_TYPE_32BIT \
       || PGM_GST_TYPE == PGM_TYPE_REAL \
       || PGM_GST_TYPE == PGM_TYPE_PROT \
       || PGM_GST_TYPE == PGM_TYPE_PAE \
       || PGM_GST_TYPE == PGM_TYPE_AMD64 ) \
    && PGM_SHW_TYPE != PGM_TYPE_NESTED \
    && PGM_SHW_TYPE != PGM_TYPE_EPT

# ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!(fPage & X86_PTE_US))
    {
        /*
         * Mark this page as safe.
         */
        /** @todo not correct for pages that contain both code and data!! */
        Log(("CSAMMarkPage %RGv; scanned=%d\n", GCPtrPage, true));
        CSAMMarkPage(pVM, GCPtrPage, true);
    }
# endif

    /*
     * Get guest PD and index.
     */
    /** @todo Performance: We've done all this a jiffy ago in the
     *        PGMGstGetPage call. */
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
#  if PGM_GST_TYPE == PGM_TYPE_32BIT
    const unsigned  iPDSrc = (uint32_t)GCPtrPage >> GST_PD_SHIFT;
    PGSTPD          pPDSrc = pgmGstGet32bitPDPtr(pVCpu);

#  elif PGM_GST_TYPE == PGM_TYPE_PAE
    unsigned        iPDSrc = 0;
    X86PDPE         PdpeSrc;
    PGSTPD          pPDSrc = pgmGstGetPaePDPtr(pVCpu, GCPtrPage, &iPDSrc, &PdpeSrc);
    if (RT_UNLIKELY(!pPDSrc))
    {
        Log(("PGMVerifyAccess: access violation for %RGv due to non-present PDPTR\n", GCPtrPage));
        return VINF_EM_RAW_GUEST_TRAP;
    }

#  elif PGM_GST_TYPE == PGM_TYPE_AMD64
    unsigned        iPDSrc = 0;         /* shut up gcc */
    PX86PML4E       pPml4eSrc = NULL;   /* ditto */
    X86PDPE         PdpeSrc;
    PGSTPD          pPDSrc = pgmGstGetLongModePDPtr(pVCpu, GCPtrPage, &pPml4eSrc, &PdpeSrc, &iPDSrc);
    if (RT_UNLIKELY(!pPDSrc))
    {
        Log(("PGMVerifyAccess: access violation for %RGv due to non-present PDPTR\n", GCPtrPage));
        return VINF_EM_RAW_GUEST_TRAP;
    }
#  endif

# else  /* !PGM_WITH_PAGING */
    PGSTPD          pPDSrc = NULL;
    const unsigned  iPDSrc = 0;
# endif /* !PGM_WITH_PAGING */
    int             rc = VINF_SUCCESS;

    pgmLock(pVM);

    /*
     * First check if the shadow pd is present.
     */
# if PGM_SHW_TYPE == PGM_TYPE_32BIT
    PX86PDE         pPdeDst = pgmShwGet32BitPDEPtr(pVCpu, GCPtrPage);

# elif PGM_SHW_TYPE == PGM_TYPE_PAE
    PX86PDEPAE      pPdeDst;
    const unsigned  iPDDst = ((GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK);
    PX86PDPAE       pPDDst;
#   if PGM_GST_TYPE != PGM_TYPE_PAE
    /* Fake PDPT entry; access control handled on the page table level, so allow everything. */
    X86PDPE         PdpeSrc;
    PdpeSrc.u  = X86_PDPE_P;   /* rw/us are reserved for PAE pdpte's; accessed bit causes invalid VT-x guest state errors */
#   endif
    rc = pgmShwSyncPaePDPtr(pVCpu, GCPtrPage, PdpeSrc.u, &pPDDst);
    if (rc != VINF_SUCCESS)
    {
        pgmUnlock(pVM);
        AssertRC(rc);
        return rc;
    }
    Assert(pPDDst);
    pPdeDst = &pPDDst->a[iPDDst];

# elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    const unsigned  iPDDst = ((GCPtrPage >> SHW_PD_SHIFT) & SHW_PD_MASK);
    PX86PDPAE       pPDDst;
    PX86PDEPAE      pPdeDst;

#  if PGM_GST_TYPE == PGM_TYPE_PROT
    /* AMD-V nested paging: Fake PML4 & PDPT entry; access control handled on the page table level, so allow everything. */
    X86PML4E        Pml4eSrc;
    X86PDPE         PdpeSrc;
    PX86PML4E       pPml4eSrc = &Pml4eSrc;
    Pml4eSrc.u = X86_PML4E_P | X86_PML4E_RW | X86_PML4E_US | X86_PML4E_A;
    PdpeSrc.u  = X86_PDPE_P  | X86_PDPE_RW  | X86_PDPE_US  | X86_PDPE_A;
#  endif

    rc = pgmShwSyncLongModePDPtr(pVCpu, GCPtrPage, pPml4eSrc->u, PdpeSrc.u, &pPDDst);
    if (rc != VINF_SUCCESS)
    {
        pgmUnlock(pVM);
        AssertRC(rc);
        return rc;
    }
    Assert(pPDDst);
    pPdeDst = &pPDDst->a[iPDDst];
# endif

    if (!pPdeDst->n.u1Present)
    {
        rc = PGM_BTH_NAME(SyncPT)(pVCpu, iPDSrc, pPDSrc, GCPtrPage);
        if (rc != VINF_SUCCESS)
        {
            PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);
            pgmUnlock(pVM);
            AssertRC(rc);
            return rc;
        }
    }

# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    /* Check for dirty bit fault */
    rc = PGM_BTH_NAME(CheckDirtyPageFault)(pVCpu, uErr, pPdeDst, &pPDSrc->a[iPDSrc], GCPtrPage);
    if (rc == VINF_PGM_HANDLED_DIRTY_BIT_FAULT)
        Log(("PGMVerifyAccess: success (dirty)\n"));
    else
# endif
    {
# if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
        GSTPDE PdeSrc       = pPDSrc->a[iPDSrc];
# else
        GSTPDE PdeSrc;
        PdeSrc.u            = 0; /* faked so we don't have to #ifdef everything */
        PdeSrc.n.u1Present  = 1;
        PdeSrc.n.u1Write    = 1;
        PdeSrc.n.u1Accessed = 1;
        PdeSrc.n.u1User     = 1;
# endif

        Assert(rc != VINF_EM_RAW_GUEST_TRAP);
        if (uErr & X86_TRAP_PF_US)
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageOutOfSyncUser));
        else /* supervisor */
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageOutOfSyncSupervisor));

        rc = PGM_BTH_NAME(SyncPage)(pVCpu, PdeSrc, GCPtrPage, 1, 0);
        if (RT_SUCCESS(rc))
        {
            /* Page was successfully synced */
            Log2(("PGMVerifyAccess: success (sync)\n"));
            rc = VINF_SUCCESS;
        }
        else
        {
            Log(("PGMVerifyAccess: access violation for %RGv rc=%Rrc\n", GCPtrPage, rc));
            rc = VINF_EM_RAW_GUEST_TRAP;
        }
    }
    PGM_DYNMAP_UNUSED_HINT(pVCpu, pPdeDst);
    pgmUnlock(pVM);
    return rc;

#else  /* PGM_SHW_TYPE == PGM_TYPE_EPT || PGM_SHW_TYPE == PGM_TYPE_NESTED */

    AssertReleaseMsgFailed(("Shw=%d Gst=%d is not implemented!\n", PGM_GST_TYPE, PGM_SHW_TYPE));
    return VERR_PGM_NOT_USED_IN_MODE;
#endif /* PGM_SHW_TYPE == PGM_TYPE_EPT || PGM_SHW_TYPE == PGM_TYPE_NESTED */
}


/**
 * Syncs the paging hierarchy starting at CR3.
 *
 * @returns VBox status code, R0/RC may return VINF_PGM_SYNC_CR3, no other
 *          informational status codes.
 * @retval  VERR_PGM_NO_HYPERVISOR_ADDRESS in raw-mode when we're unable to map
 *          the VMM into guest context.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cr0         Guest context CR0 register.
 * @param   cr3         Guest context CR3 register. Not subjected to the A20
 *                      mask.
 * @param   cr4         Guest context CR4 register.
 * @param   fGlobal     Including global page directories or not
 */
PGM_BTH_DECL(int, SyncCR3)(PVMCPU pVCpu, uint64_t cr0, uint64_t cr3, uint64_t cr4, bool fGlobal)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM); NOREF(pVM);
    NOREF(cr0); NOREF(cr3); NOREF(cr4); NOREF(fGlobal);

    LogFlow(("SyncCR3 FF=%d fGlobal=%d\n", !!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3), fGlobal));

#if PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT

    pgmLock(pVM);

# ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    if (pPool->cDirtyPages)
        pgmPoolResetDirtyPages(pVM);
# endif

    /*
     * Update page access handlers.
     * The virtual are always flushed, while the physical are only on demand.
     * WARNING: We are incorrectly not doing global flushing on Virtual Handler updates. We'll
     *          have to look into that later because it will have a bad influence on the performance.
     * @note SvL: There's no need for that. Just invalidate the virtual range(s).
     *      bird: Yes, but that won't work for aliases.
     */
    /** @todo this MUST go away. See @bugref{1557}. */
    STAM_PROFILE_START(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncCR3Handlers), h);
    PGM_GST_NAME(HandlerVirtualUpdate)(pVM, cr4);
    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncCR3Handlers), h);
    pgmUnlock(pVM);
#endif /* !NESTED && !EPT */

#if PGM_SHW_TYPE == PGM_TYPE_NESTED || PGM_SHW_TYPE == PGM_TYPE_EPT
    /*
     * Nested / EPT - almost no work.
     */
    Assert(!pgmMapAreMappingsEnabled(pVM));
    return VINF_SUCCESS;

#elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    /*
     * AMD64 (Shw & Gst) - No need to check all paging levels; we zero
     * out the shadow parts when the guest modifies its tables.
     */
    Assert(!pgmMapAreMappingsEnabled(pVM));
    return VINF_SUCCESS;

#else /* PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT && PGM_SHW_TYPE != PGM_TYPE_AMD64 */

#  ifndef PGM_WITHOUT_MAPPINGS
    /*
     * Check for and resolve conflicts with our guest mappings if they
     * are enabled and not fixed.
     */
    if (pgmMapAreMappingsFloating(pVM))
    {
        int rc = pgmMapResolveConflicts(pVM);
        Assert(rc == VINF_SUCCESS || rc == VINF_PGM_SYNC_CR3);
        if (rc == VINF_SUCCESS)
        { /* likely */ }
        else if (rc == VINF_PGM_SYNC_CR3)
        {
            LogFlow(("SyncCR3: detected conflict -> VINF_PGM_SYNC_CR3\n"));
            return VINF_PGM_SYNC_CR3;
        }
        else if (RT_FAILURE(rc))
            return rc;
        else
            AssertMsgFailed(("%Rrc\n", rc));
    }
#  else
    Assert(!pgmMapAreMappingsEnabled(pVM));
#  endif
    return VINF_SUCCESS;
#endif /* PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT && PGM_SHW_TYPE != PGM_TYPE_AMD64 */
}




#ifdef VBOX_STRICT
# ifdef IN_RC
#  undef AssertMsgFailed
#  define AssertMsgFailed Log
# endif

/**
 * Checks that the shadow page table is in sync with the guest one.
 *
 * @returns The number of errors.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cr3         Guest context CR3 register.
 * @param   cr4         Guest context CR4 register.
 * @param   GCPtr       Where to start. Defaults to 0.
 * @param   cb          How much to check. Defaults to everything.
 */
PGM_BTH_DECL(unsigned, AssertCR3)(PVMCPU pVCpu, uint64_t cr3, uint64_t cr4, RTGCPTR GCPtr, RTGCPTR cb)
{
    NOREF(pVCpu); NOREF(cr3); NOREF(cr4); NOREF(GCPtr); NOREF(cb);
#if PGM_SHW_TYPE == PGM_TYPE_NESTED || PGM_SHW_TYPE == PGM_TYPE_EPT
    return 0;
#else
    unsigned cErrors = 0;
    PVM      pVM     = pVCpu->CTX_SUFF(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool); NOREF(pPool);

# if PGM_GST_TYPE == PGM_TYPE_PAE
    /** @todo currently broken; crashes below somewhere */
    AssertFailed();
# endif

# if   PGM_GST_TYPE == PGM_TYPE_32BIT \
    || PGM_GST_TYPE == PGM_TYPE_PAE \
    || PGM_GST_TYPE == PGM_TYPE_AMD64

    bool            fBigPagesSupported = GST_IS_PSE_ACTIVE(pVCpu);
    PPGMCPU         pPGM = &pVCpu->pgm.s;
    RTGCPHYS        GCPhysGst;              /* page address derived from the guest page tables. */
    RTHCPHYS        HCPhysShw;              /* page address derived from the shadow page tables. */
#  ifndef IN_RING0
    RTHCPHYS        HCPhys;                 /* general usage. */
#  endif
    int             rc;

    /*
     * Check that the Guest CR3 and all its mappings are correct.
     */
    AssertMsgReturn(pPGM->GCPhysCR3 == PGM_A20_APPLY(pVCpu, cr3 & GST_CR3_PAGE_MASK),
                    ("Invalid GCPhysCR3=%RGp cr3=%RGp\n", pPGM->GCPhysCR3, (RTGCPHYS)cr3),
                    false);
#  if !defined(IN_RING0) && PGM_GST_TYPE != PGM_TYPE_AMD64
#   if PGM_GST_TYPE == PGM_TYPE_32BIT
    rc = PGMShwGetPage(pVCpu, (RTRCUINTPTR)pPGM->pGst32BitPdRC, NULL, &HCPhysShw);
#   else
    rc = PGMShwGetPage(pVCpu, (RTRCUINTPTR)pPGM->pGstPaePdptRC, NULL, &HCPhysShw);
#   endif
    AssertRCReturn(rc, 1);
    HCPhys = NIL_RTHCPHYS;
    rc = pgmRamGCPhys2HCPhys(pVM, PGM_A20_APPLY(pVCpu, cr3 & GST_CR3_PAGE_MASK), &HCPhys);
    AssertMsgReturn(HCPhys == HCPhysShw, ("HCPhys=%RHp HCPhyswShw=%RHp (cr3)\n", HCPhys, HCPhysShw), false);
#   if PGM_GST_TYPE == PGM_TYPE_32BIT && defined(IN_RING3)
    pgmGstGet32bitPDPtr(pVCpu);
    RTGCPHYS GCPhys;
    rc = PGMR3DbgR3Ptr2GCPhys(pVM->pUVM, pPGM->pGst32BitPdR3, &GCPhys);
    AssertRCReturn(rc, 1);
    AssertMsgReturn(PGM_A20_APPLY(pVCpu, cr3 & GST_CR3_PAGE_MASK) == GCPhys, ("GCPhys=%RGp cr3=%RGp\n", GCPhys, (RTGCPHYS)cr3), false);
#   endif
#  endif /* !IN_RING0 */

    /*
     * Get and check the Shadow CR3.
     */
#  if PGM_SHW_TYPE == PGM_TYPE_32BIT
    unsigned        cPDEs       = X86_PG_ENTRIES;
    unsigned        cIncrement  = X86_PG_ENTRIES * PAGE_SIZE;
#  elif PGM_SHW_TYPE == PGM_TYPE_PAE
#   if PGM_GST_TYPE == PGM_TYPE_32BIT
    unsigned        cPDEs       = X86_PG_PAE_ENTRIES * 4;   /* treat it as a 2048 entry table. */
#   else
    unsigned        cPDEs       = X86_PG_PAE_ENTRIES;
#   endif
    unsigned        cIncrement  = X86_PG_PAE_ENTRIES * PAGE_SIZE;
#  elif PGM_SHW_TYPE == PGM_TYPE_AMD64
    unsigned        cPDEs       = X86_PG_PAE_ENTRIES;
    unsigned        cIncrement  = X86_PG_PAE_ENTRIES * PAGE_SIZE;
#  endif
    if (cb != ~(RTGCPTR)0)
        cPDEs = RT_MIN(cb >> SHW_PD_SHIFT, 1);

/** @todo call the other two PGMAssert*() functions. */

#  if PGM_GST_TYPE == PGM_TYPE_AMD64
    unsigned iPml4 = (GCPtr >> X86_PML4_SHIFT) & X86_PML4_MASK;

    for (; iPml4 < X86_PG_PAE_ENTRIES; iPml4++)
    {
        PPGMPOOLPAGE    pShwPdpt = NULL;
        PX86PML4E       pPml4eSrc;
        PX86PML4E       pPml4eDst;
        RTGCPHYS        GCPhysPdptSrc;

        pPml4eSrc     = pgmGstGetLongModePML4EPtr(pVCpu, iPml4);
        pPml4eDst     = pgmShwGetLongModePML4EPtr(pVCpu, iPml4);

        /* Fetch the pgm pool shadow descriptor if the shadow pml4e is present. */
        if (!pPml4eDst->n.u1Present)
        {
            GCPtr += _2M * UINT64_C(512) * UINT64_C(512);
            continue;
        }

        pShwPdpt = pgmPoolGetPage(pPool, pPml4eDst->u & X86_PML4E_PG_MASK);
        GCPhysPdptSrc = PGM_A20_APPLY(pVCpu, pPml4eSrc->u & X86_PML4E_PG_MASK);

        if (pPml4eSrc->n.u1Present != pPml4eDst->n.u1Present)
        {
            AssertMsgFailed(("Present bit doesn't match! pPml4eDst.u=%#RX64 pPml4eSrc.u=%RX64\n", pPml4eDst->u, pPml4eSrc->u));
            GCPtr += _2M * UINT64_C(512) * UINT64_C(512);
            cErrors++;
            continue;
        }

        if (GCPhysPdptSrc != pShwPdpt->GCPhys)
        {
            AssertMsgFailed(("Physical address doesn't match! iPml4 %d pPml4eDst.u=%#RX64 pPml4eSrc.u=%RX64 Phys %RX64 vs %RX64\n", iPml4, pPml4eDst->u, pPml4eSrc->u, pShwPdpt->GCPhys, GCPhysPdptSrc));
            GCPtr += _2M * UINT64_C(512) * UINT64_C(512);
            cErrors++;
            continue;
        }

        if (    pPml4eDst->n.u1User      != pPml4eSrc->n.u1User
            ||  pPml4eDst->n.u1Write     != pPml4eSrc->n.u1Write
            ||  pPml4eDst->n.u1NoExecute != pPml4eSrc->n.u1NoExecute)
        {
            AssertMsgFailed(("User/Write/NoExec bits don't match! pPml4eDst.u=%#RX64 pPml4eSrc.u=%RX64\n", pPml4eDst->u, pPml4eSrc->u));
            GCPtr += _2M * UINT64_C(512) * UINT64_C(512);
            cErrors++;
            continue;
        }
#  else  /* PGM_GST_TYPE != PGM_TYPE_AMD64 */
    {
#  endif /* PGM_GST_TYPE != PGM_TYPE_AMD64 */

#  if PGM_GST_TYPE == PGM_TYPE_AMD64 || PGM_GST_TYPE == PGM_TYPE_PAE
        /*
         * Check the PDPTEs too.
         */
        unsigned iPdpt = (GCPtr >> SHW_PDPT_SHIFT) & SHW_PDPT_MASK;

        for (;iPdpt <= SHW_PDPT_MASK; iPdpt++)
        {
            unsigned        iPDSrc  = 0;        /* initialized to shut up gcc */
            PPGMPOOLPAGE    pShwPde = NULL;
            PX86PDPE        pPdpeDst;
            RTGCPHYS        GCPhysPdeSrc;
            X86PDPE         PdpeSrc;
            PdpeSrc.u = 0;                      /* initialized to shut up gcc 4.5 */
#   if PGM_GST_TYPE == PGM_TYPE_PAE
            PGSTPD          pPDSrc    = pgmGstGetPaePDPtr(pVCpu, GCPtr, &iPDSrc, &PdpeSrc);
            PX86PDPT        pPdptDst  = pgmShwGetPaePDPTPtr(pVCpu);
#   else
            PX86PML4E       pPml4eSrcIgn;
            PX86PDPT        pPdptDst;
            PX86PDPAE       pPDDst;
            PGSTPD          pPDSrc    = pgmGstGetLongModePDPtr(pVCpu, GCPtr, &pPml4eSrcIgn, &PdpeSrc, &iPDSrc);

            rc = pgmShwGetLongModePDPtr(pVCpu, GCPtr, NULL, &pPdptDst, &pPDDst);
            if (rc != VINF_SUCCESS)
            {
                AssertMsg(rc == VERR_PAGE_DIRECTORY_PTR_NOT_PRESENT, ("Unexpected rc=%Rrc\n", rc));
                GCPtr += 512 * _2M;
                continue;   /* next PDPTE */
            }
            Assert(pPDDst);
#   endif
            Assert(iPDSrc == 0);

            pPdpeDst = &pPdptDst->a[iPdpt];

            if (!pPdpeDst->n.u1Present)
            {
                GCPtr += 512 * _2M;
                continue;   /* next PDPTE */
            }

            pShwPde      = pgmPoolGetPage(pPool, pPdpeDst->u & X86_PDPE_PG_MASK);
            GCPhysPdeSrc = PGM_A20_APPLY(pVCpu, PdpeSrc.u & X86_PDPE_PG_MASK);

            if (pPdpeDst->n.u1Present != PdpeSrc.n.u1Present)
            {
                AssertMsgFailed(("Present bit doesn't match! pPdpeDst.u=%#RX64 pPdpeSrc.u=%RX64\n", pPdpeDst->u, PdpeSrc.u));
                GCPtr += 512 * _2M;
                cErrors++;
                continue;
            }

            if (GCPhysPdeSrc != pShwPde->GCPhys)
            {
#   if PGM_GST_TYPE == PGM_TYPE_AMD64
                AssertMsgFailed(("Physical address doesn't match! iPml4 %d iPdpt %d pPdpeDst.u=%#RX64 pPdpeSrc.u=%RX64 Phys %RX64 vs %RX64\n", iPml4, iPdpt, pPdpeDst->u, PdpeSrc.u, pShwPde->GCPhys, GCPhysPdeSrc));
#   else
                AssertMsgFailed(("Physical address doesn't match! iPdpt %d pPdpeDst.u=%#RX64 pPdpeSrc.u=%RX64 Phys %RX64 vs %RX64\n", iPdpt, pPdpeDst->u, PdpeSrc.u, pShwPde->GCPhys, GCPhysPdeSrc));
#   endif
                GCPtr += 512 * _2M;
                cErrors++;
                continue;
            }

#   if PGM_GST_TYPE == PGM_TYPE_AMD64
            if (    pPdpeDst->lm.u1User      != PdpeSrc.lm.u1User
                ||  pPdpeDst->lm.u1Write     != PdpeSrc.lm.u1Write
                ||  pPdpeDst->lm.u1NoExecute != PdpeSrc.lm.u1NoExecute)
            {
                AssertMsgFailed(("User/Write/NoExec bits don't match! pPdpeDst.u=%#RX64 pPdpeSrc.u=%RX64\n", pPdpeDst->u, PdpeSrc.u));
                GCPtr += 512 * _2M;
                cErrors++;
                continue;
            }
#   endif

#  else  /* PGM_GST_TYPE != PGM_TYPE_AMD64 && PGM_GST_TYPE != PGM_TYPE_PAE */
        {
#  endif /* PGM_GST_TYPE != PGM_TYPE_AMD64 && PGM_GST_TYPE != PGM_TYPE_PAE */
#  if PGM_GST_TYPE == PGM_TYPE_32BIT
            GSTPD const    *pPDSrc = pgmGstGet32bitPDPtr(pVCpu);
#   if PGM_SHW_TYPE == PGM_TYPE_32BIT
            PCX86PD         pPDDst = pgmShwGet32BitPDPtr(pVCpu);
#   endif
#  endif /* PGM_GST_TYPE == PGM_TYPE_32BIT */
            /*
            * Iterate the shadow page directory.
            */
            GCPtr = (GCPtr >> SHW_PD_SHIFT) << SHW_PD_SHIFT;
            unsigned iPDDst = (GCPtr >> SHW_PD_SHIFT) & SHW_PD_MASK;

            for (;
                iPDDst < cPDEs;
                iPDDst++, GCPtr += cIncrement)
            {
#  if PGM_SHW_TYPE == PGM_TYPE_PAE
                const SHWPDE PdeDst = *pgmShwGetPaePDEPtr(pVCpu, GCPtr);
#  else
                const SHWPDE PdeDst = pPDDst->a[iPDDst];
#  endif
                if (PdeDst.u & PGM_PDFLAGS_MAPPING)
                {
                    Assert(pgmMapAreMappingsEnabled(pVM));
                    if ((PdeDst.u & X86_PDE_AVL_MASK) != PGM_PDFLAGS_MAPPING)
                    {
                        AssertMsgFailed(("Mapping shall only have PGM_PDFLAGS_MAPPING set! PdeDst.u=%#RX64\n", (uint64_t)PdeDst.u));
                        cErrors++;
                        continue;
                    }
                }
                else if (   (PdeDst.u & X86_PDE_P)
                        || ((PdeDst.u & (X86_PDE_P | PGM_PDFLAGS_TRACK_DIRTY)) == (X86_PDE_P | PGM_PDFLAGS_TRACK_DIRTY))
                        )
                {
                    HCPhysShw = PdeDst.u & SHW_PDE_PG_MASK;
                    PPGMPOOLPAGE pPoolPage = pgmPoolGetPage(pPool, HCPhysShw);
                    if (!pPoolPage)
                    {
                        AssertMsgFailed(("Invalid page table address %RHp at %RGv! PdeDst=%#RX64\n",
                                        HCPhysShw, GCPtr, (uint64_t)PdeDst.u));
                        cErrors++;
                        continue;
                    }
                    const SHWPT *pPTDst = (const SHWPT *)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pPoolPage);

                    if (PdeDst.u & (X86_PDE4M_PWT | X86_PDE4M_PCD))
                    {
                        AssertMsgFailed(("PDE flags PWT and/or PCD is set at %RGv! These flags are not virtualized! PdeDst=%#RX64\n",
                                        GCPtr, (uint64_t)PdeDst.u));
                        cErrors++;
                    }

                    if (PdeDst.u & (X86_PDE4M_G | X86_PDE4M_D))
                    {
                        AssertMsgFailed(("4K PDE reserved flags at %RGv! PdeDst=%#RX64\n",
                                        GCPtr, (uint64_t)PdeDst.u));
                        cErrors++;
                    }

                    const GSTPDE PdeSrc = pPDSrc->a[(iPDDst >> (GST_PD_SHIFT - SHW_PD_SHIFT)) & GST_PD_MASK];
                    if (!PdeSrc.n.u1Present)
                    {
                        AssertMsgFailed(("Guest PDE at %RGv is not present! PdeDst=%#RX64 PdeSrc=%#RX64\n",
                                        GCPtr, (uint64_t)PdeDst.u, (uint64_t)PdeSrc.u));
                        cErrors++;
                        continue;
                    }

                    if (    !PdeSrc.b.u1Size
                        ||  !fBigPagesSupported)
                    {
                        GCPhysGst = GST_GET_PDE_GCPHYS(PdeSrc);
#  if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
                        GCPhysGst = PGM_A20_APPLY(pVCpu, GCPhysGst | ((iPDDst & 1) * (PAGE_SIZE / 2)));
#  endif
                    }
                    else
                    {
#  if PGM_GST_TYPE == PGM_TYPE_32BIT
                        if (PdeSrc.u & X86_PDE4M_PG_HIGH_MASK)
                        {
                            AssertMsgFailed(("Guest PDE at %RGv is using PSE36 or similar! PdeSrc=%#RX64\n",
                                            GCPtr, (uint64_t)PdeSrc.u));
                            cErrors++;
                            continue;
                        }
#  endif
                        GCPhysGst = GST_GET_BIG_PDE_GCPHYS(pVM, PdeSrc);
#  if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
                        GCPhysGst = PGM_A20_APPLY(pVCpu, GCPhysGst | (GCPtr & RT_BIT(X86_PAGE_2M_SHIFT)));
#  endif
                    }

                    if (    pPoolPage->enmKind
                        !=  (!PdeSrc.b.u1Size || !fBigPagesSupported ? BTH_PGMPOOLKIND_PT_FOR_PT : BTH_PGMPOOLKIND_PT_FOR_BIG))
                    {
                        AssertMsgFailed(("Invalid shadow page table kind %d at %RGv! PdeSrc=%#RX64\n",
                                        pPoolPage->enmKind, GCPtr, (uint64_t)PdeSrc.u));
                        cErrors++;
                    }

                    PPGMPAGE pPhysPage = pgmPhysGetPage(pVM, GCPhysGst);
                    if (!pPhysPage)
                    {
                        AssertMsgFailed(("Cannot find guest physical address %RGp in the PDE at %RGv! PdeSrc=%#RX64\n",
                                        GCPhysGst, GCPtr, (uint64_t)PdeSrc.u));
                        cErrors++;
                        continue;
                    }

                    if (GCPhysGst != pPoolPage->GCPhys)
                    {
                        AssertMsgFailed(("GCPhysGst=%RGp != pPage->GCPhys=%RGp at %RGv\n",
                                        GCPhysGst, pPoolPage->GCPhys, GCPtr));
                        cErrors++;
                        continue;
                    }

                    if (    !PdeSrc.b.u1Size
                        ||  !fBigPagesSupported)
                    {
                        /*
                        * Page Table.
                        */
                        const GSTPT *pPTSrc;
                        rc = PGM_GCPHYS_2_PTR_V2(pVM, pVCpu, PGM_A20_APPLY(pVCpu, GCPhysGst & ~(RTGCPHYS)(PAGE_SIZE - 1)),
                                                 &pPTSrc);
                        if (RT_FAILURE(rc))
                        {
                            AssertMsgFailed(("Cannot map/convert guest physical address %RGp in the PDE at %RGv! PdeSrc=%#RX64\n",
                                            GCPhysGst, GCPtr, (uint64_t)PdeSrc.u));
                            cErrors++;
                            continue;
                        }
                        if (    (PdeSrc.u & (X86_PDE_P | X86_PDE_US | X86_PDE_RW/* | X86_PDE_A*/))
                            !=  (PdeDst.u & (X86_PDE_P | X86_PDE_US | X86_PDE_RW/* | X86_PDE_A*/)))
                        {
                            /// @todo We get here a lot on out-of-sync CR3 entries. The access handler should zap them to avoid false alarms here!
                            // (This problem will go away when/if we shadow multiple CR3s.)
                            AssertMsgFailed(("4K PDE flags mismatch at %RGv! PdeSrc=%#RX64 PdeDst=%#RX64\n",
                                            GCPtr, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
                            cErrors++;
                            continue;
                        }
                        if (PdeDst.u & PGM_PDFLAGS_TRACK_DIRTY)
                        {
                            AssertMsgFailed(("4K PDEs cannot have PGM_PDFLAGS_TRACK_DIRTY set! GCPtr=%RGv PdeDst=%#RX64\n",
                                            GCPtr, (uint64_t)PdeDst.u));
                            cErrors++;
                            continue;
                        }

                        /* iterate the page table. */
#  if PGM_SHW_TYPE == PGM_TYPE_PAE && PGM_GST_TYPE == PGM_TYPE_32BIT
                        /* Select the right PDE as we're emulating a 4kb page table with 2 shadow page tables. */
                        const unsigned offPTSrc  = ((GCPtr >> SHW_PD_SHIFT) & 1) * 512;
#  else
                        const unsigned offPTSrc  = 0;
#  endif
                        for (unsigned iPT = 0, off = 0;
                            iPT < RT_ELEMENTS(pPTDst->a);
                            iPT++, off += PAGE_SIZE)
                        {
                            const SHWPTE PteDst = pPTDst->a[iPT];

                            /* skip not-present and dirty tracked entries. */
                            if (!(SHW_PTE_GET_U(PteDst) & (X86_PTE_P | PGM_PTFLAGS_TRACK_DIRTY))) /** @todo deal with ALL handlers and CSAM !P pages! */
                                continue;
                            Assert(SHW_PTE_IS_P(PteDst));

                            const GSTPTE PteSrc = pPTSrc->a[iPT + offPTSrc];
                            if (!PteSrc.n.u1Present)
                            {
#  ifdef IN_RING3
                                PGMAssertHandlerAndFlagsInSync(pVM);
                                DBGFR3PagingDumpEx(pVM->pUVM, pVCpu->idCpu, DBGFPGDMP_FLAGS_CURRENT_CR3 | DBGFPGDMP_FLAGS_CURRENT_MODE
                                                   | DBGFPGDMP_FLAGS_GUEST | DBGFPGDMP_FLAGS_HEADER | DBGFPGDMP_FLAGS_PRINT_CR3,
                                                   0, 0, UINT64_MAX, 99, NULL);
#  endif
                                AssertMsgFailed(("Out of sync (!P) PTE at %RGv! PteSrc=%#RX64 PteDst=%#RX64 pPTSrc=%RGv iPTSrc=%x PdeSrc=%x physpte=%RGp\n",
                                                GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst), pPTSrc, iPT + offPTSrc, PdeSrc.au32[0],
                                                (uint64_t)GST_GET_PDE_GCPHYS(PdeSrc) + (iPT + offPTSrc) * sizeof(PteSrc)));
                                cErrors++;
                                continue;
                            }

                            uint64_t fIgnoreFlags = GST_PTE_PG_MASK | X86_PTE_AVL_MASK | X86_PTE_G | X86_PTE_D | X86_PTE_PWT | X86_PTE_PCD | X86_PTE_PAT;
#  if 1 /** @todo sync accessed bit properly... */
                            fIgnoreFlags |= X86_PTE_A;
#  endif

                            /* match the physical addresses */
                            HCPhysShw = SHW_PTE_GET_HCPHYS(PteDst);
                            GCPhysGst = GST_GET_PTE_GCPHYS(PteSrc);

#  ifdef IN_RING3
                            rc = PGMPhysGCPhys2HCPhys(pVM, GCPhysGst, &HCPhys);
                            if (RT_FAILURE(rc))
                            {
                                if (HCPhysShw != MMR3PageDummyHCPhys(pVM)) /** @todo this is wrong. */
                                {
                                    AssertMsgFailed(("Cannot find guest physical address %RGp at %RGv! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                     GCPhysGst, GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                    cErrors++;
                                    continue;
                                }
                            }
                            else if (HCPhysShw != (HCPhys & SHW_PTE_PG_MASK))
                            {
                                AssertMsgFailed(("Out of sync (phys) at %RGv! HCPhysShw=%RHp HCPhys=%RHp GCPhysGst=%RGp PteSrc=%#RX64 PteDst=%#RX64\n",
                                                 GCPtr + off, HCPhysShw, HCPhys, GCPhysGst, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                cErrors++;
                                continue;
                            }
#  endif

                            pPhysPage = pgmPhysGetPage(pVM, GCPhysGst);
                            if (!pPhysPage)
                            {
#  ifdef IN_RING3 /** @todo make MMR3PageDummyHCPhys an 'All' function! */
                                if (HCPhysShw != MMR3PageDummyHCPhys(pVM))  /** @todo this is wrong. */
                                {
                                    AssertMsgFailed(("Cannot find guest physical address %RGp at %RGv! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                     GCPhysGst, GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                    cErrors++;
                                    continue;
                                }
#  endif
                                if (SHW_PTE_IS_RW(PteDst))
                                {
                                    AssertMsgFailed(("Invalid guest page at %RGv is writable! GCPhysGst=%RGp PteSrc=%#RX64 PteDst=%#RX64\n",
                                                     GCPtr + off, GCPhysGst, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                    cErrors++;
                                }
                                fIgnoreFlags |= X86_PTE_RW;
                            }
                            else if (HCPhysShw != PGM_PAGE_GET_HCPHYS(pPhysPage))
                            {
                                AssertMsgFailed(("Out of sync (phys) at %RGv! HCPhysShw=%RHp pPhysPage:%R[pgmpage] GCPhysGst=%RGp PteSrc=%#RX64 PteDst=%#RX64\n",
                                                GCPtr + off, HCPhysShw, pPhysPage, GCPhysGst, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                cErrors++;
                                continue;
                            }

                            /* flags */
                            if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPhysPage))
                            {
                                if (!PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPhysPage))
                                {
                                    if (SHW_PTE_IS_RW(PteDst))
                                    {
                                        AssertMsgFailed(("WRITE access flagged at %RGv but the page is writable! pPhysPage=%R[pgmpage] PteSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, pPhysPage, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                        continue;
                                    }
                                    fIgnoreFlags |= X86_PTE_RW;
                                }
                                else
                                {
                                    if (   SHW_PTE_IS_P(PteDst)
#  if PGM_SHW_TYPE == PGM_TYPE_EPT || PGM_SHW_TYPE == PGM_TYPE_PAE || PGM_SHW_TYPE == PGM_TYPE_AMD64
                                        && !PGM_PAGE_IS_MMIO(pPhysPage)
#  endif
                                       )
                                    {
                                        AssertMsgFailed(("ALL access flagged at %RGv but the page is present! pPhysPage=%R[pgmpage] PteSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, pPhysPage, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                        continue;
                                    }
                                    fIgnoreFlags |= X86_PTE_P;
                                }
                            }
                            else
                            {
                                if (!PteSrc.n.u1Dirty && PteSrc.n.u1Write)
                                {
                                    if (SHW_PTE_IS_RW(PteDst))
                                    {
                                        AssertMsgFailed(("!DIRTY page at %RGv is writable! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                        continue;
                                    }
                                    if (!SHW_PTE_IS_TRACK_DIRTY(PteDst))
                                    {
                                        AssertMsgFailed(("!DIRTY page at %RGv is not marked TRACK_DIRTY! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                        continue;
                                    }
                                    if (SHW_PTE_IS_D(PteDst))
                                    {
                                        AssertMsgFailed(("!DIRTY page at %RGv is marked DIRTY! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                    }
#  if 0 /** @todo sync access bit properly... */
                                    if (PteDst.n.u1Accessed != PteSrc.n.u1Accessed)
                                    {
                                        AssertMsgFailed(("!DIRTY page at %RGv is has mismatching accessed bit! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                        GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                    }
                                    fIgnoreFlags |= X86_PTE_RW;
#  else
                                    fIgnoreFlags |= X86_PTE_RW | X86_PTE_A;
#  endif
                                }
                                else if (SHW_PTE_IS_TRACK_DIRTY(PteDst))
                                {
                                    /* access bit emulation (not implemented). */
                                    if (PteSrc.n.u1Accessed || SHW_PTE_IS_P(PteDst))
                                    {
                                        AssertMsgFailed(("PGM_PTFLAGS_TRACK_DIRTY set at %RGv but no accessed bit emulation! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                        continue;
                                    }
                                    if (!SHW_PTE_IS_A(PteDst))
                                    {
                                        AssertMsgFailed(("!ACCESSED page at %RGv is has the accessed bit set! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                    }
                                    fIgnoreFlags |= X86_PTE_P;
                                }
#  ifdef DEBUG_sandervl
                                fIgnoreFlags |= X86_PTE_D | X86_PTE_A;
#  endif
                            }

                            if (    (PteSrc.u & ~fIgnoreFlags)                != (SHW_PTE_GET_U(PteDst) & ~fIgnoreFlags)
                                &&  (PteSrc.u & ~(fIgnoreFlags | X86_PTE_RW)) != (SHW_PTE_GET_U(PteDst) & ~fIgnoreFlags)
                            )
                            {
                                AssertMsgFailed(("Flags mismatch at %RGv! %#RX64 != %#RX64 fIgnoreFlags=%#RX64 PteSrc=%#RX64 PteDst=%#RX64\n",
                                                 GCPtr + off, (uint64_t)PteSrc.u & ~fIgnoreFlags, SHW_PTE_LOG64(PteDst) & ~fIgnoreFlags,
                                                 fIgnoreFlags, (uint64_t)PteSrc.u, SHW_PTE_LOG64(PteDst)));
                                cErrors++;
                                continue;
                            }
                        } /* foreach PTE */
                    }
                    else
                    {
                        /*
                        * Big Page.
                        */
                        uint64_t fIgnoreFlags = X86_PDE_AVL_MASK | GST_PDE_PG_MASK | X86_PDE4M_G | X86_PDE4M_D | X86_PDE4M_PS | X86_PDE4M_PWT | X86_PDE4M_PCD;
                        if (!PdeSrc.b.u1Dirty && PdeSrc.b.u1Write)
                        {
                            if (PdeDst.n.u1Write)
                            {
                                AssertMsgFailed(("!DIRTY page at %RGv is writable! PdeSrc=%#RX64 PdeDst=%#RX64\n",
                                                GCPtr, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
                                cErrors++;
                                continue;
                            }
                            if (!(PdeDst.u & PGM_PDFLAGS_TRACK_DIRTY))
                            {
                                AssertMsgFailed(("!DIRTY page at %RGv is not marked TRACK_DIRTY! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                GCPtr, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
                                cErrors++;
                                continue;
                            }
#  if 0 /** @todo sync access bit properly... */
                            if (PdeDst.n.u1Accessed != PdeSrc.b.u1Accessed)
                            {
                                AssertMsgFailed(("!DIRTY page at %RGv is has mismatching accessed bit! PteSrc=%#RX64 PteDst=%#RX64\n",
                                                GCPtr, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
                                cErrors++;
                            }
                            fIgnoreFlags |= X86_PTE_RW;
#  else
                            fIgnoreFlags |= X86_PTE_RW | X86_PTE_A;
#  endif
                        }
                        else if (PdeDst.u & PGM_PDFLAGS_TRACK_DIRTY)
                        {
                            /* access bit emulation (not implemented). */
                            if (PdeSrc.b.u1Accessed || PdeDst.n.u1Present)
                            {
                                AssertMsgFailed(("PGM_PDFLAGS_TRACK_DIRTY set at %RGv but no accessed bit emulation! PdeSrc=%#RX64 PdeDst=%#RX64\n",
                                                GCPtr, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
                                cErrors++;
                                continue;
                            }
                            if (!PdeDst.n.u1Accessed)
                            {
                                AssertMsgFailed(("!ACCESSED page at %RGv is has the accessed bit set! PdeSrc=%#RX64 PdeDst=%#RX64\n",
                                                GCPtr, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
                                cErrors++;
                            }
                            fIgnoreFlags |= X86_PTE_P;
                        }

                        if ((PdeSrc.u & ~fIgnoreFlags) != (PdeDst.u & ~fIgnoreFlags))
                        {
                            AssertMsgFailed(("Flags mismatch (B) at %RGv! %#RX64 != %#RX64 fIgnoreFlags=%#RX64 PdeSrc=%#RX64 PdeDst=%#RX64\n",
                                            GCPtr, (uint64_t)PdeSrc.u & ~fIgnoreFlags, (uint64_t)PdeDst.u & ~fIgnoreFlags,
                                            fIgnoreFlags, (uint64_t)PdeSrc.u, (uint64_t)PdeDst.u));
                            cErrors++;
                        }

                        /* iterate the page table. */
                        for (unsigned iPT = 0, off = 0;
                            iPT < RT_ELEMENTS(pPTDst->a);
                            iPT++, off += PAGE_SIZE, GCPhysGst = PGM_A20_APPLY(pVCpu, GCPhysGst + PAGE_SIZE))
                        {
                            const SHWPTE PteDst = pPTDst->a[iPT];

                            if (SHW_PTE_IS_TRACK_DIRTY(PteDst))
                            {
                                AssertMsgFailed(("The PTE at %RGv emulating a 2/4M page is marked TRACK_DIRTY! PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                 GCPtr + off, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                cErrors++;
                            }

                            /* skip not-present entries. */
                            if (!SHW_PTE_IS_P(PteDst)) /** @todo deal with ALL handlers and CSAM !P pages! */
                                continue;

                            fIgnoreFlags = X86_PTE_PAE_PG_MASK | X86_PTE_AVL_MASK | X86_PTE_PWT | X86_PTE_PCD | X86_PTE_PAT | X86_PTE_D | X86_PTE_A | X86_PTE_G | X86_PTE_PAE_NX;

                            /* match the physical addresses */
                            HCPhysShw = SHW_PTE_GET_HCPHYS(PteDst);

#  ifdef IN_RING3
                            rc = PGMPhysGCPhys2HCPhys(pVM, GCPhysGst, &HCPhys);
                            if (RT_FAILURE(rc))
                            {
                                if (HCPhysShw != MMR3PageDummyHCPhys(pVM))  /** @todo this is wrong. */
                                {
                                    AssertMsgFailed(("Cannot find guest physical address %RGp at %RGv! PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                     GCPhysGst, GCPtr + off, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                    cErrors++;
                                }
                            }
                            else if (HCPhysShw != (HCPhys & X86_PTE_PAE_PG_MASK))
                            {
                                AssertMsgFailed(("Out of sync (phys) at %RGv! HCPhysShw=%RHp HCPhys=%RHp GCPhysGst=%RGp PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                 GCPtr + off, HCPhysShw, HCPhys, GCPhysGst, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                cErrors++;
                                continue;
                            }
#  endif
                            pPhysPage = pgmPhysGetPage(pVM, GCPhysGst);
                            if (!pPhysPage)
                            {
#  ifdef IN_RING3 /** @todo make MMR3PageDummyHCPhys an 'All' function! */
                                if (HCPhysShw != MMR3PageDummyHCPhys(pVM))  /** @todo this is wrong. */
                                {
                                    AssertMsgFailed(("Cannot find guest physical address %RGp at %RGv! PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                     GCPhysGst, GCPtr + off, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                    cErrors++;
                                    continue;
                                }
#  endif
                                if (SHW_PTE_IS_RW(PteDst))
                                {
                                    AssertMsgFailed(("Invalid guest page at %RGv is writable! GCPhysGst=%RGp PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                     GCPtr + off, GCPhysGst, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                    cErrors++;
                                }
                                fIgnoreFlags |= X86_PTE_RW;
                            }
                            else if (HCPhysShw != PGM_PAGE_GET_HCPHYS(pPhysPage))
                            {
                                AssertMsgFailed(("Out of sync (phys) at %RGv! HCPhysShw=%RHp pPhysPage=%R[pgmpage] GCPhysGst=%RGp PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                 GCPtr + off, HCPhysShw, pPhysPage, GCPhysGst, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                cErrors++;
                                continue;
                            }

                            /* flags */
                            if (PGM_PAGE_HAS_ACTIVE_HANDLERS(pPhysPage))
                            {
                                if (!PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(pPhysPage))
                                {
                                    if (PGM_PAGE_GET_HNDL_PHYS_STATE(pPhysPage) != PGM_PAGE_HNDL_PHYS_STATE_DISABLED)
                                    {
                                        if (SHW_PTE_IS_RW(PteDst))
                                        {
                                            AssertMsgFailed(("WRITE access flagged at %RGv but the page is writable! pPhysPage=%R[pgmpage] PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                             GCPtr + off, pPhysPage, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                            cErrors++;
                                            continue;
                                        }
                                        fIgnoreFlags |= X86_PTE_RW;
                                    }
                                }
                                else
                                {
                                    if (   SHW_PTE_IS_P(PteDst)
#  if PGM_SHW_TYPE == PGM_TYPE_EPT || PGM_SHW_TYPE == PGM_TYPE_PAE || PGM_SHW_TYPE == PGM_TYPE_AMD64
                                        && !PGM_PAGE_IS_MMIO(pPhysPage)
#  endif
                                        )
                                    {
                                        AssertMsgFailed(("ALL access flagged at %RGv but the page is present! pPhysPage=%R[pgmpage] PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                         GCPtr + off, pPhysPage, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                        cErrors++;
                                        continue;
                                    }
                                    fIgnoreFlags |= X86_PTE_P;
                                }
                            }

                            if (    (PdeSrc.u & ~fIgnoreFlags)                != (SHW_PTE_GET_U(PteDst) & ~fIgnoreFlags)
                                &&  (PdeSrc.u & ~(fIgnoreFlags | X86_PTE_RW)) != (SHW_PTE_GET_U(PteDst) & ~fIgnoreFlags) /* lazy phys handler dereg. */
                            )
                            {
                                AssertMsgFailed(("Flags mismatch (BT) at %RGv! %#RX64 != %#RX64 fIgnoreFlags=%#RX64 PdeSrc=%#RX64 PteDst=%#RX64\n",
                                                 GCPtr + off, (uint64_t)PdeSrc.u & ~fIgnoreFlags, SHW_PTE_LOG64(PteDst) & ~fIgnoreFlags,
                                                 fIgnoreFlags, (uint64_t)PdeSrc.u, SHW_PTE_LOG64(PteDst)));
                                cErrors++;
                                continue;
                            }
                        } /* for each PTE */
                    }
                }
                /* not present */

            } /* for each PDE */

        } /* for each PDPTE */

    } /* for each PML4E */

#  ifdef DEBUG
    if (cErrors)
        LogFlow(("AssertCR3: cErrors=%d\n", cErrors));
#  endif
# endif /* GST is in {32BIT, PAE, AMD64} */
    return cErrors;
#endif /* PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT */
}
#endif /* VBOX_STRICT */


/**
 * Sets up the CR3 for shadow paging
 *
 * @returns Strict VBox status code.
 * @retval  VINF_SUCCESS.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   GCPhysCR3       The physical address in the CR3 register.  (A20
 *                          mask already applied.)
 */
PGM_BTH_DECL(int, MapCR3)(PVMCPU pVCpu, RTGCPHYS GCPhysCR3)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM); NOREF(pVM);

    /* Update guest paging info. */
#if PGM_GST_TYPE == PGM_TYPE_32BIT \
 || PGM_GST_TYPE == PGM_TYPE_PAE \
 || PGM_GST_TYPE == PGM_TYPE_AMD64

    LogFlow(("MapCR3: %RGp\n", GCPhysCR3));
    PGM_A20_ASSERT_MASKED(pVCpu, GCPhysCR3);

    /*
     * Map the page CR3 points at.
     */
    RTHCPTR     HCPtrGuestCR3;
    RTHCPHYS    HCPhysGuestCR3;
    pgmLock(pVM);
    PPGMPAGE    pPageCR3 = pgmPhysGetPage(pVM, GCPhysCR3);
    AssertReturn(pPageCR3, VERR_PGM_INVALID_CR3_ADDR);
    HCPhysGuestCR3 = PGM_PAGE_GET_HCPHYS(pPageCR3);
    /** @todo this needs some reworking wrt. locking?  */
# if defined(IN_RC) || defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0)
    HCPtrGuestCR3 = NIL_RTHCPTR;
    int rc = VINF_SUCCESS;
# else
    int rc = pgmPhysGCPhys2CCPtrInternalDepr(pVM, pPageCR3, GCPhysCR3 & GST_CR3_PAGE_MASK, (void **)&HCPtrGuestCR3); /** @todo r=bird: This GCPhysCR3 masking isn't necessary. */
# endif
    pgmUnlock(pVM);
    if (RT_SUCCESS(rc))
    {
        rc = PGMMap(pVM, (RTGCPTR)pVM->pgm.s.GCPtrCR3Mapping, HCPhysGuestCR3, PAGE_SIZE, 0);
        if (RT_SUCCESS(rc))
        {
# ifdef IN_RC
            PGM_INVL_PG(pVCpu, pVM->pgm.s.GCPtrCR3Mapping);
# endif
# if PGM_GST_TYPE == PGM_TYPE_32BIT
            pVCpu->pgm.s.pGst32BitPdR3 = (R3PTRTYPE(PX86PD))HCPtrGuestCR3;
#  ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
            pVCpu->pgm.s.pGst32BitPdR0 = (R0PTRTYPE(PX86PD))HCPtrGuestCR3;
#  endif
            pVCpu->pgm.s.pGst32BitPdRC = (RCPTRTYPE(PX86PD))(RTRCUINTPTR)pVM->pgm.s.GCPtrCR3Mapping;

# elif PGM_GST_TYPE == PGM_TYPE_PAE
            unsigned off = GCPhysCR3 & GST_CR3_PAGE_MASK & PAGE_OFFSET_MASK;
            pVCpu->pgm.s.pGstPaePdptR3 = (R3PTRTYPE(PX86PDPT))HCPtrGuestCR3;
#  ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
            pVCpu->pgm.s.pGstPaePdptR0 = (R0PTRTYPE(PX86PDPT))HCPtrGuestCR3;
#  endif
            pVCpu->pgm.s.pGstPaePdptRC = (RCPTRTYPE(PX86PDPT))((RTRCUINTPTR)pVM->pgm.s.GCPtrCR3Mapping + off);
            LogFlow(("Cached mapping %RRv\n", pVCpu->pgm.s.pGstPaePdptRC));

            /*
             * Map the 4 PDs too.
             */
            PX86PDPT pGuestPDPT = pgmGstGetPaePDPTPtr(pVCpu);
            RTGCPTR  GCPtr      = pVM->pgm.s.GCPtrCR3Mapping + PAGE_SIZE;
            for (unsigned i = 0; i < X86_PG_PAE_PDPE_ENTRIES; i++, GCPtr += PAGE_SIZE)
            {
                pVCpu->pgm.s.aGstPaePdpeRegs[i].u = pGuestPDPT->a[i].u;
                if (pGuestPDPT->a[i].n.u1Present)
                {
                    RTHCPTR     HCPtr;
                    RTHCPHYS    HCPhys;
                    RTGCPHYS    GCPhys = PGM_A20_APPLY(pVCpu, pGuestPDPT->a[i].u & X86_PDPE_PG_MASK);
                    pgmLock(pVM);
                    PPGMPAGE    pPage  = pgmPhysGetPage(pVM, GCPhys);
                    AssertReturn(pPage, VERR_PGM_INVALID_PDPE_ADDR);
                    HCPhys = PGM_PAGE_GET_HCPHYS(pPage);
#  if defined(IN_RC) || defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0)
                    HCPtr = NIL_RTHCPTR;
                    int rc2 = VINF_SUCCESS;
#  else
                    int rc2 = pgmPhysGCPhys2CCPtrInternalDepr(pVM, pPage, GCPhys, (void **)&HCPtr);
#  endif
                    pgmUnlock(pVM);
                    if (RT_SUCCESS(rc2))
                    {
                        rc = PGMMap(pVM, GCPtr, HCPhys, PAGE_SIZE, 0);
                        AssertRCReturn(rc, rc);

                        pVCpu->pgm.s.apGstPaePDsR3[i]     = (R3PTRTYPE(PX86PDPAE))HCPtr;
#  ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
                        pVCpu->pgm.s.apGstPaePDsR0[i]     = (R0PTRTYPE(PX86PDPAE))HCPtr;
#  endif
                        pVCpu->pgm.s.apGstPaePDsRC[i]     = (RCPTRTYPE(PX86PDPAE))(RTRCUINTPTR)GCPtr;
                        pVCpu->pgm.s.aGCPhysGstPaePDs[i]  = GCPhys;
#  ifdef IN_RC
                        PGM_INVL_PG(pVCpu, GCPtr);
#  endif
                        continue;
                    }
                    AssertMsgFailed(("pgmR3Gst32BitMapCR3: rc2=%d GCPhys=%RGp i=%d\n", rc2, GCPhys, i));
                }

                pVCpu->pgm.s.apGstPaePDsR3[i]     = 0;
#  ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
                pVCpu->pgm.s.apGstPaePDsR0[i]     = 0;
#  endif
                pVCpu->pgm.s.apGstPaePDsRC[i]     = 0;
                pVCpu->pgm.s.aGCPhysGstPaePDs[i]  = NIL_RTGCPHYS;
#  ifdef IN_RC
                PGM_INVL_PG(pVCpu, GCPtr); /** @todo this shouldn't be necessary? */
#  endif
            }

# elif PGM_GST_TYPE == PGM_TYPE_AMD64
            pVCpu->pgm.s.pGstAmd64Pml4R3 = (R3PTRTYPE(PX86PML4))HCPtrGuestCR3;
#  ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
            pVCpu->pgm.s.pGstAmd64Pml4R0 = (R0PTRTYPE(PX86PML4))HCPtrGuestCR3;
#  endif
# endif
        }
        else
            AssertMsgFailed(("rc=%Rrc GCPhysGuestPD=%RGp\n", rc, GCPhysCR3));
    }
    else
        AssertMsgFailed(("rc=%Rrc GCPhysGuestPD=%RGp\n", rc, GCPhysCR3));

#else /* prot/real stub */
    int rc = VINF_SUCCESS;
#endif

    /* Update shadow paging info for guest modes with paging (32, pae, 64). */
# if  (   (   PGM_SHW_TYPE == PGM_TYPE_32BIT \
           || PGM_SHW_TYPE == PGM_TYPE_PAE    \
           || PGM_SHW_TYPE == PGM_TYPE_AMD64) \
       && (   PGM_GST_TYPE != PGM_TYPE_REAL   \
           && PGM_GST_TYPE != PGM_TYPE_PROT))

    Assert(!pVM->pgm.s.fNestedPaging);
    PGM_A20_ASSERT_MASKED(pVCpu, GCPhysCR3);

    /*
     * Update the shadow root page as well since that's not fixed.
     */
    PPGMPOOL     pPool             = pVM->pgm.s.CTX_SUFF(pPool);
    PPGMPOOLPAGE pOldShwPageCR3    = pVCpu->pgm.s.CTX_SUFF(pShwPageCR3);
    PPGMPOOLPAGE pNewShwPageCR3;

    pgmLock(pVM);

# ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    if (pPool->cDirtyPages)
        pgmPoolResetDirtyPages(pVM);
# endif

    Assert(!(GCPhysCR3 >> (PAGE_SHIFT + 32)));
    rc = pgmPoolAlloc(pVM, GCPhysCR3 & GST_CR3_PAGE_MASK, BTH_PGMPOOLKIND_ROOT, PGMPOOLACCESS_DONTCARE, PGM_A20_IS_ENABLED(pVCpu),
                      NIL_PGMPOOL_IDX, UINT32_MAX, true /*fLockPage*/,
                      &pNewShwPageCR3);
    AssertFatalRC(rc);
    rc = VINF_SUCCESS;

#  ifdef IN_RC
    /*
     * WARNING! We can't deal with jumps to ring 3 in the code below as the
     *          state will be inconsistent! Flush important things now while
     *          we still can and then make sure there are no ring-3 calls.
     */
#   ifdef VBOX_WITH_REM
    REMNotifyHandlerPhysicalFlushIfAlmostFull(pVM, pVCpu);
#   endif
    VMMRZCallRing3Disable(pVCpu);
#  endif

    pVCpu->pgm.s.CTX_SUFF(pShwPageCR3) = pNewShwPageCR3;
#  ifdef IN_RING0
    pVCpu->pgm.s.pShwPageCR3R3 = MMHyperCCToR3(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
    pVCpu->pgm.s.pShwPageCR3RC = MMHyperCCToRC(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
#  elif defined(IN_RC)
    pVCpu->pgm.s.pShwPageCR3R3 = MMHyperCCToR3(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
    pVCpu->pgm.s.pShwPageCR3R0 = MMHyperCCToR0(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
#  else
    pVCpu->pgm.s.pShwPageCR3R0 = MMHyperCCToR0(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
    pVCpu->pgm.s.pShwPageCR3RC = MMHyperCCToRC(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
#  endif

#  ifndef PGM_WITHOUT_MAPPINGS
    /*
     * Apply all hypervisor mappings to the new CR3.
     * Note that SyncCR3 will be executed in case CR3 is changed in a guest paging mode; this will
     * make sure we check for conflicts in the new CR3 root.
     */
#   if PGM_WITH_PAGING(PGM_GST_TYPE, PGM_SHW_TYPE)
    Assert(VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL) || VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
#   endif
    rc = pgmMapActivateCR3(pVM, pNewShwPageCR3);
    AssertRCReturn(rc, rc);
#  endif

    /* Set the current hypervisor CR3. */
    CPUMSetHyperCR3(pVCpu, PGMGetHyperCR3(pVCpu));
    SELMShadowCR3Changed(pVM, pVCpu);

#  ifdef IN_RC
    /* NOTE: The state is consistent again. */
    VMMRZCallRing3Enable(pVCpu);
#  endif

    /* Clean up the old CR3 root. */
    if (    pOldShwPageCR3
        &&  pOldShwPageCR3 != pNewShwPageCR3    /* @todo can happen due to incorrect syncing between REM & PGM; find the real cause */)
    {
        Assert(pOldShwPageCR3->enmKind != PGMPOOLKIND_FREE);
#  ifndef PGM_WITHOUT_MAPPINGS
        /* Remove the hypervisor mappings from the shadow page table. */
        pgmMapDeactivateCR3(pVM, pOldShwPageCR3);
#  endif
        /* Mark the page as unlocked; allow flushing again. */
        pgmPoolUnlockPage(pPool, pOldShwPageCR3);

        pgmPoolFreeByPage(pPool, pOldShwPageCR3, NIL_PGMPOOL_IDX, UINT32_MAX);
    }
    pgmUnlock(pVM);
# else
    NOREF(GCPhysCR3);
# endif

    return rc;
}

/**
 * Unmaps the shadow CR3.
 *
 * @returns VBox status, no specials.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
PGM_BTH_DECL(int, UnmapCR3)(PVMCPU pVCpu)
{
    LogFlow(("UnmapCR3\n"));

    int rc  = VINF_SUCCESS;
    PVM pVM = pVCpu->CTX_SUFF(pVM); NOREF(pVM);

    /*
     * Update guest paging info.
     */
#if PGM_GST_TYPE == PGM_TYPE_32BIT
    pVCpu->pgm.s.pGst32BitPdR3 = 0;
# ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
    pVCpu->pgm.s.pGst32BitPdR0 = 0;
# endif
    pVCpu->pgm.s.pGst32BitPdRC = 0;

#elif PGM_GST_TYPE == PGM_TYPE_PAE
    pVCpu->pgm.s.pGstPaePdptR3 = 0;
# ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
    pVCpu->pgm.s.pGstPaePdptR0 = 0;
# endif
    pVCpu->pgm.s.pGstPaePdptRC = 0;
    for (unsigned i = 0; i < X86_PG_PAE_PDPE_ENTRIES; i++)
    {
        pVCpu->pgm.s.apGstPaePDsR3[i]    = 0;
# ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
        pVCpu->pgm.s.apGstPaePDsR0[i]    = 0;
# endif
        pVCpu->pgm.s.apGstPaePDsRC[i]    = 0;
        pVCpu->pgm.s.aGCPhysGstPaePDs[i] = NIL_RTGCPHYS;
    }

#elif PGM_GST_TYPE == PGM_TYPE_AMD64
    pVCpu->pgm.s.pGstAmd64Pml4R3 = 0;
# ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
    pVCpu->pgm.s.pGstAmd64Pml4R0 = 0;
# endif

#else /* prot/real mode stub */
    /* nothing to do */
#endif

#if !defined(IN_RC) /* In RC we rely on MapCR3 to do the shadow part for us at a safe time */
    /*
     * Update shadow paging info.
     */
# if  (   (   PGM_SHW_TYPE == PGM_TYPE_32BIT  \
           || PGM_SHW_TYPE == PGM_TYPE_PAE    \
           || PGM_SHW_TYPE == PGM_TYPE_AMD64))

#  if PGM_GST_TYPE != PGM_TYPE_REAL
    Assert(!pVM->pgm.s.fNestedPaging);
#  endif

    pgmLock(pVM);

# ifndef PGM_WITHOUT_MAPPINGS
    if (pVCpu->pgm.s.CTX_SUFF(pShwPageCR3))
        /* Remove the hypervisor mappings from the shadow page table. */
        pgmMapDeactivateCR3(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
# endif

    if (pVCpu->pgm.s.CTX_SUFF(pShwPageCR3))
    {
        PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

# ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
        if (pPool->cDirtyPages)
            pgmPoolResetDirtyPages(pVM);
# endif

        /* Mark the page as unlocked; allow flushing again. */
        pgmPoolUnlockPage(pPool, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));

        pgmPoolFreeByPage(pPool, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3), NIL_PGMPOOL_IDX, UINT32_MAX);
        pVCpu->pgm.s.pShwPageCR3R3 = 0;
        pVCpu->pgm.s.pShwPageCR3R0 = 0;
        pVCpu->pgm.s.pShwPageCR3RC = 0;
    }
    pgmUnlock(pVM);
# endif
#endif /* !IN_RC*/

    return rc;
}

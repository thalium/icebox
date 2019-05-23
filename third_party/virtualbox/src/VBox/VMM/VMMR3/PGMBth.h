/* $Id: PGMBth.h $ */
/** @file
 * VBox - Page Manager / Monitor, Shadow+Guest Paging Template.
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


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
RT_C_DECLS_BEGIN
PGM_BTH_DECL(int, InitData)(PVM pVM, PPGMMODEDATA pModeData, bool fResolveGCAndR0);
PGM_BTH_DECL(int, Enter)(PVMCPU pVCpu, RTGCPHYS GCPhysCR3);
PGM_BTH_DECL(int, Relocate)(PVMCPU pVCpu, RTGCPTR offDelta);

PGM_BTH_DECL(int, Trap0eHandler)(PVMCPU pVCpu, RTGCUINT uErr, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, bool *pfLockTaken);
PGM_BTH_DECL(int, SyncCR3)(PVMCPU pVCpu, uint64_t cr0, uint64_t cr3, uint64_t cr4, bool fGlobal);
PGM_BTH_DECL(int, VerifyAccessSyncPage)(PVMCPU pVCpu, RTGCPTR Addr, unsigned fPage, unsigned uError);
PGM_BTH_DECL(int, InvalidatePage)(PVMCPU pVCpu, RTGCPTR GCPtrPage);
PGM_BTH_DECL(int, PrefetchPage)(PVMCPU pVCpu, RTGCPTR GCPtrPage);
PGM_BTH_DECL(unsigned, AssertCR3)(PVMCPU pVCpu, uint64_t cr3, uint64_t cr4, RTGCPTR GCPtr = 0, RTGCPTR cb = ~(RTGCPTR)0);
PGM_BTH_DECL(int, MapCR3)(PVMCPU pVCpu, RTGCPHYS GCPhysCR3);
PGM_BTH_DECL(int, UnmapCR3)(PVMCPU pVCpu);
RT_C_DECLS_END


/**
 * Initializes the both bit of the paging mode data.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pModeData       The pointer table to initialize.
 * @param   fResolveGCAndR0 Indicate whether or not GC and Ring-0 symbols can be resolved now.
 *                          This is used early in the init process to avoid trouble with PDM
 *                          not being initialized yet.
 */
PGM_BTH_DECL(int, InitData)(PVM pVM, PPGMMODEDATA pModeData, bool fResolveGCAndR0)
{
    Assert(pModeData->uShwType == PGM_SHW_TYPE); Assert(pModeData->uGstType == PGM_GST_TYPE);

    /* Ring 3 */
    pModeData->pfnR3BthRelocate             = PGM_BTH_NAME(Relocate);
    pModeData->pfnR3BthSyncCR3              = PGM_BTH_NAME(SyncCR3);
    pModeData->pfnR3BthInvalidatePage       = PGM_BTH_NAME(InvalidatePage);
    pModeData->pfnR3BthPrefetchPage         = PGM_BTH_NAME(PrefetchPage);
    pModeData->pfnR3BthVerifyAccessSyncPage = PGM_BTH_NAME(VerifyAccessSyncPage);
#ifdef VBOX_STRICT
    pModeData->pfnR3BthAssertCR3            = PGM_BTH_NAME(AssertCR3);
#endif
    pModeData->pfnR3BthMapCR3               = PGM_BTH_NAME(MapCR3);
    pModeData->pfnR3BthUnmapCR3             = PGM_BTH_NAME(UnmapCR3);

    if (fResolveGCAndR0)
    {
        int rc;

        if (!HMIsEnabled(pVM))
        {
#if PGM_SHW_TYPE != PGM_TYPE_AMD64 && PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT /* No AMD64 for traditional virtualization, only VT-x and AMD-V. */
            /* RC */
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(Trap0eHandler),       &pModeData->pfnRCBthTrap0eHandler);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(Trap0eHandler),  rc), rc);
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(InvalidatePage),      &pModeData->pfnRCBthInvalidatePage);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(InvalidatePage), rc), rc);
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(SyncCR3),             &pModeData->pfnRCBthSyncCR3);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(SyncCR3), rc), rc);
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(PrefetchPage),        &pModeData->pfnRCBthPrefetchPage);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(PrefetchPage), rc), rc);
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(VerifyAccessSyncPage),&pModeData->pfnRCBthVerifyAccessSyncPage);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(VerifyAccessSyncPage), rc), rc);
# ifdef VBOX_STRICT
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(AssertCR3),           &pModeData->pfnRCBthAssertCR3);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(AssertCR3), rc), rc);
# endif
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(MapCR3),              &pModeData->pfnRCBthMapCR3);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(MapCR3), rc), rc);
            rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_BTH_NAME_RC_STR(UnmapCR3),            &pModeData->pfnRCBthUnmapCR3);
            AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_RC_STR(UnmapCR3), rc), rc);
#endif /* Not AMD64 shadow paging. */
        }

        /* Ring 0 */
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(Trap0eHandler),       &pModeData->pfnR0BthTrap0eHandler);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(Trap0eHandler),  rc), rc);
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(InvalidatePage),      &pModeData->pfnR0BthInvalidatePage);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(InvalidatePage), rc), rc);
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(SyncCR3),             &pModeData->pfnR0BthSyncCR3);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(SyncCR3), rc), rc);
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(PrefetchPage),        &pModeData->pfnR0BthPrefetchPage);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(PrefetchPage), rc), rc);
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(VerifyAccessSyncPage),&pModeData->pfnR0BthVerifyAccessSyncPage);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(VerifyAccessSyncPage), rc), rc);
#ifdef VBOX_STRICT
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(AssertCR3),           &pModeData->pfnR0BthAssertCR3);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(AssertCR3), rc), rc);
#endif
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(MapCR3),              &pModeData->pfnR0BthMapCR3);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(MapCR3), rc), rc);
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_BTH_NAME_R0_STR(UnmapCR3),            &pModeData->pfnR0BthUnmapCR3);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_BTH_NAME_R0_STR(UnmapCR3), rc), rc);
    }
    return VINF_SUCCESS;
}


/**
 * Enters the shadow+guest mode.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPhysCR3   The physical address from the CR3 register.
 */
PGM_BTH_DECL(int, Enter)(PVMCPU pVCpu, RTGCPHYS GCPhysCR3)
{
    /* Here we deal with allocation of the root shadow page table for real and protected mode during mode switches;
     * Other modes rely on MapCR3/UnmapCR3 to setup the shadow root page tables.
     */
#if  (   (   PGM_SHW_TYPE == PGM_TYPE_32BIT \
          || PGM_SHW_TYPE == PGM_TYPE_PAE    \
          || PGM_SHW_TYPE == PGM_TYPE_AMD64) \
      && (   PGM_GST_TYPE == PGM_TYPE_REAL   \
          || PGM_GST_TYPE == PGM_TYPE_PROT))

    PVM pVM = pVCpu->pVMR3;

    Assert(HMIsNestedPagingActive(pVM) == pVM->pgm.s.fNestedPaging);
    Assert(!pVM->pgm.s.fNestedPaging);

    pgmLock(pVM);
    /* Note: we only really need shadow paging in real and protected mode for VT-x and AMD-V (excluding nested paging/EPT modes),
     *       but any calls to GC need a proper shadow page setup as well.
     */
    /* Free the previous root mapping if still active. */
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    if (pVCpu->pgm.s.CTX_SUFF(pShwPageCR3))
    {
        Assert(pVCpu->pgm.s.pShwPageCR3R3->enmKind != PGMPOOLKIND_FREE);

        /* Mark the page as unlocked; allow flushing again. */
        pgmPoolUnlockPage(pPool, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));

# ifndef PGM_WITHOUT_MAPPINGS
        /* Remove the hypervisor mappings from the shadow page table. */
        pgmMapDeactivateCR3(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
# endif

        pgmPoolFreeByPage(pPool, pVCpu->pgm.s.pShwPageCR3R3, NIL_PGMPOOL_IDX, UINT32_MAX);
        pVCpu->pgm.s.pShwPageCR3R3 = 0;
        pVCpu->pgm.s.pShwPageCR3RC = 0;
        pVCpu->pgm.s.pShwPageCR3R0 = 0;
    }

    /* construct a fake address. */
    GCPhysCR3 = RT_BIT_64(63);
    int rc = pgmPoolAlloc(pVM, GCPhysCR3, BTH_PGMPOOLKIND_ROOT, PGMPOOLACCESS_DONTCARE, PGM_A20_IS_ENABLED(pVCpu),
                          NIL_PGMPOOL_IDX, UINT32_MAX, false /*fLockPage*/,
                          &pVCpu->pgm.s.pShwPageCR3R3);
    if (rc == VERR_PGM_POOL_FLUSHED)
    {
        Log(("Bth-Enter: PGM pool flushed -> signal sync cr3\n"));
        Assert(VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
        pgmUnlock(pVM);
        return VINF_PGM_SYNC_CR3;
    }
    AssertRCReturn(rc, rc);

    /* Mark the page as locked; disallow flushing. */
    pgmPoolLockPage(pPool, pVCpu->pgm.s.pShwPageCR3R3);

    pVCpu->pgm.s.pShwPageCR3R0 = MMHyperCCToR0(pVM, pVCpu->pgm.s.pShwPageCR3R3);
    pVCpu->pgm.s.pShwPageCR3RC = MMHyperCCToRC(pVM, pVCpu->pgm.s.pShwPageCR3R3);

    /* Set the current hypervisor CR3. */
    CPUMSetHyperCR3(pVCpu, PGMGetHyperCR3(pVCpu));

# ifndef PGM_WITHOUT_MAPPINGS
    /* Apply all hypervisor mappings to the new CR3. */
    rc = pgmMapActivateCR3(pVM, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
# endif

    pgmUnlock(pVM);
    return rc;
#else
    NOREF(pVCpu); NOREF(GCPhysCR3);
    return VINF_SUCCESS;
#endif
}


/**
 * Relocate any GC pointers related to shadow mode paging.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   offDelta    The relocation offset.
 */
PGM_BTH_DECL(int, Relocate)(PVMCPU pVCpu, RTGCPTR offDelta)
{
    /* nothing special to do here - InitData does the job. */
    NOREF(pVCpu); NOREF(offDelta);
    return VINF_SUCCESS;
}


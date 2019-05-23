/* $Id: PGMAllGst.h $ */
/** @file
 * VBox - Page Manager, Guest Paging Template - All context code.
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
#if PGM_GST_TYPE == PGM_TYPE_32BIT \
 || PGM_GST_TYPE == PGM_TYPE_PAE \
 || PGM_GST_TYPE == PGM_TYPE_AMD64
static int PGM_GST_NAME(Walk)(PVMCPU pVCpu, RTGCPTR GCPtr, PGSTPTWALK pWalk);
#endif
PGM_GST_DECL(int,  GetPage)(PVMCPU pVCpu, RTGCPTR GCPtr, uint64_t *pfFlags, PRTGCPHYS pGCPhys);
PGM_GST_DECL(int,  ModifyPage)(PVMCPU pVCpu, RTGCPTR GCPtr, size_t cb, uint64_t fFlags, uint64_t fMask);
PGM_GST_DECL(int,  GetPDE)(PVMCPU pVCpu, RTGCPTR GCPtr, PX86PDEPAE pPDE);
PGM_GST_DECL(bool, HandlerVirtualUpdate)(PVM pVM, uint32_t cr4);
RT_C_DECLS_END


#if PGM_GST_TYPE == PGM_TYPE_32BIT \
 || PGM_GST_TYPE == PGM_TYPE_PAE \
 || PGM_GST_TYPE == PGM_TYPE_AMD64


DECLINLINE(int) PGM_GST_NAME(WalkReturnNotPresent)(PVMCPU pVCpu, PGSTPTWALK pWalk, int iLevel)
{
    NOREF(iLevel); NOREF(pVCpu);
    pWalk->Core.fNotPresent     = true;
    pWalk->Core.uLevel          = (uint8_t)iLevel;
    return VERR_PAGE_TABLE_NOT_PRESENT;
}

DECLINLINE(int) PGM_GST_NAME(WalkReturnBadPhysAddr)(PVMCPU pVCpu, PGSTPTWALK pWalk, int rc, int iLevel)
{
    AssertMsg(rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc)); NOREF(rc); NOREF(pVCpu);
    pWalk->Core.fBadPhysAddr    = true;
    pWalk->Core.uLevel          = (uint8_t)iLevel;
    return VERR_PAGE_TABLE_NOT_PRESENT;
}

DECLINLINE(int) PGM_GST_NAME(WalkReturnRsvdError)(PVMCPU pVCpu, PGSTPTWALK pWalk, int iLevel)
{
    NOREF(pVCpu);
    pWalk->Core.fRsvdError      = true;
    pWalk->Core.uLevel          = (uint8_t)iLevel;
    return VERR_PAGE_TABLE_NOT_PRESENT;
}


/**
 * Performs a guest page table walk.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PAGE_TABLE_NOT_PRESENT on failure.  Check pWalk for details.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   GCPtr       The guest virtual address to walk by.
 * @param   pWalk       Where to return the walk result. This is always set.
 */
DECLINLINE(int) PGM_GST_NAME(Walk)(PVMCPU pVCpu, RTGCPTR GCPtr, PGSTPTWALK pWalk)
{
    int rc;

    /*
     * Init the walking structure.
     */
    RT_ZERO(*pWalk);
    pWalk->Core.GCPtr = GCPtr;

# if PGM_GST_TYPE == PGM_TYPE_32BIT \
  || PGM_GST_TYPE == PGM_TYPE_PAE
    /*
     * Boundary check for PAE and 32-bit (prevents trouble further down).
     */
    if (RT_UNLIKELY(GCPtr >= _4G))
        return PGM_GST_NAME(WalkReturnNotPresent)(pVCpu, pWalk, 8);
# endif

    uint32_t register fEffective = X86_PTE_RW | X86_PTE_US | X86_PTE_PWT | X86_PTE_PCD | X86_PTE_A | 1;
    {
# if PGM_GST_TYPE == PGM_TYPE_AMD64
        /*
         * The PMLE4.
         */
        rc = pgmGstGetLongModePML4PtrEx(pVCpu, &pWalk->pPml4);
        if (RT_SUCCESS(rc)) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnBadPhysAddr)(pVCpu, pWalk, 4, rc);

        PX86PML4E register  pPml4e;
        pWalk->pPml4e  = pPml4e  = &pWalk->pPml4->a[(GCPtr >> X86_PML4_SHIFT) & X86_PML4_MASK];
        X86PML4E  register  Pml4e;
        pWalk->Pml4e.u = Pml4e.u = pPml4e->u;

        if (Pml4e.n.u1Present) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnNotPresent)(pVCpu, pWalk, 4);

        if (RT_LIKELY(GST_IS_PML4E_VALID(pVCpu, Pml4e))) { /* likely */ }
        else return PGM_GST_NAME(WalkReturnRsvdError)(pVCpu, pWalk, 4);

        pWalk->Core.fEffective = fEffective = ((uint32_t)Pml4e.u & (X86_PML4E_RW  | X86_PML4E_US | X86_PML4E_PWT | X86_PML4E_PCD | X86_PML4E_A))
                                            | ((uint32_t)(Pml4e.u >> 63) ^ 1) /*NX */;

        /*
         * The PDPE.
         */
        rc = PGM_GCPHYS_2_PTR_BY_VMCPU(pVCpu, Pml4e.u & X86_PML4E_PG_MASK, &pWalk->pPdpt);
        if (RT_SUCCESS(rc)) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnBadPhysAddr)(pVCpu, pWalk, 3, rc);

# elif PGM_GST_TYPE == PGM_TYPE_PAE
        rc = pgmGstGetPaePDPTPtrEx(pVCpu, &pWalk->pPdpt);
        if (RT_SUCCESS(rc)) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnBadPhysAddr)(pVCpu, pWalk, 8, rc);
# endif
    }
    {
# if PGM_GST_TYPE == PGM_TYPE_AMD64 || PGM_GST_TYPE == PGM_TYPE_PAE
        PX86PDPE register   pPdpe;
        pWalk->pPdpe  = pPdpe  = &pWalk->pPdpt->a[(GCPtr >> GST_PDPT_SHIFT) & GST_PDPT_MASK];
        X86PDPE  register   Pdpe;
        pWalk->Pdpe.u = Pdpe.u = pPdpe->u;

        if (Pdpe.n.u1Present) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnNotPresent)(pVCpu, pWalk, 3);

        if (RT_LIKELY(GST_IS_PDPE_VALID(pVCpu, Pdpe))) { /* likely */ }
        else return PGM_GST_NAME(WalkReturnRsvdError)(pVCpu, pWalk, 3);

# if PGM_GST_TYPE == PGM_TYPE_AMD64
        pWalk->Core.fEffective = fEffective &= ((uint32_t)Pdpe.u & (X86_PDPE_RW  | X86_PDPE_US | X86_PDPE_PWT | X86_PDPE_PCD | X86_PDPE_A))
                                             | ((uint32_t)(Pdpe.u >> 63) ^ 1) /*NX */;
# else
        pWalk->Core.fEffective = fEffective  = X86_PDPE_RW  | X86_PDPE_US | X86_PDPE_A
                                             | ((uint32_t)Pdpe.u & (X86_PDPE_PWT | X86_PDPE_PCD))
                                             | ((uint32_t)(Pdpe.u >> 63) ^ 1) /*NX */;
# endif

        /*
         * The PDE.
         */
        rc = PGM_GCPHYS_2_PTR_BY_VMCPU(pVCpu, Pdpe.u & X86_PDPE_PG_MASK, &pWalk->pPd);
        if (RT_SUCCESS(rc)) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnBadPhysAddr)(pVCpu, pWalk, 2, rc);
# elif PGM_GST_TYPE == PGM_TYPE_32BIT
        rc = pgmGstGet32bitPDPtrEx(pVCpu, &pWalk->pPd);
        if (RT_SUCCESS(rc)) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnBadPhysAddr)(pVCpu, pWalk, 8, rc);
# endif
    }
    {
        PGSTPDE register    pPde;
        pWalk->pPde  = pPde  = &pWalk->pPd->a[(GCPtr >> GST_PD_SHIFT) & GST_PD_MASK];
        GSTPDE              Pde;
        pWalk->Pde.u = Pde.u = pPde->u;
        if (Pde.n.u1Present) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnNotPresent)(pVCpu, pWalk, 2);
        if (Pde.n.u1Size && GST_IS_PSE_ACTIVE(pVCpu))
        {
            if (RT_LIKELY(GST_IS_BIG_PDE_VALID(pVCpu, Pde))) { /* likely */ }
            else return PGM_GST_NAME(WalkReturnRsvdError)(pVCpu, pWalk, 2);

            /*
             * We're done.
             */
# if PGM_GST_TYPE == PGM_TYPE_32BIT
            fEffective &= Pde.u & (X86_PDE4M_RW  | X86_PDE4M_US | X86_PDE4M_PWT | X86_PDE4M_PCD | X86_PDE4M_A);
# else
            fEffective &= ((uint32_t)Pde.u & (X86_PDE4M_RW  | X86_PDE4M_US | X86_PDE4M_PWT | X86_PDE4M_PCD | X86_PDE4M_A))
                        | ((uint32_t)(Pde.u >> 63) ^ 1) /*NX */;
# endif
            fEffective |= (uint32_t)Pde.u & (X86_PDE4M_D | X86_PDE4M_G);
            fEffective |= (uint32_t)(Pde.u & X86_PDE4M_PAT) >> X86_PDE4M_PAT_SHIFT;
            pWalk->Core.fEffective = fEffective;

            pWalk->Core.fEffectiveRW = !!(fEffective & X86_PTE_RW);
            pWalk->Core.fEffectiveUS = !!(fEffective & X86_PTE_US);
# if PGM_GST_TYPE == PGM_TYPE_AMD64 || PGM_GST_TYPE == PGM_TYPE_PAE
            pWalk->Core.fEffectiveNX = !(fEffective & 1) && GST_IS_NX_ACTIVE(pVCpu);
# else
            pWalk->Core.fEffectiveNX = false;
# endif
            pWalk->Core.fBigPage     = true;
            pWalk->Core.fSucceeded   = true;

            pWalk->Core.GCPhys       = GST_GET_BIG_PDE_GCPHYS(pVCpu->CTX_SUFF(pVM), Pde)
                                     | (GCPtr & GST_BIG_PAGE_OFFSET_MASK);
            PGM_A20_APPLY_TO_VAR(pVCpu, pWalk->Core.GCPhys);
            return VINF_SUCCESS;
        }

        if (RT_UNLIKELY(!GST_IS_PDE_VALID(pVCpu, Pde)))
            return PGM_GST_NAME(WalkReturnRsvdError)(pVCpu, pWalk, 2);
# if PGM_GST_TYPE == PGM_TYPE_32BIT
        pWalk->Core.fEffective = fEffective &= Pde.u & (X86_PDE_RW  | X86_PDE_US | X86_PDE_PWT | X86_PDE_PCD | X86_PDE_A);
# else
        pWalk->Core.fEffective = fEffective &= ((uint32_t)Pde.u & (X86_PDE_RW  | X86_PDE_US | X86_PDE_PWT | X86_PDE_PCD | X86_PDE_A))
                                             | ((uint32_t)(Pde.u >> 63) ^ 1) /*NX */;
# endif

        /*
         * The PTE.
         */
        rc = PGM_GCPHYS_2_PTR_BY_VMCPU(pVCpu, GST_GET_PDE_GCPHYS(Pde), &pWalk->pPt);
        if (RT_SUCCESS(rc)) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnBadPhysAddr)(pVCpu, pWalk, 1, rc);
    }
    {
        PGSTPTE register    pPte;
        pWalk->pPte  = pPte  = &pWalk->pPt->a[(GCPtr >> GST_PT_SHIFT) & GST_PT_MASK];
        GSTPTE  register    Pte;
        pWalk->Pte.u = Pte.u = pPte->u;

        if (Pte.n.u1Present) { /* probable */ }
        else return PGM_GST_NAME(WalkReturnNotPresent)(pVCpu, pWalk, 1);

        if (RT_LIKELY(GST_IS_PTE_VALID(pVCpu, Pte))) { /* likely */ }
        else return PGM_GST_NAME(WalkReturnRsvdError)(pVCpu, pWalk, 1);

        /*
         * We're done.
         */
# if PGM_GST_TYPE == PGM_TYPE_32BIT
        fEffective &= Pte.u & (X86_PTE_RW  | X86_PTE_US | X86_PTE_PWT | X86_PTE_PCD | X86_PTE_A);
# else
        fEffective &= ((uint32_t)Pte.u & (X86_PTE_RW  | X86_PTE_US | X86_PTE_PWT | X86_PTE_PCD | X86_PTE_A))
                    | ((uint32_t)(Pte.u >> 63) ^ 1) /*NX */;
# endif
        fEffective |= (uint32_t)Pte.u & (X86_PTE_D | X86_PTE_PAT | X86_PTE_G);
        pWalk->Core.fEffective = fEffective;

        pWalk->Core.fEffectiveRW = !!(fEffective & X86_PTE_RW);
        pWalk->Core.fEffectiveUS = !!(fEffective & X86_PTE_US);
# if PGM_GST_TYPE == PGM_TYPE_AMD64 || PGM_GST_TYPE == PGM_TYPE_PAE
        pWalk->Core.fEffectiveNX = !(fEffective & 1) && GST_IS_NX_ACTIVE(pVCpu);
# else
        pWalk->Core.fEffectiveNX = false;
# endif
        pWalk->Core.fSucceeded   = true;

        pWalk->Core.GCPhys       = GST_GET_PDE_GCPHYS(Pte)
                                 | (GCPtr & PAGE_OFFSET_MASK);
        return VINF_SUCCESS;
    }
}

#endif /* 32BIT, PAE, AMD64 */

/**
 * Gets effective Guest OS page information.
 *
 * When GCPtr is in a big page, the function will return as if it was a normal
 * 4KB page. If the need for distinguishing between big and normal page becomes
 * necessary at a later point, a PGMGstGetPage Ex() will be created for that
 * purpose.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       Guest Context virtual address of the page.
 * @param   pfFlags     Where to store the flags. These are X86_PTE_*, even for big pages.
 * @param   pGCPhys     Where to store the GC physical address of the page.
 *                      This is page aligned!
 */
PGM_GST_DECL(int, GetPage)(PVMCPU pVCpu, RTGCPTR GCPtr, uint64_t *pfFlags, PRTGCPHYS pGCPhys)
{
#if PGM_GST_TYPE == PGM_TYPE_REAL \
 || PGM_GST_TYPE == PGM_TYPE_PROT
    /*
     * Fake it.
     */
    if (pfFlags)
        *pfFlags = X86_PTE_P | X86_PTE_RW | X86_PTE_US;
    if (pGCPhys)
        *pGCPhys = GCPtr & PAGE_BASE_GC_MASK;
    NOREF(pVCpu);
    return VINF_SUCCESS;

#elif PGM_GST_TYPE == PGM_TYPE_32BIT \
   || PGM_GST_TYPE == PGM_TYPE_PAE \
   || PGM_GST_TYPE == PGM_TYPE_AMD64

    GSTPTWALK Walk;
    int rc = PGM_GST_NAME(Walk)(pVCpu, GCPtr, &Walk);
    if (RT_FAILURE(rc))
        return rc;

    if (pGCPhys)
        *pGCPhys = Walk.Core.GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK;

    if (pfFlags)
    {
        if (!Walk.Core.fBigPage)
            *pfFlags = (Walk.Pte.u & ~(GST_PTE_PG_MASK | X86_PTE_RW | X86_PTE_US))                      /* NX not needed */
                     | (Walk.Core.fEffectiveRW ? X86_PTE_RW : 0)
                     | (Walk.Core.fEffectiveUS ? X86_PTE_US : 0)
# if PGM_WITH_NX(PGM_GST_TYPE, PGM_GST_TYPE)
                     | (Walk.Core.fEffectiveNX ? X86_PTE_PAE_NX : 0)
# endif
                     ;
        else
        {
            *pfFlags = (Walk.Pde.u & ~(GST_PTE_PG_MASK | X86_PDE4M_RW | X86_PDE4M_US | X86_PDE4M_PS))   /* NX not needed */
                     | ((Walk.Pde.u & X86_PDE4M_PAT) >> X86_PDE4M_PAT_SHIFT)
                     | (Walk.Core.fEffectiveRW ? X86_PTE_RW : 0)
                     | (Walk.Core.fEffectiveUS ? X86_PTE_US : 0)
# if PGM_WITH_NX(PGM_GST_TYPE, PGM_GST_TYPE)
                     | (Walk.Core.fEffectiveNX ? X86_PTE_PAE_NX : 0)
# endif
                     ;
        }
    }

    return VINF_SUCCESS;

#else
# error "shouldn't be here!"
    /* something else... */
    return VERR_NOT_SUPPORTED;
#endif
}


/**
 * Modify page flags for a range of pages in the guest's tables
 *
 * The existing flags are ANDed with the fMask and ORed with the fFlags.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       Virtual address of the first page in the range. Page aligned!
 * @param   cb          Size (in bytes) of the page range to apply the modification to. Page aligned!
 * @param   fFlags      The OR  mask - page flags X86_PTE_*, excluding the page mask of course.
 * @param   fMask       The AND mask - page flags X86_PTE_*.
 */
PGM_GST_DECL(int, ModifyPage)(PVMCPU pVCpu, RTGCPTR GCPtr, size_t cb, uint64_t fFlags, uint64_t fMask)
{
    Assert((cb & PAGE_OFFSET_MASK) == 0); RT_NOREF_PV(cb);

#if PGM_GST_TYPE == PGM_TYPE_32BIT \
 || PGM_GST_TYPE == PGM_TYPE_PAE \
 || PGM_GST_TYPE == PGM_TYPE_AMD64
    for (;;)
    {
        GSTPTWALK Walk;
        int rc = PGM_GST_NAME(Walk)(pVCpu, GCPtr, &Walk);
        if (RT_FAILURE(rc))
            return rc;

        if (!Walk.Core.fBigPage)
        {
            /*
             * 4KB Page table, process
             *
             * Walk pages till we're done.
             */
            unsigned iPTE = (GCPtr >> GST_PT_SHIFT) & GST_PT_MASK;
            while (iPTE < RT_ELEMENTS(Walk.pPt->a))
            {
                GSTPTE Pte = Walk.pPt->a[iPTE];
                Pte.u = (Pte.u & (fMask | X86_PTE_PAE_PG_MASK))
                      | (fFlags & ~GST_PTE_PG_MASK);
                Walk.pPt->a[iPTE] = Pte;

                /* next page */
                cb -= PAGE_SIZE;
                if (!cb)
                    return VINF_SUCCESS;
                GCPtr += PAGE_SIZE;
                iPTE++;
            }
        }
        else
        {
            /*
             * 2/4MB Page table
             */
            GSTPDE PdeNew;
# if PGM_GST_TYPE == PGM_TYPE_32BIT
            PdeNew.u = (Walk.Pde.u & (fMask | ((fMask & X86_PTE_PAT) << X86_PDE4M_PAT_SHIFT) | GST_PDE_BIG_PG_MASK | X86_PDE4M_PG_HIGH_MASK | X86_PDE4M_PS))
# else
            PdeNew.u = (Walk.Pde.u & (fMask | ((fMask & X86_PTE_PAT) << X86_PDE4M_PAT_SHIFT) | GST_PDE_BIG_PG_MASK | X86_PDE4M_PS))
# endif
                     | (fFlags & ~GST_PTE_PG_MASK)
                     | ((fFlags & X86_PTE_PAT) << X86_PDE4M_PAT_SHIFT);
            *Walk.pPde = PdeNew;

            /* advance */
            const unsigned cbDone = GST_BIG_PAGE_SIZE - (GCPtr & GST_BIG_PAGE_OFFSET_MASK);
            if (cbDone >= cb)
                return VINF_SUCCESS;
            cb    -= cbDone;
            GCPtr += cbDone;
        }
    }

#else
    /* real / protected mode: ignore. */
    NOREF(pVCpu); NOREF(GCPtr); NOREF(fFlags); NOREF(fMask);
    return VINF_SUCCESS;
#endif
}


/**
 * Retrieve guest PDE information.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       Guest context pointer.
 * @param   pPDE        Pointer to guest PDE structure.
 */
PGM_GST_DECL(int, GetPDE)(PVMCPU pVCpu, RTGCPTR GCPtr, PX86PDEPAE pPDE)
{
#if PGM_GST_TYPE == PGM_TYPE_32BIT \
 || PGM_GST_TYPE == PGM_TYPE_PAE   \
 || PGM_GST_TYPE == PGM_TYPE_AMD64

# if PGM_GST_TYPE != PGM_TYPE_AMD64
    /* Boundary check. */
    if (RT_UNLIKELY(GCPtr >= _4G))
        return VERR_PAGE_TABLE_NOT_PRESENT;
# endif

# if PGM_GST_TYPE == PGM_TYPE_32BIT
    unsigned    iPd = (GCPtr >> GST_PD_SHIFT) & GST_PD_MASK;
    PX86PD      pPd = pgmGstGet32bitPDPtr(pVCpu);

# elif PGM_GST_TYPE == PGM_TYPE_PAE
    unsigned    iPd = 0;                /* shut up gcc */
    PCX86PDPAE  pPd = pgmGstGetPaePDPtr(pVCpu, GCPtr, &iPd, NULL);

# elif PGM_GST_TYPE == PGM_TYPE_AMD64
    PX86PML4E   pPml4eIgn;
    X86PDPE     PdpeIgn;
    unsigned    iPd = 0;                /* shut up gcc */
    PCX86PDPAE  pPd = pgmGstGetLongModePDPtr(pVCpu, GCPtr, &pPml4eIgn, &PdpeIgn, &iPd);
    /* Note! We do not return an effective PDE here like we do for the PTE in GetPage method. */
# endif

    if (RT_LIKELY(pPd))
        pPDE->u = (X86PGPAEUINT)pPd->a[iPd].u;
    else
        pPDE->u = 0;
    return VINF_SUCCESS;

#else
    NOREF(pVCpu); NOREF(GCPtr); NOREF(pPDE);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
#endif
}


#if (   PGM_GST_TYPE == PGM_TYPE_32BIT \
     || PGM_GST_TYPE == PGM_TYPE_PAE \
     || PGM_GST_TYPE == PGM_TYPE_AMD64) \
 && defined(VBOX_WITH_RAW_MODE)
/**
 * Updates one virtual handler range.
 *
 * @returns 0
 * @param   pNode   Pointer to a PGMVIRTHANDLER.
 * @param   pvUser  Pointer to a PGMVHUARGS structure (see PGM.cpp).
 */
static DECLCALLBACK(int) PGM_GST_NAME(VirtHandlerUpdateOne)(PAVLROGCPTRNODECORE pNode, void *pvUser)
{
    PPGMHVUSTATE            pState   = (PPGMHVUSTATE)pvUser;
    PVM                     pVM      = pState->pVM;
    PVMCPU                  pVCpu    = pState->pVCpu;
    PPGMVIRTHANDLER         pCur     = (PPGMVIRTHANDLER)pNode;
    PPGMVIRTHANDLERTYPEINT  pCurType = PGMVIRTANDLER_GET_TYPE(pVM, pCur);

    Assert(pCurType->enmKind != PGMVIRTHANDLERKIND_HYPERVISOR); NOREF(pCurType);

# if PGM_GST_TYPE == PGM_TYPE_32BIT
    PX86PD          pPDSrc = pgmGstGet32bitPDPtr(pVCpu);
# endif

    RTGCPTR         GCPtr  = pCur->Core.Key;
# if PGM_GST_TYPE != PGM_TYPE_AMD64
    /* skip all stuff above 4GB if not AMD64 mode. */
    if (RT_UNLIKELY(GCPtr >= _4G))
        return 0;
# endif

    unsigned        offPage = GCPtr & PAGE_OFFSET_MASK;
    unsigned        iPage = 0;
    while (iPage < pCur->cPages)
    {
# if PGM_GST_TYPE == PGM_TYPE_32BIT
        X86PDE      Pde = pPDSrc->a[GCPtr >> X86_PD_SHIFT];
# elif PGM_GST_TYPE == PGM_TYPE_PAE
        X86PDEPAE   Pde = pgmGstGetPaePDE(pVCpu, GCPtr);
# elif PGM_GST_TYPE == PGM_TYPE_AMD64
        X86PDEPAE   Pde = pgmGstGetLongModePDE(pVCpu, GCPtr);
# endif
# if PGM_GST_TYPE == PGM_TYPE_32BIT
        bool const  fBigPage = Pde.b.u1Size && (pState->cr4 & X86_CR4_PSE);
# else
        bool const  fBigPage = Pde.b.u1Size;
# endif
        if (    Pde.n.u1Present
            &&  (  !fBigPage
                 ? GST_IS_PDE_VALID(pVCpu, Pde)
                 : GST_IS_BIG_PDE_VALID(pVCpu, Pde)) )
        {
            if (!fBigPage)
            {
                /*
                 * Normal page table.
                 */
                PGSTPT pPT;
                int rc = PGM_GCPHYS_2_PTR_V2(pVM, pVCpu, GST_GET_PDE_GCPHYS(Pde), &pPT);
                if (RT_SUCCESS(rc))
                {
                    for (unsigned iPTE = (GCPtr >> GST_PT_SHIFT) & GST_PT_MASK;
                         iPTE < RT_ELEMENTS(pPT->a) && iPage < pCur->cPages;
                         iPTE++, iPage++, GCPtr += PAGE_SIZE, offPage = 0)
                    {
                        GSTPTE      Pte = pPT->a[iPTE];
                        RTGCPHYS    GCPhysNew;
                        if (Pte.n.u1Present)
                            GCPhysNew = PGM_A20_APPLY(pVCpu, (RTGCPHYS)(pPT->a[iPTE].u & GST_PTE_PG_MASK) + offPage);
                        else
                            GCPhysNew = NIL_RTGCPHYS;
                        if (pCur->aPhysToVirt[iPage].Core.Key != GCPhysNew)
                        {
                            if (pCur->aPhysToVirt[iPage].Core.Key != NIL_RTGCPHYS)
                                pgmHandlerVirtualClearPage(pVM, pCur, iPage);
#ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
                            AssertReleaseMsg(!pCur->aPhysToVirt[iPage].offNextAlias,
                                             ("{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32} GCPhysNew=%RGp\n",
                                              pCur->aPhysToVirt[iPage].Core.Key, pCur->aPhysToVirt[iPage].Core.KeyLast,
                                              pCur->aPhysToVirt[iPage].offVirtHandler, pCur->aPhysToVirt[iPage].offNextAlias, GCPhysNew));
#endif
                            pCur->aPhysToVirt[iPage].Core.Key = GCPhysNew;
                            pState->fTodo |= PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL;
                        }
                    }
                }
                else
                {
                    /* not-present. */
                    offPage = 0;
                    AssertRC(rc);
                    for (unsigned iPTE = (GCPtr >> GST_PT_SHIFT) & GST_PT_MASK;
                         iPTE < RT_ELEMENTS(pPT->a) && iPage < pCur->cPages;
                         iPTE++, iPage++, GCPtr += PAGE_SIZE)
                    {
                        if (pCur->aPhysToVirt[iPage].Core.Key != NIL_RTGCPHYS)
                        {
                            pgmHandlerVirtualClearPage(pVM, pCur, iPage);
#ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
                            AssertReleaseMsg(!pCur->aPhysToVirt[iPage].offNextAlias,
                                             ("{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32}\n",
                                              pCur->aPhysToVirt[iPage].Core.Key, pCur->aPhysToVirt[iPage].Core.KeyLast,
                                              pCur->aPhysToVirt[iPage].offVirtHandler, pCur->aPhysToVirt[iPage].offNextAlias));
#endif
                            pCur->aPhysToVirt[iPage].Core.Key = NIL_RTGCPHYS;
                            pState->fTodo |= PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL;
                        }
                    }
                }
            }
            else
            {
                /*
                 * 2/4MB page.
                 */
                RTGCPHYS GCPhys = (RTGCPHYS)GST_GET_PDE_GCPHYS(Pde);
                for (unsigned i4KB = (GCPtr >> GST_PT_SHIFT) & GST_PT_MASK;
                     i4KB < PAGE_SIZE / sizeof(GSTPDE) && iPage < pCur->cPages;
                     i4KB++, iPage++, GCPtr += PAGE_SIZE, offPage = 0)
                {
                    RTGCPHYS GCPhysNew = PGM_A20_APPLY(pVCpu, GCPhys + (i4KB << PAGE_SHIFT) + offPage);
                    if (pCur->aPhysToVirt[iPage].Core.Key != GCPhysNew)
                    {
                        if (pCur->aPhysToVirt[iPage].Core.Key != NIL_RTGCPHYS)
                            pgmHandlerVirtualClearPage(pVM, pCur, iPage);
#ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
                        AssertReleaseMsg(!pCur->aPhysToVirt[iPage].offNextAlias,
                                         ("{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32} GCPhysNew=%RGp\n",
                                          pCur->aPhysToVirt[iPage].Core.Key, pCur->aPhysToVirt[iPage].Core.KeyLast,
                                          pCur->aPhysToVirt[iPage].offVirtHandler, pCur->aPhysToVirt[iPage].offNextAlias, GCPhysNew));
#endif
                        pCur->aPhysToVirt[iPage].Core.Key = GCPhysNew;
                        pState->fTodo |= PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL;
                    }
                }
            } /* pde type */
        }
        else
        {
            /* not-present / invalid. */
            Log(("VirtHandler: Not present / invalid Pde=%RX64\n", (uint64_t)Pde.u));
            for (unsigned cPages = (GST_PT_MASK + 1) - ((GCPtr >> GST_PT_SHIFT) & GST_PT_MASK);
                 cPages && iPage < pCur->cPages;
                 iPage++, GCPtr += PAGE_SIZE)
            {
                if (pCur->aPhysToVirt[iPage].Core.Key != NIL_RTGCPHYS)
                {
                    pgmHandlerVirtualClearPage(pVM, pCur, iPage);
                    pCur->aPhysToVirt[iPage].Core.Key = NIL_RTGCPHYS;
                    pState->fTodo |= PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL;
                }
            }
            offPage = 0;
        }
    } /* for pages in virtual mapping. */

    return 0;
}
#endif /* 32BIT, PAE and AMD64 + VBOX_WITH_RAW_MODE */


/**
 * Updates the virtual page access handlers.
 *
 * @returns true if bits were flushed.
 * @returns false if bits weren't flushed.
 * @param   pVM     The cross context VM structure.
 * @param   cr4     The cr4 register value.
 */
PGM_GST_DECL(bool, HandlerVirtualUpdate)(PVM pVM, uint32_t cr4)
{
#if (   PGM_GST_TYPE == PGM_TYPE_32BIT \
     || PGM_GST_TYPE == PGM_TYPE_PAE \
     || PGM_GST_TYPE == PGM_TYPE_AMD64) \
 && defined(VBOX_WITH_RAW_MODE)

    /** @todo
     * In theory this is not sufficient: the guest can change a single page in a range with invlpg
     */

    /*
     * Resolve any virtual address based access handlers to GC physical addresses.
     * This should be fairly quick.
     */
    RTUINT      fTodo = 0;

    pgmLock(pVM);
    STAM_PROFILE_START(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncCR3HandlerVirtualUpdate), a);

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PGMHVUSTATE State;
        PVMCPU      pVCpu = &pVM->aCpus[i];

        State.pVM   = pVM;
        State.pVCpu = pVCpu;
        State.fTodo = pVCpu->pgm.s.fSyncFlags;
        State.cr4   = cr4;
        RTAvlroGCPtrDoWithAll(&pVM->pgm.s.CTX_SUFF(pTrees)->VirtHandlers, true, PGM_GST_NAME(VirtHandlerUpdateOne), &State);

        fTodo |= State.fTodo;
    }
    STAM_PROFILE_STOP(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncCR3HandlerVirtualUpdate), a);


    /*
     * Set / reset bits?
     */
    if (fTodo & PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL)
    {
        STAM_PROFILE_START(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncCR3HandlerVirtualReset), b);
        Log(("HandlerVirtualUpdate: resets bits\n"));
        RTAvlroGCPtrDoWithAll(&pVM->pgm.s.CTX_SUFF(pTrees)->VirtHandlers, true, pgmHandlerVirtualResetOne, pVM);

        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            PVMCPU pVCpu = &pVM->aCpus[i];
            pVCpu->pgm.s.fSyncFlags &= ~PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL;
        }

        STAM_PROFILE_STOP(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,SyncCR3HandlerVirtualReset), b);
    }
    pgmUnlock(pVM);

    return !!(fTodo & PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL);

#else /* real / protected */
    NOREF(pVM); NOREF(cr4);
    return false;
#endif
}


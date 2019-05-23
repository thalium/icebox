/* $Id: PGMInline.h $ */
/** @file
 * PGM - Inlined functions.
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

#ifndef ___PGMInline_h
#define ___PGMInline_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/err.h>
#include <VBox/vmm/stam.h>
#include <VBox/param.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/dis.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/log.h>
#include <VBox/vmm/gmm.h>
#include <VBox/vmm/hm.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/avl.h>
#include <iprt/critsect.h>
#include <iprt/sha.h>



/** @addtogroup grp_pgm_int   Internals
 * @internal
 * @{
 */

/**
 * Gets the PGMRAMRANGE structure for a guest page.
 *
 * @returns Pointer to the RAM range on success.
 * @returns NULL on a VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS condition.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address.
 */
DECLINLINE(PPGMRAMRANGE) pgmPhysGetRange(PVM pVM, RTGCPHYS GCPhys)
{
    PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
    if (!pRam || GCPhys - pRam->GCPhys >= pRam->cb)
        return pgmPhysGetRangeSlow(pVM, GCPhys);
    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,RamRangeTlbHits));
    return pRam;
}


/**
 * Gets the PGMRAMRANGE structure for a guest page, if unassigned get the ram
 * range above it.
 *
 * @returns Pointer to the RAM range on success.
 * @returns NULL if the address is located after the last range.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address.
 */
DECLINLINE(PPGMRAMRANGE) pgmPhysGetRangeAtOrAbove(PVM pVM, RTGCPHYS GCPhys)
{
    PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
    if (   !pRam
        || (GCPhys - pRam->GCPhys) >= pRam->cb)
        return pgmPhysGetRangeAtOrAboveSlow(pVM, GCPhys);
    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,RamRangeTlbHits));
    return pRam;
}


/**
 * Gets the PGMPAGE structure for a guest page.
 *
 * @returns Pointer to the page on success.
 * @returns NULL on a VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS condition.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address.
 */
DECLINLINE(PPGMPAGE) pgmPhysGetPage(PVM pVM, RTGCPHYS GCPhys)
{
    PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
    RTGCPHYS off;
    if (   !pRam
        || (off = GCPhys - pRam->GCPhys) >= pRam->cb)
        return pgmPhysGetPageSlow(pVM, GCPhys);
    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,RamRangeTlbHits));
    return &pRam->aPages[off >> PAGE_SHIFT];
}


/**
 * Gets the PGMPAGE structure for a guest page.
 *
 * Old Phys code: Will make sure the page is present.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS and a valid *ppPage on success.
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if the address isn't valid.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address.
 * @param   ppPage      Where to store the page pointer on success.
 */
DECLINLINE(int) pgmPhysGetPageEx(PVM pVM, RTGCPHYS GCPhys, PPPGMPAGE ppPage)
{
    PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
    RTGCPHYS off;
    if (   !pRam
        || (off = GCPhys - pRam->GCPhys) >= pRam->cb)
        return pgmPhysGetPageExSlow(pVM, GCPhys, ppPage);
    *ppPage = &pRam->aPages[(GCPhys - pRam->GCPhys) >> PAGE_SHIFT];
    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,RamRangeTlbHits));
    return VINF_SUCCESS;
}


/**
 * Gets the PGMPAGE structure for a guest page.
 *
 * Old Phys code: Will make sure the page is present.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS and a valid *ppPage on success.
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if the address isn't valid.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address.
 * @param   ppPage      Where to store the page pointer on success.
 * @param   ppRamHint   Where to read and store the ram list hint.
 *                      The caller initializes this to NULL before the call.
 */
DECLINLINE(int) pgmPhysGetPageWithHintEx(PVM pVM, RTGCPHYS GCPhys, PPPGMPAGE ppPage, PPGMRAMRANGE *ppRamHint)
{
    RTGCPHYS off;
    PPGMRAMRANGE pRam = *ppRamHint;
    if (    !pRam
        ||  RT_UNLIKELY((off = GCPhys - pRam->GCPhys) >= pRam->cb))
    {
        pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
        if (   !pRam
            || (off = GCPhys - pRam->GCPhys) >= pRam->cb)
            return pgmPhysGetPageAndRangeExSlow(pVM, GCPhys, ppPage, ppRamHint);

        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,RamRangeTlbHits));
        *ppRamHint = pRam;
    }
    *ppPage = &pRam->aPages[off >> PAGE_SHIFT];
    return VINF_SUCCESS;
}


/**
 * Gets the PGMPAGE structure for a guest page together with the PGMRAMRANGE.
 *
 * @returns Pointer to the page on success.
 * @returns NULL on a VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS condition.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address.
 * @param   ppPage      Where to store the pointer to the PGMPAGE structure.
 * @param   ppRam       Where to store the pointer to the PGMRAMRANGE structure.
 */
DECLINLINE(int) pgmPhysGetPageAndRangeEx(PVM pVM, RTGCPHYS GCPhys, PPPGMPAGE ppPage, PPGMRAMRANGE *ppRam)
{
    PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
    RTGCPHYS off;
    if (   !pRam
        || (off = GCPhys - pRam->GCPhys) >= pRam->cb)
        return pgmPhysGetPageAndRangeExSlow(pVM, GCPhys, ppPage, ppRam);

    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,RamRangeTlbHits));
    *ppRam = pRam;
    *ppPage = &pRam->aPages[off >> PAGE_SHIFT];
    return VINF_SUCCESS;
}


/**
 * Convert GC Phys to HC Phys.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address.
 * @param   pHCPhys     Where to store the corresponding HC physical address.
 *
 * @deprecated  Doesn't deal with zero, shared or write monitored pages.
 *              Avoid when writing new code!
 */
DECLINLINE(int) pgmRamGCPhys2HCPhys(PVM pVM, RTGCPHYS GCPhys, PRTHCPHYS pHCPhys)
{
    PPGMPAGE pPage;
    int rc = pgmPhysGetPageEx(pVM, GCPhys, &pPage);
    if (RT_FAILURE(rc))
        return rc;
    *pHCPhys = PGM_PAGE_GET_HCPHYS(pPage) | (GCPhys & PAGE_OFFSET_MASK);
    return VINF_SUCCESS;
}

#if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)

/**
 * Inlined version of the ring-0 version of the host page mapping code
 * that optimizes access to pages already in the set.
 *
 * @returns VINF_SUCCESS. Will bail out to ring-3 on failure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   HCPhys      The physical address of the page.
 * @param   ppv         Where to store the mapping address.
 * @param   SRC_POS     The source location of the caller.
 */
DECLINLINE(int) pgmRZDynMapHCPageInlined(PVMCPU pVCpu, RTHCPHYS HCPhys, void **ppv RTLOG_COMMA_SRC_POS_DECL)
{
    PPGMMAPSET  pSet    = &pVCpu->pgm.s.AutoSet;

    STAM_PROFILE_START(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPageInl, a);
    Assert(!(HCPhys & PAGE_OFFSET_MASK));
    Assert(pSet->cEntries <= RT_ELEMENTS(pSet->aEntries));

    unsigned    iHash   = PGMMAPSET_HASH(HCPhys);
    unsigned    iEntry  = pSet->aiHashTable[iHash];
    if (    iEntry < pSet->cEntries
        &&  pSet->aEntries[iEntry].HCPhys == HCPhys
        &&  pSet->aEntries[iEntry].cInlinedRefs < UINT16_MAX - 1)
    {
        pSet->aEntries[iEntry].cInlinedRefs++;
        *ppv = pSet->aEntries[iEntry].pvPage;
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPageInlHits);
    }
    else
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPageInlMisses);
        pgmRZDynMapHCPageCommon(pSet, HCPhys, ppv RTLOG_COMMA_SRC_POS_ARGS);
    }

    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPageInl, a);
    return VINF_SUCCESS;
}


/**
 * Inlined version of the guest page mapping code that optimizes access to pages
 * already in the set.
 *
 * @returns VBox status code, see pgmRZDynMapGCPageCommon for details.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPhys      The guest physical address of the page.
 * @param   ppv         Where to store the mapping address.
 * @param   SRC_POS     The source location of the caller.
 */
DECLINLINE(int) pgmRZDynMapGCPageV2Inlined(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, void **ppv RTLOG_COMMA_SRC_POS_DECL)
{
    STAM_PROFILE_START(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInl, a);
    AssertMsg(!(GCPhys & PAGE_OFFSET_MASK), ("%RGp\n", GCPhys));

    /*
     * Get the ram range.
     */
    PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
    RTGCPHYS off;
    if (   !pRam
        || (off = GCPhys - pRam->GCPhys) >= pRam->cb
        /** @todo   || page state stuff */
       )
    {
        /* This case is not counted into StatRZDynMapGCPageInl. */
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlRamMisses);
        return pgmRZDynMapGCPageCommon(pVM, pVCpu, GCPhys, ppv RTLOG_COMMA_SRC_POS_ARGS);
    }

    RTHCPHYS HCPhys = PGM_PAGE_GET_HCPHYS(&pRam->aPages[off >> PAGE_SHIFT]);
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlRamHits);

    /*
     * pgmRZDynMapHCPageInlined with out stats.
     */
    PPGMMAPSET pSet = &pVCpu->pgm.s.AutoSet;
    Assert(!(HCPhys & PAGE_OFFSET_MASK));
    Assert(pSet->cEntries <= RT_ELEMENTS(pSet->aEntries));

    unsigned    iHash   = PGMMAPSET_HASH(HCPhys);
    unsigned    iEntry  = pSet->aiHashTable[iHash];
    if (    iEntry < pSet->cEntries
        &&  pSet->aEntries[iEntry].HCPhys == HCPhys
        &&  pSet->aEntries[iEntry].cInlinedRefs < UINT16_MAX - 1)
    {
        pSet->aEntries[iEntry].cInlinedRefs++;
        *ppv = pSet->aEntries[iEntry].pvPage;
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlHits);
    }
    else
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlMisses);
        pgmRZDynMapHCPageCommon(pSet, HCPhys, ppv RTLOG_COMMA_SRC_POS_ARGS);
    }

    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInl, a);
    return VINF_SUCCESS;
}


/**
 * Inlined version of the ring-0 version of guest page mapping that optimizes
 * access to pages already in the set.
 *
 * @returns VBox status code, see pgmRZDynMapGCPageCommon for details.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPhys      The guest physical address of the page.
 * @param   ppv         Where to store the mapping address.
 * @param   SRC_POS     The source location of the caller.
 */
DECLINLINE(int) pgmRZDynMapGCPageInlined(PVMCPU pVCpu, RTGCPHYS GCPhys, void **ppv RTLOG_COMMA_SRC_POS_DECL)
{
    return pgmRZDynMapGCPageV2Inlined(pVCpu->CTX_SUFF(pVM), pVCpu, GCPhys, ppv RTLOG_COMMA_SRC_POS_ARGS);
}


/**
 * Inlined version of the ring-0 version of the guest byte mapping code
 * that optimizes access to pages already in the set.
 *
 * @returns VBox status code, see pgmRZDynMapGCPageCommon for details.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPhys      The guest physical address of the page.
 * @param   ppv         Where to store the mapping address. The offset is
 *                      preserved.
 * @param   SRC_POS     The source location of the caller.
 */
DECLINLINE(int) pgmRZDynMapGCPageOffInlined(PVMCPU pVCpu, RTGCPHYS GCPhys, void **ppv RTLOG_COMMA_SRC_POS_DECL)
{
    STAM_PROFILE_START(&pVCpu->pgm.s.StatRZDynMapGCPageInl, a);

    /*
     * Get the ram range.
     */
    PVM             pVM  = pVCpu->CTX_SUFF(pVM);
    PPGMRAMRANGE    pRam = pVM->pgm.s.CTX_SUFF(apRamRangesTlb)[PGM_RAMRANGE_TLB_IDX(GCPhys)];
    RTGCPHYS        off;
    if (   !pRam
        || (off = GCPhys - pRam->GCPhys) >= pRam->cb
        /** @todo   || page state stuff */
       )
    {
        /* This case is not counted into StatRZDynMapGCPageInl. */
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlRamMisses);
        return pgmRZDynMapGCPageCommon(pVM, pVCpu, GCPhys, ppv RTLOG_COMMA_SRC_POS_ARGS);
    }

    RTHCPHYS HCPhys = PGM_PAGE_GET_HCPHYS(&pRam->aPages[off >> PAGE_SHIFT]);
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlRamHits);

    /*
     * pgmRZDynMapHCPageInlined with out stats.
     */
    PPGMMAPSET pSet = &pVCpu->pgm.s.AutoSet;
    Assert(!(HCPhys & PAGE_OFFSET_MASK));
    Assert(pSet->cEntries <= RT_ELEMENTS(pSet->aEntries));

    unsigned    iHash   = PGMMAPSET_HASH(HCPhys);
    unsigned    iEntry  = pSet->aiHashTable[iHash];
    if (    iEntry < pSet->cEntries
        &&  pSet->aEntries[iEntry].HCPhys == HCPhys
        &&  pSet->aEntries[iEntry].cInlinedRefs < UINT16_MAX - 1)
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlHits);
        pSet->aEntries[iEntry].cInlinedRefs++;
        *ppv = (void *)((uintptr_t)pSet->aEntries[iEntry].pvPage | (PAGE_OFFSET_MASK & (uintptr_t)GCPhys));
    }
    else
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInlMisses);
        pgmRZDynMapHCPageCommon(pSet, HCPhys, ppv RTLOG_COMMA_SRC_POS_ARGS);
        *ppv = (void *)((uintptr_t)*ppv | (PAGE_OFFSET_MASK & (uintptr_t)GCPhys));
    }

    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapGCPageInl, a);
    return VINF_SUCCESS;
}

#endif /* VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0 */
#if defined(IN_RC) || defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0)

/**
 * Maps the page into current context (RC and maybe R0).
 *
 * @returns pointer to the mapping.
 * @param   pVM         The cross context VM structure.
 * @param   pPage       The page.
 * @param   SRC_POS     The source location of the caller.
 */
DECLINLINE(void *) pgmPoolMapPageInlined(PVM pVM, PPGMPOOLPAGE pPage RTLOG_COMMA_SRC_POS_DECL)
{
    if (pPage->idx >= PGMPOOL_IDX_FIRST)
    {
        Assert(pPage->idx < pVM->pgm.s.CTX_SUFF(pPool)->cCurPages);
        void *pv;
        pgmRZDynMapHCPageInlined(VMMGetCpu(pVM), pPage->Core.Key, &pv RTLOG_COMMA_SRC_POS_ARGS);
        return pv;
    }
    AssertFatalMsgFailed(("pgmPoolMapPageInlined invalid page index %x\n", pPage->idx));
}


/**
 * Maps the page into current context (RC and maybe R0).
 *
 * @returns pointer to the mapping.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pPage       The page.
 * @param   SRC_POS     The source location of the caller.
 */
DECLINLINE(void *) pgmPoolMapPageV2Inlined(PVM pVM, PVMCPU pVCpu, PPGMPOOLPAGE pPage RTLOG_COMMA_SRC_POS_DECL)
{
    if (pPage->idx >= PGMPOOL_IDX_FIRST)
    {
        Assert(pPage->idx < pVM->pgm.s.CTX_SUFF(pPool)->cCurPages);
        void *pv;
        Assert(pVCpu == VMMGetCpu(pVM)); RT_NOREF_PV(pVM);
        pgmRZDynMapHCPageInlined(pVCpu, pPage->Core.Key, &pv RTLOG_COMMA_SRC_POS_ARGS);
        return pv;
    }
    AssertFatalMsgFailed(("pgmPoolMapPageV2Inlined invalid page index %x\n", pPage->idx));
}

#endif /*  VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0 || IN_RC */
#ifndef IN_RC

/**
 * Queries the Physical TLB entry for a physical guest page,
 * attempting to load the TLB entry if necessary.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if it's not a valid physical address.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The address of the guest page.
 * @param   ppTlbe      Where to store the pointer to the TLB entry.
 */
DECLINLINE(int) pgmPhysPageQueryTlbe(PVM pVM, RTGCPHYS GCPhys, PPPGMPAGEMAPTLBE ppTlbe)
{
    int rc;
    PPGMPAGEMAPTLBE pTlbe = &pVM->pgm.s.CTXSUFF(PhysTlb).aEntries[PGM_PAGEMAPTLB_IDX(GCPhys)];
    if (pTlbe->GCPhys == (GCPhys & X86_PTE_PAE_PG_MASK))
    {
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageMapTlbHits));
        rc = VINF_SUCCESS;
    }
    else
        rc = pgmPhysPageLoadIntoTlb(pVM, GCPhys);
    *ppTlbe = pTlbe;
    return rc;
}


/**
 * Queries the Physical TLB entry for a physical guest page,
 * attempting to load the TLB entry if necessary.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success
 * @retval  VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS if it's not a valid physical address.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPage       Pointer to the PGMPAGE structure corresponding to
 *                      GCPhys.
 * @param   GCPhys      The address of the guest page.
 * @param   ppTlbe      Where to store the pointer to the TLB entry.
 */
DECLINLINE(int) pgmPhysPageQueryTlbeWithPage(PVM pVM, PPGMPAGE pPage, RTGCPHYS GCPhys, PPPGMPAGEMAPTLBE ppTlbe)
{
    int rc;
    PPGMPAGEMAPTLBE pTlbe = &pVM->pgm.s.CTXSUFF(PhysTlb).aEntries[PGM_PAGEMAPTLB_IDX(GCPhys)];
    if (pTlbe->GCPhys == (GCPhys & X86_PTE_PAE_PG_MASK))
    {
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PageMapTlbHits));
        rc = VINF_SUCCESS;
# if 0 //def VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
#  ifdef IN_RING3
        if (pTlbe->pv == (void *)pVM->pgm.s.pvZeroPgR0)
#  else
        if (pTlbe->pv == (void *)pVM->pgm.s.pvZeroPgR3)
#  endif
            pTlbe->pv = pVM->pgm.s.CTX_SUFF(pvZeroPg);
# endif
        AssertPtr(pTlbe->pv);
# ifndef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
        Assert(!pTlbe->pMap || RT_VALID_PTR(pTlbe->pMap->pv));
# endif
    }
    else
        rc = pgmPhysPageLoadIntoTlbWithPage(pVM, pPage, GCPhys);
    *ppTlbe = pTlbe;
    return rc;
}

#endif /* !IN_RC */

/**
 * Enables write monitoring for an allocated page.
 *
 * The caller is responsible for updating the shadow page tables.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPage       The page to write monitor.
 * @param   GCPhysPage  The address of the page.
 */
DECLINLINE(void) pgmPhysPageWriteMonitor(PVM pVM, PPGMPAGE pPage, RTGCPHYS GCPhysPage)
{
    Assert(PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED);
    PGM_LOCK_ASSERT_OWNER(pVM);

    PGM_PAGE_SET_STATE(pVM, pPage, PGM_PAGE_STATE_WRITE_MONITORED);
    pVM->pgm.s.cMonitoredPages++;

    /* Large pages must disabled. */
    if (PGM_PAGE_GET_PDE_TYPE(pPage) == PGM_PAGE_PDE_TYPE_PDE)
    {
        PPGMPAGE pFirstPage = pgmPhysGetPage(pVM, GCPhysPage & X86_PDE2M_PAE_PG_MASK);
        AssertFatal(pFirstPage);
        if (PGM_PAGE_GET_PDE_TYPE(pFirstPage) == PGM_PAGE_PDE_TYPE_PDE)
        {
            PGM_PAGE_SET_PDE_TYPE(pVM, pFirstPage, PGM_PAGE_PDE_TYPE_PDE_DISABLED);
            pVM->pgm.s.cLargePagesDisabled++;
        }
        else
            Assert(PGM_PAGE_GET_PDE_TYPE(pFirstPage) == PGM_PAGE_PDE_TYPE_PDE_DISABLED);
    }
}


/**
 * Checks if the no-execute (NX) feature is active (EFER.NXE=1).
 *
 * Only used when the guest is in PAE or long mode.  This is inlined so that we
 * can perform consistency checks in debug builds.
 *
 * @returns true if it is, false if it isn't.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECL_FORCE_INLINE(bool) pgmGstIsNoExecuteActive(PVMCPU pVCpu)
{
    Assert(pVCpu->pgm.s.fNoExecuteEnabled == CPUMIsGuestNXEnabled(pVCpu));
    Assert(CPUMIsGuestInPAEMode(pVCpu) || CPUMIsGuestInLongMode(pVCpu));
    return pVCpu->pgm.s.fNoExecuteEnabled;
}


/**
 * Checks if the page size extension (PSE) is currently enabled (CR4.PSE=1).
 *
 * Only used when the guest is in paged 32-bit mode.  This is inlined so that
 * we can perform consistency checks in debug builds.
 *
 * @returns true if it is, false if it isn't.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECL_FORCE_INLINE(bool) pgmGst32BitIsPageSizeExtActive(PVMCPU pVCpu)
{
    Assert(pVCpu->pgm.s.fGst32BitPageSizeExtension == CPUMIsGuestPageSizeExtEnabled(pVCpu));
    Assert(!CPUMIsGuestInPAEMode(pVCpu));
    Assert(!CPUMIsGuestInLongMode(pVCpu));
    return pVCpu->pgm.s.fGst32BitPageSizeExtension;
}


/**
 * Calculated the guest physical address of the large (4 MB) page in 32 bits paging mode.
 * Takes PSE-36 into account.
 *
 * @returns guest physical address
 * @param   pVM         The cross context VM structure.
 * @param   Pde         Guest Pde
 */
DECLINLINE(RTGCPHYS) pgmGstGet4MBPhysPage(PVM pVM, X86PDE Pde)
{
    RTGCPHYS GCPhys = Pde.u & X86_PDE4M_PG_MASK;
    GCPhys |= (RTGCPHYS)Pde.b.u8PageNoHigh << 32;

    return GCPhys & pVM->pgm.s.GCPhys4MBPSEMask;
}


/**
 * Gets the address the guest page directory (32-bit paging).
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   ppPd        Where to return the mapping. This is always set.
 */
DECLINLINE(int) pgmGstGet32bitPDPtrEx(PVMCPU pVCpu, PX86PD *ppPd)
{
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
    int rc = pgmRZDynMapGCPageInlined(pVCpu, pVCpu->pgm.s.GCPhysCR3, (void **)ppPd RTLOG_COMMA_SRC_POS);
    if (RT_FAILURE(rc))
    {
        *ppPd = NULL;
        return rc;
    }
#else
    *ppPd = pVCpu->pgm.s.CTX_SUFF(pGst32BitPd);
    if (RT_UNLIKELY(!*ppPd))
        return pgmGstLazyMap32BitPD(pVCpu, ppPd);
#endif
    return VINF_SUCCESS;
}


/**
 * Gets the address the guest page directory (32-bit paging).
 *
 * @returns Pointer to the page directory entry in question.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(PX86PD) pgmGstGet32bitPDPtr(PVMCPU pVCpu)
{
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
    PX86PD pGuestPD = NULL;
    int rc = pgmRZDynMapGCPageInlined(pVCpu, pVCpu->pgm.s.GCPhysCR3, (void **)&pGuestPD RTLOG_COMMA_SRC_POS);
    if (RT_FAILURE(rc))
    {
        AssertMsg(rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc));
        return NULL;
    }
#else
    PX86PD pGuestPD = pVCpu->pgm.s.CTX_SUFF(pGst32BitPd);
    if (RT_UNLIKELY(!pGuestPD))
    {
        int rc = pgmGstLazyMap32BitPD(pVCpu, &pGuestPD);
        if (RT_FAILURE(rc))
            return NULL;
    }
#endif
    return pGuestPD;
}


/**
 * Gets the guest page directory pointer table.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   ppPdpt      Where to return the mapping.  This is always set.
 */
DECLINLINE(int) pgmGstGetPaePDPTPtrEx(PVMCPU pVCpu, PX86PDPT *ppPdpt)
{
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
    int rc = pgmRZDynMapGCPageOffInlined(pVCpu, pVCpu->pgm.s.GCPhysCR3, (void **)ppPdpt RTLOG_COMMA_SRC_POS);
    if (RT_FAILURE(rc))
    {
        *ppPdpt = NULL;
        return rc;
    }
#else
    *ppPdpt = pVCpu->pgm.s.CTX_SUFF(pGstPaePdpt);
    if (RT_UNLIKELY(!*ppPdpt))
        return pgmGstLazyMapPaePDPT(pVCpu, ppPdpt);
#endif
    return VINF_SUCCESS;
}


/**
 * Gets the guest page directory pointer table.
 *
 * @returns Pointer to the page directory in question.
 * @returns NULL if the page directory is not present or on an invalid page.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(PX86PDPT) pgmGstGetPaePDPTPtr(PVMCPU pVCpu)
{
    PX86PDPT pGuestPdpt;
    int rc = pgmGstGetPaePDPTPtrEx(pVCpu, &pGuestPdpt);
    AssertMsg(RT_SUCCESS(rc) || rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc)); NOREF(rc);
    return pGuestPdpt;
}


/**
 * Gets the guest page directory pointer table entry for the specified address.
 *
 * @returns Pointer to the page directory in question.
 * @returns NULL if the page directory is not present or on an invalid page.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 */
DECLINLINE(PX86PDPE) pgmGstGetPaePDPEPtr(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    AssertGCPtr32(GCPtr);

#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
    PX86PDPT pGuestPDPT = NULL;
    int rc = pgmRZDynMapGCPageOffInlined(pVCpu, pVCpu->pgm.s.GCPhysCR3, (void **)&pGuestPDPT RTLOG_COMMA_SRC_POS);
    AssertRCReturn(rc, NULL);
#else
    PX86PDPT pGuestPDPT = pVCpu->pgm.s.CTX_SUFF(pGstPaePdpt);
    if (RT_UNLIKELY(!pGuestPDPT))
    {
        int rc = pgmGstLazyMapPaePDPT(pVCpu, &pGuestPDPT);
        if (RT_FAILURE(rc))
            return NULL;
    }
#endif
    return &pGuestPDPT->a[(uint32_t)GCPtr >> X86_PDPT_SHIFT];
}


/**
 * Gets the page directory entry for the specified address.
 *
 * @returns The page directory entry in question.
 * @returns A non-present entry if the page directory is not present or on an invalid page.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   GCPtr       The address.
 */
DECLINLINE(X86PDEPAE) pgmGstGetPaePDE(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    AssertGCPtr32(GCPtr);
    PX86PDPT    pGuestPDPT = pgmGstGetPaePDPTPtr(pVCpu);
    if (RT_LIKELY(pGuestPDPT))
    {
        const unsigned iPdpt = (uint32_t)GCPtr >> X86_PDPT_SHIFT;
        if (    pGuestPDPT->a[iPdpt].n.u1Present
            &&  !(pGuestPDPT->a[iPdpt].u & pVCpu->pgm.s.fGstPaeMbzPdpeMask) )
        {
            const unsigned iPD = (GCPtr >> X86_PD_PAE_SHIFT) & X86_PD_PAE_MASK;
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
            PX86PDPAE   pGuestPD = NULL;
            int rc = pgmRZDynMapGCPageInlined(pVCpu,
                                              pGuestPDPT->a[iPdpt].u & X86_PDPE_PG_MASK,
                                              (void **)&pGuestPD
                                              RTLOG_COMMA_SRC_POS);
            if (RT_SUCCESS(rc))
                return pGuestPD->a[iPD];
            AssertMsg(rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc));
#else
            PX86PDPAE   pGuestPD = pVCpu->pgm.s.CTX_SUFF(apGstPaePDs)[iPdpt];
            if (    !pGuestPD
                ||  (pGuestPDPT->a[iPdpt].u & X86_PDPE_PG_MASK) != pVCpu->pgm.s.aGCPhysGstPaePDs[iPdpt])
                pgmGstLazyMapPaePD(pVCpu, iPdpt, &pGuestPD);
            if (pGuestPD)
                return pGuestPD->a[iPD];
#endif
        }
    }

    X86PDEPAE ZeroPde = {0};
    return ZeroPde;
}


/**
 * Gets the page directory pointer table entry for the specified address
 * and returns the index into the page directory
 *
 * @returns Pointer to the page directory in question.
 * @returns NULL if the page directory is not present or on an invalid page.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 * @param   piPD        Receives the index into the returned page directory
 * @param   pPdpe       Receives the page directory pointer entry. Optional.
 */
DECLINLINE(PX86PDPAE) pgmGstGetPaePDPtr(PVMCPU pVCpu, RTGCPTR GCPtr, unsigned *piPD, PX86PDPE pPdpe)
{
    AssertGCPtr32(GCPtr);

    /* The PDPE. */
    PX86PDPT        pGuestPDPT = pgmGstGetPaePDPTPtr(pVCpu);
    if (RT_UNLIKELY(!pGuestPDPT))
        return NULL;
    const unsigned  iPdpt = (uint32_t)GCPtr >> X86_PDPT_SHIFT;
    if (pPdpe)
        *pPdpe = pGuestPDPT->a[iPdpt];
    if (!pGuestPDPT->a[iPdpt].n.u1Present)
        return NULL;
    if (RT_UNLIKELY(pVCpu->pgm.s.fGstPaeMbzPdpeMask & pGuestPDPT->a[iPdpt].u))
        return NULL;

    /* The PDE. */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
    PX86PDPAE   pGuestPD = NULL;
    int rc = pgmRZDynMapGCPageInlined(pVCpu,
                                      pGuestPDPT->a[iPdpt].u & X86_PDPE_PG_MASK,
                                      (void **)&pGuestPD
                                      RTLOG_COMMA_SRC_POS);
    if (RT_FAILURE(rc))
    {
        AssertMsg(rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc));
        return NULL;
    }
#else
    PX86PDPAE   pGuestPD = pVCpu->pgm.s.CTX_SUFF(apGstPaePDs)[iPdpt];
    if (    !pGuestPD
        ||  (pGuestPDPT->a[iPdpt].u & X86_PDPE_PG_MASK) != pVCpu->pgm.s.aGCPhysGstPaePDs[iPdpt])
        pgmGstLazyMapPaePD(pVCpu, iPdpt, &pGuestPD);
#endif

    *piPD = (GCPtr >> X86_PD_PAE_SHIFT) & X86_PD_PAE_MASK;
    return pGuestPD;
}

#ifndef IN_RC

/**
 * Gets the page map level-4 pointer for the guest.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   ppPml4      Where to return the mapping.  Always set.
 */
DECLINLINE(int) pgmGstGetLongModePML4PtrEx(PVMCPU pVCpu, PX86PML4 *ppPml4)
{
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
    int rc = pgmRZDynMapGCPageInlined(pVCpu, pVCpu->pgm.s.GCPhysCR3, (void **)ppPml4 RTLOG_COMMA_SRC_POS);
    if (RT_FAILURE(rc))
    {
        *ppPml4 = NULL;
        return rc;
    }
#else
    *ppPml4 = pVCpu->pgm.s.CTX_SUFF(pGstAmd64Pml4);
    if (RT_UNLIKELY(!*ppPml4))
        return pgmGstLazyMapPml4(pVCpu, ppPml4);
#endif
    return VINF_SUCCESS;
}


/**
 * Gets the page map level-4 pointer for the guest.
 *
 * @returns Pointer to the PML4 page.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(PX86PML4) pgmGstGetLongModePML4Ptr(PVMCPU pVCpu)
{
    PX86PML4 pGuestPml4;
    int rc = pgmGstGetLongModePML4PtrEx(pVCpu, &pGuestPml4);
    AssertMsg(RT_SUCCESS(rc) || rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc)); NOREF(rc);
    return pGuestPml4;
}


/**
 * Gets the pointer to a page map level-4 entry.
 *
 * @returns Pointer to the PML4 entry.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   iPml4       The index.
 * @remarks Only used by AssertCR3.
 */
DECLINLINE(PX86PML4E) pgmGstGetLongModePML4EPtr(PVMCPU pVCpu, unsigned int iPml4)
{
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
    PX86PML4 pGuestPml4;
    int rc = pgmRZDynMapGCPageInlined(pVCpu, pVCpu->pgm.s.GCPhysCR3, (void **)&pGuestPml4 RTLOG_COMMA_SRC_POS);
    AssertRCReturn(rc, NULL);
#else
    PX86PML4 pGuestPml4 = pVCpu->pgm.s.CTX_SUFF(pGstAmd64Pml4);
    if (RT_UNLIKELY(!pGuestPml4))
    {
         int rc = pgmGstLazyMapPml4(pVCpu, &pGuestPml4);
         AssertRCReturn(rc, NULL);
    }
#endif
    return &pGuestPml4->a[iPml4];
}


/**
 * Gets the page directory entry for the specified address.
 *
 * @returns The page directory entry in question.
 * @returns A non-present entry if the page directory is not present or on an invalid page.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 */
DECLINLINE(X86PDEPAE) pgmGstGetLongModePDE(PVMCPU pVCpu, RTGCPTR64 GCPtr)
{
    /*
     * Note! To keep things simple, ASSUME invalid physical addresses will
     *       cause X86_TRAP_PF_RSVD.  This isn't a problem until we start
     *       supporting 52-bit wide physical guest addresses.
     */
    PCX86PML4       pGuestPml4 = pgmGstGetLongModePML4Ptr(pVCpu);
    const unsigned  iPml4      = (GCPtr >> X86_PML4_SHIFT) & X86_PML4_MASK;
    if (    RT_LIKELY(pGuestPml4)
        &&  pGuestPml4->a[iPml4].n.u1Present
        &&  !(pGuestPml4->a[iPml4].u & pVCpu->pgm.s.fGstAmd64MbzPml4eMask) )
    {
        PCX86PDPT   pPdptTemp;
        int rc = PGM_GCPHYS_2_PTR_BY_VMCPU(pVCpu, pGuestPml4->a[iPml4].u & X86_PML4E_PG_MASK, &pPdptTemp);
        if (RT_SUCCESS(rc))
        {
            const unsigned iPdpt = (GCPtr >> X86_PDPT_SHIFT) & X86_PDPT_MASK_AMD64;
            if (    pPdptTemp->a[iPdpt].n.u1Present
                &&  !(pPdptTemp->a[iPdpt].u & pVCpu->pgm.s.fGstAmd64MbzPdpeMask) )
            {
                PCX86PDPAE pPD;
                rc = PGM_GCPHYS_2_PTR_BY_VMCPU(pVCpu, pPdptTemp->a[iPdpt].u & X86_PDPE_PG_MASK, &pPD);
                if (RT_SUCCESS(rc))
                {
                    const unsigned iPD = (GCPtr >> X86_PD_PAE_SHIFT) & X86_PD_PAE_MASK;
                    return pPD->a[iPD];
                }
            }
        }
        AssertMsg(RT_SUCCESS(rc) || rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc));
    }

    X86PDEPAE ZeroPde = {0};
    return ZeroPde;
}


/**
 * Gets the GUEST page directory pointer for the specified address.
 *
 * @returns The page directory in question.
 * @returns NULL if the page directory is not present or on an invalid page.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 * @param   ppPml4e     Page Map Level-4 Entry (out)
 * @param   pPdpe       Page directory pointer table entry (out)
 * @param   piPD        Receives the index into the returned page directory
 */
DECLINLINE(PX86PDPAE) pgmGstGetLongModePDPtr(PVMCPU pVCpu, RTGCPTR64 GCPtr, PX86PML4E *ppPml4e, PX86PDPE pPdpe, unsigned *piPD)
{
    /* The PMLE4. */
    PX86PML4        pGuestPml4 = pgmGstGetLongModePML4Ptr(pVCpu);
    if (RT_UNLIKELY(!pGuestPml4))
        return NULL;
    const unsigned  iPml4      = (GCPtr >> X86_PML4_SHIFT) & X86_PML4_MASK;
    PCX86PML4E      pPml4e     = *ppPml4e = &pGuestPml4->a[iPml4];
    if (!pPml4e->n.u1Present)
        return NULL;
    if (RT_UNLIKELY(pPml4e->u & pVCpu->pgm.s.fGstAmd64MbzPml4eMask))
        return NULL;

    /* The PDPE. */
    PCX86PDPT       pPdptTemp;
    int rc = PGM_GCPHYS_2_PTR_BY_VMCPU(pVCpu, pPml4e->u & X86_PML4E_PG_MASK, &pPdptTemp);
    if (RT_FAILURE(rc))
    {
        AssertMsg(rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc));
        return NULL;
    }
    const unsigned iPdpt = (GCPtr >> X86_PDPT_SHIFT) & X86_PDPT_MASK_AMD64;
    *pPdpe = pPdptTemp->a[iPdpt];
    if (!pPdpe->n.u1Present)
        return NULL;
    if (RT_UNLIKELY(pPdpe->u & pVCpu->pgm.s.fGstAmd64MbzPdpeMask))
        return NULL;

    /* The PDE. */
    PX86PDPAE pPD;
    rc = PGM_GCPHYS_2_PTR_BY_VMCPU(pVCpu, pPdptTemp->a[iPdpt].u & X86_PDPE_PG_MASK, &pPD);
    if (RT_FAILURE(rc))
    {
        AssertMsg(rc == VERR_PGM_INVALID_GC_PHYSICAL_ADDRESS, ("%Rrc\n", rc));
        return NULL;
    }

    *piPD = (GCPtr >> X86_PD_PAE_SHIFT) & X86_PD_PAE_MASK;
    return pPD;
}

#endif /* !IN_RC */

/**
 * Gets the shadow page directory, 32-bit.
 *
 * @returns Pointer to the shadow 32-bit PD.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(PX86PD) pgmShwGet32BitPDPtr(PVMCPU pVCpu)
{
    return (PX86PD)PGMPOOL_PAGE_2_PTR_V2(pVCpu->CTX_SUFF(pVM), pVCpu, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
}


/**
 * Gets the shadow page directory entry for the specified address, 32-bit.
 *
 * @returns Shadow 32-bit PDE.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 */
DECLINLINE(X86PDE) pgmShwGet32BitPDE(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    PX86PD pShwPde = pgmShwGet32BitPDPtr(pVCpu);
    if (!pShwPde)
    {
        X86PDE ZeroPde = {0};
        return ZeroPde;
    }
    return pShwPde->a[(uint32_t)GCPtr >> X86_PD_SHIFT];
}


/**
 * Gets the pointer to the shadow page directory entry for the specified
 * address, 32-bit.
 *
 * @returns Pointer to the shadow 32-bit PDE.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 */
DECLINLINE(PX86PDE) pgmShwGet32BitPDEPtr(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    PX86PD pPde = pgmShwGet32BitPDPtr(pVCpu);
    AssertReturn(pPde, NULL);
    return &pPde->a[(uint32_t)GCPtr >> X86_PD_SHIFT];
}


/**
 * Gets the shadow page pointer table, PAE.
 *
 * @returns Pointer to the shadow PAE PDPT.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(PX86PDPT) pgmShwGetPaePDPTPtr(PVMCPU pVCpu)
{
    return (PX86PDPT)PGMPOOL_PAGE_2_PTR_V2(pVCpu->CTX_SUFF(pVM), pVCpu, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
}


/**
 * Gets the shadow page directory for the specified address, PAE.
 *
 * @returns Pointer to the shadow PD.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 */
DECLINLINE(PX86PDPAE) pgmShwGetPaePDPtr(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    const unsigned  iPdpt = (uint32_t)GCPtr >> X86_PDPT_SHIFT;
    PX86PDPT        pPdpt = pgmShwGetPaePDPTPtr(pVCpu);

    if (!pPdpt->a[iPdpt].n.u1Present)
        return NULL;

    /* Fetch the pgm pool shadow descriptor. */
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    PPGMPOOLPAGE pShwPde = pgmPoolGetPage(pVM->pgm.s.CTX_SUFF(pPool), pPdpt->a[iPdpt].u & X86_PDPE_PG_MASK);
    AssertReturn(pShwPde, NULL);

    return (PX86PDPAE)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPde);
}


/**
 * Gets the shadow page directory for the specified address, PAE.
 *
 * @returns Pointer to the shadow PD.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pPdpt       Pointer to the page directory pointer table.
 * @param   GCPtr       The address.
 */
DECLINLINE(PX86PDPAE) pgmShwGetPaePDPtr(PVMCPU pVCpu, PX86PDPT pPdpt, RTGCPTR GCPtr)
{
    const unsigned  iPdpt = (uint32_t)GCPtr >> X86_PDPT_SHIFT;

    if (!pPdpt->a[iPdpt].n.u1Present)
        return NULL;

    /* Fetch the pgm pool shadow descriptor. */
    PVM             pVM     = pVCpu->CTX_SUFF(pVM);
    PPGMPOOLPAGE    pShwPde = pgmPoolGetPage(pVM->pgm.s.CTX_SUFF(pPool), pPdpt->a[iPdpt].u & X86_PDPE_PG_MASK);
    AssertReturn(pShwPde, NULL);

    return (PX86PDPAE)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPde);
}


/**
 * Gets the shadow page directory entry, PAE.
 *
 * @returns PDE.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 */
DECLINLINE(X86PDEPAE) pgmShwGetPaePDE(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    const unsigned iPd = (GCPtr >> X86_PD_PAE_SHIFT) & X86_PD_PAE_MASK;

    PX86PDPAE pShwPde = pgmShwGetPaePDPtr(pVCpu, GCPtr);
    if (!pShwPde)
    {
        X86PDEPAE ZeroPde = {0};
        return ZeroPde;
    }
    return pShwPde->a[iPd];
}


/**
 * Gets the pointer to the shadow page directory entry for an address, PAE.
 *
 * @returns Pointer to the PDE.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 * @remarks Only used by AssertCR3.
 */
DECLINLINE(PX86PDEPAE) pgmShwGetPaePDEPtr(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    const unsigned iPd = (GCPtr >> X86_PD_PAE_SHIFT) & X86_PD_PAE_MASK;

    PX86PDPAE pPde = pgmShwGetPaePDPtr(pVCpu, GCPtr);
    AssertReturn(pPde, NULL);
    return &pPde->a[iPd];
}

#ifndef IN_RC

/**
 * Gets the shadow page map level-4 pointer.
 *
 * @returns Pointer to the shadow PML4.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(PX86PML4) pgmShwGetLongModePML4Ptr(PVMCPU pVCpu)
{
    return (PX86PML4)PGMPOOL_PAGE_2_PTR_V2(pVCpu->CTX_SUFF(pVM), pVCpu, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
}


/**
 * Gets the shadow page map level-4 entry for the specified address.
 *
 * @returns The entry.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCPtr       The address.
 */
DECLINLINE(X86PML4E) pgmShwGetLongModePML4E(PVMCPU pVCpu, RTGCPTR GCPtr)
{
    const unsigned  iPml4 = ((RTGCUINTPTR64)GCPtr >> X86_PML4_SHIFT) & X86_PML4_MASK;
    PX86PML4        pShwPml4 = pgmShwGetLongModePML4Ptr(pVCpu);

    if (!pShwPml4)
    {
        X86PML4E ZeroPml4e = {0};
        return ZeroPml4e;
    }
    return pShwPml4->a[iPml4];
}


/**
 * Gets the pointer to the specified shadow page map level-4 entry.
 *
 * @returns The entry.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   iPml4       The PML4 index.
 */
DECLINLINE(PX86PML4E) pgmShwGetLongModePML4EPtr(PVMCPU pVCpu, unsigned int iPml4)
{
    PX86PML4 pShwPml4 = pgmShwGetLongModePML4Ptr(pVCpu);
    if (!pShwPml4)
        return NULL;
    return &pShwPml4->a[iPml4];
}

#endif /* !IN_RC */

/**
 * Cached physical handler lookup.
 *
 * @returns Physical handler covering @a GCPhys.
 * @param   pVM                 The cross context VM structure.
 * @param   GCPhys              The lookup address.
 */
DECLINLINE(PPGMPHYSHANDLER) pgmHandlerPhysicalLookup(PVM pVM, RTGCPHYS GCPhys)
{
    PPGMPHYSHANDLER pHandler = pVM->pgm.s.CTX_SUFF(pLastPhysHandler);
    if (   pHandler
        && GCPhys >= pHandler->Core.Key
        && GCPhys < pHandler->Core.KeyLast)
    {
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PhysHandlerLookupHits));
        return pHandler;
    }

    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->CTX_MID_Z(Stat,PhysHandlerLookupMisses));
    pHandler = (PPGMPHYSHANDLER)RTAvlroGCPhysRangeGet(&pVM->pgm.s.CTX_SUFF(pTrees)->PhysHandlers, GCPhys);
    if (pHandler)
        pVM->pgm.s.CTX_SUFF(pLastPhysHandler) = pHandler;
    return pHandler;
}


#ifdef VBOX_WITH_RAW_MODE
/**
 * Clears one physical page of a virtual handler.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pCur        Virtual handler structure.
 * @param   iPage       Physical page index.
 *
 * @remark  Only used when PGM_SYNC_UPDATE_PAGE_BIT_VIRTUAL is being set, so no
 *          need to care about other handlers in the same page.
 */
DECLINLINE(void) pgmHandlerVirtualClearPage(PVM pVM, PPGMVIRTHANDLER pCur, unsigned iPage)
{
    const PPGMPHYS2VIRTHANDLER pPhys2Virt = &pCur->aPhysToVirt[iPage];

    /*
     * Remove the node from the tree (it's supposed to be in the tree if we get here!).
     */
# ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
    AssertReleaseMsg(pPhys2Virt->offNextAlias & PGMPHYS2VIRTHANDLER_IN_TREE,
                     ("pPhys2Virt=%p:{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32}\n",
                      pPhys2Virt, pPhys2Virt->Core.Key, pPhys2Virt->Core.KeyLast, pPhys2Virt->offVirtHandler, pPhys2Virt->offNextAlias));
# endif
    if (pPhys2Virt->offNextAlias & PGMPHYS2VIRTHANDLER_IS_HEAD)
    {
        /* We're the head of the alias chain. */
        PPGMPHYS2VIRTHANDLER pRemove = (PPGMPHYS2VIRTHANDLER)RTAvlroGCPhysRemove(&pVM->pgm.s.CTX_SUFF(pTrees)->PhysToVirtHandlers, pPhys2Virt->Core.Key); NOREF(pRemove);
# ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
        AssertReleaseMsg(pRemove != NULL,
                         ("pPhys2Virt=%p:{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32}\n",
                          pPhys2Virt, pPhys2Virt->Core.Key, pPhys2Virt->Core.KeyLast, pPhys2Virt->offVirtHandler, pPhys2Virt->offNextAlias));
        AssertReleaseMsg(pRemove == pPhys2Virt,
                         ("wanted: pPhys2Virt=%p:{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32}\n"
                          "   got:    pRemove=%p:{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32}\n",
                          pPhys2Virt, pPhys2Virt->Core.Key, pPhys2Virt->Core.KeyLast, pPhys2Virt->offVirtHandler, pPhys2Virt->offNextAlias,
                          pRemove, pRemove->Core.Key, pRemove->Core.KeyLast, pRemove->offVirtHandler, pRemove->offNextAlias));
# endif
        if (pPhys2Virt->offNextAlias & PGMPHYS2VIRTHANDLER_OFF_MASK)
        {
            /* Insert the next list in the alias chain into the tree. */
            PPGMPHYS2VIRTHANDLER pNext = (PPGMPHYS2VIRTHANDLER)((intptr_t)pPhys2Virt + (pPhys2Virt->offNextAlias & PGMPHYS2VIRTHANDLER_OFF_MASK));
# ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
            AssertReleaseMsg(pNext->offNextAlias & PGMPHYS2VIRTHANDLER_IN_TREE,
                             ("pNext=%p:{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32}\n",
                             pNext, pNext->Core.Key, pNext->Core.KeyLast, pNext->offVirtHandler, pNext->offNextAlias));
# endif
            pNext->offNextAlias |= PGMPHYS2VIRTHANDLER_IS_HEAD;
            bool fRc = RTAvlroGCPhysInsert(&pVM->pgm.s.CTX_SUFF(pTrees)->PhysToVirtHandlers, &pNext->Core);
            AssertRelease(fRc);
        }
    }
    else
    {
        /* Locate the previous node in the alias chain. */
        PPGMPHYS2VIRTHANDLER pPrev = (PPGMPHYS2VIRTHANDLER)RTAvlroGCPhysGet(&pVM->pgm.s.CTX_SUFF(pTrees)->PhysToVirtHandlers, pPhys2Virt->Core.Key);
# ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
        AssertReleaseMsg(pPrev != pPhys2Virt,
                         ("pPhys2Virt=%p:{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32} pPrev=%p\n",
                          pPhys2Virt, pPhys2Virt->Core.Key, pPhys2Virt->Core.KeyLast, pPhys2Virt->offVirtHandler, pPhys2Virt->offNextAlias, pPrev));
# endif
        for (;;)
        {
            PPGMPHYS2VIRTHANDLER pNext = (PPGMPHYS2VIRTHANDLER)((intptr_t)pPrev + (pPrev->offNextAlias & PGMPHYS2VIRTHANDLER_OFF_MASK));
            if (pNext == pPhys2Virt)
            {
                /* unlink. */
                LogFlow(("pgmHandlerVirtualClearPage: removed %p:{.offNextAlias=%#RX32} from alias chain. prev %p:{.offNextAlias=%#RX32} [%RGp-%RGp]\n",
                         pPhys2Virt, pPhys2Virt->offNextAlias, pPrev, pPrev->offNextAlias, pPhys2Virt->Core.Key, pPhys2Virt->Core.KeyLast));
                if (!(pPhys2Virt->offNextAlias & PGMPHYS2VIRTHANDLER_OFF_MASK))
                    pPrev->offNextAlias &= ~PGMPHYS2VIRTHANDLER_OFF_MASK;
                else
                {
                    PPGMPHYS2VIRTHANDLER pNewNext = (PPGMPHYS2VIRTHANDLER)((intptr_t)pPhys2Virt + (pPhys2Virt->offNextAlias & PGMPHYS2VIRTHANDLER_OFF_MASK));
                    pPrev->offNextAlias = ((intptr_t)pNewNext - (intptr_t)pPrev)
                                        | (pPrev->offNextAlias & ~PGMPHYS2VIRTHANDLER_OFF_MASK);
                }
                break;
            }

            /* next */
            if (pNext == pPrev)
            {
# ifdef VBOX_STRICT_PGM_HANDLER_VIRTUAL
                AssertReleaseMsg(pNext != pPrev,
                                 ("pPhys2Virt=%p:{.Core.Key=%RGp, .Core.KeyLast=%RGp, .offVirtHandler=%#RX32, .offNextAlias=%#RX32} pPrev=%p\n",
                                  pPhys2Virt, pPhys2Virt->Core.Key, pPhys2Virt->Core.KeyLast, pPhys2Virt->offVirtHandler, pPhys2Virt->offNextAlias, pPrev));
# endif
                break;
            }
            pPrev = pNext;
        }
    }
    Log2(("PHYS2VIRT: Removing %RGp-%RGp %#RX32 %s\n",
          pPhys2Virt->Core.Key, pPhys2Virt->Core.KeyLast, pPhys2Virt->offNextAlias, R3STRING(pCur->pszDesc)));
    pPhys2Virt->offNextAlias = 0;
    pPhys2Virt->Core.KeyLast = NIL_RTGCPHYS; /* require reinsert */

    /*
     * Clear the ram flags for this page.
     */
    PPGMPAGE pPage = pgmPhysGetPage(pVM, pPhys2Virt->Core.Key);
    AssertReturnVoid(pPage);
    PGM_PAGE_SET_HNDL_VIRT_STATE(pPage, PGM_PAGE_HNDL_VIRT_STATE_NONE);
}
#endif /* VBOX_WITH_RAW_MODE */


/**
 * Internal worker for finding a 'in-use' shadow page give by it's physical address.
 *
 * @returns Pointer to the shadow page structure.
 * @param   pPool       The pool.
 * @param   idx         The pool page index.
 */
DECLINLINE(PPGMPOOLPAGE) pgmPoolGetPageByIdx(PPGMPOOL pPool, unsigned idx)
{
    AssertFatalMsg(idx >= PGMPOOL_IDX_FIRST && idx < pPool->cCurPages, ("idx=%d\n", idx));
    return &pPool->aPages[idx];
}


/**
 * Clear references to guest physical memory.
 *
 * @param   pPool       The pool.
 * @param   pPoolPage   The pool page.
 * @param   pPhysPage   The physical guest page tracking structure.
 * @param   iPte        Shadow PTE index
 */
DECLINLINE(void) pgmTrackDerefGCPhys(PPGMPOOL pPool, PPGMPOOLPAGE pPoolPage, PPGMPAGE pPhysPage, uint16_t iPte)
{
    /*
     * Just deal with the simple case here.
     */
# ifdef VBOX_STRICT
    PVM pVM = pPool->CTX_SUFF(pVM); NOREF(pVM);
# endif
# ifdef LOG_ENABLED
    const unsigned uOrg = PGM_PAGE_GET_TRACKING(pPhysPage);
# endif
    const unsigned cRefs = PGM_PAGE_GET_TD_CREFS(pPhysPage);
    if (cRefs == 1)
    {
        Assert(pPoolPage->idx == PGM_PAGE_GET_TD_IDX(pPhysPage));
        Assert(iPte == PGM_PAGE_GET_PTE_INDEX(pPhysPage));
        /* Invalidate the tracking data. */
        PGM_PAGE_SET_TRACKING(pVM, pPhysPage, 0);
    }
    else
        pgmPoolTrackPhysExtDerefGCPhys(pPool, pPoolPage, pPhysPage, iPte);
    Log2(("pgmTrackDerefGCPhys: %x -> %x pPhysPage=%R[pgmpage]\n", uOrg, PGM_PAGE_GET_TRACKING(pPhysPage), pPhysPage ));
}


/**
 * Moves the page to the head of the age list.
 *
 * This is done when the cached page is used in one way or another.
 *
 * @param   pPool       The pool.
 * @param   pPage       The cached page.
 */
DECLINLINE(void) pgmPoolCacheUsed(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    PGM_LOCK_ASSERT_OWNER(pPool->CTX_SUFF(pVM));

    /*
     * Move to the head of the age list.
     */
    if (pPage->iAgePrev != NIL_PGMPOOL_IDX)
    {
        /* unlink */
        pPool->aPages[pPage->iAgePrev].iAgeNext = pPage->iAgeNext;
        if (pPage->iAgeNext != NIL_PGMPOOL_IDX)
            pPool->aPages[pPage->iAgeNext].iAgePrev = pPage->iAgePrev;
        else
            pPool->iAgeTail = pPage->iAgePrev;

        /* insert at head */
        pPage->iAgePrev = NIL_PGMPOOL_IDX;
        pPage->iAgeNext = pPool->iAgeHead;
        Assert(pPage->iAgeNext != NIL_PGMPOOL_IDX); /* we would've already been head then */
        pPool->iAgeHead = pPage->idx;
        pPool->aPages[pPage->iAgeNext].iAgePrev = pPage->idx;
    }
}


/**
 * Locks a page to prevent flushing (important for cr3 root pages or shadow pae pd pages).
 *
 * @param   pPool       The pool.
 * @param   pPage       PGM pool page
 */
DECLINLINE(void) pgmPoolLockPage(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    PGM_LOCK_ASSERT_OWNER(pPool->CTX_SUFF(pVM)); NOREF(pPool);
    ASMAtomicIncU32(&pPage->cLocked);
}


/**
 * Unlocks a page to allow flushing again
 *
 * @param   pPool       The pool.
 * @param   pPage       PGM pool page
 */
DECLINLINE(void) pgmPoolUnlockPage(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    PGM_LOCK_ASSERT_OWNER(pPool->CTX_SUFF(pVM)); NOREF(pPool);
    Assert(pPage->cLocked);
    ASMAtomicDecU32(&pPage->cLocked);
}


/**
 * Checks if the page is locked (e.g. the active CR3 or one of the four PDs of a PAE PDPT)
 *
 * @returns VBox status code.
 * @param   pPage       PGM pool page
 */
DECLINLINE(bool) pgmPoolIsPageLocked(PPGMPOOLPAGE pPage)
{
    if (pPage->cLocked)
    {
        LogFlow(("pgmPoolIsPageLocked found root page %d\n", pPage->enmKind));
        if (pPage->cModifications)
            pPage->cModifications = 1; /* reset counter (can't use 0, or else it will be reinserted in the modified list) */
        return true;
    }
    return false;
}


/**
 * Tells if mappings are to be put into the shadow page table or not.
 *
 * @returns boolean result
 * @param   pVM         The cross context VM structure.
 */
DECL_FORCE_INLINE(bool) pgmMapAreMappingsEnabled(PVM pVM)
{
#ifdef PGM_WITHOUT_MAPPINGS
    /* There are no mappings in VT-x and AMD-V mode. */
    Assert(HMIsEnabled(pVM)); NOREF(pVM);
    return false;
#else
    Assert(pVM->cCpus == 1 || HMIsEnabled(pVM));
    return !HMIsEnabled(pVM);
#endif
}


/**
 * Checks if the mappings are floating and enabled.
 *
 * @returns true / false.
 * @param   pVM         The cross context VM structure.
 */
DECL_FORCE_INLINE(bool) pgmMapAreMappingsFloating(PVM pVM)
{
#ifdef PGM_WITHOUT_MAPPINGS
    /* There are no mappings in VT-x and AMD-V mode. */
    Assert(HMIsEnabled(pVM)); NOREF(pVM);
    return false;
#else
    return !pVM->pgm.s.fMappingsFixed
        && pgmMapAreMappingsEnabled(pVM);
#endif
}

/** @} */

#endif


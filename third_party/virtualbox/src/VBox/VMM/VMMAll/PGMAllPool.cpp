/* $Id: PGMAllPool.cpp $ */
/** @file
 * PGM Shadow Page Pool.
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
#define LOG_GROUP LOG_GROUP_PGM_POOL
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/cpum.h>
#ifdef IN_RC
# include <VBox/vmm/patm.h>
#endif
#include "PGMInternal.h"
#include <VBox/vmm/vm.h>
#include "PGMInline.h"
#include <VBox/disopcode.h>
#include <VBox/vmm/hm_vmx.h>

#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
#if 0 /* unused */
DECLINLINE(unsigned) pgmPoolTrackGetShadowEntrySize(PGMPOOLKIND enmKind);
DECLINLINE(unsigned) pgmPoolTrackGetGuestEntrySize(PGMPOOLKIND enmKind);
#endif /* unused */
static void pgmPoolTrackClearPageUsers(PPGMPOOL pPool, PPGMPOOLPAGE pPage);
static void pgmPoolTrackDeref(PPGMPOOL pPool, PPGMPOOLPAGE pPage);
static int pgmPoolTrackAddUser(PPGMPOOL pPool, PPGMPOOLPAGE pPage, uint16_t iUser, uint32_t iUserTable);
static void pgmPoolMonitorModifiedRemove(PPGMPOOL pPool, PPGMPOOLPAGE pPage);
#if defined(LOG_ENABLED) || defined(VBOX_STRICT)
static const char *pgmPoolPoolKindToStr(uint8_t enmKind);
#endif
#if 0 /*defined(VBOX_STRICT) && defined(PGMPOOL_WITH_OPTIMIZED_DIRTY_PT)*/
static void pgmPoolTrackCheckPTPaePae(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT, PCX86PTPAE pGstPT);
#endif

int             pgmPoolTrackFlushGCPhysPTsSlow(PVM pVM, PPGMPAGE pPhysPage);
PPGMPOOLPHYSEXT pgmPoolTrackPhysExtAlloc(PVM pVM, uint16_t *piPhysExt);
void            pgmPoolTrackPhysExtFree(PVM pVM, uint16_t iPhysExt);
void            pgmPoolTrackPhysExtFreeList(PVM pVM, uint16_t iPhysExt);

RT_C_DECLS_END


#if 0 /* unused */
/**
 * Checks if the specified page pool kind is for a 4MB or 2MB guest page.
 *
 * @returns true if it's the shadow of a 4MB or 2MB guest page, otherwise false.
 * @param   enmKind     The page kind.
 */
DECLINLINE(bool) pgmPoolIsBigPage(PGMPOOLKIND enmKind)
{
    switch (enmKind)
    {
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
            return true;
        default:
            return false;
    }
}
#endif /* unused */


/**
 * Flushes a chain of pages sharing the same access monitor.
 *
 * @returns VBox status code suitable for scheduling.
 * @param   pPool       The pool.
 * @param   pPage       A page in the chain.
 * @todo VBOXSTRICTRC
 */
int pgmPoolMonitorChainFlush(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    LogFlow(("pgmPoolMonitorChainFlush: Flush page %RGp type=%d\n", pPage->GCPhys, pPage->enmKind));

    /*
     * Find the list head.
     */
    uint16_t idx = pPage->idx;
    if (pPage->iMonitoredPrev != NIL_PGMPOOL_IDX)
    {
        while (pPage->iMonitoredPrev != NIL_PGMPOOL_IDX)
        {
            idx = pPage->iMonitoredPrev;
            Assert(idx != pPage->idx);
            pPage = &pPool->aPages[idx];
        }
    }

    /*
     * Iterate the list flushing each shadow page.
     */
    int rc = VINF_SUCCESS;
    for (;;)
    {
        idx = pPage->iMonitoredNext;
        Assert(idx != pPage->idx);
        if (pPage->idx >= PGMPOOL_IDX_FIRST)
        {
            int rc2 = pgmPoolFlushPage(pPool, pPage);
            AssertRC(rc2);
        }
        /* next */
        if (idx == NIL_PGMPOOL_IDX)
            break;
        pPage = &pPool->aPages[idx];
    }
    return rc;
}


/**
 * Wrapper for getting the current context pointer to the entry being modified.
 *
 * @returns VBox status code suitable for scheduling.
 * @param   pVM         The cross context VM structure.
 * @param   pvDst       Destination address
 * @param   pvSrc       Pointer to the mapping of @a GCPhysSrc or NULL depending
 *                      on the context (e.g. \#PF in R0 & RC).
 * @param   GCPhysSrc   The source guest physical address.
 * @param   cb          Size of data to read
 */
DECLINLINE(int) pgmPoolPhysSimpleReadGCPhys(PVM pVM, void *pvDst, void const *pvSrc, RTGCPHYS GCPhysSrc, size_t cb)
{
#if defined(IN_RING3)
    NOREF(pVM); NOREF(GCPhysSrc);
    memcpy(pvDst, (RTHCPTR)((uintptr_t)pvSrc & ~(RTHCUINTPTR)(cb - 1)), cb);
    return VINF_SUCCESS;
#else
    /** @todo in RC we could attempt to use the virtual address, although this can cause many faults (PAE Windows XP guest). */
    NOREF(pvSrc);
    return PGMPhysSimpleReadGCPhys(pVM, pvDst, GCPhysSrc & ~(RTGCPHYS)(cb - 1), cb);
#endif
}


/**
 * Process shadow entries before they are changed by the guest.
 *
 * For PT entries we will clear them. For PD entries, we'll simply check
 * for mapping conflicts and set the SyncCR3 FF if found.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pPool       The pool.
 * @param   pPage       The head page.
 * @param   GCPhysFault The guest physical fault address.
 * @param   pvAddress   Pointer to the mapping of @a GCPhysFault or NULL
 *                      depending on the context (e.g. \#PF in R0 & RC).
 * @param   cbWrite     Write size; might be zero if the caller knows we're not crossing entry boundaries
 */
static void pgmPoolMonitorChainChanging(PVMCPU pVCpu, PPGMPOOL pPool, PPGMPOOLPAGE pPage, RTGCPHYS GCPhysFault,
                                        void const *pvAddress, unsigned cbWrite)
{
    AssertMsg(pPage->iMonitoredPrev == NIL_PGMPOOL_IDX, ("%u (idx=%u)\n", pPage->iMonitoredPrev, pPage->idx));
    const unsigned  off = GCPhysFault & PAGE_OFFSET_MASK;
    PVM             pVM = pPool->CTX_SUFF(pVM);
    NOREF(pVCpu);

    LogFlow(("pgmPoolMonitorChainChanging: %RGv phys=%RGp cbWrite=%d\n",
             (RTGCPTR)(CTXTYPE(RTGCPTR, uintptr_t, RTGCPTR))(uintptr_t)pvAddress, GCPhysFault, cbWrite));

    for (;;)
    {
       union
        {
            void           *pv;
            PX86PT          pPT;
            PPGMSHWPTPAE    pPTPae;
            PX86PD          pPD;
            PX86PDPAE       pPDPae;
            PX86PDPT        pPDPT;
            PX86PML4        pPML4;
        } uShw;

        LogFlow(("pgmPoolMonitorChainChanging: page idx=%d phys=%RGp (next=%d) kind=%s write=%#x\n",
                 pPage->idx, pPage->GCPhys, pPage->iMonitoredNext, pgmPoolPoolKindToStr(pPage->enmKind), cbWrite));

        uShw.pv = NULL;
        switch (pPage->enmKind)
        {
            case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
            {
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPT));
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = off / sizeof(X86PTE);
                LogFlow(("PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT iShw=%x\n", iShw));
                if (uShw.pPT->a[iShw].n.u1Present)
                {
                    X86PTE GstPte;

                    int rc = pgmPoolPhysSimpleReadGCPhys(pVM, &GstPte, pvAddress, GCPhysFault, sizeof(GstPte));
                    AssertRC(rc);
                    Log4(("pgmPoolMonitorChainChanging 32_32: deref %016RX64 GCPhys %08RX32\n", uShw.pPT->a[iShw].u & X86_PTE_PAE_PG_MASK, GstPte.u & X86_PTE_PG_MASK));
                    pgmPoolTracDerefGCPhysHint(pPool, pPage,
                                               uShw.pPT->a[iShw].u & X86_PTE_PAE_PG_MASK,
                                               GstPte.u & X86_PTE_PG_MASK,
                                               iShw);
                    ASMAtomicWriteU32(&uShw.pPT->a[iShw].u, 0);
                }
                break;
            }

            /* page/2 sized */
            case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
            {
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPT));
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                if (!((off ^ pPage->GCPhys) & (PAGE_SIZE / 2)))
                {
                    const unsigned iShw = (off / sizeof(X86PTE)) & (X86_PG_PAE_ENTRIES - 1);
                    LogFlow(("PGMPOOLKIND_PAE_PT_FOR_32BIT_PT iShw=%x\n", iShw));
                    if (PGMSHWPTEPAE_IS_P(uShw.pPTPae->a[iShw]))
                    {
                        X86PTE GstPte;
                        int rc = pgmPoolPhysSimpleReadGCPhys(pVM, &GstPte, pvAddress, GCPhysFault, sizeof(GstPte));
                        AssertRC(rc);

                        Log4(("pgmPoolMonitorChainChanging pae_32: deref %016RX64 GCPhys %08RX32\n", uShw.pPT->a[iShw].u & X86_PTE_PAE_PG_MASK, GstPte.u & X86_PTE_PG_MASK));
                        pgmPoolTracDerefGCPhysHint(pPool, pPage,
                                                   PGMSHWPTEPAE_GET_HCPHYS(uShw.pPTPae->a[iShw]),
                                                   GstPte.u & X86_PTE_PG_MASK,
                                                   iShw);
                        PGMSHWPTEPAE_ATOMIC_SET(uShw.pPTPae->a[iShw], 0);
                    }
                }
                break;
            }

            case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
            case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
            case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
            case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
            {
                unsigned iGst     = off / sizeof(X86PDE);
                unsigned iShwPdpt = iGst / 256;
                unsigned iShw     = (iGst % 256) * 2;
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);

                LogFlow(("pgmPoolMonitorChainChanging PAE for 32 bits: iGst=%x iShw=%x idx = %d page idx=%d\n", iGst, iShw, iShwPdpt, pPage->enmKind - PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD));
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPD));
                if (iShwPdpt == pPage->enmKind - (unsigned)PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD)
                {
                    for (unsigned i = 0; i < 2; i++)
                    {
#  ifdef VBOX_WITH_RAW_MODE_NOT_R0
                        if ((uShw.pPDPae->a[iShw + i].u & (PGM_PDFLAGS_MAPPING | X86_PDE_P)) == (PGM_PDFLAGS_MAPPING | X86_PDE_P))
                        {
                            Assert(pgmMapAreMappingsEnabled(pVM));
                            VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                            LogFlow(("pgmPoolMonitorChainChanging: Detected conflict at iShwPdpt=%#x iShw=%#x!\n", iShwPdpt, iShw+i));
                            break;
                        }
#  endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                        if (uShw.pPDPae->a[iShw+i].n.u1Present)
                        {
                            LogFlow(("pgmPoolMonitorChainChanging: pae pd iShw=%#x: %RX64 -> freeing it!\n", iShw+i, uShw.pPDPae->a[iShw+i].u));
                            pgmPoolFree(pVM,
                                        uShw.pPDPae->a[iShw+i].u & X86_PDE_PAE_PG_MASK,
                                        pPage->idx,
                                        iShw + i);
                            ASMAtomicWriteU64(&uShw.pPDPae->a[iShw+i].u, 0);
                        }

                        /* paranoia / a bit assumptive. */
                        if (    (off & 3)
                            &&  (off & 3) + cbWrite > 4)
                        {
                            const unsigned iShw2 = iShw + 2 + i;
                            if (iShw2 < RT_ELEMENTS(uShw.pPDPae->a))
                            {
#  ifdef VBOX_WITH_RAW_MODE_NOT_R0
                                if ((uShw.pPDPae->a[iShw2].u & (PGM_PDFLAGS_MAPPING | X86_PDE_P)) == (PGM_PDFLAGS_MAPPING | X86_PDE_P))
                                {
                                    Assert(pgmMapAreMappingsEnabled(pVM));
                                    VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                                    LogFlow(("pgmPoolMonitorChainChanging: Detected conflict at iShwPdpt=%#x iShw2=%#x!\n", iShwPdpt, iShw2));
                                    break;
                                }
#  endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                                if (uShw.pPDPae->a[iShw2].n.u1Present)
                                {
                                    LogFlow(("pgmPoolMonitorChainChanging: pae pd iShw=%#x: %RX64 -> freeing it!\n", iShw2, uShw.pPDPae->a[iShw2].u));
                                    pgmPoolFree(pVM,
                                                uShw.pPDPae->a[iShw2].u & X86_PDE_PAE_PG_MASK,
                                                pPage->idx,
                                                iShw2);
                                    ASMAtomicWriteU64(&uShw.pPDPae->a[iShw2].u, 0);
                                }
                            }
                        }
                    }
                }
                break;
            }

            case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
            {
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = off / sizeof(X86PTEPAE);
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPT));
                if (PGMSHWPTEPAE_IS_P(uShw.pPTPae->a[iShw]))
                {
                    X86PTEPAE GstPte;
                    int rc = pgmPoolPhysSimpleReadGCPhys(pVM, &GstPte, pvAddress, GCPhysFault, sizeof(GstPte));
                    AssertRC(rc);

                    Log4(("pgmPoolMonitorChainChanging pae: deref %016RX64 GCPhys %016RX64\n", PGMSHWPTEPAE_GET_HCPHYS(uShw.pPTPae->a[iShw]), GstPte.u & X86_PTE_PAE_PG_MASK));
                    pgmPoolTracDerefGCPhysHint(pPool, pPage,
                                               PGMSHWPTEPAE_GET_HCPHYS(uShw.pPTPae->a[iShw]),
                                               GstPte.u & X86_PTE_PAE_PG_MASK,
                                               iShw);
                    PGMSHWPTEPAE_ATOMIC_SET(uShw.pPTPae->a[iShw], 0);
                }

                /* paranoia / a bit assumptive. */
                if (    (off & 7)
                    &&  (off & 7) + cbWrite > sizeof(X86PTEPAE))
                {
                    const unsigned iShw2 = (off + cbWrite - 1) / sizeof(X86PTEPAE);
                    AssertBreak(iShw2 < RT_ELEMENTS(uShw.pPTPae->a));

                    if (PGMSHWPTEPAE_IS_P(uShw.pPTPae->a[iShw2]))
                    {
                        X86PTEPAE GstPte;
                        int rc = pgmPoolPhysSimpleReadGCPhys(pVM, &GstPte,
                                                             pvAddress ? (uint8_t const *)pvAddress + sizeof(GstPte) : NULL,
                                                             GCPhysFault + sizeof(GstPte), sizeof(GstPte));
                        AssertRC(rc);
                        Log4(("pgmPoolMonitorChainChanging pae: deref %016RX64 GCPhys %016RX64\n", PGMSHWPTEPAE_GET_HCPHYS(uShw.pPTPae->a[iShw2]), GstPte.u & X86_PTE_PAE_PG_MASK));
                        pgmPoolTracDerefGCPhysHint(pPool, pPage,
                                                   PGMSHWPTEPAE_GET_HCPHYS(uShw.pPTPae->a[iShw2]),
                                                   GstPte.u & X86_PTE_PAE_PG_MASK,
                                                   iShw2);
                        PGMSHWPTEPAE_ATOMIC_SET(uShw.pPTPae->a[iShw2], 0);
                    }
                }
                break;
            }

            case PGMPOOLKIND_32BIT_PD:
            {
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = off / sizeof(X86PTE);         // ASSUMING 32-bit guest paging!

                LogFlow(("pgmPoolMonitorChainChanging: PGMPOOLKIND_32BIT_PD %x\n", iShw));
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPD));
#  ifdef VBOX_WITH_RAW_MODE_NOT_R0
                if (uShw.pPD->a[iShw].u & PGM_PDFLAGS_MAPPING)
                {
                    Assert(pgmMapAreMappingsEnabled(pVM));
                    VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                    STAM_COUNTER_INC(&(pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZGuestCR3WriteConflict));
                    LogFlow(("pgmPoolMonitorChainChanging: Detected conflict at iShw=%#x!\n", iShw));
                    break;
                }
                else
#  endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                {
                    if (uShw.pPD->a[iShw].n.u1Present)
                    {
                        LogFlow(("pgmPoolMonitorChainChanging: 32 bit pd iShw=%#x: %RX64 -> freeing it!\n", iShw, uShw.pPD->a[iShw].u));
                        pgmPoolFree(pVM,
                                    uShw.pPD->a[iShw].u & X86_PDE_PAE_PG_MASK,
                                    pPage->idx,
                                    iShw);
                        ASMAtomicWriteU32(&uShw.pPD->a[iShw].u, 0);
                    }
                }
                /* paranoia / a bit assumptive. */
                if (    (off & 3)
                    &&  (off & 3) + cbWrite > sizeof(X86PTE))
                {
                    const unsigned iShw2 = (off + cbWrite - 1) / sizeof(X86PTE);
                    if (    iShw2 != iShw
                        &&  iShw2 < RT_ELEMENTS(uShw.pPD->a))
                    {
#   ifdef VBOX_WITH_RAW_MODE_NOT_R0
                        if (uShw.pPD->a[iShw2].u & PGM_PDFLAGS_MAPPING)
                        {
                            Assert(pgmMapAreMappingsEnabled(pVM));
                            STAM_COUNTER_INC(&(pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZGuestCR3WriteConflict));
                            VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                            LogFlow(("pgmPoolMonitorChainChanging: Detected conflict at iShw2=%#x!\n", iShw2));
                            break;
                        }
#   endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                        if (uShw.pPD->a[iShw2].n.u1Present)
                        {
                            LogFlow(("pgmPoolMonitorChainChanging: 32 bit pd iShw=%#x: %RX64 -> freeing it!\n", iShw2, uShw.pPD->a[iShw2].u));
                            pgmPoolFree(pVM,
                                        uShw.pPD->a[iShw2].u & X86_PDE_PAE_PG_MASK,
                                        pPage->idx,
                                        iShw2);
                            ASMAtomicWriteU32(&uShw.pPD->a[iShw2].u, 0);
                        }
                    }
                }
#if 0 /* useful when running PGMAssertCR3(), a bit too troublesome for general use (TLBs). - not working any longer... */
                if (    uShw.pPD->a[iShw].n.u1Present
                    &&  !VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3))
                {
                    LogFlow(("pgmPoolMonitorChainChanging: iShw=%#x: %RX32 -> freeing it!\n", iShw, uShw.pPD->a[iShw].u));
# ifdef IN_RC       /* TLB load - we're pushing things a bit... */
                    ASMProbeReadByte(pvAddress);
# endif
                    pgmPoolFree(pVM, uShw.pPD->a[iShw].u & X86_PDE_PG_MASK, pPage->idx, iShw);
                    ASMAtomicWriteU32(&uShw.pPD->a[iShw].u, 0);
                }
#endif
                break;
            }

            case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
            {
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = off / sizeof(X86PDEPAE);
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPD));
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
                if (uShw.pPDPae->a[iShw].u & PGM_PDFLAGS_MAPPING)
                {
                    Assert(pgmMapAreMappingsEnabled(pVM));
                    VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                    STAM_COUNTER_INC(&(pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZGuestCR3WriteConflict));
                    LogFlow(("pgmPoolMonitorChainChanging: Detected conflict at iShw=%#x!\n", iShw));
                    break;
                }
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                /*
                 * Causes trouble when the guest uses a PDE to refer to the whole page table level
                 * structure. (Invalidate here; faults later on when it tries to change the page
                 * table entries -> recheck; probably only applies to the RC case.)
                 */
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
                else
#endif
                {
                    if (uShw.pPDPae->a[iShw].n.u1Present)
                    {
                        LogFlow(("pgmPoolMonitorChainChanging: pae pd iShw=%#x: %RX64 -> freeing it!\n", iShw, uShw.pPDPae->a[iShw].u));
                        pgmPoolFree(pVM,
                                    uShw.pPDPae->a[iShw].u & X86_PDE_PAE_PG_MASK,
                                    pPage->idx,
                                    iShw);
                        ASMAtomicWriteU64(&uShw.pPDPae->a[iShw].u, 0);
                    }
                }
                /* paranoia / a bit assumptive. */
                if (    (off & 7)
                    &&  (off & 7) + cbWrite > sizeof(X86PDEPAE))
                {
                    const unsigned iShw2 = (off + cbWrite - 1) / sizeof(X86PDEPAE);
                    AssertBreak(iShw2 < RT_ELEMENTS(uShw.pPDPae->a));

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
                    if (    iShw2 != iShw
                        &&  uShw.pPDPae->a[iShw2].u & PGM_PDFLAGS_MAPPING)
                    {
                        Assert(pgmMapAreMappingsEnabled(pVM));
                        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                        STAM_COUNTER_INC(&(pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZGuestCR3WriteConflict));
                        LogFlow(("pgmPoolMonitorChainChanging: Detected conflict at iShw2=%#x!\n", iShw2));
                        break;
                    }
                    else
#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                    if (uShw.pPDPae->a[iShw2].n.u1Present)
                    {
                        LogFlow(("pgmPoolMonitorChainChanging: pae pd iShw2=%#x: %RX64 -> freeing it!\n", iShw2, uShw.pPDPae->a[iShw2].u));
                        pgmPoolFree(pVM,
                                    uShw.pPDPae->a[iShw2].u & X86_PDE_PAE_PG_MASK,
                                    pPage->idx,
                                    iShw2);
                        ASMAtomicWriteU64(&uShw.pPDPae->a[iShw2].u, 0);
                    }
                }
                break;
            }

            case PGMPOOLKIND_PAE_PDPT:
            {
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPDPT));
                /*
                 * Hopefully this doesn't happen very often:
                 * - touching unused parts of the page
                 * - messing with the bits of pd pointers without changing the physical address
                 */
                /* PDPT roots are not page aligned; 32 byte only! */
                const unsigned offPdpt = GCPhysFault - pPage->GCPhys;

                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = offPdpt / sizeof(X86PDPE);
                if (iShw < X86_PG_PAE_PDPE_ENTRIES)          /* don't use RT_ELEMENTS(uShw.pPDPT->a), because that's for long mode only */
                {
# ifdef VBOX_WITH_RAW_MODE_NOT_R0
                    if (uShw.pPDPT->a[iShw].u & PGM_PLXFLAGS_MAPPING)
                    {
                        Assert(pgmMapAreMappingsEnabled(pVM));
                        STAM_COUNTER_INC(&(pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZGuestCR3WriteConflict));
                        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                        LogFlow(("pgmPoolMonitorChainChanging: Detected pdpt conflict at iShw=%#x!\n", iShw));
                        break;
                    }
                    else
# endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                    if (uShw.pPDPT->a[iShw].n.u1Present)
                    {
                        LogFlow(("pgmPoolMonitorChainChanging: pae pdpt iShw=%#x: %RX64 -> freeing it!\n", iShw, uShw.pPDPT->a[iShw].u));
                        pgmPoolFree(pVM,
                                    uShw.pPDPT->a[iShw].u & X86_PDPE_PG_MASK,
                                    pPage->idx,
                                    iShw);
                        ASMAtomicWriteU64(&uShw.pPDPT->a[iShw].u, 0);
                    }

                    /* paranoia / a bit assumptive. */
                    if (    (offPdpt & 7)
                        &&  (offPdpt & 7) + cbWrite > sizeof(X86PDPE))
                    {
                        const unsigned iShw2 = (offPdpt + cbWrite - 1) / sizeof(X86PDPE);
                        if (    iShw2 != iShw
                            &&  iShw2 < X86_PG_PAE_PDPE_ENTRIES)
                        {
# ifdef VBOX_WITH_RAW_MODE_NOT_R0
                            if (uShw.pPDPT->a[iShw2].u & PGM_PLXFLAGS_MAPPING)
                            {
                                Assert(pgmMapAreMappingsEnabled(pVM));
                                STAM_COUNTER_INC(&(pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZGuestCR3WriteConflict));
                                VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
                                LogFlow(("pgmPoolMonitorChainChanging: Detected conflict at iShw2=%#x!\n", iShw2));
                                break;
                            }
                            else
# endif /* VBOX_WITH_RAW_MODE_NOT_R0 */
                            if (uShw.pPDPT->a[iShw2].n.u1Present)
                            {
                                LogFlow(("pgmPoolMonitorChainChanging: pae pdpt iShw=%#x: %RX64 -> freeing it!\n", iShw2, uShw.pPDPT->a[iShw2].u));
                                pgmPoolFree(pVM,
                                            uShw.pPDPT->a[iShw2].u & X86_PDPE_PG_MASK,
                                            pPage->idx,
                                            iShw2);
                                ASMAtomicWriteU64(&uShw.pPDPT->a[iShw2].u, 0);
                            }
                        }
                    }
                }
                break;
            }

#ifndef IN_RC
            case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
            {
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPD));
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = off / sizeof(X86PDEPAE);
                Assert(!(uShw.pPDPae->a[iShw].u & PGM_PDFLAGS_MAPPING));
                if (uShw.pPDPae->a[iShw].n.u1Present)
                {
                    LogFlow(("pgmPoolMonitorChainChanging: pae pd iShw=%#x: %RX64 -> freeing it!\n", iShw, uShw.pPDPae->a[iShw].u));
                    pgmPoolFree(pVM,
                                uShw.pPDPae->a[iShw].u & X86_PDE_PAE_PG_MASK,
                                pPage->idx,
                                iShw);
                    ASMAtomicWriteU64(&uShw.pPDPae->a[iShw].u, 0);
                }
                /* paranoia / a bit assumptive. */
                if (    (off & 7)
                    &&  (off & 7) + cbWrite > sizeof(X86PDEPAE))
                {
                    const unsigned iShw2 = (off + cbWrite - 1) / sizeof(X86PDEPAE);
                    AssertBreak(iShw2 < RT_ELEMENTS(uShw.pPDPae->a));

                    Assert(!(uShw.pPDPae->a[iShw2].u & PGM_PDFLAGS_MAPPING));
                    if (uShw.pPDPae->a[iShw2].n.u1Present)
                    {
                        LogFlow(("pgmPoolMonitorChainChanging: pae pd iShw2=%#x: %RX64 -> freeing it!\n", iShw2, uShw.pPDPae->a[iShw2].u));
                        pgmPoolFree(pVM,
                                    uShw.pPDPae->a[iShw2].u & X86_PDE_PAE_PG_MASK,
                                    pPage->idx,
                                    iShw2);
                        ASMAtomicWriteU64(&uShw.pPDPae->a[iShw2].u, 0);
                    }
                }
                break;
            }

            case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
            {
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPDPT));
                /*
                 * Hopefully this doesn't happen very often:
                 * - messing with the bits of pd pointers without changing the physical address
                 */
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = off / sizeof(X86PDPE);
                if (uShw.pPDPT->a[iShw].n.u1Present)
                {
                    LogFlow(("pgmPoolMonitorChainChanging: pdpt iShw=%#x: %RX64 -> freeing it!\n", iShw, uShw.pPDPT->a[iShw].u));
                    pgmPoolFree(pVM, uShw.pPDPT->a[iShw].u & X86_PDPE_PG_MASK, pPage->idx, iShw);
                    ASMAtomicWriteU64(&uShw.pPDPT->a[iShw].u, 0);
                }
                /* paranoia / a bit assumptive. */
                if (    (off & 7)
                    &&  (off & 7) + cbWrite > sizeof(X86PDPE))
                {
                    const unsigned iShw2 = (off + cbWrite - 1) / sizeof(X86PDPE);
                    if (uShw.pPDPT->a[iShw2].n.u1Present)
                    {
                        LogFlow(("pgmPoolMonitorChainChanging: pdpt iShw2=%#x: %RX64 -> freeing it!\n", iShw2, uShw.pPDPT->a[iShw2].u));
                        pgmPoolFree(pVM, uShw.pPDPT->a[iShw2].u & X86_PDPE_PG_MASK, pPage->idx, iShw2);
                        ASMAtomicWriteU64(&uShw.pPDPT->a[iShw2].u, 0);
                    }
                }
                break;
            }

            case PGMPOOLKIND_64BIT_PML4:
            {
                STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitor,FaultPML4));
                /*
                 * Hopefully this doesn't happen very often:
                 * - messing with the bits of pd pointers without changing the physical address
                 */
                uShw.pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
                const unsigned iShw = off / sizeof(X86PDPE);
                if (uShw.pPML4->a[iShw].n.u1Present)
                {
                    LogFlow(("pgmPoolMonitorChainChanging: pml4 iShw=%#x: %RX64 -> freeing it!\n", iShw, uShw.pPML4->a[iShw].u));
                    pgmPoolFree(pVM, uShw.pPML4->a[iShw].u & X86_PML4E_PG_MASK, pPage->idx, iShw);
                    ASMAtomicWriteU64(&uShw.pPML4->a[iShw].u, 0);
                }
                /* paranoia / a bit assumptive. */
                if (    (off & 7)
                    &&  (off & 7) + cbWrite > sizeof(X86PDPE))
                {
                    const unsigned iShw2 = (off + cbWrite - 1) / sizeof(X86PML4E);
                    if (uShw.pPML4->a[iShw2].n.u1Present)
                    {
                        LogFlow(("pgmPoolMonitorChainChanging: pml4 iShw2=%#x: %RX64 -> freeing it!\n", iShw2, uShw.pPML4->a[iShw2].u));
                        pgmPoolFree(pVM, uShw.pPML4->a[iShw2].u & X86_PML4E_PG_MASK, pPage->idx, iShw2);
                        ASMAtomicWriteU64(&uShw.pPML4->a[iShw2].u, 0);
                    }
                }
                break;
            }
#endif /* IN_RING0 */

            default:
                AssertFatalMsgFailed(("enmKind=%d\n", pPage->enmKind));
        }
        PGM_DYNMAP_UNUSED_HINT_VM(pVM, uShw.pv);

        /* next */
        if (pPage->iMonitoredNext == NIL_PGMPOOL_IDX)
            return;
        pPage = &pPool->aPages[pPage->iMonitoredNext];
    }
}

#ifndef IN_RING3

/**
 * Checks if a access could be a fork operation in progress.
 *
 * Meaning, that the guest is setting up the parent process for Copy-On-Write.
 *
 * @returns true if it's likely that we're forking, otherwise false.
 * @param   pPool       The pool.
 * @param   pDis        The disassembled instruction.
 * @param   offFault    The access offset.
 */
DECLINLINE(bool) pgmRZPoolMonitorIsForking(PPGMPOOL pPool, PDISCPUSTATE pDis, unsigned offFault)
{
    /*
     * i386 linux is using btr to clear X86_PTE_RW.
     * The functions involved are (2.6.16 source inspection):
     *      clear_bit
     *      ptep_set_wrprotect
     *      copy_one_pte
     *      copy_pte_range
     *      copy_pmd_range
     *      copy_pud_range
     *      copy_page_range
     *      dup_mmap
     *      dup_mm
     *      copy_mm
     *      copy_process
     *      do_fork
     */
    if (    pDis->pCurInstr->uOpcode == OP_BTR
        &&  !(offFault & 4)
        /** @todo Validate that the bit index is X86_PTE_RW. */
            )
    {
        STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitorPf,Fork)); RT_NOREF_PV(pPool);
        return true;
    }
    return false;
}


/**
 * Determine whether the page is likely to have been reused.
 *
 * @returns true if we consider the page as being reused for a different purpose.
 * @returns false if we consider it to still be a paging page.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pRegFrame   Trap register frame.
 * @param   pDis        The disassembly info for the faulting instruction.
 * @param   pvFault     The fault address.
 * @param   pPage       The pool page being accessed.
 *
 * @remark  The REP prefix check is left to the caller because of STOSD/W.
 */
DECLINLINE(bool) pgmRZPoolMonitorIsReused(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pDis, RTGCPTR pvFault,
                                          PPGMPOOLPAGE pPage)
{
    /* Locked (CR3, PDPTR*4) should not be reusable.  Considering them as
       such may cause loops booting tst-ubuntu-15_10-64-efi, ++. */
    if (pPage->cLocked)
    {
        Log2(("pgmRZPoolMonitorIsReused: %RGv (%p) can't have been resued, because it's locked!\n", pvFault, pPage));
        return false;
    }

# ifndef IN_RC
    /** @todo could make this general, faulting close to rsp should be a safe reuse heuristic. */
    if (   HMHasPendingIrq(pVM)
        && (pRegFrame->rsp - pvFault) < 32)
    {
        /* Fault caused by stack writes while trying to inject an interrupt event. */
        Log(("pgmRZPoolMonitorIsReused: reused %RGv for interrupt stack (rsp=%RGv).\n", pvFault, pRegFrame->rsp));
        return true;
    }
# else
    NOREF(pVM); NOREF(pvFault);
# endif

    LogFlow(("Reused instr %RGv %d at %RGv param1.fUse=%llx param1.reg=%d\n", pRegFrame->rip, pDis->pCurInstr->uOpcode, pvFault, pDis->Param1.fUse,  pDis->Param1.Base.idxGenReg));

    /* Non-supervisor mode write means it's used for something else. */
    if (CPUMGetGuestCPL(pVCpu) == 3)
        return true;

    switch (pDis->pCurInstr->uOpcode)
    {
        /* call implies the actual push of the return address faulted */
        case OP_CALL:
            Log4(("pgmRZPoolMonitorIsReused: CALL\n"));
            return true;
        case OP_PUSH:
            Log4(("pgmRZPoolMonitorIsReused: PUSH\n"));
            return true;
        case OP_PUSHF:
            Log4(("pgmRZPoolMonitorIsReused: PUSHF\n"));
            return true;
        case OP_PUSHA:
            Log4(("pgmRZPoolMonitorIsReused: PUSHA\n"));
            return true;
        case OP_FXSAVE:
            Log4(("pgmRZPoolMonitorIsReused: FXSAVE\n"));
            return true;
        case OP_MOVNTI:     /* solaris - block_zero_no_xmm */
            Log4(("pgmRZPoolMonitorIsReused: MOVNTI\n"));
            return true;
        case OP_MOVNTDQ:    /* solaris - hwblkclr & hwblkpagecopy */
            Log4(("pgmRZPoolMonitorIsReused: MOVNTDQ\n"));
            return true;
        case OP_MOVSWD:
        case OP_STOSWD:
            if (    pDis->fPrefix == (DISPREFIX_REP|DISPREFIX_REX)
                &&  pRegFrame->rcx >= 0x40
               )
            {
                Assert(pDis->uCpuMode == DISCPUMODE_64BIT);

                Log(("pgmRZPoolMonitorIsReused: OP_STOSQ\n"));
                return true;
            }
            break;

        default:
            /*
             * Anything having ESP on the left side means stack writes.
             */
            if (    (    (pDis->Param1.fUse & DISUSE_REG_GEN32)
                     ||  (pDis->Param1.fUse & DISUSE_REG_GEN64))
                &&  (pDis->Param1.Base.idxGenReg == DISGREG_ESP))
            {
                Log4(("pgmRZPoolMonitorIsReused: ESP\n"));
                return true;
            }
            break;
    }

    /*
     * Page table updates are very very unlikely to be crossing page boundraries,
     * and we don't want to deal with that in pgmPoolMonitorChainChanging and such.
     */
    uint32_t const cbWrite = DISGetParamSize(pDis, &pDis->Param1);
    if ( (((uintptr_t)pvFault + cbWrite) >> X86_PAGE_SHIFT) != ((uintptr_t)pvFault >> X86_PAGE_SHIFT) )
    {
        Log4(("pgmRZPoolMonitorIsReused: cross page write\n"));
        return true;
    }

    /*
     * Nobody does an unaligned 8 byte write to a page table, right.
     */
    if (cbWrite >= 8 && ((uintptr_t)pvFault & 7) != 0)
    {
        Log4(("pgmRZPoolMonitorIsReused: Unaligned 8+ byte write\n"));
        return true;
    }

    return false;
}


/**
 * Flushes the page being accessed.
 *
 * @returns VBox status code suitable for scheduling.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pPool       The pool.
 * @param   pPage       The pool page (head).
 * @param   pDis        The disassembly of the write instruction.
 * @param   pRegFrame   The trap register frame.
 * @param   GCPhysFault The fault address as guest physical address.
 * @param   pvFault     The fault address.
 * @todo VBOXSTRICTRC
 */
static int pgmRZPoolAccessPfHandlerFlush(PVM pVM, PVMCPU pVCpu, PPGMPOOL pPool, PPGMPOOLPAGE pPage, PDISCPUSTATE pDis,
                                         PCPUMCTXCORE pRegFrame, RTGCPHYS GCPhysFault, RTGCPTR pvFault)
{
    NOREF(pVM); NOREF(GCPhysFault);

    /*
     * First, do the flushing.
     */
    int rc = pgmPoolMonitorChainFlush(pPool, pPage);

    /*
     * Emulate the instruction (xp/w2k problem, requires pc/cr2/sp detection).
     * Must do this in raw mode (!); XP boot will fail otherwise.
     */
    VBOXSTRICTRC rc2 = EMInterpretInstructionDisasState(pVCpu, pDis, pRegFrame, pvFault, EMCODETYPE_ALL);
    if (rc2 == VINF_SUCCESS)
    { /* do nothing */ }
    else if (rc2 == VINF_EM_RESCHEDULE)
    {
        if (rc == VINF_SUCCESS)
            rc = VBOXSTRICTRC_VAL(rc2);
# ifndef IN_RING3
        VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);
# endif
    }
    else if (rc2 == VERR_EM_INTERPRETER)
    {
# ifdef IN_RC
        if (PATMIsPatchGCAddr(pVM, pRegFrame->eip))
        {
            LogFlow(("pgmRZPoolAccessPfHandlerFlush: Interpretation failed for patch code %04x:%RGv, ignoring.\n",
                     pRegFrame->cs.Sel, (RTGCPTR)pRegFrame->eip));
            rc = VINF_SUCCESS;
            STAM_COUNTER_INC(&pPool->StatMonitorPfRZIntrFailPatch2);
        }
        else
# endif
        {
            rc = VINF_EM_RAW_EMULATE_INSTR;
            STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitorPf,EmulateInstr));
        }
    }
    else if (RT_FAILURE_NP(rc2))
        rc = VBOXSTRICTRC_VAL(rc2);
    else
        AssertMsgFailed(("%Rrc\n", VBOXSTRICTRC_VAL(rc2))); /* ASSUMES no complicated stuff here. */

    LogFlow(("pgmRZPoolAccessPfHandlerFlush: returns %Rrc (flushed)\n", rc));
    return rc;
}


/**
 * Handles the STOSD write accesses.
 *
 * @returns VBox status code suitable for scheduling.
 * @param   pVM         The cross context VM structure.
 * @param   pPool       The pool.
 * @param   pPage       The pool page (head).
 * @param   pDis        The disassembly of the write instruction.
 * @param   pRegFrame   The trap register frame.
 * @param   GCPhysFault The fault address as guest physical address.
 * @param   pvFault     The fault address.
 */
DECLINLINE(int) pgmRZPoolAccessPfHandlerSTOSD(PVM pVM, PPGMPOOL pPool, PPGMPOOLPAGE pPage, PDISCPUSTATE pDis,
                                              PCPUMCTXCORE pRegFrame, RTGCPHYS GCPhysFault, RTGCPTR pvFault)
{
    unsigned uIncrement = pDis->Param1.cb;
    NOREF(pVM);

    Assert(pDis->uCpuMode == DISCPUMODE_32BIT || pDis->uCpuMode == DISCPUMODE_64BIT);
    Assert(pRegFrame->rcx <= 0x20);

# ifdef VBOX_STRICT
    if (pDis->uOpMode == DISCPUMODE_32BIT)
        Assert(uIncrement == 4);
    else
        Assert(uIncrement == 8);
# endif

    Log3(("pgmRZPoolAccessPfHandlerSTOSD\n"));

    /*
     * Increment the modification counter and insert it into the list
     * of modified pages the first time.
     */
    if (!pPage->cModifications++)
        pgmPoolMonitorModifiedInsert(pPool, pPage);

    /*
     * Execute REP STOSD.
     *
     * This ASSUMES that we're not invoked by Trap0e on in a out-of-sync
     * write situation, meaning that it's safe to write here.
     */
    PVMCPU      pVCpu = VMMGetCpu(pPool->CTX_SUFF(pVM));
    RTGCUINTPTR pu32 = (RTGCUINTPTR)pvFault;
    while (pRegFrame->rcx)
    {
# if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
        uint32_t iPrevSubset = PGMRZDynMapPushAutoSubset(pVCpu);
        pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhysFault, NULL, uIncrement);
        PGMRZDynMapPopAutoSubset(pVCpu, iPrevSubset);
# else
        pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhysFault, NULL, uIncrement);
# endif
# ifdef IN_RC
        *(uint32_t *)(uintptr_t)pu32 = pRegFrame->eax;
# else
        PGMPhysSimpleWriteGCPhys(pVM, GCPhysFault, &pRegFrame->rax, uIncrement);
# endif
        pu32           += uIncrement;
        GCPhysFault    += uIncrement;
        pRegFrame->rdi += uIncrement;
        pRegFrame->rcx--;
    }
    pRegFrame->rip += pDis->cbInstr;

    LogFlow(("pgmRZPoolAccessPfHandlerSTOSD: returns\n"));
    return VINF_SUCCESS;
}


/**
 * Handles the simple write accesses.
 *
 * @returns VBox status code suitable for scheduling.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pPool       The pool.
 * @param   pPage       The pool page (head).
 * @param   pDis        The disassembly of the write instruction.
 * @param   pRegFrame   The trap register frame.
 * @param   GCPhysFault The fault address as guest physical address.
 * @param   pvFault     The fault address.
 * @param   pfReused    Reused state (in/out)
 */
DECLINLINE(int) pgmRZPoolAccessPfHandlerSimple(PVM pVM, PVMCPU pVCpu, PPGMPOOL pPool, PPGMPOOLPAGE pPage, PDISCPUSTATE pDis,
                                               PCPUMCTXCORE pRegFrame, RTGCPHYS GCPhysFault, RTGCPTR pvFault, bool *pfReused)
{
    Log3(("pgmRZPoolAccessPfHandlerSimple\n"));
    NOREF(pVM);
    NOREF(pfReused); /* initialized by caller */

    /*
     * Increment the modification counter and insert it into the list
     * of modified pages the first time.
     */
    if (!pPage->cModifications++)
        pgmPoolMonitorModifiedInsert(pPool, pPage);

    /*
     * Clear all the pages. ASSUMES that pvFault is readable.
     */
# if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
    uint32_t iPrevSubset = PGMRZDynMapPushAutoSubset(pVCpu);
# endif

    uint32_t cbWrite = DISGetParamSize(pDis, &pDis->Param1);
    if (cbWrite <= 8)
        pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhysFault, NULL, cbWrite);
    else if (cbWrite <= 16)
    {
        pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhysFault, NULL, 8);
        pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhysFault + 8, NULL, cbWrite - 8);
    }
    else
    {
        Assert(cbWrite <= 32);
        for (uint32_t off = 0; off < cbWrite; off += 8)
            pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhysFault + off, NULL, RT_MIN(8, cbWrite - off));
    }

# if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
    PGMRZDynMapPopAutoSubset(pVCpu, iPrevSubset);
# endif

    /*
     * Interpret the instruction.
     */
    VBOXSTRICTRC rc = EMInterpretInstructionDisasState(pVCpu, pDis, pRegFrame, pvFault, EMCODETYPE_ALL);
    if (RT_SUCCESS(rc))
        AssertMsg(rc == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rc))); /* ASSUMES no complicated stuff here. */
    else if (rc == VERR_EM_INTERPRETER)
    {
        LogFlow(("pgmRZPoolAccessPfHandlerSimple: Interpretation failed for %04x:%RGv - opcode=%d\n",
                  pRegFrame->cs.Sel, (RTGCPTR)pRegFrame->rip, pDis->pCurInstr->uOpcode));
        rc = VINF_EM_RAW_EMULATE_INSTR;
        STAM_COUNTER_INC(&pPool->CTX_MID_Z(StatMonitorPf,EmulateInstr));
    }

# if 0 /* experimental code */
    if (rc == VINF_SUCCESS)
    {
        switch (pPage->enmKind)
        {
        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        {
            X86PTEPAE GstPte;
            int rc = pgmPoolPhysSimpleReadGCPhys(pVM, &GstPte, pvFault, GCPhysFault, sizeof(GstPte));
            AssertRC(rc);

            /* Check the new value written by the guest. If present and with a bogus physical address, then
             * it's fairly safe to assume the guest is reusing the PT.
             */
            if (GstPte.n.u1Present)
            {
                RTHCPHYS HCPhys = -1;
                int rc = PGMPhysGCPhys2HCPhys(pVM, GstPte.u & X86_PTE_PAE_PG_MASK, &HCPhys);
                if (rc != VINF_SUCCESS)
                {
                    *pfReused = true;
                    STAM_COUNTER_INC(&pPool->StatForceFlushReused);
                }
            }
            break;
        }
        }
    }
# endif

    LogFlow(("pgmRZPoolAccessPfHandlerSimple: returns %Rrc\n", VBOXSTRICTRC_VAL(rc)));
    return VBOXSTRICTRC_VAL(rc);
}


/**
 * @callback_method_impl{FNPGMRZPHYSPFHANDLER,
 *      \#PF access handler callback for page table pages.}
 *
 * @remarks The @a pvUser argument points to the PGMPOOLPAGE.
 */
DECLEXPORT(VBOXSTRICTRC) pgmRZPoolAccessPfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                  RTGCPTR pvFault, RTGCPHYS GCPhysFault, void *pvUser)
{
    STAM_PROFILE_START(&pVM->pgm.s.CTX_SUFF(pPool)->StatMonitorRZ, a);
    PPGMPOOL        pPool = pVM->pgm.s.CTX_SUFF(pPool);
    PPGMPOOLPAGE    pPage = (PPGMPOOLPAGE)pvUser;
    unsigned        cMaxModifications;
    bool            fForcedFlush = false;
    NOREF(uErrorCode);

    LogFlow(("pgmRZPoolAccessPfHandler: pvFault=%RGv pPage=%p:{.idx=%d} GCPhysFault=%RGp\n", pvFault, pPage, pPage->idx, GCPhysFault));

    pgmLock(pVM);
    if (PHYS_PAGE_ADDRESS(GCPhysFault) != PHYS_PAGE_ADDRESS(pPage->GCPhys))
    {
        /* Pool page changed while we were waiting for the lock; ignore. */
        Log(("CPU%d: pgmRZPoolAccessPfHandler pgm pool page for %RGp changed (to %RGp) while waiting!\n", pVCpu->idCpu, PHYS_PAGE_ADDRESS(GCPhysFault), PHYS_PAGE_ADDRESS(pPage->GCPhys)));
        STAM_PROFILE_STOP_EX(&pVM->pgm.s.CTX_SUFF(pPool)->StatMonitorPfRZ, &pPool->StatMonitorPfRZHandled, a);
        pgmUnlock(pVM);
        return VINF_SUCCESS;
    }
# ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    if (pPage->fDirty)
    {
        Assert(VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_TLB_FLUSH));
        pgmUnlock(pVM);
        return VINF_SUCCESS;    /* SMP guest case where we were blocking on the pgm lock while the same page was being marked dirty. */
    }
# endif

# if 0 /* test code defined(VBOX_STRICT) && defined(PGMPOOL_WITH_OPTIMIZED_DIRTY_PT) */
    if (pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT)
    {
        void *pvShw = PGMPOOL_PAGE_2_PTR(pPool->CTX_SUFF(pVM), pPage);
        void *pvGst;
        int rc = PGM_GCPHYS_2_PTR(pPool->CTX_SUFF(pVM), pPage->GCPhys, &pvGst); AssertReleaseRC(rc);
        pgmPoolTrackCheckPTPaePae(pPool, pPage, (PPGMSHWPTPAE)pvShw, (PCX86PTPAE)pvGst);
        PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvGst);
        PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvShw);
    }
# endif

    /*
     * Disassemble the faulting instruction.
     */
    PDISCPUSTATE pDis = &pVCpu->pgm.s.DisState;
    int rc = EMInterpretDisasCurrent(pVM, pVCpu, pDis, NULL);
    if (RT_UNLIKELY(rc != VINF_SUCCESS))
    {
        AssertMsg(rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT, ("Unexpected rc %d\n", rc));
        pgmUnlock(pVM);
        return rc;
    }

    Assert(pPage->enmKind != PGMPOOLKIND_FREE);

    /*
     * We should ALWAYS have the list head as user parameter. This
     * is because we use that page to record the changes.
     */
    Assert(pPage->iMonitoredPrev == NIL_PGMPOOL_IDX);

# ifdef IN_RING0
    /* Maximum nr of modifications depends on the page type. */
    if (    pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT
        ||  pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_32BIT_PT)
        cMaxModifications = 4;
    else
        cMaxModifications = 24;
# else
    cMaxModifications = 48;
# endif

    /*
     * Incremental page table updates should weigh more than random ones.
     * (Only applies when started from offset 0)
     */
    pVCpu->pgm.s.cPoolAccessHandler++;
    if (    pPage->GCPtrLastAccessHandlerRip >= pRegFrame->rip - 0x40      /* observed loops in Windows 7 x64 */
        &&  pPage->GCPtrLastAccessHandlerRip <  pRegFrame->rip + 0x40
        &&  pvFault == (pPage->GCPtrLastAccessHandlerFault + pDis->Param1.cb)
        &&  pVCpu->pgm.s.cPoolAccessHandler == pPage->cLastAccessHandler + 1)
    {
        Log(("Possible page reuse cMods=%d -> %d (locked=%d type=%s)\n", pPage->cModifications, pPage->cModifications * 2, pgmPoolIsPageLocked(pPage), pgmPoolPoolKindToStr(pPage->enmKind)));
        Assert(pPage->cModifications < 32000);
        pPage->cModifications               = pPage->cModifications * 2;
        pPage->GCPtrLastAccessHandlerFault  = pvFault;
        pPage->cLastAccessHandler           = pVCpu->pgm.s.cPoolAccessHandler;
        if (pPage->cModifications >= cMaxModifications)
        {
            STAM_COUNTER_INC(&pPool->StatMonitorPfRZFlushReinit);
            fForcedFlush = true;
        }
    }

    if (pPage->cModifications >= cMaxModifications)
        Log(("Mod overflow %RGv cMods=%d (locked=%d type=%s)\n", pvFault, pPage->cModifications, pgmPoolIsPageLocked(pPage), pgmPoolPoolKindToStr(pPage->enmKind)));

    /*
     * Check if it's worth dealing with.
     */
    bool fReused = false;
    bool fNotReusedNotForking = false;
    if (    (   pPage->cModifications < cMaxModifications   /** @todo \#define */ /** @todo need to check that it's not mapping EIP. */ /** @todo adjust this! */
             || pgmPoolIsPageLocked(pPage)
            )
        &&  !(fReused = pgmRZPoolMonitorIsReused(pVM, pVCpu, pRegFrame, pDis, pvFault, pPage))
        &&  !pgmRZPoolMonitorIsForking(pPool, pDis, GCPhysFault & PAGE_OFFSET_MASK))
    {
        /*
         * Simple instructions, no REP prefix.
         */
        if (!(pDis->fPrefix & (DISPREFIX_REP | DISPREFIX_REPNE)))
        {
            rc = pgmRZPoolAccessPfHandlerSimple(pVM, pVCpu, pPool, pPage, pDis, pRegFrame, GCPhysFault, pvFault, &fReused);
            if (fReused)
                goto flushPage;

            /* A mov instruction to change the first page table entry will be remembered so we can detect
             * full page table changes early on. This will reduce the amount of unnecessary traps we'll take.
             */
            if (   rc == VINF_SUCCESS
                && !pPage->cLocked                      /* only applies to unlocked pages as we can't free locked ones (e.g. cr3 root). */
                && pDis->pCurInstr->uOpcode == OP_MOV
                && (pvFault & PAGE_OFFSET_MASK) == 0)
            {
                pPage->GCPtrLastAccessHandlerFault = pvFault;
                pPage->cLastAccessHandler          = pVCpu->pgm.s.cPoolAccessHandler;
                pPage->GCPtrLastAccessHandlerRip   = pRegFrame->rip;
                /* Make sure we don't kick out a page too quickly. */
                if (pPage->cModifications > 8)
                    pPage->cModifications = 2;
            }
            else if (pPage->GCPtrLastAccessHandlerFault == pvFault)
            {
                /* ignore the 2nd write to this page table entry. */
                pPage->cLastAccessHandler       = pVCpu->pgm.s.cPoolAccessHandler;
            }
            else
            {
                pPage->GCPtrLastAccessHandlerFault = NIL_RTGCPTR;
                pPage->GCPtrLastAccessHandlerRip   = 0;
            }

            STAM_PROFILE_STOP_EX(&pVM->pgm.s.CTX_SUFF(pPool)->StatMonitorPfRZ, &pPool->StatMonitorPfRZHandled, a);
            pgmUnlock(pVM);
            return rc;
        }

        /*
         * Windows is frequently doing small memset() operations (netio test 4k+).
         * We have to deal with these or we'll kill the cache and performance.
         */
        if (    pDis->pCurInstr->uOpcode == OP_STOSWD
            &&  !pRegFrame->eflags.Bits.u1DF
            &&  pDis->uOpMode == pDis->uCpuMode
            &&  pDis->uAddrMode == pDis->uCpuMode)
        {
            bool fValidStosd = false;

            if (    pDis->uCpuMode == DISCPUMODE_32BIT
                &&  pDis->fPrefix == DISPREFIX_REP
                &&  pRegFrame->ecx <= 0x20
                &&  pRegFrame->ecx * 4 <= PAGE_SIZE - ((uintptr_t)pvFault & PAGE_OFFSET_MASK)
                &&  !((uintptr_t)pvFault & 3)
                &&  (pRegFrame->eax == 0 || pRegFrame->eax == 0x80) /* the two values observed. */
                )
            {
                fValidStosd = true;
                pRegFrame->rcx &= 0xffffffff;   /* paranoia */
            }
            else
            if (    pDis->uCpuMode == DISCPUMODE_64BIT
                &&  pDis->fPrefix == (DISPREFIX_REP | DISPREFIX_REX)
                &&  pRegFrame->rcx <= 0x20
                &&  pRegFrame->rcx * 8 <= PAGE_SIZE - ((uintptr_t)pvFault & PAGE_OFFSET_MASK)
                &&  !((uintptr_t)pvFault & 7)
                &&  (pRegFrame->rax == 0 || pRegFrame->rax == 0x80) /* the two values observed. */
                )
            {
                fValidStosd = true;
            }

            if (fValidStosd)
            {
                rc = pgmRZPoolAccessPfHandlerSTOSD(pVM, pPool, pPage, pDis, pRegFrame, GCPhysFault, pvFault);
                STAM_PROFILE_STOP_EX(&pVM->pgm.s.CTX_SUFF(pPool)->StatMonitorPfRZ, &pPool->StatMonitorPfRZRepStosd, a);
                pgmUnlock(pVM);
                return rc;
            }
        }

        /* REP prefix, don't bother. */
        STAM_COUNTER_INC(&pPool->StatMonitorPfRZRepPrefix);
        Log4(("pgmRZPoolAccessPfHandler: eax=%#x ecx=%#x edi=%#x esi=%#x rip=%RGv opcode=%d prefix=%#x\n",
              pRegFrame->eax, pRegFrame->ecx, pRegFrame->edi, pRegFrame->esi, (RTGCPTR)pRegFrame->rip, pDis->pCurInstr->uOpcode, pDis->fPrefix));
        fNotReusedNotForking = true;
    }

# if defined(PGMPOOL_WITH_OPTIMIZED_DIRTY_PT) && defined(IN_RING0)
    /* E.g. Windows 7 x64 initializes page tables and touches some pages in the table during the process. This
     * leads to pgm pool trashing and an excessive amount of write faults due to page monitoring.
     */
    if (    pPage->cModifications >= cMaxModifications
        &&  !fForcedFlush
        &&  (pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT || pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_32BIT_PT)
        &&  (   fNotReusedNotForking
             || (   !pgmRZPoolMonitorIsReused(pVM, pVCpu, pRegFrame, pDis, pvFault, pPage)
                 && !pgmRZPoolMonitorIsForking(pPool, pDis, GCPhysFault & PAGE_OFFSET_MASK))
            )
       )
    {
        Assert(!pgmPoolIsPageLocked(pPage));
        Assert(pPage->fDirty == false);

        /* Flush any monitored duplicates as we will disable write protection. */
        if (    pPage->iMonitoredNext != NIL_PGMPOOL_IDX
            ||  pPage->iMonitoredPrev != NIL_PGMPOOL_IDX)
        {
            PPGMPOOLPAGE pPageHead = pPage;

            /* Find the monitor head. */
            while (pPageHead->iMonitoredPrev != NIL_PGMPOOL_IDX)
                pPageHead = &pPool->aPages[pPageHead->iMonitoredPrev];

            while (pPageHead)
            {
                unsigned idxNext = pPageHead->iMonitoredNext;

                if (pPageHead != pPage)
                {
                    STAM_COUNTER_INC(&pPool->StatDirtyPageDupFlush);
                    Log(("Flush duplicate page idx=%d GCPhys=%RGp type=%s\n", pPageHead->idx, pPageHead->GCPhys, pgmPoolPoolKindToStr(pPageHead->enmKind)));
                    int rc2 = pgmPoolFlushPage(pPool, pPageHead);
                    AssertRC(rc2);
                }

                if (idxNext == NIL_PGMPOOL_IDX)
                    break;

                pPageHead = &pPool->aPages[idxNext];
            }
        }

        /* The flushing above might fail for locked pages, so double check. */
        if (    pPage->iMonitoredNext == NIL_PGMPOOL_IDX
            &&  pPage->iMonitoredPrev == NIL_PGMPOOL_IDX)
        {
            pgmPoolAddDirtyPage(pVM, pPool, pPage);

            /* Temporarily allow write access to the page table again. */
            rc = PGMHandlerPhysicalPageTempOff(pVM, pPage->GCPhys & PAGE_BASE_GC_MASK, pPage->GCPhys & PAGE_BASE_GC_MASK);
            if (rc == VINF_SUCCESS)
            {
                rc = PGMShwMakePageWritable(pVCpu, pvFault, PGM_MK_PG_IS_WRITE_FAULT);
                AssertMsg(rc == VINF_SUCCESS
                        /* In the SMP case the page table might be removed while we wait for the PGM lock in the trap handler. */
                        ||  rc == VERR_PAGE_TABLE_NOT_PRESENT
                        ||  rc == VERR_PAGE_NOT_PRESENT,
                        ("PGMShwModifyPage -> GCPtr=%RGv rc=%d\n", pvFault, rc));
#  ifdef VBOX_STRICT
                pPage->GCPtrDirtyFault = pvFault;
#  endif

                STAM_PROFILE_STOP(&pVM->pgm.s.CTX_SUFF(pPool)->StatMonitorPfRZ, a);
                pgmUnlock(pVM);
                return rc;
            }
        }
    }
# endif /* PGMPOOL_WITH_OPTIMIZED_DIRTY_PT */

    STAM_COUNTER_INC(&pPool->StatMonitorPfRZFlushModOverflow);
flushPage:
    /*
     * Not worth it, so flush it.
     *
     * If we considered it to be reused, don't go back to ring-3
     * to emulate failed instructions since we usually cannot
     * interpret then. This may be a bit risky, in which case
     * the reuse detection must be fixed.
     */
    rc = pgmRZPoolAccessPfHandlerFlush(pVM, pVCpu, pPool, pPage, pDis, pRegFrame, GCPhysFault, pvFault);
    if (    rc == VINF_EM_RAW_EMULATE_INSTR
        &&  fReused)
    {
        /* Make sure that the current instruction still has shadow page backing, otherwise we'll end up in a loop. */
        if (PGMShwGetPage(pVCpu, pRegFrame->rip, NULL, NULL) == VINF_SUCCESS)
            rc = VINF_SUCCESS;  /* safe to restart the instruction. */
    }
    STAM_PROFILE_STOP_EX(&pVM->pgm.s.CTX_SUFF(pPool)->StatMonitorPfRZ, &pPool->StatMonitorPfRZFlushPage, a);
    pgmUnlock(pVM);
    return rc;
}

#endif /* !IN_RING3 */

/**
 * @callback_method_impl{FNPGMPHYSHANDLER,
 *      Access handler for shadowed page table pages.}
 *
 * @remarks Only uses the VINF_PGM_HANDLER_DO_DEFAULT status.
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC)
pgmPoolAccessHandler(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, void *pvPhys, void *pvBuf, size_t cbBuf,
                     PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    PPGMPOOL        pPool = pVM->pgm.s.CTX_SUFF(pPool);
    STAM_PROFILE_START(&pPool->CTX_SUFF_Z(StatMonitor), a);
    PPGMPOOLPAGE    pPage = (PPGMPOOLPAGE)pvUser;
    LogFlow(("PGM_ALL_CB_DECL: GCPhys=%RGp %p:{.Core=%RHp, .idx=%d, .GCPhys=%RGp, .enmType=%d}\n",
             GCPhys, pPage, pPage->Core.Key, pPage->idx, pPage->GCPhys, pPage->enmKind));

    NOREF(pvPhys); NOREF(pvBuf); NOREF(enmAccessType);

    pgmLock(pVM);

#ifdef VBOX_WITH_STATISTICS
    /*
     * Collect stats on the access.
     */
    AssertCompile(RT_ELEMENTS(pPool->CTX_MID_Z(aStatMonitor,Sizes)) == 19);
    if (cbBuf <= 16 && cbBuf > 0)
        STAM_COUNTER_INC(&pPool->CTX_MID_Z(aStatMonitor,Sizes)[cbBuf - 1]);
    else if (cbBuf >= 17 && cbBuf < 32)
        STAM_COUNTER_INC(&pPool->CTX_MID_Z(aStatMonitor,Sizes)[16]);
    else if (cbBuf >= 32 && cbBuf < 64)
        STAM_COUNTER_INC(&pPool->CTX_MID_Z(aStatMonitor,Sizes)[17]);
    else if (cbBuf >= 64)
        STAM_COUNTER_INC(&pPool->CTX_MID_Z(aStatMonitor,Sizes)[18]);

    uint8_t cbAlign;
    switch (pPage->enmKind)
    {
        default:
            cbAlign = 7;
            break;
        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_32BIT_PD_PHYS:
            cbAlign = 3;
            break;
    }
    AssertCompile(RT_ELEMENTS(pPool->CTX_MID_Z(aStatMonitor,Misaligned)) == 7);
    if ((uint8_t)GCPhys & cbAlign)
        STAM_COUNTER_INC(&pPool->CTX_MID_Z(aStatMonitor,Misaligned)[((uint8_t)GCPhys & cbAlign) - 1]);
#endif

    /*
     * Make sure the pool page wasn't modified by a different CPU.
     */
    if (PHYS_PAGE_ADDRESS(GCPhys) == PHYS_PAGE_ADDRESS(pPage->GCPhys))
    {
        Assert(pPage->enmKind != PGMPOOLKIND_FREE);

        /* The max modification count before flushing depends on the context and page type. */
#ifdef IN_RING3
        uint16_t const cMaxModifications = 96; /* it's cheaper here, right? */
#else
        uint16_t cMaxModifications;
        if (    pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT
            ||  pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_32BIT_PT)
            cMaxModifications = 4;
        else
            cMaxModifications = 24;
# ifdef IN_RC
        cMaxModifications *= 2; /* traps are cheaper than exists. */
# endif
#endif

        /*
         * We don't have to be very sophisticated about this since there are relativly few calls here.
         * However, we must try our best to detect any non-cpu accesses (disk / networking).
         */
        if (   (   pPage->cModifications < cMaxModifications
                || pgmPoolIsPageLocked(pPage) )
            && enmOrigin != PGMACCESSORIGIN_DEVICE
            && cbBuf <= 16)
        {
            /* Clear the shadow entry. */
            if (!pPage->cModifications++)
                pgmPoolMonitorModifiedInsert(pPool, pPage);

            if (cbBuf <= 8)
                pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhys, pvBuf, (uint32_t)cbBuf);
            else
            {
                pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhys, pvBuf, 8);
                pgmPoolMonitorChainChanging(pVCpu, pPool, pPage, GCPhys + 8, (uint8_t *)pvBuf + 8, (uint32_t)cbBuf - 8);
            }
        }
        else
        {
            /* ASSUME that VERR_PGM_POOL_CLEARED can be ignored here and that FFs will deal with it in due time. */
            pgmPoolMonitorChainFlush(pPool, pPage);
        }

        STAM_PROFILE_STOP_EX(&pPool->CTX_SUFF_Z(StatMonitor), &pPool->CTX_MID_Z(StatMonitor,FlushPage), a);
    }
    else
        Log(("CPU%d: PGM_ALL_CB_DECL pgm pool page for %RGp changed (to %RGp) while waiting!\n", pVCpu->idCpu, PHYS_PAGE_ADDRESS(GCPhys), PHYS_PAGE_ADDRESS(pPage->GCPhys)));
    pgmUnlock(pVM);
    return VINF_PGM_HANDLER_DO_DEFAULT;
}


# ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT

#  if defined(VBOX_STRICT) && !defined(IN_RING3)

/**
 * Check references to guest physical memory in a PAE / PAE page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page table (mapping of the page).
 * @param   pGstPT      The guest page table.
 */
static void pgmPoolTrackCheckPTPaePae(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT, PCX86PTPAE pGstPT)
{
    unsigned cErrors    = 0;
    int      LastRc     = -1;           /* initialized to shut up gcc */
    unsigned LastPTE    = ~0U;          /* initialized to shut up gcc */
    RTHCPHYS LastHCPhys = NIL_RTHCPHYS; /* initialized to shut up gcc */
    PVM      pVM        = pPool->CTX_SUFF(pVM);

#ifdef VBOX_STRICT
    for (unsigned i = 0; i < RT_MIN(RT_ELEMENTS(pShwPT->a), pPage->iFirstPresent); i++)
        AssertMsg(!PGMSHWPTEPAE_IS_P(pShwPT->a[i]), ("Unexpected PTE: idx=%d %RX64 (first=%d)\n", i, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]),  pPage->iFirstPresent));
#endif
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++)
    {
        if (PGMSHWPTEPAE_IS_P(pShwPT->a[i]))
        {
            RTHCPHYS HCPhys = NIL_RTHCPHYS;
            int rc = PGMPhysGCPhys2HCPhys(pVM, pGstPT->a[i].u & X86_PTE_PAE_PG_MASK, &HCPhys);
            if (    rc != VINF_SUCCESS
                ||  PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]) != HCPhys)
            {
                Log(("rc=%d idx=%d guest %RX64 shw=%RX64 vs %RHp\n", rc, i, pGstPT->a[i].u, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]), HCPhys));
                LastPTE     = i;
                LastRc      = rc;
                LastHCPhys  = HCPhys;
                cErrors++;

                RTHCPHYS HCPhysPT = NIL_RTHCPHYS;
                rc = PGMPhysGCPhys2HCPhys(pVM, pPage->GCPhys, &HCPhysPT);
                AssertRC(rc);

                for (unsigned iPage = 0; iPage < pPool->cCurPages; iPage++)
                {
                    PPGMPOOLPAGE pTempPage = &pPool->aPages[iPage];

                    if (pTempPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT)
                    {
                        PPGMSHWPTPAE pShwPT2 = (PPGMSHWPTPAE)PGMPOOL_PAGE_2_PTR(pVM, pTempPage);

                        for (unsigned j = 0; j < RT_ELEMENTS(pShwPT->a); j++)
                        {
                            if (   PGMSHWPTEPAE_IS_P_RW(pShwPT2->a[j])
                                && PGMSHWPTEPAE_GET_HCPHYS(pShwPT2->a[j]) == HCPhysPT)
                            {
                                Log(("GCPhys=%RGp idx=%d %RX64 vs %RX64\n", pTempPage->GCPhys, j, PGMSHWPTEPAE_GET_LOG(pShwPT->a[j]), PGMSHWPTEPAE_GET_LOG(pShwPT2->a[j])));
                            }
                        }

                        PGM_DYNMAP_UNUSED_HINT_VM(pVM, pShwPT2);
                    }
                }
            }
        }
    }
    AssertMsg(!cErrors, ("cErrors=%d: last rc=%d idx=%d guest %RX64 shw=%RX64 vs %RHp\n", cErrors, LastRc, LastPTE, pGstPT->a[LastPTE].u, PGMSHWPTEPAE_GET_LOG(pShwPT->a[LastPTE]), LastHCPhys));
}


/**
 * Check references to guest physical memory in a PAE / 32-bit page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page table (mapping of the page).
 * @param   pGstPT      The guest page table.
 */
static void pgmPoolTrackCheckPTPae32Bit(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT, PCX86PT pGstPT)
{
    unsigned cErrors    = 0;
    int      LastRc     = -1;           /* initialized to shut up gcc */
    unsigned LastPTE    = ~0U;          /* initialized to shut up gcc */
    RTHCPHYS LastHCPhys = NIL_RTHCPHYS; /* initialized to shut up gcc */
    PVM      pVM        = pPool->CTX_SUFF(pVM);

#ifdef VBOX_STRICT
    for (unsigned i = 0; i < RT_MIN(RT_ELEMENTS(pShwPT->a), pPage->iFirstPresent); i++)
        AssertMsg(!PGMSHWPTEPAE_IS_P(pShwPT->a[i]), ("Unexpected PTE: idx=%d %RX64 (first=%d)\n", i, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]),  pPage->iFirstPresent));
#endif
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++)
    {
        if (PGMSHWPTEPAE_IS_P(pShwPT->a[i]))
        {
            RTHCPHYS HCPhys = NIL_RTHCPHYS;
            int rc = PGMPhysGCPhys2HCPhys(pVM, pGstPT->a[i].u & X86_PTE_PG_MASK, &HCPhys);
            if (    rc != VINF_SUCCESS
                ||  PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]) != HCPhys)
            {
                Log(("rc=%d idx=%d guest %x shw=%RX64 vs %RHp\n", rc, i, pGstPT->a[i].u, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]), HCPhys));
                LastPTE     = i;
                LastRc      = rc;
                LastHCPhys  = HCPhys;
                cErrors++;

                RTHCPHYS HCPhysPT = NIL_RTHCPHYS;
                rc = PGMPhysGCPhys2HCPhys(pVM, pPage->GCPhys, &HCPhysPT);
                AssertRC(rc);

                for (unsigned iPage = 0; iPage < pPool->cCurPages; iPage++)
                {
                    PPGMPOOLPAGE pTempPage = &pPool->aPages[iPage];

                    if (pTempPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_32BIT_PT)
                    {
                        PPGMSHWPTPAE pShwPT2 = (PPGMSHWPTPAE)PGMPOOL_PAGE_2_PTR(pVM, pTempPage);

                        for (unsigned j = 0; j < RT_ELEMENTS(pShwPT->a); j++)
                        {
                            if (   PGMSHWPTEPAE_IS_P_RW(pShwPT2->a[j])
                                && PGMSHWPTEPAE_GET_HCPHYS(pShwPT2->a[j]) == HCPhysPT)
                            {
                                Log(("GCPhys=%RGp idx=%d %RX64 vs %RX64\n", pTempPage->GCPhys, j, PGMSHWPTEPAE_GET_LOG(pShwPT->a[j]), PGMSHWPTEPAE_GET_LOG(pShwPT2->a[j])));
                            }
                        }

                        PGM_DYNMAP_UNUSED_HINT_VM(pVM, pShwPT2);
                    }
                }
            }
        }
    }
    AssertMsg(!cErrors, ("cErrors=%d: last rc=%d idx=%d guest %x shw=%RX64 vs %RHp\n", cErrors, LastRc, LastPTE, pGstPT->a[LastPTE].u, PGMSHWPTEPAE_GET_LOG(pShwPT->a[LastPTE]), LastHCPhys));
}

#  endif /* VBOX_STRICT && !IN_RING3 */

/**
 * Clear references to guest physical memory in a PAE / PAE page table.
 *
 * @returns nr of changed PTEs
 * @param   pPool           The pool.
 * @param   pPage           The page.
 * @param   pShwPT          The shadow page table (mapping of the page).
 * @param   pGstPT          The guest page table.
 * @param   pOldGstPT       The old cached guest page table.
 * @param   fAllowRemoval   Bail out as soon as we encounter an invalid PTE
 * @param   pfFlush         Flush reused page table (out)
 */
DECLINLINE(unsigned) pgmPoolTrackFlushPTPaePae(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT, PCX86PTPAE pGstPT,
                                               PCX86PTPAE pOldGstPT, bool fAllowRemoval, bool *pfFlush)
{
    unsigned cChanged = 0;

#ifdef VBOX_STRICT
    for (unsigned i = 0; i < RT_MIN(RT_ELEMENTS(pShwPT->a), pPage->iFirstPresent); i++)
        AssertMsg(!PGMSHWPTEPAE_IS_P(pShwPT->a[i]), ("Unexpected PTE: idx=%d %RX64 (first=%d)\n", i, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]),  pPage->iFirstPresent));
#endif
    *pfFlush = false;

    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++)
    {
        /* Check the new value written by the guest. If present and with a bogus physical address, then
         * it's fairly safe to assume the guest is reusing the PT.
         */
        if (    fAllowRemoval
            &&  pGstPT->a[i].n.u1Present)
        {
            if (!PGMPhysIsGCPhysValid(pPool->CTX_SUFF(pVM), pGstPT->a[i].u & X86_PTE_PAE_PG_MASK))
            {
                *pfFlush = true;
                return ++cChanged;
            }
        }
        if (PGMSHWPTEPAE_IS_P(pShwPT->a[i]))
        {
            /* If the old cached PTE is identical, then there's no need to flush the shadow copy. */
            if ((pGstPT->a[i].u & X86_PTE_PAE_PG_MASK) == (pOldGstPT->a[i].u & X86_PTE_PAE_PG_MASK))
            {
#ifdef VBOX_STRICT
                RTHCPHYS HCPhys = NIL_RTGCPHYS;
                int rc = PGMPhysGCPhys2HCPhys(pPool->CTX_SUFF(pVM), pGstPT->a[i].u & X86_PTE_PAE_PG_MASK, &HCPhys);
                AssertMsg(rc == VINF_SUCCESS && PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]) == HCPhys, ("rc=%d guest %RX64 old %RX64 shw=%RX64 vs %RHp\n", rc, pGstPT->a[i].u, pOldGstPT->a[i].u, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]), HCPhys));
#endif
                uint64_t uHostAttr  = PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & (X86_PTE_P | X86_PTE_US | X86_PTE_A | X86_PTE_D | X86_PTE_G | X86_PTE_PAE_NX);
                bool     fHostRW    = !!(PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & X86_PTE_RW);
                uint64_t uGuestAttr = pGstPT->a[i].u & (X86_PTE_P | X86_PTE_US | X86_PTE_A | X86_PTE_D | X86_PTE_G | X86_PTE_PAE_NX);
                bool     fGuestRW   = !!(pGstPT->a[i].u & X86_PTE_RW);

                if (    uHostAttr == uGuestAttr
                    &&  fHostRW <= fGuestRW)
                    continue;
            }
            cChanged++;
            /* Something was changed, so flush it. */
            Log4(("pgmPoolTrackDerefPTPaePae: i=%d pte=%RX64 hint=%RX64\n",
                  i, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pOldGstPT->a[i].u & X86_PTE_PAE_PG_MASK));
            pgmPoolTracDerefGCPhysHint(pPool, pPage, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pOldGstPT->a[i].u & X86_PTE_PAE_PG_MASK, i);
            PGMSHWPTEPAE_ATOMIC_SET(pShwPT->a[i], 0);
        }
    }
    return cChanged;
}


/**
 * Clear references to guest physical memory in a PAE / PAE page table.
 *
 * @returns nr of changed PTEs
 * @param   pPool           The pool.
 * @param   pPage           The page.
 * @param   pShwPT          The shadow page table (mapping of the page).
 * @param   pGstPT          The guest page table.
 * @param   pOldGstPT       The old cached guest page table.
 * @param   fAllowRemoval   Bail out as soon as we encounter an invalid PTE
 * @param   pfFlush         Flush reused page table (out)
 */
DECLINLINE(unsigned) pgmPoolTrackFlushPTPae32Bit(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT, PCX86PT pGstPT,
                                                 PCX86PT pOldGstPT, bool fAllowRemoval, bool *pfFlush)
{
    unsigned cChanged = 0;

#ifdef VBOX_STRICT
    for (unsigned i = 0; i < RT_MIN(RT_ELEMENTS(pShwPT->a), pPage->iFirstPresent); i++)
        AssertMsg(!PGMSHWPTEPAE_IS_P(pShwPT->a[i]), ("Unexpected PTE: idx=%d %RX64 (first=%d)\n", i, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]),  pPage->iFirstPresent));
#endif
    *pfFlush = false;

    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++)
    {
        /* Check the new value written by the guest. If present and with a bogus physical address, then
         * it's fairly safe to assume the guest is reusing the PT.
         */
        if (    fAllowRemoval
            &&  pGstPT->a[i].n.u1Present)
        {
            if (!PGMPhysIsGCPhysValid(pPool->CTX_SUFF(pVM), pGstPT->a[i].u & X86_PTE_PG_MASK))
            {
                *pfFlush = true;
                return ++cChanged;
            }
        }
        if (PGMSHWPTEPAE_IS_P(pShwPT->a[i]))
        {
            /* If the old cached PTE is identical, then there's no need to flush the shadow copy. */
            if ((pGstPT->a[i].u & X86_PTE_PG_MASK) == (pOldGstPT->a[i].u & X86_PTE_PG_MASK))
            {
#ifdef VBOX_STRICT
                RTHCPHYS HCPhys = NIL_RTGCPHYS;
                int rc = PGMPhysGCPhys2HCPhys(pPool->CTX_SUFF(pVM), pGstPT->a[i].u & X86_PTE_PG_MASK, &HCPhys);
                AssertMsg(rc == VINF_SUCCESS && PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]) == HCPhys, ("rc=%d guest %x old %x shw=%RX64 vs %RHp\n", rc, pGstPT->a[i].u, pOldGstPT->a[i].u, PGMSHWPTEPAE_GET_LOG(pShwPT->a[i]), HCPhys));
#endif
                uint64_t uHostAttr  = PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & (X86_PTE_P | X86_PTE_US | X86_PTE_A | X86_PTE_D | X86_PTE_G);
                bool     fHostRW    = !!(PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & X86_PTE_RW);
                uint64_t uGuestAttr = pGstPT->a[i].u & (X86_PTE_P | X86_PTE_US | X86_PTE_A | X86_PTE_D | X86_PTE_G);
                bool     fGuestRW   = !!(pGstPT->a[i].u & X86_PTE_RW);

                if (    uHostAttr == uGuestAttr
                    &&  fHostRW <= fGuestRW)
                    continue;
            }
            cChanged++;
            /* Something was changed, so flush it. */
            Log4(("pgmPoolTrackDerefPTPaePae: i=%d pte=%RX64 hint=%x\n",
                  i, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pOldGstPT->a[i].u & X86_PTE_PG_MASK));
            pgmPoolTracDerefGCPhysHint(pPool, pPage, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pOldGstPT->a[i].u & X86_PTE_PG_MASK, i);
            PGMSHWPTEPAE_ATOMIC_SET(pShwPT->a[i], 0);
        }
    }
    return cChanged;
}


/**
 * Flush a dirty page
 *
 * @param   pVM             The cross context VM structure.
 * @param   pPool           The pool.
 * @param   idxSlot         Dirty array slot index
 * @param   fAllowRemoval   Allow a reused page table to be removed
 */
static void pgmPoolFlushDirtyPage(PVM pVM, PPGMPOOL pPool, unsigned idxSlot, bool fAllowRemoval = false)
{
    PPGMPOOLPAGE pPage;
    unsigned     idxPage;

    Assert(idxSlot < RT_ELEMENTS(pPool->aDirtyPages));
    if (pPool->aDirtyPages[idxSlot].uIdx == NIL_PGMPOOL_IDX)
        return;

    idxPage = pPool->aDirtyPages[idxSlot].uIdx;
    AssertRelease(idxPage != NIL_PGMPOOL_IDX);
    pPage = &pPool->aPages[idxPage];
    Assert(pPage->idx == idxPage);
    Assert(pPage->iMonitoredNext == NIL_PGMPOOL_IDX && pPage->iMonitoredPrev == NIL_PGMPOOL_IDX);

    AssertMsg(pPage->fDirty, ("Page %RGp (slot=%d) not marked dirty!", pPage->GCPhys, idxSlot));
    Log(("Flush dirty page %RGp cMods=%d\n", pPage->GCPhys, pPage->cModifications));

#if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
    PVMCPU   pVCpu = VMMGetCpu(pVM);
    uint32_t iPrevSubset = PGMRZDynMapPushAutoSubset(pVCpu);
#endif

    /* First write protect the page again to catch all write accesses. (before checking for changes -> SMP) */
    int rc = PGMHandlerPhysicalReset(pVM, pPage->GCPhys & PAGE_BASE_GC_MASK);
    Assert(rc == VINF_SUCCESS);
    pPage->fDirty = false;

#ifdef VBOX_STRICT
    uint64_t fFlags = 0;
    RTHCPHYS HCPhys;
    rc = PGMShwGetPage(VMMGetCpu(pVM), pPage->GCPtrDirtyFault, &fFlags, &HCPhys);
    AssertMsg(      (   rc == VINF_SUCCESS
                     && (!(fFlags & X86_PTE_RW) || HCPhys != pPage->Core.Key))
              /* In the SMP case the page table might be removed while we wait for the PGM lock in the trap handler. */
              ||    rc == VERR_PAGE_TABLE_NOT_PRESENT
              ||    rc == VERR_PAGE_NOT_PRESENT,
              ("PGMShwGetPage -> GCPtr=%RGv rc=%d flags=%RX64\n", pPage->GCPtrDirtyFault, rc, fFlags));
#endif

    /* Flush those PTEs that have changed. */
    STAM_PROFILE_START(&pPool->StatTrackDeref,a);
    void *pvShw = PGMPOOL_PAGE_2_PTR(pVM, pPage);
    void *pvGst;
    rc = PGM_GCPHYS_2_PTR_EX(pVM, pPage->GCPhys, &pvGst); AssertReleaseRC(rc);
    bool  fFlush;
    unsigned cChanges;

    if (pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT)
        cChanges = pgmPoolTrackFlushPTPaePae(pPool, pPage, (PPGMSHWPTPAE)pvShw, (PCX86PTPAE)pvGst,
                                             (PCX86PTPAE)&pPool->aDirtyPages[idxSlot].aPage[0], fAllowRemoval, &fFlush);
    else
        cChanges = pgmPoolTrackFlushPTPae32Bit(pPool, pPage, (PPGMSHWPTPAE)pvShw, (PCX86PT)pvGst,
                                               (PCX86PT)&pPool->aDirtyPages[idxSlot].aPage[0], fAllowRemoval, &fFlush);

    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvGst);
    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvShw);
    STAM_PROFILE_STOP(&pPool->StatTrackDeref,a);
    /* Note: we might want to consider keeping the dirty page active in case there were many changes. */

    /* This page is likely to be modified again, so reduce the nr of modifications just a bit here. */
    Assert(pPage->cModifications);
    if (cChanges < 4)
        pPage->cModifications = 1;      /* must use > 0 here */
    else
        pPage->cModifications = RT_MAX(1, pPage->cModifications / 2);

    STAM_COUNTER_INC(&pPool->StatResetDirtyPages);
    if (pPool->cDirtyPages == RT_ELEMENTS(pPool->aDirtyPages))
        pPool->idxFreeDirtyPage = idxSlot;

    pPool->cDirtyPages--;
    pPool->aDirtyPages[idxSlot].uIdx = NIL_PGMPOOL_IDX;
    Assert(pPool->cDirtyPages <= RT_ELEMENTS(pPool->aDirtyPages));
    if (fFlush)
    {
        Assert(fAllowRemoval);
        Log(("Flush reused page table!\n"));
        pgmPoolFlushPage(pPool, pPage);
        STAM_COUNTER_INC(&pPool->StatForceFlushReused);
    }
    else
        Log(("Removed dirty page %RGp cMods=%d cChanges=%d\n", pPage->GCPhys, pPage->cModifications, cChanges));

#if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
    PGMRZDynMapPopAutoSubset(pVCpu, iPrevSubset);
#endif
}


# ifndef IN_RING3
/**
 * Add a new dirty page
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPool       The pool.
 * @param   pPage       The page.
 */
void pgmPoolAddDirtyPage(PVM pVM, PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    unsigned idxFree;

    PGM_LOCK_ASSERT_OWNER(pVM);
    AssertCompile(RT_ELEMENTS(pPool->aDirtyPages) == 8 || RT_ELEMENTS(pPool->aDirtyPages) == 16);
    Assert(!pPage->fDirty);

    idxFree = pPool->idxFreeDirtyPage;
    Assert(idxFree < RT_ELEMENTS(pPool->aDirtyPages));
    Assert(pPage->iMonitoredNext == NIL_PGMPOOL_IDX && pPage->iMonitoredPrev == NIL_PGMPOOL_IDX);

    if (pPool->cDirtyPages >= RT_ELEMENTS(pPool->aDirtyPages))
    {
        STAM_COUNTER_INC(&pPool->StatDirtyPageOverFlowFlush);
        pgmPoolFlushDirtyPage(pVM, pPool, idxFree, true /* allow removal of reused page tables*/);
    }
    Assert(pPool->cDirtyPages < RT_ELEMENTS(pPool->aDirtyPages));
    AssertMsg(pPool->aDirtyPages[idxFree].uIdx == NIL_PGMPOOL_IDX, ("idxFree=%d cDirtyPages=%d\n", idxFree, pPool->cDirtyPages));

    Log(("Add dirty page %RGp (slot=%d)\n", pPage->GCPhys, idxFree));

    /*
     * Make a copy of the guest page table as we require valid GCPhys addresses
     * when removing references to physical pages.
     * (The HCPhys linear lookup is *extremely* expensive!)
     */
    void *pvGst;
    int   rc  = PGM_GCPHYS_2_PTR_EX(pVM, pPage->GCPhys, &pvGst); AssertReleaseRC(rc);
    memcpy(&pPool->aDirtyPages[idxFree].aPage[0], pvGst, (pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT) ? PAGE_SIZE : PAGE_SIZE/2);
#  ifdef VBOX_STRICT
    void *pvShw = PGMPOOL_PAGE_2_PTR(pVM, pPage);
    if (pPage->enmKind == PGMPOOLKIND_PAE_PT_FOR_PAE_PT)
        pgmPoolTrackCheckPTPaePae(pPool, pPage, (PPGMSHWPTPAE)pvShw, (PCX86PTPAE)pvGst);
    else
        pgmPoolTrackCheckPTPae32Bit(pPool, pPage, (PPGMSHWPTPAE)pvShw, (PCX86PT)pvGst);
    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvShw);
#  endif
    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvGst);

    STAM_COUNTER_INC(&pPool->StatDirtyPage);
    pPage->fDirty                    = true;
    pPage->idxDirtyEntry             = (uint8_t)idxFree; Assert(pPage->idxDirtyEntry == idxFree);
    pPool->aDirtyPages[idxFree].uIdx = pPage->idx;
    pPool->cDirtyPages++;

    pPool->idxFreeDirtyPage        = (pPool->idxFreeDirtyPage + 1) & (RT_ELEMENTS(pPool->aDirtyPages) - 1);
    if (    pPool->cDirtyPages < RT_ELEMENTS(pPool->aDirtyPages)
        &&  pPool->aDirtyPages[pPool->idxFreeDirtyPage].uIdx != NIL_PGMPOOL_IDX)
    {
        unsigned i;
        for (i = 1; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
        {
            idxFree = (pPool->idxFreeDirtyPage + i) & (RT_ELEMENTS(pPool->aDirtyPages) - 1);
            if (pPool->aDirtyPages[idxFree].uIdx == NIL_PGMPOOL_IDX)
            {
                pPool->idxFreeDirtyPage = idxFree;
                break;
            }
        }
        Assert(i != RT_ELEMENTS(pPool->aDirtyPages));
    }

    Assert(pPool->cDirtyPages == RT_ELEMENTS(pPool->aDirtyPages) || pPool->aDirtyPages[pPool->idxFreeDirtyPage].uIdx == NIL_PGMPOOL_IDX);

    /*
     * Clear all references to this shadow table. See @bugref{7298}.
     */
    pgmPoolTrackClearPageUsers(pPool, pPage);
}
# endif /* !IN_RING3 */


/**
 * Check if the specified page is dirty (not write monitored)
 *
 * @return dirty or not
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          Guest physical address
 */
bool pgmPoolIsDirtyPage(PVM pVM, RTGCPHYS GCPhys)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    PGM_LOCK_ASSERT_OWNER(pVM);
    if (!pPool->cDirtyPages)
        return false;

    GCPhys = GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK;

    for (unsigned i = 0; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
    {
        if (pPool->aDirtyPages[i].uIdx != NIL_PGMPOOL_IDX)
        {
            PPGMPOOLPAGE pPage;
            unsigned     idxPage = pPool->aDirtyPages[i].uIdx;

            pPage = &pPool->aPages[idxPage];
            if (pPage->GCPhys == GCPhys)
                return true;
        }
    }
    return false;
}


/**
 * Reset all dirty pages by reinstating page monitoring.
 *
 * @param   pVM             The cross context VM structure.
 */
void pgmPoolResetDirtyPages(PVM pVM)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    PGM_LOCK_ASSERT_OWNER(pVM);
    Assert(pPool->cDirtyPages <= RT_ELEMENTS(pPool->aDirtyPages));

    if (!pPool->cDirtyPages)
        return;

    Log(("pgmPoolResetDirtyPages\n"));
    for (unsigned i = 0; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
        pgmPoolFlushDirtyPage(pVM, pPool, i, true /* allow removal of reused page tables*/);

    pPool->idxFreeDirtyPage = 0;
    if (    pPool->cDirtyPages != RT_ELEMENTS(pPool->aDirtyPages)
        &&  pPool->aDirtyPages[pPool->idxFreeDirtyPage].uIdx != NIL_PGMPOOL_IDX)
    {
        unsigned i;
        for (i = 1; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
        {
            if (pPool->aDirtyPages[i].uIdx == NIL_PGMPOOL_IDX)
            {
                pPool->idxFreeDirtyPage = i;
                break;
            }
        }
        AssertMsg(i != RT_ELEMENTS(pPool->aDirtyPages), ("cDirtyPages %d", pPool->cDirtyPages));
    }

    Assert(pPool->aDirtyPages[pPool->idxFreeDirtyPage].uIdx == NIL_PGMPOOL_IDX || pPool->cDirtyPages == RT_ELEMENTS(pPool->aDirtyPages));
    return;
}


/**
 * Invalidate the PT entry for the specified page
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPtrPage       Guest page to invalidate
 */
void pgmPoolResetDirtyPage(PVM pVM, RTGCPTR GCPtrPage)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    PGM_LOCK_ASSERT_OWNER(pVM);
    Assert(pPool->cDirtyPages <= RT_ELEMENTS(pPool->aDirtyPages));

    if (!pPool->cDirtyPages)
        return;

    Log(("pgmPoolResetDirtyPage %RGv\n", GCPtrPage)); RT_NOREF_PV(GCPtrPage);
    for (unsigned i = 0; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
    {
    }
}


/**
 * Reset all dirty pages by reinstating page monitoring.
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhysPT        Physical address of the page table
 */
void pgmPoolInvalidateDirtyPage(PVM pVM, RTGCPHYS GCPhysPT)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    PGM_LOCK_ASSERT_OWNER(pVM);
    Assert(pPool->cDirtyPages <= RT_ELEMENTS(pPool->aDirtyPages));
    unsigned idxDirtyPage = RT_ELEMENTS(pPool->aDirtyPages);

    if (!pPool->cDirtyPages)
        return;

    GCPhysPT = GCPhysPT & ~(RTGCPHYS)PAGE_OFFSET_MASK;

    for (unsigned i = 0; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
    {
        if (pPool->aDirtyPages[i].uIdx != NIL_PGMPOOL_IDX)
        {
            unsigned     idxPage = pPool->aDirtyPages[i].uIdx;

            PPGMPOOLPAGE pPage = &pPool->aPages[idxPage];
            if (pPage->GCPhys == GCPhysPT)
            {
                idxDirtyPage = i;
                break;
            }
        }
    }

    if (idxDirtyPage != RT_ELEMENTS(pPool->aDirtyPages))
    {
        pgmPoolFlushDirtyPage(pVM, pPool, idxDirtyPage, true /* allow removal of reused page tables*/);
        if (    pPool->cDirtyPages != RT_ELEMENTS(pPool->aDirtyPages)
            &&  pPool->aDirtyPages[pPool->idxFreeDirtyPage].uIdx != NIL_PGMPOOL_IDX)
        {
            unsigned i;
            for (i = 0; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
            {
                if (pPool->aDirtyPages[i].uIdx == NIL_PGMPOOL_IDX)
                {
                    pPool->idxFreeDirtyPage = i;
                    break;
                }
            }
            AssertMsg(i != RT_ELEMENTS(pPool->aDirtyPages), ("cDirtyPages %d", pPool->cDirtyPages));
        }
    }
}

# endif /* PGMPOOL_WITH_OPTIMIZED_DIRTY_PT */

/**
 * Inserts a page into the GCPhys hash table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 */
DECLINLINE(void) pgmPoolHashInsert(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    Log3(("pgmPoolHashInsert: %RGp\n", pPage->GCPhys));
    Assert(pPage->GCPhys != NIL_RTGCPHYS); Assert(pPage->iNext == NIL_PGMPOOL_IDX);
    uint16_t iHash = PGMPOOL_HASH(pPage->GCPhys);
    pPage->iNext = pPool->aiHash[iHash];
    pPool->aiHash[iHash] = pPage->idx;
}


/**
 * Removes a page from the GCPhys hash table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 */
DECLINLINE(void) pgmPoolHashRemove(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    Log3(("pgmPoolHashRemove: %RGp\n", pPage->GCPhys));
    uint16_t iHash = PGMPOOL_HASH(pPage->GCPhys);
    if (pPool->aiHash[iHash] == pPage->idx)
        pPool->aiHash[iHash] = pPage->iNext;
    else
    {
        uint16_t iPrev = pPool->aiHash[iHash];
        for (;;)
        {
            const int16_t i = pPool->aPages[iPrev].iNext;
            if (i == pPage->idx)
            {
                pPool->aPages[iPrev].iNext = pPage->iNext;
                break;
            }
            if (i == NIL_PGMPOOL_IDX)
            {
                AssertReleaseMsgFailed(("GCPhys=%RGp idx=%d\n", pPage->GCPhys, pPage->idx));
                break;
            }
            iPrev = i;
        }
    }
    pPage->iNext = NIL_PGMPOOL_IDX;
}


/**
 * Frees up one cache page.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @param   pPool       The pool.
 * @param   iUser       The user index.
 */
static int pgmPoolCacheFreeOne(PPGMPOOL pPool, uint16_t iUser)
{
#ifndef IN_RC
    const PVM pVM = pPool->CTX_SUFF(pVM);
#endif
    Assert(pPool->iAgeHead != pPool->iAgeTail); /* We shouldn't be here if there < 2 cached entries! */
    STAM_COUNTER_INC(&pPool->StatCacheFreeUpOne);

    /*
     * Select one page from the tail of the age list.
     */
    PPGMPOOLPAGE    pPage;
    for (unsigned iLoop = 0; ; iLoop++)
    {
        uint16_t iToFree = pPool->iAgeTail;
        if (iToFree == iUser && iUser != NIL_PGMPOOL_IDX)
            iToFree = pPool->aPages[iToFree].iAgePrev;
/* This is the alternative to the SyncCR3 pgmPoolCacheUsed calls.
        if (pPool->aPages[iToFree].iUserHead != NIL_PGMPOOL_USER_INDEX)
        {
            uint16_t i = pPool->aPages[iToFree].iAgePrev;
            for (unsigned j = 0; j < 10 && i != NIL_PGMPOOL_USER_INDEX; j++, i = pPool->aPages[i].iAgePrev)
            {
                if (pPool->aPages[iToFree].iUserHead == NIL_PGMPOOL_USER_INDEX)
                    continue;
                iToFree = i;
                break;
            }
        }
*/
        Assert(iToFree != iUser);
        AssertRelease(iToFree != NIL_PGMPOOL_IDX);
        pPage = &pPool->aPages[iToFree];

        /*
         * Reject any attempts at flushing the currently active shadow CR3 mapping.
         * Call pgmPoolCacheUsed to move the page to the head of the age list.
         */
        if (   !pgmPoolIsPageLocked(pPage)
            && pPage->idx >= PGMPOOL_IDX_FIRST /* paranoia (#6349) */)
            break;
        LogFlow(("pgmPoolCacheFreeOne: refuse CR3 mapping\n"));
        pgmPoolCacheUsed(pPool, pPage);
        AssertLogRelReturn(iLoop < 8192, VERR_PGM_POOL_TOO_MANY_LOOPS);
    }

    /*
     * Found a usable page, flush it and return.
     */
    int rc = pgmPoolFlushPage(pPool, pPage);
    /* This flush was initiated by us and not the guest, so explicitly flush the TLB. */
    /** @todo find out why this is necessary; pgmPoolFlushPage should trigger a flush if one is really needed. */
    if (rc == VINF_SUCCESS)
        PGM_INVL_ALL_VCPU_TLBS(pVM);
    return rc;
}


/**
 * Checks if a kind mismatch is really a page being reused
 * or if it's just normal remappings.
 *
 * @returns true if reused and the cached page (enmKind1) should be flushed
 * @returns false if not reused.
 * @param   enmKind1    The kind of the cached page.
 * @param   enmKind2    The kind of the requested page.
 */
static bool pgmPoolCacheReusedByKind(PGMPOOLKIND enmKind1, PGMPOOLKIND enmKind2)
{
    switch (enmKind1)
    {
        /*
         * Never reuse them. There is no remapping in non-paging mode.
         */
        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
        case PGMPOOLKIND_32BIT_PD_PHYS:
        case PGMPOOLKIND_PAE_PT_FOR_PHYS:
        case PGMPOOLKIND_PAE_PD_PHYS:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_PAE_PDPT_FOR_32BIT: /* never reuse them for other types */
            return false;

        /*
         * It's perfectly fine to reuse these, except for PAE and non-paging stuff.
         */
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_PAE_PDPT:
            switch (enmKind2)
            {
                case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
                case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
                case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
                case PGMPOOLKIND_64BIT_PML4:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
                case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
                case PGMPOOLKIND_PAE_PT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_EPT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PT_FOR_PHYS:
                    return true;
                default:
                    return false;
            }

        /*
         * It's perfectly fine to reuse these, except for PAE and non-paging stuff.
         */
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
        case PGMPOOLKIND_64BIT_PML4:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
            switch (enmKind2)
            {
                case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
                case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
                case PGMPOOLKIND_PAE_PT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_EPT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PT_FOR_PHYS:
                    return true;
                default:
                    return false;
            }

        /*
         * These cannot be flushed, and it's common to reuse the PDs as PTs.
         */
        case PGMPOOLKIND_ROOT_NESTED:
            return false;

        default:
            AssertFatalMsgFailed(("enmKind1=%d\n", enmKind1));
    }
}


/**
 * Attempts to satisfy a pgmPoolAlloc request from the cache.
 *
 * @returns VBox status code.
 * @retval  VINF_PGM_CACHED_PAGE on success.
 * @retval  VERR_FILE_NOT_FOUND if not found.
 * @param   pPool       The pool.
 * @param   GCPhys      The GC physical address of the page we're gonna shadow.
 * @param   enmKind     The kind of mapping.
 * @param   enmAccess   Access type for the mapping (only relevant for big pages)
 * @param   fA20Enabled Whether the CPU has the A20 gate enabled.
 * @param   iUser       The shadow page pool index of the user table.  This is
 *                      NIL_PGMPOOL_IDX for root pages.
 * @param   iUserTable  The index into the user table (shadowed).  Ignored if
 *                      root page
 * @param   ppPage      Where to store the pointer to the page.
 */
static int pgmPoolCacheAlloc(PPGMPOOL pPool, RTGCPHYS GCPhys, PGMPOOLKIND enmKind, PGMPOOLACCESS enmAccess, bool fA20Enabled,
                             uint16_t iUser, uint32_t iUserTable, PPPGMPOOLPAGE ppPage)
{
    /*
     * Look up the GCPhys in the hash.
     */
    unsigned i = pPool->aiHash[PGMPOOL_HASH(GCPhys)];
    Log3(("pgmPoolCacheAlloc: %RGp kind %s iUser=%d iUserTable=%x SLOT=%d\n", GCPhys, pgmPoolPoolKindToStr(enmKind), iUser, iUserTable, i));
    if (i != NIL_PGMPOOL_IDX)
    {
        do
        {
            PPGMPOOLPAGE pPage = &pPool->aPages[i];
            Log4(("pgmPoolCacheAlloc: slot %d found page %RGp\n", i, pPage->GCPhys));
            if (pPage->GCPhys == GCPhys)
            {
                if (   (PGMPOOLKIND)pPage->enmKind == enmKind
                    && (PGMPOOLACCESS)pPage->enmAccess == enmAccess
                    && pPage->fA20Enabled == fA20Enabled)
                {
                    /* Put it at the start of the use list to make sure pgmPoolTrackAddUser
                     * doesn't flush it in case there are no more free use records.
                     */
                    pgmPoolCacheUsed(pPool, pPage);

                    int rc = VINF_SUCCESS;
                    if (iUser != NIL_PGMPOOL_IDX)
                        rc = pgmPoolTrackAddUser(pPool, pPage, iUser, iUserTable);
                    if (RT_SUCCESS(rc))
                    {
                        Assert((PGMPOOLKIND)pPage->enmKind == enmKind);
                        *ppPage = pPage;
                        if (pPage->cModifications)
                            pPage->cModifications = 1; /* reset counter (can't use 0, or else it will be reinserted in the modified list) */
                        STAM_COUNTER_INC(&pPool->StatCacheHits);
                        return VINF_PGM_CACHED_PAGE;
                    }
                    return rc;
                }

                if ((PGMPOOLKIND)pPage->enmKind != enmKind)
                {
                    /*
                     * The kind is different. In some cases we should now flush the page
                     * as it has been reused, but in most cases this is normal remapping
                     * of PDs as PT or big pages using the GCPhys field in a slightly
                     * different way than the other kinds.
                     */
                    if (pgmPoolCacheReusedByKind((PGMPOOLKIND)pPage->enmKind, enmKind))
                    {
                        STAM_COUNTER_INC(&pPool->StatCacheKindMismatches);
                        pgmPoolFlushPage(pPool, pPage);
                        break;
                    }
                }
            }

            /* next */
            i = pPage->iNext;
        } while (i != NIL_PGMPOOL_IDX);
    }

    Log3(("pgmPoolCacheAlloc: Missed GCPhys=%RGp enmKind=%s\n", GCPhys, pgmPoolPoolKindToStr(enmKind)));
    STAM_COUNTER_INC(&pPool->StatCacheMisses);
    return VERR_FILE_NOT_FOUND;
}


/**
 * Inserts a page into the cache.
 *
 * @param   pPool           The pool.
 * @param   pPage           The cached page.
 * @param   fCanBeCached    Set if the page is fit for caching from the caller's point of view.
 */
static void pgmPoolCacheInsert(PPGMPOOL pPool, PPGMPOOLPAGE pPage, bool fCanBeCached)
{
    /*
     * Insert into the GCPhys hash if the page is fit for that.
     */
    Assert(!pPage->fCached);
    if (fCanBeCached)
    {
        pPage->fCached = true;
        pgmPoolHashInsert(pPool, pPage);
        Log3(("pgmPoolCacheInsert: Caching %p:{.Core=%RHp, .idx=%d, .enmKind=%s, GCPhys=%RGp}\n",
              pPage, pPage->Core.Key, pPage->idx, pgmPoolPoolKindToStr(pPage->enmKind), pPage->GCPhys));
        STAM_COUNTER_INC(&pPool->StatCacheCacheable);
    }
    else
    {
        Log3(("pgmPoolCacheInsert: Not caching %p:{.Core=%RHp, .idx=%d, .enmKind=%s, GCPhys=%RGp}\n",
              pPage, pPage->Core.Key, pPage->idx, pgmPoolPoolKindToStr(pPage->enmKind), pPage->GCPhys));
        STAM_COUNTER_INC(&pPool->StatCacheUncacheable);
    }

    /*
     * Insert at the head of the age list.
     */
    pPage->iAgePrev = NIL_PGMPOOL_IDX;
    pPage->iAgeNext = pPool->iAgeHead;
    if (pPool->iAgeHead != NIL_PGMPOOL_IDX)
        pPool->aPages[pPool->iAgeHead].iAgePrev = pPage->idx;
    else
        pPool->iAgeTail = pPage->idx;
    pPool->iAgeHead = pPage->idx;
}


/**
 * Flushes a cached page.
 *
 * @param   pPool       The pool.
 * @param   pPage       The cached page.
 */
static void pgmPoolCacheFlushPage(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    Log3(("pgmPoolCacheFlushPage: %RGp\n", pPage->GCPhys));

    /*
     * Remove the page from the hash.
     */
    if (pPage->fCached)
    {
        pPage->fCached = false;
        pgmPoolHashRemove(pPool, pPage);
    }
    else
        Assert(pPage->iNext == NIL_PGMPOOL_IDX);

    /*
     * Remove it from the age list.
     */
    if (pPage->iAgeNext != NIL_PGMPOOL_IDX)
        pPool->aPages[pPage->iAgeNext].iAgePrev = pPage->iAgePrev;
    else
        pPool->iAgeTail = pPage->iAgePrev;
    if (pPage->iAgePrev != NIL_PGMPOOL_IDX)
        pPool->aPages[pPage->iAgePrev].iAgeNext = pPage->iAgeNext;
    else
        pPool->iAgeHead = pPage->iAgeNext;
    pPage->iAgeNext = NIL_PGMPOOL_IDX;
    pPage->iAgePrev = NIL_PGMPOOL_IDX;
}


/**
 * Looks for pages sharing the monitor.
 *
 * @returns Pointer to the head page.
 * @returns NULL if not found.
 * @param   pPool       The Pool
 * @param   pNewPage    The page which is going to be monitored.
 */
static PPGMPOOLPAGE pgmPoolMonitorGetPageByGCPhys(PPGMPOOL pPool, PPGMPOOLPAGE pNewPage)
{
    /*
     * Look up the GCPhys in the hash.
     */
    RTGCPHYS GCPhys = pNewPage->GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK;
    unsigned i = pPool->aiHash[PGMPOOL_HASH(GCPhys)];
    if (i == NIL_PGMPOOL_IDX)
        return NULL;
    do
    {
        PPGMPOOLPAGE pPage = &pPool->aPages[i];
        if (    pPage->GCPhys - GCPhys < PAGE_SIZE
            &&  pPage != pNewPage)
        {
            switch (pPage->enmKind)
            {
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
                case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
                case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
                case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
                case PGMPOOLKIND_64BIT_PML4:
                case PGMPOOLKIND_32BIT_PD:
                case PGMPOOLKIND_PAE_PDPT:
                {
                    /* find the head */
                    while (pPage->iMonitoredPrev != NIL_PGMPOOL_IDX)
                    {
                        Assert(pPage->iMonitoredPrev != pPage->idx);
                        pPage = &pPool->aPages[pPage->iMonitoredPrev];
                    }
                    return pPage;
                }

                /* ignore, no monitoring. */
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
                case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
                case PGMPOOLKIND_PAE_PT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_EPT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PT_FOR_PHYS:
                case PGMPOOLKIND_ROOT_NESTED:
                case PGMPOOLKIND_PAE_PD_PHYS:
                case PGMPOOLKIND_PAE_PDPT_PHYS:
                case PGMPOOLKIND_32BIT_PD_PHYS:
                case PGMPOOLKIND_PAE_PDPT_FOR_32BIT:
                    break;
                default:
                    AssertFatalMsgFailed(("enmKind=%d idx=%d\n", pPage->enmKind, pPage->idx));
            }
        }

        /* next */
        i = pPage->iNext;
    } while (i != NIL_PGMPOOL_IDX);
    return NULL;
}


/**
 * Enabled write monitoring of a guest page.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @param   pPool       The pool.
 * @param   pPage       The cached page.
 */
static int pgmPoolMonitorInsert(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    LogFlow(("pgmPoolMonitorInsert %RGp\n", pPage->GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK));

    /*
     * Filter out the relevant kinds.
     */
    switch (pPage->enmKind)
    {
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
        case PGMPOOLKIND_64BIT_PML4:
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_PAE_PDPT:
            break;

        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
        case PGMPOOLKIND_PAE_PT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PT_FOR_PHYS:
        case PGMPOOLKIND_ROOT_NESTED:
            /* Nothing to monitor here. */
            return VINF_SUCCESS;

        case PGMPOOLKIND_32BIT_PD_PHYS:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
        case PGMPOOLKIND_PAE_PD_PHYS:
        case PGMPOOLKIND_PAE_PDPT_FOR_32BIT:
            /* Nothing to monitor here. */
            return VINF_SUCCESS;
        default:
            AssertFatalMsgFailed(("This can't happen! enmKind=%d\n", pPage->enmKind));
    }

    /*
     * Install handler.
     */
    int rc;
    PPGMPOOLPAGE pPageHead = pgmPoolMonitorGetPageByGCPhys(pPool, pPage);
    if (pPageHead)
    {
        Assert(pPageHead != pPage); Assert(pPageHead->iMonitoredNext != pPage->idx);
        Assert(pPageHead->iMonitoredPrev != pPage->idx);

#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
        if (pPageHead->fDirty)
            pgmPoolFlushDirtyPage(pPool->CTX_SUFF(pVM), pPool, pPageHead->idxDirtyEntry, false /* do not remove */);
#endif

        pPage->iMonitoredPrev = pPageHead->idx;
        pPage->iMonitoredNext = pPageHead->iMonitoredNext;
        if (pPageHead->iMonitoredNext != NIL_PGMPOOL_IDX)
            pPool->aPages[pPageHead->iMonitoredNext].iMonitoredPrev = pPage->idx;
        pPageHead->iMonitoredNext = pPage->idx;
        rc = VINF_SUCCESS;
    }
    else
    {
        Assert(pPage->iMonitoredNext == NIL_PGMPOOL_IDX); Assert(pPage->iMonitoredPrev == NIL_PGMPOOL_IDX);
        PVM pVM = pPool->CTX_SUFF(pVM);
        const RTGCPHYS GCPhysPage = pPage->GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK;
        rc = PGMHandlerPhysicalRegister(pVM, GCPhysPage, GCPhysPage + PAGE_OFFSET_MASK, pPool->hAccessHandlerType,
                                        MMHyperCCToR3(pVM, pPage), MMHyperCCToR0(pVM, pPage), MMHyperCCToRC(pVM, pPage),
                                        NIL_RTR3PTR /*pszDesc*/);
        /** @todo we should probably deal with out-of-memory conditions here, but for now increasing
         * the heap size should suffice. */
        AssertFatalMsgRC(rc, ("PGMHandlerPhysicalRegisterEx %RGp failed with %Rrc\n", GCPhysPage, rc));
        PVMCPU pVCpu = VMMGetCpu(pVM);
        AssertFatalMsg(!(pVCpu->pgm.s.fSyncFlags & PGM_SYNC_CLEAR_PGM_POOL) || VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3), ("fSyncFlags=%x syncff=%d\n", pVCpu->pgm.s.fSyncFlags, VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3)));
    }
    pPage->fMonitored = true;
    return rc;
}


/**
 * Disables write monitoring of a guest page.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @param   pPool       The pool.
 * @param   pPage       The cached page.
 */
static int pgmPoolMonitorFlush(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    /*
     * Filter out the relevant kinds.
     */
    switch (pPage->enmKind)
    {
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
        case PGMPOOLKIND_64BIT_PML4:
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_PAE_PDPT:
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
            break;

        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
        case PGMPOOLKIND_PAE_PT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PT_FOR_PHYS:
        case PGMPOOLKIND_ROOT_NESTED:
        case PGMPOOLKIND_PAE_PD_PHYS:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
        case PGMPOOLKIND_32BIT_PD_PHYS:
            /* Nothing to monitor here. */
            Assert(!pPage->fMonitored);
            return VINF_SUCCESS;

        default:
            AssertFatalMsgFailed(("This can't happen! enmKind=%d\n", pPage->enmKind));
    }
    Assert(pPage->fMonitored);

    /*
     * Remove the page from the monitored list or uninstall it if last.
     */
    const PVM pVM = pPool->CTX_SUFF(pVM);
    int rc;
    if (    pPage->iMonitoredNext != NIL_PGMPOOL_IDX
        ||  pPage->iMonitoredPrev != NIL_PGMPOOL_IDX)
    {
        if (pPage->iMonitoredPrev == NIL_PGMPOOL_IDX)
        {
            PPGMPOOLPAGE pNewHead = &pPool->aPages[pPage->iMonitoredNext];
            pNewHead->iMonitoredPrev = NIL_PGMPOOL_IDX;
            rc = PGMHandlerPhysicalChangeUserArgs(pVM, pPage->GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK, MMHyperCCToR3(pVM, pNewHead),
                                                  MMHyperCCToR0(pVM, pNewHead), MMHyperCCToRC(pVM, pNewHead));

            AssertFatalRCSuccess(rc);
            pPage->iMonitoredNext = NIL_PGMPOOL_IDX;
        }
        else
        {
            pPool->aPages[pPage->iMonitoredPrev].iMonitoredNext = pPage->iMonitoredNext;
            if (pPage->iMonitoredNext != NIL_PGMPOOL_IDX)
            {
                pPool->aPages[pPage->iMonitoredNext].iMonitoredPrev = pPage->iMonitoredPrev;
                pPage->iMonitoredNext = NIL_PGMPOOL_IDX;
            }
            pPage->iMonitoredPrev = NIL_PGMPOOL_IDX;
            rc = VINF_SUCCESS;
        }
    }
    else
    {
        rc = PGMHandlerPhysicalDeregister(pVM, pPage->GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK);
        AssertFatalRC(rc);
        PVMCPU pVCpu = VMMGetCpu(pVM);
        AssertFatalMsg(!(pVCpu->pgm.s.fSyncFlags & PGM_SYNC_CLEAR_PGM_POOL) || VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3),
                  ("%#x %#x\n", pVCpu->pgm.s.fSyncFlags, pVM->fGlobalForcedActions));
    }
    pPage->fMonitored = false;

    /*
     * Remove it from the list of modified pages (if in it).
     */
    pgmPoolMonitorModifiedRemove(pPool, pPage);

    return rc;
}


/**
 * Inserts the page into the list of modified pages.
 *
 * @param   pPool   The pool.
 * @param   pPage   The page.
 */
void pgmPoolMonitorModifiedInsert(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    Log3(("pgmPoolMonitorModifiedInsert: idx=%d\n", pPage->idx));
    AssertMsg(   pPage->iModifiedNext == NIL_PGMPOOL_IDX
              && pPage->iModifiedPrev == NIL_PGMPOOL_IDX
              && pPool->iModifiedHead != pPage->idx,
              ("Next=%d Prev=%d idx=%d cModifications=%d Head=%d cModifiedPages=%d\n",
               pPage->iModifiedNext, pPage->iModifiedPrev, pPage->idx, pPage->cModifications,
               pPool->iModifiedHead, pPool->cModifiedPages));

    pPage->iModifiedNext = pPool->iModifiedHead;
    if (pPool->iModifiedHead != NIL_PGMPOOL_IDX)
        pPool->aPages[pPool->iModifiedHead].iModifiedPrev = pPage->idx;
    pPool->iModifiedHead = pPage->idx;
    pPool->cModifiedPages++;
#ifdef VBOX_WITH_STATISTICS
    if (pPool->cModifiedPages > pPool->cModifiedPagesHigh)
        pPool->cModifiedPagesHigh = pPool->cModifiedPages;
#endif
}


/**
 * Removes the page from the list of modified pages and resets the
 * modification counter.
 *
 * @param   pPool   The pool.
 * @param   pPage   The page which is believed to be in the list of modified pages.
 */
static void pgmPoolMonitorModifiedRemove(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    Log3(("pgmPoolMonitorModifiedRemove: idx=%d cModifications=%d\n", pPage->idx, pPage->cModifications));
    if (pPool->iModifiedHead == pPage->idx)
    {
        Assert(pPage->iModifiedPrev == NIL_PGMPOOL_IDX);
        pPool->iModifiedHead = pPage->iModifiedNext;
        if (pPage->iModifiedNext != NIL_PGMPOOL_IDX)
        {
            pPool->aPages[pPage->iModifiedNext].iModifiedPrev = NIL_PGMPOOL_IDX;
            pPage->iModifiedNext = NIL_PGMPOOL_IDX;
        }
        pPool->cModifiedPages--;
    }
    else if (pPage->iModifiedPrev != NIL_PGMPOOL_IDX)
    {
        pPool->aPages[pPage->iModifiedPrev].iModifiedNext = pPage->iModifiedNext;
        if (pPage->iModifiedNext != NIL_PGMPOOL_IDX)
        {
            pPool->aPages[pPage->iModifiedNext].iModifiedPrev = pPage->iModifiedPrev;
            pPage->iModifiedNext = NIL_PGMPOOL_IDX;
        }
        pPage->iModifiedPrev = NIL_PGMPOOL_IDX;
        pPool->cModifiedPages--;
    }
    else
        Assert(pPage->iModifiedPrev == NIL_PGMPOOL_IDX);
    pPage->cModifications = 0;
}


/**
 * Zaps the list of modified pages, resetting their modification counters in the process.
 *
 * @param   pVM     The cross context VM structure.
 */
static void pgmPoolMonitorModifiedClearAll(PVM pVM)
{
    pgmLock(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    LogFlow(("pgmPoolMonitorModifiedClearAll: cModifiedPages=%d\n", pPool->cModifiedPages));

    unsigned cPages = 0; NOREF(cPages);

#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    pgmPoolResetDirtyPages(pVM);
#endif

    uint16_t idx = pPool->iModifiedHead;
    pPool->iModifiedHead = NIL_PGMPOOL_IDX;
    while (idx != NIL_PGMPOOL_IDX)
    {
        PPGMPOOLPAGE pPage = &pPool->aPages[idx];
        idx = pPage->iModifiedNext;
        pPage->iModifiedNext = NIL_PGMPOOL_IDX;
        pPage->iModifiedPrev = NIL_PGMPOOL_IDX;
        pPage->cModifications = 0;
        Assert(++cPages);
    }
    AssertMsg(cPages == pPool->cModifiedPages, ("%d != %d\n", cPages, pPool->cModifiedPages));
    pPool->cModifiedPages = 0;
    pgmUnlock(pVM);
}


/**
 * Handle SyncCR3 pool tasks
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if successfully added.
 * @retval  VINF_PGM_SYNC_CR3 is it needs to be deferred to ring 3 (GC only)
 * @param   pVCpu     The cross context virtual CPU structure.
 * @remark  Should only be used when monitoring is available, thus placed in
 *          the PGMPOOL_WITH_MONITORING \#ifdef.
 */
int pgmPoolSyncCR3(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    LogFlow(("pgmPoolSyncCR3 fSyncFlags=%x\n", pVCpu->pgm.s.fSyncFlags));

    /*
     * When monitoring shadowed pages, we reset the modification counters on CR3 sync.
     * Occasionally we will have to clear all the shadow page tables because we wanted
     * to monitor a page which was mapped by too many shadowed page tables. This operation
     * sometimes referred to as a 'lightweight flush'.
     */
# ifdef IN_RING3 /* Don't flush in ring-0 or raw mode, it's taking too long. */
    if (pVCpu->pgm.s.fSyncFlags & PGM_SYNC_CLEAR_PGM_POOL)
        pgmR3PoolClearAll(pVM, false /*fFlushRemTlb*/);
# else  /* !IN_RING3 */
    if (pVCpu->pgm.s.fSyncFlags & PGM_SYNC_CLEAR_PGM_POOL)
    {
        Log(("SyncCR3: PGM_SYNC_CLEAR_PGM_POOL is set -> VINF_PGM_SYNC_CR3\n"));
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3); /** @todo no need to do global sync, right? */

        /* Make sure all other VCPUs return to ring 3. */
        if (pVM->cCpus > 1)
        {
            VM_FF_SET(pVM, VM_FF_PGM_POOL_FLUSH_PENDING);
            PGM_INVL_ALL_VCPU_TLBS(pVM);
        }
        return VINF_PGM_SYNC_CR3;
    }
# endif /* !IN_RING3 */
    else
    {
        pgmPoolMonitorModifiedClearAll(pVM);

        /* pgmPoolMonitorModifiedClearAll can cause a pgm pool flush (dirty page clearing), so make sure we handle this! */
        if (pVCpu->pgm.s.fSyncFlags & PGM_SYNC_CLEAR_PGM_POOL)
        {
            Log(("pgmPoolMonitorModifiedClearAll caused a pgm flush -> call pgmPoolSyncCR3 again!\n"));
            return pgmPoolSyncCR3(pVCpu);
        }
    }
    return VINF_SUCCESS;
}


/**
 * Frees up at least one user entry.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if successfully added.
 * @retval  VERR_PGM_POOL_FLUSHED if the pool was flushed.
 * @param   pPool       The pool.
 * @param   iUser       The user index.
 */
static int pgmPoolTrackFreeOneUser(PPGMPOOL pPool, uint16_t iUser)
{
    STAM_COUNTER_INC(&pPool->StatTrackFreeUpOneUser);
    /*
     * Just free cached pages in a braindead fashion.
     */
    /** @todo walk the age list backwards and free the first with usage. */
    int rc = VINF_SUCCESS;
    do
    {
        int rc2 = pgmPoolCacheFreeOne(pPool, iUser);
        if (RT_FAILURE(rc2) && rc == VINF_SUCCESS)
            rc = rc2;
    } while (pPool->iUserFreeHead == NIL_PGMPOOL_USER_INDEX);
    return rc;
}


/**
 * Inserts a page into the cache.
 *
 * This will create user node for the page, insert it into the GCPhys
 * hash, and insert it into the age list.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if successfully added.
 * @retval  VERR_PGM_POOL_FLUSHED if the pool was flushed.
 * @param   pPool       The pool.
 * @param   pPage       The cached page.
 * @param   GCPhys      The GC physical address of the page we're gonna shadow.
 * @param   iUser       The user index.
 * @param   iUserTable  The user table index.
 */
DECLINLINE(int) pgmPoolTrackInsert(PPGMPOOL pPool, PPGMPOOLPAGE pPage, RTGCPHYS GCPhys, uint16_t iUser, uint32_t iUserTable)
{
    int rc = VINF_SUCCESS;
    PPGMPOOLUSER paUsers = pPool->CTX_SUFF(paUsers);

    LogFlow(("pgmPoolTrackInsert GCPhys=%RGp iUser=%d iUserTable=%x\n", GCPhys, iUser, iUserTable)); RT_NOREF_PV(GCPhys);

    if (iUser != NIL_PGMPOOL_IDX)
    {
#ifdef VBOX_STRICT
        /*
         * Check that the entry doesn't already exists.
         */
        if (pPage->iUserHead != NIL_PGMPOOL_USER_INDEX)
        {
            uint16_t i = pPage->iUserHead;
            do
            {
                Assert(i < pPool->cMaxUsers);
                AssertMsg(paUsers[i].iUser != iUser || paUsers[i].iUserTable != iUserTable, ("%x %x vs new %x %x\n", paUsers[i].iUser, paUsers[i].iUserTable, iUser, iUserTable));
                i = paUsers[i].iNext;
            } while (i != NIL_PGMPOOL_USER_INDEX);
        }
#endif

        /*
         * Find free a user node.
         */
        uint16_t i = pPool->iUserFreeHead;
        if (i == NIL_PGMPOOL_USER_INDEX)
        {
            rc = pgmPoolTrackFreeOneUser(pPool, iUser);
            if (RT_FAILURE(rc))
                return rc;
            i = pPool->iUserFreeHead;
        }

        /*
         * Unlink the user node from the free list,
         * initialize and insert it into the user list.
         */
        pPool->iUserFreeHead = paUsers[i].iNext;
        paUsers[i].iNext = NIL_PGMPOOL_USER_INDEX;
        paUsers[i].iUser = iUser;
        paUsers[i].iUserTable = iUserTable;
        pPage->iUserHead = i;
    }
    else
        pPage->iUserHead = NIL_PGMPOOL_USER_INDEX;


    /*
     * Insert into cache and enable monitoring of the guest page if enabled.
     *
     * Until we implement caching of all levels, including the CR3 one, we'll
     * have to make sure we don't try monitor & cache any recursive reuse of
     * a monitored CR3 page. Because all windows versions are doing this we'll
     * have to be able to do combined access monitoring, CR3 + PT and
     * PD + PT (guest PAE).
     *
     * Update:
     * We're now cooperating with the CR3 monitor if an uncachable page is found.
     */
    const bool fCanBeMonitored = true;
    pgmPoolCacheInsert(pPool, pPage, fCanBeMonitored); /* This can be expanded. */
    if (fCanBeMonitored)
    {
        rc = pgmPoolMonitorInsert(pPool, pPage);
        AssertRC(rc);
    }
    return rc;
}


/**
 * Adds a user reference to a page.
 *
 * This will move the page to the head of the
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if successfully added.
 * @retval  VERR_PGM_POOL_FLUSHED if the pool was flushed.
 * @param   pPool       The pool.
 * @param   pPage       The cached page.
 * @param   iUser       The user index.
 * @param   iUserTable  The user table.
 */
static int pgmPoolTrackAddUser(PPGMPOOL pPool, PPGMPOOLPAGE pPage, uint16_t iUser, uint32_t iUserTable)
{
    Log3(("pgmPoolTrackAddUser: GCPhys=%RGp iUser=%x iUserTable=%x\n", pPage->GCPhys, iUser, iUserTable));
    PPGMPOOLUSER paUsers = pPool->CTX_SUFF(paUsers);
    Assert(iUser != NIL_PGMPOOL_IDX);

#  ifdef VBOX_STRICT
    /*
     * Check that the entry doesn't already exists. We only allow multiple
     * users of top-level paging structures (SHW_POOL_ROOT_IDX).
     */
    if (pPage->iUserHead != NIL_PGMPOOL_USER_INDEX)
    {
        uint16_t i = pPage->iUserHead;
        do
        {
            Assert(i < pPool->cMaxUsers);
            /** @todo this assertion looks odd... Shouldn't it be && here? */
            AssertMsg(paUsers[i].iUser != iUser || paUsers[i].iUserTable != iUserTable, ("%x %x vs new %x %x\n", paUsers[i].iUser, paUsers[i].iUserTable, iUser, iUserTable));
            i = paUsers[i].iNext;
        } while (i != NIL_PGMPOOL_USER_INDEX);
    }
#  endif

    /*
     * Allocate a user node.
     */
    uint16_t i = pPool->iUserFreeHead;
    if (i == NIL_PGMPOOL_USER_INDEX)
    {
        int rc = pgmPoolTrackFreeOneUser(pPool, iUser);
        if (RT_FAILURE(rc))
            return rc;
        i = pPool->iUserFreeHead;
    }
    pPool->iUserFreeHead = paUsers[i].iNext;

    /*
     * Initialize the user node and insert it.
     */
    paUsers[i].iNext = pPage->iUserHead;
    paUsers[i].iUser = iUser;
    paUsers[i].iUserTable = iUserTable;
    pPage->iUserHead = i;

#  ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    if (pPage->fDirty)
        pgmPoolFlushDirtyPage(pPool->CTX_SUFF(pVM), pPool, pPage->idxDirtyEntry, false /* do not remove */);
#  endif

    /*
     * Tell the cache to update its replacement stats for this page.
     */
    pgmPoolCacheUsed(pPool, pPage);
    return VINF_SUCCESS;
}


/**
 * Frees a user record associated with a page.
 *
 * This does not clear the entry in the user table, it simply replaces the
 * user record to the chain of free records.
 *
 * @param   pPool       The pool.
 * @param   pPage       The shadow page.
 * @param   iUser       The shadow page pool index of the user table.
 * @param   iUserTable  The index into the user table (shadowed).
 *
 * @remarks Don't call this for root pages.
 */
static void pgmPoolTrackFreeUser(PPGMPOOL pPool, PPGMPOOLPAGE pPage, uint16_t iUser, uint32_t iUserTable)
{
    Log3(("pgmPoolTrackFreeUser %RGp %x %x\n", pPage->GCPhys, iUser, iUserTable));
    PPGMPOOLUSER paUsers = pPool->CTX_SUFF(paUsers);
    Assert(iUser != NIL_PGMPOOL_IDX);

    /*
     * Unlink and free the specified user entry.
     */

    /* Special: For PAE and 32-bit paging, there is usually no more than one user. */
    uint16_t i = pPage->iUserHead;
    if (    i != NIL_PGMPOOL_USER_INDEX
        &&  paUsers[i].iUser == iUser
        &&  paUsers[i].iUserTable == iUserTable)
    {
        pPage->iUserHead = paUsers[i].iNext;

        paUsers[i].iUser = NIL_PGMPOOL_IDX;
        paUsers[i].iNext = pPool->iUserFreeHead;
        pPool->iUserFreeHead = i;
        return;
    }

    /* General: Linear search. */
    uint16_t iPrev = NIL_PGMPOOL_USER_INDEX;
    while (i != NIL_PGMPOOL_USER_INDEX)
    {
        if (    paUsers[i].iUser == iUser
            &&  paUsers[i].iUserTable == iUserTable)
        {
            if (iPrev != NIL_PGMPOOL_USER_INDEX)
                paUsers[iPrev].iNext = paUsers[i].iNext;
            else
                pPage->iUserHead = paUsers[i].iNext;

            paUsers[i].iUser = NIL_PGMPOOL_IDX;
            paUsers[i].iNext = pPool->iUserFreeHead;
            pPool->iUserFreeHead = i;
            return;
        }
        iPrev = i;
        i = paUsers[i].iNext;
    }

    /* Fatal: didn't find it */
    AssertFatalMsgFailed(("Didn't find the user entry! iUser=%d iUserTable=%#x GCPhys=%RGp\n",
                          iUser, iUserTable, pPage->GCPhys));
}


#if 0 /* unused */
/**
 * Gets the entry size of a shadow table.
 *
 * @param   enmKind     The kind of page.
 *
 * @returns The size of the entry in bytes. That is, 4 or 8.
 * @returns If the kind is not for a table, an assertion is raised and 0 is
 *          returned.
 */
DECLINLINE(unsigned) pgmPoolTrackGetShadowEntrySize(PGMPOOLKIND enmKind)
{
    switch (enmKind)
    {
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_32BIT_PD_PHYS:
            return 4;

        case PGMPOOLKIND_PAE_PT_FOR_PHYS:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
        case PGMPOOLKIND_64BIT_PML4:
        case PGMPOOLKIND_PAE_PDPT:
        case PGMPOOLKIND_ROOT_NESTED:
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PT_FOR_PHYS:
        case PGMPOOLKIND_PAE_PD_PHYS:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
            return 8;

        default:
            AssertFatalMsgFailed(("enmKind=%d\n", enmKind));
    }
}
#endif /* unused */

#if 0 /* unused */
/**
 * Gets the entry size of a guest table.
 *
 * @param   enmKind     The kind of page.
 *
 * @returns The size of the entry in bytes. That is, 0, 4 or 8.
 * @returns If the kind is not for a table, an assertion is raised and 0 is
 *          returned.
 */
DECLINLINE(unsigned) pgmPoolTrackGetGuestEntrySize(PGMPOOLKIND enmKind)
{
    switch (enmKind)
    {
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
            return 4;

        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
        case PGMPOOLKIND_64BIT_PML4:
        case PGMPOOLKIND_PAE_PDPT:
            return 8;

        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
        case PGMPOOLKIND_PAE_PT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
        case PGMPOOLKIND_EPT_PT_FOR_PHYS:
        case PGMPOOLKIND_ROOT_NESTED:
        case PGMPOOLKIND_PAE_PD_PHYS:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
        case PGMPOOLKIND_32BIT_PD_PHYS:
            /** @todo can we return 0? (nobody is calling this...) */
            AssertFailed();
            return 0;

        default:
            AssertFatalMsgFailed(("enmKind=%d\n", enmKind));
    }
}
#endif /* unused */


/**
 * Checks one shadow page table entry for a mapping of a physical page.
 *
 * @returns true / false indicating removal of all relevant PTEs
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPhysPage   The guest page in question.
 * @param   fFlushPTEs  Flush PTEs or allow them to be updated (e.g. in case of an RW bit change)
 * @param   iShw        The shadow page table.
 * @param   iPte        Page table entry or NIL_PGMPOOL_PHYSEXT_IDX_PTE if unknown
 */
static bool pgmPoolTrackFlushGCPhysPTInt(PVM pVM, PCPGMPAGE pPhysPage, bool fFlushPTEs, uint16_t iShw, uint16_t iPte)
{
    LogFlow(("pgmPoolTrackFlushGCPhysPTInt: pPhysPage=%RHp iShw=%d iPte=%d\n", PGM_PAGE_GET_HCPHYS(pPhysPage), iShw, iPte));
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    bool     fRet  = false;

    /*
     * Assert sanity.
     */
    Assert(iPte != NIL_PGMPOOL_PHYSEXT_IDX_PTE);
    AssertFatalMsg(iShw < pPool->cCurPages && iShw != NIL_PGMPOOL_IDX, ("iShw=%d\n", iShw));
    PPGMPOOLPAGE pPage = &pPool->aPages[iShw];

    /*
     * Then, clear the actual mappings to the page in the shadow PT.
     */
    switch (pPage->enmKind)
    {
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
        {
            const uint32_t  u32 = PGM_PAGE_GET_HCPHYS(pPhysPage) | X86_PTE_P;
            PX86PT          pPT = (PX86PT)PGMPOOL_PAGE_2_PTR(pVM, pPage);
            uint32_t        u32AndMask = 0;
            uint32_t        u32OrMask  = 0;

            if (!fFlushPTEs)
            {
                switch (PGM_PAGE_GET_HNDL_PHYS_STATE(pPhysPage))
                {
                    case PGM_PAGE_HNDL_PHYS_STATE_NONE:         /** No handler installed. */
                    case PGM_PAGE_HNDL_PHYS_STATE_DISABLED:     /** Monitoring is temporarily disabled. */
                        u32OrMask = X86_PTE_RW;
                        u32AndMask = UINT32_MAX;
                        fRet = true;
                        STAM_COUNTER_INC(&pPool->StatTrackFlushEntryKeep);
                        break;

                    case PGM_PAGE_HNDL_PHYS_STATE_WRITE:        /** Write access is monitored. */
                        u32OrMask = 0;
                        u32AndMask = ~X86_PTE_RW;
                        fRet = true;
                        STAM_COUNTER_INC(&pPool->StatTrackFlushEntryKeep);
                        break;
                    default:
                        /* (shouldn't be here, will assert below) */
                        STAM_COUNTER_INC(&pPool->StatTrackFlushEntry);
                        break;
                }
            }
            else
                STAM_COUNTER_INC(&pPool->StatTrackFlushEntry);

            /* Update the counter if we're removing references. */
            if (!u32AndMask)
            {
                Assert(pPage->cPresent);
                Assert(pPool->cPresent);
                pPage->cPresent--;
                pPool->cPresent--;
            }

            if ((pPT->a[iPte].u & (X86_PTE_PG_MASK | X86_PTE_P)) == u32)
            {
                X86PTE Pte;

                Log4(("pgmPoolTrackFlushGCPhysPTs: i=%d pte=%RX32\n", iPte, pPT->a[iPte]));
                Pte.u = (pPT->a[iPte].u & u32AndMask) | u32OrMask;
                if (Pte.u & PGM_PTFLAGS_TRACK_DIRTY)
                    Pte.n.u1Write = 0;    /* need to disallow writes when dirty bit tracking is still active. */

                ASMAtomicWriteU32(&pPT->a[iPte].u, Pte.u);
                PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPT);
                return fRet;
            }
#ifdef LOG_ENABLED
            Log(("iFirstPresent=%d cPresent=%d\n", pPage->iFirstPresent, pPage->cPresent));
            for (unsigned i = 0, cFound = 0; i < RT_ELEMENTS(pPT->a); i++)
                if ((pPT->a[i].u & (X86_PTE_PG_MASK | X86_PTE_P)) == u32)
                {
                    Log(("i=%d cFound=%d\n", i, ++cFound));
                }
#endif
            AssertFatalMsgFailed(("iFirstPresent=%d cPresent=%d u32=%RX32 poolkind=%x\n", pPage->iFirstPresent, pPage->cPresent, u32, pPage->enmKind));
            /*PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPT);*/
            break;
        }

        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
        case PGMPOOLKIND_PAE_PT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PT_FOR_PHYS:   /* physical mask the same as PAE; RW bit as well; be careful! */
        {
            const uint64_t  u64 = PGM_PAGE_GET_HCPHYS(pPhysPage) | X86_PTE_P;
            PPGMSHWPTPAE    pPT = (PPGMSHWPTPAE)PGMPOOL_PAGE_2_PTR(pVM, pPage);
            uint64_t        u64OrMask  = 0;
            uint64_t        u64AndMask = 0;

            if (!fFlushPTEs)
            {
                switch (PGM_PAGE_GET_HNDL_PHYS_STATE(pPhysPage))
                {
                    case PGM_PAGE_HNDL_PHYS_STATE_NONE:         /* No handler installed. */
                    case PGM_PAGE_HNDL_PHYS_STATE_DISABLED:     /* Monitoring is temporarily disabled. */
                        u64OrMask = X86_PTE_RW;
                        u64AndMask = UINT64_MAX;
                        fRet = true;
                        STAM_COUNTER_INC(&pPool->StatTrackFlushEntryKeep);
                        break;

                    case PGM_PAGE_HNDL_PHYS_STATE_WRITE:        /* Write access is monitored. */
                        u64OrMask = 0;
                        u64AndMask = ~(uint64_t)X86_PTE_RW;
                        fRet = true;
                        STAM_COUNTER_INC(&pPool->StatTrackFlushEntryKeep);
                        break;

                    default:
                        /* (shouldn't be here, will assert below) */
                        STAM_COUNTER_INC(&pPool->StatTrackFlushEntry);
                        break;
                }
            }
            else
                STAM_COUNTER_INC(&pPool->StatTrackFlushEntry);

            /* Update the counter if we're removing references. */
            if (!u64AndMask)
            {
                Assert(pPage->cPresent);
                Assert(pPool->cPresent);
                pPage->cPresent--;
                pPool->cPresent--;
            }

            if ((PGMSHWPTEPAE_GET_U(pPT->a[iPte]) & (X86_PTE_PAE_PG_MASK | X86_PTE_P | X86_PTE_PAE_MBZ_MASK_NX)) == u64)
            {
                X86PTEPAE Pte;

                Log4(("pgmPoolTrackFlushGCPhysPTs: i=%d pte=%RX64\n", iPte, PGMSHWPTEPAE_GET_LOG(pPT->a[iPte])));
                Pte.u = (PGMSHWPTEPAE_GET_U(pPT->a[iPte]) & u64AndMask) | u64OrMask;
                if (Pte.u & PGM_PTFLAGS_TRACK_DIRTY)
                    Pte.n.u1Write = 0;    /* need to disallow writes when dirty bit tracking is still active. */

                PGMSHWPTEPAE_ATOMIC_SET(pPT->a[iPte], Pte.u);
                PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPT);
                return fRet;
            }
#ifdef LOG_ENABLED
            Log(("iFirstPresent=%d cPresent=%d\n", pPage->iFirstPresent, pPage->cPresent));
            Log(("Found %RX64 expected %RX64\n", PGMSHWPTEPAE_GET_U(pPT->a[iPte]) & (X86_PTE_PAE_PG_MASK | X86_PTE_P | X86_PTE_PAE_MBZ_MASK_NX), u64));
            for (unsigned i = 0, cFound = 0; i < RT_ELEMENTS(pPT->a); i++)
                if ((PGMSHWPTEPAE_GET_U(pPT->a[i]) & (X86_PTE_PAE_PG_MASK | X86_PTE_P | X86_PTE_PAE_MBZ_MASK_NX)) == u64)
                    Log(("i=%d cFound=%d\n", i, ++cFound));
#endif
            AssertFatalMsgFailed(("iFirstPresent=%d cPresent=%d u64=%RX64 poolkind=%x iPte=%d PT=%RX64\n", pPage->iFirstPresent, pPage->cPresent, u64, pPage->enmKind, iPte, PGMSHWPTEPAE_GET_LOG(pPT->a[iPte])));
            /*PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPT);*/
            break;
        }

#ifdef PGM_WITH_LARGE_PAGES
        /* Large page case only. */
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
        {
            Assert(pVM->pgm.s.fNestedPaging);

            const uint64_t  u64 = PGM_PAGE_GET_HCPHYS(pPhysPage) | X86_PDE4M_P | X86_PDE4M_PS;
            PEPTPD          pPD = (PEPTPD)PGMPOOL_PAGE_2_PTR(pVM, pPage);

            if ((pPD->a[iPte].u & (EPT_PDE2M_PG_MASK | X86_PDE4M_P | X86_PDE4M_PS)) == u64)
            {
                Log4(("pgmPoolTrackFlushGCPhysPTs: i=%d pde=%RX64\n", iPte, pPD->a[iPte]));
                STAM_COUNTER_INC(&pPool->StatTrackFlushEntry);
                pPD->a[iPte].u = 0;
                PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPD);

                /* Update the counter as we're removing references. */
                Assert(pPage->cPresent);
                Assert(pPool->cPresent);
                pPage->cPresent--;
                pPool->cPresent--;

                return fRet;
            }
# ifdef LOG_ENABLED
            Log(("iFirstPresent=%d cPresent=%d\n", pPage->iFirstPresent, pPage->cPresent));
            for (unsigned i = 0, cFound = 0; i < RT_ELEMENTS(pPD->a); i++)
                if ((pPD->a[i].u & (EPT_PDE2M_PG_MASK | X86_PDE4M_P | X86_PDE4M_PS)) == u64)
                    Log(("i=%d cFound=%d\n", i, ++cFound));
# endif
            AssertFatalMsgFailed(("iFirstPresent=%d cPresent=%d\n", pPage->iFirstPresent, pPage->cPresent));
            /*PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPD);*/
            break;
        }

        /* AMD-V nested paging */ /** @todo merge with EPT as we only check the parts that are identical. */
        case PGMPOOLKIND_PAE_PD_PHYS:
        {
            Assert(pVM->pgm.s.fNestedPaging);

            const uint64_t  u64 = PGM_PAGE_GET_HCPHYS(pPhysPage) | X86_PDE4M_P | X86_PDE4M_PS;
            PX86PD          pPD = (PX86PD)PGMPOOL_PAGE_2_PTR(pVM, pPage);

            if ((pPD->a[iPte].u & (X86_PDE2M_PAE_PG_MASK | X86_PDE4M_P | X86_PDE4M_PS)) == u64)
            {
                Log4(("pgmPoolTrackFlushGCPhysPTs: i=%d pde=%RX64\n", iPte, pPD->a[iPte]));
                STAM_COUNTER_INC(&pPool->StatTrackFlushEntry);
                pPD->a[iPte].u = 0;
                PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPD);

                /* Update the counter as we're removing references. */
                Assert(pPage->cPresent);
                Assert(pPool->cPresent);
                pPage->cPresent--;
                pPool->cPresent--;
                return fRet;
            }
# ifdef LOG_ENABLED
            Log(("iFirstPresent=%d cPresent=%d\n", pPage->iFirstPresent, pPage->cPresent));
            for (unsigned i = 0, cFound = 0; i < RT_ELEMENTS(pPD->a); i++)
                if ((pPD->a[i].u & (X86_PDE2M_PAE_PG_MASK | X86_PDE4M_P | X86_PDE4M_PS)) == u64)
                    Log(("i=%d cFound=%d\n", i, ++cFound));
# endif
            AssertFatalMsgFailed(("iFirstPresent=%d cPresent=%d\n", pPage->iFirstPresent, pPage->cPresent));
            /*PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPD);*/
            break;
        }
#endif /* PGM_WITH_LARGE_PAGES */

        default:
            AssertFatalMsgFailed(("enmKind=%d iShw=%d\n", pPage->enmKind, iShw));
    }

    /* not reached. */
#ifndef _MSC_VER
    return fRet;
#endif
}


/**
 * Scans one shadow page table for mappings of a physical page.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPhysPage   The guest page in question.
 * @param   fFlushPTEs  Flush PTEs or allow them to be updated (e.g. in case of an RW bit change)
 * @param   iShw        The shadow page table.
 */
static void pgmPoolTrackFlushGCPhysPT(PVM pVM, PPGMPAGE pPhysPage, bool fFlushPTEs, uint16_t iShw)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool); NOREF(pPool);

    /* We should only come here with when there's only one reference to this physical page. */
    Assert(PGMPOOL_TD_GET_CREFS(PGM_PAGE_GET_TRACKING(pPhysPage)) == 1);

    Log2(("pgmPoolTrackFlushGCPhysPT: pPhysPage=%RHp iShw=%d\n", PGM_PAGE_GET_HCPHYS(pPhysPage), iShw));
    STAM_PROFILE_START(&pPool->StatTrackFlushGCPhysPT, f);
    bool fKeptPTEs = pgmPoolTrackFlushGCPhysPTInt(pVM, pPhysPage, fFlushPTEs, iShw, PGM_PAGE_GET_PTE_INDEX(pPhysPage));
    if (!fKeptPTEs)
        PGM_PAGE_SET_TRACKING(pVM, pPhysPage, 0);
    STAM_PROFILE_STOP(&pPool->StatTrackFlushGCPhysPT, f);
}


/**
 * Flushes a list of shadow page tables mapping the same physical page.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPhysPage   The guest page in question.
 * @param   fFlushPTEs  Flush PTEs or allow them to be updated (e.g. in case of an RW bit change)
 * @param   iPhysExt    The physical cross reference extent list to flush.
 */
static void pgmPoolTrackFlushGCPhysPTs(PVM pVM, PPGMPAGE pPhysPage, bool fFlushPTEs, uint16_t iPhysExt)
{
    PGM_LOCK_ASSERT_OWNER(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    bool     fKeepList = false;

    STAM_PROFILE_START(&pPool->StatTrackFlushGCPhysPTs, f);
    Log2(("pgmPoolTrackFlushGCPhysPTs: pPhysPage=%RHp iPhysExt=%u\n", PGM_PAGE_GET_HCPHYS(pPhysPage), iPhysExt));

    const uint16_t iPhysExtStart = iPhysExt;
    PPGMPOOLPHYSEXT pPhysExt;
    do
    {
        Assert(iPhysExt < pPool->cMaxPhysExts);
        pPhysExt = &pPool->CTX_SUFF(paPhysExts)[iPhysExt];
        for (unsigned i = 0; i < RT_ELEMENTS(pPhysExt->aidx); i++)
        {
            if (pPhysExt->aidx[i] != NIL_PGMPOOL_IDX)
            {
                bool fKeptPTEs = pgmPoolTrackFlushGCPhysPTInt(pVM, pPhysPage, fFlushPTEs, pPhysExt->aidx[i], pPhysExt->apte[i]);
                if (!fKeptPTEs)
                {
                    pPhysExt->aidx[i] = NIL_PGMPOOL_IDX;
                    pPhysExt->apte[i] = NIL_PGMPOOL_PHYSEXT_IDX_PTE;
                }
                else
                    fKeepList = true;
            }
        }
        /* next */
        iPhysExt = pPhysExt->iNext;
    } while (iPhysExt != NIL_PGMPOOL_PHYSEXT_INDEX);

    if (!fKeepList)
    {
        /* insert the list into the free list and clear the ram range entry. */
        pPhysExt->iNext = pPool->iPhysExtFreeHead;
        pPool->iPhysExtFreeHead = iPhysExtStart;
        /* Invalidate the tracking data. */
        PGM_PAGE_SET_TRACKING(pVM, pPhysPage, 0);
    }

    STAM_PROFILE_STOP(&pPool->StatTrackFlushGCPhysPTs, f);
}


/**
 * Flushes all shadow page table mappings of the given guest page.
 *
 * This is typically called when the host page backing the guest one has been
 * replaced or when the page protection was changed due to a guest access
 * caught by the monitoring.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if all references has been successfully cleared.
 * @retval  VINF_PGM_SYNC_CR3 if we're better off with a CR3 sync and a page
 *          pool cleaning. FF and sync flags are set.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhysPage  GC physical address of the page in question
 * @param   pPhysPage   The guest page in question.
 * @param   fFlushPTEs  Flush PTEs or allow them to be updated (e.g. in case of an RW bit change)
 * @param   pfFlushTLBs This is set to @a true if the shadow TLBs should be
 *                      flushed, it is NOT touched if this isn't necessary.
 *                      The caller MUST initialized this to @a false.
 */
int pgmPoolTrackUpdateGCPhys(PVM pVM, RTGCPHYS GCPhysPage, PPGMPAGE pPhysPage, bool fFlushPTEs, bool *pfFlushTLBs)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);
    pgmLock(pVM);
    int rc = VINF_SUCCESS;

#ifdef PGM_WITH_LARGE_PAGES
    /* Is this page part of a large page? */
    if (PGM_PAGE_GET_PDE_TYPE(pPhysPage) == PGM_PAGE_PDE_TYPE_PDE)
    {
        RTGCPHYS GCPhysBase = GCPhysPage & X86_PDE2M_PAE_PG_MASK;
        GCPhysPage &= X86_PDE_PAE_PG_MASK;

        /* Fetch the large page base. */
        PPGMPAGE pLargePage;
        if (GCPhysBase != GCPhysPage)
        {
            pLargePage = pgmPhysGetPage(pVM, GCPhysBase);
            AssertFatal(pLargePage);
        }
        else
            pLargePage = pPhysPage;

        Log(("pgmPoolTrackUpdateGCPhys: update large page PDE for %RGp (%RGp)\n", GCPhysBase, GCPhysPage));

        if (PGM_PAGE_GET_PDE_TYPE(pLargePage) == PGM_PAGE_PDE_TYPE_PDE)
        {
            /* Mark the large page as disabled as we need to break it up to change a single page in the 2 MB range. */
            PGM_PAGE_SET_PDE_TYPE(pVM, pLargePage, PGM_PAGE_PDE_TYPE_PDE_DISABLED);
            pVM->pgm.s.cLargePagesDisabled++;

            /* Update the base as that *only* that one has a reference and there's only one PDE to clear. */
            rc = pgmPoolTrackUpdateGCPhys(pVM, GCPhysBase, pLargePage, fFlushPTEs, pfFlushTLBs);

            *pfFlushTLBs = true;
            pgmUnlock(pVM);
            return rc;
        }
    }
#else
    NOREF(GCPhysPage);
#endif /* PGM_WITH_LARGE_PAGES */

    const uint16_t u16 = PGM_PAGE_GET_TRACKING(pPhysPage);
    if (u16)
    {
        /*
         * The zero page is currently screwing up the tracking and we'll
         * have to flush the whole shebang. Unless VBOX_WITH_NEW_LAZY_PAGE_ALLOC
         * is defined, zero pages won't normally be mapped. Some kind of solution
         * will be needed for this problem of course, but it will have to wait...
         */
        if (    PGM_PAGE_IS_ZERO(pPhysPage)
            ||  PGM_PAGE_IS_BALLOONED(pPhysPage))
            rc = VINF_PGM_GCPHYS_ALIASED;
        else
        {
# if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC) /** @todo we can drop this now. */
            /* Start a subset here because pgmPoolTrackFlushGCPhysPTsSlow and
               pgmPoolTrackFlushGCPhysPTs will/may kill the pool otherwise. */
            uint32_t iPrevSubset = PGMRZDynMapPushAutoSubset(pVCpu);
# endif

            if (PGMPOOL_TD_GET_CREFS(u16) != PGMPOOL_TD_CREFS_PHYSEXT)
            {
                Assert(PGMPOOL_TD_GET_CREFS(u16) == 1);
                pgmPoolTrackFlushGCPhysPT(pVM,
                                          pPhysPage,
                                          fFlushPTEs,
                                          PGMPOOL_TD_GET_IDX(u16));
            }
            else if (u16 != PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, PGMPOOL_TD_IDX_OVERFLOWED))
                pgmPoolTrackFlushGCPhysPTs(pVM, pPhysPage, fFlushPTEs, PGMPOOL_TD_GET_IDX(u16));
            else
                rc = pgmPoolTrackFlushGCPhysPTsSlow(pVM, pPhysPage);
            *pfFlushTLBs = true;

# if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
            PGMRZDynMapPopAutoSubset(pVCpu, iPrevSubset);
# endif
        }
    }

    if (rc == VINF_PGM_GCPHYS_ALIASED)
    {
        pVCpu->pgm.s.fSyncFlags |= PGM_SYNC_CLEAR_PGM_POOL;
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
        rc = VINF_PGM_SYNC_CR3;
    }
    pgmUnlock(pVM);
    return rc;
}


/**
 * Scans all shadow page tables for mappings of a physical page.
 *
 * This may be slow, but it's most likely more efficient than cleaning
 * out the entire page pool / cache.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if all references has been successfully cleared.
 * @retval  VINF_PGM_GCPHYS_ALIASED if we're better off with a CR3 sync and
 *          a page pool cleaning.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPhysPage   The guest page in question.
 */
int pgmPoolTrackFlushGCPhysPTsSlow(PVM pVM, PPGMPAGE pPhysPage)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    STAM_PROFILE_START(&pPool->StatTrackFlushGCPhysPTsSlow, s);
    LogFlow(("pgmPoolTrackFlushGCPhysPTsSlow: cUsedPages=%d cPresent=%d pPhysPage=%R[pgmpage]\n",
             pPool->cUsedPages, pPool->cPresent, pPhysPage));

    /*
     * There is a limit to what makes sense.
     */
    if (    pPool->cPresent > 1024
        &&  pVM->cCpus == 1)
    {
        LogFlow(("pgmPoolTrackFlushGCPhysPTsSlow: giving up... (cPresent=%d)\n", pPool->cPresent));
        STAM_PROFILE_STOP(&pPool->StatTrackFlushGCPhysPTsSlow, s);
        return VINF_PGM_GCPHYS_ALIASED;
    }

    /*
     * Iterate all the pages until we've encountered all that in use.
     * This is simple but not quite optimal solution.
     */
    const uint64_t  u64   = PGM_PAGE_GET_HCPHYS(pPhysPage) | X86_PTE_P;   /** @todo drop X86_PTE_P here as we always test if present separately, anyway. */
    const uint32_t  u32   = u64;                                          /** @todo move into the 32BIT_PT_xx case */
    unsigned        cLeft = pPool->cUsedPages;
    unsigned        iPage = pPool->cCurPages;
    while (--iPage >= PGMPOOL_IDX_FIRST)
    {
        PPGMPOOLPAGE pPage = &pPool->aPages[iPage];
        if (    pPage->GCPhys != NIL_RTGCPHYS
            &&  pPage->cPresent)
        {
            switch (pPage->enmKind)
            {
                /*
                 * We only care about shadow page tables.
                 */
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
                {
                    unsigned    cPresent = pPage->cPresent;
                    PX86PT      pPT = (PX86PT)PGMPOOL_PAGE_2_PTR(pVM, pPage);
                    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pPT->a); i++)
                        if (pPT->a[i].n.u1Present)
                        {
                            if ((pPT->a[i].u & (X86_PTE_PG_MASK | X86_PTE_P)) == u32)
                            {
                                //Log4(("pgmPoolTrackFlushGCPhysPTsSlow: idx=%d i=%d pte=%RX32\n", iPage, i, pPT->a[i]));
                                pPT->a[i].u = 0;

                                /* Update the counter as we're removing references. */
                                Assert(pPage->cPresent);
                                Assert(pPool->cPresent);
                                pPage->cPresent--;
                                pPool->cPresent--;
                            }
                            if (!--cPresent)
                                break;
                        }
                    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPT);
                    break;
                }

                case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
                case PGMPOOLKIND_PAE_PT_FOR_PHYS:
                {
                    unsigned        cPresent = pPage->cPresent;
                    PPGMSHWPTPAE    pPT = (PPGMSHWPTPAE)PGMPOOL_PAGE_2_PTR(pVM, pPage);
                    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pPT->a); i++)
                        if (PGMSHWPTEPAE_IS_P(pPT->a[i]))
                        {
                            if ((PGMSHWPTEPAE_GET_U(pPT->a[i]) & (X86_PTE_PAE_PG_MASK | X86_PTE_P)) == u64)
                            {
                                //Log4(("pgmPoolTrackFlushGCPhysPTsSlow: idx=%d i=%d pte=%RX64\n", iPage, i, pPT->a[i]));
                                PGMSHWPTEPAE_SET(pPT->a[i], 0); /// @todo why not atomic?

                                /* Update the counter as we're removing references. */
                                Assert(pPage->cPresent);
                                Assert(pPool->cPresent);
                                pPage->cPresent--;
                                pPool->cPresent--;
                            }
                            if (!--cPresent)
                                break;
                        }
                    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPT);
                    break;
                }
#ifndef IN_RC
                case PGMPOOLKIND_EPT_PT_FOR_PHYS:
                {
                    unsigned  cPresent = pPage->cPresent;
                    PEPTPT    pPT = (PEPTPT)PGMPOOL_PAGE_2_PTR(pVM, pPage);
                    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pPT->a); i++)
                        if (pPT->a[i].n.u1Present)
                        {
                            if ((pPT->a[i].u & (EPT_PTE_PG_MASK | X86_PTE_P)) == u64)
                            {
                                //Log4(("pgmPoolTrackFlushGCPhysPTsSlow: idx=%d i=%d pte=%RX64\n", iPage, i, pPT->a[i]));
                                pPT->a[i].u = 0;

                                /* Update the counter as we're removing references. */
                                Assert(pPage->cPresent);
                                Assert(pPool->cPresent);
                                pPage->cPresent--;
                                pPool->cPresent--;
                            }
                            if (!--cPresent)
                                break;
                        }
                    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pPT);
                    break;
                }
#endif
            }
            if (!--cLeft)
                break;
        }
    }

    PGM_PAGE_SET_TRACKING(pVM, pPhysPage, 0);
    STAM_PROFILE_STOP(&pPool->StatTrackFlushGCPhysPTsSlow, s);

    /*
     * There is a limit to what makes sense. The above search is very expensive, so force a pgm pool flush.
     */
    if (pPool->cPresent > 1024)
    {
        LogFlow(("pgmPoolTrackFlushGCPhysPTsSlow: giving up... (cPresent=%d)\n", pPool->cPresent));
        return VINF_PGM_GCPHYS_ALIASED;
    }

    return VINF_SUCCESS;
}


/**
 * Clears the user entry in a user table.
 *
 * This is used to remove all references to a page when flushing it.
 */
static void pgmPoolTrackClearPageUser(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PCPGMPOOLUSER pUser)
{
    Assert(pUser->iUser != NIL_PGMPOOL_IDX);
    Assert(pUser->iUser < pPool->cCurPages);
    uint32_t iUserTable = pUser->iUserTable;

    /*
     * Map the user page.  Ignore references made by fictitious pages.
     */
    PPGMPOOLPAGE pUserPage = &pPool->aPages[pUser->iUser];
    LogFlow(("pgmPoolTrackClearPageUser: clear %x in %s (%RGp) (flushing %s)\n", iUserTable, pgmPoolPoolKindToStr(pUserPage->enmKind), pUserPage->Core.Key, pgmPoolPoolKindToStr(pPage->enmKind)));
    union
    {
        uint64_t       *pau64;
        uint32_t       *pau32;
    } u;
    if (pUserPage->idx < PGMPOOL_IDX_FIRST)
    {
        Assert(!pUserPage->pvPageR3);
        return;
    }
    u.pau64 = (uint64_t *)PGMPOOL_PAGE_2_PTR(pPool->CTX_SUFF(pVM), pUserPage);


    /* Safety precaution in case we change the paging for other modes too in the future. */
    Assert(!pgmPoolIsPageLocked(pPage)); RT_NOREF_PV(pPage);

#ifdef VBOX_STRICT
    /*
     * Some sanity checks.
     */
    switch (pUserPage->enmKind)
    {
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_32BIT_PD_PHYS:
            Assert(iUserTable < X86_PG_ENTRIES);
            break;
        case PGMPOOLKIND_PAE_PDPT:
        case PGMPOOLKIND_PAE_PDPT_FOR_32BIT:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
            Assert(iUserTable < 4);
            Assert(!(u.pau64[iUserTable] & PGM_PLXFLAGS_PERMANENT));
            break;
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
        case PGMPOOLKIND_PAE_PD_PHYS:
            Assert(iUserTable < X86_PG_PAE_ENTRIES);
            break;
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
            Assert(iUserTable < X86_PG_PAE_ENTRIES);
            Assert(!(u.pau64[iUserTable] & PGM_PDFLAGS_MAPPING));
            break;
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
            Assert(iUserTable < X86_PG_PAE_ENTRIES);
            Assert(!(u.pau64[iUserTable] & PGM_PLXFLAGS_PERMANENT));
            break;
        case PGMPOOLKIND_64BIT_PML4:
            Assert(!(u.pau64[iUserTable] & PGM_PLXFLAGS_PERMANENT));
            /* GCPhys >> PAGE_SHIFT is the index here */
            break;
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
            Assert(iUserTable < X86_PG_PAE_ENTRIES);
            break;

        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
            Assert(iUserTable < X86_PG_PAE_ENTRIES);
            break;

        case PGMPOOLKIND_ROOT_NESTED:
            Assert(iUserTable < X86_PG_PAE_ENTRIES);
            break;

        default:
            AssertMsgFailed(("enmKind=%d\n", pUserPage->enmKind));
            break;
    }
#endif /* VBOX_STRICT */

    /*
     * Clear the entry in the user page.
     */
    switch (pUserPage->enmKind)
    {
        /* 32-bit entries */
        case PGMPOOLKIND_32BIT_PD:
        case PGMPOOLKIND_32BIT_PD_PHYS:
            ASMAtomicWriteU32(&u.pau32[iUserTable], 0);
            break;

        /* 64-bit entries */
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
#ifdef IN_RC
            /*
             * In 32 bits PAE mode we *must* invalidate the TLB when changing a
             * PDPT entry; the CPU fetches them only during cr3 load, so any
             * non-present PDPT will continue to cause page faults.
             */
            ASMReloadCR3();
#endif
            RT_FALL_THRU();
        case PGMPOOLKIND_PAE_PD_PHYS:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
        case PGMPOOLKIND_64BIT_PML4:
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
        case PGMPOOLKIND_PAE_PDPT:
        case PGMPOOLKIND_PAE_PDPT_FOR_32BIT:
        case PGMPOOLKIND_ROOT_NESTED:
        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
            ASMAtomicWriteU64(&u.pau64[iUserTable], 0);
            break;

        default:
            AssertFatalMsgFailed(("enmKind=%d iUser=%d iUserTable=%#x\n", pUserPage->enmKind, pUser->iUser, pUser->iUserTable));
    }
    PGM_DYNMAP_UNUSED_HINT_VM(pPool->CTX_SUFF(pVM), u.pau64);
}


/**
 * Clears all users of a page.
 */
static void pgmPoolTrackClearPageUsers(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    /*
     * Free all the user records.
     */
    LogFlow(("pgmPoolTrackClearPageUsers %RGp\n", pPage->GCPhys));

    PPGMPOOLUSER paUsers = pPool->CTX_SUFF(paUsers);
    uint16_t i = pPage->iUserHead;
    while (i != NIL_PGMPOOL_USER_INDEX)
    {
        /* Clear enter in user table. */
        pgmPoolTrackClearPageUser(pPool, pPage, &paUsers[i]);

        /* Free it. */
        const uint16_t iNext = paUsers[i].iNext;
        paUsers[i].iUser = NIL_PGMPOOL_IDX;
        paUsers[i].iNext = pPool->iUserFreeHead;
        pPool->iUserFreeHead = i;

        /* Next. */
        i = iNext;
    }
    pPage->iUserHead = NIL_PGMPOOL_USER_INDEX;
}


/**
 * Allocates a new physical cross reference extent.
 *
 * @returns Pointer to the allocated extent on success. NULL if we're out of them.
 * @param   pVM         The cross context VM structure.
 * @param   piPhysExt   Where to store the phys ext index.
 */
PPGMPOOLPHYSEXT pgmPoolTrackPhysExtAlloc(PVM pVM, uint16_t *piPhysExt)
{
    PGM_LOCK_ASSERT_OWNER(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    uint16_t iPhysExt = pPool->iPhysExtFreeHead;
    if (iPhysExt == NIL_PGMPOOL_PHYSEXT_INDEX)
    {
        STAM_COUNTER_INC(&pPool->StamTrackPhysExtAllocFailures);
        return NULL;
    }
    PPGMPOOLPHYSEXT pPhysExt = &pPool->CTX_SUFF(paPhysExts)[iPhysExt];
    pPool->iPhysExtFreeHead = pPhysExt->iNext;
    pPhysExt->iNext = NIL_PGMPOOL_PHYSEXT_INDEX;
    *piPhysExt = iPhysExt;
    return pPhysExt;
}


/**
 * Frees a physical cross reference extent.
 *
 * @param   pVM         The cross context VM structure.
 * @param   iPhysExt    The extent to free.
 */
void pgmPoolTrackPhysExtFree(PVM pVM, uint16_t iPhysExt)
{
    PGM_LOCK_ASSERT_OWNER(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    Assert(iPhysExt < pPool->cMaxPhysExts);
    PPGMPOOLPHYSEXT pPhysExt = &pPool->CTX_SUFF(paPhysExts)[iPhysExt];
    for (unsigned i = 0; i < RT_ELEMENTS(pPhysExt->aidx); i++)
    {
        pPhysExt->aidx[i] = NIL_PGMPOOL_IDX;
        pPhysExt->apte[i] = NIL_PGMPOOL_PHYSEXT_IDX_PTE;
    }
    pPhysExt->iNext = pPool->iPhysExtFreeHead;
    pPool->iPhysExtFreeHead = iPhysExt;
}


/**
 * Frees a physical cross reference extent.
 *
 * @param   pVM         The cross context VM structure.
 * @param   iPhysExt    The extent to free.
 */
void pgmPoolTrackPhysExtFreeList(PVM pVM, uint16_t iPhysExt)
{
    PGM_LOCK_ASSERT_OWNER(pVM);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

    const uint16_t  iPhysExtStart = iPhysExt;
    PPGMPOOLPHYSEXT pPhysExt;
    do
    {
        Assert(iPhysExt < pPool->cMaxPhysExts);
        pPhysExt = &pPool->CTX_SUFF(paPhysExts)[iPhysExt];
        for (unsigned i = 0; i < RT_ELEMENTS(pPhysExt->aidx); i++)
        {
            pPhysExt->aidx[i] = NIL_PGMPOOL_IDX;
            pPhysExt->apte[i] = NIL_PGMPOOL_PHYSEXT_IDX_PTE;
        }

        /* next */
        iPhysExt = pPhysExt->iNext;
    } while (iPhysExt != NIL_PGMPOOL_PHYSEXT_INDEX);

    pPhysExt->iNext = pPool->iPhysExtFreeHead;
    pPool->iPhysExtFreeHead = iPhysExtStart;
}


/**
 * Insert a reference into a list of physical cross reference extents.
 *
 * @returns The new tracking data for PGMPAGE.
 *
 * @param   pVM         The cross context VM structure.
 * @param   iPhysExt    The physical extent index of the list head.
 * @param   iShwPT      The shadow page table index.
 * @param   iPte        Page table entry
 *
 */
static uint16_t pgmPoolTrackPhysExtInsert(PVM pVM, uint16_t iPhysExt, uint16_t iShwPT, uint16_t iPte)
{
    PGM_LOCK_ASSERT_OWNER(pVM);
    PPGMPOOL        pPool = pVM->pgm.s.CTX_SUFF(pPool);
    PPGMPOOLPHYSEXT paPhysExts = pPool->CTX_SUFF(paPhysExts);

    /*
     * Special common cases.
     */
    if (paPhysExts[iPhysExt].aidx[1] == NIL_PGMPOOL_IDX)
    {
        paPhysExts[iPhysExt].aidx[1] = iShwPT;
        paPhysExts[iPhysExt].apte[1] = iPte;
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackAliasedMany);
        LogFlow(("pgmPoolTrackPhysExtInsert: %d:{,%d pte %d,}\n", iPhysExt, iShwPT, iPte));
        return PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, iPhysExt);
    }
    if (paPhysExts[iPhysExt].aidx[2] == NIL_PGMPOOL_IDX)
    {
        paPhysExts[iPhysExt].aidx[2] = iShwPT;
        paPhysExts[iPhysExt].apte[2] = iPte;
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackAliasedMany);
        LogFlow(("pgmPoolTrackPhysExtInsert: %d:{,,%d pte %d}\n", iPhysExt, iShwPT, iPte));
        return PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, iPhysExt);
    }
    AssertCompile(RT_ELEMENTS(paPhysExts[iPhysExt].aidx) == 3);

    /*
     * General treatment.
     */
    const uint16_t iPhysExtStart = iPhysExt;
    unsigned cMax = 15;
    for (;;)
    {
        Assert(iPhysExt < pPool->cMaxPhysExts);
        for (unsigned i = 0; i < RT_ELEMENTS(paPhysExts[iPhysExt].aidx); i++)
            if (paPhysExts[iPhysExt].aidx[i] == NIL_PGMPOOL_IDX)
            {
                paPhysExts[iPhysExt].aidx[i] = iShwPT;
                paPhysExts[iPhysExt].apte[i] = iPte;
                STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackAliasedMany);
                LogFlow(("pgmPoolTrackPhysExtInsert: %d:{%d pte %d} i=%d cMax=%d\n", iPhysExt, iShwPT, iPte, i, cMax));
                return PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, iPhysExtStart);
            }
        if (!--cMax)
        {
            STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackOverflows);
            pgmPoolTrackPhysExtFreeList(pVM, iPhysExtStart);
            LogFlow(("pgmPoolTrackPhysExtInsert: overflow (1) iShwPT=%d\n", iShwPT));
            return PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, PGMPOOL_TD_IDX_OVERFLOWED);
        }

        /* advance */
        iPhysExt = paPhysExts[iPhysExt].iNext;
        if (iPhysExt == NIL_PGMPOOL_PHYSEXT_INDEX)
            break;
    }

    /*
     * Add another extent to the list.
     */
    PPGMPOOLPHYSEXT pNew = pgmPoolTrackPhysExtAlloc(pVM, &iPhysExt);
    if (!pNew)
    {
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackNoExtentsLeft);
        pgmPoolTrackPhysExtFreeList(pVM, iPhysExtStart);
        LogFlow(("pgmPoolTrackPhysExtInsert: pgmPoolTrackPhysExtAlloc failed iShwPT=%d\n", iShwPT));
        return PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, PGMPOOL_TD_IDX_OVERFLOWED);
    }
    pNew->iNext = iPhysExtStart;
    pNew->aidx[0] = iShwPT;
    pNew->apte[0] = iPte;
    LogFlow(("pgmPoolTrackPhysExtInsert: added new extent %d:{%d pte %d}->%d\n", iPhysExt, iShwPT, iPte, iPhysExtStart));
    return PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, iPhysExt);
}


/**
 * Add a reference to guest physical page where extents are in use.
 *
 * @returns The new tracking data for PGMPAGE.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pPhysPage   Pointer to the aPages entry in the ram range.
 * @param   u16         The ram range flags (top 16-bits).
 * @param   iShwPT      The shadow page table index.
 * @param   iPte        Page table entry
 */
uint16_t pgmPoolTrackPhysExtAddref(PVM pVM, PPGMPAGE pPhysPage, uint16_t u16, uint16_t iShwPT, uint16_t iPte)
{
    pgmLock(pVM);
    if (PGMPOOL_TD_GET_CREFS(u16) != PGMPOOL_TD_CREFS_PHYSEXT)
    {
        /*
         * Convert to extent list.
         */
        Assert(PGMPOOL_TD_GET_CREFS(u16) == 1);
        uint16_t iPhysExt;
        PPGMPOOLPHYSEXT pPhysExt = pgmPoolTrackPhysExtAlloc(pVM, &iPhysExt);
        if (pPhysExt)
        {
            LogFlow(("pgmPoolTrackPhysExtAddref: new extent: %d:{%d, %d}\n", iPhysExt, PGMPOOL_TD_GET_IDX(u16), iShwPT));
            STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackAliased);
            pPhysExt->aidx[0] = PGMPOOL_TD_GET_IDX(u16);
            pPhysExt->apte[0] = PGM_PAGE_GET_PTE_INDEX(pPhysPage);
            pPhysExt->aidx[1] = iShwPT;
            pPhysExt->apte[1] = iPte;
            u16 = PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, iPhysExt);
        }
        else
            u16 = PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, PGMPOOL_TD_IDX_OVERFLOWED);
    }
    else if (u16 != PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, PGMPOOL_TD_IDX_OVERFLOWED))
    {
        /*
         * Insert into the extent list.
         */
        u16 = pgmPoolTrackPhysExtInsert(pVM, PGMPOOL_TD_GET_IDX(u16), iShwPT, iPte);
    }
    else
        STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatTrackAliasedLots);
    pgmUnlock(pVM);
    return u16;
}


/**
 * Clear references to guest physical memory.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pPhysPage   Pointer to the aPages entry in the ram range.
 * @param   iPte        Shadow PTE index
 */
void pgmPoolTrackPhysExtDerefGCPhys(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMPAGE pPhysPage, uint16_t iPte)
{
    PVM            pVM = pPool->CTX_SUFF(pVM);
    const unsigned cRefs = PGM_PAGE_GET_TD_CREFS(pPhysPage);
    AssertFatalMsg(cRefs == PGMPOOL_TD_CREFS_PHYSEXT, ("cRefs=%d pPhysPage=%R[pgmpage] pPage=%p:{.idx=%d}\n", cRefs, pPhysPage, pPage, pPage->idx));

    uint16_t iPhysExt = PGM_PAGE_GET_TD_IDX(pPhysPage);
    if (iPhysExt != PGMPOOL_TD_IDX_OVERFLOWED)
    {
        pgmLock(pVM);

        uint16_t        iPhysExtPrev = NIL_PGMPOOL_PHYSEXT_INDEX;
        PPGMPOOLPHYSEXT paPhysExts = pPool->CTX_SUFF(paPhysExts);
        do
        {
            Assert(iPhysExt < pPool->cMaxPhysExts);

            /*
             * Look for the shadow page and check if it's all freed.
             */
            for (unsigned i = 0; i < RT_ELEMENTS(paPhysExts[iPhysExt].aidx); i++)
            {
                if (    paPhysExts[iPhysExt].aidx[i] == pPage->idx
                    &&  paPhysExts[iPhysExt].apte[i] == iPte)
                {
                    paPhysExts[iPhysExt].aidx[i] = NIL_PGMPOOL_IDX;
                    paPhysExts[iPhysExt].apte[i] = NIL_PGMPOOL_PHYSEXT_IDX_PTE;

                    for (i = 0; i < RT_ELEMENTS(paPhysExts[iPhysExt].aidx); i++)
                        if (paPhysExts[iPhysExt].aidx[i] != NIL_PGMPOOL_IDX)
                        {
                            Log2(("pgmPoolTrackPhysExtDerefGCPhys: pPhysPage=%R[pgmpage] idx=%d\n", pPhysPage, pPage->idx));
                            pgmUnlock(pVM);
                            return;
                        }

                    /* we can free the node. */
                    const uint16_t iPhysExtNext = paPhysExts[iPhysExt].iNext;
                    if (    iPhysExtPrev == NIL_PGMPOOL_PHYSEXT_INDEX
                        &&  iPhysExtNext == NIL_PGMPOOL_PHYSEXT_INDEX)
                    {
                        /* lonely node */
                        pgmPoolTrackPhysExtFree(pVM, iPhysExt);
                        Log2(("pgmPoolTrackPhysExtDerefGCPhys: pPhysPage=%R[pgmpage] idx=%d lonely\n", pPhysPage, pPage->idx));
                        PGM_PAGE_SET_TRACKING(pVM, pPhysPage, 0);
                    }
                    else if (iPhysExtPrev == NIL_PGMPOOL_PHYSEXT_INDEX)
                    {
                        /* head */
                        Log2(("pgmPoolTrackPhysExtDerefGCPhys: pPhysPage=%R[pgmpage] idx=%d head\n", pPhysPage, pPage->idx));
                        PGM_PAGE_SET_TRACKING(pVM, pPhysPage, PGMPOOL_TD_MAKE(PGMPOOL_TD_CREFS_PHYSEXT, iPhysExtNext));
                        pgmPoolTrackPhysExtFree(pVM, iPhysExt);
                    }
                    else
                    {
                        /* in list */
                        Log2(("pgmPoolTrackPhysExtDerefGCPhys: pPhysPage=%R[pgmpage] idx=%d in list\n", pPhysPage, pPage->idx));
                        paPhysExts[iPhysExtPrev].iNext = iPhysExtNext;
                        pgmPoolTrackPhysExtFree(pVM, iPhysExt);
                    }
                    iPhysExt = iPhysExtNext;
                    pgmUnlock(pVM);
                    return;
                }
            }

            /* next */
            iPhysExtPrev = iPhysExt;
            iPhysExt = paPhysExts[iPhysExt].iNext;
        } while (iPhysExt != NIL_PGMPOOL_PHYSEXT_INDEX);

        pgmUnlock(pVM);
        AssertFatalMsgFailed(("not-found! cRefs=%d pPhysPage=%R[pgmpage] pPage=%p:{.idx=%d}\n", cRefs, pPhysPage, pPage, pPage->idx));
    }
    else /* nothing to do */
        Log2(("pgmPoolTrackPhysExtDerefGCPhys: pPhysPage=%R[pgmpage]\n", pPhysPage));
}

/**
 * Clear references to guest physical memory.
 *
 * This is the same as pgmPoolTracDerefGCPhysHint except that the guest
 * physical address is assumed to be correct, so the linear search can be
 * skipped and we can assert at an earlier point.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   HCPhys      The host physical address corresponding to the guest page.
 * @param   GCPhys      The guest physical address corresponding to HCPhys.
 * @param   iPte        Shadow PTE index
 */
static void pgmPoolTracDerefGCPhys(PPGMPOOL pPool, PPGMPOOLPAGE pPage, RTHCPHYS HCPhys, RTGCPHYS GCPhys, uint16_t iPte)
{
    /*
     * Lookup the page and check if it checks out before derefing it.
     */
    PVM      pVM       = pPool->CTX_SUFF(pVM);
    PPGMPAGE pPhysPage = pgmPhysGetPage(pVM, GCPhys);
    if (pPhysPage)
    {
        Assert(PGM_PAGE_GET_HCPHYS(pPhysPage));
#ifdef LOG_ENABLED
        RTHCPHYS HCPhysPage = PGM_PAGE_GET_HCPHYS(pPhysPage);
        Log2(("pgmPoolTracDerefGCPhys %RHp vs %RHp\n", HCPhysPage, HCPhys));
#endif
        if (PGM_PAGE_GET_HCPHYS(pPhysPage) == HCPhys)
        {
            Assert(pPage->cPresent);
            Assert(pPool->cPresent);
            pPage->cPresent--;
            pPool->cPresent--;
            pgmTrackDerefGCPhys(pPool, pPage, pPhysPage, iPte);
            return;
        }

        AssertFatalMsgFailed(("HCPhys=%RHp GCPhys=%RGp; found page has HCPhys=%RHp\n",
                              HCPhys, GCPhys, PGM_PAGE_GET_HCPHYS(pPhysPage)));
    }
    AssertFatalMsgFailed(("HCPhys=%RHp GCPhys=%RGp\n", HCPhys, GCPhys));
}


/**
 * Clear references to guest physical memory.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   HCPhys      The host physical address corresponding to the guest page.
 * @param   GCPhysHint  The guest physical address which may corresponding to HCPhys.
 * @param   iPte        Shadow pte index
 */
void pgmPoolTracDerefGCPhysHint(PPGMPOOL pPool, PPGMPOOLPAGE pPage, RTHCPHYS HCPhys, RTGCPHYS GCPhysHint, uint16_t iPte)
{
    Log4(("pgmPoolTracDerefGCPhysHint %RHp %RGp\n", HCPhys, GCPhysHint));

    /*
     * Try the hint first.
     */
    RTHCPHYS HCPhysHinted;
    PVM      pVM       = pPool->CTX_SUFF(pVM);
    PPGMPAGE pPhysPage = pgmPhysGetPage(pVM, GCPhysHint);
    if (pPhysPage)
    {
        HCPhysHinted = PGM_PAGE_GET_HCPHYS(pPhysPage);
        Assert(HCPhysHinted);
        if (HCPhysHinted == HCPhys)
        {
            Assert(pPage->cPresent);
            Assert(pPool->cPresent);
            pPage->cPresent--;
            pPool->cPresent--;
            pgmTrackDerefGCPhys(pPool, pPage, pPhysPage, iPte);
            return;
        }
    }
    else
        HCPhysHinted = UINT64_C(0xdeadbeefdeadbeef);

    /*
     * Damn, the hint didn't work.  We'll have to do an expensive linear search.
     */
    STAM_COUNTER_INC(&pPool->StatTrackLinearRamSearches);
    PPGMRAMRANGE pRam = pPool->CTX_SUFF(pVM)->pgm.s.CTX_SUFF(pRamRangesX);
    while (pRam)
    {
        unsigned iPage = pRam->cb >> PAGE_SHIFT;
        while (iPage-- > 0)
        {
            if (PGM_PAGE_GET_HCPHYS(&pRam->aPages[iPage]) == HCPhys)
            {
                Log4(("pgmPoolTracDerefGCPhysHint: Linear HCPhys=%RHp GCPhysHint=%RGp GCPhysReal=%RGp\n",
                      HCPhys, GCPhysHint, pRam->GCPhys + (iPage << PAGE_SHIFT)));
                Assert(pPage->cPresent);
                Assert(pPool->cPresent);
                pPage->cPresent--;
                pPool->cPresent--;
                pgmTrackDerefGCPhys(pPool, pPage, &pRam->aPages[iPage], iPte);
                return;
            }
        }
        pRam = pRam->CTX_SUFF(pNext);
    }

    AssertFatalMsgFailed(("HCPhys=%RHp GCPhysHint=%RGp (Hinted page has HCPhys = %RHp)\n", HCPhys, GCPhysHint, HCPhysHinted));
}


/**
 * Clear references to guest physical memory in a 32-bit / 32-bit page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page table (mapping of the page).
 * @param   pGstPT      The guest page table.
 */
DECLINLINE(void) pgmPoolTrackDerefPT32Bit32Bit(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PX86PT pShwPT, PCX86PT pGstPT)
{
    RTGCPHYS32 const fPgMask = pPage->fA20Enabled ? X86_PTE_PG_MASK : X86_PTE_PG_MASK & ~RT_BIT_32(20);
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++)
    {
        Assert(!(pShwPT->a[i].u & RT_BIT_32(10)));
        if (pShwPT->a[i].n.u1Present)
        {
            Log4(("pgmPoolTrackDerefPT32Bit32Bit: i=%d pte=%RX32 hint=%RX32\n",
                  i, pShwPT->a[i].u & X86_PTE_PG_MASK, pGstPT->a[i].u & X86_PTE_PG_MASK));
            pgmPoolTracDerefGCPhysHint(pPool, pPage, pShwPT->a[i].u & X86_PTE_PG_MASK, pGstPT->a[i].u & fPgMask, i);
            if (!pPage->cPresent)
                break;
        }
    }
}


/**
 * Clear references to guest physical memory in a PAE / 32-bit page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page table (mapping of the page).
 * @param   pGstPT      The guest page table (just a half one).
 */
DECLINLINE(void) pgmPoolTrackDerefPTPae32Bit(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT, PCX86PT pGstPT)
{
    RTGCPHYS32 const fPgMask = pPage->fA20Enabled ? X86_PTE_PG_MASK : X86_PTE_PG_MASK & ~RT_BIT_32(20);
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++)
    {
        Assert(   (PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & UINT64_C(0x7ff0000000000400)) == 0
               || (PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & UINT64_C(0x7ff0000000000400)) == UINT64_C(0x7ff0000000000000));
        if (PGMSHWPTEPAE_IS_P(pShwPT->a[i]))
        {
            Log4(("pgmPoolTrackDerefPTPae32Bit: i=%d pte=%RX64 hint=%RX32\n",
                  i, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pGstPT->a[i].u & X86_PTE_PG_MASK));
            pgmPoolTracDerefGCPhysHint(pPool, pPage, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pGstPT->a[i].u & fPgMask, i);
            if (!pPage->cPresent)
                break;
        }
    }
}


/**
 * Clear references to guest physical memory in a PAE / PAE page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page table (mapping of the page).
 * @param   pGstPT      The guest page table.
 */
DECLINLINE(void) pgmPoolTrackDerefPTPaePae(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT, PCX86PTPAE pGstPT)
{
    RTGCPHYS const fPgMask = pPage->fA20Enabled ? X86_PTE_PAE_PG_MASK : X86_PTE_PAE_PG_MASK & ~RT_BIT_64(20);
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++)
    {
        Assert(   (PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & UINT64_C(0x7ff0000000000400)) == 0
               || (PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & UINT64_C(0x7ff0000000000400)) == UINT64_C(0x7ff0000000000000));
        if (PGMSHWPTEPAE_IS_P(pShwPT->a[i]))
        {
            Log4(("pgmPoolTrackDerefPTPaePae: i=%d pte=%RX32 hint=%RX32\n",
                  i, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pGstPT->a[i].u & X86_PTE_PAE_PG_MASK));
            pgmPoolTracDerefGCPhysHint(pPool, pPage, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), pGstPT->a[i].u & fPgMask, i);
            if (!pPage->cPresent)
                break;
        }
    }
}


/**
 * Clear references to guest physical memory in a 32-bit / 4MB page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page table (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPT32Bit4MB(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PX86PT pShwPT)
{
    RTGCPHYS const  GCPhysA20Mask = pPage->fA20Enabled ? UINT64_MAX : ~RT_BIT_64(20);
    RTGCPHYS        GCPhys        = pPage->GCPhys + PAGE_SIZE * pPage->iFirstPresent;
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++, GCPhys += PAGE_SIZE)
    {
        Assert(!(pShwPT->a[i].u & RT_BIT_32(10)));
        if (pShwPT->a[i].n.u1Present)
        {
            Log4(("pgmPoolTrackDerefPT32Bit4MB: i=%d pte=%RX32 GCPhys=%RGp\n",
                  i, pShwPT->a[i].u & X86_PTE_PG_MASK, GCPhys));
            pgmPoolTracDerefGCPhys(pPool, pPage, pShwPT->a[i].u & X86_PTE_PG_MASK, GCPhys & GCPhysA20Mask, i);
            if (!pPage->cPresent)
                break;
        }
    }
}


/**
 * Clear references to guest physical memory in a PAE / 2/4MB page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page table (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPTPaeBig(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PPGMSHWPTPAE pShwPT)
{
    RTGCPHYS const  GCPhysA20Mask = pPage->fA20Enabled ? UINT64_MAX : ~RT_BIT_64(20);
    RTGCPHYS        GCPhys        = pPage->GCPhys + PAGE_SIZE * pPage->iFirstPresent;
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++, GCPhys += PAGE_SIZE)
    {
        Assert(   (PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & UINT64_C(0x7ff0000000000400)) == 0
               || (PGMSHWPTEPAE_GET_U(pShwPT->a[i]) & UINT64_C(0x7ff0000000000400)) == UINT64_C(0x7ff0000000000000));
        if (PGMSHWPTEPAE_IS_P(pShwPT->a[i]))
        {
            Log4(("pgmPoolTrackDerefPTPaeBig: i=%d pte=%RX64 hint=%RGp\n",
                  i, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), GCPhys));
            pgmPoolTracDerefGCPhys(pPool, pPage, PGMSHWPTEPAE_GET_HCPHYS(pShwPT->a[i]), GCPhys & GCPhysA20Mask, i);
            if (!pPage->cPresent)
                break;
        }
    }
}


/**
 * Clear references to shadowed pages in an EPT page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPT      The shadow page directory pointer table (mapping of the
 *                      page).
 */
DECLINLINE(void) pgmPoolTrackDerefPTEPT(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PEPTPT pShwPT)
{
    RTGCPHYS const  GCPhysA20Mask = pPage->fA20Enabled ? UINT64_MAX : ~RT_BIT_64(20);
    RTGCPHYS        GCPhys        = pPage->GCPhys + PAGE_SIZE * pPage->iFirstPresent;
    for (unsigned i = pPage->iFirstPresent; i < RT_ELEMENTS(pShwPT->a); i++, GCPhys += PAGE_SIZE)
    {
        Assert((pShwPT->a[i].u & UINT64_C(0xfff0000000000f80)) == 0);
        if (pShwPT->a[i].n.u1Present)
        {
            Log4(("pgmPoolTrackDerefPTEPT: i=%d pte=%RX64 GCPhys=%RX64\n",
                  i, pShwPT->a[i].u & EPT_PTE_PG_MASK, pPage->GCPhys));
            pgmPoolTracDerefGCPhys(pPool, pPage, pShwPT->a[i].u & EPT_PTE_PG_MASK, GCPhys & GCPhysA20Mask, i);
            if (!pPage->cPresent)
                break;
        }
    }
}


/**
 * Clear references to shadowed pages in a 32 bits page directory.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPD      The shadow page directory (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPD(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PX86PD pShwPD)
{
    for (unsigned i = 0; i < RT_ELEMENTS(pShwPD->a); i++)
    {
        if (    pShwPD->a[i].n.u1Present
            &&  !(pShwPD->a[i].u & PGM_PDFLAGS_MAPPING)
           )
        {
            PPGMPOOLPAGE pSubPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, pShwPD->a[i].u & X86_PDE_PG_MASK);
            if (pSubPage)
                pgmPoolTrackFreeUser(pPool, pSubPage, pPage->idx, i);
            else
                AssertFatalMsgFailed(("%x\n", pShwPD->a[i].u & X86_PDE_PG_MASK));
        }
    }
}


/**
 * Clear references to shadowed pages in a PAE (legacy or 64 bits) page directory.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPD      The shadow page directory (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPDPae(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PX86PDPAE pShwPD)
{
    for (unsigned i = 0; i < RT_ELEMENTS(pShwPD->a); i++)
    {
        if (   pShwPD->a[i].n.u1Present
            && !(pShwPD->a[i].u & PGM_PDFLAGS_MAPPING))
        {
#ifdef PGM_WITH_LARGE_PAGES
            if (pShwPD->a[i].b.u1Size)
            {
                Log4(("pgmPoolTrackDerefPDPae: i=%d pde=%RX64 GCPhys=%RX64\n",
                      i, pShwPD->a[i].u & X86_PDE2M_PAE_PG_MASK, pPage->GCPhys));
                pgmPoolTracDerefGCPhys(pPool, pPage, pShwPD->a[i].u & X86_PDE2M_PAE_PG_MASK,
                                       pPage->GCPhys + i * 2 * _1M /* pPage->GCPhys = base address of the memory described by the PD */,
                                       i);
            }
            else
#endif
            {
                Assert((pShwPD->a[i].u & (X86_PDE_PAE_MBZ_MASK_NX | UINT64_C(0x7ff0000000000000))) == 0);
                PPGMPOOLPAGE pSubPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, pShwPD->a[i].u & X86_PDE_PAE_PG_MASK);
                if (pSubPage)
                    pgmPoolTrackFreeUser(pPool, pSubPage, pPage->idx, i);
                else
                    AssertFatalMsgFailed(("%RX64\n", pShwPD->a[i].u & X86_PDE_PAE_PG_MASK));
                /** @todo 64-bit guests: have to ensure that we're not exhausting the dynamic mappings! */
            }
        }
    }
}


/**
 * Clear references to shadowed pages in a PAE page directory pointer table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPDPT   The shadow page directory pointer table (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPDPTPae(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PX86PDPT pShwPDPT)
{
    for (unsigned i = 0; i < X86_PG_PAE_PDPE_ENTRIES; i++)
    {
        Assert((pShwPDPT->a[i].u & (X86_PDPE_PAE_MBZ_MASK | UINT64_C(0x7ff0000000000200))) == 0);
        if (    pShwPDPT->a[i].n.u1Present
            &&  !(pShwPDPT->a[i].u & PGM_PLXFLAGS_MAPPING)
           )
        {
            PPGMPOOLPAGE pSubPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, pShwPDPT->a[i].u & X86_PDPE_PG_MASK);
            if (pSubPage)
                pgmPoolTrackFreeUser(pPool, pSubPage, pPage->idx, i);
            else
                AssertFatalMsgFailed(("%RX64\n", pShwPDPT->a[i].u & X86_PDPE_PG_MASK));
        }
    }
}


/**
 * Clear references to shadowed pages in a 64-bit page directory pointer table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPDPT   The shadow page directory pointer table (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPDPT64Bit(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PX86PDPT pShwPDPT)
{
    for (unsigned i = 0; i < RT_ELEMENTS(pShwPDPT->a); i++)
    {
        Assert((pShwPDPT->a[i].u & (X86_PDPE_LM_MBZ_MASK_NX | UINT64_C(0x7ff0000000000200))) == 0);
        if (pShwPDPT->a[i].n.u1Present)
        {
            PPGMPOOLPAGE pSubPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, pShwPDPT->a[i].u & X86_PDPE_PG_MASK);
            if (pSubPage)
                pgmPoolTrackFreeUser(pPool, pSubPage, pPage->idx, i);
            else
                AssertFatalMsgFailed(("%RX64\n", pShwPDPT->a[i].u & X86_PDPE_PG_MASK));
            /** @todo 64-bit guests: have to ensure that we're not exhausting the dynamic mappings! */
        }
    }
}


/**
 * Clear references to shadowed pages in a 64-bit level 4 page table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPML4    The shadow page directory pointer table (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPML464Bit(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PX86PML4 pShwPML4)
{
    for (unsigned i = 0; i < RT_ELEMENTS(pShwPML4->a); i++)
    {
        Assert((pShwPML4->a[i].u & (X86_PML4E_MBZ_MASK_NX | UINT64_C(0x7ff0000000000200))) == 0);
        if (pShwPML4->a[i].n.u1Present)
        {
            PPGMPOOLPAGE pSubPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, pShwPML4->a[i].u & X86_PDPE_PG_MASK);
            if (pSubPage)
                pgmPoolTrackFreeUser(pPool, pSubPage, pPage->idx, i);
            else
                AssertFatalMsgFailed(("%RX64\n", pShwPML4->a[i].u & X86_PML4E_PG_MASK));
            /** @todo 64-bit guests: have to ensure that we're not exhausting the dynamic mappings! */
        }
    }
}


/**
 * Clear references to shadowed pages in an EPT page directory.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPD      The shadow page directory (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPDEPT(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PEPTPD pShwPD)
{
    for (unsigned i = 0; i < RT_ELEMENTS(pShwPD->a); i++)
    {
        Assert((pShwPD->a[i].u & UINT64_C(0xfff0000000000f80)) == 0);
        if (pShwPD->a[i].n.u1Present)
        {
#ifdef PGM_WITH_LARGE_PAGES
            if (pShwPD->a[i].b.u1Size)
            {
                Log4(("pgmPoolTrackDerefPDEPT: i=%d pde=%RX64 GCPhys=%RX64\n",
                      i, pShwPD->a[i].u & X86_PDE2M_PAE_PG_MASK, pPage->GCPhys));
                pgmPoolTracDerefGCPhys(pPool, pPage, pShwPD->a[i].u & X86_PDE2M_PAE_PG_MASK,
                                       pPage->GCPhys + i * 2 * _1M /* pPage->GCPhys = base address of the memory described by the PD */,
                                       i);
            }
            else
#endif
            {
                PPGMPOOLPAGE pSubPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, pShwPD->a[i].u & EPT_PDE_PG_MASK);
                if (pSubPage)
                    pgmPoolTrackFreeUser(pPool, pSubPage, pPage->idx, i);
                else
                    AssertFatalMsgFailed(("%RX64\n", pShwPD->a[i].u & EPT_PDE_PG_MASK));
            }
            /** @todo 64-bit guests: have to ensure that we're not exhausting the dynamic mappings! */
        }
    }
}


/**
 * Clear references to shadowed pages in an EPT page directory pointer table.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 * @param   pShwPDPT   The shadow page directory pointer table (mapping of the page).
 */
DECLINLINE(void) pgmPoolTrackDerefPDPTEPT(PPGMPOOL pPool, PPGMPOOLPAGE pPage, PEPTPDPT pShwPDPT)
{
    for (unsigned i = 0; i < RT_ELEMENTS(pShwPDPT->a); i++)
    {
        Assert((pShwPDPT->a[i].u & UINT64_C(0xfff0000000000f80)) == 0);
        if (pShwPDPT->a[i].n.u1Present)
        {
            PPGMPOOLPAGE pSubPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, pShwPDPT->a[i].u & EPT_PDPTE_PG_MASK);
            if (pSubPage)
                pgmPoolTrackFreeUser(pPool, pSubPage, pPage->idx, i);
            else
                AssertFatalMsgFailed(("%RX64\n", pShwPDPT->a[i].u & EPT_PDPTE_PG_MASK));
            /** @todo 64-bit guests: have to ensure that we're not exhausting the dynamic mappings! */
        }
    }
}


/**
 * Clears all references made by this page.
 *
 * This includes other shadow pages and GC physical addresses.
 *
 * @param   pPool       The pool.
 * @param   pPage       The page.
 */
static void pgmPoolTrackDeref(PPGMPOOL pPool, PPGMPOOLPAGE pPage)
{
    /*
     * Map the shadow page and take action according to the page kind.
     */
    PVM   pVM   = pPool->CTX_SUFF(pVM);
    void *pvShw = PGMPOOL_PAGE_2_PTR(pVM, pPage);
    switch (pPage->enmKind)
    {
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
        {
            STAM_PROFILE_START(&pPool->StatTrackDerefGCPhys, g);
            void *pvGst;
            int rc = PGM_GCPHYS_2_PTR(pVM, pPage->GCPhys, &pvGst); AssertReleaseRC(rc);
            pgmPoolTrackDerefPT32Bit32Bit(pPool, pPage, (PX86PT)pvShw, (PCX86PT)pvGst);
            PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvGst);
            STAM_PROFILE_STOP(&pPool->StatTrackDerefGCPhys, g);
            break;
        }

        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
        {
            STAM_PROFILE_START(&pPool->StatTrackDerefGCPhys, g);
            void *pvGst;
            int rc = PGM_GCPHYS_2_PTR_EX(pVM, pPage->GCPhys, &pvGst); AssertReleaseRC(rc);
            pgmPoolTrackDerefPTPae32Bit(pPool, pPage, (PPGMSHWPTPAE)pvShw, (PCX86PT)pvGst);
            PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvGst);
            STAM_PROFILE_STOP(&pPool->StatTrackDerefGCPhys, g);
            break;
        }

        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
        {
            STAM_PROFILE_START(&pPool->StatTrackDerefGCPhys, g);
            void *pvGst;
            int rc = PGM_GCPHYS_2_PTR(pVM, pPage->GCPhys, &pvGst); AssertReleaseRC(rc);
            pgmPoolTrackDerefPTPaePae(pPool, pPage, (PPGMSHWPTPAE)pvShw, (PCX86PTPAE)pvGst);
            PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvGst);
            STAM_PROFILE_STOP(&pPool->StatTrackDerefGCPhys, g);
            break;
        }

        case PGMPOOLKIND_32BIT_PT_FOR_PHYS: /* treat it like a 4 MB page */
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
        {
            STAM_PROFILE_START(&pPool->StatTrackDerefGCPhys, g);
            pgmPoolTrackDerefPT32Bit4MB(pPool, pPage, (PX86PT)pvShw);
            STAM_PROFILE_STOP(&pPool->StatTrackDerefGCPhys, g);
            break;
        }

        case PGMPOOLKIND_PAE_PT_FOR_PHYS:   /* treat it like a 2 MB page */
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
        {
            STAM_PROFILE_START(&pPool->StatTrackDerefGCPhys, g);
            pgmPoolTrackDerefPTPaeBig(pPool, pPage, (PPGMSHWPTPAE)pvShw);
            STAM_PROFILE_STOP(&pPool->StatTrackDerefGCPhys, g);
            break;
        }

        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
        case PGMPOOLKIND_PAE_PD_PHYS:
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
            pgmPoolTrackDerefPDPae(pPool, pPage, (PX86PDPAE)pvShw);
            break;

        case PGMPOOLKIND_32BIT_PD_PHYS:
        case PGMPOOLKIND_32BIT_PD:
            pgmPoolTrackDerefPD(pPool, pPage, (PX86PD)pvShw);
            break;

        case PGMPOOLKIND_PAE_PDPT_FOR_32BIT:
        case PGMPOOLKIND_PAE_PDPT:
        case PGMPOOLKIND_PAE_PDPT_PHYS:
            pgmPoolTrackDerefPDPTPae(pPool, pPage, (PX86PDPT)pvShw);
            break;

        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
            pgmPoolTrackDerefPDPT64Bit(pPool, pPage, (PX86PDPT)pvShw);
            break;

        case PGMPOOLKIND_64BIT_PML4:
            pgmPoolTrackDerefPML464Bit(pPool, pPage, (PX86PML4)pvShw);
            break;

        case PGMPOOLKIND_EPT_PT_FOR_PHYS:
            pgmPoolTrackDerefPTEPT(pPool, pPage, (PEPTPT)pvShw);
            break;

        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
            pgmPoolTrackDerefPDEPT(pPool, pPage, (PEPTPD)pvShw);
            break;

        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
            pgmPoolTrackDerefPDPTEPT(pPool, pPage, (PEPTPDPT)pvShw);
            break;

        default:
            AssertFatalMsgFailed(("enmKind=%d\n", pPage->enmKind));
    }

    /* paranoia, clear the shadow page. Remove this laser (i.e. let Alloc and ClearAll do it). */
    STAM_PROFILE_START(&pPool->StatZeroPage, z);
    ASMMemZeroPage(pvShw);
    STAM_PROFILE_STOP(&pPool->StatZeroPage, z);
    pPage->fZeroed = true;
    Assert(!pPage->cPresent);
    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pvShw);
}


/**
 * Flushes a pool page.
 *
 * This moves the page to the free list after removing all user references to it.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @param   pPool       The pool.
 * @param   pPage       The shadow page.
 * @param   fFlush      Flush the TLBS when required (should only be false in very specific use cases!!)
 */
int pgmPoolFlushPage(PPGMPOOL pPool, PPGMPOOLPAGE pPage, bool fFlush)
{
    PVM     pVM = pPool->CTX_SUFF(pVM);
    bool    fFlushRequired = false;

    int rc = VINF_SUCCESS;
    STAM_PROFILE_START(&pPool->StatFlushPage, f);
    LogFlow(("pgmPoolFlushPage: pPage=%p:{.Key=%RHp, .idx=%d, .enmKind=%s, .GCPhys=%RGp}\n",
             pPage, pPage->Core.Key, pPage->idx, pgmPoolPoolKindToStr(pPage->enmKind), pPage->GCPhys));

    /*
     * Reject any attempts at flushing any of the special root pages (shall
     * not happen).
     */
    AssertMsgReturn(pPage->idx >= PGMPOOL_IDX_FIRST,
                    ("pgmPoolFlushPage: special root page, rejected. enmKind=%s idx=%d\n",
                     pgmPoolPoolKindToStr(pPage->enmKind), pPage->idx),
                    VINF_SUCCESS);

    pgmLock(pVM);

    /*
     * Quietly reject any attempts at flushing the currently active shadow CR3 mapping
     */
    if (pgmPoolIsPageLocked(pPage))
    {
        AssertMsg(   pPage->enmKind == PGMPOOLKIND_64BIT_PML4
                  || pPage->enmKind == PGMPOOLKIND_PAE_PDPT
                  || pPage->enmKind == PGMPOOLKIND_PAE_PDPT_FOR_32BIT
                  || pPage->enmKind == PGMPOOLKIND_32BIT_PD
                  || pPage->enmKind == PGMPOOLKIND_PAE_PD_FOR_PAE_PD
                  || pPage->enmKind == PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD
                  || pPage->enmKind == PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD
                  || pPage->enmKind == PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD
                  || pPage->enmKind == PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD
                  || pPage->enmKind == PGMPOOLKIND_ROOT_NESTED,
                  ("Can't free the shadow CR3! (%RHp vs %RHp kind=%d\n", PGMGetHyperCR3(VMMGetCpu(pVM)), pPage->Core.Key, pPage->enmKind));
        Log(("pgmPoolFlushPage: current active shadow CR3, rejected. enmKind=%s idx=%d\n", pgmPoolPoolKindToStr(pPage->enmKind), pPage->idx));
        pgmUnlock(pVM);
        return VINF_SUCCESS;
    }

#if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
    /* Start a subset so we won't run out of mapping space. */
    PVMCPU pVCpu = VMMGetCpu(pVM);
    uint32_t iPrevSubset = PGMRZDynMapPushAutoSubset(pVCpu);
#endif

    /*
     * Mark the page as being in need of an ASMMemZeroPage().
     */
    pPage->fZeroed = false;

#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    if (pPage->fDirty)
        pgmPoolFlushDirtyPage(pVM, pPool, pPage->idxDirtyEntry, false /* do not remove */);
#endif

    /* If there are any users of this table, then we *must* issue a tlb flush on all VCPUs. */
    if (pPage->iUserHead != NIL_PGMPOOL_USER_INDEX)
        fFlushRequired = true;

    /*
     * Clear the page.
     */
    pgmPoolTrackClearPageUsers(pPool, pPage);
    STAM_PROFILE_START(&pPool->StatTrackDeref,a);
    pgmPoolTrackDeref(pPool, pPage);
    STAM_PROFILE_STOP(&pPool->StatTrackDeref,a);

    /*
     * Flush it from the cache.
     */
    pgmPoolCacheFlushPage(pPool, pPage);

#if defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0) || defined(IN_RC)
    /* Heavy stuff done. */
    PGMRZDynMapPopAutoSubset(pVCpu, iPrevSubset);
#endif

    /*
     * Deregistering the monitoring.
     */
    if (pPage->fMonitored)
        rc = pgmPoolMonitorFlush(pPool, pPage);

    /*
     * Free the page.
     */
    Assert(pPage->iNext == NIL_PGMPOOL_IDX);
    pPage->iNext = pPool->iFreeHead;
    pPool->iFreeHead = pPage->idx;
    pPage->enmKind = PGMPOOLKIND_FREE;
    pPage->enmAccess = PGMPOOLACCESS_DONTCARE;
    pPage->GCPhys = NIL_RTGCPHYS;
    pPage->fReusedFlushPending = false;

    pPool->cUsedPages--;

    /* Flush the TLBs of all VCPUs if required. */
    if (    fFlushRequired
        &&  fFlush)
    {
        PGM_INVL_ALL_VCPU_TLBS(pVM);
    }

    pgmUnlock(pVM);
    STAM_PROFILE_STOP(&pPool->StatFlushPage, f);
    return rc;
}


/**
 * Frees a usage of a pool page.
 *
 * The caller is responsible to updating the user table so that it no longer
 * references the shadow page.
 *
 * @param   pPool       The pool.
 * @param   pPage       The shadow page.
 * @param   iUser       The shadow page pool index of the user table.
 *                      NIL_PGMPOOL_IDX for root pages.
 * @param   iUserTable  The index into the user table (shadowed). Ignored if
 *                      root page.
 */
void pgmPoolFreeByPage(PPGMPOOL pPool, PPGMPOOLPAGE pPage, uint16_t iUser, uint32_t iUserTable)
{
    PVM pVM = pPool->CTX_SUFF(pVM);

    STAM_PROFILE_START(&pPool->StatFree, a);
    LogFlow(("pgmPoolFreeByPage: pPage=%p:{.Key=%RHp, .idx=%d, enmKind=%s} iUser=%d iUserTable=%#x\n",
             pPage, pPage->Core.Key, pPage->idx, pgmPoolPoolKindToStr(pPage->enmKind), iUser, iUserTable));
    AssertReturnVoid(pPage->idx >= PGMPOOL_IDX_FIRST); /* paranoia (#6349) */

    pgmLock(pVM);
    if (iUser != NIL_PGMPOOL_IDX)
        pgmPoolTrackFreeUser(pPool, pPage, iUser, iUserTable);
    if (!pPage->fCached)
        pgmPoolFlushPage(pPool, pPage);
    pgmUnlock(pVM);
    STAM_PROFILE_STOP(&pPool->StatFree, a);
}


/**
 * Makes one or more free page free.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PGM_POOL_FLUSHED if the pool was flushed.
 *
 * @param   pPool       The pool.
 * @param   enmKind     Page table kind
 * @param   iUser       The user of the page.
 */
static int pgmPoolMakeMoreFreePages(PPGMPOOL pPool, PGMPOOLKIND enmKind, uint16_t iUser)
{
    PVM pVM = pPool->CTX_SUFF(pVM);
    LogFlow(("pgmPoolMakeMoreFreePages: enmKind=%d iUser=%d\n", enmKind, iUser));
    NOREF(enmKind);

    /*
     * If the pool isn't full grown yet, expand it.
     */
    if (    pPool->cCurPages < pPool->cMaxPages
#if defined(IN_RC)
        /* Hack alert: we can't deal with jumps to ring 3 when called from MapCR3 and allocating pages for PAE PDs. */
        &&  enmKind != PGMPOOLKIND_PAE_PD_FOR_PAE_PD
        &&  (enmKind < PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD || enmKind > PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD)
#endif
        )
    {
        STAM_PROFILE_ADV_SUSPEND(&pPool->StatAlloc, a);
#ifdef IN_RING3
        int rc = PGMR3PoolGrow(pVM);
#else
        int rc = VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_PGM_POOL_GROW, 0);
#endif
        if (RT_FAILURE(rc))
            return rc;
        STAM_PROFILE_ADV_RESUME(&pPool->StatAlloc, a);
        if (pPool->iFreeHead != NIL_PGMPOOL_IDX)
            return VINF_SUCCESS;
    }

    /*
     * Free one cached page.
     */
    return pgmPoolCacheFreeOne(pPool, iUser);
}


/**
 * Allocates a page from the pool.
 *
 * This page may actually be a cached page and not in need of any processing
 * on the callers part.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if a NEW page was allocated.
 * @retval  VINF_PGM_CACHED_PAGE if a CACHED page was returned.
 * @retval  VERR_PGM_POOL_FLUSHED if the pool was flushed.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The GC physical address of the page we're gonna shadow.
 *                      For 4MB and 2MB PD entries, it's the first address the
 *                      shadow PT is covering.
 * @param   enmKind     The kind of mapping.
 * @param   enmAccess   Access type for the mapping (only relevant for big pages)
 * @param   fA20Enabled Whether the A20 gate is enabled or not.
 * @param   iUser       The shadow page pool index of the user table.  Root
 *                      pages should pass NIL_PGMPOOL_IDX.
 * @param   iUserTable  The index into the user table (shadowed).  Ignored for
 *                      root pages (iUser == NIL_PGMPOOL_IDX).
 * @param   fLockPage   Lock the page
 * @param   ppPage      Where to store the pointer to the page. NULL is stored here on failure.
 */
int pgmPoolAlloc(PVM pVM, RTGCPHYS GCPhys, PGMPOOLKIND enmKind, PGMPOOLACCESS enmAccess, bool fA20Enabled,
                 uint16_t iUser, uint32_t iUserTable, bool fLockPage, PPPGMPOOLPAGE ppPage)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    STAM_PROFILE_ADV_START(&pPool->StatAlloc, a);
    LogFlow(("pgmPoolAlloc: GCPhys=%RGp enmKind=%s iUser=%d iUserTable=%#x\n", GCPhys, pgmPoolPoolKindToStr(enmKind), iUser, iUserTable));
    *ppPage = NULL;
    /** @todo CSAM/PGMPrefetchPage messes up here during CSAMR3CheckGates
     *  (TRPMR3SyncIDT) because of FF priority. Try fix that?
     *  Assert(!(pVM->pgm.s.fGlobalSyncFlags & PGM_SYNC_CLEAR_PGM_POOL)); */

    pgmLock(pVM);

    if (pPool->fCacheEnabled)
    {
        int rc2 = pgmPoolCacheAlloc(pPool, GCPhys, enmKind, enmAccess, fA20Enabled, iUser, iUserTable, ppPage);
        if (RT_SUCCESS(rc2))
        {
            if (fLockPage)
                pgmPoolLockPage(pPool, *ppPage);
            pgmUnlock(pVM);
            STAM_PROFILE_ADV_STOP(&pPool->StatAlloc, a);
            LogFlow(("pgmPoolAlloc: cached returns %Rrc *ppPage=%p:{.Key=%RHp, .idx=%d}\n", rc2, *ppPage, (*ppPage)->Core.Key, (*ppPage)->idx));
            return rc2;
        }
    }

    /*
     * Allocate a new one.
     */
    int         rc = VINF_SUCCESS;
    uint16_t    iNew = pPool->iFreeHead;
    if (iNew == NIL_PGMPOOL_IDX)
    {
        rc = pgmPoolMakeMoreFreePages(pPool, enmKind, iUser);
        if (RT_FAILURE(rc))
        {
            pgmUnlock(pVM);
            Log(("pgmPoolAlloc: returns %Rrc (Free)\n", rc));
            STAM_PROFILE_ADV_STOP(&pPool->StatAlloc, a);
            return rc;
        }
        iNew = pPool->iFreeHead;
        AssertReleaseReturn(iNew != NIL_PGMPOOL_IDX, VERR_PGM_POOL_IPE);
    }

    /* unlink the free head */
    PPGMPOOLPAGE pPage = &pPool->aPages[iNew];
    pPool->iFreeHead = pPage->iNext;
    pPage->iNext = NIL_PGMPOOL_IDX;

    /*
     * Initialize it.
     */
    pPool->cUsedPages++;                /* physical handler registration / pgmPoolTrackFlushGCPhysPTsSlow requirement. */
    pPage->enmKind = enmKind;
    pPage->enmAccess = enmAccess;
    pPage->GCPhys = GCPhys;
    pPage->fA20Enabled = fA20Enabled;
    pPage->fSeenNonGlobal = false;      /* Set this to 'true' to disable this feature. */
    pPage->fMonitored = false;
    pPage->fCached = false;
    pPage->fDirty = false;
    pPage->fReusedFlushPending = false;
    pPage->cModifications = 0;
    pPage->iModifiedNext = NIL_PGMPOOL_IDX;
    pPage->iModifiedPrev = NIL_PGMPOOL_IDX;
    pPage->cPresent = 0;
    pPage->iFirstPresent = NIL_PGMPOOL_PRESENT_INDEX;
    pPage->idxDirtyEntry = 0;
    pPage->GCPtrLastAccessHandlerFault = NIL_RTGCPTR;
    pPage->GCPtrLastAccessHandlerRip   = NIL_RTGCPTR;
    pPage->cLastAccessHandler = 0;
    pPage->cLocked = 0;
# ifdef VBOX_STRICT
    pPage->GCPtrDirtyFault = NIL_RTGCPTR;
# endif

    /*
     * Insert into the tracking and cache. If this fails, free the page.
     */
    int rc3 = pgmPoolTrackInsert(pPool, pPage, GCPhys, iUser, iUserTable);
    if (RT_FAILURE(rc3))
    {
        pPool->cUsedPages--;
        pPage->enmKind      = PGMPOOLKIND_FREE;
        pPage->enmAccess    = PGMPOOLACCESS_DONTCARE;
        pPage->GCPhys       = NIL_RTGCPHYS;
        pPage->iNext        = pPool->iFreeHead;
        pPool->iFreeHead    = pPage->idx;
        pgmUnlock(pVM);
        STAM_PROFILE_ADV_STOP(&pPool->StatAlloc, a);
        Log(("pgmPoolAlloc: returns %Rrc (Insert)\n", rc3));
        return rc3;
    }

    /*
     * Commit the allocation, clear the page and return.
     */
#ifdef VBOX_WITH_STATISTICS
    if (pPool->cUsedPages > pPool->cUsedPagesHigh)
        pPool->cUsedPagesHigh = pPool->cUsedPages;
#endif

    if (!pPage->fZeroed)
    {
        STAM_PROFILE_START(&pPool->StatZeroPage, z);
        void *pv = PGMPOOL_PAGE_2_PTR(pVM, pPage);
        ASMMemZeroPage(pv);
        STAM_PROFILE_STOP(&pPool->StatZeroPage, z);
    }

    *ppPage = pPage;
    if (fLockPage)
        pgmPoolLockPage(pPool, pPage);
    pgmUnlock(pVM);
    LogFlow(("pgmPoolAlloc: returns %Rrc *ppPage=%p:{.Key=%RHp, .idx=%d, .fCached=%RTbool, .fMonitored=%RTbool}\n",
             rc, pPage, pPage->Core.Key, pPage->idx, pPage->fCached, pPage->fMonitored));
    STAM_PROFILE_ADV_STOP(&pPool->StatAlloc, a);
    return rc;
}


/**
 * Frees a usage of a pool page.
 *
 * @param   pVM         The cross context VM structure.
 * @param   HCPhys      The HC physical address of the shadow page.
 * @param   iUser       The shadow page pool index of the user table.
 *                      NIL_PGMPOOL_IDX if root page.
 * @param   iUserTable  The index into the user table (shadowed).  Ignored if
 *                      root page.
 */
void pgmPoolFree(PVM pVM, RTHCPHYS HCPhys, uint16_t iUser, uint32_t iUserTable)
{
    LogFlow(("pgmPoolFree: HCPhys=%RHp iUser=%d iUserTable=%#x\n", HCPhys, iUser, iUserTable));
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);
    pgmPoolFreeByPage(pPool, pgmPoolGetPage(pPool, HCPhys), iUser, iUserTable);
}


/**
 * Internal worker for finding a 'in-use' shadow page give by it's physical address.
 *
 * @returns Pointer to the shadow page structure.
 * @param   pPool       The pool.
 * @param   HCPhys      The HC physical address of the shadow page.
 */
PPGMPOOLPAGE pgmPoolGetPage(PPGMPOOL pPool, RTHCPHYS HCPhys)
{
    PGM_LOCK_ASSERT_OWNER(pPool->CTX_SUFF(pVM));

    /*
     * Look up the page.
     */
    PPGMPOOLPAGE pPage = (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, HCPhys & X86_PTE_PAE_PG_MASK);

    AssertFatalMsg(pPage && pPage->enmKind != PGMPOOLKIND_FREE, ("HCPhys=%RHp pPage=%p idx=%d\n", HCPhys, pPage, (pPage) ? pPage->idx : 0));
    return pPage;
}


/**
 * Internal worker for finding a page for debugging purposes, no assertions.
 *
 * @returns Pointer to the shadow page structure.  NULL on if not found.
 * @param   pPool       The pool.
 * @param   HCPhys      The HC physical address of the shadow page.
 */
PPGMPOOLPAGE pgmPoolQueryPageForDbg(PPGMPOOL pPool, RTHCPHYS HCPhys)
{
    PGM_LOCK_ASSERT_OWNER(pPool->CTX_SUFF(pVM));
    return (PPGMPOOLPAGE)RTAvloHCPhysGet(&pPool->HCPhysTree, HCPhys & X86_PTE_PAE_PG_MASK);
}

#ifdef IN_RING3 /* currently only used in ring 3; save some space in the R0 & GC modules (left it here as we might need it elsewhere later on) */

/**
 * Flush the specified page if present
 *
 * @param   pVM     The cross context VM structure.
 * @param   GCPhys  Guest physical address of the page to flush
 */
void pgmPoolFlushPageByGCPhys(PVM pVM, RTGCPHYS GCPhys)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

    VM_ASSERT_EMT(pVM);

    /*
     * Look up the GCPhys in the hash.
     */
    GCPhys = GCPhys & ~(RTGCPHYS)PAGE_OFFSET_MASK;
    unsigned i = pPool->aiHash[PGMPOOL_HASH(GCPhys)];
    if (i == NIL_PGMPOOL_IDX)
        return;

    do
    {
        PPGMPOOLPAGE pPage = &pPool->aPages[i];
        if (pPage->GCPhys - GCPhys < PAGE_SIZE)
        {
            switch (pPage->enmKind)
            {
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
                case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
                case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
                case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
                case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
                case PGMPOOLKIND_64BIT_PML4:
                case PGMPOOLKIND_32BIT_PD:
                case PGMPOOLKIND_PAE_PDPT:
                {
                    Log(("PGMPoolFlushPage: found pgm pool pages for %RGp\n", GCPhys));
#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
                    if (pPage->fDirty)
                        STAM_COUNTER_INC(&pPool->StatForceFlushDirtyPage);
                    else
#endif
                        STAM_COUNTER_INC(&pPool->StatForceFlushPage);
                    Assert(!pgmPoolIsPageLocked(pPage));
                    pgmPoolMonitorChainFlush(pPool, pPage);
                    return;
                }

                /* ignore, no monitoring. */
                case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
                case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
                case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
                case PGMPOOLKIND_PAE_PT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
                case PGMPOOLKIND_EPT_PD_FOR_PHYS:
                case PGMPOOLKIND_EPT_PT_FOR_PHYS:
                case PGMPOOLKIND_ROOT_NESTED:
                case PGMPOOLKIND_PAE_PD_PHYS:
                case PGMPOOLKIND_PAE_PDPT_PHYS:
                case PGMPOOLKIND_32BIT_PD_PHYS:
                case PGMPOOLKIND_PAE_PDPT_FOR_32BIT:
                    break;

                default:
                    AssertFatalMsgFailed(("enmKind=%d idx=%d\n", pPage->enmKind, pPage->idx));
            }
        }

        /* next */
        i = pPage->iNext;
    } while (i != NIL_PGMPOOL_IDX);
    return;
}

#endif /* IN_RING3 */
#ifdef IN_RING3

/**
 * Reset CPU on hot plugging.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu              The cross context virtual CPU structure.
 */
void pgmR3PoolResetUnpluggedCpu(PVM pVM, PVMCPU pVCpu)
{
    pgmR3ExitShadowModeBeforePoolFlush(pVCpu);

    pgmR3ReEnterShadowModeAfterPoolFlush(pVM, pVCpu);
    VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
    VMCPU_FF_SET(pVCpu, VMCPU_FF_TLB_FLUSH);
}


/**
 * Flushes the entire cache.
 *
 * It will assert a global CR3 flush (FF) and assumes the caller is aware of
 * this and execute this CR3 flush.
 *
 * @param   pVM         The cross context VM structure.
 */
void pgmR3PoolReset(PVM pVM)
{
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

    PGM_LOCK_ASSERT_OWNER(pVM);
    STAM_PROFILE_START(&pPool->StatR3Reset, a);
    LogFlow(("pgmR3PoolReset:\n"));

    /*
     * If there are no pages in the pool, there is nothing to do.
     */
    if (pPool->cCurPages <= PGMPOOL_IDX_FIRST)
    {
        STAM_PROFILE_STOP(&pPool->StatR3Reset, a);
        return;
    }

    /*
     * Exit the shadow mode since we're going to clear everything,
     * including the root page.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
        pgmR3ExitShadowModeBeforePoolFlush(&pVM->aCpus[i]);

    /*
     * Nuke the free list and reinsert all pages into it.
     */
    for (unsigned i = pPool->cCurPages - 1; i >= PGMPOOL_IDX_FIRST; i--)
    {
        PPGMPOOLPAGE pPage = &pPool->aPages[i];

        Assert(pPage->Core.Key == MMPage2Phys(pVM, pPage->pvPageR3));
        if (pPage->fMonitored)
            pgmPoolMonitorFlush(pPool, pPage);
        pPage->iModifiedNext = NIL_PGMPOOL_IDX;
        pPage->iModifiedPrev = NIL_PGMPOOL_IDX;
        pPage->iMonitoredNext = NIL_PGMPOOL_IDX;
        pPage->iMonitoredPrev = NIL_PGMPOOL_IDX;
        pPage->GCPhys     = NIL_RTGCPHYS;
        pPage->enmKind    = PGMPOOLKIND_FREE;
        pPage->enmAccess  = PGMPOOLACCESS_DONTCARE;
        Assert(pPage->idx == i);
        pPage->iNext      = i + 1;
        pPage->fA20Enabled = true;
        pPage->fZeroed    = false;       /* This could probably be optimized, but better safe than sorry. */
        pPage->fSeenNonGlobal = false;
        pPage->fMonitored = false;
        pPage->fDirty     = false;
        pPage->fCached    = false;
        pPage->fReusedFlushPending = false;
        pPage->iUserHead  = NIL_PGMPOOL_USER_INDEX;
        pPage->cPresent = 0;
        pPage->iFirstPresent = NIL_PGMPOOL_PRESENT_INDEX;
        pPage->cModifications = 0;
        pPage->iAgeNext   = NIL_PGMPOOL_IDX;
        pPage->iAgePrev   = NIL_PGMPOOL_IDX;
        pPage->idxDirtyEntry = 0;
        pPage->GCPtrLastAccessHandlerRip = NIL_RTGCPTR;
        pPage->GCPtrLastAccessHandlerFault = NIL_RTGCPTR;
        pPage->cLastAccessHandler = 0;
        pPage->cLocked    = 0;
#ifdef VBOX_STRICT
        pPage->GCPtrDirtyFault = NIL_RTGCPTR;
#endif
    }
    pPool->aPages[pPool->cCurPages - 1].iNext = NIL_PGMPOOL_IDX;
    pPool->iFreeHead = PGMPOOL_IDX_FIRST;
    pPool->cUsedPages = 0;

    /*
     * Zap and reinitialize the user records.
     */
    pPool->cPresent = 0;
    pPool->iUserFreeHead = 0;
    PPGMPOOLUSER paUsers = pPool->CTX_SUFF(paUsers);
    const unsigned cMaxUsers = pPool->cMaxUsers;
    for (unsigned i = 0; i < cMaxUsers; i++)
    {
        paUsers[i].iNext = i + 1;
        paUsers[i].iUser = NIL_PGMPOOL_IDX;
        paUsers[i].iUserTable = 0xfffffffe;
    }
    paUsers[cMaxUsers - 1].iNext = NIL_PGMPOOL_USER_INDEX;

    /*
     * Clear all the GCPhys links and rebuild the phys ext free list.
     */
    for (PPGMRAMRANGE pRam = pVM->pgm.s.CTX_SUFF(pRamRangesX);
         pRam;
         pRam = pRam->CTX_SUFF(pNext))
    {
        unsigned iPage = pRam->cb >> PAGE_SHIFT;
        while (iPage-- > 0)
            PGM_PAGE_SET_TRACKING(pVM, &pRam->aPages[iPage], 0);
    }

    pPool->iPhysExtFreeHead = 0;
    PPGMPOOLPHYSEXT paPhysExts = pPool->CTX_SUFF(paPhysExts);
    const unsigned cMaxPhysExts = pPool->cMaxPhysExts;
    for (unsigned i = 0; i < cMaxPhysExts; i++)
    {
        paPhysExts[i].iNext = i + 1;
        paPhysExts[i].aidx[0] = NIL_PGMPOOL_IDX;
        paPhysExts[i].apte[0] = NIL_PGMPOOL_PHYSEXT_IDX_PTE;
        paPhysExts[i].aidx[1] = NIL_PGMPOOL_IDX;
        paPhysExts[i].apte[1] = NIL_PGMPOOL_PHYSEXT_IDX_PTE;
        paPhysExts[i].aidx[2] = NIL_PGMPOOL_IDX;
        paPhysExts[i].apte[2] = NIL_PGMPOOL_PHYSEXT_IDX_PTE;
    }
    paPhysExts[cMaxPhysExts - 1].iNext = NIL_PGMPOOL_PHYSEXT_INDEX;

    /*
     * Just zap the modified list.
     */
    pPool->cModifiedPages = 0;
    pPool->iModifiedHead = NIL_PGMPOOL_IDX;

    /*
     * Clear the GCPhys hash and the age list.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(pPool->aiHash); i++)
        pPool->aiHash[i] = NIL_PGMPOOL_IDX;
    pPool->iAgeHead = NIL_PGMPOOL_IDX;
    pPool->iAgeTail = NIL_PGMPOOL_IDX;

#ifdef PGMPOOL_WITH_OPTIMIZED_DIRTY_PT
    /* Clear all dirty pages. */
    pPool->idxFreeDirtyPage = 0;
    pPool->cDirtyPages      = 0;
    for (unsigned i = 0; i < RT_ELEMENTS(pPool->aDirtyPages); i++)
        pPool->aDirtyPages[i].uIdx = NIL_PGMPOOL_IDX;
#endif

    /*
     * Reinsert active pages into the hash and ensure monitoring chains are correct.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        /*
         * Re-enter the shadowing mode and assert Sync CR3 FF.
         */
        PVMCPU pVCpu = &pVM->aCpus[i];
        pgmR3ReEnterShadowModeAfterPoolFlush(pVM, pVCpu);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3);
        VMCPU_FF_SET(pVCpu, VMCPU_FF_TLB_FLUSH);
    }

    STAM_PROFILE_STOP(&pPool->StatR3Reset, a);
}

#endif /* IN_RING3 */

#if defined(LOG_ENABLED) || defined(VBOX_STRICT)
/**
 * Stringifies a PGMPOOLKIND value.
 */
static const char *pgmPoolPoolKindToStr(uint8_t enmKind)
{
    switch ((PGMPOOLKIND)enmKind)
    {
        case PGMPOOLKIND_INVALID:
            return "PGMPOOLKIND_INVALID";
        case PGMPOOLKIND_FREE:
            return "PGMPOOLKIND_FREE";
        case PGMPOOLKIND_32BIT_PT_FOR_PHYS:
            return "PGMPOOLKIND_32BIT_PT_FOR_PHYS";
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT:
            return "PGMPOOLKIND_32BIT_PT_FOR_32BIT_PT";
        case PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB:
            return "PGMPOOLKIND_32BIT_PT_FOR_32BIT_4MB";
        case PGMPOOLKIND_PAE_PT_FOR_PHYS:
            return "PGMPOOLKIND_PAE_PT_FOR_PHYS";
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_PT:
            return "PGMPOOLKIND_PAE_PT_FOR_32BIT_PT";
        case PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB:
            return "PGMPOOLKIND_PAE_PT_FOR_32BIT_4MB";
        case PGMPOOLKIND_PAE_PT_FOR_PAE_PT:
            return "PGMPOOLKIND_PAE_PT_FOR_PAE_PT";
        case PGMPOOLKIND_PAE_PT_FOR_PAE_2MB:
            return "PGMPOOLKIND_PAE_PT_FOR_PAE_2MB";
        case PGMPOOLKIND_32BIT_PD:
            return "PGMPOOLKIND_32BIT_PD";
        case PGMPOOLKIND_32BIT_PD_PHYS:
            return "PGMPOOLKIND_32BIT_PD_PHYS";
        case PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD:
            return "PGMPOOLKIND_PAE_PD0_FOR_32BIT_PD";
        case PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD:
            return "PGMPOOLKIND_PAE_PD1_FOR_32BIT_PD";
        case PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD:
            return "PGMPOOLKIND_PAE_PD2_FOR_32BIT_PD";
        case PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD:
            return "PGMPOOLKIND_PAE_PD3_FOR_32BIT_PD";
        case PGMPOOLKIND_PAE_PD_FOR_PAE_PD:
            return "PGMPOOLKIND_PAE_PD_FOR_PAE_PD";
        case PGMPOOLKIND_PAE_PD_PHYS:
            return "PGMPOOLKIND_PAE_PD_PHYS";
        case PGMPOOLKIND_PAE_PDPT_FOR_32BIT:
            return "PGMPOOLKIND_PAE_PDPT_FOR_32BIT";
        case PGMPOOLKIND_PAE_PDPT:
            return "PGMPOOLKIND_PAE_PDPT";
        case PGMPOOLKIND_PAE_PDPT_PHYS:
            return "PGMPOOLKIND_PAE_PDPT_PHYS";
        case PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT:
            return "PGMPOOLKIND_64BIT_PDPT_FOR_64BIT_PDPT";
        case PGMPOOLKIND_64BIT_PDPT_FOR_PHYS:
            return "PGMPOOLKIND_64BIT_PDPT_FOR_PHYS";
        case PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD:
            return "PGMPOOLKIND_64BIT_PD_FOR_64BIT_PD";
        case PGMPOOLKIND_64BIT_PD_FOR_PHYS:
            return "PGMPOOLKIND_64BIT_PD_FOR_PHYS";
        case PGMPOOLKIND_64BIT_PML4:
            return "PGMPOOLKIND_64BIT_PML4";
        case PGMPOOLKIND_EPT_PDPT_FOR_PHYS:
            return "PGMPOOLKIND_EPT_PDPT_FOR_PHYS";
        case PGMPOOLKIND_EPT_PD_FOR_PHYS:
            return "PGMPOOLKIND_EPT_PD_FOR_PHYS";
        case PGMPOOLKIND_EPT_PT_FOR_PHYS:
            return "PGMPOOLKIND_EPT_PT_FOR_PHYS";
        case PGMPOOLKIND_ROOT_NESTED:
            return "PGMPOOLKIND_ROOT_NESTED";
    }
    return "Unknown kind!";
}
#endif /* LOG_ENABLED || VBOX_STRICT */


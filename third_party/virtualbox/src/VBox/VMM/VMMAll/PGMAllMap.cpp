/* $Id: PGMAllMap.cpp $ */
/** @file
 * PGM - Page Manager and Monitor - All context code.
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
#define LOG_GROUP LOG_GROUP_PGM
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/em.h>
#include "PGMInternal.h"
#include <VBox/vmm/vm.h>
#include "PGMInline.h"
#include <VBox/err.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>


/**
 * Maps a range of physical pages at a given virtual address
 * in the guest context.
 *
 * The GC virtual address range must be within an existing mapping.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       Where to map the page(s). Must be page aligned.
 * @param   HCPhys      Start of the range of physical pages. Must be page aligned.
 * @param   cbPages     Number of bytes to map. Must be page aligned.
 * @param   fFlags      Page flags (X86_PTE_*).
 */
VMMDECL(int) PGMMap(PVM pVM, RTGCUINTPTR GCPtr, RTHCPHYS HCPhys, uint32_t cbPages, unsigned fFlags)
{
    AssertMsg(pVM->pgm.s.offVM, ("Bad init order\n"));

    /*
     * Validate input.
     */
    AssertMsg(RT_ALIGN_T(GCPtr, PAGE_SIZE, RTGCUINTPTR) == GCPtr, ("Invalid alignment GCPtr=%#x\n", GCPtr));
    AssertMsg(cbPages > 0 && RT_ALIGN_32(cbPages, PAGE_SIZE) == cbPages, ("Invalid cbPages=%#x\n",  cbPages));
    AssertMsg(!(fFlags & X86_PDE_PG_MASK), ("Invalid flags %#x\n", fFlags));

    /* hypervisor defaults */
    if (!fFlags)
        fFlags = X86_PTE_P | X86_PTE_A | X86_PTE_D;

    /*
     * Find the mapping.
     */
    PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings);
    while (pCur)
    {
        if (GCPtr - pCur->GCPtr < pCur->cb)
        {
            if (GCPtr + cbPages - 1 > pCur->GCPtrLast)
            {
                AssertMsgFailed(("Invalid range!!\n"));
                return VERR_INVALID_PARAMETER;
            }

            /*
             * Setup PTE.
             */
            X86PTEPAE Pte;
            Pte.u = fFlags | (HCPhys & X86_PTE_PAE_PG_MASK);

            /*
             * Update the page tables.
             */
            for (;;)
            {
                RTGCUINTPTR     off = GCPtr - pCur->GCPtr;
                const unsigned  iPT = off >> X86_PD_SHIFT;
                const unsigned  iPageNo = (off >> PAGE_SHIFT) & X86_PT_MASK;

                /* 32-bit */
                pCur->aPTs[iPT].CTX_SUFF(pPT)->a[iPageNo].u = (uint32_t)Pte.u;      /* ASSUMES HCPhys < 4GB and/or that we're never gonna do 32-bit on a PAE host! */

                /* pae */
                PGMSHWPTEPAE_SET(pCur->aPTs[iPT].CTX_SUFF(paPaePTs)[iPageNo / 512].a[iPageNo % 512], Pte.u);

                /* next */
                cbPages -= PAGE_SIZE;
                if (!cbPages)
                    break;
                GCPtr += PAGE_SIZE;
                Pte.u += PAGE_SIZE;
            }

            return VINF_SUCCESS;
        }

        /* next */
        pCur = pCur->CTX_SUFF(pNext);
    }

    AssertMsgFailed(("GCPtr=%#x was not found in any mapping ranges!\n",  GCPtr));
    return VERR_INVALID_PARAMETER;
}


/**
 * Sets (replaces) the page flags for a range of pages in a mapping.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       Virtual address of the first page in the range.
 * @param   cb          Size (in bytes) of the range to apply the modification to.
 * @param   fFlags      Page flags X86_PTE_*, excluding the page mask of course.
 */
VMMDECL(int) PGMMapSetPage(PVM pVM, RTGCPTR GCPtr, uint64_t cb, uint64_t fFlags)
{
    return PGMMapModifyPage(pVM, GCPtr, cb, fFlags, 0);
}


/**
 * Modify page flags for a range of pages in a mapping.
 *
 * The existing flags are ANDed with the fMask and ORed with the fFlags.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       Virtual address of the first page in the range.
 * @param   cb          Size (in bytes) of the range to apply the modification to.
 * @param   fFlags      The OR  mask - page flags X86_PTE_*, excluding the page mask of course.
 * @param   fMask       The AND mask - page flags X86_PTE_*, excluding the page mask of course.
 */
VMMDECL(int)  PGMMapModifyPage(PVM pVM, RTGCPTR GCPtr, size_t cb, uint64_t fFlags, uint64_t fMask)
{
    /*
     * Validate input.
     */
    AssertMsg(!(fFlags & (X86_PTE_PAE_PG_MASK | X86_PTE_PAE_MBZ_MASK_NX)), ("fFlags=%#x\n", fFlags));
    Assert(cb);

    /*
     * Align the input.
     */
    cb     += (RTGCUINTPTR)GCPtr & PAGE_OFFSET_MASK;
    cb      = RT_ALIGN_Z(cb, PAGE_SIZE);
    GCPtr   = (RTGCPTR)((RTGCUINTPTR)GCPtr & PAGE_BASE_GC_MASK);

    /*
     * Find the mapping.
     */
    PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings);
    while (pCur)
    {
        RTGCUINTPTR off = (RTGCUINTPTR)GCPtr - (RTGCUINTPTR)pCur->GCPtr;
        if (off < pCur->cb)
        {
            AssertMsgReturn(off + cb <= pCur->cb,
                            ("Invalid page range %#x LB%#x. mapping '%s' %#x to %#x\n",
                             GCPtr, cb, pCur->pszDesc, pCur->GCPtr, pCur->GCPtrLast),
                            VERR_INVALID_PARAMETER);

            /*
             * Perform the requested operation.
             */
            while (cb > 0)
            {
                unsigned iPT  = off >> X86_PD_SHIFT;
                unsigned iPTE = (off >> PAGE_SHIFT) & X86_PT_MASK;
                while (cb > 0 && iPTE < RT_ELEMENTS(pCur->aPTs[iPT].CTX_SUFF(pPT)->a))
                {
                    /* 32-Bit */
                    pCur->aPTs[iPT].CTX_SUFF(pPT)->a[iPTE].u &= fMask | X86_PTE_PG_MASK;
                    pCur->aPTs[iPT].CTX_SUFF(pPT)->a[iPTE].u |= fFlags & ~X86_PTE_PG_MASK;

                    /* PAE */
                    PPGMSHWPTEPAE pPtePae = &pCur->aPTs[iPT].CTX_SUFF(paPaePTs)[iPTE / 512].a[iPTE % 512];
                    PGMSHWPTEPAE_SET(*pPtePae,
                                       (  PGMSHWPTEPAE_GET_U(*pPtePae)
                                        & (fMask | X86_PTE_PAE_PG_MASK))
                                     | (fFlags & ~(X86_PTE_PAE_PG_MASK | X86_PTE_PAE_MBZ_MASK_NX)));

                    /* invalidate tls */
                    PGM_INVL_PG(VMMGetCpu(pVM), (RTGCUINTPTR)pCur->GCPtr + off);

                    /* next */
                    iPTE++;
                    cb -= PAGE_SIZE;
                    off += PAGE_SIZE;
                }
            }

            return VINF_SUCCESS;
        }
        /* next */
        pCur = pCur->CTX_SUFF(pNext);
    }

    AssertMsgFailed(("Page range %#x LB%#x not found\n", GCPtr, cb));
    return VERR_INVALID_PARAMETER;
}


/**
 * Get information about a page in a mapping.
 *
 * This differs from PGMShwGetPage and PGMGstGetPage in that it only consults
 * the page table to calculate the flags.
 *
 * @returns VINF_SUCCESS, VERR_PAGE_NOT_PRESENT or VERR_NOT_FOUND.
 * @param   pVM                 The cross context VM structure.
 * @param   GCPtr               The page address.
 * @param   pfFlags             Where to return the flags.  Optional.
 * @param   pHCPhys             Where to return the address.  Optional.
 */
VMMDECL(int) PGMMapGetPage(PVM pVM, RTGCPTR GCPtr, uint64_t *pfFlags, PRTHCPHYS pHCPhys)
{
    /*
     * Find the mapping.
     */
    GCPtr &= PAGE_BASE_GC_MASK;
    PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings);
    while (pCur)
    {
        RTGCUINTPTR off = (RTGCUINTPTR)GCPtr - (RTGCUINTPTR)pCur->GCPtr;
        if (off < pCur->cb)
        {
            /*
             * Dig out the information.
             */
            int             rc      = VINF_SUCCESS;
            unsigned        iPT     = off >> X86_PD_SHIFT;
            unsigned        iPTE    = (off >> PAGE_SHIFT) & X86_PT_MASK;
            PCPGMSHWPTEPAE  pPtePae = &pCur->aPTs[iPT].CTX_SUFF(paPaePTs)[iPTE / 512].a[iPTE % 512];
            if (PGMSHWPTEPAE_IS_P(*pPtePae))
            {
                if (pfFlags)
                    *pfFlags = PGMSHWPTEPAE_GET_U(*pPtePae) & ~X86_PTE_PAE_PG_MASK;
                if (pHCPhys)
                    *pHCPhys = PGMSHWPTEPAE_GET_HCPHYS(*pPtePae);
            }
            else
                rc = VERR_PAGE_NOT_PRESENT;
            return rc;
        }
        /* next */
        pCur = pCur->CTX_SUFF(pNext);
    }

    return VERR_NOT_FOUND;
}

#ifndef PGM_WITHOUT_MAPPINGS

/**
 * Sets all PDEs involved with the mapping in the shadow page table.
 *
 * Ignored if mappings are disabled (i.e. if HM is enabled).
 *
 * @param   pVM         The cross context VM structure.
 * @param   pMap        Pointer to the mapping in question.
 * @param   iNewPDE     The index of the 32-bit PDE corresponding to the base of the mapping.
 */
void pgmMapSetShadowPDEs(PVM pVM, PPGMMAPPING pMap, unsigned iNewPDE)
{
    Log4(("pgmMapSetShadowPDEs new pde %x (mappings enabled %d)\n", iNewPDE, pgmMapAreMappingsEnabled(pVM)));

    if (!pgmMapAreMappingsEnabled(pVM))
        return;

    /* This only applies to raw mode where we only support 1 VCPU. */
    PVMCPU pVCpu = VMMGetCpu0(pVM);
    if (!pVCpu->pgm.s.CTX_SUFF(pShwPageCR3))
        return;    /* too early */

    PGMMODE enmShadowMode = PGMGetShadowMode(pVCpu);
    Assert(enmShadowMode <= PGMMODE_PAE_NX);

    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

    /*
     * Insert the page tables into the shadow page directories.
     */
    unsigned i = pMap->cPTs;
    iNewPDE += i;
    while (i-- > 0)
    {
        iNewPDE--;

        switch (enmShadowMode)
        {
            case PGMMODE_32_BIT:
            {
                PX86PD pShw32BitPd = pgmShwGet32BitPDPtr(pVCpu);
                AssertFatal(pShw32BitPd);

                /* Free any previous user, unless it's us. */
                Assert(   (pShw32BitPd->a[iNewPDE].u & (X86_PDE_P | PGM_PDFLAGS_MAPPING)) != (X86_PDE_P | PGM_PDFLAGS_MAPPING)
                       || (pShw32BitPd->a[iNewPDE].u & X86_PDE_PG_MASK) == pMap->aPTs[i].HCPhysPT);
                if (    pShw32BitPd->a[iNewPDE].n.u1Present
                    &&  !(pShw32BitPd->a[iNewPDE].u & PGM_PDFLAGS_MAPPING))
                    pgmPoolFree(pVM, pShw32BitPd->a[iNewPDE].u & X86_PDE_PG_MASK, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3)->idx, iNewPDE);

                /* Default mapping page directory flags are read/write and supervisor; individual page attributes determine the final flags. */
                pShw32BitPd->a[iNewPDE].u = PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US
                                          | (uint32_t)pMap->aPTs[i].HCPhysPT;
                PGM_DYNMAP_UNUSED_HINT_VM(pVM, pShw32BitPd);
                break;
            }

            case PGMMODE_PAE:
            case PGMMODE_PAE_NX:
            {
                const uint32_t  iPdPt     = iNewPDE / 256;
                unsigned        iPaePde   = iNewPDE * 2 % 512;
                PX86PDPT        pShwPdpt  = pgmShwGetPaePDPTPtr(pVCpu);
                Assert(pShwPdpt);

                /*
                 * Get the shadow PD.
                 * If no PD, sync it (PAE guest) or fake (not present or 32-bit guest).
                 * Note! The RW, US and A bits are reserved for PAE PDPTEs. Setting the
                 *       accessed bit causes invalid VT-x guest state errors.
                 */
                PX86PDPAE pShwPaePd = pgmShwGetPaePDPtr(pVCpu, iPdPt << X86_PDPT_SHIFT);
                if (!pShwPaePd)
                {
                    X86PDPE     GstPdpe;
                    if (PGMGetGuestMode(pVCpu) < PGMMODE_PAE)
                        GstPdpe.u = X86_PDPE_P;
                    else
                    {
                        PX86PDPE pGstPdpe = pgmGstGetPaePDPEPtr(pVCpu, iPdPt << X86_PDPT_SHIFT);
                        if (pGstPdpe)
                            GstPdpe = *pGstPdpe;
                        else
                            GstPdpe.u = X86_PDPE_P;
                    }
                    int rc = pgmShwSyncPaePDPtr(pVCpu, iPdPt << X86_PDPT_SHIFT, GstPdpe.u, &pShwPaePd);
                    AssertFatalRC(rc);
                }
                Assert(pShwPaePd);

                /*
                 * Mark the page as locked; disallow flushing.
                 */
                PPGMPOOLPAGE    pPoolPagePd = pgmPoolGetPage(pPool, pShwPdpt->a[iPdPt].u & X86_PDPE_PG_MASK);
                AssertFatal(pPoolPagePd);
                if (!pgmPoolIsPageLocked(pPoolPagePd))
                    pgmPoolLockPage(pPool, pPoolPagePd);
# ifdef VBOX_STRICT
                else if (pShwPaePd->a[iPaePde].u & PGM_PDFLAGS_MAPPING)
                {
                    Assert(PGMGetGuestMode(pVCpu) >= PGMMODE_PAE); /** @todo We may hit this during reset, will fix later. */
                    AssertFatalMsg(   (pShwPaePd->a[iPaePde].u & X86_PDE_PAE_PG_MASK) == pMap->aPTs[i].HCPhysPaePT0
                                   || !PGMMODE_WITH_PAGING(PGMGetGuestMode(pVCpu)),
                                   ("%RX64 vs %RX64\n", pShwPaePd->a[iPaePde+1].u & X86_PDE_PAE_PG_MASK, pMap->aPTs[i].HCPhysPaePT0));
                    Assert(pShwPaePd->a[iPaePde+1].u & PGM_PDFLAGS_MAPPING);
                    AssertFatalMsg(   (pShwPaePd->a[iPaePde+1].u & X86_PDE_PAE_PG_MASK) == pMap->aPTs[i].HCPhysPaePT1
                                   || !PGMMODE_WITH_PAGING(PGMGetGuestMode(pVCpu)),
                                   ("%RX64 vs %RX64\n", pShwPaePd->a[iPaePde+1].u & X86_PDE_PAE_PG_MASK, pMap->aPTs[i].HCPhysPaePT1));
                }
# endif

                /*
                 * Insert our first PT, freeing anything we might be replacing unless it's a mapping (i.e. us).
                 */
                Assert(   (pShwPaePd->a[iPaePde].u & (X86_PDE_P | PGM_PDFLAGS_MAPPING)) != (X86_PDE_P | PGM_PDFLAGS_MAPPING)
                       || (pShwPaePd->a[iPaePde].u & X86_PDE_PAE_PG_MASK) == pMap->aPTs[i].HCPhysPaePT0);
                if (    pShwPaePd->a[iPaePde].n.u1Present
                    &&  !(pShwPaePd->a[iPaePde].u & PGM_PDFLAGS_MAPPING))
                {
                    Assert(!(pShwPaePd->a[iPaePde].u & PGM_PDFLAGS_MAPPING));
                    pgmPoolFree(pVM, pShwPaePd->a[iPaePde].u & X86_PDE_PAE_PG_MASK, pPoolPagePd->idx, iPaePde);
                }
                pShwPaePd->a[iPaePde].u = PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US
                                        | pMap->aPTs[i].HCPhysPaePT0;

                /* 2nd 2 MB PDE of the 4 MB region, same as above. */
                iPaePde++;
                AssertFatal(iPaePde < 512);
                Assert(   (pShwPaePd->a[iPaePde].u & (X86_PDE_P | PGM_PDFLAGS_MAPPING)) != (X86_PDE_P | PGM_PDFLAGS_MAPPING)
                       || (pShwPaePd->a[iPaePde].u & X86_PDE_PAE_PG_MASK) == pMap->aPTs[i].HCPhysPaePT1);
                if (    pShwPaePd->a[iPaePde].n.u1Present
                    &&  !(pShwPaePd->a[iPaePde].u & PGM_PDFLAGS_MAPPING))
                    pgmPoolFree(pVM, pShwPaePd->a[iPaePde].u & X86_PDE_PG_MASK, pPoolPagePd->idx, iPaePde);
                pShwPaePd->a[iPaePde].u = PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US
                                        | pMap->aPTs[i].HCPhysPaePT1;

                /*
                 * Set the PGM_PDFLAGS_MAPPING flag in the page directory pointer entry. (legacy PAE guest mode)
                 */
                pShwPdpt->a[iPdPt].u |= PGM_PLXFLAGS_MAPPING;

                PGM_DYNMAP_UNUSED_HINT_VM(pVM, pShwPaePd);
                PGM_DYNMAP_UNUSED_HINT_VM(pVM, pShwPdpt);
                break;
            }

            default:
                AssertFailed();
                break;
        }
    }
}


/**
 * Clears all PDEs involved with the mapping in the shadow page table.
 *
 * Ignored if mappings are disabled (i.e. if HM is enabled).
 *
 * @param   pVM             The cross context VM structure.
 * @param   pShwPageCR3     CR3 root page
 * @param   pMap            Pointer to the mapping in question.
 * @param   iOldPDE         The index of the 32-bit PDE corresponding to the base of the mapping.
 * @param   fDeactivateCR3  Set if it's pgmMapDeactivateCR3 calling.
 */
void pgmMapClearShadowPDEs(PVM pVM, PPGMPOOLPAGE pShwPageCR3, PPGMMAPPING pMap, unsigned iOldPDE, bool fDeactivateCR3)
{
    Log(("pgmMapClearShadowPDEs: old pde %x (cPTs=%x) (mappings enabled %d) fDeactivateCR3=%RTbool\n", iOldPDE, pMap->cPTs, pgmMapAreMappingsEnabled(pVM), fDeactivateCR3));

    /*
     * Skip this if it doesn't apply.
     */
    if (!pgmMapAreMappingsEnabled(pVM))
        return;

    Assert(pShwPageCR3);

    /* This only applies to raw mode where we only support 1 VCPU. */
    PVMCPU pVCpu = VMMGetCpu0(pVM);
# ifdef IN_RC
    Assert(pShwPageCR3 != pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
# endif

    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

    PX86PDPT pCurrentShwPdpt = NULL;
    if (    PGMGetGuestMode(pVCpu) >= PGMMODE_PAE
        &&  pShwPageCR3 != pVCpu->pgm.s.CTX_SUFF(pShwPageCR3))
        pCurrentShwPdpt = pgmShwGetPaePDPTPtr(pVCpu);

    unsigned i = pMap->cPTs;
    PGMMODE  enmShadowMode = PGMGetShadowMode(pVCpu);

    iOldPDE += i;
    while (i-- > 0)
    {
        iOldPDE--;

        switch(enmShadowMode)
        {
            case PGMMODE_32_BIT:
            {
                PX86PD          pShw32BitPd = (PX86PD)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPageCR3);
                AssertFatal(pShw32BitPd);

                Assert(!pShw32BitPd->a[iOldPDE].n.u1Present || (pShw32BitPd->a[iOldPDE].u & PGM_PDFLAGS_MAPPING));
                pShw32BitPd->a[iOldPDE].u = 0;
                break;
            }

            case PGMMODE_PAE:
            case PGMMODE_PAE_NX:
            {
                const unsigned  iPdpt     = iOldPDE / 256;      /* iOldPDE * 2 / 512; iOldPDE is in 4 MB pages */
                unsigned        iPaePde   = iOldPDE * 2 % 512;
                PX86PDPT        pShwPdpt  = (PX86PDPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPageCR3);
                PX86PDPAE       pShwPaePd = pgmShwGetPaePDPtr(pVCpu, pShwPdpt, (iPdpt << X86_PDPT_SHIFT));

                /*
                 * Clear the PGM_PDFLAGS_MAPPING flag for the page directory pointer entry. (legacy PAE guest mode)
                 */
                if (fDeactivateCR3)
                    pShwPdpt->a[iPdpt].u &= ~PGM_PLXFLAGS_MAPPING;
                else if (pShwPdpt->a[iPdpt].u & PGM_PLXFLAGS_MAPPING)
                {
                    /* See if there are any other mappings here. This is suboptimal code. */
                    pShwPdpt->a[iPdpt].u &= ~PGM_PLXFLAGS_MAPPING;
                    for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur; pCur = pCur->CTX_SUFF(pNext))
                        if (    pCur != pMap
                            &&  (   (pCur->GCPtr >> X86_PDPT_SHIFT) == iPdpt
                                 || (pCur->GCPtrLast >> X86_PDPT_SHIFT) == iPdpt))
                        {
                            pShwPdpt->a[iPdpt].u |= PGM_PLXFLAGS_MAPPING;
                            break;
                        }
                }

                /*
                 * If the page directory of the old CR3 is reused in the new one, then don't
                 * clear the hypervisor mappings.
                 */
                if (    pCurrentShwPdpt
                    &&  (pCurrentShwPdpt->a[iPdpt].u & X86_PDPE_PG_MASK) == (pShwPdpt->a[iPdpt].u & X86_PDPE_PG_MASK) )
                {
                    LogFlow(("pgmMapClearShadowPDEs: Pdpe %d reused -> don't clear hypervisor mappings!\n", iPdpt));
                    break;
                }

                /*
                 * Clear the mappings in the PD.
                 */
                AssertFatal(pShwPaePd);
                Assert(!pShwPaePd->a[iPaePde].n.u1Present || (pShwPaePd->a[iPaePde].u & PGM_PDFLAGS_MAPPING));
                pShwPaePd->a[iPaePde].u = 0;

                iPaePde++;
                AssertFatal(iPaePde < 512);
                Assert(!pShwPaePd->a[iPaePde].n.u1Present || (pShwPaePd->a[iPaePde].u & PGM_PDFLAGS_MAPPING));
                pShwPaePd->a[iPaePde].u = 0;

                /*
                 * Unlock the shadow pool PD page if the PDPTE no longer holds any mappings.
                 */
                if (    fDeactivateCR3
                    ||  !(pShwPdpt->a[iPdpt].u & PGM_PLXFLAGS_MAPPING))
                {
                    PPGMPOOLPAGE pPoolPagePd = pgmPoolGetPage(pPool, pShwPdpt->a[iPdpt].u & X86_PDPE_PG_MASK);
                    AssertFatal(pPoolPagePd);
                    if (pgmPoolIsPageLocked(pPoolPagePd))
                        pgmPoolUnlockPage(pPool, pPoolPagePd);
                }
                break;
            }

            default:
                AssertFailed();
                break;
        }
    }

    PGM_DYNMAP_UNUSED_HINT_VM(pVM, pCurrentShwPdpt);
}

#endif /* PGM_WITHOUT_MAPPINGS */
#if defined(VBOX_STRICT) && !defined(IN_RING0)

/**
 * Clears all PDEs involved with the mapping in the shadow page table.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pShwPageCR3 CR3 root page
 * @param   pMap        Pointer to the mapping in question.
 * @param   iPDE        The index of the 32-bit PDE corresponding to the base of the mapping.
 */
static void pgmMapCheckShadowPDEs(PVM pVM, PVMCPU pVCpu, PPGMPOOLPAGE pShwPageCR3, PPGMMAPPING pMap, unsigned iPDE)
{
    Assert(pShwPageCR3);

    uint32_t i = pMap->cPTs;
    PGMMODE  enmShadowMode = PGMGetShadowMode(pVCpu);
    PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

    iPDE += i;
    while (i-- > 0)
    {
        iPDE--;

        switch (enmShadowMode)
        {
            case PGMMODE_32_BIT:
            {
                PCX86PD         pShw32BitPd = (PCX86PD)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPageCR3);
                AssertFatal(pShw32BitPd);

                AssertMsg(pShw32BitPd->a[iPDE].u == (PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US | (uint32_t)pMap->aPTs[i].HCPhysPT),
                          ("Expected %x vs %x; iPDE=%#x %RGv %s\n",
                           pShw32BitPd->a[iPDE].u,  (PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US | (uint32_t)pMap->aPTs[i].HCPhysPT),
                           iPDE, pMap->GCPtr, R3STRING(pMap->pszDesc) ));
                break;
            }

            case PGMMODE_PAE:
            case PGMMODE_PAE_NX:
            {
                const unsigned  iPdpt     = iPDE / 256;         /* iPDE * 2 / 512; iPDE is in 4 MB pages */
                unsigned        iPaePDE   = iPDE * 2 % 512;
                PX86PDPT        pShwPdpt  = (PX86PDPT)PGMPOOL_PAGE_2_PTR_V2(pVM, pVCpu, pShwPageCR3);
                PCX86PDPAE      pShwPaePd = pgmShwGetPaePDPtr(pVCpu, pShwPdpt, iPdpt << X86_PDPT_SHIFT);
                AssertFatal(pShwPaePd);

                AssertMsg(pShwPaePd->a[iPaePDE].u == (PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US | pMap->aPTs[i].HCPhysPaePT0),
                          ("Expected %RX64 vs %RX64; iPDE=%#x iPdpt=%#x iPaePDE=%#x %RGv %s\n",
                           pShwPaePd->a[iPaePDE].u,  (PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US | pMap->aPTs[i].HCPhysPaePT0),
                           iPDE, iPdpt, iPaePDE, pMap->GCPtr, R3STRING(pMap->pszDesc) ));

                iPaePDE++;
                AssertFatal(iPaePDE < 512);

                AssertMsg(pShwPaePd->a[iPaePDE].u == (PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US | pMap->aPTs[i].HCPhysPaePT1),
                          ("Expected %RX64 vs %RX64; iPDE=%#x iPdpt=%#x iPaePDE=%#x %RGv %s\n",
                           pShwPaePd->a[iPaePDE].u,  (PGM_PDFLAGS_MAPPING | X86_PDE_P | X86_PDE_A | X86_PDE_RW | X86_PDE_US | pMap->aPTs[i].HCPhysPaePT1),
                           iPDE, iPdpt, iPaePDE, pMap->GCPtr, R3STRING(pMap->pszDesc) ));

                AssertMsg(pShwPdpt->a[iPdpt].u & PGM_PLXFLAGS_MAPPING,
                          ("%RX64; iPdpt=%#x iPDE=%#x iPaePDE=%#x %RGv %s\n",
                           pShwPdpt->a[iPdpt].u,
                           iPDE, iPdpt, iPaePDE, pMap->GCPtr, R3STRING(pMap->pszDesc) ));

                PCPGMPOOLPAGE   pPoolPagePd = pgmPoolGetPage(pPool, pShwPdpt->a[iPdpt].u & X86_PDPE_PG_MASK);
                AssertFatal(pPoolPagePd);
                AssertMsg(pPoolPagePd->cLocked, (".idx=%d .type=%d\n", pPoolPagePd->idx, pPoolPagePd->enmKind));
                break;
            }

            default:
                AssertFailed();
                break;
        }
    }
}


/**
 * Check the hypervisor mappings in the active CR3.
 *
 * Ignored if mappings are disabled (i.e. if HM is enabled).
 *
 * @param   pVM         The cross context VM structure.
 */
VMMDECL(void) PGMMapCheck(PVM pVM)
{
    /*
     * Can skip this if mappings are disabled.
     */
    if (!pgmMapAreMappingsEnabled(pVM))
        return;

    /* This only applies to raw mode where we only support 1 VCPU. */
    Assert(pVM->cCpus == 1);
    PVMCPU pVCpu = VMMGetCpu0(pVM);
    Assert(pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));

    /*
     * Iterate mappings.
     */
    pgmLock(pVM);                           /* to avoid assertions */
    for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur; pCur = pCur->CTX_SUFF(pNext))
    {
        unsigned iPDE = pCur->GCPtr >> X86_PD_SHIFT;
        pgmMapCheckShadowPDEs(pVM, pVCpu, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3), pCur, iPDE);
    }
    pgmUnlock(pVM);
}

#endif /* defined(VBOX_STRICT) && !defined(IN_RING0) */
#ifndef PGM_WITHOUT_MAPPINGS

/**
 * Apply the hypervisor mappings to the active CR3.
 *
 * Ignored if mappings are disabled (i.e. if HM is enabled).
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pShwPageCR3 CR3 root page
 */
int pgmMapActivateCR3(PVM pVM, PPGMPOOLPAGE pShwPageCR3)
{
    RT_NOREF_PV(pShwPageCR3);

    /*
     * Skip this if it doesn't apply.
     */
    if (!pgmMapAreMappingsEnabled(pVM))
        return VINF_SUCCESS;

    /* Note! This might not be logged successfully in RC because we usually
             cannot flush the log at this point. */
    Log4(("pgmMapActivateCR3: fixed mappings=%RTbool idxShwPageCR3=%#x\n", pVM->pgm.s.fMappingsFixed, pShwPageCR3 ? pShwPageCR3->idx : NIL_PGMPOOL_IDX));

#ifdef VBOX_STRICT
    PVMCPU pVCpu = VMMGetCpu0(pVM);
    Assert(pShwPageCR3 && pShwPageCR3 == pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));
#endif

    /*
     * Iterate mappings.
     */
    for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur; pCur = pCur->CTX_SUFF(pNext))
    {
        unsigned iPDE = pCur->GCPtr >> X86_PD_SHIFT;
        pgmMapSetShadowPDEs(pVM, pCur, iPDE);
    }
    return VINF_SUCCESS;
}


/**
 * Remove the hypervisor mappings from the specified CR3
 *
 * Ignored if mappings are disabled (i.e. if HM is enabled).
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pShwPageCR3 CR3 root page
 */
int pgmMapDeactivateCR3(PVM pVM, PPGMPOOLPAGE pShwPageCR3)
{
    /*
     * Skip this if it doesn't apply.
     */
    if (!pgmMapAreMappingsEnabled(pVM))
        return VINF_SUCCESS;

    Assert(pShwPageCR3);
    Log4(("pgmMapDeactivateCR3: fixed mappings=%d idxShwPageCR3=%#x\n", pVM->pgm.s.fMappingsFixed, pShwPageCR3 ? pShwPageCR3->idx : NIL_PGMPOOL_IDX));

    /*
     * Iterate mappings.
     */
    for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur; pCur = pCur->CTX_SUFF(pNext))
    {
        unsigned iPDE = pCur->GCPtr >> X86_PD_SHIFT;
        pgmMapClearShadowPDEs(pVM, pShwPageCR3, pCur, iPDE, true /*fDeactivateCR3*/);
    }
    return VINF_SUCCESS;
}


/**
 * Checks guest PD for conflicts with VMM GC mappings.
 *
 * @returns true if conflict detected.
 * @returns false if not.
 * @param   pVM                 The cross context VM structure.
 */
VMMDECL(bool) PGMMapHasConflicts(PVM pVM)
{
    /*
     * Can skip this if mappings are safely fixed.
     */
    if (!pgmMapAreMappingsFloating(pVM))
        return false;
    AssertReturn(pgmMapAreMappingsEnabled(pVM), false);

    /* This only applies to raw mode where we only support 1 VCPU. */
    PVMCPU pVCpu = &pVM->aCpus[0];

    PGMMODE const enmGuestMode = PGMGetGuestMode(pVCpu);
    Assert(enmGuestMode <= PGMMODE_PAE_NX);

    /*
     * Iterate mappings.
     */
    if (enmGuestMode == PGMMODE_32_BIT)
    {
        /*
         * Resolve the page directory.
         */
        PX86PD pPD = pgmGstGet32bitPDPtr(pVCpu);
        Assert(pPD);

        for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur; pCur = pCur->CTX_SUFF(pNext))
        {
            unsigned iPDE = pCur->GCPtr >> X86_PD_SHIFT;
            unsigned iPT = pCur->cPTs;
            while (iPT-- > 0)
                if (    pPD->a[iPDE + iPT].n.u1Present /** @todo PGMGstGetPDE. */
                    &&  (EMIsRawRing0Enabled(pVM) || pPD->a[iPDE + iPT].n.u1User))
                {
                    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatR3DetectedConflicts);

# ifdef IN_RING3
                    Log(("PGMHasMappingConflicts: Conflict was detected at %08RX32 for mapping %s (32 bits)\n"
                         "                        iPDE=%#x iPT=%#x PDE=%RGp.\n",
                        (iPT + iPDE) << X86_PD_SHIFT, pCur->pszDesc,
                        iPDE, iPT, pPD->a[iPDE + iPT].au32[0]));
# else
                    Log(("PGMHasMappingConflicts: Conflict was detected at %08RX32 for mapping (32 bits)\n"
                         "                        iPDE=%#x iPT=%#x PDE=%RGp.\n",
                        (iPT + iPDE) << X86_PD_SHIFT,
                        iPDE, iPT, pPD->a[iPDE + iPT].au32[0]));
# endif
                    return true;
                }
        }
    }
    else if (   enmGuestMode == PGMMODE_PAE
             || enmGuestMode == PGMMODE_PAE_NX)
    {
        for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur; pCur = pCur->CTX_SUFF(pNext))
        {
            RTGCPTR   GCPtr = pCur->GCPtr;

            unsigned  iPT = pCur->cb >> X86_PD_PAE_SHIFT;
            while (iPT-- > 0)
            {
                X86PDEPAE Pde = pgmGstGetPaePDE(pVCpu, GCPtr);

                if (   Pde.n.u1Present
                    && (EMIsRawRing0Enabled(pVM) || Pde.n.u1User))
                {
                    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatR3DetectedConflicts);
# ifdef IN_RING3
                    Log(("PGMHasMappingConflicts: Conflict was detected at %RGv for mapping %s (PAE)\n"
                         "                        PDE=%016RX64.\n",
                        GCPtr, pCur->pszDesc, Pde.u));
# else
                    Log(("PGMHasMappingConflicts: Conflict was detected at %RGv for mapping (PAE)\n"
                         "                        PDE=%016RX64.\n",
                        GCPtr, Pde.u));
# endif
                    return true;
                }
                GCPtr += (1 << X86_PD_PAE_SHIFT);
            }
        }
    }
    else
        AssertFailed();

    return false;
}


/**
 * Checks and resolves (ring 3 only) guest conflicts with the guest mappings.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 */
int pgmMapResolveConflicts(PVM pVM)
{
    /* The caller is expected to check these two conditions. */
    Assert(!pVM->pgm.s.fMappingsFixed);
    Assert(pgmMapAreMappingsEnabled(pVM));

    /* This only applies to raw mode where we only support 1 VCPU. */
    Assert(pVM->cCpus == 1);
    PVMCPU          pVCpu        = &pVM->aCpus[0];
    PGMMODE const   enmGuestMode = PGMGetGuestMode(pVCpu);
    Assert(enmGuestMode <= PGMMODE_PAE_NX);

    if (enmGuestMode == PGMMODE_32_BIT)
    {
        /*
         * Resolve the page directory.
         */
        PX86PD pPD = pgmGstGet32bitPDPtr(pVCpu);
        Assert(pPD);

        /*
         * Iterate mappings.
         */
        for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur; )
        {
            PPGMMAPPING pNext = pCur->CTX_SUFF(pNext);
            unsigned    iPDE  = pCur->GCPtr >> X86_PD_SHIFT;
            unsigned    iPT   = pCur->cPTs;
            while (iPT-- > 0)
            {
                if (    pPD->a[iPDE + iPT].n.u1Present /** @todo PGMGstGetPDE. */
                    &&  (   EMIsRawRing0Enabled(pVM)
                         || pPD->a[iPDE + iPT].n.u1User))
                {
                    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatR3DetectedConflicts);

# ifdef IN_RING3
                    Log(("PGMHasMappingConflicts: Conflict was detected at %08RX32 for mapping %s (32 bits)\n"
                         "                        iPDE=%#x iPT=%#x PDE=%RGp.\n",
                         (iPT + iPDE) << X86_PD_SHIFT, pCur->pszDesc,
                         iPDE, iPT, pPD->a[iPDE + iPT].au32[0]));
                    int rc = pgmR3SyncPTResolveConflict(pVM, pCur, pPD, iPDE << X86_PD_SHIFT);
                    AssertRCReturn(rc, rc);
                    break;
# else
                    Log(("PGMHasMappingConflicts: Conflict was detected at %08RX32 for mapping (32 bits)\n"
                         "                        iPDE=%#x iPT=%#x PDE=%RGp.\n",
                         (iPT + iPDE) << X86_PD_SHIFT,
                         iPDE, iPT, pPD->a[iPDE + iPT].au32[0]));
                    return VINF_PGM_SYNC_CR3;
# endif
                }
            }
            pCur = pNext;
        }
    }
    else if (   enmGuestMode == PGMMODE_PAE
             || enmGuestMode == PGMMODE_PAE_NX)
    {
        /*
         * Iterate mappings.
         */
        for (PPGMMAPPING pCur = pVM->pgm.s.CTX_SUFF(pMappings); pCur;)
        {
            PPGMMAPPING pNext = pCur->CTX_SUFF(pNext);
            RTGCPTR     GCPtr = pCur->GCPtr;
            unsigned    iPT   = pCur->cb >> X86_PD_PAE_SHIFT;
            while (iPT-- > 0)
            {
                X86PDEPAE Pde = pgmGstGetPaePDE(pVCpu, GCPtr);

                if (   Pde.n.u1Present
                    && (EMIsRawRing0Enabled(pVM) || Pde.n.u1User))
                {
                    STAM_COUNTER_INC(&pVM->pgm.s.CTX_SUFF(pStats)->StatR3DetectedConflicts);
#ifdef IN_RING3
                    Log(("PGMHasMappingConflicts: Conflict was detected at %RGv for mapping %s (PAE)\n"
                         "                        PDE=%016RX64.\n",
                         GCPtr, pCur->pszDesc, Pde.u));
                    int rc = pgmR3SyncPTResolveConflictPAE(pVM, pCur, pCur->GCPtr);
                    AssertRCReturn(rc, rc);
                    break;
#else
                    Log(("PGMHasMappingConflicts: Conflict was detected at %RGv for mapping (PAE)\n"
                         "                        PDE=%016RX64.\n",
                         GCPtr, Pde.u));
                    return VINF_PGM_SYNC_CR3;
#endif
                }
                GCPtr += (1 << X86_PD_PAE_SHIFT);
            }
            pCur = pNext;
        }
    }
    else
        AssertFailed();

    Assert(!PGMMapHasConflicts(pVM));
    return VINF_SUCCESS;
}

#endif /* PGM_WITHOUT_MAPPINGS */


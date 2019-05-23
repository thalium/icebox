/* $Id: PGMR0SharedPage.cpp $ */
/** @file
 * PGM - Page Manager and Monitor, Page Sharing, Ring-0.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_PGM_SHARED
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/gmm.h>
#include "PGMInternal.h"
#include <VBox/vmm/vm.h>
#include "PGMInline.h"
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/mem.h>


#ifdef VBOX_WITH_PAGE_SHARING
/**
 * Check a registered module for shared page changes.
 *
 * The PGM lock shall be taken prior to calling this method.
 *
 * @returns The following VBox status codes.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pGVM                Pointer to the GVM instance data.
 * @param   idCpu               The ID of the calling virtual CPU.
 * @param   pModule             Global module description.
 * @param   paRegionsGCPtrs     Array parallel to pModules->aRegions with the
 *                              addresses of the regions in the calling
 *                              process.
 */
VMMR0DECL(int) PGMR0SharedModuleCheck(PVM pVM, PGVM pGVM, VMCPUID idCpu, PGMMSHAREDMODULE pModule, PCRTGCPTR64 paRegionsGCPtrs)
{
    PVMCPU              pVCpu         = &pVM->aCpus[idCpu];
    int                 rc            = VINF_SUCCESS;
    bool                fFlushTLBs    = false;
    bool                fFlushRemTLBs = false;
    GMMSHAREDPAGEDESC   PageDesc;

    Log(("PGMR0SharedModuleCheck: check %s %s base=%RGv size=%x\n", pModule->szName, pModule->szVersion, pModule->Core.Key, pModule->cbModule));

    PGM_LOCK_ASSERT_OWNER(pVM);     /* This cannot fail as we grab the lock in pgmR3SharedModuleRegRendezvous before calling into ring-0. */

    /*
     * Check every region of the shared module.
     */
    for (uint32_t idxRegion = 0; idxRegion < pModule->cRegions; idxRegion++)
    {
        RTGCPTR  GCPtrPage  = paRegionsGCPtrs[idxRegion] & ~(RTGCPTR)PAGE_OFFSET_MASK;
        uint32_t cbLeft     = pModule->aRegions[idxRegion].cb; Assert(!(cbLeft & PAGE_OFFSET_MASK));
        uint32_t idxPage    = 0;

        while (cbLeft)
        {
            /** @todo inefficient to fetch each guest page like this... */
            RTGCPHYS GCPhys;
            uint64_t fFlags;
            rc = PGMGstGetPage(pVCpu, GCPtrPage, &fFlags, &GCPhys);
            if (    rc == VINF_SUCCESS
                &&  !(fFlags & X86_PTE_RW)) /* important as we make assumptions about this below! */
            {
                PPGMPAGE pPage = pgmPhysGetPage(pVM, GCPhys);
                Assert(!pPage || !PGM_PAGE_IS_BALLOONED(pPage));
                if (    pPage
                    &&  PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED
                    &&  PGM_PAGE_GET_READ_LOCKS(pPage) == 0
                    &&  PGM_PAGE_GET_WRITE_LOCKS(pPage) == 0 )
                {
                    PageDesc.idPage = PGM_PAGE_GET_PAGEID(pPage);
                    PageDesc.HCPhys = PGM_PAGE_GET_HCPHYS(pPage);
                    PageDesc.GCPhys = GCPhys;

                    rc = GMMR0SharedModuleCheckPage(pGVM, pModule, idxRegion, idxPage, &PageDesc);
                    if (RT_FAILURE(rc))
                        break;

                    /*
                     * Any change for this page?
                     */
                    if (PageDesc.idPage != NIL_GMM_PAGEID)
                    {
                        Assert(PGM_PAGE_GET_STATE(pPage) == PGM_PAGE_STATE_ALLOCATED);

                        Log(("PGMR0SharedModuleCheck: shared page gst virt=%RGv phys=%RGp host %RHp->%RHp\n",
                             GCPtrPage, PageDesc.GCPhys, PGM_PAGE_GET_HCPHYS(pPage), PageDesc.HCPhys));

                        /* Page was either replaced by an existing shared
                           version of it or converted into a read-only shared
                           page, so, clear all references. */
                        bool fFlush = false;
                        rc = pgmPoolTrackUpdateGCPhys(pVM, PageDesc.GCPhys, pPage, true /* clear the entries */, &fFlush);
                        Assert(   rc == VINF_SUCCESS
                               || (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3)
                                   && (pVCpu->pgm.s.fSyncFlags & PGM_SYNC_CLEAR_PGM_POOL)));
                        if (rc == VINF_SUCCESS)
                            fFlushTLBs |= fFlush;
                        fFlushRemTLBs = true;

                        if (PageDesc.HCPhys != PGM_PAGE_GET_HCPHYS(pPage))
                        {
                            /* Update the physical address and page id now. */
                            PGM_PAGE_SET_HCPHYS(pVM, pPage, PageDesc.HCPhys);
                            PGM_PAGE_SET_PAGEID(pVM, pPage, PageDesc.idPage);

                            /* Invalidate page map TLB entry for this page too. */
                            pgmPhysInvalidatePageMapTLBEntry(pVM, PageDesc.GCPhys);
                            pVM->pgm.s.cReusedSharedPages++;
                        }
                        /* else: nothing changed (== this page is now a shared
                           page), so no need to flush anything. */

                        pVM->pgm.s.cSharedPages++;
                        pVM->pgm.s.cPrivatePages--;
                        PGM_PAGE_SET_STATE(pVM, pPage, PGM_PAGE_STATE_SHARED);

# ifdef VBOX_STRICT /* check sum hack */
                        pPage->s.u2Unused0 = PageDesc.u32StrictChecksum        & 3;
                        pPage->s.u2Unused1 = (PageDesc.u32StrictChecksum >> 8) & 3;
# endif
                    }
                }
            }
            else
            {
                Assert(    rc == VINF_SUCCESS
                       ||  rc == VERR_PAGE_NOT_PRESENT
                       ||  rc == VERR_PAGE_MAP_LEVEL4_NOT_PRESENT
                       ||  rc == VERR_PAGE_DIRECTORY_PTR_NOT_PRESENT
                       ||  rc == VERR_PAGE_TABLE_NOT_PRESENT);
                rc = VINF_SUCCESS;  /* ignore error */
            }

            idxPage++;
            GCPtrPage += PAGE_SIZE;
            cbLeft    -= PAGE_SIZE;
        }
    }

    /*
     * Do TLB flushing if necessary.
     */
    if (fFlushTLBs)
        PGM_INVL_ALL_VCPU_TLBS(pVM);

    if (fFlushRemTLBs)
        for (VMCPUID idCurCpu = 0; idCurCpu < pVM->cCpus; idCurCpu++)
            CPUMSetChangedFlags(&pVM->aCpus[idCurCpu], CPUM_CHANGED_GLOBAL_TLB_FLUSH);

    return rc;
}
#endif /* VBOX_WITH_PAGE_SHARING */


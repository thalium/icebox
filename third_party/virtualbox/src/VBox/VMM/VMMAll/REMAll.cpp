/* $Id: REMAll.cpp $ */
/** @file
 * REM - Recompiled Execution Monitor, all Contexts part.
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
#define LOG_GROUP LOG_GROUP_REM
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/em.h>
#include <VBox/vmm/vmm.h>
#include "REMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/asm.h>
#include <iprt/assert.h>


#ifndef IN_RING3

/**
 * Records a invlpg instruction for replaying upon REM entry.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPtrPage   The
 */
VMMDECL(void) REMNotifyInvalidatePage(PVM pVM, RTGCPTR GCPtrPage)
{
    /*
     * Try take the REM lock and push the address onto the array.
     */
    if (   pVM->rem.s.cInvalidatedPages < RT_ELEMENTS(pVM->rem.s.aGCPtrInvalidatedPages)
        && EMRemTryLock(pVM) == VINF_SUCCESS)
    {
        uint32_t iPage = pVM->rem.s.cInvalidatedPages;
        if (iPage < RT_ELEMENTS(pVM->rem.s.aGCPtrInvalidatedPages))
        {
            ASMAtomicWriteU32(&pVM->rem.s.cInvalidatedPages, iPage + 1);
            pVM->rem.s.aGCPtrInvalidatedPages[iPage] = GCPtrPage;

            EMRemUnlock(pVM);
            return;
        }

        CPUMSetChangedFlags(VMMGetCpu(pVM), CPUM_CHANGED_GLOBAL_TLB_FLUSH); /** @todo this array should be per-cpu technically speaking. */
        ASMAtomicWriteU32(&pVM->rem.s.cInvalidatedPages, 0); /** @todo leave this alone? Optimize this code? */

        EMRemUnlock(pVM);
    }
    else
    {
        /* Fallback: Simply tell the recompiler to flush its TLB. */
        CPUMSetChangedFlags(VMMGetCpu(pVM), CPUM_CHANGED_GLOBAL_TLB_FLUSH);
        ASMAtomicWriteU32(&pVM->rem.s.cInvalidatedPages, 0); /** @todo leave this alone?! Optimize this code? */
    }

    return;
}


/**
 * Insert pending notification
 *
 * @param   pVM             The cross context VM structure.
 * @param   pRec            Notification record to insert
 */
static void remNotifyHandlerInsert(PVM pVM, PREMHANDLERNOTIFICATION pRec)
{
    /*
     * Fetch a free record.
     */
    uint32_t                cFlushes = 0;
    uint32_t                idxFree;
    PREMHANDLERNOTIFICATION pFree;
    do
    {
        idxFree = ASMAtomicUoReadU32(&pVM->rem.s.idxFreeList);
        if (idxFree == UINT32_MAX)
        {
            do
            {
                cFlushes++;
                Assert(cFlushes != 128);
                AssertFatal(cFlushes < _1M);
                VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_REM_REPLAY_HANDLER_NOTIFICATIONS, 0);
                idxFree = ASMAtomicUoReadU32(&pVM->rem.s.idxFreeList);
            } while (idxFree == UINT32_MAX);
        }
        pFree = &pVM->rem.s.aHandlerNotifications[idxFree];
    } while (!ASMAtomicCmpXchgU32(&pVM->rem.s.idxFreeList, pFree->idxNext, idxFree));

    /*
     * Copy the record.
     */
    pFree->enmKind = pRec->enmKind;
    pFree->u = pRec->u;

    /*
     * Insert it into the pending list.
     */
    uint32_t idxNext;
    do
    {
        idxNext = ASMAtomicUoReadU32(&pVM->rem.s.idxPendingList);
        ASMAtomicWriteU32(&pFree->idxNext, idxNext);
        ASMCompilerBarrier();
    } while (!ASMAtomicCmpXchgU32(&pVM->rem.s.idxPendingList, idxFree, idxNext));

    VM_FF_SET(pVM, VM_FF_REM_HANDLER_NOTIFY);
}


/**
 * Notification about a successful PGMR3HandlerPhysicalRegister() call.
 *
 * @param   pVM             The cross context VM structure.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler have a HC callback function.
 */
VMMDECL(void) REMNotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler)
{
    REMHANDLERNOTIFICATION Rec;
    Rec.enmKind = REMHANDLERNOTIFICATIONKIND_PHYSICAL_REGISTER;
    Rec.u.PhysicalRegister.enmKind = enmKind;
    Rec.u.PhysicalRegister.GCPhys = GCPhys;
    Rec.u.PhysicalRegister.cb = cb;
    Rec.u.PhysicalRegister.fHasHCHandler = fHasHCHandler;
    remNotifyHandlerInsert(pVM, &Rec);
}


/**
 * Notification about a successful PGMR3HandlerPhysicalDeregister() operation.
 *
 * @param   pVM             The cross context VM structure.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler have a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
VMMDECL(void) REMNotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
    REMHANDLERNOTIFICATION Rec;
    Rec.enmKind = REMHANDLERNOTIFICATIONKIND_PHYSICAL_DEREGISTER;
    Rec.u.PhysicalDeregister.enmKind = enmKind;
    Rec.u.PhysicalDeregister.GCPhys = GCPhys;
    Rec.u.PhysicalDeregister.cb = cb;
    Rec.u.PhysicalDeregister.fHasHCHandler = fHasHCHandler;
    Rec.u.PhysicalDeregister.fRestoreAsRAM = fRestoreAsRAM;
    remNotifyHandlerInsert(pVM, &Rec);
}


/**
 * Notification about a successful PGMR3HandlerPhysicalModify() call.
 *
 * @param   pVM             The cross context VM structure.
 * @param   enmKind         Kind of access handler.
 * @param   GCPhysOld       Old handler range address.
 * @param   GCPhysNew       New handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler have a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
VMMDECL(void) REMNotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
    REMHANDLERNOTIFICATION Rec;
    Rec.enmKind = REMHANDLERNOTIFICATIONKIND_PHYSICAL_MODIFY;
    Rec.u.PhysicalModify.enmKind = enmKind;
    Rec.u.PhysicalModify.GCPhysOld = GCPhysOld;
    Rec.u.PhysicalModify.GCPhysNew = GCPhysNew;
    Rec.u.PhysicalModify.cb = cb;
    Rec.u.PhysicalModify.fHasHCHandler = fHasHCHandler;
    Rec.u.PhysicalModify.fRestoreAsRAM = fRestoreAsRAM;
    remNotifyHandlerInsert(pVM, &Rec);
}

#endif /* !IN_RING3 */

#ifdef IN_RC
/**
 * Flushes the physical handler notifications if the queue is almost full.
 *
 * This is for avoiding trouble in RC when changing CR3.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(void) REMNotifyHandlerPhysicalFlushIfAlmostFull(PVM pVM, PVMCPU pVCpu)
{
    Assert(pVM->cCpus == 1); NOREF(pVCpu);

    /*
     * Less than 48 items means we should flush.
     */
    uint32_t cFree = 0;
    for (uint32_t idx = pVM->rem.s.idxFreeList;
         idx != UINT32_MAX;
         idx = pVM->rem.s.aHandlerNotifications[idx].idxNext)
    {
        Assert(idx < RT_ELEMENTS(pVM->rem.s.aHandlerNotifications));
        if (++cFree >= 48)
            return;
    }
    AssertRelease(VM_FF_IS_SET(pVM, VM_FF_REM_HANDLER_NOTIFY));
    AssertRelease(pVM->rem.s.idxPendingList != UINT32_MAX);

    /* Ok, we gotta flush them. */
    VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_REM_REPLAY_HANDLER_NOTIFICATIONS, 0);

    AssertRelease(pVM->rem.s.idxPendingList == UINT32_MAX);
    AssertRelease(pVM->rem.s.idxFreeList != UINT32_MAX);
}
#endif /* IN_RC */


/**
 * Make REM flush all translation block upon the next call to REMR3State().
 *
 * @param   pVM             The cross context VM structure.
 */
VMMDECL(void) REMFlushTBs(PVM pVM)
{
    LogFlow(("REMFlushTBs: fFlushTBs=%RTbool fInREM=%RTbool fInStateSync=%RTbool\n",
             pVM->rem.s.fFlushTBs, pVM->rem.s.fInREM, pVM->rem.s.fInStateSync));
    pVM->rem.s.fFlushTBs = true;
}


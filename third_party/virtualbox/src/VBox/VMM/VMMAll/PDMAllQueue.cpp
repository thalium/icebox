/* $Id: PDMAllQueue.cpp $ */
/** @file
 * PDM Queue - Transport data and tasks to EMT and R3.
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
#define LOG_GROUP LOG_GROUP_PDM_QUEUE
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#ifndef IN_RC
# ifdef VBOX_WITH_REM
#  include <VBox/vmm/rem.h>
# endif
# include <VBox/vmm/mm.h>
#endif
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>


/**
 * Allocate an item from a queue.
 * The allocated item must be handed on to PDMR3QueueInsert() after the
 * data have been filled in.
 *
 * @returns Pointer to allocated queue item.
 * @returns NULL on failure. The queue is exhausted.
 * @param   pQueue      The queue handle.
 * @thread  Any thread.
 */
VMMDECL(PPDMQUEUEITEMCORE) PDMQueueAlloc(PPDMQUEUE pQueue)
{
    Assert(VALID_PTR(pQueue) && pQueue->CTX_SUFF(pVM));
    PPDMQUEUEITEMCORE pNew;
    uint32_t iNext;
    uint32_t i;
    do
    {
        i = pQueue->iFreeTail;
        if (i == pQueue->iFreeHead)
        {
            STAM_REL_COUNTER_INC(&pQueue->StatAllocFailures);
            return NULL;
        }
        pNew = pQueue->aFreeItems[i].CTX_SUFF(pItem);
        iNext = (i + 1) % (pQueue->cItems + PDMQUEUE_FREE_SLACK);
    } while (!ASMAtomicCmpXchgU32(&pQueue->iFreeTail, iNext, i));
    return pNew;
}


/**
 * Sets the FFs and fQueueFlushed.
 *
 * @param   pQueue              The PDM queue.
 */
static void pdmQueueSetFF(PPDMQUEUE pQueue)
{
    PVM pVM = pQueue->CTX_SUFF(pVM);
    Log2(("PDMQueueInsert: VM_FF_PDM_QUEUES %d -> 1\n", VM_FF_IS_SET(pVM, VM_FF_PDM_QUEUES)));
    VM_FF_SET(pVM, VM_FF_PDM_QUEUES);
    ASMAtomicBitSet(&pVM->pdm.s.fQueueFlushing, PDM_QUEUE_FLUSH_FLAG_PENDING_BIT);
#ifdef IN_RING3
# ifdef VBOX_WITH_REM
    REMR3NotifyQueuePending(pVM); /** @todo r=bird: we can remove REMR3NotifyQueuePending and let VMR3NotifyFF do the work. */
# endif
    VMR3NotifyGlobalFFU(pVM->pUVM, VMNOTIFYFF_FLAGS_DONE_REM);
#endif
}


/**
 * Queue an item.
 * The item must have been obtained using PDMQueueAlloc(). Once the item
 * have been passed to this function it must not be touched!
 *
 * @param   pQueue      The queue handle.
 * @param   pItem       The item to insert.
 * @thread  Any thread.
 */
VMMDECL(void) PDMQueueInsert(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem)
{
    Assert(VALID_PTR(pQueue) && pQueue->CTX_SUFF(pVM));
    Assert(VALID_PTR(pItem));

#if 0 /* the paranoid android version: */
    void *pvNext;
    do
    {
        pvNext = ASMAtomicUoReadPtr((void * volatile *)&pQueue->CTX_SUFF(pPending));
        ASMAtomicUoWritePtr((void * volatile *)&pItem->CTX_SUFF(pNext), pvNext);
    } while (!ASMAtomicCmpXchgPtr(&pQueue->CTX_SUFF(pPending), pItem, pvNext));
#else
    PPDMQUEUEITEMCORE pNext;
    do
    {
        pNext = pQueue->CTX_SUFF(pPending);
        pItem->CTX_SUFF(pNext) = pNext;
    } while (!ASMAtomicCmpXchgPtr(&pQueue->CTX_SUFF(pPending), pItem, pNext));
#endif

    if (!pQueue->pTimer)
        pdmQueueSetFF(pQueue);
    STAM_REL_COUNTER_INC(&pQueue->StatInsert);
    STAM_STATS({ ASMAtomicIncU32(&pQueue->cStatPending); });
}


/**
 * Queue an item.
 * The item must have been obtained using PDMQueueAlloc(). Once the item
 * have been passed to this function it must not be touched!
 *
 * @param   pQueue          The queue handle.
 * @param   pItem           The item to insert.
 * @param   NanoMaxDelay    The maximum delay before processing the queue, in nanoseconds.
 *                          This applies only to GC.
 * @thread  Any thread.
 */
VMMDECL(void) PDMQueueInsertEx(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem, uint64_t NanoMaxDelay)
{
    NOREF(NanoMaxDelay);
    PDMQueueInsert(pQueue, pItem);
#ifdef IN_RC
    PVM pVM = pQueue->CTX_SUFF(pVM);
    /** @todo figure out where to put this, the next bit should go there too.
    if (NanoMaxDelay)
    {

    }
    else */
    {
        VMCPU_FF_SET(VMMGetCpu0(pVM), VMCPU_FF_TO_R3);
        Log2(("PDMQueueInsertEx: Setting VMCPU_FF_TO_R3\n"));
    }
#endif
}



/**
 * Gets the RC pointer for the specified queue.
 *
 * @returns The RC address of the queue.
 * @returns NULL if pQueue is invalid.
 * @param   pQueue          The queue handle.
 */
VMMDECL(RCPTRTYPE(PPDMQUEUE)) PDMQueueRCPtr(PPDMQUEUE pQueue)
{
    Assert(VALID_PTR(pQueue));
    Assert(pQueue->pVMR3 && pQueue->pVMRC);
#ifdef IN_RC
    return pQueue;
#else
    return MMHyperCCToRC(pQueue->CTX_SUFF(pVM), pQueue);
#endif
}


/**
 * Gets the ring-0 pointer for the specified queue.
 *
 * @returns The ring-0 address of the queue.
 * @returns NULL if pQueue is invalid.
 * @param   pQueue          The queue handle.
 */
VMMDECL(R0PTRTYPE(PPDMQUEUE)) PDMQueueR0Ptr(PPDMQUEUE pQueue)
{
    Assert(VALID_PTR(pQueue));
    Assert(pQueue->pVMR3 && pQueue->pVMR0);
#ifdef IN_RING0
    return pQueue;
#else
    return MMHyperCCToR0(pQueue->CTX_SUFF(pVM), pQueue);
#endif
}


/**
 * Schedule the queue for flushing (processing) if necessary.
 *
 * @returns @c true if necessary, @c false if not.
 * @param   pQueue              The queue.
 */
VMMDECL(bool) PDMQueueFlushIfNecessary(PPDMQUEUE pQueue)
{
    AssertPtr(pQueue);
    if (   pQueue->pPendingR3 != NIL_RTR3PTR
        || pQueue->pPendingR0 != NIL_RTR0PTR
        || pQueue->pPendingRC != NIL_RTRCPTR)
    {
        pdmQueueSetFF(pQueue);
        return false;
    }
    return false;
}


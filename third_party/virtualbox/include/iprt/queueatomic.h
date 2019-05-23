/** @file
 * IPRT - Generic Work Queue with concurrent atomic access.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___iprt_queueatomic_h
#define ___iprt_queueatomic_h

#include <iprt/types.h>
#include <iprt/asm.h>

/** @defgroup grp_rt_queueatomic RTQueueAtomic - Generic Work Queue
 * @ingroup grp_rt
 *
 * Implementation of a lockless work queue for threaded environments.
 * @{
 */

RT_C_DECLS_BEGIN

/**
 * A work item
 */
typedef struct RTQUEUEATOMICITEM
{
    /** Pointer to the next work item in the list. */
    struct RTQUEUEATOMICITEM * volatile pNext;
} RTQUEUEATOMICITEM;
/** Pointer to a work item. */
typedef RTQUEUEATOMICITEM *PRTQUEUEATOMICITEM;
/** Pointer to a work item pointer. */
typedef PRTQUEUEATOMICITEM *PPRTQUEUEATOMICITEM;

/**
 * Work queue.
 */
typedef struct RTQUEUEATOMIC
{
    /* Head of the work queue. */
    volatile PRTQUEUEATOMICITEM        pHead;
} RTQUEUEATOMIC;
/** Pointer to a work queue. */
typedef RTQUEUEATOMIC *PRTQUEUEATOMIC;

/**
 * Initialize a work queue.
 *
 * @param   pWorkQueue          Pointer to an unitialised work queue.
 */
DECLINLINE(void) RTQueueAtomicInit(PRTQUEUEATOMIC pWorkQueue)
{
    ASMAtomicWriteNullPtr(&pWorkQueue->pHead);
}

/**
 * Insert a new item into the work queue.
 *
 * @param   pWorkQueue          The work queue to insert into.
 * @param   pItem               The item to insert.
 */
DECLINLINE(void) RTQueueAtomicInsert(PRTQUEUEATOMIC pWorkQueue, PRTQUEUEATOMICITEM pItem)
{
    PRTQUEUEATOMICITEM pNext = ASMAtomicUoReadPtrT(&pWorkQueue->pHead, PRTQUEUEATOMICITEM);
    PRTQUEUEATOMICITEM pHeadOld;
    pItem->pNext = pNext;
    while (!ASMAtomicCmpXchgExPtr(&pWorkQueue->pHead, pItem, pNext, &pHeadOld))
    {
        pNext = pHeadOld;
        Assert(pNext != pItem);
        pItem->pNext = pNext;
        ASMNopPause();
    }
}

/**
 * Remove all items from the given work queue and return them in the inserted order.
 *
 * @returns Pointer to the first item.
 * @param   pWorkQueue          The work queue.
 */
DECLINLINE(PRTQUEUEATOMICITEM) RTQueueAtomicRemoveAll(PRTQUEUEATOMIC pWorkQueue)
{
    PRTQUEUEATOMICITEM pHead = ASMAtomicXchgPtrT(&pWorkQueue->pHead, NULL, PRTQUEUEATOMICITEM);

    /* Reverse it. */
    PRTQUEUEATOMICITEM pCur = pHead;
    pHead = NULL;
    while (pCur)
    {
        PRTQUEUEATOMICITEM pInsert = pCur;
        pCur = pCur->pNext;
        pInsert->pNext = pHead;
        pHead = pInsert;
    }

    return pHead;
}

RT_C_DECLS_END

/** @} */

#endif


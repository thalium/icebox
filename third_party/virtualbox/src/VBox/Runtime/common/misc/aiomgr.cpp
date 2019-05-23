/* $Id: aiomgr.cpp $ */
/** @file
 * IPRT - Async I/O manager.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/aiomgr.h>
#include <iprt/err.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/file.h>
#include <iprt/list.h>
#include <iprt/thread.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/critsect.h>
#include <iprt/memcache.h>
#include <iprt/semaphore.h>
#include <iprt/queueatomic.h>

#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/** Pointer to an internal async I/O file instance. */
typedef struct RTAIOMGRFILEINT *PRTAIOMGRFILEINT;

/**
 * Blocking event types.
 */
typedef enum RTAIOMGREVENT
{
    /** Invalid tye */
    RTAIOMGREVENT_INVALID = 0,
    /** No event pending. */
    RTAIOMGREVENT_NO_EVENT,
    /** A file is added to the manager. */
    RTAIOMGREVENT_FILE_ADD,
    /** A file is about to be closed. */
    RTAIOMGREVENT_FILE_CLOSE,
    /** The async I/O manager is shut down. */
    RTAIOMGREVENT_SHUTDOWN,
    /** 32bit hack */
    RTAIOMGREVENT_32BIT_HACK = 0x7fffffff
} RTAIOMGREVENT;

/**
 * Async I/O manager instance data.
 */
typedef struct RTAIOMGRINT
{
    /** Magic value. */
    uint32_t                      u32Magic;
    /** Reference count. */
    volatile uint32_t             cRefs;
    /** Async I/O context handle. */
    RTFILEAIOCTX                  hAioCtx;
    /** async I/O thread. */
    RTTHREAD                      hThread;
    /** List of files assigned to this manager. */
    RTLISTANCHOR                  ListFiles;
    /** Number of requests active currently. */
    unsigned                      cReqsActive;
    /** Number of maximum requests active. */
    uint32_t                      cReqsActiveMax;
    /** Memory cache for requests. */
    RTMEMCACHE                    hMemCacheReqs;
    /** Critical section protecting the blocking event handling. */
    RTCRITSECT                    CritSectBlockingEvent;
    /** Event semaphore for blocking external events.
     * The caller waits on it until the async I/O manager
     * finished processing the event. */
    RTSEMEVENT                    hEventSemBlock;
    /** Blocking event type */
    volatile RTAIOMGREVENT        enmBlockingEvent;
    /** Event type data */
    union
    {
        /** The file to be added */
        volatile PRTAIOMGRFILEINT pFileAdd;
        /** The file to be closed */
        volatile PRTAIOMGRFILEINT pFileClose;
    } BlockingEventData;
} RTAIOMGRINT;
/** Pointer to an internal async I/O manager instance. */
typedef RTAIOMGRINT *PRTAIOMGRINT;

/**
 * Async I/O manager file instance data.
 */
typedef struct RTAIOMGRFILEINT
{
    /** Magic value. */
    uint32_t                      u32Magic;
    /** Reference count. */
    volatile uint32_t             cRefs;
    /** Flags. */
    uint32_t                      fFlags;
    /** Opaque user data passed on creation. */
    void                         *pvUser;
    /** File handle. */
    RTFILE                        hFile;
    /** async I/O manager this file belongs to. */
    PRTAIOMGRINT                  pAioMgr;
    /** Work queue for new requests. */
    RTQUEUEATOMIC                 QueueReqs;
    /** Completion callback for this file. */
    PFNRTAIOMGRREQCOMPLETE        pfnReqCompleted;
    /** Data for exclusive use by the assigned async I/O manager. */
    struct
    {
        /** List node of assigned files for a async I/O manager. */
        RTLISTNODE                NodeAioMgrFiles;
        /** List of requests waiting for submission. */
        RTLISTANCHOR              ListWaitingReqs;
        /** Number of requests currently being processed for this endpoint
         * (excluded flush requests). */
        unsigned                  cReqsActive;
    } AioMgr;
} RTAIOMGRFILEINT;

/** Flag whether the file is closed. */
#define RTAIOMGRFILE_FLAGS_CLOSING RT_BIT_32(1)

/**
 * Request type.
 */
typedef enum RTAIOMGRREQTYPE
{
    /** Invalid request type. */
    RTAIOMGRREQTYPE_INVALID = 0,
    /** Read reques type. */
    RTAIOMGRREQTYPE_READ,
    /** Write request. */
    RTAIOMGRREQTYPE_WRITE,
    /** Flush request. */
    RTAIOMGRREQTYPE_FLUSH,
    /** Prefetech request. */
    RTAIOMGRREQTYPE_PREFETCH,
    /** 32bit hack. */
    RTAIOMGRREQTYPE_32BIT_HACK = 0x7fffffff
} RTAIOMGRREQTYPE;
/** Pointer to a reques type. */
typedef RTAIOMGRREQTYPE *PRTAIOMGRREQTYPE;

/**
 * Async I/O manager request.
 */
typedef struct RTAIOMGRREQ
{
    /** Atomic queue work item. */
    RTQUEUEATOMICITEM WorkItem;
    /** Node for a waiting list. */
    RTLISTNODE        NodeWaitingList;
    /** Request flags. */
    uint32_t          fFlags;
    /** Transfer type. */
    RTAIOMGRREQTYPE   enmType;
    /** Assigned file request. */
    RTFILEAIOREQ      hReqIo;
    /** File the request belongs to. */
    PRTAIOMGRFILEINT  pFile;
    /** Opaque user data. */
    void             *pvUser;
    /** Start offset */
    RTFOFF            off;
    /** Data segment. */
    RTSGSEG           DataSeg;
    /** When non-zero the segment uses a bounce buffer because the provided buffer
     * doesn't meet host requirements. */
    size_t            cbBounceBuffer;
    /** Pointer to the used bounce buffer if any. */
    void             *pvBounceBuffer;
    /** Start offset in the bounce buffer to copy from. */
    uint32_t          offBounceBuffer;
} RTAIOMGRREQ;
/** Pointer to a I/O manager request. */
typedef RTAIOMGRREQ *PRTAIOMGRREQ;

/** Flag whether the request was prepared already. */
#define RTAIOMGRREQ_FLAGS_PREPARED RT_BIT_32(0)


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTAIOMGR_VALID_RETURN_RC(a_hAioMgr, a_rc) \
    do { \
        AssertPtrReturn((a_hAioMgr), (a_rc)); \
        AssertReturn((a_hAioMgr)->u32Magic == RTAIOMGR_MAGIC, (a_rc)); \
    } while (0)

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTAIOMGR_VALID_RETURN(a_hAioMgr) RTAIOMGR_VALID_RETURN_RC((hAioMgr), VERR_INVALID_HANDLE)

/** Validates a handle and returns (void) if not valid. */
#define RTAIOMGR_VALID_RETURN_VOID(a_hAioMgr) \
    do { \
        AssertPtrReturnVoid(a_hAioMgr); \
        AssertReturnVoid((a_hAioMgr)->u32Magic == RTAIOMGR_MAGIC); \
    } while (0)


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

static int rtAioMgrReqsEnqueue(PRTAIOMGRINT pThis, PRTAIOMGRFILEINT pFile,
                               PRTFILEAIOREQ pahReqs, unsigned cReqs);

/**
 * Removes an endpoint from the currently assigned manager.
 *
 * @returns TRUE if there are still requests pending on the current manager for this endpoint.
 *          FALSE otherwise.
 * @param   pFile           The endpoint to remove.
 */
static bool rtAioMgrFileRemove(PRTAIOMGRFILEINT pFile)
{
    /* Make sure that there is no request pending on this manager for the endpoint. */
    if (!pFile->AioMgr.cReqsActive)
    {
        RTListNodeRemove(&pFile->AioMgr.NodeAioMgrFiles);
        return false;
    }

    return true;
}

/**
 * Allocate a new I/O request.
 *
 * @returns Pointer to the allocated request or NULL if out of memory.
 * @param   pThis             The async I/O manager instance.
 */
static PRTAIOMGRREQ rtAioMgrReqAlloc(PRTAIOMGRINT pThis)
{
    return (PRTAIOMGRREQ)RTMemCacheAlloc(pThis->hMemCacheReqs);
}

/**
 * Frees an I/O request.
 *
 * @returns nothing.
 * @param   pThis             The async I/O manager instance.
 * @param   pReq              The request to free.
 */
static void rtAioMgrReqFree(PRTAIOMGRINT pThis, PRTAIOMGRREQ pReq)
{
    if (pReq->cbBounceBuffer)
    {
        AssertPtr(pReq->pvBounceBuffer);
        RTMemPageFree(pReq->pvBounceBuffer, pReq->cbBounceBuffer);
        pReq->pvBounceBuffer = NULL;
        pReq->cbBounceBuffer = 0;
    }
    pReq->fFlags = 0;
    RTAioMgrFileRelease(pReq->pFile);
    RTMemCacheFree(pThis->hMemCacheReqs, pReq);
}

static void rtAioMgrReqCompleteRc(PRTAIOMGRINT pThis, PRTAIOMGRREQ pReq,
                                  int rcReq, size_t cbTransfered)
{
    int rc = VINF_SUCCESS;
    PRTAIOMGRFILEINT pFile;

    pFile = pReq->pFile;
    pThis->cReqsActive--;
    pFile->AioMgr.cReqsActive--;

    /*
     * It is possible that the request failed on Linux with kernels < 2.6.23
     * if the passed buffer was allocated with remap_pfn_range or if the file
     * is on an NFS endpoint which does not support async and direct I/O at the same time.
     * The endpoint will be migrated to a failsafe manager in case a request fails.
     */
    if (RT_FAILURE(rcReq))
    {
        pFile->pfnReqCompleted(pFile, rcReq, pReq->pvUser);
        rtAioMgrReqFree(pThis, pReq);
    }
    else
    {
        /*
         * Restart an incomplete transfer.
         * This usually means that the request will return an error now
         * but to get the cause of the error (disk full, file too big, I/O error, ...)
         * the transfer needs to be continued.
         */
        if (RT_UNLIKELY(   cbTransfered < pReq->DataSeg.cbSeg
                        || (   pReq->cbBounceBuffer
                            && cbTransfered < pReq->cbBounceBuffer)))
        {
            RTFOFF offStart;
            size_t cbToTransfer;
            uint8_t *pbBuf = NULL;

            Assert(cbTransfered % 512 == 0);

            if (pReq->cbBounceBuffer)
            {
                AssertPtr(pReq->pvBounceBuffer);
                offStart     = (pReq->off & ~((RTFOFF)512-1)) + cbTransfered;
                cbToTransfer = pReq->cbBounceBuffer - cbTransfered;
                pbBuf        = (uint8_t *)pReq->pvBounceBuffer + cbTransfered;
            }
            else
            {
                Assert(!pReq->pvBounceBuffer);
                offStart     = pReq->off + cbTransfered;
                cbToTransfer = pReq->DataSeg.cbSeg - cbTransfered;
                pbBuf        = (uint8_t *)pReq->DataSeg.pvSeg + cbTransfered;
            }

            if (   pReq->enmType == RTAIOMGRREQTYPE_PREFETCH
                || pReq->enmType == RTAIOMGRREQTYPE_READ)
            {
                rc = RTFileAioReqPrepareRead(pReq->hReqIo, pFile->hFile, offStart,
                                             pbBuf, cbToTransfer, pReq);
            }
            else
            {
                AssertMsg(pReq->enmType == RTAIOMGRREQTYPE_WRITE,
                              ("Invalid transfer type\n"));
                rc = RTFileAioReqPrepareWrite(pReq->hReqIo, pFile->hFile, offStart,
                                              pbBuf, cbToTransfer, pReq);
            }
            AssertRC(rc);

            rc = rtAioMgrReqsEnqueue(pThis, pFile, &pReq->hReqIo, 1);
            AssertMsg(RT_SUCCESS(rc) || (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES),
                      ("Unexpected return code rc=%Rrc\n", rc));
        }
        else if (pReq->enmType == RTAIOMGRREQTYPE_PREFETCH)
        {
            Assert(pReq->cbBounceBuffer);
            pReq->enmType = RTAIOMGRREQTYPE_WRITE;

            memcpy(((uint8_t *)pReq->pvBounceBuffer) + pReq->offBounceBuffer,
                    pReq->DataSeg.pvSeg,
                    pReq->DataSeg.cbSeg);

            /* Write it now. */
            RTFOFF offStart = pReq->off & ~(RTFOFF)(512-1);
            size_t cbToTransfer = RT_ALIGN_Z(pReq->DataSeg.cbSeg + (pReq->off - offStart), 512);

            rc = RTFileAioReqPrepareWrite(pReq->hReqIo, pFile->hFile,
                                          offStart, pReq->pvBounceBuffer, cbToTransfer, pReq);
            AssertRC(rc);
            rc = rtAioMgrReqsEnqueue(pThis, pFile, &pReq->hReqIo, 1);
            AssertMsg(RT_SUCCESS(rc) || (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES),
                      ("Unexpected return code rc=%Rrc\n", rc));
        }
        else
        {
            if (RT_SUCCESS(rc) && pReq->cbBounceBuffer)
            {
                if (pReq->enmType == RTAIOMGRREQTYPE_READ)
                    memcpy(pReq->DataSeg.pvSeg,
                           ((uint8_t *)pReq->pvBounceBuffer) + pReq->offBounceBuffer,
                           pReq->DataSeg.cbSeg);
            }

            /* Call completion callback */
            pFile->pfnReqCompleted(pFile, rcReq, pReq->pvUser);
            rtAioMgrReqFree(pThis, pReq);
        }
    } /* request completed successfully */
}

/**
 * Wrapper around rtAioMgrReqCompleteRc().
 */
static void rtAioMgrReqComplete(PRTAIOMGRINT pThis, RTFILEAIOREQ hReq)
{
    size_t cbTransfered = 0;
    int rcReq = RTFileAioReqGetRC(hReq, &cbTransfered);
    PRTAIOMGRREQ pReq = (PRTAIOMGRREQ)RTFileAioReqGetUser(hReq);

    rtAioMgrReqCompleteRc(pThis, pReq, rcReq, cbTransfered);
}

/**
 * Wrapper around RTFIleAioCtxSubmit() which is also doing error handling.
 */
static int rtAioMgrReqsEnqueue(PRTAIOMGRINT pThis, PRTAIOMGRFILEINT pFile,
                               PRTFILEAIOREQ pahReqs, unsigned cReqs)
{
    pThis->cReqsActive += cReqs;
    pFile->AioMgr.cReqsActive += cReqs;

    int rc = RTFileAioCtxSubmit(pThis->hAioCtx, pahReqs, cReqs);
    if (RT_FAILURE(rc))
    {
        if (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES)
        {
            /* Append any not submitted task to the waiting list. */
            for (size_t i = 0; i < cReqs; i++)
            {
                int rcReq = RTFileAioReqGetRC(pahReqs[i], NULL);

                if (rcReq != VERR_FILE_AIO_IN_PROGRESS)
                {
                    PRTAIOMGRREQ pReq = (PRTAIOMGRREQ)RTFileAioReqGetUser(pahReqs[i]);

                    Assert(pReq->hReqIo == pahReqs[i]);
                    RTListAppend(&pFile->AioMgr.ListWaitingReqs, &pReq->NodeWaitingList);
                    pThis->cReqsActive--;
                    pFile->AioMgr.cReqsActive--;
                }
            }

            pThis->cReqsActiveMax = pThis->cReqsActive;
            rc = VINF_SUCCESS;
        }
        else /* Another kind of error happened (full disk, ...) */
        {
            /* An error happened. Find out which one caused the error and resubmit all other tasks. */
            for (size_t i = 0; i < cReqs; i++)
            {
                PRTAIOMGRREQ pReq = (PRTAIOMGRREQ)RTFileAioReqGetUser(pahReqs[i]);
                int rcReq = RTFileAioReqGetRC(pahReqs[i], NULL);

                if (rcReq == VERR_FILE_AIO_NOT_SUBMITTED)
                {
                    /* We call ourself again to do any error handling which might come up now. */
                    rc = rtAioMgrReqsEnqueue(pThis, pFile, &pahReqs[i], 1);
                    AssertRC(rc);
                }
                else if (rcReq != VERR_FILE_AIO_IN_PROGRESS)
                    rtAioMgrReqCompleteRc(pThis, pReq, rcReq, 0);
            }
        }
    }

    return VINF_SUCCESS;
}

/**
 * Adds a list of requests to the waiting list.
 *
 * @returns nothing.
 * @param   pFile     The file instance to add the requests to.
 * @param   pReqsHead The head of the request list to add.
 */
static void rtAioMgrFileAddReqsToWaitingList(PRTAIOMGRFILEINT pFile, PRTAIOMGRREQ pReqsHead)
{
    while (pReqsHead)
    {
        PRTAIOMGRREQ pReqCur = pReqsHead;

        pReqsHead = (PRTAIOMGRREQ)pReqsHead->WorkItem.pNext;
        pReqCur->WorkItem.pNext = NULL;
        RTListAppend(&pFile->AioMgr.ListWaitingReqs, &pReqCur->NodeWaitingList);
    }
}

/**
 * Prepare the native I/o request ensuring that all alignment prerequisites of
 * the host are met.
 *
 * @returns IPRT statuse code.
 * @param   pFile    The file instance data.
 * @param   pReq     The request to prepare.
 */
static int rtAioMgrReqPrepareNonBuffered(PRTAIOMGRFILEINT pFile, PRTAIOMGRREQ pReq)
{
    int   rc    = VINF_SUCCESS;
    RTFOFF offStart = pReq->off & ~(RTFOFF)(512-1);
    size_t cbToTransfer = RT_ALIGN_Z(pReq->DataSeg.cbSeg + (pReq->off - offStart), 512);
    void *pvBuf = pReq->DataSeg.pvSeg;
    bool fAlignedReq =     cbToTransfer == pReq->DataSeg.cbSeg
                        && offStart == pReq->off;

    /*
     * Check if the alignment requirements are met.
     * Offset, transfer size and buffer address
     * need to be on a 512 boundary.
     */
    if (   !fAlignedReq
        /** @todo || ((pEpClassFile->uBitmaskAlignment & (RTR3UINTPTR)pvBuf) != (RTR3UINTPTR)pvBuf) */)
    {
        /* Create bounce buffer. */
        pReq->cbBounceBuffer = cbToTransfer;

        AssertMsg(pReq->off >= offStart, ("Overflow in calculation off=%llu offStart=%llu\n",
                  pReq->off, offStart));
        pReq->offBounceBuffer = pReq->off - offStart;

        /** @todo I think we need something like a RTMemAllocAligned method here.
         * Current assumption is that the maximum alignment is 4096byte
         * (GPT disk on Windows)
         * so we can use RTMemPageAlloc here.
         */
        pReq->pvBounceBuffer = RTMemPageAlloc(cbToTransfer);
        if (RT_LIKELY(pReq->pvBounceBuffer))
        {
            pvBuf = pReq->pvBounceBuffer;

            if (pReq->enmType == RTAIOMGRREQTYPE_WRITE)
            {
                if (   RT_UNLIKELY(cbToTransfer != pReq->DataSeg.cbSeg)
                    || RT_UNLIKELY(offStart != pReq->off))
                {
                    /* We have to fill the buffer first before we can update the data. */
                    pReq->enmType = RTAIOMGRREQTYPE_WRITE;
                }
                else
                    memcpy(pvBuf, pReq->DataSeg.pvSeg, pReq->DataSeg.cbSeg);
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        pReq->cbBounceBuffer = 0;

    if (RT_SUCCESS(rc))
    {
        if (pReq->enmType == RTAIOMGRREQTYPE_WRITE)
        {
            rc = RTFileAioReqPrepareWrite(pReq->hReqIo, pFile->hFile,
                                          offStart, pvBuf, cbToTransfer, pReq);
        }
        else /* Read or prefetch request. */
            rc = RTFileAioReqPrepareRead(pReq->hReqIo, pFile->hFile,
                                         offStart, pvBuf, cbToTransfer, pReq);
        AssertRC(rc);
        pReq->fFlags |= RTAIOMGRREQ_FLAGS_PREPARED;
    }

    return rc;
}

/**
 * Prepare a new request for enqueuing.
 *
 * @returns IPRT status code.
 * @param   pReq    The request to prepare.
 * @param   phReqIo Where to store the handle to the native I/O request on success.
 */
static int rtAioMgrPrepareReq(PRTAIOMGRREQ pReq, PRTFILEAIOREQ phReqIo)
{
    int rc = VINF_SUCCESS;
    PRTAIOMGRFILEINT pFile = pReq->pFile;

    switch (pReq->enmType)
    {
        case RTAIOMGRREQTYPE_FLUSH:
        {
            rc = RTFileAioReqPrepareFlush(pReq->hReqIo, pFile->hFile, pReq);
            break;
        }
        case RTAIOMGRREQTYPE_READ:
        case RTAIOMGRREQTYPE_WRITE:
        {
            rc = rtAioMgrReqPrepareNonBuffered(pFile, pReq);
            break;
        }
        default:
            AssertMsgFailed(("Invalid transfer type %d\n", pReq->enmType));
    } /* switch transfer type */

    if (RT_SUCCESS(rc))
        *phReqIo = pReq->hReqIo;

    return rc;
}

/**
 * Prepare newly submitted requests for processing.
 *
 * @returns IPRT status code
 * @param   pThis    The async I/O manager instance data.
 * @param   pFile    The file instance.
 * @param   pReqsNew The list of new requests to prepare.
 */
static int rtAioMgrPrepareNewReqs(PRTAIOMGRINT pThis,
                                  PRTAIOMGRFILEINT pFile,
                                  PRTAIOMGRREQ pReqsNew)
{
    RTFILEAIOREQ  apReqs[20];
    unsigned      cRequests = 0;
    int           rc        = VINF_SUCCESS;

    /* Go through the list and queue the requests. */
    while (   pReqsNew
           && (pThis->cReqsActive + cRequests < pThis->cReqsActiveMax)
           && RT_SUCCESS(rc))
    {
        PRTAIOMGRREQ pCurr = pReqsNew;
        pReqsNew = (PRTAIOMGRREQ)pReqsNew->WorkItem.pNext;

        pCurr->WorkItem.pNext = NULL;
        AssertMsg(VALID_PTR(pCurr->pFile) && (pCurr->pFile == pFile),
                  ("Files do not match\n"));
        AssertMsg(!(pCurr->fFlags & RTAIOMGRREQ_FLAGS_PREPARED),
                  ("Request on the new list is already prepared\n"));

        rc = rtAioMgrPrepareReq(pCurr, &apReqs[cRequests]);
        if (RT_FAILURE(rc))
            rtAioMgrReqCompleteRc(pThis, pCurr, rc, 0);
        else
            cRequests++;

        /* Queue the requests if the array is full. */
        if (cRequests == RT_ELEMENTS(apReqs))
        {
            rc = rtAioMgrReqsEnqueue(pThis, pFile, apReqs, cRequests);
            cRequests = 0;
            AssertMsg(RT_SUCCESS(rc) || (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES),
                      ("Unexpected return code\n"));
        }
    }

    if (cRequests)
    {
        rc = rtAioMgrReqsEnqueue(pThis, pFile, apReqs, cRequests);
        AssertMsg(RT_SUCCESS(rc) || (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES),
                  ("Unexpected return code rc=%Rrc\n", rc));
    }

    if (pReqsNew)
    {
        /* Add the rest of the tasks to the pending list */
        rtAioMgrFileAddReqsToWaitingList(pFile, pReqsNew);
    }

    /* Insufficient resources are not fatal. */
    if (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES)
        rc = VINF_SUCCESS;

    return rc;
}

/**
 * Queues waiting requests.
 *
 * @returns IPRT status code.
 * @param   pThis      The async I/O manager instance data.
 * @param   pFile      The file to get the requests from.
 */
static int rtAioMgrQueueWaitingReqs(PRTAIOMGRINT pThis, PRTAIOMGRFILEINT pFile)
{
    RTFILEAIOREQ  apReqs[20];
    unsigned      cRequests = 0;
    int           rc        = VINF_SUCCESS;

    /* Go through the list and queue the requests. */
    PRTAIOMGRREQ  pReqIt;
    PRTAIOMGRREQ  pReqItNext;
    RTListForEachSafe(&pFile->AioMgr.ListWaitingReqs, pReqIt, pReqItNext, RTAIOMGRREQ, NodeWaitingList)
    {
        RTListNodeRemove(&pReqIt->NodeWaitingList);
        AssertMsg(VALID_PTR(pReqIt->pFile) && (pReqIt->pFile == pFile),
                  ("Files do not match\n"));

        if (!(pReqIt->fFlags & RTAIOMGRREQ_FLAGS_PREPARED))
        {
            rc = rtAioMgrPrepareReq(pReqIt, &apReqs[cRequests]);
            if (RT_FAILURE(rc))
                rtAioMgrReqCompleteRc(pThis, pReqIt, rc, 0);
            else
                cRequests++;
        }
        else
        {
            apReqs[cRequests] = pReqIt->hReqIo;
            cRequests++;
        }

        /* Queue the requests if the array is full. */
        if (cRequests == RT_ELEMENTS(apReqs))
        {
            rc = rtAioMgrReqsEnqueue(pThis, pFile, apReqs, cRequests);
            cRequests = 0;
            AssertMsg(RT_SUCCESS(rc) || (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES),
                      ("Unexpected return code\n"));
        }
    }

    if (cRequests)
    {
        rc = rtAioMgrReqsEnqueue(pThis, pFile, apReqs, cRequests);
        AssertMsg(RT_SUCCESS(rc) || (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES),
                  ("Unexpected return code rc=%Rrc\n", rc));
    }

    /* Insufficient resources are not fatal. */
    if (rc == VERR_FILE_AIO_INSUFFICIENT_RESSOURCES)
        rc = VINF_SUCCESS;

    return rc;
}

/**
 * Adds all pending requests for the given file.
 *
 * @returns IPRT status code.
 * @param   pThis      The async I/O manager instance data.
 * @param   pFile      The file to get the requests from.
 */
static int rtAioMgrQueueReqs(PRTAIOMGRINT pThis, PRTAIOMGRFILEINT pFile)
{
    int rc = VINF_SUCCESS;

    /* Check the pending list first */
    if (!RTListIsEmpty(&pFile->AioMgr.ListWaitingReqs))
        rc = rtAioMgrQueueWaitingReqs(pThis, pFile);

    if (   RT_SUCCESS(rc)
        && RTListIsEmpty(&pFile->AioMgr.ListWaitingReqs))
    {
        PRTAIOMGRREQ pReqsNew = (PRTAIOMGRREQ)RTQueueAtomicRemoveAll(&pFile->QueueReqs);

        if (pReqsNew)
        {
            rc = rtAioMgrPrepareNewReqs(pThis, pFile, pReqsNew);
            AssertRC(rc);
        }
    }

    return rc;
}

/**
 * Checks all files for new requests.
 *
 * @returns IPRT status code.
 * @param   pThis    The I/O manager instance data.
 */
static int rtAioMgrCheckFiles(PRTAIOMGRINT pThis)
{
    int rc = VINF_SUCCESS;

    PRTAIOMGRFILEINT pIt;
    RTListForEach(&pThis->ListFiles, pIt, RTAIOMGRFILEINT, AioMgr.NodeAioMgrFiles)
    {
        rc = rtAioMgrQueueReqs(pThis, pIt);
        if (RT_FAILURE(rc))
            return rc;
    }

    return rc;
}

/**
 * Process a blocking event from the outside.
 *
 * @returns IPRT status code.
 * @param   pThis    The async I/O manager instance data.
 */
static int rtAioMgrProcessBlockingEvent(PRTAIOMGRINT pThis)
{
    int rc = VINF_SUCCESS;
    bool fNotifyWaiter = false;

    switch (pThis->enmBlockingEvent)
    {
        case RTAIOMGREVENT_NO_EVENT:
            /* Nothing to do. */
            break;
        case RTAIOMGREVENT_FILE_ADD:
        {
            PRTAIOMGRFILEINT pFile = ASMAtomicReadPtrT(&pThis->BlockingEventData.pFileAdd, PRTAIOMGRFILEINT);
            AssertMsg(VALID_PTR(pFile), ("Adding file event without a file to add\n"));

            RTListAppend(&pThis->ListFiles, &pFile->AioMgr.NodeAioMgrFiles);
            fNotifyWaiter = true;
            break;
        }
        case RTAIOMGREVENT_FILE_CLOSE:
        {
            PRTAIOMGRFILEINT pFile = ASMAtomicReadPtrT(&pThis->BlockingEventData.pFileClose, PRTAIOMGRFILEINT);
            AssertMsg(VALID_PTR(pFile), ("Close file event without a file to close\n"));

            if (!(pFile->fFlags & RTAIOMGRFILE_FLAGS_CLOSING))
            {
                /* Make sure all requests finished. Process the queues a last time first. */
                rc = rtAioMgrQueueReqs(pThis, pFile);
                AssertRC(rc);

                pFile->fFlags |= RTAIOMGRFILE_FLAGS_CLOSING;
                fNotifyWaiter = !rtAioMgrFileRemove(pFile);
            }
            else if (!pFile->AioMgr.cReqsActive)
                fNotifyWaiter = true;
            break;
        }
        case RTAIOMGREVENT_SHUTDOWN:
        {
            if (!pThis->cReqsActive)
                fNotifyWaiter = true;
            break;
        }
        default:
            AssertReleaseMsgFailed(("Invalid event type %d\n", pThis->enmBlockingEvent));
    }

    if (fNotifyWaiter)
    {
        /* Release the waiting thread. */
        rc = RTSemEventSignal(pThis->hEventSemBlock);
        AssertRC(rc);
    }

    return rc;
}

/**
 * async I/O manager worker loop.
 *
 * @returns IPRT status code.
 * @param   hThreadSelf    The thread handle this worker belongs to.
 * @param   pvUser         Opaque user data (Pointer to async I/O manager instance).
 */
static DECLCALLBACK(int) rtAioMgrWorker(RTTHREAD hThreadSelf, void *pvUser)
{
    PRTAIOMGRINT pThis = (PRTAIOMGRINT)pvUser;
    /*bool fRunning = true;*/
    int rc = VINF_SUCCESS;

    do
    {
        uint32_t cReqsCompleted = 0;
        RTFILEAIOREQ ahReqsCompleted[32];
        rc = RTFileAioCtxWait(pThis->hAioCtx, 1, RT_INDEFINITE_WAIT, &ahReqsCompleted[0],
                              RT_ELEMENTS(ahReqsCompleted), &cReqsCompleted);
        if (rc == VERR_INTERRUPTED)
        {
            /* Process external event. */
            rtAioMgrProcessBlockingEvent(pThis);
            rc = rtAioMgrCheckFiles(pThis);
        }
        else if (RT_FAILURE(rc))
        {
            /* Something bad happened. */
            /** @todo */
        }
        else
        {
            /* Requests completed. */
            for (uint32_t i = 0; i < cReqsCompleted; i++)
                rtAioMgrReqComplete(pThis, ahReqsCompleted[i]);

            /* Check files for new requests and queue waiting requests. */
            rc = rtAioMgrCheckFiles(pThis);
        }
    } while (   /*fRunning - never modified
             && */ RT_SUCCESS(rc));

    RT_NOREF_PV(hThreadSelf);
    return rc;
}

/**
 * Wakes up the async I/O manager.
 *
 * @returns IPRT status code.
 * @param   pThis    The async I/O manager.
 */
static int rtAioMgrWakeup(PRTAIOMGRINT pThis)
{
    return RTFileAioCtxWakeup(pThis->hAioCtx);
}

/**
 * Waits until the async I/O manager handled the given event.
 *
 * @returns IPRT status code.
 * @param   pThis    The async I/O manager.
 * @param   enmEvent The event to pass to the manager.
 */
static int rtAioMgrWaitForBlockingEvent(PRTAIOMGRINT pThis, RTAIOMGREVENT enmEvent)
{
    Assert(pThis->enmBlockingEvent == RTAIOMGREVENT_NO_EVENT);
    ASMAtomicWriteU32((volatile uint32_t *)&pThis->enmBlockingEvent, enmEvent);

    /* Wakeup the async I/O manager */
    int rc = rtAioMgrWakeup(pThis);
    if (RT_FAILURE(rc))
        return rc;

    /* Wait for completion. */
    rc = RTSemEventWait(pThis->hEventSemBlock, RT_INDEFINITE_WAIT);
    AssertRC(rc);

    ASMAtomicWriteU32((volatile uint32_t *)&pThis->enmBlockingEvent, RTAIOMGREVENT_NO_EVENT);

    return rc;
}

/**
 * Add a given file to the given I/O manager.
 *
 * @returns IPRT status code.
 * @param   pThis    The async I/O manager.
 * @param   pFile    The file to add.
 */
static int rtAioMgrAddFile(PRTAIOMGRINT pThis, PRTAIOMGRFILEINT pFile)
{
    /* Update the assigned I/O manager. */
    ASMAtomicWritePtr(&pFile->pAioMgr, pThis);

    int rc = RTCritSectEnter(&pThis->CritSectBlockingEvent);
    AssertRCReturn(rc, rc);

    ASMAtomicWritePtr(&pThis->BlockingEventData.pFileAdd, pFile);
    rc = rtAioMgrWaitForBlockingEvent(pThis, RTAIOMGREVENT_FILE_ADD);
    ASMAtomicWriteNullPtr(&pThis->BlockingEventData.pFileAdd);

    RTCritSectLeave(&pThis->CritSectBlockingEvent);
    return rc;
}

/**
 * Removes a given file from the given I/O manager.
 *
 * @returns IPRT status code.
 * @param   pThis    The async I/O manager.
 * @param   pFile    The file to remove.
 */
static int rtAioMgrCloseFile(PRTAIOMGRINT pThis, PRTAIOMGRFILEINT pFile)
{
    int rc = RTCritSectEnter(&pThis->CritSectBlockingEvent);
    AssertRCReturn(rc, rc);

    ASMAtomicWritePtr(&pThis->BlockingEventData.pFileClose, pFile);
    rc = rtAioMgrWaitForBlockingEvent(pThis, RTAIOMGREVENT_FILE_CLOSE);
    ASMAtomicWriteNullPtr(&pThis->BlockingEventData.pFileClose);

    RTCritSectLeave(&pThis->CritSectBlockingEvent);

    return rc;
}

/**
 * Process a shutdown event.
 *
 * @returns IPRT status code.
 * @param   pThis    The async I/O manager to shut down.
 */
static int rtAioMgrShutdown(PRTAIOMGRINT pThis)
{
    int rc = RTCritSectEnter(&pThis->CritSectBlockingEvent);
    AssertRCReturn(rc, rc);

    rc = rtAioMgrWaitForBlockingEvent(pThis, RTAIOMGREVENT_SHUTDOWN);
    RTCritSectLeave(&pThis->CritSectBlockingEvent);

    return rc;
}

/**
 * Destroys an async I/O manager.
 *
 * @returns nothing.
 * @param   pThis             The async I/O manager instance to destroy.
 */
static void rtAioMgrDestroy(PRTAIOMGRINT pThis)
{
    int rc;

    rc = rtAioMgrShutdown(pThis);
    AssertRC(rc);

    rc = RTThreadWait(pThis->hThread, RT_INDEFINITE_WAIT, NULL);
    AssertRC(rc);

    rc = RTFileAioCtxDestroy(pThis->hAioCtx);
    AssertRC(rc);

    rc = RTMemCacheDestroy(pThis->hMemCacheReqs);
    AssertRC(rc);

    pThis->hThread       = NIL_RTTHREAD;
    pThis->hAioCtx       = NIL_RTFILEAIOCTX;
    pThis->hMemCacheReqs = NIL_RTMEMCACHE;
    pThis->u32Magic      = ~RTAIOMGR_MAGIC;
    RTCritSectDelete(&pThis->CritSectBlockingEvent);
    RTSemEventDestroy(pThis->hEventSemBlock);
    RTMemFree(pThis);
}

/**
 * Queues a new request for processing.
 */
static void rtAioMgrFileQueueReq(PRTAIOMGRFILEINT pThis, PRTAIOMGRREQ pReq)
{
    RTAioMgrFileRetain(pThis);
    RTQueueAtomicInsert(&pThis->QueueReqs, &pReq->WorkItem);
    rtAioMgrWakeup(pThis->pAioMgr);
}

/**
 * Destroys an async I/O manager file.
 *
 * @returns nothing.
 * @param   pThis             The async I/O manager file.
 */
static void rtAioMgrFileDestroy(PRTAIOMGRFILEINT pThis)
{
    pThis->u32Magic = ~RTAIOMGRFILE_MAGIC;
    rtAioMgrCloseFile(pThis->pAioMgr, pThis);
    RTAioMgrRelease(pThis->pAioMgr);
    RTMemFree(pThis);
}

/**
 * Queues a new I/O request.
 *
 * @returns IPRT status code.
 * @param   hAioMgrFile    The I/O manager file handle.
 * @param   off            Start offset of the I/o request.
 * @param   pSgBuf         Data S/G buffer.
 * @param   cbIo           How much to transfer.
 * @param   pvUser         Opaque user data.
 * @param   enmType        I/O direction type (read/write).
 */
static int rtAioMgrFileIoReqCreate(RTAIOMGRFILE hAioMgrFile, RTFOFF off, PRTSGBUF pSgBuf,
                                   size_t cbIo, void *pvUser, RTAIOMGRREQTYPE enmType)
{
    int rc;
    PRTAIOMGRFILEINT pFile = hAioMgrFile;
    PRTAIOMGRINT pAioMgr;

    AssertPtrReturn(pFile, VERR_INVALID_HANDLE);
    pAioMgr = pFile->pAioMgr;

    PRTAIOMGRREQ pReq = rtAioMgrReqAlloc(pAioMgr);
    if (RT_LIKELY(pReq))
    {
        unsigned cSeg = 1;
        size_t cbSeg = RTSgBufSegArrayCreate(pSgBuf, &pReq->DataSeg, &cSeg, cbIo);

        if (cbSeg == cbIo)
        {
            pReq->enmType = enmType;
            pReq->pFile   = pFile;
            pReq->pvUser  = pvUser;
            pReq->off     = off;
            rtAioMgrFileQueueReq(pFile, pReq);
            rc = VERR_FILE_AIO_IN_PROGRESS;
        }
        else
        {
            /** @todo Real S/G buffer support. */
            rtAioMgrReqFree(pAioMgr, pReq);
            rc = VERR_NOT_SUPPORTED;
        }
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

/**
 * Request constructor for the memory cache.
 *
 * @returns IPRT status code.
 * @param   hMemCache           The cache handle.
 * @param   pvObj               The memory object that should be initialized.
 * @param   pvUser              The user argument.
 */
static DECLCALLBACK(int) rtAioMgrReqCtor(RTMEMCACHE hMemCache, void *pvObj, void *pvUser)
{
    PRTAIOMGRREQ pReq = (PRTAIOMGRREQ)pvObj;
    RT_NOREF_PV(hMemCache); RT_NOREF_PV(pvUser);

    memset(pReq, 0, sizeof(RTAIOMGRREQ));
    return RTFileAioReqCreate(&pReq->hReqIo);
}

/**
 * Request destructor for the memory cache.
 *
 * @param   hMemCache           The cache handle.
 * @param   pvObj               The memory object that should be destroyed.
 * @param   pvUser              The user argument.
 */
static DECLCALLBACK(void) rtAioMgrReqDtor(RTMEMCACHE hMemCache, void *pvObj, void *pvUser)
{
    PRTAIOMGRREQ pReq = (PRTAIOMGRREQ)pvObj;
    int rc = RTFileAioReqDestroy(pReq->hReqIo);
    AssertRC(rc);

    RT_NOREF_PV(hMemCache); RT_NOREF_PV(pvUser);
}

RTDECL(int) RTAioMgrCreate(PRTAIOMGR phAioMgr, uint32_t cReqsMax)
{
    int rc = VINF_SUCCESS;
    PRTAIOMGRINT pThis;

    AssertPtrReturn(phAioMgr, VERR_INVALID_POINTER);
    AssertReturn(cReqsMax > 0, VERR_INVALID_PARAMETER);

    pThis = (PRTAIOMGRINT)RTMemAllocZ(sizeof(RTAIOMGRINT));
    if (pThis)
    {
        pThis->u32Magic = RTAIOMGR_MAGIC;
        pThis->cRefs    = 1;
        pThis->enmBlockingEvent = RTAIOMGREVENT_NO_EVENT;
        RTListInit(&pThis->ListFiles);
        rc = RTCritSectInit(&pThis->CritSectBlockingEvent);
        if (RT_SUCCESS(rc))
        {
            rc = RTSemEventCreate(&pThis->hEventSemBlock);
            if (RT_SUCCESS(rc))
            {
                rc = RTMemCacheCreate(&pThis->hMemCacheReqs, sizeof(RTAIOMGRREQ),
                                      0, UINT32_MAX, rtAioMgrReqCtor, rtAioMgrReqDtor, NULL, 0);
                if (RT_SUCCESS(rc))
                {
                    rc = RTFileAioCtxCreate(&pThis->hAioCtx, cReqsMax == UINT32_MAX
                                                             ? RTFILEAIO_UNLIMITED_REQS
                                                             : cReqsMax,
                                            RTFILEAIOCTX_FLAGS_WAIT_WITHOUT_PENDING_REQUESTS);
                    if (RT_SUCCESS(rc))
                    {
                        rc = RTThreadCreateF(&pThis->hThread, rtAioMgrWorker, pThis, 0, RTTHREADTYPE_IO,
                                             RTTHREADFLAGS_WAITABLE, "AioMgr-%u", cReqsMax);
                        if (RT_FAILURE(rc))
                        {
                            rc = RTFileAioCtxDestroy(pThis->hAioCtx);
                            AssertRC(rc);
                        }
                    }

                    if (RT_FAILURE(rc))
                        RTMemCacheDestroy(pThis->hMemCacheReqs);
                }

                if (RT_FAILURE(rc))
                    RTSemEventDestroy(pThis->hEventSemBlock);
            }

            if (RT_FAILURE(rc))
                RTCritSectDelete(&pThis->CritSectBlockingEvent);
        }

        if (RT_FAILURE(rc))
            RTMemFree(pThis);
    }
    else
        rc = VERR_NO_MEMORY;

    if (RT_SUCCESS(rc))
        *phAioMgr = pThis;

    return rc;
}

RTDECL(uint32_t) RTAioMgrRetain(RTAIOMGR hAioMgr)
{
    PRTAIOMGRINT pThis = hAioMgr;
    AssertReturn(hAioMgr != NIL_RTAIOMGR, UINT32_MAX);
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->u32Magic == RTAIOMGR_MAGIC, UINT32_MAX);

    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    AssertMsg(cRefs > 1 && cRefs < _1G, ("%#x %p\n", cRefs, pThis));
    return cRefs;
}

RTDECL(uint32_t) RTAioMgrRelease(RTAIOMGR hAioMgr)
{
    PRTAIOMGRINT pThis = hAioMgr;
    if (pThis == NIL_RTAIOMGR)
        return 0;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->u32Magic == RTAIOMGR_MAGIC, UINT32_MAX);

    uint32_t cRefs = ASMAtomicDecU32(&pThis->cRefs);
    AssertMsg(cRefs < _1G, ("%#x %p\n", cRefs, pThis));
    if (cRefs == 0)
        rtAioMgrDestroy(pThis);
    return cRefs;
}

RTDECL(int) RTAioMgrFileCreate(RTAIOMGR hAioMgr, RTFILE hFile, PFNRTAIOMGRREQCOMPLETE pfnReqComplete,
                               void *pvUser, PRTAIOMGRFILE phAioMgrFile)
{
    int rc = VINF_SUCCESS;
    PRTAIOMGRFILEINT pThis;

    AssertReturn(hAioMgr != NIL_RTAIOMGR, VERR_INVALID_HANDLE);
    AssertReturn(hFile != NIL_RTFILE, VERR_INVALID_HANDLE);
    AssertPtrReturn(pfnReqComplete, VERR_INVALID_POINTER);
    AssertPtrReturn(phAioMgrFile, VERR_INVALID_POINTER);

    pThis = (PRTAIOMGRFILEINT)RTMemAllocZ(sizeof(RTAIOMGRFILEINT));
    if (pThis)
    {
        pThis->u32Magic = RTAIOMGRFILE_MAGIC;
        pThis->cRefs    = 1;
        pThis->hFile    = hFile;
        pThis->pAioMgr  = hAioMgr;
        pThis->pvUser   = pvUser;
        pThis->pfnReqCompleted = pfnReqComplete;
        RTQueueAtomicInit(&pThis->QueueReqs);
        RTListInit(&pThis->AioMgr.ListWaitingReqs);
        RTAioMgrRetain(hAioMgr);
        rc = RTFileAioCtxAssociateWithFile(pThis->pAioMgr->hAioCtx, hFile);
        if (RT_FAILURE(rc))
            rtAioMgrFileDestroy(pThis);
        else
            rtAioMgrAddFile(pThis->pAioMgr, pThis);
    }
    else
        rc = VERR_NO_MEMORY;

    if (RT_SUCCESS(rc))
        *phAioMgrFile = pThis;

    return rc;
}

RTDECL(uint32_t) RTAioMgrFileRetain(RTAIOMGRFILE hAioMgrFile)
{
    PRTAIOMGRFILEINT pThis = hAioMgrFile;
    AssertReturn(hAioMgrFile != NIL_RTAIOMGRFILE, UINT32_MAX);
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->u32Magic == RTAIOMGRFILE_MAGIC, UINT32_MAX);

    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    AssertMsg(cRefs > 1 && cRefs < _1G, ("%#x %p\n", cRefs, pThis));
    return cRefs;
}

RTDECL(uint32_t) RTAioMgrFileRelease(RTAIOMGRFILE hAioMgrFile)
{
    PRTAIOMGRFILEINT pThis = hAioMgrFile;
    if (pThis == NIL_RTAIOMGRFILE)
        return 0;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->u32Magic == RTAIOMGRFILE_MAGIC, UINT32_MAX);

    uint32_t cRefs = ASMAtomicDecU32(&pThis->cRefs);
    AssertMsg(cRefs < _1G, ("%#x %p\n", cRefs, pThis));
    if (cRefs == 0)
        rtAioMgrFileDestroy(pThis);
    return cRefs;
}

RTDECL(void *) RTAioMgrFileGetUser(RTAIOMGRFILE hAioMgrFile)
{
    PRTAIOMGRFILEINT pThis = hAioMgrFile;

    AssertPtrReturn(pThis, NULL);
    return pThis->pvUser;
}

RTDECL(int) RTAioMgrFileRead(RTAIOMGRFILE hAioMgrFile, RTFOFF off,
                             PRTSGBUF pSgBuf, size_t cbRead, void *pvUser)
{
    return rtAioMgrFileIoReqCreate(hAioMgrFile, off, pSgBuf, cbRead, pvUser,
                                   RTAIOMGRREQTYPE_READ);
}

RTDECL(int) RTAioMgrFileWrite(RTAIOMGRFILE hAioMgrFile, RTFOFF off,
                              PRTSGBUF pSgBuf, size_t cbWrite, void *pvUser)
{
    return rtAioMgrFileIoReqCreate(hAioMgrFile, off, pSgBuf, cbWrite, pvUser,
                                   RTAIOMGRREQTYPE_WRITE);
}

RTDECL(int) RTAioMgrFileFlush(RTAIOMGRFILE hAioMgrFile, void *pvUser)
{
    PRTAIOMGRFILEINT pFile = hAioMgrFile;
    PRTAIOMGRINT pAioMgr;

    AssertPtrReturn(pFile, VERR_INVALID_HANDLE);

    pAioMgr = pFile->pAioMgr;

    PRTAIOMGRREQ pReq = rtAioMgrReqAlloc(pAioMgr);
    if (RT_UNLIKELY(!pReq))
        return VERR_NO_MEMORY;

    pReq->pFile   = pFile;
    pReq->enmType = RTAIOMGRREQTYPE_FLUSH;
    pReq->pvUser  = pvUser;
    rtAioMgrFileQueueReq(pFile, pReq);

    return VERR_FILE_AIO_IN_PROGRESS;
}


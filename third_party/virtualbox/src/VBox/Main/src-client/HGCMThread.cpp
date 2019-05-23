/* $Id: HGCMThread.cpp $ */
/** @file
 * HGCMThread - Host-Guest Communication Manager Threads
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

#define LOG_GROUP LOG_GROUP_HGCM
#include "LoggingNew.h"

#include "HGCMThread.h"

#include <VBox/err.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/string.h>


/* HGCM uses worker threads, which process messages from other threads.
 * A message consists of the message header and message specific data.
 * Message header is opaque for callers, but message data is defined
 * and used by them.
 *
 * Messages are distinguished by message identifier and worker thread
 * they are allocated for.
 *
 * Messages are allocated for a worker thread and belong to
 * the thread. A worker thread holds the queue of messages.
 *
 * The calling thread creates a message, specifying which worker thread
 * the message is created for, then, optionally, initializes message
 * specific data and, also optionally, references the message.
 *
 * Message then is posted or sent to worker thread by inserting
 * it to the worker thread message queue and referencing the message.
 * Worker thread then again may fetch next message.
 *
 * Upon processing the message the worker thread dereferences it.
 * Dereferencing also automatically deletes message from the thread
 * queue and frees memory allocated for the message, if no more
 * references left. If there are references, the message remains
 * in the queue.
 *
 */

/* Version of HGCM message header */
#define HGCMMSG_VERSION (1)

/* Thread is initializing. */
#define HGCMMSG_TF_INITIALIZING        (0x00000001)
/* Thread must be terminated. */
#define HGCMMSG_TF_TERMINATE           (0x00000002)
/* Thread has been terminated. */
#define HGCMMSG_TF_TERMINATED          (0x00000004)

/** @todo consider use of RTReq */

static DECLCALLBACK(int) hgcmWorkerThreadFunc (RTTHREAD ThreadSelf, void *pvUser);

class HGCMThread: public HGCMObject
{
    private:
    friend DECLCALLBACK(int) hgcmWorkerThreadFunc (RTTHREAD ThreadSelf, void *pvUser);

        /* Worker thread function. */
        PFNHGCMTHREAD m_pfnThread;

        /* A user supplied thread parameter. */
        void *m_pvUser;

        /* The thread runtime handle. */
        RTTHREAD m_thread;

        /* Event the thread waits for, signalled when a message
         * to process is posted to the thread.
         */
        RTSEMEVENTMULTI m_eventThread;

        /* A caller thread waits for completion of a SENT message on this event. */
        RTSEMEVENTMULTI m_eventSend;
        int32_t volatile m_i32MessagesProcessed;

        /* Critical section for accessing the thread data, mostly for message queues. */
        RTCRITSECT m_critsect;

        /* thread state/operation flags */
        uint32_t m_fu32ThreadFlags;

        /* Message queue variables. Messages are inserted at tail of message
         * queue. They are consumed by worker thread sequentially. If a message was
         * consumed, it is removed from message queue.
         */

        /* Head of message queue. */
        HGCMMsgCore *m_pMsgInputQueueHead;
        /* Message which another message will be inserted after. */
        HGCMMsgCore *m_pMsgInputQueueTail;

        /* Head of messages being processed queue. */
        HGCMMsgCore *m_pMsgInProcessHead;
        /* Message which another message will be inserted after. */
        HGCMMsgCore *m_pMsgInProcessTail;

        /* Head of free message structures list. */
        HGCMMsgCore *m_pFreeHead;
        /* Tail of free message structures list. */
        HGCMMsgCore *m_pFreeTail;

        HGCMTHREADHANDLE m_handle;

        inline int Enter (void);
        inline void Leave (void);

        HGCMMsgCore *FetchFreeListHead (void);

    protected:
        virtual ~HGCMThread (void);

    public:

        HGCMThread ();

        int WaitForTermination (void);

        int Initialize (HGCMTHREADHANDLE handle, const char *pszThreadName, PFNHGCMTHREAD pfnThread, void *pvUser);

        int MsgAlloc (HGCMMSGHANDLE *pHandle, uint32_t u32MsgId, PFNHGCMNEWMSGALLOC pfnNewMessage);
        int MsgGet (HGCMMsgCore **ppMsg);
        int MsgPost (HGCMMsgCore *pMsg, PHGCMMSGCALLBACK pfnCallback, bool bWait);
        void MsgComplete (HGCMMsgCore *pMsg, int32_t result);
};


/*
 * HGCMMsgCore implementation.
 */

#define HGCM_MSG_F_PROCESSED  (0x00000001)
#define HGCM_MSG_F_WAIT       (0x00000002)
#define HGCM_MSG_F_IN_PROCESS (0x00000004)

void HGCMMsgCore::InitializeCore (uint32_t u32MsgId, HGCMTHREADHANDLE hThread)
{
    m_u32Version  = HGCMMSG_VERSION;
    m_u32Msg      = u32MsgId;
    m_pfnCallback = NULL;
    m_pNext       = NULL;
    m_pPrev       = NULL;
    m_fu32Flags   = 0;
    m_rcSend      = VINF_SUCCESS;

    m_pThread = (HGCMThread *)hgcmObjReference (hThread, HGCMOBJ_THREAD);
    AssertRelease (m_pThread);
}

/* virtual */ HGCMMsgCore::~HGCMMsgCore ()
{
    if (m_pThread)
    {
        hgcmObjDereference (m_pThread);
        m_pThread = NULL;
    }
}

/*
 * HGCMThread implementation.
 */

static DECLCALLBACK(int) hgcmWorkerThreadFunc (RTTHREAD ThreadSelf, void *pvUser)
{
    int rc = VINF_SUCCESS;

    HGCMThread *pThread = (HGCMThread *)pvUser;

    LogFlow(("MAIN::hgcmWorkerThreadFunc: starting HGCM thread %p\n", pThread));

    AssertRelease(pThread);

    pThread->m_thread = ThreadSelf;
    pThread->m_fu32ThreadFlags &= ~HGCMMSG_TF_INITIALIZING;
    rc = RTThreadUserSignal (ThreadSelf);
    AssertRC(rc);

    pThread->m_pfnThread (pThread->Handle (), pThread->m_pvUser);

    pThread->m_fu32ThreadFlags |= HGCMMSG_TF_TERMINATED;

    pThread->m_thread = NIL_RTTHREAD;

    LogFlow(("MAIN::hgcmWorkerThreadFunc: completed HGCM thread %p\n", pThread));

    return rc;
}

HGCMThread::HGCMThread ()
    :
    HGCMObject(HGCMOBJ_THREAD),
    m_pfnThread (NULL),
    m_pvUser (NULL),
    m_thread (NIL_RTTHREAD),
    m_eventThread (0),
    m_eventSend (0),
    m_i32MessagesProcessed (0),
    m_fu32ThreadFlags (0),
    m_pMsgInputQueueHead (NULL),
    m_pMsgInputQueueTail (NULL),
    m_pMsgInProcessHead (NULL),
    m_pMsgInProcessTail (NULL),
    m_pFreeHead (NULL),
    m_pFreeTail (NULL),
    m_handle (0)
{
    RT_ZERO(m_critsect);
}

HGCMThread::~HGCMThread ()
{
    /*
     * Free resources allocated for the thread.
     */

    Assert(m_fu32ThreadFlags & HGCMMSG_TF_TERMINATED);

    if (RTCritSectIsInitialized (&m_critsect))
    {
        RTCritSectDelete (&m_critsect);
    }

    if (m_eventSend)
    {
        RTSemEventMultiDestroy (m_eventSend);
    }

    if (m_eventThread)
    {
        RTSemEventMultiDestroy (m_eventThread);
    }

    return;
}

int HGCMThread::WaitForTermination (void)
{
    int rc = VINF_SUCCESS;
    LogFlowFunc(("\n"));

    if (m_thread != NIL_RTTHREAD)
    {
        rc = RTThreadWait (m_thread, 5000, NULL);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int HGCMThread::Initialize (HGCMTHREADHANDLE handle, const char *pszThreadName, PFNHGCMTHREAD pfnThread, void *pvUser)
{
    int rc = VINF_SUCCESS;

    rc = RTSemEventMultiCreate (&m_eventThread);

    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventMultiCreate (&m_eventSend);

        if (RT_SUCCESS(rc))
        {
            rc = RTCritSectInit (&m_critsect);

            if (RT_SUCCESS(rc))
            {
                m_pfnThread = pfnThread;
                m_pvUser    = pvUser;
                m_handle    = handle;

                m_fu32ThreadFlags = HGCMMSG_TF_INITIALIZING;

                RTTHREAD thread;
                rc = RTThreadCreate (&thread, hgcmWorkerThreadFunc, this, 0, /* default stack size; some service
                                                                                may need quite a bit */
                                     RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE,
                                     pszThreadName);

                if (RT_SUCCESS(rc))
                {
                    /* Wait until the thread is ready. */
                    rc = RTThreadUserWait (thread, 30000);
                    AssertRC(rc);
                    Assert(!(m_fu32ThreadFlags & HGCMMSG_TF_INITIALIZING) || RT_FAILURE(rc));
                }
                else
                {
                    m_thread = NIL_RTTHREAD;
                    Log(("hgcmThreadCreate: FAILURE: Can't start worker thread.\n"));
                }
            }
            else
            {
                Log(("hgcmThreadCreate: FAILURE: Can't init a critical section for a hgcm worker thread.\n"));
                RT_ZERO(m_critsect);
            }
        }
        else
        {
            Log(("hgcmThreadCreate: FAILURE: Can't create an event semaphore for a sent messages.\n"));
            m_eventSend = 0;
        }
    }
    else
    {
        Log(("hgcmThreadCreate: FAILURE: Can't create an event semaphore for a hgcm worker thread.\n"));
        m_eventThread = 0;
    }

    return rc;
}

inline int HGCMThread::Enter (void)
{
    int rc = RTCritSectEnter (&m_critsect);

#ifdef LOG_ENABLED
    if (RT_FAILURE(rc))
    {
        Log(("HGCMThread::MsgPost: FAILURE: could not obtain worker thread mutex, rc = %Rrc!!!\n", rc));
    }
#endif /* LOG_ENABLED */

    return rc;
}

inline void HGCMThread::Leave (void)
{
    RTCritSectLeave (&m_critsect);
}


int HGCMThread::MsgAlloc (HGCMMSGHANDLE *pHandle, uint32_t u32MsgId, PFNHGCMNEWMSGALLOC pfnNewMessage)
{
    int rc = VINF_SUCCESS;

    HGCMMsgCore *pmsg = NULL;

    bool fFromFreeList = false;

    if (!pmsg && RT_SUCCESS(rc))
    {
        /* We have to allocate a new memory block. */
        pmsg = pfnNewMessage (u32MsgId);

        if (pmsg == NULL)
        {
            rc = VERR_NO_MEMORY;
        }
    }

    if (RT_SUCCESS(rc))
    {
        /* Initialize just allocated message core */
        pmsg->InitializeCore (u32MsgId, m_handle);

        /* and the message specific data. */
        pmsg->Initialize ();

        LogFlow(("MAIN::hgcmMsgAlloc: allocated message %p\n", pmsg));

        /** Get handle of the message. The message will be also referenced
         *  until the handle is deleted.
         */
        *pHandle = hgcmObjGenerateHandle (pmsg);

        if (fFromFreeList)
        {
            /* Message was referenced in the free list, now dereference it. */
            pmsg->Dereference ();
        }
    }

    return rc;
}

int HGCMThread::MsgPost (HGCMMsgCore *pMsg, PHGCMMSGCALLBACK pfnCallback, bool fWait)
{
    int rc = VINF_SUCCESS;

    LogFlow(("HGCMThread::MsgPost: thread = %p, pMsg = %p, pfnCallback = %p\n", this, pMsg, pfnCallback));

    rc = Enter ();

    if (RT_SUCCESS(rc))
    {
        pMsg->m_pfnCallback = pfnCallback;

        if (fWait)
        {
            pMsg->m_fu32Flags |= HGCM_MSG_F_WAIT;
        }

        /* Insert the message to the queue tail. */
        pMsg->m_pNext = NULL;
        pMsg->m_pPrev = m_pMsgInputQueueTail;

        if (m_pMsgInputQueueTail)
        {
            m_pMsgInputQueueTail->m_pNext = pMsg;
        }
        else
        {
            m_pMsgInputQueueHead = pMsg;
        }

        m_pMsgInputQueueTail = pMsg;

        Leave ();

        LogFlow(("HGCMThread::MsgPost: going to inform the thread %p about message, fWait = %d\n", this, fWait));

        /* Inform the worker thread that there is a message. */
        RTSemEventMultiSignal (m_eventThread);

        LogFlow(("HGCMThread::MsgPost: event signalled\n"));

        if (fWait)
        {
            /* Immediately check if the message has been processed. */
            while ((pMsg->m_fu32Flags & HGCM_MSG_F_PROCESSED) == 0)
            {
                /* Poll infrequently to make sure no completed message has been missed. */
                RTSemEventMultiWait (m_eventSend, 1000);

                LogFlow(("HGCMThread::MsgPost: wait completed flags = %08X\n", pMsg->m_fu32Flags));

                if ((pMsg->m_fu32Flags & HGCM_MSG_F_PROCESSED) == 0)
                {
                    RTThreadYield();
                }
            }

            /* 'Our' message has been processed, so should reset the semaphore.
             * There is still possible that another message has been processed
             * and the semaphore has been signalled again.
             * Reset only if there are no other messages completed.
             */
            int32_t c = ASMAtomicDecS32(&m_i32MessagesProcessed);
            Assert(c >= 0);
            if (c == 0)
            {
                RTSemEventMultiReset (m_eventSend);
            }

            rc = pMsg->m_rcSend;
        }
    }

    LogFlow(("HGCMThread::MsgPost: rc = %Rrc\n", rc));

    return rc;
}


int HGCMThread::MsgGet (HGCMMsgCore **ppMsg)
{
    int rc = VINF_SUCCESS;

    LogFlow(("HGCMThread::MsgGet: thread = %p, ppMsg = %p\n", this, ppMsg));

    for (;;)
    {
        if (m_fu32ThreadFlags & HGCMMSG_TF_TERMINATE)
        {
            rc = VERR_INTERRUPTED;
            break;
        }

        LogFlow(("MAIN::hgcmMsgGet: m_pMsgInputQueueHead = %p\n", m_pMsgInputQueueHead));

        if (m_pMsgInputQueueHead)
        {
            /* Move the message to the m_pMsgInProcessHead list */
            rc = Enter ();

            if (RT_FAILURE(rc))
            {
                break;
            }

            HGCMMsgCore *pMsg = m_pMsgInputQueueHead;

            /* Remove the message from the head of Queue list. */
            Assert(m_pMsgInputQueueHead->m_pPrev == NULL);

            if (m_pMsgInputQueueHead->m_pNext)
            {
                m_pMsgInputQueueHead = m_pMsgInputQueueHead->m_pNext;
                m_pMsgInputQueueHead->m_pPrev = NULL;
            }
            else
            {
                Assert(m_pMsgInputQueueHead == m_pMsgInputQueueTail);

                m_pMsgInputQueueHead = NULL;
                m_pMsgInputQueueTail = NULL;
            }

            /* Insert the message to the tail of the m_pMsgInProcessHead list. */
            pMsg->m_pNext = NULL;
            pMsg->m_pPrev = m_pMsgInProcessTail;

            if (m_pMsgInProcessTail)
            {
                m_pMsgInProcessTail->m_pNext = pMsg;
            }
            else
            {
                m_pMsgInProcessHead = pMsg;
            }

            m_pMsgInProcessTail = pMsg;

            pMsg->m_fu32Flags |= HGCM_MSG_F_IN_PROCESS;

            Leave ();

            /* Return the message to the caller. */
            *ppMsg = pMsg;

            LogFlow(("MAIN::hgcmMsgGet: got message %p\n", *ppMsg));

            break;
        }

        /* Wait for an event. */
        RTSemEventMultiWait (m_eventThread, RT_INDEFINITE_WAIT);
        RTSemEventMultiReset (m_eventThread);
    }

    LogFlow(("HGCMThread::MsgGet: *ppMsg = %p, return rc = %Rrc\n", *ppMsg, rc));

    return rc;
}

void HGCMThread::MsgComplete (HGCMMsgCore *pMsg, int32_t result)
{
    LogFlow(("HGCMThread::MsgComplete: thread = %p, pMsg = %p\n", this, pMsg));

    int rc = VINF_SUCCESS;

    AssertRelease(pMsg->m_pThread == this);
    AssertReleaseMsg((pMsg->m_fu32Flags & HGCM_MSG_F_IN_PROCESS) != 0, ("%p %x\n", pMsg, pMsg->m_fu32Flags));

    if (pMsg->m_pfnCallback)
    {
        /** @todo call callback with error code in MsgPost in case of errors */

        pMsg->m_pfnCallback (result, pMsg);

        LogFlow(("HGCMThread::MsgComplete: callback executed. pMsg = %p, thread = %p\n", pMsg, this));
    }

    /* Message processing has been completed. */

    rc = Enter ();

    if (RT_SUCCESS(rc))
    {
        /* Remove the message from the InProcess queue. */

        if (pMsg->m_pNext)
        {
            pMsg->m_pNext->m_pPrev = pMsg->m_pPrev;
        }
        else
        {
            m_pMsgInProcessTail = pMsg->m_pPrev;
        }

        if (pMsg->m_pPrev)
        {
            pMsg->m_pPrev->m_pNext = pMsg->m_pNext;
        }
        else
        {
            m_pMsgInProcessHead = pMsg->m_pNext;
        }

        pMsg->m_pNext = NULL;
        pMsg->m_pPrev = NULL;

        bool fWaited = ((pMsg->m_fu32Flags & HGCM_MSG_F_WAIT) != 0);

        if (fWaited)
        {
            ASMAtomicIncS32(&m_i32MessagesProcessed);

            /* This should be done before setting the HGCM_MSG_F_PROCESSED flag. */
            pMsg->m_rcSend = result;
        }

        /* The message is now completed. */
        pMsg->m_fu32Flags &= ~HGCM_MSG_F_IN_PROCESS;
        pMsg->m_fu32Flags &= ~HGCM_MSG_F_WAIT;
        pMsg->m_fu32Flags |= HGCM_MSG_F_PROCESSED;

        hgcmObjDeleteHandle (pMsg->Handle ());

        Leave ();

        if (fWaited)
        {
            /* Wake up all waiters. so they can decide if their message has been processed. */
            RTSemEventMultiSignal (m_eventSend);
        }
    }

    return;
}

/*
 * Thread API. Public interface.
 */

int hgcmThreadCreate (HGCMTHREADHANDLE *pHandle, const char *pszThreadName, PFNHGCMTHREAD pfnThread, void *pvUser)
{
    int rc = VINF_SUCCESS;

    LogFlow(("MAIN::hgcmThreadCreate\n"));

    HGCMTHREADHANDLE handle = 0;

    /* Allocate memory for a new thread object. */
    HGCMThread *pThread = new HGCMThread ();

    if (pThread)
    {
        /* Put just created object to pool and obtain handle for it. */
        handle = hgcmObjGenerateHandle (pThread);

        /* Initialize the object. */
        rc = pThread->Initialize (handle, pszThreadName, pfnThread, pvUser);
    }
    else
    {
        Log(("hgcmThreadCreate: FAILURE: Can't allocate memory for a hgcm worker thread.\n"));
        rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        *pHandle = handle;
    }
    else
    {
        Log(("hgcmThreadCreate: FAILURE: rc = %Rrc.\n", rc));

        if (handle != 0)
        {
            /* Delete allocated handle, this will also free the object memory. */
            hgcmObjDeleteHandle (handle);
        }
    }

    LogFlow(("MAIN::hgcmThreadCreate: rc = %Rrc\n", rc));

    return rc;
}

int hgcmThreadWait (HGCMTHREADHANDLE hThread)
{
    int rc = VERR_INVALID_HANDLE;
    LogFlowFunc(("0x%08X\n", hThread));

    HGCMThread *pThread = (HGCMThread *)hgcmObjReference (hThread, HGCMOBJ_THREAD);

    if (pThread)
    {
        rc = pThread->WaitForTermination ();

        hgcmObjDereference (pThread);
    }

    hgcmObjDeleteHandle (hThread);

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int hgcmMsgAlloc (HGCMTHREADHANDLE hThread, HGCMMSGHANDLE *pHandle, uint32_t u32MsgId, PFNHGCMNEWMSGALLOC pfnNewMessage)
{
    LogFlow(("hgcmMsgAlloc: thread handle = 0x%08X, pHandle = %p, sizeof (HGCMMsgCore) = %d\n",
             hThread, pHandle, sizeof (HGCMMsgCore)));

    if (!pHandle)
    {
        return VERR_INVALID_PARAMETER;
    }

    int rc = VINF_SUCCESS;

    HGCMThread *pThread = (HGCMThread *)hgcmObjReference (hThread, HGCMOBJ_THREAD);

    if (!pThread)
    {
        rc = VERR_INVALID_HANDLE;
    }
    else
    {
        rc = pThread->MsgAlloc (pHandle, u32MsgId, pfnNewMessage);

        hgcmObjDereference (pThread);
    }

    LogFlow(("MAIN::hgcmMsgAlloc: handle 0x%08X, rc = %Rrc\n", *pHandle, rc));

    return rc;
}

static int hgcmMsgPostInternal (HGCMMSGHANDLE hMsg, PHGCMMSGCALLBACK pfnCallback, bool fWait)
{
    LogFlow(("MAIN::hgcmMsgPostInternal: hMsg = 0x%08X, pfnCallback = %p, fWait = %d\n", hMsg, pfnCallback, fWait));

    int rc = VINF_SUCCESS;

    HGCMMsgCore *pMsg = (HGCMMsgCore *)hgcmObjReference (hMsg, HGCMOBJ_MSG);

    if (!pMsg)
    {
        rc = VERR_INVALID_HANDLE;
    }
    else
    {
        rc = pMsg->Thread()->MsgPost (pMsg, pfnCallback, fWait);

        hgcmObjDereference (pMsg);
    }

    LogFlow(("MAIN::hgcmMsgPostInternal: hMsg 0x%08X, rc = %Rrc\n", hMsg, rc));

    return rc;
}


/* Post message to worker thread with a flag indication if this is a Send or Post.
 *
 * @thread any
 */

int hgcmMsgPost (HGCMMSGHANDLE hMsg, PHGCMMSGCALLBACK pfnCallback)
{
    int rc = hgcmMsgPostInternal (hMsg, pfnCallback, false);

    if (RT_SUCCESS(rc))
    {
        rc = VINF_HGCM_ASYNC_EXECUTE;
    }

    return rc;
}

/* Send message to worker thread. Sending thread will block until message is processed.
 *
 * @thread any
 */

int hgcmMsgSend (HGCMMSGHANDLE hMsg)
{
    return hgcmMsgPostInternal (hMsg, NULL, true);
}


int hgcmMsgGet (HGCMTHREADHANDLE hThread, HGCMMsgCore **ppMsg)
{
    LogFlow(("MAIN::hgcmMsgGet: hThread = 0x%08X, ppMsg = %p\n", hThread, ppMsg));

    if (!hThread || !ppMsg)
    {
        return VERR_INVALID_PARAMETER;
    }

    int rc = VINF_SUCCESS;

    HGCMThread *pThread = (HGCMThread *)hgcmObjReference (hThread, HGCMOBJ_THREAD);

    if (!pThread)
    {
        rc = VERR_INVALID_HANDLE;
    }
    else
    {
        rc = pThread->MsgGet (ppMsg);

        hgcmObjDereference (pThread);
    }

    LogFlow(("MAIN::hgcmMsgGet: *ppMsg = %p, rc = %Rrc\n", *ppMsg, rc));

    return rc;
}


void hgcmMsgComplete (HGCMMsgCore *pMsg, int32_t u32Result)
{
    LogFlow(("MAIN::hgcmMsgComplete: pMsg = %p\n", pMsg));

    if (!pMsg)
    {
        return;
    }

    pMsg->Thread()->MsgComplete (pMsg, u32Result);

    LogFlow(("MAIN::hgcmMsgComplete: pMsg = %p, rc = void\n", pMsg));

    return;
}


int hgcmThreadInit (void)
{
    int rc = VINF_SUCCESS;

    LogFlow(("MAIN::hgcmThreadInit\n"));

    /** @todo error processing. */

    rc = hgcmObjInit ();

    LogFlow(("MAIN::hgcmThreadInit: rc = %Rrc\n", rc));

    return rc;
}

void hgcmThreadUninit (void)
{
    hgcmObjUninit ();
}

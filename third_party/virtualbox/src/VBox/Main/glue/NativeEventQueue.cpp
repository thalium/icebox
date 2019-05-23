/* $Id: NativeEventQueue.cpp $ */
/** @file
 * MS COM / XPCOM Abstraction Layer:
 * Main event queue class declaration
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

#include "VBox/com/NativeEventQueue.h"

#include <new> /* For bad_alloc. */

#ifdef RT_OS_DARWIN
# include <CoreFoundation/CFRunLoop.h>
#endif

#if defined(VBOX_WITH_XPCOM) && !defined(RT_OS_DARWIN) && !defined(RT_OS_OS2)
# define USE_XPCOM_QUEUE
#endif

#include <iprt/err.h>
#include <iprt/time.h>
#include <iprt/thread.h>
#include <iprt/log.h>
#ifdef USE_XPCOM_QUEUE
# include <errno.h>
#endif

namespace com
{

// NativeEventQueue class
////////////////////////////////////////////////////////////////////////////////

#ifndef VBOX_WITH_XPCOM

# define CHECK_THREAD_RET(ret) \
    do { \
        AssertMsg(GetCurrentThreadId() == mThreadId, ("Must be on event queue thread!")); \
        if (GetCurrentThreadId() != mThreadId) \
            return ret; \
    } while (0)

/** Magic LPARAM value for the WM_USER messages that we're posting.
 * @remarks This magic value is duplicated in
 *          vboxapi/PlatformMSCOM::interruptWaitEvents(). */
#define EVENTQUEUE_WIN_LPARAM_MAGIC   UINT32_C(0xf241b819)


#else // VBOX_WITH_XPCOM

# define CHECK_THREAD_RET(ret) \
    do { \
        if (!mEventQ) \
            return ret; \
        BOOL isOnCurrentThread = FALSE; \
        mEventQ->IsOnCurrentThread(&isOnCurrentThread); \
        AssertMsg(isOnCurrentThread, ("Must be on event queue thread!")); \
        if (!isOnCurrentThread) \
            return ret; \
    } while (0)

#endif // VBOX_WITH_XPCOM

/** Pointer to the main event queue. */
NativeEventQueue *NativeEventQueue::sMainQueue = NULL;


#ifdef VBOX_WITH_XPCOM

struct MyPLEvent : public PLEvent
{
    MyPLEvent(NativeEvent *e) : event(e) {}
    NativeEvent *event;
};

/* static */
void *PR_CALLBACK com::NativeEventQueue::plEventHandler(PLEvent *self)
{
    NativeEvent *ev = ((MyPLEvent *)self)->event;
    if (ev)
        ev->handler();
    else
    {
        NativeEventQueue *eq = (NativeEventQueue *)self->owner;
        Assert(eq);
        eq->mInterrupted = true;
    }
    return NULL;
}

/* static */
void PR_CALLBACK com::NativeEventQueue::plEventDestructor(PLEvent *self)
{
    NativeEvent *ev = ((MyPLEvent *)self)->event;
    if (ev)
        delete ev;
    delete self;
}

#endif // VBOX_WITH_XPCOM

/**
 *  Constructs an event queue for the current thread.
 *
 *  Currently, there can be only one event queue per thread, so if an event
 *  queue for the current thread already exists, this object is simply attached
 *  to the existing event queue.
 */
NativeEventQueue::NativeEventQueue()
{
#ifndef VBOX_WITH_XPCOM

    mThreadId = GetCurrentThreadId();
    // force the system to create the message queue for the current thread
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    if (!DuplicateHandle(GetCurrentProcess(),
                         GetCurrentThread(),
                         GetCurrentProcess(),
                         &mhThread,
                         0 /*dwDesiredAccess*/,
                         FALSE /*bInheritHandle*/,
                         DUPLICATE_SAME_ACCESS))
      mhThread = INVALID_HANDLE_VALUE;

#else // VBOX_WITH_XPCOM

    mEQCreated = false;
    mInterrupted = false;

    // Here we reference the global nsIEventQueueService instance and hold it
    // until we're destroyed. This is necessary to keep NS_ShutdownXPCOM() away
    // from calling StopAcceptingEvents() on all event queues upon destruction of
    // nsIEventQueueService, and makes sense when, for some reason, this happens
    // *before* we're able to send a NULL event to stop our event handler thread
    // when doing unexpected cleanup caused indirectly by NS_ShutdownXPCOM()
    // that is performing a global cleanup of everything. A good example of such
    // situation is when NS_ShutdownXPCOM() is called while the VirtualBox component
    // is still alive (because it is still referenced): eventually, it results in
    // a VirtualBox::uninit() call from where it is already not possible to post
    // NULL to the event thread (because it stopped accepting events).

    nsresult rc = NS_GetEventQueueService(getter_AddRefs(mEventQService));

    if (NS_SUCCEEDED(rc))
    {
        rc = mEventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                                                 getter_AddRefs(mEventQ));
        if (rc == NS_ERROR_NOT_AVAILABLE)
        {
            rc = mEventQService->CreateThreadEventQueue();
            if (NS_SUCCEEDED(rc))
            {
                mEQCreated = true;
                rc = mEventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                                                         getter_AddRefs(mEventQ));
            }
        }
    }
    AssertComRC(rc);

#endif // VBOX_WITH_XPCOM
}

NativeEventQueue::~NativeEventQueue()
{
#ifndef VBOX_WITH_XPCOM
    if (mhThread != INVALID_HANDLE_VALUE)
    {
        CloseHandle(mhThread);
        mhThread = INVALID_HANDLE_VALUE;
    }
#else // VBOX_WITH_XPCOM
    // process all pending events before destruction
    if (mEventQ)
    {
        if (mEQCreated)
        {
            mEventQ->StopAcceptingEvents();
            mEventQ->ProcessPendingEvents();
            mEventQService->DestroyThreadEventQueue();
        }
        mEventQ = nsnull;
        mEventQService = nsnull;
    }
#endif // VBOX_WITH_XPCOM
}

/**
 *  Initializes the main event queue instance.
 *  @returns VBox status code.
 *
 *  @remarks If you're using the rest of the COM/XPCOM glue library,
 *           com::Initialize() will take care of initializing and uninitializing
 *           the NativeEventQueue class.  If you don't call com::Initialize, you must
 *           make sure to call this method on the same thread that did the
 *           XPCOM initialization or we'll end up using the wrong main queue.
 */
/* static */
int NativeEventQueue::init()
{
    Assert(sMainQueue == NULL);
    Assert(RTThreadIsMain(RTThreadSelf()));

    try
    {
        sMainQueue = new NativeEventQueue();
        AssertPtr(sMainQueue);
#ifdef VBOX_WITH_XPCOM
        /* Check that it actually is the main event queue, i.e. that
           we're called on the right thread. */
        nsCOMPtr<nsIEventQueue> q;
        nsresult rv = NS_GetMainEventQ(getter_AddRefs(q));
        AssertComRCReturn(rv, VERR_INVALID_POINTER);
        Assert(q == sMainQueue->mEventQ);

        /* Check that it's a native queue. */
        PRBool fIsNative = PR_FALSE;
        rv = sMainQueue->mEventQ->IsQueueNative(&fIsNative);
        Assert(NS_SUCCEEDED(rv) && fIsNative);
#endif // VBOX_WITH_XPCOM
    }
    catch (std::bad_alloc &ba)
    {
        NOREF(ba);
        return VERR_NO_MEMORY;
    }

    return VINF_SUCCESS;
}

/**
 *  Uninitialize the global resources (i.e. the main event queue instance).
 *  @returns VINF_SUCCESS
 */
/* static */
int NativeEventQueue::uninit()
{
    if (sMainQueue)
    {
        /* Must process all events to make sure that no NULL event is left
         * after this point. It would need to modify the state of sMainQueue. */
#ifdef RT_OS_DARWIN /* Do not process the native runloop, the toolkit may not be ready for it. */
        sMainQueue->mEventQ->ProcessPendingEvents();
#else
        sMainQueue->processEventQueue(0);
#endif
        delete sMainQueue;
        sMainQueue = NULL;
    }
    return VINF_SUCCESS;
}

/**
 *  Get main event queue instance.
 *
 *  Depends on init() being called first.
 */
/* static */
NativeEventQueue* NativeEventQueue::getMainEventQueue()
{
    return sMainQueue;
}

#ifdef VBOX_WITH_XPCOM
# ifdef RT_OS_DARWIN
/**
 * Wait for events and process them (Darwin).
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_TIMEOUT
 * @retval  VERR_INTERRUPTED
 *
 * @param   cMsTimeout      How long to wait, or RT_INDEFINITE_WAIT.
 */
static int waitForEventsOnDarwin(RTMSINTERVAL cMsTimeout)
{
    /*
     * Wait for the requested time, if we get a hit we do a poll to process
     * any other pending messages.
     *
     * Note! About 1.0e10: According to the sources anything above 3.1556952e+9
     *       means indefinite wait and 1.0e10 is what CFRunLoopRun() uses.
     */
    CFTimeInterval rdTimeout = cMsTimeout == RT_INDEFINITE_WAIT ? 1e10 : (double)cMsTimeout / 1000;
    OSStatus orc = CFRunLoopRunInMode(kCFRunLoopDefaultMode, rdTimeout, true /*returnAfterSourceHandled*/);
    if (orc == kCFRunLoopRunHandledSource)
    {
        OSStatus orc2 = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, false /*returnAfterSourceHandled*/);
        if (   orc2 == kCFRunLoopRunStopped
            || orc2 == kCFRunLoopRunFinished)
            orc = orc2;
    }
    if (   orc == 0 /*???*/
        || orc == kCFRunLoopRunHandledSource)
        return VINF_SUCCESS;
    if (   orc == kCFRunLoopRunStopped
        || orc == kCFRunLoopRunFinished)
        return VERR_INTERRUPTED;
    AssertMsg(orc == kCFRunLoopRunTimedOut, ("Unexpected status code from CFRunLoopRunInMode: %#x", orc));
    return VERR_TIMEOUT;
}
# else // !RT_OS_DARWIN

/**
 * Wait for events (generic XPCOM).
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_TIMEOUT
 * @retval  VINF_INTERRUPTED
 * @retval  VERR_INTERNAL_ERROR_4
 *
 * @param   pQueue          The queue to wait on.
 * @param   cMsTimeout      How long to wait, or RT_INDEFINITE_WAIT.
 */
static int waitForEventsOnXPCOM(nsIEventQueue *pQueue, RTMSINTERVAL cMsTimeout)
{
    int     fd = pQueue->GetEventQueueSelectFD();
    fd_set  fdsetR;
    FD_ZERO(&fdsetR);
    FD_SET(fd, &fdsetR);

    fd_set  fdsetE = fdsetR;

    struct timeval  tv = {0,0};
    struct timeval *ptv;
    if (cMsTimeout == RT_INDEFINITE_WAIT)
        ptv = NULL;
    else
    {
        tv.tv_sec  = cMsTimeout / 1000;
        tv.tv_usec = (cMsTimeout % 1000) * 1000;
        ptv = &tv;
    }

    int rc = select(fd + 1, &fdsetR, NULL, &fdsetE, ptv);
    if (rc > 0)
        rc = VINF_SUCCESS;
    else if (rc == 0)
        rc = VERR_TIMEOUT;
    else if (errno == EINTR)
        rc = VINF_INTERRUPTED;
    else
    {
        static uint32_t s_ErrorCount = 0;
        if (s_ErrorCount < 500)
        {
            LogRel(("waitForEventsOnXPCOM rc=%d errno=%d\n", rc, errno));
            ++s_ErrorCount;
        }

        AssertMsgFailed(("rc=%d errno=%d\n", rc, errno));
        rc = VERR_INTERNAL_ERROR_4;
    }
    return rc;
}

# endif // !RT_OS_DARWIN
#endif // VBOX_WITH_XPCOM

#ifndef VBOX_WITH_XPCOM

/**
 * Dispatch a message on Windows.
 *
 * This will pick out our events and handle them specially.
 *
 * @returns @a rc or VERR_INTERRUPTED (WM_QUIT or NULL msg).
 * @param   pMsg    The message to dispatch.
 * @param   rc      The current status code.
 */
/*static*/
int NativeEventQueue::dispatchMessageOnWindows(MSG const *pMsg, int rc)
{
    /*
     * Check for and dispatch our events.
     */
    if (   pMsg->hwnd    == NULL
        && pMsg->message == WM_USER)
    {
        if (pMsg->lParam == EVENTQUEUE_WIN_LPARAM_MAGIC)
        {
            NativeEvent *pEvent = (NativeEvent *)pMsg->wParam;
            if (pEvent)
            {
                pEvent->handler();
                delete pEvent;
            }
            else
                rc = VERR_INTERRUPTED;
            return rc;
        }
        AssertMsgFailed(("lParam=%p wParam=%p\n", pMsg->lParam, pMsg->wParam));
    }

    /*
     * Check for the quit message and dispatch the message the normal way.
     */
    if (pMsg->message == WM_QUIT)
        rc = VERR_INTERRUPTED;
    TranslateMessage(pMsg);
    DispatchMessage(pMsg);

    return rc;
}


/**
 * Process pending events (Windows).
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_TIMEOUT
 * @retval  VERR_INTERRUPTED.
 */
static int processPendingEvents(void)
{
    int rc = VERR_TIMEOUT;
    MSG Msg;
    if (PeekMessage(&Msg, NULL /*hWnd*/, 0 /*wMsgFilterMin*/, 0 /*wMsgFilterMax*/, PM_REMOVE))
    {
        rc = VINF_SUCCESS;
        do
            rc = NativeEventQueue::dispatchMessageOnWindows(&Msg, rc);
        while (PeekMessage(&Msg, NULL /*hWnd*/, 0 /*wMsgFilterMin*/, 0 /*wMsgFilterMax*/, PM_REMOVE));
    }
    return rc;
}

#else // VBOX_WITH_XPCOM

/**
 * Process pending XPCOM events.
 * @param pQueue The queue to process events on.
 * @retval  VINF_SUCCESS
 * @retval  VERR_TIMEOUT
 * @retval  VERR_INTERRUPTED (darwin only)
 * @retval  VERR_INTERNAL_ERROR_2
 */
static int processPendingEvents(nsIEventQueue *pQueue)
{
    /* ProcessPendingEvents doesn't report back what it did, so check here. */
    PRBool fHasEvents = PR_FALSE;
    nsresult hr = pQueue->PendingEvents(&fHasEvents);
    if (NS_FAILED(hr))
        return VERR_INTERNAL_ERROR_2;

    /* Process pending events. */
    int rc = VINF_SUCCESS;
    if (fHasEvents)
        pQueue->ProcessPendingEvents();
    else
        rc = VERR_TIMEOUT;

# ifdef RT_OS_DARWIN
    /* Process pending native events. */
    int rc2 = waitForEventsOnDarwin(0);
    if (rc == VERR_TIMEOUT || rc2 == VERR_INTERRUPTED)
        rc = rc2;
# endif

    return rc;
}

#endif // VBOX_WITH_XPCOM

/**
 * Process events pending on this event queue, and wait up to given timeout, if
 * nothing is available.
 *
 * Must be called on same thread this event queue was created on.
 *
 * @param   cMsTimeout  The timeout specified as milliseconds.  Use
 *                      RT_INDEFINITE_WAIT to wait till an event is posted on the
 *                      queue.
 *
 * @returns VBox status code
 * @retval  VINF_SUCCESS if one or more messages was processed.
 * @retval  VERR_TIMEOUT if cMsTimeout expired.
 * @retval  VERR_INVALID_CONTEXT if called on the wrong thread.
 * @retval  VERR_INTERRUPTED if interruptEventQueueProcessing was called.
 *          On Windows will also be returned when WM_QUIT is encountered.
 *          On Darwin this may also be returned when the native queue is
 *          stopped or destroyed/finished.
 * @retval  VINF_INTERRUPTED if the native system call was interrupted by a
 *          an asynchronous event delivery (signal) or just felt like returning
 *          out of bounds.  On darwin it will also be returned if the queue is
 *          stopped.
 */
int NativeEventQueue::processEventQueue(RTMSINTERVAL cMsTimeout)
{
    int rc;
    CHECK_THREAD_RET(VERR_INVALID_CONTEXT);

#ifdef VBOX_WITH_XPCOM
    /*
     * Process pending events, if none are available and we're not in a
     * poll call, wait for some to appear.  (We have to be a little bit
     * careful after waiting for the events since Darwin will process
     * them as part of the wait, while the XPCOM case will not.)
     *
     * Note! Unfortunately, WaitForEvent isn't interruptible with Ctrl-C,
     *       while select() is.  So we cannot use it for indefinite waits.
     */
    rc = processPendingEvents(mEventQ);
    if (    rc == VERR_TIMEOUT
        &&  cMsTimeout > 0)
    {
# ifdef RT_OS_DARWIN
        /** @todo check how Ctrl-C works on Darwin. */
        rc = waitForEventsOnDarwin(cMsTimeout);
# else // !RT_OS_DARWIN
        rc = waitForEventsOnXPCOM(mEventQ, cMsTimeout);
# endif // !RT_OS_DARWIN
        if (    RT_SUCCESS(rc)
            ||  rc == VERR_TIMEOUT)
        {
            int rc2 = processPendingEvents(mEventQ);
            /* If the wait was successful don't fail the whole operation. */
            if (RT_FAILURE(rc) && RT_FAILURE(rc2))
                rc = rc2;
        }
    }

    if (  (   RT_SUCCESS(rc)
           || rc == VERR_INTERRUPTED
           || rc == VERR_TIMEOUT)
        && mInterrupted)
    {
        mInterrupted = false;
        rc = VERR_INTERRUPTED;
    }

#else // !VBOX_WITH_XPCOM
    if (cMsTimeout == RT_INDEFINITE_WAIT)
    {
        BOOL fRet = 0; /* Shut up MSC */
        MSG  Msg;
        rc = VINF_SUCCESS;
        while (   rc != VERR_INTERRUPTED
               && (fRet = GetMessage(&Msg, NULL /*hWnd*/, WM_USER, WM_USER))
               && fRet != -1)
            rc = NativeEventQueue::dispatchMessageOnWindows(&Msg, rc);
        if (fRet == 0)
            rc = VERR_INTERRUPTED;
        else if (fRet == -1)
            rc = RTErrConvertFromWin32(GetLastError());
    }
    else
    {
        rc = processPendingEvents();
        if (   rc == VERR_TIMEOUT
            && cMsTimeout != 0)
        {
            DWORD rcW = MsgWaitForMultipleObjects(1,
                                                  &mhThread,
                                                  TRUE /*fWaitAll*/,
                                                  cMsTimeout,
                                                  QS_ALLINPUT);
            AssertMsgReturn(rcW == WAIT_TIMEOUT || rcW == WAIT_OBJECT_0,
                            ("%d\n", rcW),
                            VERR_INTERNAL_ERROR_4);
            rc = processPendingEvents();
        }
    }
#endif // !VBOX_WITH_XPCOM

    Assert(rc != VERR_TIMEOUT || cMsTimeout != RT_INDEFINITE_WAIT);
    return rc;
}

/**
 * Interrupt thread waiting on event queue processing.
 *
 * Can be called on any thread.
 *
 * @returns VBox status code.
 */
int NativeEventQueue::interruptEventQueueProcessing()
{
    /* Send a NULL event. This event will be picked up and handled specially
     * both for XPCOM and Windows.  It is the responsibility of the caller to
     * take care of not running the loop again in a way which will hang. */
    postEvent(NULL);
    return VINF_SUCCESS;
}

/**
 *  Posts an event to this event loop asynchronously.
 *
 *  @param  pEvent  the event to post, must be allocated using |new|
 *  @return         @c TRUE if successful and false otherwise
 */
BOOL NativeEventQueue::postEvent(NativeEvent *pEvent)
{
#ifndef VBOX_WITH_XPCOM
    /* Note! The event == NULL case is duplicated in vboxapi/PlatformMSCOM::interruptWaitEvents(). */
    BOOL fRc = PostThreadMessage(mThreadId, WM_USER, (WPARAM)pEvent, EVENTQUEUE_WIN_LPARAM_MAGIC);
    if (!fRc)
    {
        static int s_cBitchedAboutFullNativeEventQueue = 0;
        if (   GetLastError() == ERROR_NOT_ENOUGH_QUOTA
            && s_cBitchedAboutFullNativeEventQueue < 10)
            LogRel(("Warning: Asynchronous event queue (%p, thread %RI32) full, event (%p) not delivered (%d/10)\n",
                    this, mThreadId, pEvent, ++s_cBitchedAboutFullNativeEventQueue));
        else
            AssertFailed();
    }
    return fRc;
#else // VBOX_WITH_XPCOM
    if (!mEventQ)
        return FALSE;

    try
    {
        MyPLEvent *pMyEvent = new MyPLEvent(pEvent);
        mEventQ->InitEvent(pMyEvent, this, com::NativeEventQueue::plEventHandler,
                           com::NativeEventQueue::plEventDestructor);
        HRESULT rc = mEventQ->PostEvent(pMyEvent);
        return NS_SUCCEEDED(rc);
    }
    catch (std::bad_alloc &ba)
    {
        AssertMsgFailed(("Out of memory while allocating memory for event=%p: %s\n",
                         pEvent, ba.what()));
    }

    return FALSE;
#endif // VBOX_WITH_XPCOM
}

/**
 *  Get select()'able selector for this event queue.
 *  This will return -1 on platforms and queue variants not supporting such
 *  functionality.
 */
int NativeEventQueue::getSelectFD()
{
#ifdef VBOX_WITH_XPCOM
    return mEventQ->GetEventQueueSelectFD();
#else
    return -1;
#endif
}

}
/* namespace com */

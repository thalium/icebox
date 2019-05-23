/* $Id: EventQueue.cpp $ */
/** @file
 * Event queue class declaration.
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
 */

/** @todo Adapt / update documentation! */

#include "VBox/com/EventQueue.h"

#include <iprt/asm.h>
#include <new> /* For bad_alloc. */

#include <iprt/err.h>
#include <iprt/semaphore.h>
#include <iprt/time.h>
#include <iprt/thread.h>
#include <iprt/log.h>

namespace com
{

// EventQueue class
////////////////////////////////////////////////////////////////////////////////

EventQueue::EventQueue(void)
    : mUserCnt(0),
      mShutdown(false)
{
    int rc = RTCritSectInit(&mCritSect);
    AssertRC(rc);

    rc = RTSemEventCreate(&mSemEvent);
    AssertRC(rc);
}

EventQueue::~EventQueue(void)
{
    int rc = RTCritSectDelete(&mCritSect);
    AssertRC(rc);

    rc = RTSemEventDestroy(mSemEvent);
    AssertRC(rc);

    EventQueueListIterator it  = mEvents.begin();
    while (it != mEvents.end())
    {
        (*it)->Release();
        it = mEvents.erase(it);
    }
}

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
int EventQueue::processEventQueue(RTMSINTERVAL cMsTimeout)
{
    size_t cNumEvents;
    int rc = RTCritSectEnter(&mCritSect);
    if (RT_SUCCESS(rc))
    {
        if (mUserCnt == 0) /* No concurrent access allowed. */
        {
            mUserCnt++;

            cNumEvents = mEvents.size();
            if (!cNumEvents)
            {
                int rc2 = RTCritSectLeave(&mCritSect);
                AssertRC(rc2);

                rc = RTSemEventWaitNoResume(mSemEvent, cMsTimeout);

                rc2 = RTCritSectEnter(&mCritSect);
                AssertRC(rc2);

                if (RT_SUCCESS(rc))
                {
                    if (mShutdown)
                        rc = VERR_INTERRUPTED;
                    cNumEvents = mEvents.size();
                }
            }

            if (RT_SUCCESS(rc))
                rc = processPendingEvents(cNumEvents);

            Assert(mUserCnt);
            mUserCnt--;
        }
        else
            rc = VERR_WRONG_ORDER;

        int rc2 = RTCritSectLeave(&mCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    Assert(rc != VERR_TIMEOUT || cMsTimeout != RT_INDEFINITE_WAIT);
    return rc;
}

/**
 * Processes all pending events in the queue at the time of
 * calling. Note: Does no initial locking, must be done by the
 * caller!
 *
 * @return  IPRT status code.
 */
int EventQueue::processPendingEvents(size_t cNumEvents)
{
    if (!cNumEvents) /* Nothing to process? Bail out early. */
        return VINF_SUCCESS;

    int rc = VINF_SUCCESS;

    EventQueueListIterator it = mEvents.begin();
    for (size_t i = 0;
            i   < cNumEvents
         && it != mEvents.end(); i++)
    {
        Event *pEvent = *it;
        AssertPtr(pEvent);

        mEvents.erase(it);

        int rc2 = RTCritSectLeave(&mCritSect);
        AssertRC(rc2);

        pEvent->handler();
        pEvent->Release();

        rc2 = RTCritSectEnter(&mCritSect);
        AssertRC(rc2);

        it = mEvents.begin();
        if (mShutdown)
        {
            rc = VERR_INTERRUPTED;
            break;
        }
    }

    return rc;
}

/**
 * Interrupt thread waiting on event queue processing.
 *
 * Can be called on any thread.
 *
 * @returns VBox status code.
 */
int EventQueue::interruptEventQueueProcessing(void)
{
    ASMAtomicWriteBool(&mShutdown, true);

    return RTSemEventSignal(mSemEvent);
}

/**
 *  Posts an event to this event loop asynchronously.
 *
 *  @param pEvent   the event to post, must be allocated using |new|
 *  @return         TRUE if successful and false otherwise
 */
BOOL EventQueue::postEvent(Event *pEvent)
{
    int rc = RTCritSectEnter(&mCritSect);
    if (RT_SUCCESS(rc))
    {
        try
        {
            if (pEvent)
            {
                pEvent->AddRef();
                mEvents.push_back(pEvent);
            }
            else /* No locking, since we're already in our crit sect. */
                mShutdown = true;

            size_t cEvents = mEvents.size();
            if (cEvents > _1K) /** @todo Make value configurable? */
            {
                static int s_cBitchedAboutLotEvents = 0;
                if (s_cBitchedAboutLotEvents < 10)
                    LogRel(("Warning: Event queue received lots of events (%zu), expect delayed event handling (%d/10)\n",
                            cEvents, ++s_cBitchedAboutLotEvents));
            }

            /* Leave critical section before signalling event. */
            rc = RTCritSectLeave(&mCritSect);
            if (RT_SUCCESS(rc))
            {
                int rc2 = RTSemEventSignal(mSemEvent);
                AssertRC(rc2);
            }
        }
        catch (std::bad_alloc &ba)
        {
            NOREF(ba);
            rc = VERR_NO_MEMORY;
        }

        if (RT_FAILURE(rc))
        {
            int rc2 = RTCritSectLeave(&mCritSect);
            AssertRC(rc2);
        }
    }

    return RT_SUCCESS(rc) ? TRUE : FALSE;
}

}
/* namespace com */

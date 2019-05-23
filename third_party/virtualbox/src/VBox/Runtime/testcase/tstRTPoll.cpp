/* $Id: tstRTPoll.cpp $ */
/** @file
 * IPRT Testcase - RTPoll.
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
#include <iprt/poll.h>

#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/pipe.h>
#include <iprt/string.h>
#include <iprt/test.h>


static void tstRTPoll2(void)
{
    RTTestISub("Negative");

    /*
     * Bad set pointer and handle values.
     */
    RTTESTI_CHECK_RC(RTPollSetCreate(NULL), VERR_INVALID_POINTER);
    RTPOLLSET hSetInvl = (RTPOLLSET)(intptr_t)-3;
    RTTESTI_CHECK_RC(RTPollSetDestroy(hSetInvl), VERR_INVALID_HANDLE);
    RTHANDLE Handle;
    Handle.enmType = RTHANDLETYPE_PIPE;
    Handle.u.hPipe = NIL_RTPIPE;
    RTTESTI_CHECK_RC(RTPollSetAdd(hSetInvl, &Handle, RTPOLL_EVT_ERROR, 1), VERR_INVALID_HANDLE);
    RTTESTI_CHECK_RC(RTPollSetRemove(hSetInvl, 1), VERR_INVALID_HANDLE);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSetInvl, 1, NULL), VERR_INVALID_HANDLE);
    RTTESTI_CHECK(RTPollSetGetCount(hSetInvl) == UINT32_MAX);
    RTTESTI_CHECK_RC(RTPoll(hSetInvl, 0, NULL, NULL),  VERR_INVALID_HANDLE);
    RTTESTI_CHECK_RC(RTPollNoResume(hSetInvl, 0, NULL, NULL),  VERR_INVALID_HANDLE);

    /*
     * Invalid arguments and other stuff.
     */
    RTPOLLSET hSet = NIL_RTPOLLSET;
    RTTESTI_CHECK_RC_RETV(RTPollSetCreate(&hSet), VINF_SUCCESS);

    RTTESTI_CHECK_RC(RTPoll(hSet, RT_INDEFINITE_WAIT, NULL, NULL), VERR_DEADLOCK);
    RTTESTI_CHECK_RC(RTPollNoResume(hSet, RT_INDEFINITE_WAIT, NULL, NULL), VERR_DEADLOCK);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, UINT32_MAX), VERR_INVALID_PARAMETER);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 1,  NULL), VERR_POLL_HANDLE_ID_NOT_FOUND);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 1), VERR_POLL_HANDLE_ID_NOT_FOUND);

    RTTESTI_CHECK_RC(RTPollSetAdd(hSet, NULL, RTPOLL_EVT_ERROR, 1), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPollSetAdd(hSet, &Handle, RTPOLL_EVT_ERROR, UINT32_MAX), VERR_INVALID_PARAMETER);
    RTTESTI_CHECK_RC(RTPollSetAdd(hSet, &Handle, UINT32_MAX, 3), VERR_INVALID_PARAMETER);
    Handle.enmType = RTHANDLETYPE_INVALID;
    RTTESTI_CHECK_RC(RTPollSetAdd(hSet, &Handle, RTPOLL_EVT_ERROR, 3), VERR_INVALID_PARAMETER);
    RTTESTI_CHECK_RC(RTPollSetAdd(hSet, NULL, RTPOLL_EVT_ERROR, UINT32_MAX), VERR_INVALID_PARAMETER);

    /* duplicate id */
    RTPIPE hPipeR;
    RTPIPE hPipeW;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0/*fFlags*/), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPollSetAddPipe(hSet, hPipeR, RTPOLL_EVT_ERROR, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPollSetAddPipe(hSet, hPipeR, RTPOLL_EVT_ERROR, 0), VERR_POLL_HANDLE_ID_EXISTS);
    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 0), VINF_SUCCESS);
    RTPipeClose(hPipeR);
    RTPipeClose(hPipeW);

    /* non-pollable handle */
    RTFILE hBitBucket;
    RTTESTI_CHECK_RC_RETV(RTFileOpenBitBucket(&hBitBucket, RTFILE_O_WRITE), VINF_SUCCESS);
    Handle.enmType = RTHANDLETYPE_FILE;
    Handle.u.hFile = hBitBucket;
    RTTESTI_CHECK_RC(RTPollSetAdd(hSet, &Handle, RTPOLL_EVT_WRITE, 10), VERR_POLL_HANDLE_NOT_POLLABLE);
    RTFileClose(hBitBucket);

    RTTESTI_CHECK_RC_RETV(RTPollSetDestroy(hSet), VINF_SUCCESS);



}


static void tstRTPoll1(void)
{
    RTTestISub("Basics");

    /* create and destroy. */
    RTPOLLSET hSet = NIL_RTPOLLSET;
    RTTESTI_CHECK_RC_RETV(RTPollSetCreate(&hSet), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(hSet != NIL_RTPOLLSET);
    RTTESTI_CHECK_RC(RTPollSetDestroy(hSet), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPollSetDestroy(NIL_RTPOLLSET), VINF_SUCCESS);

    /* empty set, adding a NIL handle. */
    hSet = NIL_RTPOLLSET;
    RTTESTI_CHECK_RC_RETV(RTPollSetCreate(&hSet), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(hSet != NIL_RTPOLLSET);

    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 0);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 0, NULL), VERR_POLL_HANDLE_ID_NOT_FOUND);

    RTTESTI_CHECK_RC(RTPollSetAddPipe(hSet, NIL_RTPIPE, RTPOLL_EVT_READ, 1 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 0);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 1 /*id*/, NULL), VERR_POLL_HANDLE_ID_NOT_FOUND);
    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 0), VERR_POLL_HANDLE_ID_NOT_FOUND);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 0);

    RTTESTI_CHECK_RC(RTPollSetDestroy(hSet), VINF_SUCCESS);

    /*
     * Set with pipes
     */
    RTPIPE hPipeR;
    RTPIPE hPipeW;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0/*fFlags*/), VINF_SUCCESS);

    hSet = NIL_RTPOLLSET;
    RTTESTI_CHECK_RC_RETV(RTPollSetCreate(&hSet), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(hSet != NIL_RTPOLLSET);

    /* add the read pipe */
    RTTESTI_CHECK_RC_RETV(RTPollSetAddPipe(hSet, hPipeR, RTPOLL_EVT_READ, 1 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 1);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 1 /*id*/, NULL), VINF_SUCCESS);
    RTHANDLE Handle;
    RTTESTI_CHECK_RC_RETV(RTPollSetQueryHandle(hSet, 1 /*id*/, &Handle), VINF_SUCCESS);
    RTTESTI_CHECK(Handle.enmType == RTHANDLETYPE_PIPE);
    RTTESTI_CHECK(Handle.u.hPipe == hPipeR);

    /* poll on the set, should time out. */
    RTTESTI_CHECK_RC(RTPoll(hSet, 0, NULL,  NULL), VERR_TIMEOUT);
    RTTESTI_CHECK_RC(RTPoll(hSet, 1, NULL,  NULL), VERR_TIMEOUT);

    /* add the write pipe with error detection only, check that poll still times out. remove it again. */
    RTTESTI_CHECK_RC(RTPollSetAddPipe(hSet, hPipeW, RTPOLL_EVT_ERROR, 11 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 2);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 11 /*id*/, NULL), VINF_SUCCESS);

    RTTESTI_CHECK_RC(RTPoll(hSet, 0, NULL,  NULL), VERR_TIMEOUT);
    RTTESTI_CHECK_RC(RTPoll(hSet, 1, NULL,  NULL), VERR_TIMEOUT);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 11), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 1);

    /* add the write pipe */
    RTTESTI_CHECK_RC(RTPollSetAddPipe(hSet, hPipeW, RTPOLL_EVT_WRITE, 10 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 2);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 10 /*id*/, NULL), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPollSetQueryHandle(hSet, 10 /*id*/, &Handle), VINF_SUCCESS);
    RTTESTI_CHECK(Handle.enmType == RTHANDLETYPE_PIPE);
    RTTESTI_CHECK(Handle.u.hPipe == hPipeW);

    RTTESTI_CHECK_RC_RETV(RTPollSetQueryHandle(hSet, 1 /*id*/, &Handle), VINF_SUCCESS);
    RTTESTI_CHECK(Handle.enmType == RTHANDLETYPE_PIPE);
    RTTESTI_CHECK(Handle.u.hPipe == hPipeR);

    /* poll on the set again, now it should indicate hPipeW is ready. */
    int rc;
    RTTESTI_CHECK_RC(RTPoll(hSet, 0,   NULL,  NULL), VINF_SUCCESS);
    RTTESTI_CHECK_RC(rc = RTPoll(hSet, 100, NULL,  NULL), VINF_SUCCESS);
    if (RT_SUCCESS(rc))
        RTTESTI_CHECK_RC(RTPoll(hSet, RT_INDEFINITE_WAIT, NULL,  NULL), VINF_SUCCESS);

    RTTESTI_CHECK_RC(rc = RTPollNoResume(hSet, 0,   NULL,  NULL), VINF_SUCCESS);
    RTTESTI_CHECK_RC(rc = RTPollNoResume(hSet, 100, NULL,  NULL), VINF_SUCCESS);
    if (RT_SUCCESS(rc))
        RTTESTI_CHECK_RC(rc = RTPollNoResume(hSet, RT_INDEFINITE_WAIT, NULL,  NULL), VINF_SUCCESS);

    uint32_t fEvents = UINT32_MAX;
    uint32_t id      = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 0, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 10);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    fEvents = UINT32_MAX;
    id      = UINT32_MAX;
    RTTESTI_CHECK_RC(rc = RTPoll(hSet, 250, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 10);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    if (RT_SUCCESS(rc))
    {
        fEvents = UINT32_MAX;
        id      = UINT32_MAX;
        RTTESTI_CHECK_RC(RTPoll(hSet, RT_INDEFINITE_WAIT, &fEvents, &id), VINF_SUCCESS);
        RTTESTI_CHECK(id == 10);
        RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);
    }

    fEvents = UINT32_MAX;
    id      = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPollNoResume(hSet, 0, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 10);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    fEvents = UINT32_MAX;
    id      = UINT32_MAX;
    RTTESTI_CHECK_RC(rc = RTPollNoResume(hSet, 100, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 10);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    if (RT_SUCCESS(rc))
    {
        fEvents = UINT32_MAX;
        id      = UINT32_MAX;
        RTTESTI_CHECK_RC(RTPollNoResume(hSet, RT_INDEFINITE_WAIT, &fEvents, &id), VINF_SUCCESS);
        RTTESTI_CHECK(id == 10);
        RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);
    }

    /* Write to the pipe. Currently ASSUMING we'll get the read ready now... Good idea? */
    RTTESTI_CHECK_RC(rc = RTPipeWriteBlocking(hPipeW, "hello", 5, NULL), VINF_SUCCESS);
    if (RT_SUCCESS(rc))
    {
        fEvents = UINT32_MAX;
        id      = UINT32_MAX;
        RTTESTI_CHECK_RC(RTPoll(hSet, 0, &fEvents, &id), VINF_SUCCESS);
        RTTESTI_CHECK(id == 1);
        RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);

        fEvents = UINT32_MAX;
        id      = UINT32_MAX;
        RTTESTI_CHECK_RC(rc = RTPoll(hSet, 256, &fEvents, &id), VINF_SUCCESS);
        RTTESTI_CHECK(id == 1);
        RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);

        if (RT_SUCCESS(rc))
        {
            fEvents = UINT32_MAX;
            id      = UINT32_MAX;
            RTTESTI_CHECK_RC(RTPoll(hSet, RT_INDEFINITE_WAIT, &fEvents, &id), VINF_SUCCESS);
            RTTESTI_CHECK(id == 1);
            RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);
        }

        fEvents = UINT32_MAX;
        id      = UINT32_MAX;
        RTTESTI_CHECK_RC(RTPollNoResume(hSet, 0, &fEvents, &id), VINF_SUCCESS);
        RTTESTI_CHECK(id == 1);
        RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);

        fEvents = UINT32_MAX;
        id      = UINT32_MAX;
        RTTESTI_CHECK_RC(rc = RTPollNoResume(hSet, 383, &fEvents, &id), VINF_SUCCESS);
        RTTESTI_CHECK(id == 1);
        RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);

        if (RT_SUCCESS(rc))
        {
            fEvents = UINT32_MAX;
            id      = UINT32_MAX;
            RTTESTI_CHECK_RC(RTPollNoResume(hSet, RT_INDEFINITE_WAIT, &fEvents, &id), VINF_SUCCESS);
            RTTESTI_CHECK(id == 1);
            RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);
        }
    }

    /* Remove the read pipe, do a quick poll check. */
    RTTESTI_CHECK_RC_RETV(RTPollSetRemove(hSet, 1), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 1);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 1 /*id*/, NULL), VERR_POLL_HANDLE_ID_NOT_FOUND);
    RTTESTI_CHECK_RC_RETV(RTPollSetQueryHandle(hSet, 10 /*id*/, &Handle), VINF_SUCCESS);
    RTTESTI_CHECK(Handle.enmType == RTHANDLETYPE_PIPE);
    RTTESTI_CHECK(Handle.u.hPipe == hPipeW);

    RTTESTI_CHECK_RC(RTPoll(hSet, 0, NULL, NULL), VINF_SUCCESS);

    /* Add it back and check that we now get the write handle when polling.
       (Is this FIFOing a good idea?) */
    RTTESTI_CHECK_RC_RETV(RTPoll(hSet, 0, NULL, NULL), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPollSetAddPipe(hSet, hPipeR, RTPOLL_EVT_READ, 1 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 2);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 1 /*id*/, NULL), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPollSetQueryHandle(hSet, 1 /*id*/, &Handle), VINF_SUCCESS);
    RTTESTI_CHECK(Handle.enmType == RTHANDLETYPE_PIPE);
    RTTESTI_CHECK(Handle.u.hPipe == hPipeR);

    RTTESTI_CHECK_RC_RETV(RTPollSetQueryHandle(hSet, 10 /*id*/, &Handle), VINF_SUCCESS);
    RTTESTI_CHECK(Handle.enmType == RTHANDLETYPE_PIPE);
    RTTESTI_CHECK(Handle.u.hPipe == hPipeW);

    fEvents = UINT32_MAX;
    id      = UINT32_MAX;
    RTTESTI_CHECK_RC(rc = RTPollNoResume(hSet, 555, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 10);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    /* Remove it again and break the pipe by closing the read end. */
    RTTESTI_CHECK_RC_RETV(RTPollSetRemove(hSet, 1), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RTPollSetGetCount(hSet) == 1);
    RTTESTI_CHECK_RC(RTPollSetQueryHandle(hSet, 1 /*id*/, NULL), VERR_POLL_HANDLE_ID_NOT_FOUND);
    RTTESTI_CHECK_RC_RETV(RTPollSetQueryHandle(hSet, 10 /*id*/, &Handle), VINF_SUCCESS);
    RTTESTI_CHECK(Handle.enmType == RTHANDLETYPE_PIPE);
    RTTESTI_CHECK(Handle.u.hPipe == hPipeW);

    RTTESTI_CHECK_RC(RTPoll(hSet, 0, NULL, NULL), VINF_SUCCESS);

    RTTESTI_CHECK_RC(RTPipeClose(hPipeR), VINF_SUCCESS);

    fEvents = UINT32_MAX;
    id      = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPollNoResume(hSet, 0, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 10);
    RTTESTI_CHECK_MSG(   fEvents == RTPOLL_EVT_ERROR \
                      || fEvents == (RTPOLL_EVT_ERROR | RTPOLL_EVT_WRITE), ("%#x\n", fEvents));

    RTTESTI_CHECK_RC(RTPollSetDestroy(hSet), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW), VINF_SUCCESS);

    /*
     * Check FIFO order when removing and adding.
     *
     * Note! FIFO order is not guaranteed when a handle has more than one entry
     * in the set.
     */
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0/*fFlags*/), VINF_SUCCESS);
    RTPIPE hPipeR2, hPipeW2;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR2, &hPipeW2, 0/*fFlags*/), VINF_SUCCESS);
    RTPIPE hPipeR3, hPipeW3;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR3, &hPipeW3, 0/*fFlags*/), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPollSetCreate(&hSet), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPollSetAddPipe(hSet, hPipeR,  RTPOLL_EVT_READ,  1 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPollSetAddPipe(hSet, hPipeW,  RTPOLL_EVT_WRITE, 2 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPollSetAddPipe(hSet, hPipeR2, RTPOLL_EVT_READ,  3 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPollSetAddPipe(hSet, hPipeW2, RTPOLL_EVT_WRITE, 4 /*id*/), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPollSetAddPipe(hSet, hPipeR3, RTPOLL_EVT_READ,  5 /*id*/), VINF_SUCCESS);

    id = UINT32_MAX; fEvents = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 5, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 2);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    RTTESTI_CHECK_RC(RTPipeWriteBlocking(hPipeW,  "hello", 5, NULL), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeWriteBlocking(hPipeW2, "hello", 5, NULL), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeWriteBlocking(hPipeW3, "hello", 5, NULL), VINF_SUCCESS);
    id = UINT32_MAX; fEvents = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 5, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 1);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 1), VINF_SUCCESS);
    id = UINT32_MAX; fEvents = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 5, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 2);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 2), VINF_SUCCESS);
    id = UINT32_MAX; fEvents = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 5, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 3);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 3), VINF_SUCCESS);
    id = UINT32_MAX; fEvents = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 5, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 4);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_WRITE);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 4), VINF_SUCCESS);
    id = UINT32_MAX; fEvents = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 5, &fEvents, &id), VINF_SUCCESS);
    RTTESTI_CHECK(id == 5);
    RTTESTI_CHECK(fEvents == RTPOLL_EVT_READ);

    RTTESTI_CHECK_RC(RTPollSetRemove(hSet, 5), VINF_SUCCESS);
    id = UINT32_MAX; fEvents = UINT32_MAX;
    RTTESTI_CHECK_RC(RTPoll(hSet, 5, &fEvents, &id), VERR_TIMEOUT);

    RTTESTI_CHECK_RC(RTPipeClose(hPipeW),   VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeR),   VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW2),  VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeR2),  VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW3),  VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeR3),  VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPollSetDestroy(hSet),  VINF_SUCCESS);

}

int main()
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTPoll", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * The tests.
     */
    tstRTPoll1();
    if (RTTestErrorCount(hTest) == 0)
    {
        bool fMayPanic = RTAssertMayPanic();
        bool fQuiet    = RTAssertAreQuiet();
        RTAssertSetMayPanic(false);
        RTAssertSetQuiet(true);
        tstRTPoll2();
        RTAssertSetQuiet(fQuiet);
        RTAssertSetMayPanic(fMayPanic);
    }

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}


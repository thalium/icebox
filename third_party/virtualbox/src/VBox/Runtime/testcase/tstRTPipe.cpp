/* $Id: tstRTPipe.cpp $ */
/** @file
 * IPRT Testcase - RTPipe.
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
#include <iprt/pipe.h>

#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/param.h>
#include <iprt/process.h>
#include <iprt/string.h>
#include <iprt/test.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const char g_szTest4Message[] = "This is test #4, everything is working fine.\n\r";
static const char g_szTest5Message[] = "This is test #5, everything is working fine.\n\r";


static RTEXITCODE tstRTPipe5Child(const char *pszPipe)
{
    int rc = RTR3InitExeNoArguments(0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    int64_t iNative;
    rc = RTStrToInt64Full(pszPipe, 10, &iNative);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTStrToUInt64Full(%s) -> %Rrc\n", pszPipe, rc);

    RTPIPE hPipe;
    rc = RTPipeFromNative(&hPipe, (RTHCINTPTR)iNative, RTPIPE_N_READ);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPipeFromNative(,%s,READ) -> %Rrc\n", pszPipe, rc);

    ///
    char    szTmp[1024];
    size_t  cbRead = 0;
    rc = RTPipeReadBlocking(hPipe, szTmp, sizeof(szTmp) - 1, &cbRead);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPipeReadBlocking() -> %Rrc\n", rc);
    szTmp[cbRead] = '\0';

    size_t cbRead2;
    char szTmp2[4];
    rc = RTPipeReadBlocking(hPipe, szTmp2, sizeof(szTmp2), &cbRead2);
    if (rc != VERR_BROKEN_PIPE)
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPipeReadBlocking() -> %Rrc instead of VERR_BROKEN_PIPE\n", rc);

    rc = RTPipeClose(hPipe);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPipeClose() -> %Rrc\n", rc);

    if (memcmp(szTmp, g_szTest5Message, sizeof(g_szTest5Message)))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "Message mismatch.\n:Expected '%s'\nGot     '%s'\n", g_szTest5Message, szTmp);

    return RTEXITCODE_SUCCESS;
}

static void tstRTPipe5(void)
{
    RTTestISub("Inherit non-standard pipe handle, read end");

    char    szPathSelf[RTPATH_MAX];
    RTTESTI_CHECK_RETV(RTProcGetExecutablePath(szPathSelf, sizeof(szPathSelf)) == szPathSelf);

    RTPIPE  hPipeR;
    RTPIPE  hPipeW;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, RTPIPE_C_INHERIT_READ), VINF_SUCCESS);

    RTHCINTPTR hNative = RTPipeToNative(hPipeR);
    RTTESTI_CHECK_RETV(hNative != -1);

    char szNative[64];
    RTStrPrintf(szNative, sizeof(szNative), "%RHi", hNative);
    const char *papszArgs[4] = { szPathSelf, "--child-5", szNative, NULL };

    RTPROCESS hChild;
    RTTESTI_CHECK_RC_RETV(RTProcCreate(szPathSelf, papszArgs, RTENV_DEFAULT, 0 /*fFlags*/, &hChild), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeR), VINF_SUCCESS);

    RTTESTI_CHECK_RC(RTPipeWriteBlocking(hPipeW, g_szTest5Message, sizeof(g_szTest5Message) - 1, NULL), VINF_SUCCESS);
    int rc;
    RTTESTI_CHECK_RC(rc = RTPipeClose(hPipeW), VINF_SUCCESS);
    if (RT_FAILURE(rc))
        RTTESTI_CHECK_RC(RTProcTerminate(hChild), VINF_SUCCESS);

    RTPROCSTATUS ProcStatus;
    RTTESTI_CHECK_RC(rc = RTProcWait(hChild, RTPROCWAIT_FLAGS_BLOCK, &ProcStatus), VINF_SUCCESS);
    if (RT_FAILURE(rc))
        return;
    RTTESTI_CHECK(   ProcStatus.enmReason == RTPROCEXITREASON_NORMAL
                  && ProcStatus.iStatus   == 0);
}


static RTEXITCODE tstRTPipe4Child(const char *pszPipe)
{
    int rc = RTR3InitExeNoArguments(0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    int64_t iNative;
    rc = RTStrToInt64Full(pszPipe, 10, &iNative);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTStrToUInt64Full(%s) -> %Rrc\n", pszPipe, rc);

    RTPIPE hPipe;
    rc = RTPipeFromNative(&hPipe, (RTHCINTPTR)iNative, RTPIPE_N_WRITE);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPipeFromNative(,%s,WRITE) -> %Rrc\n", pszPipe, rc);

    rc = RTPipeWriteBlocking(hPipe, g_szTest4Message, sizeof(g_szTest4Message) - 1, NULL);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPipeWriteBlocking() -> %Rrc\n", rc);

    rc = RTPipeClose(hPipe);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPipeClose() -> %Rrc\n", rc);
    return RTEXITCODE_SUCCESS;
}

static void tstRTPipe4(void)
{
    RTTestISub("Inherit non-standard pipe handle, write end");

    char    szPathSelf[RTPATH_MAX];
    RTTESTI_CHECK_RETV(RTProcGetExecutablePath(szPathSelf, sizeof(szPathSelf)) == szPathSelf);

    RTPIPE  hPipeR;
    RTPIPE  hPipeW;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, RTPIPE_C_INHERIT_WRITE), VINF_SUCCESS);

    RTHCINTPTR hNative = RTPipeToNative(hPipeW);
    RTTESTI_CHECK_RETV(hNative != -1);

    char szNative[64];
    RTStrPrintf(szNative, sizeof(szNative), "%RHi", hNative);
    const char *papszArgs[4] = { szPathSelf, "--child-4", szNative, NULL };

    RTPROCESS hChild;
    RTTESTI_CHECK_RC_RETV(RTProcCreate(szPathSelf, papszArgs, RTENV_DEFAULT, 0 /*fFlags*/, &hChild), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);

    char    szTmp[1024];
    size_t  cbRead = 0;
    int     rc;
    RTTESTI_CHECK_RC(rc = RTPipeReadBlocking(hPipeR, szTmp, sizeof(szTmp) - 1, &cbRead), VINF_SUCCESS);
    if (RT_FAILURE(rc))
        cbRead = 0;
    RTTESTI_CHECK_RETV(cbRead < sizeof(szTmp));
    szTmp[cbRead] = '\0';

    size_t cbRead2;
    char szTmp2[4];
    RTTESTI_CHECK_RC(RTPipeReadBlocking(hPipeR, szTmp2, sizeof(szTmp2), &cbRead2), VERR_BROKEN_PIPE);
    RTTESTI_CHECK_RC(rc = RTPipeClose(hPipeR), VINF_SUCCESS);
    if (RT_FAILURE(rc))
        RTTESTI_CHECK_RC(RTProcTerminate(hChild), VINF_SUCCESS);

    RTPROCSTATUS ProcStatus;
    RTTESTI_CHECK_RC(rc = RTProcWait(hChild, RTPROCWAIT_FLAGS_BLOCK, &ProcStatus), VINF_SUCCESS);
    if (RT_FAILURE(rc))
        return;
    RTTESTI_CHECK(   ProcStatus.enmReason == RTPROCEXITREASON_NORMAL
                  && ProcStatus.iStatus   == 0);
    if (memcmp(szTmp, g_szTest4Message, sizeof(g_szTest4Message)))
        RTTestIFailed("Message mismatch.\n:Expected '%s'\nGot     '%s'\n", g_szTest4Message, szTmp);
}


static void tstRTPipe3(void)
{
    RTTestISub("Full write buffer");

    RTPIPE  hPipeR = (RTPIPE)999999;
    RTPIPE  hPipeW = (RTPIPE)999999;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);

    static char s_abBuf[_256K];
    int         rc      = VINF_SUCCESS;
    size_t      cbTotal = 0;
    memset(s_abBuf, 0xff, sizeof(s_abBuf));
    for (;;)
    {
        RTTESTI_CHECK(cbTotal < _1G);
        if (cbTotal > _1G)
            break;

        size_t cbWritten = _1G;
        rc = RTPipeWrite(hPipeW, s_abBuf, sizeof(s_abBuf), &cbWritten);
        RTTESTI_CHECK_MSG(rc == VINF_SUCCESS || rc == VINF_TRY_AGAIN, ("rc=%Rrc\n", rc));
        if (rc != VINF_SUCCESS)
            break;
        cbTotal += cbWritten;
    }

    if (rc == VINF_TRY_AGAIN)
    {
        RTTestIPrintf(RTTESTLVL_ALWAYS, "cbTotal=%zu (%#zx)\n", cbTotal, cbTotal);
        RTTESTI_CHECK_RC(RTPipeSelectOne(hPipeW, 0), VERR_TIMEOUT);
        RTTESTI_CHECK_RC(RTPipeSelectOne(hPipeW, 1), VERR_TIMEOUT);
        size_t cbRead;
        RTTESTI_CHECK_RC(RTPipeRead(hPipeR, s_abBuf, RT_MIN(sizeof(s_abBuf), cbTotal) / 2, &cbRead), VINF_SUCCESS);
        RTTESTI_CHECK_RC(RTPipeSelectOne(hPipeW, 0), VINF_SUCCESS);
        RTTESTI_CHECK_RC(RTPipeSelectOne(hPipeW, 1), VINF_SUCCESS);

        size_t cbWritten = _1G;
        rc = RTPipeWrite(hPipeW, s_abBuf, sizeof(s_abBuf), &cbWritten);
        RTTESTI_CHECK(rc == VINF_SUCCESS);
    }

    RTTESTI_CHECK_RC(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW), VINF_SUCCESS);
}

static void tstRTPipe2(void)
{
    RTTestISub("Negative");

    RTPIPE  hPipeR = (RTPIPE)1;
    RTPIPE  hPipeW = (RTPIPE)1;
    RTTESTI_CHECK_RC(RTPipeCreate(&hPipeR, &hPipeW, UINT32_MAX), VERR_INVALID_PARAMETER);
    RTTESTI_CHECK_RC(RTPipeCreate(NULL, &hPipeW, 0), VERR_INVALID_POINTER);
    RTTESTI_CHECK_RC(RTPipeCreate(&hPipeR, NULL, 0), VERR_INVALID_POINTER);
    RTTESTI_CHECK(hPipeR == (RTPIPE)1);
    RTTESTI_CHECK(hPipeW == (RTPIPE)1);


    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);

    char abBuf[_4K];
    size_t cbRead = ~(size_t)3;
    RTTESTI_CHECK_RC(RTPipeRead(hPipeW, abBuf, 0, &cbRead), VERR_ACCESS_DENIED);
    RTTESTI_CHECK_RC(RTPipeRead(hPipeW, abBuf, 1, &cbRead), VERR_ACCESS_DENIED);
    RTTESTI_CHECK(cbRead == ~(size_t)3);
    RTTESTI_CHECK_RC(RTPipeReadBlocking(hPipeW, abBuf, 0, NULL), VERR_ACCESS_DENIED);
    RTTESTI_CHECK_RC(RTPipeReadBlocking(hPipeW, abBuf, 1, NULL), VERR_ACCESS_DENIED);

    size_t cbWrite = ~(size_t)5;
    RTTESTI_CHECK_RC(RTPipeWrite(hPipeR, "asdf", 0, &cbWrite), VERR_ACCESS_DENIED);
    RTTESTI_CHECK_RC(RTPipeWrite(hPipeR, "asdf", 4, &cbWrite), VERR_ACCESS_DENIED);
    RTTESTI_CHECK(cbWrite == ~(size_t)5);
    RTTESTI_CHECK_RC(RTPipeWriteBlocking(hPipeR, "asdf", 0, NULL), VERR_ACCESS_DENIED);
    RTTESTI_CHECK_RC(RTPipeWriteBlocking(hPipeR, "asdf", 4, NULL), VERR_ACCESS_DENIED);

    RTTESTI_CHECK_RC(RTPipeFlush(hPipeR), VERR_ACCESS_DENIED);

    RTTESTI_CHECK_RC(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW), VINF_SUCCESS);
}


static void tstRTPipe1(void)
{
    RTTestISub("Basics");

    RTPIPE  hPipeR = NIL_RTPIPE;
    RTPIPE  hPipeW = NIL_RTPIPE;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(hPipeR != NIL_RTPIPE);
    RTTESTI_CHECK_RETV(hPipeW != NIL_RTPIPE);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(NIL_RTPIPE), VINF_SUCCESS);

    hPipeR = NIL_RTPIPE;
    hPipeW = NIL_RTPIPE;
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, RTPIPE_C_INHERIT_READ | RTPIPE_C_INHERIT_WRITE), VINF_SUCCESS);
    int rc = RTPipeFlush(hPipeW);
    RTTESTI_CHECK_MSG(rc == VERR_NOT_SUPPORTED || rc == VINF_SUCCESS, ("%Rrc\n", rc));
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, RTPIPE_C_INHERIT_READ), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeSelectOne(hPipeR, 0), VERR_TIMEOUT);
    RTTESTI_CHECK_RC_RETV(RTPipeSelectOne(hPipeR, 1), VERR_TIMEOUT);
    RTTESTI_CHECK_RC_RETV(RTPipeSelectOne(hPipeW, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeSelectOne(hPipeW, 1), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, RTPIPE_C_INHERIT_WRITE), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, RTPIPE_C_INHERIT_READ), VINF_SUCCESS);

    size_t  cbRead = ~(size_t)0;
    char    abBuf[_64K + _4K];
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, abBuf, sizeof(abBuf), &cbRead), VINF_TRY_AGAIN);
    RTTESTI_CHECK_RETV(cbRead == 0);

    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, abBuf, 1, &cbRead), VINF_TRY_AGAIN);
    RTTESTI_CHECK_RETV(cbRead == 0);

    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, abBuf, 0, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbRead == 0);

    size_t cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC_RETV(RTPipeWrite(hPipeW, abBuf, 0, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbWritten == 0);

    /* We can write a number of bytes without blocking (see PIPE_BUF on
       POSIX systems). */
    cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC_RETV(RTPipeWrite(hPipeW, "42", 2, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK_MSG_RETV(cbWritten == 2, ("cbWritten=%zu\n", cbWritten));
    cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC_RETV(RTPipeWrite(hPipeW, "!", 1, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbWritten == 1);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, abBuf, 3, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbRead == 3);
    RTTESTI_CHECK_RETV(!memcmp(abBuf, "42!", 3));

    cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC_RETV(RTPipeWrite(hPipeW, "BigQ", 4, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbWritten == 4);
    RTTESTI_CHECK_RC_RETV(RTPipeSelectOne(hPipeR, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeSelectOne(hPipeR, 1), VINF_SUCCESS);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeQueryReadable(hPipeR, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(cbRead == cbWritten, ("cbRead=%zu cbWritten=%zu\n", cbRead, cbWritten));
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, abBuf, sizeof(abBuf), &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbRead == 4);
    RTTESTI_CHECK_RETV(!memcmp(abBuf, "BigQ", 4));

    cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC_RETV(RTPipeWrite(hPipeW, "H2G2", 4, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbWritten == 4);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeQueryReadable(hPipeR, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(cbRead == cbWritten, ("cbRead=%zu cbWritten=%zu\n", cbRead, cbWritten));
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, &abBuf[0], 1, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbRead == 1);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeQueryReadable(hPipeR, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(cbRead == cbWritten - 1, ("cbRead=%zu cbWritten=%zu\n", cbRead, cbWritten));
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, &abBuf[1], 1, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbRead == 1);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeQueryReadable(hPipeR, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(cbRead == cbWritten - 2, ("cbRead=%zu cbWritten=%zu\n", cbRead, cbWritten));
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, &abBuf[2], 1, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbRead == 1);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeQueryReadable(hPipeR, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(cbRead == cbWritten - 3, ("cbRead=%zu cbWritten=%zu\n", cbRead, cbWritten));
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, &abBuf[3], 1, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbRead == 1);
    RTTESTI_CHECK_RETV(!memcmp(abBuf, "H2G2", 4));
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeQueryReadable(hPipeR, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(cbRead == cbWritten - 4, ("cbRead=%zu cbWritten=%zu\n", cbRead, cbWritten));

    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);


    RTTestISub("VERR_BROKEN_PIPE");
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeR), VINF_SUCCESS);
    cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC(RTPipeWrite(hPipeW, "", 0, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK(cbWritten == 0);
    cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC(RTPipeWrite(hPipeW, "4", 1, &cbWritten), VERR_BROKEN_PIPE);
    RTTESTI_CHECK(cbWritten == ~(size_t)2);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC(RTPipeRead(hPipeR, abBuf, 0, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(cbRead == 0);
    cbRead = ~(size_t)3;
    RTTESTI_CHECK_RC(RTPipeRead(hPipeR, abBuf, sizeof(abBuf), &cbRead), VERR_BROKEN_PIPE);
    RTTESTI_CHECK(cbRead == ~(size_t)3);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeR), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);
    cbWritten = ~(size_t)2;
    RTTESTI_CHECK_RC(RTPipeWrite(hPipeW, "42", 2, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK(cbWritten == 2);
    RTTESTI_CHECK_RC_RETV(RTPipeClose(hPipeW), VINF_SUCCESS);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, abBuf, 0, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(cbRead == 0);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC(RTPipeRead(hPipeR, &abBuf[0], 1, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(cbRead == 1);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC(RTPipeRead(hPipeR, &abBuf[1], 1, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(cbRead == 1);
    RTTESTI_CHECK(!memcmp(abBuf, "42", 2));
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC(RTPipeRead(hPipeR, abBuf, 0, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(cbRead == 0);
    cbRead = ~(size_t)3;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, abBuf, sizeof(abBuf), &cbRead), VERR_BROKEN_PIPE);
    RTTESTI_CHECK(cbRead == ~(size_t)3);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeR), VINF_SUCCESS);

    RTTestISub("Blocking");
    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeWrite(hPipeW, "42!", 3, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK(cbWritten == 3);
    RTTESTI_CHECK_RC_RETV(RTPipeReadBlocking(hPipeR, abBuf, 3, NULL), VINF_SUCCESS);
    RTTESTI_CHECK(!memcmp(abBuf, "42!", 3));
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeReadBlocking(hPipeR, &abBuf[0], 0, NULL), VINF_SUCCESS);
    cbRead = ~(size_t)42;
    RTTESTI_CHECK_RC_RETV(RTPipeReadBlocking(hPipeR, &abBuf[0], 0, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(cbRead == 0);
    RTTESTI_CHECK_RC_RETV(RTPipeReadBlocking(hPipeR, &abBuf[0], 1, NULL), VERR_BROKEN_PIPE);
    cbRead = ~(size_t)42;
    RTTESTI_CHECK_RC_RETV(RTPipeReadBlocking(hPipeR, &abBuf[0], 1, &cbRead), VERR_BROKEN_PIPE);
    RTTESTI_CHECK(cbRead == 0);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeR), VINF_SUCCESS);

    RTTESTI_CHECK_RC_RETV(RTPipeCreate(&hPipeR, &hPipeW, 0), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeWriteBlocking(hPipeW, "42!", 3, NULL), VINF_SUCCESS);
    RTTESTI_CHECK(cbWritten == 3);
    cbRead = ~(size_t)0;
    RTTESTI_CHECK_RC_RETV(RTPipeRead(hPipeR, &abBuf[0], 1, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(cbRead == 1);
    RTTESTI_CHECK_RC_RETV(RTPipeReadBlocking(hPipeR, &abBuf[1], 1, NULL), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeReadBlocking(hPipeR, &abBuf[2], 1, NULL), VINF_SUCCESS);
    RTTESTI_CHECK(!memcmp(abBuf, "42!", 3));
    RTTESTI_CHECK_RC(RTPipeClose(hPipeR), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPipeWriteBlocking(hPipeW, "", 0, NULL), VINF_SUCCESS);
    cbWritten = ~(size_t)9;
    RTTESTI_CHECK_RC_RETV(RTPipeWriteBlocking(hPipeW, "", 0, &cbWritten), VINF_SUCCESS);
    RTTESTI_CHECK(cbWritten == 0);
    RTTESTI_CHECK_RC_RETV(RTPipeWriteBlocking(hPipeW, "42", 2, NULL), VERR_BROKEN_PIPE);
    cbWritten = ~(size_t)9;
    RTTESTI_CHECK_RC_RETV(RTPipeWriteBlocking(hPipeW, "42", 2, &cbWritten), VERR_BROKEN_PIPE);
    RTTESTI_CHECK(cbWritten == 0);
    RTTESTI_CHECK_RC(RTPipeClose(hPipeW), VINF_SUCCESS);
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "--child-4"))
        return tstRTPipe4Child(argv[2]);
    if (argc == 3 && !strcmp(argv[1], "--child-5"))
        return tstRTPipe5Child(argv[2]);

    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTPipe", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * The tests.
     */
    tstRTPipe1();
    if (RTTestErrorCount(hTest) == 0)
    {
        bool fMayPanic = RTAssertMayPanic();
        bool fQuiet    = RTAssertAreQuiet();
        RTAssertSetMayPanic(false);
        RTAssertSetQuiet(true);
        tstRTPipe2();
        RTAssertSetQuiet(fQuiet);
        RTAssertSetMayPanic(fMayPanic);

        tstRTPipe3();
        tstRTPipe4();
        tstRTPipe5();
    }

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}


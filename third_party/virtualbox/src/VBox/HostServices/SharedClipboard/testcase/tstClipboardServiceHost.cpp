/* $Id: tstClipboardServiceHost.cpp $ */
/** @file
 * Shared Clipboard host service test case.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "../VBoxClipboard.h"

#include <VBox/HostServices/VBoxClipboardSvc.h>

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/test.h>

extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable);

static VBOXCLIPBOARDCLIENTDATA g_Client;
static VBOXHGCMSVCHELPERS g_Helpers = { NULL };

/** Simple call handle structure for the guest call completion callback */
struct VBOXHGCMCALLHANDLE_TYPEDEF
{
    /** Where to store the result code */
    int32_t rc;
};

/** Call completion callback for guest calls. */
static DECLCALLBACK(void) callComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc)
{
    callHandle->rc = rc;
}

static int setupTable(VBOXHGCMSVCFNTABLE *pTable)
{
    pTable->cbSize = sizeof(*pTable);
    pTable->u32Version = VBOX_HGCM_SVC_VERSION;
    g_Helpers.pfnCallComplete = callComplete;
    pTable->pHelpers = &g_Helpers;
    return VBoxHGCMSvcLoad(pTable);
}

static void testSetMode(void)
{
    struct VBOXHGCMSVCPARM parms[2];
    VBOXHGCMSVCFNTABLE table;
    uint32_t u32Mode;
    int rc;

    RTTestISub("Testing HOST_FN_SET_MODE");
    rc = setupTable(&table);
    RTTESTI_CHECK_MSG_RETV(RT_SUCCESS(rc), ("rc=%Rrc\n", rc));
    /* Reset global variable which doesn't reset itself. */
    parms[0].setUInt32(VBOX_SHARED_CLIPBOARD_MODE_OFF);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE,
                           1, parms);
    RTTESTI_CHECK_RC_OK(rc);
    u32Mode = TestClipSvcGetMode();
    RTTESTI_CHECK_MSG(u32Mode == VBOX_SHARED_CLIPBOARD_MODE_OFF,
                      ("u32Mode=%u\n", (unsigned) u32Mode));
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE,
                           0, parms);
    RTTESTI_CHECK_RC(rc, VERR_INVALID_PARAMETER);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE,
                           2, parms);
    RTTESTI_CHECK_RC(rc, VERR_INVALID_PARAMETER);
    parms[0].setUInt64(99);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE,
                           1, parms);
    RTTESTI_CHECK_RC(rc, VERR_INVALID_PARAMETER);
    parms[0].setUInt32(VBOX_SHARED_CLIPBOARD_MODE_HOST_TO_GUEST);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE,
                           1, parms);
    RTTESTI_CHECK_RC_OK(rc);
    u32Mode = TestClipSvcGetMode();
    RTTESTI_CHECK_MSG(u32Mode == VBOX_SHARED_CLIPBOARD_MODE_HOST_TO_GUEST,
                      ("u32Mode=%u\n", (unsigned) u32Mode));
    parms[0].setUInt32(99);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE,
                           1, parms);
    RTTESTI_CHECK_RC_OK(rc);
    u32Mode = TestClipSvcGetMode();
    RTTESTI_CHECK_MSG(u32Mode == VBOX_SHARED_CLIPBOARD_MODE_OFF,
                      ("u32Mode=%u\n", (unsigned) u32Mode));
    table.pfnUnload(NULL);
}

static void testGetHostMsg(void)
{
    struct VBOXHGCMSVCPARM parms[2];
    VBOXHGCMSVCFNTABLE table;
    VBOXHGCMCALLHANDLE_TYPEDEF call;
    int rc;

    RTTestISub("Setting up VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG test");
    rc = setupTable(&table);
    RTTESTI_CHECK_MSG_RETV(RT_SUCCESS(rc), ("rc=%Rrc\n", rc));
    /* Unless we are bidirectional the host message requests will be dropped. */
    parms[0].setUInt32(VBOX_SHARED_CLIPBOARD_MODE_BIDIRECTIONAL);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE,
                           1, parms);
    RTTESTI_CHECK_RC_OK(rc);

    RTTestISub("Testing FN_GET_HOST_MSG, one format, waiting guest call.");
    RT_ZERO(g_Client);
    parms[0].setUInt32(0);
    parms[1].setUInt32(0);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK_RC(call.rc, VERR_TRY_AGAIN);  /* This should get updated only when the guest call completes. */
    vboxSvcClipboardReportMsg (&g_Client, VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA,
                               VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT);
    RTTESTI_CHECK(parms[0].u.uint32 == VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA);
    RTTESTI_CHECK(parms[1].u.uint32 == VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT);
    RTTESTI_CHECK_RC_OK(call.rc);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK_RC(call.rc, VERR_TRY_AGAIN);  /* This call should not complete yet. */

    RTTestISub("Testing FN_GET_HOST_MSG, one format, no waiting guest calls.");
    RT_ZERO(g_Client);
    vboxSvcClipboardReportMsg (&g_Client, VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA,
                               VBOX_SHARED_CLIPBOARD_FMT_HTML);
    parms[0].setUInt32(0);
    parms[1].setUInt32(0);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK(parms[0].u.uint32 == VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA);
    RTTESTI_CHECK(parms[1].u.uint32 == VBOX_SHARED_CLIPBOARD_FMT_HTML);
    RTTESTI_CHECK_RC_OK(call.rc);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK_RC(call.rc, VERR_TRY_AGAIN);  /* This call should not complete yet. */

    RTTestISub("Testing FN_GET_HOST_MSG, two formats, waiting guest call.");
    RT_ZERO(g_Client);
    parms[0].setUInt32(0);
    parms[1].setUInt32(0);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK_RC(call.rc, VERR_TRY_AGAIN);  /* This should get updated only when the guest call completes. */
    vboxSvcClipboardReportMsg (&g_Client, VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA,
                               VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT | VBOX_SHARED_CLIPBOARD_FMT_HTML);
    RTTESTI_CHECK(parms[0].u.uint32 == VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA);
    RTTESTI_CHECK(parms[1].u.uint32 == VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT);
    RTTESTI_CHECK_RC_OK(call.rc);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK(parms[0].u.uint32 == VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA);
    RTTESTI_CHECK(parms[1].u.uint32 == VBOX_SHARED_CLIPBOARD_FMT_HTML);
    RTTESTI_CHECK_RC_OK(call.rc);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK_RC(call.rc, VERR_TRY_AGAIN);  /* This call should not complete yet. */

    RTTestISub("Testing FN_GET_HOST_MSG, two formats, no waiting guest calls.");
    RT_ZERO(g_Client);
    vboxSvcClipboardReportMsg (&g_Client, VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA,
                               VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT | VBOX_SHARED_CLIPBOARD_FMT_HTML);
    parms[0].setUInt32(0);
    parms[1].setUInt32(0);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK(parms[0].u.uint32 == VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA);
    RTTESTI_CHECK(parms[1].u.uint32 == VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT);
    RTTESTI_CHECK_RC_OK(call.rc);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK(parms[0].u.uint32 == VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA);
    RTTESTI_CHECK(parms[1].u.uint32 == VBOX_SHARED_CLIPBOARD_FMT_HTML);
    RTTESTI_CHECK_RC_OK(call.rc);
    call.rc = VERR_TRY_AGAIN;
    table.pfnCall(NULL, &call, 1 /* clientId */, &g_Client, VBOX_SHARED_CLIPBOARD_FN_GET_HOST_MSG,
                  2, parms);
    RTTESTI_CHECK_RC(call.rc, VERR_TRY_AGAIN);  /* This call should not complete yet. */
    table.pfnUnload(NULL);
}

static void testSetHeadless(void)
{
    struct VBOXHGCMSVCPARM parms[2];
    VBOXHGCMSVCFNTABLE table;
    bool fHeadless;
    int rc;

    RTTestISub("Testing HOST_FN_SET_HEADLESS");
    rc = setupTable(&table);
    RTTESTI_CHECK_MSG_RETV(RT_SUCCESS(rc), ("rc=%Rrc\n", rc));
    /* Reset global variable which doesn't reset itself. */
    parms[0].setUInt32(false);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_HEADLESS,
                           1, parms);
    RTTESTI_CHECK_RC_OK(rc);
    fHeadless = vboxSvcClipboardGetHeadless();
    RTTESTI_CHECK_MSG(fHeadless == false, ("fHeadless=%RTbool\n", fHeadless));
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_HEADLESS,
                           0, parms);
    RTTESTI_CHECK_RC(rc, VERR_INVALID_PARAMETER);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_HEADLESS,
                           2, parms);
    RTTESTI_CHECK_RC(rc, VERR_INVALID_PARAMETER);
    parms[0].setUInt64(99);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_HEADLESS,
                           1, parms);
    RTTESTI_CHECK_RC(rc, VERR_INVALID_PARAMETER);
    parms[0].setUInt32(true);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_HEADLESS,
                           1, parms);
    RTTESTI_CHECK_RC_OK(rc);
    fHeadless = vboxSvcClipboardGetHeadless();
    RTTESTI_CHECK_MSG(fHeadless == true, ("fHeadless=%RTbool\n", fHeadless));
    parms[0].setUInt32(99);
    rc = table.pfnHostCall(NULL, VBOX_SHARED_CLIPBOARD_HOST_FN_SET_HEADLESS,
                           1, parms);
    RTTESTI_CHECK_RC_OK(rc);
    fHeadless = vboxSvcClipboardGetHeadless();
    RTTESTI_CHECK_MSG(fHeadless == true, ("fHeadless=%RTbool\n", fHeadless));
    table.pfnUnload(NULL);
}

static void testHostCall(void)
{
    testSetMode();
    testSetHeadless();
}


int main(int argc, char *argv[])
{
    /*
     * Init the runtime, test and say hello.
     */
    const char *pcszExecName;
    NOREF(argc);
    pcszExecName = strrchr(argv[0], '/');
    pcszExecName = pcszExecName ? pcszExecName + 1 : argv[0];
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate(pcszExecName, &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    /*
     * Run the tests.
     */
    testHostCall();
    testGetHostMsg();

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}

int vboxClipboardInit() { return VINF_SUCCESS; }
void vboxClipboardDestroy() {}
void vboxClipboardDisconnect(_VBOXCLIPBOARDCLIENTDATA*) { AssertFailed(); }
int vboxClipboardConnect(_VBOXCLIPBOARDCLIENTDATA*, bool)
{ AssertFailed(); return VERR_WRONG_ORDER; }
void vboxClipboardFormatAnnounce(_VBOXCLIPBOARDCLIENTDATA*, unsigned int)
{ AssertFailed(); }
int vboxClipboardReadData(_VBOXCLIPBOARDCLIENTDATA*, unsigned int, void*, unsigned int, unsigned int*)
{ AssertFailed(); return VERR_WRONG_ORDER; }
void vboxClipboardWriteData(_VBOXCLIPBOARDCLIENTDATA*, void*, unsigned int, unsigned int) { AssertFailed(); }
int vboxClipboardSync(_VBOXCLIPBOARDCLIENTDATA*)
{ AssertFailed(); return VERR_WRONG_ORDER; }

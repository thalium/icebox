/* $Id: tstSSLCertDownloads.cpp $ */
/** @file
 * IPRT Testcase - Simple cURL testcase.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/test.h>


/*
 * It's a private class we're testing, so we must include the source.
 * Better than emedding the testcase as public function in the class.
 */
#include "UINetworkReply.cpp"



/*static*/ void UINetworkReplyPrivateThread::testIt(RTTEST hTest)
{
    NOREF(hTest);
    QUrl Dummy1;
    UserDictionary Dummy2;
    UINetworkReplyPrivateThread TestObj(UINetworkRequestType_GET, Dummy1, Dummy2);

    /*
     * Do the first setup things that UINetworkReplyPrivateThread::run.
     */
    RTTESTI_CHECK_RC(RTHttpCreate(&TestObj.m_hHttp), VINF_SUCCESS);
    RTTESTI_CHECK_RC(TestObj.applyProxyRules(), VINF_SUCCESS);

    /*
     * Duplicate much of downloadMissingCertificates, but making sure we
     * can both get the certificate from the ZIP file(s) and the first
     * PEM URL (as the rest are currently busted).
     */
    RTTestISub("SSL Cert downloading");
    RTCRSTORE hStore;
    RTTESTI_CHECK_RC(RTCrStoreCreateInMem(&hStore, RT_ELEMENTS(s_aCerts) * 2), VINF_SUCCESS);

    int rc;

    /* ZIP files: */
    for (uint32_t iUrl = 0; iUrl < RT_ELEMENTS(s_apszRootsZipUrls); iUrl++)
    {
        RTTestIPrintf(RTTESTLVL_ALWAYS, "URL: %s\n", s_apszRootsZipUrls[iUrl]);
        void   *pvRootsZip;
        size_t  cbRootsZip;
        rc = RTHttpGetBinary(TestObj.m_hHttp, s_apszRootsZipUrls[iUrl], &pvRootsZip, &cbRootsZip);
        if (RT_SUCCESS(rc))
        {
            for (uint32_t i = 0; i < RT_ELEMENTS(s_aCerts); i++)
            {
                CERTINFO const *pInfo = (CERTINFO const *)s_aCerts[i].pvUser;
                if (pInfo->pszZipFile)
                {
                    void  *pvFile;
                    size_t cbFile;
                    RTTESTI_CHECK_RC_BREAK(RTZipPkzipMemDecompress(&pvFile, &cbFile, pvRootsZip, cbRootsZip, pInfo->pszZipFile),
                                           VINF_SUCCESS);
                    RTTESTI_CHECK_RC(convertVerifyAndAddPemCertificateToStore(hStore, pvFile, cbFile, &TestObj.s_aCerts[i]),
                                     VINF_SUCCESS);
                    RTMemFree(pvFile);
                }
            }
            RTHttpFreeResponse(pvRootsZip);
        }
        else if (   rc != VERR_HTTP_PROXY_NOT_FOUND
                 && rc != VERR_HTTP_COULDNT_CONNECT)
            RTTestIFailed("%Rrc on '%s'", rc, s_apszRootsZipUrls[iUrl]); /* code or link broken */
    }

    /* PEM files: */
    for (uint32_t i = 0; i < RT_ELEMENTS(s_aCerts); i++)
    {
        uint32_t iUrl = 0; /* First URL must always work. */
        UINetworkReplyPrivateThread::CERTINFO const *pInfo;
        pInfo = (UINetworkReplyPrivateThread::CERTINFO const *)s_aCerts[i].pvUser;
        if (pInfo->apszUrls[iUrl])
        {
            RTTestIPrintf(RTTESTLVL_ALWAYS, "URL: %s\n", pInfo->apszUrls[iUrl]);
            void  *pvResponse;
            size_t cbResponse;
            rc = RTHttpGetBinary(TestObj.m_hHttp, pInfo->apszUrls[iUrl], &pvResponse, &cbResponse);
            if (RT_SUCCESS(rc))
            {
                RTTESTI_CHECK_RC_OK(convertVerifyAndAddPemCertificateToStore(hStore, pvResponse, cbResponse,
                                                                             &TestObj.s_aCerts[i]));
                RTHttpFreeResponse(pvResponse);
            }
            else if (   rc != VERR_HTTP_PROXY_NOT_FOUND
                     && rc != VERR_HTTP_COULDNT_CONNECT)
                RTTestIFailed("%Rrc on '%s'", rc, pInfo->apszUrls[iUrl]); /* code or link broken */
        }
    }

    RTTESTI_CHECK(RTCrStoreRelease(hStore) == 0);

    /*
     * Now check the gathering of certificates on the system doesn't crash.
     */
    RTTestISub("Refreshing certificates");

    /* create an empty store and do the refresh operation, writing it to /dev/null. */
    RTTESTI_CHECK_RC(RTCrStoreCreateInMem(&hStore, RT_ELEMENTS(s_aCerts) * 2), VINF_SUCCESS);
    bool afFoundCerts[RT_ELEMENTS(s_aCerts)];
    RT_ZERO(afFoundCerts);
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    RTTESTI_CHECK_RC_OK(TestObj.refreshCertificates(NIL_RTHTTP, &hStore, afFoundCerts, "nul"));
#else
    RTTESTI_CHECK_RC_OK(TestObj.refreshCertificates(NIL_RTHTTP, &hStore, afFoundCerts, "/dev/null"));
#endif

    /* Log how many certificates we found and require at least one. */
    uint32_t cCerts = RTCrStoreCertCount(hStore);
    RTTestIValue("certificates", cCerts, RTTESTUNIT_NONE);
    RTTESTI_CHECK(cCerts > 0);

    RTTESTI_CHECK(RTCrStoreRelease(hStore) == 0);

    /*
     * We're done.
     */
    //RTTESTI_CHECK_RC(RTHttpDestroy(TestObj.m_hHttp), VINF_SUCCESS);
    RTHttpDestroy(TestObj.m_hHttp);
    TestObj.m_hHttp = NIL_RTHTTP;
}


int main()
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstSSLCertDownloads", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    UINetworkReplyPrivateThread::testIt(hTest);

    return RTTestSummaryAndDestroy(hTest);
}


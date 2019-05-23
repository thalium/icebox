/* $Id: tstRTLdrVerifyPeImage.cpp $ */
/** @file
 * SUP Testcase - Testing the Authenticode signature verification code.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#include <iprt/test.h>
#include <iprt/mem.h>
#include <iprt/ldr.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static int g_iDummy = 0;

static DECLCALLBACK(int) TestCallback(RTLDRMOD hLdrMod, RTLDRSIGNATURETYPE enmSignature,
                                      void const *pvSignature, size_t cbSignature,
                                      PRTERRINFO pErrInfo, void *pvUser)
{
    RT_NOREF_PV(hLdrMod); RT_NOREF_PV(enmSignature); RT_NOREF_PV(pvSignature); RT_NOREF_PV(cbSignature);
    RT_NOREF_PV(pErrInfo); RT_NOREF_PV(pvUser);
    return VINF_SUCCESS;
}



int main(int argc, char **argv)
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstAuthenticode", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    /*
     * Process input.
     */
    for (int i = 1; i < argc; i++)
    {
        const char *pszFullName = argv[i];
        const char *pszFilename = RTPathFilename(pszFullName);
        RTTestSub(hTest, pszFilename);

        RTLDRMOD hLdrMod;
        int rc = RTLdrOpen(pszFullName, RTLDR_O_FOR_VALIDATION, RTLDRARCH_WHATEVER, &hLdrMod);
        if (RT_SUCCESS(rc))
        {
            char szDigest[512];

            RTTESTI_CHECK_RC(rc = RTLdrHashImage(hLdrMod, RTDIGESTTYPE_MD5, szDigest, sizeof(szDigest)), VINF_SUCCESS);
            if (RT_SUCCESS(rc))
                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "md5=%s\n", szDigest);
            RTTESTI_CHECK_RC(rc = RTLdrHashImage(hLdrMod, RTDIGESTTYPE_SHA1, szDigest, sizeof(szDigest)), VINF_SUCCESS);
            if (RT_SUCCESS(rc))
                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "sha1=%s\n", szDigest);
            RTTESTI_CHECK_RC(rc = RTLdrHashImage(hLdrMod, RTDIGESTTYPE_SHA256, szDigest, sizeof(szDigest)), VINF_SUCCESS);
            if (RT_SUCCESS(rc))
                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "sha256=%s\n", szDigest);
            RTTESTI_CHECK_RC(rc = RTLdrHashImage(hLdrMod, RTDIGESTTYPE_SHA512, szDigest, sizeof(szDigest)), VINF_SUCCESS);
            if (RT_SUCCESS(rc))
                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "sha512=%s\n", szDigest);

            if (rc != VERR_NOT_SUPPORTED)
            {
                RTERRINFOSTATIC ErrInfo;
                RTErrInfoInitStatic(&ErrInfo);
                rc = RTLdrVerifySignature(hLdrMod, TestCallback, &g_iDummy, &ErrInfo.Core);
                if (RT_FAILURE(rc))
                    RTTestFailed(hTest, "%s: %s (rc=%Rrc)", pszFilename, ErrInfo.Core.pszMsg, rc);
            }
            RTLdrClose(hLdrMod);
        }
        else
            RTTestFailed(hTest, "Error opening '%s': %Rrc\n", pszFullName, rc);
    }

    return RTTestSummaryAndDestroy(hTest);
}


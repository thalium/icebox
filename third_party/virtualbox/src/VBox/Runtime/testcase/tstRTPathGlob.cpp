/* $Id: tstRTPathGlob.cpp $ */
/** @file
 * IPRT Testcase - Manual RTPathGlob test.
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
#include <iprt/path.h>

#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/test.h>


int main(int argc, char **argv)
{
    /*
     * Init RT+Test.
     */
    RTTEST hTest;
    int rc = RTTestInitExAndCreate(argc, &argv, 0, "tstRTPathGlob", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    if (argc <= 1)
        return RTTestSkipAndDestroy(hTest, "Requires arguments");


    /*
     * Manual glob testing.
     */
    for (int i = 1; i < argc; i++)
    {
        uint32_t            cResults = UINT32_MAX;
        PCRTPATHGLOBENTRY   pHead = (PCRTPATHGLOBENTRY)&cResults;
        rc = RTPathGlob(argv[i], 0, &pHead, &cResults);
        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "#%u '%s' -> %Rrc cResult=%u\n", i, argv[i], rc, cResults);
        if (RT_SUCCESS(rc))
        {
            uint32_t iEntry = 0;
            for (PCRTPATHGLOBENTRY pCur = pHead; pCur; pCur = pCur->pNext, iEntry++)
            {
                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "  #%3u: '%s'\n", iEntry, pCur->szPath);
                RTTEST_CHECK(hTest, strlen(pCur->szPath) == pCur->cchPath);
            }

            RTPathGlobFree(pHead);
        }
    }


    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}


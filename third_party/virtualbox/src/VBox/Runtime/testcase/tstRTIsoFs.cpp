/* $Id: tstRTIsoFs.cpp $ */
/** @file
 * IPRT Testcase - RTIsoFs manual testcase.
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
#include <iprt/isofs.h>

#include <iprt/err.h>
#include <iprt/test.h>
#include <iprt/string.h>


int main(int argc, char **argv)
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitExAndCreate(argc, &argv, 0 /*fRtInit*/, "tstRTIsoFs", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    if (argc <= 1)
        return RTTestSkipAndDestroy(hTest, "no input");

    /*
     * First argument is the ISO to open.
     */
    RTISOFSFILE IsoFs;
    int rc = RTIsoFsOpen(&IsoFs, argv[1]);
    if (RT_SUCCESS(rc))
    {
        /*
         * Remaining arguments specifies files in the ISO that we wish information
         * about and optionally extract.
         */
        for (int i = 2; i < argc; i++)
        {
            char *pszFile = argv[i];
            char  chSaved = 0;
            char *pszDst  = strchr(pszFile, '=');
            if (pszDst)
            {
                chSaved = *pszDst;
                *pszDst = '\0';
            }

            uint32_t    offFile = UINT32_MAX / 2;
            size_t      cbFile  = UINT32_MAX / 2;
            rc = RTIsoFsGetFileInfo(&IsoFs, pszFile, &offFile, &cbFile);
            if (RT_SUCCESS(rc))
            {
                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "%s: %u bytes at %#x\n", pszFile, (uint32_t)cbFile, offFile);
                if (pszDst)
                {
                    rc = RTIsoFsExtractFile(&IsoFs, pszFile, pszDst);
                    if (RT_SUCCESS(rc))
                        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "%s: saved as '%s'.\n", pszFile, pszDst);
                    else
                        RTTestFailed(hTest, "RTIsoFsExtractFile failed to extract '%s' to '%s': %Rrc", pszFile, pszDst, rc);
                }
            }
            else
                RTTestFailed(hTest, "RTIsoFsGetFileInfo failed for '%s': %Rrc", pszFile, rc);

            if (pszDst)
                pszDst[-1] = chSaved;
        }

        RTIsoFsClose(&IsoFs);
    }
    else
        RTTestFailed(hTest, "RTIsoFsOpen failed to open '%s': %Rrc", argv[1], rc);
    return RTTestSummaryAndDestroy(hTest);
}


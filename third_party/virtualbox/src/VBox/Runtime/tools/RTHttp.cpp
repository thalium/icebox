/* $Id: RTHttp.cpp $ */
/** @file
 * IPRT - Utility for retriving URLs.
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
#include <iprt/http.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/vfs.h>



int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    /*
     * Create a HTTP client instance.
     */
    RTHTTP hHttp;
    rc = RTHttpCreate(&hHttp);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTHttpCreate failed: %Rrc", rc);

    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--output",       'o', RTGETOPT_REQ_STRING },
        { "--quiet",        'q', RTGETOPT_REQ_NOTHING },
        { "--verbose",      'v', RTGETOPT_REQ_NOTHING },
    };

    RTEXITCODE      rcExit          = RTEXITCODE_SUCCESS;
    const char     *pszOutput       = NULL;
    unsigned        uVerbosityLevel = 1;

    RTGETOPTUNION   ValueUnion;
    RTGETOPTSTATE   GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, RTGETOPTINIT_FLAGS_OPTS_FIRST);
    while ((rc = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (rc)
        {
            case 'o':
                pszOutput = ValueUnion.psz;
                break;

            case 'q':
                uVerbosityLevel--;
                break;
            case 'v':
                uVerbosityLevel++;
                break;

            case 'h':
                RTPrintf("Usage: %s [options] URL0 [URL1 [...]]\n"
                         "\n"
                         "Options:\n"
                         "  -o,--output=file\n"
                         "      Output file. If not given, the file is displayed on stdout.\n"
                         "  -q, --quiet\n"
                         "  -v, --verbose\n"
                         "      Controls the verbosity level.\n"
                         "  -h, -?, --help\n"
                         "      Display this help text and exit successfully.\n"
                         "  -V, --version\n"
                         "      Display the revision and exit successfully.\n"
                         , RTPathFilename(argv[0]));
                return RTEXITCODE_SUCCESS;

            case 'V':
                RTPrintf("$Revision: 118769 $\n");
                return RTEXITCODE_SUCCESS;

            case VINF_GETOPT_NOT_OPTION:
            {
                int rcHttp;
                if (pszOutput && strcmp("-", pszOutput))
                {
                    if (uVerbosityLevel > 0)
                        RTStrmPrintf(g_pStdErr, "Fetching '%s' into '%s'...\n", ValueUnion.psz, pszOutput);
                    rcHttp = RTHttpGetFile(hHttp, ValueUnion.psz, pszOutput);
                }
                else
                {
                    if (uVerbosityLevel > 0)
                        RTStrmPrintf(g_pStdErr, "Fetching '%s'...\n", ValueUnion.psz);

                    void  *pvResp;
                    size_t cbResp;
                    rcHttp = RTHttpGetBinary(hHttp, ValueUnion.psz, &pvResp, &cbResp);
                    if (RT_SUCCESS(rcHttp))
                    {
                        RTVFSIOSTREAM hVfsIos;
                        rc = RTVfsIoStrmFromStdHandle(RTHANDLESTD_OUTPUT, 0, true /*fLeaveOpen*/, &hVfsIos);
                        if (RT_SUCCESS(rc))
                        {
                            rc = RTVfsIoStrmWrite(hVfsIos, pvResp, cbResp, true /*fBlocking*/, NULL);
                            if (RT_FAILURE(rc))
                                rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Error writing to stdout: %Rrc", rc);
                            RTVfsIoStrmRelease(hVfsIos);
                        }
                        else
                            rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Error opening stdout: %Rrc", rc);
                        RTHttpFreeResponse(pvResp);
                    }
                }
                if (RT_FAILURE(rcHttp))
                    rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Error %Rrc getting '%s'", rcHttp, ValueUnion.psz);
                break;
            }

            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }

    return rcExit;
}


/* $Id: tstRTZip.cpp $ */
/** @file
 * IPRT Testcase - RTZip, kind of.
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
#include <iprt/zip.h>

#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/param.h>
#include <iprt/string.h>
#include <iprt/test.h>


static void testFile(const char *pszFilename)
{
    size_t  cbSrcActually = 0;
    void   *pvSrc;
    size_t  cbSrc;
    int rc = RTFileReadAll(pszFilename, &pvSrc, &cbSrc);
    RTTESTI_CHECK_RC_OK_RETV(rc);

    size_t  cbDstActually = 0;
    size_t  cbDst = RT_MAX(cbSrc * 8, _1M);
    void   *pvDst = RTMemAllocZ(cbDst);

    rc = RTZipBlockDecompress(RTZIPTYPE_ZLIB, 0, pvSrc, cbSrc, &cbSrcActually, pvDst, cbDst, &cbDstActually);
    RTTestIPrintf(RTTESTLVL_ALWAYS, "cbSrc=%zu cbSrcActually=%zu cbDst=%zu cbDstActually=%zu rc=%Rrc\n",
                  cbSrc, cbSrcActually, cbDst, cbDstActually, rc);
    RTTESTI_CHECK_RC_OK(rc);

}


int main(int argc, char **argv)
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTZip", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
            testFile(argv[i]);
    }
    else
    {
        /** @todo testcase */
    }

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}


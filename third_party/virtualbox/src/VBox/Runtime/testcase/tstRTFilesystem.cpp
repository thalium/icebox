/* $Id: tstRTFilesystem.cpp $ */
/** @file
 * IPRT Testcase - IPRT Filesystem API (Fileystem)
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
#include <iprt/filesystem.h>
#include <iprt/vfs.h>
#include <iprt/err.h>
#include <iprt/test.h>
#include <iprt/file.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

static int tstRTFilesystem(RTTEST hTest, RTVFSFILE hVfsFile)
{
    int rc = VINF_SUCCESS;
    RTVFS hVfs = NIL_RTVFS;

    RTTestSubF(hTest, "Create filesystem object");

    rc = RTFilesystemVfsFromFile(hVfsFile, &hVfs);
    if (RT_FAILURE(rc))
    {
        RTTestIFailed("RTFilesystemVfsFromFile -> %Rrc", rc);
        return rc;
    }

    /* Check all blocks. */
    uint64_t off = 0;
    uint32_t cBlocksUsed = 0;
    uint32_t cBlocksUnused = 0;
    uint64_t cbFs = 0;

    rc = RTVfsFileGetSize(hVfsFile, &cbFs);
    if (RT_FAILURE(rc))
    {
        RTTestIFailed("RTVfsFileGetSize -> %Rrc", rc);
        return rc;
    }

    while (off < cbFs)
    {
        bool fUsed = false;

        rc = RTVfsIsRangeInUse(hVfs, off, 1024, &fUsed);
        if (RT_FAILURE(rc))
        {
            RTTestIFailed("RTVfsIsRangeInUse -> %Rrc", rc);
            break;
        }

        if (fUsed)
            cBlocksUsed++;
        else
            cBlocksUnused++;

        off += 1024;
    }

    if (RT_SUCCESS(rc))
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%u blocks used and %u blocks unused\n",
                      cBlocksUsed, cBlocksUnused);

    RTVfsRelease(hVfs);

    return rc;
}

int main(int argc, char **argv)
{
    /*
     * Initialize IPRT and create the test.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTFilesystem", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * If no args, display usage.
     */
    if (argc < 2)
    {
        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "Syntax: %s <image>\n", argv[0]);
        return RTTestSkipAndDestroy(hTest, "Missing required arguments\n");
    }

    /* Open image. */
    RTFILE hFile;
    RTVFSFILE hVfsFile;
    rc = RTFileOpen(&hFile, argv[1], RTFILE_O_OPEN | RTFILE_O_DENY_NONE | RTFILE_O_READ);
    if (RT_FAILURE(rc))
    {
        RTTestIFailed("RTFileOpen -> %Rrc", rc);
        return RTTestSummaryAndDestroy(hTest);
    }

    rc = RTVfsFileFromRTFile(hFile, 0, false, &hVfsFile);
    if (RT_FAILURE(rc))
    {
        RTTestIFailed("RTVfsFileFromRTFile -> %Rrc", rc);
        return RTTestSummaryAndDestroy(hTest);
    }

    rc = tstRTFilesystem(hTest, hVfsFile);

    RTTESTI_CHECK(rc == VINF_SUCCESS);

    RTVfsFileRelease(hVfsFile);

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}


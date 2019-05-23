/* $Id: tstFile.cpp $ */
/** @file
 * IPRT Testcase - File I/O.
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
#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/initterm.h>


int main()
{
    int         cErrors = 0;
    RTPrintf("tstFile: TESTING\n");
    RTR3InitExeNoArguments(0);

    RTFILE    File;
    int rc = RTFileOpen(&File, "tstFile#1.tst", RTFILE_O_READWRITE | RTFILE_O_CREATE_REPLACE | RTFILE_O_DENY_NONE);
    if (RT_FAILURE(rc))
    {
        RTPrintf("tstFile: FATAL ERROR - Failed to open file #1. rc=%Rrc\n", rc);
        return 1;
    }

    RTFOFF cbMax = -2;
    rc = RTFileGetMaxSizeEx(File, &cbMax);
    if (RT_FAILURE(rc))
    {
        RTPrintf("tstFile: RTFileGetMaxSizeEx failed: %Rrc\n", rc);
        cErrors++;
    }
    else if (cbMax <= 0)
    {
        RTPrintf("tstFile: RTFileGetMaxSizeEx failed: cbMax=%RTfoff\n", cbMax);
        cErrors++;
    }
    else if (RTFileGetMaxSize(File) != cbMax)
    {
        RTPrintf("tstFile: RTFileGetMaxSize failed; returns %RTfoff instead of %RTfoff\n", RTFileGetMaxSize(File), cbMax);
        cErrors++;
    }
    else
        RTPrintf("Maximum file size is %RTfoff bytes.\n", cbMax);

    /* grow file beyond 2G */
    rc = RTFileSetSize(File, _2G + _1M);
    if (RT_FAILURE(rc))
    {
        RTPrintf("Failed to grow file #1 to 2.001GB. rc=%Rrc\n", rc);
        cErrors++;
    }
    else
    {
        uint64_t cb;
        rc = RTFileGetSize(File, &cb);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failed to get file size of #1. rc=%Rrc\n", rc);
            cErrors++;
        }
        else if (cb != _2G + _1M)
        {
            RTPrintf("RTFileGetSize return %RX64 bytes, expected %RX64.\n", cb, _2G + _1M);
            cErrors++;
        }
        else
            RTPrintf("tstFile: cb=%RX64\n", cb);

        /*
         * Try some writes at the beginning of the file.
         */
        uint64_t offFile = RTFileTell(File);
        if (offFile != 0)
        {
            RTPrintf("RTFileTell -> %#RX64, expected 0 (#1)\n", offFile);
            cErrors++;
        }
        static const char szTestBuf[] = "Sausages and bacon for breakfast again!";
        size_t cbWritten = 0;
        while (cbWritten < sizeof(szTestBuf))
        {
            size_t cbWrittenPart;
            rc = RTFileWrite(File, &szTestBuf[cbWritten], sizeof(szTestBuf) - cbWritten, &cbWrittenPart);
            if (RT_FAILURE(rc))
                break;
            cbWritten += cbWrittenPart;
        }
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failed to write to file #1 at offset 0. rc=%Rrc\n", rc);
            cErrors++;
        }
        else
        {
            /* check that it was written correctly. */
            rc = RTFileSeek(File, 0, RTFILE_SEEK_BEGIN, NULL);
            if (RT_FAILURE(rc))
            {
                RTPrintf("Failed to seek offset 0 in file #1. rc=%Rrc\n", rc);
                cErrors++;
            }
            else
            {
                char        szReadBuf[sizeof(szTestBuf)];
                size_t      cbRead = 0;
                while (cbRead < sizeof(szTestBuf))
                {
                    size_t cbReadPart;
                    rc = RTFileRead(File, &szReadBuf[cbRead], sizeof(szTestBuf) - cbRead, &cbReadPart);
                    if (RT_FAILURE(rc))
                        break;
                    cbRead += cbReadPart;
                }
                if (RT_FAILURE(rc))
                {
                    RTPrintf("Failed to read from file #1 at offset 0. rc=%Rrc\n", rc);
                    cErrors++;
                }
                else
                {
                    if (!memcmp(szReadBuf, szTestBuf, sizeof(szTestBuf)))
                        RTPrintf("tstFile: head write ok\n");
                    else
                    {
                        RTPrintf("Data read from file #1 at offset 0 differs from what we wrote there.\n");
                        cErrors++;
                    }
                }
            }
        }

        /*
         * Try some writes at the end of the file.
         */
        rc = RTFileSeek(File, _2G + _1M, RTFILE_SEEK_BEGIN, NULL);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failed to seek to _2G + _1M in file #1. rc=%Rrc\n", rc);
            cErrors++;
        }
        else
        {
            offFile = RTFileTell(File);
            if (offFile != _2G + _1M)
            {
                RTPrintf("RTFileTell -> %#llx, expected %#llx (#2)\n", offFile, _2G + _1M);
                cErrors++;
            }
            else
            {
                cbWritten = 0;
                while (cbWritten < sizeof(szTestBuf))
                {
                    size_t cbWrittenPart;
                    rc = RTFileWrite(File, &szTestBuf[cbWritten], sizeof(szTestBuf) - cbWritten, &cbWrittenPart);
                    if (RT_FAILURE(rc))
                        break;
                    cbWritten += cbWrittenPart;
                }
                if (RT_FAILURE(rc))
                {
                    RTPrintf("Failed to write to file #1 at offset 2G + 1M.  rc=%Rrc\n", rc);
                    cErrors++;
                }
                else
                {
                    rc = RTFileSeek(File, offFile, RTFILE_SEEK_BEGIN, NULL);
                    if (RT_FAILURE(rc))
                    {
                        RTPrintf("Failed to seek offset %RX64 in file #1. rc=%Rrc\n", offFile, rc);
                        cErrors++;
                    }
                    else
                    {
                        char        szReadBuf[sizeof(szTestBuf)];
                        size_t      cbRead = 0;
                        while (cbRead < sizeof(szTestBuf))
                        {
                            size_t cbReadPart;
                            rc = RTFileRead(File, &szReadBuf[cbRead], sizeof(szTestBuf) - cbRead, &cbReadPart);
                            if (RT_FAILURE(rc))
                                break;
                            cbRead += cbReadPart;
                        }
                        if (RT_FAILURE(rc))
                        {
                            RTPrintf("Failed to read from file #1 at offset 2G + 1M.  rc=%Rrc\n", rc);
                            cErrors++;
                        }
                        else
                        {
                            if (!memcmp(szReadBuf, szTestBuf, sizeof(szTestBuf)))
                                RTPrintf("tstFile: tail write ok\n");
                            else
                            {
                                RTPrintf("Data read from file #1 at offset 2G + 1M differs from what we wrote there.\n");
                                cErrors++;
                            }
                        }
                    }
                }
            }
        }

        /*
         * Some general seeking around.
         */
        rc = RTFileSeek(File, _2G + 1, RTFILE_SEEK_BEGIN, NULL);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failed to seek to _2G + 1 in file #1. rc=%Rrc\n", rc);
            cErrors++;
        }
        else
        {
            offFile = RTFileTell(File);
            if (offFile != _2G + 1)
            {
                RTPrintf("RTFileTell -> %#llx, expected %#llx (#3)\n", offFile, _2G + 1);
                cErrors++;
            }
        }

        /* seek end */
        rc = RTFileSeek(File, 0, RTFILE_SEEK_END, NULL);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failed to seek to end of file #1. rc=%Rrc\n", rc);
            cErrors++;
        }
        else
        {
            offFile = RTFileTell(File);
            if (offFile != _2G + _1M + sizeof(szTestBuf)) /* assuming tail write was ok. */
            {
                RTPrintf("RTFileTell -> %#RX64, expected %#RX64 (#4)\n", offFile, _2G + _1M + sizeof(szTestBuf));
                cErrors++;
            }
        }

        /* seek start */
        rc = RTFileSeek(File, 0, RTFILE_SEEK_BEGIN, NULL);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failed to seek to end of file #1. rc=%Rrc\n", rc);
            cErrors++;
        }
        else
        {
            offFile = RTFileTell(File);
            if (offFile != 0)
            {
                RTPrintf("RTFileTell -> %#llx, expected 0 (#5)\n", offFile);
                cErrors++;
            }
        }
    }


    /*
     * Cleanup.
     */
    rc = RTFileClose(File);
    if (RT_FAILURE(rc))
    {
        RTPrintf("Failed to close file #1. rc=%Rrc\n", rc);
        cErrors++;
    }
    rc = RTFileDelete("tstFile#1.tst");
    if (RT_FAILURE(rc))
    {
        RTPrintf("Failed to delete file #1. rc=%Rrc\n", rc);
        cErrors++;
    }

    /*
     * Summary
     */
    if (cErrors == 0)
        RTPrintf("tstFile: SUCCESS\n");
    else
        RTPrintf("tstFile: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}


/* $Id: tstSSM-2.cpp $ */
/** @file
 * Saved State Manager Testcase: Extract the content of a saved state.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
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
#include <VBox/vmm/ssm.h>

#include <VBox/log.h>
#include <iprt/getopt.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/stream.h>

static RTEXITCODE extractUnit(const char *pszFilename, const char *pszUnitname, const char *pszOutputFilename)
{
    PSSMHANDLE pSSM;
    int rc = SSMR3Open(pszFilename, 0, &pSSM);
    RTEXITCODE rcExit = RTEXITCODE_FAILURE;
    if (RT_SUCCESS(rc))
    {
        RTFILE hFile;
        rc = RTFileOpen(&hFile, pszOutputFilename, RTFILE_O_DENY_NONE | RTFILE_O_WRITE | RTFILE_O_CREATE);
        if (RT_SUCCESS(rc))
        {
            uint32_t version = 0;
            rc = SSMR3Seek(pSSM, pszUnitname, 0 /* iInstance */, &version);
            size_t cbUnit = 0;
            if (RT_SUCCESS(rc))
            {
                for (;;)
                {
                    uint8_t u8;
                    rc = SSMR3GetU8(pSSM, &u8);
                    if (RT_FAILURE(rc))
                        break;
                    size_t cbWritten;
                    rc = RTFileWrite(hFile, &u8, sizeof(u8), &cbWritten);
                    cbUnit++;
                }
                RTPrintf("Unit size %zu bytes, version %d\n", cbUnit, version);
            }
            else
                RTPrintf("Cannot find unit '%s' (%Rrc)\n", pszUnitname, rc);
            RTFileClose(hFile);
        }
        else
            RTPrintf("Cannot open output file '%s' (%Rrc)\n", pszOutputFilename, rc);
        SSMR3Close(pSSM);
    }
    else
        RTPrintf("Cannot open SSM file '%s' (%Rrc)\n", pszFilename, rc);
    return rcExit;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        RTPrintf("Usage: %s <SSM filename> <SSM unitname> <outfile>\n", RTPathFilename(argv[0]));
        /* don't fail by default */
        return RTEXITCODE_SUCCESS;
    }
    return extractUnit(argv[1], argv[2], argv[3]);
}

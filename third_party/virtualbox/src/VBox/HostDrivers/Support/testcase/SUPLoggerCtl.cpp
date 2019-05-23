/* $Id: SUPLoggerCtl.cpp $ */
/** @file
 * SUPLoggerCtl - Support Driver Logger Control.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include <VBox/sup.h>
#include <iprt/buildconfig.h>
#include <iprt/initterm.h>
#include <iprt/getopt.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/ctype.h>
#include <iprt/err.h>


/**
 * Prints the usage.
 * @returns 1.
 */
static int usage(void)
{
    RTPrintf("usage: SUPLoggerCtl [-f|--flags <flags-settings>] \\\n"
             "                    [-g|--groups <groups-settings>] \\\n"
             "                    [-d|--dest <destination-specifiers>] \\\n"
             "                    [-l|--which <release|debug>] \\\n"
             "                    [-o|--what <set|create|destroy>]\n"
             "   or: SUPLoggerCtl <-h|--help>\n"
             "\n"
             );
    return 1;
}


int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_SUPLIB);

    /*
     * Options are mandatory.
     */
    if (argc <= 1)
        return usage();

    /*
     * Parse the options.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--flags",    'f', RTGETOPT_REQ_STRING },
        { "--groups",   'g', RTGETOPT_REQ_STRING },
        { "--dest",     'd', RTGETOPT_REQ_STRING },
        { "--what",     'o', RTGETOPT_REQ_STRING },
        { "--which",    'l', RTGETOPT_REQ_STRING },
    };

    const char *pszFlags  = "";
    const char *pszGroups = "";
    const char *pszDest   = "";
    SUPLOGGER   enmWhich  = SUPLOGGER_DEBUG;
    enum
    {
        kSupLoggerCtl_Set, kSupLoggerCtl_Create, kSupLoggerCtl_Destroy
    }           enmWhat   = kSupLoggerCtl_Set;

    int ch;
    RTGETOPTUNION Val;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0);
    while ((ch = RTGetOpt(&GetState, &Val)))
    {
        switch (ch)
        {
            case 'f':
                pszFlags = Val.psz;
                break;

            case 'g':
                pszGroups = Val.psz;
                break;

            case 'd':
                pszDest = Val.psz;
                break;

            case 'o':
                if (!strcmp(Val.psz, "set"))
                    enmWhat = kSupLoggerCtl_Set;
                else if (!strcmp(Val.psz, "create"))
                    enmWhat = kSupLoggerCtl_Create;
                else if (!strcmp(Val.psz, "destroy"))
                    enmWhat = kSupLoggerCtl_Destroy;
                else
                {
                    RTStrmPrintf(g_pStdErr, "SUPLoggerCtl: error: Unknown operation '%s'.\n", Val.psz);
                    return 1;
                }
                break;

            case 'l':
                if (!strcmp(Val.psz, "debug"))
                    enmWhich = SUPLOGGER_DEBUG;
                else if (!strcmp(Val.psz, "release"))
                    enmWhich = SUPLOGGER_RELEASE;
                else
                {
                    RTStrmPrintf(g_pStdErr, "SUPLoggerCtl: error: Unknown logger '%s'.\n", Val.psz);
                    return 1;
                }
                break;

            case 'h':
                return usage();

            case 'V':
                RTPrintf("%sr%s\n", RTBldCfgVersion(), RTBldCfgRevisionStr());
                return 0;

            case VINF_GETOPT_NOT_OPTION:
                RTStrmPrintf(g_pStdErr, "SUPLoggerCtl: error: Unexpected argument '%s'.\n", Val.psz);
                return 1;

            default:
                return RTGetOptPrintError(ch, &Val);
        }
    }

    /*
     * Do the requested job.
     */
    int rc;
    switch (enmWhat)
    {
        case kSupLoggerCtl_Set:
            rc = SUPR3LoggerSettings(enmWhich, pszFlags, pszGroups, pszDest);
            break;
        case kSupLoggerCtl_Create:
            rc = SUPR3LoggerCreate(enmWhich, pszFlags, pszGroups, pszDest);
            break;
        case kSupLoggerCtl_Destroy:
            rc = SUPR3LoggerDestroy(enmWhich);
            break;
        default:
            rc = VERR_INTERNAL_ERROR;
            break;
    }
    if (RT_SUCCESS(rc))
        RTPrintf("SUPLoggerCtl: Success\n");
    else
        RTStrmPrintf(g_pStdErr, "SUPLoggerCtl: error: rc=%Rrc\n", rc);

    return RT_SUCCESS(rc) ? 0 : 1;
}


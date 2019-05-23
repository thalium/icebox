/* $Id: loadgenerator.cpp $ */
/** @file
 * Load Generator.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#include <iprt/thread.h>
#include <iprt/time.h>
#include <iprt/initterm.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/mp.h>
#include <iprt/asm.h>
#include <iprt/getopt.h>
#include <VBox/sup.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Whether the threads should quit or not. */
static bool volatile    g_fQuit = false;
static const char      *g_pszProgramName = NULL;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int Error(const char *pszFormat, ...);


static void LoadGenSpin(uint64_t cNanoSeconds)
{
    const uint64_t u64StartTS = RTTimeNanoTS();
    do
    {
        for (uint32_t volatile i = 0; i < 10240 && !g_fQuit; i++)
            i++;
    } while (RTTimeNanoTS() - u64StartTS < cNanoSeconds && !g_fQuit);
}


static DECLCALLBACK(int) LoadGenSpinThreadFunction(RTTHREAD hThreadSelf, void *pvUser)
{
    NOREF(hThreadSelf);
    LoadGenSpin(*(uint64_t *)pvUser);
    return VINF_SUCCESS;
}


static int LoadGenIpiInit(void)
{
    /*
     * Try make sure the support library is initialized...
     */
    SUPR3Init(NULL);

    /*
     * Load the module.
     */
    char szPath[RTPATH_MAX];
    int rc = RTPathAppPrivateArchTop(szPath, sizeof(szPath) - sizeof("/loadgenerator.r0"));
    if (RT_SUCCESS(rc))
    {
        strcat(szPath, "/loadgeneratorR0.r0");
        void *pvImageBase;
        rc = SUPR3LoadServiceModule(szPath, "loadgeneratorR0", "LoadGenR0ServiceReqHandler", &pvImageBase);
        if (RT_SUCCESS(rc))
        {
            /* done */
        }
        else
            Error("SUPR3LoadServiceModule(%s): %Rrc\n", szPath, rc);
    }
    else
        Error("RTPathAppPrivateArch: %Rrc\n", rc);
    return rc;
}


static void LoadGenIpi(uint64_t cNanoSeconds)
{
    const uint64_t u64StartTS = RTTimeNanoTS();
    do
    {
        int rc = SUPR3CallR0Service("loadgeneratorR0", sizeof("loadgeneratorR0") - 1,
                                    0 /* uOperation */, 1 /* cIpis */, NULL /* pReqHdr */);
        if (RT_FAILURE(rc))
        {
            Error("SUPR3CallR0Service: %Rrc\n", rc);
            break;
        }
    } while (RTTimeNanoTS() - u64StartTS < cNanoSeconds && !g_fQuit);
}


static DECLCALLBACK(int) LoadGenIpiThreadFunction(RTTHREAD hThreadSelf, void *pvUser)
{
    LoadGenIpi(*(uint64_t *)pvUser);
    NOREF(hThreadSelf);
    return VINF_SUCCESS;
}


static int Error(const char *pszFormat, ...)
{
    va_list va;
    RTStrmPrintf(g_pStdErr, "%s: error: ", g_pszProgramName);
    va_start(va, pszFormat);
    RTStrmPrintfV(g_pStdErr, pszFormat, va);
    va_end(va);
    return 1;
}


static int SyntaxError(const char *pszFormat, ...)
{
    va_list va;
    RTStrmPrintf(g_pStdErr, "%s: syntax error: ", g_pszProgramName);
    va_start(va, pszFormat);
    RTStrmPrintfV(g_pStdErr, pszFormat, va);
    va_end(va);
    return 1;
}


int main(int argc, char **argv)
{
    static const struct LOADGENTYPE
    {
        const char     *pszName;
        int           (*pfnInit)(void);
        PFNRTTHREAD     pfnThread;
    }               s_aLoadTypes[] =
    {
        { "spin",   NULL,           LoadGenSpinThreadFunction },
        { "ipi",    LoadGenIpiInit, LoadGenIpiThreadFunction },
    };
    unsigned        iLoadType = 0;
    static RTTHREAD s_aThreads[256];
    int             rc;
    uint32_t        cThreads = 1;
    bool            fScaleByCpus = false;
    RTTHREADTYPE    enmThreadType = RTTHREADTYPE_DEFAULT;
    RTPROCPRIORITY  enmProcPriority = RTPROCPRIORITY_DEFAULT;
    uint64_t        cNanoSeconds = UINT64_MAX;

    RTR3InitExe(argc, &argv, 0);

    /*
     * Set program name.
     */
    g_pszProgramName = RTPathFilename(argv[0]);

    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--number-of-threads",    'n', RTGETOPT_REQ_UINT32 },
        { "--timeout",              't', RTGETOPT_REQ_STRING },
        { "--thread-type",          'p', RTGETOPT_REQ_STRING },
        { "--scale-by-cpus",        'c', RTGETOPT_REQ_NOTHING },
        { "--load",                 'l', RTGETOPT_REQ_STRING },
    };
    int ch;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0 /* fFlags */);
    while ((ch = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (ch)
        {
            case 'n':
                cThreads = ValueUnion.u64;
                if (cThreads == 0 || cThreads > RT_ELEMENTS(s_aThreads))
                    return SyntaxError("Requested number of threads, %RU32, is out of range (1..%d).\n",
                                       cThreads, RT_ELEMENTS(s_aThreads) - 1);
                break;

            case 't':
            {
                char *psz;
                rc = RTStrToUInt64Ex(ValueUnion.psz, &psz, 0, &cNanoSeconds);
                if (RT_FAILURE(rc))
                    return SyntaxError("Failed reading the alleged number '%s' (option '%s', rc=%Rrc).\n",
                                       ValueUnion.psz, rc);
                while (*psz == ' ' || *psz == '\t')
                    psz++;
                if (*psz)
                {
                    uint64_t u64Factor = 1;
                    if (!strcmp(psz, "ns"))
                        u64Factor = 1;
                    else if (!strcmp(psz, "ms"))
                        u64Factor = 1000;
                    else if (!strcmp(psz, "s"))
                        u64Factor = 1000000000;
                    else if (!strcmp(psz, "m"))
                        u64Factor = UINT64_C(60000000000);
                    else if (!strcmp(psz, "h"))
                        u64Factor = UINT64_C(3600000000000);
                    else
                        return SyntaxError("Unknown time suffix '%s'\n", psz);
                    uint64_t u64 = cNanoSeconds * u64Factor;
                    if (u64 < cNanoSeconds || (u64 < u64Factor && u64))
                        return SyntaxError("Time representation overflowed! (%RU64 * %RU64)\n",
                                           psz, cNanoSeconds, u64Factor);
                    cNanoSeconds = u64;
                }
                break;
            }

            case 'p':
            {
                enmProcPriority = RTPROCPRIORITY_NORMAL;

                uint32_t u32 = RTTHREADTYPE_INVALID;
                char *psz;
                rc = RTStrToUInt32Ex(ValueUnion.psz, &psz, 0, &u32);
                if (RT_FAILURE(rc) || *psz)
                {
                    if (!strcmp(ValueUnion.psz, "default"))
                    {
                        enmProcPriority = RTPROCPRIORITY_DEFAULT;
                        enmThreadType = RTTHREADTYPE_DEFAULT;
                    }
                    else if (!strcmp(ValueUnion.psz, "idle"))
                    {
                        enmProcPriority = RTPROCPRIORITY_LOW;
                        enmThreadType = RTTHREADTYPE_INFREQUENT_POLLER;
                    }
                    else if (!strcmp(ValueUnion.psz, "high"))
                    {
                        enmProcPriority = RTPROCPRIORITY_HIGH;
                        enmThreadType = RTTHREADTYPE_IO;
                    }
                    else
                        return SyntaxError("can't grok thread type '%s'\n",
                                           ValueUnion.psz);
                }
                else
                {
                    enmThreadType = (RTTHREADTYPE)u32;
                    if (enmThreadType <= RTTHREADTYPE_INVALID || enmThreadType >= RTTHREADTYPE_END)
                        return SyntaxError("thread type '%d' is out of range (%d..%d)\n",
                                           ValueUnion.psz, RTTHREADTYPE_INVALID + 1, RTTHREADTYPE_END - 1);
                }
                break;
            }

            case 'c':
                fScaleByCpus = true;
                break;

            case 'l':
            {
                for (unsigned i = 0; i < RT_ELEMENTS(s_aLoadTypes); i++)
                    if (!strcmp(s_aLoadTypes[i].pszName, ValueUnion.psz))
                    {
                        ValueUnion.psz = NULL;
                        iLoadType = i;
                        break;
                    }
                if (ValueUnion.psz)
                    return SyntaxError("Unknown load type '%s'.\n", ValueUnion.psz);
                break;
            }

            case 'h':
                RTStrmPrintf(g_pStdOut,
                             "Usage: %s [-p|--thread-type <type>] [-t|--timeout <sec|xxx[h|m|s|ms|ns]>] \\\n"
                             "       %*s [-n|--number-of-threads <threads>] [-l|--load <loadtype>]\n"
                             "\n"
                             "Load types: spin, ipi.\n"
                             ,
                             g_pszProgramName, strlen(g_pszProgramName), "");
                return 1;

            case 'V':
                RTPrintf("$Revision: 118412 $\n");
                return 0;

            case VINF_GETOPT_NOT_OPTION:
                return SyntaxError("Unknown argument #%d: '%s'\n", GetState.iNext-1, ValueUnion.psz);

            default:
                return RTGetOptPrintError(ch, &ValueUnion);
        }
    }

    /*
     * Scale thread count by host cpu count.
     */
    if (fScaleByCpus)
    {
        const unsigned cCpus = RTMpGetOnlineCount();
        if (cCpus * cThreads > RT_ELEMENTS(s_aThreads))
            return SyntaxError("Requested number of threads, %RU32, is out of range (1..%d) when scaled by %d.\n",
                               cThreads, RT_ELEMENTS(s_aThreads) - 1, cCpus);
        cThreads *= cCpus;
    }

    /*
     * Modify process and thread priority? (ignore failure)
     */
    if (enmProcPriority != RTPROCPRIORITY_DEFAULT)
        RTProcSetPriority(enmProcPriority);
    if (enmThreadType != RTTHREADTYPE_DEFAULT)
        RTThreadSetType(RTThreadSelf(), enmThreadType);

    /*
     * Load type specific init.
     */
    if (s_aLoadTypes[iLoadType].pfnInit)
    {
        rc = s_aLoadTypes[iLoadType].pfnInit();
        if (RT_FAILURE(rc))
            return 1;
    }


    /*
     * Start threads.
     */
    for (unsigned i = 1; i < cThreads; i++)
    {
        s_aThreads[i] = NIL_RTTHREAD;
        rc = RTThreadCreate(&s_aThreads[i], s_aLoadTypes[iLoadType].pfnThread,
                            &cNanoSeconds, 128*1024, enmThreadType, RTTHREADFLAGS_WAITABLE, "spinner");
        if (RT_FAILURE(rc))
        {
            ASMAtomicXchgBool(&g_fQuit, true);
            RTStrmPrintf(g_pStdErr, "%s: failed to create thread #%d, rc=%Rrc\n", g_pszProgramName, i, rc);
            while (i-- > 1)
                RTThreadWait(s_aThreads[i], 1500, NULL);
            return 1;
        }
    }

    /* our selves */
    s_aLoadTypes[iLoadType].pfnThread(RTThreadSelf(), &cNanoSeconds);

    /*
     * Wait for threads.
     */
    ASMAtomicXchgBool(&g_fQuit, true);
    for (unsigned i = 1; i < cThreads; i++)
        RTThreadWait(s_aThreads[i], 1500, NULL);

    return 0;
}


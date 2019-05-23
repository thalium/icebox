/* $Id: tstRTR0ThreadDriver.cpp $ */
/** @file
 * IPRT R0 Testcase - Kernel thread, driver program.
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
#include <iprt/initterm.h>

#include <iprt/err.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>
#ifdef VBOX
# include <VBox/sup.h>
# include "tstRTR0Thread.h"
#endif
#include "tstRTR0CommonDriver.h"


/**
 *  Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char **envp)
{
    RT_NOREF3(argc, argv, envp);
#ifndef VBOX
    RTPrintf("tstRTR0Thread: SKIPPED\n");
    return RTEXITCODE_SKIPPED;
#else
    /*
     * Init.
     */
    RTEXITCODE rcExit = RTR3TestR0CommonDriverInit("tstRTR0Thread");
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;

    RTR3TestR0SimpleTest(TSTRTR0THREAD_BASIC, "Basic");

    /*
     * Done.
     */
    return RTTestSummaryAndDestroy(g_hTest);
#endif
}


#if !defined(VBOX_WITH_HARDENING) || !defined(RT_OS_WINDOWS)
/**
 * Main entry point.
 */
int main(int argc, char **argv, char **envp)
{
    return TrustedMain(argc, argv, envp);
}
#endif


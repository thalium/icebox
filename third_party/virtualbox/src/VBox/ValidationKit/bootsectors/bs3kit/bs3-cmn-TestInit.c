/* $Id: bs3-cmn-TestInit.c $ */
/** @file
 * BS3Kit - Bs3TestInit
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
#include "bs3kit-template-header.h"
#include "bs3-cmn-test.h"


/**
 * Equivalent to RTTestCreate + RTTestBanner.
 *
 * @param   pszTest         The test name.
 */
#undef Bs3TestInit
BS3_CMN_DEF(void, Bs3TestInit,(const char BS3_FAR *pszTest))
{
    /*
     * Initialize the globals.
     */
    BS3_CMN_NM(g_pszBs3Test)    = pszTest;
    g_szBs3SubTest[0]           = '\0';
    g_cusBs3TestErrors          = 0;
    g_cusBs3SubTestAtErrors     = 0;
    g_fbBs3SubTestReported      = true;
    g_fbBs3SubTestSkipped       = false;
    g_cusBs3SubTests            = 0;
    g_cusBs3SubTestsFailed      = 0;
    g_fbBs3VMMDevTesting        = bs3TestIsVmmDevTestingPresent();

    /*
     * Print the name - RTTestBanner.
     */
    Bs3PrintStr(pszTest);
    Bs3PrintStr(": TESTING...\n");

    /*
     * Report it to the VMMDev.
     */
    bs3TestSendCmdWithStr(VMMDEV_TESTING_CMD_INIT, pszTest);
}


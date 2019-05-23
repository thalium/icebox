/* $Id: initterm-gc.cpp $ */
/** @file
 * IPRT - Init Raw-mode Context.
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
#define LOG_GROUP RTLOGGROUP_DEFAULT

#include <iprt/initterm.h>
#include <iprt/time.h>
#include <iprt/log.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include "internal/time.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Program start nanosecond TS.
 */
uint64_t    g_u64ProgramStartNanoTS;

/**
 * Program start microsecond TS.
 */
uint64_t    g_u64ProgramStartMicroTS;

/**
 * Program start millisecond TS.
 */
uint64_t    g_u64ProgramStartMilliTS;


/**
 * Initializes the raw-mode context runtime library.
 *
 * @returns iprt status code.
 *
 * @param   u64ProgramStartNanoTS  The startup timestamp.
 */
RTRCDECL(int) RTRCInit(uint64_t u64ProgramStartNanoTS)
{
    /*
     * Init the program start TSes.
     */
    g_u64ProgramStartNanoTS = u64ProgramStartNanoTS;
    g_u64ProgramStartMicroTS = u64ProgramStartNanoTS / 1000;
    g_u64ProgramStartMilliTS = u64ProgramStartNanoTS / 1000000;

    LogFlow(("RTGCInit: returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * Terminates the raw-mode context runtime library.
 */
RTRCDECL(void) RTRCTerm(void)
{
    /* do nothing */
}

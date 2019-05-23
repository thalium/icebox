/* $Id: logbackdoor-redirect.cpp $ */
/** @file
 * VirtualBox Runtime - RTLog stubs for the stripped down IPRT used by
 *                      RuntimeGuestR3Shared (X11), output is redirected
 *                      to the RTLogBackdoor API where possible.
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
#include <VBox/log.h>
#include <VBox/err.h>



/* All release logging goes to the backdoor logger anyway. */
RTDECL(PRTLOGGER) RTLogRelGetDefaultInstance(void)
{
    return NULL;
}


/* All release logging goes to the backdoor logger anyway. */
RTDECL(PRTLOGGER) RTLogRelGetDefaultInstanceEx(uint32_t fFlagsAndGroup)
{
    return NULL;
}


/* All logging goes to the backdoor logger anyway. */
RTDECL(PRTLOGGER) RTLogDefaultInstance(void)
{
    return NULL;
}


/* All logging goes to the backdoor logger anyway. */
RTDECL(PRTLOGGER) RTLogDefaultInstanceEx(uint32_t fFlagsAndGroup)
{
    return NULL;
}


/* All logging goes to the backdoor logger anyway. */
RTDECL(PRTLOGGER) RTLogRelSetDefaultInstance(PRTLOGGER pLogger)
{
    return NULL;
}


RTDECL(void) RTLogRelPrintf(const char *pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    RTLogBackdoorPrintfV(pszFormat, va);
    va_end(va);
}


RTDECL(void) RTLogRelPrintfV(const char *pszFormat, va_list va)
{
    RTLogBackdoorPrintfV(pszFormat, va);
}


RTDECL(void) RTLogLoggerEx(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup, const char *pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    RTLogBackdoorPrintfV(pszFormat, va);
    va_end(va);
}


RTDECL(void) RTLogPrintf(const char *pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    RTLogBackdoorPrintfV(pszFormat, va);
    va_end(va);
}


RTDECL(void) RTLogPrintfV(const char *pszFormat, va_list va)
{
    RTLogBackdoorPrintfV(pszFormat, va);
}


/* Do nothing for now. */
RTDECL(void) RTLogFlush(PRTLOGGER pLogger)
{
    NOREF(pLogger);
}

/* Do nothing. */
RTDECL(int) RTLogCreate(PRTLOGGER *ppLogger, RTUINT fFlags, const char *pszGroupSettings,
                        const char *pszEnvVarBase, unsigned cGroups, const char * const * papszGroups,
                        RTUINT fDestFlags, const char *pszFilenameFmt, ...)
{
    return VERR_NOT_IMPLEMENTED;
}


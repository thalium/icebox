/* $Id: SUPSvcInternal.h $ */
/** @file
 * VirtualBox Support Service - Internal header.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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

#ifndef ___SUPSvcInternal_h___
#define ___SUPSvcInternal_h___

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <iprt/stdarg.h>
#include <iprt/getopt.h>

RT_C_DECLS_BEGIN

/** @name Common Helpers
 * @{ */
void supSvcLogErrorStr(const char *pszMsg);
void supSvcLogErrorV(const char *pszFormat, va_list va);
void supSvcLogError(const char *pszFormat, ...);
int  supSvcLogGetOptError(const char *pszAction, int rc, int argc, char **argv, int iArg, PCRTOPTIONUNION pValue);
int  supSvcLogTooManyArgsError(const char *pszAction, int argc, char **argv, int iArg);
void supSvcDisplayErrorV(const char *pszFormat, va_list va);
void supSvcDisplayError(const char *pszFormat, ...);
int  supSvcDisplayGetOptError(const char *pszAction, int rc, int argc, char **argv, int iArg, PCRTOPTIONUNION pValue);
int  supSvcDisplayTooManyArgsError(const char *pszAction, int argc, char **argv, int iArg);
/** @} */


/** @name OS Backend
 * @{ */
/**
 * Logs the message to the appropriate system log.
 *
 * @param   pszMsg      The log string.
 */
void supSvcOsLogErrorStr(const char *pszMsg);
/** @} */


/** @name The Service Manager
 * @{ */
void supSvcStopAndDestroyServices(void);
int  supSvcTryStopServices(void);
int  supSvcCreateAndStartServices(void);
/** @} */


/** @name The Grant Service
 * @{ */
#define SUPSVC_GRANT_SERVICE_NAME   "VirtualBoxGrantSvc"
DECLCALLBACK(int)  supSvcGrantCreate(void **ppvInstance);
DECLCALLBACK(void) supSvcGrantStart(void *pvInstance);
DECLCALLBACK(int)  supSvcGrantTryStop(void *pvInstance);
DECLCALLBACK(void) supSvcGrantStopAndDestroy(void *pvInstance, bool fRunning);
/** @} */


/** @name The Global Service
 * @{ */
DECLCALLBACK(int)  supSvcGlobalCreate(void **ppvInstance);
DECLCALLBACK(void) supSvcGlobalStart(void *pvInstance);
DECLCALLBACK(int)  supSvcGlobalTryStop(void *pvInstance);
DECLCALLBACK(void) supSvcGlobalStopAndDestroy(void *pvInstance, bool fRunning);
/** @} */

RT_C_DECLS_END

#endif


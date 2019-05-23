/* $Id: RTEnvDupEx-generic.cpp $ */
/** @file
 * IPRT - Environment, RTEnvDupEx, generic.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include <iprt/env.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/mem.h>



RTDECL(char *) RTEnvDupEx(RTENV Env, const char *pszVar)
{
    /*
     * Try with a small buffer.  This helps avoid allocating a heap buffer for
     * variables that doesn't exist.
     */
    char szSmall[256];
    int rc = RTEnvGetEx(Env, pszVar, szSmall, sizeof(szSmall), NULL);
    if (RT_SUCCESS(rc))
        return RTStrDup(szSmall);
    if (rc != VERR_BUFFER_OVERFLOW)
        return NULL;

    /*
     * It's a bug bugger.
     */
    size_t  cbBuf  = _1K;
    char   *pszBuf = (char *)RTMemAlloc(cbBuf);
    for (;;)
    {
        rc = RTEnvGetEx(Env, pszVar, pszBuf, cbBuf, NULL);
        if (RT_SUCCESS(rc))
            return pszBuf; /* ASSUMES RTMemAlloc can be freed by RTStrFree! */

        if (rc != VERR_BUFFER_OVERFLOW)
            break;

        if (cbBuf >= 64 * _1M)
            break;
        cbBuf *= 2;

        char *pszNew = (char *)RTMemRealloc(pszBuf, cbBuf);
        if (!pszNew)
            break;
        pszBuf = pszNew;
    }

    RTMemFree(pszBuf);
    return NULL;
}
RT_EXPORT_SYMBOL(RTEnvGetExecEnvP);


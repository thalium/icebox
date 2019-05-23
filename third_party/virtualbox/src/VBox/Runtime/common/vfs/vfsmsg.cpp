/* $Id: vfsmsg.cpp $ */
/** @file
 * IPRT - Virtual File System, Error Messaging for Chains.
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
#include <iprt/vfs.h>

#include <iprt/err.h>
#include <iprt/message.h>


RTDECL(void) RTVfsChainMsgError(const char *pszFunction, const char *pszSpec, int rc, uint32_t offError, PRTERRINFO pErrInfo)
{
    if (RTErrInfoIsSet(pErrInfo))
    {
        if (offError > 0)
            RTMsgError("%s failed with rc=%Rrc: %s\n"
                       "    '%s'\n"
                       "     %*s^\n",
                       pszFunction, rc, pErrInfo->pszMsg, pszSpec, offError, "");
        else
            RTMsgError("%s failed to open '%s': %Rrc: %s\n", pszFunction, pszSpec, rc, pErrInfo->pszMsg);
    }
    else
    {
        if (offError > 0)
            RTMsgError("%s failed with rc=%Rrc:\n"
                       "    '%s'\n"
                       "     %*s^\n",
                       pszFunction, rc, pszSpec, offError, "");
        else
            RTMsgError("%s failed to open '%s': %Rrc\n", pszFunction, pszSpec, rc);
    }
}


RTDECL(RTEXITCODE) RTVfsChainMsgErrorExitFailure(const char *pszFunction, const char *pszSpec,
                                                 int rc, uint32_t offError, PRTERRINFO pErrInfo)
{
    RTVfsChainMsgError(pszFunction, pszSpec, rc, offError, pErrInfo);
    return RTEXITCODE_FAILURE;
}


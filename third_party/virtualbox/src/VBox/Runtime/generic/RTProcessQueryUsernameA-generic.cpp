/* $Id: RTProcessQueryUsernameA-generic.cpp $ */
/** @file
 * IPRT - RTSystemQueryOSInfo, generic stub.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/system.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/process.h>


RTR3DECL(int)   RTProcQueryUsernameA(RTPROCESS hProcess, char **ppszUser)
{
    /*
     * Validation.
     */
    AssertPtrReturn(ppszUser, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    size_t cbUser = 0;

    rc = RTProcQueryUsername(hProcess, NULL, cbUser, &cbUser);
    if (rc == VERR_BUFFER_OVERFLOW)
    {
        char *pszUser = (char *)RTStrAlloc(cbUser);
        if (pszUser)
        {
            rc = RTProcQueryUsername(hProcess, pszUser, cbUser, NULL);
            Assert(rc != VERR_BUFFER_OVERFLOW);
            if (RT_SUCCESS(rc))
                *ppszUser = pszUser;
            else
                RTStrFree(pszUser);
        }
        else
            rc = VERR_NO_STR_MEMORY;
    }

    return rc;
}
RT_EXPORT_SYMBOL(RTProcQueryUsernameA);


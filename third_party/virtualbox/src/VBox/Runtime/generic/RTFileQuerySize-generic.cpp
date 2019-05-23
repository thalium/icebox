/* $Id: RTFileQuerySize-generic.cpp $ */
/** @file
 * IPRT - RTFileQuerySize, generic implementation.
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
#define LOG_GROUP RTLOGGROUP_FILE
#include "internal/iprt.h"

#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/path.h>


RTDECL(int) RTFileQuerySize(const char *pszPath, uint64_t *pcbFile)
{
    RTFSOBJINFO ObjInfo;
    int rc = RTPathQueryInfoEx(pszPath, &ObjInfo, RTFSOBJATTRADD_NOTHING, RTPATH_F_FOLLOW_LINK);
    if (RT_SUCCESS(rc))
    {
        if (RTFS_IS_FILE(ObjInfo.Attr.fMode))
        {
            *pcbFile = ObjInfo.cbObject;
            LogFlow(("RTFileQuerySize(%p:{%s}): returns %Rrc (%#RX64)\n", pszPath, pszPath, rc, *pcbFile));
            return rc;
        }

        if (RTFS_IS_DIRECTORY(ObjInfo.Attr.fMode))
            rc = VERR_IS_A_DIRECTORY;
        else
            rc = VERR_FILE_NOT_FOUND; /** @todo VERR_NOT_A_FILE... */
    }
    LogFlow(("RTFileQuerySize(%p:{%s}): returns %Rrc\n", pszPath, pszPath, rc));
    return rc;
}

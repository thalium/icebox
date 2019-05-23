/* $Id: RTSystemQueryDmiString-linux.cpp $ */
/** @file
 * IPRT - RTSystemQueryDmiString, linux ring-3.
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
#include <iprt/system.h>
#include "internal/iprt.h"

#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/linux/sysfs.h>

#include <errno.h>


RTDECL(int) RTSystemQueryDmiString(RTSYSDMISTR enmString, char *pszBuf, size_t cbBuf)
{
    AssertPtrReturn(pszBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf > 0, VERR_INVALID_PARAMETER);
    *pszBuf = '\0';
    AssertReturn(enmString > RTSYSDMISTR_INVALID && enmString < RTSYSDMISTR_END, VERR_INVALID_PARAMETER);

    const char *pszSysFsName;
    switch (enmString)
    {
        case RTSYSDMISTR_PRODUCT_NAME:      pszSysFsName = "id/product_name"; break;
        case RTSYSDMISTR_PRODUCT_VERSION:   pszSysFsName = "id/product_version"; break;
        case RTSYSDMISTR_PRODUCT_UUID:      pszSysFsName = "id/product_uuid"; break;
        case RTSYSDMISTR_PRODUCT_SERIAL:    pszSysFsName = "id/product_serial"; break;
        case RTSYSDMISTR_MANUFACTURER:      pszSysFsName = "id/sys_vendor"; break;
        default:
            return VERR_NOT_SUPPORTED;
    }

    size_t cbRead = 0;
    int rc = RTLinuxSysFsReadStrFile(pszBuf, cbBuf, &cbRead, "devices/virtual/dmi/%s", pszSysFsName);
    if (RT_FAILURE(rc) && rc != VERR_BUFFER_OVERFLOW)
        rc = RTLinuxSysFsReadStrFile(pszBuf, cbBuf, &cbRead, "class/dmi/%s", pszSysFsName);
    if (RT_FAILURE(rc) && rc != VERR_BUFFER_OVERFLOW)
    {
        switch (rc)
        {
            case VINF_SUCCESS:
                AssertFailed();
                break;
            case VERR_FILE_NOT_FOUND:
            case VERR_PATH_NOT_FOUND:
            case VERR_IS_A_DIRECTORY:
                rc = VERR_NOT_SUPPORTED;
                break;
            case VERR_PERMISSION_DENIED:
            case VERR_ACCESS_DENIED:
                rc = VERR_ACCESS_DENIED;
                break;
        }
    }

    return rc;
}
RT_EXPORT_SYMBOL(RTSystemQueryDmiString);


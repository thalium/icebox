/* $Id: VBoxGuestR3LibLog.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Logging.
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
#include <iprt/mem.h>
#include <iprt/string.h>
#include "VBoxGuestR3LibInternal.h"


/**
 * Write to the backdoor logger from ring 3 guest code.
 *
 * @returns IPRT status code.
 *
 * @param   pch     The string to log.  Does not need to be terminated.
 * @param   cch     The number of chars (bytes) to log.
 *
 * @remarks This currently does not accept more than 255 bytes of data at
 *          one time. It should probably be rewritten to use pass a pointer
 *          in the IOCtl.
 */
VBGLR3DECL(int) VbglR3WriteLog(const char *pch, size_t cch)
{
    /*
     * Quietly skip empty strings.
     * (Happens in the RTLogBackdoorPrintf case.)
     */
    int rc;
    if (cch > 0)
    {
        if (RT_VALID_PTR(pch))
        {
            /*
             * We need to repackage the string for ring-0.
             */
            size_t      cbMsg = VBGL_IOCTL_LOG_SIZE(cch);
            PVBGLIOCLOG pMsg  = (PVBGLIOCLOG)RTMemTmpAlloc(cbMsg);
            if (pMsg)
            {
                VBGLREQHDR_INIT_EX(&pMsg->Hdr, VBGL_IOCTL_LOG_SIZE_IN(cch), VBGL_IOCTL_LOG_SIZE_OUT);
                memcpy(pMsg->u.In.szMsg, pch, cch);
                pMsg->u.In.szMsg[cch] = '\0';
                rc = vbglR3DoIOCtl(VBGL_IOCTL_LOG(cch), &pMsg->Hdr, cbMsg);

                RTMemTmpFree(pMsg);
            }
            else
                rc = VERR_NO_TMP_MEMORY;
        }
        else
            rc = VERR_INVALID_POINTER;
    }
    else
        rc = VINF_SUCCESS;
    return rc;
}


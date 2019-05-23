/* $Id: RTTimerCreate-generic.cpp $ */
/** @file
 * IPRT - Timers, Generic RTTimerCreate() Implementation.
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
#include <iprt/timer.h>
#include "internal/iprt.h"

#include <iprt/err.h>
#include <iprt/assert.h>


RTDECL(int) RTTimerCreate(PRTTIMER *ppTimer, unsigned uMilliesInterval, PFNRTTIMER pfnTimer, void *pvUser)
{
    int rc = RTTimerCreateEx(ppTimer, uMilliesInterval * RT_NS_1MS_64, 0 /* fFlags */, pfnTimer, pvUser);
    if (RT_SUCCESS(rc))
    {
        rc = RTTimerStart(*ppTimer, 0 /* u64First */);
        if (RT_FAILURE(rc))
        {
            int rc2 = RTTimerDestroy(*ppTimer); AssertRC(rc2);
            *ppTimer = NULL;
        }
    }
    return rc;
}
RT_EXPORT_SYMBOL(RTTimerCreate);


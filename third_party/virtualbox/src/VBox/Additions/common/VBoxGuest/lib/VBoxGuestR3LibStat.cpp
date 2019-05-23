/* $Id: VBoxGuestR3LibStat.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Statistics.
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
#include "VBoxGuestR3LibInternal.h"


/**
 * Query the current statistics update interval.
 *
 * @returns IPRT status code.
 * @param   pcMsInterval    Update interval in ms (out).
 */
VBGLR3DECL(int) VbglR3StatQueryInterval(PRTMSINTERVAL pcMsInterval)
{
    VMMDevGetStatisticsChangeRequest Req;

    vmmdevInitRequest(&Req.header, VMMDevReq_GetStatisticsChangeRequest);
    Req.eventAck = VMMDEV_EVENT_STATISTICS_INTERVAL_CHANGE_REQUEST;
    Req.u32StatInterval = 1;
    int rc = vbglR3GRPerform(&Req.header);
    if (RT_SUCCESS(rc))
    {
        *pcMsInterval = Req.u32StatInterval * 1000;
        if (*pcMsInterval / 1000 != Req.u32StatInterval)
            *pcMsInterval = ~(RTMSINTERVAL)0;
    }
    return rc;
}


/**
 * Report guest statistics.
 *
 * @returns IPRT status code.
 * @param   pReq        Request packet with statistics.
 */
VBGLR3DECL(int) VbglR3StatReport(VMMDevReportGuestStats *pReq)
{
    vmmdevInitRequest(&pReq->header, VMMDevReq_ReportGuestStats);
    return vbglR3GRPerform(&pReq->header);
}


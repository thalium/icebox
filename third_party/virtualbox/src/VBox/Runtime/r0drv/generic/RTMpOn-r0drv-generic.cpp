/* $Id: RTMpOn-r0drv-generic.cpp $ */
/** @file
 * IPRT - Multiprocessor, Ring-0 Driver, Generic Stubs.
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
#include <iprt/mp.h>
#include "internal/iprt.h"

#include <iprt/err.h>


RTDECL(int) RTMpOnAll(PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2)
{
    NOREF(pfnWorker);
    NOREF(pvUser1);
    NOREF(pvUser2);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTMpOnAll);


RTDECL(bool) RTMpOnAllIsConcurrentSafe(void)
{
    return false;
}
RT_EXPORT_SYMBOL(RTMpOnAllIsConcurrentSafe);


RTDECL(int) RTMpOnOthers(PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2)
{
    NOREF(pfnWorker);
    NOREF(pvUser1);
    NOREF(pvUser2);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTMpOnOthers);


RTDECL(int) RTMpOnSpecific(RTCPUID idCpu, PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2)
{
    NOREF(idCpu);
    NOREF(pfnWorker);
    NOREF(pvUser1);
    NOREF(pvUser2);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTMpOnSpecific);


RTDECL(int) RTMpOnPair(RTCPUID idCpu1, RTCPUID idCpu2, PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2)
{
    NOREF(idCpu1);
    NOREF(idCpu2);
    NOREF(pfnWorker);
    NOREF(pvUser1);
    NOREF(pvUser2);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTMpOnPair);



RTDECL(bool) RTMpOnPairIsConcurrentExecSupported(void)
{
    return false;
}
RT_EXPORT_SYMBOL(RTMpOnPairIsConcurrentExecSupported);


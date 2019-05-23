/* $Id: mp-os2.cpp $ */
/** @file
 * IPRT - Multiprocessor, OS/2.
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
#define INCL_BASE
#define INCL_ERRORS
#include <os2.h>
#undef RT_MAX

#include <iprt/mp.h>
#include <iprt/cpuset.h>
#include <iprt/assert.h>

/** @todo RTMpCpuId() */

RTDECL(int) RTMpCpuIdToSetIndex(RTCPUID idCpu)
{
    return idCpu < RTCPUSET_MAX_CPUS ? (int) idCpu : -1;
}


RTDECL(RTCPUID) RTMpCpuIdFromSetIndex(int iCpu)
{
    return (unsigned)iCpu < RTCPUSET_MAX_CPUS ? iCpu : NIL_RTCPUID;
}


RTDECL(RTCPUID) RTMpGetMaxCpuId(void)
{
    return RTCPUSET_MAX_CPUS;
}


RTDECL(bool) RTMpIsCpuPossible(RTCPUID idCpu)
{
    RTCPUSET Set;
    return RTCpuSetIsMember(RTMpGetSet(&Set), idCpu);
}


RTDECL(PRTCPUSET) RTMpGetSet(PRTCPUSET pSet)
{
    RTCPUID idCpu = RTMpGetCount();
    RTCpuSetEmpty(pSet);
    while (idCpu-- > 0)
        RTCpuSetAdd(pSet, idCpu);
    return pSet;
}


RTDECL(RTCPUID) RTMpGetCount(void)
{
    ULONG cCpus = 1;
    int rc = DosQuerySysInfo(QSV_NUMPROCESSORS, QSV_NUMPROCESSORS, &cCpus, sizeof(cCpus));
    if (rc || !cCpus)
        cCpus = 1;
    return cCpus;
}


RTDECL(bool) RTMpIsCpuOnline(RTCPUID idCpu)
{
    RTCPUSET Set;
    return RTCpuSetIsMember(RTMpGetOnlineSet(&Set), idCpu);
}


RTDECL(PRTCPUSET) RTMpGetOnlineSet(PRTCPUSET pSet)
{
    union
    {
        uint64_t u64;
        MPAFFINITY mpaff;
    } u;

    int rc = DosQueryThreadAffinity(AFNTY_SYSTEM, &u.mpaff);
    if (rc)
        u.u64 = 1;
    return RTCpuSetFromU64(pSet, u.u64);
}


RTDECL(RTCPUID) RTMpGetOnlineCount(void)
{
    RTCPUSET Set;
    RTMpGetOnlineSet(&Set);
    return RTCpuSetCount(&Set);
}


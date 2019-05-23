/* $Id: RTMpGetArraySize-generic.cpp $ */
/** @file
 * IPRT - Multiprocessor, Generic RTMpGetArraySize.
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
#include <iprt/mp.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/cpuset.h>


RTDECL(uint32_t) RTMpGetArraySize(void)
{
    /*
     * Cache the result here.  This whole point of this function is that it
     * will always return the same value, so that should be safe.
     *
     * Note! Because RTCPUSET may be to small to represent all the CPUs, we
     *       check with RTMpGetCount() as well.
     */
    static uint32_t s_cMaxCpus = 0;
    uint32_t cCpus = s_cMaxCpus;
    if (RT_UNLIKELY(cCpus == 0))
    {
        RTCPUSET    CpuSet;
        uint32_t    cCpus1 = RTCpuLastIndex(RTMpGetSet(&CpuSet)) + 1;
        uint32_t    cCpus2 = RTMpGetCount();
        cCpus              = RT_MAX(cCpus1, cCpus2);
        ASMAtomicCmpXchgU32(&s_cMaxCpus, cCpus, 0);
        return cCpus;
    }
    return s_cMaxCpus;

}
RT_EXPORT_SYMBOL(RTMpGetArraySize);


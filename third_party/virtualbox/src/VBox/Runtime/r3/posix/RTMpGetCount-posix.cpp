/* $Id: RTMpGetCount-posix.cpp $ */
/** @file
 * IPRT - RTMpGetCount, POSIX.
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
#include <iprt/mp.h>
#include <iprt/assert.h>

#include <unistd.h>
#if !defined(RT_OS_SOLARIS)
# include <sys/sysctl.h>
#endif


RTDECL(RTCPUID) RTMpGetCount(void)
{
    /*
     * The sysconf way (linux and others).
     */
#if defined(_SC_NPROCESSORS_MAX) || defined(_SC_NPROCESSORS_CONF) || defined(_SC_NPROCESSORS_ONLN)
    int cCpusSC = -1;
# ifdef _SC_NPROCESSORS_MAX
    int cMax = sysconf(_SC_NPROCESSORS_MAX);
    cCpusSC = RT_MAX(cCpusSC, cMax);
# endif
# ifdef _SC_NPROCESSORS_CONF
    int cConf = sysconf(_SC_NPROCESSORS_CONF);
    cCpusSC = RT_MAX(cCpusSC, cConf);
# endif
# ifdef _SC_NPROCESSORS_ONLN
    int cOnln = sysconf(_SC_NPROCESSORS_ONLN);
    cCpusSC = RT_MAX(cCpusSC, cOnln);
# endif
    Assert(cCpusSC > 0);
    if (cCpusSC > 0)
        return cCpusSC;
#endif

    /*
     * The BSD 4.4 way.
     */
#if defined(CTL_HW) && defined(HW_NCPU)
    int aiMib[2];
    aiMib[0] = CTL_HW;
    aiMib[1] = HW_NCPU;
    int cCpus = -1;
    size_t cb = sizeof(cCpus);
    int rc = sysctl(aiMib, RT_ELEMENTS(aiMib), &cCpus, &cb, NULL, 0);
    if (rc != -1 && cCpus >= 1)
        return cCpus;
#endif
    return 1;
}


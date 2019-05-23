/* $Id: systemmem-darwin.cpp $ */
/** @file
 * IPRT - RTSystemQueryTotalRam, darwin ring-3.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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

#include <errno.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <mach/mach.h>



RTDECL(int) RTSystemQueryTotalRam(uint64_t *pcb)
{
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);

    int aiMib[2];
    aiMib[0] = CTL_HW;
    aiMib[1] = HW_MEMSIZE;
    uint64_t    cbPhysMem = UINT64_MAX;
    size_t      cb = sizeof(cbPhysMem);
    int rc = sysctl(aiMib, RT_ELEMENTS(aiMib), &cbPhysMem, &cb, NULL, 0);
    if (rc == 0)
    {
        *pcb = cbPhysMem;
        return VINF_SUCCESS;
    }
    return RTErrConvertFromErrno(errno);
}


RTDECL(int) RTSystemQueryAvailableRam(uint64_t *pcb)
{
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);

    static mach_port_t volatile s_hSelfPort = 0;
    mach_port_t hSelfPort = s_hSelfPort;
    if (hSelfPort == 0)
        s_hSelfPort = hSelfPort = mach_host_self();

    vm_statistics_data_t    VmStats;
    mach_msg_type_number_t  cItems = sizeof(VmStats) / sizeof(natural_t);

    kern_return_t krc = host_statistics(hSelfPort, HOST_VM_INFO, (host_info_t)&VmStats, &cItems);
    if (krc == KERN_SUCCESS)
    {
        uint64_t cPages = VmStats.inactive_count;
        cPages += VmStats.free_count;
        *pcb = cPages * PAGE_SIZE;
        return VINF_SUCCESS;
    }
    return RTErrConvertFromDarwin(krc);
}


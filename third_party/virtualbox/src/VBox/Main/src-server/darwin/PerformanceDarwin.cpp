/* $Id: PerformanceDarwin.cpp $ */
/** @file
 * VBox Darwin-specific Performance Classes implementation.
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
 */

#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/mach_time.h>
#include <mach/vm_statistics.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/mp.h>
#include <iprt/param.h>
#include <iprt/system.h>
#include "Performance.h"

/* The following declarations are missing in 10.4.x SDK */
/** @todo Replace them with libproc.h and sys/proc_info.h when 10.4 is no longer supported */
extern "C" int proc_pidinfo(int pid, int flavor, uint64_t arg,  void *buffer, int buffersize);
struct proc_taskinfo {
    uint64_t    pti_virtual_size;       /* virtual memory size (bytes) */
    uint64_t    pti_resident_size;      /* resident memory size (bytes) */
    uint64_t    pti_total_user;         /* total time */
    uint64_t    pti_total_system;
    uint64_t    pti_threads_user;       /* existing threads only */
    uint64_t    pti_threads_system;
    int32_t     pti_policy;             /* default policy for new threads */
    int32_t     pti_faults;             /* number of page faults */
    int32_t     pti_pageins;            /* number of actual pageins */
    int32_t     pti_cow_faults;         /* number of copy-on-write faults */
    int32_t     pti_messages_sent;      /* number of messages sent */
    int32_t     pti_messages_received;  /* number of messages received */
    int32_t     pti_syscalls_mach;      /* number of mach system calls */
    int32_t     pti_syscalls_unix;      /* number of unix system calls */
    int32_t     pti_csw;                /* number of context switches */
    int32_t     pti_threadnum;          /* number of threads in the task */
    int32_t     pti_numrunning;         /* number of running threads */
    int32_t     pti_priority;           /* task priority*/
};
#define PROC_PIDTASKINFO 4

namespace pm {

class CollectorDarwin : public CollectorHAL
{
public:
    CollectorDarwin();
    virtual int getRawHostCpuLoad(uint64_t *user, uint64_t *kernel, uint64_t *idle);
    virtual int getHostMemoryUsage(ULONG *total, ULONG *used, ULONG *available);
    virtual int getRawProcessCpuLoad(RTPROCESS process, uint64_t *user, uint64_t *kernel, uint64_t *total);
    virtual int getProcessMemoryUsage(RTPROCESS process, ULONG *used);
private:
    ULONG    totalRAM;
    uint32_t nCpus;
};

CollectorHAL *createHAL()
{
    return new CollectorDarwin();
}

CollectorDarwin::CollectorDarwin()
{
    uint64_t cb;
    int rc = RTSystemQueryTotalRam(&cb);
    if (RT_FAILURE(rc))
        totalRAM = 0;
    else
        totalRAM = (ULONG)(cb / 1024);
    nCpus = RTMpGetOnlineCount();
    Assert(nCpus);
    if (nCpus == 0)
    {
        /* It is rather unsual to have no CPUs, but the show must go on. */
        nCpus = 1;
    }
}

int CollectorDarwin::getRawHostCpuLoad(uint64_t *user, uint64_t *kernel, uint64_t *idle)
{
    kern_return_t krc;
    mach_msg_type_number_t count;
    host_cpu_load_info_data_t info;

    count = HOST_CPU_LOAD_INFO_COUNT;

    krc = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&info, &count);
    if (krc != KERN_SUCCESS)
    {
        Log(("host_statistics() -> %s", mach_error_string(krc)));
        return RTErrConvertFromDarwinKern(krc);
    }

    *user = (uint64_t)info.cpu_ticks[CPU_STATE_USER]
                    + info.cpu_ticks[CPU_STATE_NICE];
    *kernel = (uint64_t)info.cpu_ticks[CPU_STATE_SYSTEM];
    *idle = (uint64_t)info.cpu_ticks[CPU_STATE_IDLE];
    return VINF_SUCCESS;
}

int CollectorDarwin::getHostMemoryUsage(ULONG *total, ULONG *used, ULONG *available)
{
    AssertReturn(totalRAM, VERR_INTERNAL_ERROR);
    uint64_t cb;
    int rc = RTSystemQueryAvailableRam(&cb);
    if (RT_SUCCESS(rc))
    {
        *total = totalRAM;
        *available = cb / 1024;
        *used = *total - *available;
    }
    return rc;
}

static int getProcessInfo(RTPROCESS process, struct proc_taskinfo *tinfo)
{
    Log7(("getProcessInfo() getting info for %d", process));
    int nb = proc_pidinfo(process, PROC_PIDTASKINFO, 0,  tinfo, sizeof(*tinfo));
    if (nb <= 0)
    {
        int rc = errno;
        Log(("proc_pidinfo() -> %s", strerror(rc)));
        return RTErrConvertFromDarwin(rc);
    }
    else if ((unsigned int)nb < sizeof(*tinfo))
    {
        Log(("proc_pidinfo() -> too few bytes %d", nb));
        return VERR_INTERNAL_ERROR;
    }
    return VINF_SUCCESS;
}

int CollectorDarwin::getRawProcessCpuLoad(RTPROCESS process, uint64_t *user, uint64_t *kernel, uint64_t *total)
{
    struct proc_taskinfo tinfo;

    int rc = getProcessInfo(process, &tinfo);
    if (RT_SUCCESS(rc))
    {
        /*
         * Adjust user and kernel values so 100% is when ALL cores are fully
         * utilized (see @bugref{6345}).
         */
        *user = tinfo.pti_total_user / nCpus;
        *kernel = tinfo.pti_total_system / nCpus;
        *total = mach_absolute_time();
    }
    return rc;
}

int CollectorDarwin::getProcessMemoryUsage(RTPROCESS process, ULONG *used)
{
    struct proc_taskinfo tinfo;

    int rc = getProcessInfo(process, &tinfo);
    if (RT_SUCCESS(rc))
    {
        *used = tinfo.pti_resident_size / 1024;
    }
    return rc;
}

}


/* $Id: PerformanceOs2.cpp $ */

/** @file
 *
 * VBox OS/2-specific Performance Classes implementation.
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

#include "Performance.h"

namespace pm {

class CollectorOS2 : public CollectorHAL
{
public:
    virtual int getHostCpuLoad(ULONG *user, ULONG *kernel, ULONG *idle);
    virtual int getHostCpuMHz(ULONG *mhz);
    virtual int getHostMemoryUsage(ULONG *total, ULONG *used, ULONG *available);
    virtual int getProcessCpuLoad(RTPROCESS process, ULONG *user, ULONG *kernel);
    virtual int getProcessMemoryUsage(RTPROCESS process, ULONG *used);
};


CollectorHAL *createHAL()
{
    return new CollectorOS2();
}

int CollectorOS2::getHostCpuLoad(ULONG *user, ULONG *kernel, ULONG *idle)
{
    return VERR_NOT_IMPLEMENTED;
}

int CollectorOS2::getHostCpuMHz(ULONG *mhz)
{
    return VERR_NOT_IMPLEMENTED;
}

int CollectorOS2::getHostMemoryUsage(ULONG *total, ULONG *used, ULONG *available)
{
    return VERR_NOT_IMPLEMENTED;
}

int CollectorOS2::getProcessCpuLoad(RTPROCESS process, ULONG *user, ULONG *kernel)
{
    return VERR_NOT_IMPLEMENTED;
}

int CollectorOS2::getProcessMemoryUsage(RTPROCESS process, ULONG *used)
{
    return VERR_NOT_IMPLEMENTED;
}

}

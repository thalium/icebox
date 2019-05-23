/* $Id: VBoxCpuReportMsrSup.cpp $ */
/** @file
 * MsrSup - SupDrv-specific MSR access.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "VBoxCpuReport.h"
#include <iprt/x86.h>


int VbCpuRepMsrProberInitSupDrv(PVBCPUREPMSRACCESSORS pMsrFunctions)
{
    int rc = SUPR3Init(NULL);
    if (RT_SUCCESS(rc))
    {
        /* Test if the MSR prober is available, since the interface is optional. The TSC MSR will exist on any supported CPU. */
        uint64_t uValue;
        bool     fGp;
        rc = SUPR3MsrProberRead(MSR_IA32_TSC, NIL_RTCPUID, &uValue, &fGp);
        if (   rc != VERR_NOT_IMPLEMENTED
            && rc != VERR_INVALID_FUNCTION)
        {
            pMsrFunctions->fAtomic            = true;
            pMsrFunctions->pfnMsrProberRead   = SUPR3MsrProberRead;
            pMsrFunctions->pfnMsrProberWrite  = SUPR3MsrProberWrite;
            pMsrFunctions->pfnMsrProberModify = SUPR3MsrProberModify;

            pMsrFunctions->pfnTerm            = NULL;
            return VINF_SUCCESS;

        }
        vbCpuRepDebug("warning: MSR probing not supported by the support driver (%Rrc).\n", rc);
    }
    else
        vbCpuRepDebug("warning: Unable to initialize the support library (%Rrc).\n", rc);
    return rc;
}


/* $Id: loadgeneratorR0.cpp $ */
/** @file
 * Load Generator, Ring-0 Service.
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
#include <VBox/sup.h>
#include <VBox/err.h>




/**
 * Worker for loadgenR0Ipi.
 */
static DECLCALLBACK(void) loadgenR0IpiWorker(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    NOREF(idCpu);
    NOREF(pvUser1);
    NOREF(pvUser2);
}


/**
 * Generate broadcast inter processor interrupts (IPI), aka cross calls.
 *
 * @returns VBox status code.
 * @param   cIpis       The number of IPIs to do.
 */
static int loadgenR0Ipi(uint64_t cIpis)
{
    if (cIpis > _1G || !cIpis)
        return VERR_INVALID_PARAMETER;

    while (cIpis-- > 0)
    {
        int rc = RTMpOnAll(loadgenR0IpiWorker, NULL, NULL);
        if (RT_FAILURE(rc))
            return rc;
    }
    return VINF_SUCCESS;
}


/**
 * Service request handler entry point.
 *
 * @copydoc FNSUPR0SERVICEREQHANDLER
 */
extern "C" DECLEXPORT(int) LoadGenR0ServiceReqHandler(PSUPDRVSESSION pSession, uint32_t uOperation,
                                                      uint64_t u64Arg, PSUPR0SERVICEREQHDR pReqHdr)

{
    switch (uOperation)
    {
        case 0:
            if (pReqHdr)
                return VERR_INVALID_PARAMETER;
            return loadgenR0Ipi(u64Arg);

        default:
            NOREF(pSession);
            return VERR_NOT_SUPPORTED;
    }
}


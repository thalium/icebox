/* $Id: PDMR0Driver.cpp $ */
/** @file
 * PDM - Pluggable Device and Driver Manager, R0 Driver parts.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_PDM_DRIVER
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/gvmm.h>

#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>



/**
 * PDMDrvHlpCallR0 helper.
 *
 * @returns See PFNPDMDRVREQHANDLERR0.
 * @param   pGVM    The global (ring-0) VM structure. (For validation.)
 * @param   pVM     The cross context VM structure. (For validation.)
 * @param   pReq    Pointer to the request buffer.
 */
VMMR0_INT_DECL(int) PDMR0DriverCallReqHandler(PGVM pGVM, PVM pVM, PPDMDRIVERCALLREQHANDLERREQ pReq)
{
    /*
     * Validate input and make the call.
     */
    int rc = GVMMR0ValidateGVMandVM(pGVM, pVM);
    if (RT_SUCCESS(rc))
    {
        AssertPtrReturn(pReq, VERR_INVALID_POINTER);
        AssertMsgReturn(pReq->Hdr.cbReq == sizeof(*pReq), ("%#x != %#x\n", pReq->Hdr.cbReq, sizeof(*pReq)), VERR_INVALID_PARAMETER);

        PPDMDRVINS pDrvIns = pReq->pDrvInsR0;
        AssertPtrReturn(pDrvIns, VERR_INVALID_POINTER);
        AssertReturn(pDrvIns->Internal.s.pVMR0 == pVM, VERR_INVALID_PARAMETER);

        PFNPDMDRVREQHANDLERR0 pfnReqHandlerR0 = pDrvIns->Internal.s.pfnReqHandlerR0;
        AssertPtrReturn(pfnReqHandlerR0, VERR_INVALID_POINTER);

        rc = pfnReqHandlerR0(pDrvIns, pReq->uOperation, pReq->u64Arg);
    }
    return rc;
}


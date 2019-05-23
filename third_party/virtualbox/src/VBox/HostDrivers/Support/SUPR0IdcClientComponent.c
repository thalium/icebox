/* $Id: SUPR0IdcClientComponent.c $ */
/** @file
 * VirtualBox Support Driver - IDC Client Lib, Component APIs.
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
#include "SUPR0IdcClientInternal.h"
#include <VBox/err.h>


/**
 * Registers a component factory with SUPDRV.
 *
 * @returns VBox status code.
 * @param   pHandle         The IDC handle.
 * @param   pFactory        The factory to register.
 */
SUPR0DECL(int) SUPR0IdcComponentRegisterFactory(PSUPDRVIDCHANDLE pHandle, PCSUPDRVFACTORY pFactory)
{
    SUPDRVIDCREQCOMPREGFACTORY Req;

    /*
     * Validate the handle before we access it.
     */
    AssertPtrReturn(pHandle, VERR_INVALID_HANDLE);
    AssertPtrReturn(pHandle->s.pSession, VERR_INVALID_HANDLE);

    /*
     * Construct and fire off the request.
     */
    Req.Hdr.cb = sizeof(Req);
    Req.Hdr.rc = VERR_WRONG_ORDER;
    Req.Hdr.pSession = pHandle->s.pSession;
    Req.u.In.pFactory = pFactory;

    return supR0IdcNativeCall(pHandle, SUPDRV_IDC_REQ_COMPONENT_REGISTER_FACTORY, &Req.Hdr);
}


/**
 * Deregisters a component factory with SUPDRV.
 *
 * @returns VBox status code.
 * @param   pHandle         The IDC handle.
 * @param   pFactory        The factory to register.
 */
SUPR0DECL(int) SUPR0IdcComponentDeregisterFactory(PSUPDRVIDCHANDLE pHandle, PCSUPDRVFACTORY pFactory)
{
    SUPDRVIDCREQCOMPDEREGFACTORY Req;

    /*
     * Validate the handle before we access it.
     */
    AssertPtrReturn(pHandle, VERR_INVALID_HANDLE);
    AssertPtrReturn(pHandle->s.pSession, VERR_INVALID_HANDLE);

    /*
     * Construct and fire off the request.
     */
    Req.Hdr.cb = sizeof(Req);
    Req.Hdr.rc = VERR_WRONG_ORDER;
    Req.Hdr.pSession = pHandle->s.pSession;
    Req.u.In.pFactory = pFactory;

    return supR0IdcNativeCall(pHandle, SUPDRV_IDC_REQ_COMPONENT_DEREGISTER_FACTORY, &Req.Hdr);
}


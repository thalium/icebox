/* $Id: UsbTestServiceGadgetInternal.h $ */
/** @file
 * UsbTestServ - Remote USB test configuration and execution server, Interal gadget interfaces.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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

#ifndef ___UsbTestServiceGadgetInternal_h___
#define ___UsbTestServiceGadgetInternal_h___

#include <iprt/cdefs.h>
#include <iprt/types.h>

#include "UsbTestServiceGadget.h"

RT_C_DECLS_BEGIN

/** Pointer to an opaque type dependent gadget host instance data. */
typedef struct UTSGADGETCLASSINT *PUTSGADGETCLASSINT;
/** Pointer to a gadget host instance pointer. */
typedef PUTSGADGETCLASSINT *PPUTSGADGETCLASSINT;

/**
 * Gadget class interface.
 */
typedef struct UTSGADGETCLASSIF
{
    /** The gadget class type implemented. */
    UTSGADGETCLASS            enmClass;
    /** Description. */
    const char               *pszDesc;
    /** Size of the class specific instance data. */
    size_t                    cbClass;

    /**
     * Initializes the gadget class instance.
     *
     * @returns IPRT status code.
     * @param   pClass        The interface specific instance data.
     * @param   paCfg         The configuration of the interface.
     */
    DECLR3CALLBACKMEMBER(int, pfnInit, (PUTSGADGETCLASSINT pClass, PCUTSGADGETCFGITEM paCfg));

    /**
     * Terminates the gadget class instance.
     *
     * @returns nothing.
     * @param   pClass        The interface specific instance data.
     */
    DECLR3CALLBACKMEMBER(void, pfnTerm, (PUTSGADGETCLASSINT pClass));

    /**
     * Returns the bus ID of the class instance.
     *
     * @returns Bus ID.
     * @param   pClass        The interface specific instance data.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetBusId, (PUTSGADGETCLASSINT pClass));

    /**
     * Connects the gadget.
     *
     * @returns IPRT status code.
     * @param   pClass        The interface specific instance data.
     */
    DECLR3CALLBACKMEMBER(int, pfnConnect, (PUTSGADGETCLASSINT pClass));

    /**
     * Disconnect the gadget.
     *
     * @returns IPRT status code.
     * @param   pClass        The interface specific instance data.
     */
    DECLR3CALLBACKMEMBER(int, pfnDisconnect, (PUTSGADGETCLASSINT pClass));

} UTSGADGETCLASSIF;
/** Pointer to a gadget class callback table. */
typedef UTSGADGETCLASSIF *PUTSGADGETCLASSIF;
/** Pointer to a const gadget host callback table. */
typedef const struct UTSGADGETCLASSIF *PCUTSGADGETCLASSIF;

extern UTSGADGETCLASSIF const g_UtsGadgetClassTest;

RT_C_DECLS_END

#endif


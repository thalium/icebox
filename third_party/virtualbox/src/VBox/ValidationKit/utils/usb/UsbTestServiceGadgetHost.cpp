/* $Id: UsbTestServiceGadgetHost.cpp $ */
/** @file
 * UsbTestServ - Remote USB test configuration and execution server, USB gadget host API.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/asm.h>
#include <iprt/cdefs.h>
#include <iprt/ctype.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/types.h>

#include "UsbTestServiceGadget.h"
#include "UsbTestServiceGadgetHostInternal.h"


/*********************************************************************************************************************************
*   Constants And Macros, Structures and Typedefs                                                                                *
*********************************************************************************************************************************/

/**
 * Internal UTS gadget host instance data.
 */
typedef struct UTSGADGETHOSTINT
{
    /** Reference counter. */
    volatile uint32_t         cRefs;
    /** Pointer to the gadget host callback table. */
    PCUTSGADGETHOSTIF         pHstIf;
    /** Interface specific instance data - variable in size. */
    uint8_t                   abIfInst[1];
} UTSGADGETHOSTINT;
/** Pointer to the internal gadget host instance data. */
typedef UTSGADGETHOSTINT *PUTSGADGETHOSTINT;


/*********************************************************************************************************************************
*   Global variables                                                                                                             *
*********************************************************************************************************************************/

/** Known gadget host interfaces. */
static const PCUTSGADGETHOSTIF g_apUtsGadgetHostIf[] =
{
    &g_UtsGadgetHostIfUsbIp,
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/


/**
 * Destroys a gadget host instance.
 *
 * @returns nothing.
 * @param   pThis    The gadget host instance.
 */
static void utsGadgetHostDestroy(PUTSGADGETHOSTINT pThis)
{
    /** @todo Remove all gadgets. */
    pThis->pHstIf->pfnTerm((PUTSGADGETHOSTTYPEINT)&pThis->abIfInst[0]);
    RTMemFree(pThis);
}


DECLHIDDEN(int) utsGadgetHostCreate(UTSGADGETHOSTTYPE enmType, PCUTSGADGETCFGITEM paCfg,
                                    PUTSGADGETHOST phGadgetHost)
{
    int rc = VINF_SUCCESS;
    PCUTSGADGETHOSTIF pIf = NULL;

    /* Get the interface. */
    for (unsigned i = 0; i < RT_ELEMENTS(g_apUtsGadgetHostIf); i++)
    {
        if (g_apUtsGadgetHostIf[i]->enmType == enmType)
        {
            pIf = g_apUtsGadgetHostIf[i];
            break;
        }
    }

    if (RT_LIKELY(pIf))
    {
        PUTSGADGETHOSTINT pThis = (PUTSGADGETHOSTINT)RTMemAllocZ(RT_OFFSETOF(UTSGADGETHOSTINT, abIfInst[pIf->cbIf]));
        if (RT_LIKELY(pThis))
        {
            pThis->cRefs = 1;
            pThis->pHstIf = pIf;
            rc = pIf->pfnInit((PUTSGADGETHOSTTYPEINT)&pThis->abIfInst[0], paCfg);
            if (RT_SUCCESS(rc))
                *phGadgetHost = pThis;
            else
                RTMemFree(pThis);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VERR_INVALID_PARAMETER;

    return rc;
}


DECLHIDDEN(uint32_t) utsGadgetHostRetain(UTSGADGETHOST hGadgetHost)
{
    PUTSGADGETHOSTINT pThis = hGadgetHost;

    AssertPtrReturn(pThis, 0);

    return ASMAtomicIncU32(&pThis->cRefs);
}


DECLHIDDEN(uint32_t) utsGadgetHostRelease(UTSGADGETHOST hGadgetHost)
{
    PUTSGADGETHOSTINT pThis = hGadgetHost;

    AssertPtrReturn(pThis, 0);

    uint32_t cRefs = ASMAtomicDecU32(&pThis->cRefs);
    if (!cRefs)
        utsGadgetHostDestroy(pThis);

    return cRefs;
}


DECLHIDDEN(int) utsGadgetHostGadgetConnect(UTSGADGETHOST hGadgetHost, UTSGADGET hGadget)
{
    PUTSGADGETHOSTINT pThis = hGadgetHost;

    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    return pThis->pHstIf->pfnGadgetConnect((PUTSGADGETHOSTTYPEINT)&pThis->abIfInst[0], hGadget);
}


DECLHIDDEN(int) utsGadgetHostGadgetDisconnect(UTSGADGETHOST hGadgetHost, UTSGADGET hGadget)
{
    PUTSGADGETHOSTINT pThis = hGadgetHost;

    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    return pThis->pHstIf->pfnGadgetDisconnect((PUTSGADGETHOSTTYPEINT)&pThis->abIfInst[0], hGadget);
}


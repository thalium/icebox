/* $Id: allocex-r3-generic.cpp $ */
/** @file
 * IPRT - Memory Allocation, Extended Alloc Workers, generic.
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
#define RTMEM_NO_WRAP_TO_EF_APIS
#include <iprt/mem.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include "../allocex.h"



DECLHIDDEN(int) rtMemAllocEx16BitReach(size_t cbAlloc, uint32_t fFlags, void **ppv)
{
    RT_NOREF(cbAlloc, fFlags, ppv);
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(int) rtMemAllocEx32BitReach(size_t cbAlloc, uint32_t fFlags, void **ppv)
{
    RT_NOREF(cbAlloc, fFlags, ppv);
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(void) rtMemFreeExYyBitReach(void *pv, size_t cb, uint32_t fFlags)
{
    RT_NOREF(pv, cb, fFlags);
    AssertFailed();
}


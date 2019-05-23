/* $Id: RTFileReadAllFree-generic.cpp $ */
/** @file
 * IPRT - RTFileReadAllFree, generic implementation.
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
#include <iprt/file.h>
#include "internal/iprt.h"

#include <iprt/mem.h>
#include <iprt/assert.h>


RTDECL(void) RTFileReadAllFree(void *pvFile, size_t cbFile)
{
    AssertPtrReturnVoid(pvFile);

    /*
     * We've got a 32 byte header to make sure the caller isn't screwing us.
     * It's all hardcoded fun for now...
     */
    pvFile = (void *)((uintptr_t)pvFile - 32);
    Assert(*(size_t *)pvFile == cbFile); RT_NOREF_PV(cbFile);
    *(size_t *)pvFile = ~(size_t)1;

    RTMemFree(pvFile);
}
RT_EXPORT_SYMBOL(RTFileReadAllFree);


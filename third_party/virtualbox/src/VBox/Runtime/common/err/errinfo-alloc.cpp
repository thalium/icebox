/* $Id: errinfo-alloc.cpp $ */
/** @file
 * IPRT - Error Info, Allocators.
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
#include "internal/iprt.h"
#include <iprt/err.h>

#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/string.h>



RTDECL(PRTERRINFO)  RTErrInfoAlloc(size_t cbMsg)
{
    PRTERRINFO pErrInfo;
    RTErrInfoAllocEx(cbMsg, &pErrInfo);
    return pErrInfo;
}


RTDECL(int)         RTErrInfoAllocEx(size_t cbMsg, PRTERRINFO *ppErrInfo)
{
    if (cbMsg == 0)
        cbMsg = _4K;
    else
        cbMsg = RT_ALIGN_Z(cbMsg, 256);

    PRTERRINFO pErrInfo;
    *ppErrInfo = pErrInfo = (PRTERRINFO)RTMemTmpAlloc(sizeof(*pErrInfo) + cbMsg);
    if (RT_UNLIKELY(!pErrInfo))
        return VERR_NO_TMP_MEMORY;

    RTErrInfoInit(pErrInfo, (char *)(pErrInfo + 1), cbMsg);
    pErrInfo->fFlags = RTERRINFO_FLAGS_T_ALLOC | RTERRINFO_FLAGS_MAGIC;
    return VINF_SUCCESS;
}


RTDECL(void)        RTErrInfoFree(PRTERRINFO pErrInfo)
{
    RTMemTmpFree(pErrInfo);
}


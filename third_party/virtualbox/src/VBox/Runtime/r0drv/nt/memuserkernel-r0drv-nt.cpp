/* $Id: memuserkernel-r0drv-nt.cpp $ */
/** @file
 * IPRT - User & Kernel Memory, Ring-0 Driver, NT.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include "the-nt-kernel.h"

#include <iprt/mem.h>
#include <iprt/err.h>


RTR0DECL(int) RTR0MemUserCopyFrom(void *pvDst, RTR3PTR R3PtrSrc, size_t cb)
{
    __try
    {
        ProbeForRead((PVOID)R3PtrSrc, cb, 1);
        memcpy(pvDst, (void const *)R3PtrSrc, cb);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return VERR_ACCESS_DENIED;
    }
    return VINF_SUCCESS;
}


RTR0DECL(int) RTR0MemUserCopyTo(RTR3PTR R3PtrDst, void const *pvSrc, size_t cb)
{
    __try
    {
        ProbeForWrite((PVOID)R3PtrDst, cb, 1);
        memcpy((void *)R3PtrDst, pvSrc, cb);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return VERR_ACCESS_DENIED;
    }
    return VINF_SUCCESS;
}


RTR0DECL(bool) RTR0MemUserIsValidAddr(RTR3PTR R3Ptr)
{
#ifdef IPRT_TARGET_NT4
    /* Play safe+wrong... it used to be a constant, but in w2k+ is a variable. */
    return R3Ptr < _2G;
#else
    return R3Ptr <= (uintptr_t)MM_HIGHEST_USER_ADDRESS;
#endif
}


RTR0DECL(bool) RTR0MemKernelIsValidAddr(void *pv)
{
#ifdef IPRT_TARGET_NT4
    /* Play safe+wrong... it used to be a constant, but in w2k+ is a variable. */
    return (uintptr_t)pv >= _2G;
#else
    return (uintptr_t)pv >= (uintptr_t)MM_SYSTEM_RANGE_START;
#endif
}


RTR0DECL(bool) RTR0MemAreKrnlAndUsrDifferent(void)
{
    return true;
}


RTR0DECL(int) RTR0MemKernelCopyFrom(void *pvDst, void const *pvSrc, size_t cb)
{
    __try
    {
        memcpy(pvDst, pvSrc, cb);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return VERR_ACCESS_DENIED;
    }
    return VINF_SUCCESS;
}


RTR0DECL(int) RTR0MemKernelCopyTo(void *pvDst, void const *pvSrc, size_t cb)
{
    __try
    {
        memcpy(pvDst, pvSrc, cb);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return VERR_ACCESS_DENIED;
    }
    return VINF_SUCCESS;
}


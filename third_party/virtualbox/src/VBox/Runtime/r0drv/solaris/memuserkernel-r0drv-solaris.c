/* $Id: memuserkernel-r0drv-solaris.c $ */
/** @file
 * IPRT - User & Kernel Memory, Ring-0 Driver, Solaris.
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
#include "the-solaris-kernel.h"
#include "internal/iprt.h"
#include <iprt/mem.h>

#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
# include <iprt/asm-amd64-x86.h>
#endif
#include <iprt/assert.h>
#include <iprt/err.h>


RTR0DECL(int) RTR0MemUserCopyFrom(void *pvDst, RTR3PTR R3PtrSrc, size_t cb)
{
    int rc;
    RT_ASSERT_INTS_ON();

    rc = ddi_copyin((const char *)R3PtrSrc, pvDst, cb, 0 /*flags*/);
    if (RT_LIKELY(rc == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}


RTR0DECL(int) RTR0MemUserCopyTo(RTR3PTR R3PtrDst, void const *pvSrc, size_t cb)
{
    int rc;
    RT_ASSERT_INTS_ON();

    rc = ddi_copyout(pvSrc, (void *)R3PtrDst, cb, 0 /*flags*/);
    if (RT_LIKELY(rc == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}


RTR0DECL(bool) RTR0MemUserIsValidAddr(RTR3PTR R3Ptr)
{
    return R3Ptr < kernelbase;
}


RTR0DECL(bool) RTR0MemKernelIsValidAddr(void *pv)
{
    return (uintptr_t)pv >= kernelbase;
}


RTR0DECL(bool) RTR0MemAreKrnlAndUsrDifferent(void)
{
    return true;
}


RTR0DECL(int) RTR0MemKernelCopyFrom(void *pvDst, void const *pvSrc, size_t cb)
{
    int rc = kcopy(pvSrc, pvDst, cb);
    if (RT_LIKELY(rc == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}


RTR0DECL(int) RTR0MemKernelCopyTo(void *pvDst, void const *pvSrc, size_t cb)
{
    int rc = kcopy(pvSrc, pvDst, cb);
    if (RT_LIKELY(rc == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}


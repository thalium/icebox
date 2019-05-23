/* $Id: memuserkernel-r0drv-darwin.cpp $ */
/** @file
 * IPRT - User & Kernel Memory, Ring-0 Driver, Darwin.
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
#include "the-darwin-kernel.h"
#include "internal/iprt.h"
#include <iprt/mem.h>
#include <iprt/assert.h>

#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
# include <iprt/asm-amd64-x86.h>
#endif
#include <iprt/err.h>


RTR0DECL(int) RTR0MemUserCopyFrom(void *pvDst, RTR3PTR R3PtrSrc, size_t cb)
{
    RT_ASSERT_INTS_ON();
    IPRT_DARWIN_SAVE_EFL_AC();
    int rc = copyin((const user_addr_t)R3PtrSrc, pvDst, cb);
    IPRT_DARWIN_RESTORE_EFL_AC();
    if (RT_LIKELY(rc == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}


RTR0DECL(int) RTR0MemUserCopyTo(RTR3PTR R3PtrDst, void const *pvSrc, size_t cb)
{
    RT_ASSERT_INTS_ON();
    IPRT_DARWIN_SAVE_EFL_AC();
    int rc = copyout(pvSrc, R3PtrDst, cb);
    IPRT_DARWIN_RESTORE_EFL_AC();
    if (RT_LIKELY(rc == 0))
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
}


RTR0DECL(bool) RTR0MemUserIsValidAddr(RTR3PTR R3Ptr)
{
    /* the commpage is above this. */
#ifdef RT_ARCH_X86
    return R3Ptr < VM_MAX_ADDRESS;
#else
    return R3Ptr < VM_MAX_PAGE_ADDRESS;
#endif
}


RTR0DECL(bool) RTR0MemKernelIsValidAddr(void *pv)
{
    /* Found no public #define or symbol for checking this, so we'll
       have to make do with thing found in the debugger and the sources. */
#ifdef RT_ARCH_X86
    NOREF(pv);
    return true;    /* Almost anything is a valid kernel address here. */

#elif defined(RT_ARCH_AMD64)
    return (uintptr_t)pv >= UINT64_C(0xffff800000000000);

#else
# error "PORTME"
#endif
}


RTR0DECL(bool) RTR0MemAreKrnlAndUsrDifferent(void)
{
    /* As mentioned in RTR0MemKernelIsValidAddr, found no way of checking
       this at compiler or runtime. */
#ifdef RT_ARCH_X86
    return false;
#else
    return true;
#endif
}


RTR0DECL(int) RTR0MemKernelCopyFrom(void *pvDst, void const *pvSrc, size_t cb)
{
    RT_NOREF(pvDst, pvSrc, cb);
    return VERR_NOT_SUPPORTED;
}


RTR0DECL(int) RTR0MemKernelCopyTo(void *pvDst, void const *pvSrc, size_t cb)
{
    RT_NOREF(pvDst, pvSrc, cb);
    return VERR_NOT_SUPPORTED;
}


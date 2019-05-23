/* $Id: alloc-r0drv-nt.cpp $ */
/** @file
 * IPRT - Memory Allocation, Ring-0 Driver, NT.
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
#include "the-nt-kernel.h"
#include "internal/iprt.h"
#include <iprt/mem.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include "r0drv/alloc-r0drv.h"
#include "internal-r0drv-nt.h"


/**
 * OS specific allocation function.
 */
DECLHIDDEN(int) rtR0MemAllocEx(size_t cb, uint32_t fFlags, PRTMEMHDR *ppHdr)
{
    if (!(fFlags & RTMEMHDR_FLAG_ANY_CTX))
    {
#if 1 /* This allegedly makes the driver verifier happier... */
        POOL_TYPE enmPoolType = NonPagedPool;
        if (!(fFlags & RTMEMHDR_FLAG_EXEC) && g_uRtNtVersion >= RTNT_MAKE_VERSION(8,0))
            enmPoolType = NonPagedPoolNx;
        PRTMEMHDR pHdr = (PRTMEMHDR)ExAllocatePoolWithTag(enmPoolType, cb + sizeof(*pHdr), IPRT_NT_POOL_TAG);
#else
        PRTMEMHDR pHdr = (PRTMEMHDR)ExAllocatePoolWithTag(NonPagedPool, cb + sizeof(*pHdr), IPRT_NT_POOL_TAG);
#endif
        if (pHdr)
        {
            pHdr->u32Magic  = RTMEMHDR_MAGIC;
            pHdr->fFlags    = fFlags;
            pHdr->cb        = (uint32_t)cb; Assert(pHdr->cb == cb);
            pHdr->cbReq     = (uint32_t)cb;
            *ppHdr = pHdr;
            return VINF_SUCCESS;
        }
        return VERR_NO_MEMORY;
    }
    return VERR_NOT_SUPPORTED;
}


/**
 * OS specific free function.
 */
DECLHIDDEN(void) rtR0MemFree(PRTMEMHDR pHdr)
{
    pHdr->u32Magic += 1;
    ExFreePool(pHdr);
}


/**
 * Allocates physical contiguous memory (below 4GB).
 * The allocation is page aligned and its contents is undefined.
 *
 * @returns Pointer to the memory block. This is page aligned.
 * @param   pPhys   Where to store the physical address.
 * @param   cb      The allocation size in bytes. This is always
 *                  rounded up to PAGE_SIZE.
 */
RTR0DECL(void *) RTMemContAlloc(PRTCCPHYS pPhys, size_t cb)
{
    /*
     * validate input.
     */
    Assert(VALID_PTR(pPhys));
    Assert(cb > 0);

    /*
     * Allocate and get physical address.
     * Make sure the return is page aligned.
     */
    PHYSICAL_ADDRESS MaxPhysAddr;
    MaxPhysAddr.HighPart = 0;
    MaxPhysAddr.LowPart = 0xffffffff;
    cb = RT_ALIGN_Z(cb, PAGE_SIZE);
    void *pv = MmAllocateContiguousMemory(cb, MaxPhysAddr);
    if (pv)
    {
        if (!((uintptr_t)pv & PAGE_OFFSET_MASK))    /* paranoia */
        {
            PHYSICAL_ADDRESS PhysAddr = MmGetPhysicalAddress(pv);
            if (!PhysAddr.HighPart)                 /* paranoia */
            {
                *pPhys = (RTCCPHYS)PhysAddr.LowPart;
                return pv;
            }

            /* failure */
            AssertMsgFailed(("MMAllocContiguousMemory returned high address! PhysAddr=%RX64\n", (uint64_t)PhysAddr.QuadPart));
        }
        else
            AssertMsgFailed(("MMAllocContiguousMemory didn't return a page aligned address - %p!\n", pv));

        MmFreeContiguousMemory(pv);
    }

    return NULL;
}


/**
 * Frees memory allocated ysing RTMemContAlloc().
 *
 * @param   pv      Pointer to return from RTMemContAlloc().
 * @param   cb      The cb parameter passed to RTMemContAlloc().
 */
RTR0DECL(void) RTMemContFree(void *pv, size_t cb)
{
    if (pv)
    {
        Assert(cb > 0); NOREF(cb);
        AssertMsg(!((uintptr_t)pv & PAGE_OFFSET_MASK), ("pv=%p\n", pv));
        MmFreeContiguousMemory(pv);
    }
}


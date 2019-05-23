/* $Id: alloc-r0drv-os2.cpp $ */
/** @file
 * IPRT - Memory Allocation, Ring-0 Driver, OS/2.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "the-os2-kernel.h"
#include "internal/iprt.h"
#include <iprt/mem.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/param.h>
#include "r0drv/alloc-r0drv.h" /** @todo drop the r0drv/alloc-r0drv.cpp stuff on OS/2? */


DECLHIDDEN(int) rtR0MemAllocEx(size_t cb, uint32_t fFlags, PRTMEMHDR *ppHdr)
{
    if (fFlags & RTMEMHDR_FLAG_ANY_CTX)
        return VERR_NOT_SUPPORTED;

    void *pv = NULL;
    APIRET rc = KernVMAlloc(cb + sizeof(RTMEMHDR), VMDHA_FIXED, &pv, (void **)-1, NULL);
    if (RT_UNLIKELY(rc != NO_ERROR))
        return RTErrConvertFromOS2(rc);

    PRTMEMHDR   pHdr = (PRTMEMHDR)pv;
    pHdr->u32Magic   = RTMEMHDR_MAGIC;
    pHdr->fFlags     = fFlags;
    pHdr->cb         = cb;
    pHdr->cbReq      = cb;
    *ppHdr = pHdr;
    return VINF_SUCCESS;
}


DECLHIDDEN(void) rtR0MemFree(PRTMEMHDR pHdr)
{
    pHdr->u32Magic += 1;
    APIRET rc = KernVMFree(pHdr);
    Assert(!rc); NOREF(rc);
}


RTR0DECL(void *) RTMemContAlloc(PRTCCPHYS pPhys, size_t cb)
{
    /*
     * Validate input.
     */
    AssertPtr(pPhys);
    Assert(cb > 0);

    /*
     * All physical memory in OS/2 is below 4GB, so this should be kind of easy.
     */
    cb = RT_ALIGN_Z(cb, PAGE_SIZE); /* -> page aligned result. */
    PVOID pv = NULL;
    PVOID PhysAddr = (PVOID)~0UL;
    APIRET rc = KernVMAlloc(cb, VMDHA_FIXED | VMDHA_CONTIG, &pv, &PhysAddr, NULL);
    if (!rc)
    {
        Assert(PhysAddr != (PVOID)~0UL);
        Assert(!((uintptr_t)pv & PAGE_OFFSET_MASK));
        *pPhys = (uintptr_t)PhysAddr;
        return pv;
    }
    return NULL;
}


RTR0DECL(void) RTMemContFree(void *pv, size_t cb)
{
    if (pv)
    {
        AssertMsg(!((uintptr_t)pv & PAGE_OFFSET_MASK), ("pv=%p\n", pv));
        KernVMFree(pv);
    }
}


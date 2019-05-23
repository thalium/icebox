/* $Id: alloc-r0drv-freebsd.c $ */
/** @file
 * IPRT - Memory Allocation, Ring-0 Driver, FreeBSD.
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
#include "the-freebsd-kernel.h"
#include "internal/iprt.h"
#include <iprt/mem.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/param.h>

#include "r0drv/alloc-r0drv.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/* These two statements will define two globals and add initializers
   and destructors that will be called at load/unload time (I think). */
MALLOC_DEFINE(M_IPRTHEAP, "iprtheap", "IPRT - heap");
MALLOC_DEFINE(M_IPRTCONT, "iprtcont", "IPRT - contiguous");


DECLHIDDEN(int) rtR0MemAllocEx(size_t cb, uint32_t fFlags, PRTMEMHDR *ppHdr)
{
    size_t      cbAllocated = cb;
    PRTMEMHDR   pHdr        = NULL;

#ifdef RT_ARCH_AMD64
    /*
     * Things are a bit more complicated on AMD64 for executable memory
     * because we need to be in the ~2GB..~0 range for code.
     */
    if (fFlags & RTMEMHDR_FLAG_EXEC)
    {
        if (fFlags & RTMEMHDR_FLAG_ANY_CTX)
            return VERR_NOT_SUPPORTED;

# ifdef USE_KMEM_ALLOC_PROT
        pHdr = (PRTMEMHDR)kmem_alloc_prot(kernel_map, cb + sizeof(*pHdr),
                                          VM_PROT_ALL, VM_PROT_ALL, KERNBASE);
# else
        vm_object_t pVmObject = NULL;
        vm_offset_t Addr = KERNBASE;
        cbAllocated = RT_ALIGN_Z(cb + sizeof(*pHdr), PAGE_SIZE);

        pVmObject = vm_object_allocate(OBJT_DEFAULT, cbAllocated >> PAGE_SHIFT);
        if (!pVmObject)
            return VERR_NO_EXEC_MEMORY;

        /* Addr contains a start address vm_map_find will start searching for suitable space at. */
#if __FreeBSD_version >= 1000055
        int rc = vm_map_find(kernel_map, pVmObject, 0, &Addr,
                             cbAllocated, 0, VMFS_ANY_SPACE, VM_PROT_ALL, VM_PROT_ALL, 0);
#else
        int rc = vm_map_find(kernel_map, pVmObject, 0, &Addr,
                             cbAllocated, TRUE, VM_PROT_ALL, VM_PROT_ALL, 0);
#endif
        if (rc == KERN_SUCCESS)
        {
            rc = vm_map_wire(kernel_map, Addr, Addr + cbAllocated,
                             VM_MAP_WIRE_SYSTEM | VM_MAP_WIRE_NOHOLES);
            if (rc == KERN_SUCCESS)
            {
                pHdr = (PRTMEMHDR)Addr;

                if (fFlags & RTMEMHDR_FLAG_ZEROED)
                    bzero(pHdr, cbAllocated);
            }
            else
                vm_map_remove(kernel_map,
                              Addr,
                              Addr + cbAllocated);
        }
        else
            vm_object_deallocate(pVmObject);
# endif
    }
    else
#endif
    {
        pHdr = (PRTMEMHDR)malloc(cb + sizeof(RTMEMHDR), M_IPRTHEAP,
                                 fFlags & RTMEMHDR_FLAG_ZEROED ? M_NOWAIT | M_ZERO : M_NOWAIT);
    }

    if (RT_UNLIKELY(!pHdr))
        return VERR_NO_MEMORY;

    pHdr->u32Magic   = RTMEMHDR_MAGIC;
    pHdr->fFlags     = fFlags;
    pHdr->cb         = cbAllocated;
    pHdr->cbReq      = cb;

    *ppHdr = pHdr;
    return VINF_SUCCESS;
}


DECLHIDDEN(void) rtR0MemFree(PRTMEMHDR pHdr)
{
    pHdr->u32Magic += 1;

#ifdef RT_ARCH_AMD64
    if (pHdr->fFlags & RTMEMHDR_FLAG_EXEC)
# ifdef USE_KMEM_ALLOC_PROT
        kmem_free(kernel_map, (vm_offset_t)pHdr, pHdr->cb);
# else
        vm_map_remove(kernel_map, (vm_offset_t)pHdr, ((vm_offset_t)pHdr) + pHdr->cb);
# endif
    else
#endif
        free(pHdr, M_IPRTHEAP);
}


RTR0DECL(void *) RTMemContAlloc(PRTCCPHYS pPhys, size_t cb)
{
    void *pv;

    /*
     * Validate input.
     */
    AssertPtr(pPhys);
    Assert(cb > 0);

    /*
     * This API works in pages, so no need to do any size aligning.
     */
    pv = contigmalloc(cb,                   /* size */
                      M_IPRTCONT,           /* type */
                      M_NOWAIT | M_ZERO,    /* flags */
                      0,                    /* lowest physical address*/
                      _4G-1,                /* highest physical address */
                      PAGE_SIZE,            /* alignment. */
                      0);                   /* boundary */
    if (pv)
    {
        Assert(!((uintptr_t)pv & PAGE_OFFSET_MASK));
        *pPhys = vtophys(pv);
        Assert(!(*pPhys & PAGE_OFFSET_MASK));
    }
    return pv;
}


RTR0DECL(void) RTMemContFree(void *pv, size_t cb)
{
    if (pv)
    {
        AssertMsg(!((uintptr_t)pv & PAGE_OFFSET_MASK), ("pv=%p\n", pv));
        contigfree(pv, cb, M_IPRTCONT);
    }
}


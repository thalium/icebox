/* $Id: memobj-r0drv-netbsd.c $ */
/** @file
 * IPRT - Ring-0 Memory Objects, NetBSD.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 * Copyright (c) 2011 Andriy Gapon <avg@FreeBSD.org>
 * Copyright (c) 2014 Arto Huusko
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
#include "the-netbsd-kernel.h"

#include <iprt/memobj.h>
#include <iprt/mem.h>
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/log.h>
#include <iprt/param.h>
#include <iprt/process.h>
#include "internal/memobj.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The NetBSD version of the memory object structure.
 */
typedef struct RTR0MEMOBJNETBSD
{
    /** The core structure. */
    RTR0MEMOBJINTERNAL  Core;
    size_t              size;
    struct pglist       pglist;
} RTR0MEMOBJNETBSD, *PRTR0MEMOBJNETBSD;


typedef struct vm_map* vm_map_t;

/**
 * Gets the virtual memory map the specified object is mapped into.
 *
 * @returns VM map handle on success, NULL if no map.
 * @param   pMem                The memory object.
 */
static vm_map_t rtR0MemObjNetBSDGetMap(PRTR0MEMOBJINTERNAL pMem)
{
    switch (pMem->enmType)
    {
        case RTR0MEMOBJTYPE_PAGE:
        case RTR0MEMOBJTYPE_LOW:
        case RTR0MEMOBJTYPE_CONT:
            return kernel_map;

        case RTR0MEMOBJTYPE_PHYS:
        case RTR0MEMOBJTYPE_PHYS_NC:
            return NULL; /* pretend these have no mapping atm. */

        case RTR0MEMOBJTYPE_LOCK:
            return pMem->u.Lock.R0Process == NIL_RTR0PROCESS
                ? kernel_map
                : &((struct proc *)pMem->u.Lock.R0Process)->p_vmspace->vm_map;

        case RTR0MEMOBJTYPE_RES_VIRT:
            return pMem->u.ResVirt.R0Process == NIL_RTR0PROCESS
                ? kernel_map
                : &((struct proc *)pMem->u.ResVirt.R0Process)->p_vmspace->vm_map;

        case RTR0MEMOBJTYPE_MAPPING:
            return pMem->u.Mapping.R0Process == NIL_RTR0PROCESS
                ? kernel_map
                : &((struct proc *)pMem->u.Mapping.R0Process)->p_vmspace->vm_map;

        default:
            return NULL;
    }
}


DECLHIDDEN(int) rtR0MemObjNativeFree(RTR0MEMOBJ pMem)
{
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)pMem;
    int rc;

    switch (pMemNetBSD->Core.enmType)
    {
        case RTR0MEMOBJTYPE_PAGE:
        {
            kmem_free(pMemNetBSD->Core.pv, pMemNetBSD->Core.cb);
            break;
        }
        case RTR0MEMOBJTYPE_LOW:
        case RTR0MEMOBJTYPE_CONT:
        {
            /* Unmap */
            pmap_kremove((vaddr_t)pMemNetBSD->Core.pv, pMemNetBSD->Core.cb);
            /* Free the virtual space */
            uvm_km_free(kernel_map, (vaddr_t)pMemNetBSD->Core.pv, pMemNetBSD->Core.cb, UVM_KMF_VAONLY);
            /* Free the physical pages */
            uvm_pglistfree(&pMemNetBSD->pglist);
            break;
        }
        case RTR0MEMOBJTYPE_PHYS:
        case RTR0MEMOBJTYPE_PHYS_NC:
        {
            /* Free the physical pages */
            uvm_pglistfree(&pMemNetBSD->pglist);
            break;
        }
        case RTR0MEMOBJTYPE_LOCK:
            if (pMemNetBSD->Core.u.Lock.R0Process != NIL_RTR0PROCESS)
            {
                uvm_map_pageable(
                        &((struct proc *)pMemNetBSD->Core.u.Lock.R0Process)->p_vmspace->vm_map,
                        (vaddr_t)pMemNetBSD->Core.pv,
                        ((vaddr_t)pMemNetBSD->Core.pv) + pMemNetBSD->Core.cb,
                        1, 0);
            }
            break;
        case RTR0MEMOBJTYPE_RES_VIRT:
            if (pMemNetBSD->Core.u.Lock.R0Process == NIL_RTR0PROCESS)
            {
                uvm_km_free(kernel_map, (vaddr_t)pMemNetBSD->Core.pv, pMemNetBSD->Core.cb, UVM_KMF_VAONLY);
            }
            break;
        case RTR0MEMOBJTYPE_MAPPING:
            if (pMemNetBSD->Core.u.Lock.R0Process == NIL_RTR0PROCESS)
            {
                pmap_kremove((vaddr_t)pMemNetBSD->Core.pv, pMemNetBSD->Core.cb);
                uvm_km_free(kernel_map, (vaddr_t)pMemNetBSD->Core.pv, pMemNetBSD->Core.cb, UVM_KMF_VAONLY);
            }
            break;

        default:
            AssertMsgFailed(("enmType=%d\n", pMemNetBSD->Core.enmType));
            return VERR_INTERNAL_ERROR;
    }

    return VINF_SUCCESS;
}

static int rtR0MemObjNetBSDAllocHelper(PRTR0MEMOBJNETBSD pMemNetBSD, size_t cb, bool fExecutable,
                                         paddr_t VmPhysAddrHigh, bool fContiguous)
{
    /* Virtual space first */
    vaddr_t virt = uvm_km_alloc(kernel_map, cb, 0,
            UVM_KMF_VAONLY | UVM_KMF_WAITVA | UVM_KMF_CANFAIL);
    if (virt == 0)
        return VERR_NO_MEMORY;

    struct pglist *rlist = &pMemNetBSD->pglist;

    int nsegs = fContiguous ? 1 : INT_MAX;

    /* Physical pages */
    if (uvm_pglistalloc(cb, 0, VmPhysAddrHigh,
            PAGE_SIZE, 0, rlist, nsegs, 1) != 0)
    {
        uvm_km_free(kernel_map, virt, cb, UVM_KMF_VAONLY);
        return VERR_NO_MEMORY;
    }

    /* Map */
    struct vm_page *page;
    vm_prot_t prot = VM_PROT_READ | VM_PROT_WRITE;
    if (fExecutable)
        prot |= VM_PROT_EXECUTE;
    vaddr_t virt2 = virt;
    TAILQ_FOREACH(page, rlist, pageq.queue)
    {
        pmap_kenter_pa(virt2, VM_PAGE_TO_PHYS(page), prot, 0);
        virt2 += PAGE_SIZE;
    }

    pMemNetBSD->Core.pv = (void *)virt;
    if (fContiguous)
    {
        page = TAILQ_FIRST(rlist);
        pMemNetBSD->Core.u.Cont.Phys = VM_PAGE_TO_PHYS(page);
    }
    return VINF_SUCCESS;
}

DECLHIDDEN(int) rtR0MemObjNativeAllocPage(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD),
                                                                       RTR0MEMOBJTYPE_PAGE, NULL, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    void *pvMem = kmem_alloc(cb, KM_SLEEP);
    if (RT_UNLIKELY(!pvMem))
    {
        rtR0MemObjDelete(&pMemNetBSD->Core);
        return VERR_NO_PAGE_MEMORY;
    }
    if (fExecutable)
    {
        pmap_protect(pmap_kernel(), (vaddr_t)pvMem, ((vaddr_t)pvMem) + cb,
                VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    }

    pMemNetBSD->Core.pv = pvMem;
    *ppMem = &pMemNetBSD->Core;
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocLow(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD),
                                                                       RTR0MEMOBJTYPE_LOW, NULL, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    int rc = rtR0MemObjNetBSDAllocHelper(pMemNetBSD, cb, fExecutable, _4G - 1, false);
    if (rc)
    {
        rtR0MemObjDelete(&pMemNetBSD->Core);
        return rc;
    }

    *ppMem = &pMemNetBSD->Core;
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocCont(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD),
                                                                       RTR0MEMOBJTYPE_CONT, NULL, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    int rc = rtR0MemObjNetBSDAllocHelper(pMemNetBSD, cb, fExecutable, _4G - 1, true);
    if (rc)
    {
        rtR0MemObjDelete(&pMemNetBSD->Core);
        return rc;
    }

    *ppMem = &pMemNetBSD->Core;
    return VINF_SUCCESS;
}


static int rtR0MemObjNetBSDAllocPhysPages(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJTYPE enmType,
                                           size_t cb,
                                           RTHCPHYS PhysHighest, size_t uAlignment,
                                           bool fContiguous)
{
    paddr_t VmPhysAddrHigh;

    /* create the object. */
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD),
                                                                       enmType, NULL, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    if (PhysHighest != NIL_RTHCPHYS)
        VmPhysAddrHigh = PhysHighest;
    else
        VmPhysAddrHigh = ~(paddr_t)0;

    int nsegs = fContiguous ? 1 : INT_MAX;

    int error = uvm_pglistalloc(cb, 0, VmPhysAddrHigh, uAlignment, 0, &pMemNetBSD->pglist, nsegs, 1);
    if (error)
    {
        rtR0MemObjDelete(&pMemNetBSD->Core);
        return VERR_NO_MEMORY;
    }

    if (fContiguous)
    {
        Assert(enmType == RTR0MEMOBJTYPE_PHYS);
        const struct vm_page * const pg = TAILQ_FIRST(&pMemNetBSD->pglist);
        pMemNetBSD->Core.u.Phys.PhysBase = VM_PAGE_TO_PHYS(pg);
        pMemNetBSD->Core.u.Phys.fAllocated = true;
    }
    *ppMem = &pMemNetBSD->Core;

    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPhys(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, RTHCPHYS PhysHighest, size_t uAlignment)
{
    return rtR0MemObjNetBSDAllocPhysPages(ppMem, RTR0MEMOBJTYPE_PHYS, cb, PhysHighest, uAlignment, true);
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPhysNC(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, RTHCPHYS PhysHighest)
{
    return rtR0MemObjNetBSDAllocPhysPages(ppMem, RTR0MEMOBJTYPE_PHYS_NC, cb, PhysHighest, PAGE_SIZE, false);
}


DECLHIDDEN(int) rtR0MemObjNativeEnterPhys(PPRTR0MEMOBJINTERNAL ppMem, RTHCPHYS Phys, size_t cb, uint32_t uCachePolicy)
{
    AssertReturn(uCachePolicy == RTMEM_CACHE_POLICY_DONT_CARE, VERR_NOT_SUPPORTED);

    /* create the object. */
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD), RTR0MEMOBJTYPE_PHYS, NULL, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    /* there is no allocation here, it needs to be mapped somewhere first. */
    pMemNetBSD->Core.u.Phys.fAllocated = false;
    pMemNetBSD->Core.u.Phys.PhysBase = Phys;
    pMemNetBSD->Core.u.Phys.uCachePolicy = uCachePolicy;
    TAILQ_INIT(&pMemNetBSD->pglist);
    *ppMem = &pMemNetBSD->Core;
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeLockUser(PPRTR0MEMOBJINTERNAL ppMem, RTR3PTR R3Ptr, size_t cb, uint32_t fAccess, RTR0PROCESS R0Process)
{
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD), RTR0MEMOBJTYPE_LOCK, (void *)R3Ptr, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    int rc = uvm_map_pageable(
            &((struct proc *)R0Process)->p_vmspace->vm_map,
            R3Ptr,
            R3Ptr + cb,
            0, 0);
    if (rc)
    {
        rtR0MemObjDelete(&pMemNetBSD->Core);
        return VERR_NO_MEMORY;
    }

    pMemNetBSD->Core.u.Lock.R0Process = R0Process;
    *ppMem = &pMemNetBSD->Core;
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeLockKernel(PPRTR0MEMOBJINTERNAL ppMem, void *pv, size_t cb, uint32_t fAccess)
{
    /* Kernel memory (always?) wired; all memory allocated by vbox code is? */
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD), RTR0MEMOBJTYPE_LOCK, pv, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    pMemNetBSD->Core.u.Lock.R0Process = NIL_RTR0PROCESS;
    pMemNetBSD->Core.pv = pv;
    *ppMem = &pMemNetBSD->Core;
    return VINF_SUCCESS;
}

DECLHIDDEN(int) rtR0MemObjNativeReserveKernel(PPRTR0MEMOBJINTERNAL ppMem, void *pvFixed, size_t cb, size_t uAlignment)
{
    if (pvFixed != (void *)-1)
    {
        /* can we support this? or can we assume the virtual space is already reserved? */
        printf("reserve specified kernel virtual address not supported\n");
        return VERR_NOT_SUPPORTED;
    }

    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD), RTR0MEMOBJTYPE_RES_VIRT, NULL, cb);
    if (!pMemNetBSD)
        return VERR_NO_MEMORY;

    vaddr_t virt = uvm_km_alloc(kernel_map, cb, uAlignment,
            UVM_KMF_VAONLY | UVM_KMF_WAITVA | UVM_KMF_CANFAIL);
    if (virt == 0)
    {
        rtR0MemObjDelete(&pMemNetBSD->Core);
        return VERR_NO_MEMORY;
    }

    pMemNetBSD->Core.u.ResVirt.R0Process = NIL_RTR0PROCESS;
    pMemNetBSD->Core.pv = (void *)virt;
    *ppMem = &pMemNetBSD->Core;
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeReserveUser(PPRTR0MEMOBJINTERNAL ppMem, RTR3PTR R3PtrFixed, size_t cb, size_t uAlignment, RTR0PROCESS R0Process)
{
    printf("NativeReserveUser\n");
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(int) rtR0MemObjNativeMapKernel(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJ pMemToMap, void *pvFixed, size_t uAlignment,
                                          unsigned fProt, size_t offSub, size_t cbSub)
{
    if (pvFixed != (void *)-1)
    {
        /* can we support this? or can we assume the virtual space is already reserved? */
        printf("map to specified kernel virtual address not supported\n");
        return VERR_NOT_SUPPORTED;
    }

    PRTR0MEMOBJNETBSD pMemNetBSD0 = (PRTR0MEMOBJNETBSD)pMemToMap;
    if ((pMemNetBSD0->Core.enmType != RTR0MEMOBJTYPE_PHYS)
        && (pMemNetBSD0->Core.enmType != RTR0MEMOBJTYPE_PHYS_NC))
    {
        printf("memory to map is not physical\n");
        return VERR_NOT_SUPPORTED;
    }
    size_t sz = cbSub > 0 ? cbSub : pMemNetBSD0->Core.cb;

    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)rtR0MemObjNew(sizeof(*pMemNetBSD), RTR0MEMOBJTYPE_MAPPING, NULL, sz);

    vaddr_t virt = uvm_km_alloc(kernel_map, sz, uAlignment,
            UVM_KMF_VAONLY | UVM_KMF_WAITVA | UVM_KMF_CANFAIL);
    if (virt == 0)
    {
        rtR0MemObjDelete(&pMemNetBSD->Core);
        return VERR_NO_MEMORY;
    }

    vm_prot_t prot = 0;

    if ((fProt & RTMEM_PROT_READ) == RTMEM_PROT_READ)
        prot |= VM_PROT_READ;
    if ((fProt & RTMEM_PROT_WRITE) == RTMEM_PROT_WRITE)
        prot |= VM_PROT_WRITE;
    if ((fProt & RTMEM_PROT_EXEC) == RTMEM_PROT_EXEC)
        prot |= VM_PROT_EXECUTE;

    struct vm_page *page;
    vaddr_t virt2 = virt;
    size_t map_pos = 0;
    TAILQ_FOREACH(page, &pMemNetBSD0->pglist, pageq.queue)
    {
        if (map_pos >= offSub)
        {
            if (cbSub > 0 && (map_pos >= offSub + cbSub))
                break;

            pmap_kenter_pa(virt2, VM_PAGE_TO_PHYS(page), prot, 0);
            virt2 += PAGE_SIZE;
        }
        map_pos += PAGE_SIZE;
    }

    pMemNetBSD->Core.pv = (void *)virt;
    pMemNetBSD->Core.u.Mapping.R0Process = NIL_RTR0PROCESS;
    *ppMem = &pMemNetBSD->Core;

    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeMapUser(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJ pMemToMap, RTR3PTR R3PtrFixed, size_t uAlignment,
                                        unsigned fProt, RTR0PROCESS R0Process)
{
    printf("NativeMapUser\n");
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(int) rtR0MemObjNativeProtect(PRTR0MEMOBJINTERNAL pMem, size_t offSub, size_t cbSub, uint32_t fProt)
{
    vm_prot_t          ProtectionFlags = 0;
    vaddr_t        AddrStart       = (vaddr_t)pMem->pv + offSub;
    vm_map_t           pVmMap          = rtR0MemObjNetBSDGetMap(pMem);

    if (!pVmMap)
        return VERR_NOT_SUPPORTED;

    if ((fProt & RTMEM_PROT_READ) == RTMEM_PROT_READ)
        ProtectionFlags |= UVM_PROT_R;
    if ((fProt & RTMEM_PROT_WRITE) == RTMEM_PROT_WRITE)
        ProtectionFlags |= UVM_PROT_W;
    if ((fProt & RTMEM_PROT_EXEC) == RTMEM_PROT_EXEC)
        ProtectionFlags |= UVM_PROT_X;

    int error = uvm_map_protect(pVmMap, AddrStart, AddrStart + cbSub,
        ProtectionFlags, 0);
    if (!error)
        return VINF_SUCCESS;

    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(RTHCPHYS) rtR0MemObjNativeGetPagePhysAddr(PRTR0MEMOBJINTERNAL pMem, size_t iPage)
{
    PRTR0MEMOBJNETBSD pMemNetBSD = (PRTR0MEMOBJNETBSD)pMem;

    switch (pMemNetBSD->Core.enmType)
    {
        case RTR0MEMOBJTYPE_PAGE:
        case RTR0MEMOBJTYPE_LOW:
        {
            vaddr_t va = (vaddr_t)pMemNetBSD->Core.pv + ptoa(iPage);
            paddr_t pa = 0;
            pmap_extract(pmap_kernel(), va, &pa);
            return pa;
        }
        case RTR0MEMOBJTYPE_CONT:
            return pMemNetBSD->Core.u.Cont.Phys + ptoa(iPage);
        case RTR0MEMOBJTYPE_PHYS:
            return pMemNetBSD->Core.u.Phys.PhysBase + ptoa(iPage);
        case RTR0MEMOBJTYPE_PHYS_NC:
        {
            struct vm_page *page;
            size_t i = 0;
            TAILQ_FOREACH(page, &pMemNetBSD->pglist, pageq.queue)
            {
                if (i == iPage)
                    break;
                i++;
            }
            return VM_PAGE_TO_PHYS(page);
        }
        case RTR0MEMOBJTYPE_LOCK:
        case RTR0MEMOBJTYPE_MAPPING:
        {
            pmap_t pmap;
            if (pMem->u.Lock.R0Process == NIL_RTR0PROCESS)
                pmap = pmap_kernel();
            else
                pmap = ((struct proc *)pMem->u.Lock.R0Process)->p_vmspace->vm_map.pmap;
            vaddr_t va = (vaddr_t)pMemNetBSD->Core.pv + ptoa(iPage);
            paddr_t pa = 0;
            pmap_extract(pmap, va, &pa);
            return pa;
        }
        case RTR0MEMOBJTYPE_RES_VIRT:
            return NIL_RTHCPHYS;
        default:
            return NIL_RTHCPHYS;
    }
}

/* $Id: memobj-r0drv-os2.cpp $ */
/** @file
 * IPRT - Ring-0 Memory Objects, OS/2.
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
 * The OS/2 version of the memory object structure.
 */
typedef struct RTR0MEMOBJDARWIN
{
    /** The core structure. */
    RTR0MEMOBJINTERNAL  Core;
    /** Lock for the ring-3 / ring-0 pinned objectes.
     * This member might not be allocated for some object types. */
    KernVMLock_t        Lock;
    /** Array of physical pages.
     * This array can be 0 in length for some object types. */
    KernPageList_t      aPages[1];
} RTR0MEMOBJOS2, *PRTR0MEMOBJOS2;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void rtR0MemObjFixPageList(KernPageList_t *paPages, ULONG cPages, ULONG cPagesRet);


DECLHIDDEN(int) rtR0MemObjNativeFree(RTR0MEMOBJ pMem)
{
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)pMem;
    int rc;

    switch (pMemOs2->Core.enmType)
    {
        case RTR0MEMOBJTYPE_PHYS_NC:
            AssertMsgFailed(("RTR0MEMOBJTYPE_PHYS_NC\n"));
            return VERR_INTERNAL_ERROR;
            break;

        case RTR0MEMOBJTYPE_PHYS:
            if (!pMemOs2->Core.pv)
                break;

        case RTR0MEMOBJTYPE_MAPPING:
            if (pMemOs2->Core.u.Mapping.R0Process == NIL_RTR0PROCESS)
                break;

            RT_FALL_THRU();
        case RTR0MEMOBJTYPE_PAGE:
        case RTR0MEMOBJTYPE_LOW:
        case RTR0MEMOBJTYPE_CONT:
            rc = KernVMFree(pMemOs2->Core.pv);
            AssertMsg(!rc, ("rc=%d type=%d pv=%p cb=%#zx\n", rc, pMemOs2->Core.enmType, pMemOs2->Core.pv, pMemOs2->Core.cb));
            break;

        case RTR0MEMOBJTYPE_LOCK:
            rc = KernVMUnlock(&pMemOs2->Lock);
            AssertMsg(!rc, ("rc=%d\n", rc));
            break;

        case RTR0MEMOBJTYPE_RES_VIRT:
        default:
            AssertMsgFailed(("enmType=%d\n", pMemOs2->Core.enmType));
            return VERR_INTERNAL_ERROR;
    }

    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPage(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    NOREF(fExecutable);

    /* create the object. */
    const ULONG cPages = cb >> PAGE_SHIFT;
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, aPages[cPages]), RTR0MEMOBJTYPE_PAGE, NULL, cb);
    if (!pMemOs2)
        return VERR_NO_MEMORY;

    /* do the allocation. */
    int rc = KernVMAlloc(cb, VMDHA_FIXED, &pMemOs2->Core.pv, (PPVOID)-1, NULL);
    if (!rc)
    {
        ULONG cPagesRet = cPages;
        rc = KernLinToPageList(pMemOs2->Core.pv, cb, &pMemOs2->aPages[0], &cPagesRet);
        if (!rc)
        {
            rtR0MemObjFixPageList(&pMemOs2->aPages[0], cPages, cPagesRet);
            *ppMem = &pMemOs2->Core;
            return VINF_SUCCESS;
        }
        KernVMFree(pMemOs2->Core.pv);
    }
    rtR0MemObjDelete(&pMemOs2->Core);
    return RTErrConvertFromOS2(rc);
}


DECLHIDDEN(int) rtR0MemObjNativeAllocLow(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    NOREF(fExecutable);

    /* create the object. */
    const ULONG cPages = cb >> PAGE_SHIFT;
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, aPages[cPages]), RTR0MEMOBJTYPE_LOW, NULL, cb);
    if (!pMemOs2)
        return VERR_NO_MEMORY;

    /* do the allocation. */
    int rc = KernVMAlloc(cb, VMDHA_FIXED, &pMemOs2->Core.pv, (PPVOID)-1, NULL);
    if (!rc)
    {
        ULONG cPagesRet = cPages;
        rc = KernLinToPageList(pMemOs2->Core.pv, cb, &pMemOs2->aPages[0], &cPagesRet);
        if (!rc)
        {
            rtR0MemObjFixPageList(&pMemOs2->aPages[0], cPages, cPagesRet);
            *ppMem = &pMemOs2->Core;
            return VINF_SUCCESS;
        }
        KernVMFree(pMemOs2->Core.pv);
    }
    rtR0MemObjDelete(&pMemOs2->Core);
    rc = RTErrConvertFromOS2(rc);
    return rc == VERR_NO_MEMORY ? VERR_NO_LOW_MEMORY : rc;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocCont(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    NOREF(fExecutable);

    /* create the object. */
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, Lock), RTR0MEMOBJTYPE_CONT, NULL, cb);
    if (!pMemOs2)
        return VERR_NO_MEMORY;

    /* do the allocation. */
    ULONG ulPhys = ~0UL;
    int rc = KernVMAlloc(cb, VMDHA_FIXED | VMDHA_CONTIG, &pMemOs2->Core.pv, (PPVOID)&ulPhys, NULL);
    if (!rc)
    {
        Assert(ulPhys != ~0UL);
        pMemOs2->Core.u.Cont.Phys = ulPhys;
        *ppMem = &pMemOs2->Core;
        return VINF_SUCCESS;
    }
    rtR0MemObjDelete(&pMemOs2->Core);
    return RTErrConvertFromOS2(rc);
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPhys(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, RTHCPHYS PhysHighest, size_t uAlignment)
{
    AssertMsgReturn(PhysHighest >= 16 *_1M, ("PhysHigest=%RHp\n", PhysHighest), VERR_NOT_SUPPORTED);

    /** @todo alignment  */
    if (uAlignment != PAGE_SIZE)
        return VERR_NOT_SUPPORTED;

    /* create the object. */
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, Lock), RTR0MEMOBJTYPE_PHYS, NULL, cb);
    if (!pMemOs2)
        return VERR_NO_MEMORY;

    /* do the allocation. */
    ULONG ulPhys = ~0UL;
    int rc = KernVMAlloc(cb, VMDHA_FIXED | VMDHA_CONTIG | (PhysHighest < _4G ? VMDHA_16M : 0), &pMemOs2->Core.pv, (PPVOID)&ulPhys, NULL);
    if (!rc)
    {
        Assert(ulPhys != ~0UL);
        pMemOs2->Core.u.Phys.fAllocated = true;
        pMemOs2->Core.u.Phys.PhysBase = ulPhys;
        *ppMem = &pMemOs2->Core;
        return VINF_SUCCESS;
    }
    rtR0MemObjDelete(&pMemOs2->Core);
    return RTErrConvertFromOS2(rc);
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPhysNC(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, RTHCPHYS PhysHighest)
{
    /** @todo rtR0MemObjNativeAllocPhys / darwin. */
    return rtR0MemObjNativeAllocPhys(ppMem, cb, PhysHighest, PAGE_SIZE);
}


DECLHIDDEN(int) rtR0MemObjNativeEnterPhys(PPRTR0MEMOBJINTERNAL ppMem, RTHCPHYS Phys, size_t cb, uint32_t uCachePolicy)
{
    AssertReturn(uCachePolicy == RTMEM_CACHE_POLICY_DONT_CARE, VERR_NOT_SUPPORTED);

    /* create the object. */
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, Lock), RTR0MEMOBJTYPE_PHYS, NULL, cb);
    if (!pMemOs2)
        return VERR_NO_MEMORY;

    /* there is no allocation here, right? it needs to be mapped somewhere first. */
    pMemOs2->Core.u.Phys.fAllocated = false;
    pMemOs2->Core.u.Phys.PhysBase = Phys;
    pMemOs2->Core.u.Phys.uCachePolicy = uCachePolicy;
    *ppMem = &pMemOs2->Core;
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeLockUser(PPRTR0MEMOBJINTERNAL ppMem, RTR3PTR R3Ptr, size_t cb, uint32_t fAccess,
                                         RTR0PROCESS R0Process)
{
    AssertMsgReturn(R0Process == RTR0ProcHandleSelf(), ("%p != %p\n", R0Process, RTR0ProcHandleSelf()), VERR_NOT_SUPPORTED);

    /* create the object. */
    const ULONG cPages = cb >> PAGE_SHIFT;
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, aPages[cPages]), RTR0MEMOBJTYPE_LOCK, (void *)R3Ptr, cb);
    if (!pMemOs2)
        return VERR_NO_MEMORY;

    /* lock it. */
    ULONG cPagesRet = cPages;
    int rc = KernVMLock(VMDHL_LONG | (fAccess & RTMEM_PROT_WRITE ? VMDHL_WRITE : 0),
                        (void *)R3Ptr, cb, &pMemOs2->Lock, &pMemOs2->aPages[0], &cPagesRet);
    if (!rc)
    {
        rtR0MemObjFixPageList(&pMemOs2->aPages[0], cPages, cPagesRet);
        Assert(cb == pMemOs2->Core.cb);
        Assert(R3Ptr == (RTR3PTR)pMemOs2->Core.pv);
        pMemOs2->Core.u.Lock.R0Process = R0Process;
        *ppMem = &pMemOs2->Core;
        return VINF_SUCCESS;
    }
    rtR0MemObjDelete(&pMemOs2->Core);
    return RTErrConvertFromOS2(rc);
}


DECLHIDDEN(int) rtR0MemObjNativeLockKernel(PPRTR0MEMOBJINTERNAL ppMem, void *pv, size_t cb, uint32_t fAccess)
{
    /* create the object. */
    const ULONG cPages = cb >> PAGE_SHIFT;
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, aPages[cPages]), RTR0MEMOBJTYPE_LOCK, pv, cb);
    if (!pMemOs2)
        return VERR_NO_MEMORY;

    /* lock it. */
    ULONG cPagesRet = cPages;
    int rc = KernVMLock(VMDHL_LONG | (fAccess & RTMEM_PROT_WRITE ? VMDHL_WRITE : 0),
                        pv, cb, &pMemOs2->Lock, &pMemOs2->aPages[0], &cPagesRet);
    if (!rc)
    {
        rtR0MemObjFixPageList(&pMemOs2->aPages[0], cPages, cPagesRet);
        pMemOs2->Core.u.Lock.R0Process = NIL_RTR0PROCESS;
        *ppMem = &pMemOs2->Core;
        return VINF_SUCCESS;
    }
    rtR0MemObjDelete(&pMemOs2->Core);
    return RTErrConvertFromOS2(rc);
}


DECLHIDDEN(int) rtR0MemObjNativeReserveKernel(PPRTR0MEMOBJINTERNAL ppMem, void *pvFixed, size_t cb, size_t uAlignment)
{
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(int) rtR0MemObjNativeReserveUser(PPRTR0MEMOBJINTERNAL ppMem, RTR3PTR R3PtrFixed, size_t cb, size_t uAlignment,
                                            RTR0PROCESS R0Process)
{
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(int) rtR0MemObjNativeMapKernel(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJ pMemToMap, void *pvFixed, size_t uAlignment,
                                          unsigned fProt, size_t offSub, size_t cbSub)
{
    AssertMsgReturn(!offSub && !cbSub, ("%#x %#x\n", offSub, cbSub), VERR_NOT_SUPPORTED);
    AssertMsgReturn(pvFixed == (void *)-1, ("%p\n", pvFixed), VERR_NOT_SUPPORTED);

    /*
     * Check that the specified alignment is supported.
     */
    if (uAlignment > PAGE_SIZE)
        return VERR_NOT_SUPPORTED;


/** @todo finish the implementation. */

    int rc;
    void *pvR0 = NULL;
    PRTR0MEMOBJOS2 pMemToMapOs2 = (PRTR0MEMOBJOS2)pMemToMap;
    switch (pMemToMapOs2->Core.enmType)
    {
        /*
         * These has kernel mappings.
         */
        case RTR0MEMOBJTYPE_PAGE:
        case RTR0MEMOBJTYPE_LOW:
        case RTR0MEMOBJTYPE_CONT:
            pvR0 = pMemToMapOs2->Core.pv;
            break;

        case RTR0MEMOBJTYPE_PHYS:
            pvR0 = pMemToMapOs2->Core.pv;
            if (!pvR0)
            {
                /* no ring-0 mapping, so allocate a mapping in the process. */
                AssertMsgReturn(fProt & RTMEM_PROT_WRITE, ("%#x\n", fProt), VERR_NOT_SUPPORTED);
                Assert(!pMemToMapOs2->Core.u.Phys.fAllocated);
                ULONG ulPhys = pMemToMapOs2->Core.u.Phys.PhysBase;
                rc = KernVMAlloc(pMemToMapOs2->Core.cb, VMDHA_PHYS, &pvR0, (PPVOID)&ulPhys, NULL);
                if (rc)
                    return RTErrConvertFromOS2(rc);
                pMemToMapOs2->Core.pv = pvR0;
            }
            break;

        case RTR0MEMOBJTYPE_PHYS_NC:
            AssertMsgFailed(("RTR0MEMOBJTYPE_PHYS_NC\n"));
            return VERR_INTERNAL_ERROR_3;
            break;

        case RTR0MEMOBJTYPE_LOCK:
            if (pMemToMapOs2->Core.u.Lock.R0Process != NIL_RTR0PROCESS)
                return VERR_NOT_SUPPORTED; /** @todo implement this... */
            pvR0 = pMemToMapOs2->Core.pv;
            break;

        case RTR0MEMOBJTYPE_RES_VIRT:
        case RTR0MEMOBJTYPE_MAPPING:
        default:
            AssertMsgFailed(("enmType=%d\n", pMemToMapOs2->Core.enmType));
            return VERR_INTERNAL_ERROR;
    }

    /*
     * Create a dummy mapping object for it.
     *
     * All mappings are read/write/execute in OS/2 and there isn't
     * any cache options, so sharing is ok. And the main memory object
     * isn't actually freed until all the mappings have been freed up
     * (reference counting).
     */
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, Lock), RTR0MEMOBJTYPE_MAPPING, pvR0, pMemToMapOs2->Core.cb);
    if (pMemOs2)
    {
        pMemOs2->Core.u.Mapping.R0Process = NIL_RTR0PROCESS;
        *ppMem = &pMemOs2->Core;
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}


DECLHIDDEN(int) rtR0MemObjNativeMapUser(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJ pMemToMap, RTR3PTR R3PtrFixed, size_t uAlignment, unsigned fProt, RTR0PROCESS R0Process)
{
    AssertMsgReturn(R0Process == RTR0ProcHandleSelf(), ("%p != %p\n", R0Process, RTR0ProcHandleSelf()), VERR_NOT_SUPPORTED);
    AssertMsgReturn(R3PtrFixed == (RTR3PTR)-1, ("%p\n", R3PtrFixed), VERR_NOT_SUPPORTED);
    if (uAlignment > PAGE_SIZE)
        return VERR_NOT_SUPPORTED;

    int rc;
    void *pvR0;
    void *pvR3 = NULL;
    PRTR0MEMOBJOS2 pMemToMapOs2 = (PRTR0MEMOBJOS2)pMemToMap;
    switch (pMemToMapOs2->Core.enmType)
    {
        /*
         * These has kernel mappings.
         */
        case RTR0MEMOBJTYPE_PAGE:
        case RTR0MEMOBJTYPE_LOW:
        case RTR0MEMOBJTYPE_CONT:
            pvR0 = pMemToMapOs2->Core.pv;
            break;

        case RTR0MEMOBJTYPE_PHYS:
            pvR0 = pMemToMapOs2->Core.pv;
#if 0/* this is wrong. */
            if (!pvR0)
            {
                /* no ring-0 mapping, so allocate a mapping in the process. */
                AssertMsgReturn(fProt & RTMEM_PROT_WRITE, ("%#x\n", fProt), VERR_NOT_SUPPORTED);
                Assert(!pMemToMapOs2->Core.u.Phys.fAllocated);
                ULONG ulPhys = pMemToMapOs2->Core.u.Phys.PhysBase;
                rc = KernVMAlloc(pMemToMapOs2->Core.cb, VMDHA_PHYS | VMDHA_PROCESS, &pvR3, (PPVOID)&ulPhys, NULL);
                if (rc)
                    return RTErrConvertFromOS2(rc);
            }
            break;
#endif
            return VERR_NOT_SUPPORTED;

        case RTR0MEMOBJTYPE_PHYS_NC:
            AssertMsgFailed(("RTR0MEMOBJTYPE_PHYS_NC\n"));
            return VERR_INTERNAL_ERROR_5;
            break;

        case RTR0MEMOBJTYPE_LOCK:
            if (pMemToMapOs2->Core.u.Lock.R0Process != NIL_RTR0PROCESS)
                return VERR_NOT_SUPPORTED; /** @todo implement this... */
            pvR0 = pMemToMapOs2->Core.pv;
            break;

        case RTR0MEMOBJTYPE_RES_VIRT:
        case RTR0MEMOBJTYPE_MAPPING:
        default:
            AssertMsgFailed(("enmType=%d\n", pMemToMapOs2->Core.enmType));
            return VERR_INTERNAL_ERROR;
    }

    /*
     * Map the ring-0 memory into the current process.
     */
    if (!pvR3)
    {
        Assert(pvR0);
        ULONG flFlags = 0;
        if (uAlignment == PAGE_SIZE)
            flFlags |= VMDHGP_4MB;
        if (fProt & RTMEM_PROT_WRITE)
            flFlags |= VMDHGP_WRITE;
        rc = RTR0Os2DHVMGlobalToProcess(flFlags, pvR0, pMemToMapOs2->Core.cb, &pvR3);
        if (rc)
            return RTErrConvertFromOS2(rc);
    }
    Assert(pvR3);

    /*
     * Create a mapping object for it.
     */
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJOS2, Lock), RTR0MEMOBJTYPE_MAPPING, pvR3, pMemToMapOs2->Core.cb);
    if (pMemOs2)
    {
        Assert(pMemOs2->Core.pv == pvR3);
        pMemOs2->Core.u.Mapping.R0Process = R0Process;
        *ppMem = &pMemOs2->Core;
        return VINF_SUCCESS;
    }
    KernVMFree(pvR3);
    return VERR_NO_MEMORY;
}


DECLHIDDEN(int) rtR0MemObjNativeProtect(PRTR0MEMOBJINTERNAL pMem, size_t offSub, size_t cbSub, uint32_t fProt)
{
    NOREF(pMem);
    NOREF(offSub);
    NOREF(cbSub);
    NOREF(fProt);
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(RTHCPHYS) rtR0MemObjNativeGetPagePhysAddr(PRTR0MEMOBJINTERNAL pMem, size_t iPage)
{
    PRTR0MEMOBJOS2 pMemOs2 = (PRTR0MEMOBJOS2)pMem;

    switch (pMemOs2->Core.enmType)
    {
        case RTR0MEMOBJTYPE_PAGE:
        case RTR0MEMOBJTYPE_LOW:
        case RTR0MEMOBJTYPE_LOCK:
        case RTR0MEMOBJTYPE_PHYS_NC:
            return pMemOs2->aPages[iPage].Addr;

        case RTR0MEMOBJTYPE_CONT:
            return pMemOs2->Core.u.Cont.Phys + (iPage << PAGE_SHIFT);

        case RTR0MEMOBJTYPE_PHYS:
            return pMemOs2->Core.u.Phys.PhysBase + (iPage << PAGE_SHIFT);

        case RTR0MEMOBJTYPE_RES_VIRT:
        case RTR0MEMOBJTYPE_MAPPING:
        default:
            return NIL_RTHCPHYS;
    }
}


/**
 * Expands the page list so we can index pages directly.
 *
 * @param   paPages         The page list array to fix.
 * @param   cPages          The number of pages that's supposed to go into the list.
 * @param   cPagesRet       The actual number of pages in the list.
 */
static void rtR0MemObjFixPageList(KernPageList_t *paPages, ULONG cPages, ULONG cPagesRet)
{
    Assert(cPages >= cPagesRet);
    if (cPages != cPagesRet)
    {
        ULONG iIn = cPagesRet;
        ULONG iOut = cPages;
        do
        {
            iIn--;
            iOut--;
            Assert(iIn <= iOut);

            KernPageList_t Page = paPages[iIn];
            Assert(!(Page.Addr & PAGE_OFFSET_MASK));
            Assert(Page.Size == RT_ALIGN_Z(Page.Size, PAGE_SIZE));

            if (Page.Size > PAGE_SIZE)
            {
                do
                {
                    Page.Size -= PAGE_SIZE;
                    paPages[iOut].Addr = Page.Addr + Page.Size;
                    paPages[iOut].Size = PAGE_SIZE;
                    iOut--;
                } while (Page.Size > PAGE_SIZE);
            }

            paPages[iOut].Addr = Page.Addr;
            paPages[iOut].Size = PAGE_SIZE;
        } while (   iIn != iOut
                 && iIn > 0);
    }
}


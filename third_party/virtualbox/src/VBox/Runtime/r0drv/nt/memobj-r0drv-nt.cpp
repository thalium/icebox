/* $Id: memobj-r0drv-nt.cpp $ */
/** @file
 * IPRT - Ring-0 Memory Objects, NT.
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

#include <iprt/memobj.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/log.h>
#include <iprt/param.h>
#include <iprt/string.h>
#include <iprt/process.h>
#include "internal/memobj.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Maximum number of bytes we try to lock down in one go.
 * This is supposed to have a limit right below 256MB, but this appears
 * to actually be much lower. The values here have been determined experimentally.
 */
#ifdef RT_ARCH_X86
# define MAX_LOCK_MEM_SIZE   (32*1024*1024) /* 32MB */
#endif
#ifdef RT_ARCH_AMD64
# define MAX_LOCK_MEM_SIZE   (24*1024*1024) /* 24MB */
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The NT version of the memory object structure.
 */
typedef struct RTR0MEMOBJNT
{
    /** The core structure. */
    RTR0MEMOBJINTERNAL  Core;
#ifndef IPRT_TARGET_NT4
    /** Used MmAllocatePagesForMdl(). */
    bool                fAllocatedPagesForMdl;
#endif
    /** Pointer returned by MmSecureVirtualMemory */
    PVOID               pvSecureMem;
    /** The number of PMDLs (memory descriptor lists) in the array. */
    uint32_t            cMdls;
    /** Array of MDL pointers. (variable size) */
    PMDL                apMdls[1];
} RTR0MEMOBJNT, *PRTR0MEMOBJNT;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Pointer to the MmProtectMdlSystemAddress kernel function if it's available.
 * This API was introduced in XP. */
static decltype(MmProtectMdlSystemAddress) *g_pfnMmProtectMdlSystemAddress = NULL;
/** Set if we've resolved the dynamic APIs. */
static bool volatile g_fResolvedDynamicApis = false;
static ULONG g_uMajorVersion = 5;
static ULONG g_uMinorVersion = 1;


static void rtR0MemObjNtResolveDynamicApis(void)
{
    ULONG uBuildNumber  = 0;
    PsGetVersion(&g_uMajorVersion, &g_uMinorVersion, &uBuildNumber, NULL);

#ifndef IPRT_TARGET_NT4 /* MmGetSystemRoutineAddress was introduced in w2k. */

    UNICODE_STRING RoutineName;
    RtlInitUnicodeString(&RoutineName, L"MmProtectMdlSystemAddress");
    g_pfnMmProtectMdlSystemAddress = (decltype(MmProtectMdlSystemAddress) *)MmGetSystemRoutineAddress(&RoutineName);

#endif
    ASMCompilerBarrier();
    g_fResolvedDynamicApis = true;
}


DECLHIDDEN(int) rtR0MemObjNativeFree(RTR0MEMOBJ pMem)
{
    PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)pMem;

    /*
     * Deal with it on a per type basis (just as a variation).
     */
    switch (pMemNt->Core.enmType)
    {
        case RTR0MEMOBJTYPE_LOW:
#ifndef IPRT_TARGET_NT4
            if (pMemNt->fAllocatedPagesForMdl)
            {
                Assert(pMemNt->Core.pv && pMemNt->cMdls == 1 && pMemNt->apMdls[0]);
                MmUnmapLockedPages(pMemNt->Core.pv, pMemNt->apMdls[0]);
                pMemNt->Core.pv = NULL;
                if (pMemNt->pvSecureMem)
                {
                    MmUnsecureVirtualMemory(pMemNt->pvSecureMem);
                    pMemNt->pvSecureMem = NULL;
                }

                MmFreePagesFromMdl(pMemNt->apMdls[0]);
                ExFreePool(pMemNt->apMdls[0]);
                pMemNt->apMdls[0] = NULL;
                pMemNt->cMdls = 0;
                break;
            }
#endif
            AssertFailed();
            break;

        case RTR0MEMOBJTYPE_PAGE:
            Assert(pMemNt->Core.pv);
            ExFreePool(pMemNt->Core.pv);
            pMemNt->Core.pv = NULL;

            Assert(pMemNt->cMdls == 1 && pMemNt->apMdls[0]);
            IoFreeMdl(pMemNt->apMdls[0]);
            pMemNt->apMdls[0] = NULL;
            pMemNt->cMdls = 0;
            break;

        case RTR0MEMOBJTYPE_CONT:
            Assert(pMemNt->Core.pv);
            MmFreeContiguousMemory(pMemNt->Core.pv);
            pMemNt->Core.pv = NULL;

            Assert(pMemNt->cMdls == 1 && pMemNt->apMdls[0]);
            IoFreeMdl(pMemNt->apMdls[0]);
            pMemNt->apMdls[0] = NULL;
            pMemNt->cMdls = 0;
            break;

        case RTR0MEMOBJTYPE_PHYS:
            /* rtR0MemObjNativeEnterPhys? */
            if (!pMemNt->Core.u.Phys.fAllocated)
            {
#ifndef IPRT_TARGET_NT4
                Assert(!pMemNt->fAllocatedPagesForMdl);
#endif
                /* Nothing to do here. */
                break;
            }
            RT_FALL_THRU();

        case RTR0MEMOBJTYPE_PHYS_NC:
#ifndef IPRT_TARGET_NT4
            if (pMemNt->fAllocatedPagesForMdl)
            {
                MmFreePagesFromMdl(pMemNt->apMdls[0]);
                ExFreePool(pMemNt->apMdls[0]);
                pMemNt->apMdls[0] = NULL;
                pMemNt->cMdls = 0;
                break;
            }
#endif
            AssertFailed();
            break;

        case RTR0MEMOBJTYPE_LOCK:
            if (pMemNt->pvSecureMem)
            {
                MmUnsecureVirtualMemory(pMemNt->pvSecureMem);
                pMemNt->pvSecureMem = NULL;
            }
            for (uint32_t i = 0; i < pMemNt->cMdls; i++)
            {
                MmUnlockPages(pMemNt->apMdls[i]);
                IoFreeMdl(pMemNt->apMdls[i]);
                pMemNt->apMdls[i] = NULL;
            }
            break;

        case RTR0MEMOBJTYPE_RES_VIRT:
/*            if (pMemNt->Core.u.ResVirt.R0Process == NIL_RTR0PROCESS)
            {
            }
            else
            {
            }*/
            AssertMsgFailed(("RTR0MEMOBJTYPE_RES_VIRT\n"));
            return VERR_INTERNAL_ERROR;
            break;

        case RTR0MEMOBJTYPE_MAPPING:
        {
            Assert(pMemNt->cMdls == 0 && pMemNt->Core.pv);
            PRTR0MEMOBJNT pMemNtParent = (PRTR0MEMOBJNT)pMemNt->Core.uRel.Child.pParent;
            Assert(pMemNtParent);
            if (pMemNtParent->cMdls)
            {
                Assert(pMemNtParent->cMdls == 1 && pMemNtParent->apMdls[0]);
                Assert(     pMemNt->Core.u.Mapping.R0Process == NIL_RTR0PROCESS
                       ||   pMemNt->Core.u.Mapping.R0Process == RTR0ProcHandleSelf());
                MmUnmapLockedPages(pMemNt->Core.pv, pMemNtParent->apMdls[0]);
            }
            else
            {
                Assert(     pMemNtParent->Core.enmType == RTR0MEMOBJTYPE_PHYS
                       &&   !pMemNtParent->Core.u.Phys.fAllocated);
                Assert(pMemNt->Core.u.Mapping.R0Process == NIL_RTR0PROCESS);
                MmUnmapIoSpace(pMemNt->Core.pv, pMemNt->Core.cb);
            }
            pMemNt->Core.pv = NULL;
            break;
        }

        default:
            AssertMsgFailed(("enmType=%d\n", pMemNt->Core.enmType));
            return VERR_INTERNAL_ERROR;
    }

    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPage(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    AssertMsgReturn(cb <= _1G, ("%#x\n", cb), VERR_OUT_OF_RANGE); /* for safe size_t -> ULONG */
    RT_NOREF1(fExecutable);

    /*
     * Try allocate the memory and create an MDL for them so
     * we can query the physical addresses and do mappings later
     * without running into out-of-memory conditions and similar problems.
     */
    int rc = VERR_NO_PAGE_MEMORY;
    void *pv = ExAllocatePoolWithTag(NonPagedPool, cb, IPRT_NT_POOL_TAG);
    if (pv)
    {
        PMDL pMdl = IoAllocateMdl(pv, (ULONG)cb, FALSE, FALSE, NULL);
        if (pMdl)
        {
            MmBuildMdlForNonPagedPool(pMdl);
#ifdef RT_ARCH_AMD64
            MmProtectMdlSystemAddress(pMdl, PAGE_EXECUTE_READWRITE);
#endif

            /*
             * Create the IPRT memory object.
             */
            PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_PAGE, pv, cb);
            if (pMemNt)
            {
                pMemNt->cMdls = 1;
                pMemNt->apMdls[0] = pMdl;
                *ppMem = &pMemNt->Core;
                return VINF_SUCCESS;
            }

            rc = VERR_NO_MEMORY;
            IoFreeMdl(pMdl);
        }
        ExFreePool(pv);
    }
    return rc;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocLow(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    AssertMsgReturn(cb <= _1G, ("%#x\n", cb), VERR_OUT_OF_RANGE); /* for safe size_t -> ULONG */

    /*
     * Try see if we get lucky first...
     * (We could probably just assume we're lucky on NT4.)
     */
    int rc = rtR0MemObjNativeAllocPage(ppMem, cb, fExecutable);
    if (RT_SUCCESS(rc))
    {
        size_t iPage = cb >> PAGE_SHIFT;
        while (iPage-- > 0)
            if (rtR0MemObjNativeGetPagePhysAddr(*ppMem, iPage) >= _4G)
            {
                rc = VERR_NO_LOW_MEMORY;
                break;
            }
        if (RT_SUCCESS(rc))
            return rc;

        /* The following ASSUMES that rtR0MemObjNativeAllocPage returns a completed object. */
        RTR0MemObjFree(*ppMem, false);
        *ppMem = NULL;
    }

#ifndef IPRT_TARGET_NT4
    /*
     * Use MmAllocatePagesForMdl to specify the range of physical addresses we wish to use.
     */
    PHYSICAL_ADDRESS Zero;
    Zero.QuadPart = 0;
    PHYSICAL_ADDRESS HighAddr;
    HighAddr.QuadPart = _4G - 1;
    PMDL pMdl = MmAllocatePagesForMdl(Zero, HighAddr, Zero, cb);
    if (pMdl)
    {
        if (MmGetMdlByteCount(pMdl) >= cb)
        {
            __try
            {
                void *pv = MmMapLockedPagesSpecifyCache(pMdl, KernelMode, MmCached, NULL /* no base address */,
                                                        FALSE /* no bug check on failure */, NormalPagePriority);
                if (pv)
                {
                    PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_LOW, pv, cb);
                    if (pMemNt)
                    {
                        pMemNt->fAllocatedPagesForMdl = true;
                        pMemNt->cMdls = 1;
                        pMemNt->apMdls[0] = pMdl;
                        *ppMem = &pMemNt->Core;
                        return VINF_SUCCESS;
                    }
                    MmUnmapLockedPages(pv, pMdl);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
# ifdef LOG_ENABLED
                NTSTATUS rcNt = GetExceptionCode();
                Log(("rtR0MemObjNativeAllocLow: Exception Code %#x\n", rcNt));
# endif
                /* nothing */
            }
        }
        MmFreePagesFromMdl(pMdl);
        ExFreePool(pMdl);
    }
#endif /* !IPRT_TARGET_NT4 */

    /*
     * Fall back on contiguous memory...
     */
    return rtR0MemObjNativeAllocCont(ppMem, cb, fExecutable);
}


/**
 * Internal worker for rtR0MemObjNativeAllocCont(), rtR0MemObjNativeAllocPhys()
 * and rtR0MemObjNativeAllocPhysNC() that takes a max physical address in addition
 * to what rtR0MemObjNativeAllocCont() does.
 *
 * @returns IPRT status code.
 * @param   ppMem           Where to store the pointer to the ring-0 memory object.
 * @param   cb              The size.
 * @param   fExecutable     Whether the mapping should be executable or not.
 * @param   PhysHighest     The highest physical address for the pages in allocation.
 * @param   uAlignment      The alignment of the physical memory to allocate.
 *                          Supported values are PAGE_SIZE, _2M, _4M and _1G.
 */
static int rtR0MemObjNativeAllocContEx(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable, RTHCPHYS PhysHighest,
                                       size_t uAlignment)
{
    AssertMsgReturn(cb <= _1G, ("%#x\n", cb), VERR_OUT_OF_RANGE); /* for safe size_t -> ULONG */
    RT_NOREF1(fExecutable);
#ifdef IPRT_TARGET_NT4
    if (uAlignment != PAGE_SIZE)
        return VERR_NOT_SUPPORTED;
#endif

    /*
     * Allocate the memory and create an MDL for it.
     */
    PHYSICAL_ADDRESS PhysAddrHighest;
    PhysAddrHighest.QuadPart  = PhysHighest;
#ifndef IPRT_TARGET_NT4
    PHYSICAL_ADDRESS PhysAddrLowest, PhysAddrBoundary;
    PhysAddrLowest.QuadPart   = 0;
    PhysAddrBoundary.QuadPart = (uAlignment == PAGE_SIZE) ? 0 : uAlignment;
    void *pv = MmAllocateContiguousMemorySpecifyCache(cb, PhysAddrLowest, PhysAddrHighest, PhysAddrBoundary, MmCached);
#else
    void *pv = MmAllocateContiguousMemory(cb, PhysAddrHighest);
#endif
    if (!pv)
        return VERR_NO_MEMORY;

    PMDL pMdl = IoAllocateMdl(pv, (ULONG)cb, FALSE, FALSE, NULL);
    if (pMdl)
    {
        MmBuildMdlForNonPagedPool(pMdl);
#ifdef RT_ARCH_AMD64
        MmProtectMdlSystemAddress(pMdl, PAGE_EXECUTE_READWRITE);
#endif

        PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_CONT, pv, cb);
        if (pMemNt)
        {
            pMemNt->Core.u.Cont.Phys = (RTHCPHYS)*MmGetMdlPfnArray(pMdl) << PAGE_SHIFT;
            pMemNt->cMdls = 1;
            pMemNt->apMdls[0] = pMdl;
            *ppMem = &pMemNt->Core;
            return VINF_SUCCESS;
        }

        IoFreeMdl(pMdl);
    }
    MmFreeContiguousMemory(pv);
    return VERR_NO_MEMORY;
}


DECLHIDDEN(int) rtR0MemObjNativeAllocCont(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, bool fExecutable)
{
    return rtR0MemObjNativeAllocContEx(ppMem, cb, fExecutable, _4G-1, PAGE_SIZE /* alignment */);
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPhys(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, RTHCPHYS PhysHighest, size_t uAlignment)
{
#ifndef IPRT_TARGET_NT4
    /*
     * Try and see if we're lucky and get a contiguous chunk from MmAllocatePagesForMdl.
     *
     * This is preferable to using MmAllocateContiguousMemory because there are
     * a few situations where the memory shouldn't be mapped, like for instance
     * VT-x control memory. Since these are rather small allocations (one or
     * two pages) MmAllocatePagesForMdl will probably be able to satisfy the
     * request.
     *
     * If the allocation is big, the chances are *probably* not very good. The
     * current limit is kind of random...
     */
    if (   cb < _128K
        && uAlignment == PAGE_SIZE)

    {
        PHYSICAL_ADDRESS Zero;
        Zero.QuadPart = 0;
        PHYSICAL_ADDRESS HighAddr;
        HighAddr.QuadPart = PhysHighest == NIL_RTHCPHYS ? MAXLONGLONG : PhysHighest;
        PMDL pMdl = MmAllocatePagesForMdl(Zero, HighAddr, Zero, cb);
        if (pMdl)
        {
            if (MmGetMdlByteCount(pMdl) >= cb)
            {
                PPFN_NUMBER paPfns = MmGetMdlPfnArray(pMdl);
                PFN_NUMBER Pfn = paPfns[0] + 1;
                const size_t cPages = cb >> PAGE_SHIFT;
                size_t iPage;
                for (iPage = 1; iPage < cPages; iPage++, Pfn++)
                    if (paPfns[iPage] != Pfn)
                        break;
                if (iPage >= cPages)
                {
                    PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_PHYS, NULL, cb);
                    if (pMemNt)
                    {
                        pMemNt->Core.u.Phys.fAllocated = true;
                        pMemNt->Core.u.Phys.PhysBase = (RTHCPHYS)paPfns[0] << PAGE_SHIFT;
                        pMemNt->fAllocatedPagesForMdl = true;
                        pMemNt->cMdls = 1;
                        pMemNt->apMdls[0] = pMdl;
                        *ppMem = &pMemNt->Core;
                        return VINF_SUCCESS;
                    }
                }
            }
            MmFreePagesFromMdl(pMdl);
            ExFreePool(pMdl);
        }
    }
#endif /* !IPRT_TARGET_NT4 */

    return rtR0MemObjNativeAllocContEx(ppMem, cb, false, PhysHighest, uAlignment);
}


DECLHIDDEN(int) rtR0MemObjNativeAllocPhysNC(PPRTR0MEMOBJINTERNAL ppMem, size_t cb, RTHCPHYS PhysHighest)
{
#ifndef IPRT_TARGET_NT4
    PHYSICAL_ADDRESS Zero;
    Zero.QuadPart = 0;
    PHYSICAL_ADDRESS HighAddr;
    HighAddr.QuadPart = PhysHighest == NIL_RTHCPHYS ? MAXLONGLONG : PhysHighest;
    PMDL pMdl = MmAllocatePagesForMdl(Zero, HighAddr, Zero, cb);
    if (pMdl)
    {
        if (MmGetMdlByteCount(pMdl) >= cb)
        {
            PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_PHYS_NC, NULL, cb);
            if (pMemNt)
            {
                pMemNt->fAllocatedPagesForMdl = true;
                pMemNt->cMdls = 1;
                pMemNt->apMdls[0] = pMdl;
                *ppMem = &pMemNt->Core;
                return VINF_SUCCESS;
            }
        }
        MmFreePagesFromMdl(pMdl);
        ExFreePool(pMdl);
    }
    return VERR_NO_MEMORY;
#else   /* IPRT_TARGET_NT4 */
    RT_NOREF(ppMem, cb, PhysHighest);
    return VERR_NOT_SUPPORTED;
#endif  /* IPRT_TARGET_NT4 */
}


DECLHIDDEN(int) rtR0MemObjNativeEnterPhys(PPRTR0MEMOBJINTERNAL ppMem, RTHCPHYS Phys, size_t cb, uint32_t uCachePolicy)
{
    AssertReturn(uCachePolicy == RTMEM_CACHE_POLICY_DONT_CARE || uCachePolicy == RTMEM_CACHE_POLICY_MMIO, VERR_NOT_SUPPORTED);

    /*
     * Validate the address range and create a descriptor for it.
     */
    PFN_NUMBER Pfn = (PFN_NUMBER)(Phys >> PAGE_SHIFT);
    if (((RTHCPHYS)Pfn << PAGE_SHIFT) != Phys)
        return VERR_ADDRESS_TOO_BIG;

    /*
     * Create the IPRT memory object.
     */
    PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_PHYS, NULL, cb);
    if (pMemNt)
    {
        pMemNt->Core.u.Phys.PhysBase = Phys;
        pMemNt->Core.u.Phys.fAllocated = false;
        pMemNt->Core.u.Phys.uCachePolicy = uCachePolicy;
        *ppMem = &pMemNt->Core;
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}


/**
 * Internal worker for locking down pages.
 *
 * @return IPRT status code.
 *
 * @param   ppMem           Where to store the memory object pointer.
 * @param   pv              First page.
 * @param   cb              Number of bytes.
 * @param   fAccess         The desired access, a combination of RTMEM_PROT_READ
 *                          and RTMEM_PROT_WRITE.
 * @param   R0Process       The process \a pv and \a cb refers to.
 */
static int rtR0MemObjNtLock(PPRTR0MEMOBJINTERNAL ppMem, void *pv, size_t cb, uint32_t fAccess, RTR0PROCESS R0Process)
{
    /*
     * Calc the number of MDLs we need and allocate the memory object structure.
     */
    size_t cMdls = cb / MAX_LOCK_MEM_SIZE;
    if (cb % MAX_LOCK_MEM_SIZE)
        cMdls++;
    if (cMdls >= UINT32_MAX)
        return VERR_OUT_OF_RANGE;
    PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(RT_OFFSETOF(RTR0MEMOBJNT, apMdls[cMdls]),
                                                        RTR0MEMOBJTYPE_LOCK, pv, cb);
    if (!pMemNt)
        return VERR_NO_MEMORY;

    /*
     * Loop locking down the sub parts of the memory.
     */
    int         rc = VINF_SUCCESS;
    size_t      cbTotal = 0;
    uint8_t    *pb = (uint8_t *)pv;
    uint32_t    iMdl;
    for (iMdl = 0; iMdl < cMdls; iMdl++)
    {
        /*
         * Calc the Mdl size and allocate it.
         */
        size_t cbCur = cb - cbTotal;
        if (cbCur > MAX_LOCK_MEM_SIZE)
            cbCur = MAX_LOCK_MEM_SIZE;
        AssertMsg(cbCur, ("cbCur: 0!\n"));
        PMDL pMdl = IoAllocateMdl(pb, (ULONG)cbCur, FALSE, FALSE, NULL);
        if (!pMdl)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        /*
         * Lock the pages.
         */
        __try
        {
            MmProbeAndLockPages(pMdl,
                                R0Process == NIL_RTR0PROCESS ? KernelMode : UserMode,
                                fAccess == RTMEM_PROT_READ
                                ? IoReadAccess
                                : fAccess == RTMEM_PROT_WRITE
                                ? IoWriteAccess
                                : IoModifyAccess);

            pMemNt->apMdls[iMdl] = pMdl;
            pMemNt->cMdls++;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            IoFreeMdl(pMdl);
            rc = VERR_LOCK_FAILED;
            break;
        }

        if (R0Process != NIL_RTR0PROCESS)
        {
            /* Make sure the user process can't change the allocation. */
            pMemNt->pvSecureMem = MmSecureVirtualMemory(pv, cb,
                                                        fAccess & RTMEM_PROT_WRITE
                                                        ? PAGE_READWRITE
                                                        : PAGE_READONLY);
            if (!pMemNt->pvSecureMem)
            {
                rc = VERR_NO_MEMORY;
                break;
            }
        }

        /* next */
        cbTotal += cbCur;
        pb      += cbCur;
    }
    if (RT_SUCCESS(rc))
    {
        Assert(pMemNt->cMdls == cMdls);
        pMemNt->Core.u.Lock.R0Process = R0Process;
        *ppMem = &pMemNt->Core;
        return rc;
    }

    /*
     * We failed, perform cleanups.
     */
    while (iMdl-- > 0)
    {
        MmUnlockPages(pMemNt->apMdls[iMdl]);
        IoFreeMdl(pMemNt->apMdls[iMdl]);
        pMemNt->apMdls[iMdl] = NULL;
    }
    if (pMemNt->pvSecureMem)
    {
        MmUnsecureVirtualMemory(pMemNt->pvSecureMem);
        pMemNt->pvSecureMem = NULL;
    }

    rtR0MemObjDelete(&pMemNt->Core);
    return rc;
}


DECLHIDDEN(int) rtR0MemObjNativeLockUser(PPRTR0MEMOBJINTERNAL ppMem, RTR3PTR R3Ptr, size_t cb, uint32_t fAccess,
                                         RTR0PROCESS R0Process)
{
    AssertMsgReturn(R0Process == RTR0ProcHandleSelf(), ("%p != %p\n", R0Process, RTR0ProcHandleSelf()), VERR_NOT_SUPPORTED);
    /* (Can use MmProbeAndLockProcessPages if we need to mess with other processes later.) */
    return rtR0MemObjNtLock(ppMem, (void *)R3Ptr, cb, fAccess, R0Process);
}


DECLHIDDEN(int) rtR0MemObjNativeLockKernel(PPRTR0MEMOBJINTERNAL ppMem, void *pv, size_t cb, uint32_t fAccess)
{
    return rtR0MemObjNtLock(ppMem, pv, cb, fAccess, NIL_RTR0PROCESS);
}


DECLHIDDEN(int) rtR0MemObjNativeReserveKernel(PPRTR0MEMOBJINTERNAL ppMem, void *pvFixed, size_t cb, size_t uAlignment)
{
    /*
     * MmCreateSection(SEC_RESERVE) + MmMapViewInSystemSpace perhaps?
     */
    RT_NOREF4(ppMem, pvFixed, cb, uAlignment);
    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(int) rtR0MemObjNativeReserveUser(PPRTR0MEMOBJINTERNAL ppMem, RTR3PTR R3PtrFixed, size_t cb, size_t uAlignment,
                                            RTR0PROCESS R0Process)
{
    /*
     * ZeCreateSection(SEC_RESERVE) + ZwMapViewOfSection perhaps?
     */
    RT_NOREF5(ppMem, R3PtrFixed, cb, uAlignment, R0Process);
    return VERR_NOT_SUPPORTED;
}


/**
 * Internal worker for rtR0MemObjNativeMapKernel and rtR0MemObjNativeMapUser.
 *
 * @returns IPRT status code.
 * @param   ppMem       Where to store the memory object for the mapping.
 * @param   pMemToMap   The memory object to map.
 * @param   pvFixed     Where to map it. (void *)-1 if anywhere is fine.
 * @param   uAlignment  The alignment requirement for the mapping.
 * @param   fProt       The desired page protection for the mapping.
 * @param   R0Process   If NIL_RTR0PROCESS map into system (kernel) memory.
 *                      If not nil, it's the current process.
 */
static int rtR0MemObjNtMap(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJ pMemToMap, void *pvFixed, size_t uAlignment,
                           unsigned fProt, RTR0PROCESS R0Process)
{
    int rc = VERR_MAP_FAILED;

    /*
     * Check that the specified alignment is supported.
     */
    if (uAlignment > PAGE_SIZE)
        return VERR_NOT_SUPPORTED;

    /*
     * There are two basic cases here, either we've got an MDL and can
     * map it using MmMapLockedPages, or we've got a contiguous physical
     * range (MMIO most likely) and can use MmMapIoSpace.
     */
    PRTR0MEMOBJNT pMemNtToMap = (PRTR0MEMOBJNT)pMemToMap;
    if (pMemNtToMap->cMdls)
    {
        /* don't attempt map locked regions with more than one mdl. */
        if (pMemNtToMap->cMdls != 1)
            return VERR_NOT_SUPPORTED;

#ifdef IPRT_TARGET_NT4
        /* NT SP0 can't map to a specific address. */
        if (pvFixed != (void *)-1)
            return VERR_NOT_SUPPORTED;
#endif

        /* we can't map anything to the first page, sorry. */
        if (pvFixed == 0)
            return VERR_NOT_SUPPORTED;

        /* only one system mapping for now - no time to figure out MDL restrictions right now. */
        if (    pMemNtToMap->Core.uRel.Parent.cMappings
            &&  R0Process == NIL_RTR0PROCESS)
            return VERR_NOT_SUPPORTED;

        __try
        {
            /** @todo uAlignment */
            /** @todo How to set the protection on the pages? */
#ifdef IPRT_TARGET_NT4
            void *pv = MmMapLockedPages(pMemNtToMap->apMdls[0],
                                        R0Process == NIL_RTR0PROCESS ? KernelMode : UserMode);
#else
            void *pv = MmMapLockedPagesSpecifyCache(pMemNtToMap->apMdls[0],
                                                    R0Process == NIL_RTR0PROCESS ? KernelMode : UserMode,
                                                    MmCached,
                                                    pvFixed != (void *)-1 ? pvFixed : NULL,
                                                    FALSE /* no bug check on failure */,
                                                    NormalPagePriority);
#endif
            if (pv)
            {
                NOREF(fProt);

                PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_MAPPING, pv,
                                                                    pMemNtToMap->Core.cb);
                if (pMemNt)
                {
                    pMemNt->Core.u.Mapping.R0Process = R0Process;
                    *ppMem = &pMemNt->Core;
                    return VINF_SUCCESS;
                }

                rc = VERR_NO_MEMORY;
                MmUnmapLockedPages(pv, pMemNtToMap->apMdls[0]);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
#ifdef LOG_ENABLED
            NTSTATUS rcNt = GetExceptionCode();
            Log(("rtR0MemObjNtMap: Exception Code %#x\n", rcNt));
#endif

            /* nothing */
            rc = VERR_MAP_FAILED;
        }

    }
    else
    {
        AssertReturn(   pMemNtToMap->Core.enmType == RTR0MEMOBJTYPE_PHYS
                     && !pMemNtToMap->Core.u.Phys.fAllocated, VERR_INTERNAL_ERROR);

        /* cannot map phys mem to user space (yet). */
        if (R0Process != NIL_RTR0PROCESS)
            return VERR_NOT_SUPPORTED;

        /** @todo uAlignment */
        /** @todo How to set the protection on the pages? */
        PHYSICAL_ADDRESS Phys;
        Phys.QuadPart = pMemNtToMap->Core.u.Phys.PhysBase;
        void *pv = MmMapIoSpace(Phys, pMemNtToMap->Core.cb,
                                pMemNtToMap->Core.u.Phys.uCachePolicy == RTMEM_CACHE_POLICY_MMIO ? MmNonCached : MmCached);
        if (pv)
        {
            PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)rtR0MemObjNew(sizeof(*pMemNt), RTR0MEMOBJTYPE_MAPPING, pv,
                                                                pMemNtToMap->Core.cb);
            if (pMemNt)
            {
                pMemNt->Core.u.Mapping.R0Process = R0Process;
                *ppMem = &pMemNt->Core;
                return VINF_SUCCESS;
            }

            rc = VERR_NO_MEMORY;
            MmUnmapIoSpace(pv, pMemNtToMap->Core.cb);
        }
    }

    NOREF(uAlignment); NOREF(fProt);
    return rc;
}


DECLHIDDEN(int) rtR0MemObjNativeMapKernel(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJ pMemToMap, void *pvFixed, size_t uAlignment,
                              unsigned fProt, size_t offSub, size_t cbSub)
{
    AssertMsgReturn(!offSub && !cbSub, ("%#x %#x\n", offSub, cbSub), VERR_NOT_SUPPORTED);
    return rtR0MemObjNtMap(ppMem, pMemToMap, pvFixed, uAlignment, fProt, NIL_RTR0PROCESS);
}


DECLHIDDEN(int) rtR0MemObjNativeMapUser(PPRTR0MEMOBJINTERNAL ppMem, RTR0MEMOBJ pMemToMap, RTR3PTR R3PtrFixed, size_t uAlignment, unsigned fProt, RTR0PROCESS R0Process)
{
    AssertReturn(R0Process == RTR0ProcHandleSelf(), VERR_NOT_SUPPORTED);
    return rtR0MemObjNtMap(ppMem, pMemToMap, (void *)R3PtrFixed, uAlignment, fProt, R0Process);
}


DECLHIDDEN(int) rtR0MemObjNativeProtect(PRTR0MEMOBJINTERNAL pMem, size_t offSub, size_t cbSub, uint32_t fProt)
{
#if 0
    PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)pMem;
#endif
    if (!g_fResolvedDynamicApis)
        rtR0MemObjNtResolveDynamicApis();

    /*
     * Seems there are some issues with this MmProtectMdlSystemAddress API, so
     * this code isn't currently enabled until we've tested it with the verifier.
     */
#if 0
    /*
     * The API we've got requires a kernel mapping.
     */
    if (   pMemNt->cMdls
        && g_pfnMmProtectMdlSystemAddress
        && (g_uMajorVersion > 6 || (g_uMajorVersion == 6 && g_uMinorVersion >= 1)) /* Windows 7 and later. */
        && pMemNt->Core.pv != NULL
        && (   pMemNt->Core.enmType == RTR0MEMOBJTYPE_PAGE
            || pMemNt->Core.enmType == RTR0MEMOBJTYPE_LOW
            || pMemNt->Core.enmType == RTR0MEMOBJTYPE_CONT
            || (   pMemNt->Core.enmType             == RTR0MEMOBJTYPE_LOCK
                && pMemNt->Core.u.Lock.R0Process    == NIL_RTPROCESS)
            || (   pMemNt->Core.enmType             == RTR0MEMOBJTYPE_MAPPING
                && pMemNt->Core.u.Mapping.R0Process == NIL_RTPROCESS) ) )
    {
        /* Convert the protection. */
        LOCK_OPERATION enmLockOp;
        ULONG fAccess;
        switch (fProt)
        {
            case RTMEM_PROT_NONE:
                fAccess = PAGE_NOACCESS;
                enmLockOp = IoReadAccess;
                break;
            case RTMEM_PROT_READ:
                fAccess = PAGE_READONLY;
                enmLockOp = IoReadAccess;
                break;
            case RTMEM_PROT_WRITE:
            case RTMEM_PROT_WRITE | RTMEM_PROT_READ:
                fAccess = PAGE_READWRITE;
                enmLockOp = IoModifyAccess;
                break;
            case RTMEM_PROT_EXEC:
                fAccess = PAGE_EXECUTE;
                enmLockOp = IoReadAccess;
                break;
            case RTMEM_PROT_EXEC | RTMEM_PROT_READ:
                fAccess = PAGE_EXECUTE_READ;
                enmLockOp = IoReadAccess;
                break;
            case RTMEM_PROT_EXEC | RTMEM_PROT_WRITE:
            case RTMEM_PROT_EXEC | RTMEM_PROT_WRITE | RTMEM_PROT_READ:
                fAccess = PAGE_EXECUTE_READWRITE;
                enmLockOp = IoModifyAccess;
                break;
            default:
                AssertFailedReturn(VERR_INVALID_FLAGS);
        }

        NTSTATUS rcNt = STATUS_SUCCESS;
# if 0 /** @todo test this against the verifier. */
        if (offSub == 0 && pMemNt->Core.cb == cbSub)
        {
            uint32_t iMdl = pMemNt->cMdls;
            while (iMdl-- > 0)
            {
                rcNt = g_pfnMmProtectMdlSystemAddress(pMemNt->apMdls[i], fAccess);
                if (!NT_SUCCESS(rcNt))
                    break;
            }
        }
        else
# endif
        {
            /*
             * We ASSUME the following here:
             *   - MmProtectMdlSystemAddress can deal with nonpaged pool memory
             *   - MmProtectMdlSystemAddress doesn't actually store anything in the MDL we pass it.
             *   - We are not required to call MmProtectMdlSystemAddress with PAGE_READWRITE for the
             *     exact same ranges prior to freeing them.
             *
             * So, we lock the pages temporarily, call the API and unlock them.
             */
            uint8_t *pbCur = (uint8_t *)pMemNt->Core.pv + offSub;
            while (cbSub > 0 && NT_SUCCESS(rcNt))
            {
                size_t cbCur = cbSub;
                if (cbCur > MAX_LOCK_MEM_SIZE)
                    cbCur = MAX_LOCK_MEM_SIZE;
                PMDL pMdl = IoAllocateMdl(pbCur, (ULONG)cbCur, FALSE, FALSE, NULL);
                if (pMdl)
                {
                    __try
                    {
                        MmProbeAndLockPages(pMdl, KernelMode, enmLockOp);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        rcNt = GetExceptionCode();
                    }
                    if (NT_SUCCESS(rcNt))
                    {
                        rcNt = g_pfnMmProtectMdlSystemAddress(pMdl, fAccess);
                        MmUnlockPages(pMdl);
                    }
                    IoFreeMdl(pMdl);
                }
                else
                    rcNt = STATUS_NO_MEMORY;
                pbCur += cbCur;
                cbSub -= cbCur;
            }
        }

        if (NT_SUCCESS(rcNt))
            return VINF_SUCCESS;
        return RTErrConvertFromNtStatus(rcNt);
    }
#else
    RT_NOREF4(pMem, offSub, cbSub, fProt);
#endif

    return VERR_NOT_SUPPORTED;
}


DECLHIDDEN(RTHCPHYS) rtR0MemObjNativeGetPagePhysAddr(PRTR0MEMOBJINTERNAL pMem, size_t iPage)
{
    PRTR0MEMOBJNT pMemNt = (PRTR0MEMOBJNT)pMem;

    if (pMemNt->cMdls)
    {
        if (pMemNt->cMdls == 1)
        {
            PPFN_NUMBER paPfns = MmGetMdlPfnArray(pMemNt->apMdls[0]);
            return (RTHCPHYS)paPfns[iPage] << PAGE_SHIFT;
        }

        size_t iMdl = iPage / (MAX_LOCK_MEM_SIZE >> PAGE_SHIFT);
        size_t iMdlPfn = iPage % (MAX_LOCK_MEM_SIZE >> PAGE_SHIFT);
        PPFN_NUMBER paPfns = MmGetMdlPfnArray(pMemNt->apMdls[iMdl]);
        return (RTHCPHYS)paPfns[iMdlPfn] << PAGE_SHIFT;
    }

    switch (pMemNt->Core.enmType)
    {
        case RTR0MEMOBJTYPE_MAPPING:
            return rtR0MemObjNativeGetPagePhysAddr(pMemNt->Core.uRel.Child.pParent, iPage);

        case RTR0MEMOBJTYPE_PHYS:
            return pMemNt->Core.u.Phys.PhysBase + (iPage << PAGE_SHIFT);

        case RTR0MEMOBJTYPE_PAGE:
        case RTR0MEMOBJTYPE_PHYS_NC:
        case RTR0MEMOBJTYPE_LOW:
        case RTR0MEMOBJTYPE_CONT:
        case RTR0MEMOBJTYPE_LOCK:
        default:
            AssertMsgFailed(("%d\n", pMemNt->Core.enmType));
        case RTR0MEMOBJTYPE_RES_VIRT:
            return NIL_RTHCPHYS;
    }
}


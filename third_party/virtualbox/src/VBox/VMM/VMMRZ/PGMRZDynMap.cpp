/* $Id: PGMRZDynMap.cpp $ */
/** @file
 * PGM - Page Manager and Monitor, dynamic mapping cache.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_PGM_DYNMAP
#include <VBox/vmm/pgm.h>
#include "PGMInternal.h"
#include <VBox/vmm/vm.h>
#include "PGMInline.h"
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/sup.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#ifndef IN_RC
# include <iprt/cpuset.h>
# include <iprt/mem.h>
# include <iprt/memobj.h>
# include <iprt/mp.h>
# include <iprt/semaphore.h>
# include <iprt/spinlock.h>
#endif
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifdef IN_RING0
/** The max size of the mapping cache (in pages). */
# define PGMR0DYNMAP_MAX_PAGES              ((16*_1M) >> PAGE_SHIFT)
/** The small segment size that is adopted on out-of-memory conditions with a
 * single big segment. */
# define PGMR0DYNMAP_SMALL_SEG_PAGES        128
/** The number of pages we reserve per CPU. */
# define PGMR0DYNMAP_PAGES_PER_CPU          256
/** The minimum number of pages we reserve per CPU.
 * This must be equal or larger than the autoset size. */
# define PGMR0DYNMAP_PAGES_PER_CPU_MIN      64
/** Calcs the overload threshold (safety margin).  Current set at 50%. */
# define PGMR0DYNMAP_CALC_OVERLOAD(cPages)  ((cPages) / 2)
/** The number of guard pages.
 * @remarks Never do tuning of the hashing or whatnot with a strict build! */
# if defined(VBOX_STRICT)
#  define PGMR0DYNMAP_GUARD_PAGES           1
# else
#  define PGMR0DYNMAP_GUARD_PAGES           0
# endif
#endif /* IN_RING0 */
/** The dummy physical address of guard pages. */
#define PGMR0DYNMAP_GUARD_PAGE_HCPHYS       UINT32_C(0x7777feed)
/** The dummy reference count of guard pages. (Must be non-zero.) */
#define PGMR0DYNMAP_GUARD_PAGE_REF_COUNT    INT32_C(0x7777feed)
#if 0
/** Define this to just clear the present bit on guard pages.
 * The alternative is to replace the entire PTE with an bad not-present
 * PTE. Either way, XNU will screw us. :-/ */
# define PGMR0DYNMAP_GUARD_NP
#endif
/** The dummy PTE value for a page. */
#define PGMR0DYNMAP_GUARD_PAGE_LEGACY_PTE   X86_PTE_PG_MASK
/** The dummy PTE value for a page. */
#define PGMR0DYNMAP_GUARD_PAGE_PAE_PTE      UINT64_MAX /*X86_PTE_PAE_PG_MASK*/

#ifdef IN_RING0 /* Note! Assertions causes panics if preemption is disabled,
                 *       disable this to work around that. */
/**
 * Acquire the spinlock.
 * This will declare a temporary variable and expands to two statements!
 */
# define PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis) \
    RTSpinlockAcquire((pThis)->hSpinlock)

/**
 * Releases the spinlock.
 */
# define PGMRZDYNMAP_SPINLOCK_RELEASE(pThis) \
    RTSpinlockRelease((pThis)->hSpinlock)

/**
 * Re-acquires the spinlock.
 */
# define PGMRZDYNMAP_SPINLOCK_REACQUIRE(pThis) \
    RTSpinlockAcquire((pThis)->hSpinlock)
#else
# define PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis)   do { } while (0)
# define PGMRZDYNMAP_SPINLOCK_RELEASE(pThis)   do { } while (0)
# define PGMRZDYNMAP_SPINLOCK_REACQUIRE(pThis) do { } while (0)
#endif


/** Converts a PGMCPUM::AutoSet pointer into a PVMCPU. */
#define PGMRZDYNMAP_SET_2_VMCPU(pSet)       (RT_FROM_MEMBER(pSet, VMCPU, pgm.s.AutoSet))

/** Converts a PGMCPUM::AutoSet pointer into a PVM. */
#define PGMRZDYNMAP_SET_2_VM(pSet)          (PGMRZDYNMAP_SET_2_VMCPU(pSet)->CTX_SUFF(pVM))

/** Converts a PGMCPUM::AutoSet pointer into a PVM. */
#ifdef IN_RC
# define PGMRZDYNMAP_SET_2_DYNMAP(pSet)     (PGMRZDYNMAP_SET_2_VM(pSet)->pgm.s.pRCDynMap)
#else
# define PGMRZDYNMAP_SET_2_DYNMAP(pSet)     (g_pPGMR0DynMap)
#endif

/**
 * Gets the set index of the current CPU.
 *
 * This always returns 0 when in raw-mode context because there is only ever
 * one EMT in that context (at least presently).
 */
#ifdef IN_RC
# define PGMRZDYNMAP_CUR_CPU()              (0)
#else
# define PGMRZDYNMAP_CUR_CPU()              RTMpCurSetIndex()
#endif

/** PGMRZDYNMAP::u32Magic. (Jens Christian Bugge Wesseltoft) */
#define PGMRZDYNMAP_MAGIC                   UINT32_C(0x19640201)


/** Zaps an set entry. */
#define PGMRZDYNMAP_ZAP_ENTRY(pEntry) \
    do \
    { \
        (pEntry)->iPage        = UINT16_MAX; \
        (pEntry)->cRefs        = 0; \
        (pEntry)->cInlinedRefs = 0; \
        (pEntry)->cUnrefs      = 0; \
    } while (0)


/** @def PGMRZDYNMAP_STRICT_RELEASE
 * Define this to force pages to be released and make non-present ASAP after
 * use.  This should not normally be enabled as it is a bit expensive. */
#if 0 || defined(DOXYGEN_RUNNING)
# define PGMRZDYNMAP_STRICT_RELEASE
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
#ifdef IN_RING0
/**
 * Ring-0 dynamic mapping cache segment.
 *
 * The dynamic mapping cache can be extended with additional segments if the
 * load is found to be too high.  This done the next time a VM is created, under
 * the protection of the init mutex.  The arrays is reallocated and the new
 * segment is added to the end of these.  Nothing is rehashed of course, as the
 * indexes / addresses must remain unchanged.
 *
 * This structure is only modified while owning the init mutex or during module
 * init / term.
 */
typedef struct PGMR0DYNMAPSEG
{
    /** Pointer to the next segment. */
    struct PGMR0DYNMAPSEG      *pNext;
    /** The memory object for the virtual address range that we're abusing. */
    RTR0MEMOBJ                  hMemObj;
    /** The start page in the cache. (I.e. index into the arrays.) */
    uint16_t                    iPage;
    /** The number of pages this segment contributes. */
    uint16_t                    cPages;
    /** The number of page tables. */
    uint16_t                    cPTs;
    /** The memory objects for the page tables. */
    RTR0MEMOBJ                  ahMemObjPTs[1];
} PGMR0DYNMAPSEG;
/** Pointer to a ring-0 dynamic mapping cache segment. */
typedef PGMR0DYNMAPSEG *PPGMR0DYNMAPSEG;


/**
 * Ring-0 dynamic mapping cache entry.
 *
 * @sa PGMRZDYNMAPENTRY, PGMRCDYNMAPENTRY.
 */
typedef struct PGMR0DYNMAPENTRY
{
    /** The physical address of the currently mapped page.
     * This is duplicate for three reasons: cache locality, cache policy of the PT
     * mappings and sanity checks. */
    RTHCPHYS                    HCPhys;
    /** Pointer to the page. */
    void                       *pvPage;
    /** The number of references. */
    int32_t volatile            cRefs;
    /** PTE pointer union. */
    union PGMR0DYNMAPENTRY_PPTE
    {
        /** PTE pointer, 32-bit legacy version. */
        PX86PTE                 pLegacy;
        /** PTE pointer, PAE version. */
        PX86PTEPAE              pPae;
        /** PTE pointer, the void version. */
        void                   *pv;
    } uPte;
    /** CPUs that haven't invalidated this entry after it's last update. */
    RTCPUSET                    PendingSet;
} PGMR0DYNMAPENTRY;
/** Pointer a mapping cache entry for the ring-0.
 * @sa PPGMRZDYNMAPENTRY, PPGMRCDYNMAPENTRY, */
typedef PGMR0DYNMAPENTRY *PPGMR0DYNMAPENTRY;


/**
 * Dynamic mapping cache for ring-0.
 *
 * This is initialized during VMMR0 module init but no segments are allocated
 * at that time.  Segments will be added when the first VM is started and
 * removed again when the last VM shuts down, thus avoid consuming memory while
 * dormant. At module termination, the remaining bits will be freed up.
 *
 * @sa PPGMRZDYNMAP, PGMRCDYNMAP.
 */
typedef struct PGMR0DYNMAP
{
    /** The usual magic number / eye catcher (PGMRZDYNMAP_MAGIC). */
    uint32_t                    u32Magic;
    /** Spinlock serializing the normal operation of the cache. */
    RTSPINLOCK                  hSpinlock;
    /** Array for tracking and managing the pages. */
    PPGMR0DYNMAPENTRY           paPages;
    /** The cache size given as a number of pages. */
    uint32_t                    cPages;
    /** Whether it's 32-bit legacy or PAE/AMD64 paging mode. */
    bool                        fLegacyMode;
    /** The current load.
     * This does not include guard pages. */
    uint32_t                    cLoad;
    /** The max load ever.
     * This is maintained to trigger the adding of more mapping space. */
    uint32_t                    cMaxLoad;
    /** Initialization / termination lock. */
    RTSEMFASTMUTEX              hInitLock;
    /** The number of guard pages. */
    uint32_t                    cGuardPages;
    /** The number of users (protected by hInitLock). */
    uint32_t                    cUsers;
    /** Array containing a copy of the original page tables.
     * The entries are either X86PTE or X86PTEPAE according to fLegacyMode. */
    void                       *pvSavedPTEs;
    /** List of segments. */
    PPGMR0DYNMAPSEG             pSegHead;
    /** The paging mode. */
    SUPPAGINGMODE               enmPgMode;
} PGMR0DYNMAP;


/**
 * Paging level data.
 */
typedef struct PGMR0DYNMAPPGLVL
{
    uint32_t            cLevels;    /**< The number of levels. */
    struct
    {
        RTHCPHYS        HCPhys;     /**< The address of the page for the current level,
                                     *  i.e. what hMemObj/hMapObj is currently mapping. */
        RTHCPHYS        fPhysMask;  /**< Mask for extracting HCPhys from uEntry. */
        RTR0MEMOBJ      hMemObj;    /**< Memory object for HCPhys, PAGE_SIZE. */
        RTR0MEMOBJ      hMapObj;    /**< Mapping object for hMemObj. */
        uint32_t        fPtrShift;  /**< The pointer shift count. */
        uint64_t        fPtrMask;   /**< The mask to apply to the shifted pointer to get the table index. */
        uint64_t        fAndMask;   /**< And mask to check entry flags. */
        uint64_t        fResMask;   /**< The result from applying fAndMask. */
        union
        {
            void        *pv;        /**< hMapObj address. */
            PX86PGUINT   paLegacy;  /**< Legacy table view. */
            PX86PGPAEUINT paPae;    /**< PAE/AMD64 table view. */
        } u;
    } a[4];
} PGMR0DYNMAPPGLVL;
/** Pointer to paging level data. */
typedef PGMR0DYNMAPPGLVL *PPGMR0DYNMAPPGLVL;
#endif

/** Mapping cache entry for the current context.
 * @sa PGMR0DYNMAPENTRY, PGMRCDYNMAPENTRY */
typedef CTX_MID(PGM,DYNMAPENTRY) PGMRZDYNMAPENTRY;
/** Pointer a mapping cache entry for the current context.
 * @sa PGMR0DYNMAPENTRY, PGMRCDYNMAPENTRY */
typedef PGMRZDYNMAPENTRY *PPGMRZDYNMAPENTRY;

/** Pointer to the mapping cache instance for the current context.
 * @sa PGMR0DYNMAP, PGMRCDYNMAP */
typedef CTX_MID(PGM,DYNMAP) *PPGMRZDYNMAP;



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef IN_RING0
/** Pointer to the ring-0 dynamic mapping cache. */
static PGMR0DYNMAP *g_pPGMR0DynMap;
#endif
/** For overflow testing. */
static bool         g_fPGMR0DynMapTestRunning = false;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void pgmRZDynMapReleasePage(PPGMRZDYNMAP pThis, uint32_t iPage, uint32_t cRefs);
#ifdef IN_RING0
static int  pgmR0DynMapSetup(PPGMRZDYNMAP pThis);
static int  pgmR0DynMapExpand(PPGMRZDYNMAP pThis);
static void pgmR0DynMapTearDown(PPGMRZDYNMAP pThis);
#endif
#if 0 /*def DEBUG*/
static int  pgmR0DynMapTest(PVM pVM);
#endif


/**
 * Initializes the auto mapping sets for a VM.
 *
 * @returns VINF_SUCCESS on success, VERR_PGM_DYNMAP_IPE on failure.
 * @param   pVM         The cross context VM structure.
 */
static int pgmRZDynMapInitAutoSetsForVM(PVM pVM)
{
    VMCPUID idCpu = pVM->cCpus;
    AssertReturn(idCpu > 0 && idCpu <= VMM_MAX_CPU_COUNT, VERR_PGM_DYNMAP_IPE);
    while (idCpu-- > 0)
    {
        PPGMMAPSET pSet = &pVM->aCpus[idCpu].pgm.s.AutoSet;
        uint32_t j = RT_ELEMENTS(pSet->aEntries);
        while (j-- > 0)
        {
            pSet->aEntries[j].pvPage        = NULL;
            pSet->aEntries[j].HCPhys        = NIL_RTHCPHYS;
            PGMRZDYNMAP_ZAP_ENTRY(&pSet->aEntries[j]);
        }
        pSet->cEntries = PGMMAPSET_CLOSED;
        pSet->iSubset = UINT32_MAX;
        pSet->iCpu = -1;
        memset(&pSet->aiHashTable[0], 0xff, sizeof(pSet->aiHashTable));
    }

    return VINF_SUCCESS;
}


#ifdef IN_RING0

/**
 * Initializes the ring-0 dynamic mapping cache.
 *
 * @returns VBox status code.
 */
VMMR0DECL(int) PGMR0DynMapInit(void)
{
    Assert(!g_pPGMR0DynMap);

    /*
     * Create and initialize the cache instance.
     */
    PPGMRZDYNMAP pThis = (PPGMRZDYNMAP)RTMemAllocZ(sizeof(*pThis));
    AssertLogRelReturn(pThis, VERR_NO_MEMORY);
    int             rc = VINF_SUCCESS;
    pThis->enmPgMode = SUPR0GetPagingMode();
    switch (pThis->enmPgMode)
    {
        case SUPPAGINGMODE_32_BIT:
        case SUPPAGINGMODE_32_BIT_GLOBAL:
            pThis->fLegacyMode = false;
            break;
        case SUPPAGINGMODE_PAE:
        case SUPPAGINGMODE_PAE_GLOBAL:
        case SUPPAGINGMODE_PAE_NX:
        case SUPPAGINGMODE_PAE_GLOBAL_NX:
        case SUPPAGINGMODE_AMD64:
        case SUPPAGINGMODE_AMD64_GLOBAL:
        case SUPPAGINGMODE_AMD64_NX:
        case SUPPAGINGMODE_AMD64_GLOBAL_NX:
            pThis->fLegacyMode = false;
            break;
        default:
            rc = VERR_PGM_DYNMAP_IPE;
            break;
    }
    if (RT_SUCCESS(rc))
    {
        rc = RTSemFastMutexCreate(&pThis->hInitLock);
        if (RT_SUCCESS(rc))
        {
            rc = RTSpinlockCreate(&pThis->hSpinlock, RTSPINLOCK_FLAGS_INTERRUPT_UNSAFE, "PGMR0DynMap");
            if (RT_SUCCESS(rc))
            {
                pThis->u32Magic = PGMRZDYNMAP_MAGIC;
                g_pPGMR0DynMap = pThis;
                return VINF_SUCCESS;
            }
            RTSemFastMutexDestroy(pThis->hInitLock);
        }
    }
    RTMemFree(pThis);
    return rc;
}


/**
 * Terminates the ring-0 dynamic mapping cache.
 */
VMMR0DECL(void) PGMR0DynMapTerm(void)
{
    /*
     * Destroy the cache.
     *
     * There is not supposed to be any races here, the loader should
     * make sure about that. So, don't bother locking anything.
     *
     * The VM objects should all be destroyed by now, so there is no
     * dangling users or anything like that to clean up. This routine
     * is just a mirror image of PGMR0DynMapInit.
     */
    PPGMRZDYNMAP pThis = g_pPGMR0DynMap;
    if (pThis)
    {
        AssertPtr(pThis);
        g_pPGMR0DynMap = NULL;

        /* This should *never* happen, but in case it does try not to leak memory. */
        AssertLogRelMsg(!pThis->cUsers && !pThis->paPages && !pThis->pvSavedPTEs && !pThis->cPages,
                        ("cUsers=%d paPages=%p pvSavedPTEs=%p cPages=%#x\n",
                         pThis->cUsers, pThis->paPages, pThis->pvSavedPTEs, pThis->cPages));
        if (pThis->paPages)
            pgmR0DynMapTearDown(pThis);

        /* Free the associated resources. */
        RTSemFastMutexDestroy(pThis->hInitLock);
        pThis->hInitLock = NIL_RTSEMFASTMUTEX;
        RTSpinlockDestroy(pThis->hSpinlock);
        pThis->hSpinlock = NIL_RTSPINLOCK;
        pThis->u32Magic = UINT32_MAX;
        RTMemFree(pThis);
    }
}


/**
 * Initializes the dynamic mapping cache for a new VM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR0DECL(int) PGMR0DynMapInitVM(PVM pVM)
{
    AssertMsgReturn(!pVM->pgm.s.pvR0DynMapUsed, ("%p (pThis=%p)\n", pVM->pgm.s.pvR0DynMapUsed, g_pPGMR0DynMap), VERR_WRONG_ORDER);

    /*
     * Initialize the auto sets.
     */
    int rc = pgmRZDynMapInitAutoSetsForVM(pVM);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Do we need the cache? Skip the last bit if we don't.
     */
    if (!HMIsEnabled(pVM))
        return VINF_SUCCESS;

    /*
     * Reference and if necessary setup or expand the cache.
     */
    PPGMRZDYNMAP pThis = g_pPGMR0DynMap;
    AssertPtrReturn(pThis, VERR_PGM_DYNMAP_IPE);
    rc = RTSemFastMutexRequest(pThis->hInitLock);
    AssertLogRelRCReturn(rc, rc);

    pThis->cUsers++;
    if (pThis->cUsers == 1)
    {
        rc = pgmR0DynMapSetup(pThis);
#if 0 /*def DEBUG*/
        if (RT_SUCCESS(rc))
        {
            rc = pgmR0DynMapTest(pVM);
            if (RT_FAILURE(rc))
                pgmR0DynMapTearDown(pThis);
        }
#endif
    }
    else if (pThis->cMaxLoad > PGMR0DYNMAP_CALC_OVERLOAD(pThis->cPages - pThis->cGuardPages))
        rc = pgmR0DynMapExpand(pThis);
    if (RT_SUCCESS(rc))
        pVM->pgm.s.pvR0DynMapUsed = pThis;
    else
        pThis->cUsers--;

    RTSemFastMutexRelease(pThis->hInitLock);
    return rc;
}


/**
 * Terminates the dynamic mapping cache usage for a VM.
 *
 * @param   pVM         The cross context VM structure.
 */
VMMR0DECL(void) PGMR0DynMapTermVM(PVM pVM)
{
    /*
     * Return immediately if we're not using the cache.
     */
    if (!pVM->pgm.s.pvR0DynMapUsed)
        return;

    PPGMRZDYNMAP pThis = g_pPGMR0DynMap;
    AssertPtrReturnVoid(pThis);

    int rc = RTSemFastMutexRequest(pThis->hInitLock);
    AssertLogRelRCReturnVoid(rc);

    if (pVM->pgm.s.pvR0DynMapUsed == pThis)
    {
        pVM->pgm.s.pvR0DynMapUsed = NULL;

#ifdef VBOX_STRICT
        PGMR0DynMapAssertIntegrity();
#endif

        /*
         * Clean up and check the auto sets.
         */
        VMCPUID idCpu = pVM->cCpus;
        while (idCpu-- > 0)
        {
            PPGMMAPSET pSet = &pVM->aCpus[idCpu].pgm.s.AutoSet;
            uint32_t j = pSet->cEntries;
            if (j <= RT_ELEMENTS(pSet->aEntries))
            {
                /*
                 * The set is open, close it.
                 */
                while (j-- > 0)
                {
                    int32_t  cRefs = pSet->aEntries[j].cRefs;
                    uint32_t iPage = pSet->aEntries[j].iPage;
                    LogRel(("PGMR0DynMapTermVM: %d dangling refs to %#x\n", cRefs, iPage));
                    if (iPage < pThis->cPages && cRefs > 0)
                        pgmRZDynMapReleasePage(pThis, iPage, cRefs);
                    else
                        AssertLogRelMsgFailed(("cRefs=%d iPage=%#x cPages=%u\n", cRefs, iPage, pThis->cPages));

                    PGMRZDYNMAP_ZAP_ENTRY(&pSet->aEntries[j]);
                }
                pSet->cEntries = PGMMAPSET_CLOSED;
                pSet->iSubset = UINT32_MAX;
                pSet->iCpu = -1;
            }
            else
                AssertMsg(j == PGMMAPSET_CLOSED, ("cEntries=%#x\n", j));

            j = RT_ELEMENTS(pSet->aEntries);
            while (j-- > 0)
            {
                Assert(pSet->aEntries[j].iPage == UINT16_MAX);
                Assert(!pSet->aEntries[j].cRefs);
            }
        }

        /*
         * Release our reference to the mapping cache.
         */
        Assert(pThis->cUsers > 0);
        pThis->cUsers--;
        if (!pThis->cUsers)
            pgmR0DynMapTearDown(pThis);
    }
    else
        AssertLogRelMsgFailed(("pvR0DynMapUsed=%p pThis=%p\n", pVM->pgm.s.pvR0DynMapUsed, pThis));

    RTSemFastMutexRelease(pThis->hInitLock);
}


/**
 * Shoots down the TLBs for all the cache pages, pgmR0DynMapTearDown helper.
 *
 * @param   idCpu           The current CPU.
 * @param   pvUser1         The dynamic mapping cache instance.
 * @param   pvUser2         Unused, NULL.
 */
static DECLCALLBACK(void) pgmR0DynMapShootDownTlbs(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    Assert(!pvUser2);
    PPGMRZDYNMAP        pThis   = (PPGMRZDYNMAP)pvUser1;
    Assert(pThis == g_pPGMR0DynMap);
    PPGMRZDYNMAPENTRY   paPages = pThis->paPages;
    uint32_t            iPage   = pThis->cPages;
    while (iPage-- > 0)
        ASMInvalidatePage((uintptr_t)paPages[iPage].pvPage);
}


/**
 * Shoot down the TLBs for every single cache entry on all CPUs.
 *
 * @returns IPRT status code (RTMpOnAll).
 * @param   pThis       The dynamic mapping cache instance.
 */
static int pgmR0DynMapTlbShootDown(PPGMRZDYNMAP pThis)
{
    int rc = RTMpOnAll(pgmR0DynMapShootDownTlbs, pThis, NULL);
    AssertRC(rc);
    if (RT_FAILURE(rc))
    {
        uint32_t iPage = pThis->cPages;
        while (iPage-- > 0)
            ASMInvalidatePage((uintptr_t)pThis->paPages[iPage].pvPage);
    }
    return rc;
}


/**
 * Calculate the new cache size based on cMaxLoad statistics.
 *
 * @returns Number of pages.
 * @param   pThis       The dynamic mapping cache instance.
 * @param   pcMinPages  The minimal size in pages.
 */
static uint32_t pgmR0DynMapCalcNewSize(PPGMRZDYNMAP pThis, uint32_t *pcMinPages)
{
    Assert(pThis->cPages <= PGMR0DYNMAP_MAX_PAGES);

    /* cCpus * PGMR0DYNMAP_PAGES_PER_CPU(_MIN). */
    RTCPUID     cCpus     = RTMpGetCount();
    AssertReturn(cCpus > 0 && cCpus <= RTCPUSET_MAX_CPUS, 0);
    uint32_t    cPages    = cCpus * PGMR0DYNMAP_PAGES_PER_CPU;
    uint32_t    cMinPages = cCpus * PGMR0DYNMAP_PAGES_PER_CPU_MIN;

    /* adjust against cMaxLoad. */
    AssertMsg(pThis->cMaxLoad <= PGMR0DYNMAP_MAX_PAGES, ("%#x\n", pThis->cMaxLoad));
    if (pThis->cMaxLoad > PGMR0DYNMAP_MAX_PAGES)
        pThis->cMaxLoad = 0;

    while (pThis->cMaxLoad > PGMR0DYNMAP_CALC_OVERLOAD(cPages))
        cPages += PGMR0DYNMAP_PAGES_PER_CPU;

    if (pThis->cMaxLoad > cMinPages)
        cMinPages = pThis->cMaxLoad;

    /* adjust against max and current size. */
    if (cPages < pThis->cPages)
        cPages = pThis->cPages;
    cPages *= PGMR0DYNMAP_GUARD_PAGES + 1;
    if (cPages > PGMR0DYNMAP_MAX_PAGES)
        cPages = PGMR0DYNMAP_MAX_PAGES;

    if (cMinPages < pThis->cPages)
        cMinPages = pThis->cPages;
    cMinPages *= PGMR0DYNMAP_GUARD_PAGES + 1;
    if (cMinPages > PGMR0DYNMAP_MAX_PAGES)
        cMinPages = PGMR0DYNMAP_MAX_PAGES;

    Assert(cMinPages);
    *pcMinPages = cMinPages;
    return cPages;
}


/**
 * Initializes the paging level data.
 *
 * @param   pThis       The dynamic mapping cache instance.
 * @param   pPgLvl      The paging level data.
 */
void pgmR0DynMapPagingArrayInit(PPGMRZDYNMAP pThis, PPGMR0DYNMAPPGLVL pPgLvl)
{
    RTCCUINTREG     cr4 = ASMGetCR4();
    switch (pThis->enmPgMode)
    {
        case SUPPAGINGMODE_32_BIT:
        case SUPPAGINGMODE_32_BIT_GLOBAL:
            pPgLvl->cLevels = 2;
            pPgLvl->a[0].fPhysMask = X86_CR3_PAGE_MASK;
            pPgLvl->a[0].fAndMask  = X86_PDE_P | X86_PDE_RW | (cr4 & X86_CR4_PSE ? X86_PDE_PS : 0);
            pPgLvl->a[0].fResMask  = X86_PDE_P | X86_PDE_RW;
            pPgLvl->a[0].fPtrMask  = X86_PD_MASK;
            pPgLvl->a[0].fPtrShift = X86_PD_SHIFT;

            pPgLvl->a[1].fPhysMask = X86_PDE_PG_MASK;
            pPgLvl->a[1].fAndMask  = X86_PTE_P | X86_PTE_RW;
            pPgLvl->a[1].fResMask  = X86_PTE_P | X86_PTE_RW;
            pPgLvl->a[1].fPtrMask  = X86_PT_MASK;
            pPgLvl->a[1].fPtrShift = X86_PT_SHIFT;
            break;

        case SUPPAGINGMODE_PAE:
        case SUPPAGINGMODE_PAE_GLOBAL:
        case SUPPAGINGMODE_PAE_NX:
        case SUPPAGINGMODE_PAE_GLOBAL_NX:
            pPgLvl->cLevels = 3;
            pPgLvl->a[0].fPhysMask = X86_CR3_PAE_PAGE_MASK;
            pPgLvl->a[0].fPtrMask  = X86_PDPT_MASK_PAE;
            pPgLvl->a[0].fPtrShift = X86_PDPT_SHIFT;
            pPgLvl->a[0].fAndMask  = X86_PDPE_P;
            pPgLvl->a[0].fResMask  = X86_PDPE_P;

            pPgLvl->a[1].fPhysMask = X86_PDPE_PG_MASK;
            pPgLvl->a[1].fPtrMask  = X86_PD_PAE_MASK;
            pPgLvl->a[1].fPtrShift = X86_PD_PAE_SHIFT;
            pPgLvl->a[1].fAndMask  = X86_PDE_P | X86_PDE_RW | (cr4 & X86_CR4_PSE ? X86_PDE_PS : 0);
            pPgLvl->a[1].fResMask  = X86_PDE_P | X86_PDE_RW;

            pPgLvl->a[2].fPhysMask = X86_PDE_PAE_PG_MASK;
            pPgLvl->a[2].fPtrMask  = X86_PT_PAE_MASK;
            pPgLvl->a[2].fPtrShift = X86_PT_PAE_SHIFT;
            pPgLvl->a[2].fAndMask  = X86_PTE_P | X86_PTE_RW;
            pPgLvl->a[2].fResMask  = X86_PTE_P | X86_PTE_RW;
            break;

        case SUPPAGINGMODE_AMD64:
        case SUPPAGINGMODE_AMD64_GLOBAL:
        case SUPPAGINGMODE_AMD64_NX:
        case SUPPAGINGMODE_AMD64_GLOBAL_NX:
            pPgLvl->cLevels = 4;
            pPgLvl->a[0].fPhysMask = X86_CR3_AMD64_PAGE_MASK;
            pPgLvl->a[0].fPtrShift = X86_PML4_SHIFT;
            pPgLvl->a[0].fPtrMask  = X86_PML4_MASK;
            pPgLvl->a[0].fAndMask  = X86_PML4E_P | X86_PML4E_RW;
            pPgLvl->a[0].fResMask  = X86_PML4E_P | X86_PML4E_RW;

            pPgLvl->a[1].fPhysMask = X86_PML4E_PG_MASK;
            pPgLvl->a[1].fPtrShift = X86_PDPT_SHIFT;
            pPgLvl->a[1].fPtrMask  = X86_PDPT_MASK_AMD64;
            pPgLvl->a[1].fAndMask  = X86_PDPE_P | X86_PDPE_RW /** @todo check for X86_PDPT_PS support. */;
            pPgLvl->a[1].fResMask  = X86_PDPE_P | X86_PDPE_RW;

            pPgLvl->a[2].fPhysMask = X86_PDPE_PG_MASK;
            pPgLvl->a[2].fPtrShift = X86_PD_PAE_SHIFT;
            pPgLvl->a[2].fPtrMask  = X86_PD_PAE_MASK;
            pPgLvl->a[2].fAndMask  = X86_PDE_P | X86_PDE_RW | (cr4 & X86_CR4_PSE ? X86_PDE_PS : 0);
            pPgLvl->a[2].fResMask  = X86_PDE_P | X86_PDE_RW;

            pPgLvl->a[3].fPhysMask = X86_PDE_PAE_PG_MASK;
            pPgLvl->a[3].fPtrShift = X86_PT_PAE_SHIFT;
            pPgLvl->a[3].fPtrMask  = X86_PT_PAE_MASK;
            pPgLvl->a[3].fAndMask  = X86_PTE_P | X86_PTE_RW;
            pPgLvl->a[3].fResMask  = X86_PTE_P | X86_PTE_RW;
            break;

        default:
            AssertFailed();
            pPgLvl->cLevels = 0;
            break;
    }

    for (uint32_t i = 0; i < 4; i++) /* ASSUMING array size. */
    {
        pPgLvl->a[i].HCPhys = NIL_RTHCPHYS;
        pPgLvl->a[i].hMapObj = NIL_RTR0MEMOBJ;
        pPgLvl->a[i].hMemObj = NIL_RTR0MEMOBJ;
        pPgLvl->a[i].u.pv = NULL;
    }
}


/**
 * Maps a PTE.
 *
 * This will update the segment structure when new PTs are mapped.
 *
 * It also assumes that we (for paranoid reasons) wish to establish a mapping
 * chain from CR3 to the PT that all corresponds to the processor we're
 * currently running on, and go about this by running with interrupts disabled
 * and restarting from CR3 for every change.
 *
 * @returns VBox status code, VINF_TRY_AGAIN if we changed any mappings and had
 *          to re-enable interrupts.
 * @param   pThis       The dynamic mapping cache instance.
 * @param   pPgLvl      The paging level structure.
 * @param   pvPage      The page.
 * @param   pSeg        The segment.
 * @param   cMaxPTs     The max number of PTs expected in the segment.
 * @param   ppvPTE      Where to store the PTE address.
 */
static int pgmR0DynMapPagingArrayMapPte(PPGMRZDYNMAP pThis, PPGMR0DYNMAPPGLVL pPgLvl, void *pvPage,
                                        PPGMR0DYNMAPSEG pSeg, uint32_t cMaxPTs, void **ppvPTE)
{
    Assert(!(ASMGetFlags() & X86_EFL_IF));
    void           *pvEntry = NULL;
    X86PGPAEUINT    uEntry = ASMGetCR3();
    for (uint32_t i = 0; i < pPgLvl->cLevels; i++)
    {
        RTHCPHYS HCPhys = uEntry & pPgLvl->a[i].fPhysMask;
        if (pPgLvl->a[i].HCPhys != HCPhys)
        {
            /*
             * Need to remap this level.
             * The final level, the PT, will not be freed since that is what it's all about.
             */
            ASMIntEnable();
            if (i + 1 == pPgLvl->cLevels)
                AssertReturn(pSeg->cPTs < cMaxPTs, VERR_PGM_DYNMAP_IPE);
            else
            {
                int rc2 = RTR0MemObjFree(pPgLvl->a[i].hMemObj, true /* fFreeMappings */); AssertRC(rc2);
                pPgLvl->a[i].hMemObj = pPgLvl->a[i].hMapObj = NIL_RTR0MEMOBJ;
            }

            int rc = RTR0MemObjEnterPhys(&pPgLvl->a[i].hMemObj, HCPhys, PAGE_SIZE, RTMEM_CACHE_POLICY_DONT_CARE);
            if (RT_SUCCESS(rc))
            {
                rc = RTR0MemObjMapKernel(&pPgLvl->a[i].hMapObj, pPgLvl->a[i].hMemObj,
                                         (void *)-1 /* pvFixed */, 0 /* cbAlignment */,
                                         RTMEM_PROT_WRITE | RTMEM_PROT_READ);
                if (RT_SUCCESS(rc))
                {
                    pPgLvl->a[i].u.pv   = RTR0MemObjAddress(pPgLvl->a[i].hMapObj);
                    AssertMsg(((uintptr_t)pPgLvl->a[i].u.pv & ~(uintptr_t)PAGE_OFFSET_MASK), ("%p\n", pPgLvl->a[i].u.pv));
                    pPgLvl->a[i].HCPhys = HCPhys;
                    if (i + 1 == pPgLvl->cLevels)
                        pSeg->ahMemObjPTs[pSeg->cPTs++] = pPgLvl->a[i].hMemObj;
                    ASMIntDisable();
                    return VINF_TRY_AGAIN;
                }

                pPgLvl->a[i].hMapObj = NIL_RTR0MEMOBJ;
            }
            else
                pPgLvl->a[i].hMemObj = NIL_RTR0MEMOBJ;
            pPgLvl->a[i].HCPhys = NIL_RTHCPHYS;
            return rc;
        }

        /*
         * The next level.
         */
        uint32_t iEntry = ((uint64_t)(uintptr_t)pvPage >> pPgLvl->a[i].fPtrShift) & pPgLvl->a[i].fPtrMask;
        if (pThis->fLegacyMode)
        {
            pvEntry = &pPgLvl->a[i].u.paLegacy[iEntry];
            uEntry  = pPgLvl->a[i].u.paLegacy[iEntry];
        }
        else
        {
            pvEntry = &pPgLvl->a[i].u.paPae[iEntry];
            uEntry  = pPgLvl->a[i].u.paPae[iEntry];
        }

        if ((uEntry & pPgLvl->a[i].fAndMask) != pPgLvl->a[i].fResMask)
        {
            LogRel(("PGMR0DynMap: internal error - iPgLvl=%u cLevels=%u uEntry=%#llx fAnd=%#llx fRes=%#llx got=%#llx\n"
                    "PGMR0DynMap: pv=%p pvPage=%p iEntry=%#x fLegacyMode=%RTbool\n",
                    i, pPgLvl->cLevels, uEntry, pPgLvl->a[i].fAndMask, pPgLvl->a[i].fResMask, uEntry & pPgLvl->a[i].fAndMask,
                    pPgLvl->a[i].u.pv, pvPage, iEntry, pThis->fLegacyMode));
            return VERR_PGM_DYNMAP_IPE;
        }
        /*Log(("#%d: iEntry=%4d uEntry=%#llx pvEntry=%p HCPhys=%RHp \n", i, iEntry, uEntry, pvEntry, pPgLvl->a[i].HCPhys));*/
    }

    /* made it thru without needing to remap anything. */
    *ppvPTE = pvEntry;
    return VINF_SUCCESS;
}


/**
 * Sets up a guard page.
 *
 * @param   pThis       The dynamic mapping cache instance.
 * @param   pPage       The page.
 */
DECLINLINE(void) pgmR0DynMapSetupGuardPage(PPGMRZDYNMAP pThis, PPGMRZDYNMAPENTRY pPage)
{
    memset(pPage->pvPage, 0xfd, PAGE_SIZE);
    pPage->cRefs  = PGMR0DYNMAP_GUARD_PAGE_REF_COUNT;
    pPage->HCPhys = PGMR0DYNMAP_GUARD_PAGE_HCPHYS;
#ifdef PGMR0DYNMAP_GUARD_NP
    ASMAtomicBitClear(pPage->uPte.pv, X86_PTE_BIT_P);
#else
    if (pThis->fLegacyMode)
        ASMAtomicWriteU32(&pPage->uPte.pLegacy->u, PGMR0DYNMAP_GUARD_PAGE_LEGACY_PTE);
    else
        ASMAtomicWriteU64(&pPage->uPte.pPae->u,    PGMR0DYNMAP_GUARD_PAGE_PAE_PTE);
#endif
    pThis->cGuardPages++;
}


/**
 * Adds a new segment of the specified size.
 *
 * @returns VBox status code.
 * @param   pThis       The dynamic mapping cache instance.
 * @param   cPages      The size of the new segment, give as a page count.
 */
static int pgmR0DynMapAddSeg(PPGMRZDYNMAP pThis, uint32_t cPages)
{
    int rc2;
    AssertReturn(ASMGetFlags() & X86_EFL_IF, VERR_PREEMPT_DISABLED);

    /*
     * Do the array reallocations first.
     * (The pages array has to be replaced behind the spinlock of course.)
     */
    void *pvSavedPTEs = RTMemRealloc(pThis->pvSavedPTEs, (pThis->fLegacyMode ? sizeof(X86PGUINT) : sizeof(X86PGPAEUINT)) * (pThis->cPages + cPages));
    if (!pvSavedPTEs)
        return VERR_NO_MEMORY;
    pThis->pvSavedPTEs = pvSavedPTEs;

    void *pvPages = RTMemAllocZ(sizeof(pThis->paPages[0]) * (pThis->cPages + cPages));
    if (!pvPages)
    {
        pvSavedPTEs = RTMemRealloc(pThis->pvSavedPTEs, (pThis->fLegacyMode ? sizeof(X86PGUINT) : sizeof(X86PGPAEUINT)) * pThis->cPages);
        if (pvSavedPTEs)
            pThis->pvSavedPTEs = pvSavedPTEs;
        return VERR_NO_MEMORY;
    }

    PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis);

    memcpy(pvPages, pThis->paPages, sizeof(pThis->paPages[0]) * pThis->cPages);
    void *pvToFree = pThis->paPages;
    pThis->paPages = (PPGMRZDYNMAPENTRY)pvPages;

    PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
    RTMemFree(pvToFree);

    /*
     * Allocate the segment structure and pages of memory, then touch all the pages (paranoia).
     */
    uint32_t cMaxPTs = cPages / (pThis->fLegacyMode ? X86_PG_ENTRIES : X86_PG_PAE_ENTRIES) + 2;
    PPGMR0DYNMAPSEG pSeg = (PPGMR0DYNMAPSEG)RTMemAllocZ(RT_UOFFSETOF(PGMR0DYNMAPSEG, ahMemObjPTs[cMaxPTs]));
    if (!pSeg)
        return VERR_NO_MEMORY;
    pSeg->pNext  = NULL;
    pSeg->cPages = cPages;
    pSeg->iPage  = pThis->cPages;
    pSeg->cPTs   = 0;
    int rc = RTR0MemObjAllocPage(&pSeg->hMemObj, cPages << PAGE_SHIFT, false);
    if (RT_SUCCESS(rc))
    {
        uint8_t            *pbPage = (uint8_t *)RTR0MemObjAddress(pSeg->hMemObj);
        AssertMsg(VALID_PTR(pbPage) && !((uintptr_t)pbPage & PAGE_OFFSET_MASK), ("%p\n", pbPage));
        memset(pbPage, 0xfe, cPages << PAGE_SHIFT);

        /*
         * Walk thru the pages and set them up with a mapping of their PTE and everything.
         */
        ASMIntDisable();
        PGMR0DYNMAPPGLVL    PgLvl;
        pgmR0DynMapPagingArrayInit(pThis, &PgLvl);
        uint32_t const      iEndPage = pSeg->iPage + cPages;
        for (uint32_t iPage = pSeg->iPage;
             iPage < iEndPage;
             iPage++, pbPage += PAGE_SIZE)
        {
            /* Initialize the page data. */
            pThis->paPages[iPage].HCPhys = NIL_RTHCPHYS;
            pThis->paPages[iPage].pvPage = pbPage;
            pThis->paPages[iPage].cRefs  = 0;
            pThis->paPages[iPage].uPte.pPae = 0;
#ifndef IN_RC
            RTCpuSetFill(&pThis->paPages[iPage].PendingSet);
#endif

            /* Map its page table, retry until we've got a clean run (paranoia). */
            do
                rc = pgmR0DynMapPagingArrayMapPte(pThis, &PgLvl, pbPage, pSeg, cMaxPTs,
                                                  &pThis->paPages[iPage].uPte.pv);
            while (rc == VINF_TRY_AGAIN);
            if (RT_FAILURE(rc))
                break;

            /* Save the PTE. */
            if (pThis->fLegacyMode)
                ((PX86PGUINT)pThis->pvSavedPTEs)[iPage]    = pThis->paPages[iPage].uPte.pLegacy->u;
            else
                ((PX86PGPAEUINT)pThis->pvSavedPTEs)[iPage] = pThis->paPages[iPage].uPte.pPae->u;

#ifdef VBOX_STRICT
            /* Check that we've got the right entry. */
            RTHCPHYS HCPhysPage = RTR0MemObjGetPagePhysAddr(pSeg->hMemObj, iPage - pSeg->iPage);
            RTHCPHYS HCPhysPte  = pThis->fLegacyMode
                                ? pThis->paPages[iPage].uPte.pLegacy->u & X86_PTE_PG_MASK
                                : pThis->paPages[iPage].uPte.pPae->u    & X86_PTE_PAE_PG_MASK;
            if (HCPhysPage != HCPhysPte)
            {
                LogRel(("pgmR0DynMapAddSeg: internal error - page #%u HCPhysPage=%RHp HCPhysPte=%RHp pbPage=%p pvPte=%p\n",
                        iPage - pSeg->iPage, HCPhysPage, HCPhysPte, pbPage, pThis->paPages[iPage].uPte.pv));
                rc = VERR_PGM_DYNMAP_IPE;
                break;
            }
#endif
        } /* for each page */
        ASMIntEnable();

        /* cleanup non-PT mappings */
        for (uint32_t i = 0; i < PgLvl.cLevels - 1; i++)
            RTR0MemObjFree(PgLvl.a[i].hMemObj, true /* fFreeMappings */);

        if (RT_SUCCESS(rc))
        {
#if PGMR0DYNMAP_GUARD_PAGES > 0
            /*
             * Setup guard pages.
             * (Note: TLBs will be shot down later on.)
             */
            uint32_t iPage = pSeg->iPage;
            while (iPage < iEndPage)
            {
                for (uint32_t iGPg = 0; iGPg < PGMR0DYNMAP_GUARD_PAGES && iPage < iEndPage; iGPg++, iPage++)
                    pgmR0DynMapSetupGuardPage(pThis, &pThis->paPages[iPage]);
                iPage++; /* the guarded page */
            }

            /* Make sure the very last page is a guard page too. */
            iPage = iEndPage - 1;
            if (pThis->paPages[iPage].cRefs != PGMR0DYNMAP_GUARD_PAGE_REF_COUNT)
                pgmR0DynMapSetupGuardPage(pThis, &pThis->paPages[iPage]);
#endif /* PGMR0DYNMAP_GUARD_PAGES > 0 */

            /*
             * Commit it by adding the segment to the list and updating the page count.
             */
            pSeg->pNext = pThis->pSegHead;
            pThis->pSegHead = pSeg;
            pThis->cPages += cPages;
            return VINF_SUCCESS;
        }

        /*
         * Bail out.
         */
        while (pSeg->cPTs-- > 0)
        {
            rc2 = RTR0MemObjFree(pSeg->ahMemObjPTs[pSeg->cPTs], true /* fFreeMappings */);
            AssertRC(rc2);
            pSeg->ahMemObjPTs[pSeg->cPTs] = NIL_RTR0MEMOBJ;
        }

        rc2 = RTR0MemObjFree(pSeg->hMemObj, true /* fFreeMappings */);
        AssertRC(rc2);
        pSeg->hMemObj = NIL_RTR0MEMOBJ;
    }
    else if (rc == VERR_NO_PAGE_MEMORY || rc == VERR_NO_PHYS_MEMORY)
        rc = VERR_NO_MEMORY;
    RTMemFree(pSeg);

    /* Don't bother resizing the arrays, but free them if we're the only user. */
    if (!pThis->cPages)
    {
        RTMemFree(pThis->paPages);
        pThis->paPages = NULL;
        RTMemFree(pThis->pvSavedPTEs);
        pThis->pvSavedPTEs = NULL;
    }
    return rc;
}


/**
 * Called by PGMR0DynMapInitVM under the init lock.
 *
 * @returns VBox status code.
 * @param   pThis       The dynamic mapping cache instance.
 */
static int pgmR0DynMapSetup(PPGMRZDYNMAP pThis)
{
    /*
     * Calc the size and add a segment of that size.
     */
    uint32_t cMinPages;
    uint32_t cPages = pgmR0DynMapCalcNewSize(pThis, &cMinPages);
    AssertReturn(cPages, VERR_PGM_DYNMAP_IPE);
    int rc = pgmR0DynMapAddSeg(pThis, cPages);
    if (rc == VERR_NO_MEMORY)
    {
        /*
         * Try adding smaller segments.
         */
        do
            rc = pgmR0DynMapAddSeg(pThis, PGMR0DYNMAP_SMALL_SEG_PAGES);
        while (RT_SUCCESS(rc) && pThis->cPages < cPages);
        if (rc == VERR_NO_MEMORY && pThis->cPages >= cMinPages)
            rc = VINF_SUCCESS;
        if (rc == VERR_NO_MEMORY)
        {
            if (pThis->cPages)
                pgmR0DynMapTearDown(pThis);
            rc = VERR_PGM_DYNMAP_SETUP_ERROR;
        }
    }
    Assert(ASMGetFlags() & X86_EFL_IF);

#if PGMR0DYNMAP_GUARD_PAGES > 0
    /* paranoia */
    if (RT_SUCCESS(rc))
        pgmR0DynMapTlbShootDown(pThis);
#endif
    return rc;
}


/**
 * Called by PGMR0DynMapInitVM under the init lock.
 *
 * @returns VBox status code.
 * @param   pThis       The dynamic mapping cache instance.
 */
static int pgmR0DynMapExpand(PPGMRZDYNMAP pThis)
{
    /*
     * Calc the new target size and add a segment of the appropriate size.
     */
    uint32_t cMinPages;
    uint32_t cPages = pgmR0DynMapCalcNewSize(pThis, &cMinPages);
    AssertReturn(cPages, VERR_PGM_DYNMAP_IPE);
    if (pThis->cPages >= cPages)
        return VINF_SUCCESS;

    uint32_t cAdd = cPages - pThis->cPages;
    int rc = pgmR0DynMapAddSeg(pThis, cAdd);
    if (rc == VERR_NO_MEMORY)
    {
        /*
         * Try adding smaller segments.
         */
        do
            rc = pgmR0DynMapAddSeg(pThis, PGMR0DYNMAP_SMALL_SEG_PAGES);
        while (RT_SUCCESS(rc) && pThis->cPages < cPages);
        if (rc == VERR_NO_MEMORY && pThis->cPages >= cMinPages)
            rc = VINF_SUCCESS;
        if (rc == VERR_NO_MEMORY)
            rc = VERR_PGM_DYNMAP_EXPAND_ERROR;
    }
    Assert(ASMGetFlags() & X86_EFL_IF);

#if PGMR0DYNMAP_GUARD_PAGES > 0
    /* paranoia */
    if (RT_SUCCESS(rc))
        pgmR0DynMapTlbShootDown(pThis);
#endif
    return rc;
}


/**
 * Called by PGMR0DynMapTermVM under the init lock.
 *
 * @returns VBox status code.
 * @param   pThis       The dynamic mapping cache instance.
 */
static void pgmR0DynMapTearDown(PPGMRZDYNMAP pThis)
{
    /*
     * Restore the original page table entries
     */
    PPGMRZDYNMAPENTRY   paPages = pThis->paPages;
    uint32_t            iPage   = pThis->cPages;
    if (pThis->fLegacyMode)
    {
        X86PGUINT const    *paSavedPTEs = (X86PGUINT const *)pThis->pvSavedPTEs;
        while (iPage-- > 0)
        {
            X86PGUINT       uOld  = paPages[iPage].uPte.pLegacy->u;
            X86PGUINT       uOld2 = uOld; NOREF(uOld2);
            X86PGUINT       uNew  = paSavedPTEs[iPage];
            while (!ASMAtomicCmpXchgExU32(&paPages[iPage].uPte.pLegacy->u, uNew, uOld, &uOld))
                AssertMsgFailed(("uOld=%#x uOld2=%#x uNew=%#x\n", uOld, uOld2, uNew));
            Assert(paPages[iPage].uPte.pLegacy->u == paSavedPTEs[iPage]);
        }
    }
    else
    {
        X86PGPAEUINT const *paSavedPTEs = (X86PGPAEUINT const *)pThis->pvSavedPTEs;
        while (iPage-- > 0)
        {
            X86PGPAEUINT    uOld  = paPages[iPage].uPte.pPae->u;
            X86PGPAEUINT    uOld2 = uOld; NOREF(uOld2);
            X86PGPAEUINT    uNew  = paSavedPTEs[iPage];
            while (!ASMAtomicCmpXchgExU64(&paPages[iPage].uPte.pPae->u, uNew, uOld, &uOld))
                AssertMsgFailed(("uOld=%#llx uOld2=%#llx uNew=%#llx\n", uOld, uOld2, uNew));
            Assert(paPages[iPage].uPte.pPae->u == paSavedPTEs[iPage]);
        }
    }

    /*
     * Shoot down the TLBs on all CPUs before freeing them.
     */
    pgmR0DynMapTlbShootDown(pThis);

    /*
     * Free the segments.
     */
    while (pThis->pSegHead)
    {
        int             rc;
        PPGMR0DYNMAPSEG pSeg = pThis->pSegHead;
        pThis->pSegHead = pSeg->pNext;

        uint32_t iPT = pSeg->cPTs;
        while (iPT-- > 0)
        {
            rc = RTR0MemObjFree(pSeg->ahMemObjPTs[iPT], true /* fFreeMappings */); AssertRC(rc);
            pSeg->ahMemObjPTs[iPT] = NIL_RTR0MEMOBJ;
        }
        rc = RTR0MemObjFree(pSeg->hMemObj, true /* fFreeMappings */); AssertRC(rc);
        pSeg->hMemObj   = NIL_RTR0MEMOBJ;
        pSeg->pNext     = NULL;
        pSeg->iPage     = UINT16_MAX;
        pSeg->cPages    = 0;
        pSeg->cPTs      = 0;
        RTMemFree(pSeg);
    }

    /*
     * Free the arrays and restore the initial state.
     * The cLoadMax value is left behind for the next setup.
     */
    RTMemFree(pThis->paPages);
    pThis->paPages = NULL;
    RTMemFree(pThis->pvSavedPTEs);
    pThis->pvSavedPTEs = NULL;
    pThis->cPages = 0;
    pThis->cLoad = 0;
    pThis->cGuardPages = 0;
}

#endif /* IN_RING0 */
#ifdef IN_RC

/**
 * Initializes the dynamic mapping cache in raw-mode context.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 */
VMMRCDECL(int) PGMRCDynMapInit(PVM pVM)
{
    /*
     * Allocate and initialize the instance data and page array.
     */
    PPGMRZDYNMAP    pThis;
    size_t const    cPages = MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE;
    size_t const    cb     = RT_ALIGN_Z(sizeof(*pThis), 32)
                           + sizeof(PGMRZDYNMAPENTRY) * cPages;
    int rc = MMHyperAlloc(pVM, cb, 32, MM_TAG_PGM, (void **)&pThis);
    if (RT_FAILURE(rc))
        return rc;

    pThis->u32Magic     = PGMRZDYNMAP_MAGIC;
    pThis->paPages      = RT_ALIGN_PT(pThis + 1, 32, PPGMRZDYNMAPENTRY);
    pThis->cPages       = cPages;
    pThis->cLoad        = 0;
    pThis->cMaxLoad     = 0;
    pThis->cGuardPages  = 0;
    pThis->cUsers       = 1;

    for (size_t iPage = 0; iPage < cPages; iPage++)
    {
        pThis->paPages[iPage].HCPhys = NIL_RTHCPHYS;
        pThis->paPages[iPage].pvPage = pVM->pgm.s.pbDynPageMapBaseGC + iPage * PAGE_SIZE;
        pThis->paPages[iPage].cRefs  = 0;
        pThis->paPages[iPage].uPte.pLegacy = &pVM->pgm.s.paDynPageMap32BitPTEsGC[iPage];
        pThis->paPages[iPage].uPte.pPae    = (PX86PTEPAE)&pVM->pgm.s.paDynPageMapPaePTEsGC[iPage];
    }

    pVM->pgm.s.pRCDynMap = pThis;

    /*
     * Initialize the autosets the VM.
     */
    rc = pgmRZDynMapInitAutoSetsForVM(pVM);
    if (RT_FAILURE(rc))
        return rc;

    return VINF_SUCCESS;
}

#endif /* IN_RC */

/**
 * Release references to a page, caller owns the spin lock.
 *
 * @param   pThis       The dynamic mapping cache instance.
 * @param   iPage       The page.
 * @param   cRefs       The number of references to release.
 */
DECLINLINE(void) pgmRZDynMapReleasePageLocked(PPGMRZDYNMAP pThis, uint32_t iPage, int32_t cRefs)
{
    cRefs = ASMAtomicSubS32(&pThis->paPages[iPage].cRefs, cRefs) - cRefs;
    AssertMsg(cRefs >= 0, ("%d\n", cRefs));
    if (!cRefs)
    {
        pThis->cLoad--;
#ifdef PGMRZDYNMAP_STRICT_RELEASE
        pThis->paPages[iPage].HCPhys = NIL_RTHCPHYS;
        ASMAtomicBitClear(pThis->paPages[iPage].uPte.pv, X86_PTE_BIT_P);
        ASMInvalidatePage((uintptr_t)pThis->paPages[iPage].pvPage);
#endif
    }
}


/**
 * Release references to a page, caller does not own the spin lock.
 *
 * @param   pThis       The dynamic mapping cache instance.
 * @param   iPage       The page.
 * @param   cRefs       The number of references to release.
 */
static void pgmRZDynMapReleasePage(PPGMRZDYNMAP pThis, uint32_t iPage, uint32_t cRefs)
{
    PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis);
    pgmRZDynMapReleasePageLocked(pThis, iPage, cRefs);
    PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
}


/**
 * pgmR0DynMapPage worker that deals with the tedious bits.
 *
 * @returns The page index on success, UINT32_MAX on failure.
 * @param   pThis   The dynamic mapping cache instance.
 * @param   HCPhys  The address of the page to be mapped.
 * @param   iPage   The page index pgmR0DynMapPage hashed HCPhys to.
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 *                  For statistics.
 * @param   pfNew   Set to @c true if a new entry was made and @c false if
 *                  an old entry was found and reused.
 */
static uint32_t pgmR0DynMapPageSlow(PPGMRZDYNMAP pThis, RTHCPHYS HCPhys, uint32_t iPage, PVMCPU pVCpu, bool *pfNew)
{
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPageSlow); RT_NOREF_PV(pVCpu);

    /*
     * Check if any of the first 3 pages are unreferenced since the caller
     * already has made sure they aren't matching.
     */
#ifdef VBOX_WITH_STATISTICS
    bool                fLooped = false;
#endif
    uint32_t const      cPages  = pThis->cPages;
    PPGMRZDYNMAPENTRY   paPages = pThis->paPages;
    uint32_t            iFreePage;
    if (!paPages[iPage].cRefs)
        iFreePage = iPage;
    else if (!paPages[(iPage + 1) % cPages].cRefs)
        iFreePage   = (iPage + 1) % cPages;
    else if (!paPages[(iPage + 2) % cPages].cRefs)
        iFreePage   = (iPage + 2) % cPages;
    else
    {
        /*
         * Search for an unused or matching entry.
         */
        iFreePage = (iPage + 3) % cPages;
        for (;;)
        {
            if (paPages[iFreePage].HCPhys == HCPhys)
            {
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPageSlowLoopHits);
                *pfNew = false;
                return iFreePage;
            }
            if (!paPages[iFreePage].cRefs)
                break;

            /* advance */
            iFreePage = (iFreePage + 1) % cPages;
            if (RT_UNLIKELY(iFreePage == iPage))
                return UINT32_MAX;
        }
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPageSlowLoopMisses);
#ifdef VBOX_WITH_STATISTICS
        fLooped = true;
#endif
    }
    Assert(iFreePage < cPages);

#if 0 //def VBOX_WITH_STATISTICS
    /* Check for lost hits. */
    if (!fLooped)
        for (uint32_t iPage2 = (iPage + 3) % cPages; iPage2 != iPage; iPage2 = (iPage2 + 1) % cPages)
            if (paPages[iPage2].HCPhys == HCPhys)
                STAM_COUNTER_INC(&pVCpu->pgm.s.StatRZDynMapPageSlowLostHits);
#endif

    /*
     * Setup the new entry.
     */
    *pfNew = true;
    /*Log6(("pgmR0DynMapPageSlow: old - %RHp %#x %#llx\n", paPages[iFreePage].HCPhys, paPages[iFreePage].cRefs, paPages[iFreePage].uPte.pPae->u));*/
    paPages[iFreePage].HCPhys = HCPhys;
#ifndef IN_RC
    RTCpuSetFill(&paPages[iFreePage].PendingSet);

    if (pThis->fLegacyMode)
#endif
    {
        X86PGUINT       uOld  = paPages[iFreePage].uPte.pLegacy->u;
        X86PGUINT       uOld2 = uOld; NOREF(uOld2);
        X86PGUINT       uNew  = (uOld & (X86_PTE_G | X86_PTE_PAT | X86_PTE_PCD | X86_PTE_PWT))
                              | X86_PTE_P | X86_PTE_RW | X86_PTE_A | X86_PTE_D
                              | (HCPhys & X86_PTE_PG_MASK);
        while (!ASMAtomicCmpXchgExU32(&paPages[iFreePage].uPte.pLegacy->u, uNew, uOld, &uOld))
            AssertMsgFailed(("uOld=%#x uOld2=%#x uNew=%#x\n", uOld, uOld2, uNew));
        Assert(paPages[iFreePage].uPte.pLegacy->u == uNew);
    }
#ifndef IN_RC
    else
#endif
    {
        X86PGPAEUINT    uOld  = paPages[iFreePage].uPte.pPae->u;
        X86PGPAEUINT    uOld2 = uOld; NOREF(uOld2);
        X86PGPAEUINT    uNew  = (uOld & (X86_PTE_G | X86_PTE_PAT | X86_PTE_PCD | X86_PTE_PWT))
                              | X86_PTE_P | X86_PTE_RW | X86_PTE_A | X86_PTE_D
                              | (HCPhys & X86_PTE_PAE_PG_MASK);
        while (!ASMAtomicCmpXchgExU64(&paPages[iFreePage].uPte.pPae->u, uNew, uOld, &uOld))
            AssertMsgFailed(("uOld=%#llx uOld2=%#llx uNew=%#llx\n", uOld, uOld2, uNew));
        Assert(paPages[iFreePage].uPte.pPae->u == uNew);
        /*Log6(("pgmR0DynMapPageSlow: #%x - %RHp %p %#llx\n", iFreePage, HCPhys, paPages[iFreePage].pvPage, uNew));*/
    }
    return iFreePage;
}


/**
 * Maps a page into the pool.
 *
 * @returns Page index on success, UINT32_MAX on failure.
 * @param   pThis       The dynamic mapping cache instance.
 * @param   HCPhys      The address of the page to be mapped.
 * @param   iRealCpu    The real cpu set index. (optimization)
 * @param   pVCpu       The cross context virtual CPU structure of the calling
 *                      EMT.  For statistics.
 * @param   ppvPage     Where to the page address.
 */
DECLINLINE(uint32_t) pgmR0DynMapPage(PPGMRZDYNMAP pThis, RTHCPHYS HCPhys, int32_t iRealCpu, PVMCPU pVCpu, void **ppvPage)
{
    PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis);
    AssertMsg(!(HCPhys & PAGE_OFFSET_MASK), ("HCPhys=%RHp\n", HCPhys));
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPage);

    /*
     * Find an entry, if possible a matching one. The HCPhys address is hashed
     * down to a page index, collisions are handled by linear searching.
     * Optimized for a hit in the first 3 pages.
     *
     * Field easy hits here and defer the tedious searching and inserting
     * to pgmR0DynMapPageSlow().
     */
    bool                fNew    = false;
    uint32_t const      cPages  = pThis->cPages;
    uint32_t            iPage   = (HCPhys >> PAGE_SHIFT) % cPages;
    PPGMRZDYNMAPENTRY   paPages = pThis->paPages;
    if (RT_LIKELY(paPages[iPage].HCPhys == HCPhys))
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPageHits0);
    else
    {
        uint32_t        iPage2 = (iPage + 1) % cPages;
        if (RT_LIKELY(paPages[iPage2].HCPhys == HCPhys))
        {
            iPage = iPage2;
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPageHits1);
        }
        else
        {
            iPage2 = (iPage + 2) % cPages;
            if (paPages[iPage2].HCPhys == HCPhys)
            {
                iPage = iPage2;
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPageHits2);
            }
            else
            {
                iPage = pgmR0DynMapPageSlow(pThis, HCPhys, iPage, pVCpu, &fNew);
                if (RT_UNLIKELY(iPage == UINT32_MAX))
                {
                    PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
                    *ppvPage = NULL;
                    return iPage;
                }
            }
        }
    }

    /*
     * Reference it, update statistics and get the return address.
     */
    int32_t cRefs = ASMAtomicIncS32(&paPages[iPage].cRefs);
    if (cRefs == 1)
    {
        pThis->cLoad++;
        if (pThis->cLoad > pThis->cMaxLoad)
            pThis->cMaxLoad = pThis->cLoad;
        AssertMsg(pThis->cLoad <= pThis->cPages - pThis->cGuardPages, ("%d/%d\n", pThis->cLoad, pThis->cPages - pThis->cGuardPages));
    }
    else if (RT_UNLIKELY(cRefs <= 0))
    {
        ASMAtomicDecS32(&paPages[iPage].cRefs);
        PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
        *ppvPage = NULL;
        AssertLogRelMsgFailedReturn(("cRefs=%d iPage=%u HCPhys=%RHp\n", cRefs, iPage, HCPhys), UINT32_MAX);
    }
    void *pvPage = paPages[iPage].pvPage;

#ifndef IN_RC
    /*
     * Invalidate the entry?
     */
    bool fInvalidateIt = RTCpuSetIsMemberByIndex(&paPages[iPage].PendingSet, iRealCpu);
    if (RT_UNLIKELY(fInvalidateIt))
        RTCpuSetDelByIndex(&paPages[iPage].PendingSet, iRealCpu);
#else
    NOREF(iRealCpu);
#endif

    PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);

    /*
     * Do the actual invalidation outside the spinlock.
     */
#ifdef IN_RC
    if (RT_UNLIKELY(fNew))
#else
    if (RT_UNLIKELY(fInvalidateIt))
#endif
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapPageInvlPg);
        ASMInvalidatePage((uintptr_t)pvPage);
    }

    *ppvPage = pvPage;
    return iPage;
}


/**
 * Assert the integrity of the pool.
 *
 * @returns VBox status code.
 */
static int pgmRZDynMapAssertIntegrity(PPGMRZDYNMAP pThis)
{
    /*
     * Basic pool stuff that doesn't require any lock, just assumes we're a user.
     */
    if (!pThis)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertReturn(pThis->u32Magic == PGMRZDYNMAP_MAGIC, VERR_INVALID_MAGIC);
    if (!pThis->cUsers)
        return VERR_INVALID_PARAMETER;

    PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis);

#define CHECK_RET(expr, a) \
    do { \
        if (RT_UNLIKELY(!(expr))) \
        { \
            PGMRZDYNMAP_SPINLOCK_RELEASE(pThis); \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            return VERR_PGM_DYNMAP_IPE; \
        } \
    } while (0)

    /*
     * Check that the PTEs are correct.
     */
    uint32_t            cGuard      = 0;
    uint32_t            cLoad       = 0;
    PPGMRZDYNMAPENTRY   paPages     = pThis->paPages;

#ifndef IN_RC
    if (pThis->fLegacyMode)
#endif
    {
#ifdef IN_RING0
        PCX86PGUINT     paSavedPTEs = (PCX86PGUINT)pThis->pvSavedPTEs; NOREF(paSavedPTEs);
#endif
        uint32_t        iPage       = pThis->cPages;
        while (iPage-- > 0)
        {
            CHECK_RET(!((uintptr_t)paPages[iPage].pvPage & PAGE_OFFSET_MASK), ("#%u: %p\n", iPage, paPages[iPage].pvPage));
            if (    paPages[iPage].cRefs  == PGMR0DYNMAP_GUARD_PAGE_REF_COUNT
                &&  paPages[iPage].HCPhys == PGMR0DYNMAP_GUARD_PAGE_HCPHYS)
            {
#ifdef PGMR0DYNMAP_GUARD_NP
                CHECK_RET(paPages[iPage].uPte.pLegacy->u == (paSavedPTEs[iPage] & ~(X86PGUINT)X86_PTE_P),
                          ("#%u: %#x %#x", iPage, paPages[iPage].uPte.pLegacy->u, paSavedPTEs[iPage]));
#else
                CHECK_RET(paPages[iPage].uPte.pLegacy->u == PGMR0DYNMAP_GUARD_PAGE_LEGACY_PTE,
                          ("#%u: %#x", iPage, paPages[iPage].uPte.pLegacy->u));
#endif
                cGuard++;
            }
            else if (paPages[iPage].HCPhys != NIL_RTHCPHYS)
            {
                CHECK_RET(!(paPages[iPage].HCPhys & PAGE_OFFSET_MASK), ("#%u: %RHp\n", iPage, paPages[iPage].HCPhys));
                X86PGUINT uPte = X86_PTE_P | X86_PTE_RW | X86_PTE_A | X86_PTE_D
#ifdef IN_RING0
                               | (paSavedPTEs[iPage] & (X86_PTE_G | X86_PTE_PAT | X86_PTE_PCD | X86_PTE_PWT))
#endif
                               | (paPages[iPage].HCPhys & X86_PTE_PAE_PG_MASK);
                CHECK_RET(paPages[iPage].uPte.pLegacy->u == uPte,
                          ("#%u: %#x %#x", iPage, paPages[iPage].uPte.pLegacy->u, uPte));
                if (paPages[iPage].cRefs)
                    cLoad++;
            }
#if defined(IN_RING0) && !defined(PGMRZDYNMAP_STRICT_RELEASE)
            else
                CHECK_RET(paPages[iPage].uPte.pLegacy->u == paSavedPTEs[iPage],
                          ("#%u: %#x %#x", iPage, paPages[iPage].uPte.pLegacy->u, paSavedPTEs[iPage]));
#endif
        }
    }
#ifndef IN_RC
    else
#endif
    {
#ifdef IN_RING0
        PCX86PGPAEUINT  paSavedPTEs = (PCX86PGPAEUINT)pThis->pvSavedPTEs; NOREF(paSavedPTEs);
#endif
        uint32_t        iPage       = pThis->cPages;
        while (iPage-- > 0)
        {
            CHECK_RET(!((uintptr_t)paPages[iPage].pvPage & PAGE_OFFSET_MASK), ("#%u: %p\n", iPage, paPages[iPage].pvPage));
            if (    paPages[iPage].cRefs  == PGMR0DYNMAP_GUARD_PAGE_REF_COUNT
                &&  paPages[iPage].HCPhys == PGMR0DYNMAP_GUARD_PAGE_HCPHYS)
            {
#ifdef PGMR0DYNMAP_GUARD_NP
                CHECK_RET(paPages[iPage].uPte.pPae->u == (paSavedPTEs[iPage] & ~(X86PGPAEUINT)X86_PTE_P),
                          ("#%u: %#llx %#llx", iPage, paPages[iPage].uPte.pPae->u, paSavedPTEs[iPage]));
#else
                CHECK_RET(paPages[iPage].uPte.pPae->u == PGMR0DYNMAP_GUARD_PAGE_PAE_PTE,
                          ("#%u: %#llx", iPage, paPages[iPage].uPte.pPae->u));
#endif
                cGuard++;
            }
            else if (paPages[iPage].HCPhys != NIL_RTHCPHYS)
            {
                CHECK_RET(!(paPages[iPage].HCPhys & PAGE_OFFSET_MASK), ("#%u: %RHp\n", iPage, paPages[iPage].HCPhys));
                X86PGPAEUINT uPte = X86_PTE_P | X86_PTE_RW | X86_PTE_A | X86_PTE_D
#ifdef IN_RING0
                                  | (paSavedPTEs[iPage] & (X86_PTE_G | X86_PTE_PAT | X86_PTE_PCD | X86_PTE_PWT))
#endif
                                  | (paPages[iPage].HCPhys & X86_PTE_PAE_PG_MASK);
                CHECK_RET(paPages[iPage].uPte.pPae->u == uPte,
                          ("#%u: %#llx %#llx", iPage, paPages[iPage].uPte.pLegacy->u, uPte));
                if (paPages[iPage].cRefs)
                    cLoad++;
            }
#ifdef IN_RING0
            else
                CHECK_RET(paPages[iPage].uPte.pPae->u == paSavedPTEs[iPage],
                          ("#%u: %#llx %#llx", iPage, paPages[iPage].uPte.pPae->u, paSavedPTEs[iPage]));
#endif
        }
    }

    CHECK_RET(cLoad == pThis->cLoad, ("%u %u\n", cLoad, pThis->cLoad));
    CHECK_RET(cGuard == pThis->cGuardPages, ("%u %u\n", cGuard, pThis->cGuardPages));

#undef CHECK_RET
    PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
    return VINF_SUCCESS;
}

#ifdef IN_RING0
/**
 * Assert the integrity of the pool.
 *
 * @returns VBox status code.
 */
VMMR0DECL(int) PGMR0DynMapAssertIntegrity(void)
{
    return pgmRZDynMapAssertIntegrity(g_pPGMR0DynMap);
}
#endif /* IN_RING0 */

#ifdef IN_RC
/**
 * Assert the integrity of the pool.
 *
 * @returns VBox status code.
 */
VMMRCDECL(int) PGMRCDynMapAssertIntegrity(PVM pVM)
{
    return pgmRZDynMapAssertIntegrity((PPGMRZDYNMAP)pVM->pgm.s.pRCDynMap);
}
#endif /* IN_RC */


/**
 * As a final resort for a (somewhat) full auto set or full cache, try merge
 * duplicate entries and flush the ones we can.
 *
 * @param   pSet        The set.
 */
static void pgmDynMapOptimizeAutoSet(PPGMMAPSET pSet)
{
    LogFlow(("pgmDynMapOptimizeAutoSet\n"));

    for (uint32_t i = 0 ; i < pSet->cEntries; i++)
    {
        /*
         * Try merge entries.
         */
        uint16_t const  iPage = pSet->aEntries[i].iPage;
        uint32_t        j     = i + 1;
        while (   j < pSet->cEntries
               && (   pSet->iSubset == UINT32_MAX
                   || pSet->iSubset < pSet->cEntries) )
        {
            if (pSet->aEntries[j].iPage != iPage)
                j++;
            else
            {
                uint32_t const  cHardRefs    = (uint32_t)pSet->aEntries[i].cRefs
                                             + (uint32_t)pSet->aEntries[j].cRefs;
                uint32_t        cInlinedRefs = (uint32_t)pSet->aEntries[i].cInlinedRefs
                                             + (uint32_t)pSet->aEntries[j].cInlinedRefs;
                uint32_t        cUnrefs      = (uint32_t)pSet->aEntries[i].cUnrefs
                                             + (uint32_t)pSet->aEntries[j].cUnrefs;
                uint32_t        cSub         = RT_MIN(cUnrefs, cInlinedRefs);
                cInlinedRefs -= cSub;
                cUnrefs      -= cSub;

                if (    cHardRefs    < UINT16_MAX
                    &&  cInlinedRefs < UINT16_MAX
                    &&  cUnrefs      < UINT16_MAX)
                {
                    /* merge j into i removing j. */
                    Log2(("pgmDynMapOptimizeAutoSet: Merging #%u into #%u\n", j, i));
                    pSet->aEntries[i].cRefs        = cHardRefs;
                    pSet->aEntries[i].cInlinedRefs = cInlinedRefs;
                    pSet->aEntries[i].cUnrefs      = cUnrefs;
                    pSet->cEntries--;
                    if (j < pSet->cEntries)
                    {
                        pSet->aEntries[j] = pSet->aEntries[pSet->cEntries];
                        PGMRZDYNMAP_ZAP_ENTRY(&pSet->aEntries[pSet->cEntries]);
                    }
                    else
                        PGMRZDYNMAP_ZAP_ENTRY(&pSet->aEntries[j]);
                }
#if 0 /* too complicated, skip it. */
                else
                {
                    /* migrate the max number of refs from j into i and quit the inner loop. */
                    uint32_t cMigrate = UINT16_MAX - 1 - pSet->aEntries[i].cRefs;
                    Assert(pSet->aEntries[j].cRefs > cMigrate);
                    pSet->aEntries[j].cRefs -= cMigrate;
                    pSet->aEntries[i].cRefs = UINT16_MAX - 1;
                    break;
                }
#endif
            }
        }

        /*
         * Try make use of the unused hinting (cUnrefs) to evict entries
         * from both the set as well as the mapping cache.
         */

        uint32_t const cTotalRefs = (uint32_t)pSet->aEntries[i].cRefs + pSet->aEntries[i].cInlinedRefs;
        Log2(("pgmDynMapOptimizeAutoSet: #%u/%u/%u pvPage=%p iPage=%u cRefs=%u cInlinedRefs=%u cUnrefs=%u cTotalRefs=%u\n",
              i,
              pSet->iSubset,
              pSet->cEntries,
              pSet->aEntries[i].pvPage,
              pSet->aEntries[i].iPage,
              pSet->aEntries[i].cRefs,
              pSet->aEntries[i].cInlinedRefs,
              pSet->aEntries[i].cUnrefs,
              cTotalRefs));
        Assert(cTotalRefs >= pSet->aEntries[i].cUnrefs);

        if (    cTotalRefs == pSet->aEntries[i].cUnrefs
            &&  (   pSet->iSubset == UINT32_MAX
                 || pSet->iSubset < pSet->cEntries)
           )
        {
            Log2(("pgmDynMapOptimizeAutoSet: Releasing iPage=%d/%p\n", pSet->aEntries[i].iPage, pSet->aEntries[i].pvPage));
            //LogFlow(("pgmDynMapOptimizeAutoSet: Releasing iPage=%d/%p\n", pSet->aEntries[i].iPage, pSet->aEntries[i].pvPage));
            pgmRZDynMapReleasePage(PGMRZDYNMAP_SET_2_DYNMAP(pSet),
                                   pSet->aEntries[i].iPage,
                                   pSet->aEntries[i].cRefs);
            pSet->cEntries--;
            if (i < pSet->cEntries)
            {
                pSet->aEntries[i] = pSet->aEntries[pSet->cEntries];
                PGMRZDYNMAP_ZAP_ENTRY(&pSet->aEntries[pSet->cEntries]);
            }

            i--;
        }
    }
}




/**
 * Signals the start of a new set of mappings.
 *
 * Mostly for strictness. PGMDynMapHCPage won't work unless this
 * API is called.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(void) PGMRZDynMapStartAutoSet(PVMCPU pVCpu)
{
    LogFlow(("PGMRZDynMapStartAutoSet:\n"));
    Assert(pVCpu->pgm.s.AutoSet.cEntries == PGMMAPSET_CLOSED);
    Assert(pVCpu->pgm.s.AutoSet.iSubset == UINT32_MAX);
    pVCpu->pgm.s.AutoSet.cEntries = 0;
    pVCpu->pgm.s.AutoSet.iCpu = PGMRZDYNMAP_CUR_CPU();
}


#ifdef IN_RING0
/**
 * Starts or migrates the autoset of a virtual CPU.
 *
 * This is used by HMR0Enter.  When we've longjumped out of the HM
 * execution loop with the set open, we'll migrate it when re-entering.  While
 * under normal circumstances, we'll start it so VMXR0LoadGuestState can access
 * guest memory.
 *
 * @returns @c true if started, @c false if migrated.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @thread  EMT
 */
VMMR0DECL(bool) PGMR0DynMapStartOrMigrateAutoSet(PVMCPU pVCpu)
{
    bool fStartIt = pVCpu->pgm.s.AutoSet.cEntries == PGMMAPSET_CLOSED;
    if (fStartIt)
        PGMRZDynMapStartAutoSet(pVCpu);
    else
        PGMR0DynMapMigrateAutoSet(pVCpu);
    return fStartIt;
}
#endif /* IN_RING0 */


/**
 * Checks if the set has high load.
 *
 * @returns true on high load, otherwise false.
 * @param   pSet            The set.
 */
DECLINLINE(bool) pgmRZDynMapHasHighLoad(PPGMMAPSET pSet)
{
#ifdef IN_RC
    if (pSet->cEntries < MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE / 2)
        return false;
#endif

    PPGMRZDYNMAP pThis = PGMRZDYNMAP_SET_2_DYNMAP(pSet);
    uint32_t cUnusedPages = pThis->cPages - pThis->cLoad;
#ifdef IN_RC
    return cUnusedPages <= MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE * 36 / 100;
#else
    return cUnusedPages <= PGMR0DYNMAP_PAGES_PER_CPU_MIN;
#endif
}


/**
 * Worker that performs the actual flushing of the set.
 *
 * @param   pSet        The set to flush.
 * @param   cEntries    The number of entries.
 */
DECLINLINE(void) pgmDynMapFlushAutoSetWorker(PPGMMAPSET pSet, uint32_t cEntries)
{
    /*
     * Release any pages it's referencing.
     */
    if (    cEntries != 0
        &&  RT_LIKELY(cEntries <= RT_ELEMENTS(pSet->aEntries)))
    {
        PPGMRZDYNMAP    pThis   = PGMRZDYNMAP_SET_2_DYNMAP(pSet);
        PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis);

        uint32_t i = cEntries;
        while (i-- > 0)
        {
            uint32_t iPage = pSet->aEntries[i].iPage;
            Assert(iPage < pThis->cPages);
            int32_t  cRefs = pSet->aEntries[i].cRefs;
            Assert(cRefs > 0);
            pgmRZDynMapReleasePageLocked(pThis, iPage, cRefs);

            PGMRZDYNMAP_ZAP_ENTRY(&pSet->aEntries[i]);
        }

        Assert(pThis->cLoad <= pThis->cPages - pThis->cGuardPages);
        PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
    }
}


/**
 * Releases the dynamic memory mappings made by PGMDynMapHCPage and associates
 * since the PGMDynMapStartAutoSet call.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(void) PGMRZDynMapReleaseAutoSet(PVMCPU pVCpu)
{
    PPGMMAPSET  pSet = &pVCpu->pgm.s.AutoSet;

    /*
     * Close and flush the set.
     */
    uint32_t    cEntries = pSet->cEntries;
    AssertReturnVoid(cEntries != PGMMAPSET_CLOSED);
    pSet->cEntries = PGMMAPSET_CLOSED;
    pSet->iSubset = UINT32_MAX;
    pSet->iCpu = -1;

#ifdef IN_RC
    if (RT_ELEMENTS(pSet->aEntries) > MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE)
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->aStatRZDynMapSetFilledPct[(cEntries * 10 / (MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE)) % 11]);
    else
#endif
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->aStatRZDynMapSetFilledPct[(cEntries * 10 / RT_ELEMENTS(pSet->aEntries)) % 11]);
    if (cEntries > RT_ELEMENTS(pSet->aEntries) * 50 / 100)
        Log(("PGMRZDynMapReleaseAutoSet: cEntries=%d\n", cEntries));
    else
        LogFlow(("PGMRZDynMapReleaseAutoSet: cEntries=%d\n", cEntries));

    pgmDynMapFlushAutoSetWorker(pSet, cEntries);
}


/**
 * Flushes the set if it's above a certain threshold.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(void) PGMRZDynMapFlushAutoSet(PVMCPU pVCpu)
{
    PPGMMAPSET  pSet = &pVCpu->pgm.s.AutoSet;
    AssertMsg(pSet->iCpu == PGMRZDYNMAP_CUR_CPU(), ("%d %d efl=%#x\n", pSet->iCpu, PGMRZDYNMAP_CUR_CPU(), ASMGetFlags()));

    /*
     * Only flush it if it's 45% full.
     */
    uint32_t cEntries = pSet->cEntries;
    AssertReturnVoid(cEntries != PGMMAPSET_CLOSED);
    Assert(pSet->iSubset == UINT32_MAX);
#ifdef IN_RC
    if (RT_ELEMENTS(pSet->aEntries) > MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE)
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->aStatRZDynMapSetFilledPct[(cEntries * 10 / (MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE)) % 11]);
    else
#endif
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->aStatRZDynMapSetFilledPct[(cEntries * 10 / RT_ELEMENTS(pSet->aEntries)) % 11]);
    if (   cEntries >= RT_ELEMENTS(pSet->aEntries) * 45 / 100
        || pgmRZDynMapHasHighLoad(pSet))
    {
        pSet->cEntries = 0;
        Log(("PGMDynMapFlushAutoSet: cEntries=%d\n", pSet->cEntries));

        pgmDynMapFlushAutoSetWorker(pSet, cEntries);
        AssertMsg(pSet->iCpu == PGMRZDYNMAP_CUR_CPU(), ("%d %d efl=%#x\n", pSet->iCpu, PGMRZDYNMAP_CUR_CPU(), ASMGetFlags()));
    }
}


#ifndef IN_RC
/**
 * Migrates the automatic mapping set of the current vCPU if it's active and
 * necessary.
 *
 * This is called when re-entering the hardware assisted execution mode after a
 * nip down to ring-3.  We run the risk that the CPU might have change and we
 * will therefore make sure all the cache entries currently in the auto set will
 * be valid on the new CPU.  If the cpu didn't change nothing will happen as all
 * the entries will have been flagged as invalidated.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @thread  EMT
 */
VMMR0DECL(void) PGMR0DynMapMigrateAutoSet(PVMCPU pVCpu)
{
    LogFlow(("PGMR0DynMapMigrateAutoSet\n"));
    PPGMMAPSET      pSet     = &pVCpu->pgm.s.AutoSet;
    int32_t         iRealCpu = PGMRZDYNMAP_CUR_CPU();
    if (pSet->iCpu != iRealCpu)
    {
        uint32_t    i        = pSet->cEntries;
        if (i != PGMMAPSET_CLOSED)
        {
            AssertMsg(i <= RT_ELEMENTS(pSet->aEntries), ("%#x (%u)\n", i, i));
            if (i != 0 && RT_LIKELY(i <= RT_ELEMENTS(pSet->aEntries)))
            {
                PPGMRZDYNMAP    pThis  = PGMRZDYNMAP_SET_2_DYNMAP(pSet);
                PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis);

                while (i-- > 0)
                {
                    Assert(pSet->aEntries[i].cRefs > 0);
                    uint32_t iPage = pSet->aEntries[i].iPage;
                    Assert(iPage < pThis->cPages);
                    if (RTCpuSetIsMemberByIndex(&pThis->paPages[iPage].PendingSet, iRealCpu))
                    {
                        RTCpuSetDelByIndex(&pThis->paPages[iPage].PendingSet, iRealCpu);
                        PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);

                        ASMInvalidatePage((uintptr_t)pThis->paPages[iPage].pvPage);
                        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapMigrateInvlPg);

                        PGMRZDYNMAP_SPINLOCK_REACQUIRE(pThis);
                    }
                }

                PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
            }
        }
        pSet->iCpu = iRealCpu;
    }
}
#endif /* !IN_RC */


/**
 * Worker function that flushes the current subset.
 *
 * This is called when the set is popped or when the set
 * hash a too high load. As also pointed out elsewhere, the
 * whole subset thing is a hack for working around code that
 * accesses too many pages. Like PGMPool.
 *
 * @param   pSet        The set which subset to flush.
 */
static void pgmDynMapFlushSubset(PPGMMAPSET pSet)
{
    uint32_t iSubset = pSet->iSubset;
    uint32_t i       = pSet->cEntries;
    Assert(i <= RT_ELEMENTS(pSet->aEntries));
    if (    i > iSubset
        &&  i <= RT_ELEMENTS(pSet->aEntries))
    {
        Log(("pgmDynMapFlushSubset: cEntries=%d iSubset=%d\n", pSet->cEntries, iSubset));
        pSet->cEntries = iSubset;

        PPGMRZDYNMAP    pThis = PGMRZDYNMAP_SET_2_DYNMAP(pSet);
        PGMRZDYNMAP_SPINLOCK_ACQUIRE(pThis);

        while (i-- > iSubset)
        {
            uint32_t iPage = pSet->aEntries[i].iPage;
            Assert(iPage < pThis->cPages);
            int32_t  cRefs = pSet->aEntries[i].cRefs;
            Assert(cRefs > 0);
            pgmRZDynMapReleasePageLocked(pThis, iPage, cRefs);

            PGMRZDYNMAP_ZAP_ENTRY(&pSet->aEntries[i]);
        }

        PGMRZDYNMAP_SPINLOCK_RELEASE(pThis);
    }
}


/**
 * Creates a subset.
 *
 * A subset is a hack to avoid having to rewrite code that touches a lot of
 * pages. It prevents the mapping set from being overflowed by automatically
 * flushing previous mappings when a certain threshold is reached.
 *
 * Pages mapped after calling this function are only valid until the next page
 * is mapped.
 *
 * @returns The index of the previous subset. Pass this to
 *          PGMDynMapPopAutoSubset when popping it.
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(uint32_t) PGMRZDynMapPushAutoSubset(PVMCPU pVCpu)
{
    PPGMMAPSET      pSet = &pVCpu->pgm.s.AutoSet;
    AssertReturn(pSet->cEntries != PGMMAPSET_CLOSED, UINT32_MAX);
    uint32_t        iPrevSubset = pSet->iSubset;
    LogFlow(("PGMRZDynMapPushAutoSubset: pVCpu=%p iPrevSubset=%u\n", pVCpu, iPrevSubset));

    /*
     * If it looks like we're approaching the max set size or mapping space
     * optimize the set to drop off unused pages.
     */
    if (   pSet->cEntries > RT_ELEMENTS(pSet->aEntries) * 60 / 100
        || pgmRZDynMapHasHighLoad(pSet))
    {
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapSetOptimize);
        pgmDynMapOptimizeAutoSet(pSet);
    }

    pSet->iSubset = pSet->cEntries;
    STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapSubsets);

    AssertMsg(iPrevSubset <= pSet->iSubset || iPrevSubset == UINT32_MAX, ("iPrevSubset=%#x iSubset=%#x\n", iPrevSubset, pSet->iSubset));
    return iPrevSubset;
}


/**
 * Pops a subset created by a previous call to PGMDynMapPushAutoSubset.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   iPrevSubset     What PGMDynMapPushAutoSubset returned.
 */
VMMDECL(void) PGMRZDynMapPopAutoSubset(PVMCPU pVCpu, uint32_t iPrevSubset)
{
    PPGMMAPSET      pSet = &pVCpu->pgm.s.AutoSet;
    uint32_t        cEntries = pSet->cEntries;
    LogFlow(("PGMRZDynMapPopAutoSubset: pVCpu=%p iPrevSubset=%u iSubset=%u cEntries=%u\n", pVCpu, iPrevSubset, pSet->iSubset, cEntries));
    AssertReturnVoid(cEntries != PGMMAPSET_CLOSED);
    AssertMsgReturnVoid(pSet->iSubset >= iPrevSubset || iPrevSubset == UINT32_MAX, ("iPrevSubset=%u iSubset=%u cEntries=%u\n", iPrevSubset, pSet->iSubset, cEntries));
#ifdef IN_RC
    if (RT_ELEMENTS(pSet->aEntries) > MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE)
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->aStatRZDynMapSetFilledPct[(cEntries * 10 / (MM_HYPER_DYNAMIC_SIZE / PAGE_SIZE)) % 11]);
    else
#endif
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->aStatRZDynMapSetFilledPct[(cEntries * 10 / RT_ELEMENTS(pSet->aEntries)) % 11]);
    if (    cEntries >= RT_ELEMENTS(pSet->aEntries) * 40 / 100
        &&  cEntries != pSet->iSubset)
    {
        pgmDynMapFlushSubset(pSet);
        Assert(pSet->cEntries >= iPrevSubset || iPrevSubset == UINT32_MAX);
    }
    pSet->iSubset = iPrevSubset;
}


/**
 * Indicates that the given page is unused and its mapping can be re-used.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   pvHint          The page that is now unused.  This does not have to
 *                          point at the start of the page.  NULL is ignored.
 */
#ifdef LOG_ENABLED
void pgmRZDynMapUnusedHint(PVMCPU pVCpu, void *pvHint, RT_SRC_POS_DECL)
#else
void pgmRZDynMapUnusedHint(PVMCPU pVCpu, void *pvHint)
#endif
{
    /*
     * Ignore NULL pointers and mask off the page offset bits.
     */
    if (pvHint == NULL)
        return;
    pvHint = (void *)((uintptr_t)pvHint & ~(uintptr_t)PAGE_OFFSET_MASK);

    PPGMMAPSET  pSet    = &pVCpu->pgm.s.AutoSet;
    uint32_t    iEntry  = pSet->cEntries;
    AssertReturnVoid(iEntry > 0);

    /*
     * Find the entry in the usual unrolled fashion.
     */
    /** @todo add a hint to the set which entry was used last since it's not
     *        always the last entry? */
#define IS_MATCHING_ENTRY(pSet, iEntry, pvHint) \
        (   (pSet)->aEntries[(iEntry)].pvPage == (pvHint) \
         &&   (uint32_t)(pSet)->aEntries[(iEntry)].cRefs + (pSet)->aEntries[(iEntry)].cInlinedRefs \
            > (pSet)->aEntries[(iEntry)].cUnrefs )
    if (     iEntry >= 1 && IS_MATCHING_ENTRY(pSet, iEntry - 1, pvHint))
        iEntry = iEntry - 1;
    else if (iEntry >= 2 && IS_MATCHING_ENTRY(pSet, iEntry - 2, pvHint))
        iEntry = iEntry - 2;
    else if (iEntry >= 3 && IS_MATCHING_ENTRY(pSet, iEntry - 3, pvHint))
        iEntry = iEntry - 3;
    else if (iEntry >= 4 && IS_MATCHING_ENTRY(pSet, iEntry - 4, pvHint))
        iEntry = iEntry - 4;
    else if (iEntry >= 5 && IS_MATCHING_ENTRY(pSet, iEntry - 5, pvHint))
        iEntry = iEntry - 5;
    else if (iEntry >= 6 && IS_MATCHING_ENTRY(pSet, iEntry - 6, pvHint))
        iEntry = iEntry - 6;
    else if (iEntry >= 7 && IS_MATCHING_ENTRY(pSet, iEntry - 7, pvHint))
        iEntry = iEntry - 7;
    else
    {
        /*
         * Loop till we find it.
         */
        bool fFound = false;
        if (iEntry > 7)
        {
            iEntry -= 7;
            while (iEntry-- > 0)
                if (IS_MATCHING_ENTRY(pSet, iEntry, pvHint))
                {
                    fFound = true;
                    break;
                }
        }
        AssertMsgReturnVoid(fFound,
                            ("pvHint=%p cEntries=%#x iSubset=%#x\n"
                             "aEntries[0] = {%#x, %#x, %#x, %#x, %p}\n"
                             "aEntries[1] = {%#x, %#x, %#x, %#x, %p}\n"
                             "aEntries[2] = {%#x, %#x, %#x, %#x, %p}\n"
                             "aEntries[3] = {%#x, %#x, %#x, %#x, %p}\n"
                             "aEntries[4] = {%#x, %#x, %#x, %#x, %p}\n"
                             "aEntries[5] = {%#x, %#x, %#x, %#x, %p}\n"
                             ,
                             pvHint, pSet->cEntries, pSet->iSubset,
                             pSet->aEntries[0].iPage, pSet->aEntries[0].cRefs, pSet->aEntries[0].cInlinedRefs, pSet->aEntries[0].cUnrefs, pSet->aEntries[0].pvPage,
                             pSet->aEntries[1].iPage, pSet->aEntries[1].cRefs, pSet->aEntries[1].cInlinedRefs, pSet->aEntries[1].cUnrefs, pSet->aEntries[1].pvPage,
                             pSet->aEntries[2].iPage, pSet->aEntries[2].cRefs, pSet->aEntries[2].cInlinedRefs, pSet->aEntries[2].cUnrefs, pSet->aEntries[2].pvPage,
                             pSet->aEntries[3].iPage, pSet->aEntries[3].cRefs, pSet->aEntries[3].cInlinedRefs, pSet->aEntries[3].cUnrefs, pSet->aEntries[3].pvPage,
                             pSet->aEntries[4].iPage, pSet->aEntries[4].cRefs, pSet->aEntries[4].cInlinedRefs, pSet->aEntries[4].cUnrefs, pSet->aEntries[4].pvPage,
                             pSet->aEntries[5].iPage, pSet->aEntries[5].cRefs, pSet->aEntries[5].cInlinedRefs, pSet->aEntries[5].cUnrefs, pSet->aEntries[5].pvPage));
    }
#undef IS_MATCHING_ENTRY

    /*
     * Update it.
     */
    uint32_t const  cTotalRefs = (uint32_t)pSet->aEntries[iEntry].cRefs + pSet->aEntries[iEntry].cInlinedRefs;
    uint32_t const  cUnrefs    = pSet->aEntries[iEntry].cUnrefs;
    LogFlow(("pgmRZDynMapUnusedHint: pvHint=%p #%u cRefs=%d cInlinedRefs=%d cUnrefs=%d (+1) cTotalRefs=%d %s(%d) %s\n",
             pvHint, iEntry, pSet->aEntries[iEntry].cRefs, pSet->aEntries[iEntry].cInlinedRefs, cUnrefs, cTotalRefs, pszFile, iLine, pszFunction));
    AssertReturnVoid(cTotalRefs > cUnrefs);

    if (RT_LIKELY(cUnrefs < UINT16_MAX - 1))
        pSet->aEntries[iEntry].cUnrefs++;
    else if (pSet->aEntries[iEntry].cInlinedRefs)
    {
        uint32_t cSub = RT_MIN(pSet->aEntries[iEntry].cInlinedRefs, pSet->aEntries[iEntry].cUnrefs);
        pSet->aEntries[iEntry].cInlinedRefs -= cSub;
        pSet->aEntries[iEntry].cUnrefs      -= cSub;
        pSet->aEntries[iEntry].cUnrefs++;
    }
    else
        Log(("pgmRZDynMapUnusedHint: pvHint=%p ignored because of overflow! %s(%d) %s\n", pvHint, pszFile, iLine, pszFunction));

#ifdef PGMRZDYNMAP_STRICT_RELEASE
    /*
     * Optimize the set to trigger the unmapping and invalidation of the page.
     */
    if (cUnrefs + 1 == cTotalRefs)
        pgmDynMapOptimizeAutoSet(pSet);
#endif
}


/**
 * Common worker code for pgmRZDynMapHCPageInlined, pgmRZDynMapHCPageV2Inlined
 * and pgmR0DynMapGCPageOffInlined.
 *
 * @returns VINF_SUCCESS, bails out to ring-3 on failure.
 * @param   pSet        The set.
 * @param   HCPhys      The physical address of the page.
 * @param   ppv         Where to store the address of the mapping on success.
 *
 * @remarks This is a very hot path.
 */
int pgmRZDynMapHCPageCommon(PPGMMAPSET pSet, RTHCPHYS HCPhys, void **ppv RTLOG_COMMA_SRC_POS_DECL)
{
    AssertMsg(pSet->iCpu == PGMRZDYNMAP_CUR_CPU(), ("%d %d efl=%#x\n", pSet->iCpu, PGMRZDYNMAP_CUR_CPU(), ASMGetFlags()));
    PVMCPU pVCpu = PGMRZDYNMAP_SET_2_VMCPU(pSet);
    STAM_PROFILE_START(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPage, a);

    /*
     * Map it.
     */
    void           *pvPage;
    PPGMRZDYNMAP    pThis = PGMRZDYNMAP_SET_2_DYNMAP(pSet);
    uint32_t        iPage = pgmR0DynMapPage(pThis, HCPhys, pSet->iCpu, pVCpu, &pvPage);
    if (RT_UNLIKELY(iPage == UINT32_MAX))
    {
        /*
         * We're out of mapping space, optimize our set to try remedy the
         * situation.  (Only works if there are unreference hints.)
         */
        STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapSetOptimize);
        pgmDynMapOptimizeAutoSet(pSet);

        iPage = pgmR0DynMapPage(pThis, HCPhys, pSet->iCpu, pVCpu, &pvPage);
        if (RT_UNLIKELY(iPage == UINT32_MAX))
        {
            RTAssertMsg2Weak("pgmRZDynMapHCPageCommon: cLoad=%u/%u cPages=%u cGuardPages=%u\n",
                             pThis->cLoad, pThis->cMaxLoad, pThis->cPages, pThis->cGuardPages);
            if (!g_fPGMR0DynMapTestRunning)
                VMMRZCallRing3NoCpu(PGMRZDYNMAP_SET_2_VM(pSet), VMMCALLRING3_VM_R0_ASSERTION, 0);
            *ppv = NULL;
            STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPage, a);
            return VERR_PGM_DYNMAP_FAILED;
        }
    }

    /*
     * Add the page to the auto reference set.
     *
     * The typical usage pattern means that the same pages will be mapped
     * several times in the same set. We can catch most of these
     * remappings by looking a few pages back into the set. (The searching
     * and set optimizing path will hardly ever be used when doing this.)
     */
    AssertCompile(RT_ELEMENTS(pSet->aEntries) >= 8);
    int32_t i = pSet->cEntries;
    if (i-- < 5)
    {
        unsigned iEntry = pSet->cEntries++;
        pSet->aEntries[iEntry].cRefs        = 1;
        pSet->aEntries[iEntry].cUnrefs      = 0;
        pSet->aEntries[iEntry].cInlinedRefs = 0;
        pSet->aEntries[iEntry].iPage        = iPage;
        pSet->aEntries[iEntry].pvPage       = pvPage;
        pSet->aEntries[iEntry].HCPhys       = HCPhys;
        pSet->aiHashTable[PGMMAPSET_HASH(HCPhys)] = iEntry;
        LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=%u/0/0 iPage=%#x  [a] %s(%d) %s\n",
                 pSet, HCPhys, iEntry, iEntry + 1, pvPage, 1, iPage, pszFile, iLine, pszFunction));
    }
    /* Any of the last 5 pages? */
    else if (   pSet->aEntries[i - 0].iPage == iPage
             && pSet->aEntries[i - 0].cRefs < UINT16_MAX - 1)
    {
        pSet->aEntries[i - 0].cRefs++;
        LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=%u/%u/%u iPage=%#x [0] %s(%d) %s\n", pSet, HCPhys, i - 0, pSet->cEntries, pvPage, pSet->aEntries[i - 0].cRefs, pSet->aEntries[i - 0].cInlinedRefs, pSet->aEntries[i - 0].cUnrefs, iPage, pszFile, iLine, pszFunction));
    }
    else if (   pSet->aEntries[i - 1].iPage == iPage
             && pSet->aEntries[i - 1].cRefs < UINT16_MAX - 1)
    {
        pSet->aEntries[i - 1].cRefs++;
        LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=%u/%u/%u iPage=%#x [1] %s(%d) %s\n", pSet, HCPhys, i - 1, pSet->cEntries, pvPage, pSet->aEntries[i - 1].cRefs, pSet->aEntries[i - 1].cInlinedRefs, pSet->aEntries[i - 1].cUnrefs, iPage, pszFile, iLine, pszFunction));
    }
    else if (   pSet->aEntries[i - 2].iPage == iPage
             && pSet->aEntries[i - 2].cRefs < UINT16_MAX - 1)
    {
        pSet->aEntries[i - 2].cRefs++;
        LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=%u/%u/%u iPage=%#x [2] %s(%d) %s\n", pSet, HCPhys, i - 2, pSet->cEntries, pvPage, pSet->aEntries[i - 2].cRefs, pSet->aEntries[i - 2].cInlinedRefs, pSet->aEntries[i - 2].cUnrefs, iPage, pszFile, iLine, pszFunction));
    }
    else if (   pSet->aEntries[i - 3].iPage == iPage
             && pSet->aEntries[i - 3].cRefs < UINT16_MAX - 1)
    {
        pSet->aEntries[i - 3].cRefs++;
        LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=%u/%u/%u iPage=%#x [4] %s(%d) %s\n", pSet, HCPhys, i - 3, pSet->cEntries, pvPage, pSet->aEntries[i - 3].cRefs, pSet->aEntries[i - 3].cInlinedRefs, pSet->aEntries[i - 3].cUnrefs, iPage, pszFile, iLine, pszFunction));
    }
    else if (   pSet->aEntries[i - 4].iPage == iPage
             && pSet->aEntries[i - 4].cRefs < UINT16_MAX - 1)
    {
        pSet->aEntries[i - 4].cRefs++;
        LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=%u/%u/%u iPage=%#x [4] %s(%d) %s\n", pSet, HCPhys, i - 4, pSet->cEntries, pvPage, pSet->aEntries[i - 4].cRefs, pSet->aEntries[i - 4].cInlinedRefs, pSet->aEntries[i - 4].cUnrefs, iPage, pszFile, iLine, pszFunction));
    }
    /* Don't bother searching unless we're above a 60% load. */
    else if (RT_LIKELY(i <= (int32_t)RT_ELEMENTS(pSet->aEntries) * 60 / 100))
    {
        unsigned iEntry = pSet->cEntries++;
        pSet->aEntries[iEntry].cRefs        = 1;
        pSet->aEntries[iEntry].cUnrefs      = 0;
        pSet->aEntries[iEntry].cInlinedRefs = 0;
        pSet->aEntries[iEntry].iPage        = iPage;
        pSet->aEntries[iEntry].pvPage       = pvPage;
        pSet->aEntries[iEntry].HCPhys       = HCPhys;
        pSet->aiHashTable[PGMMAPSET_HASH(HCPhys)] = iEntry;
        LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=1/0/0 iPage=%#x [b] %s(%d) %s\n", pSet, HCPhys, iEntry, pSet->cEntries, pvPage, iPage, pszFile, iLine, pszFunction));
    }
    else
    {
        /* Search the rest of the set. */
        Assert(pSet->cEntries <= RT_ELEMENTS(pSet->aEntries));
        i -= 4;
        while (i-- > 0)
            if (    pSet->aEntries[i].iPage == iPage
                &&  pSet->aEntries[i].cRefs < UINT16_MAX - 1)
            {
                pSet->aEntries[i].cRefs++;
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapSetSearchHits);
                LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=%u/%u/%u iPage=%#x [c] %s(%d) %s\n", pSet, HCPhys, i, pSet->cEntries, pvPage, pSet->aEntries[i].cRefs, pSet->aEntries[i].cInlinedRefs, pSet->aEntries[i].cUnrefs, iPage, pszFile, iLine, pszFunction));
                break;
            }
        if (i < 0)
        {
            STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapSetSearchMisses);
#if 0 /* this is very bogus */
            if (pSet->iSubset < pSet->cEntries)
            {
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapSetSearchFlushes);
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->aStatRZDynMapSetFilledPct[(pSet->cEntries * 10 / RT_ELEMENTS(pSet->aEntries)) % 11]);
                pgmDynMapFlushSubset(pSet);
            }
#endif

            if (RT_UNLIKELY(pSet->cEntries >= RT_ELEMENTS(pSet->aEntries)))
            {
                STAM_COUNTER_INC(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapSetOptimize);
                pgmDynMapOptimizeAutoSet(pSet);
            }

            if (RT_LIKELY(pSet->cEntries < RT_ELEMENTS(pSet->aEntries)))
            {
                unsigned iEntry = pSet->cEntries++;
                pSet->aEntries[iEntry].cRefs        = 1;
                pSet->aEntries[iEntry].cUnrefs      = 0;
                pSet->aEntries[iEntry].cInlinedRefs = 0;
                pSet->aEntries[iEntry].iPage        = iPage;
                pSet->aEntries[iEntry].pvPage       = pvPage;
                pSet->aEntries[iEntry].HCPhys       = HCPhys;
                pSet->aiHashTable[PGMMAPSET_HASH(HCPhys)] = iEntry;
                LogFlow(("pgmRZDynMapHCPageCommon: pSet=%p HCPhys=%RHp #%u/%u/%p cRefs=1/0/0 iPage=%#x [d] %s(%d) %s\n", pSet, HCPhys, iEntry, pSet->cEntries, pvPage, iPage, pszFile, iLine, pszFunction));
            }
            else
            {
                /* We're screwed. */
                pgmRZDynMapReleasePage(pThis, iPage, 1);

                RTAssertMsg2Weak("pgmRZDynMapHCPageCommon: set is full!\n");
                if (!g_fPGMR0DynMapTestRunning)
                    VMMRZCallRing3NoCpu(PGMRZDYNMAP_SET_2_VM(pSet), VMMCALLRING3_VM_R0_ASSERTION, 0);
                *ppv = NULL;
                STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPage, a);
                return VERR_PGM_DYNMAP_FULL_SET;
            }
        }
    }

    *ppv = pvPage;
    STAM_PROFILE_STOP(&pVCpu->pgm.s.CTX_SUFF(pStats)->StatRZDynMapHCPage, a);
    return VINF_SUCCESS;
}


#if 0 /*def DEBUG*/
/** For pgmR0DynMapTest3PerCpu. */
typedef struct PGMR0DYNMAPTEST
{
    uint32_t            u32Expect;
    uint32_t           *pu32;
    uint32_t volatile   cFailures;
} PGMR0DYNMAPTEST;
typedef PGMR0DYNMAPTEST *PPGMR0DYNMAPTEST;

/**
 * Checks that the content of the page is the same on all CPUs, i.e. that there
 * are no CPU specific PTs or similar nasty stuff involved.
 *
 * @param   idCpu           The current CPU.
 * @param   pvUser1         Pointer a PGMR0DYNMAPTEST structure.
 * @param   pvUser2         Unused, ignored.
 */
static DECLCALLBACK(void) pgmR0DynMapTest3PerCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PPGMR0DYNMAPTEST    pTest = (PPGMR0DYNMAPTEST)pvUser1;
    ASMInvalidatePage(pTest->pu32);
    if (*pTest->pu32 != pTest->u32Expect)
        ASMAtomicIncU32(&pTest->cFailures);
    NOREF(pvUser2); NOREF(idCpu);
}


/**
 * Performs some basic tests in debug builds.
 */
static int pgmR0DynMapTest(PVM pVM)
{
    LogRel(("pgmR0DynMapTest: ****** START ******\n"));
    PPGMMAPSET      pSet  = &pVM->aCpus[0].pgm.s.AutoSet;
    PPGMRZDYNMAP    pThis = PGMRZDYNMAP_SET_2_DYNMAP(pSet);
    uint32_t        i;

    /*
     * Assert internal integrity first.
     */
    LogRel(("Test #0\n"));
    int rc = PGMR0DynMapAssertIntegrity();
    if (RT_FAILURE(rc))
        return rc;

    void           *pvR0DynMapUsedSaved = pVM->pgm.s.pvR0DynMapUsed;
    pVM->pgm.s.pvR0DynMapUsed = pThis;
    g_fPGMR0DynMapTestRunning = true;

    /*
     * Simple test, map CR3 twice and check that we're getting the
     * same mapping address back.
     */
    LogRel(("Test #1\n"));
    ASMIntDisable();
    PGMRZDynMapStartAutoSet(&pVM->aCpus[0]);

    uint64_t cr3 = ASMGetCR3() & ~(uint64_t)PAGE_OFFSET_MASK;
    void    *pv  = (void *)(intptr_t)-1;
    void    *pv2 = (void *)(intptr_t)-2;
    rc           = pgmRZDynMapHCPageCommon(pVM, cr3, &pv  RTLOG_COMMA_SRC_POS);
    int      rc2 = pgmRZDynMapHCPageCommon(pVM, cr3, &pv2 RTLOG_COMMA_SRC_POS);
    ASMIntEnable();
    if (    RT_SUCCESS(rc2)
        &&  RT_SUCCESS(rc)
        &&  pv == pv2)
    {
        LogRel(("Load=%u/%u/%u Set=%u/%u\n", pThis->cLoad, pThis->cMaxLoad, pThis->cPages - pThis->cPages, pSet->cEntries, RT_ELEMENTS(pSet->aEntries)));
        rc = PGMR0DynMapAssertIntegrity();

        /*
         * Check that the simple set overflow code works by filling it
         * with more CR3 mappings.
         */
        LogRel(("Test #2\n"));
        ASMIntDisable();
        PGMR0DynMapMigrateAutoSet(&pVM->aCpus[0]);
        for (i = 0 ; i < UINT16_MAX*2 - 1 && RT_SUCCESS(rc) && pv2 == pv; i++)
        {
            pv2 = (void *)(intptr_t)-4;
            rc = pgmRZDynMapHCPageCommon(pVM, cr3, &pv2 RTLOG_COMMA_SRC_POS);
        }
        ASMIntEnable();
        if (RT_FAILURE(rc) || pv != pv2)
        {
            LogRel(("failed(%d): rc=%Rrc; pv=%p pv2=%p i=%p\n", __LINE__, rc, pv, pv2, i));
            if (RT_SUCCESS(rc)) rc = VERR_PGM_DYNMAP_IPE;
        }
        else if (pSet->cEntries != 5)
        {
            LogRel(("failed(%d): cEntries=%d expected %d\n", __LINE__, pSet->cEntries, RT_ELEMENTS(pSet->aEntries) / 2));
            rc = VERR_PGM_DYNMAP_IPE;
        }
        else if (   pSet->aEntries[4].cRefs != UINT16_MAX - 1
                 || pSet->aEntries[3].cRefs != UINT16_MAX - 1
                 || pSet->aEntries[2].cRefs != 1
                 || pSet->aEntries[1].cRefs != 1
                 || pSet->aEntries[0].cRefs != 1)
        {
            LogRel(("failed(%d): bad set dist: ", __LINE__));
            for (i = 0; i < pSet->cEntries; i++)
                LogRel(("[%d]=%d, ", i, pSet->aEntries[i].cRefs));
            LogRel(("\n"));
            rc = VERR_PGM_DYNMAP_IPE;
        }
        if (RT_SUCCESS(rc))
            rc = PGMR0DynMapAssertIntegrity();
        if (RT_SUCCESS(rc))
        {
            /*
             * Trigger an set optimization run (exactly).
             */
            LogRel(("Test #3\n"));
            ASMIntDisable();
            PGMR0DynMapMigrateAutoSet(&pVM->aCpus[0]);
            pv2 = NULL;
            for (i = 0 ; i < RT_ELEMENTS(pSet->aEntries) - 5 && RT_SUCCESS(rc) && pv2 != pv; i++)
            {
                pv2 = (void *)(intptr_t)(-5 - i);
                rc = pgmRZDynMapHCPageCommon(pVM, cr3 + PAGE_SIZE * (i + 5), &pv2 RTLOG_COMMA_SRC_POS);
            }
            ASMIntEnable();
            if (RT_FAILURE(rc) || pv == pv2)
            {
                LogRel(("failed(%d): rc=%Rrc; pv=%p pv2=%p i=%d\n", __LINE__, rc, pv, pv2, i));
                if (RT_SUCCESS(rc)) rc = VERR_PGM_DYNMAP_IPE;
            }
            else if (pSet->cEntries != RT_ELEMENTS(pSet->aEntries))
            {
                LogRel(("failed(%d): cEntries=%d expected %d\n", __LINE__, pSet->cEntries, RT_ELEMENTS(pSet->aEntries)));
                rc = VERR_PGM_DYNMAP_IPE;
            }
            LogRel(("Load=%u/%u/%u Set=%u/%u\n", pThis->cLoad, pThis->cMaxLoad, pThis->cPages - pThis->cPages, pSet->cEntries, RT_ELEMENTS(pSet->aEntries)));
            if (RT_SUCCESS(rc))
                rc = PGMR0DynMapAssertIntegrity();
            if (RT_SUCCESS(rc))
            {
                /*
                 * Trigger an overflow error.
                 */
                LogRel(("Test #4\n"));
                ASMIntDisable();
                PGMR0DynMapMigrateAutoSet(&pVM->aCpus[0]);
                for (i = 0 ; i < RT_ELEMENTS(pSet->aEntries) + 2; i++)
                {
                    rc = pgmRZDynMapHCPageCommon(pVM, cr3 - PAGE_SIZE * (i + 5), &pv2 RTLOG_COMMA_SRC_POS);
                    if (RT_SUCCESS(rc))
                        rc = PGMR0DynMapAssertIntegrity();
                    if (RT_FAILURE(rc))
                        break;
                }
                ASMIntEnable();
                if (rc == VERR_PGM_DYNMAP_FULL_SET)
                {
                    /* flush the set. */
                    LogRel(("Test #5\n"));
                    ASMIntDisable();
                    PGMR0DynMapMigrateAutoSet(&pVM->aCpus[0]);
                    PGMRZDynMapReleaseAutoSet(&pVM->aCpus[0]);
                    PGMRZDynMapStartAutoSet(&pVM->aCpus[0]);
                    ASMIntEnable();

                    rc = PGMR0DynMapAssertIntegrity();
                }
                else
                {
                    LogRel(("failed(%d): rc=%Rrc, wanted %d ; pv2=%p Set=%u/%u; i=%d\n", __LINE__,
                            rc, VERR_PGM_DYNMAP_FULL_SET, pv2, pSet->cEntries, RT_ELEMENTS(pSet->aEntries), i));
                    if (RT_SUCCESS(rc)) rc = VERR_PGM_DYNMAP_IPE;
                }
            }
        }
    }
    else
    {
        LogRel(("failed(%d): rc=%Rrc rc2=%Rrc; pv=%p pv2=%p\n", __LINE__, rc, rc2, pv, pv2));
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    /*
     * Check that everyone sees the same stuff.
     */
    if (RT_SUCCESS(rc))
    {
        LogRel(("Test #5\n"));
        ASMIntDisable();
        PGMR0DynMapMigrateAutoSet(&pVM->aCpus[0]);
        RTHCPHYS  HCPhysPT = RTR0MemObjGetPagePhysAddr(pThis->pSegHead->ahMemObjPTs[0], 0);
        rc  = pgmRZDynMapHCPageCommon(pVM, HCPhysPT, &pv RTLOG_COMMA_SRC_POS);
        if (RT_SUCCESS(rc))
        {
            PGMR0DYNMAPTEST Test;
            uint32_t       *pu32Real = &pThis->paPages[pThis->pSegHead->iPage].uPte.pLegacy->u;
            Test.pu32       = (uint32_t *)((uintptr_t)pv | ((uintptr_t)pu32Real & PAGE_OFFSET_MASK));
            Test.u32Expect  = *pu32Real;
            ASMAtomicWriteU32(&Test.cFailures, 0);
            ASMIntEnable();

            rc = RTMpOnAll(pgmR0DynMapTest3PerCpu, &Test, NULL);
            if (RT_FAILURE(rc))
                LogRel(("failed(%d): RTMpOnAll rc=%Rrc\n", __LINE__, rc));
            else if (Test.cFailures)
            {
                LogRel(("failed(%d): cFailures=%d pu32Real=%p pu32=%p u32Expect=%#x *pu32=%#x\n", __LINE__,
                        Test.cFailures, pu32Real, Test.pu32, Test.u32Expect, *Test.pu32));
                rc = VERR_PGM_DYNMAP_IPE;
            }
            else
                LogRel(("pu32Real=%p pu32=%p u32Expect=%#x *pu32=%#x\n",
                        pu32Real, Test.pu32, Test.u32Expect, *Test.pu32));
        }
        else
        {
            ASMIntEnable();
            LogRel(("failed(%d): rc=%Rrc\n", rc));
        }
    }

    /*
     * Clean up.
     */
    LogRel(("Cleanup.\n"));
    ASMIntDisable();
    PGMR0DynMapMigrateAutoSet(&pVM->aCpus[0]);
    PGMRZDynMapFlushAutoSet(&pVM->aCpus[0]);
    PGMRZDynMapReleaseAutoSet(&pVM->aCpus[0]);
    ASMIntEnable();

    if (RT_SUCCESS(rc))
        rc = PGMR0DynMapAssertIntegrity();
    else
        PGMR0DynMapAssertIntegrity();

    g_fPGMR0DynMapTestRunning = false;
    LogRel(("Result: rc=%Rrc Load=%u/%u/%u Set=%#x/%u\n", rc,
            pThis->cLoad, pThis->cMaxLoad, pThis->cPages - pThis->cPages, pSet->cEntries, RT_ELEMENTS(pSet->aEntries)));
    pVM->pgm.s.pvR0DynMapUsed = pvR0DynMapUsedSaved;
    LogRel(("pgmR0DynMapTest: ****** END ******\n"));
    return rc;
}
#endif /* DEBUG */


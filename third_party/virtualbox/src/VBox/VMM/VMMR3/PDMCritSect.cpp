/* $Id: PDMCritSect.cpp $ */
/** @file
 * PDM - Critical Sections, Ring-3.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_PDM//_CRITSECT
#include "PDMInternal.h"
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmcritsectrw.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>

#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/sup.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/lockvalidator.h>
#include <iprt/string.h>
#include <iprt/thread.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int pdmR3CritSectDeleteOne(PVM pVM, PUVM pUVM, PPDMCRITSECTINT pCritSect, PPDMCRITSECTINT pPrev, bool fFinal);
static int pdmR3CritSectRwDeleteOne(PVM pVM, PUVM pUVM, PPDMCRITSECTRWINT pCritSect, PPDMCRITSECTRWINT pPrev, bool fFinal);



/**
 * Register statistics related to the critical sections.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
int pdmR3CritSectBothInitStats(PVM pVM)
{
    RT_NOREF_PV(pVM);
    STAM_REG(pVM, &pVM->pdm.s.StatQueuedCritSectLeaves, STAMTYPE_COUNTER, "/PDM/QueuedCritSectLeaves", STAMUNIT_OCCURENCES,
             "Number of times a critical section leave request needed to be queued for ring-3 execution.");
    return VINF_SUCCESS;
}


/**
 * Relocates all the critical sections.
 *
 * @param   pVM         The cross context VM structure.
 */
void pdmR3CritSectBothRelocate(PVM pVM)
{
    PUVM pUVM = pVM->pUVM;
    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);

    for (PPDMCRITSECTINT pCur = pUVM->pdm.s.pCritSects;
         pCur;
         pCur = pCur->pNext)
        pCur->pVMRC = pVM->pVMRC;

    for (PPDMCRITSECTRWINT pCur = pUVM->pdm.s.pRwCritSects;
         pCur;
         pCur = pCur->pNext)
        pCur->pVMRC = pVM->pVMRC;

    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
}


/**
 * Deletes all remaining critical sections.
 *
 * This is called at the very end of the termination process.  It is also called
 * at the end of vmR3CreateU failure cleanup, which may cause it to be called
 * twice depending on where vmR3CreateU actually failed.  We have to do the
 * latter call because other components expect the critical sections to be
 * automatically deleted.
 *
 * @returns VBox status code.
 *          First error code, rest is lost.
 * @param   pVM             The cross context VM structure.
 * @remark  Don't confuse this with PDMR3CritSectDelete.
 */
VMMR3_INT_DECL(int) PDMR3CritSectBothTerm(PVM pVM)
{
    PUVM    pUVM = pVM->pUVM;
    int     rc   = VINF_SUCCESS;
    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);

    while (pUVM->pdm.s.pCritSects)
    {
        int rc2 = pdmR3CritSectDeleteOne(pVM, pUVM, pUVM->pdm.s.pCritSects, NULL, true /* final */);
        AssertRC(rc2);
        if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
            rc = rc2;
    }

    while (pUVM->pdm.s.pRwCritSects)
    {
        int rc2 = pdmR3CritSectRwDeleteOne(pVM, pUVM, pUVM->pdm.s.pRwCritSects, NULL, true /* final */);
        AssertRC(rc2);
        if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
            rc = rc2;
    }

    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
    return rc;
}


/**
 * Initializes a critical section and inserts it into the list.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pCritSect       The critical section.
 * @param   pvKey           The owner key.
 * @param   SRC_POS         The source position.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   va              Arguments for the format string.
 */
static int pdmR3CritSectInitOne(PVM pVM, PPDMCRITSECTINT pCritSect, void *pvKey, RT_SRC_POS_DECL,
                                const char *pszNameFmt, va_list va)
{
    VM_ASSERT_EMT(pVM);
    Assert(pCritSect->Core.u32Magic != RTCRITSECT_MAGIC);

    /*
     * Allocate the semaphore.
     */
    AssertCompile(sizeof(SUPSEMEVENT) == sizeof(pCritSect->Core.EventSem));
    int rc = SUPSemEventCreate(pVM->pSession, (PSUPSEMEVENT)&pCritSect->Core.EventSem);
    if (RT_SUCCESS(rc))
    {
        /* Only format the name once. */
        char *pszName = RTStrAPrintf2V(pszNameFmt, va); /** @todo plug the "leak"... */
        if (pszName)
        {
            RT_SRC_POS_NOREF();
#ifndef PDMCRITSECT_STRICT
            pCritSect->Core.pValidatorRec = NULL;
#else
            rc = RTLockValidatorRecExclCreate(&pCritSect->Core.pValidatorRec,
# ifdef RT_LOCK_STRICT_ORDER
                                              RTLockValidatorClassForSrcPos(RT_SRC_POS_ARGS, "%s", pszName),
# else
                                              NIL_RTLOCKVALCLASS,
# endif
                                              RTLOCKVAL_SUB_CLASS_NONE,
                                              pCritSect, true, "%s", pszName);
#endif
            if (RT_SUCCESS(rc))
            {
                /*
                 * Initialize the structure (first bit is c&p from RTCritSectInitEx).
                 */
                pCritSect->Core.u32Magic             = RTCRITSECT_MAGIC;
                pCritSect->Core.fFlags               = 0;
                pCritSect->Core.cNestings            = 0;
                pCritSect->Core.cLockers             = -1;
                pCritSect->Core.NativeThreadOwner    = NIL_RTNATIVETHREAD;
                pCritSect->pVMR3                     = pVM;
                pCritSect->pVMR0                     = pVM->pVMR0;
                pCritSect->pVMRC                     = pVM->pVMRC;
                pCritSect->pvKey                     = pvKey;
                pCritSect->fAutomaticDefaultCritsect = false;
                pCritSect->fUsedByTimerOrSimilar     = false;
                pCritSect->hEventToSignal            = NIL_SUPSEMEVENT;
                pCritSect->pszName                   = pszName;

                STAMR3RegisterF(pVM, &pCritSect->StatContentionRZLock,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSects/%s/ContentionRZLock", pCritSect->pszName);
                STAMR3RegisterF(pVM, &pCritSect->StatContentionRZUnlock,STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSects/%s/ContentionRZUnlock", pCritSect->pszName);
                STAMR3RegisterF(pVM, &pCritSect->StatContentionR3,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSects/%s/ContentionR3", pCritSect->pszName);
#ifdef VBOX_WITH_STATISTICS
                STAMR3RegisterF(pVM, &pCritSect->StatLocked,        STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_OCCURENCE, NULL, "/PDM/CritSects/%s/Locked", pCritSect->pszName);
#endif

                PUVM pUVM = pVM->pUVM;
                RTCritSectEnter(&pUVM->pdm.s.ListCritSect);
                pCritSect->pNext = pUVM->pdm.s.pCritSects;
                pUVM->pdm.s.pCritSects = pCritSect;
                RTCritSectLeave(&pUVM->pdm.s.ListCritSect);

                return VINF_SUCCESS;
            }

            RTStrFree(pszName);
        }
        else
            rc = VERR_NO_STR_MEMORY;
        SUPSemEventClose(pVM->pSession, (SUPSEMEVENT)pCritSect->Core.EventSem);
    }
    return rc;
}


/**
 * Initializes a read/write critical section and inserts it into the list.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pCritSect       The read/write critical section.
 * @param   pvKey           The owner key.
 * @param   SRC_POS         The source position.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   va              Arguments for the format string.
 */
static int pdmR3CritSectRwInitOne(PVM pVM, PPDMCRITSECTRWINT pCritSect, void *pvKey, RT_SRC_POS_DECL,
                                  const char *pszNameFmt, va_list va)
{
    VM_ASSERT_EMT(pVM);
    Assert(pCritSect->Core.u32Magic != RTCRITSECTRW_MAGIC);

    /*
     * Allocate the semaphores.
     */
    AssertCompile(sizeof(SUPSEMEVENT) == sizeof(pCritSect->Core.hEvtWrite));
    int rc = SUPSemEventCreate(pVM->pSession, (PSUPSEMEVENT)&pCritSect->Core.hEvtWrite);
    if (RT_SUCCESS(rc))
    {
        AssertCompile(sizeof(SUPSEMEVENTMULTI) == sizeof(pCritSect->Core.hEvtRead));
        rc = SUPSemEventMultiCreate(pVM->pSession, (PSUPSEMEVENT)&pCritSect->Core.hEvtRead);
        if (RT_SUCCESS(rc))
        {
            /* Only format the name once. */
            char *pszName = RTStrAPrintf2V(pszNameFmt, va); /** @todo plug the "leak"... */
            if (pszName)
            {
                pCritSect->Core.pValidatorRead  = NULL;
                pCritSect->Core.pValidatorWrite = NULL;
                RT_SRC_POS_NOREF();
#ifdef PDMCRITSECTRW_STRICT
# ifdef RT_LOCK_STRICT_ORDER
                RTLOCKVALCLASS hClass = RTLockValidatorClassForSrcPos(RT_SRC_POS_ARGS, "%s", pszName);
# else
                RTLOCKVALCLASS hClass = NIL_RTLOCKVALCLASS;
# endif
                rc = RTLockValidatorRecExclCreate(&pCritSect->Core.pValidatorWrite, hClass, RTLOCKVAL_SUB_CLASS_NONE,
                                                  pCritSect, true, "%s", pszName);
                if (RT_SUCCESS(rc))
                    rc = RTLockValidatorRecSharedCreate(&pCritSect->Core.pValidatorRead, hClass, RTLOCKVAL_SUB_CLASS_NONE,
                                                        pCritSect, false /*fSignaller*/, true, "%s", pszName);
#endif
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Initialize the structure (first bit is c&p from RTCritSectRwInitEx).
                     */
                    pCritSect->Core.u32Magic             = RTCRITSECTRW_MAGIC;
                    pCritSect->Core.fNeedReset           = false;
                    pCritSect->Core.u64State             = 0;
                    pCritSect->Core.hNativeWriter        = NIL_RTNATIVETHREAD;
                    pCritSect->Core.cWriterReads         = 0;
                    pCritSect->Core.cWriteRecursions     = 0;
#if HC_ARCH_BITS == 32
                    pCritSect->Core.HCPtrPadding         = NIL_RTHCPTR;
#endif
                    pCritSect->pVMR3                     = pVM;
                    pCritSect->pVMR0                     = pVM->pVMR0;
                    pCritSect->pVMRC                     = pVM->pVMRC;
                    pCritSect->pvKey                     = pvKey;
                    pCritSect->pszName                   = pszName;

                    STAMR3RegisterF(pVM, &pCritSect->StatContentionRZEnterExcl,   STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/ContentionRZEnterExcl", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatContentionRZLeaveExcl,   STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/ContentionRZLeaveExcl", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatContentionRZEnterShared, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/ContentionRZEnterShared", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatContentionRZLeaveShared, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/ContentionRZLeaveShared", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatContentionR3EnterExcl,   STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/ContentionR3EnterExcl", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatContentionR3EnterShared, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/ContentionR3EnterShared", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatRZEnterExcl,             STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/RZEnterExcl", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatRZEnterShared,           STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/RZEnterShared", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatR3EnterExcl,             STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/R3EnterExcl", pCritSect->pszName);
                    STAMR3RegisterF(pVM, &pCritSect->StatR3EnterShared,           STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,          NULL, "/PDM/CritSectsRw/%s/R3EnterShared", pCritSect->pszName);
#ifdef VBOX_WITH_STATISTICS
                    STAMR3RegisterF(pVM, &pCritSect->StatWriteLocked,         STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_OCCURENCE, NULL, "/PDM/CritSectsRw/%s/WriteLocked", pCritSect->pszName);
#endif

                    PUVM pUVM = pVM->pUVM;
                    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);
                    pCritSect->pNext = pUVM->pdm.s.pRwCritSects;
                    pUVM->pdm.s.pRwCritSects = pCritSect;
                    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);

                    return VINF_SUCCESS;
                }

                RTStrFree(pszName);
            }
            else
                rc = VERR_NO_STR_MEMORY;
            SUPSemEventMultiClose(pVM->pSession, (SUPSEMEVENT)pCritSect->Core.hEvtRead);
        }
        SUPSemEventClose(pVM->pSession, (SUPSEMEVENT)pCritSect->Core.hEvtWrite);
    }
    return rc;
}


/**
 * Initializes a PDM critical section for internal use.
 *
 * The PDM critical sections are derived from the IPRT critical sections, but
 * works in ring-0 and raw-mode context as well.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pCritSect       Pointer to the critical section.
 * @param   SRC_POS         Use RT_SRC_POS.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   ...             Arguments for the format string.
 * @thread  EMT
 */
VMMR3DECL(int) PDMR3CritSectInit(PVM pVM, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL, const char *pszNameFmt, ...)
{
#if HC_ARCH_BITS == 64 && GC_ARCH_BITS == 32
    AssertCompile(sizeof(pCritSect->padding) >= sizeof(pCritSect->s));
#endif
    Assert(RT_ALIGN_P(pCritSect, sizeof(uintptr_t)) == pCritSect);
    va_list va;
    va_start(va, pszNameFmt);
    int rc = pdmR3CritSectInitOne(pVM, &pCritSect->s, pCritSect, RT_SRC_POS_ARGS, pszNameFmt, va);
    va_end(va);
    return rc;
}


/**
 * Initializes a PDM read/write critical section for internal use.
 *
 * The PDM read/write critical sections are derived from the IPRT read/write
 * critical sections, but works in ring-0 and raw-mode context as well.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pCritSect       Pointer to the read/write critical section.
 * @param   SRC_POS         Use RT_SRC_POS.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   ...             Arguments for the format string.
 * @thread  EMT
 */
VMMR3DECL(int) PDMR3CritSectRwInit(PVM pVM, PPDMCRITSECTRW pCritSect, RT_SRC_POS_DECL, const char *pszNameFmt, ...)
{
#if HC_ARCH_BITS == 64 && GC_ARCH_BITS == 32
    AssertCompile(sizeof(pCritSect->padding) >= sizeof(pCritSect->s));
#endif
    Assert(RT_ALIGN_P(pCritSect, sizeof(uintptr_t)) == pCritSect);
    va_list va;
    va_start(va, pszNameFmt);
    int rc = pdmR3CritSectRwInitOne(pVM, &pCritSect->s, pCritSect, RT_SRC_POS_ARGS, pszNameFmt, va);
    va_end(va);
    return rc;
}


/**
 * Initializes a PDM critical section for a device.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         Device instance.
 * @param   pCritSect       Pointer to the critical section.
 * @param   SRC_POS         The source position.  Optional.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   va              Arguments for the format string.
 */
int pdmR3CritSectInitDevice(PVM pVM, PPDMDEVINS pDevIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL,
                            const char *pszNameFmt, va_list va)
{
    return pdmR3CritSectInitOne(pVM, &pCritSect->s, pDevIns, RT_SRC_POS_ARGS, pszNameFmt, va);
}


/**
 * Initializes a PDM read/write critical section for a device.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         Device instance.
 * @param   pCritSect       Pointer to the read/write critical section.
 * @param   SRC_POS         The source position.  Optional.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   va              Arguments for the format string.
 */
int pdmR3CritSectRwInitDevice(PVM pVM, PPDMDEVINS pDevIns, PPDMCRITSECTRW pCritSect, RT_SRC_POS_DECL,
                              const char *pszNameFmt, va_list va)
{
    return pdmR3CritSectRwInitOne(pVM, &pCritSect->s, pDevIns, RT_SRC_POS_ARGS, pszNameFmt, va);
}


/**
 * Initializes the automatic default PDM critical section for a device.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         Device instance.
 * @param   SRC_POS         The source position.  Optional.
 * @param   pCritSect       Pointer to the critical section.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   ...             Arguments for the format string.
 */
int pdmR3CritSectInitDeviceAuto(PVM pVM, PPDMDEVINS pDevIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL,
                                const char *pszNameFmt, ...)
{
    va_list va;
    va_start(va, pszNameFmt);
    int rc = pdmR3CritSectInitOne(pVM, &pCritSect->s, pDevIns, RT_SRC_POS_ARGS, pszNameFmt, va);
    if (RT_SUCCESS(rc))
        pCritSect->s.fAutomaticDefaultCritsect = true;
    va_end(va);
    return rc;
}


/**
 * Initializes a PDM critical section for a driver.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDrvIns         Driver instance.
 * @param   pCritSect       Pointer to the critical section.
 * @param   SRC_POS         The source position.  Optional.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   ...             Arguments for the format string.
 */
int pdmR3CritSectInitDriver(PVM pVM, PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL,
                            const char *pszNameFmt, ...)
{
    va_list va;
    va_start(va, pszNameFmt);
    int rc = pdmR3CritSectInitOne(pVM, &pCritSect->s, pDrvIns, RT_SRC_POS_ARGS, pszNameFmt, va);
    va_end(va);
    return rc;
}


/**
 * Initializes a PDM read/write critical section for a driver.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pDrvIns         Driver instance.
 * @param   pCritSect       Pointer to the read/write critical section.
 * @param   SRC_POS         The source position.  Optional.
 * @param   pszNameFmt      Format string for naming the critical section.  For
 *                          statistics and lock validation.
 * @param   ...             Arguments for the format string.
 */
int pdmR3CritSectRwInitDriver(PVM pVM, PPDMDRVINS pDrvIns, PPDMCRITSECTRW pCritSect, RT_SRC_POS_DECL,
                              const char *pszNameFmt, ...)
{
    va_list va;
    va_start(va, pszNameFmt);
    int rc = pdmR3CritSectRwInitOne(pVM, &pCritSect->s, pDrvIns, RT_SRC_POS_ARGS, pszNameFmt, va);
    va_end(va);
    return rc;
}


/**
 * Deletes one critical section.
 *
 * @returns Return code from RTCritSectDelete.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pUVM        The user mode VM handle.
 * @param   pCritSect   The critical section.
 * @param   pPrev       The previous critical section in the list.
 * @param   fFinal      Set if this is the final call and statistics shouldn't be deregistered.
 *
 * @remarks Caller must have entered the ListCritSect.
 */
static int pdmR3CritSectDeleteOne(PVM pVM, PUVM pUVM, PPDMCRITSECTINT pCritSect, PPDMCRITSECTINT pPrev, bool fFinal)
{
    /*
     * Assert free waiters and so on (c&p from RTCritSectDelete).
     */
    Assert(pCritSect->Core.u32Magic == RTCRITSECT_MAGIC);
    //Assert(pCritSect->Core.cNestings == 0); - we no longer reset this when leaving.
    Assert(pCritSect->Core.cLockers == -1);
    Assert(pCritSect->Core.NativeThreadOwner == NIL_RTNATIVETHREAD);
    Assert(RTCritSectIsOwner(&pUVM->pdm.s.ListCritSect));

    /*
     * Unlink it.
     */
    if (pPrev)
        pPrev->pNext = pCritSect->pNext;
    else
        pUVM->pdm.s.pCritSects = pCritSect->pNext;

    /*
     * Delete it (parts taken from RTCritSectDelete).
     * In case someone is waiting we'll signal the semaphore cLockers + 1 times.
     */
    ASMAtomicWriteU32(&pCritSect->Core.u32Magic, 0);
    SUPSEMEVENT hEvent = (SUPSEMEVENT)pCritSect->Core.EventSem;
    pCritSect->Core.EventSem = NIL_RTSEMEVENT;
    while (pCritSect->Core.cLockers-- >= 0)
        SUPSemEventSignal(pVM->pSession, hEvent);
    ASMAtomicWriteS32(&pCritSect->Core.cLockers, -1);
    int rc = SUPSemEventClose(pVM->pSession, hEvent);
    AssertRC(rc);
    RTLockValidatorRecExclDestroy(&pCritSect->Core.pValidatorRec);
    pCritSect->pNext   = NULL;
    pCritSect->pvKey   = NULL;
    pCritSect->pVMR3   = NULL;
    pCritSect->pVMR0   = NIL_RTR0PTR;
    pCritSect->pVMRC   = NIL_RTRCPTR;
    if (!fFinal)
        STAMR3DeregisterF(pVM->pUVM, "/PDM/CritSects/%s/*", pCritSect->pszName);
    RTStrFree((char *)pCritSect->pszName);
    pCritSect->pszName = NULL;
    return rc;
}


/**
 * Deletes one read/write critical section.
 *
 * @returns VBox status code.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pUVM        The user mode VM handle.
 * @param   pCritSect   The read/write critical section.
 * @param   pPrev       The previous critical section in the list.
 * @param   fFinal      Set if this is the final call and statistics shouldn't be deregistered.
 *
 * @remarks Caller must have entered the ListCritSect.
 */
static int pdmR3CritSectRwDeleteOne(PVM pVM, PUVM pUVM, PPDMCRITSECTRWINT pCritSect, PPDMCRITSECTRWINT pPrev, bool fFinal)
{
    /*
     * Assert free waiters and so on (c&p from RTCritSectRwDelete).
     */
    Assert(pCritSect->Core.u32Magic == RTCRITSECTRW_MAGIC);
    //Assert(pCritSect->Core.cNestings == 0);
    //Assert(pCritSect->Core.cLockers == -1);
    Assert(pCritSect->Core.hNativeWriter == NIL_RTNATIVETHREAD);

    /*
     * Invalidate the structure and free the semaphores.
     */
    if (!ASMAtomicCmpXchgU32(&pCritSect->Core.u32Magic, RTCRITSECTRW_MAGIC_DEAD, RTCRITSECTRW_MAGIC))
        AssertFailed();

    /*
     * Unlink it.
     */
    if (pPrev)
        pPrev->pNext = pCritSect->pNext;
    else
        pUVM->pdm.s.pRwCritSects = pCritSect->pNext;

    /*
     * Delete it (parts taken from RTCritSectRwDelete).
     * In case someone is waiting we'll signal the semaphore cLockers + 1 times.
     */
    pCritSect->Core.fFlags   = 0;
    pCritSect->Core.u64State = 0;

    SUPSEMEVENT      hEvtWrite = (SUPSEMEVENT)pCritSect->Core.hEvtWrite;
    pCritSect->Core.hEvtWrite  = NIL_RTSEMEVENT;
    AssertCompile(sizeof(hEvtWrite) == sizeof(pCritSect->Core.hEvtWrite));

    SUPSEMEVENTMULTI hEvtRead  = (SUPSEMEVENTMULTI)pCritSect->Core.hEvtRead;
    pCritSect->Core.hEvtRead   = NIL_RTSEMEVENTMULTI;
    AssertCompile(sizeof(hEvtRead) == sizeof(pCritSect->Core.hEvtRead));

    int rc1 = SUPSemEventClose(pVM->pSession, hEvtWrite);     AssertRC(rc1);
    int rc2 = SUPSemEventMultiClose(pVM->pSession, hEvtRead); AssertRC(rc2);

    RTLockValidatorRecSharedDestroy(&pCritSect->Core.pValidatorRead);
    RTLockValidatorRecExclDestroy(&pCritSect->Core.pValidatorWrite);

    pCritSect->pNext   = NULL;
    pCritSect->pvKey   = NULL;
    pCritSect->pVMR3   = NULL;
    pCritSect->pVMR0   = NIL_RTR0PTR;
    pCritSect->pVMRC   = NIL_RTRCPTR;
    if (!fFinal)
        STAMR3DeregisterF(pVM->pUVM, "/PDM/CritSectsRw/%s/*", pCritSect->pszName);
    RTStrFree((char *)pCritSect->pszName);
    pCritSect->pszName = NULL;

    return RT_SUCCESS(rc1) ? rc2 : rc1;
}


/**
 * Deletes all critical sections with a give initializer key.
 *
 * @returns VBox status code.
 *          The entire list is processed on failure, so we'll only
 *          return the first error code. This shouldn't be a problem
 *          since errors really shouldn't happen here.
 * @param   pVM     The cross context VM structure.
 * @param   pvKey   The initializer key.
 */
static int pdmR3CritSectDeleteByKey(PVM pVM, void *pvKey)
{
    /*
     * Iterate the list and match key.
     */
    PUVM            pUVM  = pVM->pUVM;
    int             rc    = VINF_SUCCESS;
    PPDMCRITSECTINT pPrev = NULL;
    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);
    PPDMCRITSECTINT pCur  = pUVM->pdm.s.pCritSects;
    while (pCur)
    {
        if (pCur->pvKey == pvKey)
        {
            int rc2 = pdmR3CritSectDeleteOne(pVM, pUVM, pCur, pPrev, false /* not final */);
            AssertRC(rc2);
            if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                rc = rc2;
        }

        /* next */
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
    return rc;
}


/**
 * Deletes all read/write critical sections with a give initializer key.
 *
 * @returns VBox status code.
 *          The entire list is processed on failure, so we'll only
 *          return the first error code. This shouldn't be a problem
 *          since errors really shouldn't happen here.
 * @param   pVM     The cross context VM structure.
 * @param   pvKey   The initializer key.
 */
static int pdmR3CritSectRwDeleteByKey(PVM pVM, void *pvKey)
{
    /*
     * Iterate the list and match key.
     */
    PUVM                pUVM  = pVM->pUVM;
    int                 rc    = VINF_SUCCESS;
    PPDMCRITSECTRWINT   pPrev = NULL;
    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);
    PPDMCRITSECTRWINT   pCur  = pUVM->pdm.s.pRwCritSects;
    while (pCur)
    {
        if (pCur->pvKey == pvKey)
        {
            int rc2 = pdmR3CritSectRwDeleteOne(pVM, pUVM, pCur, pPrev, false /* not final */);
            AssertRC(rc2);
            if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                rc = rc2;
        }

        /* next */
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
    return rc;
}


/**
 * Deletes all undeleted critical sections (both types) initialized by a given
 * device.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDevIns     The device handle.
 */
int pdmR3CritSectBothDeleteDevice(PVM pVM, PPDMDEVINS pDevIns)
{
    int rc1 = pdmR3CritSectDeleteByKey(pVM, pDevIns);
    int rc2 = pdmR3CritSectRwDeleteByKey(pVM, pDevIns);
    return RT_SUCCESS(rc1) ? rc2 : rc1;
}


/**
 * Deletes all undeleted critical sections (both types) initialized by a given
 * driver.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDrvIns     The driver handle.
 */
int pdmR3CritSectBothDeleteDriver(PVM pVM, PPDMDRVINS pDrvIns)
{
    int rc1 = pdmR3CritSectDeleteByKey(pVM, pDrvIns);
    int rc2 = pdmR3CritSectRwDeleteByKey(pVM, pDrvIns);
    return RT_SUCCESS(rc1) ? rc2 : rc1;
}


/**
 * Deletes the critical section.
 *
 * @returns VBox status code.
 * @param   pCritSect           The PDM critical section to destroy.
 */
VMMR3DECL(int) PDMR3CritSectDelete(PPDMCRITSECT pCritSect)
{
    if (!RTCritSectIsInitialized(&pCritSect->s.Core))
        return VINF_SUCCESS;

    /*
     * Find and unlink it.
     */
    PVM             pVM   = pCritSect->s.pVMR3;
    PUVM            pUVM  = pVM->pUVM;
    AssertReleaseReturn(pVM, VERR_PDM_CRITSECT_IPE);
    PPDMCRITSECTINT pPrev = NULL;
    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);
    PPDMCRITSECTINT pCur  = pUVM->pdm.s.pCritSects;
    while (pCur)
    {
        if (pCur == &pCritSect->s)
        {
            int rc = pdmR3CritSectDeleteOne(pVM, pUVM, pCur, pPrev, false /* not final */);
            RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
            return rc;
        }

        /* next */
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
    AssertReleaseMsgFailed(("pCritSect=%p wasn't found!\n", pCritSect));
    return VERR_PDM_CRITSECT_NOT_FOUND;
}


/**
 * Deletes the read/write critical section.
 *
 * @returns VBox status code.
 * @param   pCritSect           The PDM read/write critical section to destroy.
 */
VMMR3DECL(int) PDMR3CritSectRwDelete(PPDMCRITSECTRW pCritSect)
{
    if (!PDMCritSectRwIsInitialized(pCritSect))
        return VINF_SUCCESS;

    /*
     * Find and unlink it.
     */
    PVM                 pVM   = pCritSect->s.pVMR3;
    PUVM                pUVM  = pVM->pUVM;
    AssertReleaseReturn(pVM, VERR_PDM_CRITSECT_IPE);
    PPDMCRITSECTRWINT   pPrev = NULL;
    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);
    PPDMCRITSECTRWINT   pCur  = pUVM->pdm.s.pRwCritSects;
    while (pCur)
    {
        if (pCur == &pCritSect->s)
        {
            int rc = pdmR3CritSectRwDeleteOne(pVM, pUVM, pCur, pPrev, false /* not final */);
            RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
            return rc;
        }

        /* next */
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
    AssertReleaseMsgFailed(("pCritSect=%p wasn't found!\n", pCritSect));
    return VERR_PDM_CRITSECT_NOT_FOUND;
}


/**
 * Gets the name of the critical section.
 *
 *
 * @returns Pointer to the critical section name (read only) on success,
 *          NULL on failure (invalid critical section).
 * @param   pCritSect           The critical section.
 */
VMMR3DECL(const char *) PDMR3CritSectName(PCPDMCRITSECT pCritSect)
{
    AssertPtrReturn(pCritSect, NULL);
    AssertReturn(pCritSect->s.Core.u32Magic == RTCRITSECT_MAGIC, NULL);
    return pCritSect->s.pszName;
}


/**
 * Gets the name of the read/write critical section.
 *
 *
 * @returns Pointer to the critical section name (read only) on success,
 *          NULL on failure (invalid critical section).
 * @param   pCritSect           The read/write critical section.
 */
VMMR3DECL(const char *) PDMR3CritSectRwName(PCPDMCRITSECTRW pCritSect)
{
    AssertPtrReturn(pCritSect, NULL);
    AssertReturn(pCritSect->s.Core.u32Magic == RTCRITSECTRW_MAGIC, NULL);
    return pCritSect->s.pszName;
}


/**
 * Yield the critical section if someone is waiting on it.
 *
 * When yielding, we'll leave the critical section and try to make sure the
 * other waiting threads get a chance of entering before we reclaim it.
 *
 * @retval  true if yielded.
 * @retval  false if not yielded.
 * @param   pCritSect           The critical section.
 */
VMMR3DECL(bool) PDMR3CritSectYield(PPDMCRITSECT pCritSect)
{
    AssertPtrReturn(pCritSect, false);
    AssertReturn(pCritSect->s.Core.u32Magic == RTCRITSECT_MAGIC, false);
    Assert(pCritSect->s.Core.NativeThreadOwner == RTThreadNativeSelf());
    Assert(!(pCritSect->s.Core.fFlags & RTCRITSECT_FLAGS_NOP));

    /* No recursion allowed here. */
    int32_t const cNestings = pCritSect->s.Core.cNestings;
    AssertReturn(cNestings == 1, false);

    int32_t const cLockers  = ASMAtomicReadS32(&pCritSect->s.Core.cLockers);
    if (cLockers < cNestings)
        return false;

#ifdef PDMCRITSECT_STRICT
    RTLOCKVALSRCPOS const SrcPos = pCritSect->s.Core.pValidatorRec->SrcPos;
#endif
    PDMCritSectLeave(pCritSect);

    /*
     * If we're lucky, then one of the waiters has entered the lock already.
     * We spin a little bit in hope for this to happen so we can avoid the
     * yield detour.
     */
    if (ASMAtomicUoReadS32(&pCritSect->s.Core.cNestings) == 0)
    {
        int cLoops = 20;
        while (   cLoops > 0
               && ASMAtomicUoReadS32(&pCritSect->s.Core.cNestings) == 0
               && ASMAtomicUoReadS32(&pCritSect->s.Core.cLockers)  >= 0)
        {
            ASMNopPause();
            cLoops--;
        }
        if (cLoops == 0)
            RTThreadYield();
    }

#ifdef PDMCRITSECT_STRICT
    int rc = PDMCritSectEnterDebug(pCritSect, VERR_IGNORED,
                                   SrcPos.uId, SrcPos.pszFile, SrcPos.uLine, SrcPos.pszFunction);
#else
    int rc = PDMCritSectEnter(pCritSect, VERR_IGNORED);
#endif
    AssertLogRelRC(rc);
    return true;
}


/**
 * PDMR3CritSectBothCountOwned worker.
 *
 * @param   pszName         The critical section name.
 * @param   ppszNames       Pointer to the pszNames variable.
 * @param   pcchLeft        Pointer to the cchLeft variable.
 * @param   fFirst          Whether this is the first name or not.
 */
static void pdmR3CritSectAppendNameToList(char const *pszName, char **ppszNames, size_t *pcchLeft, bool fFirst)
{
    size_t cchLeft = *pcchLeft;
    if (cchLeft)
    {
        char *pszNames = *ppszNames;

        /* try add comma. */
        if (fFirst)
        {
            *pszNames++ = ',';
            if (--cchLeft)
            {
                *pszNames++ = ' ';
                cchLeft--;
            }
        }

        /* try copy the name. */
        if (cchLeft)
        {
            size_t const cchName = strlen(pszName);
            if (cchName < cchLeft)
            {
                memcpy(pszNames, pszName, cchName);
                pszNames += cchName;
                cchLeft -= cchName;
            }
            else
            {
                if (cchLeft > 2)
                {
                    memcpy(pszNames, pszName, cchLeft - 2);
                    pszNames += cchLeft - 2;
                    cchLeft = 2;
                }
                while (cchLeft-- > 0)
                    *pszNames++ = '+';
            }
        }
        *pszNames = '\0';

        *pcchLeft  = cchLeft;
        *ppszNames = pszNames;
    }
}


/**
 * Counts the critical sections (both type) owned by the calling thread,
 * optionally returning a comma separated list naming them.
 *
 * Read ownerships are not included in non-strict builds.
 *
 * This is for diagnostic purposes only.
 *
 * @returns Lock count.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pszNames        Where to return the critical section names.
 * @param   cbNames         The size of the buffer.
 */
VMMR3DECL(uint32_t) PDMR3CritSectCountOwned(PVM pVM, char *pszNames, size_t cbNames)
{
    /*
     * Init the name buffer.
     */
    size_t cchLeft = cbNames;
    if (cchLeft)
    {
        cchLeft--;
        pszNames[0] = pszNames[cchLeft] = '\0';
    }

    /*
     * Iterate the critical sections.
     */
    uint32_t                cCritSects = 0;
    RTNATIVETHREAD const    hNativeThread = RTThreadNativeSelf();
    /* This is unsafe, but wtf. */
    for (PPDMCRITSECTINT pCur = pVM->pUVM->pdm.s.pCritSects;
         pCur;
         pCur = pCur->pNext)
    {
        /* Same as RTCritSectIsOwner(). */
        if (pCur->Core.NativeThreadOwner == hNativeThread)
        {
            cCritSects++;
            pdmR3CritSectAppendNameToList(pCur->pszName, &pszNames, &cchLeft, cCritSects == 1);
        }
    }

    /* This is unsafe, but wtf. */
    for (PPDMCRITSECTRWINT pCur = pVM->pUVM->pdm.s.pRwCritSects;
         pCur;
         pCur = pCur->pNext)
    {
        if (   pCur->Core.hNativeWriter == hNativeThread
            || PDMCritSectRwIsReadOwner((PPDMCRITSECTRW)pCur, false /*fWannaHear*/) )
        {
            cCritSects++;
            pdmR3CritSectAppendNameToList(pCur->pszName, &pszNames, &cchLeft, cCritSects == 1);
        }
    }

    return cCritSects;
}


/**
 * Leave all critical sections the calling thread owns.
 *
 * This is only used when entering guru meditation in order to prevent other
 * EMTs and I/O threads from deadlocking.
 *
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(void) PDMR3CritSectLeaveAll(PVM pVM)
{
    RTNATIVETHREAD const hNativeSelf = RTThreadNativeSelf();
    PUVM                 pUVM        = pVM->pUVM;

    RTCritSectEnter(&pUVM->pdm.s.ListCritSect);
    for (PPDMCRITSECTINT pCur = pUVM->pdm.s.pCritSects;
         pCur;
         pCur = pCur->pNext)
    {
        while (     pCur->Core.NativeThreadOwner == hNativeSelf
               &&   pCur->Core.cNestings > 0)
            PDMCritSectLeave((PPDMCRITSECT)pCur);
    }
    RTCritSectLeave(&pUVM->pdm.s.ListCritSect);
}


/**
 * Gets the address of the NOP critical section.
 *
 * The NOP critical section will not perform any thread serialization but let
 * all enter immediately and concurrently.
 *
 * @returns The address of the NOP critical section.
 * @param   pVM                 The cross context VM structure.
 */
VMMR3DECL(PPDMCRITSECT)             PDMR3CritSectGetNop(PVM pVM)
{
    VM_ASSERT_VALID_EXT_RETURN(pVM, NULL);
    return &pVM->pdm.s.NopCritSect;
}


/**
 * Gets the ring-0 address of the NOP critical section.
 *
 * @returns The ring-0 address of the NOP critical section.
 * @param   pVM                 The cross context VM structure.
 */
VMMR3DECL(R0PTRTYPE(PPDMCRITSECT))  PDMR3CritSectGetNopR0(PVM pVM)
{
    VM_ASSERT_VALID_EXT_RETURN(pVM, NIL_RTR0PTR);
    return MMHyperR3ToR0(pVM, &pVM->pdm.s.NopCritSect);
}


/**
 * Gets the raw-mode context address of the NOP critical section.
 *
 * @returns The raw-mode context address of the NOP critical section.
 * @param   pVM                 The cross context VM structure.
 */
VMMR3DECL(RCPTRTYPE(PPDMCRITSECT))  PDMR3CritSectGetNopRC(PVM pVM)
{
    VM_ASSERT_VALID_EXT_RETURN(pVM, NIL_RTRCPTR);
    return MMHyperR3ToRC(pVM, &pVM->pdm.s.NopCritSect);
}


/** @file
 * TM - Time Manager.
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

#ifndef ___VBox_vmm_tm_h
#define ___VBox_vmm_tm_h

#include <VBox/types.h>
#ifdef IN_RING3
# include <iprt/time.h>
#endif

RT_C_DECLS_BEGIN

/** @defgroup grp_tm        The Time Manager API
 * @ingroup grp_vmm
 * @{
 */

/** Enable a timer hack which improves the timer response/resolution a bit. */
#define VBOX_HIGH_RES_TIMERS_HACK


/**
 * Clock type.
 */
typedef enum TMCLOCK
{
    /** Real host time.
     * This clock ticks all the time, so use with care. */
    TMCLOCK_REAL = 0,
    /** Virtual guest time.
     * This clock only ticks when the guest is running.  It's implemented
     * as an offset to monotonic real time (GIP). */
    TMCLOCK_VIRTUAL,
    /** Virtual guest synchronized timer time.
     * This is a special clock and timer queue for synchronizing virtual timers
     * and virtual time sources.  This clock is trying to keep up with
     * TMCLOCK_VIRTUAL, but will wait for timers to be executed.  If it lags
     * too far behind TMCLOCK_VIRTUAL, it will try speed up to close the
     * distance.
     * @remarks Do not use this unless you really *must*. */
    TMCLOCK_VIRTUAL_SYNC,
    /** Virtual CPU timestamp.
     * By default this is a function of TMCLOCK_VIRTUAL_SYNC and the virtual
     * CPU frequency. */
    TMCLOCK_TSC,
    /** Number of clocks. */
    TMCLOCK_MAX
} TMCLOCK;


/** @defgroup grp_tm_timer_flags Timer flags.
 * @{ */
/** Use the default critical section for the class of timers. */
#define TMTIMER_FLAGS_DEFAULT_CRIT_SECT 0
/** No critical section needed or a custom one is set using
 *  TMR3TimerSetCritSect(). */
#define TMTIMER_FLAGS_NO_CRIT_SECT      RT_BIT_32(0)
/** @} */


VMMDECL(void)           TMNotifyStartOfExecution(PVMCPU pVCpu);
VMMDECL(void)           TMNotifyEndOfExecution(PVMCPU pVCpu);
VMM_INT_DECL(void)      TMNotifyStartOfHalt(PVMCPU pVCpu);
VMM_INT_DECL(void)      TMNotifyEndOfHalt(PVMCPU pVCpu);
#ifdef IN_RING3
VMMR3DECL(int)          TMR3NotifySuspend(PVM pVM, PVMCPU pVCpu);
VMMR3DECL(int)          TMR3NotifyResume(PVM pVM, PVMCPU pVCpu);
VMMR3DECL(int)          TMR3SetWarpDrive(PUVM pUVM, uint32_t u32Percent);
VMMR3DECL(uint32_t)     TMR3GetWarpDrive(PUVM pUVM);
#endif
VMM_INT_DECL(uint32_t)  TMCalcHostTimerFrequency(PVM pVM, PVMCPU pVCpu);
#ifdef IN_RING3
VMMR3DECL(int)          TMR3GetCpuLoadTimes(PVM pVM, VMCPUID idCpu, uint64_t *pcNsTotal, uint64_t *pcNsExecuting,
                                            uint64_t *pcNsHalted, uint64_t *pcNsOther);
#endif


/** @name Real Clock Methods
 * @{
 */
VMM_INT_DECL(uint64_t)  TMRealGet(PVM pVM);
VMM_INT_DECL(uint64_t)  TMRealGetFreq(PVM pVM);
/** @} */


/** @name Virtual Clock Methods
 * @{
 */
VMM_INT_DECL(uint64_t)  TMVirtualGet(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualGetNoCheck(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetLag(PVM pVM);
VMM_INT_DECL(uint32_t)  TMVirtualSyncGetCatchUpPct(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualGetFreq(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGet(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetNoCheck(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetEx(PVM pVM, bool fCheckTimers);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetWithDeadlineNoCheck(PVM pVM, uint64_t *pcNsToDeadline);
VMMDECL(uint64_t)       TMVirtualSyncGetNsToDeadline(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualToNano(PVM pVM, uint64_t u64VirtualTicks);
VMM_INT_DECL(uint64_t)  TMVirtualToMicro(PVM pVM, uint64_t u64VirtualTicks);
VMM_INT_DECL(uint64_t)  TMVirtualToMilli(PVM pVM, uint64_t u64VirtualTicks);
VMM_INT_DECL(uint64_t)  TMVirtualFromNano(PVM pVM, uint64_t u64NanoTS);
VMM_INT_DECL(uint64_t)  TMVirtualFromMicro(PVM pVM, uint64_t u64MicroTS);
VMM_INT_DECL(uint64_t)  TMVirtualFromMilli(PVM pVM, uint64_t u64MilliTS);
VMM_INT_DECL(bool)      TMVirtualIsTicking(PVM pVM);

VMMR3DECL(uint64_t)     TMR3TimeVirtGet(PUVM pUVM);
VMMR3DECL(uint64_t)     TMR3TimeVirtGetMilli(PUVM pUVM);
VMMR3DECL(uint64_t)     TMR3TimeVirtGetMicro(PUVM pUVM);
VMMR3DECL(uint64_t)     TMR3TimeVirtGetNano(PUVM pUVM);
/** @} */


/** @name CPU Clock Methods
 * @{
 */
VMMDECL(uint64_t)       TMCpuTickGet(PVMCPU pVCpu);
VMM_INT_DECL(uint64_t)  TMCpuTickGetNoCheck(PVMCPU pVCpu);
VMM_INT_DECL(bool)      TMCpuTickCanUseRealTSC(PVM pVM, PVMCPU pVCpu, uint64_t *poffRealTSC, bool *pfParavirtTsc);
VMM_INT_DECL(uint64_t)  TMCpuTickGetDeadlineAndTscOffset(PVM pVM, PVMCPU pVCpu, uint64_t *poffRealTSC, bool *pfOffsettedTsc, bool *pfParavirtTsc);
VMM_INT_DECL(int)       TMCpuTickSet(PVM pVM, PVMCPU pVCpu, uint64_t u64Tick);
VMM_INT_DECL(int)       TMCpuTickSetLastSeen(PVMCPU pVCpu, uint64_t u64LastSeenTick);
VMM_INT_DECL(uint64_t)  TMCpuTickGetLastSeen(PVMCPU pVCpu);
VMMDECL(uint64_t)       TMCpuTicksPerSecond(PVM pVM);
VMM_INT_DECL(bool)      TMCpuTickIsTicking(PVMCPU pVCpu);
/** @} */


/** @name Timer Methods
 * @{
 */
/**
 * Device timer callback function.
 *
 * @param   pDevIns         Device instance of the device which registered the timer.
 * @param   pTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACK(void) FNTMTIMERDEV(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser);
/** Pointer to a device timer callback function. */
typedef FNTMTIMERDEV *PFNTMTIMERDEV;

/**
 * USB device timer callback function.
 *
 * @param   pUsbIns         The USB device instance the timer is associated
 *                          with.
 * @param   pTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACK(void) FNTMTIMERUSB(PPDMUSBINS pUsbIns, PTMTIMER pTimer, void *pvUser);
/** Pointer to a timer callback for a USB device. */
typedef FNTMTIMERUSB *PFNTMTIMERUSB;

/**
 * Driver timer callback function.
 *
 * @param   pDrvIns         Device instance of the device which registered the timer.
 * @param   pTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACK(void) FNTMTIMERDRV(PPDMDRVINS pDrvIns, PTMTIMER pTimer, void *pvUser);
/** Pointer to a driver timer callback function. */
typedef FNTMTIMERDRV *PFNTMTIMERDRV;

/**
 * Service timer callback function.
 *
 * @param   pSrvIns         Service instance of the device which registered the timer.
 * @param   pTimer          The timer handle.
 */
typedef DECLCALLBACK(void) FNTMTIMERSRV(PPDMSRVINS pSrvIns, PTMTIMER pTimer);
/** Pointer to a service timer callback function. */
typedef FNTMTIMERSRV *PFNTMTIMERSRV;

/**
 * Internal timer callback function.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACK(void) FNTMTIMERINT(PVM pVM, PTMTIMER pTimer, void *pvUser);
/** Pointer to internal timer callback function. */
typedef FNTMTIMERINT *PFNTMTIMERINT;

/**
 * External timer callback function.
 *
 * @param   pvUser          User argument as specified when the timer was created.
 */
typedef DECLCALLBACK(void) FNTMTIMEREXT(void *pvUser);
/** Pointer to an external timer callback function. */
typedef FNTMTIMEREXT *PFNTMTIMEREXT;

VMMDECL(PTMTIMERR3)     TMTimerR3Ptr(PTMTIMER pTimer);
VMMDECL(PTMTIMERR0)     TMTimerR0Ptr(PTMTIMER pTimer);
VMMDECL(PTMTIMERRC)     TMTimerRCPtr(PTMTIMER pTimer);
VMMDECL(int)            TMTimerLock(PTMTIMER pTimer, int rcBusy);
VMMDECL(void)           TMTimerUnlock(PTMTIMER pTimer);
VMMDECL(bool)           TMTimerIsLockOwner(PTMTIMER pTimer);
VMMDECL(int)            TMTimerSet(PTMTIMER pTimer, uint64_t u64Expire);
VMMDECL(int)            TMTimerSetRelative(PTMTIMER pTimer, uint64_t cTicksToNext, uint64_t *pu64Now);
VMMDECL(int)            TMTimerSetFrequencyHint(PTMTIMER pTimer, uint32_t uHz);
VMMDECL(uint64_t)       TMTimerGet(PTMTIMER pTimer);
VMMDECL(int)            TMTimerStop(PTMTIMER pTimer);
VMMDECL(bool)           TMTimerIsActive(PTMTIMER pTimer);

VMMDECL(int)            TMTimerSetMillies(PTMTIMER pTimer, uint32_t cMilliesToNext);
VMMDECL(int)            TMTimerSetMicro(PTMTIMER pTimer, uint64_t cMicrosToNext);
VMMDECL(int)            TMTimerSetNano(PTMTIMER pTimer, uint64_t cNanosToNext);
VMMDECL(uint64_t)       TMTimerGetNano(PTMTIMER pTimer);
VMMDECL(uint64_t)       TMTimerGetMicro(PTMTIMER pTimer);
VMMDECL(uint64_t)       TMTimerGetMilli(PTMTIMER pTimer);
VMMDECL(uint64_t)       TMTimerGetFreq(PTMTIMER pTimer);
VMMDECL(uint64_t)       TMTimerGetExpire(PTMTIMER pTimer);
VMMDECL(uint64_t)       TMTimerToNano(PTMTIMER pTimer, uint64_t cTicks);
VMMDECL(uint64_t)       TMTimerToMicro(PTMTIMER pTimer, uint64_t cTicks);
VMMDECL(uint64_t)       TMTimerToMilli(PTMTIMER pTimer, uint64_t cTicks);
VMMDECL(uint64_t)       TMTimerFromNano(PTMTIMER pTimer, uint64_t cNanoSecs);
VMMDECL(uint64_t)       TMTimerFromMicro(PTMTIMER pTimer, uint64_t cMicroSecs);
VMMDECL(uint64_t)       TMTimerFromMilli(PTMTIMER pTimer, uint64_t cMilliSecs);

VMMDECL(bool)           TMTimerPollBool(PVM pVM, PVMCPU pVCpu);
VMM_INT_DECL(void)      TMTimerPollVoid(PVM pVM, PVMCPU pVCpu);
VMM_INT_DECL(uint64_t)  TMTimerPollGIP(PVM pVM, PVMCPU pVCpu, uint64_t *pu64Delta);
/** @} */


#ifdef IN_RING3
/** @defgroup grp_tm_r3     The TM Host Context Ring-3 API
 * @{
 */
VMM_INT_DECL(int)       TMR3Init(PVM pVM);
VMM_INT_DECL(int)       TMR3InitFinalize(PVM pVM);
VMM_INT_DECL(void)      TMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMM_INT_DECL(int)       TMR3Term(PVM pVM);
VMM_INT_DECL(void)      TMR3Reset(PVM pVM);
VMM_INT_DECL(int)       TMR3GetImportRC(PVM pVM, const char *pszSymbol, PRTRCPTR pRCPtrValue);
VMM_INT_DECL(int)       TMR3TimerCreateDevice(PVM pVM, PPDMDEVINS pDevIns, TMCLOCK enmClock, PFNTMTIMERDEV pfnCallback, void *pvUser, uint32_t fFlags, const char *pszDesc, PPTMTIMERR3 ppTimer);
VMM_INT_DECL(int)       TMR3TimerCreateUsb(PVM pVM, PPDMUSBINS pUsbIns, TMCLOCK enmClock, PFNTMTIMERUSB pfnCallback, void *pvUser, uint32_t fFlags, const char *pszDesc, PPTMTIMERR3 ppTimer);
VMM_INT_DECL(int)       TMR3TimerCreateDriver(PVM pVM, PPDMDRVINS pDrvIns, TMCLOCK enmClock, PFNTMTIMERDRV pfnCallback, void *pvUser, uint32_t fFlags, const char *pszDesc, PPTMTIMERR3 ppTimer);
VMMR3DECL(int)          TMR3TimerCreateInternal(PVM pVM, TMCLOCK enmClock, PFNTMTIMERINT pfnCallback, void *pvUser, const char *pszDesc, PPTMTIMERR3 ppTimer);
VMMR3DECL(PTMTIMERR3)   TMR3TimerCreateExternal(PVM pVM, TMCLOCK enmClock, PFNTMTIMEREXT pfnCallback, void *pvUser, const char *pszDesc);
VMMR3DECL(int)          TMR3TimerDestroy(PTMTIMER pTimer);
VMM_INT_DECL(int)       TMR3TimerDestroyDevice(PVM pVM, PPDMDEVINS pDevIns);
VMM_INT_DECL(int)       TMR3TimerDestroyUsb(PVM pVM, PPDMUSBINS pUsbIns);
VMM_INT_DECL(int)       TMR3TimerDestroyDriver(PVM pVM, PPDMDRVINS pDrvIns);
VMMR3DECL(int)          TMR3TimerSave(PTMTIMERR3 pTimer, PSSMHANDLE pSSM);
VMMR3DECL(int)          TMR3TimerLoad(PTMTIMERR3 pTimer, PSSMHANDLE pSSM);
VMMR3DECL(int)          TMR3TimerSkip(PSSMHANDLE pSSM, bool *pfActive);
VMMR3DECL(int)          TMR3TimerSetCritSect(PTMTIMERR3 pTimer, PPDMCRITSECT pCritSect);
VMMR3DECL(void)         TMR3TimerQueuesDo(PVM pVM);
VMMR3_INT_DECL(void)    TMR3VirtualSyncFF(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(PRTTIMESPEC) TMR3UtcNow(PVM pVM, PRTTIMESPEC pTime);

VMMR3_INT_DECL(int)     TMR3CpuTickParavirtEnable(PVM pVM);
VMMR3_INT_DECL(int)     TMR3CpuTickParavirtDisable(PVM pVM);
VMMR3_INT_DECL(bool)    TMR3CpuTickIsFixedRateMonotonic(PVM pVM, bool fWithParavirtEnabled);
/** @} */
#endif /* IN_RING3 */


/** @} */

RT_C_DECLS_END

#endif


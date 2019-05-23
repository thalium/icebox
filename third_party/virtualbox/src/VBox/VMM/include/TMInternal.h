/* $Id: TMInternal.h $ */
/** @file
 * TM - Internal header file.
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

#ifndef ___TMInternal_h
#define ___TMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <iprt/time.h>
#include <iprt/timer.h>
#include <iprt/assert.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/pdmcritsect.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_tm_int       Internal
 * @ingroup grp_tm
 * @internal
 * @{
 */

/** Frequency of the real clock. */
#define TMCLOCK_FREQ_REAL       UINT32_C(1000)
/** Frequency of the virtual clock. */
#define TMCLOCK_FREQ_VIRTUAL    UINT32_C(1000000000)


/**
 * Timer type.
 */
typedef enum TMTIMERTYPE
{
    /** Device timer. */
    TMTIMERTYPE_DEV = 1,
    /** USB device timer. */
    TMTIMERTYPE_USB,
    /** Driver timer. */
    TMTIMERTYPE_DRV,
    /** Internal timer . */
    TMTIMERTYPE_INTERNAL,
    /** External timer. */
    TMTIMERTYPE_EXTERNAL
} TMTIMERTYPE;

/**
 * Timer state
 */
typedef enum TMTIMERSTATE
{
    /** Timer is stopped. */
    TMTIMERSTATE_STOPPED = 1,
    /** Timer is active. */
    TMTIMERSTATE_ACTIVE,
    /** Timer is expired, getting expire and unlinking. */
    TMTIMERSTATE_EXPIRED_GET_UNLINK,
    /** Timer is expired and is being delivered. */
    TMTIMERSTATE_EXPIRED_DELIVER,

    /** Timer is stopped but still in the active list.
     * Currently in the ScheduleTimers list. */
    TMTIMERSTATE_PENDING_STOP,
    /** Timer is stopped but needs unlinking from the ScheduleTimers list.
     * Currently in the ScheduleTimers list. */
    TMTIMERSTATE_PENDING_STOP_SCHEDULE,
    /** Timer is being modified and will soon be pending scheduling.
     * Currently in the ScheduleTimers list. */
    TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE,
    /** Timer is pending scheduling.
     * Currently in the ScheduleTimers list. */
    TMTIMERSTATE_PENDING_SCHEDULE,
    /** Timer is being modified and will soon be pending rescheduling.
     * Currently in the ScheduleTimers list and the active list. */
    TMTIMERSTATE_PENDING_RESCHEDULE_SET_EXPIRE,
    /** Timer is modified and is now pending rescheduling.
     * Currently in the ScheduleTimers list and the active list. */
    TMTIMERSTATE_PENDING_RESCHEDULE,
    /** Timer is being destroyed. */
    TMTIMERSTATE_DESTROY,
    /** Timer is free. */
    TMTIMERSTATE_FREE
} TMTIMERSTATE;

/** Predicate that returns true if the give state is pending scheduling or
 *  rescheduling of any kind. Will reference the argument more than once! */
#define TMTIMERSTATE_IS_PENDING_SCHEDULING(enmState) \
    (   (enmState) <= TMTIMERSTATE_PENDING_RESCHEDULE \
     && (enmState) >= TMTIMERSTATE_PENDING_SCHEDULE_SET_EXPIRE)


/**
 * Internal representation of a timer.
 *
 * For correct serialization (without the use of semaphores and
 * other blocking/slow constructs) certain rules applies to updating
 * this structure:
 *      - For thread other than EMT only u64Expire, enmState and pScheduleNext*
 *        are changeable. Everything else is out of bounds.
 *      - Updating of u64Expire timer can only happen in the TMTIMERSTATE_STOPPED
 *        and TMTIMERSTATE_PENDING_RESCHEDULING_SET_EXPIRE states.
 *      - Timers in the TMTIMERSTATE_EXPIRED state are only accessible from EMT.
 *      - Actual destruction of a timer can only be done at scheduling time.
 */
typedef struct TMTIMER
{
    /** Expire time. */
    volatile uint64_t       u64Expire;
    /** Clock to apply to u64Expire. */
    TMCLOCK                 enmClock;
    /** Timer callback type. */
    TMTIMERTYPE             enmType;
    /** Type specific data. */
    union
    {
        /** TMTIMERTYPE_DEV. */
        struct
        {
            /** Callback. */
            R3PTRTYPE(PFNTMTIMERDEV)    pfnTimer;
            /** Device instance. */
            PPDMDEVINSR3                pDevIns;
        } Dev;

        /** TMTIMERTYPE_DEV. */
        struct
        {
            /** Callback. */
            R3PTRTYPE(PFNTMTIMERUSB)    pfnTimer;
            /** USB device instance. */
            PPDMUSBINS                  pUsbIns;
        } Usb;

        /** TMTIMERTYPE_DRV. */
        struct
        {
            /** Callback. */
            R3PTRTYPE(PFNTMTIMERDRV)    pfnTimer;
            /** Device instance. */
            R3PTRTYPE(PPDMDRVINS)       pDrvIns;
        } Drv;

        /** TMTIMERTYPE_INTERNAL. */
        struct
        {
            /** Callback. */
            R3PTRTYPE(PFNTMTIMERINT)    pfnTimer;
        } Internal;

        /** TMTIMERTYPE_EXTERNAL. */
        struct
        {
            /** Callback. */
            R3PTRTYPE(PFNTMTIMEREXT)    pfnTimer;
        } External;
    } u;

    /** Timer state. */
    volatile TMTIMERSTATE   enmState;
    /** Timer relative offset to the next timer in the schedule list. */
    int32_t volatile        offScheduleNext;

    /** Timer relative offset to the next timer in the chain. */
    int32_t                 offNext;
    /** Timer relative offset to the previous timer in the chain. */
    int32_t                 offPrev;

    /** Pointer to the VM the timer belongs to - R3 Ptr. */
    PVMR3                   pVMR3;
    /** Pointer to the VM the timer belongs to - R0 Ptr. */
    PVMR0                   pVMR0;
    /** Pointer to the VM the timer belongs to - RC Ptr. */
    PVMRC                   pVMRC;
    /** The timer frequency hint.  This is 0 if not hint was given. */
    uint32_t volatile       uHzHint;

    /** User argument. */
    RTR3PTR                 pvUser;
    /** The critical section associated with the lock. */
    R3PTRTYPE(PPDMCRITSECT) pCritSect;

    /** Pointer to the next timer in the list of created or free timers. (TM::pTimers or TM::pFree) */
    PTMTIMERR3              pBigNext;
    /** Pointer to the previous timer in the list of all created timers. (TM::pTimers) */
    PTMTIMERR3              pBigPrev;
    /** Pointer to the timer description. */
    R3PTRTYPE(const char *) pszDesc;
#if HC_ARCH_BITS == 32
    uint32_t                padding0; /**< pad structure to multiple of 8 bytes. */
#endif
} TMTIMER;
AssertCompileMemberSize(TMTIMER, enmState, sizeof(uint32_t));


/**
 * Updates a timer state in the correct atomic manner.
 */
#if 1
# define TM_SET_STATE(pTimer, state) \
    ASMAtomicWriteU32((uint32_t volatile *)&(pTimer)->enmState, state)
#else
# define TM_SET_STATE(pTimer, state) \
    do { \
        uint32_t uOld1 = (pTimer)->enmState; \
        Log(("%s: %p: %d -> %d\n", __FUNCTION__, (pTimer), (pTimer)->enmState, state)); \
        uint32_t uOld2 = ASMAtomicXchgU32((uint32_t volatile *)&(pTimer)->enmState, state); \
        Assert(uOld1 == uOld2); \
    } while (0)
#endif

/**
 * Tries to updates a timer state in the correct atomic manner.
 */
#if 1
# define TM_TRY_SET_STATE(pTimer, StateNew, StateOld, fRc) \
    (fRc) = ASMAtomicCmpXchgU32((uint32_t volatile *)&(pTimer)->enmState, StateNew, StateOld)
#else
# define TM_TRY_SET_STATE(pTimer, StateNew, StateOld, fRc) \
    do { (fRc) = ASMAtomicCmpXchgU32((uint32_t volatile *)&(pTimer)->enmState, StateNew, StateOld); \
         Log(("%s: %p: %d -> %d %RTbool\n", __FUNCTION__, (pTimer), StateOld, StateNew, fRc)); \
    } while (0)
#endif

/** Get the previous timer. */
#define TMTIMER_GET_PREV(pTimer) ((PTMTIMER)((pTimer)->offPrev ? (intptr_t)(pTimer) + (pTimer)->offPrev : 0))
/** Get the next timer. */
#define TMTIMER_GET_NEXT(pTimer) ((PTMTIMER)((pTimer)->offNext ? (intptr_t)(pTimer) + (pTimer)->offNext : 0))
/** Set the previous timer link. */
#define TMTIMER_SET_PREV(pTimer, pPrev) ((pTimer)->offPrev = (pPrev) ? (intptr_t)(pPrev) - (intptr_t)(pTimer) : 0)
/** Set the next timer link. */
#define TMTIMER_SET_NEXT(pTimer, pNext) ((pTimer)->offNext = (pNext) ? (intptr_t)(pNext) - (intptr_t)(pTimer) : 0)


/**
 * A timer queue.
 *
 * This is allocated on the hyper heap.
 */
typedef struct TMTIMERQUEUE
{
    /** The cached expire time for this queue.
     * Updated by EMT when scheduling the queue or modifying the head timer.
     * Assigned UINT64_MAX when there is no head timer. */
    uint64_t                u64Expire;
    /** Doubly linked list of active timers.
     *
     * When no scheduling is pending, this list is will be ordered by expire time (ascending).
     * Access is serialized by only letting the emulation thread (EMT) do changes.
     *
     * The offset is relative to the queue structure.
     */
    int32_t                 offActive;
    /** List of timers pending scheduling of some kind.
     *
     * Timer stats allowed in the list are TMTIMERSTATE_PENDING_STOPPING,
     * TMTIMERSTATE_PENDING_DESTRUCTION, TMTIMERSTATE_PENDING_STOPPING_DESTRUCTION,
     * TMTIMERSTATE_PENDING_RESCHEDULING and TMTIMERSTATE_PENDING_SCHEDULE.
     *
     * The offset is relative to the queue structure.
     */
    int32_t volatile        offSchedule;
    /** The clock for this queue. */
    TMCLOCK                 enmClock;
    /** Pad the structure up to 32 bytes. */
    uint32_t                au32Padding[3];
} TMTIMERQUEUE;

/** Pointer to a timer queue. */
typedef TMTIMERQUEUE *PTMTIMERQUEUE;

/** Get the head of the active timer list. */
#define TMTIMER_GET_HEAD(pQueue)        ((PTMTIMER)((pQueue)->offActive ? (intptr_t)(pQueue) + (pQueue)->offActive : 0))
/** Set the head of the active timer list. */
#define TMTIMER_SET_HEAD(pQueue, pHead) ((pQueue)->offActive = pHead ? (intptr_t)pHead - (intptr_t)(pQueue) : 0)


/**
 * CPU load data set.
 * Mainly used by tmR3CpuLoadTimer.
 */
typedef struct TMCPULOADSTATE
{
    /** The percent of the period spent executing guest code. */
    uint8_t                 cPctExecuting;
    /** The percent of the period spent halted. */
    uint8_t                 cPctHalted;
    /** The percent of the period spent on other things. */
    uint8_t                 cPctOther;
    /** Explicit alignment padding */
    uint8_t                 au8Alignment[5];

    /** Previous cNsTotal value. */
    uint64_t                cNsPrevTotal;
    /** Previous cNsExecuting value. */
    uint64_t                cNsPrevExecuting;
    /** Previous cNsHalted value. */
    uint64_t                cNsPrevHalted;
} TMCPULOADSTATE;
AssertCompileSizeAlignment(TMCPULOADSTATE, 8);
AssertCompileMemberAlignment(TMCPULOADSTATE, cNsPrevTotal, 8);
/** Pointer to a CPU load data set. */
typedef TMCPULOADSTATE *PTMCPULOADSTATE;


/**
 * TSC mode.
 *
 * The main modes of how TM implements the TSC clock (TMCLOCK_TSC).
 */
typedef enum TMTSCMODE
{
    /** The guest TSC is an emulated, virtual TSC. */
    TMTSCMODE_VIRT_TSC_EMULATED = 1,
    /** The guest TSC is an offset of the real TSC. */
    TMTSCMODE_REAL_TSC_OFFSET,
    /** The guest TSC is dynamically derived through emulating or offsetting. */
    TMTSCMODE_DYNAMIC
} TMTSCMODE;
AssertCompileSize(TMTSCMODE, sizeof(uint32_t));


/**
 * Converts a TM pointer into a VM pointer.
 * @returns Pointer to the VM structure the TM is part of.
 * @param   pTM   Pointer to TM instance data.
 */
#define TM2VM(pTM)  ( (PVM)((char*)pTM - pTM->offVM) )


/**
 * TM VM Instance data.
 * Changes to this must checked against the padding of the cfgm union in VM!
 */
typedef struct TM
{
    /** Offset to the VM structure.
     * See TM2VM(). */
    RTUINT                      offVM;

    /** The current TSC mode of the VM.
     *  Config variable: Mode (string). */
    TMTSCMODE                   enmTSCMode;
    /** The original TSC mode of the VM. */
    TMTSCMODE                   enmOriginalTSCMode;
    /** Alignment padding. */
    uint32_t                    u32Alignment0;
    /** Whether the TSC is tied to the execution of code.
     * Config variable: TSCTiedToExecution (bool) */
    bool                        fTSCTiedToExecution;
    /** Modifier for fTSCTiedToExecution which pauses the TSC while halting if true.
     * Config variable: TSCNotTiedToHalt (bool) */
    bool                        fTSCNotTiedToHalt;
    /** Whether TM TSC mode switching is allowed at runtime. */
    bool                        fTSCModeSwitchAllowed;
    /** Whether the guest has enabled use of paravirtualized TSC. */
    bool                        fParavirtTscEnabled;
    /** The ID of the virtual CPU that normally runs the timers. */
    VMCPUID                     idTimerCpu;

    /** The number of CPU clock ticks per second (TMCLOCK_TSC).
     * Config variable: TSCTicksPerSecond (64-bit unsigned int)
     * The config variable implies @c enmTSCMode would be
     * TMTSCMODE_VIRT_TSC_EMULATED. */
    uint64_t                    cTSCTicksPerSecond;
    /** The TSC difference introduced by pausing the VM. */
    uint64_t                    offTSCPause;
    /** The TSC value when the last TSC was paused. */
    uint64_t                    u64LastPausedTSC;
    /** CPU TSCs ticking indicator (one for each VCPU). */
    uint32_t volatile           cTSCsTicking;

    /** Virtual time ticking enabled indicator (counter for each VCPU). (TMCLOCK_VIRTUAL) */
    uint32_t volatile           cVirtualTicking;
    /** Virtual time is not running at 100%. */
    bool                        fVirtualWarpDrive;
    /** Virtual timer synchronous time ticking enabled indicator (bool). (TMCLOCK_VIRTUAL_SYNC) */
    bool volatile               fVirtualSyncTicking;
    /** Virtual timer synchronous time catch-up active. */
    bool volatile               fVirtualSyncCatchUp;
    /** Alignment padding. */
    bool                        afAlignment1[1];
    /** WarpDrive percentage.
     * 100% is normal (fVirtualSyncNormal == true). When other than 100% we apply
     * this percentage to the raw time source for the period it's been valid in,
     * i.e. since u64VirtualWarpDriveStart. */
    uint32_t                    u32VirtualWarpDrivePercentage;

    /** The offset of the virtual clock relative to it's timesource.
     * Only valid if fVirtualTicking is set. */
    uint64_t                    u64VirtualOffset;
    /** The guest virtual time when fVirtualTicking is cleared. */
    uint64_t                    u64Virtual;
    /** When the Warp drive was started or last adjusted.
     * Only valid when fVirtualWarpDrive is set. */
    uint64_t                    u64VirtualWarpDriveStart;
    /** The previously returned nano TS.
     * This handles TSC drift on SMP systems and expired interval.
     * This is a valid range u64NanoTS to u64NanoTS + 1000000000 (ie. 1sec). */
    uint64_t volatile           u64VirtualRawPrev;
    /** The ring-3 data structure for the RTTimeNanoTS workers used by tmVirtualGetRawNanoTS. */
    RTTIMENANOTSDATAR3          VirtualGetRawDataR3;
    /** The ring-0 data structure for the RTTimeNanoTS workers used by tmVirtualGetRawNanoTS. */
    RTTIMENANOTSDATAR0          VirtualGetRawDataR0;
    /** The ring-0 data structure for the RTTimeNanoTS workers used by tmVirtualGetRawNanoTS. */
    RTTIMENANOTSDATARC          VirtualGetRawDataRC;
    /** Pointer to the ring-3 tmVirtualGetRawNanoTS worker function. */
    R3PTRTYPE(PFNTIMENANOTSINTERNAL) pfnVirtualGetRawR3;
    /** Pointer to the ring-0 tmVirtualGetRawNanoTS worker function. */
    R0PTRTYPE(PFNTIMENANOTSINTERNAL) pfnVirtualGetRawR0;
    /** Pointer to the raw-mode tmVirtualGetRawNanoTS worker function. */
    RCPTRTYPE(PFNTIMENANOTSINTERNAL) pfnVirtualGetRawRC;
    /** Alignment. */
    RTRCPTR                     AlignmentRCPtr;
    /** The guest virtual timer synchronous time when fVirtualSyncTicking is cleared.
     * When fVirtualSyncTicking is set it holds the last time returned to
     * the guest (while the lock was held). */
    uint64_t volatile           u64VirtualSync;
    /** The offset of the timer synchronous virtual clock (TMCLOCK_VIRTUAL_SYNC) relative
     * to the virtual clock (TMCLOCK_VIRTUAL).
     * (This is accessed by the timer thread and must be updated atomically.) */
    uint64_t volatile           offVirtualSync;
    /** The offset into offVirtualSync that's been irrevocably given up by failed catch-up attempts.
     * Thus the current lag is offVirtualSync - offVirtualSyncGivenUp. */
    uint64_t                    offVirtualSyncGivenUp;
    /** The TMCLOCK_VIRTUAL at the previous TMVirtualGetSync call when catch-up is active. */
    uint64_t volatile           u64VirtualSyncCatchUpPrev;
    /** The current catch-up percentage. */
    uint32_t volatile           u32VirtualSyncCatchUpPercentage;
    /** How much slack when processing timers. */
    uint32_t                    u32VirtualSyncScheduleSlack;
    /** When to stop catch-up. */
    uint64_t                    u64VirtualSyncCatchUpStopThreshold;
    /** When to give up catch-up. */
    uint64_t                    u64VirtualSyncCatchUpGiveUpThreshold;
/** @def TM_MAX_CATCHUP_PERIODS
 * The number of catchup rates. */
#define TM_MAX_CATCHUP_PERIODS  10
    /** The aggressiveness of the catch-up relative to how far we've lagged behind.
     * The idea is to have increasing catch-up percentage as the lag increases. */
    struct TMCATCHUPPERIOD
    {
        uint64_t                u64Start;       /**< When this period starts. (u64VirtualSyncOffset). */
        uint32_t                u32Percentage;  /**< The catch-up percent to apply. */
        uint32_t                u32Alignment;   /**< Structure alignment */
    }                           aVirtualSyncCatchUpPeriods[TM_MAX_CATCHUP_PERIODS];

    /** The current max timer Hz hint. */
    uint32_t volatile           uMaxHzHint;
    /** Whether to recalulate the HzHint next time its queried. */
    bool volatile               fHzHintNeedsUpdating;
    /** Alignment */
    bool                        afAlignment2[3];
    /** @cfgm{/TM/HostHzMax, uint32_t, Hz, 0, UINT32_MAX, 20000}
     * The max host Hz frequency hint returned by TMCalcHostTimerFrequency.  */
    uint32_t                    cHostHzMax;
    /** @cfgm{/TM/HostHzFudgeFactorTimerCpu, uint32_t, Hz, 0, UINT32_MAX, 111}
     * The number of Hz TMCalcHostTimerFrequency adds for the timer CPU.  */
    uint32_t                    cPctHostHzFudgeFactorTimerCpu;
    /** @cfgm{/TM/HostHzFudgeFactorOtherCpu, uint32_t, Hz, 0, UINT32_MAX, 110}
     * The number of Hz TMCalcHostTimerFrequency adds for the other CPUs. */
    uint32_t                    cPctHostHzFudgeFactorOtherCpu;
    /** @cfgm{/TM/HostHzFudgeFactorCatchUp100, uint32_t, Hz, 0, UINT32_MAX, 300}
     *  The fudge factor (expressed in percent) that catch-up percentages below
     * 100% is multiplied by. */
    uint32_t                    cPctHostHzFudgeFactorCatchUp100;
    /** @cfgm{/TM/HostHzFudgeFactorCatchUp200, uint32_t, Hz, 0, UINT32_MAX, 250}
     * The fudge factor (expressed in percent) that catch-up percentages
     * 100%-199% is multiplied by. */
    uint32_t                    cPctHostHzFudgeFactorCatchUp200;
    /** @cfgm{/TM/HostHzFudgeFactorCatchUp400, uint32_t, Hz, 0, UINT32_MAX, 200}
     * The fudge factor (expressed in percent) that catch-up percentages
     * 200%-399% is multiplied by. */
    uint32_t                    cPctHostHzFudgeFactorCatchUp400;

    /** The UTC offset in ns.
     * This is *NOT* for converting UTC to local time. It is for converting real
     * world UTC time to VM UTC time. This feature is indented for doing date
     * testing of software and similar.
     * @todo Implement warpdrive on UTC. */
    int64_t                     offUTC;
    /** The last value TMR3UtcNow returned. */
    int64_t volatile            nsLastUtcNow;
    /** File to touch on UTC jump. */
    R3PTRTYPE(char *)           pszUtcTouchFileOnJump;
    /** Just to avoid dealing with 32-bit alignment trouble. */
    R3PTRTYPE(char *)           pszAlignment2b;

    /** Timer queues for the different clock types - R3 Ptr */
    R3PTRTYPE(PTMTIMERQUEUE)    paTimerQueuesR3;
    /** Timer queues for the different clock types - R0 Ptr */
    R0PTRTYPE(PTMTIMERQUEUE)    paTimerQueuesR0;
    /** Timer queues for the different clock types - RC Ptr */
    RCPTRTYPE(PTMTIMERQUEUE)    paTimerQueuesRC;

    /** Pointer to our RC mapping of the GIP. */
    RCPTRTYPE(void *)           pvGIPRC;
    /** Pointer to our R3 mapping of the GIP. */
    R3PTRTYPE(void *)           pvGIPR3;

    /** Pointer to a singly linked list of free timers.
     * This chain is using the TMTIMER::pBigNext members.
     * Only accessible from the emulation thread. */
    PTMTIMERR3                  pFree;

    /** Pointer to a doubly linked list of created timers.
     * This chain is using the TMTIMER::pBigNext and TMTIMER::pBigPrev members.
     * Only accessible from the emulation thread. */
    PTMTIMERR3                  pCreated;

    /** The schedule timer timer handle (runtime timer).
     * This timer will do frequent check on pending queue schedules and
     * raise VM_FF_TIMER to pull EMTs attention to them.
     */
    R3PTRTYPE(PRTTIMER)         pTimer;
    /** Interval in milliseconds of the pTimer timer. */
    uint32_t                    u32TimerMillies;

    /** Indicates that queues are being run. */
    bool volatile               fRunningQueues;
    /** Indicates that the virtual sync queue is being run. */
    bool volatile               fRunningVirtualSyncQueue;
    /** Alignment */
    bool                        afAlignment3[2];

    /** Lock serializing access to the timer lists. */
    PDMCRITSECT                 TimerCritSect;
    /** Lock serializing access to the VirtualSync clock and the associated
     * timer queue. */
    PDMCRITSECT                 VirtualSyncLock;

    /** CPU load state for all the virtual CPUs (tmR3CpuLoadTimer). */
    TMCPULOADSTATE              CpuLoad;

    /** TMR3TimerQueuesDo
     * @{ */
    STAMPROFILE                 StatDoQueues;
    STAMPROFILEADV              aStatDoQueues[TMCLOCK_MAX];
    /** @} */
    /** tmSchedule
     * @{ */
    STAMPROFILE                 StatScheduleOneRZ;
    STAMPROFILE                 StatScheduleOneR3;
    STAMCOUNTER                 StatScheduleSetFF;
    STAMCOUNTER                 StatPostponedR3;
    STAMCOUNTER                 StatPostponedRZ;
    /** @} */
    /** Read the time
     * @{ */
    STAMCOUNTER                 StatVirtualGet;
    STAMCOUNTER                 StatVirtualGetSetFF;
    STAMCOUNTER                 StatVirtualSyncGet;
    STAMCOUNTER                 StatVirtualSyncGetAdjLast;
    STAMCOUNTER                 StatVirtualSyncGetELoop;
    STAMCOUNTER                 StatVirtualSyncGetExpired;
    STAMCOUNTER                 StatVirtualSyncGetLockless;
    STAMCOUNTER                 StatVirtualSyncGetLocked;
    STAMCOUNTER                 StatVirtualSyncGetSetFF;
    STAMCOUNTER                 StatVirtualPause;
    STAMCOUNTER                 StatVirtualResume;
    /** @} */
    /** TMTimerPoll
     * @{ */
    STAMCOUNTER                 StatPoll;
    STAMCOUNTER                 StatPollAlreadySet;
    STAMCOUNTER                 StatPollELoop;
    STAMCOUNTER                 StatPollMiss;
    STAMCOUNTER                 StatPollRunning;
    STAMCOUNTER                 StatPollSimple;
    STAMCOUNTER                 StatPollVirtual;
    STAMCOUNTER                 StatPollVirtualSync;
    /** @} */
    /** TMTimerSet sans virtual sync timers.
     * @{ */
    STAMCOUNTER                 StatTimerSet;
    STAMCOUNTER                 StatTimerSetOpt;
    STAMPROFILE                 StatTimerSetRZ;
    STAMPROFILE                 StatTimerSetR3;
    STAMCOUNTER                 StatTimerSetStStopped;
    STAMCOUNTER                 StatTimerSetStExpDeliver;
    STAMCOUNTER                 StatTimerSetStActive;
    STAMCOUNTER                 StatTimerSetStPendStop;
    STAMCOUNTER                 StatTimerSetStPendStopSched;
    STAMCOUNTER                 StatTimerSetStPendSched;
    STAMCOUNTER                 StatTimerSetStPendResched;
    STAMCOUNTER                 StatTimerSetStOther;
    /** @}  */
    /** TMTimerSet on virtual sync timers.
     * @{ */
    STAMCOUNTER                 StatTimerSetVs;
    STAMPROFILE                 StatTimerSetVsRZ;
    STAMPROFILE                 StatTimerSetVsR3;
    STAMCOUNTER                 StatTimerSetVsStStopped;
    STAMCOUNTER                 StatTimerSetVsStExpDeliver;
    STAMCOUNTER                 StatTimerSetVsStActive;
    /** @} */
    /** TMTimerSetRelative sans virtual sync timers
     * @{ */
    STAMCOUNTER                 StatTimerSetRelative;
    STAMPROFILE                 StatTimerSetRelativeRZ;
    STAMPROFILE                 StatTimerSetRelativeR3;
    STAMCOUNTER                 StatTimerSetRelativeOpt;
    STAMCOUNTER                 StatTimerSetRelativeStStopped;
    STAMCOUNTER                 StatTimerSetRelativeStExpDeliver;
    STAMCOUNTER                 StatTimerSetRelativeStActive;
    STAMCOUNTER                 StatTimerSetRelativeStPendStop;
    STAMCOUNTER                 StatTimerSetRelativeStPendStopSched;
    STAMCOUNTER                 StatTimerSetRelativeStPendSched;
    STAMCOUNTER                 StatTimerSetRelativeStPendResched;
    STAMCOUNTER                 StatTimerSetRelativeStOther;
    /** @} */
    /** TMTimerSetRelative on virtual sync timers.
     * @{ */
    STAMCOUNTER                 StatTimerSetRelativeVs;
    STAMPROFILE                 StatTimerSetRelativeVsRZ;
    STAMPROFILE                 StatTimerSetRelativeVsR3;
    STAMCOUNTER                 StatTimerSetRelativeVsStStopped;
    STAMCOUNTER                 StatTimerSetRelativeVsStExpDeliver;
    STAMCOUNTER                 StatTimerSetRelativeVsStActive;
    /** @} */
    /** TMTimerStop sans virtual sync.
     * @{ */
    STAMPROFILE                 StatTimerStopRZ;
    STAMPROFILE                 StatTimerStopR3;
    /** @} */
    /** TMTimerStop on virtual sync timers.
     * @{ */
    STAMPROFILE                 StatTimerStopVsRZ;
    STAMPROFILE                 StatTimerStopVsR3;
    /** @} */
    /** VirtualSync - Running and Catching Up
     * @{ */
    STAMCOUNTER                 StatVirtualSyncRun;
    STAMCOUNTER                 StatVirtualSyncRunRestart;
    STAMPROFILE                 StatVirtualSyncRunSlack;
    STAMCOUNTER                 StatVirtualSyncRunStop;
    STAMCOUNTER                 StatVirtualSyncRunStoppedAlready;
    STAMCOUNTER                 StatVirtualSyncGiveUp;
    STAMCOUNTER                 StatVirtualSyncGiveUpBeforeStarting;
    STAMPROFILEADV              StatVirtualSyncCatchup;
    STAMCOUNTER                 aStatVirtualSyncCatchupInitial[TM_MAX_CATCHUP_PERIODS];
    STAMCOUNTER                 aStatVirtualSyncCatchupAdjust[TM_MAX_CATCHUP_PERIODS];
    /** @} */
    /** TMR3VirtualSyncFF (non dedicated EMT). */
    STAMPROFILE                 StatVirtualSyncFF;
    /** The timer callback. */
    STAMCOUNTER                 StatTimerCallbackSetFF;
    STAMCOUNTER                 StatTimerCallback;

    /** Calls to TMCpuTickSet. */
    STAMCOUNTER                 StatTSCSet;

    /** TSC starts and stops. */
    STAMCOUNTER                 StatTSCPause;
    STAMCOUNTER                 StatTSCResume;

    /** @name Reasons for refusing TSC offsetting in TMCpuTickCanUseRealTSC.
     * @{ */
    STAMCOUNTER                 StatTSCNotFixed;
    STAMCOUNTER                 StatTSCNotTicking;
    STAMCOUNTER                 StatTSCCatchupLE010;
    STAMCOUNTER                 StatTSCCatchupLE025;
    STAMCOUNTER                 StatTSCCatchupLE100;
    STAMCOUNTER                 StatTSCCatchupOther;
    STAMCOUNTER                 StatTSCWarp;
    STAMCOUNTER                 StatTSCUnderflow;
    STAMCOUNTER                 StatTSCSyncNotTicking;
    /** @} */
} TM;
/** Pointer to TM VM instance data. */
typedef TM *PTM;

/**
 * TM VMCPU Instance data.
 * Changes to this must checked against the padding of the tm union in VM!
 */
typedef struct TMCPU
{
    /** Offset to the VMCPU structure.
     * See TMCPU2VM(). */
    RTUINT                      offVMCPU;

    /** CPU timestamp ticking enabled indicator (bool). (RDTSC) */
    bool                        fTSCTicking;
    bool                        afAlignment0[3]; /**< alignment padding */

    /** The offset between the host tick (TSC/virtual depending on the TSC mode) and
     *  the guest tick. */
    uint64_t                    offTSCRawSrc;

    /** The guest TSC when fTicking is cleared. */
    uint64_t                    u64TSC;

    /** The last seen TSC by the guest. */
    uint64_t                    u64TSCLastSeen;

#ifndef VBOX_WITHOUT_NS_ACCOUNTING
    /** The nanosecond timestamp of the CPU start or resume.
     * This is recalculated when the VM is started so that
     * cNsTotal = RTTimeNanoTS() - u64NsTsStartCpu. */
    uint64_t                    u64NsTsStartTotal;
    /** The nanosecond timestamp of the last start-execute notification. */
    uint64_t                    u64NsTsStartExecuting;
    /** The nanosecond timestamp of the last start-halt notification. */
    uint64_t                    u64NsTsStartHalting;
    /** The cNsXXX generation. */
    uint32_t volatile           uTimesGen;
    /** Explicit alignment padding.  */
    uint32_t                    u32Alignment;
    /** The number of nanoseconds total run time.
     * @remarks This is updated when cNsExecuting and cNsHalted are updated. */
    uint64_t                    cNsTotal;
    /** The number of nanoseconds spent executing. */
    uint64_t                    cNsExecuting;
    /** The number of nanoseconds being halted. */
    uint64_t                    cNsHalted;
    /** The number of nanoseconds spent on other things.
     * @remarks This is updated when cNsExecuting and cNsHalted are updated. */
    uint64_t                    cNsOther;
    /** The number of halts. */
    uint64_t                    cPeriodsHalted;
    /** The number of guest execution runs. */
    uint64_t                    cPeriodsExecuting;
# if defined(VBOX_WITH_STATISTICS) || defined(VBOX_WITH_NS_ACCOUNTING_STATS)
    /** Resettable version of cNsTotal. */
    STAMCOUNTER                 StatNsTotal;
    /** Resettable version of cNsExecuting. */
    STAMPROFILE                 StatNsExecuting;
    /** Long execution intervals. */
    STAMPROFILE                 StatNsExecLong;
    /** Short execution intervals . */
    STAMPROFILE                 StatNsExecShort;
    /** Tiny execution intervals . */
    STAMPROFILE                 StatNsExecTiny;
    /** Resettable version of cNsHalted. */
    STAMPROFILE                 StatNsHalted;
    /** Resettable version of cNsOther. */
    STAMPROFILE                 StatNsOther;
# endif

    /** CPU load state for this virtual CPU (tmR3CpuLoadTimer). */
    TMCPULOADSTATE              CpuLoad;
#endif
} TMCPU;
/** Pointer to TM VMCPU instance data. */
typedef TMCPU *PTMCPU;

const char             *tmTimerState(TMTIMERSTATE enmState);
void                    tmTimerQueueSchedule(PVM pVM, PTMTIMERQUEUE pQueue);
#ifdef VBOX_STRICT
void                    tmTimerQueuesSanityChecks(PVM pVM, const char *pszWhere);
#endif

uint64_t                tmR3CpuTickGetRawVirtualNoCheck(PVM pVM);
int                     tmCpuTickPause(PVMCPU pVCpu);
int                     tmCpuTickPauseLocked(PVM pVM, PVMCPU pVCpu);
int                     tmCpuTickResume(PVM pVM, PVMCPU pVCpu);
int                     tmCpuTickResumeLocked(PVM pVM, PVMCPU pVCpu);

int                     tmVirtualPauseLocked(PVM pVM);
int                     tmVirtualResumeLocked(PVM pVM);
DECLCALLBACK(DECLEXPORT(void))      tmVirtualNanoTSBad(PRTTIMENANOTSDATA pData, uint64_t u64NanoTS,
                                                       uint64_t u64DeltaPrev, uint64_t u64PrevNanoTS);
DECLCALLBACK(DECLEXPORT(uint64_t))  tmVirtualNanoTSRediscover(PRTTIMENANOTSDATA pData);
DECLCALLBACK(DECLEXPORT(uint64_t))  tmVirtualNanoTSBadCpuIndex(PRTTIMENANOTSDATA pData, uint16_t idApic,
                                                               uint16_t iCpuSet, uint16_t iGipCpu);

/**
 * Try take the timer lock, wait in ring-3 return VERR_SEM_BUSY in R0/RC.
 *
 * @retval  VINF_SUCCESS on success (always in ring-3).
 * @retval  VERR_SEM_BUSY in RC and R0 if the semaphore is busy.
 *
 * @param   a_pVM       Pointer to the VM.
 *
 * @remarks The virtual sync timer queue requires the virtual sync lock.
 */
#define TM_LOCK_TIMERS(a_pVM)       PDMCritSectEnter(&(a_pVM)->tm.s.TimerCritSect, VERR_SEM_BUSY)

/**
 * Try take the timer lock, no waiting.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_SEM_BUSY if busy.
 *
 * @param   a_pVM       Pointer to the VM.
 *
 * @remarks The virtual sync timer queue requires the virtual sync lock.
 */
#define TM_TRY_LOCK_TIMERS(a_pVM)   PDMCritSectTryEnter(&(a_pVM)->tm.s.TimerCritSect)

/** Lock the timers (sans the virtual sync queue). */
#define TM_UNLOCK_TIMERS(a_pVM)     do { PDMCritSectLeave(&(a_pVM)->tm.s.TimerCritSect); } while (0)

/** Checks that the caller owns the timer lock.  */
#define TM_ASSERT_TIMER_LOCK_OWNERSHIP(a_pVM) \
    Assert(PDMCritSectIsOwner(&(a_pVM)->tm.s.TimerCritSect))

/** @} */

RT_C_DECLS_END

#endif


/* $Id: gvmm.h $ */
/** @file
 * GVMM - The Global VM Manager.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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

#ifndef ___VBox_vmm_gvmm_h
#define ___VBox_vmm_gvmm_h

#include <VBox/types.h>
#include <VBox/sup.h>
#include <iprt/cpuset.h> /* RTCPUSET_MAX_CPUS */


RT_C_DECLS_BEGIN

/** @defgroup grp_gvmm  GVMM - The Global VM Manager.
 * @ingroup grp_vmm
 * @{
 */

/** @def IN_GVMM_R0
 * Used to indicate whether we're inside the same link module as the ring 0
 * part of the Global VM Manager or not.
 */
#ifdef DOXYGEN_RUNNING
# define IN_GVMM_R0
#endif
/** @def GVMMR0DECL
 * Ring 0 VM export or import declaration.
 * @param   type    The return type of the function declaration.
 */
#ifdef IN_GVMM_R0
# define GVMMR0DECL(type)    DECLEXPORT(type) VBOXCALL
#else
# define GVMMR0DECL(type)    DECLIMPORT(type) VBOXCALL
#endif

/** @def NIL_GVM_HANDLE
 * The nil GVM VM handle value (VM::hSelf).
 */
#define NIL_GVM_HANDLE 0


/**
 * The scheduler statistics
 */
typedef struct GVMMSTATSSCHED
{
    /** The number of calls to GVMMR0SchedHalt. */
    uint64_t        cHaltCalls;
    /** The number of times we did go to sleep in GVMMR0SchedHalt. */
    uint64_t        cHaltBlocking;
    /** The number of times we timed out in GVMMR0SchedHalt. */
    uint64_t        cHaltTimeouts;
    /** The number of times we didn't go to sleep in GVMMR0SchedHalt. */
    uint64_t        cHaltNotBlocking;
    /** The number of wake ups done during GVMMR0SchedHalt. */
    uint64_t        cHaltWakeUps;

    /** The number of calls to GVMMR0WakeUp. */
    uint64_t        cWakeUpCalls;
    /** The number of times the EMT thread wasn't actually halted when GVMMR0WakeUp
     *  was called. */
    uint64_t        cWakeUpNotHalted;
    /** The number of wake ups done during GVMMR0WakeUp (not counting the explicit
     *  one). */
    uint64_t        cWakeUpWakeUps;

    /** The number of calls to GVMMR0Poke. */
    uint64_t        cPokeCalls;
    /** The number of times the EMT thread wasn't actually busy when
     *  GVMMR0Poke was called. */
    uint64_t        cPokeNotBusy;

    /** The number of calls to GVMMR0SchedPoll. */
    uint64_t        cPollCalls;
    /** The number of times the EMT has halted in a GVMMR0SchedPoll call. */
    uint64_t        cPollHalts;
    /** The number of wake ups done during GVMMR0SchedPoll. */
    uint64_t        cPollWakeUps;

    uint64_t        u64Alignment; /**< padding */
} GVMMSTATSSCHED;
/** Pointer to the GVMM scheduler statistics. */
typedef GVMMSTATSSCHED *PGVMMSTATSSCHED;

/**
 * Per host cpu statistics.
 */
typedef struct GVMMSTATSHOSTCPU
{
    /** The CPU ID. */
    RTCPUID         idCpu;
    /** The CPU's set index. */
    uint32_t        idxCpuSet;
    /** The desired PPT frequency. */
    uint32_t        uDesiredHz;
    /** The current PPT timer frequency.  */
    uint32_t        uTimerHz;
    /** The number of times the PPT was changed. */
    uint32_t        cChanges;
    /** The number of times the PPT was started. */
    uint32_t        cStarts;
} GVMMSTATSHOSTCPU;
/** Pointer to the GVMM per host CPU statistics. */
typedef GVMMSTATSHOSTCPU *PGVMMSTATSHOSTCPU;

/**
 * The GVMM statistics.
 */
typedef struct GVMMSTATS
{
    /** The VM statistics if a VM was specified. */
    GVMMSTATSSCHED      SchedVM;
    /** The sum statistics of all VMs accessible to the caller. */
    GVMMSTATSSCHED      SchedSum;
    /** The number of VMs accessible to the caller. */
    uint32_t            cVMs;
    /** The number of emulation threads in those VMs. */
    uint32_t            cEMTs;
    /** Padding.  */
    uint32_t            u32Padding;
    /** The number of valid entries in aHostCpus. */
    uint32_t            cHostCpus;
    /** Per host CPU statistics. */
    GVMMSTATSHOSTCPU    aHostCpus[RTCPUSET_MAX_CPUS];
} GVMMSTATS;
/** Pointer to the GVMM statistics. */
typedef GVMMSTATS *PGVMMSTATS;
/** Const pointer to the GVMM statistics. */
typedef const GVMMSTATS *PCGVMMSTATS;



GVMMR0DECL(int)     GVMMR0Init(void);
GVMMR0DECL(void)    GVMMR0Term(void);
GVMMR0DECL(int)     GVMMR0SetConfig(PSUPDRVSESSION pSession, const char *pszName, uint64_t u64Value);
GVMMR0DECL(int)     GVMMR0QueryConfig(PSUPDRVSESSION pSession, const char *pszName, uint64_t *pu64Value);

GVMMR0DECL(int)     GVMMR0CreateVM(PSUPDRVSESSION pSession, uint32_t cCpus, PVM *ppVM);
GVMMR0DECL(int)     GVMMR0InitVM(PGVM pGVM);
GVMMR0DECL(void)    GVMMR0DoneInitVM(PGVM pGVM);
GVMMR0DECL(bool)    GVMMR0DoingTermVM(PGVM pGVM);
GVMMR0DECL(int)     GVMMR0DestroyVM(PGVM pGVM, PVM pVM);
GVMMR0DECL(int)     GVMMR0RegisterVCpu(PGVM pGVM, PVM pVM, VMCPUID idCpu);
GVMMR0DECL(int)     GVMMR0DeregisterVCpu(PGVM pGVM, PVM pVM, VMCPUID idCpu);
GVMMR0DECL(PGVM)    GVMMR0ByHandle(uint32_t hGVM);
GVMMR0DECL(int)     GVMMR0ValidateGVMandVM(PGVM pGVM, PVM pVM);
GVMMR0DECL(int)     GVMMR0ValidateGVMandVMandEMT(PGVM pGVM, PVM pVM, VMCPUID idCpu);
GVMMR0DECL(PVM)     GVMMR0GetVMByEMT(RTNATIVETHREAD hEMT);
GVMMR0DECL(int)     GVMMR0SchedHalt(PGVM pGVM, PVM pVM, VMCPUID idCpu, uint64_t u64ExpireGipTime);
GVMMR0DECL(int)     GVMMR0SchedWakeUp(PGVM pGVM, PVM pVM, VMCPUID idCpu);
GVMMR0DECL(int)     GVMMR0SchedWakeUpEx(PGVM pGVM, PVM pVM, VMCPUID idCpu, bool fTakeUsedLock);
GVMMR0DECL(int)     GVMMR0SchedWakeUpNoGVMNoLock(PVM pVM, VMCPUID idCpu);
GVMMR0DECL(int)     GVMMR0SchedPoke(PGVM pGVM, PVM pVM, VMCPUID idCpu);
GVMMR0DECL(int)     GVMMR0SchedPokeEx(PGVM pGVM, PVM pVM, VMCPUID idCpu, bool fTakeUsedLock);
GVMMR0DECL(int)     GVMMR0SchedPokeNoGVMNoLock(PVM pVM, VMCPUID idCpu);
GVMMR0DECL(int)     GVMMR0SchedWakeUpAndPokeCpus(PGVM pGVM, PVM pVM, PCVMCPUSET pSleepSet, PCVMCPUSET pPokeSet);
GVMMR0DECL(int)     GVMMR0SchedPoll(PGVM pGVM, PVM pVM, VMCPUID idCpu, bool fYield);
GVMMR0DECL(void)    GVMMR0SchedUpdatePeriodicPreemptionTimer(PVM pVM, RTCPUID idHostCpu, uint32_t uHz);
GVMMR0DECL(int)     GVMMR0QueryStatistics(PGVMMSTATS pStats, PSUPDRVSESSION pSession, PGVM pGVM, PVM pVM);
GVMMR0DECL(int)     GVMMR0ResetStatistics(PCGVMMSTATS pStats, PSUPDRVSESSION pSession, PGVM pGVM, PVM pVM);


/**
 * Request packet for calling GVMMR0CreateVM.
 */
typedef struct GVMMCREATEVMREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** The support driver session. (IN) */
    PSUPDRVSESSION  pSession;
    /** Number of virtual CPUs for the new VM. (IN) */
    uint32_t        cCpus;
    /** Pointer to the ring-3 mapping of the shared VM structure on return. (OUT) */
    PVMR3           pVMR3;
    /** Pointer to the ring-0 mapping of the shared VM structure on return. (OUT) */
    PVMR0           pVMR0;
} GVMMCREATEVMREQ;
/** Pointer to a GVMMR0CreateVM request packet. */
typedef GVMMCREATEVMREQ *PGVMMCREATEVMREQ;

GVMMR0DECL(int)     GVMMR0CreateVMReq(PGVMMCREATEVMREQ pReq, PSUPDRVSESSION pSession);


/**
 * Request buffer for GVMMR0SchedWakeUpAndPokeCpusReq / VMMR0_DO_GVMM_SCHED_WAKE_UP_AND_POKE_CPUS.
 * @see GVMMR0SchedWakeUpAndPokeCpus.
 */
typedef struct GVMMSCHEDWAKEUPANDPOKECPUSREQ /* nice and unreadable... */
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The sleeper set. */
    VMCPUSET        SleepSet;
    /** The set of virtual CPUs to poke. */
    VMCPUSET        PokeSet;
} GVMMSCHEDWAKEUPANDPOKECPUSREQ;
/** Pointer to a GVMMR0QueryStatisticsReq / VMMR0_DO_GVMM_QUERY_STATISTICS request buffer. */
typedef GVMMSCHEDWAKEUPANDPOKECPUSREQ *PGVMMSCHEDWAKEUPANDPOKECPUSREQ;

GVMMR0DECL(int)     GVMMR0SchedWakeUpAndPokeCpusReq(PGVM pGVM, PVM pVM, PGVMMSCHEDWAKEUPANDPOKECPUSREQ pReq);


/**
 * Request buffer for GVMMR0QueryStatisticsReq / VMMR0_DO_GVMM_QUERY_STATISTICS.
 * @see GVMMR0QueryStatistics.
 */
typedef struct GVMMQUERYSTATISTICSSREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The support driver session. */
    PSUPDRVSESSION  pSession;
    /** The statistics. */
    GVMMSTATS       Stats;
} GVMMQUERYSTATISTICSSREQ;
/** Pointer to a GVMMR0QueryStatisticsReq / VMMR0_DO_GVMM_QUERY_STATISTICS request buffer. */
typedef GVMMQUERYSTATISTICSSREQ *PGVMMQUERYSTATISTICSSREQ;

GVMMR0DECL(int)     GVMMR0QueryStatisticsReq(PGVM pGVM, PVM pVM, PGVMMQUERYSTATISTICSSREQ pReq, PSUPDRVSESSION pSession);


/**
 * Request buffer for GVMMR0ResetStatisticsReq / VMMR0_DO_GVMM_RESET_STATISTICS.
 * @see GVMMR0ResetStatistics.
 */
typedef struct GVMMRESETSTATISTICSSREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The support driver session. */
    PSUPDRVSESSION  pSession;
    /** The statistics to reset.
     * Any non-zero entry will be reset (if permitted). */
    GVMMSTATS       Stats;
} GVMMRESETSTATISTICSSREQ;
/** Pointer to a GVMMR0ResetStatisticsReq / VMMR0_DO_GVMM_RESET_STATISTICS request buffer. */
typedef GVMMRESETSTATISTICSSREQ *PGVMMRESETSTATISTICSSREQ;

GVMMR0DECL(int)     GVMMR0ResetStatisticsReq(PGVM pGVM, PVM pVM, PGVMMRESETSTATISTICSSREQ pReq, PSUPDRVSESSION pSession);


/** @} */

RT_C_DECLS_END

#endif


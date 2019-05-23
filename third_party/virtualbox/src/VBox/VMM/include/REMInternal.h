/* $Id: REMInternal.h $ */
/** @file
 * REM - Internal header file.
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

#ifndef ___REMInternal_h
#define ___REMInternal_h

#include <VBox/types.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/pdmcritsect.h>
#ifdef REM_INCLUDE_CPU_H
# include "target-i386/cpu.h"
#endif



/** @defgroup grp_rem_int   Internals
 * @ingroup grp_rem
 * @internal
 * @{
 */

/** The saved state version number. */
#define REM_SAVED_STATE_VERSION_VER1_6   6
#define REM_SAVED_STATE_VERSION          7


/** @def REM_MONITOR_CODE_PAGES
 * Enable to monitor code pages that have been translated by the recompiler. */
/** Currently broken and interferes with CSAM monitoring (see @bugref{2784}) */
////#define REM_MONITOR_CODE_PAGES
#ifdef DOXYGEN_RUNNING
# define REM_MONITOR_CODE_PAGES
#endif

typedef enum REMHANDLERNOTIFICATIONKIND
{
    /** The usual invalid 0 entry. */
    REMHANDLERNOTIFICATIONKIND_INVALID = 0,
    /** REMR3NotifyHandlerPhysicalRegister. */
    REMHANDLERNOTIFICATIONKIND_PHYSICAL_REGISTER,
    /** REMR3NotifyHandlerPhysicalDeregister. */
    REMHANDLERNOTIFICATIONKIND_PHYSICAL_DEREGISTER,
    /** REMR3NotifyHandlerPhysicalModify. */
    REMHANDLERNOTIFICATIONKIND_PHYSICAL_MODIFY,
    /** The usual 32-bit hack. */
    REMHANDLERNOTIFICATIONKIND_32BIT_HACK = 0x7fffffff
} REMHANDLERNOTIFICATIONKIND;


/**
 * A recorded handler notification.
 */
typedef struct REMHANDLERNOTIFICATION
{
    /** The notification kind. */
    REMHANDLERNOTIFICATIONKIND  enmKind;
    uint32_t                    padding;
    /** Type specific data. */
    union
    {
        struct
        {
            RTGCPHYS            GCPhys;
            RTGCPHYS            cb;
            PGMPHYSHANDLERKIND  enmKind;
            bool                fHasHCHandler;
        } PhysicalRegister;

        struct
        {
            RTGCPHYS            GCPhys;
            RTGCPHYS            cb;
            PGMPHYSHANDLERKIND  enmKind;
            bool                fHasHCHandler;
            bool                fRestoreAsRAM;
        } PhysicalDeregister;

        struct
        {
            RTGCPHYS            GCPhysOld;
            RTGCPHYS            GCPhysNew;
            RTGCPHYS            cb;
            PGMPHYSHANDLERKIND  enmKind;
            bool                fHasHCHandler;
            bool                fRestoreAsRAM;
        } PhysicalModify;
        uint64_t                padding[5];
    } u;
    uint32_t                    idxSelf;
    uint32_t volatile           idxNext;
} REMHANDLERNOTIFICATION;
/** Pointer to a handler notification record. */
typedef REMHANDLERNOTIFICATION *PREMHANDLERNOTIFICATION;

/**
 * Converts a REM pointer into a VM pointer.
 * @returns Pointer to the VM structure the REM is part of.
 * @param   pREM    Pointer to REM instance data.
 */
#define REM2VM(pREM)  ( (PVM)((char*)pREM - pREM->offVM) )


/**
 * REM Data (part of VM)
 */
typedef struct REM
{
    /** Offset to the VM structure. */
    RTINT                   offVM;
    /** Alignment padding. */
    RTUINT                  uPadding0;

    /** Cached pointer of the register context of the current VCPU. */
    R3PTRTYPE(PCPUMCTX)     pCtx;

    /** In REM mode.
     * I.e. the correct CPU state and some other bits are with REM. */
    bool volatile           fInREM;
    /** In REMR3State. */
    bool                    fInStateSync;

    /** Set when the translation blocks cache need to be flushed. */
    bool                    fFlushTBs;

    /** Ignore CR3 load notifications from the REM. */
    bool                    fIgnoreCR3Load;
    /** Ignore invlpg notifications from the REM. */
    bool                    fIgnoreInvlPg;
    /** Ignore CR0, CR4 and EFER load. */
    bool                    fIgnoreCpuMode;
    /** Ignore set page. */
    bool                    fIgnoreSetPage;
    bool                    bPadding1;

    /** Ignore all that can be ignored. */
    uint32_t                cIgnoreAll;

    /** Number of times REMR3CanExecuteRaw has been called.
     * It is used to prevent rescheduling on the first call. */
    uint32_t                cCanExecuteRaw;

    /** Pending interrupt that remR3LoadDone will assert with TRPM. */
    uint32_t                uStateLoadPendingInterrupt;

    /** Number of recorded invlpg instructions. */
    uint32_t volatile       cInvalidatedPages;
#if HC_ARCH_BITS == 32
    uint32_t                uPadding2;
#endif
    /** Array of recorded invlpg instruction.
     * These instructions are replayed when entering REM. */
    RTGCPTR                 aGCPtrInvalidatedPages[48];

    /** Array of recorded handler notifications.
     * These are replayed when entering REM. */
    REMHANDLERNOTIFICATION  aHandlerNotifications[64];
    volatile uint32_t       idxPendingList;
    volatile uint32_t       idxFreeList;

    /** MMIO memory type.
     * This is used to register MMIO physical access handlers. */
    int32_t                 iMMIOMemType;
    /** Handler memory type.
     * This is used to register non-MMIO physical access handlers which are executed in HC. */
    int32_t                 iHandlerMemType;

    /** Pending exception */
    uint32_t                uPendingException;
    /** Nr of pending exceptions */
    uint32_t                cPendingExceptions;
    /** Pending exception's EIP */
    RTGCPTR                 uPendingExcptEIP;
    /** Pending exception's CR2 */
    RTGCPTR                 uPendingExcptCR2;

    /** The highest known RAM address. */
    RTGCPHYS                GCPhysLastRam;
    /** Whether GCPhysLastRam has been fixed (see REMR3Init()). */
    bool                    fGCPhysLastRamFixed;

    /** Pending rc. */
    int32_t                 rc;

    /** REM critical section.
     * This protects cpu_register_physical_memory usage
     */
    PDMCRITSECT             CritSectRegister;

    /** Time spent in QEMU. */
    STAMPROFILEADV          StatsInQEMU;
    /** Time spent in rawmode.c. */
    STAMPROFILEADV          StatsInRAWEx;
    /** Time spent switching state. */
    STAMPROFILE             StatsState;
    /** Time spent switching state back. */
    STAMPROFILE             StatsStateBack;

    /** Padding the CPUX86State structure to 64 byte. */
    uint32_t                abPadding[HC_ARCH_BITS == 32 ? 4 : 4];

# define REM_ENV_SIZE       0xff00

    /** Recompiler CPU state. */
#ifdef REM_INCLUDE_CPU_H
    CPUX86State             Env;
#else
    struct FakeEnv
    {
        char                achPadding[REM_ENV_SIZE];
    }                       Env;
#endif /* !REM_INCLUDE_CPU_H */
} REM;

/** Pointer to the REM Data. */
typedef REM *PREM;


#ifdef REM_INCLUDE_CPU_H
bool    remR3CanExecuteRaw(CPUState *env, RTGCPTR eip, unsigned fFlags, int *piException);
void    remR3CSAMCheckEIP(CPUState *env, RTGCPTR GCPtrCode);
# ifdef VBOX_WITH_RAW_MODE
bool    remR3GetOpcode(CPUState *env, RTGCPTR GCPtrInstr, uint8_t *pu8Byte);
# endif
bool    remR3DisasInstr(CPUState *env, int f32BitCode, char *pszPrefix);
void    remR3FlushPage(CPUState *env, RTGCPTR GCPtr);
void    remR3FlushTLB(CPUState *env, bool fGlobal);
void    remR3ProtectCode(CPUState *env, RTGCPTR GCPtr);
void    remR3ChangeCpuMode(CPUState *env);
void    remR3DmaRun(CPUState *env);
void    remR3TimersRun(CPUState *env);
int     remR3NotifyTrap(CPUState *env, uint32_t uTrap, uint32_t uErrorCode, RTGCPTR pvNextEIP);
void    remR3TrapStat(CPUState *env, uint32_t uTrap);
void    remR3RecordCall(CPUState *env);
#endif /* REM_INCLUDE_CPU_H */
void    remR3TrapClear(PVM pVM);
void    remR3RaiseRC(PVM pVM, int rc);
void    remR3DumpLnxSyscall(PVMCPU pVCpu);
void    remR3DumpOBsdSyscall(PVMCPU pVCpu);


/** @todo r=bird: clean up the RAWEx stats. */
/* temporary hacks */
#define RAWEx_ProfileStart(a, b)    remR3ProfileStart(b)
#define RAWEx_ProfileStop(a, b)     remR3ProfileStop(b)


#ifdef VBOX_WITH_STATISTICS

# define STATS_EMULATE_SINGLE_INSTR         1
# define STATS_QEMU_COMPILATION             2
# define STATS_QEMU_RUN_EMULATED_CODE       3
# define STATS_QEMU_TOTAL                   4
# define STATS_QEMU_RUN_TIMERS              5
# define STATS_TLB_LOOKUP                   6
# define STATS_IRQ_HANDLING                 7
# define STATS_RAW_CHECK                    8

void remR3ProfileStart(int statcode);
void remR3ProfileStop(int statcode);

#else  /* !VBOX_WITH_STATISTICS */
# define remR3ProfileStart(c)   do { } while (0)
# define remR3ProfileStop(c)    do { } while (0)
#endif /* !VBOX_WITH_STATISTICS */

/** @} */

#endif


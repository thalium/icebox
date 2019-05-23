/* $Id: TRPMInternal.h $ */
/** @file
 * TRPM - Internal header file.
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

#ifndef ___TRPMInternal_h
#define ___TRPMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pgm.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_trpm_int   Internals
 * @ingroup grp_trpm
 * @internal
 * @{
 */


#ifdef VBOX_WITH_RAW_MODE
/** Enable or disable tracking of Guest's IDT. */
# define TRPM_TRACK_GUEST_IDT_CHANGES
/** Enable or disable tracking of Shadow IDT. */
# define TRPM_TRACK_SHADOW_IDT_CHANGES
#endif


/** Enable to allow trap forwarding in GC. */
#ifdef VBOX_WITH_RAW_MODE
# define TRPM_FORWARD_TRAPS_IN_GC
#endif

/** First interrupt handler. Used for validating input. */
#define TRPM_HANDLER_INT_BASE  0x20


/** @name   TRPMGCTrapIn* flags.
 * The lower bits are offsets into the CPUMCTXCORE structure.
 * @{ */
/** The mask for the operation. */
#define TRPM_TRAP_IN_OP_MASK    0xffff
/** Traps on MOV GS, eax. */
#define TRPM_TRAP_IN_MOV_GS     1
/** Traps on MOV FS, eax. */
#define TRPM_TRAP_IN_MOV_FS     2
/** Traps on MOV ES, eax. */
#define TRPM_TRAP_IN_MOV_ES     3
/** Traps on MOV DS, eax. */
#define TRPM_TRAP_IN_MOV_DS     4
/** Traps on IRET. */
#define TRPM_TRAP_IN_IRET       5
/** Set if this is a V86 resume. */
#define TRPM_TRAP_IN_V86        RT_BIT(30)
/** @} */


#if 0 /* not used */
/**
 * Converts a TRPM pointer into a VM pointer.
 * @returns Pointer to the VM structure the TRPM is part of.
 * @param   pTRPM       Pointer to TRPM instance data.
 */
#define TRPM_2_VM(pTRPM)            ( (PVM)((uint8_t *)(pTRPM) - (pTRPM)->offVM) )
#endif

/**
 * Converts a TRPM pointer into a TRPMCPU pointer.
 * @returns Pointer to the VM structure the TRPMCPU is part of.
 * @param   pTrpmCpu    Pointer to TRPMCPU instance data.
 * @remarks Raw-mode only, not SMP safe.
 */
#define TRPM_2_TRPMCPU(pTrpmCpu)     ( (PTRPMCPU)((uint8_t *)(pTrpmCpu) + (pTrpmCpu)->offTRPMCPU) )


/**
 * TRPM Data (part of VM)
 *
 * IMPORTANT! Keep the nasm version of this struct up-to-date.
 */
typedef struct TRPM
{
    /** Offset to the VM structure.
     * See TRPM_2_VM(). */
    RTINT                   offVM;
    /** Offset to the TRPMCPU structure.
     * See TRPM2TRPMCPU(). */
    RTINT                   offTRPMCPU;

    /** Whether monitoring of the guest IDT is enabled or not.
     *
     * This configuration option is provided for speeding up guest like Solaris
     * that put the IDT on the same page as a whole lot of other data that is
     * frequently updated. The updates will cause \#PFs and have to be interpreted
     * by PGMInterpretInstruction which is slow compared to raw execution.
     *
     * If the guest is well behaved and doesn't change the IDT after loading it,
     * there is no problem with dropping the IDT monitoring.
     *
     * @cfgm{/TRPM/SafeToDropGuestIDTMonitoring, boolean, defaults to false.}
     */
    bool                    fSafeToDropGuestIDTMonitoring;

    /** Padding to get the IDTs at a 16 byte alignment. */
    uint8_t                 abPadding1[7];
    /** IDTs. Aligned at 16 byte offset for speed. */
    VBOXIDTE                aIdt[256];

    /** Bitmap for IDTEs that contain PATM handlers. (needed for relocation) */
    uint32_t                au32IdtPatched[8];

    /** Temporary Hypervisor trap handlers.
     * NULL means default action. */
    RCPTRTYPE(void *)       aTmpTrapHandlers[256];

    /** RC Pointer to the IDT shadow area (aIdt) in HMA. */
    RCPTRTYPE(void *)       pvMonShwIdtRC;
    /** padding. */
    uint8_t                 au8Padding[2];
    /** Current (last) Guest's IDTR. */
    VBOXIDTR                GuestIdtr;
    /** Shadow IDT virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hShadowIdtWriteHandlerType;
    /** Guest IDT virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hGuestIdtWriteHandlerType;

    /** Checked trap & interrupt handler array */
    RCPTRTYPE(void *)       aGuestTrapHandler[256];

    /** RC: The number of times writes to the Guest IDT were detected. */
    STAMCOUNTER             StatRCWriteGuestIDTFault;
    STAMCOUNTER             StatRCWriteGuestIDTHandled;

    /** HC: Profiling of the TRPMR3SyncIDT() method. */
    STAMPROFILE             StatSyncIDT;
    /** GC: Statistics for the trap handlers. */
    STAMPROFILEADV          aStatGCTraps[0x14];

    STAMPROFILEADV          StatForwardProfR3;
    STAMPROFILEADV          StatForwardProfRZ;
    STAMCOUNTER             StatForwardFailNoHandler;
    STAMCOUNTER             StatForwardFailPatchAddr;
    STAMCOUNTER             StatForwardFailR3;
    STAMCOUNTER             StatForwardFailRZ;

    STAMPROFILE             StatTrap0dDisasm;
    STAMCOUNTER             StatTrap0dRdTsc;    /**< Number of RDTSC \#GPs. */

#ifdef VBOX_WITH_STATISTICS
    /** Statistics for interrupt handlers (allocated on the hypervisor heap) - R3
     * pointer. */
    R3PTRTYPE(PSTAMCOUNTER) paStatForwardedIRQR3;
    /** Statistics for interrupt handlers - RC pointer. */
    RCPTRTYPE(PSTAMCOUNTER) paStatForwardedIRQRC;

    /** Host interrupt statistics (allocated on the hypervisor heap) - RC ptr. */
    RCPTRTYPE(PSTAMCOUNTER) paStatHostIrqRC;
    /** Host interrupt statistics (allocated on the hypervisor heap) - R3 ptr. */
    R3PTRTYPE(PSTAMCOUNTER) paStatHostIrqR3;
#endif
} TRPM;
AssertCompileMemberAlignment(TRPM, GuestIdtr.pIdt, 8);

/** Pointer to TRPM Data. */
typedef TRPM *PTRPM;


/**
 * Converts a TRPMCPU pointer into a VM pointer.
 * @returns Pointer to the VM structure the TRPMCPU is part of.
 * @param   pTrpmCpu    Pointer to TRPMCPU instance data.
 */
#define TRPMCPU_2_VM(pTrpmCpu)      ( (PVM)((uint8_t *)(pTrpmCpu) - (pTrpmCpu)->offVM) )

/**
 * Converts a TRPMCPU pointer into a VMCPU pointer.
 * @returns Pointer to the VMCPU structure the TRPMCPU is part of.
 * @param   pTrpmCpu    Pointer to TRPMCPU instance data.
 */
#define TRPMCPU_2_VMCPU(pTrpmCpu)   ( (PVMCPU)((uint8_t *)(pTrpmCpu) - (pTrpmCpu)->offVMCpu) )


/**
 * Per CPU data for TRPM.
 */
typedef struct TRPMCPU
{
    /** Offset into the VM structure.
     * See TRPMCPU_2_VM(). */
    uint32_t                offVM;
    /** Offset into the VMCPU structure.
     * See TRPMCPU_2_VMCPU().  */
    uint32_t                offVMCpu;

    /** Active Interrupt or trap vector number.
     * If not UINT32_MAX this indicates that we're currently processing a
     * interrupt, trap, fault, abort, whatever which have arrived at that
     * vector number.
     */
    uint32_t                uActiveVector;

    /** Active trap type. */
    TRPMEVENT               enmActiveType;

    /** Errorcode for the active interrupt/trap. */
    RTGCUINT                uActiveErrorCode; /**< @todo don't use RTGCUINT */

    /** CR2 at the time of the active exception. */
    RTGCUINTPTR             uActiveCR2;

    /** Saved trap vector number. */
    RTGCUINT                uSavedVector; /**< @todo don't use RTGCUINT */

    /** Saved errorcode. */
    RTGCUINT                uSavedErrorCode;

    /** Saved cr2. */
    RTGCUINTPTR             uSavedCR2;

    /** Saved trap type. */
    TRPMEVENT               enmSavedType;

    /** Instruction length for software interrupts and software exceptions
     * (\#BP, \#OF) */
    uint8_t                 cbInstr;

    /** Saved instruction length. */
    uint8_t                 cbSavedInstr;

    /** Padding. */
    uint8_t                 au8Padding[2];

    /** Previous trap vector # - for debugging. */
    RTGCUINT                uPrevVector;
} TRPMCPU;

/** Pointer to TRPMCPU Data. */
typedef TRPMCPU *PTRPMCPU;


PGM_ALL_CB2_PROTO(FNPGMVIRTHANDLER) trpmGuestIDTWriteHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    trpmRCGuestIDTWritePfHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    trpmRCShadowIDTWritePfHandler;

/**
 * Clear guest trap/interrupt gate handler
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   iTrap       Interrupt/trap number.
 */
VMMDECL(int) trpmClearGuestTrapHandler(PVM pVM, unsigned iTrap);


#ifdef IN_RING3
int trpmR3ClearPassThroughHandler(PVM pVM, unsigned iTrap);
#endif


#ifdef IN_RING0

/**
 * Calls the interrupt gate as if we received an interrupt while in Ring-0.
 *
 * @param   uIP     The interrupt gate IP.
 * @param   SelCS   The interrupt gate CS.
 * @param   RSP     The interrupt gate RSP. ~0 if no stack switch should take place. (only AMD64)
 */
DECLASM(void) trpmR0DispatchHostInterrupt(RTR0UINTPTR uIP, RTSEL SelCS, RTR0UINTPTR RSP);

/**
 * Issues a software interrupt to the specified interrupt vector.
 *
 * @param   uActiveVector   The vector number.
 */
DECLASM(void) trpmR0DispatchHostInterruptSimple(RTUINT uActiveVector);

#endif /* IN_RING0 */

/** @} */

RT_C_DECLS_END

#endif

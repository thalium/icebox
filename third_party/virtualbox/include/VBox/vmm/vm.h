/** @file
 * VM - The Virtual Machine, data.
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

#ifndef ___VBox_vmm_vm_h
#define ___VBox_vmm_vm_h

#ifndef VBOX_FOR_DTRACE_LIB
# include <iprt/param.h>
# include <VBox/types.h>
# include <VBox/vmm/cpum.h>
# include <VBox/vmm/stam.h>
# include <VBox/vmm/vmapi.h>
# include <VBox/vmm/vmm.h>
# include <VBox/sup.h>
#else
# pragma D depends_on library vbox-types.d
# pragma D depends_on library CPUMInternal.d
# define ___CPUMInternal_h
#endif



/** @defgroup grp_vm    The Virtual Machine
 * @ingroup grp_vmm
 * @{
 */

/**
 * The state of a Virtual CPU.
 *
 * The basic state indicated here is whether the CPU has been started or not. In
 * addition, there are sub-states when started for assisting scheduling (GVMM
 * mostly).
 *
 * The transition out of the STOPPED state is done by a vmR3PowerOn.
 * The transition back to the STOPPED state is done by vmR3PowerOff.
 *
 * (Alternatively we could let vmR3PowerOn start CPU 0 only and let the SPIP
 * handling switch on the other CPUs. Then vmR3Reset would stop all but CPU 0.)
 */
typedef enum VMCPUSTATE
{
    /** The customary invalid zero. */
    VMCPUSTATE_INVALID = 0,

    /** Virtual CPU has not yet been started.  */
    VMCPUSTATE_STOPPED,

    /** CPU started. */
    VMCPUSTATE_STARTED,
    /** CPU started in HM context. */
    VMCPUSTATE_STARTED_HM,
    /** Executing guest code and can be poked (RC or STI bits of HM). */
    VMCPUSTATE_STARTED_EXEC,
    /** Executing guest code in the recompiler. */
    VMCPUSTATE_STARTED_EXEC_REM,
    /** Halted. */
    VMCPUSTATE_STARTED_HALTED,

    /** The end of valid virtual CPU states. */
    VMCPUSTATE_END,

    /** Ensure 32-bit type. */
    VMCPUSTATE_32BIT_HACK = 0x7fffffff
} VMCPUSTATE;


/**
 * The cross context virtual CPU structure.
 *
 * Run 'kmk run-struct-tests' (from src/VBox/VMM if you like) after updating!
 */
typedef struct VMCPU
{
    /** Per CPU forced action.
     * See the VMCPU_FF_* \#defines. Updated atomically. */
    uint32_t volatile       fLocalForcedActions;                    /* 0 */
    /** The CPU state. */
    VMCPUSTATE volatile     enmState;                               /* 4 */

    /** Pointer to the ring-3 UVMCPU structure. */
    PUVMCPU                 pUVCpu;                                 /* 8 */
    /** Ring-3 Host Context VM Pointer. */
    PVMR3                   pVMR3;                                  /* 16 / 12 */
    /** Ring-0 Host Context VM Pointer. */
    PVMR0                   pVMR0;                                  /* 24 / 16 */
    /** Raw-mode Context VM Pointer. */
    PVMRC                   pVMRC;                                  /* 32 / 20 */
    /** The CPU ID.
     * This is the index into the VM::aCpu array. */
    VMCPUID                 idCpu;                                  /* 36 / 24 */
    /** The native thread handle. */
    RTNATIVETHREAD          hNativeThread;                          /* 40 / 28 */
    /** The native R0 thread handle. (different from the R3 handle!) */
    RTNATIVETHREAD          hNativeThreadR0;                        /* 48 / 32 */
    /** Which host CPU ID is this EMT running on.
     * Only valid when in RC or HMR0 with scheduling disabled. */
    RTCPUID volatile        idHostCpu;                              /* 56 / 36 */
    /** The CPU set index corresponding to idHostCpu, UINT32_MAX if not valid.
     * @remarks Best to make sure iHostCpuSet shares cache line with idHostCpu! */
    uint32_t volatile       iHostCpuSet;                            /* 60 / 40 */

#if HC_ARCH_BITS == 32
    /** Align the structures below bit on a 64-byte boundary and make sure it starts
     * at the same offset in both 64-bit and 32-bit builds.
     *
     * @remarks The alignments of the members that are larger than 48 bytes should be
     *          64-byte for cache line reasons. structs containing small amounts of
     *          data could be lumped together at the end with a < 64 byte padding
     *          following it (to grow into and align the struct size).
     */
    uint8_t                 abAlignment1[HC_ARCH_BITS == 64 ? 0 : 20];
#endif

    /** IEM part.
     * @remarks This comes first as it allows the use of 8-bit immediates for the
     *          first 64 bytes of the structure, reducing code size a wee bit. */
#ifdef ___IEMInternal_h /* For PDB hacking. */
    union VMCPUUNIONIEMFULL
#else
    union VMCPUUNIONIEMSTUB
#endif
    {
#ifdef ___IEMInternal_h
        struct IEMCPU       s;
#endif
        uint8_t             padding[18496];     /* multiple of 64 */
    } iem;

    /** HM part. */
    union VMCPUUNIONHM
    {
#ifdef ___HMInternal_h
        struct HMCPU    s;
#endif
        uint8_t             padding[5824];      /* multiple of 64 */
    } hm;

    /** EM part. */
    union VMCPUUNIONEM
    {
#ifdef ___EMInternal_h
        struct EMCPU        s;
#endif
        uint8_t             padding[1408];      /* multiple of 64 */
    } em;

    /** TRPM part. */
    union VMCPUUNIONTRPM
    {
#ifdef ___TRPMInternal_h
        struct TRPMCPU      s;
#endif
        uint8_t             padding[128];       /* multiple of 64 */
    } trpm;

    /** TM part. */
    union VMCPUUNIONTM
    {
#ifdef ___TMInternal_h
        struct TMCPU        s;
#endif
        uint8_t             padding[384];       /* multiple of 64 */
    } tm;

    /** VMM part. */
    union VMCPUUNIONVMM
    {
#ifdef ___VMMInternal_h
        struct VMMCPU       s;
#endif
        uint8_t             padding[704];       /* multiple of 64 */
    } vmm;

    /** PDM part. */
    union VMCPUUNIONPDM
    {
#ifdef ___PDMInternal_h
        struct PDMCPU       s;
#endif
        uint8_t             padding[256];       /* multiple of 64 */
    } pdm;

    /** IOM part. */
    union VMCPUUNIONIOM
    {
#ifdef ___IOMInternal_h
        struct IOMCPU       s;
#endif
        uint8_t             padding[512];       /* multiple of 64 */
    } iom;

    /** DBGF part.
     * @todo Combine this with other tiny structures. */
    union VMCPUUNIONDBGF
    {
#ifdef ___DBGFInternal_h
        struct DBGFCPU      s;
#endif
        uint8_t             padding[256];       /* multiple of 64 */
    } dbgf;

    /** GIM part. */
    union VMCPUUNIONGIM
    {
#ifdef ___GIMInternal_h
        struct GIMCPU s;
#endif
        uint8_t             padding[512];       /* multiple of 64 */
    } gim;

    /** APIC part. */
    union VMCPUUNIONAPIC
    {
#ifdef ___APICInternal_h
        struct APICCPU      s;
#endif
        uint8_t             padding[1792];      /* multiple of 64 */
    } apic;

    /*
     * Some less frequently used global members that doesn't need to take up
     * precious space at the head of the structure.
     */

    /** Trace groups enable flags.  */
    uint32_t                fTraceGroups;                           /* 64 / 44 */
    /** State data for use by ad hoc profiling. */
    uint32_t                uAdHoc;
    /** Profiling samples for use by ad hoc profiling. */
    STAMPROFILEADV          aStatAdHoc[8];                          /* size: 40*8 = 320 */

    /** Align the following members on page boundary. */
    uint8_t                 abAlignment2[2104];

    /** PGM part. */
    union VMCPUUNIONPGM
    {
#ifdef ___PGMInternal_h
        struct PGMCPU       s;
#endif
        uint8_t             padding[4096];      /* multiple of 4096 */
    } pgm;

    /** CPUM part. */
    union VMCPUUNIONCPUM
    {
#ifdef ___CPUMInternal_h
        struct CPUMCPU      s;
#endif
#ifdef VMCPU_INCL_CPUM_GST_CTX
        /** The guest CPUM context for direct use by execution engines.
         * This is not for general consumption, but for HM, REM, IEM, and maybe a few
         * others.  The rest will use the function based CPUM API. */
        CPUMCTX             GstCtx;
#endif
        uint8_t             padding[4096];      /* multiple of 4096 */
    } cpum;
} VMCPU;


#ifndef VBOX_FOR_DTRACE_LIB

/** @name Operations on VMCPU::enmState
 * @{ */
/** Gets the VMCPU state. */
#define VMCPU_GET_STATE(pVCpu)              ( (pVCpu)->enmState )
/** Sets the VMCPU state. */
#define VMCPU_SET_STATE(pVCpu, enmNewState) \
    ASMAtomicWriteU32((uint32_t volatile *)&(pVCpu)->enmState, (enmNewState))
/** Cmpares and sets the VMCPU state. */
#define VMCPU_CMPXCHG_STATE(pVCpu, enmNewState, enmOldState) \
    ASMAtomicCmpXchgU32((uint32_t volatile *)&(pVCpu)->enmState, (enmNewState), (enmOldState))
/** Checks the VMCPU state. */
#ifdef VBOX_STRICT
# define VMCPU_ASSERT_STATE(pVCpu, enmExpectedState) \
    do { \
        VMCPUSTATE enmState = VMCPU_GET_STATE(pVCpu); \
        AssertMsg(enmState == (enmExpectedState), \
                  ("enmState=%d  enmExpectedState=%d idCpu=%u\n", \
                  enmState, enmExpectedState, (pVCpu)->idCpu)); \
    } while (0)
#else
# define VMCPU_ASSERT_STATE(pVCpu, enmExpectedState) do { } while (0)
#endif
/** Tests if the state means that the CPU is started. */
#define VMCPUSTATE_IS_STARTED(enmState)     ( (enmState) > VMCPUSTATE_STOPPED )
/** Tests if the state means that the CPU is stopped. */
#define VMCPUSTATE_IS_STOPPED(enmState)     ( (enmState) == VMCPUSTATE_STOPPED )
/** @} */


/** The name of the raw-mode context VMM Core module. */
#define VMMRC_MAIN_MODULE_NAME          "VMMRC.rc"
/** The name of the ring-0 context VMM Core module. */
#define VMMR0_MAIN_MODULE_NAME          "VMMR0.r0"

/**
 * Wrapper macro for avoiding too much \#ifdef VBOX_WITH_RAW_MODE.
 */
#ifdef VBOX_WITH_RAW_MODE
# define VM_WHEN_RAW_MODE(a_WithExpr, a_WithoutExpr)    a_WithExpr
#else
# define VM_WHEN_RAW_MODE(a_WithExpr, a_WithoutExpr)    a_WithoutExpr
#endif


/** VM Forced Action Flags.
 *
 * Use the VM_FF_SET() and VM_FF_CLEAR() macros to change the force
 * action mask of a VM.
 *
 * Available VM bits:
 *      0, 1, 5, 6, 7, 13, 14, 15, 16, 17, 21, 22, 23, 24, 25, 26, 27, 28, 30
 *
 *
 * Available VMCPU bits:
 *      11, 14, 15, 31
 *
 * @todo If we run low on VMCPU, we may consider merging the SELM bits
 *
 * @{
 */
/** The virtual sync clock has been stopped, go to TM until it has been
 *  restarted... */
#define VM_FF_TM_VIRTUAL_SYNC               RT_BIT_32(2)
/** PDM Queues are pending. */
#define VM_FF_PDM_QUEUES                    RT_BIT_32(VM_FF_PDM_QUEUES_BIT)
/** The bit number for VM_FF_PDM_QUEUES. */
#define VM_FF_PDM_QUEUES_BIT                3
/** PDM DMA transfers are pending. */
#define VM_FF_PDM_DMA                       RT_BIT_32(VM_FF_PDM_DMA_BIT)
/** The bit number for VM_FF_PDM_DMA. */
#define VM_FF_PDM_DMA_BIT                   4
/** This action forces the VM to call DBGF so DBGF can service debugger
 * requests in the emulation thread.
 * This action flag stays asserted till DBGF clears it.*/
#define VM_FF_DBGF                          RT_BIT_32(VM_FF_DBGF_BIT)
/** The bit number for VM_FF_DBGF. */
#define VM_FF_DBGF_BIT                      8
/** This action forces the VM to service pending requests from other
 * thread or requests which must be executed in another context. */
#define VM_FF_REQUEST                       RT_BIT_32(9)
/** Check for VM state changes and take appropriate action. */
#define VM_FF_CHECK_VM_STATE                RT_BIT_32(VM_FF_CHECK_VM_STATE_BIT)
/** The bit number for VM_FF_CHECK_VM_STATE. */
#define VM_FF_CHECK_VM_STATE_BIT            10
/** Reset the VM. (postponed) */
#define VM_FF_RESET                         RT_BIT_32(VM_FF_RESET_BIT)
/** The bit number for VM_FF_RESET. */
#define VM_FF_RESET_BIT                     11
/** EMT rendezvous in VMM. */
#define VM_FF_EMT_RENDEZVOUS                RT_BIT_32(VM_FF_EMT_RENDEZVOUS_BIT)
/** The bit number for VM_FF_EMT_RENDEZVOUS. */
#define VM_FF_EMT_RENDEZVOUS_BIT            12

/** PGM needs to allocate handy pages. */
#define VM_FF_PGM_NEED_HANDY_PAGES          RT_BIT_32(18)
/** PGM is out of memory.
 * Abandon all loops and code paths which can be resumed and get up to the EM
 * loops. */
#define VM_FF_PGM_NO_MEMORY                 RT_BIT_32(19)
 /** PGM is about to perform a lightweight pool flush
  *  Guest SMP: all EMT threads should return to ring 3
  */
#define VM_FF_PGM_POOL_FLUSH_PENDING        RT_BIT_32(20)
/** REM needs to be informed about handler changes. */
#define VM_FF_REM_HANDLER_NOTIFY            RT_BIT_32(VM_FF_REM_HANDLER_NOTIFY_BIT)
/** The bit number for VM_FF_REM_HANDLER_NOTIFY. */
#define VM_FF_REM_HANDLER_NOTIFY_BIT        29
/** Suspend the VM - debug only. */
#define VM_FF_DEBUG_SUSPEND                 RT_BIT_32(31)


/** This action forces the VM to check any pending interrupts on the APIC. */
#define VMCPU_FF_INTERRUPT_APIC             RT_BIT_32(0)
/** This action forces the VM to check any pending interrups on the PIC. */
#define VMCPU_FF_INTERRUPT_PIC              RT_BIT_32(1)
/** This action forces the VM to schedule and run pending timer (TM).
 * @remarks Don't move - PATM compatibility.  */
#define VMCPU_FF_TIMER                      RT_BIT_32(2)
/** This action forces the VM to check any pending NMIs. */
#define VMCPU_FF_INTERRUPT_NMI_BIT          3
#define VMCPU_FF_INTERRUPT_NMI              RT_BIT_32(VMCPU_FF_INTERRUPT_NMI_BIT)
/** This action forces the VM to check any pending SMIs. */
#define VMCPU_FF_INTERRUPT_SMI_BIT          4
#define VMCPU_FF_INTERRUPT_SMI              RT_BIT_32(VMCPU_FF_INTERRUPT_SMI_BIT)
/** PDM critical section unlocking is pending, process promptly upon return to R3. */
#define VMCPU_FF_PDM_CRITSECT               RT_BIT_32(5)
/** Special EM internal force flag that is used by EMUnhaltAndWakeUp() to force
 * the virtual CPU out of the next (/current) halted state.  It is not processed
 * nor cleared by emR3ForcedActions (similar to VMCPU_FF_BLOCK_NMIS), instead it
 * is cleared the next time EM leaves the HALTED state. */
#define VMCPU_FF_UNHALT                     RT_BIT_32(6)
/** Pending IEM action (bit number). */
#define VMCPU_FF_IEM_BIT                    7
/** Pending IEM action (mask). */
#define VMCPU_FF_IEM                        RT_BIT_32(VMCPU_FF_IEM_BIT)
/** Pending APIC action (bit number). */
#define VMCPU_FF_UPDATE_APIC_BIT            8
/** This action forces the VM to update APIC's asynchronously arrived
 *  interrupts as pending interrupts. */
#define VMCPU_FF_UPDATE_APIC                RT_BIT_32(VMCPU_FF_UPDATE_APIC_BIT)
/** This action forces the VM to service pending requests from other
 * thread or requests which must be executed in another context. */
#define VMCPU_FF_REQUEST                    RT_BIT_32(9)
/** Pending DBGF event (alternative to passing VINF_EM_DBG_EVENT around).  */
#define VMCPU_FF_DBGF                       RT_BIT_32(VMCPU_FF_DBGF_BIT)
/** The bit number for VMCPU_FF_DBGF. */
#define VMCPU_FF_DBGF_BIT                   10
/** This action forces the VM to service any pending updates to CR3 (used only
 *  by HM). */
#define VMCPU_FF_HM_UPDATE_CR3              RT_BIT_32(12)
/** This action forces the VM to service any pending updates to PAE PDPEs (used
 *  only by HM). */
#define VMCPU_FF_HM_UPDATE_PAE_PDPES        RT_BIT_32(13)
/** This action forces the VM to resync the page tables before going
 * back to execute guest code. (GLOBAL FLUSH) */
#define VMCPU_FF_PGM_SYNC_CR3               RT_BIT_32(16)
/** Same as VM_FF_PGM_SYNC_CR3 except that global pages can be skipped.
 * (NON-GLOBAL FLUSH) */
#define VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL    RT_BIT_32(17)
/** Check for pending TLB shootdown actions (deprecated)
 * Reserved for furture HM re-use if necessary / safe.
 * Consumer: HM */
#define VMCPU_FF_TLB_SHOOTDOWN_UNUSED       RT_BIT_32(18)
/** Check for pending TLB flush action.
 * Consumer: HM
 * @todo rename to VMCPU_FF_HM_TLB_FLUSH  */
#define VMCPU_FF_TLB_FLUSH                  RT_BIT_32(VMCPU_FF_TLB_FLUSH_BIT)
/** The bit number for VMCPU_FF_TLB_FLUSH. */
#define VMCPU_FF_TLB_FLUSH_BIT              19
#ifdef VBOX_WITH_RAW_MODE
/** Check the interrupt and trap gates */
# define VMCPU_FF_TRPM_SYNC_IDT             RT_BIT_32(20)
/** Check Guest's TSS ring 0 stack */
# define VMCPU_FF_SELM_SYNC_TSS             RT_BIT_32(21)
/** Check Guest's GDT table */
# define VMCPU_FF_SELM_SYNC_GDT             RT_BIT_32(22)
/** Check Guest's LDT table */
# define VMCPU_FF_SELM_SYNC_LDT             RT_BIT_32(23)
#endif /* VBOX_WITH_RAW_MODE */
/** Inhibit interrupts pending. See EMGetInhibitInterruptsPC(). */
#define VMCPU_FF_INHIBIT_INTERRUPTS         RT_BIT_32(24)
/** Block injection of non-maskable interrupts to the guest. */
#define VMCPU_FF_BLOCK_NMIS                 RT_BIT_32(25)
#ifdef VBOX_WITH_RAW_MODE
/** CSAM needs to scan the page that's being executed */
# define VMCPU_FF_CSAM_SCAN_PAGE            RT_BIT_32(26)
/** CSAM needs to do some homework. */
# define VMCPU_FF_CSAM_PENDING_ACTION       RT_BIT_32(27)
#endif /* VBOX_WITH_RAW_MODE */
/** Force return to Ring-3. */
#define VMCPU_FF_TO_R3                      RT_BIT_32(28)
/** Force return to ring-3 to service pending I/O or MMIO write.
 * This is a backup for mechanism VINF_IOM_R3_IOPORT_COMMIT_WRITE and
 * VINF_IOM_R3_MMIO_COMMIT_WRITE, allowing VINF_EM_DBG_BREAKPOINT and similar
 * status codes to be propagated at the same time without loss. */
#define VMCPU_FF_IOM                        RT_BIT_32(29)
#ifdef VBOX_WITH_RAW_MODE
/** CPUM need to adjust CR0.TS/EM before executing raw-mode code again.  */
# define VMCPU_FF_CPUM                      RT_BIT_32(VMCPU_FF_CPUM_BIT)
/** The bit number for VMCPU_FF_CPUM. */
# define VMCPU_FF_CPUM_BIT                  30
#endif /* VBOX_WITH_RAW_MODE */
/** Hardware virtualized nested-guest interrupt pending. */
#define VMCPU_FF_INTERRUPT_NESTED_GUEST     RT_BIT_32(31)

/** Externally VM forced actions. Used to quit the idle/wait loop. */
#define VM_FF_EXTERNAL_SUSPENDED_MASK           (  VM_FF_CHECK_VM_STATE | VM_FF_DBGF | VM_FF_REQUEST | VM_FF_EMT_RENDEZVOUS )
/** Externally VMCPU forced actions. Used to quit the idle/wait loop. */
#define VMCPU_FF_EXTERNAL_SUSPENDED_MASK        (  VMCPU_FF_REQUEST  | VMCPU_FF_DBGF )

/** Externally forced VM actions. Used to quit the idle/wait loop. */
#define VM_FF_EXTERNAL_HALTED_MASK              (  VM_FF_CHECK_VM_STATE | VM_FF_DBGF    | VM_FF_REQUEST \
                                                 | VM_FF_PDM_QUEUES     | VM_FF_PDM_DMA | VM_FF_EMT_RENDEZVOUS )
/** Externally forced VMCPU actions. Used to quit the idle/wait loop. */
#define VMCPU_FF_EXTERNAL_HALTED_MASK           (  VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC \
                                                 | VMCPU_FF_REQUEST     | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI \
                                                 | VMCPU_FF_UNHALT      | VMCPU_FF_TIMER          | VMCPU_FF_DBGF )

/** High priority VM pre-execution actions. */
#define VM_FF_HIGH_PRIORITY_PRE_MASK            (  VM_FF_CHECK_VM_STATE | VM_FF_DBGF                 | VM_FF_TM_VIRTUAL_SYNC \
                                                 | VM_FF_DEBUG_SUSPEND  | VM_FF_PGM_NEED_HANDY_PAGES | VM_FF_PGM_NO_MEMORY \
                                                 | VM_FF_EMT_RENDEZVOUS )
/** High priority VMCPU pre-execution actions. */
#define VMCPU_FF_HIGH_PRIORITY_PRE_MASK         (  VMCPU_FF_TIMER        | VMCPU_FF_INTERRUPT_APIC     | VMCPU_FF_INTERRUPT_PIC \
                                                 | VMCPU_FF_UPDATE_APIC  | VMCPU_FF_INHIBIT_INTERRUPTS | VMCPU_FF_DBGF \
                                                 | VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL \
                                                 | VM_WHEN_RAW_MODE(  VMCPU_FF_SELM_SYNC_TSS | VMCPU_FF_TRPM_SYNC_IDT \
                                                                    | VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_LDT, 0 ) )

/** High priority VM pre raw-mode execution mask. */
#define VM_FF_HIGH_PRIORITY_PRE_RAW_MASK        (  VM_FF_PGM_NEED_HANDY_PAGES | VM_FF_PGM_NO_MEMORY )
/** High priority VMCPU pre raw-mode execution mask. */
#define VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK     (  VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL \
                                                 | VMCPU_FF_INHIBIT_INTERRUPTS \
                                                 | VM_WHEN_RAW_MODE(  VMCPU_FF_SELM_SYNC_TSS | VMCPU_FF_TRPM_SYNC_IDT \
                                                                    | VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_LDT, 0) )

/** High priority post-execution actions. */
#define VM_FF_HIGH_PRIORITY_POST_MASK           (  VM_FF_PGM_NO_MEMORY )
/** High priority post-execution actions. */
#define VMCPU_FF_HIGH_PRIORITY_POST_MASK        (  VMCPU_FF_PDM_CRITSECT  | VM_WHEN_RAW_MODE(VMCPU_FF_CSAM_PENDING_ACTION, 0) \
                                                 | VMCPU_FF_HM_UPDATE_CR3 | VMCPU_FF_HM_UPDATE_PAE_PDPES \
                                                 | VMCPU_FF_IEM           | VMCPU_FF_IOM )

/** Normal priority VM post-execution actions. */
#define VM_FF_NORMAL_PRIORITY_POST_MASK         (  VM_FF_CHECK_VM_STATE | VM_FF_DBGF | VM_FF_RESET \
                                                 | VM_FF_PGM_NO_MEMORY  | VM_FF_EMT_RENDEZVOUS)
/** Normal priority VMCPU post-execution actions. */
#define VMCPU_FF_NORMAL_PRIORITY_POST_MASK      ( VM_WHEN_RAW_MODE(VMCPU_FF_CSAM_SCAN_PAGE, 0) | VMCPU_FF_DBGF )

/** Normal priority VM actions. */
#define VM_FF_NORMAL_PRIORITY_MASK              (  VM_FF_REQUEST            | VM_FF_PDM_QUEUES | VM_FF_PDM_DMA \
                                                 | VM_FF_REM_HANDLER_NOTIFY | VM_FF_EMT_RENDEZVOUS)
/** Normal priority VMCPU actions. */
#define VMCPU_FF_NORMAL_PRIORITY_MASK           (  VMCPU_FF_REQUEST )

/** Flags to clear before resuming guest execution. */
#define VMCPU_FF_RESUME_GUEST_MASK              (  VMCPU_FF_TO_R3 )


/** VM flags that cause the REP[|NE|E] STRINS loops to yield immediately. */
#define VM_FF_HIGH_PRIORITY_POST_REPSTR_MASK    (  VM_FF_TM_VIRTUAL_SYNC | VM_FF_PGM_NEED_HANDY_PAGES   | VM_FF_PGM_NO_MEMORY \
                                                 | VM_FF_EMT_RENDEZVOUS  | VM_FF_PGM_POOL_FLUSH_PENDING | VM_FF_RESET)
/** VM flags that cause the REP[|NE|E] STRINS loops to yield. */
#define VM_FF_YIELD_REPSTR_MASK                 (  VM_FF_HIGH_PRIORITY_POST_REPSTR_MASK \
                                                 | VM_FF_PDM_QUEUES | VM_FF_PDM_DMA | VM_FF_DBGF | VM_FF_DEBUG_SUSPEND )
/** VMCPU flags that cause the REP[|NE|E] STRINS loops to yield immediately. */
#ifdef IN_RING3
# define VMCPU_FF_HIGH_PRIORITY_POST_REPSTR_MASK ( VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL | VMCPU_FF_DBGF )
#else
# define VMCPU_FF_HIGH_PRIORITY_POST_REPSTR_MASK (  VMCPU_FF_TO_R3 | VMCPU_FF_IEM | VMCPU_FF_IOM | VMCPU_FF_PGM_SYNC_CR3 \
                                                  | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL | VMCPU_FF_DBGF )
#endif
/** VMCPU flags that cause the REP[|NE|E] STRINS loops to yield, interrupts
 *  enabled. */
#define VMCPU_FF_YIELD_REPSTR_MASK              (  VMCPU_FF_HIGH_PRIORITY_POST_REPSTR_MASK \
                                                 | VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_UPDATE_APIC   | VMCPU_FF_INTERRUPT_PIC \
                                                 | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI | VMCPU_FF_PDM_CRITSECT \
                                                 | VMCPU_FF_TIMER          | VMCPU_FF_REQUEST )
/** VMCPU flags that cause the REP[|NE|E] STRINS loops to yield, interrupts
 *  disabled. */
#define VMCPU_FF_YIELD_REPSTR_NOINT_MASK        (  VMCPU_FF_YIELD_REPSTR_MASK \
                                                 & ~(VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_UPDATE_APIC   | VMCPU_FF_INTERRUPT_PIC) )

/** VM Flags that cause the HM loops to go back to ring-3. */
#define VM_FF_HM_TO_R3_MASK                     (  VM_FF_TM_VIRTUAL_SYNC | VM_FF_PGM_NEED_HANDY_PAGES | VM_FF_PGM_NO_MEMORY \
                                                 | VM_FF_PDM_QUEUES | VM_FF_EMT_RENDEZVOUS)
/** VMCPU Flags that cause the HM loops to go back to ring-3. */
#define VMCPU_FF_HM_TO_R3_MASK                  (  VMCPU_FF_TO_R3 | VMCPU_FF_TIMER | VMCPU_FF_PDM_CRITSECT \
                                                 | VMCPU_FF_IEM   | VMCPU_FF_IOM)

/** High priority ring-0 VM pre HM-mode execution mask. */
#define VM_FF_HP_R0_PRE_HM_MASK                 (VM_FF_HM_TO_R3_MASK | VM_FF_REQUEST | VM_FF_PGM_POOL_FLUSH_PENDING | VM_FF_PDM_DMA)
/** High priority ring-0 VMCPU pre HM-mode execution mask. */
#define VMCPU_FF_HP_R0_PRE_HM_MASK              (  VMCPU_FF_HM_TO_R3_MASK | VMCPU_FF_PGM_SYNC_CR3 \
                                                 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL | VMCPU_FF_REQUEST)
/** High priority ring-0 VM pre HM-mode execution mask, single stepping. */
#define VM_FF_HP_R0_PRE_HM_STEP_MASK            (VM_FF_HP_R0_PRE_HM_MASK & ~(  VM_FF_TM_VIRTUAL_SYNC | VM_FF_PDM_QUEUES  \
                                                                             | VM_FF_EMT_RENDEZVOUS | VM_FF_REQUEST \
                                                                             | VM_FF_PDM_DMA) )
/** High priority ring-0 VMCPU pre HM-mode execution mask, single stepping. */
#define VMCPU_FF_HP_R0_PRE_HM_STEP_MASK         (VMCPU_FF_HP_R0_PRE_HM_MASK & ~(  VMCPU_FF_TO_R3 | VMCPU_FF_TIMER \
                                                                                | VMCPU_FF_PDM_CRITSECT | VMCPU_FF_REQUEST) )

/** All the forced VM flags. */
#define VM_FF_ALL_MASK                          (UINT32_MAX)
/** All the forced VMCPU flags. */
#define VMCPU_FF_ALL_MASK                       (UINT32_MAX)

/** All the forced VM flags except those related to raw-mode and hardware
 * assisted execution. */
#define VM_FF_ALL_REM_MASK                      (~(VM_FF_HIGH_PRIORITY_PRE_RAW_MASK) | VM_FF_PGM_NEED_HANDY_PAGES | VM_FF_PGM_NO_MEMORY)
/** All the forced VMCPU flags except those related to raw-mode and hardware
 * assisted execution. */
#define VMCPU_FF_ALL_REM_MASK                   (~(  VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK | VMCPU_FF_PDM_CRITSECT \
                                                   | VMCPU_FF_TLB_FLUSH | VM_WHEN_RAW_MODE(VMCPU_FF_CSAM_PENDING_ACTION, 0) ))
/** @} */

/** @def VM_FF_SET
 * Sets a force action flag.
 *
 * @param   pVM     The cross context VM structure.
 * @param   fFlag   The flag to set.
 */
#if 1
# define VM_FF_SET(pVM, fFlag)              ASMAtomicOrU32(&(pVM)->fGlobalForcedActions, (fFlag))
#else
# define VM_FF_SET(pVM, fFlag) \
    do { ASMAtomicOrU32(&(pVM)->fGlobalForcedActions, (fFlag)); \
         RTLogPrintf("VM_FF_SET  : %08x %s - %s(%d) %s\n", (pVM)->fGlobalForcedActions, #fFlag, __FILE__, __LINE__, __FUNCTION__); \
    } while (0)
#endif

/** @def VMCPU_FF_SET
 * Sets a force action flag for the given VCPU.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to set.
 */
#define VMCPU_FF_SET(pVCpu, fFlag)          ASMAtomicOrU32(&(pVCpu)->fLocalForcedActions, (fFlag))

/** @def VM_FF_CLEAR
 * Clears a force action flag.
 *
 * @param   pVM     The cross context VM structure.
 * @param   fFlag   The flag to clear.
 */
#if 1
# define VM_FF_CLEAR(pVM, fFlag)            ASMAtomicAndU32(&(pVM)->fGlobalForcedActions, ~(fFlag))
#else
# define VM_FF_CLEAR(pVM, fFlag) \
    do { ASMAtomicAndU32(&(pVM)->fGlobalForcedActions, ~(fFlag)); \
         RTLogPrintf("VM_FF_CLEAR: %08x %s - %s(%d) %s\n", (pVM)->fGlobalForcedActions, #fFlag, __FILE__, __LINE__, __FUNCTION__); \
    } while (0)
#endif

/** @def VMCPU_FF_CLEAR
 * Clears a force action flag for the given VCPU.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to clear.
 */
#define VMCPU_FF_CLEAR(pVCpu, fFlag)        ASMAtomicAndU32(&(pVCpu)->fLocalForcedActions, ~(fFlag))

/** @def VM_FF_IS_SET
 * Checks if a force action flag is set.
 *
 * @param   pVM     The cross context VM structure.
 * @param   fFlag   The flag to check.
 */
#define VM_FF_IS_SET(pVM, fFlag)            (((pVM)->fGlobalForcedActions & (fFlag)) == (fFlag))

/** @def VMCPU_FF_IS_SET
 * Checks if a force action flag is set for the given VCPU.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to check.
 */
#define VMCPU_FF_IS_SET(pVCpu, fFlag)       (((pVCpu)->fLocalForcedActions & (fFlag)) == (fFlag))

/** @def VM_FF_IS_PENDING
 * Checks if one or more force action in the specified set is pending.
 *
 * @param   pVM     The cross context VM structure.
 * @param   fFlags  The flags to check for.
 */
#define VM_FF_IS_PENDING(pVM, fFlags)       RT_BOOL((pVM)->fGlobalForcedActions & (fFlags))

/** @def VM_FF_TEST_AND_CLEAR
 * Checks if one (!) force action in the specified set is pending and clears it atomically
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 * @param   pVM     The cross context VM structure.
 * @param   iBit    Bit position to check and clear
 */
#define VM_FF_TEST_AND_CLEAR(pVM, iBit)     (ASMAtomicBitTestAndClear(&(pVM)->fGlobalForcedActions, iBit##_BIT))

/** @def VMCPU_FF_TEST_AND_CLEAR
 * Checks if one (!) force action in the specified set is pending and clears it atomically
 *
 * @returns true if the bit was set.
 * @returns false if the bit was clear.
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   iBit    Bit position to check and clear
 */
#define VMCPU_FF_TEST_AND_CLEAR(pVCpu, iBit) (ASMAtomicBitTestAndClear(&(pVCpu)->fLocalForcedActions, iBit##_BIT))

/** @def VMCPU_FF_IS_PENDING
 * Checks if one or more force action in the specified set is pending for the given VCPU.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The flags to check for.
 */
#define VMCPU_FF_IS_PENDING(pVCpu, fFlags)  RT_BOOL((pVCpu)->fLocalForcedActions & (fFlags))

/** @def VM_FF_IS_PENDING_EXCEPT
 * Checks if one or more force action in the specified set is pending while one
 * or more other ones are not.
 *
 * @param   pVM     The cross context VM structure.
 * @param   fFlags  The flags to check for.
 * @param   fExcpt  The flags that should not be set.
 */
#define VM_FF_IS_PENDING_EXCEPT(pVM, fFlags, fExcpt)            ( ((pVM)->fGlobalForcedActions & (fFlags)) && !((pVM)->fGlobalForcedActions & (fExcpt)) )

/** @def VMCPU_FF_IS_PENDING_EXCEPT
 * Checks if one or more force action in the specified set is pending for the given
 * VCPU while one or more other ones are not.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The flags to check for.
 * @param   fExcpt  The flags that should not be set.
 */
#define VMCPU_FF_IS_PENDING_EXCEPT(pVCpu, fFlags, fExcpt) ( ((pVCpu)->fLocalForcedActions & (fFlags)) && !((pVCpu)->fLocalForcedActions & (fExcpt)) )

/** @def VM_IS_EMT
 * Checks if the current thread is the emulation thread (EMT).
 *
 * @remark  The ring-0 variation will need attention if we expand the ring-0
 *          code to let threads other than EMT mess around with the VM.
 */
#ifdef IN_RC
# define VM_IS_EMT(pVM)                     true
#else
# define VM_IS_EMT(pVM)                     (VMMGetCpu(pVM) != NULL)
#endif

/** @def VMCPU_IS_EMT
 * Checks if the current thread is the emulation thread (EMT) for the specified
 * virtual CPU.
 */
#ifdef IN_RC
# define VMCPU_IS_EMT(pVCpu)                true
#else
# define VMCPU_IS_EMT(pVCpu)                ((pVCpu) && ((pVCpu) == VMMGetCpu((pVCpu)->CTX_SUFF(pVM))))
#endif

/** @def VM_ASSERT_EMT
 * Asserts that the current thread IS the emulation thread (EMT).
 */
#ifdef IN_RC
# define VM_ASSERT_EMT(pVM)                 Assert(VM_IS_EMT(pVM))
#elif defined(IN_RING0)
# define VM_ASSERT_EMT(pVM)                 Assert(VM_IS_EMT(pVM))
#else
# define VM_ASSERT_EMT(pVM) \
    AssertMsg(VM_IS_EMT(pVM), \
        ("Not emulation thread! Thread=%RTnthrd ThreadEMT=%RTnthrd\n", RTThreadNativeSelf(), VMR3GetVMCPUNativeThread(pVM)))
#endif

/** @def VMCPU_ASSERT_EMT
 * Asserts that the current thread IS the emulation thread (EMT) of the
 * specified virtual CPU.
 */
#ifdef IN_RC
# define VMCPU_ASSERT_EMT(pVCpu)            Assert(VMCPU_IS_EMT(pVCpu))
#elif defined(IN_RING0)
# define VMCPU_ASSERT_EMT(pVCpu)            AssertMsg(VMCPU_IS_EMT(pVCpu), \
                                                      ("Not emulation thread! Thread=%RTnthrd ThreadEMT=%RTnthrd idCpu=%u\n", \
                                                      RTThreadNativeSelf(), (pVCpu) ? (pVCpu)->hNativeThreadR0 : 0, \
                                                      (pVCpu) ? (pVCpu)->idCpu : 0))
#else
# define VMCPU_ASSERT_EMT(pVCpu)            AssertMsg(VMCPU_IS_EMT(pVCpu), \
                                                      ("Not emulation thread! Thread=%RTnthrd ThreadEMT=%RTnthrd idCpu=%#x\n", \
                                                      RTThreadNativeSelf(), (pVCpu)->hNativeThread, (pVCpu)->idCpu))
#endif

/** @def VM_ASSERT_EMT_RETURN
 * Asserts that the current thread IS the emulation thread (EMT) and returns if it isn't.
 */
#ifdef IN_RC
# define VM_ASSERT_EMT_RETURN(pVM, rc)      AssertReturn(VM_IS_EMT(pVM), (rc))
#elif defined(IN_RING0)
# define VM_ASSERT_EMT_RETURN(pVM, rc)      AssertReturn(VM_IS_EMT(pVM), (rc))
#else
# define VM_ASSERT_EMT_RETURN(pVM, rc) \
    AssertMsgReturn(VM_IS_EMT(pVM), \
        ("Not emulation thread! Thread=%RTnthrd ThreadEMT=%RTnthrd\n", RTThreadNativeSelf(), VMR3GetVMCPUNativeThread(pVM)), \
        (rc))
#endif

/** @def VMCPU_ASSERT_EMT_RETURN
 * Asserts that the current thread IS the emulation thread (EMT) and returns if it isn't.
 */
#ifdef IN_RC
# define VMCPU_ASSERT_EMT_RETURN(pVCpu, rc) AssertReturn(VMCPU_IS_EMT(pVCpu), (rc))
#elif defined(IN_RING0)
# define VMCPU_ASSERT_EMT_RETURN(pVCpu, rc) AssertReturn(VMCPU_IS_EMT(pVCpu), (rc))
#else
# define VMCPU_ASSERT_EMT_RETURN(pVCpu, rc) \
    AssertMsgReturn(VMCPU_IS_EMT(pVCpu), \
                    ("Not emulation thread! Thread=%RTnthrd ThreadEMT=%RTnthrd idCpu=%#x\n", \
                     RTThreadNativeSelf(), (pVCpu)->hNativeThread, (pVCpu)->idCpu), \
                    (rc))
#endif

/** @def VMCPU_ASSERT_EMT_OR_GURU
 * Asserts that the current thread IS the emulation thread (EMT) of the
 * specified virtual CPU.
 */
#if defined(IN_RC) || defined(IN_RING0)
# define VMCPU_ASSERT_EMT_OR_GURU(pVCpu)    Assert(   VMCPU_IS_EMT(pVCpu) \
                                                   || pVCpu->CTX_SUFF(pVM)->enmVMState == VMSTATE_GURU_MEDITATION \
                                                   || pVCpu->CTX_SUFF(pVM)->enmVMState == VMSTATE_GURU_MEDITATION_LS )
#else
# define VMCPU_ASSERT_EMT_OR_GURU(pVCpu) \
    AssertMsg(   VMCPU_IS_EMT(pVCpu) \
              || pVCpu->CTX_SUFF(pVM)->enmVMState == VMSTATE_GURU_MEDITATION \
              || pVCpu->CTX_SUFF(pVM)->enmVMState == VMSTATE_GURU_MEDITATION_LS, \
              ("Not emulation thread! Thread=%RTnthrd ThreadEMT=%RTnthrd idCpu=%#x\n", \
               RTThreadNativeSelf(), (pVCpu)->hNativeThread, (pVCpu)->idCpu))
#endif

/** @def VMCPU_ASSERT_EMT_OR_NOT_RUNNING
 * Asserts that the current thread IS the emulation thread (EMT) of the
 * specified virtual CPU or the VM is not running.
 */
#if defined(IN_RC) || defined(IN_RING0)
# define VMCPU_ASSERT_EMT_OR_NOT_RUNNING(pVCpu) \
    Assert(    VMCPU_IS_EMT(pVCpu) \
           || !VM_IS_RUNNING_FOR_ASSERTIONS_ONLY((pVCpu)->CTX_SUFF(pVM)) )
#else
# define VMCPU_ASSERT_EMT_OR_NOT_RUNNING(pVCpu) \
    AssertMsg(    VMCPU_IS_EMT(pVCpu) \
              || !VM_IS_RUNNING_FOR_ASSERTIONS_ONLY((pVCpu)->CTX_SUFF(pVM)), \
              ("Not emulation thread! Thread=%RTnthrd ThreadEMT=%RTnthrd idCpu=%#x\n", \
               RTThreadNativeSelf(), (pVCpu)->hNativeThread, (pVCpu)->idCpu))
#endif

/** @def VM_IS_RUNNING_FOR_ASSERTIONS_ONLY
 * Checks if the the VM is running.
 * @note Thie is only for pure debug assertions.  No AssertReturn or similar!
 */
#define VM_IS_RUNNING_FOR_ASSERTIONS_ONLY(pVM) \
    (   (pVM)->enmVMState == VMSTATE_RUNNING \
     || (pVM)->enmVMState == VMSTATE_RUNNING_LS \
     || (pVM)->enmVMState == VMSTATE_RUNNING_FT )

/** @def VM_ASSERT_IS_NOT_RUNNING
 * Asserts that the VM is not running.
 */
#if defined(IN_RC) || defined(IN_RING0)
#define VM_ASSERT_IS_NOT_RUNNING(pVM)       Assert(!VM_IS_RUNNING_FOR_ASSERTIONS_ONLY(pVM))
#else
#define VM_ASSERT_IS_NOT_RUNNING(pVM)       AssertMsg(!VM_IS_RUNNING_FOR_ASSERTIONS_ONLY(pVM), \
                                                      ("VM is running. enmVMState=%d\n", (pVM)->enmVMState))
#endif

/** @def VM_ASSERT_EMT0
 * Asserts that the current thread IS emulation thread \#0 (EMT0).
 */
#define VM_ASSERT_EMT0(pVM)                 VMCPU_ASSERT_EMT(&(pVM)->aCpus[0])

/** @def VM_ASSERT_EMT0_RETURN
 * Asserts that the current thread IS emulation thread \#0 (EMT0) and returns if
 * it isn't.
 */
#define VM_ASSERT_EMT0_RETURN(pVM, rc)      VMCPU_ASSERT_EMT_RETURN(&(pVM)->aCpus[0], (rc))


/**
 * Asserts that the current thread is NOT the emulation thread.
 */
#define VM_ASSERT_OTHER_THREAD(pVM) \
    AssertMsg(!VM_IS_EMT(pVM), ("Not other thread!!\n"))


/** @def VM_ASSERT_STATE
 * Asserts a certain VM state.
 */
#define VM_ASSERT_STATE(pVM, _enmState) \
        AssertMsg((pVM)->enmVMState == (_enmState), \
                  ("state %s, expected %s\n", VMGetStateName((pVM)->enmVMState), VMGetStateName(_enmState)))

/** @def VM_ASSERT_STATE_RETURN
 * Asserts a certain VM state and returns if it doesn't match.
 */
#define VM_ASSERT_STATE_RETURN(pVM, _enmState, rc) \
        AssertMsgReturn((pVM)->enmVMState == (_enmState), \
                        ("state %s, expected %s\n", VMGetStateName((pVM)->enmVMState), VMGetStateName(_enmState)), \
                        (rc))

/** @def VM_IS_VALID_EXT
 * Asserts a the VM handle is valid for external access, i.e. not being destroy
 * or terminated. */
#define VM_IS_VALID_EXT(pVM) \
        (    RT_VALID_ALIGNED_PTR(pVM, PAGE_SIZE) \
         &&  (   (unsigned)(pVM)->enmVMState < (unsigned)VMSTATE_DESTROYING \
              || (   (unsigned)(pVM)->enmVMState == (unsigned)VMSTATE_DESTROYING \
                  && VM_IS_EMT(pVM))) )

/** @def VM_ASSERT_VALID_EXT_RETURN
 * Asserts a the VM handle is valid for external access, i.e. not being
 * destroy or terminated.
 */
#define VM_ASSERT_VALID_EXT_RETURN(pVM, rc) \
        AssertMsgReturn(VM_IS_VALID_EXT(pVM), \
                        ("pVM=%p state %s\n", (pVM), RT_VALID_ALIGNED_PTR(pVM, PAGE_SIZE) \
                         ? VMGetStateName(pVM->enmVMState) : ""), \
                        (rc))

/** @def VMCPU_ASSERT_VALID_EXT_RETURN
 * Asserts a the VMCPU handle is valid for external access, i.e. not being
 * destroy or terminated.
 */
#define VMCPU_ASSERT_VALID_EXT_RETURN(pVCpu, rc) \
        AssertMsgReturn(    RT_VALID_ALIGNED_PTR(pVCpu, 64) \
                        &&  RT_VALID_ALIGNED_PTR((pVCpu)->CTX_SUFF(pVM), PAGE_SIZE) \
                        &&  (unsigned)(pVCpu)->CTX_SUFF(pVM)->enmVMState < (unsigned)VMSTATE_DESTROYING, \
                        ("pVCpu=%p pVM=%p state %s\n", (pVCpu), RT_VALID_ALIGNED_PTR(pVCpu, 64) ? (pVCpu)->CTX_SUFF(pVM) : NULL, \
                         RT_VALID_ALIGNED_PTR(pVCpu, 64) && RT_VALID_ALIGNED_PTR((pVCpu)->CTX_SUFF(pVM), PAGE_SIZE) \
                         ? VMGetStateName((pVCpu)->pVMR3->enmVMState) : ""), \
                        (rc))

#endif /* !VBOX_FOR_DTRACE_LIB */



/**
 * The cross context VM structure.
 *
 * It contains all the VM data which have to be available in all contexts.
 * Even if it contains all the data the idea is to use APIs not to modify all
 * the members all around the place.  Therefore we make use of unions to hide
 * everything which isn't local to the current source module.  This means we'll
 * have to pay a little bit of attention when adding new members to structures
 * in the unions and make sure to keep the padding sizes up to date.
 *
 * Run 'kmk run-struct-tests' (from src/VBox/VMM if you like) after updating!
 */
typedef struct VM
{
    /** The state of the VM.
     * This field is read only to everyone except the VM and EM. */
    VMSTATE volatile            enmVMState;
    /** Forced action flags.
     * See the VM_FF_* \#defines. Updated atomically.
     */
    volatile uint32_t           fGlobalForcedActions;
    /** Pointer to the array of page descriptors for the VM structure allocation. */
    R3PTRTYPE(PSUPPAGE)         paVMPagesR3;
    /** Session handle. For use when calling SUPR0 APIs. */
    PSUPDRVSESSION              pSession;
    /** Pointer to the ring-3 VM structure. */
    PUVM                        pUVM;
    /** Ring-3 Host Context VM Pointer. */
    R3PTRTYPE(struct VM *)      pVMR3;
    /** Ring-0 Host Context VM Pointer. */
    R0PTRTYPE(struct VM *)      pVMR0;
    /** Raw-mode Context VM Pointer. */
    RCPTRTYPE(struct VM *)      pVMRC;

    /** The GVM VM handle. Only the GVM should modify this field. */
    uint32_t                    hSelf;
    /** Number of virtual CPUs. */
    uint32_t                    cCpus;
    /** CPU excution cap (1-100) */
    uint32_t                    uCpuExecutionCap;

    /** Size of the VM structure including the VMCPU array. */
    uint32_t                    cbSelf;

    /** Offset to the VMCPU array starting from beginning of this structure. */
    uint32_t                    offVMCPU;

    /**
     * VMMSwitcher assembly entry point returning to host context.
     *
     * Depending on how the host handles the rc status given in @a eax, this may
     * return and let the caller resume whatever it was doing prior to the call.
     *
     *
     * @param   eax         The return code, register.
     * @remark  Assume interrupts disabled.
     * @remark  This method pointer lives here because TRPM needs it.
     */
    RTRCPTR                     pfnVMMRCToHostAsm/*(int32_t eax)*/;

    /**
     * VMMSwitcher assembly entry point returning to host context without saving the
     * raw-mode context (hyper) registers.
     *
     * Unlike pfnVMMRC2HCAsm, this will not return to the caller.  Instead it
     * expects the caller to save a RC context in CPUM where one might return if the
     * return code indicate that this is possible.
     *
     * This method pointer lives here because TRPM needs it.
     *
     * @param   eax         The return code, register.
     * @remark  Assume interrupts disabled.
     * @remark  This method pointer lives here because TRPM needs it.
     */
    RTRCPTR                     pfnVMMRCToHostAsmNoReturn/*(int32_t eax)*/;

    /** @name Various items that are frequently accessed.
     * @{ */
    /** Whether to recompile user mode code or run it raw/hm. */
    bool                        fRecompileUser;
    /** Whether to recompile supervisor mode code or run it raw/hm. */
    bool                        fRecompileSupervisor;
    /** Whether raw mode supports ring-1 code or not. */
    bool                        fRawRing1Enabled;
    /** PATM enabled flag.
     * This is placed here for performance reasons. */
    bool                        fPATMEnabled;
    /** CSAM enabled flag.
     * This is placed here for performance reasons. */
    bool                        fCSAMEnabled;
    /** Hardware VM support is available and enabled.
     * Determined very early during init.
     * This is placed here for performance reasons. */
    bool                        fHMEnabled;
    /** For asserting on fHMEnable usage. */
    bool                        fHMEnabledFixed;
    /** Hardware VM support requires a minimal raw-mode context.
     * This is never set on 64-bit hosts, only 32-bit hosts requires it. */
    bool                        fHMNeedRawModeCtx;
    /** Set when this VM is the master FT node.
     * @todo This doesn't need to be here, FTM should store it in it's own
     *       structures instead. */
    bool                        fFaultTolerantMaster;
    /** Large page enabled flag.
     * @todo This doesn't need to be here, PGM should store it in it's own
     *       structures instead. */
    bool                        fUseLargePages;
    /** @} */

    /** Alignment padding. */
    uint8_t                     uPadding1[2];

    /** @name Debugging
     * @{ */
    /** Raw-mode Context VM Pointer. */
    RCPTRTYPE(RTTRACEBUF)       hTraceBufRC;
    /** Ring-3 Host Context VM Pointer. */
    R3PTRTYPE(RTTRACEBUF)       hTraceBufR3;
    /** Ring-0 Host Context VM Pointer. */
    R0PTRTYPE(RTTRACEBUF)       hTraceBufR0;
    /** @} */

#if HC_ARCH_BITS == 32
    /** Alignment padding. */
    uint32_t                    uPadding2;
#endif

    /** @name Switcher statistics (remove)
     * @{ */
    /** Profiling the total time from Qemu to GC. */
    STAMPROFILEADV              StatTotalQemuToGC;
    /** Profiling the total time from GC to Qemu. */
    STAMPROFILEADV              StatTotalGCToQemu;
    /** Profiling the total time spent in GC. */
    STAMPROFILEADV              StatTotalInGC;
    /** Profiling the total time spent not in Qemu. */
    STAMPROFILEADV              StatTotalInQemu;
    /** Profiling the VMMSwitcher code for going to GC. */
    STAMPROFILEADV              StatSwitcherToGC;
    /** Profiling the VMMSwitcher code for going to HC. */
    STAMPROFILEADV              StatSwitcherToHC;
    STAMPROFILEADV              StatSwitcherSaveRegs;
    STAMPROFILEADV              StatSwitcherSysEnter;
    STAMPROFILEADV              StatSwitcherDebug;
    STAMPROFILEADV              StatSwitcherCR0;
    STAMPROFILEADV              StatSwitcherCR4;
    STAMPROFILEADV              StatSwitcherJmpCR3;
    STAMPROFILEADV              StatSwitcherRstrRegs;
    STAMPROFILEADV              StatSwitcherLgdt;
    STAMPROFILEADV              StatSwitcherLidt;
    STAMPROFILEADV              StatSwitcherLldt;
    STAMPROFILEADV              StatSwitcherTSS;
    /** @} */

    /** Padding - the unions must be aligned on a 64 bytes boundary and the unions
     *  must start at the same offset on both 64-bit and 32-bit hosts. */
    uint8_t                     abAlignment3[(HC_ARCH_BITS == 32 ? 24 : 0) + 40];

    /** CPUM part. */
    union
    {
#ifdef ___CPUMInternal_h
        struct CPUM s;
#endif
#ifdef ___VBox_vmm_cpum_h
        /** Read only info exposed about the host and guest CPUs. */
        struct
        {
            /** Padding for hidden fields. */
            uint8_t                 abHidden0[64];
            /** Host CPU feature information. */
            CPUMFEATURES            HostFeatures;
            /** Guest CPU feature information. */
            CPUMFEATURES            GuestFeatures;
        } const ro;
#endif
        uint8_t     padding[1536];      /* multiple of 64 */
    } cpum;

    /** VMM part. */
    union
    {
#ifdef ___VMMInternal_h
        struct VMM  s;
#endif
        uint8_t     padding[1600];      /* multiple of 64 */
    } vmm;

    /** PGM part. */
    union
    {
#ifdef ___PGMInternal_h
        struct PGM  s;
#endif
        uint8_t     padding[4096*2+6080];      /* multiple of 64 */
    } pgm;

    /** HM part. */
    union
    {
#ifdef ___HMInternal_h
        struct HM s;
#endif
        uint8_t     padding[5440];      /* multiple of 64 */
    } hm;

    /** TRPM part. */
    union
    {
#ifdef ___TRPMInternal_h
        struct TRPM s;
#endif
        uint8_t     padding[5248];      /* multiple of 64 */
    } trpm;

    /** SELM part. */
    union
    {
#ifdef ___SELMInternal_h
        struct SELM s;
#endif
        uint8_t     padding[768];       /* multiple of 64 */
    } selm;

    /** MM part. */
    union
    {
#ifdef ___MMInternal_h
        struct MM   s;
#endif
        uint8_t     padding[192];       /* multiple of 64 */
    } mm;

    /** PDM part. */
    union
    {
#ifdef ___PDMInternal_h
        struct PDM s;
#endif
        uint8_t     padding[1920];      /* multiple of 64 */
    } pdm;

    /** IOM part. */
    union
    {
#ifdef ___IOMInternal_h
        struct IOM s;
#endif
        uint8_t     padding[896];       /* multiple of 64 */
    } iom;

    /** EM part. */
    union
    {
#ifdef ___EMInternal_h
        struct EM   s;
#endif
        uint8_t     padding[256];       /* multiple of 64 */
    } em;

    /** TM part. */
    union
    {
#ifdef ___TMInternal_h
        struct TM   s;
#endif
        uint8_t     padding[2496];      /* multiple of 64 */
    } tm;

    /** DBGF part. */
    union
    {
#ifdef ___DBGFInternal_h
        struct DBGF s;
#endif
#ifdef ___VBox_vmm_dbgf_h
        /** Read only info exposed about interrupt breakpoints and selected events. */
        struct
        {
            /** Bitmap of enabled hardware interrupt breakpoints. */
            uint32_t                    bmHardIntBreakpoints[256 / 32];
            /** Bitmap of enabled software interrupt breakpoints. */
            uint32_t                    bmSoftIntBreakpoints[256 / 32];
            /** Bitmap of selected events.
             * This includes non-selectable events too for simplicity, we maintain the
             * state for some of these, as it may come in handy. */
            uint64_t                    bmSelectedEvents[(DBGFEVENT_END + 63) / 64];
            /** Enabled hardware interrupt breakpoints. */
            uint32_t                    cHardIntBreakpoints;
            /** Enabled software interrupt breakpoints. */
            uint32_t                    cSoftIntBreakpoints;
            /** The number of selected events. */
            uint32_t                    cSelectedEvents;
            /** The number of enabled hardware breakpoints. */
            uint8_t                     cEnabledHwBreakpoints;
            /** The number of enabled hardware I/O breakpoints. */
            uint8_t                     cEnabledHwIoBreakpoints;
            /** The number of enabled INT3 breakpoints. */
            uint8_t                     cEnabledInt3Breakpoints;
            uint8_t                     abPadding[1]; /**< Unused padding space up for grabs. */
        } const     ro;
#endif
        uint8_t     padding[2368];      /* multiple of 64 */
    } dbgf;

    /** SSM part. */
    union
    {
#ifdef ___SSMInternal_h
        struct SSM  s;
#endif
        uint8_t     padding[128];       /* multiple of 64 */
    } ssm;

    /** FTM part. */
    union
    {
#ifdef ___FTMInternal_h
        struct FTM  s;
#endif
        uint8_t     padding[512];       /* multiple of 64 */
    } ftm;

#ifdef VBOX_WITH_RAW_MODE
    /** PATM part. */
    union
    {
# ifdef ___PATMInternal_h
        struct PATM s;
# endif
        uint8_t     padding[768];       /* multiple of 64 */
    } patm;

    /** CSAM part. */
    union
    {
# ifdef ___CSAMInternal_h
        struct CSAM s;
# endif
        uint8_t     padding[1088];      /* multiple of 64 */
    } csam;
#endif

#ifdef VBOX_WITH_REM
    /** REM part. */
    union
    {
# ifdef ___REMInternal_h
        struct REM  s;
# endif
        uint8_t     padding[0x11100];   /* multiple of 64 */
    } rem;
#endif

    union
    {
#ifdef ___GIMInternal_h
        struct GIM s;
#endif
        uint8_t     padding[448];       /* multiple of 64 */
    } gim;

    union
    {
#ifdef ___APICInternal_h
        struct APIC s;
#endif
        uint8_t     padding[128];       /* multiple of 8 */
    } apic;

    /* ---- begin small stuff ---- */

    /** VM part. */
    union
    {
#ifdef ___VMInternal_h
        struct VMINT s;
#endif
        uint8_t     padding[24];        /* multiple of 8 */
    } vm;

    /** CFGM part. */
    union
    {
#ifdef ___CFGMInternal_h
        struct CFGM s;
#endif
        uint8_t     padding[8];         /* multiple of 8 */
    } cfgm;

    /** Padding for aligning the cpu array on a page boundary. */
#if defined(VBOX_WITH_REM) && defined(VBOX_WITH_RAW_MODE)
    uint8_t         abAlignment2[3870];
#elif defined(VBOX_WITH_REM) && !defined(VBOX_WITH_RAW_MODE)
    uint8_t         abAlignment2[1630];
#elif !defined(VBOX_WITH_REM) && defined(VBOX_WITH_RAW_MODE)
    uint8_t         abAlignment2[30];
#else
    uint8_t         abAlignment2[1886];
#endif

    /* ---- end small stuff ---- */

    /** VMCPU array for the configured number of virtual CPUs.
     * Must be aligned on a page boundary for TLB hit reasons as well as
     * alignment of VMCPU members. */
    VMCPU           aCpus[1];
} VM;


#ifdef IN_RC
RT_C_DECLS_BEGIN

/** The VM structure.
 * This is imported from the VMMRCBuiltin module, i.e. it's a one of those magic
 * globals which we should avoid using.
 */
extern DECLIMPORT(VM)   g_VM;

RT_C_DECLS_END
#endif

/** @} */

#endif


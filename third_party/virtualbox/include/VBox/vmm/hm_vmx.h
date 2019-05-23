/** @file
 * HM - VMX Structures and Definitions. (VMM)
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

#ifndef ___VBox_vmm_vmx_h
#define ___VBox_vmm_vmx_h

#include <VBox/types.h>
#include <VBox/err.h>
#include <iprt/x86.h>
#include <iprt/assert.h>

/* In Visual C++ versions prior to 2012, the vmx intrinsics are only available
   when targeting AMD64. */
#if RT_INLINE_ASM_USES_INTRIN >= 16 && defined(RT_ARCH_AMD64)
# pragma warning(push)
# pragma warning(disable:4668) /* Several incorrect __cplusplus uses. */
# pragma warning(disable:4255) /* Incorrect __slwpcb prototype. */
# include <intrin.h>
# pragma warning(pop)
/* We always want them as intrinsics, no functions. */
# pragma intrinsic(__vmx_on)
# pragma intrinsic(__vmx_off)
# pragma intrinsic(__vmx_vmclear)
# pragma intrinsic(__vmx_vmptrld)
# pragma intrinsic(__vmx_vmread)
# pragma intrinsic(__vmx_vmwrite)
# define VMX_USE_MSC_INTRINSICS 1
#else
# define VMX_USE_MSC_INTRINSICS 0
#endif


/** @defgroup grp_hm_vmx    VMX Types and Definitions
 * @ingroup grp_hm
 * @{
 */

/** @def HMVMXCPU_GST_SET_UPDATED
 * Sets a guest-state-updated flag.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to set.
 */
#define HMVMXCPU_GST_SET_UPDATED(pVCpu, fFlag)        (ASMAtomicUoOrU32(&(pVCpu)->hm.s.vmx.fUpdatedGuestState, (fFlag)))

/** @def HMVMXCPU_GST_IS_SET
 * Checks if all the flags in the specified guest-state-updated set is pending.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to check.
 */
#define HMVMXCPU_GST_IS_SET(pVCpu, fFlag)             ((ASMAtomicUoReadU32(&(pVCpu)->hm.s.vmx.fUpdatedGuestState) & (fFlag)) == (fFlag))

/** @def HMVMXCPU_GST_IS_UPDATED
 * Checks if one or more of the flags in the specified guest-state-updated set
 * is updated.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The flags to check for.
 */
#define HMVMXCPU_GST_IS_UPDATED(pVCpu, fFlags)        RT_BOOL(ASMAtomicUoReadU32(&(pVCpu)->hm.s.vmx.fUpdatedGuestState) & (fFlags))

/** @def HMVMXCPU_GST_RESET_TO
 * Resets the guest-state-updated flags to the specified value.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The new value.
 */
#define HMVMXCPU_GST_RESET_TO(pVCpu, fFlags)          (ASMAtomicUoWriteU32(&(pVCpu)->hm.s.vmx.fUpdatedGuestState, (fFlags)))

/** @def HMVMXCPU_GST_VALUE
 * Returns the current guest-state-updated flags value.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 */
#define HMVMXCPU_GST_VALUE(pVCpu)                     (ASMAtomicUoReadU32(&(pVCpu)->hm.s.vmx.fUpdatedGuestState))

/** @name Host-state restoration flags.
 * @note If you change these values don't forget to update the assembly
 *       defines as well!
 * @{
 */
#define VMX_RESTORE_HOST_SEL_DS               RT_BIT(0)
#define VMX_RESTORE_HOST_SEL_ES               RT_BIT(1)
#define VMX_RESTORE_HOST_SEL_FS               RT_BIT(2)
#define VMX_RESTORE_HOST_SEL_GS               RT_BIT(3)
#define VMX_RESTORE_HOST_SEL_TR               RT_BIT(4)
#define VMX_RESTORE_HOST_GDTR                 RT_BIT(5)
#define VMX_RESTORE_HOST_IDTR                 RT_BIT(6)
#define VMX_RESTORE_HOST_GDT_READ_ONLY        RT_BIT(7)
#define VMX_RESTORE_HOST_REQUIRED             RT_BIT(8)
#define VMX_RESTORE_HOST_GDT_NEED_WRITABLE    RT_BIT(9)
/** @} */

/**
 * Host-state restoration structure.
 * This holds host-state fields that require manual restoration.
 * Assembly version found in hm_vmx.mac (should be automatically verified).
 */
typedef struct VMXRESTOREHOST
{
    RTSEL       uHostSelDS;     /* 0x00 */
    RTSEL       uHostSelES;     /* 0x02 */
    RTSEL       uHostSelFS;     /* 0x04 */
    RTSEL       uHostSelGS;     /* 0x06 */
    RTSEL       uHostSelTR;     /* 0x08 */
    uint8_t     abPadding0[4];
    X86XDTR64   HostGdtr;       /**< 0x0e - should be aligned by it's 64-bit member. */
    uint8_t     abPadding1[6];
    X86XDTR64   HostGdtrRw;     /**< 0x1e - should be aligned by it's 64-bit member. */
    uint8_t     abPadding2[6];
    X86XDTR64   HostIdtr;       /**< 0x2e - should be aligned by it's 64-bit member. */
    uint64_t    uHostFSBase;    /* 0x38 */
    uint64_t    uHostGSBase;    /* 0x40 */
} VMXRESTOREHOST;
/** Pointer to VMXRESTOREHOST. */
typedef VMXRESTOREHOST *PVMXRESTOREHOST;
AssertCompileSize(X86XDTR64, 10);
AssertCompileMemberOffset(VMXRESTOREHOST, HostGdtr.uAddr, 16);
AssertCompileMemberOffset(VMXRESTOREHOST, HostGdtrRw.uAddr, 32);
AssertCompileMemberOffset(VMXRESTOREHOST, HostIdtr.uAddr, 48);
AssertCompileMemberOffset(VMXRESTOREHOST, uHostFSBase,    56);
AssertCompileSize(VMXRESTOREHOST, 72);
AssertCompileSizeAlignment(VMXRESTOREHOST, 8);

/** @name Host-state MSR lazy-restoration flags.
 * @{
 */
/** The host MSRs have been saved. */
#define VMX_LAZY_MSRS_SAVED_HOST              RT_BIT(0)
/** The guest MSRs are loaded and in effect. */
#define VMX_LAZY_MSRS_LOADED_GUEST            RT_BIT(1)
/** @} */

/** @name VMX HM-error codes for VERR_HM_UNSUPPORTED_CPU_FEATURE_COMBO.
 *  UFC = Unsupported Feature Combination.
 * @{
 */
/** Unsupported pin-based VM-execution controls combo. */
#define VMX_UFC_CTRL_PIN_EXEC                                   1
/** Unsupported processor-based VM-execution controls combo. */
#define VMX_UFC_CTRL_PROC_EXEC                                  2
/** Unsupported move debug register VM-exit combo. */
#define VMX_UFC_CTRL_PROC_MOV_DRX_EXIT                          3
/** Unsupported VM-entry controls combo. */
#define VMX_UFC_CTRL_ENTRY                                      4
/** Unsupported VM-exit controls combo. */
#define VMX_UFC_CTRL_EXIT                                       5
/** MSR storage capacity of the VMCS autoload/store area is not sufficient
 *  for storing host MSRs. */
#define VMX_UFC_INSUFFICIENT_HOST_MSR_STORAGE                   6
/** MSR storage capacity of the VMCS autoload/store area is not sufficient
 *  for storing guest MSRs. */
#define VMX_UFC_INSUFFICIENT_GUEST_MSR_STORAGE                  7
/** Invalid VMCS size. */
#define VMX_UFC_INVALID_VMCS_SIZE                               8
/** Unsupported secondary processor-based VM-execution controls combo. */
#define VMX_UFC_CTRL_PROC_EXEC2                                 9
/** Invalid unrestricted-guest execution controls combo. */
#define VMX_UFC_INVALID_UX_COMBO                                10
/** EPT flush type not supported. */
#define VMX_UFC_EPT_FLUSH_TYPE_UNSUPPORTED                      11
/** EPT paging structure memory type is not write-back. */
#define VMX_UFC_EPT_MEM_TYPE_NOT_WB                             12
/** EPT requires INVEPT instr. support but it's not available. */
#define VMX_UFC_EPT_INVEPT_UNAVAILABLE                          13
/** EPT requires page-walk length of 4. */
#define VMX_UFC_EPT_PAGE_WALK_LENGTH_UNSUPPORTED                14
/** @} */

/** @name VMX HM-error codes for VERR_VMX_INVALID_GUEST_STATE.
 *  IGS = Invalid Guest State.
 * @{
 */
/** An error occurred while checking invalid-guest-state. */
#define VMX_IGS_ERROR                                           500
/** The invalid guest-state checks did not find any reason why. */
#define VMX_IGS_REASON_NOT_FOUND                                501
/** CR0 fixed1 bits invalid. */
#define VMX_IGS_CR0_FIXED1                                      502
/** CR0 fixed0 bits invalid. */
#define VMX_IGS_CR0_FIXED0                                      503
/** CR0.PE and CR0.PE invalid VT-x/host combination. */
#define VMX_IGS_CR0_PG_PE_COMBO                                 504
/** CR4 fixed1 bits invalid. */
#define VMX_IGS_CR4_FIXED1                                      505
/** CR4 fixed0 bits invalid. */
#define VMX_IGS_CR4_FIXED0                                      506
/** Reserved bits in VMCS' DEBUGCTL MSR field not set to 0 when
 *  VMX_VMCS_CTRL_ENTRY_LOAD_DEBUG is used. */
#define VMX_IGS_DEBUGCTL_MSR_RESERVED                           507
/** CR0.PG not set for long-mode when not using unrestricted guest. */
#define VMX_IGS_CR0_PG_LONGMODE                                 508
/** CR4.PAE not set for long-mode guest when not using unrestricted guest. */
#define VMX_IGS_CR4_PAE_LONGMODE                                509
/** CR4.PCIDE set for 32-bit guest. */
#define VMX_IGS_CR4_PCIDE                                       510
/** VMCS' DR7 reserved bits not set to 0. */
#define VMX_IGS_DR7_RESERVED                                    511
/** VMCS' PERF_GLOBAL MSR reserved bits not set to 0. */
#define VMX_IGS_PERF_GLOBAL_MSR_RESERVED                        512
/** VMCS' EFER MSR reserved bits not set to 0. */
#define VMX_IGS_EFER_MSR_RESERVED                               513
/** VMCS' EFER MSR.LMA does not match the IA32e mode guest control. */
#define VMX_IGS_EFER_LMA_GUEST_MODE_MISMATCH                    514
/** VMCS' EFER MSR.LMA does not match EFER.LME of the guest when using paging
 *  without unrestricted guest. */
#define VMX_IGS_EFER_LMA_LME_MISMATCH                           515
/** CS.Attr.P bit invalid. */
#define VMX_IGS_CS_ATTR_P_INVALID                               516
/** CS.Attr reserved bits not set to 0.  */
#define VMX_IGS_CS_ATTR_RESERVED                                517
/** CS.Attr.G bit invalid. */
#define VMX_IGS_CS_ATTR_G_INVALID                               518
/** CS is unusable. */
#define VMX_IGS_CS_ATTR_UNUSABLE                                519
/** CS and SS DPL unequal. */
#define VMX_IGS_CS_SS_ATTR_DPL_UNEQUAL                          520
/** CS and SS DPL mismatch. */
#define VMX_IGS_CS_SS_ATTR_DPL_MISMATCH                         521
/** CS Attr.Type invalid. */
#define VMX_IGS_CS_ATTR_TYPE_INVALID                            522
/** CS and SS RPL unequal. */
#define VMX_IGS_SS_CS_RPL_UNEQUAL                               523
/** SS.Attr.DPL and SS RPL unequal. */
#define VMX_IGS_SS_ATTR_DPL_RPL_UNEQUAL                         524
/** SS.Attr.DPL invalid for segment type. */
#define VMX_IGS_SS_ATTR_DPL_INVALID                             525
/** SS.Attr.Type invalid. */
#define VMX_IGS_SS_ATTR_TYPE_INVALID                            526
/** SS.Attr.P bit invalid. */
#define VMX_IGS_SS_ATTR_P_INVALID                               527
/** SS.Attr reserved bits not set to 0. */
#define VMX_IGS_SS_ATTR_RESERVED                                528
/** SS.Attr.G bit invalid. */
#define VMX_IGS_SS_ATTR_G_INVALID                               529
/** DS.Attr.A bit invalid. */
#define VMX_IGS_DS_ATTR_A_INVALID                               530
/** DS.Attr.P bit invalid. */
#define VMX_IGS_DS_ATTR_P_INVALID                               531
/** DS.Attr.DPL and DS RPL unequal. */
#define VMX_IGS_DS_ATTR_DPL_RPL_UNEQUAL                         532
/** DS.Attr reserved bits not set to 0. */
#define VMX_IGS_DS_ATTR_RESERVED                                533
/** DS.Attr.G bit invalid. */
#define VMX_IGS_DS_ATTR_G_INVALID                               534
/** DS.Attr.Type invalid. */
#define VMX_IGS_DS_ATTR_TYPE_INVALID                            535
/** ES.Attr.A bit invalid. */
#define VMX_IGS_ES_ATTR_A_INVALID                               536
/** ES.Attr.P bit invalid. */
#define VMX_IGS_ES_ATTR_P_INVALID                               537
/** ES.Attr.DPL and DS RPL unequal. */
#define VMX_IGS_ES_ATTR_DPL_RPL_UNEQUAL                         538
/** ES.Attr reserved bits not set to 0. */
#define VMX_IGS_ES_ATTR_RESERVED                                539
/** ES.Attr.G bit invalid. */
#define VMX_IGS_ES_ATTR_G_INVALID                               540
/** ES.Attr.Type invalid. */
#define VMX_IGS_ES_ATTR_TYPE_INVALID                            541
/** FS.Attr.A bit invalid. */
#define VMX_IGS_FS_ATTR_A_INVALID                               542
/** FS.Attr.P bit invalid. */
#define VMX_IGS_FS_ATTR_P_INVALID                               543
/** FS.Attr.DPL and DS RPL unequal. */
#define VMX_IGS_FS_ATTR_DPL_RPL_UNEQUAL                         544
/** FS.Attr reserved bits not set to 0. */
#define VMX_IGS_FS_ATTR_RESERVED                                545
/** FS.Attr.G bit invalid. */
#define VMX_IGS_FS_ATTR_G_INVALID                               546
/** FS.Attr.Type invalid. */
#define VMX_IGS_FS_ATTR_TYPE_INVALID                            547
/** GS.Attr.A bit invalid. */
#define VMX_IGS_GS_ATTR_A_INVALID                               548
/** GS.Attr.P bit invalid. */
#define VMX_IGS_GS_ATTR_P_INVALID                               549
/** GS.Attr.DPL and DS RPL unequal. */
#define VMX_IGS_GS_ATTR_DPL_RPL_UNEQUAL                         550
/** GS.Attr reserved bits not set to 0. */
#define VMX_IGS_GS_ATTR_RESERVED                                551
/** GS.Attr.G bit invalid. */
#define VMX_IGS_GS_ATTR_G_INVALID                               552
/** GS.Attr.Type invalid. */
#define VMX_IGS_GS_ATTR_TYPE_INVALID                            553
/** V86 mode CS.Base invalid. */
#define VMX_IGS_V86_CS_BASE_INVALID                             554
/** V86 mode CS.Limit invalid. */
#define VMX_IGS_V86_CS_LIMIT_INVALID                            555
/** V86 mode CS.Attr invalid. */
#define VMX_IGS_V86_CS_ATTR_INVALID                             556
/** V86 mode SS.Base invalid. */
#define VMX_IGS_V86_SS_BASE_INVALID                             557
/** V86 mode SS.Limit invalid. */
#define VMX_IGS_V86_SS_LIMIT_INVALID                            558
/** V86 mode SS.Attr invalid. */
#define VMX_IGS_V86_SS_ATTR_INVALID                             559
/** V86 mode DS.Base invalid. */
#define VMX_IGS_V86_DS_BASE_INVALID                             560
/** V86 mode DS.Limit invalid. */
#define VMX_IGS_V86_DS_LIMIT_INVALID                            561
/** V86 mode DS.Attr invalid. */
#define VMX_IGS_V86_DS_ATTR_INVALID                             562
/** V86 mode ES.Base invalid. */
#define VMX_IGS_V86_ES_BASE_INVALID                             563
/** V86 mode ES.Limit invalid. */
#define VMX_IGS_V86_ES_LIMIT_INVALID                            564
/** V86 mode ES.Attr invalid. */
#define VMX_IGS_V86_ES_ATTR_INVALID                             565
/** V86 mode FS.Base invalid. */
#define VMX_IGS_V86_FS_BASE_INVALID                             566
/** V86 mode FS.Limit invalid. */
#define VMX_IGS_V86_FS_LIMIT_INVALID                            567
/** V86 mode FS.Attr invalid. */
#define VMX_IGS_V86_FS_ATTR_INVALID                             568
/** V86 mode GS.Base invalid. */
#define VMX_IGS_V86_GS_BASE_INVALID                             569
/** V86 mode GS.Limit invalid. */
#define VMX_IGS_V86_GS_LIMIT_INVALID                            570
/** V86 mode GS.Attr invalid. */
#define VMX_IGS_V86_GS_ATTR_INVALID                             571
/** Longmode CS.Base invalid. */
#define VMX_IGS_LONGMODE_CS_BASE_INVALID                        572
/** Longmode SS.Base invalid. */
#define VMX_IGS_LONGMODE_SS_BASE_INVALID                        573
/** Longmode DS.Base invalid. */
#define VMX_IGS_LONGMODE_DS_BASE_INVALID                        574
/** Longmode ES.Base invalid. */
#define VMX_IGS_LONGMODE_ES_BASE_INVALID                        575
/** SYSENTER ESP is not canonical. */
#define VMX_IGS_SYSENTER_ESP_NOT_CANONICAL                      576
/** SYSENTER EIP is not canonical. */
#define VMX_IGS_SYSENTER_EIP_NOT_CANONICAL                      577
/** PAT MSR invalid. */
#define VMX_IGS_PAT_MSR_INVALID                                 578
/** PAT MSR reserved bits not set to 0. */
#define VMX_IGS_PAT_MSR_RESERVED                                579
/** GDTR.Base is not canonical. */
#define VMX_IGS_GDTR_BASE_NOT_CANONICAL                         580
/** IDTR.Base is not canonical. */
#define VMX_IGS_IDTR_BASE_NOT_CANONICAL                         581
/** GDTR.Limit invalid. */
#define VMX_IGS_GDTR_LIMIT_INVALID                              582
/** IDTR.Limit invalid. */
#define VMX_IGS_IDTR_LIMIT_INVALID                              583
/** Longmode RIP is invalid. */
#define VMX_IGS_LONGMODE_RIP_INVALID                            584
/** RFLAGS reserved bits not set to 0. */
#define VMX_IGS_RFLAGS_RESERVED                                 585
/** RFLAGS RA1 reserved bits not set to 1. */
#define VMX_IGS_RFLAGS_RESERVED1                                586
/** RFLAGS.VM (V86 mode) invalid. */
#define VMX_IGS_RFLAGS_VM_INVALID                               587
/** RFLAGS.IF invalid. */
#define VMX_IGS_RFLAGS_IF_INVALID                               588
/** Activity state invalid. */
#define VMX_IGS_ACTIVITY_STATE_INVALID                          589
/** Activity state HLT invalid when SS.Attr.DPL is not zero. */
#define VMX_IGS_ACTIVITY_STATE_HLT_INVALID                      590
/** Activity state ACTIVE invalid when block-by-STI or MOV SS. */
#define VMX_IGS_ACTIVITY_STATE_ACTIVE_INVALID                   591
/** Activity state SIPI WAIT invalid. */
#define VMX_IGS_ACTIVITY_STATE_SIPI_WAIT_INVALID                592
/** Interruptibility state reserved bits not set to 0. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_RESERVED                 593
/** Interruptibility state cannot be block-by-STI -and- MOV SS. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_STI_MOVSS_INVALID        594
/** Interruptibility state block-by-STI invalid for EFLAGS. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_STI_EFL_INVALID          595
/** Interruptibility state invalid while trying to deliver external
 *  interrupt. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_EXT_INT_INVALID          596
/** Interruptibility state block-by-MOVSS invalid while trying to deliver an
 *  NMI. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_MOVSS_INVALID            597
/** Interruptibility state block-by-SMI invalid when CPU is not in SMM. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_SMI_INVALID              598
/** Interruptibility state block-by-SMI invalid when trying to enter SMM. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_SMI_SMM_INVALID          599
/** Interruptibility state block-by-STI (maybe) invalid when trying to
 *  deliver an NMI. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_STI_INVALID              600
/** Interruptibility state block-by-NMI invalid when virtual-NMIs control is
 *  active. */
#define VMX_IGS_INTERRUPTIBILITY_STATE_NMI_INVALID              601
/** Pending debug exceptions reserved bits not set to 0. */
#define VMX_IGS_PENDING_DEBUG_RESERVED                          602
/** Longmode pending debug exceptions reserved bits not set to 0. */
#define VMX_IGS_LONGMODE_PENDING_DEBUG_RESERVED                 603
/** Pending debug exceptions.BS bit is not set when it should be. */
#define VMX_IGS_PENDING_DEBUG_XCPT_BS_NOT_SET                   604
/** Pending debug exceptions.BS bit is not clear when it should be. */
#define VMX_IGS_PENDING_DEBUG_XCPT_BS_NOT_CLEAR                 605
/** VMCS link pointer reserved bits not set to 0. */
#define VMX_IGS_VMCS_LINK_PTR_RESERVED                          606
/** TR cannot index into LDT, TI bit MBZ. */
#define VMX_IGS_TR_TI_INVALID                                   607
/** LDTR cannot index into LDT. TI bit MBZ. */
#define VMX_IGS_LDTR_TI_INVALID                                 608
/** TR.Base is not canonical. */
#define VMX_IGS_TR_BASE_NOT_CANONICAL                           609
/** FS.Base is not canonical. */
#define VMX_IGS_FS_BASE_NOT_CANONICAL                           610
/** GS.Base is not canonical. */
#define VMX_IGS_GS_BASE_NOT_CANONICAL                           611
/** LDTR.Base is not canonical. */
#define VMX_IGS_LDTR_BASE_NOT_CANONICAL                         612
/** TR is unusable. */
#define VMX_IGS_TR_ATTR_UNUSABLE                                613
/** TR.Attr.S bit invalid. */
#define VMX_IGS_TR_ATTR_S_INVALID                               614
/** TR is not present. */
#define VMX_IGS_TR_ATTR_P_INVALID                               615
/** TR.Attr reserved bits not set to 0. */
#define VMX_IGS_TR_ATTR_RESERVED                                616
/** TR.Attr.G bit invalid. */
#define VMX_IGS_TR_ATTR_G_INVALID                               617
/** Longmode TR.Attr.Type invalid. */
#define VMX_IGS_LONGMODE_TR_ATTR_TYPE_INVALID                   618
/** TR.Attr.Type invalid. */
#define VMX_IGS_TR_ATTR_TYPE_INVALID                            619
/** CS.Attr.S invalid. */
#define VMX_IGS_CS_ATTR_S_INVALID                               620
/** CS.Attr.DPL invalid. */
#define VMX_IGS_CS_ATTR_DPL_INVALID                             621
/** PAE PDPTE reserved bits not set to 0. */
#define VMX_IGS_PAE_PDPTE_RESERVED                              623
/** @} */

/** @name VMX VMCS-Read cache indices.
 * @{
 */
#define VMX_VMCS_GUEST_ES_BASE_CACHE_IDX                        0
#define VMX_VMCS_GUEST_CS_BASE_CACHE_IDX                        1
#define VMX_VMCS_GUEST_SS_BASE_CACHE_IDX                        2
#define VMX_VMCS_GUEST_DS_BASE_CACHE_IDX                        3
#define VMX_VMCS_GUEST_FS_BASE_CACHE_IDX                        4
#define VMX_VMCS_GUEST_GS_BASE_CACHE_IDX                        5
#define VMX_VMCS_GUEST_LDTR_BASE_CACHE_IDX                      6
#define VMX_VMCS_GUEST_TR_BASE_CACHE_IDX                        7
#define VMX_VMCS_GUEST_GDTR_BASE_CACHE_IDX                      8
#define VMX_VMCS_GUEST_IDTR_BASE_CACHE_IDX                      9
#define VMX_VMCS_GUEST_RSP_CACHE_IDX                            10
#define VMX_VMCS_GUEST_RIP_CACHE_IDX                            11
#define VMX_VMCS_GUEST_SYSENTER_ESP_CACHE_IDX                   12
#define VMX_VMCS_GUEST_SYSENTER_EIP_CACHE_IDX                   13
#define VMX_VMCS_RO_EXIT_QUALIFICATION_CACHE_IDX                14
#define VMX_VMCS_MAX_CACHE_IDX                                  (VMX_VMCS_RO_EXIT_QUALIFICATION_CACHE_IDX + 1)
#define VMX_VMCS_GUEST_CR3_CACHE_IDX                            15
#define VMX_VMCS_MAX_NESTED_PAGING_CACHE_IDX                    (VMX_VMCS_GUEST_CR3_CACHE_IDX + 1)
/** @} */

/** @name VMX EPT paging structures
 * @{
 */

/**
 * Number of page table entries in the EPT. (PDPTE/PDE/PTE)
 */
#define EPT_PG_ENTRIES          X86_PG_PAE_ENTRIES

/**
 * EPT Page Directory Pointer Entry. Bit view.
 * @todo uint64_t isn't safe for bitfields (gcc pedantic warnings, and IIRC,
 *       this did cause trouble with one compiler/version).
 */
typedef struct EPTPML4EBITS
{
    /** Present bit. */
    uint64_t    u1Present       : 1;
    /** Writable bit. */
    uint64_t    u1Write         : 1;
    /** Executable bit. */
    uint64_t    u1Execute       : 1;
    /** Reserved (must be 0). */
    uint64_t    u5Reserved      : 5;
    /** Available for software. */
    uint64_t    u4Available     : 4;
    /** Physical address of the next level (PD). Restricted by maximum physical address width of the cpu. */
    uint64_t    u40PhysAddr     : 40;
    /** Available for software. */
    uint64_t    u12Available    : 12;
} EPTPML4EBITS;
AssertCompileSize(EPTPML4EBITS, 8);

/** Bits 12-51 - - EPT - Physical Page number of the next level. */
#define EPT_PML4E_PG_MASK       X86_PML4E_PG_MASK
/** The page shift to get the PML4 index. */
#define EPT_PML4_SHIFT          X86_PML4_SHIFT
/** The PML4 index mask (apply to a shifted page address). */
#define EPT_PML4_MASK           X86_PML4_MASK

/**
 * EPT PML4E.
 */
typedef union EPTPML4E
{
    /** Normal view. */
    EPTPML4EBITS    n;
    /** Unsigned integer view. */
    X86PGPAEUINT    u;
    /** 64 bit unsigned integer view. */
    uint64_t        au64[1];
    /** 32 bit unsigned integer view. */
    uint32_t        au32[2];
} EPTPML4E;
AssertCompileSize(EPTPML4E, 8);
/** Pointer to a PML4 table entry. */
typedef EPTPML4E *PEPTPML4E;
/** Pointer to a const PML4 table entry. */
typedef const EPTPML4E *PCEPTPML4E;

/**
 * EPT PML4 Table.
 */
typedef struct EPTPML4
{
    EPTPML4E    a[EPT_PG_ENTRIES];
} EPTPML4;
AssertCompileSize(EPTPML4, 0x1000);
/** Pointer to an EPT PML4 Table. */
typedef EPTPML4 *PEPTPML4;
/** Pointer to a const EPT PML4 Table. */
typedef const EPTPML4 *PCEPTPML4;

/**
 * EPT Page Directory Pointer Entry. Bit view.
 */
typedef struct EPTPDPTEBITS
{
    /** Present bit. */
    uint64_t    u1Present       : 1;
    /** Writable bit. */
    uint64_t    u1Write         : 1;
    /** Executable bit. */
    uint64_t    u1Execute       : 1;
    /** Reserved (must be 0). */
    uint64_t    u5Reserved      : 5;
    /** Available for software. */
    uint64_t    u4Available     : 4;
    /** Physical address of the next level (PD). Restricted by maximum physical address width of the cpu. */
    uint64_t    u40PhysAddr     : 40;
    /** Available for software. */
    uint64_t    u12Available    : 12;
} EPTPDPTEBITS;
AssertCompileSize(EPTPDPTEBITS, 8);

/** Bits 12-51 - - EPT - Physical Page number of the next level. */
#define EPT_PDPTE_PG_MASK       X86_PDPE_PG_MASK
/** The page shift to get the PDPT index. */
#define EPT_PDPT_SHIFT          X86_PDPT_SHIFT
/** The PDPT index mask (apply to a shifted page address). */
#define EPT_PDPT_MASK           X86_PDPT_MASK_AMD64

/**
 * EPT Page Directory Pointer.
 */
typedef union EPTPDPTE
{
    /** Normal view. */
    EPTPDPTEBITS    n;
    /** Unsigned integer view. */
    X86PGPAEUINT    u;
    /** 64 bit unsigned integer view. */
    uint64_t        au64[1];
    /** 32 bit unsigned integer view. */
    uint32_t        au32[2];
} EPTPDPTE;
AssertCompileSize(EPTPDPTE, 8);
/** Pointer to an EPT Page Directory Pointer Entry. */
typedef EPTPDPTE *PEPTPDPTE;
/** Pointer to a const EPT Page Directory Pointer Entry. */
typedef const EPTPDPTE *PCEPTPDPTE;

/**
 * EPT Page Directory Pointer Table.
 */
typedef struct EPTPDPT
{
    EPTPDPTE    a[EPT_PG_ENTRIES];
} EPTPDPT;
AssertCompileSize(EPTPDPT, 0x1000);
/** Pointer to an EPT Page Directory Pointer Table. */
typedef EPTPDPT *PEPTPDPT;
/** Pointer to a const EPT Page Directory Pointer Table. */
typedef const EPTPDPT *PCEPTPDPT;


/**
 * EPT Page Directory Table Entry. Bit view.
 */
typedef struct EPTPDEBITS
{
    /** Present bit. */
    uint64_t    u1Present       : 1;
    /** Writable bit. */
    uint64_t    u1Write         : 1;
    /** Executable bit. */
    uint64_t    u1Execute       : 1;
    /** Reserved (must be 0). */
    uint64_t    u4Reserved      : 4;
    /** Big page (must be 0 here). */
    uint64_t    u1Size          : 1;
    /** Available for software. */
    uint64_t    u4Available     : 4;
    /** Physical address of page table. Restricted by maximum physical address width of the cpu. */
    uint64_t    u40PhysAddr     : 40;
    /** Available for software. */
    uint64_t    u12Available    : 12;
} EPTPDEBITS;
AssertCompileSize(EPTPDEBITS, 8);

/** Bits 12-51 - - EPT - Physical Page number of the next level. */
#define EPT_PDE_PG_MASK         X86_PDE_PAE_PG_MASK
/** The page shift to get the PD index. */
#define EPT_PD_SHIFT            X86_PD_PAE_SHIFT
/** The PD index mask (apply to a shifted page address). */
#define EPT_PD_MASK             X86_PD_PAE_MASK

/**
 * EPT 2MB Page Directory Table Entry. Bit view.
 */
typedef struct EPTPDE2MBITS
{
    /** Present bit. */
    uint64_t    u1Present       : 1;
    /** Writable bit. */
    uint64_t    u1Write         : 1;
    /** Executable bit. */
    uint64_t    u1Execute       : 1;
    /** EPT Table Memory Type. MBZ for non-leaf nodes. */
    uint64_t    u3EMT           : 3;
    /** Ignore PAT memory type */
    uint64_t    u1IgnorePAT     : 1;
    /** Big page (must be 1 here). */
    uint64_t    u1Size          : 1;
    /** Available for software. */
    uint64_t    u4Available     : 4;
    /** Reserved (must be 0). */
    uint64_t    u9Reserved      : 9;
    /** Physical address of the 2MB page. Restricted by maximum physical address width of the cpu. */
    uint64_t    u31PhysAddr     : 31;
    /** Available for software. */
    uint64_t    u12Available    : 12;
} EPTPDE2MBITS;
AssertCompileSize(EPTPDE2MBITS, 8);

/** Bits 21-51 - - EPT - Physical Page number of the next level. */
#define EPT_PDE2M_PG_MASK       X86_PDE2M_PAE_PG_MASK

/**
 * EPT Page Directory Table Entry.
 */
typedef union EPTPDE
{
    /** Normal view. */
    EPTPDEBITS      n;
    /** 2MB view (big). */
    EPTPDE2MBITS    b;
    /** Unsigned integer view. */
    X86PGPAEUINT    u;
    /** 64 bit unsigned integer view. */
    uint64_t        au64[1];
    /** 32 bit unsigned integer view. */
    uint32_t        au32[2];
} EPTPDE;
AssertCompileSize(EPTPDE, 8);
/** Pointer to an EPT Page Directory Table Entry. */
typedef EPTPDE *PEPTPDE;
/** Pointer to a const EPT Page Directory Table Entry. */
typedef const EPTPDE *PCEPTPDE;

/**
 * EPT Page Directory Table.
 */
typedef struct EPTPD
{
    EPTPDE      a[EPT_PG_ENTRIES];
} EPTPD;
AssertCompileSize(EPTPD, 0x1000);
/** Pointer to an EPT Page Directory Table. */
typedef EPTPD *PEPTPD;
/** Pointer to a const EPT Page Directory Table. */
typedef const EPTPD *PCEPTPD;


/**
 * EPT Page Table Entry. Bit view.
 */
typedef struct EPTPTEBITS
{
    /** 0 - Present bit.
     * @remark This is a convenience "misnomer".  The bit actually indicates
     *         read access and the CPU will consider an entry with any of the
     *         first three bits set as present.  Since all our valid entries
     *         will have this bit set, it can be used as a present indicator
     *         and allow some code sharing. */
    uint64_t    u1Present       : 1;
    /** 1 - Writable bit. */
    uint64_t    u1Write         : 1;
    /** 2 - Executable bit. */
    uint64_t    u1Execute       : 1;
    /** 5:3 - EPT Memory Type. MBZ for non-leaf nodes. */
    uint64_t    u3EMT           : 3;
    /** 6 - Ignore PAT memory type */
    uint64_t    u1IgnorePAT     : 1;
    /** 11:7 - Available for software. */
    uint64_t    u5Available     : 5;
    /** 51:12 - Physical address of page. Restricted by maximum physical
     *  address width of the cpu. */
    uint64_t    u40PhysAddr     : 40;
    /** 63:52 - Available for software. */
    uint64_t    u12Available    : 12;
} EPTPTEBITS;
AssertCompileSize(EPTPTEBITS, 8);

/** Bits 12-51 - - EPT - Physical Page number of the next level. */
#define EPT_PTE_PG_MASK         X86_PTE_PAE_PG_MASK
/** The page shift to get the EPT PTE index. */
#define EPT_PT_SHIFT            X86_PT_PAE_SHIFT
/** The EPT PT index mask (apply to a shifted page address). */
#define EPT_PT_MASK             X86_PT_PAE_MASK

/**
 * EPT Page Table Entry.
 */
typedef union EPTPTE
{
    /** Normal view. */
    EPTPTEBITS      n;
    /** Unsigned integer view. */
    X86PGPAEUINT    u;
    /** 64 bit unsigned integer view. */
    uint64_t        au64[1];
    /** 32 bit unsigned integer view. */
    uint32_t        au32[2];
} EPTPTE;
AssertCompileSize(EPTPTE, 8);
/** Pointer to an EPT Page Directory Table Entry. */
typedef EPTPTE *PEPTPTE;
/** Pointer to a const EPT Page Directory Table Entry. */
typedef const EPTPTE *PCEPTPTE;

/**
 * EPT Page Table.
 */
typedef struct EPTPT
{
    EPTPTE      a[EPT_PG_ENTRIES];
} EPTPT;
AssertCompileSize(EPTPT, 0x1000);
/** Pointer to an extended page table. */
typedef EPTPT *PEPTPT;
/** Pointer to a const extended table. */
typedef const EPTPT *PCEPTPT;

/** @} */

/** VMX VPID flush types.
 * @note Valid enum members are in accordance to the VT-x spec.
 */
typedef enum
{
    /** Invalidate a specific page. */
    VMXFLUSHVPID_INDIV_ADDR                    = 0,
    /** Invalidate one context (specific VPID). */
    VMXFLUSHVPID_SINGLE_CONTEXT                = 1,
    /** Invalidate all contexts (all VPIDs). */
    VMXFLUSHVPID_ALL_CONTEXTS                  = 2,
    /** Invalidate a single VPID context retaining global mappings. */
    VMXFLUSHVPID_SINGLE_CONTEXT_RETAIN_GLOBALS = 3,
    /** Unsupported by VirtualBox. */
    VMXFLUSHVPID_NOT_SUPPORTED                 = 0xbad0,
    /** Unsupported by CPU. */
    VMXFLUSHVPID_NONE                          = 0xbad1
} VMXFLUSHVPID;
AssertCompileSize(VMXFLUSHVPID, 4);

/** VMX EPT flush types.
 * @note Valid enums values are in accordance to the VT-x spec.
 */
typedef enum
{
    /** Invalidate one context (specific EPT). */
    VMXFLUSHEPT_SINGLE_CONTEXT                 = 1,
    /* Invalidate all contexts (all EPTs) */
    VMXFLUSHEPT_ALL_CONTEXTS                   = 2,
    /** Unsupported by VirtualBox.   */
    VMXFLUSHEPT_NOT_SUPPORTED                  = 0xbad0,
    /** Unsupported by CPU. */
    VMXFLUSHEPT_NONE                           = 0xbad1
} VMXFLUSHEPT;
AssertCompileSize(VMXFLUSHEPT, 4);

/** VMX Posted Interrupt Descriptor.
 * In accordance to the VT-x spec.
 */
typedef struct VMXPOSTEDINTRDESC
{
    uint32_t    aVectorBitmap[8];
    uint32_t    fOutstandingNotification : 1;
    uint32_t    uReserved0               : 31;
    uint8_t     au8Reserved0[28];
} VMXPOSTEDINTRDESC;
AssertCompileMemberSize(VMXPOSTEDINTRDESC, aVectorBitmap, 32);
AssertCompileSize(VMXPOSTEDINTRDESC, 64);
/** Pointer to a posted interrupt descriptor. */
typedef VMXPOSTEDINTRDESC *PVMXPOSTEDINTRDESC;
/** Pointer to a const posted interrupt descriptor. */
typedef const VMXPOSTEDINTRDESC *PCVMXPOSTEDINTRDESC;

/** VMX MSR autoload/store element.
 * In accordance to the VT-x spec.
 */
typedef struct VMXAUTOMSR
{
    /** The MSR Id. */
    uint32_t    u32Msr;
    /** Reserved (MBZ). */
    uint32_t    u32Reserved;
    /** The MSR value. */
    uint64_t    u64Value;
} VMXAUTOMSR;
AssertCompileSize(VMXAUTOMSR, 16);
/** Pointer to an MSR load/store element. */
typedef VMXAUTOMSR *PVMXAUTOMSR;
/** Pointer to a const MSR load/store element. */
typedef const VMXAUTOMSR *PCVMXAUTOMSR;

/**
 * VMX-capability qword
 */
typedef union
{
    struct
    {
        /** Bits set here -must- be set in the corresponding VM-execution controls. */
        uint32_t        disallowed0;
        /** Bits cleared here -must- be cleared in the corresponding VM-execution
         *  controls. */
        uint32_t        allowed1;
    } n;
    uint64_t            u;
} VMXCAPABILITY;
AssertCompileSize(VMXCAPABILITY, 8);

/**
 * VMX MSRs.
 */
typedef struct VMXMSRS
{
    uint64_t                u64FeatureCtrl;
    uint64_t                u64BasicInfo;
    VMXCAPABILITY           VmxPinCtls;
    VMXCAPABILITY           VmxProcCtls;
    VMXCAPABILITY           VmxProcCtls2;
    VMXCAPABILITY           VmxExit;
    VMXCAPABILITY           VmxEntry;
    uint64_t                u64Misc;
    uint64_t                u64Cr0Fixed0;
    uint64_t                u64Cr0Fixed1;
    uint64_t                u64Cr4Fixed0;
    uint64_t                u64Cr4Fixed1;
    uint64_t                u64VmcsEnum;
    uint64_t                u64Vmfunc;
    uint64_t                u64EptVpidCaps;
} VMXMSRS;
AssertCompileSizeAlignment(VMXMSRS, 8);
/** Pointer to a VMXMSRS struct. */
typedef VMXMSRS *PVMXMSRS;

/** @name VMX EFLAGS reserved bits.
 * @{
 */
/** And-mask for setting reserved bits to zero */
#define VMX_EFLAGS_RESERVED_0                                   (X86_EFL_1 | X86_EFL_LIVE_MASK)
/** Or-mask for setting reserved bits to 1 */
#define VMX_EFLAGS_RESERVED_1                                   X86_EFL_1
/** @} */

/** @name VMX Basic Exit Reasons.
 * @{
 */
/** -1 Invalid exit code */
#define VMX_EXIT_INVALID                                        -1
/** 0 Exception or non-maskable interrupt (NMI). */
#define VMX_EXIT_XCPT_OR_NMI                                    0
/** 1 External interrupt. */
#define VMX_EXIT_EXT_INT                                        1
/** 2 Triple fault. */
#define VMX_EXIT_TRIPLE_FAULT                                   2
/** 3 INIT signal. */
#define VMX_EXIT_INIT_SIGNAL                                    3
/** 4 Start-up IPI (SIPI). */
#define VMX_EXIT_SIPI                                           4
/** 5 I/O system-management interrupt (SMI). */
#define VMX_EXIT_IO_SMI                                         5
/** 6 Other SMI. */
#define VMX_EXIT_SMI                                            6
/** 7 Interrupt window exiting. */
#define VMX_EXIT_INT_WINDOW                                     7
/** 8 NMI window exiting. */
#define VMX_EXIT_NMI_WINDOW                                     8
/** 9 Task switch. */
#define VMX_EXIT_TASK_SWITCH                                    9
/** 10 Guest software attempted to execute CPUID. */
#define VMX_EXIT_CPUID                                          10
/** 11 Guest software attempted to execute GETSEC. */
#define VMX_EXIT_GETSEC                                         11
/** 12 Guest software attempted to execute HLT. */
#define VMX_EXIT_HLT                                            12
/** 13 Guest software attempted to execute INVD. */
#define VMX_EXIT_INVD                                           13
/** 14 Guest software attempted to execute INVLPG. */
#define VMX_EXIT_INVLPG                                         14
/** 15 Guest software attempted to execute RDPMC. */
#define VMX_EXIT_RDPMC                                          15
/** 16 Guest software attempted to execute RDTSC. */
#define VMX_EXIT_RDTSC                                          16
/** 17 Guest software attempted to execute RSM in SMM. */
#define VMX_EXIT_RSM                                            17
/** 18 Guest software executed VMCALL. */
#define VMX_EXIT_VMCALL                                         18
/** 19 Guest software executed VMCLEAR. */
#define VMX_EXIT_VMCLEAR                                        19
/** 20 Guest software executed VMLAUNCH. */
#define VMX_EXIT_VMLAUNCH                                       20
/** 21 Guest software executed VMPTRLD. */
#define VMX_EXIT_VMPTRLD                                        21
/** 22 Guest software executed VMPTRST. */
#define VMX_EXIT_VMPTRST                                        22
/** 23 Guest software executed VMREAD. */
#define VMX_EXIT_VMREAD                                         23
/** 24 Guest software executed VMRESUME. */
#define VMX_EXIT_VMRESUME                                       24
/** 25 Guest software executed VMWRITE. */
#define VMX_EXIT_VMWRITE                                        25
/** 26 Guest software executed VMXOFF. */
#define VMX_EXIT_VMXOFF                                         26
/** 27 Guest software executed VMXON. */
#define VMX_EXIT_VMXON                                          27
/** 28 Control-register accesses. */
#define VMX_EXIT_MOV_CRX                                        28
/** 29 Debug-register accesses. */
#define VMX_EXIT_MOV_DRX                                        29
/** 30 I/O instruction. */
#define VMX_EXIT_IO_INSTR                                       30
/** 31 RDMSR. Guest software attempted to execute RDMSR. */
#define VMX_EXIT_RDMSR                                          31
/** 32 WRMSR. Guest software attempted to execute WRMSR. */
#define VMX_EXIT_WRMSR                                          32
/** 33 VM-entry failure due to invalid guest state. */
#define VMX_EXIT_ERR_INVALID_GUEST_STATE                        33
/** 34 VM-entry failure due to MSR loading. */
#define VMX_EXIT_ERR_MSR_LOAD                                   34
/** 36 Guest software executed MWAIT. */
#define VMX_EXIT_MWAIT                                          36
/** 37 VM-exit due to monitor trap flag. */
#define VMX_EXIT_MTF                                            37
/** 39 Guest software attempted to execute MONITOR. */
#define VMX_EXIT_MONITOR                                        39
/** 40 Guest software attempted to execute PAUSE. */
#define VMX_EXIT_PAUSE                                          40
/** 41 VM-entry failure due to machine-check. */
#define VMX_EXIT_ERR_MACHINE_CHECK                              41
/** 43 TPR below threshold. Guest software executed MOV to CR8. */
#define VMX_EXIT_TPR_BELOW_THRESHOLD                            43
/** 44 APIC access. Guest software attempted to access memory at a physical address on the APIC-access page. */
#define VMX_EXIT_APIC_ACCESS                                    44
/** 45 Virtualized EOI. EOI virtualization was performed for a virtual interrupt
whose vector indexed a bit set in the EOI-exit bitmap. */
#define VMX_EXIT_VIRTUALIZED_EOI                                45
/** 46 Access to GDTR or IDTR. Guest software attempted to execute LGDT, LIDT, SGDT, or SIDT. */
#define VMX_EXIT_XDTR_ACCESS                                    46
/** 47 Access to LDTR or TR. Guest software attempted to execute LLDT, LTR, SLDT, or STR. */
#define VMX_EXIT_TR_ACCESS                                      47
/** 48 EPT violation. An attempt to access memory with a guest-physical address was disallowed by the configuration of the EPT paging structures. */
#define VMX_EXIT_EPT_VIOLATION                                  48
/** 49 EPT misconfiguration. An attempt to access memory with a guest-physical address encountered a misconfigured EPT paging-structure entry. */
#define VMX_EXIT_EPT_MISCONFIG                                  49
/** 50 INVEPT. Guest software attempted to execute INVEPT. */
#define VMX_EXIT_INVEPT                                         50
/** 51 RDTSCP. Guest software attempted to execute RDTSCP. */
#define VMX_EXIT_RDTSCP                                         51
/** 52 VMX-preemption timer expired. The preemption timer counted down to zero. */
#define VMX_EXIT_PREEMPT_TIMER                                  52
/** 53 INVVPID. Guest software attempted to execute INVVPID. */
#define VMX_EXIT_INVVPID                                        53
/** 54 WBINVD. Guest software attempted to execute WBINVD. */
#define VMX_EXIT_WBINVD                                         54
/** 55 XSETBV. Guest software attempted to execute XSETBV. */
#define VMX_EXIT_XSETBV                                         55
/** 56 APIC write. Guest completed write to virtual-APIC. */
#define VMX_EXIT_APIC_WRITE                                     56
/** 57 RDRAND. Guest software attempted to execute RDRAND. */
#define VMX_EXIT_RDRAND                                         57
/** 58 INVPCID. Guest software attempted to execute INVPCID. */
#define VMX_EXIT_INVPCID                                        58
/** 59 VMFUNC. Guest software attempted to execute VMFUNC. */
#define VMX_EXIT_VMFUNC                                         59
/** 60 ENCLS. Guest software attempted to execute ENCLS. */
#define VMX_EXIT_ENCLS                                          60
/** 61 - RDSEED - Guest software attempted to executed RDSEED and exiting was
 * enabled. */
#define VMX_EXIT_RDSEED                                         61
/** 62 - Page-modification log full. */
#define VMX_EXIT_PML_FULL                                       62
/** 63 - XSAVES - Guest software attempted to executed XSAVES and exiting was
 * enabled (XSAVES/XRSTORS was enabled too, of course). */
#define VMX_EXIT_XSAVES                                         63
/** 63 - XRSTORS - Guest software attempted to executed XRSTORS and exiting
 * was enabled (XSAVES/XRSTORS was enabled too, of course). */
#define VMX_EXIT_XRSTORS                                        64
/** The maximum exit value (inclusive). */
#define VMX_EXIT_MAX                                            (VMX_EXIT_XRSTORS)
/** @} */


/** @name VM Instruction Errors
 * @{
 */
/** VMCALL executed in VMX root operation. */
#define VMX_ERROR_VMCALL                                        1
/** VMCLEAR with invalid physical address. */
#define VMX_ERROR_VMCLEAR_INVALID_PHYS_ADDR                     2
/** VMCLEAR with VMXON pointer. */
#define VMX_ERROR_VMCLEAR_INVALID_VMXON_PTR                     3
/** VMLAUNCH with non-clear VMCS. */
#define VMX_ERROR_VMLAUCH_NON_CLEAR_VMCS                        4
/** VMRESUME with non-launched VMCS. */
#define VMX_ERROR_VMRESUME_NON_LAUNCHED_VMCS                    5
/** VMRESUME with a corrupted VMCS (indicates corruption of the current VMCS). */
#define VMX_ERROR_VMRESUME_CORRUPTED_VMCS                       6
/** VM-entry with invalid control field(s). */
#define VMX_ERROR_VMENTRY_INVALID_CONTROL_FIELDS                7
/** VM-entry with invalid host-state field(s). */
#define VMX_ERROR_VMENTRY_INVALID_HOST_STATE                    8
/** VMPTRLD with invalid physical address. */
#define VMX_ERROR_VMPTRLD_INVALID_PHYS_ADDR                     9
/** VMPTRLD with VMXON pointer. */
#define VMX_ERROR_VMPTRLD_VMXON_PTR                             10
/** VMPTRLD with incorrect VMCS revision identifier. */
#define VMX_ERROR_VMPTRLD_WRONG_VMCS_REVISION                   11
/** VMREAD/VMWRITE from/to unsupported VMCS component. */
#define VMX_ERROR_VMREAD_INVALID_COMPONENT                      12
#define VMX_ERROR_VMWRITE_INVALID_COMPONENT                     VMX_ERROR_VMREAD_INVALID_COMPONENT
/** VMWRITE to read-only VMCS component. */
#define VMX_ERROR_VMWRITE_READONLY_COMPONENT                    13
/** VMXON executed in VMX root operation. */
#define VMX_ERROR_VMXON_IN_VMX_ROOT_OP                          15
/** VM-entry with invalid executive-VMCS pointer. */
#define VMX_ERROR_VMENTRY_INVALID_VMCS_EXEC_PTR                 16
/** VM-entry with non-launched executive VMCS. */
#define VMX_ERROR_VMENTRY_NON_LAUNCHED_EXEC_VMCS                17
/** VM-entry with executive-VMCS pointer not VMXON pointer. */
#define VMX_ERROR_VMENTRY_EXEC_VMCS_PTR                         18
/** VMCALL with non-clear VMCS. */
#define VMX_ERROR_VMCALL_NON_CLEAR_VMCS                         19
/** VMCALL with invalid VM-exit control fields. */
#define VMX_ERROR_VMCALL_INVALID_VMEXIT_FIELDS                  20
/** VMCALL with incorrect MSEG revision identifier. */
#define VMX_ERROR_VMCALL_INVALID_MSEG_REVISION                  22
/** VMXOFF under dual-monitor treatment of SMIs and SMM. */
#define VMX_ERROR_VMXOFF_DUAL_MONITOR                           23
/** VMCALL with invalid SMM-monitor features. */
#define VMX_ERROR_VMCALL_INVALID_SMM_MONITOR                    24
/** VM-entry with invalid VM-execution control fields in executive VMCS. */
#define VMX_ERROR_VMENTRY_INVALID_VM_EXEC_CTRL                  25
/** VM-entry with events blocked by MOV SS. */
#define VMX_ERROR_VMENTRY_MOV_SS                                26
/** Invalid operand to INVEPT/INVVPID. */
#define VMX_ERROR_INVEPTVPID_INVALID_OPERAND                    28
/** @} */


/** @name VMX MSRs - Basic VMX information.
 * @{
 */
/** VMCS revision identifier used by the processor. */
#define MSR_IA32_VMX_BASIC_INFO_VMCS_ID(a)                      ((a) & 0x7FFFFFFF)
/** Size of the VMCS. */
#define MSR_IA32_VMX_BASIC_INFO_VMCS_SIZE(a)                    (((a) >> 32) & 0x1FFF)
/** Width of physical address used for the VMCS.
 *  0 -> limited to the available amount of physical ram
 *  1 -> within the first 4 GB
 */
#define MSR_IA32_VMX_BASIC_INFO_VMCS_PHYS_WIDTH(a)              (((a) >> 48) & 1)
/** Whether the processor supports the dual-monitor treatment of system-management interrupts and system-management code. (always 1) */
#define MSR_IA32_VMX_BASIC_INFO_VMCS_DUAL_MON(a)                (((a) >> 49) & 1)
/** Memory type that must be used for the VMCS. */
#define MSR_IA32_VMX_BASIC_INFO_VMCS_MEM_TYPE(a)                (((a) >> 50) & 0xF)
/** Whether the processor provides additional information for exits due to INS/OUTS. */
#define MSR_IA32_VMX_BASIC_INFO_VMCS_INS_OUTS(a)                ((a) & RT_BIT_64(54))
/** Whether default 1 bits in control MSRs (pin/proc/exit/entry) may be
 *  cleared to 0 and that 'true' control MSRs are supported. */
#define MSR_IA32_VMX_BASIC_INFO_TRUE_CONTROLS(a)                ((a) & RT_BIT_64(55))
/** @} */


/** @name VMX MSRs - Misc VMX info.
 * @{
 */
/** Relationship between the preemption timer and tsc; count down every time bit x of the tsc changes. */
#define MSR_IA32_VMX_MISC_PREEMPT_TSC_BIT(a)                    ((a) & 0x1f)
/** Whether VM-exit stores EFER.LMA into the "IA32e mode guest" field. */
#define MSR_IA32_VMX_MISC_STORE_EFERLMA_VMEXIT(a)               (((a) >> 5) & 1)
/** Activity states supported by the implementation. */
#define MSR_IA32_VMX_MISC_ACTIVITY_STATES(a)                    (((a) >> 6) & 0x7)
/** Number of CR3 target values supported by the processor. (0-256) */
#define MSR_IA32_VMX_MISC_CR3_TARGET(a)                         (((a) >> 16) & 0x1FF)
/** Maximum number of MSRs in the VMCS. (N+1)*512. */
#define MSR_IA32_VMX_MISC_MAX_MSR(a)                            (((((a) >> 25) & 0x7) + 1) * 512)
/** Whether RDMSR can be used to read IA32_SMBASE_MSR in SMM. */
#define MSR_IA32_VMX_MISC_RDMSR_SMBASE_MSR_SMM(a)               (((a) >> 15) & 1)
/** Whether bit 2 of IA32_SMM_MONITOR_CTL can be set to 1. */
#define MSR_IA32_VMX_MISC_SMM_MONITOR_CTL_B2(a)                 (((a) >> 28) & 1)
/** Whether VMWRITE can be used to write VM-exit information fields. */
#define MSR_IA32_VMX_MISC_VMWRITE_VMEXIT_INFO(a)                (((a) >> 29) & 1)
/** MSEG revision identifier used by the processor. */
#define MSR_IA32_VMX_MISC_MSEG_ID(a)                            ((a) >> 32)
/** @} */


/** @name VMX MSRs - VMCS enumeration field info
 * @{
 */
/** Highest field index. */
#define MSR_IA32_VMX_VMCS_ENUM_HIGHEST_INDEX(a)                 (((a) >> 1) & 0x1FF)
/** @} */


/** @name MSR_IA32_VMX_EPT_VPID_CAPS; EPT capabilities MSR
 * @{
 */
#define MSR_IA32_VMX_EPT_VPID_CAP_RWX_X_ONLY                             RT_BIT_64(0)
#define MSR_IA32_VMX_EPT_VPID_CAP_PAGE_WALK_LENGTH_4                     RT_BIT_64(6)
#define MSR_IA32_VMX_EPT_VPID_CAP_EMT_UC                                 RT_BIT_64(8)
#define MSR_IA32_VMX_EPT_VPID_CAP_EMT_WB                                 RT_BIT_64(14)
#define MSR_IA32_VMX_EPT_VPID_CAP_PDE_2M                                 RT_BIT_64(16)
#define MSR_IA32_VMX_EPT_VPID_CAP_PDPTE_1G                               RT_BIT_64(17)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVEPT                                 RT_BIT_64(20)
#define MSR_IA32_VMX_EPT_VPID_CAP_EPT_ACCESS_DIRTY                       RT_BIT_64(21)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVEPT_SINGLE_CONTEXT                  RT_BIT_64(25)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVEPT_ALL_CONTEXTS                    RT_BIT_64(26)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVVPID                                RT_BIT_64(32)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_INDIV_ADDR                     RT_BIT_64(40)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT                 RT_BIT_64(41)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_ALL_CONTEXTS                   RT_BIT_64(42)
#define MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_RETAIN_GLOBALS  RT_BIT_64(43)
/** @} */

/** @name Extended Page Table Pointer (EPTP)
 * @{
 */
/** Uncachable EPT paging structure memory type. */
#define VMX_EPT_MEMTYPE_UC                                      0
/** Write-back EPT paging structure memory type. */
#define VMX_EPT_MEMTYPE_WB                                      6
/** Shift value to get the EPT page walk length (bits 5-3) */
#define VMX_EPT_PAGE_WALK_LENGTH_SHIFT                          3
/** Mask value to get the EPT page walk length (bits 5-3) */
#define VMX_EPT_PAGE_WALK_LENGTH_MASK                           7
/** Default EPT page-walk length (1 less than the actual EPT page-walk
 *  length) */
#define VMX_EPT_PAGE_WALK_LENGTH_DEFAULT                        3
/** @} */


/** @name VMCS field encoding - 16 bits guest fields
 * @{
 */
#define VMX_VMCS16_VPID                                         0x000
#define VMX_VMCS16_POSTED_INTR_NOTIFY_VECTOR                    0x002
#define VMX_VMCS16_EPTP_INDEX                                   0x004
#define VMX_VMCS16_GUEST_ES_SEL                                 0x800
#define VMX_VMCS16_GUEST_CS_SEL                                 0x802
#define VMX_VMCS16_GUEST_SS_SEL                                 0x804
#define VMX_VMCS16_GUEST_DS_SEL                                 0x806
#define VMX_VMCS16_GUEST_FS_SEL                                 0x808
#define VMX_VMCS16_GUEST_GS_SEL                                 0x80A
#define VMX_VMCS16_GUEST_LDTR_SEL                               0x80C
#define VMX_VMCS16_GUEST_TR_SEL                                 0x80E
#define VMX_VMCS16_GUEST_INTR_STATUS                            0x810
/** @} */

/** @name VMCS field encoding - 16 bits host fields
 * @{
 */
#define VMX_VMCS16_HOST_ES_SEL                                  0xC00
#define VMX_VMCS16_HOST_CS_SEL                                  0xC02
#define VMX_VMCS16_HOST_SS_SEL                                  0xC04
#define VMX_VMCS16_HOST_DS_SEL                                  0xC06
#define VMX_VMCS16_HOST_FS_SEL                                  0xC08
#define VMX_VMCS16_HOST_GS_SEL                                  0xC0A
#define VMX_VMCS16_HOST_TR_SEL                                  0xC0C
/** @}          */

/** @name VMCS field encoding - 64 bits host fields
 * @{
 */
#define VMX_VMCS64_HOST_PAT_FULL                                0x2C00
#define VMX_VMCS64_HOST_PAT_HIGH                                0x2C01
#define VMX_VMCS64_HOST_EFER_FULL                               0x2C02
#define VMX_VMCS64_HOST_EFER_HIGH                               0x2C03
#define VMX_VMCS64_HOST_PERF_GLOBAL_CTRL_FULL                   0x2C04      /**< MSR IA32_PERF_GLOBAL_CTRL */
#define VMX_VMCS64_HOST_PERF_GLOBAL_CTRL_HIGH                   0x2C05      /**< MSR IA32_PERF_GLOBAL_CTRL */
/** @}          */


/** @name VMCS field encoding - 64 Bits control fields
 * @{
 */
#define VMX_VMCS64_CTRL_IO_BITMAP_A_FULL                        0x2000
#define VMX_VMCS64_CTRL_IO_BITMAP_A_HIGH                        0x2001
#define VMX_VMCS64_CTRL_IO_BITMAP_B_FULL                        0x2002
#define VMX_VMCS64_CTRL_IO_BITMAP_B_HIGH                        0x2003

/* Optional */
#define VMX_VMCS64_CTRL_MSR_BITMAP_FULL                         0x2004
#define VMX_VMCS64_CTRL_MSR_BITMAP_HIGH                         0x2005

#define VMX_VMCS64_CTRL_EXIT_MSR_STORE_FULL                     0x2006
#define VMX_VMCS64_CTRL_EXIT_MSR_STORE_HIGH                     0x2007
#define VMX_VMCS64_CTRL_EXIT_MSR_LOAD_FULL                      0x2008
#define VMX_VMCS64_CTRL_EXIT_MSR_LOAD_HIGH                      0x2009

#define VMX_VMCS64_CTRL_ENTRY_MSR_LOAD_FULL                     0x200A
#define VMX_VMCS64_CTRL_ENTRY_MSR_LOAD_HIGH                     0x200B

#define VMX_VMCS64_CTRL_EXEC_VMCS_PTR_FULL                      0x200C
#define VMX_VMCS64_CTRL_EXEC_VMCS_PTR_HIGH                      0x200D

#define VMX_VMCS64_CTRL_TSC_OFFSET_FULL                         0x2010
#define VMX_VMCS64_CTRL_TSC_OFFSET_HIGH                         0x2011

/** Optional (VMX_VMCS_CTRL_PROC_EXEC_USE_TPR_SHADOW) */
#define VMX_VMCS64_CTRL_VAPIC_PAGEADDR_FULL                     0x2012
#define VMX_VMCS64_CTRL_VAPIC_PAGEADDR_HIGH                     0x2013

/** Optional (VMX_VMCS_CTRL_PROC_EXEC2_VIRT_APIC) */
#define VMX_VMCS64_CTRL_APIC_ACCESSADDR_FULL                    0x2014
#define VMX_VMCS64_CTRL_APIC_ACCESSADDR_HIGH                    0x2015

/** Optional (VMX_VMCS_CTRL_PROC_EXEC2_VMFUNC) */
#define VMX_VMCS64_CTRL_POSTED_INTR_DESC_FULL                   0x2016
#define VMX_VMCS64_CTRL_POSTED_INTR_DESC_HIGH                   0x2017

/** Optional (VMX_VMCS_CTRL_PROC_EXEC2_VMFUNC) */
#define VMX_VMCS64_CTRL_VMFUNC_CTRLS_FULL                       0x2018
#define VMX_VMCS64_CTRL_VMFUNC_CTRLS_HIGH                       0x2019

/** Extended page table pointer. */
#define VMX_VMCS64_CTRL_EPTP_FULL                               0x201A
#define VMX_VMCS64_CTRL_EPTP_HIGH                               0x201B

/** EOI-exit bitmap 0. */
#define VMX_VMCS64_CTRL_EOI_BITMAP_0_FULL                       0x201C
#define VMX_VMCS64_CTRL_EOI_BITMAP_0_HIGH                       0x201D

/** EOI-exit bitmap 1. */
#define VMX_VMCS64_CTRL_EOI_BITMAP_1_FULL                       0x201E
#define VMX_VMCS64_CTRL_EOI_BITMAP_1_HIGH                       0x201F

/** EOI-exit bitmap 2. */
#define VMX_VMCS64_CTRL_EOI_BITMAP_2_FULL                       0x2020
#define VMX_VMCS64_CTRL_EOI_BITMAP_2_HIGH                       0x2021

/** EOI-exit bitmap 3. */
#define VMX_VMCS64_CTRL_EOI_BITMAP_3_FULL                       0x2022
#define VMX_VMCS64_CTRL_EOI_BITMAP_3_HIGH                       0x2023

/** Extended page table pointer lists. */
#define VMX_VMCS64_CTRL_EPTP_LIST_FULL                          0x2024
#define VMX_VMCS64_CTRL_EPTP_LIST_HIGH                          0x2025

/** VM-read bitmap. */
#define VMX_VMCS64_CTRL_VMREAD_BITMAP_FULL                      0x2026
#define VMX_VMCS64_CTRL_VMREAD_BITMAP_HIGH                      0x2027

/** VM-write bitmap. */
#define VMX_VMCS64_CTRL_VMWRITE_BITMAP_FULL                     0x2028
#define VMX_VMCS64_CTRL_VMWRITE_BITMAP_HIGH                     0x2029

/** Virtualization-exception information address. */
#define VMX_VMCS64_CTRL_VIRTXCPT_INFO_ADDR_FULL                 0x202A
#define VMX_VMCS64_CTRL_VIRTXCPT_INFO_ADDR_HIGH                 0x202B

/** XSS-exiting bitmap. */
#define VMX_VMCS64_CTRL_XSS_EXITING_BITMAP_FULL                 0x202C
#define VMX_VMCS64_CTRL_XSS_EXITING_BITMAP_HIGH                 0x202D

/** TSC multiplier. */
#define VMX_VMCS64_CTRL_TSC_MULTIPLIER_FULL                     0x2032
#define VMX_VMCS64_CTRL_TSC_MULTIPLIER_HIGH                     0x2033

/** VM-exit guest physical address. */
#define VMX_VMCS64_EXIT_GUEST_PHYS_ADDR_FULL                    0x2400
#define VMX_VMCS64_EXIT_GUEST_PHYS_ADDR_HIGH                    0x2401
/** @} */


/** @name VMCS field encoding - 64 Bits guest fields
 * @{
 */
#define VMX_VMCS64_GUEST_VMCS_LINK_PTR_FULL                     0x2800
#define VMX_VMCS64_GUEST_VMCS_LINK_PTR_HIGH                     0x2801
#define VMX_VMCS64_GUEST_DEBUGCTL_FULL                          0x2802      /**< MSR IA32_DEBUGCTL */
#define VMX_VMCS64_GUEST_DEBUGCTL_HIGH                          0x2803      /**< MSR IA32_DEBUGCTL */
#define VMX_VMCS64_GUEST_PAT_FULL                               0x2804
#define VMX_VMCS64_GUEST_PAT_HIGH                               0x2805
#define VMX_VMCS64_GUEST_EFER_FULL                              0x2806
#define VMX_VMCS64_GUEST_EFER_HIGH                              0x2807
#define VMX_VMCS64_GUEST_PERF_GLOBAL_CTRL_FULL                  0x2808      /**< MSR IA32_PERF_GLOBAL_CTRL */
#define VMX_VMCS64_GUEST_PERF_GLOBAL_CTRL_HIGH                  0x2809      /**< MSR IA32_PERF_GLOBAL_CTRL */
#define VMX_VMCS64_GUEST_PDPTE0_FULL                            0x280A
#define VMX_VMCS64_GUEST_PDPTE0_HIGH                            0x280B
#define VMX_VMCS64_GUEST_PDPTE1_FULL                            0x280C
#define VMX_VMCS64_GUEST_PDPTE1_HIGH                            0x280D
#define VMX_VMCS64_GUEST_PDPTE2_FULL                            0x280E
#define VMX_VMCS64_GUEST_PDPTE2_HIGH                            0x280F
#define VMX_VMCS64_GUEST_PDPTE3_FULL                            0x2810
#define VMX_VMCS64_GUEST_PDPTE3_HIGH                            0x2811
/** @} */


/** @name VMCS field encoding - 32 Bits control fields
 * @{
 */
#define VMX_VMCS32_CTRL_PIN_EXEC                                0x4000
#define VMX_VMCS32_CTRL_PROC_EXEC                               0x4002
#define VMX_VMCS32_CTRL_EXCEPTION_BITMAP                        0x4004
#define VMX_VMCS32_CTRL_PAGEFAULT_ERROR_MASK                    0x4006
#define VMX_VMCS32_CTRL_PAGEFAULT_ERROR_MATCH                   0x4008
#define VMX_VMCS32_CTRL_CR3_TARGET_COUNT                        0x400A
#define VMX_VMCS32_CTRL_EXIT                                    0x400C
#define VMX_VMCS32_CTRL_EXIT_MSR_STORE_COUNT                    0x400E
#define VMX_VMCS32_CTRL_EXIT_MSR_LOAD_COUNT                     0x4010
#define VMX_VMCS32_CTRL_ENTRY                                   0x4012
#define VMX_VMCS32_CTRL_ENTRY_MSR_LOAD_COUNT                    0x4014
#define VMX_VMCS32_CTRL_ENTRY_INTERRUPTION_INFO                 0x4016
#define VMX_VMCS32_CTRL_ENTRY_EXCEPTION_ERRCODE                 0x4018
#define VMX_VMCS32_CTRL_ENTRY_INSTR_LENGTH                      0x401A
#define VMX_VMCS32_CTRL_TPR_THRESHOLD                           0x401C
#define VMX_VMCS32_CTRL_PROC_EXEC2                              0x401E
#define VMX_VMCS32_CTRL_PLE_GAP                                 0x4020
#define VMX_VMCS32_CTRL_PLE_WINDOW                              0x4022
/** @} */


/** @name VMX_VMCS_CTRL_PIN_EXEC
 * @{
 */
/** External interrupts cause VM-exits if set; otherwise dispatched through the guest's IDT. */
#define VMX_VMCS_CTRL_PIN_EXEC_EXT_INT_EXIT                     RT_BIT(0)
/** Non-maskable interrupts cause VM-exits if set; otherwise dispatched through the guest's IDT. */
#define VMX_VMCS_CTRL_PIN_EXEC_NMI_EXIT                         RT_BIT(3)
/** Virtual NMIs. */
#define VMX_VMCS_CTRL_PIN_EXEC_VIRTUAL_NMI                      RT_BIT(5)
/** Activate VMX preemption timer. */
#define VMX_VMCS_CTRL_PIN_EXEC_PREEMPT_TIMER                    RT_BIT(6)
/** Process interrupts with the posted-interrupt notification vector. */
#define VMX_VMCS_CTRL_PIN_EXEC_POSTED_INTR                      RT_BIT(7)
/* All other bits are reserved and must be set according to MSR IA32_VMX_PROCBASED_CTLS. */
/** @} */

/** @name VMX_VMCS_CTRL_PROC_EXEC
 * @{
 */
/** VM-exit as soon as RFLAGS.IF=1 and no blocking is active. */
#define VMX_VMCS_CTRL_PROC_EXEC_INT_WINDOW_EXIT                 RT_BIT(2)
/** Use timestamp counter offset. */
#define VMX_VMCS_CTRL_PROC_EXEC_USE_TSC_OFFSETTING              RT_BIT(3)
/** VM-exit when executing the HLT instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_HLT_EXIT                        RT_BIT(7)
/** VM-exit when executing the INVLPG instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_INVLPG_EXIT                     RT_BIT(9)
/** VM-exit when executing the MWAIT instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_MWAIT_EXIT                      RT_BIT(10)
/** VM-exit when executing the RDPMC instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_RDPMC_EXIT                      RT_BIT(11)
/** VM-exit when executing the RDTSC/RDTSCP instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_RDTSC_EXIT                      RT_BIT(12)
/** VM-exit when executing the MOV to CR3 instruction. (forced to 1 on the 'first' VT-x capable CPUs; this actually includes the newest Nehalem CPUs) */
#define VMX_VMCS_CTRL_PROC_EXEC_CR3_LOAD_EXIT                   RT_BIT(15)
/** VM-exit when executing the MOV from CR3 instruction. (forced to 1 on the 'first' VT-x capable CPUs; this actually includes the newest Nehalem CPUs) */
#define VMX_VMCS_CTRL_PROC_EXEC_CR3_STORE_EXIT                  RT_BIT(16)
/** VM-exit on CR8 loads. */
#define VMX_VMCS_CTRL_PROC_EXEC_CR8_LOAD_EXIT                   RT_BIT(19)
/** VM-exit on CR8 stores. */
#define VMX_VMCS_CTRL_PROC_EXEC_CR8_STORE_EXIT                  RT_BIT(20)
/** Use TPR shadow. */
#define VMX_VMCS_CTRL_PROC_EXEC_USE_TPR_SHADOW                  RT_BIT(21)
/** VM-exit when virtual NMI blocking is disabled. */
#define VMX_VMCS_CTRL_PROC_EXEC_NMI_WINDOW_EXIT                 RT_BIT(22)
/** VM-exit when executing a MOV DRx instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_MOV_DR_EXIT                     RT_BIT(23)
/** VM-exit when executing IO instructions. */
#define VMX_VMCS_CTRL_PROC_EXEC_UNCOND_IO_EXIT                  RT_BIT(24)
/** Use IO bitmaps. */
#define VMX_VMCS_CTRL_PROC_EXEC_USE_IO_BITMAPS                  RT_BIT(25)
/** Monitor trap flag. */
#define VMX_VMCS_CTRL_PROC_EXEC_MONITOR_TRAP_FLAG               RT_BIT(27)
/** Use MSR bitmaps. */
#define VMX_VMCS_CTRL_PROC_EXEC_USE_MSR_BITMAPS                 RT_BIT(28)
/** VM-exit when executing the MONITOR instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_MONITOR_EXIT                    RT_BIT(29)
/** VM-exit when executing the PAUSE instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC_PAUSE_EXIT                      RT_BIT(30)
/** Determines whether the secondary processor based VM-execution controls are used. */
#define VMX_VMCS_CTRL_PROC_EXEC_USE_SECONDARY_EXEC_CTRL         RT_BIT(31)
/** @} */

/** @name VMX_VMCS_CTRL_PROC_EXEC2
 * @{
 */
/** Virtualize APIC access. */
#define VMX_VMCS_CTRL_PROC_EXEC2_VIRT_APIC                      RT_BIT(0)
/** EPT supported/enabled. */
#define VMX_VMCS_CTRL_PROC_EXEC2_EPT                            RT_BIT(1)
/** Descriptor table instructions cause VM-exits. */
#define VMX_VMCS_CTRL_PROC_EXEC2_DESCRIPTOR_TABLE_EXIT          RT_BIT(2)
/** RDTSCP supported/enabled. */
#define VMX_VMCS_CTRL_PROC_EXEC2_RDTSCP                         RT_BIT(3)
/** Virtualize x2APIC mode. */
#define VMX_VMCS_CTRL_PROC_EXEC2_VIRT_X2APIC                    RT_BIT(4)
/** VPID supported/enabled. */
#define VMX_VMCS_CTRL_PROC_EXEC2_VPID                           RT_BIT(5)
/** VM-exit when executing the WBINVD instruction. */
#define VMX_VMCS_CTRL_PROC_EXEC2_WBINVD_EXIT                    RT_BIT(6)
/** Unrestricted guest execution. */
#define VMX_VMCS_CTRL_PROC_EXEC2_UNRESTRICTED_GUEST             RT_BIT(7)
/** APIC register virtualization. */
#define VMX_VMCS_CTRL_PROC_EXEC2_APIC_REG_VIRT                  RT_BIT(8)
/** Virtual-interrupt delivery. */
#define VMX_VMCS_CTRL_PROC_EXEC2_VIRT_INTR_DELIVERY             RT_BIT(9)
/** A specified number of pause loops cause a VM-exit. */
#define VMX_VMCS_CTRL_PROC_EXEC2_PAUSE_LOOP_EXIT                RT_BIT(10)
/** VM-exit when executing RDRAND instructions. */
#define VMX_VMCS_CTRL_PROC_EXEC2_RDRAND_EXIT                    RT_BIT(11)
/** Enables INVPCID instructions. */
#define VMX_VMCS_CTRL_PROC_EXEC2_INVPCID                        RT_BIT(12)
/** Enables VMFUNC instructions. */
#define VMX_VMCS_CTRL_PROC_EXEC2_VMFUNC                         RT_BIT(13)
/** Enables VMCS shadowing. */
#define VMX_VMCS_CTRL_PROC_EXEC2_VMCS_SHADOWING                 RT_BIT(14)
/** Enables ENCLS VM-exits. */
#define VMX_VMCS_CTRL_PROC_EXEC2_ENCLS_EXIT                     RT_BIT(15)
/** VM-exit when executing RDSEED. */
#define VMX_VMCS_CTRL_PROC_EXEC2_RDSEED_EXIT                    RT_BIT(16)
/** Enables page-modification logging. */
#define VMX_VMCS_CTRL_PROC_EXEC2_PML                            RT_BIT(17)
/** Controls whether EPT-violations may cause \#VE instead of exits. */
#define VMX_VMCS_CTRL_PROC_EXEC2_EPT_VE                         RT_BIT(18)
/** Conceal VMX non-root operation from Intel processor trace (PT). */
#define VMX_VMCS_CTRL_PROC_EXEC2_CONCEAL_FROM_PT                RT_BIT(19)
/** Enables XSAVES/XRSTORS instructions. */
#define VMX_VMCS_CTRL_PROC_EXEC2_XSAVES_XRSTORS                 RT_BIT(20)
/** Use TSC scaling. */
#define VMX_VMCS_CTRL_PROC_EXEC2_TSC_SCALING                    RT_BIT(25)

/** @} */


/** @name VMX_VMCS_CTRL_ENTRY
 * @{
 */
/** Load guest debug controls (dr7 & IA32_DEBUGCTL_MSR) (forced to 1 on the 'first' VT-x capable CPUs; this actually includes the newest Nehalem CPUs) */
#define VMX_VMCS_CTRL_ENTRY_LOAD_DEBUG                          RT_BIT(2)
/** 64 bits guest mode. Must be 0 for CPUs that don't support AMD64. */
#define VMX_VMCS_CTRL_ENTRY_IA32E_MODE_GUEST                    RT_BIT(9)
/** In SMM mode after VM-entry. */
#define VMX_VMCS_CTRL_ENTRY_ENTRY_SMM                           RT_BIT(10)
/** Disable dual treatment of SMI and SMM; must be zero for VM-entry outside of SMM. */
#define VMX_VMCS_CTRL_ENTRY_DEACTIVATE_DUALMON                  RT_BIT(11)
/** Whether the guest IA32_PERF_GLOBAL_CTRL MSR is loaded on VM-entry. */
#define VMX_VMCS_CTRL_ENTRY_LOAD_GUEST_PERF_MSR                 RT_BIT(13)
/** Whether the guest IA32_PAT MSR is loaded on VM-entry. */
#define VMX_VMCS_CTRL_ENTRY_LOAD_GUEST_PAT_MSR                  RT_BIT(14)
/** Whether the guest IA32_EFER MSR is loaded on VM-entry. */
#define VMX_VMCS_CTRL_ENTRY_LOAD_GUEST_EFER_MSR                 RT_BIT(15)
/** @} */


/** @name VMX_VMCS_CTRL_EXIT
 * @{
 */
/** Save guest debug controls (dr7 & IA32_DEBUGCTL_MSR) (forced to 1 on the 'first' VT-x capable CPUs; this actually includes the newest Nehalem CPUs) */
#define VMX_VMCS_CTRL_EXIT_SAVE_DEBUG                           RT_BIT(2)
/** Return to long mode after a VM-exit. */
#define VMX_VMCS_CTRL_EXIT_HOST_ADDR_SPACE_SIZE                 RT_BIT(9)
/** Whether the IA32_PERF_GLOBAL_CTRL MSR is loaded on VM-exit. */
#define VMX_VMCS_CTRL_EXIT_LOAD_PERF_MSR                        RT_BIT(12)
/** Acknowledge external interrupts with the irq controller if one caused a VM-exit. */
#define VMX_VMCS_CTRL_EXIT_ACK_EXT_INT                          RT_BIT(15)
/** Whether the guest IA32_PAT MSR is saved on VM-exit. */
#define VMX_VMCS_CTRL_EXIT_SAVE_GUEST_PAT_MSR                   RT_BIT(18)
/** Whether the host IA32_PAT MSR is loaded on VM-exit. */
#define VMX_VMCS_CTRL_EXIT_LOAD_HOST_PAT_MSR                    RT_BIT(19)
/** Whether the guest IA32_EFER MSR is saved on VM-exit. */
#define VMX_VMCS_CTRL_EXIT_SAVE_GUEST_EFER_MSR                  RT_BIT(20)
/** Whether the host IA32_EFER MSR is loaded on VM-exit. */
#define VMX_VMCS_CTRL_EXIT_LOAD_HOST_EFER_MSR                   RT_BIT(21)
/** Whether the value of the VMX preemption timer is saved on every VM-exit. */
#define VMX_VMCS_CTRL_EXIT_SAVE_VMX_PREEMPT_TIMER               RT_BIT(22)
/** @} */


/** @name VMX_VMCS_CTRL_VMFUNC
 * @{
 */
/** EPTP-switching function changes the value of the EPTP to one chosen from the EPTP list. */
#define VMX_VMCS_CTRL_VMFUNC_EPTP_SWITCHING                     RT_BIT_64(0)
/** @} */


/** @name VMCS field encoding - 32 Bits read-only fields
 * @{
 */
#define VMX_VMCS32_RO_VM_INSTR_ERROR                            0x4400
#define VMX_VMCS32_RO_EXIT_REASON                               0x4402
#define VMX_VMCS32_RO_EXIT_INTERRUPTION_INFO                    0x4404
#define VMX_VMCS32_RO_EXIT_INTERRUPTION_ERROR_CODE              0x4406
#define VMX_VMCS32_RO_IDT_INFO                                  0x4408
#define VMX_VMCS32_RO_IDT_ERROR_CODE                            0x440A
#define VMX_VMCS32_RO_EXIT_INSTR_LENGTH                         0x440C
#define VMX_VMCS32_RO_EXIT_INSTR_INFO                           0x440E
/** @} */

/** @name VMX_VMCS32_RO_EXIT_REASON
 * @{
 */
#define VMX_EXIT_REASON_BASIC(a)                                ((a) & 0xffff)
/** @} */

/** @name VMX_VMCS32_CTRL_ENTRY_INTERRUPTION_INFO
 * @{
 */
#define VMX_ENTRY_INTERRUPTION_INFO_IS_VALID(a)                 RT_BOOL((a) & RT_BIT(31))
#define VMX_ENTRY_INTERRUPTION_INFO_TYPE_SHIFT                  8
#define VMX_ENTRY_INTERRUPTION_INFO_TYPE(a)                     ((a >> VMX_ENTRY_INTERRUPTION_INFO_TYPE_SHIFT) & 7)
/** @} */


/** @name VMX_VMCS32_RO_EXIT_INTERRUPTION_INFO
 * @{
 */
#define VMX_EXIT_INTERRUPTION_INFO_VECTOR(a)                    ((a) & 0xff)
#define VMX_EXIT_INTERRUPTION_INFO_TYPE_SHIFT                   8
#define VMX_EXIT_INTERRUPTION_INFO_TYPE(a)                      (((a) >> VMX_EXIT_INTERRUPTION_INFO_TYPE_SHIFT) & 7)
#define VMX_EXIT_INTERRUPTION_INFO_ERROR_CODE_VALID             RT_BIT(11)
#define VMX_EXIT_INTERRUPTION_INFO_ERROR_CODE_IS_VALID(a)       RT_BOOL((a) & VMX_EXIT_INTERRUPTION_INFO_ERROR_CODE_VALID)
#define VMX_EXIT_INTERRUPTION_INFO_NMI_UNBLOCK_IRET(a)          ((a) & RT_BIT(12))
#define VMX_EXIT_INTERRUPTION_INFO_VALID                        RT_BIT(31)
#define VMX_EXIT_INTERRUPTION_INFO_IS_VALID(a)                  RT_BOOL((a) & RT_BIT(31))
/** Construct an irq event injection value from the exit interruption info value (same except that bit 12 is reserved). */
#define VMX_VMCS_CTRL_ENTRY_IRQ_INFO_FROM_EXIT_INT_INFO(a)      ((a) & ~RT_BIT(12))
/** @} */

/** @name VMX_VMCS_RO_EXIT_INTERRUPTION_INFO_TYPE
 * @{
 */
#define VMX_EXIT_INTERRUPTION_INFO_TYPE_EXT_INT                 0
#define VMX_EXIT_INTERRUPTION_INFO_TYPE_NMI                     2
#define VMX_EXIT_INTERRUPTION_INFO_TYPE_HW_XCPT                 3
#define VMX_EXIT_INTERRUPTION_INFO_TYPE_SW_INT                  4
#define VMX_EXIT_INTERRUPTION_INFO_TYPE_PRIV_SW_XCPT            5
#define VMX_EXIT_INTERRUPTION_INFO_TYPE_SW_XCPT                 6
/** @} */

/** @name VMX_VMCS32_RO_IDT_VECTORING_INFO
 * @{
 */
#define VMX_IDT_VECTORING_INFO_VECTOR(a)                        ((a) & 0xff)
#define VMX_IDT_VECTORING_INFO_TYPE_SHIFT                       8
#define VMX_IDT_VECTORING_INFO_TYPE(a)                          (((a) >> VMX_IDT_VECTORING_INFO_TYPE_SHIFT) & 7)
#define VMX_IDT_VECTORING_INFO_ERROR_CODE_VALID                 RT_BIT(11)
#define VMX_IDT_VECTORING_INFO_ERROR_CODE_IS_VALID(a)           RT_BOOL((a) & VMX_IDT_VECTORING_INFO_ERROR_CODE_VALID)
#define VMX_IDT_VECTORING_INFO_VALID(a)                         ((a) & RT_BIT(31))
#define VMX_ENTRY_INT_INFO_FROM_EXIT_IDT_INFO(a)                ((a) & ~RT_BIT(12))
/** @} */

/** @name VMX_VMCS_RO_IDT_VECTORING_INFO_TYPE
 * @{
 */
#define VMX_IDT_VECTORING_INFO_TYPE_EXT_INT                     0
#define VMX_IDT_VECTORING_INFO_TYPE_NMI                         2
#define VMX_IDT_VECTORING_INFO_TYPE_HW_XCPT                     3
#define VMX_IDT_VECTORING_INFO_TYPE_SW_INT                      4
#define VMX_IDT_VECTORING_INFO_TYPE_PRIV_SW_XCPT                5
#define VMX_IDT_VECTORING_INFO_TYPE_SW_XCPT                     6
/** @} */


/** @name VMCS field encoding - 32 Bits guest state fields
 * @{
 */
#define VMX_VMCS32_GUEST_ES_LIMIT                               0x4800
#define VMX_VMCS32_GUEST_CS_LIMIT                               0x4802
#define VMX_VMCS32_GUEST_SS_LIMIT                               0x4804
#define VMX_VMCS32_GUEST_DS_LIMIT                               0x4806
#define VMX_VMCS32_GUEST_FS_LIMIT                               0x4808
#define VMX_VMCS32_GUEST_GS_LIMIT                               0x480A
#define VMX_VMCS32_GUEST_LDTR_LIMIT                             0x480C
#define VMX_VMCS32_GUEST_TR_LIMIT                               0x480E
#define VMX_VMCS32_GUEST_GDTR_LIMIT                             0x4810
#define VMX_VMCS32_GUEST_IDTR_LIMIT                             0x4812
#define VMX_VMCS32_GUEST_ES_ACCESS_RIGHTS                       0x4814
#define VMX_VMCS32_GUEST_CS_ACCESS_RIGHTS                       0x4816
#define VMX_VMCS32_GUEST_SS_ACCESS_RIGHTS                       0x4818
#define VMX_VMCS32_GUEST_DS_ACCESS_RIGHTS                       0x481A
#define VMX_VMCS32_GUEST_FS_ACCESS_RIGHTS                       0x481C
#define VMX_VMCS32_GUEST_GS_ACCESS_RIGHTS                       0x481E
#define VMX_VMCS32_GUEST_LDTR_ACCESS_RIGHTS                     0x4820
#define VMX_VMCS32_GUEST_TR_ACCESS_RIGHTS                       0x4822
#define VMX_VMCS32_GUEST_INTERRUPTIBILITY_STATE                 0x4824
#define VMX_VMCS32_GUEST_ACTIVITY_STATE                         0x4826
#define VMX_VMCS32_GUEST_SYSENTER_CS                            0x482A  /**< MSR IA32_SYSENTER_CS */
#define VMX_VMCS32_GUEST_PREEMPT_TIMER_VALUE                    0x482E
/** @} */


/** @name VMX_VMCS_GUEST_ACTIVITY_STATE
 * @{
 */
/** The logical processor is active. */
#define VMX_VMCS_GUEST_ACTIVITY_ACTIVE                          0x0
/** The logical processor is inactive, because executed a HLT instruction. */
#define VMX_VMCS_GUEST_ACTIVITY_HLT                             0x1
/** The logical processor is inactive, because of a triple fault or other serious error. */
#define VMX_VMCS_GUEST_ACTIVITY_SHUTDOWN                        0x2
/** The logical processor is inactive, because it's waiting for a startup-IPI */
#define VMX_VMCS_GUEST_ACTIVITY_SIPI_WAIT                       0x3
/** @} */


/** @name VMX_VMCS_GUEST_INTERRUPTIBILITY_STATE
 * @{
 */
#define VMX_VMCS_GUEST_INTERRUPTIBILITY_STATE_BLOCK_STI         RT_BIT(0)
#define VMX_VMCS_GUEST_INTERRUPTIBILITY_STATE_BLOCK_MOVSS       RT_BIT(1)
#define VMX_VMCS_GUEST_INTERRUPTIBILITY_STATE_BLOCK_SMI         RT_BIT(2)
#define VMX_VMCS_GUEST_INTERRUPTIBILITY_STATE_BLOCK_NMI         RT_BIT(3)
/** @} */


/** @name VMCS field encoding - 32 Bits host state fields
 * @{
 */
#define VMX_VMCS32_HOST_SYSENTER_CS                             0x4C00
/** @} */

/** @name Natural width control fields
 * @{
 */
#define VMX_VMCS_CTRL_CR0_MASK                                  0x6000
#define VMX_VMCS_CTRL_CR4_MASK                                  0x6002
#define VMX_VMCS_CTRL_CR0_READ_SHADOW                           0x6004
#define VMX_VMCS_CTRL_CR4_READ_SHADOW                           0x6006
#define VMX_VMCS_CTRL_CR3_TARGET_VAL0                           0x6008
#define VMX_VMCS_CTRL_CR3_TARGET_VAL1                           0x600A
#define VMX_VMCS_CTRL_CR3_TARGET_VAL2                           0x600C
#define VMX_VMCS_CTRL_CR3_TARGET_VAL31                          0x600E
/** @} */


/** @name Natural width read-only data fields
 * @{
 */
#define VMX_VMCS_RO_EXIT_QUALIFICATION                          0x6400
#define VMX_VMCS_RO_IO_RCX                                      0x6402
#define VMX_VMCS_RO_IO_RSX                                      0x6404
#define VMX_VMCS_RO_IO_RDI                                      0x6406
#define VMX_VMCS_RO_IO_RIP                                      0x6408
#define VMX_VMCS_RO_EXIT_GUEST_LINEAR_ADDR                      0x640A
/** @} */


/** @name VMX_VMCS_RO_EXIT_QUALIFICATION
 * @{
 */
/** 0-2:  Debug register number */
#define VMX_EXIT_QUALIFICATION_DRX_REGISTER(a)                  ((a) & 7)
/** 3:    Reserved; cleared to 0. */
#define VMX_EXIT_QUALIFICATION_DRX_RES1(a)                      (((a) >> 3) & 1)
/** 4:    Direction of move (0 = write, 1 = read) */
#define VMX_EXIT_QUALIFICATION_DRX_DIRECTION(a)                 (((a) >> 4) & 1)
/** 5-7:  Reserved; cleared to 0. */
#define VMX_EXIT_QUALIFICATION_DRX_RES2(a)                      (((a) >> 5) & 7)
/** 8-11: General purpose register number. */
#define VMX_EXIT_QUALIFICATION_DRX_GENREG(a)                    (((a) >> 8) & 0xF)
/** Rest: reserved. */
/** @} */

/** @name VMX_EXIT_QUALIFICATION_DRX_DIRECTION values
 * @{
 */
#define VMX_EXIT_QUALIFICATION_DRX_DIRECTION_WRITE              0
#define VMX_EXIT_QUALIFICATION_DRX_DIRECTION_READ               1
/** @} */



/** @name CRx accesses
 * @{
 */
/** 0-3:   Control register number (0 for CLTS & LMSW) */
#define VMX_EXIT_QUALIFICATION_CRX_REGISTER(a)                  ((a) & 0xF)
/** 4-5:   Access type. */
#define VMX_EXIT_QUALIFICATION_CRX_ACCESS(a)                    (((a) >> 4) & 3)
/** 6:     LMSW operand type */
#define VMX_EXIT_QUALIFICATION_CRX_LMSW_OP(a)                   (((a) >> 6) & 1)
/** 7:     Reserved; cleared to 0. */
#define VMX_EXIT_QUALIFICATION_CRX_RES1(a)                      (((a) >> 7) & 1)
/** 8-11:  General purpose register number (0 for CLTS & LMSW). */
#define VMX_EXIT_QUALIFICATION_CRX_GENREG(a)                    (((a) >> 8) & 0xF)
/** 12-15: Reserved; cleared to 0. */
#define VMX_EXIT_QUALIFICATION_CRX_RES2(a)                      (((a) >> 12) & 0xF)
/** 16-31: LMSW source data (else 0). */
#define VMX_EXIT_QUALIFICATION_CRX_LMSW_DATA(a)                 (((a) >> 16) & 0xFFFF)
/* Rest: reserved. */
/** @} */

/** @name VMX_EXIT_QUALIFICATION_CRX_ACCESS
 * @{
 */
#define VMX_EXIT_QUALIFICATION_CRX_ACCESS_WRITE                 0
#define VMX_EXIT_QUALIFICATION_CRX_ACCESS_READ                  1
#define VMX_EXIT_QUALIFICATION_CRX_ACCESS_CLTS                  2
#define VMX_EXIT_QUALIFICATION_CRX_ACCESS_LMSW                  3
/** @} */

/** @name VMX_EXIT_QUALIFICATION_TASK_SWITCH
 * @{
 */
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_SELECTOR(a)          ((a) & 0xffff)
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_TYPE(a)              (((a) >> 30) & 0x3)
/** Task switch caused by a call instruction. */
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_TYPE_CALL            0
/** Task switch caused by an iret instruction. */
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_TYPE_IRET            1
/** Task switch caused by a jmp instruction. */
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_TYPE_JMP             2
/** Task switch caused by an interrupt gate. */
#define VMX_EXIT_QUALIFICATION_TASK_SWITCH_TYPE_IDT             3
/** @} */


/** @name VMX_EXIT_EPT_VIOLATION
 * @{
 */
/** Set if the violation was caused by a data read. */
#define VMX_EXIT_QUALIFICATION_EPT_DATA_READ                    RT_BIT(0)
/** Set if the violation was caused by a data write. */
#define VMX_EXIT_QUALIFICATION_EPT_DATA_WRITE                   RT_BIT(1)
/** Set if the violation was caused by an instruction fetch. */
#define VMX_EXIT_QUALIFICATION_EPT_INSTR_FETCH                  RT_BIT(2)
/** AND of the present bit of all EPT structures. */
#define VMX_EXIT_QUALIFICATION_EPT_ENTRY_PRESENT                RT_BIT(3)
/** AND of the write bit of all EPT structures. */
#define VMX_EXIT_QUALIFICATION_EPT_ENTRY_WRITE                  RT_BIT(4)
/** AND of the execute bit of all EPT structures. */
#define VMX_EXIT_QUALIFICATION_EPT_ENTRY_EXECUTE                RT_BIT(5)
/** Set if the guest linear address field contains the faulting address. */
#define VMX_EXIT_QUALIFICATION_EPT_GUEST_ADDR_VALID             RT_BIT(7)
/** If bit 7 is one: (reserved otherwise)
 *  1 - violation due to physical address access.
 *  0 - violation caused by page walk or access/dirty bit updates
 */
#define VMX_EXIT_QUALIFICATION_EPT_TRANSLATED_ACCESS            RT_BIT(8)
/** @} */


/** @name VMX_EXIT_PORT_IO
 * @{
 */
/** 0-2:   IO operation width. */
#define VMX_EXIT_QUALIFICATION_IO_WIDTH(a)                      ((a) & 7)
/** 3:     IO operation direction. */
#define VMX_EXIT_QUALIFICATION_IO_DIRECTION(a)                  (((a) >> 3) & 1)
/** 4:     String IO operation (INS / OUTS). */
#define VMX_EXIT_QUALIFICATION_IO_IS_STRING(a)                  RT_BOOL((a) & RT_BIT_64(4))
/** 5:     Repeated IO operation. */
#define VMX_EXIT_QUALIFICATION_IO_IS_REP(a)                     RT_BOOL((a) & RT_BIT_64(5))
/** 6:     Operand encoding. */
#define VMX_EXIT_QUALIFICATION_IO_ENCODING(a)                   (((a) >> 6) & 1)
/** 16-31: IO Port (0-0xffff). */
#define VMX_EXIT_QUALIFICATION_IO_PORT(a)                       (((a) >> 16) & 0xffff)
/* Rest reserved. */
/** @} */

/** @name VMX_EXIT_QUALIFICATION_IO_DIRECTION
 * @{
 */
#define VMX_EXIT_QUALIFICATION_IO_DIRECTION_OUT                 0
#define VMX_EXIT_QUALIFICATION_IO_DIRECTION_IN                  1
/** @} */


/** @name VMX_EXIT_QUALIFICATION_IO_ENCODING
 * @{
 */
#define VMX_EXIT_QUALIFICATION_IO_ENCODING_DX                   0
#define VMX_EXIT_QUALIFICATION_IO_ENCODING_IMM                  1
/** @} */

/** @name VMX_EXIT_APIC_ACCESS
 * @{
 */
/** 0-11:   If the APIC-access VM-exit is due to a linear access, the offset of access within the APIC page. */
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_OFFSET(a)            ((a) & 0xfff)
/** 12-15:  Access type. */
#define VMX_EXIT_QUALIFICATION_APIC_ACCESS_TYPE(a)              (((a) & 0xf000) >> 12)
/* Rest reserved. */
/** @} */


/** @name VMX_EXIT_QUALIFICATION_APIC_ACCESS_TYPE return values
 * @{
 */
/** Linear read access. */
#define VMX_APIC_ACCESS_TYPE_LINEAR_READ                        0
/** Linear write access. */
#define VMX_APIC_ACCESS_TYPE_LINEAR_WRITE                       1
/** Linear instruction fetch access. */
#define VMX_APIC_ACCESS_TYPE_LINEAR_INSTR_FETCH                 2
/** Linear read/write access during event delivery. */
#define VMX_APIC_ACCESS_TYPE_LINEAR_EVENT_DELIVERY              3
/** Physical read/write access during event delivery. */
#define VMX_APIC_ACCESS_TYPE_PHYSICAL_EVENT_DELIVERY            10
/** Physical access for an instruction fetch or during instruction execution. */
#define VMX_APIC_ACCESS_TYPE_PHYSICAL_INSTR                     15
/** @} */

/** @name VMX_XDTR_INSINFO_XXX - VMX_EXIT_XDTR_ACCESS instruction information
 * Found in VMX_VMCS32_RO_EXIT_INSTR_INFO.
 * @{
 */
/** Address calculation scaling field (powers of two). */
#define VMX_XDTR_INSINFO_SCALE_SHIFT                            0
#define VMX_XDTR_INSINFO_SCALE_MASK                             UINT32_C(0x00000003)
/** Bits 2 thru 6 are undefined. */
#define VMX_XDTR_INSINFO_UNDEF_2_6_SHIFT                        2
#define VMX_XDTR_INSINFO_UNDEF_2_6_MASK                         UINT32_C(0x0000007c)
/** Address size, only 0(=16), 1(=32) and 2(=64) are defined.
 * @remarks anyone's guess why this is a 3 bit field...  */
#define VMX_XDTR_INSINFO_ADDR_SIZE_SHIFT                        7
#define VMX_XDTR_INSINFO_ADDR_SIZE_MASK                         UINT32_C(0x00000380)
/** Bit 10 is defined as zero. */
#define VMX_XDTR_INSINFO_ZERO_10_SHIFT                          10
#define VMX_XDTR_INSINFO_ZERO_10_MASK                           UINT32_C(0x00000400)
/** Operand size, either (1=)32-bit or (0=)16-bit, but get this, it's undefined
 * for exits from 64-bit code as the operand size there is fixed. */
#define VMX_XDTR_INSINFO_OP_SIZE_SHIFT                          11
#define VMX_XDTR_INSINFO_OP_SIZE_MASK                           UINT32_C(0x00000800)
/** Bits 12 thru 14 are undefined. */
#define VMX_XDTR_INSINFO_UNDEF_12_14_SHIFT                      12
#define VMX_XDTR_INSINFO_UNDEF_12_14_MASK                       UINT32_C(0x00007000)
/** Applicable segment register (X86_SREG_XXX values). */
#define VMX_XDTR_INSINFO_SREG_SHIFT                             15
#define VMX_XDTR_INSINFO_SREG_MASK                              UINT32_C(0x00038000)
/** Index register (X86_GREG_XXX values). Undefined if HAS_INDEX_REG is clear. */
#define VMX_XDTR_INSINFO_INDEX_REG_SHIFT                        18
#define VMX_XDTR_INSINFO_INDEX_REG_MASK                         UINT32_C(0x003c0000)
/** Is VMX_XDTR_INSINFO_INDEX_REG_XXX valid (=1) or not (=0). */
#define VMX_XDTR_INSINFO_HAS_INDEX_REG_SHIFT                    22
#define VMX_XDTR_INSINFO_HAS_INDEX_REG_MASK                     UINT32_C(0x00400000)
/** Base register (X86_GREG_XXX values). Undefined if HAS_BASE_REG is clear. */
#define VMX_XDTR_INSINFO_BASE_REG_SHIFT                         23
#define VMX_XDTR_INSINFO_BASE_REG_MASK                          UINT32_C(0x07800000)
/** Is VMX_XDTR_INSINFO_BASE_REG_XXX valid (=1) or not (=0). */
#define VMX_XDTR_INSINFO_HAS_BASE_REG_SHIFT                     27
#define VMX_XDTR_INSINFO_HAS_BASE_REG_MASK                      UINT32_C(0x08000000)
/** The instruction identity (VMX_XDTR_INSINFO_II_XXX values) */
#define VMX_XDTR_INSINFO_INSTR_ID_SHIFT                         28
#define VMX_XDTR_INSINFO_INSTR_ID_MASK                          UINT32_C(0x30000000)
#define VMX_XDTR_INSINFO_II_SGDT                                0 /**< Instruction ID: SGDT */
#define VMX_XDTR_INSINFO_II_SIDT                                1 /**< Instruction ID: SIDT */
#define VMX_XDTR_INSINFO_II_LGDT                                2 /**< Instruction ID: LGDT */
#define VMX_XDTR_INSINFO_II_LIDT                                3 /**< Instruction ID: LIDT */
/** Bits 30 & 31 are undefined. */
#define VMX_XDTR_INSINFO_UNDEF_30_31_SHIFT                      30
#define VMX_XDTR_INSINFO_UNDEF_30_31_MASK                       UINT32_C(0xc0000000)
RT_BF_ASSERT_COMPILE_CHECKS(VMX_XDTR_INSINFO_, UINT32_C(0), UINT32_MAX,
                            (SCALE, UNDEF_2_6, ADDR_SIZE, ZERO_10, OP_SIZE, UNDEF_12_14, SREG, INDEX_REG, HAS_INDEX_REG,
                             BASE_REG, HAS_BASE_REG, INSTR_ID, UNDEF_30_31));
/** @} */


/** @name VMX_YYTR_INSINFO_XXX - VMX_EXIT_TR_ACCESS instruction information
 * Found in VMX_VMCS32_RO_EXIT_INSTR_INFO.
 * This is similar to VMX_XDTR_INSINFO_XXX.
 * @{
 */
/** Address calculation scaling field (powers of two). */
#define VMX_YYTR_INSINFO_SCALE_SHIFT                            0
#define VMX_YYTR_INSINFO_SCALE_MASK                             UINT32_C(0x00000003)
/** Bit 2 is undefined. */
#define VMX_YYTR_INSINFO_UNDEF_2_SHIFT                          2
#define VMX_YYTR_INSINFO_UNDEF_2_MASK                           UINT32_C(0x00000004)
/** Register operand 1. Undefined if VMX_YYTR_INSINFO_HAS_REG1 is clear. */
#define VMX_YYTR_INSINFO_REG1_SHIFT                             3
#define VMX_YYTR_INSINFO_REG1_MASK                              UINT32_C(0x00000078)
/** Address size, only 0(=16), 1(=32) and 2(=64) are defined.
 * @remarks anyone's guess why this is a 3 bit field...  */
#define VMX_YYTR_INSINFO_ADDR_SIZE_SHIFT                        7
#define VMX_YYTR_INSINFO_ADDR_SIZE_MASK                         UINT32_C(0x00000380)
/** Is VMX_YYTR_INSINFO_REG1_XXX valid (=1) or not (=0). */
#define VMX_YYTR_INSINFO_HAS_REG1_SHIFT                         10
#define VMX_YYTR_INSINFO_HAS_REG1_MASK                          UINT32_C(0x00000400)
/** Bits 11 thru 14 are undefined. */
#define VMX_YYTR_INSINFO_UNDEF_11_14_SHIFT                      11
#define VMX_YYTR_INSINFO_UNDEF_11_14_MASK                       UINT32_C(0x00007800)
/** Applicable segment register (X86_SREG_XXX values). */
#define VMX_YYTR_INSINFO_SREG_SHIFT                             15
#define VMX_YYTR_INSINFO_SREG_MASK                              UINT32_C(0x00038000)
/** Index register (X86_GREG_XXX values). Undefined if HAS_INDEX_REG is clear. */
#define VMX_YYTR_INSINFO_INDEX_REG_SHIFT                        18
#define VMX_YYTR_INSINFO_INDEX_REG_MASK                         UINT32_C(0x003c0000)
/** Is VMX_YYTR_INSINFO_INDEX_REG_XXX valid (=1) or not (=0). */
#define VMX_YYTR_INSINFO_HAS_INDEX_REG_SHIFT                    22
#define VMX_YYTR_INSINFO_HAS_INDEX_REG_MASK                     UINT32_C(0x00400000)
/** Base register (X86_GREG_XXX values). Undefined if HAS_BASE_REG is clear. */
#define VMX_YYTR_INSINFO_BASE_REG_SHIFT                         23
#define VMX_YYTR_INSINFO_BASE_REG_MASK                          UINT32_C(0x07800000)
/** Is VMX_YYTR_INSINFO_BASE_REG_XXX valid (=1) or not (=0). */
#define VMX_YYTR_INSINFO_HAS_BASE_REG_SHIFT                     27
#define VMX_YYTR_INSINFO_HAS_BASE_REG_MASK                      UINT32_C(0x08000000)
/** The instruction identity (VMX_YYTR_INSINFO_II_XXX values) */
#define VMX_YYTR_INSINFO_INSTR_ID_SHIFT                         28
#define VMX_YYTR_INSINFO_INSTR_ID_MASK                          UINT32_C(0x30000000)
#define VMX_YYTR_INSINFO_II_SLDT                                0 /**< Instruction ID: SLDT */
#define VMX_YYTR_INSINFO_II_STR                                 1 /**< Instruction ID: STR */
#define VMX_YYTR_INSINFO_II_LLDT                                2 /**< Instruction ID: LLDT */
#define VMX_YYTR_INSINFO_II_LTR                                 3 /**< Instruction ID: LTR */
/** Bits 30 & 31 are undefined. */
#define VMX_YYTR_INSINFO_UNDEF_30_31_SHIFT                      30
#define VMX_YYTR_INSINFO_UNDEF_30_31_MASK                       UINT32_C(0xc0000000)
RT_BF_ASSERT_COMPILE_CHECKS(VMX_YYTR_INSINFO_, UINT32_C(0), UINT32_MAX,
                            (SCALE, UNDEF_2, REG1, ADDR_SIZE, HAS_REG1, UNDEF_11_14, SREG, INDEX_REG, HAS_INDEX_REG,
                             BASE_REG, HAS_BASE_REG, INSTR_ID, UNDEF_30_31));
/** @} */


/** @name VMCS field encoding - Natural width guest state fields
 * @{
 */
#define VMX_VMCS_GUEST_CR0                                      0x6800
#define VMX_VMCS_GUEST_CR3                                      0x6802
#define VMX_VMCS_GUEST_CR4                                      0x6804
#define VMX_VMCS_GUEST_ES_BASE                                  0x6806
#define VMX_VMCS_GUEST_CS_BASE                                  0x6808
#define VMX_VMCS_GUEST_SS_BASE                                  0x680A
#define VMX_VMCS_GUEST_DS_BASE                                  0x680C
#define VMX_VMCS_GUEST_FS_BASE                                  0x680E
#define VMX_VMCS_GUEST_GS_BASE                                  0x6810
#define VMX_VMCS_GUEST_LDTR_BASE                                0x6812
#define VMX_VMCS_GUEST_TR_BASE                                  0x6814
#define VMX_VMCS_GUEST_GDTR_BASE                                0x6816
#define VMX_VMCS_GUEST_IDTR_BASE                                0x6818
#define VMX_VMCS_GUEST_DR7                                      0x681A
#define VMX_VMCS_GUEST_RSP                                      0x681C
#define VMX_VMCS_GUEST_RIP                                      0x681E
#define VMX_VMCS_GUEST_RFLAGS                                   0x6820
#define VMX_VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS                 0x6822
#define VMX_VMCS_GUEST_SYSENTER_ESP                             0x6824  /**< MSR IA32_SYSENTER_ESP */
#define VMX_VMCS_GUEST_SYSENTER_EIP                             0x6826  /**< MSR IA32_SYSENTER_EIP */
/** @} */


/** @name VMX_VMCS_GUEST_DEBUG_EXCEPTIONS
 * Bits 4-11, 13 and 15-63 are reserved.
 * @{
 */
/** Hardware breakpoint 0 was met. */
#define VMX_VMCS_GUEST_DEBUG_EXCEPTIONS_B0                      RT_BIT(0)
/** Hardware breakpoint 1 was met. */
#define VMX_VMCS_GUEST_DEBUG_EXCEPTIONS_B1                      RT_BIT(1)
/** Hardware breakpoint 2 was met. */
#define VMX_VMCS_GUEST_DEBUG_EXCEPTIONS_B2                      RT_BIT(2)
/** Hardware breakpoint 3 was met. */
#define VMX_VMCS_GUEST_DEBUG_EXCEPTIONS_B3                      RT_BIT(3)
/** At least one data or IO breakpoint was hit. */
#define VMX_VMCS_GUEST_DEBUG_EXCEPTIONS_BREAKPOINT_ENABLED      RT_BIT(12)
/** A debug exception would have been triggered by single-step execution mode. */
#define VMX_VMCS_GUEST_DEBUG_EXCEPTIONS_BS                      RT_BIT(14)
/** @} */

/** @name VMCS field encoding - Natural width host state fields
 * @{
 */
#define VMX_VMCS_HOST_CR0                                       0x6C00
#define VMX_VMCS_HOST_CR3                                       0x6C02
#define VMX_VMCS_HOST_CR4                                       0x6C04
#define VMX_VMCS_HOST_FS_BASE                                   0x6C06
#define VMX_VMCS_HOST_GS_BASE                                   0x6C08
#define VMX_VMCS_HOST_TR_BASE                                   0x6C0A
#define VMX_VMCS_HOST_GDTR_BASE                                 0x6C0C
#define VMX_VMCS_HOST_IDTR_BASE                                 0x6C0E
#define VMX_VMCS_HOST_SYSENTER_ESP                              0x6C10
#define VMX_VMCS_HOST_SYSENTER_EIP                              0x6C12
#define VMX_VMCS_HOST_RSP                                       0x6C14
#define VMX_VMCS_HOST_RIP                                       0x6C16
/** @} */


/** @defgroup grp_hm_vmx_asm    VMX Assembly Helpers
 * @{
 */

/**
 * Restores some host-state fields that need not be done on every VM-exit.
 *
 * @returns VBox status code.
 * @param   fRestoreHostFlags   Flags of which host registers needs to be
 *                              restored.
 * @param   pRestoreHost        Pointer to the host-restore structure.
 */
DECLASM(int) VMXRestoreHostState(uint32_t fRestoreHostFlags, PVMXRESTOREHOST pRestoreHost);


/**
 * Dispatches an NMI to the host.
 */
DECLASM(int) VMXDispatchHostNmi(void);


/**
 * Executes VMXON.
 *
 * @returns VBox status code.
 * @param   HCPhysVmxOn      Physical address of VMXON structure.
 */
#if ((RT_INLINE_ASM_EXTERNAL || !defined(RT_ARCH_X86)) && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXEnable(RTHCPHYS HCPhysVmxOn);
#else
DECLINLINE(int) VMXEnable(RTHCPHYS HCPhysVmxOn)
{
# if RT_INLINE_ASM_GNU_STYLE
    int rc = VINF_SUCCESS;
    __asm__ __volatile__ (
       "push     %3                                             \n\t"
       "push     %2                                             \n\t"
       ".byte    0xF3, 0x0F, 0xC7, 0x34, 0x24  # VMXON [esp]    \n\t"
       "ja       2f                                             \n\t"
       "je       1f                                             \n\t"
       "movl     $" RT_XSTR(VERR_VMX_INVALID_VMXON_PTR)", %0    \n\t"
       "jmp      2f                                             \n\t"
       "1:                                                      \n\t"
       "movl     $" RT_XSTR(VERR_VMX_VMXON_FAILED)", %0         \n\t"
       "2:                                                      \n\t"
       "add      $8, %%esp                                      \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"((uint32_t)HCPhysVmxOn),        /* don't allow direct memory reference here, */
        "ir"((uint32_t)(HCPhysVmxOn >> 32)) /* this would not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;

# elif VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc = __vmx_on(&HCPhysVmxOn);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMXON_PTR : VERR_VMX_VMXON_FAILED;

# else
    int rc = VINF_SUCCESS;
    __asm
    {
        push    dword ptr [HCPhysVmxOn + 4]
        push    dword ptr [HCPhysVmxOn]
        _emit   0xF3
        _emit   0x0F
        _emit   0xC7
        _emit   0x34
        _emit   0x24     /* VMXON [esp] */
        jnc     vmxon_good
        mov     dword ptr [rc], VERR_VMX_INVALID_VMXON_PTR
        jmp     the_end

vmxon_good:
        jnz     the_end
        mov     dword ptr [rc], VERR_VMX_VMXON_FAILED
the_end:
        add     esp, 8
    }
    return rc;
# endif
}
#endif


/**
 * Executes VMXOFF.
 */
#if ((RT_INLINE_ASM_EXTERNAL || !defined(RT_ARCH_X86)) && !VMX_USE_MSC_INTRINSICS)
DECLASM(void) VMXDisable(void);
#else
DECLINLINE(void) VMXDisable(void)
{
# if RT_INLINE_ASM_GNU_STYLE
    __asm__ __volatile__ (
       ".byte 0x0F, 0x01, 0xC4  # VMXOFF                        \n\t"
       );

# elif VMX_USE_MSC_INTRINSICS
    __vmx_off();

# else
    __asm
    {
        _emit   0x0F
        _emit   0x01
        _emit   0xC4   /* VMXOFF */
    }
# endif
}
#endif


/**
 * Executes VMCLEAR.
 *
 * @returns VBox status code.
 * @param   HCPhysVmcs       Physical address of VM control structure.
 */
#if ((RT_INLINE_ASM_EXTERNAL || !defined(RT_ARCH_X86)) && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXClearVmcs(RTHCPHYS HCPhysVmcs);
#else
DECLINLINE(int) VMXClearVmcs(RTHCPHYS HCPhysVmcs)
{
# if RT_INLINE_ASM_GNU_STYLE
    int rc = VINF_SUCCESS;
    __asm__ __volatile__ (
       "push    %3                                              \n\t"
       "push    %2                                              \n\t"
       ".byte   0x66, 0x0F, 0xC7, 0x34, 0x24  # VMCLEAR [esp]   \n\t"
       "jnc     1f                                              \n\t"
       "movl    $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0      \n\t"
       "1:                                                      \n\t"
       "add     $8, %%esp                                       \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"((uint32_t)HCPhysVmcs),        /* don't allow direct memory reference here, */
        "ir"((uint32_t)(HCPhysVmcs >> 32)) /* this would not work with -fomit-frame-pointer */
       :"memory"
       );
    return rc;

# elif VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc = __vmx_vmclear(&HCPhysVmcs);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return VERR_VMX_INVALID_VMCS_PTR;

# else
    int rc = VINF_SUCCESS;
    __asm
    {
        push    dword ptr [HCPhysVmcs + 4]
        push    dword ptr [HCPhysVmcs]
        _emit   0x66
        _emit   0x0F
        _emit   0xC7
        _emit   0x34
        _emit   0x24     /* VMCLEAR [esp] */
        jnc     success
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR
success:
        add     esp, 8
    }
    return rc;
# endif
}
#endif


/**
 * Executes VMPTRLD.
 *
 * @returns VBox status code.
 * @param   HCPhysVmcs       Physical address of VMCS structure.
 */
#if ((RT_INLINE_ASM_EXTERNAL || !defined(RT_ARCH_X86)) && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXActivateVmcs(RTHCPHYS HCPhysVmcs);
#else
DECLINLINE(int) VMXActivateVmcs(RTHCPHYS HCPhysVmcs)
{
# if RT_INLINE_ASM_GNU_STYLE
    int rc = VINF_SUCCESS;
    __asm__ __volatile__ (
       "push    %3                                              \n\t"
       "push    %2                                              \n\t"
       ".byte   0x0F, 0xC7, 0x34, 0x24  # VMPTRLD [esp]         \n\t"
       "jnc     1f                                              \n\t"
       "movl    $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0      \n\t"
       "1:                                                      \n\t"
       "add     $8, %%esp                                       \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "ir"((uint32_t)HCPhysVmcs),        /* don't allow direct memory reference here, */
        "ir"((uint32_t)(HCPhysVmcs >> 32)) /* this will not work with -fomit-frame-pointer */
       );
    return rc;

# elif VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc = __vmx_vmptrld(&HCPhysVmcs);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return VERR_VMX_INVALID_VMCS_PTR;

# else
    int rc = VINF_SUCCESS;
    __asm
    {
        push    dword ptr [HCPhysVmcs + 4]
        push    dword ptr [HCPhysVmcs]
        _emit   0x0F
        _emit   0xC7
        _emit   0x34
        _emit   0x24     /* VMPTRLD [esp] */
        jnc     success
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR

success:
        add     esp, 8
    }
    return rc;
# endif
}
#endif

/**
 * Executes VMPTRST.
 *
 * @returns VBox status code.
 * @param   pHCPhysVmcs    Where to store the physical address of the current
 *                         VMCS.
 */
DECLASM(int) VMXGetActivatedVmcs(RTHCPHYS *pHCPhysVmcs);

/**
 * Executes VMWRITE.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   idxField        VMCS index.
 * @param   u32Val          32-bit value.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if ((RT_INLINE_ASM_EXTERNAL || !defined(RT_ARCH_X86)) && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXWriteVmcs32(uint32_t idxField, uint32_t u32Val);
#else
DECLINLINE(int) VMXWriteVmcs32(uint32_t idxField, uint32_t u32Val)
{
# if RT_INLINE_ASM_GNU_STYLE
    int rc = VINF_SUCCESS;
    __asm__ __volatile__ (
       ".byte  0x0F, 0x79, 0xC2        # VMWRITE eax, edx       \n\t"
       "ja     2f                                               \n\t"
       "je     1f                                               \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0       \n\t"
       "jmp    2f                                               \n\t"
       "1:                                                      \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_FIELD)", %0     \n\t"
       "2:                                                      \n\t"
       :"=rm"(rc)
       :"0"(VINF_SUCCESS),
        "a"(idxField),
        "d"(u32Val)
       );
    return rc;

# elif VMX_USE_MSC_INTRINSICS
     unsigned char rcMsc = __vmx_vmwrite(idxField, u32Val);
     if (RT_LIKELY(rcMsc == 0))
         return VINF_SUCCESS;
     return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;

#else
    int rc = VINF_SUCCESS;
    __asm
    {
        push   dword ptr [u32Val]
        mov    eax, [idxField]
        _emit  0x0F
        _emit  0x79
        _emit  0x04
        _emit  0x24     /* VMWRITE eax, [esp] */
        jnc    valid_vmcs
        mov    dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR
        jmp    the_end

valid_vmcs:
        jnz    the_end
        mov    dword ptr [rc], VERR_VMX_INVALID_VMCS_FIELD
the_end:
        add    esp, 4
    }
    return rc;
# endif
}
#endif

/**
 * Executes VMWRITE.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   idxField        VMCS index.
 * @param   u64Val          16, 32 or 64-bit value.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if !defined(RT_ARCH_X86)
# if !VMX_USE_MSC_INTRINSICS || ARCH_BITS != 64
DECLASM(int) VMXWriteVmcs64(uint32_t idxField, uint64_t u64Val);
# else  /* VMX_USE_MSC_INTRINSICS */
DECLINLINE(int) VMXWriteVmcs64(uint32_t idxField, uint64_t u64Val)
{
    unsigned char rcMsc = __vmx_vmwrite(idxField, u64Val);
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;
}
# endif /* VMX_USE_MSC_INTRINSICS */
#else
# define VMXWriteVmcs64(idxField, u64Val)    VMXWriteVmcs64Ex(pVCpu, idxField, u64Val) /** @todo dead ugly, picking up pVCpu like this */
VMMR0DECL(int) VMXWriteVmcs64Ex(PVMCPU pVCpu, uint32_t idxField, uint64_t u64Val);
#endif

#if ARCH_BITS == 32
# define VMXWriteVmcsHstN                       VMXWriteVmcs32
# define VMXWriteVmcsGstN(idxField, u64Val)     VMXWriteVmcs64Ex(pVCpu, idxField, u64Val)
#else  /* ARCH_BITS == 64 */
# define VMXWriteVmcsHstN                       VMXWriteVmcs64
# define VMXWriteVmcsGstN                       VMXWriteVmcs64
#endif


/**
 * Invalidate a page using INVEPT.
 *
 * @returns VBox status code.
 * @param   enmFlush        Type of flush.
 * @param   pDescriptor     Pointer to the descriptor.
 */
DECLASM(int) VMXR0InvEPT(VMXFLUSHEPT enmFlush, uint64_t *pDescriptor);

/**
 * Invalidate a page using INVVPID.
 *
 * @returns VBox status code.
 * @param   enmFlush        Type of flush.
 * @param   pDescriptor     Pointer to the descriptor.
 */
DECLASM(int) VMXR0InvVPID(VMXFLUSHVPID enmFlush, uint64_t *pDescriptor);

/**
 * Executes VMREAD.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   idxField        VMCS index.
 * @param   pData           Where to store VM field value.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if ((RT_INLINE_ASM_EXTERNAL || !defined(RT_ARCH_X86)) && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXReadVmcs32(uint32_t idxField, uint32_t *pData);
#else
DECLINLINE(int) VMXReadVmcs32(uint32_t idxField, uint32_t *pData)
{
# if RT_INLINE_ASM_GNU_STYLE
    int rc = VINF_SUCCESS;
    __asm__ __volatile__ (
       "movl   $" RT_XSTR(VINF_SUCCESS)", %0                     \n\t"
       ".byte  0x0F, 0x78, 0xc2        # VMREAD eax, edx         \n\t"
       "ja     2f                                                \n\t"
       "je     1f                                                \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_PTR)", %0        \n\t"
       "jmp    2f                                                \n\t"
       "1:                                                       \n\t"
       "movl   $" RT_XSTR(VERR_VMX_INVALID_VMCS_FIELD)", %0      \n\t"
       "2:                                                       \n\t"
       :"=&r"(rc),
        "=d"(*pData)
       :"a"(idxField),
        "d"(0)
       );
    return rc;

# elif VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc;
#  if ARCH_BITS == 32
    rcMsc = __vmx_vmread(idxField, pData);
#  else
    uint64_t u64Tmp;
    rcMsc = __vmx_vmread(idxField, &u64Tmp);
    *pData = (uint32_t)u64Tmp;
#  endif
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;

#else
    int rc = VINF_SUCCESS;
    __asm
    {
        sub     esp, 4
        mov     dword ptr [esp], 0
        mov     eax, [idxField]
        _emit   0x0F
        _emit   0x78
        _emit   0x04
        _emit   0x24     /* VMREAD eax, [esp] */
        mov     edx, pData
        pop     dword ptr [edx]
        jnc     valid_vmcs
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_PTR
        jmp     the_end

valid_vmcs:
        jnz     the_end
        mov     dword ptr [rc], VERR_VMX_INVALID_VMCS_FIELD
the_end:
    }
    return rc;
# endif
}
#endif

/**
 * Executes VMREAD.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS.
 * @retval  VERR_VMX_INVALID_VMCS_PTR.
 * @retval  VERR_VMX_INVALID_VMCS_FIELD.
 *
 * @param   idxField        VMCS index.
 * @param   pData           Where to store VM field value.
 *
 * @remarks The values of the two status codes can be OR'ed together, the result
 *          will be VERR_VMX_INVALID_VMCS_PTR.
 */
#if (!defined(RT_ARCH_X86) && !VMX_USE_MSC_INTRINSICS)
DECLASM(int) VMXReadVmcs64(uint32_t idxField, uint64_t *pData);
#else
DECLINLINE(int) VMXReadVmcs64(uint32_t idxField, uint64_t *pData)
{
# if VMX_USE_MSC_INTRINSICS
    unsigned char rcMsc;
#  if ARCH_BITS == 32
    size_t        uLow;
    size_t        uHigh;
    rcMsc  = __vmx_vmread(idxField, &uLow);
    rcMsc |= __vmx_vmread(idxField + 1, &uHigh);
    *pData = RT_MAKE_U64(uLow, uHigh);
# else
    rcMsc = __vmx_vmread(idxField, pData);
# endif
    if (RT_LIKELY(rcMsc == 0))
        return VINF_SUCCESS;
    return rcMsc == 2 ? VERR_VMX_INVALID_VMCS_PTR : VERR_VMX_INVALID_VMCS_FIELD;

# elif ARCH_BITS == 32
    int rc;
    uint32_t val_hi, val;
    rc  = VMXReadVmcs32(idxField, &val);
    rc |= VMXReadVmcs32(idxField + 1, &val_hi);
    AssertRC(rc);
    *pData = RT_MAKE_U64(val, val_hi);
    return rc;

# else
#  error "Shouldn't be here..."
# endif
}
#endif

/**
 * Gets the last instruction error value from the current VMCS.
 *
 * @returns VBox status code.
 */
DECLINLINE(uint32_t) VMXGetLastError(void)
{
#if ARCH_BITS == 64
    uint64_t uLastError = 0;
    int rc = VMXReadVmcs64(VMX_VMCS32_RO_VM_INSTR_ERROR, &uLastError);
    AssertRC(rc);
    return (uint32_t)uLastError;

#else /* 32-bit host: */
    uint32_t uLastError = 0;
    int rc = VMXReadVmcs32(VMX_VMCS32_RO_VM_INSTR_ERROR, &uLastError);
    AssertRC(rc);
    return uLastError;
#endif
}

#ifdef IN_RING0
VMMR0DECL(int) VMXR0InvalidatePage(PVM pVM, PVMCPU pVCpu, RTGCPTR GCVirt);
VMMR0DECL(int) VMXR0InvalidatePhysPage(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys);
#endif /* IN_RING0 */

/** @} */

/** @} */

#endif


/* $Id: APICInternal.h $ */
/** @file
 * APIC - Advanced Programmable Interrupt Controller.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___APICInternal_h
#define ___APICInternal_h

#include <VBox/sup.h>
#include <VBox/types.h>
#include <VBox/vmm/apic.h>

/** @defgroup grp_apic_int       Internal
 * @ingroup grp_apic
 * @internal
 * @{
 */

/** The APIC hardware version number for Pentium 4. */
#define XAPIC_HARDWARE_VERSION_P4            UINT8_C(0x14)
/** Maximum number of LVT entries for Pentium 4. */
#define XAPIC_MAX_LVT_ENTRIES_P4             UINT8_C(6)
/** Size of the APIC ID bits for Pentium 4. */
#define XAPIC_APIC_ID_BIT_COUNT_P4           UINT8_C(8)

/** The APIC hardware version number for Pentium 6. */
#define XAPIC_HARDWARE_VERSION_P6            UINT8_C(0x10)
/** Maximum number of LVT entries for Pentium 6. */
#define XAPIC_MAX_LVT_ENTRIES_P6             UINT8_C(4)
/** Size of the APIC ID bits for Pentium 6. */
#define XAPIC_APIC_ID_BIT_COUNT_P6           UINT8_C(4)

/** The APIC hardware version we are emulating. */
#define XAPIC_HARDWARE_VERSION               XAPIC_HARDWARE_VERSION_P4

#define VMCPU_TO_XAPICPAGE(a_pVCpu)          ((PXAPICPAGE)(CTX_SUFF((a_pVCpu)->apic.s.pvApicPage)))
#define VMCPU_TO_CXAPICPAGE(a_pVCpu)         ((PCXAPICPAGE)(CTX_SUFF((a_pVCpu)->apic.s.pvApicPage)))

#define VMCPU_TO_X2APICPAGE(a_pVCpu)         ((PX2APICPAGE)(CTX_SUFF((a_pVCpu)->apic.s.pvApicPage)))
#define VMCPU_TO_CX2APICPAGE(a_pVCpu)        ((PCX2APICPAGE)(CTX_SUFF((a_pVCpu)->apic.s.pvApicPage)))

#define VMCPU_TO_APICCPU(a_pVCpu)            (&(a_pVCpu)->apic.s)
#define VM_TO_APIC(a_pVM)                    (&(a_pVM)->apic.s)
#define VM_TO_APICDEV(a_pVM)                 CTX_SUFF(VM_TO_APIC(a_pVM)->pApicDev)

#define APICCPU_TO_XAPICPAGE(a_ApicCpu)      ((PXAPICPAGE)(CTX_SUFF((a_ApicCpu)->pvApicPage)))
#define APICCPU_TO_CXAPICPAGE(a_ApicCpu)     ((PCXAPICPAGE)(CTX_SUFF((a_ApicCpu)->pvApicPage)))

/** Whether the APIC is in X2APIC mode or not. */
#define XAPIC_IN_X2APIC_MODE(a_pVCpu)        (   (  ((a_pVCpu)->apic.s.uApicBaseMsr) \
                                                  & (MSR_IA32_APICBASE_EN | MSR_IA32_APICBASE_EXTD)) \
                                              ==    (MSR_IA32_APICBASE_EN | MSR_IA32_APICBASE_EXTD) )

/** Get an xAPIC page offset for an x2APIC MSR value. */
#define X2APIC_GET_XAPIC_OFF(a_uMsr)         ((((a_uMsr) - MSR_IA32_X2APIC_START) << 4) & UINT32_C(0xff0))
/** Get an x2APIC MSR for an xAPIC page offset. */
#define XAPIC_GET_X2APIC_MSR(a_offReg)       ((((a_offReg) & UINT32_C(0xff0)) >> 4) | MSR_IA32_X2APIC_START)

/** Illegal APIC vector value start. */
#define XAPIC_ILLEGAL_VECTOR_START           UINT8_C(0)
/** Illegal APIC vector value end (inclusive). */
#define XAPIC_ILLEGAL_VECTOR_END             UINT8_C(15)
/** Reserved APIC vector value start. */
#define XAPIC_RSVD_VECTOR_START              UINT8_C(16)
/** Reserved APIC vector value end (inclusive). */
#define XAPIC_RSVD_VECTOR_END                UINT8_C(31)

/** Vector offset in an APIC 256-bit sparse register. */
#define XAPIC_REG256_VECTOR_OFF(a_Vector)    (((a_Vector) & UINT32_C(0xe0)) >> 1)
/** Bit position at offset in an APIC 256-bit sparse register. */
#define XAPIC_REG256_VECTOR_BIT(a_Vector)    ((a_Vector) & UINT32_C(0x1f))

/** Maximum valid offset for a register (16-byte aligned, 4 byte wide access). */
#define XAPIC_OFF_MAX_VALID                  (sizeof(XAPICPAGE) - 4 * sizeof(uint32_t))

#if XAPIC_HARDWARE_VERSION == XAPIC_HARDWARE_VERSION_P6
/** ESR - Send checksum error. */
# define XAPIC_ESR_SEND_CHKSUM_ERROR         RT_BIT(0)
/** ESR - Send accept error. */
# define XAPIC_ESR_RECV_CHKSUM_ERROR         RT_BIT(1)
/** ESR - Send accept error. */
# define XAPIC_ESR_SEND_ACCEPT_ERROR         RT_BIT(2)
/** ESR - Receive accept error. */
# define XAPIC_ESR_RECV_ACCEPT_ERROR         RT_BIT(3)
#endif
/** ESR - Redirectable IPI. */
#define XAPIC_ESR_REDIRECTABLE_IPI           RT_BIT(4)
/** ESR - Send accept error. */
#define XAPIC_ESR_SEND_ILLEGAL_VECTOR        RT_BIT(5)
/** ESR - Send accept error. */
#define XAPIC_ESR_RECV_ILLEGAL_VECTOR        RT_BIT(6)
/** ESR - Send accept error. */
#define XAPIC_ESR_ILLEGAL_REG_ADDRESS        RT_BIT(7)
/** ESR - Valid write-only bits. */
#define XAPIC_ESR_WO_VALID                   UINT32_C(0x0)

/** TPR - Valid bits. */
#define XAPIC_TPR_VALID                      UINT32_C(0xff)
/** TPR - Task-priority class. */
#define XAPIC_TPR_TP                         UINT32_C(0xf0)
/** TPR - Task-priority subclass. */
#define XAPIC_TPR_TP_SUBCLASS                UINT32_C(0x0f)
/** TPR - Gets the task-priority class. */
#define XAPIC_TPR_GET_TP(a_Tpr)              ((a_Tpr) & XAPIC_TPR_TP)
/** TPR - Gets the task-priority subclass. */
#define XAPIC_TPR_GET_TP_SUBCLASS(a_Tpr)     ((a_Tpr) & XAPIC_TPR_TP_SUBCLASS)

/** PPR - Valid bits. */
#define XAPIC_PPR_VALID                      UINT32_C(0xff)
/** PPR - Processor-priority class. */
#define XAPIC_PPR_PP                         UINT32_C(0xf0)
/** PPR - Processor-priority subclass. */
#define XAPIC_PPR_PP_SUBCLASS                UINT32_C(0x0f)
/** PPR - Get the processor-priority class. */
#define XAPIC_PPR_GET_PP(a_Ppr)              ((a_Ppr) & XAPIC_PPR_PP)
/** PPR - Get the processor-priority subclass. */
#define XAPIC_PPR_GET_PP_SUBCLASS(a_Ppr)     ((a_Ppr) & XAPIC_PPR_PP_SUBCLASS)

/** Timer mode - One-shot. */
#define XAPIC_TIMER_MODE_ONESHOT             UINT32_C(0)
/** Timer mode - Periodic. */
#define XAPIC_TIMER_MODE_PERIODIC            UINT32_C(1)
/** Timer mode - TSC deadline. */
#define XAPIC_TIMER_MODE_TSC_DEADLINE        UINT32_C(2)

/** LVT - The vector. */
#define XAPIC_LVT_VECTOR                     UINT32_C(0xff)
/** LVT - Gets the vector from an LVT entry. */
#define XAPIC_LVT_GET_VECTOR(a_Lvt)          ((a_Lvt) & XAPIC_LVT_VECTOR)
/** LVT - The mask. */
#define XAPIC_LVT_MASK                       RT_BIT(16)
/** LVT - Is the LVT masked? */
#define XAPIC_LVT_IS_MASKED(a_Lvt)           RT_BOOL((a_Lvt) & XAPIC_LVT_MASK)
/** LVT - Timer mode. */
#define XAPIC_LVT_TIMER_MODE                 RT_BIT(17)
/** LVT - Timer TSC-deadline timer mode. */
#define XAPIC_LVT_TIMER_TSCDEADLINE          RT_BIT(18)
/** LVT - Gets the timer mode. */
#define XAPIC_LVT_GET_TIMER_MODE(a_Lvt)      (XAPICTIMERMODE)(((a_Lvt) >> 17) & UINT32_C(3))
/** LVT - Delivery mode. */
#define XAPIC_LVT_DELIVERY_MODE              (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
/** LVT - Gets the delivery mode. */
#define XAPIC_LVT_GET_DELIVERY_MODE(a_Lvt)   (XAPICDELIVERYMODE)(((a_Lvt) >> 8) & UINT32_C(7))
/** LVT - Delivery status. */
#define XAPIC_LVT_DELIVERY_STATUS            RT_BIT(12)
/** LVT - Trigger mode. */
#define XAPIC_LVT_TRIGGER_MODE               RT_BIT(15)
/** LVT - Gets the trigger mode. */
#define XAPIC_LVT_GET_TRIGGER_MODE(a_Lvt)    (XAPICTRIGGERMODE)(((a_Lvt) >> 15) & UINT32_C(1))
/** LVT - Remote IRR. */
#define XAPIC_LVT_REMOTE_IRR                 RT_BIT(14)
/** LVT - Gets the Remote IRR. */
#define XAPIC_LVT_GET_REMOTE_IRR(a_Lvt)      (((a_Lvt) >> 14) & 1)
/** LVT - Interrupt Input Pin Polarity. */
#define XAPIC_LVT_POLARITY                   RT_BIT(13)
/** LVT - Gets the Interrupt Input Pin Polarity. */
#define XAPIC_LVT_GET_POLARITY(a_Lvt)        (((a_Lvt) >> 13) & 1)
/** LVT - Valid bits common to all LVTs. */
#define XAPIC_LVT_COMMON_VALID               (XAPIC_LVT_VECTOR | XAPIC_LVT_DELIVERY_STATUS | XAPIC_LVT_MASK)
/** LVT CMCI - Valid bits. */
#define XAPIC_LVT_CMCI_VALID                 (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE)
/** LVT Timer - Valid bits. */
#define XAPIC_LVT_TIMER_VALID                (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_TIMER_MODE | XAPIC_LVT_TIMER_TSCDEADLINE)
/** LVT Thermal - Valid bits. */
#define XAPIC_LVT_THERMAL_VALID              (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE)
/** LVT Perf - Valid bits. */
#define XAPIC_LVT_PERF_VALID                 (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE)
/** LVT LINTx - Valid bits. */
#define XAPIC_LVT_LINT_VALID                 (  XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE | XAPIC_LVT_DELIVERY_STATUS \
                                              | XAPIC_LVT_POLARITY | XAPIC_LVT_REMOTE_IRR | XAPIC_LVT_TRIGGER_MODE)
/** LVT Error - Valid bits. */
#define XAPIC_LVT_ERROR_VALID                (XAPIC_LVT_COMMON_VALID)

/** SVR - The vector. */
#define XAPIC_SVR_VECTOR                     UINT32_C(0xff)
/** SVR - APIC Software enable. */
#define XAPIC_SVR_SOFTWARE_ENABLE            RT_BIT(8)
/** SVR - Supress EOI broadcast. */
#define XAPIC_SVR_SUPRESS_EOI_BROADCAST      RT_BIT(12)
#if XAPIC_HARDWARE_VERSION == XAPIC_HARDWARE_VERSION_P4
/** SVR - Valid bits. */
# define XAPIC_SVR_VALID                     (XAPIC_SVR_VECTOR | XAPIC_SVR_SOFTWARE_ENABLE)
#else
# error "Implement Pentium and P6 family APIC architectures"
#endif

/** DFR - Valid bits. */
#define XAPIC_DFR_VALID                      UINT32_C(0xf0000000)
/** DFR - Reserved bits that must always remain set. */
#define XAPIC_DFR_RSVD_MB1                   UINT32_C(0x0fffffff)
/** DFR - The model. */
#define XAPIC_DFR_MODEL                      UINT32_C(0xf)
/** DFR - Gets the destination model. */
#define XAPIC_DFR_GET_MODEL(a_uReg)          (((a_uReg) >> 28) & XAPIC_DFR_MODEL)

/** LDR - Valid bits. */
#define XAPIC_LDR_VALID                      UINT32_C(0xff000000)
/** LDR - Cluster ID mask (x2APIC). */
#define X2APIC_LDR_CLUSTER_ID                UINT32_C(0xffff0000)
/** LDR - Mask of the LDR cluster ID (x2APIC). */
#define X2APIC_LDR_GET_CLUSTER_ID(a_uReg)    ((a_uReg) & X2APIC_LDR_CLUSTER_ID)
/** LDR - Mask of the LDR logical ID (x2APIC). */
#define X2APIC_LDR_LOGICAL_ID                UINT32_C(0x0000ffff)

/** LDR - Flat mode logical ID mask. */
#define XAPIC_LDR_FLAT_LOGICAL_ID            UINT32_C(0xff)
/** LDR - Clustered mode cluster ID mask. */
#define XAPIC_LDR_CLUSTERED_CLUSTER_ID       UINT32_C(0xf0)
/** LDR - Clustered mode logical ID mask. */
#define XAPIC_LDR_CLUSTERED_LOGICAL_ID       UINT32_C(0x0f)
/** LDR - Gets the clustered mode cluster ID. */
#define XAPIC_LDR_CLUSTERED_GET_CLUSTER_ID(a_uReg)   ((a_uReg) & XAPIC_LDR_CLUSTERED_CLUSTER_ID)


/** EOI - Valid write-only bits. */
#define XAPIC_EOI_WO_VALID                   UINT32_C(0x0)
/** Timer ICR - Valid bits. */
#define XAPIC_TIMER_ICR_VALID                UINT32_C(0xffffffff)
/** Timer DCR - Valid bits. */
#define XAPIC_TIMER_DCR_VALID                (RT_BIT(0) | RT_BIT(1) | RT_BIT(3))

/** Self IPI - Valid bits. */
#define XAPIC_SELF_IPI_VALID                 UINT32_C(0xff)
/** Self IPI - The vector. */
#define XAPIC_SELF_IPI_VECTOR                UINT32_C(0xff)
/** Self IPI - Gets the vector. */
#define XAPIC_SELF_IPI_GET_VECTOR(a_uReg)    ((a_uReg) & XAPIC_SELF_IPI_VECTOR)

/** ICR Low - The Vector. */
#define XAPIC_ICR_LO_VECTOR                  UINT32_C(0xff)
/** ICR Low - Gets the vector. */
#define XAPIC_ICR_LO_GET_VECTOR(a_uIcr)      ((a_uIcr) & XAPIC_ICR_LO_VECTOR)
/** ICR Low - The delivery mode. */
#define XAPIC_ICR_LO_DELIVERY_MODE           (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
/** ICR Low - The destination mode. */
#define XAPIC_ICR_LO_DEST_MODE               RT_BIT(11)
/** ICR Low - The delivery status. */
#define XAPIC_ICR_LO_DELIVERY_STATUS         RT_BIT(12)
/** ICR Low - The level. */
#define XAPIC_ICR_LO_LEVEL                   RT_BIT(14)
/** ICR Low - The trigger mode. */
#define XAPIC_ICR_TRIGGER_MODE               RT_BIT(15)
/** ICR Low - The destination shorthand. */
#define XAPIC_ICR_LO_DEST_SHORTHAND          (RT_BIT(18) | RT_BIT(19))
/** ICR Low - Valid write bits. */
#define XAPIC_ICR_LO_WR_VALID                (  XAPIC_ICR_LO_VECTOR | XAPIC_ICR_LO_DELIVERY_MODE | XAPIC_ICR_LO_DEST_MODE \
                                              | XAPIC_ICR_LO_LEVEL | XAPIC_ICR_TRIGGER_MODE | XAPIC_ICR_LO_DEST_SHORTHAND)

/** ICR High - The destination field. */
#define XAPIC_ICR_HI_DEST                    UINT32_C(0xff000000)
/** ICR High - Get the destination field. */
#define XAPIC_ICR_HI_GET_DEST(a_u32IcrHi)    (((a_u32IcrHi) >> 24) & XAPIC_ICR_HI_DEST)
/** ICR High - Valid write bits in xAPIC mode. */
#define XAPIC_ICR_HI_WR_VALID                XAPIC_ICR_HI_DEST

/** APIC ID broadcast mask - x2APIC mode. */
#define X2APIC_ID_BROADCAST_MASK             UINT32_C(0xffffffff)
#if XAPIC_HARDWARE_VERSION == XAPIC_HARDWARE_VERSION_P4
/** APIC ID broadcast mask - xAPIC mode. */
# define XAPIC_ID_BROADCAST_MASK             UINT32_C(0xff)
#else
# error "Implement Pentium and P6 family APIC architectures"
#endif

/**
 * The xAPIC sparse 256-bit register.
 */
typedef union XAPIC256BITREG
{
    /** The sparse-bitmap view. */
    struct
    {
        uint32_t    u32Reg;
        uint32_t    uReserved0[3];
    } u[8];
    /** The 32-bit view. */
    uint32_t        au32[32];
} XAPIC256BITREG;
/** Pointer to an xAPIC sparse bitmap register. */
typedef XAPIC256BITREG *PXAPIC256BITREG;
/** Pointer to a const xAPIC sparse bitmap register. */
typedef XAPIC256BITREG const *PCXAPIC256BITREG;
AssertCompileSize(XAPIC256BITREG, 128);

/**
 * The xAPIC memory layout as per Intel/AMD specs.
 */
typedef struct XAPICPAGE
{
    /* 0x00 - Reserved. */
    uint32_t                    uReserved0[8];
    /* 0x20 - APIC ID. */
    struct
    {
        uint8_t                 u8Reserved0[3];
        uint8_t                 u8ApicId;
        uint32_t                u32Reserved0[3];
    } id;
    /* 0x30 - APIC version register. */
    union
    {
        struct
        {
#if XAPIC_HARDWARE_VERSION == XAPIC_HARDWARE_VERSION_P4
            uint8_t             u8Version;
#else
# error "Implement Pentium and P6 family APIC architectures"
#endif
            uint8_t             uReserved0;
            uint8_t             u8MaxLvtEntry;
            uint8_t             fEoiBroadcastSupression : 1;
            uint8_t             u7Reserved1   : 7;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32Version;
            uint32_t            u32Reserved0[3];
        } all;
    } version;
    /* 0x40 - Reserved. */
    uint32_t                    uReserved1[16];
    /* 0x80 - Task Priority Register (TPR). */
    struct
    {
        uint8_t                 u8Tpr;
        uint8_t                 u8Reserved0[3];
        uint32_t                u32Reserved0[3];
    } tpr;
    /* 0x90 - Arbitration Priority Register (APR). */
    struct
    {
        uint8_t                 u8Apr;
        uint8_t                 u8Reserved0[3];
        uint32_t                u32Reserved0[3];
    } apr;
    /* 0xA0 - Processor Priority Register (PPR). */
    struct
    {
        uint8_t                 u8Ppr;
        uint8_t                 u8Reserved0[3];
        uint32_t                u32Reserved0[3];
    } ppr;
    /* 0xB0 - End Of Interrupt Register (EOI). */
    struct
    {
        uint32_t                u32Eoi;
        uint32_t                u32Reserved0[3];
    } eoi;
    /* 0xC0 - Remote Read Register (RRD). */
    struct
    {
        uint32_t                u32Rrd;
        uint32_t                u32Reserved0[3];
    } rrd;
    /* 0xD0 - Logical Destination Register (LDR). */
    union
    {
        struct
        {
            uint8_t             u8Reserved0[3];
            uint8_t             u8LogicalApicId;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32Ldr;
            uint32_t            u32Reserved0[3];
        } all;
    } ldr;
    /* 0xE0 - Destination Format Register (DFR). */
    union
    {
        struct
        {
            uint32_t            u28ReservedMb1 : 28;    /* MB1 */
            uint32_t            u4Model        : 4;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32Dfr;
            uint32_t            u32Reserved0[3];
        } all;
    } dfr;
    /* 0xF0 - Spurious-Interrupt Vector Register (SVR). */
    union
    {
        struct
        {
            uint32_t            u8SpuriousVector     : 8;
            uint32_t            fApicSoftwareEnable  : 1;
            uint32_t            u3Reserved0          : 3;
            uint32_t            fSupressEoiBroadcast : 1;
            uint32_t            u19Reserved1         : 19;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32Svr;
            uint32_t            u32Reserved0[3];
        } all;
    } svr;
    /* 0x100 - In-service Register (ISR). */
    XAPIC256BITREG              isr;
    /* 0x180 - Trigger Mode Register (TMR). */
    XAPIC256BITREG              tmr;
    /* 0x200 - Interrupt Request Register (IRR). */
    XAPIC256BITREG              irr;
    /* 0x280 - Error Status Register (ESR). */
    union
    {
        struct
        {
#if XAPIC_HARDWARE_VERSION == XAPIC_HARDWARE_VERSION_P4
            uint32_t            u4Reserved0        : 4;
#else
# error "Implement Pentium and P6 family APIC architectures"
#endif
            uint32_t            fRedirectableIpi   : 1;
            uint32_t            fSendIllegalVector : 1;
            uint32_t            fRcvdIllegalVector : 1;
            uint32_t            fIllegalRegAddr    : 1;
            uint32_t            u24Reserved1       : 24;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32Errors;
            uint32_t            u32Reserved0[3];
        } all;
    } esr;
    /* 0x290 - Reserved. */
    uint32_t                    uReserved2[28];
    /* 0x300 - Interrupt Command Register (ICR) - Low. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1DestMode       : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            fReserved0       : 1;
            uint32_t            u1Level          : 1;
            uint32_t            u1TriggerMode    : 1;
            uint32_t            u2Reserved1      : 2;
            uint32_t            u2DestShorthand  : 2;
            uint32_t            u12Reserved2     : 12;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32IcrLo;
            uint32_t            u32Reserved0[3];
        } all;
    } icr_lo;
    /* 0x310 - Interrupt Comannd Register (ICR) - High. */
    union
    {
        struct
        {
            uint32_t            u24Reserved0 : 24;
            uint32_t            u8Dest       : 8;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32IcrHi;
            uint32_t            u32Reserved0[3];
        } all;
    } icr_hi;
    /* 0x320 - Local Vector Table (LVT) Timer Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u4Reserved0      : 4;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u2TimerMode      : 2;
            uint32_t            u13Reserved2     : 13;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtTimer;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_timer;
    /* 0x330 - Local Vector Table (LVT) Thermal Sensor Register. */
#if XAPIC_HARDWARE_VERSION == XAPIC_HARDWARE_VERSION_P4
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtThermal;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_thermal;
#else
# error "Implement Pentium and P6 family APIC architectures"
#endif
    /* 0x340 - Local Vector Table (LVT) Performance Monitor Counter (PMC) Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtPerf;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_perf;
    /* 0x350 - Local Vector Table (LVT) LINT0 Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u1IntrPolarity   : 1;
            uint32_t            u1RemoteIrr      : 1;
            uint32_t            u1TriggerMode    : 1;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtLint0;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_lint0;
    /* 0x360 - Local Vector Table (LVT) LINT1 Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u1IntrPolarity   : 1;
            uint32_t            u1RemoteIrr      : 1;
            uint32_t            u1TriggerMode    : 1;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtLint1;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_lint1;
    /* 0x370 - Local Vector Table (LVT) Error Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u4Reserved0      : 4;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtError;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_error;
    /* 0x380 - Timer Initial Counter Register. */
    struct
    {
        uint32_t                u32InitialCount;
        uint32_t                u32Reserved0[3];
    } timer_icr;
    /* 0x390 - Timer Current Counter Register. */
    struct
    {
        uint32_t                u32CurrentCount;
        uint32_t                u32Reserved0[3];
    } timer_ccr;
    /* 0x3A0 - Reserved. */
    uint32_t                    u32Reserved3[16];
    /* 0x3E0 - Timer Divide Configuration Register. */
    union
    {
        struct
        {
            uint32_t            u2DivideValue0 : 2;
            uint32_t            u1Reserved0    : 1;
            uint32_t            u1DivideValue1 : 1;
            uint32_t            u28Reserved1   : 28;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32DivideValue;
            uint32_t            u32Reserved0[3];
        } all;
    } timer_dcr;
    /* 0x3F0 - Reserved. */
    uint8_t                     u8Reserved0[3088];
} XAPICPAGE;
/** Pointer to a XAPICPAGE struct. */
typedef XAPICPAGE *PXAPICPAGE;
/** Pointer to a const XAPICPAGE struct. */
typedef const XAPICPAGE *PCXAPICPAGE;
AssertCompileSize(XAPICPAGE, 4096);
AssertCompileMemberOffset(XAPICPAGE, id,          XAPIC_OFF_ID);
AssertCompileMemberOffset(XAPICPAGE, version,     XAPIC_OFF_VERSION);
AssertCompileMemberOffset(XAPICPAGE, tpr,         XAPIC_OFF_TPR);
AssertCompileMemberOffset(XAPICPAGE, apr,         XAPIC_OFF_APR);
AssertCompileMemberOffset(XAPICPAGE, ppr,         XAPIC_OFF_PPR);
AssertCompileMemberOffset(XAPICPAGE, eoi,         XAPIC_OFF_EOI);
AssertCompileMemberOffset(XAPICPAGE, rrd,         XAPIC_OFF_RRD);
AssertCompileMemberOffset(XAPICPAGE, ldr,         XAPIC_OFF_LDR);
AssertCompileMemberOffset(XAPICPAGE, dfr,         XAPIC_OFF_DFR);
AssertCompileMemberOffset(XAPICPAGE, svr,         XAPIC_OFF_SVR);
AssertCompileMemberOffset(XAPICPAGE, isr,         XAPIC_OFF_ISR0);
AssertCompileMemberOffset(XAPICPAGE, tmr,         XAPIC_OFF_TMR0);
AssertCompileMemberOffset(XAPICPAGE, irr,         XAPIC_OFF_IRR0);
AssertCompileMemberOffset(XAPICPAGE, esr,         XAPIC_OFF_ESR);
AssertCompileMemberOffset(XAPICPAGE, icr_lo,      XAPIC_OFF_ICR_LO);
AssertCompileMemberOffset(XAPICPAGE, icr_hi,      XAPIC_OFF_ICR_HI);
AssertCompileMemberOffset(XAPICPAGE, lvt_timer,   XAPIC_OFF_LVT_TIMER);
AssertCompileMemberOffset(XAPICPAGE, lvt_thermal, XAPIC_OFF_LVT_THERMAL);
AssertCompileMemberOffset(XAPICPAGE, lvt_perf,    XAPIC_OFF_LVT_PERF);
AssertCompileMemberOffset(XAPICPAGE, lvt_lint0,   XAPIC_OFF_LVT_LINT0);
AssertCompileMemberOffset(XAPICPAGE, lvt_lint1,   XAPIC_OFF_LVT_LINT1);
AssertCompileMemberOffset(XAPICPAGE, lvt_error,   XAPIC_OFF_LVT_ERROR);
AssertCompileMemberOffset(XAPICPAGE, timer_icr,   XAPIC_OFF_TIMER_ICR);
AssertCompileMemberOffset(XAPICPAGE, timer_ccr,   XAPIC_OFF_TIMER_CCR);
AssertCompileMemberOffset(XAPICPAGE, timer_dcr,   XAPIC_OFF_TIMER_DCR);

/**
 * The x2APIC memory layout as per Intel/AMD specs.
 */
typedef struct X2APICPAGE
{
    /* 0x00 - Reserved. */
    uint32_t                    uReserved0[8];
    /* 0x20 - APIC ID. */
    struct
    {
        uint32_t                u32ApicId;
        uint32_t                u32Reserved0[3];
    } id;
    /* 0x30 - APIC version register. */
    union
    {
        struct
        {
            uint8_t             u8Version;
            uint8_t             u8Reserved0;
            uint8_t             u8MaxLvtEntry;
            uint8_t             fEoiBroadcastSupression : 1;
            uint8_t             u7Reserved1   : 7;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32Version;
            uint32_t            u32Reserved2[3];
        } all;
    } version;
    /* 0x40 - Reserved. */
    uint32_t                    uReserved1[16];
    /* 0x80 - Task Priority Register (TPR). */
    struct
    {
        uint8_t                 u8Tpr;
        uint8_t                 u8Reserved0[3];
        uint32_t                u32Reserved0[3];
    } tpr;
    /* 0x90 - Reserved. */
    uint32_t                    uReserved2[4];
    /* 0xA0 - Processor Priority Register (PPR). */
    struct
    {
        uint8_t                 u8Ppr;
        uint8_t                 u8Reserved0[3];
        uint32_t                u32Reserved0[3];
    } ppr;
    /* 0xB0 - End Of Interrupt Register (EOI). */
    struct
    {
        uint32_t                u32Eoi;
        uint32_t                u32Reserved0[3];
    } eoi;
    /* 0xC0 - Remote Read Register (RRD). */
    struct
    {
        uint32_t                u32Rrd;
        uint32_t                u32Reserved0[3];
    } rrd;
    /* 0xD0 - Logical Destination Register (LDR). */
    struct
    {
        uint32_t                u32LogicalApicId;
        uint32_t                u32Reserved1[3];
    } ldr;
    /* 0xE0 - Reserved. */
    uint32_t                    uReserved3[4];
    /* 0xF0 - Spurious-Interrupt Vector Register (SVR). */
    union
    {
        struct
        {
            uint32_t            u8SpuriousVector     : 8;
            uint32_t            fApicSoftwareEnable  : 1;
            uint32_t            u3Reserved0          : 3;
            uint32_t            fSupressEoiBroadcast : 1;
            uint32_t            u19Reserved1         : 19;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32Svr;
            uint32_t            uReserved0[3];
        } all;
    } svr;
    /* 0x100 - In-service Register (ISR). */
    XAPIC256BITREG              isr;
    /* 0x180 - Trigger Mode Register (TMR). */
    XAPIC256BITREG              tmr;
    /* 0x200 - Interrupt Request Register (IRR). */
    XAPIC256BITREG              irr;
    /* 0x280 - Error Status Register (ESR). */
    union
    {
        struct
        {
#if XAPIC_HARDWARE_VERSION == XAPIC_HARDWARE_VERSION_P4
            uint32_t            u4Reserved0        : 4;
#else
# error "Implement Pentium and P6 family APIC architectures"
#endif
            uint32_t            fRedirectableIpi   : 1;
            uint32_t            fSendIllegalVector : 1;
            uint32_t            fRcvdIllegalVector : 1;
            uint32_t            fIllegalRegAddr    : 1;
            uint32_t            u24Reserved1       : 24;
            uint32_t            uReserved0[3];
        } u;
        struct
        {
            uint32_t            u32Errors;
            uint32_t            u32Reserved0[3];
        } all;
    } esr;
    /* 0x290 - Reserved. */
    uint32_t                    uReserved4[28];
    /* 0x300 - Interrupt Command Register (ICR) - Low. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1DestMode       : 1;
            uint32_t            u2Reserved0      : 2;
            uint32_t            u1Level          : 1;
            uint32_t            u1TriggerMode    : 1;
            uint32_t            u2Reserved1      : 2;
            uint32_t            u2DestShorthand  : 2;
            uint32_t            u12Reserved2     : 12;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32IcrLo;
            uint32_t            u32Reserved3[3];
        } all;
    } icr_lo;
    /* 0x310 - Interrupt Comannd Register (ICR) - High. */
    struct
    {
        uint32_t                u32IcrHi;
        uint32_t                uReserved1[3];
    } icr_hi;
    /* 0x320 - Local Vector Table (LVT) Timer Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u4Reserved0      : 4;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u2TimerMode      : 2;
            uint32_t            u13Reserved2     : 13;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtTimer;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_timer;
    /* 0x330 - Local Vector Table (LVT) Thermal Sensor Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtThermal;
            uint32_t            uReserved0[3];
        } all;
    } lvt_thermal;
    /* 0x340 - Local Vector Table (LVT) Performance Monitor Counter (PMC) Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtPerf;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_perf;
    /* 0x350 - Local Vector Table (LVT) LINT0 Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u1IntrPolarity   : 1;
            uint32_t            u1RemoteIrr      : 1;
            uint32_t            u1TriggerMode    : 1;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtLint0;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_lint0;
    /* 0x360 - Local Vector Table (LVT) LINT1 Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u3DeliveryMode   : 3;
            uint32_t            u1Reserved0      : 1;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u1IntrPolarity   : 1;
            uint32_t            u1RemoteIrr      : 1;
            uint32_t            u1TriggerMode    : 1;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtLint1;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_lint1;
    /* 0x370 - Local Vector Table (LVT) Error Register. */
    union
    {
        struct
        {
            uint32_t            u8Vector         : 8;
            uint32_t            u4Reserved0      : 4;
            uint32_t            u1DeliveryStatus : 1;
            uint32_t            u3Reserved1      : 3;
            uint32_t            u1Mask           : 1;
            uint32_t            u15Reserved2     : 15;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32LvtError;
            uint32_t            u32Reserved0[3];
        } all;
    } lvt_error;
    /* 0x380 - Timer Initial Counter Register. */
    struct
    {
        uint32_t                u32InitialCount;
        uint32_t                u32Reserved0[3];
    } timer_icr;
    /* 0x390 - Timer Current Counter Register. */
    struct
    {
        uint32_t                u32CurrentCount;
        uint32_t                u32Reserved0[3];
    } timer_ccr;
    /* 0x3A0 - Reserved. */
    uint32_t                    uReserved5[16];
    /* 0x3E0 - Timer Divide Configuration Register. */
    union
    {
        struct
        {
            uint32_t            u2DivideValue0 : 2;
            uint32_t            u1Reserved0    : 1;
            uint32_t            u1DivideValue1 : 1;
            uint32_t            u28Reserved1   : 28;
            uint32_t            u32Reserved0[3];
        } u;
        struct
        {
            uint32_t            u32DivideValue;
            uint32_t            u32Reserved0[3];
        } all;
    } timer_dcr;
    /* 0x3F0 - Self IPI Register. */
    struct
    {
        uint32_t                u8Vector     : 8;
        uint32_t                u24Reserved0 : 24;
        uint32_t                u32Reserved0[3];
    } self_ipi;
    /* 0x400 - Reserved. */
    uint8_t                     u8Reserved0[3072];
} X2APICPAGE;
/** Pointer to a X2APICPAGE struct. */
typedef X2APICPAGE *PX2APICPAGE;
/** Pointer to a const X2APICPAGE struct. */
typedef const X2APICPAGE *PCX2APICPAGE;
AssertCompileSize(X2APICPAGE, 4096);
AssertCompileSize(X2APICPAGE, sizeof(XAPICPAGE));
AssertCompileMemberOffset(X2APICPAGE, id,          XAPIC_OFF_ID);
AssertCompileMemberOffset(X2APICPAGE, version,     XAPIC_OFF_VERSION);
AssertCompileMemberOffset(X2APICPAGE, tpr,         XAPIC_OFF_TPR);
AssertCompileMemberOffset(X2APICPAGE, ppr,         XAPIC_OFF_PPR);
AssertCompileMemberOffset(X2APICPAGE, eoi,         XAPIC_OFF_EOI);
AssertCompileMemberOffset(X2APICPAGE, rrd,         XAPIC_OFF_RRD);
AssertCompileMemberOffset(X2APICPAGE, ldr,         XAPIC_OFF_LDR);
AssertCompileMemberOffset(X2APICPAGE, svr,         XAPIC_OFF_SVR);
AssertCompileMemberOffset(X2APICPAGE, isr,         XAPIC_OFF_ISR0);
AssertCompileMemberOffset(X2APICPAGE, tmr,         XAPIC_OFF_TMR0);
AssertCompileMemberOffset(X2APICPAGE, irr,         XAPIC_OFF_IRR0);
AssertCompileMemberOffset(X2APICPAGE, esr,         XAPIC_OFF_ESR);
AssertCompileMemberOffset(X2APICPAGE, icr_lo,      XAPIC_OFF_ICR_LO);
AssertCompileMemberOffset(X2APICPAGE, icr_hi,      XAPIC_OFF_ICR_HI);
AssertCompileMemberOffset(X2APICPAGE, lvt_timer,   XAPIC_OFF_LVT_TIMER);
AssertCompileMemberOffset(X2APICPAGE, lvt_thermal, XAPIC_OFF_LVT_THERMAL);
AssertCompileMemberOffset(X2APICPAGE, lvt_perf,    XAPIC_OFF_LVT_PERF);
AssertCompileMemberOffset(X2APICPAGE, lvt_lint0,   XAPIC_OFF_LVT_LINT0);
AssertCompileMemberOffset(X2APICPAGE, lvt_lint1,   XAPIC_OFF_LVT_LINT1);
AssertCompileMemberOffset(X2APICPAGE, lvt_error,   XAPIC_OFF_LVT_ERROR);
AssertCompileMemberOffset(X2APICPAGE, timer_icr,   XAPIC_OFF_TIMER_ICR);
AssertCompileMemberOffset(X2APICPAGE, timer_ccr,   XAPIC_OFF_TIMER_CCR);
AssertCompileMemberOffset(X2APICPAGE, timer_dcr,   XAPIC_OFF_TIMER_DCR);
AssertCompileMemberOffset(X2APICPAGE, self_ipi,    X2APIC_OFF_SELF_IPI);

/**
 * APIC MSR access error.
 * @note The values must match the array indices in apicMsrAccessError().
 */
typedef enum APICMSRACCESS
{
    /* MSR read while not in x2APIC. */
    APICMSRACCESS_INVALID_READ_MODE = 0,
    /* MSR write while not in x2APIC. */
    APICMSRACCESS_INVALID_WRITE_MODE,
    /* MSR read for a reserved/unknown/invalid MSR. */
    APICMSRACCESS_READ_RSVD_OR_UNKNOWN,
    /* MSR write for a reserved/unknown/invalid MSR. */
    APICMSRACCESS_WRITE_RSVD_OR_UNKNOWN,
    /* MSR read for a write-only MSR. */
    APICMSRACCESS_READ_WRITE_ONLY,
    /* MSR write for a read-only MSR. */
    APICMSRACCESS_WRITE_READ_ONLY,
    /* MSR read to reserved bits. */
    APICMSRACCESS_READ_RSVD_BITS,
    /* MSR write to reserved bits. */
    APICMSRACCESS_WRITE_RSVD_BITS,
    /* MSR write with invalid value. */
    APICMSRACCESS_WRITE_INVALID,
    /** MSR write disallowed due to incompatible config. */
    APICMSRACCESS_WRITE_DISALLOWED_CONFIG,
    /** MSR read disallowed due to incompatible config. */
    APICMSRACCESS_READ_DISALLOWED_CONFIG,
    /* Count of enum members (don't use). */
    APICMSRACCESS_COUNT
} APICMSRACCESS;

/** @name xAPIC Destination Format Register bits.
 * See Intel spec. 10.6.2.2 "Logical Destination Mode".
 * @{ */
typedef enum XAPICDESTFORMAT
{
    XAPICDESTFORMAT_FLAT    = 0xf,
    XAPICDESTFORMAT_CLUSTER = 0
} XAPICDESTFORMAT;
/** @} */

/** @name xAPIC Timer Mode bits.
 * See Intel spec. 10.5.1 "Local Vector Table".
 * @{ */
typedef enum XAPICTIMERMODE
{
    XAPICTIMERMODE_ONESHOT      = XAPIC_TIMER_MODE_ONESHOT,
    XAPICTIMERMODE_PERIODIC     = XAPIC_TIMER_MODE_PERIODIC,
    XAPICTIMERMODE_TSC_DEADLINE = XAPIC_TIMER_MODE_TSC_DEADLINE
} XAPICTIMERMODE;
/** @} */

/** @name xAPIC Interrupt Command Register bits.
 * See Intel spec. 10.6.1 "Interrupt Command Register (ICR)".
 * See Intel spec. 10.5.1 "Local Vector Table".
 * @{ */
/**
 * xAPIC destination shorthand.
 */
typedef enum XAPICDESTSHORTHAND
{
    XAPICDESTSHORTHAND_NONE = 0,
    XAPICDESTSHORTHAND_SELF,
    XAPIDDESTSHORTHAND_ALL_INCL_SELF,
    XAPICDESTSHORTHAND_ALL_EXCL_SELF
} XAPICDESTSHORTHAND;

/**
 * xAPIC INIT level de-assert delivery mode.
 */
typedef enum XAPICINITLEVEL
{
    XAPICINITLEVEL_DEASSERT = 0,
    XAPICINITLEVEL_ASSERT
} XAPICLEVEL;

/**
 * xAPIC destination mode.
 */
typedef enum XAPICDESTMODE
{
    XAPICDESTMODE_PHYSICAL = 0,
    XAPICDESTMODE_LOGICAL
} XAPICDESTMODE;

/**
 * xAPIC delivery mode type.
 */
typedef enum XAPICDELIVERYMODE
{
    XAPICDELIVERYMODE_FIXED               = 0,
    XAPICDELIVERYMODE_LOWEST_PRIO         = 1,
    XAPICDELIVERYMODE_SMI                 = 2,
    XAPICDELIVERYMODE_NMI                 = 4,
    XAPICDELIVERYMODE_INIT                = 5,
    XAPICDELIVERYMODE_STARTUP             = 6,
    XAPICDELIVERYMODE_EXTINT              = 7
} XAPICDELIVERYMODE;
/** @} */

/** @def APIC_CACHE_LINE_SIZE
 * Padding (in bytes) for aligning data in different cache lines. Present
 * generation x86 CPUs use 64-byte cache lines[1]. However, Intel NetBurst
 * architecture supposedly uses 128-byte cache lines[2]. Since 128 is a
 * multiple of 64, we use the larger one here.
 *
 * [1] - Intel spec "Table 11-1. Characteristics of the Caches, TLBs, Store
 *       Buffer, and Write Combining Buffer in Intel 64 and IA-32 Processors"
 * [2] - Intel spec. 8.10.6.7 "Place Locks and Semaphores in Aligned, 128-Byte
 *       Blocks of Memory".
 */
#define APIC_CACHE_LINE_SIZE              128

/**
 * APIC Pending-Interrupt Bitmap (PIB).
 */
typedef struct APICPIB
{
    uint64_t volatile au64VectorBitmap[4];
    uint32_t volatile fOutstandingNotification;
    uint8_t           au8Reserved[APIC_CACHE_LINE_SIZE - sizeof(uint32_t) - (sizeof(uint64_t) * 4)];
} APICPIB;
AssertCompileMemberOffset(APICPIB, fOutstandingNotification, 256 / 8);
AssertCompileSize(APICPIB, APIC_CACHE_LINE_SIZE);
/** Pointer to a pending-interrupt bitmap. */
typedef APICPIB *PAPICPIB;
/** Pointer to a const pending-interrupt bitmap. */
typedef const APICPIB *PCAPICPIB;

/**
 * APIC PDM instance data (per-VM).
 */
typedef struct APICDEV
{
    /** The device instance - R3 Ptr. */
    PPDMDEVINSR3                pDevInsR3;
    /** Alignment padding. */
    R3PTRTYPE(void *)           pvAlignment0;

    /** The device instance - R0 Ptr. */
    PPDMDEVINSR0                pDevInsR0;
    /** Alignment padding. */
    R0PTRTYPE(void *)           pvAlignment1;

    /** The device instance - RC Ptr. */
    PPDMDEVINSRC                pDevInsRC;
} APICDEV;
/** Pointer to an APIC device. */
typedef APICDEV *PAPICDEV;
/** Pointer to a const APIC device. */
typedef APICDEV const *PCAPICDEV;

/**
 * APIC VM Instance data.
 */
typedef struct APIC
{
    /** @name The APIC PDM device instance.
     * @{ */
    /** The APIC device - R0 ptr. */
    R0PTRTYPE(PAPICDEV)         pApicDevR0;
    /** The APIC device - R3 ptr. */
    R3PTRTYPE(PAPICDEV)         pApicDevR3;
    /** The APIC device - RC ptr. */
    RCPTRTYPE(PAPICDEV)         pApicDevRC;
    /** Alignment padding. */
    RTRCPTR                     RCPtrAlignment0;
    /** @} */

    /** @name The APIC pending-interrupt bitmap (PIB).
     * @{ */
    /** The host-context physical address of the PIB. */
    RTHCPHYS                    HCPhysApicPib;
    /** The ring-0 memory object of the PIB. */
    RTR0MEMOBJ                  hMemObjApicPibR0;
    /** The ring-3 mapping of the memory object of the PIB. */
    RTR0MEMOBJ                  hMapObjApicPibR0;
    /** The APIC PIB virtual address - R0 ptr. */
    R0PTRTYPE(void *)           pvApicPibR0;
    /** The APIC PIB virtual address - R3 ptr. */
    R3PTRTYPE(void *)           pvApicPibR3;
    /** The APIC PIB virtual address - RC ptr. */
    RCPTRTYPE(void *)           pvApicPibRC;
    /** Alignment padding. */
    RTRCPTR                     RCPtrAlignment1;
    /** The size of the page in bytes. */
    uint32_t                    cbApicPib;
    /** Alignment padding. */
    uint32_t                    u32Aligment0;
    /** @} */

    /** @name Other miscellaneous data.
     * @{ */
    /** Whether full APIC register virtualization is enabled. */
    bool                        fVirtApicRegsEnabled;
    /** Whether posted-interrupt processing is enabled. */
    bool                        fPostedIntrsEnabled;
    /** Whether TSC-deadline timer mode is supported for the guest. */
    bool                        fSupportsTscDeadline;
    /** Whether this VM has an IO-APIC. */
    bool                        fIoApicPresent;
    /** Whether RZ is enabled or not (applies to MSR handling as well). */
    bool                        fRZEnabled;
    /** Whether Hyper-V x2APIC compatibility mode is enabled. */
    bool                        fHyperVCompatMode;
    /** Alignment padding. */
    bool                        afAlignment[2];
    /** The max supported APIC mode from CFGM.  */
    PDMAPICMODE                 enmMaxMode;
    /** Alignment padding. */
    uint32_t                    u32Alignment1;
    /** @} */
} APIC;
/** Pointer to APIC VM instance data. */
typedef APIC *PAPIC;
/** Pointer to const APIC VM instance data. */
typedef APIC const *PCAPIC;
AssertCompileMemberAlignment(APIC, cbApicPib, 8);
AssertCompileMemberAlignment(APIC, fVirtApicRegsEnabled, 8);
AssertCompileMemberAlignment(APIC, enmMaxMode, 8);
AssertCompileSizeAlignment(APIC, 8);

/**
 * APIC VMCPU Instance data.
 */
typedef struct APICCPU
{
    /** @name The APIC page.
     * @{ */
    /** The host-context physical address of the page. */
    RTHCPHYS                    HCPhysApicPage;
    /** The ring-0 memory object of the page. */
    RTR0MEMOBJ                  hMemObjApicPageR0;
    /** The ring-3 mapping of the memory object of the page. */
    RTR0MEMOBJ                  hMapObjApicPageR0;
    /** The APIC page virtual address - R0 ptr. */
    R0PTRTYPE(void *)           pvApicPageR0;
    /** The APIC page virtual address - R3 ptr. */
    R3PTRTYPE(void *)           pvApicPageR3;
    /** The APIC page virtual address - RC ptr. */
    RCPTRTYPE(void *)           pvApicPageRC;
    /** Alignment padding. */
    RTRCPTR                     RCPtrAlignment0;
    /** The size of the page in bytes. */
    uint32_t                    cbApicPage;
    /** @} */

    /** @name Auxiliary state.
     * @{ */
    /** The error status register's internal state. */
    uint32_t                    uEsrInternal;
    /** The APIC base MSR.*/
    uint64_t volatile           uApicBaseMsr;
    /** @} */

    /** @name The pending-interrupt bitmaps (PIB).
     * @{ */
    /** The host-context physical address of the page. */
    RTHCPHYS                    HCPhysApicPib;
    /** The APIC PIB virtual address - R0 ptr. */
    R0PTRTYPE(void *)           pvApicPibR0;
    /** The APIC PIB virtual address - R3 ptr. */
    R3PTRTYPE(void *)           pvApicPibR3;
    /** The APIC PIB virtual address - RC ptr. */
    RCPTRTYPE(void *)           pvApicPibRC;
    /** Alignment padding. */
    RTRCPTR                     RCPtrAlignment1;
    /** The APIC PIB for level-sensitive interrupts. */
    APICPIB                     ApicPibLevel;
    /** @} */

    /** @name Other miscellaneous data.
     * @{ */
    /** Whether the LINT0 interrupt line is active. */
    bool volatile               fActiveLint0;
    /** Whether the LINT1 interrupt line is active. */
    bool volatile               fActiveLint1;
    /** Alignment padding. */
    uint8_t                     auAlignment0[6];
    /** The source tags corresponding to each interrupt vector (debugging). */
    uint32_t                    auSrcTags[256];
    /** @} */

    /** @name The APIC timer.
     * @{ */
    /** The timer - R0 ptr. */
    PTMTIMERR0                  pTimerR0;
    /** The timer - R3 ptr. */
    PTMTIMERR3                  pTimerR3;
    /** The timer - RC ptr. */
    PTMTIMERRC                  pTimerRC;
    /** Alignment padding. */
    RTRCPTR                     RCPtrAlignment2;
    /** The timer critical sect protecting @a u64TimerInitial  */
    PDMCRITSECT                 TimerCritSect;
    /** The time stamp when the timer was initialized. */
    uint64_t                    u64TimerInitial;
    /** Cache of timer initial count of the frequency hint to TM. */
    uint32_t                    uHintedTimerInitialCount;
    /** Cache of timer shift of the frequency hint to TM. */
    uint32_t                    uHintedTimerShift;
    /** The timer description. */
    char                        szTimerDesc[32];
    /** @} */

#ifdef VBOX_WITH_STATISTICS
    /** @name APIC statistics.
     * @{ */
    /** Number of MMIO reads in RZ. */
    STAMCOUNTER                 StatMmioReadRZ;
    /** Number of MMIO reads in R3. */
    STAMCOUNTER                 StatMmioReadR3;

    /** Number of MMIO writes in RZ. */
    STAMCOUNTER                 StatMmioWriteRZ;
    /** Number of MMIO writes in R3. */
    STAMCOUNTER                 StatMmioWriteR3;

    /** Number of MSR reads in RZ. */
    STAMCOUNTER                 StatMsrReadRZ;
    /** Number of MSR reads in R3. */
    STAMCOUNTER                 StatMsrReadR3;

    /** Number of MSR writes in RZ. */
    STAMCOUNTER                 StatMsrWriteRZ;
    /** Number of MSR writes in R3. */
    STAMCOUNTER                 StatMsrWriteR3;

    /** Profiling of APICUpdatePendingInterrupts().  */
    STAMPROFILE                 StatUpdatePendingIntrs;
    /** Profiling of apicPostInterrupt().  */
    STAMPROFILE                 StatPostIntr;
    /** Number of times an interrupt is already pending in
     *  apicPostInterrupts().*/
    STAMCOUNTER                 StatPostIntrAlreadyPending;
    /** Number of times the timer callback is invoked. */
    STAMCOUNTER                 StatTimerCallback;
    /** Number of times the TPR is written. */
    STAMCOUNTER                 StatTprWrite;
    /** Number of times the TPR is read. */
    STAMCOUNTER                 StatTprRead;
    /** Number of times the EOI is written. */
    STAMCOUNTER                 StatEoiWrite;
    /** Number of times TPR masks an interrupt in apicGetInterrupt(). */
    STAMCOUNTER                 StatMaskedByTpr;
    /** Number of times PPR masks an interrupt in apicGetInterrupt(). */
    STAMCOUNTER                 StatMaskedByPpr;
    /** Number of times the timer ICR is written. */
    STAMCOUNTER                 StatTimerIcrWrite;
    /** Number of times the ICR Lo (send IPI) is written. */
    STAMCOUNTER                 StatIcrLoWrite;
    /** Number of times the ICR Hi is written. */
    STAMCOUNTER                 StatIcrHiWrite;
    /** Number of times the full ICR (x2APIC send IPI) is written. */
    STAMCOUNTER                 StatIcrFullWrite;
    /** @} */
#endif
} APICCPU;
/** Pointer to APIC VMCPU instance data. */
typedef APICCPU *PAPICCPU;
/** Pointer to a const APIC VMCPU instance data. */
typedef APICCPU const *PCAPICCPU;
AssertCompileMemberAlignment(APICCPU, uApicBaseMsr, 8);

/**
 * APIC operating modes as returned by apicGetMode().
 *
 * The values match hardware states.
 * See Intel spec. 10.12.1 "Detecting and Enabling x2APIC Mode".
 */
typedef enum APICMODE
{
    APICMODE_DISABLED = 0,
    APICMODE_INVALID,
    APICMODE_XAPIC,
    APICMODE_X2APIC
} APICMODE;

/**
 * Gets the timer shift value.
 *
 * @returns The timer shift value.
 * @param   pXApicPage      The xAPIC page.
 */
DECLINLINE(uint8_t) apicGetTimerShift(PCXAPICPAGE pXApicPage)
{
    /* See Intel spec. 10.5.4 "APIC Timer". */
    uint32_t uShift = pXApicPage->timer_dcr.u.u2DivideValue0 | (pXApicPage->timer_dcr.u.u1DivideValue1 << 2);
    return (uShift + 1) & 7;
}

RT_C_DECLS_BEGIN


/** @def APICBOTHCBDECL
 * Macro for declaring a callback which is static in HC and exported in GC.
 */
#if defined(IN_RC) || defined(IN_RING0)
# define APICBOTHCBDECL(type)    DECLEXPORT(type)
#else
# define APICBOTHCBDECL(type)    DECLCALLBACK(type)
#endif

const char                   *apicGetModeName(APICMODE enmMode);
const char                   *apicGetDestFormatName(XAPICDESTFORMAT enmDestFormat);
const char                   *apicGetDeliveryModeName(XAPICDELIVERYMODE enmDeliveryMode);
const char                   *apicGetDestModeName(XAPICDESTMODE enmDestMode);
const char                   *apicGetTriggerModeName(XAPICTRIGGERMODE enmTriggerMode);
const char                   *apicGetDestShorthandName(XAPICDESTSHORTHAND enmDestShorthand);
const char                   *apicGetTimerModeName(XAPICTIMERMODE enmTimerMode);
void                          apicHintTimerFreq(PAPICCPU pApicCpu, uint32_t uInitialCount, uint8_t uTimerShift);
APICMODE                      apicGetMode(uint64_t uApicBaseMsr);

APICBOTHCBDECL(uint64_t)      apicGetBaseMsr(PPDMDEVINS pDevIns, PVMCPU pVCpu);
APICBOTHCBDECL(VBOXSTRICTRC)  apicSetBaseMsr(PPDMDEVINS pDevIns, PVMCPU pVCpu, uint64_t uBase);
APICBOTHCBDECL(uint8_t)       apicGetTpr(PPDMDEVINS pDevIns, PVMCPU pVCpu, bool *pfPending, uint8_t *pu8PendingIntr);
APICBOTHCBDECL(void)          apicSetTpr(PPDMDEVINS pDevIns, PVMCPU pVCpu, uint8_t u8Tpr);
APICBOTHCBDECL(uint64_t)      apicGetTimerFreq(PPDMDEVINS pDevIns);
APICBOTHCBDECL(int)           apicReadMmio(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb);
APICBOTHCBDECL(int)           apicWriteMmio(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb);
APICBOTHCBDECL(VBOXSTRICTRC)  apicReadMsr(PPDMDEVINS pDevIns,  PVMCPU pVCpu, uint32_t u32Reg, uint64_t *pu64Val);
APICBOTHCBDECL(VBOXSTRICTRC)  apicWriteMsr(PPDMDEVINS pDevIns, PVMCPU pVCpu, uint32_t u32Reg, uint64_t u64Val);
APICBOTHCBDECL(int)           apicGetInterrupt(PPDMDEVINS pDevIns,  PVMCPU pVCpu, uint8_t *puVector, uint32_t *puTagSrc);
APICBOTHCBDECL(VBOXSTRICTRC)  apicLocalInterrupt(PPDMDEVINS pDevIns, PVMCPU pVCpu, uint8_t u8Pin, uint8_t u8Level, int rcRZ);
APICBOTHCBDECL(int)           apicBusDeliver(PPDMDEVINS pDevIns, uint8_t uDest, uint8_t uDestMode, uint8_t uDeliveryMode,
                                             uint8_t uVector, uint8_t uPolarity, uint8_t uTriggerMode, uint32_t uSrcTag);

VMM_INT_DECL(bool)            apicPostInterrupt(PVMCPU pVCpu, uint8_t uVector, XAPICTRIGGERMODE enmTriggerMode, uint32_t uSrcTag);
VMM_INT_DECL(void)            apicStartTimer(PVMCPU pVCpu, uint32_t uInitialCount);
VMM_INT_DECL(void)            apicStopTimer(PVMCPU pVCpu);
VMM_INT_DECL(void)            apicSetInterruptFF(PVMCPU pVCpu, PDMAPICIRQ enmType);
VMM_INT_DECL(void)            apicClearInterruptFF(PVMCPU pVCpu, PDMAPICIRQ enmType);

#ifdef IN_RING3
VMMR3_INT_DECL(void)          apicR3ResetCpu(PVMCPU pVCpu, bool fResetApicBaseMsr);
#endif

RT_C_DECLS_END

/** @} */

#endif


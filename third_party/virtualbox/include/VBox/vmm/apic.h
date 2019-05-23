/** @file
 * APIC - Advanced Programmable Interrupt Controller.
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

#ifndef ___VBox_vmm_apic_h
#define ___VBox_vmm_apic_h

#include <VBox/vmm/pdmins.h>
#include <VBox/vmm/pdmdev.h>

/** @defgroup grp_apic   The local APIC VMM API
 * @ingroup grp_vmm
 * @{
 */

/** Offset of APIC ID Register. */
#define XAPIC_OFF_ID                         0x020
/** Offset of APIC Version Register. */
#define XAPIC_OFF_VERSION                    0x030
/** Offset of Task Priority Register. */
#define XAPIC_OFF_TPR                        0x080
/** Offset of Arbitrartion Priority register. */
#define XAPIC_OFF_APR                        0x090
/** Offset of Processor Priority register. */
#define XAPIC_OFF_PPR                        0x0A0
/** Offset of End Of Interrupt register. */
#define XAPIC_OFF_EOI                        0x0B0
/** Offset of Remote Read Register. */
#define XAPIC_OFF_RRD                        0x0C0
/** Offset of Logical Destination Register. */
#define XAPIC_OFF_LDR                        0x0D0
/** Offset of Destination Format Register. */
#define XAPIC_OFF_DFR                        0x0E0
/** Offset of Spurious Interrupt Vector Register. */
#define XAPIC_OFF_SVR                        0x0F0
/** Offset of In-service Register (bits 31:0). */
#define XAPIC_OFF_ISR0                       0x100
/** Offset of In-service Register (bits 63:32). */
#define XAPIC_OFF_ISR1                       0x110
/** Offset of In-service Register (bits 95:64). */
#define XAPIC_OFF_ISR2                       0x120
/** Offset of In-service Register (bits 127:96). */
#define XAPIC_OFF_ISR3                       0x130
/** Offset of In-service Register (bits 159:128). */
#define XAPIC_OFF_ISR4                       0x140
/** Offset of In-service Register (bits 191:160). */
#define XAPIC_OFF_ISR5                       0x150
/** Offset of In-service Register (bits 223:192). */
#define XAPIC_OFF_ISR6                       0x160
/** Offset of In-service Register (bits 255:224). */
#define XAPIC_OFF_ISR7                       0x170
/** Offset of Trigger Mode Register (bits 31:0). */
#define XAPIC_OFF_TMR0                       0x180
/** Offset of Trigger Mode Register (bits 63:32). */
#define XAPIC_OFF_TMR1                       0x190
/** Offset of Trigger Mode Register (bits 95:64). */
#define XAPIC_OFF_TMR2                       0x1A0
/** Offset of Trigger Mode Register (bits 127:96). */
#define XAPIC_OFF_TMR3                       0x1B0
/** Offset of Trigger Mode Register (bits 159:128). */
#define XAPIC_OFF_TMR4                       0x1C0
/** Offset of Trigger Mode Register (bits 191:160). */
#define XAPIC_OFF_TMR5                       0x1D0
/** Offset of Trigger Mode Register (bits 223:192). */
#define XAPIC_OFF_TMR6                       0x1E0
/** Offset of Trigger Mode Register (bits 255:224). */
#define XAPIC_OFF_TMR7                       0x1F0
/** Offset of Interrupt Request Register (bits 31:0). */
#define XAPIC_OFF_IRR0                       0x200
/** Offset of Interrupt Request Register (bits 63:32). */
#define XAPIC_OFF_IRR1                       0x210
/** Offset of Interrupt Request Register (bits 95:64). */
#define XAPIC_OFF_IRR2                       0x220
/** Offset of Interrupt Request Register (bits 127:96). */
#define XAPIC_OFF_IRR3                       0x230
/** Offset of Interrupt Request Register (bits 159:128). */
#define XAPIC_OFF_IRR4                       0x240
/** Offset of Interrupt Request Register (bits 191:160). */
#define XAPIC_OFF_IRR5                       0x250
/** Offset of Interrupt Request Register (bits 223:192). */
#define XAPIC_OFF_IRR6                       0x260
/** Offset of Interrupt Request Register (bits 255:224). */
#define XAPIC_OFF_IRR7                       0x270
/** Offset of Error Status Register. */
#define XAPIC_OFF_ESR                        0x280
/** Offset of LVT CMCI Register. */
#define XAPIC_OFF_LVT_CMCI                   0x2F0
/** Offset of Interrupt Command Register - Lo. */
#define XAPIC_OFF_ICR_LO                     0x300
/** Offset of Interrupt Command Register - Hi. */
#define XAPIC_OFF_ICR_HI                     0x310
/** Offset of LVT Timer Register. */
#define XAPIC_OFF_LVT_TIMER                  0x320
/** Offset of LVT Thermal Sensor Register. */
#define XAPIC_OFF_LVT_THERMAL                0x330
/** Offset of LVT Performance Counter Register. */
#define XAPIC_OFF_LVT_PERF                   0x340
/** Offset of LVT LINT0 Register. */
#define XAPIC_OFF_LVT_LINT0                  0x350
/** Offset of LVT LINT1 Register. */
#define XAPIC_OFF_LVT_LINT1                  0x360
/** Offset of LVT Error Register . */
#define XAPIC_OFF_LVT_ERROR                  0x370
/** Offset of Timer Initial Count Register. */
#define XAPIC_OFF_TIMER_ICR                  0x380
/** Offset of Timer Current Count Register. */
#define XAPIC_OFF_TIMER_CCR                  0x390
/** Offset of Timer Divide Configuration Register. */
#define XAPIC_OFF_TIMER_DCR                  0x3E0
/** Offset of Self-IPI Register (x2APIC only). */
#define X2APIC_OFF_SELF_IPI                  0x3F0

/** Offset of LVT range start. */
#define XAPIC_OFF_LVT_START                  XAPIC_OFF_LVT_TIMER
/** Offset of LVT range end (inclusive).  */
#define XAPIC_OFF_LVT_END                    XAPIC_OFF_LVT_ERROR
/** Offset of LVT extended range start. */
#define XAPIC_OFF_LVT_EXT_START              XAPIC_OFF_LVT_CMCI
/** Offset of LVT extended range end (inclusive). */
#define XAPIC_OFF_LVT_EXT_END                XAPIC_OFF_LVT_CMCI

/**
 * xAPIC trigger mode.
 */
typedef enum XAPICTRIGGERMODE
{
    XAPICTRIGGERMODE_EDGE = 0,
    XAPICTRIGGERMODE_LEVEL
} XAPICTRIGGERMODE;

RT_C_DECLS_BEGIN

#ifdef IN_RING3
/** @defgroup grp_apic_r3  The APIC Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(void)        APICR3InitIpi(PVMCPU pVCpu);
VMMR3_INT_DECL(void)        APICR3HvEnable(PVM pVM);
/** @} */
#endif /* IN_RING3 */

/* These functions are exported as they are called from external modules (recompiler). */
VMMDECL(void)               APICUpdatePendingInterrupts(PVMCPU pVCpu);
VMMDECL(int)                APICGetTpr(PVMCPU pVCpu, uint8_t *pu8Tpr, bool *pfPending, uint8_t *pu8PendingIntr);
VMMDECL(int)                APICSetTpr(PVMCPU pVCpu, uint8_t u8Tpr);

/* These functions are VMM internal. */
VMM_INT_DECL(bool)          APICIsEnabled(PVMCPU pVCpu);
VMM_INT_DECL(bool)          APICGetHighestPendingInterrupt(PVMCPU pVCpu, uint8_t *pu8PendingIntr);
VMM_INT_DECL(bool)          APICQueueInterruptToService(PVMCPU pVCpu, uint8_t u8PendingIntr);
VMM_INT_DECL(void)          APICDequeueInterruptFromService(PVMCPU pVCpu, uint8_t u8PendingIntr);
VMM_INT_DECL(VBOXSTRICTRC)  APICReadMsr(PVMCPU pVCpu, uint32_t u32Reg, uint64_t *pu64Value);
VMM_INT_DECL(VBOXSTRICTRC)  APICWriteMsr(PVMCPU pVCpu, uint32_t u32Reg, uint64_t u64Value);
VMM_INT_DECL(int)           APICGetTimerFreq(PVM pVM, uint64_t *pu64Value);
VMM_INT_DECL(VBOXSTRICTRC)  APICLocalInterrupt(PVMCPU pVCpu, uint8_t u8Pin, uint8_t u8Level, int rcRZ);
VMM_INT_DECL(uint64_t)      APICGetBaseMsrNoCheck(PVMCPU pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  APICGetBaseMsr(PVMCPU pVCpu, uint64_t *pu64Value);
VMM_INT_DECL(VBOXSTRICTRC)  APICSetBaseMsr(PVMCPU pVCpu, uint64_t u64BaseMsr);
VMM_INT_DECL(int)           APICGetInterrupt(PVMCPU pVCpu, uint8_t *pu8Vector, uint32_t *pu32TagSrc);
VMM_INT_DECL(int)           APICBusDeliver(PVM pVM, uint8_t uDest, uint8_t uDestMode, uint8_t uDeliveryMode, uint8_t uVector,
                                           uint8_t uPolarity, uint8_t uTriggerMode, uint32_t uTagSrc);
VMM_INT_DECL(int)           APICGetApicPageForCpu(PVMCPU pVCpu, PRTHCPHYS pHCPhys, PRTR0PTR pR0Ptr, PRTR3PTR pR3Ptr,
                                                  PRTRCPTR pRCPtr);

/** @name Hyper-V interface (Ring-3 and all-context API).
 * @{ */
#ifdef IN_RING3
VMMR3_INT_DECL(void)        APICR3HvSetCompatMode(PVM pVM, bool fHyperVCompatMode);
#endif
VMM_INT_DECL(void)          APICHvSendInterrupt(PVMCPU pVCpu, uint8_t uVector, bool fAutoEoi, XAPICTRIGGERMODE enmTriggerMode);
VMM_INT_DECL(VBOXSTRICTRC)  APICHvSetTpr(PVMCPU pVCpu, uint8_t uTpr);
VMM_INT_DECL(uint8_t)       APICHvGetTpr(PVMCPU pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  APICHvSetIcr(PVMCPU pVCpu, uint64_t uIcr);
VMM_INT_DECL(uint64_t)      APICHvGetIcr(PVMCPU pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  APICHvSetEoi(PVMCPU pVCpu, uint32_t uEoi);
/** @} */

RT_C_DECLS_END

extern const PDMDEVREG      g_DeviceAPIC;
/** @} */

#endif


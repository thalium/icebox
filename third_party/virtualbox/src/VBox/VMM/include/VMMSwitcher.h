/* $Id: VMMSwitcher.h $ */
/** @file
 * VMM - World Switchers.
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

#ifndef ___VMMSwitcher_h
#define ___VMMSwitcher_h

#include <VBox/vmm/vmm.h>

/** @name   Fixup Types.
 * @{
 */
/** @todo document what arguments these take and what they do. */
#define FIX_HC_2_GC_NEAR_REL    1
#define FIX_HC_2_ID_NEAR_REL    2
#define FIX_GC_2_HC_NEAR_REL    3
#define FIX_GC_2_ID_NEAR_REL    4
#define FIX_ID_2_HC_NEAR_REL    5
#define FIX_ID_2_GC_NEAR_REL    6
#define FIX_GC_FAR32            7
#define FIX_GC_CPUM_OFF         8
#define FIX_GC_VM_OFF           9   /** @todo cleanup fixup names GC->RC, HC->R0. */
#define FIX_HC_CPUM_OFF         10
#define FIX_HC_VM_OFF           11
#define FIX_INTER_32BIT_CR3     12
#define FIX_INTER_PAE_CR3       13
#define FIX_INTER_AMD64_CR3     14
#define FIX_HYPER_CS            18
#define FIX_HYPER_DS            19
#define FIX_HYPER_TSS           20
#define FIX_GC_TSS_GDTE_DW2     21
/*#define FIX_NO_FXSAVE_JMP       24 - reusable */
#define FIX_NO_SYSENTER_JMP     25
#define FIX_NO_SYSCALL_JMP      26
#define FIX_HC_32BIT            27
#define FIX_HC_64BIT            28
#define FIX_HC_64BIT_CPUM       29
#define FIX_HC_64BIT_CS         30
#define FIX_ID_32BIT            31
#define FIX_ID_64BIT            32
#define FIX_ID_FAR32_TO_64BIT_MODE 33
#define FIX_GC_APIC_BASE_32BIT  34
#define FIX_HC_64BIT_NOCHECK    35
#define FIX_GC_64_BIT_CPUM_OFF  36
#define FIX_GC_CPUMCPU_OFF      37
#define FIX_EFER_OR_MASK        38
#define FIX_THE_END             255
/** @} */


/** Pointer to a switcher definition. */
typedef struct VMMSWITCHERDEF *PVMMSWITCHERDEF;

/**
 * Callback function for relocating the core code belonging to a switcher.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pSwitcher   Pointer to the switcher structure.
 * @param   R0PtrCode   Pointer to the first code byte in the ring-0 mapping.
 * @param   pu8CodeR3   Pointer to the first code byte in the ring-3 mapping.
 * @param   GCPtrCode   The GC address of the first code byte.
 * @param   u32IDCode   The address of the identity mapped code (first byte).
 */
typedef DECLCALLBACK(void) FNVMMSWITCHERRELOCATE(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3,
                                                 RTGCPTR GCPtrCode, uint32_t u32IDCode);
/** Pointer to a FNVMMSWITCHERRELOCATE(). */
typedef FNVMMSWITCHERRELOCATE *PFNVMMSWITCHERRELOCATE;

/**
 * VMM Switcher structure.
 */
#pragma pack(1)
typedef struct VMMSWITCHERDEF
{
    /** Pointer to the code. */
    void       *pvCode;
    /** Pointer to the fixup records. */
    void       *pvFixups;
    /** Pointer to the description. */
    const char *pszDesc;
    /** Function which performs the necessary relocations. */
    PFNVMMSWITCHERRELOCATE pfnRelocate;
    /** The switcher type. */
    VMMSWITCHER enmType;
    /** Size of the entire code chunk. */
    uint32_t    cbCode;
    /** vmmR0ToRawMode C entrypoint. */
    uint32_t    offR0ToRawMode;
    /** vmmRCToHost C entrypoint. */
    uint32_t    offRCToHost;
    /** vmmRCCallTrampoline address. */
    uint32_t    offRCCallTrampoline;
    /** vmmRCToHostAsm - Assembly language entry point for switching from raw-mode
     *  context to host-context.  This saves the RC register context.  */
    uint32_t    offRCToHostAsm;
    /** vmmRCToHostNoReturn - Assembly language entry point for switching from
     *  raw-mode context to host-context.  This does not save any RC register
     *  context and expects the caller to have done that already. */
    uint32_t    offRCToHostAsmNoReturn;
    /** @name Disassembly Regions.
     * @{ */
    uint32_t    offHCCode0;
    uint32_t    cbHCCode0;
    uint32_t    offHCCode1;
    uint32_t    cbHCCode1;
    uint32_t    offIDCode0;
    uint32_t    cbIDCode0;
    uint32_t    offIDCode1;
    uint32_t    cbIDCode1;
    uint32_t    offGCCode;
    uint32_t    cbGCCode;
    /** @} */
} VMMSWITCHERDEF;
#pragma pack()

RT_C_DECLS_BEGIN
extern VMMSWITCHERDEF vmmR3SwitcherX86Stub_Def;
extern VMMSWITCHERDEF vmmR3Switcher32BitTo32Bit_Def;
extern VMMSWITCHERDEF vmmR3Switcher32BitToPAE_Def;
extern VMMSWITCHERDEF vmmR3Switcher32BitToAMD64_Def;
extern VMMSWITCHERDEF vmmR3SwitcherPAETo32Bit_Def;
extern VMMSWITCHERDEF vmmR3SwitcherPAEToPAE_Def;
extern VMMSWITCHERDEF vmmR3SwitcherPAEToAMD64_Def;
extern VMMSWITCHERDEF vmmR3SwitcherAMD64Stub_Def;
extern VMMSWITCHERDEF vmmR3SwitcherAMD64To32Bit_Def;
extern VMMSWITCHERDEF vmmR3SwitcherAMD64ToPAE_Def;
extern VMMSWITCHERDEF vmmR3SwitcherAMD64ToAMD64_Def;

extern DECLCALLBACK(void) vmmR3Switcher32BitTo32Bit_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3Switcher32BitToPAE_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3Switcher32BitToAMD64_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3SwitcherPAETo32Bit_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3SwitcherPAEToPAE_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3SwitcherPAEToAMD64_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3SwitcherAMD64To32Bit_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3SwitcherAMD64ToPAE_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
extern DECLCALLBACK(void) vmmR3SwitcherAMD64ToAMD64_Relocate(PVM pVM, PVMMSWITCHERDEF pSwitcher, RTR0PTR R0PtrCode, uint8_t *pu8CodeR3, RTGCPTR GCPtrCode, uint32_t u32IdCode);
RT_C_DECLS_END

#endif

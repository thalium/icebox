/** @file
 * X86 (and AMD64) Local APIC registers (VMM,++).
 *
 * apic.mac is generated from this file by running 'kmk incs' in the root.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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

#ifndef ___VBox_apic_h
#define ___VBox_apic_h

#include <iprt/types.h>

#define APIC_REG_VERSION                        0x0030
#define APIC_REG_VERSION_GET_VER(u32)           (u32 & 0xff)
#define APIC_REG_VERSION_GET_MAX_LVT(u32)       ((u32 & 0xff0000) >> 16)

/* Defines according to Figure 10-8 of the Intel Software Developers Manual Vol 3A */
#define APIC_REG_LVT_LINT0                      0x0350
#define APIC_REG_LVT_LINT1                      0x0360
#define APIC_REG_LVT_ERR                        0x0370
#define APIC_REG_LVT_PC                         0x0340
#define APIC_REG_LVT_THMR                       0x0330
#define APIC_REG_LVT_CMCI                       0x02F0
#define APIC_REG_EILVT0                         0x0500
#define APIC_REG_EILVT1                         0x0510
#define APIC_REG_EILVT2                         0x0520
#define APIC_REG_EILVT3                         0x0530
#define APIC_REG_LVT_MODE_MASK                  (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
#define APIC_REG_LVT_MODE_FIXED                 0
#define APIC_REG_LVT_MODE_NMI                   RT_BIT(10)
#define APIC_REG_LVT_MODE_EXTINT                (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
#define APIC_REG_LVT_PIN_POLARIY                RT_BIT(13)
#define APIC_REG_LVT_REMOTE_IRR                 RT_BIT(14)
#define APIC_REG_LVT_LEVEL_TRIGGER              RT_BIT(15)
#define APIC_REG_LVT_MASKED                     RT_BIT(16)

DECLINLINE(uint32_t) ApicRegRead(void *pvBase, uint32_t offReg)
{
    return *(const volatile uint32_t *)((uintptr_t)pvBase + offReg);
}


#ifdef ___iprt_asm_amd64_x86_h
/**
 * Reads an X2APIC register.
 *
 * @param   offReg      MMIO offset, APIC_REG_XXX.
 */
DECLINLINE(uint32_t) ApicX2RegRead32(uint32_t offReg)
{
    return ASMRdMsr((offReg >> 4) + MSR_IA32_X2APIC_START);
}
#endif

#endif


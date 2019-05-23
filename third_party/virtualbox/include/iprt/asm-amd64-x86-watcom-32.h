/** @file
 * IPRT - AMD64 and x86 Specific Assembly Functions, 32-bit Watcom C pragma aux.
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

#ifndef ___iprt_asm_amd64_x86_h
# error "Don't include this header directly."
#endif
#ifndef ___iprt_asm_amd64_x86_watcom_32_h
#define ___iprt_asm_amd64_x86_watcom_32_h

#ifndef __FLAT__
# error "Only works with flat pointers! (-mf)"
#endif

/*
 * Note! The #undef that preceds the #pragma aux statements is for undoing
 *       the mangling, because the symbol in #pragma aux [symbol] statements
 *       doesn't get subjected to preprocessing.  This is also why we include
 *       the watcom header at the top rather than at the bottom of the
 *       asm-amd64-x86.h file.
 */

#undef      ASMGetIDTR
#pragma aux ASMGetIDTR = \
    "sidt fword ptr [ecx]" \
    parm [ecx] \
    modify exact [];

#undef      ASMGetIdtrLimit
#pragma aux ASMGetIdtrLimit = \
    "sub  esp, 8" \
    "sidt fword ptr [esp]" \
    "mov  cx, [esp]" \
    "add  esp, 8" \
    parm [] \
    value [cx] \
    modify exact [ecx];

#undef      ASMSetIDTR
#pragma aux ASMSetIDTR = \
    "lidt fword ptr [ecx]" \
    parm [ecx] nomemory \
    modify nomemory;

#undef      ASMGetGDTR
#pragma aux ASMGetGDTR = \
    "sgdt fword ptr [ecx]" \
    parm [ecx] \
    modify exact [];

#undef      ASMSetGDTR
#pragma aux ASMSetGDTR = \
    "lgdt fword ptr [ecx]" \
    parm [ecx] nomemory \
    modify exact [] nomemory;

#undef      ASMGetCS
#pragma aux ASMGetCS = \
    "mov ax, cs" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

#undef      ASMGetDS
#pragma aux ASMGetDS = \
    "mov ax, ds" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

#undef      ASMGetES
#pragma aux ASMGetES = \
    "mov ax, es" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

#undef      ASMGetFS
#pragma aux ASMGetFS = \
    "mov ax, fs" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

#undef      ASMGetGS
#pragma aux ASMGetGS = \
    "mov ax, gs" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

#undef      ASMGetSS
#pragma aux ASMGetSS = \
    "mov ax, ss" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

#undef      ASMGetTR
#pragma aux ASMGetTR = \
    "str ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

#undef      ASMGetLDTR
#pragma aux ASMGetLDTR = \
    "sldt ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [eax] nomemory;

/** @todo ASMGetSegAttr   */

#undef      ASMGetFlags
#pragma aux ASMGetFlags = \
    "pushfd" \
    "pop eax" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMSetFlags
#pragma aux ASMSetFlags = \
    "push eax" \
    "popfd" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMChangeFlags
#pragma aux ASMChangeFlags = \
    "pushfd" \
    "pop eax" \
    "and edx, eax" \
    "or  edx, ecx" \
    "push edx" \
    "popfd" \
    parm [edx] [ecx] nomemory \
    value [eax] \
    modify exact [edx] nomemory;

#undef      ASMAddFlags
#pragma aux ASMAddFlags = \
    "pushfd" \
    "pop eax" \
    "or  edx, eax" \
    "push edx" \
    "popfd" \
    parm [edx] nomemory \
    value [eax] \
    modify exact [edx] nomemory;

#undef      ASMClearFlags
#pragma aux ASMClearFlags = \
    "pushfd" \
    "pop eax" \
    "and edx, eax" \
    "push edx" \
    "popfd" \
    parm [edx] nomemory \
    value [eax] \
    modify exact [edx] nomemory;

/* Note! Must use the 64-bit integer return value convension.
         The order of registers in the value [set] does not seem to mean anything. */
#undef      ASMReadTSC
#pragma aux ASMReadTSC = \
    ".586" \
    "rdtsc" \
    parm [] nomemory \
    value [eax edx] \
    modify exact [edx eax] nomemory;

#undef      ASMReadTscWithAux
#pragma aux ASMReadTscWithAux = \
    0x0f 0x01 0xf9 \
    "mov [ebx], ecx" \
    parm [ebx] \
    value [eax edx] \
    modify exact [eax edx ecx];

/* ASMCpuId: Implemented externally, too many parameters. */
/* ASMCpuId_Idx_ECX: Implemented externally, too many parameters. */
/* ASMCpuIdExSlow: Always implemented externally. */

#undef      ASMCpuId_ECX_EDX
#pragma aux ASMCpuId_ECX_EDX = \
    "cpuid" \
    "mov [edi], ecx" \
    "mov [esi], edx" \
    parm [eax] [edi] [esi] \
    modify exact [eax ebx ecx edx];

#undef      ASMCpuId_EAX
#pragma aux ASMCpuId_EAX = \
    "cpuid" \
    parm [eax] \
    value [eax] \
    modify exact [eax ebx ecx edx];

#undef      ASMCpuId_EBX
#pragma aux ASMCpuId_EBX = \
    "cpuid" \
    parm [eax] \
    value [ebx] \
    modify exact [eax ebx ecx edx];

#undef      ASMCpuId_ECX
#pragma aux ASMCpuId_ECX = \
    "cpuid" \
    parm [eax] \
    value [ecx] \
    modify exact [eax ebx ecx edx];

#undef      ASMCpuId_EDX
#pragma aux ASMCpuId_EDX = \
    "cpuid" \
    parm [eax] \
    value [edx] \
    modify exact [eax ebx ecx edx];

/* ASMHasCpuId: MSC inline in main source file. */
/* ASMGetApicId: Implemented externally, lazy bird. */

#undef      ASMGetCR0
#pragma aux ASMGetCR0 = \
    "mov eax, cr0" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMSetCR0
#pragma aux ASMSetCR0 = \
    "mov cr0, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMGetCR2
#pragma aux ASMGetCR2 = \
    "mov eax, cr2" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMSetCR2
#pragma aux ASMSetCR2 = \
    "mov cr2, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMGetCR3
#pragma aux ASMGetCR3 = \
    "mov eax, cr3" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMSetCR3
#pragma aux ASMSetCR3 = \
    "mov cr3, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMReloadCR3
#pragma aux ASMReloadCR3 = \
    "mov eax, cr3" \
    "mov cr3, eax" \
    parm [] nomemory \
    modify exact [eax] nomemory;

#undef      ASMGetCR4
#pragma aux ASMGetCR4 = \
    "mov eax, cr4" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMSetCR4
#pragma aux ASMSetCR4 = \
    "mov cr4, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

/* ASMGetCR8: Don't bother for 32-bit. */
/* ASMSetCR8: Don't bother for 32-bit. */

#undef      ASMIntEnable
#pragma aux ASMIntEnable = \
    "sti" \
    parm [] nomemory \
    modify exact [] nomemory;

#undef      ASMIntDisable
#pragma aux ASMIntDisable = \
    "cli" \
    parm [] nomemory \
    modify exact [] nomemory;

#undef      ASMIntDisableFlags
#pragma aux ASMIntDisableFlags = \
    "pushfd" \
    "cli" \
    "pop eax" \
    parm [] nomemory \
    value [eax] \
    modify exact [] nomemory;

#undef      ASMHalt
#pragma aux ASMHalt = \
    "hlt" \
    parm [] nomemory \
    modify exact [] nomemory;

#undef      ASMRdMsr
#pragma aux ASMRdMsr = \
    ".586" \
    "rdmsr" \
    parm [ecx] nomemory \
    value [eax edx] \
    modify exact [eax edx] nomemory;

#undef      ASMWrMsr
#pragma aux ASMWrMsr = \
    ".586" \
    "wrmsr" \
    parm [ecx] [eax edx] nomemory \
    modify exact [] nomemory;

#undef      ASMRdMsrEx
#pragma aux ASMRdMsrEx = \
    ".586" \
    "rdmsr" \
    parm [ecx] [edi] nomemory \
    value [eax edx] \
    modify exact [eax edx] nomemory;

#undef      ASMWrMsrEx
#pragma aux ASMWrMsrEx = \
    ".586" \
    "wrmsr" \
    parm [ecx] [edi] [eax edx] nomemory \
    modify exact [] nomemory;

#undef      ASMRdMsr_Low
#pragma aux ASMRdMsr_Low = \
    ".586" \
    "rdmsr" \
    parm [ecx] nomemory \
    value [eax] \
    modify exact [eax edx] nomemory;

#undef      ASMRdMsr_High
#pragma aux ASMRdMsr_High = \
    ".586" \
    "rdmsr" \
    parm [ecx] nomemory \
    value [edx] \
    modify exact [eax edx] nomemory;


#undef      ASMGetDR0
#pragma aux ASMGetDR0 = \
    "mov eax, dr0" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMGetDR1
#pragma aux ASMGetDR1 = \
    "mov eax, dr1" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMGetDR2
#pragma aux ASMGetDR2 = \
    "mov eax, dr2" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMGetDR3
#pragma aux ASMGetDR3 = \
    "mov eax, dr3" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMGetDR6
#pragma aux ASMGetDR6 = \
    "mov eax, dr6" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMGetAndClearDR6
#pragma aux ASMGetAndClearDR6 = \
    "mov edx, 0ffff0ff0h" \
    "mov eax, dr6" \
    "mov dr6, edx" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax edx] nomemory;

#undef      ASMGetDR7
#pragma aux ASMGetDR7 = \
    "mov eax, dr7" \
    parm [] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMSetDR0
#pragma aux ASMSetDR0 = \
    "mov dr0, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMSetDR1
#pragma aux ASMSetDR1 = \
    "mov dr1, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMSetDR2
#pragma aux ASMSetDR2 = \
    "mov dr2, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMSetDR3
#pragma aux ASMSetDR3 = \
    "mov dr3, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMSetDR6
#pragma aux ASMSetDR6 = \
    "mov dr6, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMSetDR7
#pragma aux ASMSetDR7 = \
    "mov dr7, eax" \
    parm [eax] nomemory \
    modify exact [] nomemory;

/* Yeah, could've used outp here, but this keeps the main file simpler. */
#undef      ASMOutU8
#pragma aux ASMOutU8 = \
    "out dx, al" \
    parm [dx] [al] nomemory \
    modify exact [] nomemory;

#undef      ASMInU8
#pragma aux ASMInU8 = \
    "in al, dx" \
    parm [dx] nomemory \
    value [al] \
    modify exact [] nomemory;

#undef      ASMOutU16
#pragma aux ASMOutU16 = \
    "out dx, ax" \
    parm [dx] [ax] nomemory \
    modify exact [] nomemory;

#undef      ASMInU16
#pragma aux ASMInU16 = \
    "in ax, dx" \
    parm [dx] nomemory \
    value [ax] \
    modify exact [] nomemory;

#undef      ASMOutU32
#pragma aux ASMOutU32 = \
    "out dx, eax" \
    parm [dx] [eax] nomemory \
    modify exact [] nomemory;

#undef      ASMInU32
#pragma aux ASMInU32 = \
    "in eax, dx" \
    parm [dx] nomemory \
    value [eax] \
    modify exact [] nomemory;

#undef      ASMOutStrU8
#pragma aux ASMOutStrU8 = \
    "rep outsb" \
    parm [dx] [esi] [ecx] nomemory \
    modify exact [esi ecx] nomemory;

#undef      ASMInStrU8
#pragma aux ASMInStrU8 = \
    "rep insb" \
    parm [dx] [edi] [ecx] \
    modify exact [edi ecx];

#undef      ASMOutStrU16
#pragma aux ASMOutStrU16 = \
    "rep outsw" \
    parm [dx] [esi] [ecx] nomemory \
    modify exact [esi ecx] nomemory;

#undef      ASMInStrU16
#pragma aux ASMInStrU16 = \
    "rep insw" \
    parm [dx] [edi] [ecx] \
    modify exact [edi ecx];

#undef      ASMOutStrU32
#pragma aux ASMOutStrU32 = \
    "rep outsd" \
    parm [dx] [esi] [ecx] nomemory \
    modify exact [esi ecx] nomemory;

#undef      ASMInStrU32
#pragma aux ASMInStrU32 = \
    "rep insd" \
    parm [dx] [edi] [ecx] \
    modify exact [edi ecx];

#undef      ASMInvalidatePage
#pragma aux ASMInvalidatePage = \
    "invlpg [eax]" \
    parm [eax] \
    modify exact [];

#undef      ASMWriteBackAndInvalidateCaches
#pragma aux ASMWriteBackAndInvalidateCaches = \
    ".486" \
    "wbinvd" \
    parm [] nomemory \
    modify exact [] nomemory;

#undef      ASMInvalidateInternalCaches
#pragma aux ASMInvalidateInternalCaches = \
    ".486" \
    "invd" \
    parm [] \
    modify exact [];

#endif


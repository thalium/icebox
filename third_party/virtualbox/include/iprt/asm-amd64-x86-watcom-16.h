/** @file
 * IPRT - AMD64 and x86 Specific Assembly Functions, 16-bit Watcom C pragma aux.
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
#ifndef ___iprt_asm_amd64_x86_watcom_16_h
#define ___iprt_asm_amd64_x86_watcom_16_h

/*
 * Turns out we cannot use 'ds' for segment stuff here because the compiler
 * seems to insists on loading the DGROUP segment into 'ds' before calling
 * stuff when using -ecc.  Using 'es' instead as this seems to work fine.
 *
 * Note! The #undef that preceds the #pragma aux statements is for undoing
 *       the mangling, because the symbol in #pragma aux [symbol] statements
 *       doesn't get subjected to preprocessing.  This is also why we include
 *       the watcom header at the top rather than at the bottom of the
 *       asm-amd64-x86.h file.
 */

#undef      ASMGetIDTR
#pragma aux ASMGetIDTR = \
    ".286p" \
    "sidt fword ptr es:[bx]" \
    parm [es bx] \
    modify exact [];

#undef      ASMGetIdtrLimit
#pragma aux ASMGetIdtrLimit = \
    ".286p" \
    "sub  sp, 8" \
    "mov  bx, sp" \
    "sidt fword ptr ss:[bx]" \
    "mov  bx, ss:[bx]" \
    "add  sp, 8" \
    parm [] \
    value [bx] \
    modify exact [bx];

#undef      ASMSetIDTR
#pragma aux ASMSetIDTR = \
    ".286p" \
    "lidt fword ptr es:[bx]" \
    parm [es bx] nomemory \
    modify nomemory;

#undef      ASMGetGDTR
#pragma aux ASMGetGDTR = \
    ".286p" \
    "sgdt fword ptr es:[bx]" \
    parm [es bx] \
    modify exact [];

#undef      ASMSetGDTR
#pragma aux ASMSetGDTR = \
    ".286p" \
    "lgdt fword ptr es:[bx]" \
    parm [es bx] nomemory \
    modify exact [] nomemory;

#undef      ASMGetCS
#pragma aux ASMGetCS = \
    "mov ax, cs" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMGetDS
#pragma aux ASMGetDS = \
    "mov ax, ds" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMGetES
#pragma aux ASMGetES = \
    "mov ax, es" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMGetFS
#pragma aux ASMGetFS = \
    ".386" \
    "mov ax, fs" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMGetGS
#pragma aux ASMGetGS = \
    ".386" \
    "mov ax, gs" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMGetSS
#pragma aux ASMGetSS = \
    "mov ax, ss" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMGetTR
#pragma aux ASMGetTR = \
    ".286" \
    "str ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMGetLDTR
#pragma aux ASMGetLDTR = \
    ".286" \
    "sldt ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

/** @todo ASMGetSegAttr   */

#undef      ASMGetFlags
#pragma aux ASMGetFlags = \
    "pushf" \
    "pop ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMSetFlags
#pragma aux ASMSetFlags = \
    "push ax" \
    "popf" \
    parm [ax] nomemory \
    modify exact [] nomemory;

#undef      ASMChangeFlags
#pragma aux ASMChangeFlags = \
    "pushf" \
    "pop ax" \
    "and dx, ax" \
    "or  dx, cx" \
    "push dx" \
    "popf" \
    parm [dx] [cx] nomemory \
    value [ax] \
    modify exact [dx] nomemory;

#undef      ASMAddFlags
#pragma aux ASMAddFlags = \
    "pushf" \
    "pop ax" \
    "or  dx, ax" \
    "push dx" \
    "popf" \
    parm [dx] nomemory \
    value [ax] \
    modify exact [dx] nomemory;

#undef      ASMClearFlags
#pragma aux ASMClearFlags = \
    "pushf" \
    "pop ax" \
    "and dx, ax" \
    "push dx" \
    "popf" \
    parm [dx] nomemory \
    value [ax] \
    modify exact [dx] nomemory;

/* Note! Must use the 64-bit integer return value convension.
         The order of registers in the value [set] does not seem to mean anything. */
#undef      ASMReadTSC
#pragma aux ASMReadTSC = \
    ".586" \
    "rdtsc" \
    "mov ebx, edx" \
    "mov ecx, eax" \
    "shr ecx, 16" \
    "xchg eax, edx" \
    "shr eax, 16" \
    parm [] nomemory \
    value [dx cx bx ax] \
    modify exact [ax bx cx dx] nomemory;

/** @todo ASMReadTscWithAux if needed (rdtscp not recognized by compiler)   */


/* ASMCpuId: Implemented externally, too many parameters. */
/* ASMCpuId_Idx_ECX: Implemented externally, too many parameters. */
/* ASMCpuIdExSlow: Always implemented externally. */
/* ASMCpuId_ECX_EDX: Implemented externally, too many parameters. */

#undef      ASMCpuId_EAX
#pragma aux ASMCpuId_EAX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [ax dx] \
    value [ax dx] \
    modify exact [ax bx cx dx];

#undef      ASMCpuId_EBX
#pragma aux ASMCpuId_EBX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov ax, bx" \
    "shr ebx, 16" \
    parm [ax dx] \
    value [ax bx] \
    modify exact [ax bx cx dx];

#undef      ASMCpuId_ECX
#pragma aux ASMCpuId_ECX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov ax, cx" \
    "shr ecx, 16" \
    parm [ax dx] \
    value [ax cx] \
    modify exact [ax bx cx dx];

#undef      ASMCpuId_EDX
#pragma aux ASMCpuId_EDX = \
    ".586" \
    "xchg ax, dx" \
    "shl eax, 16" \
    "mov ax, dx" \
    "cpuid" \
    "mov ax, dx" \
    "shr edx, 16" \
    parm [ax dx] \
    value [ax dx] \
    modify exact [ax bx cx dx];

/* ASMHasCpuId: MSC inline in main source file. */
/* ASMGetApicId: Implemented externally, lazy bird. */

/* Note! Again, when returning two registers, watcom have certain fixed ordering rules (low:high):
            ax:bx, ax:cx, ax:dx, ax:si, ax:di
            bx:cx, bx:dx, bx:si, bx:di
            dx:cx, si:cx, di:cx
            si:dx, di:dx
            si:di
         This ordering seems to apply to parameter values too. */
#undef      ASMGetCR0
#pragma aux ASMGetCR0 = \
    ".386" \
    "mov eax, cr0" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMSetCR0
#pragma aux ASMSetCR0 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr0, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMGetCR2
#pragma aux ASMGetCR2 = \
    ".386" \
    "mov eax, cr2" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMSetCR2
#pragma aux ASMSetCR2 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr2, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMGetCR3
#pragma aux ASMGetCR3 = \
    ".386" \
    "mov eax, cr3" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMSetCR3
#pragma aux ASMSetCR3 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr3, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMReloadCR3
#pragma aux ASMReloadCR3 = \
    ".386" \
    "mov eax, cr3" \
    "mov cr3, eax" \
    parm [] nomemory \
    modify exact [ax] nomemory;

#undef      ASMGetCR4
#pragma aux ASMGetCR4 = \
    ".386" \
    "mov eax, cr4" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMSetCR4
#pragma aux ASMSetCR4 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov cr4, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

/* ASMGetCR8: Don't bother for 16-bit. */
/* ASMSetCR8: Don't bother for 16-bit. */

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
    "pushf" \
    "cli" \
    "pop ax" \
    parm [] nomemory \
    value [ax] \
    modify exact [] nomemory;

#undef      ASMHalt
#pragma aux ASMHalt = \
    "hlt" \
    parm [] nomemory \
    modify exact [] nomemory;

#undef      ASMRdMsr
#pragma aux ASMRdMsr = \
    ".586" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "rdmsr" \
    "mov ebx, edx" \
    "mov ecx, eax" \
    "shr ecx, 16" \
    "xchg eax, edx" \
    "shr eax, 16" \
    parm [ax cx] nomemory \
    value [dx cx bx ax] \
    modify exact [ax bx cx dx] nomemory;

/* ASMWrMsr: Implemented externally, lazy bird. */
/* ASMRdMsrEx: Implemented externally, lazy bird. */
/* ASMWrMsrEx: Implemented externally, lazy bird. */

#undef      ASMRdMsr_Low
#pragma aux ASMRdMsr_Low = \
    ".586" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "rdmsr" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [ax cx] nomemory \
    value [ax dx] \
    modify exact [ax bx cx dx] nomemory;

#undef      ASMRdMsr_High
#pragma aux ASMRdMsr_High = \
    ".586" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "rdmsr" \
    "mov eax, edx" \
    "shr edx, 16" \
    parm [ax cx] nomemory \
    value [ax dx] \
    modify exact [ax bx cx dx] nomemory;


#undef      ASMGetDR0
#pragma aux ASMGetDR0 = \
    ".386" \
    "mov eax, dr0" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMGetDR1
#pragma aux ASMGetDR1 = \
    ".386" \
    "mov eax, dr1" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMGetDR2
#pragma aux ASMGetDR2 = \
    ".386" \
    "mov eax, dr2" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMGetDR3
#pragma aux ASMGetDR3 = \
    ".386" \
    "mov eax, dr3" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMGetDR6
#pragma aux ASMGetDR6 = \
    ".386" \
    "mov eax, dr6" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMGetAndClearDR6
#pragma aux ASMGetAndClearDR6 = \
    ".386" \
    "mov edx, 0ffff0ff0h" \
    "mov eax, dr6" \
    "mov dr6, edx" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMGetDR7
#pragma aux ASMGetDR7 = \
    ".386" \
    "mov eax, dr7" \
    "mov edx, eax" \
    "shr edx, 16" \
    parm [] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMSetDR0
#pragma aux ASMSetDR0 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr0, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMSetDR1
#pragma aux ASMSetDR1 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr1, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMSetDR2
#pragma aux ASMSetDR2 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr2, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMSetDR3
#pragma aux ASMSetDR3 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr3, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMSetDR6
#pragma aux ASMSetDR6 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr6, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

#undef      ASMSetDR7
#pragma aux ASMSetDR7 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "mov dr7, edx" \
    parm [ax dx] nomemory \
    modify exact [dx] nomemory;

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
    ".386" \
    "shl ecx, 16" \
    "mov cx, ax" \
    "mov eax, ecx" \
    "out dx, eax" \
    parm [dx] [ax cx] nomemory \
    modify exact [] nomemory;

#undef      ASMInU32
#pragma aux ASMInU32 = \
    ".386" \
    "in eax, dx" \
    "mov ecx, eax" \
    "shr ecx, 16" \
    parm [dx] nomemory \
    value [ax cx] \
    modify exact [] nomemory;

#undef      ASMOutStrU8
#pragma aux ASMOutStrU8 = \
    ".186" \
    "mov ds, bx" \
    "rep outsb" \
    parm [dx] [bx si] [cx] nomemory \
    modify exact [si cx ds] nomemory;

#undef      ASMInStrU8
#pragma aux ASMInStrU8 = \
    ".186" \
    "rep insb" \
    parm [dx] [es di] [cx] \
    modify exact [di cx];

#undef      ASMOutStrU16
#pragma aux ASMOutStrU16 = \
    ".186" \
    "mov ds, bx" \
    "rep outsw" \
    parm [dx] [bx si] [cx] nomemory \
    modify exact [si cx ds] nomemory;

#undef      ASMInStrU16
#pragma aux ASMInStrU16 = \
    ".186" \
    "rep insw" \
    parm [dx] [es di] [cx] \
    modify exact [di cx];

#undef      ASMOutStrU32
#pragma aux ASMOutStrU32 = \
    ".386" \
    "mov ds, bx" \
    "rep outsd" \
    parm [dx] [bx si] [cx] nomemory \
    modify exact [si cx ds] nomemory;

#undef      ASMInStrU32
#pragma aux ASMInStrU32 = \
    ".386" \
    "rep insd" \
    parm [dx] [es di] [cx] \
    modify exact [di cx];

#undef ASMInvalidatePage
#pragma aux ASMInvalidatePage = \
    ".486" \
    "shl edx, 16" \
    "mov dx, ax" \
    "invlpg [edx]" \
    parm [ax dx] \
    modify exact [dx];

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


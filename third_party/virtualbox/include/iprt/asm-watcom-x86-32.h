/** @file
 * IPRT - Assembly Functions, x86 32-bit Watcom C/C++ pragma aux.
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

#ifndef ___iprt_asm_h
# error "Don't include this header directly."
#endif
#ifndef ___iprt_asm_watcom_x86_32_h
#define ___iprt_asm_watcom_x86_32_h

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

#undef       ASMCompilerBarrier
#if 0 /* overkill version. */
# pragma aux ASMCompilerBarrier = \
    "nop" \
    parm [] \
    modify exact [eax ebx ecx edx es ds fs gs];
#else
# pragma aux ASMCompilerBarrier = \
    "" \
    parm [] \
    modify exact [];
#endif

#undef      ASMNopPause
#pragma aux ASMNopPause = \
    ".686p" \
    ".xmm2" \
    "pause" \
    parm [] nomemory \
    modify exact [] nomemory;

#undef      ASMAtomicXchgU8
#pragma aux ASMAtomicXchgU8 = \
    "xchg [ecx], al" \
    parm [ecx] [al] \
    value [al] \
    modify exact [al];

#undef      ASMAtomicXchgU16
#pragma aux ASMAtomicXchgU16 = \
    "xchg [ecx], ax" \
    parm [ecx] [ax] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicXchgU32
#pragma aux ASMAtomicXchgU32 = \
    "xchg [ecx], eax" \
    parm [ecx] [eax] \
    value [eax] \
    modify exact [eax];

#undef      ASMAtomicXchgU64
#pragma aux ASMAtomicXchgU64 = \
    ".586" \
    "try_again:" \
    "lock cmpxchg8b [esi]" \
    "jnz try_again" \
    parm [esi] [ebx ecx] \
    value [eax edx] \
    modify exact [edx ecx ebx eax];

#undef      ASMAtomicCmpXchgU8
#pragma aux ASMAtomicCmpXchgU8 = \
    ".486" \
    "lock cmpxchg [edx], cl" \
    "setz al" \
    parm [edx] [cl] [al] \
    value [al] \
    modify exact [al];

#undef      ASMAtomicCmpXchgU16
#pragma aux ASMAtomicCmpXchgU16 = \
    ".486" \
    "lock cmpxchg [edx], cx" \
    "setz al" \
    parm [edx] [cx] [ax] \
    value [al] \
    modify exact [ax];

#undef      ASMAtomicCmpXchgU32
#pragma aux ASMAtomicCmpXchgU32 = \
    ".486" \
    "lock cmpxchg [edx], ecx" \
    "setz al" \
    parm [edx] [ecx] [eax] \
    value [al] \
    modify exact [eax];

#undef      ASMAtomicCmpXchgU64
#pragma aux ASMAtomicCmpXchgU64 = \
    ".586" \
    "lock cmpxchg8b [edi]" \
    "setz al" \
    parm [edi] [ebx ecx] [eax edx] \
    value [al] \
    modify exact [eax edx];

#undef      ASMAtomicCmpXchgExU32
#pragma aux ASMAtomicCmpXchgExU32 = \
    ".586" \
    "lock cmpxchg [edx], ecx" \
    "mov [edi], eax" \
    "setz al" \
    parm [edx] [ecx] [eax] [edi] \
    value [al] \
    modify exact [eax];

#undef      ASMAtomicCmpXchgExU64
#pragma aux ASMAtomicCmpXchgExU64 = \
    ".586" \
    "lock cmpxchg8b [edi]" \
    "mov [esi], eax" \
    "mov [esi + 4], edx" \
    "setz al" \
    parm [edi] [ebx ecx] [eax edx] [esi] \
    value [al] \
    modify exact [eax edx];

#undef      ASMSerializeInstructionCpuId
#pragma aux ASMSerializeInstructionCpuId = \
    ".586" \
    "xor eax, eax" \
    "cpuid" \
    parm [] \
    modify exact [eax ebx ecx edx];

#undef ASMSerializeInstructionIRet
#pragma aux ASMSerializeInstructionIRet = \
    "pushf" \
    "push cs" \
    "call foo" /* 'push offset done' doesn't work */ \
    "jmp  done" \
    "foo:" \
    "iret" \
    "done:" \
    parm [] \
    modify exact [];

#undef      ASMSerializeInstructionRdTscp
#pragma aux ASMSerializeInstructionRdTscp = \
    0x0f 0x01 0xf9 \
    parm [] \
    modify exact [eax edx ecx];

#undef      ASMAtomicReadU64
#pragma aux ASMAtomicReadU64 = \
    ".586" \
    "xor eax, eax" \
    "mov edx, eax" \
    "mov ebx, eax" \
    "mov ecx, eax" \
    "lock cmpxchg8b [edi]" \
    parm [edi] \
    value [eax edx] \
    modify exact [eax ebx ecx edx];

#undef      ASMAtomicUoReadU64
#pragma aux ASMAtomicUoReadU64 = \
    ".586" \
    "xor eax, eax" \
    "mov edx, eax" \
    "mov ebx, eax" \
    "mov ecx, eax" \
    "lock cmpxchg8b [edi]" \
    parm [edi] \
    value [eax edx] \
    modify exact [eax ebx ecx edx];

#undef      ASMAtomicAddU16
#pragma aux ASMAtomicAddU16 = \
    ".486" \
    "lock xadd [ecx], ax" \
    parm [ecx] [ax] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicAddU32
#pragma aux ASMAtomicAddU32 = \
    ".486" \
    "lock xadd [ecx], eax" \
    parm [ecx] [eax] \
    value [eax] \
    modify exact [eax];

#undef      ASMAtomicIncU16
#pragma aux ASMAtomicIncU16 = \
    ".486" \
    "mov ax, 1" \
    "lock xadd [ecx], ax" \
    "inc ax" \
    parm [ecx] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicIncU32
#pragma aux ASMAtomicIncU32 = \
    ".486" \
    "mov eax, 1" \
    "lock xadd [ecx], eax" \
    "inc eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];

/* ASMAtomicIncU64: Should be done by C inline or in external file. */

#undef      ASMAtomicDecU16
#pragma aux ASMAtomicDecU16 = \
    ".486" \
    "mov ax, 0ffffh" \
    "lock xadd [ecx], ax" \
    "dec ax" \
    parm [ecx] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicDecU32
#pragma aux ASMAtomicDecU32 = \
    ".486" \
    "mov eax, 0ffffffffh" \
    "lock xadd [ecx], eax" \
    "dec eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];

/* ASMAtomicDecU64: Should be done by C inline or in external file. */

#undef      ASMAtomicOrU32
#pragma aux ASMAtomicOrU32 = \
    "lock or [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

/* ASMAtomicOrU64: Should be done by C inline or in external file. */

#undef      ASMAtomicAndU32
#pragma aux ASMAtomicAndU32 = \
    "lock and [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

/* ASMAtomicAndU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoOrU32
#pragma aux ASMAtomicUoOrU32 = \
    "or [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

/* ASMAtomicUoOrU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoAndU32
#pragma aux ASMAtomicUoAndU32 = \
    "and [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

/* ASMAtomicUoAndU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoIncU32
#pragma aux ASMAtomicUoIncU32 = \
    ".486" \
    "xadd [ecx], eax" \
    "inc eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];

#undef      ASMAtomicUoDecU32
#pragma aux ASMAtomicUoDecU32 = \
    ".486" \
    "mov eax, 0ffffffffh" \
    "xadd [ecx], eax" \
    "dec eax" \
    parm [ecx] \
    value [eax] \
    modify exact [eax];

#undef      ASMMemZeroPage
#pragma aux ASMMemZeroPage = \
    "mov ecx, 1024" \
    "xor eax, eax" \
    "rep stosd"  \
    parm [edi] \
    modify exact [eax ecx edi];

#undef      ASMMemZero32
#pragma aux ASMMemZero32 = \
    "shr ecx, 2" \
    "xor eax, eax" \
    "rep stosd"  \
    parm [edi] [ecx] \
    modify exact [eax ecx edi];

#undef      ASMMemFill32
#pragma aux ASMMemFill32 = \
    "shr ecx, 2" \
    "rep stosd"  \
    parm [edi] [ecx] [eax]\
    modify exact [ecx edi];

#undef      ASMProbeReadByte
#pragma aux ASMProbeReadByte = \
    "mov al, [ecx]" \
    parm [ecx] \
    value [al] \
    modify exact [al];

#undef      ASMBitSet
#pragma aux ASMBitSet = \
    "bts [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

#undef      ASMAtomicBitSet
#pragma aux ASMAtomicBitSet = \
    "lock bts [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

#undef      ASMBitClear
#pragma aux ASMBitClear = \
    "btr [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

#undef      ASMAtomicBitClear
#pragma aux ASMAtomicBitClear = \
    "lock btr [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

#undef      ASMBitToggle
#pragma aux ASMBitToggle = \
    "btc [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];

#undef      ASMAtomicBitToggle
#pragma aux ASMAtomicBitToggle = \
    "lock btc [ecx], eax" \
    parm [ecx] [eax] \
    modify exact [];


#undef      ASMBitTestAndSet
#pragma aux ASMBitTestAndSet = \
    "bts [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];

#undef      ASMAtomicBitTestAndSet
#pragma aux ASMAtomicBitTestAndSet = \
    "lock bts [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];

#undef      ASMBitTestAndClear
#pragma aux ASMBitTestAndClear = \
    "btr [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];

#undef      ASMAtomicBitTestAndClear
#pragma aux ASMAtomicBitTestAndClear = \
    "lock btr [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];

#undef      ASMBitTestAndToggle
#pragma aux ASMBitTestAndToggle = \
    "btc [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];

#undef      ASMAtomicBitTestAndToggle
#pragma aux ASMAtomicBitTestAndToggle = \
    "lock btc [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] \
    value [al] \
    modify exact [eax];

#undef      ASMBitTest
#pragma aux ASMBitTest = \
    "bt  [ecx], eax" \
    "setc al" \
    parm [ecx] [eax] nomemory \
    value [al] \
    modify exact [eax] nomemory;

#if 0
/** @todo this is way to much inline assembly, better off in an external function. */
#undef      ASMBitFirstClear
#pragma aux ASMBitFirstClear = \
    "mov edx, edi" /* save start of bitmap for later */ \
    "add ecx, 31" \
    "shr ecx, 5" /* cDWord = RT_ALIGN_32(cBits, 32) / 32; */  \
    "mov eax, 0ffffffffh" \
    "repe scasd" \
    "je done" \
    "lea edi, [edi - 4]" /* rewind edi */ \
    "xor eax, [edi]" /* load inverted bits */ \
    "sub edi, edx" /* calc byte offset */ \
    "shl edi, 3" /* convert byte to bit offset */ \
    "mov edx, eax" \
    "bsf eax, edx" \
    "add eax, edi" \
    "done:" \
    parm [edi] [ecx] \
    value [eax] \
    modify exact [eax ecx edx edi];

/* ASMBitNextClear: Too much work, do when needed. */

/** @todo this is way to much inline assembly, better off in an external function. */
#undef      ASMBitFirstSet
#pragma aux ASMBitFirstSet = \
    "mov edx, edi" /* save start of bitmap for later */ \
    "add ecx, 31" \
    "shr ecx, 5" /* cDWord = RT_ALIGN_32(cBits, 32) / 32; */  \
    "mov eax, 0ffffffffh" \
    "repe scasd" \
    "je done" \
    "lea edi, [edi - 4]" /* rewind edi */ \
    "mov eax, [edi]" /* reload previous dword */ \
    "sub edi, edx" /* calc byte offset */ \
    "shl edi, 3" /* convert byte to bit offset */ \
    "mov edx, eax" \
    "bsf eax, edx" \
    "add eax, edi" \
    "done:" \
    parm [edi] [ecx] \
    value [eax] \
    modify exact [eax ecx edx edi];

/* ASMBitNextSet: Too much work, do when needed. */
#else
/* ASMBitFirstClear: External file.  */
/* ASMBitNextClear:  External file.  */
/* ASMBitFirstSet:   External file.  */
/* ASMBitNextSet:    External file.  */
#endif

#undef      ASMBitFirstSetU32
#pragma aux ASMBitFirstSetU32 = \
    "bsf eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [eax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMBitFirstSetU64
#pragma aux ASMBitFirstSetU64 = \
    "bsf eax, eax" \
    "jz  not_found_low" \
    "inc eax" \
    "jmp done" \
    \
    "not_found_low:" \
    "bsf eax, edx" \
    "jz  not_found_high" \
    "add eax, 33" \
    "jmp done" \
    \
    "not_found_high:" \
    "xor eax, eax" \
    "done:" \
    parm [eax edx] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMBitFirstSetU16
#pragma aux ASMBitFirstSetU16 = \
    "movzx eax, ax" \
    "bsf eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [ax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMBitLastSetU32
#pragma aux ASMBitLastSetU32 = \
    "bsr eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [eax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMBitLastSetU64
#pragma aux ASMBitLastSetU64 = \
    "bsf eax, eax" \
    "jz  not_found_low" \
    "inc eax" \
    "jmp done" \
    \
    "not_found_low:" \
    "bsf eax, edx" \
    "jz  not_found_high" \
    "add eax, 33" \
    "jmp done" \
    \
    "not_found_high:" \
    "xor eax, eax" \
    "done:" \
    parm [eax edx] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMBitLastSetU16
#pragma aux ASMBitLastSetU16 = \
    "movzx eax, ax" \
    "bsr eax, eax" \
    "jz  not_found" \
    "inc eax" \
    "jmp done" \
    "not_found:" \
    "xor eax, eax" \
    "done:" \
    parm [ax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMByteSwapU16
#pragma aux ASMByteSwapU16 = \
    "ror ax, 8" \
    parm [ax] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMByteSwapU32
#pragma aux ASMByteSwapU32 = \
    "bswap eax" \
    parm [eax] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMRotateLeftU32
#pragma aux ASMRotateLeftU32 = \
    "rol    eax, cl" \
    parm [eax] [ecx] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#undef      ASMRotateRightU32
#pragma aux ASMRotateRightU32 = \
    "ror    eax, cl" \
    parm [eax] [ecx] nomemory \
    value [eax] \
    modify exact [eax] nomemory;

#endif


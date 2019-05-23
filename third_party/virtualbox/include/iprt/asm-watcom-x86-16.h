/** @file
 * IPRT - Assembly Functions, x86 16-bit Watcom C/C++ pragma aux.
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
#ifndef ___iprt_asm_watcom_x86_16_h
#define ___iprt_asm_watcom_x86_16_h


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

#undef       ASMCompilerBarrier
#if 0 /* overkill version. */
# pragma aux ASMCompilerBarrier = \
    "nop" \
    parm [] \
    modify exact [ax bx cx dx es ds];
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
    "xchg es:[bx], al" \
    parm [es bx] [al] \
    value [al] \
    modify exact [al];

#undef      ASMAtomicXchgU16
#pragma aux ASMAtomicXchgU16 = \
    "xchg es:[bx], ax" \
    parm [es bx] [ax] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicXchgU32
#pragma aux ASMAtomicXchgU32 = \
    ".386" \
    "shl  ecx, 16" \
    "mov  cx, ax" \
    "xchg es:[bx], ecx" \
    "mov  eax, ecx" \
    "shr  ecx, 16" \
    parm [es bx] [ax cx] \
    value [ax cx] \
    modify exact [ax cx];

#undef      ASMAtomicXchgU64
#pragma aux ASMAtomicXchgU64 = \
    ".586" \
    "shl eax, 16" \
    "mov ax, bx" /* eax = high dword */ \
    "shl ecx, 16" \
    "mov cx, dx" /* ecx = low dword */ \
    "mov ebx, ecx" /* ebx = low */ \
    "mov ecx, eax" /* ecx = high */ \
    "try_again:" \
    "lock cmpxchg8b es:[si]" \
    "jnz try_again" \
    "xchg eax, edx" \
    "mov  ebx, eax" \
    "shr  eax, 16" \
    "mov  ecx, edx" \
    "shr  ecx, 16" \
    parm [es si] [dx cx bx ax] \
    value [dx cx bx ax] \
    modify exact [dx cx bx ax];

#undef      ASMAtomicCmpXchgU8
#pragma aux ASMAtomicCmpXchgU8 = \
    ".486" \
    "lock cmpxchg es:[bx], cl" \
    "setz al" \
    parm [es bx] [cl] [al] \
    value [al] \
    modify exact [al];

#undef      ASMAtomicCmpXchgU16
#pragma aux ASMAtomicCmpXchgU16 = \
    ".486" \
    "lock cmpxchg es:[bx], cx" \
    "setz al" \
    parm [es bx] [cx] [ax] \
    value [al] \
    modify exact [ax];

#undef      ASMAtomicCmpXchgU32
#pragma aux ASMAtomicCmpXchgU32 = \
    ".486" \
    "shl ecx, 16" \
    "mov cx, dx" \
    "shl eax, 16" \
    "mov ax, di" \
    "rol eax, 16" \
    "lock cmpxchg es:[bx], ecx" \
    "setz al" \
    parm [es bx] [cx dx] [ax di] \
    value [al] \
    modify exact [ax cx];

/* ASMAtomicCmpXchgU64: External assembly implementation, too few registers for parameters.  */
/* ASMAtomicCmpXchgExU32: External assembly implementation, too few registers for parameters.  */
/* ASMAtomicCmpXchgExU64: External assembly implementation, too few registers for parameters.  */

#undef      ASMSerializeInstructionCpuId
#pragma aux ASMSerializeInstructionCpuId = \
    ".586" \
    "xor eax, eax" \
    "cpuid" \
    parm [] \
    modify exact [ax bx cx dx];

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
    modify exact [ax dx cx];

#undef      ASMAtomicReadU64
#pragma aux ASMAtomicReadU64 = \
    ".586" \
    "xor eax, eax" \
    "xor edx, edx" \
    "xor ebx, ebx" \
    "xor ecx, ecx" \
    "lock cmpxchg8b es:[si]" \
    "xchg eax, edx" \
    "mov  ebx, eax" \
    "shr  eax, 16" \
    "mov  ecx, edx" \
    "shr  ecx, 16" \
    parm [es si] \
    value [dx cx bx ax] \
    modify exact [dx cx bx ax];

#undef      ASMAtomicUoReadU64
#pragma aux ASMAtomicUoReadU64 = \
    ".586" \
    "xor eax, eax" \
    "xor edx, edx" \
    "xor ebx, ebx" \
    "xor ecx, ecx" \
    "lock cmpxchg8b es:[si]" \
    "xchg eax, edx" \
    "mov  ebx, eax" \
    "shr  eax, 16" \
    "mov  ecx, edx" \
    "shr  ecx, 16" \
    parm [es si] \
    value [dx cx bx ax] \
    modify exact [dx cx bx ax];

#undef      ASMAtomicAddU16
#pragma aux ASMAtomicAddU16 = \
    ".486" \
    "lock xadd es:[bx], ax" \
    parm [es bx] [ax] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicAddU32
#pragma aux ASMAtomicAddU32 = \
    ".486" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock xadd es:[bx], edx" \
    "mov ax, dx" \
    "shr edx, 16" \
    parm [es bx] [ax dx] \
    value [ax dx] \
    modify exact [ax dx];

#undef      ASMAtomicIncU16
#pragma aux ASMAtomicIncU16 = \
    ".486" \
    "mov ax, 1" \
    "lock xadd es:[bx], ax" \
    "inc ax" \
    parm [es bx] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicIncU32
#pragma aux ASMAtomicIncU32 = \
    ".486" \
    "mov edx, 1" \
    "lock xadd es:[bx], edx" \
    "inc edx" \
    "mov ax, dx" \
    "shr edx, 16" \
    parm [es bx] \
    value [ax dx] \
    modify exact [ax dx];

/* ASMAtomicIncU64: Should be done by C inline or in external file. */

#undef      ASMAtomicDecU16
#pragma aux ASMAtomicDecU16 = \
    ".486" \
    "mov ax, 0ffffh" \
    "lock xadd es:[bx], ax" \
    "dec ax" \
    parm [es bx] \
    value [ax] \
    modify exact [ax];

#undef      ASMAtomicDecU32
#pragma aux ASMAtomicDecU32 = \
    ".486" \
    "mov edx, 0ffffffffh" \
    "lock xadd es:[bx], edx" \
    "dec edx" \
    "mov ax, dx" \
    "shr edx, 16" \
    parm [es bx] \
    value [ax dx] \
    modify exact [ax dx];

/* ASMAtomicDecU64: Should be done by C inline or in external file. */

#undef      ASMAtomicOrU32
#pragma aux ASMAtomicOrU32 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock or es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];

/* ASMAtomicOrU64: Should be done by C inline or in external file. */

#undef      ASMAtomicAndU32
#pragma aux ASMAtomicAndU32 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock and es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];

/* ASMAtomicAndU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoOrU32
#pragma aux ASMAtomicUoOrU32 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "or es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];

/* ASMAtomicUoOrU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoAndU32
#pragma aux ASMAtomicUoAndU32 = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "and es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];

/* ASMAtomicUoAndU64: Should be done by C inline or in external file. */

#undef      ASMAtomicUoIncU32
#pragma aux ASMAtomicUoIncU32 = \
    ".486" \
    "mov edx, 1" \
    "xadd es:[bx], edx" \
    "inc edx" \
    "mov ax, dx" \
    "shr edx, 16" \
    parm [es bx] \
    value [ax dx] \
    modify exact [ax dx];

#undef      ASMAtomicUoDecU32
#pragma aux ASMAtomicUoDecU32 = \
    ".486" \
    "mov edx, 0ffffffffh" \
    "xadd es:[bx], edx" \
    "dec edx" \
    "mov ax, dx" \
    "shr edx, 16" \
    parm [es bx] \
    value [ax dx] \
    modify exact [ax dx];

#undef      ASMMemZeroPage
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMMemZeroPage = \
    "mov cx, 2048" \
    "xor ax, ax" \
    "rep stosw"  \
    parm [es di] \
    modify exact [ax cx di];
#else
# pragma aux ASMMemZeroPage = \
    "mov ecx, 1024" \
    "xor eax, eax" \
    "rep stosd"  \
    parm [es di] \
    modify exact [ax cx di];
#endif

#undef      ASMMemZero32
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMMemZero32 = \
    "xor ax, ax" \
    "shr cx, 1" \
    "shr cx, 1" \
    "rep stosw" \
    parm [es di] [cx] \
    modify exact [ax dx cx di];
#else
# pragma aux ASMMemZero32 = \
    "and ecx, 0ffffh" /* probably not necessary, lazy bird should check... */ \
    "shr ecx, 2" \
    "xor eax, eax" \
    "rep stosd"  \
    parm [es di] [cx] \
    modify exact [ax cx di];
#endif

#undef      ASMMemFill32
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMMemFill32 = \
    "   shr     cx, 1" \
    "   shr     cx, 1" \
    "   jz      done" \
    "again:" \
    "   stosw" \
    "   xchg    ax, dx" \
    "   stosw" \
    "   xchg    ax, dx" \
    "   dec     cx" \
    "   jnz     again" \
    "done:" \
    parm [es di] [cx] [ax dx]\
    modify exact [cx di];
#else
# pragma aux ASMMemFill32 = \
    "and ecx, 0ffffh" /* probably not necessary, lazy bird should check... */ \
    "shr ecx, 2" \
    "shl eax, 16" \
    "mov ax, dx" \
    "rol eax, 16" \
    "rep stosd"  \
    parm [es di] [cx] [ax dx]\
    modify exact [ax cx di];
#endif

#undef      ASMProbeReadByte
#pragma aux ASMProbeReadByte = \
    "mov al, es:[bx]" \
    parm [es bx] \
    value [al] \
    modify exact [al];

#undef      ASMBitSet
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMBitSet = \
    "   mov     ch, cl" /* Only the three lowest bits are relevant due to 64KB segments */ \
    "   mov     cl, 5" \
    "   shl     ch, cl" \
    "   add     bh, ch" /* Adjust the pointer. */ \
    "   mov     cl, al" \
    "   shr     ax, 1"  /* convert to byte offset */ \
    "   shr     ax, 1" \
    "   shr     ax, 1" \
    "   add     bx, ax" /* adjust pointer again */\
    "   and     cl, 7" \
    "   mov     al, 1" \
    "   shl     al, cl" /* al=bitmask */ \
    "   or      es:[bx], al" \
    parm [es bx] [ax cx] \
    modify exact [ax bx cx];
#else
# pragma aux ASMBitSet = \
    "shl edx, 16" \
    "mov dx, ax" \
    "bts es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];
#endif

#undef      ASMAtomicBitSet
#pragma aux ASMAtomicBitSet = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock bts es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];

#undef      ASMBitClear
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMBitClear = \
    "   mov     ch, cl" /* Only the three lowest bits are relevant due to 64KB segments */ \
    "   mov     cl, 5" \
    "   shl     ch, cl" \
    "   add     bh, ch" /* Adjust the pointer. */ \
    "   mov     cl, al" \
    "   shr     ax, 1"  /* convert to byte offset */ \
    "   shr     ax, 1" \
    "   shr     ax, 1" \
    "   add     bx, ax" /* adjust pointer again */\
    "   and     cl, 7" \
    "   mov     al, 1" \
    "   shl     al, cl" \
    "   not     al" /* al=bitmask */ \
    "   and     es:[bx], al" \
    parm [es bx] [ax cx] \
    modify exact [ax bx cx];
#else
# pragma aux ASMBitClear = \
    "shl edx, 16" \
    "mov dx, ax" \
    "btr es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];
#endif

#undef      ASMAtomicBitClear
#pragma aux ASMAtomicBitClear = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock btr es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];

#undef      ASMBitToggle
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMBitToggle = \
    "   mov     ch, cl" /* Only the three lowest bits are relevant due to 64KB segments */ \
    "   mov     cl, 5" \
    "   shl     ch, cl" \
    "   add     bh, ch" /* Adjust the pointer. */ \
    "   mov     cl, al" \
    "   shr     ax, 1"  /* convert to byte offset */ \
    "   shr     ax, 1" \
    "   shr     ax, 1" \
    "   add     bx, ax" /* adjust pointer again */\
    "   and     cl, 7" \
    "   mov     al, 1" \
    "   shl     al, cl" /* al=bitmask */ \
    "   xor     es:[bx], al" \
    parm [es bx] [ax cx] \
    modify exact [ax bx cx];
#else
# pragma aux ASMBitToggle = \
    "shl edx, 16" \
    "mov dx, ax" \
    "btc es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];
#endif

#undef      ASMAtomicBitToggle
#pragma aux ASMAtomicBitToggle = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock btc es:[bx], edx" \
    parm [es bx] [ax dx] \
    modify exact [dx];

#undef      ASMBitTestAndSet
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMBitTestAndSet = \
    "   mov     ch, cl" /* Only the three lowest bits are relevant due to 64KB segments */ \
    "   mov     cl, 5" \
    "   shl     ch, cl" \
    "   add     bh, ch" /* Adjust the pointer. */ \
    "   mov     cl, al" \
    "   shr     ax, 1"  /* convert to byte offset */ \
    "   shr     ax, 1" \
    "   shr     ax, 1" \
    "   add     bx, ax" /* adjust pointer again */\
    "   and     cl, 7" /* cl=byte shift count */ \
    "   mov     ah, 1" \
    "   shl     ah, cl" /* ah=bitmask */ \
    "   mov     al, es:[bx]" \
    "   or      ah, al" \
    "   mov     es:[bx], ah" \
    "   shr     al, cl" \
    "   and     al, 1" \
    parm [es bx] [ax cx] \
    value [al] \
    modify exact [ax bx cx];
#else
# pragma aux ASMBitTestAndSet = \
    "shl edx, 16" \
    "mov dx, ax" \
    "bts es:[bx], edx" \
    "setc al" \
    parm [es bx] [ax dx] \
    value [al] \
    modify exact [ax dx];
#endif

#undef      ASMAtomicBitTestAndSet
#pragma aux ASMAtomicBitTestAndSet = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock bts es:[bx], edx" \
    "setc al" \
    parm [es bx] [ax dx] \
    value [al] \
    modify exact [ax dx];

#undef      ASMBitTestAndClear
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMBitTestAndClear = \
    "   mov     ch, cl" /* Only the three lowest bits are relevant due to 64KB segments */ \
    "   mov     cl, 5" \
    "   shl     ch, cl" \
    "   add     bh, ch" /* Adjust the pointer. */ \
    "   mov     cl, al" \
    "   shr     ax, 1"  /* convert to byte offset */ \
    "   shr     ax, 1" \
    "   shr     ax, 1" \
    "   add     bx, ax" /* adjust pointer again */\
    "   and     cl, 7" /* cl=byte shift count */ \
    "   mov     ah, 1" \
    "   shl     ah, cl" \
    "   not     ah" /* ah=bitmask */ \
    "   mov     al, es:[bx]" \
    "   and     ah, al" \
    "   mov     es:[bx], ah" \
    "   shr     al, cl" \
    "   and     al, 1" \
    parm [es bx] [ax cx] \
    value [al] \
    modify exact [ax bx cx];
#else
# pragma aux ASMBitTestAndClear = \
    "shl edx, 16" \
    "mov dx, ax" \
    "btr es:[bx], edx" \
    "setc al" \
    parm [es bx] [ax dx] \
    value [al] \
    modify exact [ax dx];
#endif

#undef      ASMAtomicBitTestAndClear
#pragma aux ASMAtomicBitTestAndClear = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock btr es:[bx], edx" \
    "setc al" \
    parm [es bx] [ax dx] \
    value [al] \
    modify exact [ax dx];

#undef      ASMBitTestAndToggle
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMBitTestAndToggle = \
    "   mov     ch, cl" /* Only the three lowest bits are relevant due to 64KB segments */ \
    "   mov     cl, 5" \
    "   shl     ch, cl" \
    "   add     bh, ch" /* Adjust the pointer. */ \
    "   mov     cl, al" \
    "   shr     ax, 1"  /* convert to byte offset */ \
    "   shr     ax, 1" \
    "   shr     ax, 1" \
    "   add     bx, ax" /* adjust pointer again */\
    "   and     cl, 7" /* cl=byte shift count */ \
    "   mov     ah, 1" \
    "   shl     ah, cl" /* ah=bitmask */ \
    "   mov     al, es:[bx]" \
    "   xor     ah, al" \
    "   mov     es:[bx], ah" \
    "   shr     al, cl" \
    "   and     al, 1" \
    parm [es bx] [ax cx] \
    value [al] \
    modify exact [ax bx cx];
#else
# pragma aux ASMBitTestAndToggle = \
    "shl edx, 16" \
    "mov dx, ax" \
    "btc es:[bx], edx" \
    "setc al" \
    parm [es bx] [ax dx] \
    value [al] \
    modify exact [ax dx];
#endif

#undef      ASMAtomicBitTestAndToggle
#pragma aux ASMAtomicBitTestAndToggle = \
    ".386" \
    "shl edx, 16" \
    "mov dx, ax" \
    "lock btc es:[bx], edx" \
    "setc al" \
    parm [es bx] [ax dx] \
    value [al] \
    modify exact [ax dx];

#undef      ASMBitTest
#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
# pragma aux ASMBitTest = \
    "   mov     ch, cl" /* Only the three lowest bits are relevant due to 64KB segments */ \
    "   mov     cl, 5" \
    "   shl     ch, cl" \
    "   add     bh, ch" /* Adjust the pointer. */ \
    "   mov     cl, al" \
    "   shr     ax, 1"  /* convert to byte offset */ \
    "   shr     ax, 1" \
    "   shr     ax, 1" \
    "   add     bx, ax" /* adjust pointer again */\
    "   and     cl, 7" \
    "   mov     al, es:[bx]" \
    "   shr     al, cl" \
    "   and     al, 1" \
    parm [es bx] [ax cx] \
    value [al] \
    modify exact [ax bx cx];
#else
# pragma aux ASMBitTest = \
    "shl edx, 16" \
    "mov dx, ax" \
    "bt  es:[bx], edx" \
    "setc al" \
    parm [es bx] [ax dx] nomemory \
    value [al] \
    modify exact [ax dx] nomemory;
#endif

/* ASMBitFirstClear: External file.  */
/* ASMBitNextClear:  External file.  */
/* ASMBitFirstSet:   External file.  */
/* ASMBitNextSet:    External file.  */

#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
/* ASMBitFirstSetU32: External file. */
#else
# undef      ASMBitFirstSetU32
# pragma aux ASMBitFirstSetU32 = \
    "shl edx, 16" \
    "mov dx, ax" \
    "bsf eax, edx" \
    "jz  not_found" \
    "inc ax" \
    "jmp done" \
    "not_found:" \
    "xor ax, ax" \
    "done:" \
    parm [ax dx] nomemory \
    value [ax] \
    modify exact [ax dx] nomemory;
#endif

#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
/* ASMBitFirstSetU64: External file. */
#else
# undef      ASMBitFirstSetU64
# pragma aux ASMBitFirstSetU64 = \
    ".386" \
    "shl ecx, 16" \
    "mov cx, dx" \
    "bsf ecx, ecx" \
    "jz  not_found_low" \
    "mov ax, cx" \
    "inc ax" \
    "jmp done" \
    \
    "not_found_low:" \
    "shr eax, 16" \
    "mov ax, bx" \
    "bsf eax, eax" \
    "jz  not_found_high" \
    "add ax, 33" \
    "jmp done" \
    \
    "not_found_high:" \
    "xor ax, ax" \
    "done:" \
    parm [dx cx bx ax] nomemory \
    value [ax] \
    modify exact [ax cx] nomemory;
#endif

#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
/* ASMBitFirstSetU16: External file. */
#else
# undef      ASMBitFirstSetU16
# pragma aux ASMBitFirstSetU16 = \
    "bsf ax, ax" \
    "jz  not_found" \
    "inc ax" \
    "jmp done" \
    "not_found:" \
    "xor ax, ax" \
    "done:" \
    parm [ax] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
/* ASMBitLastSetU32: External file. */
#else
# undef      ASMBitLastSetU32
# pragma aux ASMBitLastSetU32 = \
    "shl edx, 16" \
    "mov dx, ax" \
    "bsr eax, edx" \
    "jz  not_found" \
    "inc ax" \
    "jmp done" \
    "not_found:" \
    "xor ax, ax" \
    "done:" \
    parm [ax dx] nomemory \
    value [ax] \
    modify exact [ax dx] nomemory;
#endif

#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
/* ASMBitLastSetU64: External file. */
#else
# undef      ASMBitLastSetU64
# pragma aux ASMBitLastSetU64 = \
    ".386" \
    "shl ecx, 16" \
    "mov cx, dx" \
    "bsf ecx, ecx" \
    "jz  not_found_low" \
    "mov ax, cx" \
    "inc ax" \
    "jmp done" \
    \
    "not_found_low:" \
    "shr eax, 16" \
    "mov ax, bx" \
    "bsf eax, eax" \
    "jz  not_found_high" \
    "add ax, 33" \
    "jmp done" \
    \
    "not_found_high:" \
    "xor ax, ax" \
    "done:" \
    parm [dx cx bx ax] nomemory \
    value [ax] \
    modify exact [ax cx] nomemory;
#endif

#if defined(__SW_0) || defined(__SW_1) || defined(__SW_2)
/* ASMBitLastSetU16: External file. */
#else
# undef      ASMBitLastSetU16
# pragma aux ASMBitLastSetU16 = \
    "bsr ax, ax" \
    "jz  not_found" \
    "inc ax" \
    "jmp done" \
    "not_found:" \
    "xor ax, ax" \
    "done:" \
    parm [ax] nomemory \
    value [ax] \
    modify exact [ax] nomemory;
#endif

#undef      ASMByteSwapU16
#pragma aux ASMByteSwapU16 = \
    "xchg al, ah" \
    parm [ax] nomemory \
    value [ax] \
    modify exact [ax] nomemory;

#undef      ASMByteSwapU32
#pragma aux ASMByteSwapU32 = \
    "xchg dh, al" \
    "xchg dl, ah" \
    parm [ax dx] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMRotateLeftU32
#pragma aux ASMRotateLeftU32 = \
    ".386" \
    "shl    edx, 16" \
    "mov    dx, ax" \
    "rol    edx, cl" \
    "mov    eax, edx" \
    "shr    edx, 16" \
    parm [ax dx] [cx] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#undef      ASMRotateRightU32
#pragma aux ASMRotateRightU32 = \
    ".386" \
    "shl    edx, 16" \
    "mov    dx, ax" \
    "ror    edx, cl" \
    "mov    eax, edx" \
    "shr    edx, 16" \
    parm [ax dx] [cx] nomemory \
    value [ax dx] \
    modify exact [ax dx] nomemory;

#endif

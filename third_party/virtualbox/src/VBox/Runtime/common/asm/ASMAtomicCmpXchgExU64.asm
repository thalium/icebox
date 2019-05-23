; $Id: ASMAtomicCmpXchgExU64.asm $
;; @file
; IPRT - ASMAtomicCmpXchgExU64().
;

;
; Copyright (C) 2006-2017 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL) only, as it comes in the "COPYING.CDDL" file of the
; VirtualBox OSE distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"

BEGINCODE

;;
; Atomically Exchange an unsigned 64-bit value, ordered.
;
; @param    pu64     x86:ebp+8   gcc:rdi  msc:rcx
; @param    u64New   x86:ebp+c   gcc:rsi  msc:rdx
; @param    u64Old   x86:ebp+14  gcc:rdx  msc:r8
; @param    pu64Old  x86:ebp+1c  gcc:rcx  msc:r9
;
; @returns  bool result: true if successfully exchanged, false if not.
;           x86:al
;
BEGINPROC_EXPORTED ASMAtomicCmpXchgExU64
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        mov     rax, r8
        lock cmpxchg [rcx], rdx
        mov     [r9], rax
 %else
        mov     rax, rdx
        lock cmpxchg [rdi], rsi
        mov     [rcx], rax
 %endif
        setz    al
        movzx   eax, al
        ret
%endif
%ifdef RT_ARCH_X86
        push    ebp
        mov     ebp, esp
        push    ebx
        push    edi

        mov     ebx, dword [ebp+0ch]
        mov     ecx, dword [ebp+0ch + 4]
        mov     edi, [ebp+08h]
        mov     eax, dword [ebp+14h]
        mov     edx, dword [ebp+14h + 4]
        lock cmpxchg8b [edi]
        mov     edi, [ebp + 1ch]
        mov     [edi],     eax
        mov     [edi + 4], edx
        setz    al
        movzx   eax, al

        pop     edi
        pop     ebx
        leave
        ret
%endif
ENDPROC ASMAtomicCmpXchgExU64


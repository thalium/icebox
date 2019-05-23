; $Id: ASMAtomicUoAndU64.asm $
;; @file
; IPRT - ASMAtomicUoAndU64().
;

;
; Copyright (C) 2013-2017 Oracle Corporation
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
; Atomically AND an unsigned 64-bit value, unordered.
;
; @param    pu64     x86:ebp+8   gcc:rdi  msc:rcx
; @param    u64Or    x86:ebp+c   gcc:rsi  msc:rdx
;
; @returns  void
;
BEGINPROC_EXPORTED ASMAtomicUoAndU64
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        and     [rcx], rdx
 %else
        and     [rdi], rsi
 %endif
%elifdef RT_ARCH_X86
        push    ebp
        mov     ebp, esp
        push    ebx
        push    edi

        mov     edi, [ebp + 08h]
        mov     ebx, [ebp + 0ch]
        mov     ecx, [ebp + 0ch + 4]
        mov     eax, ebx
        mov     edx, ecx
.try_again:
        cmpxchg8b [edi]
        jz      .done
        mov     ebx, eax
        and     ebx, [ebp + 0ch]
        mov     ecx, edx
        and     ecx, [ebp + 0ch + 4]
        jmp     .try_again

.done:
        pop     edi
        pop     ebx
        leave
%endif
        ret
ENDPROC ASMAtomicUoAndU64


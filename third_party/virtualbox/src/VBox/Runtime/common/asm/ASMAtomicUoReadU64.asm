; $Id: ASMAtomicUoReadU64.asm $
;; @file
; IPRT - ASMAtomicUoReadU64().
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
; Atomically reads 64-bit value.
;
; @param   pu64     x86:ebp+8
;
; @returns The current value. (x86:eax+edx)
;
;
BEGINPROC_EXPORTED ASMAtomicUoReadU64
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        mov     rax, [rcx]
 %else
        mov     rax, [rdi]
 %endif
        ret
%endif
%ifdef RT_ARCH_X86
        push    ebp
        mov     ebp, esp
        push    ebx
        push    edi

        xor     eax, eax
        xor     edx, edx
        mov     edi, [ebp+08h]
        xor     ecx, ecx
        xor     ebx, ebx
        lock cmpxchg8b [edi]

        pop     edi
        pop     ebx
        leave
        ret
%endif
ENDPROC ASMAtomicUoReadU64


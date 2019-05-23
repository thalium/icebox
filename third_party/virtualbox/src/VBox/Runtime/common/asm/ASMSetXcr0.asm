; $Id: ASMSetXcr0.asm $
;; @file
; IPRT - ASMSetXcr0().
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
%define RT_ASM_WITH_SEH64
%include "iprt/asmdefs.mac"

BEGINCODE

;;
; Sets the content of the Xcr0 CPU register.
; @param   uXcr0    The new XCR0 content.
;                   msc=rcx, gcc=rdi, x86=[esp+4]
;
BEGINPROC_EXPORTED ASMSetXcr0
SEH64_END_PROLOGUE
%ifdef ASM_CALL64_MSC
        mov     rdx, rcx
        shr     rdx, 32
        mov     eax, ecx
%elifdef ASM_CALL64_GCC
        mov     rdx, rdi
        shr     rdx, 32
        mov     eax, edi
%elif ARCH_BITS == 32
        mov     eax, [esp + 4]
        mov     edx, [esp + 8]
%elif ARCH_BITS == 16
        push    bp
        mov     bp, sp
        mov     eax, [bp + 4]
        mov     edx, [bp + 8]
%else
 %error "Undefined arch?"
%endif

        xor     ecx, ecx
        xsetbv

%if ARCH_BITS == 16
        leave
%endif
        ret
ENDPROC ASMSetXcr0


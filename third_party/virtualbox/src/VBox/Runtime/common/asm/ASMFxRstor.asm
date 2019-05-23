; $Id: ASMFxRstor.asm $
;; @file
; IPRT - ASMFxRstor().
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
; Loads extended CPU state.
; @param    pFxState    Pointer to the FXRSTOR state area.
;                       msc=rcx, gcc=rdi, x86=[esp+4]
;
BEGINPROC_EXPORTED ASMFxRstor
SEH64_END_PROLOGUE
%ifdef ASM_CALL64_MSC
        o64 fxrstor [rcx]
%elifdef ASM_CALL64_GCC
        o64 fxrstor [rdi]
%elif ARCH_BITS == 32
        mov     ecx, [esp + 4]
        fxrstor [ecx]
%elif ARCH_BITS == 16
        push    bp
        mov     bp, sp
        push    es
        push    bx
        les     bx, [bp + 4]
        fxrstor [es:bx]
        pop     bx
        pop     es
        pop     bp
%else
 %error "Undefined arch?"
%endif
        ret
ENDPROC ASMFxRstor


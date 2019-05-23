; $Id: ASMWrMsrEx.asm $
;; @file
; IPRT - ASMWrMsrEx().
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
; Special version of ASMRdMsr that allow specifying the rdi value.
;
; @param    uMsr    msc=rcx, gcc=rdi, x86=[ebp+8]   The MSR to read.
; @param    uXDI    msc=rdx, gcc=rsi, x86=[ebp+12]  The EDI/RDI value.
; @param    uValue  msc=r8,  gcc=rdx, x86=[ebp+16]  The 64-bit value to write.
;
BEGINPROC_EXPORTED ASMWrMsrEx
%ifdef ASM_CALL64_MSC
proc_frame ASMWrMsrEx_DupWarningHack
        push    rdi
        [pushreg rdi]
[endprolog]
        and     ecx, ecx                ; serious paranoia
        mov     rdi, rdx
        mov     eax, r8d
        mov     rdx, r8
        shr     rdx, 32
        wrmsr
        pop     rdi
        ret
endproc_frame
%elifdef ASM_CALL64_GCC
        mov     ecx, edi
        mov     rdi, rsi
        mov     eax, edx
        shr     edx, 32
        wrmsr
        ret
%elifdef RT_ARCH_X86
        push    ebp
        mov     ebp, esp
        push    edi
        mov     ecx, [ebp + 8]
        mov     edi, [esp + 12]
        mov     eax, [esp + 16]
        mov     edx, [esp + 20]
        wrmsr
        pop     edi
        leave
        ret
%else
 %error "Undefined arch?"
%endif
ENDPROC ASMWrMsrEx


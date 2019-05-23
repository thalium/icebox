; $Id: ASMAtomicXchgU16.asm $
;; @file
; IPRT - ASMAtomicXchgU16().
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
; Atomically Exchange an unsigned 16-bit value, ordered.
;
; @param    pu16     x86:ebp+8   gcc:rdi  msc:rcx
; @param    u16New   x86:ebp+c   gcc:si   msc:dx
;
; @returns Current (i.e. old) *pu16 value (AX).
;
BEGINPROC_EXPORTED ASMAtomicXchgU16
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        mov     ax, dx
        xchg    [rcx], ax
 %else
        mov     ax, si
        xchg    [rdi], ax
 %endif
%elifdef RT_ARCH_X86
        mov     ecx, [esp+04h]
        mov     ax, [esp+08h]
        xchg    [ecx], ax
%else
 %error "Unsupport arch."
%endif
        ret
ENDPROC ASMAtomicXchgU16


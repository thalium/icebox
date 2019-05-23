; $Id: trunc.asm $
;; @file
; IPRT - No-CRT trunc - AMD64 & X86.
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


%include "iprt/asmdefs.mac"

BEGINCODE

;;
; Round to truncated integer value.
; @returns 32-bit: st(0)   64-bit: xmm0
; @param    rd      32-bit: [ebp + 8]   64-bit: xmm0
BEGINPROC RT_NOCRT(trunc)
    push    xBP
    mov     xBP, xSP
    sub     xSP, 10h

%ifdef RT_ARCH_AMD64
    movsd   [xSP], xmm0
    fld     qword [xSP]
%else
    fld     qword [xBP + xCB*2]
%endif

    ; Make it truncate up by modifying the fpu control word.
    fstcw   [xBP - 10h]
    mov     eax, [xBP - 10h]
    or      eax, 00c00h
    mov     [xBP - 08h], eax
    fldcw   [xBP - 08h]

    ; Round ST(0) to integer.
    frndint

    ; Restore the fpu control word.
    fldcw   [xBP - 10h]

%ifdef RT_ARCH_AMD64
    fstp    qword [xSP]
    movsd   xmm0, [xSP]
%endif
    leave
    ret
ENDPROC   RT_NOCRT(trunc)


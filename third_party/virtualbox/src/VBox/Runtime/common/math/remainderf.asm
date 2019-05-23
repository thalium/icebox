; $Id: remainderf.asm $
;; @file
; IPRT - No-CRT remainderf - AMD64 & X86.
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
; See SUS.
; @returns st(0)
; @param    rf1    [ebp + 08h]  xmm0
; @param    rf2    [ebp + 0ch]  xmm1
BEGINPROC RT_NOCRT(remainderf)
    push    xBP
    mov     xBP, xSP
    sub     xSP, 20h

%ifdef RT_ARCH_AMD64
    movss   [rsp], xmm1
    movss   [rsp + 10h], xmm0
    fld     dword [rsp]
    fld     dword [rsp + 10h]
%else
    fld     dword [ebp + 0ch]
    fld     dword [ebp + 8h]
%endif

    fprem1
    fstsw   ax
    test    ah, 04h
    jnz     .done
    fstp    st1

.done:
%ifdef RT_ARCH_AMD64
    fstp    dword [rsp]
    movss   xmm0, [rsp]
%endif

    leave
    ret
ENDPROC   RT_NOCRT(remainderf)


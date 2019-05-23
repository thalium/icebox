; $Id: remainderl.asm $
;; @file
; IPRT - No-CRT remainderl - AMD64 & X86.
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
; @param    lrd1    [rbp + 10h]
; @param    lrd2    [rbp + 20h]
BEGINPROC RT_NOCRT(remainderl)
    push    xBP
    mov     xBP, xSP

%ifdef RT_ARCH_AMD64
    fld     tword [rbp + 10h + RTLRD_CB]
    fld     tword [rbp + 10h]
%else
    fld     tword [ebp + 8h + RTLRD_CB]
    fld     tword [ebp + 8h]
%endif

    fprem1
    fstsw   ax
    test    ah, 04h
    jnz     .done
    fstp    st1

.done:
    leave
    ret
ENDPROC   RT_NOCRT(remainderl)


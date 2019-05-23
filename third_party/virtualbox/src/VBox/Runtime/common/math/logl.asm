; $Id: logl.asm $
;; @file
; IPRT - No-CRT logl - AMD64 & X86.
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
; compute the natural logarithm of lrd
; @returns st(0)
; @param    lrd     [rbp + xCB*2]
BEGINPROC RT_NOCRT(logl)
    push    xBP
    mov     xBP, xSP
    sub     xSP, 10h

    fldln2                              ; st0=log(2)
    fld     tword [xBP + xCB*2]         ; st1=log(2) st0=lrd
    fld     st0                         ; st1=log(2) st0=lrd st0=lrd
    fsub    qword [.one xWrtRIP]        ; st2=log(2) st1=lrd st0=lrd-1.0
    fld     st0                         ; st3=log(2) st2=lrd st1=lrd-1.0 st0=lrd-1.0
    fabs                                ; st3=log(2) st2=lrd st1=lrd-1.0 st0=abs(lrd-1.0)
    fcomp   qword [.limit xWrtRIP]      ; st2=log(2) st1=lrd st0=lrd-1.0
    fnstsw  ax
    and     eax, 04500h
    jnz     .use_st1

    fstp    st0                         ; st1=log(2) st0=lrd
    fyl2x                               ; log(lrd)
    jmp     .done

.use_st1:
    fstp    st1                         ; st1=log(2) st0=lrd-1.0
    fyl2xp1                             ; log(lrd)

.done:
    leave
    ret
.one:   dq  1.0
.limit: dq  0.29
ENDPROC   RT_NOCRT(logl)


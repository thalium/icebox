; $Id: ldexpl.asm $
;; @file
; IPRT - No-CRT ldexpl - AMD64 & X86.
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
; Computes lrd * 2^exp
; @returns st(0)
; @param    lrd     [rbp + xCB*2]
; @param    exp     [ebp + 14h]  gcc:edi  msc:ecx
BEGINPROC RT_NOCRT(ldexpl)
    push    xBP
    mov     xBP, xSP
    sub     xSP, 10h

    ; load exp
%ifdef RT_ARCH_AMD64 ; ASSUMES ONLY GCC HERE!
    mov     [rsp], edi
    fild    dword [rsp]
%else
    fild    dword [ebp + xCB*2 + RTLRD_CB]
%endif
    fld     tword [xBP + xCB*2]
    fscale
    fstp    st1

    leave
    ret
ENDPROC   RT_NOCRT(ldexpl)


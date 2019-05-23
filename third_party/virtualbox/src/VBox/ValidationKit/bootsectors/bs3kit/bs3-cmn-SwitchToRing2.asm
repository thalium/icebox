; $Id: bs3-cmn-SwitchToRing2.asm $
;; @file
; BS3Kit - Bs3SwitchToRing2
;

;
; Copyright (C) 2007-2017 Oracle Corporation
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

;*********************************************************************************************************************************
;*  Header Files                                                                                                                 *
;*********************************************************************************************************************************
%include "bs3kit-template-header.mac"


;*********************************************************************************************************************************
;*  External Symbols                                                                                                             *
;*********************************************************************************************************************************
BS3_EXTERN_CMN_FAR Bs3SwitchToRingX
TMPL_BEGIN_TEXT


;;
; @cproto   BS3_DECL(void) Bs3SwitchToRing2(void);
;
; @remarks  Does not require 20h of parameter scratch space in 64-bit mode.
; @uses     No GPRs.
;
BS3_PROC_BEGIN_CMN Bs3SwitchToRing2, BS3_PBC_HYBRID_0_ARGS
%if TMPL_BITS == 64
        push    rcx
        sub     rsp, 20h
        mov     ecx, 2
        mov     [rsp], rcx
        call    Bs3SwitchToRingX
        add     rsp, 20h
        pop     rcx
%else
        push    2
TONLY16 push    cs
        call    Bs3SwitchToRingX
        add     xSP, xCB
%endif
        BS3_HYBRID_RET
BS3_PROC_END_CMN   Bs3SwitchToRing2


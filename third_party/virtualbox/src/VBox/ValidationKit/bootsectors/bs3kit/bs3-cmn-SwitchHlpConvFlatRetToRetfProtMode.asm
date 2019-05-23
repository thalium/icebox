; $Id: bs3-cmn-SwitchHlpConvFlatRetToRetfProtMode.asm $
;; @file
; BS3Kit - SwitchHlpConvFlatRetToRetfProtMode
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

%include "bs3kit-template-header.mac"

%if TMPL_BITS != 16
BS3_EXTERN_CMN Bs3SelFlatCodeToProtFar16

;;
; SwitchToXxx helper that converts a 32-bit or 64-bit flat return address
; into a 16-bit protected mode far return.
;
;
; The caller calls this routine before switching modes. The flat return
; to be converted is immediately after our own return address on the stack.
;
; @uses     Nothing.
; @remarks  No 16-bit version.
;
BS3_PROC_BEGIN_CMN Bs3SwitchHlpConvFlatRetToRetfProtMode, BS3_PBC_NEAR
 %if TMPL_BITS == 64
        push        xAX
        push        xCX
        sub         xSP, 20h

        mov         xCX, [xSP + xCB*3 + 20h]
        call        Bs3SelFlatCodeToProtFar16 ; well behaved assembly function, only clobbers ecx
        mov         [xSP + xCB*3 + 20h + 4], eax

        add         xSP, 20h
        pop         xCX
        pop         xAX
        ret         4
 %else
        xchg        eax, [xSP + xCB]
        push        xAX
        call        Bs3SelFlatCodeToProtFar16 ; well behaved assembly function, only clobbers eax
        add         xSP, 4
        xchg        [xSP + xCB], eax
        ret
 %endif
BS3_PROC_END_CMN   Bs3SwitchHlpConvFlatRetToRetfProtMode

%endif ; 32 || 64


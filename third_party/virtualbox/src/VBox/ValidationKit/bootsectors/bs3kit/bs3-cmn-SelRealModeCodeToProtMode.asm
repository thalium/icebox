; $Id: bs3-cmn-SelRealModeCodeToProtMode.asm $
;; @file
; BS3Kit - Bs3SelRealModeCodeToProtMode.
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

;
; Make sure we can get at all the segments.
;
BS3_BEGIN_TEXT16
BS3_BEGIN_RMTEXT16
BS3_BEGIN_X0TEXT16
BS3_BEGIN_X1TEXT16
TMPL_BEGIN_TEXT


;;
; @cproto   BS3_CMN_PROTO(uint16_t, Bs3SelRealModeCodeToProtMode,(uint16_t uRealSel), false);
; @uses     ax (return register)
;
BS3_PROC_BEGIN_CMN Bs3SelRealModeCodeToProtMode, BS3_PBC_NEAR
        BS3_CALL_CONV_PROLOG 1
        push    xBP
        mov     xBP, xSP

        mov     ax, [xBP + xCB + cbCurRetAddr]
        cmp     ax, CGROUP16
        je      .bs3text16
        cmp     ax, BS3GROUPRMTEXT16
        je      .bs3rmtext16
        cmp     ax, BS3GROUPX0TEXT16
        je      .bs3x0text16
        cmp     ax, BS3GROUPX1TEXT16
        je      .bs3x1text16

        extern  BS3_CMN_NM(Bs3Panic)
        call    BS3_CMN_NM(Bs3Panic)
        jmp     .return

.bs3x1text16:
        mov     ax, BS3_SEL_X1TEXT16_CS
        jmp     .return
.bs3x0text16:
        mov     ax, BS3_SEL_X0TEXT16_CS
        jmp     .return
.bs3rmtext16:
        mov     ax, BS3_SEL_RMTEXT16_CS
        jmp     .return
.bs3text16:
        mov     ax, BS3_SEL_R0_CS16
.return:
        pop     xBP
        BS3_CALL_CONV_EPILOG 1
        BS3_HYBRID_RET
BS3_PROC_END_CMN   Bs3SelRealModeCodeToProtMode

;
; We may be using the near code in some critical code paths, so don't
; penalize it.
;
BS3_CMN_FAR_STUB   Bs3SelRealModeCodeToProtMode, 2


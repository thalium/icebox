; $Id: bs3-cmn-KbdWait.asm $
;; @file
; BS3Kit - Bs3KbdWait.
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

;;
; Waits for the keyboard controller to become ready.
;
; @cproto   BS3_DECL(void) Bs3KbdWait_c16(void);
;
BS3_PROC_BEGIN_CMN Bs3KbdWait, BS3_PBC_HYBRID_0_ARGS
        push    xBP
        mov     xBP, xSP
        push    xAX

.check_status:
        in      al, 64h
        test    al, 1                   ; KBD_STAT_OBF
        jnz     .read_data_and_status
        test    al, 2                   ; KBD_STAT_IBF
        jnz     .check_status

        pop     xAX
        pop     xBP
        BS3_HYBRID_RET

.read_data_and_status:
        in      al, 60h
        jmp     .check_status
BS3_PROC_END_CMN   Bs3KbdWait


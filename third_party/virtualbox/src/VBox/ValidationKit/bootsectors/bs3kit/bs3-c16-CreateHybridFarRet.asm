; $Id: bs3-c16-CreateHybridFarRet.asm $
;; @file
; BS3Kit - Bs3A20Disable.
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


%include "bs3kit.mac"


;;
; Worker for BS3_PROC_BEGIN_CMN
; @uses nothing
BS3_PROC_BEGIN Bs3CreateHybridFarRet_c16
        push    ax                      ; reserve space
        push    bp
        mov     bp, sp
        push    ax                      ; save it

        ; Move the return address up a word.
        mov     ax, [bp + 4]
        mov     [bp + 2], ax
        ; Move the caller's return address up a word.
        mov     ax, [bp + 6]
        mov     [bp + 4], ax
        ; Add CS to the caller's far return address.
        mov     [bp + 6], cs

        pop     ax
        pop     bp
        ret
BS3_PROC_END   Bs3CreateHybridFarRet_c16


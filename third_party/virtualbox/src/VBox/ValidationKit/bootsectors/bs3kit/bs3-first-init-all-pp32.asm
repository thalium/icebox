; $Id: bs3-first-init-all-pp32.asm $
;; @file
; BS3Kit - First Object, calling 32-bit paged protected mode main() after full init.
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


;
; Segment defs, grouping and related variables.
; Defines the entry point 'start' as well, leaving us in BS3TEXT16.
;
%include "bs3-first-common.mac"

extern NAME(Bs3InitAll_rm)
extern NAME(Bs3SwitchToPP32_rm)

;; Entry point.
        push    word 0                  ; zero return address.
        push    word 0                  ; zero caller BP
        mov     bp, sp

        ;
        ; Init all while we're in real mode.
        ;
        mov     ax, BS3_SEL_DATA16
        mov     es, ax
        mov     ds, ax
        call    NAME(Bs3InitAll_rm)

        ;
        ; Switch to 32-bit protected mode and call main.
        ;
        call    NAME(Bs3SwitchToPP32_rm)
        BS3_SET_BITS 32
        call    _Main_pp32
extern _Main_pp32
BS3_EXTERN_CMN Bs3Shutdown
        call    Bs3Shutdown


; $Id: bootsector2-boot-registers-1.asm $
;; @file
; Bootsector that prints the register status at boot.
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


%include "iprt/asmdefs.mac"
%include "iprt/x86.mac"
%include "VBox/VMMDevTesting.mac"

;
; Initialize in real mode, preserving the registers.
;
%define BS2_INIT_RM
%define BS2_INIT_SAVE_REGS
%include "bootsector2-common-init-code.mac"

main:
        mov     ax, BS2_REG_SAVE_ADDR
        call    NAME(TestDumpRegisters_r86)

        xor     ax, ax
        mov     al, [es:di]
        push    ax
        mov     al, [es:di + 1]
        push    ax
        mov     al, [es:di + 2]
        push    ax
        mov     al, [es:di + 3]
        push    ax
        push    ds
        push    .s_szPnpFmt1
        call    NAME(PrintF_r86)
        pop     ax
        push    .s_szPnpFmt2
        call    NAME(PrintF_r86)
        pop     ax
        push    .s_szPnpFmt3
        call    NAME(PrintF_r86)

        call    NAME(Bs2Panic)

.s_szPnpFmt1:
        db      'es:di -> %RX8 %RX8 %RX8 %RX8 ',0
.s_szPnpFmt2:
        db      '%c%c%c%c', 0
.s_szPnpFmt3:
        db      13, 10, 0

;
; Pad the image so it loads cleanly.
;
BS2_PAD_IMAGE
the_end:


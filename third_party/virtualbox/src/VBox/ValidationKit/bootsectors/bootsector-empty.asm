; $Id: bootsector-empty.asm $
;; @file
; Empty bootsector can be used as example
;

;
; Copyright (C) 2012-2017 Oracle Corporation
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


;; The boot sector load address.
%define BS_ADDR         0x7c00
%define PDP_ADDR        0x9000
%define PD_ADDR         0xa000


BITS 16
start:
    ; Start with a jump just to follow the convention.
    jmp short the_code
    nop
times 3ah db 0

the_code:
    ; put the code here



hlt_again:
    hlt
    cli
    jmp hlt_again

    ;
    ; The GDT.
    ;
padding:
times 510 - (padding - start) db 0
    db 055h, 0aah


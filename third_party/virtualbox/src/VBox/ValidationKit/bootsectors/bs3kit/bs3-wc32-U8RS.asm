; $Id: bs3-wc32-U8RS.asm $
;; @file
; BS3Kit - 32-bit Watcom C/C++, 64-bit unsigned integer right shift.
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
; 64-bit unsigned integer right shift.
;
; @returns  EDX:EAX
; @param    EDX:EAX     Value to shift.
; @param    BL          Shift count (it's specified as ECX:EBX, but we only use BL).
;
global $??U8RS
$??U8RS:
        push    ecx                     ; We're allowed to trash ECX, but why bother.

        mov     cl, bl
        and     cl, 3fh
        test    cl, 20h
        jnz     .big_shift

        ; Shifting less than 32.
        shrd    eax, edx, cl
        shr     edx, cl

.return:
        pop     ecx
        ret

.big_shift:
        ; Shifting 32 or more.
        mov     eax, edx
        shr     eax, cl                 ; Only uses lower 5 bits.
        xor     edx, edx
        jmp     .return


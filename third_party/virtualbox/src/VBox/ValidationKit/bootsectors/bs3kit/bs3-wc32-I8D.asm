; $Id: bs3-wc32-I8D.asm $
;; @file
; BS3Kit - 32-bit Watcom C/C++, 64-bit signed integer division.
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

BS3_EXTERN_CMN Bs3Int64Div


;;
; 64-bit signed integer division.
;
; @returns  EDX:EAX Quotient, ECX:EBX Remainder.
; @param    EDX:EAX     Dividend.
; @param    ECX:EBX     Divisor
;
global $??I8D
$??I8D:
        ;
        ; Convert to a C __cdecl call - not doing this in assembly.
        ;

        ; Set up a frame, allocating 16 bytes for the result buffer.
        push    ebp
        mov     ebp, esp
        sub     esp, 10h

        ; Pointer to the return buffer.
        push    esp

        ; The dividend.
        push    ecx
        push    ebx

        ; The dividend.
        push    edx
        push    eax

        call    Bs3Int64Div

        ; Load the result.
        mov     ecx, [ebp - 10h + 12]
        mov     ebx, [ebp - 10h + 8]
        mov     edx, [ebp - 10h + 4]
        mov     eax, [ebp - 10h]

        leave
        ret


; $Id: bs3-cmn-PrintU32.asm $
;; @file
; BS3Kit - Bs3PrintU32, Common.
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


BS3_EXTERN_CMN Bs3PrintStr

;;
; Prints a 32-bit unsigned integer value.
;
; @param    [xBP + xCB*2]   32-bit value to format and print.
;
BS3_PROC_BEGIN_CMN Bs3PrintU32, BS3_PBC_HYBRID
        BS3_CALL_CONV_PROLOG 1
        push    xBP
        mov     xBP, xSP
        push    sAX
        push    sDX
        push    sCX
        push    sBX
BONLY16 push    ds

        mov     eax, [xBP + xCB + cbCurRetAddr]

        ; Allocate a stack buffer and terminate it. ds:bx points ot the end.
        sub     xSP, 30h
BONLY16 mov     bx, ss
BONLY16 mov     ds, bx
        mov     xBX, xSP
        add     xBX, 2fh
        mov     byte [xBX], 0

        mov     ecx, 10                 ; what to divide by
.next:
        xor     edx, edx
        div     ecx                     ; edx:eax / ecx -> eax and rest in edx.
        add     dl, '0'
        dec     xBX
        mov     [BS3_ONLY_16BIT(ss:)xBX], dl
        cmp     eax, 0
        jnz     .next

        ; Print the string.
BONLY64 add     rsp, 18h
BONLY16 push    ss
        push    xBX
        BS3_CALL Bs3PrintStr, 1

        add     xSP, 30h + BS3_IF_16_32_64BIT(2, 0, 18h) + xCB
BONLY16 pop     ds
        pop     sBX
        pop     sCX
        pop     sDX
        pop     sAX
        leave
        BS3_CALL_CONV_EPILOG 1
        BS3_HYBRID_RET
BS3_PROC_END_CMN   Bs3PrintU32


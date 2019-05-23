; $Id: bs3-wc16-I4D.asm $
;; @file
; BS3Kit - 16-bit Watcom C/C++, 32-bit signed integer division.
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
; 32-bit signed integer division.
;
; @returns  DX:AX quotient, CX:BX remainder.
; @param    DX:AX           Dividend.
; @param    CX:BX           Divisor
;
; @uses     Nothing.
;
global $_?I4D
$_?I4D:
;; @todo no idea if we're getting the negative division stuff right here according to what watcom expectes...
extern TODO_NEGATIVE_SIGNED_DIVISION
        ; Move dividend into EDX:EAX
        shl     eax, 10h
        mov     ax, dx
        sar     dx, 0fh
        movsx   edx, dx

        ; Move divisor into ebx.
        shl     ebx, 10h
        mov     bx, cx

        ; Do it!
        idiv    ebx

        ; Reminder in to CX:BX
        mov     bx, dx
        shr     edx, 10h
        mov     cx, dx

        ; Quotient into DX:AX
        mov     edx, eax
        shr     edx, 10h

%ifdef ASM_MODEL_FAR_CODE
        retf
%else
        ret
%endif


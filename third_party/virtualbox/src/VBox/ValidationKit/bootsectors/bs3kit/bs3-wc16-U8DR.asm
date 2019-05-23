; $Id: bs3-wc16-U8DR.asm $
;; @file
; BS3Kit - 16-bit Watcom C/C++, 64-bit unsigned integer modulo.
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

BS3_EXTERN_CMN Bs3UInt64Div


;;
; 64-bit unsigned integer modulo, SS variant.
;
; @returns  ax:bx:cx:dx reminder. (AX is the most significant, DX the least)
; @param    ax:bx:cx:dx     Dividend.
; @param    [ss:si]         Divisor
;
; @uses     Nothing.
;
global $_?U8DR
$_?U8DR:
        push    es
        push    ss
        pop     es
%ifdef ASM_MODEL_FAR_CODE
        push    cs
%endif
        call    $_?U8DRE
        pop     es
%ifdef ASM_MODEL_FAR_CODE
        retf
%else
        ret
%endif

;;
; 64-bit unsigned integer modulo, ES variant.
;
; @returns  ax:bx:cx:dx reminder. (AX is the most significant, DX the least)
; @param    ax:bx:cx:dx     Dividend.
; @param    [es:si]         Divisor
;
; @uses     Nothing.
;
global $_?U8DRE
$_?U8DRE:
        push    ds
        push    es

        ;
        ; Convert to a C __cdecl call - not doing this in assembly.
        ;

        ; Set up a frame of sorts, allocating 16 bytes for the result buffer.
        push    bp
        sub     sp, 10h
        mov     bp, sp

        ; Pointer to the return buffer.
        push    ss
        push    bp
        add     bp, 10h                 ; Correct bp.

        ; The divisor.
        push    word [es:si + 6]
        push    word [es:si + 4]
        push    word [es:si + 2]
        push    word [es:si]

        ; The dividend.
        push    ax
        push    bx
        push    cx
        push    dx

        call    Bs3UInt64Div

        ; Load the reminder.
        mov     ax, [bp - 10h + 8 + 6]
        mov     bx, [bp - 10h + 8 + 4]
        mov     cx, [bp - 10h + 8 + 2]
        mov     dx, [bp - 10h + 8]

        mov     sp, bp
        pop     bp
        pop     es
        pop     ds
%ifdef ASM_MODEL_FAR_CODE
        retf
%else
        ret
%endif


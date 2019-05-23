; $Id: fmemcpy.asm $
;; @file
; Compiler support routines.
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


;*******************************************************************************
;*  Exported Symbols                                                           *
;*******************************************************************************
public          _fmemcpy_


                .8086

_TEXT           segment public 'CODE' use16
                assume cs:_TEXT


;;
; memcpy taking far pointers.
;
; cx, es may be modified; si, di are preserved
;
; @returns  dx:ax unchanged.
; @param    dx:ax   Pointer to the destination memory.
; @param    cx:bx   Pointer to the source memory.
; @param    sp+2    The number of bytes to copy (dw).
;
_fmemcpy_:
                push    bp
                mov     bp, sp
                push    di
                push    ds
                push    si

                mov     es, dx
                mov     di, ax
                mov     ds, cx
                mov     si, bx
                mov     cx, [bp + 4]
                rep     movsb

                pop     si
                pop     ds
                pop     di
                mov     sp, bp
                pop     bp
                ret


_TEXT           ends
                end


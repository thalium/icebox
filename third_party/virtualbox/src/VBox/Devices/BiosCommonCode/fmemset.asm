; $Id: fmemset.asm $
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
public          _fmemset_


                .8086

_TEXT           segment public 'CODE' use16
                assume cs:_TEXT


;;
; memset taking a far pointer.
;
; cx, es may be modified; di is preserved
;
; @returns  dx:ax unchanged.
; @param    dx:ax   Pointer to the memory.
; @param    bl      The fill value.
; @param    cx      The number of bytes to fill.
;
_fmemset_:
                push    di

                mov     es, dx
                mov     di, ax
                xchg    al, bl
                rep stosb
                xchg    al, bl

                pop     di
                ret

_TEXT           ends
                end


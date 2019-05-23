; $Id: __U8RS.asm $
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
public          __U8RS


                .8086

_TEXT           segment public 'CODE' use16
                assume cs:_TEXT

;;
; 64-bit unsigned right shift.
;
; @param    ax:bx:cx:dx Value. (AX is the most significant, DX the least)
; @param    si          Shift count.
; @returns  ax:bx:cx:dx Shifted value.
; si is zeroed
;
__U8RS:

                test    si, si
                jz      u8rs_quit
u8rs_rot:
                shr     ax, 1
                rcr     bx, 1
                rcr     cx, 1
                rcr     dx, 1
                dec     si
                jnz     u8rs_rot
u8rs_quit:
                ret

_TEXT           ends
                end


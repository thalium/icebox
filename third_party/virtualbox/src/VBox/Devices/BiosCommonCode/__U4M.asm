; $Id: __U4M.asm $
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
public          __U4M

                .8086

_TEXT           segment public 'CODE' use16
                assume cs:_TEXT

;;
; 32-bit unsigned multiplication.
;
; @param    dx:ax   Factor 1.
; @param    cx:bx   Factor 2.
; @returns  dx:ax   Result.
;
__U4M:
                pushf
if VBOX_BIOS_CPU ge 80386
                .386
                push    eax
                push    edx
                push    ecx

                rol     eax, 16
                mov     ax, dx
                ror     eax, 16
                xor     edx, edx

                shr     ecx, 16
                mov     cx, bx

                mul     ecx                 ; eax * ecx -> edx:eax

                pop     ecx

                pop     edx
                ror     eax, 16
                mov     dx, ax
                add     sp, 2
                pop     ax
                rol     eax, 16
                .8086

else
                push    si              ; high result
                push    di              ; low result

                ;
                ;        dx:ax * cx:bx =
                ;-----------------------
                ;        ax*bx
                ; +   dx*bx             ; only lower 16 bits relevant.
                ; +   ax*cx             ; ditto
                ; +dx*cx                ; not relevant
                ; -------------
                ; =      dx:ax
                ;

                push    ax              ; stash the low factor 1 part for the 3rd multiplication.
                mov     di, dx          ; stash the high factor 1 part for the 2nd multiplication.

                ; multiply the two low factor "digits": ax * bx
                mul     bx
                mov     si, dx
                xchg    di, ax          ; save low result and loads high factor 1 into ax for the next step

                ; Multiply the low right "digit" by the high left one and add it to the high result part
                mul     bx
                add     si, ax

                ; Multiply the high right "digit" by the low left on and add it ot the high result part.
                pop     ax
                mul     cx
                add     si, ax

                ; Load the result.
                mov     dx, si
                mov     ax, di

                pop     di
                pop     si
endif
                popf
                ret


_TEXT           ends
                end


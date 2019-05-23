; $Id: __I4D.asm $
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
public          __I4D

if VBOX_BIOS_CPU lt 80386
extrn NeedToImplementOn8086__I4D:near
endif

                .8086


_TEXT           segment public 'CODE' use16
                assume cs:_TEXT

;;
; 32-bit signed division.
;
; @param    dx:ax   Dividend.
; @param    cx:bx   Divisor.
; @returns  dx:ax   Quotient.
;           cx:bx   Remainder.
;
__I4D:
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

                idiv    ecx                 ; eax:edx / ecx -> eax=quotient, edx=remainder.

                mov     bx, dx
                pop     ecx
                shr     edx, 16
                mov     cx, dx

                pop     edx
                ror     eax, 16
                mov     dx, ax
                add     sp, 2
                pop     ax
                rol     eax, 16
                .8086
else
                call    NeedToImplementOn8086__I4D
endif
                popf
                ret


_TEXT           ends
                end


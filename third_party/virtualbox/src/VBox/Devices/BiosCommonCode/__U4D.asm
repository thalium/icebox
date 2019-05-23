; $Id: __U4D.asm $
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
public          __U4D

                .8086

if VBOX_BIOS_CPU lt 80386
extrn _DoUInt32Div:near
endif


_TEXT           segment public 'CODE' use16
                assume cs:_TEXT


;;
; 32-bit unsigned division.
;
; @param    dx:ax   Dividend.
; @param    cx:bx   Divisor.
; @returns  dx:ax   Quotient.
;           cx:bx   Remainder.
;
__U4D:
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

                div     ecx                 ; eax:edx / ecx -> eax=quotient, edx=remainder.

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

                ; Call C function do this.
                push    ds
                push    es

                ;
                ; Convert to a C __cdecl call - not doing this in assembly.
                ;

                ; Set up a frame of sorts, allocating 4 bytes for the result buffer.
                push    bp
                sub     sp, 04h
                mov     bp, sp

                ; Pointer to the return buffer.
                push    ss
                push    bp
                add     bp, 04h                 ; Correct bp.

                ; The divisor.
                push    cx
                push    bx

                ; The dividend.
                push    dx
                push    ax

                call    _DoUInt32Div

                ; Load the remainder.
                mov     cx, [bp - 02h]
                mov     bx, [bp - 04h]

                ; The quotient is already in dx:ax

                mov     sp, bp
                pop     bp
                pop     es
                pop     ds
endif
                popf
                ret

_TEXT           ends
                end


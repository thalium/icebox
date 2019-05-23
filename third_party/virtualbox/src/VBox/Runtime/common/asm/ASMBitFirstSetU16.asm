; $Id: ASMBitFirstSetU16.asm $
;; @file
; IPRT - ASMBitFirstSetU16().
;

;
; Copyright (C) 2006-2017 Oracle Corporation
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


;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"

BEGINCODE

;;
; Finds the first bit which is set in the given 16-bit integer.
;
; Bits are numbered from 1 (least significant) to 16.
;
; @returns (xAX)        index [1..16] of the first set bit.
; @returns (xAX)        0 if all bits are cleared.
; @param   msc:cx gcc:di x86:stack u16  Integer to search for set bits.
;
; @cproto DECLASM(unsigned) ASMBitFirstSetU16(uint16_t u16);
;
BEGINPROC_EXPORTED ASMBitFirstSetU16
%if ARCH_BITS == 16
        CPU     8086
        push    bp
        mov     bp, sp

        ; 16:0
        mov     ax, 1
        mov     cx, [bp + 2 + 2 + 0]
        test    cx, cx
        jz      .return_zero

        ; find the bit that was set.
.next_bit:
        shr     cx, 1
        jc      .return
        inc     ax
        jmp     .next_bit

.return_zero:
        xor     ax, ax
.return:
        pop     bp
        ret

%else
        xor     eax, eax
 %if    ARCH_BITS == 64
  %ifdef ASM_CALL64_GCC
        bsf     ax, si
  %else
        bsf     ax, cx
  %endif
 %elif ARCH_BITS == 32
        bsf     ax, word [esp + 4]
 %else
  %error "Missing or invalid ARCH_BITS."
 %endif
        jz      .return_zero
        inc     eax
.return:
        ret
.return_zero:
        xor     eax, eax
        ret
%endif
ENDPROC ASMBitFirstSetU16


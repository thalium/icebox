; $Id: ASMBitLastSetU16.asm $
;; @file
; BiosCommonCode - ASMBitLastSetU16() - borrowed from IPRT.
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


;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
public _ASMBitLastSetU16

        .8086

_TEXT   segment public 'CODE' use16
        assume cs:_TEXT


;;
; Finds the last bit which is set in the given 16-bit integer.
;
; Bits are numbered from 1 (least significant) to 16.
;
; @returns (ax)     index [1..16] of the last set bit.
; @returns (ax)     0 if all bits are cleared.
; @param   u16      Integer to search for set bits.
;
; @cproto DECLASM(unsigned) ASMBitLastSetU16(uint32_t u16);
;
_ASMBitLastSetU16   proc
        .8086
        push    bp
        mov     bp, sp

        mov     cx, [bp + 2 + 2]
        test    cx, cx                  ; check if zero (eliminates checking dec ax result)
        jz      return_zero

        mov     ax, 16
next_bit:
        shl     cx, 1
        jc      return
        dec     ax
        jmp     next_bit

return_zero:
        xor     ax, ax
return:
        pop     bp
        ret
_ASMBitLastSetU16   endp

_TEXT           ends
                end


; $Id: bs3-cmn-SwitchHlpConvRealModeRetfPopBpDecBpAndReturn.asm $
;; @file
; BS3Kit - SwitchHlpConvRealModeRetfPopBpDecBpAndReturn
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

BS3_EXTERN_CMN Bs3SelFar32ToFlat32

;;
; SwitchToXxx helper that converts a 16-bit real mode far return
; into something suitable for the current mode and performs the return.
;
; The caller jmps to this routine.  The stack holds an incremented BP (odd is
; far indicator) and a 16-bit far return address.
;
; @uses     Nothing.
; @remarks  16-bit ASSUMES we're returning to protected mode!!
;
%if TMPL_BITS == 16
BS3_BEGIN_TEXT16_FARSTUBS
%endif
BS3_PROC_BEGIN_CMN Bs3SwitchHlpConvRealModeRetfPopBpDecBpAndReturn, BS3_PBC_NEAR
%if TMPL_BITS == 16
        ; Convert the selector of the 16:16 real mode return address to the
        ; corresponding 16-bit protected mode selector.
        push        ax

        mov         ax, [bp + 2 + 2]
        push        ax
        BS3_EXTERN_CMN Bs3SelRealModeCodeToProtMode
        call        Bs3SelRealModeCodeToProtMode    ; This doesn't trash any registers (except AX).
        add         sp, 2
        mov         [bp + 2 + 2], ax

        pop         ax

        pop         bp
        dec         bp
        retf

%elif TMPL_BITS == 32
        push    xAX
        push    xDX

        movzx   eax, word [xSP + xCB*2 + 2 + 2]     ; return segment
        movzx   edx, word [xSP + xCB*2 + 2]         ; return offset
        shl     eax, 4
        add     eax, edx
        mov     [xSP + xCB*2 + 2], eax

        pop     xDX
        pop     xAX
        pop     bp
        dec     bp
        ret
%else
        sub     rsp, 2h

        push    xAX
        push    xDX

        movzx   eax, word [xSP + xCB*2 + 4 + 2]     ; return segment
        movzx   edx, word [xSP + xCB*2 + 4]         ; return offset
        shl     eax, 4
        add     eax, edx

        mov     bp, [xSP + xCB*2 + 2]
        dec     bp

        mov     [xSP + xCB*2], rax

        pop     xDX
        pop     xAX
        ret
%endif
BS3_PROC_END_CMN   Bs3SwitchHlpConvRealModeRetfPopBpDecBpAndReturn


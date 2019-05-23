; $Id: bs3-cmn-SwitchHlpConvProtModeRetfPopBpDecBpAndReturn.asm $
;; @file
; BS3Kit - Bs3SwitchToPP32
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


%if TMPL_BITS == 16
BS3_EXTERN_CMN Bs3SelProtModeCodeToRealMode
%else
BS3_EXTERN_CMN Bs3SelFar32ToFlat32
%endif


;;
; SwitchToXxx helper that converts a 16-bit protected mode far return
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
BS3_PROC_BEGIN_CMN Bs3SwitchHlpConvProtModeRetfPopBpDecBpAndReturn, BS3_PBC_NEAR
%if TMPL_BITS == 16
        ; Convert the selector of the 16:16 protected mode return address to the
        ; corresponding 16-bit real mode segment.
        push        ax

        mov         ax, [bp + 2 + 2]
        push        ax
        call        Bs3SelProtModeCodeToRealMode ; This doesn't trash any registers (except AX).
        add         sp, 2
        mov         [bp + 2 + 2], ax

        pop         ax

        pop         bp
        dec         bp
        retf

%elif TMPL_BITS == 32
        push    eax
        push    ecx
        push    edx

        movzx   eax, word [esp + 4*3 + 2]       ; return offset
        movzx   edx, word [esp + 4*3 + 2 + 2]   ; return selector
        push    eax
        push    edx
        call    Bs3SelFar32ToFlat32
        add     esp, 8
        mov     [esp + 4*3 + 2], eax

        pop     edx
        pop     ecx
        pop     eax
        pop     bp
        dec     bp
        ret
%else
        push    rax
        push    rcx
        push    rdx
        push    r8
        push    r9
        push    r10
        push    r11

        movzx   ecx, word [rsp + 8*7 + 2]       ; return offset
        movzx   edx, word [rsp + 8*7 + 2 + 2]   ; return selector
        sub     rsp, 20h
        call    Bs3SelFar32ToFlat32
        add     rsp, 20h
        mov     [rsp + 8*7 + 2], eax

        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rdx
        pop     rcx
        pop     rax
        mov     bp, [rsp]
        add     rsp, 2h
        dec     bp
        o32 ret
%endif
BS3_PROC_END_CMN   Bs3SwitchHlpConvProtModeRetfPopBpDecBpAndReturn


; $Id: bs3-cmn-MemZero.asm $
;; @file
; BS3Kit - Bs3MemZero.
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

;;
; @cproto   BS3_DECL(void) Bs3MemZero_c16(void BS3_FAR *pvDst, size_t cbDst);
;
BS3_PROC_BEGIN_CMN Bs3MemZero, BS3_PBC_HYBRID
%ifdef RT_ARCH_AMD64
        push    rdi

        mov     rdi, rcx                ; rdi = pvDst
        mov     rcx, rdx                ; rcx = cbDst
        shr     rcx, 3                  ; calc qword count.
        xor     eax, eax                ; rax = 0 (filler qword)
        cld
        rep stosq

        mov     rcx, rdx                ; cbDst
        and     rcx, 7                  ; calc trailing byte count.
        rep stosb

        pop     rdi
        BS3_HYBRID_RET

%elif ARCH_BITS == 16
        push    bp
        mov     bp, sp
        push    di
        push    es

        mov     di, [bp + 2 + cbCurRetAddr]     ; pvDst.off
        mov     dx, [bp + 2 + cbCurRetAddr + 2] ; pvDst.sel
        mov     es, dx
        mov     cx, [bp + 2 + cbCurRetAddr + 4] ; cbDst
        shr     cx, 1                           ; calc dword count.
        xor     ax, ax
        rep stosw

        mov     cx, [bp + 2 + cbCurRetAddr + 4] ; cbDst
        and     cx, 1                           ; calc tailing byte count.
        rep stosb

        pop     es
        pop     di
        pop     bp
        BS3_HYBRID_RET

%elif ARCH_BITS == 32
        push    edi

        mov     edi, [esp + 8]          ; pvDst
        mov     ecx, [esp + 8 + 4]      ; cbDst
        shr     cx, 2                   ; calc dword count.
        xor     eax, eax
        rep stosd

        mov     ecx, [esp + 8 + 4]      ; cbDst
        and     ecx, 3                  ; calc tailing byte count.
        rep stosb

        pop     edi
        BS3_HYBRID_RET

%else
 %error "Unknown bitness."
%endif
BS3_PROC_END_CMN   Bs3MemZero


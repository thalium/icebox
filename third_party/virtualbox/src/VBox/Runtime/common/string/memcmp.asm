; $Id: memcmp.asm $
;; @file
; IPRT - No-CRT memcmp - AMD64 & X86.
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

%include "iprt/asmdefs.mac"

BEGINCODE

;;
; @param    pv1     gcc: rdi  msc: rcx  x86:[esp+4]
; @param    pv2     gcc: rsi  msc: rdx  x86:[esp+8]
; @param    cb      gcc: rdx  msc: r8   x86:[esp+0ch]
RT_NOCRT_BEGINPROC memcmp
        cld
        xor     eax, eax

        ; Do the bulk of the work.
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        mov     r10, rdi                ; save
        mov     r11, rsi                ; save
        mov     rdi, rcx
        mov     rsi, rdx
        mov     rcx, r8
        mov     rdx, r8
 %else
        mov     rcx, rdx
 %endif
        shr     rcx, 3
        repe cmpsq
        jne     .not_equal_qword
%else
        push    edi
        push    esi

        mov     ecx, [esp + 0ch + 8]
        mov     edi, [esp + 04h + 8]
        mov     esi, [esp + 08h + 8]
        mov     edx, ecx
        jecxz   .done
        shr     ecx, 2
        repe cmpsd
        jne     .not_equal_dword
%endif

        ; The remaining bytes.
%ifdef RT_ARCH_AMD64
        test    dl, 4
        jz      .dont_cmp_dword
        cmpsd
        jne     .not_equal_dword
%endif
.dont_cmp_dword:
        test    dl, 2
        jz      .dont_cmp_word
        cmpsw
        jne     .not_equal_word
.dont_cmp_word:
        test    dl, 1
        jz      .dont_cmp_byte
        cmpsb
        jne     .not_equal_byte
.dont_cmp_byte:

.done:
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        mov     rdi, r10
        mov     rsi, r11
 %endif
%else
        pop     esi
        pop     edi
%endif
        ret

;
; Mismatches.
;
%ifdef RT_ARCH_AMD64
.not_equal_qword:
        mov     ecx, 8
        sub     rsi, 8
        sub     rdi, 8
        repe cmpsb
.not_equal_byte:
        mov     al, [xDI-1]
        movzx   ecx, byte [xSI-1]
        sub     eax, ecx
        jmp     .done
%endif

.not_equal_dword:
        mov     ecx, 4
        sub     xSI, 4
        sub     xDI, 4
        repe cmpsb
%ifdef RT_ARCH_AMD64
        jmp     .not_equal_byte
%else
.not_equal_byte:
        mov     al, [xDI-1]
        movzx   ecx, byte [xSI-1]
        sub     eax, ecx
        jmp     .done
%endif

.not_equal_word:
        mov     ecx, 2
        sub     xSI, 2
        sub     xDI, 2
        repe cmpsb
        jmp     .not_equal_byte
ENDPROC RT_NOCRT(memcmp)


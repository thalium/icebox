; $Id: strcmp.asm $
;; @file
; IPRT - No-CRT strcmp - AMD64 & X86.
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
; @param    psz1   gcc: rdi  msc: rcx  x86:[esp+4]
; @param    psz2   gcc: rsi  msc: rdx  x86:[esp+8]
RT_NOCRT_BEGINPROC strcmp
        ; input
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
  %define psz1 rcx
  %define psz2 rdx
 %else
  %define psz1 rdi
  %define psz2 rsi
 %endif
%else
        mov     ecx, [esp + 4]
        mov     edx, [esp + 8]
  %define psz1 ecx
  %define psz2 edx
%endif

        ;
        ; The loop.
        ;
.next:
        mov     al, [psz1]
        mov     ah, [psz2]
        cmp     al, ah
        jne     .not_equal
        test    al, al
        jz      .equal

        mov     al, [psz1 + 1]
        mov     ah, [psz2 + 1]
        cmp     al, ah
        jne     .not_equal
        test    al, al
        jz      .equal

        mov     al, [psz1 + 2]
        mov     ah, [psz2 + 2]
        cmp     al, ah
        jne     .not_equal
        test    al, al
        jz      .equal

        mov     al, [psz1 + 3]
        mov     ah, [psz2 + 3]
        cmp     al, ah
        jne     .not_equal
        test    al, al
        jz      .equal

        add     psz1, 4
        add     psz2, 4
        jmp     .next

.equal:
        xor     eax, eax
        ret

.not_equal:
        movzx   ecx, ah
        and     eax, 0ffh
        sub     eax, ecx
        ret
ENDPROC RT_NOCRT(strcmp)


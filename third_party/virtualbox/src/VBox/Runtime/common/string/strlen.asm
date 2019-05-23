; $Id: strlen.asm $
;; @file
; IPRT - No-CRT strlen - AMD64 & X86.
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
; @param    psz     gcc: rdi  msc: rcx  x86: [esp+4]
RT_NOCRT_BEGINPROC strlen
        cld
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        mov     r9, rdi                 ; save rdi
        mov     rdi, rcx
 %endif
%else
        mov     edx, edi                ; save edi
        mov     edi, [esp + 4]
%endif

        ; do the search
        mov     xCX, -1
        xor     eax, eax
        repne   scasb

        ; found it
        neg     xCX
        lea     xAX, [xCX - 2]
%ifdef ASM_CALL64_MSC
        mov     rdi, r9
%endif
%ifdef RT_ARCH_X86
        mov     edi, edx
%endif
        ret
ENDPROC RT_NOCRT(strlen)


; $Id: tstAsmRegs-1.asm $
;; @file
; Disassembly testcase - Accessing all the registers
;
; This is a build test, that means it will be assembled, disassembled,
; then the disassembly output will be assembled and the new binary will
; compared with the original.
;

;
; Copyright (C) 2008-2017 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

%include "tstAsm.mac"

    BITS TEST_BITS

    ;; @todo
    ; Missing a bunch of permutations and encoding variants
    ;

    mov     al, al
    mov     al, ah
    mov     al, bl
    mov     al, bh
    mov     al, cl
    mov     al, ch
    mov     al, dl
    mov     al, dh
%if TEST_BITS == 64
    mov     al, dil
    mov     al, sil
    mov     al, bpl
    mov     al, spl
    mov     al, r8b
    mov     al, r9b
    mov     al, r10b
    mov     al, r11b
    mov     al, r12b
    mov     al, r13b
    mov     al, r14b
    mov     al, r15b
%endif

    mov     ax, ax
    mov     ax, bx
    mov     ax, cx
    mov     ax, dx
    mov     ax, si
    mov     ax, di
    mov     ax, bp
    mov     ax, sp
%if TEST_BITS == 64
    mov     ax, r8w
    mov     ax, r9w
    mov     ax, r10w
    mov     ax, r11w
    mov     ax, r12w
    mov     ax, r13w
    mov     ax, r14w
    mov     ax, r15w
%endif

    mov     eax, eax
    mov     eax, ebx
    mov     eax, ecx
    mov     eax, edx
    mov     eax, esi
    mov     eax, edi
    mov     eax, ebp
    mov     eax, esp
%if TEST_BITS == 64
    mov     eax, r8d
    mov     eax, r9d
    mov     eax, r10d
    mov     eax, r11d
    mov     eax, r12d
    mov     eax, r13d
    mov     eax, r14d
    mov     eax, r15d
%endif

%if TEST_BITS == 64
    mov     rax, rax
    mov     rax, rbx
    mov     rax, rcx
    mov     rax, rdx
    mov     rax, rsi
    mov     rax, rdi
    mov     rax, rbp
    mov     rax, rsp
    mov     rax, r8
    mov     rax, r9
    mov     rax, r10
    mov     rax, r11
    mov     rax, r12
    mov     rax, r13
    mov     rax, r14
    mov     rax, r15
%endif


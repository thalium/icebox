; $Id: tstAsmMovSeg-1.asm $
;; @file
; Disassembly testcase - Valid mov from/to segment instructions.
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

    BITS TEST_BITS

    mov fs, eax
    mov fs, ax
%if TEST_BITS == 64
    mov fs, rax
%endif

    mov fs, [ebx]
%if TEST_BITS != 64
    mov fs, [bx]
%else
    mov fs, [rbx]
%endif

    mov ax, fs
    mov eax, fs
%if TEST_BITS == 64
    mov rax, fs
%endif

    mov [ebx], fs
%if TEST_BITS != 64
    mov [bx], fs
%else
    mov [rbx], fs
%endif

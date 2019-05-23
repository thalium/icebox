; $Id: tstAsmSignExtend-1.asm $
;; @file
; Disassembly testcase - Valid sign extension instructions.
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

    movsx ax, al
    movsx eax, al
    movsx eax, ax

    ;
    ; ParseImmByteSX
    ;

    ; 83 /x
    add eax, strict byte 8
    add eax, strict byte -1
    cmp ebx, strict byte -1

    add ax, strict byte 8
    add ax, strict byte -1
    cmp bx, strict byte -1

%if TEST_BITS == 64 ; check that these come out with qword values and not words or dwords.
    add rax, strict byte 8
    add rax, strict byte -1
    cmp rbx, strict byte -1
%endif

    ; push %Ib
    push strict byte -1
    push strict byte -128
    push strict byte 127

    ;; @todo imul

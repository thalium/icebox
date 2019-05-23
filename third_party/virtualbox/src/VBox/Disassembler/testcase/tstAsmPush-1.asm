; $Id: tstAsmPush-1.asm $
;; @file
; Disassembly testcase - Valid push sequences and related instructions.
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
%if TEST_BITS != 64
    push    bp
    push    ebp
    push    word [bp]
    push    dword [bp]
    push    word [ebp]
    push    dword [ebp]
%else
 %if 0 ; doesn't work yet - default operand size is wrong?
    push    rbp
    push    qword [rbp]
 %endif
%endif


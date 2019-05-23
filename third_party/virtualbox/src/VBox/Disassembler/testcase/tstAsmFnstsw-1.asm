; $Id: tstAsmFnstsw-1.asm $
;; @file
; Disassembly testcase - Valid fnstsw* instructitons.
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

    fstsw ax
    fnstsw ax
    fstsw [xBX]
    fnstsw [xBX]
    fstsw [xBX+xDI]
    fnstsw [xBX+xDI]


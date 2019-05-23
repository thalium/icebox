; $Id: vboxport.asm $
;; @file
; asm specifics
;

;
; Copyright (C) 2013-2017 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"

BEGINCODE

;;
; Get the FPU control word
;
align 16
BEGINPROC VBoxAsmFpuFCWGet
    fnstcw  [xSP - 8]
    mov     ax, word [xSP - 8]
    ret
ENDPROC   VBoxAsmFpuFCWGet

;;
; Set the FPU control word
;
; @param  u16FCW New FPU control word
align 16
BEGINPROC VBoxAsmFpuFCWSet
    mov     xAX, rcx
    push    xAX
    fldcw   [xSP]
    pop     xAX
    ret
ENDPROC   VBoxAsmFpuFCWSet


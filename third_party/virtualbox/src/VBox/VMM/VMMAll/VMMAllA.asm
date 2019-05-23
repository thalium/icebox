; $Id: VMMAllA.asm $
;; @file
; VMM - All Contexts Assembly Routines.
;

;
; Copyright (C) 2009-2017 Oracle Corporation
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
%include "VBox/asmdefs.mac"


;*******************************************************************************
;* Defined Constants And Macros                                                *
;*******************************************************************************
%ifdef IN_RING3
 %ifdef RT_ARCH_AMD64
  %define VMM_TRASH_XMM_REGS
 %endif
%endif
%ifdef IN_RING0
 %ifdef RT_ARCH_AMD64
  %ifdef RT_OS_WINDOWS
   %define VMM_TRASH_XMM_REGS
  %endif
 %endif
%endif
%ifndef VMM_TRASH_XMM_REGS
 %ifdef VBOX_WITH_KERNEL_USING_XMM
  %define VMM_TRASH_XMM_REGS
 %endif
%endif

BEGINCODE


;;
; Trashes the volatile XMM registers in the current ABI.
;
BEGINPROC VMMTrashVolatileXMMRegs
%ifdef VMM_TRASH_XMM_REGS
        push    xBP
        mov     xBP, xSP

        ; take whatever is on the stack.
        and     xSP, ~15
        sub     xSP, 80h

        movdqa  xmm0, [xSP + 0]
        movdqa  xmm1, [xSP + 010h]
        movdqa  xmm2, [xSP + 020h]
        movdqa  xmm3, [xSP + 030h]
        movdqa  xmm4, [xSP + 040h]
        movdqa  xmm5, [xSP + 050h]
 %ifdef ASM_CALL64_GCC
        movdqa  xmm6, [xSP + 060h]
        movdqa  xmm7, [xSP + 070h]
        movdqa  xmm8, [xSP + 000h]
        movdqa  xmm9, [xSP + 010h]
        movdqa  xmm10,[xSP + 020h]
        movdqa  xmm11,[xSP + 030h]
        movdqa  xmm12,[xSP + 040h]
        movdqa  xmm13,[xSP + 050h]
        movdqa  xmm14,[xSP + 060h]
        movdqa  xmm15,[xSP + 070h]
 %endif
        leave
%endif ; VMM_TRASH_XMM_REGS
        xor     eax, eax                ; for good measure.
        ret
ENDPROC   VMMTrashVolatileXMMRegs


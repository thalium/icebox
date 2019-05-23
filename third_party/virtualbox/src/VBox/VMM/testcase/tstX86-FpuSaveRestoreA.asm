; $Id: tstX86-FpuSaveRestoreA.asm $
;; @file
; tstX86-FpuSaveRestore - Experimenting with saving and restoring FPU, assembly bits.
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
;*  Header Files                                                               *
;*******************************************************************************
%include "iprt/asmdefs.mac"
%include "iprt/x86.mac"


;*******************************************************************************
;*  Global Variables                                                           *
;*******************************************************************************
BEGINCODE
g_r80_Zero:     dt 0.0
g_r80_One:      dt 1.0


BEGINCODE

;; Prepares a FPU exception.
BEGINPROC MyFpuPrepXcpt
        fld tword [g_r80_One  xWrtRIP]
        fld tword [g_r80_Zero xWrtRIP]
        fdiv    st0
        ret
ENDPROC   MyFpuPrepXcpt


;; Same as above, just different address.
BEGINPROC MyFpuPrepXcpt2
        fld tword [g_r80_One  xWrtRIP]
        fld tword [g_r80_Zero xWrtRIP]
        fdiv    st0
        ret
ENDPROC   MyFpuPrepXcpt2


BEGINPROC MyFpuSave
%ifdef ASM_CALL64_MSC
        o64 fxsave  [rcx]
%elifdef ASM_CALL64_GCC
        o64 fxsave  [rdi]
%elifdef RT_ARCH_X86
        mov         ecx, [esp + 4]
        fxsave      [ecx]
%else
 %error "Unsupported architecture."
        bad arch
%endif
        ret
ENDPROC   MyFpuSave


BEGINPROC MyFpuStoreEnv
%ifdef ASM_CALL64_MSC
        fstenv  [rcx]
%elifdef ASM_CALL64_GCC
        fstenv  [rdi]
%elifdef RT_ARCH_X86
        mov     ecx, [esp + 4]
        fstenv  [ecx]
%else
 %error "Unsupported architecture."
        bad arch
%endif
        ret
ENDPROC   MyFpuStoreEnv


BEGINPROC MyFpuRestore
%ifdef ASM_CALL64_MSC
        o64 fxrstor [rcx]
%elifdef ASM_CALL64_GCC
        o64 fxrstor [rdi]
%elifdef RT_ARCH_X86
        mov         ecx, [esp + 4]
        fxrstor     [ecx]
%else
 %error "Unsupported architecture."
        bad arch
%endif
        ret
ENDPROC   MyFpuRestore


BEGINPROC MyFpuLoadEnv
%ifdef ASM_CALL64_MSC
        fldenv  [rcx]
%elifdef ASM_CALL64_GCC
        fldenv  [rdi]
%elifdef RT_ARCH_X86
        mov     ecx, [esp + 4]
        fldenv  [ecx]
%else
 %error "Unsupported architecture."
        bad arch
%endif
        ret
ENDPROC   MyFpuLoadEnv


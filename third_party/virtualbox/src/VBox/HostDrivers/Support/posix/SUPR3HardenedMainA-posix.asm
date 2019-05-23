; $Id: SUPR3HardenedMainA-posix.asm $
;; @file
; VirtualBox Support Library - Hardened main(), Posix assembly bits.
;

;
; Copyright (C) 2017 Oracle Corporation
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


;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"


;*********************************************************************************************************************************
;*  External Symbols                                                                                                             *
;*********************************************************************************************************************************
; External code.
BEGINCODE
extern NAME(supR3HardenedPosixMonitor_VerifyLibrary)

; External data
BEGINDATA
extern NAME(g_pfnDlopenReal)
%ifdef SUP_HARDENED_WITH_DLMOPEN
extern NAME(g_pfnDlmopenReal)
%endif



BEGINCODE

;;
; Wrapper for dlopen() handing the call over to the file verification code
; and resuming the call if we get a green light to load the library.
;
align 16
BEGINPROC supR3HardenedPosixMonitor_Dlopen
        push    xBP
        mov     xBP, xSP

%ifdef RT_ARCH_AMD64
        ; Save parameters on the stack
        push    rdi
        push    rsi
%else
        sub     esp, 4                  ; 16-byte stack alignment before call.
        push    dword [xBP + 08h]       ; first parameter.
%endif

        ;
        ; Call the verification method.
        ;
        call    NAME(supR3HardenedPosixMonitor_VerifyLibrary)

        ;
        ; Restore parameters for the next call and get the stack back to the
        ; original state.
        ;
%ifdef RT_ARCH_AMD64
        pop     rsi
        pop     rdi
%endif
        leave

        ; Check the result and resume the call if the result is positive,
        ; otherwise clean up and return NULL
        test    al, al
        je short .failed

        ; Resume the original dlopen call by jumping into the saved code.
        jmp     [NAME(g_pfnDlopenReal) xWrtRIP]

.failed:
        ;
        ; Don't use leave here as we didn't use the enter instruction. Just clear
        ; xAX and return
        ;
        xor     xAX, xAX
        ret
ENDPROC   supR3HardenedPosixMonitor_Dlopen


%ifdef SUP_HARDENED_WITH_DLMOPEN
;;
; Wrapper for dlmopen() handing the call over to the file verification code
; and resuming the call if we get a green light to load the library.
;
align 16
BEGINPROC supR3HardenedPosixMonitor_Dlmopen
        push    xBP
        mov     xBP, xSP

%ifdef RT_ARCH_AMD64
        sub     rsp, 8                  ; 16-byte stack alignment before call.

        ; Save parameters on the stack
        push    rdi
        push    rsi
        push    rdx

        mov     rdi, rsi                ; Move the second parameter to the front
%else
        sub     esp, 4                  ; 16-byte stack alignment before call.
        push    dword [xBP + 0ch]       ; Move the second parameter to the front
%endif

        ;
        ; Call the verification method.
        ;
        call    NAME(supR3HardenedPosixMonitor_VerifyLibrary)

        ;
        ; Restore parameters for the next call and get the stack back to the
        ; original state.
        ;
%ifdef RT_ARCH_AMD64
        pop     rdx
        pop     rsi
        pop     rdi
%endif
        leave

        ; Check the result and resume the call if the result is positive,
        ; otherwise clean up and return NULL
        test    al, al
        je short .failed

        ; Resume the original dlopen call by jumping into the saved code.
        jmp     [NAME(g_pfnDlmopenReal) xWrtRIP]

.failed:
        ;
        ; Don't use leave here as we didn't use the enter instruction. Just clear
        ; xAX and return
        ;
        xor     xAX, xAX
        ret
ENDPROC   supR3HardenedPosixMonitor_Dlmopen
%endif


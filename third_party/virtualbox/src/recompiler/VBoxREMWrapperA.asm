; $Id: VBoxREMWrapperA.asm $
;; @file
; VBoxREM Wrapper, Assembly routines and wrapper Templates.
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




;*******************************************************************************
;*  Header Files                                                               *
;*******************************************************************************
%include "iprt/asmdefs.mac"

%define REM_FIXUP_32_REAL_STUFF    0deadbeefh
%define REM_FIXUP_64_REAL_STUFF    0deadf00df00ddeadh
%define REM_FIXUP_64_DESC          0dead00010001deadh
%define REM_FIXUP_64_LOG_ENTRY     0dead00020002deadh
%define REM_FIXUP_64_LOG_EXIT      0dead00030003deadh
%define REM_FIXUP_64_WRAP_GCC_CB   0dead00040004deadh

;%define ENTRY_LOGGING   1
;%define EXIT_LOGGING    1


%ifdef RT_ARCH_AMD64
 ;;
 ; 64-bit pushad
 %macro MY_PUSHAQ 0
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
 %endmacro

 ;;
 ; 64-bit popad
 %macro MY_POPAQ 0
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
 %endmacro

 ;;
 ; Entry logging
 %ifdef ENTRY_LOGGING
  %macro LOG_ENTRY 0
    MY_PUSHAQ
    push    rbp
    mov     rbp, rsp
    and     rsp, ~0fh
    sub     rsp, 20h                    ; shadow space

   %ifdef RT_OS_WINDOWS
    mov     rcx, REM_FIXUP_64_DESC
   %else
    mov     rdi, REM_FIXUP_64_DESC
   %endif
    mov     rax, REM_FIXUP_64_LOG_ENTRY
    call    rax

    leave
    MY_POPAQ
  %endmacro
 %else
  %define LOG_ENTRY
 %endif

 ;;
 ; Exit logging
 %ifdef EXIT_LOGGING
  %macro LOG_EXIT 0
    MY_PUSHAQ
    push    rbp
    mov     rbp, rsp
    and     rsp, ~0fh
    sub     rsp, 20h                    ; shadow space

   %ifdef RT_OS_WINDOWS
    mov     rdx, rax
    mov     rcx, REM_FIXUP_64_DESC
   %else
    mov     rsi, eax
    mov     rdi, REM_FIXUP_64_DESC
   %endif
    mov     rax, REM_FIXUP_64_LOG_EXIT
    call    rax

    leave
    MY_POPAQ
  %endmacro
 %else
  %define LOG_EXIT
 %endif

%else
 %define LOG_ENTRY
 %define LOG_EXIT
%endif


BEGINCODE

%ifdef RT_OS_WINDOWS
 %ifdef RT_ARCH_AMD64


BEGINPROC WrapGCC2MSC0Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h

%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC0Int


BEGINPROC WrapGCC2MSC1Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h

    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC1Int


BEGINPROC WrapGCC2MSC2Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h

    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC2Int


BEGINPROC WrapGCC2MSC3Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h

    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC3Int


BEGINPROC WrapGCC2MSC4Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h

    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC4Int


BEGINPROC WrapGCC2MSC5Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 30h

    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC5Int


BEGINPROC WrapGCC2MSC6Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 30h

    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC6Int


BEGINPROC WrapGCC2MSC7Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 40h

    mov     r11, [rbp + 10h]
    mov     [rsp + 30h], r11
    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC7Int


BEGINPROC WrapGCC2MSC8Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 40h

    mov     r10, [rbp + 18h]
    mov     [rsp + 38h], r10
    mov     r11, [rbp + 10h]
    mov     [rsp + 30h], r11
    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC8Int


BEGINPROC WrapGCC2MSC9Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 50h

    mov     rax, [rbp + 20h]
    mov     [rsp + 40h], rax
    mov     r10, [rbp + 18h]
    mov     [rsp + 38h], r10
    mov     r11, [rbp + 10h]
    mov     [rsp + 30h], r11
    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC9Int


BEGINPROC WrapGCC2MSC10Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 50h

    mov     r11, [rbp + 28h]
    mov     [rsp + 48h], r11
    mov     rax, [rbp + 20h]
    mov     [rsp + 40h], rax
    mov     r10, [rbp + 18h]
    mov     [rsp + 38h], r10
    mov     r11, [rbp + 10h]
    mov     [rsp + 30h], r11
    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC10Int


BEGINPROC WrapGCC2MSC11Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 60h

    mov     r10, [rbp + 30h]
    mov     [rsp + 50h], r10
    mov     r11, [rbp + 28h]
    mov     [rsp + 48h], r11
    mov     rax, [rbp + 20h]
    mov     [rsp + 40h], rax
    mov     r10, [rbp + 18h]
    mov     [rsp + 38h], r10
    mov     r11, [rbp + 10h]
    mov     [rsp + 30h], r11
    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC11Int


BEGINPROC WrapGCC2MSC12Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 60h

    mov     rax, [rbp + 28h]
    mov     [rsp + 48h], rax
    mov     r10, [rbp + 30h]
    mov     [rsp + 50h], r10
    mov     r11, [rbp + 28h]
    mov     [rsp + 48h], r11
    mov     rax, [rbp + 20h]
    mov     [rsp + 40h], rax
    mov     r10, [rbp + 18h]
    mov     [rsp + 38h], r10
    mov     r11, [rbp + 10h]
    mov     [rsp + 30h], r11
    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     r8, rdx
    mov     rdx, rsi
    mov     rcx, rdi
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC12Int



BEGINPROC WrapGCC2MSCVariadictInt
    LOG_ENTRY
%ifdef DEBUG
    ; check that there are NO floting point arguments in XMM registers!
    or      rax, rax
    jz      .ok
    int3
.ok:
%endif
    sub     rsp, 28h
    mov     r11, [rsp + 28h]            ; r11 = return address.
    mov     [rsp + 28h], r9
    mov     [rsp + 20h], r8
    mov     r9, rcx
    mov     [rsp + 18h], r9             ; (*)
    mov     r8, rdx
    mov     [rsp + 14h], r8             ; (*)
    mov     rdx, rsi
    mov     [rsp + 8h], rdx             ; (*)
    mov     rcx, rdi
    mov     [rsp], rcx                  ; (*)
    mov     rsi, r11                    ; rsi is preserved by the callee.
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    add     rsp, 30h
    LOG_EXIT
    jmp     rsi
    ; (*) unconditionally spill the registers, just in case '...' implies weird stuff on MSC. Check this out!
ENDPROC WrapGCC2MSCVariadictInt


;;
; Custom template for SSMR3RegisterInternal.
;
; (This is based on the WrapGCC2MSC11Int template.)
;
; @cproto
;
; SSMR3DECL(int) SSMR3RegisterInternal(PVM pVM, const char *pszName, uint32_t u32Instance, uint32_t u32Version, size_t cbGuess,
;    PFNSSMINTLIVEPREP pfnLivePrep, PFNSSMINTLIVEEXEC pfnLiveExec, PFNSSMINTLIVEVOTE pfnLiveVote,
;    PFNSSMINTSAVEPREP pfnSavePrep, PFNSSMINTSAVEEXEC pfnSaveExec, PFNSSMINTSAVEDONE pfnSaveDone,
;    PFNSSMINTLOADPREP pfnLoadPrep, PFNSSMINTLOADEXEC pfnLoadExec, PFNSSMINTLOADDONE pfnLoadDone);
;
; @param    pVM             rdi              0
; @param    pszName         rsi              1
; @param    u32Instance     rdx              2
; @param    u32Version      rcx              3
; @param    cbGuess         r8               4
; @param    pfnLivePrep     r9               5
; @param    pfnLiveExec     rbp + 10h        6
; @param    pfnLiveVote     rbp + 18h        7
; @param    pfnSavePrep     rbp + 20h        8
; @param    pfnSaveExec     rbp + 28h        9
; @param    pfnSaveDone     rbp + 30h       10
; @param    pfnLoadPrep     rbp + 38h       11
; @param    pfnLoadExec     rbp + 40h       12
; @param    pfnLoadDone     rbp + 48h       13
;
BEGINPROC WrapGCC2MSC_SSMR3RegisterInternal
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp

    sub     rsp, 80h

    mov     r10, [rbp + 48h]
    mov     [rsp + 68h], r10            ; pfnLiveDone
    mov     r11, [rbp + 40h]
    mov     [rsp + 60h], r11            ; pfnLiveExec
    mov     rax, [rbp + 38h]
    mov     [rsp + 58h], rax            ; pfnLivePrep
    mov     r10, [rbp + 30h]
    mov     [rsp + 50h], r10            ; pfnLoadDone
    mov     r11, [rbp + 28h]
    mov     [rsp + 48h], r11            ; pfnLoadExec
    mov     rax, [rbp + 20h]
    mov     [rsp + 40h], rax            ; pfnLoadPrep
    mov     r10, [rbp + 18h]
    mov     [rsp + 38h], r10            ; pfnSaveDone
    mov     r11, [rbp + 10h]
    mov     [rsp + 30h], r11            ; pfnSaveExec
    mov     [rsp + 28h], r9             ; pfnSavePrep
    mov     [rsp + 20h], r8
    mov     [rsp + 18h], rcx            ; -> r9
    mov     [rsp + 10h], rdx            ; -> r8
    mov     [rsp + 08h], rsi            ; -> rdx
    mov     [rsp], rdi                  ; -> rcx

    ; Now convert the function pointers. Have to setup a new shadow
    ; space here since the SSMR3RegisterInternal one is already in use.
    sub     rsp, 20h

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 28h + 20h]      ; pValue
    mov     r8d, 5                      ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 30h + 20h]      ; pValue
    mov     r8d, 6                      ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 38h + 20h]      ; pValue
    mov     r8d, 7                      ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 40h + 20h]      ; pValue
    mov     r8d, 8                      ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 48h + 20h]      ; pValue
    mov     r8d, 9                      ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 50h + 20h]      ; pValue
    mov     r8d, 10                     ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 58h + 20h]      ; pValue
    mov     r8d, 11                     ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 60h + 20h]      ; pValue
    mov     r8d, 12                     ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    mov     rcx, REM_FIXUP_64_DESC      ; pDesc
    lea     rdx, [rsp + 68h + 20h]      ; pValue
    mov     r8d, 13                     ; iParam
    mov     rax, REM_FIXUP_64_WRAP_GCC_CB
    call    rax

    add     rsp, 20h

    ; finally do the call.
    mov     r9,  [rsp + 18h]
    mov     r8,  [rsp + 10h]
    mov     rdx, [rsp + 08h]
    mov     rcx, [rsp]
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    leave
    LOG_EXIT
    ret
ENDPROC WrapGCC2MSC_SSMR3RegisterInternal


;
; The other way around:
;


BEGINPROC WrapMSC2GCC0Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 10h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC0Int


BEGINPROC WrapMSC2GCC1Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC1Int


BEGINPROC WrapMSC2GCC2Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
%ifdef USE_DIRECT_CALLS
    call    $+5+REM_FIXUP_32_REAL_STUFF
%else
    mov     rax, REM_FIXUP_64_REAL_STUFF
    call    rax
%endif

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC2Int


BEGINPROC WrapMSC2GCC3Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    call    $+5+REM_FIXUP_32_REAL_STUFF

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC3Int


BEGINPROC WrapMSC2GCC4Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    mov     rcx, r9
    call    $+5+REM_FIXUP_32_REAL_STUFF

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC4Int


BEGINPROC WrapMSC2GCC5Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    mov     rcx, r9
    mov     r8, [rbp + 30h]
    call    $+5+REM_FIXUP_32_REAL_STUFF

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC5Int


BEGINPROC WrapMSC2GCC6Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 20h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    mov     rcx, r9
    mov     r8, [rbp + 30h]
    mov     r9, [rbp + 38h]
    call    $+5+REM_FIXUP_32_REAL_STUFF

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC6Int


BEGINPROC WrapMSC2GCC7Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 30h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    mov     rcx, r9
    mov     r8, [rbp + 30h]
    mov     r9, [rbp + 38h]
    mov     r10, [rbp + 40h]
    mov     [rsp], r10
    call    $+5+REM_FIXUP_32_REAL_STUFF

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC7Int


BEGINPROC WrapMSC2GCC8Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 30h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    mov     rcx, r9
    mov     r8, [rbp + 30h]
    mov     r9, [rbp + 38h]
    mov     r10, [rbp + 40h]
    mov     [rsp], r10
    mov     r11, [rbp + 48h]
    mov     [rsp + 8], r11
    call    $+5+REM_FIXUP_32_REAL_STUFF

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC8Int


BEGINPROC WrapMSC2GCC9Int
    LOG_ENTRY
    push    rbp
    mov     rbp, rsp
    sub     rsp, 40h
    mov     [rbp - 10h], rsi
    mov     [rbp - 18h], rdi

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    mov     rcx, r9
    mov     r8, [rbp + 30h]
    mov     r9, [rbp + 38h]
    mov     r10, [rbp + 40h]
    mov     [rsp], r10
    mov     r11, [rbp + 48h]
    mov     [rsp + 8], r11
    mov     rax, [rbp + 50h]
    mov     [rsp + 10h], rax
    call    $+5+REM_FIXUP_32_REAL_STUFF

    mov     rdi, [rbp - 18h]
    mov     rsi, [rbp - 10h]
    leave
    LOG_EXIT
    ret
ENDPROC WrapMSC2GCC9Int

 %endif ; RT_ARCH_AMD64
%endif ; RT_OS_WINDOWS


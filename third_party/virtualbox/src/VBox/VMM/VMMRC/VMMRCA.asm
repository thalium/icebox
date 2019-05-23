; $Id: VMMRCA.asm $
;; @file
; VMMRC - Raw-mode Context Virtual Machine Monitor assembly routines.
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
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "iprt/x86.mac"
%include "VBox/sup.mac"
%include "VBox/vmm/vm.mac"
%include "VMMInternal.mac"
%include "VMMRC.mac"


;*******************************************************************************
;*   Defined Constants And Macros                                              *
;*******************************************************************************
;; save all registers before loading special values for the faulting.
%macro SaveAndLoadAll 0
    pushad
    push    ds
    push    es
    push    fs
    push    gs
    call    NAME(vmmGCTestLoadRegs)
%endmacro

;; restore all registers after faulting.
%macro RestoreAll 0
    pop     gs
    pop     fs
    pop     es
    pop     ds
    popad
%endmacro


;*******************************************************************************
;* External Symbols                                                            *
;*******************************************************************************
extern IMPNAME(g_VM)
extern IMPNAME(g_Logger)
extern IMPNAME(g_RelLogger)
extern NAME(RTLogLogger)
extern NAME(vmmRCProbeFireHelper)
extern NAME(TRPMRCTrapHyperHandlerSetEIP)


BEGINCODE

;/**
; * Internal GC logger worker: Logger wrapper.
; */
;VMMRCDECL(void) vmmGCLoggerWrapper(const char *pszFormat, ...);
EXPORTEDNAME vmmGCLoggerWrapper
%ifdef __YASM__
%ifdef ASM_FORMAT_ELF
    push    dword IMP(g_Logger)         ; YASM BUG #67! YASMCHECK!
%else
    push    IMP(g_Logger)
%endif
%else
    push    IMP(g_Logger)
%endif
    call    NAME(RTLogLogger)
    add     esp, byte 4
    ret
ENDPROC vmmGCLoggerWrapper


;/**
; * Internal GC logger worker: Logger (release) wrapper.
; */
;VMMRCDECL(void) vmmGCRelLoggerWrapper(const char *pszFormat, ...);
EXPORTEDNAME vmmGCRelLoggerWrapper
%ifdef __YASM__
%ifdef ASM_FORMAT_ELF
    push    dword IMP(g_RelLogger)         ; YASM BUG #67! YASMCHECK!
%else
    push    IMP(g_RelLogger)
%endif
%else
    push    IMP(g_RelLogger)
%endif
    call    NAME(RTLogLogger)
    add     esp, byte 4
    ret
ENDPROC vmmGCRelLoggerWrapper


;;
; Enables write protection.
BEGINPROC vmmGCEnableWP
    push    eax
    mov     eax, cr0
    or      eax, X86_CR0_WRITE_PROTECT
    mov     cr0, eax
    pop     eax
    ret
ENDPROC vmmGCEnableWP


;;
; Disables write protection.
BEGINPROC vmmGCDisableWP
    push    eax
    mov     eax, cr0
    and     eax, ~X86_CR0_WRITE_PROTECT
    mov     cr0, eax
    pop     eax
    ret
ENDPROC vmmGCDisableWP


;;
; Load special register set expected upon faults.
; All registers are changed.
BEGINPROC vmmGCTestLoadRegs
    mov     eax, ss
    mov     ds, eax
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     edi, 001234567h
    mov     esi, 042000042h
    mov     ebp, 0ffeeddcch
    mov     ebx, 089abcdefh
    mov     ecx, 0ffffaaaah
    mov     edx, 077778888h
    mov     eax, 0f0f0f0f0h
    ret
ENDPROC vmmGCTestLoadRegs


;;
; A Trap 3 testcase.
GLOBALNAME vmmGCTestTrap3
    SaveAndLoadAll

    int     3
EXPORTEDNAME vmmGCTestTrap3_FaultEIP

    RestoreAll
    mov     eax, 0ffffffffh
    ret
ENDPROC vmmGCTestTrap3


;;
; A Trap 8 testcase.
GLOBALNAME vmmGCTestTrap8
    SaveAndLoadAll

    sub     esp, byte 8
    sidt    [esp]
    mov     word [esp], 111 ; make any #PF double fault.
    lidt    [esp]
    add     esp, byte 8

    COM_S_CHAR '!'

    xor     eax, eax
EXPORTEDNAME vmmGCTestTrap8_FaultEIP
    mov     eax, [eax]


    COM_S_CHAR '2'

    RestoreAll
    mov     eax, 0ffffffffh
    ret
ENDPROC vmmGCTestTrap8


;;
; A simple Trap 0d testcase.
GLOBALNAME vmmGCTestTrap0d
    SaveAndLoadAll

    push    ds
EXPORTEDNAME vmmGCTestTrap0d_FaultEIP
    ltr     [esp]
    pop     eax

    RestoreAll
    mov     eax, 0ffffffffh
    ret
ENDPROC vmmGCTestTrap0d


;;
; A simple Trap 0e testcase.
GLOBALNAME vmmGCTestTrap0e
    SaveAndLoadAll

    xor     eax, eax
EXPORTEDNAME vmmGCTestTrap0e_FaultEIP
    mov     eax, [eax]

    RestoreAll
    mov     eax, 0ffffffffh
    ret

EXPORTEDNAME vmmGCTestTrap0e_ResumeEIP
    RestoreAll
    xor     eax, eax
    ret
ENDPROC vmmGCTestTrap0e



;;
; Safely reads an MSR.
; @returns  boolean
; @param    uMsr        The MSR to red.
; @param    pu64Value   Where to return the value on success.
;
GLOBALNAME vmmRCSafeMsrRead
    push    ebp
    mov     ebp, esp
    pushf
    cli
    push    esi
    push    edi
    push    ebx
    push    ebp

    mov     ecx, [ebp + 8]              ; The MSR to read.
    mov     eax, 0deadbeefh
    mov     edx, 0deadbeefh

TRPM_GP_HANDLER NAME(TRPMRCTrapHyperHandlerSetEIP), .trapped
    rdmsr

    mov     ecx, [ebp + 0ch]            ; Where to store the result.
    mov     [ecx], eax
    mov     [ecx + 4], edx

    mov     eax, 1
.return:
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    popf
    leave
    ret

.trapped:
    mov     eax, 0
    jmp     .return
ENDPROC vmmRCSafeMsrRead


;;
; Safely writes an MSR.
; @returns  boolean
; @param    uMsr        The MSR to red.
; @param    u64Value    The value to write.
;
GLOBALNAME vmmRCSafeMsrWrite
    push    ebp
    mov     ebp, esp
    pushf
    cli
    push    esi
    push    edi
    push    ebx
    push    ebp

    mov     ecx, [ebp + 8]              ; The MSR to write to.
    mov     eax, [ebp + 12]             ; The value to write.
    mov     edx, [ebp + 16]

TRPM_GP_HANDLER NAME(TRPMRCTrapHyperHandlerSetEIP), .trapped
    wrmsr

    mov     eax, 1
.return:
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    popf
    leave
    ret

.trapped:
    mov     eax, 0
    jmp     .return
ENDPROC vmmRCSafeMsrWrite



;;
; The raw-mode context equivalent of SUPTracerFireProbe.
;
; See also SUPLibTracerA.asm.
;
EXPORTEDNAME VMMRCProbeFire
        push    ebp
        mov     ebp, esp

        ;
        ; Save edx and eflags so we can use them.
        ;
        pushf
        push    edx

        ;
        ; Get the address of the tracer context record after first checking
        ; that host calls hasn't been disabled.
        ;
        mov     edx, IMP(g_VM)
        add     edx, [edx + VM.offVMCPU]
        cmp     dword [edx + VMCPU.vmm + VMMCPU.cCallRing3Disabled], 0
        jnz     .return
        add     edx, VMCPU.vmm + VMMCPU.TracerCtx

        ;
        ; Save the X86 context.
        ;
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.eax], eax
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.ecx], ecx
        pop     eax
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.edx], eax
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.ebx], ebx
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.esi], esi
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.edi], edi
        pop     eax
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.eflags], eax
        mov     eax, [ebp + 4]
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.eip], eax
        mov     eax, [ebp]
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.ebp], eax
        lea     eax, [ebp + 4*2]
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.esp], eax

        mov     ecx, [ebp + 4*2]
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.uVtgProbeLoc], ecx

        mov     eax, [ecx + 4]          ; VTGPROBELOC::idProbe.
        mov     [edx + SUPDRVTRACERUSRCTX32.idProbe], eax
        mov     dword [edx + SUPDRVTRACERUSRCTX32.cBits], 32

        ; Copy the arguments off the stack.
%macro COPY_ONE_ARG 1
        mov     eax, [ebp + 12 + %1 * 4]
        mov     [edx + SUPDRVTRACERUSRCTX32.u.X86.aArgs + %1*4], eax
%endmacro
        COPY_ONE_ARG 0
        COPY_ONE_ARG 1
        COPY_ONE_ARG 2
        COPY_ONE_ARG 3
        COPY_ONE_ARG 4
        COPY_ONE_ARG 5
        COPY_ONE_ARG 6
        COPY_ONE_ARG 7
        COPY_ONE_ARG 8
        COPY_ONE_ARG 9
        COPY_ONE_ARG 10
        COPY_ONE_ARG 11
        COPY_ONE_ARG 12
        COPY_ONE_ARG 13
        COPY_ONE_ARG 14
        COPY_ONE_ARG 15
        COPY_ONE_ARG 16
        COPY_ONE_ARG 17
        COPY_ONE_ARG 18
        COPY_ONE_ARG 19

        ;
        ; Call the helper (too lazy to do the VMM structure stuff).
        ;
        mov     ecx, IMP(g_VM)
        push    ecx
        call    NAME(vmmRCProbeFireHelper)

.return:
        leave
        ret
ENDPROC      VMMRCProbeFire


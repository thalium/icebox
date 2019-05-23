; $Id: TRPMR0A.asm $
;; @file
; TRPM - Host Context Ring-0
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


BEGINCODE

;;
; Calls the interrupt gate as if we received an interrupt while in Ring-0.
;
; @param   uIP     x86:[ebp+8]   msc:rcx  gcc:rdi  The interrupt gate IP.
; @param   SelCS   x86:[ebp+12]  msc:dx   gcc:si   The interrupt gate CS.
; @param   RSP                   msc:r8   gcc:rdx  The interrupt gate RSP. ~0 if no stack switch should take place. (only AMD64)
;DECLASM(void) trpmR0DispatchHostInterrupt(RTR0UINTPTR uIP, RTSEL SelCS, RTR0UINTPTR RSP);
ALIGNCODE(16)
BEGINPROC trpmR0DispatchHostInterrupt
    push    xBP
    mov     xBP, xSP

%ifdef RT_ARCH_AMD64
    mov     r11, rsp                    ; save the RSP for the iret frame.
    and     rsp, 0fffffffffffffff0h     ; align the stack. (do it unconditionally saves some jump mess)

    ; switch stack?
 %ifdef ASM_CALL64_MSC
    cmp     r8,  0ffffffffffffffffh
    je      .no_stack_switch
    mov     rsp, r8
 %else
    cmp     rdx, 0ffffffffffffffffh
    je      .no_stack_switch
    mov     rsp, rdx
 %endif
.no_stack_switch:

    ; create the iret frame
    push    0                           ; SS
    push    r11                         ; RSP
    pushfq                              ; RFLAGS
    and     dword [rsp], ~X86_EFL_IF
    mov     ax, cs
    push    rax                         ; CS
    lea     r10, [.return wrt rip]      ; RIP
    push    r10

    ; create the retf frame
 %ifdef ASM_CALL64_MSC
    movzx   rdx, dx
    cmp     rdx, r11
    je      .dir_jump
    push    rdx
    push    rcx
 %else
    movzx   rsi, si
    cmp     rsi, r11
    je      .dir_jump
    push    rsi
    push    rdi
 %endif

    ; dispatch it
    db 048h
    retf

    ; dispatch it by a jmp (don't mess up the IST stack)
.dir_jump:
 %ifdef ASM_CALL64_MSC
    jmp     rcx
 %else
    jmp     rdi
 %endif

%else ; 32-bit:
    mov     ecx, [ebp + 8]              ; uIP
    movzx   edx, word [ebp + 12]        ; SelCS

    ; create the iret frame
    pushfd                              ; EFLAGS
    and     dword [esp], ~X86_EFL_IF
    push    cs                          ; CS
    push    .return                     ; EIP

    ; create the retf frame
    push    edx
    push    ecx

    ; dispatch it!
    retf
%endif
.return:
    cli

    leave
    ret
ENDPROC trpmR0DispatchHostInterrupt


;;
; Issues a software interrupt to the specified interrupt vector.
;
; @param   uActiveVector   x86:[esp+4]   msc:rcx  gcc:rdi   The vector number.
;
;DECLASM(void) trpmR0DispatchHostInterruptSimple(RTUINT uActiveVector);
ALIGNCODE(16)
BEGINPROC trpmR0DispatchHostInterruptSimple
%ifdef RT_ARCH_X86
    mov     eax, [esp + 4]
    jmp     dword [.jmp_table + eax * 4]
%else
    lea     r9, [.jmp_table wrt rip]
 %ifdef ASM_CALL64_MSC
    jmp     qword [r9 + rcx * 8]
 %else
    jmp     qword [r9 + rdi * 8]
 %endif
%endif

ALIGNCODE(4)
.jmp_table:
%assign i 0
%rep 256
RTCCPTR_DEF .int_ %+ i
%assign i i+1
%endrep

%assign i 0
%rep 256
    ALIGNCODE(4)
.int_ %+ i:
    int i
    ret
%assign i i+1
%endrep

ENDPROC trpmR0DispatchHostInterruptSimple


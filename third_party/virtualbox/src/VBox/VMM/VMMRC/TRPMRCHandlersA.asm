; $Id: TRPMRCHandlersA.asm $
;; @file
; TRPM - Raw-mode Context Trap Handlers
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
%include "VMMRC.mac"
%include "iprt/x86.mac"
%include "VBox/vmm/cpum.mac"
%include "VBox/vmm/stam.mac"
%include "VBox/vmm/vm.mac"
%include "TRPMInternal.mac"
%include "VBox/err.mac"
%include "VBox/vmm/trpm.mac"


;*******************************************************************************
;* External Symbols                                                            *
;*******************************************************************************
extern IMPNAME(g_TRPM)                  ; These IMPNAME(g_*) symbols resolve to the import table
extern IMPNAME(g_TRPMCPU)               ; where there is a pointer to the real symbol. PE imports
extern IMPNAME(g_VM)                    ; are a bit confusing at first... :-)
extern IMPNAME(g_trpmGuestCtxCore)
extern IMPNAME(g_trpmHyperCtxCore)
extern NAME(trpmRCTrapInGeneric)
extern NAME(TRPMGCTrap01Handler)
extern NAME(TRPMGCHyperTrap01Handler)
%ifdef VBOX_WITH_NMI
extern NAME(TRPMGCTrap02Handler)
extern NAME(TRPMGCHyperTrap02Handler)
%endif
extern NAME(TRPMGCTrap03Handler)
extern NAME(TRPMGCHyperTrap03Handler)
extern NAME(TRPMGCTrap06Handler)
extern NAME(TRPMGCTrap07Handler)
extern NAME(TRPMGCTrap0bHandler)
extern NAME(TRPMGCHyperTrap0bHandler)
extern NAME(TRPMGCTrap0dHandler)
extern NAME(TRPMGCHyperTrap0dHandler)
extern NAME(TRPMGCTrap0eHandler)
extern NAME(TRPMGCHyperTrap0eHandler)
extern NAME(CPUMRCAssertPreExecutionSanity)


;*******************************************************************************
;*  Defined Constants And Macros                                               *
;*******************************************************************************
;; Some conditional COM port debugging.
;%define DEBUG_STUFF 1
;%define DEBUG_STUFF_TRPG 1
;%define DEBUG_STUFF_INT 1


BEGINCODE

;;
; Jump table for trap handlers for hypervisor traps.
;
g_apfnStaticTrapHandlersHyper:
                                        ; N  - M M -  T  - C - D i
                                        ; o  - n o -  y  - o - e p
                                        ;    - e n -  p  - d - s t
                                        ;    -   i -  e  - e - c .
                                        ;    -   c -     -   - r
                                        ; =============================================================
    dd 0                                ;  0 - #DE - F   - N - Divide error
    dd NAME(TRPMGCHyperTrap01Handler)   ;  1 - #DB - F/T - N - Single step, INT 1 instruction
%ifdef VBOX_WITH_NMI
    dd NAME(TRPMGCHyperTrap02Handler)   ;  2 -     - I   - N - Non-Maskable Interrupt (NMI)
%else
    dd 0                                ;  2 -     - I   - N - Non-Maskable Interrupt (NMI)
%endif
    dd NAME(TRPMGCHyperTrap03Handler)   ;  3 - #BP - T   - N - Breakpoint, INT 3 instruction.
    dd 0                                ;  4 - #OF - T   - N - Overflow, INTO instruction.
    dd 0                                ;  5 - #BR - F   - N - BOUND Range Exceeded, BOUND instruction.
    dd 0                                ;  6 - #UD - F   - N - Undefined(/Invalid) Opcode.
    dd 0                                ;  7 - #NM - F   - N - Device not available, FP or (F)WAIT instruction.
    dd 0                                ;  8 - #DF - A   - 0 - Double fault.
    dd 0                                ;  9 -     - F   - N - Coprocessor Segment Overrun (obsolete).
    dd 0                                ;  a - #TS - F   - Y - Invalid TSS, Taskswitch or TSS access.
    dd NAME(TRPMGCHyperTrap0bHandler)   ;  b - #NP - F   - Y - Segment not present.
    dd 0                                ;  c - #SS - F   - Y - Stack-Segment fault.
    dd NAME(TRPMGCHyperTrap0dHandler)   ;  d - #GP - F   - Y - General protection fault.
    dd NAME(TRPMGCHyperTrap0eHandler)   ;  e - #PF - F   - Y - Page fault.
    dd 0                                ;  f -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 10 - #MF - F   - N - x86 FPU Floating-Point Error (Math fault), FP or (F)WAIT instruction.
    dd 0                                ; 11 - #AC - F   - 0 - Alignment Check.
    dd 0                                ; 12 - #MC - A   - N - Machine Check.
    dd 0                                ; 13 - #XF - F   - N - SIMD Floating-Point Exception.
    dd 0                                ; 14 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 15 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 16 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 17 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 18 -     -     -   - Intel Reserved. Do not use.


;;
; Jump table for trap handlers for guest traps
;
g_apfnStaticTrapHandlersGuest:
                                        ; N  - M M -  T  - C - D i
                                        ; o  - n o -  y  - o - e p
                                        ;    - e n -  p  - d - s t
                                        ;    -   i -  e  - e - c .
                                        ;    -   c -     -   - r
                                        ; =============================================================
    dd 0                                ;  0 - #DE - F   - N - Divide error
    dd NAME(TRPMGCTrap01Handler)        ;  1 - #DB - F/T - N - Single step, INT 1 instruction
%ifdef VBOX_WITH_NMI
    dd NAME(TRPMGCTrap02Handler)        ;  2 -     - I   - N - Non-Maskable Interrupt (NMI)
%else
    dd 0                                ;  2 -     - I   - N - Non-Maskable Interrupt (NMI)
%endif
    dd NAME(TRPMGCTrap03Handler)        ;  3 - #BP - T   - N - Breakpoint, INT 3 instruction.
    dd 0                                ;  4 - #OF - T   - N - Overflow, INTO instruction.
    dd 0                                ;  5 - #BR - F   - N - BOUND Range Exceeded, BOUND instruction.
    dd NAME(TRPMGCTrap06Handler)        ;  6 - #UD - F   - N - Undefined(/Invalid) Opcode.
    dd NAME(TRPMGCTrap07Handler)        ;  7 - #NM - F   - N - Device not available, FP or (F)WAIT instruction.
    dd 0                                ;  8 - #DF - A   - 0 - Double fault.
    dd 0                                ;  9 -     - F   - N - Coprocessor Segment Overrun (obsolete).
    dd 0                                ;  a - #TS - F   - Y - Invalid TSS, Taskswitch or TSS access.
    dd NAME(TRPMGCTrap0bHandler)        ;  b - #NP - F   - Y - Segment not present.
    dd 0                                ;  c - #SS - F   - Y - Stack-Segment fault.
    dd NAME(TRPMGCTrap0dHandler)        ;  d - #GP - F   - Y - General protection fault.
    dd NAME(TRPMGCTrap0eHandler)        ;  e - #PF - F   - Y - Page fault.
    dd 0                                ;  f -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 10 - #MF - F   - N - x86 FPU Floating-Point Error (Math fault), FP or (F)WAIT instruction.
    dd 0                                ; 11 - #AC - F   - 0 - Alignment Check.
    dd 0                                ; 12 - #MC - A   - N - Machine Check.
    dd 0                                ; 13 - #XF - F   - N - SIMD Floating-Point Exception.
    dd 0                                ; 14 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 15 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 16 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 17 -     -     -   - Intel Reserved. Do not use.
    dd 0                                ; 18 -     -     -   - Intel Reserved. Do not use.



;;
; We start by 24 push <vector no.> + jmp <generic entry point>
;
ALIGNCODE(16)
BEGINPROC_EXPORTED TRPMGCHandlerGeneric
%macro TRPMGenericEntry 1
    db 06ah, i                          ; push imm8 - note that this is a signextended value.
    jmp   %1
    ALIGNCODE(8)
%assign i i+1
%endmacro

%assign i 0                             ; start counter.
    TRPMGenericEntry GenericTrap        ; 0
    TRPMGenericEntry GenericTrap        ; 1
    TRPMGenericEntry GenericTrap        ; 2
    TRPMGenericEntry GenericTrap        ; 3
    TRPMGenericEntry GenericTrap        ; 4
    TRPMGenericEntry GenericTrap        ; 5
    TRPMGenericEntry GenericTrap        ; 6
    TRPMGenericEntry GenericTrap        ; 7
    TRPMGenericEntry GenericTrapErrCode ; 8
    TRPMGenericEntry GenericTrap        ; 9
    TRPMGenericEntry GenericTrapErrCode ; a
    TRPMGenericEntry GenericTrapErrCode ; b
    TRPMGenericEntry GenericTrapErrCode ; c
    TRPMGenericEntry GenericTrapErrCode ; d
    TRPMGenericEntry GenericTrapErrCode ; e
    TRPMGenericEntry GenericTrap        ; f  (reserved)
    TRPMGenericEntry GenericTrap        ; 10
    TRPMGenericEntry GenericTrapErrCode ; 11
    TRPMGenericEntry GenericTrap        ; 12
    TRPMGenericEntry GenericTrap        ; 13
    TRPMGenericEntry GenericTrap        ; 14 (reserved)
    TRPMGenericEntry GenericTrap        ; 15 (reserved)
    TRPMGenericEntry GenericTrap        ; 16 (reserved)
    TRPMGenericEntry GenericTrap        ; 17 (reserved)
%undef i
%undef TRPMGenericEntry

;;
; Main exception handler for the guest context
;
; Stack:
;           14  SS
;           10  ESP
;            c  EFLAGS
;            8  CS
;            4  EIP
;            0  vector number
;
; @uses     none
;
ALIGNCODE(8)
GenericTrap:
    ;
    ; for the present we fake an error code ~0
    ;
    push    eax
    mov     eax, 0ffffffffh
    xchg    [esp + 4], eax              ; get vector number, set error code
    xchg    [esp], eax                  ; get saved eax, set vector number
    jmp short GenericTrapErrCode


;;
; Main exception handler for the guest context with error code
;
; Stack:
;           28  GS          (V86 only)
;           24  FS          (V86 only)
;           20  DS          (V86 only)
;           1C  ES          (V86 only)
;           18  SS          (only if ring transition.)
;           14  ESP         (only if ring transition.)
;           10  EFLAGS
;            c  CS
;            8  EIP
;            4  Error code. (~0 for vectors which don't take an error code.)
;            0  vector number
;
; Error code:
;
; 31          16    15      3       2           1           0
;
; reserved          segment         TI          IDT         EXT
;                   selector        GDT/LDT     (1) IDT     External interrupt
;                   index           (IDT=0)     index
;
; NOTE: Page faults (trap 14) have a different error code
;
; @uses     none
;
ALIGNCODE(8)
GenericTrapErrCode:
    cld

    ;
    ; Save ds, es, fs, gs, eax and ebx so we have a context pointer (ebx) and
    ; scratch (eax) register to work with.  A sideeffect of using ebx is that
    ; it's preserved accross cdecl calls.
    ;
    ; In order to safely access data, we need to load our flat DS & ES selector,
    ; clear FS and GS (stale guest selector prevention), and clear make sure
    ; that CR0.WP is cleared.
    ;
    push    ds                          ; +14h
    push    es                          ; +10h
    push    fs                          ; +0ch
    push    gs                          ; +08h
    push    eax                         ; +04h
    push    ebx                         ; +00h
%push StackFrame
%define %$STK_SAVED_EBX   esp
%define %$STK_SAVED_EAX   esp + 04h
%define %$STK_SAVED_GS    esp + 08h
%define %$STK_SAVED_FS    esp + 0ch
%define %$STK_SAVED_ES    esp + 10h
%define %$STK_SAVED_DS    esp + 14h
%define %$ESPOFF                18h
%define %$STK_VECTOR      esp + 00h + %$ESPOFF
%define %$STK_ERRCD       esp + 04h + %$ESPOFF
%define %$STK_EIP         esp + 08h + %$ESPOFF
%define %$STK_CS          esp + 0ch + %$ESPOFF
%define %$STK_EFLAGS      esp + 10h + %$ESPOFF
%define %$STK_ESP         esp + 14h + %$ESPOFF
%define %$STK_SS          esp + 18h + %$ESPOFF
%define %$STK_V86_ES      esp + 1ch + %$ESPOFF
%define %$STK_V86_DS      esp + 20h + %$ESPOFF
%define %$STK_V86_FS      esp + 24h + %$ESPOFF
%define %$STK_V86_GS      esp + 28h + %$ESPOFF

    mov     bx, ss                      ; load
    mov     ds, bx
    mov     es, bx

    xor     bx, bx                      ; load 0 into gs and fs.
    mov     gs, bx
    mov     fs, bx

    mov     eax, cr0                        ;; @todo elimitate this read?
    and     eax, ~X86_CR0_WRITE_PROTECT
    mov     cr0, eax

    mov     ebx, IMP(g_trpmGuestCtxCore)    ; Assume GC as the most common.
    test    byte [%$STK_CS], 3h             ; check RPL of the cs selector
    jnz     .save_guest_state
    test    dword [%$STK_EFLAGS], X86_EFL_VM; If in V86, then guest.
    jnz     .save_guest_state
    mov     ebx, IMP(g_trpmHyperCtxCore)    ; It's raw-mode context, actually.

    ;
    ; Save the state.
    ;
.save_hyper_state:
    mov     [ebx + CPUMCTXCORE.ecx], ecx
    lea     eax, [%$STK_ESP]
    mov     [ebx + CPUMCTXCORE.esp], eax
    mov     cx, ss
    mov     [ebx + CPUMCTXCORE.ss.Sel], cx
    jmp     .save_state_common

.save_guest_state:
    mov     [ebx + CPUMCTXCORE.ecx], ecx
    mov     eax, [%$STK_ESP]
    mov     [ebx + CPUMCTXCORE.esp], eax
    mov     cx, [%$STK_SS]
    mov     [ebx + CPUMCTXCORE.ss.Sel], cx

.save_state_common:
    mov     eax, [%$STK_SAVED_EAX]
    mov     [ebx + CPUMCTXCORE.eax], eax
    mov     [ebx + CPUMCTXCORE.edx], edx
    mov     eax, [%$STK_SAVED_EBX]
    mov     [ebx + CPUMCTXCORE.ebx], eax
    mov     [ebx + CPUMCTXCORE.esi], esi
    mov     [ebx + CPUMCTXCORE.edi], edi
    mov     [ebx + CPUMCTXCORE.ebp], ebp

    mov     cx, [%$STK_CS]
    mov     [ebx + CPUMCTXCORE.cs.Sel], cx
    mov     eax, [%$STK_EIP]
    mov     [ebx + CPUMCTXCORE.eip], eax
    mov     eax, [%$STK_EFLAGS]
    mov     [ebx + CPUMCTXCORE.eflags], eax

%if GC_ARCH_BITS == 64 ; zero out the high dwords - probably not necessary any more.
    mov     dword [ebx + CPUMCTXCORE.eax + 4], 0
    mov     dword [ebx + CPUMCTXCORE.ecx + 4], 0
    mov     dword [ebx + CPUMCTXCORE.edx + 4], 0
    mov     dword [ebx + CPUMCTXCORE.ebx + 4], 0
    mov     dword [ebx + CPUMCTXCORE.esi + 4], 0
    mov     dword [ebx + CPUMCTXCORE.edi + 4], 0
    mov     dword [ebx + CPUMCTXCORE.ebp + 4], 0
    mov     dword [ebx + CPUMCTXCORE.esp + 4], 0
    mov     dword [ebx + CPUMCTXCORE.eip + 4], 0
%endif

    test    dword [%$STK_EFLAGS], X86_EFL_VM
    jnz     .save_V86_segregs

    mov     cx, [%$STK_SAVED_ES]
    mov     [ebx + CPUMCTXCORE.es.Sel], cx
    mov     cx, [%$STK_SAVED_DS]
    mov     [ebx + CPUMCTXCORE.ds.Sel], cx
    mov     cx, [%$STK_SAVED_FS]
    mov     [ebx + CPUMCTXCORE.fs.Sel], cx
    mov     cx, [%$STK_SAVED_GS]
    mov     [ebx + CPUMCTXCORE.gs.Sel], cx
    jmp     .done_saving

    ;
    ; The DS, ES, FS and GS registers are zeroed in V86 mode and their real
    ; values are on the stack.
    ;
.save_V86_segregs:
    mov     cx, [%$STK_V86_ES]
    mov     [ebx + CPUMCTXCORE.es.Sel], cx
    mov     cx, [%$STK_V86_DS]
    mov     [ebx + CPUMCTXCORE.ds.Sel], cx
    mov     cx, [%$STK_V86_FS]
    mov     [ebx + CPUMCTXCORE.fs.Sel], cx
    mov     cx, [%$STK_V86_GS]
    mov     [ebx + CPUMCTXCORE.gs.Sel], cx

.done_saving:

%ifdef VBOX_WITH_STATISTICS
    ;
    ; Start profiling.
    ;
    mov     edx, [%$STK_VECTOR]
    imul    edx, edx, byte STAMPROFILEADV_size ; assumes < 128.
    add     edx, TRPM.aStatGCTraps
    add     edx, IMP(g_TRPM)
    STAM_PROFILE_ADV_START edx
%endif

    ;
    ; Store the information about the active trap/interrupt.
    ;
    mov     esi, IMP(g_TRPMCPU)         ; esi = TRPMCPU until resume!
    movzx   edx, byte [%$STK_VECTOR]
    mov     [esi + TRPMCPU.uActiveVector], edx
    mov     edx, [%$STK_ERRCD]
    mov     [esi + TRPMCPU.uActiveErrorCode], edx
    mov     dword [esi + TRPMCPU.enmActiveType], TRPM_TRAP
    mov     edx, cr2                       ;; @todo Check how expensive cr2 reads are!
    mov     dword [esi + TRPMCPU.uActiveCR2], edx
%if GC_ARCH_BITS == 64 ; zero out the high dwords.
    mov     dword [esi + TRPMCPU.uActiveErrorCode + 4], 0
    mov     dword [esi + TRPMCPU.uActiveCR2 + 4], 0
%endif

    ;
    ; Check if we're in the raw-mode context (RC / hypervisor) when this happened.
    ;
    test    dword [%$STK_EFLAGS], X86_EFL_VM
    jnz short .gc_not_raw_mode_context

    test    byte [%$STK_CS], 3h           ; check RPL of the cs selector
    jz      .rc_in_raw_mode_context

    ;
    ; Trap in guest code.
    ;
.gc_not_raw_mode_context:
%ifdef DEBUG_STUFF_TRPG
    mov     eax, [%$STK_ERRCD]
    mov     ecx, 'trpG'                 ; indicate trap.
    mov     edx, [%$STK_VECTOR]
    call    trpmDbgDumpRegisterFrame
%endif

    ;
    ; Do we have a GC handler for these traps?
    ;
    mov     edx, [%$STK_VECTOR]
    mov     eax, [g_apfnStaticTrapHandlersGuest + edx * 4]
    or      eax, eax
    jnz short .gc_have_static_handler
    mov     eax, VINF_EM_RAW_GUEST_TRAP
    jmp short .gc_guest_trap

    ;
    ; Call static handler.
    ;
.gc_have_static_handler:
    push    ebx                         ; Param 2 - CPUMCTXCORE pointer.
    push    esi                         ; Param 1 - Pointer to TRPMCPU.
    call    eax
    add     esp, byte 8                 ; cleanup stack (cdecl)
    or      eax, eax
    je near .gc_continue_guest

    ;
    ; Switch back to the host and process it there.
    ;
.gc_guest_trap:
%ifdef VBOX_WITH_STATISTICS
    mov     edx, [%$STK_VECTOR]
    imul    edx, edx, byte STAMPROFILEADV_size ; assume < 128
    add     edx, IMP(g_TRPM)
    add     edx, TRPM.aStatGCTraps
    STAM_PROFILE_ADV_STOP edx
%endif
    mov     edx, IMP(g_VM)
    call    [edx + VM.pfnVMMRCToHostAsm]

    ; We shouldn't ever return this way. So, raise a special IPE if we do.
.gc_panic_again:
    mov     eax, VERR_TRPM_IPE_3
    mov     edx, IMP(g_VM)
    call    [edx + VM.pfnVMMRCToHostAsm]
    jmp     .gc_panic_again

    ;
    ; Continue(/Resume/Restart/Whatever) guest execution.
    ;
ALIGNCODE(16)
.gc_continue_guest:
%ifdef VBOX_WITH_STATISTICS
    mov     edx, [%$STK_VECTOR]
    imul    edx, edx, byte STAMPROFILEADV_size ; assumes < 128
    add     edx, TRPM.aStatGCTraps
    add     edx, IMP(g_TRPM)
    STAM_PROFILE_ADV_STOP edx
%endif

%ifdef VBOX_STRICT
    ; Call CPUM to check sanity.
    mov     edx, IMP(g_VM)
    push    edx
    call    NAME(CPUMRCAssertPreExecutionSanity)
    add     esp, 4
%endif

    ; For v8086 mode we must branch off before we enable write protection.
    test    dword [ebx + CPUMCTXCORE.eflags], X86_EFL_VM
    jnz     .gc_V86_return

    ; enable WP
    mov     eax, cr0                    ;; @todo try elimiate this read.
    or      eax, X86_CR0_WRITE_PROTECT
    mov     cr0, eax

    ; restore guest state and start executing again.
    mov     eax, [ebx + CPUMCTXCORE.eax]
    mov     [%$STK_SAVED_EAX], eax
    mov     ecx, [ebx + CPUMCTXCORE.ecx]
    mov     edx, [ebx + CPUMCTXCORE.edx]
    mov     eax, [ebx + CPUMCTXCORE.ebx]
    mov     [%$STK_SAVED_EBX], eax
    mov     ebp, [ebx + CPUMCTXCORE.ebp]
    mov     esi, [ebx + CPUMCTXCORE.esi]
    mov     edi, [ebx + CPUMCTXCORE.edi]

    mov     eax, [ebx + CPUMCTXCORE.esp]
    mov     [%$STK_ESP], eax
    mov     eax, dword [ebx + CPUMCTXCORE.ss.Sel]
    mov     [%$STK_SS], eax
    mov     eax, [ebx + CPUMCTXCORE.eflags]
    mov     [%$STK_EFLAGS], eax
    mov     eax, dword [ebx + CPUMCTXCORE.cs.Sel]
    mov     [%$STK_CS], eax
    mov     eax, [ebx + CPUMCTXCORE.eip]
    mov     [%$STK_EIP], eax

    mov     ax, [ebx + CPUMCTXCORE.gs.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_GS
    mov     gs, ax

    mov     ax, [ebx + CPUMCTXCORE.fs.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_FS
    mov     fs, ax

    mov     ax, [ebx + CPUMCTXCORE.es.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_ES
    mov     es, ax

    mov     ax, [ebx + CPUMCTXCORE.ds.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_DS
    mov     ds, ax

    ; finally restore our scratch register eax and ebx.
    pop     ebx
    pop     eax
    add     esp, 16 + 8                 ; skip segregs, error code, and vector number.

    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_IRET
    iret

ALIGNCODE(16)
.gc_V86_return:
    ;
    ; We may be returning to V8086 while having entered from protected mode!
    ; So, we have to push the whole stack frame.  There's code in CPUMRC that
    ; does exactly that, so call it instead of duplicating it.
    ;
    push    ebx
    extern  NAME(CPUMGCCallV86Code)
    call    NAME(CPUMGCCallV86Code)
    int3                                ; doesn't return...


    ;
    ; Trap in Hypervisor, try to handle it.
    ;
    ;   (eax = pTRPMCPU)
    ;
ALIGNCODE(16)
.rc_in_raw_mode_context:
    ; fix ss:esp.
    lea     ecx, [%$STK_ESP]              ; calc esp at trap
    mov     [ebx + CPUMCTXCORE.esp], ecx; update esp in register frame
    mov     [ebx + CPUMCTXCORE.ss.Sel], ss ; update ss in register frame

    ; check for temporary handler.
    movzx   edx, byte [esi + TRPMCPU.uActiveVector]
    mov     edi, IMP(g_TRPM)
    xor     ecx, ecx
    xchg    ecx, [edi + TRPM.aTmpTrapHandlers + edx * 4]    ; ecx = Temp handler pointer or 0
    or      ecx, ecx
    jnz short .rc_have_temporary_handler

    ; check for static trap handler.
    mov     ecx, [g_apfnStaticTrapHandlersHyper + edx * 4]  ; ecx = Static handler pointer or 0
    or      ecx, ecx
    jnz short .rc_have_static_handler
    jmp     .rc_abandon_ship


    ;
    ; Temporary trap handler present, call it (CDECL).
    ;
.rc_have_temporary_handler:
    push    ebx                         ; Param 2 - Pointer to CPUMCTXCORE.
    push    IMP(g_VM)                   ; Param 1 - Pointer to VM.
    call    ecx
    add     esp, byte 8                 ; cleanup stack (cdecl)

    cmp     eax, byte VINF_SUCCESS      ; If completely handled Then resume execution.
    je      near .rc_continue
    jmp     .rc_abandon_ship


    ;
    ; Static trap handler present, call it (CDECL).
    ;
.rc_have_static_handler:
    push    ebx                         ; Param 2 - Pointer to CPUMCTXCORE.
    push    esi                         ; Param 1 - Pointer to TRPMCPU
    call    ecx
    add     esp, byte 8                 ; cleanup stack (cdecl)

    cmp     eax, byte VINF_SUCCESS      ; If completely handled Then resume execution.
    je short .rc_continue
    cmp     eax, VINF_EM_DBG_HYPER_STEPPED
    je short .rc_to_host
    cmp     eax, VINF_EM_DBG_HYPER_BREAKPOINT
    je short .rc_to_host
    cmp     eax, VINF_EM_DBG_HYPER_ASSERTION
    je short .rc_to_host
    jmp     .rc_abandon_ship

    ;
    ; Pop back to the host to service the error.
    ;
.rc_to_host:
    mov     edx, IMP(g_VM)
    call    [edx + VM.pfnVMMRCToHostAsmNoReturn]
    mov     eax, VERR_TRPM_DONT_PANIC
    jmp     .rc_to_host

    ;
    ; Continue(/Resume/Restart/Whatever) hypervisor execution.
    ; Don't reset the TRPM state. Caller takes care of that.
    ;
ALIGNCODE(16)
.rc_continue:
%ifdef DEBUG_STUFF
    mov     eax, [%$STK_ERRCD]
    mov     ecx, 'resH'                 ; indicate trap.
    mov     edx, [%$STK_VECTOR]
    call    trpmDbgDumpRegisterFrame
%endif

%ifdef VBOX_WITH_STATISTICS
    mov     edx, [%$STK_VECTOR]
    imul    edx, edx, byte STAMPROFILEADV_size ; assumes < 128
    add     edx, TRPM.aStatGCTraps
    add     edx, IMP(g_TRPM)
    STAM_PROFILE_ADV_STOP edx
%endif

    ; restore
    mov     eax, [ebx + CPUMCTXCORE.eax]
    mov     [%$STK_SAVED_EAX], eax
    mov     ecx, [ebx + CPUMCTXCORE.ecx]
    mov     edx, [ebx + CPUMCTXCORE.edx]
    mov     eax, [ebx + CPUMCTXCORE.ebx]
    mov     [%$STK_SAVED_EBX], eax
    mov     ebp, [ebx + CPUMCTXCORE.ebp]
    mov     esi, [ebx + CPUMCTXCORE.esi]
    mov     edi, [ebx + CPUMCTXCORE.edi]

                                                ; skipping esp & ss.

    mov     eax, [ebx + CPUMCTXCORE.eflags]
    mov     [%$STK_EFLAGS], eax
    mov     eax, dword [ebx + CPUMCTXCORE.cs.Sel]
    mov     [%$STK_CS], eax
    mov     eax, [ebx + CPUMCTXCORE.eip]
    mov     [%$STK_EIP], eax

    mov     ax, [ebx + CPUMCTXCORE.gs.Sel]
    mov     gs, ax

    mov     ax, [ebx + CPUMCTXCORE.fs.Sel]
    mov     fs, ax

    mov     ax, [ebx + CPUMCTXCORE.es.Sel]
    mov     es, ax

    mov     ax, [ebx + CPUMCTXCORE.ds.Sel]
    mov     ds, ax

    ; finally restore our scratch register eax and ebx.
    pop     ebx
    pop     eax
    add     esp, 16 + 8                 ; skip segregs, error code, and vector number.

    iret


    ;
    ; Internal processing error - don't panic, start meditating!
    ;
.rc_abandon_ship:
%ifdef DEBUG_STUFF
    mov     eax, [%$STK_ERRCD]
    mov     ecx, 'trpH'                 ; indicate trap.
    mov     edx, [%$STK_VECTOR]
    call    trpmDbgDumpRegisterFrame
%endif

.rc_do_not_panic:
    mov     edx, IMP(g_VM)
    mov     eax, VERR_TRPM_DONT_PANIC
    call    [edx + VM.pfnVMMRCToHostAsmNoReturn]
%ifdef DEBUG_STUFF
    COM_S_PRINT 'bad!!!'
%endif
    jmp     .rc_do_not_panic            ; this shall never ever happen!
%pop
ENDPROC TRPMGCHandlerGeneric





;;
; We start by 256 push <vector no.> + jmp interruptworker
;
ALIGNCODE(16)
BEGINPROC_EXPORTED TRPMGCHandlerInterupt
    ; NASM has some nice features, here an example of a loop.
%assign i 0
%rep 256
    db 06ah, i   ; push imm8 - note that this is a signextended value.
    jmp   ti_GenericInterrupt
    ALIGNCODE(8)
%assign i i+1
%endrep

;;
; Main interrupt handler for the guest context
;
; Stack:
;        24 GS          (V86 only)
;        20 FS          (V86 only)
;        1C DS          (V86 only)
;        18 ES          (V86 only)
;        14 SS
;        10 ESP
;         c EFLAGS
;         8 CS
;         4 EIP
; ESP ->  0 Vector number (only use low byte!).
;
; @uses     none
ti_GenericInterrupt:
    cld

    ;
    ; Save ds, es, fs, gs, eax and ebx so we have a context pointer (ebx) and
    ; scratch (eax) register to work with.  A sideeffect of using ebx is that
    ; it's preserved accross cdecl calls.
    ;
    ; In order to safely access data, we need to load our flat DS & ES selector,
    ; clear FS and GS (stale guest selector prevention), and clear make sure
    ; that CR0.WP is cleared.
    ;
    push    ds                          ; +14h
    push    es                          ; +10h
    push    fs                          ; +0ch
    push    gs                          ; +08h
    push    eax                         ; +04h
    push    ebx                         ; +00h
%push StackFrame
%define %$STK_SAVED_EBX   esp
%define %$STK_SAVED_EAX   esp + 04h
%define %$STK_SAVED_GS    esp + 08h
%define %$STK_SAVED_FS    esp + 0ch
%define %$STK_SAVED_ES    esp + 10h
%define %$STK_SAVED_DS    esp + 14h
%define %$ESPOFF                18h
%define %$STK_VECTOR      esp + 00h + %$ESPOFF
%define %$STK_EIP         esp + 04h + %$ESPOFF
%define %$STK_CS          esp + 08h + %$ESPOFF
%define %$STK_EFLAGS      esp + 0ch + %$ESPOFF
%define %$STK_ESP         esp + 10h + %$ESPOFF
%define %$STK_SS          esp + 14h + %$ESPOFF
%define %$STK_V86_ES      esp + 18h + %$ESPOFF
%define %$STK_V86_DS      esp + 1ch + %$ESPOFF
%define %$STK_V86_FS      esp + 20h + %$ESPOFF
%define %$STK_V86_GS      esp + 24h + %$ESPOFF

    mov     bx, ss                      ; load
    mov     ds, bx
    mov     es, bx

    xor     bx, bx                      ; load 0 into gs and fs.
    mov     gs, bx
    mov     fs, bx

    mov     eax, cr0                        ;; @todo elimitate this read?
    and     eax, ~X86_CR0_WRITE_PROTECT
    mov     cr0, eax

    mov     ebx, IMP(g_trpmGuestCtxCore)    ; Assume GC as the most common.
    test    byte [%$STK_CS], 3h             ; check RPL of the cs selector
    jnz     .save_guest_state
    test    dword [%$STK_EFLAGS], X86_EFL_VM ; If in V86, then guest.
    jnz     .save_guest_state
    mov     ebx, IMP(g_trpmHyperCtxCore)    ; It's raw-mode context, actually.

    ;
    ; Save the state.
    ;
.save_hyper_state:
    mov     [ebx + CPUMCTXCORE.ecx], ecx
    lea     eax, [%$STK_ESP]
    mov     [ebx + CPUMCTXCORE.esp], eax
    mov     cx, ss
    mov     [ebx + CPUMCTXCORE.ss.Sel], cx
    jmp     .save_state_common

.save_guest_state:
    mov     [ebx + CPUMCTXCORE.ecx], ecx
    mov     eax, [%$STK_ESP]
    mov     [ebx + CPUMCTXCORE.esp], eax
    mov     cx, [%$STK_SS]
    mov     [ebx + CPUMCTXCORE.ss.Sel], cx

.save_state_common:
    mov     eax, [%$STK_SAVED_EAX]
    mov     [ebx + CPUMCTXCORE.eax], eax
    mov     [ebx + CPUMCTXCORE.edx], edx
    mov     eax, [%$STK_SAVED_EBX]
    mov     [ebx + CPUMCTXCORE.ebx], eax
    mov     [ebx + CPUMCTXCORE.esi], esi
    mov     [ebx + CPUMCTXCORE.edi], edi
    mov     [ebx + CPUMCTXCORE.ebp], ebp

    mov     cx, [%$STK_CS]
    mov     [ebx + CPUMCTXCORE.cs.Sel], cx
    mov     eax, [%$STK_EIP]
    mov     [ebx + CPUMCTXCORE.eip], eax
    mov     eax, [%$STK_EFLAGS]
    mov     [ebx + CPUMCTXCORE.eflags], eax

%if GC_ARCH_BITS == 64 ; zero out the high dwords - probably not necessary any more.
    mov     dword [ebx + CPUMCTXCORE.eax + 4], 0
    mov     dword [ebx + CPUMCTXCORE.ecx + 4], 0
    mov     dword [ebx + CPUMCTXCORE.edx + 4], 0
    mov     dword [ebx + CPUMCTXCORE.ebx + 4], 0
    mov     dword [ebx + CPUMCTXCORE.esi + 4], 0
    mov     dword [ebx + CPUMCTXCORE.edi + 4], 0
    mov     dword [ebx + CPUMCTXCORE.ebp + 4], 0
    mov     dword [ebx + CPUMCTXCORE.esp + 4], 0
    mov     dword [ebx + CPUMCTXCORE.eip + 4], 0
%endif

    test    dword [%$STK_EFLAGS], X86_EFL_VM
    jnz     .save_V86_segregs

    mov     cx, [%$STK_SAVED_ES]
    mov     [ebx + CPUMCTXCORE.es.Sel], cx
    mov     cx, [%$STK_SAVED_DS]
    mov     [ebx + CPUMCTXCORE.ds.Sel], cx
    mov     cx, [%$STK_SAVED_FS]
    mov     [ebx + CPUMCTXCORE.fs.Sel], cx
    mov     cx, [%$STK_SAVED_GS]
    mov     [ebx + CPUMCTXCORE.gs.Sel], cx
    jmp     .done_saving

    ;
    ; The DS, ES, FS and GS registers are zeroed in V86 mode and their real
    ; values are on the stack.
    ;
.save_V86_segregs:
    mov     cx, [%$STK_V86_ES]
    mov     [ebx + CPUMCTXCORE.es.Sel], cx
    mov     cx, [%$STK_V86_DS]
    mov     [ebx + CPUMCTXCORE.ds.Sel], cx
    mov     cx, [%$STK_V86_FS]
    mov     [ebx + CPUMCTXCORE.fs.Sel], cx
    mov     cx, [%$STK_V86_GS]
    mov     [ebx + CPUMCTXCORE.gs.Sel], cx

.done_saving:

    ;
    ; Store the information about the active trap/interrupt.
    ;
    mov     esi, IMP(g_TRPMCPU)         ; esi = TRPMCPU until resume!
    movzx   edx, byte [%$STK_VECTOR]
    mov     [esi + TRPMCPU.uActiveVector], edx
    mov     dword [esi + TRPMCPU.uActiveErrorCode], 0
    mov     dword [esi + TRPMCPU.enmActiveType], TRPM_TRAP
    mov     dword [esi + TRPMCPU.uActiveCR2], edx
%if GC_ARCH_BITS == 64 ; zero out the high dwords.
    mov     dword [esi + TRPMCPU.uActiveErrorCode + 4], 0
    mov     dword [esi + TRPMCPU.uActiveCR2 + 4], 0
%endif

%ifdef VBOX_WITH_STATISTICS
    ;
    ; Update statistics.
    ;
    mov     edi, IMP(g_TRPM)
    movzx   edx, byte [%$STK_VECTOR]   ; vector number
    imul    edx, edx, byte STAMCOUNTER_size
    add     edx, [edi + TRPM.paStatHostIrqRC]
    STAM_COUNTER_INC edx
%endif

    ;
    ; Check if we're in the raw-mode context (RC / hypervisor) when this happened.
    ;
    test    dword [%$STK_EFLAGS], X86_EFL_VM
    jnz short .gc_not_raw_mode_context

    test    byte [%$STK_CS], 3h           ; check RPL of the cs selector
    jz      .rc_in_raw_mode_context

    ;
    ; Trap in guest code.
    ;
.gc_not_raw_mode_context:
    and     dword [ebx + CPUMCTXCORE.eflags], ~X86_EFL_RF ; Clear RF.
                                           ; The guest shall not see this in it's state.
%ifdef DEBUG_STUFF_INT
    xor     eax, eax
    mov     ecx, 'intG'                    ; indicate trap in GC.
    movzx   edx, byte [%$STK_VECTOR]
    call    trpmDbgDumpRegisterFrame
%endif

    ;
    ; Switch back to the host and process it there.
    ;
    mov     edx, IMP(g_VM)
    mov     eax, VINF_EM_RAW_INTERRUPT
    call    [edx + VM.pfnVMMRCToHostAsm]

    ;
    ; We've returned!
    ;

    ; Reset TRPM state
    xor     edx, edx
    dec     edx                         ; edx = 0ffffffffh
    xchg    [esi + TRPMCPU.uActiveVector], edx
    mov     [esi + TRPMCPU.uPrevVector], edx

%ifdef VBOX_STRICT
    ; Call CPUM to check sanity.
    mov     edx, IMP(g_VM)
    push    edx
    call    NAME(CPUMRCAssertPreExecutionSanity)
    add     esp, 4
%endif

    ; For v8086 mode we must branch off before we enable write protection.
    test    dword [ebx + CPUMCTXCORE.eflags], X86_EFL_VM
    jnz     .gc_V86_return

    ; enable WP
    mov     eax, cr0                    ;; @todo try elimiate this read.
    or      eax, X86_CR0_WRITE_PROTECT
    mov     cr0, eax

    ; restore guest state and start executing again.
    mov     eax, [ebx + CPUMCTXCORE.eax]
    mov     [%$STK_SAVED_EAX], eax
    mov     ecx, [ebx + CPUMCTXCORE.ecx]
    mov     edx, [ebx + CPUMCTXCORE.edx]
    mov     eax, [ebx + CPUMCTXCORE.ebx]
    mov     [%$STK_SAVED_EBX], eax
    mov     ebp, [ebx + CPUMCTXCORE.ebp]
    mov     esi, [ebx + CPUMCTXCORE.esi]
    mov     edi, [ebx + CPUMCTXCORE.edi]

    mov     eax, [ebx + CPUMCTXCORE.esp]
    mov     [%$STK_ESP], eax
    mov     eax, dword [ebx + CPUMCTXCORE.ss.Sel]
    mov     [%$STK_SS], eax
    mov     eax, [ebx + CPUMCTXCORE.eflags]
    mov     [%$STK_EFLAGS], eax
    mov     eax, dword [ebx + CPUMCTXCORE.cs.Sel]
    mov     [%$STK_CS], eax
    mov     eax, [ebx + CPUMCTXCORE.eip]
    mov     [%$STK_EIP], eax

    mov     ax, [ebx + CPUMCTXCORE.gs.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_GS
    mov     gs, ax

    mov     ax, [ebx + CPUMCTXCORE.fs.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_FS
    mov     fs, ax

    mov     ax, [ebx + CPUMCTXCORE.es.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_ES
    mov     es, ax

    mov     ax, [ebx + CPUMCTXCORE.ds.Sel]
    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_MOV_DS
    mov     ds, ax

    ; finally restore our scratch register eax and ebx.
    pop     ebx
    pop     eax
    add     esp, 16 + 4                 ; skip segregs, and vector number.

    TRPM_NP_GP_HANDLER NAME(trpmRCTrapInGeneric), TRPM_TRAP_IN_IRET
    iret

ALIGNCODE(16)
.gc_V86_return:
    ;
    ; We may be returning to V8086 while having entered from protected mode!
    ; So, we have to push the whole stack frame.  There's code in CPUMRC that
    ; does exactly that, so call it instead of duplicating it.
    ;
    push    ebx
    extern  NAME(CPUMGCCallV86Code)
    call    NAME(CPUMGCCallV86Code)
    int3                                ; doesn't return...


    ; -+- Entry point -+-
    ;
    ; We're in hypervisor mode which means no guest context
    ; and special care to be taken to restore the hypervisor
    ; context correctly.
    ;
    ; ATM the only place this can happen is when entering a trap handler.
    ; We make ASSUMPTIONS about this in respects to the WP CR0 bit
    ;
ALIGNCODE(16)
.rc_in_raw_mode_context:
    ; fix ss:esp.
    lea     ecx, [%$STK_ESP]                ; calc esp at trap
    mov     [ebx + CPUMCTXCORE.esp], ecx    ; update esp in register frame
    mov     [ebx + CPUMCTXCORE.ss.Sel], ss  ; update ss in register frame

%ifdef DEBUG_STUFF_INT
    xor     eax, eax
    mov     ecx, 'intH'                    ; indicate trap in RC.
    movzx   edx, byte [%$STK_VECTOR]
    call    trpmDbgDumpRegisterFrame
%endif

    mov     edx, IMP(g_VM)
    mov     eax, VINF_EM_RAW_INTERRUPT_HYPER
    call    [edx + VM.pfnVMMRCToHostAsmNoReturn]
%ifdef DEBUG_STUFF_INT
    COM_S_CHAR '!'
%endif

    ;
    ; We've returned!
    ; Continue(/Resume/Restart/Whatever) hypervisor execution.
    ;

    ; Reset TRPM state - don't record this.
    ;mov     esi, IMP(g_TRPMCPU)
    mov     dword [esi + TRPMCPU.uActiveVector], 0ffffffffh

    ;
    ; Restore the hypervisor context and return.
    ;
    mov     eax, [ebx + CPUMCTXCORE.eax]
    mov     [%$STK_SAVED_EAX], eax
    mov     ecx, [ebx + CPUMCTXCORE.ecx]
    mov     edx, [ebx + CPUMCTXCORE.edx]
    mov     eax, [ebx + CPUMCTXCORE.ebx]
    mov     [%$STK_SAVED_EBX], eax
    mov     ebp, [ebx + CPUMCTXCORE.ebp]
    mov     esi, [ebx + CPUMCTXCORE.esi]
    mov     edi, [ebx + CPUMCTXCORE.edi]

                                        ; skipping esp & ss.

    mov     eax, [ebx + CPUMCTXCORE.eflags]
    mov     [%$STK_EFLAGS], eax
    mov     eax, dword [ebx + CPUMCTXCORE.cs.Sel]
    mov     [%$STK_CS], eax
    mov     eax, [ebx + CPUMCTXCORE.eip]
    mov     [%$STK_EIP], eax

    mov     ax, [ebx + CPUMCTXCORE.gs.Sel]
    mov     gs, ax

    mov     ax, [ebx + CPUMCTXCORE.fs.Sel]
    mov     fs, ax

    mov     ax, [ebx + CPUMCTXCORE.es.Sel]
    mov     es, ax

    mov     ax, [ebx + CPUMCTXCORE.ds.Sel]
    mov     ds, ax

    ; finally restore our scratch register eax and ebx.
    pop     ebx
    pop     eax
    add     esp, 16 + 4                 ; skip segregs, and vector number.

    iret
%pop
ENDPROC TRPMGCHandlerInterupt



;;
; Trap handler for #MC
;
; This handler will forward the #MC to the host OS. Since this
; is generalized in the generic interrupt handler, we just disable
; interrupts and push vector number and jump to the generic code.
;
; Stack:
;           10  SS          (only if ring transition.)
;            c  ESP         (only if ring transition.)
;            8  EFLAGS
;            4  CS
;            0  EIP
;
; @uses     none
;
ALIGNCODE(16)
BEGINPROC_EXPORTED TRPMGCHandlerTrap12
    push    byte 12h
    jmp     ti_GenericInterrupt
ENDPROC TRPMGCHandlerTrap12




;;
; Trap handler for double fault (#DF).
;
; This is a special trap handler executes in separate task with own TSS, with
; one of the intermediate memory contexts instead of the shadow context.
; The handler will unconditionally print an report to the comport configured
; for the COM_S_* macros before attempting to return to the host. If it it ends
; up double faulting more than 10 times, it will simply cause an triple fault
; to get us out of the mess.
;
; @param    esp         Half way down the hypervisor stack + the trap frame.
; @param    ebp         Half way down the hypervisor stack.
; @param    eflags      Interrupts disabled, nested flag is probably set (we don't care).
; @param    ecx         The address of the hypervisor TSS.
; @param    edi         Same as ecx.
; @param    eax         Same as ecx.
; @param    edx         Address of the VM structure.
; @param    esi         Same as edx.
; @param    ebx         Same as edx.
; @param    ss          Hypervisor DS.
; @param    ds          Hypervisor DS.
; @param    es          Hypervisor DS.
; @param    fs          0
; @param    gs          0
;
;
; @remark   To be able to catch errors with WP turned off, it is required that the
;           TSS GDT descriptor and the TSSes are writable (X86_PTE_RW). See SELM.cpp
;           for how to enable this.
;
; @remark   It is *not* safe to resume the VMM after a double fault. (At least not
;           without clearing the busy flag of the TssTrap8 and fixing whatever cause it.)
;
ALIGNCODE(16)
BEGINPROC_EXPORTED TRPMGCHandlerTrap08
    ; be careful.
    cli
    cld

    ;
    ; Disable write protection.
    ;
    mov     eax, cr0
    and     eax, ~X86_CR0_WRITE_PROTECT
    mov     cr0, eax

    ;
    ; Load Hypervisor DS and ES (get it from the SS) - paranoia, but the TSS could be overwritten.. :)
    ;
    mov     eax, ss
    mov     ds, eax
    mov     es, eax

    COM_S_PRINT 10,13,'*** Guru Meditation 00000008 - Double Fault! ***',10,13

    COM_S_PRINT 'VM='
    COM_S_DWORD_REG edx
    COM_S_PRINT '    prevTSS='
    COM_S_DWORD_REG ecx
    COM_S_PRINT '    prevCR3='
    mov    eax, [ecx + VBOXTSS.cr3]
    COM_S_DWORD_REG eax
    COM_S_PRINT '    prevLdtr='
    movzx  eax, word [ecx + VBOXTSS.selLdt]
    COM_S_DWORD_REG eax
    COM_S_NEWLINE

    ;
    ; Create CPUMCTXCORE structure.
    ;
    mov     ebx, IMP(g_trpmHyperCtxCore)    ; It's raw-mode context, actually.

    mov     eax, [ecx + VBOXTSS.eip]
    mov     [ebx + CPUMCTXCORE.eip], eax
%if GC_ARCH_BITS == 64
    ; zero out the high dword
    mov     dword [ebx + CPUMCTXCORE.eip + 4], 0
%endif
    mov     eax, [ecx + VBOXTSS.eflags]
    mov     [ebx + CPUMCTXCORE.eflags], eax

    movzx   eax, word [ecx + VBOXTSS.cs]
    mov     dword [ebx + CPUMCTXCORE.cs.Sel], eax
    movzx   eax, word [ecx + VBOXTSS.ds]
    mov     dword [ebx + CPUMCTXCORE.ds.Sel], eax
    movzx   eax, word [ecx + VBOXTSS.es]
    mov     dword [ebx + CPUMCTXCORE.es.Sel], eax
    movzx   eax, word [ecx + VBOXTSS.fs]
    mov     dword [ebx + CPUMCTXCORE.fs.Sel], eax
    movzx   eax, word [ecx + VBOXTSS.gs]
    mov     dword [ebx + CPUMCTXCORE.gs.Sel], eax
    movzx   eax, word [ecx + VBOXTSS.ss]
    mov     [ebx + CPUMCTXCORE.ss.Sel], eax
    mov     eax, [ecx + VBOXTSS.esp]
    mov     [ebx + CPUMCTXCORE.esp], eax
%if GC_ARCH_BITS == 64
    ; zero out the high dword
    mov     dword [ebx + CPUMCTXCORE.esp + 4], 0
%endif
    mov     eax, [ecx + VBOXTSS.ecx]
    mov     [ebx + CPUMCTXCORE.ecx], eax
    mov     eax, [ecx + VBOXTSS.edx]
    mov     [ebx + CPUMCTXCORE.edx], eax
    mov     eax, [ecx + VBOXTSS.ebx]
    mov     [ebx + CPUMCTXCORE.ebx], eax
    mov     eax, [ecx + VBOXTSS.eax]
    mov     [ebx + CPUMCTXCORE.eax], eax
    mov     eax, [ecx + VBOXTSS.ebp]
    mov     [ebx + CPUMCTXCORE.ebp], eax
    mov     eax, [ecx + VBOXTSS.esi]
    mov     [ebx + CPUMCTXCORE.esi], eax
    mov     eax, [ecx + VBOXTSS.edi]
    mov     [ebx + CPUMCTXCORE.edi], eax

    ;
    ; Show regs
    ;
    mov     eax, 0ffffffffh
    mov     ecx, 'trpH'                 ; indicate trap.
    mov     edx, 08h                    ; vector number
    call    trpmDbgDumpRegisterFrame

    ;
    ; Should we try go back?
    ;
    inc     dword [df_Count]
    cmp     dword [df_Count], byte 10
    jb      df_to_host
    jmp     df_tripple_fault
df_Count: dd 0

    ;
    ; Try return to the host.
    ;
df_to_host:
    COM_S_PRINT 'Trying to return to host...',10,13
    mov     edx, IMP(g_VM)
    mov     eax, VERR_TRPM_PANIC
    call    [edx + VM.pfnVMMRCToHostAsmNoReturn]
    jmp short df_to_host

    ;
    ; Perform a tripple fault.
    ;
df_tripple_fault:
    COM_S_PRINT 'Giving up - tripple faulting the machine...',10,13
    push    byte 0
    push    byte 0
    sidt    [esp]
    mov     word [esp], 0
    lidt    [esp]
    xor     eax, eax
    mov     dword [eax], 0
    jmp     df_tripple_fault

ENDPROC TRPMGCHandlerTrap08




;;
; Internal procedure used to dump registers.
;
; @param    ebx     Pointer to CPUMCTXCORE.
; @param    edx     Vector number
; @param    ecx     'trap' if trap, 'int' if interrupt.
; @param    eax     Error code if trap.
;
trpmDbgDumpRegisterFrame:
    sub     esp, byte 8                 ; working space for sidt/sgdt/etc

;   Init _must_ be done on host before crashing!
;    push    edx
;    push    eax
;    COM_INIT
;    pop     eax
;    pop     edx

    cmp     ecx, 'trpH'
    je near tddrf_trpH
    cmp     ecx, 'trpG'
    je near tddrf_trpG
    cmp     ecx, 'intH'
    je near tddrf_intH
    cmp     ecx, 'intG'
    je near tddrf_intG
    cmp     ecx, 'resH'
    je near tddrf_resH
    COM_S_PRINT 10,13,'*** Bogus Dump Code '
    jmp     tddrf_regs

%if 1 ; the verbose version

tddrf_intG:
    COM_S_PRINT 10,13,'*** Interrupt (Guest) '
    COM_S_DWORD_REG edx
    jmp     tddrf_regs

tddrf_intH:
    COM_S_PRINT 10,13,'*** Interrupt (Hypervisor) '
    COM_S_DWORD_REG edx
    jmp     tddrf_regs

tddrf_trpG:
    COM_S_PRINT 10,13,'*** Trap '
    jmp     tddrf_trap_rest

%else ; the short version

tddrf_intG:
    COM_S_CHAR 'I'
    jmp     tddrf_ret

tddrf_intH:
    COM_S_CHAR 'i'
    jmp     tddrf_ret

tddrf_trpG:
    COM_S_CHAR 'T'
    jmp     tddrf_ret

%endif ; the short version

tddrf_trpH:
    COM_S_PRINT 10,13,'*** Guru Meditation '
    jmp     tddrf_trap_rest

tddrf_resH:
    COM_S_PRINT 10,13,'*** Resuming Hypervisor Trap '
    jmp     tddrf_trap_rest

tddrf_trap_rest:
    COM_S_DWORD_REG edx
    COM_S_PRINT ' ErrorCode='
    COM_S_DWORD_REG eax
    COM_S_PRINT ' cr2='
    mov ecx, cr2
    COM_S_DWORD_REG ecx

tddrf_regs:
    COM_S_PRINT ' ***',10,13,'cs:eip='
    movzx   ecx, word [ebx + CPUMCTXCORE.cs.Sel]
    COM_S_DWORD_REG ecx
    COM_S_CHAR ':'
    mov     ecx, [ebx + CPUMCTXCORE.eip]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '    ss:esp='
    movzx   ecx, word [ebx + CPUMCTXCORE.ss.Sel]
    COM_S_DWORD_REG ecx
    COM_S_CHAR ':'
    mov     ecx, [ebx + CPUMCTXCORE.esp]
    COM_S_DWORD_REG ecx


    sgdt    [esp]
    COM_S_PRINT 10,13,'  gdtr='
    movzx   ecx, word [esp]
    COM_S_DWORD_REG ecx
    COM_S_CHAR  ':'
    mov     ecx, [esp + 2]
    COM_S_DWORD_REG ecx

    sidt    [esp]
    COM_S_PRINT '      idtr='
    movzx   ecx, word [esp]
    COM_S_DWORD_REG ecx
    COM_S_CHAR  ':'
    mov     ecx, [esp + 2]
    COM_S_DWORD_REG ecx


    str     [esp]                       ; yasm BUG! it generates sldt [esp] here! YASMCHECK!
    COM_S_PRINT 10,13,' tr='
    movzx   ecx, word [esp]
    COM_S_DWORD_REG ecx

    sldt    [esp]
    COM_S_PRINT ' ldtr='
    movzx   ecx, word [esp]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  eflags='
    mov     ecx, [ebx + CPUMCTXCORE.eflags]
    COM_S_DWORD_REG ecx


    COM_S_PRINT 10,13,'cr0='
    mov     ecx, cr0
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  cr2='
    mov     ecx, cr2
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  cr3='
    mov     ecx, cr3
    COM_S_DWORD_REG ecx
    COM_S_PRINT '  cr4='
    mov     ecx, cr4
    COM_S_DWORD_REG ecx


    COM_S_PRINT 10,13,' ds='
    movzx   ecx, word [ebx + CPUMCTXCORE.ds.Sel]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '   es='
    movzx   ecx, word [ebx + CPUMCTXCORE.es.Sel]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '   fs='
    movzx   ecx, word [ebx + CPUMCTXCORE.fs.Sel]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '   gs='
    movzx   ecx, word [ebx + CPUMCTXCORE.gs.Sel]
    COM_S_DWORD_REG ecx


    COM_S_PRINT 10,13,'eax='
    mov     ecx, [ebx + CPUMCTXCORE.eax]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  ebx='
    mov     ecx, [ebx + CPUMCTXCORE.ebx]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  ecx='
    mov     ecx, [ebx + CPUMCTXCORE.ecx]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  edx='
    mov     ecx, [ebx + CPUMCTXCORE.edx]
    COM_S_DWORD_REG ecx


    COM_S_PRINT 10,13,'esi='
    mov     ecx, [ebx + CPUMCTXCORE.esi]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  edi='
    mov     ecx, [ebx + CPUMCTXCORE.edi]
    COM_S_DWORD_REG ecx

    COM_S_PRINT '  ebp='
    mov     ecx, [ebx + CPUMCTXCORE.ebp]
    COM_S_DWORD_REG ecx


    COM_S_NEWLINE

tddrf_ret:
    add     esp, byte 8
    ret


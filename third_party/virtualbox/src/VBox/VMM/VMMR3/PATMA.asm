; $Id: PATMA.asm $
;; @file
; PATM Assembly Routines.
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

;;
; @note This method has problems in theory. If we fault for any reason, then we won't be able to restore
;       the guest's context properly!!
;       E.g if one of the push instructions causes a fault or SS isn't wide open and our patch GC state accesses aren't valid.
; @assumptions
;       - Enough stack for a few pushes
;       - The SS selector has base 0 and limit 0xffffffff
;
; @todo stack probing is currently hardcoded and not present everywhere (search for 'probe stack')


;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "VBox/err.mac"
%include "iprt/x86.mac"
%include "VBox/vmm/cpum.mac"
%include "VBox/vmm/vm.mac"
%include "PATMA.mac"


;*******************************************************************************
;*  Defined Constants And Macros                                               *
;*******************************************************************************
%ifdef DEBUG
; Noisy, but useful for debugging certain problems
;;;%define PATM_LOG_PATCHINSTR
;;%define PATM_LOG_PATCHIRET
%endif

;;
; Simple PATCHASMRECORD initializer
; @param %1     The patch function name.
; @param %2     The number of fixups.
;
%macro PATCHASMRECORD_INIT 2
istruc PATCHASMRECORD
    at PATCHASMRECORD.pbFunction,     RTCCPTR_DEF NAME(%1)
    at PATCHASMRECORD.offJump,        DD          0
    at PATCHASMRECORD.offRelJump,     DD          0
    at PATCHASMRECORD.offSizeOverride,DD          0
    at PATCHASMRECORD.cbFunction,     DD          NAME(%1 %+ _EndProc) - NAME(%1)
    at PATCHASMRECORD.cRelocs,        DD          %2
iend
%endmacro

;;
; Simple PATCHASMRECORD initializer
; @param %1     The patch function name.
; @param %2     Jump lable.
; @param %3     The number of fixups.
;
%macro PATCHASMRECORD_INIT_JUMP 3
istruc PATCHASMRECORD
    at PATCHASMRECORD.pbFunction,     RTCCPTR_DEF NAME(%1)
    at PATCHASMRECORD.offJump,        DD          %2 - NAME(%1)
    at PATCHASMRECORD.offRelJump,     DD          0
    at PATCHASMRECORD.offSizeOverride,DD          0
    at PATCHASMRECORD.cbFunction,     DD          NAME(%1 %+ _EndProc) - NAME(%1)
    at PATCHASMRECORD.cRelocs,        DD          %3
iend
%endmacro

;;
; Simple PATCHASMRECORD initializer
; @param %1     The patch function name.
; @param %2     Jump lable (or nothing).
; @param %3     Relative jump label (or nothing).
; @param %4     Size override label (or nothing).
; @param %5     The number of fixups.
;
%macro PATCHASMRECORD_INIT_EX 5
istruc PATCHASMRECORD
    at PATCHASMRECORD.pbFunction,     RTCCPTR_DEF NAME(%1)
%ifid %2
    at PATCHASMRECORD.offJump,        DD          %2 - NAME(%1)
%else
    at PATCHASMRECORD.offJump,        DD          0
%endif
%ifid %3
    at PATCHASMRECORD.offRelJump,     DD          %3 - NAME(%1)
%else
    at PATCHASMRECORD.offRelJump,     DD          0
%endif
%ifid %4
    at PATCHASMRECORD.offSizeOverride,DD          %4 - NAME(%1)
%else
    at PATCHASMRECORD.offSizeOverride,DD          0
%endif
    at PATCHASMRECORD.cbFunction,     DD          NAME(%1 %+ _EndProc) - NAME(%1)
    at PATCHASMRECORD.cRelocs,        DD          %5
iend
%endmacro

;;
; Switches to the code section and aligns the function.
;
; @remarks This section must be different from the patch readonly data section!
;
%macro BEGIN_PATCH_CODE_SECTION 0
BEGINCODE
align 32
%endmacro
%macro BEGIN_PATCH_CODE_SECTION_NO_ALIGN 0
BEGINCODE
%endmacro

;;
; Switches to the data section for the read-only patch descriptor data and
; aligns it appropriately.
;
; @remarks This section must be different from the patch code section!
;
%macro BEGIN_PATCH_RODATA_SECTION 0
BEGINDATA
align 16
%endmacro
%macro BEGIN_PATCH_RODATA_SECTION_NO_ALIGN 0
BEGINDATA
%endmacro


;;
; Starts a patch.
;
; @param %1     The patch record name (externally visible).
; @param %2     The patch function name (considered internal).
;
%macro BEGIN_PATCH 2
; The patch record.
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME %1
PATCHASMRECORD_INIT PATMCpuidReplacement, (RT_CONCAT(%1,_FixupEnd) - RT_CONCAT(%1,_FixupStart)) / 8
RT_CONCAT(%1,_FixupStart):

; The patch code.
BEGIN_PATCH_CODE_SECTION
BEGINPROC %2
%endmacro

;;
; Emit a fixup.
; @param %1     The fixup type.
%macro PATCH_FIXUP 1
BEGIN_PATCH_RODATA_SECTION_NO_ALIGN
    dd      %1, 0
BEGIN_PATCH_CODE_SECTION_NO_ALIGN
%endmacro

;;
; Emit a fixup with extra info.
; @param %1     The fixup type.
; @param %2     The extra fixup info.
%macro PATCH_FIXUP_2 2
BEGIN_PATCH_RODATA_SECTION_NO_ALIGN
    dd      %1, %2
BEGIN_PATCH_CODE_SECTION_NO_ALIGN
%endmacro

;;
; Ends a patch.
;
; This terminates the function and fixup array.
;
; @param %1     The patch record name (externally visible).
; @param %2     The patch function name (considered internal).
;
%macro END_PATCH 2
ENDPROC %2

; Terminate the fixup array.
BEGIN_PATCH_RODATA_SECTION_NO_ALIGN
RT_CONCAT(%1,_FixupEnd):
    dd      0ffffffffh, 0ffffffffh
BEGIN_PATCH_CODE_SECTION_NO_ALIGN
%endmacro


;
; Switch to 32-bit mode (x86).
;
%ifdef RT_ARCH_AMD64
 BITS 32
%endif


%ifdef VBOX_WITH_STATISTICS
;
; Patch call statistics
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMStats
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf
    inc     dword [ss:PATM_ASMFIX_ALLPATCHCALLS]
    inc     dword [ss:PATM_ASMFIX_PERPATCHCALLS]
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMStats

; Patch record for statistics
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmStatsRecord
    PATCHASMRECORD_INIT PATMStats, 4
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_ALLPATCHCALLS, 0
    DD      PATM_ASMFIX_PERPATCHCALLS, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh
%endif ; VBOX_WITH_STATISTICS


;
; Set PATM_ASMFIX_INTERRUPTFLAG
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMSetPIF
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMSetPIF

; Patch record for setting PATM_ASMFIX_INTERRUPTFLAG
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmSetPIFRecord
    PATCHASMRECORD_INIT PATMSetPIF, 1
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh

;
; Clear PATM_ASMFIX_INTERRUPTFLAG
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMClearPIF
    ; probe stack here as we can't recover from page faults later on
    not     dword [esp-64]
    not     dword [esp-64]
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
ENDPROC     PATMClearPIF

; Patch record for clearing PATM_ASMFIX_INTERRUPTFLAG
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmClearPIFRecord
    PATCHASMRECORD_INIT PATMClearPIF, 1
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh

;
; Clear PATM_ASMFIX_INHIBITIRQADDR and fault if IF=0
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMClearInhibitIRQFaultIF0
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    mov     dword [ss:PATM_ASMFIX_INHIBITIRQADDR], 0
    pushf

    test    dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF
    jz      PATMClearInhibitIRQFaultIF0_Fault

    ; if interrupts are pending, then we must go back to the host context to handle them!
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST
    jz      PATMClearInhibitIRQFaultIF0_Continue

    ; Go to our hypervisor trap handler to dispatch the pending irq
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_EDI], edi
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX | PATM_RESTORE_EDI
    mov     eax, PATM_ACTION_DISPATCH_PENDING_IRQ
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, PATM_ASMFIX_NEXTINSTRADDR
    popfd                   ; restore flags we pushed above (the or instruction changes the flags as well)
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return

PATMClearInhibitIRQFaultIF0_Fault:
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

PATMClearInhibitIRQFaultIF0_Continue:
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMClearInhibitIRQFaultIF0

; Patch record for clearing PATM_ASMFIX_INHIBITIRQADDR
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmClearInhibitIRQFaultIF0Record
    PATCHASMRECORD_INIT PATMClearInhibitIRQFaultIF0, 12
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_INHIBITIRQADDR,     0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VM_FORCEDACTIONS,   0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_EDI,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      PATM_ASMFIX_NEXTINSTRADDR,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      0ffffffffh, 0ffffffffh


;
; Clear PATM_ASMFIX_INHIBITIRQADDR and continue if IF=0 (duplicated function only; never jump back to guest code afterwards!!)
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMClearInhibitIRQContIF0
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    mov     dword [ss:PATM_ASMFIX_INHIBITIRQADDR], 0
    pushf

    test    dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF
    jz      PATMClearInhibitIRQContIF0_Continue

    ; if interrupts are pending, then we must go back to the host context to handle them!
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST
    jz      PATMClearInhibitIRQContIF0_Continue

    ; Go to our hypervisor trap handler to dispatch the pending irq
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_EDI], edi
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX | PATM_RESTORE_EDI
    mov     eax, PATM_ACTION_DISPATCH_PENDING_IRQ
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, PATM_ASMFIX_NEXTINSTRADDR
    popfd                   ; restore flags we pushed above (the or instruction changes the flags as well)
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return

PATMClearInhibitIRQContIF0_Continue:
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMClearInhibitIRQContIF0

; Patch record for clearing PATM_ASMFIX_INHIBITIRQADDR
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmClearInhibitIRQContIF0Record
    PATCHASMRECORD_INIT PATMClearInhibitIRQContIF0, 11
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_INHIBITIRQADDR,     0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VM_FORCEDACTIONS,   0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_EDI,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      PATM_ASMFIX_NEXTINSTRADDR,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      0ffffffffh, 0ffffffffh


;
;
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMCliReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf
%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ecx
    mov     eax, PATM_ACTION_LOG_CLI
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     ecx
    pop     eax
%endif

    and     dword [ss:PATM_ASMFIX_VMFLAGS], ~X86_EFL_IF
    popf

    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMCliJump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMCliReplacement

; Patch record for 'cli'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmCliRecord
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT_JUMP PATMCliReplacement, PATMCliJump, 4
%else
    PATCHASMRECORD_INIT_JUMP PATMCliReplacement, PATMCliJump, 3
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION, 0
%endif
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
;
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMStiReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    mov     dword [ss:PATM_ASMFIX_INHIBITIRQADDR], PATM_ASMFIX_NEXTINSTRADDR
    pushf
%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ecx
    mov     eax, PATM_ACTION_LOG_STI
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     ecx
    pop     eax
%endif
    or      dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMStiReplacement

; Patch record for 'sti'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmStiRecord
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT PATMStiReplacement, 6
%else
    PATCHASMRECORD_INIT PATMStiReplacement, 5
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG,  0
    DD      PATM_ASMFIX_INHIBITIRQADDR, 0
    DD      PATM_ASMFIX_NEXTINSTRADDR,  0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION,  0
%endif
    DD      PATM_ASMFIX_VMFLAGS,        0
    DD      PATM_ASMFIX_INTERRUPTFLAG,  0
    DD      0ffffffffh, 0ffffffffh


;
; Trampoline code for trap entry (without error code on the stack)
;
; esp + 32 - GS         (V86 only)
; esp + 28 - FS         (V86 only)
; esp + 24 - DS         (V86 only)
; esp + 20 - ES         (V86 only)
; esp + 16 - SS         (if transfer to inner ring)
; esp + 12 - ESP        (if transfer to inner ring)
; esp + 8  - EFLAGS
; esp + 4  - CS
; esp      - EIP
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMTrapEntry
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf

%ifdef PATM_LOG_PATCHIRET
    push    eax
    push    ecx
    push    edx
    lea     edx, dword [ss:esp+12+4]        ;3 dwords + pushed flags -> iret eip
    mov     eax, PATM_ACTION_LOG_GATE_ENTRY
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     eax
%endif

    test    dword [esp+12], X86_EFL_VM
    jnz     PATMTrapNoRing1

    ; make sure the saved CS selector for ring 1 is made 0
    test    dword [esp+8], 2
    jnz     PATMTrapNoRing1
    test    dword [esp+8], 1
    jz      PATMTrapNoRing1
    and     dword [esp+8], dword ~1     ; yasm / nasm dword
PATMTrapNoRing1:

    ; correct EFLAGS on the stack to include the current IOPL
    push    eax
    mov     eax, dword [ss:PATM_ASMFIX_VMFLAGS]
    and     eax, X86_EFL_IOPL
    and     dword [esp+16], ~X86_EFL_IOPL       ; esp+16 = eflags = esp+8+4(efl)+4(eax)
    or      dword [esp+16], eax
    pop     eax

    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMTrapEntryJump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMTrapEntry

; Patch record for trap gate entrypoint
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmTrapEntryRecord
%ifdef PATM_LOG_PATCHIRET
    PATCHASMRECORD_INIT_JUMP PATMTrapEntry, PATMTrapEntryJump, 4
%else
    PATCHASMRECORD_INIT_JUMP PATMTrapEntry, PATMTrapEntryJump, 3
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
%ifdef PATM_LOG_PATCHIRET
    DD      PATM_ASMFIX_PENDINGACTION, 0
%endif
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; Trampoline code for trap entry (with error code on the stack)
;
; esp + 36 - GS         (V86 only)
; esp + 32 - FS         (V86 only)
; esp + 28 - DS         (V86 only)
; esp + 24 - ES         (V86 only)
; esp + 20 - SS         (if transfer to inner ring)
; esp + 16 - ESP        (if transfer to inner ring)
; esp + 12 - EFLAGS
; esp + 8  - CS
; esp + 4  - EIP
; esp      - error code
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMTrapEntryErrorCode
PATMTrapErrorCodeEntryStart:
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf

%ifdef PATM_LOG_PATCHIRET
    push    eax
    push    ecx
    push    edx
    lea     edx, dword [ss:esp+12+4+4]        ;3 dwords + pushed flags + error code -> iret eip
    mov     eax, PATM_ACTION_LOG_GATE_ENTRY
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     eax
%endif

    test    dword [esp+16], X86_EFL_VM
    jnz     PATMTrapErrorCodeNoRing1

    ; make sure the saved CS selector for ring 1 is made 0
    test    dword [esp+12], 2
    jnz     PATMTrapErrorCodeNoRing1
    test    dword [esp+12], 1
    jz      PATMTrapErrorCodeNoRing1
    and     dword [esp+12], dword ~1     ; yasm / nasm dword
PATMTrapErrorCodeNoRing1:

    ; correct EFLAGS on the stack to include the current IOPL
    push    eax
    mov     eax, dword [ss:PATM_ASMFIX_VMFLAGS]
    and     eax, X86_EFL_IOPL
    and     dword [esp+20], ~X86_EFL_IOPL       ; esp+20 = eflags = esp+8+4(efl)+4(error code)+4(eax)
    or      dword [esp+20], eax
    pop     eax

    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMTrapErrorCodeEntryJump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMTrapEntryErrorCode

; Patch record for trap gate entrypoint
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmTrapEntryRecordErrorCode
%ifdef PATM_LOG_PATCHIRET
    PATCHASMRECORD_INIT_JUMP PATMTrapEntryErrorCode, PATMTrapErrorCodeEntryJump, 4
%else
    PATCHASMRECORD_INIT_JUMP PATMTrapEntryErrorCode, PATMTrapErrorCodeEntryJump, 3
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
%ifdef PATM_LOG_PATCHIRET
    DD      PATM_ASMFIX_PENDINGACTION, 0
%endif
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; Trampoline code for interrupt gate entry (without error code on the stack)
;
; esp + 32 - GS         (V86 only)
; esp + 28 - FS         (V86 only)
; esp + 24 - DS         (V86 only)
; esp + 20 - ES         (V86 only)
; esp + 16 - SS         (if transfer to inner ring)
; esp + 12 - ESP        (if transfer to inner ring)
; esp + 8  - EFLAGS
; esp + 4  - CS
; esp      - EIP
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMIntEntry
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf

%ifdef PATM_LOG_PATCHIRET
    push    eax
    push    ecx
    push    edx
    lea     edx, dword [ss:esp+12+4]        ;3 dwords + pushed flags -> iret eip
    mov     eax, PATM_ACTION_LOG_GATE_ENTRY
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     eax
%endif

    test    dword [esp+12], X86_EFL_VM
    jnz     PATMIntNoRing1

    ; make sure the saved CS selector for ring 1 is made 0
    test    dword [esp+8], 2
    jnz     PATMIntNoRing1
    test    dword [esp+8], 1
    jz      PATMIntNoRing1
    and     dword [esp+8], dword ~1     ; yasm / nasm dword
PATMIntNoRing1:

    ; correct EFLAGS on the stack to include the current IOPL
    push    eax
    mov     eax, dword [ss:PATM_ASMFIX_VMFLAGS]
    and     eax, X86_EFL_IOPL
    and     dword [esp+16], ~X86_EFL_IOPL       ; esp+16 = eflags = esp+8+4(efl)+4(eax)
    or      dword [esp+16], eax
    pop     eax

    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMIntEntry

; Patch record for interrupt gate entrypoint
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmIntEntryRecord
%ifdef PATM_LOG_PATCHIRET
    PATCHASMRECORD_INIT PATMIntEntry, 4
%else
    PATCHASMRECORD_INIT PATMIntEntry, 3
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
%ifdef PATM_LOG_PATCHIRET
    DD      PATM_ASMFIX_PENDINGACTION, 0
%endif
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; Trampoline code for interrupt gate entry (*with* error code on the stack)
;
; esp + 36 - GS         (V86 only)
; esp + 32 - FS         (V86 only)
; esp + 28 - DS         (V86 only)
; esp + 24 - ES         (V86 only)
; esp + 20 - SS         (if transfer to inner ring)
; esp + 16 - ESP        (if transfer to inner ring)
; esp + 12 - EFLAGS
; esp + 8  - CS
; esp + 4  - EIP
; esp      - error code
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMIntEntryErrorCode
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf

%ifdef PATM_LOG_PATCHIRET
    push    eax
    push    ecx
    push    edx
    lea     edx, dword [ss:esp+12+4+4]        ;3 dwords + pushed flags + error code -> iret eip
    mov     eax, PATM_ACTION_LOG_GATE_ENTRY
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     eax
%endif

    test    dword [esp+16], X86_EFL_VM
    jnz     PATMIntNoRing1_ErrorCode

    ; make sure the saved CS selector for ring 1 is made 0
    test    dword [esp+12], 2
    jnz     PATMIntNoRing1_ErrorCode
    test    dword [esp+12], 1
    jz      PATMIntNoRing1_ErrorCode
    and     dword [esp+12], dword ~1     ; yasm / nasm dword
PATMIntNoRing1_ErrorCode:

    ; correct EFLAGS on the stack to include the current IOPL
    push    eax
    mov     eax, dword [ss:PATM_ASMFIX_VMFLAGS]
    and     eax, X86_EFL_IOPL
    and     dword [esp+20], ~X86_EFL_IOPL       ; esp+20 = eflags = esp+8+4(efl)+4(eax)+4(error code)
    or      dword [esp+20], eax
    pop     eax

    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMIntEntryErrorCode

; Patch record for interrupt gate entrypoint
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmIntEntryRecordErrorCode
%ifdef PATM_LOG_PATCHIRET
    PATCHASMRECORD_INIT PATMIntEntryErrorCode, 4
%else
    PATCHASMRECORD_INIT PATMIntEntryErrorCode, 3
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
%ifdef PATM_LOG_PATCHIRET
    DD      PATM_ASMFIX_PENDINGACTION, 0
%endif
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; 32 bits Popf replacement that faults when IF remains 0
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMPopf32Replacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ecx
    mov     eax, PATM_ACTION_LOG_POPF_IF1
    test    dword [esp+8], X86_EFL_IF
    jnz     PATMPopf32_Log
    mov     eax, PATM_ACTION_LOG_POPF_IF0

PATMPopf32_Log:
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     ecx
    pop     eax
%endif

    test    dword [esp], X86_EFL_IF
    jnz     PATMPopf32_Ok
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

PATMPopf32_Ok:
    ; Note: we don't allow popf instructions to change the current IOPL; we simply ignore such changes (!!!)
    ; In this particular patch it's rather unlikely the pushf was included, so we have no way to check if the flags on the stack were correctly synced
    ; PATMPopf32Replacement_NoExit is different, because it's only used in IDT and function patches
    or      dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF

    ; if interrupts are pending, then we must go back to the host context to handle them!
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST
    jz      PATMPopf32_Continue

    ; Go to our hypervisor trap handler to dispatch the pending irq
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_EDI], edi
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX | PATM_RESTORE_EDI
    mov     eax, PATM_ACTION_DISPATCH_PENDING_IRQ
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, PATM_ASMFIX_NEXTINSTRADDR

    popfd                   ; restore flags we pushed above (the or instruction changes the flags as well)
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return

PATMPopf32_Continue:
    popfd                   ; restore flags we pushed above
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMPopf32Jump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMPopf32Replacement

; Patch record for 'popfd'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmPopf32Record
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT_JUMP PATMPopf32Replacement, PATMPopf32Jump, 12
%else
    PATCHASMRECORD_INIT_JUMP PATMPopf32Replacement, PATMPopf32Jump, 11
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION,      0
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VM_FORCEDACTIONS,   0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_EDI,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      PATM_ASMFIX_NEXTINSTRADDR,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      0ffffffffh, 0ffffffffh


;
; no need to check the IF flag when popf isn't an exit point of a patch (e.g. function duplication)
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMPopf32Replacement_NoExit
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ecx
    mov     eax, PATM_ACTION_LOG_POPF_IF1
    test    dword [esp+8], X86_EFL_IF
    jnz     PATMPopf32_NoExitLog
    mov     eax, PATM_ACTION_LOG_POPF_IF0

PATMPopf32_NoExitLog:
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     ecx
    pop     eax
%endif
    test    dword [esp], X86_EFL_IF
    jz      PATMPopf32_NoExit_Continue

    ; if interrupts are pending, then we must go back to the host context to handle them!
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST
    jz      PATMPopf32_NoExit_Continue

    ; Go to our hypervisor trap handler to dispatch the pending irq
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_EDI], edi
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX | PATM_RESTORE_EDI
    mov     eax, PATM_ACTION_DISPATCH_PENDING_IRQ
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, PATM_ASMFIX_NEXTINSTRADDR

    pop     dword [ss:PATM_ASMFIX_VMFLAGS]     ; restore flags now (the or instruction changes the flags as well)
    push    dword [ss:PATM_ASMFIX_VMFLAGS]
    popfd

    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return

PATMPopf32_NoExit_Continue:
    pop     dword [ss:PATM_ASMFIX_VMFLAGS]
    push    dword [ss:PATM_ASMFIX_VMFLAGS]
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMPopf32Replacement_NoExit

; Patch record for 'popfd'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmPopf32Record_NoExit
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT PATMPopf32Replacement_NoExit, 14
%else
    PATCHASMRECORD_INIT PATMPopf32Replacement_NoExit, 13
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION,      0
%endif
    DD      PATM_ASMFIX_VM_FORCEDACTIONS,   0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_EDI,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      PATM_ASMFIX_NEXTINSTRADDR,      0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      0ffffffffh, 0ffffffffh


;
; 16 bits Popf replacement that faults when IF remains 0
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMPopf16Replacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    test    word [esp], X86_EFL_IF
    jnz     PATMPopf16_Ok
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

PATMPopf16_Ok:
    ; if interrupts are pending, then we must go back to the host context to handle them!
    ; @note we destroy the flags here, but that should really not matter (PATM_INT3 case)
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST
    jz      PATMPopf16_Continue
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

PATMPopf16_Continue:

    pop     word [ss:PATM_ASMFIX_VMFLAGS]
    push    word [ss:PATM_ASMFIX_VMFLAGS]
    and     dword [ss:PATM_ASMFIX_VMFLAGS], PATM_VIRTUAL_FLAGS_MASK
    or      dword [ss:PATM_ASMFIX_VMFLAGS], PATM_VIRTUAL_FLAGS_MASK

    DB      0x66    ; size override
    popf    ;after the and and or operations!! (flags must be preserved)
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1

    DB      0xE9
PATMPopf16Jump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMPopf16Replacement

; Patch record for 'popf'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmPopf16Record
    PATCHASMRECORD_INIT_JUMP PATMPopf16Replacement, PATMPopf16Jump, 9
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      PATM_ASMFIX_VM_FORCEDACTIONS, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      0ffffffffh, 0ffffffffh


;
; 16 bits Popf replacement that faults when IF remains 0
; @todo not necessary to fault in that case (see 32 bits version)
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMPopf16Replacement_NoExit
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    test    word [esp], X86_EFL_IF
    jnz     PATMPopf16_Ok_NoExit
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

PATMPopf16_Ok_NoExit:
    ; if interrupts are pending, then we must go back to the host context to handle them!
    ; @note we destroy the flags here, but that should really not matter (PATM_INT3 case)
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST
    jz      PATMPopf16_Continue_NoExit
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

PATMPopf16_Continue_NoExit:

    pop     word [ss:PATM_ASMFIX_VMFLAGS]
    push    word [ss:PATM_ASMFIX_VMFLAGS]
    and     dword [ss:PATM_ASMFIX_VMFLAGS], PATM_VIRTUAL_FLAGS_MASK
    or      dword [ss:PATM_ASMFIX_VMFLAGS], PATM_VIRTUAL_FLAGS_MASK

    DB      0x66    ; size override
    popf    ;after the and and or operations!! (flags must be preserved)
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMPopf16Replacement_NoExit

; Patch record for 'popf'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmPopf16Record_NoExit
    PATCHASMRECORD_INIT PATMPopf16Replacement_NoExit, 9
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      PATM_ASMFIX_VM_FORCEDACTIONS, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_VMFLAGS,          0
    DD      PATM_ASMFIX_INTERRUPTFLAG,    0
    DD      0ffffffffh, 0ffffffffh


;
;
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMPushf32Replacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushfd
%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ecx
    mov     eax, PATM_ACTION_LOG_PUSHF
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     ecx
    pop     eax
%endif

    pushfd
    push    eax
    mov     eax, dword [esp+8]
    and     eax, PATM_FLAGS_MASK
    or      eax, dword [ss:PATM_ASMFIX_VMFLAGS]
    mov     dword [esp+8], eax
    pop     eax
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMPushf32Replacement

; Patch record for 'pushfd'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmPushf32Record
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT PATMPushf32Replacement, 4
%else
    PATCHASMRECORD_INIT PATMPushf32Replacement, 3
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION, 0
%endif
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
;
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMPushf16Replacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    DB      0x66    ; size override
    pushf
    DB      0x66    ; size override
    pushf
    push    eax
    xor     eax, eax
    mov     ax, word [esp+6]
    and     eax, PATM_FLAGS_MASK
    or      eax, dword [ss:PATM_ASMFIX_VMFLAGS]
    mov     word [esp+6], ax
    pop     eax

    DB      0x66    ; size override
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC     PATMPushf16Replacement

; Patch record for 'pushf'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmPushf16Record
    PATCHASMRECORD_INIT PATMPushf16Replacement, 3
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
;
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMPushCSReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    push    cs
    pushfd

    test    dword [esp+4], 2
    jnz     pushcs_notring1

    ; change dpl from 1 to 0
    and     dword [esp+4], dword ~1     ; yasm / nasm dword

pushcs_notring1:
    popfd

    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMPushCSJump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMPushCSReplacement

; Patch record for 'push cs'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmPushCSRecord
    PATCHASMRECORD_INIT_JUMP PATMPushCSReplacement, PATMPushCSJump, 2
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
;
;****************************************************
; Abstract:
;
; if eflags.NT==0 && iretstack.eflags.VM==0 && iretstack.eflags.IOPL==0
; then
;     if return to ring 0 (iretstack.new_cs & 3 == 0)
;     then
;          if iretstack.new_eflags.IF == 1 && iretstack.new_eflags.IOPL == 0
;          then
;              iretstack.new_cs |= 1
;          else
;              int 3
;     endif
;     uVMFlags &= ~X86_EFL_IF
;     iret
; else
;     int 3
;****************************************************
;
; Stack:
;
; esp + 32 - GS         (V86 only)
; esp + 28 - FS         (V86 only)
; esp + 24 - DS         (V86 only)
; esp + 20 - ES         (V86 only)
; esp + 16 - SS         (if transfer to outer ring)
; esp + 12 - ESP        (if transfer to outer ring)
; esp + 8  - EFLAGS
; esp + 4  - CS
; esp      - EIP
;;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMIretReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushfd

%ifdef PATM_LOG_PATCHIRET
    push    eax
    push    ecx
    push    edx
    lea     edx, dword [ss:esp+12+4]        ;3 dwords + pushed flags -> iret eip
    mov     eax, PATM_ACTION_LOG_IRET
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     eax
%endif

    test    dword [esp], X86_EFL_NT
    jnz near iret_fault1

    ; we can't do an iret to v86 code, as we run with CPL=1. The iret would attempt a protected mode iret and (most likely) fault.
    test    dword [esp+12], X86_EFL_VM
    jnz     near iret_return_to_v86

    ;;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ;;@todo: not correct for iret back to ring 2!!!!!
    ;;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    test    dword [esp+8], 2
    jnz     iret_notring0

    test    dword [esp+12], X86_EFL_IF
    jz near iret_clearIF

    ; force ring 1 CS RPL
    or      dword [esp+8], 1        ;-> @todo we leave traces or raw mode if we jump back to the host context to handle pending interrupts! (below)
iret_notring0:

; if interrupts are pending, then we must go back to the host context to handle them!
; Note: This is very important as pending pic interrupts can be overridden by apic interrupts if we don't check early enough (Fedora 5 boot)
; @@todo fix this properly, so we can dispatch pending interrupts in GC
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC
    jz      iret_continue

; Go to our hypervisor trap handler to dispatch the pending irq
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_EDI], edi
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX | PATM_RESTORE_EDI
    mov     eax, PATM_ACTION_PENDING_IRQ_AFTER_IRET
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, PATM_ASMFIX_CURINSTRADDR

    popfd
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return

iret_continue :
    ; This section must *always* be executed (!!)
    ; Extract the IOPL from the return flags, save them to our virtual flags and
    ; put them back to zero
    ; @note we assume iretd doesn't fault!!!
    push    eax
    mov     eax, dword [esp+16]
    and     eax, X86_EFL_IOPL
    and     dword [ss:PATM_ASMFIX_VMFLAGS], ~X86_EFL_IOPL
    or      dword [ss:PATM_ASMFIX_VMFLAGS], eax
    pop     eax
    and     dword [esp+12], ~X86_EFL_IOPL

    ; Set IF again; below we make sure this won't cause problems.
    or      dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF

    ; make sure iret is executed fully (including the iret below; cli ... iret can otherwise be interrupted)
    mov     dword [ss:PATM_ASMFIX_INHIBITIRQADDR], PATM_ASMFIX_CURINSTRADDR

    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    iretd
    PATM_INT3

iret_fault:
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

iret_fault1:
    nop
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

iret_clearIF:
    push    dword [esp+4]           ; eip to return to
    pushfd
    push    eax
    push    PATM_ASMFIX_FIXUP
    DB      0E8h                    ; call
    DD      PATM_ASMFIX_IRET_FUNCTION
    add     esp, 4                  ; pushed address of jump table

    cmp     eax, 0
    je      near iret_fault3

    mov     dword [esp+12+4], eax   ; stored eip in iret frame
    pop     eax
    popfd
    add     esp, 4                  ; pushed eip

    ; always ring 0 return -> change to ring 1 (CS in iret frame)
    or      dword [esp+8], 1

    ; This section must *always* be executed (!!)
    ; Extract the IOPL from the return flags, save them to our virtual flags and
    ; put them back to zero
    push    eax
    mov     eax, dword [esp+16]
    and     eax, X86_EFL_IOPL
    and     dword [ss:PATM_ASMFIX_VMFLAGS], ~X86_EFL_IOPL
    or      dword [ss:PATM_ASMFIX_VMFLAGS], eax
    pop     eax
    and     dword [esp+12], ~X86_EFL_IOPL

    ; Clear IF
    and     dword [ss:PATM_ASMFIX_VMFLAGS], ~X86_EFL_IF
    popfd

                                                ; the patched destination code will set PATM_ASMFIX_INTERRUPTFLAG after the return!
    iretd

iret_return_to_v86:
    test    dword [esp+12], X86_EFL_IF
    jz      iret_fault

    ; Go to our hypervisor trap handler to perform the iret to v86 code
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX
    mov     eax, PATM_ACTION_DO_V86_IRET
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC

    popfd

    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return


iret_fault3:
    pop     eax
    popfd
    add     esp, 4                  ; pushed eip
    jmp     iret_fault

align   4
PATMIretTable:
    DW      PATM_MAX_JUMPTABLE_ENTRIES          ; nrSlots
    DW      0                                   ; ulInsertPos
    DD      0                                   ; cAddresses
    TIMES PATCHJUMPTABLE_SIZE DB 0              ; lookup slots

ENDPROC     PATMIretReplacement

; Patch record for 'iretd'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmIretRecord
%ifdef PATM_LOG_PATCHIRET
    PATCHASMRECORD_INIT PATMIretReplacement, 26
%else
    PATCHASMRECORD_INIT PATMIretReplacement, 25
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
%ifdef PATM_LOG_PATCHIRET
    DD      PATM_ASMFIX_PENDINGACTION,      0
%endif
    DD      PATM_ASMFIX_VM_FORCEDACTIONS,   0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_EDI,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      PATM_ASMFIX_CURINSTRADDR,       0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_INHIBITIRQADDR,     0
    DD      PATM_ASMFIX_CURINSTRADDR,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_FIXUP,              PATMIretTable - NAME(PATMIretReplacement)
    DD      PATM_ASMFIX_IRET_FUNCTION,      0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      0ffffffffh,              0ffffffffh


;
;
;****************************************************
; Abstract:
;
; if eflags.NT==0 && iretstack.eflags.VM==0 && iretstack.eflags.IOPL==0
; then
;     if return to ring 0 (iretstack.new_cs & 3 == 0)
;     then
;          if iretstack.new_eflags.IF == 1 && iretstack.new_eflags.IOPL == 0
;          then
;              iretstack.new_cs |= 1
;          else
;              int 3
;     endif
;     uVMFlags &= ~X86_EFL_IF
;     iret
; else
;     int 3
;****************************************************
;
; Stack:
;
; esp + 32 - GS         (V86 only)
; esp + 28 - FS         (V86 only)
; esp + 24 - DS         (V86 only)
; esp + 20 - ES         (V86 only)
; esp + 16 - SS         (if transfer to outer ring)
; esp + 12 - ESP        (if transfer to outer ring)
; esp + 8  - EFLAGS
; esp + 4  - CS
; esp      - EIP
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMIretRing1Replacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushfd

%ifdef PATM_LOG_PATCHIRET
    push    eax
    push    ecx
    push    edx
    lea     edx, dword [ss:esp+12+4]        ;3 dwords + pushed flags -> iret eip
    mov     eax, PATM_ACTION_LOG_IRET
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     eax
%endif

    test    dword [esp], X86_EFL_NT
    jnz     near iretring1_fault1

    ; we can't do an iret to v86 code, as we run with CPL=1. The iret would attempt a protected mode iret and (most likely) fault.
    test    dword [esp+12], X86_EFL_VM
    jnz     near iretring1_return_to_v86

    ;;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ;;@todo: not correct for iret back to ring 2!!!!!
    ;;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    test    dword [esp+8], 2
    jnz     iretring1_checkpendingirq

    test    dword [esp+12], X86_EFL_IF
    jz      near iretring1_clearIF

iretring1_checkpendingirq:

; if interrupts are pending, then we must go back to the host context to handle them!
; Note: This is very important as pending pic interrupts can be overridden by apic interrupts if we don't check early enough (Fedora 5 boot)
; @@todo fix this properly, so we can dispatch pending interrupts in GC
    test    dword [ss:PATM_ASMFIX_VM_FORCEDACTIONS], VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC
    jz      iretring1_continue

; Go to our hypervisor trap handler to dispatch the pending irq
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_EDI], edi
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX | PATM_RESTORE_EDI
    mov     eax, PATM_ACTION_PENDING_IRQ_AFTER_IRET
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, PATM_ASMFIX_CURINSTRADDR

    popfd
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return

iretring1_continue:

    test    dword [esp+8], 2
    jnz     iretring1_notring01

    test    dword [esp+8], 1
    jz      iretring1_ring0

    ; ring 1 return change CS & SS RPL to 2 from 1
    and     dword [esp+8], ~1       ; CS
    or      dword [esp+8], 2

    and     dword [esp+20], ~1      ; SS
    or      dword [esp+20], 2

    jmp     short iretring1_notring01
iretring1_ring0:
    ; force ring 1 CS RPL
    or      dword [esp+8], 1

iretring1_notring01:
    ; This section must *always* be executed (!!)
    ; Extract the IOPL from the return flags, save them to our virtual flags and
    ; put them back to zero
    ; @note we assume iretd doesn't fault!!!
    push    eax
    mov     eax, dword [esp+16]
    and     eax, X86_EFL_IOPL
    and     dword [ss:PATM_ASMFIX_VMFLAGS], ~X86_EFL_IOPL
    or      dword [ss:PATM_ASMFIX_VMFLAGS], eax
    pop     eax
    and     dword [esp+12], ~X86_EFL_IOPL

    ; Set IF again; below we make sure this won't cause problems.
    or      dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF

    ; make sure iret is executed fully (including the iret below; cli ... iret can otherwise be interrupted)
    mov     dword [ss:PATM_ASMFIX_INHIBITIRQADDR], PATM_ASMFIX_CURINSTRADDR

    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    iretd
    PATM_INT3

iretring1_fault:
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

iretring1_fault1:
    nop
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

iretring1_clearIF:
    push    dword [esp+4]           ; eip to return to
    pushfd
    push    eax
    push    PATM_ASMFIX_FIXUP
    DB      0E8h                    ; call
    DD      PATM_ASMFIX_IRET_FUNCTION
    add     esp, 4                  ; pushed address of jump table

    cmp     eax, 0
    je      near iretring1_fault3

    mov     dword [esp+12+4], eax   ; stored eip in iret frame
    pop     eax
    popfd
    add     esp, 4                  ; pushed eip

    ; This section must *always* be executed (!!)
    ; Extract the IOPL from the return flags, save them to our virtual flags and
    ; put them back to zero
    push    eax
    mov     eax, dword [esp+16]
    and     eax, X86_EFL_IOPL
    and     dword [ss:PATM_ASMFIX_VMFLAGS], ~X86_EFL_IOPL
    or      dword [ss:PATM_ASMFIX_VMFLAGS], eax
    pop     eax
    and     dword [esp+12], ~X86_EFL_IOPL

    ; Clear IF
    and     dword [ss:PATM_ASMFIX_VMFLAGS], ~X86_EFL_IF
    popfd

    test    dword [esp+8], 1
    jz      iretring1_clearIF_ring0

    ; ring 1 return change CS & SS RPL to 2 from 1
    and     dword [esp+8], ~1       ; CS
    or      dword [esp+8], 2

    and     dword [esp+20], ~1      ; SS
    or      dword [esp+20], 2
                                                ; the patched destination code will set PATM_ASMFIX_INTERRUPTFLAG after the return!
    iretd

iretring1_clearIF_ring0:
    ; force ring 1 CS RPL
    or      dword [esp+8], 1
                                                ; the patched destination code will set PATM_ASMFIX_INTERRUPTFLAG after the return!
    iretd

iretring1_return_to_v86:
    test    dword [esp+12], X86_EFL_IF
    jz      iretring1_fault

    ; Go to our hypervisor trap handler to perform the iret to v86 code
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], eax
    mov     dword [ss:PATM_ASMFIX_TEMP_ECX], ecx
    mov     dword [ss:PATM_ASMFIX_TEMP_RESTORE_FLAGS], PATM_RESTORE_EAX | PATM_RESTORE_ECX
    mov     eax, PATM_ACTION_DO_V86_IRET
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], eax
    mov     ecx, PATM_ACTION_MAGIC

    popfd

    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    ; does not return


iretring1_fault3:
    pop     eax
    popfd
    add     esp, 4                  ; pushed eip
    jmp     iretring1_fault

align   4
PATMIretRing1Table:
    DW      PATM_MAX_JUMPTABLE_ENTRIES          ; nrSlots
    DW      0                                   ; ulInsertPos
    DD      0                                   ; cAddresses
    TIMES PATCHJUMPTABLE_SIZE DB 0              ; lookup slots

ENDPROC     PATMIretRing1Replacement

; Patch record for 'iretd'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmIretRing1Record
%ifdef PATM_LOG_PATCHIRET
    PATCHASMRECORD_INIT PATMIretRing1Replacement, 26
%else
    PATCHASMRECORD_INIT PATMIretRing1Replacement, 25
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
%ifdef PATM_LOG_PATCHIRET
    DD      PATM_ASMFIX_PENDINGACTION,      0
%endif
    DD      PATM_ASMFIX_VM_FORCEDACTIONS,   0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_EDI,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      PATM_ASMFIX_CURINSTRADDR,       0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_INHIBITIRQADDR,     0
    DD      PATM_ASMFIX_CURINSTRADDR,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_INTERRUPTFLAG,      0
    DD      PATM_ASMFIX_FIXUP,              PATMIretRing1Table - NAME(PATMIretRing1Replacement)
    DD      PATM_ASMFIX_IRET_FUNCTION,      0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_VMFLAGS,            0
    DD      PATM_ASMFIX_TEMP_EAX,           0
    DD      PATM_ASMFIX_TEMP_ECX,           0
    DD      PATM_ASMFIX_TEMP_RESTORE_FLAGS, 0
    DD      PATM_ASMFIX_PENDINGACTION,      0
    DD      0ffffffffh,              0ffffffffh


;
; global function for implementing 'iret' to code with IF cleared
;
; Caller is responsible for right stack layout
;  + 16 original return address
;  + 12 eflags
;  +  8 eax
;  +  4 Jump table address
;( +  0 return address )
;
; @note assumes PATM_ASMFIX_INTERRUPTFLAG is zero
; @note assumes it can trash eax and eflags
;
; @returns eax=0 on failure
;          otherwise return address in eax
;
; @note NEVER change this without bumping the SSM version
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMIretFunction
    push    ecx
    push    edx
    push    edi

    ; Event order:
    ; 1) Check if the return patch address can be found in the lookup table
    ; 2) Query return patch address from the hypervisor

    ; 1) Check if the return patch address can be found in the lookup table
    mov     edx, dword [esp+12+16]  ; pushed target address

    xor     eax, eax                ; default result -> nothing found
    mov     edi, dword [esp+12+4]  ; jump table
    mov     ecx, [ss:edi + PATCHJUMPTABLE.cAddresses]
    cmp     ecx, 0
    je      near PATMIretFunction_AskHypervisor

PATMIretFunction_SearchStart:
    cmp     [ss:edi + PATCHJUMPTABLE.Slot_pInstrGC + eax*8], edx        ; edx = GC address to search for
    je      near PATMIretFunction_SearchHit
    inc     eax
    cmp     eax, ecx
    jl      near PATMIretFunction_SearchStart

PATMIretFunction_AskHypervisor:
    ; 2) Query return patch address from the hypervisor
    ; @todo private ugly interface, since we have nothing generic at the moment
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOOKUP_ADDRESS
    mov     eax, PATM_ACTION_LOOKUP_ADDRESS
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, dword [esp+12+4]               ; jump table address
    mov     edx, dword [esp+12+16]              ; original return address
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    jmp     near PATMIretFunction_SearchEnd

PATMIretFunction_SearchHit:
    mov     eax, [ss:edi + PATCHJUMPTABLE.Slot_pRelPatchGC + eax*8]        ; found a match!
    ;@note can be zero, so the next check is required!!

PATMIretFunction_SearchEnd:
    cmp     eax, 0
    jz      PATMIretFunction_Failure

    add     eax, PATM_ASMFIX_PATCHBASE

    pop     edi
    pop     edx
    pop     ecx
    ret

PATMIretFunction_Failure:
    ;signal error
    xor     eax, eax
    pop     edi
    pop     edx
    pop     ecx
    ret
ENDPROC     PATMIretFunction

BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmIretFunctionRecord
    PATCHASMRECORD_INIT PATMIretFunction, 2
    DD      PATM_ASMFIX_PENDINGACTION, 0
    DD      PATM_ASMFIX_PATCHBASE,     0
    DD      0ffffffffh, 0ffffffffh


;
; PATMCpuidReplacement
;
; Calls a helper function that does the job.
;
; This way we can change the CPUID structures and how we organize them without
; breaking patches.  It also saves a bit of memory for patch code and fixups.
;
BEGIN_PATCH g_patmCpuidRecord, PATMCpuidReplacement
    not     dword [esp-32]              ; probe stack before starting
    not     dword [esp-32]

    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
PATCH_FIXUP PATM_ASMFIX_INTERRUPTFLAG
    pushf

    db      0e8h                        ; call
    dd      PATM_ASMFIX_PATCH_HLP_CPUM_CPUID
PATCH_FIXUP PATM_ASMFIX_PATCH_HLP_CPUM_CPUID

    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
PATCH_FIXUP PATM_ASMFIX_INTERRUPTFLAG
END_PATCH g_patmCpuidRecord, PATMCpuidReplacement


;
;
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMJEcxReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushfd
PATMJEcxSizeOverride:
    DB      0x90             ; nop
    cmp     ecx, dword 0                ; yasm / nasm dword
    jnz     PATMJEcxContinue

    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMJEcxJump:
    DD      PATM_ASMFIX_JUMPDELTA

PATMJEcxContinue:
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC PATMJEcxReplacement

; Patch record for 'JEcx'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmJEcxRecord
    PATCHASMRECORD_INIT_EX PATMJEcxReplacement, , PATMJEcxJump, PATMJEcxSizeOverride, 3
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
;
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMLoopReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushfd
PATMLoopSizeOverride:
    DB      0x90             ; nop
    dec     ecx
    jz      PATMLoopContinue

    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMLoopJump:
    DD      PATM_ASMFIX_JUMPDELTA

PATMLoopContinue:
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC PATMLoopReplacement

; Patch record for 'Loop'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmLoopRecord
    PATCHASMRECORD_INIT_EX PATMLoopReplacement, , PATMLoopJump, PATMLoopSizeOverride, 3
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; jump if ZF=1 AND (E)CX != 0
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMLoopZReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    jnz     NAME(PATMLoopZReplacement_EndProc)
    pushfd
PATMLoopZSizeOverride:
    DB      0x90             ; nop
    dec     ecx
    jz      PATMLoopZContinue

    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMLoopZJump:
    DD      PATM_ASMFIX_JUMPDELTA

PATMLoopZContinue:
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC PATMLoopZReplacement

; Patch record for 'Loopz'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmLoopZRecord
    PATCHASMRECORD_INIT_EX PATMLoopZReplacement, , PATMLoopZJump, PATMLoopZSizeOverride, 3
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; jump if ZF=0 AND (E)CX != 0
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMLoopNZReplacement
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    jz      NAME(PATMLoopNZReplacement_EndProc)
    pushfd
PATMLoopNZSizeOverride:
    DB      0x90             ; nop
    dec     ecx
    jz      PATMLoopNZContinue

    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMLoopNZJump:
    DD      PATM_ASMFIX_JUMPDELTA

PATMLoopNZContinue:
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
ENDPROC PATMLoopNZReplacement

; Patch record for 'LoopNZ'
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmLoopNZRecord
    PATCHASMRECORD_INIT_EX PATMLoopNZReplacement, , PATMLoopNZJump, PATMLoopNZSizeOverride, 3
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; Global patch function for indirect calls
; Caller is responsible for clearing PATM_ASMFIX_INTERRUPTFLAG and doing:
;  + 20 push    [pTargetGC]
;  + 16 pushfd
;  + 12 push    [JumpTableAddress]
;  +  8 push    [PATMRelReturnAddress]
;  +  4 push    [GuestReturnAddress]
;( +  0 return address )
;
; @note NEVER change this without bumping the SSM version
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC PATMLookupAndCall
    push    eax
    push    edx
    push    edi
    push    ecx

    mov     eax, dword [esp+16+4]                   ; guest return address
    mov     dword [ss:PATM_ASMFIX_CALL_RETURN_ADDR], eax                               ; temporary storage

    mov     edx, dword [esp+16+20]  ; pushed target address

    xor     eax, eax                ; default result -> nothing found
    mov     edi, dword [esp+16+12]  ; jump table
    mov     ecx, [ss:edi + PATCHJUMPTABLE.cAddresses]
    cmp     ecx, 0
    je      near PATMLookupAndCall_QueryPATM

PATMLookupAndCall_SearchStart:
    cmp     [ss:edi + PATCHJUMPTABLE.Slot_pInstrGC + eax*8], edx        ; edx = GC address to search for
    je      near PATMLookupAndCall_SearchHit
    inc     eax
    cmp     eax, ecx
    jl      near PATMLookupAndCall_SearchStart

PATMLookupAndCall_QueryPATM:
    ; nothing found -> let our trap handler try to find it
    ; @todo private ugly interface, since we have nothing generic at the moment
    lock    or  dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOOKUP_ADDRESS
    mov     eax, PATM_ACTION_LOOKUP_ADDRESS
    mov     ecx, PATM_ACTION_MAGIC
    ; edx = GC address to find
    ; edi = jump table address
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)

    jmp     near PATMLookupAndCall_SearchEnd

PATMLookupAndCall_Failure:
    ; return to caller; it must raise an error, due to patch to guest address translation (remember that there's only one copy of this code block).
    pop     ecx
    pop     edi
    pop     edx
    pop     eax
    ret

PATMLookupAndCall_SearchHit:
    mov     eax, [ss:edi + PATCHJUMPTABLE.Slot_pRelPatchGC + eax*8]        ; found a match!

    ;@note can be zero, so the next check is required!!

PATMLookupAndCall_SearchEnd:
    cmp     eax, 0
    je      near PATMLookupAndCall_Failure

    mov     ecx, eax                            ; ECX = target address (relative!)
    add     ecx, PATM_ASMFIX_PATCHBASE                 ; Make it absolute

    mov     edx, dword PATM_ASMFIX_STACKPTR
    cmp     dword [ss:edx], PATM_STACK_SIZE
    ja      near PATMLookupAndCall_Failure                    ; should never happen actually!!!
    cmp     dword [ss:edx], 0
    je      near PATMLookupAndCall_Failure                    ; no more room

    ; save the patch return address on our private stack
    sub     dword [ss:edx], 4                   ; sizeof(RTGCPTR)
    mov     eax, dword PATM_ASMFIX_STACKBASE
    add     eax, dword [ss:edx]                 ; stack base + stack position
    mov     edi, dword [esp+16+8]               ; PATM return address
    mov     dword [ss:eax], edi                 ; relative address of patch return (instruction following this block)

    ; save the original return address as well (checked by ret to make sure the guest hasn't messed around with the stack)
    mov     edi, dword PATM_ASMFIX_STACKBASE_GUEST
    add     edi, dword [ss:edx]                 ; stack base (guest) + stack position
    mov     eax, dword [esp+16+4]               ; guest return address
    mov     dword [ss:edi], eax

    mov     dword [ss:PATM_ASMFIX_CALL_PATCH_TARGET_ADDR], ecx       ; temporarily store the target address
    pop     ecx
    pop     edi
    pop     edx
    pop     eax
    add     esp, 24                             ; parameters + return address pushed by caller (changes the flags, but that shouldn't matter)

%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ecx
    push    edx
    lea     edx, [esp + 12 - 4]                 ; stack address to store return address
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOG_CALL
    mov     eax, PATM_ACTION_LOG_CALL
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     eax
%endif

    push    dword [ss:PATM_ASMFIX_CALL_RETURN_ADDR]   ; push original guest return address

    ; the called function will set PATM_ASMFIX_INTERRUPTFLAG (!!)
    jmp     dword [ss:PATM_ASMFIX_CALL_PATCH_TARGET_ADDR]
    ; returning here -> do not add code here or after the jmp!!!!!
ENDPROC PATMLookupAndCall

; Patch record for indirect calls and jumps
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmLookupAndCallRecord
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT PATMLookupAndCall, 10
%else
    PATCHASMRECORD_INIT PATMLookupAndCall, 9
%endif
    DD      PATM_ASMFIX_CALL_RETURN_ADDR,       0
    DD      PATM_ASMFIX_PENDINGACTION,          0
    DD      PATM_ASMFIX_PATCHBASE,              0
    DD      PATM_ASMFIX_STACKPTR,               0
    DD      PATM_ASMFIX_STACKBASE,              0
    DD      PATM_ASMFIX_STACKBASE_GUEST,        0
    DD      PATM_ASMFIX_CALL_PATCH_TARGET_ADDR, 0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION,          0
%endif
    DD      PATM_ASMFIX_CALL_RETURN_ADDR,       0
    DD      PATM_ASMFIX_CALL_PATCH_TARGET_ADDR, 0
    DD      0ffffffffh, 0ffffffffh


;
; Global patch function for indirect jumps
; Caller is responsible for clearing PATM_ASMFIX_INTERRUPTFLAG and doing:
;  +  8 push    [pTargetGC]
;  +  4 push    [JumpTableAddress]
;( +  0 return address )
; And saving eflags in PATM_ASMFIX_TEMP_EFLAGS
;
; @note NEVER change this without bumping the SSM version
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC PATMLookupAndJump
    push    eax
    push    edx
    push    edi
    push    ecx

    mov     edx, dword [esp+16+8]  ; pushed target address

    xor     eax, eax                ; default result -> nothing found
    mov     edi, dword [esp+16+4]  ; jump table
    mov     ecx, [ss:edi + PATCHJUMPTABLE.cAddresses]
    cmp     ecx, 0
    je      near PATMLookupAndJump_QueryPATM

PATMLookupAndJump_SearchStart:
    cmp     [ss:edi + PATCHJUMPTABLE.Slot_pInstrGC + eax*8], edx        ; edx = GC address to search for
    je      near PATMLookupAndJump_SearchHit
    inc     eax
    cmp     eax, ecx
    jl      near PATMLookupAndJump_SearchStart

PATMLookupAndJump_QueryPATM:
    ; nothing found -> let our trap handler try to find it
    ; @todo private ugly interface, since we have nothing generic at the moment
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOOKUP_ADDRESS
    mov     eax, PATM_ACTION_LOOKUP_ADDRESS
    mov     ecx, PATM_ACTION_MAGIC
    ; edx = GC address to find
    ; edi = jump table address
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)

    jmp     near PATMLookupAndJump_SearchEnd

PATMLookupAndJump_Failure:
    ; return to caller; it must raise an error, due to patch to guest address translation (remember that there's only one copy of this code block).
    pop     ecx
    pop     edi
    pop     edx
    pop     eax
    ret

PATMLookupAndJump_SearchHit:
    mov     eax, [ss:edi + PATCHJUMPTABLE.Slot_pRelPatchGC + eax*8]        ; found a match!

    ;@note can be zero, so the next check is required!!

PATMLookupAndJump_SearchEnd:
    cmp     eax, 0
    je      near PATMLookupAndJump_Failure

    mov     ecx, eax                            ; ECX = target address (relative!)
    add     ecx, PATM_ASMFIX_PATCHBASE                 ; Make it absolute

    ; save jump patch target
    mov     dword [ss:PATM_ASMFIX_TEMP_EAX], ecx
    pop     ecx
    pop     edi
    pop     edx
    pop     eax
    add     esp, 12                             ; parameters + return address pushed by caller
    ; restore flags (just to be sure)
    push    dword [ss:PATM_ASMFIX_TEMP_EFLAGS]
    popfd

    ; the jump destination will set PATM_ASMFIX_INTERRUPTFLAG (!!)
    jmp     dword [ss:PATM_ASMFIX_TEMP_EAX]                      ; call duplicated patch destination address
ENDPROC PATMLookupAndJump

; Patch record for indirect calls and jumps
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmLookupAndJumpRecord
    PATCHASMRECORD_INIT PATMLookupAndJump, 5
    DD      PATM_ASMFIX_PENDINGACTION, 0
    DD      PATM_ASMFIX_PATCHBASE,     0
    DD      PATM_ASMFIX_TEMP_EAX,      0
    DD      PATM_ASMFIX_TEMP_EFLAGS,   0
    DD      PATM_ASMFIX_TEMP_EAX,      0
    DD      0ffffffffh, 0ffffffffh


; Patch function for static calls
; @note static calls have only one lookup slot!
; Caller is responsible for clearing PATM_ASMFIX_INTERRUPTFLAG and adding:
;   push    [pTargetGC]
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC PATMCall
    pushfd
    push    PATM_ASMFIX_FIXUP              ; fixup for jump table below
    push    PATM_ASMFIX_PATCHNEXTBLOCK
    push    PATM_ASMFIX_RETURNADDR
    DB      0E8h                    ; call
    DD      PATM_ASMFIX_LOOKUP_AND_CALL_FUNCTION
    ; we only return in case of a failure
    add     esp, 12                 ; pushed address of jump table
    popfd
    add     esp, 4                  ; pushed by caller (changes the flags, but that shouldn't matter (@todo))
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3
%ifdef DEBUG
    ; for disassembly
    jmp     NAME(PATMCall_EndProc)
%endif

align   4
PATMCallTable:
    DW      1                                   ; nrSlots
    DW      0                                   ; ulInsertPos
    DD      0                                   ; cAddresses
    TIMES PATCHDIRECTJUMPTABLE_SIZE DB 0        ; only one lookup slot

    ; returning here -> do not add code here or after the jmp!!!!!
ENDPROC PATMCall

; Patch record for direct calls
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmCallRecord
    PATCHASMRECORD_INIT PATMCall, 5
    DD      PATM_ASMFIX_FIXUP,                     PATMCallTable - NAME(PATMCall)
    DD      PATM_ASMFIX_PATCHNEXTBLOCK,            0
    DD      PATM_ASMFIX_RETURNADDR,                0
    DD      PATM_ASMFIX_LOOKUP_AND_CALL_FUNCTION,  0
    DD      PATM_ASMFIX_INTERRUPTFLAG,             0
    DD      0ffffffffh, 0ffffffffh


; Patch function for indirect calls
; Caller is responsible for clearing PATM_ASMFIX_INTERRUPTFLAG and adding:
;   push    [pTargetGC]
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC PATMCallIndirect
    pushfd
    push    PATM_ASMFIX_FIXUP              ; fixup for jump table below
    push    PATM_ASMFIX_PATCHNEXTBLOCK
    push    PATM_ASMFIX_RETURNADDR
    DB      0E8h                    ; call
    DD      PATM_ASMFIX_LOOKUP_AND_CALL_FUNCTION
    ; we only return in case of a failure
    add     esp, 12                 ; pushed address of jump table
    popfd
    add     esp, 4                  ; pushed by caller (changes the flags, but that shouldn't matter (@todo))
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3
%ifdef DEBUG
    ; for disassembly
    jmp     NAME(PATMCallIndirect_EndProc)
%endif

align   4
PATMCallIndirectTable:
    DW      PATM_MAX_JUMPTABLE_ENTRIES          ; nrSlots
    DW      0                                   ; ulInsertPos
    DD      0                                   ; cAddresses
    TIMES PATCHJUMPTABLE_SIZE DB 0              ; lookup slots

    ; returning here -> do not add code here or after the jmp!!!!!
ENDPROC PATMCallIndirect

; Patch record for indirect calls
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmCallIndirectRecord
    PATCHASMRECORD_INIT PATMCallIndirect, 5
    DD      PATM_ASMFIX_FIXUP,                     PATMCallIndirectTable - NAME(PATMCallIndirect)
    DD      PATM_ASMFIX_PATCHNEXTBLOCK,            0
    DD      PATM_ASMFIX_RETURNADDR,                0
    DD      PATM_ASMFIX_LOOKUP_AND_CALL_FUNCTION,  0
    DD      PATM_ASMFIX_INTERRUPTFLAG,             0
    DD      0ffffffffh, 0ffffffffh


;
; Patch function for indirect jumps
; Caller is responsible for clearing PATM_ASMFIX_INTERRUPTFLAG and adding:
;   push    [pTargetGC]
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC PATMJumpIndirect
    ; save flags (just to be sure)
    pushfd
    pop     dword [ss:PATM_ASMFIX_TEMP_EFLAGS]

    push    PATM_ASMFIX_FIXUP              ; fixup for jump table below
    DB      0E8h                    ; call
    DD      PATM_ASMFIX_LOOKUP_AND_JUMP_FUNCTION
    ; we only return in case of a failure
    add     esp, 8                  ; pushed address of jump table + pushed target address

    ; restore flags (just to be sure)
    push    dword [ss:PATM_ASMFIX_TEMP_EFLAGS]
    popfd

    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

%ifdef DEBUG
    ; for disassembly
    jmp     NAME(PATMJumpIndirect_EndProc)
%endif

align   4
PATMJumpIndirectTable:
    DW      PATM_MAX_JUMPTABLE_ENTRIES          ; nrSlots
    DW      0                                   ; ulInsertPos
    DD      0                                   ; cAddresses
    TIMES PATCHJUMPTABLE_SIZE DB 0              ; lookup slots

    ; returning here -> do not add code here or after the jmp!!!!!
ENDPROC PATMJumpIndirect

; Patch record for indirect jumps
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmJumpIndirectRecord
    PATCHASMRECORD_INIT PATMJumpIndirect, 5
    DD      PATM_ASMFIX_TEMP_EFLAGS,               0
    DD      PATM_ASMFIX_FIXUP,                     PATMJumpIndirectTable - NAME(PATMJumpIndirect)
    DD      PATM_ASMFIX_LOOKUP_AND_JUMP_FUNCTION,  0
    DD      PATM_ASMFIX_TEMP_EFLAGS,               0
    DD      PATM_ASMFIX_INTERRUPTFLAG,             0
    DD      0ffffffffh, 0ffffffffh


;
; return from duplicated function
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMRet
    ; probe stack here as we can't recover from page faults later on
    not     dword [esp-32]
    not     dword [esp-32]
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushfd
    push    eax
    push    PATM_ASMFIX_FIXUP
    DB      0E8h                    ; call
    DD      PATM_ASMFIX_RETURN_FUNCTION
    add     esp, 4                  ; pushed address of jump table

    cmp     eax, 0
    jne     near PATMRet_Success

    pop     eax
    popfd
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

%ifdef DEBUG
    ; for disassembly
    jmp     PATMRet_Success
%endif
align   4
PATMRetTable:
    DW      PATM_MAX_JUMPTABLE_ENTRIES          ; nrSlots
    DW      0                                   ; ulInsertPos
    DD      0                                   ; cAddresses
    TIMES PATCHJUMPTABLE_SIZE DB 0              ; lookup slots

PATMRet_Success:
    mov     dword [esp+8], eax                  ; overwrite the saved return address
    pop     eax
    popf
                                                ; caller will duplicate the ret or ret n instruction
                                                ; the patched call will set PATM_ASMFIX_INTERRUPTFLAG after the return!
ENDPROC     PATMRet

BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmRetRecord
    PATCHASMRECORD_INIT PATMRet, 4
    DD      PATM_ASMFIX_INTERRUPTFLAG,   0
    DD      PATM_ASMFIX_FIXUP,           PATMRetTable - NAME(PATMRet)
    DD      PATM_ASMFIX_RETURN_FUNCTION, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG,   0
    DD      0ffffffffh, 0ffffffffh


;
; global function for implementing 'retn'
;
; Caller is responsible for right stack layout
;  + 16 original return address
;  + 12 eflags
;  +  8 eax
;  +  4 Jump table address
;( +  0 return address )
;
; @note assumes PATM_ASMFIX_INTERRUPTFLAG is zero
; @note assumes it can trash eax and eflags
;
; @returns eax=0 on failure
;          otherwise return address in eax
;
; @note NEVER change this without bumping the SSM version
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMRetFunction
    push    ecx
    push    edx
    push    edi

    ; Event order:
    ; (@todo figure out which path is taken most often (1 or 2))
    ; 1) Check if the return patch address was pushed onto the PATM stack
    ; 2) Check if the return patch address can be found in the lookup table
    ; 3) Query return patch address from the hypervisor


    ; 1) Check if the return patch address was pushed on the PATM stack
    cmp     dword [ss:PATM_ASMFIX_STACKPTR], PATM_STACK_SIZE
    jae     near PATMRetFunction_FindReturnAddress

    mov     edx, dword PATM_ASMFIX_STACKPTR

    ; check if the return address is what we expect it to be
    mov     eax, dword PATM_ASMFIX_STACKBASE_GUEST
    add     eax, dword [ss:edx]                 ; stack base + stack position
    mov     eax, dword [ss:eax]                 ; original return address
    cmp     eax, dword [esp+12+16]              ; pushed return address

    ; the return address was changed -> let our trap handler try to find it
    ; (can happen when the guest messes with the stack (seen it) or when we didn't call this function ourselves)
    jne     near PATMRetFunction_FindReturnAddress

    ; found it, convert relative to absolute patch address and return the result to the caller
    mov     eax, dword PATM_ASMFIX_STACKBASE
    add     eax, dword [ss:edx]                 ; stack base + stack position
    mov     eax, dword [ss:eax]                 ; relative patm return address
    add     eax, PATM_ASMFIX_PATCHBASE

%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ebx
    push    ecx
    push    edx
    mov     edx, eax                            ; return address
    lea     ebx, [esp+16+12+16]                 ; stack address containing the return address
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOG_RET
    mov     eax, PATM_ACTION_LOG_RET
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax
%endif

    add     dword [ss:edx], 4                   ; pop return address from the PATM stack (sizeof(RTGCPTR); @note hardcoded assumption!)

    pop     edi
    pop     edx
    pop     ecx
    ret

PATMRetFunction_FindReturnAddress:
    ; 2) Check if the return patch address can be found in the lookup table
    mov     edx, dword [esp+12+16]  ; pushed target address

    xor     eax, eax                ; default result -> nothing found
    mov     edi, dword [esp+12+4]  ; jump table
    mov     ecx, [ss:edi + PATCHJUMPTABLE.cAddresses]
    cmp     ecx, 0
    je      near PATMRetFunction_AskHypervisor

PATMRetFunction_SearchStart:
    cmp     [ss:edi + PATCHJUMPTABLE.Slot_pInstrGC + eax*8], edx        ; edx = GC address to search for
    je      near PATMRetFunction_SearchHit
    inc     eax
    cmp     eax, ecx
    jl      near PATMRetFunction_SearchStart

PATMRetFunction_AskHypervisor:
    ; 3) Query return patch address from the hypervisor
    ; @todo private ugly interface, since we have nothing generic at the moment
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOOKUP_ADDRESS
    mov     eax, PATM_ACTION_LOOKUP_ADDRESS
    mov     ecx, PATM_ACTION_MAGIC
    mov     edi, dword [esp+12+4]               ; jump table address
    mov     edx, dword [esp+12+16]              ; original return address
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    jmp     near PATMRetFunction_SearchEnd

PATMRetFunction_SearchHit:
    mov     eax, [ss:edi + PATCHJUMPTABLE.Slot_pRelPatchGC + eax*8]        ; found a match!
    ;@note can be zero, so the next check is required!!

PATMRetFunction_SearchEnd:
    cmp     eax, 0
    jz      PATMRetFunction_Failure

    add     eax, PATM_ASMFIX_PATCHBASE

%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ebx
    push    ecx
    push    edx
    mov     edx, eax                            ; return address
    lea     ebx, [esp+16+12+16]                 ; stack address containing the return address
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOG_RET
    mov     eax, PATM_ACTION_LOG_RET
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax
%endif

    pop     edi
    pop     edx
    pop     ecx
    ret

PATMRetFunction_Failure:
    ;signal error
    xor     eax, eax
    pop     edi
    pop     edx
    pop     ecx
    ret
ENDPROC     PATMRetFunction

BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmRetFunctionRecord
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT PATMRetFunction, 9
%else
    PATCHASMRECORD_INIT PATMRetFunction, 7
%endif
    DD      PATM_ASMFIX_STACKPTR,        0
    DD      PATM_ASMFIX_STACKPTR,        0
    DD      PATM_ASMFIX_STACKBASE_GUEST, 0
    DD      PATM_ASMFIX_STACKBASE,       0
    DD      PATM_ASMFIX_PATCHBASE,       0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION,   0
%endif
    DD      PATM_ASMFIX_PENDINGACTION,   0
    DD      PATM_ASMFIX_PATCHBASE,       0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION,   0
%endif
    DD      0ffffffffh, 0ffffffffh


;
; Jump to original instruction if IF=1
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMCheckIF
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf
    test    dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF
    jnz     PATMCheckIF_Safe
    nop

    ; IF=0 -> unsafe, so we must call the duplicated function (which we don't do here)
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    jmp     NAME(PATMCheckIF_EndProc)

PATMCheckIF_Safe:
    ; invalidate the PATM stack as we'll jump back to guest code
    mov     dword [ss:PATM_ASMFIX_STACKPTR], PATM_STACK_SIZE

%ifdef PATM_LOG_PATCHINSTR
    push    eax
    push    ecx
    lock    or dword [ss:PATM_ASMFIX_PENDINGACTION], PATM_ACTION_LOG_IF1
    mov     eax, PATM_ACTION_LOG_IF1
    mov     ecx, PATM_ACTION_MAGIC
    db      0fh, 0bh        ; illegal instr (hardcoded assumption in PATMHandleIllegalInstrTrap)
    pop     ecx
    pop     eax
%endif
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    ; IF=1 -> we can safely jump back to the original instruction
    DB      0xE9
PATMCheckIF_Jump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMCheckIF

; Patch record for call instructions
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmCheckIFRecord
%ifdef PATM_LOG_PATCHINSTR
    PATCHASMRECORD_INIT_JUMP PATMCheckIF, PATMCheckIF_Jump, 6
%else
    PATCHASMRECORD_INIT_JUMP PATMCheckIF, PATMCheckIF_Jump, 5
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_STACKPTR,      0
%ifdef PATM_LOG_PATCHINSTR
    DD      PATM_ASMFIX_PENDINGACTION, 0
%endif
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; Jump back to guest if IF=1, else fault
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC   PATMJumpToGuest_IF1
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 0
    pushf
    test    dword [ss:PATM_ASMFIX_VMFLAGS], X86_EFL_IF
    jnz     PATMJumpToGuest_IF1_Safe
    nop

    ; IF=0 -> unsafe, so fault
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    PATM_INT3

PATMJumpToGuest_IF1_Safe:
    ; IF=1 -> we can safely jump back to the original instruction
    popf
    mov     dword [ss:PATM_ASMFIX_INTERRUPTFLAG], 1
    DB      0xE9
PATMJumpToGuest_IF1_Jump:
    DD      PATM_ASMFIX_JUMPDELTA
ENDPROC     PATMJumpToGuest_IF1

; Patch record for call instructions
BEGIN_PATCH_RODATA_SECTION
GLOBALNAME PATMJumpToGuest_IF1Record
    PATCHASMRECORD_INIT_JUMP PATMJumpToGuest_IF1, PATMJumpToGuest_IF1_Jump, 4
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_VMFLAGS,       0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      PATM_ASMFIX_INTERRUPTFLAG, 0
    DD      0ffffffffh, 0ffffffffh


;
; Check and correct RPL of pushed ss.
;
BEGIN_PATCH_CODE_SECTION
BEGINPROC PATMMovFromSS
    push    eax
    pushfd
    mov     ax, ss
    and     ax, 3
    cmp     ax, 1
    jne     near PATMMovFromSS_Continue

    and     dword [esp+8], ~3     ; clear RPL 1
PATMMovFromSS_Continue:
    popfd
    pop     eax
ENDPROC PATMMovFromSS

BEGIN_PATCH_RODATA_SECTION
GLOBALNAME g_patmMovFromSSRecord
    PATCHASMRECORD_INIT PATMMovFromSS, 0
    DD      0ffffffffh, 0ffffffffh




;; For assertion during init (to make absolutely sure the flags are in sync in vm.mac & vm.h)
BEGINCONST
GLOBALNAME g_fPatmInterruptFlag
    DD      VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_TIMER | VMCPU_FF_REQUEST


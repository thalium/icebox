; $Id: IEMAllAImpl.asm $
;; @file
; IEM - Instruction Implementation in Assembly.
;

;
; Copyright (C) 2011-2017 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   Header Files                                                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%include "VBox/asmdefs.mac"
%include "VBox/err.mac"
%include "iprt/x86.mac"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   Defined Constants And Macros                                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
; RET XX / RET wrapper for fastcall.
;
%macro RET_FASTCALL 1
%ifdef RT_ARCH_X86
 %ifdef RT_OS_WINDOWS
    ret %1
 %else
    ret
 %endif
%else
    ret
%endif
%endmacro

;;
; NAME for fastcall functions.
;
;; @todo 'global @fastcall@12' is still broken in yasm and requires dollar
;         escaping (or whatever the dollar is good for here).  Thus the ugly
;         prefix argument.
;
%define NAME_FASTCALL(a_Name, a_cbArgs, a_Prefix)   NAME(a_Name)
%ifdef RT_ARCH_X86
 %ifdef RT_OS_WINDOWS
  %undef NAME_FASTCALL
  %define NAME_FASTCALL(a_Name, a_cbArgs, a_Prefix) a_Prefix %+ a_Name %+ @ %+ a_cbArgs
 %endif
%endif

;;
; BEGINPROC for fastcall functions.
;
; @param        1       The function name (C).
; @param        2       The argument size on x86.
;
%macro BEGINPROC_FASTCALL 2
 %ifdef ASM_FORMAT_PE
  export %1=NAME_FASTCALL(%1,%2,$@)
 %endif
 %ifdef __NASM__
  %ifdef ASM_FORMAT_OMF
   export NAME(%1) NAME_FASTCALL(%1,%2,$@)
  %endif
 %endif
 %ifndef ASM_FORMAT_BIN
  global NAME_FASTCALL(%1,%2,$@)
 %endif
NAME_FASTCALL(%1,%2,@):
%endmacro


;
; We employ some macro assembly here to hid the calling convention differences.
;
%ifdef RT_ARCH_AMD64
 %macro PROLOGUE_1_ARGS 0
 %endmacro
 %macro EPILOGUE_1_ARGS 0
        ret
 %endmacro
 %macro EPILOGUE_1_ARGS_EX 0
        ret
 %endmacro

 %macro PROLOGUE_2_ARGS 0
 %endmacro
 %macro EPILOGUE_2_ARGS 0
        ret
 %endmacro
 %macro EPILOGUE_2_ARGS_EX 1
        ret
 %endmacro

 %macro PROLOGUE_3_ARGS 0
 %endmacro
 %macro EPILOGUE_3_ARGS 0
        ret
 %endmacro
 %macro EPILOGUE_3_ARGS_EX 1
        ret
 %endmacro

 %macro PROLOGUE_4_ARGS 0
 %endmacro
 %macro EPILOGUE_4_ARGS 0
        ret
 %endmacro
 %macro EPILOGUE_4_ARGS_EX 1
        ret
 %endmacro

 %ifdef ASM_CALL64_GCC
  %define A0        rdi
  %define A0_32     edi
  %define A0_16     di
  %define A0_8      dil

  %define A1        rsi
  %define A1_32     esi
  %define A1_16     si
  %define A1_8      sil

  %define A2        rdx
  %define A2_32     edx
  %define A2_16     dx
  %define A2_8      dl

  %define A3        rcx
  %define A3_32     ecx
  %define A3_16     cx
 %endif

 %ifdef ASM_CALL64_MSC
  %define A0        rcx
  %define A0_32     ecx
  %define A0_16     cx
  %define A0_8      cl

  %define A1        rdx
  %define A1_32     edx
  %define A1_16     dx
  %define A1_8      dl

  %define A2        r8
  %define A2_32     r8d
  %define A2_16     r8w
  %define A2_8      r8b

  %define A3        r9
  %define A3_32     r9d
  %define A3_16     r9w
 %endif

 %define T0         rax
 %define T0_32      eax
 %define T0_16      ax
 %define T0_8       al

 %define T1         r11
 %define T1_32      r11d
 %define T1_16      r11w
 %define T1_8       r11b

%else
 ; x86
 %macro PROLOGUE_1_ARGS 0
        push    edi
 %endmacro
 %macro EPILOGUE_1_ARGS 0
        pop     edi
        ret     0
 %endmacro
 %macro EPILOGUE_1_ARGS_EX 1
        pop     edi
        ret     %1
 %endmacro

 %macro PROLOGUE_2_ARGS 0
        push    edi
 %endmacro
 %macro EPILOGUE_2_ARGS 0
        pop     edi
        ret     0
 %endmacro
 %macro EPILOGUE_2_ARGS_EX 1
        pop     edi
        ret     %1
 %endmacro

 %macro PROLOGUE_3_ARGS 0
        push    ebx
        mov     ebx, [esp + 4 + 4]
        push    edi
 %endmacro
 %macro EPILOGUE_3_ARGS_EX 1
  %if (%1) < 4
   %error "With three args, at least 4 bytes must be remove from the stack upon return (32-bit)."
  %endif
        pop     edi
        pop     ebx
        ret     %1
 %endmacro
 %macro EPILOGUE_3_ARGS 0
        EPILOGUE_3_ARGS_EX 4
 %endmacro

 %macro PROLOGUE_4_ARGS 0
        push    ebx
        push    edi
        push    esi
        mov     ebx, [esp + 12 + 4 + 0]
        mov     esi, [esp + 12 + 4 + 4]
 %endmacro
 %macro EPILOGUE_4_ARGS_EX 1
  %if (%1) < 8
   %error "With four args, at least 8 bytes must be remove from the stack upon return (32-bit)."
  %endif
        pop     esi
        pop     edi
        pop     ebx
        ret     %1
 %endmacro
 %macro EPILOGUE_4_ARGS 0
        EPILOGUE_4_ARGS_EX 8
 %endmacro

 %define A0         ecx
 %define A0_32      ecx
 %define A0_16       cx
 %define A0_8        cl

 %define A1         edx
 %define A1_32      edx
 %define A1_16      dx
 %define A1_8       dl

 %define A2         ebx
 %define A2_32      ebx
 %define A2_16      bx
 %define A2_8       bl

 %define A3         esi
 %define A3_32      esi
 %define A3_16      si

 %define T0         eax
 %define T0_32      eax
 %define T0_16      ax
 %define T0_8       al

 %define T1         edi
 %define T1_32      edi
 %define T1_16      di
%endif


;;
; Load the relevant flags from [%1] if there are undefined flags (%3).
;
; @remarks      Clobbers T0, stack. Changes EFLAGS.
; @param        A2      The register pointing to the flags.
; @param        1       The parameter (A0..A3) pointing to the eflags.
; @param        2       The set of modified flags.
; @param        3       The set of undefined flags.
;
%macro IEM_MAYBE_LOAD_FLAGS 3
 ;%if (%3) != 0
        pushf                           ; store current flags
        mov     T0_32, [%1]             ; load the guest flags
        and     dword [xSP], ~(%2 | %3) ; mask out the modified and undefined flags
        and     T0_32, (%2 | %3)        ; select the modified and undefined flags.
        or      [xSP], T0               ; merge guest flags with host flags.
        popf                            ; load the mixed flags.
 ;%endif
%endmacro

;;
; Update the flag.
;
; @remarks  Clobbers T0, T1, stack.
; @param        1       The register pointing to the EFLAGS.
; @param        2       The mask of modified flags to save.
; @param        3       The mask of undefined flags to (maybe) save.
;
%macro IEM_SAVE_FLAGS 3
 %if (%2 | %3) != 0
        pushf
        pop     T1
        mov     T0_32, [%1]             ; flags
        and     T0_32, ~(%2 | %3)       ; clear the modified & undefined flags.
        and     T1_32, (%2 | %3)        ; select the modified and undefined flags.
        or      T0_32, T1_32            ; combine the flags.
        mov     [%1], T0_32             ; save the flags.
 %endif
%endmacro


;;
; Macro for implementing a binary operator.
;
; This will generate code for the 8, 16, 32 and 64 bit accesses with locked
; variants, except on 32-bit system where the 64-bit accesses requires hand
; coding.
;
; All the functions takes a pointer to the destination memory operand in A0,
; the source register operand in A1 and a pointer to eflags in A2.
;
; @param        1       The instruction mnemonic.
; @param        2       Non-zero if there should be a locked version.
; @param        3       The modified flags.
; @param        4       The undefined flags.
;
%macro IEMIMPL_BIN_OP 4
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u8, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        %1      byte [A0], A1_8
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u8

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        %1      word [A0], A1_16
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        %1      dword [A0], A1_32
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32

 %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 16
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        %1      qword [A0], A1
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS_EX 8
ENDPROC iemAImpl_ %+ %1 %+ _u64
  %endif ; RT_ARCH_AMD64

 %if %2 != 0 ; locked versions requested?

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u8_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        lock %1 byte [A0], A1_8
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u8_locked

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        lock %1 word [A0], A1_16
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16_locked

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        lock %1 dword [A0], A1_32
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32_locked

  %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64_locked, 16
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        lock %1 qword [A0], A1
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS_EX 8
ENDPROC iemAImpl_ %+ %1 %+ _u64_locked
  %endif ; RT_ARCH_AMD64
 %endif ; locked
%endmacro

;            instr,lock,modified-flags.
IEMIMPL_BIN_OP add,  1, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
IEMIMPL_BIN_OP adc,  1, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
IEMIMPL_BIN_OP sub,  1, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
IEMIMPL_BIN_OP sbb,  1, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
IEMIMPL_BIN_OP or,   1, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), X86_EFL_AF
IEMIMPL_BIN_OP xor,  1, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), X86_EFL_AF
IEMIMPL_BIN_OP and,  1, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), X86_EFL_AF
IEMIMPL_BIN_OP cmp,  0, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
IEMIMPL_BIN_OP test, 0, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), X86_EFL_AF


;;
; Macro for implementing a bit operator.
;
; This will generate code for the 16, 32 and 64 bit accesses with locked
; variants, except on 32-bit system where the 64-bit accesses requires hand
; coding.
;
; All the functions takes a pointer to the destination memory operand in A0,
; the source register operand in A1 and a pointer to eflags in A2.
;
; @param        1       The instruction mnemonic.
; @param        2       Non-zero if there should be a locked version.
; @param        3       The modified flags.
; @param        4       The undefined flags.
;
%macro IEMIMPL_BIT_OP 4
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        %1      word [A0], A1_16
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        %1      dword [A0], A1_32
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32

 %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 16
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        %1      qword [A0], A1
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS_EX 8
ENDPROC iemAImpl_ %+ %1 %+ _u64
  %endif ; RT_ARCH_AMD64

 %if %2 != 0 ; locked versions requested?

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        lock %1 word [A0], A1_16
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16_locked

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        lock %1 dword [A0], A1_32
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32_locked

  %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64_locked, 16
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %3, %4
        lock %1 qword [A0], A1
        IEM_SAVE_FLAGS                 A2, %3, %4
        EPILOGUE_3_ARGS_EX 8
ENDPROC iemAImpl_ %+ %1 %+ _u64_locked
  %endif ; RT_ARCH_AMD64
 %endif ; locked
%endmacro
IEMIMPL_BIT_OP bt,  0, (X86_EFL_CF), (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
IEMIMPL_BIT_OP btc, 1, (X86_EFL_CF), (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
IEMIMPL_BIT_OP bts, 1, (X86_EFL_CF), (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
IEMIMPL_BIT_OP btr, 1, (X86_EFL_CF), (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)

;;
; Macro for implementing a bit search operator.
;
; This will generate code for the 16, 32 and 64 bit accesses, except on 32-bit
; system where the 64-bit accesses requires hand coding.
;
; All the functions takes a pointer to the destination memory operand in A0,
; the source register operand in A1 and a pointer to eflags in A2.
;
; @param        1       The instruction mnemonic.
; @param        2       The modified flags.
; @param        3       The undefined flags.
;
%macro IEMIMPL_BIT_OP 3
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %2, %3
        %1      T0_16, A1_16
        jz      .unchanged_dst
        mov     [A0], T0_16
.unchanged_dst:
        IEM_SAVE_FLAGS                 A2, %2, %3
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %2, %3
        %1      T0_32, A1_32
        jz      .unchanged_dst
        mov     [A0], T0_32
.unchanged_dst:
        IEM_SAVE_FLAGS                 A2, %2, %3
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32

 %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 16
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS           A2, %2, %3
        %1      T0, A1
        jz      .unchanged_dst
        mov     [A0], T0
.unchanged_dst:
        IEM_SAVE_FLAGS                 A2, %2, %3
        EPILOGUE_3_ARGS_EX 8
ENDPROC iemAImpl_ %+ %1 %+ _u64
 %endif ; RT_ARCH_AMD64
%endmacro
IEMIMPL_BIT_OP bsf, (X86_EFL_ZF), (X86_EFL_OF | X86_EFL_SF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF)
IEMIMPL_BIT_OP bsr, (X86_EFL_ZF), (X86_EFL_OF | X86_EFL_SF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF)


;
; IMUL is also a similar but yet different case (no lock, no mem dst).
; The rDX:rAX variant of imul is handled together with mul further down.
;
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_imul_two_u16, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
        imul    A1_16, word [A0]
        mov     [A0], A1_16
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_imul_two_u16

BEGINPROC_FASTCALL iemAImpl_imul_two_u32, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
        imul    A1_32, dword [A0]
        mov     [A0], A1_32
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_imul_two_u32

%ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_imul_two_u64, 16
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
        imul    A1, qword [A0]
        mov     [A0], A1
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
        EPILOGUE_3_ARGS_EX 8
ENDPROC iemAImpl_imul_two_u64
%endif ; RT_ARCH_AMD64


;
; XCHG for memory operands.  This implies locking.  No flag changes.
;
; Each function takes two arguments, first the pointer to the memory,
; then the pointer to the register.  They all return void.
;
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_xchg_u8, 8
        PROLOGUE_2_ARGS
        mov     T0_8, [A1]
        xchg    [A0], T0_8
        mov     [A1], T0_8
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_xchg_u8

BEGINPROC_FASTCALL iemAImpl_xchg_u16, 8
        PROLOGUE_2_ARGS
        mov     T0_16, [A1]
        xchg    [A0], T0_16
        mov     [A1], T0_16
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_xchg_u16

BEGINPROC_FASTCALL iemAImpl_xchg_u32, 8
        PROLOGUE_2_ARGS
        mov     T0_32, [A1]
        xchg    [A0], T0_32
        mov     [A1], T0_32
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_xchg_u32

%ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_xchg_u64, 8
        PROLOGUE_2_ARGS
        mov     T0, [A1]
        xchg    [A0], T0
        mov     [A1], T0
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_xchg_u64
%endif


;
; XADD for memory operands.
;
; Each function takes three arguments, first the pointer to the
; memory/register, then the pointer to the register, and finally a pointer to
; eflags.  They all return void.
;
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_xadd_u8, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0_8, [A1]
        xadd    [A0], T0_8
        mov     [A1], T0_8
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u8

BEGINPROC_FASTCALL iemAImpl_xadd_u16, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0_16, [A1]
        xadd    [A0], T0_16
        mov     [A1], T0_16
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u16

BEGINPROC_FASTCALL iemAImpl_xadd_u32, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0_32, [A1]
        xadd    [A0], T0_32
        mov     [A1], T0_32
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u32

%ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_xadd_u64, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0, [A1]
        xadd    [A0], T0
        mov     [A1], T0
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u64
%endif ; RT_ARCH_AMD64

BEGINPROC_FASTCALL iemAImpl_xadd_u8_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0_8, [A1]
        lock xadd [A0], T0_8
        mov     [A1], T0_8
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u8_locked

BEGINPROC_FASTCALL iemAImpl_xadd_u16_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0_16, [A1]
        lock xadd [A0], T0_16
        mov     [A1], T0_16
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u16_locked

BEGINPROC_FASTCALL iemAImpl_xadd_u32_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0_32, [A1]
        lock xadd [A0], T0_32
        mov     [A1], T0_32
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u32_locked

%ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_xadd_u64_locked, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        mov     T0, [A1]
        lock xadd [A0], T0
        mov     [A1], T0
        IEM_SAVE_FLAGS       A2, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_xadd_u64_locked
%endif ; RT_ARCH_AMD64


;
; CMPXCHG8B.
;
; These are tricky register wise, so the code is duplicated for each calling
; convention.
;
; WARNING! This code make ASSUMPTIONS about which registers T1 and T0 are mapped to!
;
; C-proto:
; IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg8b,(uint64_t *pu64Dst, PRTUINT64U pu64EaxEdx, PRTUINT64U pu64EbxEcx,
;                                             uint32_t *pEFlags));
;
; Note! Identical to iemAImpl_cmpxchg16b.
;
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_cmpxchg8b, 16
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        push    rbx

        mov     r11, rdx                ; pu64EaxEdx (is also T1)
        mov     r10, rcx                ; pu64Dst

        mov     ebx, [r8]
        mov     ecx, [r8 + 4]
        IEM_MAYBE_LOAD_FLAGS r9, (X86_EFL_ZF), 0 ; clobbers T0 (eax)
        mov     eax, [r11]
        mov     edx, [r11 + 4]

        lock cmpxchg8b [r10]

        mov     [r11], eax
        mov     [r11 + 4], edx
        IEM_SAVE_FLAGS       r9, (X86_EFL_ZF), 0 ; clobbers T0+T1 (eax, r11)

        pop     rbx
        ret
 %else
        push    rbx

        mov     r10, rcx                ; pEFlags
        mov     r11, rdx                ; pu64EbxEcx (is also T1)

        mov     ebx, [r11]
        mov     ecx, [r11 + 4]
        IEM_MAYBE_LOAD_FLAGS r10, (X86_EFL_ZF), 0 ; clobbers T0 (eax)
        mov     eax, [rsi]
        mov     edx, [rsi + 4]

        lock cmpxchg8b [rdi]

        mov     [rsi], eax
        mov     [rsi + 4], edx
        IEM_SAVE_FLAGS       r10, (X86_EFL_ZF), 0 ; clobbers T0+T1 (eax, r11)

        pop     rbx
        ret

 %endif
%else
        push    esi
        push    edi
        push    ebx
        push    ebp

        mov     edi, ecx                ; pu64Dst
        mov     esi, edx                ; pu64EaxEdx
        mov     ecx, [esp + 16 + 4 + 0] ; pu64EbxEcx
        mov     ebp, [esp + 16 + 4 + 4] ; pEFlags

        mov     ebx, [ecx]
        mov     ecx, [ecx + 4]
        IEM_MAYBE_LOAD_FLAGS ebp, (X86_EFL_ZF), 0  ; clobbers T0 (eax)
        mov     eax, [esi]
        mov     edx, [esi + 4]

        lock cmpxchg8b [edi]

        mov     [esi], eax
        mov     [esi + 4], edx
        IEM_SAVE_FLAGS       ebp, (X86_EFL_ZF), 0 ; clobbers T0+T1 (eax, edi)

        pop     ebp
        pop     ebx
        pop     edi
        pop     esi
        ret     8
%endif
ENDPROC iemAImpl_cmpxchg8b

BEGINPROC_FASTCALL iemAImpl_cmpxchg8b_locked, 16
        ; Lazy bird always lock prefixes cmpxchg8b.
        jmp     NAME_FASTCALL(iemAImpl_cmpxchg8b,16,$@)
ENDPROC iemAImpl_cmpxchg8b_locked

%ifdef RT_ARCH_AMD64

;
; CMPXCHG16B.
;
; These are tricky register wise, so the code is duplicated for each calling
; convention.
;
; WARNING! This code make ASSUMPTIONS about which registers T1 and T0 are mapped to!
;
; C-proto:
; IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg16b,(PRTUINT128U pu128Dst, PRTUINT128U pu1284RaxRdx, PRTUINT128U pu128RbxRcx,
;                                              uint32_t *pEFlags));
;
; Note! Identical to iemAImpl_cmpxchg8b.
;
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_cmpxchg16b, 16
 %ifdef ASM_CALL64_MSC
        push    rbx

        mov     r11, rdx                ; pu64RaxRdx (is also T1)
        mov     r10, rcx                ; pu64Dst

        mov     rbx, [r8]
        mov     rcx, [r8 + 8]
        IEM_MAYBE_LOAD_FLAGS r9, (X86_EFL_ZF), 0 ; clobbers T0 (eax)
        mov     rax, [r11]
        mov     rdx, [r11 + 8]

        lock cmpxchg16b [r10]

        mov     [r11], rax
        mov     [r11 + 8], rdx
        IEM_SAVE_FLAGS       r9, (X86_EFL_ZF), 0 ; clobbers T0+T1 (eax, r11)

        pop     rbx
        ret
 %else
        push    rbx

        mov     r10, rcx                ; pEFlags
        mov     r11, rdx                ; pu64RbxRcx (is also T1)

        mov     rbx, [r11]
        mov     rcx, [r11 + 8]
        IEM_MAYBE_LOAD_FLAGS r10, (X86_EFL_ZF), 0 ; clobbers T0 (eax)
        mov     rax, [rsi]
        mov     rdx, [rsi + 8]

        lock cmpxchg16b [rdi]

        mov     [rsi], eax
        mov     [rsi + 8], edx
        IEM_SAVE_FLAGS       r10, (X86_EFL_ZF), 0 ; clobbers T0+T1 (eax, r11)

        pop     rbx
        ret

 %endif
ENDPROC iemAImpl_cmpxchg16b

BEGINPROC_FASTCALL iemAImpl_cmpxchg16b_locked, 16
        ; Lazy bird always lock prefixes cmpxchg8b.
        jmp     NAME_FASTCALL(iemAImpl_cmpxchg16b,16,$@)
ENDPROC iemAImpl_cmpxchg16b_locked

%endif ; RT_ARCH_AMD64


;
; CMPXCHG.
;
; WARNING! This code make ASSUMPTIONS about which registers T1 and T0 are mapped to!
;
; C-proto:
; IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg,(uintX_t *puXDst, uintX_t puEax, uintX_t uReg, uint32_t *pEFlags));
;
BEGINCODE
%macro IEMIMPL_CMPXCHG 2
BEGINPROC_FASTCALL iemAImpl_cmpxchg_u8 %+ %2, 16
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0 (eax)
        mov     al, [A1]
        %1 cmpxchg [A0], A2_8
        mov     [A1], al
        IEM_SAVE_FLAGS       A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0+T1 (eax, r11/edi)
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_cmpxchg_u8 %+ %2

BEGINPROC_FASTCALL iemAImpl_cmpxchg_u16 %+ %2, 16
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0 (eax)
        mov     ax, [A1]
        %1 cmpxchg [A0], A2_16
        mov     [A1], ax
        IEM_SAVE_FLAGS       A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0+T1 (eax, r11/edi)
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_cmpxchg_u16 %+ %2

BEGINPROC_FASTCALL iemAImpl_cmpxchg_u32 %+ %2, 16
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0 (eax)
        mov     eax, [A1]
        %1 cmpxchg [A0], A2_32
        mov     [A1], eax
        IEM_SAVE_FLAGS       A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0+T1 (eax, r11/edi)
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_cmpxchg_u32 %+ %2

BEGINPROC_FASTCALL iemAImpl_cmpxchg_u64 %+ %2, 16
%ifdef RT_ARCH_AMD64
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0 (eax)
        mov     rax, [A1]
        %1 cmpxchg [A0], A2
        mov     [A1], rax
        IEM_SAVE_FLAGS       A3, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0+T1 (eax, r11/edi)
        EPILOGUE_4_ARGS
%else
        ;
        ; Must use cmpxchg8b here. See also iemAImpl_cmpxchg8b.
        ;
        push    esi
        push    edi
        push    ebx
        push    ebp

        mov     edi, ecx                ; pu64Dst
        mov     esi, edx                ; pu64Rax
        mov     ecx, [esp + 16 + 4 + 0] ; pu64Reg - Note! Pointer on 32-bit hosts!
        mov     ebp, [esp + 16 + 4 + 4] ; pEFlags

        mov     ebx, [ecx]
        mov     ecx, [ecx + 4]
        IEM_MAYBE_LOAD_FLAGS ebp, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0  ; clobbers T0 (eax)
        mov     eax, [esi]
        mov     edx, [esi + 4]

        lock cmpxchg8b [edi]

        ; cmpxchg8b doesn't set CF, PF, AF, SF and OF, so we have to do that.
        jz      .cmpxchg8b_not_equal
        cmp     eax, eax                ; just set the other flags.
.store:
        mov     [esi], eax
        mov     [esi + 4], edx
        IEM_SAVE_FLAGS       ebp, (X86_EFL_ZF | X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_SF | X86_EFL_OF), 0 ; clobbers T0+T1 (eax, edi)

        pop     ebp
        pop     ebx
        pop     edi
        pop     esi
        ret     8

.cmpxchg8b_not_equal:
        cmp     [esi + 4], edx          ;; @todo FIXME - verify 64-bit compare implementation
        jne     .store
        cmp     [esi], eax
        jmp     .store

%endif
ENDPROC iemAImpl_cmpxchg_u64 %+ %2
%endmacro ; IEMIMPL_CMPXCHG

IEMIMPL_CMPXCHG , ,
IEMIMPL_CMPXCHG lock, _locked

;;
; Macro for implementing a unary operator.
;
; This will generate code for the 8, 16, 32 and 64 bit accesses with locked
; variants, except on 32-bit system where the 64-bit accesses requires hand
; coding.
;
; All the functions takes a pointer to the destination memory operand in A0,
; the source register operand in A1 and a pointer to eflags in A2.
;
; @param        1       The instruction mnemonic.
; @param        2       The modified flags.
; @param        3       The undefined flags.
;
%macro IEMIMPL_UNARY_OP 3
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u8, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        %1      byte [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u8

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u8_locked, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        lock %1 byte [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u8_locked

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        %1      word [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16_locked, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        lock %1 word [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16_locked

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        %1      dword [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32_locked, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        lock %1 dword [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32_locked

 %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        %1      qword [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u64

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64_locked, 8
        PROLOGUE_2_ARGS
        IEM_MAYBE_LOAD_FLAGS A1, %2, %3
        lock %1 qword [A0]
        IEM_SAVE_FLAGS       A1, %2, %3
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u64_locked
 %endif ; RT_ARCH_AMD64

%endmacro

IEMIMPL_UNARY_OP inc, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF), 0
IEMIMPL_UNARY_OP dec, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF), 0
IEMIMPL_UNARY_OP neg, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
IEMIMPL_UNARY_OP not, 0, 0


;;
; Macro for implementing memory fence operation.
;
; No return value, no operands or anything.
;
; @param        1      The instruction.
;
%macro IEMIMPL_MEM_FENCE 1
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1, 0
        %1
        ret
ENDPROC iemAImpl_ %+ %1
%endmacro

IEMIMPL_MEM_FENCE lfence
IEMIMPL_MEM_FENCE sfence
IEMIMPL_MEM_FENCE mfence

;;
; Alternative for non-SSE2 host.
;
BEGINPROC_FASTCALL iemAImpl_alt_mem_fence, 0
        push    xAX
        xchg    xAX, [xSP]
        add     xSP, xCB
        ret
ENDPROC iemAImpl_alt_mem_fence



;;
; Macro for implementing a shift operation.
;
; This will generate code for the 8, 16, 32 and 64 bit accesses, except on
; 32-bit system where the 64-bit accesses requires hand coding.
;
; All the functions takes a pointer to the destination memory operand in A0,
; the shift count in A1 and a pointer to eflags in A2.
;
; @param        1       The instruction mnemonic.
; @param        2       The modified flags.
; @param        3       The undefined flags.
;
; Makes ASSUMPTIONS about A0, A1 and A2 assignments.
;
%macro IEMIMPL_SHIFT_OP 3
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u8, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, %2, %3
 %ifdef ASM_CALL64_GCC
        mov     cl, A1_8
        %1      byte [A0], cl
 %else
        xchg    A1, A0
        %1      byte [A1], cl
 %endif
        IEM_SAVE_FLAGS       A2, %2, %3
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u8

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, %2, %3
 %ifdef ASM_CALL64_GCC
        mov     cl, A1_8
        %1      word [A0], cl
 %else
        xchg    A1, A0
        %1      word [A1], cl
 %endif
        IEM_SAVE_FLAGS       A2, %2, %3
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, %2, %3
 %ifdef ASM_CALL64_GCC
        mov     cl, A1_8
        %1      dword [A0], cl
 %else
        xchg    A1, A0
        %1      dword [A1], cl
 %endif
        IEM_SAVE_FLAGS       A2, %2, %3
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32

 %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, %2, %3
 %ifdef ASM_CALL64_GCC
        mov     cl, A1_8
        %1      qword [A0], cl
 %else
        xchg    A1, A0
        %1      qword [A1], cl
 %endif
        IEM_SAVE_FLAGS       A2, %2, %3
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u64
 %endif ; RT_ARCH_AMD64

%endmacro

IEMIMPL_SHIFT_OP rol, (X86_EFL_OF | X86_EFL_CF), 0
IEMIMPL_SHIFT_OP ror, (X86_EFL_OF | X86_EFL_CF), 0
IEMIMPL_SHIFT_OP rcl, (X86_EFL_OF | X86_EFL_CF), 0
IEMIMPL_SHIFT_OP rcr, (X86_EFL_OF | X86_EFL_CF), 0
IEMIMPL_SHIFT_OP shl, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), (X86_EFL_AF)
IEMIMPL_SHIFT_OP shr, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), (X86_EFL_AF)
IEMIMPL_SHIFT_OP sar, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), (X86_EFL_AF)


;;
; Macro for implementing a double precision shift operation.
;
; This will generate code for the 16, 32 and 64 bit accesses, except on
; 32-bit system where the 64-bit accesses requires hand coding.
;
; The functions takes the destination operand (r/m) in A0, the source (reg) in
; A1, the shift count in A2 and a pointer to the eflags variable/register in A3.
;
; @param        1       The instruction mnemonic.
; @param        2       The modified flags.
; @param        3       The undefined flags.
;
; Makes ASSUMPTIONS about A0, A1, A2 and A3 assignments.
;
%macro IEMIMPL_SHIFT_DBL_OP 3
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 16
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
 %ifdef ASM_CALL64_GCC
        xchg    A3, A2
        %1      [A0], A1_16, cl
        xchg    A3, A2
 %else
        xchg    A0, A2
        %1      [A2], A1_16, cl
 %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 16
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
 %ifdef ASM_CALL64_GCC
        xchg    A3, A2
        %1      [A0], A1_32, cl
        xchg    A3, A2
 %else
        xchg    A0, A2
        %1      [A2], A1_32, cl
 %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32

 %ifdef RT_ARCH_AMD64
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 20
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
 %ifdef ASM_CALL64_GCC
        xchg    A3, A2
        %1      [A0], A1, cl
        xchg    A3, A2
 %else
        xchg    A0, A2
        %1      [A2], A1, cl
 %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        EPILOGUE_4_ARGS_EX 12
ENDPROC iemAImpl_ %+ %1 %+ _u64
 %endif ; RT_ARCH_AMD64

%endmacro

IEMIMPL_SHIFT_DBL_OP shld, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), (X86_EFL_AF)
IEMIMPL_SHIFT_DBL_OP shrd, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_PF | X86_EFL_CF), (X86_EFL_AF)


;;
; Macro for implementing a multiplication operations.
;
; This will generate code for the 8, 16, 32 and 64 bit accesses, except on
; 32-bit system where the 64-bit accesses requires hand coding.
;
; The 8-bit function only operates on AX, so it takes no DX pointer.  The other
; functions takes a pointer to rAX in A0, rDX in A1, the operand in A2 and a
; pointer to eflags in A3.
;
; The functions all return 0 so the caller can be used for div/idiv as well as
; for the mul/imul implementation.
;
; @param        1       The instruction mnemonic.
; @param        2       The modified flags.
; @param        3       The undefined flags.
;
; Makes ASSUMPTIONS about A0, A1, A2, A3, T0 and T1 assignments.
;
%macro IEMIMPL_MUL_OP 3
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u8, 12
        PROLOGUE_3_ARGS
        IEM_MAYBE_LOAD_FLAGS A2, %2, %3
        mov     al, [A0]
        %1      A1_8
        mov     [A0], ax
        IEM_SAVE_FLAGS       A2, %2, %3
        xor     eax, eax
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u8

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 16
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
        mov     ax, [A0]
 %ifdef ASM_CALL64_GCC
        %1      A2_16
        mov     [A0], ax
        mov     [A1], dx
 %else
        mov     T1, A1
        %1      A2_16
        mov     [A0], ax
        mov     [T1], dx
 %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        xor     eax, eax
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 16
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
        mov     eax, [A0]
 %ifdef ASM_CALL64_GCC
        %1      A2_32
        mov     [A0], eax
        mov     [A1], edx
 %else
        mov     T1, A1
        %1      A2_32
        mov     [A0], eax
        mov     [T1], edx
 %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        xor     eax, eax
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u32

 %ifdef RT_ARCH_AMD64 ; The 32-bit host version lives in IEMAllAImplC.cpp.
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 20
        PROLOGUE_4_ARGS
        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
        mov     rax, [A0]
  %ifdef ASM_CALL64_GCC
        %1      A2
        mov     [A0], rax
        mov     [A1], rdx
  %else
        mov     T1, A1
        %1      A2
        mov     [A0], rax
        mov     [T1], rdx
  %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        xor     eax, eax
        EPILOGUE_4_ARGS_EX 12
ENDPROC iemAImpl_ %+ %1 %+ _u64
 %endif ; !RT_ARCH_AMD64

%endmacro

IEMIMPL_MUL_OP mul,  (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)
IEMIMPL_MUL_OP imul, (X86_EFL_OF | X86_EFL_CF), (X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF)


BEGINCODE
;;
; Worker function for negating a 32-bit number in T1:T0
; @uses None (T0,T1)
iemAImpl_negate_T0_T1_u32:
        push    0
        push    0
        xchg    T0_32, [xSP]
        xchg    T1_32, [xSP + xCB]
        sub     T0_32, [xSP]
        sbb     T1_32, [xSP + xCB]
        add     xSP, xCB*2
        ret

%ifdef RT_ARCH_AMD64
;;
; Worker function for negating a 64-bit number in T1:T0
; @uses None (T0,T1)
iemAImpl_negate_T0_T1_u64:
        push    0
        push    0
        xchg    T0, [xSP]
        xchg    T1, [xSP + xCB]
        sub     T0, [xSP]
        sbb     T1, [xSP + xCB]
        add     xSP, xCB*2
        ret
%endif


;;
; Macro for implementing a division operations.
;
; This will generate code for the 8, 16, 32 and 64 bit accesses, except on
; 32-bit system where the 64-bit accesses requires hand coding.
;
; The 8-bit function only operates on AX, so it takes no DX pointer.  The other
; functions takes a pointer to rAX in A0, rDX in A1, the operand in A2 and a
; pointer to eflags in A3.
;
; The functions all return 0 on success and -1 if a divide error should be
; raised by the caller.
;
; @param        1       The instruction mnemonic.
; @param        2       The modified flags.
; @param        3       The undefined flags.
; @param        4       1 if signed, 0 if unsigned.
;
; Makes ASSUMPTIONS about A0, A1, A2, A3, T0 and T1 assignments.
;
%macro IEMIMPL_DIV_OP 4
BEGINCODE
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u8, 12
        PROLOGUE_3_ARGS

        ; div by chainsaw check.
        test    A1_8, A1_8
        jz      .div_zero

        ; Overflow check - unsigned division is simple to verify, haven't
        ; found a simple way to check signed division yet unfortunately.
 %if %4 == 0
        cmp     [A0 + 1], A1_8
        jae     .div_overflow
 %else
        mov     T0_16, [A0]             ; T0 = dividend
        mov     T1, A1                  ; T1 = saved divisor (because of missing T1_8 in 32-bit)
        test    A1_8, A1_8
        js      .divisor_negative
        test    T0_16, T0_16
        jns     .both_positive
        neg     T0_16
.one_of_each:                           ; OK range is 2^(result-with - 1) + (divisor - 1).
        push    T0                      ; Start off like unsigned below.
        shr     T0_16, 7
        cmp     T0_8, A1_8
        pop     T0
        jb      .div_no_overflow
        ja      .div_overflow
        and     T0_8, 0x7f              ; Special case for covering (divisor - 1).
        cmp     T0_8, A1_8
        jae     .div_overflow
        jmp     .div_no_overflow

.divisor_negative:
        neg     A1_8
        test    T0_16, T0_16
        jns     .one_of_each
        neg     T0_16
.both_positive:                         ; Same as unsigned shifted by sign indicator bit.
        shr     T0_16, 7
        cmp     T0_8, A1_8
        jae     .div_overflow
.div_no_overflow:
        mov     A1, T1                  ; restore divisor
 %endif

        IEM_MAYBE_LOAD_FLAGS A2, %2, %3
        mov     ax, [A0]
        %1      A1_8
        mov     [A0], ax
        IEM_SAVE_FLAGS       A2, %2, %3
        xor     eax, eax

.return:
        EPILOGUE_3_ARGS

.div_zero:
.div_overflow:
        mov     eax, -1
        jmp     .return
ENDPROC iemAImpl_ %+ %1 %+ _u8

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u16, 16
        PROLOGUE_4_ARGS

        ; div by chainsaw check.
        test    A2_16, A2_16
        jz      .div_zero

        ; Overflow check - unsigned division is simple to verify, haven't
        ; found a simple way to check signed division yet unfortunately.
 %if %4 == 0
        cmp     [A1], A2_16
        jae     .div_overflow
 %else
        mov     T0_16, [A1]
        shl     T0_32, 16
        mov     T0_16, [A0]              ; T0 = dividend
        mov     T1, A2                   ; T1 = divisor
        test    T1_16, T1_16
        js      .divisor_negative
        test    T0_32, T0_32
        jns     .both_positive
        neg     T0_32
.one_of_each:                           ; OK range is 2^(result-with - 1) + (divisor - 1).
        push    T0                      ; Start off like unsigned below.
        shr     T0_32, 15
        cmp     T0_16, T1_16
        pop     T0
        jb      .div_no_overflow
        ja      .div_overflow
        and     T0_16, 0x7fff           ; Special case for covering (divisor - 1).
        cmp     T0_16, T1_16
        jae     .div_overflow
        jmp     .div_no_overflow

.divisor_negative:
        neg     T1_16
        test    T0_32, T0_32
        jns     .one_of_each
        neg     T0_32
.both_positive:                         ; Same as unsigned shifted by sign indicator bit.
        shr     T0_32, 15
        cmp     T0_16, T1_16
        jae     .div_overflow
.div_no_overflow:
 %endif

        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
 %ifdef ASM_CALL64_GCC
        mov     T1, A2
        mov     ax, [A0]
        mov     dx, [A1]
        %1      T1_16
        mov     [A0], ax
        mov     [A1], dx
 %else
        mov     T1, A1
        mov     ax, [A0]
        mov     dx, [T1]
        %1      A2_16
        mov     [A0], ax
        mov     [T1], dx
 %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        xor     eax, eax

.return:
        EPILOGUE_4_ARGS

.div_zero:
.div_overflow:
        mov     eax, -1
        jmp     .return
ENDPROC iemAImpl_ %+ %1 %+ _u16

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u32, 16
        PROLOGUE_4_ARGS

        ; div by chainsaw check.
        test    A2_32, A2_32
        jz      .div_zero

        ; Overflow check - unsigned division is simple to verify, haven't
        ; found a simple way to check signed division yet unfortunately.
 %if %4 == 0
        cmp     [A1], A2_32
        jae     .div_overflow
 %else
        push    A2                      ; save A2 so we modify it (we out of regs on x86).
        mov     T0_32, [A0]             ; T0 = dividend low
        mov     T1_32, [A1]             ; T1 = dividend high
        test    A2_32, A2_32
        js      .divisor_negative
        test    T1_32, T1_32
        jns     .both_positive
        call    iemAImpl_negate_T0_T1_u32
.one_of_each:                           ; OK range is 2^(result-with - 1) + (divisor - 1).
        push    T0                      ; Start off like unsigned below.
        shl     T1_32, 1
        shr     T0_32, 31
        or      T1_32, T0_32
        cmp     T1_32, A2_32
        pop     T0
        jb      .div_no_overflow
        ja      .div_overflow
        and     T0_32, 0x7fffffff       ; Special case for covering (divisor - 1).
        cmp     T0_32, A2_32
        jae     .div_overflow
        jmp     .div_no_overflow

.divisor_negative:
        neg     A2_32
        test    T1_32, T1_32
        jns     .one_of_each
        call    iemAImpl_negate_T0_T1_u32
.both_positive:                         ; Same as unsigned shifted by sign indicator bit.
        shl     T1_32, 1
        shr     T0_32, 31
        or      T1_32, T0_32
        cmp     T1_32, A2_32
        jae     .div_overflow
.div_no_overflow:
        pop     A2
 %endif

        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
        mov     eax, [A0]
 %ifdef ASM_CALL64_GCC
        mov     T1, A2
        mov     eax, [A0]
        mov     edx, [A1]
        %1      T1_32
        mov     [A0], eax
        mov     [A1], edx
 %else
        mov     T1, A1
        mov     eax, [A0]
        mov     edx, [T1]
        %1      A2_32
        mov     [A0], eax
        mov     [T1], edx
 %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        xor     eax, eax

.return:
        EPILOGUE_4_ARGS

.div_overflow:
 %if %4 != 0
        pop     A2
 %endif
.div_zero:
        mov     eax, -1
        jmp     .return
ENDPROC iemAImpl_ %+ %1 %+ _u32

 %ifdef RT_ARCH_AMD64 ; The 32-bit host version lives in IEMAllAImplC.cpp.
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 20
        PROLOGUE_4_ARGS

        test    A2, A2
        jz      .div_zero
  %if %4 == 0
        cmp     [A1], A2
        jae     .div_overflow
  %else
        push    A2                      ; save A2 so we modify it (we out of regs on x86).
        mov     T0, [A0]                ; T0 = dividend low
        mov     T1, [A1]                ; T1 = dividend high
        test    A2, A2
        js      .divisor_negative
        test    T1, T1
        jns     .both_positive
        call    iemAImpl_negate_T0_T1_u64
.one_of_each:                           ; OK range is 2^(result-with - 1) + (divisor - 1).
        push    T0                      ; Start off like unsigned below.
        shl     T1, 1
        shr     T0, 63
        or      T1, T0
        cmp     T1, A2
        pop     T0
        jb      .div_no_overflow
        ja      .div_overflow
        mov     T1, 0x7fffffffffffffff
        and     T0, T1                  ; Special case for covering (divisor - 1).
        cmp     T0, A2
        jae     .div_overflow
        jmp     .div_no_overflow

.divisor_negative:
        neg     A2
        test    T1, T1
        jns     .one_of_each
        call    iemAImpl_negate_T0_T1_u64
.both_positive:                         ; Same as unsigned shifted by sign indicator bit.
        shl     T1, 1
        shr     T0, 63
        or      T1, T0
        cmp     T1, A2
        jae     .div_overflow
.div_no_overflow:
        pop     A2
  %endif

        IEM_MAYBE_LOAD_FLAGS A3, %2, %3
        mov     rax, [A0]
  %ifdef ASM_CALL64_GCC
        mov     T1, A2
        mov     rax, [A0]
        mov     rdx, [A1]
        %1      T1
        mov     [A0], rax
        mov     [A1], rdx
  %else
        mov     T1, A1
        mov     rax, [A0]
        mov     rdx, [T1]
        %1      A2
        mov     [A0], rax
        mov     [T1], rdx
  %endif
        IEM_SAVE_FLAGS       A3, %2, %3
        xor     eax, eax

.return:
        EPILOGUE_4_ARGS_EX 12

.div_overflow:
  %if %4 != 0
        pop     A2
  %endif
.div_zero:
        mov     eax, -1
        jmp     .return
ENDPROC iemAImpl_ %+ %1 %+ _u64
 %endif ; !RT_ARCH_AMD64

%endmacro

IEMIMPL_DIV_OP div,  0, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 0
IEMIMPL_DIV_OP idiv, 0, (X86_EFL_OF | X86_EFL_SF | X86_EFL_ZF | X86_EFL_AF | X86_EFL_PF | X86_EFL_CF), 1


;
; BSWAP. No flag changes.
;
; Each function takes one argument, pointer to the value to bswap
; (input/output). They all return void.
;
BEGINPROC_FASTCALL iemAImpl_bswap_u16, 4
        PROLOGUE_1_ARGS
        mov     T0_32, [A0]             ; just in case any of the upper bits are used.
        db 66h
        bswap   T0_32
        mov     [A0], T0_32
        EPILOGUE_1_ARGS
ENDPROC iemAImpl_bswap_u16

BEGINPROC_FASTCALL iemAImpl_bswap_u32, 4
        PROLOGUE_1_ARGS
        mov     T0_32, [A0]
        bswap   T0_32
        mov     [A0], T0_32
        EPILOGUE_1_ARGS
ENDPROC iemAImpl_bswap_u32

BEGINPROC_FASTCALL iemAImpl_bswap_u64, 4
%ifdef RT_ARCH_AMD64
        PROLOGUE_1_ARGS
        mov     T0, [A0]
        bswap   T0
        mov     [A0], T0
        EPILOGUE_1_ARGS
%else
        PROLOGUE_1_ARGS
        mov     T0, [A0]
        mov     T1, [A0 + 4]
        bswap   T0
        bswap   T1
        mov     [A0 + 4], T0
        mov     [A0], T1
        EPILOGUE_1_ARGS
%endif
ENDPROC iemAImpl_bswap_u64


;;
; Initialize the FPU for the actual instruction being emulated, this means
; loading parts of the guest's control word and status word.
;
; @uses     24 bytes of stack.
; @param    1       Expression giving the address of the FXSTATE of the guest.
;
%macro FPU_LD_FXSTATE_FCW_AND_SAFE_FSW 1
        fnstenv [xSP]

        ; FCW - for exception, precision and rounding control.
        movzx   T0, word [%1 + X86FXSTATE.FCW]
        and     T0, X86_FCW_MASK_ALL | X86_FCW_PC_MASK | X86_FCW_RC_MASK
        mov     [xSP + X86FSTENV32P.FCW], T0_16

        ; FSW - for undefined C0, C1, C2, and C3.
        movzx   T1, word [%1 + X86FXSTATE.FSW]
        and     T1, X86_FSW_C_MASK
        movzx   T0, word [xSP + X86FSTENV32P.FSW]
        and     T0, X86_FSW_TOP_MASK
        or      T0, T1
        mov     [xSP + X86FSTENV32P.FSW], T0_16

        fldenv  [xSP]
%endmacro


;;
; Need to move this as well somewhere better?
;
struc IEMFPURESULT
    .r80Result  resw 5
    .FSW        resw 1
endstruc


;;
; Need to move this as well somewhere better?
;
struc IEMFPURESULTTWO
    .r80Result1 resw 5
    .FSW        resw 1
    .r80Result2 resw 5
endstruc


;
;---------------------- 16-bit signed integer operations ----------------------
;


;;
; Converts a 16-bit floating point value to a 80-bit one (fpu register).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 16-bit floating point value to convert.
;
BEGINPROC_FASTCALL iemAImpl_fild_i16_to_r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fild    word [A2]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_fild_i16_to_r80


;;
; Store a 80-bit floating point value (register) as a 16-bit signed integer (memory).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 16-bit signed integer value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fist_r80_to_i16, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fistp   word [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fist_r80_to_i16


;;
; Store a 80-bit floating point value (register) as a 16-bit signed integer
; (memory) with truncation.
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 16-bit signed integer value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fistt_r80_to_i16, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fisttp  dword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fistt_r80_to_i16


;;
; FPU instruction working on one 80-bit and one 16-bit signed integer value.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 16-bit value.
;
%macro IEMIMPL_FPU_R80_BY_I16 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_i16, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      word [A3]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_i16
%endmacro

IEMIMPL_FPU_R80_BY_I16 fiadd
IEMIMPL_FPU_R80_BY_I16 fimul
IEMIMPL_FPU_R80_BY_I16 fisub
IEMIMPL_FPU_R80_BY_I16 fisubr
IEMIMPL_FPU_R80_BY_I16 fidiv
IEMIMPL_FPU_R80_BY_I16 fidivr


;;
; FPU instruction working on one 80-bit and one 16-bit signed integer value,
; only returning FSW.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to store the output FSW.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 64-bit value.
;
%macro IEMIMPL_FPU_R80_BY_I16_FSW 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_i16, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      word [A3]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_i16
%endmacro

IEMIMPL_FPU_R80_BY_I16_FSW ficom



;
;---------------------- 32-bit signed integer operations ----------------------
;


;;
; Converts a 32-bit floating point value to a 80-bit one (fpu register).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 32-bit floating point value to convert.
;
BEGINPROC_FASTCALL iemAImpl_fild_i32_to_r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fild    dword [A2]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_fild_i32_to_r80


;;
; Store a 80-bit floating point value (register) as a 32-bit signed integer (memory).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 32-bit signed integer value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fist_r80_to_i32, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fistp   dword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fist_r80_to_i32


;;
; Store a 80-bit floating point value (register) as a 32-bit signed integer
; (memory) with truncation.
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 32-bit signed integer value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fistt_r80_to_i32, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fisttp  dword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fistt_r80_to_i32


;;
; FPU instruction working on one 80-bit and one 32-bit signed integer value.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 32-bit value.
;
%macro IEMIMPL_FPU_R80_BY_I32 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_i32, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      dword [A3]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_i32
%endmacro

IEMIMPL_FPU_R80_BY_I32 fiadd
IEMIMPL_FPU_R80_BY_I32 fimul
IEMIMPL_FPU_R80_BY_I32 fisub
IEMIMPL_FPU_R80_BY_I32 fisubr
IEMIMPL_FPU_R80_BY_I32 fidiv
IEMIMPL_FPU_R80_BY_I32 fidivr


;;
; FPU instruction working on one 80-bit and one 32-bit signed integer value,
; only returning FSW.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to store the output FSW.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 64-bit value.
;
%macro IEMIMPL_FPU_R80_BY_I32_FSW 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_i32, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      dword [A3]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_i32
%endmacro

IEMIMPL_FPU_R80_BY_I32_FSW ficom



;
;---------------------- 64-bit signed integer operations ----------------------
;


;;
; Converts a 64-bit floating point value to a 80-bit one (fpu register).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 64-bit floating point value to convert.
;
BEGINPROC_FASTCALL iemAImpl_fild_i64_to_r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fild    qword [A2]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_fild_i64_to_r80


;;
; Store a 80-bit floating point value (register) as a 64-bit signed integer (memory).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 64-bit signed integer value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fist_r80_to_i64, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fistp   qword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fist_r80_to_i64


;;
; Store a 80-bit floating point value (register) as a 64-bit signed integer
; (memory) with truncation.
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 64-bit signed integer value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fistt_r80_to_i64, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fisttp  qword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fistt_r80_to_i64



;
;---------------------- 32-bit floating point operations ----------------------
;

;;
; Converts a 32-bit floating point value to a 80-bit one (fpu register).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 32-bit floating point value to convert.
;
BEGINPROC_FASTCALL iemAImpl_fld_r32_to_r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fld     dword [A2]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_fld_r32_to_r80


;;
; Store a 80-bit floating point value (register) as a 32-bit one (memory).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 32-bit value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fst_r80_to_r32, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fst     dword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fst_r80_to_r32


;;
; FPU instruction working on one 80-bit and one 32-bit floating point value.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 32-bit value.
;
%macro IEMIMPL_FPU_R80_BY_R32 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r32, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      dword [A3]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r32
%endmacro

IEMIMPL_FPU_R80_BY_R32 fadd
IEMIMPL_FPU_R80_BY_R32 fmul
IEMIMPL_FPU_R80_BY_R32 fsub
IEMIMPL_FPU_R80_BY_R32 fsubr
IEMIMPL_FPU_R80_BY_R32 fdiv
IEMIMPL_FPU_R80_BY_R32 fdivr


;;
; FPU instruction working on one 80-bit and one 32-bit floating point value,
; only returning FSW.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to store the output FSW.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 64-bit value.
;
%macro IEMIMPL_FPU_R80_BY_R32_FSW 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r32, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      dword [A3]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r32
%endmacro

IEMIMPL_FPU_R80_BY_R32_FSW fcom



;
;---------------------- 64-bit floating point operations ----------------------
;

;;
; Converts a 64-bit floating point value to a 80-bit one (fpu register).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 64-bit floating point value to convert.
;
BEGINPROC_FASTCALL iemAImpl_fld_r64_to_r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fld     qword [A2]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_fld_r64_to_r80


;;
; Store a 80-bit floating point value (register) as a 64-bit one (memory).
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 64-bit value.
; @param    A3      Pointer to the 80-bit value.
;
BEGINPROC_FASTCALL iemAImpl_fst_r80_to_r64, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fst     qword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fst_r80_to_r64


;;
; FPU instruction working on one 80-bit and one 64-bit floating point value.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 64-bit value.
;
%macro IEMIMPL_FPU_R80_BY_R64 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r64, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      qword [A3]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r64
%endmacro

IEMIMPL_FPU_R80_BY_R64 fadd
IEMIMPL_FPU_R80_BY_R64 fmul
IEMIMPL_FPU_R80_BY_R64 fsub
IEMIMPL_FPU_R80_BY_R64 fsubr
IEMIMPL_FPU_R80_BY_R64 fdiv
IEMIMPL_FPU_R80_BY_R64 fdivr

;;
; FPU instruction working on one 80-bit and one 64-bit floating point value,
; only returning FSW.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to store the output FSW.
; @param    A2      Pointer to the 80-bit value.
; @param    A3      Pointer to the 64-bit value.
;
%macro IEMIMPL_FPU_R80_BY_R64_FSW 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r64, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      qword [A3]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r64
%endmacro

IEMIMPL_FPU_R80_BY_R64_FSW fcom



;
;---------------------- 80-bit floating point operations ----------------------
;

;;
; Loads a 80-bit floating point register value from memory.
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 80-bit floating point value to load.
;
BEGINPROC_FASTCALL iemAImpl_fld_r80_from_r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fld     tword [A2]

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_fld_r80_from_r80


;;
; Store a 80-bit floating point register to memory
;
; @param    A0      FPU context (fxsave).
; @param    A1      Where to return the output FSW.
; @param    A2      Where to store the 80-bit value.
; @param    A3      Pointer to the 80-bit register value.
;
BEGINPROC_FASTCALL iemAImpl_fst_r80_to_r80, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        fstp    tword [A2]

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_fst_r80_to_r80


;;
; FPU instruction working on two 80-bit floating point values.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the first 80-bit value (ST0)
; @param    A3      Pointer to the second 80-bit value (STn).
;
%macro IEMIMPL_FPU_R80_BY_R80 2
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r80, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      %2

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r80
%endmacro

IEMIMPL_FPU_R80_BY_R80 fadd,   {st0, st1}
IEMIMPL_FPU_R80_BY_R80 fmul,   {st0, st1}
IEMIMPL_FPU_R80_BY_R80 fsub,   {st0, st1}
IEMIMPL_FPU_R80_BY_R80 fsubr,  {st0, st1}
IEMIMPL_FPU_R80_BY_R80 fdiv,   {st0, st1}
IEMIMPL_FPU_R80_BY_R80 fdivr,  {st0, st1}
IEMIMPL_FPU_R80_BY_R80 fprem,  {}
IEMIMPL_FPU_R80_BY_R80 fprem1, {}
IEMIMPL_FPU_R80_BY_R80 fscale, {}


;;
; FPU instruction working on two 80-bit floating point values, ST1 and ST0,
; storing the result in ST1 and popping the stack.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the first 80-bit value (ST1).
; @param    A3      Pointer to the second 80-bit value (ST0).
;
%macro IEMIMPL_FPU_R80_BY_R80_ST1_ST0_POP 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r80, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        fld     tword [A3]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r80
%endmacro

IEMIMPL_FPU_R80_BY_R80_ST1_ST0_POP fpatan
IEMIMPL_FPU_R80_BY_R80_ST1_ST0_POP fyl2x
IEMIMPL_FPU_R80_BY_R80_ST1_ST0_POP fyl2xp1


;;
; FPU instruction working on two 80-bit floating point values, only
; returning FSW.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a uint16_t for the resulting FSW.
; @param    A2      Pointer to the first 80-bit value.
; @param    A3      Pointer to the second 80-bit value.
;
%macro IEMIMPL_FPU_R80_BY_R80_FSW 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r80, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      st0, st1

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r80
%endmacro

IEMIMPL_FPU_R80_BY_R80_FSW fcom
IEMIMPL_FPU_R80_BY_R80_FSW fucom


;;
; FPU instruction working on two 80-bit floating point values,
; returning FSW and EFLAGS (eax).
;
; @param    1       The instruction
;
; @returns  EFLAGS in EAX.
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a uint16_t for the resulting FSW.
; @param    A2      Pointer to the first 80-bit value.
; @param    A3      Pointer to the second 80-bit value.
;
%macro IEMIMPL_FPU_R80_BY_R80_EFL 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_by_r80, 16
        PROLOGUE_4_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A3]
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1      st1

        fnstsw  word  [A1]
        pushf
        pop     xAX

        fninit
        add     xSP, 20h
        EPILOGUE_4_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_by_r80
%endmacro

IEMIMPL_FPU_R80_BY_R80_EFL fcomi
IEMIMPL_FPU_R80_BY_R80_EFL fucomi


;;
; FPU instruction working on one 80-bit floating point value.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
; @param    A2      Pointer to the 80-bit value.
;
%macro IEMIMPL_FPU_R80 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80
%endmacro

IEMIMPL_FPU_R80 fchs
IEMIMPL_FPU_R80 fabs
IEMIMPL_FPU_R80 f2xm1
IEMIMPL_FPU_R80 fsqrt
IEMIMPL_FPU_R80 frndint
IEMIMPL_FPU_R80 fsin
IEMIMPL_FPU_R80 fcos


;;
; FPU instruction working on one 80-bit floating point value, only
; returning FSW.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a uint16_t for the resulting FSW.
; @param    A2      Pointer to the 80-bit value.
;
%macro IEMIMPL_FPU_R80_FSW 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1

        fnstsw  word  [A1]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80
%endmacro

IEMIMPL_FPU_R80_FSW ftst
IEMIMPL_FPU_R80_FSW fxam



;;
; FPU instruction loading a 80-bit floating point constant.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULT for the output.
;
%macro IEMIMPL_FPU_R80_CONST 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1, 8
        PROLOGUE_2_ARGS
        sub     xSP, 20h

        fninit
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1

        fnstsw  word  [A1 + IEMFPURESULT.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULT.r80Result]

        fninit
        add     xSP, 20h
        EPILOGUE_2_ARGS
ENDPROC iemAImpl_ %+ %1 %+
%endmacro

IEMIMPL_FPU_R80_CONST fld1
IEMIMPL_FPU_R80_CONST fldl2t
IEMIMPL_FPU_R80_CONST fldl2e
IEMIMPL_FPU_R80_CONST fldpi
IEMIMPL_FPU_R80_CONST fldlg2
IEMIMPL_FPU_R80_CONST fldln2
IEMIMPL_FPU_R80_CONST fldz


;;
; FPU instruction working on one 80-bit floating point value, outputing two.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to a IEMFPURESULTTWO for the output.
; @param    A2      Pointer to the 80-bit value.
;
%macro IEMIMPL_FPU_R80_R80 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _r80_r80, 12
        PROLOGUE_3_ARGS
        sub     xSP, 20h

        fninit
        fld     tword [A2]
        FPU_LD_FXSTATE_FCW_AND_SAFE_FSW A0
        %1

        fnstsw  word  [A1 + IEMFPURESULTTWO.FSW]
        fnclex
        fstp    tword [A1 + IEMFPURESULTTWO.r80Result2]
        fnclex
        fstp    tword [A1 + IEMFPURESULTTWO.r80Result1]

        fninit
        add     xSP, 20h
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _r80_r80
%endmacro

IEMIMPL_FPU_R80_R80 fptan
IEMIMPL_FPU_R80_R80 fxtract
IEMIMPL_FPU_R80_R80 fsincos




;---------------------- SSE and MMX Operations ----------------------

;; @todo what do we need to do for MMX?
%macro IEMIMPL_MMX_PROLOGUE 0
%endmacro
%macro IEMIMPL_MMX_EPILOGUE 0
%endmacro

;; @todo what do we need to do for SSE?
%macro IEMIMPL_SSE_PROLOGUE 0
%endmacro
%macro IEMIMPL_SSE_EPILOGUE 0
%endmacro


;;
; Media instruction working on two full sized registers.
;
; @param    1       The instruction
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to the first media register size operand (input/output).
; @param    A2      Pointer to the second media register size operand (input).
;
%macro IEMIMPL_MEDIA_F2 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 12
        PROLOGUE_3_ARGS
        IEMIMPL_MMX_PROLOGUE

        movq    mm0, [A1]
        movq    mm1, [A2]
        %1      mm0, mm1
        movq    [A1], mm0

        IEMIMPL_MMX_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u64

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u128, 12
        PROLOGUE_3_ARGS
        IEMIMPL_SSE_PROLOGUE

        movdqu   xmm0, [A1]
        movdqu   xmm1, [A2]
        %1       xmm0, xmm1
        movdqu   [A1], xmm0

        IEMIMPL_SSE_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u128
%endmacro

IEMIMPL_MEDIA_F2 pxor
IEMIMPL_MEDIA_F2 pcmpeqb
IEMIMPL_MEDIA_F2 pcmpeqw
IEMIMPL_MEDIA_F2 pcmpeqd


;;
; Media instruction working on one full sized and one half sized register (lower half).
;
; @param    1       The instruction
; @param    2       1 if MMX is included, 0 if not.
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to the first full sized media register operand (input/output).
; @param    A2      Pointer to the second half sized media register operand (input).
;
%macro IEMIMPL_MEDIA_F1L1 2
 %if %2 != 0
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 12
        PROLOGUE_3_ARGS
        IEMIMPL_MMX_PROLOGUE

        movq    mm0, [A1]
        movd    mm1, [A2]
        %1      mm0, mm1
        movq    [A1], mm0

        IEMIMPL_MMX_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u64
 %endif

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u128, 12
        PROLOGUE_3_ARGS
        IEMIMPL_SSE_PROLOGUE

        movdqu   xmm0, [A1]
        movq     xmm1, [A2]
        %1       xmm0, xmm1
        movdqu   [A1], xmm0

        IEMIMPL_SSE_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u128
%endmacro

IEMIMPL_MEDIA_F1L1 punpcklbw,  1
IEMIMPL_MEDIA_F1L1 punpcklwd,  1
IEMIMPL_MEDIA_F1L1 punpckldq,  1
IEMIMPL_MEDIA_F1L1 punpcklqdq, 0


;;
; Media instruction working on one full sized and one half sized register (high half).
;
; @param    1       The instruction
; @param    2       1 if MMX is included, 0 if not.
;
; @param    A0      FPU context (fxsave).
; @param    A1      Pointer to the first full sized media register operand (input/output).
; @param    A2      Pointer to the second full sized media register operand, where we
;                   will only use the upper half (input).
;
%macro IEMIMPL_MEDIA_F1H1 2
 %if %2 != 0
BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u64, 12
        PROLOGUE_3_ARGS
        IEMIMPL_MMX_PROLOGUE

        movq    mm0, [A1]
        movq    mm1, [A2]
        %1      mm0, mm1
        movq    [A1], mm0

        IEMIMPL_MMX_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u64
 %endif

BEGINPROC_FASTCALL iemAImpl_ %+ %1 %+ _u128, 12
        PROLOGUE_3_ARGS
        IEMIMPL_SSE_PROLOGUE

        movdqu   xmm0, [A1]
        movdqu   xmm1, [A2]
        %1       xmm0, xmm1
        movdqu   [A1], xmm0

        IEMIMPL_SSE_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_ %+ %1 %+ _u128
%endmacro

IEMIMPL_MEDIA_F1L1 punpckhbw,  1
IEMIMPL_MEDIA_F1L1 punpckhwd,  1
IEMIMPL_MEDIA_F1L1 punpckhdq,  1
IEMIMPL_MEDIA_F1L1 punpckhqdq, 0


;
; Shufflers with evil 8-bit immediates.
;

BEGINPROC_FASTCALL iemAImpl_pshufw, 16
        PROLOGUE_4_ARGS
        IEMIMPL_MMX_PROLOGUE

        movq    mm0, [A1]
        movq    mm1, [A2]
        lea     T0, [A3 + A3*4]         ; sizeof(pshufw+ret) == 5
        lea     T1, [.imm0 xWrtRIP]
        lea     T1, [T1 + T0]
        call    T1
        movq    [A1], mm0

        IEMIMPL_MMX_EPILOGUE
        EPILOGUE_4_ARGS
%assign bImm 0
%rep 256
.imm %+ bImm:
       pshufw    mm0, mm1, bImm
       ret
 %assign bImm bImm + 1
%endrep
.immEnd:                                ; 256*5 == 0x500
dw 0xfaff  + (.immEnd - .imm0)          ; will cause warning if entries are too big.
dw 0x104ff - (.immEnd - .imm0)          ; will cause warning if entries are small big.
ENDPROC iemAImpl_pshufw


%macro IEMIMPL_MEDIA_SSE_PSHUFXX 1
BEGINPROC_FASTCALL iemAImpl_ %+ %1, 16
        PROLOGUE_4_ARGS
        IEMIMPL_SSE_PROLOGUE

        movdqu  xmm0, [A1]
        movdqu  xmm1, [A2]
        lea     T1, [.imm0 xWrtRIP]
        lea     T0, [A3 + A3*2]         ; sizeof(pshufXX+ret) == 6: (A3 * 3) *2
        lea     T1, [T1 + T0*2]
        call    T1
        movdqu  [A1], xmm0

        IEMIMPL_SSE_EPILOGUE
        EPILOGUE_4_ARGS
 %assign bImm 0
 %rep 256
.imm %+ bImm:
       %1       xmm0, xmm1, bImm
       ret
  %assign bImm bImm + 1
 %endrep
.immEnd:                                ; 256*6 == 0x600
dw 0xf9ff  + (.immEnd - .imm0)          ; will cause warning if entries are too big.
dw 0x105ff - (.immEnd - .imm0)          ; will cause warning if entries are small big.
ENDPROC iemAImpl_ %+ %1
%endmacro

IEMIMPL_MEDIA_SSE_PSHUFXX pshufhw
IEMIMPL_MEDIA_SSE_PSHUFXX pshuflw
IEMIMPL_MEDIA_SSE_PSHUFXX pshufd


;
; Move byte mask.
;

BEGINPROC_FASTCALL iemAImpl_pmovmskb_u64, 12
        PROLOGUE_3_ARGS
        IEMIMPL_MMX_PROLOGUE

        mov     T0,  [A1]
        movq    mm1, [A2]
        pmovmskb T0, mm1
        mov     [A1], T0
%ifdef RT_ARCH_X86
        mov     dword [A1 + 4], 0
%endif
        IEMIMPL_MMX_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_pmovmskb_u64

BEGINPROC_FASTCALL iemAImpl_pmovmskb_u128, 12
        PROLOGUE_3_ARGS
        IEMIMPL_SSE_PROLOGUE

        mov     T0,  [A1]
        movdqu  xmm1, [A2]
        pmovmskb T0, xmm1
        mov     [A1], T0
%ifdef RT_ARCH_X86
        mov     dword [A1 + 4], 0
%endif
        IEMIMPL_SSE_EPILOGUE
        EPILOGUE_3_ARGS
ENDPROC iemAImpl_pmovmskb_u128


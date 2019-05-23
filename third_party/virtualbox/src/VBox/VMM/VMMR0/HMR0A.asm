; $Id: HMR0A.asm $
;; @file
; HM - Ring-0 VMX, SVM world-switch and helper routines
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

;*********************************************************************************************************************************
;*  Header Files                                                                                                                 *
;*********************************************************************************************************************************
%include "VBox/asmdefs.mac"
%include "VBox/err.mac"
%include "VBox/vmm/hm_vmx.mac"
%include "VBox/vmm/cpum.mac"
%include "VBox/vmm/vm.mac"
%include "iprt/x86.mac"
%include "HMInternal.mac"

%ifdef RT_OS_OS2 ;; @todo fix OMF support in yasm and kick nasm out completely.
 %macro vmwrite 2,
    int3
 %endmacro
 %define vmlaunch int3
 %define vmresume int3
 %define vmsave int3
 %define vmload int3
 %define vmrun int3
 %define clgi int3
 %define stgi int3
 %macro invlpga 2,
    int3
 %endmacro
%endif

;*********************************************************************************************************************************
;*  Defined Constants And Macros                                                                                                 *
;*********************************************************************************************************************************
;; The offset of the XMM registers in X86FXSTATE.
; Use define because I'm too lazy to convert the struct.
%define XMM_OFF_IN_X86FXSTATE   160

;; Spectre filler for 32-bit mode.
; Some user space address that points to a 4MB page boundrary in hope that it
; will somehow make it less useful.
%define SPECTRE_FILLER32        0x227fffff
;; Spectre filler for 64-bit mode.
; Choosen to be an invalid address (also with 5 level paging).
%define SPECTRE_FILLER64        0x02204204207fffff
;; Spectre filler for the current CPU mode.
%ifdef RT_ARCH_AMD64
 %define SPECTRE_FILLER         SPECTRE_FILLER64
%else
 %define SPECTRE_FILLER         SPECTRE_FILLER32
%endif

;;
; Determine skipping restoring of GDTR, IDTR, TR across VMX non-root operation
;
%ifdef RT_ARCH_AMD64
 %define VMX_SKIP_GDTR
 %define VMX_SKIP_TR
 %define VBOX_SKIP_RESTORE_SEG
 %ifdef RT_OS_DARWIN
  ; Load the NULL selector into DS, ES, FS and GS on 64-bit darwin so we don't
  ; risk loading a stale LDT value or something invalid.
  %define HM_64_BIT_USE_NULL_SEL
  ; Darwin (Mavericks) uses IDTR limit to store the CPU Id so we need to restore it always.
  ; See @bugref{6875}.
 %else
  %define VMX_SKIP_IDTR
 %endif
%endif

;; @def MYPUSHAD
; Macro generating an equivalent to pushad

;; @def MYPOPAD
; Macro generating an equivalent to popad

;; @def MYPUSHSEGS
; Macro saving all segment registers on the stack.
; @param 1  full width register name
; @param 2  16-bit register name for \a 1.

;; @def MYPOPSEGS
; Macro restoring all segment registers on the stack
; @param 1  full width register name
; @param 2  16-bit register name for \a 1.

%ifdef ASM_CALL64_GCC
 %macro MYPUSHAD64 0
   push    r15
   push    r14
   push    r13
   push    r12
   push    rbx
 %endmacro
 %macro MYPOPAD64 0
   pop     rbx
   pop     r12
   pop     r13
   pop     r14
   pop     r15
 %endmacro

%else ; ASM_CALL64_MSC
 %macro MYPUSHAD64 0
   push    r15
   push    r14
   push    r13
   push    r12
   push    rbx
   push    rsi
   push    rdi
 %endmacro
 %macro MYPOPAD64 0
   pop     rdi
   pop     rsi
   pop     rbx
   pop     r12
   pop     r13
   pop     r14
   pop     r15
 %endmacro
%endif

%ifdef VBOX_SKIP_RESTORE_SEG
 %macro MYPUSHSEGS64 2
 %endmacro

 %macro MYPOPSEGS64 2
 %endmacro
%else       ; !VBOX_SKIP_RESTORE_SEG
 ; trashes, rax, rdx & rcx
 %macro MYPUSHSEGS64 2
  %ifndef HM_64_BIT_USE_NULL_SEL
   mov     %2, es
   push    %1
   mov     %2, ds
   push    %1
  %endif

   ; Special case for FS; Windows and Linux either don't use it or restore it when leaving kernel mode, Solaris OTOH doesn't and we must save it.
   mov     ecx, MSR_K8_FS_BASE
   rdmsr
   push    rdx
   push    rax
  %ifndef HM_64_BIT_USE_NULL_SEL
   push    fs
  %endif

   ; Special case for GS; OSes typically use swapgs to reset the hidden base register for GS on entry into the kernel. The same happens on exit
   mov     ecx, MSR_K8_GS_BASE
   rdmsr
   push    rdx
   push    rax
  %ifndef HM_64_BIT_USE_NULL_SEL
   push    gs
  %endif
 %endmacro

 ; trashes, rax, rdx & rcx
 %macro MYPOPSEGS64 2
   ; Note: do not step through this code with a debugger!
  %ifndef HM_64_BIT_USE_NULL_SEL
   xor     eax, eax
   mov     ds, ax
   mov     es, ax
   mov     fs, ax
   mov     gs, ax
  %endif

  %ifndef HM_64_BIT_USE_NULL_SEL
   pop     gs
  %endif
   pop     rax
   pop     rdx
   mov     ecx, MSR_K8_GS_BASE
   wrmsr

  %ifndef HM_64_BIT_USE_NULL_SEL
   pop     fs
  %endif
   pop     rax
   pop     rdx
   mov     ecx, MSR_K8_FS_BASE
   wrmsr
   ; Now it's safe to step again

  %ifndef HM_64_BIT_USE_NULL_SEL
   pop     %1
   mov     ds, %2
   pop     %1
   mov     es, %2
  %endif
 %endmacro
%endif ; VBOX_SKIP_RESTORE_SEG

%macro MYPUSHAD32 0
  pushad
%endmacro
%macro MYPOPAD32 0
  popad
%endmacro

%macro MYPUSHSEGS32 2
  push    ds
  push    es
  push    fs
  push    gs
%endmacro
%macro MYPOPSEGS32 2
  pop     gs
  pop     fs
  pop     es
  pop     ds
%endmacro

%ifdef RT_ARCH_AMD64
 %define MYPUSHAD       MYPUSHAD64
 %define MYPOPAD        MYPOPAD64
 %define MYPUSHSEGS     MYPUSHSEGS64
 %define MYPOPSEGS      MYPOPSEGS64
%else
 %define MYPUSHAD       MYPUSHAD32
 %define MYPOPAD        MYPOPAD32
 %define MYPUSHSEGS     MYPUSHSEGS32
 %define MYPOPSEGS      MYPOPSEGS32
%endif

;;
; Creates an indirect branch prediction barrier on CPUs that need and supports that.
; @clobbers eax, edx, ecx
; @param    1   How to address CPUMCTX.
; @param    2   Which flag to test for (CPUMCTX_WSF_IBPB_ENTRY or CPUMCTX_WSF_IBPB_EXIT)
%macro INDIRECT_BRANCH_PREDICTION_BARRIER 2
    test    byte [%1 + CPUMCTX.fWorldSwitcher], %2
    jz      %%no_indirect_branch_barrier
    mov     ecx, MSR_IA32_PRED_CMD
    mov     eax, MSR_IA32_PRED_CMD_F_IBPB
    xor     edx, edx
    wrmsr
%%no_indirect_branch_barrier:
%endmacro


;*********************************************************************************************************************************
;*  External Symbols                                                                                                             *
;*********************************************************************************************************************************
%ifdef VBOX_WITH_KERNEL_USING_XMM
extern NAME(CPUMIsGuestFPUStateActive)
%endif


BEGINCODE


;/**
; * Restores host-state fields.
; *
; * @returns VBox status code
; * @param   f32RestoreHost x86: [ebp + 08h]  msc: ecx  gcc: edi   RestoreHost flags.
; * @param   pRestoreHost   x86: [ebp + 0ch]  msc: rdx  gcc: rsi   Pointer to the RestoreHost struct.
; */
ALIGNCODE(16)
BEGINPROC VMXRestoreHostState
%ifdef RT_ARCH_AMD64
 %ifndef ASM_CALL64_GCC
    ; Use GCC's input registers since we'll be needing both rcx and rdx further
    ; down with the wrmsr instruction.  Use the R10 and R11 register for saving
    ; RDI and RSI since MSC preserve the two latter registers.
    mov         r10, rdi
    mov         r11, rsi
    mov         rdi, rcx
    mov         rsi, rdx
 %endif

    test        edi, VMX_RESTORE_HOST_GDTR
    jz          .test_idtr
    lgdt        [rsi + VMXRESTOREHOST.HostGdtr]

.test_idtr:
    test        edi, VMX_RESTORE_HOST_IDTR
    jz          .test_ds
    lidt        [rsi + VMXRESTOREHOST.HostIdtr]

.test_ds:
    test        edi, VMX_RESTORE_HOST_SEL_DS
    jz          .test_es
    mov         ax, [rsi + VMXRESTOREHOST.uHostSelDS]
    mov         ds, eax

.test_es:
    test        edi, VMX_RESTORE_HOST_SEL_ES
    jz          .test_tr
    mov         ax, [rsi + VMXRESTOREHOST.uHostSelES]
    mov         es, eax

.test_tr:
    test        edi, VMX_RESTORE_HOST_SEL_TR
    jz          .test_fs
    ; When restoring the TR, we must first clear the busy flag or we'll end up faulting.
    mov         dx, [rsi + VMXRESTOREHOST.uHostSelTR]
    mov         ax, dx
    and         eax, X86_SEL_MASK_OFF_RPL                       ; Mask away TI and RPL bits leaving only the descriptor offset.
    test        edi, VMX_RESTORE_HOST_GDT_READ_ONLY | VMX_RESTORE_HOST_GDT_NEED_WRITABLE
    jnz         .gdt_readonly
    add         rax, qword [rsi + VMXRESTOREHOST.HostGdtr + 2]  ; xAX <- descriptor offset + GDTR.pGdt.
    and         dword [rax + 4], ~RT_BIT(9)                     ; Clear the busy flag in TSS desc (bits 0-7=base, bit 9=busy bit).
    ltr         dx
    jmp short   .test_fs
.gdt_readonly:
    test        edi, VMX_RESTORE_HOST_GDT_NEED_WRITABLE
    jnz         .gdt_readonly_need_writable
    mov         rcx, cr0
    mov         r9, rcx
    add         rax, qword [rsi + VMXRESTOREHOST.HostGdtr + 2]  ; xAX <- descriptor offset + GDTR.pGdt.
    and         rcx, ~X86_CR0_WP
    mov         cr0, rcx
    and         dword [rax + 4], ~RT_BIT(9)                     ; Clear the busy flag in TSS desc (bits 0-7=base, bit 9=busy bit).
    ltr         dx
    mov         cr0, r9
    jmp short   .test_fs
.gdt_readonly_need_writable:
    add         rax, qword [rsi + VMXRESTOREHOST.HostGdtrRw + 2]  ; xAX <- descriptor offset + GDTR.pGdtRw.
    and         dword [rax + 4], ~RT_BIT(9)                     ; Clear the busy flag in TSS desc (bits 0-7=base, bit 9=busy bit).
    lgdt        [rsi + VMXRESTOREHOST.HostGdtrRw]
    ltr         dx
    lgdt        [rsi + VMXRESTOREHOST.HostGdtr]                 ; Load the original GDT

.test_fs:
    ;
    ; When restoring the selector values for FS and GS, we'll temporarily trash
    ; the base address (at least the high 32-bit bits, but quite possibly the
    ; whole base address), the wrmsr will restore it correctly. (VT-x actually
    ; restores the base correctly when leaving guest mode, but not the selector
    ; value, so there is little problem with interrupts being enabled prior to
    ; this restore job.)
    ; We'll disable ints once for both FS and GS as that's probably faster.
    ;
    test        edi, VMX_RESTORE_HOST_SEL_FS | VMX_RESTORE_HOST_SEL_GS
    jz          .restore_success
    pushfq
    cli                                   ; (see above)

    test        edi, VMX_RESTORE_HOST_SEL_FS
    jz          .test_gs
    mov         ax, word [rsi + VMXRESTOREHOST.uHostSelFS]
    mov         fs, eax
    mov         eax, dword [rsi + VMXRESTOREHOST.uHostFSBase]         ; uHostFSBase - Lo
    mov         edx, dword [rsi + VMXRESTOREHOST.uHostFSBase + 4h]    ; uHostFSBase - Hi
    mov         ecx, MSR_K8_FS_BASE
    wrmsr

.test_gs:
    test        edi, VMX_RESTORE_HOST_SEL_GS
    jz          .restore_flags
    mov         ax, word [rsi + VMXRESTOREHOST.uHostSelGS]
    mov         gs, eax
    mov         eax, dword [rsi + VMXRESTOREHOST.uHostGSBase]         ; uHostGSBase - Lo
    mov         edx, dword [rsi + VMXRESTOREHOST.uHostGSBase + 4h]    ; uHostGSBase - Hi
    mov         ecx, MSR_K8_GS_BASE
    wrmsr

.restore_flags:
    popfq

.restore_success:
    mov         eax, VINF_SUCCESS
 %ifndef ASM_CALL64_GCC
    ; Restore RDI and RSI on MSC.
    mov         rdi, r10
    mov         rsi, r11
 %endif
%else  ; RT_ARCH_X86
    mov         eax, VERR_NOT_IMPLEMENTED
%endif
    ret
ENDPROC VMXRestoreHostState


;/**
; * Dispatches an NMI to the host.
; */
ALIGNCODE(16)
BEGINPROC VMXDispatchHostNmi
    int 2   ; NMI is always vector 2. The IDT[2] IRQ handler cannot be anything else. See Intel spec. 6.3.1 "External Interrupts".
    ret
ENDPROC VMXDispatchHostNmi


;/**
; * Executes VMWRITE, 64-bit value.
; *
; * @returns VBox status code.
; * @param   idxField   x86: [ebp + 08h]  msc: rcx  gcc: rdi   VMCS index.
; * @param   u64Data    x86: [ebp + 0ch]  msc: rdx  gcc: rsi   VM field value.
; */
ALIGNCODE(16)
BEGINPROC VMXWriteVmcs64
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    and         edi, 0ffffffffh
    xor         rax, rax
    vmwrite     rdi, rsi
 %else
    and         ecx, 0ffffffffh
    xor         rax, rax
    vmwrite     rcx, rdx
 %endif
%else  ; RT_ARCH_X86
    mov         ecx, [esp + 4]          ; idxField
    lea         edx, [esp + 8]          ; &u64Data
    vmwrite     ecx, [edx]              ; low dword
    jz          .done
    jc          .done
    inc         ecx
    xor         eax, eax
    vmwrite     ecx, [edx + 4]          ; high dword
.done:
%endif ; RT_ARCH_X86
    jnc         .valid_vmcs
    mov         eax, VERR_VMX_INVALID_VMCS_PTR
    ret
.valid_vmcs:
    jnz         .the_end
    mov         eax, VERR_VMX_INVALID_VMCS_FIELD
.the_end:
    ret
ENDPROC VMXWriteVmcs64


;/**
; * Executes VMREAD, 64-bit value.
; *
; * @returns VBox status code.
; * @param   idxField        VMCS index.
; * @param   pData           Where to store VM field value.
; */
;DECLASM(int) VMXReadVmcs64(uint32_t idxField, uint64_t *pData);
ALIGNCODE(16)
BEGINPROC VMXReadVmcs64
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    and         edi, 0ffffffffh
    xor         rax, rax
    vmread      [rsi], rdi
 %else
    and         ecx, 0ffffffffh
    xor         rax, rax
    vmread      [rdx], rcx
 %endif
%else  ; RT_ARCH_X86
    mov         ecx, [esp + 4]          ; idxField
    mov         edx, [esp + 8]          ; pData
    vmread      [edx], ecx              ; low dword
    jz          .done
    jc          .done
    inc         ecx
    xor         eax, eax
    vmread      [edx + 4], ecx          ; high dword
.done:
%endif ; RT_ARCH_X86
    jnc         .valid_vmcs
    mov         eax, VERR_VMX_INVALID_VMCS_PTR
    ret
.valid_vmcs:
    jnz         .the_end
    mov         eax, VERR_VMX_INVALID_VMCS_FIELD
.the_end:
    ret
ENDPROC VMXReadVmcs64


;/**
; * Executes VMREAD, 32-bit value.
; *
; * @returns VBox status code.
; * @param   idxField        VMCS index.
; * @param   pu32Data        Where to store VM field value.
; */
;DECLASM(int) VMXReadVmcs32(uint32_t idxField, uint32_t *pu32Data);
ALIGNCODE(16)
BEGINPROC VMXReadVmcs32
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    and     edi, 0ffffffffh
    xor     rax, rax
    vmread  r10, rdi
    mov     [rsi], r10d
 %else
    and     ecx, 0ffffffffh
    xor     rax, rax
    vmread  r10, rcx
    mov     [rdx], r10d
 %endif
%else  ; RT_ARCH_X86
    mov     ecx, [esp + 4]              ; idxField
    mov     edx, [esp + 8]              ; pu32Data
    xor     eax, eax
    vmread  [edx], ecx
%endif ; RT_ARCH_X86
    jnc     .valid_vmcs
    mov     eax, VERR_VMX_INVALID_VMCS_PTR
    ret
.valid_vmcs:
    jnz     .the_end
    mov     eax, VERR_VMX_INVALID_VMCS_FIELD
.the_end:
    ret
ENDPROC VMXReadVmcs32


;/**
; * Executes VMWRITE, 32-bit value.
; *
; * @returns VBox status code.
; * @param   idxField        VMCS index.
; * @param   u32Data         Where to store VM field value.
; */
;DECLASM(int) VMXWriteVmcs32(uint32_t idxField, uint32_t u32Data);
ALIGNCODE(16)
BEGINPROC VMXWriteVmcs32
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    and     edi, 0ffffffffh
    and     esi, 0ffffffffh
    xor     rax, rax
    vmwrite rdi, rsi
 %else
    and     ecx, 0ffffffffh
    and     edx, 0ffffffffh
    xor     rax, rax
    vmwrite rcx, rdx
 %endif
%else  ; RT_ARCH_X86
    mov     ecx, [esp + 4]              ; idxField
    mov     edx, [esp + 8]              ; u32Data
    xor     eax, eax
    vmwrite ecx, edx
%endif ; RT_ARCH_X86
    jnc     .valid_vmcs
    mov     eax, VERR_VMX_INVALID_VMCS_PTR
    ret
.valid_vmcs:
    jnz     .the_end
    mov     eax, VERR_VMX_INVALID_VMCS_FIELD
.the_end:
    ret
ENDPROC VMXWriteVmcs32


;/**
; * Executes VMXON.
; *
; * @returns VBox status code.
; * @param   HCPhysVMXOn      Physical address of VMXON structure.
; */
;DECLASM(int) VMXEnable(RTHCPHYS HCPhysVMXOn);
BEGINPROC VMXEnable
%ifdef RT_ARCH_AMD64
    xor     rax, rax
 %ifdef ASM_CALL64_GCC
    push    rdi
 %else
    push    rcx
 %endif
    vmxon   [rsp]
%else  ; RT_ARCH_X86
    xor     eax, eax
    vmxon   [esp + 4]
%endif ; RT_ARCH_X86
    jnc     .good
    mov     eax, VERR_VMX_INVALID_VMXON_PTR
    jmp     .the_end

.good:
    jnz     .the_end
    mov     eax, VERR_VMX_VMXON_FAILED

.the_end:
%ifdef RT_ARCH_AMD64
    add     rsp, 8
%endif
    ret
ENDPROC VMXEnable


;/**
; * Executes VMXOFF.
; */
;DECLASM(void) VMXDisable(void);
BEGINPROC VMXDisable
    vmxoff
.the_end:
    ret
ENDPROC VMXDisable


;/**
; * Executes VMCLEAR.
; *
; * @returns VBox status code.
; * @param   HCPhysVmcs     Physical address of VM control structure.
; */
;DECLASM(int) VMXClearVmcs(RTHCPHYS HCPhysVmcs);
ALIGNCODE(16)
BEGINPROC VMXClearVmcs
%ifdef RT_ARCH_AMD64
    xor     rax, rax
 %ifdef ASM_CALL64_GCC
    push    rdi
 %else
    push    rcx
 %endif
    vmclear [rsp]
%else  ; RT_ARCH_X86
    xor     eax, eax
    vmclear [esp + 4]
%endif ; RT_ARCH_X86
    jnc     .the_end
    mov     eax, VERR_VMX_INVALID_VMCS_PTR
.the_end:
%ifdef RT_ARCH_AMD64
    add     rsp, 8
%endif
    ret
ENDPROC VMXClearVmcs


;/**
; * Executes VMPTRLD.
; *
; * @returns VBox status code.
; * @param   HCPhysVmcs     Physical address of VMCS structure.
; */
;DECLASM(int) VMXActivateVmcs(RTHCPHYS HCPhysVmcs);
ALIGNCODE(16)
BEGINPROC VMXActivateVmcs
%ifdef RT_ARCH_AMD64
    xor     rax, rax
 %ifdef ASM_CALL64_GCC
    push    rdi
 %else
    push    rcx
 %endif
    vmptrld [rsp]
%else
    xor     eax, eax
    vmptrld [esp + 4]
%endif
    jnc     .the_end
    mov     eax, VERR_VMX_INVALID_VMCS_PTR
.the_end:
%ifdef RT_ARCH_AMD64
    add     rsp, 8
%endif
    ret
ENDPROC VMXActivateVmcs


;/**
; * Executes VMPTRST.
; *
; * @returns VBox status code.
; * @param    [esp + 04h]  gcc:rdi  msc:rcx   Param 1 - First parameter - Address that will receive the current pointer.
; */
;DECLASM(int) VMXGetActivatedVmcs(RTHCPHYS *pVMCS);
BEGINPROC VMXGetActivatedVmcs
%ifdef RT_OS_OS2
    mov     eax, VERR_NOT_SUPPORTED
    ret
%else
 %ifdef RT_ARCH_AMD64
  %ifdef ASM_CALL64_GCC
    vmptrst qword [rdi]
  %else
    vmptrst qword [rcx]
  %endif
 %else
    vmptrst qword [esp+04h]
 %endif
    xor     eax, eax
.the_end:
    ret
%endif
ENDPROC VMXGetActivatedVmcs

;/**
; * Invalidate a page using INVEPT.
; @param   enmFlush     msc:ecx  gcc:edi  x86:[esp+04]  Type of flush.
; @param   pDescriptor  msc:edx  gcc:esi  x86:[esp+08]  Descriptor pointer.
; */
;DECLASM(int) VMXR0InvEPT(VMX_FLUSH enmFlush, uint64_t *pDescriptor);
BEGINPROC VMXR0InvEPT
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    and         edi, 0ffffffffh
    xor         rax, rax
;    invept      rdi, qword [rsi]
    DB          0x66, 0x0F, 0x38, 0x80, 0x3E
 %else
    and         ecx, 0ffffffffh
    xor         rax, rax
;    invept      rcx, qword [rdx]
    DB          0x66, 0x0F, 0x38, 0x80, 0xA
 %endif
%else
    mov         ecx, [esp + 4]
    mov         edx, [esp + 8]
    xor         eax, eax
;    invept      ecx, qword [edx]
    DB          0x66, 0x0F, 0x38, 0x80, 0xA
%endif
    jnc         .valid_vmcs
    mov         eax, VERR_VMX_INVALID_VMCS_PTR
    ret
.valid_vmcs:
    jnz         .the_end
    mov         eax, VERR_INVALID_PARAMETER
.the_end:
    ret
ENDPROC VMXR0InvEPT


;/**
; * Invalidate a page using invvpid
; @param   enmFlush     msc:ecx  gcc:edi  x86:[esp+04]  Type of flush
; @param   pDescriptor  msc:edx  gcc:esi  x86:[esp+08]  Descriptor pointer
; */
;DECLASM(int) VMXR0InvVPID(VMX_FLUSH enmFlush, uint64_t *pDescriptor);
BEGINPROC VMXR0InvVPID
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    and         edi, 0ffffffffh
    xor         rax, rax
;    invvpid     rdi, qword [rsi]
    DB          0x66, 0x0F, 0x38, 0x81, 0x3E
 %else
    and         ecx, 0ffffffffh
    xor         rax, rax
;    invvpid     rcx, qword [rdx]
    DB          0x66, 0x0F, 0x38, 0x81, 0xA
 %endif
%else
    mov         ecx, [esp + 4]
    mov         edx, [esp + 8]
    xor         eax, eax
;    invvpid     ecx, qword [edx]
    DB          0x66, 0x0F, 0x38, 0x81, 0xA
%endif
    jnc         .valid_vmcs
    mov         eax, VERR_VMX_INVALID_VMCS_PTR
    ret
.valid_vmcs:
    jnz         .the_end
    mov         eax, VERR_INVALID_PARAMETER
.the_end:
    ret
ENDPROC VMXR0InvVPID


%if GC_ARCH_BITS == 64
;;
; Executes INVLPGA
;
; @param   pPageGC  msc:rcx  gcc:rdi  x86:[esp+04]  Virtual page to invalidate
; @param   uASID    msc:rdx  gcc:rsi  x86:[esp+0C]  Tagged TLB id
;
;DECLASM(void) SVMR0InvlpgA(RTGCPTR pPageGC, uint32_t uASID);
BEGINPROC SVMR0InvlpgA
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    mov     rax, rdi
    mov     rcx, rsi
 %else
    mov     rax, rcx
    mov     rcx, rdx
 %endif
%else
    mov     eax, [esp + 4]
    mov     ecx, [esp + 0Ch]
%endif
    invlpga [xAX], ecx
    ret
ENDPROC SVMR0InvlpgA

%else ; GC_ARCH_BITS != 64
;;
; Executes INVLPGA
;
; @param   pPageGC  msc:ecx  gcc:edi  x86:[esp+04]  Virtual page to invalidate
; @param   uASID    msc:edx  gcc:esi  x86:[esp+08]  Tagged TLB id
;
;DECLASM(void) SVMR0InvlpgA(RTGCPTR pPageGC, uint32_t uASID);
BEGINPROC SVMR0InvlpgA
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    movzx   rax, edi
    mov     ecx, esi
 %else
    ; from http://www.cs.cmu.edu/~fp/courses/15213-s06/misc/asm64-handout.pdf:
    ; ``Perhaps unexpectedly, instructions that move or generate 32-bit register
    ;   values also set the upper 32 bits of the register to zero. Consequently
    ;   there is no need for an instruction movzlq.''
    mov     eax, ecx
    mov     ecx, edx
 %endif
%else
    mov     eax, [esp + 4]
    mov     ecx, [esp + 8]
%endif
    invlpga [xAX], ecx
    ret
ENDPROC SVMR0InvlpgA

%endif ; GC_ARCH_BITS != 64


%ifdef VBOX_WITH_KERNEL_USING_XMM

;;
; Wrapper around vmx.pfnStartVM that preserves host XMM registers and
; load the guest ones when necessary.
;
; @cproto       DECLASM(int) HMR0VMXStartVMhmR0DumpDescriptorM(RTHCUINT fResume, PCPUMCTX pCtx, PVMCSCACHE pCache, PVM pVM,
;                                                              PVMCPU pVCpu, PFNHMVMXSTARTVM pfnStartVM);
;
; @returns      eax
;
; @param        fResumeVM       msc:rcx
; @param        pCtx            msc:rdx
; @param        pVMCSCache      msc:r8
; @param        pVM             msc:r9
; @param        pVCpu           msc:[rbp+30h]   The cross context virtual CPU structure of the calling EMT.
; @param        pfnStartVM      msc:[rbp+38h]
;
; @remarks      This is essentially the same code as hmR0SVMRunWrapXMM, only the parameters differ a little bit.
;
; @remarks      Drivers shouldn't use AVX registers without saving+loading:
;                   https://msdn.microsoft.com/en-us/library/windows/hardware/ff545910%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
;               However the compiler docs have different idea:
;                   https://msdn.microsoft.com/en-us/library/9z1stfyw.aspx
;               We'll go with the former for now.
;
; ASSUMING 64-bit and windows for now.
;
ALIGNCODE(16)
BEGINPROC hmR0VMXStartVMWrapXMM
        push    xBP
        mov     xBP, xSP
        sub     xSP, 0b0h + 040h ; Don't bother optimizing the frame size.

        ; spill input parameters.
        mov     [xBP + 010h], rcx       ; fResumeVM
        mov     [xBP + 018h], rdx       ; pCtx
        mov     [xBP + 020h], r8        ; pVMCSCache
        mov     [xBP + 028h], r9        ; pVM

        ; Ask CPUM whether we've started using the FPU yet.
        mov     rcx, [xBP + 30h]        ; pVCpu
        call    NAME(CPUMIsGuestFPUStateActive)
        test    al, al
        jnz     .guest_fpu_state_active

        ; No need to mess with XMM registers just call the start routine and return.
        mov     r11, [xBP + 38h]        ; pfnStartVM
        mov     r10, [xBP + 30h]        ; pVCpu
        mov     [xSP + 020h], r10
        mov     rcx, [xBP + 010h]       ; fResumeVM
        mov     rdx, [xBP + 018h]       ; pCtx
        mov     r8,  [xBP + 020h]       ; pVMCSCache
        mov     r9,  [xBP + 028h]       ; pVM
        call    r11

        leave
        ret

ALIGNCODE(8)
.guest_fpu_state_active:
        ; Save the non-volatile host XMM registers.
        movdqa  [rsp + 040h + 000h], xmm6
        movdqa  [rsp + 040h + 010h], xmm7
        movdqa  [rsp + 040h + 020h], xmm8
        movdqa  [rsp + 040h + 030h], xmm9
        movdqa  [rsp + 040h + 040h], xmm10
        movdqa  [rsp + 040h + 050h], xmm11
        movdqa  [rsp + 040h + 060h], xmm12
        movdqa  [rsp + 040h + 070h], xmm13
        movdqa  [rsp + 040h + 080h], xmm14
        movdqa  [rsp + 040h + 090h], xmm15
        stmxcsr [rsp + 040h + 0a0h]

        mov     r10, [xBP + 018h]       ; pCtx
        mov     eax, [r10 + CPUMCTX.fXStateMask]
        test    eax, eax
        jz      .guest_fpu_state_manually

        ;
        ; Using XSAVE to load the guest XMM, YMM and ZMM registers.
        ;
        and     eax, CPUM_VOLATILE_XSAVE_GUEST_COMPONENTS
        xor     edx, edx
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        xrstor  [r10]

        ; Make the call (same as in the other case ).
        mov     r11, [xBP + 38h]        ; pfnStartVM
        mov     r10, [xBP + 30h]        ; pVCpu
        mov     [xSP + 020h], r10
        mov     rcx, [xBP + 010h]       ; fResumeVM
        mov     rdx, [xBP + 018h]       ; pCtx
        mov     r8,  [xBP + 020h]       ; pVMCSCache
        mov     r9,  [xBP + 028h]       ; pVM
        call    r11

        mov     r11d, eax               ; save return value (xsave below uses eax)

        ; Save the guest XMM registers.
        mov     r10, [xBP + 018h]       ; pCtx
        mov     eax, [r10 + CPUMCTX.fXStateMask]
        and     eax, CPUM_VOLATILE_XSAVE_GUEST_COMPONENTS
        xor     edx, edx
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        xsave  [r10]

        mov     eax, r11d               ; restore return value.

.restore_non_volatile_host_xmm_regs:
        ; Load the non-volatile host XMM registers.
        movdqa  xmm6,  [rsp + 040h + 000h]
        movdqa  xmm7,  [rsp + 040h + 010h]
        movdqa  xmm8,  [rsp + 040h + 020h]
        movdqa  xmm9,  [rsp + 040h + 030h]
        movdqa  xmm10, [rsp + 040h + 040h]
        movdqa  xmm11, [rsp + 040h + 050h]
        movdqa  xmm12, [rsp + 040h + 060h]
        movdqa  xmm13, [rsp + 040h + 070h]
        movdqa  xmm14, [rsp + 040h + 080h]
        movdqa  xmm15, [rsp + 040h + 090h]
        ldmxcsr        [rsp + 040h + 0a0h]
        leave
        ret

        ;
        ; No XSAVE, load and save the guest XMM registers manually.
        ;
.guest_fpu_state_manually:
        ; Load the full guest XMM register state.
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        movdqa  xmm0,  [r10 + XMM_OFF_IN_X86FXSTATE + 000h]
        movdqa  xmm1,  [r10 + XMM_OFF_IN_X86FXSTATE + 010h]
        movdqa  xmm2,  [r10 + XMM_OFF_IN_X86FXSTATE + 020h]
        movdqa  xmm3,  [r10 + XMM_OFF_IN_X86FXSTATE + 030h]
        movdqa  xmm4,  [r10 + XMM_OFF_IN_X86FXSTATE + 040h]
        movdqa  xmm5,  [r10 + XMM_OFF_IN_X86FXSTATE + 050h]
        movdqa  xmm6,  [r10 + XMM_OFF_IN_X86FXSTATE + 060h]
        movdqa  xmm7,  [r10 + XMM_OFF_IN_X86FXSTATE + 070h]
        movdqa  xmm8,  [r10 + XMM_OFF_IN_X86FXSTATE + 080h]
        movdqa  xmm9,  [r10 + XMM_OFF_IN_X86FXSTATE + 090h]
        movdqa  xmm10, [r10 + XMM_OFF_IN_X86FXSTATE + 0a0h]
        movdqa  xmm11, [r10 + XMM_OFF_IN_X86FXSTATE + 0b0h]
        movdqa  xmm12, [r10 + XMM_OFF_IN_X86FXSTATE + 0c0h]
        movdqa  xmm13, [r10 + XMM_OFF_IN_X86FXSTATE + 0d0h]
        movdqa  xmm14, [r10 + XMM_OFF_IN_X86FXSTATE + 0e0h]
        movdqa  xmm15, [r10 + XMM_OFF_IN_X86FXSTATE + 0f0h]
        ldmxcsr        [r10 + X86FXSTATE.MXCSR]

        ; Make the call (same as in the other case ).
        mov     r11, [xBP + 38h]        ; pfnStartVM
        mov     r10, [xBP + 30h]        ; pVCpu
        mov     [xSP + 020h], r10
        mov     rcx, [xBP + 010h]       ; fResumeVM
        mov     rdx, [xBP + 018h]       ; pCtx
        mov     r8,  [xBP + 020h]       ; pVMCSCache
        mov     r9,  [xBP + 028h]       ; pVM
        call    r11

        ; Save the guest XMM registers.
        mov     r10, [xBP + 018h]       ; pCtx
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        stmxcsr [r10 + X86FXSTATE.MXCSR]
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 000h], xmm0
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 010h], xmm1
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 020h], xmm2
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 030h], xmm3
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 040h], xmm4
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 050h], xmm5
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 060h], xmm6
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 070h], xmm7
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 080h], xmm8
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 090h], xmm9
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0a0h], xmm10
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0b0h], xmm11
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0c0h], xmm12
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0d0h], xmm13
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0e0h], xmm14
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0f0h], xmm15
        jmp     .restore_non_volatile_host_xmm_regs
ENDPROC   hmR0VMXStartVMWrapXMM

;;
; Wrapper around svm.pfnVMRun that preserves host XMM registers and
; load the guest ones when necessary.
;
; @cproto       DECLASM(int) hmR0SVMRunWrapXMM(RTHCPHYS HCPhysVmcbHost, RTHCPHYS HCPhysVmcb, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu,
;                                              PFNHMSVMVMRUN pfnVMRun);
;
; @returns      eax
;
; @param        HCPhysVmcbHost  msc:rcx
; @param        HCPhysVmcb      msc:rdx
; @param        pCtx            msc:r8
; @param        pVM             msc:r9
; @param        pVCpu           msc:[rbp+30h]   The cross context virtual CPU structure of the calling EMT.
; @param        pfnVMRun        msc:[rbp+38h]
;
; @remarks      This is essentially the same code as hmR0VMXStartVMWrapXMM, only the parameters differ a little bit.
;
; @remarks      Drivers shouldn't use AVX registers without saving+loading:
;                   https://msdn.microsoft.com/en-us/library/windows/hardware/ff545910%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
;               However the compiler docs have different idea:
;                   https://msdn.microsoft.com/en-us/library/9z1stfyw.aspx
;               We'll go with the former for now.
;
; ASSUMING 64-bit and windows for now.
ALIGNCODE(16)
BEGINPROC hmR0SVMRunWrapXMM
        push    xBP
        mov     xBP, xSP
        sub     xSP, 0b0h + 040h        ; Don't bother optimizing the frame size.

        ; spill input parameters.
        mov     [xBP + 010h], rcx       ; HCPhysVmcbHost
        mov     [xBP + 018h], rdx       ; HCPhysVmcb
        mov     [xBP + 020h], r8        ; pCtx
        mov     [xBP + 028h], r9        ; pVM

        ; Ask CPUM whether we've started using the FPU yet.
        mov     rcx, [xBP + 30h]        ; pVCpu
        call    NAME(CPUMIsGuestFPUStateActive)
        test    al, al
        jnz     .guest_fpu_state_active

        ; No need to mess with XMM registers just call the start routine and return.
        mov     r11, [xBP + 38h]        ; pfnVMRun
        mov     r10, [xBP + 30h]        ; pVCpu
        mov     [xSP + 020h], r10
        mov     rcx, [xBP + 010h]       ; HCPhysVmcbHost
        mov     rdx, [xBP + 018h]       ; HCPhysVmcb
        mov     r8,  [xBP + 020h]       ; pCtx
        mov     r9,  [xBP + 028h]       ; pVM
        call    r11

        leave
        ret

ALIGNCODE(8)
.guest_fpu_state_active:
        ; Save the non-volatile host XMM registers.
        movdqa  [rsp + 040h + 000h], xmm6
        movdqa  [rsp + 040h + 010h], xmm7
        movdqa  [rsp + 040h + 020h], xmm8
        movdqa  [rsp + 040h + 030h], xmm9
        movdqa  [rsp + 040h + 040h], xmm10
        movdqa  [rsp + 040h + 050h], xmm11
        movdqa  [rsp + 040h + 060h], xmm12
        movdqa  [rsp + 040h + 070h], xmm13
        movdqa  [rsp + 040h + 080h], xmm14
        movdqa  [rsp + 040h + 090h], xmm15
        stmxcsr [rsp + 040h + 0a0h]

        mov     r10, [xBP + 020h]       ; pCtx
        mov     eax, [r10 + CPUMCTX.fXStateMask]
        test    eax, eax
        jz      .guest_fpu_state_manually

        ;
        ; Using XSAVE.
        ;
        and     eax, CPUM_VOLATILE_XSAVE_GUEST_COMPONENTS
        xor     edx, edx
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        xrstor  [r10]

        ; Make the call (same as in the other case ).
        mov     r11, [xBP + 38h]        ; pfnVMRun
        mov     r10, [xBP + 30h]        ; pVCpu
        mov     [xSP + 020h], r10
        mov     rcx, [xBP + 010h]       ; HCPhysVmcbHost
        mov     rdx, [xBP + 018h]       ; HCPhysVmcb
        mov     r8,  [xBP + 020h]       ; pCtx
        mov     r9,  [xBP + 028h]       ; pVM
        call    r11

        mov     r11d, eax               ; save return value (xsave below uses eax)

        ; Save the guest XMM registers.
        mov     r10, [xBP + 020h]       ; pCtx
        mov     eax, [r10 + CPUMCTX.fXStateMask]
        and     eax, CPUM_VOLATILE_XSAVE_GUEST_COMPONENTS
        xor     edx, edx
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        xsave  [r10]

        mov     eax, r11d               ; restore return value.

.restore_non_volatile_host_xmm_regs:
        ; Load the non-volatile host XMM registers.
        movdqa  xmm6,  [rsp + 040h + 000h]
        movdqa  xmm7,  [rsp + 040h + 010h]
        movdqa  xmm8,  [rsp + 040h + 020h]
        movdqa  xmm9,  [rsp + 040h + 030h]
        movdqa  xmm10, [rsp + 040h + 040h]
        movdqa  xmm11, [rsp + 040h + 050h]
        movdqa  xmm12, [rsp + 040h + 060h]
        movdqa  xmm13, [rsp + 040h + 070h]
        movdqa  xmm14, [rsp + 040h + 080h]
        movdqa  xmm15, [rsp + 040h + 090h]
        ldmxcsr [rsp + 040h + 0a0h]
        leave
        ret

        ;
        ; No XSAVE, load and save the guest XMM registers manually.
        ;
.guest_fpu_state_manually:
        ; Load the full guest XMM register state.
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        movdqa  xmm0,  [r10 + XMM_OFF_IN_X86FXSTATE + 000h]
        movdqa  xmm1,  [r10 + XMM_OFF_IN_X86FXSTATE + 010h]
        movdqa  xmm2,  [r10 + XMM_OFF_IN_X86FXSTATE + 020h]
        movdqa  xmm3,  [r10 + XMM_OFF_IN_X86FXSTATE + 030h]
        movdqa  xmm4,  [r10 + XMM_OFF_IN_X86FXSTATE + 040h]
        movdqa  xmm5,  [r10 + XMM_OFF_IN_X86FXSTATE + 050h]
        movdqa  xmm6,  [r10 + XMM_OFF_IN_X86FXSTATE + 060h]
        movdqa  xmm7,  [r10 + XMM_OFF_IN_X86FXSTATE + 070h]
        movdqa  xmm8,  [r10 + XMM_OFF_IN_X86FXSTATE + 080h]
        movdqa  xmm9,  [r10 + XMM_OFF_IN_X86FXSTATE + 090h]
        movdqa  xmm10, [r10 + XMM_OFF_IN_X86FXSTATE + 0a0h]
        movdqa  xmm11, [r10 + XMM_OFF_IN_X86FXSTATE + 0b0h]
        movdqa  xmm12, [r10 + XMM_OFF_IN_X86FXSTATE + 0c0h]
        movdqa  xmm13, [r10 + XMM_OFF_IN_X86FXSTATE + 0d0h]
        movdqa  xmm14, [r10 + XMM_OFF_IN_X86FXSTATE + 0e0h]
        movdqa  xmm15, [r10 + XMM_OFF_IN_X86FXSTATE + 0f0h]
        ldmxcsr        [r10 + X86FXSTATE.MXCSR]

        ; Make the call (same as in the other case ).
        mov     r11, [xBP + 38h]        ; pfnVMRun
        mov     r10, [xBP + 30h]        ; pVCpu
        mov     [xSP + 020h], r10
        mov     rcx, [xBP + 010h]       ; HCPhysVmcbHost
        mov     rdx, [xBP + 018h]       ; HCPhysVmcb
        mov     r8,  [xBP + 020h]       ; pCtx
        mov     r9,  [xBP + 028h]       ; pVM
        call    r11

        ; Save the guest XMM registers.
        mov     r10, [xBP + 020h]       ; pCtx
        mov     r10, [r10 + CPUMCTX.pXStateR0]
        stmxcsr [r10 + X86FXSTATE.MXCSR]
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 000h], xmm0
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 010h], xmm1
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 020h], xmm2
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 030h], xmm3
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 040h], xmm4
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 050h], xmm5
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 060h], xmm6
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 070h], xmm7
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 080h], xmm8
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 090h], xmm9
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0a0h], xmm10
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0b0h], xmm11
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0c0h], xmm12
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0d0h], xmm13
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0e0h], xmm14
        movdqa  [r10 + XMM_OFF_IN_X86FXSTATE + 0f0h], xmm15
        jmp     .restore_non_volatile_host_xmm_regs
ENDPROC   hmR0SVMRunWrapXMM

%endif ; VBOX_WITH_KERNEL_USING_XMM


;; @def RESTORE_STATE_VM32
; Macro restoring essential host state and updating guest state
; for common host, 32-bit guest for VT-x.
%macro RESTORE_STATE_VM32 0
    ; Restore base and limit of the IDTR & GDTR.
 %ifndef VMX_SKIP_IDTR
    lidt    [xSP]
    add     xSP, xCB * 2
 %endif
 %ifndef VMX_SKIP_GDTR
    lgdt    [xSP]
    add     xSP, xCB * 2
 %endif

    push    xDI
 %ifndef VMX_SKIP_TR
    mov     xDI, [xSP + xCB * 3]         ; pCtx (*3 to skip the saved xDI, TR, LDTR).
 %else
    mov     xDI, [xSP + xCB * 2]         ; pCtx (*2 to skip the saved xDI, LDTR).
 %endif

    mov     [ss:xDI + CPUMCTX.eax], eax
    mov     xAX, SPECTRE_FILLER
    mov     [ss:xDI + CPUMCTX.ebx], ebx
    mov     xBX, xAX
    mov     [ss:xDI + CPUMCTX.ecx], ecx
    mov     xCX, xAX
    mov     [ss:xDI + CPUMCTX.edx], edx
    mov     xDX, xAX
    mov     [ss:xDI + CPUMCTX.esi], esi
    mov     xSI, xAX
    mov     [ss:xDI + CPUMCTX.ebp], ebp
    mov     xBP, xAX
    mov     xAX, cr2
    mov     [ss:xDI + CPUMCTX.cr2], xAX

 %ifdef RT_ARCH_AMD64
    pop     xAX                                 ; The guest edi we pushed above.
    mov     dword [ss:xDI + CPUMCTX.edi], eax
 %else
    pop     dword [ss:xDI + CPUMCTX.edi]        ; The guest edi we pushed above.
 %endif

    ; Fight spectre.
    INDIRECT_BRANCH_PREDICTION_BARRIER ss:xDI, CPUMCTX_WSF_IBPB_EXIT

 %ifndef VMX_SKIP_TR
    ; Restore TSS selector; must mark it as not busy before using ltr (!)
    ; ASSUME that this is supposed to be 'BUSY'. (saves 20-30 ticks on the T42p)
    ; @todo get rid of sgdt
    pop     xBX         ; Saved TR
    sub     xSP, xCB * 2
    sgdt    [xSP]
    mov     xAX, xBX
    and     eax, X86_SEL_MASK_OFF_RPL               ; Mask away TI and RPL bits leaving only the descriptor offset.
    add     xAX, [xSP + 2]                          ; eax <- GDTR.address + descriptor offset.
    and     dword [ss:xAX + 4], ~RT_BIT(9)          ; Clear the busy flag in TSS desc (bits 0-7=base, bit 9=busy bit).
    ltr     bx
    add     xSP, xCB * 2
 %endif

    pop     xAX         ; Saved LDTR
 %ifdef RT_ARCH_AMD64
    cmp     eax, 0
    je      %%skip_ldt_write32
 %endif
    lldt    ax

%%skip_ldt_write32:
    add     xSP, xCB     ; pCtx

 %ifdef VMX_USE_CACHED_VMCS_ACCESSES
    pop     xDX         ; Saved pCache

    ; Note! If we get here as a result of invalid VMCS pointer, all the following
    ; vmread's will fail (only eflags.cf=1 will be set) but that shouldn't cause any
    ; trouble only just less efficient.
    mov     ecx, [ss:xDX + VMCSCACHE.Read.cValidEntries]
    cmp     ecx, 0      ; Can't happen
    je      %%no_cached_read32
    jmp     %%cached_read32

ALIGN(16)
%%cached_read32:
    dec     xCX
    mov     eax, [ss:xDX + VMCSCACHE.Read.aField + xCX * 4]
    ; Note! This leaves the high 32 bits of the cache entry unmodified!!
    vmread  [ss:xDX + VMCSCACHE.Read.aFieldVal + xCX * 8], xAX
    cmp     xCX, 0
    jnz     %%cached_read32
%%no_cached_read32:
 %endif

    ; Restore segment registers.
    MYPOPSEGS xAX, ax

    ; Restore the host XCR0 if necessary.
    pop     xCX
    test    ecx, ecx
    jnz     %%xcr0_after_skip
    pop     xAX
    pop     xDX
    xsetbv                              ; ecx is already zero.
%%xcr0_after_skip:

    ; Restore general purpose registers.
    MYPOPAD
%endmacro


;;
; Prepares for and executes VMLAUNCH/VMRESUME (32 bits guest mode)
;
; @returns VBox status code
; @param    fResume    x86:[ebp+8], msc:rcx,gcc:rdi     Whether to use vmlauch/vmresume.
; @param    pCtx       x86:[ebp+c], msc:rdx,gcc:rsi     Pointer to the guest-CPU context.
; @param    pCache     x86:[ebp+10],msc:r8, gcc:rdx     Pointer to the VMCS cache.
; @param    pVM        x86:[ebp+14],msc:r9, gcc:rcx     The cross context VM structure.
; @param    pVCpu      x86:[ebp+18],msc:[ebp+30],gcc:r8 The cross context virtual CPU structure of the calling EMT.
;
ALIGNCODE(16)
BEGINPROC VMXR0StartVM32
    push    xBP
    mov     xBP, xSP

    pushf
    cli

    ;
    ; Save all general purpose host registers.
    ;
    MYPUSHAD

    ;
    ; First we have to write some final guest CPU context registers.
    ;
    mov     eax, VMX_VMCS_HOST_RIP
%ifdef RT_ARCH_AMD64
    lea     r10, [.vmlaunch_done wrt rip]
    vmwrite rax, r10
%else
    mov     ecx, .vmlaunch_done
    vmwrite eax, ecx
%endif
    ; Note: assumes success!

    ;
    ; Unify input parameter registers.
    ;
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_GCC
    ; fResume already in rdi
    ; pCtx    already in rsi
    mov     rbx, rdx        ; pCache
 %else
    mov     rdi, rcx        ; fResume
    mov     rsi, rdx        ; pCtx
    mov     rbx, r8         ; pCache
 %endif
%else
    mov     edi, [ebp + 8]  ; fResume
    mov     esi, [ebp + 12] ; pCtx
    mov     ebx, [ebp + 16] ; pCache
%endif

    ;
    ; Save the host XCR0 and load the guest one if necessary.
    ; Note! Trashes rdx and rcx.
    ;
%ifdef ASM_CALL64_MSC
    mov     rax, [xBP + 30h]            ; pVCpu
%elifdef ASM_CALL64_GCC
    mov     rax, r8                     ; pVCpu
%else
    mov     eax, [xBP + 18h]            ; pVCpu
%endif
    test    byte [xAX + VMCPU.hm + HMCPU.fLoadSaveGuestXcr0], 1
    jz      .xcr0_before_skip

    xor     ecx, ecx
    xgetbv                              ; Save the host one on the stack.
    push    xDX
    push    xAX

    mov     eax, [xSI + CPUMCTX.aXcr]   ; Load the guest one.
    mov     edx, [xSI + CPUMCTX.aXcr + 4]
    xor     ecx, ecx                    ; paranoia
    xsetbv

    push    0                           ; Indicate that we must restore XCR0 (popped into ecx, thus 0).
    jmp     .xcr0_before_done

.xcr0_before_skip:
    push    3fh                         ; indicate that we need not.
.xcr0_before_done:

    ;
    ; Save segment registers.
    ; Note! Trashes rdx & rcx, so we moved it here (amd64 case).
    ;
    MYPUSHSEGS xAX, ax

%ifdef VMX_USE_CACHED_VMCS_ACCESSES
    mov     ecx, [xBX + VMCSCACHE.Write.cValidEntries]
    cmp     ecx, 0
    je      .no_cached_writes
    mov     edx, ecx
    mov     ecx, 0
    jmp     .cached_write

ALIGN(16)
.cached_write:
    mov     eax, [xBX + VMCSCACHE.Write.aField + xCX * 4]
    vmwrite xAX, [xBX + VMCSCACHE.Write.aFieldVal + xCX * 8]
    inc     xCX
    cmp     xCX, xDX
    jl     .cached_write

    mov     dword [xBX + VMCSCACHE.Write.cValidEntries], 0
.no_cached_writes:

    ; Save the pCache pointer.
    push    xBX
%endif

    ; Save the pCtx pointer.
    push    xSI

    ; Save host LDTR.
    xor     eax, eax
    sldt    ax
    push    xAX

%ifndef VMX_SKIP_TR
    ; The host TR limit is reset to 0x67; save & restore it manually.
    str     eax
    push    xAX
%endif

%ifndef VMX_SKIP_GDTR
    ; VT-x only saves the base of the GDTR & IDTR and resets the limit to 0xffff; we must restore the limit correctly!
    sub     xSP, xCB * 2
    sgdt    [xSP]
%endif
%ifndef VMX_SKIP_IDTR
    sub     xSP, xCB * 2
    sidt    [xSP]
%endif

    ; Load CR2 if necessary (may be expensive as writing CR2 is a synchronizing instruction).
    mov     xBX, [xSI + CPUMCTX.cr2]
    mov     xDX, cr2
    cmp     xBX, xDX
    je      .skip_cr2_write32
    mov     cr2, xBX

.skip_cr2_write32:
    mov     eax, VMX_VMCS_HOST_RSP
    vmwrite xAX, xSP
    ; Note: assumes success!
    ; Don't mess with ESP anymore!!!

    ; Fight spectre.
    INDIRECT_BRANCH_PREDICTION_BARRIER xSI, CPUMCTX_WSF_IBPB_ENTRY

    ; Load guest general purpose registers.
    mov     eax, [xSI + CPUMCTX.eax]
    mov     ebx, [xSI + CPUMCTX.ebx]
    mov     ecx, [xSI + CPUMCTX.ecx]
    mov     edx, [xSI + CPUMCTX.edx]
    mov     ebp, [xSI + CPUMCTX.ebp]

    ; Resume or start VM?
    cmp     xDI, 0                  ; fResume

    ; Load guest edi & esi.
    mov     edi, [xSI + CPUMCTX.edi]
    mov     esi, [xSI + CPUMCTX.esi]

    je      .vmlaunch_launch

    vmresume
    jc      near .vmxstart_invalid_vmcs_ptr
    jz      near .vmxstart_start_failed
    jmp     .vmlaunch_done;      ; Here if vmresume detected a failure.

.vmlaunch_launch:
    vmlaunch
    jc      near .vmxstart_invalid_vmcs_ptr
    jz      near .vmxstart_start_failed
    jmp     .vmlaunch_done;      ; Here if vmlaunch detected a failure.

ALIGNCODE(16) ;; @todo YASM BUG - this alignment is wrong on darwin, it's 1 byte off.
.vmlaunch_done:
    RESTORE_STATE_VM32
    mov     eax, VINF_SUCCESS

.vmstart_end:
    popf
    pop     xBP
    ret

.vmxstart_invalid_vmcs_ptr:
    RESTORE_STATE_VM32
    mov     eax, VERR_VMX_INVALID_VMCS_PTR_TO_START_VM
    jmp     .vmstart_end

.vmxstart_start_failed:
    RESTORE_STATE_VM32
    mov     eax, VERR_VMX_UNABLE_TO_START_VM
    jmp     .vmstart_end

ENDPROC VMXR0StartVM32


%ifdef RT_ARCH_AMD64
;; @def RESTORE_STATE_VM64
; Macro restoring essential host state and updating guest state
; for 64-bit host, 64-bit guest for VT-x.
;
%macro RESTORE_STATE_VM64 0
    ; Restore base and limit of the IDTR & GDTR
 %ifndef VMX_SKIP_IDTR
    lidt    [xSP]
    add     xSP, xCB * 2
 %endif
 %ifndef VMX_SKIP_GDTR
    lgdt    [xSP]
    add     xSP, xCB * 2
 %endif

    push    xDI
 %ifndef VMX_SKIP_TR
    mov     xDI, [xSP + xCB * 3]        ; pCtx (*3 to skip the saved xDI, TR, LDTR)
 %else
    mov     xDI, [xSP + xCB * 2]        ; pCtx (*2 to skip the saved xDI, LDTR)
 %endif

    mov     qword [xDI + CPUMCTX.eax], rax
    mov     rax, SPECTRE_FILLER64
    mov     qword [xDI + CPUMCTX.ebx], rbx
    mov     rbx, rax
    mov     qword [xDI + CPUMCTX.ecx], rcx
    mov     rcx, rax
    mov     qword [xDI + CPUMCTX.edx], rdx
    mov     rdx, rax
    mov     qword [xDI + CPUMCTX.esi], rsi
    mov     rsi, rax
    mov     qword [xDI + CPUMCTX.ebp], rbp
    mov     rbp, rax
    mov     qword [xDI + CPUMCTX.r8],  r8
    mov     r8, rax
    mov     qword [xDI + CPUMCTX.r9],  r9
    mov     r9, rax
    mov     qword [xDI + CPUMCTX.r10], r10
    mov     r10, rax
    mov     qword [xDI + CPUMCTX.r11], r11
    mov     r11, rax
    mov     qword [xDI + CPUMCTX.r12], r12
    mov     r12, rax
    mov     qword [xDI + CPUMCTX.r13], r13
    mov     r13, rax
    mov     qword [xDI + CPUMCTX.r14], r14
    mov     r14, rax
    mov     qword [xDI + CPUMCTX.r15], r15
    mov     r15, rax
    mov     rax, cr2
    mov     qword [xDI + CPUMCTX.cr2], rax

    pop     xAX                                 ; The guest rdi we pushed above
    mov     qword [xDI + CPUMCTX.edi], rax

    ; Fight spectre.
    INDIRECT_BRANCH_PREDICTION_BARRIER xDI, CPUMCTX_WSF_IBPB_EXIT

 %ifndef VMX_SKIP_TR
    ; Restore TSS selector; must mark it as not busy before using ltr (!)
    ; ASSUME that this is supposed to be 'BUSY'. (saves 20-30 ticks on the T42p).
    ; @todo get rid of sgdt
    pop     xBX         ; Saved TR
    sub     xSP, xCB * 2
    sgdt    [xSP]
    mov     xAX, xBX
    and     eax, X86_SEL_MASK_OFF_RPL           ; Mask away TI and RPL bits leaving only the descriptor offset.
    add     xAX, [xSP + 2]                      ; eax <- GDTR.address + descriptor offset.
    and     dword [xAX + 4], ~RT_BIT(9)         ; Clear the busy flag in TSS desc (bits 0-7=base, bit 9=busy bit).
    ltr     bx
    add     xSP, xCB * 2
 %endif

    pop     xAX         ; Saved LDTR
    cmp     eax, 0
    je      %%skip_ldt_write64
    lldt    ax

%%skip_ldt_write64:
    pop     xSI         ; pCtx (needed in rsi by the macros below)

 %ifdef VMX_USE_CACHED_VMCS_ACCESSES
    pop     xDX         ; Saved pCache

    ; Note! If we get here as a result of invalid VMCS pointer, all the following
    ; vmread's will fail (only eflags.cf=1 will be set) but that shouldn't cause any
    ; trouble only just less efficient.
    mov     ecx, [xDX + VMCSCACHE.Read.cValidEntries]
    cmp     ecx, 0      ; Can't happen
    je      %%no_cached_read64
    jmp     %%cached_read64

ALIGN(16)
%%cached_read64:
    dec     xCX
    mov     eax, [xDX + VMCSCACHE.Read.aField + xCX * 4]
    vmread  [xDX + VMCSCACHE.Read.aFieldVal + xCX * 8], xAX
    cmp     xCX, 0
    jnz     %%cached_read64
%%no_cached_read64:
 %endif

    ; Restore segment registers.
    MYPOPSEGS xAX, ax

    ; Restore the host XCR0 if necessary.
    pop     xCX
    test    ecx, ecx
    jnz     %%xcr0_after_skip
    pop     xAX
    pop     xDX
    xsetbv                              ; ecx is already zero.
%%xcr0_after_skip:

    ; Restore general purpose registers.
    MYPOPAD
%endmacro


;;
; Prepares for and executes VMLAUNCH/VMRESUME (64 bits guest mode)
;
; @returns VBox status code
; @param    fResume    msc:rcx, gcc:rdi     Whether to use vmlauch/vmresume.
; @param    pCtx       msc:rdx, gcc:rsi     Pointer to the guest-CPU context.
; @param    pCache     msc:r8,  gcc:rdx     Pointer to the VMCS cache.
; @param    pVM        msc:r9,  gcc:rcx     The cross context VM structure.
; @param    pVCpu      msc:[ebp+30], gcc:r8 The cross context virtual CPU structure of the calling EMT.
;
ALIGNCODE(16)
BEGINPROC VMXR0StartVM64
    push    xBP
    mov     xBP, xSP

    pushf
    cli

    ; Save all general purpose host registers.
    MYPUSHAD

    ; First we have to save some final CPU context registers.
    lea     r10, [.vmlaunch64_done wrt rip]
    mov     rax, VMX_VMCS_HOST_RIP      ; Return address (too difficult to continue after VMLAUNCH?).
    vmwrite rax, r10
    ; Note: assumes success!

    ;
    ; Unify the input parameter registers.
    ;
%ifdef ASM_CALL64_GCC
    ; fResume already in rdi
    ; pCtx    already in rsi
    mov     rbx, rdx        ; pCache
%else
    mov     rdi, rcx        ; fResume
    mov     rsi, rdx        ; pCtx
    mov     rbx, r8         ; pCache
%endif

    ;
    ; Save the host XCR0 and load the guest one if necessary.
    ; Note! Trashes rdx and rcx.
    ;
%ifdef ASM_CALL64_MSC
    mov     rax, [xBP + 30h]            ; pVCpu
%else
    mov     rax, r8                     ; pVCpu
%endif
    test    byte [xAX + VMCPU.hm + HMCPU.fLoadSaveGuestXcr0], 1
    jz      .xcr0_before_skip

    xor     ecx, ecx
    xgetbv                              ; Save the host one on the stack.
    push    xDX
    push    xAX

    mov     eax, [xSI + CPUMCTX.aXcr]   ; Load the guest one.
    mov     edx, [xSI + CPUMCTX.aXcr + 4]
    xor     ecx, ecx                    ; paranoia
    xsetbv

    push    0                           ; Indicate that we must restore XCR0 (popped into ecx, thus 0).
    jmp     .xcr0_before_done

.xcr0_before_skip:
    push    3fh                         ; indicate that we need not.
.xcr0_before_done:

    ;
    ; Save segment registers.
    ; Note! Trashes rdx & rcx, so we moved it here (amd64 case).
    ;
    MYPUSHSEGS xAX, ax

%ifdef VMX_USE_CACHED_VMCS_ACCESSES
    mov     ecx, [xBX + VMCSCACHE.Write.cValidEntries]
    cmp     ecx, 0
    je      .no_cached_writes
    mov     edx, ecx
    mov     ecx, 0
    jmp     .cached_write

ALIGN(16)
.cached_write:
    mov     eax, [xBX + VMCSCACHE.Write.aField + xCX * 4]
    vmwrite xAX, [xBX + VMCSCACHE.Write.aFieldVal + xCX * 8]
    inc     xCX
    cmp     xCX, xDX
    jl     .cached_write

    mov     dword [xBX + VMCSCACHE.Write.cValidEntries], 0
.no_cached_writes:

    ; Save the pCache pointer.
    push    xBX
%endif

    ; Save the pCtx pointer.
    push    xSI

    ; Save host LDTR.
    xor     eax, eax
    sldt    ax
    push    xAX

%ifndef VMX_SKIP_TR
    ; The host TR limit is reset to 0x67; save & restore it manually.
    str     eax
    push    xAX
%endif

%ifndef VMX_SKIP_GDTR
    ; VT-x only saves the base of the GDTR & IDTR and resets the limit to 0xffff; we must restore the limit correctly!
    sub     xSP, xCB * 2
    sgdt    [xSP]
%endif
%ifndef VMX_SKIP_IDTR
    sub     xSP, xCB * 2
    sidt    [xSP]
%endif

    ; Load CR2 if necessary (may be expensive as writing CR2 is a synchronizing instruction).
    mov     rbx, qword [xSI + CPUMCTX.cr2]
    mov     rdx, cr2
    cmp     rbx, rdx
    je      .skip_cr2_write
    mov     cr2, rbx

.skip_cr2_write:
    mov     eax, VMX_VMCS_HOST_RSP
    vmwrite xAX, xSP
    ; Note: assumes success!
    ; Don't mess with ESP anymore!!!

    ; Fight spectre.
    INDIRECT_BRANCH_PREDICTION_BARRIER xSI, CPUMCTX_WSF_IBPB_ENTRY

    ; Load guest general purpose registers.
    mov     rax, qword [xSI + CPUMCTX.eax]
    mov     rbx, qword [xSI + CPUMCTX.ebx]
    mov     rcx, qword [xSI + CPUMCTX.ecx]
    mov     rdx, qword [xSI + CPUMCTX.edx]
    mov     rbp, qword [xSI + CPUMCTX.ebp]
    mov     r8,  qword [xSI + CPUMCTX.r8]
    mov     r9,  qword [xSI + CPUMCTX.r9]
    mov     r10, qword [xSI + CPUMCTX.r10]
    mov     r11, qword [xSI + CPUMCTX.r11]
    mov     r12, qword [xSI + CPUMCTX.r12]
    mov     r13, qword [xSI + CPUMCTX.r13]
    mov     r14, qword [xSI + CPUMCTX.r14]
    mov     r15, qword [xSI + CPUMCTX.r15]

    ; Resume or start VM?
    cmp     xDI, 0                  ; fResume

    ; Load guest rdi & rsi.
    mov     rdi, qword [xSI + CPUMCTX.edi]
    mov     rsi, qword [xSI + CPUMCTX.esi]

    je      .vmlaunch64_launch

    vmresume
    jc      near .vmxstart64_invalid_vmcs_ptr
    jz      near .vmxstart64_start_failed
    jmp     .vmlaunch64_done;      ; Here if vmresume detected a failure.

.vmlaunch64_launch:
    vmlaunch
    jc      near .vmxstart64_invalid_vmcs_ptr
    jz      near .vmxstart64_start_failed
    jmp     .vmlaunch64_done;      ; Here if vmlaunch detected a failure.

ALIGNCODE(16)
.vmlaunch64_done:
    RESTORE_STATE_VM64
    mov     eax, VINF_SUCCESS

.vmstart64_end:
    popf
    pop     xBP
    ret

.vmxstart64_invalid_vmcs_ptr:
    RESTORE_STATE_VM64
    mov     eax, VERR_VMX_INVALID_VMCS_PTR_TO_START_VM
    jmp     .vmstart64_end

.vmxstart64_start_failed:
    RESTORE_STATE_VM64
    mov     eax, VERR_VMX_UNABLE_TO_START_VM
    jmp     .vmstart64_end
ENDPROC VMXR0StartVM64
%endif ; RT_ARCH_AMD64


;;
; Prepares for and executes VMRUN (32 bits guests)
;
; @returns  VBox status code
; @param    HCPhysVmcbHost  msc:rcx,gcc:rdi     Physical address of host VMCB.
; @param    HCPhysVmcb      msc:rdx,gcc:rsi     Physical address of guest VMCB.
; @param    pCtx            msc:r8,gcc:rdx      Pointer to the guest CPU-context.
; @param    pVM             msc:r9,gcc:rcx      The cross context VM structure.
; @param    pVCpu           msc:[rsp+28],gcc:r8 The cross context virtual CPU structure of the calling EMT.
;
ALIGNCODE(16)
BEGINPROC SVMR0VMRun
%ifdef RT_ARCH_AMD64 ; fake a cdecl stack frame
 %ifdef ASM_CALL64_GCC
    push    r8                ; pVCpu
    push    rcx               ; pVM
    push    rdx               ; pCtx
    push    rsi               ; HCPhysVmcb
    push    rdi               ; HCPhysVmcbHost
 %else
    mov     rax, [rsp + 28h]
    push    rax               ; pVCpu
    push    r9                ; pVM
    push    r8                ; pCtx
    push    rdx               ; HCPhysVmcb
    push    rcx               ; HCPhysVmcbHost
 %endif
    push    0
%endif
    push    xBP
    mov     xBP, xSP
    pushf

    ; Save all general purpose host registers.
    MYPUSHAD

    ; Load pCtx into xSI.
    mov     xSI, [xBP + xCB * 2 + RTHCPHYS_CB * 2]  ; pCtx

    ; Save the host XCR0 and load the guest one if necessary.
    mov     xAX, [xBP + xCB * 2 + RTHCPHYS_CB * 2 + xCB * 2] ; pVCpu
    test    byte [xAX + VMCPU.hm + HMCPU.fLoadSaveGuestXcr0], 1
    jz      .xcr0_before_skip

    xor     ecx, ecx
    xgetbv                                  ; Save the host XCR0 on the stack
    push    xDX
    push    xAX

    mov     xSI, [xBP + xCB * 2 + RTHCPHYS_CB * 2]  ; pCtx
    mov     eax, [xSI + CPUMCTX.aXcr]       ; load the guest XCR0
    mov     edx, [xSI + CPUMCTX.aXcr + 4]
    xor     ecx, ecx                        ; paranoia
    xsetbv

    push    0                               ; indicate that we must restore XCR0 (popped into ecx, thus 0)
    jmp     .xcr0_before_done

.xcr0_before_skip:
    push    3fh                             ; indicate that we need not restore XCR0
.xcr0_before_done:

    ; Save guest CPU-context pointer for simplifying saving of the GPRs afterwards.
    push    xSI

    ; Save host fs, gs, sysenter msr etc.
    mov     xAX, [xBP + xCB * 2]                    ; HCPhysVmcbHost (64 bits physical address; x86: take low dword only)
    push    xAX                                     ; save for the vmload after vmrun
    vmsave

    ; Fight spectre.
    INDIRECT_BRANCH_PREDICTION_BARRIER xSI, CPUMCTX_WSF_IBPB_ENTRY

    ; Setup xAX for VMLOAD.
    mov     xAX, [xBP + xCB * 2 + RTHCPHYS_CB]      ; HCPhysVmcb (64 bits physical address; x86: take low dword only)

    ; Load guest general purpose registers.
    ; eax is loaded from the VMCB by VMRUN.
    mov     ebx, [xSI + CPUMCTX.ebx]
    mov     ecx, [xSI + CPUMCTX.ecx]
    mov     edx, [xSI + CPUMCTX.edx]
    mov     edi, [xSI + CPUMCTX.edi]
    mov     ebp, [xSI + CPUMCTX.ebp]
    mov     esi, [xSI + CPUMCTX.esi]

    ; Clear the global interrupt flag & execute sti to make sure external interrupts cause a world switch.
    clgi
    sti

    ; Load guest fs, gs, sysenter msr etc.
    vmload

    ; Run the VM.
    vmrun

    ; Save guest fs, gs, sysenter msr etc.
    vmsave

    ; Load host fs, gs, sysenter msr etc.
    pop     xAX                             ; load HCPhysVmcbHost (pushed above)
    vmload

    ; Set the global interrupt flag again, but execute cli to make sure IF=0.
    cli
    stgi

    ; Pop the context pointer (pushed above) and save the guest GPRs (sans RSP and RAX).
    pop     xAX

    mov     [ss:xAX + CPUMCTX.ebx], ebx
    mov     xBX, SPECTRE_FILLER
    mov     [ss:xAX + CPUMCTX.ecx], ecx
    mov     xCX, xBX
    mov     [ss:xAX + CPUMCTX.edx], edx
    mov     xDX, xBX
    mov     [ss:xAX + CPUMCTX.esi], esi
    mov     xSI, xBX
    mov     [ss:xAX + CPUMCTX.edi], edi
    mov     xDI, xBX
    mov     [ss:xAX + CPUMCTX.ebp], ebp
    mov     xBP, xBX

    ; Fight spectre.  Note! Trashes xAX!
    INDIRECT_BRANCH_PREDICTION_BARRIER ss:xAX, CPUMCTX_WSF_IBPB_EXIT

    ; Restore the host xcr0 if necessary.
    pop     xCX
    test    ecx, ecx
    jnz     .xcr0_after_skip
    pop     xAX
    pop     xDX
    xsetbv                              ; ecx is already zero
.xcr0_after_skip:

    ; Restore host general purpose registers.
    MYPOPAD

    mov     eax, VINF_SUCCESS

    popf
    pop     xBP
%ifdef RT_ARCH_AMD64
    add     xSP, 6*xCB
%endif
    ret
ENDPROC SVMR0VMRun


%ifdef RT_ARCH_AMD64
;;
; Prepares for and executes VMRUN (64 bits guests)
;
; @returns  VBox status code
; @param    HCPhysVmcbHost  msc:rcx,gcc:rdi     Physical address of host VMCB.
; @param    HCPhysVmcb      msc:rdx,gcc:rsi     Physical address of guest VMCB.
; @param    pCtx            msc:r8,gcc:rdx      Pointer to the guest-CPU context.
; @param    pVM             msc:r9,gcc:rcx      The cross context VM structure.
; @param    pVCpu           msc:[rsp+28],gcc:r8 The cross context virtual CPU structure of the calling EMT.
;
ALIGNCODE(16)
BEGINPROC SVMR0VMRun64
    ; Fake a cdecl stack frame
 %ifdef ASM_CALL64_GCC
    push    r8                ;pVCpu
    push    rcx               ;pVM
    push    rdx               ;pCtx
    push    rsi               ;HCPhysVmcb
    push    rdi               ;HCPhysVmcbHost
 %else
    mov     rax, [rsp + 28h]
    push    rax               ; rbp + 30h pVCpu
    push    r9                ; rbp + 28h pVM
    push    r8                ; rbp + 20h pCtx
    push    rdx               ; rbp + 18h HCPhysVmcb
    push    rcx               ; rbp + 10h HCPhysVmcbHost
 %endif
    push    0                 ; rbp + 08h "fake ret addr"
    push    rbp               ; rbp + 00h
    mov     rbp, rsp
    pushf

    ; Manual save and restore:
    ;  - General purpose registers except RIP, RSP, RAX
    ;
    ; Trashed:
    ;  - CR2 (we don't care)
    ;  - LDTR (reset to 0)
    ;  - DRx (presumably not changed at all)
    ;  - DR7 (reset to 0x400)

    ; Save all general purpose host registers.
    MYPUSHAD

    ; Load pCtx into xSI.
    mov     xSI, [rbp + xCB * 2 + RTHCPHYS_CB * 2]

    ; Save the host XCR0 and load the guest one if necessary.
    mov     rax, [xBP + 30h]                ; pVCpu
    test    byte [xAX + VMCPU.hm + HMCPU.fLoadSaveGuestXcr0], 1
    jz      .xcr0_before_skip

    xor     ecx, ecx
    xgetbv                                  ; save the host XCR0 on the stack.
    push    xDX
    push    xAX

    mov     xSI, [xBP + xCB * 2 + RTHCPHYS_CB * 2]  ; pCtx
    mov     eax, [xSI + CPUMCTX.aXcr]       ; load the guest XCR0
    mov     edx, [xSI + CPUMCTX.aXcr + 4]
    xor     ecx, ecx                        ; paranoia
    xsetbv

    push    0                               ; indicate that we must restore XCR0 (popped into ecx, thus 0)
    jmp     .xcr0_before_done

.xcr0_before_skip:
    push    3fh                             ; indicate that we need not restore XCR0
.xcr0_before_done:

    ; Save guest CPU-context pointer for simplifying saving of the GPRs afterwards.
    push    rsi

    ; Save host fs, gs, sysenter msr etc.
    mov     rax, [rbp + xCB * 2]                    ; HCPhysVmcbHost (64 bits physical address; x86: take low dword only)
    push    rax                                     ; save for the vmload after vmrun
    vmsave

    ; Fight spectre.
    INDIRECT_BRANCH_PREDICTION_BARRIER xSI, CPUMCTX_WSF_IBPB_ENTRY

    ; Setup rax for VMLOAD.
    mov     rax, [rbp + xCB * 2 + RTHCPHYS_CB]      ; HCPhysVmcb (64 bits physical address; take low dword only)

    ; Load guest general purpose registers (rax is loaded from the VMCB by VMRUN).
    mov     rbx, qword [xSI + CPUMCTX.ebx]
    mov     rcx, qword [xSI + CPUMCTX.ecx]
    mov     rdx, qword [xSI + CPUMCTX.edx]
    mov     rdi, qword [xSI + CPUMCTX.edi]
    mov     rbp, qword [xSI + CPUMCTX.ebp]
    mov     r8,  qword [xSI + CPUMCTX.r8]
    mov     r9,  qword [xSI + CPUMCTX.r9]
    mov     r10, qword [xSI + CPUMCTX.r10]
    mov     r11, qword [xSI + CPUMCTX.r11]
    mov     r12, qword [xSI + CPUMCTX.r12]
    mov     r13, qword [xSI + CPUMCTX.r13]
    mov     r14, qword [xSI + CPUMCTX.r14]
    mov     r15, qword [xSI + CPUMCTX.r15]
    mov     rsi, qword [xSI + CPUMCTX.esi]

    ; Clear the global interrupt flag & execute sti to make sure external interrupts cause a world switch.
    clgi
    sti

    ; Load guest FS, GS, Sysenter MSRs etc.
    vmload

    ; Run the VM.
    vmrun

    ; Save guest fs, gs, sysenter msr etc.
    vmsave

    ; Load host fs, gs, sysenter msr etc.
    pop     rax                         ; load HCPhysVmcbHost (pushed above)
    vmload

    ; Set the global interrupt flag again, but execute cli to make sure IF=0.
    cli
    stgi

    ; Pop the context pointer (pushed above) and save the guest GPRs (sans RSP and RAX).
    pop     rax

    mov     qword [rax + CPUMCTX.ebx], rbx
    mov     rbx, SPECTRE_FILLER64
    mov     qword [rax + CPUMCTX.ecx], rcx
    mov     rcx, rbx
    mov     qword [rax + CPUMCTX.edx], rdx
    mov     rdx, rbx
    mov     qword [rax + CPUMCTX.esi], rsi
    mov     rsi, rbx
    mov     qword [rax + CPUMCTX.edi], rdi
    mov     rdi, rbx
    mov     qword [rax + CPUMCTX.ebp], rbp
    mov     rbp, rbx
    mov     qword [rax + CPUMCTX.r8],  r8
    mov     r8, rbx
    mov     qword [rax + CPUMCTX.r9],  r9
    mov     r9, rbx
    mov     qword [rax + CPUMCTX.r10], r10
    mov     r10, rbx
    mov     qword [rax + CPUMCTX.r11], r11
    mov     r11, rbx
    mov     qword [rax + CPUMCTX.r12], r12
    mov     r12, rbx
    mov     qword [rax + CPUMCTX.r13], r13
    mov     r13, rbx
    mov     qword [rax + CPUMCTX.r14], r14
    mov     r14, rbx
    mov     qword [rax + CPUMCTX.r15], r15
    mov     r15, rbx

    ; Fight spectre.  Note! Trashes rax!
    INDIRECT_BRANCH_PREDICTION_BARRIER rax, CPUMCTX_WSF_IBPB_EXIT

    ; Restore the host xcr0 if necessary.
    pop     xCX
    test    ecx, ecx
    jnz     .xcr0_after_skip
    pop     xAX
    pop     xDX
    xsetbv                              ; ecx is already zero
.xcr0_after_skip:

    ; Restore host general purpose registers.
    MYPOPAD

    mov     eax, VINF_SUCCESS

    popf
    pop     rbp
    add     rsp, 6 * xCB
    ret
ENDPROC SVMR0VMRun64
%endif ; RT_ARCH_AMD64


; $Id: CPUMRCA.asm $
;; @file
; CPUM - Raw-mode Context Assembly Routines.
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
%include "VBox/vmm/vm.mac"
%include "VBox/err.mac"
%include "VBox/vmm/stam.mac"
%include "CPUMInternal.mac"
%include "iprt/x86.mac"
%include "VBox/vmm/cpum.mac"


;*******************************************************************************
;* External Symbols                                                            *
;*******************************************************************************
extern IMPNAME(g_CPUM)                 ; VMM GC Builtin import
extern IMPNAME(g_VM)                   ; VMM GC Builtin import
extern NAME(cpumRCHandleNPAndGP)       ; CPUMGC.cpp
extern NAME(CPUMRCAssertPreExecutionSanity)


;
; Enables write protection of Hypervisor memory pages.
; !note! Must be commented out for Trap8 debug handler.
;
%define ENABLE_WRITE_PROTECTION 1

BEGINCODE


;;
; Handles lazy FPU saving and restoring.
;
; This handler will implement lazy fpu (sse/mmx/stuff) saving.
; Two actions may be taken in this handler since the Guest OS may
; be doing lazy fpu switching. So, we'll have to generate those
; traps which the Guest CPU CTX shall have according to the
; its CR0 flags. If no traps for the Guest OS, we'll save the host
; context and restore the guest context.
;
; @returns  0 if caller should continue execution.
; @returns  VINF_EM_RAW_GUEST_TRAP if a guest trap should be generated.
; @param    pCpumCpu    [ebp+8]     Pointer to the CPUMCPU.
;
align 16
BEGINPROC   cpumHandleLazyFPUAsm
        push    ebp
        mov     ebp, esp
        push    ebx
        push    esi
        mov     ebx, [ebp + 8]
%define pCpumCpu ebx
%define pXState  esi

        ;
        ; Figure out what to do.
        ;
        ; There are two basic actions:
        ;   1. Save host fpu and restore guest fpu.
        ;   2. Generate guest trap.
        ;
        ; When entering the hypervisor we'll always enable MP (for proper wait
        ; trapping) and TS (for intercepting all fpu/mmx/sse stuff). The EM flag
        ; is taken from the guest OS in order to get proper SSE handling.
        ;
        ;
        ; Actions taken depending on the guest CR0 flags:
        ;
        ;   3    2    1
        ;  TS | EM | MP | FPUInstr | WAIT :: VMM Action
        ; ------------------------------------------------------------------------
        ;   0 |  0 |  0 | Exec     | Exec :: Clear TS & MP, Save HC, Load GC.
        ;   0 |  0 |  1 | Exec     | Exec :: Clear TS, Save HC, Load GC.
        ;   0 |  1 |  0 | #NM      | Exec :: Clear TS & MP, Save HC, Load GC;
        ;   0 |  1 |  1 | #NM      | Exec :: Clear TS, Save HC, Load GC.
        ;   1 |  0 |  0 | #NM      | Exec :: Clear MP, Save HC, Load GC. (EM is already cleared.)
        ;   1 |  0 |  1 | #NM      | #NM  :: Go to host taking trap there.
        ;   1 |  1 |  0 | #NM      | Exec :: Clear MP, Save HC, Load GC. (EM is already set.)
        ;   1 |  1 |  1 | #NM      | #NM  :: Go to host taking trap there.

        ;
        ; Before taking any of these actions we're checking if we have already
        ; loaded the GC FPU. Because if we have, this is an trap for the guest - raw ring-3.
        ;
        test    dword [pCpumCpu + CPUMCPU.fUseFlags], CPUM_USED_FPU_GUEST
        jz      hlfpua_not_loaded
        jmp     hlfpua_guest_trap

    ;
    ; Take action.
    ;
align 16
hlfpua_not_loaded:
        mov     eax, [pCpumCpu + CPUMCPU.Guest.cr0]
        and     eax, X86_CR0_MP | X86_CR0_EM | X86_CR0_TS
        jmp     dword [eax*2 + hlfpuajmp1]
align 16
;; jump table using fpu related cr0 flags as index.
hlfpuajmp1:
        RTCCPTR_DEF hlfpua_switch_fpu_ctx
        RTCCPTR_DEF hlfpua_switch_fpu_ctx
        RTCCPTR_DEF hlfpua_switch_fpu_ctx
        RTCCPTR_DEF hlfpua_switch_fpu_ctx
        RTCCPTR_DEF hlfpua_switch_fpu_ctx
        RTCCPTR_DEF hlfpua_guest_trap
        RTCCPTR_DEF hlfpua_switch_fpu_ctx
        RTCCPTR_DEF hlfpua_guest_trap
;; and mask for cr0.
hlfpu_afFlags:
        RTCCPTR_DEF ~(X86_CR0_TS | X86_CR0_MP)
        RTCCPTR_DEF ~(X86_CR0_TS)
        RTCCPTR_DEF ~(X86_CR0_TS | X86_CR0_MP)
        RTCCPTR_DEF ~(X86_CR0_TS)
        RTCCPTR_DEF ~(X86_CR0_MP)
        RTCCPTR_DEF 0
        RTCCPTR_DEF ~(X86_CR0_MP)
        RTCCPTR_DEF 0

        ;
        ; Action - switch FPU context and change cr0 flags.
        ;
align 16
hlfpua_switch_fpu_ctx:
        mov     ecx, cr0
        mov     edx, ecx
        and     ecx, [eax*2 + hlfpu_afFlags] ; Calc the new cr0 flags. Do NOT use ECX until we restore it!
        and     edx, ~(X86_CR0_TS | X86_CR0_EM)
        mov     cr0, edx                ; Clear flags so we don't trap here.

        test    dword [pCpumCpu + CPUMCPU.fUseFlags], CPUM_USED_FPU_HOST
        jnz     hlfpua_host_done

        mov     eax, [pCpumCpu + CPUMCPU.Host.fXStateMask]
        mov     pXState, [pCpumCpu + CPUMCPU.Host.pXStateRC]
        or      eax, eax
        jz      hlfpua_host_fxsave
        mov     edx, [pCpumCpu + CPUMCPU.Host.fXStateMask + 4]
        xsave   [pXState]
        jmp     hlfpua_host_done
hlfpua_host_fxsave:
        fxsave  [pXState]
hlfpua_host_done:

        mov     eax, [pCpumCpu + CPUMCPU.Guest.fXStateMask]
        mov     pXState, [pCpumCpu + CPUMCPU.Guest.pXStateRC]
        or      eax, eax
        jz      hlfpua_guest_fxrstor
        mov     edx, [pCpumCpu + CPUMCPU.Guest.fXStateMask + 4]
        xrstor  [pXState]
        jmp     hlfpua_guest_done
hlfpua_guest_fxrstor:
        fxrstor [pXState]
hlfpua_guest_done:

hlfpua_finished_switch:
        or      dword [pCpumCpu + CPUMCPU.fUseFlags], (CPUM_USED_FPU_HOST | CPUM_USED_FPU_GUEST | CPUM_USED_FPU_SINCE_REM)

        ; Load new CR0 value.
        mov     cr0, ecx                ; load the new cr0 flags.

        ; return continue execution.
        pop     esi
        pop     ebx
        xor     eax, eax
        leave
        ret

        ;
        ; Action - Generate Guest trap.
        ;
hlfpua_action_4:
hlfpua_guest_trap:
        pop     esi
        pop     ebx
        mov     eax, VINF_EM_RAW_GUEST_TRAP
        leave
        ret
ENDPROC     cpumHandleLazyFPUAsm


;;
; Calls a guest trap/interrupt handler directly
; Assumes a trap stack frame has already been setup on the guest's stack!
;
; @param   pRegFrame   [esp + 4]  Original trap/interrupt context
; @param   selCS       [esp + 8]  Code selector of handler
; @param   pHandler    [esp + 12] GC virtual address of handler
; @param   eflags      [esp + 16] Callee's EFLAGS
; @param   selSS       [esp + 20] Stack selector for handler
; @param   pEsp        [esp + 24] Stack address for handler
;
; @remark This call never returns!
;
; VMMRCDECL(void) CPUMGCCallGuestTrapHandler(PCPUMCTXCORE pRegFrame, uint32_t selCS, RTGCPTR pHandler, uint32_t eflags, uint32_t selSS, RTGCPTR pEsp);
align 16
BEGINPROC_EXPORTED CPUMGCCallGuestTrapHandler
        mov     ebp, esp

        ; construct iret stack frame
        push    dword [ebp + 20]        ; SS
        push    dword [ebp + 24]        ; ESP
        push    dword [ebp + 16]        ; EFLAGS
        push    dword [ebp + 8]         ; CS
        push    dword [ebp + 12]        ; EIP

        ;
        ; enable WP
        ;
%ifdef ENABLE_WRITE_PROTECTION
        mov     eax, cr0
        or      eax, X86_CR0_WRITE_PROTECT
        mov     cr0, eax
%endif

        ; restore CPU context (all except cs, eip, ss, esp & eflags; which are restored or overwritten by iret)
        mov     ebp, [ebp + 4]          ; pRegFrame
        mov     ebx, [ebp + CPUMCTXCORE.ebx]
        mov     ecx, [ebp + CPUMCTXCORE.ecx]
        mov     edx, [ebp + CPUMCTXCORE.edx]
        mov     esi, [ebp + CPUMCTXCORE.esi]
        mov     edi, [ebp + CPUMCTXCORE.edi]

        ;; @todo  load segment registers *before* enabling WP.
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_GS | CPUM_HANDLER_CTXCORE_IN_EBP
        mov     gs, [ebp + CPUMCTXCORE.gs.Sel]
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_FS | CPUM_HANDLER_CTXCORE_IN_EBP
        mov     fs, [ebp + CPUMCTXCORE.fs.Sel]
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_ES | CPUM_HANDLER_CTXCORE_IN_EBP
        mov     es, [ebp + CPUMCTXCORE.es.Sel]
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_DS | CPUM_HANDLER_CTXCORE_IN_EBP
        mov     ds, [ebp + CPUMCTXCORE.ds.Sel]

        mov     eax, [ebp + CPUMCTXCORE.eax]
        mov     ebp, [ebp + CPUMCTXCORE.ebp]

        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_IRET
        iret
ENDPROC CPUMGCCallGuestTrapHandler


;;
; Performs an iret to V86 code
; Assumes a trap stack frame has already been setup on the guest's stack!
;
; @param   pRegFrame   Original trap/interrupt context
;
; This function does not return!
;
;VMMRCDECL(void) CPUMGCCallV86Code(PCPUMCTXCORE pRegFrame);
align 16
BEGINPROC CPUMGCCallV86Code
        push    ebp
        mov     ebp, esp
        mov     ebx, [ebp + 8]          ; pRegFrame

        ; Construct iret stack frame.
        push    dword [ebx + CPUMCTXCORE.gs.Sel]
        push    dword [ebx + CPUMCTXCORE.fs.Sel]
        push    dword [ebx + CPUMCTXCORE.ds.Sel]
        push    dword [ebx + CPUMCTXCORE.es.Sel]
        push    dword [ebx + CPUMCTXCORE.ss.Sel]
        push    dword [ebx + CPUMCTXCORE.esp]
        push    dword [ebx + CPUMCTXCORE.eflags]
        push    dword [ebx + CPUMCTXCORE.cs.Sel]
        push    dword [ebx + CPUMCTXCORE.eip]

        ; Invalidate all segment registers.
        mov     al, ~CPUMSELREG_FLAGS_VALID
        and     [ebx + CPUMCTXCORE.fs.fFlags], al
        and     [ebx + CPUMCTXCORE.ds.fFlags], al
        and     [ebx + CPUMCTXCORE.es.fFlags], al
        and     [ebx + CPUMCTXCORE.ss.fFlags], al
        and     [ebx + CPUMCTXCORE.gs.fFlags], al
        and     [ebx + CPUMCTXCORE.cs.fFlags], al

        ;
        ; enable WP
        ;
%ifdef ENABLE_WRITE_PROTECTION
        mov     eax, cr0
        or      eax, X86_CR0_WRITE_PROTECT
        mov     cr0, eax
%endif

        ; restore CPU context (all except cs, eip, ss, esp, eflags, ds, es, fs & gs; which are restored or overwritten by iret)
        mov     eax, [ebx + CPUMCTXCORE.eax]
        mov     ecx, [ebx + CPUMCTXCORE.ecx]
        mov     edx, [ebx + CPUMCTXCORE.edx]
        mov     esi, [ebx + CPUMCTXCORE.esi]
        mov     edi, [ebx + CPUMCTXCORE.edi]
        mov     ebp, [ebx + CPUMCTXCORE.ebp]
        mov     ebx, [ebx + CPUMCTXCORE.ebx]

        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_IRET
        iret
ENDPROC CPUMGCCallV86Code


;;
; This is a main entry point for resuming (or starting) guest
; code execution.
;
; We get here directly from VMMSwitcher.asm (jmp at the end
; of VMMSwitcher_HostToGuest).
;
; This call never returns!
;
; @param    edx     Pointer to CPUMCPU structure.
;
align 16
BEGINPROC_EXPORTED CPUMGCResumeGuest
%ifdef VBOX_STRICT
        ; Call CPUM to check sanity.
        push    edx
        mov     edx, IMP(g_VM)
        push    edx
        call    NAME(CPUMRCAssertPreExecutionSanity)
        add     esp, 4
        pop     edx
%endif

        ;
        ; Setup iretd
        ;
        push    dword [edx + CPUMCPU.Guest.ss.Sel]
        push    dword [edx + CPUMCPU.Guest.esp]
        push    dword [edx + CPUMCPU.Guest.eflags]
        push    dword [edx + CPUMCPU.Guest.cs.Sel]
        push    dword [edx + CPUMCPU.Guest.eip]

        ;
        ; Restore registers.
        ;
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_ES
        mov     es,  [edx + CPUMCPU.Guest.es.Sel]
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_FS
        mov     fs,  [edx + CPUMCPU.Guest.fs.Sel]
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_GS
        mov     gs,  [edx + CPUMCPU.Guest.gs.Sel]

%ifdef VBOX_WITH_STATISTICS
        ;
        ; Statistics.
        ;
        push    edx
        mov     edx, IMP(g_VM)
        lea     edx, [edx + VM.StatTotalQemuToGC]
        STAM_PROFILE_ADV_STOP edx

        mov     edx, IMP(g_VM)
        lea     edx, [edx + VM.StatTotalInGC]
        STAM_PROFILE_ADV_START edx
        pop     edx
%endif

        ;
        ; enable WP
        ;
%ifdef ENABLE_WRITE_PROTECTION
        mov     eax, cr0
        or      eax, X86_CR0_WRITE_PROTECT
        mov     cr0, eax
%endif

        ;
        ; Continue restore.
        ;
        mov     esi, [edx + CPUMCPU.Guest.esi]
        mov     edi, [edx + CPUMCPU.Guest.edi]
        mov     ebp, [edx + CPUMCPU.Guest.ebp]
        mov     ebx, [edx + CPUMCPU.Guest.ebx]
        mov     ecx, [edx + CPUMCPU.Guest.ecx]
        mov     eax, [edx + CPUMCPU.Guest.eax]
        push    dword [edx + CPUMCPU.Guest.ds.Sel]
        mov     edx, [edx + CPUMCPU.Guest.edx]
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_DS
        pop     ds

        ; restart execution.
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_IRET
        iretd
ENDPROC     CPUMGCResumeGuest


;;
; This is a main entry point for resuming (or starting) guest
; code execution for raw V86 mode
;
; We get here directly from VMMSwitcher.asm (jmp at the end
; of VMMSwitcher_HostToGuest).
;
; This call never returns!
;
; @param    edx     Pointer to CPUMCPU structure.
;
align 16
BEGINPROC_EXPORTED CPUMGCResumeGuestV86
%ifdef VBOX_STRICT
        ; Call CPUM to check sanity.
        push    edx
        mov     edx, IMP(g_VM)
        push    edx
        call    NAME(CPUMRCAssertPreExecutionSanity)
        add     esp, 4
        pop     edx
%endif

        ;
        ; Setup iretd
        ;
        push    dword [edx + CPUMCPU.Guest.gs.Sel]
        push    dword [edx + CPUMCPU.Guest.fs.Sel]
        push    dword [edx + CPUMCPU.Guest.ds.Sel]
        push    dword [edx + CPUMCPU.Guest.es.Sel]

        push    dword [edx + CPUMCPU.Guest.ss.Sel]
        push    dword [edx + CPUMCPU.Guest.esp]

        push    dword [edx + CPUMCPU.Guest.eflags]
        push    dword [edx + CPUMCPU.Guest.cs.Sel]
        push    dword [edx + CPUMCPU.Guest.eip]

        ;
        ; Restore registers.
        ;

%ifdef VBOX_WITH_STATISTICS
        ;
        ; Statistics.
        ;
        push    edx
        mov     edx, IMP(g_VM)
        lea     edx, [edx + VM.StatTotalQemuToGC]
        STAM_PROFILE_ADV_STOP edx

        mov     edx, IMP(g_VM)
        lea     edx, [edx + VM.StatTotalInGC]
        STAM_PROFILE_ADV_START edx
        pop     edx
%endif

        ;
        ; enable WP
        ;
%ifdef ENABLE_WRITE_PROTECTION
        mov     eax, cr0
        or      eax, X86_CR0_WRITE_PROTECT
        mov     cr0, eax
%endif

        ;
        ; Continue restore.
        ;
        mov     esi, [edx + CPUMCPU.Guest.esi]
        mov     edi, [edx + CPUMCPU.Guest.edi]
        mov     ebp, [edx + CPUMCPU.Guest.ebp]
        mov     ecx, [edx + CPUMCPU.Guest.ecx]
        mov     ebx, [edx + CPUMCPU.Guest.ebx]
        mov     eax, [edx + CPUMCPU.Guest.eax]
        mov     edx, [edx + CPUMCPU.Guest.edx]

        ; restart execution.
        TRPM_NP_GP_HANDLER NAME(cpumRCHandleNPAndGP), CPUM_HANDLER_IRET
        iretd
ENDPROC     CPUMGCResumeGuestV86


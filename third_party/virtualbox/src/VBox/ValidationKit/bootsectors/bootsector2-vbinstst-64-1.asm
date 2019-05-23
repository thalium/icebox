; $Id: bootsector2-vbinstst-64-1.asm $
;; @file
; Bootsector tests instructions in 64-bit mode.
;   VBoxManage setextradata bs-vbinstst-64-1 VBoxInternal/Devices/VMMDev/0/Config/TestingEnabled  1
;

;
; Copyright (C) 2007-2017 Oracle Corporation
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


%include "iprt/asmdefs.mac"
%include "iprt/x86.mac"
%include "VBox/VMMDevTesting.mac"

;
; Include and execute the init code.
;
        %define BS2_INIT_RM
        %define BS2_INC_LM64
        %define BS2_WITH_TRAPS
        %include "bootsector2-common-init-code.mac"


;
; The benchmark driver
;
BEGINPROC main
        ;
        ; Test prologue.
        ;
        cli
        mov     ax, .s_szTstName
        call    TestInit_r86
        call    Bs2EnableA20_r86
        call    Bs2PanicIfVMMDevTestingIsMissing_r86
        lea     eax, [dword the_end]
        cmp     eax, dword 80000h
        jae     .size_nok


        ;
        ; Do the testing.
        ;
        call    Bs2IsModeSupported_rm_lm64
        jz      .done
        call    Bs2EnterMode_rm_lm64
BITS 64

        call    TestInstrMain_lm64

        call    TestSubDone_p64
        call    Bs2ExitMode_lm64
BITS 16
.done:

        ;
        ; We're done.
        ;
        call    TestTerm_r86
        call    Bs2Panic
        jmp     .done

.size_nok:
        push    eax
        push    word ds
        push    .s_szSizeNok
        call    TestFailedF_r86
        jmp     .done

.s_szSizeNok:
        db      'Test is too big (%RX32)', 0
.s_szTstName:
        db      'VBInsTst-64-1', 0
ENDPROC   main


;
; Instantiate the template code.
;
%include "bootsector2-template-footer.mac"  ; reset the initial environemnt.

%define TMPL_LM64
%include "bootsector2-template-header.mac"
BITS 64
%include "VBInsTst-64.asm"
%include "bootsector2-template-footer.mac"


;
; End sections and image.
;
%include "bootsector2-common-end.mac"


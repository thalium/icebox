; $Id: bootsector2-vbinstst-big-template.asm $
;; @file
; Boot Sector 2 with big instruction test image template.  For use with
; bootsector2-vbinstst-kernel.asm.  Requires:
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


;
; Set up the assembler environment.
;
%include "bootsector2-first.mac"
%include "bootsector2-api.mac"

%include "bootsector2-template-footer.mac"
%ifdef BS2_BIG_IMAGE_LM64
 %define TMPL_LM64
 %include "bootsector2-template-header.mac"
        BITS 64

%elifdef BS2_BIG_IMAGE_PAE32
 %define TMPL_PAE32
 %include "bootsector2-template-header.mac"
        BITS 32

%elifdef BS2_BIG_IMAGE_PP32
 %define TMPL_PP32
 %include "bootsector2-template-header.mac"
        BITS 32

%else
 %error Do not know which mode to run in.
        mov bad,instr
%endif


        ORG BS2_BIG_LOAD_ADDR

;
; The entry point is the first byte in the image.
;
bs2_big_image_start:
entrypoint:
        mov     xAX, .s_szTestName
        call    [TMPL_NM_CMN(g_pfnTestInit) xWrtRIP]
        call    TMPL_NM(TestInstrMain)
        call    [TMPL_NM_CMN(g_pfnTestTerm) xWrtRIP]
.hltloop:
        hlt
        jmp     .hltloop

.s_szTestName:
        db BS2_BIG_IMAGE_GEN_TEST_NAME, 0


;
; Instantiate the template code.
;
%include "BS2_BIG_IMAGE_GEN_SOURCE_FILE"


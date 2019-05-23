; $Id: bs3-cpu-instr-2-asm.asm $
;; @file
; BS3Kit - bs3-cpu-instr-2
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


;*********************************************************************************************************************************
;*  Header Files                                                                                                                 *
;*********************************************************************************************************************************
%include "bs3kit.mac"


;*********************************************************************************************************************************
;*      Global Variables                                                                                                         *
;*********************************************************************************************************************************
;BS3_BEGIN_DATA16
;BS3_GLOBAL_DATA g_bs3CpuBasic2_ud2_FlatAddr, 4
;        dd  _bs3CpuBasic2_ud2 wrt FLAT



;
; CPU mode agnostic test code snippets.
;
BS3_BEGIN_TEXT16

BS3_PROC_BEGIN _bs3CpuInstr2_imul_bl_ud2
        imul    bl
.again:
        ud2
        jmp     .again
BS3_PROC_END   _bs3CpuInstr2_imul_bl_ud2



;
; Instantiate code templates.
;
BS3_INSTANTIATE_COMMON_TEMPLATE          "bs3-cpu-instr-2-template.mac"
BS3_INSTANTIATE_TEMPLATE_WITH_WEIRD_ONES "bs3-cpu-instr-2-template.mac"


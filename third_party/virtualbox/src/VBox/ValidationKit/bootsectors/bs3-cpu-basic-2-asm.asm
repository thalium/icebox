; $Id: bs3-cpu-basic-2-asm.asm $
;; @file
; BS3Kit - bs3-cpu-basic-2
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
BS3_BEGIN_DATA16
BS3_GLOBAL_DATA g_bs3CpuBasic2_ud2_FlatAddr, 4
        dd  _bs3CpuBasic2_ud2 wrt FLAT



;
; CPU mode agnostic test code snippets.
;
BS3_BEGIN_TEXT16

BS3_PROC_BEGIN _bs3CpuBasic2_ud2
.again:
        ud2
        jmp     .again
BS3_PROC_END   _bs3CpuBasic2_ud2


BS3_PROC_BEGIN _bs3CpuBasic2_Int80
        int     80h
.again: ud2
        jmp     .again
BS3_PROC_END   _bs3CpuBasic2_Int80


BS3_PROC_BEGIN _bs3CpuBasic2_Int81
        int     81h
.again: ud2
        jmp     .again
BS3_PROC_END   _bs3CpuBasic2_Int81


BS3_PROC_BEGIN _bs3CpuBasic2_Int82
        int     82h
.again: ud2
        jmp     .again
BS3_PROC_END   _bs3CpuBasic2_Int82


BS3_PROC_BEGIN _bs3CpuBasic2_Int83
        int     83h
.again: ud2
        jmp     .again
BS3_PROC_END   _bs3CpuBasic2_Int83


BS3_PROC_BEGIN _bs3CpuBasic2_iret
        iret
BS3_PROC_END   _bs3CpuBasic2_iret
AssertCompile(_bs3CpuBasic2_iret_EndProc - _bs3CpuBasic2_iret == 1)


BS3_PROC_BEGIN _bs3CpuBasic2_iret_opsize
        iretd
BS3_PROC_END   _bs3CpuBasic2_iret_opsize
AssertCompile(_bs3CpuBasic2_iret_opsize_EndProc - _bs3CpuBasic2_iret_opsize == 2)


BS3_PROC_BEGIN _bs3CpuBasic2_iret_rexw
        BS3_SET_BITS 64
        iretq
        BS3_SET_BITS 16
BS3_PROC_END   _bs3CpuBasic2_iret_rexw
AssertCompile(_bs3CpuBasic2_iret_rexw_EndProc - _bs3CpuBasic2_iret_rexw == 2)


;
; CPU mode agnostic test code snippets.
;
BS3_BEGIN_TEXT32

;;
; @param    [xBP + xCB*2]   puDst
; @param    [xBP + xCB*3]   uNewValue
BS3_PROC_BEGIN_CMN bs3CpuBasic2_Store_mov, BS3_PBC_NEAR
        push    xBP
        mov     xBP, xSP
        mov     xCX, [xBP + xCB*2]
        mov     xAX, [xBP + xCB*3]
        mov     [xCX], xAX
        leave
        ret
BS3_PROC_END_CMN   bs3CpuBasic2_Store_mov

;;
; @param    [xBP + xCB*2]   puDst
; @param    [xBP + xCB*3]   uNewValue
BS3_PROC_BEGIN_CMN bs3CpuBasic2_Store_xchg, BS3_PBC_NEAR
        push    xBP
        mov     xBP, xSP
        mov     xCX, [xBP + xCB*2]
        mov     xAX, [xBP + xCB*3]
        xchg    [xCX], xAX
        leave
        ret
BS3_PROC_END_CMN   bs3CpuBasic2_Store_xchg

;;
; @param    [xBP + xCB*2]   puDst
; @param    [xBP + xCB*3]   uNewValue
; @param    [xBP + xCB*4]   uOldValue
BS3_PROC_BEGIN_CMN bs3CpuBasic2_Store_cmpxchg, BS3_PBC_NEAR
        push    xBP
        mov     xBP, xSP
        mov     xCX, [xBP + xCB*2]
        mov     xDX, [xBP + xCB*3]
        mov     xAX, [xBP + xCB*4]
.again:
        cmpxchg [xCX], xDX
        jnz     .again
        leave
        ret
BS3_PROC_END_CMN   bs3CpuBasic2_Store_cmpxchg


BS3_BEGIN_TEXT16

;
; Instantiate code templates.
;
BS3_INSTANTIATE_COMMON_TEMPLATE          "bs3-cpu-basic-2-template.mac"
BS3_INSTANTIATE_TEMPLATE_WITH_WEIRD_ONES "bs3-cpu-basic-2-template.mac"


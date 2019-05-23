; $Id: bs3-cmn-ExtCtxRestore.asm $
;; @file
; BS3Kit - Bs3ExtCtxRestore.
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

%include "bs3kit-template-header.mac"


;;
; Restores the extended CPU context (FPU, SSE, AVX, ++).
;
; @param    pExtCtx
;
BS3_PROC_BEGIN_CMN Bs3ExtCtxRestore, BS3_PBC_NEAR
        push    xBP
        mov     xBP, xSP
        push    sAX
        push    sCX
        push    sDX
        push    xBX
BONLY16 push    es

%if ARCH_BITS == 16
        les     bx, [xBP + xCB + cbCurRetAddr]
        mov     al, [es:bx + BS3EXTCTX.enmMethod]
        cmp     al, BS3EXTCTXMETHOD_XSAVE
        je      .do_16_xsave
        cmp     al, BS3EXTCTXMETHOD_FXSAVE
        je      .do_16_fxsave
        cmp     al, BS3EXTCTXMETHOD_ANCIENT
        je      .do_16_ancient
        int3

.do_16_ancient:
        frstor  [es:bx + BS3EXTCTX.Ctx]
        jmp     .return

.do_16_fxsave:
        fxrstor  [es:bx + BS3EXTCTX.Ctx]
        jmp     .return

.do_16_xsave:
        xor     ecx, ecx
        mov     eax, [es:bx + BS3EXTCTX.fXcr0Nominal]
        mov     edx, [es:bx + BS3EXTCTX.fXcr0Nominal + 4]
        xsetbv

        xrstor  [es:bx + BS3EXTCTX.Ctx]

        mov     eax, [es:bx + BS3EXTCTX.fXcr0Saved]
        mov     edx, [es:bx + BS3EXTCTX.fXcr0Saved + 4]
        xsetbv
        ;jmp     .return

%else
BONLY32 mov     ebx, [xBP + xCB + cbCurRetAddr]
BONLY64 mov     rbx, rcx

        mov     al, [xBX + BS3EXTCTX.enmMethod]
        cmp     al, BS3EXTCTXMETHOD_XSAVE
        je      .do_xsave
        cmp     al, BS3EXTCTXMETHOD_FXSAVE
        je      .do_fxsave
        cmp     al, BS3EXTCTXMETHOD_ANCIENT
        je      .do_ancient
        int3

.do_ancient:
        frstor  [xBX + BS3EXTCTX.Ctx]
        jmp     .return

.do_fxsave:
BONLY32 fxrstor [xBX + BS3EXTCTX.Ctx]
BONLY64 fxrstor64 [xBX + BS3EXTCTX.Ctx]
        jmp     .return

.do_xsave:
        xor     ecx, ecx
        mov     eax, [xBX + BS3EXTCTX.fXcr0Nominal]
        mov     edx, [xBX + BS3EXTCTX.fXcr0Nominal + 4]
        xsetbv

BONLY32 xrstor  [xBX + BS3EXTCTX.Ctx]
BONLY64 xrstor64 [xBX + BS3EXTCTX.Ctx]

        mov     eax, [xBX + BS3EXTCTX.fXcr0Saved]
        mov     edx, [xBX + BS3EXTCTX.fXcr0Saved + 4]
        xsetbv
        ;jmp     .return

%endif

.return:
BONLY16 pop     es
        pop     xBX
        pop     sDX
        pop     sCX
        pop     sAX
        mov     xSP, xBP
        pop     xBP
        ret
BS3_PROC_END_CMN   Bs3ExtCtxRestore


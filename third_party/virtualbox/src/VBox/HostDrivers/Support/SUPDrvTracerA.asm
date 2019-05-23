; $Id: SUPDrvTracerA.asm $
;; @file
; VirtualBox Support Driver - Tracer Interface, Assembly bits.
;

;
; Copyright (C) 2012-2017 Oracle Corporation
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

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"


; External data.
extern NAME(g_pfnSupdrvProbeFireKernel)


BEGINCODE

;; Dummy stub function that just returns.
BEGINPROC supdrvTracerProbeFireStub
        ret
ENDPROC   supdrvTracerProbeFireStub


;; Tail jump function.
EXPORTEDNAME SUPR0TracerFireProbe
%ifdef RT_ARCH_AMD64
        mov     rax, [NAME(g_pfnSupdrvProbeFireKernel) wrt rip]
        jmp     rax
%else
        mov     eax, [NAME(g_pfnSupdrvProbeFireKernel)]
        jmp     eax
%endif
ENDPROC SUPR0TracerFireProbe


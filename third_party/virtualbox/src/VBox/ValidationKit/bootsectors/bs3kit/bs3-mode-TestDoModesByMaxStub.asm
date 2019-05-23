; $Id: bs3-mode-TestDoModesByMaxStub.asm $
;; @file
; BS3Kit - Bs3TestDoModesByMax near stub.
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
%include "bs3kit-template-header.mac"

;
; Near stub for the API call (16-bit only).
;
%if TMPL_BITS == 16
 %if TMPL_MODE == BS3_MODE_RM
BS3_BEGIN_RMTEXT16
 %endif
BS3_BEGIN_TEXT16_NEARSTUBS
BS3_PROC_BEGIN_MODE Bs3TestDoModesByMax, BS3_PBC_NEAR
        pop     ax
        push    cs
        push    ax
 %if TMPL_MODE == BS3_MODE_RM
        extern TMPL_FAR_NM(Bs3TestDoModesByMax):wrt BS3GROUPRMTEXT16
        jmp far TMPL_FAR_NM(Bs3TestDoModesByMax)
 %else
        extern TMPL_FAR_NM(Bs3TestDoModesByMax):wrt CGROUP16
        jmp     TMPL_NM(Bs3TestDoModesByMax)
 %endif
BS3_PROC_END_MODE   Bs3TestDoModesByMax
%endif


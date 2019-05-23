; $Id: bs3-mode-PagingGetRootForPP16.asm $
;; @file
; BS3Kit - Bs3PagingGetRootForPP16
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


extern TMPL_NM(Bs3PagingGetRootForPP32)


;;
; @cproto   BS3_DECL(uint32_t) Bs3PagingGetRootForPP16(void)
;
; @returns  eax
;
; @uses     ax
;
; @remarks  returns value in EAX, not dx:ax!
;
BS3_PROC_BEGIN_MODE Bs3PagingGetRootForPP16, BS3_PBC_NEAR ; Internal function, no far variant necessary.
        jmp     TMPL_NM(Bs3PagingGetRootForPP32)
BS3_PROC_END_MODE   Bs3PagingGetRootForPP16


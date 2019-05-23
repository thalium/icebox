; $Id: bs3-cmn-ConvertRMStackToP16UsingCxReturnToAx.asm $
;; @file
; BS3Kit - Bs3ConvertRMStackToP16UsingCxReturnToAx.
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


%if TMPL_BITS != 16
 %error 16-bit only!
%endif

;;
; An internal helper for converting a real-mode stack into a 16-bit protected
; mode stack.
;
; This is used by the mode switchers that ends up in 16-bit mode.  It is
; assumed that we're in ring-0.
;
; @param    ax          The return address.
;
; @uses     cx, ss, esp
;
BS3_PROC_BEGIN_CMN Bs3ConvertRMStackToP16UsingCxReturnToAx, BS3_PBC_NEAR

        ;
        ; Check if it looks like the normal stack, if use BS3_SEL_R0_SS16.
        ;
        mov     cx, ss
        cmp     cx, 0
        jne     .stack_tiled
        mov     cx, BS3_SEL_R0_SS16
        mov     ss, cx
        jmp     ax

        ;
        ; Some custom stack address, just use the 16-bit tiled mappings
        ;
.stack_tiled:
int3                                    ; debug this, shouldn't happen yet.  Bs3EnteredMode_xxx isn't prepared.
        shl     cx, 4
        add     sp, cx
        mov     cx, ss
        jc      .stack_carry
        shr     cx, 12
        jmp     .stack_join_up_again
.stack_carry:
        shr     cx, 12
        inc     cx
.stack_join_up_again:
        shl     cx, 3
        adc     cx, BS3_SEL_TILED
        mov     ss, cx
        jmp     ax

BS3_PROC_END_CMN   Bs3ConvertRMStackToP16UsingCxReturnToAx


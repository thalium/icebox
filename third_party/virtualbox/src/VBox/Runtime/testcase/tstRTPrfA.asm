; $Id: tstRTPrfA.asm $
;; @file
; IPRT - Comparing CPU registers and memory (cache).
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


%define NUM_LOOPS 10000h

BEGINCODE

BEGINPROC tstRTPRfARegisterAccess
        push    xBP
        mov     xBP, xSP
        and     xSP, ~3fh               ; 64 byte align xSP
        push    xBP
        mov     xBP, xSP
        sub     xSP, 20h

        mov     xAX, 1
        mov     xDX, 1
        mov     ecx, NUM_LOOPS
.again:
        add     eax, ecx
        add     xDX, xAX
        shr     xAX, 3
        shl     xAX, 1
        xor     xDX, 01010101h

        add     eax, ecx
        add     xDX, xAX
        shr     xAX, 3
        shl     xAX, 1
        xor     xDX, 01010101h

        add     eax, ecx
        add     xDX, xAX
        shr     xAX, 3
        shl     xAX, 1
        xor     xDX, 01010101h

        dec     ecx
        jnz     .again

        leave
        leave
        ret
ENDPROC   tstRTPRfARegisterAccess


BEGINPROC tstRTPRfAMemoryAccess
        push    xBP
        mov     xBP, xSP
        and     xSP, ~3fh               ; 64 byte align xSP
        push    xBP
        mov     xBP, xSP
        sub     xSP, 20h

%define VAR_XAX [xBP - xCB*1]
%define VAR_XDX [xBP - xCB*2]
%define VAR_ECX [xBP - xCB*3]

        mov     RTCCPTR_PRE VAR_XAX, 1
        mov     RTCCPTR_PRE VAR_XDX, 1
        mov     dword VAR_ECX, NUM_LOOPS
.again:

        mov     eax, VAR_ECX
        add     VAR_XAX, eax
        mov     xAX, VAR_XAX
        add     VAR_XDX, xAX
        shr     RTCCPTR_PRE VAR_XAX, 3
        shl     RTCCPTR_PRE VAR_XAX, 1
        xor     RTCCPTR_PRE VAR_XDX, 01010101h

        mov     eax, VAR_ECX
        add     VAR_XAX, eax
        mov     xAX, VAR_XAX
        add     VAR_XDX, xAX
        shr     RTCCPTR_PRE VAR_XAX, 3
        shl     RTCCPTR_PRE VAR_XAX, 1
        xor     RTCCPTR_PRE VAR_XDX, 01010101h

        mov     eax, VAR_ECX
        add     VAR_XAX, eax
        mov     xAX, VAR_XAX
        add     VAR_XDX, xAX
        shr     RTCCPTR_PRE VAR_XAX, 3
        shl     RTCCPTR_PRE VAR_XAX, 1
        xor     RTCCPTR_PRE VAR_XDX, 01010101h

        dec     dword VAR_ECX
        jnz     .again

%undef VAR_XAX
%undef VAR_XDX
%undef VAR_ECX

        leave
        leave
        ret
ENDPROC   tstRTPRfAMemoryAccess


BEGINPROC tstRTPRfAMemoryUnalignedAccess
        push    xBP
        mov     xBP, xSP
        and     xSP, ~3fh               ; 64 byte align xSP
        push    xBP
        mov     xBP, xSP
        sub     xSP, 20h

%define VAR_XAX [xBP - xCB*1 - 1]
%define VAR_XDX [xBP - xCB*2 - 1]
%define VAR_ECX [xBP - xCB*3 - 1]

        mov     RTCCPTR_PRE VAR_XAX, 1
        mov     RTCCPTR_PRE VAR_XDX, 1
        mov     dword VAR_ECX, NUM_LOOPS
.again:

        mov     eax, VAR_ECX
        add     VAR_XAX, eax
        mov     xAX, VAR_XAX
        add     VAR_XDX, xAX
        shr     RTCCPTR_PRE VAR_XAX, 3
        shl     RTCCPTR_PRE VAR_XAX, 1
        xor     RTCCPTR_PRE VAR_XDX, 01010101h

        mov     eax, VAR_ECX
        add     VAR_XAX, eax
        mov     xAX, VAR_XAX
        add     VAR_XDX, xAX
        shr     RTCCPTR_PRE VAR_XAX, 3
        shl     RTCCPTR_PRE VAR_XAX, 1
        xor     RTCCPTR_PRE VAR_XDX, 01010101h

        mov     eax, VAR_ECX
        add     VAR_XAX, eax
        mov     xAX, VAR_XAX
        add     VAR_XDX, xAX
        shr     RTCCPTR_PRE VAR_XAX, 3
        shl     RTCCPTR_PRE VAR_XAX, 1
        xor     RTCCPTR_PRE VAR_XDX, 01010101h

        dec     dword VAR_ECX
        jnz     .again

%undef VAR_XAX
%undef VAR_XDX
%undef VAR_ECX

        leave
        leave
        ret
ENDPROC   tstRTPRfAMemoryUnalignedAccess


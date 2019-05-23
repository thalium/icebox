; $Id: Win2kWorkaroundsA.asm $
;; @file
; VirtualBox Windows Guest Shared Folders - Windows 2000 Hacks, Assembly Parts.
;

;
; Copyright (C) 2006-2017 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

;*******************************************************************************
;*  Header Files                                                               *
;*******************************************************************************
%include "iprt/asmdefs.mac"

%ifndef RT_ARCH_X86
 %error "This is x86 only code.
%endif


%macro MAKE_IMPORT_ENTRY 2
extern _ %+ %1 %+ @ %+ %2
global __imp__ %+ %1 %+ @ %+ %2
__imp__ %+ %1 %+ @ %+ %2:
    dd _ %+ %1 %+ @ %+ %2

%endmacro

BEGINDATA

;MAKE_IMPORT_ENTRY FsRtlTeardownPerStreamContexts, 4
MAKE_IMPORT_ENTRY RtlGetVersion, 4
MAKE_IMPORT_ENTRY PsGetProcessImageFileName, 4


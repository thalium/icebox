; $Id: vcc100-kernel32-fakesA.asm $
;; @file
; IPRT - Wrappers for kernel32 APIs misisng NT4.
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
GLOBALNAME vcc100_kernel32_fakes_asm

MAKE_IMPORT_ENTRY DecodePointer, 4
MAKE_IMPORT_ENTRY EncodePointer, 4
MAKE_IMPORT_ENTRY InitializeCriticalSectionAndSpinCount, 8
MAKE_IMPORT_ENTRY HeapSetInformation, 16
MAKE_IMPORT_ENTRY HeapQueryInformation, 20
MAKE_IMPORT_ENTRY CreateTimerQueue, 0
MAKE_IMPORT_ENTRY CreateTimerQueueTimer, 28
MAKE_IMPORT_ENTRY DeleteTimerQueueTimer, 12
MAKE_IMPORT_ENTRY InitializeSListHead, 4
MAKE_IMPORT_ENTRY InterlockedFlushSList, 4
MAKE_IMPORT_ENTRY InterlockedPopEntrySList, 4
MAKE_IMPORT_ENTRY InterlockedPushEntrySList, 8
MAKE_IMPORT_ENTRY QueryDepthSList, 4
MAKE_IMPORT_ENTRY VerifyVersionInfoA, 16
MAKE_IMPORT_ENTRY VerSetConditionMask, 16


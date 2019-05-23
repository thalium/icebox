;; @file
; VBoxGuest - VirtualBox Guest Additions Interface, MASM/ALP header.
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


;; ASSUMES OS/2 for now.
VBOXGUEST_DEVICE_NAME_SHORT EQU "vboxgst$"

VBGLOS2ATTACHDD STRUC
u32Version         DD ?
u32Session         DD ?
pfnServiceEP       DD ?
fpfnServiceEP      DD ?
fpfnServiceAsmEP   DD ?
VBGLOS2ATTACHDD ENDS

VBGLREQHDR STRUC
cbIn               DD ?
uVersion           DD ?
uType              DD ?
rc                 DD ?
cbOut              DD ?
uReserved          DD ?
VBGLREQHDR ENDS

VBGL_IOC_VERSION        EQU     000010000h

; From VMMDev.h
VMMDEV_VERSION_MAJOR    EQU     1
VMMDEV_VERSION_MINOR    EQU     4
VMMDEV_VERSION          EQU     000010004h


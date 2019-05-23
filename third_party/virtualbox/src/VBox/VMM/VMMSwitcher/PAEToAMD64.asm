; $Id: PAEToAMD64.asm $
;; @file
; VMM - World Switchers, PAE to AMD64
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
;*   Defined Constants And Macros                                              *
;*******************************************************************************
%define SWITCHER_TYPE               VMMSWITCHER_PAE_TO_AMD64
%define SWITCHER_DESCRIPTION        "PAE to/from AMD64 intermediate context"
%define NAME_OVERLOAD(name)         vmmR3SwitcherPAEToAMD64_ %+ name
%define SWITCHER_FIX_INTER_CR3_HC   FIX_INTER_PAE_CR3
%define NEED_PAE_ON_HOST  1

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "VMMSwitcher/LegacyandAMD64.mac"


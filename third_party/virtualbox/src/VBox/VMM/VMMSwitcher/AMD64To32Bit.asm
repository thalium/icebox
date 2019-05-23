; $Id: AMD64To32Bit.asm $
;; @file
; VMM - World Switchers, AMD64 to 32-bit
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
%undef  SWITCHER_TO_PAE
%define SWITCHER_TO_32BIT           1
%define SWITCHER_TYPE               VMMSWITCHER_AMD64_TO_32
%define SWITCHER_DESCRIPTION        "AMD64 to/from 32-bit"
%define NAME_OVERLOAD(name)         vmmR3SwitcherAMD64To32Bit_ %+ name
;%define SWITCHER_FIX_INTER_CR3_HC   FIX_INTER_AMD64_CR3
%define SWITCHER_FIX_INTER_CR3_GC   FIX_INTER_32BIT_CR3


;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "VMMSwitcher/AMD64andLegacy.mac"


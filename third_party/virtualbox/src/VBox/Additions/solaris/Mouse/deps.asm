; $Id: deps.asm $
;; @file
; Solaris kernel module dependency
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

%include "iprt/solaris/kmoddeps.mac"

kmoddeps_header  ; ELF header, section table and shared string table

kmoddeps_dynstr_start  ; ELF .dynstr section
kmoddeps_dynstr_string str_misc_ctf, "misc/ctf"
kmoddeps_dynstr_string str_drv_vboxguest, "drv/vboxguest"
kmoddeps_dynstr_end

kmoddeps_dynamic_start  ; ELF .dynamic section
kmoddeps_dynamic_needed str_misc_ctf
kmoddeps_dynamic_needed str_drv_vboxguest
kmoddeps_dynamic_end

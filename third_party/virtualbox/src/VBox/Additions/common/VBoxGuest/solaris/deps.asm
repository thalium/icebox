; $Id: deps.asm $
;; @file
; Solaris kernel module dependency
;

;
; Copyright (C) 2012-2016 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

%include "iprt/solaris/kmoddeps.mac"

kmoddeps_header  ; ELF header, section table and shared string table

kmoddeps_dynstr_start  ; ELF .dynstr section
kmoddeps_dynstr_string str_misc_ctf, "misc/ctf"
kmoddeps_dynstr_end

kmoddeps_dynamic_start  ; ELF .dynamic section
kmoddeps_dynamic_needed str_misc_ctf
kmoddeps_dynamic_end


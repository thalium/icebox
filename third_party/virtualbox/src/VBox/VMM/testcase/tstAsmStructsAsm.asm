; $Id: tstAsmStructsAsm.asm $
;; @file
; Assembly / C structure layout testcase.
;
; Make yasm/nasm create absolute symbols for the structure definition
; which we can parse and make code from using objdump and sed.
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

%ifdef RT_ARCH_AMD64
BITS 64
%endif

%include "CPUMInternal.mac"
%include "HMInternal.mac"
%include "TRPMInternal.mac"
%include "VMMInternal.mac"
%include "VBox/vmm/cpum.mac"
%include "VBox/vmm/vm.mac"
%include "VBox/vmm/hm_vmx.mac"
%include "VBox/sup.mac"
%include "VMMSwitcher.mac"
%ifdef DO_GLOBALS
 %include "tstAsmStructsAsm.mac"
%endif

.text
.data
.bss


; $Id: AMD64Stub.asm $
;; @file
; VMM - World Switchers, AMD64 Stub.
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
;*  Defined Constants And Macros                                               *
;*******************************************************************************
%define NAME_OVERLOAD(name)         vmmR3SwitcherAMD64Stub_ %+ name

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "VBox/err.mac"
%include "VMMSwitcher.mac"


BEGINCODE
GLOBALNAME Start

BITS 32

BEGINPROC vmmR0ToRawMode
    mov     eax, VERR_VMM_SWITCHER_STUB
    ret
ENDPROC vmmR0ToRawMode

BITS 32
BEGINPROC vmmRCCallTrampoline
.tight_loop:
    int3
    jmp     .tight_loop
ENDPROC vmmRCCallTrampoline

BEGINPROC vmmRCToHost
    mov     eax, VERR_VMM_SWITCHER_STUB
    ret
ENDPROC vmmRCToHost

BEGINPROC vmmRCToHostAsmNoReturn
    mov     eax, VERR_VMM_SWITCHER_STUB
    ret
ENDPROC vmmRCToHostAsmNoReturn

BEGINPROC vmmRCToHostAsm
    mov     eax, VERR_VMM_SWITCHER_STUB
    ret
ENDPROC   vmmRCToHostAsm

GLOBALNAME End

;
; The description string (in the text section).
;
NAME(Description):
    db "AMD64 Stub."
    db 0


;
; Dummy fixups.
;
BEGINDATA
GLOBALNAME Fixups
    db FIX_THE_END                      ; final entry.
GLOBALNAME FixupsEnd


;;
; The switcher definition structure.
ALIGNDATA(16)
GLOBALNAME Def
    istruc VMMSWITCHERDEF
        at VMMSWITCHERDEF.pvCode,                       RTCCPTR_DEF NAME(Start)
        at VMMSWITCHERDEF.pvFixups,                     RTCCPTR_DEF NAME(Fixups)
        at VMMSWITCHERDEF.pszDesc,                      RTCCPTR_DEF NAME(Description)
        at VMMSWITCHERDEF.pfnRelocate,                  RTCCPTR_DEF 0
        at VMMSWITCHERDEF.enmType,                      dd VMMSWITCHER_AMD64_STUB
        at VMMSWITCHERDEF.cbCode,                       dd NAME(End)                        - NAME(Start)
        at VMMSWITCHERDEF.offR0ToRawMode,               dd NAME(vmmR0ToRawMode)             - NAME(Start)
        at VMMSWITCHERDEF.offRCToHost,                  dd NAME(vmmRCToHost)                - NAME(Start)
        at VMMSWITCHERDEF.offRCCallTrampoline,          dd NAME(vmmRCCallTrampoline)        - NAME(Start)
        at VMMSWITCHERDEF.offRCToHostAsm,               dd NAME(vmmRCToHostAsm)             - NAME(Start)
        at VMMSWITCHERDEF.offRCToHostAsmNoReturn,       dd NAME(vmmRCToHostAsmNoReturn)     - NAME(Start)
        ; disasm help
        at VMMSWITCHERDEF.offHCCode0,                   dd 0
        at VMMSWITCHERDEF.cbHCCode0,                    dd NAME(vmmRCCallTrampoline)        - NAME(Start)
        at VMMSWITCHERDEF.offHCCode1,                   dd 0
        at VMMSWITCHERDEF.cbHCCode1,                    dd 0
        at VMMSWITCHERDEF.offIDCode0,                   dd 0
        at VMMSWITCHERDEF.cbIDCode0,                    dd 0
        at VMMSWITCHERDEF.offIDCode1,                   dd 0
        at VMMSWITCHERDEF.cbIDCode1,                    dd 0
        at VMMSWITCHERDEF.offGCCode,                    dd NAME(vmmRCCallTrampoline)        - NAME(Start)
        at VMMSWITCHERDEF.cbGCCode,                     dd NAME(End)                        - NAME(vmmRCCallTrampoline)

    iend


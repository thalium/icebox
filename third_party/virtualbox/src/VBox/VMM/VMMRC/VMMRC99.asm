; $Id: VMMRC99.asm $
;; @file
; VMMRC99 - The last object module in the link.
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

%include "VMMRC.mac"


;;
; End the Trap0b segment.
VMMR0_SEG Trap0b
GLOBALNAME g_aTrap0bHandlersEnd
    dd  0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0


;;
; End the Trap0d segment.
VMMR0_SEG Trap0d
GLOBALNAME g_aTrap0dHandlersEnd
    dd  0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0


;;
; End the Trap0e segment.
VMMR0_SEG Trap0e
GLOBALNAME g_aTrap0eHandlersEnd
    dd  0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0


;;
; End the patch helper segment
BEGIN_PATCH_HLP_SEG
EXPORTEDNAME g_PatchHlpEnd
    dd  0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0


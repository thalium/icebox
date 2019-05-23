; $Id: bs3-mode-NameShortLower.asm $
;; @file
; BS3Kit - g_szBs3ModeName_xxx
;

;
; Copyright (C) 2007-2017 Oracle Corporation
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

%include "bs3kit-template-header.mac"


BS3_BEGIN_DATA16
%undef  MY_MODE_NAME_STR
%defstr MY_MODE_NAME_STR TMPL_MODE_LNAME
BS3_GLOBAL_NAME_EX RT_CONCAT3(g_szBs3ModeNameShortLower, _, TMPL_MODE_LNAME), , %strlen(MY_MODE_NAME_STR)
BS3_GLOBAL_NAME_EX RT_CONCAT3(_g_szBs3ModeNameShortLower, _, TMPL_MODE_LNAME), , %strlen(MY_MODE_NAME_STR)
        db MY_MODE_NAME_STR, 0


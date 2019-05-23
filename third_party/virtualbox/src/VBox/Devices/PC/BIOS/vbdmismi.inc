; $Id: vbdmismi.inc $
;; @file
; DMI and SMBIOS definitions for VBox.
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

;; TODO: make dynamic

VBOX_DMI_TABLE_BASE	equ	0E1000h
VBOX_DMI_TABLE_VER	equ	25h
VBOX_DMI_TABLE_SIZE	equ	352

VBOX_SMBIOS_MAJOR_VER	equ	2
VBOX_SMBIOS_MINOR_VER	equ	5
VBOX_SMBIOS_MAXSS	equ	0FFh


;; @file
;; Initial system setup which needs to run in protected mode.
;;

;;
;; Copyright (C) 2004-2015 Oracle Corporation
;;
;; This file is part of VirtualBox Open Source Edition (OSE), as
;; available from http://www.virtualbox.org. This file is free software;
;; you can redistribute it and/or modify it under the terms of the GNU
;; General Public License (GPL) as published by the Free Software
;; Foundation, in version 2 as it comes in the "COPYING" file of the
;; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;;

SVR		equ	0FEE000F0h
LVT_LINT0	equ	0FEE00350h
LVT_LINT1	equ	0FEE00360h

public		pmode_setup

;; Enable the local APIC and program LINT0/LINT1 entries. Without that,
;; virtual wire interrupts could not be delivered. Note that the APIC must
;; be enabled first because when disabled, all LVTs are forced masked.

pmode_setup	proc	near

		.386
		push	eax
		push	esi
		pushf
		cli			; Interrupts would kill us!
		call	pmode_enter

		mov	eax, cr0	; Clear CR0.CD and CR0.NW
		and	eax, 09FFFFFFFh
		mov	cr0, eax

		mov	esi, SVR	; Program the SVR -- enable the APIC,
		mov	eax, 010Fh	; set spurious interrupt vector to 15
		mov	[esi], eax

		mov	esi, LVT_LINT0	; Program LINT0 to ExtINT and unmask
		mov	eax, [esi]
		and	eax, 0FFFE00FFh
		or	ah,  7
		mov	[esi], eax

		mov	esi, LVT_LINT1	; Program LINT1 to NMI and unmask
		mov	eax, [esi]
		and	eax, 0FFFE00FFh
		or	ah,  4
		mov	[esi], eax

		call	pmode_exit
		popf
		pop	esi
		pop	eax
		SET_DEFAULT_CPU_286
		ret

pmode_setup	endp

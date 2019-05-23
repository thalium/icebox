;; @file
;; Enter and exit a minimal protected-mode environment.
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

;; Caveats: May only be called from the F000 segment (16-bit). Does not
;; switch stacks. Must be run with disabled interrupts(!), any exceptions
;; will cause a crash. On return from pmode_enter, DS contains a selector
;; which can address the entire 4GB address space.

public		pmode_enter
public		pmode_exit
public		pmbios_gdt_desc
public		pmbios_gdt

pmode_enter	proc	near

		push	cs
		pop 	ds
		.386p
		lgdt	fword ptr [pmbios_gdt_desc]
		mov	eax, cr0
		or	al, 1
		mov	cr0, eax
;		jmp	far ptr 20h:really_enter_pm
		db	0EAh
		dw	really_enter_pm
		dw	20h
really_enter_pm:
		mov	ax, 18h
		mov	ds, ax
		ret

pmode_enter	endp


pmode_exit	proc	near

		mov	ax, 40h		; Ensure RM limit/attributes
		mov	ds, ax		; (where base = selector << 4)

		mov	eax, cr0
		and	al, 0FEh
		mov	cr0, eax
		SET_DEFAULT_CPU_286
		jmp	far ptr really_exit_pm
really_exit_pm:
		ret

pmode_exit	endp



pmbios_gdt_desc:
		dw	40h + 7                 ; last selector plus 8 - 1
		dw	pmbios_gdt
		dw	000Fh

pmbios_gdt:
		dw 0, 0, 0, 0
		dw 0, 0, 0, 0
		dw 0ffffh, 0, 9b00h, 00cfh	; 32-bit code (0x10)
		dw 0ffffh, 0, 9300h, 00cfh	; 32-bit data (0x18)
		dw 0ffffh, 0, 9b0fh, 0000h	; 16-bit code, base=0xf0000
		dw 0ffffh, 0, 9300h, 0000h	; 16-bit data, base=0x0
		dw 0, 0, 0, 0
		dw 0, 0, 0, 0
		dw 0ffffh, 400h, 9300h, 0000h   ; 16-bit data, base=0x40

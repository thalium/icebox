; $Id: bootsector-shutdown.asm $
;; @file
; Bootsector for grub chainloading that shutdowns the VM.
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

%include "VBox/bios.mac"


BITS 16
start:
    ; Start with a jump just to follow the convention.
    jmp short the_code
    nop
times 3ah db 0

the_code:
    cli

    ;
    ; VBox/Bochs shutdown request - write "Shutdown" byte by byte to shutdown port.
    ;
    mov cx, 64
    mov dx, VBOX_BIOS_SHUTDOWN_PORT
    mov bx, VBOX_BIOS_OLD_SHUTDOWN_PORT
retry:
    mov al, 'S'
    out dx, al
    mov al, 'h'
    out dx, al
    mov al, 'u'
    out dx, al
    mov al, 't'
    out dx, al
    mov al, 'd'
    out dx, al
    mov al, 'o'
    out dx, al
    mov al, 'w'
    out dx, al
    mov al, 'n'
    out dx, al
    xchg dx, bx                         ; alternate between the new (VBox) and old (Bochs) ports.
    loop retry

    ;
    ; Shutdown failed!
    ;

    ;; @todo print some message before halting.
    hlt

    ;
    ; Padd the remainder of the sector with zeros and
    ; end it with the dos signature.
    ;
padding:
times 510 - (padding - start) db 0
    db 055h, 0aah


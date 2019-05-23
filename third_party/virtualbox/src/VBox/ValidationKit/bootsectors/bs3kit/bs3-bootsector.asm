; $Id: bs3-bootsector.asm $
;; @file
; Generic bootsector for BS3.
;
; This sets up stack at %fff0 and loads the next sectors from the floppy at
; %10000 (1000:0000 in real mode), then starts executing at cs:ip=1000:0000.
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



;*********************************************************************************************************************************
;*      Header Files                                                                                                             *
;*********************************************************************************************************************************
%include "bs3kit.mac"
%include "iprt/asmdefs.mac"
%include "iprt/x86.mac"


%ifdef __YASM__
[map all]
%endif

;
; Start with a jump just to follow the convention.
; Also declare all segments/sections to establish them and their order.
;
        ORG 07c00h

BITS 16
CPU 8086
start:
        jmp short bs3InitCode
        db 0ah                          ; Should be nop, but this looks better.
g_OemId:                                ; 003h
        db 'BS3Kit', 0ah, 0ah

;
; DOS 4.0 Extended Bios Parameter Block:
;
g_cBytesPerSector:                      ; 00bh
        dw 512
g_cSectorsPerCluster:                   ; 00dh
        db 1
g_cReservedSectors:                     ; 00eh
        dw 1
g_cFATs:                                ; 010h
        db 0
g_cRootDirEntries:                      ; 011h
        dw 0
g_cTotalSectors:                        ; 013h
        dw 0
g_bMediaDescriptor:                     ; 015h
        db 0
g_cSectorsPerFAT:                       ; 016h
        dw 0
g_cPhysSectorsPerTrack:                 ; 018h
        dw 18
g_cHeads:                               ; 01ah
        dw 2
g_cHiddentSectors:                      ; 01ch
        dd 1
g_cLargeTotalSectors:                   ; 020h - We (ab)use this to indicate the number of sectors to load.
        dd 0
g_bBootDrv:                             ; 024h
        db 80h
g_bFlagsEtc:                            ; 025h
        db 0
g_bExtendedSignature:                   ; 026h
        db 0x29
g_dwSerialNumber:                       ; 027h
        dd 0x0a458634
g_abLabel:                              ; 02bh
        db 'VirtualBox', 0ah
g_abFSType:                             ; 036h
        db 'RawCode', 0ah
g_BpbEnd:                               ; 03ch


;
; Where to real init code starts.
;
bs3InitCode:
        cli

        ; save the registers.
        mov     [cs:BS3_ADDR_REG_SAVE + BS3REGCTX.rax], ax
        mov     [cs:BS3_ADDR_REG_SAVE + BS3REGCTX.rsp], sp
        mov     [cs:BS3_ADDR_REG_SAVE + BS3REGCTX.ss], ss
        mov     [cs:BS3_ADDR_REG_SAVE + BS3REGCTX.ds], ds

        ; set up the segment reisters and stack.
        mov     ax, 0
        mov     ds, ax
        mov     ss, ax
        mov     sp, BS3_ADDR_STACK

        ; Save more registers, without needing cs prefix.
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rcx], cx
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rdi], di
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.es], es
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rbp], bp

        ; Load es and setup bp frame.
        mov     es, ax
        mov     bp, sp
        mov     [bp], ax                ; clear the first 8 bytes (terminates the ebp chain)
        mov     [bp + 02h], ax
        mov     [bp + 04h], ax
        mov     [bp + 06h], ax

        ; Save flags now that we know that there's a valid stack.
        pushf

        ;
        ; Clear the register area.
        ;
        mov     di, BS3_ADDR_REG_SAVE
        mov     cx, BS3REGCTX_size/2
        cld
        rep stosw

        ;
        ; Do basic CPU detection.
        ;

        ; 1. bit 15-bit was fixed to 1 in pre-286 CPUs, and fixed to 0 in 286+.
        mov     ax, [bp - 2]
        test    ah, 080h                ; always set on pre 286, clear on 286 and later
        jnz     .pre_80286

        ; 2. On a 286 you cannot popf IOPL and NT from real mode.
.detect_286_or_386plus:
CPU 286
        mov     ah, (X86_EFL_IOPL | X86_EFL_NT) >> 8
        push    ax
        popf
        pushf
        cmp     ah, [bp - 3]
        pop     ax
        je      .is_386plus
.is_80286:
CPU 286
        smsw    [BS3_ADDR_REG_SAVE + BS3REGCTX.cr0]
.pre_80286:
CPU 8086
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rbx], bx
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rdx], dx
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rsi], si
        jmp     .do_load

        ; Save 386 registers. We can now skip the CS prefix as DS is flat.
CPU 386
.is_386plus:
        shr     eax, 16
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rax+2], ax
        mov     eax, esp
        shr     eax, 16
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rsp+2], ax
        mov     eax, ebp
        shr     eax, 16
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rbp+2], ax
        shr     edi, 16
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rdi+2], di
        shr     ecx, 16
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rcx+2], cx
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.fs], fs
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.gs], gs
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rbx], ebx
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rdx], edx
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rsi], esi
        mov     eax, cr2
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.cr2], eax
        mov     eax, cr3
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.cr3], eax
        mov     byte [BS3_ADDR_REG_SAVE + BS3REGCTX.bMode], BS3_MODE_RM
        mov     [cs:BS3_ADDR_REG_SAVE + BS3REGCTX.cs], cs
        xor     eax, eax
        mov     ax, start
        mov     [cs:BS3_ADDR_REG_SAVE + BS3REGCTX.rip], eax

        ; Pentium/486+: CR4 requires VME/CPUID, so we need to detect that before accessing it.
        mov     [cs:BS3_ADDR_REG_SAVE + BS3REGCTX.cr4], eax
        popf                            ; (restores IOPL+NT)
        pushfd
        pop     eax
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.rflags], eax
        xor     eax, X86_EFL_ID
        push    eax
        popfd
        pushfd
        pop     ebx
        cmp     ebx, eax
        jne     .no_cr4
        mov     eax, cr4
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.cr4], eax
.no_cr4:
        ; Make sure caching is enabled and alignment is off.
        mov     eax, cr0
        mov     [BS3_ADDR_REG_SAVE + BS3REGCTX.cr0], eax
        and     eax, ~(X86_CR0_NW | X86_CR0_CD | X86_CR0_AM)
        mov     cr0, eax

        ; Load all the code.
.do_load
        mov     [g_bBootDrv], dl
        call    NAME(bs3InitLoadImage)
%if 0
        mov     al, '='
        call    NAME(bs3PrintChrInAl)
%endif

        ;
        ; Call the user 'main' procedure (shouldn't return).
        ;
        cld
        call    BS3_SEL_TEXT16:0000h

        ; Panic/hang.
Bs3Panic:
        cli
        jmp     Bs3Panic


;; For debug and error handling.
; @uses ax
bs3PrintHexInAl:
CPU 286
        push    ax
        shr     al, 4
        call    bs3PrintHexDigitInAl
        pop     ax
bs3PrintHexDigitInAl:
        and     al, 0fh
        cmp     al, 10
        jb      .decimal
        add     al, 'a' - '0' - 10
.decimal:
        add     al, '0'
bs3PrintChrInAl:
        push    bx
        mov     ah, 0eh
        mov     bx, 0ff00h
        int     10h
        pop     bx
        ret


;;
; Loads the image off the floppy.
;
; This uses g_cLargeTotalSectors to figure out how much to load.
;
; Clobbers everything except ebp and esp.  Panics on failure.
;
; @param    dl          The boot drive number (from BIOS).
; @uses     ax, cx, bx, esi, di
;
BEGINPROC bs3InitLoadImage
        push    bp
        mov     bp, sp
        push    es
%define bSavedDiskNo    byte [bp - 04h]
        push    dx
%define bMaxSector      byte [bp - 06h]
        xor     ax, ax
        push    ax
%define bMaxHead        byte [bp - 08h]
        push    ax
%define bMaxCylinder    byte [bp - 0ah]
        push    ax

        ;
        ; Try figure the geometry.
        ;
        mov     ah, 08h
        int     13h
        jc      .failure
        mov     bMaxSector, cl
        mov     bMaxHead, dh
        mov     bMaxCylinder, ch
        mov     dl, bSavedDiskNo

        ;
        ; Reload all the sectors one at a time (avoids problems).
        ;
        mov     si, [g_cLargeTotalSectors] ; 16-bit sector count ==> max 512 * 65 535 = 33 553 920 bytes.
        dec     si
        mov     di, BS3_ADDR_LOAD / 16  ; The current load segment.
        mov     cx, 0002h               ; ch/cylinder=0 (0-based); cl/sector=2 (1-based)
        xor     dh, dh                  ; dh/head=0
.the_load_loop:
%if 0
        mov al, 'c'
        call bs3PrintChrInAl
        mov al, ch
        call bs3PrintHexInAl
        mov al, 's'
        call bs3PrintChrInAl
        mov al, cl
        call bs3PrintHexInAl
        mov al, 'h'
        call bs3PrintChrInAl
        mov al, dh
        call bs3PrintHexInAl
        mov al, ';'
        call bs3PrintChrInAl
%endif
        xor     bx, bx
        mov     es, di                  ; es:bx -> buffer
        mov     ax, 0201h               ; al=1 sector; ah=read function
        int     13h
        jc      .failure

        ; advance to the next sector/head/cylinder.
        inc     cl
        cmp     cl, bMaxSector
        jbe     .adv_addr

        mov     cl, 1
        inc     dh
        cmp     dh, bMaxHead
        jbe     .adv_addr

        mov     dh, 0
        inc     ch

.adv_addr:
        add     di, 512 / 16
        dec     si
        jnz     .the_load_loop
%if 0
        mov     al, 'D'
        call bs3PrintChrInAl
%endif

        add     sp, 3*2
        pop     dx
        pop     es
        pop     bp
        ret

        ;
        ; Something went wrong, display a message.
        ;
.failure:
        push    ax

        ; print message
        mov     si, .s_szErrMsg
.failure_next_char:
        lodsb
        call    bs3PrintChrInAl
        cmp     si, .s_szErrMsgEnd
        jb      .failure_next_char

        ; panic
        pop     ax
%if 1
        mov     al, ah
        push    bs3PrintHexInAl
%endif
        call    Bs3Panic
.s_szErrMsg:
        db 13, 10, 'rd err! '
.s_szErrMsgEnd:
;ENDPROC bs3InitLoadImage - don't want the padding.


;
; Pad the remainder of the sector with int3's and end it with the DOS signature.
;
bs3Padding:
        times ( 510 - ( (bs3Padding - start) % 512 ) ) db 0cch
        db      055h, 0aah


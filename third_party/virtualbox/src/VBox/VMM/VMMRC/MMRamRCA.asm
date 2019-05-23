; $Id: MMRamRCA.asm $
;; @file
; MMRamGCA - Guest Context Ram access Assembly Routines.
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
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "VBox/err.mac"
%include "iprt/err.mac"
%include "iprt/x86.mac"


BEGINCODE


;;
; Read data in guest context, CDECL calling conv.
; VMMRCDECL(int) MMGCRamRead(void *pDst, void *pSrc, size_t cb);
; MMRamGC page fault handler must be installed prior this call for safe operation.
;
; @returns eax=0 if data read, other code - invalid access, #PF was generated.
; @param    [esp + 04h]    Param 1 - Pointer where to store result data (pDst).
; @param    [esp + 08h]    Param 2 - Pointer of data to read (pSrc).
; @param    [esp + 0ch]    Param 3 - Size of data to read, only 1/2/4/8 is valid.
; @uses     eax, ecx, edx
;
; @remark   Data is saved to destination (Param 1) even if read error occurred!
;
align 16
BEGINPROC   MMGCRamReadNoTrapHandler
    mov     eax, [esp + 0ch]            ; eax = size of data to read
    cmp     eax, byte 8                 ; yes, it's slow, validate input
    ja      ramread_InvalidSize
    mov     edx, [esp + 04h]            ; edx = result address
    mov     ecx, [esp + 08h]            ; ecx = data address
    jmp     [ramread_table + eax*4]

ramread_byte:
    xor     eax, eax                    ; rc = VINF_SUCCESS by default
    mov     cl, [ecx]                   ; read data
    mov     [edx], cl                   ; save data
    ret

ramread_word:
    xor     eax, eax                    ; rc = VINF_SUCCESS by default
    mov     cx, [ecx]                   ; read data
    mov     [edx], cx                   ; save data
    ret

ramread_dword:
    xor     eax, eax                    ; rc = VINF_SUCCESS by default
    mov     ecx, [ecx]                  ; read data
    mov     [edx], ecx                  ; save data
    ret

ramread_qword:
    mov     eax, [ecx]                  ; read data
    mov     [edx], eax                  ; save data
    mov     eax, [ecx+4]                ; read data
    mov     [edx+4], eax                ; save data
    xor     eax, eax                    ; rc = VINF_SUCCESS by default
    ret

; Read error - we will be here after our page fault handler.
GLOBALNAME MMGCRamRead_Error
    mov     eax, VERR_ACCESS_DENIED
    ret

; Invalid data size
ramread_InvalidSize:
    mov     eax, VERR_INVALID_PARAMETER
    ret

; Jump table
ramread_table:
    DD      ramread_InvalidSize
    DD      ramread_byte
    DD      ramread_word
    DD      ramread_InvalidSize
    DD      ramread_dword
    DD      ramread_InvalidSize
    DD      ramread_InvalidSize
    DD      ramread_InvalidSize
    DD      ramread_qword
ENDPROC     MMGCRamReadNoTrapHandler


;;
; Write data in guest context, CDECL calling conv.
; VMMRCDECL(int) MMGCRamWrite(void *pDst, void *pSrc, size_t cb);
;
; @returns eax=0 if data written, other code - invalid access, #PF was generated.
; @param    [esp + 04h]    Param 1 - Pointer where to write data (pDst).
; @param    [esp + 08h]    Param 2 - Pointer of data to write (pSrc).
; @param    [esp + 0ch]    Param 3 - Size of data to write, only 1/2/4 is valid.
; @uses     eax, ecx, edx
;
align 16
BEGINPROC   MMGCRamWriteNoTrapHandler
    mov     eax, [esp + 0ch]            ; eax = size of data to write
    cmp     eax, byte 4                 ; yes, it's slow, validate input
    ja      ramwrite_InvalidSize
    mov     edx, [esp + 04h]            ; edx = write address
    mov     ecx, [esp + 08h]            ; ecx = data address
    jmp     [ramwrite_table + eax*4]

ramwrite_byte:
    xor     eax, eax                    ; rc = VINF_SUCCESS by default
    mov     cl, [ecx]                   ; read data
    mov     [edx], cl                   ; write data
    ret

ramwrite_word:
    xor     eax, eax                    ; rc = VINF_SUCCESS by default
    mov     cx, [ecx]                   ; read data
    mov     [edx], cx                   ; write data
    ret

ramwrite_dword:
    xor     eax, eax                    ; rc = VINF_SUCCESS by default
    mov     ecx, [ecx]                  ; read data
    mov     [edx], ecx                  ; write data
    ret

; Write error - we will be here after our page fault handler.
GLOBALNAME MMGCRamWrite_Error
    mov     eax, VERR_ACCESS_DENIED
    ret

; Invalid data size
ramwrite_InvalidSize:
    mov     eax, VERR_INVALID_PARAMETER
    ret

; Jump table
ramwrite_table:
    DD      ramwrite_InvalidSize
    DD      ramwrite_byte
    DD      ramwrite_word
    DD      ramwrite_InvalidSize
    DD      ramwrite_dword
ENDPROC     MMGCRamWriteNoTrapHandler


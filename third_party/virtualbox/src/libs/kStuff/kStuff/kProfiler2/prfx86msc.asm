; $Id: prfx86msc.asm 29 2009-07-01 20:30:29Z bird $
;; @file
; kProfiler Mark 2 - Microsoft C/C++ Compiler Interaction, x86.
;

;
; Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
;
; Permission is hereby granted, free of charge, to any person
; obtaining a copy of this software and associated documentation
; files (the "Software"), to deal in the Software without
; restriction, including without limitation the rights to use,
; copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following
; conditions:
;
; The above copyright notice and this permission notice shall be
; included in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
; OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
; HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
; WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
; OTHER DEALINGS IN THE SOFTWARE.
;

[section .data]
;
g_fCalibrated:
        dd 0
g_OverheadAdj:
        dd 0

[section .text]

extern KPRF_ENTER
extern KPRF_LEAVE

global __penter
global __pexit

;ifdef  UNDEFINED
global common_return_path
global common_overhead
global common_no_overhead
global calibrate
global calib_inner_update_minimum
global calib_inner_next
global calib_outer_dec
global calib_outer_inc
global calib_done
global calib_nullproc
;endif


;;
; On x86 the call to this function has been observed to be put before
; creating the stack frame, as the very first instruction in the function.
;
; Thus the stack layout is as follows:
;       24      return address of the calling function.
;       20      our return address - the address of the calling function + 5.
;       1c      eax
;       18      edx
;       14      eflags
;       10      ecx
;       c       tsc high       - param 3
;       8       tsc low
;       4       frame pointer  - param 2
;       0       function ptr   - param 1
;
;
align 16
__penter:
        ; save volatile register and get the time stamp.
        push    eax
        push    edx
        rdtsc
        pushfd
        push    ecx

        ; setting up the enter call frame (cdecl).
        sub     esp, 4 + 4 + 8
        mov     [esp + 0ch], edx        ; Param 3 - the timestamp
        mov     [esp + 08h], eax
        lea     edx, [esp + 24h]        ; Param 2 - frame pointer (pointer to the return address of the function calling us)
        mov     [esp + 04h], edx
        mov     eax, [esp + 20h]        ; Param 1 - The function address
        sub     eax, 5                  ; call instruction
        mov     [esp], eax

        call    KPRF_ENTER
        jmp     common_return_path


;;
; On x86 the call to this function has been observed to be put right before
; return instruction. This fact matters since since we have to calc the same
; stack address as in _penter.
;
; Thus the stack layout is as follows:
;       24      return address of the calling function.
;       20      our return address - the address of the calling function + 5.
;       1c      eax
;       18      edx
;       14      eflags
;       10      ecx
;       c       tsc high       - param 3
;       8       tsc low
;       4       frame pointer  - param 2
;       0       function ptr   - param 1
;
;
align 16
__pexit:
        ; save volatile register and get the time stamp.
        push    eax
        push    edx
        rdtsc
        pushfd
        push    ecx

        ; setting up the leave call frame (cdecl).
        sub     esp, 4 + 4 + 8
        mov     [esp + 0ch], edx        ; Param 3 - the timestamp
        mov     [esp + 08h], eax
        lea     edx, [esp + 24h]        ; Param 2 - frame pointer (pointer to the return address of the function calling us)
        mov     [esp + 04h], edx
        mov     eax, [esp + 20h]        ; Param 1 - Some address in the function.
        sub     eax, 5                  ; call instruction
        mov     [esp], eax

        call    KPRF_LEAVE
        jmp common_return_path


;;
; This is the common return path for both the enter and exit hooks.
; It's kept common because we can then use the same overhead adjustment
; and save some calibration efforts. It also saves space :-)
align 16
common_return_path:
        ; Update overhead
        test    eax, eax
        jz      common_no_overhead
        cmp     byte [g_fCalibrated], 0
        jnz     common_overhead
        call    calibrate
common_overhead:
        mov     ecx, eax                ; ecx <- pointer to overhead counter.
        mov     eax, [g_OverheadAdj]    ; apply the adjustment before reading tsc
        sub     [esp + 08h], eax
        sbb     dword [esp + 0ch], 0

        rdtsc
        sub     eax, [esp + 08h]
        sbb     edx, [esp + 0ch]
        add     [ecx], eax
        adc     [ecx + 4], edx
common_no_overhead:
        add     esp, 4 + 4 + 8

        ; restore volatile registers.
        pop     ecx
        popfd
        pop     edx
        pop     eax
        ret

;;
; Data esi points to while we're calibrating.
struc CALIBDATA
    .OverheadLo resd 1
    .OverheadHi resd 1
    .ProfiledLo resd 1
    .ProfiledHi resd 1
    .EnterTSLo  resd 1
    .EnterTSHi  resd 1
    .MinLo      resd 1
    .MinHi      resd 1
endstruc



align 16
;;
; Do necessary calibrations.
;
calibrate:
        ; prolog
        push    ebp
        mov     ebp, esp
        pushfd
        pushad
        sub     esp, CALIBDATA_size
        mov     esi, esp                ; esi points to the CALIBDATA

        ;
        ; Indicate that we have finished calibrating.
        ;
        mov     eax, 1
        xchg    dword [g_fCalibrated], eax

        ;
        ; The outer loop - find the right adjustment.
        ;
        mov     ebx, 200h               ; loop counter.
calib_outer_loop:

        ;
        ; The inner loop - calls the function number of times to establish a
        ;                  good minimum value
        ;
        mov     ecx, 200h
        mov     dword [esi + CALIBDATA.MinLo], 0ffffffffh
        mov     dword [esi + CALIBDATA.MinHi], 07fffffffh
calib_inner_loop:

        ; zero the overhead and profiled times.
        xor     eax, eax
        mov     [esi + CALIBDATA.OverheadLo], eax
        mov     [esi + CALIBDATA.OverheadHi], eax
        mov     [esi + CALIBDATA.ProfiledLo], eax
        mov     [esi + CALIBDATA.ProfiledHi], eax
        call    calib_nullproc

        ; subtract the overhead
        mov     eax, [esi + CALIBDATA.ProfiledLo]
        mov     edx, [esi + CALIBDATA.ProfiledHi]
        sub     eax, [esi + CALIBDATA.OverheadLo]
        sbb     edx, [esi + CALIBDATA.OverheadHi]

        ; update the minimum value.
        test    edx, 080000000h
        jnz near calib_outer_dec        ; if negative, just simplify and shortcut
        cmp     edx, [esi + CALIBDATA.MinHi]
        jg      calib_inner_next
        jl      calib_inner_update_minimum
        cmp     eax, [esi + CALIBDATA.MinLo]
        jge     calib_inner_next
calib_inner_update_minimum:
        mov     [esi + CALIBDATA.MinLo], eax
        mov     [esi + CALIBDATA.MinHi], edx
calib_inner_next:
        loop    calib_inner_loop

        ; Is the minimum value acceptable?
        test    dword [esi + CALIBDATA.MinHi], 80000000h
        jnz     calib_outer_dec         ; simplify if negative.
        cmp     dword [esi + CALIBDATA.MinHi], 0
        jnz     calib_outer_inc         ; this shouldn't be possible
        cmp     dword [esi + CALIBDATA.MinLo], 1fh
        jbe     calib_outer_dec         ; too low - 2 ticks per pair is the minimum!
        cmp     dword [esi + CALIBDATA.MinLo], 30h
        jbe     calib_done              ; this is fine!
calib_outer_inc:
        inc     dword [g_OverheadAdj]
        jmp     calib_outer_next
calib_outer_dec:
        cmp     dword [g_OverheadAdj], 1
        je      calib_done
        dec     dword [g_OverheadAdj]
calib_outer_next:
        dec     ebx
        jnz     calib_outer_loop
calib_done:

        ; epilog
        add     esp, CALIBDATA_size
        popad
        popfd
        leave
        ret




;;
; The calibration __penter - this must be identical to the real thing except for the KPRF call.
align 16
calib_penter:
        ; This part must be identical
        push    eax
        push    edx
        rdtsc
        pushfd
        push    ecx

        ; store the entry
        mov     [esi + CALIBDATA.EnterTSLo], eax
        mov     [esi + CALIBDATA.EnterTSHi], edx

        ; create the call frame
        push    edx
        push    eax
        push    0
        push    0

        lea     eax, [esi + CALIBDATA.OverheadLo]
        jmp     common_overhead


;;
; The calibration __pexit - this must be identical to the real thing except for the KPRF call.
align 16
calib_pexit:
        ; This part must be identical
        push    eax
        push    edx
        rdtsc
        pushfd
        push    ecx

        ; update the time
        push    eax
        push    edx
        sub     eax, [esi + CALIBDATA.EnterTSLo]
        sbb     edx, [esi + CALIBDATA.EnterTSHi]
        add     [esi + CALIBDATA.ProfiledLo], eax
        adc     [esi + CALIBDATA.ProfiledHi], edx
        pop     edx
        pop     eax

        ; create the call frame
        push    edx
        push    eax
        push    0
        push    0

        lea     eax, [esi + CALIBDATA.EnterTSLo]
        jmp     common_overhead


;;
; The 'function' we're profiling.
; The general idea is that each pair should take something like 2-10 ticks.
;
; (Btw. If we don't use multiple pairs here, we end up with the wrong result.)
align 16
calib_nullproc:
        call    calib_penter ;0
        call    calib_pexit

        call    calib_penter ;1
        call    calib_pexit

        call    calib_penter ;2
        call    calib_pexit

        call    calib_penter ;3
        call    calib_pexit

        call    calib_penter ;4
        call    calib_pexit

        call    calib_penter ;5
        call    calib_pexit

        call    calib_penter ;6
        call    calib_pexit

        call    calib_penter ;7
        call    calib_pexit

        call    calib_penter ;8
        call    calib_pexit

        call    calib_penter ;9
        call    calib_pexit

        call    calib_penter ;a
        call    calib_pexit

        call    calib_penter ;b
        call    calib_pexit

        call    calib_penter ;c
        call    calib_pexit

        call    calib_penter ;d
        call    calib_pexit

        call    calib_penter ;e
        call    calib_pexit

        call    calib_penter ;f
        call    calib_pexit
        ret


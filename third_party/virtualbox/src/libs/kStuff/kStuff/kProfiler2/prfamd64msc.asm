; $Id: prfamd64msc.asm 29 2009-07-01 20:30:29Z bird $;
;; @file
; kProfiler Mark 2 - Microsoft C/C++ Compiler Interaction, AMD64.
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

global _penter
global _pexit

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
_penter:
        ; save volatile register and get the time stamp.
        push    rax
        push    rdx
        rdtsc
        pushfq
        push    rcx
        push    r8
        push    r9
        push    r10
        push    r11
        sub     rsp, 28h                ; rsp is unaligned at this point (8 pushes).
                                        ; reserve 20h for spill, and 8 bytes for ts.

        ; setting up the enter call frame
        mov     r8d, edx
        shl     r8, 32
        or      r8, rax                 ; param 3 - the timestamp
        mov     [rsp + 20h], r8         ; save the tsc for later use.
        lea     rdx, [rsp + 8*8 + 28h]  ; Param 2 - default frame pointer
        mov     rcx, [rdx]              ; Param 1 - The function address

        ; MSC seems to put the _penter both before and after the typical sub rsp, xxh
        ; statement as if it cannot quite make up its mind. We'll try adjust for this
        ; to make the unwinding a bit more accurate wrt to longjmp/throw. But since
        ; there are also an uneven amount of push/pop around the _penter/_pexit we
        ; can never really make a perfect job of it. sigh.
        cmp     word [rcx - 5 - 4], 08348h  ; sub rsp, imm8
        jne     .not_byte_sub
        cmp     byte [rcx - 5 - 2], 0ech
        jne     .not_byte_sub
        movzx   eax, byte [rcx - 5 - 1]     ; imm8
        add     rdx, rax
        jmp     .call_prf_enter
.not_byte_sub:
        cmp     word [rcx - 5 - 7], 08148h  ; sub rsp, imm32
        jne     .not_dword_sub
        cmp     byte [rcx - 5 - 5], 0ech
        jne     .not_dword_sub
        mov     eax, [rcx - 5 - 4]          ; imm32
        add     rdx, rax
;        jmp     .call_prf_enter
.not_dword_sub:
.call_prf_enter:
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
_pexit:
        ; save volatile register and get the time stamp.
        push    rax
        push    rdx
        rdtsc
        pushfq
        push    rcx
        push    r8
        push    r9
        push    r10
        push    r11
        sub     rsp, 28h                ; rsp is unaligned at this point (8 pushes).
                                        ; reserve 20h for spill, and 8 bytes for ts.

        ; setting up the enter call frame
        mov     r8d, edx
        shl     r8, 32
        or      r8, rax                 ; param 3 - the timestamp
        mov     [rsp + 20h], r8         ; save the tsc for later use.
        lea     rdx, [rsp + 8*8 + 28h]  ; Param 2 - frame pointer.
        mov     rcx, [rdx]              ; Param 1 - The function address

        ; MSC some times put the _pexit before the add rsp, xxh. To try match up with
        ; any adjustments made in _penter, we'll try detect this.
        cmp     word [rcx], 08348h      ; add rsp, imm8
        jne     .not_byte_sub
        cmp     byte [rcx + 2], 0c4h
        jne     .not_byte_sub
        movzx   eax, byte [rcx + 3]     ; imm8
        add     rdx, rax
        jmp     .call_prf_leave
.not_byte_sub:
        cmp     word [rcx], 08148h      ; add rsp, imm32
        jne     .not_dword_sub
        cmp     byte [rcx + 2], 0c4h
        jne     .not_dword_sub
        mov     eax, [rcx + 3]          ; imm32
        add     rdx, rax
;        jmp     .call_prf_leave
.not_dword_sub:
.call_prf_leave:
        call    KPRF_LEAVE
        jmp common_return_path


;;
; This is the common return path for both the enter and exit hooks.
; It's kept common because we can then use the same overhead adjustment
; and save some calibration efforts. It also saves space :-)
align 16
common_return_path:
        ; Update overhead
        test    rax, rax
        jz      common_no_overhead
        cmp     byte [g_fCalibrated wrt rip], 0
        jnz     common_overhead
        call    calibrate
common_overhead:
        mov     rcx, rax                ; rcx <- pointer to overhead counter.
        mov     eax, [g_OverheadAdj wrt rip]; apply the adjustment before reading tsc
        sub     [rsp + 20h], rax

        rdtsc
        shl     rdx, 32
        or      rdx, rax                ; rdx = 64-bit timestamp
        sub     rdx, [rsp + 20h]        ; rdx = elapsed
        lock add [rcx], rdx             ; update counter.
common_no_overhead:

        ; restore volatile registers.
        add     rsp, 28h
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rcx
        popfq
        pop     rdx
        pop     rax
        ret

;;
; Data rsi points to while we're calibrating.
struc CALIBDATA
    .Overhead   resq 1
    .Profiled   resq 1
    .EnterTS    resq 1
    .Min        resq 1
endstruc



align 16
;;
; Do necessary calibrations.
;
calibrate:
        ; prolog - save everything
        push    rbp
        pushfq
        push    rax                     ; pushaq
        push    rbx
        push    rcx
        push    rdx
        push    rdi
        push    rsi
        push    r8
        push    r9
        push    r10
        push    r11
        push    r12
        push    r13
        push    r14
        push    r15
        mov     rbp, rsp

        sub     rsp, CALIBDATA_size
        mov     rsi, rsp                ; rsi points to the CALIBDATA

        and     rsp, -16

        ;
        ; Indicate that we have finished calibrating.
        ;
        mov     eax, 1
        xchg    dword [g_fCalibrated wrt rip], eax

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
        mov     dword [rsi + CALIBDATA.Min], 0ffffffffh
        mov     dword [rsi + CALIBDATA.Min + 4], 07fffffffh
calib_inner_loop:

        ; zero the overhead and profiled times.
        xor     eax, eax
        mov     [rsi + CALIBDATA.Overhead], rax
        mov     [rsi + CALIBDATA.Profiled], rax
        call    calib_nullproc

        ; subtract the overhead
        mov     rax, [rsi + CALIBDATA.Profiled]
        sub     rax, [rsi + CALIBDATA.Overhead]

        ; update the minimum value.
        bt      rax, 63
        jc near calib_outer_dec        ; if negative, just simplify and shortcut
        cmp     rax, [rsi + CALIBDATA.Min]
        jge     calib_inner_next
calib_inner_update_minimum:
        mov     [rsi + CALIBDATA.Min], rax
calib_inner_next:
        loop    calib_inner_loop

        ; Is the minimum value acceptable?
        test    dword [rsi + CALIBDATA.Min + 4], 80000000h
        jnz     calib_outer_dec         ; simplify if negative.
        cmp     dword [rsi + CALIBDATA.Min + 4], 0
        jnz     calib_outer_inc         ; this shouldn't be possible
        cmp     dword [rsi + CALIBDATA.Min], 1fh
        jbe     calib_outer_dec         ; too low - 2 ticks per pair is the minimum!
        ;cmp     dword [rsi + CALIBDATA.Min], 30h
        ;jbe     calib_done              ; this is fine!
        cmp     dword [rsi + CALIBDATA.Min], 70h ; - a bit weird...
        jbe     calib_outer_next         ; do the full 200h*200h iteration
calib_outer_inc:
        inc     dword [g_OverheadAdj wrt rip]
        jmp     calib_outer_next
calib_outer_dec:
        cmp     dword [g_OverheadAdj wrt rip], 1
        je      calib_done
        dec     dword [g_OverheadAdj wrt rip]
calib_outer_next:
        dec     ebx
        jnz     calib_outer_loop
calib_done:

        ; epilog - restore it all.
        mov     rsp, rbp
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rsi
        pop     rdi
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        popfq
        pop     rbp
        ret




;;
; The calibration _penter - this must be identical to the real thing except for the KPRF call.
align 16
calib_penter:
        ; This part must be identical past the rdtsc.
        push    rax
        push    rdx
        rdtsc
        pushfq
        push    rcx
        push    r8
        push    r9
        push    r10
        push    r11
        sub     rsp, 28h                ; rsp is unaligned at this point (8 pushes).
                                        ; reserve 20h for spill, and 8 bytes for ts.

        ; store the entry / stack frame.
        mov     r8d, edx
        shl     r8, 32
        or      r8, rax
        mov     [rsp + 20h], r8

        mov     [rsi + CALIBDATA.EnterTS], r8

        lea     rax, [rsi + CALIBDATA.Overhead]
        jmp     common_overhead


;;
; The calibration _pexit - this must be identical to the real thing except for the KPRF call.
align 16
calib_pexit:
        ; This part must be identical past the rdtsc.
        push    rax
        push    rdx
        rdtsc
        pushfq
        push    rcx
        push    r8
        push    r9
        push    r10
        push    r11
        sub     rsp, 28h                ; rsp is unaligned at this point (8 pushes).
                                        ; reserve 20h for spill, and 8 bytes for ts.

        ; store the entry / stack frame.
        mov     r8d, edx
        shl     r8, 32
        or      r8, rax
        mov     [rsp + 20h], r8

        sub     r8, [rsi + CALIBDATA.EnterTS]
        add     [rsi + CALIBDATA.Profiled], r8

        lea     rax, [rsi + CALIBDATA.EnterTS]
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


;
; Dummy stack check function.
;
global __chkstk
__chkstk:
    ret

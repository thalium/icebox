; $Id: kLdrA-os2.asm 29 2009-07-01 20:30:29Z bird $
;; @file
; kLdr - The Dynamic Loader, OS/2 Assembly Helpers.
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

segment TEXT32 public align=16 CLASS=CODE use32

;
; _DLL_InitTerm
;
..start:
extern _DLL_InitTerm
    jmp _DLL_InitTerm


;
; kLdrLoadExe wrapper which loads the bootstrap stack.
;
global _kLdrDyldLoadExe
_kLdrDyldLoadExe:
    push    ebp
    mov     ebp, esp

    ; switch stack.
;    extern _abStack
;    lea     esp, [_abStack + 8192 - 4]
    push    dword [ebp + 8 + 20]
    push    dword [ebp + 8 + 16]
    push    dword [ebp + 8 + 12]
    push    dword [ebp + 8 +  8]

    ; call worker on the new stack.
    extern  _kldrDyldLoadExe
    call    _kldrDyldLoadExe

    ; we shouldn't return!
we_re_not_supposed_to_get_here:
    int3
    int3
    jmp short we_re_not_supposed_to_get_here


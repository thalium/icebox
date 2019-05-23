; $Id: kLdrExeStub-os2.asm 29 2009-07-01 20:30:29Z bird $
;; @file
; kLdr - OS/2 Loader Stub.
;
; This file contains a 64kb code/data/stack segment which is used to kick off
; the loader dll that loads the process.
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

struc KLDRARGS
    .fFlags         resd 1
    .enmSearch      resd 1
    .szExecutable   resb 260
    .szDefPrefix    resb 16
    .szDefSuffix    resb 16
    .szLibPath      resb (4096 - (4 + 4 + 16 + 16 + 260))
endstruc

extern _kLdrDyldLoadExe


segment DATA32 stack CLASS=DATA align=16 use32
..start:
    push    args
    jmp     _kLdrDyldLoadExe

;
; Argument structure.
;
align 4
args:
istruc KLDRARGS
    at KLDRARGS.fFlags,         dd 0
    at KLDRARGS.enmSearch,      dd 2 ;KLDRDYLD_SEARCH_HOST
    at KLDRARGS.szDefPrefix,    db ''
    at KLDRARGS.szDefSuffix,    db '.dll'
;    at KLDRARGS.szExecutable,   db 'tst-0.exe'
    at KLDRARGS.szLibPath,      db ''
iend

segment STACK32 stack CLASS=STACK align=16 use32
; pad up to 64KB.
resb 60*1024

global WEAK$ZERO
WEAK$ZERO EQU 0
group DGROUP, DATA32  STACK32


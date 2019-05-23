;------------------------------------------------------------------------------
; @file
; Sets the CR3 register for 64-bit paging
;
; Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

BITS    32

%define PAGE_PRESENT            0x01
%define PAGE_READ_WRITE         0x02
%define PAGE_USER_SUPERVISOR    0x04
%define PAGE_WRITE_THROUGH      0x08
%define PAGE_CACHE_DISABLE     0x010
%define PAGE_ACCESSED          0x020
%define PAGE_DIRTY             0x040
%define PAGE_PAT               0x080
%define PAGE_GLOBAL           0x0100
%define PAGE_2M_MBO            0x080
%define PAGE_2M_PAT          0x01000

%define PAGE_2M_PDE_ATTR (PAGE_2M_MBO + \
                          PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

%define PAGE_PDP_ATTR (PAGE_ACCESSED + \
                       PAGE_READ_WRITE + \
                       PAGE_PRESENT)


;
; Modified:  EAX, ECX
;
SetCr3ForPageTables64:

    ;
%ifdef VBOX
    ; For OVMF, build some initial page tables at 0x2000000-0x2006000.
    ; Required because we had to relocate MEMFD where the page table resides 
    ; from 0x800000 to not interfere with certain OS X bootloaders.
%else
    ; For OVMF, build some initial page tables at 0x800000-0x806000.
%endif
    ;
    ; This range should match with PcdOvmfSecPageTablesBase and
    ; PcdOvmfSecPageTablesSize which are declared in the FDF files.
    ;
    ; At the end of PEI, the pages tables will be rebuilt into a
    ; more permanent location by DxeIpl.
    ;

    mov     ecx, 6 * 0x1000 / 4
    xor     eax, eax
clearPageTablesMemoryLoop:
%ifdef VBOX
    mov     dword[ecx * 4 + 0x2000000 - 4], eax
%else
    mov     dword[ecx * 4 + 0x800000 - 4], eax
%endif
    loop    clearPageTablesMemoryLoop

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
%ifdef VBOX
    mov     dword[0x2000000], 0x2001000 + PAGE_PDP_ATTR
%else
    mov     dword[0x800000], 0x801000 + PAGE_PDP_ATTR
%endif

    ;
    ; Next level Page Directory Pointers (4 * 1GB entries => 4GB)
    ;
%ifdef VBOX
    mov     dword[0x2001000], 0x2002000 + PAGE_PDP_ATTR
    mov     dword[0x2001008], 0x2003000 + PAGE_PDP_ATTR
    mov     dword[0x2001010], 0x2004000 + PAGE_PDP_ATTR
    mov     dword[0x2001018], 0x2005000 + PAGE_PDP_ATTR
%else
    mov     dword[0x801000], 0x802000 + PAGE_PDP_ATTR
    mov     dword[0x801008], 0x803000 + PAGE_PDP_ATTR
    mov     dword[0x801010], 0x804000 + PAGE_PDP_ATTR
    mov     dword[0x801018], 0x805000 + PAGE_PDP_ATTR
%endif

    ;
    ; Page Table Entries (2048 * 2MB entries => 4GB)
    ;
    mov     ecx, 0x800
pageTableEntriesLoop:
    mov     eax, ecx
    dec     eax
    shl     eax, 21
    add     eax, PAGE_2M_PDE_ATTR
%ifdef VBOX
    mov     [ecx * 8 + 0x2002000 - 8], eax
%else
    mov     [ecx * 8 + 0x802000 - 8], eax
%endif
    loop    pageTableEntriesLoop

    ;
    ; Set CR3 now that the paging structures are available
    ;
%ifdef VBOX
    mov     eax, 0x2000000
%else
    mov     eax, 0x800000
%endif
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64

/*
  Copyright (C) 2000,2004,2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2019 David Anderson. All Rights Reserved.
  Portions Copyright (C) 2011-2012 SN Systems Ltd. All Rights Reserved

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write the Free Software Foundation, Inc., 51
  Franklin Street - Fifth Floor, Boston MA 02110-1301, USA.

*/
#include "globals.h"
#ifdef DWARF_WITH_LIBELF
#ifndef SELFTEST
#define DWARF_RELOC_MIPS
#define DWARF_RELOC_PPC
#define DWARF_RELOC_PPC64
#define DWARF_RELOC_ARM
#define DWARF_RELOC_X86_64
#define DWARF_RELOC_386

#include "print_reloc.h"
#endif /* SELFTEST */
#include "print_reloc_decls.h"
#include "section_bitmaps.h"
#include "esb.h"
#include "sanitized.h"

int
main()
{
    int failcount = 0;
    unsigned long i = 1;
    for ( ; i < DW_SECTION_REL_ARRAY_SIZE; ++i) {
        if (rel_info[i].index != i) {
            printf(" FAIL rel_info check, i = %lu vs %lu\n",
                i,
                (unsigned long)rel_info[i].index);
            ++failcount;
        }
    }
    if(failcount) {
        printf("FAIL print_reloc selftest\n");
        exit(1);
    }
    printf("PASS print_reloc selftest\n");
    return 0;
}
#else
/*  Without libelf we don't care about this as
    we will not try to print relocation data. Pretend ok. */
int
main()
{
    return 0;
}
#endif /* DWARF_WITH_LIBELF */

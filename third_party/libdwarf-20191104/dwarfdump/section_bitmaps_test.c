/*
  Copyright (C) 2017-2018 David Anderson. All Rights Reserved.

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
/*  section_bitmaps.h and .c actually involved  bits,
    bit shifting, and bit masks,
    but now the 'maps' are simple byte arrays.
    See reloc_map and section_map in command_options.c */

#include "section_bitmaps.h"

int main()
{
    unsigned i = 1;

    unsigned arraycount = section_bitmap_array_size();

    if (arraycount  !=  DW_HDR_ARRAY_SIZE) {
        printf("FAIL map_sections.c sections array wrong size "
            "%u vs %u\n",
            arraycount,DW_HDR_ARRAY_SIZE);
        exit(1);
    }
    for ( ; i < DW_HDR_ARRAY_SIZE; ++i) {

        struct section_map_s * mp = map_sectnames+i;
        if (mp->value != i) {
            printf("FAIL map_sections.c at entry %s we have "
            "0x%x vs 0x%x"
                " mismatch\n",
                mp->name?mp->name:"<no name",
                mp->value,
                i);
            exit(1);
        }
        if (!mp->name) {
            printf("FAIL map_sections.c at entry %u we have no name!\n",i);
            exit(1);
        }
    }
    printf("PASS section maps\n");
    return 0;
}

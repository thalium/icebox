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

struct section_map_s
map_sectnames[DW_HDR_ARRAY_SIZE] = {
    {0,0},
    {DW_SECTNAME_DEBUG_INFO,            DW_HDR_DEBUG_INFO},
    {DW_SECTNAME_DEBUG_INFO_DWO,        DW_HDR_DEBUG_INFO_DWO},
    {DW_SECTNAME_DEBUG_LINE,            DW_HDR_DEBUG_LINE},
    {DW_SECTNAME_DEBUG_LINE_DWO,        DW_HDR_DEBUG_LINE_DWO},
    {DW_SECTNAME_DEBUG_PUBNAMES,        DW_HDR_DEBUG_PUBNAMES},
    {DW_SECTNAME_DEBUG_ABBREV,          DW_HDR_DEBUG_ABBREV},
    {DW_SECTNAME_DEBUG_ABBREV_DWO,      DW_HDR_DEBUG_ABBREV_DWO},
    {DW_SECTNAME_DEBUG_ARANGES,         DW_HDR_DEBUG_ARANGES},
    {DW_SECTNAME_DEBUG_FRAME,           DW_HDR_DEBUG_FRAME},
    {DW_SECTNAME_DEBUG_LOC,             DW_HDR_DEBUG_LOC},
    {DW_SECTNAME_DEBUG_LOCLISTS,        DW_HDR_DEBUG_LOCLISTS},
    {DW_SECTNAME_DEBUG_LOCLISTS_DWO,    DW_HDR_DEBUG_LOCLISTS_DWO},
    {DW_SECTNAME_DEBUG_RANGES,          DW_HDR_DEBUG_RANGES},
    {DW_SECTNAME_DEBUG_RNGLISTS,        DW_HDR_DEBUG_RNGLISTS},
    {DW_SECTNAME_DEBUG_RNGLISTS_DWO,    DW_HDR_DEBUG_RNGLISTS_DWO},
    {DW_SECTNAME_DEBUG_STR,             DW_HDR_DEBUG_STR},
    {DW_SECTNAME_DEBUG_STR_DWO,         DW_HDR_DEBUG_STR_DWO},
    {DW_SECTNAME_DEBUG_STR_OFFSETS,     DW_HDR_DEBUG_STR_OFFSETS},
    {DW_SECTNAME_DEBUG_STR_OFFSETS_DWO, DW_HDR_DEBUG_STR_OFFSETS_DWO},
    {DW_SECTNAME_DEBUG_PUBTYPES,        DW_HDR_DEBUG_PUBTYPES},
    {DW_SECTNAME_DEBUG_TYPES,           DW_HDR_DEBUG_TYPES},
    {DW_SECTNAME_TEXT,                  DW_HDR_TEXT},
    {DW_SECTNAME_GDB_INDEX,             DW_HDR_GDB_INDEX},
    {DW_SECTNAME_EH_FRAME,              DW_HDR_EH_FRAME},
    {DW_SECTNAME_DEBUG_MACINFO,         DW_HDR_DEBUG_MACINFO},
    {DW_SECTNAME_DEBUG_MACRO,           DW_HDR_DEBUG_MACRO},
    {DW_SECTNAME_DEBUG_MACRO_DWO,       DW_HDR_DEBUG_MACRO_DWO},
    {DW_SECTNAME_DEBUG_NAMES,           DW_HDR_DEBUG_NAMES},
    {DW_SECTNAME_DEBUG_CU_INDEX,        DW_HDR_DEBUG_CU_INDEX},
    {DW_SECTNAME_DEBUG_TU_INDEX,        DW_HDR_DEBUG_TU_INDEX},
    {"Elf Header",                      DW_HDR_HEADER},
};

/* See section_bitmaps.c, .h Control section header
   printing. (not DWARF printing)  */
static char reloc_map[DW_SECTION_REL_ARRAY_SIZE];
static char section_map[DW_HDR_ARRAY_SIZE];

static boolean all_sections_on;

boolean
section_name_is_debug_and_wanted(const char *section_name)
{
    unsigned i = 1;
    if (all_sections_on) {
        return TRUE;
    }
    for ( ; i < DW_HDR_ARRAY_SIZE; ++i) {
        if(!strcmp(section_name,map_sectnames[i].name) &&
            section_map[map_sectnames[i].value]) {
            return TRUE;
        }
    }
    return FALSE;
}

/* For now defaults matches all but .text. */
void
set_all_section_defaults(void)
{
    unsigned i = 1;
    for ( ; i < DW_HDR_ARRAY_SIZE; ++i) {
        section_map[i] = TRUE;
    }
}

void
set_all_sections_on(void)
{
    unsigned i = 1;
    all_sections_on = TRUE;
    for ( ; i < DW_HDR_ARRAY_SIZE; ++i) {
        section_map[i] = TRUE;
    }
}
void set_all_reloc_sections_on(void)
{
    unsigned i = 1;
    for ( ; i < DW_SECTION_REL_ARRAY_SIZE; ++i) {
        reloc_map[i] = TRUE;
    }
}

boolean
any_section_header_to_print(void)
{
    unsigned i = 1;
    for ( ; i < DW_HDR_HEADER; ++i) {
        if(section_map[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

/*  TRUE if the section map entry specified by the index has been enabled. */
boolean
section_map_enabled(unsigned index)
{
    if (index <= 0 || index >= DW_HDR_ARRAY_SIZE)
        return FALSE;
    return section_map[index];
}

void
enable_section_map_entry(unsigned index)
{
    if (index > 0 && index < DW_HDR_ARRAY_SIZE) {
        section_map[index] = TRUE;
    }
}

/*  TRUE if the reloc map entry specified by the index has been enabled. */
boolean
reloc_map_enabled(unsigned index)
{
    if (index <= 0 || index >= DW_SECTION_REL_ARRAY_SIZE)
        return FALSE;
    return reloc_map[index];
}

void
enable_reloc_map_entry(unsigned index)
{
    if (index > 0 && index < DW_SECTION_REL_ARRAY_SIZE) {
        reloc_map[index] = TRUE;
    }
}

#ifdef SELFTEST

int main()
{
    unsigned i = 1;

    unsigned arraycount = sizeof(map_sectnames)/
        sizeof (struct section_map_s);;

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




#endif /* SELFTEST */

/*
  Copyright (C) 2017-2017 David Anderson. All Rights Reserved.

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

#ifndef SECTION_BITMAPS_H_INCLUDED
#define SECTION_BITMAPS_H_INCLUDED

/*  section_bitmaps.h and .c actually involved  bits,
    bit shifting, and bit masks,
    but now the 'maps' are simple byte arrays.
    See reloc_map and section_map in dwarfdump.c */

/* Value is one of the DW_HDR_DEBUG_* names. */
struct section_map_s {
    const char *name;
    unsigned    value;
};

extern struct section_map_s  map_sectnames[] ;

#define DW_HDR_DEBUG_INFO            1
#define DW_HDR_DEBUG_INFO_DWO        2
#define DW_HDR_DEBUG_LINE            3
#define DW_HDR_DEBUG_LINE_DWO        4
#define DW_HDR_DEBUG_PUBNAMES        5
#define DW_HDR_DEBUG_ABBREV          6
#define DW_HDR_DEBUG_ABBREV_DWO      7
#define DW_HDR_DEBUG_ARANGES         8
#define DW_HDR_DEBUG_FRAME           9
#define DW_HDR_DEBUG_LOC             10
#define DW_HDR_DEBUG_LOCLISTS        11
#define DW_HDR_DEBUG_LOCLISTS_DWO    12
#define DW_HDR_DEBUG_RANGES          13
#define DW_HDR_DEBUG_RNGLISTS        14
#define DW_HDR_DEBUG_RNGLISTS_DWO    15
#define DW_HDR_DEBUG_STR             16
#define DW_HDR_DEBUG_STR_DWO         17
#define DW_HDR_DEBUG_STR_OFFSETS     18
#define DW_HDR_DEBUG_STR_OFFSETS_DWO 19
#define DW_HDR_DEBUG_PUBTYPES        20
#define DW_HDR_DEBUG_TYPES           21
#define DW_HDR_TEXT                  22
#define DW_HDR_GDB_INDEX             23
#define DW_HDR_EH_FRAME              24
#define DW_HDR_DEBUG_MACINFO         25
#define DW_HDR_DEBUG_MACRO           26
#define DW_HDR_DEBUG_MACRO_DWO       27
#define DW_HDR_DEBUG_NAMES           28
#define DW_HDR_DEBUG_CU_INDEX        29
#define DW_HDR_DEBUG_TU_INDEX        30
#define DW_HDR_HEADER                31

#define DW_HDR_ARRAY_SIZE            32



/* Debug section names to be included in printing */
#define DW_SECTNAME_DEBUG_INFO       ".debug_info"
#define DW_SECTNAME_DEBUG_INFO_DWO   ".debug_info.dwo"
#define DW_SECTNAME_DEBUG_LINE       ".debug_line"
#define DW_SECTNAME_DEBUG_LINE_DWO   ".debug_line.dwo"
#define DW_SECTNAME_DEBUG_PUBNAMES   ".debug_pubnames"
#define DW_SECTNAME_DEBUG_ABBREV     ".debug_abbrev"
#define DW_SECTNAME_DEBUG_ABBREV_DWO ".debug_abbrev.dwo"
#define DW_SECTNAME_DEBUG_ARANGES    ".debug_aranges"
#define DW_SECTNAME_DEBUG_FRAME      ".debug_frame"
#define DW_SECTNAME_DEBUG_LOC        ".debug_loc"
#define DW_SECTNAME_DEBUG_LOCLISTS   ".debug_loclists"
#define DW_SECTNAME_DEBUG_LOCLISTS_DWO ".debug_loclists.dwo"
#define DW_SECTNAME_DEBUG_RANGES     ".debug_ranges"
#define DW_SECTNAME_DEBUG_RNGLISTS   ".debug_rnglists"
#define DW_SECTNAME_DEBUG_RNGLISTS_DWO ".debug_rnglists.dwo"
#define DW_SECTNAME_DEBUG_STR        ".debug_str"
#define DW_SECTNAME_DEBUG_STR_DWO    ".debug_str.dwo"
#define DW_SECTNAME_DEBUG_STR_OFFSETS ".debug_str_offsets"
#define DW_SECTNAME_DEBUG_STR_OFFSETS_DWO ".debug_str_offsets.dwo"
/*  obsolete SGI-only section was .debug_typenames */
#define DW_SECTNAME_DEBUG_PUBTYPES  ".debug_pubtypes"
#define DW_SECTNAME_DEBUG_TYPES     ".debug_types"
#define DW_SECTNAME_TEXT            ".text"
#define DW_SECTNAME_GDB_INDEX       ".gdb_index"
#define DW_SECTNAME_EH_FRAME        ".eh_frame"
#define DW_SECTNAME_DEBUG_SUP       ".debug_sup"
#define DW_SECTNAME_DEBUG_MACINFO   ".debug_macinfo"
#define DW_SECTNAME_DEBUG_MACRO     ".debug_macro"
#define DW_SECTNAME_DEBUG_MACRO_DWO ".debug_macro.dwo"
#define DW_SECTNAME_DEBUG_NAMES     ".debug_names"
#define DW_SECTNAME_DEBUG_CU_INDEX  ".debug_cu_index"
#define DW_SECTNAME_DEBUG_TU_INDEX  ".debug_tu_index"

/* Definitions for printing relocations. */
#define DW_SECTION_REL_DEBUG_INFO     1
#define DW_SECTION_REL_DEBUG_LINE     2
#define DW_SECTION_REL_DEBUG_PUBNAMES 3
#define DW_SECTION_REL_DEBUG_ABBREV   4
#define DW_SECTION_REL_DEBUG_ARANGES  5
#define DW_SECTION_REL_DEBUG_FRAME    6
#define DW_SECTION_REL_DEBUG_LOC      7
#define DW_SECTION_REL_DEBUG_LOCLISTS 8
#define DW_SECTION_REL_DEBUG_RANGES   9
#define DW_SECTION_REL_DEBUG_RNGLISTS 10
#define DW_SECTION_REL_DEBUG_TYPES    11
#define DW_SECTION_REL_DEBUG_STR_OFFSETS 12
#define DW_SECTION_REL_DEBUG_PUBTYPES    13
#define DW_SECTION_REL_GDB_INDEX   14
#define DW_SECTION_REL_EH_FRAME    15
#define DW_SECTION_REL_DEBUG_SUP         16
#define DW_SECTION_REL_DEBUG_MACINFO     17
#define DW_SECTION_REL_DEBUG_MACRO       18
#define DW_SECTION_REL_DEBUG_NAMES       19
#define DW_SECTION_REL_ARRAY_SIZE 20

#define DW_SECTNAME_RELA_DEBUG_INFO     ".rela.debug_info"
#define DW_SECTNAME_RELA_DEBUG_LINE     ".rela.debug_line"
#define DW_SECTNAME_RELA_DEBUG_PUBNAMES ".rela.debug_pubnames"
#define DW_SECTNAME_RELA_DEBUG_ABBREV   ".rela.debug_abbrev"
#define DW_SECTNAME_RELA_DEBUG_ARANGES  ".rela.debug_aranges"
#define DW_SECTNAME_RELA_DEBUG_FRAME    ".rela.debug_frame"
#define DW_SECTNAME_RELA_DEBUG_LOC      ".rela.debug_loc"
#define DW_SECTNAME_RELA_DEBUG_LOCLISTS ".rela.debug_loclists"
#define DW_SECTNAME_RELA_DEBUG_RANGES   ".rela.debug_ranges"
#define DW_SECTNAME_RELA_DEBUG_RNGLISTS ".rela.debug_rnglists"
#define DW_SECTNAME_RELA_DEBUG_TYPES    ".rela.debug_types"
#define DW_SECTNAME_RELA_DEBUG_STR_OFFSETS    ".rela.debug_str_offsets"
#define DW_SECTNAME_RELA_DEBUG_PUBTYPES ".rela.debug_pubtypes"
#define DW_SECTNAME_RELA_GDB_INDEX ".rela.debug_gdb_index"
#define DW_SECTNAME_RELA_EH_FRAME ".rela.eh_frame"
#define DW_SECTNAME_RELA_DEBUG_SUP      ".rela.debug_sup"
#define DW_SECTNAME_RELA_DEBUG_MACINFO  ".rela.debug_macinfo"
#define DW_SECTNAME_RELA_DEBUG_MACRO    ".rela.debug_macro"
#define DW_SECTNAME_RELA_DEBUG_NAMES    ".rela.debug_names"

#define DW_SECTNAME_REL_DEBUG_INFO     ".rel.debug_info"
#define DW_SECTNAME_REL_DEBUG_LINE     ".rel.debug_line"
#define DW_SECTNAME_REL_DEBUG_PUBNAMES ".rel.debug_pubnames"
#define DW_SECTNAME_REL_DEBUG_ABBREV   ".rel.debug_abbrev"
#define DW_SECTNAME_REL_DEBUG_ARANGES  ".rel.debug_aranges"
#define DW_SECTNAME_REL_DEBUG_FRAME    ".rel.debug_frame"
#define DW_SECTNAME_REL_DEBUG_LOC      ".rel.debug_loc"
#define DW_SECTNAME_REL_DEBUG_LOCLISTS ".rel.debug_loclists"
#define DW_SECTNAME_REL_DEBUG_RANGES   ".rel.debug_ranges"
#define DW_SECTNAME_REL_DEBUG_RNGLISTS ".rel.debug_rnglists"
#define DW_SECTNAME_REL_DEBUG_TYPES    ".rel.debug_types"
#define DW_SECTNAME_REL_DEBUG_STR_OFFSETS ".rel.debug_str_offsets"
#define DW_SECTNAME_REL_DEBUG_PUBTYPES ".rel.debug_pubtypes"
#define DW_SECTNAME_REL_GDB_INDEX ".rel.debug_gdb_index"
#define DW_SECTNAME_REL_EH_FRAME ".rel.eh_frame"
#define DW_SECTNAME_REL_DEBUG_SUP      ".rel.debug_sup"
#define DW_SECTNAME_REL_DEBUG_MACINFO  ".rel.debug_macinfo"
#define DW_SECTNAME_REL_DEBUG_MACRO    ".rel.debug_macro"
#define DW_SECTNAME_REL_DEBUG_NAMES    ".rel.debug_names"


boolean section_name_is_debug_and_wanted(const char *section_name);

void set_all_section_defaults(void);

boolean any_section_header_to_print(void);

void enable_section_map_entry(unsigned index);
boolean section_map_enabled(unsigned index);
void set_all_sections_on(void);

void enable_reloc_map_entry(unsigned index);
boolean reloc_map_enabled(unsigned index);
void set_all_reloc_sections_on(void);

#endif /* SECTION_BITMAPS_H_INCLUDED*/

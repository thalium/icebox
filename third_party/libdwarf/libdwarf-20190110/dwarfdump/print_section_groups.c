/*
  Copyright (C) 2017-2017  David Anderson. All rights reserved.

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
#include "sanitized.h"
#include "naming.h"

/*  Two purposes here related to COMDAT:
    A)  get and print the data on sections and groups.
    B)  reset certain global 'print' flags so printing
        a COMDAT section does not print the major
        sections that are from group 1 (DW_GROUPDATA_BASE)

    The functions are called from dwarfdump.c and only once.
    So static vars are safe.
*/

static    Dwarf_Unsigned  group_map_entry_count = 0;
static    Dwarf_Unsigned  selected_group = 0;
static    Dwarf_Unsigned *sec_nums = 0;
static    Dwarf_Unsigned *group_nums = 0;
static    const char **   sec_names = 0;
static    Dwarf_Unsigned  group_count;
static    Dwarf_Unsigned  section_count;

static void
freeall_groups_tables(void)
{
    free(sec_nums);
    sec_nums = 0;
    free(group_nums);
    group_nums = 0;
    /*  Cast prevents an ugly warning about the const
        being stripped off. */
    free((void*)sec_names);
    sec_names = 0;
    group_map_entry_count = 0;
    selected_group = 0;
    group_count = 0;
    section_count = 0;
}

#define TRUE 1
#define FALSE 0

static struct  glfsetting_s {
    const char *secname;
    boolean *flag;
    boolean origset;
    boolean origflag;
} glftab[] = {
{".debug_abbrev",       &glflags.gf_abbrev_flag,FALSE,FALSE},
{".debug_aranges",      &glflags.gf_aranges_flag,FALSE,FALSE},
{".debug_debug_macinfo",&glflags.gf_macinfo_flag,FALSE,FALSE},
{".debug_debug_macro",  &glflags.gf_macro_flag,FALSE,FALSE},
{".debug_debug_names",  &glflags.gf_debug_names_flag,FALSE,FALSE},
{".debug_eh_frame",     &glflags.gf_eh_frame_flag,FALSE,FALSE},
{".debug_frame",        &glflags.gf_frame_flag,FALSE,FALSE},
{".gdb_index",          &glflags.gf_gdbindex_flag,FALSE,FALSE},
{".debug_info",         &glflags.gf_info_flag,FALSE,FALSE},
{".debug_line",         &glflags.gf_line_flag,FALSE,FALSE},
{".debug_loc",          &glflags.gf_loc_flag,FALSE,FALSE},
/*{".debug_loclists",     &glflags.gf_loclists_flag,FALSE,FALSE}, */
{".debug_pubnames",     &glflags.gf_pubnames_flag,FALSE,FALSE},
{".debug_pubtypes",     &glflags.gf_pubtypes_flag,FALSE,FALSE}, /* SGI only */
{".debug_ranges",       &glflags.gf_ranges_flag,FALSE,FALSE},
/*{".debug_rnglists",     &glflags.gf_rnglists_flag,FALSE,FALSE},  */
{".debug_static_func",  &glflags.gf_static_func_flag,FALSE,FALSE}, /* SGI only */
{".debug_static_var",   &glflags.gf_static_var_flag,FALSE,FALSE}, /* SGI only */
{".debug_str",          &glflags.gf_string_flag,FALSE,FALSE},
{".debug_types",        &glflags.gf_types_flag,FALSE,FALSE},
{".debug_weaknames",    &glflags.gf_weakname_flag,FALSE,FALSE}, /* SGI only */
{0,0,0,0}
};


/*  If a section is not in group N but is in group 1
    then turn off its flag. Since sections are never
    in both (various DW_DLE*DUPLICATE errors
    if libdwarf tries to set in both), just look in
    group one.

    See groups_restore_subsidiary_flags() just below.

    FIXME: It would be good if, for a wholly missing
    section related to a flag, that the flag got turned
    off.  */
static void
turn_off_subsidiary_flags(UNUSEDARG Dwarf_Debug dbg)
{
    Dwarf_Unsigned i = 0;

    for( ; i < group_map_entry_count; ++i) {
        if (group_nums[i] == 1) {
            unsigned k = 0;
            const char* oursec = sec_names[i];

            for( ; glftab[k].secname; ++k ) {
                if (!strcmp(oursec,glftab[k].secname)) {
                    if(!glftab[k].origset) {
                        glftab[k].origset = TRUE;
                        glftab[k].origflag = *(glftab[k].flag);
                    }
                    *(glftab[k].flag) = FALSE;
                }
            }
        }
    }
}

/*  Restoring original condition in the glftab array
    and in the global flags it points to.
    So that when processing an archive one can restore
    the user-chosen flags and print subsequent object
    groups correctly.
    New October 16, 2017.  */

void
groups_restore_subsidiary_flags(void)
{
    unsigned k = 0;

    /*  Duplicative but harmless free. */
    freeall_groups_tables();

    for( ; glftab[k].secname; ++k ) {
        if(glftab[k].origset) {
            *(glftab[k].flag) = glftab[k].origflag;
            glftab[k].origset = FALSE;
            glftab[k].origflag = FALSE;
        }
    }
}


/*  NEW May 2017.
    Has a side effect of using the local table set up by
    print_section_groups_data() and then frees the
    table data.

    For multi-object archive reading: main() calls
    groups_restore_subsidiary_flags() at the end
    of each object file to restore
    the original flags that turn_off_subsidiary_flags()
    changed.
    */
void
update_section_flags_per_groups(Dwarf_Debug dbg)
{
    if (!sec_names) {
        /*  The tables are absent. Internal logic
            error here somewhere. */
        freeall_groups_tables();
        return;
    }
    if (selected_group == DW_GROUPNUMBER_BASE) {
        freeall_groups_tables();
        return;
    }
    if (selected_group == DW_GROUPNUMBER_DWO) {
        freeall_groups_tables();
        return;
    }
    turn_off_subsidiary_flags(dbg);
    freeall_groups_tables();
}

/*  NEW May 2017.
    Reports on section groupings like DWO(split dwarf)
    and COMDAT groups. As a side effect creates local
    table of section and group data */
void
print_section_groups_data(Dwarf_Debug dbg)
{
    int res = 0;
    Dwarf_Error error = 0;
    Dwarf_Unsigned i = 0;

    res = dwarf_sec_group_sizes(dbg,&section_count,
        &group_count,&selected_group, &group_map_entry_count,
        &error);
    if(res != DW_DLV_OK) {
        print_error(dbg, "dwarf_sec_group_sizes", res, error);
    }
    if (group_count == 1 && selected_group ==1 ) {
        /*  This is the traditional DWARF with no split-dwarf
            and no COMDAT data.
            We don't want to print anything as we do not want
            to see differences from existing output in this case.
            Simplifies regression testing for now. */
        return;
    }
    printf("Section Groups data\n");
    printf("  Number of Elf-like sections: %4" DW_PR_DUu "\n",
        section_count);
    printf("  Number of groups           : %4" DW_PR_DUu "\n",
        group_count);
    printf("  Group to print             : %4" DW_PR_DUu "\n",
        selected_group);
    printf("  Count of map entries       : %4" DW_PR_DUu "\n",
        group_map_entry_count);

    sec_nums = calloc(group_map_entry_count,sizeof(Dwarf_Unsigned));
    if(!sec_nums) {
        printf("ERROR: Unable to allocate %4" DW_PR_DUu
            " map section values, cannot print group map\n",
            group_map_entry_count);
        return;
    }
    group_nums = calloc(group_map_entry_count,sizeof(Dwarf_Unsigned));
    if(!group_nums) {
        free(group_nums);
        printf("ERROR: Unable to allocate %4" DW_PR_DUu
            " map group values, cannot print group map\n",
            group_map_entry_count);
        return;
    }
    sec_names = calloc(group_map_entry_count,sizeof(char*));
    if(!sec_names) {
        free(group_nums);
        free(sec_nums);
        printf("ERROR: Unable to allocate %4" DW_PR_DUu
            " section name pointers, cannot print group map\n",
            group_map_entry_count);
        return;
    }

    res = dwarf_sec_group_map(dbg,group_map_entry_count,
        group_nums,sec_nums,sec_names,&error);
    if(res != DW_DLV_OK) {
        print_error(dbg, "dwarf_sec_group_map", res, error);
    }

    for( i = 0; i < group_map_entry_count; ++i) {
        if (i == 0) {
            printf("  [index]  group section\n");
        }
        printf("  [%5" DW_PR_DUu "] "
            "%4" DW_PR_DUu
            "  %4" DW_PR_DUu
            " %s\n",i,
            group_nums[i],sec_nums[i],sanitized(sec_names[i]));
    }
    /* Do not free our allocations. Will do later. */
    return;
}

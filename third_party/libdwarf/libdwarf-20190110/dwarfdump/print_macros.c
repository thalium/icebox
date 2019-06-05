/*
  Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
  Portions Copyright 2009-2018 SN Systems Ltd. All rights reserved.
  Portions Copyright 2008-2018 David Anderson. All rights reserved.

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
#include "naming.h"
#include "macrocheck.h"
#include "esb.h"
#include "esb_using_functions.h"
#include "sanitized.h"

#include "print_sections.h"
#include "print_frames.h"

#define TRUE 1
#define FALSE 0

struct macro_counts_s {
    long mc_start_file;
    long mc_end_file;
    long mc_define;
    long mc_undef;
    long mc_extension;
    long mc_code_zero;
    long mc_unknown;
};

static void
print_one_macro_entry_detail(long i,
    char *type,
    struct Dwarf_Macro_Details_s *mdp)
{
    /* "DW_MACINFO_*: section-offset file-index [line] string\n" */
    if (glflags.gf_do_print_dwarf) {
        if (mdp->dmd_macro) {
            printf("%3ld %s: %6" DW_PR_DUu " %2" DW_PR_DSd " [%4"
                DW_PR_DSd "] \"%s\" \n",
                i,
                type,
                (Dwarf_Unsigned)mdp->dmd_offset,
                mdp->dmd_fileindex, mdp->dmd_lineno,
                sanitized(mdp->dmd_macro));
        } else {
            printf("%3ld %s: %6" DW_PR_DUu " %2" DW_PR_DSd " [%4"
                DW_PR_DSd "] 0\n",
                i,
                type,
                (Dwarf_Unsigned)mdp->dmd_offset,
                mdp->dmd_fileindex, mdp->dmd_lineno);
        }
    }

}

static void
print_one_macro_entry(long i,
    struct Dwarf_Macro_Details_s *mdp,
    struct macro_counts_s *counts)
{

    switch (mdp->dmd_type) {
    case 0:
        counts->mc_code_zero++;
        print_one_macro_entry_detail(i, "DW_MACINFO_type-code-0", mdp);
        break;

    case DW_MACINFO_start_file:
        counts->mc_start_file++;
        print_one_macro_entry_detail(i, "DW_MACINFO_start_file", mdp);
        break;

    case DW_MACINFO_end_file:
        counts->mc_end_file++;
        print_one_macro_entry_detail(i, "DW_MACINFO_end_file  ", mdp);
        break;

    case DW_MACINFO_vendor_ext:
        counts->mc_extension++;
        print_one_macro_entry_detail(i, "DW_MACINFO_vendor_ext", mdp);
        break;

    case DW_MACINFO_define:
        counts->mc_define++;
        print_one_macro_entry_detail(i, "DW_MACINFO_define    ", mdp);
        break;

    case DW_MACINFO_undef:
        counts->mc_undef++;
        print_one_macro_entry_detail(i, "DW_MACINFO_undef     ", mdp);
        break;

    default:
        {
            struct esb_s typeb;

            esb_constructor(&typeb);
            counts->mc_unknown++;
            esb_append_printf(&typeb,
                "DW_MACINFO_0x%x", mdp->dmd_type);
            print_one_macro_entry_detail(i,
                esb_get_string(&typeb), mdp);
            esb_destructor(&typeb);
        }
        break;
    }
}

/*  print data in .debug_macinfo */
/*ARGSUSED*/ extern void
print_macinfo_by_offset(Dwarf_Debug dbg,Dwarf_Unsigned offset)
{
    Dwarf_Unsigned max = 0;
    Dwarf_Signed count = 0;
    Dwarf_Macro_Details *maclist = NULL;
    int lres = 0;
    long i = 0;
    struct macro_counts_s counts;
    Dwarf_Unsigned totallen = 0;
    Dwarf_Bool is_primary = TRUE;
    Dwarf_Error error = 0;

    glflags.current_section_id = DEBUG_MACINFO;

    /*  No real need to get the real section name, this
        section not used much in modern compilers
        as this definition of macro data (V2-V4)
        is obsolete as it takes too much space to be
        much used. */

    lres = dwarf_get_macro_details(dbg, offset,
        max, &count, &maclist, &error);
    if (lres == DW_DLV_ERROR) {
        print_error(dbg, "dwarf_get_macro_details", lres, error);
    } else if (lres == DW_DLV_NO_ENTRY) {
        return;
    }

    memset(&counts, 0, sizeof(counts));
    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_macinfo",
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
        printf("\n");
        printf("compilation-unit .debug_macinfo offset "
            "0x%" DW_PR_XZEROS DW_PR_DUx "\n",offset);
        printf("num name section-offset file-index [line] \"string\"\n");
    }
    for (i = 0; i < count; i++) {
        struct Dwarf_Macro_Details_s *mdp = &maclist[i];

        print_one_macro_entry(i, mdp, &counts);
    }

    if (counts.mc_start_file == 0) {
        printf("DW_MACINFO file count of zero is invalid DWARF2/3/4\n");
    }
    if (counts.mc_start_file != counts.mc_end_file) {
        printf("Counts of DW_MACINFO file (%ld) end_file (%ld) "
            "do not match!.\n",
            counts.mc_start_file, counts.mc_end_file);
    }
    if (counts.mc_code_zero < 1) {
        printf("Count of zeros in macro group should be non-zero "
            "(1 preferred), count is %ld\n",
            counts.mc_code_zero);
    }
    /* next byte is  maclist[count - 1].dmd_offset + 1; */
    totallen = (maclist[count - 1].dmd_offset + 1) - offset;
    add_macro_import(&macinfo_check_tree,is_primary, offset);
    add_macro_area_len(&macinfo_check_tree,offset,totallen);
    if (glflags.gf_do_print_dwarf) {
        printf("Macro counts: start file %ld, "
            "end file %ld, "
            "define %ld, "
            "undef %ld, "
            "ext %ld, "
            "code-zero %ld, "
            "unknown %ld\n",
            counts.mc_start_file,
            counts.mc_end_file,
            counts.mc_define,
            counts.mc_undef,
            counts.mc_extension,
            counts.mc_code_zero, counts.mc_unknown);
    }

    /* int type= maclist[count - 1].dmd_type; */
    /* ASSERT: type is zero */

    dwarf_dealloc(dbg, maclist, DW_DLA_STRING);
    return;
}

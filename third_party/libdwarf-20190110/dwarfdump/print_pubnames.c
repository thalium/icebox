/*
  Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
v Portions Copyright 2009-2011 SN Systems Ltd. All rights reserved.
  Portions Copyright 2008-2011 David Anderson. All rights reserved.

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
#include "esb.h"
#include "esb_using_functions.h"
#include "print_sections.h"
#include "sanitized.h"

/*  This unifies the code for some error checks to
    avoid code duplication.
*/
static void
check_info_offset_sanity(char *sec,
    char *field,
    char *global,
    Dwarf_Unsigned offset, Dwarf_Unsigned maxoff)
{
    if (maxoff == 0) {
        /* Lets make a heuristic check. */
        if (offset > 0xffffffff) {
            printf("Warning: section %s %s %s offset 0x%"
                DW_PR_XZEROS DW_PR_DUx " "
                "exceptionally large \n",
                sec, field, global, offset);
        }
        return;
    }
    if (offset >= maxoff) {
        printf("Warning: section %s %s %s offset 0x%"
            DW_PR_XZEROS  DW_PR_DUx " "
                "larger than max of 0x%" DW_PR_DUx "\n",
                sec, field, global,  offset, maxoff);
    }
}

/* Unified pubnames style output.
   The error checking here against maxoff may be useless
   (in that libdwarf may return an error if the offset is bad
   and we will not get called here).
   But we leave it in nonetheless as it looks sensible.
   In at least one gigantic executable such offsets turned out wrong.
*/
void
print_pubname_style_entry(Dwarf_Debug dbg,
   char *line_title,
   char *name,
   Dwarf_Unsigned die_off,
   Dwarf_Unsigned cu_off,
   Dwarf_Unsigned global_cu_offset,
   Dwarf_Unsigned maxoff)
{
    Dwarf_Die die = NULL;
    Dwarf_Off die_CU_off = 0;
    int dres = 0;
    int ddres = 0;
    int cudres = 0;
    Dwarf_Error err = 0;

    /* get die at die_off */
    dres = dwarf_offdie(dbg, die_off, &die, &err);
    if (dres != DW_DLV_OK) {
        struct esb_s details;
        esb_constructor(&details);
        esb_append(&details,line_title);
        esb_append(&details," dwarf_offdie : "
            "die offset does not reference valid DIE.  ");
        esb_append_printf(&details,"0x%"  DW_PR_DUx, die_off);
        esb_append(&details,".");
        print_error(dbg, esb_get_string(&details), dres, err);
        esb_destructor(&details);
    }

    /* get offset of die from its cu-header */
    ddres = dwarf_die_CU_offset(die, &die_CU_off, &err);
    if (ddres != DW_DLV_OK) {
        struct esb_s details;
        esb_constructor(&details);
        esb_append(&details,line_title);
        esb_append(&details," cannot get CU die offset");
        print_error(dbg, esb_get_string(&details), dres, err);
        esb_destructor(&details);
        die_CU_off = 0;
    }

    /* Get die at offset cu_off to check its existence. */
    {
        Dwarf_Die cu_die = NULL;
        cudres = dwarf_offdie(dbg, cu_off, &cu_die, &err);
        if (cudres != DW_DLV_OK) {
            struct esb_s details;
            esb_constructor(&details);
            esb_append(&details,line_title);
            esb_append(&details," dwarf_offdie: "
                "cu die offset  does not reference valid CU DIE.  ");
            esb_append_printf(&details,"0x%"  DW_PR_DUx, cu_off);
            esb_append(&details,".");
            print_error(dbg, esb_get_string(&details), dres, err);
            esb_destructor(&details);
        } else {
            /* It exists, all is well. */
            dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
        }
    }
    /* Display offsets */
    if (glflags.gf_display_offsets) {
        /* Print 'name'at the end for better layout */
        printf("%s die-in-sect 0x%" DW_PR_XZEROS DW_PR_DUx
            ", cu-in-sect 0x%" DW_PR_XZEROS DW_PR_DUx ","
            " die-in-cu 0x%" DW_PR_XZEROS DW_PR_DUx
            ", cu-header-in-sect 0x%" DW_PR_XZEROS DW_PR_DUx ,
            line_title,
            die_off, cu_off,
            (Dwarf_Unsigned) die_CU_off,
            /*  Following is absolute offset of the
                beginning of the cu */
            (Dwarf_Signed) (die_off - die_CU_off));
    }

    if ((die_off - die_CU_off) != global_cu_offset) {
        printf(" error: real cuhdr 0x%" DW_PR_XZEROS DW_PR_DUx,
            global_cu_offset);
        exit(1);
    }

    /* Display offsets */
    if (glflags.gf_display_offsets && glflags.verbose) {
        printf(" cuhdr 0x%" DW_PR_XZEROS DW_PR_DUx , global_cu_offset);
    }

    /* Print 'name'at the end for better layout */
    printf(" '%s'\n",name);

    dwarf_dealloc(dbg, die, DW_DLA_DIE);

    check_info_offset_sanity(line_title,
        "die offset", name, die_off, maxoff);
    check_info_offset_sanity(line_title,
        "die cu offset", name, die_CU_off, maxoff);
    check_info_offset_sanity(line_title,
        "cu offset", name,
        (die_off - die_CU_off), maxoff);

}

/* Get all the data in .debug_pubnames */
void
print_pubnames(Dwarf_Debug dbg)
{
    Dwarf_Global *globbuf = NULL;
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Off die_off = 0;
    Dwarf_Off cu_off = 0;
    Dwarf_Addr elf_max_address = 0;
    /* Offset to previous CU */
    Dwarf_Off prev_cu_off = elf_max_address;
    Dwarf_Error err = 0;
    char *name = 0;
    int res = 0;

    glflags.current_section_id = DEBUG_PUBNAMES;
    get_address_size_and_max(dbg,0,&elf_max_address,&err);
    res = dwarf_get_globals(dbg, &globbuf, &count, &err);
    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_pubnames",
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
    }
    if (res == DW_DLV_ERROR) {
        print_error(dbg, "dwarf_get_globals", res, err);
    } else if (res == DW_DLV_NO_ENTRY) {
        /*  (err == 0 && count == DW_DLV_NOCOUNT) means there are no
            pubnames.  */
    } else {
        Dwarf_Unsigned maxoff = get_info_max_offset(dbg);

        for (i = 0; i < count; i++) {
            int nres = 0;
            int cures3 = 0;
            Dwarf_Off global_cu_off = 0;

            nres = dwarf_global_name_offsets(globbuf[i],
                &name, &die_off, &cu_off,
                &err);
            deal_with_name_offset_err(dbg, "dwarf_global_name_offsets",
                name, die_off, nres, err);
            cures3 = dwarf_global_cu_offset(globbuf[i],
                &global_cu_off, &err);
            if (cures3 != DW_DLV_OK) {
                print_error(dbg, "dwarf_global_cu_offset", cures3, err);
            }

            if (glflags.gf_check_pubname_attr) {
                Dwarf_Bool has_attr;
                int ares;
                int dres;
                Dwarf_Die die;

                /*  We are processing a new set of pubnames
                    for a different CU; get the producer ID, at 'cu_off'
                    to see if we need to skip these pubnames */
                if (cu_off != prev_cu_off) {

                    /* Record offset for previous CU */
                    prev_cu_off = cu_off;

                    dres = dwarf_offdie(dbg, cu_off, &die, &err);
                    if (dres != DW_DLV_OK) {
                        print_error(dbg, "print pubnames: dwarf_offdie a", dres,err);
                    }

                    {
                        /*  Get producer name for this CU
                            and update compiler list */
                        struct esb_s producername;
                        esb_constructor(&producername);
                        get_producer_name(dbg,die,cu_off,&producername);
                        update_compiler_target(esb_get_string(&producername));
                        esb_destructor(&producername);
                    }

                    dwarf_dealloc(dbg, die, DW_DLA_DIE);
                }

                /* get die at die_off */
                dres = dwarf_offdie(dbg, die_off, &die, &err);
                if (dres != DW_DLV_OK) {
                    print_error(dbg, "print pubnames: dwarf_offdie b", dres, err);
                }


                ares =
                    dwarf_hasattr(die, DW_AT_external, &has_attr, &err);
                if (ares == DW_DLV_ERROR) {
                    print_error(dbg, "hassattr on DW_AT_external", ares,
                        err);
                }

                /*  Check for specific compiler */
                if (checking_this_compiler()) {
                    DWARF_CHECK_COUNT(pubname_attr_result,1);
                    if (ares == DW_DLV_OK && has_attr) {
                        /* Should the value of flag be examined? */
                    } else {
                        DWARF_CHECK_ERROR2(pubname_attr_result,name,
                            "pubname does not have DW_AT_external");
                    }
                }
                dwarf_dealloc(dbg, die, DW_DLA_DIE);
            }

            /* Now print pubname, after the test */
            if (glflags.gf_do_print_dwarf ||
                (glflags.gf_record_dwarf_error &&
                glflags.gf_check_verbose_mode)) {
                print_pubname_style_entry(dbg,
                    "global",
                    name, die_off, cu_off,
                    global_cu_off, maxoff);
                glflags.gf_record_dwarf_error = FALSE;
            }

        }
        dwarf_globals_dealloc(dbg, globbuf, count);
    }
}   /* print_pubnames() */

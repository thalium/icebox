/*
  Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
  Portions Copyright 2009-2018 SN Systems Ltd. All rights reserved.
  Portions Copyright 2008-2019 David Anderson. All rights reserved.

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

/*  The address of the Free Software Foundation is
    Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
    Boston, MA 02110-1301, USA.
    SGI has moved from the Crittenden Lane address.
*/

#include "globals.h"
#include "naming.h"
#include "esb.h"
#include "esb_using_functions.h"
#include "sanitized.h"

#include "print_sections.h"


static void
do_checking(Dwarf_Debug dbg, Dwarf_Arange *arange_buf,Dwarf_Signed i,
    Dwarf_Off cu_die_offset,Dwarf_Bool first_cu,
    Dwarf_Off cu_die_offset_prev, Dwarf_Die cu_die )
{
    int dres = 0;
    Dwarf_Off cuhdroff = 0;
    Dwarf_Off cudieoff3 = 0;
    Dwarf_Error checking_err = 0;
    /*  .debug_types has no address ranges, only .debug_info[.dwo]
        has them.*/
    int is_info = 1;


    dres = dwarf_get_arange_cu_header_offset(
        arange_buf[i],&cuhdroff,&checking_err);
    if (dres == DW_DLV_OK) {
        Dwarf_Off cudieoff2 = 0;

        /* Get the CU offset for easy error reporting */
        if (first_cu || cu_die_offset != cu_die_offset_prev) {
            dres = dwarf_die_offsets(cu_die,&glflags.DIE_overall_offset,
                &glflags.DIE_offset,&checking_err);
            glflags.DIE_CU_overall_offset = glflags.DIE_overall_offset;
            glflags.DIE_CU_offset = glflags.DIE_offset;
            if (dres != DW_DLV_OK) {
                print_error(dbg, "dwarf_die_offsets", dres, checking_err);
            }
        }
        dres = dwarf_get_cu_die_offset_given_cu_header_offset_b(
            dbg,cuhdroff,is_info,&cudieoff2,&checking_err);
        if (dres == DW_DLV_OK) {
            /* Get the CU offset for easy error reporting */
            dwarf_die_offsets(cu_die,&glflags.DIE_overall_offset,
                &glflags.DIE_offset,&checking_err);
            glflags.DIE_CU_overall_offset = glflags.DIE_overall_offset;
            glflags.DIE_CU_offset = glflags.DIE_offset;
            DWARF_CHECK_COUNT(aranges_result,1);
            if (cu_die_offset != cudieoff2) {
                printf("Error, cu_die offsets mismatch,  0x%"
                    DW_PR_DUx " != 0x%" DW_PR_DUx
                    " from arange data",
                    cu_die_offset,cudieoff2);
                DWARF_CHECK_ERROR(aranges_result,
                    " dwarf_get_cu_die_offset_given_cu..."
                    " gets wrong offset");
            }
        } else {
            print_error(dbg, "dwarf_get_cu_die_offset_given...", dres, checking_err);
        }
    } else {
        print_error(dbg, "dwarf_get_arange_cu_header_offset", dres, checking_err);
    }
    dres = dwarf_get_cu_die_offset(arange_buf[i],&cudieoff3,
        &checking_err);
    if (dres == DW_DLV_OK) {
        DWARF_CHECK_COUNT(aranges_result,1);
        if (cudieoff3 != cu_die_offset) {
            printf(
                "Error, cu_die offsets (b) mismatch ,  0x%"
                DW_PR_DUx
                " != 0x%" DW_PR_DUx " from arange data",
                cu_die_offset,cudieoff3);
            DWARF_CHECK_ERROR(aranges_result,
                " dwarf_get_cu_die_offset "
                " gets wrong offset");
        }
    } else {
        print_error(dbg, "dwarf_get_cu_die_offset failed ",
            dres,checking_err);
    }
}

/* get all the data in .debug_aranges */
extern void
print_aranges(Dwarf_Debug dbg)
{
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    Dwarf_Arange *arange_buf = NULL;
    int ares = 0;
    int aires = 0;
    Dwarf_Off prev_off = 0; /* Holds previous CU offset */
    Dwarf_Bool first_cu = TRUE;
    Dwarf_Off cu_die_offset_prev = 0;
    Dwarf_Error pa_error = 0;

    /* Reset the global state, so we can traverse the debug_info */
    glflags.seen_CU = FALSE;
    glflags.need_CU_name = TRUE;
    glflags.need_CU_base_address = TRUE;
    glflags.need_CU_high_address = TRUE;

    glflags.current_section_id = DEBUG_ARANGES;
    ares = dwarf_get_aranges(dbg, &arange_buf, &count, &pa_error);
    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_aranges",
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
    if (ares == DW_DLV_ERROR) {
        print_error(dbg, "dwarf_get_aranges", ares, pa_error);
    } else if (ares == DW_DLV_NO_ENTRY) {
        /* no arange is included */
    } else {
        for (i = 0; i < count; i++) {
            Dwarf_Unsigned segment = 0;
            Dwarf_Unsigned segment_entry_size = 0;
            Dwarf_Addr start = 0;
            Dwarf_Unsigned length = 0;
            Dwarf_Off cu_die_offset = 0;
            Dwarf_Die cu_die = NULL;
            aires = dwarf_get_arange_info_b(arange_buf[i],
                &segment,
                &segment_entry_size,
                &start, &length,
                &cu_die_offset, &pa_error);
            if (aires != DW_DLV_OK) {
                print_error(dbg, "dwarf_get_arange_info", aires, pa_error);
            } else {
                int dres;
                struct esb_s producer_name;
                esb_constructor(&producer_name);
                /*  Get basic locations for error reporting */
                dres = dwarf_offdie(dbg, cu_die_offset, &cu_die, &pa_error);
                if (dres != DW_DLV_OK) {
                    print_error(dbg, "dwarf_offdie", dres, pa_error);
                }

                if (glflags.gf_cu_name_flag) {
                    if (should_skip_this_cu(dbg,cu_die)) {
                        continue;
                    }
                }
                /* Get producer name for this CU and update compiler list */
                get_producer_name(dbg,cu_die,cu_die_offset,&producer_name);
                update_compiler_target(esb_get_string(&producer_name));
                esb_destructor(&producer_name);
                if (!checking_this_compiler()) {
                    continue;
                }

                if (glflags.gf_check_aranges) {
                    do_checking(dbg,arange_buf,i,
                        cu_die_offset,first_cu,
                        cu_die_offset_prev,cu_die);
                }
                /*  Get the offset of the cu header itself in the
                    section, but not for end-entries. */
                if (start || length) {
                    Dwarf_Off off = 0;
                    int cures3 = dwarf_get_arange_cu_header_offset(
                        arange_buf[i], &off, &pa_error);
                    if (cures3 != DW_DLV_OK) {
                        print_error(dbg, "dwarf_get_cu_hdr_offset",
                            cures3, pa_error);
                    }

                    /* Print the CU information if different.  */
                    if (prev_off != off || first_cu) {
                        first_cu = FALSE;
                        prev_off = off;
                        /*  We are faking the indent level. We do not know
                            what level it is, really.

                            If do_check_dwarf we do not want to do
                            the die print call as it will do
                            check/print we may not have asked for.
                            And if we did ask for debug_info checks
                            this will do the checks a second time!
                            So only call print_one_die if printing.
                        */
                        if (glflags.gf_do_print_dwarf){
                            /* There is no die if its a set-end entry */
                            print_one_die(dbg, cu_die,
                                cu_die_offset,
                                /* print_information= */ (boolean) TRUE,
                                /* indent_level = */0,
                                /* srcfiles= */ 0,
                                /* cnt= */ 0,
                                /* ignore_die_stack= */TRUE);
                        }
                        /* Reset the state, so we can traverse the debug_info */
                        glflags.seen_CU = FALSE;
                        glflags.need_CU_name = TRUE;
                        if (glflags.gf_do_print_dwarf) {
                            printf("\n");
                        }
                    }

                    if (glflags.gf_do_print_dwarf) {
                        /* Print current aranges record */
                        if (segment_entry_size) {
                            printf(
                                "\narange starts at seg,off 0x%"
                                DW_PR_XZEROS DW_PR_DUx
                                ",0x%" DW_PR_XZEROS DW_PR_DUx
                                ", ",
                                segment,
                                (Dwarf_Unsigned)start);
                        } else {
                            printf("\narange starts at 0x%"
                                DW_PR_XZEROS DW_PR_DUx ", ",
                                (Dwarf_Unsigned)start);
                        }
                        printf("length of 0x%" DW_PR_XZEROS DW_PR_DUx
                            ", cu_die_offset = 0x%" DW_PR_XZEROS DW_PR_DUx,
                            length,
                            (Dwarf_Unsigned)cu_die_offset);

                    }
                    if (glflags.verbose && glflags.gf_do_print_dwarf) {
                        printf(" cuhdr 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                            (Dwarf_Unsigned)off);
                    }
                    dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
                    cu_die = 0;
                } else {
                    /*  Must be a range end. We really do want to print
                        this as there is a real record here, an
                        'arange end' record. */
                    if (glflags.gf_do_print_dwarf) {
                        printf("\narange end\n");
                    }
                }/* end start||length test */
            }  /* end aires DW_DLV_OK test */

            /* print associated die too? */
            dwarf_dealloc(dbg, arange_buf[i], DW_DLA_ARANGE);
        }
        dwarf_dealloc(dbg, arange_buf, DW_DLA_LIST);
    }
}

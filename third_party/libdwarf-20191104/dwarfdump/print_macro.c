/*
  Copyright 2015-2019 David Anderson. All rights reserved.

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
#include "uri.h"
#include <ctype.h>
#include <time.h>

#include "print_sections.h"
#include "macrocheck.h"
#include "sanitized.h"

static void
print_source_intro(Dwarf_Die cu_die)
{
    Dwarf_Off off = 0;
    int ores = 0;
    Dwarf_Error err = 0;

    ores = dwarf_dieoffset(cu_die, &off, &err);
    if (ores == DW_DLV_OK) {
        int lres = 0;
        const char *sec_name = 0;

        lres = dwarf_get_die_section_name_b(cu_die,
            &sec_name,&err);
        if (lres != DW_DLV_OK ||  !sec_name || !strlen(sec_name)) {
            sec_name = ".debug_info";
        }
        printf("Macro data from CU-DIE at %s offset 0x%"
            DW_PR_XZEROS DW_PR_DUx ":\n",
            sec_name,
            (Dwarf_Unsigned) off);
    } else {
        printf("Macro data (for the CU-DIE at unknown location):\n");
    }
}

static void
print_macro_ops(Dwarf_Debug dbg,
    Dwarf_Macro_Context mcontext,
    Dwarf_Unsigned number_of_ops)
{
    unsigned k = 0;

    for (k = 0; k < number_of_ops; ++k) {
        Dwarf_Unsigned  section_offset = 0;
        Dwarf_Half      macro_operator = 0;
        Dwarf_Half      forms_count = 0;
        const Dwarf_Small *formcode_array = 0;
        Dwarf_Unsigned  line_number = 0;
        Dwarf_Unsigned  index = 0;
        Dwarf_Unsigned  offset =0;
        const char    * macro_string =0;
        int lres = 0;
        Dwarf_Error err = 0;

        lres = dwarf_get_macro_op(mcontext,
            k, &section_offset,&macro_operator,
            &forms_count, &formcode_array,&err);
        if (lres != DW_DLV_OK) {
            print_error(dbg,
                "ERROR from  dwarf_get_macro_op()",
                lres,err);
            return;
        }
        if (glflags.gf_do_print_dwarf) {
            printf("   [%3d] 0x%02x %-20s",
                k,macro_operator,
                (macro_operator?
                    get_MACRO_name(macro_operator,dwarf_names_print_on_error):
                    "end-of-macros"));
        }
        if (glflags.gf_do_print_dwarf && glflags.show_form_used &&
            forms_count > 0) {
            unsigned l = 0;
            printf("\n     Forms count %2u:",forms_count);
            for(; l < forms_count;++l) {
                Dwarf_Small form = formcode_array[l];
                printf(" 0x%02x %-18s ",
                    form,
                    get_FORM_name(form,dwarf_names_print_on_error));
            }
            printf("\n   ");
        }
        switch(macro_operator) {
        case 0:
        case DW_MACRO_end_file:
            if(glflags.gf_do_print_dwarf) {
                printf("\n");
            }
            break;
        case DW_MACRO_define:
        case DW_MACRO_undef: {
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                &err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from  dwarf_get_macro_defundef()",
                    lres,err);
            }
            if (glflags.gf_do_print_dwarf) {
                printf("  line %" DW_PR_DUu
                    " %s\n",
                    line_number,
                    macro_string?sanitized(macro_string):"<no-name-available>");
            }
            break;
            }

        case DW_MACRO_define_strp:
        case DW_MACRO_undef_strp: {
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                &err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from  strp dwarf_get_macro_defundef()",
                    lres,err);
            }
            if (glflags.gf_do_print_dwarf) {
                printf("  line %" DW_PR_DUu
                    " str offset 0x%" DW_PR_XZEROS DW_PR_DUx
                    " %s\n",
                    line_number,offset,
                    macro_string?sanitized(macro_string):"<no-name-available>");
            }
            break;
            }
        case DW_MACRO_define_strx:
        case DW_MACRO_undef_strx: {
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                &err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from strx dwarf_get_macro_defundef()",
                    lres,err);
            }
            if (glflags.gf_do_print_dwarf) {
                printf("  line %" DW_PR_DUu
                    " index 0x%" DW_PR_XZEROS DW_PR_DUx
                    " str offset 0x%" DW_PR_XZEROS DW_PR_DUx
                    " %s\n",
                    line_number,
                    index,offset,
                    macro_string?macro_string:"<no-name-available>");
            }
            break;
            }
        case DW_MACRO_define_sup:
        case DW_MACRO_undef_sup: {
            lres = dwarf_get_macro_defundef(mcontext,
                k,
                &line_number,
                &index,
                &offset,
                &forms_count,
                &macro_string,
                &err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from sup dwarf_get_macro_defundef()",
                    lres,err);
            }
            if (glflags.gf_do_print_dwarf) {
                printf("  line %" DW_PR_DUu
                    " sup str offset 0x%" DW_PR_XZEROS DW_PR_DUx
                    " %s\n",
                    line_number,offset,
                    macro_string?sanitized(macro_string):"<no-name-available>");
            }
            break;
            }
        case DW_MACRO_start_file: {
            lres = dwarf_get_macro_startend_file(mcontext,
                k,&line_number,
                &index,
                &macro_string,&err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from  dwarf_get_macro_startend_file()",
                    lres,err);
            }
            if (glflags.gf_do_print_dwarf) {
                printf("  line %" DW_PR_DUu
                    " file number %" DW_PR_DUu
                    " %s\n",
                    line_number,
                    index,
                    macro_string?sanitized(macro_string):"<no-name-available>");
            }
            break;
            }
        case DW_MACRO_import: {
            Dwarf_Bool is_primary = FALSE;
            lres = dwarf_get_macro_import(mcontext,
                k,&offset,&err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from  dwarf_get_macro_import()",
                    lres,err);
            }
            add_macro_import(&macro_check_tree,
                is_primary,offset);
            if (glflags.gf_do_print_dwarf) {
                printf("  offset 0x%" DW_PR_XZEROS DW_PR_DUx "\n",offset);
            }
            break;
            }
        case DW_MACRO_import_sup: {
            lres = dwarf_get_macro_import(mcontext,
                k,&offset,&err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from  dwarf_get_macro_import()(sup)",
                    lres,err);
            }
            add_macro_import_sup(&macro_check_tree,offset);
            if (glflags.gf_do_print_dwarf) {
                printf("  sup_offset 0x%" DW_PR_XZEROS DW_PR_DUx "\n",offset);
            }
            break;
            }
        }
    }
}

extern void
print_macros_5style_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die,
    int by_offset, Dwarf_Unsigned offset)
{
    int lres = 0;
    Dwarf_Unsigned version = 0;
    Dwarf_Macro_Context macro_context = 0;
    Dwarf_Unsigned macro_unit_offset = 0;
    Dwarf_Unsigned number_of_ops = 0;
    Dwarf_Unsigned ops_total_byte_len = 0;
    Dwarf_Bool is_primary = TRUE;
    Dwarf_Error err = 0;
    Dwarf_Off dieprint_cu_goffset = 0;
    Dwarf_Off cudie_local_offset = 0;
    int atres = 0;

    glflags.current_section_id = DEBUG_MACRO;
    if(!by_offset) {
        lres = dwarf_get_macro_context(cu_die,
            &version,&macro_context,
            &macro_unit_offset,
            &number_of_ops,
            &ops_total_byte_len,
            &err);
        offset = macro_unit_offset;
    } else {
        lres = dwarf_get_macro_context_by_offset(cu_die,
            offset,
            &version,&macro_context,
            &number_of_ops,
            &ops_total_byte_len,
            &err);
        is_primary = FALSE;
    }
    if(lres == DW_DLV_NO_ENTRY) {
        return;
    }
    if(lres == DW_DLV_ERROR) {
        print_error(dbg,"Unable to dwarf_get_macro_context()",
            lres,err);
        return;
    }
    atres = dwarf_die_offsets(cu_die,&dieprint_cu_goffset,
        &cudie_local_offset,&err);
    DROP_ERROR_INSTANCE(dbg,atres,err);
    add_macro_import(&macro_check_tree,is_primary, offset);
    add_macro_area_len(&macro_check_tree,offset,ops_total_byte_len);

    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,".debug_macro",
            &truename,TRUE);

        if(!by_offset) {
            printf("\n%s: Macro info for a single cu\n",
                sanitized(esb_get_string(&truename)));
            print_source_intro(cu_die);
        } else {
            printf("\n%s: Macro info for imported CU at offset "
                "0x%" DW_PR_XZEROS DW_PR_DUx
                "\n",
                sanitized(esb_get_string(&truename)),
                offset);
        }
        esb_destructor(&truename);
    } else {
        /* We are checking, not printing. */
        Dwarf_Half tag = 0;
        int tres = dwarf_tag(cu_die, &tag, &err);
        if (tres != DW_DLV_OK) {
            /*  Something broken here. */
            print_error(dbg,"Unable to see CU DIE tag "
                "though we could see it earlier. Something broken.",
                tres,err);
            return;
        } else if (tag == DW_TAG_type_unit) {
            /*  Not checking since type units missing
                address or range in CU header. */
            return;
        }
    }
    if (glflags.gf_do_print_dwarf && glflags.verbose > 1) {
#if 0
        int errcount = 0;
#endif
        print_one_die(dbg, cu_die,
            dieprint_cu_goffset,
            /* print_information= */ 1,
            /* indent level */0,
            /* srcfiles= */ 0, /* cnt= */ 0,
            /* ignore_die_stack= */TRUE);
#if 0
        DWARF_CHECK_COUNT(lines_result,1);
        lres = dwarf_print_lines(cu_die, &err,&errcount);
        if (errcount > 0) {
            DWARF_ERROR_COUNT(lines_result,errcount);
            DWARF_CHECK_COUNT(lines_result,(errcount-1));
        }
        if (lres == DW_DLV_ERROR) {
            print_error(dbg, "dwarf_srclines details", lres, err);
        }
#endif
    }
    {
        Dwarf_Half lversion =0;
        Dwarf_Unsigned mac_offset =0;
        Dwarf_Unsigned mac_len =0;
        Dwarf_Unsigned mac_header_len =0;
        Dwarf_Unsigned line_offset =0;
        unsigned mflags = 0;
        Dwarf_Bool has_line_offset = FALSE;
        Dwarf_Bool has_offset_size_64 = FALSE;
        Dwarf_Bool has_operands_table = FALSE;
        Dwarf_Half opcode_count = 0;
        Dwarf_Half offset_size = 4;

        lres = dwarf_macro_context_head(macro_context,
            &lversion, &mac_offset,&mac_len,
            &mac_header_len,&mflags,&has_line_offset,
            &line_offset,
            &has_offset_size_64,&has_operands_table,
            &opcode_count,&err);
        if(lres == DW_DLV_NO_ENTRY) {
            /* Impossible */
            return;
        }
        if(lres == DW_DLV_ERROR) {
            print_error(dbg,"Call to dwarf_macro_context_head() failed",
                lres,err);
            return;
        }
        if (has_offset_size_64) {
            offset_size = 8;
        }
        if (glflags.gf_do_print_dwarf) {
            printf("  Macro version: %d\n",lversion);
        }
        if (glflags.verbose && glflags.gf_do_print_dwarf) {
            printf("  macro section offset 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                mac_offset);
            printf("  flags: 0x%x, line offset? %u offsetsize %u, "
                "operands_table? %u\n",
                mflags,has_line_offset,has_offset_size_64, has_operands_table);
            printf("  offset size 0x%x\n",offset_size);
            printf("  header length: 0x%" DW_PR_XZEROS DW_PR_DUx
                "  total length: 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                mac_header_len,mac_len);
            if (has_line_offset) {
                printf("  debug_line_offset: 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
                    line_offset);
            }
            if (has_operands_table) {
                Dwarf_Half i = 0;

                for( i = 0; i < opcode_count; ++i) {
                    Dwarf_Half opcode_num = 0;
                    Dwarf_Half operand_count = 0;
                    const Dwarf_Small *operand_array = 0;
                    Dwarf_Half j = 0;

                    lres = dwarf_macro_operands_table(macro_context,
                        i, &opcode_num, &operand_count,&operand_array,&err);
                    if (lres == DW_DLV_NO_ENTRY) {
                        dwarf_dealloc_macro_context(macro_context);
                        print_error(dbg,
                            "NO ENTRY? dwarf_macro_operands_table()",
                            lres,err);
                        return;
                    }
                    if (lres == DW_DLV_ERROR) {
                        dwarf_dealloc_macro_context(macro_context);
                        print_error(dbg,
                            "ERROR from  dwarf_macro_operands_table()",
                            lres,err);
                        return;
                    }
                    printf("  [%3u]  op: 0x%04x  %20s  operandcount: %u\n",
                        i,opcode_num,
                        get_MACRO_name(opcode_num, dwarf_names_print_on_error),
                        operand_count);
                    for (j = 0; j < operand_count; ++j) {
                        Dwarf_Small opnd = operand_array[j];
                        printf("    [%3u] 0x%04x %20s\n", j,opnd,
                            get_FORM_name(opnd, dwarf_names_print_on_error));
                    }
                }
            }
        }
        if (glflags.gf_do_print_dwarf) {
            printf("  MacroInformationEntries count: %" DW_PR_DUu
                ", bytes length: %" DW_PR_DUu "\n",
                number_of_ops,ops_total_byte_len);
        }
        print_macro_ops(dbg,macro_context,number_of_ops);
    }
#if 0
    if (check_lines && checking_this_compiler()) {
        DWARF_CHECK_COUNT(lines_result,1);
        dwarf_check_lineheader(cu_die,&line_errs);
        if (line_errs > 0) {
            DWARF_CHECK_ERROR_PRINT_CU();
            DWARF_ERROR_COUNT(lines_result,line_errs);
            DWARF_CHECK_COUNT(lines_result,(line_errs-1));
        }
    }
#endif
    dwarf_dealloc_macro_context(macro_context);
    macro_context = 0;
    mark_macro_offset_printed(&macro_check_tree,offset);
}

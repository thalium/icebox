/*
  Copyright (C) 2000,2002,2004,2005,2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2018 David Anderson. All Rights Reserved.
  Portions Copyright 2012 SN Systems Ltd. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2.1 of the GNU Lesser General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write the Free Software
  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston MA 02110-1301,
  USA.

*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dwarf_incl.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#include "dwarf_line.h"

#define PRINTING_DETAILS 1

static void
print_line_header(Dwarf_Debug dbg,
    Dwarf_Bool is_single_tab,
    Dwarf_Bool is_actuals_tab)
{
if (!is_single_tab) {
    /* Ugly indenting follows, it makes lines shorter to see them better. */
    if (is_actuals_tab) {
        dwarf_printf(dbg,"\nActuals Table\n");
dwarf_printf(dbg,
"                                                         be\n"
"                                                         ls\n"
"                                                         ce\n"
" section    op                                           kq\n"
" offset     code                address/index    row isa ??\n");
    return;
    } else {
        dwarf_printf(dbg,"\nLogicals Table\n");
dwarf_printf(dbg,
"                                                                              s pe\n"
"                                                                              tirp\n"
"                                                                              msoi\n"
" section          op                                                          tall\n"
" offset      row  code                address/indx fil lne col disc cntx subp ????\n");
    return;
    }
}
/* Single level table */
dwarf_printf(dbg,
"                                                         s b e p e i d\n"
"                                                         t l s r p s i\n"
"                                                         m c e o i a s\n"
" section    op                                       col t k q l l   c\n"
" offset     code               address     file line umn ? ? ? ? ? \n");
}

static void
print_line_detail(
    Dwarf_Debug dbg,
    const char *prefix,
    int opcode,
    unsigned curr_line,
    struct Dwarf_Line_Registers_s * regs,
    Dwarf_Bool is_single_table, Dwarf_Bool is_actuals_table)
{
    if(!is_single_table && is_actuals_table) {
        dwarf_printf(dbg,
            "%-15s %3d 0x%" DW_PR_XZEROS DW_PR_DUx
            "/%01u"
            " %5lu"  /* lr_line, really logical row */
            " %3d"   /* isa */
            "   %1d"
            "%1d\n",
            prefix,
            (int) opcode,
            (Dwarf_Unsigned) regs->lr_address,
            (unsigned) regs->lr_op_index,
            (unsigned long) regs->lr_line, /*logical row */
            regs->lr_isa,
            (int) regs->lr_basic_block,
            (int) regs->lr_end_sequence);
        return;
    }
    if(!is_single_table && !is_actuals_table) {
        dwarf_printf(dbg,
            "[%3d] "  /* row number */
            "%-15s %3d x%" DW_PR_XZEROS DW_PR_DUx "/%01u"
            " %2lu %4lu  %1lu",
            curr_line,
            prefix,
            (int) opcode,
            (Dwarf_Unsigned) regs->lr_address,
            (unsigned) regs->lr_op_index,
            (unsigned long) regs->lr_file,
            (unsigned long) regs->lr_line,
            (unsigned long) regs->lr_column);
        if (regs->lr_discriminator ||
            regs->lr_prologue_end ||
            regs->lr_epilogue_begin ||
            regs->lr_isa ||
            regs->lr_is_stmt ||
            regs->lr_call_context ||
            regs->lr_subprogram) {
            dwarf_printf(dbg,
                "   x%02" DW_PR_DUx , regs->lr_discriminator); /* DWARF4 */
            dwarf_printf(dbg,
                "  x%02" DW_PR_DUx , regs->lr_call_context); /* EXPERIMENTAL */
            dwarf_printf(dbg,
                "  x%02" DW_PR_DUx , regs->lr_subprogram); /* EXPERIMENTAL */
            dwarf_printf(dbg,
                "  %1d", (int) regs->lr_is_stmt);
            dwarf_printf(dbg,
                "%1d", (int) regs->lr_isa);
            dwarf_printf(dbg,
                "%1d", regs->lr_prologue_end); /* DWARF3 */
            dwarf_printf(dbg,
                "%1d", regs->lr_epilogue_begin); /* DWARF3 */
        }
        dwarf_printf(dbg, "\n");
        return;
    }
    /*  In the first quoted line below:
        3d looks better than 2d, but best to do that as separate
        change and test from two-level-line-tables.  */
    dwarf_printf(dbg,
        "%-15s %2d 0x%" DW_PR_XZEROS DW_PR_DUx " "
        "%2lu   %4lu %2lu   %1d %1d %1d",
        prefix,
        (int) opcode,
        (Dwarf_Unsigned) regs->lr_address,
        (unsigned long) regs->lr_file,
        (unsigned long) regs->lr_line,
        (unsigned long) regs->lr_column,
        (int) regs->lr_is_stmt,
        (int) regs->lr_basic_block,
        (int) regs->lr_end_sequence);
    if (regs->lr_discriminator ||
        regs->lr_prologue_end ||
        regs->lr_epilogue_begin ||
        regs->lr_isa) {
        dwarf_printf(dbg,
            " %1d", regs->lr_prologue_end); /* DWARF3 */
        dwarf_printf(dbg,
            " %1d", regs->lr_epilogue_begin); /* DWARF3 */
        dwarf_printf(dbg,
            " %1d", regs->lr_isa); /* DWARF3 */
        dwarf_printf(dbg,
            " 0x%" DW_PR_DUx , regs->lr_discriminator); /* DWARF4 */
    }
    dwarf_printf(dbg, "\n");
}


#include "dwarf_line_table_reader_common.h"

/* Not yet implemented, at least not usefully. FIXME */
void
_dwarf_print_line_context_record(UNUSEDARG Dwarf_Debug dbg,
    UNUSEDARG Dwarf_Line_Context line_context)
{
    return;
}

/*  return DW_DLV_OK if ok. else DW_DLV_NO_ENTRY or DW_DLV_ERROR
    If err_count_out is non-NULL, this is a special 'check'
    call.  */
static int
_dwarf_internal_printlines(Dwarf_Die die,
    Dwarf_Error * error,
    int * err_count_out,
    int only_line_header)
{
    /*  This pointer is used to scan the portion of the .debug_line
        section for the current cu. */
    Dwarf_Small *line_ptr = 0;
    Dwarf_Small *orig_line_ptr = 0;

    /*  Pointer to a DW_AT_stmt_list attribute in case it exists in the
        die. */
    Dwarf_Attribute stmt_list_attr = 0;

    /*  Pointer to DW_AT_comp_dir attribute in die. */
    Dwarf_Attribute comp_dir_attr = 0;

    /*  Pointer to name of compilation directory. */
    Dwarf_Small *comp_dir = NULL;

    /*  Offset into .debug_line specified by a DW_AT_stmt_list
        attribute. */
    Dwarf_Unsigned line_offset = 0;

    Dwarf_Sword i=0;
    Dwarf_Word u=0;

    /*  These variables are used to decode leb128 numbers. Leb128_num
        holds the decoded number, and leb128_length is its length in
        bytes. */
    Dwarf_Half attrform = 0;

    /*  In case there are wierd bytes 'after' the line table
        prologue this lets us print something. This is a gcc
        compiler bug and we expect the bytes count to be 12.  */
    Dwarf_Small* bogus_bytes_ptr = 0;
    Dwarf_Unsigned bogus_bytes_count = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned fission_offset = 0;


    /* The Dwarf_Debug this die belongs to. */
    Dwarf_Debug dbg=0;
    Dwarf_CU_Context cu_context = 0;
    Dwarf_Line_Context line_context = 0;
    int resattr = DW_DLV_ERROR;
    int lres =    DW_DLV_ERROR;
    int res  =    DW_DLV_ERROR;
    Dwarf_Small *line_ptr_actuals  = 0;
    Dwarf_Small *line_ptr_end = 0;
    Dwarf_Small *section_start = 0;

    /* ***** BEGIN CODE ***** */

    if (error != NULL) {
        *error = NULL;
    }

    CHECK_DIE(die, DW_DLV_ERROR);
    cu_context = die->di_cu_context;
    dbg = cu_context->cc_dbg;

    res = _dwarf_load_section(dbg, &dbg->de_debug_line,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (!dbg->de_debug_line.dss_size) {
        return (DW_DLV_NO_ENTRY);
    }

    address_size = _dwarf_get_address_size(dbg, die);
    resattr = dwarf_attr(die, DW_AT_stmt_list, &stmt_list_attr, error);
    if (resattr != DW_DLV_OK) {
        return resattr;
    }


    /*  The list of relevant FORMs is small.
        DW_FORM_data4, DW_FORM_data8, DW_FORM_sec_offset
    */
    lres = dwarf_whatform(stmt_list_attr,&attrform,error);
    if (lres != DW_DLV_OK) {
        return lres;
    }
    if (attrform != DW_FORM_data4 && attrform != DW_FORM_data8 &&
        attrform != DW_FORM_sec_offset ) {
        _dwarf_error(dbg, error, DW_DLE_LINE_OFFSET_BAD);
        return (DW_DLV_ERROR);
    }
    lres = dwarf_global_formref(stmt_list_attr, &line_offset, error);
    if (lres != DW_DLV_OK) {
        return lres;
    }

    if (line_offset >= dbg->de_debug_line.dss_size) {
        _dwarf_error(dbg, error, DW_DLE_LINE_OFFSET_BAD);
        return (DW_DLV_ERROR);
    }
    section_start =  dbg->de_debug_line.dss_data;
    {
        Dwarf_Unsigned fission_size = 0;
        int resfis = _dwarf_get_fission_addition_die(die, DW_SECT_LINE,
            &fission_offset,&fission_size,error);
        if(resfis != DW_DLV_OK) {
            return resfis;
        }
    }

    orig_line_ptr = section_start + line_offset + fission_offset;
    line_ptr = orig_line_ptr;
    dwarf_dealloc(dbg, stmt_list_attr, DW_DLA_ATTR);

    /*  If die has DW_AT_comp_dir attribute, get the string that names
        the compilation directory. */
    resattr = dwarf_attr(die, DW_AT_comp_dir, &comp_dir_attr, error);
    if (resattr == DW_DLV_ERROR) {
        return resattr;
    }
    if (resattr == DW_DLV_OK) {
        int cres = DW_DLV_ERROR;
        char *cdir = 0;

        cres = dwarf_formstring(comp_dir_attr, &cdir, error);
        if (cres == DW_DLV_ERROR) {
            return cres;
        } else if (cres == DW_DLV_OK) {
            comp_dir = (Dwarf_Small *) cdir;
        }
    }
    if (resattr == DW_DLV_OK) {
        dwarf_dealloc(dbg, comp_dir_attr, DW_DLA_ATTR);
    }
    line_context = (Dwarf_Line_Context)
        _dwarf_get_alloc(dbg, DW_DLA_LINE_CONTEXT, 1);
    if (line_context == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return (DW_DLV_ERROR);
    }
    {
        Dwarf_Small *newlinep = 0;
        int dres = _dwarf_read_line_table_header(dbg,
            cu_context,
            section_start,
            line_ptr,
            dbg->de_debug_line.dss_size,
            &newlinep,
            line_context,
            &bogus_bytes_ptr,
            &bogus_bytes_count,
            error,
            err_count_out);
        if (dres == DW_DLV_ERROR) {
            dwarf_srclines_dealloc_b(line_context);
            return dres;
        }
        if (dres == DW_DLV_NO_ENTRY) {
            dwarf_srclines_dealloc_b(line_context);
            return dres;
        }
        line_ptr_end = line_context->lc_line_ptr_end;
        line_ptr = newlinep;
        if (line_context->lc_actuals_table_offset > 0) {
            line_ptr_actuals = line_context->lc_line_prologue_start +
                line_context->lc_actuals_table_offset;
        }
    }
    line_context->lc_compilation_directory = comp_dir;
    if (only_line_header) {
        /* Just checking for header errors, nothing more here.*/
        dwarf_srclines_dealloc_b(line_context);
        return DW_DLV_OK;
    }

    dwarf_printf(dbg,
        "total line info length %ld bytes,"
        " line offset 0x%" DW_PR_XZEROS DW_PR_DUx
        " %" DW_PR_DUu "\n",
        (long) line_context->lc_total_length,
        line_context->lc_section_offset,
        line_context->lc_section_offset);
    if (line_context->lc_version_number <= DW_LINE_VERSION5) {
        dwarf_printf(dbg,
            "line table version %d\n",(int) line_context->lc_version_number);
    } else {
        dwarf_printf(dbg,
            "line table version 0x%x\n",(int) line_context->lc_version_number);
    }
    dwarf_printf(dbg,
        "line table length field length %d prologue length %d\n",
        (int)line_context->lc_length_field_length,
        (int)line_context->lc_prologue_length);
    dwarf_printf(dbg,
        "compilation_directory %s\n",
        comp_dir ? ((char *) comp_dir) : "");

    dwarf_printf(dbg,
        "  min instruction length %d\n",
        (int) line_context->lc_minimum_instruction_length);
    if (line_context->lc_version_number == EXPERIMENTAL_LINE_TABLES_VERSION) {
        dwarf_printf(dbg, "  actuals table offset "
            "0x%" DW_PR_XZEROS DW_PR_DUx
            " logicals table offset "
            "0x%" DW_PR_XZEROS DW_PR_DUx "\n",
            line_context->lc_actuals_table_offset,
            line_context->lc_logicals_table_offset);
    }
    if (line_context->lc_version_number == DW_LINE_VERSION5) {
        dwarf_printf(dbg,
            "  segment selector size %d\n",
            (int) line_context->lc_segment_selector_size);
        dwarf_printf(dbg,
            "  address    size       %d\n",
            (int) line_context->lc_address_size);
    }
    dwarf_printf(dbg,
        "  default is stmt        %d\n",(int)line_context->lc_default_is_stmt);
    dwarf_printf(dbg,
        "  line base              %d\n",(int)line_context->lc_line_base);
    dwarf_printf(dbg,
        "  line_range             %d\n",(int)line_context->lc_line_range);
    dwarf_printf(dbg,
        "  opcode base            %d\n",(int)line_context->lc_opcode_base);
    dwarf_printf(dbg,
        "  standard opcode count  %d\n",(int)line_context->lc_std_op_count);

    for (i = 1; i < line_context->lc_opcode_base; i++) {
        dwarf_printf(dbg,
            "  opcode[%2d] length  %d\n", (int) i,
            (int) line_context->lc_opcode_length_table[i - 1]);
    }
    dwarf_printf(dbg,
        "  include directories count %d\n",
        (int) line_context->lc_include_directories_count);
    for (u = 0; u < line_context->lc_include_directories_count; ++u) {
        dwarf_printf(dbg,
            "  include dir[%u] %s\n",
            (int) u, line_context->lc_include_directories[u]);
    }
    dwarf_printf(dbg,
        "  files count            %d\n",
        (int) line_context->lc_file_entry_count);

    if (line_context->lc_file_entry_count) {
        Dwarf_File_Entry fe = line_context->lc_file_entries;
        Dwarf_File_Entry fe2 = fe;
        unsigned fiu = 0;

        for (fiu = 0 ; fe2 ; fe2 = fe->fi_next,++fiu ) {
            Dwarf_Unsigned tlm2 = 0;
            Dwarf_Unsigned di = 0;
            Dwarf_Unsigned fl = 0;
            unsigned filenum = 0;

            fe = fe2;
            tlm2 = fe->fi_time_last_mod;
            di = fe->fi_dir_index;
            fl = fe->fi_file_length;
            filenum = fiu+1; /* Assuming DWARF2,3,4 */
            if (line_context->lc_version_number == DW_LINE_VERSION5) {
                /* index starts 0 for DWARF5, show 0 base. */
                filenum = fiu;
            }

            dwarf_printf(dbg,
                "  file[%u]  %s (file-number: %u) \n",
                (unsigned) fiu, (char *) fe->fi_file_name,
                (unsigned)(filenum));
            dwarf_printf(dbg,
                "    dir index %d\n", (int) di);
            {
                time_t tt = (time_t) tlm2;

                /* ctime supplies newline */
                dwarf_printf(dbg,
                    "    last time 0x%x %s",
                    (unsigned) tlm2, ctime(&tt));
                }
            dwarf_printf(dbg,
                "    file length %ld 0x%lx\n",
                (long) fl, (unsigned long) fl);
            if (fe->fi_md5_present) {
                char *c = (char *)&fe->fi_md5_value;
                char *end = c+sizeof(fe->fi_md5_value);
                dwarf_printf(dbg, "    file md5 value 0x");
                while(c < end) {
                    dwarf_printf(dbg,"%02x",0xff&*c);
                    ++c;
                }
                dwarf_printf(dbg,"\n");
            }
        }
    }

    if (line_context->lc_version_number == EXPERIMENTAL_LINE_TABLES_VERSION) {
        /*  Print the subprograms list. */
        Dwarf_Unsigned count = line_context->lc_subprogs_count;
        Dwarf_Unsigned exu = 0;
        Dwarf_Subprog_Entry sub = line_context->lc_subprogs;
        dwarf_printf(dbg,"  subprograms count"
            " %" DW_PR_DUu "\n",count);
        if (count > 0) {
            dwarf_printf(dbg,"    indx  file   line   name\n");
        }
        for (exu = 0 ; exu < count ; exu++,sub++) {
            dwarf_printf(dbg,"    [%2" DW_PR_DUu "] %4" DW_PR_DUu
                "    %4" DW_PR_DUu " %s\n",
                exu+1,
                sub->ds_decl_file,
                sub->ds_decl_line,
                sub->ds_subprog_name);
        }
    }
    {
        Dwarf_Unsigned offset = 0;
        if (bogus_bytes_count > 0) {
            Dwarf_Unsigned wcount = bogus_bytes_count;
            Dwarf_Unsigned boffset = bogus_bytes_ptr - section_start;
            dwarf_printf(dbg,
                "*** DWARF CHECK: the line table prologue  header_length "
                " is %" DW_PR_DUu " too high, we pretend it is smaller."
                "Section offset: 0x%" DW_PR_XZEROS DW_PR_DUx
                " (%" DW_PR_DUu ") ***\n",
                wcount, boffset,boffset);
            *err_count_out += 1;
        }
        offset = line_ptr - section_start;
        dwarf_printf(dbg,
            "  statement prog offset in section: 0x%"
            DW_PR_XZEROS DW_PR_DUx " (%" DW_PR_DUu ")\n",
            offset, offset);
    }

    {
        Dwarf_Bool doaddrs = false;
        Dwarf_Bool dolines = true;

        _dwarf_print_line_context_record(dbg,line_context);
        if (!line_ptr_actuals) {
            /* Normal single level line table. */

            Dwarf_Bool is_single_table = true;
            Dwarf_Bool is_actuals_table = false;
            print_line_header(dbg, is_single_table, is_actuals_table);
            res = read_line_table_program(dbg,
                line_ptr, line_ptr_end, orig_line_ptr,
                section_start,
                line_context,
                address_size, doaddrs, dolines,
                is_single_table,
                is_actuals_table,
                error,
                err_count_out);
            if (res != DW_DLV_OK) {
                dwarf_srclines_dealloc_b(line_context);
                return res;
            }
        } else {
            Dwarf_Bool is_single_table = false;
            Dwarf_Bool is_actuals_table = false;
            if (line_context->lc_version_number !=
                EXPERIMENTAL_LINE_TABLES_VERSION) {
                dwarf_srclines_dealloc_b(line_context);
                _dwarf_error(dbg, error, DW_DLE_VERSION_STAMP_ERROR);
                return (DW_DLV_ERROR);
            }
            /* Read Logicals */
            print_line_header(dbg, is_single_table, is_actuals_table);
            res = read_line_table_program(dbg,
                line_ptr, line_ptr_actuals, orig_line_ptr,
                section_start,
                line_context,
                address_size, doaddrs, dolines,
                is_single_table,
                is_actuals_table,
                error,err_count_out);
            if (res != DW_DLV_OK) {
                dwarf_srclines_dealloc_b(line_context);
                return res;
            }
            if (line_context->lc_actuals_table_offset > 0) {
                is_actuals_table = true;
                /* Read Actuals */

                print_line_header(dbg, is_single_table, is_actuals_table);
                res = read_line_table_program(dbg,
                    line_ptr_actuals, line_ptr_end, orig_line_ptr,
                    section_start,
                    line_context,
                    address_size, doaddrs, dolines,
                    is_single_table,
                    is_actuals_table,
                    error,
                    err_count_out);
                if (res != DW_DLV_OK) {
                    dwarf_srclines_dealloc_b(line_context);
                    return res;
                }
            }
        }
    }
    dwarf_srclines_dealloc_b(line_context);
    return DW_DLV_OK;
}



/*  This is support for dwarfdump: making it possible
    for clients wanting line detail info on stdout
    to get that detail without including internal libdwarf
    header information.
    Caller passes in compilation unit DIE.
    The _dwarf_ version is obsolete (though supported for
    compatibility).
    The dwarf_ version is preferred.
    The functions are intentionally identical: having
    _dwarf_print_lines call dwarf_print_lines might
    better emphasize they are intentionally identical, but
    that seemed slightly silly given how short the functions are.
    Interface adds error_count (output value) February 2009.

    These *print_lines() functions print two-level tables in full
    even when the user is not asking for both (ie, when
    the caller asked for dwarf_srclines().
    It was an accident, but after a short reflection
    this seems like a good idea for -vvv. */
int
dwarf_print_lines(Dwarf_Die die, Dwarf_Error * error,int *error_count)
{
    int only_line_header = 0;
    int res = _dwarf_internal_printlines(die, error,
        error_count,
        only_line_header);
    return res;
}
int
_dwarf_print_lines(Dwarf_Die die, Dwarf_Error * error)
{
    int only_line_header = 0;
    int err_count = 0;
    int res = _dwarf_internal_printlines(die, error,
        &err_count,
        only_line_header);
    /* No way to get error count back in this interface */
    return res;
}

/* The check is in case we are not printing full line data,
   this gets some of the issues noted with .debug_line,
   but not all. Call dwarf_print_lines() to get all issues.
   Intended for apps like dwarfdump.
*/
void
dwarf_check_lineheader(Dwarf_Die die, int *err_count_out)
{
    Dwarf_Error err;
    int only_line_header = 1;
    _dwarf_internal_printlines(die, &err,err_count_out,
        only_line_header);
    return;
}

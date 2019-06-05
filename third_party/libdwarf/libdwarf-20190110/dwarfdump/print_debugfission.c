/*
  Copyright 2014-2014 David Anderson. All rights reserved.

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
#include "sanitized.h"
#include "print_sections.h"

#define TRUE 1
#define FALSE 0

static int
hashval_zero(Dwarf_Sig8 *val)
{
    unsigned u = 0;

    for(u=0 ; u < sizeof(Dwarf_Sig8);++u) {
        if (val->signature[u]) {
            return FALSE;
        }
    }
    return TRUE;
}

extern void
print_debugfission_index(Dwarf_Debug dbg,const char *type)
{
    int res = 0;
    Dwarf_Xu_Index_Header xuhdr = 0;
    Dwarf_Unsigned version_number = 0;
    Dwarf_Unsigned offsets_count = 0;
    Dwarf_Unsigned units_count = 0;
    Dwarf_Unsigned hash_slots_count = 0;
    Dwarf_Error fierr = 0;
    const char * section_name = 0;
    const char * section_type2 = 0;
    const char * section_name2 = 0;
    int is_cu = !strcmp(type,"cu")?TRUE:FALSE;

    res = dwarf_get_xu_index_header(dbg,
        type,
        &xuhdr,
        &version_number,
        &offsets_count,
        &units_count,
        &hash_slots_count,
        &section_name,
        &fierr);
    if (res == DW_DLV_NO_ENTRY) {
        /* This applies to most object files. */
        return;
    }
    if (res == DW_DLV_ERROR) {
        /* Odd. FIXME */
        return;
    }
    res = dwarf_get_xu_index_section_type(xuhdr,
        &section_type2,
        &section_name2,
        &fierr);
    if (res == DW_DLV_NO_ENTRY) {
        /* Impossible. */
        print_error(dbg,"dwarf_get_xu_index_section_type",
            DW_DLE_XU_IMPOSSIBLE_ERROR,fierr);
        dwarf_xu_header_free(xuhdr);
        return;
    }
    if (res == DW_DLV_ERROR) {
        /* Impossible. FIXME */
        print_error(dbg,"dwarf_get_xu_index_section_type", res,fierr);
        dwarf_xu_header_free(xuhdr);
        return;
    }
    if (strcmp(section_type2,type)) {
        print_error(dbg,"dwarf_get_xu_index_section_type",
            DW_DLE_XU_IMPOSSIBLE_ERROR,fierr);
        dwarf_xu_header_free(xuhdr);
        return;
    }
    if(!section_name || !*section_name) {
        section_name = (is_cu?".debug_cu_index":".debug_tu_index");
    }
    {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,section_name,
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
    printf("  Version:           %" DW_PR_DUu "\n",
        version_number);
    printf("  Number of columns: %" DW_PR_DUu "\n",
        offsets_count);
    printf("  number of entries: %" DW_PR_DUu  "\n",
        units_count);
    printf("  Number of slots:   %" DW_PR_DUu "\n",
        hash_slots_count);

    if (hash_slots_count > 0) {
        printf("\n");
        printf("           hash               index\n");
    }

    {
        Dwarf_Unsigned h = 0;
        for( h = 0; h < hash_slots_count; h++) {
            Dwarf_Sig8 hashval;
            Dwarf_Unsigned index = 0;
            Dwarf_Unsigned col = 0;
            struct esb_s hashhexstring;

            esb_constructor(&hashhexstring);
            memset(&hashval,0,sizeof(hashval));
            res = dwarf_get_xu_hash_entry(xuhdr,h,
                &hashval,&index,&fierr);
            if (res == DW_DLV_ERROR) {
                print_error(dbg,"dwarf_get_xu_hash_entry",res,fierr);
                dwarf_xu_header_free(xuhdr);
                esb_destructor(&hashhexstring);
                return;
            } else if (res == DW_DLV_NO_ENTRY) {
                /* Impossible */
                printf("  [%4" DW_PR_DUu "]  "
                    "dwarf_get_xu_hash_entry impossible return code: "
                    "No entry?\n",
                    h);
                dwarf_xu_header_free(xuhdr);
                esb_destructor(&hashhexstring);
                return;
            } else if (hashval_zero(&hashval) && index == 0 ) {
                /* An unused hash slot, we do not print them */
                continue;
            }
            format_sig8_string(&hashval,&hashhexstring);
            printf("  [%4" DW_PR_DUu "] %s"
                " %8" DW_PR_DUu  "\n",
                h,
                esb_get_string(&hashhexstring),
                index);
            esb_destructor(&hashhexstring);
            printf("      col              section   "
                "offset                size\n");
            for (col = 0; col < offsets_count; col++) {
                Dwarf_Unsigned off = 0;
                Dwarf_Unsigned len = 0;
                const char * name = 0;
                Dwarf_Unsigned num = 0;
                res = dwarf_get_xu_section_names(xuhdr,
                    col,&num,&name,&fierr);
                if (res != DW_DLV_OK) {
                    print_error(dbg,"dwarf_get_xu_section_names",res,fierr);
                    dwarf_xu_header_free(xuhdr);
                    return;
                }
                res = dwarf_get_xu_section_offset(xuhdr,
                    index,col,&off,&len,&fierr);
                if (res != DW_DLV_OK) {
                    print_error(dbg,"dwarf_get_xu_section_offset",res,fierr);
                    dwarf_xu_header_free(xuhdr);
                    return;
                }
                printf("    [,%2" DW_PR_DUu "] %20s "
                    "0x%" DW_PR_XZEROS DW_PR_DUx
                    " (%8" DW_PR_DUu ") "
                    "0x%" DW_PR_XZEROS DW_PR_DUx
                    " (%8" DW_PR_DUu ")\n",
                    col,name,
                    off,off,
                    len,len);
            }
        }
    }
    dwarf_xu_header_free(xuhdr);
}

/*
    Copyright 2018-2019 David Anderson. All rights reserved.

    This program is free software; you can redistribute it
    and/or modify it
    under the terms of version 2 of the GNU General Public License as
    published by the Free Software Foundation.

    This program is distributed in the hope that it would be
    useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Further, this software is distributed without any warranty
    that it is free of the rightful claim of any third person
    regarding infringement or the like.  Any license provided
    herein, whether implied or otherwise, applies only to
    this software file.  Patent licenses, if any, provided
    herein do not apply to combinations of this program with other
    software, or any other product whatsoever.

    You should have received a copy of the GNU General Public
    License along with this program; if not, write the
    Free Software Foundation, Inc., 51 Franklin Street -
    Fifth Floor, Boston MA 02110-1301, USA.

*/

#include "globals.h"
#include "naming.h"
#include "esb.h"
#include "esb_using_functions.h"
#include "sanitized.h"

/* print data in .debug_str_offsets.
   There is no guarantee this will work because
   the DWARF5 standard is silent about
   whether arbitrary non-zero bytes, or odd
   alignments, or unused data spaces  are allowed
   in the section
*/
void
print_str_offsets_section(Dwarf_Debug dbg)
{
    int res = 0;
    Dwarf_Str_Offsets_Table sot = 0;
    Dwarf_Unsigned wasted_byte_count = 0;
    Dwarf_Unsigned table_count = 0;
    Dwarf_Error error = 0;
    Dwarf_Unsigned tabnum = 0;

    res = dwarf_open_str_offsets_table_access(dbg, &sot,&error);
    if(res == DW_DLV_NO_ENTRY) {
        /* No such table */
        return;
    }
    if(res == DW_DLV_ERROR) {
        print_error_and_continue(dbg,
            "dwarf_open_str_offsets_table_access",
            res, error);
        return;
    }
    for(;; ++tabnum) {
        Dwarf_Unsigned unit_length =0;
        Dwarf_Unsigned unit_length_offset =0;
        Dwarf_Unsigned table_start_offset =0;
        Dwarf_Half     entry_size = 0;
        Dwarf_Half     version =0;
        Dwarf_Half     padding =0;
        Dwarf_Unsigned table_value_count =0;
        Dwarf_Unsigned i = 0;
        Dwarf_Unsigned table_entry_value = 0;
        unsigned rowlim = 4;
        unsigned count_in_row = 0;

        res = dwarf_next_str_offsets_table(sot,
            &unit_length, &unit_length_offset,
            &table_start_offset,
            &entry_size,&version,&padding,
            &table_value_count,&error);
        if (res == DW_DLV_NO_ENTRY) {
            /* We have dealt with all tables */
            break;
        }
        if (res == DW_DLV_ERROR) {
            print_error_and_continue(dbg,
                "dwarf_next_str_offsets_table", res,error);
            return;
        }
        if (tabnum == 0) {
            struct esb_s truename;
            char buf[DWARF_SECNAME_BUFFER_SIZE];

            esb_constructor_fixed(&truename,buf,sizeof(buf));
            get_true_section_name(dbg,".debug_str_offsets",
                &truename,TRUE);
            printf("\n%s\n",sanitized(esb_get_string(&truename)));
            esb_destructor(&truename);
        } else {
            printf("\n");
        }
        printf(" table %" DW_PR_DUu "\n",tabnum);
        printf(" tableheader 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
            unit_length_offset);
        printf(" arrayoffset 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
            table_start_offset);
        printf(" unit length 0x%" DW_PR_XZEROS DW_PR_DUx "\n",
            unit_length);
        printf(" entry size  %u\n",entry_size);
        printf(" version     %u\n",version);
        if (padding) {
            printf("Error: padding is non-zero. Something is wrong.\n");
        }
        printf(" padding     0x%x\n",padding);
        printf(" arraysize   %" DW_PR_DUu "\n",table_value_count);

        /*  Lets print 4 per row. */
        count_in_row = 0;
        for (i=0; i < table_value_count; ++i) {

            res = dwarf_str_offsets_value_by_index(sot,i,
                &table_entry_value,&error);
            if (res != DW_DLV_OK) {
                print_error_and_continue(dbg,
                    "dwarf_next_str_offsets_table", res,error);
                return;
            }
            if (!count_in_row) {
                printf(" Entry [%4" DW_PR_DUu "]: ",i);
            }
            printf(" 0x%" DW_PR_XZEROS DW_PR_DUx ,
                table_entry_value);
            ++count_in_row;
            if( count_in_row < rowlim) {
                continue;
            }
            printf("\n");
            count_in_row = 0;
        }
        if (count_in_row) {
            printf("\n");
        }
        res = dwarf_str_offsets_statistics(sot,&wasted_byte_count,
            &table_count,&error);
        if (res == DW_DLV_OK) {
            printf(" wasted      %" DW_PR_DUu " bytes\n",
                wasted_byte_count);
        }
    }
    if (wasted_byte_count) {
        res = dwarf_str_offsets_statistics(sot,&wasted_byte_count,
            &table_count,&error);
        if (res == DW_DLV_OK) {
            printf(" finalwasted %" DW_PR_DUu " bytes\n",
                wasted_byte_count);
        } else {
            print_error_and_continue(dbg,
                "dwarf_open_str_offsets_statistics",
                res, error);
            return;
        }
    }
    res = dwarf_close_str_offsets_table_access(sot,&error);
    if (res != DW_DLV_OK) {
        print_error_and_continue(dbg,
            "dwarf_close_str_offsets_table_access",
            res, error);
        return;
    }
    sot = 0;
}

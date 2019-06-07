/*
  Copyright 2017-2018 David Anderson. All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of version 2 of the GNU General
  Public License as published by the Free Software Foundation.

  This program is distributed in the hope that it would be
  useful, but WITHOUT ANY WARRANTY; without even the implied
  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  Further, this software is distributed without any warranty
  that it is free of the rightful claim of any third person
  regarding infringement or the like.  Any license provided
  herein, whether implied or otherwise, applies only to this
  software file.  Patent licenses, if any, provided herein
  do not apply to combinations of this program with other
  software, or any other product whatsoever.

  You should have received a copy of the GNU General Public
  License along with this program; if not, write the Free
  Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
  Boston MA 02110-1301, USA.

*/

#include "globals.h"
#include "naming.h"
#include "sanitized.h"
#include "esb.h"
#include "esb_using_functions.h"

extern void
print_debug_names(Dwarf_Debug dbg)
{
    Dwarf_Dnames_Head dnhead = 0;
    Dwarf_Unsigned dn_count = 0;
    Dwarf_Unsigned dnindex = 0;
    Dwarf_Error error = 0;
    int res = 0;

    if(!dbg) {
        printf("Cannot print .debug_names, no Dwarf_Debug passed in");
        printf("dwarfdump giving up. exit.\n");
        exit(1);
    }
    glflags.current_section_id = DEBUG_NAMES;


    /*  Only print anything if we know it has debug names
        present. And for now there is none. FIXME. */
    res = dwarf_debugnames_header(dbg,&dnhead,&dn_count,&error);
    if (res == DW_DLV_NO_ENTRY) {
        return;
    }
    if (res == DW_DLV_ERROR) {
        const char *msg = "Section .debug_names is not openable";
        print_error(dbg,msg, res, error);
        return;
    }
    /* Do nothing if not printing. */
    if (glflags.gf_do_print_dwarf) {
        const char * section_name = ".debug_names";
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,section_name,
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
    if (glflags.gf_do_print_dwarf) {
        printf("names tables: %" DW_PR_DUu "\n",dn_count);
    }
    for (  ; dnindex < dn_count; ++dnindex) {
        if (glflags.gf_do_print_dwarf) {
            printf("names table %" DW_PR_DUu "\n",dnindex);
        }
    }
    dwarf_dealloc(dbg,dnhead,DW_DLA_DNAMES_HEAD);
    return;
}

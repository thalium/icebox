/*
    Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
    Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
    Portions Copyright 2009-2010 SN Systems Ltd. All rights reserved.
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
#include "print_frames.h"
#include "sanitized.h"

/*  Get all the data in .debug_varnames
    (an SGI extension)
    For a long time erroneously called .debug_static_vars bere. */
extern void
print_static_vars(Dwarf_Debug dbg)
{
    Dwarf_Var *globbuf = NULL;
    Dwarf_Signed count = 0;
    int res = 0;
    Dwarf_Error err = 0;
    struct esb_s truename;
    char buf[DWARF_SECNAME_BUFFER_SIZE];
    struct esb_s sanitname;

    glflags.current_section_id = DEBUG_STATIC_VARS;

    esb_constructor_fixed(&truename,buf,sizeof(buf));
    get_true_section_name(dbg,".debug_varnames",
        &truename,TRUE);
    {
        esb_constructor(&sanitname);
        /*  Sanitized cannot be safely reused,there is a static buffer,
            so we make a safe copy. */
        esb_append(&sanitname,sanitized(esb_get_string(&truename)));
    }
    esb_destructor(&truename);
    if (glflags.verbose) {
        /* For best testing! */
        res = dwarf_return_empty_pubnames(dbg,1,&err);
        if (res != DW_DLV_OK) {
            printf("FAIL: Erroneous libdwarf call "
                "of dwarf_return_empty_pubnames: Fix dwarfdump");
            return;
        }
    }
    res = dwarf_get_vars(dbg, &globbuf, &count, &err);
    if (res == DW_DLV_ERROR) {
        print_error(dbg, "dwarf_get_vars", res, err);
    } else if (res == DW_DLV_NO_ENTRY) {
        /* no static vars */
        esb_destructor(&sanitname);
        dwarf_return_empty_pubnames(dbg,0,&err);
        return;
    } else {
        if (glflags.gf_do_print_dwarf && count > 0) {
            /* SGI specific so only mention if present. */
            printf("\n%s\n",esb_get_string(&sanitname));
        }
        print_all_pubnames_style_records(dbg,
            "static-var",
            esb_get_string(&sanitname),
            (Dwarf_Global *)globbuf, count,
            &err);
        esb_destructor(&sanitname);
        dwarf_vars_dealloc(dbg, globbuf, count);
    }
    dwarf_return_empty_pubnames(dbg,0,&err);
    esb_destructor(&sanitname);
}   /* print_static_vars */

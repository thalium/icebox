/*
  Copyright 2015-2016 David Anderson. All rights reserved.

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
#ifndef MACROCHECK_H
#define MACROCHECK_H

struct Macrocheck_Map_Entry_s {
    Dwarf_Unsigned mp_key; /* Key is offset */
    Dwarf_Unsigned mp_len; /* len in bytes off this macro set */

    /*  We count number of uses. More than 1 primary is an error.
        Both primary and secondary is ok or error?  */
    Dwarf_Unsigned mp_refcount_primary;
    Dwarf_Unsigned mp_refcount_secondary;

    /* So we go through each one just once. */
    Dwarf_Bool     mp_printed;
};

void add_macro_import(void **base,Dwarf_Bool is_primary,
    Dwarf_Unsigned offset);
void add_macro_import_sup(void **base,Dwarf_Unsigned offset);
void add_macro_area_len(void **base, Dwarf_Unsigned offset,
    Dwarf_Unsigned len);

int get_next_unprinted_macro_offset(void **base, Dwarf_Unsigned * off);
void mark_macro_offset_printed(void **base, Dwarf_Unsigned offset);

void print_macro_statistics(const char *name,void **basep,
    Dwarf_Unsigned section_size);
void clear_macro_statistics(void **basep);
#endif /* MACROCHECK_H */

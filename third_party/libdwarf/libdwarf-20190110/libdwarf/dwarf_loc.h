/*
  Copyright (C) 2000, 2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2015-2015 David Anderson. All Rights Reserved.

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

typedef struct Dwarf_Loc_Chain_s *Dwarf_Loc_Chain;
struct Dwarf_Loc_Chain_s {
    Dwarf_Small lc_atom;
    Dwarf_Unsigned lc_number;
    Dwarf_Unsigned lc_number2;
    Dwarf_Unsigned lc_number3;
    Dwarf_Unsigned lc_offset;
    Dwarf_Unsigned lc_opnumber;
    Dwarf_Loc_Chain lc_next;
};


/* Contains info on an uninterpreted block of data
   Used with certain frame information functions.
*/
typedef struct {
    Dwarf_Unsigned  bl_len;         /* length of block bl_data points at */
    Dwarf_Ptr       bl_data;        /* uninterpreted data */

    /*  0 if location description,
        1 if .debug_info loclist,
        2 if .debug_info.dwo split dwarf loclist. */
    Dwarf_Small     bl_from_loclist;

    /* Section (not CU) offset which 'data' comes from. */
    Dwarf_Unsigned  bl_section_offset;

    /*  Section offset where the location description itself starts.
        So a few bytes lower than bl_section_offset */
    Dwarf_Unsigned  bl_locdesc_offset;
} Dwarf_Block_c;

/* Location record. Records up to 3 operand values.
   For DWARF5 ops with a 1 byte size and then a block
   of data of that size we the size in an operand
   and follow that with the next operand as a
   pointer to the block. The pointer is inserted
   via  cast, so an ugly hack.
   This struct is opaque. Not visible to callers.
*/
struct Dwarf_Loc_c_s {
    Dwarf_Small     lr_atom;        /* Location operation */

    Dwarf_Unsigned  lr_number;      /* First operand */

    /*  Second operand.
        For OP_bregx, OP_bit_piece, OP_[GNU_]const_type,
        OP_[GNU_]deref_type, OP_[GNU_]entry_value, OP_implicit_value,
        OP_[GNU_]implicit_pointer, OP_[GNU_]regval_type,
        OP_xderef_type,  */
    Dwarf_Unsigned  lr_number2;

    /*  Third Operand.
        For OP_[GNU_]const type, pointer to
        block of length 'lr_number2' */
    Dwarf_Unsigned  lr_number3;

    /*  The number assigned. 0 to the number-of-ops - 1 in
        the expression we are expanding. */
    Dwarf_Unsigned  lr_opnumber;
    Dwarf_Unsigned  lr_offset;      /* offset in locexpr for OP_BRA etc */
    Dwarf_Loc_c     lr_next;        /* When a list is useful. */
};

/* Location description DWARF 2,3,4,5
   Adds the DW_LLE value (new in DWARF5).
   This struct is opaque. Not visible to callers. */
struct Dwarf_Locdesc_c_s {
    /*  The DW_LLE value of the entry.  Synthesized
        by libdwarf in a non-split-dwarf loclist,
        recorded in a split dwarf loclist. */
    Dwarf_Small     ld_lle_value;

    /*  Beginning of active range. This is actually an offset
        of an applicable base address, not a pc value.  */
    Dwarf_Addr      ld_lopc;

    /*  End of active range. This is actually an offset
        of an applicable base address, or a length, never a pc value.  */
    Dwarf_Addr      ld_hipc;        /* end of active range */

    /* count of struct Dwarf_Loc_c_s in array. */
    Dwarf_Half      ld_cents;
    /* pointer to array of struct Dwarf_Loc_c_s*/
    Dwarf_Loc_c     ld_s;

    Dwarf_Small     ld_from_loclist;

    /* Section (not CU) offset where loc-expr begins*/
    Dwarf_Unsigned  ld_section_offset;

    /* Section (not CU) offset where location descr begins*/
    Dwarf_Unsigned  ld_locdesc_offset;

    /* Pointer to our header (in which we are located). */
    Dwarf_Loc_Head_c  ld_loclist_head;
};

/*  A 'header' to the loclist and  the
    location description(s)  attached to an attribute.
    This struct is opaque. Not visible to callers. */
struct Dwarf_Loc_Head_c_s {

    /*  The array (1 or more entries) of
        struct Loc_Desc_c_s
        If 1 it may really be a locexpr */
    Dwarf_Locdesc_c   ll_locdesc;
    /*  Entry count of the ll_locdesc array.  */
    Dwarf_Unsigned    ll_locdesc_count;

    /*  0 if locexpr (in which case ll_locdesc_count is 1),
        1 if non-split (dwarf 2,3,4) .debug_loc,
        2 if split-dwarf .debug_loc.dwo */
    Dwarf_Small       ll_from_loclist;

    /*  The CU Context of this loclist or locexpr. */
    Dwarf_CU_Context ll_context;

    Dwarf_Debug      ll_dbg;
};

int _dwarf_loc_block_sanity_check(Dwarf_Debug dbg,
    Dwarf_Block_c *loc_block,Dwarf_Error*error);

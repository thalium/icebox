/*
   Copyright (C) 2000 Silicon Graphics, Inc.  All Rights Reserved.
   Portions Copyright (C) 2008-2011  David Anderson. All Rights Reserved.

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




/*  In a given CU, one of these is (eventually) set up
    for every abbreviation we need to find (and for all.
    those ealier in the abbreviations for that CU).
    So we don't want elements needlessly big.
*/
struct Dwarf_Abbrev_s {
    /*  No TAG should exceed DW_TAG_hi_user, 0xffff, but
        we do allow a larger value here. */
    Dwarf_Word dab_tag;
    /*  Abbreviations are numbered (normally sequentially from
        1 and so 16 bits is not enough!  */
    Dwarf_Word dab_code;
    Dwarf_Small dab_has_child;
    Dwarf_Byte_Ptr dab_abbrev_ptr;
    Dwarf_Debug dab_dbg;

    /* Section global offset of the abbrev. */
    Dwarf_Off    dab_goffset;
    Dwarf_Off    dab_count;
};

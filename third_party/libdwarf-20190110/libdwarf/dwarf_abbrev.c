/*
  Copyright (C) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2009-2018 David Anderson. All Rights Reserved.

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
#include "dwarf_incl.h"
#include "dwarf_abbrev.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"

/*  This is used to print a .debug_abbrev section without
    knowing about the DIEs that use the abbrevs.

    dwarf_get_abbrev() and,
    in dwarf_util.c,  _dwarf_get_abbrev_for_code()


    When we have a simple .o
    there is at least a hope of iterating through
    the abbrevs meaningfully without knowing
    a CU context.

    This often fails or gets incorrect info
    because there is no guarantee the .debug_abbrev
    section is free of garbage bytes.

    In an object with multiple CU/TUs the
    output is difficult/impossible to usefully interpret.

    In a dwp (Package File)  it is really impossible
    to associate abbrevs with a CU.

*/

int
dwarf_get_abbrev(Dwarf_Debug dbg,
    Dwarf_Unsigned offset,
    Dwarf_Abbrev * returned_abbrev,
    Dwarf_Unsigned * length,
    Dwarf_Unsigned * abbr_count, Dwarf_Error * error)
{
    Dwarf_Small *abbrev_ptr = 0;
    Dwarf_Small *abbrev_section_end = 0;
    Dwarf_Half attr         = 0;
    Dwarf_Half attr_form    = 0;
    Dwarf_Abbrev ret_abbrev = 0;
    Dwarf_Unsigned labbr_count = 0;
    Dwarf_Unsigned utmp     = 0;

    if (!dbg) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return DW_DLV_ERROR;
    }
    if (dbg->de_debug_abbrev.dss_data == 0) {
        /*  Loads abbrev section (and .debug_info as we do those
            together). */
        int res = _dwarf_load_debug_info(dbg, error);

        if (res != DW_DLV_OK) {
            return res;
        }
    }

    if (offset >= dbg->de_debug_abbrev.dss_size) {
        return DW_DLV_NO_ENTRY;
    }
    ret_abbrev = (Dwarf_Abbrev) _dwarf_get_alloc(dbg, DW_DLA_ABBREV, 1);
    if (ret_abbrev == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }
    ret_abbrev->dab_dbg = dbg;
    if (returned_abbrev == 0 || abbr_count == 0) {
        dwarf_dealloc(dbg, ret_abbrev, DW_DLA_ABBREV);
        _dwarf_error(dbg, error, DW_DLE_DWARF_ABBREV_NULL);
        return DW_DLV_ERROR;
    }


    *abbr_count = 0;
    if (length) {
        *length = 1;
    }


    abbrev_ptr = dbg->de_debug_abbrev.dss_data + offset;
    abbrev_section_end =
        dbg->de_debug_abbrev.dss_data + dbg->de_debug_abbrev.dss_size;

    DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp,
        dbg,error,abbrev_section_end);
    ret_abbrev->dab_code = (Dwarf_Word) utmp;
    if (ret_abbrev->dab_code == 0) {
        *returned_abbrev = ret_abbrev;
        *abbr_count = 0;
        if (length) {
            *length = 1;
        }
        return DW_DLV_OK;
    }

    DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp,
        dbg,error,abbrev_section_end);
    ret_abbrev->dab_tag = utmp;
    if (abbrev_ptr >= abbrev_section_end) {
        _dwarf_error(dbg, error, DW_DLE_ABBREV_DECODE_ERROR);
        return DW_DLV_ERROR;
    }
    ret_abbrev->dab_has_child = *(abbrev_ptr++);
    ret_abbrev->dab_abbrev_ptr = abbrev_ptr;

    do {
        Dwarf_Unsigned utmp2 = 0;

        DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp2,
            dbg,error,abbrev_section_end);
        attr = (Dwarf_Half) utmp2;
        DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp2,
            dbg,error,abbrev_section_end);
        attr_form = (Dwarf_Half) utmp2;
        if (!_dwarf_valid_form_we_know(dbg,attr_form,attr)) {
            _dwarf_error(dbg, error, DW_DLE_UNKNOWN_FORM);
            return (DW_DLV_ERROR);
        }
        if (attr_form ==  DW_FORM_implicit_const) {
            UNUSEDARG Dwarf_Signed implicit_const = 0;
            /* The value is here, not in a DIE. */
            DECODE_LEB128_SWORD_CK(abbrev_ptr, implicit_const,
                dbg,error,abbrev_section_end);
        }
        if (attr != 0) {
            labbr_count++;
        }
    } while (abbrev_ptr < abbrev_section_end &&
        (attr != 0 || attr_form != 0));
    /* Global section offset. */
    ret_abbrev->dab_goffset = offset;
    ret_abbrev->dab_count = labbr_count;
    if (abbrev_ptr > abbrev_section_end) {
        dwarf_dealloc(dbg, ret_abbrev, DW_DLA_ABBREV);
        _dwarf_error(dbg, error, DW_DLE_ABBREV_DECODE_ERROR);
        return DW_DLV_ERROR;
    }
    if (length) {
        *length = abbrev_ptr - dbg->de_debug_abbrev.dss_data - offset;
    }
    *returned_abbrev = ret_abbrev;
    *abbr_count = labbr_count;
    return DW_DLV_OK;
}

int
dwarf_get_abbrev_code(Dwarf_Abbrev abbrev,
    Dwarf_Unsigned * returned_code,
    Dwarf_Error * error)
{
    if (abbrev == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DWARF_ABBREV_NULL);
        return (DW_DLV_ERROR);
    }

    *returned_code = abbrev->dab_code;
    return (DW_DLV_OK);
}

/*  DWARF defines DW_TAG_hi_user as 0xffff so no tag should be
    over 16 bits.  */
int
dwarf_get_abbrev_tag(Dwarf_Abbrev abbrev,
    Dwarf_Half * returned_tag, Dwarf_Error * error)
{
    if (abbrev == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DWARF_ABBREV_NULL);
        return (DW_DLV_ERROR);
    }

    *returned_tag = abbrev->dab_tag;
    return (DW_DLV_OK);
}


int
dwarf_get_abbrev_children_flag(Dwarf_Abbrev abbrev,
    Dwarf_Signed * returned_flag,
    Dwarf_Error * error)
{
    if (abbrev == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DWARF_ABBREV_NULL);
        return (DW_DLV_ERROR);
    }

    *returned_flag = abbrev->dab_has_child;
    return (DW_DLV_OK);
}


int
dwarf_get_abbrev_entry(Dwarf_Abbrev abbrev,
    Dwarf_Signed indx,
    Dwarf_Half * returned_attr_num,
    Dwarf_Signed * form,
    Dwarf_Off * offset, Dwarf_Error * error)
{
    Dwarf_Byte_Ptr abbrev_ptr = 0;
    Dwarf_Byte_Ptr abbrev_end = 0;
    Dwarf_Byte_Ptr mark_abbrev_ptr = 0;
    Dwarf_Half attr = 0;
    Dwarf_Half attr_form = 0;

    if (indx < 0)
        return (DW_DLV_NO_ENTRY);

    if (abbrev == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DWARF_ABBREV_NULL);
        return (DW_DLV_ERROR);
    }

    if (abbrev->dab_code == 0) {
        return (DW_DLV_NO_ENTRY);
    }

    if (abbrev->dab_dbg == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return (DW_DLV_ERROR);
    }

    abbrev_ptr = abbrev->dab_abbrev_ptr;
    abbrev_end =
        abbrev->dab_dbg->de_debug_abbrev.dss_data +
        abbrev->dab_dbg->de_debug_abbrev.dss_size;

    for (attr = 1, attr_form = 1;
        indx >= 0 && abbrev_ptr < abbrev_end && (attr != 0 ||
            attr_form != 0);
        indx--) {
        Dwarf_Unsigned utmp4;

        mark_abbrev_ptr = abbrev_ptr;
        DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp4,abbrev->dab_dbg,
            error,abbrev_end);
        attr = (Dwarf_Half) utmp4;
        DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp4,abbrev->dab_dbg,
            error,abbrev_end);
        attr_form = (Dwarf_Half) utmp4;
        if (attr_form ==  DW_FORM_implicit_const) {
            UNUSEDARG Dwarf_Signed implicit_const;
            /* The value is here, not in a DIE. */
            DECODE_LEB128_SWORD_CK( abbrev_ptr, implicit_const,
                abbrev->dab_dbg,error,abbrev_end);
        }
    }

    if (abbrev_ptr >= abbrev_end) {
        _dwarf_error(abbrev->dab_dbg, error, DW_DLE_ABBREV_DECODE_ERROR);
        return (DW_DLV_ERROR);
    }

    if (indx >= 0) {
        return (DW_DLV_NO_ENTRY);
    }

    if (form != NULL) {
        *form = attr_form;
    }
    if (offset != NULL) {
        *offset = mark_abbrev_ptr - abbrev->dab_dbg->de_debug_abbrev.dss_data;
    }
    *returned_attr_num = (attr);
    return DW_DLV_OK;
}

/*  This function is not entirely safe to call.
    The problem is that the DWARF[234] specification does not insist
    that bytes in .debug_abbrev that are not referenced by .debug_info
    or .debug_types need to be initialized to anything specific.
    Any garbage bytes may cause trouble.  Not all compilers/linkers
    leave unreferenced garbage bytes in .debug_abbrev, so this may
    work for most objects.
    In case of error could return a bogus value, there is
    no documented way to detect error. */
int
dwarf_get_abbrev_count(Dwarf_Debug dbg)
{
    Dwarf_Abbrev ab;
    Dwarf_Unsigned offset = 0;
    Dwarf_Unsigned length = 0;
    Dwarf_Unsigned attr_count = 0;
    Dwarf_Unsigned abbrev_count = 0;
    int abres = DW_DLV_OK;
    Dwarf_Error err = 0;

    while ((abres = dwarf_get_abbrev(dbg, offset, &ab,
        &length, &attr_count,
        &err)) == DW_DLV_OK) {

        ++abbrev_count;
        offset += length;
        dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
    }
    if (err) {
        dwarf_dealloc(dbg,err,DW_DLA_ERROR);
        err = 0;
    }
    return abbrev_count;
}

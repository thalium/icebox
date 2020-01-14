/*
  Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2019 David Anderson. All Rights Reserved.
  Portions Copyright 2012 SN Systems Ltd. All rights reserved.

  This program is free software; you can redistribute it
  and/or modify it under the terms of version 2.1 of the
  GNU Lesser General Public License as published by the Free
  Software Foundation.

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

  You should have received a copy of the GNU Lesser General
  Public License along with this program; if not, write the
  Free Software Foundation, Inc., 51 Franklin Street - Fifth
  Floor, Boston MA 02110-1301, USA.

*/

#include "config.h"
#include <stdio.h>
#include "dwarf_incl.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#include "dwarf_die_deliv.h"

#define FALSE 0
#define TRUE 1

/* These are sanity checks, not 'rules'. */
#define MINIMUM_ADDRESS_SIZE 2
#define MAXIMUM_ADDRESS_SIZE 8

static void assign_correct_unit_type(Dwarf_CU_Context cu_context);
static int find_cu_die_base_fields(Dwarf_Debug dbg,
    Dwarf_Die cudie,
    Dwarf_Sig8 *    dwoid_return,
    Dwarf_Bool *    dwoid_present_return,
    Dwarf_Unsigned *str_offsets_base_return,
    Dwarf_Bool *    str_offsets_base_present_return,
    Dwarf_Unsigned *addr_base_return,
    Dwarf_Bool *    addr_base_present_return,
    Dwarf_Unsigned *ranges_base_return, /* rnglists_base */
    Dwarf_Bool *    ranges_base_present_return,
    Dwarf_Unsigned *loclists_base_return, /* rnglists_base */
    Dwarf_Bool *    loclists_base_present_return,
    char **         dwo_name_return, /* rnglists_base */
    Dwarf_Bool *    dwo_name_present_return,
    Dwarf_Bool *    has_children_return,
    Dwarf_Error*    error);


/*  see cuandunit.txt for an overview of the
    DWARF5 split dwarf sections and values
    and the DWARF4 GNU cc version of a draft
    version of DWARF5 (quite different from
    the final DWARF5).
*/

/*  New October 2011.  Enables client code to know if
    it is a debug_info or debug_types context. */
Dwarf_Bool
dwarf_get_die_infotypes_flag(Dwarf_Die die)
{
    return die->di_is_info;
}

#if 0
static void
dump_bytes(char * msg,Dwarf_Small * start, long len)
{
    Dwarf_Small *end = start + len;
    Dwarf_Small *cur = start;

    printf("%s ",msg);
    for (; cur < end; cur++) {
        printf("%02x ", *cur);
    }
    printf("\n");
}
#endif



/*
    For a given Dwarf_Debug dbg, this function checks
    if a CU that includes the given offset has been read
    or not.  If yes, it returns the Dwarf_CU_Context
    for the CU.  Otherwise it returns NULL.  Being an
    internal routine, it is assumed that a valid dbg
    is passed.

    **This is a sequential search.  May be too slow.

    If debug_info and debug_abbrev not loaded, this will
    wind up returning NULL. So no need to load before calling
    this.
*/
static Dwarf_CU_Context
_dwarf_find_CU_Context(Dwarf_Debug dbg, Dwarf_Off offset,Dwarf_Bool is_info)
{
    Dwarf_CU_Context cu_context = 0;
    Dwarf_Debug_InfoTypes dis = is_info? &dbg->de_info_reading:
        &dbg->de_types_reading;

    if (offset >= dis->de_last_offset)
        return (NULL);

    if (dis->de_cu_context != NULL &&
        dis->de_cu_context->cc_next != NULL &&
        dis->de_cu_context->cc_next->cc_debug_offset == offset) {

        return (dis->de_cu_context->cc_next);
    }

    if (dis->de_cu_context != NULL &&
        dis->de_cu_context->cc_debug_offset <= offset) {

        for (cu_context = dis->de_cu_context;
            cu_context != NULL; cu_context = cu_context->cc_next) {

            if (offset >= cu_context->cc_debug_offset &&
                offset < cu_context->cc_debug_offset +
                cu_context->cc_length + cu_context->cc_length_size
                + cu_context->cc_extension_size) {

                return (cu_context);
            }
        }
    }

    for (cu_context = dis->de_cu_context_list;
        cu_context != NULL; cu_context = cu_context->cc_next) {

        if (offset >= cu_context->cc_debug_offset &&
            offset < cu_context->cc_debug_offset +
            cu_context->cc_length + cu_context->cc_length_size
            + cu_context->cc_extension_size) {

            return (cu_context);
        }
    }

    return (NULL);
}


/*  This routine checks the dwarf_offdie() list of
    CU contexts for the right CU context.  */
static Dwarf_CU_Context
_dwarf_find_offdie_CU_Context(Dwarf_Debug dbg, Dwarf_Off offset,
    Dwarf_Bool is_info)
{
    Dwarf_CU_Context cu_context = 0;
    Dwarf_Debug_InfoTypes dis = is_info? &dbg->de_info_reading:
        &dbg->de_types_reading;

    for (cu_context = dis->de_offdie_cu_context;
        cu_context != NULL; cu_context = cu_context->cc_next)

        if (offset >= cu_context->cc_debug_offset &&
            offset < cu_context->cc_debug_offset +
            cu_context->cc_length + cu_context->cc_length_size
            + cu_context->cc_extension_size)

            return (cu_context);

    return (NULL);
}

int
dwarf_get_debugfission_for_die(Dwarf_Die die,
    struct Dwarf_Debug_Fission_Per_CU_s *fission_out,
    Dwarf_Error *error)
{
    Dwarf_CU_Context context = 0;
    Dwarf_Debug dbg = 0;
    struct Dwarf_Debug_Fission_Per_CU_s * percu = 0;

    CHECK_DIE(die, DW_DLV_ERROR);
    context = die->di_cu_context;
    dbg = context->cc_dbg;
    if (!_dwarf_file_has_debug_fission_index(dbg)) {
        return DW_DLV_NO_ENTRY;
    }

    /*  Logic should work for DW4 and DW5. */
    if (context->cc_unit_type == DW_UT_type||
        context->cc_unit_type == DW_UT_split_type ) {
        if (!_dwarf_file_has_debug_fission_tu_index(dbg)) {
            return DW_DLV_NO_ENTRY;
        }
    } else if (context->cc_unit_type == DW_UT_split_compile) {
        if (!_dwarf_file_has_debug_fission_cu_index(dbg)) {
            return DW_DLV_NO_ENTRY;
        }
    }
    percu = &context->cc_dwp_offsets;
    if (!percu->pcu_type) {
        return DW_DLV_NO_ENTRY;
    }
    *fission_out = *percu;
    return DW_DLV_OK;
}

static Dwarf_Bool
is_unknown_UT_value(int ut)
{
    switch(ut) {
    case DW_UT_compile:
    case DW_UT_type:
    case DW_UT_partial:
        return FALSE;
    case DW_UT_skeleton:
    case DW_UT_split_compile:
    case DW_UT_split_type:
        return FALSE;
    }
    return TRUE;
}


/*  ASSERT: whichone is a DW_SECT* macro value. */
Dwarf_Unsigned
_dwarf_get_dwp_extra_offset(struct Dwarf_Debug_Fission_Per_CU_s* dwp,
    unsigned whichone, Dwarf_Unsigned * size)
{
    Dwarf_Unsigned sectoff = 0;
    if (!dwp->pcu_type) {
        return 0;
    }
    sectoff = dwp->pcu_offset[whichone];
    *size = dwp->pcu_size[whichone];
    return sectoff;
}


/*  _dwarf_get_fission_addition_die returns DW_DLV_OK etc.
*/
int
_dwarf_get_fission_addition_die(Dwarf_Die die, int dw_sect_index,
   Dwarf_Unsigned *offset,
   Dwarf_Unsigned *size,
   Dwarf_Error *error)
{
    /* We do not yet know the DIE hash, so we cannot use it
        to identify the offset. */
    Dwarf_CU_Context context = 0;
    Dwarf_Unsigned dwpadd = 0;
    Dwarf_Unsigned dwpsize = 0;

    CHECK_DIE(die, DW_DLV_ERROR);
    context = die->di_cu_context;
    dwpadd =  _dwarf_get_dwp_extra_offset(
        &context->cc_dwp_offsets,
        dw_sect_index,&dwpsize);
    *offset = dwpadd;
    *size = dwpsize;
    return DW_DLV_OK;
}

/*  Not sure if this is the only way to be sure early on in
    reading a compile unit.  */
static int
section_name_ends_with_dwo(const char *name)
{
    int lenstr = 0;
    int dotpos = 0;
    if (!name) {
        return FALSE;
    }
    lenstr = strlen(name);
    if (lenstr < 5) {
        return FALSE;
    }
    dotpos = lenstr - 4;
    if(strcmp(name+dotpos,".dwo")) {
        return FALSE;
    }
    return TRUE;
}



/*  New January 2017 */
static int
_dwarf_read_cu_version_and_abbrev_offset(Dwarf_Debug dbg,
    Dwarf_Small *data,
    Dwarf_Bool is_info,
    UNUSEDARG unsigned group_number,
    unsigned offset_size, /* 4 or 8 */
    Dwarf_CU_Context cu_context,
    /* end_data used for sanity checking */
    Dwarf_Small *    end_data,
    Dwarf_Unsigned * bytes_read_out,
    Dwarf_Error *    error)
{
    Dwarf_Small *  data_start = data;
    Dwarf_Small *  dataptr = data;
    int            unit_type = 0;
    Dwarf_Ubyte    addrsize =  0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half     version = 0;

    READ_UNALIGNED_CK(dbg, version, Dwarf_Half,
        dataptr,DWARF_HALF_SIZE,error,end_data);
    dataptr += DWARF_HALF_SIZE;
    if (version == DW_CU_VERSION5) {
        Dwarf_Ubyte unit_typeb = 0;

        READ_UNALIGNED_CK(dbg, unit_typeb, Dwarf_Ubyte,
            dataptr, sizeof(unit_typeb),error,end_data);
        dataptr += sizeof(unit_typeb);

        unit_type = unit_typeb;
        /* We do not need is_info flag in DWARF5 */
        if (is_unknown_UT_value(unit_type)) {
            /*  DWARF5 object file is corrupt. Invalid value */
            _dwarf_error(dbg, error, DW_DLE_CU_UT_TYPE_ERROR);
            return DW_DLV_ERROR;
        }
        READ_UNALIGNED_CK(dbg, addrsize, unsigned char,
            dataptr, sizeof(addrsize),error,end_data);
        dataptr += sizeof(char);

        READ_UNALIGNED_CK(dbg, abbrev_offset, Dwarf_Unsigned,
            dataptr, offset_size,error,end_data);
        dataptr += offset_size;

    } else if (version == DW_CU_VERSION2 ||
        version == DW_CU_VERSION3 ||
        version == DW_CU_VERSION4) {
        /*  DWARF2,3,4  */
        READ_UNALIGNED_CK(dbg, abbrev_offset, Dwarf_Unsigned,
            dataptr, offset_size,error,end_data);
        dataptr += offset_size;

        READ_UNALIGNED_CK(dbg, addrsize, Dwarf_Ubyte,
            dataptr, sizeof(addrsize),error,end_data);
        dataptr += sizeof(addrsize);

        /*  This is an initial approximation of unit_type.
            For DW4 we will refine this after we
            have built the CU header (by reading
            CU_die)
        */
        unit_type = is_info?DW_UT_compile:DW_UT_type;
    } else {
        _dwarf_error(dbg, error, DW_DLE_VERSION_STAMP_ERROR);
        return DW_DLV_ERROR;
    }
    cu_context->cc_version_stamp = version;
    cu_context->cc_unit_type = unit_type;
    cu_context->cc_address_size = addrsize;
    cu_context->cc_abbrev_offset = abbrev_offset;
    if (!addrsize) {
        _dwarf_error(dbg,error,DW_DLE_ADDRESS_SIZE_ZERO);
        return DW_DLV_ERROR;
    }
    if (addrsize < MINIMUM_ADDRESS_SIZE ||
        addrsize > MAXIMUM_ADDRESS_SIZE ) {
        _dwarf_error(dbg,error,DW_DLE_ADDRESS_SIZE_ERROR);
        return DW_DLV_ERROR;
    }
    if (addrsize  > sizeof(Dwarf_Addr)) {
        _dwarf_error(dbg, error, DW_DLE_CU_ADDRESS_SIZE_BAD);
        return DW_DLV_ERROR;
    }



    /* We are ignoring this. Can get it from DWARF5. */
    cu_context->cc_segment_selector_size = 0;
    *bytes_read_out = (dataptr - data_start);
    return DW_DLV_OK;
}

/*  .debug_info[.dwo]   .debug_types[.dwo]
    the latter only DWARF4. */
static int
read_info_area_length_and_check(Dwarf_Debug dbg,
    Dwarf_CU_Context cu_context,
    Dwarf_Unsigned offset,
    Dwarf_Byte_Ptr *cu_ptr_io,
    Dwarf_Unsigned section_size,
    Dwarf_Byte_Ptr section_end_ptr,
    Dwarf_Unsigned *max_cu_global_offset_out,
    Dwarf_Error *error)
{
    Dwarf_Byte_Ptr  cu_ptr = 0;
    int local_length_size = 0;
    int local_extension_size = 0;
    Dwarf_Unsigned max_cu_global_offset = 0;
    Dwarf_Unsigned length = 0;

    cu_ptr = *cu_ptr_io;
    /* READ_AREA_LENGTH updates cu_ptr for consumed bytes */
    READ_AREA_LENGTH_CK(dbg, length, Dwarf_Unsigned,
        cu_ptr, local_length_size, local_extension_size,
        error,section_size,section_end_ptr);
    if (!length) {
        return DW_DLV_NO_ENTRY;
    }

    cu_context->cc_length_size = local_length_size;
    cu_context->cc_extension_size = local_extension_size;
    cu_context->cc_length = length;

    /*  This is a bare minimum, not the real max offset.
        A preliminary sanity check. */
    max_cu_global_offset =  offset + length +
        local_extension_size + local_length_size;
    if(length > section_size) {
        _dwarf_error(dbg, error, DW_DLE_CU_LENGTH_ERROR);
        return DW_DLV_ERROR;
    }
    if(max_cu_global_offset > section_size) {
        _dwarf_error(dbg, error, DW_DLE_CU_LENGTH_ERROR);
        return DW_DLV_ERROR;
    }
    *cu_ptr_io = cu_ptr;
    *max_cu_global_offset_out = max_cu_global_offset;
    return DW_DLV_OK;
}


/*  In DWARF4  GNU dwp there is a problem.
    We cannot read the CU die  and it's
    DW_AT_GNU_dwo_id until we know the
    section offsets from the index files.
    Hence we do not know how to search the
    index files by key. So search by offset.

    There is no such problem in DWARF5.

    We have not yet corrected the unit_type so, for DWARF4,
    we check for simpler unit types.
*/

static int
fill_in_dwp_offsets_if_present(Dwarf_Debug dbg,
    Dwarf_CU_Context cu_context,
    Dwarf_Sig8 * signaturedata,
    Dwarf_Off    offset,
    Dwarf_Error *error)
{
    Dwarf_Half unit_type = cu_context->cc_unit_type;
    const char * typename = 0;
    Dwarf_Half ver = cu_context->cc_version_stamp;

    if (unit_type == DW_UT_split_type ||
        (ver == DW_CU_VERSION4 && unit_type == DW_UT_type)){
        typename = "tu";
        if (!_dwarf_file_has_debug_fission_tu_index(dbg) ){
            /* nothing to do. */
            return DW_DLV_OK;
        }
    } else if (unit_type == DW_UT_split_compile ||
        (ver == DW_CU_VERSION4 &&
        unit_type == DW_UT_compile)){
        typename = "cu";
        if (!_dwarf_file_has_debug_fission_cu_index(dbg) ){
            /* nothing to do. */
            return DW_DLV_OK;
        }
    } else {
        /* nothing to do. */
        return DW_DLV_OK;
    }

    if (cu_context->cc_signature_present) {
        int resdf = 0;

        resdf = dwarf_get_debugfission_for_key(dbg,
            signaturedata,
            typename,
            &cu_context->cc_dwp_offsets,
            error);
        if (resdf == DW_DLV_ERROR) {
            return resdf;
        } else if (resdf == DW_DLV_NO_ENTRY) {
            _dwarf_error(dbg, error,
                DW_DLE_MISSING_REQUIRED_CU_OFFSET_HASH);
            return DW_DLV_ERROR;
        }
    } else {
        int resdf = 0;

        resdf = _dwarf_get_debugfission_for_offset(dbg,
            offset,
            typename,
            &cu_context->cc_dwp_offsets,
            error);
        if (resdf == DW_DLV_ERROR) {
            return resdf;
        } else if (resdf == DW_DLV_NO_ENTRY) {
            _dwarf_error(dbg, error,
                DW_DLE_MISSING_REQUIRED_CU_OFFSET_HASH);
            return DW_DLV_ERROR;
        }
        cu_context->cc_signature =
            cu_context->cc_dwp_offsets.pcu_hash;
        cu_context->cc_signature_present = TRUE;
    }
    return DW_DLV_OK;
}

static Dwarf_Bool
_dwarf_may_have_base_fields(Dwarf_CU_Context cu_context)
{
    if (cu_context->cc_version_stamp < DW_CU_VERSION4) {
        return FALSE;
    }
    return TRUE;
}

static int
finish_cu_context_via_cudie_inner(
    Dwarf_Debug dbg,
    Dwarf_CU_Context cu_context,
    Dwarf_Error *error)
{
    if (_dwarf_may_have_base_fields(cu_context)) {
        /*  DW4: Look for DW_AT_dwo_id and
            if there is one pick up the hash
            DW5: hash in skeleton CU die
            Also pick up cc_str_offset_base and
            any other base values. */

        Dwarf_Die cudie = 0;
        int resdwo = 0;

        /*  Must call the internal siblingof so
            we do not depend on the dbg...de_cu_context
            used by and for dwarf_cu_header_* calls. */
        resdwo = _dwarf_siblingof_internal(dbg,NULL,
            cu_context,
            cu_context->cc_is_info,
            &cudie, error);
        if (resdwo == DW_DLV_OK) {
            Dwarf_Sig8 dwosignature;
            Dwarf_Bool dwoid_present = FALSE;
            Dwarf_Unsigned str_offsets_base = 0;
            Dwarf_Unsigned addr_base = 0;
            Dwarf_Unsigned ranges_base = 0;
            Dwarf_Unsigned loclists_base = 0;
            char *     dwo_name = 0;
            Dwarf_Bool str_offsets_base_present = FALSE;
            Dwarf_Bool addr_base_present = FALSE;
            Dwarf_Bool ranges_base_present = FALSE;
            Dwarf_Bool loclists_base_present = FALSE;
            Dwarf_Bool dwo_name_present = FALSE;
            Dwarf_Bool has_children = TRUE;
            Dwarf_Half cutag = 0;
            int resdwob = 0;

            memset(&dwosignature,0,sizeof(dwosignature));
            resdwob = find_cu_die_base_fields(dbg,
                cudie,&dwosignature,&dwoid_present,
                &str_offsets_base,&str_offsets_base_present,
                &addr_base,&addr_base_present,
                &ranges_base,&ranges_base_present,
                &loclists_base,&loclists_base_present,
                &dwo_name,&dwo_name_present,
                &has_children,
                error);

            if (resdwob == DW_DLV_OK) {
                if(dwoid_present) {
                    if(!cu_context->cc_signature_present) {
                        /*  this can be in executable or ordinary .o
                            or .dwo or .dwp.  Non-standard DWARF4. */
                        cu_context->cc_signature = dwosignature;
                        cu_context->cc_signature_present = TRUE;
                    } else {
                        /*  VERIFY signatures match.
                            It's not clear if we ever get here
                            except in case of a corrupted
                            object file (so bad DWARF). */
                        /*  FIXME how report? Or just let it go
                            for the consumer to find? */
#if 0
                        if (memcmp(&cu_context->cc_signature,
                            &dwosignature,sizeof(Dwarf_Sig8))) {
                            _dwarf_error(NULL, error,
                                DW_DLE_DWP_SIGNATURE_MISMATCH);
                            return DW_DLV_ERROR;
                        }
#endif
                    }
                }
                cu_context->cc_cu_die_has_children = has_children;
                if (addr_base_present) {
                    cu_context->cc_addr_base = addr_base;
                    cu_context->cc_addr_base_present = TRUE;
                }

                if(str_offsets_base_present) {
                    cu_context->cc_str_offsets_base = str_offsets_base;
                    cu_context->cc_str_offsets_base_present = TRUE;
                }
                if(ranges_base_present) {
                    cu_context->cc_ranges_base = ranges_base;
                    cu_context->cc_ranges_base_present = TRUE;
                }
                if(loclists_base_present) {
                    cu_context->cc_loclists_base = ranges_base;
                    cu_context->cc_loclists_base_present = TRUE;
                }
                if (dwo_name_present ) {
                    cu_context->cc_dwo_name = dwo_name;
                }
            } else  if (resdwob == DW_DLV_NO_ENTRY) {
                /* The CU die has no children */
                dwarf_dealloc(dbg,cudie,DW_DLA_DIE);
                return DW_DLV_OK;
            } else {
                /*  Not applicable or an error */
                dwarf_dealloc(dbg,cudie,DW_DLA_DIE);
                return resdwob;
            }
            resdwob = dwarf_tag(cudie,&cutag,error);
            if (resdwob == DW_DLV_OK) {
                cu_context->cc_cu_die_tag = cutag;
            }
            dwarf_dealloc(dbg,cudie,DW_DLA_DIE);
            return resdwob;
        } else  if (resdwo == DW_DLV_NO_ENTRY) {
            /* no die. Empty CU */
            return DW_DLV_OK;
        } else {
            return resdwo;
        }
    }
    return DW_DLV_OK;
}


/*  This function is used to create a CU Context for
    a compilation-unit that begins at offset in
    .debug_info.  The CU Context is attached to the
    list of CU Contexts for this dbg.  It is assumed
    that the CU at offset has not been read before,
    and so do not call this routine before making
    sure of this with _dwarf_find_CU_Context().
    Returns NULL on error.  As always, being an
    internal routine, assumes a good dbg.

    The offset argument is global offset, the offset
    in the section, irrespective of CUs.
    The offset has the DWP Package File offset built in
    as it comes from the actual section.

    max_cu_local_offset is a local offset in this CU.
    So zero of this field is immediately following the length
    field of the CU header. so max_cu_local_offset is
    identical to the CU length field.
    max_cu_global_offset is the offset one-past the end
    of this entire CU.  */
static int
_dwarf_make_CU_Context(Dwarf_Debug dbg,
    Dwarf_Off offset,Dwarf_Bool is_info,
    Dwarf_CU_Context * context_out,Dwarf_Error * error)
{
    Dwarf_CU_Context cu_context = 0;
    Dwarf_Unsigned   length = 0;
    Dwarf_Unsigned   typeoffset = 0;
    Dwarf_Sig8       signaturedata;
    Dwarf_Unsigned   types_extra_len = 0;
    Dwarf_Unsigned   max_cu_local_offset =  0;
    Dwarf_Unsigned   max_cu_global_offset =  0;
    Dwarf_Byte_Ptr   cu_ptr = 0;
    Dwarf_Byte_Ptr   section_end_ptr = 0;
    int              local_length_size = 0;
    Dwarf_Unsigned   bytes_read = 0;
    const char *     secname = is_info?dbg->de_debug_info.dss_name:
        dbg->de_debug_types.dss_name;
    Dwarf_Debug_InfoTypes dis = is_info? &dbg->de_info_reading:
        &dbg->de_types_reading;
    Dwarf_Unsigned   section_size = is_info? dbg->de_debug_info.dss_size:
        dbg->de_debug_types.dss_size;
    int              unit_type = 0;
    int              version = 0;
    Dwarf_Small *    dataptr = 0;
    int              res = 0;

    cu_context =
        (Dwarf_CU_Context) _dwarf_get_alloc(dbg, DW_DLA_CU_CONTEXT, 1);
    if (cu_context == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }
    cu_context->cc_dbg = dbg;
    cu_context->cc_is_info = is_info;

    dataptr = is_info? dbg->de_debug_info.dss_data:
        dbg->de_debug_types.dss_data;
    /*  Preliminary sanity checking. */
    if (!dataptr) {
        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        _dwarf_error(dbg, error, DW_DLE_INFO_HEADER_ERROR);
        return DW_DLV_ERROR;
    }
    if (offset >= section_size) {
        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        _dwarf_error(dbg, error, DW_DLE_INFO_HEADER_ERROR);
        return DW_DLV_ERROR;
    }
    if ((offset+4) > section_size) {
        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        _dwarf_error(dbg, error, DW_DLE_INFO_HEADER_ERROR);
        return DW_DLV_ERROR;
    }
    section_end_ptr = dataptr+section_size;
    cu_ptr = (Dwarf_Byte_Ptr) (dataptr+offset);

    if (section_name_ends_with_dwo(secname)) {
        cu_context->cc_is_dwo = TRUE;
    }
    res = read_info_area_length_and_check(dbg,
        cu_context,
        offset,
        &cu_ptr,
        section_size,
        section_end_ptr,
        &max_cu_global_offset,
        error);
    if (res != DW_DLV_OK) {
        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        return res;
    }
    local_length_size = cu_context->cc_length_size;
    length = cu_context->cc_length;
    max_cu_local_offset =  length;
    res  = _dwarf_read_cu_version_and_abbrev_offset(dbg,
        cu_ptr,
        is_info,
        dbg->de_groupnumber,
        local_length_size,
        cu_context,
        section_end_ptr,
        &bytes_read,error);
    if (res != DW_DLV_OK) {
        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        return res;
    }
    version = cu_context->cc_version_stamp;
    cu_ptr += bytes_read;
    unit_type = cu_context->cc_unit_type;
    if (cu_ptr > section_end_ptr) {
        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        _dwarf_error(dbg, error, DW_DLE_INFO_HEADER_ERROR);
        return DW_DLV_ERROR;
    }

    /*  In a dwp context, the abbrev_offset is
        still  incomplete.
        We need to add in the base from the .debug_cu_index
        or .debug_tu_index . Done below */

    /*  At this point, for DW4, the unit_type is not fully
        correct as we don't know if it is a skeleton or
        a split_compile or split_type */
    if (version ==  DW_CU_VERSION5 ||
        version == DW_CU_VERSION4) {
        /*  DW4/DW5  header fields, depending on UT type.
            See DW5  section 7.5.1.x, DW4
            data is a GNU extension of DW4. */
        switch(unit_type) {
        case DW_UT_split_type:
        case DW_UT_type: {
            types_extra_len = sizeof(Dwarf_Sig8) /* 8 */ +
                local_length_size /*type_offset size*/;
            break;
        }
        case DW_UT_skeleton:
        case DW_UT_split_compile: {
            types_extra_len = sizeof(Dwarf_Sig8) /* 8 */;
            break;
        }
        case DW_UT_compile: /*  No additional fields */
        case DW_UT_partial: /*  No additional fields */
            break;
        default:
            /*  Data corruption in libdwarf? */
            dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
            _dwarf_error(dbg,error,DW_DLE_CU_UT_TYPE_VALUE);
            return DW_DLV_ERROR;
        }
    }

    /*  Compare the space following the length field
        to the bytes in the CU header. */
    if (length <
        (CU_VERSION_STAMP_SIZE /* is 2 */ +
        local_length_size /*for debug_abbrev offset */ +
        CU_ADDRESS_SIZE_SIZE /* is 1 */ +
        /* and finally size of the rest of the header: */
        types_extra_len)) {

        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        _dwarf_error(dbg, error, DW_DLE_CU_LENGTH_ERROR);
        return DW_DLV_ERROR;
    }
    /*  Now we can read the fields with some confidence,
        we know the fields of the header are inside
        the section. */

    cu_context->cc_unit_type = unit_type;
    switch(unit_type) {
    case DW_UT_split_type:
    case DW_UT_type: {
        /*  ASSERT: DW_CU_VERSION4 or DW_CU_VERSION5,
            determined by logic above.
            Now read the debug_types extra header fields of
            the signature (8 bytes) and the typeoffset.
            This can be in executable, ordinary object
            (as in Type Unit),
            there was no dwo in DWARF4
        */
        memcpy(&signaturedata,cu_ptr,sizeof(signaturedata));
        cu_ptr += sizeof(signaturedata);
        READ_UNALIGNED_CK(dbg, typeoffset, Dwarf_Unsigned,
            cu_ptr, local_length_size,error,section_end_ptr);
        cu_context->cc_signature = signaturedata;
        cu_context->cc_signature_offset = typeoffset;
        cu_context->cc_signature_present = TRUE;
        if (typeoffset >= max_cu_local_offset) {
            dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
            _dwarf_error(dbg, error,
                DW_DLE_DEBUG_TYPEOFFSET_BAD);
            return DW_DLV_ERROR;
        }
        }
        break;
    case DW_UT_skeleton:
    case DW_UT_split_compile: {
        /*  These unit types make a pair and
            paired units have identical signature.*/
        memcpy(&signaturedata,cu_ptr,sizeof(signaturedata));
        cu_context->cc_signature = signaturedata;
        cu_context->cc_signature_present = TRUE;

        break;
        }
    /* The following with no additional fields */
    case DW_UT_compile:
    case DW_UT_partial:
        break;
    default:
        /*  Data corruption in libdwarf? */
        dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
        _dwarf_error(dbg,error,DW_DLE_CU_UT_TYPE_VALUE);
        return DW_DLV_ERROR;
    }
    cu_context->cc_abbrev_hash_table =
        (Dwarf_Hash_Table) _dwarf_get_alloc(dbg, DW_DLA_HASH_TABLE, 1);
    if (cu_context->cc_abbrev_hash_table == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }

    cu_context->cc_debug_offset = offset;

    /*  This is recording an overall section value for later
        sanity checking. */
    dis->de_last_offset = max_cu_global_offset;
    *context_out  = cu_context;
    return DW_DLV_OK;
}

static int
reloc_incomplete(int res,Dwarf_Error err)
{
    int e = 0;

    if (res == DW_DLV_OK) {
        return FALSE;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return FALSE;
    }
    e = dwarf_errno(err);
    if (e == DW_DLE_RELOC_MISMATCH_INDEX        ||
        e == DW_DLE_RELOC_MISMATCH_RELOC_INDEX  ||
        e == DW_DLE_RELOC_MISMATCH_STRTAB_INDEX ||
        e == DW_DLE_RELOC_SECTION_MISMATCH      ||
        e == DW_DLE_RELOC_SECTION_MISSING_INDEX ||
        e == DW_DLE_RELOC_SECTION_LENGTH_ODD    ||
        e == DW_DLE_RELOC_SECTION_PTR_NULL      ||
        e == DW_DLE_RELOC_SECTION_MALLOC_FAIL   ||
        e == DW_DLE_SEEK_OFF_END                ||
        e == DW_DLE_RELOC_INVALID               ||
        e == DW_DLE_RELOC_SECTION_SYMBOL_INDEX_BAD ) {
        return TRUE;
    }
    return FALSE;
}



/*  Returns offset of next compilation-unit thru next_cu_offset
    pointer.
    It sequentially moves from one
    cu to the next.  The current cu is recorded
    internally by libdwarf.

    The _b form is new for DWARF4 adding new returned fields.  */
int
dwarf_next_cu_header(Dwarf_Debug dbg,
    Dwarf_Unsigned * cu_header_length,
    Dwarf_Half * version_stamp,
    Dwarf_Unsigned * abbrev_offset,
    Dwarf_Half * address_size,
    Dwarf_Unsigned * next_cu_offset,
    Dwarf_Error * error)
{
    Dwarf_Bool is_info = true;
    Dwarf_Half header_type = 0;
    return _dwarf_next_cu_header_internal(dbg,
        is_info,
        cu_header_length,
        version_stamp,
        abbrev_offset,
        address_size,
        0,0,0,0,0,
        next_cu_offset,
        &header_type,
        error);
}
int
dwarf_next_cu_header_b(Dwarf_Debug dbg,
    Dwarf_Unsigned * cu_header_length,
    Dwarf_Half * version_stamp,
    Dwarf_Unsigned * abbrev_offset,
    Dwarf_Half * address_size,
    Dwarf_Half * offset_size,
    Dwarf_Half * extension_size,
    Dwarf_Unsigned * next_cu_offset,
    Dwarf_Error * error)
{
    Dwarf_Bool is_info = true;
    Dwarf_Half header_type = 0;
    return _dwarf_next_cu_header_internal(dbg,
        is_info,
        cu_header_length,
        version_stamp,
        abbrev_offset,
        address_size,
        offset_size,extension_size,
        0,0,0,
        next_cu_offset,
        &header_type,
        error);
}

int
dwarf_next_cu_header_c(Dwarf_Debug dbg,
    Dwarf_Bool is_info,
    Dwarf_Unsigned * cu_header_length,
    Dwarf_Half * version_stamp,
    Dwarf_Unsigned * abbrev_offset,
    Dwarf_Half * address_size,
    Dwarf_Half * offset_size,
    Dwarf_Half * extension_size,
    Dwarf_Sig8 * signature,
    Dwarf_Unsigned * typeoffset,
    Dwarf_Unsigned * next_cu_offset,
    Dwarf_Error * error)
{
    Dwarf_Half header_type = 0;
    int res =_dwarf_next_cu_header_internal(dbg,
        is_info,
        cu_header_length,
        version_stamp,
        abbrev_offset,
        address_size,
        offset_size,
        extension_size,
        signature,
        0,
        typeoffset,
        next_cu_offset,
        &header_type,
        error);
    return res;
}
int
dwarf_next_cu_header_d(Dwarf_Debug dbg,
    Dwarf_Bool is_info,
    Dwarf_Unsigned * cu_header_length,
    Dwarf_Half * version_stamp,
    Dwarf_Unsigned * abbrev_offset,
    Dwarf_Half * address_size,
    Dwarf_Half * offset_size,
    Dwarf_Half * extension_size,
    Dwarf_Sig8 * signature,
    Dwarf_Unsigned * typeoffset,
    Dwarf_Unsigned * next_cu_offset,
    Dwarf_Half * header_cu_type,
    Dwarf_Error * error)
{
    /* Faking has_signature to do nothing. */
    Dwarf_Bool* has_signature = 0;
    int res = 0;

    res = _dwarf_next_cu_header_internal(dbg,
        is_info,
        cu_header_length,
        version_stamp,
        abbrev_offset,
        address_size,
        offset_size,
        extension_size,
        signature,
        has_signature,
        typeoffset,
        next_cu_offset,
        header_cu_type,
        error);
    return res;
}



/*
    A DWO/DWP CU has different base fields than
    a normal object/executable, but this finds
    the base fields for both types.
*/
static int
find_cu_die_base_fields(Dwarf_Debug dbg,
    Dwarf_Die cudie,
    Dwarf_Sig8 *    dwoid_return,
    Dwarf_Bool *    dwoid_present_return,
    Dwarf_Unsigned *str_offsets_base_return,
    Dwarf_Bool *    str_offsets_base_present_return,
    Dwarf_Unsigned *addr_base_return,
    Dwarf_Bool *    addr_base_present_return,
    Dwarf_Unsigned *ranges_base_return, /* rnglists_base */
    Dwarf_Bool *    ranges_base_present_return,
    Dwarf_Unsigned *loclists_base_return, /* rnglists_base */
    Dwarf_Bool *    loclists_base_present_return,
    char **         dwo_name_return, /* rnglists_base */
    Dwarf_Bool *    dwo_name_present_return,
    Dwarf_Bool *    has_children_return,
    Dwarf_Error*    error)
{
    Dwarf_Sig8 signature;
    Dwarf_Bool dwoid_sig_present = FALSE;
    Dwarf_Off  str_offsets_base = 0;
    Dwarf_Off  ranges_base = 0;
    Dwarf_Off  addr_base = 0;
    Dwarf_Bool str_offsets_base_present = FALSE;
    Dwarf_Bool addr_base_present = FALSE;
    Dwarf_Bool ranges_base_present = FALSE;
    Dwarf_Off  loclists_base = 0;
    Dwarf_Bool loclists_base_present = 0;
    char *     dwo_name = 0;
    Dwarf_Bool dwo_name_present = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_CU_Context  cu_context = 0;
    Dwarf_Attribute * alist = 0;
    Dwarf_Signed      atcount = 0;
    int               alres = 0;
    Dwarf_Half        has_children = TRUE;
    int chres = 0;

    cu_context = cudie->di_cu_context;
    version_stamp = cu_context->cc_version_stamp;

    alres = dwarf_attrlist(cudie, &alist,
        &atcount,error);
    if(alres == DW_DLV_OK) {
        /*  DW_AT_dwo_id and/or DW_AT_GNU_dwo_id
            are only found  in some
            experimental DWARF4.
            DWARF5 changed CU header contents
            to make this attribute unnecessary.
            DW_AT_GNU_odr_signature is the same format,
            but is in a different namespace so not
            appropriate here..
        */
        Dwarf_Signed i = 0;
        for(i = 0;  i < atcount; ++i) {
            Dwarf_Half attrnum;
            int ares = 0;
            Dwarf_Attribute attr = alist[i];
            ares = dwarf_whatattr(attr,&attrnum,error);
            if (ares == DW_DLV_OK) {
                switch(attrnum) {
                case DW_AT_dwo_id:
                case DW_AT_GNU_dwo_id: {
                    /*  This is for DWARF4 with an early
                        non-standard version
                        of split dwarf. Not DWARF5. */
                    int sres = 0;
                    if (version_stamp < DW_CU_VERSION4) {
                        /* Not supposed to happen. */
                        _dwarf_error(dbg,error,
                            DW_DLE_IMPROPER_DWO_ID);
                        return DW_DLV_ERROR;
                    }

                    sres = dwarf_formsig8_const(attr,
                        &signature,error);
                    if(sres == DW_DLV_OK) {
                        dwoid_sig_present = TRUE;
                    } else {
                        /* Something is badly wrong. */
                        dwarf_dealloc(dbg,attr,DW_DLA_ATTR);
                        dwarf_dealloc(dbg,alist,DW_DLA_LIST);
                        return sres;
                    }
                    break;
                }
                case DW_AT_str_offsets_base:{
                    int udres = 0;
                    udres = dwarf_global_formref(attr,&str_offsets_base,
                        error);
                    if(udres == DW_DLV_OK) {
                        str_offsets_base_present = TRUE;
                    } else {
                        dwarf_dealloc(dbg,attr,DW_DLA_ATTR);
                        dwarf_dealloc(dbg,alist,DW_DLA_LIST);
                        /* Something is badly wrong. */
                        return udres;
                    }
                    break;
                }
                case DW_AT_addr_base:
                case DW_AT_GNU_addr_base: {
                    int udres = 0;
                    udres = dwarf_global_formref(attr,&addr_base,
                        error);
                    if(udres == DW_DLV_OK) {
                        addr_base_present = TRUE;
                    } else {
                        dwarf_dealloc(dbg,attr,DW_DLA_ATTR);
                        dwarf_dealloc(dbg,alist,DW_DLA_LIST);
                        /* Something is badly wrong. */
                        return udres;
                    }
                    break;
                }
                case DW_AT_GNU_ranges_base:
                /*  The DW4 ranges base was never used in GNU
                    but did get emitted in skeletons.
                    http://llvm.1065342.n5.nabble.com/DebugInfo-DW-AT-GNU-ranges-base-in-non-fission-td64194.html
                    We therefore ignore it.  */
                    break;
                case  DW_AT_rnglists_base: {
                    int udres = 0;
                    udres = dwarf_global_formref(attr,&ranges_base,
                        error);
                    if(udres == DW_DLV_OK) {
                        ranges_base_present = TRUE;
                    } else {
                        dwarf_dealloc(dbg,attr,DW_DLA_ATTR);
                        dwarf_dealloc(dbg,alist,DW_DLA_LIST);
                        /* Something is badly wrong. */
                        return udres;
                    }
                    break;
                }
                case DW_AT_GNU_dwo_name:
                case DW_AT_dwo_name: {
                    int dnres = 0;

                    dwo_name_present = TRUE;
                    dnres = dwarf_formstring(attr,
                        &dwo_name,error);
                    if (dnres != DW_DLV_OK) {
                        return dnres;
                    }
                    break;
                }
                case DW_AT_loclists_base: {
                    int udres = 0;
                    udres = dwarf_global_formref(attr,&loclists_base,
                        error);
                    if(udres == DW_DLV_OK) {
                        loclists_base_present = TRUE;
                    } else {
                        dwarf_dealloc(dbg,attr,DW_DLA_ATTR);
                        dwarf_dealloc(dbg,alist,DW_DLA_LIST);
                        /* Something is badly wrong. */
                        return udres;
                    }
                    break;
                }
                default: /* do nothing, not an attribute
                    we need to deal with here. */
                    break;
                }
            }
            dwarf_dealloc(dbg,attr,DW_DLA_ATTR);
        }
        dwarf_dealloc(dbg,alist,DW_DLA_LIST);
    } else {
        /* Something is badly wrong. No attrlist! */
        return alres;
    }
    *dwoid_present_return =            dwoid_sig_present;
    *dwoid_return =                    signature;
    *str_offsets_base_present_return = str_offsets_base_present;
    *str_offsets_base_return =         str_offsets_base;
    *addr_base_present_return =        addr_base_present;
    *addr_base_return =                addr_base;
    *ranges_base_present_return =      ranges_base_present;
    *ranges_base_return =              ranges_base;
    *loclists_base_present_return =    loclists_base_present;
    *loclists_base_return =            loclists_base;
    *dwo_name_present_return =         dwo_name_present;
    *dwo_name_return =                 dwo_name;
    chres = dwarf_die_abbrev_children_flag(cudie, &has_children);
    *has_children_return = TRUE;
    if (chres == DW_DLV_OK) {
        if (!has_children) {
            *has_children_return = FALSE;
        }
    } else {
        /*  Nothing we do here makes much sense. Pretend has
            children. */
    }
    return DW_DLV_OK;
}

/*  Called only for DWARF4 */
static void
assign_correct_unit_type(Dwarf_CU_Context cu_context)
{
    Dwarf_Half tag = cu_context->cc_cu_die_tag;
    if(!cu_context->cc_cu_die_has_children) {
        if(cu_context->cc_signature_present) {
            if (tag == DW_TAG_compile_unit ||
                tag == DW_TAG_type_unit ) {
                cu_context->cc_unit_type = DW_UT_skeleton;
            }
        }
    } else {
        if(cu_context->cc_signature_present) {
            if (tag == DW_TAG_compile_unit) {
                cu_context->cc_unit_type = DW_UT_split_compile;
            } else if (tag == DW_TAG_type_unit) {
                cu_context->cc_unit_type = DW_UT_split_type;
            }
        }
    }
}

static int
finish_up_cu_context_from_cudie(Dwarf_Debug dbg,
    Dwarf_Unsigned offset,
    Dwarf_CU_Context cu_context,
    Dwarf_Error *error)
{
    int version = cu_context->cc_version_stamp;
    Dwarf_Sig8 signaturedata;
    int res = 0;


    signaturedata = cu_context->cc_signature;

    res = fill_in_dwp_offsets_if_present(dbg,
        cu_context,
        &signaturedata,
        offset,
        error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (res != DW_DLV_OK) {
        return res;
    }

    if (cu_context->cc_dwp_offsets.pcu_type) {
        Dwarf_Unsigned absize = 0;
        Dwarf_Unsigned aboff = 0;

        aboff = _dwarf_get_dwp_extra_offset(
            &cu_context->cc_dwp_offsets,
            DW_SECT_ABBREV, &absize);
        cu_context->cc_abbrev_offset +=  aboff;
    }

    if (cu_context->cc_abbrev_offset >=
        dbg->de_debug_abbrev.dss_size) {
        _dwarf_error(dbg, error, DW_DLE_ABBREV_OFFSET_ERROR);
        return DW_DLV_ERROR;
    }
    /*  Now we can read the CU die and determine
        the correct DW_UT_ type for DWARF4 and some
        offset base fields for DW4-fission and DW5 */
    if (version == DW_CU_VERSION4 || version == DW_CU_VERSION5) {
        res = finish_cu_context_via_cudie_inner(dbg,
            cu_context,
            error);
        if(res == DW_DLV_ERROR) {
            return res;
        }
        if(res != DW_DLV_OK) {
            return res;
        }
        if (version == DW_CU_VERSION4) {
            assign_correct_unit_type(cu_context);
        }
    }
    return DW_DLV_OK;
}

int
_dwarf_next_cu_header_internal(Dwarf_Debug dbg,
    Dwarf_Bool is_info,
    Dwarf_Unsigned * cu_header_length,
    Dwarf_Half * version_stamp,
    Dwarf_Unsigned * abbrev_offset,
    Dwarf_Half * address_size,
    Dwarf_Half * offset_size,
    Dwarf_Half * extension_size,
    Dwarf_Sig8 * signature_out,
    Dwarf_Bool * has_signature,
    Dwarf_Unsigned *typeoffset,
    Dwarf_Unsigned * next_cu_offset,

    /*  header_type: DW_UT_compile, DW_UT_partial,
        DW_UT_type, returned through the pointer.
        A new item in DWARF5, synthesized for earlier DWARF
        CUs (& TUs). */
    Dwarf_Half * header_type,
    Dwarf_Error * error)
{
    /* Offset for current and new CU. */
    Dwarf_Unsigned new_offset = 0;

    /* CU Context for current CU. */
    Dwarf_CU_Context cu_context = 0;
    Dwarf_Debug_InfoTypes dis = 0;
    Dwarf_Unsigned section_size =  0;
    int res = 0;

    /* ***** BEGIN CODE ***** */

    if (dbg == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return (DW_DLV_ERROR);
    }
    dis = is_info? &dbg->de_info_reading: &dbg->de_types_reading;
    /*  Get offset into .debug_info of next CU.
        If dbg has no context,
        this has to be the first one.  */
    if (!dis->de_cu_context) {
        Dwarf_Small *dataptr = is_info? dbg->de_debug_info.dss_data:
            dbg->de_debug_types.dss_data;
        new_offset = 0;
        if (!dataptr) {
            Dwarf_Error err2= 0;
            int resd = is_info?_dwarf_load_debug_info(dbg, &err2):
                _dwarf_load_debug_types(dbg,&err2);

            if (resd != DW_DLV_OK) {
                if (reloc_incomplete(resd,err2)) {
                    /*  We will assume all is ok, though it is not.
                        Relocation errors need not be fatal. */
                    char msg_buf[300];
                    char *dwerrmsg = 0;
                    char *msgprefix =
                        "Relocations did not complete successfully, "
                        "but we are " " ignoring error: ";
                    size_t totallen = 0;
                    size_t prefixlen = 0;

                    dwerrmsg = dwarf_errmsg(err2);
                    prefixlen = strlen(msgprefix);
                    totallen = prefixlen + strlen(dwerrmsg);
                    if( totallen >= sizeof(msg_buf)) {
                        /*  Impossible unless something corrupted.
                            Provide a shorter dwerrmsg*/
                        strcpy(msg_buf,"Error:corrupted dwarf message table!");
                    } else {
                        strcpy(msg_buf,msgprefix);
                        strcpy(msg_buf+prefixlen,dwerrmsg);
                    }
                    dwarf_insert_harmless_error(dbg,msg_buf);
                    /*  Fall thru to use the newly loaded section.
                        even though it might not be adequately
                        relocated. */
                } else {
                    if (error) {
                        *error = err2;
                        err2 = 0;
                    }
                    /*  There is nothing here, or
                        what is here is damaged. */
                    return resd;
                }

            }
        }
        /*  We are leaving new_offset zero. We are at the
            start of a section. */
    } else {
        /* We already have is_info  cu_context. */

        new_offset = dis->de_cu_context->cc_debug_offset +
            dis->de_cu_context->cc_length +
            dis->de_cu_context->cc_length_size +
            dis->de_cu_context->cc_extension_size;
    }

    /*  Check that there is room in .debug_info beyond
        the new offset for at least a new cu header.
        If not, return -1 (DW_DLV_NO_ENTRY) to indicate end
        of debug_info section, and reset
        de_cu_debug_info_offset to
        enable looping back through the cu's. */
    section_size = is_info? dbg->de_debug_info.dss_size:
        dbg->de_debug_types.dss_size;
    if ((new_offset + _dwarf_length_of_cu_header_simple(dbg,is_info)) >=
        section_size) {
        dis->de_cu_context = NULL;
        return DW_DLV_NO_ENTRY;
    }

    /* Check if this CU has been read before. */
    cu_context = _dwarf_find_CU_Context(dbg, new_offset,is_info);

    /* If not, make CU Context for it. */
    if (cu_context == NULL) {
        res = _dwarf_make_CU_Context(dbg, new_offset,is_info,
            &cu_context,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            return res;
        }
        res = finish_up_cu_context_from_cudie(dbg,new_offset,
            cu_context,error);
        if (res == DW_DLV_ERROR) {
            dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
            return res;
        }

        dis->de_cu_context = cu_context;
        if (dis->de_cu_context_list == NULL) {
            dis->de_cu_context_list = cu_context;
            dis->de_cu_context_list_end = cu_context;
        } else {
            dis->de_cu_context_list_end->cc_next = cu_context;
            dis->de_cu_context_list_end = cu_context;
        }

    } else {
        dis->de_cu_context = cu_context;
    }

    if (cu_header_length != NULL) {
        *cu_header_length = cu_context->cc_length;
    }

    if (version_stamp != NULL) {
        *version_stamp = cu_context->cc_version_stamp;
    }
    if (abbrev_offset != NULL) {
        *abbrev_offset = cu_context->cc_abbrev_offset;
    }

    if (address_size != NULL) {
        *address_size = cu_context->cc_address_size;
    }
    if (offset_size != NULL) {
        *offset_size = cu_context->cc_length_size;
    }
    if (extension_size != NULL) {
        *extension_size = cu_context->cc_extension_size;
    }
    if (header_type) {
        *header_type = cu_context->cc_unit_type;
    }

    if (typeoffset) {
        *typeoffset = cu_context->cc_signature_offset;
    }
    if (signature_out) {
        *signature_out = cu_context->cc_signature;
    }
    if (has_signature) {
        *has_signature = cu_context->cc_signature_present;
    }
    /*  Determine the offset of the next CU. */
    new_offset = new_offset + cu_context->cc_length +
        cu_context->cc_length_size + cu_context->cc_extension_size;
    /*  Allowing null argument starting 22 April 2019. */
    if (next_cu_offset) {
        *next_cu_offset = new_offset;
    }
    return DW_DLV_OK;
}

/*  This involves data in a split dwarf or package file.

    Given hash signature, return the CU_die of the applicable CU.
    The hash is assumed to be from 'somewhere'.
    For DWARF 4:
        From a skeleton DIE DW_AT_GNU_dwo_id  ("cu" case) or
        From a DW_FORM_ref_sig8 ("tu" case).
    For DWARF5:
        From  dwo_id in a skeleton CU header (DW_UT_skeleton).
        From a DW_FORM_ref_sig8 ("tu" case).


    If "tu" request,  the CU_die
    of of the type unit.
    Works on either a dwp package file or a dwo object.

    If "cu" request,  the CU_die
    of the compilation unit.
    Works on either a dwp package file or a dwo object.

    If the hash passed is not present, returns DW_DLV_NO_ENTRY
    (but read the next two paragraphs for more detail).

    If a dwp package file with the hash signature
    is present in the applicable index but no matching
    compilation unit can be found, it returns DW_DLV_ERROR.

    If a .dwo object there is no index and we look at the
    compilation units (possibly all of them). If not present
    then we return DW_DLV_NO_ENTRY.

    The returned_die is a CU DIE if the sig_type is "cu".
    The returned_die is a type DIE if the sig_type is "tu".
    Perhaps both should return CU die.

    New 27 April, 2015
*/
int
dwarf_die_from_hash_signature(Dwarf_Debug dbg,
    Dwarf_Sig8 *     hash_sig,
    const char *     sig_type  /* "tu" or "cu"*/,
    Dwarf_Die  *     returned_die,
    Dwarf_Error*     error)
{
    Dwarf_Bool is_type_unit = FALSE;
    int sres = 0;

    sres = _dwarf_load_debug_info(dbg,error);
    if (sres == DW_DLV_ERROR) {
        return sres;
    }
    sres = _dwarf_load_debug_types(dbg,error);
    if (sres == DW_DLV_ERROR) {
        return sres;
    }

    if (!strcmp(sig_type,"tu")) {
        is_type_unit = TRUE;
    } else if (!strcmp(sig_type,"cu")) {
        is_type_unit = FALSE;
    } else {
        _dwarf_error(dbg,error,DW_DLE_SIG_TYPE_WRONG_STRING);
        return DW_DLV_ERROR;
    }

    if (_dwarf_file_has_debug_fission_index(dbg)) {
        /* This is a dwp package file. */
        int fisres = 0;
        Dwarf_Bool is_info2 = 0;
        Dwarf_Off cu_header_off = 0;
        Dwarf_Off cu_size = 0;
        Dwarf_Off cu_die_off = 0;
        Dwarf_Off typeoffset = 0;
        Dwarf_Die cudie = 0;
        Dwarf_Die typedie = 0;
        Dwarf_CU_Context context = 0;
        Dwarf_Debug_Fission_Per_CU fiss;

        memset(&fiss,0,sizeof(fiss));
        fisres = dwarf_get_debugfission_for_key(dbg,hash_sig,
            sig_type,&fiss,error);
        if (fisres != DW_DLV_OK) {
            return fisres;
        }
        /* Found it */
        if(is_type_unit) {
            /*  DW4 has debug_types, so look in .debug_types
                Else look in .debug_info.  */
            is_info2 = dbg->de_debug_types.dss_size?FALSE:TRUE;
        } else {
            is_info2 = TRUE;
        }

        cu_header_off = _dwarf_get_dwp_extra_offset(&fiss,
            is_info2?DW_SECT_INFO:DW_SECT_TYPES,
            &cu_size);

        fisres = dwarf_get_cu_die_offset_given_cu_header_offset_b(
            dbg,cu_header_off,
            is_info2,
            &cu_die_off,error);
        if (fisres != DW_DLV_OK) {
            return fisres;
        }
        fisres = dwarf_offdie_b(dbg,cu_die_off,is_info2,
            &cudie,error);
        if (fisres != DW_DLV_OK) {
            return fisres;
        }
        if (!is_type_unit) {
            *returned_die = cudie;
            return DW_DLV_OK;
        }
        context = cudie->di_cu_context;
        typeoffset = context->cc_signature_offset;
        typeoffset += cu_header_off;
        fisres = dwarf_offdie_b(dbg,typeoffset,is_info2,
            &typedie,error);
        if (fisres != DW_DLV_OK) {
            dwarf_dealloc(dbg,cudie,DW_DLA_DIE);
            return fisres;
        }
        *returned_die = typedie;
        dwarf_dealloc(dbg,cudie,DW_DLA_DIE);
        return DW_DLV_OK;
    }
    /*  Look thru all the CUs, there is no DWP tu/cu index.
        There will be COMDAT sections for  the type TUs
            (DW_UT_type).
        A single non-comdat for the DW_UT_compile. */
    /*  FIXME: DW_DLE_DEBUG_FISSION_INCOMPLETE  */
    _dwarf_error(dbg,error,DW_DLE_DEBUG_FISSION_INCOMPLETE);
    return DW_DLV_ERROR;
}

static int
dwarf_ptr_CU_offset(Dwarf_CU_Context cu_context,
    Dwarf_Byte_Ptr di_ptr,
    Dwarf_Bool is_info,
    Dwarf_Off * cu_off)
{
    Dwarf_Debug dbg = cu_context->cc_dbg;
    Dwarf_Small *dataptr = is_info? dbg->de_debug_info.dss_data:
        dbg->de_debug_types.dss_data;
    *cu_off = (di_ptr - dataptr);
    return DW_DLV_OK;
}
#if 0 /* FOR DEBUGGING */
/* Just for debug purposes */
void print_sib_offset(Dwarf_Die sibling)
{
    Dwarf_Off sib_off;
    Dwarf_Error error;
    dwarf_dieoffset(sibling,&sib_off,&error);
    fprintf(stderr," SIB OFF = 0x%" DW_PR_XZEROS DW_PR_DUx,sib_off);
}
void print_ptr_offset(Dwarf_CU_Context cu_context,Dwarf_Byte_Ptr di_ptr)
{
    Dwarf_Off ptr_off;
    dwarf_ptr_CU_offset(cu_context,di_ptr,&ptr_off);
    fprintf(stderr," PTR OFF = 0x%" DW_PR_XZEROS DW_PR_DUx,ptr_off);
}
#endif


/*  Validate the sibling DIE. This only makes sense to call
    if the sibling's DIEs have been travsersed and
    dwarf_child() called on each,
    so that the last DIE dwarf_child saw was the last.
    Essentially ensuring that (after such traversal) that we
    are in the same place a sibling attribute would identify.
    In case we return DW_DLV_ERROR, the global offset of the last
    DIE traversed by dwarf_child is returned through *offset

    It is essentially guaranteed that  dbg->de_last_die
    is a stale DIE pointer of a deallocated DIE when we get here.
    It must not be used as a DIE pointer here,
    just as a sort of anonymous pointer that we just check against
    NULL.

    There is a (subtle?) dependence on the fact that when we call this
    the last dwarf_child() call would have been for this sibling.
    Meaning that this works in a depth-first traversal even though there
    is no stack of 'de_last_die' values.

    The check for dbg->de_last_die just ensures sanity.

    If one is switching between normal debug_frame and eh_frame
    (traversing them in tandem, let us say) in a single
    Dwarf_Debug this validator makes no sense.
    It works if one processes a .debug_frame (entirely) and
    then an eh_frame (or vice versa) though.
    Use caution.
*/
int
dwarf_validate_die_sibling(Dwarf_Die sibling,Dwarf_Off *offset)
{
    Dwarf_Debug dbg = 0;
    Dwarf_Error *error = 0;
    Dwarf_Debug_InfoTypes dis = 0;
    CHECK_DIE(sibling, DW_DLV_ERROR);
    dbg = sibling->di_cu_context->cc_dbg;

    dis = sibling->di_is_info? &dbg->de_info_reading: &dbg->de_types_reading;

    *offset = 0;
    if (dis->de_last_die && dis->de_last_di_ptr) {
        if (sibling->di_debug_ptr == dis->de_last_di_ptr) {
            return (DW_DLV_OK);
        }
    }
    /* Calculate global offset used for error reporting */
    dwarf_ptr_CU_offset(sibling->di_cu_context,
        dis->de_last_di_ptr,sibling->di_is_info,offset);
    return (DW_DLV_ERROR);
}

/*  This function does two slightly different things
    depending on the input flag want_AT_sibling.  If
    this flag is true, it checks if the input die has
    a DW_AT_sibling attribute.  If it does it returns
    a pointer to the start of the sibling die in the
    .debug_info section.  Otherwise it behaves the
    same as the want_AT_sibling false case.

    If the want_AT_sibling flag is false, it returns
    a pointer to the immediately adjacent die in the
    .debug_info section.

    Die_info_end points to the end of the .debug_info
    portion for the cu the die belongs to.  It is used
    to check that the search for the next die does not
    cross the end of the current cu.  Cu_info_start points
    to the start of the .debug_info portion for the
    current cu, and is used to add to the offset for
    DW_AT_sibling attributes.  Finally, has_die_child
    is a pointer to a Dwarf_Bool that is set true if
    the present die has children, false otherwise.
    However, in case want_AT_child is true and the die
    has a DW_AT_sibling attribute *has_die_child is set
    false to indicate that the children are being skipped.

    die_info_end  points to the last byte+1 of the cu.  */
static int
_dwarf_next_die_info_ptr(Dwarf_Byte_Ptr die_info_ptr,
    Dwarf_CU_Context cu_context,
    Dwarf_Byte_Ptr die_info_end,
    Dwarf_Byte_Ptr cu_info_start,
    Dwarf_Bool want_AT_sibling,
    Dwarf_Bool * has_die_child,
    Dwarf_Byte_Ptr *next_die_ptr_out,
    Dwarf_Error *error)
{
    Dwarf_Byte_Ptr info_ptr = 0;
    Dwarf_Byte_Ptr abbrev_ptr = 0;
    Dwarf_Unsigned abbrev_code = 0;
    Dwarf_Abbrev_List abbrev_list = 0;
    Dwarf_Half attr = 0;
    Dwarf_Half attr_form = 0;
    Dwarf_Unsigned offset = 0;
    Dwarf_Unsigned utmp = 0;
    Dwarf_Debug dbg = 0;
    Dwarf_Byte_Ptr abbrev_end = 0;
    int lres = 0;

    info_ptr = die_info_ptr;
    DECODE_LEB128_UWORD_CK(info_ptr, utmp,dbg,error,die_info_end);
    abbrev_code = (Dwarf_Unsigned) utmp;
    if (abbrev_code == 0) {
        /*  Should never happen. Tested before we got here. */
        _dwarf_error(dbg, error, DW_DLE_NEXT_DIE_PTR_NULL);
        return DW_DLV_ERROR;
    }


    lres = _dwarf_get_abbrev_for_code(cu_context, abbrev_code,
        &abbrev_list,error);
    if (lres == DW_DLV_ERROR) {
        return lres;
    }
    if (lres == DW_DLV_NO_ENTRY) {
        _dwarf_error(dbg, error, DW_DLE_NEXT_DIE_NO_ABBREV_LIST);
        return DW_DLV_ERROR;
    }
    dbg = cu_context->cc_dbg;

    *has_die_child = abbrev_list->abl_has_child;

    abbrev_ptr = abbrev_list->abl_abbrev_ptr;
    abbrev_end = _dwarf_calculate_abbrev_section_end_ptr(cu_context);

    do {
        Dwarf_Unsigned utmp2;

        DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp2,dbg,error,
            abbrev_end);
        if (utmp2 > DW_AT_hi_user) {
            _dwarf_error(dbg, error, DW_DLE_ATTR_CORRUPT);
            return DW_DLV_ERROR;
        }
        attr = (Dwarf_Half) utmp2;
        DECODE_LEB128_UWORD_CK(abbrev_ptr, utmp2,dbg,error,
            abbrev_end);
        if (!_dwarf_valid_form_we_know(utmp2,attr)) {
            _dwarf_error(dbg, error, DW_DLE_UNKNOWN_FORM);
            return DW_DLV_ERROR;
        }
        attr_form = (Dwarf_Half) utmp2;
        if (attr_form == DW_FORM_indirect) {
            Dwarf_Unsigned utmp6;

            /* DECODE_LEB128_UWORD updates info_ptr */
            DECODE_LEB128_UWORD_CK(info_ptr, utmp6,dbg,error,
                die_info_end);
            attr_form = (Dwarf_Half) utmp6;
        }
        if (attr_form == DW_FORM_implicit_const) {
            UNUSEDARG Dwarf_Signed cval = 0;

            DECODE_LEB128_SWORD_CK(abbrev_ptr, cval,dbg,error,
                abbrev_end);
        }

        if (want_AT_sibling && attr == DW_AT_sibling) {
            switch (attr_form) {
            case DW_FORM_ref1:
                READ_UNALIGNED_CK(dbg, offset, Dwarf_Unsigned,
                    info_ptr, sizeof(Dwarf_Small),
                    error,die_info_end);
                break;
            case DW_FORM_ref2:
                /* READ_UNALIGNED does not update info_ptr */
                READ_UNALIGNED_CK(dbg, offset, Dwarf_Unsigned,
                    info_ptr,DWARF_HALF_SIZE,
                    error,die_info_end);
                break;
            case DW_FORM_ref4:
                READ_UNALIGNED_CK(dbg, offset, Dwarf_Unsigned,
                    info_ptr, DWARF_32BIT_SIZE,
                    error,die_info_end);
                break;
            case DW_FORM_ref8:
                READ_UNALIGNED_CK(dbg, offset, Dwarf_Unsigned,
                    info_ptr, DWARF_64BIT_SIZE,
                    error,die_info_end);
                break;
            case DW_FORM_ref_udata:
                DECODE_LEB128_UWORD_CK(info_ptr, offset,
                    dbg,error,die_info_end);
                break;
            case DW_FORM_ref_addr:
                /*  Very unusual.  The FORM is intended to refer to
                    a different CU, but a different CU cannot
                    be a sibling, can it?
                    We could ignore this and treat as if no
                    DW_AT_sibling
                    present.   Or derive the offset from it and if
                    it is in the same CU use it directly.
                    The offset here is *supposed* to be a
                    global offset,
                    so adding cu_info_start is wrong  to any offset
                    we find here unless cu_info_start
                    is zero! Lets pretend there is no DW_AT_sibling
                    attribute.  */
                goto no_sibling_attr;
            default:
                _dwarf_error(dbg, error, DW_DLE_NEXT_DIE_WRONG_FORM);
                return DW_DLV_ERROR;
            }

            /*  Reset *has_die_child to indicate children skipped.  */
            *has_die_child = false;

            /*  A value beyond die_info_end indicates an error. Exactly
                at die_info_end means 1-past-cu-end and simply means we
                are at the end, do not return error. Higher level
                will detect that we are at the end. */
            {   /*  Care required here. Offset can be garbage. */
                ptrdiff_t plen = die_info_end - cu_info_start;
                ptrdiff_t signdoffset = (ptrdiff_t)offset;
                if (signdoffset > plen || signdoffset < 0) {
                    /* Error case, bad DWARF. */
                    _dwarf_error(dbg, error,DW_DLE_SIBLING_OFFSET_WRONG);
                    return DW_DLV_ERROR;
                }
            }
            /* At or before end-of-cu */
            *next_die_ptr_out = cu_info_start + offset;
            return DW_DLV_OK;
        }

        no_sibling_attr:
        if (attr_form != 0 && attr_form != DW_FORM_implicit_const) {
            int res = 0;
            Dwarf_Unsigned sizeofval = 0;
            ptrdiff_t  sizeb = 0;

            res = _dwarf_get_size_of_val(cu_context->cc_dbg,
                attr_form,
                cu_context->cc_version_stamp,
                cu_context->cc_address_size,
                info_ptr,
                cu_context->cc_length_size,
                &sizeofval,
                die_info_end,
                error);
            if(res != DW_DLV_OK) {
                return res;
            }
            /*  It is ok for info_ptr == die_info_end, as we will test
                later before using a too-large info_ptr */
            sizeb = (ptrdiff_t)sizeofval;
            if (sizeb > (die_info_end - info_ptr) ||
                sizeb < 0) {
                _dwarf_error(dbg, error, DW_DLE_NEXT_DIE_PAST_END);
                return DW_DLV_ERROR;
            }
            info_ptr += sizeofval;
            if (info_ptr > die_info_end) {
                /*  More than one-past-end indicates a bug somewhere,
                    likely bad dwarf generation. */
                _dwarf_error(dbg, error, DW_DLE_NEXT_DIE_PAST_END);
                return DW_DLV_ERROR;
            }
        }
    } while (attr != 0 || attr_form != 0);
    *next_die_ptr_out = info_ptr;
    return DW_DLV_OK;
}

/*  Multiple TAGs are in fact compile units.
    Allow them all.
    Return non-zero if a CU tag.
    Else return 0.
*/
static int
is_cu_tag(int t)
{
    if (t == DW_TAG_compile_unit  ||
        t == DW_TAG_partial_unit  ||
        t == DW_TAG_skeleton_unit ||
        t == DW_TAG_type_unit) {
        return 1;
    }
    return 0;
}

/*  Given a Dwarf_Debug dbg, and a Dwarf_Die die, it returns
    a Dwarf_Die for the sibling of die.  In case die is NULL,
    it returns (thru ptr) a Dwarf_Die for the first die in the current
    cu in dbg.  Returns DW_DLV_ERROR on error.

    It is assumed that every sibling chain including those with
    only one element is terminated with a NULL die, except a
    chain with only a NULL die.

    The algorithm moves from one die to the adjacent one.  It
    returns when the depth of children it sees equals the number
    of sibling chain terminations.  A single count, child_depth
    is used to track the depth of children and sibling terminations
    encountered.  Child_depth is incremented when a die has the
    Has-Child flag set unless the child happens to be a NULL die.
    Child_depth is decremented when a die has Has-Child false,
    and the adjacent die is NULL.  Algorithm returns when
    child_depth is 0.

    **NOTE: Do not modify input die, since it is used at the end.  */
int
dwarf_siblingof(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Die * caller_ret_die, Dwarf_Error * error)
{
    int res = 0;
    Dwarf_Bool is_info = TRUE;
    Dwarf_Debug_InfoTypes dis = 0;

    dis = &dbg->de_info_reading;
    res = _dwarf_siblingof_internal(dbg,die,
        die?die->di_cu_context:dis->de_cu_context,
        is_info,caller_ret_die,error);
    return res;
}
/*  This is the new form, October 2011.  On calling with 'die' NULL,
    we cannot tell if this is debug_info or debug_types, so
    we must be informed!. */
int
dwarf_siblingof_b(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Bool is_info,
    Dwarf_Die * caller_ret_die, Dwarf_Error * error)
{
    int res;
    Dwarf_Debug_InfoTypes dis = 0;

    dis = is_info? &dbg->de_info_reading:
        &dbg->de_types_reading;

    res = _dwarf_siblingof_internal(dbg,die,
        die?die->di_cu_context:dis->de_cu_context,
        is_info,caller_ret_die,error);
    return res;
}
int
_dwarf_siblingof_internal(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_CU_Context context,
    Dwarf_Bool is_info,
    Dwarf_Die * caller_ret_die, Dwarf_Error * error)
{
    Dwarf_Die ret_die = 0;
    Dwarf_Byte_Ptr die_info_ptr = 0;
    Dwarf_Byte_Ptr cu_info_start = 0;

    /* die_info_end points 1-past end of die (once set) */
    Dwarf_Byte_Ptr die_info_end = 0;
    Dwarf_Unsigned abbrev_code = 0;
    Dwarf_Unsigned utmp = 0;
    int lres = 0;
    /* Since die may be NULL, we rely on the input argument. */
    Dwarf_Small *dataptr =  0;

    if (dbg == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return (DW_DLV_ERROR);
    }
    dataptr = is_info? dbg->de_debug_info.dss_data:
        dbg->de_debug_types.dss_data;

    if (die == NULL) {
        /*  Find root die of cu */
        /*  die_info_end is untouched here, need not be set in this
            branch. */
        Dwarf_Off off2 = 0;
        Dwarf_Unsigned headerlen = 0;
        int cres = 0;

        /*  If we've not loaded debug_info
            context will be NULL. */
        if (!context) {
            _dwarf_error(dbg, error, DW_DLE_DBG_NO_CU_CONTEXT);
            return (DW_DLV_ERROR);
        }
        off2 = context->cc_debug_offset;
        cu_info_start = dataptr + off2;
        cres = _dwarf_length_of_cu_header(dbg, off2,is_info,
            &headerlen,error);
        if (cres != DW_DLV_OK) {
            return cres;
        }
        die_info_ptr = cu_info_start + headerlen;
        die_info_end = _dwarf_calculate_info_section_end_ptr(context);

        /*  Recording the CU die pointer so we can later access
            for special FORMs relating to .debug_str_offsets
            and .debug_addr  */
        context->cc_cu_die_offset_present = TRUE;
        context->cc_cu_die_global_sec_offset = off2 + headerlen;
    } else {
        /* Find sibling die. */
        Dwarf_Bool has_child = false;
        Dwarf_Signed child_depth = 0;

        /*  We cannot have a legal die unless debug_info
            was loaded, so
            no need to load debug_info here. */
        CHECK_DIE(die, DW_DLV_ERROR);

        die_info_ptr = die->di_debug_ptr;
        if (*die_info_ptr == 0) {
            return (DW_DLV_NO_ENTRY);
        }
        context = die->di_cu_context;
        cu_info_start = dataptr+ context->cc_debug_offset;
        die_info_end = _dwarf_calculate_info_section_end_ptr(context);

        if ((*die_info_ptr) == 0) {
            return (DW_DLV_NO_ENTRY);
        }
        child_depth = 0;
        do {
            int res2 = 0;
            Dwarf_Byte_Ptr die_info_ptr2 = 0;
            res2 = _dwarf_next_die_info_ptr(die_info_ptr,
                context, die_info_end,
                cu_info_start, true, &has_child,
                &die_info_ptr2,
                error);
            if(res2 != DW_DLV_OK) {
                return res2;
            }
            if (die_info_ptr2 < die_info_ptr) {
                /*  There is something very wrong, our die value
                    decreased.  Bad DWARF. */
                _dwarf_error(dbg, error, DW_DLE_NEXT_DIE_LOW_ERROR);
                return (DW_DLV_ERROR);
            }
            if (die_info_ptr2 > die_info_end) {
                _dwarf_error(dbg, error, DW_DLE_NEXT_DIE_PAST_END);
                return (DW_DLV_ERROR);
            }
            die_info_ptr = die_info_ptr2;

            /*  die_info_end is one past end. Do not read it!
                A test for ``!= die_info_end''  would work as well,
                but perhaps < reads more like the meaning. */
            if (die_info_ptr < die_info_end) {
                if ((*die_info_ptr) == 0 && has_child) {
                    die_info_ptr++;
                    has_child = false;
                }
            }

            /*  die_info_ptr can be one-past-end.  */
            if ((die_info_ptr == die_info_end) ||
                ((*die_info_ptr) == 0)) {
                /* We are at the end of a sibling list.
                    get back to the next containing
                    sibling list (looking for a libling
                    list with more on it).
                    */
                for (;;) {
                    if (child_depth == 0) {
                        /*  Meaning there is no outer list,
                            so stop. */
                        break;
                    }
                    if (die_info_ptr == die_info_end) {
                        /*  September 2016: do not deref
                            if we are past end.
                            If we are at end at this point
                            it means the sibling list
                            inside this CU is not properly
                            terminated.
                            August 2019:
                            We used to declare an error,
                            DW_DLE_SIBLING_LIST_IMPROPER but
                            now we just silently
                            declare this is the end of the list.
                            Each level of a sibling nest should
                            have a single NUL byte, but here
                            things are wrong, the DWARF
                            is corrupt.  */
                        return DW_DLV_NO_ENTRY;
                    }
                    if (*die_info_ptr) {
                        /* We have a real sibling. */
                        break;
                    }
                    /*  Move out one DIE level.
                        Move past NUL byte marking end of
                        this sibling list. */
                    child_depth--;
                    die_info_ptr++;
                }
            } else {
                child_depth = has_child ? child_depth + 1 : child_depth;
            }
        } while (child_depth != 0);
    }

    /*  die_info_ptr > die_info_end is really a bug (possibly in dwarf
        generation)(but we are past end, no more DIEs here), whereas
        die_info_ptr == die_info_end means 'one past end, no more DIEs
        here'. */
    if (die_info_ptr >= die_info_end) {
        return (DW_DLV_NO_ENTRY);
    }
    if ((*die_info_ptr) == 0) {
        return (DW_DLV_NO_ENTRY);
    }

    ret_die = (Dwarf_Die) _dwarf_get_alloc(dbg, DW_DLA_DIE, 1);
    if (ret_die == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return (DW_DLV_ERROR);
    }

    ret_die->di_is_info = is_info;
    ret_die->di_debug_ptr = die_info_ptr;
    ret_die->di_cu_context =
        die == NULL ? context : die->di_cu_context;

    DECODE_LEB128_UWORD_CK(die_info_ptr, utmp,dbg,error,die_info_end);
    if (die_info_ptr > die_info_end) {
        /*  We managed to go past the end of the CU!.
            Something is badly wrong. */
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        _dwarf_error(dbg, error, DW_DLE_ABBREV_DECODE_ERROR);
        return (DW_DLV_ERROR);
    }
    abbrev_code = (Dwarf_Unsigned) utmp;
    if (abbrev_code == 0) {
        /* Zero means a null DIE */
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        return (DW_DLV_NO_ENTRY);
    }
    ret_die->di_abbrev_code = abbrev_code;
    lres = _dwarf_get_abbrev_for_code(ret_die->di_cu_context, abbrev_code,
        &ret_die->di_abbrev_list,error);
    if (lres == DW_DLV_ERROR) {
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        return lres;
    }
    if (lres == DW_DLV_NO_ENTRY) {
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        _dwarf_error(dbg, error, DW_DLE_DIE_ABBREV_LIST_NULL);
        return DW_DLV_ERROR;
    }
    if (die == NULL && !is_cu_tag(ret_die->di_abbrev_list->abl_tag)) {
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        _dwarf_error(dbg, error, DW_DLE_FIRST_DIE_NOT_CU);
        return DW_DLV_ERROR;
    }

    *caller_ret_die = ret_die;
    return (DW_DLV_OK);
}


int
dwarf_child(Dwarf_Die die,
    Dwarf_Die * caller_ret_die,
    Dwarf_Error * error)
{
    Dwarf_Byte_Ptr die_info_ptr = 0;
    Dwarf_Byte_Ptr die_info_ptr2 = 0;

    /* die_info_end points one-past-end of die area. */
    Dwarf_Byte_Ptr die_info_end = 0;
    Dwarf_Die ret_die = 0;
    Dwarf_Bool has_die_child = 0;
    Dwarf_Debug dbg;
    Dwarf_Unsigned abbrev_code = 0;
    Dwarf_Unsigned utmp = 0;
    Dwarf_Debug_InfoTypes dis = 0;
    int res = 0;
    Dwarf_CU_Context context = 0;
    int lres = 0;

    CHECK_DIE(die, DW_DLV_ERROR);
    dbg = die->di_cu_context->cc_dbg;
    dis = die->di_is_info? &dbg->de_info_reading:
        &dbg->de_types_reading;
    die_info_ptr = die->di_debug_ptr;

    /*  We are saving a DIE pointer here, but the pointer
        will not be presumed live later, when it is tested. */
    dis->de_last_die = die;
    dis->de_last_di_ptr = die_info_ptr;

    /* NULL die has no child. */
    if ((*die_info_ptr) == 0) {
        return DW_DLV_NO_ENTRY;
    }
    context = die->di_cu_context;
    die_info_end = _dwarf_calculate_info_section_end_ptr(context);

    res = _dwarf_next_die_info_ptr(die_info_ptr,
        die->di_cu_context,
        die_info_end,
        NULL, false,
        &has_die_child,
        &die_info_ptr2,
        error);
    if(res != DW_DLV_OK) {
        return res;
    }
    if (die_info_ptr == die_info_end) {
        return DW_DLV_NO_ENTRY;
    }
    die_info_ptr = die_info_ptr2;

    dis->de_last_di_ptr = die_info_ptr;

    if (!has_die_child) {
        /* Look for end of sibling chain. */
        while (dis->de_last_di_ptr < die_info_end) {
            if (*dis->de_last_di_ptr) {
                break;
            }
            ++dis->de_last_di_ptr;
        }
        return DW_DLV_NO_ENTRY;
    }

    ret_die = (Dwarf_Die) _dwarf_get_alloc(dbg, DW_DLA_DIE, 1);
    if (ret_die == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }
    ret_die->di_debug_ptr = die_info_ptr;
    ret_die->di_cu_context = die->di_cu_context;
    ret_die->di_is_info = die->di_is_info;

    DECODE_LEB128_UWORD_CK(die_info_ptr, utmp,
        dbg,error,die_info_end);
    abbrev_code = (Dwarf_Unsigned) utmp;

    dis->de_last_di_ptr = die_info_ptr;

    if (abbrev_code == 0) {
        /* Look for end of sibling chain */
        while (dis->de_last_di_ptr < die_info_end) {
            if (*dis->de_last_di_ptr) {
                break;
            }
            ++dis->de_last_di_ptr;
        }

        /*  We have arrived at a null DIE, at the end of a CU or the end
            of a list of siblings. */
        *caller_ret_die = 0;
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        return DW_DLV_NO_ENTRY;
    }
    ret_die->di_abbrev_code = abbrev_code;
    lres = _dwarf_get_abbrev_for_code(die->di_cu_context, abbrev_code,
        &ret_die->di_abbrev_list,error);
    if (lres == DW_DLV_ERROR) {
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        return lres;
    }
    if (lres == DW_DLV_NO_ENTRY) {
        dwarf_dealloc(dbg, ret_die, DW_DLA_DIE);
        _dwarf_error(dbg, error, DW_DLE_ABBREV_MISSING);
        return DW_DLV_ERROR;
    }

    *caller_ret_die = ret_die;
    return (DW_DLV_OK);
}

/*  Given a (global, not cu_relative) die offset, this returns
    a pointer to a DIE thru *new_die.
    It is up to the caller to do a
    dwarf_dealloc(dbg,*new_die,DW_DLE_DIE);
    The old form only works with debug_info.
    The new _b form works with debug_info or debug_types.
    */
int
dwarf_offdie(Dwarf_Debug dbg,
    Dwarf_Off offset, Dwarf_Die * new_die, Dwarf_Error * error)
{
    Dwarf_Bool is_info = true;
    return dwarf_offdie_b(dbg,offset,is_info,new_die,error);
}

int
dwarf_offdie_b(Dwarf_Debug dbg,
    Dwarf_Off offset, Dwarf_Bool is_info,
    Dwarf_Die * new_die, Dwarf_Error * error)
{
    Dwarf_CU_Context cu_context = 0;
    Dwarf_Off new_cu_offset = 0;
    Dwarf_Die die = 0;
    Dwarf_Byte_Ptr info_ptr = 0;
    Dwarf_Unsigned abbrev_code = 0;
    Dwarf_Unsigned utmp = 0;
    int lres = 0;
    Dwarf_Debug_InfoTypes dis = 0;
    Dwarf_Byte_Ptr die_info_end = 0;

    if (dbg == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return (DW_DLV_ERROR);
    }
    dis = is_info? &dbg->de_info_reading:
        &dbg->de_types_reading;

    cu_context = _dwarf_find_CU_Context(dbg, offset,is_info);
    if (cu_context == NULL) {
        cu_context = _dwarf_find_offdie_CU_Context(dbg, offset,is_info);
    }

    if (cu_context == NULL) {
        Dwarf_Unsigned section_size = is_info? dbg->de_debug_info.dss_size:
            dbg->de_debug_types.dss_size;
        int res = is_info?_dwarf_load_debug_info(dbg, error):
            _dwarf_load_debug_types(dbg,error);

        if (res != DW_DLV_OK) {
            return res;
        }

        if (dis->de_offdie_cu_context_end != NULL) {
            Dwarf_CU_Context lcu_context =
                dis->de_offdie_cu_context_end;
            new_cu_offset =
                lcu_context->cc_debug_offset +
                lcu_context->cc_length +
                lcu_context->cc_length_size +
                lcu_context->cc_extension_size;
        }


        do {
            if ((new_cu_offset +
                _dwarf_length_of_cu_header_simple(dbg,is_info)) >=
                section_size) {
                _dwarf_error(dbg, error, DW_DLE_OFFSET_BAD);
                return (DW_DLV_ERROR);
            }
            res = _dwarf_make_CU_Context(dbg, new_cu_offset,is_info,
                &cu_context,error);
            if (res != DW_DLV_OK) {
                return res;
            }

            res = finish_up_cu_context_from_cudie(dbg,new_cu_offset,
                cu_context,error);
            if (res == DW_DLV_ERROR) {
                dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
                return res;
            }
            if (res == DW_DLV_NO_ENTRY) {
                dwarf_dealloc(dbg, cu_context, DW_DLA_CU_CONTEXT);
                return res;
            }
            if (dis->de_offdie_cu_context == NULL) {
                dis->de_offdie_cu_context = cu_context;
                dis->de_offdie_cu_context_end = cu_context;
            } else {
                dis->de_offdie_cu_context_end->cc_next = cu_context;
                dis->de_offdie_cu_context_end = cu_context;
            }
            new_cu_offset = new_cu_offset + cu_context->cc_length +
                cu_context->cc_length_size +
                cu_context->cc_extension_size;
        } while (offset >= new_cu_offset);
    }

    die_info_end = _dwarf_calculate_info_section_end_ptr(cu_context);
    die = (Dwarf_Die) _dwarf_get_alloc(dbg, DW_DLA_DIE, 1);
    if (die == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return (DW_DLV_ERROR);
    }
    die->di_cu_context = cu_context;
    die->di_is_info = is_info;

    {
        Dwarf_Small *dataptr = is_info? dbg->de_debug_info.dss_data:
            dbg->de_debug_types.dss_data;
        info_ptr = dataptr + offset;
    }
    die->di_debug_ptr = info_ptr;
    DECODE_LEB128_UWORD_CK(info_ptr, utmp,dbg,error,die_info_end);
    abbrev_code = utmp;
    if (abbrev_code == 0) {
        /* we are at a null DIE (or there is a bug). */
        *new_die = 0;
        dwarf_dealloc(dbg, die, DW_DLA_DIE);
        return DW_DLV_NO_ENTRY;
    }
    die->di_abbrev_code = abbrev_code;
    lres = _dwarf_get_abbrev_for_code(cu_context, abbrev_code,
        &die->di_abbrev_list,error);
    if (lres == DW_DLV_ERROR) {
        dwarf_dealloc(dbg, die, DW_DLA_DIE);
        return lres;
    }
    if (lres == DW_DLV_NO_ENTRY) {
        dwarf_dealloc(dbg, die, DW_DLA_DIE);
        _dwarf_error(dbg, error, DW_DLE_DIE_ABBREV_LIST_NULL);
        return DW_DLV_ERROR;
    }
    *new_die = die;
    return DW_DLV_OK;
}

/*  New March 2016.
    Lets one cross check the abbreviations section and
    the DIE information presented  by dwarfdump -i -G -v. */
int
dwarf_die_abbrev_global_offset(Dwarf_Die die,
    Dwarf_Off       * abbrev_goffset,
    Dwarf_Unsigned  * abbrev_count,
    Dwarf_Error*      error)
{
    Dwarf_Abbrev_List dal = 0;
    Dwarf_Debug dbg = 0;

    CHECK_DIE(die, DW_DLV_ERROR);
    dbg = die->di_cu_context->cc_dbg;
    dal = die->di_abbrev_list;
    if(!dal) {
        _dwarf_error(dbg,error,DW_DLE_DWARF_ABBREV_NULL);
        return DW_DLV_ERROR;
    }
    *abbrev_goffset = dal->abl_goffset;
    *abbrev_count = dal->abl_count;
    return DW_DLV_OK;
}


/*  New August 2018.
    Because some real compressed sections
    have .zdebug instead
    of .debug as the leading characters.
    actual_sec_name_out points to a static
    string so so not free it. */
int
dwarf_get_real_section_name(Dwarf_Debug dbg,
    const char  *std_section_name,
    const char **actual_sec_name_out,
    Dwarf_Small *marked_zcompressed, /* zdebug */
    Dwarf_Small *marked_zlib_compressed, /* ZLIB string */
    Dwarf_Small *marked_shf_compressed, /* SHF_COMPRESSED */
    Dwarf_Unsigned *compressed_length,
    Dwarf_Unsigned *uncompressed_length,
    Dwarf_Error *error)
{
    unsigned i = 0;
    char tbuf[50];
    unsigned std_sec_name_len = strlen(std_section_name);

    tbuf[0] = 0;
    /*  std_section_name never has the .dwo on the end,
        so allow for that and allow one (arbitrarily) more. */
    if ((std_sec_name_len + 5) < sizeof(tbuf)) {
        strcpy(tbuf,std_section_name);
        strcpy(tbuf+std_sec_name_len,".dwo");
    }
    if (dbg == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return (DW_DLV_ERROR);
    }
    for (i=0; i < dbg->de_debug_sections_total_entries; i++) {
        struct Dwarf_dbg_sect_s *sdata = &dbg->de_debug_sections[i];
        struct Dwarf_Section_s *section = sdata->ds_secdata;
        const char *std = section->dss_standard_name;

        if (!strcmp(std,std_section_name) ||
            !strcmp(std,tbuf)) {
            const char *used = section->dss_name;
            *actual_sec_name_out = used;
            if (sdata->ds_have_zdebug) {
                *marked_zcompressed = TRUE;
            }
            if (section->dss_ZLIB_compressed) {
                *marked_zlib_compressed = TRUE;
                if (uncompressed_length) {
                    *uncompressed_length =
                        section->dss_uncompressed_length;
                }
                if (compressed_length) {
                    *compressed_length =
                        section->dss_compressed_length;
                }
            }
            if (section->dss_shf_compressed) {
                *marked_shf_compressed = TRUE;
                if (uncompressed_length) {
                    *uncompressed_length =
                        section->dss_uncompressed_length;
                }
                if (compressed_length) {
                    *compressed_length =
                        section->dss_compressed_length;
                }
            }
            return DW_DLV_OK;
        }
    }
    return DW_DLV_NO_ENTRY;
}
/*  This is useful when printing DIE data.
    The string pointer returned must not be freed.
    With non-elf objects it is possible the
    string returned might be empty or NULL,
    so callers should be prepared for that kind
    of return. */
int
dwarf_get_die_section_name(Dwarf_Debug dbg,
    Dwarf_Bool    is_info,
    const char ** sec_name,
    Dwarf_Error * error)
{
    struct Dwarf_Section_s *sec = 0;

    if (dbg == NULL) {
        _dwarf_error(NULL, error, DW_DLE_DBG_NULL);
        return (DW_DLV_ERROR);
    }
    if (is_info) {
        sec = &dbg->de_debug_info;
    } else {
        sec = &dbg->de_debug_types;
    }
    if (sec->dss_size == 0) {
        /* We don't have such a  section at all. */
        return DW_DLV_NO_ENTRY;
    }
    *sec_name = sec->dss_name;
    return DW_DLV_OK;
}

/* This one assumes is_info not known to caller but a DIE is known. */
int
dwarf_get_die_section_name_b(Dwarf_Die die,
    const char ** sec_name,
    Dwarf_Error * error)
{
    Dwarf_CU_Context context = 0;
    Dwarf_Bool is_info = 0;
    Dwarf_Debug dbg = 0;

    CHECK_DIE(die, DW_DLV_ERROR);
    context = die->di_cu_context;
    dbg = context->cc_dbg;
    is_info = context->cc_is_info;
    return dwarf_get_die_section_name(dbg,is_info,sec_name,error);
}

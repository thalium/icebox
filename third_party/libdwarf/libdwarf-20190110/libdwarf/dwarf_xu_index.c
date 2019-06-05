/*
  Copyright (C) 2014-2018 David Anderson. All Rights Reserved.

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

/*  The file and functions have  'xu' because
    the .debug_cu_index and .debug_tu_index
    sections have the same layout and this deals with both.

    This is DebugFission, part of DWARF5.

    It allows properly reading a .dwp object file
    with debug-information (no code).
*/



#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include "dwarf_incl.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#include "dwarf_xu_index.h"

#define  HASHSIGNATURELEN 8
#define  LEN32BIT   4

#define TRUE 1
#define FALSE 0

/* zerohashkey used as all-zero-bits for comparison. */
static Dwarf_Sig8 zerohashkey;

int
dwarf_get_xu_index_header(Dwarf_Debug dbg,
    /* Pass in section_type "tu" or "cu" */
    const char *     section_type,
    Dwarf_Xu_Index_Header * xuptr,
    Dwarf_Unsigned * version,
    Dwarf_Unsigned * number_of_columns, /* L */
    Dwarf_Unsigned * number_of_CUs,     /* N */
    Dwarf_Unsigned * number_of_slots,   /* M */
    const char    ** section_name,
    Dwarf_Error    * error)
{
    Dwarf_Xu_Index_Header indexptr = 0;
    int res = DW_DLV_ERROR;
    struct Dwarf_Section_s *sect = 0;
    Dwarf_Unsigned local_version = 0;
    Dwarf_Unsigned num_col  = 0;
    Dwarf_Unsigned num_CUs  = 0;
    Dwarf_Unsigned num_slots  = 0;
    Dwarf_Small *data = 0;
    Dwarf_Unsigned tables_end_offset = 0;
    Dwarf_Unsigned hash_tab_offset = 0;
    Dwarf_Unsigned indexes_tab_offset = 0;
    Dwarf_Unsigned section_offsets_tab_offset = 0;
    Dwarf_Unsigned section_sizes_tab_offset = 0;
    unsigned datalen32 = LEN32BIT;
    Dwarf_Small *section_end = 0;

    if (!strcmp(section_type,"cu") ) {
        sect = &dbg->de_debug_cu_index;
    } else if (!strcmp(section_type,"tu") ) {
        sect = &dbg->de_debug_tu_index;
    } else {
        _dwarf_error(dbg, error, DW_DLE_XU_TYPE_ARG_ERROR);
        return DW_DLV_ERROR;
    }
    if (!sect->dss_size) {
        return DW_DLV_NO_ENTRY;
    }

    if (!sect->dss_data) {
        res = _dwarf_load_section(dbg, sect,error);
        if (res != DW_DLV_OK) {
            return res;
        }
    }

    data = sect->dss_data;
    section_end = data + sect->dss_size;

    if (sect->dss_size < (4*datalen32) ) {
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }
    READ_UNALIGNED_CK(dbg,local_version, Dwarf_Unsigned,
        data,datalen32,
        error,section_end);
    data += datalen32;
    READ_UNALIGNED_CK(dbg,num_col, Dwarf_Unsigned,
        data,datalen32,
        error,section_end);
    data += datalen32;
    READ_UNALIGNED_CK(dbg,num_CUs, Dwarf_Unsigned,
        data,datalen32,
        error,section_end);
    data += datalen32;
    READ_UNALIGNED_CK(dbg,num_slots, Dwarf_Unsigned,
        data,datalen32,
        error,section_end);
    data += datalen32;
    hash_tab_offset = datalen32*4;
    indexes_tab_offset = hash_tab_offset + (num_slots * HASHSIGNATURELEN);
    /*  Look for corrupt section data. */
    if (num_slots > sect->dss_size) {
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }
    if ( (4*num_slots) > sect->dss_size) {
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }

    section_offsets_tab_offset = indexes_tab_offset +
        (num_slots *datalen32);

    if ( num_col > sect->dss_size) {
        /* Something is badly wrong here. */
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }
    if ( (4*num_col) > sect->dss_size) {
        /* Something is badly wrong here. */
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }
    section_sizes_tab_offset = section_offsets_tab_offset +
        ((num_CUs +1) *num_col* datalen32) ;
    tables_end_offset = section_sizes_tab_offset +
        ((num_CUs   ) *num_col* datalen32);

    if ( tables_end_offset > sect->dss_size) {
        /* Something is badly wrong here. */
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }


    indexptr = (Dwarf_Xu_Index_Header)_dwarf_get_alloc(dbg,DW_DLA_XU_INDEX,1);
    if (indexptr == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return (DW_DLV_ERROR);
    }
    /*  Only "cu" or "tu" allowed, that is checked above.
        But for safety we just copy the allowed bytes*/
    indexptr->gx_type[0] = section_type[0];
    indexptr->gx_type[1] = section_type[1];
    indexptr->gx_type[2] = 0;
    indexptr->gx_dbg = dbg;
    indexptr->gx_section_length = sect->dss_size;
    indexptr->gx_section_data   = sect->dss_data;
    indexptr->gx_section_name   = sect->dss_name;
    indexptr->gx_version        = local_version;
    indexptr->gx_column_count_sections = num_col;
    indexptr->gx_units_in_index = num_CUs;
    indexptr->gx_slots_in_hash  = num_slots;
    indexptr->gx_hash_table_offset  =  hash_tab_offset;
    indexptr->gx_index_table_offset  = indexes_tab_offset;
    indexptr->gx_section_offsets_offset  = section_offsets_tab_offset;
    indexptr->gx_section_sizes_offset  = section_sizes_tab_offset;

    *xuptr             =     indexptr;
    *version           = indexptr->gx_version;
    *number_of_columns = indexptr->gx_column_count_sections;
    *number_of_CUs     = indexptr->gx_units_in_index;
    *number_of_slots   = indexptr->gx_slots_in_hash;
    *section_name      = indexptr->gx_section_name;
    return DW_DLV_OK;
}



int dwarf_get_xu_index_section_type(Dwarf_Xu_Index_Header xuhdr,
    /*  the function returns a pointer to
        the immutable string "tu" or "cu" via this arg. Do not free.  */
    const char ** typename,
    /*  the function returns a pointer to
        the immutable section name. Do not free.
        .debug_cu_index or .debug_tu_index */
    const char ** sectionname,
    UNUSEDARG Dwarf_Error * err)
{
    *typename    = &xuhdr->gx_type[0];
    *sectionname = xuhdr->gx_section_name;
    return DW_DLV_OK;
}

/*  Index values 0 to M-1 are valid. */
int dwarf_get_xu_hash_entry(Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned    index,
    /* returns the hash value. 64 bits. */
    Dwarf_Sig8     *  hash_value,

    /* returns the index into rows of offset/size tables. */
    Dwarf_Unsigned *  index_to_sections,
    Dwarf_Error *     err)
{
    Dwarf_Debug dbg = xuhdr->gx_dbg;
    Dwarf_Small *hashtab = xuhdr->gx_section_data +
        xuhdr->gx_hash_table_offset;
    Dwarf_Small *indextab = xuhdr->gx_section_data +
        xuhdr->gx_index_table_offset;
    Dwarf_Small *indexentry = 0;
    Dwarf_Small *hashentry = 0;
    Dwarf_Sig8 hashval;
    Dwarf_Unsigned indexval = 0;
    Dwarf_Small *section_end = xuhdr->gx_section_data +
        xuhdr->gx_section_length;

    memset(&hashval,0,sizeof(hashval));
    if (xuhdr->gx_slots_in_hash > 0) {
        if (index >= xuhdr->gx_slots_in_hash) {
            _dwarf_error(dbg, err,  DW_DLE_XU_HASH_ROW_ERROR);
            return DW_DLV_ERROR;
        }
        hashentry = hashtab + (index * HASHSIGNATURELEN);
        memcpy(&hashval,hashentry,sizeof(hashval));
    } else {
        _dwarf_error(dbg, err,  DW_DLE_XU_HASH_ROW_ERROR);
        return DW_DLV_ERROR;
    }

    indexentry = indextab + (index * LEN32BIT);
    memcpy(hash_value,&hashval,sizeof(hashval));
    READ_UNALIGNED_CK(dbg,indexval,Dwarf_Unsigned, indexentry,
        LEN32BIT,
        err,section_end);
    if (indexval > xuhdr->gx_units_in_index) {
        _dwarf_error(dbg, err,  DW_DLE_XU_HASH_INDEX_ERROR);
        return DW_DLV_ERROR;
    }

    *index_to_sections = indexval;
    return DW_DLV_OK;
}


static const char * dwp_secnames[] = {
"No name for zero",
"DW_SECT_INFO"        /* 1 */ /*".debug_info.dwo"*/,
"DW_SECT_TYPES"       /* 2 */ /*".debug_types.dwo"*/,
"DW_SECT_ABBREV"      /* 3 */ /*".debug_abbrev.dwo"*/,
"DW_SECT_LINE"        /* 4 */ /*".debug_line.dwo"*/,
"DW_SECT_LOC"         /* 5 */ /*".debug_loc.dwo"*/,
"DW_SECT_STR_OFFSETS" /* 6 */ /*".debug_str_offsets.dwo"*/,
"DW_SECT_MACRO"       /* 7 */ /*".debug_macro.dwo"*/,
"DW_SECT_RNGLISTS"       /* 8 */ /*".debug_rnglists.dwo"*/,
"No name > 8",
};

/*  Row 0 of the Table of Section Offsets,
    columns 0 to L-1,  are the section id's,
    and names, such as DW_SECT_INFO (ie, 1)  */
int
dwarf_get_xu_section_names(Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned  column_index,
    Dwarf_Unsigned* number,
    const char **   name,
    Dwarf_Error *   err)
{
    Dwarf_Unsigned sec_num = 0;
    Dwarf_Small *section_end = xuhdr->gx_section_data +
        xuhdr->gx_section_length;


    Dwarf_Debug dbg = xuhdr->gx_dbg;
    Dwarf_Small *namerow =  xuhdr->gx_section_offsets_offset +
        xuhdr->gx_section_data;
    Dwarf_Small *nameloc =  0;
    if( column_index >= xuhdr->gx_column_count_sections) {
        _dwarf_error(dbg, err, DW_DLE_XU_NAME_COL_ERROR);
        return DW_DLV_ERROR;
    }
    nameloc = namerow + LEN32BIT *column_index;
    READ_UNALIGNED_CK(dbg,sec_num,Dwarf_Unsigned, nameloc,
        LEN32BIT,
        err,section_end);
    if (sec_num > DW_SECT_RNGLISTS) {
        _dwarf_error(dbg, err, DW_DLE_XU_NAME_COL_ERROR);
        return DW_DLV_ERROR;
    }
    if (sec_num < 1) {
        return DW_DLV_NO_ENTRY;
    }
    *number = sec_num;
    *name =  dwp_secnames[sec_num];
    return DW_DLV_OK;
}


/*  Rows 1 to N
    col 0 to L-1
    are section offset and length values from
    the Table of Section Offsets and Table of Section Sizes. */
int
dwarf_get_xu_section_offset(Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned  row_index,
    Dwarf_Unsigned  column_index,
    Dwarf_Unsigned* sec_offset,
    Dwarf_Unsigned* sec_size,
    Dwarf_Error *   err)
{
    Dwarf_Debug dbg = xuhdr->gx_dbg;
    /* get to base of tables first. */
    Dwarf_Small *offsetrow =  xuhdr->gx_section_offsets_offset +
        xuhdr->gx_section_data;
    Dwarf_Small *sizerow =  xuhdr->gx_section_sizes_offset +
        xuhdr->gx_section_data;
    Dwarf_Small *offsetentry = 0;
    Dwarf_Small *sizeentry =  0;
    Dwarf_Unsigned offset = 0;
    Dwarf_Unsigned size = 0;
    Dwarf_Unsigned column_count = xuhdr->gx_column_count_sections;
    Dwarf_Small *section_end = xuhdr->gx_section_data +
        xuhdr->gx_section_length;


    if( row_index > xuhdr->gx_units_in_index) {
        _dwarf_error(dbg, err, DW_DLE_XU_NAME_COL_ERROR);
        return DW_DLV_ERROR;
    }
    if( row_index < 1 ) {
        _dwarf_error(dbg, err, DW_DLE_XU_NAME_COL_ERROR);
        return DW_DLV_ERROR;
    }

    if( column_index >=  xuhdr->gx_column_count_sections) {
        _dwarf_error(dbg, err, DW_DLE_XU_NAME_COL_ERROR);
        return DW_DLV_ERROR;
    }
    offsetrow = offsetrow + (row_index*column_count * LEN32BIT);
    offsetentry = offsetrow + (column_index *  LEN32BIT);

    sizerow = sizerow + ((row_index-1)*column_count * LEN32BIT);
    sizeentry = sizerow + (column_index *  LEN32BIT);

    READ_UNALIGNED_CK(dbg,offset,Dwarf_Unsigned, offsetentry,
        LEN32BIT,err,section_end);
    READ_UNALIGNED_CK(dbg,size,Dwarf_Unsigned, sizeentry,
        LEN32BIT,err,section_end);
    *sec_offset = offset;
    *sec_size =  size;
    return DW_DLV_OK;
}


static int
_dwarf_search_fission_for_key(UNUSEDARG Dwarf_Debug dbg,
    Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Sig8 *key_in,
    Dwarf_Unsigned * percu_index_out,
    Dwarf_Error *error)
{
    Dwarf_Unsigned key = 0;
    Dwarf_Unsigned primary_hash = 0;
    Dwarf_Unsigned hashprime = 0;
    Dwarf_Unsigned slots =  xuhdr->gx_slots_in_hash;
    Dwarf_Unsigned mask = slots -1;
    Dwarf_Sig8 hashentry_key;
    Dwarf_Unsigned percu_index = 0;

    /*  Look for corrupt section data. */
    if (slots > xuhdr->gx_section_length) {
        /* Something is badly wrong here. */
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }
    if ( (4*slots) > xuhdr->gx_section_length) {
        /* Something is badly wrong here. */
        _dwarf_error(dbg, error, DW_DLE_ERRONEOUS_XU_INDEX_SECTION);
        return (DW_DLV_ERROR);
    }
    key = *(Dwarf_Unsigned *)(key_in);
    primary_hash = key & mask;
    hashprime =  (((key >>32) &mask) |1);
    while (1) {
        int res = dwarf_get_xu_hash_entry(xuhdr,
            primary_hash,&hashentry_key,
            &percu_index,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (percu_index == 0 &&
            !memcmp(&hashentry_key,&zerohashkey,sizeof(Dwarf_Sig8))) {
            return DW_DLV_NO_ENTRY;
        }
        if (!memcmp(key_in,&hashentry_key,sizeof(Dwarf_Sig8))) {
            /* FOUND */
            *percu_index_out = percu_index;
            return  DW_DLV_OK;
        }
        primary_hash = (primary_hash + hashprime) % slots;
    }
    /* ASSERT: Cannot get here. */
    return DW_DLV_NO_ENTRY;
}

/* Slow. Consider tsearch. */
/* For type units and for CUs. */
static int
_dwarf_search_fission_for_offset(Dwarf_Debug dbg,
    Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned offset,
    Dwarf_Unsigned dfp_sect_num, /* DW_SECT_INFO or TYPES */
    Dwarf_Unsigned * percu_index_out,
    Dwarf_Sig8 * key_out,
    Dwarf_Error *error)
{
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned m = 0;
    int secnum_index = -1;  /* L index */
    int res = 0;

    for ( i = 0; i< xuhdr->gx_column_count_sections; i++) {
        /*  We could put the secnums array into xuhdr
            if looping here is too slow. */
        const char *name = 0;
        Dwarf_Unsigned num = 0;
        res = dwarf_get_xu_section_names(xuhdr,i,&num,&name,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (num == dfp_sect_num) {
            secnum_index = i;
            break;
        }
    }
    if (secnum_index == -1) {
        _dwarf_error(dbg,error,DW_DLE_FISSION_SECNUM_ERR);
        return DW_DLV_ERROR;
    }
    for ( m = 0; m < xuhdr->gx_slots_in_hash; ++m) {
        Dwarf_Sig8 hash;
        Dwarf_Unsigned indexn = 0;
        Dwarf_Unsigned sec_offset = 0;
        Dwarf_Unsigned sec_size = 0;

        res = dwarf_get_xu_hash_entry(xuhdr,m,&hash,&indexn,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (indexn == 0 &&
            !memcmp(&hash,&zerohashkey,sizeof(Dwarf_Sig8))) {
            /* Empty slot. */
            continue;
        }

        res = dwarf_get_xu_section_offset(xuhdr,
            indexn,secnum_index,&sec_offset,&sec_size,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (sec_offset != offset) {
            continue;
        }
        *percu_index_out = indexn;
        *key_out = hash;
        return DW_DLV_OK;
    }
    return DW_DLV_NO_ENTRY;
}

static int
_dwarf_get_xuhdr(Dwarf_Debug dbg,
   const char *sigtype,
   Dwarf_Xu_Index_Header *xuout,
   Dwarf_Error *error)
{
   if (!strcmp(sigtype,"tu")) {
        if (!dbg->de_tu_hashindex_data) {
            return DW_DLV_NO_ENTRY;
        }
        *xuout = dbg->de_tu_hashindex_data;
    } else if (!strcmp(sigtype,"cu")) {
        if (!dbg->de_cu_hashindex_data) {
            return DW_DLV_NO_ENTRY;
        }
        *xuout = dbg->de_cu_hashindex_data;
    } else {
        _dwarf_error(dbg,error,DW_DLE_SIG_TYPE_WRONG_STRING);
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;

}

static int
transform_xu_to_dfp(Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned percu_index,
    Dwarf_Sig8 *key,
    const char *sig_type,
    Dwarf_Debug_Fission_Per_CU *  percu_out,
    Dwarf_Error *error)
{
    unsigned i = 0;
    unsigned l = 0;
    unsigned n = 1;
    unsigned max_cols = xuhdr->gx_column_count_sections;  /* L */
    unsigned secnums[DW_FISSION_SECT_COUNT];
    int res;
    for ( i = 0; i< max_cols; i++) {
        /*  We could put the secnums array into xuhdr
            if recreating it is too slow. */
        const char *name = 0;
        Dwarf_Unsigned num = 0;
        res = dwarf_get_xu_section_names(xuhdr,i,&num,&name,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        secnums[i] = num;
    }
    n = percu_index;
    for(l = 0; l < max_cols; ++l) {  /* L */
        Dwarf_Unsigned sec_off = 0;
        Dwarf_Unsigned sec_size = 0;
        unsigned l_as_sect = secnums[l];
        res = dwarf_get_xu_section_offset(xuhdr,n,l,
            &sec_off,&sec_size,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        percu_out->pcu_offset[l_as_sect] = sec_off;
        percu_out->pcu_size[l_as_sect] = sec_size;
    }
    percu_out->pcu_type = sig_type;
    percu_out->pcu_index = percu_index;
    percu_out->pcu_hash = *key;
    return DW_DLV_OK;
}

/*  This should only be called for a CU, never a TU.
    For a TU the type hash is known while reading
    the TU Header.  Not so for a CU. */
int
_dwarf_get_debugfission_for_offset(Dwarf_Debug dbg,
    Dwarf_Off    offset_wanted,
    struct Dwarf_Debug_Fission_Per_CU_s *  percu_out,
    Dwarf_Error *error)
{
    Dwarf_Xu_Index_Header xuhdr = 0;
    int sres = 0;
    Dwarf_Unsigned percu_index = 0;
    Dwarf_Unsigned sect_index_base = 0;
    const char * key_type = "cu";
    Dwarf_Sig8 key;

    sect_index_base = DW_SECT_INFO;

    sres = _dwarf_get_xuhdr(dbg,key_type, &xuhdr,error);
    if (sres != DW_DLV_OK) {
        return sres;
    }
    sres = _dwarf_search_fission_for_offset(dbg,
        xuhdr,offset_wanted, sect_index_base, &percu_index,
        &key,
        error);
    if (sres != DW_DLV_OK) {
        return sres;
    }

    sres = transform_xu_to_dfp(xuhdr,percu_index,&key,
        key_type,percu_out, error);
    return sres;

}
int
dwarf_get_debugfission_for_key(Dwarf_Debug dbg,
    Dwarf_Sig8 *  key  /* pointer to hash signature */,
    const char * key_type  /*  "cu" or "tu" */,
    Dwarf_Debug_Fission_Per_CU *  percu_out,
    Dwarf_Error *  error )
{
    Dwarf_Xu_Index_Header xuhdr = 0;
    int sres = 0;
    Dwarf_Unsigned percu_index = 0;

    sres = _dwarf_load_debug_info(dbg,error);
    if (sres == DW_DLV_ERROR) {
        return sres;
    }
    sres = _dwarf_load_debug_types(dbg,error);
    if (sres == DW_DLV_ERROR) {
        return sres;
    }

    sres = _dwarf_get_xuhdr(dbg,key_type, &xuhdr,error);
    if (sres != DW_DLV_OK) {
        return sres;
    }

    sres = _dwarf_search_fission_for_key(dbg,
        xuhdr,key,&percu_index,error);
    if (sres != DW_DLV_OK) {
        return sres;
    }

    sres = transform_xu_to_dfp(xuhdr,percu_index,key,key_type,percu_out,error);
    return sres;
}

void
dwarf_xu_header_free(Dwarf_Xu_Index_Header indexptr)
{
    if(indexptr) {
        Dwarf_Debug dbg = indexptr->gx_dbg;
        dwarf_dealloc(dbg,indexptr,DW_DLA_XU_INDEX);
    }
}

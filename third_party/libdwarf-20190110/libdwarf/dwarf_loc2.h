/*
  Copyright (C) 2000-2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2012 David Anderson. All Rights Reserved.
  Portions Copyright (C) 2010-2012 SN Systems Ltd. All Rights Reserved.

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

int
_dwarf_loc_block_sanity_check(Dwarf_Debug dbg,
    Dwarf_Block_c *loc_block,Dwarf_Error* error)
{
    if (loc_block->bl_from_loclist) {
        Dwarf_Small *loc_ptr = 0;
        Dwarf_Unsigned loc_len = 0;
        Dwarf_Small *end_ptr = 0;

        loc_ptr = loc_block->bl_data;
        loc_len = loc_block->bl_len;
        end_ptr =  dbg->de_debug_loc.dss_size +
            dbg->de_debug_loc.dss_data;
        if ((loc_ptr +loc_len) > end_ptr) {
            _dwarf_error(dbg,error,DW_DLE_DEBUG_LOC_SECTION_SHORT);
            return DW_DLV_ERROR;
        }
    }
    return DW_DLV_OK;
}

/*  #included in dwarf_loc.c to compile.
    This in a separate file to make it easy to
    see the early (pre _c) and current (_c) versions
    at the same time (in distinct windows).

    This synthesizes the ld_lle_value of the locdesc,
    but when it is an actual dwo this value gets
    overridden by our caller with the true ld_lle_value.  */

static int
_dwarf_get_locdesc_op_c(Dwarf_Debug dbg,
    Dwarf_Unsigned locdesc_index,
    Dwarf_Loc_Head_c loc_head,
    Dwarf_Block_c * loc_block,
    Dwarf_Half address_size,
    Dwarf_Half offset_size,
    Dwarf_Small version_stamp,
    Dwarf_Addr lowpc,
    Dwarf_Addr highpc,
    Dwarf_Error * error)
{
    /* Offset of current operator from start of block. */
    Dwarf_Unsigned offset = 0;

    /* Used to chain the Dwarf_Loc_Chain_s structs. */
    Dwarf_Loc_Chain new_loc = NULL;
    Dwarf_Loc_Chain prev_loc = NULL;
    Dwarf_Loc_Chain head_loc = NULL;

    Dwarf_Unsigned op_count = 0;

    /* Contiguous block of Dwarf_Loc's for Dwarf_Locdesc. */
    Dwarf_Loc_c block_loc = 0;
    Dwarf_Locdesc_c locdesc = loc_head->ll_locdesc + locdesc_index;
    Dwarf_Unsigned i = 0;
    int res = 0;
    Dwarf_Small *section_start = 0;
    Dwarf_Unsigned section_size = 0;
    Dwarf_Small *section_end = 0;
    const char *section_name = 0;
    Dwarf_Small *blockdataptr = 0;

    /* ***** BEGIN CODE ***** */
    blockdataptr = loc_block->bl_data;
    if (!blockdataptr || !loc_block->bl_len) {
        /*  an empty block has no operations so
            no section or tests need be done.. */
    } else {
        res = _dwarf_what_section_are_we(dbg,
            blockdataptr,&section_name,&section_start,
            &section_size,&section_end,error);
        if (res != DW_DLV_OK) {
            _dwarf_error(dbg, error,DW_DLE_POINTER_SECTION_UNKNOWN);
            return DW_DLV_ERROR;
        }
        res = _dwarf_loc_block_sanity_check(dbg,loc_block,error);
        if (res != DW_DLV_OK) {
            return res;
        }
    }
    /* New loop getting Loc operators. Non DWO */
    while (offset <= loc_block->bl_len) {
        Dwarf_Unsigned nextoffset = 0;
        struct Dwarf_Loc_c_s temp_loc;

        /*  This call is ok even if bl_data NULL and bl_len 0 */
        res = _dwarf_read_loc_expr_op(dbg,loc_block,
            op_count,
            version_stamp,
            offset_size,
            address_size,
            offset,
            section_end,
            &nextoffset,
            &temp_loc,
            error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /*  Normal end.
                Also the end for an empty loc_block.  */
            break;
        }
        op_count++;
        new_loc =
            (Dwarf_Loc_Chain) _dwarf_get_alloc(dbg, DW_DLA_LOC_CHAIN, 1);
        if (new_loc == NULL) {
            /*  Some memory may leak here.  */
            _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
            return DW_DLV_ERROR;
        }

        /* Copying all the fields. DWARF 2,3,4,5. */
        new_loc->lc_atom    = temp_loc.lr_atom;
        new_loc->lc_opnumber= temp_loc.lr_opnumber;
        new_loc->lc_number  = temp_loc.lr_number;
        new_loc->lc_number2 = temp_loc.lr_number2;
        new_loc->lc_number3 = temp_loc.lr_number3;
        new_loc->lc_offset  = temp_loc.lr_offset;
        offset = nextoffset;

        if (head_loc == NULL)
            head_loc = prev_loc = new_loc;
        else {
            prev_loc->lc_next = new_loc;
            prev_loc = new_loc;
        }
        offset = nextoffset;
    }
    block_loc =
        (Dwarf_Loc_c ) _dwarf_get_alloc(dbg, DW_DLA_LOC_BLOCK_C,
        op_count);
    if (block_loc == NULL) {
        /*  Some memory does leak here.  */
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }

    /* op_count could be zero. */
    new_loc = head_loc;
    for (i = 0; i < op_count; i++) {
        /* Copying only the fields needed by DWARF 2,3,4 */
        (block_loc + i)->lr_atom = new_loc->lc_atom;
        (block_loc + i)->lr_number = new_loc->lc_number;
        (block_loc + i)->lr_number2 = new_loc->lc_number2;
        (block_loc + i)->lr_number3 = new_loc->lc_number3;
        (block_loc + i)->lr_offset = new_loc->lc_offset;
        (block_loc + i)->lr_opnumber = new_loc->lc_opnumber;
        prev_loc = new_loc;
        new_loc = prev_loc->lc_next;
        dwarf_dealloc(dbg, prev_loc, DW_DLA_LOC_CHAIN);
    }
    /* Synthesizing the DW_LLE values. */
    if(highpc == 0 && lowpc == 0) {
        locdesc->ld_lle_value =  DW_LLEX_end_of_list_entry;
    } else if(lowpc == MAX_ADDR) {
        locdesc->ld_lle_value = DW_LLEX_base_address_selection_entry;
    } else {
        locdesc->ld_lle_value = DW_LLEX_offset_pair_entry;
    }
    locdesc->ld_cents = op_count;
    locdesc->ld_s = block_loc;
    locdesc->ld_from_loclist = loc_block->bl_from_loclist;
    locdesc->ld_section_offset = loc_block->bl_section_offset;
    locdesc->ld_locdesc_offset = loc_block->bl_locdesc_offset;
    locdesc->ld_lopc = lowpc;
    locdesc->ld_hipc = highpc;
    return DW_DLV_OK;
}


static int
_dwarf_read_loc_section_dwo(Dwarf_Debug dbg,
    Dwarf_Block_c * return_block,
    Dwarf_Addr * lowpc,
    Dwarf_Addr * highpc,
    Dwarf_Bool *at_end,
    Dwarf_Half * lle_op,
    Dwarf_Off sec_offset,
    Dwarf_Half address_size,
    Dwarf_Error * error)
{
    Dwarf_Small *beg = dbg->de_debug_loc.dss_data + sec_offset;
    Dwarf_Small *locptr = 0;
    Dwarf_Small llecode = 0;
    Dwarf_Unsigned expr_offset  = sec_offset;
    Dwarf_Byte_Ptr section_end = dbg->de_debug_loc.dss_data
        + dbg->de_debug_loc.dss_size;

    if (sec_offset >= dbg->de_debug_loc.dss_size) {
        /* We're at the end. No more present. */
        return DW_DLV_NO_ENTRY;
    }
    memset(return_block,0,sizeof(*return_block));

    /* not the same as non-split loclist, but still a list. */
    return_block->bl_from_loclist = 2;

    return_block->bl_locdesc_offset = sec_offset;
    llecode = *beg;
    locptr = beg +1;
    expr_offset++;
    switch(llecode) {
    case DW_LLEX_end_of_list_entry:
        *at_end = TRUE;
        return_block->bl_section_offset = expr_offset;
        expr_offset++;
        break;
    case DW_LLEX_base_address_selection_entry: {
        Dwarf_Unsigned addr_index = 0;

        DECODE_LEB128_UWORD_CK(locptr,addr_index,
            dbg,error,section_end);
        return_block->bl_section_offset = expr_offset;
        /* So this behaves much like non-dwo loclist */
        *lowpc=MAX_ADDR;
        *highpc=addr_index;
        }
        break;
    case DW_LLEX_start_end_entry: {
        Dwarf_Unsigned addr_indexs = 0;
        Dwarf_Unsigned addr_indexe= 0;
        Dwarf_Unsigned exprlen = 0;
        Dwarf_Word leb128_length = 0;

        DECODE_LEB128_UWORD_LEN_CK(locptr,addr_indexs,
            leb128_length,
            dbg,error,section_end);
        expr_offset += leb128_length;

        DECODE_LEB128_UWORD_LEN_CK(locptr,addr_indexe,
            leb128_length,
            dbg,error,section_end);
        expr_offset +=leb128_length;

        *lowpc=addr_indexs;
        *highpc=addr_indexe;

        READ_UNALIGNED_CK(dbg, exprlen, Dwarf_Unsigned, locptr,
            DWARF_HALF_SIZE,
            error,section_end);
        locptr += DWARF_HALF_SIZE;
        expr_offset += DWARF_HALF_SIZE;

        return_block->bl_len = exprlen;
        return_block->bl_data = locptr;
        return_block->bl_section_offset = expr_offset;

        expr_offset += exprlen;
        if (expr_offset > dbg->de_debug_loc.dss_size) {

            _dwarf_error(NULL, error, DW_DLE_DEBUG_LOC_SECTION_SHORT);
            return DW_DLV_ERROR;
        }
        }
        break;
    case DW_LLEX_start_length_entry: {
        Dwarf_Unsigned addr_index = 0;
        Dwarf_Unsigned  range_length = 0;
        Dwarf_Unsigned exprlen = 0;
        Dwarf_Word leb128_length = 0;

        DECODE_LEB128_UWORD_LEN_CK(locptr,addr_index,
            leb128_length,
            dbg,error,section_end);
        expr_offset +=leb128_length;

        READ_UNALIGNED_CK(dbg, range_length, Dwarf_Unsigned, locptr,
            DWARF_32BIT_SIZE,
            error,section_end);
        locptr += DWARF_32BIT_SIZE;
        expr_offset += DWARF_32BIT_SIZE;

        READ_UNALIGNED_CK(dbg, exprlen, Dwarf_Unsigned, locptr,
            DWARF_HALF_SIZE,
            error,section_end);
        locptr += DWARF_HALF_SIZE;
        expr_offset += DWARF_HALF_SIZE;

        *lowpc = addr_index;
        *highpc = range_length;
        return_block->bl_len = exprlen;
        return_block->bl_data = locptr;
        return_block->bl_section_offset = expr_offset;
        /* exprblock_size can be zero, means no expression */

        expr_offset += exprlen;
        if (expr_offset > dbg->de_debug_loc.dss_size) {
            _dwarf_error(NULL, error, DW_DLE_DEBUG_LOC_SECTION_SHORT);
            return DW_DLV_ERROR;
        }
        }
        break;
    case DW_LLEX_offset_pair_entry: {
        Dwarf_Unsigned  startoffset = 0;
        Dwarf_Unsigned  endoffset = 0;
        Dwarf_Unsigned exprlen = 0;

        READ_UNALIGNED_CK(dbg, startoffset,
            Dwarf_Unsigned, locptr,
            DWARF_32BIT_SIZE,
            error,section_end);
        locptr += DWARF_32BIT_SIZE;
        expr_offset += DWARF_32BIT_SIZE;

        READ_UNALIGNED_CK(dbg, endoffset,
            Dwarf_Unsigned, locptr,
            DWARF_32BIT_SIZE,
            error,section_end);
        locptr += DWARF_32BIT_SIZE;
        expr_offset +=  DWARF_32BIT_SIZE;
        *lowpc= startoffset;
        *highpc = endoffset;

        READ_UNALIGNED_CK(dbg, exprlen, Dwarf_Unsigned, locptr,
            DWARF_HALF_SIZE,
            error,section_end);
        locptr += DWARF_HALF_SIZE;
        expr_offset += DWARF_HALF_SIZE;

        return_block->bl_len = exprlen;
        return_block->bl_data = locptr;
        return_block->bl_section_offset = expr_offset;

        expr_offset += exprlen;
        if (expr_offset > dbg->de_debug_loc.dss_size) {
            _dwarf_error(NULL, error, DW_DLE_DEBUG_LOC_SECTION_SHORT);
            return DW_DLV_ERROR;
        }
        }
        break;
    default:
        _dwarf_error(dbg,error,DW_DLE_LLE_CODE_UNKNOWN);
        return DW_DLV_ERROR;
    }
    *lle_op = llecode;
    return DW_DLV_OK;
}


static int
_dwarf_get_loclist_count_dwo(Dwarf_Debug dbg,
    Dwarf_Off loclist_offset,
    Dwarf_Half address_size,
    int *loclist_count, Dwarf_Error * error)
{
    int count = 0;
    Dwarf_Off offset = loclist_offset;

    for (;;) {
        Dwarf_Block_c b;
        Dwarf_Bool at_end = FALSE;
        Dwarf_Addr lowpc = 0;
        Dwarf_Addr highpc = 0;
        Dwarf_Half lle_op = 0;
        int res = _dwarf_read_loc_section_dwo(dbg, &b,
            &lowpc,
            &highpc,
            &at_end,
            &lle_op,
            offset,
            address_size,
            error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (at_end) {
            count++;
            break;
        }
        offset = b.bl_len + b.bl_section_offset;
        count++;
    }
    *loclist_count = count;
    return DW_DLV_OK;
}

/*  New October 2015
    This interface requires the use of interface functions
    to get data from Dwarf_Locdesc_c.  The structures
    are not visible to callers. */
int
dwarf_get_loclist_c(Dwarf_Attribute attr,
    Dwarf_Loc_Head_c * ll_header_out,
    Dwarf_Unsigned    * listlen_out,
    Dwarf_Error     * error)
{
    Dwarf_Debug dbg;

    /*  Dwarf_Attribute that describes the DW_AT_location in die, if
        present. */
    Dwarf_Attribute loc_attr = attr;

    /* Dwarf_Block that describes a single location expression. */
    Dwarf_Block_c loc_block;

    Dwarf_Half form = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Unsigned     listlen = 0;
    Dwarf_Locdesc_c  llbuf = 0;
    Dwarf_Loc_Head_c llhead = 0;
    Dwarf_CU_Context cucontext = 0;
    unsigned address_size = 0;
    int cuvstamp = 0;
    Dwarf_Bool is_cu = FALSE;

    int blkres = DW_DLV_ERROR;
    int setup_res = DW_DLV_ERROR;

    /* ***** BEGIN CODE ***** */
    setup_res = _dwarf_setup_loc(attr, &dbg,&cucontext, &form, error);
    if (setup_res != DW_DLV_OK) {
        return setup_res;
    }
    cuvstamp = cucontext->cc_version_stamp;
    address_size = cucontext->cc_address_size;
    /*  If this is a form_block then it's a location expression. If it's
        DW_FORM_data4 or DW_FORM_data8  in DWARF2 or DWARF3
        (or in DWARF4 or 5 a DW_FORM_sec_offset) it's a loclist offset */
    /* In final DWARF5 the situation changes . FIXME */
    if (((cuvstamp == DW_CU_VERSION2 || cuvstamp == DW_CU_VERSION3) &&
        (form == DW_FORM_data4 || form == DW_FORM_data8)) ||
        ((cuvstamp == DW_CU_VERSION4 || cuvstamp == DW_CU_VERSION5) &&
        form == DW_FORM_sec_offset)) {
        /* Here we have a loclist to deal with. */
        setup_res = context_is_cu_not_tu(cucontext,&is_cu,error);
        if(setup_res != DW_DLV_OK) {
            return setup_res;
        }
        if (cucontext->cc_is_dwo) {
            /*  dwo loclist. If this were a skeleton CU
                (ie, in the base, not dwo/dwp) then
                it could not have a loclist.  */
            /*  A reference to .debug_loc.dwo, with an offset
                in .debug_loc.dwo of a loclist */
            /* In DWARF5 the situation changes . FIXME */
            Dwarf_Unsigned loclist_offset = 0;
            int off_res  = DW_DLV_ERROR;
            int count_res = DW_DLV_ERROR;
            int loclist_count = 0;
            Dwarf_Unsigned lli = 0;

            off_res = _dwarf_get_loclist_header_start(dbg,
                attr, &loclist_offset, error);
            if (off_res != DW_DLV_OK) {
                return off_res;
            }
            count_res = _dwarf_get_loclist_count_dwo(dbg, loclist_offset,
                address_size, &loclist_count, error);
            if (count_res != DW_DLV_OK) {
                return count_res;
            }
            listlen = loclist_count;
            if (loclist_count == 0) {
                return DW_DLV_NO_ENTRY;
            }

            llhead = (Dwarf_Loc_Head_c)_dwarf_get_alloc(dbg,
                DW_DLA_LOC_HEAD_C, 1);
            if (!llhead) {
                _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
                return (DW_DLV_ERROR);
            }
            listlen = loclist_count;
            llbuf = (Dwarf_Locdesc_c)
                _dwarf_get_alloc(dbg, DW_DLA_LOCDESC_C, listlen);
            if (!llbuf) {
                dwarf_loc_head_c_dealloc(llhead);
                _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
                return (DW_DLV_ERROR);
            }
            llhead->ll_locdesc = llbuf;
            llhead->ll_locdesc_count = listlen;
            llhead->ll_from_loclist = 2;
            llhead->ll_context = cucontext;
            llhead->ll_dbg = dbg;

            /* New get loc ops, DWO version */
            for (lli = 0; lli < listlen; ++lli) {
                int lres = 0;
                Dwarf_Half lle_op = 0;
                Dwarf_Bool at_end = 0;

                blkres = _dwarf_read_loc_section_dwo(dbg, &loc_block,
                    &lowpc,
                    &highpc,
                    &at_end,
                    &lle_op,
                    loclist_offset,
                    address_size,
                    error);
                if (blkres != DW_DLV_OK) {
                    dwarf_loc_head_c_dealloc(llhead);
                    return blkres;
                }
                /* Fills in the locdesc op at index lli */
                lres = _dwarf_get_locdesc_op_c(dbg,
                    lli,
                    llhead,
                    &loc_block,
                    address_size,
                    cucontext->cc_length_size,
                    cucontext->cc_version_stamp,
                    lowpc,
                    highpc,
                    error);
                if (lres != DW_DLV_OK) {
                    dwarf_loc_head_c_dealloc(llhead);
                    /* low level error already set: let it be passed back */
                    return lres;
                }
                /*  Override the syntesized lle value with the
                    real one. */
                llhead->ll_locdesc[lli].ld_lle_value = lle_op;

                /* Now get to next loclist entry offset. */
                loclist_offset = loc_block.bl_section_offset +
                    loc_block.bl_len;
            }
        } else {
            /*  Non-dwo loclist. If this were a skeleton CU
                (ie, in the base, not dwo/dwp) then
                it could not have a loclist.  */
            /*  A reference to .debug_loc, with an offset
                in .debug_loc of a loclist */
            Dwarf_Unsigned loclist_offset = 0;
            int off_res  = DW_DLV_ERROR;
            int count_res = DW_DLV_ERROR;
            int loclist_count = 0;
            Dwarf_Unsigned lli = 0;

            off_res = _dwarf_get_loclist_header_start(dbg,
                attr, &loclist_offset, error);
            if (off_res != DW_DLV_OK) {
                return off_res;
            }
            count_res = _dwarf_get_loclist_count(dbg, loclist_offset,
                address_size, &loclist_count, error);
            listlen = loclist_count;
            if (count_res != DW_DLV_OK) {
                return count_res;
            }
            if (loclist_count == 0) {
                return DW_DLV_NO_ENTRY;
            }
            llhead = (Dwarf_Loc_Head_c)_dwarf_get_alloc(dbg,
                DW_DLA_LOC_HEAD_C, 1);
            if (!llhead) {
                _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
                return (DW_DLV_ERROR);
            }
            listlen = loclist_count;
            llbuf = (Dwarf_Locdesc_c)
                _dwarf_get_alloc(dbg, DW_DLA_LOCDESC_C, listlen);
            if (!llbuf) {
                dwarf_loc_head_c_dealloc(llhead);
                _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
                return (DW_DLV_ERROR);
            }
            llhead->ll_locdesc = llbuf;
            llhead->ll_locdesc_count = listlen;
            llhead->ll_from_loclist = 1;
            llhead->ll_context = cucontext;
            llhead->ll_dbg = dbg;

            /* New locdesc and Loc,  non-DWO, so old format */
            for (lli = 0; lli < listlen; ++lli) {
                int lres = 0;
                Dwarf_Block_c c;
                blkres = _dwarf_read_loc_section(dbg, &c,
                    &lowpc,
                    &highpc,
                    loclist_offset,
                    address_size,
                    error);
                if (blkres != DW_DLV_OK) {
                    dwarf_loc_head_c_dealloc(llhead);
                    return (blkres);
                }
                loc_block.bl_len = c.bl_len;
                loc_block.bl_data = c.bl_data;
                loc_block.bl_from_loclist = c.bl_from_loclist;
                loc_block.bl_section_offset = c.bl_section_offset;
                loc_block.bl_locdesc_offset = loclist_offset;

                /* Fills in the locdesc at index lli */
                lres = _dwarf_get_locdesc_op_c(dbg,
                    lli,
                    llhead,
                    &loc_block,
                    address_size,
                    cucontext->cc_length_size,
                    cucontext->cc_version_stamp,
                    lowpc, highpc,
                    error);
                if (lres != DW_DLV_OK) {
                    dwarf_loc_head_c_dealloc(llhead);
                    /*  low level error already set:
                        let it be passed back */
                    return lres;
                }

                /* Now get to next loclist entry offset. */
                loclist_offset = loc_block.bl_section_offset +
                    loc_block.bl_len;
            }
        }
    } else {
        llhead = (Dwarf_Loc_Head_c)
            _dwarf_get_alloc(dbg, DW_DLA_LOC_HEAD_C, 1);
        if (!llhead) {
            _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
            return (DW_DLV_ERROR);
        }
        if( form == DW_FORM_exprloc) {
            blkres = dwarf_formexprloc(loc_attr,&loc_block.bl_len,
                &loc_block.bl_data,error);
            if(blkres != DW_DLV_OK) {
                dwarf_loc_head_c_dealloc(llhead);
                return blkres;
            }
            loc_block.bl_from_loclist = 0;
            loc_block.bl_section_offset  =
                (char *)loc_block.bl_data -
                (char *)dbg->de_debug_info.dss_data;
            loc_block.bl_locdesc_offset = 0; /* not relevant */
        } else {
            Dwarf_Block *tblock = 0;
            blkres = dwarf_formblock(loc_attr, &tblock, error);
            if (blkres != DW_DLV_OK) {
                dwarf_loc_head_c_dealloc(llhead);
                return (blkres);
            }
            loc_block.bl_len = tblock->bl_len;
            loc_block.bl_data = tblock->bl_data;
            loc_block.bl_from_loclist = tblock->bl_from_loclist;
            loc_block.bl_section_offset = tblock->bl_section_offset;
            loc_block.bl_locdesc_offset = 0; /* not relevant */
            /*  We copied tblock contents to the stack var, so can dealloc
                tblock now.  Avoids leaks. */
            dwarf_dealloc(dbg, tblock, DW_DLA_BLOCK);
        }
        listlen = 1; /* One by definition of a location entry. */
        /*  This hack ensures that the Locdesc_c
            is marked DW_LLEX_start_end_entry */
        lowpc = 0;   /* HACK */
        highpc = (Dwarf_Unsigned) (-1LL); /* HACK */

        llbuf = (Dwarf_Locdesc_c)
            _dwarf_get_alloc(dbg, DW_DLA_LOCDESC_C, listlen);
        if (!llbuf) {
            dwarf_loc_head_c_dealloc(llhead);
            _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
            return (DW_DLV_ERROR);
        }
        llhead->ll_locdesc = llbuf;
        llhead->ll_locdesc = llbuf;
        llhead->ll_locdesc_count = listlen;
        llhead->ll_from_loclist = 0;
        llhead->ll_context = cucontext;
        llhead->ll_dbg = dbg;

        /*  An empty location description (block length 0) means the
            code generator emitted no variable, the variable was not
            generated, it was unused or perhaps never tested after being
            set. Dwarf2, section 2.4.1 In other words, it is not an
            error, and we don't test for block length 0 specially here. */
        /* Fills in the locdesc at index 0 */
        blkres = _dwarf_get_locdesc_op_c(dbg,
            0, /* fake locdesc is index 0 */
            llhead,
            &loc_block,
            address_size,
            cucontext->cc_length_size,
            cucontext->cc_version_stamp,
            lowpc, highpc,
            error);
        if (blkres != DW_DLV_OK) {
            dwarf_loc_head_c_dealloc(llhead);
            /* low level error already set: let it be passed back */
            return blkres;
        }
    }
    *ll_header_out = llhead;
    *listlen_out = listlen;
    return (DW_DLV_OK);
}

/* An interface giving us no cu context! */
int
dwarf_loclist_from_expr_c(Dwarf_Debug dbg,
    Dwarf_Ptr expression_in,
    Dwarf_Unsigned expression_length,
    Dwarf_Half address_size,
    Dwarf_Half offset_size,
    Dwarf_Small dwarf_version,
    Dwarf_Loc_Head_c *loc_head,
    Dwarf_Unsigned * listlen,
    Dwarf_Error * error)
{
    /* Dwarf_Block that describes a single location expression. */
    Dwarf_Block_c loc_block;
    Dwarf_Loc_Head_c llhead = 0;
    Dwarf_Locdesc_c llbuf = 0;
    int local_listlen = 1;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = MAX_ADDR;
    Dwarf_Small version_stamp = dwarf_version;
    int res = 0;

    llhead = (Dwarf_Loc_Head_c)_dwarf_get_alloc(dbg,
        DW_DLA_LOC_HEAD_C, 1);
    if (!llhead) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return (DW_DLV_ERROR);
    }
    memset(&loc_block,0,sizeof(loc_block));
    loc_block.bl_len = expression_length;
    loc_block.bl_data = expression_in;
    loc_block.bl_from_loclist = 0; /* Not from loclist. */
    loc_block.bl_section_offset = 0; /* Fake. Not meaningful. */
    loc_block.bl_locdesc_offset = 0; /* Fake. Not meaningful. */
    llbuf = (Dwarf_Locdesc_c)
        _dwarf_get_alloc(dbg, DW_DLA_LOCDESC_C, local_listlen);
    if (!llbuf) {
        dwarf_loc_head_c_dealloc(llhead);
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return (DW_DLV_ERROR);
    }
    llhead->ll_locdesc = llbuf;
    llhead->ll_locdesc_count = local_listlen;
    llhead->ll_from_loclist = 0;
    llhead->ll_context = 0; /* Not available! */
    llhead->ll_dbg = dbg;

    /* An empty location description (block length 0) means the code
    generator emitted no variable, the variable was not generated,
    it was unused or perhaps never tested after being set. Dwarf2,
    section 2.4.1 In other words, it is not an error, and we don't
    test for block length 0 specially here.  */

    res = _dwarf_get_locdesc_op_c(dbg,
        0,
        llhead,
        &loc_block,
        address_size,
        offset_size,
        version_stamp,
        lowpc,
        highpc,
        error);
    if (res != DW_DLV_OK) {
        /* low level error already set: let it be passed back */
        dwarf_loc_head_c_dealloc(llhead);
        return (DW_DLV_ERROR);
    }
    *loc_head = llhead;
    *listlen = local_listlen;
    return (DW_DLV_OK);
}

int
dwarf_get_locdesc_entry_c(Dwarf_Loc_Head_c loclist_head,
   Dwarf_Unsigned   index,
   Dwarf_Small    * lle_value_out, /* identifies type of loclist entry*/
   Dwarf_Addr     * lowpc_out,
   Dwarf_Addr     * hipc_out,
   Dwarf_Unsigned * loclist_count_out,

   /* Returns pointer to the specific locdesc of the index; */
   Dwarf_Locdesc_c* locdesc_entry_out,
   Dwarf_Small    * loclist_source_out, /* 0,1, or 2 */
   Dwarf_Unsigned * expression_offset_out,
   Dwarf_Unsigned * locdesc_offset_out,
   Dwarf_Error    * error)
{
    Dwarf_Locdesc_c descs_base =  0;
    Dwarf_Locdesc_c desc =  0;
    Dwarf_Unsigned desc_count = 0;
    Dwarf_Debug dbg;

    desc_count = loclist_head->ll_locdesc_count;
    descs_base  = loclist_head->ll_locdesc;
    dbg = loclist_head->ll_dbg;
    if (index >= desc_count) {
        _dwarf_error(dbg, error, DW_DLE_LOCLIST_INDEX_ERROR);
        return (DW_DLV_ERROR);
    }
    desc = descs_base + index;
    *lle_value_out = desc->ld_lle_value;
    *lowpc_out = desc->ld_lopc;
    *hipc_out = desc->ld_hipc;
    *loclist_count_out = desc->ld_cents;
    *locdesc_entry_out = desc;
    *loclist_source_out = desc->ld_from_loclist;
    *expression_offset_out = desc->ld_section_offset;
    *locdesc_offset_out = desc->ld_locdesc_offset;
    return DW_DLV_OK;
}

int
dwarf_get_location_op_value_c(Dwarf_Locdesc_c locdesc,
    Dwarf_Unsigned   index,
    Dwarf_Small    * atom_out,
    Dwarf_Unsigned * operand1,
    Dwarf_Unsigned * operand2,
    Dwarf_Unsigned * operand3,
    Dwarf_Unsigned * offset_for_branch,
    Dwarf_Error*     error)
{
    Dwarf_Loc_c op = 0;
    Dwarf_Unsigned max = locdesc->ld_cents;

    if(index >= max) {
        Dwarf_Debug dbg = locdesc->ld_loclist_head->ll_dbg;
        _dwarf_error(dbg, error, DW_DLE_LOCLIST_INDEX_ERROR);
        return (DW_DLV_ERROR);
    }
    op = locdesc->ld_s + index;
    *atom_out = op->lr_atom;
    *operand1 = op->lr_number;
    *operand2 = op->lr_number2;
    *operand3 = op->lr_number3;
    *offset_for_branch = op->lr_offset;
    return DW_DLV_OK;
}



void
dwarf_loc_head_c_dealloc(Dwarf_Loc_Head_c loclist_head)
{
    Dwarf_Debug dbg = loclist_head->ll_dbg;
    Dwarf_Locdesc_c desc = loclist_head->ll_locdesc;
    if( desc) {
        Dwarf_Unsigned listlen = loclist_head->ll_locdesc_count;
        Dwarf_Unsigned i = 0;
        for ( ; i < listlen; ++i) {
            Dwarf_Loc_c loc = desc[i].ld_s;
            if(loc) {
                dwarf_dealloc(dbg,loc,DW_DLA_LOC_BLOCK_C);
            }
        }
        dwarf_dealloc(dbg,desc,DW_DLA_LOCDESC_C);
    }
    dwarf_dealloc(dbg,loclist_head,DW_DLA_LOC_HEAD_C);
}

/*
  Copyright (C) 2008-2018 David Anderson. All Rights Reserved.
  Portions Copyright 2012 SN Systems Ltd. All rights reserved.

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
#include <stdlib.h>
#include "dwarf_incl.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"

struct ranges_entry {
   struct ranges_entry *next;
   Dwarf_Ranges cur;
};


static void
free_allocated_ranges( struct ranges_entry *base)
{
    struct ranges_entry *cur = 0;
    struct ranges_entry *next = 0;
    for ( cur = base ; cur ; cur = next ) {
        next = cur->next;
        free(cur);
    }
}

/*  We encapsulate the macro use so we can
    free local malloc resources that would otherwise
    leak. See the call points below. */
static int
read_unaligned_addr_check(Dwarf_Debug dbg,
    Dwarf_Addr *addr_out,
    Dwarf_Small *rangeptr,
    unsigned address_size,
    Dwarf_Error *error,
    Dwarf_Small *section_end)
{
    Dwarf_Addr a = 0;

    READ_UNALIGNED_CK(dbg,a,
        Dwarf_Addr, rangeptr,
        address_size,
        error,section_end);
    *addr_out = a;
    return DW_DLV_OK;
}

/*  Ranges are never in a split dwarf object. In the base object
    instead. So use the tied_object if present.
    We return an error which is on the incoming dbg, not
    the possibly-tied-dbg localdbg.
    If incoming die is NULL there is no context, so do not look
    for a tied file, and address_size is the size
    of the overall object, not the address_size of the context. */
#define MAX_ADDR ((address_size == 8)?0xffffffffffffffffULL:0xffffffff)
int dwarf_get_ranges_a(Dwarf_Debug dbg,
    Dwarf_Off rangesoffset,
    Dwarf_Die die,
    Dwarf_Ranges ** rangesbuf,
    Dwarf_Signed * listlen,
    Dwarf_Unsigned * bytecount,
    Dwarf_Error * error)
{
    Dwarf_Small *rangeptr = 0;
    Dwarf_Small *beginrangeptr = 0;
    Dwarf_Small *section_end = 0;
    unsigned entry_count = 0;
    struct ranges_entry *base = 0;
    struct ranges_entry *last = 0;
    struct ranges_entry *curre = 0;
    Dwarf_Ranges * ranges_data_out = 0;
    unsigned copyindex = 0;
    Dwarf_Half address_size = 0;
    int res = DW_DLV_ERROR;
    Dwarf_Unsigned rangebase = 0;
    Dwarf_Debug localdbg = dbg;
    Dwarf_Error localerror = 0;

    if (die &&localdbg->de_tied_data.td_tied_object) {
        /*  ASSERT: localdbg->de_debug_ranges is missing: DW_DLV_NO_ENTRY.
            So lets not look in dbg. */
        Dwarf_CU_Context context = 0;
        int restied = 0;

        context = die->di_cu_context;
        restied = _dwarf_get_ranges_base_attr_from_tied(localdbg,
            context,
            &rangebase,
            error);
        if (restied == DW_DLV_ERROR ) {
            if(!error) {
                return restied;
            }
            dwarf_dealloc(localdbg,*error,DW_DLA_ERROR);
            *error = 0;
            /* Nothing else to do. Look in original dbg. */
        } else if (restied == DW_DLV_NO_ENTRY ) {
            /* Nothing else to do. Look in original dbg. */
        } else {
            /*  Ranges are never in a split dwarf object.
                In the base object
                instead. Use the tied_object */
            localdbg = dbg->de_tied_data.td_tied_object;
        }
    }


    res = _dwarf_load_section(localdbg, &localdbg->de_debug_ranges,&localerror);
    if (res == DW_DLV_ERROR) {
        _dwarf_error_mv_s_to_t(localdbg,&localerror,dbg,error);
        return res;
    } else if (res == DW_DLV_NO_ENTRY) {
        return res;
    }

    if ((rangesoffset +rangebase) >= localdbg->de_debug_ranges.dss_size) {
        _dwarf_error(dbg, error, DW_DLE_DEBUG_RANGES_OFFSET_BAD);
        return (DW_DLV_ERROR);

    }
    address_size = _dwarf_get_address_size(localdbg, die);
    section_end = localdbg->de_debug_ranges.dss_data +
        localdbg->de_debug_ranges.dss_size;
    rangeptr = localdbg->de_debug_ranges.dss_data + rangesoffset+rangebase;
    beginrangeptr = rangeptr;

    for (;;) {
        struct ranges_entry * re = 0;

        if (rangeptr == section_end) {
            break;
        }
        if (rangeptr  > section_end) {
            free_allocated_ranges(base);
            _dwarf_error(dbg, error, DW_DLE_DEBUG_RANGES_OFFSET_BAD);
            return (DW_DLV_ERROR);
            break;
        }
        re = calloc(sizeof(struct ranges_entry),1);
        if (!re) {
            free_allocated_ranges(base);
            _dwarf_error(dbg, error, DW_DLE_DEBUG_RANGES_OUT_OF_MEM);
            return (DW_DLV_ERROR);
        }
        if ((rangeptr + (2*address_size)) > section_end) {
            free(re);
            free_allocated_ranges(base);
            _dwarf_error(dbg, error, DW_DLE_DEBUG_RANGES_OFFSET_BAD);
            return (DW_DLV_ERROR);
        }
        entry_count++;
        res = read_unaligned_addr_check(localdbg,&re->cur.dwr_addr1,
            rangeptr, address_size,error,section_end);
        if (res != DW_DLV_OK) {
            free(re);
            free_allocated_ranges(base);
            return res;
        }
        rangeptr +=  address_size;
        res = read_unaligned_addr_check(localdbg,&re->cur.dwr_addr2,
            rangeptr, address_size,error,section_end);
        if (res != DW_DLV_OK) {
            free(re);
            free_allocated_ranges(base);
            return res;
        }
        rangeptr +=  address_size;
        if (!base) {
            base = re;
            last = re;
        } else {
            last->next = re;
            last = re;
        }
        if (re->cur.dwr_addr1 == 0 && re->cur.dwr_addr2 == 0) {
            re->cur.dwr_type =  DW_RANGES_END;
            break;
        } else if (re->cur.dwr_addr1 == MAX_ADDR) {
            re->cur.dwr_type =  DW_RANGES_ADDRESS_SELECTION;
        } else {
            re->cur.dwr_type =  DW_RANGES_ENTRY;
        }
    }

    /* We return ranges on dbg, so use that to allocate. */
    ranges_data_out =   (Dwarf_Ranges *)
        _dwarf_get_alloc(dbg,DW_DLA_RANGES,entry_count);
    if (!ranges_data_out) {
        /* Error, apply to original, not local dbg. */
        free_allocated_ranges(base);
        _dwarf_error(dbg, error, DW_DLE_DEBUG_RANGES_OUT_OF_MEM);
        return (DW_DLV_ERROR);
    }
    curre = base;
    *rangesbuf = ranges_data_out;
    *listlen = entry_count;
    for (copyindex = 0; curre && (copyindex < entry_count);
        ++copyindex,++ranges_data_out) {

        *ranges_data_out = curre->cur;
        curre = curre->next;
    }
    /* ASSERT: curre == NULL */
    free_allocated_ranges(base);
    base = 0;
    /* Callers will often not care about the bytes used. */
    if (bytecount) {
        *bytecount = rangeptr - beginrangeptr;
    }
    return DW_DLV_OK;
}
int dwarf_get_ranges(Dwarf_Debug dbg,
    Dwarf_Off rangesoffset,
    Dwarf_Ranges ** rangesbuf,
    Dwarf_Signed * listlen,
    Dwarf_Unsigned * bytecount,
    Dwarf_Error * error)
{
    Dwarf_Die die = 0;
    int res = dwarf_get_ranges_a(dbg,rangesoffset,die,
        rangesbuf,listlen,bytecount,error);
    return res;
}

void
dwarf_ranges_dealloc(Dwarf_Debug dbg, Dwarf_Ranges * rangesbuf,
    UNUSEDARG Dwarf_Signed rangecount)
{
    dwarf_dealloc(dbg,rangesbuf, DW_DLA_RANGES);
}

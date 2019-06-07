/*
  Copyright (C) 2000-2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2017 David Anderson. All Rights Reserved.
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

#include "config.h"
#include <stdio.h> /* for debugging only. */
#include "dwarf_incl.h"
#include "dwarf_alloc.h"
#include "dwarf_error.h"
#include "dwarf_util.h"
#include "dwarf_loc.h"

#define TRUE 1
#define FALSE 0

/*  Richard Henderson, on DW_OP_GNU_encoded_addr:
    The operand is an absolute
    address.  The first byte of the value
    is an encoding length: 0 2 4 or 8.  If zero
    it means the following is address-size.
    The address then follows immediately for
    that number of bytes. */
static int
read_encoded_addr(Dwarf_Small *loc_ptr,
   Dwarf_Debug dbg,
   Dwarf_Small *section_end_ptr,
   Dwarf_Unsigned * val_out,
   int * len_out,
   Dwarf_Error *error)
{
    int len = 0;
    Dwarf_Small op = *loc_ptr;
    Dwarf_Unsigned operand = 0;
    len++;
    if (op == 0) {
        /* FIXME: should be CU specific. */
        op = dbg->de_pointer_size;
    }
    switch (op) {
    case 1:
        *val_out = *loc_ptr;
        len++;
        break;

    case 2:
        READ_UNALIGNED_CK(dbg, operand, Dwarf_Unsigned, loc_ptr, 2,
            error,section_end_ptr);
        *val_out = operand;
        len +=2;
        break;
    case 4:
        READ_UNALIGNED_CK(dbg, operand, Dwarf_Unsigned, loc_ptr, 4,
            error,section_end_ptr);
        *val_out = operand;
        len +=4;
        break;
    case 8:
        READ_UNALIGNED_CK(dbg, operand, Dwarf_Unsigned, loc_ptr, 8,
            error,section_end_ptr);
        *val_out = operand;
        len +=8;
        break;
    default:
        /* We do not know how much to read. */
        _dwarf_error(dbg, error, DW_DLE_GNU_OPCODE_ERROR);
        return DW_DLV_ERROR;
    };
    *len_out = len;
    return DW_DLV_OK;
}

/*  Return DW_DLV_NO_ENTRY when at the end of
    the ops for this block (a single Dwarf_Loccesc
    and multiple Dwarf_Locs will eventually result
    from calling this till DW_DLV_NO_ENTRY).

    All op reader code should call this to
    extract operator fields. For any
    DWARF version.
*/
static int
_dwarf_read_loc_expr_op(Dwarf_Debug dbg,
    Dwarf_Block_c * loc_block,
    /* Caller: Start numbering at 0. */
    Dwarf_Signed opnumber,

    /* 2 for DWARF 2 etc. */
    Dwarf_Half version_stamp,
    Dwarf_Half offset_size, /* 4 or 8 */
    Dwarf_Half address_size, /* 2,4, 8  */
    Dwarf_Signed startoffset_in, /* offset in block,
        not section offset */
    Dwarf_Small *section_end,

    /* nextoffset_out so caller knows next entry startoffset */
    Dwarf_Unsigned *nextoffset_out,

    /*  The values picked up. */
    Dwarf_Loc_c curr_loc,
    Dwarf_Error * error)
{
    Dwarf_Small *loc_ptr = 0;
    Dwarf_Unsigned loc_len = 0;
    Dwarf_Unsigned offset = startoffset_in;
    Dwarf_Unsigned operand1 = 0;
    Dwarf_Unsigned operand2 = 0;
    Dwarf_Unsigned operand3 = 0;
    Dwarf_Small atom = 0;
    Dwarf_Word leb128_length = 0;

    if (offset > loc_block->bl_len) {
        _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
        return DW_DLV_ERROR;
    }
    loc_len = loc_block->bl_len;
    if (offset == loc_len) {
        return DW_DLV_NO_ENTRY;
    }

    loc_ptr = (Dwarf_Small*)loc_block->bl_data + offset;
    if ((loc_ptr+1) > section_end) {
        _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
        return DW_DLV_ERROR;
    }
    memset(curr_loc,0,sizeof(*curr_loc));

    curr_loc->lr_opnumber = opnumber;
    curr_loc->lr_offset = offset;

    /*  loc_ptr is ok to deref, see loc_ptr+1 test just above. */
    atom = *(Dwarf_Small *) loc_ptr;
    loc_ptr++;
    offset++;
    curr_loc->lr_atom = atom;
    switch (atom) {

    case DW_OP_reg0:
    case DW_OP_reg1:
    case DW_OP_reg2:
    case DW_OP_reg3:
    case DW_OP_reg4:
    case DW_OP_reg5:
    case DW_OP_reg6:
    case DW_OP_reg7:
    case DW_OP_reg8:
    case DW_OP_reg9:
    case DW_OP_reg10:
    case DW_OP_reg11:
    case DW_OP_reg12:
    case DW_OP_reg13:
    case DW_OP_reg14:
    case DW_OP_reg15:
    case DW_OP_reg16:
    case DW_OP_reg17:
    case DW_OP_reg18:
    case DW_OP_reg19:
    case DW_OP_reg20:
    case DW_OP_reg21:
    case DW_OP_reg22:
    case DW_OP_reg23:
    case DW_OP_reg24:
    case DW_OP_reg25:
    case DW_OP_reg26:
    case DW_OP_reg27:
    case DW_OP_reg28:
    case DW_OP_reg29:
    case DW_OP_reg30:
    case DW_OP_reg31:
        break;

    case DW_OP_regx:
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_lit0:
    case DW_OP_lit1:
    case DW_OP_lit2:
    case DW_OP_lit3:
    case DW_OP_lit4:
    case DW_OP_lit5:
    case DW_OP_lit6:
    case DW_OP_lit7:
    case DW_OP_lit8:
    case DW_OP_lit9:
    case DW_OP_lit10:
    case DW_OP_lit11:
    case DW_OP_lit12:
    case DW_OP_lit13:
    case DW_OP_lit14:
    case DW_OP_lit15:
    case DW_OP_lit16:
    case DW_OP_lit17:
    case DW_OP_lit18:
    case DW_OP_lit19:
    case DW_OP_lit20:
    case DW_OP_lit21:
    case DW_OP_lit22:
    case DW_OP_lit23:
    case DW_OP_lit24:
    case DW_OP_lit25:
    case DW_OP_lit26:
    case DW_OP_lit27:
    case DW_OP_lit28:
    case DW_OP_lit29:
    case DW_OP_lit30:
    case DW_OP_lit31:
        operand1 = atom - DW_OP_lit0;
        break;

    case DW_OP_addr:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned,
            loc_ptr, address_size,
            error,section_end);
        loc_ptr += address_size;
        offset += address_size;
        break;

    case DW_OP_const1u:
        if (loc_ptr >= section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        operand1 = *(Dwarf_Small *) loc_ptr;
        loc_ptr = loc_ptr + 1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + 1;
        break;

    case DW_OP_const1s:
        if (loc_ptr >= section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        operand1 = *(Dwarf_Sbyte *) loc_ptr;
        SIGN_EXTEND(operand1,1);
        loc_ptr = loc_ptr + 1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + 1;
        break;

    case DW_OP_const2u:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 2,
            error,section_end);
        loc_ptr = loc_ptr + 2;
        offset = offset + 2;
        break;

    case DW_OP_const2s:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 2,
            error, section_end);
        SIGN_EXTEND(operand1,2);
        loc_ptr = loc_ptr + 2;
        offset = offset + 2;
        break;

    case DW_OP_const4u:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 4,
            error, section_end);
        loc_ptr = loc_ptr + 4;
        offset = offset + 4;
        break;

    case DW_OP_const4s:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 4,
            error, section_end);
        SIGN_EXTEND(operand1,4);
        loc_ptr = loc_ptr + 4;
        offset = offset + 4;
        break;

    case DW_OP_const8u:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 8,
            error, section_end);
        loc_ptr = loc_ptr + 8;
        offset = offset + 8;
        break;

    case DW_OP_const8s:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 8,
            error, section_end);
        loc_ptr = loc_ptr + 8;
        offset = offset + 8;
        break;

    case DW_OP_constu:
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_consts:
        DECODE_LEB128_SWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_fbreg:
        DECODE_LEB128_SWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_breg0:
    case DW_OP_breg1:
    case DW_OP_breg2:
    case DW_OP_breg3:
    case DW_OP_breg4:
    case DW_OP_breg5:
    case DW_OP_breg6:
    case DW_OP_breg7:
    case DW_OP_breg8:
    case DW_OP_breg9:
    case DW_OP_breg10:
    case DW_OP_breg11:
    case DW_OP_breg12:
    case DW_OP_breg13:
    case DW_OP_breg14:
    case DW_OP_breg15:
    case DW_OP_breg16:
    case DW_OP_breg17:
    case DW_OP_breg18:
    case DW_OP_breg19:
    case DW_OP_breg20:
    case DW_OP_breg21:
    case DW_OP_breg22:
    case DW_OP_breg23:
    case DW_OP_breg24:
    case DW_OP_breg25:
    case DW_OP_breg26:
    case DW_OP_breg27:
    case DW_OP_breg28:
    case DW_OP_breg29:
    case DW_OP_breg30:
    case DW_OP_breg31:
        DECODE_LEB128_SWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_bregx:
        /* uleb reg num followed by sleb offset */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;

        DECODE_LEB128_SWORD_LEN_CK(loc_ptr, operand2,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_dup:
    case DW_OP_drop:
        break;

    case DW_OP_pick:
        if (loc_ptr >= section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        operand1 = *(Dwarf_Small *) loc_ptr;
        loc_ptr = loc_ptr + 1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + 1;
        break;

    case DW_OP_over:
    case DW_OP_swap:
    case DW_OP_rot:
    case DW_OP_deref:
        break;

    case DW_OP_deref_size:
        if (loc_ptr >= section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        operand1 = *(Dwarf_Small *) loc_ptr;
        loc_ptr = loc_ptr + 1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + 1;
        break;

    case DW_OP_xderef:
        break;

    case DW_OP_xderef_type:        /* DWARF5 */
        if (loc_ptr >= section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        operand1 = *(Dwarf_Small *) loc_ptr;
        loc_ptr = loc_ptr + 1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + 1;
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand2,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;

        break;

    case DW_OP_xderef_size:
        if (loc_ptr >= section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        operand1 = *(Dwarf_Small *) loc_ptr;
        loc_ptr = loc_ptr + 1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + 1;
        break;

    case DW_OP_abs:
    case DW_OP_and:
    case DW_OP_div:
    case DW_OP_minus:
    case DW_OP_mod:
    case DW_OP_mul:
    case DW_OP_neg:
    case DW_OP_not:
    case DW_OP_or:
    case DW_OP_plus:
        break;

    case DW_OP_plus_uconst:
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_shl:
    case DW_OP_shr:
    case DW_OP_shra:
    case DW_OP_xor:
        break;

    case DW_OP_le:
    case DW_OP_ge:
    case DW_OP_eq:
    case DW_OP_lt:
    case DW_OP_gt:
    case DW_OP_ne:
        break;

    case DW_OP_skip:
    case DW_OP_bra:
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 2,
            error,section_end);
        loc_ptr = loc_ptr + 2;
        offset = offset + 2;
        break;

    case DW_OP_piece:
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_nop:
        break;
    case DW_OP_push_object_address: /* DWARF3 */
        break;
    case DW_OP_call2:       /* DWARF3 */
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 2,
            error,section_end);
        loc_ptr = loc_ptr + 2;
        offset = offset + 2;
        break;

    case DW_OP_call4:       /* DWARF3 */
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 4,
            error,section_end);
        loc_ptr = loc_ptr + 4;
        offset = offset + 4;
        break;
    case DW_OP_call_ref:    /* DWARF3 */
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr,
            offset_size,
            error,section_end);
        loc_ptr = loc_ptr + offset_size;
        offset = offset + offset_size;
        break;

    case DW_OP_form_tls_address:    /* DWARF3f */
        break;
    case DW_OP_call_frame_cfa:      /* DWARF3f */
        break;
    case DW_OP_bit_piece:   /* DWARF3f */
        /* uleb size in bits followed by uleb offset in bits */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;

        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand2,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

        /*  The operator means: push the currently computed
            (by the operations encountered so far in this
            expression) onto the expression stack as the offset
            in thread-local-storage of the variable. */
    case DW_OP_GNU_push_tls_address: /* 0xe0  */
        /* Believed to have no operands. */
        /* Unimplemented in gdb 7.5.1 ? */
        break;
    case DW_OP_deref_type:     /* DWARF5 */
    case DW_OP_GNU_deref_type: /* 0xf6 */
        if (loc_ptr >= section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        operand1 = *(Dwarf_Small *) loc_ptr;
        loc_ptr = loc_ptr + 1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + 1;

        /* die offset (uleb128). */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand2,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;

    case DW_OP_implicit_value: /* DWARF4 0xa0 */
        /*  uleb length of value bytes followed by that
            number of bytes of the value. */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;

        /*  Second operand is block of 'operand1' bytes of stuff. */
        /*  This using the second operand as a pointer
            is quite ugly. */
        /*  This gets an ugly compiler warning. Sorry. */
        operand2 = (Dwarf_Unsigned)loc_ptr;
        offset = offset + operand1;
        loc_ptr = loc_ptr + operand1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        break;
    case DW_OP_stack_value:  /* DWARF4 */
        break;
    case DW_OP_GNU_uninit:            /* 0xf0 */
        /*  Unimplemented in gdb 7.5.1  */
        /*  Carolyn Tice: Follws a DW_OP_reg or DW_OP_regx
            and marks the reg as being uninitialized. */
        break;
    case DW_OP_GNU_encoded_addr: {      /*  0xf1 */
        /*  Richard Henderson: The operand is an absolute
            address.  The first byte of the value
            is an encoding length: 0 2 4 or 8.  If zero
            it means the following is address-size.
            The address then follows immediately for
            that number of bytes. */
        int length = 0;
            int reares = read_encoded_addr(loc_ptr,dbg,
                section_end,
                &operand1, &length,error);
            if (reares != DW_DLV_OK) {
                return reares;
            }
            loc_ptr += length;
            if (loc_ptr > section_end) {
                _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
                return DW_DLV_ERROR;
            }
            offset  += length;
        }
        break;
    case DW_OP_implicit_pointer:       /* DWARF5 */
    case DW_OP_GNU_implicit_pointer:{  /* 0xf2 */
        /*  Jakub Jelinek: The value is an optimized-out
            pointer value. Represented as
            an offset_size DIE offset
            (a simple unsigned integer) in DWARF3,4
            followed by a signed leb128 offset.
            For DWARF2, it is actually pointer size
            (address size).
            http://www.dwarfstd.org/ShowIssue.php?issue=100831.1 */
        Dwarf_Small iplen = offset_size;
        if (version_stamp == DW_CU_VERSION2 /* 2 */ ) {
            iplen = address_size;
        }
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr,
            iplen,error,section_end);
        loc_ptr = loc_ptr + iplen;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + iplen;

        DECODE_LEB128_SWORD_LEN_CK(loc_ptr, operand2,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        }

        break;
    case DW_OP_entry_value:       /* DWARF5 */
    case DW_OP_GNU_entry_value:       /* 0xf3 */
        /*  Jakub Jelinek: A register reused really soon,
            but the value is unchanged.  So to represent
            that value we have a uleb128 size followed
            by a DWARF expression block that size.
            http://www.dwarfstd.org/ShowIssue.php?issue=100909.1 */

        /*  uleb length of value bytes followed by that
            number of bytes of the value. */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;

        /*  Second operand is block of 'operand1' bytes of stuff. */
        /*  This using the second operand as a pointer
            is quite ugly. */
        /*  This gets an ugly compiler warning. Sorry. */
        operand2 = (Dwarf_Unsigned)loc_ptr;
        offset = offset + operand1;
        loc_ptr = loc_ptr + operand1;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        break;
    case DW_OP_const_type:           /* DWARF5 */
    case DW_OP_GNU_const_type:       /* 0xf4 */
        {
        /* die offset as uleb. */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;

        /*  Next byte is size of following data block.  */
        operand2 = *loc_ptr;
        loc_ptr = loc_ptr + 1;
        offset = offset + 1;

        /*  Operand 3 points to a value in the block of size
            just gotten as operand2. */
        /*  This gets an ugly compiler warning. Sorry. */
        operand3 = (Dwarf_Unsigned) loc_ptr;
        loc_ptr = loc_ptr + operand2;
        if (loc_ptr > section_end) {
            _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
            return DW_DLV_ERROR;
        }
        offset = offset + operand2;
        }
        break;

    case DW_OP_regval_type:           /* DWARF5 */
    case DW_OP_GNU_regval_type:       /* 0xf5 */
        /* reg num uleb*/
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        /* cu die off uleb*/
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand2,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;
    case DW_OP_convert:           /* DWARF5 */
    case DW_OP_GNU_convert:       /* 0xf7 */
    case DW_OP_reinterpret:       /* DWARF5 */
    case DW_OP_GNU_reinterpret:       /* 0xf9 */
        /* die offset  or zero */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;
    case DW_OP_GNU_parameter_ref :       /* 0xfa */
        /* 4 byte unsigned int */
        READ_UNALIGNED_CK(dbg, operand1, Dwarf_Unsigned, loc_ptr, 4,
            error,section_end);;
        loc_ptr = loc_ptr + 4;
        offset = offset + 4;
        break;
    case DW_OP_addrx :           /* DWARF5 */
    case DW_OP_GNU_addr_index :  /* 0xfb DebugFission */
        /*  Index into .debug_addr. The value in .debug_addr
            is an address. */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;
    case DW_OP_constx :          /* DWARF5 */
    case DW_OP_GNU_const_index : /* 0xfc DebugFission */
        /*  Index into .debug_addr. The value in .debug_addr
            is a constant that fits in an address. */
        DECODE_LEB128_UWORD_LEN_CK(loc_ptr, operand1,leb128_length,
            dbg,error,section_end);
        offset = offset + leb128_length;
        break;
    default:
        _dwarf_error(dbg, error, DW_DLE_LOC_EXPR_BAD);
        return DW_DLV_ERROR;
    }
    if (loc_ptr > section_end) {
        _dwarf_error(dbg,error,DW_DLE_LOCEXPR_OFF_SECTION_END);
        return DW_DLV_ERROR;
    }
    /* If offset == loc_len this would be normal end-of-expression. */
    if (offset > loc_len) {
        /*  We stepped past the end of the expression.
            This has to be a compiler bug.
            Operators missing their values cannot be detected
            as such except at the end of an expression (like this).
            The results would be wrong if returned.
        */
        _dwarf_error(dbg, error, DW_DLE_LOC_BAD_TERMINATION);
        return DW_DLV_ERROR;
    }
    curr_loc->lr_atom = atom;
    curr_loc->lr_number =  operand1;
    curr_loc->lr_number2 = operand2;
    curr_loc->lr_number3 = operand3;
    *nextoffset_out = offset;
    return DW_DLV_OK;
}

/*  Given a Dwarf_Block that represents a location expression,
    this function returns a pointer to a Dwarf_Locdesc struct
    that has its ld_cents field set to the number of location
    operators in the block, and its ld_s field pointing to a
    contiguous block of Dwarf_Loc structs.  However, the
    ld_lopc and ld_hipc values are uninitialized.  Returns
    DW_DLV_ERROR on error.
    This function assumes that the length of
    the block is greater than 0.  Zero length location expressions
    to represent variables that have been optimized away are
    handled in the calling function.

    address_size, offset_size, and version_stamp are
    per-CU, not per-object or per dbg.
    We cannot use dbg directly to get those values.

    Use for DWARF 2,3,4. Not for DWARF5.

*/
static int
_dwarf_get_locdesc(Dwarf_Debug dbg,
    Dwarf_Block_c * loc_block,
    Dwarf_Half address_size,
    Dwarf_Half offset_size,
    Dwarf_Small version_stamp,
    Dwarf_Addr lowpc,
    Dwarf_Addr highpc,
    Dwarf_Small * section_end,
    Dwarf_Locdesc ** locdesc_out,
    Dwarf_Error * error)
{
    /* Offset of current operator from start of block. */
    Dwarf_Unsigned offset = 0;

    /* Used to chain the Dwarf_Loc_Chain_s structs. */
    Dwarf_Loc_Chain new_loc = NULL;
    Dwarf_Loc_Chain prev_loc = NULL;
    Dwarf_Loc_Chain head_loc = NULL;

    /* Count of the number of location operators. */
    Dwarf_Unsigned op_count = 0;

    /* Contiguous block of Dwarf_Loc's for Dwarf_Locdesc. */
    Dwarf_Loc *block_loc = 0;

    /* Dwarf_Locdesc pointer to be returned. */
    Dwarf_Locdesc *locdesc = 0;

    Dwarf_Unsigned i = 0;
    int res = 0;

    /* ***** BEGIN CODE ***** */

    offset = 0;
    op_count = 0;


    res = _dwarf_loc_block_sanity_check(dbg,loc_block,error);
    if (res != DW_DLV_OK) {
        return res;
    }

    /* OLD loop getting Loc operators. No DWARF5 */
    while (offset <= loc_block->bl_len) {
        Dwarf_Unsigned nextoffset = 0;
        struct Dwarf_Loc_c_s temp_loc;

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
            /* Normal end. */
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

        /* Copying only the fields needed by DWARF 2,3,4 */
        new_loc->lc_atom    = temp_loc.lr_atom;
        new_loc->lc_opnumber= temp_loc.lr_opnumber;
        new_loc->lc_number  = temp_loc.lr_number;
        new_loc->lc_number2 = temp_loc.lr_number2;
        new_loc->lc_offset  = temp_loc.lr_offset;
        offset = nextoffset;

        if (head_loc == NULL)
            head_loc = prev_loc = new_loc;
        else {
            prev_loc->lc_next = new_loc;
            prev_loc = new_loc;
        }
    }

    block_loc =
        (Dwarf_Loc *) _dwarf_get_alloc(dbg, DW_DLA_LOC_BLOCK, op_count);
    if (block_loc == NULL) {
        /*  Some memory does leak here.  */
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }

    new_loc = head_loc;
    for (i = 0; i < op_count; i++) {
        /* Copying only the fields needed by DWARF 2,3,4 */
        (block_loc + i)->lr_atom = new_loc->lc_atom;
        (block_loc + i)->lr_number = new_loc->lc_number;
        (block_loc + i)->lr_number2 = new_loc->lc_number2;
        (block_loc + i)->lr_offset = new_loc->lc_offset;
        prev_loc = new_loc;
        new_loc = prev_loc->lc_next;
        dwarf_dealloc(dbg, prev_loc, DW_DLA_LOC_CHAIN);
    }

    locdesc =
        (Dwarf_Locdesc *) _dwarf_get_alloc(dbg, DW_DLA_LOCDESC, 1);
    if (locdesc == NULL) {
        _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
        return DW_DLV_ERROR;
    }

    locdesc->ld_cents = op_count;
    locdesc->ld_s = block_loc;
    locdesc->ld_from_loclist = loc_block->bl_from_loclist;
    locdesc->ld_section_offset = loc_block->bl_section_offset;
    locdesc->ld_lopc = lowpc;
    locdesc->ld_hipc = highpc;

    *locdesc_out = locdesc;
    return DW_DLV_OK;
}

/*  Using a loclist offset to get the in-memory
    address of .debug_loc data to read, returns the loclist
    'header' info in return_block.
*/

#define MAX_ADDR ((address_size == 8)?0xffffffffffffffffULL:0xffffffff)


static int
_dwarf_read_loc_section(Dwarf_Debug dbg,
    Dwarf_Block_c * return_block,
    Dwarf_Addr * lowpc, Dwarf_Addr * hipc,
    Dwarf_Off sec_offset,
    Dwarf_Half address_size,
    Dwarf_Error * error)
{
    Dwarf_Small *beg = dbg->de_debug_loc.dss_data + sec_offset;
    Dwarf_Small *loc_section_end =
        dbg->de_debug_loc.dss_data + dbg->de_debug_loc.dss_size;

    /*  start_addr and end_addr are actually offsets
        of the applicable base address of the CU.
        They are address-size. */
    Dwarf_Addr start_addr = 0;
    Dwarf_Addr end_addr = 0;
    Dwarf_Half exprblock_size = 0;
    Dwarf_Unsigned exprblock_off =
        2 * address_size + DWARF_HALF_SIZE;

    if (sec_offset >= dbg->de_debug_loc.dss_size) {
        /* We're at the end. No more present. */
        return DW_DLV_NO_ENTRY;
    }

    /* If it goes past end, error */
    if (exprblock_off > dbg->de_debug_loc.dss_size) {
        _dwarf_error(NULL, error, DW_DLE_DEBUG_LOC_SECTION_SHORT);
        return DW_DLV_ERROR;
    }


    READ_UNALIGNED_CK(dbg, start_addr, Dwarf_Addr, beg, address_size,
        error,loc_section_end);
    READ_UNALIGNED_CK(dbg, end_addr, Dwarf_Addr,
        beg + address_size, address_size,
        error,loc_section_end);
    if (start_addr == 0 && end_addr == 0) {
        /*  If start_addr and end_addr are 0, it's the end and no
            exprblock_size field follows. */
        exprblock_size = 0;
        exprblock_off -= DWARF_HALF_SIZE;
    } else if (start_addr == MAX_ADDR) {
        /*  End address is a base address, no exprblock_size field here
            either */
        exprblock_size = 0;
        exprblock_off -=  DWARF_HALF_SIZE;
    } else {
        READ_UNALIGNED_CK(dbg, exprblock_size, Dwarf_Half,
            beg + 2 * address_size, DWARF_HALF_SIZE,
            error,loc_section_end);
        /* exprblock_size can be zero, means no expression */
        if ( exprblock_size >= dbg->de_debug_loc.dss_size) {
            _dwarf_error(NULL, error, DW_DLE_DEBUG_LOC_SECTION_SHORT);
            return DW_DLV_ERROR;
        }
        if ((sec_offset +exprblock_off + exprblock_size) >
            dbg->de_debug_loc.dss_size) {
            _dwarf_error(NULL, error, DW_DLE_DEBUG_LOC_SECTION_SHORT);
            return DW_DLV_ERROR;
        }
    }
    *lowpc = start_addr;
    *hipc = end_addr;

    return_block->bl_len = exprblock_size;
    return_block->bl_from_loclist = 1;
    return_block->bl_data = beg + exprblock_off;
    return_block->bl_section_offset =
        ((Dwarf_Small *) return_block->bl_data) - dbg->de_debug_loc.dss_data;
    return DW_DLV_OK;
}

static int
_dwarf_get_loclist_count(Dwarf_Debug dbg,
    Dwarf_Off loclist_offset,
    Dwarf_Half address_size,
    int *loclist_count, Dwarf_Error * error)
{
    int count = 0;
    Dwarf_Off offset = loclist_offset;


    for (;;) {
        Dwarf_Block_c b;
        Dwarf_Addr lowpc;
        Dwarf_Addr highpc;
        int res = _dwarf_read_loc_section(dbg, &b,
            &lowpc, &highpc,
            offset, address_size,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        offset = b.bl_len + b.bl_section_offset;
        if (lowpc == 0 && highpc == 0) {
            break;
        }
        count++;
    }
    *loclist_count = count;
    return DW_DLV_OK;
}

/* Helper routine to avoid code duplication.
*/
static int
_dwarf_setup_loc(Dwarf_Attribute attr,
    Dwarf_Debug *     dbg_ret,
    Dwarf_CU_Context *cucontext_ret,
    Dwarf_Half       *form_ret,
    Dwarf_Error      *error)
{
    Dwarf_Debug dbg = 0;
    Dwarf_Half form = 0;
    int blkres = DW_DLV_ERROR;

    if (attr == NULL) {
        _dwarf_error(NULL, error, DW_DLE_ATTR_NULL);
        return (DW_DLV_ERROR);
    }
    if (attr->ar_cu_context == NULL) {
        _dwarf_error(NULL, error, DW_DLE_ATTR_NO_CU_CONTEXT);
        return (DW_DLV_ERROR);
    }
    *cucontext_ret = attr->ar_cu_context;

    dbg = attr->ar_cu_context->cc_dbg;
    if (dbg == NULL) {
        _dwarf_error(NULL, error, DW_DLE_ATTR_DBG_NULL);
        return (DW_DLV_ERROR);
    }
    *dbg_ret = dbg;
    blkres = dwarf_whatform(attr, &form, error);
    if (blkres != DW_DLV_OK) {
        _dwarf_error(dbg, error, DW_DLE_LOC_EXPR_BAD);
        return blkres;
    }
    *form_ret = form;
    return DW_DLV_OK;
}

/* Helper routine  to avoid code duplication.
*/
static int
_dwarf_get_loclist_header_start(Dwarf_Debug dbg,
    Dwarf_Attribute attr,
    Dwarf_Unsigned * loclist_offset_out,
    Dwarf_Error * error)
{
    Dwarf_Unsigned loc_sec_size = 0;
    Dwarf_Unsigned loclist_offset = 0;

    int blkres = dwarf_global_formref(attr, &loclist_offset, error);
    if (blkres != DW_DLV_OK) {
        return (blkres);
    }
    if (!dbg->de_debug_loc.dss_data) {
        int secload = _dwarf_load_section(dbg, &dbg->de_debug_loc,error);
        if (secload != DW_DLV_OK) {
            return secload;
        }
        if (!dbg->de_debug_loc.dss_size) {
            return (DW_DLV_NO_ENTRY);
        }
    }
    loc_sec_size = dbg->de_debug_loc.dss_size;
    if (loclist_offset >= loc_sec_size) {
        _dwarf_error(dbg, error, DW_DLE_LOCLIST_OFFSET_BAD);
        return DW_DLV_ERROR;
    }

    {
        int fisres = 0;
        Dwarf_Unsigned fissoff = 0;
        Dwarf_Unsigned size = 0;
        fisres = _dwarf_get_fission_addition_die(attr->ar_die, DW_SECT_LOCLISTS,
            &fissoff, &size,error);
        if(fisres != DW_DLV_OK) {
            return fisres;
        }
        if (fissoff >= loc_sec_size) {
            _dwarf_error(dbg, error, DW_DLE_LOCLIST_OFFSET_BAD);
            return DW_DLV_ERROR;
        }
        loclist_offset += fissoff;
        if  (loclist_offset >= loc_sec_size) {
            _dwarf_error(dbg, error, DW_DLE_LOCLIST_OFFSET_BAD);
            return DW_DLV_ERROR;
        }
    }
    *loclist_offset_out = loclist_offset;
    return DW_DLV_OK;
}

/* When llbuf (see dwarf_loclist_n) is partially set up
   and an error is encountered, tear it down as it
   won't be used.
*/
static void
_dwarf_cleanup_llbuf(Dwarf_Debug dbg, Dwarf_Locdesc ** llbuf, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        dwarf_dealloc(dbg, llbuf[i]->ld_s, DW_DLA_LOC_BLOCK);
        dwarf_dealloc(dbg, llbuf[i], DW_DLA_LOCDESC);
    }
    dwarf_dealloc(dbg, llbuf, DW_DLA_LIST);
}

static int
context_is_cu_not_tu(Dwarf_CU_Context context,
    Dwarf_Bool *r,Dwarf_Error *err)
{
    Dwarf_Debug dbg = 0;
    if(context->cc_unit_type == DW_UT_type) {
        *r =FALSE;
        return DW_DLV_OK;
    }
    if(context->cc_unit_type == DW_UT_compile) {
        *r = TRUE;
        return DW_DLV_OK;
    }
    if(context->cc_unit_type == DW_UT_partial) {
        *r = TRUE;
        return DW_DLV_OK;
    }
    /* This should be impossible */
    dbg = context->cc_dbg;
    _dwarf_error(dbg,err, DW_DLE_NO_TIED_FILE_AVAILABLE);
    return DW_DLV_ERROR;
}

/*  Handles simple location entries and loclists.
    Returns all the Locdesc's thru llbuf.

    Will not work properly for DWARF5 and may not
    work for some recent versions of gcc or llvm emitting
    DWARF4 with location extensions.

    Does not work for .debug_loc.dwo
*/
int
dwarf_loclist_n(Dwarf_Attribute attr,
    Dwarf_Locdesc *** llbuf_out,
    Dwarf_Signed * listlen_out, Dwarf_Error * error)
{
    Dwarf_Debug dbg = 0;

    /*  Dwarf_Attribute that describes the DW_AT_location in die, if
        present. */
    Dwarf_Attribute loc_attr = attr;

    /* Dwarf_Block that describes a single location expression. */
    Dwarf_Block_c loc_block;

    /* A pointer to the current Dwarf_Locdesc read. */
    Dwarf_Locdesc *locdesc = 0;

    Dwarf_Half form = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Signed listlen = 0;
    Dwarf_Locdesc **llbuf = 0;
    Dwarf_CU_Context cucontext = 0;
    unsigned address_size = 0;
    int cuvstamp = 0;
    Dwarf_Bool is_cu = FALSE;

    int blkres = DW_DLV_ERROR;
    int setup_res = DW_DLV_ERROR;
    Dwarf_Small * info_section_end = 0;

    /* ***** BEGIN CODE ***** */
    setup_res = _dwarf_setup_loc(attr, &dbg,&cucontext, &form, error);
    if (setup_res != DW_DLV_OK) {
        return setup_res;
    }
    info_section_end = _dwarf_calculate_info_section_end_ptr(cucontext);
    cuvstamp = cucontext->cc_version_stamp;
    address_size = cucontext->cc_address_size;
    /*  If this is a form_block then it's a location expression. If it's
        DW_FORM_data4 or DW_FORM_data8  in DWARF2 or DWARF3
        (or in DWARF4 or 5 a DW_FORM_sec_offset) it's a loclist offset */
    if (cuvstamp == DW_CU_VERSION5) {
        /* Use a newer interface. */
        _dwarf_error(dbg, error, DW_DLE_LOCLIST_INTERFACE_ERROR);
        return (DW_DLV_ERROR);
    }
    if (((cuvstamp == DW_CU_VERSION2 || cuvstamp == DW_CU_VERSION3) &&
        (form == DW_FORM_data4 || form == DW_FORM_data8)) ||
        ((cuvstamp == DW_CU_VERSION4) && form == DW_FORM_sec_offset)) {

        /*  A reference to .debug_loc, with an offset in .debug_loc of a
            loclist */
        Dwarf_Small *loc_section_end = 0;
        Dwarf_Unsigned loclist_offset = 0;
        int off_res  = DW_DLV_ERROR;
        int count_res = DW_DLV_ERROR;
        int loclist_count = 0;
        int lli = 0;

        setup_res = context_is_cu_not_tu(cucontext,&is_cu,error);
        if(setup_res != DW_DLV_OK) {
            return setup_res;
        }

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

        llbuf = (Dwarf_Locdesc **)
            _dwarf_get_alloc(dbg, DW_DLA_LIST, loclist_count);
        if (!llbuf) {
            _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
            return (DW_DLV_ERROR);
        }

        loc_section_end = dbg->de_debug_loc.dss_data+
            dbg->de_debug_loc.dss_size;
        for (lli = 0; lli < loclist_count; ++lli) {
            int lres = 0;

            blkres = _dwarf_read_loc_section(dbg, &loc_block,
                &lowpc,
                &highpc,
                loclist_offset,
                address_size,
                error);
            if (blkres != DW_DLV_OK) {
                _dwarf_cleanup_llbuf(dbg, llbuf, lli);
                return (blkres);
            }
            loc_section_end = dbg->de_debug_loc.dss_data+
                dbg->de_debug_loc.dss_size;
            lres = _dwarf_get_locdesc(dbg, &loc_block,
                address_size,
                cucontext->cc_length_size,
                cucontext->cc_version_stamp,
                lowpc, highpc,
                loc_section_end,
                &locdesc,
                error);
            if (lres != DW_DLV_OK) {
                _dwarf_cleanup_llbuf(dbg, llbuf, lli);
                /* low level error already set: let it be passed back */
                return lres;
            }
            llbuf[lli] = locdesc;

            /* Now get to next loclist entry offset. */
            loclist_offset = loc_block.bl_section_offset +
                loc_block.bl_len;
        }
    } else {
        if( form == DW_FORM_exprloc) {
            blkres = dwarf_formexprloc(loc_attr,&loc_block.bl_len,
                &loc_block.bl_data,error);
            if(blkres != DW_DLV_OK) {
                return blkres;
            }
            loc_block.bl_from_loclist = 0;
            loc_block.bl_section_offset  =
                (char *)loc_block.bl_data -
                (char *)dbg->de_debug_info.dss_data;
        } else {
            Dwarf_Block *tblock = 0;
            blkres = dwarf_formblock(loc_attr, &tblock, error);
            if (blkres != DW_DLV_OK) {
                return (blkres);
            }
            loc_block.bl_len = tblock->bl_len;
            loc_block.bl_data = tblock->bl_data;
            loc_block.bl_from_loclist = tblock->bl_from_loclist;
            loc_block.bl_section_offset = tblock->bl_section_offset;
            loc_block.bl_locdesc_offset = 0; /* not relevent */
            /*  We copied tblock contents to the stack var, so can dealloc
                tblock now.  Avoids leaks. */
            dwarf_dealloc(dbg, tblock, DW_DLA_BLOCK);
        }
        listlen = 1; /* One by definition of a location entry. */
        lowpc = 0;   /* HACK */
        highpc = (Dwarf_Unsigned) (-1LL); /* HACK */

        /*  An empty location description (block length 0) means the
            code generator emitted no variable, the variable was not
            generated, it was unused or perhaps never tested after being
            set. Dwarf2, section 2.4.1 In other words, it is not an
            error, and we don't test for block length 0 specially here. */
        blkres = _dwarf_get_locdesc(dbg, &loc_block,
            address_size,
            cucontext->cc_length_size,
            cucontext->cc_version_stamp,
            lowpc, highpc,
            info_section_end,
            &locdesc,
            error);
        if (blkres != DW_DLV_OK) {
            /* low level error already set: let it be passed back */
            return blkres;
        }
        llbuf = (Dwarf_Locdesc **)
            _dwarf_get_alloc(dbg, DW_DLA_LIST, listlen);
        if (!llbuf) {
            /* Free the locdesc we allocated but won't use. */
            dwarf_dealloc(dbg, locdesc, DW_DLA_LOCDESC);
            _dwarf_error(dbg, error, DW_DLE_ALLOC_FAIL);
            return (DW_DLV_ERROR);
        }
        llbuf[0] = locdesc;
    }

    *llbuf_out = llbuf;
    *listlen_out = listlen;
    return (DW_DLV_OK);
}

/*  Handles only a location expression.
    If called on a loclist, just returns one of those.
    Cannot not handle a real loclist.
    It returns the location expression as a loclist with
    a single entry.
    See dwarf_loclist_n() which handles any number
    of location list entries.

    This is the original definition, and it simply
    does not work for loclists.
    Nor does it work on DWARF4 nor on some
    versions of gcc generating DWARF4.
    Kept for compatibility.
*/
int
dwarf_loclist(Dwarf_Attribute attr,
    Dwarf_Locdesc ** llbuf,
    Dwarf_Signed * listlen, Dwarf_Error * error)
{
    Dwarf_Debug dbg;

    /*  Dwarf_Attribute that describes the DW_AT_location in die, if
        present. */
    Dwarf_Attribute loc_attr = attr;

    /*  Dwarf_Block that describes a single location expression. */
    Dwarf_Block_c loc_block;

    /*  A pointer to the current Dwarf_Locdesc read. */
    Dwarf_Locdesc *locdesc = 0;

    Dwarf_Small *info_section_end = 0;
    Dwarf_Half form = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_CU_Context cucontext = 0;
    unsigned address_size = 0;

    int blkres = DW_DLV_ERROR;
    int setup_res = DW_DLV_ERROR;
    int cuvstamp = 0;

    /* ***** BEGIN CODE ***** */
    setup_res = _dwarf_setup_loc(attr, &dbg, &cucontext, &form, error);
    if (setup_res != DW_DLV_OK) {
        return setup_res;
    }
    info_section_end = _dwarf_calculate_info_section_end_ptr(cucontext);
    cuvstamp = cucontext->cc_version_stamp;
    address_size = cucontext->cc_address_size;
    /*  If this is a form_block then it's a location expression. If it's
        DW_FORM_data4 or DW_FORM_data8 it's a loclist offset */
    if (((cuvstamp == DW_CU_VERSION2 ||
        cuvstamp == DW_CU_VERSION3) &&
            (form == DW_FORM_data4 || form == DW_FORM_data8)) ||
        ((cuvstamp == DW_CU_VERSION4) &&
            form == DW_FORM_sec_offset))
        {

        /*  A reference to .debug_loc, with an offset in .debug_loc of a
            loclist. */
        Dwarf_Unsigned loclist_offset = 0;
        int off_res = DW_DLV_ERROR;

        off_res = _dwarf_get_loclist_header_start(dbg,
            attr, &loclist_offset,
            error);
        if (off_res != DW_DLV_OK) {
            return off_res;
        }

        /* With dwarf_loclist, just read a single entry */
        blkres = _dwarf_read_loc_section(dbg, &loc_block,
            &lowpc,
            &highpc,
            loclist_offset,
            address_size,
            error);
        if (blkres != DW_DLV_OK) {
            return (blkres);
        }
    } else {
        if( form == DW_FORM_exprloc) {
            blkres = dwarf_formexprloc(loc_attr,&loc_block.bl_len,
                &loc_block.bl_data,error);
            if(blkres != DW_DLV_OK) {
                return blkres;
            }
            loc_block.bl_from_loclist = 0;
            loc_block.bl_section_offset  =
                (char *)loc_block.bl_data -
                (char *)dbg->de_debug_info.dss_data;
        } else {
            Dwarf_Block *tblock = 0;

            /* If DWARF5 this will surely fail, get an error. */
            blkres = dwarf_formblock(loc_attr, &tblock, error);
            if (blkres != DW_DLV_OK) {
                return (blkres);
            }
            loc_block.bl_len = tblock->bl_len;
            loc_block.bl_data = tblock->bl_data;
            loc_block.bl_from_loclist = tblock->bl_from_loclist;
            loc_block.bl_section_offset = tblock->bl_section_offset;
            /*  We copied tblock contents to the stack var, so can dealloc
                tblock now.  Avoids leaks. */
            dwarf_dealloc(dbg, tblock, DW_DLA_BLOCK);
        }
        lowpc = 0;              /* HACK */
        highpc = (Dwarf_Unsigned) (-1LL);       /* HACK */
    }

    /*  An empty location description (block length 0) means the code
        generator emitted no variable, the variable was not generated,
        it was unused or perhaps never tested after being set. Dwarf2,
        section 2.4.1 In other words, it is not an error, and we don't
        test for block length 0 specially here.
        See *dwarf_loclist_n() which handles the general case, this case
        handles only a single location expression.  */
    blkres = _dwarf_get_locdesc(dbg, &loc_block,
        address_size,
        cucontext->cc_length_size,
        cucontext->cc_version_stamp,
        lowpc, highpc,
        info_section_end,
        &locdesc,
        error);
    if (blkres != DW_DLV_OK) {
        /* low level error already set: let it be passed back */
        return blkres;
    }

    *llbuf = locdesc;
    *listlen = 1;
    return (DW_DLV_OK);
}



/*  Handles only a location expression.
    It returns the location expression as a loclist with
    a single entry.

    Usable to access dwarf expressions from any source, but
    specifically from
        DW_CFA_def_cfa_expression
        DW_CFA_expression
        DW_CFA_val_expression

    expression_in must point to a valid dwarf expression
    set of bytes of length expression_length. Not
    a DW_FORM_block*, just the expression bytes.

    If the address_size != de_pointer_size this will not work
    right.
    See dwarf_loclist_from_expr_b() for a better interface.
*/
int
dwarf_loclist_from_expr(Dwarf_Debug dbg,
    Dwarf_Ptr expression_in,
    Dwarf_Unsigned expression_length,
    Dwarf_Locdesc ** llbuf,
    Dwarf_Signed * listlen, Dwarf_Error * error)
{
    int res = 0;
    Dwarf_Half addr_size =  dbg->de_pointer_size;
    res = dwarf_loclist_from_expr_a(dbg,expression_in,
        expression_length, addr_size,llbuf,listlen,error);
    return res;
}

/*  New April 27 2009. Adding addr_size argument for the rare
    cases where an object has CUs with a different address_size.

    As of 2012 we have yet another version, dwarf_loclist_from_expr_b()
    with the version_stamp and offset size (length size) passed in.
*/
int
dwarf_loclist_from_expr_a(Dwarf_Debug dbg,
    Dwarf_Ptr expression_in,
    Dwarf_Unsigned expression_length,
    Dwarf_Half addr_size,
    Dwarf_Locdesc ** llbuf,
    Dwarf_Signed * listlen,
    Dwarf_Error * error)
{
    int res;
    Dwarf_Debug_InfoTypes info_reading = &dbg->de_info_reading;
    Dwarf_CU_Context current_cu_context =
        info_reading->de_cu_context;
    Dwarf_Small version_stamp =  DW_CU_VERSION2;
    Dwarf_Half offset_size = dbg->de_length_size;

    if (current_cu_context) {
        /*  This is ugly. It is not necessarily right. Due to
            oddity in DW_OP_GNU_implicit_pointer, see its
            implementation above.
            For correctness, use dwarf_loclist_from_expr_b()
            instead of dwarf_loclist_from_expr_a(). */
        version_stamp = current_cu_context->cc_version_stamp;
        offset_size = current_cu_context->cc_length_size;
        if (version_stamp < 2) {
            /* This is probably totally silly.  */
            version_stamp = DW_CU_VERSION2;
        }
    }
    res = dwarf_loclist_from_expr_b(dbg,
        expression_in,
        expression_length,
        addr_size,
        offset_size,
        version_stamp, /* CU context DWARF version */
        llbuf,
        listlen,
        error);
    return res;
}
/*  New November 13 2012. Adding
    DWARF version number argument.
*/
int
dwarf_loclist_from_expr_b(Dwarf_Debug dbg,
    Dwarf_Ptr expression_in,
    Dwarf_Unsigned expression_length,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Small dwarf_version,
    Dwarf_Locdesc ** llbuf,
    Dwarf_Signed * listlen,
    Dwarf_Error * error)
{
    /* Dwarf_Block that describes a single location expression. */
    Dwarf_Block_c loc_block;

    /* A pointer to the current Dwarf_Locdesc read. */
    Dwarf_Locdesc *locdesc = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = (Dwarf_Unsigned) (-1LL);
    Dwarf_Small version_stamp = dwarf_version;
    int res = 0;
    /* We do not know what the end is in this interface. */
    Dwarf_Small *section_start = 0;
    Dwarf_Unsigned section_size = 0;
    Dwarf_Small *section_end = 0;
    const char *section_name = 0;

    res = _dwarf_what_section_are_we(dbg,
        expression_in,&section_name,&section_start,
        &section_size,&section_end,error);
    if (res != DW_DLV_OK) {
        _dwarf_error(dbg, error,DW_DLE_POINTER_SECTION_UNKNOWN);
        return DW_DLV_ERROR;
    }

    memset(&loc_block,0,sizeof(loc_block));
    loc_block.bl_len = expression_length;
    loc_block.bl_data = expression_in;
    loc_block.bl_from_loclist = 0; /* Not from loclist. */
    loc_block.bl_section_offset = 0; /* Fake. Not meaningful. */

    /* An empty location description (block length 0) means the code
    generator emitted no variable, the variable was not generated,
    it was unused or perhaps never tested after being set. Dwarf2,
    section 2.4.1 In other words, it is not an error, and we don't
    test for block length 0 specially here.  */
    /* We need the DWARF version to get a locdesc! */
    res = _dwarf_get_locdesc(dbg, &loc_block,
        addr_size,
        offset_size,
        version_stamp,
        lowpc, highpc,
        section_end,
        &locdesc,
        error);
    if (res != DW_DLV_OK) {
        /* low level error already set: let it be passed back */
        return res;
    }

    *llbuf = locdesc;
    *listlen = 1;
    return DW_DLV_OK;
}

/* Usable to read a single loclist or to read a block of them
   or to read an entire section's loclists.

   It's BROKEN because it's not safe to read a loclist entry
   when we do not know the address size (in any object where
   address size can vary by compilation unit).

   It also does not recognize split dwarf or DWARF4
   or DWARF5 adequately.
*/

/*ARGSUSED*/ int
dwarf_get_loclist_entry(Dwarf_Debug dbg,
    Dwarf_Unsigned offset,
    Dwarf_Addr * hipc_offset,
    Dwarf_Addr * lopc_offset,
    Dwarf_Ptr * data,
    Dwarf_Unsigned * entry_len,
    Dwarf_Unsigned * next_entry,
    Dwarf_Error * error)
{
    Dwarf_Block_c b;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Half address_size = 0;
    int res = DW_DLV_ERROR;

    if (!dbg->de_debug_loc.dss_data) {
        int secload = _dwarf_load_section(dbg, &dbg->de_debug_loc,error);
        if (secload != DW_DLV_OK) {
            return secload;
        }
    }

    /* FIXME: address_size is not necessarily the same in every frame. */
    address_size = dbg->de_pointer_size;
    res = _dwarf_read_loc_section(dbg,
        &b, &lowpc, &highpc, offset,
        address_size,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    *hipc_offset = highpc;
    *lopc_offset = lowpc;
    *entry_len = b.bl_len;
    *data = b.bl_data;
    *next_entry = b.bl_len + b.bl_section_offset;
    return DW_DLV_OK;
}

/* Bring in the code for the October 2015 interfaces. */
#include "dwarf_loc2.h"

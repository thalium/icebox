/* $Id: dbgmoddwarf.cpp $ */
/** @file
 * IPRT - Debug Info Reader For DWARF.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP   RTLOGGROUP_DBG_DWARF
#include <iprt/dbg.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/list.h>
#include <iprt/log.h>
#include <iprt/mem.h>
#define RTDBGMODDWARF_WITH_MEM_CACHE
#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
# include <iprt/memcache.h>
#endif
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/strcache.h>
#include "internal/dbgmod.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @name Standard DWARF Line Number Opcodes
 * @{ */
#define DW_LNS_extended                    UINT8_C(0x00)
#define DW_LNS_copy                        UINT8_C(0x01)
#define DW_LNS_advance_pc                  UINT8_C(0x02)
#define DW_LNS_advance_line                UINT8_C(0x03)
#define DW_LNS_set_file                    UINT8_C(0x04)
#define DW_LNS_set_column                  UINT8_C(0x05)
#define DW_LNS_negate_stmt                 UINT8_C(0x06)
#define DW_LNS_set_basic_block             UINT8_C(0x07)
#define DW_LNS_const_add_pc                UINT8_C(0x08)
#define DW_LNS_fixed_advance_pc            UINT8_C(0x09)
#define DW_LNS_set_prologue_end            UINT8_C(0x0a)
#define DW_LNS_set_epilogue_begin          UINT8_C(0x0b)
#define DW_LNS_set_isa                     UINT8_C(0x0c)
#define DW_LNS_what_question_mark          UINT8_C(0x0d)
/** @} */


/** @name Extended DWARF Line Number Opcodes
 * @{ */
#define DW_LNE_end_sequence                 UINT8_C(1)
#define DW_LNE_set_address                  UINT8_C(2)
#define DW_LNE_define_file                  UINT8_C(3)
#define DW_LNE_set_descriminator            UINT8_C(4)
/** @} */

/** @name DIE Tags.
 * @{ */
#define DW_TAG_array_type                   UINT16_C(0x0001)
#define DW_TAG_class_type                   UINT16_C(0x0002)
#define DW_TAG_entry_point                  UINT16_C(0x0003)
#define DW_TAG_enumeration_type             UINT16_C(0x0004)
#define DW_TAG_formal_parameter             UINT16_C(0x0005)
#define DW_TAG_imported_declaration         UINT16_C(0x0008)
#define DW_TAG_label                        UINT16_C(0x000a)
#define DW_TAG_lexical_block                UINT16_C(0x000b)
#define DW_TAG_member                       UINT16_C(0x000d)
#define DW_TAG_pointer_type                 UINT16_C(0x000f)
#define DW_TAG_reference_type               UINT16_C(0x0010)
#define DW_TAG_compile_unit                 UINT16_C(0x0011)
#define DW_TAG_string_type                  UINT16_C(0x0012)
#define DW_TAG_structure_type               UINT16_C(0x0013)
#define DW_TAG_subroutine_type              UINT16_C(0x0015)
#define DW_TAG_typedef                      UINT16_C(0x0016)
#define DW_TAG_union_type                   UINT16_C(0x0017)
#define DW_TAG_unspecified_parameters       UINT16_C(0x0018)
#define DW_TAG_variant                      UINT16_C(0x0019)
#define DW_TAG_common_block                 UINT16_C(0x001a)
#define DW_TAG_common_inclusion             UINT16_C(0x001b)
#define DW_TAG_inheritance                  UINT16_C(0x001c)
#define DW_TAG_inlined_subroutine           UINT16_C(0x001d)
#define DW_TAG_module                       UINT16_C(0x001e)
#define DW_TAG_ptr_to_member_type           UINT16_C(0x001f)
#define DW_TAG_set_type                     UINT16_C(0x0020)
#define DW_TAG_subrange_type                UINT16_C(0x0021)
#define DW_TAG_with_stmt                    UINT16_C(0x0022)
#define DW_TAG_access_declaration           UINT16_C(0x0023)
#define DW_TAG_base_type                    UINT16_C(0x0024)
#define DW_TAG_catch_block                  UINT16_C(0x0025)
#define DW_TAG_const_type                   UINT16_C(0x0026)
#define DW_TAG_constant                     UINT16_C(0x0027)
#define DW_TAG_enumerator                   UINT16_C(0x0028)
#define DW_TAG_file_type                    UINT16_C(0x0029)
#define DW_TAG_friend                       UINT16_C(0x002a)
#define DW_TAG_namelist                     UINT16_C(0x002b)
#define DW_TAG_namelist_item                UINT16_C(0x002c)
#define DW_TAG_packed_type                  UINT16_C(0x002d)
#define DW_TAG_subprogram                   UINT16_C(0x002e)
#define DW_TAG_template_type_parameter      UINT16_C(0x002f)
#define DW_TAG_template_value_parameter     UINT16_C(0x0030)
#define DW_TAG_thrown_type                  UINT16_C(0x0031)
#define DW_TAG_try_block                    UINT16_C(0x0032)
#define DW_TAG_variant_part                 UINT16_C(0x0033)
#define DW_TAG_variable                     UINT16_C(0x0034)
#define DW_TAG_volatile_type                UINT16_C(0x0035)
#define DW_TAG_dwarf_procedure              UINT16_C(0x0036)
#define DW_TAG_restrict_type                UINT16_C(0x0037)
#define DW_TAG_interface_type               UINT16_C(0x0038)
#define DW_TAG_namespace                    UINT16_C(0x0039)
#define DW_TAG_imported_module              UINT16_C(0x003a)
#define DW_TAG_unspecified_type             UINT16_C(0x003b)
#define DW_TAG_partial_unit                 UINT16_C(0x003c)
#define DW_TAG_imported_unit                UINT16_C(0x003d)
#define DW_TAG_condition                    UINT16_C(0x003f)
#define DW_TAG_shared_type                  UINT16_C(0x0040)
#define DW_TAG_type_unit                    UINT16_C(0x0041)
#define DW_TAG_rvalue_reference_type        UINT16_C(0x0042)
#define DW_TAG_template_alias               UINT16_C(0x0043)
#define DW_TAG_lo_user                      UINT16_C(0x4080)
#define DW_TAG_GNU_call_site                UINT16_C(0x4109)
#define DW_TAG_GNU_call_site_parameter      UINT16_C(0x410a)
#define DW_TAG_hi_user                      UINT16_C(0xffff)
/** @} */


/** @name DIE Attributes.
 * @{ */
#define DW_AT_sibling                       UINT16_C(0x0001)
#define DW_AT_location                      UINT16_C(0x0002)
#define DW_AT_name                          UINT16_C(0x0003)
#define DW_AT_ordering                      UINT16_C(0x0009)
#define DW_AT_byte_size                     UINT16_C(0x000b)
#define DW_AT_bit_offset                    UINT16_C(0x000c)
#define DW_AT_bit_size                      UINT16_C(0x000d)
#define DW_AT_stmt_list                     UINT16_C(0x0010)
#define DW_AT_low_pc                        UINT16_C(0x0011)
#define DW_AT_high_pc                       UINT16_C(0x0012)
#define DW_AT_language                      UINT16_C(0x0013)
#define DW_AT_discr                         UINT16_C(0x0015)
#define DW_AT_discr_value                   UINT16_C(0x0016)
#define DW_AT_visibility                    UINT16_C(0x0017)
#define DW_AT_import                        UINT16_C(0x0018)
#define DW_AT_string_length                 UINT16_C(0x0019)
#define DW_AT_common_reference              UINT16_C(0x001a)
#define DW_AT_comp_dir                      UINT16_C(0x001b)
#define DW_AT_const_value                   UINT16_C(0x001c)
#define DW_AT_containing_type               UINT16_C(0x001d)
#define DW_AT_default_value                 UINT16_C(0x001e)
#define DW_AT_inline                        UINT16_C(0x0020)
#define DW_AT_is_optional                   UINT16_C(0x0021)
#define DW_AT_lower_bound                   UINT16_C(0x0022)
#define DW_AT_producer                      UINT16_C(0x0025)
#define DW_AT_prototyped                    UINT16_C(0x0027)
#define DW_AT_return_addr                   UINT16_C(0x002a)
#define DW_AT_start_scope                   UINT16_C(0x002c)
#define DW_AT_bit_stride                    UINT16_C(0x002e)
#define DW_AT_upper_bound                   UINT16_C(0x002f)
#define DW_AT_abstract_origin               UINT16_C(0x0031)
#define DW_AT_accessibility                 UINT16_C(0x0032)
#define DW_AT_address_class                 UINT16_C(0x0033)
#define DW_AT_artificial                    UINT16_C(0x0034)
#define DW_AT_base_types                    UINT16_C(0x0035)
#define DW_AT_calling_convention            UINT16_C(0x0036)
#define DW_AT_count                         UINT16_C(0x0037)
#define DW_AT_data_member_location          UINT16_C(0x0038)
#define DW_AT_decl_column                   UINT16_C(0x0039)
#define DW_AT_decl_file                     UINT16_C(0x003a)
#define DW_AT_decl_line                     UINT16_C(0x003b)
#define DW_AT_declaration                   UINT16_C(0x003c)
#define DW_AT_discr_list                    UINT16_C(0x003d)
#define DW_AT_encoding                      UINT16_C(0x003e)
#define DW_AT_external                      UINT16_C(0x003f)
#define DW_AT_frame_base                    UINT16_C(0x0040)
#define DW_AT_friend                        UINT16_C(0x0041)
#define DW_AT_identifier_case               UINT16_C(0x0042)
#define DW_AT_macro_info                    UINT16_C(0x0043)
#define DW_AT_namelist_item                 UINT16_C(0x0044)
#define DW_AT_priority                      UINT16_C(0x0045)
#define DW_AT_segment                       UINT16_C(0x0046)
#define DW_AT_specification                 UINT16_C(0x0047)
#define DW_AT_static_link                   UINT16_C(0x0048)
#define DW_AT_type                          UINT16_C(0x0049)
#define DW_AT_use_location                  UINT16_C(0x004a)
#define DW_AT_variable_parameter            UINT16_C(0x004b)
#define DW_AT_virtuality                    UINT16_C(0x004c)
#define DW_AT_vtable_elem_location          UINT16_C(0x004d)
#define DW_AT_allocated                     UINT16_C(0x004e)
#define DW_AT_associated                    UINT16_C(0x004f)
#define DW_AT_data_location                 UINT16_C(0x0050)
#define DW_AT_byte_stride                   UINT16_C(0x0051)
#define DW_AT_entry_pc                      UINT16_C(0x0052)
#define DW_AT_use_UTF8                      UINT16_C(0x0053)
#define DW_AT_extension                     UINT16_C(0x0054)
#define DW_AT_ranges                        UINT16_C(0x0055)
#define DW_AT_trampoline                    UINT16_C(0x0056)
#define DW_AT_call_column                   UINT16_C(0x0057)
#define DW_AT_call_file                     UINT16_C(0x0058)
#define DW_AT_call_line                     UINT16_C(0x0059)
#define DW_AT_description                   UINT16_C(0x005a)
#define DW_AT_binary_scale                  UINT16_C(0x005b)
#define DW_AT_decimal_scale                 UINT16_C(0x005c)
#define DW_AT_small                         UINT16_C(0x005d)
#define DW_AT_decimal_sign                  UINT16_C(0x005e)
#define DW_AT_digit_count                   UINT16_C(0x005f)
#define DW_AT_picture_string                UINT16_C(0x0060)
#define DW_AT_mutable                       UINT16_C(0x0061)
#define DW_AT_threads_scaled                UINT16_C(0x0062)
#define DW_AT_explicit                      UINT16_C(0x0063)
#define DW_AT_object_pointer                UINT16_C(0x0064)
#define DW_AT_endianity                     UINT16_C(0x0065)
#define DW_AT_elemental                     UINT16_C(0x0066)
#define DW_AT_pure                          UINT16_C(0x0067)
#define DW_AT_recursive                     UINT16_C(0x0068)
#define DW_AT_signature                     UINT16_C(0x0069)
#define DW_AT_main_subprogram               UINT16_C(0x006a)
#define DW_AT_data_bit_offset               UINT16_C(0x006b)
#define DW_AT_const_expr                    UINT16_C(0x006c)
#define DW_AT_enum_class                    UINT16_C(0x006d)
#define DW_AT_linkage_name                  UINT16_C(0x006e)
#define DW_AT_lo_user                       UINT16_C(0x2000)
/** Used by GCC and others, same as DW_AT_linkage_name. See http://wiki.dwarfstd.org/index.php?title=DW_AT_linkage_name*/
#define DW_AT_MIPS_linkage_name             UINT16_C(0x2007)
#define DW_AT_hi_user                       UINT16_C(0x3fff)
/** @} */

/** @name DIE Forms.
 * @{ */
#define DW_FORM_addr                        UINT16_C(0x01)
/* What was 0x02? */
#define DW_FORM_block2                      UINT16_C(0x03)
#define DW_FORM_block4                      UINT16_C(0x04)
#define DW_FORM_data2                       UINT16_C(0x05)
#define DW_FORM_data4                       UINT16_C(0x06)
#define DW_FORM_data8                       UINT16_C(0x07)
#define DW_FORM_string                      UINT16_C(0x08)
#define DW_FORM_block                       UINT16_C(0x09)
#define DW_FORM_block1                      UINT16_C(0x0a)
#define DW_FORM_data1                       UINT16_C(0x0b)
#define DW_FORM_flag                        UINT16_C(0x0c)
#define DW_FORM_sdata                       UINT16_C(0x0d)
#define DW_FORM_strp                        UINT16_C(0x0e)
#define DW_FORM_udata                       UINT16_C(0x0f)
#define DW_FORM_ref_addr                    UINT16_C(0x10)
#define DW_FORM_ref1                        UINT16_C(0x11)
#define DW_FORM_ref2                        UINT16_C(0x12)
#define DW_FORM_ref4                        UINT16_C(0x13)
#define DW_FORM_ref8                        UINT16_C(0x14)
#define DW_FORM_ref_udata                   UINT16_C(0x15)
#define DW_FORM_indirect                    UINT16_C(0x16)
#define DW_FORM_sec_offset                  UINT16_C(0x17)
#define DW_FORM_exprloc                     UINT16_C(0x18)
#define DW_FORM_flag_present                UINT16_C(0x19)
#define DW_FORM_ref_sig8                    UINT16_C(0x20)
/** @} */

/** @name Address classes.
 * @{ */
#define DW_ADDR_none            UINT8_C(0)
#define DW_ADDR_i386_near16     UINT8_C(1)
#define DW_ADDR_i386_far16      UINT8_C(2)
#define DW_ADDR_i386_huge16     UINT8_C(3)
#define DW_ADDR_i386_near32     UINT8_C(4)
#define DW_ADDR_i386_far32      UINT8_C(5)
/** @} */


/** @name Location Expression Opcodes
 * @{ */
#define DW_OP_addr              UINT8_C(0x03) /**< 1 operand, a constant address (size target specific). */
#define DW_OP_deref             UINT8_C(0x06) /**< 0 operands. */
#define DW_OP_const1u           UINT8_C(0x08) /**< 1 operand, a 1-byte constant. */
#define DW_OP_const1s           UINT8_C(0x09) /**< 1 operand, a 1-byte constant. */
#define DW_OP_const2u           UINT8_C(0x0a) /**< 1 operand, a 2-byte constant. */
#define DW_OP_const2s           UINT8_C(0x0b) /**< 1 operand, a 2-byte constant. */
#define DW_OP_const4u           UINT8_C(0x0c) /**< 1 operand, a 4-byte constant. */
#define DW_OP_const4s           UINT8_C(0x0d) /**< 1 operand, a 4-byte constant. */
#define DW_OP_const8u           UINT8_C(0x0e) /**< 1 operand, a 8-byte constant. */
#define DW_OP_const8s           UINT8_C(0x0f) /**< 1 operand, a 8-byte constant. */
#define DW_OP_constu            UINT8_C(0x10) /**< 1 operand, a ULEB128 constant. */
#define DW_OP_consts            UINT8_C(0x11) /**< 1 operand, a SLEB128 constant. */
#define DW_OP_dup               UINT8_C(0x12) /**< 0 operands. */
#define DW_OP_drop              UINT8_C(0x13) /**< 0 operands. */
#define DW_OP_over              UINT8_C(0x14) /**< 0 operands. */
#define DW_OP_pick              UINT8_C(0x15) /**< 1 operands, a 1-byte stack index. */
#define DW_OP_swap              UINT8_C(0x16) /**< 0 operands. */
#define DW_OP_rot               UINT8_C(0x17) /**< 0 operands. */
#define DW_OP_xderef            UINT8_C(0x18) /**< 0 operands. */
#define DW_OP_abs               UINT8_C(0x19) /**< 0 operands. */
#define DW_OP_and               UINT8_C(0x1a) /**< 0 operands. */
#define DW_OP_div               UINT8_C(0x1b) /**< 0 operands. */
#define DW_OP_minus             UINT8_C(0x1c) /**< 0 operands. */
#define DW_OP_mod               UINT8_C(0x1d) /**< 0 operands. */
#define DW_OP_mul               UINT8_C(0x1e) /**< 0 operands. */
#define DW_OP_neg               UINT8_C(0x1f) /**< 0 operands. */
#define DW_OP_not               UINT8_C(0x20) /**< 0 operands. */
#define DW_OP_or                UINT8_C(0x21) /**< 0 operands. */
#define DW_OP_plus              UINT8_C(0x22) /**< 0 operands. */
#define DW_OP_plus_uconst       UINT8_C(0x23) /**< 1 operands, a ULEB128 addend. */
#define DW_OP_shl               UINT8_C(0x24) /**< 0 operands. */
#define DW_OP_shr               UINT8_C(0x25) /**< 0 operands. */
#define DW_OP_shra              UINT8_C(0x26) /**< 0 operands. */
#define DW_OP_xor               UINT8_C(0x27) /**< 0 operands. */
#define DW_OP_skip              UINT8_C(0x2f) /**< 1 signed 2-byte constant. */
#define DW_OP_bra               UINT8_C(0x28) /**< 1 signed 2-byte constant. */
#define DW_OP_eq                UINT8_C(0x29) /**< 0 operands. */
#define DW_OP_ge                UINT8_C(0x2a) /**< 0 operands. */
#define DW_OP_gt                UINT8_C(0x2b) /**< 0 operands. */
#define DW_OP_le                UINT8_C(0x2c) /**< 0 operands. */
#define DW_OP_lt                UINT8_C(0x2d) /**< 0 operands. */
#define DW_OP_ne                UINT8_C(0x2e) /**< 0 operands. */
#define DW_OP_lit0              UINT8_C(0x30) /**< 0 operands - literals 0..31 */
#define DW_OP_lit31             UINT8_C(0x4f) /**< last litteral. */
#define DW_OP_reg0              UINT8_C(0x50) /**< 0 operands - reg 0..31. */
#define DW_OP_reg31             UINT8_C(0x6f) /**< last register. */
#define DW_OP_breg0             UINT8_C(0x70) /**< 1 operand, a SLEB128 offset. */
#define DW_OP_breg31            UINT8_C(0x8f) /**< last branch register. */
#define DW_OP_regx              UINT8_C(0x90) /**< 1 operand, a ULEB128 register. */
#define DW_OP_fbreg             UINT8_C(0x91) /**< 1 operand, a SLEB128 offset. */
#define DW_OP_bregx             UINT8_C(0x92) /**< 2 operands, a ULEB128 register followed by a SLEB128 offset. */
#define DW_OP_piece             UINT8_C(0x93) /**< 1 operand, a ULEB128 size of piece addressed. */
#define DW_OP_deref_size        UINT8_C(0x94) /**< 1 operand, a 1-byte size of data retrieved. */
#define DW_OP_xderef_size       UINT8_C(0x95) /**< 1 operand, a 1-byte size of data retrieved. */
#define DW_OP_nop               UINT8_C(0x96) /**< 0 operands. */
#define DW_OP_lo_user           UINT8_C(0xe0) /**< First user opcode */
#define DW_OP_hi_user           UINT8_C(0xff) /**< Last user opcode. */
/** @} */


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Pointer to a DWARF section reader. */
typedef struct RTDWARFCURSOR *PRTDWARFCURSOR;
/** Pointer to an attribute descriptor. */
typedef struct RTDWARFATTRDESC const *PCRTDWARFATTRDESC;
/** Pointer to a DIE. */
typedef struct RTDWARFDIE *PRTDWARFDIE;
/** Pointer to a const DIE. */
typedef struct RTDWARFDIE const *PCRTDWARFDIE;

/**
 * DWARF sections.
 */
typedef enum krtDbgModDwarfSect
{
    krtDbgModDwarfSect_abbrev = 0,
    krtDbgModDwarfSect_aranges,
    krtDbgModDwarfSect_frame,
    krtDbgModDwarfSect_info,
    krtDbgModDwarfSect_inlined,
    krtDbgModDwarfSect_line,
    krtDbgModDwarfSect_loc,
    krtDbgModDwarfSect_macinfo,
    krtDbgModDwarfSect_pubnames,
    krtDbgModDwarfSect_pubtypes,
    krtDbgModDwarfSect_ranges,
    krtDbgModDwarfSect_str,
    krtDbgModDwarfSect_types,
    /** End of valid parts (exclusive). */
    krtDbgModDwarfSect_End
} krtDbgModDwarfSect;

/**
 * Abbreviation cache entry.
 */
typedef struct RTDWARFABBREV
{
    /** Whether there are children or not. */
    bool                fChildren;
    /** The tag. */
    uint16_t            uTag;
    /** Offset into the abbrev section of the specification pairs. */
    uint32_t            offSpec;
    /** The abbreviation table offset this is entry is valid for.
     * UINT32_MAX if not valid. */
    uint32_t            offAbbrev;
} RTDWARFABBREV;
/** Pointer to an abbreviation cache entry. */
typedef RTDWARFABBREV *PRTDWARFABBREV;
/** Pointer to a const abbreviation cache entry. */
typedef RTDWARFABBREV const *PCRTDWARFABBREV;

/**
 * Structure for gathering segment info.
 */
typedef struct RTDBGDWARFSEG
{
    /** The highest offset in the segment. */
    uint64_t            offHighest;
    /** Calculated base address. */
    uint64_t            uBaseAddr;
    /** Estimated The segment size. */
    uint64_t            cbSegment;
    /** Segment number (RTLDRSEG::Sel16bit). */
    RTSEL               uSegment;
} RTDBGDWARFSEG;
/** Pointer to segment info. */
typedef RTDBGDWARFSEG *PRTDBGDWARFSEG;


/**
 * The instance data of the DWARF reader.
 */
typedef struct RTDBGMODDWARF
{
    /** The debug container containing doing the real work. */
    RTDBGMOD                hCnt;
    /** The image module (no reference). */
    PRTDBGMODINT            pImgMod;
    /** The debug info module (no reference). */
    PRTDBGMODINT            pDbgInfoMod;
    /** Nested image module (with reference ofc). */
    PRTDBGMODINT            pNestedMod;

    /** DWARF debug info sections. */
    struct
    {
        /** The file offset of the part. */
        RTFOFF              offFile;
        /** The size of the part. */
        size_t              cb;
        /** The memory mapping of the part. */
        void const         *pv;
        /** Set if present. */
        bool                fPresent;
        /** The debug info ordinal number in the image file. */
        uint32_t            iDbgInfo;
    } aSections[krtDbgModDwarfSect_End];

    /** The offset into the abbreviation section of the current cache. */
    uint32_t                offCachedAbbrev;
    /** The number of cached abbreviations we've allocated space for. */
    uint32_t                cCachedAbbrevsAlloced;
    /** Array of cached abbreviations, indexed by code. */
    PRTDWARFABBREV          paCachedAbbrevs;
    /** Used by rtDwarfAbbrev_Lookup when the result is uncachable. */
    RTDWARFABBREV           LookupAbbrev;

    /** The list of compilation units (RTDWARFDIE). */
    RTLISTANCHOR            CompileUnitList;

    /** Set if we have to use link addresses because the module does not have
     *  fixups (mach_kernel). */
    bool                    fUseLinkAddress;
    /** This is set to -1 if we're doing everything in one pass.
     * Otherwise it's 1 or 2:
     *      - In pass 1, we collect segment info.
     *      - In pass 2, we add debug info to the container.
     * The two pass parsing is necessary for watcom generated symbol files as
     * these contains no information about the code and data segments in the
     * image.  So we have to figure out some approximate stuff based on the
     * segments and offsets we encounter in the debug info. */
    int8_t                  iWatcomPass;
    /** Segment index hint. */
    uint16_t                iSegHint;
    /** The number of segments in paSegs.
     * (During segment copying, this is abused to count useful segments.) */
    uint32_t                cSegs;
    /** Pointer to segments if iWatcomPass isn't -1. */
    PRTDBGDWARFSEG          paSegs;
#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
    /** DIE allocators. */
    struct
    {
        RTMEMCACHE          hMemCache;
        uint32_t            cbMax;
    } aDieAllocators[2];
#endif
} RTDBGMODDWARF;
/** Pointer to instance data of the DWARF reader. */
typedef RTDBGMODDWARF *PRTDBGMODDWARF;

/**
 * DWARF cursor for reading byte data.
 */
typedef struct RTDWARFCURSOR
{
    /** The current position. */
    uint8_t const          *pb;
    /** The number of bytes left to read. */
    size_t                  cbLeft;
    /** The number of bytes left to read in the current unit. */
    size_t                  cbUnitLeft;
    /** The DWARF debug info reader instance. */
    PRTDBGMODDWARF          pDwarfMod;
    /** Set if this is 64-bit DWARF, clear if 32-bit. */
    bool                    f64bitDwarf;
    /** Set if the format endian is native, clear if endian needs to be
     * inverted. */
    bool                    fNativEndian;
    /** The size of a native address. */
    uint8_t                 cbNativeAddr;
    /** The cursor status code.  This is VINF_SUCCESS until some error
     *  occurs. */
    int                     rc;
    /** The start of the area covered by the cursor.
     * Used for repositioning the cursor relative to the start of a section. */
    uint8_t const          *pbStart;
    /** The section. */
    krtDbgModDwarfSect      enmSect;
} RTDWARFCURSOR;


/**
 * DWARF line number program state.
 */
typedef struct RTDWARFLINESTATE
{
    /** Virtual Line Number Machine Registers. */
    struct
    {
        uint64_t        uAddress;
        uint64_t        idxOp;
        uint32_t        iFile;
        uint32_t        uLine;
        uint32_t        uColumn;
        bool            fIsStatement;
        bool            fBasicBlock;
        bool            fEndSequence;
        bool            fPrologueEnd;
        bool            fEpilogueBegin;
        uint32_t        uIsa;
        uint32_t        uDiscriminator;
        RTSEL           uSegment;
    } Regs;
    /** @} */

    /** Header. */
    struct
    {
        uint32_t        uVer;
        uint64_t        offFirstOpcode;
        uint8_t         cbMinInstr;
        uint8_t         cMaxOpsPerInstr;
        uint8_t         u8DefIsStmt;
        int8_t          s8LineBase;
        uint8_t         u8LineRange;
        uint8_t         u8OpcodeBase;
        uint8_t const  *pacStdOperands;
    } Hdr;

    /** @name Include Path Table (0-based)
     * @{ */
    const char    **papszIncPaths;
    uint32_t        cIncPaths;
    /** @} */

    /** @name File Name Table (0-based, dummy zero entry)
     * @{ */
    char          **papszFileNames;
    uint32_t        cFileNames;
    /** @} */

    /** The DWARF debug info reader instance. */
    PRTDBGMODDWARF  pDwarfMod;
} RTDWARFLINESTATE;
/** Pointer to a DWARF line number program state. */
typedef RTDWARFLINESTATE *PRTDWARFLINESTATE;


/**
 * Decodes an attribute and stores it in the specified DIE member field.
 *
 * @returns IPRT status code.
 * @param   pDie            Pointer to the DIE structure.
 * @param   pbMember        Pointer to the first byte in the member.
 * @param   pDesc           The attribute descriptor.
 * @param   uForm           The data form.
 * @param   pCursor         The cursor to read data from.
 */
typedef DECLCALLBACK(int) FNRTDWARFATTRDECODER(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                               uint32_t uForm, PRTDWARFCURSOR pCursor);
/** Pointer to an attribute decoder callback. */
typedef FNRTDWARFATTRDECODER *PFNRTDWARFATTRDECODER;

/**
 * Attribute descriptor.
 */
typedef struct RTDWARFATTRDESC
{
    /** The attribute. */
    uint16_t                uAttr;
    /** The data member offset. */
    uint16_t                off;
    /** The data member size and initialization method. */
    uint8_t                 cbInit;
    uint8_t                 bPadding[3]; /**< Alignment padding. */
    /** The decoder function. */
    PFNRTDWARFATTRDECODER   pfnDecoder;
} RTDWARFATTRDESC;

/** Define a attribute entry. */
#define ATTR_ENTRY(a_uAttr, a_Struct, a_Member, a_Init, a_pfnDecoder) \
    { \
        a_uAttr, \
        (uint16_t)RT_OFFSETOF(a_Struct, a_Member), \
        a_Init | ((uint8_t)RT_SIZEOFMEMB(a_Struct, a_Member) & ATTR_SIZE_MASK), \
        { 0, 0, 0 }, \
        a_pfnDecoder\
    }

/** @name Attribute size and init methods.
 * @{ */
#define ATTR_INIT_ZERO      UINT8_C(0x00)
#define ATTR_INIT_FFFS      UINT8_C(0x80)
#define ATTR_INIT_MASK      UINT8_C(0x80)
#define ATTR_SIZE_MASK      UINT8_C(0x3f)
#define ATTR_GET_SIZE(a_pAttrDesc)   ((a_pAttrDesc)->cbInit & ATTR_SIZE_MASK)
/** @} */


/**
 * DIE descriptor.
 */
typedef struct RTDWARFDIEDESC
{
    /** The size of the DIE. */
    size_t              cbDie;
    /** The number of attributes. */
    size_t              cAttributes;
    /** Pointer to the array of attributes. */
    PCRTDWARFATTRDESC   paAttributes;
} RTDWARFDIEDESC;
typedef struct RTDWARFDIEDESC const *PCRTDWARFDIEDESC;
/** DIE descriptor initializer. */
#define DIE_DESC_INIT(a_Type, a_aAttrs)  { sizeof(a_Type), RT_ELEMENTS(a_aAttrs), &a_aAttrs[0] }


/**
 * DIE core structure, all inherits (starts with) this.
 */
typedef struct RTDWARFDIE
{
    /** Pointer to the parent node. NULL if root unit. */
    struct RTDWARFDIE  *pParent;
    /** Our node in the sibling list. */
    RTLISTNODE          SiblingNode;
    /** List of children. */
    RTLISTNODE          ChildList;
    /** The number of attributes successfully decoded. */
    uint8_t             cDecodedAttrs;
    /** The number of unknown or otherwise unhandled attributes. */
    uint8_t             cUnhandledAttrs;
#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
    /** The allocator index. */
    uint8_t             iAllocator;
#endif
    /** The die tag, indicating which union structure to use. */
    uint16_t            uTag;
    /** Offset of the abbreviation specification (within debug_abbrev). */
    uint32_t            offSpec;
} RTDWARFDIE;


/**
 * DWARF address structure.
 */
typedef struct RTDWARFADDR
{
    /** The address. */
    uint64_t            uAddress;
} RTDWARFADDR;
typedef RTDWARFADDR *PRTDWARFADDR;
typedef RTDWARFADDR const *PCRTDWARFADDR;


/**
 * DWARF address range.
 */
typedef struct RTDWARFADDRRANGE
{
    uint64_t            uLowAddress;
    uint64_t            uHighAddress;
    uint8_t const      *pbRanges; /* ?? */
    uint8_t             cAttrs           : 2;
    uint8_t             fHaveLowAddress  : 1;
    uint8_t             fHaveHighAddress : 1;
    uint8_t             fHaveHighIsAddress : 1;
    uint8_t             fHaveRanges      : 1;
} RTDWARFADDRRANGE;
typedef RTDWARFADDRRANGE *PRTDWARFADDRRANGE;
typedef RTDWARFADDRRANGE const *PCRTDWARFADDRRANGE;

/** What a RTDWARFREF is relative to. */
typedef enum krtDwarfRef
{
    krtDwarfRef_NotSet,
    krtDwarfRef_LineSection,
    krtDwarfRef_LocSection,
    krtDwarfRef_RangesSection,
    krtDwarfRef_InfoSection,
    krtDwarfRef_SameUnit,
    krtDwarfRef_TypeId64
} krtDwarfRef;

/**
 * DWARF reference.
 */
typedef struct RTDWARFREF
{
    /** The offset. */
    uint64_t        off;
    /** What the offset is relative to. */
    krtDwarfRef     enmWrt;
} RTDWARFREF;
typedef RTDWARFREF *PRTDWARFREF;
typedef RTDWARFREF const *PCRTDWARFREF;


/**
 * DWARF Location state.
 */
typedef struct RTDWARFLOCST
{
    /** The input cursor. */
    RTDWARFCURSOR   Cursor;
    /** Points to the current top of the stack. Initial value -1. */
    int32_t         iTop;
    /** The value stack. */
    uint64_t        auStack[64];
} RTDWARFLOCST;
/** Pointer to location state. */
typedef RTDWARFLOCST *PRTDWARFLOCST;



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static FNRTDWARFATTRDECODER rtDwarfDecode_Address;
static FNRTDWARFATTRDECODER rtDwarfDecode_Bool;
static FNRTDWARFATTRDECODER rtDwarfDecode_LowHighPc;
static FNRTDWARFATTRDECODER rtDwarfDecode_Ranges;
static FNRTDWARFATTRDECODER rtDwarfDecode_Reference;
static FNRTDWARFATTRDECODER rtDwarfDecode_SectOff;
static FNRTDWARFATTRDECODER rtDwarfDecode_String;
static FNRTDWARFATTRDECODER rtDwarfDecode_UnsignedInt;
static FNRTDWARFATTRDECODER rtDwarfDecode_SegmentLoc;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** RTDWARFDIE description. */
static const RTDWARFDIEDESC g_CoreDieDesc = { sizeof(RTDWARFDIE), 0, NULL };


/**
 * DW_TAG_compile_unit & DW_TAG_partial_unit.
 */
typedef struct RTDWARFDIECOMPILEUNIT
{
    /** The DIE core structure. */
    RTDWARFDIE          Core;
    /** The unit name. */
    const char         *pszName;
    /** The address range of the code belonging to this unit. */
    RTDWARFADDRRANGE    PcRange;
    /** The language name. */
    uint16_t            uLanguage;
    /** The identifier case. */
    uint8_t             uIdentifierCase;
    /** String are UTF-8 encoded.  If not set, the encoding is
     * unknown. */
    bool                fUseUtf8;
    /** The unit contains main() or equivalent. */
    bool                fMainFunction;
    /** The line numbers for this unit. */
    RTDWARFREF          StmtListRef;
    /** The macro information for this unit. */
    RTDWARFREF          MacroInfoRef;
    /** Reference to the base types. */
    RTDWARFREF          BaseTypesRef;
    /** Working directory for the unit. */
    const char         *pszCurDir;
    /** The name of the compiler or whatever that produced this unit. */
    const char         *pszProducer;

    /** @name From the unit header.
     * @{ */
    /** The offset into debug_info of this unit (for references). */
    uint64_t            offUnit;
    /** The length of this unit. */
    uint64_t            cbUnit;
    /** The offset into debug_abbrev of the abbreviation for this unit. */
    uint64_t            offAbbrev;
    /** The native address size. */
    uint8_t             cbNativeAddr;
    /** The DWARF version. */
    uint8_t             uDwarfVer;
    /** @} */
} RTDWARFDIECOMPILEUNIT;
typedef RTDWARFDIECOMPILEUNIT *PRTDWARFDIECOMPILEUNIT;


/** RTDWARFDIECOMPILEUNIT attributes. */
static const RTDWARFATTRDESC g_aCompileUnitAttrs[] =
{
    ATTR_ENTRY(DW_AT_name,              RTDWARFDIECOMPILEUNIT, pszName,        ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_low_pc,            RTDWARFDIECOMPILEUNIT, PcRange,        ATTR_INIT_ZERO, rtDwarfDecode_LowHighPc),
    ATTR_ENTRY(DW_AT_high_pc,           RTDWARFDIECOMPILEUNIT, PcRange,        ATTR_INIT_ZERO, rtDwarfDecode_LowHighPc),
    ATTR_ENTRY(DW_AT_ranges,            RTDWARFDIECOMPILEUNIT, PcRange,        ATTR_INIT_ZERO, rtDwarfDecode_Ranges),
    ATTR_ENTRY(DW_AT_language,          RTDWARFDIECOMPILEUNIT, uLanguage,      ATTR_INIT_ZERO, rtDwarfDecode_UnsignedInt),
    ATTR_ENTRY(DW_AT_macro_info,        RTDWARFDIECOMPILEUNIT, MacroInfoRef,   ATTR_INIT_ZERO, rtDwarfDecode_SectOff),
    ATTR_ENTRY(DW_AT_stmt_list,         RTDWARFDIECOMPILEUNIT, StmtListRef,    ATTR_INIT_ZERO, rtDwarfDecode_SectOff),
    ATTR_ENTRY(DW_AT_comp_dir,          RTDWARFDIECOMPILEUNIT, pszCurDir,      ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_producer,          RTDWARFDIECOMPILEUNIT, pszProducer,    ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_identifier_case,   RTDWARFDIECOMPILEUNIT, uIdentifierCase,ATTR_INIT_ZERO, rtDwarfDecode_UnsignedInt),
    ATTR_ENTRY(DW_AT_base_types,        RTDWARFDIECOMPILEUNIT, BaseTypesRef,   ATTR_INIT_ZERO, rtDwarfDecode_Reference),
    ATTR_ENTRY(DW_AT_use_UTF8,          RTDWARFDIECOMPILEUNIT, fUseUtf8,       ATTR_INIT_ZERO, rtDwarfDecode_Bool),
    ATTR_ENTRY(DW_AT_main_subprogram,   RTDWARFDIECOMPILEUNIT, fMainFunction,  ATTR_INIT_ZERO, rtDwarfDecode_Bool)
};

/** RTDWARFDIECOMPILEUNIT description. */
static const RTDWARFDIEDESC g_CompileUnitDesc = DIE_DESC_INIT(RTDWARFDIECOMPILEUNIT, g_aCompileUnitAttrs);


/**
 * DW_TAG_subprogram.
 */
typedef struct RTDWARFDIESUBPROGRAM
{
    /** The DIE core structure. */
    RTDWARFDIE          Core;
    /** The name. */
    const char         *pszName;
    /** The linkage name. */
    const char         *pszLinkageName;
    /** The address range of the code belonging to this unit. */
    RTDWARFADDRRANGE    PcRange;
    /** The first instruction in the function. */
    RTDWARFADDR         EntryPc;
    /** Segment number (watcom). */
    RTSEL               uSegment;
    /** Reference to the specification. */
    RTDWARFREF          SpecRef;
} RTDWARFDIESUBPROGRAM;
/** Pointer to a DW_TAG_subprogram DIE. */
typedef RTDWARFDIESUBPROGRAM *PRTDWARFDIESUBPROGRAM;
/** Pointer to a const DW_TAG_subprogram DIE. */
typedef RTDWARFDIESUBPROGRAM const *PCRTDWARFDIESUBPROGRAM;


/** RTDWARFDIESUBPROGRAM attributes. */
static const RTDWARFATTRDESC g_aSubProgramAttrs[] =
{
    ATTR_ENTRY(DW_AT_name,              RTDWARFDIESUBPROGRAM, pszName,        ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_linkage_name,      RTDWARFDIESUBPROGRAM, pszLinkageName, ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_MIPS_linkage_name, RTDWARFDIESUBPROGRAM, pszLinkageName, ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_low_pc,            RTDWARFDIESUBPROGRAM, PcRange,        ATTR_INIT_ZERO, rtDwarfDecode_LowHighPc),
    ATTR_ENTRY(DW_AT_high_pc,           RTDWARFDIESUBPROGRAM, PcRange,        ATTR_INIT_ZERO, rtDwarfDecode_LowHighPc),
    ATTR_ENTRY(DW_AT_ranges,            RTDWARFDIESUBPROGRAM, PcRange,        ATTR_INIT_ZERO, rtDwarfDecode_Ranges),
    ATTR_ENTRY(DW_AT_entry_pc,          RTDWARFDIESUBPROGRAM, EntryPc,        ATTR_INIT_ZERO, rtDwarfDecode_Address),
    ATTR_ENTRY(DW_AT_segment,           RTDWARFDIESUBPROGRAM, uSegment,       ATTR_INIT_ZERO, rtDwarfDecode_SegmentLoc),
    ATTR_ENTRY(DW_AT_specification,     RTDWARFDIESUBPROGRAM, SpecRef,        ATTR_INIT_ZERO, rtDwarfDecode_Reference)
};

/** RTDWARFDIESUBPROGRAM description. */
static const RTDWARFDIEDESC g_SubProgramDesc = DIE_DESC_INIT(RTDWARFDIESUBPROGRAM, g_aSubProgramAttrs);


/** RTDWARFDIESUBPROGRAM attributes for the specification hack. */
static const RTDWARFATTRDESC g_aSubProgramSpecHackAttrs[] =
{
    ATTR_ENTRY(DW_AT_name,              RTDWARFDIESUBPROGRAM, pszName,        ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_linkage_name,      RTDWARFDIESUBPROGRAM, pszLinkageName, ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_MIPS_linkage_name, RTDWARFDIESUBPROGRAM, pszLinkageName, ATTR_INIT_ZERO, rtDwarfDecode_String),
};

/** RTDWARFDIESUBPROGRAM description for the specification hack. */
static const RTDWARFDIEDESC g_SubProgramSpecHackDesc = DIE_DESC_INIT(RTDWARFDIESUBPROGRAM, g_aSubProgramSpecHackAttrs);


/**
 * DW_TAG_label.
 */
typedef struct RTDWARFDIELABEL
{
    /** The DIE core structure. */
    RTDWARFDIE          Core;
    /** The name. */
    const char         *pszName;
    /** The address of the first instruction. */
    RTDWARFADDR         Address;
    /** Segment number (watcom). */
    RTSEL               uSegment;
    /** Externally visible? */
    bool                fExternal;
} RTDWARFDIELABEL;
/** Pointer to a DW_TAG_label DIE. */
typedef RTDWARFDIELABEL *PRTDWARFDIELABEL;
/** Pointer to a const DW_TAG_label DIE. */
typedef RTDWARFDIELABEL const *PCRTDWARFDIELABEL;


/** RTDWARFDIESUBPROGRAM attributes. */
static const RTDWARFATTRDESC g_aLabelAttrs[] =
{
    ATTR_ENTRY(DW_AT_name,              RTDWARFDIELABEL, pszName,               ATTR_INIT_ZERO, rtDwarfDecode_String),
    ATTR_ENTRY(DW_AT_low_pc,            RTDWARFDIELABEL, Address,               ATTR_INIT_ZERO, rtDwarfDecode_Address),
    ATTR_ENTRY(DW_AT_segment,           RTDWARFDIELABEL, uSegment,              ATTR_INIT_ZERO, rtDwarfDecode_SegmentLoc),
    ATTR_ENTRY(DW_AT_external,          RTDWARFDIELABEL, fExternal,             ATTR_INIT_ZERO, rtDwarfDecode_Bool)
};

/** RTDWARFDIESUBPROGRAM description. */
static const RTDWARFDIEDESC g_LabelDesc = DIE_DESC_INIT(RTDWARFDIELABEL, g_aLabelAttrs);


/**
 * Tag names and descriptors.
 */
static const struct RTDWARFTAGDESC
{
    /** The tag value. */
    uint16_t            uTag;
    /** The tag name as string. */
    const char         *pszName;
    /** The DIE descriptor to use. */
    PCRTDWARFDIEDESC    pDesc;
}   g_aTagDescs[] =
{
#define TAGDESC(a_Name, a_pDesc)        { DW_ ## a_Name, #a_Name, a_pDesc }
#define TAGDESC_EMPTY()                 { 0, NULL, NULL }
#define TAGDESC_CORE(a_Name)            TAGDESC(a_Name, &g_CoreDieDesc)
    TAGDESC_EMPTY(),                            /* 0x00 */
    TAGDESC_CORE(TAG_array_type),
    TAGDESC_CORE(TAG_class_type),
    TAGDESC_CORE(TAG_entry_point),
    TAGDESC_CORE(TAG_enumeration_type),         /* 0x04 */
    TAGDESC_CORE(TAG_formal_parameter),
    TAGDESC_EMPTY(),
    TAGDESC_EMPTY(),
    TAGDESC_CORE(TAG_imported_declaration),     /* 0x08 */
    TAGDESC_EMPTY(),
    TAGDESC(TAG_label, &g_LabelDesc),
    TAGDESC_CORE(TAG_lexical_block),
    TAGDESC_EMPTY(),                            /* 0x0c */
    TAGDESC_CORE(TAG_member),
    TAGDESC_EMPTY(),
    TAGDESC_CORE(TAG_pointer_type),
    TAGDESC_CORE(TAG_reference_type),           /* 0x10 */
    TAGDESC_CORE(TAG_compile_unit),
    TAGDESC_CORE(TAG_string_type),
    TAGDESC_CORE(TAG_structure_type),
    TAGDESC_EMPTY(),                            /* 0x14 */
    TAGDESC_CORE(TAG_subroutine_type),
    TAGDESC_CORE(TAG_typedef),
    TAGDESC_CORE(TAG_union_type),
    TAGDESC_CORE(TAG_unspecified_parameters),   /* 0x18 */
    TAGDESC_CORE(TAG_variant),
    TAGDESC_CORE(TAG_common_block),
    TAGDESC_CORE(TAG_common_inclusion),
    TAGDESC_CORE(TAG_inheritance),              /* 0x1c */
    TAGDESC_CORE(TAG_inlined_subroutine),
    TAGDESC_CORE(TAG_module),
    TAGDESC_CORE(TAG_ptr_to_member_type),
    TAGDESC_CORE(TAG_set_type),                 /* 0x20 */
    TAGDESC_CORE(TAG_subrange_type),
    TAGDESC_CORE(TAG_with_stmt),
    TAGDESC_CORE(TAG_access_declaration),
    TAGDESC_CORE(TAG_base_type),                /* 0x24 */
    TAGDESC_CORE(TAG_catch_block),
    TAGDESC_CORE(TAG_const_type),
    TAGDESC_CORE(TAG_constant),
    TAGDESC_CORE(TAG_enumerator),               /* 0x28 */
    TAGDESC_CORE(TAG_file_type),
    TAGDESC_CORE(TAG_friend),
    TAGDESC_CORE(TAG_namelist),
    TAGDESC_CORE(TAG_namelist_item),            /* 0x2c */
    TAGDESC_CORE(TAG_packed_type),
    TAGDESC(TAG_subprogram, &g_SubProgramDesc),
    TAGDESC_CORE(TAG_template_type_parameter),
    TAGDESC_CORE(TAG_template_value_parameter), /* 0x30 */
    TAGDESC_CORE(TAG_thrown_type),
    TAGDESC_CORE(TAG_try_block),
    TAGDESC_CORE(TAG_variant_part),
    TAGDESC_CORE(TAG_variable),                 /* 0x34 */
    TAGDESC_CORE(TAG_volatile_type),
    TAGDESC_CORE(TAG_dwarf_procedure),
    TAGDESC_CORE(TAG_restrict_type),
    TAGDESC_CORE(TAG_interface_type),           /* 0x38 */
    TAGDESC_CORE(TAG_namespace),
    TAGDESC_CORE(TAG_imported_module),
    TAGDESC_CORE(TAG_unspecified_type),
    TAGDESC_CORE(TAG_partial_unit),             /* 0x3c */
    TAGDESC_CORE(TAG_imported_unit),
    TAGDESC_EMPTY(),
    TAGDESC_CORE(TAG_condition),
    TAGDESC_CORE(TAG_shared_type),              /* 0x40 */
    TAGDESC_CORE(TAG_type_unit),
    TAGDESC_CORE(TAG_rvalue_reference_type),
    TAGDESC_CORE(TAG_template_alias)
#undef TAGDESC
#undef TAGDESC_EMPTY
#undef TAGDESC_CORE
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int rtDwarfInfo_ParseDie(PRTDBGMODDWARF pThis, PRTDWARFDIE pDie, PCRTDWARFDIEDESC pDieDesc,
                                PRTDWARFCURSOR pCursor, PCRTDWARFABBREV pAbbrev, bool fInitDie);



#if defined(LOG_ENABLED) || defined(RT_STRICT)

# if 0 /* unused */
/**
 * Turns a tag value into a string for logging purposes.
 *
 * @returns String name.
 * @param   uTag            The tag.
 */
static const char *rtDwarfLog_GetTagName(uint32_t uTag)
{
    if (uTag < RT_ELEMENTS(g_aTagDescs))
    {
        const char *pszTag = g_aTagDescs[uTag].pszName;
        if (pszTag)
            return pszTag;
    }

    static char s_szStatic[32];
    RTStrPrintf(s_szStatic, sizeof(s_szStatic),"DW_TAG_%#x", uTag);
    return s_szStatic;
}
# endif


/**
 * Turns an attributevalue into a string for logging purposes.
 *
 * @returns String name.
 * @param   uAttr               The attribute.
 */
static const char *rtDwarfLog_AttrName(uint32_t uAttr)
{
    switch (uAttr)
    {
        RT_CASE_RET_STR(DW_AT_sibling);
        RT_CASE_RET_STR(DW_AT_location);
        RT_CASE_RET_STR(DW_AT_name);
        RT_CASE_RET_STR(DW_AT_ordering);
        RT_CASE_RET_STR(DW_AT_byte_size);
        RT_CASE_RET_STR(DW_AT_bit_offset);
        RT_CASE_RET_STR(DW_AT_bit_size);
        RT_CASE_RET_STR(DW_AT_stmt_list);
        RT_CASE_RET_STR(DW_AT_low_pc);
        RT_CASE_RET_STR(DW_AT_high_pc);
        RT_CASE_RET_STR(DW_AT_language);
        RT_CASE_RET_STR(DW_AT_discr);
        RT_CASE_RET_STR(DW_AT_discr_value);
        RT_CASE_RET_STR(DW_AT_visibility);
        RT_CASE_RET_STR(DW_AT_import);
        RT_CASE_RET_STR(DW_AT_string_length);
        RT_CASE_RET_STR(DW_AT_common_reference);
        RT_CASE_RET_STR(DW_AT_comp_dir);
        RT_CASE_RET_STR(DW_AT_const_value);
        RT_CASE_RET_STR(DW_AT_containing_type);
        RT_CASE_RET_STR(DW_AT_default_value);
        RT_CASE_RET_STR(DW_AT_inline);
        RT_CASE_RET_STR(DW_AT_is_optional);
        RT_CASE_RET_STR(DW_AT_lower_bound);
        RT_CASE_RET_STR(DW_AT_producer);
        RT_CASE_RET_STR(DW_AT_prototyped);
        RT_CASE_RET_STR(DW_AT_return_addr);
        RT_CASE_RET_STR(DW_AT_start_scope);
        RT_CASE_RET_STR(DW_AT_bit_stride);
        RT_CASE_RET_STR(DW_AT_upper_bound);
        RT_CASE_RET_STR(DW_AT_abstract_origin);
        RT_CASE_RET_STR(DW_AT_accessibility);
        RT_CASE_RET_STR(DW_AT_address_class);
        RT_CASE_RET_STR(DW_AT_artificial);
        RT_CASE_RET_STR(DW_AT_base_types);
        RT_CASE_RET_STR(DW_AT_calling_convention);
        RT_CASE_RET_STR(DW_AT_count);
        RT_CASE_RET_STR(DW_AT_data_member_location);
        RT_CASE_RET_STR(DW_AT_decl_column);
        RT_CASE_RET_STR(DW_AT_decl_file);
        RT_CASE_RET_STR(DW_AT_decl_line);
        RT_CASE_RET_STR(DW_AT_declaration);
        RT_CASE_RET_STR(DW_AT_discr_list);
        RT_CASE_RET_STR(DW_AT_encoding);
        RT_CASE_RET_STR(DW_AT_external);
        RT_CASE_RET_STR(DW_AT_frame_base);
        RT_CASE_RET_STR(DW_AT_friend);
        RT_CASE_RET_STR(DW_AT_identifier_case);
        RT_CASE_RET_STR(DW_AT_macro_info);
        RT_CASE_RET_STR(DW_AT_namelist_item);
        RT_CASE_RET_STR(DW_AT_priority);
        RT_CASE_RET_STR(DW_AT_segment);
        RT_CASE_RET_STR(DW_AT_specification);
        RT_CASE_RET_STR(DW_AT_static_link);
        RT_CASE_RET_STR(DW_AT_type);
        RT_CASE_RET_STR(DW_AT_use_location);
        RT_CASE_RET_STR(DW_AT_variable_parameter);
        RT_CASE_RET_STR(DW_AT_virtuality);
        RT_CASE_RET_STR(DW_AT_vtable_elem_location);
        RT_CASE_RET_STR(DW_AT_allocated);
        RT_CASE_RET_STR(DW_AT_associated);
        RT_CASE_RET_STR(DW_AT_data_location);
        RT_CASE_RET_STR(DW_AT_byte_stride);
        RT_CASE_RET_STR(DW_AT_entry_pc);
        RT_CASE_RET_STR(DW_AT_use_UTF8);
        RT_CASE_RET_STR(DW_AT_extension);
        RT_CASE_RET_STR(DW_AT_ranges);
        RT_CASE_RET_STR(DW_AT_trampoline);
        RT_CASE_RET_STR(DW_AT_call_column);
        RT_CASE_RET_STR(DW_AT_call_file);
        RT_CASE_RET_STR(DW_AT_call_line);
        RT_CASE_RET_STR(DW_AT_description);
        RT_CASE_RET_STR(DW_AT_binary_scale);
        RT_CASE_RET_STR(DW_AT_decimal_scale);
        RT_CASE_RET_STR(DW_AT_small);
        RT_CASE_RET_STR(DW_AT_decimal_sign);
        RT_CASE_RET_STR(DW_AT_digit_count);
        RT_CASE_RET_STR(DW_AT_picture_string);
        RT_CASE_RET_STR(DW_AT_mutable);
        RT_CASE_RET_STR(DW_AT_threads_scaled);
        RT_CASE_RET_STR(DW_AT_explicit);
        RT_CASE_RET_STR(DW_AT_object_pointer);
        RT_CASE_RET_STR(DW_AT_endianity);
        RT_CASE_RET_STR(DW_AT_elemental);
        RT_CASE_RET_STR(DW_AT_pure);
        RT_CASE_RET_STR(DW_AT_recursive);
        RT_CASE_RET_STR(DW_AT_signature);
        RT_CASE_RET_STR(DW_AT_main_subprogram);
        RT_CASE_RET_STR(DW_AT_data_bit_offset);
        RT_CASE_RET_STR(DW_AT_const_expr);
        RT_CASE_RET_STR(DW_AT_enum_class);
        RT_CASE_RET_STR(DW_AT_linkage_name);
        RT_CASE_RET_STR(DW_AT_MIPS_linkage_name);
    }
    static char s_szStatic[32];
    RTStrPrintf(s_szStatic, sizeof(s_szStatic),"DW_AT_%#x", uAttr);
    return s_szStatic;
}


/**
 * Turns a form value into a string for logging purposes.
 *
 * @returns String name.
 * @param   uForm               The form.
 */
static const char *rtDwarfLog_FormName(uint32_t uForm)
{
    switch (uForm)
    {
        RT_CASE_RET_STR(DW_FORM_addr);
        RT_CASE_RET_STR(DW_FORM_block2);
        RT_CASE_RET_STR(DW_FORM_block4);
        RT_CASE_RET_STR(DW_FORM_data2);
        RT_CASE_RET_STR(DW_FORM_data4);
        RT_CASE_RET_STR(DW_FORM_data8);
        RT_CASE_RET_STR(DW_FORM_string);
        RT_CASE_RET_STR(DW_FORM_block);
        RT_CASE_RET_STR(DW_FORM_block1);
        RT_CASE_RET_STR(DW_FORM_data1);
        RT_CASE_RET_STR(DW_FORM_flag);
        RT_CASE_RET_STR(DW_FORM_sdata);
        RT_CASE_RET_STR(DW_FORM_strp);
        RT_CASE_RET_STR(DW_FORM_udata);
        RT_CASE_RET_STR(DW_FORM_ref_addr);
        RT_CASE_RET_STR(DW_FORM_ref1);
        RT_CASE_RET_STR(DW_FORM_ref2);
        RT_CASE_RET_STR(DW_FORM_ref4);
        RT_CASE_RET_STR(DW_FORM_ref8);
        RT_CASE_RET_STR(DW_FORM_ref_udata);
        RT_CASE_RET_STR(DW_FORM_indirect);
        RT_CASE_RET_STR(DW_FORM_sec_offset);
        RT_CASE_RET_STR(DW_FORM_exprloc);
        RT_CASE_RET_STR(DW_FORM_flag_present);
        RT_CASE_RET_STR(DW_FORM_ref_sig8);
    }
    static char s_szStatic[32];
    RTStrPrintf(s_szStatic, sizeof(s_szStatic),"DW_FORM_%#x", uForm);
    return s_szStatic;
}

#endif /* LOG_ENABLED || RT_STRICT */



/** @callback_method_impl{FNRTLDRENUMSEGS} */
static DECLCALLBACK(int) rtDbgModDwarfScanSegmentsCallback(RTLDRMOD hLdrMod, PCRTLDRSEG pSeg, void *pvUser)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pvUser;
    Log(("Segment %.*s: LinkAddress=%#llx RVA=%#llx cb=%#llx\n",
         pSeg->cchName, pSeg->pszName, (uint64_t)pSeg->LinkAddress, (uint64_t)pSeg->RVA, pSeg->cb));
    NOREF(hLdrMod);

    /* Count relevant segments. */
    if (pSeg->RVA != NIL_RTLDRADDR)
        pThis->cSegs++;

    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTLDRENUMSEGS} */
static DECLCALLBACK(int) rtDbgModDwarfAddSegmentsCallback(RTLDRMOD hLdrMod, PCRTLDRSEG pSeg, void *pvUser)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pvUser;
    Log(("Segment %.*s: LinkAddress=%#llx RVA=%#llx cb=%#llx cbMapped=%#llx\n",
         pSeg->cchName, pSeg->pszName, (uint64_t)pSeg->LinkAddress, (uint64_t)pSeg->RVA, pSeg->cb, pSeg->cbMapped));
    NOREF(hLdrMod);
    Assert(pSeg->cchName > 0);
    Assert(!pSeg->pszName[pSeg->cchName]);

    /* If the segment doesn't have a mapping, just add a dummy so the indexing
       works out correctly (same as for the image). */
    if (pSeg->RVA == NIL_RTLDRADDR)
        return RTDbgModSegmentAdd(pThis->hCnt, 0, 0, pSeg->pszName, 0 /*fFlags*/, NULL);

    /* The link address is 0 for all segments in a relocatable ELF image. */
    RTLDRADDR cb = RT_MAX(pSeg->cb, pSeg->cbMapped);
    return RTDbgModSegmentAdd(pThis->hCnt, pSeg->RVA, cb, pSeg->pszName, 0 /*fFlags*/, NULL);
}


/**
 * Calls pfnSegmentAdd for each segment in the executable image.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 */
static int rtDbgModDwarfAddSegmentsFromImage(PRTDBGMODDWARF pThis)
{
    AssertReturn(pThis->pImgMod && pThis->pImgMod->pImgVt, VERR_INTERNAL_ERROR_2);
    Assert(!pThis->cSegs);
    int rc = pThis->pImgMod->pImgVt->pfnEnumSegments(pThis->pImgMod, rtDbgModDwarfScanSegmentsCallback, pThis);
    if (RT_SUCCESS(rc))
    {
        if (pThis->cSegs == 0)
            pThis->iWatcomPass = 1;
        else
        {
            pThis->cSegs = 0;
            pThis->iWatcomPass = -1;
            rc = pThis->pImgMod->pImgVt->pfnEnumSegments(pThis->pImgMod, rtDbgModDwarfAddSegmentsCallback, pThis);
        }
    }

    return rc;
}


/**
 * Looks up a segment.
 *
 * @returns Pointer to the segment on success, NULL if not found.
 * @param   pThis               The DWARF instance.
 * @param   uSeg                The segment number / selector.
 */
static PRTDBGDWARFSEG rtDbgModDwarfFindSegment(PRTDBGMODDWARF pThis, RTSEL uSeg)
{
    uint32_t        cSegs  = pThis->cSegs;
    uint32_t        iSeg   = pThis->iSegHint;
    PRTDBGDWARFSEG  paSegs = pThis->paSegs;
    if (   iSeg < cSegs
        && paSegs[iSeg].uSegment == uSeg)
        return &paSegs[iSeg];

    for (iSeg = 0; iSeg < cSegs; iSeg++)
        if (uSeg == paSegs[iSeg].uSegment)
        {
            pThis->iSegHint = iSeg;
            return &paSegs[iSeg];
        }

    AssertFailed();
    return NULL;
}


/**
 * Record a segment:offset during pass 1.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 * @param   uSeg                The segment number / selector.
 * @param   offSeg              The segment offset.
 */
static int rtDbgModDwarfRecordSegOffset(PRTDBGMODDWARF pThis, RTSEL uSeg, uint64_t offSeg)
{
    /* Look up the segment. */
    uint32_t        cSegs  = pThis->cSegs;
    uint32_t        iSeg   = pThis->iSegHint;
    PRTDBGDWARFSEG  paSegs = pThis->paSegs;
    if (   iSeg >= cSegs
        || paSegs[iSeg].uSegment != uSeg)
    {
        for (iSeg = 0; iSeg < cSegs; iSeg++)
            if (uSeg <= paSegs[iSeg].uSegment)
                break;
        if (   iSeg >= cSegs
            || paSegs[iSeg].uSegment != uSeg)
        {
            /* Add */
            void *pvNew = RTMemRealloc(paSegs, (pThis->cSegs + 1) * sizeof(paSegs[0]));
            if (!pvNew)
                return VERR_NO_MEMORY;
            pThis->paSegs = paSegs = (PRTDBGDWARFSEG)pvNew;
            if (iSeg != cSegs)
                memmove(&paSegs[iSeg + 1], &paSegs[iSeg], (cSegs - iSeg) * sizeof(paSegs[0]));
            paSegs[iSeg].offHighest = offSeg;
            paSegs[iSeg].uBaseAddr  = 0;
            paSegs[iSeg].cbSegment  = 0;
            paSegs[iSeg].uSegment   = uSeg;
            pThis->cSegs++;
        }

        pThis->iSegHint = iSeg;
    }

    /* Increase it's range? */
    if (paSegs[iSeg].offHighest < offSeg)
    {
        Log3(("rtDbgModDwarfRecordSegOffset: iSeg=%d uSeg=%#06x offSeg=%#llx\n", iSeg, uSeg, offSeg));
        paSegs[iSeg].offHighest = offSeg;
    }

    return VINF_SUCCESS;
}


/**
 * Calls pfnSegmentAdd for each segment in the executable image.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 */
static int rtDbgModDwarfAddSegmentsFromPass1(PRTDBGMODDWARF pThis)
{
    AssertReturn(pThis->cSegs, VERR_DWARF_BAD_INFO);
    uint32_t const  cSegs   = pThis->cSegs;
    PRTDBGDWARFSEG  paSegs  = pThis->paSegs;

    /*
     * Are the segments assigned more or less in numerical order?
     */
    if (   paSegs[0].uSegment < 16U
        && paSegs[cSegs - 1].uSegment - paSegs[0].uSegment + 1U <= cSegs + 16U)
    {
        /** @todo heuristics, plase. */
        AssertFailedReturn(VERR_DWARF_TODO);

    }
    /*
     * Assume DOS segmentation.
     */
    else
    {
        for (uint32_t iSeg = 0; iSeg < cSegs; iSeg++)
            paSegs[iSeg].uBaseAddr = (uint32_t)paSegs[iSeg].uSegment << 16;
        for (uint32_t iSeg = 0; iSeg < cSegs; iSeg++)
            paSegs[iSeg].cbSegment = paSegs[iSeg].offHighest;
    }

    /*
     * Add them.
     */
    for (uint32_t iSeg = 0; iSeg < cSegs; iSeg++)
    {
        Log3(("rtDbgModDwarfAddSegmentsFromPass1: Seg#%u: %#010llx LB %#llx uSegment=%#x\n",
              iSeg, paSegs[iSeg].uBaseAddr, paSegs[iSeg].cbSegment, paSegs[iSeg].uSegment));
        char szName[32];
        RTStrPrintf(szName, sizeof(szName), "seg-%#04xh", paSegs[iSeg].uSegment);
        int rc = RTDbgModSegmentAdd(pThis->hCnt, paSegs[iSeg].uBaseAddr, paSegs[iSeg].cbSegment,
                                    szName, 0 /*fFlags*/, NULL);
        if (RT_FAILURE(rc))
            return rc;
    }

    return VINF_SUCCESS;
}


/**
 * Loads a DWARF section from the image file.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 * @param   enmSect             The section to load.
 */
static int rtDbgModDwarfLoadSection(PRTDBGMODDWARF pThis, krtDbgModDwarfSect enmSect)
{
    /*
     * Don't load stuff twice.
     */
    if (pThis->aSections[enmSect].pv)
        return VINF_SUCCESS;

    /*
     * Sections that are not present cannot be loaded, treat them like they
     * are empty
     */
    if (!pThis->aSections[enmSect].fPresent)
    {
        Assert(pThis->aSections[enmSect].cb);
        return VINF_SUCCESS;
    }
    if (!pThis->aSections[enmSect].cb)
        return VINF_SUCCESS;

    /*
     * Sections must be readable with the current image interface.
     */
    if (pThis->aSections[enmSect].offFile < 0)
        return VERR_OUT_OF_RANGE;

    /*
     * Do the job.
     */
    return pThis->pDbgInfoMod->pImgVt->pfnMapPart(pThis->pDbgInfoMod,
                                                  pThis->aSections[enmSect].iDbgInfo,
                                                  pThis->aSections[enmSect].offFile,
                                                  pThis->aSections[enmSect].cb,
                                                  &pThis->aSections[enmSect].pv);
}


#ifdef SOME_UNUSED_FUNCTION
/**
 * Unloads a DWARF section previously mapped by rtDbgModDwarfLoadSection.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 * @param   enmSect             The section to unload.
 */
static int rtDbgModDwarfUnloadSection(PRTDBGMODDWARF pThis, krtDbgModDwarfSect enmSect)
{
    if (!pThis->aSections[enmSect].pv)
        return VINF_SUCCESS;

    int rc = pThis->pDbgInfoMod->pImgVt->pfnUnmapPart(pThis->pDbgInfoMod, pThis->aSections[enmSect].cb, &pThis->aSections[enmSect].pv);
    AssertRC(rc);
    return rc;
}
#endif


/**
 * Converts to UTF-8 or otherwise makes sure it's valid UTF-8.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 * @param   ppsz                Pointer to the string pointer.  May be
 *                              reallocated (RTStr*).
 */
static int rtDbgModDwarfStringToUtf8(PRTDBGMODDWARF pThis, char **ppsz)
{
    /** @todo DWARF & UTF-8. */
    NOREF(pThis);
    RTStrPurgeEncoding(*ppsz);
    return VINF_SUCCESS;
}


/**
 * Convers a link address into a segment+offset or RVA.
 *
 * @returns IPRT status code.
 * @param   pThis           The DWARF instance.
 * @param   uSegment        The segment, 0 if not applicable.
 * @param   LinkAddress     The address to convert..
 * @param   piSeg           The segment index.
 * @param   poffSeg         Where to return the segment offset.
 */
static int rtDbgModDwarfLinkAddressToSegOffset(PRTDBGMODDWARF pThis, RTSEL uSegment, uint64_t LinkAddress,
                                               PRTDBGSEGIDX piSeg, PRTLDRADDR poffSeg)
{
    if (pThis->paSegs)
    {
        PRTDBGDWARFSEG pSeg = rtDbgModDwarfFindSegment(pThis, uSegment);
        if (pSeg)
        {
            *piSeg   = pSeg - pThis->paSegs;
            *poffSeg = LinkAddress;
            return VINF_SUCCESS;
        }
    }

    if (pThis->fUseLinkAddress)
        return pThis->pImgMod->pImgVt->pfnLinkAddressToSegOffset(pThis->pImgMod, LinkAddress, piSeg, poffSeg);
    return pThis->pImgMod->pImgVt->pfnRvaToSegOffset(pThis->pImgMod, LinkAddress, piSeg, poffSeg);
}


/*
 *
 * DWARF Cursor.
 * DWARF Cursor.
 * DWARF Cursor.
 *
 */


/**
 * Reads a 8-bit unsigned integer and advances the cursor.
 *
 * @returns 8-bit unsigned integer. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on read error.
 */
static uint8_t rtDwarfCursor_GetU8(PRTDWARFCURSOR pCursor, uint8_t uErrValue)
{
    if (pCursor->cbUnitLeft < 1)
    {
        pCursor->rc = VERR_DWARF_UNEXPECTED_END;
        return uErrValue;
    }

    uint8_t u8 = pCursor->pb[0];
    pCursor->pb         += 1;
    pCursor->cbUnitLeft -= 1;
    pCursor->cbLeft     -= 1;
    return u8;
}


/**
 * Reads a 16-bit unsigned integer and advances the cursor.
 *
 * @returns 16-bit unsigned integer. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on read error.
 */
static uint16_t rtDwarfCursor_GetU16(PRTDWARFCURSOR pCursor, uint16_t uErrValue)
{
    if (pCursor->cbUnitLeft < 2)
    {
        pCursor->pb         += pCursor->cbUnitLeft;
        pCursor->cbLeft     -= pCursor->cbUnitLeft;
        pCursor->cbUnitLeft  = 0;
        pCursor->rc          = VERR_DWARF_UNEXPECTED_END;
        return uErrValue;
    }

    uint16_t u16 = RT_MAKE_U16(pCursor->pb[0], pCursor->pb[1]);
    pCursor->pb         += 2;
    pCursor->cbUnitLeft -= 2;
    pCursor->cbLeft     -= 2;
    if (!pCursor->fNativEndian)
        u16 = RT_BSWAP_U16(u16);
    return u16;
}


/**
 * Reads a 32-bit unsigned integer and advances the cursor.
 *
 * @returns 32-bit unsigned integer. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on read error.
 */
static uint32_t rtDwarfCursor_GetU32(PRTDWARFCURSOR pCursor, uint32_t uErrValue)
{
    if (pCursor->cbUnitLeft < 4)
    {
        pCursor->pb         += pCursor->cbUnitLeft;
        pCursor->cbLeft     -= pCursor->cbUnitLeft;
        pCursor->cbUnitLeft  = 0;
        pCursor->rc          = VERR_DWARF_UNEXPECTED_END;
        return uErrValue;
    }

    uint32_t u32 = RT_MAKE_U32_FROM_U8(pCursor->pb[0], pCursor->pb[1], pCursor->pb[2], pCursor->pb[3]);
    pCursor->pb         += 4;
    pCursor->cbUnitLeft -= 4;
    pCursor->cbLeft     -= 4;
    if (!pCursor->fNativEndian)
        u32 = RT_BSWAP_U32(u32);
    return u32;
}


/**
 * Reads a 64-bit unsigned integer and advances the cursor.
 *
 * @returns 64-bit unsigned integer. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on read error.
 */
static uint64_t rtDwarfCursor_GetU64(PRTDWARFCURSOR pCursor, uint64_t uErrValue)
{
    if (pCursor->cbUnitLeft < 8)
    {
        pCursor->pb         += pCursor->cbUnitLeft;
        pCursor->cbLeft     -= pCursor->cbUnitLeft;
        pCursor->cbUnitLeft  = 0;
        pCursor->rc          = VERR_DWARF_UNEXPECTED_END;
        return uErrValue;
    }

    uint64_t u64 = RT_MAKE_U64_FROM_U8(pCursor->pb[0], pCursor->pb[1], pCursor->pb[2], pCursor->pb[3],
                                       pCursor->pb[4], pCursor->pb[5], pCursor->pb[6], pCursor->pb[7]);
    pCursor->pb         += 8;
    pCursor->cbUnitLeft -= 8;
    pCursor->cbLeft     -= 8;
    if (!pCursor->fNativEndian)
        u64 = RT_BSWAP_U64(u64);
    return u64;
}


/**
 * Reads an unsigned LEB128 encoded number.
 *
 * @returns unsigned 64-bit number. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           The value to return on error.
 */
static uint64_t rtDwarfCursor_GetULeb128(PRTDWARFCURSOR pCursor, uint64_t uErrValue)
{
    if (pCursor->cbUnitLeft < 1)
    {
        pCursor->rc = VERR_DWARF_UNEXPECTED_END;
        return uErrValue;
    }

    /*
     * Special case - single byte.
     */
    uint8_t b = pCursor->pb[0];
    if (!(b & 0x80))
    {
        pCursor->pb         += 1;
        pCursor->cbUnitLeft -= 1;
        pCursor->cbLeft     -= 1;
        return b;
    }

    /*
     * Generic case.
     */
    /* Decode. */
    uint32_t off    = 1;
    uint64_t u64Ret = b & 0x7f;
    do
    {
        if (off == pCursor->cbUnitLeft)
        {
            pCursor->rc = VERR_DWARF_UNEXPECTED_END;
            u64Ret = uErrValue;
            break;
        }
        b = pCursor->pb[off];
        u64Ret |= (b & 0x7f) << off * 7;
        off++;
    } while (b & 0x80);

    /* Update the cursor. */
    pCursor->pb         += off;
    pCursor->cbUnitLeft -= off;
    pCursor->cbLeft     -= off;

    /* Check the range. */
    uint32_t cBits = off * 7;
    if (cBits > 64)
    {
        pCursor->rc = VERR_DWARF_LEB_OVERFLOW;
        u64Ret = uErrValue;
    }

    return u64Ret;
}


/**
 * Reads a signed LEB128 encoded number.
 *
 * @returns signed 64-bit number. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   sErrValue           The value to return on error.
 */
static int64_t rtDwarfCursor_GetSLeb128(PRTDWARFCURSOR pCursor, int64_t sErrValue)
{
    if (pCursor->cbUnitLeft < 1)
    {
        pCursor->rc = VERR_DWARF_UNEXPECTED_END;
        return sErrValue;
    }

    /*
     * Special case - single byte.
     */
    uint8_t b = pCursor->pb[0];
    if (!(b & 0x80))
    {
        pCursor->pb         += 1;
        pCursor->cbUnitLeft -= 1;
        pCursor->cbLeft     -= 1;
        if (b & 0x40)
            b |= 0x80;
        return (int8_t)b;
    }

    /*
     * Generic case.
     */
    /* Decode it. */
    uint32_t off    = 1;
    uint64_t u64Ret = b & 0x7f;
    do
    {
        if (off == pCursor->cbUnitLeft)
        {
            pCursor->rc = VERR_DWARF_UNEXPECTED_END;
            u64Ret = (uint64_t)sErrValue;
            break;
        }
        b = pCursor->pb[off];
        u64Ret |= (b & 0x7f) << off * 7;
        off++;
    } while (b & 0x80);

    /* Update cursor. */
    pCursor->pb         += off;
    pCursor->cbUnitLeft -= off;
    pCursor->cbLeft     -= off;

    /* Check the range. */
    uint32_t cBits = off * 7;
    if (cBits > 64)
    {
        pCursor->rc = VERR_DWARF_LEB_OVERFLOW;
        u64Ret = (uint64_t)sErrValue;
    }
    /* Sign extend the value. */
    else if (u64Ret & RT_BIT_64(cBits - 1))
        u64Ret |= ~(RT_BIT_64(cBits - 1) - 1);

    return (int64_t)u64Ret;
}


/**
 * Reads an unsigned LEB128 encoded number, max 32-bit width.
 *
 * @returns unsigned 32-bit number. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           The value to return on error.
 */
static uint32_t rtDwarfCursor_GetULeb128AsU32(PRTDWARFCURSOR pCursor, uint32_t uErrValue)
{
    uint64_t u64 = rtDwarfCursor_GetULeb128(pCursor, uErrValue);
    if (u64 > UINT32_MAX)
    {
        pCursor->rc = VERR_DWARF_LEB_OVERFLOW;
        return uErrValue;
    }
    return (uint32_t)u64;
}


/**
 * Reads a signed LEB128 encoded number, max 32-bit width.
 *
 * @returns signed 32-bit number. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   sErrValue           The value to return on error.
 */
static int32_t rtDwarfCursor_GetSLeb128AsS32(PRTDWARFCURSOR pCursor, int32_t sErrValue)
{
    int64_t s64 = rtDwarfCursor_GetSLeb128(pCursor, sErrValue);
    if (s64 > INT32_MAX || s64 < INT32_MIN)
    {
        pCursor->rc = VERR_DWARF_LEB_OVERFLOW;
        return sErrValue;
    }
    return (int32_t)s64;
}


/**
 * Skips a LEB128 encoded number.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 */
static int rtDwarfCursor_SkipLeb128(PRTDWARFCURSOR pCursor)
{
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;

    if (pCursor->cbUnitLeft < 1)
        return pCursor->rc = VERR_DWARF_UNEXPECTED_END;

    uint32_t offSkip = 1;
    if (pCursor->pb[0] & 0x80)
        do
        {
            if (offSkip == pCursor->cbUnitLeft)
            {
                pCursor->rc = VERR_DWARF_UNEXPECTED_END;
                break;
            }
        } while (pCursor->pb[offSkip++] & 0x80);

    pCursor->pb         += offSkip;
    pCursor->cbUnitLeft -= offSkip;
    pCursor->cbLeft     -= offSkip;
    return pCursor->rc;
}


/**
 * Advances the cursor a given number of bytes.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 * @param   offSkip             The number of bytes to advance.
 */
static int rtDwarfCursor_SkipBytes(PRTDWARFCURSOR pCursor, uint64_t offSkip)
{
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;
    if (pCursor->cbUnitLeft < offSkip)
        return pCursor->rc = VERR_DWARF_UNEXPECTED_END;

    size_t const offSkipSizeT = (size_t)offSkip;
    pCursor->cbUnitLeft -= offSkipSizeT;
    pCursor->cbLeft     -= offSkipSizeT;
    pCursor->pb         += offSkipSizeT;

    return VINF_SUCCESS;
}


/**
 * Reads a zero terminated string, advancing the cursor beyond the terminator.
 *
 * @returns Pointer to the string.
 * @param   pCursor             The cursor.
 * @param   pszErrValue         What to return if the string isn't terminated
 *                              before the end of the unit.
 */
static const char *rtDwarfCursor_GetSZ(PRTDWARFCURSOR pCursor, const char *pszErrValue)
{
    const char *pszRet = (const char *)pCursor->pb;
    for (;;)
    {
        if (!pCursor->cbUnitLeft)
        {
            pCursor->rc = VERR_DWARF_BAD_STRING;
            return pszErrValue;
        }
        pCursor->cbUnitLeft--;
        pCursor->cbLeft--;
        if (!*pCursor->pb++)
            break;
    }
    return pszRet;
}


/**
 * Reads a 1, 2, 4 or 8 byte unsgined value.
 *
 * @returns 64-bit unsigned value.
 * @param   pCursor             The cursor.
 * @param   cbValue             The value size.
 * @param   uErrValue           The error value.
 */
static uint64_t rtDwarfCursor_GetVarSizedU(PRTDWARFCURSOR pCursor, size_t cbValue, uint64_t uErrValue)
{
    uint64_t u64Ret;
    switch (cbValue)
    {
        case 1: u64Ret = rtDwarfCursor_GetU8( pCursor, UINT8_MAX); break;
        case 2: u64Ret = rtDwarfCursor_GetU16(pCursor, UINT16_MAX); break;
        case 4: u64Ret = rtDwarfCursor_GetU32(pCursor, UINT32_MAX); break;
        case 8: u64Ret = rtDwarfCursor_GetU64(pCursor, UINT64_MAX); break;
        default:
            pCursor->rc = VERR_DWARF_BAD_INFO;
            return uErrValue;
    }
    if (RT_FAILURE(pCursor->rc))
        return uErrValue;
    return u64Ret;
}


#if 0 /* unused */
/**
 * Gets the pointer to a variable size block and advances the cursor.
 *
 * @returns Pointer to the block at the current cursor location. On error
 *          RTDWARFCURSOR::rc is set and NULL returned.
 * @param   pCursor             The cursor.
 * @param   cbBlock             The block size.
 */
static const uint8_t *rtDwarfCursor_GetBlock(PRTDWARFCURSOR pCursor, uint32_t cbBlock)
{
    if (cbBlock > pCursor->cbUnitLeft)
    {
        pCursor->rc = VERR_DWARF_UNEXPECTED_END;
        return NULL;
    }

    uint8_t const *pb = &pCursor->pb[0];
    pCursor->pb         += cbBlock;
    pCursor->cbUnitLeft -= cbBlock;
    pCursor->cbLeft     -= cbBlock;
    return pb;
}
#endif


/**
 * Reads an unsigned DWARF half number.
 *
 * @returns The number. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on error.
 */
static uint16_t rtDwarfCursor_GetUHalf(PRTDWARFCURSOR pCursor, uint16_t uErrValue)
{
    return rtDwarfCursor_GetU16(pCursor, uErrValue);
}


/**
 * Reads an unsigned DWARF byte number.
 *
 * @returns The number. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on error.
 */
static uint8_t rtDwarfCursor_GetUByte(PRTDWARFCURSOR pCursor, uint8_t uErrValue)
{
    return rtDwarfCursor_GetU8(pCursor, uErrValue);
}


/**
 * Reads a signed DWARF byte number.
 *
 * @returns The number. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   iErrValue           What to return on error.
 */
static int8_t rtDwarfCursor_GetSByte(PRTDWARFCURSOR pCursor, int8_t iErrValue)
{
    return (int8_t)rtDwarfCursor_GetU8(pCursor, (uint8_t)iErrValue);
}


/**
 * Reads a unsigned DWARF offset value.
 *
 * @returns The value. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on error.
 */
static uint64_t rtDwarfCursor_GetUOff(PRTDWARFCURSOR pCursor, uint64_t uErrValue)
{
    if (pCursor->f64bitDwarf)
        return rtDwarfCursor_GetU64(pCursor, uErrValue);
    return rtDwarfCursor_GetU32(pCursor, (uint32_t)uErrValue);
}


/**
 * Reads a unsigned DWARF native offset value.
 *
 * @returns The value. On error RTDWARFCURSOR::rc is set and @a
 *          uErrValue is returned.
 * @param   pCursor             The cursor.
 * @param   uErrValue           What to return on error.
 */
static uint64_t rtDwarfCursor_GetNativeUOff(PRTDWARFCURSOR pCursor, uint64_t uErrValue)
{
    switch (pCursor->cbNativeAddr)
    {
        case 1: return rtDwarfCursor_GetU8(pCursor,  (uint8_t )uErrValue);
        case 2: return rtDwarfCursor_GetU16(pCursor, (uint16_t)uErrValue);
        case 4: return rtDwarfCursor_GetU32(pCursor, (uint32_t)uErrValue);
        case 8: return rtDwarfCursor_GetU64(pCursor, uErrValue);
        default:
            pCursor->rc = VERR_INTERNAL_ERROR_2;
            return uErrValue;
    }
}


/**
 * Gets the unit length, updating the unit length member and DWARF bitness
 * members of the cursor.
 *
 * @returns The unit length.
 * @param   pCursor             The cursor.
 */
static uint64_t rtDwarfCursor_GetInitalLength(PRTDWARFCURSOR pCursor)
{
    /*
     * Read the initial length.
     */
    pCursor->cbUnitLeft = pCursor->cbLeft;
    uint64_t cbUnit = rtDwarfCursor_GetU32(pCursor, 0);
    if (cbUnit != UINT32_C(0xffffffff))
        pCursor->f64bitDwarf = false;
    else
    {
        pCursor->f64bitDwarf = true;
        cbUnit = rtDwarfCursor_GetU64(pCursor, 0);
    }


    /*
     * Set the unit length, quitely fixing bad lengths.
     */
    pCursor->cbUnitLeft = (size_t)cbUnit;
    if (   pCursor->cbUnitLeft > pCursor->cbLeft
        || pCursor->cbUnitLeft != cbUnit)
        pCursor->cbUnitLeft = pCursor->cbLeft;

    return cbUnit;
}


/**
 * Calculates the section offset corresponding to the current cursor position.
 *
 * @returns 32-bit section offset. If out of range, RTDWARFCURSOR::rc will be
 *          set and UINT32_MAX returned.
 * @param   pCursor             The cursor.
 */
static uint32_t rtDwarfCursor_CalcSectOffsetU32(PRTDWARFCURSOR pCursor)
{
    size_t off = pCursor->pb - (uint8_t const *)pCursor->pDwarfMod->aSections[pCursor->enmSect].pv;
    uint32_t offRet = (uint32_t)off;
    if (offRet != off)
    {
        AssertFailed();
        pCursor->rc = VERR_OUT_OF_RANGE;
        offRet = UINT32_MAX;
    }
    return offRet;
}


/**
 * Calculates an absolute cursor position from one relative to the current
 * cursor position.
 *
 * @returns The absolute cursor position.
 * @param   pCursor             The cursor.
 * @param   offRelative         The relative position.  Must be a positive
 *                              offset.
 */
static uint8_t const *rtDwarfCursor_CalcPos(PRTDWARFCURSOR pCursor, size_t offRelative)
{
    if (offRelative > pCursor->cbUnitLeft)
    {
        Log(("rtDwarfCursor_CalcPos: bad position %#zx, cbUnitLeft=%#zu\n", offRelative, pCursor->cbUnitLeft));
        pCursor->rc = VERR_DWARF_BAD_POS;
        return NULL;
    }
    return pCursor->pb + offRelative;
}


/**
 * Advances the cursor to the given position.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 * @param   pbNewPos            The new position - returned by
 *                              rtDwarfCursor_CalcPos().
 */
static int rtDwarfCursor_AdvanceToPos(PRTDWARFCURSOR pCursor, uint8_t const *pbNewPos)
{
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;
    AssertPtr(pbNewPos);
    if ((uintptr_t)pbNewPos < (uintptr_t)pCursor->pb)
    {
        Log(("rtDwarfCursor_AdvanceToPos: bad position %p, current %p\n", pbNewPos, pCursor->pb));
        return pCursor->rc = VERR_DWARF_BAD_POS;
    }

    uintptr_t cbAdj = (uintptr_t)pbNewPos - (uintptr_t)pCursor->pb;
    if (RT_UNLIKELY(cbAdj > pCursor->cbUnitLeft))
    {
        AssertFailed();
        pCursor->rc = VERR_DWARF_BAD_POS;
        cbAdj = pCursor->cbUnitLeft;
    }

    pCursor->cbUnitLeft -= cbAdj;
    pCursor->cbLeft     -= cbAdj;
    pCursor->pb         += cbAdj;
    return pCursor->rc;
}


/**
 * Check if the cursor is at the end of the current DWARF unit.
 *
 * @retval  true if at the end or a cursor error is pending.
 * @retval  false if not.
 * @param   pCursor             The cursor.
 */
static bool rtDwarfCursor_IsAtEndOfUnit(PRTDWARFCURSOR pCursor)
{
    return !pCursor->cbUnitLeft || RT_FAILURE(pCursor->rc);
}


/**
 * Skips to the end of the current unit.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 */
static int rtDwarfCursor_SkipUnit(PRTDWARFCURSOR pCursor)
{
    pCursor->pb        += pCursor->cbUnitLeft;
    pCursor->cbLeft    -= pCursor->cbUnitLeft;
    pCursor->cbUnitLeft = 0;
    return pCursor->rc;
}


/**
 * Check if the cursor is at the end of the section (or whatever the cursor is
 * processing).
 *
 * @retval  true if at the end or a cursor error is pending.
 * @retval  false if not.
 * @param   pCursor             The cursor.
 */
static bool rtDwarfCursor_IsAtEnd(PRTDWARFCURSOR pCursor)
{
    return !pCursor->cbLeft || RT_FAILURE(pCursor->rc);
}


/**
 * Initialize a section reader cursor.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 * @param   pThis               The dwarf module.
 * @param   enmSect             The name of the section to read.
 */
static int rtDwarfCursor_Init(PRTDWARFCURSOR pCursor, PRTDBGMODDWARF pThis, krtDbgModDwarfSect enmSect)
{
    int rc = rtDbgModDwarfLoadSection(pThis, enmSect);
    if (RT_FAILURE(rc))
        return rc;

    pCursor->enmSect          = enmSect;
    pCursor->pbStart          = (uint8_t const *)pThis->aSections[enmSect].pv;
    pCursor->pb               = pCursor->pbStart;
    pCursor->cbLeft           = pThis->aSections[enmSect].cb;
    pCursor->cbUnitLeft       = pCursor->cbLeft;
    pCursor->pDwarfMod        = pThis;
    pCursor->f64bitDwarf      = false;
    /** @todo ask the image about the endian used as well as the address
     *        width. */
    pCursor->fNativEndian     = true;
    pCursor->cbNativeAddr     = 4;
    pCursor->rc               = VINF_SUCCESS;

    return VINF_SUCCESS;
}


/**
 * Initialize a section reader cursor with an offset.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 * @param   pThis               The dwarf module.
 * @param   enmSect             The name of the section to read.
 * @param   offSect             The offset into the section.
 */
static int rtDwarfCursor_InitWithOffset(PRTDWARFCURSOR pCursor, PRTDBGMODDWARF pThis,
                                        krtDbgModDwarfSect enmSect, uint32_t offSect)
{
    if (offSect > pThis->aSections[enmSect].cb)
    {
        Log(("rtDwarfCursor_InitWithOffset: offSect=%#x cb=%#x enmSect=%d\n", offSect, pThis->aSections[enmSect].cb, enmSect));
        return VERR_DWARF_BAD_POS;
    }

    int rc = rtDwarfCursor_Init(pCursor, pThis, enmSect);
    if (RT_SUCCESS(rc))
    {
        pCursor->pbStart    += offSect;
        pCursor->pb         += offSect;
        pCursor->cbLeft     -= offSect;
        pCursor->cbUnitLeft -= offSect;
    }

    return rc;
}


/**
 * Initialize a cursor for a block (subsection) retrieved from the given cursor.
 *
 * The parent cursor will be advanced past the block.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 * @param   pParent             The parent cursor. Will be moved by @a cbBlock.
 * @param   cbBlock             The size of the block the new cursor should
 *                              cover.
 */
static int rtDwarfCursor_InitForBlock(PRTDWARFCURSOR pCursor, PRTDWARFCURSOR pParent, uint32_t cbBlock)
{
    if (RT_FAILURE(pParent->rc))
        return pParent->rc;
    if (pParent->cbUnitLeft < cbBlock)
    {
        Log(("rtDwarfCursor_InitForBlock: cbUnitLeft=%#x < cbBlock=%#x \n", pParent->cbUnitLeft, cbBlock));
        return VERR_DWARF_BAD_POS;
    }

    *pCursor = *pParent;
    pCursor->cbLeft     = cbBlock;
    pCursor->cbUnitLeft = cbBlock;

    pParent->pb         += cbBlock;
    pParent->cbLeft     -= cbBlock;
    pParent->cbUnitLeft -= cbBlock;

    return VINF_SUCCESS;
}


/**
 * Deletes a section reader initialized by rtDwarfCursor_Init.
 *
 * @returns @a rcOther or RTDWARCURSOR::rc.
 * @param   pCursor             The section reader.
 * @param   rcOther             Other error code to be returned if it indicates
 *                              error or if the cursor status is OK.
 */
static int rtDwarfCursor_Delete(PRTDWARFCURSOR pCursor, int rcOther)
{
    /* ... and a drop of poison. */
    pCursor->pb         = NULL;
    pCursor->cbLeft     = ~(size_t)0;
    pCursor->cbUnitLeft = ~(size_t)0;
    pCursor->pDwarfMod  = NULL;
    if (RT_FAILURE(pCursor->rc) && RT_SUCCESS(rcOther))
        rcOther = pCursor->rc;
    pCursor->rc         = VERR_INTERNAL_ERROR_4;
    return rcOther;
}


/*
 *
 * DWARF Line Numbers.
 * DWARF Line Numbers.
 * DWARF Line Numbers.
 *
 */


/**
 * Defines a file name.
 *
 * @returns IPRT status code.
 * @param   pLnState            The line number program state.
 * @param   pszFilename         The name of the file.
 * @param   idxInc              The include path index.
 */
static int rtDwarfLine_DefineFileName(PRTDWARFLINESTATE pLnState, const char *pszFilename, uint64_t idxInc)
{
    /*
     * Resize the array if necessary.
     */
    uint32_t iFileName = pLnState->cFileNames;
    if ((iFileName % 2) == 0)
    {
        void *pv = RTMemRealloc(pLnState->papszFileNames, sizeof(pLnState->papszFileNames[0]) * (iFileName + 2));
        if (!pv)
            return VERR_NO_MEMORY;
        pLnState->papszFileNames = (char **)pv;
    }

    /*
     * Add the file name.
     */
    if (   pszFilename[0] == '/'
        || pszFilename[0] == '\\'
        || (RT_C_IS_ALPHA(pszFilename[0]) && pszFilename[1] == ':') )
        pLnState->papszFileNames[iFileName] = RTStrDup(pszFilename);
    else if (idxInc < pLnState->cIncPaths)
        pLnState->papszFileNames[iFileName] = RTPathJoinA(pLnState->papszIncPaths[idxInc], pszFilename);
    else
        return VERR_DWARF_BAD_LINE_NUMBER_HEADER;
    if (!pLnState->papszFileNames[iFileName])
        return VERR_NO_STR_MEMORY;
    pLnState->cFileNames = iFileName + 1;

    /*
     * Sanitize the name.
     */
    int rc = rtDbgModDwarfStringToUtf8(pLnState->pDwarfMod, &pLnState->papszFileNames[iFileName]);
    Log(("  File #%02u = '%s'\n", iFileName, pLnState->papszFileNames[iFileName]));
    return rc;
}


/**
 * Adds a line to the table and resets parts of the state (DW_LNS_copy).
 *
 * @returns IPRT status code
 * @param   pLnState            The line number program state.
 * @param   offOpCode           The opcode offset (for logging
 *                              purposes).
 */
static int rtDwarfLine_AddLine(PRTDWARFLINESTATE pLnState, uint32_t offOpCode)
{
    PRTDBGMODDWARF  pThis = pLnState->pDwarfMod;
    int             rc;
    if (pThis->iWatcomPass == 1)
        rc = rtDbgModDwarfRecordSegOffset(pThis, pLnState->Regs.uSegment, pLnState->Regs.uAddress + 1);
    else
    {
        const char *pszFile = pLnState->Regs.iFile < pLnState->cFileNames
                            ? pLnState->papszFileNames[pLnState->Regs.iFile]
                            : "<bad file name index>";
        NOREF(offOpCode);

        RTDBGSEGIDX iSeg;
        RTUINTPTR   offSeg;
        rc = rtDbgModDwarfLinkAddressToSegOffset(pLnState->pDwarfMod, pLnState->Regs.uSegment, pLnState->Regs.uAddress,
                                                 &iSeg, &offSeg); /*AssertRC(rc);*/
        if (RT_SUCCESS(rc))
        {
            Log2(("rtDwarfLine_AddLine: %x:%08llx (%#llx) %s(%d) [offOpCode=%08x]\n", iSeg, offSeg, pLnState->Regs.uAddress, pszFile, pLnState->Regs.uLine, offOpCode));
            rc = RTDbgModLineAdd(pLnState->pDwarfMod->hCnt, pszFile, pLnState->Regs.uLine, iSeg, offSeg, NULL);

            /* Ignore address conflicts for now. */
            if (rc == VERR_DBG_ADDRESS_CONFLICT)
                rc = VINF_SUCCESS;
        }
        else
            rc = VINF_SUCCESS; /* ignore failure */
    }

    pLnState->Regs.fBasicBlock    = false;
    pLnState->Regs.fPrologueEnd   = false;
    pLnState->Regs.fEpilogueBegin = false;
    pLnState->Regs.uDiscriminator = 0;
    return rc;
}


/**
 * Reset the program to the start-of-sequence state.
 *
 * @param   pLnState            The line number program state.
 */
static void rtDwarfLine_ResetState(PRTDWARFLINESTATE pLnState)
{
    pLnState->Regs.uAddress         = 0;
    pLnState->Regs.idxOp            = 0;
    pLnState->Regs.iFile            = 1;
    pLnState->Regs.uLine            = 1;
    pLnState->Regs.uColumn          = 0;
    pLnState->Regs.fIsStatement     = RT_BOOL(pLnState->Hdr.u8DefIsStmt);
    pLnState->Regs.fBasicBlock      = false;
    pLnState->Regs.fEndSequence     = false;
    pLnState->Regs.fPrologueEnd     = false;
    pLnState->Regs.fEpilogueBegin   = false;
    pLnState->Regs.uIsa             = 0;
    pLnState->Regs.uDiscriminator   = 0;
    pLnState->Regs.uSegment         = 0;
}


/**
 * Runs the line number program.
 *
 * @returns IPRT status code.
 * @param   pLnState            The line number program state.
 * @param   pCursor             The cursor.
 */
static int rtDwarfLine_RunProgram(PRTDWARFLINESTATE pLnState, PRTDWARFCURSOR pCursor)
{
    LogFlow(("rtDwarfLine_RunProgram: cbUnitLeft=%zu\n", pCursor->cbUnitLeft));

    int rc = VINF_SUCCESS;
    rtDwarfLine_ResetState(pLnState);

    while (!rtDwarfCursor_IsAtEndOfUnit(pCursor))
    {
#ifdef LOG_ENABLED
        uint32_t const offOpCode = rtDwarfCursor_CalcSectOffsetU32(pCursor);
#else
        uint32_t const offOpCode = 0;
#endif
        uint8_t        bOpCode   = rtDwarfCursor_GetUByte(pCursor, DW_LNS_extended);
        if (bOpCode >= pLnState->Hdr.u8OpcodeBase)
        {
            /*
             * Special opcode.
             */
            uint8_t const bLogOpCode = bOpCode; NOREF(bLogOpCode);
            bOpCode -= pLnState->Hdr.u8OpcodeBase;

            int32_t const cLineDelta = bOpCode % pLnState->Hdr.u8LineRange + (int32_t)pLnState->Hdr.s8LineBase;
            bOpCode /= pLnState->Hdr.u8LineRange;

            uint64_t uTmp = bOpCode + pLnState->Regs.idxOp;
            uint64_t const cAddressDelta = uTmp / pLnState->Hdr.cMaxOpsPerInstr * pLnState->Hdr.cbMinInstr;
            uint64_t const cOpIndexDelta = uTmp % pLnState->Hdr.cMaxOpsPerInstr;

            pLnState->Regs.uLine    += cLineDelta;
            pLnState->Regs.uAddress += cAddressDelta;
            pLnState->Regs.idxOp    += cOpIndexDelta;
            Log2(("%08x: DW Special Opcode %#04x: uLine + %d => %u; uAddress + %#llx => %#llx; idxOp + %#llx => %#llx\n",
                  offOpCode, bLogOpCode, cLineDelta, pLnState->Regs.uLine, cAddressDelta, pLnState->Regs.uAddress,
                  cOpIndexDelta, pLnState->Regs.idxOp));

            /*
             * LLVM emits debug info for global constructors (_GLOBAL__I_a) which are not part of source
             * code but are inserted by the compiler: The resulting line number will be 0
             * because they are not part of the source file obviously (see https://reviews.llvm.org/rL205999),
             * so skip adding them when they are encountered.
             */
            if (pLnState->Regs.uLine)
                rc = rtDwarfLine_AddLine(pLnState, offOpCode);
        }
        else
        {
            switch (bOpCode)
            {
                /*
                 * Standard opcode.
                 */
                case DW_LNS_copy:
                    Log2(("%08x: DW_LNS_copy\n", offOpCode));
                    /* See the comment about LLVM above. */
                    if (pLnState->Regs.uLine)
                        rc = rtDwarfLine_AddLine(pLnState, offOpCode);
                    break;

                case DW_LNS_advance_pc:
                {
                    uint64_t u64Adv = rtDwarfCursor_GetULeb128(pCursor, 0);
                    pLnState->Regs.uAddress += (pLnState->Regs.idxOp + u64Adv) / pLnState->Hdr.cMaxOpsPerInstr
                                             * pLnState->Hdr.cbMinInstr;
                    pLnState->Regs.idxOp    += (pLnState->Regs.idxOp + u64Adv) % pLnState->Hdr.cMaxOpsPerInstr;
                    Log2(("%08x: DW_LNS_advance_pc: u64Adv=%#llx (%lld) )\n", offOpCode, u64Adv, u64Adv));
                    break;
                }

                case DW_LNS_advance_line:
                {
                    int32_t cLineDelta = rtDwarfCursor_GetSLeb128AsS32(pCursor, 0);
                    pLnState->Regs.uLine += cLineDelta;
                    Log2(("%08x: DW_LNS_advance_line: uLine + %d => %u\n", offOpCode, cLineDelta, pLnState->Regs.uLine));
                    break;
                }

                case DW_LNS_set_file:
                    pLnState->Regs.iFile = rtDwarfCursor_GetULeb128AsU32(pCursor, 0);
                    Log2(("%08x: DW_LNS_set_file: iFile=%u\n", offOpCode, pLnState->Regs.iFile));
                    break;

                case DW_LNS_set_column:
                    pLnState->Regs.uColumn = rtDwarfCursor_GetULeb128AsU32(pCursor, 0);
                    Log2(("%08x: DW_LNS_set_column\n", offOpCode));
                    break;

                case DW_LNS_negate_stmt:
                    pLnState->Regs.fIsStatement = !pLnState->Regs.fIsStatement;
                    Log2(("%08x: DW_LNS_negate_stmt\n", offOpCode));
                    break;

                case DW_LNS_set_basic_block:
                    pLnState->Regs.fBasicBlock = true;
                    Log2(("%08x: DW_LNS_set_basic_block\n", offOpCode));
                    break;

                case DW_LNS_const_add_pc:
                {
                    uint8_t u8Adv = (255 - pLnState->Hdr.u8OpcodeBase) / pLnState->Hdr.u8LineRange;
                    if (pLnState->Hdr.cMaxOpsPerInstr <= 1)
                        pLnState->Regs.uAddress += (uint32_t)pLnState->Hdr.cbMinInstr * u8Adv;
                    else
                    {
                        pLnState->Regs.uAddress += (pLnState->Regs.idxOp + u8Adv) / pLnState->Hdr.cMaxOpsPerInstr
                                                 * pLnState->Hdr.cbMinInstr;
                        pLnState->Regs.idxOp     = (pLnState->Regs.idxOp + u8Adv) % pLnState->Hdr.cMaxOpsPerInstr;
                    }
                    Log2(("%08x: DW_LNS_const_add_pc\n", offOpCode));
                    break;
                }
                case DW_LNS_fixed_advance_pc:
                    pLnState->Regs.uAddress += rtDwarfCursor_GetUHalf(pCursor, 0);
                    pLnState->Regs.idxOp     = 0;
                    Log2(("%08x: DW_LNS_fixed_advance_pc\n", offOpCode));
                    break;

                case DW_LNS_set_prologue_end:
                    pLnState->Regs.fPrologueEnd = true;
                    Log2(("%08x: DW_LNS_set_prologue_end\n", offOpCode));
                    break;

                case DW_LNS_set_epilogue_begin:
                    pLnState->Regs.fEpilogueBegin = true;
                    Log2(("%08x: DW_LNS_set_epilogue_begin\n", offOpCode));
                    break;

                case DW_LNS_set_isa:
                    pLnState->Regs.uIsa = rtDwarfCursor_GetULeb128AsU32(pCursor, 0);
                    Log2(("%08x: DW_LNS_set_isa %#x\n", offOpCode, pLnState->Regs.uIsa));
                    break;

                default:
                {
                    unsigned cOpsToSkip = pLnState->Hdr.pacStdOperands[bOpCode - 1];
                    Log(("rtDwarfLine_RunProgram: Unknown standard opcode %#x, %#x operands, at %08x.\n", bOpCode, cOpsToSkip, offOpCode));
                    while (cOpsToSkip-- > 0)
                        rc = rtDwarfCursor_SkipLeb128(pCursor);
                    break;
                }

                /*
                 * Extended opcode.
                 */
                case DW_LNS_extended:
                {
                    /* The instruction has a length prefix. */
                    uint64_t cbInstr = rtDwarfCursor_GetULeb128(pCursor, UINT64_MAX);
                    if (RT_FAILURE(pCursor->rc))
                        return pCursor->rc;
                    if (cbInstr > pCursor->cbUnitLeft)
                        return VERR_DWARF_BAD_LNE;
                    uint8_t const * const pbEndOfInstr = rtDwarfCursor_CalcPos(pCursor, cbInstr);

                    /* Get the opcode and deal with it if we know it. */
                    bOpCode = rtDwarfCursor_GetUByte(pCursor, 0);
                    switch (bOpCode)
                    {
                        case DW_LNE_end_sequence:
#if 0 /* No need for this, I think. */
                            pLnState->Regs.fEndSequence = true;
                            rc = rtDwarfLine_AddLine(pLnState, offOpCode);
#endif
                            rtDwarfLine_ResetState(pLnState);
                            Log2(("%08x: DW_LNE_end_sequence\n", offOpCode));
                            break;

                        case DW_LNE_set_address:
                            pLnState->Regs.uAddress = rtDwarfCursor_GetVarSizedU(pCursor, cbInstr - 1, UINT64_MAX);
                            pLnState->Regs.idxOp    = 0;
                            Log2(("%08x: DW_LNE_set_address: %#llx\n", offOpCode, pLnState->Regs.uAddress));
                            break;

                        case DW_LNE_define_file:
                        {
                            const char *pszFilename = rtDwarfCursor_GetSZ(pCursor, NULL);
                            uint32_t    idxInc      = rtDwarfCursor_GetULeb128AsU32(pCursor, UINT32_MAX);
                            rtDwarfCursor_SkipLeb128(pCursor); /* st_mtime */
                            rtDwarfCursor_SkipLeb128(pCursor); /* st_size */
                            Log2(("%08x: DW_LNE_define_file: {%d}/%s\n", offOpCode, idxInc, pszFilename));

                            rc = rtDwarfCursor_AdvanceToPos(pCursor, pbEndOfInstr);
                            if (RT_SUCCESS(rc))
                                rc = rtDwarfLine_DefineFileName(pLnState, pszFilename, idxInc);
                            break;
                        }

                        /*
                         * Note! Was defined in DWARF 4.  But... Watcom used it
                         *       for setting the segment in DWARF 2, creating
                         *       an incompatibility with the newer standard.
                         */
                        case DW_LNE_set_descriminator:
                            if (pLnState->Hdr.uVer != 2)
                            {
                                Assert(pLnState->Hdr.uVer >= 4);
                                pLnState->Regs.uDiscriminator = rtDwarfCursor_GetULeb128AsU32(pCursor, UINT32_MAX);
                                Log2(("%08x: DW_LNE_set_descriminator: %u\n", offOpCode, pLnState->Regs.uDiscriminator));
                            }
                            else
                            {
                                uint64_t uSeg = rtDwarfCursor_GetVarSizedU(pCursor, cbInstr - 1, UINT64_MAX);
                                Log2(("%08x: DW_LNE_set_segment: %#llx, cbInstr=%#x - Watcom Extension\n", offOpCode, uSeg, cbInstr));
                                pLnState->Regs.uSegment = (RTSEL)uSeg;
                                AssertStmt(pLnState->Regs.uSegment == uSeg, rc = VERR_DWARF_BAD_INFO);
                            }
                            break;

                        default:
                            Log(("rtDwarfLine_RunProgram: Unknown extended opcode %#x, length %#x at %08x\n", bOpCode, cbInstr, offOpCode));
                            break;
                    }

                    /* Advance the cursor to the end of the instruction . */
                    rtDwarfCursor_AdvanceToPos(pCursor, pbEndOfInstr);
                    break;
                }
            }
        }

        /*
         * Check the status before looping.
         */
        if (RT_FAILURE(rc))
            return rc;
        if (RT_FAILURE(pCursor->rc))
            return pCursor->rc;
    }
    return rc;
}


/**
 * Reads the include directories for a line number unit.
 *
 * @returns IPRT status code
 * @param   pLnState            The line number program state.
 * @param   pCursor             The cursor.
 */
static int rtDwarfLine_ReadFileNames(PRTDWARFLINESTATE pLnState, PRTDWARFCURSOR pCursor)
{
    int rc = rtDwarfLine_DefineFileName(pLnState, "/<bad-zero-file-name-entry>", 0);
    if (RT_FAILURE(rc))
        return rc;

    for (;;)
    {
        const char *psz = rtDwarfCursor_GetSZ(pCursor, NULL);
        if (!*psz)
            break;

        uint64_t idxInc = rtDwarfCursor_GetULeb128(pCursor, UINT64_MAX);
        rtDwarfCursor_SkipLeb128(pCursor); /* st_mtime */
        rtDwarfCursor_SkipLeb128(pCursor); /* st_size */

        rc = rtDwarfLine_DefineFileName(pLnState, psz, idxInc);
        if (RT_FAILURE(rc))
            return rc;
    }
    return pCursor->rc;
}


/**
 * Reads the include directories for a line number unit.
 *
 * @returns IPRT status code
 * @param   pLnState            The line number program state.
 * @param   pCursor             The cursor.
 */
static int rtDwarfLine_ReadIncludePaths(PRTDWARFLINESTATE pLnState, PRTDWARFCURSOR pCursor)
{
    const char *psz = "";   /* The zeroth is the unit dir. */
    for (;;)
    {
        if ((pLnState->cIncPaths % 2) == 0)
        {
            void *pv = RTMemRealloc(pLnState->papszIncPaths, sizeof(pLnState->papszIncPaths[0]) * (pLnState->cIncPaths + 2));
            if (!pv)
                return VERR_NO_MEMORY;
            pLnState->papszIncPaths = (const char **)pv;
        }
        Log(("  Path #%02u = '%s'\n", pLnState->cIncPaths, psz));
        pLnState->papszIncPaths[pLnState->cIncPaths] = psz;
        pLnState->cIncPaths++;

        psz = rtDwarfCursor_GetSZ(pCursor, NULL);
        if (!*psz)
            break;
    }

    return pCursor->rc;
}


/**
 * Explodes the line number table for a compilation unit.
 *
 * @returns IPRT status code
 * @param   pThis               The DWARF instance.
 * @param   pCursor             The cursor to read the line number information
 *                              via.
 */
static int rtDwarfLine_ExplodeUnit(PRTDBGMODDWARF pThis, PRTDWARFCURSOR pCursor)
{
    RTDWARFLINESTATE LnState;
    RT_ZERO(LnState);
    LnState.pDwarfMod = pThis;

    /*
     * Parse the header.
     */
    rtDwarfCursor_GetInitalLength(pCursor);
    LnState.Hdr.uVer           = rtDwarfCursor_GetUHalf(pCursor, 0);
    if (   LnState.Hdr.uVer < 2
        || LnState.Hdr.uVer > 4)
        return rtDwarfCursor_SkipUnit(pCursor);

    LnState.Hdr.offFirstOpcode = rtDwarfCursor_GetUOff(pCursor, 0);
    uint8_t const * const pbFirstOpcode = rtDwarfCursor_CalcPos(pCursor, LnState.Hdr.offFirstOpcode);

    LnState.Hdr.cbMinInstr     = rtDwarfCursor_GetUByte(pCursor, 0);
    if (LnState.Hdr.uVer >= 4)
        LnState.Hdr.cMaxOpsPerInstr = rtDwarfCursor_GetUByte(pCursor, 0);
    else
        LnState.Hdr.cMaxOpsPerInstr = 1;
    LnState.Hdr.u8DefIsStmt    = rtDwarfCursor_GetUByte(pCursor, 0);
    LnState.Hdr.s8LineBase     = rtDwarfCursor_GetSByte(pCursor, 0);
    LnState.Hdr.u8LineRange    = rtDwarfCursor_GetUByte(pCursor, 0);
    LnState.Hdr.u8OpcodeBase   = rtDwarfCursor_GetUByte(pCursor, 0);

    if (   !LnState.Hdr.u8OpcodeBase
        || !LnState.Hdr.cMaxOpsPerInstr
        || !LnState.Hdr.u8LineRange
        || LnState.Hdr.u8DefIsStmt > 1)
        return VERR_DWARF_BAD_LINE_NUMBER_HEADER;
    Log2(("DWARF Line number header:\n"
          "    uVer             %d\n"
          "    offFirstOpcode   %#llx\n"
          "    cbMinInstr       %u\n"
          "    cMaxOpsPerInstr  %u\n"
          "    u8DefIsStmt      %u\n"
          "    s8LineBase       %d\n"
          "    u8LineRange      %u\n"
          "    u8OpcodeBase     %u\n",
          LnState.Hdr.uVer,    LnState.Hdr.offFirstOpcode, LnState.Hdr.cbMinInstr,  LnState.Hdr.cMaxOpsPerInstr,
          LnState.Hdr.u8DefIsStmt, LnState.Hdr.s8LineBase, LnState.Hdr.u8LineRange, LnState.Hdr.u8OpcodeBase));

    LnState.Hdr.pacStdOperands = pCursor->pb;
    for (uint8_t iStdOpcode = 1; iStdOpcode < LnState.Hdr.u8OpcodeBase; iStdOpcode++)
        rtDwarfCursor_GetUByte(pCursor, 0);

    int rc = pCursor->rc;
    if (RT_SUCCESS(rc))
        rc = rtDwarfLine_ReadIncludePaths(&LnState, pCursor);
    if (RT_SUCCESS(rc))
        rc = rtDwarfLine_ReadFileNames(&LnState, pCursor);

    /*
     * Run the program....
     */
    if (RT_SUCCESS(rc))
        rc = rtDwarfCursor_AdvanceToPos(pCursor, pbFirstOpcode);
    if (RT_SUCCESS(rc))
        rc = rtDwarfLine_RunProgram(&LnState, pCursor);

    /*
     * Clean up.
     */
    size_t i = LnState.cFileNames;
    while (i-- > 0)
        RTStrFree(LnState.papszFileNames[i]);
    RTMemFree(LnState.papszFileNames);
    RTMemFree(LnState.papszIncPaths);

    Assert(rtDwarfCursor_IsAtEndOfUnit(pCursor) || RT_FAILURE(rc));
    return rc;
}


/**
 * Explodes the line number table.
 *
 * The line numbers are insered into the debug info container.
 *
 * @returns IPRT status code
 * @param   pThis               The DWARF instance.
 */
static int rtDwarfLine_ExplodeAll(PRTDBGMODDWARF pThis)
{
    if (!pThis->aSections[krtDbgModDwarfSect_line].fPresent)
        return VINF_SUCCESS;

    RTDWARFCURSOR Cursor;
    int rc = rtDwarfCursor_Init(&Cursor, pThis, krtDbgModDwarfSect_line);
    if (RT_FAILURE(rc))
        return rc;

    while (   !rtDwarfCursor_IsAtEnd(&Cursor)
           && RT_SUCCESS(rc))
        rc = rtDwarfLine_ExplodeUnit(pThis, &Cursor);

    return rtDwarfCursor_Delete(&Cursor, rc);
}


/*
 *
 * DWARF Abbreviations.
 * DWARF Abbreviations.
 * DWARF Abbreviations.
 *
 */

/**
 * Deals with a cache miss in rtDwarfAbbrev_Lookup.
 *
 * @returns Pointer to abbreviation cache entry (read only).  May be rendered
 *          invalid by subsequent calls to this function.
 * @param   pThis               The DWARF instance.
 * @param   uCode               The abbreviation code to lookup.
 */
static PCRTDWARFABBREV rtDwarfAbbrev_LookupMiss(PRTDBGMODDWARF pThis, uint32_t uCode)
{
    /*
     * There is no entry with code zero.
     */
    if (!uCode)
        return NULL;

    /*
     * Resize the cache array if the code is considered cachable.
     */
    bool fFillCache = true;
    if (pThis->cCachedAbbrevsAlloced < uCode)
    {
        if (uCode >= _64K)
            fFillCache = false;
        else
        {
            uint32_t cNew = RT_ALIGN(uCode, 64);
            void *pv = RTMemRealloc(pThis->paCachedAbbrevs, sizeof(pThis->paCachedAbbrevs[0]) * cNew);
            if (!pv)
                fFillCache = false;
            else
            {
                Log(("rtDwarfAbbrev_LookupMiss: Growing from %u to %u...\n", pThis->cCachedAbbrevsAlloced, cNew));
                pThis->paCachedAbbrevs       = (PRTDWARFABBREV)pv;
                for (uint32_t i = pThis->cCachedAbbrevsAlloced; i < cNew; i++)
                    pThis->paCachedAbbrevs[i].offAbbrev = UINT32_MAX;
                pThis->cCachedAbbrevsAlloced = cNew;
            }
        }
    }

    /*
     * Walk the abbreviations till we find the desired code.
     */
    RTDWARFCURSOR Cursor;
    int rc = rtDwarfCursor_InitWithOffset(&Cursor, pThis, krtDbgModDwarfSect_abbrev, pThis->offCachedAbbrev);
    if (RT_FAILURE(rc))
        return NULL;

    PRTDWARFABBREV pRet = NULL;
    if (fFillCache)
    {
        /*
         * Search for the entry and fill the cache while doing so.
         * We assume that abbreviation codes for a unit will stop when we see
         * zero code or when the code value drops.
         */
        uint32_t uPrevCode = 0;
        for (;;)
        {
            /* Read the 'header'. Skipping zero code bytes. */
            uint32_t const uCurCode = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
            if (pRet && (uCurCode == 0 || uCurCode < uPrevCode))
                break; /* probably end of unit. */
            if (uCurCode != 0)
            {
                uint32_t const uCurTag   = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
                uint8_t  const uChildren = rtDwarfCursor_GetU8(&Cursor, 0);
                if (RT_FAILURE(Cursor.rc))
                    break;
                if (   uCurTag > 0xffff
                    || uChildren > 1)
                {
                    Cursor.rc = VERR_DWARF_BAD_ABBREV;
                    break;
                }

                /* Cache it? */
                if (uCurCode <= pThis->cCachedAbbrevsAlloced)
                {
                    PRTDWARFABBREV pEntry = &pThis->paCachedAbbrevs[uCurCode - 1];
                    if (pEntry->offAbbrev != pThis->offCachedAbbrev)
                    {
                        pEntry->offAbbrev = pThis->offCachedAbbrev;
                        pEntry->fChildren = RT_BOOL(uChildren);
                        pEntry->uTag      = uCurTag;
                        pEntry->offSpec   = rtDwarfCursor_CalcSectOffsetU32(&Cursor);

                        if (uCurCode == uCode)
                        {
                            Assert(!pRet);
                            pRet = pEntry;
                            if (uCurCode == pThis->cCachedAbbrevsAlloced)
                                break;
                        }
                    }
                    else if (pRet)
                        break; /* Next unit, don't cache more. */
                    /* else: We're growing the cache and re-reading old data. */
                }

                /* Skip the specification. */
                uint32_t uAttr, uForm;
                do
                {
                    uAttr = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
                    uForm = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
                } while (uAttr != 0);
            }
            if (RT_FAILURE(Cursor.rc))
                break;

            /* Done? (Maximize cache filling.) */
            if (   pRet != NULL
                && uCurCode >= pThis->cCachedAbbrevsAlloced)
                break;
            uPrevCode = uCurCode;
        }
    }
    else
    {
        /*
         * Search for the entry with the desired code, no cache filling.
         */
        for (;;)
        {
            /* Read the 'header'. */
            uint32_t const uCurCode  = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
            uint32_t const uCurTag   = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
            uint8_t  const uChildren = rtDwarfCursor_GetU8(&Cursor, 0);
            if (RT_FAILURE(Cursor.rc))
                break;
            if (   uCurTag > 0xffff
                || uChildren > 1)
            {
                Cursor.rc = VERR_DWARF_BAD_ABBREV;
                break;
            }

            /* Do we have a match? */
            if (uCurCode == uCode)
            {
                pRet = &pThis->LookupAbbrev;
                pRet->fChildren = RT_BOOL(uChildren);
                pRet->uTag      = uCurTag;
                pRet->offSpec   = rtDwarfCursor_CalcSectOffsetU32(&Cursor);
                pRet->offAbbrev = pThis->offCachedAbbrev;
                break;
            }

            /* Skip the specification. */
            uint32_t uAttr, uForm;
            do
            {
                uAttr = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
                uForm = rtDwarfCursor_GetULeb128AsU32(&Cursor, 0);
            } while (uAttr != 0);
            if (RT_FAILURE(Cursor.rc))
                break;
        }
    }

    rtDwarfCursor_Delete(&Cursor, VINF_SUCCESS);
    return pRet;
}


/**
 * Looks up an abbreviation.
 *
 * @returns Pointer to abbreviation cache entry (read only).  May be rendered
 *          invalid by subsequent calls to this function.
 * @param   pThis               The DWARF instance.
 * @param   uCode               The abbreviation code to lookup.
 */
static PCRTDWARFABBREV rtDwarfAbbrev_Lookup(PRTDBGMODDWARF pThis, uint32_t uCode)
{
    if (   uCode - 1 >= pThis->cCachedAbbrevsAlloced
        || pThis->paCachedAbbrevs[uCode - 1].offAbbrev != pThis->offCachedAbbrev)
        return rtDwarfAbbrev_LookupMiss(pThis, uCode);
    return &pThis->paCachedAbbrevs[uCode - 1];
}


/**
 * Sets the abbreviation offset of the current unit.
 *
 * @param   pThis               The DWARF instance.
 * @param   offAbbrev           The offset into the abbreviation section.
 */
static void rtDwarfAbbrev_SetUnitOffset(PRTDBGMODDWARF pThis, uint32_t offAbbrev)
{
    pThis->offCachedAbbrev = offAbbrev;
}



/*
 *
 * DIE Attribute Parsers.
 * DIE Attribute Parsers.
 * DIE Attribute Parsers.
 *
 */

/**
 * Gets the compilation unit a DIE belongs to.
 *
 * @returns The compilation unit DIE.
 * @param   pDie                Some DIE in the unit.
 */
static PRTDWARFDIECOMPILEUNIT rtDwarfDie_GetCompileUnit(PRTDWARFDIE pDie)
{
    while (pDie->pParent)
        pDie = pDie->pParent;
    AssertReturn(   pDie->uTag == DW_TAG_compile_unit
                 || pDie->uTag == DW_TAG_partial_unit,
                 NULL);
    return (PRTDWARFDIECOMPILEUNIT)pDie;
}


/**
 * Resolves a string section (debug_str) reference.
 *
 * @returns Pointer to the string (inside the string section).
 * @param   pThis               The DWARF instance.
 * @param   pCursor             The cursor.
 * @param   pszErrValue         What to return on failure (@a
 *                              pCursor->rc is set).
 */
static const char *rtDwarfDecodeHlp_GetStrp(PRTDBGMODDWARF pThis, PRTDWARFCURSOR pCursor, const char *pszErrValue)
{
    uint64_t offDebugStr = rtDwarfCursor_GetUOff(pCursor, UINT64_MAX);
    if (RT_FAILURE(pCursor->rc))
        return pszErrValue;

    if (offDebugStr >= pThis->aSections[krtDbgModDwarfSect_str].cb)
    {
        /* Ugly: Exploit the cursor status field for reporting errors. */
        pCursor->rc = VERR_DWARF_BAD_INFO;
        return pszErrValue;
    }

    if (!pThis->aSections[krtDbgModDwarfSect_str].pv)
    {
        int rc = rtDbgModDwarfLoadSection(pThis, krtDbgModDwarfSect_str);
        if (RT_FAILURE(rc))
        {
            /* Ugly: Exploit the cursor status field for reporting errors. */
            pCursor->rc = rc;
            return pszErrValue;
        }
    }

    return (const char *)pThis->aSections[krtDbgModDwarfSect_str].pv + (size_t)offDebugStr;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_Address(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                               uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    AssertReturn(ATTR_GET_SIZE(pDesc) == sizeof(RTDWARFADDR), VERR_INTERNAL_ERROR_3);
    NOREF(pDie);

    uint64_t uAddr;
    switch (uForm)
    {
        case DW_FORM_addr:  uAddr = rtDwarfCursor_GetNativeUOff(pCursor, 0); break;
        case DW_FORM_data1: uAddr = rtDwarfCursor_GetU8(pCursor, 0); break;
        case DW_FORM_data2: uAddr = rtDwarfCursor_GetU16(pCursor, 0); break;
        case DW_FORM_data4: uAddr = rtDwarfCursor_GetU32(pCursor, 0); break;
        case DW_FORM_data8: uAddr = rtDwarfCursor_GetU64(pCursor, 0); break;
        case DW_FORM_udata: uAddr = rtDwarfCursor_GetULeb128(pCursor, 0); break;
        default:
            AssertMsgFailedReturn(("%#x (%s)\n", uForm, rtDwarfLog_FormName(uForm)), VERR_DWARF_UNEXPECTED_FORM);
    }
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;

    PRTDWARFADDR pAddr = (PRTDWARFADDR)pbMember;
    pAddr->uAddress = uAddr;

    Log4(("          %-20s  %#010llx  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr), uAddr, rtDwarfLog_FormName(uForm)));
    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_Bool(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                            uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    AssertReturn(ATTR_GET_SIZE(pDesc) == sizeof(bool), VERR_INTERNAL_ERROR_3);
    NOREF(pDie);

    bool *pfMember = (bool *)pbMember;
    switch (uForm)
    {
        case DW_FORM_flag:
        {
            uint8_t b = rtDwarfCursor_GetU8(pCursor, UINT8_MAX);
            if (b > 1)
            {
                Log(("Unexpected boolean value %#x\n", b));
                return RT_FAILURE(pCursor->rc) ? pCursor->rc : pCursor->rc = VERR_DWARF_BAD_INFO;
            }
            *pfMember = RT_BOOL(b);
            break;
        }

        case DW_FORM_flag_present:
            *pfMember = true;
            break;

        default:
            AssertMsgFailedReturn(("%#x\n", uForm), VERR_DWARF_UNEXPECTED_FORM);
    }

    Log4(("          %-20s  %RTbool  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr), *pfMember, rtDwarfLog_FormName(uForm)));
    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_LowHighPc(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                                 uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    AssertReturn(ATTR_GET_SIZE(pDesc) == sizeof(RTDWARFADDRRANGE), VERR_INTERNAL_ERROR_3);
    AssertReturn(pDesc->uAttr == DW_AT_low_pc || pDesc->uAttr == DW_AT_high_pc, VERR_INTERNAL_ERROR_3);
    NOREF(pDie);

    uint64_t uAddr;
    switch (uForm)
    {
        case DW_FORM_addr:  uAddr = rtDwarfCursor_GetNativeUOff(pCursor, 0); break;
        case DW_FORM_data1: uAddr = rtDwarfCursor_GetU8(pCursor, 0); break;
        case DW_FORM_data2: uAddr = rtDwarfCursor_GetU16(pCursor, 0); break;
        case DW_FORM_data4: uAddr = rtDwarfCursor_GetU32(pCursor, 0); break;
        case DW_FORM_data8: uAddr = rtDwarfCursor_GetU64(pCursor, 0); break;
        case DW_FORM_udata: uAddr = rtDwarfCursor_GetULeb128(pCursor, 0); break;
        default:
            AssertMsgFailedReturn(("%#x\n", uForm), VERR_DWARF_UNEXPECTED_FORM);
    }
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;

    PRTDWARFADDRRANGE pRange = (PRTDWARFADDRRANGE)pbMember;
    if (pDesc->uAttr == DW_AT_low_pc)
    {
        if (pRange->fHaveLowAddress)
        {
            Log(("rtDwarfDecode_LowHighPc: Duplicate DW_AT_low_pc\n"));
            return pCursor->rc = VERR_DWARF_BAD_INFO;
        }
        pRange->fHaveLowAddress  = true;
        pRange->uLowAddress      = uAddr;
    }
    else
    {
        if (pRange->fHaveHighAddress)
        {
            Log(("rtDwarfDecode_LowHighPc: Duplicate DW_AT_high_pc\n"));
            return pCursor->rc = VERR_DWARF_BAD_INFO;
        }
        pRange->fHaveHighAddress = true;
        pRange->fHaveHighIsAddress = uForm == DW_FORM_addr;
        if (!pRange->fHaveHighIsAddress && pRange->fHaveLowAddress)
        {
            pRange->fHaveHighIsAddress = true;
            pRange->uHighAddress     = uAddr + pRange->uLowAddress;
        }
        else
            pRange->uHighAddress     = uAddr;

    }
    pRange->cAttrs++;

    Log4(("          %-20s  %#010llx  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr), uAddr, rtDwarfLog_FormName(uForm)));
    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_Ranges(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                              uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    AssertReturn(ATTR_GET_SIZE(pDesc) == sizeof(RTDWARFADDRRANGE), VERR_INTERNAL_ERROR_3);
    AssertReturn(pDesc->uAttr == DW_AT_ranges, VERR_INTERNAL_ERROR_3);
    NOREF(pDie);

    /* Decode it. */
    uint64_t off;
    switch (uForm)
    {
        case DW_FORM_addr:          off = rtDwarfCursor_GetNativeUOff(pCursor, 0); break;
        case DW_FORM_data4:         off = rtDwarfCursor_GetU32(pCursor, 0); break;
        case DW_FORM_data8:         off = rtDwarfCursor_GetU64(pCursor, 0); break;
        case DW_FORM_sec_offset:    off = rtDwarfCursor_GetUOff(pCursor, 0); break;
        default:
            AssertMsgFailedReturn(("%#x\n", uForm), VERR_DWARF_UNEXPECTED_FORM);
    }
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;

    /* Validate the offset and load the ranges. */
    PRTDBGMODDWARF pThis = pCursor->pDwarfMod;
    if (off >= pThis->aSections[krtDbgModDwarfSect_ranges].cb)
    {
        Log(("rtDwarfDecode_Ranges: bad ranges off=%#llx\n", off));
        return pCursor->rc = VERR_DWARF_BAD_POS;
    }

    if (!pThis->aSections[krtDbgModDwarfSect_ranges].pv)
    {
        int rc = rtDbgModDwarfLoadSection(pThis, krtDbgModDwarfSect_ranges);
        if (RT_FAILURE(rc))
            return pCursor->rc = rc;
    }

    /* Store the result. */
    PRTDWARFADDRRANGE pRange = (PRTDWARFADDRRANGE)pbMember;
    if (pRange->fHaveRanges)
    {
        Log(("rtDwarfDecode_Ranges: Duplicate DW_AT_ranges\n"));
        return pCursor->rc = VERR_DWARF_BAD_INFO;
    }
    pRange->fHaveRanges = true;
    pRange->cAttrs++;
    pRange->pbRanges    = (uint8_t const *)pThis->aSections[krtDbgModDwarfSect_ranges].pv + (size_t)off;

    Log4(("          %-20s  TODO  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr), rtDwarfLog_FormName(uForm)));
    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_Reference(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                                 uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    AssertReturn(ATTR_GET_SIZE(pDesc) == sizeof(RTDWARFREF), VERR_INTERNAL_ERROR_3);

    /* Decode it. */
    uint64_t        off;
    krtDwarfRef     enmWrt = krtDwarfRef_SameUnit;
    switch (uForm)
    {
        case DW_FORM_ref1:          off = rtDwarfCursor_GetU8(pCursor, 0); break;
        case DW_FORM_ref2:          off = rtDwarfCursor_GetU16(pCursor, 0); break;
        case DW_FORM_ref4:          off = rtDwarfCursor_GetU32(pCursor, 0); break;
        case DW_FORM_ref8:          off = rtDwarfCursor_GetU64(pCursor, 0); break;
        case DW_FORM_ref_udata:     off = rtDwarfCursor_GetULeb128(pCursor, 0); break;

        case DW_FORM_ref_addr:
            enmWrt = krtDwarfRef_InfoSection;
            off = rtDwarfCursor_GetUOff(pCursor, 0);
            break;

        case DW_FORM_ref_sig8:
            enmWrt = krtDwarfRef_TypeId64;
            off = rtDwarfCursor_GetU64(pCursor, 0);
            break;

        default:
            AssertMsgFailedReturn(("%#x\n", uForm), VERR_DWARF_UNEXPECTED_FORM);
    }
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;

    /* Validate the offset and convert to debug_info relative offsets. */
    if (enmWrt == krtDwarfRef_InfoSection)
    {
        if (off >= pCursor->pDwarfMod->aSections[krtDbgModDwarfSect_info].cb)
        {
            Log(("rtDwarfDecode_Reference: bad info off=%#llx\n", off));
            return pCursor->rc = VERR_DWARF_BAD_POS;
        }
    }
    else if (enmWrt == krtDwarfRef_SameUnit)
    {
        PRTDWARFDIECOMPILEUNIT pUnit = rtDwarfDie_GetCompileUnit(pDie);
        if (off >= pUnit->cbUnit)
        {
            Log(("rtDwarfDecode_Reference: bad unit off=%#llx\n", off));
            return pCursor->rc = VERR_DWARF_BAD_POS;
        }
        off += pUnit->offUnit;
        enmWrt = krtDwarfRef_InfoSection;
    }
    /* else: not bother verifying/resolving the indirect type reference yet. */

    /* Store it */
    PRTDWARFREF pRef = (PRTDWARFREF)pbMember;
    pRef->enmWrt = enmWrt;
    pRef->off    = off;

    Log4(("          %-20s  %d:%#010llx  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr), enmWrt, off, rtDwarfLog_FormName(uForm)));
    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_SectOff(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                               uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    AssertReturn(ATTR_GET_SIZE(pDesc) == sizeof(RTDWARFREF), VERR_INTERNAL_ERROR_3);
    NOREF(pDie);

    uint64_t off;
    switch (uForm)
    {
        case DW_FORM_data4:      off = rtDwarfCursor_GetU32(pCursor, 0);  break;
        case DW_FORM_data8:      off = rtDwarfCursor_GetU64(pCursor, 0);  break;
        case DW_FORM_sec_offset: off = rtDwarfCursor_GetUOff(pCursor, 0); break;
        default:
            AssertMsgFailedReturn(("%#x (%s)\n", uForm, rtDwarfLog_FormName(uForm)), VERR_DWARF_UNEXPECTED_FORM);
    }
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;

    krtDbgModDwarfSect  enmSect;
    krtDwarfRef         enmWrt;
    switch (pDesc->uAttr)
    {
        case DW_AT_stmt_list:   enmSect = krtDbgModDwarfSect_line;    enmWrt = krtDwarfRef_LineSection;    break;
        case DW_AT_macro_info:  enmSect = krtDbgModDwarfSect_loc;     enmWrt = krtDwarfRef_LocSection;     break;
        case DW_AT_ranges:      enmSect = krtDbgModDwarfSect_ranges;  enmWrt = krtDwarfRef_RangesSection;  break;
        default:
            AssertMsgFailedReturn(("%u (%s)\n", pDesc->uAttr, rtDwarfLog_AttrName(pDesc->uAttr)), VERR_INTERNAL_ERROR_4);
    }
    size_t cbSect = pCursor->pDwarfMod->aSections[enmSect].cb;
    if (off >= cbSect)
    {
        /* Watcom generates offset past the end of the section, increasing the
           offset by one for each compile unit. So, just fudge it. */
        Log(("rtDwarfDecode_SectOff: bad off=%#llx, attr %#x (%s), enmSect=%d cb=%#llx; Assuming watcom/gcc.\n", off,
             pDesc->uAttr, rtDwarfLog_AttrName(pDesc->uAttr), enmSect, cbSect));
        off = cbSect;
    }

    PRTDWARFREF pRef = (PRTDWARFREF)pbMember;
    pRef->enmWrt = enmWrt;
    pRef->off    = off;

    Log4(("          %-20s  %d:%#010llx  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr), enmWrt, off, rtDwarfLog_FormName(uForm)));
    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_String(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                              uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    AssertReturn(ATTR_GET_SIZE(pDesc) == sizeof(const char *), VERR_INTERNAL_ERROR_3);
    NOREF(pDie);

    const char *psz;
    switch (uForm)
    {
        case DW_FORM_string:
            psz = rtDwarfCursor_GetSZ(pCursor, NULL);
            break;

        case DW_FORM_strp:
            psz = rtDwarfDecodeHlp_GetStrp(pCursor->pDwarfMod, pCursor, NULL);
            break;

        default:
            AssertMsgFailedReturn(("%#x\n", uForm), VERR_DWARF_UNEXPECTED_FORM);
    }

    *(const char **)pbMember = psz;
    Log4(("          %-20s  '%s'  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr), psz, rtDwarfLog_FormName(uForm)));
    return pCursor->rc;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_UnsignedInt(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                                   uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    NOREF(pDie);
    uint64_t u64Val;
    switch (uForm)
    {
        case DW_FORM_udata: u64Val = rtDwarfCursor_GetULeb128(pCursor, 0); break;
        case DW_FORM_data1: u64Val = rtDwarfCursor_GetU8(pCursor, 0); break;
        case DW_FORM_data2: u64Val = rtDwarfCursor_GetU16(pCursor, 0); break;
        case DW_FORM_data4: u64Val = rtDwarfCursor_GetU32(pCursor, 0); break;
        case DW_FORM_data8: u64Val = rtDwarfCursor_GetU64(pCursor, 0); break;
        default:
            AssertMsgFailedReturn(("%#x\n", uForm), VERR_DWARF_UNEXPECTED_FORM);
    }
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;

    switch (ATTR_GET_SIZE(pDesc))
    {
        case 1:
            *pbMember = (uint8_t)u64Val;
            if (*pbMember != u64Val)
            {
                AssertFailed();
                return VERR_OUT_OF_RANGE;
            }
            break;

        case 2:
            *(uint16_t *)pbMember = (uint16_t)u64Val;
            if (*(uint16_t *)pbMember != u64Val)
            {
                AssertFailed();
                return VERR_OUT_OF_RANGE;
            }
            break;

        case 4:
            *(uint32_t *)pbMember = (uint32_t)u64Val;
            if (*(uint32_t *)pbMember != u64Val)
            {
                AssertFailed();
                return VERR_OUT_OF_RANGE;
            }
            break;

        case 8:
            *(uint64_t *)pbMember = (uint64_t)u64Val;
            if (*(uint64_t *)pbMember != u64Val)
            {
                AssertFailed();
                return VERR_OUT_OF_RANGE;
            }
            break;

        default:
            AssertMsgFailedReturn(("%#x\n", ATTR_GET_SIZE(pDesc)), VERR_INTERNAL_ERROR_2);
    }
    return VINF_SUCCESS;
}


/**
 * Initialize location interpreter state from cursor & form.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if no location information (i.e. there is source but
 *          it resulted in no byte code).
 * @param   pLoc                The location state structure to initialize.
 * @param   pCursor             The cursor to read from.
 * @param   uForm               The attribute form.
 */
static int rtDwarfLoc_Init(PRTDWARFLOCST pLoc, PRTDWARFCURSOR pCursor, uint32_t uForm)
{
    uint32_t cbBlock;
    switch (uForm)
    {
        case DW_FORM_block1:
            cbBlock = rtDwarfCursor_GetU8(pCursor, 0);
            break;

        case DW_FORM_block2:
            cbBlock = rtDwarfCursor_GetU16(pCursor, 0);
            break;

        case DW_FORM_block4:
            cbBlock = rtDwarfCursor_GetU32(pCursor, 0);
            break;

        case DW_FORM_block:
            cbBlock = rtDwarfCursor_GetULeb128(pCursor, 0);
            break;

        default:
            AssertMsgFailedReturn(("uForm=%#x\n", uForm), VERR_DWARF_UNEXPECTED_FORM);
    }
    if (!cbBlock)
        return VERR_NOT_FOUND;

    int rc = rtDwarfCursor_InitForBlock(&pLoc->Cursor, pCursor, cbBlock);
    if (RT_FAILURE(rc))
        return rc;
    pLoc->iTop = -1;
    return VINF_SUCCESS;
}


/**
 * Pushes a value onto the stack.
 *
 * @returns VINF_SUCCESS or VERR_DWARF_STACK_OVERFLOW.
 * @param   pLoc                The state.
 * @param   uValue              The value to push.
 */
static int rtDwarfLoc_Push(PRTDWARFLOCST pLoc, uint64_t uValue)
{
    int iTop = pLoc->iTop + 1;
    AssertReturn((unsigned)iTop < RT_ELEMENTS(pLoc->auStack), VERR_DWARF_STACK_OVERFLOW);
    pLoc->auStack[iTop] = uValue;
    pLoc->iTop = iTop;
    return VINF_SUCCESS;
}


static int rtDwarfLoc_Evaluate(PRTDWARFLOCST pLoc, void *pvLater, void *pvUser)
{
    RT_NOREF_PV(pvLater); RT_NOREF_PV(pvUser);

    while (!rtDwarfCursor_IsAtEndOfUnit(&pLoc->Cursor))
    {
        /* Read the next opcode.*/
        uint8_t const bOpcode = rtDwarfCursor_GetU8(&pLoc->Cursor, 0);

        /* Get its operands. */
        uint64_t uOperand1 = 0;
        uint64_t uOperand2 = 0;
        switch (bOpcode)
        {
            case DW_OP_addr:
                uOperand1 = rtDwarfCursor_GetNativeUOff(&pLoc->Cursor, 0);
                break;
            case DW_OP_pick:
            case DW_OP_const1u:
            case DW_OP_deref_size:
            case DW_OP_xderef_size:
                uOperand1 = rtDwarfCursor_GetU8(&pLoc->Cursor, 0);
                break;
            case DW_OP_const1s:
                uOperand1 = (int8_t)rtDwarfCursor_GetU8(&pLoc->Cursor, 0);
                break;
            case DW_OP_const2u:
                uOperand1 = rtDwarfCursor_GetU16(&pLoc->Cursor, 0);
                break;
            case DW_OP_skip:
            case DW_OP_bra:
            case DW_OP_const2s:
                uOperand1 = (int16_t)rtDwarfCursor_GetU16(&pLoc->Cursor, 0);
                break;
            case DW_OP_const4u:
                uOperand1 = rtDwarfCursor_GetU32(&pLoc->Cursor, 0);
                break;
            case DW_OP_const4s:
                uOperand1 = (int32_t)rtDwarfCursor_GetU32(&pLoc->Cursor, 0);
                break;
            case DW_OP_const8u:
                uOperand1 = rtDwarfCursor_GetU64(&pLoc->Cursor, 0);
                break;
            case DW_OP_const8s:
                uOperand1 = rtDwarfCursor_GetU64(&pLoc->Cursor, 0);
                break;
            case DW_OP_regx:
            case DW_OP_piece:
            case DW_OP_plus_uconst:
            case DW_OP_constu:
                uOperand1 = rtDwarfCursor_GetULeb128(&pLoc->Cursor, 0);
                break;
            case DW_OP_consts:
            case DW_OP_fbreg:
            case DW_OP_breg0+0: case DW_OP_breg0+1: case DW_OP_breg0+2: case DW_OP_breg0+3:
            case DW_OP_breg0+4: case DW_OP_breg0+5: case DW_OP_breg0+6: case DW_OP_breg0+7:
            case DW_OP_breg0+8: case DW_OP_breg0+9: case DW_OP_breg0+10: case DW_OP_breg0+11:
            case DW_OP_breg0+12: case DW_OP_breg0+13: case DW_OP_breg0+14: case DW_OP_breg0+15:
            case DW_OP_breg0+16: case DW_OP_breg0+17: case DW_OP_breg0+18: case DW_OP_breg0+19:
            case DW_OP_breg0+20: case DW_OP_breg0+21: case DW_OP_breg0+22: case DW_OP_breg0+23:
            case DW_OP_breg0+24: case DW_OP_breg0+25: case DW_OP_breg0+26: case DW_OP_breg0+27:
            case DW_OP_breg0+28: case DW_OP_breg0+29: case DW_OP_breg0+30: case DW_OP_breg0+31:
                uOperand1 = rtDwarfCursor_GetSLeb128(&pLoc->Cursor, 0);
                break;
            case DW_OP_bregx:
                uOperand1 = rtDwarfCursor_GetULeb128(&pLoc->Cursor, 0);
                uOperand2 = rtDwarfCursor_GetSLeb128(&pLoc->Cursor, 0);
                break;
        }
        if (RT_FAILURE(pLoc->Cursor.rc))
            break;

        /* Interpret the opcode. */
        int rc;
        switch (bOpcode)
        {
            case DW_OP_const1u:
            case DW_OP_const1s:
            case DW_OP_const2u:
            case DW_OP_const2s:
            case DW_OP_const4u:
            case DW_OP_const4s:
            case DW_OP_const8u:
            case DW_OP_const8s:
            case DW_OP_constu:
            case DW_OP_consts:
            case DW_OP_addr:
                rc = rtDwarfLoc_Push(pLoc, uOperand1);
                break;
            case DW_OP_lit0 +  0: case DW_OP_lit0 +  1: case DW_OP_lit0 +  2: case DW_OP_lit0 +  3:
            case DW_OP_lit0 +  4: case DW_OP_lit0 +  5: case DW_OP_lit0 +  6: case DW_OP_lit0 +  7:
            case DW_OP_lit0 +  8: case DW_OP_lit0 +  9: case DW_OP_lit0 + 10: case DW_OP_lit0 + 11:
            case DW_OP_lit0 + 12: case DW_OP_lit0 + 13: case DW_OP_lit0 + 14: case DW_OP_lit0 + 15:
            case DW_OP_lit0 + 16: case DW_OP_lit0 + 17: case DW_OP_lit0 + 18: case DW_OP_lit0 + 19:
            case DW_OP_lit0 + 20: case DW_OP_lit0 + 21: case DW_OP_lit0 + 22: case DW_OP_lit0 + 23:
            case DW_OP_lit0 + 24: case DW_OP_lit0 + 25: case DW_OP_lit0 + 26: case DW_OP_lit0 + 27:
            case DW_OP_lit0 + 28: case DW_OP_lit0 + 29: case DW_OP_lit0 + 30: case DW_OP_lit0 + 31:
                rc = rtDwarfLoc_Push(pLoc, bOpcode - DW_OP_lit0);
                break;
            case DW_OP_nop:
                break;
            case DW_OP_dup:               /** @todo 0 operands. */
            case DW_OP_drop:              /** @todo 0 operands. */
            case DW_OP_over:              /** @todo 0 operands. */
            case DW_OP_pick:              /** @todo 1 operands, a 1-byte stack index. */
            case DW_OP_swap:              /** @todo 0 operands. */
            case DW_OP_rot:               /** @todo 0 operands. */
            case DW_OP_abs:               /** @todo 0 operands. */
            case DW_OP_and:               /** @todo 0 operands. */
            case DW_OP_div:               /** @todo 0 operands. */
            case DW_OP_minus:             /** @todo 0 operands. */
            case DW_OP_mod:               /** @todo 0 operands. */
            case DW_OP_mul:               /** @todo 0 operands. */
            case DW_OP_neg:               /** @todo 0 operands. */
            case DW_OP_not:               /** @todo 0 operands. */
            case DW_OP_or:                /** @todo 0 operands. */
            case DW_OP_plus:              /** @todo 0 operands. */
            case DW_OP_plus_uconst:       /** @todo 1 operands, a ULEB128 addend. */
            case DW_OP_shl:               /** @todo 0 operands. */
            case DW_OP_shr:               /** @todo 0 operands. */
            case DW_OP_shra:              /** @todo 0 operands. */
            case DW_OP_xor:               /** @todo 0 operands. */
            case DW_OP_skip:              /** @todo 1 signed 2-byte constant. */
            case DW_OP_bra:               /** @todo 1 signed 2-byte constant. */
            case DW_OP_eq:                /** @todo 0 operands. */
            case DW_OP_ge:                /** @todo 0 operands. */
            case DW_OP_gt:                /** @todo 0 operands. */
            case DW_OP_le:                /** @todo 0 operands. */
            case DW_OP_lt:                /** @todo 0 operands. */
            case DW_OP_ne:                /** @todo 0 operands. */
            case DW_OP_reg0 +  0: case DW_OP_reg0 +  1: case DW_OP_reg0 +  2: case DW_OP_reg0 +  3: /** @todo 0 operands - reg 0..31. */
            case DW_OP_reg0 +  4: case DW_OP_reg0 +  5: case DW_OP_reg0 +  6: case DW_OP_reg0 +  7:
            case DW_OP_reg0 +  8: case DW_OP_reg0 +  9: case DW_OP_reg0 + 10: case DW_OP_reg0 + 11:
            case DW_OP_reg0 + 12: case DW_OP_reg0 + 13: case DW_OP_reg0 + 14: case DW_OP_reg0 + 15:
            case DW_OP_reg0 + 16: case DW_OP_reg0 + 17: case DW_OP_reg0 + 18: case DW_OP_reg0 + 19:
            case DW_OP_reg0 + 20: case DW_OP_reg0 + 21: case DW_OP_reg0 + 22: case DW_OP_reg0 + 23:
            case DW_OP_reg0 + 24: case DW_OP_reg0 + 25: case DW_OP_reg0 + 26: case DW_OP_reg0 + 27:
            case DW_OP_reg0 + 28: case DW_OP_reg0 + 29: case DW_OP_reg0 + 30: case DW_OP_reg0 + 31:
            case DW_OP_breg0+  0: case DW_OP_breg0+  1: case DW_OP_breg0+  2: case DW_OP_breg0+  3: /** @todo 1 operand, a SLEB128 offset. */
            case DW_OP_breg0+  4: case DW_OP_breg0+  5: case DW_OP_breg0+  6: case DW_OP_breg0+  7:
            case DW_OP_breg0+  8: case DW_OP_breg0+  9: case DW_OP_breg0+ 10: case DW_OP_breg0+ 11:
            case DW_OP_breg0+ 12: case DW_OP_breg0+ 13: case DW_OP_breg0+ 14: case DW_OP_breg0+ 15:
            case DW_OP_breg0+ 16: case DW_OP_breg0+ 17: case DW_OP_breg0+ 18: case DW_OP_breg0+ 19:
            case DW_OP_breg0+ 20: case DW_OP_breg0+ 21: case DW_OP_breg0+ 22: case DW_OP_breg0+ 23:
            case DW_OP_breg0+ 24: case DW_OP_breg0+ 25: case DW_OP_breg0+ 26: case DW_OP_breg0+ 27:
            case DW_OP_breg0+ 28: case DW_OP_breg0+ 29: case DW_OP_breg0+ 30: case DW_OP_breg0+ 31:
            case DW_OP_piece:             /** @todo 1 operand, a ULEB128 size of piece addressed. */
            case DW_OP_regx:              /** @todo 1 operand, a ULEB128 register. */
            case DW_OP_fbreg:             /** @todo 1 operand, a SLEB128 offset. */
            case DW_OP_bregx:             /** @todo 2 operands, a ULEB128 register followed by a SLEB128 offset. */
            case DW_OP_deref:             /** @todo 0 operands. */
            case DW_OP_deref_size:        /** @todo 1 operand, a 1-byte size of data retrieved. */
            case DW_OP_xderef:            /** @todo 0 operands. */
            case DW_OP_xderef_size:        /** @todo 1 operand, a 1-byte size of data retrieved. */
                AssertMsgFailedReturn(("bOpcode=%#x\n", bOpcode), VERR_DWARF_TODO);
            default:
                AssertMsgFailedReturn(("bOpcode=%#x\n", bOpcode), VERR_DWARF_UNKNOWN_LOC_OPCODE);
        }
    }

    return pLoc->Cursor.rc;
}


/** @callback_method_impl{FNRTDWARFATTRDECODER} */
static DECLCALLBACK(int) rtDwarfDecode_SegmentLoc(PRTDWARFDIE pDie, uint8_t *pbMember, PCRTDWARFATTRDESC pDesc,
                                                  uint32_t uForm, PRTDWARFCURSOR pCursor)
{
    NOREF(pDie);
    AssertReturn(ATTR_GET_SIZE(pDesc) == 2, VERR_DWARF_IPE);

    RTDWARFLOCST LocSt;
    int rc = rtDwarfLoc_Init(&LocSt, pCursor, uForm);
    if (RT_SUCCESS(rc))
    {
        rc = rtDwarfLoc_Evaluate(&LocSt, NULL, NULL);
        if (RT_SUCCESS(rc))
        {
            if (LocSt.iTop >= 0)
            {
                *(uint16_t *)pbMember = LocSt.auStack[LocSt.iTop];
                Log4(("          %-20s  %#06llx  [%s]\n", rtDwarfLog_AttrName(pDesc->uAttr),
                      LocSt.auStack[LocSt.iTop],  rtDwarfLog_FormName(uForm)));
                return VINF_SUCCESS;
            }
            rc = VERR_DWARF_STACK_UNDERFLOW;
        }
    }
    return rc;
}

/*
 *
 * DWARF debug_info parser
 * DWARF debug_info parser
 * DWARF debug_info parser
 *
 */


/**
 * Special hack to get the name and/or linkage name for a subprogram via a
 * specification reference.
 *
 * Since this is a hack, we ignore failure.
 *
 * If we want to really make use of DWARF info, we'll have to create some kind
 * of lookup tree for handling this. But currently we don't, so a hack will
 * suffice.
 *
 * @param   pThis               The DWARF instance.
 * @param   pSubProgram         The subprogram which is short on names.
 */
static void rtDwarfInfo_TryGetSubProgramNameFromSpecRef(PRTDBGMODDWARF pThis, PRTDWARFDIESUBPROGRAM pSubProgram)
{
    /*
     * Must have a spec ref, and it must be in the info section.
     */
    if (pSubProgram->SpecRef.enmWrt != krtDwarfRef_InfoSection)
        return;

    /*
     * Create a cursor for reading the info and then the abbrivation code
     * starting the off the DIE.
     */
    RTDWARFCURSOR InfoCursor;
    int rc = rtDwarfCursor_InitWithOffset(&InfoCursor, pThis, krtDbgModDwarfSect_info, pSubProgram->SpecRef.off);
    if (RT_FAILURE(rc))
        return;

    uint32_t uAbbrCode = rtDwarfCursor_GetULeb128AsU32(&InfoCursor, UINT32_MAX);
    if (uAbbrCode)
    {
        /* Only references to subprogram tags are interesting here. */
        PCRTDWARFABBREV pAbbrev = rtDwarfAbbrev_Lookup(pThis, uAbbrCode);
        if (   pAbbrev
            && pAbbrev->uTag == DW_TAG_subprogram)
        {
            /*
             * Use rtDwarfInfo_ParseDie to do the parsing, but with a different
             * attribute spec than usual.
             */
            rtDwarfInfo_ParseDie(pThis, &pSubProgram->Core, &g_SubProgramSpecHackDesc, &InfoCursor,
                                 pAbbrev, false /*fInitDie*/);
        }
    }

    rtDwarfCursor_Delete(&InfoCursor, VINF_SUCCESS);
}


/**
 * Select which name to use.
 *
 * @returns One of the names.
 * @param   pszName             The DWARF name, may exclude namespace and class.
 *                              Can also be NULL.
 * @param   pszLinkageName      The linkage name. Can be NULL.
 */
static const char *rtDwarfInfo_SelectName(const char *pszName,  const char *pszLinkageName)
{
    if (!pszName || !pszLinkageName)
        return pszName ? pszName : pszLinkageName;

    /*
     * Some heuristics for selecting the link name if the normal name is missing
     * namespace or class prefixes.
     */
    size_t cchName = strlen(pszName);
    size_t cchLinkageName = strlen(pszLinkageName);
    if (cchLinkageName <= cchName + 1)
        return pszName;

    const char *psz = strstr(pszLinkageName, pszName);
    if (!psz || psz - pszLinkageName < 4)
        return pszName;

    return pszLinkageName;
}


/**
 * Parse the attributes of a DIE.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 * @param   pDie                The internal DIE structure to fill.
 */
static int rtDwarfInfo_SnoopSymbols(PRTDBGMODDWARF pThis, PRTDWARFDIE pDie)
{
    int rc = VINF_SUCCESS;
    switch (pDie->uTag)
    {
        case DW_TAG_subprogram:
        {
            PRTDWARFDIESUBPROGRAM pSubProgram = (PRTDWARFDIESUBPROGRAM)pDie;

            /* Obtain referenced specification there is only partial info. */
            if (   pSubProgram->PcRange.cAttrs
                && !pSubProgram->pszName)
                rtDwarfInfo_TryGetSubProgramNameFromSpecRef(pThis, pSubProgram);

            if (pSubProgram->PcRange.cAttrs)
            {
                if (pSubProgram->PcRange.fHaveRanges)
                    Log5(("subprogram %s (%s) <implement ranges>\n", pSubProgram->pszName, pSubProgram->pszLinkageName));
                else
                {
                    Log5(("subprogram %s (%s) %#llx-%#llx%s\n", pSubProgram->pszName, pSubProgram->pszLinkageName,
                          pSubProgram->PcRange.uLowAddress, pSubProgram->PcRange.uHighAddress,
                          pSubProgram->PcRange.cAttrs == 2 ? "" : " !bad!"));
                    if (   ( pSubProgram->pszName || pSubProgram->pszLinkageName)
                        && pSubProgram->PcRange.cAttrs == 2)
                    {
                        if (pThis->iWatcomPass == 1)
                            rc = rtDbgModDwarfRecordSegOffset(pThis, pSubProgram->uSegment, pSubProgram->PcRange.uHighAddress);
                        else
                        {
                            RTDBGSEGIDX iSeg;
                            RTUINTPTR   offSeg;
                            rc = rtDbgModDwarfLinkAddressToSegOffset(pThis, pSubProgram->uSegment,
                                                                     pSubProgram->PcRange.uLowAddress,
                                                                     &iSeg, &offSeg);
                            if (RT_SUCCESS(rc))
                            {
                                uint64_t cb;
                                if (pSubProgram->PcRange.uHighAddress >= pSubProgram->PcRange.uLowAddress)
                                    cb = pSubProgram->PcRange.uHighAddress - pSubProgram->PcRange.uLowAddress;
                               else
                                    cb = 1;
                                rc = RTDbgModSymbolAdd(pThis->hCnt,
                                                       rtDwarfInfo_SelectName(pSubProgram->pszName, pSubProgram->pszLinkageName),
                                                       iSeg, offSeg, cb, 0 /*fFlags*/, NULL /*piOrdinal*/);
                                if (RT_FAILURE(rc))
                                {
                                    if (   rc == VERR_DBG_DUPLICATE_SYMBOL
                                        || rc == VERR_DBG_ADDRESS_CONFLICT /** @todo figure why this happens with 10.6.8 mach_kernel, 32-bit. */
                                        )
                                        rc = VINF_SUCCESS;
                                    else
                                        AssertMsgFailed(("%Rrc\n", rc));
                                }
                            }
                            else if (   pSubProgram->PcRange.uLowAddress  == 0 /* see with vmlinux */
                                     && pSubProgram->PcRange.uHighAddress == 0)
                            {
                                Log5(("rtDbgModDwarfLinkAddressToSegOffset: Ignoring empty range.\n"));
                                rc = VINF_SUCCESS; /* ignore */
                            }
                            else
                            {
                                AssertRC(rc);
                                Log5(("rtDbgModDwarfLinkAddressToSegOffset failed: %Rrc\n", rc));
                            }
                        }
                    }
                }
            }
            else
                Log5(("subprogram %s (%s) external\n", pSubProgram->pszName, pSubProgram->pszLinkageName));
            break;
        }

        case DW_TAG_label:
        {
            PCRTDWARFDIELABEL pLabel = (PCRTDWARFDIELABEL)pDie;
            if (pLabel->fExternal)
            {
                Log5(("label %s %#x:%#llx\n", pLabel->pszName, pLabel->uSegment, pLabel->Address.uAddress));
                if (pThis->iWatcomPass == 1)
                    rc = rtDbgModDwarfRecordSegOffset(pThis, pLabel->uSegment, pLabel->Address.uAddress);
                else
                {
                    RTDBGSEGIDX iSeg;
                    RTUINTPTR   offSeg;
                    rc = rtDbgModDwarfLinkAddressToSegOffset(pThis, pLabel->uSegment, pLabel->Address.uAddress,
                                                             &iSeg, &offSeg);
                    AssertRC(rc);
                    if (RT_SUCCESS(rc))
                    {
                        rc = RTDbgModSymbolAdd(pThis->hCnt, pLabel->pszName, iSeg, offSeg, 0 /*cb*/,
                                               0 /*fFlags*/, NULL /*piOrdinal*/);
                        AssertRC(rc);
                    }
                    else
                        Log5(("rtDbgModDwarfLinkAddressToSegOffset failed: %Rrc\n", rc));
                }

            }
            break;
        }

    }
    return rc;
}


/**
 * Initializes the non-core fields of an internal  DIE structure.
 *
 * @param   pDie                The DIE structure.
 * @param   pDieDesc            The DIE descriptor.
 */
static void rtDwarfInfo_InitDie(PRTDWARFDIE pDie, PCRTDWARFDIEDESC pDieDesc)
{
    size_t i = pDieDesc->cAttributes;
    while (i-- > 0)
    {
        switch (pDieDesc->paAttributes[i].cbInit & ATTR_INIT_MASK)
        {
            case ATTR_INIT_ZERO:
                /* Nothing to do (RTMemAllocZ). */
                break;

            case ATTR_INIT_FFFS:
                switch (pDieDesc->paAttributes[i].cbInit & ATTR_SIZE_MASK)
                {
                    case 1:
                        *(uint8_t *)((uintptr_t)pDie + pDieDesc->paAttributes[i].off)  = UINT8_MAX;
                        break;
                    case 2:
                        *(uint16_t *)((uintptr_t)pDie + pDieDesc->paAttributes[i].off) = UINT16_MAX;
                        break;
                    case 4:
                        *(uint32_t *)((uintptr_t)pDie + pDieDesc->paAttributes[i].off) = UINT32_MAX;
                        break;
                    case 8:
                        *(uint64_t *)((uintptr_t)pDie + pDieDesc->paAttributes[i].off) = UINT64_MAX;
                        break;
                    default:
                        AssertFailed();
                        memset((uint8_t *)pDie + pDieDesc->paAttributes[i].off, 0xff,
                               pDieDesc->paAttributes[i].cbInit & ATTR_SIZE_MASK);
                        break;
                }
                break;

            default:
                AssertFailed();
        }
    }
}


/**
 * Creates a new internal DIE structure and links it up.
 *
 * @returns Pointer to the new DIE structure.
 * @param   pThis               The DWARF instance.
 * @param   pDieDesc            The DIE descriptor (for size and init).
 * @param   pAbbrev             The abbreviation cache entry.
 * @param   pParent             The parent DIE (NULL if unit).
 */
static PRTDWARFDIE rtDwarfInfo_NewDie(PRTDBGMODDWARF pThis, PCRTDWARFDIEDESC pDieDesc,
                                      PCRTDWARFABBREV pAbbrev, PRTDWARFDIE pParent)
{
    NOREF(pThis);
    Assert(pDieDesc->cbDie >= sizeof(RTDWARFDIE));
#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
    uint32_t iAllocator = pDieDesc->cbDie > pThis->aDieAllocators[0].cbMax;
    Assert(pDieDesc->cbDie <= pThis->aDieAllocators[iAllocator].cbMax);
    PRTDWARFDIE pDie = (PRTDWARFDIE)RTMemCacheAlloc(pThis->aDieAllocators[iAllocator].hMemCache);
#else
    PRTDWARFDIE pDie = (PRTDWARFDIE)RTMemAllocZ(pDieDesc->cbDie);
#endif
    if (pDie)
    {
#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
        RT_BZERO(pDie, pDieDesc->cbDie);
        pDie->iAllocator   = iAllocator;
#endif
        rtDwarfInfo_InitDie(pDie, pDieDesc);

        pDie->uTag         = pAbbrev->uTag;
        pDie->offSpec      = pAbbrev->offSpec;
        pDie->pParent      = pParent;
        if (pParent)
            RTListAppend(&pParent->ChildList, &pDie->SiblingNode);
        else
            RTListInit(&pDie->SiblingNode);
        RTListInit(&pDie->ChildList);

    }
    return pDie;
}


/**
 * Free all children of a DIE.
 *
 * @param   pThis               The DWARF instance.
 * @param   pParentDie          The parent DIE.
 */
static void rtDwarfInfo_FreeChildren(PRTDBGMODDWARF pThis, PRTDWARFDIE pParentDie)
{
    PRTDWARFDIE pChild, pNextChild;
    RTListForEachSafe(&pParentDie->ChildList, pChild, pNextChild, RTDWARFDIE, SiblingNode)
    {
        if (!RTListIsEmpty(&pChild->ChildList))
            rtDwarfInfo_FreeChildren(pThis, pChild);
        RTListNodeRemove(&pChild->SiblingNode);
#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
        RTMemCacheFree(pThis->aDieAllocators[pChild->iAllocator].hMemCache, pChild);
#else
        RTMemFree(pChild);
#endif
    }
}


/**
 * Free a DIE an all its children.
 *
 * @param   pThis               The DWARF instance.
 * @param   pDie                The DIE to free.
 */
static void rtDwarfInfo_FreeDie(PRTDBGMODDWARF pThis, PRTDWARFDIE pDie)
{
    rtDwarfInfo_FreeChildren(pThis, pDie);
    RTListNodeRemove(&pDie->SiblingNode);
#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
    RTMemCacheFree(pThis->aDieAllocators[pDie->iAllocator].hMemCache, pDie);
#else
    RTMemFree(pChild);
#endif
}


/**
 * Skips a form.
 * @returns IPRT status code
 * @param   pCursor             The cursor.
 * @param   uForm               The form to skip.
 */
static int rtDwarfInfo_SkipForm(PRTDWARFCURSOR pCursor, uint32_t uForm)
{
    switch (uForm)
    {
        case DW_FORM_addr:
            return rtDwarfCursor_SkipBytes(pCursor, pCursor->cbNativeAddr);

        case DW_FORM_block:
        case DW_FORM_exprloc:
            return rtDwarfCursor_SkipBytes(pCursor, rtDwarfCursor_GetULeb128(pCursor, 0));

        case DW_FORM_block1:
            return rtDwarfCursor_SkipBytes(pCursor, rtDwarfCursor_GetU8(pCursor, 0));

        case DW_FORM_block2:
            return rtDwarfCursor_SkipBytes(pCursor, rtDwarfCursor_GetU16(pCursor, 0));

        case DW_FORM_block4:
            return rtDwarfCursor_SkipBytes(pCursor, rtDwarfCursor_GetU32(pCursor, 0));

        case DW_FORM_data1:
        case DW_FORM_ref1:
        case DW_FORM_flag:
            return rtDwarfCursor_SkipBytes(pCursor, 1);

        case DW_FORM_data2:
        case DW_FORM_ref2:
            return rtDwarfCursor_SkipBytes(pCursor, 2);

        case DW_FORM_data4:
        case DW_FORM_ref4:
            return rtDwarfCursor_SkipBytes(pCursor, 4);

        case DW_FORM_data8:
        case DW_FORM_ref8:
        case DW_FORM_ref_sig8:
            return rtDwarfCursor_SkipBytes(pCursor, 8);

        case DW_FORM_udata:
        case DW_FORM_sdata:
        case DW_FORM_ref_udata:
            return rtDwarfCursor_SkipLeb128(pCursor);

        case DW_FORM_string:
            rtDwarfCursor_GetSZ(pCursor, NULL);
            return pCursor->rc;

        case DW_FORM_indirect:
            return rtDwarfInfo_SkipForm(pCursor, rtDwarfCursor_GetULeb128AsU32(pCursor, UINT32_MAX));

        case DW_FORM_strp:
        case DW_FORM_ref_addr:
        case DW_FORM_sec_offset:
            return rtDwarfCursor_SkipBytes(pCursor, pCursor->f64bitDwarf ? 8 : 4);

        case DW_FORM_flag_present:
            return pCursor->rc; /* no data */

        default:
            return VERR_DWARF_UNKNOWN_FORM;
    }
}



#ifdef SOME_UNUSED_FUNCTION
/**
 * Skips a DIE.
 *
 * @returns IPRT status code.
 * @param   pCursor         The cursor.
 * @param   pAbbrevCursor   The abbreviation cursor.
 */
static int rtDwarfInfo_SkipDie(PRTDWARFCURSOR pCursor, PRTDWARFCURSOR pAbbrevCursor)
{
    for (;;)
    {
        uint32_t uAttr = rtDwarfCursor_GetULeb128AsU32(pAbbrevCursor, 0);
        uint32_t uForm = rtDwarfCursor_GetULeb128AsU32(pAbbrevCursor, 0);
        if (uAttr == 0 && uForm == 0)
            break;

        int rc = rtDwarfInfo_SkipForm(pCursor, uForm);
        if (RT_FAILURE(rc))
            return rc;
    }
    return RT_FAILURE(pCursor->rc) ? pCursor->rc : pAbbrevCursor->rc;
}
#endif


/**
 * Parse the attributes of a DIE.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 * @param   pDie                The internal DIE structure to fill.
 * @param   pDieDesc            The DIE descriptor.
 * @param   pCursor             The debug_info cursor.
 * @param   pAbbrev             The abbreviation cache entry.
 * @param   fInitDie            Whether to initialize the DIE first.  If not (@c
 *                              false) it's safe to assume we're following a
 *                              DW_AT_specification or DW_AT_abstract_origin,
 *                              and that we shouldn't be snooping any symbols.
 */
static int rtDwarfInfo_ParseDie(PRTDBGMODDWARF pThis, PRTDWARFDIE pDie, PCRTDWARFDIEDESC pDieDesc,
                                PRTDWARFCURSOR pCursor, PCRTDWARFABBREV pAbbrev, bool fInitDie)
{
    RTDWARFCURSOR AbbrevCursor;
    int rc = rtDwarfCursor_InitWithOffset(&AbbrevCursor, pThis, krtDbgModDwarfSect_abbrev, pAbbrev->offSpec);
    if (RT_FAILURE(rc))
        return rc;

    if (fInitDie)
        rtDwarfInfo_InitDie(pDie, pDieDesc);
    for (;;)
    {
        uint32_t uAttr = rtDwarfCursor_GetULeb128AsU32(&AbbrevCursor, 0);
        uint32_t uForm = rtDwarfCursor_GetULeb128AsU32(&AbbrevCursor, 0);
        if (uAttr == 0)
            break;
        if (uForm == DW_FORM_indirect)
            uForm = rtDwarfCursor_GetULeb128AsU32(pCursor, 0);

        /* Look up the attribute in the descriptor and invoke the decoder. */
        PCRTDWARFATTRDESC pAttr = NULL;
        size_t i = pDieDesc->cAttributes;
        while (i-- > 0)
            if (pDieDesc->paAttributes[i].uAttr == uAttr)
            {
                pAttr = &pDieDesc->paAttributes[i];
                rc = pAttr->pfnDecoder(pDie, (uint8_t *)pDie + pAttr->off, pAttr, uForm, pCursor);
                break;
            }

        /* Some house keeping. */
        if (pAttr)
            pDie->cDecodedAttrs++;
        else
        {
            pDie->cUnhandledAttrs++;
            rc = rtDwarfInfo_SkipForm(pCursor, uForm);
            Log4(("          %-20s    [%s]\n", rtDwarfLog_AttrName(uAttr), rtDwarfLog_FormName(uForm)));
        }
        if (RT_FAILURE(rc))
            break;
    }

    rc = rtDwarfCursor_Delete(&AbbrevCursor, rc);
    if (RT_SUCCESS(rc))
        rc = pCursor->rc;

    /*
     * Snoop up symbols on the way out.
     */
    if (RT_SUCCESS(rc) && fInitDie)
    {
        rc = rtDwarfInfo_SnoopSymbols(pThis, pDie);
        /* Ignore duplicates, get work done instead. */
        /** @todo clean up global/static symbol mess. */
        if (rc == VERR_DBG_DUPLICATE_SYMBOL)
            rc = VINF_SUCCESS;
    }

    return rc;
}


/**
 * Load the debug information of a unit.
 *
 * @returns IPRT status code.
 * @param   pThis               The DWARF instance.
 * @param   pCursor             The debug_info cursor.
 * @param   fKeepDies           Whether to keep the DIEs or discard them as soon
 *                              as possible.
 */
static int rtDwarfInfo_LoadUnit(PRTDBGMODDWARF pThis, PRTDWARFCURSOR pCursor, bool fKeepDies)
{
    Log(("rtDwarfInfo_LoadUnit: %#x\n", rtDwarfCursor_CalcSectOffsetU32(pCursor)));

    /*
     * Read the compilation unit header.
     */
    uint64_t offUnit = rtDwarfCursor_CalcSectOffsetU32(pCursor);
    uint64_t cbUnit  = rtDwarfCursor_GetInitalLength(pCursor);
    cbUnit += rtDwarfCursor_CalcSectOffsetU32(pCursor) - offUnit;
    uint16_t const uVer = rtDwarfCursor_GetUHalf(pCursor, 0);
    if (   uVer < 2
        || uVer > 4)
        return rtDwarfCursor_SkipUnit(pCursor);
    uint64_t const offAbbrev    = rtDwarfCursor_GetUOff(pCursor, UINT64_MAX);
    uint8_t  const cbNativeAddr = rtDwarfCursor_GetU8(pCursor, UINT8_MAX);
    if (RT_FAILURE(pCursor->rc))
        return pCursor->rc;
    Log(("   uVer=%d  offAbbrev=%#llx cbNativeAddr=%d\n", uVer, offAbbrev, cbNativeAddr));

    /*
     * Set up the abbreviation cache and store the native address size in the cursor.
     */
    if (offAbbrev > UINT32_MAX)
    {
        Log(("Unexpected abbrviation code offset of %#llx\n", offAbbrev));
        return VERR_DWARF_BAD_INFO;
    }
    rtDwarfAbbrev_SetUnitOffset(pThis, (uint32_t)offAbbrev);
    pCursor->cbNativeAddr = cbNativeAddr;

    /*
     * The first DIE is a compile or partial unit, parse it here.
     */
    uint32_t uAbbrCode = rtDwarfCursor_GetULeb128AsU32(pCursor, UINT32_MAX);
    if (!uAbbrCode)
    {
        Log(("Unexpected abbrviation code of zero\n"));
        return VERR_DWARF_BAD_INFO;
    }
    PCRTDWARFABBREV pAbbrev = rtDwarfAbbrev_Lookup(pThis, uAbbrCode);
    if (!pAbbrev)
        return VERR_DWARF_ABBREV_NOT_FOUND;
    if (   pAbbrev->uTag != DW_TAG_compile_unit
        && pAbbrev->uTag != DW_TAG_partial_unit)
    {
        Log(("Unexpected compile/partial unit tag %#x\n", pAbbrev->uTag));
        return VERR_DWARF_BAD_INFO;
    }

    PRTDWARFDIECOMPILEUNIT pUnit;
    pUnit = (PRTDWARFDIECOMPILEUNIT)rtDwarfInfo_NewDie(pThis, &g_CompileUnitDesc, pAbbrev, NULL /*pParent*/);
    if (!pUnit)
        return VERR_NO_MEMORY;
    pUnit->offUnit      = offUnit;
    pUnit->cbUnit       = cbUnit;
    pUnit->offAbbrev    = offAbbrev;
    pUnit->cbNativeAddr = cbNativeAddr;
    pUnit->uDwarfVer    = (uint8_t)uVer;
    RTListAppend(&pThis->CompileUnitList, &pUnit->Core.SiblingNode);

    int rc = rtDwarfInfo_ParseDie(pThis, &pUnit->Core, &g_CompileUnitDesc, pCursor, pAbbrev, true /*fInitDie*/);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Parse DIEs.
     */
    uint32_t    cDepth     = 0;
    PRTDWARFDIE pParentDie = &pUnit->Core;
    while (!rtDwarfCursor_IsAtEndOfUnit(pCursor))
    {
#ifdef LOG_ENABLED
        uint32_t offLog = rtDwarfCursor_CalcSectOffsetU32(pCursor);
#endif
        uAbbrCode = rtDwarfCursor_GetULeb128AsU32(pCursor, UINT32_MAX);
        if (!uAbbrCode)
        {
            /* End of siblings, up one level. (Is this correct?) */
            if (pParentDie->pParent)
            {
                pParentDie = pParentDie->pParent;
                cDepth--;
                if (!fKeepDies && pParentDie->pParent)
                    rtDwarfInfo_FreeChildren(pThis, pParentDie);
            }
        }
        else
        {
            /*
             * Look up the abbreviation and match the tag up with a descriptor.
             */
            pAbbrev = rtDwarfAbbrev_Lookup(pThis, uAbbrCode);
            if (!pAbbrev)
                return VERR_DWARF_ABBREV_NOT_FOUND;

            PCRTDWARFDIEDESC pDieDesc;
            const char      *pszName;
            if (pAbbrev->uTag < RT_ELEMENTS(g_aTagDescs))
            {
                Assert(g_aTagDescs[pAbbrev->uTag].uTag == pAbbrev->uTag || g_aTagDescs[pAbbrev->uTag].uTag == 0);
                pszName  = g_aTagDescs[pAbbrev->uTag].pszName;
                pDieDesc = g_aTagDescs[pAbbrev->uTag].pDesc;
            }
            else
            {
                pszName  = "<unknown>";
                pDieDesc = &g_CoreDieDesc;
            }
            Log4(("%08x: %*stag=%s (%#x, abbrev %u)%s\n", offLog, cDepth * 2, "", pszName,
                  pAbbrev->uTag, uAbbrCode, pAbbrev->fChildren ? " has children" : ""));

            /*
             * Create a new internal DIE structure and parse the
             * attributes.
             */
            PRTDWARFDIE pNewDie = rtDwarfInfo_NewDie(pThis, pDieDesc, pAbbrev, pParentDie);
            if (!pNewDie)
                return VERR_NO_MEMORY;

            if (pAbbrev->fChildren)
            {
                pParentDie = pNewDie;
                cDepth++;
            }

            rc = rtDwarfInfo_ParseDie(pThis, pNewDie, pDieDesc, pCursor, pAbbrev, true /*fInitDie*/);
            if (RT_FAILURE(rc))
                return rc;

            if (!fKeepDies && !pAbbrev->fChildren)
                rtDwarfInfo_FreeDie(pThis, pNewDie);
        }
    } /* while more DIEs */


    /* Unlink and free child DIEs if told to do so. */
    if (!fKeepDies)
        rtDwarfInfo_FreeChildren(pThis, &pUnit->Core);

    return RT_SUCCESS(rc) ? pCursor->rc : rc;
}


/**
 * Extracts the symbols.
 *
 * The symbols are insered into the debug info container.
 *
 * @returns IPRT status code
 * @param   pThis               The DWARF instance.
 */
static int rtDwarfInfo_LoadAll(PRTDBGMODDWARF pThis)
{
    RTDWARFCURSOR Cursor;
    int rc = rtDwarfCursor_Init(&Cursor, pThis, krtDbgModDwarfSect_info);
    if (RT_SUCCESS(rc))
    {
        while (   !rtDwarfCursor_IsAtEnd(&Cursor)
               && RT_SUCCESS(rc))
            rc = rtDwarfInfo_LoadUnit(pThis, &Cursor, false /* fKeepDies */);

        rc = rtDwarfCursor_Delete(&Cursor, rc);
    }
    return rc;
}



/*
 *
 * Public and image level symbol handling.
 * Public and image level symbol handling.
 * Public and image level symbol handling.
 * Public and image level symbol handling.
 *
 *
 */

#define RTDBGDWARF_SYM_ENUM_BASE_ADDRESS  UINT32_C(0x200000)

/** @callback_method_impl{FNRTLDRENUMSYMS,
 *  Adds missing symbols from the image symbol table.} */
static DECLCALLBACK(int) rtDwarfSyms_EnumSymbolsCallback(RTLDRMOD hLdrMod, const char *pszSymbol, unsigned uSymbol,
                                                         RTLDRADDR Value, void *pvUser)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pvUser;
    RT_NOREF_PV(hLdrMod); RT_NOREF_PV(uSymbol);
    Assert(pThis->iWatcomPass != 1);

    RTLDRADDR uRva = Value - RTDBGDWARF_SYM_ENUM_BASE_ADDRESS;
    if (   Value >= RTDBGDWARF_SYM_ENUM_BASE_ADDRESS
        && uRva  <  _1G)
    {
        RTDBGSYMBOL SymInfo;
        RTINTPTR    offDisp;
        int rc = RTDbgModSymbolByAddr(pThis->hCnt, RTDBGSEGIDX_RVA, uRva, RTDBGSYMADDR_FLAGS_LESS_OR_EQUAL, &offDisp, &SymInfo);
        if (   RT_FAILURE(rc)
            || offDisp != 0)
        {
            rc = RTDbgModSymbolAdd(pThis->hCnt, pszSymbol, RTDBGSEGIDX_RVA, uRva, 1, 0 /*fFlags*/, NULL /*piOrdinal*/);
            Log(("Dwarf: Symbol #%05u %#018RTptr %s [%Rrc]\n", uSymbol, Value, pszSymbol, rc)); NOREF(rc);
        }
    }
    else
        Log(("Dwarf: Symbol #%05u %#018RTptr '%s' [SKIPPED - INVALID ADDRESS]\n", uSymbol, Value, pszSymbol));
    return VINF_SUCCESS;
}



/**
 * Loads additional symbols from the pubnames section and the executable image.
 *
 * The symbols are insered into the debug info container.
 *
 * @returns IPRT status code
 * @param   pThis               The DWARF instance.
 */
static int rtDwarfSyms_LoadAll(PRTDBGMODDWARF pThis)
{
    /*
     * pubnames.
     */
    int rc = VINF_SUCCESS;
    if (pThis->aSections[krtDbgModDwarfSect_pubnames].fPresent)
    {
//        RTDWARFCURSOR Cursor;
//        int rc = rtDwarfCursor_Init(&Cursor, pThis, krtDbgModDwarfSect_info);
//        if (RT_SUCCESS(rc))
//        {
//            while (   !rtDwarfCursor_IsAtEnd(&Cursor)
//                   && RT_SUCCESS(rc))
//                rc = rtDwarfInfo_LoadUnit(pThis, &Cursor, false /* fKeepDies */);
//
//            rc = rtDwarfCursor_Delete(&Cursor, rc);
//        }
//        return rc;
    }

    /*
     * The executable image.
     */
    if (   pThis->pImgMod
        && pThis->pImgMod->pImgVt->pfnEnumSymbols
        && pThis->iWatcomPass != 1
        && RT_SUCCESS(rc))
    {
        rc = pThis->pImgMod->pImgVt->pfnEnumSymbols(pThis->pImgMod,
                                                    RTLDR_ENUM_SYMBOL_FLAGS_ALL | RTLDR_ENUM_SYMBOL_FLAGS_NO_FWD,
                                                    RTDBGDWARF_SYM_ENUM_BASE_ADDRESS,
                                                    rtDwarfSyms_EnumSymbolsCallback,
                                                    pThis);
    }

    return rc;
}




/*
 *
 * DWARF Debug module implementation.
 * DWARF Debug module implementation.
 * DWARF Debug module implementation.
 *
 */


/** @interface_method_impl{RTDBGMODVTDBG,pfnLineByAddr} */
static DECLCALLBACK(int) rtDbgModDwarf_LineByAddr(PRTDBGMODINT pMod, RTDBGSEGIDX iSeg, RTUINTPTR off,
                                                  PRTINTPTR poffDisp, PRTDBGLINE pLineInfo)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModLineByAddr(pThis->hCnt, iSeg, off, poffDisp, pLineInfo);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnLineByOrdinal} */
static DECLCALLBACK(int) rtDbgModDwarf_LineByOrdinal(PRTDBGMODINT pMod, uint32_t iOrdinal, PRTDBGLINE pLineInfo)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModLineByOrdinal(pThis->hCnt, iOrdinal, pLineInfo);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnLineCount} */
static DECLCALLBACK(uint32_t) rtDbgModDwarf_LineCount(PRTDBGMODINT pMod)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModLineCount(pThis->hCnt);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnLineAdd} */
static DECLCALLBACK(int) rtDbgModDwarf_LineAdd(PRTDBGMODINT pMod, const char *pszFile, size_t cchFile, uint32_t uLineNo,
                                               uint32_t iSeg, RTUINTPTR off, uint32_t *piOrdinal)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    Assert(!pszFile[cchFile]); NOREF(cchFile);
    return RTDbgModLineAdd(pThis->hCnt, pszFile, uLineNo, iSeg, off, piOrdinal);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSymbolByAddr} */
static DECLCALLBACK(int) rtDbgModDwarf_SymbolByAddr(PRTDBGMODINT pMod, RTDBGSEGIDX iSeg, RTUINTPTR off, uint32_t fFlags,
                                                    PRTINTPTR poffDisp, PRTDBGSYMBOL pSymInfo)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModSymbolByAddr(pThis->hCnt, iSeg, off, fFlags, poffDisp, pSymInfo);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSymbolByName} */
static DECLCALLBACK(int) rtDbgModDwarf_SymbolByName(PRTDBGMODINT pMod, const char *pszSymbol, size_t cchSymbol,
                                                    PRTDBGSYMBOL pSymInfo)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    Assert(!pszSymbol[cchSymbol]); RT_NOREF_PV(cchSymbol);
    return RTDbgModSymbolByName(pThis->hCnt, pszSymbol/*, cchSymbol*/, pSymInfo);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSymbolByOrdinal} */
static DECLCALLBACK(int) rtDbgModDwarf_SymbolByOrdinal(PRTDBGMODINT pMod, uint32_t iOrdinal, PRTDBGSYMBOL pSymInfo)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModSymbolByOrdinal(pThis->hCnt, iOrdinal, pSymInfo);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSymbolCount} */
static DECLCALLBACK(uint32_t) rtDbgModDwarf_SymbolCount(PRTDBGMODINT pMod)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModSymbolCount(pThis->hCnt);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSymbolAdd} */
static DECLCALLBACK(int) rtDbgModDwarf_SymbolAdd(PRTDBGMODINT pMod, const char *pszSymbol, size_t cchSymbol,
                                                 RTDBGSEGIDX iSeg, RTUINTPTR off, RTUINTPTR cb, uint32_t fFlags,
                                                 uint32_t *piOrdinal)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    Assert(!pszSymbol[cchSymbol]); NOREF(cchSymbol);
    return RTDbgModSymbolAdd(pThis->hCnt, pszSymbol, iSeg, off, cb, fFlags, piOrdinal);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSegmentByIndex} */
static DECLCALLBACK(int) rtDbgModDwarf_SegmentByIndex(PRTDBGMODINT pMod, RTDBGSEGIDX iSeg, PRTDBGSEGMENT pSegInfo)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModSegmentByIndex(pThis->hCnt, iSeg, pSegInfo);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSegmentCount} */
static DECLCALLBACK(RTDBGSEGIDX) rtDbgModDwarf_SegmentCount(PRTDBGMODINT pMod)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModSegmentCount(pThis->hCnt);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnSegmentAdd} */
static DECLCALLBACK(int) rtDbgModDwarf_SegmentAdd(PRTDBGMODINT pMod, RTUINTPTR uRva, RTUINTPTR cb, const char *pszName, size_t cchName,
                                                  uint32_t fFlags, PRTDBGSEGIDX piSeg)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    Assert(!pszName[cchName]); NOREF(cchName);
    return RTDbgModSegmentAdd(pThis->hCnt, uRva, cb, pszName, fFlags, piSeg);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnImageSize} */
static DECLCALLBACK(RTUINTPTR) rtDbgModDwarf_ImageSize(PRTDBGMODINT pMod)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    RTUINTPTR cb1 = RTDbgModImageSize(pThis->hCnt);
    RTUINTPTR cb2 = pThis->pImgMod->pImgVt->pfnImageSize(pMod);
    return RT_MAX(cb1, cb2);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnRvaToSegOff} */
static DECLCALLBACK(RTDBGSEGIDX) rtDbgModDwarf_RvaToSegOff(PRTDBGMODINT pMod, RTUINTPTR uRva, PRTUINTPTR poffSeg)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;
    return RTDbgModRvaToSegOff(pThis->hCnt, uRva, poffSeg);
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnClose} */
static DECLCALLBACK(int) rtDbgModDwarf_Close(PRTDBGMODINT pMod)
{
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pMod->pvDbgPriv;

    for (unsigned iSect = 0; iSect < RT_ELEMENTS(pThis->aSections); iSect++)
        if (pThis->aSections[iSect].pv)
            pThis->pDbgInfoMod->pImgVt->pfnUnmapPart(pThis->pDbgInfoMod, pThis->aSections[iSect].cb, &pThis->aSections[iSect].pv);

    RTDbgModRelease(pThis->hCnt);
    RTMemFree(pThis->paCachedAbbrevs);
    if (pThis->pNestedMod)
    {
        pThis->pNestedMod->pImgVt->pfnClose(pThis->pNestedMod);
        RTStrCacheRelease(g_hDbgModStrCache, pThis->pNestedMod->pszName);
        RTStrCacheRelease(g_hDbgModStrCache, pThis->pNestedMod->pszDbgFile);
        RTMemFree(pThis->pNestedMod);
        pThis->pNestedMod = NULL;
    }

#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
    uint32_t i = RT_ELEMENTS(pThis->aDieAllocators);
    while (i-- > 0)
    {
        RTMemCacheDestroy(pThis->aDieAllocators[i].hMemCache);
        pThis->aDieAllocators[i].hMemCache = NIL_RTMEMCACHE;
    }
#endif

    RTMemFree(pThis);

    return VINF_SUCCESS;
}


/** @callback_method_impl{FNRTLDRENUMDBG} */
static DECLCALLBACK(int) rtDbgModDwarfEnumCallback(RTLDRMOD hLdrMod, PCRTLDRDBGINFO pDbgInfo, void *pvUser)
{
    RT_NOREF_PV(hLdrMod);

    /*
     * Skip stuff we can't handle.
     */
    if (pDbgInfo->enmType != RTLDRDBGINFOTYPE_DWARF)
        return VINF_SUCCESS;
    const char *pszSection = pDbgInfo->u.Dwarf.pszSection;
    if (!pszSection || !*pszSection)
        return VINF_SUCCESS;
    Assert(!pDbgInfo->pszExtFile);

    /*
     * Must have a part name starting with debug_ and possibly prefixed by dots
     * or underscores.
     */
    if (!strncmp(pszSection, RT_STR_TUPLE(".debug_")))       /* ELF */
        pszSection += sizeof(".debug_") - 1;
    else if (!strncmp(pszSection, RT_STR_TUPLE("__debug_"))) /* Mach-O */
        pszSection += sizeof("__debug_") - 1;
    else if (!strcmp(pszSection, ".WATCOM_references"))
        return VINF_SUCCESS; /* Ignore special watcom section for now.*/
    else if (   !strcmp(pszSection, "__apple_types")
             || !strcmp(pszSection, "__apple_namespac")
             || !strcmp(pszSection, "__apple_objc")
             || !strcmp(pszSection, "__apple_names"))
        return VINF_SUCCESS; /* Ignore special apple sections for now. */
    else
        AssertMsgFailedReturn(("%s\n", pszSection), VINF_SUCCESS /*ignore*/);

    /*
     * Figure out which part we're talking about.
     */
    krtDbgModDwarfSect enmSect;
    if (0) { /* dummy */ }
#define ELSE_IF_STRCMP_SET(a_Name) else if (!strcmp(pszSection, #a_Name))  enmSect = krtDbgModDwarfSect_ ## a_Name
    ELSE_IF_STRCMP_SET(abbrev);
    ELSE_IF_STRCMP_SET(aranges);
    ELSE_IF_STRCMP_SET(frame);
    ELSE_IF_STRCMP_SET(info);
    ELSE_IF_STRCMP_SET(inlined);
    ELSE_IF_STRCMP_SET(line);
    ELSE_IF_STRCMP_SET(loc);
    ELSE_IF_STRCMP_SET(macinfo);
    ELSE_IF_STRCMP_SET(pubnames);
    ELSE_IF_STRCMP_SET(pubtypes);
    ELSE_IF_STRCMP_SET(ranges);
    ELSE_IF_STRCMP_SET(str);
    ELSE_IF_STRCMP_SET(types);
#undef ELSE_IF_STRCMP_SET
    else
    {
        AssertMsgFailed(("%s\n", pszSection));
        return VINF_SUCCESS;
    }

    /*
     * Record the section.
     */
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)pvUser;
    AssertMsgReturn(!pThis->aSections[enmSect].fPresent, ("duplicate %s\n", pszSection), VINF_SUCCESS /*ignore*/);

    pThis->aSections[enmSect].fPresent  = true;
    pThis->aSections[enmSect].offFile   = pDbgInfo->offFile;
    pThis->aSections[enmSect].pv        = NULL;
    pThis->aSections[enmSect].cb        = (size_t)pDbgInfo->cb;
    pThis->aSections[enmSect].iDbgInfo  = pDbgInfo->iDbgInfo;
    if (pThis->aSections[enmSect].cb != pDbgInfo->cb)
        pThis->aSections[enmSect].cb    = ~(size_t)0;

    return VINF_SUCCESS;
}


static int rtDbgModDwarfTryOpenDbgFile(PRTDBGMODINT pDbgMod, PRTDBGMODDWARF pThis, RTLDRARCH enmArch)
{
    if (   !pDbgMod->pszDbgFile
        || RTPathIsSame(pDbgMod->pszDbgFile, pDbgMod->pszImgFile) == (int)true /* returns VERR too */)
        return VERR_DBG_NO_MATCHING_INTERPRETER;

    /*
     * Only open the image.
     */
    PRTDBGMODINT pDbgInfoMod = (PRTDBGMODINT)RTMemAllocZ(sizeof(*pDbgInfoMod));
    if (!pDbgInfoMod)
        return VERR_NO_MEMORY;

    int rc;
    pDbgInfoMod->u32Magic     = RTDBGMOD_MAGIC;
    pDbgInfoMod->cRefs        = 1;
    if (RTStrCacheRetain(pDbgMod->pszDbgFile) != UINT32_MAX)
    {
        pDbgInfoMod->pszImgFile = pDbgMod->pszDbgFile;
        if (RTStrCacheRetain(pDbgMod->pszName) != UINT32_MAX)
        {
            pDbgInfoMod->pszName = pDbgMod->pszName;
            pDbgInfoMod->pImgVt  = &g_rtDbgModVtImgLdr;
            rc = pDbgInfoMod->pImgVt->pfnTryOpen(pDbgInfoMod, enmArch);
            if (RT_SUCCESS(rc))
            {
                pThis->pDbgInfoMod = pDbgInfoMod;
                pThis->pNestedMod  = pDbgInfoMod;
                return VINF_SUCCESS;
            }

            RTStrCacheRelease(g_hDbgModStrCache, pDbgInfoMod->pszName);
        }
        else
            rc = VERR_NO_STR_MEMORY;
        RTStrCacheRelease(g_hDbgModStrCache,  pDbgInfoMod->pszImgFile);
    }
    else
        rc = VERR_NO_STR_MEMORY;
    RTMemFree(pDbgInfoMod);
    return rc;
}


/** @interface_method_impl{RTDBGMODVTDBG,pfnTryOpen} */
static DECLCALLBACK(int) rtDbgModDwarf_TryOpen(PRTDBGMODINT pMod, RTLDRARCH enmArch)
{
    /*
     * DWARF is only supported when part of an image.
     */
    if (!pMod->pImgVt)
        return VERR_DBG_NO_MATCHING_INTERPRETER;

    /*
     * Create the module instance data.
     */
    PRTDBGMODDWARF pThis = (PRTDBGMODDWARF)RTMemAllocZ(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;
    pThis->pDbgInfoMod = pMod;
    pThis->pImgMod     = pMod;
    RTListInit(&pThis->CompileUnitList);
    /** @todo better fUseLinkAddress heuristics! */
    if (   (pMod->pszDbgFile          && strstr(pMod->pszDbgFile,          "mach_kernel"))
        || (pMod->pszImgFile          && strstr(pMod->pszImgFile,          "mach_kernel"))
        || (pMod->pszImgFileSpecified && strstr(pMod->pszImgFileSpecified, "mach_kernel")) )
        pThis->fUseLinkAddress = true;

#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
    AssertCompile(RT_ELEMENTS(pThis->aDieAllocators) == 2);
    pThis->aDieAllocators[0].cbMax = sizeof(RTDWARFDIE);
    pThis->aDieAllocators[1].cbMax = sizeof(RTDWARFDIECOMPILEUNIT);
    for (uint32_t i = 0; i < RT_ELEMENTS(g_aTagDescs); i++)
        if (g_aTagDescs[i].pDesc && g_aTagDescs[i].pDesc->cbDie > pThis->aDieAllocators[1].cbMax)
            pThis->aDieAllocators[1].cbMax = (uint32_t)g_aTagDescs[i].pDesc->cbDie;
    pThis->aDieAllocators[1].cbMax = RT_ALIGN_32(pThis->aDieAllocators[1].cbMax, sizeof(uint64_t));

    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aDieAllocators); i++)
    {
        int rc = RTMemCacheCreate(&pThis->aDieAllocators[i].hMemCache, pThis->aDieAllocators[i].cbMax, sizeof(uint64_t),
                                  UINT32_MAX, NULL /*pfnCtor*/, NULL /*pfnDtor*/, NULL /*pvUser*/, 0 /*fFlags*/);
        if (RT_FAILURE(rc))
        {
            while (i-- > 0)
                RTMemCacheDestroy(pThis->aDieAllocators[i].hMemCache);
            RTMemFree(pThis);
            return rc;
        }
    }
#endif

    /*
     * If the debug file name is set, let's see if it's an ELF image with DWARF
     * inside it. In that case we'll have to deal with two image modules, one
     * for segments and address translation and one for the debug information.
     */
    if (pMod->pszDbgFile != NULL)
        rtDbgModDwarfTryOpenDbgFile(pMod, pThis, enmArch);

    /*
     * Enumerate the debug info in the module, looking for DWARF bits.
     */
    int rc = pThis->pDbgInfoMod->pImgVt->pfnEnumDbgInfo(pThis->pDbgInfoMod, rtDbgModDwarfEnumCallback, pThis);
    if (RT_SUCCESS(rc))
    {
        if (pThis->aSections[krtDbgModDwarfSect_info].fPresent)
        {
            /*
             * Extract / explode the data we want (symbols and line numbers)
             * storing them in a container module.
             */
            rc = RTDbgModCreate(&pThis->hCnt, pMod->pszName, 0 /*cbSeg*/, 0 /*fFlags*/);
            if (RT_SUCCESS(rc))
            {
                pMod->pvDbgPriv = pThis;

                rc = rtDbgModDwarfAddSegmentsFromImage(pThis);
                if (RT_SUCCESS(rc))
                    rc = rtDwarfInfo_LoadAll(pThis);
                if (RT_SUCCESS(rc))
                    rc = rtDwarfSyms_LoadAll(pThis);
                if (RT_SUCCESS(rc))
                    rc = rtDwarfLine_ExplodeAll(pThis);
                if (RT_SUCCESS(rc) && pThis->iWatcomPass == 1)
                {
                    rc = rtDbgModDwarfAddSegmentsFromPass1(pThis);
                    pThis->iWatcomPass = 2;
                    if (RT_SUCCESS(rc))
                        rc = rtDwarfInfo_LoadAll(pThis);
                    if (RT_SUCCESS(rc))
                        rc = rtDwarfSyms_LoadAll(pThis);
                    if (RT_SUCCESS(rc))
                        rc = rtDwarfLine_ExplodeAll(pThis);
                }
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Free the cached abbreviations and unload all sections.
                     */
                    pThis->cCachedAbbrevsAlloced = 0;
                    RTMemFree(pThis->paCachedAbbrevs);
                    pThis->paCachedAbbrevs = NULL;

                    for (unsigned iSect = 0; iSect < RT_ELEMENTS(pThis->aSections); iSect++)
                        if (pThis->aSections[iSect].pv)
                            pThis->pDbgInfoMod->pImgVt->pfnUnmapPart(pThis->pDbgInfoMod, pThis->aSections[iSect].cb,
                                                                     &pThis->aSections[iSect].pv);

                    /** @todo Kill pThis->CompileUnitList and the alloc caches. */
                    return VINF_SUCCESS;
                }

                /* bail out. */
                RTDbgModRelease(pThis->hCnt);
                pMod->pvDbgPriv = NULL;
            }
        }
        else
            rc = VERR_DBG_NO_MATCHING_INTERPRETER;
    }

    RTMemFree(pThis->paCachedAbbrevs);

#ifdef RTDBGMODDWARF_WITH_MEM_CACHE
    uint32_t i = RT_ELEMENTS(pThis->aDieAllocators);
    while (i-- > 0)
    {
        RTMemCacheDestroy(pThis->aDieAllocators[i].hMemCache);
        pThis->aDieAllocators[i].hMemCache = NIL_RTMEMCACHE;
    }
#endif

    RTMemFree(pThis);

    return rc;
}



/** Virtual function table for the DWARF debug info reader. */
DECL_HIDDEN_CONST(RTDBGMODVTDBG) const g_rtDbgModVtDbgDwarf =
{
    /*.u32Magic = */            RTDBGMODVTDBG_MAGIC,
    /*.fSupports = */           RT_DBGTYPE_DWARF,
    /*.pszName = */             "dwarf",
    /*.pfnTryOpen = */          rtDbgModDwarf_TryOpen,
    /*.pfnClose = */            rtDbgModDwarf_Close,

    /*.pfnRvaToSegOff = */      rtDbgModDwarf_RvaToSegOff,
    /*.pfnImageSize = */        rtDbgModDwarf_ImageSize,

    /*.pfnSegmentAdd = */       rtDbgModDwarf_SegmentAdd,
    /*.pfnSegmentCount = */     rtDbgModDwarf_SegmentCount,
    /*.pfnSegmentByIndex = */   rtDbgModDwarf_SegmentByIndex,

    /*.pfnSymbolAdd = */        rtDbgModDwarf_SymbolAdd,
    /*.pfnSymbolCount = */      rtDbgModDwarf_SymbolCount,
    /*.pfnSymbolByOrdinal = */  rtDbgModDwarf_SymbolByOrdinal,
    /*.pfnSymbolByName = */     rtDbgModDwarf_SymbolByName,
    /*.pfnSymbolByAddr = */     rtDbgModDwarf_SymbolByAddr,

    /*.pfnLineAdd = */          rtDbgModDwarf_LineAdd,
    /*.pfnLineCount = */        rtDbgModDwarf_LineCount,
    /*.pfnLineByOrdinal = */    rtDbgModDwarf_LineByOrdinal,
    /*.pfnLineByAddr = */       rtDbgModDwarf_LineByAddr,

    /*.u32EndMagic = */         RTDBGMODVTDBG_MAGIC
};


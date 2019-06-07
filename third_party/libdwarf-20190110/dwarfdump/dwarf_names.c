/* Generated routines, do not edit. */
/* Generated sourcedate  2019-01-10 09:25:09-08:00   */

/* BEGIN FILE */

#include "dwarf.h"

#include "libdwarf.h"

typedef struct Names_Data {
    const char *l_name; 
    unsigned    value;  
} Names_Data;

/* Use standard binary search to get entry */
static int
find_entry(Names_Data *table,const int last,unsigned value, const char **s_out)
{
    int low = 0;
    int high = last;
    int mid;
    unsigned maxval = table[last-1].value;

    if (value > maxval) {
        return DW_DLV_NO_ENTRY;
    }
    while (low < high) {
        mid = low + ((high - low) / 2);
        if(mid == last) {
            break;
        }
        if (table[mid].value < value) {
            low = mid + 1;
        }
        else {
              high = mid;
        }
    }

    if (low < last && table[low].value == value) {
        /* Found: low is the entry */
      *s_out = table[low].l_name;
      return DW_DLV_OK;
    }
    return DW_DLV_NO_ENTRY;
}

/* ARGSUSED */
int
dwarf_get_TAG_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_TAG_n[] = {
    {/*   0 */ "DW_TAG_array_type",  DW_TAG_array_type},
    {/*   1 */ "DW_TAG_class_type",  DW_TAG_class_type},
    {/*   2 */ "DW_TAG_entry_point",  DW_TAG_entry_point},
    {/*   3 */ "DW_TAG_enumeration_type",  DW_TAG_enumeration_type},
    {/*   4 */ "DW_TAG_formal_parameter",  DW_TAG_formal_parameter},
    {/*   5 */ "DW_TAG_imported_declaration",  DW_TAG_imported_declaration},
    {/*   6 */ "DW_TAG_label",  DW_TAG_label},
    {/*   7 */ "DW_TAG_lexical_block",  DW_TAG_lexical_block},
    {/*   8 */ "DW_TAG_member",  DW_TAG_member},
    {/*   9 */ "DW_TAG_pointer_type",  DW_TAG_pointer_type},
    {/*  10 */ "DW_TAG_reference_type",  DW_TAG_reference_type},
    {/*  11 */ "DW_TAG_compile_unit",  DW_TAG_compile_unit},
    {/*  12 */ "DW_TAG_string_type",  DW_TAG_string_type},
    {/*  13 */ "DW_TAG_structure_type",  DW_TAG_structure_type},
    {/*  14 */ "DW_TAG_subroutine_type",  DW_TAG_subroutine_type},
    {/*  15 */ "DW_TAG_typedef",  DW_TAG_typedef},
    {/*  16 */ "DW_TAG_union_type",  DW_TAG_union_type},
    {/*  17 */ "DW_TAG_unspecified_parameters",  DW_TAG_unspecified_parameters},
    {/*  18 */ "DW_TAG_variant",  DW_TAG_variant},
    {/*  19 */ "DW_TAG_common_block",  DW_TAG_common_block},
    {/*  20 */ "DW_TAG_common_inclusion",  DW_TAG_common_inclusion},
    {/*  21 */ "DW_TAG_inheritance",  DW_TAG_inheritance},
    {/*  22 */ "DW_TAG_inlined_subroutine",  DW_TAG_inlined_subroutine},
    {/*  23 */ "DW_TAG_module",  DW_TAG_module},
    {/*  24 */ "DW_TAG_ptr_to_member_type",  DW_TAG_ptr_to_member_type},
    {/*  25 */ "DW_TAG_set_type",  DW_TAG_set_type},
    {/*  26 */ "DW_TAG_subrange_type",  DW_TAG_subrange_type},
    {/*  27 */ "DW_TAG_with_stmt",  DW_TAG_with_stmt},
    {/*  28 */ "DW_TAG_access_declaration",  DW_TAG_access_declaration},
    {/*  29 */ "DW_TAG_base_type",  DW_TAG_base_type},
    {/*  30 */ "DW_TAG_catch_block",  DW_TAG_catch_block},
    {/*  31 */ "DW_TAG_const_type",  DW_TAG_const_type},
    {/*  32 */ "DW_TAG_constant",  DW_TAG_constant},
    {/*  33 */ "DW_TAG_enumerator",  DW_TAG_enumerator},
    {/*  34 */ "DW_TAG_file_type",  DW_TAG_file_type},
    {/*  35 */ "DW_TAG_friend",  DW_TAG_friend},
    {/*  36 */ "DW_TAG_namelist",  DW_TAG_namelist},
    {/*  37 */ "DW_TAG_namelist_item",  DW_TAG_namelist_item},
    /* Skipping alternate spelling of value 0x2c. DW_TAG_namelist_items */
    {/*  38 */ "DW_TAG_packed_type",  DW_TAG_packed_type},
    {/*  39 */ "DW_TAG_subprogram",  DW_TAG_subprogram},
    {/*  40 */ "DW_TAG_template_type_parameter",  DW_TAG_template_type_parameter},
    /* Skipping alternate spelling of value 0x2f. DW_TAG_template_type_param */
    {/*  41 */ "DW_TAG_template_value_parameter",  DW_TAG_template_value_parameter},
    /* Skipping alternate spelling of value 0x30. DW_TAG_template_value_param */
    {/*  42 */ "DW_TAG_thrown_type",  DW_TAG_thrown_type},
    {/*  43 */ "DW_TAG_try_block",  DW_TAG_try_block},
    {/*  44 */ "DW_TAG_variant_part",  DW_TAG_variant_part},
    {/*  45 */ "DW_TAG_variable",  DW_TAG_variable},
    {/*  46 */ "DW_TAG_volatile_type",  DW_TAG_volatile_type},
    {/*  47 */ "DW_TAG_dwarf_procedure",  DW_TAG_dwarf_procedure},
    {/*  48 */ "DW_TAG_restrict_type",  DW_TAG_restrict_type},
    {/*  49 */ "DW_TAG_interface_type",  DW_TAG_interface_type},
    {/*  50 */ "DW_TAG_namespace",  DW_TAG_namespace},
    {/*  51 */ "DW_TAG_imported_module",  DW_TAG_imported_module},
    {/*  52 */ "DW_TAG_unspecified_type",  DW_TAG_unspecified_type},
    {/*  53 */ "DW_TAG_partial_unit",  DW_TAG_partial_unit},
    {/*  54 */ "DW_TAG_imported_unit",  DW_TAG_imported_unit},
    {/*  55 */ "DW_TAG_mutable_type",  DW_TAG_mutable_type},
    {/*  56 */ "DW_TAG_condition",  DW_TAG_condition},
    {/*  57 */ "DW_TAG_shared_type",  DW_TAG_shared_type},
    {/*  58 */ "DW_TAG_type_unit",  DW_TAG_type_unit},
    {/*  59 */ "DW_TAG_rvalue_reference_type",  DW_TAG_rvalue_reference_type},
    {/*  60 */ "DW_TAG_template_alias",  DW_TAG_template_alias},
    {/*  61 */ "DW_TAG_coarray_type",  DW_TAG_coarray_type},
    {/*  62 */ "DW_TAG_generic_subrange",  DW_TAG_generic_subrange},
    {/*  63 */ "DW_TAG_dynamic_type",  DW_TAG_dynamic_type},
    {/*  64 */ "DW_TAG_atomic_type",  DW_TAG_atomic_type},
    {/*  65 */ "DW_TAG_call_site",  DW_TAG_call_site},
    {/*  66 */ "DW_TAG_call_site_parameter",  DW_TAG_call_site_parameter},
    {/*  67 */ "DW_TAG_skeleton_unit",  DW_TAG_skeleton_unit},
    {/*  68 */ "DW_TAG_immutable_type",  DW_TAG_immutable_type},
    {/*  69 */ "DW_TAG_lo_user",  DW_TAG_lo_user},
    {/*  70 */ "DW_TAG_MIPS_loop",  DW_TAG_MIPS_loop},
    {/*  71 */ "DW_TAG_HP_array_descriptor",  DW_TAG_HP_array_descriptor},
    {/*  72 */ "DW_TAG_format_label",  DW_TAG_format_label},
    {/*  73 */ "DW_TAG_function_template",  DW_TAG_function_template},
    {/*  74 */ "DW_TAG_class_template",  DW_TAG_class_template},
    {/*  75 */ "DW_TAG_GNU_BINCL",  DW_TAG_GNU_BINCL},
    {/*  76 */ "DW_TAG_GNU_EINCL",  DW_TAG_GNU_EINCL},
    {/*  77 */ "DW_TAG_GNU_template_template_parameter",  DW_TAG_GNU_template_template_parameter},
    /* Skipping alternate spelling of value 0x4106. DW_TAG_GNU_template_template_param */
    {/*  78 */ "DW_TAG_GNU_template_parameter_pack",  DW_TAG_GNU_template_parameter_pack},
    {/*  79 */ "DW_TAG_GNU_formal_parameter_pack",  DW_TAG_GNU_formal_parameter_pack},
    {/*  80 */ "DW_TAG_GNU_call_site",  DW_TAG_GNU_call_site},
    {/*  81 */ "DW_TAG_GNU_call_site_parameter",  DW_TAG_GNU_call_site_parameter},
    {/*  82 */ "DW_TAG_SUN_function_template",  DW_TAG_SUN_function_template},
    {/*  83 */ "DW_TAG_SUN_class_template",  DW_TAG_SUN_class_template},
    {/*  84 */ "DW_TAG_SUN_struct_template",  DW_TAG_SUN_struct_template},
    {/*  85 */ "DW_TAG_SUN_union_template",  DW_TAG_SUN_union_template},
    {/*  86 */ "DW_TAG_SUN_indirect_inheritance",  DW_TAG_SUN_indirect_inheritance},
    {/*  87 */ "DW_TAG_SUN_codeflags",  DW_TAG_SUN_codeflags},
    {/*  88 */ "DW_TAG_SUN_memop_info",  DW_TAG_SUN_memop_info},
    {/*  89 */ "DW_TAG_SUN_omp_child_func",  DW_TAG_SUN_omp_child_func},
    {/*  90 */ "DW_TAG_SUN_rtti_descriptor",  DW_TAG_SUN_rtti_descriptor},
    {/*  91 */ "DW_TAG_SUN_dtor_info",  DW_TAG_SUN_dtor_info},
    {/*  92 */ "DW_TAG_SUN_dtor",  DW_TAG_SUN_dtor},
    {/*  93 */ "DW_TAG_SUN_f90_interface",  DW_TAG_SUN_f90_interface},
    {/*  94 */ "DW_TAG_SUN_fortran_vax_structure",  DW_TAG_SUN_fortran_vax_structure},
    {/*  95 */ "DW_TAG_SUN_hi",  DW_TAG_SUN_hi},
    {/*  96 */ "DW_TAG_ALTIUM_circ_type",  DW_TAG_ALTIUM_circ_type},
    {/*  97 */ "DW_TAG_ALTIUM_mwa_circ_type",  DW_TAG_ALTIUM_mwa_circ_type},
    {/*  98 */ "DW_TAG_ALTIUM_rev_carry_type",  DW_TAG_ALTIUM_rev_carry_type},
    {/*  99 */ "DW_TAG_ALTIUM_rom",  DW_TAG_ALTIUM_rom},
    {/* 100 */ "DW_TAG_upc_shared_type",  DW_TAG_upc_shared_type},
    {/* 101 */ "DW_TAG_upc_strict_type",  DW_TAG_upc_strict_type},
    {/* 102 */ "DW_TAG_upc_relaxed_type",  DW_TAG_upc_relaxed_type},
    {/* 103 */ "DW_TAG_PGI_kanji_type",  DW_TAG_PGI_kanji_type},
    {/* 104 */ "DW_TAG_PGI_interface_block",  DW_TAG_PGI_interface_block},
    {/* 105 */ "DW_TAG_hi_user",  DW_TAG_hi_user}
    };

    const int last_entry = 106;
    /* find the entry */
    int r = find_entry(Dwarf_TAG_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_children_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_children_n[] = {
    {/*   0 */ "DW_children_no",  DW_children_no},
    {/*   1 */ "DW_children_yes",  DW_children_yes}
    };

    const int last_entry = 2;
    /* find the entry */
    int r = find_entry(Dwarf_children_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_FORM_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_FORM_n[] = {
    {/*   0 */ "DW_FORM_addr",  DW_FORM_addr},
    {/*   1 */ "DW_FORM_block2",  DW_FORM_block2},
    {/*   2 */ "DW_FORM_block4",  DW_FORM_block4},
    {/*   3 */ "DW_FORM_data2",  DW_FORM_data2},
    {/*   4 */ "DW_FORM_data4",  DW_FORM_data4},
    {/*   5 */ "DW_FORM_data8",  DW_FORM_data8},
    {/*   6 */ "DW_FORM_string",  DW_FORM_string},
    {/*   7 */ "DW_FORM_block",  DW_FORM_block},
    {/*   8 */ "DW_FORM_block1",  DW_FORM_block1},
    {/*   9 */ "DW_FORM_data1",  DW_FORM_data1},
    {/*  10 */ "DW_FORM_flag",  DW_FORM_flag},
    {/*  11 */ "DW_FORM_sdata",  DW_FORM_sdata},
    {/*  12 */ "DW_FORM_strp",  DW_FORM_strp},
    {/*  13 */ "DW_FORM_udata",  DW_FORM_udata},
    {/*  14 */ "DW_FORM_ref_addr",  DW_FORM_ref_addr},
    {/*  15 */ "DW_FORM_ref1",  DW_FORM_ref1},
    {/*  16 */ "DW_FORM_ref2",  DW_FORM_ref2},
    {/*  17 */ "DW_FORM_ref4",  DW_FORM_ref4},
    {/*  18 */ "DW_FORM_ref8",  DW_FORM_ref8},
    {/*  19 */ "DW_FORM_ref_udata",  DW_FORM_ref_udata},
    {/*  20 */ "DW_FORM_indirect",  DW_FORM_indirect},
    {/*  21 */ "DW_FORM_sec_offset",  DW_FORM_sec_offset},
    {/*  22 */ "DW_FORM_exprloc",  DW_FORM_exprloc},
    {/*  23 */ "DW_FORM_flag_present",  DW_FORM_flag_present},
    {/*  24 */ "DW_FORM_strx",  DW_FORM_strx},
    {/*  25 */ "DW_FORM_addrx",  DW_FORM_addrx},
    {/*  26 */ "DW_FORM_ref_sup4",  DW_FORM_ref_sup4},
    {/*  27 */ "DW_FORM_strp_sup",  DW_FORM_strp_sup},
    {/*  28 */ "DW_FORM_data16",  DW_FORM_data16},
    {/*  29 */ "DW_FORM_line_strp",  DW_FORM_line_strp},
    {/*  30 */ "DW_FORM_ref_sig8",  DW_FORM_ref_sig8},
    {/*  31 */ "DW_FORM_implicit_const",  DW_FORM_implicit_const},
    {/*  32 */ "DW_FORM_loclistx",  DW_FORM_loclistx},
    {/*  33 */ "DW_FORM_rnglistx",  DW_FORM_rnglistx},
    {/*  34 */ "DW_FORM_ref_sup8",  DW_FORM_ref_sup8},
    {/*  35 */ "DW_FORM_strx1",  DW_FORM_strx1},
    {/*  36 */ "DW_FORM_strx2",  DW_FORM_strx2},
    {/*  37 */ "DW_FORM_strx3",  DW_FORM_strx3},
    {/*  38 */ "DW_FORM_strx4",  DW_FORM_strx4},
    {/*  39 */ "DW_FORM_addrx1",  DW_FORM_addrx1},
    {/*  40 */ "DW_FORM_addrx2",  DW_FORM_addrx2},
    {/*  41 */ "DW_FORM_addrx3",  DW_FORM_addrx3},
    {/*  42 */ "DW_FORM_addrx4",  DW_FORM_addrx4},
    {/*  43 */ "DW_FORM_GNU_addr_index",  DW_FORM_GNU_addr_index},
    {/*  44 */ "DW_FORM_GNU_str_index",  DW_FORM_GNU_str_index},
    {/*  45 */ "DW_FORM_GNU_ref_alt",  DW_FORM_GNU_ref_alt},
    {/*  46 */ "DW_FORM_GNU_strp_alt",  DW_FORM_GNU_strp_alt}
    };

    const int last_entry = 47;
    /* find the entry */
    int r = find_entry(Dwarf_FORM_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_AT_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_AT_n[] = {
    {/*   0 */ "DW_AT_sibling",  DW_AT_sibling},
    {/*   1 */ "DW_AT_location",  DW_AT_location},
    {/*   2 */ "DW_AT_name",  DW_AT_name},
    {/*   3 */ "DW_AT_ordering",  DW_AT_ordering},
    {/*   4 */ "DW_AT_subscr_data",  DW_AT_subscr_data},
    {/*   5 */ "DW_AT_byte_size",  DW_AT_byte_size},
    {/*   6 */ "DW_AT_bit_offset",  DW_AT_bit_offset},
    {/*   7 */ "DW_AT_bit_size",  DW_AT_bit_size},
    {/*   8 */ "DW_AT_element_list",  DW_AT_element_list},
    {/*   9 */ "DW_AT_stmt_list",  DW_AT_stmt_list},
    {/*  10 */ "DW_AT_low_pc",  DW_AT_low_pc},
    {/*  11 */ "DW_AT_high_pc",  DW_AT_high_pc},
    {/*  12 */ "DW_AT_language",  DW_AT_language},
    {/*  13 */ "DW_AT_member",  DW_AT_member},
    {/*  14 */ "DW_AT_discr",  DW_AT_discr},
    {/*  15 */ "DW_AT_discr_value",  DW_AT_discr_value},
    {/*  16 */ "DW_AT_visibility",  DW_AT_visibility},
    {/*  17 */ "DW_AT_import",  DW_AT_import},
    {/*  18 */ "DW_AT_string_length",  DW_AT_string_length},
    {/*  19 */ "DW_AT_common_reference",  DW_AT_common_reference},
    {/*  20 */ "DW_AT_comp_dir",  DW_AT_comp_dir},
    {/*  21 */ "DW_AT_const_value",  DW_AT_const_value},
    {/*  22 */ "DW_AT_containing_type",  DW_AT_containing_type},
    {/*  23 */ "DW_AT_default_value",  DW_AT_default_value},
    {/*  24 */ "DW_AT_inline",  DW_AT_inline},
    {/*  25 */ "DW_AT_is_optional",  DW_AT_is_optional},
    {/*  26 */ "DW_AT_lower_bound",  DW_AT_lower_bound},
    {/*  27 */ "DW_AT_producer",  DW_AT_producer},
    {/*  28 */ "DW_AT_prototyped",  DW_AT_prototyped},
    {/*  29 */ "DW_AT_return_addr",  DW_AT_return_addr},
    {/*  30 */ "DW_AT_start_scope",  DW_AT_start_scope},
    {/*  31 */ "DW_AT_bit_stride",  DW_AT_bit_stride},
    /* Skipping alternate spelling of value 0x2e. DW_AT_stride_size */
    {/*  32 */ "DW_AT_upper_bound",  DW_AT_upper_bound},
    {/*  33 */ "DW_AT_abstract_origin",  DW_AT_abstract_origin},
    {/*  34 */ "DW_AT_accessibility",  DW_AT_accessibility},
    {/*  35 */ "DW_AT_address_class",  DW_AT_address_class},
    {/*  36 */ "DW_AT_artificial",  DW_AT_artificial},
    {/*  37 */ "DW_AT_base_types",  DW_AT_base_types},
    {/*  38 */ "DW_AT_calling_convention",  DW_AT_calling_convention},
    {/*  39 */ "DW_AT_count",  DW_AT_count},
    {/*  40 */ "DW_AT_data_member_location",  DW_AT_data_member_location},
    {/*  41 */ "DW_AT_decl_column",  DW_AT_decl_column},
    {/*  42 */ "DW_AT_decl_file",  DW_AT_decl_file},
    {/*  43 */ "DW_AT_decl_line",  DW_AT_decl_line},
    {/*  44 */ "DW_AT_declaration",  DW_AT_declaration},
    {/*  45 */ "DW_AT_discr_list",  DW_AT_discr_list},
    {/*  46 */ "DW_AT_encoding",  DW_AT_encoding},
    {/*  47 */ "DW_AT_external",  DW_AT_external},
    {/*  48 */ "DW_AT_frame_base",  DW_AT_frame_base},
    {/*  49 */ "DW_AT_friend",  DW_AT_friend},
    {/*  50 */ "DW_AT_identifier_case",  DW_AT_identifier_case},
    {/*  51 */ "DW_AT_macro_info",  DW_AT_macro_info},
    {/*  52 */ "DW_AT_namelist_item",  DW_AT_namelist_item},
    {/*  53 */ "DW_AT_priority",  DW_AT_priority},
    {/*  54 */ "DW_AT_segment",  DW_AT_segment},
    {/*  55 */ "DW_AT_specification",  DW_AT_specification},
    {/*  56 */ "DW_AT_static_link",  DW_AT_static_link},
    {/*  57 */ "DW_AT_type",  DW_AT_type},
    {/*  58 */ "DW_AT_use_location",  DW_AT_use_location},
    {/*  59 */ "DW_AT_variable_parameter",  DW_AT_variable_parameter},
    {/*  60 */ "DW_AT_virtuality",  DW_AT_virtuality},
    {/*  61 */ "DW_AT_vtable_elem_location",  DW_AT_vtable_elem_location},
    {/*  62 */ "DW_AT_allocated",  DW_AT_allocated},
    {/*  63 */ "DW_AT_associated",  DW_AT_associated},
    {/*  64 */ "DW_AT_data_location",  DW_AT_data_location},
    {/*  65 */ "DW_AT_byte_stride",  DW_AT_byte_stride},
    /* Skipping alternate spelling of value 0x51. DW_AT_stride */
    {/*  66 */ "DW_AT_entry_pc",  DW_AT_entry_pc},
    {/*  67 */ "DW_AT_use_UTF8",  DW_AT_use_UTF8},
    {/*  68 */ "DW_AT_extension",  DW_AT_extension},
    {/*  69 */ "DW_AT_ranges",  DW_AT_ranges},
    {/*  70 */ "DW_AT_trampoline",  DW_AT_trampoline},
    {/*  71 */ "DW_AT_call_column",  DW_AT_call_column},
    {/*  72 */ "DW_AT_call_file",  DW_AT_call_file},
    {/*  73 */ "DW_AT_call_line",  DW_AT_call_line},
    {/*  74 */ "DW_AT_description",  DW_AT_description},
    {/*  75 */ "DW_AT_binary_scale",  DW_AT_binary_scale},
    {/*  76 */ "DW_AT_decimal_scale",  DW_AT_decimal_scale},
    {/*  77 */ "DW_AT_small",  DW_AT_small},
    {/*  78 */ "DW_AT_decimal_sign",  DW_AT_decimal_sign},
    {/*  79 */ "DW_AT_digit_count",  DW_AT_digit_count},
    {/*  80 */ "DW_AT_picture_string",  DW_AT_picture_string},
    {/*  81 */ "DW_AT_mutable",  DW_AT_mutable},
    {/*  82 */ "DW_AT_threads_scaled",  DW_AT_threads_scaled},
    {/*  83 */ "DW_AT_explicit",  DW_AT_explicit},
    {/*  84 */ "DW_AT_object_pointer",  DW_AT_object_pointer},
    {/*  85 */ "DW_AT_endianity",  DW_AT_endianity},
    {/*  86 */ "DW_AT_elemental",  DW_AT_elemental},
    {/*  87 */ "DW_AT_pure",  DW_AT_pure},
    {/*  88 */ "DW_AT_recursive",  DW_AT_recursive},
    {/*  89 */ "DW_AT_signature",  DW_AT_signature},
    {/*  90 */ "DW_AT_main_subprogram",  DW_AT_main_subprogram},
    {/*  91 */ "DW_AT_data_bit_offset",  DW_AT_data_bit_offset},
    {/*  92 */ "DW_AT_const_expr",  DW_AT_const_expr},
    {/*  93 */ "DW_AT_enum_class",  DW_AT_enum_class},
    {/*  94 */ "DW_AT_linkage_name",  DW_AT_linkage_name},
    {/*  95 */ "DW_AT_string_length_bit_size",  DW_AT_string_length_bit_size},
    {/*  96 */ "DW_AT_string_length_byte_size",  DW_AT_string_length_byte_size},
    {/*  97 */ "DW_AT_rank",  DW_AT_rank},
    {/*  98 */ "DW_AT_str_offsets_base",  DW_AT_str_offsets_base},
    {/*  99 */ "DW_AT_addr_base",  DW_AT_addr_base},
    {/* 100 */ "DW_AT_rnglists_base",  DW_AT_rnglists_base},
    {/* 101 */ "DW_AT_dwo_id",  DW_AT_dwo_id},
    {/* 102 */ "DW_AT_dwo_name",  DW_AT_dwo_name},
    {/* 103 */ "DW_AT_reference",  DW_AT_reference},
    {/* 104 */ "DW_AT_rvalue_reference",  DW_AT_rvalue_reference},
    {/* 105 */ "DW_AT_macros",  DW_AT_macros},
    {/* 106 */ "DW_AT_call_all_calls",  DW_AT_call_all_calls},
    {/* 107 */ "DW_AT_call_all_source_calls",  DW_AT_call_all_source_calls},
    {/* 108 */ "DW_AT_call_all_tail_calls",  DW_AT_call_all_tail_calls},
    {/* 109 */ "DW_AT_call_return_pc",  DW_AT_call_return_pc},
    {/* 110 */ "DW_AT_call_value",  DW_AT_call_value},
    {/* 111 */ "DW_AT_call_origin",  DW_AT_call_origin},
    {/* 112 */ "DW_AT_call_parameter",  DW_AT_call_parameter},
    {/* 113 */ "DW_AT_call_pc",  DW_AT_call_pc},
    {/* 114 */ "DW_AT_call_tail_call",  DW_AT_call_tail_call},
    {/* 115 */ "DW_AT_call_target",  DW_AT_call_target},
    {/* 116 */ "DW_AT_call_target_clobbered",  DW_AT_call_target_clobbered},
    {/* 117 */ "DW_AT_call_data_location",  DW_AT_call_data_location},
    {/* 118 */ "DW_AT_call_data_value",  DW_AT_call_data_value},
    {/* 119 */ "DW_AT_noreturn",  DW_AT_noreturn},
    {/* 120 */ "DW_AT_alignment",  DW_AT_alignment},
    {/* 121 */ "DW_AT_export_symbols",  DW_AT_export_symbols},
    {/* 122 */ "DW_AT_deleted",  DW_AT_deleted},
    {/* 123 */ "DW_AT_defaulted",  DW_AT_defaulted},
    {/* 124 */ "DW_AT_loclists_base",  DW_AT_loclists_base},
    {/* 125 */ "DW_AT_HP_block_index",  DW_AT_HP_block_index},
    /* Skipping alternate spelling of value 0x2000. DW_AT_lo_user */
    {/* 126 */ "DW_AT_MIPS_fde",  DW_AT_MIPS_fde},
    /* Skipping alternate spelling of value 0x2001. DW_AT_HP_unmodifiable */
    /* Skipping alternate spelling of value 0x2001. DW_AT_CPQ_discontig_ranges */
    {/* 127 */ "DW_AT_MIPS_loop_begin",  DW_AT_MIPS_loop_begin},
    /* Skipping alternate spelling of value 0x2002. DW_AT_CPQ_semantic_events */
    {/* 128 */ "DW_AT_MIPS_tail_loop_begin",  DW_AT_MIPS_tail_loop_begin},
    /* Skipping alternate spelling of value 0x2003. DW_AT_CPQ_split_lifetimes_var */
    {/* 129 */ "DW_AT_MIPS_epilog_begin",  DW_AT_MIPS_epilog_begin},
    /* Skipping alternate spelling of value 0x2004. DW_AT_CPQ_split_lifetimes_rtn */
    {/* 130 */ "DW_AT_MIPS_loop_unroll_factor",  DW_AT_MIPS_loop_unroll_factor},
    /* Skipping alternate spelling of value 0x2005. DW_AT_CPQ_prologue_length */
    {/* 131 */ "DW_AT_MIPS_software_pipeline_depth",  DW_AT_MIPS_software_pipeline_depth},
    {/* 132 */ "DW_AT_MIPS_linkage_name",  DW_AT_MIPS_linkage_name},
    {/* 133 */ "DW_AT_MIPS_stride",  DW_AT_MIPS_stride},
    {/* 134 */ "DW_AT_MIPS_abstract_name",  DW_AT_MIPS_abstract_name},
    {/* 135 */ "DW_AT_MIPS_clone_origin",  DW_AT_MIPS_clone_origin},
    {/* 136 */ "DW_AT_MIPS_has_inlines",  DW_AT_MIPS_has_inlines},
    {/* 137 */ "DW_AT_MIPS_stride_byte",  DW_AT_MIPS_stride_byte},
    {/* 138 */ "DW_AT_MIPS_stride_elem",  DW_AT_MIPS_stride_elem},
    {/* 139 */ "DW_AT_MIPS_ptr_dopetype",  DW_AT_MIPS_ptr_dopetype},
    {/* 140 */ "DW_AT_MIPS_allocatable_dopetype",  DW_AT_MIPS_allocatable_dopetype},
    {/* 141 */ "DW_AT_MIPS_assumed_shape_dopetype",  DW_AT_MIPS_assumed_shape_dopetype},
    /* Skipping alternate spelling of value 0x2010. DW_AT_HP_actuals_stmt_list */
    {/* 142 */ "DW_AT_MIPS_assumed_size",  DW_AT_MIPS_assumed_size},
    /* Skipping alternate spelling of value 0x2011. DW_AT_HP_proc_per_section */
    {/* 143 */ "DW_AT_HP_raw_data_ptr",  DW_AT_HP_raw_data_ptr},
    {/* 144 */ "DW_AT_HP_pass_by_reference",  DW_AT_HP_pass_by_reference},
    {/* 145 */ "DW_AT_HP_opt_level",  DW_AT_HP_opt_level},
    {/* 146 */ "DW_AT_HP_prof_version_id",  DW_AT_HP_prof_version_id},
    {/* 147 */ "DW_AT_HP_opt_flags",  DW_AT_HP_opt_flags},
    {/* 148 */ "DW_AT_HP_cold_region_low_pc",  DW_AT_HP_cold_region_low_pc},
    {/* 149 */ "DW_AT_HP_cold_region_high_pc",  DW_AT_HP_cold_region_high_pc},
    {/* 150 */ "DW_AT_HP_all_variables_modifiable",  DW_AT_HP_all_variables_modifiable},
    {/* 151 */ "DW_AT_HP_linkage_name",  DW_AT_HP_linkage_name},
    {/* 152 */ "DW_AT_HP_prof_flags",  DW_AT_HP_prof_flags},
    {/* 153 */ "DW_AT_INTEL_other_endian",  DW_AT_INTEL_other_endian},
    {/* 154 */ "DW_AT_sf_names",  DW_AT_sf_names},
    {/* 155 */ "DW_AT_src_info",  DW_AT_src_info},
    {/* 156 */ "DW_AT_mac_info",  DW_AT_mac_info},
    {/* 157 */ "DW_AT_src_coords",  DW_AT_src_coords},
    {/* 158 */ "DW_AT_body_begin",  DW_AT_body_begin},
    {/* 159 */ "DW_AT_body_end",  DW_AT_body_end},
    {/* 160 */ "DW_AT_GNU_vector",  DW_AT_GNU_vector},
    {/* 161 */ "DW_AT_GNU_guarded_by",  DW_AT_GNU_guarded_by},
    {/* 162 */ "DW_AT_GNU_pt_guarded_by",  DW_AT_GNU_pt_guarded_by},
    {/* 163 */ "DW_AT_GNU_guarded",  DW_AT_GNU_guarded},
    {/* 164 */ "DW_AT_GNU_pt_guarded",  DW_AT_GNU_pt_guarded},
    {/* 165 */ "DW_AT_GNU_locks_excluded",  DW_AT_GNU_locks_excluded},
    {/* 166 */ "DW_AT_GNU_exclusive_locks_required",  DW_AT_GNU_exclusive_locks_required},
    {/* 167 */ "DW_AT_GNU_shared_locks_required",  DW_AT_GNU_shared_locks_required},
    {/* 168 */ "DW_AT_GNU_odr_signature",  DW_AT_GNU_odr_signature},
    {/* 169 */ "DW_AT_GNU_template_name",  DW_AT_GNU_template_name},
    {/* 170 */ "DW_AT_GNU_call_site_value",  DW_AT_GNU_call_site_value},
    {/* 171 */ "DW_AT_GNU_call_site_data_value",  DW_AT_GNU_call_site_data_value},
    {/* 172 */ "DW_AT_GNU_call_site_target",  DW_AT_GNU_call_site_target},
    {/* 173 */ "DW_AT_GNU_call_site_target_clobbered",  DW_AT_GNU_call_site_target_clobbered},
    {/* 174 */ "DW_AT_GNU_tail_call",  DW_AT_GNU_tail_call},
    {/* 175 */ "DW_AT_GNU_all_tail_call_sites",  DW_AT_GNU_all_tail_call_sites},
    {/* 176 */ "DW_AT_GNU_all_call_sites",  DW_AT_GNU_all_call_sites},
    {/* 177 */ "DW_AT_GNU_all_source_call_sites",  DW_AT_GNU_all_source_call_sites},
    {/* 178 */ "DW_AT_GNU_macros",  DW_AT_GNU_macros},
    {/* 179 */ "DW_AT_GNU_dwo_name",  DW_AT_GNU_dwo_name},
    {/* 180 */ "DW_AT_GNU_dwo_id",  DW_AT_GNU_dwo_id},
    {/* 181 */ "DW_AT_GNU_ranges_base",  DW_AT_GNU_ranges_base},
    {/* 182 */ "DW_AT_GNU_addr_base",  DW_AT_GNU_addr_base},
    {/* 183 */ "DW_AT_GNU_pubnames",  DW_AT_GNU_pubnames},
    {/* 184 */ "DW_AT_GNU_pubtypes",  DW_AT_GNU_pubtypes},
    {/* 185 */ "DW_AT_GNU_discriminator",  DW_AT_GNU_discriminator},
    {/* 186 */ "DW_AT_SUN_template",  DW_AT_SUN_template},
    /* Skipping alternate spelling of value 0x2201. DW_AT_VMS_rtnbeg_pd_address */
    {/* 187 */ "DW_AT_SUN_alignment",  DW_AT_SUN_alignment},
    {/* 188 */ "DW_AT_SUN_vtable",  DW_AT_SUN_vtable},
    {/* 189 */ "DW_AT_SUN_count_guarantee",  DW_AT_SUN_count_guarantee},
    {/* 190 */ "DW_AT_SUN_command_line",  DW_AT_SUN_command_line},
    {/* 191 */ "DW_AT_SUN_vbase",  DW_AT_SUN_vbase},
    {/* 192 */ "DW_AT_SUN_compile_options",  DW_AT_SUN_compile_options},
    {/* 193 */ "DW_AT_SUN_language",  DW_AT_SUN_language},
    {/* 194 */ "DW_AT_SUN_browser_file",  DW_AT_SUN_browser_file},
    {/* 195 */ "DW_AT_SUN_vtable_abi",  DW_AT_SUN_vtable_abi},
    {/* 196 */ "DW_AT_SUN_func_offsets",  DW_AT_SUN_func_offsets},
    {/* 197 */ "DW_AT_SUN_cf_kind",  DW_AT_SUN_cf_kind},
    {/* 198 */ "DW_AT_SUN_vtable_index",  DW_AT_SUN_vtable_index},
    {/* 199 */ "DW_AT_SUN_omp_tpriv_addr",  DW_AT_SUN_omp_tpriv_addr},
    {/* 200 */ "DW_AT_SUN_omp_child_func",  DW_AT_SUN_omp_child_func},
    {/* 201 */ "DW_AT_SUN_func_offset",  DW_AT_SUN_func_offset},
    {/* 202 */ "DW_AT_SUN_memop_type_ref",  DW_AT_SUN_memop_type_ref},
    {/* 203 */ "DW_AT_SUN_profile_id",  DW_AT_SUN_profile_id},
    {/* 204 */ "DW_AT_SUN_memop_signature",  DW_AT_SUN_memop_signature},
    {/* 205 */ "DW_AT_SUN_obj_dir",  DW_AT_SUN_obj_dir},
    {/* 206 */ "DW_AT_SUN_obj_file",  DW_AT_SUN_obj_file},
    {/* 207 */ "DW_AT_SUN_original_name",  DW_AT_SUN_original_name},
    {/* 208 */ "DW_AT_SUN_hwcprof_signature",  DW_AT_SUN_hwcprof_signature},
    {/* 209 */ "DW_AT_SUN_amd64_parmdump",  DW_AT_SUN_amd64_parmdump},
    {/* 210 */ "DW_AT_SUN_part_link_name",  DW_AT_SUN_part_link_name},
    {/* 211 */ "DW_AT_SUN_link_name",  DW_AT_SUN_link_name},
    {/* 212 */ "DW_AT_SUN_pass_with_const",  DW_AT_SUN_pass_with_const},
    {/* 213 */ "DW_AT_SUN_return_with_const",  DW_AT_SUN_return_with_const},
    {/* 214 */ "DW_AT_SUN_import_by_name",  DW_AT_SUN_import_by_name},
    {/* 215 */ "DW_AT_SUN_f90_pointer",  DW_AT_SUN_f90_pointer},
    {/* 216 */ "DW_AT_SUN_pass_by_ref",  DW_AT_SUN_pass_by_ref},
    {/* 217 */ "DW_AT_SUN_f90_allocatable",  DW_AT_SUN_f90_allocatable},
    {/* 218 */ "DW_AT_SUN_f90_assumed_shape_array",  DW_AT_SUN_f90_assumed_shape_array},
    {/* 219 */ "DW_AT_SUN_c_vla",  DW_AT_SUN_c_vla},
    {/* 220 */ "DW_AT_SUN_return_value_ptr",  DW_AT_SUN_return_value_ptr},
    {/* 221 */ "DW_AT_SUN_dtor_start",  DW_AT_SUN_dtor_start},
    {/* 222 */ "DW_AT_SUN_dtor_length",  DW_AT_SUN_dtor_length},
    {/* 223 */ "DW_AT_SUN_dtor_state_initial",  DW_AT_SUN_dtor_state_initial},
    {/* 224 */ "DW_AT_SUN_dtor_state_final",  DW_AT_SUN_dtor_state_final},
    {/* 225 */ "DW_AT_SUN_dtor_state_deltas",  DW_AT_SUN_dtor_state_deltas},
    {/* 226 */ "DW_AT_SUN_import_by_lname",  DW_AT_SUN_import_by_lname},
    {/* 227 */ "DW_AT_SUN_f90_use_only",  DW_AT_SUN_f90_use_only},
    {/* 228 */ "DW_AT_SUN_namelist_spec",  DW_AT_SUN_namelist_spec},
    {/* 229 */ "DW_AT_SUN_is_omp_child_func",  DW_AT_SUN_is_omp_child_func},
    {/* 230 */ "DW_AT_SUN_fortran_main_alias",  DW_AT_SUN_fortran_main_alias},
    {/* 231 */ "DW_AT_SUN_fortran_based",  DW_AT_SUN_fortran_based},
    {/* 232 */ "DW_AT_ALTIUM_loclist",  DW_AT_ALTIUM_loclist},
    {/* 233 */ "DW_AT_use_GNAT_descriptive_type",  DW_AT_use_GNAT_descriptive_type},
    {/* 234 */ "DW_AT_GNAT_descriptive_type",  DW_AT_GNAT_descriptive_type},
    {/* 235 */ "DW_AT_GNU_numerator",  DW_AT_GNU_numerator},
    {/* 236 */ "DW_AT_GNU_denominator",  DW_AT_GNU_denominator},
    {/* 237 */ "DW_AT_GNU_bias",  DW_AT_GNU_bias},
    {/* 238 */ "DW_AT_upc_threads_scaled",  DW_AT_upc_threads_scaled},
    {/* 239 */ "DW_AT_PGI_lbase",  DW_AT_PGI_lbase},
    {/* 240 */ "DW_AT_PGI_soffset",  DW_AT_PGI_soffset},
    {/* 241 */ "DW_AT_PGI_lstride",  DW_AT_PGI_lstride},
    {/* 242 */ "DW_AT_APPLE_optimized",  DW_AT_APPLE_optimized},
    {/* 243 */ "DW_AT_APPLE_flags",  DW_AT_APPLE_flags},
    {/* 244 */ "DW_AT_APPLE_isa",  DW_AT_APPLE_isa},
    {/* 245 */ "DW_AT_APPLE_block",  DW_AT_APPLE_block},
    /* Skipping alternate spelling of value 0x3fe4. DW_AT_APPLE_closure */
    {/* 246 */ "DW_AT_APPLE_major_runtime_vers",  DW_AT_APPLE_major_runtime_vers},
    /* Skipping alternate spelling of value 0x3fe5. DW_AT_APPLE_major_runtime_vers */
    {/* 247 */ "DW_AT_APPLE_runtime_class",  DW_AT_APPLE_runtime_class},
    /* Skipping alternate spelling of value 0x3fe6. DW_AT_APPLE_runtime_class */
    {/* 248 */ "DW_AT_APPLE_omit_frame_ptr",  DW_AT_APPLE_omit_frame_ptr},
    {/* 249 */ "DW_AT_hi_user",  DW_AT_hi_user}
    };

    const int last_entry = 250;
    /* find the entry */
    int r = find_entry(Dwarf_AT_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_OP_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_OP_n[] = {
    {/*   0 */ "DW_OP_addr",  DW_OP_addr},
    {/*   1 */ "DW_OP_deref",  DW_OP_deref},
    {/*   2 */ "DW_OP_const1u",  DW_OP_const1u},
    {/*   3 */ "DW_OP_const1s",  DW_OP_const1s},
    {/*   4 */ "DW_OP_const2u",  DW_OP_const2u},
    {/*   5 */ "DW_OP_const2s",  DW_OP_const2s},
    {/*   6 */ "DW_OP_const4u",  DW_OP_const4u},
    {/*   7 */ "DW_OP_const4s",  DW_OP_const4s},
    {/*   8 */ "DW_OP_const8u",  DW_OP_const8u},
    {/*   9 */ "DW_OP_const8s",  DW_OP_const8s},
    {/*  10 */ "DW_OP_constu",  DW_OP_constu},
    {/*  11 */ "DW_OP_consts",  DW_OP_consts},
    {/*  12 */ "DW_OP_dup",  DW_OP_dup},
    {/*  13 */ "DW_OP_drop",  DW_OP_drop},
    {/*  14 */ "DW_OP_over",  DW_OP_over},
    {/*  15 */ "DW_OP_pick",  DW_OP_pick},
    {/*  16 */ "DW_OP_swap",  DW_OP_swap},
    {/*  17 */ "DW_OP_rot",  DW_OP_rot},
    {/*  18 */ "DW_OP_xderef",  DW_OP_xderef},
    {/*  19 */ "DW_OP_abs",  DW_OP_abs},
    {/*  20 */ "DW_OP_and",  DW_OP_and},
    {/*  21 */ "DW_OP_div",  DW_OP_div},
    {/*  22 */ "DW_OP_minus",  DW_OP_minus},
    {/*  23 */ "DW_OP_mod",  DW_OP_mod},
    {/*  24 */ "DW_OP_mul",  DW_OP_mul},
    {/*  25 */ "DW_OP_neg",  DW_OP_neg},
    {/*  26 */ "DW_OP_not",  DW_OP_not},
    {/*  27 */ "DW_OP_or",  DW_OP_or},
    {/*  28 */ "DW_OP_plus",  DW_OP_plus},
    {/*  29 */ "DW_OP_plus_uconst",  DW_OP_plus_uconst},
    {/*  30 */ "DW_OP_shl",  DW_OP_shl},
    {/*  31 */ "DW_OP_shr",  DW_OP_shr},
    {/*  32 */ "DW_OP_shra",  DW_OP_shra},
    {/*  33 */ "DW_OP_xor",  DW_OP_xor},
    {/*  34 */ "DW_OP_bra",  DW_OP_bra},
    {/*  35 */ "DW_OP_eq",  DW_OP_eq},
    {/*  36 */ "DW_OP_ge",  DW_OP_ge},
    {/*  37 */ "DW_OP_gt",  DW_OP_gt},
    {/*  38 */ "DW_OP_le",  DW_OP_le},
    {/*  39 */ "DW_OP_lt",  DW_OP_lt},
    {/*  40 */ "DW_OP_ne",  DW_OP_ne},
    {/*  41 */ "DW_OP_skip",  DW_OP_skip},
    {/*  42 */ "DW_OP_lit0",  DW_OP_lit0},
    {/*  43 */ "DW_OP_lit1",  DW_OP_lit1},
    {/*  44 */ "DW_OP_lit2",  DW_OP_lit2},
    {/*  45 */ "DW_OP_lit3",  DW_OP_lit3},
    {/*  46 */ "DW_OP_lit4",  DW_OP_lit4},
    {/*  47 */ "DW_OP_lit5",  DW_OP_lit5},
    {/*  48 */ "DW_OP_lit6",  DW_OP_lit6},
    {/*  49 */ "DW_OP_lit7",  DW_OP_lit7},
    {/*  50 */ "DW_OP_lit8",  DW_OP_lit8},
    {/*  51 */ "DW_OP_lit9",  DW_OP_lit9},
    {/*  52 */ "DW_OP_lit10",  DW_OP_lit10},
    {/*  53 */ "DW_OP_lit11",  DW_OP_lit11},
    {/*  54 */ "DW_OP_lit12",  DW_OP_lit12},
    {/*  55 */ "DW_OP_lit13",  DW_OP_lit13},
    {/*  56 */ "DW_OP_lit14",  DW_OP_lit14},
    {/*  57 */ "DW_OP_lit15",  DW_OP_lit15},
    {/*  58 */ "DW_OP_lit16",  DW_OP_lit16},
    {/*  59 */ "DW_OP_lit17",  DW_OP_lit17},
    {/*  60 */ "DW_OP_lit18",  DW_OP_lit18},
    {/*  61 */ "DW_OP_lit19",  DW_OP_lit19},
    {/*  62 */ "DW_OP_lit20",  DW_OP_lit20},
    {/*  63 */ "DW_OP_lit21",  DW_OP_lit21},
    {/*  64 */ "DW_OP_lit22",  DW_OP_lit22},
    {/*  65 */ "DW_OP_lit23",  DW_OP_lit23},
    {/*  66 */ "DW_OP_lit24",  DW_OP_lit24},
    {/*  67 */ "DW_OP_lit25",  DW_OP_lit25},
    {/*  68 */ "DW_OP_lit26",  DW_OP_lit26},
    {/*  69 */ "DW_OP_lit27",  DW_OP_lit27},
    {/*  70 */ "DW_OP_lit28",  DW_OP_lit28},
    {/*  71 */ "DW_OP_lit29",  DW_OP_lit29},
    {/*  72 */ "DW_OP_lit30",  DW_OP_lit30},
    {/*  73 */ "DW_OP_lit31",  DW_OP_lit31},
    {/*  74 */ "DW_OP_reg0",  DW_OP_reg0},
    {/*  75 */ "DW_OP_reg1",  DW_OP_reg1},
    {/*  76 */ "DW_OP_reg2",  DW_OP_reg2},
    {/*  77 */ "DW_OP_reg3",  DW_OP_reg3},
    {/*  78 */ "DW_OP_reg4",  DW_OP_reg4},
    {/*  79 */ "DW_OP_reg5",  DW_OP_reg5},
    {/*  80 */ "DW_OP_reg6",  DW_OP_reg6},
    {/*  81 */ "DW_OP_reg7",  DW_OP_reg7},
    {/*  82 */ "DW_OP_reg8",  DW_OP_reg8},
    {/*  83 */ "DW_OP_reg9",  DW_OP_reg9},
    {/*  84 */ "DW_OP_reg10",  DW_OP_reg10},
    {/*  85 */ "DW_OP_reg11",  DW_OP_reg11},
    {/*  86 */ "DW_OP_reg12",  DW_OP_reg12},
    {/*  87 */ "DW_OP_reg13",  DW_OP_reg13},
    {/*  88 */ "DW_OP_reg14",  DW_OP_reg14},
    {/*  89 */ "DW_OP_reg15",  DW_OP_reg15},
    {/*  90 */ "DW_OP_reg16",  DW_OP_reg16},
    {/*  91 */ "DW_OP_reg17",  DW_OP_reg17},
    {/*  92 */ "DW_OP_reg18",  DW_OP_reg18},
    {/*  93 */ "DW_OP_reg19",  DW_OP_reg19},
    {/*  94 */ "DW_OP_reg20",  DW_OP_reg20},
    {/*  95 */ "DW_OP_reg21",  DW_OP_reg21},
    {/*  96 */ "DW_OP_reg22",  DW_OP_reg22},
    {/*  97 */ "DW_OP_reg23",  DW_OP_reg23},
    {/*  98 */ "DW_OP_reg24",  DW_OP_reg24},
    {/*  99 */ "DW_OP_reg25",  DW_OP_reg25},
    {/* 100 */ "DW_OP_reg26",  DW_OP_reg26},
    {/* 101 */ "DW_OP_reg27",  DW_OP_reg27},
    {/* 102 */ "DW_OP_reg28",  DW_OP_reg28},
    {/* 103 */ "DW_OP_reg29",  DW_OP_reg29},
    {/* 104 */ "DW_OP_reg30",  DW_OP_reg30},
    {/* 105 */ "DW_OP_reg31",  DW_OP_reg31},
    {/* 106 */ "DW_OP_breg0",  DW_OP_breg0},
    {/* 107 */ "DW_OP_breg1",  DW_OP_breg1},
    {/* 108 */ "DW_OP_breg2",  DW_OP_breg2},
    {/* 109 */ "DW_OP_breg3",  DW_OP_breg3},
    {/* 110 */ "DW_OP_breg4",  DW_OP_breg4},
    {/* 111 */ "DW_OP_breg5",  DW_OP_breg5},
    {/* 112 */ "DW_OP_breg6",  DW_OP_breg6},
    {/* 113 */ "DW_OP_breg7",  DW_OP_breg7},
    {/* 114 */ "DW_OP_breg8",  DW_OP_breg8},
    {/* 115 */ "DW_OP_breg9",  DW_OP_breg9},
    {/* 116 */ "DW_OP_breg10",  DW_OP_breg10},
    {/* 117 */ "DW_OP_breg11",  DW_OP_breg11},
    {/* 118 */ "DW_OP_breg12",  DW_OP_breg12},
    {/* 119 */ "DW_OP_breg13",  DW_OP_breg13},
    {/* 120 */ "DW_OP_breg14",  DW_OP_breg14},
    {/* 121 */ "DW_OP_breg15",  DW_OP_breg15},
    {/* 122 */ "DW_OP_breg16",  DW_OP_breg16},
    {/* 123 */ "DW_OP_breg17",  DW_OP_breg17},
    {/* 124 */ "DW_OP_breg18",  DW_OP_breg18},
    {/* 125 */ "DW_OP_breg19",  DW_OP_breg19},
    {/* 126 */ "DW_OP_breg20",  DW_OP_breg20},
    {/* 127 */ "DW_OP_breg21",  DW_OP_breg21},
    {/* 128 */ "DW_OP_breg22",  DW_OP_breg22},
    {/* 129 */ "DW_OP_breg23",  DW_OP_breg23},
    {/* 130 */ "DW_OP_breg24",  DW_OP_breg24},
    {/* 131 */ "DW_OP_breg25",  DW_OP_breg25},
    {/* 132 */ "DW_OP_breg26",  DW_OP_breg26},
    {/* 133 */ "DW_OP_breg27",  DW_OP_breg27},
    {/* 134 */ "DW_OP_breg28",  DW_OP_breg28},
    {/* 135 */ "DW_OP_breg29",  DW_OP_breg29},
    {/* 136 */ "DW_OP_breg30",  DW_OP_breg30},
    {/* 137 */ "DW_OP_breg31",  DW_OP_breg31},
    {/* 138 */ "DW_OP_regx",  DW_OP_regx},
    {/* 139 */ "DW_OP_fbreg",  DW_OP_fbreg},
    {/* 140 */ "DW_OP_bregx",  DW_OP_bregx},
    {/* 141 */ "DW_OP_piece",  DW_OP_piece},
    {/* 142 */ "DW_OP_deref_size",  DW_OP_deref_size},
    {/* 143 */ "DW_OP_xderef_size",  DW_OP_xderef_size},
    {/* 144 */ "DW_OP_nop",  DW_OP_nop},
    {/* 145 */ "DW_OP_push_object_address",  DW_OP_push_object_address},
    {/* 146 */ "DW_OP_call2",  DW_OP_call2},
    {/* 147 */ "DW_OP_call4",  DW_OP_call4},
    {/* 148 */ "DW_OP_call_ref",  DW_OP_call_ref},
    {/* 149 */ "DW_OP_form_tls_address",  DW_OP_form_tls_address},
    {/* 150 */ "DW_OP_call_frame_cfa",  DW_OP_call_frame_cfa},
    {/* 151 */ "DW_OP_bit_piece",  DW_OP_bit_piece},
    {/* 152 */ "DW_OP_implicit_value",  DW_OP_implicit_value},
    {/* 153 */ "DW_OP_stack_value",  DW_OP_stack_value},
    {/* 154 */ "DW_OP_implicit_pointer",  DW_OP_implicit_pointer},
    {/* 155 */ "DW_OP_addrx",  DW_OP_addrx},
    {/* 156 */ "DW_OP_constx",  DW_OP_constx},
    {/* 157 */ "DW_OP_entry_value",  DW_OP_entry_value},
    {/* 158 */ "DW_OP_const_type",  DW_OP_const_type},
    {/* 159 */ "DW_OP_regval_type",  DW_OP_regval_type},
    {/* 160 */ "DW_OP_deref_type",  DW_OP_deref_type},
    {/* 161 */ "DW_OP_xderef_type",  DW_OP_xderef_type},
    {/* 162 */ "DW_OP_convert",  DW_OP_convert},
    {/* 163 */ "DW_OP_reinterpret",  DW_OP_reinterpret},
    {/* 164 */ "DW_OP_GNU_push_tls_address",  DW_OP_GNU_push_tls_address},
    /* Skipping alternate spelling of value 0xe0. DW_OP_lo_user */
    /* Skipping alternate spelling of value 0xe0. DW_OP_HP_unknown */
    {/* 165 */ "DW_OP_HP_is_value",  DW_OP_HP_is_value},
    {/* 166 */ "DW_OP_HP_fltconst4",  DW_OP_HP_fltconst4},
    {/* 167 */ "DW_OP_HP_fltconst8",  DW_OP_HP_fltconst8},
    {/* 168 */ "DW_OP_HP_mod_range",  DW_OP_HP_mod_range},
    {/* 169 */ "DW_OP_HP_unmod_range",  DW_OP_HP_unmod_range},
    {/* 170 */ "DW_OP_HP_tls",  DW_OP_HP_tls},
    {/* 171 */ "DW_OP_INTEL_bit_piece",  DW_OP_INTEL_bit_piece},
    {/* 172 */ "DW_OP_GNU_uninit",  DW_OP_GNU_uninit},
    /* Skipping alternate spelling of value 0xf0. DW_OP_APPLE_uninit */
    {/* 173 */ "DW_OP_GNU_encoded_addr",  DW_OP_GNU_encoded_addr},
    {/* 174 */ "DW_OP_GNU_implicit_pointer",  DW_OP_GNU_implicit_pointer},
    {/* 175 */ "DW_OP_GNU_entry_value",  DW_OP_GNU_entry_value},
    {/* 176 */ "DW_OP_GNU_const_type",  DW_OP_GNU_const_type},
    {/* 177 */ "DW_OP_GNU_regval_type",  DW_OP_GNU_regval_type},
    {/* 178 */ "DW_OP_GNU_deref_type",  DW_OP_GNU_deref_type},
    {/* 179 */ "DW_OP_GNU_convert",  DW_OP_GNU_convert},
    {/* 180 */ "DW_OP_PGI_omp_thread_num",  DW_OP_PGI_omp_thread_num},
    {/* 181 */ "DW_OP_GNU_reinterpret",  DW_OP_GNU_reinterpret},
    {/* 182 */ "DW_OP_GNU_parameter_ref",  DW_OP_GNU_parameter_ref},
    {/* 183 */ "DW_OP_GNU_addr_index",  DW_OP_GNU_addr_index},
    {/* 184 */ "DW_OP_GNU_const_index",  DW_OP_GNU_const_index},
    {/* 185 */ "DW_OP_hi_user",  DW_OP_hi_user}
    };

    const int last_entry = 186;
    /* find the entry */
    int r = find_entry(Dwarf_OP_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_ATE_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_ATE_n[] = {
    {/*   0 */ "DW_ATE_address",  DW_ATE_address},
    {/*   1 */ "DW_ATE_boolean",  DW_ATE_boolean},
    {/*   2 */ "DW_ATE_complex_float",  DW_ATE_complex_float},
    {/*   3 */ "DW_ATE_float",  DW_ATE_float},
    {/*   4 */ "DW_ATE_signed",  DW_ATE_signed},
    {/*   5 */ "DW_ATE_signed_char",  DW_ATE_signed_char},
    {/*   6 */ "DW_ATE_unsigned",  DW_ATE_unsigned},
    {/*   7 */ "DW_ATE_unsigned_char",  DW_ATE_unsigned_char},
    {/*   8 */ "DW_ATE_imaginary_float",  DW_ATE_imaginary_float},
    {/*   9 */ "DW_ATE_packed_decimal",  DW_ATE_packed_decimal},
    {/*  10 */ "DW_ATE_numeric_string",  DW_ATE_numeric_string},
    {/*  11 */ "DW_ATE_edited",  DW_ATE_edited},
    {/*  12 */ "DW_ATE_signed_fixed",  DW_ATE_signed_fixed},
    {/*  13 */ "DW_ATE_unsigned_fixed",  DW_ATE_unsigned_fixed},
    {/*  14 */ "DW_ATE_decimal_float",  DW_ATE_decimal_float},
    {/*  15 */ "DW_ATE_UTF",  DW_ATE_UTF},
    {/*  16 */ "DW_ATE_UCS",  DW_ATE_UCS},
    {/*  17 */ "DW_ATE_ASCII",  DW_ATE_ASCII},
    {/*  18 */ "DW_ATE_ALTIUM_fract",  DW_ATE_ALTIUM_fract},
    /* Skipping alternate spelling of value 0x80. DW_ATE_lo_user */
    /* Skipping alternate spelling of value 0x80. DW_ATE_HP_float80 */
    {/*  19 */ "DW_ATE_ALTIUM_accum",  DW_ATE_ALTIUM_accum},
    /* Skipping alternate spelling of value 0x81. DW_ATE_HP_complex_float80 */
    {/*  20 */ "DW_ATE_HP_float128",  DW_ATE_HP_float128},
    {/*  21 */ "DW_ATE_HP_complex_float128",  DW_ATE_HP_complex_float128},
    {/*  22 */ "DW_ATE_HP_floathpintel",  DW_ATE_HP_floathpintel},
    {/*  23 */ "DW_ATE_HP_imaginary_float80",  DW_ATE_HP_imaginary_float80},
    {/*  24 */ "DW_ATE_HP_imaginary_float128",  DW_ATE_HP_imaginary_float128},
    {/*  25 */ "DW_ATE_SUN_interval_float",  DW_ATE_SUN_interval_float},
    {/*  26 */ "DW_ATE_SUN_imaginary_float",  DW_ATE_SUN_imaginary_float},
    {/*  27 */ "DW_ATE_hi_user",  DW_ATE_hi_user}
    };

    const int last_entry = 28;
    /* find the entry */
    int r = find_entry(Dwarf_ATE_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_DEFAULTED_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_DEFAULTED_n[] = {
    {/*   0 */ "DW_DEFAULTED_no",  DW_DEFAULTED_no},
    {/*   1 */ "DW_DEFAULTED_in_class",  DW_DEFAULTED_in_class},
    {/*   2 */ "DW_DEFAULTED_out_of_class",  DW_DEFAULTED_out_of_class}
    };

    const int last_entry = 3;
    /* find the entry */
    int r = find_entry(Dwarf_DEFAULTED_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_IDX_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_IDX_n[] = {
    {/*   0 */ "DW_IDX_compile_unit",  DW_IDX_compile_unit},
    {/*   1 */ "DW_IDX_type_unit",  DW_IDX_type_unit},
    {/*   2 */ "DW_IDX_die_offset",  DW_IDX_die_offset},
    {/*   3 */ "DW_IDX_parent",  DW_IDX_parent},
    {/*   4 */ "DW_IDX_type_hash",  DW_IDX_type_hash},
    {/*   5 */ "DW_IDX_hi_user",  DW_IDX_hi_user},
    {/*   6 */ "DW_IDX_lo_user",  DW_IDX_lo_user}
    };

    const int last_entry = 7;
    /* find the entry */
    int r = find_entry(Dwarf_IDX_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_LLEX_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_LLEX_n[] = {
    {/*   0 */ "DW_LLEX_end_of_list_entry",  DW_LLEX_end_of_list_entry},
    {/*   1 */ "DW_LLEX_base_address_selection_entry",  DW_LLEX_base_address_selection_entry},
    {/*   2 */ "DW_LLEX_start_end_entry",  DW_LLEX_start_end_entry},
    {/*   3 */ "DW_LLEX_start_length_entry",  DW_LLEX_start_length_entry},
    {/*   4 */ "DW_LLEX_offset_pair_entry",  DW_LLEX_offset_pair_entry}
    };

    const int last_entry = 5;
    /* find the entry */
    int r = find_entry(Dwarf_LLEX_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_LLE_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_LLE_n[] = {
    {/*   0 */ "DW_LLE_end_of_list",  DW_LLE_end_of_list},
    {/*   1 */ "DW_LLE_base_addressx",  DW_LLE_base_addressx},
    {/*   2 */ "DW_LLE_startx_endx",  DW_LLE_startx_endx},
    {/*   3 */ "DW_LLE_startx_length",  DW_LLE_startx_length},
    {/*   4 */ "DW_LLE_offset_pair",  DW_LLE_offset_pair},
    {/*   5 */ "DW_LLE_default_location",  DW_LLE_default_location},
    {/*   6 */ "DW_LLE_base_address",  DW_LLE_base_address},
    {/*   7 */ "DW_LLE_start_end",  DW_LLE_start_end},
    {/*   8 */ "DW_LLE_start_length",  DW_LLE_start_length}
    };

    const int last_entry = 9;
    /* find the entry */
    int r = find_entry(Dwarf_LLE_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_RLE_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_RLE_n[] = {
    {/*   0 */ "DW_RLE_end_of_list",  DW_RLE_end_of_list},
    {/*   1 */ "DW_RLE_base_addressx",  DW_RLE_base_addressx},
    {/*   2 */ "DW_RLE_startx_endx",  DW_RLE_startx_endx},
    {/*   3 */ "DW_RLE_startx_length",  DW_RLE_startx_length},
    {/*   4 */ "DW_RLE_offset_pair",  DW_RLE_offset_pair},
    {/*   5 */ "DW_RLE_base_address",  DW_RLE_base_address},
    {/*   6 */ "DW_RLE_start_end",  DW_RLE_start_end},
    {/*   7 */ "DW_RLE_start_length",  DW_RLE_start_length}
    };

    const int last_entry = 8;
    /* find the entry */
    int r = find_entry(Dwarf_RLE_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_UT_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_UT_n[] = {
    {/*   0 */ "DW_UT_compile",  DW_UT_compile},
    {/*   1 */ "DW_UT_type",  DW_UT_type},
    {/*   2 */ "DW_UT_partial",  DW_UT_partial},
    {/*   3 */ "DW_UT_skeleton",  DW_UT_skeleton},
    {/*   4 */ "DW_UT_split_compile",  DW_UT_split_compile},
    {/*   5 */ "DW_UT_split_type",  DW_UT_split_type},
    {/*   6 */ "DW_UT_lo_user",  DW_UT_lo_user},
    {/*   7 */ "DW_UT_hi_user",  DW_UT_hi_user}
    };

    const int last_entry = 8;
    /* find the entry */
    int r = find_entry(Dwarf_UT_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_SECT_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_SECT_n[] = {
    {/*   0 */ "DW_SECT_INFO",  DW_SECT_INFO},
    {/*   1 */ "DW_SECT_TYPES",  DW_SECT_TYPES},
    {/*   2 */ "DW_SECT_ABBREV",  DW_SECT_ABBREV},
    {/*   3 */ "DW_SECT_LINE",  DW_SECT_LINE},
    {/*   4 */ "DW_SECT_LOCLISTS",  DW_SECT_LOCLISTS},
    {/*   5 */ "DW_SECT_STR_OFFSETS",  DW_SECT_STR_OFFSETS},
    {/*   6 */ "DW_SECT_MACRO",  DW_SECT_MACRO},
    {/*   7 */ "DW_SECT_RNGLISTS",  DW_SECT_RNGLISTS}
    };

    const int last_entry = 8;
    /* find the entry */
    int r = find_entry(Dwarf_SECT_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_DS_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_DS_n[] = {
    {/*   0 */ "DW_DS_unsigned",  DW_DS_unsigned},
    {/*   1 */ "DW_DS_leading_overpunch",  DW_DS_leading_overpunch},
    {/*   2 */ "DW_DS_trailing_overpunch",  DW_DS_trailing_overpunch},
    {/*   3 */ "DW_DS_leading_separate",  DW_DS_leading_separate},
    {/*   4 */ "DW_DS_trailing_separate",  DW_DS_trailing_separate}
    };

    const int last_entry = 5;
    /* find the entry */
    int r = find_entry(Dwarf_DS_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_END_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_END_n[] = {
    {/*   0 */ "DW_END_default",  DW_END_default},
    {/*   1 */ "DW_END_big",  DW_END_big},
    {/*   2 */ "DW_END_little",  DW_END_little},
    {/*   3 */ "DW_END_lo_user",  DW_END_lo_user},
    {/*   4 */ "DW_END_hi_user",  DW_END_hi_user}
    };

    const int last_entry = 5;
    /* find the entry */
    int r = find_entry(Dwarf_END_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_ATCF_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_ATCF_n[] = {
    {/*   0 */ "DW_ATCF_lo_user",  DW_ATCF_lo_user},
    {/*   1 */ "DW_ATCF_SUN_mop_bitfield",  DW_ATCF_SUN_mop_bitfield},
    {/*   2 */ "DW_ATCF_SUN_mop_spill",  DW_ATCF_SUN_mop_spill},
    {/*   3 */ "DW_ATCF_SUN_mop_scopy",  DW_ATCF_SUN_mop_scopy},
    {/*   4 */ "DW_ATCF_SUN_func_start",  DW_ATCF_SUN_func_start},
    {/*   5 */ "DW_ATCF_SUN_end_ctors",  DW_ATCF_SUN_end_ctors},
    {/*   6 */ "DW_ATCF_SUN_branch_target",  DW_ATCF_SUN_branch_target},
    {/*   7 */ "DW_ATCF_SUN_mop_stack_probe",  DW_ATCF_SUN_mop_stack_probe},
    {/*   8 */ "DW_ATCF_SUN_func_epilog",  DW_ATCF_SUN_func_epilog},
    {/*   9 */ "DW_ATCF_hi_user",  DW_ATCF_hi_user}
    };

    const int last_entry = 10;
    /* find the entry */
    int r = find_entry(Dwarf_ATCF_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_ACCESS_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_ACCESS_n[] = {
    {/*   0 */ "DW_ACCESS_public",  DW_ACCESS_public},
    {/*   1 */ "DW_ACCESS_protected",  DW_ACCESS_protected},
    {/*   2 */ "DW_ACCESS_private",  DW_ACCESS_private}
    };

    const int last_entry = 3;
    /* find the entry */
    int r = find_entry(Dwarf_ACCESS_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_VIS_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_VIS_n[] = {
    {/*   0 */ "DW_VIS_local",  DW_VIS_local},
    {/*   1 */ "DW_VIS_exported",  DW_VIS_exported},
    {/*   2 */ "DW_VIS_qualified",  DW_VIS_qualified}
    };

    const int last_entry = 3;
    /* find the entry */
    int r = find_entry(Dwarf_VIS_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_VIRTUALITY_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_VIRTUALITY_n[] = {
    {/*   0 */ "DW_VIRTUALITY_none",  DW_VIRTUALITY_none},
    {/*   1 */ "DW_VIRTUALITY_virtual",  DW_VIRTUALITY_virtual},
    {/*   2 */ "DW_VIRTUALITY_pure_virtual",  DW_VIRTUALITY_pure_virtual}
    };

    const int last_entry = 3;
    /* find the entry */
    int r = find_entry(Dwarf_VIRTUALITY_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_LANG_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_LANG_n[] = {
    {/*   0 */ "DW_LANG_C89",  DW_LANG_C89},
    {/*   1 */ "DW_LANG_C",  DW_LANG_C},
    {/*   2 */ "DW_LANG_Ada83",  DW_LANG_Ada83},
    {/*   3 */ "DW_LANG_C_plus_plus",  DW_LANG_C_plus_plus},
    {/*   4 */ "DW_LANG_Cobol74",  DW_LANG_Cobol74},
    {/*   5 */ "DW_LANG_Cobol85",  DW_LANG_Cobol85},
    {/*   6 */ "DW_LANG_Fortran77",  DW_LANG_Fortran77},
    {/*   7 */ "DW_LANG_Fortran90",  DW_LANG_Fortran90},
    {/*   8 */ "DW_LANG_Pascal83",  DW_LANG_Pascal83},
    {/*   9 */ "DW_LANG_Modula2",  DW_LANG_Modula2},
    {/*  10 */ "DW_LANG_Java",  DW_LANG_Java},
    {/*  11 */ "DW_LANG_C99",  DW_LANG_C99},
    {/*  12 */ "DW_LANG_Ada95",  DW_LANG_Ada95},
    {/*  13 */ "DW_LANG_Fortran95",  DW_LANG_Fortran95},
    {/*  14 */ "DW_LANG_PLI",  DW_LANG_PLI},
    {/*  15 */ "DW_LANG_ObjC",  DW_LANG_ObjC},
    {/*  16 */ "DW_LANG_ObjC_plus_plus",  DW_LANG_ObjC_plus_plus},
    {/*  17 */ "DW_LANG_UPC",  DW_LANG_UPC},
    {/*  18 */ "DW_LANG_D",  DW_LANG_D},
    {/*  19 */ "DW_LANG_Python",  DW_LANG_Python},
    {/*  20 */ "DW_LANG_OpenCL",  DW_LANG_OpenCL},
    {/*  21 */ "DW_LANG_Go",  DW_LANG_Go},
    {/*  22 */ "DW_LANG_Modula3",  DW_LANG_Modula3},
    {/*  23 */ "DW_LANG_Haskel",  DW_LANG_Haskel},
    {/*  24 */ "DW_LANG_C_plus_plus_03",  DW_LANG_C_plus_plus_03},
    {/*  25 */ "DW_LANG_C_plus_plus_11",  DW_LANG_C_plus_plus_11},
    {/*  26 */ "DW_LANG_OCaml",  DW_LANG_OCaml},
    {/*  27 */ "DW_LANG_Rust",  DW_LANG_Rust},
    {/*  28 */ "DW_LANG_C11",  DW_LANG_C11},
    {/*  29 */ "DW_LANG_Swift",  DW_LANG_Swift},
    {/*  30 */ "DW_LANG_Julia",  DW_LANG_Julia},
    {/*  31 */ "DW_LANG_Dylan",  DW_LANG_Dylan},
    {/*  32 */ "DW_LANG_C_plus_plus_14",  DW_LANG_C_plus_plus_14},
    {/*  33 */ "DW_LANG_Fortran03",  DW_LANG_Fortran03},
    {/*  34 */ "DW_LANG_Fortran08",  DW_LANG_Fortran08},
    {/*  35 */ "DW_LANG_RenderScript",  DW_LANG_RenderScript},
    {/*  36 */ "DW_LANG_BLISS",  DW_LANG_BLISS},
    {/*  37 */ "DW_LANG_lo_user",  DW_LANG_lo_user},
    {/*  38 */ "DW_LANG_Mips_Assembler",  DW_LANG_Mips_Assembler},
    {/*  39 */ "DW_LANG_Upc",  DW_LANG_Upc},
    {/*  40 */ "DW_LANG_SUN_Assembler",  DW_LANG_SUN_Assembler},
    {/*  41 */ "DW_LANG_ALTIUM_Assembler",  DW_LANG_ALTIUM_Assembler},
    {/*  42 */ "DW_LANG_hi_user",  DW_LANG_hi_user}
    };

    const int last_entry = 43;
    /* find the entry */
    int r = find_entry(Dwarf_LANG_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_ID_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_ID_n[] = {
    {/*   0 */ "DW_ID_case_sensitive",  DW_ID_case_sensitive},
    {/*   1 */ "DW_ID_up_case",  DW_ID_up_case},
    {/*   2 */ "DW_ID_down_case",  DW_ID_down_case},
    {/*   3 */ "DW_ID_case_insensitive",  DW_ID_case_insensitive}
    };

    const int last_entry = 4;
    /* find the entry */
    int r = find_entry(Dwarf_ID_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_CC_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_CC_n[] = {
    {/*   0 */ "DW_CC_normal",  DW_CC_normal},
    {/*   1 */ "DW_CC_program",  DW_CC_program},
    {/*   2 */ "DW_CC_nocall",  DW_CC_nocall},
    {/*   3 */ "DW_CC_pass_by_reference",  DW_CC_pass_by_reference},
    {/*   4 */ "DW_CC_pass_by_value",  DW_CC_pass_by_value},
    {/*   5 */ "DW_CC_lo_user",  DW_CC_lo_user},
    /* Skipping alternate spelling of value 0x40. DW_CC_GNU_renesas_sh */
    {/*   6 */ "DW_CC_GNU_borland_fastcall_i386",  DW_CC_GNU_borland_fastcall_i386},
    {/*   7 */ "DW_CC_ALTIUM_interrupt",  DW_CC_ALTIUM_interrupt},
    {/*   8 */ "DW_CC_ALTIUM_near_system_stack",  DW_CC_ALTIUM_near_system_stack},
    {/*   9 */ "DW_CC_ALTIUM_near_user_stack",  DW_CC_ALTIUM_near_user_stack},
    {/*  10 */ "DW_CC_ALTIUM_huge_user_stack",  DW_CC_ALTIUM_huge_user_stack},
    {/*  11 */ "DW_CC_hi_user",  DW_CC_hi_user}
    };

    const int last_entry = 12;
    /* find the entry */
    int r = find_entry(Dwarf_CC_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_INL_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_INL_n[] = {
    {/*   0 */ "DW_INL_not_inlined",  DW_INL_not_inlined},
    {/*   1 */ "DW_INL_inlined",  DW_INL_inlined},
    {/*   2 */ "DW_INL_declared_not_inlined",  DW_INL_declared_not_inlined},
    {/*   3 */ "DW_INL_declared_inlined",  DW_INL_declared_inlined}
    };

    const int last_entry = 4;
    /* find the entry */
    int r = find_entry(Dwarf_INL_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_ORD_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_ORD_n[] = {
    {/*   0 */ "DW_ORD_row_major",  DW_ORD_row_major},
    {/*   1 */ "DW_ORD_col_major",  DW_ORD_col_major}
    };

    const int last_entry = 2;
    /* find the entry */
    int r = find_entry(Dwarf_ORD_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_DSC_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_DSC_n[] = {
    {/*   0 */ "DW_DSC_label",  DW_DSC_label},
    {/*   1 */ "DW_DSC_range",  DW_DSC_range}
    };

    const int last_entry = 2;
    /* find the entry */
    int r = find_entry(Dwarf_DSC_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_LNCT_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_LNCT_n[] = {
    {/*   0 */ "DW_LNCT_path",  DW_LNCT_path},
    {/*   1 */ "DW_LNCT_directory_index",  DW_LNCT_directory_index},
    {/*   2 */ "DW_LNCT_timestamp",  DW_LNCT_timestamp},
    {/*   3 */ "DW_LNCT_size",  DW_LNCT_size},
    {/*   4 */ "DW_LNCT_MD5",  DW_LNCT_MD5},
    {/*   5 */ "DW_LNCT_GNU_subprogram_name",  DW_LNCT_GNU_subprogram_name},
    {/*   6 */ "DW_LNCT_GNU_decl_file",  DW_LNCT_GNU_decl_file},
    {/*   7 */ "DW_LNCT_GNU_decl_line",  DW_LNCT_GNU_decl_line},
    {/*   8 */ "DW_LNCT_lo_user",  DW_LNCT_lo_user},
    {/*   9 */ "DW_LNCT_hi_user",  DW_LNCT_hi_user}
    };

    const int last_entry = 10;
    /* find the entry */
    int r = find_entry(Dwarf_LNCT_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_LNS_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_LNS_n[] = {
    {/*   0 */ "DW_LNS_copy",  DW_LNS_copy},
    {/*   1 */ "DW_LNS_advance_pc",  DW_LNS_advance_pc},
    {/*   2 */ "DW_LNS_advance_line",  DW_LNS_advance_line},
    {/*   3 */ "DW_LNS_set_file",  DW_LNS_set_file},
    {/*   4 */ "DW_LNS_set_column",  DW_LNS_set_column},
    {/*   5 */ "DW_LNS_negate_stmt",  DW_LNS_negate_stmt},
    {/*   6 */ "DW_LNS_set_basic_block",  DW_LNS_set_basic_block},
    {/*   7 */ "DW_LNS_const_add_pc",  DW_LNS_const_add_pc},
    {/*   8 */ "DW_LNS_fixed_advance_pc",  DW_LNS_fixed_advance_pc},
    {/*   9 */ "DW_LNS_set_prologue_end",  DW_LNS_set_prologue_end},
    {/*  10 */ "DW_LNS_set_epilogue_begin",  DW_LNS_set_epilogue_begin},
    {/*  11 */ "DW_LNS_set_isa",  DW_LNS_set_isa},
    {/*  12 */ "DW_LNS_set_address_from_logical",  DW_LNS_set_address_from_logical},
    /* Skipping alternate spelling of value 0xd. DW_LNS_set_subprogram */
    {/*  13 */ "DW_LNS_inlined_call",  DW_LNS_inlined_call},
    {/*  14 */ "DW_LNS_pop_context",  DW_LNS_pop_context}
    };

    const int last_entry = 15;
    /* find the entry */
    int r = find_entry(Dwarf_LNS_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_LNE_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_LNE_n[] = {
    {/*   0 */ "DW_LNE_end_sequence",  DW_LNE_end_sequence},
    {/*   1 */ "DW_LNE_set_address",  DW_LNE_set_address},
    {/*   2 */ "DW_LNE_define_file",  DW_LNE_define_file},
    {/*   3 */ "DW_LNE_set_discriminator",  DW_LNE_set_discriminator},
    {/*   4 */ "DW_LNE_HP_negate_is_UV_update",  DW_LNE_HP_negate_is_UV_update},
    {/*   5 */ "DW_LNE_HP_push_context",  DW_LNE_HP_push_context},
    {/*   6 */ "DW_LNE_HP_pop_context",  DW_LNE_HP_pop_context},
    {/*   7 */ "DW_LNE_HP_set_file_line_column",  DW_LNE_HP_set_file_line_column},
    {/*   8 */ "DW_LNE_HP_set_routine_name",  DW_LNE_HP_set_routine_name},
    {/*   9 */ "DW_LNE_HP_set_sequence",  DW_LNE_HP_set_sequence},
    {/*  10 */ "DW_LNE_HP_negate_post_semantics",  DW_LNE_HP_negate_post_semantics},
    {/*  11 */ "DW_LNE_HP_negate_function_exit",  DW_LNE_HP_negate_function_exit},
    {/*  12 */ "DW_LNE_HP_negate_front_end_logical",  DW_LNE_HP_negate_front_end_logical},
    {/*  13 */ "DW_LNE_HP_define_proc",  DW_LNE_HP_define_proc},
    {/*  14 */ "DW_LNE_HP_source_file_correlation",  DW_LNE_HP_source_file_correlation},
    /* Skipping alternate spelling of value 0x80. DW_LNE_lo_user */
    {/*  15 */ "DW_LNE_hi_user",  DW_LNE_hi_user}
    };

    const int last_entry = 16;
    /* find the entry */
    int r = find_entry(Dwarf_LNE_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_ISA_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_ISA_n[] = {
    {/*   0 */ "DW_ISA_UNKNOWN",  DW_ISA_UNKNOWN},
    {/*   1 */ "DW_ISA_ARM_thumb",  DW_ISA_ARM_thumb},
    {/*   2 */ "DW_ISA_ARM_arm",  DW_ISA_ARM_arm}
    };

    const int last_entry = 3;
    /* find the entry */
    int r = find_entry(Dwarf_ISA_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_MACRO_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_MACRO_n[] = {
    {/*   0 */ "DW_MACRO_define",  DW_MACRO_define},
    {/*   1 */ "DW_MACRO_undef",  DW_MACRO_undef},
    {/*   2 */ "DW_MACRO_start_file",  DW_MACRO_start_file},
    {/*   3 */ "DW_MACRO_end_file",  DW_MACRO_end_file},
    {/*   4 */ "DW_MACRO_define_strp",  DW_MACRO_define_strp},
    {/*   5 */ "DW_MACRO_undef_strp",  DW_MACRO_undef_strp},
    {/*   6 */ "DW_MACRO_import",  DW_MACRO_import},
    {/*   7 */ "DW_MACRO_define_sup",  DW_MACRO_define_sup},
    {/*   8 */ "DW_MACRO_undef_sup",  DW_MACRO_undef_sup},
    {/*   9 */ "DW_MACRO_import_sup",  DW_MACRO_import_sup},
    {/*  10 */ "DW_MACRO_define_strx",  DW_MACRO_define_strx},
    {/*  11 */ "DW_MACRO_undef_strx",  DW_MACRO_undef_strx},
    {/*  12 */ "DW_MACRO_lo_user",  DW_MACRO_lo_user},
    {/*  13 */ "DW_MACRO_hi_user",  DW_MACRO_hi_user}
    };

    const int last_entry = 14;
    /* find the entry */
    int r = find_entry(Dwarf_MACRO_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_MACINFO_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_MACINFO_n[] = {
    {/*   0 */ "DW_MACINFO_define",  DW_MACINFO_define},
    {/*   1 */ "DW_MACINFO_undef",  DW_MACINFO_undef},
    {/*   2 */ "DW_MACINFO_start_file",  DW_MACINFO_start_file},
    {/*   3 */ "DW_MACINFO_end_file",  DW_MACINFO_end_file},
    {/*   4 */ "DW_MACINFO_vendor_ext",  DW_MACINFO_vendor_ext}
    };

    const int last_entry = 5;
    /* find the entry */
    int r = find_entry(Dwarf_MACINFO_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_CFA_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_CFA_n[] = {
    {/*   0 */ "DW_CFA_extended",  DW_CFA_extended},
    /* Skipping alternate spelling of value 0x0. DW_CFA_nop */
    {/*   1 */ "DW_CFA_set_loc",  DW_CFA_set_loc},
    {/*   2 */ "DW_CFA_advance_loc1",  DW_CFA_advance_loc1},
    {/*   3 */ "DW_CFA_advance_loc2",  DW_CFA_advance_loc2},
    {/*   4 */ "DW_CFA_advance_loc4",  DW_CFA_advance_loc4},
    {/*   5 */ "DW_CFA_offset_extended",  DW_CFA_offset_extended},
    {/*   6 */ "DW_CFA_restore_extended",  DW_CFA_restore_extended},
    {/*   7 */ "DW_CFA_undefined",  DW_CFA_undefined},
    {/*   8 */ "DW_CFA_same_value",  DW_CFA_same_value},
    {/*   9 */ "DW_CFA_register",  DW_CFA_register},
    {/*  10 */ "DW_CFA_remember_state",  DW_CFA_remember_state},
    {/*  11 */ "DW_CFA_restore_state",  DW_CFA_restore_state},
    {/*  12 */ "DW_CFA_def_cfa",  DW_CFA_def_cfa},
    {/*  13 */ "DW_CFA_def_cfa_register",  DW_CFA_def_cfa_register},
    {/*  14 */ "DW_CFA_def_cfa_offset",  DW_CFA_def_cfa_offset},
    {/*  15 */ "DW_CFA_def_cfa_expression",  DW_CFA_def_cfa_expression},
    {/*  16 */ "DW_CFA_expression",  DW_CFA_expression},
    {/*  17 */ "DW_CFA_offset_extended_sf",  DW_CFA_offset_extended_sf},
    {/*  18 */ "DW_CFA_def_cfa_sf",  DW_CFA_def_cfa_sf},
    {/*  19 */ "DW_CFA_def_cfa_offset_sf",  DW_CFA_def_cfa_offset_sf},
    {/*  20 */ "DW_CFA_val_offset",  DW_CFA_val_offset},
    {/*  21 */ "DW_CFA_val_offset_sf",  DW_CFA_val_offset_sf},
    {/*  22 */ "DW_CFA_val_expression",  DW_CFA_val_expression},
    {/*  23 */ "DW_CFA_lo_user",  DW_CFA_lo_user},
    /* Skipping alternate spelling of value 0x1c. DW_CFA_low_user */
    {/*  24 */ "DW_CFA_MIPS_advance_loc8",  DW_CFA_MIPS_advance_loc8},
    {/*  25 */ "DW_CFA_GNU_window_save",  DW_CFA_GNU_window_save},
    {/*  26 */ "DW_CFA_GNU_args_size",  DW_CFA_GNU_args_size},
    {/*  27 */ "DW_CFA_GNU_negative_offset_extended",  DW_CFA_GNU_negative_offset_extended},
    {/*  28 */ "DW_CFA_METAWARE_info",  DW_CFA_METAWARE_info},
    {/*  29 */ "DW_CFA_high_user",  DW_CFA_high_user},
    {/*  30 */ "DW_CFA_advance_loc",  DW_CFA_advance_loc},
    {/*  31 */ "DW_CFA_offset",  DW_CFA_offset},
    {/*  32 */ "DW_CFA_restore",  DW_CFA_restore}
    };

    const int last_entry = 33;
    /* find the entry */
    int r = find_entry(Dwarf_CFA_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_EH_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_EH_n[] = {
    {/*   0 */ "DW_EH_PE_absptr",  DW_EH_PE_absptr},
    {/*   1 */ "DW_EH_PE_uleb128",  DW_EH_PE_uleb128},
    {/*   2 */ "DW_EH_PE_udata2",  DW_EH_PE_udata2},
    {/*   3 */ "DW_EH_PE_udata4",  DW_EH_PE_udata4},
    {/*   4 */ "DW_EH_PE_udata8",  DW_EH_PE_udata8},
    {/*   5 */ "DW_EH_PE_sleb128",  DW_EH_PE_sleb128},
    {/*   6 */ "DW_EH_PE_sdata2",  DW_EH_PE_sdata2},
    {/*   7 */ "DW_EH_PE_sdata4",  DW_EH_PE_sdata4},
    {/*   8 */ "DW_EH_PE_sdata8",  DW_EH_PE_sdata8},
    {/*   9 */ "DW_EH_PE_pcrel",  DW_EH_PE_pcrel},
    {/*  10 */ "DW_EH_PE_textrel",  DW_EH_PE_textrel},
    {/*  11 */ "DW_EH_PE_datarel",  DW_EH_PE_datarel},
    {/*  12 */ "DW_EH_PE_funcrel",  DW_EH_PE_funcrel},
    {/*  13 */ "DW_EH_PE_aligned",  DW_EH_PE_aligned},
    {/*  14 */ "DW_EH_PE_omit",  DW_EH_PE_omit}
    };

    const int last_entry = 15;
    /* find the entry */
    int r = find_entry(Dwarf_EH_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_FRAME_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_FRAME_n[] = {
    {/*   0 */ "DW_FRAME_CFA_COL",  DW_FRAME_CFA_COL},
    /* Skipping alternate spelling of value 0x0. DW_FRAME_LAST_REG_NUM */
    /* Skipping alternate spelling of value 0x0. DW_FRAME_RA_COL */
    /* Skipping alternate spelling of value 0x0. DW_FRAME_STATIC_LINK */
    {/*   1 */ "DW_FRAME_REG1",  DW_FRAME_REG1},
    {/*   2 */ "DW_FRAME_REG2",  DW_FRAME_REG2},
    {/*   3 */ "DW_FRAME_REG3",  DW_FRAME_REG3},
    {/*   4 */ "DW_FRAME_REG4",  DW_FRAME_REG4},
    {/*   5 */ "DW_FRAME_REG5",  DW_FRAME_REG5},
    {/*   6 */ "DW_FRAME_REG6",  DW_FRAME_REG6},
    {/*   7 */ "DW_FRAME_REG7",  DW_FRAME_REG7},
    {/*   8 */ "DW_FRAME_REG8",  DW_FRAME_REG8},
    {/*   9 */ "DW_FRAME_REG9",  DW_FRAME_REG9},
    {/*  10 */ "DW_FRAME_REG10",  DW_FRAME_REG10},
    {/*  11 */ "DW_FRAME_REG11",  DW_FRAME_REG11},
    {/*  12 */ "DW_FRAME_REG12",  DW_FRAME_REG12},
    {/*  13 */ "DW_FRAME_REG13",  DW_FRAME_REG13},
    {/*  14 */ "DW_FRAME_REG14",  DW_FRAME_REG14},
    {/*  15 */ "DW_FRAME_REG15",  DW_FRAME_REG15},
    {/*  16 */ "DW_FRAME_REG16",  DW_FRAME_REG16},
    {/*  17 */ "DW_FRAME_REG17",  DW_FRAME_REG17},
    {/*  18 */ "DW_FRAME_REG18",  DW_FRAME_REG18},
    {/*  19 */ "DW_FRAME_REG19",  DW_FRAME_REG19},
    {/*  20 */ "DW_FRAME_REG20",  DW_FRAME_REG20},
    {/*  21 */ "DW_FRAME_REG21",  DW_FRAME_REG21},
    {/*  22 */ "DW_FRAME_REG22",  DW_FRAME_REG22},
    {/*  23 */ "DW_FRAME_REG23",  DW_FRAME_REG23},
    {/*  24 */ "DW_FRAME_REG24",  DW_FRAME_REG24},
    {/*  25 */ "DW_FRAME_REG25",  DW_FRAME_REG25},
    {/*  26 */ "DW_FRAME_REG26",  DW_FRAME_REG26},
    {/*  27 */ "DW_FRAME_REG27",  DW_FRAME_REG27},
    {/*  28 */ "DW_FRAME_REG28",  DW_FRAME_REG28},
    {/*  29 */ "DW_FRAME_REG29",  DW_FRAME_REG29},
    {/*  30 */ "DW_FRAME_REG30",  DW_FRAME_REG30},
    {/*  31 */ "DW_FRAME_REG31",  DW_FRAME_REG31},
    {/*  32 */ "DW_FRAME_FREG0",  DW_FRAME_FREG0},
    {/*  33 */ "DW_FRAME_FREG1",  DW_FRAME_FREG1},
    {/*  34 */ "DW_FRAME_FREG2",  DW_FRAME_FREG2},
    {/*  35 */ "DW_FRAME_FREG3",  DW_FRAME_FREG3},
    {/*  36 */ "DW_FRAME_FREG4",  DW_FRAME_FREG4},
    {/*  37 */ "DW_FRAME_FREG5",  DW_FRAME_FREG5},
    {/*  38 */ "DW_FRAME_FREG6",  DW_FRAME_FREG6},
    {/*  39 */ "DW_FRAME_FREG7",  DW_FRAME_FREG7},
    {/*  40 */ "DW_FRAME_FREG8",  DW_FRAME_FREG8},
    {/*  41 */ "DW_FRAME_FREG9",  DW_FRAME_FREG9},
    {/*  42 */ "DW_FRAME_FREG10",  DW_FRAME_FREG10},
    {/*  43 */ "DW_FRAME_FREG11",  DW_FRAME_FREG11},
    {/*  44 */ "DW_FRAME_FREG12",  DW_FRAME_FREG12},
    {/*  45 */ "DW_FRAME_FREG13",  DW_FRAME_FREG13},
    {/*  46 */ "DW_FRAME_FREG14",  DW_FRAME_FREG14},
    {/*  47 */ "DW_FRAME_FREG15",  DW_FRAME_FREG15},
    {/*  48 */ "DW_FRAME_FREG16",  DW_FRAME_FREG16},
    {/*  49 */ "DW_FRAME_FREG17",  DW_FRAME_FREG17},
    {/*  50 */ "DW_FRAME_FREG18",  DW_FRAME_FREG18},
    {/*  51 */ "DW_FRAME_FREG19",  DW_FRAME_FREG19},
    {/*  52 */ "DW_FRAME_FREG20",  DW_FRAME_FREG20},
    {/*  53 */ "DW_FRAME_FREG21",  DW_FRAME_FREG21},
    {/*  54 */ "DW_FRAME_FREG22",  DW_FRAME_FREG22},
    {/*  55 */ "DW_FRAME_FREG23",  DW_FRAME_FREG23},
    {/*  56 */ "DW_FRAME_FREG24",  DW_FRAME_FREG24},
    {/*  57 */ "DW_FRAME_FREG25",  DW_FRAME_FREG25},
    {/*  58 */ "DW_FRAME_FREG26",  DW_FRAME_FREG26},
    {/*  59 */ "DW_FRAME_FREG27",  DW_FRAME_FREG27},
    {/*  60 */ "DW_FRAME_FREG28",  DW_FRAME_FREG28},
    {/*  61 */ "DW_FRAME_FREG29",  DW_FRAME_FREG29},
    {/*  62 */ "DW_FRAME_FREG30",  DW_FRAME_FREG30},
    {/*  63 */ "DW_FRAME_FREG31",  DW_FRAME_FREG31},
    {/*  64 */ "DW_FRAME_FREG32",  DW_FRAME_FREG32},
    {/*  65 */ "DW_FRAME_FREG33",  DW_FRAME_FREG33},
    {/*  66 */ "DW_FRAME_FREG34",  DW_FRAME_FREG34},
    {/*  67 */ "DW_FRAME_FREG35",  DW_FRAME_FREG35},
    {/*  68 */ "DW_FRAME_FREG36",  DW_FRAME_FREG36},
    {/*  69 */ "DW_FRAME_FREG37",  DW_FRAME_FREG37},
    {/*  70 */ "DW_FRAME_FREG38",  DW_FRAME_FREG38},
    {/*  71 */ "DW_FRAME_FREG39",  DW_FRAME_FREG39},
    {/*  72 */ "DW_FRAME_FREG40",  DW_FRAME_FREG40},
    {/*  73 */ "DW_FRAME_FREG41",  DW_FRAME_FREG41},
    {/*  74 */ "DW_FRAME_FREG42",  DW_FRAME_FREG42},
    {/*  75 */ "DW_FRAME_FREG43",  DW_FRAME_FREG43},
    {/*  76 */ "DW_FRAME_FREG44",  DW_FRAME_FREG44},
    {/*  77 */ "DW_FRAME_FREG45",  DW_FRAME_FREG45},
    {/*  78 */ "DW_FRAME_FREG46",  DW_FRAME_FREG46},
    {/*  79 */ "DW_FRAME_FREG47",  DW_FRAME_FREG47},
    {/*  80 */ "DW_FRAME_FREG48",  DW_FRAME_FREG48},
    {/*  81 */ "DW_FRAME_FREG49",  DW_FRAME_FREG49},
    {/*  82 */ "DW_FRAME_FREG50",  DW_FRAME_FREG50},
    {/*  83 */ "DW_FRAME_FREG51",  DW_FRAME_FREG51},
    {/*  84 */ "DW_FRAME_FREG52",  DW_FRAME_FREG52},
    {/*  85 */ "DW_FRAME_FREG53",  DW_FRAME_FREG53},
    {/*  86 */ "DW_FRAME_FREG54",  DW_FRAME_FREG54},
    {/*  87 */ "DW_FRAME_FREG55",  DW_FRAME_FREG55},
    {/*  88 */ "DW_FRAME_FREG56",  DW_FRAME_FREG56},
    {/*  89 */ "DW_FRAME_FREG57",  DW_FRAME_FREG57},
    {/*  90 */ "DW_FRAME_FREG58",  DW_FRAME_FREG58},
    {/*  91 */ "DW_FRAME_FREG59",  DW_FRAME_FREG59},
    {/*  92 */ "DW_FRAME_FREG60",  DW_FRAME_FREG60},
    {/*  93 */ "DW_FRAME_FREG61",  DW_FRAME_FREG61},
    {/*  94 */ "DW_FRAME_FREG62",  DW_FRAME_FREG62},
    {/*  95 */ "DW_FRAME_FREG63",  DW_FRAME_FREG63},
    {/*  96 */ "DW_FRAME_FREG64",  DW_FRAME_FREG64},
    {/*  97 */ "DW_FRAME_FREG65",  DW_FRAME_FREG65},
    {/*  98 */ "DW_FRAME_FREG66",  DW_FRAME_FREG66},
    {/*  99 */ "DW_FRAME_FREG67",  DW_FRAME_FREG67},
    {/* 100 */ "DW_FRAME_FREG68",  DW_FRAME_FREG68},
    {/* 101 */ "DW_FRAME_FREG69",  DW_FRAME_FREG69},
    {/* 102 */ "DW_FRAME_FREG70",  DW_FRAME_FREG70},
    {/* 103 */ "DW_FRAME_FREG71",  DW_FRAME_FREG71},
    {/* 104 */ "DW_FRAME_FREG72",  DW_FRAME_FREG72},
    {/* 105 */ "DW_FRAME_FREG73",  DW_FRAME_FREG73},
    {/* 106 */ "DW_FRAME_FREG74",  DW_FRAME_FREG74},
    {/* 107 */ "DW_FRAME_FREG75",  DW_FRAME_FREG75},
    {/* 108 */ "DW_FRAME_FREG76",  DW_FRAME_FREG76},
    {/* 109 */ "DW_FRAME_HIGHEST_NORMAL_REGISTER",  DW_FRAME_HIGHEST_NORMAL_REGISTER}
    };

    const int last_entry = 110;
    /* find the entry */
    int r = find_entry(Dwarf_FRAME_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_CHILDREN_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_CHILDREN_n[] = {
    {/*   0 */ "DW_CHILDREN_no",  DW_CHILDREN_no},
    {/*   1 */ "DW_CHILDREN_yes",  DW_CHILDREN_yes}
    };

    const int last_entry = 2;
    /* find the entry */
    int r = find_entry(Dwarf_CHILDREN_n,last_entry,val,s_out);
    return r;
}
/* ARGSUSED */
int
dwarf_get_ADDR_name (unsigned int val,const char ** s_out)
{
    static Names_Data Dwarf_ADDR_n[] = {
    {/*   0 */ "DW_ADDR_none",  DW_ADDR_none}
    };

    const int last_entry = 1;
    /* find the entry */
    int r = find_entry(Dwarf_ADDR_n,last_entry,val,s_out);
    return r;
}

/* END FILE */

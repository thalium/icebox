/* Generated code, do not edit. */
/* Generated sourcedate  2019-11-05 09:59:27-08:00   */

/* BEGIN FILE */

#ifndef HAVE_USAGE_TAG_ATTR
#define HAVE_USAGE_TAG_ATTR 1
#endif /* HAVE_USAGE_TAG_ATTR */

#ifdef HAVE_USAGE_TAG_ATTR
#include "dwarf.h"
#include "libdwarf.h"

typedef struct {
    unsigned int count; /* Tag count */
    Dwarf_Half tag;     /* Tag value */
} Usage_Tag_Tree;

/* 0x23 - DW_TAG_access_declaration */
static Usage_Tag_Tree tag_tree_23[] = {
    {/*      */ 0, 0}
};

/* 0x01 - DW_TAG_array_type */
static Usage_Tag_Tree tag_tree_01[] = {
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/*      */ 0, 0}
};

/* 0x24 - DW_TAG_base_type */
static Usage_Tag_Tree tag_tree_24[] = {
    {/*      */ 0, 0}
};

/* 0x48 - DW_TAG_call_site */
static Usage_Tag_Tree tag_tree_48[] = {
    {/* 0x49 */ 0, DW_TAG_call_site_parameter},
    {/*      */ 0, 0}
};

/* 0x49 - DW_TAG_call_site_parameter */
static Usage_Tag_Tree tag_tree_49[] = {
    {/*      */ 0, 0}
};

/* 0x25 - DW_TAG_catch_block */
static Usage_Tag_Tree tag_tree_25[] = {
    {/* 0x05 */ 0, DW_TAG_formal_parameter},
    {/* 0x18 */ 0, DW_TAG_unspecified_parameters},
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x29 */ 0, DW_TAG_file_type},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x02 - DW_TAG_class_type */
static Usage_Tag_Tree tag_tree_02[] = {
    {/* 0x0d */ 0, DW_TAG_member},
    {/* 0x1c */ 0, DW_TAG_inheritance},
    {/* 0x23 */ 0, DW_TAG_access_declaration},
    {/* 0x2a */ 0, DW_TAG_friend},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x2f */ 0, DW_TAG_template_type_parameter},
    {/* 0x30 */ 0, DW_TAG_template_value_parameter},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/* 0x43 */ 0, DW_TAG_template_alias},
    {/*      */ 0, 0}
};

/* 0x44 - DW_TAG_coarray_type */
static Usage_Tag_Tree tag_tree_44[] = {
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/*      */ 0, 0}
};

/* 0x1a - DW_TAG_common_block */
static Usage_Tag_Tree tag_tree_1a[] = {
    {/* 0x34 */ 0, DW_TAG_variable},
    {/*      */ 0, 0}
};

/* 0x1b - DW_TAG_common_inclusion */
static Usage_Tag_Tree tag_tree_1b[] = {
    {/*      */ 0, 0}
};

/* 0x11 - DW_TAG_compile_unit */
static Usage_Tag_Tree tag_tree_11[] = {
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x36 */ 0, DW_TAG_dwarf_procedure},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x42 */ 0, DW_TAG_rvalue_reference_type},
    {/* 0x37 */ 0, DW_TAG_restrict_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1a */ 0, DW_TAG_common_block},
    {/* 0x1d */ 0, DW_TAG_inlined_subroutine},
    {/* 0x1e */ 0, DW_TAG_module},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x29 */ 0, DW_TAG_file_type},
    {/* 0x2b */ 0, DW_TAG_namelist},
    {/* 0x39 */ 0, DW_TAG_namespace},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/* 0x3a */ 0, DW_TAG_imported_module},
    {/* 0x43 */ 0, DW_TAG_template_alias},
    {/* 0x3b */ 0, DW_TAG_unspecified_type},
    {/*      */ 0, 0}
};

/* 0x41 - DW_TAG_type_unit */
static Usage_Tag_Tree tag_tree_41[] = {
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1a */ 0, DW_TAG_common_block},
    {/* 0x1d */ 0, DW_TAG_inlined_subroutine},
    {/* 0x1e */ 0, DW_TAG_module},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x29 */ 0, DW_TAG_file_type},
    {/* 0x2b */ 0, DW_TAG_namelist},
    {/* 0x39 */ 0, DW_TAG_namespace},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/* 0x3a */ 0, DW_TAG_imported_module},
    {/* 0x43 */ 0, DW_TAG_template_alias},
    {/*      */ 0, 0}
};

/* 0x3f - DW_TAG_condition */
static Usage_Tag_Tree tag_tree_3f[] = {
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/*      */ 0, 0}
};

/* 0x47 - DW_TAG_atomic_type */
static Usage_Tag_Tree tag_tree_47[] = {
    {/*      */ 0, 0}
};

/* 0x26 - DW_TAG_const_type */
static Usage_Tag_Tree tag_tree_26[] = {
    {/*      */ 0, 0}
};

/* 0x27 - DW_TAG_constant */
static Usage_Tag_Tree tag_tree_27[] = {
    {/*      */ 0, 0}
};

/* 0x36 - DW_TAG_dwarf_procedure */
static Usage_Tag_Tree tag_tree_36[] = {
    {/*      */ 0, 0}
};

/* 0x03 - DW_TAG_entry_point */
static Usage_Tag_Tree tag_tree_03[] = {
    {/* 0x05 */ 0, DW_TAG_formal_parameter},
    {/* 0x18 */ 0, DW_TAG_unspecified_parameters},
    {/* 0x1b */ 0, DW_TAG_common_inclusion},
    {/*      */ 0, 0}
};

/* 0x04 - DW_TAG_enumeration_type */
static Usage_Tag_Tree tag_tree_04[] = {
    {/* 0x28 */ 0, DW_TAG_enumerator},
    {/*      */ 0, 0}
};

/* 0x28 - DW_TAG_enumerator */
static Usage_Tag_Tree tag_tree_28[] = {
    {/*      */ 0, 0}
};

/* 0x29 - DW_TAG_file_type */
static Usage_Tag_Tree tag_tree_29[] = {
    {/*      */ 0, 0}
};

/* 0x05 - DW_TAG_formal_parameter */
static Usage_Tag_Tree tag_tree_05[] = {
    {/*      */ 0, 0}
};

/* 0x2a - DW_TAG_friend */
static Usage_Tag_Tree tag_tree_2a[] = {
    {/*      */ 0, 0}
};

/* 0x08 - DW_TAG_imported_declaration */
static Usage_Tag_Tree tag_tree_08[] = {
    {/*      */ 0, 0}
};

/* 0x3a - DW_TAG_imported_module */
static Usage_Tag_Tree tag_tree_3a[] = {
    {/*      */ 0, 0}
};

/* 0x3d - DW_TAG_imported_unit */
static Usage_Tag_Tree tag_tree_3d[] = {
    {/*      */ 0, 0}
};

/* 0x1c - DW_TAG_inheritance */
static Usage_Tag_Tree tag_tree_1c[] = {
    {/*      */ 0, 0}
};

/* 0x1d - DW_TAG_inlined_subroutine */
static Usage_Tag_Tree tag_tree_1d[] = {
    {/* 0x05 */ 0, DW_TAG_formal_parameter},
    {/* 0x18 */ 0, DW_TAG_unspecified_parameters},
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x0b */ 0, DW_TAG_lexical_block},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1d */ 0, DW_TAG_inlined_subroutine},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x29 */ 0, DW_TAG_file_type},
    {/* 0x2b */ 0, DW_TAG_namelist},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x38 - DW_TAG_interface_type */
static Usage_Tag_Tree tag_tree_38[] = {
    {/* 0x0d */ 0, DW_TAG_member},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/*      */ 0, 0}
};

/* 0x0a - DW_TAG_label */
static Usage_Tag_Tree tag_tree_0a[] = {
    {/*      */ 0, 0}
};

/* 0x0b - DW_TAG_lexical_block */
static Usage_Tag_Tree tag_tree_0b[] = {
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/* 0x3a */ 0, DW_TAG_imported_module},
    {/* 0x0a */ 0, DW_TAG_label},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1d */ 0, DW_TAG_inlined_subroutine},
    {/* 0x0b */ 0, DW_TAG_lexical_block},
    {/* 0x1e */ 0, DW_TAG_module},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x2b */ 0, DW_TAG_namelist},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/* 0x05 */ 0, DW_TAG_formal_parameter},
    {/*      */ 0, 0}
};

/* 0x0d - DW_TAG_member */
static Usage_Tag_Tree tag_tree_0d[] = {
    {/*      */ 0, 0}
};

/* 0x1e - DW_TAG_module */
static Usage_Tag_Tree tag_tree_1e[] = {
    {/*      */ 0, 0}
};

/* 0x2b - DW_TAG_namelist */
static Usage_Tag_Tree tag_tree_2b[] = {
    {/* 0x2c */ 0, DW_TAG_namelist_item},
    {/*      */ 0, 0}
};

/* 0x2c - DW_TAG_namelist_item */
static Usage_Tag_Tree tag_tree_2c[] = {
    {/*      */ 0, 0}
};

/* 0x39 - DW_TAG_namespace */
static Usage_Tag_Tree tag_tree_39[] = {
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1a */ 0, DW_TAG_common_block},
    {/* 0x1d */ 0, DW_TAG_inlined_subroutine},
    {/* 0x1e */ 0, DW_TAG_module},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x2b */ 0, DW_TAG_namelist},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/* 0x39 */ 0, DW_TAG_namespace},
    {/* 0x3a */ 0, DW_TAG_imported_module},
    {/*      */ 0, 0}
};

/* 0x2d - DW_TAG_packed_type */
static Usage_Tag_Tree tag_tree_2d[] = {
    {/*      */ 0, 0}
};

/* 0x3c - DW_TAG_partial_unit */
static Usage_Tag_Tree tag_tree_3c[] = {
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x46 */ 0, DW_TAG_dynamic_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1a */ 0, DW_TAG_common_block},
    {/* 0x1d */ 0, DW_TAG_inlined_subroutine},
    {/* 0x1e */ 0, DW_TAG_module},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x29 */ 0, DW_TAG_file_type},
    {/* 0x2b */ 0, DW_TAG_namelist},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x0f - DW_TAG_pointer_type */
static Usage_Tag_Tree tag_tree_0f[] = {
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x37 */ 0, DW_TAG_restrict_type},
    {/* 0x42 */ 0, DW_TAG_rvalue_reference_type},
    {/* 0x40 */ 0, DW_TAG_shared_type},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x1f - DW_TAG_ptr_to_member_type */
static Usage_Tag_Tree tag_tree_1f[] = {
    {/*      */ 0, 0}
};

/* 0x10 - DW_TAG_reference_type */
static Usage_Tag_Tree tag_tree_10[] = {
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x37 */ 0, DW_TAG_restrict_type},
    {/* 0x42 */ 0, DW_TAG_rvalue_reference_type},
    {/* 0x40 */ 0, DW_TAG_shared_type},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x42 - DW_TAG_rvalue_reference_type */
static Usage_Tag_Tree tag_tree_42[] = {
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x37 */ 0, DW_TAG_restrict_type},
    {/* 0x40 */ 0, DW_TAG_shared_type},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x37 - DW_TAG_restrict_type */
static Usage_Tag_Tree tag_tree_37[] = {
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x42 */ 0, DW_TAG_rvalue_reference_type},
    {/* 0x40 */ 0, DW_TAG_shared_type},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x20 - DW_TAG_set_type */
static Usage_Tag_Tree tag_tree_20[] = {
    {/*      */ 0, 0}
};

/* 0x40 - DW_TAG_shared_type */
static Usage_Tag_Tree tag_tree_40[] = {
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x37 */ 0, DW_TAG_restrict_type},
    {/* 0x42 */ 0, DW_TAG_rvalue_reference_type},
    {/* 0x40 */ 0, DW_TAG_shared_type},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/*      */ 0, 0}
};

/* 0x12 - DW_TAG_string_type */
static Usage_Tag_Tree tag_tree_12[] = {
    {/*      */ 0, 0}
};

/* 0x13 - DW_TAG_structure_type */
static Usage_Tag_Tree tag_tree_13[] = {
    {/* 0x0d */ 0, DW_TAG_member},
    {/* 0x1c */ 0, DW_TAG_inheritance},
    {/* 0x23 */ 0, DW_TAG_access_declaration},
    {/* 0x2a */ 0, DW_TAG_friend},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x33 */ 0, DW_TAG_variant_part},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x2f */ 0, DW_TAG_template_type_parameter},
    {/* 0x30 */ 0, DW_TAG_template_value_parameter},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/* 0x43 */ 0, DW_TAG_template_alias},
    {/*      */ 0, 0}
};

/* 0x2e - DW_TAG_subprogram */
static Usage_Tag_Tree tag_tree_2e[] = {
    {/* 0x05 */ 0, DW_TAG_formal_parameter},
    {/* 0x18 */ 0, DW_TAG_unspecified_parameters},
    {/* 0x31 */ 0, DW_TAG_thrown_type},
    {/* 0x2f */ 0, DW_TAG_template_type_parameter},
    {/* 0x30 */ 0, DW_TAG_template_value_parameter},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x1b */ 0, DW_TAG_common_inclusion},
    {/* 0x1a */ 0, DW_TAG_common_block},
    {/* 0x01 */ 0, DW_TAG_array_type},
    {/* 0x44 */ 0, DW_TAG_coarray_type},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x0f */ 0, DW_TAG_pointer_type},
    {/* 0x10 */ 0, DW_TAG_reference_type},
    {/* 0x12 */ 0, DW_TAG_string_type},
    {/* 0x0b */ 0, DW_TAG_lexical_block},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x15 */ 0, DW_TAG_subroutine_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/* 0x1d */ 0, DW_TAG_inlined_subroutine},
    {/* 0x1f */ 0, DW_TAG_ptr_to_member_type},
    {/* 0x20 */ 0, DW_TAG_set_type},
    {/* 0x21 */ 0, DW_TAG_subrange_type},
    {/* 0x45 */ 0, DW_TAG_generic_subrange},
    {/* 0x24 */ 0, DW_TAG_base_type},
    {/* 0x26 */ 0, DW_TAG_const_type},
    {/* 0x47 */ 0, DW_TAG_atomic_type},
    {/* 0x27 */ 0, DW_TAG_constant},
    {/* 0x29 */ 0, DW_TAG_file_type},
    {/* 0x2b */ 0, DW_TAG_namelist},
    {/* 0x2d */ 0, DW_TAG_packed_type},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x34 */ 0, DW_TAG_variable},
    {/* 0x35 */ 0, DW_TAG_volatile_type},
    {/* 0x0a */ 0, DW_TAG_label},
    {/* 0x3a */ 0, DW_TAG_imported_module},
    {/* 0x08 */ 0, DW_TAG_imported_declaration},
    {/*      */ 0, 0}
};

/* 0x21 - DW_TAG_subrange_type */
static Usage_Tag_Tree tag_tree_21[] = {
    {/*      */ 0, 0}
};

/* 0x45 - DW_TAG_generic_subrange */
static Usage_Tag_Tree tag_tree_45[] = {
    {/*      */ 0, 0}
};

/* 0x15 - DW_TAG_subroutine_type */
static Usage_Tag_Tree tag_tree_15[] = {
    {/* 0x05 */ 0, DW_TAG_formal_parameter},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x18 */ 0, DW_TAG_unspecified_parameters},
    {/*      */ 0, 0}
};

/* 0x2f - DW_TAG_template_type_parameter */
static Usage_Tag_Tree tag_tree_2f[] = {
    {/*      */ 0, 0}
};

/* 0x30 - DW_TAG_template_value_parameter */
static Usage_Tag_Tree tag_tree_30[] = {
    {/*      */ 0, 0}
};

/* 0x31 - DW_TAG_thrown_type */
static Usage_Tag_Tree tag_tree_31[] = {
    {/*      */ 0, 0}
};

/* 0x32 - DW_TAG_try_block */
static Usage_Tag_Tree tag_tree_32[] = {
    {/*      */ 0, 0}
};

/* 0x16 - DW_TAG_typedef */
static Usage_Tag_Tree tag_tree_16[] = {
    {/*      */ 0, 0}
};

/* 0x17 - DW_TAG_union_type */
static Usage_Tag_Tree tag_tree_17[] = {
    {/* 0x2a */ 0, DW_TAG_friend},
    {/* 0x0d */ 0, DW_TAG_member},
    {/* 0x02 */ 0, DW_TAG_class_type},
    {/* 0x04 */ 0, DW_TAG_enumeration_type},
    {/* 0x13 */ 0, DW_TAG_structure_type},
    {/* 0x16 */ 0, DW_TAG_typedef},
    {/* 0x2e */ 0, DW_TAG_subprogram},
    {/* 0x2f */ 0, DW_TAG_template_type_parameter},
    {/* 0x30 */ 0, DW_TAG_template_value_parameter},
    {/* 0x17 */ 0, DW_TAG_union_type},
    {/*      */ 0, 0}
};

/* 0x43 - DW_TAG_template_alias */
static Usage_Tag_Tree tag_tree_43[] = {
    {/* 0x2f */ 0, DW_TAG_template_type_parameter},
    {/* 0x30 */ 0, DW_TAG_template_value_parameter},
    {/*      */ 0, 0}
};

/* 0x18 - DW_TAG_unspecified_parameters */
static Usage_Tag_Tree tag_tree_18[] = {
    {/*      */ 0, 0}
};

/* 0x3b - DW_TAG_unspecified_type */
static Usage_Tag_Tree tag_tree_3b[] = {
    {/*      */ 0, 0}
};

/* 0x34 - DW_TAG_variable */
static Usage_Tag_Tree tag_tree_34[] = {
    {/*      */ 0, 0}
};

/* 0x19 - DW_TAG_variant */
static Usage_Tag_Tree tag_tree_19[] = {
    {/* 0x33 */ 0, DW_TAG_variant_part},
    {/*      */ 0, 0}
};

/* 0x33 - DW_TAG_variant_part */
static Usage_Tag_Tree tag_tree_33[] = {
    {/*      */ 0, 0}
};

/* 0x35 - DW_TAG_volatile_type */
static Usage_Tag_Tree tag_tree_35[] = {
    {/*      */ 0, 0}
};

/* 0x22 - DW_TAG_with_stmt */
static Usage_Tag_Tree tag_tree_22[] = {
    {/*      */ 0, 0}
};

static Usage_Tag_Tree *usage_tag_tree[] = {
    0,
   tag_tree_01, /* 0x01 - DW_TAG_array_type */
   tag_tree_02, /* 0x02 - DW_TAG_class_type */
   tag_tree_03, /* 0x03 - DW_TAG_entry_point */
   tag_tree_04, /* 0x04 - DW_TAG_enumeration_type */
   tag_tree_05, /* 0x05 - DW_TAG_formal_parameter */
    0,
    0,
   tag_tree_08, /* 0x08 - DW_TAG_imported_declaration */
    0,
   tag_tree_0a, /* 0x0a - DW_TAG_label */
   tag_tree_0b, /* 0x0b - DW_TAG_lexical_block */
    0,
   tag_tree_0d, /* 0x0d - DW_TAG_member */
    0,
   tag_tree_0f, /* 0x0f - DW_TAG_pointer_type */
   tag_tree_10, /* 0x10 - DW_TAG_reference_type */
   tag_tree_11, /* 0x11 - DW_TAG_compile_unit */
   tag_tree_12, /* 0x12 - DW_TAG_string_type */
   tag_tree_13, /* 0x13 - DW_TAG_structure_type */
    0,
   tag_tree_15, /* 0x15 - DW_TAG_subroutine_type */
   tag_tree_16, /* 0x16 - DW_TAG_typedef */
   tag_tree_17, /* 0x17 - DW_TAG_union_type */
   tag_tree_18, /* 0x18 - DW_TAG_unspecified_parameters */
   tag_tree_19, /* 0x19 - DW_TAG_variant */
   tag_tree_1a, /* 0x1a - DW_TAG_common_block */
   tag_tree_1b, /* 0x1b - DW_TAG_common_inclusion */
   tag_tree_1c, /* 0x1c - DW_TAG_inheritance */
   tag_tree_1d, /* 0x1d - DW_TAG_inlined_subroutine */
   tag_tree_1e, /* 0x1e - DW_TAG_module */
   tag_tree_1f, /* 0x1f - DW_TAG_ptr_to_member_type */
   tag_tree_20, /* 0x20 - DW_TAG_set_type */
   tag_tree_21, /* 0x21 - DW_TAG_subrange_type */
   tag_tree_22, /* 0x22 - DW_TAG_with_stmt */
   tag_tree_23, /* 0x23 - DW_TAG_access_declaration */
   tag_tree_24, /* 0x24 - DW_TAG_base_type */
   tag_tree_25, /* 0x25 - DW_TAG_catch_block */
   tag_tree_26, /* 0x26 - DW_TAG_const_type */
   tag_tree_27, /* 0x27 - DW_TAG_constant */
   tag_tree_28, /* 0x28 - DW_TAG_enumerator */
   tag_tree_29, /* 0x29 - DW_TAG_file_type */
   tag_tree_2a, /* 0x2a - DW_TAG_friend */
   tag_tree_2b, /* 0x2b - DW_TAG_namelist */
   tag_tree_2c, /* 0x2c - DW_TAG_namelist_item */
   tag_tree_2d, /* 0x2d - DW_TAG_packed_type */
   tag_tree_2e, /* 0x2e - DW_TAG_subprogram */
   tag_tree_2f, /* 0x2f - DW_TAG_template_type_parameter */
   tag_tree_30, /* 0x30 - DW_TAG_template_value_parameter */
   tag_tree_31, /* 0x31 - DW_TAG_thrown_type */
   tag_tree_32, /* 0x32 - DW_TAG_try_block */
   tag_tree_33, /* 0x33 - DW_TAG_variant_part */
   tag_tree_34, /* 0x34 - DW_TAG_variable */
   tag_tree_35, /* 0x35 - DW_TAG_volatile_type */
   tag_tree_36, /* 0x36 - DW_TAG_dwarf_procedure */
   tag_tree_37, /* 0x37 - DW_TAG_restrict_type */
   tag_tree_38, /* 0x38 - DW_TAG_interface_type */
   tag_tree_39, /* 0x39 - DW_TAG_namespace */
   tag_tree_3a, /* 0x3a - DW_TAG_imported_module */
   tag_tree_3b, /* 0x3b - DW_TAG_unspecified_type */
   tag_tree_3c, /* 0x3c - DW_TAG_partial_unit */
   tag_tree_3d, /* 0x3d - DW_TAG_imported_unit */
    0,
   tag_tree_3f, /* 0x3f - DW_TAG_condition */
   tag_tree_40, /* 0x40 - DW_TAG_shared_type */
   tag_tree_41, /* 0x41 - DW_TAG_type_unit */
   tag_tree_42, /* 0x42 - DW_TAG_rvalue_reference_type */
   tag_tree_43, /* 0x43 - DW_TAG_template_alias */
   tag_tree_44, /* 0x44 - DW_TAG_coarray_type */
   tag_tree_45, /* 0x45 - DW_TAG_generic_subrange */
    0,
   tag_tree_47, /* 0x47 - DW_TAG_atomic_type */
   tag_tree_48, /* 0x48 - DW_TAG_call_site */
   tag_tree_49, /* 0x49 - DW_TAG_call_site_parameter */
    0
};

typedef struct {
    Dwarf_Small legal; /* Legal tags */
    Dwarf_Small found; /* Found tags */
} Rate_Tag_Tree;

static Rate_Tag_Tree rate_tag_tree[] = {
    {0, 0},
    { 4, 0 /* 0x01 - DW_TAG_array_type */},
    {21, 0 /* 0x02 - DW_TAG_class_type */},
    { 3, 0 /* 0x03 - DW_TAG_entry_point */},
    { 1, 0 /* 0x04 - DW_TAG_enumeration_type */},
    { 0, 0 /* 0x05 - DW_TAG_formal_parameter */},
    {0, 0},
    {0, 0},
    { 0, 0 /* 0x08 - DW_TAG_imported_declaration */},
    {0, 0},
    { 0, 0 /* 0x0a - DW_TAG_label */},
    {32, 0 /* 0x0b - DW_TAG_lexical_block */},
    {0, 0},
    { 0, 0 /* 0x0d - DW_TAG_member */},
    {0, 0},
    { 8, 0 /* 0x0f - DW_TAG_pointer_type */},
    { 8, 0 /* 0x10 - DW_TAG_reference_type */},
    {37, 0 /* 0x11 - DW_TAG_compile_unit */},
    { 0, 0 /* 0x12 - DW_TAG_string_type */},
    {21, 0 /* 0x13 - DW_TAG_structure_type */},
    {0, 0},
    { 3, 0 /* 0x15 - DW_TAG_subroutine_type */},
    { 0, 0 /* 0x16 - DW_TAG_typedef */},
    {10, 0 /* 0x17 - DW_TAG_union_type */},
    { 0, 0 /* 0x18 - DW_TAG_unspecified_parameters */},
    { 1, 0 /* 0x19 - DW_TAG_variant */},
    { 1, 0 /* 0x1a - DW_TAG_common_block */},
    { 0, 0 /* 0x1b - DW_TAG_common_inclusion */},
    { 0, 0 /* 0x1c - DW_TAG_inheritance */},
    {30, 0 /* 0x1d - DW_TAG_inlined_subroutine */},
    { 0, 0 /* 0x1e - DW_TAG_module */},
    { 0, 0 /* 0x1f - DW_TAG_ptr_to_member_type */},
    { 0, 0 /* 0x20 - DW_TAG_set_type */},
    { 0, 0 /* 0x21 - DW_TAG_subrange_type */},
    { 0, 0 /* 0x22 - DW_TAG_with_stmt */},
    { 0, 0 /* 0x23 - DW_TAG_access_declaration */},
    { 0, 0 /* 0x24 - DW_TAG_base_type */},
    {25, 0 /* 0x25 - DW_TAG_catch_block */},
    { 0, 0 /* 0x26 - DW_TAG_const_type */},
    { 0, 0 /* 0x27 - DW_TAG_constant */},
    { 0, 0 /* 0x28 - DW_TAG_enumerator */},
    { 0, 0 /* 0x29 - DW_TAG_file_type */},
    { 0, 0 /* 0x2a - DW_TAG_friend */},
    { 1, 0 /* 0x2b - DW_TAG_namelist */},
    { 0, 0 /* 0x2c - DW_TAG_namelist_item */},
    { 0, 0 /* 0x2d - DW_TAG_packed_type */},
    {38, 0 /* 0x2e - DW_TAG_subprogram */},
    { 0, 0 /* 0x2f - DW_TAG_template_type_parameter */},
    { 0, 0 /* 0x30 - DW_TAG_template_value_parameter */},
    { 0, 0 /* 0x31 - DW_TAG_thrown_type */},
    { 0, 0 /* 0x32 - DW_TAG_try_block */},
    { 0, 0 /* 0x33 - DW_TAG_variant_part */},
    { 0, 0 /* 0x34 - DW_TAG_variable */},
    { 0, 0 /* 0x35 - DW_TAG_volatile_type */},
    { 0, 0 /* 0x36 - DW_TAG_dwarf_procedure */},
    { 8, 0 /* 0x37 - DW_TAG_restrict_type */},
    { 2, 0 /* 0x38 - DW_TAG_interface_type */},
    {31, 0 /* 0x39 - DW_TAG_namespace */},
    { 0, 0 /* 0x3a - DW_TAG_imported_module */},
    { 0, 0 /* 0x3b - DW_TAG_unspecified_type */},
    {30, 0 /* 0x3c - DW_TAG_partial_unit */},
    { 0, 0 /* 0x3d - DW_TAG_imported_unit */},
    {0, 0},
    { 2, 0 /* 0x3f - DW_TAG_condition */},
    { 9, 0 /* 0x40 - DW_TAG_shared_type */},
    {33, 0 /* 0x41 - DW_TAG_type_unit */},
    { 8, 0 /* 0x42 - DW_TAG_rvalue_reference_type */},
    { 2, 0 /* 0x43 - DW_TAG_template_alias */},
    { 5, 0 /* 0x44 - DW_TAG_coarray_type */},
    { 0, 0 /* 0x45 - DW_TAG_generic_subrange */},
    {0, 0},
    { 0, 0 /* 0x47 - DW_TAG_atomic_type */},
    { 1, 0 /* 0x48 - DW_TAG_call_site */},
    { 0, 0 /* 0x49 - DW_TAG_call_site_parameter */},
    {0, 0}
};

#endif /* HAVE_USAGE_TAG_ATTR */

#define TAG_TREE_COLUMN_COUNT 3

#define TAG_TREE_ROW_COUNT 73

static unsigned int tag_tree_combination_table[TAG_TREE_ROW_COUNT][TAG_TREE_COLUMN_COUNT] = {
/* 0x00 - <no name known for the TAG>          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x01 - DW_TAG_array_type                    */
    { 0x00000010,0x00000002,0x00000060,},
/* 0x02 - DW_TAG_class_type                    */
    { 0x90c8a114,0x0001c458,0x000000d8,},
/* 0x03 - DW_TAG_entry_point                   */
    { 0x09000020,0x00000000,0x00000000,},
/* 0x04 - DW_TAG_enumeration_type              */
    { 0x00000000,0x00000100,0x00000000,},
/* 0x05 - DW_TAG_formal_parameter              */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x06 - <no name known for the TAG>          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x07 - <no name known for the TAG>          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x08 - DW_TAG_imported_declaration          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x09 - <no name known for the TAG>          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x0a - DW_TAG_label                         */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x0b - DW_TAG_lexical_block                 */
    { 0xe0ed8d36,0x043068d3,0x000000f0,},
/* 0x0c - <no name known for the TAG>          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x0d - DW_TAG_member                        */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x0e - <no name known for the TAG>          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x0f - DW_TAG_pointer_type                  */
    { 0x00010000,0x00a02040,0x00000085,},
/* 0x10 - DW_TAG_reference_type                */
    { 0x00008000,0x00a02040,0x00000085,},
/* 0x11 - DW_TAG_compile_unit                  */
    { 0xe4ed8116,0x0ef06ad3,0x000000fc,},
/* 0x12 - DW_TAG_string_type                   */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x13 - DW_TAG_structure_type                */
    { 0x90c8a114,0x0009c458,0x00000098,},
/* 0x14 - <no name known for the TAG>          */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x15 - DW_TAG_subroutine_type               */
    { 0x01400020,0x00000000,0x00000000,},
/* 0x16 - DW_TAG_typedef                       */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x17 - DW_TAG_union_type                    */
    { 0x00c82014,0x0001c400,0x00000000,},
/* 0x18 - DW_TAG_unspecified_parameters        */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x19 - DW_TAG_variant                       */
    { 0x00000000,0x00080000,0x00000000,},
/* 0x1a - DW_TAG_common_block                  */
    { 0x00000000,0x00100000,0x00000000,},
/* 0x1b - DW_TAG_common_inclusion              */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x1c - DW_TAG_inheritance                   */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x1d - DW_TAG_inlined_subroutine            */
    { 0xa1ed8836,0x00306ad3,0x000000f0,},
/* 0x1e - DW_TAG_module                        */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x1f - DW_TAG_ptr_to_member_type            */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x20 - DW_TAG_set_type                      */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x21 - DW_TAG_subrange_type                 */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x22 - DW_TAG_with_stmt                     */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x23 - DW_TAG_access_declaration            */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x24 - DW_TAG_base_type                     */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x25 - DW_TAG_catch_block                   */
    { 0x81ed8036,0x003062d3,0x00000080,},
/* 0x26 - DW_TAG_const_type                    */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x27 - DW_TAG_constant                      */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x28 - DW_TAG_enumerator                    */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x29 - DW_TAG_file_type                     */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x2a - DW_TAG_friend                        */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x2b - DW_TAG_namelist                      */
    { 0x00000000,0x00001000,0x00000000,},
/* 0x2c - DW_TAG_namelist_item                 */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x2d - DW_TAG_packed_type                   */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x2e - DW_TAG_subprogram                    */
    { 0xaded8d36,0x0433ead3,0x000000b0,},
/* 0x2f - DW_TAG_template_type_parameter       */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x30 - DW_TAG_template_value_parameter      */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x31 - DW_TAG_thrown_type                   */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x32 - DW_TAG_try_block                     */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x33 - DW_TAG_variant_part                  */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x34 - DW_TAG_variable                      */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x35 - DW_TAG_volatile_type                 */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x36 - DW_TAG_dwarf_procedure               */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x37 - DW_TAG_restrict_type                 */
    { 0x00018000,0x00202040,0x00000085,},
/* 0x38 - DW_TAG_interface_type                */
    { 0x00002000,0x00004000,0x00000000,},
/* 0x39 - DW_TAG_namespace                     */
    { 0xe4ed8116,0x063068d3,0x000000f0,},
/* 0x3a - DW_TAG_imported_module               */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x3b - DW_TAG_unspecified_type              */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x3c - DW_TAG_partial_unit                  */
    { 0xe4ed8116,0x00306ad3,0x000000f0,},
/* 0x3d - DW_TAG_imported_unit                 */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x3e - DW_TAG_mutable_type                  */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x3f - DW_TAG_condition                     */
    { 0x00000000,0x00000082,0x00000000,},
/* 0x40 - DW_TAG_shared_type                   */
    { 0x00018000,0x00a02040,0x00000085,},
/* 0x41 - DW_TAG_type_unit                     */
    { 0xe4ed8116,0x06306ad3,0x000000f8,},
/* 0x42 - DW_TAG_rvalue_reference_type         */
    { 0x00018000,0x00a02040,0x00000081,},
/* 0x43 - DW_TAG_template_alias                */
    { 0x00000000,0x00018000,0x00000000,},
/* 0x44 - DW_TAG_coarray_type                  */
    { 0x00000002,0x00000012,0x00000060,},
/* 0x45 - DW_TAG_generic_subrange              */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x46 - DW_TAG_dynamic_type                  */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x47 - DW_TAG_atomic_type                   */
    { 0x00000000,0x00000000,0x00000000,},
/* 0x48 - DW_TAG_call_site                     */
    { 0x00000000,0x00000000,0x00000200,},
};

/* END FILE */

/*
  Copyright (C) 2017-2017 David Anderson. All Rights Reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write the Free Software Foundation, Inc., 51
  Franklin Street - Fifth Floor, Boston MA 02110-1301, USA.
*/

/*  All the dwarfdump flags are gathered into a single
    global struct as it has been hard to know how many there
    were or what they were all for. */

struct esb_s;
struct dwconf_s;

enum line_flag_type_e {
  singledw5,  /* Meaning choose single table DWARF5 new interfaces. */
  s2l,        /* Meaning choose two-level DWARF5 new interfaces. */
  orig,       /* Meaning choose DWARF2,3,4 single level interface. */
  orig2l      /* Meaning choose DWARF 2,3,4 two-level interface. */
};

/* Check categories corresponding to the -k option */
typedef enum /* Dwarf_Check_Categories */ {
    abbrev_code_result,
    pubname_attr_result,
    reloc_offset_result,
    attr_tag_result,
    tag_tree_result,
    type_offset_result,
    decl_file_result,
    ranges_result,
    lines_result,
    aranges_result,
    /*  Harmless errors are errors detected inside libdwarf but
        not reported via DW_DLE_ERROR returns because the errors
        won't really affect client code.  The 'harmless' errors
        are reported and otherwise ignored.  It is difficult to report
        the error when the error is noticed by libdwarf, the error
        is reported at a later time.
        The other errors dwarfdump reports are also generally harmless
        but are detected by dwarfdump so it's possble to report the
        error as soon as the error is discovered. */
    harmless_result,
    fde_duplication,
    frames_result,
    locations_result,
    names_result,
    abbreviations_result,
    dwarf_constants_result,
    di_gaps_result,
    forward_decl_result,
    self_references_result,
    attr_encoding_result,
    duplicated_attributes_result,
    total_check_result,
    LAST_CATEGORY  /* Must be last */
} Dwarf_Check_Categories;

struct section_high_offsets_s {
    Dwarf_Unsigned debug_info_size;
    Dwarf_Unsigned debug_abbrev_size;
    Dwarf_Unsigned debug_line_size;
    Dwarf_Unsigned debug_loc_size;
    Dwarf_Unsigned debug_aranges_size;
    Dwarf_Unsigned debug_macinfo_size;
    Dwarf_Unsigned debug_pubnames_size;
    Dwarf_Unsigned debug_str_size;
    Dwarf_Unsigned debug_frame_size;
    Dwarf_Unsigned debug_ranges_size;
    Dwarf_Unsigned debug_pubtypes_size;
    Dwarf_Unsigned debug_types_size;
    Dwarf_Unsigned debug_macro_size;
    Dwarf_Unsigned debug_str_offsets_size;
    Dwarf_Unsigned debug_sup_size;
    Dwarf_Unsigned debug_cu_index_size;
    Dwarf_Unsigned debug_tu_index_size;
};

/* Options to enable debug tracing. */
#define MAX_TRACE_LEVEL 10

struct glflags_s {

    /*  This so both
        dwarf_loclist_n()  and dwarf_get_loclist_c()
        and the dwarf_loclist_from_expr
        variations can be
        tested. Defaults to new
        dwarf_get_loclist_c(). See -g option.
        original IRIX dwarf_loclist() no longer tested
        as of October 2015. */
    boolean gf_use_old_dwarf_loclist;
    enum line_flag_type_e gf_line_flag_selection;

    boolean gf_abbrev_flag;
    boolean gf_aranges_flag; /* .debug_aranges section. */
    boolean gf_debug_names_flag;
    boolean gf_eh_frame_flag;   /* GNU .eh_frame section. */
    boolean gf_frame_flag;      /* .debug_frame section. */
    boolean gf_gdbindex_flag;   /* .gdbindex section. */
    boolean gf_info_flag;  /* .debug_info */
    boolean gf_line_flag;
    boolean gf_line_print_pc;
    boolean gf_line_skeleton_flag;
    boolean gf_loc_flag;
    boolean gf_macinfo_flag; /* DWARF2,3,4. Old macro section*/
    boolean gf_macro_flag; /* DWARF5 */
    boolean gf_pubnames_flag;
    boolean gf_ranges_flag; /* .debug_ranges section. */
    boolean gf_reloc_flag;  /* Elf relocations, not DWARF. */
    boolean gf_static_func_flag;/* SGI only */
    boolean gf_static_var_flag; /* SGI only */
    boolean gf_string_flag;
    boolean gf_pubtypes_flag;   /* SGI only */
    boolean gf_types_flag; /* .debug_types, not all CU types */
    boolean gf_weakname_flag;   /* SGI only */

    boolean gf_header_flag; /* Control printing of Elf header. */
    boolean gf_section_groups_flag;

    boolean gf_producer_children_flag;   /* List of CUs per compiler */
    boolean gf_check_abbrev_code;
    boolean gf_check_pubname_attr;
    boolean gf_check_reloc_offset;
    boolean gf_check_attr_tag;
    boolean gf_check_tag_tree;
    boolean gf_check_type_offset;
    boolean gf_check_decl_file;
    boolean gf_check_macros;
    boolean gf_check_lines;
    boolean gf_check_fdes;
    boolean gf_check_ranges;
    boolean gf_check_aranges;
    boolean gf_check_harmless;
    boolean gf_check_abbreviations;
    boolean gf_check_dwarf_constants;
    boolean gf_check_di_gaps;
    boolean gf_check_forward_decl;
    boolean gf_check_self_references;
    boolean gf_check_attr_encoding;   /* Attributes encoding */
    boolean gf_generic_1200_regs;
    boolean gf_suppress_check_extensions_tables;
    boolean gf_check_duplicated_attributes;
    /* lots of checks make no sense on a dwp debugfission object. */
    boolean gf_suppress_checking_on_dwp;

    /*  suppress_nested_name_search is a band-aid.
        A workaround. A real fix for N**2 behavior is needed.  */
    boolean gf_suppress_nested_name_search;
    boolean gf_uri_options_translation;
    boolean gf_do_print_uri_in_input;

    /* Print global (unique) error messages */
    boolean gf_print_unique_errors;
    boolean gf_found_error_message;

    boolean gf_check_names;
    boolean gf_check_verbose_mode; /* During '-k' mode, display errors */
    boolean gf_check_frames;
    boolean gf_check_frames_extended;    /* Extensive frames check */
    boolean gf_check_locations;          /* Location list check */

    boolean gf_print_usage_tag_attr;      /* Print basic usage */
    boolean gf_print_usage_tag_attr_full; /* Print full usage */

    boolean gf_check_all_compilers;
    boolean gf_check_snc_compiler; /* Check SNC compiler */
    boolean gf_check_gcc_compiler;
    boolean gf_print_summary_all;
    boolean gf_file_use_no_libelf;

    /*  The check and print flags here make it easy to
        allow check-only or print-only.  We no longer support
        check-and-print in a single run.  */
    boolean gf_do_check_dwarf;
    boolean gf_do_print_dwarf;
    boolean gf_check_show_results;  /* Display checks results. */
    boolean gf_record_dwarf_error;  /* A test has failed, this
        is normally set FALSE shortly after being set TRUE, it is
        a short-range hint we should print something we might not
        otherwise print (under the circumstances). */

    boolean gf_check_debug_names;

    /* Display parent/children when in wide format? */
    boolean gf_display_parent_tree;
    boolean gf_display_children_tree;
    int     gf_stop_indent_level;

    /* Print search results in wide format? */
    boolean gf_search_wide_format;

    /* -S option: strings for 'any' and 'match' */
    boolean gf_search_is_on;

    boolean gf_search_print_results;
    boolean gf_cu_name_flag;
    boolean gf_show_global_offsets;
    boolean gf_display_offsets;
    boolean gf_print_str_offsets;

    /*  Base address has a special meaning in DWARF4 relative to address ranges. */
    boolean seen_PU;                  /* Detected a PU */
    boolean seen_CU;                  /* Detected a CU */
    boolean need_CU_name;             /* Need CU name */
    boolean need_CU_base_address;     /* Need CU Base address */
    boolean need_CU_high_address;     /* Need CU High address */
    boolean need_PU_valid_code;       /* Need PU valid code */

    boolean seen_PU_base_address;     /* Detected a Base address for PU */
    boolean seen_PU_high_address;     /* Detected a High address for PU */
    Dwarf_Addr PU_base_address;       /* PU Base address */
    Dwarf_Addr PU_high_address;       /* PU High address */

    Dwarf_Off  DIE_offset;            /* DIE offset in compile unit */
    Dwarf_Off  DIE_overall_offset;    /* DIE offset in .debug_info */

    /*  These globals enable better error reporting. */
    Dwarf_Off  DIE_CU_offset;         /* CU DIE offset in compile unit */
    Dwarf_Off  DIE_CU_overall_offset; /* CU DIE offset in .debug_info */
    int current_section_id;           /* Section being process */

    /*  Base Address is needed for range lists and must come from a CU.
        Low address is for information and can come from a function
        or something in the CU. */
    Dwarf_Addr CU_base_address;       /* CU Base address */
    Dwarf_Addr CU_low_address;        /* CU low address */
    Dwarf_Addr CU_high_address;       /* CU High address */

    Dwarf_Off fde_offset_for_cu_low;
    Dwarf_Off fde_offset_for_cu_high;

    const char *program_name;
    const char *program_fullname;

    const char *search_any_text;
    const char *search_match_text;
    const char *search_regex_text;
    int search_occurrences;

    /* -S option: the compiled_regex */
#ifdef HAVE_REGEX
    regex_t *search_re;
#endif

    /*  Start verbose at zero. verbose can
        be incremented with -v but not decremented. */
    int verbose;

    boolean dense;
    boolean ellipsis;
    boolean show_form_used;

    /*  break_after_n_units is mainly for testing.
        It enables easy limiting of output size/running time
        when one wants the output limited.
        For example,
            -H 2
        limits the -i output to 2 compilation units and
        the -f or -F output to 2 FDEs and 2 CIEs.
    */
    int break_after_n_units;

    struct section_high_offsets_s *section_high_offsets_global;

    /*  pRangesInfo records the DW_AT_high_pc and DW_AT_low_pc
        and is used to check that line range info falls inside
        the known valid ranges.   The data is per CU, and is
        reset per CU in tag_specific_checks_setup(). */
    Bucket_Group *pRangesInfo;

    /*  pLinkonceInfo records data about the link once sections.
        If a line range is not valid in the current CU it might
        be valid in a linkonce section, this data records the
        linkonce sections.  So it is filled in when an
        object file is read and remains unchanged for an entire
        object file.  */
    Bucket_Group *pLinkonceInfo;

    /*  pVisitedInfo records a recursive traversal of DIE attributes
        DW_AT_specification DW_AT_abstract_origin DW_AT_type
        that let DWARF refer (as in a general graph) to
        arbitrary other DIEs.
        These traversals use pVisitedInfo to
        detect any compiler errors that introduce circular references.
        Printing of the traversals is also done on request.
        Entries are added and deleted as they are visited in
        a depth-first traversal.  */
    Bucket_Group *pVisitedInfo;

    /*  Compilation Unit information for improved error messages.
        If the strings are too short we just truncate so fixed length
        here is fine.  */
    #define COMPILE_UNIT_NAME_LEN 512
    char PU_name[COMPILE_UNIT_NAME_LEN]; /* PU Name */
    char CU_name[COMPILE_UNIT_NAME_LEN]; /* CU Name */
    char CU_producer[COMPILE_UNIT_NAME_LEN];  /* CU Producer Name */

    /*  Options to enable debug tracing. */
    int nTrace[MAX_TRACE_LEVEL + 1];

    /* Output filename */
    const char *output_file;
    int group_number;

    /*  Global esb-buffers. */
    struct esb_s *newprogname;
    struct esb_s *cu_name;
    struct esb_s *config_file_path;
    struct esb_s *config_file_tiedpath;
    struct dwconf_s *config_file_data;

    /*  Check errors. */
    int check_error;
};

extern struct glflags_s glflags;

void init_global_flags(void);
void reset_global_flags(void);
void set_checks_off(void);
void reset_overall_CU_error_data(void);
boolean cu_data_is_set(void);

/*  Shortcuts for additional trace options */
#define DUMP_OPTIONS                0   /* Dump options. */
#define DUMP_RANGES_INFO            1   /* Dump RangesInfo Table. */
#define DUMP_LOCATION_SECTION_INFO  2   /* Dump Location (.debug_loc) Info. */
#define DUMP_RANGES_SECTION_INFO    3   /* Dump Ranges (.debug_ranges) Info. */
#define DUMP_LINKONCE_INFO          4   /* Dump Linkonce Table. */
#define DUMP_VISITED_INFO           5   /* Dump Visited Info. */

#define dump_options                glflags.nTrace[DUMP_OPTIONS]
#define dump_ranges_info            glflags.nTrace[DUMP_RANGES_INFO]
#define dump_location_section_info  glflags.nTrace[DUMP_LOCATION_SECTION_INFO]
#define dump_ranges_section_info    glflags.nTrace[DUMP_RANGES_SECTION_INFO]
#define dump_linkonce_info          glflags.nTrace[DUMP_LINKONCE_INFO]
#define dump_visited_info           glflags.nTrace[DUMP_VISITED_INFO]

/* Section IDs */
#define DEBUG_ABBREV      1
#define DEBUG_ARANGES     2
#define DEBUG_FRAME       3
#define DEBUG_INFO        4
#define DEBUG_LINE        5
#define DEBUG_LOC         6
#define DEBUG_MACINFO     7
#define DEBUG_PUBNAMES    8
#define DEBUG_RANGES      9
#define DEBUG_STATIC_VARS 10
#define DEBUG_STATIC_FUNC 11
#define DEBUG_STR         12
#define DEBUG_WEAKNAMES   13
#define DEBUG_TYPES       14
#define DEBUG_GDB_INDEX   15
#define DEBUG_FRAME_EH_GNU 16
#define DEBUG_MACRO       17
#define DEBUG_NAMES       18

/* Print the information only if unique errors is set and it is first time */
#define PRINTING_UNIQUE (!glflags.gf_found_error_message)

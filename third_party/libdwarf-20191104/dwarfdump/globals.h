/*
  Copyright (C) 2000,2004,2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2018 David Anderson. All Rights Reserved.
  Portions Copyright 2012-2018 SN Systems Ltd. All rights reserved.

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

#ifndef globals_INCLUDED
#define globals_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#ifdef DWARF_WITH_LIBELF  /* Without libelf no need for _GNU_SOURCE */
#if (!defined(HAVE_RAW_LIBELF_OK) && defined(HAVE_LIBELF_OFF64_OK) )
/* At a certain point libelf.h requires _GNU_SOURCE.
   here we assume the criteria in configure determined that
   usefully.
*/
#define _GNU_SOURCE 1
#endif
#endif /* DWARF_WITH_LIBELF */

#include "warningcontrol.h"

#define DWARF_SECNAME_BUFFER_SIZE 50

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for exit(), C89 malloc */
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
/* Useful include for some Windows compilers. */
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#ifdef HAVE_STRING_H
#include <string.h> /* for strchr etc */
#endif /* HAVE_STRING_H */

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#ifdef DWARF_WITH_LIBELF
#ifdef HAVE_ELF_H
#include <elf.h>
#endif /* HAVE_ELF_H */
#ifdef HAVE_LIBELF_H
# include <libelf.h>
#else /* !HAVE_LIBELF_H */
# ifdef HAVE_LIBELF_LIBELF_H
# include <libelf/libelf.h>
# endif /* HAVE_LIBELF_LIBELF_H */
#endif /* HAVE_LIB_ELF */
#endif /* DWARF_WITH_LIBELF */
#include "dwarf.h"
#include "libdwarf.h"
#ifdef HAVE_REGEX
#include <regex.h>
#endif
#include "checkutil.h"
#include "defined_types.h"
#include "glflags.h"

/* Used to try to avoid leakage when we hide errors. */
#define DROP_ERROR_INSTANCE(d,r,e)       \
    if (r == DW_DLV_ERROR) {             \
        dwarf_dealloc(d,e,DW_DLA_ERROR); \
        e = 0;                           \
    }

/* Calculate wasted space */
extern void calculate_attributes_usage(Dwarf_Half attr,Dwarf_Half theform,
    Dwarf_Unsigned value);

extern boolean is_strstrnocase(const char *data, const char *pattern);

/* tsearch tree used in macro checking. */
extern void *  macro_check_tree; /* DWARF5 macros. */
extern void *  macinfo_check_tree; /* DWARF2,3,4 macros */

/* Process TAGs for checking mode and reset pRangesInfo table
   if appropriate. */
extern void tag_specific_checks_setup(Dwarf_Half val,int die_indent_level);

extern void print_error_and_continue (Dwarf_Debug dbg, const char * msg,int res, Dwarf_Error err);
extern void print_error (Dwarf_Debug dbg, const char * msg,int res, Dwarf_Error err);

extern void print_line_numbers_this_cu (Dwarf_Debug dbg, Dwarf_Die in_die);

extern void print_frames (Dwarf_Debug dbg, int print_debug_frame,
    int print_eh_frame,struct dwconf_s *);
extern void printreg(Dwarf_Unsigned reg,struct dwconf_s *config_data);
extern void print_ranges (Dwarf_Debug dbg);
extern void print_pubnames (Dwarf_Debug dbg);
extern void print_macinfo (Dwarf_Debug dbg);
extern void print_infos (Dwarf_Debug dbg,Dwarf_Bool is_info);
extern void print_locs (Dwarf_Debug dbg);
extern void print_abbrevs (Dwarf_Debug dbg);
extern void print_strings (Dwarf_Debug dbg);
extern void print_aranges (Dwarf_Debug dbg);
extern void print_static_funcs(Dwarf_Debug dbg);
extern void print_static_vars(Dwarf_Debug dbg);
enum type_type_e {SGI_TYPENAME, DWARF_PUBTYPES} ;
extern void print_types(Dwarf_Debug dbg,enum type_type_e type_type);
extern void print_weaknames(Dwarf_Debug dbg);
extern void print_exception_tables(Dwarf_Debug dbg);
extern void print_debug_names(Dwarf_Debug dbg);
int print_all_pubnames_style_records(Dwarf_Debug dbg,
    const char * linetitle,
    const char * section_true_name,
    Dwarf_Global *globbuf,
    Dwarf_Signed count,
    Dwarf_Error *err);

/*  These three ELF only */
extern void print_object_header(Dwarf_Debug dbg);
extern void print_relocinfo (Dwarf_Debug dbg);
void clean_up_syms_malloc_data(void);

/*  Space used to record range information */
extern void allocate_range_array_info(void);
extern void release_range_array_info(void);
extern void record_range_array_info_entry(Dwarf_Off die_off,
    Dwarf_Off range_off);
extern void check_range_array_info(Dwarf_Debug dbg);

boolean should_skip_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die);

void get_address_size_and_max(Dwarf_Debug dbg,
   Dwarf_Half * size,
   Dwarf_Addr * max,
   Dwarf_Error *err);

/* Returns the producer of the CU */
int get_cu_name(Dwarf_Debug dbg,Dwarf_Die cu_die,
    Dwarf_Off  dieprint_cu_offset,
    char **short_name,char **long_name);

/* Get number of abbreviations for a CU */
extern void get_abbrev_array_info(Dwarf_Debug dbg,Dwarf_Unsigned offset);
/* Validate an abbreviation */
extern void validate_abbrev_code(Dwarf_Debug dbg,Dwarf_Unsigned abbrev_code);

extern void print_die_and_children(
    Dwarf_Debug dbg,
    Dwarf_Die in_die,
    Dwarf_Off dieprint_cu_offset,
    Dwarf_Bool is_info,
    char **srcfiles,
    Dwarf_Signed cnt);
extern boolean print_one_die(
    Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Off dieprint_cu_offset,
    boolean print_information,
    int die_indent_level,
    char **srcfiles,
    Dwarf_Signed cnt,
    boolean ignore_die_stack);

/* Check for specific compiler */
extern boolean checking_this_compiler(void);
extern void update_compiler_target(const char *producer_name);
extern void add_cu_name_compiler_target(char *name);

/*  General error reporting routines. These were
    macros for a short time and when changed into functions
    they kept (for now) their capitalization.
    The capitalization will likely change. */
extern void PRINT_CU_INFO(void);
extern void DWARF_CHECK_COUNT(Dwarf_Check_Categories category, int inc);
extern void DWARF_ERROR_COUNT(Dwarf_Check_Categories category, int inc);
extern void DWARF_CHECK_ERROR_PRINT_CU(void);
#define DWARF_CHECK_ERROR(c,d)    DWARF_CHECK_ERROR3(c,d,0,0)
#define DWARF_CHECK_ERROR2(c,d,e) DWARF_CHECK_ERROR3(c,d,e,0)
extern void DWARF_CHECK_ERROR3(Dwarf_Check_Categories category,
    const char *str1, const char *str2, const char *strexpl);

extern void print_macinfo_by_offset(Dwarf_Debug dbg,Dwarf_Unsigned offset);

void ranges_esb_string_destructor(void);
void destruct_abbrev_array(void);

extern Dwarf_Die current_cu_die_for_print_frames; /* This is
    an awful hack, making current_cu_die_for_print_frames public.
    But it enables cleaning up (doing all dealloc needed). */
/* defined in print_sections.c, die for the current compile unit,
   used in get_fde_proc_name() */


int get_proc_name(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Addr low_pc,
    char *proc_name_buf, int proc_name_buf_len, void **pcMap);

extern void dump_block(char *prefix, char *data, Dwarf_Signed len);

extern void print_gdb_index(Dwarf_Debug dbg);
extern void print_debugfission_index(Dwarf_Debug dbg,const char *type);

void clean_up_die_esb(void);
void safe_strcpy(char *out, long outlen, const char *in, long inlen);

void print_macros_5style_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die,
    Dwarf_Bool in_import_list, Dwarf_Unsigned offset);

/* Detailed attributes encoding space */
void print_attributes_encoding(Dwarf_Debug dbg);

/* Detailed tag and attributes usage */
void print_tag_attributes_usage(Dwarf_Debug dbg);

void print_section_groups_data(Dwarf_Debug dbg);
void update_section_flags_per_groups(Dwarf_Debug dbg);
void groups_restore_subsidiary_flags(void);

void print_str_offsets_section(Dwarf_Debug dbg);

void print_any_harmless_errors(Dwarf_Debug dbg);

void print_secname(Dwarf_Debug dbg,const char *secname);

#ifdef __cplusplus
}
#endif

#endif /* globals_INCLUDED */

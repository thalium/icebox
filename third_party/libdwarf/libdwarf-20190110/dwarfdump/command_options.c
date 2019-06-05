/*
  Copyright 2010-2018 David Anderson. All rights reserved.

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

#include "globals.h"
#include "dwconf.h"
#include "dwgetopt.h"

#include "common.h"
#include "makename.h"
#include "uri.h"
#include "esb.h"                /* For flexible string buffer. */
#include "sanitized.h"
#include "tag_common.h"
#include "section_bitmaps.h"
#include "command_options.h"
#include "compiler_info.h"

static const char *remove_quotes_pair(const char *text);
static char *special_program_name(char *n);
static void suppress_check_dwarf(void);

extern char *dwoptarg;

/*  These configure items are for the
    frame data.  We're flexible in
    the path to dwarfdump.conf .
    The HOME strings here are transformed in
    dwconf.c to reference the environment
    variable $HOME .

    As of August 2018 CONFPREFIX is always set as it
    comes from autoconf --prefix, aka  $prefix
    which defaults to /usr/local

    The install puts the .conf file in
    CONFPREFIX/dwarfdump/
*/
static char *config_file_defaults[] = {
    "dwarfdump.conf",
    "./dwarfdump.conf",
    "HOME/.dwarfdump.conf",
    "HOME/dwarfdump.conf",
#ifdef CONFPREFIX
/* See Makefile.am dwarfdump_CFLAGS. This prefix
    is the --prefix option (defaults to /usr/local
    and Makefile.am adds /share/dwarfdump ) */
/* We need 2 levels of macro to get the name turned into
   the string we want. */
#define STR2(s) # s
#define STR(s)  STR2(s)
    STR(CONFPREFIX) "/dwarfdump.conf",
#else
    /*  This no longer used as of August 2018. */
    "/usr/lib/dwarfdump.conf",
#endif
    0
};
static const char *config_file_abi = 0;

/* Do printing of most sections.
   Do not do detailed checking.
*/
static void
do_all(void)
{
    glflags.gf_frame_flag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag =  TRUE; /* .debug_types */
    glflags.gf_line_flag = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_macinfo_flag = TRUE;
    glflags.gf_macro_flag = TRUE;
    glflags.gf_aranges_flag = TRUE;
    /*  Do not do
        glflags.gf_loc_flag = TRUE
        glflags.gf_abbrev_flag = TRUE;
        glflags.gf_ranges_flag = TRUE;
        because nothing in
        the DWARF spec guarantees the sections are free of random bytes
        in areas not referenced by .debug_info */
    glflags.gf_string_flag = TRUE;
    /*  Do not do
        glflags.gf_reloc_flag = TRUE;
        as print_relocs makes no sense for non-elf dwarfdump users.  */
    glflags.gf_static_func_flag = TRUE; /* SGI only*/
    glflags.gf_static_var_flag = TRUE; /* SGI only*/
    glflags.gf_pubtypes_flag = TRUE;  /* both SGI typenames and dwarf_pubtypes. */
    glflags.gf_weakname_flag = TRUE; /* SGI only*/
    glflags.gf_header_flag = TRUE; /* Dump header info */
    glflags.gf_debug_names_flag = TRUE;
}

static int
get_number_value(char *v_in,long int *v_out)
{
    long int v= 0;
    size_t len = strlen(v_in);
    char *endptr = 0;

    if (len < 1) {
        return DW_DLV_ERROR;
    }
    v = strtol(v_in,&endptr,10);
    if (endptr == v_in) {
        return DW_DLV_NO_ENTRY;
    }
    if (*endptr != '\0') {
        return DW_DLV_ERROR;
    }
    *v_out = v;
    return DW_DLV_OK;
}

static void suppress_print_dwarf(void)
{
    glflags.gf_do_print_dwarf = FALSE;
    glflags.gf_do_check_dwarf = TRUE;
}

/* Remove matching leading/trailing quotes.
   Does not alter the passed in string.
   If quotes removed does a makename on a modified string. */
static const char *
remove_quotes_pair(const char *text)
{
    static char single_quote = '\'';
    static char double_quote = '\"';
    char quote = 0;
    const char *p = text;
    int len = strlen(text);

    if (len < 2) {
        return p;
    }

    /* Compare first character with ' or " */
    if (p[0] == single_quote) {
        quote = single_quote;
    } else {
        if (p[0] == double_quote) {
            quote = double_quote;
        }
        else {
            return p;
        }
    }
    {
        if (p[len - 1] == quote) {
            char *altered = calloc(1,len+1);
            const char *str2 = 0;
            strcpy(altered,p+1);
            altered[len - 2] = '\0';
            str2 =  makename(altered);
            free(altered);
            return str2;
        }
    }
    return p;
}

/*  By trimming a /dwarfdump.O
    down to /dwarfdump  (keeping any prefix
    or suffix)
    we can avoid a sed command in
    regressiontests/DWARFTEST.sh
    and save 12 minutes run time of a regression
    test.

    The effect is, when nothing has changed in the
    normal output, that the program_name matches too.
    Because we don't want a different name of dwarfdump
    to cause a mismatch.  */
static char *
special_program_name(char *n)
{
    char * mp = "/dwarfdump.O";
    char * revstr = "/dwarfdump";
    char *cp = n;
    size_t mslen = strlen(mp);

    for(  ; *cp; ++cp ) {
        if (*cp == *mp) {
            if(!strncmp(cp,mp,mslen)){
                esb_append(glflags.newprogname,revstr);
                cp += mslen-1;
            } else {
                esb_appendn(glflags.newprogname,cp,1);
            }
        } else {
            esb_appendn(glflags.newprogname,cp,1);
        }
    }
    return esb_get_string(glflags.newprogname);
}

static void suppress_check_dwarf(void)
{
    glflags.gf_do_print_dwarf = TRUE;
    if (glflags.gf_do_check_dwarf) {
        printf("Warning: check flag turned off, "
            "checking and printing are separate.\n");
    }
    glflags.gf_do_check_dwarf = FALSE;
    set_checks_off();
}

/*  The strings whose pointers are returned here
    from makename are never destructed, but
    that is ok since there are only about 10 created at most.  */
const char *
do_uri_translation(const char *s,const char *context)
{
    struct esb_s str;
    char *finalstr = 0;
    if (!glflags.gf_uri_options_translation) {
        return makename(s);
    }
    esb_constructor(&str);
    translate_from_uri(s,&str);
    if (glflags.gf_do_print_uri_in_input) {
        if (strcmp(s,esb_get_string(&str))) {
            printf("Uri Translation on option %s\n",context);
            printf("    \'%s\'\n",s);
            printf("    \'%s\'\n",esb_get_string(&str));
        }
    }
    finalstr = makename(esb_get_string(&str));
    esb_destructor(&str);
    return finalstr;
}

/*  Support for short (-option) and long (--option) names options.
    These functions implement the individual options. They are called from
    short names and long names options. Implementation code is shared for
    both types of formats. */

/*  Handlers for the short/long names options. */
static void arg_check_abbrev(void);
static void arg_check_all(void);
static void arg_check_aranges(void);
static void arg_check_attr_dup(void);
static void arg_check_attr_encodings(void);
static void arg_check_attr_names(void);
static void arg_check_constants(void);
static void arg_check_files_lines(void);
static void arg_check_forward_refs(void);
static void arg_check_frame_basic(void);
static void arg_check_frame_extended(void);
static void arg_check_frame_info(void);
static void arg_check_gaps(void);
static void arg_check_loc(void);
static void arg_check_macros(void);
static void arg_check_pubnames(void);
static void arg_check_ranges(void);
static void arg_check_self_refs(void);
static void arg_check_show(void);
static void arg_check_silent(void);
static void arg_check_summary(void);
static void arg_check_tag_attr(void);
static void arg_check_tag_tag(void);
static void arg_check_type(void);
static void arg_check_unique(void);

#ifdef HAVE_USAGE_TAG_ATTR
static void arg_check_usage(void);
static void arg_check_usage_extended(void);
#endif /* HAVE_USAGE_TAG_ATTR */

static void arg_elf(void);
static void arg_elf_abbrev(void);
static void arg_elf_aranges(void);
static void arg_elf_default(void);
static void arg_elf_fission(void);
static void arg_elf_frames(void);
static void arg_elf_header(void);
static void arg_elf_info(void);
static void arg_elf_line(void);
static void arg_elf_loc(void);
static void arg_elf_macinfo(void);
static void arg_elf_pubnames(void);
static void arg_elf_pubtypes(void);
static void arg_elf_ranges(void);
static void arg_elf_strings(void);
static void arg_elf_text(void);

static void arg_file_abi(void);
static void arg_file_line5(void);
static void arg_file_name(void);
static void arg_file_output(void);
static void arg_file_tied(void);
static void arg_file_use_no_libelf(void);

static void arg_format_attr_name(void);
static void arg_format_dense(void);
static void arg_format_ellipsis(void);
static void arg_format_extensions(void);
static void arg_format_global_offsets(void);
static void arg_format_loc(void);
static void arg_format_registers(void);
static void arg_format_suppress_data(void);
static void arg_format_suppress_group(void);
static void arg_format_suppress_lookup(void);
static void arg_format_suppress_offsets(void);
static void arg_format_suppress_sanitize(void);
static void arg_format_suppress_uri(void);
static void arg_format_suppress_uri_msg(void);

static void arg_format_file(void);
static void arg_format_gcc(void);
static void arg_format_groupnumber(void);
static void arg_format_limit(void);
static void arg_format_producer(void);
static void arg_format_snc(void);

static void arg_print_all(void);
static void arg_print_abbrev(void);
static void arg_print_aranges(void);
static void arg_print_debug_frame(void);
static void arg_print_debug_names(void);
static void arg_print_fission(void);
static void arg_print_gnu_frame(void);
static void arg_print_info(void);
static void arg_print_lines(void);
static void arg_print_lines_short(void);
static void arg_print_loc(void);
static void arg_print_macinfo(void);
static void arg_print_pubnames(void);
static void arg_print_producers(void);
static void arg_print_ranges(void);
static void arg_print_static(void);
static void arg_print_static_func(void);
static void arg_print_static_var(void);
static void arg_print_str_offsets(void);
static void arg_print_strings(void);
static void arg_print_types(void);
static void arg_print_weaknames(void);

static void arg_reloc(void);
static void arg_reloc_abbrev(void);
static void arg_reloc_aranges(void);
static void arg_reloc_frames(void);
static void arg_reloc_info(void);
static void arg_reloc_line(void);
static void arg_reloc_loc(void);
static void arg_reloc_pubnames(void);
static void arg_reloc_ranges(void);

static void arg_search_any(void);
static void arg_search_any_count(void);
static void arg_search_match(void);
static void arg_search_match_count(void);
static void arg_search_regex(void);
static void arg_search_regex_count(void);
static void arg_search_count(void);
static void arg_search_invalid(void);

static void arg_search_print_children(void);
static void arg_search_print_parent(void);
static void arg_search_print_tree(void);

static void arg_help(void);
static void arg_trace(void);
static void arg_verbose(void);
static void arg_version(void);

static void arg_c_multiple_selection(void);
static void arg_E_multiple_selection(void);
static void arg_h_multiple_selection(void);
static void arg_l_multiple_selection(void);
static void arg_k_multiple_selection(void);
static void arg_kx_multiple_selection(void);
#ifdef HAVE_USAGE_TAG_ATTR
static void arg_ku_multiple_selection(void);
#endif /* HAVE_USAGE_TAG_ATTR */
static void arg_o_multiple_selection(void);
static void arg_O_multiple_selection(void);
static void arg_S_multiple_selection(void);
static void arg_t_multiple_selection(void);
static void arg_W_multiple_selection(void);
static void arg_x_multiple_selection(void);

static void arg_not_supported(void);
static void arg_x_invalid(void);

/*  Extracted from 'process_args', as they are used by option handlers. */
static boolean arg_usage_error = FALSE;
static int arg_option = 0;

static const char *usage_debug_text[] = {
"Usage: DwarfDump <debug_options>",
"options:\t-0\tprint this information",
"\t\t-1\tDump RangesInfo Table",
"\t\t-2\tDump Location (.debug_loc) Info",
"\t\t-3\tDump Ranges (.debug_ranges) Info",
"\t\t-4\tDump Linkonce Table",
"\t\t-5\tDump Visited Info",
""
};

static const char *usage_long_text[] = {
"Usage: DwarfDump <options> <object file>",
" ",
"----------------------------------------------------------------------",
"Print Debug Sections",
"----------------------------------------------------------------------",
"-b   --print-abbrev      Print abbrev section",
"-a   --print-all         Print all debug_* sections",
"-r   --print-aranges     Print aranges section",
"-F   --print-eh-frame    Print gnu .eh_frame section",
"-I   --print-fission     Print fission sections:",
"                         .gdb_index, .debug_cu_index, .debug_tu_index,",
"                         .debug_tu_index, .gdb_index, .debug_cu_index,",
"                         .debug_tu_index, .debug_tu_index, .gdb_index,",
"                         .debug_cu_index, .debug_tu_index",
"-f   --print-frame       Print dwarf frame section",
"-i   --print-info        Print info section",
"-l   --print-lines       Print line section",
"-ls  --print-lines-short Print line section, but do not",
"                         print <pc> address",
"-c   --print-loc         Print loc section",
"-m   --print-macinfo     Print macinfo section",
"-P   --print-producers   Print list of compile units per producer",
"-p   --print-pubnames    Print pubnames section",
"-N   --print-ranges      Print ranges section",
"-ta  --print-static      Print both static sections",
"-tf  --print-static-func Print static func section",
"-tv  --print-static-var  Print static var section",
"-s   --print-strings     Print string section",
"     --print-str-offsets Print the .debug_str_offsets section",
"-y   --print-type        Print type section",
"-w   --print-weakname    Print weakname section",
" ",
"----------------------------------------------------------------------",
"Print Relocations Info",
"----------------------------------------------------------------------",
"-o   --reloc           Print relocation info [afiloprR]",
"-oa  --reloc-abbrev    Print relocation .debug_abbrev section",
"-or  --reloc-aranges   Print relocation .debug_aranges section",
"-of  --reloc-frames    Print relocation .debug_frame section",
"-oi  --reloc-info      Print relocation .debug_info section",
"-ol  --reloc-line      Print relocation .debug_line section",
"-oo  --reloc-loc       Print relocation .debug_loc section",
"-op  --reloc-pubnames  Print relocation .debug_pubnames section",
"-oR  --reloc-ranges    Print relocation .debug_ranges section",
" ",
"----------------------------------------------------------------------",
"Print ELF sections header",
"----------------------------------------------------------------------",
"-E   --elf           Print object Header and/or section information",
"                     Same as -E[adfhiIlmoprRstx]",
"-Ea  --elf-abbrev    Print .debug_abbrev header",
"-Er  --elf-aranges   Print .debug_aranges header",
"-Ed  --elf-default   Same as -E and {liaprfoRstx}",
"-EI  --elf-fission   Print fission headers,",
"                     .gdb_index, .debug_cu_index, .debug_tu_index",
"-Ef  --elf-frames    Print .debug_frame header",
"-Eh  --elf-header    Print ELF header",
"-Ei  --elf-info      Print .debug_info header",
"-El  --elf-line      Print .debug_line header",
"-Eo  --elf-loc       Print .debug_loc header",
"-Em  --elf-macinfo   Print old macinfo and dwarf5 macro header",
"-Ep  --elf-pubnames  Print .debug_pubnames header",
"-Et  --elf-pubtypes  Print .debug_types header",
"-ER  --elf-ranges    Print .debug_ranges header",
"-Es  --elf-strings   Print .debug_string header",
"-Ex  --elf-text      Print .text header",
" ",
"----------------------------------------------------------------------",
"Check DWARF Integrity",
"----------------------------------------------------------------------",
"-kb  --check-abbrev         Check abbreviations",
"-ka  --check-all            Do all checks",
"-kM  --check-aranges        Check ranges list (.debug_aranges)",
"-kD  --check-attr-dup       Check duplicated attributes",
"-kE  --check-attr-encodings Examine attributes encodings",
"-kn  --check-attr-names     Examine names in attributes",
"-kc  --check-constants      Examine DWARF constants",
"-kF  --check-files-lines    Examine integrity of files-lines",
"                            attributes",
"-kR  --check-forward-refs   Check forward references to DIEs",
"                            (declarations)",
"-kx  --check-frame-basic    Basic frames check (.eh_frame,",
"                            .debug_frame)",
"-kxe --check-frame-extended Extensive frames check (.eh_frame,",
"                            .debug_frame)",
"-kf  --check-frame-info     Examine frame information (use with",
"                            -f or -F)",
"-kg  --check-gaps           Check debug info gaps",
"-kl  --check-loc            Check location list (.debug_loc)",
"-kw  --check-macros         Check macros",
"-ke  --check-pubnames       Examine attributes of pubnames",
"-km  --check-ranges         Check ranges list (.debug_ranges)",
"-kS  --check-self-refs      Check self references to DIEs",
"-kd  --check-show           Show check results",
"-ks  --check-silent         Perform checks in silent mode",
"-ki  --check-summary        Display summary for all compilers",
"-kr  --check-tag-attr       Examine tag-attr relation",
"-kt  --check-tag-tag        Examine tag-tag relations",
"                            Unless -C option given certain common",
"                            tag-attr and tag-tag extensions are",
"                            assumed to be ok (not reported).",
"-ky  --check-type           Eexamine type info",
"-kG  --check-unique         Print only unique errors",
#ifdef HAVE_USAGE_TAG_ATTR
"-ku  --check-usage          Print tag-tree & tag-attr usage",
"                            (basic format)",
"-kuf --check-usage-extended Print tag-tree & tag-attr usage",
"                            (full format)",
#endif /* HAVE_USAGE_TAG_ATTR */
" ",
"----------------------------------------------------------------------",
"Print Output Qualifiers",
"----------------------------------------------------------------------",
"-M   --format-attr-name        Print the form name for each attribute",
"-d   --format-dense            One line per entry (info section only)",
"-e   --format-ellipsis         Short names for tags, attrs etc.",
"-G   --format-global-offsets   Show global die offsets",
"-g   --format-loc              (incomplete loclist support. Do not use.)",
"-R   --format-registers        Print frame register names as r33 etc",
"                               and allow up to 1200 registers.",
"                               Print using a 'generic' register set.",
"-Q   --format-suppress-data    Suppress printing section data",
"-x noprintsectiongroups",
"     --format-suppress-group   do not print section groups",
"-n   --format-suppress-lookup  Suppress frame information function name",
"                               lookup(when printing frame information",
"                               from multi-gigabyte object files this",
"                               option may save significant time).",
"-D   --format-suppress-offsets do not show offsets",
#if 0
"-x nosanitizestrings",
"     --format-suppress-sanitize Bogus string characters come thru printf",
#endif
"-U   --format-suppress-uri     Suppress uri-translate",
"-q   --format-suppress-uri-msg Suppress uri-did-translate notification",
"-C   --format-extensions       Activate printing (with -i) of warnings",
"                               about certain common DWARF extensions.",
" ",
"----------------------------------------------------------------------",
"Print Output Limiters",
"----------------------------------------------------------------------",
"-u<file> --format-file=<file>  Print only specified file (CU name)",
"-cg      --format-gcc          Check only GCC compiler objects",
"-x groupnumber=<n>    --format-group=<n>      Groupnumber to print",
"-H<num>  --format-limit=<num>  Limit output to the first <num>",
"                               major units.",
"                               Stop after <num> compilation units",
"-c<str>  --format-producer=<str> Check only specific compiler objects",
"                               <str> is described by 'DW_AT_producer'",
"                               -c'350.1' check only compiler objects",
"                               with 350.1 in the CU name",
"-cs      --format-snc          Check only SNC compiler objects",
" ",
"----------------------------------------------------------------------",
"File Specifications",
"----------------------------------------------------------------------",
"-x abi=<abi>     --file-abi=<abi>      Name abi in dwarfdump.conf",
"-x name=<path>   --file-name=<path>    Name dwarfdump.conf",
"-x line5=<val>   --file-line5=<val>    Table DWARF5 new interfaces",
"                                         where <val> is:",
"                                           std, s2l, orig or orig2l",
"-O file=<path>   --file-output=<path>  Name the output file",
"-x tied=<path>   --file-tied=<path>    Name an associated object file",
"                                         (Split DWARF)",
"                 --file-use-no-libelf  Use non-libelf to read objects",
"                                         (as much as possible)",
" ",
"----------------------------------------------------------------------",
"Search text in attributes",
"----------------------------------------------------------------------",
"-S any=<text>    --search-any=<text>       Search any <text>",
"-Svany=<text>    --search-any-count=<text> print number of occurrences",
"-S match=<text>  --search-match=<text>     Search matching <text>",
"-Svmatch=<text>  --search-match-count<text> print number of occurrences",
#ifdef HAVE_REGEX
"-S regex=<text>  --search-regex=<text>     Use regular expression",
"                                           matching",
"-Svregex=<text>  --search-regex-count<text> print number of",
"                                            occurrences",
#endif /* HAVE_REGEX */
"                             only one -S option allowed, any= and",
"                             regex= only usable if the functions",
"                             required are found at configure time",
" ",
"-Wc  --search-print-children Print children tree (wide format) with -S",
"-Wp  --search-print-parent   Print parent tree (wide format) with -S",
"-W   --search-print-tree     Print parent/children tree ",
"                             (wide format) with -S",
" ",
"----------------------------------------------------------------------",
"Help & Version",
"----------------------------------------------------------------------",
"-h   --help          Print this dwarfdump help message.",
"-v   --verbose       Show more information.",
"-vv  --verbose-more  Show even more information.",
"-V   --version       Print version information.",
"",
};

enum longopts_vals {
  OPT_BEGIN = 999,

  /* Check DWARF Integrity                                                   */
  OPT_CHECK_ABBREV,             /* -kb  --check-abbrev                       */
  OPT_CHECK_ALL,                /* -ka  --check-all                          */
  OPT_CHECK_ARANGES,            /* -kM  --check-aranges                      */
  OPT_CHECK_ATTR_DUP,           /* -kD  --check-attr-dup                     */
  OPT_CHECK_ATTR_ENCODINGS,     /* -kE  --check-attr-encodings               */
  OPT_CHECK_ATTR_NAMES,         /* -kn  --check-attr-names                   */
  OPT_CHECK_CONSTANTS,          /* -kc  --check-constants                    */
  OPT_CHECK_FILES_LINES,        /* -kF  --check-files-lines                  */
  OPT_CHECK_FORWARD_REFS,       /* -kR  --check-forward-refs                 */
  OPT_CHECK_FRAME_BASIC,        /* -kx  --check-frame-basic                  */
  OPT_CHECK_FRAME_EXTENDED,     /* -kxe --check-frame-extended               */
  OPT_CHECK_FRAME_INFO,         /* -kf  --check-frame-info                   */
  OPT_CHECK_GAPS,               /* -kg  --check-gaps                         */
  OPT_CHECK_LOC,                /* -kl  --check-loc                          */
  OPT_CHECK_MACROS,             /* -kw  --check-macros                       */
  OPT_CHECK_PUBNAMES,           /* -ke  --check-pubnames                     */
  OPT_CHECK_RANGES,             /* -km  --check-ranges                       */
  OPT_CHECK_SELF_REFS,          /* -kS  --check-self-refs                    */
  OPT_CHECK_SHOW,               /* -kd  --check-show                         */
  OPT_CHECK_SILENT,             /* -ks  --check-silent                       */
  OPT_CHECK_SUMMARY,            /* -ki  --check-summary                      */
  OPT_CHECK_TAG_ATTR,           /* -kr  --check-tag-attr                     */
  OPT_CHECK_TAG_TAG,            /* -kt  --check-tag-tag                      */
  OPT_CHECK_TYPE,               /* -ky  --check-type                         */
  OPT_CHECK_UNIQUE,             /* -kG  --check-unique                       */
#ifdef HAVE_USAGE_TAG_ATTR
  OPT_CHECK_USAGE,              /* -ku  --check-usage                        */
  OPT_CHECK_USAGE_EXTENDED,     /* -kuf --check-usage-extended               */
#endif /* HAVE_USAGE_TAG_ATTR */

  /* Print ELF sections header                                               */
  OPT_ELF,                      /* -E   --elf                                */
  OPT_ELF_ABBREV,               /* -Ea  --elf-abbrev                         */
  OPT_ELF_ARANGES,              /* -Er  --elf-aranges                        */
  OPT_ELF_DEFAULT,              /* -Ed  --elf-default                        */
  OPT_ELF_FISSION,              /* -EI  --elf-fission                        */
  OPT_ELF_FRAMES,               /* -Ef  --elf-frames                         */
  OPT_ELF_HEADER,               /* -Eh  --elf-header                         */
  OPT_ELF_INFO,                 /* -Ei  --elf-info                           */
  OPT_ELF_LINE,                 /* -El  --elf-line                           */
  OPT_ELF_LOC,                  /* -Eo  --elf-loc                            */
  OPT_ELF_MACINFO,              /* -Em  --elf-macinfo                        */
  OPT_ELF_PUBNAMES,             /* -Ep  --elf-pubnames                       */
  OPT_ELF_PUBTYPES,             /* -Et  --elf-pubtypes                       */
  OPT_ELF_RANGES,               /* -ER  --elf-ranges                         */
  OPT_ELF_STRINGS,              /* -Es  --elf-strings                        */
  OPT_ELF_TEXT,                 /* -Ex  --elf-text                           */

  /* File Specifications                                                     */
  OPT_FILE_ABI,                 /* -x abi=<abi>    --file-abi=<abi>          */
  OPT_FILE_LINE5,               /* -x line5=<val>  --file-line5=<val>        */
  OPT_FILE_NAME,                /* -x name=<path>  --file-name=<path>        */
  OPT_FILE_OUTPUT,              /* -O file=<path>  --file-output=<path>      */
  OPT_FILE_TIED,                /* -x tied=<path>  --file-tied=<path>        */
  OPT_FILE_USE_NO_LIBELF,       /* --file-use-no-libelf=<path>        */

  /* Print Output Qualifiers                                                 */
  OPT_FORMAT_ATTR_NAME,         /* -M   --format-attr-name                   */
  OPT_FORMAT_DENSE,             /* -d   --format-dense                       */
  OPT_FORMAT_ELLIPSIS,          /* -e   --format-ellipsis                    */
  OPT_FORMAT_EXTENSIONS,        /* -C   --format-extensions                  */
  OPT_FORMAT_GLOBAL_OFFSETS,    /* -G   --format-global-offsets              */
  OPT_FORMAT_LOC,               /* -g   --format-loc                         */
  OPT_FORMAT_REGISTERS,         /* -R   --format-registers                   */
  OPT_FORMAT_SUPPRESS_DATA,     /* -Q   --format-suppress-data               */
  OPT_FORMAT_SUPPRESS_GROUP  ,  /* -x   --format-suppress-group              */
  OPT_FORMAT_SUPPRESS_LOOKUP,   /* -n   --format-suppress-lookup             */
  OPT_FORMAT_SUPPRESS_OFFSETS,  /* -D   --format-suppress-offsets            */
  OPT_FORMAT_SUPPRESS_SANITIZE, /* -x?? --format-suppress-sanitize           */
  OPT_FORMAT_SUPPRESS_URI,      /* -U   --format-suppress-uri                */
  OPT_FORMAT_SUPPRESS_URI_MSG,  /* -q   --format-suppress-uri-msg            */

  /* Print Output Limiters                                                   */
  OPT_FORMAT_FILE,              /* -u<file> --format-file=<file>             */
  OPT_FORMAT_GCC,               /* -cg      --format-gcc                     */
  OPT_FORMAT_GROUP_NUMBER,      /* -x<n>    --format-group-number=<n>        */
  OPT_FORMAT_LIMIT,             /* -H<num>  --format-limit=<num>             */
  OPT_FORMAT_PRODUCER,          /* -c<str>  --format-producer=<str>          */
  OPT_FORMAT_SNC,               /* -cs      --format-snc                     */

  /* Print Debug Sections                                                    */
  OPT_PRINT_ABBREV,             /* -b   --print-abbrev                       */
  OPT_PRINT_ALL,                /* -a   --print-all                          */
  OPT_PRINT_ARANGES,            /* -r   --print-aranges                      */
  OPT_PRINT_DEBUG_NAMES,        /*      --print-debug-name                   */
  OPT_PRINT_EH_FRAME,           /* -F   --print-eh-frame                     */
  OPT_PRINT_FISSION,            /* -I   --print-fission                      */
  OPT_PRINT_FRAME,              /* -f   --print-frame                        */
  OPT_PRINT_INFO,               /* -i   --print-info                         */
  OPT_PRINT_LINES,              /* -l   --print-lines                        */
  OPT_PRINT_LINES_SHORT,        /* -ls  --print-lines-short                  */
  OPT_PRINT_LOC,                /* -c   --print-loc                          */
  OPT_PRINT_MACINFO,            /* -m   --print-macinfo                      */
  OPT_PRINT_PRODUCERS,          /* -P   --print-producers                    */
  OPT_PRINT_PUBNAMES,           /* -p   --print-pubnames                     */
  OPT_PRINT_RANGES,             /* -N   --print-ranges                       */
  OPT_PRINT_STATIC,             /* -ta  --print-static                       */
  OPT_PRINT_STATIC_FUNC,        /* -tf  --print-static-func                  */
  OPT_PRINT_STATIC_VAR,         /* -tv  --print-static-var                   */
  OPT_PRINT_STRINGS,            /* -s   --print-strings                      */
  OPT_PRINT_STR_OFFSETS,        /*      --print-str-offsets                  */
  OPT_PRINT_TYPE,               /* -y   --print-type                         */
  OPT_PRINT_WEAKNAME,           /* -w   --print-weakname                     */

  /* Print Relocations Info                                                  */
  OPT_RELOC,                    /* -o   --reloc                              */
  OPT_RELOC_ABBREV,             /* -oa  --reloc-abbrev                       */
  OPT_RELOC_ARANGES,            /* -or  --reloc-aranges                      */
  OPT_RELOC_FRAMES,             /* -of  --reloc-frames                       */
  OPT_RELOC_INFO,               /* -oi  --reloc-info                         */
  OPT_RELOC_LINE,               /* -ol  --reloc-line                         */
  OPT_RELOC_LOC,                /* -oo  --reloc-loc                          */
  OPT_RELOC_PUBNAMES,           /* -op  --reloc-pubnames                     */
  OPT_RELOC_RANGES,             /* -oR  --reloc-ranges                       */

  /* Search text in attributes                                               */
  OPT_SEARCH_ANY,               /* -S any=<text>   --search-any=<text>       */
  OPT_SEARCH_ANY_COUNT,         /* -Svany=<text>   --search-any-count=<text> */
  OPT_SEARCH_MATCH,             /* -S match=<text> --search-match=<text>     */
  OPT_SEARCH_MATCH_COUNT,       /* -Svmatch=<text> --search-match-count<text>*/
  OPT_SEARCH_PRINT_CHILDREN,    /* -Wc --search-print-children               */
  OPT_SEARCH_PRINT_PARENT,      /* -Wp --search-print-parent                 */
  OPT_SEARCH_PRINT_TREE,        /* -W  --search-print-tree                   */
#ifdef HAVE_REGEX
  OPT_SEARCH_REGEX,             /* -S regex=<text> --search-regex=<text>     */
  OPT_SEARCH_REGEX_COUNT,       /* -Svregex=<text> --search-regex-count<text>*/
#endif /* HAVE_REGEX */

  /* Help & Version                                                          */
  OPT_HELP,                     /* -h  --help                                */
  OPT_VERBOSE,                  /* -v  --verbose                             */
  OPT_VERBOSE_MORE,             /* -vv --verbose-more                        */
  OPT_VERSION,                  /* -V  --version                             */

  /* Trace                                                                   */
  OPT_TRACE,                    /* -# --trace=<num>                           */

  OPT_END,
};

static struct dwoption longopts[] =  {

  /* Check DWARF Integrity. */
  {"check-abbrev",         dwno_argument, 0, OPT_CHECK_ABBREV        },
  {"check-all",            dwno_argument, 0, OPT_CHECK_ALL           },
  {"check-aranges",        dwno_argument, 0, OPT_CHECK_ARANGES       },
  {"check-attr-dup",       dwno_argument, 0, OPT_CHECK_ATTR_DUP      },
  {"check-attr-encodings", dwno_argument, 0, OPT_CHECK_ATTR_ENCODINGS},
  {"check-attr-names",     dwno_argument, 0, OPT_CHECK_ATTR_NAMES    },
  {"check-constants",      dwno_argument, 0, OPT_CHECK_CONSTANTS     },
  {"check-files-lines",    dwno_argument, 0, OPT_CHECK_FILES_LINES   },
  {"check-forward-refs",   dwno_argument, 0, OPT_CHECK_FORWARD_REFS  },
  {"check-frame-basic",    dwno_argument, 0, OPT_CHECK_FRAME_BASIC   },
  {"check-frame-extended", dwno_argument, 0, OPT_CHECK_FRAME_EXTENDED},
  {"check-frame-info",     dwno_argument, 0, OPT_CHECK_FRAME_INFO    },
  {"check-gaps",           dwno_argument, 0, OPT_CHECK_GAPS          },
  {"check-loc",            dwno_argument, 0, OPT_CHECK_LOC           },
  {"check-macros",         dwno_argument, 0, OPT_CHECK_MACROS        },
  {"check-pubnames",       dwno_argument, 0, OPT_CHECK_PUBNAMES      },
  {"check-ranges",         dwno_argument, 0, OPT_CHECK_RANGES        },
  {"check-self-refs",      dwno_argument, 0, OPT_CHECK_SELF_REFS     },
  {"check-show",           dwno_argument, 0, OPT_CHECK_SHOW          },
  {"check-silent",         dwno_argument, 0, OPT_CHECK_SILENT        },
  {"check-summary",        dwno_argument, 0, OPT_CHECK_SUMMARY       },
  {"check-tag-attr",       dwno_argument, 0, OPT_CHECK_TAG_ATTR      },
  {"check-tag-tag",        dwno_argument, 0, OPT_CHECK_TAG_TAG       },
  {"check-type",           dwno_argument, 0, OPT_CHECK_TYPE          },
  {"check-unique",         dwno_argument, 0, OPT_CHECK_UNIQUE        },
#ifdef HAVE_USAGE_TAG_ATTR
  {"check-usage",          dwno_argument, 0, OPT_CHECK_USAGE         },
  {"check-usage-extended", dwno_argument, 0, OPT_CHECK_USAGE_EXTENDED},
#endif /* HAVE_USAGE_TAG_ATTR */

  /* Print ELF sections header. */
  {"elf",          dwno_argument, 0, OPT_ELF         },
  {"elf-abbrev",   dwno_argument, 0, OPT_ELF_ABBREV  },
  {"elf-aranges",  dwno_argument, 0, OPT_ELF_ARANGES },
  {"elf-default",  dwno_argument, 0, OPT_ELF_DEFAULT },
  {"elf-fission",  dwno_argument, 0, OPT_ELF_FISSION },
  {"elf-frames",   dwno_argument, 0, OPT_ELF_FRAMES  },
  {"elf-header",   dwno_argument, 0, OPT_ELF_HEADER  },
  {"elf-info",     dwno_argument, 0, OPT_ELF_INFO    },
  {"elf-line",     dwno_argument, 0, OPT_ELF_LINE    },
  {"elf-loc",      dwno_argument, 0, OPT_ELF_LOC     },
  {"elf-macinfo",  dwno_argument, 0, OPT_ELF_MACINFO },
  {"elf-pubnames", dwno_argument, 0, OPT_ELF_PUBNAMES},
  {"elf-pubtypes", dwno_argument, 0, OPT_ELF_PUBTYPES},
  {"elf-ranges",   dwno_argument, 0, OPT_ELF_RANGES  },
  {"elf-strings",  dwno_argument, 0, OPT_ELF_STRINGS },
  {"elf-text",     dwno_argument, 0, OPT_ELF_TEXT    },

  /* File Specifications. */
  {"file-abi",    dwrequired_argument, 0, OPT_FILE_ABI   },
  {"file-line5",  dwrequired_argument, 0, OPT_FILE_LINE5 },
  {"file-name",   dwrequired_argument, 0, OPT_FILE_NAME  },
  {"file-output", dwrequired_argument, 0, OPT_FILE_OUTPUT},
  {"file-tied",   dwrequired_argument, 0, OPT_FILE_TIED  },
  {"file-use-no-libelf",   dwno_argument, 0, OPT_FILE_USE_NO_LIBELF  },

  /* Print Output Qualifiers. */
  {"format-attr-name",         dwno_argument, 0, OPT_FORMAT_ATTR_NAME        },
  {"format-dense",             dwno_argument, 0, OPT_FORMAT_DENSE            },
  {"format-ellipsis",          dwno_argument, 0, OPT_FORMAT_ELLIPSIS         },
  {"format-extensions",        dwno_argument, 0, OPT_FORMAT_EXTENSIONS       },
  {"format-global-offsets",    dwno_argument, 0, OPT_FORMAT_GLOBAL_OFFSETS   },
  {"format-loc",               dwno_argument, 0, OPT_FORMAT_LOC              },
  {"format-registers",         dwno_argument, 0, OPT_FORMAT_REGISTERS        },
  {"format-suppress-data",     dwno_argument, 0, OPT_FORMAT_SUPPRESS_DATA    },
  {"format-suppress-group",    dwno_argument, 0, OPT_FORMAT_SUPPRESS_GROUP   },
  {"format-suppress-lookup",   dwno_argument, 0, OPT_FORMAT_SUPPRESS_LOOKUP  },
  {"format-suppress-offsets",  dwno_argument, 0, OPT_FORMAT_SUPPRESS_OFFSETS },
  {"format-suppress-sanitize", dwno_argument, 0, OPT_FORMAT_SUPPRESS_SANITIZE},
  {"format-suppress-uri",      dwno_argument, 0, OPT_FORMAT_SUPPRESS_URI     },
  {"format-suppress-uri-msg",  dwno_argument, 0, OPT_FORMAT_SUPPRESS_URI_MSG },

  /* Print Output Limiters. */
  {"format-file",         dwrequired_argument, 0, OPT_FORMAT_FILE        },
  {"format-gcc",          dwno_argument,       0, OPT_FORMAT_GCC         },
  {"format-group-number", dwrequired_argument, 0, OPT_FORMAT_GROUP_NUMBER},
  {"format-limit",        dwrequired_argument, 0, OPT_FORMAT_LIMIT       },
  {"format-producer",     dwrequired_argument, 0, OPT_FORMAT_PRODUCER    },
  {"format-snc",          dwno_argument,       0, OPT_FORMAT_SNC         },

  /* Print Debug Sections. */
  {"print-abbrev",      dwno_argument, 0, OPT_PRINT_ABBREV     },
  {"print-all",         dwno_argument, 0, OPT_PRINT_ALL        },
  {"print-aranges",     dwno_argument, 0, OPT_PRINT_ARANGES    },
  {"print-debug-names", dwno_argument, 0, OPT_PRINT_DEBUG_NAMES},
  {"print-eh-frame",    dwno_argument, 0, OPT_PRINT_EH_FRAME   },
  {"print-fission",     dwno_argument, 0, OPT_PRINT_FISSION    },
  {"print-frame",       dwno_argument, 0, OPT_PRINT_FRAME      },
  {"print-info",        dwno_argument, 0, OPT_PRINT_INFO       },
  {"print-lines",       dwno_argument, 0, OPT_PRINT_LINES      },
  {"print-lines-short", dwno_argument, 0, OPT_PRINT_LINES_SHORT},
  {"print-loc",         dwno_argument, 0, OPT_PRINT_LOC        },
  {"print-macinfo",     dwno_argument, 0, OPT_PRINT_MACINFO    },
  {"print-producers",   dwno_argument, 0, OPT_PRINT_PRODUCERS  },
  {"print-pubnames",    dwno_argument, 0, OPT_PRINT_PUBNAMES   },
  {"print-ranges",      dwno_argument, 0, OPT_PRINT_RANGES     },
  {"print-static",      dwno_argument, 0, OPT_PRINT_STATIC     },
  {"print-static-func", dwno_argument, 0, OPT_PRINT_STATIC_FUNC},
  {"print-static-var",  dwno_argument, 0, OPT_PRINT_STATIC_VAR },
  {"print-strings",     dwno_argument, 0, OPT_PRINT_STRINGS    },
  {"print-str-offsets", dwno_argument, 0, OPT_PRINT_STR_OFFSETS},
  {"print-type",        dwno_argument, 0, OPT_PRINT_TYPE       },
  {"print-weakname",    dwno_argument, 0, OPT_PRINT_WEAKNAME   },

  /* Print Relocations Info. */
  {"reloc",          dwno_argument, 0, OPT_RELOC         },
  {"reloc-abbrev",   dwno_argument, 0, OPT_RELOC_ABBREV  },
  {"reloc-aranges",  dwno_argument, 0, OPT_RELOC_ARANGES },
  {"reloc-frames",   dwno_argument, 0, OPT_RELOC_FRAMES  },
  {"reloc-info",     dwno_argument, 0, OPT_RELOC_INFO    },
  {"reloc-line",     dwno_argument, 0, OPT_RELOC_LINE    },
  {"reloc-loc",      dwno_argument, 0, OPT_RELOC_LOC     },
  {"reloc-pubnames", dwno_argument, 0, OPT_RELOC_PUBNAMES},
  {"reloc-ranges",   dwno_argument, 0, OPT_RELOC_RANGES  },

  /* Search text in attributes. */
  {"search-any",            dwrequired_argument, 0, OPT_SEARCH_ANY           },
  {"search-any-count",      dwrequired_argument, 0, OPT_SEARCH_ANY_COUNT     },
  {"search-match",          dwrequired_argument, 0, OPT_SEARCH_MATCH         },
  {"search-match-count",    dwrequired_argument, 0, OPT_SEARCH_MATCH_COUNT   },
  {"search-print-children", dwno_argument,       0, OPT_SEARCH_PRINT_CHILDREN},
  {"search-print-parent",   dwno_argument,       0, OPT_SEARCH_PRINT_PARENT  },
  {"search-print-tree",     dwno_argument,       0, OPT_SEARCH_PRINT_TREE    },
#ifdef HAVE_REGEX
  {"search-regex",          dwrequired_argument, 0, OPT_SEARCH_REGEX         },
  {"search-regex-count",    dwrequired_argument, 0, OPT_SEARCH_REGEX_COUNT   },
#endif /* HAVE_REGEX */

  /* Help & Version. */
  {"help",          dwno_argument, 0, OPT_HELP         },
  {"verbose",       dwno_argument, 0, OPT_VERBOSE      },
  {"verbose-more",  dwno_argument, 0, OPT_VERBOSE_MORE },
  {"version",       dwno_argument, 0, OPT_VERSION      },

  /* Trace. */
  {"trace", dwrequired_argument, 0, OPT_TRACE},

  {0,0,0,0}
};

/*  Handlers for the command line options. */

/*  Option 'print_debug_names' */
void arg_print_debug_names(void)
{
    glflags.gf_debug_names_flag = TRUE;
}

/*  Option '--print_str_offsets' */
void arg_print_str_offsets(void)
{
    glflags.gf_print_str_offsets = TRUE;
}

/*  Option '-#' */
void arg_trace(void)
{
    int nTraceLevel = atoi(dwoptarg);
    if (nTraceLevel >= 0 && nTraceLevel <= MAX_TRACE_LEVEL) {
        glflags.nTrace[nTraceLevel] = 1;
    }
    /* Display dwarfdump debug options. */
    if (dump_options) {
        print_usage_message(glflags.program_name,usage_debug_text);
        exit(OKAY);
    }
}

/*  Option '-a' */
void arg_print_all(void)
{
    suppress_check_dwarf();
    do_all();
}

/*  Option '-b' */
void arg_print_abbrev(void)
{
    glflags.gf_abbrev_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-c[...]' */
void arg_c_multiple_selection(void)
{
    /* Specify compiler name. */
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 's': arg_format_snc();      break;
        case 'g': arg_format_gcc();      break;
        default:  arg_format_producer(); break;
        }
    } else {
        arg_print_loc();
    }
}

/*  Option '-c' */
void arg_print_loc(void)
{
    glflags.gf_loc_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-cs' */
void arg_format_snc(void)
{
    /* -cs : Check SNC compiler */
    glflags.gf_check_snc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  Option '-cg' */
void arg_format_gcc(void)
{
    /* -cg : Check GCC compiler */
    glflags.gf_check_gcc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  Option '-c<producer>' */
void arg_format_producer(void)
{
    /*  Assume a compiler version to check,
        most likely a substring of a compiler name.  */
    if (!record_producer(dwoptarg)) {
        fprintf(stderr, "Compiler table max %d exceeded, "
            "limiting the tracked compilers to %d\n",
            COMPILER_TABLE_MAX,COMPILER_TABLE_MAX);
    }
}

/*  Option '-C' */
void arg_format_extensions(void)
{
    glflags.gf_suppress_check_extensions_tables = TRUE;
}

/*  Option '-d' */
void arg_format_dense(void)
{
    glflags.gf_do_print_dwarf = TRUE;
    /*  This is sort of useless unless printing,
        but harmless, so we do not insist we
        are printing with suppress_check_dwarf(). */
    glflags.dense = TRUE;
}

/*  Option '-D' */
void arg_format_suppress_offsets(void)
{
    /* Do not emit offset in output */
    glflags.gf_display_offsets = FALSE;
}

/*  Option '-e' */
void arg_format_ellipsis(void)
{
    suppress_check_dwarf();
    glflags.ellipsis = TRUE;
}

/*  Option '-E[...]' */
void arg_E_multiple_selection(void)
{
    /* Object Header information (but maybe really print) */
    /* Selected printing of section info */
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 'a': arg_elf_abbrev();      break;
        case 'd': arg_elf_default();     break;
        case 'f': arg_elf_frames();      break;
        case 'h': arg_elf_header();      break;
        case 'i': arg_elf_info();        break;
        case 'I': arg_elf_fission();     break;
        case 'l': arg_elf_line();        break;
        case 'm': arg_elf_macinfo();     break;
        case 'o': arg_elf_loc();         break;
        case 'p': arg_elf_pubnames();    break;
        case 'r': arg_elf_aranges();     break;
        case 'R': arg_elf_ranges();      break;
        case 's': arg_elf_strings();     break;
        case 't': arg_elf_pubtypes();    break;
        case 'x': arg_elf_text();        break;
        default: arg_usage_error = TRUE; break;
        }
    } else {
        arg_elf();
    }
}

/*  Option '-E' */
void arg_elf(void)
{
    /* Display header and all sections info */
    glflags.gf_header_flag = TRUE;
    set_all_sections_on();
}

/*  Option '-Ea' */
void arg_elf_abbrev(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_ABBREV);
}

/*  Option '-Ed' */
void arg_elf_default(void)
{
    /* case 'd', use the default section set */
    glflags.gf_header_flag = TRUE;
    set_all_section_defaults();
}

/*  Option '-Ef' */
void arg_elf_frames(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_FRAME);
}

/*  Option '-Eh' */
void arg_elf_header(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_HEADER);
}

/*  Option '-Ei' */
void arg_elf_info(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_INFO);
    enable_section_map_entry(DW_HDR_DEBUG_TYPES);
}

/*  Option '-EI' */
void arg_elf_fission(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_GDB_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_CU_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_TU_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_NAMES);
}

/*  Option '-El' */
void arg_elf_line(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_LINE);
}

/*  Option '-Em' */
void arg_elf_macinfo(void)
{
    /*  For both old macinfo and dwarf5 macro */
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_MACINFO);
}

/*  Option '-Eo' */
void arg_elf_loc(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_LOC);
}

/*  Option '-Ep' */
void arg_elf_pubnames(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_PUBNAMES);
}

/*  Option '-Er' */
void arg_elf_aranges(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_ARANGES);
}

/*  Option '-ER' */
void arg_elf_ranges(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_RANGES);
    enable_section_map_entry(DW_HDR_DEBUG_RNGLISTS);
}

/*  Option '-Es' */
void arg_elf_strings(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_STR);
}

/*  Option '-Et' */
void arg_elf_pubtypes(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_PUBTYPES);
}

/*  Option '-Ex' */
void arg_elf_text(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_TEXT);
}

/*  Option '-f' */
void arg_print_debug_frame(void)
{
    glflags.gf_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-F' */
void arg_print_gnu_frame(void)
{
    glflags.gf_eh_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-g' */
void arg_format_loc(void)
{
    /*info_flag = TRUE;  removed  from -g. Nov 2015 */
    glflags.gf_use_old_dwarf_loclist = TRUE;
    suppress_check_dwarf();
}

/*  Option '-G' */
void arg_format_global_offsets(void)
{
    glflags.gf_show_global_offsets = TRUE;
}

/*  Option '-h' */
void arg_h_multiple_selection(void)
{
    if (dwoptarg) {
        arg_usage_error = TRUE;
    } else {
        arg_help();
    }
}

/*  Option '-h' */
void arg_help(void)
{
    print_usage_message(glflags.program_name,usage_long_text);
    exit(OKAY);
}

/*  Option '-H' */
void arg_format_limit(void)
{
    int break_val = atoi(dwoptarg);
    if (break_val > 0) {
        glflags.break_after_n_units = break_val;
    }
}

/*  Option '-i' */
void arg_print_info(void)
{
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-I' */
void arg_print_fission(void)
{
    glflags.gf_gdbindex_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-k[...]' */
void arg_k_multiple_selection(void)
{
    switch (dwoptarg[0]) {
    case 'a': arg_check_all();             break;
    case 'b': arg_check_abbrev();          break;
    case 'c': arg_check_constants();       break;
    case 'd': arg_check_show();            break;
    case 'D': arg_check_attr_dup();        break;
    case 'e': arg_check_pubnames();        break;
    case 'E': arg_check_attr_encodings();  break;
    case 'f': arg_check_frame_info();      break;
    case 'F': arg_check_files_lines();     break;
    case 'g': arg_check_gaps();            break;
    case 'G': arg_check_unique();          break;
    case 'i': arg_check_summary();         break;
    case 'l': arg_check_loc();             break;
    case 'm': arg_check_ranges();          break;
    case 'M': arg_check_aranges();         break;
    case 'n': arg_check_attr_names();      break;
    case 'r': arg_check_tag_attr();        break;
    case 'R': arg_check_forward_refs();    break;
    case 's': arg_check_silent();          break;
    case 'S': arg_check_self_refs();       break;
    case 't': arg_check_tag_tag();         break;
#ifdef HAVE_USAGE_TAG_ATTR
    case 'u': arg_ku_multiple_selection(); break;
#endif /* HAVE_USAGE_TAG_ATTR */
    case 'w': arg_check_macros();          break;
    case 'x': arg_kx_multiple_selection(); break;
    case 'y': arg_check_type();            break;
    default: arg_usage_error = TRUE;       break;
    }
}

/*  Option '-ka' */
void arg_check_all(void)
{
    suppress_print_dwarf();
    glflags.gf_check_pubname_attr = TRUE;
    glflags.gf_check_attr_tag = TRUE;
    glflags.gf_check_tag_tree = TRUE;
    glflags.gf_check_type_offset = TRUE;
    glflags.gf_check_names = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_gdbindex_flag = TRUE;
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_check_macros = TRUE;
    glflags.gf_check_frames = TRUE;
    glflags.gf_check_frames_extended = FALSE;
    glflags.gf_check_locations = TRUE;
    glflags.gf_frame_flag = TRUE;
    glflags.gf_eh_frame_flag = TRUE;
    glflags.gf_check_ranges = TRUE;
    glflags.gf_check_lines = TRUE;
    glflags.gf_check_fdes = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_aranges = TRUE;
    glflags.gf_aranges_flag = TRUE;  /* Aranges section */
    glflags.gf_check_abbreviations = TRUE;
    glflags.gf_check_dwarf_constants = TRUE;
    glflags.gf_check_di_gaps = TRUE;
    glflags.gf_check_forward_decl = TRUE;
    glflags.gf_check_self_references = TRUE;
    glflags.gf_check_attr_encoding = TRUE;
    glflags.gf_print_usage_tag_attr = TRUE;
    glflags.gf_check_duplicated_attributes = TRUE;
}

/*  Option '-kb' */
void arg_check_abbrev(void)
{
    /* Abbreviations */
    suppress_print_dwarf();
    glflags.gf_check_abbreviations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  Option '-kc' */
void arg_check_constants(void)
{
    /* DWARF constants */
    suppress_print_dwarf();
    glflags.gf_check_dwarf_constants = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kd' */
void arg_check_show(void)
{
    /* Display check results */
    suppress_print_dwarf();
    glflags.gf_check_show_results = TRUE;
}

/*  Option '-kD' */
void arg_check_attr_dup(void)
{
    /* Check duplicated attributes */
    suppress_print_dwarf();
    glflags.gf_check_duplicated_attributes = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  Option '-ke' */
void arg_check_pubnames(void)
{
    suppress_print_dwarf();
    glflags.gf_check_pubname_attr = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  Option '-kE' */
void arg_check_attr_encodings(void)
{
    /* Attributes encoding usage */
    suppress_print_dwarf();
    glflags.gf_check_attr_encoding = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kf' */
void arg_check_frame_info(void)
{
    suppress_print_dwarf();
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  Option '-kF' */
void arg_check_files_lines(void)
{
    /* files-lines */
    suppress_print_dwarf();
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_check_lines = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kg' */
void arg_check_gaps(void)
{
    /* Check debug info gaps */
    suppress_print_dwarf();
    glflags.gf_check_di_gaps = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kG' */
void arg_check_unique(void)
{
    /* Print just global (unique) errors */
    suppress_print_dwarf();
    glflags.gf_print_unique_errors = TRUE;
}

/*  Option '-ki' */
void arg_check_summary(void)
{
    /* Summary for each compiler */
    suppress_print_dwarf();
    glflags.gf_print_summary_all = TRUE;
}

/*  Option '-kl' */
void arg_check_loc(void)
{
    /* Locations list */
    suppress_print_dwarf();
    glflags.gf_check_locations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_loc_flag = TRUE;
}

/*  Option '-km' */
void arg_check_ranges(void)
{
    /* Ranges */
    suppress_print_dwarf();
    glflags.gf_check_ranges = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kM' */
void arg_check_aranges(void)
{
    /* Aranges */
    suppress_print_dwarf();
    glflags.gf_check_aranges = TRUE;
    glflags.gf_aranges_flag = TRUE;
}

/*  Option '-kn' */
void arg_check_attr_names(void)
{
    /* invalid names */
    suppress_print_dwarf();
    glflags.gf_check_names = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kr' */
void arg_check_tag_attr(void)
{
    suppress_print_dwarf();
    glflags.gf_check_attr_tag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
}

/*  Option '-kR' */
void arg_check_forward_refs(void)
{
    /* forward declarations in DW_AT_specification */
    suppress_print_dwarf();
    glflags.gf_check_forward_decl = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-ks' */
void arg_check_silent(void)
{
    /* Check verbose mode */
    suppress_print_dwarf();
    glflags.gf_check_verbose_mode = FALSE;
}

/*  Option '-kS' */
void arg_check_self_refs(void)
{
    /*  self references in:
        DW_AT_specification, DW_AT_type, DW_AT_abstract_origin */
    suppress_print_dwarf();
    glflags.gf_check_self_references = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kt' */
void arg_check_tag_tag(void)
{
    suppress_print_dwarf();
    glflags.gf_check_tag_tree = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

#ifdef HAVE_USAGE_TAG_ATTR
/*  Option '-ku[...]' */
void arg_ku_multiple_selection(void)
{
    /* Tag-Tree and Tag-Attr usage */
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'f': arg_check_usage_extended(); break;
        default: arg_usage_error = TRUE;      break;
        }
    } else {
        arg_check_usage();
    }
}

/*  Option '-ku' */
void arg_check_usage(void)
{
    suppress_print_dwarf();
    glflags.gf_print_usage_tag_attr = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kuf' */
void arg_check_usage_extended(void)
{
    arg_check_usage();

    /* -kuf : Full report */
    glflags.gf_print_usage_tag_attr_full = TRUE;
}
#endif /* HAVE_USAGE_TAG_ATTR */

/*  Option '-kw' */
void arg_check_macros(void)
{
    suppress_print_dwarf();
    glflags.gf_check_macros = TRUE;
    glflags.gf_macro_flag = TRUE;
    glflags.gf_macinfo_flag = TRUE;
}

/*  Option '-kx[...]' */
void arg_kx_multiple_selection(void)
{
    /* Frames check */
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'e': arg_check_frame_extended(); break;
        default: arg_usage_error = TRUE;      break;
        }
    } else {
        arg_check_frame_basic();
    }
}

/*  Option '-kx' */
void arg_check_frame_basic(void)
{
    suppress_print_dwarf();
    glflags.gf_check_frames = TRUE;
    glflags.gf_frame_flag = TRUE;
    glflags.gf_eh_frame_flag = TRUE;
}

/*  Option '-kxe' */
void arg_check_frame_extended(void)
{
    arg_check_frame_basic();

    /* -xe : Extended frames check */
    glflags.gf_check_frames = FALSE;
    glflags.gf_check_frames_extended = TRUE;
}

/*  Option '-ky' */
void arg_check_type(void)
{
    suppress_print_dwarf();
    glflags.gf_check_type_offset = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_pubtypes_flag = TRUE;
    glflags.gf_check_ranges = TRUE;
    glflags.gf_check_aranges = TRUE;
}

/*  Option '-l[...]' */
void arg_l_multiple_selection(void)
{
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 's': arg_print_lines_short(); break;
        default: arg_usage_error = TRUE;   break;
        }
    } else {
        arg_print_lines();
    }
}

/*  Option '-l' */
void arg_print_lines(void)
{
    /* Enable to suppress offsets printing */
    glflags.gf_line_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-ls' */
void arg_print_lines_short(void)
{
  /* -ls : suppress <pc> addresses */
  glflags.gf_line_print_pc = FALSE;
  arg_print_lines();
}

/*  Option '-m' */
void arg_print_macinfo(void)
{
    glflags.gf_macinfo_flag = TRUE; /* DWARF2,3,4 */
    glflags.gf_macro_flag   = TRUE; /* DWARF5 */
    suppress_check_dwarf();
}

/*  Option '-M' */
void arg_format_attr_name(void)
{
    glflags.show_form_used =  TRUE;
}

/*  Option '-n' */
void arg_format_suppress_lookup(void)
{
    glflags.gf_suppress_nested_name_search = TRUE;
}

/*  Option '-N' */
void arg_print_ranges(void)
{
    glflags.gf_ranges_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-o[...]' */
void arg_o_multiple_selection(void)
{
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 'a': arg_reloc_abbrev();    break;
        case 'i': arg_reloc_info();      break;
        case 'l': arg_reloc_line();      break;
        case 'p': arg_reloc_pubnames();  break;
        case 'r': arg_reloc_aranges();   break;
        case 'f': arg_reloc_frames();    break;
        case 'o': arg_reloc_loc();       break;
        case 'R': arg_reloc_ranges();    break;
        default: arg_usage_error = TRUE; break;
        }
    } else {
        arg_reloc();
    }
}

/*  Option '-o' */
void arg_reloc(void)
{
    glflags.gf_reloc_flag = TRUE;
    set_all_reloc_sections_on();
}

/*  Option '-oa' */
void arg_reloc_abbrev(void)
{
    /*  Case a has no effect, no relocations can point out
        of the abbrev section. */
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_ABBREV);
}

/*  Option '-of' */
void arg_reloc_frames(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_FRAME);
}

/*  Option '-oi' */
void arg_reloc_info(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_INFO);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_TYPES);
}

/*  Option '-ol' */
void arg_reloc_line(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LINE);
}

/*  Option '-oo' */
void arg_reloc_loc(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LOC);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LOCLISTS);
}

/*  Option '-op' */
void arg_reloc_pubnames(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_PUBNAMES);
}

/*  Option '-or' */
void arg_reloc_aranges(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_ARANGES);
}

/*  Option '-oR' */
void arg_reloc_ranges(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_RANGES);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_RNGLISTS);
}

/*  Option '-O' */
void arg_O_multiple_selection(void)
{
    /* Output filename */
    /*  -O file=<filename> */
    if (strncmp(dwoptarg,"file=",5) == 0) {
        dwoptarg = &dwoptarg[5];
        arg_file_output();
    } else {
        arg_usage_error = TRUE;
    }
}

/*  Option '-O file=' */
void arg_file_output(void)
{
    const char *ctx = arg_option > OPT_BEGIN ? "--file-output=" : "-O file=";

    const char *path = do_uri_translation(dwoptarg,ctx);
    if (strlen(path) > 0) {
        glflags.output_file = path;
    } else {
        arg_usage_error = TRUE;
    }
}

/*  Option '-p' */
void arg_print_pubnames(void)
{
    glflags.gf_pubnames_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-P' */
void arg_print_producers(void)
{
    /* List of CUs per compiler */
    glflags.gf_producer_children_flag = TRUE;
}

/*  Option '-q' */
void arg_format_suppress_uri_msg(void)
{
    /* Suppress uri-did-translate notification */
    glflags.gf_do_print_uri_in_input = FALSE;
}

/*  Option '-Q' */
void arg_format_suppress_data(void)
{
    /* Q suppresses section data printing. */
    glflags.gf_do_print_dwarf = FALSE;
}

/*  Option '-r' */
void arg_print_aranges(void)
{
    glflags.gf_aranges_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-R' */
void arg_format_registers(void)
{
    glflags.gf_generic_1200_regs = TRUE;
}

/*  Option '-s' */
void arg_print_strings(void)
{
    glflags.gf_string_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-S' */
void arg_S_multiple_selection(void)
{
    /* 'v' option, to print number of occurrences */
    /* -S[v]match|any|regex=text*/
    if (dwoptarg[0] == 'v') {
        ++dwoptarg;
        arg_search_count();
    }

    if (strncmp(dwoptarg,"match=",6) == 0) {
        dwoptarg = &dwoptarg[6];
        arg_search_match();
    } else if (strncmp(dwoptarg,"any=",4) == 0) {
        dwoptarg = &dwoptarg[4];
        arg_search_any();
    }
#ifdef HAVE_REGEX
    else if (strncmp(dwoptarg,"regex=",6) == 0) {
        dwoptarg = &dwoptarg[6];
        arg_search_regex();
    }
#endif /* HAVE_REGEX */
    else {
        arg_search_invalid();
    }
}

/*  Option '-S any=' */
void arg_search_any(void)
{
    const char *tempstr = 0;
    const char *ctx = arg_option > OPT_BEGIN ? "--search-any=" : "-S any=";

    /* -S any=<text> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_any_text = makename(dwoptarg);
    tempstr = remove_quotes_pair(glflags.search_any_text);
    glflags.search_any_text = do_uri_translation(tempstr,ctx);
    if (strlen(glflags.search_any_text) <= 0) {
        arg_search_invalid();
    }
}

/*  Option '-Sv any=' */
void arg_search_any_count(void)
{
    arg_search_count();
    arg_search_any();
}

/*  Option '-S match=' */
void arg_search_match(void)
{
    const char *tempstr = 0;
    const char *ctx = arg_option > OPT_BEGIN ? "--search-match=" : "-S match=";

    /* -S match=<text> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_match_text = makename(dwoptarg);
    tempstr = remove_quotes_pair(glflags.search_match_text);
    glflags.search_match_text = do_uri_translation(tempstr,ctx);
    if (strlen(glflags.search_match_text) <= 0) {
        arg_search_invalid();
    }
}

/*  Option '-Sv match=' */
void arg_search_match_count(void)
{
    arg_search_count();
    arg_search_match();
}

#ifdef HAVE_REGEX
/*  Option '-S regex=' */
void arg_search_regex(void)
{
    const char *tempstr = 0;
    const char *ctx = arg_option > OPT_BEGIN ? "--search-regex=" : "-S regex=";

    /* -S regex=<regular expression> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_regex_text = makename(dwoptarg);
    tempstr = remove_quotes_pair(glflags.search_regex_text);
    glflags.search_regex_text = do_uri_translation(tempstr,ctx);
    if (strlen(glflags.search_regex_text) > 0) {
        if (regcomp(glflags.search_re,
            glflags.search_regex_text,
            REG_EXTENDED)) {
            fprintf(stderr,
                "regcomp: unable to compile %s\n",
                glflags.search_regex_text);
        }
    } else {
        arg_search_invalid();
    }
}

/*  Option '-Sv regex=' */
void arg_search_regex_count(void)
{
    arg_search_count();
    arg_search_regex();
}
#endif /* HAVE_REGEX */

/*  Option '-Sv' */
void arg_search_count(void)
{
    glflags.gf_search_print_results = TRUE;
}

/*  Option '-t' */
void arg_t_multiple_selection(void)
{
    switch (dwoptarg[0]) {
    case 'a': arg_print_static();      break;
    case 'f': arg_print_static_func(); break;
    case 'v': arg_print_static_var();  break;
    default: arg_usage_error = TRUE;   break;
    }
}

/*  Option '-ta' */
void arg_print_static(void)
{
    /* all */
    glflags.gf_static_func_flag =  TRUE;
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-tf' */
void arg_print_static_func(void)
{
    /* .debug_static_func */
    glflags.gf_static_func_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-tv' */
void arg_print_static_var(void)
{
    /* .debug_static_var */
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-u' */
void arg_format_file(void)
{
    const char *ctx = arg_option > OPT_BEGIN ? "--format-file=" : "-u<cu name>";

    /* compile unit */
    const char *tstr = 0;
    glflags.gf_cu_name_flag = TRUE;
    tstr = do_uri_translation(dwoptarg,ctx);
    esb_append(glflags.cu_name,tstr);
}

/*  Option '-U' */
void arg_format_suppress_uri(void)
{
    glflags.gf_uri_options_translation = FALSE;
}

/*  Option '-v' */
void arg_verbose(void)
{
    glflags.verbose++;
}

/*  Option '-V' */
void arg_version(void)
{
    /* Display dwarfdump compilation date and time */
    print_version_details(glflags.program_fullname,TRUE);
    exit(OKAY);
}

/*  Option '-w' */
void arg_print_weaknames(void)
{
    /* .debug_weaknames */
    glflags.gf_weakname_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-W[...]' */
void arg_W_multiple_selection(void)
{
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 'c': arg_search_print_children(); break;
        case 'p': arg_search_print_parent();   break;
        default: arg_usage_error = TRUE;       break;
        }
    } else {
        arg_search_print_tree();
    }
}

/*  Option '-W' */
void arg_search_print_tree(void)
{
    /* Search results in wide format */
    glflags.gf_search_wide_format = TRUE;

    /* -W : Display parent and children tree */
    glflags.gf_display_children_tree = TRUE;
    glflags.gf_display_parent_tree = TRUE;
}

/*  Option '-Wc' */
void arg_search_print_children(void)
{
    /* -Wc : Display children tree */
    arg_search_print_tree();
    glflags.gf_display_children_tree = TRUE;
    glflags.gf_display_parent_tree = FALSE;
}

/*  Option '-Wp' */
void arg_search_print_parent(void)
{
    /* -Wp : Display parent tree */
    arg_search_print_tree();
    glflags.gf_display_children_tree = FALSE;
    glflags.gf_display_parent_tree = TRUE;
}

/*  Option '-x[...]' */
void arg_x_multiple_selection(void)
{
    if (strncmp(dwoptarg,"name=",5) == 0) {
        dwoptarg = &dwoptarg[5];
        arg_file_name();
    } else if (strncmp(dwoptarg,"abi=",4) == 0) {
        dwoptarg = &dwoptarg[4];
        arg_file_abi();
    } else if (strncmp(dwoptarg,"groupnumber=",12) == 0) {
        dwoptarg = &dwoptarg[12];
        arg_format_groupnumber();
    } else if (strncmp(dwoptarg,"tied=",5) == 0) {
        dwoptarg = &dwoptarg[5];
        arg_file_tied();
    } else if (strncmp(dwoptarg,"line5=",6) == 0) {
        dwoptarg = &dwoptarg[6];
        arg_file_line5();
    } else if (strcmp(dwoptarg,"nosanitizestrings") == 0) {
        arg_format_suppress_sanitize();
    } else if (strcmp(dwoptarg,"noprintsectiongroups") == 0) {
        arg_format_suppress_group();
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x abi=' */
static void
arg_file_abi(void)
{
    const char *ctx = arg_option > OPT_BEGIN ? "--file-abi=" : "-x abi=";

    /*  -x abi=<abi> meaning select abi from dwarfdump.conf
        file. Must always select abi to use dwarfdump.conf */
    const char *abi = do_uri_translation(dwoptarg,ctx);
    if (strlen(abi) > 0) {
        config_file_abi = abi;
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x groupnumber=' */
static void
arg_format_groupnumber(void)
{
    /*  By default prints the lowest groupnumber in the object.
        Default is  -x groupnumber=0
        For group 1 (standard base dwarfdata)
            -x groupnumber=1
        For group 1 (DWARF5 .dwo sections and dwp data)
            -x groupnumber=2 */
    long int gnum = 0;

    int res = get_number_value(dwoptarg,&gnum);
    if (res == DW_DLV_OK) {
        glflags.group_number = gnum;
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x line5=' */
static void
arg_file_line5(void)
{
    if (!strcmp(dwoptarg,"std")) {
        glflags.gf_line_flag_selection = singledw5;
    } else if (!strcmp(dwoptarg,"s2l")) {
        glflags.gf_line_flag_selection= s2l;
    } else if (!strcmp(dwoptarg,"orig")) {
        glflags.gf_line_flag_selection= orig;
    } else if (!strcmp(dwoptarg,"orig2l")) {
        glflags.gf_line_flag_selection= orig2l;
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x name=' */
static void arg_file_name(void)
{
    const char *ctx = arg_option > OPT_BEGIN ? "--file-name=" : "-x name=";

    /*  -x name=<path> meaning name dwarfdump.conf file. */
    const char *path = do_uri_translation(dwoptarg,ctx);
    if (strlen(path) > 0) {
        esb_empty_string(glflags.config_file_path);
        esb_append(glflags.config_file_path,path);
    } else {
        arg_x_invalid();
    }
}

/*  Option '-x noprintsectiongroups' */
static void arg_format_suppress_group(void)
{
    glflags.gf_section_groups_flag = FALSE;
}

/*  Option '-x nosanitizestrings' */
static void arg_format_suppress_sanitize(void)
{
    no_sanitize_string_garbage = TRUE;
}

/*  Option '-x tied=' */
static void arg_file_tied(void)
{
    const char *ctx = arg_option > OPT_BEGIN ? "--file-tied=" : "-x tied=";

    const char *tiedpath = do_uri_translation(dwoptarg,ctx);
    if (strlen(tiedpath) > 0) {
        esb_empty_string(glflags.config_file_tiedpath);
        esb_append(glflags.config_file_tiedpath,tiedpath);
    } else {
        arg_x_invalid();
    }
}

/*  Option '--file-use-no-libelf' */
static void arg_file_use_no_libelf(void)
{
    glflags.gf_file_use_no_libelf = TRUE;
}

/*  Option '-y' */

/*  Option '-y' */
static void arg_print_types(void)
{
    /* .debug_pubtypes */
    /* Also for SGI-only, and obsolete, .debug_typenames */
    suppress_check_dwarf();
    glflags.gf_pubtypes_flag = TRUE;
}

/*  Option not supported */
static void arg_not_supported(void)
{
    fprintf(stderr, "-%c is no longer supported:ignored\n",arg_option);
}

/*  Error message for invalid '-S' option. */
static void arg_search_invalid(void)
{
    fprintf(stderr,
        "-S any=<text> or -S match=<text> or"
        " -S regex=<text>\n");
    fprintf(stderr, "is allowed, not -S %s\n",dwoptarg);
    arg_usage_error = TRUE;
}

/*  Error message for invalid '-x' option. */
static void arg_x_invalid(void)
{
    fprintf(stderr, "-x name=<path-to-conf> \n");
    fprintf(stderr, " and  \n");
    fprintf(stderr, "-x abi=<abi-in-conf> \n");
    fprintf(stderr, " and  \n");
    fprintf(stderr, "-x tied=<tied-file-path> \n");
    fprintf(stderr, " and  \n");
    fprintf(stderr, "-x line5={std,s2l,orig,orig2l} \n");
    fprintf(stderr, " and  \n");
    fprintf(stderr, "-x nosanitizestrings \n");
    fprintf(stderr, "are legal, not -x %s\n", dwoptarg);
    arg_usage_error = TRUE;
}

/*  Process the command line arguments and sets the appropiated options. All
    the options are within the global flags structure. */
static void
set_command_options(int argc, char *argv[])
{
    int longindex = 0;

    /* j unused */
    while ((arg_option = dwgetopt_long(argc, argv,
        "#:abc::CdDeE::fFgGhH:iIk:l::mMnNo::O:pPqQrRsS:t:u:UvVwW::x:yz",
        longopts,&longindex)) != EOF) {

        switch (arg_option) {
        case '#': arg_trace();                   break;
        case 'a': arg_print_all();               break;
        case 'b': arg_print_abbrev();            break;
        case 'c': arg_c_multiple_selection();    break;
        case 'C': arg_format_extensions();       break;
        case 'd': arg_format_dense();            break;
        case 'D': arg_format_suppress_offsets(); break;
        case 'e': arg_format_ellipsis();         break;
        case 'E': arg_E_multiple_selection();    break;
        case 'f': arg_print_debug_frame();       break;
        case 'F': arg_print_gnu_frame();         break;
        case 'g': arg_format_loc();              break;
        case 'G': arg_format_global_offsets();   break;
        case 'h': arg_h_multiple_selection();    break;
        case 'H': arg_format_limit();            break;
        case 'i': arg_print_info();              break;
        case 'I': arg_print_fission();           break;
        case 'k': arg_k_multiple_selection();    break;
        case 'l': arg_l_multiple_selection();    break;
        case 'm': arg_print_macinfo();           break;
        case 'M': arg_format_attr_name();        break;
        case 'n': arg_format_suppress_lookup();  break;
        case 'N': arg_print_ranges();            break;
        case 'o': arg_o_multiple_selection();    break;
        case 'O': arg_O_multiple_selection();    break;
        case 'p': arg_print_pubnames();          break;
        case 'P': arg_print_producers();         break;
        case 'q': arg_format_suppress_uri_msg(); break;
        case 'Q': arg_format_suppress_data();    break;
        case 'r': arg_print_aranges();           break;
        case 'R': arg_format_registers();        break;
        case 's': arg_print_strings();           break;
        case 'S': arg_S_multiple_selection();    break;
        case 't': arg_t_multiple_selection();    break;
        case 'u': arg_format_file();             break;
        case 'U': arg_format_suppress_uri();     break;
        case 'v': arg_verbose();                 break;
        case 'V': arg_version();                 break;
        case 'w': arg_print_weaknames();         break;
        case 'W': arg_W_multiple_selection();    break;
        case 'x': arg_x_multiple_selection();    break;
        case 'y': arg_print_types();             break;
        case 'z': arg_not_supported();           break;

        /* Check DWARF Integrity. */
        case OPT_CHECK_ABBREV:         arg_check_abbrev();         break;
        case OPT_CHECK_ALL:            arg_check_all();            break;
        case OPT_CHECK_ARANGES:        arg_check_aranges();        break;
        case OPT_CHECK_ATTR_DUP:       arg_check_attr_dup();       break;
        case OPT_CHECK_ATTR_ENCODINGS: arg_check_attr_encodings(); break;
        case OPT_CHECK_ATTR_NAMES:     arg_check_attr_names();     break;
        case OPT_CHECK_CONSTANTS:      arg_check_constants();      break;
        case OPT_CHECK_FILES_LINES:    arg_check_files_lines();    break;
        case OPT_CHECK_FORWARD_REFS:   arg_check_forward_refs();   break;
        case OPT_CHECK_FRAME_BASIC:    arg_check_frame_basic();    break;
        case OPT_CHECK_FRAME_EXTENDED: arg_check_frame_extended(); break;
        case OPT_CHECK_FRAME_INFO:     arg_check_frame_info();     break;
        case OPT_CHECK_GAPS:           arg_check_gaps();           break;
        case OPT_CHECK_LOC:            arg_check_loc();            break;
        case OPT_CHECK_MACROS:         arg_check_macros();         break;
        case OPT_CHECK_PUBNAMES:       arg_check_pubnames();       break;
        case OPT_CHECK_RANGES:         arg_check_ranges();         break;
        case OPT_CHECK_SELF_REFS:      arg_check_self_refs();      break;
        case OPT_CHECK_SHOW:           arg_check_show();           break;
        case OPT_CHECK_SILENT:         arg_check_silent();         break;
        case OPT_CHECK_SUMMARY:        arg_check_summary();        break;
        case OPT_CHECK_TAG_ATTR:       arg_check_tag_attr();       break;
        case OPT_CHECK_TAG_TAG:        arg_check_tag_tag();        break;
        case OPT_CHECK_TYPE:           arg_check_type();           break;
        case OPT_CHECK_UNIQUE:         arg_check_unique();         break;
    #ifdef HAVE_USAGE_TAG_ATTR
        case OPT_CHECK_USAGE:          arg_check_usage();          break;
        case OPT_CHECK_USAGE_EXTENDED: arg_check_usage_extended(); break;
    #endif /* HAVE_USAGE_TAG_ATTR */

        /* Print ELF sections header. */
        case OPT_ELF:           arg_elf();          break;
        case OPT_ELF_ABBREV:    arg_elf_abbrev();   break;
        case OPT_ELF_ARANGES:   arg_elf_aranges();  break;
        case OPT_ELF_DEFAULT:   arg_elf_default();  break;
        case OPT_ELF_FISSION:   arg_elf_fission();  break;
        case OPT_ELF_FRAMES:    arg_elf_frames();   break;
        case OPT_ELF_HEADER:    arg_elf_header();   break;
        case OPT_ELF_INFO:      arg_elf_info();     break;
        case OPT_ELF_LINE:      arg_elf_line();     break;
        case OPT_ELF_LOC:       arg_elf_loc();      break;
        case OPT_ELF_MACINFO:   arg_elf_macinfo();  break;
        case OPT_ELF_PUBNAMES:  arg_elf_pubnames(); break;
        case OPT_ELF_PUBTYPES:  arg_elf_pubtypes(); break;
        case OPT_ELF_RANGES:    arg_elf_ranges();   break;
        case OPT_ELF_STRINGS:   arg_elf_strings();  break;
        case OPT_ELF_TEXT:      arg_elf_text();     break;

        /* File Specifications. */
        case OPT_FILE_ABI:    arg_file_abi();    break;
        case OPT_FILE_LINE5:  arg_file_line5();  break;
        case OPT_FILE_NAME:   arg_file_name();   break;
        case OPT_FILE_OUTPUT: arg_file_output(); break;
        case OPT_FILE_TIED:   arg_file_tied();   break;
        case OPT_FILE_USE_NO_LIBELF:   arg_file_use_no_libelf();   break;

        /* Print Output Qualifiers. */
        case OPT_FORMAT_ATTR_NAME:        arg_format_attr_name();        break;
        case OPT_FORMAT_DENSE:            arg_format_dense();            break;
        case OPT_FORMAT_ELLIPSIS:         arg_format_ellipsis();         break;
        case OPT_FORMAT_EXTENSIONS:       arg_format_extensions();       break;
        case OPT_FORMAT_GLOBAL_OFFSETS:   arg_format_global_offsets();   break;
        case OPT_FORMAT_LOC:              arg_format_loc();              break;
        case OPT_FORMAT_REGISTERS:        arg_format_registers();        break;
        case OPT_FORMAT_SUPPRESS_DATA:    arg_format_suppress_data();    break;
        case OPT_FORMAT_SUPPRESS_GROUP:   arg_format_suppress_group();   break;
        case OPT_FORMAT_SUPPRESS_OFFSETS: arg_format_suppress_offsets(); break;
        case OPT_FORMAT_SUPPRESS_LOOKUP:  arg_format_suppress_lookup();  break;
        case OPT_FORMAT_SUPPRESS_SANITIZE:arg_format_suppress_sanitize();break;
        case OPT_FORMAT_SUPPRESS_URI:     arg_format_suppress_uri();     break;
        case OPT_FORMAT_SUPPRESS_URI_MSG: arg_format_suppress_uri_msg(); break;

        /* Print Output Limiters. */
        case OPT_FORMAT_FILE:         arg_format_file();        break;
        case OPT_FORMAT_GCC:          arg_format_gcc();         break;
        case OPT_FORMAT_GROUP_NUMBER: arg_format_groupnumber(); break;
        case OPT_FORMAT_LIMIT:        arg_format_limit();       break;
        case OPT_FORMAT_PRODUCER:     arg_format_producer();    break;
        case OPT_FORMAT_SNC:          arg_format_snc();         break;

        /* Print Debug Sections. */
        case OPT_PRINT_ABBREV:      arg_print_abbrev();      break;
        case OPT_PRINT_ALL:         arg_print_all();         break;
        case OPT_PRINT_ARANGES:     arg_print_aranges();     break;
        case OPT_PRINT_DEBUG_NAMES: arg_print_debug_names(); break;
        case OPT_PRINT_EH_FRAME:    arg_print_gnu_frame();   break;
        case OPT_PRINT_FISSION:     arg_print_fission();     break;
        case OPT_PRINT_FRAME:       arg_print_debug_frame(); break;
        case OPT_PRINT_INFO:        arg_print_info();        break;
        case OPT_PRINT_LINES:       arg_print_lines();       break;
        case OPT_PRINT_LINES_SHORT: arg_print_lines_short(); break;
        case OPT_PRINT_LOC:         arg_print_loc();         break;
        case OPT_PRINT_MACINFO:     arg_print_macinfo();     break;
        case OPT_PRINT_PRODUCERS:   arg_print_producers();   break;
        case OPT_PRINT_PUBNAMES:    arg_print_pubnames();    break;
        case OPT_PRINT_RANGES:      arg_print_ranges();      break;
        case OPT_PRINT_STATIC:      arg_print_static();      break;
        case OPT_PRINT_STATIC_FUNC: arg_print_static_func(); break;
        case OPT_PRINT_STATIC_VAR:  arg_print_static_var();  break;
        case OPT_PRINT_STRINGS:     arg_print_strings();     break;
        case OPT_PRINT_STR_OFFSETS: arg_print_str_offsets(); break;
        case OPT_PRINT_TYPE:        arg_print_types();       break;
        case OPT_PRINT_WEAKNAME:    arg_print_weaknames();   break;

        /* Print Relocations Info. */
        case OPT_RELOC:          arg_reloc();          break;
        case OPT_RELOC_ABBREV:   arg_reloc_abbrev();   break;
        case OPT_RELOC_ARANGES:  arg_reloc_aranges();  break;
        case OPT_RELOC_FRAMES:   arg_reloc_frames();   break;
        case OPT_RELOC_INFO:     arg_reloc_info();     break;
        case OPT_RELOC_LINE:     arg_reloc_line();     break;
        case OPT_RELOC_LOC:      arg_reloc_loc();      break;
        case OPT_RELOC_PUBNAMES: arg_reloc_pubnames(); break;
        case OPT_RELOC_RANGES:   arg_reloc_ranges();   break;

        /* Search text in attributes. */
        case OPT_SEARCH_ANY:            arg_search_any();            break;
        case OPT_SEARCH_ANY_COUNT:      arg_search_any_count();      break;
        case OPT_SEARCH_MATCH:          arg_search_match();          break;
        case OPT_SEARCH_MATCH_COUNT:    arg_search_match_count();    break;
        case OPT_SEARCH_PRINT_CHILDREN: arg_search_print_children(); break;
        case OPT_SEARCH_PRINT_PARENT:   arg_search_print_parent();   break;
        case OPT_SEARCH_PRINT_TREE:     arg_search_print_tree();     break;
    #ifdef HAVE_REGEX
        case OPT_SEARCH_REGEX:          arg_search_regex();          break;
        case OPT_SEARCH_REGEX_COUNT:    arg_search_regex_count();    break;
    #endif /* HAVE_REGEX */

        /* Help & Version. */
        case OPT_HELP:          arg_help();          break;
        case OPT_VERBOSE:       arg_verbose();       break;
        case OPT_VERBOSE_MORE:  arg_verbose();       break;
        case OPT_VERSION:       arg_version();       break;

        /* Trace. */
        case OPT_TRACE: arg_trace(); break;

        default: arg_usage_error = TRUE; break;
        }
    }
}

/* process arguments and return object filename */
const char *
process_args(int argc, char *argv[])
{
    glflags.program_name = special_program_name(argv[0]);
    glflags.program_fullname = argv[0];

    suppress_check_dwarf();
    if (argv[1] != NULL && argv[1][0] != '-') {
        do_all();
    }
    glflags.gf_section_groups_flag = TRUE;

    /*  Process the arguments and sets the appropiated option */
    set_command_options(argc, argv);

    init_conf_file_data(glflags.config_file_data);
    if (config_file_abi && glflags.gf_generic_1200_regs) {
        printf("Specifying both -R and -x abi= is not allowed. Use one "
            "or the other.  -x abi= ignored.\n");
        config_file_abi = FALSE;
    }
    if (glflags.gf_generic_1200_regs) {
        init_generic_config_1200_regs(glflags.config_file_data);
    }
    if (config_file_abi &&
        (glflags.gf_frame_flag || glflags.gf_eh_frame_flag)) {
        int res = 0;
        res = find_conf_file_and_read_config(
            esb_get_string(glflags.config_file_path),
            config_file_abi,
            config_file_defaults,
            glflags.config_file_data);

        if (res > 0) {
            printf
                ("Frame not configured due to error(s). Giving up.\n");
            glflags.gf_eh_frame_flag = FALSE;
            glflags.gf_frame_flag = FALSE;
        }
    }
    if (arg_usage_error ) {
        printf("%s option error.\n",glflags.program_name);
        printf("To see the options list: %s -h\n",glflags.program_name);
        exit(FAILED);
    }
    if (dwoptind != (argc - 1)) {
        printf("No object file name provided to %s\n",glflags.program_name);
        printf("To see the options list: %s -h\n",glflags.program_name);
        exit(FAILED);
    }
    /*  FIXME: it seems silly to be printing section names
        where the section does not exist in the object file.
        However we continue the long-standard practice
        of printing such by default in most cases. For now. */
    if (glflags.group_number == DW_GROUPNUMBER_DWO) {
        /*  For split-dwarf/DWO some sections make no sense.
            This prevents printing of meaningless headers where no
            data can exist. */
        glflags.gf_pubnames_flag = FALSE;
        glflags.gf_eh_frame_flag = FALSE;
        glflags.gf_frame_flag    = FALSE;
        glflags.gf_macinfo_flag  = FALSE;
        glflags.gf_aranges_flag  = FALSE;
        glflags.gf_ranges_flag   = FALSE;
        glflags.gf_static_func_flag = FALSE;
        glflags.gf_static_var_flag = FALSE;
        glflags.gf_weakname_flag = FALSE;
    }
    if (glflags.group_number > DW_GROUPNUMBER_BASE) {
        /* These no longer apply, no one uses. */
        glflags.gf_static_func_flag = FALSE;
        glflags.gf_static_var_flag = FALSE;
        glflags.gf_weakname_flag = FALSE;
        glflags.gf_pubnames_flag = FALSE;
    }

    if (glflags.gf_do_check_dwarf) {
        /* Reduce verbosity when checking (checking means checking-only). */
        glflags.verbose = 1;
    }
    return do_uri_translation(argv[dwoptind],"file-to-process");
}

/*
  Copyright (C) 2000,2002,2004,2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2019 David Anderson. All Rights Reserved.
  Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
  Portions Copyright 2012 SN Systems Ltd. All rights reserved.

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

/* The address of the Free Software Foundation is
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.
   SGI has moved from the Crittenden Lane address.
*/

#include "globals.h"
/* for 'open' */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for dup2() */
#elif defined(_WIN32) && defined(_MSC_VER)
#include <io.h>
#endif

#include "makename.h"
#include "macrocheck.h"
#include "dwconf.h"
#include "dwconf_using_functions.h"
#include "common.h"
#include "helpertree.h"
#include "esb.h"                /* For flexible string buffer. */
#include "esb_using_functions.h"
#include "sanitized.h"
#include "tag_common.h"
#include "libdwarf_version.h" /* for DW_VERSION_DATE_STR */

#include "command_options.h"
#include "compiler_info.h"

#ifndef O_RDONLY
/*  This is for a Windows environment */
# define O_RDONLY _O_RDONLY
#endif

#ifdef _O_BINARY
/*  This is for a Windows environment */
#define O_BINARY _O_BINARY
#else
# ifndef O_BINARY
# define O_BINARY 0  /* So it does nothing in Linux/Unix */
# endif
#endif /* O_BINARY */

#ifdef HAVE_ELF_OPEN
extern int elf_open(const char *name,int mode);
#endif /* HAVE_ELF_OPEN */

#ifdef HAVE_CUSTOM_LIBELF
extern int elf_is_custom_format(void *header, size_t headerlen, size_t *size,
    unsigned *endian, unsigned *offsetsize, int *errcode);
#endif /* HAVE_CUSTOM_LIBELF */

#define BYTES_PER_INSTRUCTION 4

/*  The type of Bucket. */
#define KIND_RANGES_INFO       1
#define KIND_SECTIONS_INFO     2
#define KIND_VISITED_INFO      3

/* Build section information */
void build_linkonce_info(Dwarf_Debug dbg);

struct glflags_s glflags;

/* Functions used to manage the unique errors table */
static void allocate_unique_errors_table(void);
static void release_unique_errors_table(void);
#ifdef TESTING
static void dump_unique_errors_table(void);
#endif
static boolean add_to_unique_errors_table(char * error_text);

static struct esb_s esb_short_cu_name;
static struct esb_s esb_long_cu_name;
static struct esb_s dwarf_error_line;

static int process_one_file(int fd, int tiedfd,
    Elf *efp, Elf * tiedfp,
    const char * file_name,
    const char * tied_file_name,
#ifdef DWARF_WITH_LIBELF
    int archive,
#endif
    struct dwconf_s *conf);

static void
print_gnu_debuglink(Dwarf_Debug dbg);


static int
open_a_file(const char * name)
{
    /* Set to a file number that cannot be legal. */
    int fd = -1;

#if HAVE_ELF_OPEN
    /*  It is not possible to share file handles
        between applications or DLLs. Each application has its own
        file-handle table. For two applications to use the same file
        using a DLL, they must both open the file individually.
        Let the 'libelf' dll open and close the file.  */
    fd = elf_open(name, O_RDONLY | O_BINARY);
#else
    fd = open(name, O_RDONLY | O_BINARY);
#endif
    return fd;

}
static void
close_a_file(int f)
{
    if (f != -1) {
        close(f);
    }
}

#ifdef DWARF_WITH_LIBELF
static int
is_it_known_elf_header(Elf *elf)
{
    Elf32_Ehdr *eh32;

    eh32 = elf32_getehdr(elf);
    if (eh32) {
        return 1;
    }
#ifdef HAVE_ELF64_GETEHDR
    {
        Elf64_Ehdr *eh64;
        /* not a 32-bit obj */
        eh64 = elf64_getehdr(elf);
        if (eh64) {
            return 1;
        }
    }
#endif /* HAVE_ELF64_GETEHDR */
    /* Not something we can handle. */
    return 0;
}
#endif /* DWARF_WITH_LIBELF */

static void
check_for_major_errors(void)
{
    if (glflags.gf_count_major_errors) {
#if 0
        causes hundreds of test mismatches, so not reporting this.
        printf("There were %ld DWARF errors reported: "
            "see ERROR above\n",
            glflags.gf_count_major_errors);
#endif /* 0 */
        exit(FAILED);
    }
    return;
}

static void
flag_data_pre_allocation(void)
{
    memset(glflags.section_high_offsets_global,0,
        sizeof(*glflags.section_high_offsets_global));
    /*  If we are checking .debug_line, .debug_ranges, .debug_aranges,
        or .debug_loc build the tables containing
        the pairs LowPC and HighPC. It is safer  (and not
        expensive) to build all
        of these at once so mistakes in options do not lead
        to coredumps (like -ka -p did once). */
    if (glflags.gf_check_decl_file || glflags.gf_check_ranges ||
        glflags.gf_check_locations ||
        glflags.gf_do_check_dwarf ||
        glflags.gf_check_self_references) {
        glflags.pRangesInfo = AllocateBucketGroup(KIND_RANGES_INFO);
        glflags.pLinkonceInfo = AllocateBucketGroup(KIND_SECTIONS_INFO);
        glflags.pVisitedInfo = AllocateBucketGroup(KIND_VISITED_INFO);
    }
    /* Create the unique error table */
    if (glflags.gf_print_unique_errors) {
        allocate_unique_errors_table();
    }
    /* Allocate range array to be used by all CUs */
    if (glflags.gf_check_ranges) {
        allocate_range_array_info();
    }
}

static void flag_data_post_cleanup(void)
{
#ifdef DWARF_WITH_LIBELF
    clean_up_syms_malloc_data();
#endif /* DWARF_WITH_LIBELF */
    if (glflags.pRangesInfo) {
        ReleaseBucketGroup(glflags.pRangesInfo);
        glflags.pRangesInfo = 0;
    }
    if (glflags.pLinkonceInfo) {
        ReleaseBucketGroup(glflags.pLinkonceInfo);
        glflags.pLinkonceInfo = 0;
    }
    if (glflags.pVisitedInfo) {
        ReleaseBucketGroup(glflags.pVisitedInfo);
        glflags.pVisitedInfo = 0;
    }
    /* Release range array to be used by all CUs */
    if (glflags.gf_check_ranges) {
        release_range_array_info();
    }
    /* Delete the unique error set */
    if (glflags.gf_print_unique_errors) {
        release_unique_errors_table();
    }
    clean_up_compilers_detected();
    destruct_abbrev_array();
}

#ifdef DWARF_WITH_LIBELF
static int
process_using_libelf(int fd, int tiedfd,
    const char *file_name,
    char       *out_path_buf,
    const char *tied_file_name,
    int archive)
{
    Elf_Cmd cmd = 0;
    Elf *arf = 0;
    Elf *elf = 0;
    Elf *elftied = 0;
    int archmemnum = 0;

    (void) elf_version(EV_NONE);
    if (elf_version(EV_CURRENT) == EV_NONE) {
        (void) fprintf(stderr,
            "dwarfdump: libelf.a out of date.\n");
        exit(FAILED);
    }

    /*  We will use libelf to process an archive
        so long as is convienient.
        we don't intend to ever write our own
        archive reader.  Archive support was never
        tested and may disappear. */
    cmd = ELF_C_READ;
    arf = elf_begin(fd, cmd, (Elf *) 0);
    if (!arf) {
        fprintf(stderr, "%s ERROR:  "
            "Unable to obtain ELF descriptor for %s\n",
            glflags.program_name,
            file_name);
        free(out_path_buf);
        return (FAILED);
    }
    if (esb_string_len(glflags.config_file_tiedpath) > 0) {
        int isknown = 0;
        if (tiedfd == -1) {
            fprintf(stderr, "%s ERROR:  "
                "can't open tied file %s\n",
                glflags.program_name,
                tied_file_name);
            free(out_path_buf);
            return (FAILED);
        }
        elftied = elf_begin(tiedfd, cmd, (Elf *) 0);
        if (elf_kind(elftied) == ELF_K_AR) {
            fprintf(stderr, "%s ERROR:  tied file  %s is "
                "an archive. Not allowed. Giving up.\n",
                glflags.program_name,
                tied_file_name);
            free(out_path_buf);
            return (FAILED);
        }
        isknown = is_it_known_elf_header(elftied);
        if (!isknown) {
            fprintf(stderr,
                "Cannot process tied file %s: unknown format\n",
                tied_file_name);
            free(out_path_buf);
            return FAILED;
        }
    }
    while ((elf = elf_begin(fd, cmd, arf)) != 0) {
        int isknown = is_it_known_elf_header(elf);

        if (!isknown) {
            /* not a 64-bit obj either! */
            /* dwarfdump is almost-quiet when not an object */
            if (archive) {
                Elf_Arhdr *mem_header = elf_getarhdr(elf);
                const char *memname =
                    (mem_header && mem_header->ar_name)?
                    mem_header->ar_name:"";

                /*  / and // archive entries are not archive
                    objects, but are not errors.
                    For the ATT/USL type of archive. */
                if (strcmp(memname,"/") && strcmp(memname,"//")) {
                    fprintf(stderr, "Can't process archive member "
                        "%d %s of %s: unknown format\n",
                        archmemnum,
                        sanitized(memname),
                        file_name);
                }
            } else {
                fprintf(stderr, "Can't process %s: unknown format\n",
                    file_name);
            }
            glflags.check_error = 1;
            cmd = elf_next(elf);
            elf_end(elf);
            continue;
        }
        flag_data_pre_allocation();
        process_one_file(fd,tiedfd,
            elf,elftied,
            file_name,
            tied_file_name,
            archive,
            glflags.config_file_data);
        flag_data_post_cleanup();
        cmd = elf_next(elf);
        elf_end(elf);
        archmemnum += 1;
    }
    elf_end(arf);
    if (elftied) {
        elf_end(elftied);
        elftied = 0;
    }
    return 0; /* normal return. */
}
#endif /* DWARF_WITH_LIBELF */


/*
   Iterate through dwarf and print all info.
*/
int
main(int argc, char *argv[])
{
    const char * file_name = 0;
    const char * tied_file_name = 0;
    int fd = -1;
    int tiedfd = -1;
    unsigned         ftype = 0;
    unsigned         endian = 0;
    unsigned         offsetsize = 0;
    Dwarf_Unsigned   filesize = 0;
    int      errcode = 0;
    char *out_path_buf = 0;
    unsigned out_path_buf_len = 0;
    int res = 0;

#ifdef _WIN32
    /*  Open the null device used during formatting printing */
    if (!esb_open_null_device()) {
        fprintf(stderr,"dwarfdump: Unable to open null device.\n");
        exit(FAILED);
    }
#endif /* _WIN32 */

    /*  Global flags initialization and esb-buffers construction. */
    init_global_flags();

    set_checks_off();
    esb_constructor(&esb_short_cu_name);
    esb_constructor(&esb_long_cu_name);
    esb_constructor(&dwarf_error_line);

#ifdef _WIN32
    /*  Often we redirect the output to a file, but we have found
        issues due to the buffering associated with stdout. Some issues
        were fixed just by the use of 'fflush', but the main issued
        remained.
        The stdout stream is buffered, so will only display what's in the
        buffer after it reaches a newline (or when it's told to). We have a
        few options to print immediately:
        - Print to stderr instead using fprintf.
        - Print to stdout and flush stdout whenever we need it to using fflush.
        - We can also disable buffering on stdout by using setbuf:
            setbuf(stdout,NULL);
            Make stdout unbuffered; this seems to work for all cases.
        The problem is no longer present. September 2018.
    */

    /*  Calling setbuf() with NULL argument, it turns off
        all buffering for the specified stream.
        Then writing to and/or reading from the stream
        will be exactly as directed by the program.
        But if dwarfdump is used over a network drive,
        it shows a dramatic
        slowdown when sending the output to a file.
        An operation that takes
        couple of seconds, it was taking few hours. */
    /*  setbuf(stdout,NULL); */
    /*  Redirect stderr to stdout. */
    dup2(fileno(stdout),fileno(stderr));
#endif /* _WIN32 */

    print_version_details(argv[0],FALSE);
    file_name = process_args(argc, argv);
    print_args(argc,argv);

    /*  Redirect stdout and stderr to an specific file */
    if (glflags.output_file) {
        if (NULL == freopen(glflags.output_file,"w",stdout)) {
            fprintf(stderr,
                "dwarfdump: Unable to redirect output to '%s'\n",
                glflags.output_file);
            exit(FAILED);
        }
        dup2(fileno(stdout),fileno(stderr));
        /* Record version and arguments in the output file */
        print_version_details(argv[0],FALSE);
        print_args(argc,argv);
    }

    /*  Because LibDwarf now generates some new warnings,
        allow the user to hide them by using command line options */
    {
        Dwarf_Cmdline_Options wcmd;
        /* The struct has just one field!. */
        wcmd.check_verbose_mode = glflags.gf_check_verbose_mode;
        dwarf_record_cmdline_options(wcmd);
    }
    /*  The 100+2 etc is more than suffices for the expansion that a
        MacOS dsym might need. */
    out_path_buf_len = strlen(file_name)*2 + 100 + 2;
    out_path_buf = malloc(out_path_buf_len);
    if(!out_path_buf) {
        fprintf(stderr, "%s ERROR:  Unable to malloc %lu bytes "
            "for possible path string %s.\n",
            glflags.program_name,(unsigned long)out_path_buf_len,
            file_name);
        return (FAILED);
    }
    res = dwarf_object_detector_path(file_name,
        out_path_buf,out_path_buf_len,
        &ftype,&endian,&offsetsize,&filesize,&errcode);
    if ( res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            char *errmsg = dwarf_errmsg_by_number(errcode);
            fprintf(stderr, "%s ERROR:  can't open %s:"
                " %s\n",
                glflags.program_name, file_name,
                errmsg);
        } else {
            fprintf(stderr, "%s ERROR:  Can't open %s\n",
                glflags.program_name, file_name);
        }
        free(out_path_buf);
        return (FAILED);
    }
    if (strcmp(file_name,out_path_buf)) {
        /* We have a MacOS dsym, file_name altered */
        file_name = makename(out_path_buf);
    }
    fd = open_a_file(file_name);
    if (fd == -1) {
        fprintf(stderr, "%s ERROR:  can't open %s\n",
            glflags.program_name,
            file_name);
        free(out_path_buf);
        return (FAILED);
    }

    if (esb_string_len(glflags.config_file_tiedpath) > 0) {
        unsigned         tftype = 0;
        unsigned         tendian = 0;
        unsigned         toffsetsize = 0;
        Dwarf_Unsigned   tfilesize = 0;

        tied_file_name = esb_get_string(glflags.config_file_tiedpath);
        res = dwarf_object_detector_path(file_name,
            out_path_buf,out_path_buf_len,
            &tftype,&tendian,&toffsetsize,&tfilesize,&errcode);
        if ( res != DW_DLV_OK) {
            if (res == DW_DLV_ERROR) {
                char *errmsg = dwarf_errmsg_by_number(errcode);
                fprintf(stderr, "%s ERROR:  can't open tied file %s:"
                    " %s\n",
                    glflags.program_name, tied_file_name,
                    errmsg);
            } else {
                fprintf(stderr,
                    "%s ERROR: tied file not an object file '%s'.\n",
                    glflags.program_name, tied_file_name);
            }
            free(out_path_buf);
            return (FAILED);
        }
        if (ftype != tftype || endian != tendian ||
            offsetsize != toffsetsize) {
            fprintf(stderr, "%s ERROR:  tied file \'%s\' and "
                "main file \'%s\' not "
                "the same kind of object!\n",
                glflags.program_name, tied_file_name,out_path_buf);
            free(out_path_buf);
            return (FAILED);
        }
        if (strcmp(file_name,out_path_buf)) {
            /*  We have a MacOS dsym, file_name altered.
                Can this really happen with a tied file? */
            esb_empty_string(glflags.config_file_tiedpath);
            esb_append(glflags.config_file_tiedpath,out_path_buf);
            tied_file_name = out_path_buf;
        }
        tiedfd = open_a_file(tied_file_name);
        if (tiedfd == -1) {
            fprintf(stderr, "%s ERROR:  can't open tied file %s\n",
                glflags.program_name,
                tied_file_name);
            free(out_path_buf);
            return (FAILED);
        }
    }
    if ( (ftype == DW_FTYPE_ELF && (glflags.gf_reloc_flag ||
        glflags.gf_header_flag)) ||
#ifdef HAVE_CUSTOM_LIBELF
        ftype == DW_FTYPE_CUSTOM_ELF ||
#endif /* HAVE_CUSTOM_LIBELF */
        ftype == DW_FTYPE_ARCHIVE) {
#ifdef DWARF_WITH_LIBELF
        int excode = 0;

        excode = process_using_libelf(fd,tiedfd,file_name,
            out_path_buf, tied_file_name,
            (ftype == DW_FTYPE_ARCHIVE)? TRUE:FALSE);
        if (excode) {
            free(out_path_buf);
            exit(excode);
        }
#else /* !DWARF_WITH_LIBELF */
        fprintf(stderr, "Can't process %s: archives and "
            "printing elf headers not supported in this dwarfdump "
            "--disable-libelf build.\n",
            file_name);
#endif /* DWARF_WITH_LIBELF */
    } else if (ftype == DW_FTYPE_ELF) {
        flag_data_pre_allocation();
        process_one_file(fd,tiedfd,
            0,0,
            file_name,
            tied_file_name,
#ifdef DWARF_WITH_LIBELF
            0 /* elf_archive */,
#endif
            glflags.config_file_data);
        flag_data_post_cleanup();
    } else if (ftype == DW_FTYPE_MACH_O) {
        flag_data_pre_allocation();
        process_one_file(fd,tiedfd,
            0,0,
            file_name,
            tied_file_name,
#ifdef DWARF_WITH_LIBELF
            0 /* mach_o_archive */,
#endif
            glflags.config_file_data);
        flag_data_post_cleanup();
    } else if (ftype == DW_FTYPE_PE) {

        flag_data_pre_allocation();
        process_one_file(fd,tiedfd,
            0,0,
            file_name,
            tied_file_name,
#ifdef DWARF_WITH_LIBELF
            0/* pe_archive */,
#endif
            glflags.config_file_data);
        flag_data_post_cleanup();
    } else {
        fprintf(stderr, "Can't process %s: unhandled format\n",
            file_name);
    }
    free(out_path_buf);
    out_path_buf = 0;

    /*  These cleanups only necessary once all
        objects processed. */
#ifdef HAVE_REGEX
    if (glflags.search_regex_text) {
        regfree(glflags.search_re);
    }
#endif
    makename_destructor();
    esb_destructor(&esb_long_cu_name);
    esb_destructor(&esb_short_cu_name);
    esb_destructor(&dwarf_error_line);
    sanitized_string_destructor();
    ranges_esb_string_destructor();

    /*  Global flags initialization and esb-buffers destruction. */
    reset_global_flags();

    close_a_file(fd);
    close_a_file(tiedfd);

#ifdef _WIN32
    /* Close the null device used during formatting printing */
    esb_close_null_device();
#endif /* _WIN32 */

    check_for_major_errors();
    if (glflags.gf_count_major_errors) {
        printf("There were %ld DWARF errors reported: "
            "see ERROR above\n",
            glflags.gf_count_major_errors);
        exit(FAILED);
    }
    /*  As the tool have reached this point, it means there are
        no internal errors and we should return an OKAY condition,
        regardless if the file being processed has
        minor errors. */
    return OKAY;
}

void
print_any_harmless_errors(Dwarf_Debug dbg)
{
#define LOCAL_PTR_ARY_COUNT 50
    /*  We do not need to initialize the local array,
        libdwarf does it. */
    const char *buf[LOCAL_PTR_ARY_COUNT];
    unsigned totalcount = 0;
    unsigned i = 0;
    unsigned printcount = 0;
    int res = dwarf_get_harmless_error_list(dbg,LOCAL_PTR_ARY_COUNT,buf,
        &totalcount);
    if (res == DW_DLV_NO_ENTRY) {
        return;
    }
    if (totalcount > 0) {
        printf("\n*** HARMLESS ERROR COUNT: %u ***\n",totalcount);
    }
    for (i = 0 ; buf[i]; ++i) {
        ++printcount;
        DWARF_CHECK_COUNT(harmless_result,1);
        DWARF_CHECK_ERROR(harmless_result,buf[i]);
    }
    if (totalcount > printcount) {
        /*harmless_result.checks += (totalcount - printcount); */
        DWARF_CHECK_COUNT(harmless_result,(totalcount - printcount));
        /*harmless_result.errors += (totalcount - printcount); */
        DWARF_ERROR_COUNT(harmless_result,(totalcount - printcount));
    }
}

/* Print a summary of search results */
static void
print_search_results(void)
{
    const char *search_type = 0;
    const char *search_text = 0;
    if (glflags.search_any_text) {
        search_type = "any";
        search_text = glflags.search_any_text;
    } else {
        if (glflags.search_match_text) {
            search_type = "match";
            search_text = glflags.search_match_text;
        } else {
            search_type = "regex";
            search_text = glflags.search_regex_text;
        }
    }
    fflush(stdout);
    fflush(stderr);
    printf("\nSearch type      : '%s'\n",search_type);
    printf("Pattern searched : '%s'\n",search_text);
    printf("Occurrences Found: %d\n",glflags.search_occurrences);
    fflush(stdout);
}

/* This is for dwarf_print_lines() */
static void
printf_callback_for_libdwarf(UNUSEDARG void *userdata,
    const char *data)
{
    printf("%s",data);
}


/*  Does not return on error. */
void
get_address_size_and_max(Dwarf_Debug dbg,
   Dwarf_Half * size,
   Dwarf_Addr * max,
   Dwarf_Error *aerr)
{
    int dres = 0;
    Dwarf_Half lsize = 4;
    /* Get address size and largest representable address */
    dres = dwarf_get_address_size(dbg,&lsize,aerr);
    if (dres != DW_DLV_OK) {
        print_error(dbg, "get_address_size()", dres, *aerr);
    }
    if(max) {
        *max = (lsize == 8 ) ? 0xffffffffffffffffULL : 0xffffffff;
    }
    if(size) {
        *size = lsize;
    }
}


/* dbg is often null when dbgtied was passed in. */
static void
dbgsetup(Dwarf_Debug dbg,struct dwconf_s *setup_config_file_data)
{
    if (!dbg) {
        return;
    }
    dwarf_set_frame_rule_initial_value(dbg,
        setup_config_file_data->cf_initial_rule_value);
    dwarf_set_frame_rule_table_size(dbg,
        setup_config_file_data->cf_table_entry_count);
    dwarf_set_frame_cfa_value(dbg,
        setup_config_file_data->cf_cfa_reg);
    dwarf_set_frame_same_value(dbg,
        setup_config_file_data->cf_same_val);
    dwarf_set_frame_undefined_value(dbg,
        setup_config_file_data->cf_undefined_val);
    if (setup_config_file_data->cf_address_size) {
        dwarf_set_default_address_size(dbg,
            setup_config_file_data->cf_address_size);
    }
    dwarf_set_harmless_error_list_size(dbg,50);
}

/*  Callable at any time, Sets section sizes with the sizes
    known as of the call.
    Repeat whenever about to  reference a size that might not
    have been set as of the last call. */
static void
set_global_section_sizes(Dwarf_Debug dbg)
{
    dwarf_get_section_max_offsets_c(dbg,
        &glflags.section_high_offsets_global->debug_info_size,
        &glflags.section_high_offsets_global->debug_abbrev_size,
        &glflags.section_high_offsets_global->debug_line_size,
        &glflags.section_high_offsets_global->debug_loc_size,
        &glflags.section_high_offsets_global->debug_aranges_size,
        &glflags.section_high_offsets_global->debug_macinfo_size,
        &glflags.section_high_offsets_global->debug_pubnames_size,
        &glflags.section_high_offsets_global->debug_str_size,
        &glflags.section_high_offsets_global->debug_frame_size,
        &glflags.section_high_offsets_global->debug_ranges_size,
        &glflags.section_high_offsets_global->debug_pubtypes_size,
        &glflags.section_high_offsets_global->debug_types_size,
        &glflags.section_high_offsets_global->debug_macro_size,
        &glflags.section_high_offsets_global->debug_str_offsets_size,
        &glflags.section_high_offsets_global->debug_sup_size,
        &glflags.section_high_offsets_global->debug_cu_index_size,
        &glflags.section_high_offsets_global->debug_tu_index_size);
}

/*
  Given a file which we know is an elf file, process
  the dwarf data.

*/
static int
process_one_file(int fd, int tiedfd,
    Elf *elf, Elf *tiedelf,
    const char * file_name,
    const char * tied_file_name,
#ifdef DWARF_WITH_LIBELF
    int archive,
#endif
    struct dwconf_s *l_config_file_data)
{
    Dwarf_Debug dbg = 0;
    Dwarf_Debug dbgtied = 0;
    int dres = 0;
    struct Dwarf_Printf_Callback_Info_s printfcallbackdata;
    Dwarf_Half elf_address_size = 0;      /* Target pointer size */
    Dwarf_Error onef_err = 0;
    const char *title = 0;

    /*  If using a tied file group number should be 2 DW_GROUPNUMBER_DWO
        but in a dwp or separate-split-dwarf object then
        0 will find the .dwo data automatically. */
    if (elf) {
        title = "dwarf_elf_init_b";
        dres = dwarf_elf_init_b(elf, DW_DLC_READ,glflags.group_number,
            NULL, NULL, &dbg, &onef_err);
    } else {

        title = "dwarf_init_b";
        dres = dwarf_init_b(fd, DW_DLC_READ,glflags.group_number,
            NULL, NULL, &dbg, &onef_err);
    }
    if (dres == DW_DLV_NO_ENTRY) {
        if (glflags.group_number > 0) {
            printf("No DWARF information present in %s "
                "for section group %d \n", file_name,glflags.group_number);
        } else {
            printf("No DWARF information present in %s\n",file_name);
        }
        return 0;
    }
    if (dres != DW_DLV_OK) {
        print_error(dbg, title, dres, onef_err);
    }

    dres = dwarf_add_file_path(dbg,file_name,&onef_err);
    if (dres != DW_DLV_OK) {
        print_error(dbg,"Unable to add file path to object file data", dres, onef_err);
    }

    if (tiedelf || tiedfd >= 0) {
        if (tiedelf) {
            dres = dwarf_elf_init_b(tiedelf, DW_DLC_READ,
                DW_GROUPNUMBER_BASE, NULL, NULL, &dbgtied,
                &onef_err);
        } else {
            /*  The tied file we define as group 1, BASE. */
            dres = dwarf_init_b(tiedfd, DW_DLC_READ,
                DW_GROUPNUMBER_BASE, NULL, NULL, &dbgtied,
                &onef_err);
        }
        if (dres == DW_DLV_NO_ENTRY) {
            printf("No DWARF information present in tied file: %s\n",
                tied_file_name);
            return 0;
        }
        if (dres != DW_DLV_OK) {
            print_error(dbg, "dwarf_elf_init on tied_file", dres, onef_err);
        }
        dres = dwarf_add_file_path(dbgtied,tied_file_name,&onef_err);
        if (dres != DW_DLV_OK) {
            print_error(dbg, "Unable to add tied file name to tied file", dres, onef_err);
        }
    }

    memset(&printfcallbackdata,0,sizeof(printfcallbackdata));
    printfcallbackdata.dp_fptr = printf_callback_for_libdwarf;
    dwarf_register_printf_callback(dbg,&printfcallbackdata);
    if (dbgtied) {
        dwarf_register_printf_callback(dbgtied,&printfcallbackdata);
    }
    memset(&printfcallbackdata,0,sizeof(printfcallbackdata));

    dbgsetup(dbg,l_config_file_data);
    dbgsetup(dbgtied,l_config_file_data);
    get_address_size_and_max(dbg,&elf_address_size,0,&onef_err);
#ifdef DWARF_WITH_LIBELF
    if (archive) {
        Elf_Arhdr *mem_header = elf_getarhdr(elf);
        const char *memname =
            (mem_header && mem_header->ar_name)?
            mem_header->ar_name:"";

        printf("\narchive member \t%s\n",sanitized(memname));
    }
#endif /* DWARF_WITH_LIBELF */

    /*  Ok for dbgtied to be NULL. */
    dres = dwarf_set_tied_dbg(dbg,dbgtied,&onef_err);
    if (dres != DW_DLV_OK) {
        print_error(dbg, "dwarf_set_tied_dbg() failed", dres, onef_err);
    }

    /* Get .text and .debug_ranges info if in check mode */
    if (glflags.gf_do_check_dwarf) {
        Dwarf_Addr lower = 0;
        Dwarf_Addr upper = 0;
        Dwarf_Unsigned size = 0;
        int res = 0;
        res = dwarf_get_section_info_by_name(dbg,".text",&lower,&size,&onef_err);
        if (DW_DLV_OK == res) {
            upper = lower + size;
        }

        /* Set limits for Ranges Information */
        if (glflags.pRangesInfo) {
            SetLimitsBucketGroup(glflags.pRangesInfo,lower,upper);
        }

        /* Build section information */
        build_linkonce_info(dbg);
    }

    if (glflags.gf_header_flag && elf) {
#ifdef DWARF_WITH_LIBELF
        print_object_header(dbg);
#endif /* DWARF_WITH_LIBELF */
    }

    if (glflags.gf_section_groups_flag) {
        print_section_groups_data(dbg);
        /*  If groupnum > 2 this turns off some
            of the gf_flags here so we don't print
            section names of things we do not
            want to print. */
        update_section_flags_per_groups(dbg);
    }

    reset_overall_CU_error_data();
    if (glflags.gf_info_flag || glflags.gf_line_flag ||
        glflags.gf_types_flag ||
        glflags.gf_check_macros || glflags.gf_macinfo_flag ||
        glflags.gf_macro_flag ||
        glflags.gf_cu_name_flag || glflags.gf_search_is_on ||
        glflags.gf_producer_children_flag) {

        print_infos(dbg,TRUE);
        reset_overall_CU_error_data();
        print_infos(dbg,FALSE);
        if (glflags.gf_check_macros) {
            set_global_section_sizes(dbg);
            if(macro_check_tree) {
                /* Fake item representing end of section. */
                /* Length of the fake item is zero. */
                print_macro_statistics("DWARF5 .debug_macro",
                    &macro_check_tree,
                    glflags.section_high_offsets_global->debug_macro_size);
            }
            if(macinfo_check_tree) {
                /* Fake item representing end of section. */
                /* Length of the fake item is zero. */
                print_macro_statistics("DWARF2 .debug_macinfo",
                    &macinfo_check_tree,
                    glflags.section_high_offsets_global->debug_macinfo_size);
            }
        }
        clear_macro_statistics(&macro_check_tree);
        clear_macro_statistics(&macinfo_check_tree);
    }
    if (glflags.gf_gdbindex_flag) {
        reset_overall_CU_error_data();
        /*  By definition if gdb_index is present
            then "cu" and "tu" will not be. And vice versa.  */
        print_gdb_index(dbg);
        print_debugfission_index(dbg,"cu");
        print_debugfission_index(dbg,"tu");
    }
    if (glflags.gf_pubnames_flag) {
        reset_overall_CU_error_data();
        print_pubnames(dbg);
    }
    if (glflags.gf_loc_flag) {
        reset_overall_CU_error_data();
        print_locs(dbg);
    }
    if (glflags.gf_abbrev_flag) {
        reset_overall_CU_error_data();
        print_abbrevs(dbg);
    }
    if (glflags.gf_string_flag) {
        reset_overall_CU_error_data();
        print_strings(dbg);
    }
    if (glflags.gf_aranges_flag) {
        reset_overall_CU_error_data();
        print_aranges(dbg);
    }
    if (glflags.gf_ranges_flag) {
        reset_overall_CU_error_data();
        print_ranges(dbg);
    }
    if (glflags.gf_frame_flag || glflags.gf_eh_frame_flag) {
        reset_overall_CU_error_data();
        current_cu_die_for_print_frames = 0;
        print_frames(dbg, glflags.gf_frame_flag,
            glflags.gf_eh_frame_flag, l_config_file_data);
    }
    if (glflags.gf_static_func_flag) {
        reset_overall_CU_error_data();
        print_static_funcs(dbg);
    }
    if (glflags.gf_static_var_flag) {
        reset_overall_CU_error_data();
        print_static_vars(dbg);
    }
    /*  DWARF_PUBTYPES is the standard typenames dwarf section.
        SGI_TYPENAME is the same concept but is SGI specific ( it was
        defined 10 years before dwarf pubtypes). */

    if (glflags.gf_pubtypes_flag) {
        reset_overall_CU_error_data();
        print_types(dbg, DWARF_PUBTYPES);
        reset_overall_CU_error_data();
        print_types(dbg, SGI_TYPENAME);
    }
    if (glflags.gf_weakname_flag) {
        reset_overall_CU_error_data();
        print_weaknames(dbg);
    }
    if (glflags.gf_reloc_flag && elf) {
        reset_overall_CU_error_data();
#ifdef DWARF_WITH_LIBELF
        print_relocinfo(dbg);
#endif /* DWARF_WITH_LIBELF */
    }
    if (glflags.gf_debug_names_flag) {
        reset_overall_CU_error_data();
        print_debug_names(dbg);
    }

    /* Print search results */
    if (glflags.gf_search_print_results && glflags.gf_search_is_on) {
        print_search_results();
    }

    /* The right time to do this is unclear. But we need to do it. */
    if (glflags.gf_check_harmless) {
        print_any_harmless_errors(dbg);
    }

    /* Print error report only if errors have been detected */
    /* Print error report if the -kd option */
    print_checks_results();

    /*  Print the detailed attribute usage space
        and free the attributes_encoding data allocated. */
    if (glflags.gf_check_attr_encoding) {
        print_attributes_encoding(dbg);
    }

    /* Print the tags and attribute usage */
    if (glflags.gf_print_usage_tag_attr) {
        print_tag_attributes_usage(dbg);
    }

    if (glflags.gf_print_str_offsets) {
        /*  print the .debug_str_offsets section, if any. */
        print_str_offsets_section(dbg);
    }

    /*  prints nothing unless section .gnu_debuglink is present.
        Lets print for a few critical sections.  */
    if( glflags.gf_gnu_debuglink_flag) {
        print_gnu_debuglink(dbg);
    }

    /*  Could finish dbg first. Either order ok. */
    if (dbgtied) {
        dres = dwarf_finish(dbgtied,&onef_err);
        if (dres != DW_DLV_OK) {
            print_error(dbg, "dwarf_finish on dbgtied", dres, onef_err);
        }
        dbgtied = 0;
    }
    groups_restore_subsidiary_flags();
    dres = dwarf_finish(dbg, &onef_err);
    if (dres != DW_DLV_OK) {
        print_error(dbg, "dwarf_finish", dres, onef_err);
        dbg = 0;
    }
    printf("\n");
#ifdef DWARF_WITH_LIBELF
    clean_up_syms_malloc_data();
#endif /* DWARF_WITH_LIBELF */
    destruct_abbrev_array();
    esb_close_null_device();
    helpertree_clear_statistics(&helpertree_offsets_base_info);
    helpertree_clear_statistics(&helpertree_offsets_base_types);
    return 0;
}

/* Generic constants for debugging */
#define DUMP_RANGES_INFO            1   /* Dump RangesInfo Table. */
#define DUMP_LOCATION_SECTION_INFO  2   /* Dump Location (.debug_loc) Info. */
#define DUMP_RANGES_SECTION_INFO    3   /* Dump Ranges (.debug_ranges) Info. */
#define DUMP_LINKONCE_INFO          4   /* Dump Linkonce Table. */
#define DUMP_VISITED_INFO           5   /* Dump Visited Info. */

/* ARGSUSED */
static void
print_error_maybe_continue(UNUSEDARG Dwarf_Debug dbg,
    const char * msg,
    int dwarf_code,
    Dwarf_Error lerr,
    Dwarf_Bool do_continue)
{
    unsigned long realmajorerr = glflags.gf_count_major_errors;
    printf("\n");
    if (dwarf_code == DW_DLV_ERROR) {
        char * errmsg = dwarf_errmsg(lerr);

        /*  We now (April 2016) guarantee the
            error number is in
            the error string so we do not need to print
            the dwarf_errno() value to show the number. */
        if (do_continue) {
            printf( "%s ERROR:  %s:  %s. Attempting to continue.\n",
                glflags.program_name, msg, errmsg);
        } else {
            printf( "%s ERROR:  %s:  %s\n",
                glflags.program_name, msg, errmsg);
        }
    } else if (dwarf_code == DW_DLV_NO_ENTRY) {
        printf("%s NO ENTRY:  %s: \n", glflags.program_name, msg);
    } else if (dwarf_code == DW_DLV_OK) {
        printf("%s:  %s \n", glflags.program_name, msg);
    } else {
        printf("%s InternalError:  %s:  code %d\n",
            glflags.program_name, msg, dwarf_code);
    }

    /* Display compile unit name */
    PRINT_CU_INFO();
    glflags.gf_count_major_errors = realmajorerr;
}

void
print_error(Dwarf_Debug dbg,
    const char * msg,
    int dwarf_code,
    Dwarf_Error lerr)
{
    print_error_maybe_continue(dbg,msg,dwarf_code,lerr,FALSE);
    if (dbg) {
        Dwarf_Error ignored_err = 0;
        /*  If dbg was never initialized dwarf_finish
            can do nothing useful. There is no
            global-state for libdwarf to clean up. */
        if (dwarf_code == DW_DLV_ERROR) {
            dwarf_dealloc(dbg,lerr,DW_DLA_ERROR);
        }
        dwarf_finish(dbg, &ignored_err);
        check_for_major_errors();
    }
    exit(FAILED);
}
/* ARGSUSED */
void
print_error_and_continue(Dwarf_Debug dbg,
    const char * msg,
    int dwarf_code,
    Dwarf_Error lerr)
{
    glflags.gf_count_major_errors++;
    print_error_maybe_continue(dbg,
        msg,dwarf_code,lerr,TRUE);
}

/*  Predicate function. Returns 'true' if the CU should
    be skipped as the DW_AT_name of the CU
    does not match the command-line-supplied
    cu name.  Else returns false.*/
boolean
should_skip_this_cu(Dwarf_Debug dbg, Dwarf_Die cu_die)
{
    Dwarf_Half tag = 0;
    Dwarf_Attribute attrib = 0;
    Dwarf_Half theform = 0;
    int dares = 0;
    int tres = 0;
    int fres = 0;
    Dwarf_Error lerr = 0;

    tres = dwarf_tag(cu_die, &tag, &lerr);
    if (tres != DW_DLV_OK) {
        print_error(dbg, "dwarf_tag in aranges",
            tres, lerr);
    }
    dares = dwarf_attr(cu_die, DW_AT_name, &attrib,
        &lerr);
    if (dares != DW_DLV_OK) {
        print_error(dbg, "dwarf_attr arange"
            " derived die has no name",
            dares, lerr);
        }
    fres = dwarf_whatform(attrib, &theform, &lerr);
    if (fres == DW_DLV_OK) {
        if (theform == DW_FORM_string
            || theform == DW_FORM_strp) {
            char * temps = 0;
            int sres = dwarf_formstring(attrib, &temps,
                &lerr);
            if (sres == DW_DLV_OK) {
                char *lcun = esb_get_string(glflags.cu_name);
                char *p = temps;
                if (lcun[0] != '/') {
                    p = strrchr(temps, '/');
                    if (p == NULL) {
                        p = temps;
                    } else {
                        p++;
                    }
                }
                /* Ignore case if Windows */
#if _WIN32
                if (stricmp(lcun, p)) {
                    /* skip this cu. */
                    return TRUE;
                }
#else
                if (strcmp(lcun, p)) {
                    /* skip this cu. */
                    return TRUE;
                }
#endif /* _WIN32 */

            } else {
                print_error(dbg,
                    "arange: string missing",
                    sres, lerr);
            }
        }
    } else {
        print_error(dbg,
            "dwarf_whatform unexpected value.",
            fres, lerr);
    }
    dwarf_dealloc(dbg, attrib, DW_DLA_ATTR);
    return FALSE;
}

/* Returns the cu of the CU. In case of error, give up, do not return. */
int
get_cu_name(Dwarf_Debug dbg, Dwarf_Die cu_die,
    Dwarf_Off dieprint_cu_offset,
    char * *short_name, char * *long_name)
{
    Dwarf_Attribute name_attr = 0;
    Dwarf_Error lerr = 0;
    int ares;

    ares = dwarf_attr(cu_die, DW_AT_name, &name_attr, &lerr);
    if (ares == DW_DLV_ERROR) {
        print_error(dbg, "hassattr on DW_AT_name", ares, lerr);
    } else {
        if (ares == DW_DLV_NO_ENTRY) {
            *short_name = "<unknown name>";
            *long_name = "<unknown name>";
        } else {
            /* DW_DLV_OK */
            /*  The string return is valid until the next call to this
                function; so if the caller needs to keep the returned
                string, the string must be copied (makename()). */
            char *filename = 0;

            esb_empty_string(&esb_long_cu_name);
            get_attr_value(dbg, DW_TAG_compile_unit,
                cu_die, dieprint_cu_offset,
                name_attr, NULL, 0, &esb_long_cu_name,
                0 /*show_form_used*/,0 /* verbose */);
            *long_name = esb_get_string(&esb_long_cu_name);
            /* Generate the short name (filename) */
            filename = strrchr(*long_name,'/');
            if (!filename) {
                filename = strrchr(*long_name,'\\');
            }
            if (filename) {
                ++filename;
            } else {
                filename = *long_name;
            }
            esb_empty_string(&esb_short_cu_name);
            esb_append(&esb_short_cu_name,filename);
            *short_name = esb_get_string(&esb_short_cu_name);
        }
    }
    dwarf_dealloc(dbg, name_attr, DW_DLA_ATTR);
    return ares;
}

/*  Returns the producer of the CU
    Caller must ensure producernameout is
    a valid, constructed, empty esb_s instance before calling.
    Never returns DW_DLV_ERROR.  */
int
get_producer_name(Dwarf_Debug dbg, Dwarf_Die cu_die,
    Dwarf_Off dieprint_cu_offset,
    struct esb_s *producernameout)
{
    Dwarf_Attribute producer_attr = 0;
    Dwarf_Error pnerr = 0;

    int ares = dwarf_attr(cu_die, DW_AT_producer,
        &producer_attr, &pnerr);
    if (ares == DW_DLV_ERROR) {
        print_error(dbg, "hassattr on DW_AT_producer", ares, pnerr);
    }
    if (ares == DW_DLV_NO_ENTRY) {
        /*  We add extra quotes so it looks more like
            the names for real producers that get_attr_value
            produces. */
        esb_append(producernameout,"\"<CU-missing-DW_AT_producer>\"");
    } else {
        /*  DW_DLV_OK */
        /*  The string return is valid until the next call to this
            function; so if the caller needs to keep the returned
            string, the string must be copied (makename()). */
        get_attr_value(dbg, DW_TAG_compile_unit,
            cu_die, dieprint_cu_offset,
            producer_attr, NULL, 0, producernameout,
            0 /*show_form_used*/,0 /* verbose */);
    }
    /*  If ares is error or missing case,
        producer_attr will be left
        NULL by the call,
        which is safe when calling dealloc(). */
    dwarf_dealloc(dbg, producer_attr, DW_DLA_ATTR);
    return ares;
}

void
print_secname(Dwarf_Debug dbg,const char *secname)
{
    if (glflags.gf_do_print_dwarf) {
        struct esb_s truename;
        char buf[DWARF_SECNAME_BUFFER_SIZE];

        esb_constructor_fixed(&truename,buf,sizeof(buf));
        get_true_section_name(dbg,secname,
            &truename,TRUE);
        printf("\n%s\n",sanitized(esb_get_string(&truename)));
        esb_destructor(&truename);
    }
}

/*  We'll check for errors when checking.
    print only if printing (as opposed to checking). */
static void
print_gnu_debuglink(Dwarf_Debug dbg)
{
    int         res = 0;
    char *      name = 0;
    unsigned char *crcbytes = 0;
    char *      link_path = 0;
    unsigned    link_path_len = 0;
    unsigned    buildidtype = 0;
    char       *buildidowner = 0;
    unsigned char *buildidbyteptr = 0;
    unsigned    buildidlength = 0;
    char      **paths_array = 0;
    unsigned    paths_array_length = 0;
    Dwarf_Error linkerror = 0;

    res = dwarf_gnu_debuglink(dbg,
        &name,
        &crcbytes,
        &link_path,     /* free this */
        &link_path_len,
        &buildidtype,
        &buildidowner,
        &buildidbyteptr, &buildidlength,
        &paths_array,  /* free this */
        &paths_array_length,
        &linkerror);
    if (res == DW_DLV_NO_ENTRY) {
        return;
    } else if (res == DW_DLV_ERROR) {
        print_error_and_continue(dbg,
            "Error accessing debuglink or note section",
            res,linkerror);
    }
    if (crcbytes) {
        print_secname(dbg,".gnu_debuglink");
        /* Done with error checking, so print if we are printing. */
        if (glflags.gf_do_print_dwarf)  {
            printf(" Debuglink name  : %s",sanitized(name));
            {
                unsigned char *crc = 0;
                unsigned char *end = 0;

                crc = crcbytes;
                end = crcbytes +4;
                printf("   crc 0X: ");
                for (; crc < end; crc++) {
                    printf("%02x ", *crc);
                }
            }
            printf("\n");
            if (link_path_len) {
                printf(" Debuglink target: %s\n",sanitized(link_path));
            }
        }
    }
    if (buildidlength) {
        print_secname(dbg,".note.gnu.build-id");
        if (glflags.gf_do_print_dwarf)  {
            printf(" Build-id  type     : %u\n", buildidtype);
            printf(" Build-id  ownername: %s\n",
                sanitized(buildidowner));
            printf(" Build-id  length   : %u\n",buildidlength);
            printf(" Build-id           : ");
            {
                const unsigned char *cur = 0;
                const unsigned char *end = 0;

                cur = buildidbyteptr;
                end = cur + buildidlength;
                for (; cur < end; cur++) {
                    printf("%02x", (unsigned char)*cur);
                }
            }
            printf("\n");
        }
    }
    if (paths_array_length) {
        unsigned i = 0;

        printf(" Possible "
            ".gnu_debuglink/.note.gnu.build-id pathnames for\n");
        printf(" an alternate object file with more detailed DWARF\n");
        for( ; i < paths_array_length; ++i) {
            char *path = paths_array[i];
            printf("  [%u] %s\n",i,sanitized(path));
        }
        printf("\n");
    }
    free(link_path);
    free(paths_array);
}

/* GCC linkonce names */
char *lo_text           = ".text.";               /*".gnu.linkonce.t.";*/
char *lo_debug_abbr     = ".gnu.linkonce.wa.";
char *lo_debug_aranges  = ".gnu.linkonce.wr.";
char *lo_debug_frame_1  = ".gnu.linkonce.wf.";
char *lo_debug_frame_2  = ".gnu.linkonce.wF.";
char *lo_debug_info     = ".gnu.linkonce.wi.";
char *lo_debug_line     = ".gnu.linkonce.wl.";
char *lo_debug_macinfo  = ".gnu.linkonce.wm.";
char *lo_debug_loc      = ".gnu.linkonce.wo.";
char *lo_debug_pubnames = ".gnu.linkonce.wp.";
char *lo_debug_ranges   = ".gnu.linkonce.wR.";
char *lo_debug_str      = ".gnu.linkonce.ws.";

/* SNC compiler/linker linkonce names */
char *nlo_text           = ".text.";
char *nlo_debug_abbr     = ".debug.wa.";
char *nlo_debug_aranges  = ".debug.wr.";
char *nlo_debug_frame_1  = ".debug.wf.";
char *nlo_debug_frame_2  = ".debug.wF.";
char *nlo_debug_info     = ".debug.wi.";
char *nlo_debug_line     = ".debug.wl.";
char *nlo_debug_macinfo  = ".debug.wm.";
char *nlo_debug_loc      = ".debug.wo.";
char *nlo_debug_pubnames = ".debug.wp.";
char *nlo_debug_ranges   = ".debug.wR.";
char *nlo_debug_str      = ".debug.ws.";

/* Build linkonce section information */
void
build_linkonce_info(Dwarf_Debug dbg)
{
    int nCount = 0;
    int section_index = 0;
    int res = 0;

    static char **linkonce_names[] = {
        &lo_text,            /* .text */
        &nlo_text,           /* .text */
        &lo_debug_abbr,      /* .debug_abbr */
        &nlo_debug_abbr,     /* .debug_abbr */
        &lo_debug_aranges,   /* .debug_aranges */
        &nlo_debug_aranges,  /* .debug_aranges */
        &lo_debug_frame_1,   /* .debug_frame */
        &nlo_debug_frame_1,  /* .debug_frame */
        &lo_debug_frame_2,   /* .debug_frame */
        &nlo_debug_frame_2,  /* .debug_frame */
        &lo_debug_info,      /* .debug_info */
        &nlo_debug_info,     /* .debug_info */
        &lo_debug_line,      /* .debug_line */
        &nlo_debug_line,     /* .debug_line */
        &lo_debug_macinfo,   /* .debug_macinfo */
        &nlo_debug_macinfo,  /* .debug_macinfo */
        &lo_debug_loc,       /* .debug_loc */
        &nlo_debug_loc,      /* .debug_loc */
        &lo_debug_pubnames,  /* .debug_pubnames */
        &nlo_debug_pubnames, /* .debug_pubnames */
        &lo_debug_ranges,    /* .debug_ranges */
        &nlo_debug_ranges,   /* .debug_ranges */
        &lo_debug_str,       /* .debug_str */
        &nlo_debug_str,      /* .debug_str */
        NULL
    };

    const char *section_name = NULL;
    Dwarf_Addr section_addr = 0;
    Dwarf_Unsigned section_size = 0;
    Dwarf_Error error = 0;
    int nIndex = 0;

    nCount = dwarf_get_section_count(dbg);

    /* Ignore section with index=0 */
    for (section_index = 1; section_index < nCount; ++section_index) {
        res = dwarf_get_section_info_by_index(dbg,section_index,
            &section_name,
            &section_addr,
            &section_size,
            &error);

        if (res == DW_DLV_OK) {
            for (nIndex = 0; linkonce_names[nIndex]; ++nIndex) {
                if (section_name == strstr(section_name,
                    *linkonce_names[nIndex])) {

                    /* Insert only linkonce sections */
                    AddEntryIntoBucketGroup(glflags.pLinkonceInfo,
                        section_index,
                        section_addr,section_addr,
                        section_addr + section_size,
                        section_name,
                        TRUE);
                    break;
                }
            }
        }
    }

    if (dump_linkonce_info) {
        PrintBucketGroup(glflags.pLinkonceInfo,TRUE);
    }
}

/* Check for specific TAGs and initialize some
    information used by '-k' options */
void
tag_specific_checks_setup(Dwarf_Half val,int die_indent_level)
{
    switch (val) {
    case DW_TAG_compile_unit:
        /* To help getting the compile unit name */
        glflags.seen_CU = TRUE;
        /*  If we are checking line information, build
            the table containing the pairs LowPC and HighPC */
        if (glflags.gf_check_decl_file ||
            glflags.gf_check_ranges ||
            glflags.gf_check_locations) {
            ResetBucketGroup(glflags.pRangesInfo);
        }
        /*  The following flag indicate that only low_pc and high_pc
            values found in DW_TAG_subprograms are going to be considered when
            building the address table used to check ranges, lines, etc */
        glflags.need_PU_valid_code = TRUE;
        break;

    case DW_TAG_subprogram:
        /* Keep track of a PU */
        if (die_indent_level == 1) {
            /*  A DW_TAG_subprogram can be nested, when is used to
                declare a member function for a local class; process the DIE
                only if we are at level zero in the DIEs tree */
            glflags.seen_PU = TRUE;
            glflags.seen_PU_base_address = FALSE;
            glflags.seen_PU_high_address = FALSE;
            glflags.PU_name[0] = 0;
            glflags.need_PU_valid_code = TRUE;
        }
        break;
    }
}

/*  Print CU basic information but
    use the local DIE for the offsets. */
void PRINT_CU_INFO(void)
{
    Dwarf_Unsigned loff = glflags.DIE_offset;
    Dwarf_Unsigned goff = glflags.DIE_overall_offset;
    char lbuf[50];
    char hbuf[50];

    if (glflags.current_section_id == DEBUG_LINE ||
        glflags.current_section_id == DEBUG_FRAME ||
        glflags.current_section_id == DEBUG_FRAME_EH_GNU ||
        glflags.current_section_id == DEBUG_ARANGES ||
        glflags.current_section_id == DEBUG_MACRO ||
        glflags.current_section_id == DEBUG_MACINFO ) {
        /*  These sections involve the CU die, so
            use the CU offsets.
            The DEBUG_MAC* cases are logical but
            not yet useful (Dec 2015).
            In other cases the local DIE offset makes
            more sense. */
        loff = glflags.DIE_CU_offset;
        goff = glflags.DIE_CU_overall_offset;
    }
    if (!cu_data_is_set()) {
        return;
    }
    printf("\n");
    printf("CU Name = %s\n",sanitized(glflags.CU_name));
    printf("CU Producer = %s\n",sanitized(glflags.CU_producer));
    printf("DIE OFF = 0x%" DW_PR_XZEROS DW_PR_DUx
        " GOFF = 0x%" DW_PR_XZEROS DW_PR_DUx,
        loff,goff);
    /* We used to print leftover and incorrect values at times. */
    if (glflags.need_CU_high_address) {
        safe_strcpy(hbuf,sizeof(hbuf),"unknown   ",10);
    } else {
        /* safe, hbuf is large enough. */
        sprintf(hbuf,
            "0x%"  DW_PR_XZEROS DW_PR_DUx,glflags.CU_high_address);
    }
    if (glflags.need_CU_base_address) {
        safe_strcpy(lbuf,sizeof(lbuf),"unknown   ",10);
    } else {
        /* safe, lbuf is large enough. */
        sprintf(lbuf,
            "0x%"  DW_PR_XZEROS DW_PR_DUx,glflags.CU_low_address);
    }
#if 0 /* Old format print */
    printf(", Low PC = 0x%08" DW_PR_DUx ", High PC = 0x%08" DW_PR_DUx ,
        CU_low_address,CU_high_address);
#endif
    printf(", Low PC = %s, High PC = %s", lbuf,hbuf);
    printf("\n");
}

void DWARF_CHECK_ERROR_PRINT_CU()
{
    if (glflags.gf_check_verbose_mode) {
        if (glflags.gf_print_unique_errors) {
            if (!glflags.gf_found_error_message) {
                PRINT_CU_INFO();
            }
        } else {
            PRINT_CU_INFO();
        }
    }
    glflags.check_error++;
    glflags.gf_record_dwarf_error = TRUE;
}

/*  Sometimes is useful, just to know the kind of errors in an object file;
    not much interest in the number of errors; the specific case is just to
    have a general idea about the DWARF quality in the file */
char ** set_unique_errors = NULL;
unsigned int set_unique_errors_entries = 0;
unsigned int set_unique_errors_size = 0;
#define SET_UNIQUE_ERRORS_DELTA 64

/*  Create the space to store the unique error messages */
void allocate_unique_errors_table(void)
{
    if (!set_unique_errors) {
        set_unique_errors = (char **)
            malloc(SET_UNIQUE_ERRORS_DELTA * sizeof(char*));
        set_unique_errors_size = SET_UNIQUE_ERRORS_DELTA;
        set_unique_errors_entries = 0;
    }
}

#ifdef TESTING
/* Just for debugging purposes, dump the unique errors table */
void dump_unique_errors_table(void)
{
    unsigned int index;
    printf("*** Unique Errors Table ***\n");
    printf("Delta  : %d\n",SET_UNIQUE_ERRORS_DELTA);
    printf("Size   : %d\n",set_unique_errors_size);
    printf("Entries: %d\n",set_unique_errors_entries);
    for (index = 0; index < set_unique_errors_entries; ++index) {
        printf("%3d: '%s'\n",index,set_unique_errors[index]);
    }
}
#endif

/*  Release the space used to store the unique error messages */
void release_unique_errors_table(void)
{
    unsigned int index;
    for (index = 0; index < set_unique_errors_entries; ++index) {
        free(set_unique_errors[index]);
    }
    free(set_unique_errors);
    set_unique_errors = 0;
    set_unique_errors_entries = 0;
    set_unique_errors_size = 0;
}

/*  Returns TRUE if the text is already in the set; otherwise FALSE */
boolean add_to_unique_errors_table(char * error_text)
{
    unsigned int index;
    size_t len;
    char * stored_text;
    char * filtered_text;
    char * start = NULL;
    char * end = NULL;
    char * pattern = "0x";
    char * white = " ";
    char * question = "?";

    /* Create a copy of the incoming text */
    filtered_text = makename(error_text);
    len = strlen(filtered_text);

    /*  Remove from the error_text, any hexadecimal numbers (start with 0x),
        because for some errors, an additional information is given in the
        form of addresses; we are interested just in the general error. */
    start = strstr(filtered_text,pattern);
    while (start) {
        /* We have found the start of the pattern; look for a space */
        end = strstr(start,white);
        if (!end) {
            /* Preserve any line terminator */
            end = filtered_text + len -1;
        }
        memset(start,*question,end - start);
        start = strstr(filtered_text,pattern);
    }

    /* Check if the error text is already in the table */
    for (index = 0; index < set_unique_errors_entries; ++index) {
        stored_text = set_unique_errors[index];
        if (!strcmp(stored_text,filtered_text)) {
            return TRUE;
        }
    }

    /* Store the new text; check if we have space to store the error text */
    if (set_unique_errors_entries + 1 == set_unique_errors_size) {
        set_unique_errors_size += SET_UNIQUE_ERRORS_DELTA;
        set_unique_errors = (char **)realloc(set_unique_errors,
            set_unique_errors_size * sizeof(char*));
    }

    set_unique_errors[set_unique_errors_entries] = filtered_text;
    ++set_unique_errors_entries;

    return FALSE;
}

/*  Print a DWARF error message and if in "reduced" output only print one
    error of each kind; this feature is useful, when we are interested only
    in the kind of errors and not on the number of errors.
    PRECONDITION: if s3 non-null so are s1,s2.
        If  s2 is non-null so is s1.
        s1 is always non-null. */
static void
print_dwarf_check_error(const char *s1,const char *s2, const char *s3)
{
    static boolean do_init = TRUE;
    boolean found = FALSE;
    char * error_text = NULL;
    static char *leader =  "\n*** DWARF CHECK: ";
    static char *trailer = " ***\n";

    if (do_init) {
        esb_constructor(&dwarf_error_line);
        do_init = FALSE;
    }
    esb_empty_string(&dwarf_error_line);
    esb_append(&dwarf_error_line,leader);

    /*  print_dwarf_check_error("\n*** DWARF CHECK: %s ***\n", str);
        print_dwarf_check_error("\n*** DWARF CHECK: %s: %s ***\n",
        print_dwarf_check_error("\n*** DWARF CHECK: %s -> %s: %s ***\n", */
    if (s3) {
        esb_append(&dwarf_error_line,s1);
        esb_append(&dwarf_error_line," -> ");
        esb_append(&dwarf_error_line,s2);
        esb_append(&dwarf_error_line,": ");
        esb_append(&dwarf_error_line,s3);
    } else if (s2) {
        esb_append(&dwarf_error_line,s1);
        esb_append(&dwarf_error_line,": ");
        esb_append(&dwarf_error_line,s2);
    } else {
        esb_append(&dwarf_error_line,s1);
    }
    esb_append(&dwarf_error_line,trailer);

    error_text = esb_get_string(&dwarf_error_line);
    if (glflags.gf_print_unique_errors) {
        found = add_to_unique_errors_table(error_text);
        if (!found) {
            printf("%s",error_text);
        }
    } else {
        printf("%s",error_text);
    }

    /* To indicate if the current error message have been found or not */
    glflags.gf_found_error_message = found;
}

void DWARF_CHECK_ERROR3(Dwarf_Check_Categories category,
    const char *str1, const char *str2, const char *strexpl)
{
    if (checking_this_compiler()) {
        DWARF_ERROR_COUNT(category,1);
        if (glflags.gf_check_verbose_mode) {
            print_dwarf_check_error(str1, str2,strexpl);
        }
        DWARF_CHECK_ERROR_PRINT_CU();
    }
}

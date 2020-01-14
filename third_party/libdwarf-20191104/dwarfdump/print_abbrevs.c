   /*
  Copyright (C) 2000-2006 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2007-2010 Sun Microsystems, Inc. All rights reserved.
  Portions Copyright 2009-2011 SN Systems Ltd. All rights reserved.
  Portions Copyright 2008-2019 David Anderson. All rights reserved.

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

/*  The address of the Free Software Foundation is
    Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
    Boston, MA 02110-1301, USA.
    SGI has moved from the Crittenden Lane address.
*/

#include "globals.h"
#include "naming.h"
#include "sanitized.h"
#include "esb.h"
#include "esb_using_functions.h"

#include "print_sections.h"

#define TRUE  1
#define FALSE 0


/* The following relevent for one specific Linker. */
#define SNLINKER_MAX_ATTRIB_COUNT  16
/* a warning limit which is arbitrary but leaves a bit more flexibility. */
#define GENERAL_MAX_ATTRIB_COUNT   32

/* Print data in .debug_abbrev
   This is inherently unsafe as it assumes there
   are no byte sequences in .debug_abbrev other than
   legal abbrev sequences.  But the Dwarf spec
   does not promise that. The spec only promises
   that any bytes at an offset referred to from
   .debug_info are legal sequences.
*/

struct abbrev_entry_s {
    Dwarf_Unsigned ae_number;
    Dwarf_Unsigned ae_offset;
    Dwarf_Unsigned ae_attr;
    Dwarf_Unsigned ae_form;
    Dwarf_Unsigned ae_impl_const;
    unsigned ae_dupcount;
};

static int
ab_compare(const void *lin, const void *rin)
{
    const struct abbrev_entry_s *l =
        (const struct abbrev_entry_s *)lin;
    const struct abbrev_entry_s *r =
        (const struct abbrev_entry_s *)rin;
    if (l->ae_attr < r->ae_attr) {
        return -1;
    }
    if (l->ae_attr > r->ae_attr) {
        return 1;
    }
    if (l->ae_form < r->ae_form) {
        return -1;
    }
    if (l->ae_form > r->ae_form) {
        return 1;
    }
    if (l->ae_number < r->ae_number) {
        return -1;
    }
    if (l->ae_number > r->ae_number) {
        return 1;
    }
    return 0;
}

static int
attr_unknown(Dwarf_Unsigned attr)
{
    const char *n = 0;
    int res = 0;

    if (!attr) {
        return TRUE;
    }
    if (attr <= DW_AT_loclists_base) {
        return FALSE;
    }
    if (attr > DW_AT_hi_user) {
        return TRUE;
    }
    res = dwarf_get_AT_name(attr,&n);
    if (res == DW_DLV_NO_ENTRY) {
        return TRUE;
    }
    return FALSE;
}
static int
is_valid_form_we_know(Dwarf_Unsigned form)
{
    int res = 0;
    const char *n = 0;

    res = dwarf_get_FORM_name(form,&n);
    if (res == DW_DLV_NO_ENTRY) {
        return FALSE;
    }
    return TRUE;
}

static void
printdupab(struct abbrev_entry_s * lastaep)
{
    struct esb_s msg;

    esb_constructor(&msg);
    esb_append_printf_u(&msg,
        "Attribute "
        "0x%"  DW_PR_XZEROS DW_PR_DUx ,
        lastaep->ae_attr);
    esb_append_printf_s(&msg,
        " (%s)", get_AT_name(lastaep->ae_attr,
        dwarf_names_print_on_error));
    esb_append_printf_u(&msg,
        " %u times", lastaep->ae_dupcount);
    esb_append_printf_u(&msg,
        " near offset "
        "0x%"  DW_PR_XZEROS DW_PR_DUx ".",
        lastaep->ae_offset);

    DWARF_CHECK_ERROR2(abbreviations_result,
        esb_get_string(&msg),
        "Duplicated attribute in abbrevs ");
    esb_destructor(&msg);
}

extern void
print_abbrevs(Dwarf_Debug dbg)
{
    Dwarf_Abbrev ab;
    Dwarf_Unsigned offset = 0;
    Dwarf_Off off = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Unsigned abbrev_num = 1;
    Dwarf_Signed child_flag = 0;
    int abres = 0;
    int tres = 0;
    int acres = 0;
    Dwarf_Error paerr = 0;
    unsigned loopct = 0;
    char const_integer_string[60];

    glflags.current_section_id = DEBUG_ABBREV;

    for (loopct = 0; ; ++loopct) {
        const char *tagname = "";
        struct abbrev_entry_s *entryarray =0;
        unsigned               entryarray_size = 0;
        Dwarf_Unsigned abbrev_entry_count = 0;
        Dwarf_Unsigned abbrev_code = 0;
        Dwarf_Half tag = 0;
        Dwarf_Unsigned length = 0;

        abres = dwarf_get_abbrev(dbg, offset, &ab,
            &length, &abbrev_entry_count, &paerr);
        if (!loopct) {
            /*  Do this here (not before the loop)
                so the section is loaded and uncompressed
                if necessary. We get information printed
                about the compression (if any) this way. */
            print_secname(dbg,".debug_abbrev");
        }
        if (abres == DW_DLV_ERROR) {
            print_error_and_continue(dbg,
                "Error reading abbreviations", abres, paerr);
            return;
        }
        if (abres == DW_DLV_NO_ENTRY) {
            return;
        }
        /*  Here offset is the global offset in .debug_abbrev.
            The abbrev_num is a relatively worthless counter
            of all abbreviations.  */
        tres = dwarf_get_abbrev_tag(ab, &tag, &paerr);
        if (tres == DW_DLV_ERROR) {
            dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
            print_error(dbg, "Error reading abbreviation Tag", tres, paerr);
        }
        tres = dwarf_get_abbrev_code(ab, &abbrev_code, &paerr);
        if (tres != DW_DLV_OK) {
            dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
            print_error_and_continue(dbg, "Error reading abbreviation code",
                tres, paerr);
        }
        if (!tag) {
            tagname = "Abbrev 0: null abbrev entry";
        } else {
            tagname = get_TAG_name(tag,dwarf_names_print_on_error);
        }
        if ( glflags.gf_do_print_dwarf) {
            if (glflags.dense) {
                printf("<%" DW_PR_DUu "><0x%" DW_PR_XZEROS  DW_PR_DUx
                    "><code: %" DW_PR_DUu ">",
                    abbrev_num, offset,abbrev_code);
                if (glflags.verbose) {
                    printf("<length: 0x%" DW_PR_XZEROS  DW_PR_DUx ">",
                        length);
                }
                printf(" %s", tagname);
            }
            else {
                printf("<%5" DW_PR_DUu "><0x%" DW_PR_XZEROS DW_PR_DUx
                    "><code: %3" DW_PR_DUu ">",
                    abbrev_num, offset, abbrev_code);
                if (glflags.verbose) {
                    printf("<length: 0x%" DW_PR_XZEROS  DW_PR_DUx ">",
                        length);
                }
                printf(" %-27s", tagname);
            }
        }
        /* Process specific TAGs specially. */
        tag_specific_checks_setup(tag,0);
        ++abbrev_num;
        acres = dwarf_get_abbrev_children_flag(ab, &child_flag, &paerr);
        if (acres == DW_DLV_ERROR) {
            dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
            print_error(dbg, "Error reading abbreviation children flag", acres,
                paerr);
            return;
        }
        if (acres == DW_DLV_NO_ENTRY) {
            child_flag = 0;
        }
        /*  If tag is zero, it is a null byte, not a real abbreviation,
            so there is no 'children' flag to print.  */
        if (tag && glflags.gf_do_print_dwarf) {
            const char * child_name = 0;

            child_name = get_children_name(child_flag,
                dwarf_names_print_on_error);
            printf(" %s", child_name);
        }
        if(!glflags.dense) {
            if ( glflags.gf_do_print_dwarf) {
                printf("\n");
            }
        }
        if (abbrev_entry_count < 1) {
            if(tag) {
                printf("   This abbreviation code has no entries\n");
            }
            if (length == 0 || length == 1 ) {
                if ( glflags.gf_do_print_dwarf && glflags.dense ) {
                    printf("\n");
                }
                if (!length) {
                    dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
                    break;
                }
            }
            /* printed null abrev name above */
            offset += length;
            dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
            continue;
        }
        /*  Abbrev contains the format of a die,
            which debug_info then points to with the
            real data. So here we just print the
            given format. */
        entryarray_size = abbrev_entry_count;
        entryarray = calloc(entryarray_size,
            sizeof(struct abbrev_entry_s));
        if (!entryarray) {
            printf( "%s ERROR:  Malloc of %u abbrev_entry_s"
                " structs failed. Near section global offset 0x%"
                DW_PR_DUx "  .\n",
            glflags.program_name,entryarray_size,offset);
            entryarray_size = 0;
        }
        for (i = 0; i < abbrev_entry_count ; i++) {
            int aeres = 0;
            Dwarf_Bool dofilter = FALSE;
            Dwarf_Unsigned form = 0;
            struct abbrev_entry_s *aep = entryarray+i;
            Dwarf_Unsigned attr = 0;
            Dwarf_Signed impl_const = 0;

            aeres = dwarf_get_abbrev_entry_b(ab, i,
                dofilter,&attr, &form,&impl_const, &off,
                &paerr);
            if (aeres == DW_DLV_ERROR) {
                dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
                free(entryarray);
                print_error(dbg, "Error reading abbreviation entry",
                    aeres, paerr);
            }
            aep->ae_number = i;
            aep->ae_attr   = attr;
            aep->ae_form   = form;
            aep->ae_offset = off;
            aep->ae_impl_const = impl_const;
            if (glflags.gf_do_print_dwarf) {
                const_integer_string[0] = 0;
                if (form == DW_FORM_implicit_const) {
                    sprintf(const_integer_string,
                        " <%" DW_PR_DSd
                        " (0x%" DW_PR_DUx
                        ")>",impl_const,impl_const);
                }
                if (glflags.dense) {
                    printf(" <%ld>%s<%s>%s", (unsigned long) off,
                        get_AT_name(attr,dwarf_names_print_on_error),
                        get_FORM_name((Dwarf_Half) form,
                            dwarf_names_print_on_error),
                        const_integer_string);
                } else if (!const_integer_string[0])  {
                    printf("       <0x%08lx>              %-28s%s\n",
                        (unsigned long) off,
                        get_AT_name(attr,
                            dwarf_names_print_on_error),
                        get_FORM_name((Dwarf_Half) form,
                            dwarf_names_print_on_error));
                } else {
                    printf("       <0x%08lx>              %-28s%-20s%s\n",
                        (unsigned long) off,
                        get_AT_name(attr,
                            dwarf_names_print_on_error),
                        get_FORM_name((Dwarf_Half) form,
                            dwarf_names_print_on_error),
                        const_integer_string);
                }
            }
        }
        if (glflags.gf_check_abbreviations &&
            entryarray_size > 0) {
            unsigned l = 0;
            struct abbrev_entry_s *lastaep = 0;

            DWARF_CHECK_COUNT(abbreviations_result,1);
            qsort((void *)entryarray,entryarray_size,
                sizeof(struct abbrev_entry_s),ab_compare);

            for(l = 0; l < entryarray_size ; ++l) {
                struct abbrev_entry_s *aep = entryarray+l;

                if (attr_unknown(aep->ae_attr) ) {
                    struct esb_s msg;

                    esb_constructor(&msg);
                    esb_append_printf_u(&msg,
                        "Attribute "
                        "0x%"  DW_PR_XZEROS DW_PR_DUx ,
                        aep->ae_attr);
                    esb_append_printf_u(&msg,
                        " near offset "
                        "0x%"  DW_PR_XZEROS DW_PR_DUx "." ,
                        aep->ae_offset);
                    DWARF_CHECK_ERROR2(abbreviations_result,
                        "Attr number unknown",
                        esb_get_string(&msg));
                    esb_destructor(&msg);
                }
                if (!is_valid_form_we_know(aep->ae_form)){
                    struct esb_s msg;

                    esb_constructor(&msg);
                    esb_append_printf_u(&msg,
                        "Form "
                        "0x%"  DW_PR_XZEROS DW_PR_DUx,
                        aep->ae_form);
                    esb_append_printf_u(&msg,
                        " near offset "
                        "0x%"  DW_PR_XZEROS DW_PR_DUx ".",
                        aep->ae_offset);
                    DWARF_CHECK_ERROR2(abbreviations_result,
                        "Form number unknown",
                        esb_get_string(&msg));
                    esb_destructor(&msg);
                }
                if (l == 0) {
                    lastaep = aep;
                } else  if (lastaep->ae_attr == aep->ae_attr) {
                    lastaep->ae_dupcount++;
                } else {
                    if (lastaep->ae_dupcount) {
                        printdupab(lastaep);
                    }
                    lastaep = aep;
                }
            }
            if (lastaep->ae_dupcount) {
                printdupab(lastaep);
            }
        }
        dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
        free(entryarray);
        entryarray = 0;
        entryarray_size = 0;
        offset += length;
        if (glflags.gf_do_print_dwarf && glflags.dense) {
            printf("\n");
        }
    }
}

/*  Abbreviations array info for checking  abbrev tags.
    The [zero] entry is not used.
    We never shrink the array, but it never grows beyond
    the largest abbreviation count of all the CUs.
    It is set up when we start a new CU and
    used to validate abbreviations on each DIE in the CU.
    See print_die.c
*/

static Dwarf_Unsigned *abbrev_array = NULL;
/*  Size of the array, the same as the abbrev tag
    count of the CU with the most of them.
    Be careful as abbrev_array[abbrev_array_size]
    is outside the high bound. */
static Dwarf_Unsigned abbrev_array_size = 0;

#define ABBREV_ARRAY_INITIAL_SIZE 64

void
destruct_abbrev_array(void)
{
    free(abbrev_array);
    abbrev_array = 0;
    abbrev_array_size = 0;
}

/*  Normally abbreviation numbers are allocated in sequence from 1
    and increase by 1
    but in case of a compiler bug or a damaged object file one can
    see strange things. This looks for surprises and reports them.
    Returns the abbrev_code unless the value looks very wrong,
    and then it returns zero as we do not want a gigantic
    abbrev code to cause trouble.
*/
static Dwarf_Unsigned
check_abbrev_num_sequence(Dwarf_Unsigned abbrev_code,
    Dwarf_Unsigned last_abbrev_code,
    UNUSEDARG Dwarf_Unsigned l_abbrev_array_size,
    UNUSEDARG Dwarf_Unsigned abbrev_entry_count,
    UNUSEDARG Dwarf_Unsigned total_abbrevs_counted)
{
    char buf[128];

    DWARF_CHECK_COUNT(abbreviations_result,1);
    if (abbrev_code > last_abbrev_code) {
        if ((abbrev_code-last_abbrev_code) > 100 ) {
#ifdef ORIGINAL_SPRINTF
            snprintf(buf, sizeof(buf),
                "Abbrev code %" DW_PR_DUu
                " skips up by %" DW_PR_DUu
                " from last abbrev code of %" DW_PR_DUu ,
                abbrev_code,
                (abbrev_code-last_abbrev_code),
                last_abbrev_code);
            DWARF_CHECK_ERROR2(abbreviations_result,buf,
                "Questionable abbreviation code! Not checking reuse.");
#else
            struct esb_s ar;
            esb_constructor_fixed(&ar,buf,sizeof(buf));
            esb_append_printf_u(&ar,
                "Abbrev code %" DW_PR_DUu,abbrev_code);
            esb_append_printf_u(&ar,
                " skips up by %" DW_PR_DUu,
                (abbrev_code-last_abbrev_code));
            esb_append_printf_u(&ar,
                " from last abbrev code of %" DW_PR_DUu ,
                last_abbrev_code);
            DWARF_CHECK_ERROR2(abbreviations_result,
                esb_get_string(&ar),
                "Questionable abbreviation code! Not checking reuse.");
            esb_destructor(&ar);
#endif
            return 0;
        } else if ((abbrev_code-last_abbrev_code) > 1 ) {
#ifdef ORIGINAL_SPRINTF
            snprintf(buf, sizeof(buf),
                "Abbrev code %" DW_PR_DUu
                " skips up by %" DW_PR_DUu
                " from last abbrev code of %" DW_PR_DUu ,
                abbrev_code,
                (abbrev_code-last_abbrev_code),
                last_abbrev_code);
            DWARF_CHECK_ERROR2(abbreviations_result,buf,
                "Questionable abbreviation code.");
#else
            struct esb_s ar;
            esb_constructor_fixed(&ar,buf,sizeof(buf));
            esb_append_printf_u(&ar,
                "Abbrev code %" DW_PR_DUu,
                abbrev_code);
            esb_append_printf_u(&ar,
                " skips up by %" DW_PR_DUu,
                (abbrev_code-last_abbrev_code));
            esb_append_printf_u(&ar,
                " from last abbrev code of %" DW_PR_DUu ,
                last_abbrev_code);
            DWARF_CHECK_ERROR2(abbreviations_result,
                esb_get_string(&ar),
                "Questionable abbreviation code.");
            esb_destructor(&ar);
#endif
        }
    } else if (abbrev_code < last_abbrev_code) {
#ifdef ORIGINAL_SPRINT
        snprintf(buf, sizeof(buf),
            "Abbrev code %" DW_PR_DUu
            " skips down by %" DW_PR_DUu
            " from last abbrev code of %" DW_PR_DUu ,
            abbrev_code,
            (last_abbrev_code - abbrev_code),
            last_abbrev_code);
        DWARF_CHECK_ERROR2(abbreviations_result,buf,
            "Questionable abbreviation code.");
#else
        struct esb_s ar;
        esb_constructor_fixed(&ar,buf,sizeof(buf));
        esb_append_printf_u(&ar,
            "Abbrev code %" DW_PR_DUu,abbrev_code);
        esb_append_printf_u(&ar,
            " skips down by %" DW_PR_DUu,
            (last_abbrev_code - abbrev_code));
        esb_append_printf_u(&ar,
            " from last abbrev code of %" DW_PR_DUu ,
            last_abbrev_code);
        DWARF_CHECK_ERROR2(abbreviations_result,
            esb_get_string(&ar),
            "Questionable abbreviation code.");
        esb_destructor(&ar);
#endif
    } else {
#ifdef ORIGINAL_SPRINTF
        snprintf(buf, sizeof(buf),
            "Abbrev code %" DW_PR_DUu
            " unchanged from last abbrev code!.",
            abbrev_code);
        DWARF_CHECK_ERROR2(abbreviations_result,buf,
            "Questionable abbreviation code.");
#else
        struct esb_s ar;

        esb_constructor_fixed(&ar,buf,sizeof(buf));
        esb_append_printf_u(&ar,
            "Abbrev code %" DW_PR_DUu
            " unchanged from last abbrev code!.",
            abbrev_code);
        DWARF_CHECK_ERROR2(abbreviations_result,
            esb_get_string(&ar),
            "Questionable abbreviation code.");
        esb_destructor(&ar);
#endif
    }
    return abbrev_code;
}

static void
check_reused_code(Dwarf_Unsigned abbrev_code,
    Dwarf_Unsigned abbrev_entry_count)
{
    char buf[128];

    if (abbrev_code >= abbrev_array_size) {
#ifdef ORIGINAL_SPRINTF
        snprintf(buf, sizeof(buf),
            "Abbrev code %" DW_PR_DUu
            " entry_count unchecked: %" DW_PR_DUu  " ",
            abbrev_code,abbrev_entry_count);
        DWARF_CHECK_ERROR2(abbreviations_result,buf,
            "Questionable abbreviation code.");
#else
        struct esb_s ar;

        esb_constructor_fixed(&ar,buf,sizeof(buf));
        esb_append_printf_u(&ar,
            "Abbrev code %" DW_PR_DUu,
            abbrev_code);
        esb_append_printf_u(&ar,
            " entry_count unchecked: %" DW_PR_DUu  " ",
            abbrev_entry_count);
        DWARF_CHECK_ERROR2(abbreviations_result,
            esb_get_string(&ar),
            "Questionable abbreviation code.");
        esb_destructor(&ar);
#endif
        return;
    }
    if (abbrev_array[abbrev_code]) {
        DWARF_CHECK_COUNT(abbreviations_result,1);
        /* This abbrev code slot was used before. */
        if (abbrev_array[abbrev_code] == abbrev_entry_count) {
#ifdef ORIGINAL_SPRINTF
            snprintf(buf, sizeof(buf),
                "Abbrev code %" DW_PR_DUu
                " reused for same entry_count: %" DW_PR_DUu  " ",
                abbrev_code,abbrev_entry_count);
            DWARF_CHECK_ERROR2(abbreviations_result,buf,
                "Questionable abbreviation code.");
#else
            struct esb_s ar;

            esb_constructor_fixed(&ar,buf,sizeof(buf));
            esb_append_printf_u(&ar,
                "Abbrev code %" DW_PR_DUu,
                abbrev_code);
            esb_append_printf_u(&ar,
                " reused for same entry_count: %" DW_PR_DUu  " ",
                abbrev_entry_count);
            DWARF_CHECK_ERROR2(abbreviations_result,
                esb_get_string(&ar),
                "Questionable abbreviation code.");
            esb_destructor(&ar);
#endif
        } else {
#ifdef ORIGINAL_SPRINTF
            snprintf(buf, sizeof(buf),
                "Abbrev code %" DW_PR_DUu
                " reused for different entry_count. "
                " %" DW_PR_DUu " now %" DW_PR_DUu
                " ",
                abbrev_code,
                abbrev_array[abbrev_code],
                abbrev_entry_count);
            DWARF_CHECK_ERROR2(abbreviations_result,buf,
                "Invalid abbreviation code.");
#else
            struct esb_s ar;

            esb_constructor_fixed(&ar,buf,sizeof(buf));
            esb_append_printf_u(&ar,
                "Abbrev code %" DW_PR_DUu,
                abbrev_code);
            esb_append_printf_u(&ar,
                " reused for different entry_count. "
                " %" DW_PR_DUu , abbrev_array[abbrev_code]);
            esb_append_printf_u(&ar,
                " now %" DW_PR_DUu
                " ",
                abbrev_entry_count);
            DWARF_CHECK_ERROR2(abbreviations_result,
                esb_get_string(&ar),
                "Invalid abbreviation code.");
            esb_destructor(&ar);
#endif
        }
    }
}


/* Calculate the number of abbreviations for the
   current CU and set up basic abbreviations array info,
   storing the number of attributes per abbreviation
*/
void
get_abbrev_array_info(Dwarf_Debug dbg, Dwarf_Unsigned offset_in)
{
    Dwarf_Unsigned offset = offset_in;
    if (glflags.gf_check_abbreviations) {
        Dwarf_Abbrev ab = 0;
        Dwarf_Unsigned length = 0;
        Dwarf_Unsigned abbrev_entry_count = 0;
        Dwarf_Unsigned abbrev_code;
        int abres = DW_DLV_OK;
        Dwarf_Error aberr = 0;
        Dwarf_Unsigned last_abbrev_code = 0;

        Dwarf_Bool bMore = TRUE;
        Dwarf_Unsigned CU_abbrev_count = 0;

        if (abbrev_array == NULL) {
            /* Allocate initial abbreviation array info */
            abbrev_array_size = ABBREV_ARRAY_INITIAL_SIZE;
            abbrev_array = (Dwarf_Unsigned *)
                calloc(abbrev_array_size,sizeof(Dwarf_Unsigned));
        } else {
            /* Clear out values from previous CU */
            memset((void *)abbrev_array,0,
                (abbrev_array_size) * sizeof(Dwarf_Unsigned));
        }

        while (bMore && (abres = dwarf_get_abbrev(dbg, offset, &ab,
            &length, &abbrev_entry_count,
            &aberr)) == DW_DLV_OK) {
            dwarf_get_abbrev_code(ab,&abbrev_code,&aberr);
            if (abbrev_code == 0) {
                /* End of abbreviation table for this CU */
                ++offset; /* Skip abbreviation code */
                bMore = FALSE;
            } else {
                /* Valid abbreviation code. We hope. */
                if (abbrev_code > 0) {
                    Dwarf_Unsigned abhigh = check_abbrev_num_sequence(
                        abbrev_code,
                        last_abbrev_code,
                        abbrev_array_size,abbrev_entry_count,
                        CU_abbrev_count);
                    if (abhigh >= abbrev_array_size) {
                        /*  It is a new high, but is not outrageous. */
                        while (abbrev_code >= abbrev_array_size) {
                            Dwarf_Unsigned old_size = abbrev_array_size;
                            size_t addl_size_bytes = old_size *
                                sizeof(Dwarf_Unsigned);

                            /*  Resize abbreviation array.
                                Only a bogus abbreviation number
                                will iterate
                                more than once. The abhigh check.
                                prevents a runaway. */
                            abbrev_array_size *= 2;
                            abbrev_array = (Dwarf_Unsigned *)
                                realloc(abbrev_array,
                                abbrev_array_size * sizeof(Dwarf_Unsigned));
                            /* Zero out the new bytes. */
                            memset(abbrev_array + old_size,0,
                                addl_size_bytes);
                        }
                        last_abbrev_code = abbrev_code;
                        check_reused_code(abbrev_code,
                            abbrev_entry_count);
                        abbrev_array[abbrev_code] = abbrev_entry_count;
                    } else {
                        /* Zero is the case of 'too high' abbev_code. */
                        if (abhigh > 0) {
                            /*  More or less normal abbrev_code. */
                            last_abbrev_code = abbrev_code;
                            check_reused_code(abbrev_code,
                                abbrev_entry_count);
                            abbrev_array[abbrev_code] =
                                abbrev_entry_count;
                        }
                    }
                    ++CU_abbrev_count;
                    offset += length;
                } else {
                    /* Invalid abbreviation code */
                    print_error(dbg, "get_abbrev_array_info",
                        abres, aberr);
                    last_abbrev_code = abbrev_code;
                }
            }
            dwarf_dealloc(dbg, ab, DW_DLA_ABBREV);
        }
    }
}

/*  Validate an abbreviation for the current CU.
    In case of bogus abbrev input the CU_abbrev_count
    might not be as large as abbrev_array_size says
    the array is.  This should catch that case. */
void
validate_abbrev_code(UNUSEDARG Dwarf_Debug dbg,
    Dwarf_Unsigned abbrev_code)
{
    char buf[128];

    DWARF_CHECK_COUNT(abbreviations_result,1);
    if (abbrev_code && abbrev_code >= abbrev_array_size) {
#ifdef ORIGINAL_SPRINTF
        snprintf(buf, sizeof(buf),
            "Abbrev code %" DW_PR_DUu
            " outside valid range of [0-%" DW_PR_DUu "]",
            abbrev_code,abbrev_array_size);
        DWARF_CHECK_ERROR2(abbreviations_result,buf,
            "Invalid abbreviation code.");
#else
        struct esb_s ar;
        esb_constructor_fixed(&ar,buf,sizeof(buf));
        esb_append_printf_u(&ar,
            "Abbrev code %" DW_PR_DUu, abbrev_code);
        esb_append_printf_u(&ar,
            " outside valid range of [0-%" DW_PR_DUu "]",
            abbrev_array_size);
        DWARF_CHECK_ERROR2(abbreviations_result,
            esb_get_string(&ar),
            "Invalid abbreviation code.");

        esb_destructor(&ar);
#endif
    } else {
        Dwarf_Unsigned abbrev_entry_count =
            abbrev_array[abbrev_code];
        if (abbrev_entry_count > SNLINKER_MAX_ATTRIB_COUNT) {
            if (abbrev_entry_count > GENERAL_MAX_ATTRIB_COUNT) {
#ifdef ORIGINAL_SPRINTF
                snprintf(buf, sizeof(buf),
                    "Abbrev code %" DW_PR_DUu
                    ", with %" DW_PR_DUu " attributes: "
                    "outside a sanity-check maximum of %d.",
                    abbrev_code,
                    abbrev_entry_count,
                    GENERAL_MAX_ATTRIB_COUNT);
                DWARF_CHECK_ERROR2(abbreviations_result,buf,
                    "Number of attributes exceeds sanity check");
#else
                struct esb_s ar;
                esb_constructor_fixed(&ar,buf,sizeof(buf));
                esb_append_printf_u(&ar,
                    "Abbrev code %" DW_PR_DUu,abbrev_code);
                esb_append_printf_u(&ar,
                    ", with %" DW_PR_DUu " attributes: ",
                    abbrev_entry_count);
                esb_append_printf_i(&ar,
                    "outside a sanity-check maximum of %d.",
                    GENERAL_MAX_ATTRIB_COUNT);

                DWARF_CHECK_ERROR2(abbreviations_result,
                    esb_get_string(&ar),
                    "Number of attributes exceeds sanity check");
                esb_destructor(&ar);
#endif
            } else {
                /*  These apply only to one compiliation environment,
                    and are not generally applicable.  */
#ifdef ORIGINAL_SPRINTF
                snprintf(buf, sizeof(buf),
                    "Abbrev code %" DW_PR_DUu
                    ", with %" DW_PR_DUu " attributes: "
                    "outside an SN-LINKER expected-maximum of %d.",
                    abbrev_code,
                    abbrev_entry_count,
                    SNLINKER_MAX_ATTRIB_COUNT);
                DWARF_CHECK_ERROR2(abbreviations_result,buf,
                    "Number of attributes exceeds "
                    "SN-LINKER-specific sanity check.");
#else
                struct esb_s ar;
                esb_constructor_fixed(&ar,buf,sizeof(buf));
                esb_append_printf_u(&ar,
                    "Abbrev code %" DW_PR_DUu,abbrev_code);
                esb_append_printf_u(&ar,
                    ", with %" DW_PR_DUu " attributes: ",
                    abbrev_entry_count);
                esb_append_printf_i(&ar,
                    "outside an SN-LINKER expected-maximum of %d.",
                    SNLINKER_MAX_ATTRIB_COUNT);
                DWARF_CHECK_ERROR2(abbreviations_result,
                    esb_get_string(&ar),
                    "Number of attributes exceeds "
                    "SN-LINKER-specific sanity check.");
                esb_destructor(&ar);
#endif
            }
        }
    }
}

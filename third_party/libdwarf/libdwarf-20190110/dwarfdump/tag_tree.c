/*
  Copyright (C) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright 2009-2012 SN Systems Ltd. All rights reserved.
  Portions Copyright 2009-2017 David Anderson. All rights reserved.

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
#include <errno.h>              /* For errno declaration. */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "common.h"
#include "tag_common.h"
#include "dwgetopt.h"
#include "libdwarf_version.h" /* for DW_VERSION_DATE_STR */

unsigned int tag_tree_combination_table[TAG_TABLE_ROW_MAXIMUM][TAG_TABLE_COLUMN_MAXIMUM];

#ifdef HAVE_USAGE_TAG_ATTR
/* Working array for a specific tag and its valid tags */
static Dwarf_Half tag_tree_vector[DW_TAG_last] = {0};
static Dwarf_Half tag_parents[DW_TAG_last] = {0};
static Dwarf_Half tag_children[DW_TAG_last] = {0};
static Dwarf_Small tag_tree_legal[DW_TAG_last] = {0};
#endif /* HAVE_USAGE_TAG_ATTR */

const char * program_name;

boolean ellipsis = FALSE; /* So we can use dwarf_names.c */

/* Expected input format

0xffffffff
value of a tag
value of a standard tag that may be a child ofthat tag
...
0xffffffff
value of a tag
value of a standard tag that may be a child ofthat tag
...
0xffffffff
...

No commentary allowed, no symbols, just numbers.
Blank lines are allowed and are dropped.

*/

static const char *usage[] = {
  "Usage: tag_tree_build <options>",
  "options:\t-t\tGenerate Tags table",
  "    -i Input-file-path",
  "    -o Output-table-path",
  "    -e   (Want Extended table (common extensions))",
  "    -s   (Want Standard table)",
  ""
};

static char *input_name = 0;
static char *output_name = 0;
int extended_flag = FALSE;
int standard_flag = FALSE;

static void
process_args(int argc, char *argv[])
{
    int c = 0;
    boolean usage_error = FALSE;

    program_name = argv[0];

    while ((c = dwgetopt(argc, argv, "i:o:es")) != EOF) {
        switch (c) {
        case 'i':
            input_name = (char *)strdup(dwoptarg);
            break;
        case 'o':
            output_name = (char *)strdup(dwoptarg);
            break;
        case 'e':
            extended_flag = TRUE;
            break;
        case 's':
            standard_flag = TRUE;
            break;

        default:
            usage_error = TRUE;
            break;
        }
    }

    if (usage_error || 1 == dwoptind || dwoptind != argc) {
        print_usage_message(argv[0],usage);
        exit(FAILED);
    }
}
/*  New naming routine May 2016.
    Instead of directly calling dwarf_get_*()
    When bad tag/attr numbers are presented we return
    a warning string through the pointer.
    The thought is that eventually someone will notice the error.
    It might, of course, be better to emit an error message
    and stop. */
static void
ta_get_TAG_name(unsigned int tagnum,const char **nameout)
{
    int res = 0;

    res = dwarf_get_TAG_name(tagnum,nameout);
    if (res == DW_DLV_OK) {
        return;
    }
    if (res == DW_DLV_NO_ENTRY) {
        *nameout = "<no name known for the TAG>";
        return;
    }
    *nameout = "<ERROR so no name known for the TAG>";
    return;
}

/*  these are used in assignments. */
static unsigned maxrowused = 0;
static unsigned maxcolused = 0;

/*  The argument sizes are declared in tag_common.h, so
    the max usable is toprow-1 and topcol-1. */
static void
check_unused_combo(unsigned toprow,unsigned topcol)
{
    if ((toprow-1) !=  maxrowused) {
        printf("Providing for %u rows but used 0-%u\n",
            toprow,maxrowused);
        printf("Giving up\n");
        exit(1);
    }
    if ((topcol-1) != maxcolused) {
        printf("Providing for %u cols but used 0-%u\n",
            topcol,maxcolused);
        printf("Giving up\n");
        exit(1);
    }

}

static void
validate_row_col(const char *position,
    unsigned crow,
    unsigned ccol,
    unsigned maxrow,
    unsigned maxcol)
{
    if (crow >= TAG_TABLE_ROW_MAXIMUM) {
        printf("error generating row in tag-attr array, %s "
            "current row: %u  size of static array decl: %u\n",
            position,crow, TAG_TABLE_ROW_MAXIMUM);
        exit(1);
    }

    if (crow >= maxrow) {
        printf("error generating row in tree tag array, %s "
            "current row: %u  max allowed: %u\n",
            position,crow,maxrow-1);
        exit(1);
    }
    if (ccol >= TAG_TABLE_COLUMN_MAXIMUM) {
        printf("error generating column in tag-attr array, %s "
            "current col: %u  size of static array decl: %u\n",
            position,ccol, TAG_TABLE_COLUMN_MAXIMUM);
        exit(1);
    }

    if (ccol >= maxcol) {
        printf("error generating column in tree tag array, %s "
            "current row: %u  max allowed: %u\n",
            position,ccol,maxcol-1);
        exit(1);
    }
    if (crow > maxrowused) {
        maxrowused = crow;
    }
    if (ccol > maxcolused) {
        maxcolused = ccol;
    }
    return;
}


int
main(int argc, char **argv)
{
    unsigned u = 0;
    unsigned int num = 0;
    int input_eof = 0;
    unsigned table_rows = 0;
    unsigned table_columns = 0;
    unsigned current_row = 0;
    FILE *fileInp = 0;
    FILE *fileOut = 0;
    const char *aname = 0;
    unsigned int index = 0;

    print_version_details(argv[0],FALSE);
    print_args(argc,argv);
    process_args(argc,argv);

    if (!input_name ) {
        fprintf(stderr,"Input name required, not supplied.\n");
        print_usage_message(argv[0],usage);
        exit(FAILED);
    }
    fileInp = fopen(input_name,"r");
    if (!fileInp) {
        fprintf(stderr,"Invalid input filename, could not open '%s'\n",
            input_name);
        print_usage_message(argv[0],usage);
        exit(FAILED);
    }

    if (!output_name ) {
        fprintf(stderr,"Output name required, not supplied.\n");
        print_usage_message(argv[0],usage);
        exit(FAILED);
    }
    fileOut = fopen(output_name,"w");
    if (!fileOut) {
        fprintf(stderr,"Invalid output filename, could not open: '%s'\n",
            output_name);
        print_usage_message(argv[0],usage);
        exit(FAILED);
    }
    if ((standard_flag && extended_flag) || (!standard_flag && !extended_flag)) {
        fprintf(stderr,"Invalid table type\n");
        fprintf(stderr,"Choose -e  or -s .\n");
        print_usage_message(argv[0],usage);
        exit(FAILED);
    }
    if (standard_flag) {
        table_rows = STD_TAG_TABLE_ROWS;
        table_columns = STD_TAG_TABLE_COLUMNS;
    } else {
        table_rows = EXT_TAG_TABLE_ROWS;
        table_columns = EXT_TAG_TABLE_COLS;
    }

    input_eof = read_value(&num,fileInp);       /* 0xffffffff */
    if (IS_EOF == input_eof) {
        bad_line_input("Empty input file");
    }
    if (num != MAGIC_TOKEN_VALUE) {
        bad_line_input("Expected 0xffffffff");
    }

    /*  Generate main header, regardless of contents */
    fprintf(fileOut,"/* Generated code, do not edit. */\n");
    fprintf(fileOut,"/* Generated sourcedate %s */\n",DW_VERSION_DATE_STR );
    fprintf(fileOut,"\n/* BEGIN FILE */\n\n");

#ifdef HAVE_USAGE_TAG_ATTR
    /* Generate the data type to record the usage of the pairs tag-tag */
    if (standard_flag) {
        fprintf(fileOut,"#ifndef HAVE_USAGE_TAG_ATTR\n");
        fprintf(fileOut,"#define HAVE_USAGE_TAG_ATTR 1\n");
        fprintf(fileOut,"#endif /* HAVE_USAGE_TAG_ATTR */\n\n");
        fprintf(fileOut,"#ifdef HAVE_USAGE_TAG_ATTR\n");
        fprintf(fileOut,"#include \"dwarf.h\"\n");
        fprintf(fileOut,"#include \"libdwarf.h\"\n\n");
        fprintf(fileOut,"typedef struct {\n");
        fprintf(fileOut,"    unsigned int count; /* Tag count */\n");
        fprintf(fileOut,"    Dwarf_Half tag;     /* Tag value */\n");
        fprintf(fileOut,"} Usage_Tag_Tree;\n\n");
    }
#endif /* HAVE_USAGE_TAG_ATTR */

    while (!feof(stdin)) {
        unsigned int tag = 0;
        unsigned nTagLoc = 0;
        unsigned int cur_tag = 0;
        unsigned int child_tag;

        input_eof = read_value(&tag,fileInp);
        if (IS_EOF == input_eof) {
            /* Reached normal eof */
            break;
        }
        if (standard_flag) {
            if (current_row >= table_rows ) {
                bad_line_input("tag value exceeds standard table size");
            }
        } else {
            if (current_row >= table_rows) {
                bad_line_input("too many extended table rows.");
            }
            validate_row_col("Reading tag",current_row,0,
                table_rows,table_columns);
            tag_tree_combination_table[current_row][0] = tag;
        }
        input_eof = read_value(&num,fileInp);
        if (IS_EOF == input_eof) {
            bad_line_input("Not terminated correctly..");
        }
        nTagLoc = 1;
        cur_tag = 1;

#ifdef HAVE_USAGE_TAG_ATTR
        /* Check if we have duplicated tags */
        if (standard_flag) {
            if (tag_parents[tag]) {
                bad_line_input("tag 0x%02x already defined",tag);
            }
            tag_parents[tag] = tag;

            /* Clear out the working attribute vector */
            memset(tag_tree_vector,0,DW_TAG_last * sizeof(Dwarf_Half));
        }
#endif /* HAVE_USAGE_TAG_ATTR */

        while (num != MAGIC_TOKEN_VALUE) {
            if (standard_flag) {
                unsigned idx = num / BITS_PER_WORD;
                unsigned bit = num % BITS_PER_WORD;

                if (idx >= table_columns) {
                    fprintf(stderr,"Want column %d, have only %d\n",
                        idx,table_columns);
                    bad_line_input("too many TAGs: table incomplete.");
                }
                validate_row_col("Update columns bit",tag,idx,
                    table_rows,table_columns);
                tag_tree_combination_table[tag][idx] |= (((unsigned)1) << bit);
            } else {
                if (nTagLoc >= table_columns) {
                    printf("Attempting to use column %d, max is %d\n",
                        nTagLoc,table_columns);
                    bad_line_input("too many subTAGs, table incomplete.");
                }
                validate_row_col("Update tagloc",current_row,nTagLoc,
                    table_rows,table_columns);
                tag_tree_combination_table[current_row][nTagLoc] = num;
                nTagLoc++;
            }

#ifdef HAVE_USAGE_TAG_ATTR
            /* Record the usage only for standard tables */
            if (standard_flag) {
                /* Add child tag to current tag */
                if (cur_tag >= DW_TAG_last) {
                    bad_line_input("too many TAGs: table incomplete.");
                }
                /* Check for duplicated entries */
                if (tag_tree_vector[cur_tag]) {
                    bad_line_input("duplicated tag: table incomplete.");
                }
                tag_tree_vector[cur_tag] = num;
                cur_tag++;
            }
#endif /* HAVE_USAGE_TAG_ATTR */

            input_eof = read_value(&num,fileInp);
            if (IS_EOF == input_eof) {
                bad_line_input("Not terminated correctly.");
            }
        }

#ifdef HAVE_USAGE_TAG_ATTR
        /* Generate the tag-tree vector for current tag */
        if (standard_flag) {
            if (tag >= DW_TAG_last) {
                bad_line_input("tag value exceeds standard table size");
            }
            if (tag_children[tag]) {
                bad_line_input("subtag 0x%02x already defined",tag);
            }
            tag_children[tag] = tag;
            /* Generate reference vector */
            aname = 0;
            ta_get_TAG_name(tag,&aname);
            fprintf(fileOut,"/* 0x%02x - %s */\n",tag,aname);
            fprintf(fileOut,
                "static Usage_Tag_Tree tag_tree_%02x[] = {\n",tag);
            for (index = 1; index < cur_tag; ++index) {
                child_tag = tag_tree_vector[index];
                ta_get_TAG_name(child_tag,&aname);
                fprintf(fileOut,"    {/* 0x%02x */ 0, %s},\n",child_tag,aname);
            }
            fprintf(fileOut,"    {/* %4s */ 0, 0}\n};\n\n"," ");
            /* Record allowed number of attributes */
            tag_tree_legal[tag] = cur_tag - 1;
        }
#endif /* HAVE_USAGE_TAG_ATTR */

        ++current_row; /* for extended table */
    }

#ifdef HAVE_USAGE_TAG_ATTR
    /* Generate the parent of the individual vectors */
    if (standard_flag) {
        unsigned int tag = 0;
        unsigned int legal = 0;

        fprintf(fileOut,
            "static Usage_Tag_Tree *usage_tag_tree[] = {\n");
        for (index = 0; index < DW_TAG_last; ++index) {
            tag = tag_children[index];
            if (tag) {
                aname = 0;
                ta_get_TAG_name(tag,&aname);
                fprintf(fileOut,
                    "   tag_tree_%02x, /* 0x%02x - %s */\n",tag,tag,aname);
            } else {
                fprintf(fileOut,"    0,\n");
            }
        }
        fprintf(fileOut,"    0\n};\n\n");

        /* Generate table with allowed number of tags */
        fprintf(fileOut,"typedef struct {\n");
        fprintf(fileOut,"    Dwarf_Small legal; /* Legal tags */\n");
        fprintf(fileOut,"    Dwarf_Small found; /* Found tags */\n");
        fprintf(fileOut,"} Rate_Tag_Tree;\n\n");
        fprintf(fileOut,
            "static Rate_Tag_Tree rate_tag_tree[] = {\n");
        for (tag = 0; tag < DW_TAG_last; ++tag) {
            if (tag_children[tag]) {
                legal = tag_tree_legal[tag];
                aname = 0;
                ta_get_TAG_name(tag,&aname);
                fprintf(fileOut,
                    "    {%2d, 0 /* 0x%02x - %s */},\n",legal,tag,aname);
            } else {
                fprintf(fileOut,"    {0, 0},\n");
            }
        }
        fprintf(fileOut,"    {0, 0}\n};\n\n");
        fprintf(fileOut,"#endif /* HAVE_USAGE_TAG_ATTR */\n\n");
    }
#endif /* HAVE_USAGE_TAG_ATTR */

    check_unused_combo(table_rows, table_columns);
    if (standard_flag) {
        fprintf(fileOut,"#define TAG_TREE_COLUMN_COUNT %d\n\n",table_columns);
        fprintf(fileOut,"#define TAG_TREE_ROW_COUNT %d\n\n",table_rows);
        fprintf(fileOut,
            "static unsigned int tag_tree_combination_table"
            "[TAG_TREE_ROW_COUNT][TAG_TREE_COLUMN_COUNT] = {\n");
    } else {
        fprintf(fileOut,"#define TAG_TREE_EXT_COLUMN_COUNT %d\n\n",
            table_columns);
        fprintf(fileOut,"#define TAG_TREE_EXT_ROW_COUNT %d\n\n",table_rows);
        fprintf(fileOut,"/* Common extensions */\n");
        fprintf(fileOut,
            "static unsigned int tag_tree_combination_ext_table"
            "[TAG_TREE_EXT_ROW_COUNT][TAG_TREE_EXT_COLUMN_COUNT] = {\n");
    }

    for (u = 0; u < table_rows; u++) {
        unsigned j = 0;
        const char *name = 0;
        if (standard_flag) {
            ta_get_TAG_name(u,&name);
            fprintf(fileOut,"/* 0x%02x - %-37s*/\n",u, name);
        } else {
            unsigned k = tag_tree_combination_table[u][0];
            ta_get_TAG_name(k,&name);
            fprintf(fileOut,"/* 0x%02x - %-37s*/\n", k, name);
        }
        fprintf(fileOut,"    { ");
        for (j = 0; j < table_columns; ++j ) {
            fprintf(fileOut,"0x%08x,",tag_tree_combination_table[u][j]);
        }
        fprintf(fileOut,"},\n");

    }
    fprintf(fileOut,"};\n");
    fprintf(fileOut,"\n/* END FILE */\n");
    fclose(fileInp);
    fclose(fileOut);
    return (0);
}

/* A fake so we can use dwarf_names.c */
void print_error (UNUSEDARG Dwarf_Debug dbg,
    UNUSEDARG const char *msg,
    UNUSEDARG int res,
    UNUSEDARG Dwarf_Error localerr)
{
}

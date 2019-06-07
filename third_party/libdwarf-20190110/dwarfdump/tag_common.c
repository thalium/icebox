/*
  Copyright (C) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2009-2012 SN Systems Ltd. All Rights Reserved.
  Portions Copyright (C) 2009-2012 David Anderson. All Rights Reserved.

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

#include <dwarf.h>
#include <stdio.h>
#include <stdarg.h>   /* For va_start va_arg va_list */
#include <stdlib.h>/* For exit() declaration etc. */
#include <errno.h>/* For errno declaration. */
#include <ctype.h>    /*  For isspace() declaration */

#include "globals.h"
#include "naming.h"
#include "tag_common.h"

static int linecount = 0;
static char line_in[MAX_LINE_SIZE];

void
bad_line_input(char *format,...)
{
    va_list ap;
    va_start(ap,format);
    fprintf(stderr,
        "tag_(tree,attr) table build failed, line %d: \"%s\". ",
        linecount, line_in);
    vfprintf(stderr,format, ap);
    /*  "The object ap may be passed as an argument to another
        function; if that function invokes the va_arg()
        macro with parameter ap, the value of ap in the calling
        function is unspecified and shall be passed to the va_end()
        macro prior to any further reference to ap."
        Single Unix Specification. */
    va_end(ap);
    fprintf(stderr,"\n");
    exit(FAILED);
}

void
trim_newline(char *line, int max)
{
    char *end = line + max - 1;

    for (; *line && (line < end); ++line) {
        if (*line == '\n') {
            /* Found newline, drop it */
            *line = 0;
            return;
        }
   }
   return;
}

/*  Detect empty lines (and other lines we do not want to read) */
static boolean
is_skippable_line(char *pLine)
{
    boolean empty = TRUE;

    if (pLine[0] == '#') {
        /* Preprocessor lines are of no interest. */
        return TRUE;
    }
    for (; *pLine && empty; ++pLine) {
        empty = isspace(*pLine);
    }
    return empty;
}

/*  Reads a value from the text table.
    Exits  with non-zero status
    if the table is erroneous in some way.
*/
int
read_value(unsigned int *outval, FILE*file)
{
    char *res = 0;
    unsigned long lval;
    char *strout = 0;
    boolean bBlankLine = TRUE;

    ++linecount;
    *outval = 0;

    while (bBlankLine) {
        res = fgets(line_in, sizeof(line_in), file);
        if (res == 0) {
            if (ferror(file)) {
                fprintf(stderr,
                    "tag_attr: Error reading table, %d lines read\n",
                    linecount);
                exit(FAILED);
            }

            if (feof(file)) {
                return IS_EOF;
            }

            /* Impossible */
            fprintf(stderr, "tag_attr: Impossible error reading table, "
                "%d lines read\n", linecount);
            exit(FAILED);
        }

        bBlankLine = is_skippable_line(line_in);
    }

    trim_newline(line_in, sizeof(line_in));
    errno = 0;
    lval = strtoul(line_in, &strout, 0);

    if (strout == line_in) {
        bad_line_input("bad number input!");
    }

    if (errno != 0) {
        int myerr = errno;

        fprintf(stderr, "tag_attr errno %d\n", myerr);
        bad_line_input("invalid number on line");
    }

    *outval = (int) lval;

    return NOT_EOF;
}

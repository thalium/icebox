/*
  Copyright (C) 2000-2010 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2009-2010 SN Systems Ltd. All Rights Reserved.
  Portions Copyright (C) 2009-2016 David Anderson. All Rights Reserved.

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

#ifndef tag_common_INCLUDED
#define tag_common_INCLUDED



/* The following is the magic token used to
   distinguish real tags/attrs from group-delimiters.
   Blank lines have been eliminated by an awk script.
*/
#define MAGIC_TOKEN_VALUE 0xffffffff

/*  These next two should match the last DW_TAG+1 and last DW_AT+1 in
    the standard set from dwarf.h */
#define DW_TAG_last 0x4a
#define DW_AT_last 0x8a

/* TAG_TREE.LIST Expected input format

0xffffffff
value of a tag
value of a standard tag that may be a child of that tag
...
0xffffffff
value of a tag
value of a standard tag that may be a child of that tag
...
0xffffffff
...

No blank lines or commentary allowed, no symbols, just numbers.

*/

/* TAG_ATTR.LIST Expected input format

0xffffffff
value of a tag
value of a standard attribute that follows that tag
...
0xffffffff
value of a tag
value of a standard attribute that follows that tag
...
0xffffffff
...

No blank lines or commentary allowed, no symbols, just numbers.

*/

/* We don't need really long lines: the input file is simple. */
#define MAX_LINE_SIZE 1000

/*  1 more than the highest number in the DW_TAG defines,
    this is for standard TAGs. Number of rows. */
#define STD_TAG_TABLE_ROWS  73
/* Enough entries to have a bit for each standard legal tag. */
#define STD_TAG_TABLE_COLUMNS 3

/* TAG tree common extension maximums. */
#define EXT_TAG_TABLE_ROWS  9
#define EXT_TAG_TABLE_COLS  5

/*  The following 2 used in tag_tree.c only.
    They must be large enough but they are only used
    declaring array during build
    (not compiled into dwarfdump)
    so if a bit too large there is no
    side effect on anything.  */
#define TAG_TABLE_ROW_MAXIMUM     80
#define TAG_TABLE_COLUMN_MAXIMUM  8

/*  Number of attributes columns per tag. The array is bit fields,
    BITS_PER_WORD fields per word. Dense and quick to inspect */
#define COUNT_ATTRIBUTE_STD 7

#define STD_ATTR_TABLE_ROWS 74
#define STD_ATTR_TABLE_COLUMNS  5
/* tag/attr tree common extension maximums. */
#define EXT_ATTR_TABLE_ROWS 14
#define EXT_ATTR_TABLE_COLS 10

/*  The following 2 used in tag_attr.c only.
    They must be large enough but they are only used
    declaring an array during build
    (not compiled into dwarfdump)
    so if a bit too large there is no
    side effect on anything.  */
#define ATTR_TABLE_ROW_MAXIMUM 74
#define ATTR_TABLE_COLUMN_MAXIMUM  10

/* Bits per 'int' to mark legal attrs. */
#define BITS_PER_WORD 32

#define IS_EOF 1
#define NOT_EOF 0

extern void bad_line_input(char *format,...);
extern void trim_newline(char *line, int max);
extern boolean is_blank_line(char *pLine);
extern int read_value(unsigned int *outval,FILE *f);

/* Define to 1 to support the generation of tag-attr usage */
#define HAVE_USAGE_TAG_ATTR 1

#endif /* tag_common_INCLUDED */

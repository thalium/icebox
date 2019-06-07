/*
  Copyright (C) 2000,2004 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright(C) David Anderson 2016. All Rights reserved.

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

   makename.c
   $Revision: 1.4 $
   $Date: 2005/11/08 21:48:42 $

   This used to be elaborate stuff.
   Now it is trivial, as duplicating names is
   unimportant in dwarfdump (in general).

   And in fact, this is only called for attributes and
   tags etc whose true name is unknown. Not for
   any normal case.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dwarf_tsearch.h"
#include "makename.h"
#include "globals.h"

#ifdef _WIN32
#pragma warning(disable:4996)    /* Warning when migrated to VS2010 */
#endif /* _WIN32 */

#define TRUE 1
#define FALSE 0

static void * makename_data;
#define VALTYPE char *
#define DW_TSHASHTYPE char *

static int
value_compare_func(const void *l, const void *r)
{
    VALTYPE ml = (VALTYPE)l;
    VALTYPE mr = (VALTYPE)r;
    return strcmp(ml,mr);
}
/* Nothing to free for the 'value' example. */
static void
value_node_free(void *valp)
{
   VALTYPE v = (VALTYPE)valp;
   free(v);
}

void
makename_destructor(void)
{
    dwarf_tdestroy(makename_data,value_node_free);
    makename_data = 0;
}

/*  WARNING: the tree walk functions will, if presented **tree
    when *tree is wanted, simply find nothing. No error,
    just bad results. So when a walk produces nothing
    suspect a code mistake here.
    The basic problem is void* is a terrible way to
    pass in a pointer. But it's how tsearch was defined
    long ago.
*/

char *
makename(const char *s)
{
    char *newstr = 0;
    VALTYPE re = 0;
    void *retval = 0;

    if (!s) {
        return "";
    }
#ifdef SELFTEST
    /*printf("Selftest with name %s\n",s);*/
#endif

    newstr = (char *)strdup(s);
    retval = dwarf_tfind(newstr,&makename_data, value_compare_func);
    if (retval) {
        /* We found our string, it existed already. */
        re = *(VALTYPE *)retval;
        free(newstr);
        return re;
    }
    retval = dwarf_tsearch(newstr,&makename_data, value_compare_func);
    if (!retval) {
        /*  Out of memory, lets just use the string we dup'd and
            let it leak. Things will surely fail anyway. */
        return newstr;
    }
    re = *(VALTYPE *)retval;
    return re;
}

/*  We will make a search tree using a simple value
    (the pointer from an strdup) */




#ifdef SELFTEST

char *samples[]  = {
"abcd",
"efgh",
"a",
"abcd",
0
};

int main()
{
    char *e1 = 0;
    char *e2= 0;
    char *e3= 0;
    char *e4= 0;
    int j = 0;
    int errct = 0;


    e1 = makename(samples[0]);
    e2 = makename(samples[1]);
    e3 = makename(samples[2]);
    e4 = makename(samples[3]);

    if (e1 != e4) {
        printf(" FAIL. mismatch  pointers\n");
        ++errct;
    }
    if (e1 == e2 ) {
        printf(" FAIL. match  pointers\n");
        ++errct;
    }
    if ( e1 == e3) {
        printf(" FAIL. match  pointers\n");
        ++errct;
    }
    if (errct) {
        exit(1);
    }
    printf("PASS makename test\n");
    return 0;
}
#endif /* SELFTEST */

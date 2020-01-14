/*

  Copyright (C) 2015-2015 David Anderson. All Rights Reserved.

  This program is free software; you can redistribute it
  and/or modify it under the terms of version 2.1 of the
  GNU Lesser General Public License as published by the Free
  Software Foundation.

  This program is distributed in the hope that it would be
  useful, but WITHOUT ANY WARRANTY; without even the implied
  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  Further, this software is distributed without any warranty
  that it is free of the rightful claim of any third person
  regarding infringement or the like.  Any license provided
  herein, whether implied or otherwise, applies only to this
  software file.  Patent licenses, if any, provided herein
  do not apply to combinations of this program with other
  software, or any other product whatsoever.

  You should have received a copy of the GNU Lesser General
  Public License along with this program; if not, write the
  Free Software Foundation, Inc., 51 Franklin Street - Fifth
  Floor, Boston MA 02110-1301, USA.

*/

#include "config.h"
#include "dwarf_incl.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for free(). */
#endif /* HAVE_STDLIB_H */
#include <stdio.h> /* For debugging. */
#ifdef HAVE_STDINT_H
#include <stdint.h> /* For uintptr_t */
#endif /* HAVE_STDINT_H */
#include "dwarf_tsearch.h"
#include "dwarf_tied_decls.h"

#define TRUE  1
#define FALSE 0
#define TRUE  1
#define FALSE 0

struct test_data_s {
   const char action;
   unsigned long val;
}  testdata[] = {
{'a', 0x33c8},
{'a', 0x34d8},
{'a', 0x35c8},
{'a', 0x3640},
{'a', 0x3820},
{'a', 0x38d0},
{'a', 0x3958},
{'a', 0x39e8},
{'a', 0x3a78},
{'a', 0x3b08},
{'a', 0x3b98},
{'a', 0x3c28},
{'a', 0x3cb8},
{'d', 0x3c28},
{'a', 0x3d48},
{'d', 0x3cb8},
{'a', 0x3dd8},
{'d', 0x3d48},
{'a', 0x3e68},
{'d', 0x3dd8},
{'a', 0x3ef8},
{'a', 0x3f88},
{'d', 0x3e68},
{'a', 0x4018},
{'d', 0x3ef8},
{0,0}
};

/* We don't test this here, referenced from dwarf_tied.c. */
int
_dwarf_next_cu_header_internal(
    UNUSEDARG Dwarf_Debug dbg,
    UNUSEDARG Dwarf_Bool is_info,
    UNUSEDARG Dwarf_Unsigned * cu_header_length,
    UNUSEDARG Dwarf_Half * version_stamp,
    UNUSEDARG Dwarf_Unsigned * abbrev_offset,
    UNUSEDARG Dwarf_Half * address_size,
    UNUSEDARG Dwarf_Half * offset_size,
    UNUSEDARG Dwarf_Half * extension_size,
    UNUSEDARG Dwarf_Sig8 * signature,
    UNUSEDARG Dwarf_Bool * has_signature,
    UNUSEDARG Dwarf_Unsigned *typeoffset,
    UNUSEDARG Dwarf_Unsigned * next_cu_offset,
    UNUSEDARG Dwarf_Half     * header_cu_type,
    UNUSEDARG Dwarf_Error * error)
{
    return DW_DLV_NO_ENTRY;
}



static struct Dwarf_Tied_Entry_s *
makeentry(unsigned long instance, unsigned ct)
{
    Dwarf_Sig8 s8;
    Dwarf_CU_Context context = 0;
    struct Dwarf_Tied_Entry_s * entry = 0;

    memset(&s8,0,sizeof(s8));
    /* Silly, but just a test...*/
    memcpy(&s8,&instance,sizeof(instance));
    context = (Dwarf_CU_Context)instance;

    entry = (struct Dwarf_Tied_Entry_s *)
        _dwarf_tied_make_entry(&s8,context);
    if (!entry) {
        printf("Out of memory in test! %u\n",ct);
        exit(1);
    }
    return entry;
}

static int
insone(void**tree,unsigned long instance, unsigned ct)
{
    struct Dwarf_Tied_Entry_s * entry = 0;
    void *retval = 0;

    entry = makeentry(instance, ct);
    retval = dwarf_tsearch(entry,tree, _dwarf_tied_compare_function);

    if(retval == 0) {
        printf("FAIL ENOMEM in search on rec %u adr  0x%lu,"
            " error in insone\n",
            ct,(unsigned long)instance);
        exit(1);
    } else {
        struct Dwarf_Tied_Entry_s *re = 0;
        re = *(struct Dwarf_Tied_Entry_s **)retval;
        if(re != entry) {
            /* Found existing, error. */
            printf("insertone rec %u addr 0x%lu found record"
                " preexisting, error\n",
                ct,(unsigned long)instance);
            _dwarf_tied_destroy_free_node(entry);
            exit(1);
        } else {
            /* inserted new entry, make sure present. */
            struct Dwarf_Tied_Entry_s * entry2 = 0;
            entry2 = makeentry(instance,ct);
            retval = dwarf_tfind(entry2,tree,
                _dwarf_tied_compare_function);
            _dwarf_tied_destroy_free_node(entry2);
            if(!retval) {
                printf("insertonebypointer record %d addr 0x%lu "
                    "failed to add as desired,"
                    " error\n",
                    ct,(unsigned long)instance);
                exit(1);
            }
        }
    }
    return 0;
}

static int
delone(void**tree,unsigned long instance, unsigned ct)
{
    struct Dwarf_Tied_Entry_s * entry = 0;
    void *r = 0;


    entry = makeentry(instance, ct);
    r = dwarf_tfind(entry,(void *const*)tree,
        _dwarf_tied_compare_function);
    if (r) {
        struct Dwarf_Tied_Entry_s *re3 =
            *(struct Dwarf_Tied_Entry_s **)r;
        re3 = *(struct Dwarf_Tied_Entry_s **)r;
        dwarf_tdelete(entry,tree,_dwarf_tied_compare_function);
        _dwarf_tied_destroy_free_node(entry);
        _dwarf_tied_destroy_free_node(re3);
    } else {
        printf("delone could not find rec %u ! error! addr"
            " 0x%lx\n",
            ct,(unsigned long)instance);
        exit(1) ;
    }
    return 0;

}

int main()
{
    void *tied_data = 0;
    unsigned u = 0;

    INITTREE(tied_data,_dwarf_tied_data_hashfunc);
    for ( ; testdata[u].action; ++u) {
        char action = testdata[u].action;
        unsigned long v = testdata[u].val;
        if (action == 'a') {
            insone(&tied_data,v,u);
        } else if (action == 'd') {
            delone(&tied_data,v,u);
        } else  {
            printf("FAIL testtied on action %u, "
                "not a or d\n",action);
            exit(1);
        }
    }
    printf("PASS tsearch works for Dwarf_Tied_Entry_s.\n");
    return 0;
}

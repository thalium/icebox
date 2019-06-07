/*
  Copyright (C) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2018  David Anderson. All Rights Reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2.1 of the GNU Lesser General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write the Free Software
  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston MA 02110-1301,
  USA.

*/

#undef  DEBUG

#include "config.h"
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>

#include "dwarf_incl.h"
#include "dwarf_error.h"
#include "dwarf_alloc.h"
/*  These files are included to get the sizes
    of structs for malloc.
*/
#include "dwarf_util.h"
#include "dwarf_line.h"
#include "dwarf_global.h"
#include "dwarf_arange.h"
#include "dwarf_abbrev.h"
#include "dwarf_die_deliv.h"
#include "dwarf_frame.h"
#include "dwarf_loc.h"
#include "dwarf_funcs.h"
#include "dwarf_types.h"
#include "dwarf_vars.h"
#include "dwarf_weaks.h"
#include "dwarf_harmless.h"
#include "dwarf_tsearch.h"
#include "dwarf_gdbindex.h"
#include "dwarf_xu_index.h"
#include "dwarf_macro5.h"
#include "dwarf_dnames.h"
#include "dwarf_dsc.h"
#include "dwarf_str_offsets.h"

#define TRUE 1
#define FALSE 0
/*  Some allocations are simple some not. These reduce
    the issue of determining which sort of thing to a simple
    test. See ia_multiply_count
    Usually when MULTIPLY_NO is set the count
    is 1, so MULTIPY_CT would work as well.  */
#define MULTIPLY_NO 0
#define MULTIPLY_CT 1
#define MULTIPLY_SP 2
/*  This translates into de_alloc_hdr into a per-instance size
    and allows room for a constructor/destructor pointer.
    Rearranging the DW_DLA values would break binary compatibility
    so that is not an option.
*/
struct ial_s {
    /*  In bytes, one struct instance.  */
    short ia_struct_size;

    /*  Not a count, but a MULTIPLY{_NO,_CT,_SP} value. */
    short ia_multiply_count;

    /*  When we really need a constructor/destructor
        these make applying such quite simple. */
    int (*specialconstructor) (Dwarf_Debug, void *);
    void (*specialdestructor) (void *);
};

/*  Used as a way to return meaningful errors when
    the malloc arena is exhausted (when malloc returns NULL).
    Not normally used.
    New in December 2014.*/
struct Dwarf_Error_s _dwarf_failsafe_error = {
    DW_DLE_FAILSAFE_ERRVAL,
    1
};


/*  To do destructors we need some extra data in every
    _dwarf_get_alloc situation. */
/* Here is the extra we malloc for a prefix. */
struct reserve_size_s {
   void *dummy_rsv1;
   void *dummy_rsv2;
};
/* Here is how we use the extra prefix area. */
struct reserve_data_s {
   void *rd_dbg;
   unsigned short rd_length;
   unsigned short rd_type;
};
#define DW_RESERVE sizeof(struct reserve_size_s)


static const
struct ial_s alloc_instance_basics[ALLOC_AREA_INDEX_TABLE_MAX] = {
    { 1,MULTIPLY_NO, 0, 0},            /* 0  none */
    { 1,MULTIPLY_CT, 0, 0},            /* 1 DW_DLA_STRING */
    { sizeof(Dwarf_Loc),MULTIPLY_NO, 0, 0} ,/* 2 DW_DLA_LOC */
    { sizeof(Dwarf_Locdesc),MULTIPLY_NO, 0, 0} , /* 3 DW_DLA_LOCDESC */
    { 1,MULTIPLY_NO, 0, 0} , /* not used *//* 4 DW_DLA_ELLIST */
    { 1,MULTIPLY_NO, 0, 0} , /* not used *//* 5 DW_DLA_BOUNDS */
    { sizeof(Dwarf_Block),MULTIPLY_NO,  0, 0} , /* 6 DW_DLA_BLOCK */

    /* the actual dwarf_debug structure */ /* 7 DW_DLA_DEBUG */
    { 1,MULTIPLY_NO, 0, 0} ,

    {sizeof(struct Dwarf_Die_s),MULTIPLY_NO, 0, 0},/* 8 DW_DLA_DIE */
    {sizeof(struct Dwarf_Line_s),MULTIPLY_NO, 0, 0},/* 9 DW_DLA_LINE */

    /* 10 DW_DLA_ATTR */
    {sizeof(struct Dwarf_Attribute_s),MULTIPLY_NO,  0, 0},

    {1,MULTIPLY_NO,  0, 0}, /* not used */ /* 11 DW_DLA_TYPE */
    {1,MULTIPLY_NO,  0, 0}, /* not used */ /* 12 DW_DLA_SUBSCR */

    /* 13 DW_DLA_GLOBAL */
    {sizeof(struct Dwarf_Global_s),MULTIPLY_NO,  0, 0},

    /* 14 DW_DLA_ERROR */
    {sizeof(struct Dwarf_Error_s),MULTIPLY_NO,  0, 0},

    {sizeof(Dwarf_Ptr),MULTIPLY_CT, 0, 0},  /* 15 DW_DLA_LIST */
    {1,MULTIPLY_NO, 0, 0},    /* not used *//* 16 DW_DLA_LINEBUF */

    /* 17 DW_DLA_ARANGE */
    {sizeof(struct Dwarf_Arange_s),MULTIPLY_NO,  0, 0},

    /* 18 DW_DLA_ABBREV */
    {sizeof(struct Dwarf_Abbrev_s),MULTIPLY_NO,  0, 0},

    /* 19 DW_DLA_FRAME_OP */
    {sizeof(Dwarf_Frame_Op),MULTIPLY_NO,  0, 0} ,

    /* 20 DW_DLA_CIE */
    {sizeof(struct Dwarf_Cie_s),MULTIPLY_NO,  0, 0},

    {sizeof(struct Dwarf_Fde_s),MULTIPLY_NO,  0,
        _dwarf_fde_destructor},/* 21 DW_DLA_FDE */
    {sizeof(Dwarf_Loc),MULTIPLY_CT, 0, 0},          /* 22 DW_DLA_LOC_BLOCK */
    {sizeof(Dwarf_Frame_Op),MULTIPLY_CT, 0, 0},     /* 23 DW_DLA_FRAME_BLOCK */

    /* 24 DW_DLA_FUNC UNUSED */
    {sizeof(struct Dwarf_Global_s),MULTIPLY_NO,  0, 0},

    /* 25 DW_DLA_TYPENAME UNUSED */
    {sizeof(struct Dwarf_Global_s),MULTIPLY_NO,  0, 0},

    /* 26 DW_DLA_VAR UNUSED */
    {sizeof(struct Dwarf_Global_s),MULTIPLY_NO,  0, 0},

    /* 27 DW_DLA_WEAK UNUSED */
    {sizeof(struct Dwarf_Global_s),MULTIPLY_NO,  0, 0},

    {1,MULTIPLY_SP, 0, 0},                    /* 28 DW_DLA_ADDR */
    {sizeof(Dwarf_Ranges),MULTIPLY_CT, 0,0 }, /* 29 DW_DLA_RANGES */

    /*  The following DW_DLA data types
        are known only inside libdwarf.  */

    /* 30 DW_DLA_ABBREV_LIST */
    { sizeof(struct Dwarf_Abbrev_List_s),MULTIPLY_NO, 0, 0},

    /* 31 DW_DLA_CHAIN */
    {sizeof(struct Dwarf_Chain_s),MULTIPLY_NO, 0, 0},

    /* 32 DW_DLA_CU_CONTEXT */
    {sizeof(struct Dwarf_CU_Context_s),MULTIPLY_NO,  0, 0},

    {sizeof(struct Dwarf_Frame_s),MULTIPLY_NO,
        _dwarf_frame_constructor,
        _dwarf_frame_destructor},  /* 33 DW_DLA_FRAME */

    /* 34 DW_DLA_GLOBAL_CONTEXT */
    {sizeof(struct Dwarf_Global_Context_s),MULTIPLY_NO,  0, 0},

    /* 35 DW_DLA_FILE_ENTRY */
    {sizeof(struct Dwarf_File_Entry_s),MULTIPLY_NO,  0, 0},

    /* 36 DW_DLA_LINE_CONTEXT */
    {sizeof(struct Dwarf_Line_Context_s),MULTIPLY_NO,
        _dwarf_line_context_constructor,
        _dwarf_line_context_destructor},

    /* 37 DW_DLA_LOC_CHAIN */
    {sizeof(struct Dwarf_Loc_Chain_s),MULTIPLY_NO,  0, 0},

    /* 38 0x26 DW_DLA_HASH_TABLE */
    {sizeof(struct Dwarf_Hash_Table_s),MULTIPLY_NO, 0, 0},

    /*  The following really use Global struct: used to be unique struct
    per type, but now merged (11/99).  The opaque types
    are visible in the interface. The types  for
    DW_DLA_FUNC, DW_DLA_TYPENAME, DW_DLA_VAR, DW_DLA_WEAK also use
    the global types.  */

    /* 39 0x27 DW_DLA_FUNC_CONTEXT */
    {sizeof(struct Dwarf_Global_Context_s),MULTIPLY_NO,  0, 0},

    /* 40 0x28 DW_DLA_TYPENAME_CONTEXT */
    {sizeof(struct Dwarf_Global_Context_s),MULTIPLY_NO,  0, 0},

    /* 41 0x29 DW_DLA_VAR_CONTEXT */
    {sizeof(struct Dwarf_Global_Context_s),MULTIPLY_NO,  0, 0},

    /* 42 0x2a DW_DLA_WEAK_CONTEXT */
    {sizeof(struct Dwarf_Global_Context_s),MULTIPLY_NO,  0, 0},

    /* 43 0x2b DW_DLA_PUBTYPES_CONTEXT DWARF3 */
    {sizeof(struct Dwarf_Global_Context_s),MULTIPLY_NO,  0, 0},

    /* 44 0x2c DW_DLA_HASH_TABLE_ENTRY */
    {sizeof(struct Dwarf_Hash_Table_Entry_s),MULTIPLY_CT,0,0 },
    /* 0x2d - 0x2f, reserved for future internal use. */

    /* 45 0x2d  DW_DLA_FISSION_PERCU  */
    {sizeof(struct Dwarf_Fission_Per_CU_s),MULTIPLY_CT,  0, 0},

    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    /* 0x30-0x36 reserved for future internal use. */
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/
    {sizeof(int),MULTIPLY_NO,  0, 0}, /* reserved for future internal  types*/

    /*  now,  we have types that are public. */
    /* 0x37 55.  New in June 2014. Gdb. */
    {sizeof(struct Dwarf_Gdbindex_s),MULTIPLY_NO,  0, 0},

    /* 0x38 56.  New in July 2014. DWARF5 DebugFission dwp file sections
        .debug_cu_index and .debug_tu_index . */
    {sizeof(struct Dwarf_Xu_Index_Header_s),MULTIPLY_NO,  0, 0},

    /*  These required by new features in DWARF5. Also usable
        for DWARF2,3,4. */
    /* 57 DW_DLA_LOC_BLOCK_C */
    {sizeof(struct Dwarf_Loc_c_s),MULTIPLY_CT, 0, 0},
    /* 58 DW_DLA_LOCDESC_C */
    {sizeof(struct Dwarf_Locdesc_c_s),MULTIPLY_CT, 0, 0},
    /* 59 DW_DLA_LOC_HEAD_C */
    {sizeof(struct Dwarf_Loc_Head_c_s),MULTIPLY_NO, 0, 0},
    /* 60 DW_DLA_MACRO_CONTEXT */
    {sizeof(struct Dwarf_Macro_Context_s),MULTIPLY_NO,
        _dwarf_macro_constructor,
        _dwarf_macro_destructor},

    /* 61 DW_DLA_CHAIN_2 */
    {sizeof(struct Dwarf_Chain_o),MULTIPLY_NO, 0, 0},
    /* 62 DW_DLA_DSC_HEAD 0x3e */
    {sizeof(struct Dwarf_Dsc_Head_s),MULTIPLY_NO, 0,
        _dwarf_dsc_destructor},
    /* 63 DW_DLA_DNAMES_HEAD 0x3f */
    {sizeof(struct Dwarf_Dnames_Head_s),MULTIPLY_NO, 0,
        _dwarf_debugnames_destructor},
    /* 64 DW_DLA_STR_OFFSETS 0x40 */
    {sizeof(struct Dwarf_Str_Offsets_Table_s),MULTIPLY_NO, 0,0},
};

/*  We are simply using the incoming pointer as the key-pointer.
*/

static DW_TSHASHTYPE
simple_value_hashfunc(const void *keyp)
{
    DW_TSHASHTYPE up = (DW_TSHASHTYPE)keyp;
    return up;
}
/*  We did alloc something but not a fixed-length thing.
    Instead, it starts with some special data we noted.
    The incoming pointer is to the caller data, we
    destruct based on caller, but find the special
    extra data in a prefix area. */
static void
tdestroy_free_node(void *nodep)
{
    char * m = (char *)nodep;
    char * malloc_addr = m - DW_RESERVE;
    struct reserve_data_s * reserve =(struct reserve_data_s *)malloc_addr;
    unsigned type = reserve->rd_type;
    if (type >= ALLOC_AREA_INDEX_TABLE_MAX) {
        /* Internal error, corrupted data. */
        return;
    }
    if(!reserve->rd_dbg) {
        /*  Unused (corrupted?) node in the tree.
            Should never happen. */
        return;
    }
    if(!reserve->rd_type) {
        /*  Unused (corrupted?) node in the tree.
            Should never happen. */
        return;
    }
    if (alloc_instance_basics[type].specialdestructor) {
        alloc_instance_basics[type].specialdestructor(m);
    }
    free(malloc_addr);
}

/* The sort of hash table entries result in very simple helper functions. */
static int
simple_compare_function(const void *l, const void *r)
{
    DW_TSHASHTYPE lp = (DW_TSHASHTYPE)l;
    DW_TSHASHTYPE rp = (DW_TSHASHTYPE)r;
    if(lp < rp) {
        return -1;
    }
    if(lp > rp) {
        return 1;
    }
    return 0;
}

/*  This function returns a pointer to a region
    of memory.  For alloc_types that are not
    strings or lists of pointers, only 1 struct
    can be requested at a time.  This is indicated
    by an input count of 1.  For strings, count
    equals the length of the string it will
    contain, i.e it the length of the string
    plus 1 for the terminating null.  For lists
    of pointers, count is equal to the number of
    pointers.  For DW_DLA_FRAME_BLOCK, DW_DLA_RANGES, and
    DW_DLA_LOC_BLOCK allocation types also, count
    is the count of the number of structs needed.

    This function cannot be used to allocate a
    Dwarf_Debug_s struct.  */

char *
_dwarf_get_alloc(Dwarf_Debug dbg,
    Dwarf_Small alloc_type, Dwarf_Unsigned count)
{
    char * alloc_mem = 0;
    Dwarf_Signed basesize = 0;
    Dwarf_Signed size = 0;
    unsigned int type = alloc_type;
    short action = 0;

    if (dbg == NULL) {
        return NULL;
    }
    if (type >= ALLOC_AREA_INDEX_TABLE_MAX) {
        /* internal error */
        return NULL;
    }
    basesize = alloc_instance_basics[alloc_type].ia_struct_size;
    action = alloc_instance_basics[alloc_type].ia_multiply_count;
    if(action == MULTIPLY_NO) {
        /* Usually count is 1, but do not assume it. */
        size = basesize;
    } else if (action == MULTIPLY_CT) {
        size = basesize * count;
    }  else {
        /* MULTIPLY_SP */
        /* DW_DLA_ADDR.. count * largest size */
        size = count *
            (sizeof(Dwarf_Addr) > sizeof(Dwarf_Off) ?
            sizeof(Dwarf_Addr) : sizeof(Dwarf_Off));
    }
    size += DW_RESERVE;
    alloc_mem = malloc(size);
    if (!alloc_mem) {
        return NULL;
    }
    {
        char * ret_mem = alloc_mem + DW_RESERVE;
        void *key = ret_mem;
        struct reserve_data_s *r = (struct reserve_data_s*)alloc_mem;
        void *result = 0;

        memset(alloc_mem, 0, size);
        /* We are not actually using rd_dbg, we are using rd_type. */
        r->rd_dbg = dbg;
        r->rd_type = alloc_type;
        r->rd_length = size;
        if (alloc_instance_basics[type].specialconstructor) {
            int res =
                alloc_instance_basics[type].specialconstructor(dbg, ret_mem);
            if (res != DW_DLV_OK) {
                /*  We leak what we allocated in _dwarf_find_memory when
                    constructor fails. */
                return NULL;
            }
        }
        result = dwarf_tsearch((void *)key,
            &dbg->de_alloc_tree,simple_compare_function);
        if(!result) {
            /*  Something badly wrong. Out of memory.
                pretend all is well. */
        }
        return (ret_mem);
    }
}

/*  This was once a long list of tests using dss_data
    and dss_size to see if 'space' was inside a debug section.
    This tfind approach removes that maintenance headache. */
static int
string_is_in_debug_section(Dwarf_Debug dbg,void * space)
{
    /*  See dwarf_line.c dwarf_srcfiles()
        for one way we can wind up with
        a DW_DLA_STRING string that may or may not be malloc-ed
        by _dwarf_get_alloc().

        dwarf_formstring(), for example, returns strings
        which point into .debug_info or .debug_types but
        dwarf_dealloc is never supposed to be applied
        to strings dwarf_formstring() returns!

        Lots of calls returning strings
        have always been documented as requiring
        dwarf_dealloc(...DW_DLA_STRING) when the code
        just returns a pointer to a portion of a loaded section!
        It is too late to change the documentation. */

    void *result = 0;
    result = dwarf_tfind((void *)space,
        &dbg->de_alloc_tree,simple_compare_function);
    if(!result) {
        /*  Not in the tree, so not malloc-ed
            Nothing to delete. */
        return TRUE;
    }
    /*  We found the address in the tree, so it is NOT
        part of .debug_info or any other dwarf section,
        but is space malloc-d in _dwarf_get_alloc(). */
    return FALSE;
}

/*
    This function is used to deallocate a region of memory
    that was obtained by a call to _dwarf_get_alloc.  Note
    that though dwarf_dealloc() is a public function,
    _dwarf_get_alloc() isn't.

    For lists, typically arrays of pointers, it is assumed
    that the space was allocated by a direct call to malloc,
    and so a straight free() is done.  This is also the case
    for variable length blocks such as DW_DLA_FRAME_BLOCK
    and DW_DLA_LOC_BLOCK and DW_DLA_RANGES.

    For strings, the pointer might point to a string in
    .debug_info or .debug_string.  After this is checked,
    and if found not to be the case, a free() is done,
    again on the assumption that a malloc was used to
    obtain the space.

    This function does not return anything.
*/
void
dwarf_dealloc(Dwarf_Debug dbg,
    Dwarf_Ptr space, Dwarf_Unsigned alloc_type)
{
    unsigned int type = 0;
    char * malloc_addr = 0;
    struct reserve_data_s * r = 0;

    if (space == NULL) {
        return;
    }
    if (dbg) {
        /*  If it's a string in debug_info etc doing
            (char *)space - DW_RESERVE is totally bogus. */
        if (alloc_type == DW_DLA_STRING &&
            string_is_in_debug_section(dbg,space)) {
            /*  A string pointer may point into .debug_info or
                .debug_string etc.
                So must not be freed.  And strings have no need of a
                specialdestructor().
                Mostly a historical mistake here. */
            return;
        }
        /*  Otherwise it might be allocated string so it is ok
            do the (char *)space - DW_RESERVE  */
    } else {
        /*  App error, or an app that failed to succeed in a
            dwarf_init() call. */
        return;
    }
    if (alloc_type == DW_DLA_ERROR) {
        Dwarf_Error ep = (Dwarf_Error)space;
        if (ep->er_static_alloc == DE_STATIC) {
            /*  This is special, malloc arena
                was exhausted or a NULL dbg
                was used for the error because the real
                dbg was unavailable. There is nothing to delete, really.
                Set er_errval to signal that the space was dealloc'd. */
            _dwarf_failsafe_error.er_errval = DW_DLE_FAILSAFE_ERRVAL;
            return;
        }
        if (ep->er_static_alloc == DE_MALLOC) {
            /*  This is special, we had no arena
                so just malloc'd a Dwarf_Error_s. */
            free(space);
            return;
        }
        /* Was normal alloc, use normal dealloc. */
    }
    type = alloc_type;
    malloc_addr = (char *)space - DW_RESERVE;
    r =(struct reserve_data_s *)malloc_addr;
    if(dbg != r->rd_dbg) {
        /*  Something is badly wrong. Better to leak than
            to crash. */
        return;
    }
    if (type >= ALLOC_AREA_INDEX_TABLE_MAX) {
        /* internal or user app error */
        return;
    }


    if (alloc_instance_basics[type].specialdestructor) {
        alloc_instance_basics[type].specialdestructor(space);
    }
    {
        /*  The 'space' pointer we get points after the reserve space.
            The key and address to free are just a few bytes before
            'space'. */
        void *key = space;
        dwarf_tdelete(key,&dbg->de_alloc_tree,simple_compare_function);
        /*  If dwarf_tdelete returns NULL it might mean
            a) tree is empty.
            b) If hashsearch, then a single chain might now be empty,
                so we do not know of a 'parent node'.
            c) We did not find that key, we did nothing.

            In any case, we simply don't worry about it.
            Not Supposed To Happen. */

        free(malloc_addr);
        return;
    }
}


/*
    Allocates space for a Dwarf_Debug_s struct,
    since one does not exist.
*/
Dwarf_Debug
_dwarf_get_debug(void)
{
    Dwarf_Debug dbg;

    dbg = (Dwarf_Debug) malloc(sizeof(struct Dwarf_Debug_s));
    if (dbg == NULL) {
        return (NULL);
    }
    memset(dbg, 0, sizeof(struct Dwarf_Debug_s));
    /* Set up for a dwarf_tsearch hash table */

    dwarf_initialize_search_hash(&dbg->de_alloc_tree,simple_value_hashfunc,0);


    return (dbg);
}

/*
    This function prints out the statistics
    collected on allocation of memory chunks.
    No longer used.
*/
void
dwarf_print_memory_stats(UNUSEDARG Dwarf_Debug dbg)
{
}



/* In the 'rela' relocation case we might have malloc'd
   space to ensure it is read-write. In that case, free the space.  */
static void
rela_free(struct Dwarf_Section_s * sec)
{
    if (sec->dss_data_was_malloc) {
        free(sec->dss_data);
    }
    sec->dss_data = 0;
    sec->dss_data_was_malloc = 0;
}

static void
freecontextlist(Dwarf_Debug dbg, Dwarf_Debug_InfoTypes dis)
{
    Dwarf_CU_Context context = 0;
    Dwarf_CU_Context nextcontext = 0;
    for (context = dis->de_cu_context_list;
        context; context = nextcontext) {
        Dwarf_Hash_Table hash_table = context->cc_abbrev_hash_table;
        _dwarf_free_abbrev_hash_table_contents(dbg,hash_table);
        nextcontext = context->cc_next;
        dwarf_dealloc(dbg, hash_table, DW_DLA_HASH_TABLE);
        context->cc_abbrev_hash_table = 0;
        dwarf_dealloc(dbg, context, DW_DLA_CU_CONTEXT);
    }
    dis->de_cu_context_list = 0;
}

/*
    Used to free all space allocated for this Dwarf_Debug.
    The caller should assume that the Dwarf_Debug pointer
    itself is no longer valid upon return from this function.

    In case of difficulty, this function simply returns quietly.
*/
int
_dwarf_free_all_of_one_debug(Dwarf_Debug dbg)
{
    if (dbg == NULL) {
        return (DW_DLV_ERROR);
    }

    /*  To do complete validation that we have no surprising missing or
        erroneous deallocs it is advisable to do the dwarf_deallocs here
        that are not things the user can otherwise request.
        Housecleaning.  */
    if (dbg->de_cu_hashindex_data) {
        dwarf_xu_header_free(dbg->de_cu_hashindex_data);
        dbg->de_cu_hashindex_data = 0;
    }
    if (dbg->de_tu_hashindex_data) {
        dwarf_xu_header_free(dbg->de_tu_hashindex_data);
        dbg->de_tu_hashindex_data = 0;
    }
    if( dbg->de_printf_callback_null_device_handle) {
        fclose(dbg->de_printf_callback_null_device_handle);
        dbg->de_printf_callback_null_device_handle = 0;
    }
    freecontextlist(dbg,&dbg->de_info_reading);
    freecontextlist(dbg,&dbg->de_types_reading);

    /* Housecleaning done. Now really free all the space. */
    rela_free(&dbg->de_debug_info);
    rela_free(&dbg->de_debug_types);
    rela_free(&dbg->de_debug_abbrev);
    rela_free(&dbg->de_debug_line);
    rela_free(&dbg->de_debug_line_str);
    rela_free(&dbg->de_debug_loc);
    rela_free(&dbg->de_debug_aranges);
    rela_free(&dbg->de_debug_macinfo);
    rela_free(&dbg->de_debug_macro);
    rela_free(&dbg->de_debug_names);
    rela_free(&dbg->de_debug_pubnames);
    rela_free(&dbg->de_debug_str);
    rela_free(&dbg->de_debug_sup);
    rela_free(&dbg->de_debug_frame);
    rela_free(&dbg->de_debug_frame_eh_gnu);
    rela_free(&dbg->de_debug_pubtypes);
    rela_free(&dbg->de_debug_funcnames);
    rela_free(&dbg->de_debug_typenames);
    rela_free(&dbg->de_debug_varnames);
    rela_free(&dbg->de_debug_weaknames);
    rela_free(&dbg->de_debug_ranges);
    rela_free(&dbg->de_debug_str_offsets);
    rela_free(&dbg->de_debug_addr);
    rela_free(&dbg->de_debug_gdbindex);
    rela_free(&dbg->de_debug_cu_index);
    rela_free(&dbg->de_debug_tu_index);
    dwarf_harmless_cleanout(&dbg->de_harmless_errors);

    if (dbg->de_printf_callback.dp_buffer &&
        !dbg->de_printf_callback.dp_buffer_user_provided ) {
        free(dbg->de_printf_callback.dp_buffer);
    }

    _dwarf_destroy_group_map(dbg);
    dwarf_tdestroy(dbg->de_alloc_tree,tdestroy_free_node);
    dbg->de_alloc_tree = 0;
    if (dbg->de_tied_data.td_tied_search) {
        dwarf_tdestroy(dbg->de_tied_data.td_tied_search,
            _dwarf_tied_destroy_free_node);
        dbg->de_tied_data.td_tied_search = 0;
    }
    memset(dbg, 0, sizeof(*dbg)); /* Prevent accidental use later. */
    free(dbg);
    return (DW_DLV_OK);
}
/*  A special case: we have no dbg, no alloc header etc.
    So create something out of thin air that we can recognize
    in dwarf_dealloc.
    Something with the prefix (prefix space hidden from caller).

    Only applies to DW_DLA_ERROR, and  making up an error record.
    The allocated space simply leaks.
*/
struct Dwarf_Error_s *
_dwarf_special_no_dbg_error_malloc(void)
{
    Dwarf_Error e = 0;
    /* The union unused things are to guarantee proper alignment */
    char *mem = malloc(sizeof(struct Dwarf_Error_s));
    if (mem == 0) {
        return 0;
    }
    memset(mem, 0, sizeof(struct Dwarf_Error_s));
    e = (Dwarf_Error)mem;
    e->er_static_alloc = DE_MALLOC;
    return e;
}

/*
  Copyright (C) 2000,2004,2005 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2007-2019 David Anderson. All Rights Reserved.
  Portions Copyright (C) 2011-2012 SN Systems Ltd. All Rights Reserved

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
#ifdef DWARF_WITH_LIBELF
#define DWARF_RELOC_MIPS
#define DWARF_RELOC_PPC
#define DWARF_RELOC_PPC64
#define DWARF_RELOC_ARM
#define DWARF_RELOC_X86_64
#define DWARF_RELOC_386

#ifndef SELFTEST
#include "print_reloc.h"
#endif /* SELFTEST */
#include "section_bitmaps.h"
#include "esb.h"
#include "sanitized.h"

#ifndef HAVE_ELF64_GETEHDR
#define Elf64_Addr  long
#define Elf64_Word  unsigned long
#define Elf64_Xword unsigned long
#define Elf64_Sym   long
#endif

struct sect_data_s {
    Dwarf_Small *buf;
    Dwarf_Unsigned size;
    Dwarf_Bool display; /* Display reloc if TRUE */
    const char *name;   /* Section name */
    Elf64_Xword type;   /* To cover 32 and 64 records types */
};
#ifndef SELFTEST
static struct sect_data_s sect_data[DW_SECTION_REL_ARRAY_SIZE];
#endif /* SELFTEST */

typedef size_t indx_type;

typedef struct {
    indx_type indx;
    char *name;
    Elf32_Addr value;
    Elf32_Word size;
    int type;
    int bind;
    unsigned char other;
    Elf32_Half shndx;
} SYM;


typedef struct {
    indx_type indx;
    char *name;
    Elf64_Addr value;
    Elf64_Xword size;
    int type;
    int bind;
    unsigned char other;
    unsigned short shndx;
} SYM64;
#ifndef SELFTEST
static void print_reloc_information_64(int section_no,
    Dwarf_Small * buf,
    Dwarf_Unsigned size,
    Elf64_Xword type,
    char **scn_names,
    int scn_names_count);
static void print_reloc_information_32(int section_no,
    Dwarf_Small * buf,
    Dwarf_Unsigned size,
    Elf64_Xword type,
    char **scn_names,
    int scn_names_count);
static SYM *readsyms(Elf32_Sym * data, size_t num, Elf * elf,
    Elf32_Word link);
static SYM64 *read_64_syms(Elf64_Sym * data, size_t num, Elf * elf,
    Elf64_Word link);
static void *get_scndata(Elf_Scn * fd_scn, size_t * scn_size);
static void print_relocinfo_64(Dwarf_Debug dbg, Elf * elf);
static void print_relocinfo_32(Dwarf_Debug dbg, Elf * elf);

static SYM   *sym_data;
static SYM64 *sym_data_64;
static unsigned long   sym_data_entry_count;
static unsigned long   sym_data_64_entry_count;

/*  Include Section type, to be able to deal with all the
    Elf32_Rel, Elf32_Rela, Elf64_Rel, Elf64_Rela relocation types
    print_error does not return, so the following esb_destructor()
    call is unnecessary but reads well  :-) */
#define SECT_DATA_SET(x,t,n,sout,r2) {                        \
    data = elf_getdata(scn, 0);                               \
    if (!data || !data->d_size) {                             \
        struct esb_s data_disaster;                           \
        esb_constructor(&data_disaster);                      \
        esb_append(&data_disaster,(n));                       \
        esb_append(&data_disaster," null");                   \
        print_error(dbg,esb_get_string(&data_disaster),DW_DLV_OK, err); \
        esb_destructor(&data_disaster);                       \
    }                                                         \
    sout[(r2)]      = sect_data[(x)];                         \
    sout[(r2)].buf  = data->d_buf;                            \
    sout[(r2)].size = data->d_size;                           \
    sout[(r2)].type = (t);                                    \
    sout[(r2)].name = (n);                                    \
    }
/* Record the relocation table name information */
static const char **reloc_type_names = NULL;
static int   number_of_reloc_type_names = 0;
#endif /*  SELFTEST */


typedef struct {
    indx_type index;
    char *name_rel;     /* .rel.debug_* names  */
    char *name_rela;    /* .rela.debug_* names */
} REL_INFO;

/*  If the incoming scn_name is known, record the name
    in our reloc section names table.
    For a given (debug) section there can be a .rel or a .rela,
    not both.
    The name-to-index in this table is fixed, invariant.

*/
static REL_INFO rel_info[DW_SECTION_REL_ARRAY_SIZE] = {
    {0,0,0},
    {/*1*/ DW_SECTION_REL_DEBUG_INFO,
    DW_SECTNAME_REL_DEBUG_INFO,
    DW_SECTNAME_RELA_DEBUG_INFO},

    {/*2*/ DW_SECTION_REL_DEBUG_LINE,
    DW_SECTNAME_REL_DEBUG_LINE,
    DW_SECTNAME_RELA_DEBUG_LINE},

    {/*3*/ DW_SECTION_REL_DEBUG_PUBNAMES,
    DW_SECTNAME_REL_DEBUG_PUBNAMES,
    DW_SECTNAME_RELA_DEBUG_PUBNAMES},

    {/*4*/ DW_SECTION_REL_DEBUG_ABBREV,
    DW_SECTNAME_REL_DEBUG_ABBREV,
    DW_SECTNAME_RELA_DEBUG_ABBREV},

    {/*5*/ DW_SECTION_REL_DEBUG_ARANGES,
    DW_SECTNAME_REL_DEBUG_ARANGES,
    DW_SECTNAME_RELA_DEBUG_ARANGES},

    {/*6*/ DW_SECTION_REL_DEBUG_FRAME,
    DW_SECTNAME_REL_DEBUG_FRAME,
    DW_SECTNAME_RELA_DEBUG_FRAME},

    {/*7*/ DW_SECTION_REL_DEBUG_LOC,
    DW_SECTNAME_REL_DEBUG_LOC,
    DW_SECTNAME_RELA_DEBUG_LOC},

    {/*8*/ DW_SECTION_REL_DEBUG_LOCLISTS,
    DW_SECTNAME_REL_DEBUG_LOCLISTS,
    DW_SECTNAME_RELA_DEBUG_LOCLISTS},

    {/*9*/ DW_SECTION_REL_DEBUG_RANGES,
    DW_SECTNAME_REL_DEBUG_RANGES,
    DW_SECTNAME_RELA_DEBUG_RANGES},

    {/*10*/ DW_SECTION_REL_DEBUG_RNGLISTS,
    DW_SECTNAME_REL_DEBUG_RNGLISTS,
    DW_SECTNAME_RELA_DEBUG_RNGLISTS},

    {/*11*/ DW_SECTION_REL_DEBUG_TYPES,
    DW_SECTNAME_REL_DEBUG_TYPES,
    DW_SECTNAME_RELA_DEBUG_TYPES},

    {/*12*/ DW_SECTION_REL_DEBUG_STR_OFFSETS,
    DW_SECTNAME_REL_DEBUG_STR_OFFSETS,
    DW_SECTNAME_RELA_DEBUG_STR_OFFSETS},

    {/*13*/ DW_SECTION_REL_DEBUG_PUBTYPES,
    DW_SECTNAME_REL_DEBUG_PUBTYPES,
    DW_SECTNAME_RELA_DEBUG_PUBTYPES},

    {/*14*/ DW_SECTION_REL_GDB_INDEX,
    DW_SECTNAME_REL_GDB_INDEX,
    DW_SECTNAME_RELA_GDB_INDEX},

    {/*15*/ DW_SECTION_REL_EH_FRAME,
    DW_SECTNAME_REL_EH_FRAME,
    DW_SECTNAME_RELA_EH_FRAME},

    {/*16*/ DW_SECTION_REL_DEBUG_SUP,
    DW_SECTNAME_REL_DEBUG_SUP,
    DW_SECTNAME_RELA_DEBUG_SUP},

    {/*17*/ DW_SECTION_REL_DEBUG_MACINFO,
    DW_SECTNAME_REL_DEBUG_MACINFO,
    DW_SECTNAME_RELA_DEBUG_MACINFO},

    {/*18*/ DW_SECTION_REL_DEBUG_MACRO,
    DW_SECTNAME_REL_DEBUG_MACRO,
    DW_SECTNAME_RELA_DEBUG_MACRO},

    {/*19*/ DW_SECTION_REL_DEBUG_NAMES,
    DW_SECTNAME_REL_DEBUG_NAMES,
    DW_SECTNAME_RELA_DEBUG_NAMES},
};

#endif /* DWARF_WITH_LIBELF */

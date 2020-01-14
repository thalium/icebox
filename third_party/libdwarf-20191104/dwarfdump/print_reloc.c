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

#include "print_reloc.h"
#include "print_reloc_decls.h"
#include "section_bitmaps.h"
#include "esb.h"
#include "sanitized.h"

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


/* Set the relocation names based on the machine type */
static void
set_relocation_table_names(Dwarf_Small machine_type)
{
    reloc_type_names = 0;
    number_of_reloc_type_names = 0;
    switch (machine_type) {
    case EM_MIPS:
#ifdef DWARF_RELOC_MIPS
        reloc_type_names = reloc_type_names_MIPS;
        number_of_reloc_type_names =
            sizeof(reloc_type_names_MIPS) / sizeof(char *);
#endif /* DWARF_RELOC_MIPS */
        break;
    case EM_PPC:
#ifdef DWARF_RELOC_PPC
        reloc_type_names = reloc_type_names_PPC;
        number_of_reloc_type_names =
            sizeof(reloc_type_names_PPC) / sizeof(char *);
#endif /* DWARF_RELOC_PPC */
        break;
    case EM_PPC64:
#ifdef DWARF_RELOC_PPC64
        reloc_type_names = reloc_type_names_PPC64;
        number_of_reloc_type_names =
            sizeof(reloc_type_names_PPC64) / sizeof(char *);
#endif /* DWARF_RELOC_PPC64 */
        break;
    case EM_ARM:
#ifdef DWARF_RELOC_ARM
        reloc_type_names = reloc_type_names_ARM;
        number_of_reloc_type_names =
            sizeof(reloc_type_names_ARM) / sizeof(char *);
#endif /* DWARF_RELOC_ARM */
        break;
    case EM_386:
#ifdef DWARF_RELOC_386
        reloc_type_names = reloc_type_names_386;
        number_of_reloc_type_names =
            sizeof(reloc_type_names_386) / sizeof(char *);
#endif /* DWARF_RELOC_386 */
        break;
    case EM_X86_64:
#ifdef DWARF_RELOC_X86_64
        reloc_type_names = reloc_type_names_X86_64;
        number_of_reloc_type_names =
            sizeof(reloc_type_names_X86_64) / sizeof(char *);
#endif /* DWARF_RELOC_X86_64 */
        break;
    default:
        /* We don't have others covered. */
        reloc_type_names = 0;
        number_of_reloc_type_names = 0;
        break;
  }
}

static void
get_reloc_type_names(int index,struct esb_s* outbuf)
{
    if (index < 0 || index >= number_of_reloc_type_names) {
        if (number_of_reloc_type_names == 0) {
            /* No table provided. */
            esb_append_printf(outbuf,
                "reloc type %d", (int) index);
        } else {
            /* Table incomplete? */
            esb_append_printf(outbuf,
                "reloc type %d unknown", (int) index);
        }
        return;
    }
    esb_append(outbuf, reloc_type_names[index]);
}


static void
get_reloc_section(Dwarf_Debug dbg,
    Elf_Scn *scn,
    char *scn_name,
    Elf64_Word sh_type,
    struct sect_data_s * printable_sect,
    unsigned sectnum)
{
    Elf_Data *data;
    int index;
    /*  Check for reloc records we are interested in. */
    for (index = 1; index < DW_SECTION_REL_ARRAY_SIZE; ++index) {
        const char *n = rel_info[index].name_rel;
        const char *na = rel_info[index].name_rela;
        Dwarf_Error err = 0;

        if (strcmp(scn_name, n) == 0) {
            SECT_DATA_SET(rel_info[index].index,sh_type,n,
                printable_sect,sectnum)
            return;
        }
        if (strcmp(scn_name, na) == 0) {
            SECT_DATA_SET(rel_info[index].index,sh_type,na,
                printable_sect,sectnum)
            return;
        }
    }
    return;
}

void
print_relocinfo(Dwarf_Debug dbg)
{
    Elf *elf = 0;
    char *endr_ident = 0;
    int is_64bit = 0;
    int res = 0;
    int i = 0;
    Dwarf_Error err = 0;

    for (i = 1; i < DW_SECTION_REL_ARRAY_SIZE; i++) {
        sect_data[i].display = reloc_map_enabled(i);
        sect_data[i].buf = 0;
        sect_data[i].size = 0;
        sect_data[i].type = SHT_NULL;
    }
    res = dwarf_get_elf(dbg, &elf, &err);
    if (res == DW_DLV_NO_ENTRY) {
        printf(" No Elf, so no elf relocations to print.\n");
        return;
    } else if (res == DW_DLV_ERROR) {
        print_error(dbg, "dwarf_get_elf error", res, err);
    }
    endr_ident = elf_getident(elf, NULL);
    if (!endr_ident) {
        print_error(dbg, "DW_ELF_GETIDENT_ERROR", res, err);
    }
    is_64bit = (endr_ident[EI_CLASS] == ELFCLASS64);
    if (is_64bit) {
        print_relocinfo_64(dbg, elf);
    } else {
        print_relocinfo_32(dbg, elf);
    }
}

static void
print_relocinfo_64(Dwarf_Debug dbg, Elf * elf)
{
#ifdef HAVE_ELF64_GETEHDR
    Elf_Scn *scn = NULL;
    unsigned sect_number = 0;
    Elf64_Ehdr *ehdr64 = 0;
    Elf64_Shdr *shdr64 = 0;
    char *scn_name = 0;
    int i = 0;
    Elf64_Sym *sym_64 = 0;
    char **scn_names = 0;
    struct sect_data_s *printable_sects = 0;

    int scn_names_cnt = 0;
    Dwarf_Error err = 0;

    ehdr64 = elf64_getehdr(elf);
    if (ehdr64 == NULL) {
        print_error(dbg, "DW_ELF_GETEHDR_ERROR", DW_DLV_OK, err);
    }

    /*  Make the section name array big enough
        that we don't need to check for overrun in the loop. */
    scn_names_cnt = ehdr64->e_shnum + 1;
    scn_names = (char **)calloc(scn_names_cnt, sizeof(char *));
    if (!scn_names) {
        print_error(dbg, "Out of malloc space in relocation print names",
            DW_DLV_OK, err);
    }
    printable_sects = (struct sect_data_s *)calloc(scn_names_cnt,
        sizeof(struct sect_data_s));
    if (!printable_sects) {
        free(scn_names);
        print_error(dbg, "Out of malloc space in relocation print sects",
            DW_DLV_OK, err);
    }

    /*  First nextscn returns section 1 */
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        ++sect_number;
        shdr64 = elf64_getshdr(scn);
        if (shdr64 == NULL) {
            free(scn_names);
            free(printable_sects);
            print_error(dbg, "DW_ELF_GETSHDR_ERROR", DW_DLV_OK, err);
        }
        scn_name = elf_strptr(elf, ehdr64->e_shstrndx, shdr64->sh_name);
        if (scn_name  == NULL) {
            print_error(dbg, "DW_ELF_STRPTR_ERROR", DW_DLV_OK, err);
        }
            /* elf_nextscn() skips section with index '0' */
        scn_names[sect_number] = scn_name;
        if (shdr64->sh_type == SHT_SYMTAB) {
            size_t sym_size = 0;
            size_t count = 0;

            sym_64 = (Elf64_Sym *) get_scndata(scn, &sym_size);
            if (sym_64 == NULL) {
                free(scn_names);
                free(printable_sects);
                print_error(dbg, "no Elf64 symbol table data", DW_DLV_OK,
                    err);
            }
            count = sym_size / sizeof(Elf64_Sym);
            if(sym_size%sizeof(Elf64_Sym)) {
                print_error(dbg, "Elf64 problem reading .symtab data",
                    DW_DLV_OK, err);
            }
            sym_64++;
            count--;
            free(sym_data_64);
            sym_data_64 = 0;
            sym_data_64 = read_64_syms(sym_64, count, elf, shdr64->sh_link);
            sym_data_64_entry_count = count;
            if (sym_data_64  == NULL) {
                free(scn_names);
                free(printable_sects);
                print_error(dbg, "problem reading Elf64 symbol table data",
                    DW_DLV_OK, err);
            }
        } else  {
            get_reloc_section(dbg,scn,scn_name,shdr64->sh_type,
                printable_sects,sect_number);
        }
    }

    /* Set the relocation names based on the machine type */
    set_relocation_table_names(ehdr64->e_machine);

    for (i = 1; i < ehdr64->e_shnum + 1; i++) {
        if (printable_sects[i].display &&
            printable_sects[i].buf != NULL &&
            printable_sects[i].size > 0) {
            print_reloc_information_64(i,
                printable_sects[i].buf,
                printable_sects[i].size,
                printable_sects[i].type,
                scn_names,scn_names_cnt);
        }
    }
    free(printable_sects);
    free(scn_names);
#endif
}

static void
print_relocinfo_32(Dwarf_Debug dbg, Elf * elf)
{
    Elf_Scn *scn = NULL;
    Elf32_Ehdr *ehdr32 = 0;
    Elf32_Shdr *shdr32 = 0;
    unsigned sect_number = 0;
    char *scn_name = 0;
    int i = 0;
    Elf32_Sym  *sym = 0;
    char **scn_names = 0;
    int scn_names_cnt = 0;
    Dwarf_Error err = 0;
    struct sect_data_s *printable_sects = 0;

    ehdr32 = elf32_getehdr(elf);
    if (ehdr32 == NULL) {
        print_error(dbg, "DW_ELF_GETEHDR_ERROR", DW_DLV_OK, err);
    }

    /*  Make the section name array big enough
        that we don't need to check for overrun in the loop. */
    scn_names_cnt = ehdr32->e_shnum + 1;
    scn_names = (char **)calloc(scn_names_cnt, sizeof(char *));
    if (!scn_names) {
        print_error(dbg, "Out of malloc space in relocation print names",
            DW_DLV_OK, err);
    }
    printable_sects = (struct sect_data_s *)calloc(scn_names_cnt,
        sizeof(struct sect_data_s));
    if (!printable_sects) {
        free(scn_names);
        print_error(dbg, "Out of malloc space in relocation print sects",
            DW_DLV_OK, err);
    }

    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        ++sect_number;
        shdr32 = elf32_getshdr(scn);
        if (shdr32 == NULL) {
            free(printable_sects);
            free(scn_names);
            print_error(dbg, "DW_ELF_GETSHDR_ERROR", DW_DLV_OK, err);
        }
        scn_name = elf_strptr(elf, ehdr32->e_shstrndx, shdr32->sh_name);
        if (scn_name == NULL) {
            free(printable_sects);
            free(scn_names);
            print_error(dbg, "DW_ELF_STRPTR_ERROR", DW_DLV_OK, err);
        }

        scn_names[sect_number] = scn_name;
        if (shdr32->sh_type == SHT_SYMTAB) {
            size_t sym_size = 0;
            size_t count = 0;

            sym = (Elf32_Sym *) get_scndata(scn, &sym_size);
            if (sym == NULL) {
                free(printable_sects);
                free(scn_names);
                print_error(dbg, "No Elf32 symbol table data", DW_DLV_OK,
                    err);
            }
            count = sym_size / sizeof(Elf32_Sym);
            if(sym_size%sizeof(Elf32_Sym)) {
                print_error(dbg, "Elf32 problem reading .symtab data",
                    DW_DLV_OK, err);
            }
            sym++;
            count--;
            free(sym_data);
            sym_data = 0;
            sym_data = readsyms(sym, count, elf, shdr32->sh_link);
            sym_data_entry_count = count;
            if (sym_data  == NULL) {
                free(printable_sects);
                free(scn_names);
                print_error(dbg, "problem reading Elf32 symbol table data",
                    DW_DLV_OK, err);
            }
        } else {
            get_reloc_section(dbg,scn,scn_name,shdr32->sh_type,
                printable_sects,sect_number);
        }
    }  /* End while. */

    /* Set the relocation names based on the machine type */
    set_relocation_table_names(ehdr32->e_machine);
    for (i = 1; i < ehdr32->e_shnum + 1; i++) {
        if (printable_sects[i].display &&
            printable_sects[i].buf != NULL &&
            printable_sects[i].size > 0) {
            print_reloc_information_32(i,
                printable_sects[i].buf,
                printable_sects[i].size,
                printable_sects[i].type,
                scn_names,scn_names_cnt);
        }
    }
    free(printable_sects);
    free(scn_names);
}

#if HAVE_ELF64_R_INFO
#ifndef ELF64_R_TYPE
#define ELF64_R_TYPE(x) 0       /* FIXME */
#endif
#ifndef ELF64_R_SYM
#define ELF64_R_SYM(x) 0        /* FIXME */
#endif
#ifndef ELF64_ST_TYPE
#define ELF64_ST_TYPE(x) 0      /* FIXME */
#endif
#ifndef ELF64_ST_BIND
#define ELF64_ST_BIND(x) 0      /* FIXME */
#endif
#endif /* HAVE_ELF64_R_INFO */


static void
print_reloc_information_64(int section_no, Dwarf_Small * buf,
    Dwarf_Unsigned size, Elf64_Xword type,
    char **scn_names,int scn_names_count)
{
    /* Include support for Elf64_Rel and Elf64_Rela */
    Dwarf_Unsigned add = 0;
    Dwarf_Half rel_size = SHT_RELA == type ?
        sizeof(Elf64_Rela) : sizeof(Elf64_Rel);
    Dwarf_Unsigned off = 0;
    struct esb_s tempesb;
    struct esb_s tempesc;

    printf("\n[%3d] %s:\n",section_no, sanitized(scn_names[section_no]));
    /* Print some headers and change the order for better reading */
    printf("Offset     Addend     %-26s Index   Symbol Name\n","Reloc Type");

#if HAVE_ELF64_GETEHDR
    for (off = 0; off < size; off += rel_size) {
#if HAVE_ELF64_R_INFO
        /* This works for the Elf64_Rel in linux */
        Elf64_Rel *p = (Elf64_Rel *) (buf + off);
        char *name = 0;

        if (sym_data_64) {
            size_t index = ELF64_R_SYM(p->r_info) - 1;
            if (index < sym_data_64_entry_count) {
                name = sym_data_64[index].name;
                if (!name || !name[0]){
                    SYM64 *sym_64 = &sym_data_64[index];
                    if (sym_64->type == STT_SECTION &&
                        sym_64->shndx < scn_names_count) {
                        name = scn_names[sym_64->shndx];
                    }
                }
            }
        }
        if (!name || !name[0]) {
            name = "<no name>";
        }
        if (SHT_RELA == type) {
            Elf64_Rela *pa = (Elf64_Rela *)p;
            add = pa->r_addend;
        }
        esb_constructor(&tempesb);
        esb_constructor(&tempesc);
        get_reloc_type_names(ELF64_R_TYPE(p->r_info),&tempesc);
        esb_append(&tempesb,sanitized(esb_get_string(&tempesc)));
        /* sanitized uses a static buffer, call just once here */
        printf("0x%08lx 0x%08lx %-26s <%5ld> %s\n",
            (unsigned long int) (p->r_offset),
            (unsigned long int) (add),
            esb_get_string(&tempesb),
            (long)ELF64_R_SYM(p->r_info),
            sanitized(name));
        esb_destructor(&tempesb);
        esb_destructor(&tempesc);
#else  /* ! R_INFO */
        /*  sgi/mips -64 does not have r_info in the 64bit relocations,
            but seperate fields, with 3 types, actually. Only one of
            which prints here, as only one really used with dwarf */
        Elf64_Rel *p = (Elf64_Rel *) (buf + off);
        char *name = 0;

        /*  We subtract 1 from sym indexes since we left
            symtab entry 0 out of the sym_data[_64] array */
        if (sym_data_64) {
            size_t index = p->r_sym - 1;
            if (index < sym_data_64_entry_count) {
                name = sym_data_64[index].name;
            }
        }
        if (!name || !name[0]) {
            name = "<no name>";
        }
        esb_constructor(&tempesb);
        esb_constructor(&tempesc);
        get_reloc_type_names(p->r_type,&tempesc));
        esb_append(&tempesb,sanitized(esb_get_string(&tempesc)));
        printf("%5" DW_PR_DUu " %-26s <%3ld> %s\n",
            (Dwarf_Unsigned) (p->r_offset),
            esb_get_string(&tempesb),
            (long)p->r_sym,
            sanitized(name));
        esb_destructor(&tempesb);
        esb_destructor(&tempesc);
#endif
    }
#endif /* HAVE_ELF64_GETEHDR */
}

static void
print_reloc_information_32(int section_no, Dwarf_Small * buf,
   Dwarf_Unsigned size, Elf64_Xword type, char **scn_names,
   int scn_names_count)
{
    /*  Include support for Elf32_Rel and Elf32_Rela */
    Dwarf_Unsigned add = 0;
    Dwarf_Half rel_size = SHT_RELA == type ?
        sizeof(Elf32_Rela) : sizeof(Elf32_Rel);
    Dwarf_Unsigned off = 0;
    struct esb_s tempesb;
    struct esb_s tempesc;

    printf("\n[%3d] %s:\n",section_no, sanitized(scn_names[section_no]));

    /* Print some headers and change the order for better reading. */
    printf("Offset     Addend     %-26s Index   Symbol Name\n","Reloc Type");

    for (off = 0; off < size; off += rel_size) {
        Elf32_Rel *p = (Elf32_Rel *) (buf + off);
        char *name = 0;

        /*  We subtract 1 from sym indexes since we left
            symtab entry 0 out of the sym_data[_64] array */
        if (sym_data) {
            size_t index = ELF32_R_SYM(p->r_info) - 1;

            if(index < sym_data_entry_count) {
                name = sym_data[index].name;
                if ((!name || !name[0]) && sym_data) {
                    SYM *sym = &sym_data[index];
                    if (sym->type == STT_SECTION&&
                        sym->shndx < scn_names_count) {
                        name = scn_names[sym->shndx];
                    }
                }
            }
        }
        if (!name || !name[0]) {
            name = "<no name>";
        }
        if (SHT_RELA == type) {
            Elf32_Rela *pa = (Elf32_Rela *)p;
            add = pa->r_addend;
        }
        esb_constructor(&tempesb);
        esb_constructor(&tempesc);
        get_reloc_type_names(ELF32_R_TYPE(p->r_info),&tempesc);
        esb_append(&tempesb,esb_get_string(&tempesc));
        /* sanitized uses a static buffer, call just once here */
        printf("0x%08lx 0x%08lx %-26s <%5ld> %s\n",
            (unsigned long int) (p->r_offset),
            (unsigned long int) (add),
            esb_get_string(&tempesb),
            (long)ELF32_R_SYM(p->r_info),
            sanitized(name));
        esb_destructor(&tempesb);
        esb_destructor(&tempesc);
    }
}

/*  We are only reading and saving syms 1...num-1. */
static SYM *
readsyms(Elf32_Sym * data, size_t num, Elf * elf, Elf32_Word link)
{
    SYM *s      = 0;
    SYM *buf    = 0;
    indx_type i = 0;

    buf = (SYM *) calloc(num, sizeof(SYM));
    if (buf == NULL) {
        return NULL;
    }
    s = buf; /* save pointer to head of array */
    for (i = 0; i < num; i++, data++, buf++) {
        buf->indx = i;
        buf->name = (char *) elf_strptr(elf, link, data->st_name);
        buf->value = data->st_value;
        buf->size = data->st_size;
        buf->type = ELF32_ST_TYPE(data->st_info);
        buf->bind = ELF32_ST_BIND(data->st_info);
        buf->other = data->st_other;
        buf->shndx = data->st_shndx;
    }   /* end for loop */
    return (s);
}

/*  We are only reading and saving syms 1...num-1. */
static SYM64 *
read_64_syms(Elf64_Sym * data, size_t num, Elf * elf, Elf64_Word link)
{
#ifdef HAVE_ELF64_GETEHDR

    SYM64 *s    = 0;
    SYM64 *buf  = 0;
    indx_type i = 0;

    buf = (SYM64 *) calloc(num, sizeof(SYM64));
    if (buf == NULL) {
        return NULL;
    }
    s = buf;                    /* save pointer to head of array */
    for (i = 0; i < num; i++, data++, buf++) {
        buf->indx = i;
        buf->name = (char *) elf_strptr(elf, link, data->st_name);
        buf->value = data->st_value;
        buf->size = data->st_size;
        buf->type = ELF64_ST_TYPE(data->st_info);
        buf->bind = ELF64_ST_BIND(data->st_info);
        buf->other = data->st_other;
        buf->shndx = data->st_shndx;
    }                           /* end for loop */
    return (s);
#else
    return 0;
#endif /* HAVE_ELF64_GETEHDR */
}

static void *
get_scndata(Elf_Scn * fd_scn, size_t * scn_size)
{
    Elf_Data *p_data;

    p_data = 0;
    if ((p_data = elf_getdata(fd_scn, p_data)) == 0 ||
        p_data->d_size == 0) {
        return NULL;
    }
    *scn_size = p_data->d_size;
    return (p_data->d_buf);
}

/* Cleanup of malloc space (some of the pointers will be 0 here)
   so dwarfdump looks 'clean' to a malloc checker.
*/
void
clean_up_syms_malloc_data()
{
    free(sym_data);
    sym_data = 0;
    free(sym_data_64);
    sym_data_64 = 0;
    sym_data_64_entry_count = 0;
    sym_data_entry_count = 0;
}



void
print_object_header(Dwarf_Debug dbg)
{
    Elf *elf = 0;
    int res = 0;
    Dwarf_Error err = 0;

    /* Check if header information is required */
    res = dwarf_get_elf(dbg, &elf, &err);
    if (res == DW_DLV_NO_ENTRY) {
        printf(" No Elf, so no elf headers to print.\n");
        return;
    } else if (res == DW_DLV_ERROR) {
        print_error(dbg, "dwarf_get_elf error", res, err);
    }

    if (section_map_enabled(DW_HDR_HEADER)) {
#ifdef _WIN32
#ifdef HAVE_ELF32_GETEHDR
    /*  Standard libelf has no function generating the names of the
        encodings, but this libelf apparently does. */
    Elf_Ehdr_Literal eh_literals;
    Elf32_Ehdr *eh32 = 0;
#ifdef HAVE_ELF64_GETEHDR
    Elf64_Ehdr *eh64 = 0;
#endif /* HAVE_ELF64_GETEHDR */

    eh32 = elf32_getehdr(elf);
    if (eh32) {
        /* Get literal strings for header fields */
        elf32_gethdr_literals(eh32,&eh_literals);
        /* Print 32-bit obj header */
        printf("\nObject Header:\ne_ident:\n");
        printf("  File ID       = %s\n",eh_literals.e_ident_file_id);
        printf("  File class    = %02x (%s)\n",
            eh32->e_ident[EI_CLASS],eh_literals.e_ident_file_class);
        printf("  Data encoding = %02x (%s)\n",
            eh32->e_ident[EI_DATA],eh_literals.e_ident_data_encoding);
        printf("  File version  = %02x (%s)\n",
            eh32->e_ident[EI_VERSION],eh_literals.e_ident_file_version);
        printf("  OS ABI        = %02x (%s) (%s)\n",eh32->e_ident[EI_OSABI],
            eh_literals.e_ident_os_abi_s,eh_literals.e_ident_os_abi_l);
        printf("  ABI version   = %02x (%s)\n",
            eh32->e_ident[EI_ABIVERSION], eh_literals.e_ident_abi_version);
        printf("e_type     : 0x%x (%s)\n",
            eh32->e_type,eh_literals.e_type);
        printf("e_machine  : 0x%x (%s) (%s)\n",eh32->e_machine,
            eh_literals.e_machine_s,eh_literals.e_machine_l);
        printf("e_version  : 0x%x\n", eh32->e_version);
        printf("e_entry    : 0x%08x\n",eh32->e_entry);
        printf("e_phoff    : 0x%08x\n",eh32->e_phoff);
        printf("e_shoff    : 0x%08x\n",eh32->e_shoff);
        printf("e_flags    : 0x%x\n",eh32->e_flags);
        printf("e_ehsize   : 0x%x\n",eh32->e_ehsize);
        printf("e_phentsize: 0x%x\n",eh32->e_phentsize);
        printf("e_phnum    : 0x%x\n",eh32->e_phnum);
        printf("e_shentsize: 0x%x\n",eh32->e_shentsize);
        printf("e_shnum    : 0x%x\n",eh32->e_shnum);
        printf("e_shstrndx : 0x%x\n",eh32->e_shstrndx);
    }
    else {
#ifdef HAVE_ELF64_GETEHDR
        /* not a 32-bit obj */
        eh64 = elf64_getehdr(elf);
        if (eh64) {
            /* Get literal strings for header fields */
            elf64_gethdr_literals(eh64,&eh_literals);
            /* Print 64-bit obj header */
            printf("\nObject Header:\ne_ident:\n");
            printf("  File ID       = %s\n",eh_literals.e_ident_file_id);
            printf("  File class    = %02x (%s)\n",
                eh64->e_ident[EI_CLASS],eh_literals.e_ident_file_class);
            printf("  Data encoding = %02x (%s)\n",
                eh64->e_ident[EI_DATA],eh_literals.e_ident_data_encoding);
            printf("  File version  = %02x (%s)\n",
                eh64->e_ident[EI_VERSION],eh_literals.e_ident_file_version);
            printf("  OS ABI        = %02x (%s) (%s)\n",eh64->e_ident[EI_OSABI],
                eh_literals.e_ident_os_abi_s,eh_literals.e_ident_os_abi_l);
            printf("  ABI version   = %02x (%s)\n",
                eh64->e_ident[EI_ABIVERSION], eh_literals.e_ident_abi_version);
            printf("e_type     : 0x%x (%s)\n",
                eh64->e_type,eh_literals.e_type);
            printf("e_machine  : 0x%x (%s) (%s)\n",eh64->e_machine,
                eh_literals.e_machine_s,eh_literals.e_machine_l);
            printf("e_version  : 0x%x\n", eh64->e_version);
            printf("e_entry    : 0x%" DW_PR_XZEROS DW_PR_DUx "\n",eh64->e_entry);
            printf("e_phoff    : 0x%" DW_PR_XZEROS DW_PR_DUx "\n",eh64->e_phoff);
            printf("e_shoff    : 0x%" DW_PR_XZEROS DW_PR_DUx "\n",eh64->e_shoff);
            printf("e_flags    : 0x%x\n",eh64->e_flags);
            printf("e_ehsize   : 0x%x\n",eh64->e_ehsize);
            printf("e_phentsize: 0x%x\n",eh64->e_phentsize);
            printf("e_phnum    : 0x%x\n",eh64->e_phnum);
            printf("e_shentsize: 0x%x\n",eh64->e_shentsize);
            printf("e_shnum    : 0x%x\n",eh64->e_shnum);
            printf("e_shstrndx : 0x%x\n",eh64->e_shstrndx);
        }
#endif /* HAVE_ELF64_GETEHDR */
    }
#endif /* HAVE_ELF32_GETEHDR */
#endif /* _WIN32 */
    }
    /* Print basic section information is required */
    /* Mask only known sections (debug and text) bits */
    if (any_section_header_to_print()) {
        int nCount = 0;
        int section_index = 0;
        const char *section_name = NULL;
        Dwarf_Addr section_addr = 0;
        Dwarf_Unsigned section_size = 0;
        Dwarf_Error error = 0;
        Dwarf_Unsigned total_bytes = 0;
        int printed_sections = 0;

        /* Print section information (name, size, address). */
        nCount = dwarf_get_section_count(dbg);
        printf("\nInfo for %d sections:\n"
            "  Nro Index Address    Size(h)    Size(d)  Name\n",
            nCount);
        /* Ignore section with index=0 */
        for (section_index = 1; section_index < nCount;
            ++section_index) {
            res = dwarf_get_section_info_by_index(dbg,section_index,
                &section_name,
                &section_addr,
                &section_size,
                &error);
            if (res == DW_DLV_OK) {
                boolean print_it = FALSE;

                /* Use original mapping */
                /* Check if the section name is a debug section */
                print_it = section_name_is_debug_and_wanted(
                    section_name);
                if (print_it) {
                    ++printed_sections;
                    printf("  %3d "                         /* nro */
                        "0x%03x "                        /* index */
                        "0x%" DW_PR_XZEROS DW_PR_DUx " " /* address */
                        "0x%" DW_PR_XZEROS DW_PR_DUx " " /* size (hex) */
                        "%" DW_PR_XZEROS DW_PR_DUu " "   /* size (dec) */
                        "%s\n",                          /* name */
                        printed_sections,
                        section_index,
                        section_addr,
                        section_size, section_size,
                        section_name);
                    total_bytes += section_size;
                }
            }
        }
        printf("*** Summary: %" DW_PR_DUu " bytes for %d section(s) ***\n",
            total_bytes, printed_sections);
    }
}
#endif /* DWARF_WITH_LIBELF */

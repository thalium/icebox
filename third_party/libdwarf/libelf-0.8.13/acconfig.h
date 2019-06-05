/*
 * acconfig.h - Special definitions for libelf, processed by autoheader.
 * Copyright (C) 1995 - 2001, 2004, 2006 Michael Riepe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* @(#) $Id: acconfig.h,v 1.16 2008/05/23 08:17:56 michael Exp $ */

/* Define if you want to include extra debugging code */
#undef ENABLE_DEBUG

/* Define if you want to support extended ELF formats */
#undef ENABLE_EXTENDED_FORMAT

/* Define if you want ELF format sanity checks by default */
#undef ENABLE_SANITY_CHECKS

/* Define if memmove() does not copy overlapping arrays correctly */
#undef HAVE_BROKEN_MEMMOVE

/* Define if you have the catgets function. */
#undef HAVE_CATGETS

/* Define if you have the dgettext function. */
#undef HAVE_DGETTEXT

/* Define if you have the memset function.  */
#undef HAVE_MEMSET

/* Define if struct nlist is declared in <elf.h> or <sys/elf.h> */
#undef HAVE_STRUCT_NLIST_DECLARATION

/* Define if Elf32_Dyn is declared in <link.h> */
#undef __LIBELF_NEED_LINK_H

/* Define if Elf32_Dyn is declared in <sys/link.h> */
#undef __LIBELF_NEED_SYS_LINK_H

/* Define to `<elf.h>' or `<sys/elf.h>' if one of them is present */
#undef __LIBELF_HEADER_ELF_H

/* Define if you want 64-bit support (and your system supports it) */
#undef __LIBELF64

/* Define if you want 64-bit support, and are running IRIX */
#undef __LIBELF64_IRIX

/* Define if you want 64-bit support, and are running Linux */
#undef __LIBELF64_LINUX

/* Define if you want symbol versioning (and your system supports it) */
#undef __LIBELF_SYMBOL_VERSIONS

/* Define if symbol versioning uses Sun section type (SHT_SUNW_*) */
#undef __LIBELF_SUN_SYMBOL_VERSIONS

/* Define if symbol versioning uses GNU section types (SHT_GNU_*) */
#undef __LIBELF_GNU_SYMBOL_VERSIONS

/* Define to a 64-bit signed integer type if one exists */
#undef __libelf_i64_t

/* Define to a 64-bit unsigned integer type if one exists */
#undef __libelf_u64_t

/* Define to a 32-bit signed integer type if one exists */
#undef __libelf_i32_t

/* Define to a 32-bit unsigned integer type if one exists */
#undef __libelf_u32_t

/* Define to a 16-bit signed integer type if one exists */
#undef __libelf_i16_t

/* Define to a 16-bit unsigned integer type if one exists */
#undef __libelf_u16_t

/* $Id: DBGPlugInCommonELF.h $ */
/** @file
 * DBGPlugInCommonELF - Common code for dealing with ELF images, Header.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___Debugger_DBGPlugInCommonELF_h
#define ___Debugger_DBGPlugInCommonELF_h

#include <VBox/types.h>
#include <iprt/formats/elf32.h>
#include <iprt/formats/elf64.h>

/** @name DBGDiggerCommonParseElf32Mod and DBGDiggerCommonParseElf64Mod flags
 * @{ */
/** Whether to adjust the symbol values or not. */
#define DBG_DIGGER_ELF_ADJUST_SYM_VALUE     RT_BIT_32(0)
/** Indicates that we're missing section headers and that
 * all section indexes are to be considered invalid. (Solaris hack.)
 * This flag is incompatible with DBG_DIGGER_ELF_ADJUST_SYM_VALUE. */
#define DBG_DIGGER_ELF_FUNNY_SHDRS          RT_BIT_32(1)
/** Valid bit mask. */
#define DBG_DIGGER_ELF_MASK                 UINT32_C(0x00000003)
/* @} */

int DBGDiggerCommonParseElf32Mod(PUVM pUVM, const char *pszModName, const char *pszFilename, uint32_t fFlags,
                                 Elf32_Ehdr const *pEhdr, Elf32_Shdr const *paShdrs,
                                 Elf32_Sym const *paSyms, size_t cMaxSyms,
                                 char const *pbStrings, size_t cbMaxStrings,
                                 RTGCPTR MinAddr, RTGCPTR MaxAddr, uint64_t uModTag);

int DBGDiggerCommonParseElf64Mod(PUVM pUVM, const char *pszModName, const char *pszFilename, uint32_t fFlags,
                                 Elf64_Ehdr const *pEhdr, Elf64_Shdr const *paShdrs,
                                 Elf64_Sym const *paSyms, size_t cMaxSyms,
                                 char const *pbStrings, size_t cbMaxStrings,
                                 RTGCPTR MinAddr, RTGCPTR MaxAddr, uint64_t uModTag);

#endif


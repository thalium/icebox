/* $Id: ldrELF.cpp $ */
/** @file
 * IPRT - Binary Image Loader, Executable and Linker Format (ELF).
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_LDR
#include <iprt/ldr.h>
#include "internal/iprt.h"

#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/log.h>
#include <iprt/err.h>
#include <iprt/formats/elf32.h>
#include <iprt/formats/elf64.h>
#include <iprt/formats/elf-i386.h>
#include <iprt/formats/elf-amd64.h>
#include "internal/ldr.h"



/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Finds an ELF symbol table string. */
#define ELF_STR(pHdrs, iStr) ((pHdrs)->pStr + (iStr))
/** Finds an ELF section header string. */
#define ELF_SH_STR(pHdrs, iStr) ((pHdrs)->pShStr + (iStr))



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#ifdef LOG_ENABLED
static const char *rtldrElfGetShdrType(uint32_t iType);
#endif


/* Select ELF mode and include the template. */
#define ELF_MODE            32
#define Elf_Reloc           Elf_Rel
#include "ldrELFRelocatable.cpp.h"
#undef ELF_MODE
#undef Elf_Reloc


#define ELF_MODE            64
#define Elf_Reloc           Elf_Rela
#include "ldrELFRelocatable.cpp.h"
#undef ELF_MODE
#undef Elf_Reloc


#ifdef LOG_ENABLED
/**
 * Gets the section type.
 *
 * @returns Pointer to read only string.
 * @param   iType       The section type index.
 */
static const char *rtldrElfGetShdrType(uint32_t iType)
{
    switch (iType)
    {
        case SHT_NULL:          return "SHT_NULL";
        case SHT_PROGBITS:      return "SHT_PROGBITS";
        case SHT_SYMTAB:        return "SHT_SYMTAB";
        case SHT_STRTAB:        return "SHT_STRTAB";
        case SHT_RELA:          return "SHT_RELA";
        case SHT_HASH:          return "SHT_HASH";
        case SHT_DYNAMIC:       return "SHT_DYNAMIC";
        case SHT_NOTE:          return "SHT_NOTE";
        case SHT_NOBITS:        return "SHT_NOBITS";
        case SHT_REL:           return "SHT_REL";
        case SHT_SHLIB:         return "SHT_SHLIB";
        case SHT_DYNSYM:        return "SHT_DYNSYM";
        default:
            return "";
    }
}
#endif


/**
 * Open an ELF image.
 *
 * @returns iprt status code.
 * @param   pReader     The loader reader instance which will provide the raw image bits.
 * @param   fFlags      Reserved, MBZ.
 * @param   enmArch     Architecture specifier.
 * @param   phLdrMod    Where to store the handle.
 * @param   pErrInfo    Where to return extended error information. Optional.
 */
int rtldrELFOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, PRTLDRMOD phLdrMod, PRTERRINFO pErrInfo)
{
    const char *pszLogName = pReader->pfnLogName(pReader); NOREF(pszLogName);

    RT_NOREF_PV(pErrInfo); /** @todo implement */

    /*
     * Read the ident to decide if this is 32-bit or 64-bit
     * and worth dealing with.
     */
    uint8_t e_ident[EI_NIDENT];
    int rc = pReader->pfnRead(pReader, &e_ident, sizeof(e_ident), 0);
    if (RT_FAILURE(rc))
        return rc;
    if (    e_ident[EI_MAG0] != ELFMAG0
        ||  e_ident[EI_MAG1] != ELFMAG1
        ||  e_ident[EI_MAG2] != ELFMAG2
        ||  e_ident[EI_MAG3] != ELFMAG3
        ||  (   e_ident[EI_CLASS] != ELFCLASS32
             && e_ident[EI_CLASS] != ELFCLASS64)
       )
    {
        Log(("RTLdrELF: %s: Unsupported/invalid ident %.*Rhxs\n", pszLogName, sizeof(e_ident), e_ident));
        return VERR_BAD_EXE_FORMAT;
    }
    if (e_ident[EI_DATA] != ELFDATA2LSB)
    {
        Log(("RTLdrELF: %s: ELF endian %x is unsupported\n", pszLogName, e_ident[EI_DATA]));
        return VERR_LDRELF_ODD_ENDIAN;
    }
    if (e_ident[EI_CLASS] == ELFCLASS32)
        rc = rtldrELF32Open(pReader, fFlags, enmArch, phLdrMod);
    else
        rc = rtldrELF64Open(pReader, fFlags, enmArch, phLdrMod);
    return rc;
}


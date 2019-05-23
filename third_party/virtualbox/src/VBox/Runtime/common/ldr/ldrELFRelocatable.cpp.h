/* $Id: ldrELFRelocatable.cpp.h $ */
/** @file
 * IPRT - Binary Image Loader, Template for ELF Relocatable Images.
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


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#if ELF_MODE == 32
#define RTLDRELF_NAME(name) rtldrELF32##name
#define RTLDRELF_SUFF(name) name##32
#define RTLDRELF_MID(pre,suff) pre##32##suff
#define FMT_ELF_ADDR    "%08RX32"
#define FMT_ELF_HALF    "%04RX16"
#define FMT_ELF_OFF     "%08RX32"
#define FMT_ELF_SIZE    "%08RX32"
#define FMT_ELF_SWORD   "%RI32"
#define FMT_ELF_WORD    "%08RX32"
#define FMT_ELF_XWORD   "%08RX32"
#define FMT_ELF_SXWORD  "%RI32"

#elif ELF_MODE == 64
#define RTLDRELF_NAME(name) rtldrELF64##name
#define RTLDRELF_SUFF(name) name##64
#define RTLDRELF_MID(pre,suff) pre##64##suff
#define FMT_ELF_ADDR    "%016RX64"
#define FMT_ELF_HALF    "%04RX16"
#define FMT_ELF_SHALF   "%RI16"
#define FMT_ELF_OFF     "%016RX64"
#define FMT_ELF_SIZE    "%016RX64"
#define FMT_ELF_SWORD   "%RI32"
#define FMT_ELF_WORD    "%08RX32"
#define FMT_ELF_XWORD   "%016RX64"
#define FMT_ELF_SXWORD  "%RI64"
#endif

#define Elf_Ehdr            RTLDRELF_MID(Elf,_Ehdr)
#define Elf_Phdr            RTLDRELF_MID(Elf,_Phdr)
#define Elf_Shdr            RTLDRELF_MID(Elf,_Shdr)
#define Elf_Sym             RTLDRELF_MID(Elf,_Sym)
#define Elf_Rel             RTLDRELF_MID(Elf,_Rel)
#define Elf_Rela            RTLDRELF_MID(Elf,_Rela)
#define Elf_Nhdr            RTLDRELF_MID(Elf,_Nhdr)
#define Elf_Dyn             RTLDRELF_MID(Elf,_Dyn)
#define Elf_Addr            RTLDRELF_MID(Elf,_Addr)
#define Elf_Half            RTLDRELF_MID(Elf,_Half)
#define Elf_Off             RTLDRELF_MID(Elf,_Off)
#define Elf_Size            RTLDRELF_MID(Elf,_Size)
#define Elf_Sword           RTLDRELF_MID(Elf,_Sword)
#define Elf_Word            RTLDRELF_MID(Elf,_Word)

#define RTLDRMODELF         RTLDRELF_MID(RTLDRMODELF,RT_NOTHING)
#define PRTLDRMODELF        RTLDRELF_MID(PRTLDRMODELF,RT_NOTHING)

#define ELF_R_SYM(info)     RTLDRELF_MID(ELF,_R_SYM)(info)
#define ELF_R_TYPE(info)    RTLDRELF_MID(ELF,_R_TYPE)(info)
#define ELF_R_INFO(sym, type) RTLDRELF_MID(ELF,_R_INFO)(sym, type)

#define ELF_ST_BIND(info)   RTLDRELF_MID(ELF,_ST_BIND)(info)



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * The ELF loader structure.
 */
typedef struct RTLDRMODELF
{
    /** Core module structure. */
    RTLDRMODINTERNAL        Core;
    /** Pointer to readonly mapping of the image bits.
     * This mapping is provided by the pReader. */
    const void             *pvBits;

    /** The ELF header. */
    Elf_Ehdr                Ehdr;
    /** Pointer to our copy of the section headers with sh_addr as RVAs.
     * The virtual addresses in this array is the 0 based assignments we've given the image.
     * Not valid if the image is DONE. */
    Elf_Shdr               *paShdrs;
    /** Unmodified section headers (allocated after paShdrs, so no need to free).
     * Not valid if the image is DONE. */
    Elf_Shdr const         *paOrgShdrs;
    /** The size of the loaded image. */
    size_t                  cbImage;

    /** The image base address if it's an EXEC or DYN image. */
    Elf_Addr                LinkAddress;

    /** The symbol section index. */
    unsigned                iSymSh;
    /** Number of symbols in the table. */
    unsigned                cSyms;
    /** Pointer to symbol table within RTLDRMODELF::pvBits. */
    const Elf_Sym          *paSyms;

    /** The string section index. */
    unsigned                iStrSh;
    /** Size of the string table. */
    unsigned                cbStr;
    /** Pointer to string table within RTLDRMODELF::pvBits. */
    const char             *pStr;

    /** Size of the section header string table. */
    unsigned                cbShStr;
    /** Pointer to section header string table within RTLDRMODELF::pvBits. */
    const char             *pShStr;
} RTLDRMODELF, *PRTLDRMODELF;


/**
 * Maps the image bits into memory and resolve pointers into it.
 *
 * @returns iprt status code.
 * @param   pModElf         The ELF loader module instance data.
 * @param   fNeedsBits      Set if we actually need the pvBits member.
 *                          If we don't, we can simply read the string and symbol sections, thus saving memory.
 */
static int RTLDRELF_NAME(MapBits)(PRTLDRMODELF pModElf, bool fNeedsBits)
{
    NOREF(fNeedsBits);
    if (pModElf->pvBits)
        return VINF_SUCCESS;
    int rc = pModElf->Core.pReader->pfnMap(pModElf->Core.pReader, &pModElf->pvBits);
    if (RT_SUCCESS(rc))
    {
        const uint8_t *pu8 = (const uint8_t *)pModElf->pvBits;
        if (pModElf->iSymSh != ~0U)
            pModElf->paSyms = (const Elf_Sym *)(pu8 + pModElf->paShdrs[pModElf->iSymSh].sh_offset);
        if (pModElf->iStrSh != ~0U)
            pModElf->pStr   =    (const char *)(pu8 + pModElf->paShdrs[pModElf->iStrSh].sh_offset);
        pModElf->pShStr     =    (const char *)(pu8 + pModElf->paShdrs[pModElf->Ehdr.e_shstrndx].sh_offset);
    }
    return rc;
}


/*
 *
 * EXEC & DYN.
 * EXEC & DYN.
 * EXEC & DYN.
 * EXEC & DYN.
 * EXEC & DYN.
 *
 */


/**
 * Applies the fixups for a section in an executable image.
 *
 * @returns iprt status code.
 * @param   pModElf         The ELF loader module instance data.
 * @param   BaseAddr        The base address which the module is being fixedup to.
 * @param   pfnGetImport    The callback function to use to resolve imports (aka unresolved externals).
 * @param   pvUser          User argument to pass to the callback.
 * @param   SecAddr         The section address. This is the address the relocations are relative to.
 * @param   cbSec           The section size. The relocations must be inside this.
 * @param   pu8SecBaseR     Where we read section bits from.
 * @param   pu8SecBaseW     Where we write section bits to.
 * @param   pvRelocs        Pointer to where we read the relocations from.
 * @param   cbRelocs        Size of the relocations.
 */
static int RTLDRELF_NAME(RelocateSectionExecDyn)(PRTLDRMODELF pModElf, Elf_Addr BaseAddr,
                                                 PFNRTLDRIMPORT pfnGetImport, void *pvUser,
                                                 const Elf_Addr SecAddr, Elf_Size cbSec,
                                                 const uint8_t *pu8SecBaseR, uint8_t *pu8SecBaseW,
                                                 const void *pvRelocs, Elf_Size cbRelocs)
{
#if ELF_MODE != 32
    NOREF(pu8SecBaseR);
#endif

    /*
     * Iterate the relocations.
     * The relocations are stored in an array of Elf32_Rel records and covers the entire relocation section.
     */
    const Elf_Addr    offDelta = BaseAddr - pModElf->LinkAddress;
    const Elf_Reloc  *paRels   = (const Elf_Reloc *)pvRelocs;
    const unsigned    iRelMax   = (unsigned)(cbRelocs / sizeof(paRels[0]));
    AssertMsgReturn(iRelMax == cbRelocs / sizeof(paRels[0]), (FMT_ELF_SIZE "\n", cbRelocs / sizeof(paRels[0])),
                    VERR_IMAGE_TOO_BIG);
    for (unsigned iRel = 0; iRel < iRelMax; iRel++)
    {
        /*
         * Skip R_XXX_NONE entries early to avoid confusion in the symbol
         * getter code.
         */
#if   ELF_MODE == 32
        if (ELF_R_TYPE(paRels[iRel].r_info) == R_386_NONE)
            continue;
#elif ELF_MODE == 64
        if (ELF_R_TYPE(paRels[iRel].r_info) == R_X86_64_NONE)
            continue;
#endif

        /*
         * Validate and find the symbol, resolve undefined ones.
         */
        Elf_Size iSym = ELF_R_SYM(paRels[iRel].r_info);
        if (iSym >= pModElf->cSyms)
        {
            AssertMsgFailed(("iSym=%d is an invalid symbol index!\n", iSym));
            return VERR_LDRELF_INVALID_SYMBOL_INDEX;
        }
        const Elf_Sym *pSym = &pModElf->paSyms[iSym];
        if (pSym->st_name >= pModElf->cbStr)
        {
            AssertMsgFailed(("iSym=%d st_name=%d str sh_size=%d\n", iSym, pSym->st_name, pModElf->cbStr));
            return VERR_LDRELF_INVALID_SYMBOL_NAME_OFFSET;
        }

        Elf_Addr SymValue = 0;
        if (pSym->st_shndx == SHN_UNDEF)
        {
            /* Try to resolve the symbol. */
            const char *pszName = ELF_STR(pModElf, pSym->st_name);
            RTUINTPTR   ExtValue;
            int rc = pfnGetImport(&pModElf->Core, "", pszName, ~0U, &ExtValue, pvUser);
            AssertMsgRCReturn(rc, ("Failed to resolve '%s' rc=%Rrc\n", pszName, rc), rc);
            SymValue = (Elf_Addr)ExtValue;
            AssertMsgReturn((RTUINTPTR)SymValue == ExtValue, ("Symbol value overflowed! '%s'\n", pszName),
                            VERR_SYMBOL_VALUE_TOO_BIG);
            Log2(("rtldrELF: #%-3d - UNDEF " FMT_ELF_ADDR " '%s'\n", iSym, SymValue, pszName));
        }
        else
        {
            AssertMsgReturn(pSym->st_shndx < pModElf->Ehdr.e_shnum || pSym->st_shndx == SHN_ABS, ("%#x\n", pSym->st_shndx),
                            VERR_LDRELF_INVALID_RELOCATION_OFFSET);
#if   ELF_MODE == 64
            SymValue = pSym->st_value;
#endif
        }

#if   ELF_MODE == 64
        /* Calc the value (indexes checked above; assumes SHN_UNDEF == 0). */
        Elf_Addr Value;
        if (pSym->st_shndx < pModElf->Ehdr.e_shnum)
            Value = SymValue + offDelta;
        else /* SHN_ABS: */
            Value = SymValue + paRels[iRel].r_addend;
#endif

        /*
         * Apply the fixup.
         */
        AssertMsgReturn(paRels[iRel].r_offset < cbSec, (FMT_ELF_ADDR " " FMT_ELF_SIZE "\n", paRels[iRel].r_offset, cbSec), VERR_LDRELF_INVALID_RELOCATION_OFFSET);
#if   ELF_MODE == 32
        const Elf_Addr *pAddrR = (const Elf_Addr *)(pu8SecBaseR + paRels[iRel].r_offset);    /* Where to read the addend. */
#endif
        Elf_Addr       *pAddrW =       (Elf_Addr *)(pu8SecBaseW + paRels[iRel].r_offset);    /* Where to write the fixup. */
        switch (ELF_R_TYPE(paRels[iRel].r_info))
        {
#if   ELF_MODE == 32
            /*
             * Absolute addressing.
             */
            case R_386_32:
            {
                Elf_Addr Value;
                if (pSym->st_shndx < pModElf->Ehdr.e_shnum)
                    Value = *pAddrR + offDelta;         /* Simplified. */
                else if (pSym->st_shndx == SHN_ABS)
                    continue;                           /* Internal fixup, no need to apply it. */
                else if (pSym->st_shndx == SHN_UNDEF)
                    Value = SymValue + *pAddrR;
                else
                    AssertFailedReturn(VERR_LDR_GENERAL_FAILURE); /** @todo SHN_COMMON */
                *(uint32_t *)pAddrW = Value;
                Log4((FMT_ELF_ADDR": R_386_32   Value=" FMT_ELF_ADDR "\n", SecAddr + paRels[iRel].r_offset + BaseAddr, Value));
                break;
            }

            /*
             * PC relative addressing.
             */
            case R_386_PC32:
            {
                Elf_Addr Value;
                if (pSym->st_shndx < pModElf->Ehdr.e_shnum)
                    continue;                           /* Internal fixup, no need to apply it. */
                else if (pSym->st_shndx == SHN_ABS)
                    Value = *pAddrR + offDelta;         /* Simplified. */
                else if (pSym->st_shndx == SHN_UNDEF)
                {
                    const Elf_Addr SourceAddr = SecAddr + paRels[iRel].r_offset + BaseAddr; /* Where the source really is. */
                    Value = SymValue + *(uint32_t *)pAddrR - SourceAddr;
                    *(uint32_t *)pAddrW = Value;
                }
                else
                    AssertFailedReturn(VERR_LDR_GENERAL_FAILURE); /** @todo SHN_COMMON */
                Log4((FMT_ELF_ADDR": R_386_PC32 Value=" FMT_ELF_ADDR "\n", SecAddr + paRels[iRel].r_offset + BaseAddr, Value));
                break;
            }

#elif ELF_MODE == 64

            /*
             * Absolute addressing
             */
            case R_X86_64_64:
            {
                *(uint64_t *)pAddrW = Value;
                Log4((FMT_ELF_ADDR": R_X86_64_64   Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SecAddr + paRels[iRel].r_offset + BaseAddr, Value, SymValue));
                break;
            }

            /*
             * Truncated 32-bit value (zero-extendedable to the 64-bit value).
             */
            case R_X86_64_32:
            {
                *(uint32_t *)pAddrW = (uint32_t)Value;
                Log4((FMT_ELF_ADDR": R_X86_64_32   Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SecAddr + paRels[iRel].r_offset + BaseAddr, Value, SymValue));
                AssertMsgReturn((Elf_Addr)*(uint32_t *)pAddrW == SymValue, ("Value=" FMT_ELF_ADDR "\n", SymValue),
                                VERR_SYMBOL_VALUE_TOO_BIG);
                break;
            }

            /*
             * Truncated 32-bit value (sign-extendedable to the 64-bit value).
             */
            case R_X86_64_32S:
            {
                *(int32_t *)pAddrW = (int32_t)Value;
                Log4((FMT_ELF_ADDR": R_X86_64_32S  Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SecAddr + paRels[iRel].r_offset + BaseAddr, Value, SymValue));
                AssertMsgReturn((Elf_Addr)*(int32_t *)pAddrW == Value, ("Value=" FMT_ELF_ADDR "\n", Value), VERR_SYMBOL_VALUE_TOO_BIG); /** @todo check the sign-extending here. */
                break;
            }

            /*
             * PC relative addressing.
             */
            case R_X86_64_PC32:
            {
                const Elf_Addr SourceAddr = SecAddr + paRels[iRel].r_offset + BaseAddr; /* Where the source really is. */
                Value -= SourceAddr;
                *(int32_t *)pAddrW = (int32_t)Value;
                Log4((FMT_ELF_ADDR": R_X86_64_PC32 Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SourceAddr, Value, SymValue));
                AssertMsgReturn((Elf_Addr)*(int32_t *)pAddrW == Value, ("Value=" FMT_ELF_ADDR "\n", Value), VERR_SYMBOL_VALUE_TOO_BIG); /** @todo check the sign-extending here. */
                break;
            }
#endif

            default:
                AssertMsgFailed(("Unknown relocation type: %d (iRel=%d iRelMax=%d)\n",
                                 ELF_R_TYPE(paRels[iRel].r_info), iRel, iRelMax));
                return VERR_LDRELF_RELOCATION_NOT_SUPPORTED;
        }
    }

    return VINF_SUCCESS;
}



/*
 *
 * REL
 * REL
 * REL
 * REL
 * REL
 *
 */

/**
 * Get the symbol and symbol value.
 *
 * @returns iprt status code.
 * @param   pModElf         The ELF loader module instance data.
 * @param   BaseAddr        The base address which the module is being fixedup to.
 * @param   pfnGetImport    The callback function to use to resolve imports (aka unresolved externals).
 * @param   pvUser          User argument to pass to the callback.
 * @param   iSym            The symbol to get.
 * @param   ppSym           Where to store the symbol pointer on success. (read only)
 * @param   pSymValue       Where to store the symbol value on success.
 */
static int RTLDRELF_NAME(Symbol)(PRTLDRMODELF pModElf, Elf_Addr BaseAddr, PFNRTLDRIMPORT pfnGetImport, void *pvUser,
                                 Elf_Size iSym, const Elf_Sym **ppSym, Elf_Addr *pSymValue)
{
    /*
     * Validate and find the symbol.
     */
    if (iSym >= pModElf->cSyms)
    {
        AssertMsgFailed(("iSym=%d is an invalid symbol index!\n", iSym));
        return VERR_LDRELF_INVALID_SYMBOL_INDEX;
    }
    const Elf_Sym *pSym = &pModElf->paSyms[iSym];
    *ppSym = pSym;

    if (pSym->st_name >= pModElf->cbStr)
    {
        AssertMsgFailed(("iSym=%d st_name=%d str sh_size=%d\n", iSym, pSym->st_name, pModElf->cbStr));
        return VERR_LDRELF_INVALID_SYMBOL_NAME_OFFSET;
    }
    const char *pszName = ELF_STR(pModElf, pSym->st_name);

    /*
     * Determine the symbol value.
     *
     * Symbols needs different treatment depending on which section their are in.
     * Undefined and absolute symbols goes into special non-existing sections.
     */
    switch (pSym->st_shndx)
    {
        /*
         * Undefined symbol, needs resolving.
         *
         * Since ELF has no generic concept of importing from specific module (the OS/2 ELF format
         * has but that's a OS extension and only applies to programs and dlls), we'll have to ask
         * the resolver callback to do a global search.
         */
        case SHN_UNDEF:
        {
            /* Try to resolve the symbol. */
            RTUINTPTR Value;
            int rc = pfnGetImport(&pModElf->Core, "", pszName, ~0U, &Value, pvUser);
            if (RT_FAILURE(rc))
            {
                AssertMsgFailed(("Failed to resolve '%s' rc=%Rrc\n", pszName, rc));
                return rc;
            }
            *pSymValue = (Elf_Addr)Value;
            if ((RTUINTPTR)*pSymValue != Value)
            {
                AssertMsgFailed(("Symbol value overflowed! '%s'\n", pszName));
                return VERR_SYMBOL_VALUE_TOO_BIG;
            }

            Log2(("rtldrELF: #%-3d - UNDEF " FMT_ELF_ADDR " '%s'\n", iSym, *pSymValue, pszName));
            break;
        }

        /*
         * Absolute symbols needs no fixing since they are, well, absolute.
         */
        case SHN_ABS:
            *pSymValue = pSym->st_value;
            Log2(("rtldrELF: #%-3d - ABS   " FMT_ELF_ADDR " '%s'\n", iSym, *pSymValue, pszName));
            break;

        /*
         * All other symbols are addressed relative to their section and need to be fixed up.
         */
        default:
            if (pSym->st_shndx >= pModElf->Ehdr.e_shnum)
            {
                /* what about common symbols? */
                AssertMsg(pSym->st_shndx < pModElf->Ehdr.e_shnum,
                          ("iSym=%d st_shndx=%d e_shnum=%d pszName=%s\n", iSym, pSym->st_shndx, pModElf->Ehdr.e_shnum, pszName));
                return VERR_BAD_EXE_FORMAT;
            }
            *pSymValue = pSym->st_value + pModElf->paShdrs[pSym->st_shndx].sh_addr + BaseAddr;
            Log2(("rtldrELF: #%-3d - %5d " FMT_ELF_ADDR " '%s'\n", iSym, pSym->st_shndx, *pSymValue, pszName));
            break;
    }

    return VINF_SUCCESS;
}


/**
 * Applies the fixups for a sections.
 *
 * @returns iprt status code.
 * @param   pModElf         The ELF loader module instance data.
 * @param   BaseAddr        The base address which the module is being fixedup to.
 * @param   pfnGetImport    The callback function to use to resolve imports (aka unresolved externals).
 * @param   pvUser          User argument to pass to the callback.
 * @param   SecAddr         The section address. This is the address the relocations are relative to.
 * @param   cbSec           The section size. The relocations must be inside this.
 * @param   pu8SecBaseR     Where we read section bits from.
 * @param   pu8SecBaseW     Where we write section bits to.
 * @param   pvRelocs        Pointer to where we read the relocations from.
 * @param   cbRelocs        Size of the relocations.
 */
static int RTLDRELF_NAME(RelocateSection)(PRTLDRMODELF pModElf, Elf_Addr BaseAddr, PFNRTLDRIMPORT pfnGetImport, void *pvUser,
                                          const Elf_Addr SecAddr, Elf_Size cbSec, const uint8_t *pu8SecBaseR, uint8_t *pu8SecBaseW,
                                          const void *pvRelocs, Elf_Size cbRelocs)
{
#if ELF_MODE != 32
    NOREF(pu8SecBaseR);
#endif

    /*
     * Iterate the relocations.
     * The relocations are stored in an array of Elf32_Rel records and covers the entire relocation section.
     */
    const Elf_Reloc  *paRels = (const Elf_Reloc *)pvRelocs;
    const unsigned   iRelMax = (unsigned)(cbRelocs / sizeof(paRels[0]));
    AssertMsgReturn(iRelMax == cbRelocs / sizeof(paRels[0]), (FMT_ELF_SIZE "\n", cbRelocs / sizeof(paRels[0])), VERR_IMAGE_TOO_BIG);
    for (unsigned iRel = 0; iRel < iRelMax; iRel++)
    {
        /*
         * Skip R_XXX_NONE entries early to avoid confusion in the symbol
         * getter code.
         */
#if   ELF_MODE == 32
        if (ELF_R_TYPE(paRels[iRel].r_info) == R_386_NONE)
            continue;
#elif ELF_MODE == 64
        if (ELF_R_TYPE(paRels[iRel].r_info) == R_X86_64_NONE)
            continue;
#endif


        /*
         * Get the symbol.
         */
        const Elf_Sym  *pSym = NULL; /* shut up gcc */
        Elf_Addr        SymValue = 0; /* shut up gcc-4 */
        int rc = RTLDRELF_NAME(Symbol)(pModElf, BaseAddr, pfnGetImport, pvUser, ELF_R_SYM(paRels[iRel].r_info), &pSym, &SymValue);
        if (RT_FAILURE(rc))
            return rc;

        Log3(("rtldrELF: " FMT_ELF_ADDR " %02x %06x - " FMT_ELF_ADDR " %3d %02x %s\n",
              paRels[iRel].r_offset, ELF_R_TYPE(paRels[iRel].r_info), (unsigned)ELF_R_SYM(paRels[iRel].r_info),
              SymValue, (unsigned)pSym->st_shndx, pSym->st_info, ELF_STR(pModElf, pSym->st_name)));

        /*
         * Apply the fixup.
         */
        AssertMsgReturn(paRels[iRel].r_offset < cbSec, (FMT_ELF_ADDR " " FMT_ELF_SIZE "\n", paRels[iRel].r_offset, cbSec), VERR_LDRELF_INVALID_RELOCATION_OFFSET);
#if   ELF_MODE == 32
        const Elf_Addr *pAddrR = (const Elf_Addr *)(pu8SecBaseR + paRels[iRel].r_offset);    /* Where to read the addend. */
#endif
        Elf_Addr       *pAddrW =       (Elf_Addr *)(pu8SecBaseW + paRels[iRel].r_offset);    /* Where to write the fixup. */
        switch (ELF_R_TYPE(paRels[iRel].r_info))
        {
#if   ELF_MODE == 32
            /*
             * Absolute addressing.
             */
            case R_386_32:
            {
                const Elf_Addr Value = SymValue + *pAddrR;
                *(uint32_t *)pAddrW = Value;
                Log4((FMT_ELF_ADDR": R_386_32   Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SecAddr + paRels[iRel].r_offset + BaseAddr, Value, SymValue));
                break;
            }

            /*
             * PC relative addressing.
             */
            case R_386_PC32:
            {
                const Elf_Addr SourceAddr = SecAddr + paRels[iRel].r_offset + BaseAddr; /* Where the source really is. */
                const Elf_Addr Value = SymValue + *(uint32_t *)pAddrR - SourceAddr;
                *(uint32_t *)pAddrW = Value;
                Log4((FMT_ELF_ADDR": R_386_PC32 Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SourceAddr, Value, SymValue));
                break;
            }

            /* ignore */
            case R_386_NONE:
                break;

#elif ELF_MODE == 64

            /*
             * Absolute addressing
             */
            case R_X86_64_64:
            {
                const Elf_Addr Value = SymValue + paRels[iRel].r_addend;
                *(uint64_t *)pAddrW = Value;
                Log4((FMT_ELF_ADDR": R_X86_64_64   Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SecAddr + paRels[iRel].r_offset + BaseAddr, Value, SymValue));
                break;
            }

            /*
             * Truncated 32-bit value (zero-extendedable to the 64-bit value).
             */
            case R_X86_64_32:
            {
                const Elf_Addr Value = SymValue + paRels[iRel].r_addend;
                *(uint32_t *)pAddrW = (uint32_t)Value;
                Log4((FMT_ELF_ADDR": R_X86_64_32   Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SecAddr + paRels[iRel].r_offset + BaseAddr, Value, SymValue));
                AssertMsgReturn((Elf_Addr)*(uint32_t *)pAddrW == Value, ("Value=" FMT_ELF_ADDR "\n", Value), VERR_SYMBOL_VALUE_TOO_BIG);
                break;
            }

            /*
             * Truncated 32-bit value (sign-extendedable to the 64-bit value).
             */
            case R_X86_64_32S:
            {
                const Elf_Addr Value = SymValue + paRels[iRel].r_addend;
                *(int32_t *)pAddrW = (int32_t)Value;
                Log4((FMT_ELF_ADDR": R_X86_64_32S  Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SecAddr + paRels[iRel].r_offset + BaseAddr, Value, SymValue));
                AssertMsgReturn((Elf_Addr)*(int32_t *)pAddrW == Value, ("Value=" FMT_ELF_ADDR "\n", Value), VERR_SYMBOL_VALUE_TOO_BIG); /** @todo check the sign-extending here. */
                break;
            }

            /*
             * PC relative addressing.
             */
            case R_X86_64_PC32:
            {
                const Elf_Addr SourceAddr = SecAddr + paRels[iRel].r_offset + BaseAddr; /* Where the source really is. */
                const Elf_Addr Value = SymValue + paRels[iRel].r_addend - SourceAddr;
                *(int32_t *)pAddrW = (int32_t)Value;
                Log4((FMT_ELF_ADDR": R_X86_64_PC32 Value=" FMT_ELF_ADDR " SymValue=" FMT_ELF_ADDR "\n",
                      SourceAddr, Value, SymValue));
                AssertMsgReturn((Elf_Addr)*(int32_t *)pAddrW == Value, ("Value=" FMT_ELF_ADDR "\n", Value), VERR_SYMBOL_VALUE_TOO_BIG); /** @todo check the sign-extending here. */
                break;
            }

            /* ignore */
            case R_X86_64_NONE:
                break;
#endif

            default:
                AssertMsgFailed(("Unknown relocation type: %d (iRel=%d iRelMax=%d)\n",
                                 ELF_R_TYPE(paRels[iRel].r_info), iRel, iRelMax));
                return VERR_LDRELF_RELOCATION_NOT_SUPPORTED;
        }
    }

    return VINF_SUCCESS;
}



/** @copydoc RTLDROPS::pfnClose */
static DECLCALLBACK(int) RTLDRELF_NAME(Close)(PRTLDRMODINTERNAL pMod)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;

    if (pModElf->paShdrs)
    {
        RTMemFree(pModElf->paShdrs);
        pModElf->paShdrs = NULL;
    }

    pModElf->pvBits = NULL;

    return VINF_SUCCESS;
}


/** @copydoc RTLDROPS::Done */
static DECLCALLBACK(int) RTLDRELF_NAME(Done)(PRTLDRMODINTERNAL pMod)
{
    NOREF(pMod); /*PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;*/
    /** @todo  Have to think more about this .... */
    return -1;
}


/** @copydoc RTLDROPS::EnumSymbols */
static DECLCALLBACK(int) RTLDRELF_NAME(EnumSymbols)(PRTLDRMODINTERNAL pMod, unsigned fFlags, const void *pvBits, RTUINTPTR BaseAddress,
                                                    PFNRTLDRENUMSYMS pfnCallback, void *pvUser)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;
    NOREF(pvBits);

    /*
     * Validate the input.
     */
    Elf_Addr BaseAddr = (Elf_Addr)BaseAddress;
    AssertMsgReturn((RTUINTPTR)BaseAddr == BaseAddress, ("%RTptr", BaseAddress), VERR_IMAGE_BASE_TOO_HIGH);

    /*
     * Make sure we've got the string and symbol tables. (We don't need the pvBits.)
     */
    int rc = RTLDRELF_NAME(MapBits)(pModElf, false);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Enumerate the symbol table.
     */
    const Elf_Sym  *paSyms = pModElf->paSyms;
    unsigned        cSyms  = pModElf->cSyms;
    for (unsigned iSym = 1; iSym < cSyms; iSym++)
    {
        /*
         * Skip imports (undefined).
         */
        if (paSyms[iSym].st_shndx != SHN_UNDEF)
        {
            /*
             * Calc value and get name.
             */
            Elf_Addr Value;
            if (paSyms[iSym].st_shndx == SHN_ABS)
                /* absolute symbols are not subject to any relocation. */
                Value = paSyms[iSym].st_value;
            else if (paSyms[iSym].st_shndx < pModElf->Ehdr.e_shnum)
            {
                if (pModElf->Ehdr.e_type == ET_REL)
                    /* relative to the section. */
                    Value = BaseAddr + paSyms[iSym].st_value + pModElf->paShdrs[paSyms[iSym].st_shndx].sh_addr;
                else /* Fixed up for link address. */
                    Value = BaseAddr + paSyms[iSym].st_value - pModElf->LinkAddress;
            }
            else
            {
                AssertMsgFailed(("Arg! paSyms[%u].st_shndx=" FMT_ELF_HALF "\n", iSym, paSyms[iSym].st_shndx));
                return VERR_BAD_EXE_FORMAT;
            }
            const char *pszName = ELF_STR(pModElf, paSyms[iSym].st_name);
            if (    (pszName && *pszName)
                &&  (   (fFlags & RTLDR_ENUM_SYMBOL_FLAGS_ALL)
                     || ELF_ST_BIND(paSyms[iSym].st_info) == STB_GLOBAL)
               )
            {
                /*
                 * Call back.
                 */
                AssertMsgReturn(Value == (RTUINTPTR)Value, (FMT_ELF_ADDR "\n", Value), VERR_SYMBOL_VALUE_TOO_BIG);
                rc = pfnCallback(pMod, pszName, ~0U, (RTUINTPTR)Value, pvUser);
                if (rc)
                    return rc;
            }
        }
    }

    return VINF_SUCCESS;
}


/** @copydoc RTLDROPS::GetImageSize */
static DECLCALLBACK(size_t) RTLDRELF_NAME(GetImageSize)(PRTLDRMODINTERNAL pMod)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;

    return pModElf->cbImage;
}


/** @copydoc RTLDROPS::GetBits */
static DECLCALLBACK(int) RTLDRELF_NAME(GetBits)(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR BaseAddress, PFNRTLDRIMPORT pfnGetImport, void *pvUser)
{
    PRTLDRMODELF    pModElf = (PRTLDRMODELF)pMod;

    /*
     * This operation is currently only available on relocatable images.
     */
    switch (pModElf->Ehdr.e_type)
    {
        case ET_REL:
            break;
        case ET_EXEC:
            Log(("RTLdrELF: %s: Executable images are not supported yet!\n", pModElf->Core.pReader->pfnLogName(pModElf->Core.pReader)));
            return VERR_LDRELF_EXEC;
        case ET_DYN:
            Log(("RTLdrELF: %s: Dynamic images are not supported yet!\n", pModElf->Core.pReader->pfnLogName(pModElf->Core.pReader)));
            return VERR_LDRELF_DYN;
        default: AssertFailedReturn(VERR_BAD_EXE_FORMAT);
    }

    /*
     * Load the bits into pvBits.
     */
    const Elf_Shdr *paShdrs = pModElf->paShdrs;
    for (unsigned iShdr = 0; iShdr < pModElf->Ehdr.e_shnum; iShdr++)
    {
        if (paShdrs[iShdr].sh_flags & SHF_ALLOC)
        {
            AssertMsgReturn((size_t)paShdrs[iShdr].sh_size == (size_t)paShdrs[iShdr].sh_size, (FMT_ELF_SIZE "\n", paShdrs[iShdr].sh_size), VERR_IMAGE_TOO_BIG);
            switch (paShdrs[iShdr].sh_type)
            {
                case SHT_NOBITS:
                    memset((uint8_t *)pvBits + paShdrs[iShdr].sh_addr, 0, (size_t)paShdrs[iShdr].sh_size);
                    break;

                case SHT_PROGBITS:
                default:
                {
                    int rc = pModElf->Core.pReader->pfnRead(pModElf->Core.pReader, (uint8_t *)pvBits + paShdrs[iShdr].sh_addr,
                                                            (size_t)paShdrs[iShdr].sh_size, paShdrs[iShdr].sh_offset);
                    if (RT_FAILURE(rc))
                    {
                        Log(("RTLdrELF: %s: Read error when reading " FMT_ELF_SIZE " bytes at " FMT_ELF_OFF ", iShdr=%d\n",
                             pModElf->Core.pReader->pfnLogName(pModElf->Core.pReader),
                             paShdrs[iShdr].sh_size, paShdrs[iShdr].sh_offset, iShdr));
                        return rc;
                    }
                }
            }
        }
    }

    /*
     * Relocate the image.
     */
    return pModElf->Core.pOps->pfnRelocate(pMod, pvBits, BaseAddress, ~(RTUINTPTR)0, pfnGetImport, pvUser);
}


/** @copydoc RTLDROPS::Relocate */
static DECLCALLBACK(int) RTLDRELF_NAME(Relocate)(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR NewBaseAddress,
                                                 RTUINTPTR OldBaseAddress, PFNRTLDRIMPORT pfnGetImport, void *pvUser)
{
    PRTLDRMODELF    pModElf = (PRTLDRMODELF)pMod;
#ifdef LOG_ENABLED
    const char     *pszLogName = pModElf->Core.pReader->pfnLogName(pModElf->Core.pReader);
#endif
    NOREF(OldBaseAddress);

    /*
     * This operation is currently only available on relocatable images.
     */
    switch (pModElf->Ehdr.e_type)
    {
        case ET_REL:
            break;
        case ET_EXEC:
            Log(("RTLdrELF: %s: Executable images are not supported yet!\n", pszLogName));
            return VERR_LDRELF_EXEC;
        case ET_DYN:
            Log(("RTLdrELF: %s: Dynamic images are not supported yet!\n", pszLogName));
            return VERR_LDRELF_DYN;
        default: AssertFailedReturn(VERR_BAD_EXE_FORMAT);
    }

    /*
     * Validate the input.
     */
    Elf_Addr BaseAddr = (Elf_Addr)NewBaseAddress;
    AssertMsgReturn((RTUINTPTR)BaseAddr == NewBaseAddress, ("%RTptr", NewBaseAddress), VERR_IMAGE_BASE_TOO_HIGH);

    /*
     * Map the image bits if not already done and setup pointer into it.
     */
    int rc = RTLDRELF_NAME(MapBits)(pModElf, true);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Iterate the sections looking for interesting SHT_REL[A] sections.
     * SHT_REL[A] sections have the section index of the section they contain fixups
     * for in the sh_info member.
     */
    const Elf_Shdr *paShdrs = pModElf->paShdrs;
    Log2(("rtLdrElf: %s: Fixing up image\n", pszLogName));
    for (unsigned iShdr = 0; iShdr < pModElf->Ehdr.e_shnum; iShdr++)
    {
        const Elf_Shdr *pShdrRel = &paShdrs[iShdr];

        /*
         * Skip sections without interest to us.
         */
#if ELF_MODE == 32
        if (pShdrRel->sh_type != SHT_REL)
#else
        if (pShdrRel->sh_type != SHT_RELA)
#endif
            continue;
        if (pShdrRel->sh_info >= pModElf->Ehdr.e_shnum)
            continue;
        const Elf_Shdr *pShdr = &paShdrs[pShdrRel->sh_info]; /* the section to fixup. */
        if (!(pShdr->sh_flags & SHF_ALLOC))
            continue;

        /*
         * Relocate the section.
         */
        Log2(("rtldrELF: %s: Relocation records for #%d [%s] (sh_info=%d sh_link=%d) found in #%d [%s] (sh_info=%d sh_link=%d)\n",
              pszLogName, (int)pShdrRel->sh_info, ELF_SH_STR(pModElf, pShdr->sh_name), (int)pShdr->sh_info, (int)pShdr->sh_link,
              iShdr, ELF_SH_STR(pModElf, pShdrRel->sh_name), (int)pShdrRel->sh_info, (int)pShdrRel->sh_link));

        /** @todo Make RelocateSection a function pointer so we can select the one corresponding to the machine when opening the image. */
        if (pModElf->Ehdr.e_type == ET_REL)
            rc = RTLDRELF_NAME(RelocateSection)(pModElf, BaseAddr, pfnGetImport, pvUser,
                                                pShdr->sh_addr,
                                                pShdr->sh_size,
                                                (const uint8_t *)pModElf->pvBits + pShdr->sh_offset,
                                                (uint8_t *)pvBits + pShdr->sh_addr,
                                                (const uint8_t *)pModElf->pvBits + pShdrRel->sh_offset,
                                                pShdrRel->sh_size);
        else
            rc = RTLDRELF_NAME(RelocateSectionExecDyn)(pModElf, BaseAddr, pfnGetImport, pvUser,
                                                       pShdr->sh_addr,
                                                       pShdr->sh_size,
                                                       (const uint8_t *)pModElf->pvBits + pShdr->sh_offset,
                                                       (uint8_t *)pvBits + pShdr->sh_addr,
                                                       (const uint8_t *)pModElf->pvBits + pShdrRel->sh_offset,
                                                       pShdrRel->sh_size);
        if (RT_FAILURE(rc))
            return rc;
    }
    return VINF_SUCCESS;
}


/**
 * Worker for pfnGetSymbolEx.
 */
static int RTLDRELF_NAME(ReturnSymbol)(PRTLDRMODELF pThis, const Elf_Sym *pSym, Elf_Addr uBaseAddr, PRTUINTPTR pValue)
{
    Elf_Addr Value;
    if (pSym->st_shndx == SHN_ABS)
        /* absolute symbols are not subject to any relocation. */
        Value = pSym->st_value;
    else if (pSym->st_shndx < pThis->Ehdr.e_shnum)
    {
        if (pThis->Ehdr.e_type == ET_REL)
            /* relative to the section. */
            Value = uBaseAddr + pSym->st_value + pThis->paShdrs[pSym->st_shndx].sh_addr;
        else /* Fixed up for link address. */
            Value = uBaseAddr + pSym->st_value - pThis->LinkAddress;
    }
    else
    {
        AssertMsgFailed(("Arg! pSym->st_shndx=%d\n", pSym->st_shndx));
        return VERR_BAD_EXE_FORMAT;
    }
    AssertMsgReturn(Value == (RTUINTPTR)Value, (FMT_ELF_ADDR "\n", Value), VERR_SYMBOL_VALUE_TOO_BIG);
    *pValue = (RTUINTPTR)Value;
    return VINF_SUCCESS;
}


/** @copydoc RTLDROPS::pfnGetSymbolEx */
static DECLCALLBACK(int) RTLDRELF_NAME(GetSymbolEx)(PRTLDRMODINTERNAL pMod, const void *pvBits, RTUINTPTR BaseAddress,
                                                    uint32_t iOrdinal, const char *pszSymbol, RTUINTPTR *pValue)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;
    NOREF(pvBits);

    /*
     * Validate the input.
     */
    Elf_Addr uBaseAddr = (Elf_Addr)BaseAddress;
    AssertMsgReturn((RTUINTPTR)uBaseAddr == BaseAddress, ("%RTptr", BaseAddress), VERR_IMAGE_BASE_TOO_HIGH);

    /*
     * Map the image bits if not already done and setup pointer into it.
     */
    int rc = RTLDRELF_NAME(MapBits)(pModElf, true);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Calc all kinds of pointers before we start iterating the symbol table.
     */
    const Elf_Sym     *paSyms = pModElf->paSyms;
    unsigned            cSyms = pModElf->cSyms;
    if (iOrdinal == UINT32_MAX)
    {
        const char     *pStr  = pModElf->pStr;
        for (unsigned iSym = 1; iSym < cSyms; iSym++)
        {
            /* Undefined symbols are not exports, they are imports. */
            if (    paSyms[iSym].st_shndx != SHN_UNDEF
                &&  (   ELF_ST_BIND(paSyms[iSym].st_info) == STB_GLOBAL
                     || ELF_ST_BIND(paSyms[iSym].st_info) == STB_WEAK))
            {
                /* Validate the name string and try match with it. */
                if (paSyms[iSym].st_name < pModElf->cbStr)
                {
                    if (!strcmp(pszSymbol, pStr + paSyms[iSym].st_name))
                    {
                        /* matched! */
                        return RTLDRELF_NAME(ReturnSymbol)(pModElf, &paSyms[iSym], uBaseAddr, pValue);
                    }
                }
                else
                {
                    AssertMsgFailed(("String outside string table! iSym=%d paSyms[iSym].st_name=%#x\n", iSym, paSyms[iSym].st_name));
                    return VERR_LDRELF_INVALID_SYMBOL_NAME_OFFSET;
                }
            }
        }
    }
    else if (iOrdinal < cSyms)
    {
        if (    paSyms[iOrdinal].st_shndx != SHN_UNDEF
            &&  (   ELF_ST_BIND(paSyms[iOrdinal].st_info) == STB_GLOBAL
                 || ELF_ST_BIND(paSyms[iOrdinal].st_info) == STB_WEAK))
            return RTLDRELF_NAME(ReturnSymbol)(pModElf, &paSyms[iOrdinal], uBaseAddr, pValue);
    }

    return VERR_SYMBOL_NOT_FOUND;
}


/** @copydoc RTLDROPS::pfnEnumDbgInfo */
static DECLCALLBACK(int) RTLDRELF_NAME(EnumDbgInfo)(PRTLDRMODINTERNAL pMod, const void *pvBits,
                                                    PFNRTLDRENUMDBG pfnCallback, void *pvUser)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;
    RT_NOREF_PV(pvBits);

    /*
     * Map the image bits if not already done and setup pointer into it.
     */
    int rc = RTLDRELF_NAME(MapBits)(pModElf, true);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Do the enumeration.
     */
    const Elf_Shdr *paShdrs = pModElf->paOrgShdrs;
    for (unsigned iShdr = 0; iShdr < pModElf->Ehdr.e_shnum; iShdr++)
    {
        /* Debug sections are expected to be PROGBITS and not allocated. */
        if (paShdrs[iShdr].sh_type != SHT_PROGBITS)
            continue;
        if (paShdrs[iShdr].sh_flags & SHF_ALLOC)
            continue;

        RTLDRDBGINFO DbgInfo;
        const char *pszSectName = ELF_SH_STR(pModElf, paShdrs[iShdr].sh_name);
        if (   !strncmp(pszSectName, RT_STR_TUPLE(".debug_"))
            || !strcmp(pszSectName, ".WATCOM_references") )
        {
            RT_ZERO(DbgInfo.u);
            DbgInfo.enmType         = RTLDRDBGINFOTYPE_DWARF;
            DbgInfo.pszExtFile      = NULL;
            DbgInfo.offFile         = paShdrs[iShdr].sh_offset;
            DbgInfo.cb              = paShdrs[iShdr].sh_size;
            DbgInfo.u.Dwarf.pszSection = pszSectName;
        }
        else if (!strcmp(pszSectName, ".gnu_debuglink"))
        {
            if ((paShdrs[iShdr].sh_size & 3) || paShdrs[iShdr].sh_size < 8)
                return VERR_BAD_EXE_FORMAT;

            RT_ZERO(DbgInfo.u);
            DbgInfo.enmType         = RTLDRDBGINFOTYPE_DWARF_DWO;
            DbgInfo.pszExtFile      = (const char *)((uintptr_t)pModElf->pvBits + (uintptr_t)paShdrs[iShdr].sh_offset);
            if (!RTStrEnd(DbgInfo.pszExtFile, paShdrs[iShdr].sh_size))
                return VERR_BAD_EXE_FORMAT;
            DbgInfo.u.Dwo.uCrc32    = *(uint32_t *)((uintptr_t)DbgInfo.pszExtFile + (uintptr_t)paShdrs[iShdr].sh_size
                                                    - sizeof(uint32_t));
            DbgInfo.offFile         = -1;
            DbgInfo.cb              = 0;
        }
        else
            continue;

        DbgInfo.LinkAddress         = NIL_RTLDRADDR;
        DbgInfo.iDbgInfo            = iShdr - 1;

        rc = pfnCallback(pMod, &DbgInfo, pvUser);
        if (rc != VINF_SUCCESS)
            return rc;

    }

    return VINF_SUCCESS;
}


/**
 * Helper that locates the first allocated section.
 *
 * @returns Pointer to the section header if found, NULL if none.
 * @param   pShdr   The section header to start searching at.
 * @param   cLeft   The number of section headers left to search. Can be 0.
 */
static const Elf_Shdr *RTLDRELF_NAME(GetFirstAllocatedSection)(const Elf_Shdr *pShdr, unsigned cLeft)
{
    while (cLeft-- > 0)
    {
        if (pShdr->sh_flags & SHF_ALLOC)
            return pShdr;
        pShdr++;
    }
    return NULL;
}

/** @copydoc RTLDROPS::pfnEnumSegments. */
static DECLCALLBACK(int) RTLDRELF_NAME(EnumSegments)(PRTLDRMODINTERNAL pMod, PFNRTLDRENUMSEGS pfnCallback, void *pvUser)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;

    /*
     * Map the image bits if not already done and setup pointer into it.
     */
    int rc = RTLDRELF_NAME(MapBits)(pModElf, true);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Do the enumeration.
     */
    char            szName[32];
    Elf_Addr        uPrevMappedRva = 0;
    const Elf_Shdr *paShdrs    = pModElf->paShdrs;
    const Elf_Shdr *paOrgShdrs = pModElf->paOrgShdrs;
    for (unsigned iShdr = 1; iShdr < pModElf->Ehdr.e_shnum; iShdr++)
    {
        RTLDRSEG Seg;
        Seg.pszName     = ELF_SH_STR(pModElf, paShdrs[iShdr].sh_name);
        Seg.cchName     = (uint32_t)strlen(Seg.pszName);
        if (Seg.cchName == 0)
        {
            Seg.pszName = szName;
            Seg.cchName = (uint32_t)RTStrPrintf(szName, sizeof(szName), "UnamedSect%02u", iShdr);
        }
        Seg.SelFlat     = 0;
        Seg.Sel16bit    = 0;
        Seg.fFlags      = 0;
        Seg.fProt       = RTMEM_PROT_READ;
        if (paShdrs[iShdr].sh_flags & SHF_WRITE)
            Seg.fProt  |= RTMEM_PROT_WRITE;
        if (paShdrs[iShdr].sh_flags & SHF_EXECINSTR)
            Seg.fProt  |= RTMEM_PROT_EXEC;
        Seg.cb          = paShdrs[iShdr].sh_size;
        Seg.Alignment   = paShdrs[iShdr].sh_addralign;
        if (paShdrs[iShdr].sh_flags & SHF_ALLOC)
        {
            Seg.LinkAddress = paOrgShdrs[iShdr].sh_addr;
            Seg.RVA         = paShdrs[iShdr].sh_addr;
            const Elf_Shdr *pShdr2 = RTLDRELF_NAME(GetFirstAllocatedSection)(&paShdrs[iShdr + 1],
                                                                             pModElf->Ehdr.e_shnum - iShdr - 1);
            if (   pShdr2
                && pShdr2->sh_addr >= paShdrs[iShdr].sh_addr
                && Seg.RVA >= uPrevMappedRva)
                Seg.cbMapped = pShdr2->sh_addr - paShdrs[iShdr].sh_addr;
            else
                Seg.cbMapped = RT_MAX(paShdrs[iShdr].sh_size, paShdrs[iShdr].sh_addralign);
            uPrevMappedRva = Seg.RVA;
        }
        else
        {
            Seg.LinkAddress = NIL_RTLDRADDR;
            Seg.RVA         = NIL_RTLDRADDR;
            Seg.cbMapped    = NIL_RTLDRADDR;
        }
        if (paShdrs[iShdr].sh_type != SHT_NOBITS)
        {
            Seg.offFile     = paShdrs[iShdr].sh_offset;
            Seg.cbFile      = paShdrs[iShdr].sh_size;
        }
        else
        {
            Seg.offFile     = -1;
            Seg.cbFile      = 0;
        }

        rc = pfnCallback(pMod, &Seg, pvUser);
        if (rc != VINF_SUCCESS)
            return rc;
    }

    return VINF_SUCCESS;
}


/** @copydoc RTLDROPS::pfnLinkAddressToSegOffset. */
static DECLCALLBACK(int) RTLDRELF_NAME(LinkAddressToSegOffset)(PRTLDRMODINTERNAL pMod, RTLDRADDR LinkAddress,
                                                               uint32_t *piSeg, PRTLDRADDR poffSeg)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;

    const Elf_Shdr *pShdrEnd = NULL;
    unsigned        cLeft    = pModElf->Ehdr.e_shnum - 1;
    const Elf_Shdr *pShdr    = &pModElf->paOrgShdrs[cLeft];
    while (cLeft-- > 0)
    {
        if (pShdr->sh_flags & SHF_ALLOC)
        {
            RTLDRADDR offSeg = LinkAddress - pShdr->sh_addr;
            if (offSeg < pShdr->sh_size)
            {
                *poffSeg = offSeg;
                *piSeg   = cLeft;
                return VINF_SUCCESS;
            }
            if (offSeg == pShdr->sh_size)
                pShdrEnd = pShdr;
        }
        pShdr--;
    }

    if (pShdrEnd)
    {
        *poffSeg = pShdrEnd->sh_size;
        *piSeg   = pShdrEnd - pModElf->paOrgShdrs - 1;
        return VINF_SUCCESS;
    }

    return VERR_LDR_INVALID_LINK_ADDRESS;
}


/** @copydoc RTLDROPS::pfnLinkAddressToRva. */
static DECLCALLBACK(int) RTLDRELF_NAME(LinkAddressToRva)(PRTLDRMODINTERNAL pMod, RTLDRADDR LinkAddress, PRTLDRADDR pRva)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;
    uint32_t     iSeg;
    RTLDRADDR    offSeg;
    int rc = RTLDRELF_NAME(LinkAddressToSegOffset)(pMod, LinkAddress, &iSeg, &offSeg);
    if (RT_SUCCESS(rc))
        *pRva = pModElf->paShdrs[iSeg + 1].sh_addr + offSeg;
    return rc;
}


/** @copydoc RTLDROPS::pfnSegOffsetToRva. */
static DECLCALLBACK(int) RTLDRELF_NAME(SegOffsetToRva)(PRTLDRMODINTERNAL pMod, uint32_t iSeg, RTLDRADDR offSeg,
                                                       PRTLDRADDR pRva)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;
    if (iSeg >= pModElf->Ehdr.e_shnum - 1U)
        return VERR_LDR_INVALID_SEG_OFFSET;

    iSeg++; /* skip section 0 */
    if (offSeg > pModElf->paShdrs[iSeg].sh_size)
    {
        const Elf_Shdr *pShdr2 = RTLDRELF_NAME(GetFirstAllocatedSection)(&pModElf->paShdrs[iSeg + 1],
                                                                         pModElf->Ehdr.e_shnum - iSeg - 1);
        if (   !pShdr2
            || offSeg > (pShdr2->sh_addr - pModElf->paShdrs[iSeg].sh_addr))
            return VERR_LDR_INVALID_SEG_OFFSET;
    }

    if (!(pModElf->paShdrs[iSeg].sh_flags & SHF_ALLOC))
        return VERR_LDR_INVALID_SEG_OFFSET;

    *pRva = pModElf->paShdrs[iSeg].sh_addr;
    return VINF_SUCCESS;
}


/** @copydoc RTLDROPS::pfnRvaToSegOffset. */
static DECLCALLBACK(int) RTLDRELF_NAME(RvaToSegOffset)(PRTLDRMODINTERNAL pMod, RTLDRADDR Rva,
                                                       uint32_t *piSeg, PRTLDRADDR poffSeg)
{
    PRTLDRMODELF pModElf = (PRTLDRMODELF)pMod;

    Elf_Addr        PrevAddr = 0;
    unsigned        cLeft    = pModElf->Ehdr.e_shnum - 1;
    const Elf_Shdr *pShdr    = &pModElf->paShdrs[cLeft];
    while (cLeft-- > 0)
    {
        if (pShdr->sh_flags & SHF_ALLOC)
        {
            Elf_Addr    cbSeg  = PrevAddr ? PrevAddr - pShdr->sh_addr : pShdr->sh_size;
            RTLDRADDR   offSeg = Rva - pShdr->sh_addr;
            if (offSeg <= cbSeg)
            {
                *poffSeg = offSeg;
                *piSeg   = cLeft;
                return VINF_SUCCESS;
            }
            PrevAddr = pShdr->sh_addr;
        }
        pShdr--;
    }

    return VERR_LDR_INVALID_RVA;
}


/** @callback_method_impl{FNRTLDRIMPORT, Stub used by ReadDbgInfo.} */
static DECLCALLBACK(int) RTLDRELF_NAME(GetImportStubCallback)(RTLDRMOD hLdrMod, const char *pszModule, const char *pszSymbol,
                                                              unsigned uSymbol, PRTLDRADDR pValue, void *pvUser)
{
    RT_NOREF_PV(hLdrMod); RT_NOREF_PV(pszModule); RT_NOREF_PV(pszSymbol);
    RT_NOREF_PV(uSymbol); RT_NOREF_PV(pValue); RT_NOREF_PV(pvUser);
    return VERR_SYMBOL_NOT_FOUND;
}


/** @copydoc RTLDROPS::pfnRvaToSegOffset. */
static DECLCALLBACK(int) RTLDRELF_NAME(ReadDbgInfo)(PRTLDRMODINTERNAL pMod, uint32_t iDbgInfo, RTFOFF off,
                                                    size_t cb, void *pvBuf)
{
    PRTLDRMODELF pThis = (PRTLDRMODELF)pMod;
    LogFlow(("%s: iDbgInfo=%#x off=%RTfoff cb=%#zu\n", __FUNCTION__, iDbgInfo, off, cb));

    /*
     * Input validation.
     */
    AssertReturn(iDbgInfo < pThis->Ehdr.e_shnum && iDbgInfo + 1 < pThis->Ehdr.e_shnum, VERR_INVALID_PARAMETER);
    iDbgInfo++;
    AssertReturn(!(pThis->paShdrs[iDbgInfo].sh_flags & SHF_ALLOC), VERR_INVALID_PARAMETER);
    AssertReturn(pThis->paShdrs[iDbgInfo].sh_type   == SHT_PROGBITS, VERR_INVALID_PARAMETER);
    AssertReturn(pThis->paShdrs[iDbgInfo].sh_offset == (uint64_t)off, VERR_INVALID_PARAMETER);
    AssertReturn(pThis->paShdrs[iDbgInfo].sh_size   == cb, VERR_INVALID_PARAMETER);
    RTFOFF cbRawImage = pThis->Core.pReader->pfnSize(pThis->Core.pReader);
    AssertReturn(cbRawImage >= 0, VERR_INVALID_PARAMETER);
    AssertReturn(off >= 0 && cb <= (uint64_t)cbRawImage && (uint64_t)off + cb <= (uint64_t)cbRawImage, VERR_INVALID_PARAMETER);

    /*
     * Read it from the file and look for fixup sections.
     */
    int rc;
    if (pThis->pvBits)
        memcpy(pvBuf, (const uint8_t *)pThis->pvBits + (size_t)off, cb);
    else
    {
        rc = pThis->Core.pReader->pfnRead(pThis->Core.pReader, pvBuf, cb, off);
        if (RT_FAILURE(rc))
            return rc;
    }

    uint32_t iRelocs = iDbgInfo + 1;
    if (   iRelocs >= pThis->Ehdr.e_shnum
        || pThis->paShdrs[iRelocs].sh_info != iDbgInfo
        || (   pThis->paShdrs[iRelocs].sh_type != SHT_REL
            && pThis->paShdrs[iRelocs].sh_type != SHT_RELA) )
    {
        iRelocs = 0;
        while (   iRelocs < pThis->Ehdr.e_shnum
               && (   pThis->paShdrs[iRelocs].sh_info != iDbgInfo
                   || (   pThis->paShdrs[iRelocs].sh_type != SHT_REL
                       && pThis->paShdrs[iRelocs].sh_type != SHT_RELA)) )
            iRelocs++;
    }
    if (   iRelocs < pThis->Ehdr.e_shnum
        && pThis->paShdrs[iRelocs].sh_size > 0)
    {
        /*
         * Load the relocations.
         */
        uint8_t       *pbRelocsBuf = NULL;
        const uint8_t *pbRelocs;
        if (pThis->pvBits)
            pbRelocs = (const uint8_t *)pThis->pvBits + pThis->paShdrs[iRelocs].sh_offset;
        else
        {
            pbRelocs = pbRelocsBuf = (uint8_t *)RTMemTmpAlloc(pThis->paShdrs[iRelocs].sh_size);
            if (!pbRelocsBuf)
                return VERR_NO_TMP_MEMORY;
            rc = pThis->Core.pReader->pfnRead(pThis->Core.pReader, pbRelocsBuf,
                                              pThis->paShdrs[iRelocs].sh_size,
                                              pThis->paShdrs[iRelocs].sh_offset);
            if (RT_FAILURE(rc))
            {
                RTMemTmpFree(pbRelocsBuf);
                return rc;
            }
        }

        /*
         * Apply the relocations.
         */
        if (pThis->Ehdr.e_type == ET_REL)
            rc = RTLDRELF_NAME(RelocateSection)(pThis, pThis->LinkAddress,
                                                RTLDRELF_NAME(GetImportStubCallback), NULL /*pvUser*/,
                                                pThis->paShdrs[iDbgInfo].sh_addr,
                                                pThis->paShdrs[iDbgInfo].sh_size,
                                                (const uint8_t *)pvBuf,
                                                (uint8_t *)pvBuf,
                                                pbRelocs,
                                                pThis->paShdrs[iRelocs].sh_size);
        else
            rc = RTLDRELF_NAME(RelocateSectionExecDyn)(pThis, pThis->LinkAddress,
                                                       RTLDRELF_NAME(GetImportStubCallback), NULL /*pvUser*/,
                                                       pThis->paShdrs[iDbgInfo].sh_addr,
                                                       pThis->paShdrs[iDbgInfo].sh_size,
                                                       (const uint8_t *)pvBuf,
                                                       (uint8_t *)pvBuf,
                                                       pbRelocs,
                                                       pThis->paShdrs[iRelocs].sh_size);

        RTMemTmpFree(pbRelocsBuf);
    }
    else
        rc = VINF_SUCCESS;
    return rc;
}



/**
 * The ELF module operations.
 */
static RTLDROPS RTLDRELF_MID(s_rtldrElf,Ops) =
{
#if   ELF_MODE == 32
    "elf32",
#elif ELF_MODE == 64
    "elf64",
#endif
    RTLDRELF_NAME(Close),
    NULL, /* Get Symbol */
    RTLDRELF_NAME(Done),
    RTLDRELF_NAME(EnumSymbols),
    /* ext: */
    RTLDRELF_NAME(GetImageSize),
    RTLDRELF_NAME(GetBits),
    RTLDRELF_NAME(Relocate),
    RTLDRELF_NAME(GetSymbolEx),
    NULL /*pfnQueryForwarderInfo*/,
    RTLDRELF_NAME(EnumDbgInfo),
    RTLDRELF_NAME(EnumSegments),
    RTLDRELF_NAME(LinkAddressToSegOffset),
    RTLDRELF_NAME(LinkAddressToRva),
    RTLDRELF_NAME(SegOffsetToRva),
    RTLDRELF_NAME(RvaToSegOffset),
    RTLDRELF_NAME(ReadDbgInfo),
    NULL /*pfnQueryProp*/,
    NULL /*pfnVerifySignature*/,
    NULL /*pfnHashImage*/,
    42
};



/**
 * Validates the ELF header.
 *
 * @returns iprt status code.
 * @param   pEhdr       Pointer to the ELF header.
 * @param   pszLogName  The log name.
 * @param   cbRawImage  The size of the raw image.
 */
static int RTLDRELF_NAME(ValidateElfHeader)(const Elf_Ehdr *pEhdr, const char *pszLogName, uint64_t cbRawImage,
                                            PRTLDRARCH penmArch)
{
    Log3(("RTLdrELF:     e_ident: %.*Rhxs\n"
          "RTLdrELF:      e_type: " FMT_ELF_HALF "\n"
          "RTLdrELF:   e_version: " FMT_ELF_HALF "\n"
          "RTLdrELF:     e_entry: " FMT_ELF_ADDR "\n"
          "RTLdrELF:     e_phoff: " FMT_ELF_OFF  "\n"
          "RTLdrELF:     e_shoff: " FMT_ELF_OFF  "\n"
          "RTLdrELF:     e_flags: " FMT_ELF_WORD "\n"
          "RTLdrELF:    e_ehsize: " FMT_ELF_HALF "\n"
          "RTLdrELF: e_phentsize: " FMT_ELF_HALF "\n"
          "RTLdrELF:     e_phnum: " FMT_ELF_HALF "\n"
          "RTLdrELF: e_shentsize: " FMT_ELF_HALF "\n"
          "RTLdrELF:     e_shnum: " FMT_ELF_HALF "\n"
          "RTLdrELF:  e_shstrndx: " FMT_ELF_HALF "\n",
          RT_ELEMENTS(pEhdr->e_ident), &pEhdr->e_ident[0], pEhdr->e_type, pEhdr->e_version,
          pEhdr->e_entry, pEhdr->e_phoff, pEhdr->e_shoff,pEhdr->e_flags, pEhdr->e_ehsize, pEhdr->e_phentsize,
          pEhdr->e_phnum, pEhdr->e_shentsize, pEhdr->e_shnum, pEhdr->e_shstrndx));

    if (    pEhdr->e_ident[EI_MAG0] != ELFMAG0
        ||  pEhdr->e_ident[EI_MAG1] != ELFMAG1
        ||  pEhdr->e_ident[EI_MAG2] != ELFMAG2
        ||  pEhdr->e_ident[EI_MAG3] != ELFMAG3
       )
    {
        Log(("RTLdrELF: %s: Invalid ELF magic (%.*Rhxs)\n", pszLogName, sizeof(pEhdr->e_ident), pEhdr->e_ident)); NOREF(pszLogName);
        return VERR_BAD_EXE_FORMAT;
    }
    if (pEhdr->e_ident[EI_CLASS] != RTLDRELF_SUFF(ELFCLASS))
    {
        Log(("RTLdrELF: %s: Invalid ELF class (%.*Rhxs)\n", pszLogName, sizeof(pEhdr->e_ident), pEhdr->e_ident));
        return VERR_BAD_EXE_FORMAT;
    }
    if (pEhdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        Log(("RTLdrELF: %s: ELF endian %x is unsupported\n", pszLogName, pEhdr->e_ident[EI_DATA]));
        return VERR_LDRELF_ODD_ENDIAN;
    }
    if (pEhdr->e_version != EV_CURRENT)
    {
        Log(("RTLdrELF: %s: ELF version %x is unsupported\n", pszLogName, pEhdr->e_version));
        return VERR_LDRELF_VERSION;
    }

    if (sizeof(Elf_Ehdr) != pEhdr->e_ehsize)
    {
        Log(("RTLdrELF: %s: Elf header e_ehsize is %d expected %d!\n",
             pszLogName, pEhdr->e_ehsize, sizeof(Elf_Ehdr)));
        return VERR_BAD_EXE_FORMAT;
    }
    if (    sizeof(Elf_Phdr) != pEhdr->e_phentsize
        &&  (    pEhdr->e_phnum != 0
             ||  pEhdr->e_type == ET_DYN))
    {
        Log(("RTLdrELF: %s: Elf header e_phentsize is %d expected %d!\n",
             pszLogName, pEhdr->e_phentsize, sizeof(Elf_Phdr)));
        return VERR_BAD_EXE_FORMAT;
    }
    if (sizeof(Elf_Shdr) != pEhdr->e_shentsize)
    {
        Log(("RTLdrELF: %s: Elf header e_shentsize is %d expected %d!\n",
             pszLogName, pEhdr->e_shentsize, sizeof(Elf_Shdr)));
        return VERR_BAD_EXE_FORMAT;
    }

    switch (pEhdr->e_type)
    {
        case ET_REL:
        case ET_EXEC:
        case ET_DYN:
            break;
        default:
            Log(("RTLdrELF: %s: image type %#x is not supported!\n", pszLogName, pEhdr->e_type));
            return VERR_BAD_EXE_FORMAT;
    }

    switch (pEhdr->e_machine)
    {
#if   ELF_MODE == 32
        case EM_386:
        case EM_486:
            *penmArch = RTLDRARCH_X86_32;
            break;
#elif ELF_MODE == 64
        case EM_X86_64:
            *penmArch = RTLDRARCH_AMD64;
            break;
#endif
        default:
            Log(("RTLdrELF: %s: machine type %u is not supported!\n", pszLogName, pEhdr->e_machine));
            return VERR_LDRELF_MACHINE;
    }

    if (    pEhdr->e_phoff < pEhdr->e_ehsize
        &&  !(pEhdr->e_phoff && pEhdr->e_phnum)
        &&  pEhdr->e_phnum)
    {
        Log(("RTLdrELF: %s: The program headers overlap with the ELF header! e_phoff=" FMT_ELF_OFF "\n",
             pszLogName, pEhdr->e_phoff));
        return VERR_BAD_EXE_FORMAT;
    }
    if (    pEhdr->e_phoff + pEhdr->e_phnum * pEhdr->e_phentsize > cbRawImage
        ||  pEhdr->e_phoff + pEhdr->e_phnum * pEhdr->e_phentsize < pEhdr->e_phoff)
    {
        Log(("RTLdrELF: %s: The program headers extends beyond the file! e_phoff=" FMT_ELF_OFF " e_phnum=" FMT_ELF_HALF "\n",
             pszLogName, pEhdr->e_phoff, pEhdr->e_phnum));
        return VERR_BAD_EXE_FORMAT;
    }


    if (    pEhdr->e_shoff < pEhdr->e_ehsize
        &&  !(pEhdr->e_shoff && pEhdr->e_shnum))
    {
        Log(("RTLdrELF: %s: The section headers overlap with the ELF header! e_shoff=" FMT_ELF_OFF "\n",
             pszLogName, pEhdr->e_shoff));
        return VERR_BAD_EXE_FORMAT;
    }
    if (    pEhdr->e_shoff + pEhdr->e_shnum * pEhdr->e_shentsize > cbRawImage
        ||  pEhdr->e_shoff + pEhdr->e_shnum * pEhdr->e_shentsize < pEhdr->e_shoff)
    {
        Log(("RTLdrELF: %s: The section headers extends beyond the file! e_shoff=" FMT_ELF_OFF " e_shnum=" FMT_ELF_HALF "\n",
             pszLogName, pEhdr->e_shoff, pEhdr->e_shnum));
        return VERR_BAD_EXE_FORMAT;
    }

    if (pEhdr->e_shstrndx == 0 || pEhdr->e_shstrndx > pEhdr->e_shnum)
    {
        Log(("RTLdrELF: %s: The section headers string table is out of bounds! e_shstrndx=" FMT_ELF_HALF " e_shnum=" FMT_ELF_HALF "\n",
             pszLogName, pEhdr->e_shstrndx, pEhdr->e_shnum));
        return VERR_BAD_EXE_FORMAT;
    }

    return VINF_SUCCESS;
}

/**
 * Gets the section header name.
 *
 * @returns pszName.
 * @param   pEhdr           The elf header.
 * @param   offName         The offset of the section header name.
 * @param   pszName         Where to store the name.
 * @param   cbName          The size of the buffer pointed to by pszName.
 */
const char *RTLDRELF_NAME(GetSHdrName)(PRTLDRMODELF pModElf, Elf_Word offName, char *pszName, size_t cbName)
{
    RTFOFF off = pModElf->paShdrs[pModElf->Ehdr.e_shstrndx].sh_offset + offName;
    int rc = pModElf->Core.pReader->pfnRead(pModElf->Core.pReader, pszName, cbName - 1, off);
    if (RT_FAILURE(rc))
    {
        /* read by for byte. */
        for (unsigned i = 0; i < cbName; i++, off++)
        {
            rc = pModElf->Core.pReader->pfnRead(pModElf->Core.pReader, pszName + i, 1, off);
            if (RT_FAILURE(rc))
            {
                pszName[i] = '\0';
                break;
            }
        }
    }

    pszName[cbName - 1] = '\0';
    return pszName;
}


/**
 * Validates a section header.
 *
 * @returns iprt status code.
 * @param   pModElf     Pointer to the module structure.
 * @param   iShdr       The index of section header which should be validated.
 *                      The section headers are found in the pModElf->paShdrs array.
 * @param   pszLogName  The log name.
 * @param   cbRawImage  The size of the raw image.
 */
static int RTLDRELF_NAME(ValidateSectionHeader)(PRTLDRMODELF pModElf, unsigned iShdr, const char *pszLogName, RTFOFF cbRawImage)
{
    const Elf_Shdr *pShdr = &pModElf->paShdrs[iShdr];
    char szSectionName[80]; NOREF(szSectionName);
    Log3(("RTLdrELF: Section Header #%d:\n"
          "RTLdrELF:      sh_name: " FMT_ELF_WORD " - %s\n"
          "RTLdrELF:      sh_type: " FMT_ELF_WORD " (%s)\n"
          "RTLdrELF:     sh_flags: " FMT_ELF_XWORD "\n"
          "RTLdrELF:      sh_addr: " FMT_ELF_ADDR "\n"
          "RTLdrELF:    sh_offset: " FMT_ELF_OFF "\n"
          "RTLdrELF:      sh_size: " FMT_ELF_XWORD "\n"
          "RTLdrELF:      sh_link: " FMT_ELF_WORD "\n"
          "RTLdrELF:      sh_info: " FMT_ELF_WORD "\n"
          "RTLdrELF: sh_addralign: " FMT_ELF_XWORD "\n"
          "RTLdrELF:   sh_entsize: " FMT_ELF_XWORD "\n",
          iShdr,
          pShdr->sh_name, RTLDRELF_NAME(GetSHdrName)(pModElf, pShdr->sh_name, szSectionName, sizeof(szSectionName)),
          pShdr->sh_type, rtldrElfGetShdrType(pShdr->sh_type), pShdr->sh_flags, pShdr->sh_addr,
          pShdr->sh_offset, pShdr->sh_size, pShdr->sh_link, pShdr->sh_info, pShdr->sh_addralign,
          pShdr->sh_entsize));

    if (iShdr == 0)
    {
        if (   pShdr->sh_name       != 0
            || pShdr->sh_type       != SHT_NULL
            || pShdr->sh_flags      != 0
            || pShdr->sh_addr       != 0
            || pShdr->sh_size       != 0
            || pShdr->sh_offset     != 0
            || pShdr->sh_link       != SHN_UNDEF
            || pShdr->sh_addralign  != 0
            || pShdr->sh_entsize    != 0 )
        {
            Log(("RTLdrELF: %s: Bad #0 section: %.*Rhxs\n", pszLogName, sizeof(*pShdr), pShdr ));
            return VERR_BAD_EXE_FORMAT;
        }
        return VINF_SUCCESS;
    }

    if (pShdr->sh_name >= pModElf->cbShStr)
    {
        Log(("RTLdrELF: %s: Shdr #%d: sh_name (%d) is beyond the end of the section header string table (%d)!\n",
             pszLogName, iShdr, pShdr->sh_name, pModElf->cbShStr)); NOREF(pszLogName);
        return VERR_BAD_EXE_FORMAT;
    }

    if (pShdr->sh_link >= pModElf->Ehdr.e_shnum)
    {
        Log(("RTLdrELF: %s: Shdr #%d: sh_link (%d) is beyond the end of the section table (%d)!\n",
             pszLogName, iShdr, pShdr->sh_link, pModElf->Ehdr.e_shnum)); NOREF(pszLogName);
        return VERR_BAD_EXE_FORMAT;
    }

    switch (pShdr->sh_type)
    {
        /** @todo find specs and check up which sh_info fields indicates section table entries */
        case 12301230:
            if (pShdr->sh_info >= pModElf->Ehdr.e_shnum)
            {
                Log(("RTLdrELF: %s: Shdr #%d: sh_info (%d) is beyond the end of the section table (%d)!\n",
                     pszLogName, iShdr, pShdr->sh_link, pModElf->Ehdr.e_shnum));
                return VERR_BAD_EXE_FORMAT;
            }
            break;

        case SHT_NULL:
            break;
        case SHT_PROGBITS:
        case SHT_SYMTAB:
        case SHT_STRTAB:
        case SHT_RELA:
        case SHT_HASH:
        case SHT_DYNAMIC:
        case SHT_NOTE:
        case SHT_NOBITS:
        case SHT_REL:
        case SHT_SHLIB:
        case SHT_DYNSYM:
            /*
             * For these types sh_info doesn't have any special meaning, or anything which
             * we need/can validate now.
             */
            break;


        default:
            Log(("RTLdrELF: %s: Warning, unknown type %d!\n", pszLogName, pShdr->sh_type));
            break;
    }

    if (    pShdr->sh_type != SHT_NOBITS
        &&  pShdr->sh_size)
    {
        RTFOFF offEnd = pShdr->sh_offset + pShdr->sh_size;
        if (    offEnd > cbRawImage
            ||  offEnd < (RTFOFF)pShdr->sh_offset)
        {
            Log(("RTLdrELF: %s: Shdr #%d: sh_offset (" FMT_ELF_OFF ") + sh_size (" FMT_ELF_XWORD " = %RTfoff) is beyond the end of the file (%RTfoff)!\n",
                 pszLogName, iShdr, pShdr->sh_offset, pShdr->sh_size, offEnd, cbRawImage));
            return VERR_BAD_EXE_FORMAT;
        }
        if (pShdr->sh_offset < sizeof(Elf_Ehdr))
        {
            Log(("RTLdrELF: %s: Shdr #%d: sh_offset (" FMT_ELF_OFF ") + sh_size (" FMT_ELF_XWORD ") is starting in the ELF header!\n",
                 pszLogName, iShdr, pShdr->sh_offset, pShdr->sh_size));
            return VERR_BAD_EXE_FORMAT;
        }
    }

    return VINF_SUCCESS;
}



/**
 * Opens an ELF image, fixed bitness.
 *
 * @returns iprt status code.
 * @param   pReader     The loader reader instance which will provide the raw image bits.
 * @param   fFlags      Reserved, MBZ.
 * @param   enmArch     Architecture specifier.
 * @param   phLdrMod    Where to store the handle.
 */
static int RTLDRELF_NAME(Open)(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, PRTLDRMOD phLdrMod)
{
    const char *pszLogName = pReader->pfnLogName(pReader);
    RTFOFF      cbRawImage = pReader->pfnSize(pReader);
    RT_NOREF_PV(fFlags);

    /*
     * Create the loader module instance.
     */
    PRTLDRMODELF pModElf = (PRTLDRMODELF)RTMemAllocZ(sizeof(*pModElf));
    if (!pModElf)
        return VERR_NO_MEMORY;

    pModElf->Core.u32Magic  = RTLDRMOD_MAGIC;
    pModElf->Core.eState    = LDR_STATE_INVALID;
    pModElf->Core.pReader   = pReader;
    pModElf->Core.enmFormat = RTLDRFMT_ELF;
    pModElf->Core.enmType   = RTLDRTYPE_OBJECT;
    pModElf->Core.enmEndian = RTLDRENDIAN_LITTLE;
#if ELF_MODE == 32
    pModElf->Core.enmArch   = RTLDRARCH_X86_32;
#else
    pModElf->Core.enmArch   = RTLDRARCH_AMD64;
#endif
    //pModElf->pvBits         = NULL;
    //pModElf->Ehdr           = {0};
    //pModElf->paShdrs        = NULL;
    //pModElf->paSyms         = NULL;
    pModElf->iSymSh         = ~0U;
    //pModElf->cSyms          = 0;
    pModElf->iStrSh         = ~0U;
    //pModElf->cbStr          = 0;
    //pModElf->cbImage        = 0;
    //pModElf->LinkAddress    = 0;
    //pModElf->pStr           = NULL;
    //pModElf->cbShStr        = 0;
    //pModElf->pShStr         = NULL;

    /*
     * Read and validate the ELF header and match up the CPU architecture.
     */
    int rc = pReader->pfnRead(pReader, &pModElf->Ehdr, sizeof(pModElf->Ehdr), 0);
    if (RT_SUCCESS(rc))
    {
        RTLDRARCH enmArchImage = RTLDRARCH_INVALID; /* shut up gcc */
        rc = RTLDRELF_NAME(ValidateElfHeader)(&pModElf->Ehdr, pszLogName, cbRawImage, &enmArchImage);
        if (RT_SUCCESS(rc))
        {
            if (    enmArch != RTLDRARCH_WHATEVER
                &&  enmArch != enmArchImage)
                rc = VERR_LDR_ARCH_MISMATCH;
        }
    }
    if (RT_SUCCESS(rc))
    {
        /*
         * Read the section headers, keeping a prestine copy for the module
         * introspection methods.
         */
        size_t const cbShdrs = pModElf->Ehdr.e_shnum * sizeof(Elf_Shdr);
        Elf_Shdr *paShdrs = (Elf_Shdr *)RTMemAlloc(cbShdrs * 2);
        if (paShdrs)
        {
            pModElf->paShdrs = paShdrs;
            rc = pReader->pfnRead(pReader, paShdrs, cbShdrs, pModElf->Ehdr.e_shoff);
            if (RT_SUCCESS(rc))
            {
                memcpy(&paShdrs[pModElf->Ehdr.e_shnum], paShdrs, cbShdrs);
                pModElf->paOrgShdrs = &paShdrs[pModElf->Ehdr.e_shnum];

                pModElf->cbShStr = paShdrs[pModElf->Ehdr.e_shstrndx].sh_size;

                /*
                 * Validate the section headers and find relevant sections.
                 */
                Elf_Addr uNextAddr = 0;
                for (unsigned i = 0; i < pModElf->Ehdr.e_shnum; i++)
                {
                    rc = RTLDRELF_NAME(ValidateSectionHeader)(pModElf, i, pszLogName, cbRawImage);
                    if (RT_FAILURE(rc))
                        break;

                    /* We're looking for symbol tables. */
                    if (paShdrs[i].sh_type == SHT_SYMTAB)
                    {
                        if (pModElf->iSymSh != ~0U)
                        {
                            Log(("RTLdrElf: %s: Multiple symbol tabs! iSymSh=%d i=%d\n", pszLogName, pModElf->iSymSh, i));
                            rc = VERR_LDRELF_MULTIPLE_SYMTABS;
                            break;
                        }
                        pModElf->iSymSh = i;
                        pModElf->cSyms  = (unsigned)(paShdrs[i].sh_size / sizeof(Elf_Sym));
                        AssertReturn(pModElf->cSyms == paShdrs[i].sh_size / sizeof(Elf_Sym), VERR_IMAGE_TOO_BIG);
                        pModElf->iStrSh = paShdrs[i].sh_link;
                        pModElf->cbStr  = (unsigned)paShdrs[pModElf->iStrSh].sh_size;
                        AssertReturn(pModElf->cbStr == paShdrs[pModElf->iStrSh].sh_size, VERR_IMAGE_TOO_BIG);
                    }

                    /* Special checks for the section string table. */
                    if (i == pModElf->Ehdr.e_shstrndx)
                    {
                        if (paShdrs[i].sh_type != SHT_STRTAB)
                        {
                            Log(("RTLdrElf: Section header string table is not a SHT_STRTAB: %#x\n", paShdrs[i].sh_type));
                            rc = VERR_BAD_EXE_FORMAT;
                            break;
                        }
                        if (paShdrs[i].sh_size == 0)
                        {
                            Log(("RTLdrElf: Section header string table is empty\n"));
                            rc = VERR_BAD_EXE_FORMAT;
                            break;
                        }
                    }

                    /* Kluge for the .data..percpu segment in 64-bit linux kernels. */
                    if (paShdrs[i].sh_flags & SHF_ALLOC)
                    {
                        if (   paShdrs[i].sh_addr == 0
                            && paShdrs[i].sh_addr < uNextAddr)
                        {
                            Elf_Addr uAddr = RT_ALIGN_T(uNextAddr, paShdrs[i].sh_addralign, Elf_Addr);
                            Log(("RTLdrElf: Out of order section #%d; adjusting sh_addr from " FMT_ELF_ADDR " to " FMT_ELF_ADDR "\n",
                                 i, paShdrs[i].sh_addr, uAddr));
                            paShdrs[i].sh_addr = uAddr;
                        }
                        uNextAddr = paShdrs[i].sh_addr + paShdrs[i].sh_size;
                    }
                } /* for each section header */

                /*
                 * Calculate the image base address if the image isn't relocatable.
                 */
                if (RT_SUCCESS(rc) && pModElf->Ehdr.e_type != ET_REL)
                {
                    pModElf->LinkAddress = ~(Elf_Addr)0;
                    for (unsigned i = 0; i < pModElf->Ehdr.e_shnum; i++)
                        if (   (paShdrs[i].sh_flags & SHF_ALLOC)
                            && paShdrs[i].sh_addr < pModElf->LinkAddress)
                            pModElf->LinkAddress = paShdrs[i].sh_addr;
                    if (pModElf->LinkAddress == ~(Elf_Addr)0)
                    {
                        AssertFailed();
                        rc = VERR_LDR_GENERAL_FAILURE;
                    }
                }

                /*
                 * Perform allocations / RVA calculations, determine the image size.
                 */
                if (RT_SUCCESS(rc))
                    for (unsigned i = 0; i < pModElf->Ehdr.e_shnum; i++)
                        if (paShdrs[i].sh_flags & SHF_ALLOC)
                        {
                            if (pModElf->Ehdr.e_type == ET_REL)
                                paShdrs[i].sh_addr = paShdrs[i].sh_addralign
                                                   ? RT_ALIGN_T(pModElf->cbImage, paShdrs[i].sh_addralign, Elf_Addr)
                                                   : (Elf_Addr)pModElf->cbImage;
                            else
                                paShdrs[i].sh_addr -= pModElf->LinkAddress;
                            Elf_Addr EndAddr = paShdrs[i].sh_addr + paShdrs[i].sh_size;
                            if (pModElf->cbImage < EndAddr)
                            {
                                pModElf->cbImage = (size_t)EndAddr;
                                AssertMsgReturn(pModElf->cbImage == EndAddr, (FMT_ELF_ADDR "\n", EndAddr), VERR_IMAGE_TOO_BIG);
                            }
                            Log2(("RTLdrElf: %s: Assigned " FMT_ELF_ADDR " to section #%d\n", pszLogName, paShdrs[i].sh_addr, i));
                        }

                Log2(("RTLdrElf: iSymSh=%u cSyms=%u iStrSh=%u cbStr=%u rc=%Rrc cbImage=%#zx LinkAddress=" FMT_ELF_ADDR "\n",
                      pModElf->iSymSh, pModElf->cSyms, pModElf->iStrSh, pModElf->cbStr, rc,
                      pModElf->cbImage, pModElf->LinkAddress));
                if (RT_SUCCESS(rc))
                {
                    pModElf->Core.pOps      = &RTLDRELF_MID(s_rtldrElf,Ops);
                    pModElf->Core.eState    = LDR_STATE_OPENED;
                    *phLdrMod = &pModElf->Core;

                    LogFlow(("%s: %s: returns VINF_SUCCESS *phLdrMod=%p\n", __FUNCTION__, pszLogName, *phLdrMod));
                    return VINF_SUCCESS;
                }
            }

            RTMemFree(paShdrs);
        }
        else
            rc = VERR_NO_MEMORY;
    }

    RTMemFree(pModElf);
    LogFlow(("%s: returns %Rrc\n", __FUNCTION__, rc));
    return rc;
}




/*******************************************************************************
*   Cleanup Constants And Macros                                               *
*******************************************************************************/
#undef RTLDRELF_NAME
#undef RTLDRELF_SUFF
#undef RTLDRELF_MID

#undef FMT_ELF_ADDR
#undef FMT_ELF_HALF
#undef FMT_ELF_SHALF
#undef FMT_ELF_OFF
#undef FMT_ELF_SIZE
#undef FMT_ELF_SWORD
#undef FMT_ELF_WORD
#undef FMT_ELF_XWORD
#undef FMT_ELF_SXWORD

#undef Elf_Ehdr
#undef Elf_Phdr
#undef Elf_Shdr
#undef Elf_Sym
#undef Elf_Rel
#undef Elf_Rela
#undef Elf_Reloc
#undef Elf_Nhdr
#undef Elf_Dyn

#undef Elf_Addr
#undef Elf_Half
#undef Elf_Off
#undef Elf_Size
#undef Elf_Sword
#undef Elf_Word

#undef RTLDRMODELF
#undef PRTLDRMODELF

#undef ELF_R_SYM
#undef ELF_R_TYPE
#undef ELF_R_INFO

#undef ELF_ST_BIND


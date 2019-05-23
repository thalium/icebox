/* $Id: kLdrMod.c 81 2016-08-18 22:10:38Z bird $ */
/** @file
 * kLdr - The Module Interpreter.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kLdr.h>
#include "kLdrInternal.h"
#include <k/kCpu.h>
#include <k/kLdrFmts/mz.h>
#if 1 /* testing headers */
# include <k/kLdrFmts/pe.h>
# include <k/kLdrFmts/lx.h>
# include <k/kLdrFmts/mach-o.h>
#endif


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def KLDRMOD_STRICT
 * Define KLDRMOD_STRICT to enabled strict checks in KLDRMOD. */
#define KLDRMOD_STRICT 1

/** @def KLDRMOD_ASSERT
 * Assert that an expression is true when KLDR_STRICT is defined.
 */
#ifdef KLDRMOD_STRICT
# define KLDRMOD_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRMOD_ASSERT(expr)  do {} while (0)
#endif

/** Return / crash validation of a module argument. */
#define KLDRMOD_VALIDATE_EX(pMod, rc) \
    do  { \
        if (    (pMod)->u32Magic != KLDRMOD_MAGIC \
            ||  (pMod)->pOps == NULL \
           )\
        { \
            return (rc); \
        } \
    } while (0)

/** Return / crash validation of a module argument. */
#define KLDRMOD_VALIDATE(pMod) \
    KLDRMOD_VALIDATE_EX(pMod, KERR_INVALID_PARAMETER)

/** Return / crash validation of a module argument. */
#define KLDRMOD_VALIDATE_VOID(pMod) \
    do  { \
        if (    (pMod)->u32Magic != KLDRMOD_MAGIC \
            ||  (pMod)->pOps == NULL \
           )\
        { \
            return; \
        } \
    } while (0)


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The list of module interpreters. */
static PCKLDRMODOPS g_pModInterpreterHead = NULL;



/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/



/**
 * Open a executable image by file name.
 *
 * @returns 0 on success and *ppMod pointing to a module instance.
 *          On failure, a non-zero OS specific error code is returned.
 * @param   pszFilename     The filename to open.
 * @param   fFlags          Flags, MBZ.
 * @param   enmCpuArch      The desired CPU architecture. KCPUARCH_UNKNOWN means
 *                          anything goes, but with a preference for the current
 *                          host architecture.
 * @param   ppMod           Where to store the module handle.
 */
int kLdrModOpen(const char *pszFilename, KU32 fFlags, KCPUARCH enmCpuArch, PPKLDRMOD ppMod)
{
    /*
     * Open the file using a bit provider.
     */
    PKRDR pRdr;
    int rc = kRdrOpen(&pRdr, pszFilename);
    if (!rc)
    {
        rc = kLdrModOpenFromRdr(pRdr, fFlags, enmCpuArch, ppMod);
        if (!rc)
            return 0;
       kRdrClose(pRdr);
    }
    return rc;
}


/**
 * Select image from the FAT according to the enmCpuArch and fFlag.
 *
 * @returns 0 on success and *poffHdr set to the image header.
 *          On failure, a non-zero error code is returned.
 *
 * @param   pRdr            The file provider instance to use.
 * @param   fFlags          Flags, MBZ.
 * @param   enmCpuArch      The desired CPU architecture. KCPUARCH_UNKNOWN means
 *                          anything goes, but with a preference for the current
 *                          host architecture.
 * @param   u32Magic        The FAT magic.
 * @param   poffHdr         Where to store the offset of the selected image.
 */
static int kldrModOpenFromRdrSelectImageFromFAT(PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch, KU32 u32Magic, KLDRFOFF *poffHdr)
{
    int         rcRet = KLDR_ERR_CPU_ARCH_MISMATCH;
    KLDRFOFF    off = *poffHdr + sizeof(KU32);
    KLDRFOFF    offEndFAT;
    KBOOL       fCpuArchWhatever;
    KU32        cArchs;
    KU32        iArch;
    int         rc;
    K_NOREF(fFlags);

    /* Read fat_header_t::nfat_arch. */
    rc = kRdrRead(pRdr, &cArchs, sizeof(cArchs), off);
    if (rc)
        return rc;
    off += sizeof(KU32);
    if (u32Magic == IMAGE_FAT_SIGNATURE_OE)
        cArchs = K_E2E_U32(cArchs);
    if (cArchs == 0)
        return KLDR_ERR_FAT_INVALID;

    /* Deal with KCPUARCH_UNKNOWN. */
    fCpuArchWhatever = enmCpuArch == KCPUARCH_UNKNOWN;
    if (fCpuArchWhatever)
    {
        KCPU enmCpuIgnored;
        kCpuGetArchAndCpu(&enmCpuArch, &enmCpuIgnored);
    }

    /*
     * Iterate the architecture list.
     */
    offEndFAT = off + cArchs * sizeof(fat_arch_t);
    for (iArch = 0; iArch < cArchs; iArch++)
    {
        KCPUARCH    enmEntryArch;
        fat_arch_t  Arch;
        rc = kRdrRead(pRdr, &Arch, sizeof(Arch), off);
        if (rc)
            return rc;
        off += sizeof(Arch);

        if (u32Magic == IMAGE_FAT_SIGNATURE_OE)
        {
            Arch.cputype    = K_E2E_U32(Arch.cputype);
            Arch.cpusubtype = K_E2E_U32(Arch.cpusubtype);
            Arch.offset     = K_E2E_U32(Arch.offset);
            Arch.size       = K_E2E_U32(Arch.size);
            Arch.align      = K_E2E_U32(Arch.align);
        }

        /* Simple validation. */
        if (    (KLDRFOFF)Arch.offset < offEndFAT
            ||  (KLDRFOFF)Arch.offset >= kRdrSize(pRdr)
            ||  Arch.align >= 32
            ||  Arch.offset & ((KU32_C(1) << Arch.align) - KU32_C(1)))
            return KLDR_ERR_FAT_INVALID;

        /* deal with the cputype and cpusubtype. (See similar code in kLdrModMachO.c.) */
        switch (Arch.cputype)
        {
            case CPU_TYPE_X86:
                enmEntryArch = KCPUARCH_X86_32;
                switch (Arch.cpusubtype)
                {
                    case CPU_SUBTYPE_I386_ALL:
                    /*case CPU_SUBTYPE_386: ^^ ;*/
                    case CPU_SUBTYPE_486:
                    case CPU_SUBTYPE_486SX:
                    /*case CPU_SUBTYPE_586: vv */
                    case CPU_SUBTYPE_PENT:
                    case CPU_SUBTYPE_PENTPRO:
                    case CPU_SUBTYPE_PENTII_M3:
                    case CPU_SUBTYPE_PENTII_M5:
                    case CPU_SUBTYPE_CELERON:
                    case CPU_SUBTYPE_CELERON_MOBILE:
                    case CPU_SUBTYPE_PENTIUM_3:
                    case CPU_SUBTYPE_PENTIUM_3_M:
                    case CPU_SUBTYPE_PENTIUM_3_XEON:
                    case CPU_SUBTYPE_PENTIUM_M:
                    case CPU_SUBTYPE_PENTIUM_4:
                    case CPU_SUBTYPE_PENTIUM_4_M:
                    case CPU_SUBTYPE_XEON:
                    case CPU_SUBTYPE_XEON_MP:
                        break;
                    default:
                        return KLDR_ERR_FAT_UNSUPPORTED_CPU_SUBTYPE;
                }
                break;

            case CPU_TYPE_X86_64:
                enmEntryArch = KCPUARCH_AMD64;
                switch (Arch.cpusubtype & ~CPU_SUBTYPE_MASK)
                {
                    case CPU_SUBTYPE_X86_64_ALL:
                        break;
                    default:
                        return KLDR_ERR_FAT_UNSUPPORTED_CPU_SUBTYPE;
                }
                break;

            default:
                enmEntryArch = KCPUARCH_UNKNOWN;
                break;
        }

        /*
         * Finally the actual image selecting.
         *
         * Return immediately on a perfect match. Otherwise continue looking,
         * if we're none too picky, remember the first image in case we don't
         * get lucky.
         */
        if (enmEntryArch == enmCpuArch)
        {
            *poffHdr = Arch.offset;
            return 0;
        }

        if (    fCpuArchWhatever
            &&  rcRet == KLDR_ERR_CPU_ARCH_MISMATCH)
        {
            *poffHdr = Arch.offset;
            rcRet = 0;
        }
    }

    return rcRet;
}


/**
 * Open a executable image from a file provider instance.
 *
 * @returns 0 on success and *ppMod pointing to a module instance.
 *          On failure, a non-zero OS specific error code is returned.
 * @param   pRdr            The file provider instance to use.
 *                          On success, the ownership of the instance is taken by the
 *                          module and the caller must not ever touch it again.
 *                          (The instance is not closed on failure, the call has to do that.)
 * @param   fFlags          Flags, MBZ.
 * @param   enmCpuArch      The desired CPU architecture. KCPUARCH_UNKNOWN means
 *                          anything goes, but with a preference for the current
 *                          host architecture.
 * @param   ppMod           Where to store the module handle.
 */
int kLdrModOpenFromRdr(PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch, PPKLDRMOD ppMod)
{
    union
    {
        KU32        u32;
        KU16        u16;
        KU16        au16[2];
        KU8         au8[4];
    }           u;
    KLDRFOFF    offHdr = 0;
    int         rc;

    kHlpAssertReturn(!(fFlags & ~KLDRMOD_OPEN_FLAGS_VALID_MASK), KERR_INVALID_PARAMETER);

    for (;;)
    {
        /*
         * Try figure out what kind of image this is.
         * Always read the 'new header' if we encounter MZ.
         */
        rc = kRdrRead(pRdr, &u, sizeof(u), offHdr);
        if (rc)
            return rc;
        if (    u.u16 == IMAGE_DOS_SIGNATURE
            &&  kRdrSize(pRdr) > (KFOFF)sizeof(IMAGE_DOS_HEADER))
        {
            rc = kRdrRead(pRdr, &u, sizeof(u.u32), K_OFFSETOF(IMAGE_DOS_HEADER, e_lfanew));
            if (rc)
                return rc;
            if ((KLDRFOFF)u.u32 < kRdrSize(pRdr))
            {
                offHdr = u.u32;
                rc = kRdrRead(pRdr, &u, sizeof(u.u32), offHdr);
                if (rc)
                    return rc;
            }
            else
                u.u16 = IMAGE_DOS_SIGNATURE;
        }

        /*
         * Handle FAT images too here (one only).
         */
        if (    (   u.u32 == IMAGE_FAT_SIGNATURE
                 || u.u32 == IMAGE_FAT_SIGNATURE_OE)
            &&  offHdr == 0)
        {
            rc = kldrModOpenFromRdrSelectImageFromFAT(pRdr, fFlags, enmCpuArch, u.u32, &offHdr);
            if (rc)
                return rc;
            if (offHdr)
                continue;
        }
        break;
    }


    /*
     * Use the magic to select the appropriate image interpreter head on.
     */
    if (u.u16 == IMAGE_DOS_SIGNATURE)
        rc = KLDR_ERR_MZ_NOT_SUPPORTED;
    else if (u.u16 == IMAGE_NE_SIGNATURE)
        rc = KLDR_ERR_NE_NOT_SUPPORTED;
    else if (u.u16 == IMAGE_LX_SIGNATURE)
        rc = g_kLdrModLXOps.pfnCreate(&g_kLdrModLXOps, pRdr, fFlags, enmCpuArch, offHdr, ppMod);
    else if (u.u16 == IMAGE_LE_SIGNATURE)
        rc = KLDR_ERR_LE_NOT_SUPPORTED;
    else if (u.u32 == IMAGE_NT_SIGNATURE)
        rc = g_kLdrModPEOps.pfnCreate(&g_kLdrModPEOps, pRdr, fFlags, enmCpuArch, offHdr, ppMod);
    else if (   u.u32 == IMAGE_MACHO32_SIGNATURE
             || u.u32 == IMAGE_MACHO32_SIGNATURE_OE
             || u.u32 == IMAGE_MACHO64_SIGNATURE
             || u.u32 == IMAGE_MACHO64_SIGNATURE_OE)
        rc = g_kLdrModMachOOps.pfnCreate(&g_kLdrModMachOOps, pRdr, fFlags, enmCpuArch, offHdr, ppMod);
    else if (u.u32 == IMAGE_ELF_SIGNATURE)
        rc = KLDR_ERR_ELF_NOT_SUPPORTED;
    else
        rc = KLDR_ERR_UNKNOWN_FORMAT;

    /*
     * If no head on hit, let each interpreter have a go.
     */
    if (rc)
    {
        PCKLDRMODOPS pOps;
        for (pOps = g_pModInterpreterHead; pOps; pOps = pOps->pNext)
        {
            int rc2 = pOps->pfnCreate(pOps, pRdr, fFlags, enmCpuArch, offHdr, ppMod);
            if (!rc2)
                return rc;
        }
        *ppMod = NULL;
    }
    return rc;
}


/**
 * Closes an open module.
 *
 * The caller is responsible for calling kLdrModUnmap() and kLdrFreeTLS()
 * before closing the module.
 *
 * @returns 0 on success, non-zero on failure. The module instance state
 *          is unknown on failure, it's best not to touch it.
 * @param   pMod    The module.
 */
int     kLdrModClose(PKLDRMOD pMod)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnDestroy(pMod);
}


/**
 * Queries a symbol by name or ordinal number.
 *
 * @returns 0 and *puValue and *pfKind on success.
 *          KLDR_ERR_SYMBOL_NOT_FOUND is returned if the symbol wasn't found.
 *          Other failures could stem from bad executable format failures,
 *          read failure in case pvBits isn't specified and no mapping should be used.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits() currently located at BaseAddress.
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   BaseAddress     The module base address to use when calculating the symbol value.
 *                          There are two special values that can be used:
 *                              KLDRMOD_BASEADDRESS_LINK and KLDRMOD_BASEADDRESS_MAP.
 * @param   iSymbol         The symbol ordinal. (optional)
 * @param   pchSymbol       The symbol name. (optional)
 *                          Important, this doesn't have to be a null-terminated string.
 * @param   cchSymbol       The length of the symbol name.
 * @param   pszVersion      The symbol version. NULL if not versioned.
 * @param   pfnGetForwarder The callback to use when resolving a forwarder symbol. This is optional
 *                          and if not specified KLDR_ERR_FORWARDER is returned instead.
 * @param   pvUser          The user argument for the pfnGetForwarder callback.
 * @param   puValue         Where to store the symbol value. (optional)
 * @param   pfKind          On input one of the KLDRSYMKIND_REQ_* #defines.
 *                          On output the symbol kind. (optional)
 */
int     kLdrModQuerySymbol(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 iSymbol,
                           const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                           PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind)
{
    KLDRMOD_VALIDATE(pMod);
    if (!puValue && !pfKind)
        return KERR_INVALID_PARAMETER;
    if (puValue)
        *puValue = 0;
    if (pfKind)
        K_VALIDATE_FLAGS(*pfKind, KLDRSYMKIND_REQ_SEGMENTED);
    return pMod->pOps->pfnQuerySymbol(pMod, pvBits, BaseAddress, iSymbol, pchSymbol, cchSymbol, pszVersion,
                                      pfnGetForwarder, pvUser, puValue, pfKind);
}


/**
 * Enumerate the symbols in the module.
 *
 * @returns 0 on success and non-zero a status code on failure.
 * @param   pMod            The module which symbols should be enumerated.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits() currently located at BaseAddress.
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   BaseAddress     The module base address to use when calculating the symbol values.
 *                          There are two special values that could be can:
 *                              KLDRMOD_BASEADDRESS_LINK and KLDRMOD_BASEADDRESS_MAP.
 * @param   fFlags          The enumeration flags. A combination of the KLDRMOD_ENUM_SYMS_FLAGS_* \#defines.
 * @param   pfnCallback     The enumeration callback function.
 * @param   pvUser          The user argument to the callback function.
 */
int     kLdrModEnumSymbols(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 fFlags,
                           PFNKLDRMODENUMSYMS pfnCallback, void *pvUser)
{
    KLDRMOD_VALIDATE(pMod);
    K_VALIDATE_FLAGS(fFlags, KLDRMOD_ENUM_SYMS_FLAGS_ALL);
    return pMod->pOps->pfnEnumSymbols(pMod, pvBits, BaseAddress, fFlags, pfnCallback, pvUser);
}


/**
 * Get the name of an import module by ordinal number.
 *
 * @returns 0 and name in pszName on success.
 *          On buffer overruns KERR_BUFFER_OVERFLOW will be returned.
 *          On other failures and appropriate error code is returned.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits().
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   iImport         The import module ordinal number.
 * @param   pszName         Where to store the name.
 * @param   cchName         The size of the name buffer.
 */
int     kLdrModGetImport(PKLDRMOD pMod, const void *pvBits, KU32 iImport, char *pszName, KSIZE cchName)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnGetImport(pMod, pvBits, iImport, pszName, cchName);
}


/**
 * Get the number of import modules.
 *
 * @returns The number of import modules. -1 if something really bad happens.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits().
 *                          This can be used by some module interpreters to reduce memory consumption.
 */
KI32 kLdrModNumberOfImports(PKLDRMOD pMod, const void *pvBits)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnNumberOfImports(pMod, pvBits);
}


/**
 * Checks if this module can be executed by the specified arch+cpu.
 *
 * @returns 0 if it can, KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE if it can't.
 *          Other failures may occur and cause other return values.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits().
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   enmArch         The CPU architecture.
 * @param   enmCpu          The CPU series/model.
 */
int     kLdrModCanExecuteOn(PKLDRMOD pMod, const void *pvBits, KCPUARCH enmArch, KCPU enmCpu)
{
    KLDRMOD_VALIDATE(pMod);
    if (pMod->pOps->pfnCanExecuteOn)
        return pMod->pOps->pfnCanExecuteOn(pMod, pvBits, enmArch, enmCpu);
    return kCpuCompare(pMod->enmArch, pMod->enmCpu, enmArch, enmCpu);
}


/**
 * Gets the image stack info.
 *
 * @returns 0 on success, non-zero on failure.
 * @param   pMod
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits() currently located at BaseAddress.
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   BaseAddress     The module base address to use when calculating the stack address.
 *                          There are two special values that can be used:
 *                              KLDRMOD_BASEADDRESS_LINK and KLDRMOD_BASEADDRESS_MAP.
 * @param   pStackInfo      The stack information.
 */
int     kLdrModGetStackInfo(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRSTACKINFO pStackInfo)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnGetStackInfo(pMod, pvBits, BaseAddress, pStackInfo);
}


/**
 * Queries the main entrypoint of the module.
 *
 * Only executable are supposed to have an main entrypoint, though some object and DLL
 * formats will also allow this.
 *
 * @returns 0 and *pMainEPAddress on success. Non-zero status code on failure.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits() currently located at BaseAddress.
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   BaseAddress     The module base address to use when calculating the entrypoint address.
 *                          There are two special values that can be used:
 *                              KLDRMOD_BASEADDRESS_LINK and KLDRMOD_BASEADDRESS_MAP.
 * @param   pMainEPAddress  Where to store the entry point address.
 */
int     kLdrModQueryMainEntrypoint(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRADDR pMainEPAddress)
{
    KLDRMOD_VALIDATE(pMod);
    *pMainEPAddress = 0;
    return pMod->pOps->pfnQueryMainEntrypoint(pMod, pvBits, BaseAddress, pMainEPAddress);
}


/**
 * Queries the image UUID, if the image has one.
 *
 * @returns 0 and *pvUuid. Non-zero status code on failure.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits() currently located at BaseAddress.
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   pvUuid          Where to store the UUID.
 * @param   cbUuid          Size of the UUID buffer, must be at least 16 bytes.
 */
int     kLdrModQueryImageUuid(PKLDRMOD pMod, const void *pvBits, void *pvUuid, KSIZE cbUuid)
{
    KLDRMOD_VALIDATE(pMod);
    if (cbUuid < 16)
        return KERR_INVALID_SIZE;
    if (pMod->pOps->pfnQueryImageUuid)
        return pMod->pOps->pfnQueryImageUuid(pMod, pvBits, pvUuid, cbUuid);
    return KLDR_ERR_NO_IMAGE_UUID;
}


/**
 * Queries info about a resource.
 *
 * If there are multiple resources matching the criteria, the best or
 * first match will be return.
 *
 *
 * @returns 0 on success.
 * @returns Whatever non-zero status returned by pfnCallback (enumeration was stopped).
 * @returns non-zero kLdr or native status code on failure.
 *
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits() currently located at BaseAddress.
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   BaseAddress     The module base address to use when calculating the resource addresses.
 *                          There are two special values that can be used:
 *                              KLDRMOD_BASEADDRESS_LINK and KLDRMOD_BASEADDRESS_MAP.
 * @param   idType          The resource type id to match if not NIL_KLDRMOD_RSRC_TYPE_ID.
 * @param   pszType         The resource type name to match if no NULL.
 * @param   idName          The resource name id to match if not NIL_KLDRMOD_RSRC_NAME_ID.
 * @param   pszName         The resource name to match if not NULL.
 * @param   idLang          The language id to match.
 * @param   pfnCallback     The callback function.
 * @param   pvUser          The user argument for the callback.
 */
int     kLdrModQueryResource(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 idType, const char *pszType,
                             KU32 idName, const char *pszName, KU32 idLang, PKLDRADDR pAddrRsrc, KSIZE *pcbRsrc)
{
    KLDRMOD_VALIDATE(pMod);
    if (!pAddrRsrc && !pcbRsrc)
        return KERR_INVALID_PARAMETER;
    if (pAddrRsrc)
        *pAddrRsrc = NIL_KLDRADDR;
    if (pcbRsrc)
        *pcbRsrc = 0;
    return pMod->pOps->pfnQueryResource(pMod, pvBits, BaseAddress, idType, pszType, idName, pszName, idLang, pAddrRsrc, pcbRsrc);
}


/**
 * Enumerates the resources matching the specfied criteria.
 *
 *
 * @returns 0 on success.
 * @returns Whatever non-zero status returned by pfnCallback (enumeration was stopped).
 * @returns non-zero kLdr or native status code on failure.
 *
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits() currently located at BaseAddress.
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   BaseAddress     The module base address to use when calculating the resource addresses.
 *                          There are two special values that can be used:
 *                              KLDRMOD_BASEADDRESS_LINK and KLDRMOD_BASEADDRESS_MAP.
 * @param   idType          The resource type id to match if not NIL_KLDRMOD_RSRC_TYPE_ID.
 * @param   pszType         The resource type name to match if no NULL.
 * @param   idName          The resource name id to match if not NIL_KLDRMOD_RSRC_NAME_ID.
 * @param   pszName         The resource name to match if not NULL.
 * @param   idLang          The language id to match.
 * @param   pfnCallback     The callback function.
 * @param   pvUser          The user argument for the callback.
 */
int     kLdrModEnumResources(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 idType, const char *pszType,
                             KU32 idName, const char *pszName, KU32 idLang, PFNKLDRENUMRSRC pfnCallback, void *pvUser)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnEnumResources(pMod, pvBits, BaseAddress, idType, pszType, idName, pszName, idLang, pfnCallback, pvUser);
}


/**
 * Enumerate the debug info formats contained in the executable image.
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure, or non-zero callback status.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits().
 *                          This can be used by some module interpreters to reduce memory consumption.
 * @param   pfnCallback     The callback function.
 * @param   pvUser          The user argument.
 * @see pg_kDbg for the debug info reader.
 */
int     kLdrModEnumDbgInfo(PKLDRMOD pMod, const void *pvBits, PFNKLDRENUMDBG pfnCallback, void *pvUser)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnEnumDbgInfo(pMod, pvBits, pfnCallback, pvUser);
}


/**
 * Checks if the module has debug info embedded or otherwise associated with it.
 *
 * @returns 0 if it has debug info, KLDR_ERR_NO_DEBUG_INFO if no debug info,
 *          and non-zero OS or kLdr status code on failure.
 * @param   pMod            The module.
 * @param   pvBits          Optional pointer to bits returned by kLdrModGetBits().
 *                          This can be used by some module interpreters to reduce memory consumption.
 */
int     kLdrModHasDbgInfo(PKLDRMOD pMod, const void *pvBits)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnHasDbgInfo(pMod, pvBits);
}


/**
 * May free up some resources held by the module.
 *
 * @todo define exactly what it possible to do after this call.
 *
 * @returns 0 on success, KLDR_ERR_* on failure.
 * @param   pMod    The module.
 */
int     kLdrModMostlyDone(PKLDRMOD pMod)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnMostlyDone(pMod);
}


/**
 * Maps the module into the memory of the caller.
 *
 * On success the actual addresses for the segments can be found in MapAddress
 * member of each segment in the segment array.
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module to be mapped.
 * @remark  kLdr only supports one mapping at a time of a module.
 */
int     kLdrModMap(PKLDRMOD pMod)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnMap(pMod);
}


/**
 * Unmaps a module previously mapped by kLdrModMap().
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module to unmap.
 */
int     kLdrModUnmap(PKLDRMOD pMod)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnUnmap(pMod);
}


/**
 * Reloads all dirty pages in a module previously mapped by kLdrModMap().
 *
 * The module interpreter may omit code pages if it can safely apply code
 * fixups again in a subsequent kLdrModFixupMapping() call.
 *
 * The caller is responsible for freeing TLS before calling this function.
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module.
 */
int     kLdrModReload(PKLDRMOD pMod)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnReload(pMod);
}


/**
 * Fixup the mapping made by kLdrModMap().
 *
 * The caller is only responsible for not calling this function more than
 * once without doing kLDrModReload() inbetween.
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module.
 * @param   pfnGetImport    The callback for resolving external (imported) symbols.
 * @param   pvUser          The callback user argument.
 */
int     kLdrModFixupMapping(PKLDRMOD pMod, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnFixupMapping(pMod, pfnGetImport, pvUser);
}


/**
 * Get the size of the mapped module.
 *
 * @returns The size of the mapped module (in bytes).
 * @param   pMod            The module.
 */
KLDRADDR kLdrModSize(PKLDRMOD pMod)
{
    KLDRMOD_VALIDATE_EX(pMod, 0);
    return pMod->pOps->pfnSize(pMod);
}


/**
 * Gets the module bits.
 *
 * The module interpreter will fill a mapping allocated by the caller with the
 * module bits reallocated to the specified address.
 *
 * @returns 0 on succes, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module.
 * @param   pvBits          Where to put the bits.
 * @param   BaseAddress     The base address that should correspond to the first byte in pvBits
 *                          upon return.
 * @param   pfnGetImport    The callback ufor resolving external (imported) symbols.
 * @param   pvUser          The callback user argument.
 */
int     kLdrModGetBits(PKLDRMOD pMod, void *pvBits, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnGetBits(pMod, pvBits, BaseAddress, pfnGetImport, pvUser);
}


/**
 * Relocates the module bits previously obtained by kLdrModGetBits().
 *
 * @returns 0 on succes, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module.
 * @param   pvBits          Where to put the bits.
 * @param   NewBaseAddress  The new base address.
 * @param   OldBaseAddress  The old base address (i.e. the one specified to kLdrModGetBits() or as
 *                          NewBaseAddressto the previous kLdrModRelocateBits() call).
 * @param   pfnGetImport    The callback ufor resolving external (imported) symbols.
 * @param   pvUser          The callback user argument.
 */
int     kLdrModRelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                            PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnRelocateBits(pMod, pvBits, NewBaseAddress, OldBaseAddress, pfnGetImport, pvUser);
}


/**
 * Allocates Thread Local Storage for module mapped by kLdrModMap().
 *
 * Calling kLdrModAllocTLS() more than once without calling kLdrModFreeTLS()
 * between each invocation is not supported.
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module.
 * @param   pvMapping       The external mapping address or RTLDRMOD_INT_MAP.
 */
int     kLdrModAllocTLS(PKLDRMOD pMod, void *pvMapping)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnAllocTLS(pMod, pvMapping);
}


/**
 * Frees Thread Local Storage previously allocated by kLdrModAllocTLS().
 *
 * The caller is responsible for only calling kLdrModFreeTLS() once
 * after calling kLdrModAllocTLS().
 *
 * @returns 0 on success, non-zero OS or kLdr status code on failure.
 * @param   pMod            The module.
 * @param   pvMapping       The external mapping address or RTLDRMOD_INT_MAP.
 */
void    kLdrModFreeTLS(PKLDRMOD pMod, void *pvMapping)
{
    KLDRMOD_VALIDATE_VOID(pMod);
    pMod->pOps->pfnFreeTLS(pMod, pvMapping);
}




/**
 * Call the module initializiation function of a mapped module (if any).
 *
 * @returns 0 on success or no init function, non-zero on init function failure or invalid pMod.
 * @param   pMod            The module.
 * @param   pvMapping       The external mapping address or RTLDRMOD_INT_MAP.
 * @param   uHandle         The module handle to use if any of the init functions requires the module handle.
 */
int     kLdrModCallInit(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnCallInit(pMod, pvMapping, uHandle);
}


/**
 * Call the module termination function of a mapped module (if any).
 *
 * @returns 0 on success or no term function, non-zero on invalid pMod.
 * @param   pMod            The module.
 * @param   pvMapping       The external mapping address or RTLDRMOD_INT_MAP.
 * @param   uHandle         The module handle to use if any of the term functions requires the module handle.
 *
 * @remark  Termination function failure will be ignored by the module interpreter.
 */
int     kLdrModCallTerm(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    KLDRMOD_VALIDATE(pMod);
    return pMod->pOps->pfnCallTerm(pMod, pvMapping, uHandle);
}


/**
 * Call the thread attach or detach function of a mapped module (if any).
 *
 * Any per-thread TLS initialization/termination will have to be done at this time too.
 *
 * @returns 0 on success or no attach/detach function, non-zero on attach failure or invalid pMod.
 * @param   pMod            The module.
 * @param   pvMapping       The external mapping address or RTLDRMOD_INT_MAP.
 * @param   uHandle         The module handle to use if any of the thread attach/detach functions
 *                          requires the module handle.
 *
 * @remark  Detach function failure will be ignored by the module interpreter.
 */
int     kLdrModCallThread(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle, unsigned fAttachingOrDetaching)
{
    KLDRMOD_VALIDATE(pMod);
    K_VALIDATE_FLAGS(fAttachingOrDetaching, 1);
    return pMod->pOps->pfnCallThread(pMod, pvMapping, uHandle, fAttachingOrDetaching);
}


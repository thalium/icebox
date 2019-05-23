/* $Id: kLdrModMachO.c 102 2017-10-02 10:45:31Z bird $ */
/** @file
 * kLdr - The Module Interpreter for the MACH-O format.
 */

/*
 * Copyright (c) 2006-2013 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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
#include <k/kLdrFmts/mach-o.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def KLDRMODMACHO_STRICT
 * Define KLDRMODMACHO_STRICT to enabled strict checks in KLDRMODMACHO. */
#define KLDRMODMACHO_STRICT 1

/** @def KLDRMODMACHO_ASSERT
 * Assert that an expression is true when KLDR_STRICT is defined.
 */
#ifdef KLDRMODMACHO_STRICT
# define KLDRMODMACHO_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRMODMACHO_ASSERT(expr)  do {} while (0)
#endif

/** @def KLDRMODMACHO_CHECK_RETURN
 * Checks that an expression is true and return if it isn't.
 * This is a debug aid.
 */
#ifdef KLDRMODMACHO_STRICT2
# define KLDRMODMACHO_CHECK_RETURN(expr, rc)  kHlpAssertReturn(expr, rc)
#else
# define KLDRMODMACHO_CHECK_RETURN(expr, rc)  do { if (!(expr)) { return (rc); } } while (0)
#endif

/** @def KLDRMODMACHO_CHECK_RETURN
 * Checks that an expression is true and return if it isn't.
 * This is a debug aid.
 */
#ifdef KLDRMODMACHO_STRICT2
# define KLDRMODMACHO_FAILED_RETURN(rc)  kHlpAssertFailedReturn(rc)
#else
# define KLDRMODMACHO_FAILED_RETURN(rc)  return (rc)
#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Mach-O section details.
 */
typedef struct KLDRMODMACHOSECT
{
    /** The size of the section (in bytes). */
    KLDRSIZE                cb;
    /** The link address of this section. */
    KLDRADDR                LinkAddress;
    /** The RVA of this section. */
    KLDRADDR                RVA;
    /** The file offset of this section.
     * This is -1 if the section doesn't have a file backing. */
    KLDRFOFF                offFile;
    /** The number of fixups. */
    KU32                    cFixups;
    /** The array of fixups. (lazy loaded) */
    macho_relocation_info_t *paFixups;
    /** The file offset of the fixups for this section.
     * This is -1 if the section doesn't have any fixups. */
    KLDRFOFF                offFixups;
    /** Mach-O section flags. */
    KU32                    fFlags;
    /** kLdr segment index. */
    KU32                    iSegment;
    /** Pointer to the Mach-O section structure. */
    void                   *pvMachoSection;
} KLDRMODMACHOSECT, *PKLDRMODMACHOSECT;

/**
 * Extra per-segment info.
 *
 * This is corresponds to a kLdr segment, not a Mach-O segment!
 */
typedef struct KLDRMODMACHOSEG
{
    /** The orignal segment number (in case we had to resort it). */
    KU32                    iOrgSegNo;
    /** The number of sections in the segment. */
    KU32                    cSections;
    /** Pointer to the sections belonging to this segment.
     * The array resides in the big memory chunk allocated for
     * the module handle, so it doesn't need freeing. */
    PKLDRMODMACHOSECT       paSections;

} KLDRMODMACHOSEG, *PKLDRMODMACHOSEG;

/**
 * Instance data for the Mach-O MH_OBJECT module interpreter.
 * @todo interpret the other MH_* formats.
 */
typedef struct KLDRMODMACHO
{
    /** Pointer to the module. (Follows the section table.) */
    PKLDRMOD                pMod;
    /** Pointer to the RDR file mapping of the raw file bits. NULL if not mapped. */
    const void             *pvBits;
    /** Pointer to the user mapping. */
    void                   *pvMapping;
    /** The module open flags. */
    KU32                    fOpenFlags;

    /** The offset of the image. (FAT fun.) */
    KLDRFOFF                offImage;
    /** The link address. */
    KLDRADDR                LinkAddress;
    /** The size of the mapped image. */
    KLDRADDR                cbImage;
    /** Whether we're capable of loading the image. */
    KBOOL                   fCanLoad;
    /** Whether we're creating a global offset table segment.
     * This dependes on the cputype and image type. */
    KBOOL                   fMakeGot;
    /** The size of a indirect GOT jump stub entry.
     * This is 0 if not needed. */
    KU8                     cbJmpStub;
    /** Effective file type.  If the original was a MH_OBJECT file, the
     * corresponding MH_DSYM needs the segment translation of a MH_OBJECT too.
     * The MH_DSYM normally has a separate __DWARF segment, but this is
     * automatically skipped during the transation. */
    KU8                     uEffFileType;
    /** Pointer to the load commands. (endian converted) */
    KU8                    *pbLoadCommands;
    /** The Mach-O header. (endian converted)
     * @remark The reserved field is only valid for real 64-bit headers. */
    mach_header_64_t        Hdr;

    /** The offset of the symbol table. */
    KLDRFOFF                offSymbols;
    /** The number of symbols. */
    KU32                    cSymbols;
    /** The pointer to the loaded symbol table. */
    void                   *pvaSymbols;
    /** The offset of the string table. */
    KLDRFOFF                offStrings;
    /** The size of the of the string table. */
    KU32                    cchStrings;
    /** Pointer to the loaded string table. */
    char                   *pchStrings;

    /** The image UUID, all zeros if not found. */
    KU8                     abImageUuid[16];

    /** The RVA of the Global Offset Table. */
    KLDRADDR                GotRVA;
    /** The RVA of the indirect GOT jump stubs.  */
    KLDRADDR                JmpStubsRVA;

    /** The number of sections. */
    KU32                    cSections;
    /** Pointer to the section array running in parallel to the Mach-O one. */
    PKLDRMODMACHOSECT       paSections;

    /** Array of segments parallel to the one in KLDRMOD. */
    KLDRMODMACHOSEG         aSegments[1];
} KLDRMODMACHO, *PKLDRMODMACHO;



/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
#if 0
static KI32 kldrModMachONumberOfImports(PKLDRMOD pMod, const void *pvBits);
#endif
static int kldrModMachORelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                                    PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);

static int  kldrModMachODoCreate(PKRDR pRdr, KLDRFOFF offImage, KU32 fOpenFlags, PKLDRMODMACHO *ppMod);
static int  kldrModMachOPreParseLoadCommands(KU8 *pbLoadCommands, const mach_header_32_t *pHdr, PKRDR pRdr, KLDRFOFF offImage,
                                             KU32 fOpenFlags, KU32 *pcSegments, KU32 *pcSections, KU32 *pcbStringPool,
                                             PKBOOL pfCanLoad, PKLDRADDR pLinkAddress, KU8 *puEffFileType);
static int  kldrModMachOParseLoadCommands(PKLDRMODMACHO pModMachO, char *pbStringPool, KU32 cbStringPool);
static int  kldrModMachOAdjustBaseAddress(PKLDRMODMACHO pModMachO, PKLDRADDR pBaseAddress);

/*static int  kldrModMachOLoadLoadCommands(PKLDRMODMACHO pModMachO);*/
static int  kldrModMachOLoadObjSymTab(PKLDRMODMACHO pModMachO);
static int  kldrModMachOLoadFixups(PKLDRMODMACHO pModMachO, KLDRFOFF offFixups, KU32 cFixups, macho_relocation_info_t **ppaFixups);
static int  kldrModMachOMapVirginBits(PKLDRMODMACHO pModMachO);

static int  kldrModMachODoQuerySymbol32Bit(PKLDRMODMACHO pModMachO, const macho_nlist_32_t *paSyms, KU32 cSyms, const char *pchStrings,
                                           KU32 cchStrings, KLDRADDR BaseAddress, KU32 iSymbol, const char *pchSymbol,
                                           KU32 cchSymbol, PKLDRADDR puValue, KU32 *pfKind);
static int  kldrModMachODoQuerySymbol64Bit(PKLDRMODMACHO pModMachO, const macho_nlist_64_t *paSyms, KU32 cSyms, const char *pchStrings,
                                           KU32 cchStrings, KLDRADDR BaseAddress, KU32 iSymbol, const char *pchSymbol,
                                           KU32 cchSymbol, PKLDRADDR puValue, KU32 *pfKind);
static int  kldrModMachODoEnumSymbols32Bit(PKLDRMODMACHO pModMachO, const macho_nlist_32_t *paSyms, KU32 cSyms,
                                           const char *pchStrings, KU32 cchStrings, KLDRADDR BaseAddress,
                                           KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser);
static int  kldrModMachODoEnumSymbols64Bit(PKLDRMODMACHO pModMachO, const macho_nlist_64_t *paSyms, KU32 cSyms,
                                           const char *pchStrings, KU32 cchStrings, KLDRADDR BaseAddress,
                                           KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser);
static int  kldrModMachOObjDoImports(PKLDRMODMACHO pModMachO, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
static int  kldrModMachOObjDoFixups(PKLDRMODMACHO pModMachO, void *pvMapping, KLDRADDR NewBaseAddress);
static int  kldrModMachOFixupSectionGeneric32Bit(PKLDRMODMACHO pModMachO, KU8 *pbSectBits, PKLDRMODMACHOSECT pFixupSect,
                                                 macho_nlist_32_t *paSyms, KU32 cSyms, KLDRADDR NewBaseAddress);
static int  kldrModMachOFixupSectionAMD64(PKLDRMODMACHO pModMachO, KU8 *pbSectBits, PKLDRMODMACHOSECT pFixupSect,
                                          macho_nlist_64_t *paSyms, KU32 cSyms, KLDRADDR NewBaseAddress);

static int  kldrModMachOMakeGOT(PKLDRMODMACHO pModMachO, void *pvBits, KLDRADDR NewBaseAddress);

/*static int  kldrModMachODoFixups(PKLDRMODMACHO pModMachO, void *pvMapping, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress);
static int  kldrModMachODoImports(PKLDRMODMACHO pModMachO, void *pvMapping, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);*/


/**
 * Create a loader module instance interpreting the executable image found
 * in the specified file provider instance.
 *
 * @returns 0 on success and *ppMod pointing to a module instance.
 *          On failure, a non-zero OS specific error code is returned.
 * @param   pOps            Pointer to the registered method table.
 * @param   pRdr            The file provider instance to use.
 * @param   fFlags          Flags, MBZ.
 * @param   enmCpuArch      The desired CPU architecture. KCPUARCH_UNKNOWN means
 *                          anything goes, but with a preference for the current
 *                          host architecture.
 * @param   offNewHdr       The offset of the new header in MZ files. -1 if not found.
 * @param   ppMod           Where to store the module instance pointer.
 */
static int kldrModMachOCreate(PCKLDRMODOPS pOps, PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch, KLDRFOFF offNewHdr, PPKLDRMOD ppMod)
{
    PKLDRMODMACHO pModMachO;
    int rc;

    /*
     * Create the instance data and do a minimal header validation.
     */
    rc = kldrModMachODoCreate(pRdr, offNewHdr == -1 ? 0 : offNewHdr, fFlags, &pModMachO);
    if (!rc)
    {

        /*
         * Match up against the requested CPU architecture.
         */
        if (    enmCpuArch == KCPUARCH_UNKNOWN
            ||  pModMachO->pMod->enmArch == enmCpuArch)
        {
            pModMachO->pMod->pOps = pOps;
            pModMachO->pMod->u32Magic = KLDRMOD_MAGIC;
            *ppMod = pModMachO->pMod;
            return 0;
        }
        rc = KLDR_ERR_CPU_ARCH_MISMATCH;
    }
    if (pModMachO)
    {
        kHlpFree(pModMachO->pbLoadCommands);
        kHlpFree(pModMachO);
    }
    return rc;
}


/**
 * Separate function for reading creating the Mach-O module instance to
 * simplify cleanup on failure.
 */
static int kldrModMachODoCreate(PKRDR pRdr, KLDRFOFF offImage, KU32 fOpenFlags, PKLDRMODMACHO *ppModMachO)
{
    union
    {
        mach_header_32_t    Hdr32;
        mach_header_64_t    Hdr64;
    } s;
    PKLDRMODMACHO pModMachO;
    PKLDRMOD pMod;
    KU8 *pbLoadCommands;
    KU32 cSegments = 0; /* (MSC maybe used uninitialized) */
    KU32 cSections = 0; /* (MSC maybe used uninitialized) */
    KU32 cbStringPool = 0; /* (MSC maybe used uninitialized) */
    KSIZE cchFilename;
    KSIZE cb;
    KBOOL fMakeGot;
    KBOOL fCanLoad = K_TRUE;
    KLDRADDR LinkAddress = NIL_KLDRADDR; /* (MSC maybe used uninitialized) */
    KU8 cbJmpStub;
    KU8 uEffFileType = 0; /* (MSC maybe used uninitialized) */
    int rc;
    *ppModMachO = NULL;

    kHlpAssert(&s.Hdr32.magic == &s.Hdr64.magic);
    kHlpAssert(&s.Hdr32.flags == &s.Hdr64.flags);

    /*
     * Read the Mach-O header.
     */
    rc = kRdrRead(pRdr, &s, sizeof(s), offImage);
    if (rc)
        return rc;
    if (    s.Hdr32.magic != IMAGE_MACHO32_SIGNATURE
        &&  s.Hdr32.magic != IMAGE_MACHO64_SIGNATURE
        )
    {
        if (    s.Hdr32.magic == IMAGE_MACHO32_SIGNATURE_OE
            ||  s.Hdr32.magic == IMAGE_MACHO64_SIGNATURE_OE)
            return KLDR_ERR_MACHO_OTHER_ENDIAN_NOT_SUPPORTED;
        return KLDR_ERR_UNKNOWN_FORMAT;
    }

    /* sanity checks. */
    if (    s.Hdr32.sizeofcmds > kRdrSize(pRdr) - sizeof(mach_header_32_t)
        ||  s.Hdr32.sizeofcmds < sizeof(load_command_t) * s.Hdr32.ncmds
        ||  (s.Hdr32.flags & ~MH_VALID_FLAGS))
        return KLDR_ERR_MACHO_BAD_HEADER;
    switch (s.Hdr32.cputype)
    {
        case CPU_TYPE_X86:
            fMakeGot = K_FALSE;
            cbJmpStub = 0;
            break;
        case CPU_TYPE_X86_64:
            fMakeGot = s.Hdr32.filetype == MH_OBJECT;
            cbJmpStub = fMakeGot ? 8 : 0;
            break;
        default:
            return KLDR_ERR_MACHO_UNSUPPORTED_MACHINE;
    }
    if (   s.Hdr32.filetype != MH_OBJECT
        && s.Hdr32.filetype != MH_EXECUTE
        && s.Hdr32.filetype != MH_DYLIB
        && s.Hdr32.filetype != MH_BUNDLE
        && s.Hdr32.filetype != MH_DSYM
        && s.Hdr32.filetype != MH_KEXT_BUNDLE)
        return KLDR_ERR_MACHO_UNSUPPORTED_FILE_TYPE;

    /*
     * Read and pre-parse the load commands to figure out how many segments we'll be needing.
     */
    pbLoadCommands = kHlpAlloc(s.Hdr32.sizeofcmds);
    if (!pbLoadCommands)
        return KERR_NO_MEMORY;
    rc = kRdrRead(pRdr, pbLoadCommands, s.Hdr32.sizeofcmds,
                     s.Hdr32.magic == IMAGE_MACHO32_SIGNATURE
                  || s.Hdr32.magic == IMAGE_MACHO32_SIGNATURE_OE
                  ? sizeof(mach_header_32_t) + offImage
                  : sizeof(mach_header_64_t) + offImage);
    if (!rc)
        rc = kldrModMachOPreParseLoadCommands(pbLoadCommands, &s.Hdr32, pRdr, offImage, fOpenFlags,
                                              &cSegments, &cSections, &cbStringPool, &fCanLoad, &LinkAddress, &uEffFileType);
    if (rc)
    {
        kHlpFree(pbLoadCommands);
        return rc;
    }
    cSegments += fMakeGot;


    /*
     * Calc the instance size, allocate and initialize it.
     */
    cchFilename = kHlpStrLen(kRdrName(pRdr));
    cb = K_ALIGN_Z(  K_OFFSETOF(KLDRMODMACHO, aSegments[cSegments])
                      + sizeof(KLDRMODMACHOSECT) * cSections, 16)
       + K_OFFSETOF(KLDRMOD, aSegments[cSegments])
       + cchFilename + 1
       + cbStringPool;
    pModMachO = (PKLDRMODMACHO)kHlpAlloc(cb);
    if (!pModMachO)
        return KERR_NO_MEMORY;
    *ppModMachO = pModMachO;
    pModMachO->pbLoadCommands = pbLoadCommands;
    pModMachO->offImage = offImage;

    /* KLDRMOD */
    pMod = (PKLDRMOD)((KU8 *)pModMachO + K_ALIGN_Z(  K_OFFSETOF(KLDRMODMACHO, aSegments[cSegments])
                                                      + sizeof(KLDRMODMACHOSECT) * cSections, 16));
    pMod->pvData = pModMachO;
    pMod->pRdr = pRdr;
    pMod->pOps = NULL;      /* set upon success. */
    pMod->cSegments = cSegments;
    pMod->cchFilename = (KU32)cchFilename;
    pMod->pszFilename = (char *)&pMod->aSegments[pMod->cSegments];
    kHlpMemCopy((char *)pMod->pszFilename, kRdrName(pRdr), cchFilename + 1);
    pMod->pszName = kHlpGetFilename(pMod->pszFilename);
    pMod->cchName = (KU32)(cchFilename - (pMod->pszName - pMod->pszFilename));
    pMod->fFlags = 0;
    switch (s.Hdr32.cputype)
    {
        case CPU_TYPE_X86:
            pMod->enmArch = KCPUARCH_X86_32;
            pMod->enmEndian = KLDRENDIAN_LITTLE;
            switch (s.Hdr32.cpusubtype)
            {
                case CPU_SUBTYPE_I386_ALL:          pMod->enmCpu = KCPU_X86_32_BLEND; break;
                /*case CPU_SUBTYPE_386: ^^           pMod->enmCpu = KCPU_I386; break;*/
                case CPU_SUBTYPE_486:               pMod->enmCpu = KCPU_I486; break;
                case CPU_SUBTYPE_486SX:             pMod->enmCpu = KCPU_I486SX; break;
                /*case CPU_SUBTYPE_586: vv */
                case CPU_SUBTYPE_PENT:              pMod->enmCpu = KCPU_I586; break;
                case CPU_SUBTYPE_PENTPRO:
                case CPU_SUBTYPE_PENTII_M3:
                case CPU_SUBTYPE_PENTII_M5:
                case CPU_SUBTYPE_CELERON:
                case CPU_SUBTYPE_CELERON_MOBILE:
                case CPU_SUBTYPE_PENTIUM_3:
                case CPU_SUBTYPE_PENTIUM_3_M:
                case CPU_SUBTYPE_PENTIUM_3_XEON:    pMod->enmCpu = KCPU_I686; break;
                case CPU_SUBTYPE_PENTIUM_M:
                case CPU_SUBTYPE_PENTIUM_4:
                case CPU_SUBTYPE_PENTIUM_4_M:
                case CPU_SUBTYPE_XEON:
                case CPU_SUBTYPE_XEON_MP:           pMod->enmCpu = KCPU_P4; break;
                    break;

                default:
                    /* Hack for kextutil output. */
                    if (   s.Hdr32.cpusubtype == 0
                        && s.Hdr32.filetype   == MH_OBJECT)
                        break;
                    return KLDR_ERR_MACHO_UNSUPPORTED_MACHINE;
            }
            break;

        case CPU_TYPE_X86_64:
            pMod->enmArch = KCPUARCH_AMD64;
            pMod->enmEndian = KLDRENDIAN_LITTLE;
            switch (s.Hdr32.cpusubtype & ~CPU_SUBTYPE_MASK)
            {
                case CPU_SUBTYPE_X86_64_ALL:        pMod->enmCpu = KCPU_AMD64_BLEND; break;
                default:
                    return KLDR_ERR_MACHO_UNSUPPORTED_MACHINE;
            }
            break;

        default:
            return KLDR_ERR_MACHO_UNSUPPORTED_MACHINE;
    }

    pMod->enmFmt = KLDRFMT_MACHO;
    switch (s.Hdr32.filetype)
    {
        case MH_OBJECT:     pMod->enmType = KLDRTYPE_OBJECT; break;
        case MH_EXECUTE:    pMod->enmType = KLDRTYPE_EXECUTABLE_FIXED; break;
        case MH_DYLIB:      pMod->enmType = KLDRTYPE_SHARED_LIBRARY_RELOCATABLE; break;
        case MH_BUNDLE:     pMod->enmType = KLDRTYPE_SHARED_LIBRARY_RELOCATABLE; break;
        case MH_KEXT_BUNDLE:pMod->enmType = KLDRTYPE_SHARED_LIBRARY_RELOCATABLE; break;
        case MH_DSYM:       pMod->enmType = KLDRTYPE_DEBUG_INFO; break;
        default:
            return KLDR_ERR_MACHO_UNSUPPORTED_FILE_TYPE;
    }
    pMod->u32Magic = 0;     /* set upon success. */

    /* KLDRMODMACHO */
    pModMachO->pMod = pMod;
    pModMachO->pvBits = NULL;
    pModMachO->pvMapping = NULL;
    pModMachO->fOpenFlags = fOpenFlags;
    pModMachO->Hdr = s.Hdr64;
    if (    s.Hdr32.magic == IMAGE_MACHO32_SIGNATURE
        ||  s.Hdr32.magic == IMAGE_MACHO32_SIGNATURE_OE)
        pModMachO->Hdr.reserved = 0;
    pModMachO->LinkAddress = LinkAddress;
    pModMachO->cbImage = 0;
    pModMachO->fCanLoad = fCanLoad;
    pModMachO->fMakeGot = fMakeGot;
    pModMachO->cbJmpStub = cbJmpStub;
    pModMachO->uEffFileType = uEffFileType;
    pModMachO->offSymbols = 0;
    pModMachO->cSymbols = 0;
    pModMachO->pvaSymbols = NULL;
    pModMachO->offStrings = 0;
    pModMachO->cchStrings = 0;
    pModMachO->pchStrings = NULL;
    kHlpMemSet(pModMachO->abImageUuid, 0, sizeof(pModMachO->abImageUuid));
    pModMachO->GotRVA = NIL_KLDRADDR;
    pModMachO->JmpStubsRVA = NIL_KLDRADDR;
    pModMachO->cSections = cSections;
    pModMachO->paSections = (PKLDRMODMACHOSECT)&pModMachO->aSegments[pModMachO->pMod->cSegments];

    /*
     * Setup the KLDRMOD segment array.
     */
    rc = kldrModMachOParseLoadCommands(pModMachO, (char *)pMod->pszFilename + pMod->cchFilename + 1, cbStringPool);
    if (rc)
        return rc;

    /*
     * We're done.
     */
    return 0;
}


/**
 * Converts, validates and preparses the load commands before we carve
 * out the module instance.
 *
 * The conversion that's preformed is format endian to host endian.  The
 * preparsing has to do with segment counting, section counting and string pool
 * sizing.
 *
 * Segment are created in two different ways, depending on the file type.
 *
 * For object files there is only one segment command without a given segment
 * name. The sections inside that segment have different segment names and are
 * not sorted by their segname attribute.  We create one segment for each
 * section, with the segment name being 'segname.sectname' in order to hopefully
 * keep the names unique.  Debug sections does not get segments.
 *
 * For non-object files, one kLdr segment is created for each Mach-O segment.
 * Debug segments is not exposed by kLdr via the kLdr segment table, but via the
 * debug enumeration callback API.
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_MACHO_* on failure.
 * @param   pbLoadCommands  The load commands to parse.
 * @param   pHdr            The header.
 * @param   pRdr            The file reader.
 * @param   offImage        The image header (FAT fun).
 * @param   pcSegments      Where to store the segment count.
 * @param   pcSegments      Where to store the section count.
 * @param   pcbStringPool   Where to store the string pool size.
 * @param   pfCanLoad       Where to store the can-load-image indicator.
 * @param   pLinkAddress    Where to store the image link address (i.e. the
 *                          lowest segment address).
 */
static int  kldrModMachOPreParseLoadCommands(KU8 *pbLoadCommands, const mach_header_32_t *pHdr, PKRDR pRdr, KLDRFOFF offImage,
                                             KU32 fOpenFlags, KU32 *pcSegments, KU32 *pcSections, KU32 *pcbStringPool,
                                             PKBOOL pfCanLoad, PKLDRADDR pLinkAddress, KU8 *puEffFileType)
{
    union
    {
        KU8                  *pb;
        load_command_t       *pLoadCmd;
        segment_command_32_t *pSeg32;
        segment_command_64_t *pSeg64;
        thread_command_t     *pThread;
        symtab_command_t     *pSymTab;
        uuid_command_t       *pUuid;
    } u;
    const KU64 cbFile = kRdrSize(pRdr) - offImage;
    KU32 cSegments = 0;
    KU32 cSections = 0;
    KSIZE cbStringPool = 0;
    KU32 cLeft = pHdr->ncmds;
    KU32 cbLeft = pHdr->sizeofcmds;
    KU8 *pb = pbLoadCommands;
    int cSegmentCommands = 0;
    int cSymbolTabs = 0;
    int fConvertEndian = pHdr->magic == IMAGE_MACHO32_SIGNATURE_OE
                      || pHdr->magic == IMAGE_MACHO64_SIGNATURE_OE;
    KU8 uEffFileType = *puEffFileType = pHdr->filetype;

    *pcSegments = 0;
    *pcSections = 0;
    *pcbStringPool = 0;
    *pfCanLoad = K_TRUE;
    *pLinkAddress = ~(KLDRADDR)0;

    while (cLeft-- > 0)
    {
        u.pb = pb;

        /*
         * Convert and validate command header.
         */
        KLDRMODMACHO_CHECK_RETURN(cbLeft >= sizeof(load_command_t), KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
        if (fConvertEndian)
        {
            u.pLoadCmd->cmd = K_E2E_U32(u.pLoadCmd->cmd);
            u.pLoadCmd->cmdsize = K_E2E_U32(u.pLoadCmd->cmdsize);
        }
        KLDRMODMACHO_CHECK_RETURN(u.pLoadCmd->cmdsize <= cbLeft, KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
        cbLeft -= u.pLoadCmd->cmdsize;
        pb += u.pLoadCmd->cmdsize;

        /*
         * Convert endian if needed, parse and validate the command.
         */
        switch (u.pLoadCmd->cmd)
        {
            case LC_SEGMENT_32:
            {
                segment_command_32_t *pSrcSeg = (segment_command_32_t *)u.pLoadCmd;
                section_32_t   *pFirstSect    = (section_32_t *)(pSrcSeg + 1);
                section_32_t   *pSect         = pFirstSect;
                KU32            cSectionsLeft = pSrcSeg->nsects;
                KU64            offSect       = 0;

                /* Convert and verify the segment. */
                KLDRMODMACHO_CHECK_RETURN(u.pLoadCmd->cmdsize >= sizeof(segment_command_32_t), KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
                KLDRMODMACHO_CHECK_RETURN(   pHdr->magic == IMAGE_MACHO32_SIGNATURE_OE
                                          || pHdr->magic == IMAGE_MACHO32_SIGNATURE, KLDR_ERR_MACHO_BIT_MIX);
                if (fConvertEndian)
                {
                    pSrcSeg->vmaddr   = K_E2E_U32(pSrcSeg->vmaddr);
                    pSrcSeg->vmsize   = K_E2E_U32(pSrcSeg->vmsize);
                    pSrcSeg->fileoff  = K_E2E_U32(pSrcSeg->fileoff);
                    pSrcSeg->filesize = K_E2E_U32(pSrcSeg->filesize);
                    pSrcSeg->maxprot  = K_E2E_U32(pSrcSeg->maxprot);
                    pSrcSeg->initprot = K_E2E_U32(pSrcSeg->initprot);
                    pSrcSeg->nsects   = K_E2E_U32(pSrcSeg->nsects);
                    pSrcSeg->flags    = K_E2E_U32(pSrcSeg->flags);
                }

                /* Validation code shared with the 64-bit variant. */
                #define VALIDATE_AND_ADD_SEGMENT(a_cBits) \
                do { \
                    KBOOL fSkipSeg = !kHlpStrComp(pSrcSeg->segname, "__DWARF")   /* Note: Not for non-object files. */ \
                                  || (   !kHlpStrComp(pSrcSeg->segname, "__CTF") /* Their CTF tool did/does weird things, */ \
                                      && pSrcSeg->vmsize == 0)                   /* overlapping vmaddr and zero vmsize. */ \
                                  || (cSectionsLeft > 0 && (pFirstSect->flags & S_ATTR_DEBUG)); \
                    \
                    /* MH_DSYM files for MH_OBJECT files must have MH_OBJECT segment translation. */ \
                    if (   uEffFileType == MH_DSYM \
                        && cSegmentCommands == 0 \
                        && pSrcSeg->segname[0] == '\0') \
                        *puEffFileType = uEffFileType = MH_OBJECT; \
                    \
                    KLDRMODMACHO_CHECK_RETURN(   pSrcSeg->filesize == 0 \
                                              || (   pSrcSeg->fileoff <= cbFile \
                                                  && (KU64)pSrcSeg->fileoff + pSrcSeg->filesize <= cbFile), \
                                              KLDR_ERR_MACHO_BAD_LOAD_COMMAND); \
                    KLDRMODMACHO_CHECK_RETURN(   pSrcSeg->filesize <= pSrcSeg->vmsize \
                                              || (fSkipSeg && !kHlpStrComp(pSrcSeg->segname, "__CTF") /* see above */), \
                                              KLDR_ERR_MACHO_BAD_LOAD_COMMAND); \
                    KLDRMODMACHO_CHECK_RETURN(!(~pSrcSeg->maxprot & pSrcSeg->initprot), \
                                              KLDR_ERR_MACHO_BAD_LOAD_COMMAND); \
                    KLDRMODMACHO_CHECK_RETURN(!(pSrcSeg->flags & ~(SG_HIGHVM | SG_FVMLIB | SG_NORELOC | SG_PROTECTED_VERSION_1)), \
                                              KLDR_ERR_MACHO_BAD_LOAD_COMMAND); \
                    KLDRMODMACHO_CHECK_RETURN(   pSrcSeg->nsects * sizeof(section_##a_cBits##_t) \
                                              <= u.pLoadCmd->cmdsize - sizeof(segment_command_##a_cBits##_t), \
                                              KLDR_ERR_MACHO_BAD_LOAD_COMMAND); \
                    KLDRMODMACHO_CHECK_RETURN(   uEffFileType != MH_OBJECT \
                                              || cSegmentCommands == 0 \
                                              || (   cSegmentCommands == 1 \
                                                  && uEffFileType == MH_OBJECT \
                                                  && pHdr->filetype == MH_DSYM \
                                                  && fSkipSeg), \
                                              KLDR_ERR_MACHO_BAD_OBJECT_FILE); \
                    cSegmentCommands++; \
                    \
                    /* Add the segment, if not object file. */ \
                    if (!fSkipSeg && uEffFileType != MH_OBJECT) \
                    { \
                        cbStringPool += kHlpStrNLen(&pSrcSeg->segname[0], sizeof(pSrcSeg->segname)) + 1; \
                        cSegments++; \
                        if (cSegments == 1) /* The link address is set by the first segment. */  \
                            *pLinkAddress = pSrcSeg->vmaddr; \
                    } \
                } while (0)

                VALIDATE_AND_ADD_SEGMENT(32);

                /*
                 * Convert, validate and parse the sections.
                 */
                cSectionsLeft = pSrcSeg->nsects;
                pFirstSect = pSect = (section_32_t *)(pSrcSeg + 1);
                while (cSectionsLeft-- > 0)
                {
                    if (fConvertEndian)
                    {
                        pSect->addr      = K_E2E_U32(pSect->addr);
                        pSect->size      = K_E2E_U32(pSect->size);
                        pSect->offset    = K_E2E_U32(pSect->offset);
                        pSect->align     = K_E2E_U32(pSect->align);
                        pSect->reloff    = K_E2E_U32(pSect->reloff);
                        pSect->nreloc    = K_E2E_U32(pSect->nreloc);
                        pSect->flags     = K_E2E_U32(pSect->flags);
                        pSect->reserved1 = K_E2E_U32(pSect->reserved1);
                        pSect->reserved2 = K_E2E_U32(pSect->reserved2);
                    }

                    /* Validation code shared with the 64-bit variant. */
                    #define VALIDATE_AND_ADD_SECTION(a_cBits) \
                    do { \
                        int fFileBits; \
                        \
                        /* validate */ \
                        if (uEffFileType != MH_OBJECT) \
                            KLDRMODMACHO_CHECK_RETURN(!kHlpStrComp(pSect->segname, pSrcSeg->segname),\
                                                      KLDR_ERR_MACHO_BAD_SECTION); \
                        \
                        switch (pSect->flags & SECTION_TYPE) \
                        { \
                            case S_ZEROFILL: \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved1, KLDR_ERR_MACHO_BAD_SECTION); \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved2, KLDR_ERR_MACHO_BAD_SECTION); \
                                fFileBits = 0; \
                                break; \
                            case S_REGULAR: \
                            case S_CSTRING_LITERALS: \
                            case S_COALESCED: \
                            case S_4BYTE_LITERALS: \
                            case S_8BYTE_LITERALS: \
                            case S_16BYTE_LITERALS: \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved1, KLDR_ERR_MACHO_BAD_SECTION); \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved2, KLDR_ERR_MACHO_BAD_SECTION); \
                                fFileBits = 1; \
                                break; \
                            \
                            case S_SYMBOL_STUBS: \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved1, KLDR_ERR_MACHO_BAD_SECTION); \
                                /* reserved2 == stub size. 0 has been seen (corecrypto.kext) */ \
                                KLDRMODMACHO_CHECK_RETURN(pSect->reserved2 < 64, KLDR_ERR_MACHO_BAD_SECTION); \
                                fFileBits = 1; \
                                break; \
                            \
                            case S_NON_LAZY_SYMBOL_POINTERS: \
                            case S_LAZY_SYMBOL_POINTERS: \
                            case S_LAZY_DYLIB_SYMBOL_POINTERS: \
                                /* (reserved 1 = is indirect symbol table index) */ \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved2, KLDR_ERR_MACHO_BAD_SECTION); \
                                *pfCanLoad = K_FALSE; \
                                fFileBits = -1; /* __DATA.__got in the 64-bit mach_kernel has bits, any things without bits? */ \
                                break; \
                            \
                            case S_MOD_INIT_FUNC_POINTERS: \
                                /** @todo this requires a query API or flag... (e.g. C++ constructors) */ \
                                KLDRMODMACHO_CHECK_RETURN(fOpenFlags & KLDRMOD_OPEN_FLAGS_FOR_INFO, \
                                                          KLDR_ERR_MACHO_UNSUPPORTED_INIT_SECTION); \
                                /* Falls through. */ \
                            case S_MOD_TERM_FUNC_POINTERS: \
                                /** @todo this requires a query API or flag... (e.g. C++ destructors) */ \
                                KLDRMODMACHO_CHECK_RETURN(fOpenFlags & KLDRMOD_OPEN_FLAGS_FOR_INFO, \
                                                          KLDR_ERR_MACHO_UNSUPPORTED_TERM_SECTION); \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved1, KLDR_ERR_MACHO_BAD_SECTION); \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved2, KLDR_ERR_MACHO_BAD_SECTION); \
                                fFileBits = 1; \
                                break; /* ignored */ \
                            \
                            case S_LITERAL_POINTERS: \
                            case S_DTRACE_DOF: \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved1, KLDR_ERR_MACHO_BAD_SECTION); \
                                KLDRMODMACHO_CHECK_RETURN(!pSect->reserved2, KLDR_ERR_MACHO_BAD_SECTION); \
                                fFileBits = 1; \
                                break; \
                            \
                            case S_INTERPOSING: \
                            case S_GB_ZEROFILL: \
                                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_UNSUPPORTED_SECTION); \
                            \
                            default: \
                                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_UNKNOWN_SECTION); \
                        } \
                        KLDRMODMACHO_CHECK_RETURN(!(pSect->flags & ~(  S_ATTR_PURE_INSTRUCTIONS | S_ATTR_NO_TOC | S_ATTR_STRIP_STATIC_SYMS \
                                                                     | S_ATTR_NO_DEAD_STRIP | S_ATTR_LIVE_SUPPORT | S_ATTR_SELF_MODIFYING_CODE \
                                                                     | S_ATTR_DEBUG | S_ATTR_SOME_INSTRUCTIONS | S_ATTR_EXT_RELOC \
                                                                     | S_ATTR_LOC_RELOC | SECTION_TYPE)), \
                                                  KLDR_ERR_MACHO_BAD_SECTION); \
                        KLDRMODMACHO_CHECK_RETURN((pSect->flags & S_ATTR_DEBUG) == (pFirstSect->flags & S_ATTR_DEBUG), \
                                                  KLDR_ERR_MACHO_MIXED_DEBUG_SECTION_FLAGS); \
                        \
                        KLDRMODMACHO_CHECK_RETURN(pSect->addr - pSrcSeg->vmaddr <= pSrcSeg->vmsize, \
                                                  KLDR_ERR_MACHO_BAD_SECTION); \
                        KLDRMODMACHO_CHECK_RETURN(   pSect->addr - pSrcSeg->vmaddr + pSect->size <= pSrcSeg->vmsize \
                                                  || !kHlpStrComp(pSrcSeg->segname, "__CTF") /* see above */, \
                                                  KLDR_ERR_MACHO_BAD_SECTION); \
                        KLDRMODMACHO_CHECK_RETURN(pSect->align < 31, \
                                                  KLDR_ERR_MACHO_BAD_SECTION); \
                        /* Workaround for buggy ld64 (or as, llvm, ++) that produces a misaligned __TEXT.__unwind_info. */ \
                        /* Seen: pSect->align = 4, pSect->addr = 0x5ebe14.  Just adjust the alignment down. */ \
                        if (   ((K_BIT32(pSect->align) - KU32_C(1)) & pSect->addr) \
                            && pSect->align == 4 \
                            && kHlpStrComp(pSect->sectname, "__unwind_info") == 0) \
                            pSect->align = 2; \
                        KLDRMODMACHO_CHECK_RETURN(!((K_BIT32(pSect->align) - KU32_C(1)) & pSect->addr), \
                                                  KLDR_ERR_MACHO_BAD_SECTION); \
                        KLDRMODMACHO_CHECK_RETURN(!((K_BIT32(pSect->align) - KU32_C(1)) & pSrcSeg->vmaddr), \
                                                  KLDR_ERR_MACHO_BAD_SECTION); \
                        \
                        /* Adjust the section offset before we check file offset. */ \
                        offSect = (offSect + K_BIT64(pSect->align) - KU64_C(1)) & ~(K_BIT64(pSect->align) - KU64_C(1)); \
                        if (pSect->addr) \
                        { \
                            KLDRMODMACHO_CHECK_RETURN(offSect <= pSect->addr - pSrcSeg->vmaddr, KLDR_ERR_MACHO_BAD_SECTION); \
                            if (offSect < pSect->addr - pSrcSeg->vmaddr) \
                                offSect = pSect->addr - pSrcSeg->vmaddr; \
                        } \
                        \
                        if (fFileBits && pSect->offset == 0 && pSrcSeg->fileoff == 0 && pHdr->filetype == MH_DSYM) \
                            fFileBits = 0; \
                        if (fFileBits) \
                        { \
                            if (uEffFileType != MH_OBJECT) \
                            { \
                                KLDRMODMACHO_CHECK_RETURN(pSect->offset == pSrcSeg->fileoff + offSect, \
                                                          KLDR_ERR_MACHO_NON_CONT_SEG_BITS); \
                                KLDRMODMACHO_CHECK_RETURN(pSect->offset - pSrcSeg->fileoff <= pSrcSeg->filesize, \
                                                          KLDR_ERR_MACHO_BAD_SECTION); \
                            } \
                            KLDRMODMACHO_CHECK_RETURN(pSect->offset <= cbFile, \
                                                      KLDR_ERR_MACHO_BAD_SECTION); \
                            KLDRMODMACHO_CHECK_RETURN((KU64)pSect->offset + pSect->size <= cbFile, \
                                                      KLDR_ERR_MACHO_BAD_SECTION); \
                        } \
                        else \
                            KLDRMODMACHO_CHECK_RETURN(pSect->offset == 0, KLDR_ERR_MACHO_BAD_SECTION); \
                        \
                        if (!pSect->nreloc) \
                            KLDRMODMACHO_CHECK_RETURN(!pSect->reloff, \
                                                      KLDR_ERR_MACHO_BAD_SECTION); \
                        else \
                        { \
                            KLDRMODMACHO_CHECK_RETURN(pSect->reloff <= cbFile, \
                                                      KLDR_ERR_MACHO_BAD_SECTION); \
                            KLDRMODMACHO_CHECK_RETURN(     (KU64)pSect->reloff \
                                                         + (KLDRFOFF)pSect->nreloc * sizeof(macho_relocation_info_t) \
                                                      <= cbFile, \
                                                      KLDR_ERR_MACHO_BAD_SECTION); \
                        } \
                        \
                        /* Validate against file type (pointless?) and count the section, for object files add segment. */ \
                        switch (uEffFileType) \
                        { \
                            case MH_OBJECT: \
                                if (   !(pSect->flags & S_ATTR_DEBUG) \
                                    && kHlpStrComp(pSect->segname, "__DWARF")) \
                                { \
                                    cbStringPool += kHlpStrNLen(&pSect->segname[0], sizeof(pSect->segname)) + 1; \
                                    cbStringPool += kHlpStrNLen(&pSect->sectname[0], sizeof(pSect->sectname)) + 1; \
                                    cSegments++; \
                                    if (cSegments == 1) /* The link address is set by the first segment. */  \
                                        *pLinkAddress = pSect->addr; \
                                } \
                                /* Falls through. */ \
                            case MH_EXECUTE: \
                            case MH_DYLIB: \
                            case MH_BUNDLE: \
                            case MH_DSYM: \
                            case MH_KEXT_BUNDLE: \
                                cSections++; \
                                break; \
                            default: \
                                KLDRMODMACHO_FAILED_RETURN(KERR_INVALID_PARAMETER); \
                        } \
                        \
                        /* Advance the section offset, since we're also aligning it. */ \
                        offSect += pSect->size; \
                    } while (0) /* VALIDATE_AND_ADD_SECTION */

                    VALIDATE_AND_ADD_SECTION(32);

                    /* next */
                    pSect++;
                }
                break;
            }

            case LC_SEGMENT_64:
            {
                segment_command_64_t *pSrcSeg = (segment_command_64_t *)u.pLoadCmd;
                section_64_t   *pFirstSect    = (section_64_t *)(pSrcSeg + 1);
                section_64_t   *pSect         = pFirstSect;
                KU32            cSectionsLeft = pSrcSeg->nsects;
                KU64            offSect       = 0;

                /* Convert and verify the segment. */
                KLDRMODMACHO_CHECK_RETURN(u.pLoadCmd->cmdsize >= sizeof(segment_command_64_t), KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
                KLDRMODMACHO_CHECK_RETURN(   pHdr->magic == IMAGE_MACHO64_SIGNATURE_OE
                                          || pHdr->magic == IMAGE_MACHO64_SIGNATURE, KLDR_ERR_MACHO_BIT_MIX);
                if (fConvertEndian)
                {
                    pSrcSeg->vmaddr   = K_E2E_U64(pSrcSeg->vmaddr);
                    pSrcSeg->vmsize   = K_E2E_U64(pSrcSeg->vmsize);
                    pSrcSeg->fileoff  = K_E2E_U64(pSrcSeg->fileoff);
                    pSrcSeg->filesize = K_E2E_U64(pSrcSeg->filesize);
                    pSrcSeg->maxprot  = K_E2E_U32(pSrcSeg->maxprot);
                    pSrcSeg->initprot = K_E2E_U32(pSrcSeg->initprot);
                    pSrcSeg->nsects   = K_E2E_U32(pSrcSeg->nsects);
                    pSrcSeg->flags    = K_E2E_U32(pSrcSeg->flags);
                }

                VALIDATE_AND_ADD_SEGMENT(64);

                /*
                 * Convert, validate and parse the sections.
                 */
                while (cSectionsLeft-- > 0)
                {
                    if (fConvertEndian)
                    {
                        pSect->addr      = K_E2E_U64(pSect->addr);
                        pSect->size      = K_E2E_U64(pSect->size);
                        pSect->offset    = K_E2E_U32(pSect->offset);
                        pSect->align     = K_E2E_U32(pSect->align);
                        pSect->reloff    = K_E2E_U32(pSect->reloff);
                        pSect->nreloc    = K_E2E_U32(pSect->nreloc);
                        pSect->flags     = K_E2E_U32(pSect->flags);
                        pSect->reserved1 = K_E2E_U32(pSect->reserved1);
                        pSect->reserved2 = K_E2E_U32(pSect->reserved2);
                    }

                    VALIDATE_AND_ADD_SECTION(64);

                    /* next */
                    pSect++;
                }
                break;
            } /* LC_SEGMENT_64 */


            case LC_SYMTAB:
            {
                KSIZE cbSym;
                if (fConvertEndian)
                {
                    u.pSymTab->symoff  = K_E2E_U32(u.pSymTab->symoff);
                    u.pSymTab->nsyms   = K_E2E_U32(u.pSymTab->nsyms);
                    u.pSymTab->stroff  = K_E2E_U32(u.pSymTab->stroff);
                    u.pSymTab->strsize = K_E2E_U32(u.pSymTab->strsize);
                }

                /* verify */
                cbSym = pHdr->magic == IMAGE_MACHO32_SIGNATURE
                     || pHdr->magic == IMAGE_MACHO32_SIGNATURE_OE
                      ? sizeof(macho_nlist_32_t)
                      : sizeof(macho_nlist_64_t);
                if (    u.pSymTab->symoff >= cbFile
                    ||  (KU64)u.pSymTab->symoff + u.pSymTab->nsyms * cbSym > cbFile)
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
                if (    u.pSymTab->stroff >= cbFile
                    ||  (KU64)u.pSymTab->stroff + u.pSymTab->strsize > cbFile)
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);

                /* only one string in objects, please. */
                cSymbolTabs++;
                if (    uEffFileType == MH_OBJECT
                    &&  cSymbolTabs != 1)
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_OBJECT_FILE);
                break;
            }

            case LC_DYSYMTAB:
                /** @todo deal with this! */
                break;

            case LC_THREAD:
            case LC_UNIXTHREAD:
            {
                KU32 *pu32 = (KU32 *)(u.pb + sizeof(load_command_t));
                KU32 cItemsLeft = (u.pThread->cmdsize - sizeof(load_command_t)) / sizeof(KU32);
                while (cItemsLeft)
                {
                    /* convert & verify header items ([0] == flavor, [1] == KU32 count). */
                    if (cItemsLeft < 2)
                        KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
                    if (fConvertEndian)
                    {
                        pu32[0] = K_E2E_U32(pu32[0]);
                        pu32[1] = K_E2E_U32(pu32[1]);
                    }
                    if (pu32[1] + 2 > cItemsLeft)
                        KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);

                    /* convert & verify according to flavor. */
                    switch (pu32[0])
                    {
                        /** @todo */
                        default:
                            break;
                    }

                    /* next */
                    cItemsLeft -= pu32[1] + 2;
                    pu32 += pu32[1] + 2;
                }
                break;
            }

            case LC_UUID:
                if (u.pUuid->cmdsize != sizeof(uuid_command_t))
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
                /** @todo Check anything here need converting? */
                break;

            case LC_CODE_SIGNATURE:
                if (u.pUuid->cmdsize != sizeof(linkedit_data_command_t))
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
                break;

            case LC_VERSION_MIN_MACOSX:
            case LC_VERSION_MIN_IPHONEOS:
                if (u.pUuid->cmdsize != sizeof(version_min_command_t))
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);
                break;

            case LC_SOURCE_VERSION:     /* Harmless. It just gives a clue regarding the source code revision/version. */
            case LC_DATA_IN_CODE:       /* Ignore */
            case LC_DYLIB_CODE_SIGN_DRS:/* Ignore */
                /** @todo valid command size. */
                break;

            case LC_FUNCTION_STARTS:    /** @todo dylib++ */
                /* Ignore for now. */
                break;
            case LC_ID_DYLIB:           /** @todo dylib */
            case LC_LOAD_DYLIB:         /** @todo dylib */
            case LC_LOAD_DYLINKER:      /** @todo dylib */
            case LC_TWOLEVEL_HINTS:     /** @todo dylib */
            case LC_LOAD_WEAK_DYLIB:    /** @todo dylib */
            case LC_ID_DYLINKER:        /** @todo dylib */
            case LC_RPATH:              /** @todo dylib */
            case LC_SEGMENT_SPLIT_INFO: /** @todo dylib++ */
            case LC_REEXPORT_DYLIB:     /** @todo dylib */
            case LC_DYLD_INFO:          /** @todo dylib */
            case LC_DYLD_INFO_ONLY:     /** @todo dylib */
            case LC_LOAD_UPWARD_DYLIB:  /** @todo dylib */
            case LC_DYLD_ENVIRONMENT:   /** @todo dylib */
            case LC_MAIN: /** @todo parse this and find and entry point or smth. */
                /** @todo valid command size. */
                if (!(fOpenFlags & KLDRMOD_OPEN_FLAGS_FOR_INFO))
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_UNSUPPORTED_LOAD_COMMAND);
                *pfCanLoad = K_FALSE;
                break;

            case LC_LOADFVMLIB:
            case LC_IDFVMLIB:
            case LC_IDENT:
            case LC_FVMFILE:
            case LC_PREPAGE:
            case LC_PREBOUND_DYLIB:
            case LC_ROUTINES:
            case LC_ROUTINES_64:
            case LC_SUB_FRAMEWORK:
            case LC_SUB_UMBRELLA:
            case LC_SUB_CLIENT:
            case LC_SUB_LIBRARY:
            case LC_PREBIND_CKSUM:
            case LC_SYMSEG:
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_UNSUPPORTED_LOAD_COMMAND);

            default:
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_UNKNOWN_LOAD_COMMAND);
        }
    }

    /* be strict. */
    if (cbLeft)
        KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_LOAD_COMMAND);

    switch (uEffFileType)
    {
        case MH_OBJECT:
        case MH_EXECUTE:
        case MH_DYLIB:
        case MH_BUNDLE:
        case MH_DSYM:
        case MH_KEXT_BUNDLE:
            if (!cSegments)
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_OBJECT_FILE);
            break;
    }

    *pcSegments = cSegments;
    *pcSections = cSections;
    *pcbStringPool = (KU32)cbStringPool;

    return 0;
}


/**
 * Parses the load commands after we've carved out the module instance.
 *
 * This fills in the segment table and perhaps some other properties.
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_MACHO_* on failure.
 * @param   pModMachO       The module.
 * @param   pbStringPool    The string pool
 * @param   cbStringPool    The size of the string pool.
 */
static int  kldrModMachOParseLoadCommands(PKLDRMODMACHO pModMachO, char *pbStringPool, KU32 cbStringPool)
{
    union
    {
        const KU8                  *pb;
        const load_command_t       *pLoadCmd;
        const segment_command_32_t *pSeg32;
        const segment_command_64_t *pSeg64;
        const symtab_command_t     *pSymTab;
        const uuid_command_t       *pUuid;
    } u;
    KU32 cLeft = pModMachO->Hdr.ncmds;
    KU32 cbLeft = pModMachO->Hdr.sizeofcmds;
    const KU8 *pb = pModMachO->pbLoadCommands;
    PKLDRSEG pDstSeg = &pModMachO->pMod->aSegments[0];
    PKLDRMODMACHOSEG pSegExtra = &pModMachO->aSegments[0];
    PKLDRMODMACHOSECT pSectExtra = pModMachO->paSections;
    const KU32 cSegments = pModMachO->pMod->cSegments;
    PKLDRSEG pSegItr;
    K_NOREF(cbStringPool);

    while (cLeft-- > 0)
    {
        u.pb = pb;
        cbLeft -= u.pLoadCmd->cmdsize;
        pb += u.pLoadCmd->cmdsize;

        /*
         * Convert endian if needed, parse and validate the command.
         */
        switch (u.pLoadCmd->cmd)
        {
            case LC_SEGMENT_32:
            {
                const segment_command_32_t *pSrcSeg = (const segment_command_32_t *)u.pLoadCmd;
                section_32_t   *pFirstSect    = (section_32_t *)(pSrcSeg + 1);
                section_32_t   *pSect         = pFirstSect;
                KU32            cSectionsLeft = pSrcSeg->nsects;

                /* Adds a segment, used by the macro below and thus shared with the 64-bit segment variant. */
                #define NEW_SEGMENT(a_cBits, a_achName1, a_fObjFile, a_achName2, a_SegAddr, a_cbSeg, a_fFileBits, a_offFile, a_cbFile) \
                do { \
                    pDstSeg->pvUser = NULL; \
                    pDstSeg->pchName = pbStringPool; \
                    pDstSeg->cchName = (KU32)kHlpStrNLen(a_achName1, sizeof(a_achName1)); \
                    kHlpMemCopy(pbStringPool, a_achName1, pDstSeg->cchName); \
                    pbStringPool += pDstSeg->cchName; \
                    if (a_fObjFile) \
                    {   /* MH_OBJECT: Add '.sectname' - sections aren't sorted by segments. */ \
                        KSIZE cchName2 = kHlpStrNLen(a_achName2, sizeof(a_achName2)); \
                        *pbStringPool++ = '.'; \
                        kHlpMemCopy(pbStringPool, a_achName2, cchName2); \
                        pbStringPool += cchName2; \
                        pDstSeg->cchName += (KU32)cchName2; \
                    } \
                    *pbStringPool++ = '\0'; \
                    pDstSeg->SelFlat = 0; \
                    pDstSeg->Sel16bit = 0; \
                    pDstSeg->fFlags = 0; \
                    pDstSeg->enmProt = KPROT_EXECUTE_WRITECOPY; /** @todo fixme! */ \
                    pDstSeg->cb = (a_cbSeg); \
                    pDstSeg->Alignment = 1; /* updated while parsing sections. */ \
                    pDstSeg->LinkAddress = (a_SegAddr); \
                    if (a_fFileBits) \
                    { \
                        pDstSeg->offFile = (KLDRFOFF)((a_offFile) + pModMachO->offImage); \
                        pDstSeg->cbFile  = (KLDRFOFF)(a_cbFile); \
                    } \
                    else \
                    { \
                        pDstSeg->offFile = -1; \
                        pDstSeg->cbFile  = -1; \
                    } \
                    pDstSeg->RVA = (a_SegAddr) - pModMachO->LinkAddress; \
                    pDstSeg->cbMapped = 0; \
                    pDstSeg->MapAddress = 0; \
                    \
                    pSegExtra->iOrgSegNo = (KU32)(pSegExtra - &pModMachO->aSegments[0]); \
                    pSegExtra->cSections = 0; \
                    pSegExtra->paSections = pSectExtra; \
                } while (0)

                /* Closes the new segment - part of NEW_SEGMENT. */
                #define CLOSE_SEGMENT() \
                do { \
                    pSegExtra->cSections = (KU32)(pSectExtra - pSegExtra->paSections); \
                    pSegExtra++; \
                    pDstSeg++; \
                } while (0)


                /* Shared with the 64-bit variant. */
                #define ADD_SEGMENT_AND_ITS_SECTIONS(a_cBits) \
                do { \
                    KBOOL fAddSegOuter = K_FALSE; \
                    \
                    /* \
                     * Check that the segment name is unique.  We couldn't do that \
                     * in the preparsing stage. \
                     */ \
                    if (pModMachO->uEffFileType != MH_OBJECT) \
                        for (pSegItr = &pModMachO->pMod->aSegments[0]; pSegItr != pDstSeg; pSegItr++) \
                            if (!kHlpStrNComp(pSegItr->pchName, pSrcSeg->segname, sizeof(pSrcSeg->segname))) \
                                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_DUPLICATE_SEGMENT_NAME); \
                    \
                    /* \
                     * Create a new segment, unless we're supposed to skip this one. \
                     */ \
                    if (   pModMachO->uEffFileType != MH_OBJECT \
                        && (cSectionsLeft == 0 || !(pFirstSect->flags & S_ATTR_DEBUG)) \
                        && kHlpStrComp(pSrcSeg->segname, "__DWARF") \
                        && kHlpStrComp(pSrcSeg->segname, "__CTF") ) \
                    { \
                        NEW_SEGMENT(a_cBits, pSrcSeg->segname, K_FALSE /*a_fObjFile*/, 0 /*a_achName2*/, \
                                    pSrcSeg->vmaddr, pSrcSeg->vmsize, \
                                    pSrcSeg->filesize != 0, pSrcSeg->fileoff, pSrcSeg->filesize); \
                        fAddSegOuter = K_TRUE; \
                    } \
                    \
                    /* \
                     * Convert and parse the sections. \
                     */ \
                    while (cSectionsLeft-- > 0) \
                    { \
                        /* New segment if object file. */ \
                        KBOOL fAddSegInner = K_FALSE; \
                        if (   pModMachO->uEffFileType == MH_OBJECT \
                            && !(pSect->flags & S_ATTR_DEBUG) \
                            && kHlpStrComp(pSrcSeg->segname, "__DWARF") \
                            && kHlpStrComp(pSrcSeg->segname, "__CTF") ) \
                        { \
                            kHlpAssert(!fAddSegOuter); \
                            NEW_SEGMENT(a_cBits, pSect->segname, K_TRUE /*a_fObjFile*/, pSect->sectname, \
                                        pSect->addr, pSect->size, \
                                        pSect->offset != 0, pSect->offset, pSect->size); \
                            fAddSegInner = K_TRUE; \
                        } \
                        \
                        /* Section data extract. */ \
                        pSectExtra->cb = pSect->size; \
                        pSectExtra->RVA = pSect->addr - pDstSeg->LinkAddress; \
                        pSectExtra->LinkAddress = pSect->addr; \
                        if (pSect->offset) \
                            pSectExtra->offFile = pSect->offset + pModMachO->offImage; \
                        else \
                            pSectExtra->offFile = -1; \
                        pSectExtra->cFixups = pSect->nreloc; \
                        pSectExtra->paFixups = NULL; \
                        if (pSect->nreloc) \
                            pSectExtra->offFixups = pSect->reloff + pModMachO->offImage; \
                        else \
                            pSectExtra->offFixups = -1; \
                        pSectExtra->fFlags = pSect->flags; \
                        pSectExtra->iSegment = (KU32)(pSegExtra - &pModMachO->aSegments[0]); \
                        pSectExtra->pvMachoSection = pSect; \
                        \
                        /* Update the segment alignment, if we're not skipping it. */ \
                        if (   (fAddSegOuter || fAddSegInner) \
                            && pDstSeg->Alignment < ((KLDRADDR)1 << pSect->align)) \
                            pDstSeg->Alignment = (KLDRADDR)1 << pSect->align; \
                        \
                        /* Next section, and if object file next segment. */ \
                        pSectExtra++; \
                        pSect++; \
                        if (fAddSegInner) \
                            CLOSE_SEGMENT(); \
                    } \
                    \
                    /* Close the segment and advance. */ \
                    if (fAddSegOuter) \
                        CLOSE_SEGMENT(); \
                } while (0) /* ADD_SEGMENT_AND_ITS_SECTIONS */

                ADD_SEGMENT_AND_ITS_SECTIONS(32);
                break;
            }

            case LC_SEGMENT_64:
            {
                const segment_command_64_t *pSrcSeg = (const segment_command_64_t *)u.pLoadCmd;
                section_64_t   *pFirstSect    = (section_64_t *)(pSrcSeg + 1);
                section_64_t   *pSect         = pFirstSect;
                KU32            cSectionsLeft = pSrcSeg->nsects;

                ADD_SEGMENT_AND_ITS_SECTIONS(64);
                break;
            }

            case LC_SYMTAB:
                switch (pModMachO->uEffFileType)
                {
                    case MH_OBJECT:
                    case MH_EXECUTE:
                    case MH_DYLIB: /** @todo ??? */
                    case MH_BUNDLE:  /** @todo ??? */
                    case MH_DSYM:
                    case MH_KEXT_BUNDLE:
                        pModMachO->offSymbols = u.pSymTab->symoff + pModMachO->offImage;
                        pModMachO->cSymbols = u.pSymTab->nsyms;
                        pModMachO->offStrings = u.pSymTab->stroff + pModMachO->offImage;
                        pModMachO->cchStrings = u.pSymTab->strsize;
                        break;
                }
                break;

            case LC_UUID:
                kHlpMemCopy(pModMachO->abImageUuid, u.pUuid->uuid, sizeof(pModMachO->abImageUuid));
                break;

            default:
                break;
        } /* command switch */
    } /* while more commands */

    kHlpAssert(pDstSeg == &pModMachO->pMod->aSegments[cSegments - pModMachO->fMakeGot]);

    /*
     * Adjust mapping addresses calculating the image size.
     */
    {
        KBOOL               fLoadLinkEdit = K_FALSE;
        PKLDRMODMACHOSECT   pSectExtraItr;
        KLDRADDR            uNextRVA = 0;
        KLDRADDR            cb;
        KU32                cSegmentsToAdjust = cSegments - pModMachO->fMakeGot;
        KU32                c;

        for (;;)
        {
            /* Check if there is __DWARF segment at the end and make sure it's left
               out of the RVA negotiations and image loading. */
            if (   cSegmentsToAdjust > 0
                && !kHlpStrComp(pModMachO->pMod->aSegments[cSegmentsToAdjust - 1].pchName, "__DWARF"))
            {
                cSegmentsToAdjust--;
                pModMachO->pMod->aSegments[cSegmentsToAdjust].RVA = NIL_KLDRADDR;
                pModMachO->pMod->aSegments[cSegmentsToAdjust].cbMapped = 0;
                continue;
            }

            /* If we're skipping the __LINKEDIT segment, check for it and adjust
               the number of segments we'll be messing with here.  ASSUMES it's
               last (by now anyway). */
            if (   !fLoadLinkEdit
                && cSegmentsToAdjust > 0
                && !kHlpStrComp(pModMachO->pMod->aSegments[cSegmentsToAdjust - 1].pchName, "__LINKEDIT"))
            {
                cSegmentsToAdjust--;
                pModMachO->pMod->aSegments[cSegmentsToAdjust].RVA = NIL_KLDRADDR;
                pModMachO->pMod->aSegments[cSegmentsToAdjust].cbMapped = 0;
                continue;
            }
            break;
        }

        /* Adjust RVAs. */
        c = cSegmentsToAdjust;
        for (pDstSeg = &pModMachO->pMod->aSegments[0]; c-- > 0; pDstSeg++)
        {
            cb = pDstSeg->RVA - uNextRVA;
            if (cb >= 0x00100000) /* 1MB */
            {
                pDstSeg->RVA = uNextRVA;
                pModMachO->pMod->fFlags |= KLDRMOD_FLAGS_NON_CONTIGUOUS_LINK_ADDRS;
            }
            uNextRVA = pDstSeg->RVA + KLDR_ALIGN_ADDR(pDstSeg->cb, pDstSeg->Alignment);
        }

        /* Calculate the cbMapping members. */
        c = cSegmentsToAdjust;
        for (pDstSeg = &pModMachO->pMod->aSegments[0]; c-- > 1; pDstSeg++)
        {

            cb = pDstSeg[1].RVA - pDstSeg->RVA;
            pDstSeg->cbMapped = (KSIZE)cb == cb ? (KSIZE)cb : KSIZE_MAX;
        }

        cb = KLDR_ALIGN_ADDR(pDstSeg->cb, pDstSeg->Alignment);
        pDstSeg->cbMapped = (KSIZE)cb == cb ? (KSIZE)cb : KSIZE_MAX;

        /* Set the image size. */
        pModMachO->cbImage = pDstSeg->RVA + cb;

        /* Fixup the section RVAs (internal). */
        c        = cSegmentsToAdjust;
        uNextRVA = pModMachO->cbImage;
        pDstSeg  = &pModMachO->pMod->aSegments[0];
        for (pSectExtraItr = pModMachO->paSections; pSectExtraItr != pSectExtra; pSectExtraItr++)
        {
            if (pSectExtraItr->iSegment < c)
                pSectExtraItr->RVA += pDstSeg[pSectExtraItr->iSegment].RVA;
            else
            {
                pSectExtraItr->RVA = uNextRVA;
                uNextRVA += KLDR_ALIGN_ADDR(pSectExtraItr->cb, 64);
            }
        }
    }

    /*
     * Make the GOT segment if necessary.
     */
    if (pModMachO->fMakeGot)
    {
        KU32 cbPtr = (   pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
                      || pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
                   ? sizeof(KU32)
                   : sizeof(KU64);
        KU32 cbGot = pModMachO->cSymbols * cbPtr;
        KU32 cbJmpStubs;

        pModMachO->GotRVA = pModMachO->cbImage;

        if (pModMachO->cbJmpStub)
        {
            cbGot = K_ALIGN_Z(cbGot, 64);
            pModMachO->JmpStubsRVA = pModMachO->GotRVA + cbGot;
            cbJmpStubs = pModMachO->cbJmpStub * pModMachO->cSymbols;
        }
        else
        {
            pModMachO->JmpStubsRVA = NIL_KLDRADDR;
            cbJmpStubs = 0;
        }

        pDstSeg = &pModMachO->pMod->aSegments[cSegments - 1];
        pDstSeg->pvUser = NULL;
        pDstSeg->pchName = "GOT";
        pDstSeg->cchName = 3;
        pDstSeg->SelFlat = 0;
        pDstSeg->Sel16bit = 0;
        pDstSeg->fFlags = 0;
        pDstSeg->enmProt = KPROT_READONLY;
        pDstSeg->cb = cbGot + cbJmpStubs;
        pDstSeg->Alignment = 64;
        pDstSeg->LinkAddress = pModMachO->LinkAddress + pModMachO->GotRVA;
        pDstSeg->offFile = -1;
        pDstSeg->cbFile  = -1;
        pDstSeg->RVA = pModMachO->GotRVA;
        pDstSeg->cbMapped = (KSIZE)KLDR_ALIGN_ADDR(cbGot + cbJmpStubs, pDstSeg->Alignment);
        pDstSeg->MapAddress = 0;

        pSegExtra->iOrgSegNo = KU32_MAX;
        pSegExtra->cSections = 0;
        pSegExtra->paSections = NULL;

        pModMachO->cbImage += pDstSeg->cbMapped;
    }

    return 0;
}


/** @copydoc KLDRMODOPS::pfnDestroy */
static int kldrModMachODestroy(PKLDRMOD pMod)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    int rc = 0;
    KU32 i, j;
    KLDRMODMACHO_ASSERT(!pModMachO->pvMapping);

    i = pMod->cSegments;
    while (i-- > 0)
    {
        j = pModMachO->aSegments[i].cSections;
        while (j-- > 0)
        {
            kHlpFree(pModMachO->aSegments[i].paSections[j].paFixups);
            pModMachO->aSegments[i].paSections[j].paFixups = NULL;
        }
    }

    if (pMod->pRdr)
    {
        rc = kRdrClose(pMod->pRdr);
        pMod->pRdr = NULL;
    }
    pMod->u32Magic = 0;
    pMod->pOps = NULL;
    kHlpFree(pModMachO->pbLoadCommands);
    pModMachO->pbLoadCommands = NULL;
    kHlpFree(pModMachO->pchStrings);
    pModMachO->pchStrings = NULL;
    kHlpFree(pModMachO->pvaSymbols);
    pModMachO->pvaSymbols = NULL;
    kHlpFree(pModMachO);
    return rc;
}


/**
 * Gets the right base address.
 *
 * @returns 0 on success.
 * @returns A non-zero status code if the BaseAddress isn't right.
 * @param   pModMachO       The interpreter module instance
 * @param   pBaseAddress    The base address, IN & OUT. Optional.
 */
static int kldrModMachOAdjustBaseAddress(PKLDRMODMACHO pModMachO, PKLDRADDR pBaseAddress)
{
    /*
     * Adjust the base address.
     */
    if (*pBaseAddress == KLDRMOD_BASEADDRESS_MAP)
        *pBaseAddress = pModMachO->pMod->aSegments[0].MapAddress;
    else if (*pBaseAddress == KLDRMOD_BASEADDRESS_LINK)
        *pBaseAddress = pModMachO->LinkAddress;

    return 0;
}


/**
 * Resolves a linker generated symbol.
 *
 * The Apple linker generates symbols indicating the start and end of sections
 * and segments.  This function checks for these and returns the right value.
 *
 * @returns 0 or KLDR_ERR_SYMBOL_NOT_FOUND.
 * @param   pModMachO           The interpreter module instance.
 * @param   pMod                The generic module instance.
 * @param   pchSymbol           The symbol.
 * @param   cchSymbol           The length of the symbol.
 * @param   BaseAddress         The base address to apply when calculating the
 *                              value.
 * @param   puValue             Where to return the symbol value.
 */
static int kldrModMachOQueryLinkerSymbol(PKLDRMODMACHO pModMachO, PKLDRMOD pMod, const char *pchSymbol, KSIZE cchSymbol,
                                         KLDRADDR BaseAddress, PKLDRADDR puValue)
{
    /*
     * Match possible name prefixes.
     */
    static const struct
    {
        const char *pszPrefix;
        KU8         cchPrefix;
        KBOOL       fSection;
        KBOOL       fStart;
    }   s_aPrefixes[] =
    {
        { "section$start$",  (KU8)sizeof("section$start$") - 1,   K_TRUE,  K_TRUE },
        { "section$end$",    (KU8)sizeof("section$end$") - 1,     K_TRUE,  K_FALSE},
        { "segment$start$",  (KU8)sizeof("segment$start$") - 1,   K_FALSE, K_TRUE },
        { "segment$end$",    (KU8)sizeof("segment$end$") - 1,     K_FALSE, K_FALSE},
    };
    KSIZE       cchSectName = 0;
    const char *pchSectName = "";
    KSIZE       cchSegName  = 0;
    const char *pchSegName  = NULL;
    KU32        iPrefix     = K_ELEMENTS(s_aPrefixes) - 1;
    KU32        iSeg;
    KLDRADDR    uValue;

    for (;;)
    {
        KU8 const cchPrefix = s_aPrefixes[iPrefix].cchPrefix;
        if (   cchSymbol > cchPrefix
            && kHlpStrNComp(pchSymbol, s_aPrefixes[iPrefix].pszPrefix, cchPrefix) == 0)
        {
            pchSegName = pchSymbol + cchPrefix;
            cchSegName = cchSymbol - cchPrefix;
            break;
        }

        /* next */
        if (!iPrefix)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        iPrefix--;
    }

    /*
     * Split the remainder into segment and section name, if necessary.
     */
    if (s_aPrefixes[iPrefix].fSection)
    {
        pchSectName = kHlpMemChr(pchSegName, '$', cchSegName);
        if (!pchSectName)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        cchSegName  = pchSectName - pchSegName;
        pchSectName++;
        cchSectName = cchSymbol - (pchSectName - pchSymbol);
    }

    /*
     * Locate the segment.
     */
    if (!pMod->cSegments)
        return KLDR_ERR_SYMBOL_NOT_FOUND;
    for (iSeg = 0; iSeg < pMod->cSegments; iSeg++)
    {
        if (   pMod->aSegments[iSeg].cchName >= cchSegName
            && kHlpMemComp(pMod->aSegments[iSeg].pchName, pchSegName, cchSegName) == 0)
        {
            section_32_t const *pSect;
            if (   pMod->aSegments[iSeg].cchName == cchSegName
                && pModMachO->Hdr.filetype != MH_OBJECT /* Good enough for __DWARF segs in MH_DHSYM, I hope. */)
                break;

            pSect = (section_32_t *)pModMachO->aSegments[iSeg].paSections[0].pvMachoSection;
            if (   pModMachO->uEffFileType == MH_OBJECT
                && pMod->aSegments[iSeg].cchName > cchSegName + 1
                && pMod->aSegments[iSeg].pchName[cchSegName] == '.'
                && kHlpStrNComp(&pMod->aSegments[iSeg].pchName[cchSegName + 1], pSect->sectname, sizeof(pSect->sectname)) == 0
                && pMod->aSegments[iSeg].cchName - cchSegName - 1 <= sizeof(pSect->sectname) )
                break;
        }
    }
    if (iSeg >= pMod->cSegments)
        return KLDR_ERR_SYMBOL_NOT_FOUND;

    if (!s_aPrefixes[iPrefix].fSection)
    {
        /*
         * Calculate the segment start/end address.
         */
        uValue = pMod->aSegments[iSeg].RVA;
        if (!s_aPrefixes[iPrefix].fStart)
            uValue += pMod->aSegments[iSeg].cb;
    }
    else
    {
        /*
         * Locate the section.
         */
        KU32 iSect = pModMachO->aSegments[iSeg].cSections;
        if (!iSect)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        for (;;)
        {
            section_32_t *pSect = (section_32_t *)pModMachO->aSegments[iSeg].paSections[iSect].pvMachoSection;
            if (   cchSectName <= sizeof(pSect->sectname)
                && kHlpMemComp(pSect->sectname, pchSectName, cchSectName) == 0
                && (   cchSectName == sizeof(pSect->sectname)
                    || pSect->sectname[cchSectName] == '\0') )
                break;
            /* next */
            if (!iSect)
                return KLDR_ERR_SYMBOL_NOT_FOUND;
            iSect--;
        }

        uValue = pModMachO->aSegments[iSeg].paSections[iSect].RVA;
        if (!s_aPrefixes[iPrefix].fStart)
            uValue += pModMachO->aSegments[iSeg].paSections[iSect].cb;
    }

    /*
     * Convert from RVA to load address.
     */
    uValue += BaseAddress;
    if (puValue)
        *puValue = uValue;

    return 0;
}


/** @copydoc kLdrModQuerySymbol */
static int kldrModMachOQuerySymbol(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 iSymbol,
                                   const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                                   PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    int rc;
    K_NOREF(pvBits);
    K_NOREF(pszVersion);
    K_NOREF(pfnGetForwarder);
    K_NOREF(pvUser);

    /*
     * Resolve defaults.
     */
    rc  = kldrModMachOAdjustBaseAddress(pModMachO, &BaseAddress);
    if (rc)
        return rc;

    /*
     * Refuse segmented requests for now.
     */
    KLDRMODMACHO_CHECK_RETURN(   !pfKind
                              || (*pfKind & KLDRSYMKIND_REQ_TYPE_MASK) == KLDRSYMKIND_REQ_FLAT,
                              KLDR_ERR_TODO);

    /*
     * Take action according to file type.
     */
    if (   pModMachO->Hdr.filetype == MH_OBJECT
        || pModMachO->Hdr.filetype == MH_EXECUTE /** @todo dylib, execute, dsym: symbols */
        || pModMachO->Hdr.filetype == MH_DYLIB
        || pModMachO->Hdr.filetype == MH_BUNDLE
        || pModMachO->Hdr.filetype == MH_DSYM
        || pModMachO->Hdr.filetype == MH_KEXT_BUNDLE)
    {
        rc = kldrModMachOLoadObjSymTab(pModMachO);
        if (!rc)
        {
            if (    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
                ||  pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
                rc = kldrModMachODoQuerySymbol32Bit(pModMachO, (macho_nlist_32_t *)pModMachO->pvaSymbols, pModMachO->cSymbols,
                                                    pModMachO->pchStrings, pModMachO->cchStrings, BaseAddress, iSymbol, pchSymbol,
                                                    (KU32)cchSymbol, puValue, pfKind);
            else
                rc = kldrModMachODoQuerySymbol64Bit(pModMachO, (macho_nlist_64_t *)pModMachO->pvaSymbols, pModMachO->cSymbols,
                                                    pModMachO->pchStrings, pModMachO->cchStrings, BaseAddress, iSymbol, pchSymbol,
                                                    (KU32)cchSymbol, puValue, pfKind);
        }

        /*
         * Check for link-editor generated symbols and supply what we can.
         *
         * As small service to clients that insists on adding a '_' prefix
         * before querying symbols, we will ignore the prefix.
         */
        if (  rc == KLDR_ERR_SYMBOL_NOT_FOUND
            && cchSymbol > sizeof("section$end$") - 1
            && (    pchSymbol[0] == 's'
                || (pchSymbol[1] == 's' && pchSymbol[0] == '_') )
            && kHlpMemChr(pchSymbol, '$', cchSymbol) )
        {
            if (pchSymbol[0] == '_')
                rc = kldrModMachOQueryLinkerSymbol(pModMachO, pMod, pchSymbol + 1, cchSymbol - 1, BaseAddress, puValue);
            else
                rc = kldrModMachOQueryLinkerSymbol(pModMachO, pMod, pchSymbol, cchSymbol, BaseAddress, puValue);
        }
    }
    else
        rc = KLDR_ERR_TODO;

    return rc;
}


/**
 * Lookup a symbol in a 32-bit symbol table.
 *
 * @returns See kLdrModQuerySymbol.
 * @param   pModMachO
 * @param   paSyms      Pointer to the symbol table.
 * @param   cSyms       Number of symbols in the table.
 * @param   pchStrings  Pointer to the string table.
 * @param   cchStrings  Size of the string table.
 * @param   BaseAddress Adjusted base address, see kLdrModQuerySymbol.
 * @param   iSymbol     See kLdrModQuerySymbol.
 * @param   pchSymbol   See kLdrModQuerySymbol.
 * @param   cchSymbol   See kLdrModQuerySymbol.
 * @param   puValue     See kLdrModQuerySymbol.
 * @param   pfKind      See kLdrModQuerySymbol.
 */
static int kldrModMachODoQuerySymbol32Bit(PKLDRMODMACHO pModMachO, const macho_nlist_32_t *paSyms, KU32 cSyms,
                                          const char *pchStrings, KU32 cchStrings, KLDRADDR BaseAddress, KU32 iSymbol,
                                          const char *pchSymbol, KU32 cchSymbol, PKLDRADDR puValue, KU32 *pfKind)
{
    /*
     * Find a valid symbol matching the search criteria.
     */
    if (iSymbol == NIL_KLDRMOD_SYM_ORDINAL)
    {
        /* simplify validation. */
        if (cchStrings <= cchSymbol)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        cchStrings -= cchSymbol;

        /* external symbols are usually at the end, so search the other way. */
        for (iSymbol = cSyms - 1; iSymbol != KU32_MAX; iSymbol--)
        {
            const char *psz;

            /* Skip irrellevant and non-public symbols. */
            if (paSyms[iSymbol].n_type & MACHO_N_STAB)
                continue;
            if ((paSyms[iSymbol].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
                continue;
            if (!(paSyms[iSymbol].n_type & MACHO_N_EXT)) /*??*/
                continue;
            if (paSyms[iSymbol].n_type & MACHO_N_PEXT) /*??*/
                continue;

            /* get name */
            if (!paSyms[iSymbol].n_un.n_strx)
                continue;
            if ((KU32)paSyms[iSymbol].n_un.n_strx >= cchStrings)
                continue;
            psz = &pchStrings[paSyms[iSymbol].n_un.n_strx];
            if (psz[cchSymbol])
                continue;
            if (kHlpMemComp(psz, pchSymbol, cchSymbol))
                continue;

            /* match! */
            break;
        }
        if (iSymbol == KU32_MAX)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
    }
    else
    {
        if (iSymbol >= cSyms)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        if (paSyms[iSymbol].n_type & MACHO_N_STAB)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        if ((paSyms[iSymbol].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
    }

    /*
     * Calc the return values.
     */
    if (pfKind)
    {
        if (    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
            ||  pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
            *pfKind = KLDRSYMKIND_32BIT | KLDRSYMKIND_NO_TYPE;
        else
            *pfKind = KLDRSYMKIND_64BIT | KLDRSYMKIND_NO_TYPE;
        if (paSyms[iSymbol].n_desc & N_WEAK_DEF)
            *pfKind |= KLDRSYMKIND_WEAK;
    }

    switch (paSyms[iSymbol].n_type & MACHO_N_TYPE)
    {
        case MACHO_N_SECT:
        {
            PKLDRMODMACHOSECT pSect;
            KLDRADDR offSect;
            KLDRMODMACHO_CHECK_RETURN((KU32)(paSyms[iSymbol].n_sect - 1) < pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
            pSect = &pModMachO->paSections[paSyms[iSymbol].n_sect - 1];

            offSect = paSyms[iSymbol].n_value - pSect->LinkAddress;
            KLDRMODMACHO_CHECK_RETURN(   offSect <= pSect->cb
                                      || (   paSyms[iSymbol].n_sect == 1 /* special hack for __mh_execute_header */
                                          && offSect == 0U - pSect->RVA
                                          && pModMachO->uEffFileType != MH_OBJECT),
                                      KLDR_ERR_MACHO_BAD_SYMBOL);
            if (puValue)
                *puValue = BaseAddress + pSect->RVA + offSect;

            if (    pfKind
                &&  (pSect->fFlags & (S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SELF_MODIFYING_CODE)))
                *pfKind = (*pfKind & ~KLDRSYMKIND_TYPE_MASK) | KLDRSYMKIND_CODE;
            break;
        }

        case MACHO_N_ABS:
            if (puValue)
                *puValue = paSyms[iSymbol].n_value;
            /*if (pfKind)
                pfKind |= KLDRSYMKIND_ABS;*/
            break;

        case MACHO_N_PBUD:
        case MACHO_N_INDR:
            /** @todo implement indirect and prebound symbols. */
        default:
            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
    }

    return 0;
}


/**
 * Lookup a symbol in a 64-bit symbol table.
 *
 * @returns See kLdrModQuerySymbol.
 * @param   pModMachO
 * @param   paSyms      Pointer to the symbol table.
 * @param   cSyms       Number of symbols in the table.
 * @param   pchStrings  Pointer to the string table.
 * @param   cchStrings  Size of the string table.
 * @param   BaseAddress Adjusted base address, see kLdrModQuerySymbol.
 * @param   iSymbol     See kLdrModQuerySymbol.
 * @param   pchSymbol   See kLdrModQuerySymbol.
 * @param   cchSymbol   See kLdrModQuerySymbol.
 * @param   puValue     See kLdrModQuerySymbol.
 * @param   pfKind      See kLdrModQuerySymbol.
 */
static int kldrModMachODoQuerySymbol64Bit(PKLDRMODMACHO pModMachO, const macho_nlist_64_t *paSyms, KU32 cSyms,
                                          const char *pchStrings, KU32 cchStrings, KLDRADDR BaseAddress, KU32 iSymbol,
                                          const char *pchSymbol, KU32 cchSymbol, PKLDRADDR puValue, KU32 *pfKind)
{
    /*
     * Find a valid symbol matching the search criteria.
     */
    if (iSymbol == NIL_KLDRMOD_SYM_ORDINAL)
    {
        /* simplify validation. */
        if (cchStrings <= cchSymbol)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        cchStrings -= cchSymbol;

        /* external symbols are usually at the end, so search the other way. */
        for (iSymbol = cSyms - 1; iSymbol != KU32_MAX; iSymbol--)
        {
            const char *psz;

            /* Skip irrellevant and non-public symbols. */
            if (paSyms[iSymbol].n_type & MACHO_N_STAB)
                continue;
            if ((paSyms[iSymbol].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
                continue;
            if (!(paSyms[iSymbol].n_type & MACHO_N_EXT)) /*??*/
                continue;
            if (paSyms[iSymbol].n_type & MACHO_N_PEXT) /*??*/
                continue;

            /* get name */
            if (!paSyms[iSymbol].n_un.n_strx)
                continue;
            if ((KU32)paSyms[iSymbol].n_un.n_strx >= cchStrings)
                continue;
            psz = &pchStrings[paSyms[iSymbol].n_un.n_strx];
            if (psz[cchSymbol])
                continue;
            if (kHlpMemComp(psz, pchSymbol, cchSymbol))
                continue;

            /* match! */
            break;
        }
        if (iSymbol == KU32_MAX)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
    }
    else
    {
        if (iSymbol >= cSyms)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        if (paSyms[iSymbol].n_type & MACHO_N_STAB)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
        if ((paSyms[iSymbol].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
            return KLDR_ERR_SYMBOL_NOT_FOUND;
    }

    /*
     * Calc the return values.
     */
    if (pfKind)
    {
        if (    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
            ||  pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
            *pfKind = KLDRSYMKIND_32BIT | KLDRSYMKIND_NO_TYPE;
        else
            *pfKind = KLDRSYMKIND_64BIT | KLDRSYMKIND_NO_TYPE;
        if (paSyms[iSymbol].n_desc & N_WEAK_DEF)
            *pfKind |= KLDRSYMKIND_WEAK;
    }

    switch (paSyms[iSymbol].n_type & MACHO_N_TYPE)
    {
        case MACHO_N_SECT:
        {
            PKLDRMODMACHOSECT pSect;
            KLDRADDR offSect;
            KLDRMODMACHO_CHECK_RETURN((KU32)(paSyms[iSymbol].n_sect - 1) < pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
            pSect = &pModMachO->paSections[paSyms[iSymbol].n_sect - 1];

            offSect = paSyms[iSymbol].n_value - pSect->LinkAddress;
            KLDRMODMACHO_CHECK_RETURN(   offSect <= pSect->cb
                                      || (   paSyms[iSymbol].n_sect == 1 /* special hack for __mh_execute_header */
                                          && offSect == 0U - pSect->RVA
                                          && pModMachO->uEffFileType != MH_OBJECT),
                                      KLDR_ERR_MACHO_BAD_SYMBOL);
            if (puValue)
                *puValue = BaseAddress + pSect->RVA + offSect;

            if (    pfKind
                &&  (pSect->fFlags & (S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SELF_MODIFYING_CODE)))
                *pfKind = (*pfKind & ~KLDRSYMKIND_TYPE_MASK) | KLDRSYMKIND_CODE;
            break;
        }

        case MACHO_N_ABS:
            if (puValue)
                *puValue = paSyms[iSymbol].n_value;
            /*if (pfKind)
                pfKind |= KLDRSYMKIND_ABS;*/
            break;

        case MACHO_N_PBUD:
        case MACHO_N_INDR:
            /** @todo implement indirect and prebound symbols. */
        default:
            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
    }

    return 0;
}


/** @copydoc kLdrModEnumSymbols */
static int kldrModMachOEnumSymbols(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress,
                                   KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    int rc;
    K_NOREF(pvBits);

    /*
     * Resolve defaults.
     */
    rc  = kldrModMachOAdjustBaseAddress(pModMachO, &BaseAddress);
    if (rc)
        return rc;

    /*
     * Take action according to file type.
     */
    if (   pModMachO->Hdr.filetype == MH_OBJECT
        || pModMachO->Hdr.filetype == MH_EXECUTE /** @todo dylib, execute, dsym: symbols */
        || pModMachO->Hdr.filetype == MH_DYLIB
        || pModMachO->Hdr.filetype == MH_BUNDLE
        || pModMachO->Hdr.filetype == MH_DSYM
        || pModMachO->Hdr.filetype == MH_KEXT_BUNDLE)
    {
        rc = kldrModMachOLoadObjSymTab(pModMachO);
        if (!rc)
        {
            if (    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
                ||  pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
                rc = kldrModMachODoEnumSymbols32Bit(pModMachO, (macho_nlist_32_t *)pModMachO->pvaSymbols, pModMachO->cSymbols,
                                                    pModMachO->pchStrings, pModMachO->cchStrings, BaseAddress,
                                                    fFlags, pfnCallback, pvUser);
            else
                rc = kldrModMachODoEnumSymbols64Bit(pModMachO, (macho_nlist_64_t *)pModMachO->pvaSymbols, pModMachO->cSymbols,
                                                    pModMachO->pchStrings, pModMachO->cchStrings, BaseAddress,
                                                    fFlags, pfnCallback, pvUser);
        }
    }
    else
        KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);

    return rc;
}


/**
 * Enum a 32-bit symbol table.
 *
 * @returns See kLdrModQuerySymbol.
 * @param   pModMachO
 * @param   paSyms      Pointer to the symbol table.
 * @param   cSyms       Number of symbols in the table.
 * @param   pchStrings  Pointer to the string table.
 * @param   cchStrings  Size of the string table.
 * @param   BaseAddress Adjusted base address, see kLdrModEnumSymbols.
 * @param   fFlags      See kLdrModEnumSymbols.
 * @param   pfnCallback See kLdrModEnumSymbols.
 * @param   pvUser      See kLdrModEnumSymbols.
 */
static int kldrModMachODoEnumSymbols32Bit(PKLDRMODMACHO pModMachO, const macho_nlist_32_t *paSyms, KU32 cSyms,
                                          const char *pchStrings, KU32 cchStrings, KLDRADDR BaseAddress,
                                          KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser)
{
    const KU32 fKindBase =    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
                           || pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE
                         ? KLDRSYMKIND_32BIT : KLDRSYMKIND_64BIT;
    KU32 iSym;
    int rc;

    /*
     * Iterate the symbol table.
     */
    for (iSym = 0; iSym < cSyms; iSym++)
    {
        KU32 fKind;
        KLDRADDR uValue;
        const char *psz;
        KSIZE cch;

        /* Skip debug symbols and undefined symbols. */
        if (paSyms[iSym].n_type & MACHO_N_STAB)
            continue;
        if ((paSyms[iSym].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
            continue;

        /* Skip non-public symbols unless they are requested explicitly. */
        if (!(fFlags & KLDRMOD_ENUM_SYMS_FLAGS_ALL))
        {
            if (!(paSyms[iSym].n_type & MACHO_N_EXT)) /*??*/
                continue;
            if (paSyms[iSym].n_type & MACHO_N_PEXT) /*??*/
                continue;
            if (!paSyms[iSym].n_un.n_strx)
                continue;
        }

        /*
         * Gather symbol info
         */

        /* name */
        KLDRMODMACHO_CHECK_RETURN((KU32)paSyms[iSym].n_un.n_strx < cchStrings, KLDR_ERR_MACHO_BAD_SYMBOL);
        psz = &pchStrings[paSyms[iSym].n_un.n_strx];
        cch = kHlpStrLen(psz);
        if (!cch)
            psz = NULL;

        /* kind & value */
        fKind = fKindBase;
        if (paSyms[iSym].n_desc & N_WEAK_DEF)
            fKind |= KLDRSYMKIND_WEAK;
        switch (paSyms[iSym].n_type & MACHO_N_TYPE)
        {
            case MACHO_N_SECT:
            {
                PKLDRMODMACHOSECT pSect;
                KLDRMODMACHO_CHECK_RETURN((KU32)(paSyms[iSym].n_sect - 1) < pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                pSect = &pModMachO->paSections[paSyms[iSym].n_sect - 1];

                uValue = paSyms[iSym].n_value - pSect->LinkAddress;
                KLDRMODMACHO_CHECK_RETURN(   uValue <= pSect->cb
                                          || (   paSyms[iSym].n_sect == 1 /* special hack for __mh_execute_header */
                                              && uValue == 0U - pSect->RVA
                                              && pModMachO->uEffFileType != MH_OBJECT),
                                          KLDR_ERR_MACHO_BAD_SYMBOL);
                uValue += BaseAddress + pSect->RVA;

                if (pSect->fFlags & (S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SELF_MODIFYING_CODE))
                    fKind |= KLDRSYMKIND_CODE;
                else
                    fKind |= KLDRSYMKIND_NO_TYPE;
                break;
            }

            case MACHO_N_ABS:
                uValue = paSyms[iSym].n_value;
                fKind |= KLDRSYMKIND_NO_TYPE /*KLDRSYMKIND_ABS*/;
                break;

            case MACHO_N_PBUD:
            case MACHO_N_INDR:
                /** @todo implement indirect and prebound symbols. */
            default:
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
        }

        /*
         * Do callback.
         */
        rc = pfnCallback(pModMachO->pMod, iSym, psz, cch, NULL, uValue, fKind, pvUser);
        if (rc)
            return rc;
    }
    return 0;
}


/**
 * Enum a 64-bit symbol table.
 *
 * @returns See kLdrModQuerySymbol.
 * @param   pModMachO
 * @param   paSyms      Pointer to the symbol table.
 * @param   cSyms       Number of symbols in the table.
 * @param   pchStrings  Pointer to the string table.
 * @param   cchStrings  Size of the string table.
 * @param   BaseAddress Adjusted base address, see kLdrModEnumSymbols.
 * @param   fFlags      See kLdrModEnumSymbols.
 * @param   pfnCallback See kLdrModEnumSymbols.
 * @param   pvUser      See kLdrModEnumSymbols.
 */
static int kldrModMachODoEnumSymbols64Bit(PKLDRMODMACHO pModMachO, const macho_nlist_64_t *paSyms, KU32 cSyms,
                                          const char *pchStrings, KU32 cchStrings, KLDRADDR BaseAddress,
                                          KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser)
{
    const KU32 fKindBase =    pModMachO->Hdr.magic == IMAGE_MACHO64_SIGNATURE
                           || pModMachO->Hdr.magic == IMAGE_MACHO64_SIGNATURE_OE
                         ? KLDRSYMKIND_64BIT : KLDRSYMKIND_32BIT;
    KU32 iSym;
    int rc;

    /*
     * Iterate the symbol table.
     */
    for (iSym = 0; iSym < cSyms; iSym++)
    {
        KU32 fKind;
        KLDRADDR uValue;
        const char *psz;
        KSIZE cch;

        /* Skip debug symbols and undefined symbols. */
        if (paSyms[iSym].n_type & MACHO_N_STAB)
            continue;
        if ((paSyms[iSym].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
            continue;

        /* Skip non-public symbols unless they are requested explicitly. */
        if (!(fFlags & KLDRMOD_ENUM_SYMS_FLAGS_ALL))
        {
            if (!(paSyms[iSym].n_type & MACHO_N_EXT)) /*??*/
                continue;
            if (paSyms[iSym].n_type & MACHO_N_PEXT) /*??*/
                continue;
            if (!paSyms[iSym].n_un.n_strx)
                continue;
        }

        /*
         * Gather symbol info
         */

        /* name */
        KLDRMODMACHO_CHECK_RETURN((KU32)paSyms[iSym].n_un.n_strx < cchStrings, KLDR_ERR_MACHO_BAD_SYMBOL);
        psz = &pchStrings[paSyms[iSym].n_un.n_strx];
        cch = kHlpStrLen(psz);
        if (!cch)
            psz = NULL;

        /* kind & value */
        fKind = fKindBase;
        if (paSyms[iSym].n_desc & N_WEAK_DEF)
            fKind |= KLDRSYMKIND_WEAK;
        switch (paSyms[iSym].n_type & MACHO_N_TYPE)
        {
            case MACHO_N_SECT:
            {
                PKLDRMODMACHOSECT pSect;
                KLDRMODMACHO_CHECK_RETURN((KU32)(paSyms[iSym].n_sect - 1) < pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                pSect = &pModMachO->paSections[paSyms[iSym].n_sect - 1];

                uValue = paSyms[iSym].n_value - pSect->LinkAddress;
                KLDRMODMACHO_CHECK_RETURN(   uValue <= pSect->cb
                                          || (   paSyms[iSym].n_sect == 1 /* special hack for __mh_execute_header */
                                              && uValue == 0U - pSect->RVA
                                              && pModMachO->uEffFileType != MH_OBJECT),
                                          KLDR_ERR_MACHO_BAD_SYMBOL);
                uValue += BaseAddress + pSect->RVA;

                if (pSect->fFlags & (S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SELF_MODIFYING_CODE))
                    fKind |= KLDRSYMKIND_CODE;
                else
                    fKind |= KLDRSYMKIND_NO_TYPE;
                break;
            }

            case MACHO_N_ABS:
                uValue = paSyms[iSym].n_value;
                fKind |= KLDRSYMKIND_NO_TYPE /*KLDRSYMKIND_ABS*/;
                break;

            case MACHO_N_PBUD:
            case MACHO_N_INDR:
                /** @todo implement indirect and prebound symbols. */
            default:
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
        }

        /*
         * Do callback.
         */
        rc = pfnCallback(pModMachO->pMod, iSym, psz, cch, NULL, uValue, fKind, pvUser);
        if (rc)
            return rc;
    }
    return 0;
}


/** @copydoc kLdrModGetImport */
static int kldrModMachOGetImport(PKLDRMOD pMod, const void *pvBits, KU32 iImport, char *pszName, KSIZE cchName)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    K_NOREF(pvBits);
    K_NOREF(iImport);
    K_NOREF(pszName);
    K_NOREF(cchName);

    if (pModMachO->Hdr.filetype == MH_OBJECT)
        return KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS;

    /* later */
    return KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS;
}


/** @copydoc kLdrModNumberOfImports */
static KI32 kldrModMachONumberOfImports(PKLDRMOD pMod, const void *pvBits)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    K_NOREF(pvBits);

    if (pModMachO->Hdr.filetype == MH_OBJECT)
        return 0;

    /* later */
    return 0;
}


/** @copydoc kLdrModGetStackInfo */
static int kldrModMachOGetStackInfo(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRSTACKINFO pStackInfo)
{
    /*PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;*/
    K_NOREF(pMod);
    K_NOREF(pvBits);
    K_NOREF(BaseAddress);

    pStackInfo->Address = NIL_KLDRADDR;
    pStackInfo->LinkAddress = NIL_KLDRADDR;
    pStackInfo->cbStack = pStackInfo->cbStackThread = 0;
    /* later */

    return 0;
}


/** @copydoc kLdrModQueryMainEntrypoint */
static int kldrModMachOQueryMainEntrypoint(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRADDR pMainEPAddress)
{
#if 0
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    int rc;

    /*
     * Resolve base address alias if any.
     */
    rc = kldrModMachOBitsAndBaseAddress(pModMachO, NULL, &BaseAddress);
    if (rc)
        return rc;

    /*
     * Convert the address from the header.
     */
    *pMainEPAddress = pModMachO->Hdrs.OptionalHeader.AddressOfEntryPoint
        ? BaseAddress + pModMachO->Hdrs.OptionalHeader.AddressOfEntryPoint
        : NIL_KLDRADDR;
#else
    *pMainEPAddress = NIL_KLDRADDR;
    K_NOREF(pvBits);
    K_NOREF(BaseAddress);
    K_NOREF(pMod);
#endif
    return 0;
}


/** @copydoc kLdrModQueryImageUuid */
static int kldrModMachOQueryImageUuid(PKLDRMOD pMod, const void *pvBits, void *pvUuid, KSIZE cbUuid)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    K_NOREF(pvBits);

    kHlpMemSet(pvUuid, 0, cbUuid);
    if (kHlpMemComp(pvUuid, pModMachO->abImageUuid, sizeof(pModMachO->abImageUuid)) == 0)
        return KLDR_ERR_NO_IMAGE_UUID;

    kHlpMemCopy(pvUuid, pModMachO->abImageUuid, sizeof(pModMachO->abImageUuid));
    return 0;
}


/** @copydoc kLdrModEnumDbgInfo */
static int kldrModMachOEnumDbgInfo(PKLDRMOD pMod, const void *pvBits, PFNKLDRENUMDBG pfnCallback, void *pvUser)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    int rc = 0;
    KU32 iSect;
    K_NOREF(pvBits);

    for (iSect = 0; iSect < pModMachO->cSections; iSect++)
    {
        section_32_t *pMachOSect = pModMachO->paSections[iSect].pvMachoSection; /* (32-bit & 64-bit starts the same way) */
        char          szTmp[sizeof(pMachOSect->sectname) + 1];

        if (kHlpStrComp(pMachOSect->segname, "__DWARF"))
            continue;

        kHlpMemCopy(szTmp, pMachOSect->sectname, sizeof(pMachOSect->sectname));
        szTmp[sizeof(pMachOSect->sectname)] = '\0';

        rc = pfnCallback(pMod, iSect, KLDRDBGINFOTYPE_DWARF, 0, 0, szTmp,
                         pModMachO->paSections[iSect].offFile,
                         pModMachO->paSections[iSect].LinkAddress,
                         pModMachO->paSections[iSect].cb,
                         NULL, pvUser);
        if (rc != 0)
            break;
    }

    return rc;
}


/** @copydoc kLdrModHasDbgInfo */
static int kldrModMachOHasDbgInfo(PKLDRMOD pMod, const void *pvBits)
{
    /*PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;*/

#if 0
    /*
     * Base this entirely on the presence of a debug directory.
     */
    if (    pModMachO->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size
            < sizeof(IMAGE_DEBUG_DIRECTORY) /* screw borland linkers */
        ||  !pModMachO->Hdrs.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress)
        return KLDR_ERR_NO_DEBUG_INFO;
    return 0;
#else
    K_NOREF(pMod);
    K_NOREF(pvBits);
    return KLDR_ERR_NO_DEBUG_INFO;
#endif
}


/** @copydoc kLdrModMap */
static int kldrModMachOMap(PKLDRMOD pMod)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    unsigned fFixed;
    KU32 i;
    void *pvBase;
    int rc;

    if (!pModMachO->fCanLoad)
        return KLDR_ERR_TODO;

    /*
     * Already mapped?
     */
    if (pModMachO->pvMapping)
        return KLDR_ERR_ALREADY_MAPPED;

    /*
     * Map it.
     */
    /* fixed image? */
    fFixed = pMod->enmType == KLDRTYPE_EXECUTABLE_FIXED
          || pMod->enmType == KLDRTYPE_SHARED_LIBRARY_FIXED;
    if (!fFixed)
        pvBase = NULL;
    else
    {
        pvBase = (void *)(KUPTR)pMod->aSegments[0].LinkAddress;
        if ((KUPTR)pvBase != pMod->aSegments[0].LinkAddress)
            return KLDR_ERR_ADDRESS_OVERFLOW;
    }

    /* try do the prepare */
    rc = kRdrMap(pMod->pRdr, &pvBase, pMod->cSegments, pMod->aSegments, fFixed);
    if (rc)
        return rc;

    /*
     * Update the segments with their map addresses.
     */
    for (i = 0; i < pMod->cSegments; i++)
    {
        if (pMod->aSegments[i].RVA != NIL_KLDRADDR)
            pMod->aSegments[i].MapAddress = (KUPTR)pvBase + (KUPTR)pMod->aSegments[i].RVA;
    }
    pModMachO->pvMapping = pvBase;

    return 0;
}


/** @copydoc kLdrModUnmap */
static int kldrModMachOUnmap(PKLDRMOD pMod)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    KU32 i;
    int rc;

    /*
     * Mapped?
     */
    if (!pModMachO->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /*
     * Try unmap the image.
     */
    rc = kRdrUnmap(pMod->pRdr, pModMachO->pvMapping, pMod->cSegments, pMod->aSegments);
    if (rc)
        return rc;

    /*
     * Update the segments to reflect that they aren't mapped any longer.
     */
    pModMachO->pvMapping = NULL;
    for (i = 0; i < pMod->cSegments; i++)
        pMod->aSegments[i].MapAddress = 0;

    return 0;
}


/** @copydoc kLdrModAllocTLS */
static int kldrModMachOAllocTLS(PKLDRMOD pMod, void *pvMapping)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;

    /*
     * Mapped?
     */
    if (   pvMapping == KLDRMOD_INT_MAP
        && !pModMachO->pvMapping )
        return KLDR_ERR_NOT_MAPPED;
    return 0;
}


/** @copydoc kLdrModFreeTLS */
static void kldrModMachOFreeTLS(PKLDRMOD pMod, void *pvMapping)
{
    K_NOREF(pMod);
    K_NOREF(pvMapping);
}


/** @copydoc kLdrModReload */
static int kldrModMachOReload(PKLDRMOD pMod)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;

    /*
     * Mapped?
     */
    if (!pModMachO->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /* the file provider does it all */
    return kRdrRefresh(pMod->pRdr, pModMachO->pvMapping, pMod->cSegments, pMod->aSegments);
}


/** @copydoc kLdrModFixupMapping */
static int kldrModMachOFixupMapping(PKLDRMOD pMod, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    int rc, rc2;

    /*
     * Mapped?
     */
    if (!pModMachO->pvMapping)
        return KLDR_ERR_NOT_MAPPED;

    /*
     * Before doing anything we'll have to make all pages writable.
     */
    rc = kRdrProtect(pMod->pRdr, pModMachO->pvMapping, pMod->cSegments, pMod->aSegments, 1 /* unprotect */);
    if (rc)
        return rc;

    /*
     * Resolve imports and apply base relocations.
     */
    rc = kldrModMachORelocateBits(pMod, pModMachO->pvMapping, (KUPTR)pModMachO->pvMapping, pModMachO->LinkAddress,
                                  pfnGetImport, pvUser);

    /*
     * Restore protection.
     */
    rc2 = kRdrProtect(pMod->pRdr, pModMachO->pvMapping, pMod->cSegments, pMod->aSegments, 0 /* protect */);
    if (!rc && rc2)
        rc = rc2;
    return rc;
}


/**
 * MH_OBJECT: Resolves undefined symbols (imports).
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModMachO       The Mach-O module interpreter instance.
 * @param   pfnGetImport    The callback for resolving an imported symbol.
 * @param   pvUser          User argument to the callback.
 */
static int  kldrModMachOObjDoImports(PKLDRMODMACHO pModMachO, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    const KU32 cSyms = pModMachO->cSymbols;
    KU32 iSym;
    int rc;

    /*
     * Ensure that we've got the symbol table and section fixups handy.
     */
    rc = kldrModMachOLoadObjSymTab(pModMachO);
    if (rc)
        return rc;

    /*
     * Iterate the symbol table and resolve undefined symbols.
     * We currently ignore REFERENCE_TYPE.
     */
    if (    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
        ||  pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
    {
        macho_nlist_32_t *paSyms = (macho_nlist_32_t *)pModMachO->pvaSymbols;
        for (iSym = 0; iSym < cSyms; iSym++)
        {
            /* skip stabs */
            if (paSyms[iSym].n_type & MACHO_N_STAB)
                continue;

            if ((paSyms[iSym].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
            {
                const char *pszSymbol;
                KSIZE cchSymbol;
                KU32 fKind = KLDRSYMKIND_REQ_FLAT;
                KLDRADDR Value = NIL_KLDRADDR;

                /** @todo Implement N_REF_TO_WEAK. */
                KLDRMODMACHO_CHECK_RETURN(!(paSyms[iSym].n_desc & N_REF_TO_WEAK), KLDR_ERR_TODO);

                /* Get the symbol name. */
                KLDRMODMACHO_CHECK_RETURN((KU32)paSyms[iSym].n_un.n_strx < pModMachO->cchStrings, KLDR_ERR_MACHO_BAD_SYMBOL);
                pszSymbol = &pModMachO->pchStrings[paSyms[iSym].n_un.n_strx];
                cchSymbol = kHlpStrLen(pszSymbol);

                /* Check for linker defined symbols relating to sections and segments. */
                if (   cchSymbol > sizeof("section$end$") - 1
                    && *pszSymbol == 's'
                    && kHlpMemChr(pszSymbol, '$', cchSymbol))
                    rc = kldrModMachOQueryLinkerSymbol(pModMachO, pModMachO->pMod, pszSymbol, cchSymbol, BaseAddress, &Value);
                else
                    rc = KLDR_ERR_SYMBOL_NOT_FOUND;

                /* Ask the user for an address to the symbol. */
                if (rc)
                    rc = pfnGetImport(pModMachO->pMod, NIL_KLDRMOD_IMPORT, iSym, pszSymbol, cchSymbol, NULL,
                                      &Value, &fKind, pvUser);
                if (rc)
                {
                    /* weak reference? */
                    if (!(paSyms[iSym].n_desc & N_WEAK_REF))
                        break;
                    Value = 0;
                }

                /* Update the symbol. */
                paSyms[iSym].n_value = (KU32)Value;
                if (paSyms[iSym].n_value != Value)
                {
                    rc = KLDR_ERR_ADDRESS_OVERFLOW;
                    break;
                }
            }
            else if (paSyms[iSym].n_desc & N_WEAK_DEF)
            {
                /** @todo implement weak symbols. */
                /*return KLDR_ERR_TODO; - ignored for now. */
            }
        }
    }
    else
    {
        /* (Identical to the 32-bit code, just different paSym type. (and n_strx is unsigned)) */
        macho_nlist_64_t *paSyms = (macho_nlist_64_t *)pModMachO->pvaSymbols;
        for (iSym = 0; iSym < cSyms; iSym++)
        {
            /* skip stabs */
            if (paSyms[iSym].n_type & MACHO_N_STAB)
                continue;

            if ((paSyms[iSym].n_type & MACHO_N_TYPE) == MACHO_N_UNDF)
            {
                const char *pszSymbol;
                KSIZE cchSymbol;
                KU32 fKind = KLDRSYMKIND_REQ_FLAT;
                KLDRADDR Value = NIL_KLDRADDR;

                /** @todo Implement N_REF_TO_WEAK. */
                KLDRMODMACHO_CHECK_RETURN(!(paSyms[iSym].n_desc & N_REF_TO_WEAK), KLDR_ERR_TODO);

                 /* Get the symbol name. */
                KLDRMODMACHO_CHECK_RETURN(paSyms[iSym].n_un.n_strx < pModMachO->cchStrings, KLDR_ERR_MACHO_BAD_SYMBOL);
                pszSymbol = &pModMachO->pchStrings[paSyms[iSym].n_un.n_strx];
                cchSymbol = kHlpStrLen(pszSymbol);

                /* Check for linker defined symbols relating to sections and segments. */
                if (   cchSymbol > sizeof("section$end$") - 1
                    && *pszSymbol == 's'
                    && kHlpMemChr(pszSymbol, '$', cchSymbol))
                    rc = kldrModMachOQueryLinkerSymbol(pModMachO, pModMachO->pMod, pszSymbol, cchSymbol, BaseAddress, &Value);
                else
                    rc = KLDR_ERR_SYMBOL_NOT_FOUND;

                /* Ask the user for an address to the symbol. */
                if (rc)
                    rc = pfnGetImport(pModMachO->pMod, NIL_KLDRMOD_IMPORT, iSym, pszSymbol, cchSymbol, NULL,
                                      &Value, &fKind, pvUser);
                if (rc)
                {
                    /* weak reference? */
                    if (!(paSyms[iSym].n_desc & N_WEAK_REF))
                        break;
                    Value = 0;
                }

                /* Update the symbol. */
                paSyms[iSym].n_value = Value;
                if (paSyms[iSym].n_value != Value)
                {
                    rc = KLDR_ERR_ADDRESS_OVERFLOW;
                    break;
                }
            }
            else if (paSyms[iSym].n_desc & N_WEAK_DEF)
            {
                /** @todo implement weak symbols. */
                /*return KLDR_ERR_TODO; - ignored for now. */
            }
        }
    }

    return rc;
}


/**
 * MH_OBJECT: Applies base relocations to a (unprotected) image mapping.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModMachO       The Mach-O module interpreter instance.
 * @param   pvMapping       The mapping to fixup.
 * @param   NewBaseAddress  The address to fixup the mapping to.
 * @param   OldBaseAddress  The address the mapping is currently fixed up to.
 */
static int  kldrModMachOObjDoFixups(PKLDRMODMACHO pModMachO, void *pvMapping, KLDRADDR NewBaseAddress)
{
    KU32 iSeg;
    int rc;


    /*
     * Ensure that we've got the symbol table and section fixups handy.
     */
    rc = kldrModMachOLoadObjSymTab(pModMachO);
    if (rc)
        return rc;

    /*
     * Iterate over the segments and their sections and apply fixups.
     */
    for (iSeg = rc = 0; !rc && iSeg < pModMachO->pMod->cSegments; iSeg++)
    {
        PKLDRMODMACHOSEG pSeg = &pModMachO->aSegments[iSeg];
        KU32 iSect;

        for (iSect = 0; iSect < pSeg->cSections; iSect++)
        {
            PKLDRMODMACHOSECT pSect = &pSeg->paSections[iSect];
            KU8 *pbSectBits;

            /* skip sections without fixups. */
            if (!pSect->cFixups)
                continue;

            /* lazy load (and endian convert) the fixups. */
            if (!pSect->paFixups)
            {
                rc = kldrModMachOLoadFixups(pModMachO, pSect->offFixups, pSect->cFixups, &pSect->paFixups);
                if (rc)
                    break;
            }

            /*
             * Apply the fixups.
             */
            pbSectBits = (KU8 *)pvMapping + (KUPTR)pSect->RVA;
            if (pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE) /** @todo this aint right. */
                rc = kldrModMachOFixupSectionGeneric32Bit(pModMachO, pbSectBits, pSect,
                                                          (macho_nlist_32_t *)pModMachO->pvaSymbols,
                                                          pModMachO->cSymbols, NewBaseAddress);
            else if (   pModMachO->Hdr.magic == IMAGE_MACHO64_SIGNATURE
                     && pModMachO->Hdr.cputype == CPU_TYPE_X86_64)
                rc = kldrModMachOFixupSectionAMD64(pModMachO, pbSectBits, pSect,
                                                   (macho_nlist_64_t *)pModMachO->pvaSymbols,
                                                   pModMachO->cSymbols, NewBaseAddress);
            else
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
            if (rc)
                break;
        }
    }

    return rc;
}


/**
 * Applies generic fixups to a section in an image of the same endian-ness
 * as the host CPU.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModMachO       The Mach-O module interpreter instance.
 * @param   pbSectBits      Pointer to the section bits.
 * @param   pFixupSect      The section being fixed up.
 * @param   NewBaseAddress  The new base image address.
 */
static int  kldrModMachOFixupSectionGeneric32Bit(PKLDRMODMACHO pModMachO, KU8 *pbSectBits, PKLDRMODMACHOSECT pFixupSect,
                                                 macho_nlist_32_t *paSyms, KU32 cSyms, KLDRADDR NewBaseAddress)
{
    const macho_relocation_info_t *paFixups = pFixupSect->paFixups;
    const KU32 cFixups = pFixupSect->cFixups;
    KSIZE cbSectBits = (KSIZE)pFixupSect->cb;
    const KU8 *pbSectVirginBits;
    KU32 iFixup;
    KLDRPU uFixVirgin;
    KLDRPU uFix;
    KLDRADDR SymAddr = ~(KLDRADDR)0;
    int rc;

    /*
     * Find the virgin bits.
     */
    if (pFixupSect->offFile != -1)
    {
        rc = kldrModMachOMapVirginBits(pModMachO);
        if (rc)
            return rc;
        pbSectVirginBits = (const KU8 *)pModMachO->pvBits + pFixupSect->offFile;
    }
    else
        pbSectVirginBits = NULL;

    /*
     * Iterate the fixups and apply them.
     */
    for (iFixup = 0; iFixup < cFixups; iFixup++)
    {
        union
        {
            macho_relocation_info_t     r;
            scattered_relocation_info_t s;
        } Fixup;
        Fixup.r = paFixups[iFixup];

        if (!(Fixup.r.r_address & R_SCATTERED))
        {
            /* sanity */
            if ((KU32)Fixup.r.r_address >= cbSectBits)
                return KLDR_ERR_BAD_FIXUP;

            /* calc fixup addresses. */
            uFix.pv = pbSectBits + Fixup.r.r_address;
            uFixVirgin.pv = pbSectVirginBits ? (KU8 *)pbSectVirginBits + Fixup.r.r_address : 0;

            /*
             * Calc the symbol value.
             */
            /* Calc the linked symbol address / addend. */
            switch (Fixup.r.r_length)
            {
                /** @todo Deal with unaligned accesses on non x86 platforms. */
                case 0: SymAddr = *uFixVirgin.pi8; break;
                case 1: SymAddr = *uFixVirgin.pi16; break;
                case 2: SymAddr = *uFixVirgin.pi32; break;
                case 3: SymAddr = *uFixVirgin.pi64; break;
            }
            if (Fixup.r.r_pcrel)
                SymAddr += Fixup.r.r_address + pFixupSect->LinkAddress;

            /* Add symbol / section address. */
            if (Fixup.r.r_extern)
            {
                const macho_nlist_32_t *pSym;
                if (Fixup.r.r_symbolnum >= cSyms)
                    return KLDR_ERR_BAD_FIXUP;
                pSym = &paSyms[Fixup.r.r_symbolnum];

                if (pSym->n_type & MACHO_N_STAB)
                    return KLDR_ERR_BAD_FIXUP;

                switch (pSym->n_type & MACHO_N_TYPE)
                {
                    case MACHO_N_SECT:
                    {
                        PKLDRMODMACHOSECT pSymSect;
                        KLDRMODMACHO_CHECK_RETURN((KU32)pSym->n_sect - 1 <= pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                        pSymSect = &pModMachO->paSections[pSym->n_sect - 1];

                        SymAddr += pSym->n_value - pSymSect->LinkAddress + pSymSect->RVA + NewBaseAddress;
                        break;
                    }

                    case MACHO_N_UNDF:
                    case MACHO_N_ABS:
                        SymAddr += pSym->n_value;
                        break;

                    case MACHO_N_INDR:
                    case MACHO_N_PBUD:
                        KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
                    default:
                        KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_SYMBOL);
                }
            }
            else if (Fixup.r.r_symbolnum != R_ABS)
            {
                PKLDRMODMACHOSECT pSymSect;
                if (Fixup.r.r_symbolnum > pModMachO->cSections)
                    return KLDR_ERR_BAD_FIXUP;
                pSymSect = &pModMachO->paSections[Fixup.r.r_symbolnum - 1];

                SymAddr -= pSymSect->LinkAddress;
                SymAddr += pSymSect->RVA + NewBaseAddress;
            }

            /* adjust for PC relative */
            if (Fixup.r.r_pcrel)
                SymAddr -= Fixup.r.r_address + pFixupSect->RVA + NewBaseAddress;
        }
        else
        {
            PKLDRMODMACHOSECT pSymSect;
            KU32 iSymSect;
            KLDRADDR Value;

            /* sanity */
            KLDRMODMACHO_ASSERT(Fixup.s.r_scattered);
            if ((KU32)Fixup.s.r_address >= cbSectBits)
                return KLDR_ERR_BAD_FIXUP;

            /* calc fixup addresses. */
            uFix.pv = pbSectBits + Fixup.s.r_address;
            uFixVirgin.pv = pbSectVirginBits ? (KU8 *)pbSectVirginBits + Fixup.s.r_address : 0;

            /*
             * Calc the symbol value.
             */
            /* The addend is stored in the code. */
            switch (Fixup.s.r_length)
            {
                case 0: SymAddr = *uFixVirgin.pi8; break;
                case 1: SymAddr = *uFixVirgin.pi16; break;
                case 2: SymAddr = *uFixVirgin.pi32; break;
                case 3: SymAddr = *uFixVirgin.pi64; break;
            }
            if (Fixup.s.r_pcrel)
                SymAddr += Fixup.s.r_address;
            Value = Fixup.s.r_value;
            SymAddr -= Value;                   /* (-> addend only) */

            /* Find the section number from the r_value. */
            pSymSect = NULL;
            for (iSymSect = 0; iSymSect < pModMachO->cSections; iSymSect++)
            {
                KLDRADDR off = Value - pModMachO->paSections[iSymSect].LinkAddress;
                if (off < pModMachO->paSections[iSymSect].cb)
                {
                    pSymSect = &pModMachO->paSections[iSymSect];
                    break;
                }
                else if (off == pModMachO->paSections[iSymSect].cb) /* edge case */
                    pSymSect = &pModMachO->paSections[iSymSect];
            }
            if (!pSymSect)
                return KLDR_ERR_BAD_FIXUP;

            /* Calc the symbol address. */
            SymAddr += Value - pSymSect->LinkAddress + pSymSect->RVA + NewBaseAddress;
            if (Fixup.s.r_pcrel)
                SymAddr -= Fixup.s.r_address + pFixupSect->RVA + NewBaseAddress;

            Fixup.r.r_length = ((scattered_relocation_info_t *)&paFixups[iFixup])->r_length;
            Fixup.r.r_type   = ((scattered_relocation_info_t *)&paFixups[iFixup])->r_type;
        }

        /*
         * Write back the fixed up value.
         */
        if (Fixup.r.r_type == GENERIC_RELOC_VANILLA)
        {
            switch (Fixup.r.r_length)
            {
                case 0: *uFix.pu8  = (KU8)SymAddr; break;
                case 1: *uFix.pu16 = (KU16)SymAddr; break;
                case 2: *uFix.pu32 = (KU32)SymAddr; break;
                case 3: *uFix.pu64 = (KU64)SymAddr; break;
            }
        }
        else if (Fixup.r.r_type <= GENERIC_RELOC_LOCAL_SECTDIFF)
            return KLDR_ERR_MACHO_UNSUPPORTED_FIXUP_TYPE;
        else
            return KLDR_ERR_BAD_FIXUP;
    }

    return 0;
}


/**
 * Applies AMD64 fixups to a section.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModMachO       The Mach-O module interpreter instance.
 * @param   pbSectBits      Pointer to the section bits.
 * @param   pFixupSect      The section being fixed up.
 * @param   NewBaseAddress  The new base image address.
 */
static int  kldrModMachOFixupSectionAMD64(PKLDRMODMACHO pModMachO, KU8 *pbSectBits, PKLDRMODMACHOSECT pFixupSect,
                                          macho_nlist_64_t *paSyms, KU32 cSyms, KLDRADDR NewBaseAddress)
{
    const macho_relocation_info_t *paFixups = pFixupSect->paFixups;
    const KU32 cFixups = pFixupSect->cFixups;
    KSIZE cbSectBits = (KSIZE)pFixupSect->cb;
    const KU8 *pbSectVirginBits;
    KU32 iFixup;
    KLDRPU uFixVirgin;
    KLDRPU uFix;
    KLDRADDR SymAddr;
    int rc;

    /*
     * Find the virgin bits.
     */
    if (pFixupSect->offFile != -1)
    {
        rc = kldrModMachOMapVirginBits(pModMachO);
        if (rc)
            return rc;
        pbSectVirginBits = (const KU8 *)pModMachO->pvBits + pFixupSect->offFile;
    }
    else
        pbSectVirginBits = NULL;

    /*
     * Iterate the fixups and apply them.
     */
    for (iFixup = 0; iFixup < cFixups; iFixup++)
    {
        union
        {
            macho_relocation_info_t     r;
            scattered_relocation_info_t s;
        } Fixup;
        Fixup.r = paFixups[iFixup];

        /* AMD64 doesn't use scattered fixups. */
        KLDRMODMACHO_CHECK_RETURN(!(Fixup.r.r_address & R_SCATTERED), KLDR_ERR_BAD_FIXUP);

        /* sanity */
        KLDRMODMACHO_CHECK_RETURN((KU32)Fixup.r.r_address < cbSectBits, KLDR_ERR_BAD_FIXUP);

        /* calc fixup addresses. */
        uFix.pv = pbSectBits + Fixup.r.r_address;
        uFixVirgin.pv = pbSectVirginBits ? (KU8 *)pbSectVirginBits + Fixup.r.r_address : 0;

        /*
         * Calc the symbol value.
         */
        /* Calc the linked symbol address / addend. */
        switch (Fixup.r.r_length)
        {
            /** @todo Deal with unaligned accesses on non x86 platforms. */
            case 2: SymAddr = *uFixVirgin.pi32; break;
            case 3: SymAddr = *uFixVirgin.pi64; break;
            default:
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_BAD_FIXUP);
        }

        /* Add symbol / section address. */
        if (Fixup.r.r_extern)
        {
            const macho_nlist_64_t *pSym;

            KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_symbolnum < cSyms, KLDR_ERR_BAD_FIXUP);
            pSym = &paSyms[Fixup.r.r_symbolnum];
            KLDRMODMACHO_CHECK_RETURN(!(pSym->n_type & MACHO_N_STAB), KLDR_ERR_BAD_FIXUP);

            switch (Fixup.r.r_type)
            {
                /* GOT references just needs to have their symbol verified.
                   Later, we'll optimize GOT building here using a parallel sym->got array. */
                case X86_64_RELOC_GOT_LOAD:
                case X86_64_RELOC_GOT:
                    switch (pSym->n_type & MACHO_N_TYPE)
                    {
                        case MACHO_N_SECT:
                        case MACHO_N_UNDF:
                        case MACHO_N_ABS:
                            break;
                        case MACHO_N_INDR:
                        case MACHO_N_PBUD:
                            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
                        default:
                            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_SYMBOL);
                    }
                    SymAddr = sizeof(KU64) * Fixup.r.r_symbolnum + pModMachO->GotRVA + NewBaseAddress;
                    KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_length == 2, KLDR_ERR_BAD_FIXUP);
                    SymAddr -= 4;
                    break;

                /* Verify the r_pcrel field for signed fixups on the way into the default case. */
                case X86_64_RELOC_BRANCH:
                case X86_64_RELOC_SIGNED:
                case X86_64_RELOC_SIGNED_1:
                case X86_64_RELOC_SIGNED_2:
                case X86_64_RELOC_SIGNED_4:
                    KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_pcrel, KLDR_ERR_BAD_FIXUP);
                    /* Falls through. */
                default:
                {
                    /* Adjust with fixup specific addend and vierfy unsigned/r_pcrel. */
                    switch (Fixup.r.r_type)
                    {
                        case X86_64_RELOC_UNSIGNED:
                            KLDRMODMACHO_CHECK_RETURN(!Fixup.r.r_pcrel, KLDR_ERR_BAD_FIXUP);
                            break;
                        case X86_64_RELOC_BRANCH:
                            KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_length == 2, KLDR_ERR_BAD_FIXUP);
                            SymAddr -= 4;
                            break;
                        case X86_64_RELOC_SIGNED:
                        case X86_64_RELOC_SIGNED_1:
                        case X86_64_RELOC_SIGNED_2:
                        case X86_64_RELOC_SIGNED_4:
                            SymAddr -= 4;
                            break;
                        default:
                            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_BAD_FIXUP);
                    }

                    switch (pSym->n_type & MACHO_N_TYPE)
                    {
                        case MACHO_N_SECT:
                        {
                            PKLDRMODMACHOSECT pSymSect;
                            KLDRMODMACHO_CHECK_RETURN((KU32)pSym->n_sect - 1 <= pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                            pSymSect = &pModMachO->paSections[pSym->n_sect - 1];
                            SymAddr += pSym->n_value - pSymSect->LinkAddress + pSymSect->RVA + NewBaseAddress;
                            break;
                        }

                        case MACHO_N_UNDF:
                            /* branch to an external symbol may have to take a short detour. */
                            if (   Fixup.r.r_type == X86_64_RELOC_BRANCH
                                &&       SymAddr + Fixup.r.r_address + pFixupSect->RVA + NewBaseAddress
                                       - pSym->n_value
                                       + KU64_C(0x80000000)
                                    >= KU64_C(0xffffff20))
                                SymAddr += pModMachO->cbJmpStub * Fixup.r.r_symbolnum + pModMachO->JmpStubsRVA + NewBaseAddress;
                            else
                                SymAddr += pSym->n_value;
                            break;

                        case MACHO_N_ABS:
                            SymAddr += pSym->n_value;
                            break;

                        case MACHO_N_INDR:
                        case MACHO_N_PBUD:
                            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
                        default:
                            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_SYMBOL);
                    }
                    break;
                }

                /*
                 * This is a weird customer, it will always be follows by an UNSIGNED fixup.
                 */
                case X86_64_RELOC_SUBTRACTOR:
                {
                    macho_relocation_info_t Fixup2;

                    /* Deal with the SUBTRACT symbol first, by subtracting it from SymAddr. */
                    switch (pSym->n_type & MACHO_N_TYPE)
                    {
                        case MACHO_N_SECT:
                        {
                            PKLDRMODMACHOSECT pSymSect;
                            KLDRMODMACHO_CHECK_RETURN((KU32)pSym->n_sect - 1 <= pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                            pSymSect = &pModMachO->paSections[pSym->n_sect - 1];
                            SymAddr -= pSym->n_value - pSymSect->LinkAddress + pSymSect->RVA + NewBaseAddress;
                            break;
                        }

                        case MACHO_N_UNDF:
                        case MACHO_N_ABS:
                            SymAddr -= pSym->n_value;
                            break;

                        case MACHO_N_INDR:
                        case MACHO_N_PBUD:
                            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
                        default:
                            KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_SYMBOL);
                    }

                    /* Load the 2nd fixup, check sanity. */
                    iFixup++;
                    KLDRMODMACHO_CHECK_RETURN(!Fixup.r.r_pcrel && iFixup < cFixups, KLDR_ERR_BAD_FIXUP);
                    Fixup2 = paFixups[iFixup];
                    KLDRMODMACHO_CHECK_RETURN(   Fixup2.r_address == Fixup.r.r_address
                                              && Fixup2.r_length == Fixup.r.r_length
                                              && Fixup2.r_type == X86_64_RELOC_UNSIGNED
                                              && !Fixup2.r_pcrel
                                              && Fixup2.r_symbolnum < cSyms,
                                              KLDR_ERR_BAD_FIXUP);

                    if (Fixup2.r_extern)
                    {
                        KLDRMODMACHO_CHECK_RETURN(Fixup2.r_symbolnum < cSyms, KLDR_ERR_BAD_FIXUP);
                        pSym = &paSyms[Fixup2.r_symbolnum];
                        KLDRMODMACHO_CHECK_RETURN(!(pSym->n_type & MACHO_N_STAB), KLDR_ERR_BAD_FIXUP);

                        /* Add it's value to SymAddr. */
                        switch (pSym->n_type & MACHO_N_TYPE)
                        {
                            case MACHO_N_SECT:
                            {
                                PKLDRMODMACHOSECT pSymSect;
                                KLDRMODMACHO_CHECK_RETURN((KU32)pSym->n_sect - 1 <= pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                                pSymSect = &pModMachO->paSections[pSym->n_sect - 1];
                                SymAddr += pSym->n_value - pSymSect->LinkAddress + pSymSect->RVA + NewBaseAddress;
                                break;
                            }

                            case MACHO_N_UNDF:
                            case MACHO_N_ABS:
                                SymAddr += pSym->n_value;
                                break;

                            case MACHO_N_INDR:
                            case MACHO_N_PBUD:
                                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
                            default:
                                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_MACHO_BAD_SYMBOL);
                        }
                    }
                    else if (Fixup2.r_symbolnum != R_ABS)
                    {
                        PKLDRMODMACHOSECT pSymSect;
                        KLDRMODMACHO_CHECK_RETURN(Fixup2.r_symbolnum <= pModMachO->cSections, KLDR_ERR_BAD_FIXUP);
                        pSymSect = &pModMachO->paSections[Fixup2.r_symbolnum - 1];
                        SymAddr += pSymSect->RVA + NewBaseAddress;
                    }
                    else
                        KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_BAD_FIXUP);
                }
                break;
            }
        }
        else
        {
            /* verify against fixup type and make adjustments */
            switch (Fixup.r.r_type)
            {
                case X86_64_RELOC_UNSIGNED:
                    KLDRMODMACHO_CHECK_RETURN(!Fixup.r.r_pcrel, KLDR_ERR_BAD_FIXUP);
                    break;
                case X86_64_RELOC_BRANCH:
                    KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_pcrel, KLDR_ERR_BAD_FIXUP);
                    SymAddr += 4; /* dunno what the assmbler/linker really is doing here... */
                    break;
                case X86_64_RELOC_SIGNED:
                case X86_64_RELOC_SIGNED_1:
                case X86_64_RELOC_SIGNED_2:
                case X86_64_RELOC_SIGNED_4:
                    KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_pcrel, KLDR_ERR_BAD_FIXUP);
                    break;
                /*case X86_64_RELOC_GOT_LOAD:*/
                /*case X86_64_RELOC_GOT: */
                /*case X86_64_RELOC_SUBTRACTOR: - must be r_extern=1 says as. */
                default:
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_BAD_FIXUP);
            }
            if (Fixup.r.r_symbolnum != R_ABS)
            {
                PKLDRMODMACHOSECT pSymSect;
                KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_symbolnum <= pModMachO->cSections, KLDR_ERR_BAD_FIXUP);
                pSymSect = &pModMachO->paSections[Fixup.r.r_symbolnum - 1];

                SymAddr -= pSymSect->LinkAddress;
                SymAddr += pSymSect->RVA + NewBaseAddress;
                if (Fixup.r.r_pcrel)
                    SymAddr += Fixup.r.r_address;
            }
        }

        /* adjust for PC relative */
        if (Fixup.r.r_pcrel)
            SymAddr -= Fixup.r.r_address + pFixupSect->RVA + NewBaseAddress;

        /*
         * Write back the fixed up value.
         */
        switch (Fixup.r.r_length)
        {
            case 3:
                *uFix.pu64 = (KU64)SymAddr;
                break;
            case 2:
                KLDRMODMACHO_CHECK_RETURN(Fixup.r.r_pcrel || Fixup.r.r_type == X86_64_RELOC_SUBTRACTOR, KLDR_ERR_BAD_FIXUP);
                KLDRMODMACHO_CHECK_RETURN((KI32)SymAddr == (KI64)SymAddr, KLDR_ERR_ADDRESS_OVERFLOW);
                *uFix.pu32 = (KU32)SymAddr;
                break;
            default:
                KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_BAD_FIXUP);
        }
    }

    return 0;
}


/**
 * Loads the symbol table for a MH_OBJECT file.
 *
 * The symbol table is pointed to by KLDRMODMACHO::pvaSymbols.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModMachO       The Mach-O module interpreter instance.
 */
static int  kldrModMachOLoadObjSymTab(PKLDRMODMACHO pModMachO)
{
    int rc = 0;

    if (    !pModMachO->pvaSymbols
        &&  pModMachO->cSymbols)
    {
        KSIZE cbSyms;
        KSIZE cbSym;
        void *pvSyms;
        void *pvStrings;

        /* sanity */
        KLDRMODMACHO_CHECK_RETURN(   pModMachO->offSymbols
                                  && (!pModMachO->cchStrings || pModMachO->offStrings),
                                  KLDR_ERR_MACHO_BAD_OBJECT_FILE);

        /* allocate */
        cbSym = pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
             || pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE
             ? sizeof(macho_nlist_32_t)
             : sizeof(macho_nlist_64_t);
        cbSyms = pModMachO->cSymbols * cbSym;
        KLDRMODMACHO_CHECK_RETURN(cbSyms / cbSym == pModMachO->cSymbols, KLDR_ERR_SIZE_OVERFLOW);
        rc = KERR_NO_MEMORY;
        pvSyms = kHlpAlloc(cbSyms);
        if (pvSyms)
        {
            if (pModMachO->cchStrings)
                pvStrings = kHlpAlloc(pModMachO->cchStrings);
            else
                pvStrings = kHlpAllocZ(4);
            if (pvStrings)
            {
                /* read */
                rc = kRdrRead(pModMachO->pMod->pRdr, pvSyms, cbSyms, pModMachO->offSymbols);
                if (!rc && pModMachO->cchStrings)
                    rc = kRdrRead(pModMachO->pMod->pRdr, pvStrings, pModMachO->cchStrings, pModMachO->offStrings);
                if (!rc)
                {
                    pModMachO->pvaSymbols = pvSyms;
                    pModMachO->pchStrings = (char *)pvStrings;

                    /* perform endian conversion? */
                    if (pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
                    {
                        KU32 cLeft = pModMachO->cSymbols;
                        macho_nlist_32_t *pSym = (macho_nlist_32_t *)pvSyms;
                        while (cLeft-- > 0)
                        {
                            pSym->n_un.n_strx = K_E2E_U32(pSym->n_un.n_strx);
                            pSym->n_desc = (KI16)K_E2E_U16(pSym->n_desc);
                            pSym->n_value = K_E2E_U32(pSym->n_value);
                            pSym++;
                        }
                    }
                    else if (pModMachO->Hdr.magic == IMAGE_MACHO64_SIGNATURE_OE)
                    {
                        KU32 cLeft = pModMachO->cSymbols;
                        macho_nlist_64_t *pSym = (macho_nlist_64_t *)pvSyms;
                        while (cLeft-- > 0)
                        {
                            pSym->n_un.n_strx = K_E2E_U32(pSym->n_un.n_strx);
                            pSym->n_desc = (KI16)K_E2E_U16(pSym->n_desc);
                            pSym->n_value = K_E2E_U64(pSym->n_value);
                            pSym++;
                        }
                    }

                    return 0;
                }
                kHlpFree(pvStrings);
            }
            kHlpFree(pvSyms);
        }
    }
    else
        KLDRMODMACHO_ASSERT(pModMachO->pchStrings || pModMachO->Hdr.filetype == MH_DSYM);

    return rc;
}


/**
 * Loads the fixups at the given address and performs endian
 * conversion if necessary.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModMachO       The Mach-O module interpreter instance.
 * @param   offFixups       The file offset of the fixups.
 * @param   cFixups         The number of fixups to load.
 * @param   ppaFixups       Where to put the pointer to the allocated fixup array.
 */
static int  kldrModMachOLoadFixups(PKLDRMODMACHO pModMachO, KLDRFOFF offFixups, KU32 cFixups, macho_relocation_info_t **ppaFixups)
{
    macho_relocation_info_t *paFixups;
    KSIZE cbFixups;
    int rc;

    /* allocate the memory. */
    cbFixups = cFixups * sizeof(*paFixups);
    KLDRMODMACHO_CHECK_RETURN(cbFixups / sizeof(*paFixups) == cFixups, KLDR_ERR_SIZE_OVERFLOW);
    paFixups = (macho_relocation_info_t *)kHlpAlloc(cbFixups);
    if (!paFixups)
        return KERR_NO_MEMORY;

    /* read the fixups. */
    rc = kRdrRead(pModMachO->pMod->pRdr, paFixups, cbFixups, offFixups);
    if (!rc)
    {
        *ppaFixups = paFixups;

        /* do endian conversion if necessary. */
        if (    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE
            ||  pModMachO->Hdr.magic == IMAGE_MACHO64_SIGNATURE_OE)
        {
            KU32 iFixup;
            for (iFixup = 0; iFixup < cFixups; iFixup++)
            {
                KU32 *pu32 = (KU32 *)&paFixups[iFixup];
                pu32[0] = K_E2E_U32(pu32[0]);
                pu32[1] = K_E2E_U32(pu32[1]);
            }
        }
    }
    else
        kHlpFree(paFixups);
    return rc;
}


/**
 * Maps the virgin file bits into memory if not already done.
 *
 * @returns 0 on success, non-zero kLdr status code on failure.
 * @param   pModMachO       The Mach-O module interpreter instance.
 */
static int kldrModMachOMapVirginBits(PKLDRMODMACHO pModMachO)
{
    int rc = 0;
    if (!pModMachO->pvBits)
        rc = kRdrAllMap(pModMachO->pMod->pRdr, &pModMachO->pvBits);
    return rc;
}


/** @copydoc kLdrModCallInit */
static int kldrModMachOCallInit(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    /* later */
    K_NOREF(pMod);
    K_NOREF(pvMapping);
    K_NOREF(uHandle);
    return 0;
}


/** @copydoc kLdrModCallTerm */
static int kldrModMachOCallTerm(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle)
{
    /* later */
    K_NOREF(pMod);
    K_NOREF(pvMapping);
    K_NOREF(uHandle);
    return 0;
}


/** @copydoc kLdrModCallThread */
static int kldrModMachOCallThread(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle, unsigned fAttachingOrDetaching)
{
    /* Relevant for Mach-O? */
    K_NOREF(pMod);
    K_NOREF(pvMapping);
    K_NOREF(uHandle);
    K_NOREF(fAttachingOrDetaching);
    return 0;
}


/** @copydoc kLdrModSize */
static KLDRADDR kldrModMachOSize(PKLDRMOD pMod)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    return pModMachO->cbImage;
}


/** @copydoc kLdrModGetBits */
static int kldrModMachOGetBits(PKLDRMOD pMod, void *pvBits, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODMACHO  pModMachO = (PKLDRMODMACHO)pMod->pvData;
    KU32        i;
    int         rc;

    if (!pModMachO->fCanLoad)
        return KLDR_ERR_TODO;

    /*
     * Zero the entire buffer first to simplify things.
     */
    kHlpMemSet(pvBits, 0, (KSIZE)pModMachO->cbImage);

    /*
     * When possible use the segment table to load the data.
     */
    for (i = 0; i < pMod->cSegments; i++)
    {
        /* skip it? */
        if (    pMod->aSegments[i].cbFile == -1
            ||  pMod->aSegments[i].offFile == -1
            ||  pMod->aSegments[i].LinkAddress == NIL_KLDRADDR
            ||  !pMod->aSegments[i].Alignment)
            continue;
        rc = kRdrRead(pMod->pRdr,
                      (KU8 *)pvBits + pMod->aSegments[i].RVA,
                      pMod->aSegments[i].cbFile,
                      pMod->aSegments[i].offFile);
        if (rc)
            return rc;
    }

    /*
     * Perform relocations.
     */
    return kldrModMachORelocateBits(pMod, pvBits, BaseAddress, pModMachO->LinkAddress, pfnGetImport, pvUser);
}


/** @copydoc kLdrModRelocateBits */
static int kldrModMachORelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                                    PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMODMACHO pModMachO = (PKLDRMODMACHO)pMod->pvData;
    int rc;
    K_NOREF(OldBaseAddress);

    /*
     * Call workers to do the jobs.
     */
    if (pModMachO->Hdr.filetype == MH_OBJECT)
    {
        rc = kldrModMachOObjDoImports(pModMachO, NewBaseAddress, pfnGetImport, pvUser);
        if (!rc)
            rc = kldrModMachOObjDoFixups(pModMachO, pvBits, NewBaseAddress);

    }
    else
        rc = KLDR_ERR_TODO;
    /*{
        rc = kldrModMachODoFixups(pModMachO, pvBits, NewBaseAddress, OldBaseAddress, pfnGetImport, pvUser);
        if (!rc)
            rc = kldrModMachODoImports(pModMachO, pvBits, pfnGetImport, pvUser);
    }*/

    /*
     * Construct the global offset table if necessary, it's always the last
     * segment when present.
     */
    if (!rc && pModMachO->fMakeGot)
        rc = kldrModMachOMakeGOT(pModMachO, pvBits, NewBaseAddress);

    return rc;
}


/**
 * Builds the GOT.
 *
 * Assumes the symbol table has all external symbols resolved correctly and that
 * the bits has been cleared up front.
 */
static int kldrModMachOMakeGOT(PKLDRMODMACHO pModMachO, void *pvBits, KLDRADDR NewBaseAddress)
{
    KU32  iSym = pModMachO->cSymbols;
    if (    pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE
        ||  pModMachO->Hdr.magic == IMAGE_MACHO32_SIGNATURE_OE)
    {
        macho_nlist_32_t const *paSyms = (macho_nlist_32_t const *)pModMachO->pvaSymbols;
        KU32 *paGOT = (KU32 *)((KU8 *)pvBits + pModMachO->GotRVA);
        while (iSym-- > 0)
            switch (paSyms[iSym].n_type & MACHO_N_TYPE)
            {
                case MACHO_N_SECT:
                {
                    PKLDRMODMACHOSECT pSymSect;
                    KLDRMODMACHO_CHECK_RETURN((KU32)paSyms[iSym].n_sect - 1 <= pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                    pSymSect = &pModMachO->paSections[paSyms[iSym].n_sect - 1];
                    paGOT[iSym] = (KU32)(paSyms[iSym].n_value - pSymSect->LinkAddress + pSymSect->RVA + NewBaseAddress);
                    break;
                }

                case MACHO_N_UNDF:
                case MACHO_N_ABS:
                    paGOT[iSym] = paSyms[iSym].n_value;
                    break;
            }
    }
    else
    {
        macho_nlist_64_t const *paSyms = (macho_nlist_64_t const *)pModMachO->pvaSymbols;
        KU64 *paGOT = (KU64 *)((KU8 *)pvBits + pModMachO->GotRVA);
        while (iSym-- > 0)
        {
            switch (paSyms[iSym].n_type & MACHO_N_TYPE)
            {
                case MACHO_N_SECT:
                {
                    PKLDRMODMACHOSECT pSymSect;
                    KLDRMODMACHO_CHECK_RETURN((KU32)paSyms[iSym].n_sect - 1 <= pModMachO->cSections, KLDR_ERR_MACHO_BAD_SYMBOL);
                    pSymSect = &pModMachO->paSections[paSyms[iSym].n_sect - 1];
                    paGOT[iSym] = paSyms[iSym].n_value - pSymSect->LinkAddress + pSymSect->RVA + NewBaseAddress;
                    break;
                }

                case MACHO_N_UNDF:
                case MACHO_N_ABS:
                    paGOT[iSym] = paSyms[iSym].n_value;
                    break;
            }
        }

        if (pModMachO->JmpStubsRVA != NIL_KLDRADDR)
        {
            iSym = pModMachO->cSymbols;
            switch (pModMachO->Hdr.cputype)
            {
                /*
                 * AMD64 is simple since the GOT and the indirect jmps are parallel
                 * arrays with entries of the same size. The relative offset will
                 * be the the same for each entry, kind of nice. :-)
                 */
                case CPU_TYPE_X86_64:
                {
                    KU64   *paJmps = (KU64 *)((KU8 *)pvBits + pModMachO->JmpStubsRVA);
                    KI32    off;
                    KU64    u64Tmpl;
                    union
                    {
                        KU8     ab[8];
                        KU64    u64;
                    }       Tmpl;

                    /* create the template. */
                    off = (KI32)(pModMachO->GotRVA - (pModMachO->JmpStubsRVA + 6));
                    Tmpl.ab[0] = 0xff; /* jmp [GOT-entry wrt RIP] */
                    Tmpl.ab[1] = 0x25;
                    Tmpl.ab[2] =  off        & 0xff;
                    Tmpl.ab[3] = (off >>  8) & 0xff;
                    Tmpl.ab[4] = (off >> 16) & 0xff;
                    Tmpl.ab[5] = (off >> 24) & 0xff;
                    Tmpl.ab[6] = 0xcc;
                    Tmpl.ab[7] = 0xcc;
                    u64Tmpl = Tmpl.u64;

                    /* copy the template to every jmp table entry. */
                    while (iSym-- > 0)
                        paJmps[iSym] = u64Tmpl;
                    break;
                }

                default:
                    KLDRMODMACHO_FAILED_RETURN(KLDR_ERR_TODO);
            }
        }
    }
    return 0;
}


/**
 * The Mach-O module interpreter method table.
 */
KLDRMODOPS g_kLdrModMachOOps =
{
    "Mach-O",
    NULL,
    kldrModMachOCreate,
    kldrModMachODestroy,
    kldrModMachOQuerySymbol,
    kldrModMachOEnumSymbols,
    kldrModMachOGetImport,
    kldrModMachONumberOfImports,
    NULL /* can execute one is optional */,
    kldrModMachOGetStackInfo,
    kldrModMachOQueryMainEntrypoint,
    kldrModMachOQueryImageUuid,
    NULL,
    NULL,
    kldrModMachOEnumDbgInfo,
    kldrModMachOHasDbgInfo,
    kldrModMachOMap,
    kldrModMachOUnmap,
    kldrModMachOAllocTLS,
    kldrModMachOFreeTLS,
    kldrModMachOReload,
    kldrModMachOFixupMapping,
    kldrModMachOCallInit,
    kldrModMachOCallTerm,
    kldrModMachOCallThread,
    kldrModMachOSize,
    kldrModMachOGetBits,
    kldrModMachORelocateBits,
    NULL, /** @todo mostly done */
    42 /* the end */
};


/* $Id: ldr.h $ */
/** @file
 * IPRT - Loader Internals.
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

#ifndef ___internal_ldr_h
#define ___internal_ldr_h

#include <iprt/types.h>
#include "internal/magics.h"

RT_C_DECLS_BEGIN


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#ifdef DOXYGEN_RUNNING
/** @def LDR_WITH_NATIVE
 * Define this to get native support. */
# define LDR_WITH_NATIVE

/** @def LDR_WITH_ELF32
 * Define this to get 32-bit ELF support. */
# define LDR_WITH_ELF32

/** @def LDR_WITH_ELF64
 * Define this to get 64-bit ELF support. */
# define LDR_WITH_ELF64

/** @def LDR_WITH_PE
 * Define this to get 32-bit and 64-bit PE support. */
# define LDR_WITH_PE

/** @def LDR_WITH_LX
 * Define this to get LX support. */
# define LDR_WITH_LX

/** @def LDR_WITH_MACHO
 * Define this to get mach-o support (not implemented yet). */
# define LDR_WITH_MACHO
#endif /* DOXYGEN_RUNNING */

#if defined(LDR_WITH_ELF32) || defined(LDR_WITH_ELF64)
/** @def LDR_WITH_ELF
 * This is defined if any of the ELF versions is requested.
 */
# define LDR_WITH_ELF
#endif

/* These two may clash with winnt.h. */
#undef IMAGE_DOS_SIGNATURE
#undef IMAGE_NT_SIGNATURE


/** Little endian uint32_t ELF signature ("\x7fELF"). */
#define IMAGE_ELF_SIGNATURE (0x7f | ('E' << 8) | ('L' << 16) | ('F' << 24))
/** Little endian uint32_t PE signature ("PE\0\0"). */
#define IMAGE_NT_SIGNATURE  0x00004550
/** Little endian uint16_t LX signature ("LX") */
#define IMAGE_LX_SIGNATURE  ('L' | ('X' << 8))
/** Little endian uint16_t LE signature ("LE") */
#define IMAGE_LE_SIGNATURE  ('L' | ('E' << 8))
/** Little endian uint16_t NE signature ("NE") */
#define IMAGE_NE_SIGNATURE  ('N' | ('E' << 8))
/** Little endian uint16_t MZ signature ("MZ"). */
#define IMAGE_DOS_SIGNATURE ('M' | ('Z' << 8))


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Loader state.
 */
typedef enum RTLDRSTATE
{
    /** Invalid. */
    LDR_STATE_INVALID = 0,
    /** Opened. */
    LDR_STATE_OPENED,
    /** The image can no longer be relocated. */
    LDR_STATE_DONE,
    /** The image was loaded, not opened. */
    LDR_STATE_LOADED,
    /** The usual 32-bit hack. */
    LDR_STATE_32BIT_HACK = 0x7fffffff
} RTLDRSTATE;


/** Pointer to a loader item. */
typedef struct RTLDRMODINTERNAL *PRTLDRMODINTERNAL;

/**
 * Loader module operations.
 */
typedef struct RTLDROPS
{
    /** The name of the executable format. */
    const char *pszName;

    /**
     * Release any resources attached to the module.
     * The caller will do RTMemFree on pMod on return.
     *
     * @returns iprt status code.
     * @param   pMod    Pointer to the loader module structure.
     * @remark  Compulsory entry point.
     */
    DECLCALLBACKMEMBER(int, pfnClose)(PRTLDRMODINTERNAL pMod);

    /**
     * Gets a simple symbol.
     * This entrypoint can be omitted if RTLDROPS::pfnGetSymbolEx() is provided.
     *
     * @returns iprt status code.
     * @param   pMod        Pointer to the loader module structure.
     * @param   pszSymbol   The symbol name.
     * @param   ppvValue    Where to store the symbol value.
     */
    DECLCALLBACKMEMBER(int, pfnGetSymbol)(PRTLDRMODINTERNAL pMod, const char *pszSymbol, void **ppvValue);

    /**
     * Called when we're done with getting bits and relocating them.
     * This is used to release resources used by the loader to support those actions.
     *
     * After this call none of the extended loader functions can be called.
     *
     * @returns iprt status code.
     * @param   pMod        Pointer to the loader module structure.
     * @remark  This is an optional entry point.
     */
    DECLCALLBACKMEMBER(int, pfnDone)(PRTLDRMODINTERNAL pMod);

    /**
     * Enumerates the symbols exported by the module.
     *
     * @returns iprt status code, which might have been returned by pfnCallback.
     * @param   pMod        Pointer to the loader module structure.
     * @param   fFlags      Flags indicating what to return and such.
     * @param   pvBits      Pointer to the bits returned by RTLDROPS::pfnGetBits(), optional.
     * @param   BaseAddress The image base addressto use when calculating the symbol values.
     * @param   pfnCallback The callback function which each symbol is to be
     *                      fed to.
     * @param   pvUser      User argument to pass to the enumerator.
     * @remark  This is an optional entry point.
     */
    DECLCALLBACKMEMBER(int, pfnEnumSymbols)(PRTLDRMODINTERNAL pMod, unsigned fFlags, const void *pvBits, RTUINTPTR BaseAddress,
                                            PFNRTLDRENUMSYMS pfnCallback, void *pvUser);


/* extended functions: */

    /**
     * Gets the size of the loaded image (i.e. in memory).
     *
     * @returns in memory size, in bytes.
     * @returns ~(size_t)0 if it's not an extended image.
     * @param   pMod    Pointer to the loader module structure.
     * @remark  Extended loader feature.
     */
    DECLCALLBACKMEMBER(size_t, pfnGetImageSize)(PRTLDRMODINTERNAL pMod);

    /**
     * Gets the image bits fixed up for a specified address.
     *
     * @returns iprt status code.
     * @param   pMod            Pointer to the loader module structure.
     * @param   pvBits          Where to store the bits. The size of this buffer is equal or
     *                          larger to the value returned by pfnGetImageSize().
     * @param   BaseAddress     The base address which the image should be fixed up to.
     * @param   pfnGetImport    The callback function to use to resolve imports (aka unresolved externals).
     * @param   pvUser          User argument to pass to the callback.
     * @remark  Extended loader feature.
     */
    DECLCALLBACKMEMBER(int, pfnGetBits)(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR BaseAddress, PFNRTLDRIMPORT pfnGetImport, void *pvUser);

    /**
     * Relocate bits obtained using pfnGetBits to a new address.
     *
     * @returns iprt status code.
     * @param   pMod            Pointer to the loader module structure.
     * @param   pvBits          Where to store the bits. The size of this buffer is equal or
     *                          larger to the value returned by pfnGetImageSize().
     * @param   NewBaseAddress  The base address which the image should be fixed up to.
     * @param   OldBaseAddress  The base address which the image is currently fixed up to.
     * @param   pfnGetImport    The callback function to use to resolve imports (aka unresolved externals).
     * @param   pvUser          User argument to pass to the callback.
     * @remark  Extended loader feature.
     */
    DECLCALLBACKMEMBER(int, pfnRelocate)(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR NewBaseAddress, RTUINTPTR OldBaseAddress, PFNRTLDRIMPORT pfnGetImport, void *pvUser);

    /**
     * Gets a symbol with special base address and stuff.
     * This entrypoint can be omitted if RTLDROPS::pfnGetSymbolEx() is provided and the special BaseAddress feature isn't supported.
     *
     * @returns iprt status code.
     * @retval  VERR_LDR_FORWARDER forwarder, use pfnQueryForwarderInfo. Buffer size
     *          in @a pValue.
     * @param   pMod        Pointer to the loader module structure.
     * @param   pvBits      Pointer to bits returned by RTLDROPS::pfnGetBits(), optional.
     * @param   BaseAddress The image base address to use when calculating the symbol value.
     * @param   iOrdinal    Symbol table ordinal, UINT32_MAX if the symbol name
     *                      should be used.
     * @param   pszSymbol   The symbol name.
     * @param   pValue      Where to store the symbol value.
     * @remark  Extended loader feature.
     */
    DECLCALLBACKMEMBER(int, pfnGetSymbolEx)(PRTLDRMODINTERNAL pMod, const void *pvBits, RTUINTPTR BaseAddress,
                                            uint32_t iOrdinal, const char *pszSymbol, RTUINTPTR *pValue);

    /**
     * Query forwarder information on the specified symbol.
     *
     * This is an optional entrypoint.
     *
     * @returns iprt status code.
     * @param   pMod        Pointer to the loader module structure.
     * @param   pvBits      Pointer to bits returned by RTLDROPS::pfnGetBits(),
     *                      optional.
     * @param   iOrdinal    Symbol table ordinal of the forwarded symbol to query.
     *                      UINT32_MAX if the symbol name should be used.
     * @param   pszSymbol   The symbol name of the forwarded symbol to query.
     * @param   pInfo       Where to return the forwarder information.
     * @param   cbInfo      The size of the pInfo buffer. The pfnGetSymbolEx
     *                      entrypoint returns the required size in @a pValue when
     *                      the return code is VERR_LDR_FORWARDER.
     * @remark  Extended loader feature.
     */
    DECLCALLBACKMEMBER(int, pfnQueryForwarderInfo)(PRTLDRMODINTERNAL pMod, const void *pvBits, uint32_t iOrdinal,
                                                   const char *pszSymbol, PRTLDRIMPORTINFO pInfo, size_t cbInfo);

    /**
     * Enumerates the debug info contained in the module.
     *
     * @returns iprt status code, which might have been returned by pfnCallback.
     * @param   pMod        Pointer to the loader module structure.
     * @param   pvBits      Pointer to the bits returned by RTLDROPS::pfnGetBits(), optional.
     * @param   pfnCallback The callback function which each debug info part is
     *                      to be fed to.
     * @param   pvUser      User argument to pass to the enumerator.
     * @remark  This is an optional entry point that can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnEnumDbgInfo)(PRTLDRMODINTERNAL pMod, const void *pvBits,
                                            PFNRTLDRENUMDBG pfnCallback, void *pvUser);

    /**
     * Enumerates the segments in the module.
     *
     * @returns iprt status code, which might have been returned by pfnCallback.
     * @param   pMod        Pointer to the loader module structure.
     * @param   pfnCallback The callback function which each debug info part is
     *                      to be fed to.
     * @param   pvUser      User argument to pass to the enumerator.
     * @remark  This is an optional entry point that can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnEnumSegments)(PRTLDRMODINTERNAL pMod, PFNRTLDRENUMSEGS pfnCallback, void *pvUser);

    /**
     * Converts a link address to a segment:offset address.
     *
     * @returns IPRT status code.
     *
     * @param   pMod            Pointer to the loader module structure.
     * @param   LinkAddress     The link address to convert.
     * @param   piSeg           Where to return the segment index.
     * @param   poffSeg         Where to return the segment offset.
     * @remark  This is an optional entry point that can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnLinkAddressToSegOffset)(PRTLDRMODINTERNAL pMod, RTLDRADDR LinkAddress,
                                                       uint32_t *piSeg, PRTLDRADDR poffSeg);

    /**
     * Converts a link address to a RVA.
     *
     * @returns IPRT status code.
     *
     * @param   pMod            Pointer to the loader module structure.
     * @param   LinkAddress     The link address to convert.
     * @param   pRva            Where to return the RVA.
     * @remark  This is an optional entry point that can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnLinkAddressToRva)(PRTLDRMODINTERNAL pMod, RTLDRADDR LinkAddress, PRTLDRADDR pRva);

    /**
     * Converts a segment:offset to a RVA.
     *
     * @returns IPRT status code.
     *
     * @param   pMod            Pointer to the loader module structure.
     * @param   iSeg            The segment index.
     * @param   offSeg          The segment offset.
     * @param   pRva            Where to return the RVA.
     * @remark  This is an optional entry point that can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnSegOffsetToRva)(PRTLDRMODINTERNAL pMod, uint32_t iSeg, RTLDRADDR offSeg, PRTLDRADDR pRva);

    /**
     * Converts a RVA to a segment:offset.
     *
     * @returns IPRT status code.
     *
     * @param   pMod            Pointer to the loader module structure.
     * @param   Rva             The RVA to convert.
     * @param   piSeg           Where to return the segment index.
     * @param   poffSeg         Where to return the segment offset.
     * @remark  This is an optional entry point that can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnRvaToSegOffset)(PRTLDRMODINTERNAL pMod, RTLDRADDR Rva, uint32_t *piSeg, PRTLDRADDR poffSeg);

    /**
     * Reads a debug info part (section) from the image.
     *
     * This is primarily needed for getting DWARF sections in ELF image with fixups
     * applied and won't be required by most other loader backends.
     *
     * @returns IPRT status code.
     *
     * @param   pMod            Pointer to the loader module structure.
     * @param   pvBuf           The buffer to read into.
     * @param   iDbgInfo        The debug info ordinal number if the request
     *                          corresponds exactly to a debug info part from
     *                          pfnEnumDbgInfo.  Otherwise, pass UINT32_MAX.
     * @param   off             The offset into the image file.
     * @param   cb              The number of bytes to read.
     * @param   pMod            Pointer to the loader module structure.
     */
    DECLCALLBACKMEMBER(int, pfnReadDbgInfo)(PRTLDRMODINTERNAL pMod, uint32_t iDbgInfo, RTFOFF off, size_t cb, void *pvBuf);

    /**
     * Generic method for querying image properties.
     *
     * @returns IPRT status code.
     * @retval  VERR_NOT_SUPPORTED if the property query isn't supported (either all
     *          or that specific property).
     * @retval  VERR_NOT_FOUND the property was not found in the module.
     *
     * @param   pMod            Pointer to the loader module structure.
     * @param   enmProp         The property to query (valid).
     * @param   pvBits          Pointer to the bits returned by
     *                          RTLDROPS::pfnGetBits(), optional.
     * @param   pvBuf           Pointer to the input / output buffer. This is valid.
     *                          Normally only used for returning data, but in some
     *                          cases it also holds input.
     * @param   cbBuf           The size of the buffer (valid as per
     *                          property).
     * @param   pcbRet          The number of bytes actually returned.  If
     *                          VERR_BUFFER_OVERFLOW is returned, this is set to the
     *                          required buffer size.
     */
    DECLCALLBACKMEMBER(int, pfnQueryProp)(PRTLDRMODINTERNAL pMod, RTLDRPROP enmProp, void const *pvBits,
                                          void *pvBuf, size_t cbBuf, size_t *pcbRet);

    /**
     * Verify the image signature.
     *
     * This may permform additional integrity checks on the image structures that
     * was not done when opening the image.
     *
     * @returns IPRT status code.
     * @retval  VERR_LDRVI_NOT_SIGNED if not signed.
     *
     * @param   pMod            Pointer to the loader module structure.
     * @param   pfnCallback     Callback that does the signature and certificate
     *                          verficiation.
     * @param   pvUser          User argument for the callback.
     * @param   pErrInfo        Pointer to an error info buffer. Optional.
     */
    DECLCALLBACKMEMBER(int, pfnVerifySignature)(PRTLDRMODINTERNAL pMod, PFNRTLDRVALIDATESIGNEDDATA pfnCallback, void *pvUser,
                                                PRTERRINFO pErrInfo);

    /**
     * Calculate the image hash according the image signing rules.
     *
     * @returns IPRT status code.
     * @param   pMod            The module handle.
     * @param   enmDigest       Which kind of digest.
     * @param   pszDigest       Where to store the image digest.
     * @param   cbDigest        Size of the buffer @a pszDigest points at.
     */
    DECLCALLBACKMEMBER(int, pfnHashImage)(PRTLDRMODINTERNAL pMod, RTDIGESTTYPE enmDigest, char *pszDigest, size_t cbDigest);

    /** Dummy entry to make sure we've initialized it all. */
    RTUINT uDummy;
} RTLDROPS;
typedef RTLDROPS *PRTLDROPS;
typedef const RTLDROPS *PCRTLDROPS;


/**
 * Loader module core.
 */
typedef struct RTLDRMODINTERNAL
{
    /** The loader magic value (RTLDRMOD_MAGIC). */
    uint32_t                u32Magic;
    /** State. */
    RTLDRSTATE              eState;
    /** Loader ops. */
    PCRTLDROPS              pOps;
    /** Pointer to the reader instance. This is NULL for native image. */
    PRTLDRREADER            pReader;
    /** Image format.  */
    RTLDRFMT                enmFormat;
    /** Image type.  */
    RTLDRTYPE               enmType;
    /** Image endianness.  */
    RTLDRENDIAN             enmEndian;
    /** Image target architecture.  */
    RTLDRARCH               enmArch;
} RTLDRMODINTERNAL;


/**
 * Validates that a loader module handle is valid.
 *
 * @returns true if valid.
 * @returns false if invalid.
 * @param   hLdrMod     The loader module handle.
 */
DECLINLINE(bool) rtldrIsValid(RTLDRMOD hLdrMod)
{
    return VALID_PTR(hLdrMod)
        && ((PRTLDRMODINTERNAL)hLdrMod)->u32Magic == RTLDRMOD_MAGIC;
}


/**
 * Native loader module.
 */
typedef struct RTLDRMODNATIVE
{
    /** The core structure. */
    RTLDRMODINTERNAL    Core;
    /** The native handle. */
    uintptr_t           hNative;
    /** The load flags (RTLDRLOAD_FLAGS_XXX). */
    uint32_t            fFlags;
} RTLDRMODNATIVE;
/** Pointer to a native module. */
typedef RTLDRMODNATIVE *PRTLDRMODNATIVE;

/** @copydoc RTLDROPS::pfnGetSymbol */
DECLCALLBACK(int) rtldrNativeGetSymbol(PRTLDRMODINTERNAL pMod, const char *pszSymbol, void **ppvValue);
/** @copydoc RTLDROPS::pfnClose */
DECLCALLBACK(int) rtldrNativeClose(PRTLDRMODINTERNAL pMod);

/**
 * Load a native module using the native loader.
 *
 * @returns iprt status code.
 * @param   pszFilename     The image filename.
 * @param   phHandle        Where to store the module handle on success.
 * @param   fFlags          RTLDRLOAD_FLAGS_XXX.
 * @param   pErrInfo        Where to return extended error information. Optional.
 */
int rtldrNativeLoad(const char *pszFilename, uintptr_t *phHandle, uint32_t fFlags, PRTERRINFO pErrInfo);

/**
 * Load a system library.
 *
 * @returns iprt status code.
 * @param   pszFilename     The image filename.
 * @param   pszExt          Extension to add. NULL if none.
 * @param   fFlags          RTLDRLOAD_FLAGS_XXX.
 * @param   phLdrMod        Where to return the module handle on success.
 */
int rtldrNativeLoadSystem(const char *pszFilename, const char *pszExt, uint32_t fFlags, PRTLDRMOD phLdrMod);

int rtldrPEOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, RTFOFF offNtHdrs, PRTLDRMOD phLdrMod, PRTERRINFO pErrInfo);
int rtldrELFOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, PRTLDRMOD phLdrMod, PRTERRINFO pErrInfo);
int rtldrkLdrOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, PRTLDRMOD phLdrMod, PRTERRINFO pErrInfo);
/*int rtldrLXOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, RTFOFF offLX, PRTLDRMOD phLdrMod);
int rtldrMachoOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, RTFOFF offSomething, PRTLDRMOD phLdrMod);*/


DECLHIDDEN(int) rtLdrReadAt(RTLDRMOD hLdrMod, void *pvBuf, uint32_t iDbgInfo, RTFOFF off, size_t cb);
DECLHIDDEN(const char *) rtLdrArchName(RTLDRARCH enmArch);

RT_C_DECLS_END

#endif


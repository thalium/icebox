/* $Id: kDbg.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader.
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

#ifndef ___k_kDbg_h___
#define ___k_kDbg_h___

#include <k/kDefs.h>
#include <k/kTypes.h>
#include <k/kRdr.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup grp_kDbg      Debug Info Reader
 * @{
 */

/** @def KDBG_DECL
 * Declares a kDbg function according to build context.
 * @param type          The return type.
 */
#if defined(KDBG_BUILDING_DYNAMIC)
# define KDBG_DECL(type)        K_DECL_EXPORT(type)
#elif defined(KDBG_BUILT_DYNAMIC)
# define KDBG_DECL(type)        K_DECL_IMPORT(type)
#else
# define KDBG_DECL(type)        type
#endif


/** The kDbg address type. */
typedef KU64                    KDBGADDR;
/** Pointer to a kDbg address. */
typedef KDBGADDR               *PKDBGADDR;
/** Pointer to a const kDbg address. */
typedef const KDBGADDR         *PCKDBGADDR;
/** @def KDBGADDR_PRI
 * printf format type. */
#define KDBGADDR_PRI            KX64_PRI
/** @def KDBGADDR_MAX
 * Max kDbg address value. */
#define KDBGADDR_MAX            KU64_C(0xfffffffffffffffe)
/** @def KDBGADDR_C
 * kDbg address constant.
 * @param c         The constant value. */
#define KDBGADDR_C(c)           KU64_C(c)
/** NIL address. */
#define NIL_KDBGADDR            KU64_MAX


/** @name   Special Segments
 * @{ */
/** Relative Virtual Address.
 * The specified offset is relative to the image base. The image base is the lowest memory
 * address used by the image when loaded with the address assignments indicated in the image. */
#define KDBGSEG_RVA             (-1)
/** Absolute segment. The offset isn't relative to anything. */
#define KDBGSEG_ABS             (-2)
/** @} */


/** The max filename path length used by the debug reader. */
#define KDBG_PATH_MAX           260

/**
 * Line number details.
 */
typedef struct KDBGLINE
{
    /** The relative virtual address. */
    KDBGADDR    RVA;
    /** The offset into the segment. */
    KDBGADDR    offSegment;
    /** The segment number. */
    KI32        iSegment;
    /** The Line number. */
    KU32        iLine;
    /** The actual size of this structure. */
    KU16        cbSelf;
    /** The length of the filename. */
    KU16        cchFile;
    /** The name of the file this line number relates to. */
    char        szFile[KDBG_PATH_MAX];
} KDBGLINE;
/** Pointer to line number details. */
typedef KDBGLINE *PKDBGLINE;
/** Pointer to const line number details. */
typedef const KDBGLINE *PCKDBGLINE;
/** Pointer to a pointer to line number details. */
typedef PKDBGLINE *PPKDBGLINE;

/**
 * Duplicates a line number.
 *
 * To save heap space, the returned line number will not own more heap space
 * than it strictly need to. So, it's not possible to append stuff to the symbol
 * or anything of that kind.
 *
 * @returns Pointer to the duplicate.
 *          This must be freed using RTDbgSymbolFree().
 * @param   pLine       The line number to be duplicated.
 */
KDBG_DECL(PKDBGLINE) kDbgLineDup(PCKDBGLINE pLine);

/**
 * Frees a line number obtained from the RTDbg API.
 *
 * @returns VINF_SUCCESS on success.
 * @returns KERR_INVALID_POINTER if a NULL pointer or an !KDBG_VALID_PTR() is passed in.
 *
 * @param   pLine       The line number to be freed.
 */
KDBG_DECL(int) kDbgLineFree(PKDBGLINE pLine);


/** @name Symbol Flags.
 * @{ */
/** The symbol is weak. */
#define KDBGSYM_FLAGS_WEAK      KU32_C(0x00000000)
/** The symbol is absolute.
 * (This also indicated by the segment number.) */
#define KDBGSYM_FLAGS_ABS       KU32_C(0x00000001)
/** The symbol is exported. */
#define KDBGSYM_FLAGS_EXPORTED  KU32_C(0x00000002)
/** The symbol is a function/method/procedure/whatever-executable-code. */
#define KDBGSYM_FLAGS_CODE      KU32_C(0x00000004)
/** The symbol is some kind of data. */
#define KDBGSYM_FLAGS_DATA      KU32_C(0x00000008)
/** @} */

/** The max symbol name length used by the debug reader. */
#define KDBG_SYMBOL_MAX         384

/**
 * Symbol details.
 */
typedef struct KDBGSYMBOL
{
    /** The adddress of this symbol in the relevant space.
     * This is NIL_KDBGADDR unless the information was
     * returned by a kDbgSpace API. */
    KDBGADDR    Address;
    /** The relative virtual address. */
    KDBGADDR    RVA;
    /** The symbol size.
     * This is not a reliable field, it could be a bad guess. Ignore if zero. */
    KDBGADDR    cb;
    /** The offset into the segment. */
    KDBGADDR    offSegment;
    /** The segment number. */
    KI32        iSegment;
    /** The symbol flags. */
    KU32        fFlags;
/** @todo type info. */
    /** The actual size of this structure. */
    KU16        cbSelf;
    /** The length of the symbol name. */
    KU16        cchName;
    /** The symbol name. */
    char        szName[KDBG_SYMBOL_MAX];
} KDBGSYMBOL;
/** Pointer to symbol details. */
typedef KDBGSYMBOL *PKDBGSYMBOL;
/** Pointer to const symbol details. */
typedef const KDBGSYMBOL *PCKDBGSYMBOL;
/** Pointer to a pointer to symbol details. */
typedef PKDBGSYMBOL *PPKDBGSYMBOL;

/**
 * Duplicates a symbol.
 *
 * To save heap space, the returned symbol will not own more heap space than
 * it strictly need to. So, it's not possible to append stuff to the symbol
 * or anything of that kind.
 *
 * @returns Pointer to the duplicate.
 *          This must be freed using kDbgSymbolFree().
 * @param   pSymbol     The symbol to be freed.
 */
KDBG_DECL(PKDBGSYMBOL) kDbgSymbolDup(PCKDBGSYMBOL pSymbol);

/**
 * Frees a symbol obtained from the kDbg API.
 *
 * @returns VINF_SUCCESS on success.
 * @returns KERR_INVALID_POINTER if a NULL pointer or an !KDBG_VALID_PTR() is passed in.
 *
 * @param   pSymbol     The symbol to be freed.
 */
KDBG_DECL(int) kDbgSymbolFree(PKDBGSYMBOL pSymbol);


/** Pointer to a debug module. */
typedef struct KDBGMOD *PKDBGMOD;
/** Pointer to a debug module pointer. */
typedef PKDBGMOD *PPKDBGMOD;


KDBG_DECL(int) kDbgModuleOpen(PPKDBGMOD ppDbgMod, const char *pszFilename, struct KLDRMOD *pLdrMod);
KDBG_DECL(int) kDbgModuleOpenFile(PPKDBGMOD ppDbgMod, PKRDR pRdr, struct KLDRMOD *pLdrMod);
KDBG_DECL(int) kDbgModuleOpenFilePart(PPKDBGMOD ppDbgMod, PKRDR pRdr, KFOFF off, KFOFF cb, struct KLDRMOD *pLdrMod);
KDBG_DECL(int) kDbgModuleClose(PKDBGMOD pMod);
KDBG_DECL(int) kDbgModuleQuerySymbol(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGSYMBOL pSym);
KDBG_DECL(int) kDbgModuleQuerySymbolA(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PPKDBGSYMBOL ppSym);
KDBG_DECL(int) kDbgModuleQueryLine(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PKDBGLINE pLine);
KDBG_DECL(int) kDbgModuleQueryLineA(PKDBGMOD pMod, KI32 iSegment, KDBGADDR off, PPKDBGLINE ppLine);


/** @} */

#ifdef __cplusplus
}
#endif

#endif

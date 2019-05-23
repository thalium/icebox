/* $Id: kLdr.h 81 2016-08-18 22:10:38Z bird $ */
/** @file
 * kLdr - The Dynamic Loader.
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

#ifndef ___k_kLdr_h___
#define ___k_kLdr_h___

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Include the base typedefs and macros.
 */
#include <k/kDefs.h>
#include <k/kTypes.h>
#include <k/kCpus.h>


/** @defgroup grp_kLdrBasic     kLdr Basic Types
 * @{ */

/** The kLdr address type. */
typedef KU64 KLDRADDR;
/** Pointer to a kLdr address. */
typedef KLDRADDR *PKLDRADDR;
/** Pointer to a const kLdr address. */
typedef const KLDRADDR *PCKLDRADDR;

/** NIL address. */
#define NIL_KLDRADDR    (~(KU64)0)

/** @def PRI_KLDRADDR
 * printf format type. */
#ifdef _MSC_VER
# define PRI_KLDRADDR    "I64x"
#else
# define PRI_KLDRADDR    "llx"
#endif

/** Align a KSIZE value. */
#define KLDR_ALIGN_ADDR(val, align) ( ((val) + ((align) - 1)) & ~(KLDRADDR)((align) - 1) )


/** The kLdr size type. */
typedef KU64 KLDRSIZE;
/** Pointer to a kLdr size. */
typedef KLDRSIZE *PKLDRSIZE;
/** Pointer to a const kLdr size. */
typedef const KLDRSIZE *PCKLDRSIZE;

/** @def PRI_KLDRSIZE
 * printf format type. */
#ifdef _MSC_VER
# define PRI_KLDRSIZE    "I64x"
#else
# define PRI_KLDRSIZE    "llx"
#endif


/** The kLdr file offset type. */
typedef long KLDRFOFF;
/** Pointer to a kLdr file offset type. */
typedef KLDRFOFF *PKLDRFOFF;
/** Pointer to a const kLdr file offset type. */
typedef const KLDRFOFF *PCKLDRFOFF;

/** @def PRI_KLDRFOFF
 * printf format type. */
#define PRI_KLDRFOFF     "lx"


/**
 * Union of all the integer types.
 */
typedef union KLDRU
{
    KI8             i8;     /**< KI8 view. */
    KU8             u8;     /**< KU8 view. */
    KI16            i16;    /**< KI16 view. */
    KU16            u16;    /**< KU16 view. */
    KI32            i32;    /**< KI32 view. */
    KU32            u32;    /**< KU32 view. */
    KI64            i64;    /**< KI64 view. */
    KU64            u64;    /**< KU64 view. */

    KI8             ai8[8]; /**< KI8 array view . */
    KU8             au8[8]; /**< KU8 array view. */
    KI16            ai16[4];/**< KI16 array view . */
    KU16            au16[4];/**< KU16 array view. */
    KI32            ai32[2];/**< KI32 array view . */
    KU32            au32[2];/**< KU32 array view. */

    signed char     ch;     /**< signed char view. */
    unsigned char   uch;    /**< unsigned char view. */
    signed short    s;      /**< signed short view. */
    unsigned short  us;     /**< unsigned short view. */
    signed int      i;      /**< signed int view. */
    unsigned int    u;      /**< unsigned int view. */
    signed long     l;      /**< signed long view. */
    unsigned long   ul;     /**< unsigned long view. */
    void           *pv;     /**< void pointer view. */

    KLDRADDR        Addr;   /**< kLdr address view. */
    KLDRSIZE        Size;   /**< kLdr size view. */
} KLDRU;
/** Pointer to an integer union. */
typedef KLDRU *PKLDRU;
/** Pointer to a const integer union. */
typedef const KLDRU *PCKLDRU;


/**
 * Union of pointers to all the integer types.
 */
typedef union KLDRPU
{
    KI8            *pi8;    /**< KI8 view. */
    KU8            *pu8;    /**< KU8 view. */
    KI16           *pi16;   /**< KI16 view. */
    KU16           *pu16;   /**< KU16 view. */
    KI32           *pi32;   /**< KI32 view. */
    KU32           *pu32;   /**< KU32 view. */
    KI64           *pi64;   /**< KI64 view. */
    KU64           *pu64;   /**< KU64 view. */

    signed char    *pch;    /**< signed char view. */
    unsigned char  *puch;   /**< unsigned char view. */
    signed short   *ps;     /**< signed short view. */
    unsigned short *pus;    /**< unsigned short view. */
    signed int     *pi;     /**< signed int view. */
    unsigned int   *pu;     /**< unsigned int view. */
    signed long    *pl;     /**< signed long view. */
    unsigned long  *pul;    /**< unsigned long view. */
    void           *pv;     /**< void pointer view. */
} KLDRPU;
/** Pointer to an integer pointer union. */
typedef KLDRPU *PKLDRPU;
/** Pointer to a const integer pointer union. */
typedef const KLDRPU *PCKLDRPU;

/** @} */


/** @defgroup grp_kLdrMod   kLdrMod - The executable image intepreter
 * @{ */

/**
 * Debug info type (from the loader point of view).
 */
typedef enum KLDRDBGINFOTYPE
{
    /** The usual invalid enum value. */
    KLDRDBGINFOTYPE_INVALID = 0,
    /** Unknown debug info format. */
    KLDRDBGINFOTYPE_UNKNOWN,
    /** Stabs. */
    KLDRDBGINFOTYPE_STABS,
    /** Debug With Arbitrary Record Format (DWARF). */
    KLDRDBGINFOTYPE_DWARF,
    /** Microsoft Codeview debug info. */
    KLDRDBGINFOTYPE_CODEVIEW,
    /** Watcom debug info. */
    KLDRDBGINFOTYPE_WATCOM,
    /** IBM High Level Language debug info.. */
    KLDRDBGINFOTYPE_HLL,
    /** The end of the valid debug info values (exclusive). */
    KLDRDBGINFOTYPE_END,
    /** Blow the type up to 32-bit. */
    KLDRDBGINFOTYPE_32BIT_HACK = 0x7fffffff
} KLDRDBGINFOTYPE;
/** Pointer to a kLdr debug info type. */
typedef KLDRDBGINFOTYPE *PKLDRDBGINFOTYPE;


/**
 * Stack information.
 */
typedef struct KLDRSTACKINFO
{
    /** The base address of the stack (sub) segment.
     * Set this to NIL_KLDRADDR if the module doesn't include any stack segment. */
    KLDRADDR        Address;
    /** The base address of the stack (sub) segment, link address.
     * Set this to NIL_KLDRADDR if the module doesn't include any stack (sub)segment. */
    KLDRADDR        LinkAddress;
    /** The stack size of the main thread.
     * If no stack (sub)segment in the module, this is the stack size of the main thread.
     * If the module doesn't contain this kind of information this field will be set to 0. */
    KLDRSIZE        cbStack;
    /** The stack size of non-main threads.
     * If the module doesn't contain this kind of information this field will be set to 0. */
    KLDRSIZE        cbStackThread;
} KLDRSTACKINFO;
/** Pointer to stack information. */
typedef KLDRSTACKINFO *PKLDRSTACKINFO;
/** Pointer to const stack information. */
typedef const KLDRSTACKINFO *PCKLDRSTACKINFO;


/**
 * Loader segment.
 */
typedef struct KLDRSEG
{
    /** Variable free to use for the kLdr user. */
    void           *pvUser;
    /** The segment name. (Might not be zero terminated!) */
    const char     *pchName;
    /** The length of the segment name. */
    KU32            cchName;
    /** The flat selector to use for the segment (i.e. data/code).
     * Primarily a way for the user to specify selectors for the LX/LE and NE interpreters. */
    KU16            SelFlat;
    /** The 16-bit selector to use for the segment.
     * Primarily a way for the user to specify selectors for the LX/LE and NE interpreters. */
    KU16            Sel16bit;
    /** Segment flags. */
    KU32            fFlags;
    /** The segment protection. */
    KPROT           enmProt;
    /** The size of the segment. */
    KLDRSIZE        cb;
    /** The required segment alignment.
     * The to 0 if the segment isn't supposed to be mapped. */
    KLDRADDR        Alignment;
    /** The link address.
     * Set to NIL_KLDRADDR if the segment isn't supposed to be
     * mapped or if the image doesn't have link addresses. */
    KLDRADDR        LinkAddress;
    /** File offset of the segment.
     * Set to -1 if no file backing (like BSS). */
    KLDRFOFF        offFile;
    /** Size of the file bits of the segment.
     * Set to -1 if no file backing (like BSS). */
    KLDRFOFF        cbFile;
    /** The relative virtual address when mapped.
     * Set to NIL_KLDRADDR if the segment isn't supposed to be mapped. */
    KLDRADDR        RVA;
    /** The size of the segment including the alignment gap up to the next segment when mapped. */
    KSIZE           cbMapped;
    /** The address the segment was mapped at by kLdrModMap().
     * Set to 0 if not mapped. */
    KUPTR           MapAddress;
} KLDRSEG;


/** @name Segment flags
 * @{ */
/** The segment is 16-bit. When not set the default of the target architecture is assumed. */
#define KLDRSEG_FLAG_16BIT          1
/** The segment requires a 16-bit selector alias. (OS/2) */
#define KLDRSEG_FLAG_OS2_ALIAS16    2
/** Conforming segment (x86 weirdness). (OS/2) */
#define KLDRSEG_FLAG_OS2_CONFORM    4
/** IOPL (ring-2) segment. (OS/2) */
#define KLDRSEG_FLAG_OS2_IOPL       8
/** @} */


/**
 * Loader module format.
 */
typedef enum KLDRFMT
{
    /** The usual invalid 0 format. */
    KLDRFMT_INVALID = 0,
    /** The native OS loader. */
    KLDRFMT_NATIVE,
    /** The AOUT loader. */
    KLDRFMT_AOUT,
    /** The ELF loader. */
    KLDRFMT_ELF,
    /** The LX loader. */
    KLDRFMT_LX,
    /** The Mach-O loader. */
    KLDRFMT_MACHO,
    /** The PE loader. */
    KLDRFMT_PE,
    /** The end of the valid format values (exclusive). */
    KLDRFMT_END,
    /** Hack to blow the type up to 32-bit. */
    KLDRFMT_32BIT_HACK = 0x7fffffff
} KLDRFMT;


/**
 * Loader module type.
 */
typedef enum KLDRTYPE
{
    /** The usual invalid 0 type. */
    KLDRTYPE_INVALID = 0,
    /** Object file. */
    KLDRTYPE_OBJECT,
    /** Executable module, fixed load address. */
    KLDRTYPE_EXECUTABLE_FIXED,
    /** Executable module, relocatable, non-fixed load address. */
    KLDRTYPE_EXECUTABLE_RELOCATABLE,
    /** Executable module, position independent code, non-fixed load address. */
    KLDRTYPE_EXECUTABLE_PIC,
    /** Shared library, fixed load address.
     * Typically a system library. */
    KLDRTYPE_SHARED_LIBRARY_FIXED,
    /** Shared library, relocatable, non-fixed load address. */
    KLDRTYPE_SHARED_LIBRARY_RELOCATABLE,
    /** Shared library, position independent code, non-fixed load address. */
    KLDRTYPE_SHARED_LIBRARY_PIC,
    /** DLL that contains no code or data only imports and exports. (Chiefly OS/2.) */
    KLDRTYPE_FORWARDER_DLL,
    /** Core or dump. */
    KLDRTYPE_CORE,
    /** Debug module (debug info with empty code & data segments). */
    KLDRTYPE_DEBUG_INFO,
    /** The end of the valid types values (exclusive). */
    KLDRTYPE_END,
    /** Hack to blow the type up to 32-bit. */
    KLDRTYPE_32BIT_HACK = 0x7fffffff
} KLDRTYPE;


/**
 * Loader endian indicator.
 */
typedef enum KLDRENDIAN
{
    /** The usual invalid endian. */
    KLDRENDIAN_INVALID,
    /** Little endian. */
    KLDRENDIAN_LITTLE,
    /** Bit endian. */
    KLDRENDIAN_BIG,
    /** Endianness doesn't have a meaning in the context. */
    KLDRENDIAN_NA,
    /** The end of the valid endian values (exclusive). */
    KLDRENDIAN_END,
    /** Hack to blow the type up to 32-bit. */
    KLDRENDIAN_32BIT_HACK = 0x7fffffff
} KLDRENDIAN;


/** @name KLDRMOD::fFlags
 * @{ */
/** The link address doesn't form a contiguous image, from the first to the
 * last segment.   */
#define KLDRMOD_FLAGS_NON_CONTIGUOUS_LINK_ADDRS         K_BIT32(0)
/** @} */

/** Pointer to a module interpreter method table. */
typedef struct KLDRMODOPS *PKLDRMODOPS;
/** Pointer to const module interpreter methods table. */
typedef const struct KLDRMODOPS *PCKLDRMODOPS;

/**
 * Module interpreter instance.
 * All members are read only unless you're kLdrMod or the module interpreter.
 */
typedef struct KLDRMOD
{
    /** Magic number (KLDRMOD_MAGIC). */
    KU32                u32Magic;
    /** The format of this module. */
    KLDRFMT             enmFmt;
    /** The type of module. */
    KLDRTYPE            enmType;
    /** The CPU architecture this module was built for. */
    KCPUARCH            enmArch;
    /** The minium cpu this module was built for.
     * This might not be accurate, so use kLdrModCanExecuteOn() to check. */
    KCPU                enmCpu;
    /** The endian used by the module. */
    KLDRENDIAN          enmEndian;
    /** Module falgs. */
    KU32                fFlags;
    /** The filename length (bytes). */
    KU32                cchFilename;
    /** The filename. */
    const char         *pszFilename;
    /** The module name. */
    const char         *pszName;
    /** The module name length (bytes). */
    KU32                cchName;
    /** The number of segments in the module. */
    KU32                cSegments;
    /** Pointer to the loader methods.
     * Not meant for calling directly thru! */
    PCKLDRMODOPS        pOps;
    /** Pointer to the read instance. (Can be NULL after kLdrModDone().)*/
    PKRDR               pRdr;
    /** The module data. */
    void               *pvData;
    /** Segments. (variable size, can be zero) */
    KLDRSEG             aSegments[1];
} KLDRMOD, *PKLDRMOD, **PPKLDRMOD;

/** The magic for KLDRMOD::u32Magic. (Kosuke Fujishima) */
#define KLDRMOD_MAGIC   0x19640707


/** Special base address value alias for the link address. */
#define KLDRMOD_BASEADDRESS_LINK            (~(KLDRADDR)1)
/** Special base address value alias for the actual load address (must be mapped). */
#define KLDRMOD_BASEADDRESS_MAP             (~(KLDRADDR)2)

/** Special import module ordinal value used to indicate that there is no
 * specific module associated with the requested symbol. */
#define NIL_KLDRMOD_IMPORT                  (~(KU32)0)

/** Special symbol ordinal value used to indicate that the symbol
 * only has a string name. */
#define NIL_KLDRMOD_SYM_ORDINAL             (~(KU32)0)


/** @name Load symbol kind flags.
 * @{ */
/** The bitness doesn't matter. */
#define KLDRSYMKIND_NO_BIT                  0x00000000
/** 16-bit symbol. */
#define KLDRSYMKIND_16BIT                   0x00000001
/** 32-bit symbol. */
#define KLDRSYMKIND_32BIT                   0x00000002
/** 64-bit symbol. */
#define KLDRSYMKIND_64BIT                   0x00000003
/** Mask out the bit.*/
#define KLDRSYMKIND_BIT_MASK                0x00000003
/** We don't know the type of symbol. */
#define KLDRSYMKIND_NO_TYPE                 0x00000000
/** The symbol is a code object (method/function/procedure/whateveryouwannacallit). */
#define KLDRSYMKIND_CODE                    0x00000010
/** The symbol is a data object. */
#define KLDRSYMKIND_DATA                    0x00000020
/** Mask out the symbol type. */
#define KLDRSYMKIND_TYPE_MASK               0x00000030
/** Valid symbol kind mask. */
#define KLDRSYMKIND_MASK                    0x00000033
/** Weak symbol. */
#define KLDRSYMKIND_WEAK                    0x00000100
/** Forwarder symbol. */
#define KLDRSYMKIND_FORWARDER               0x00000200
/** Request a flat symbol address. */
#define KLDRSYMKIND_REQ_FLAT                0x00000000
/** Request a segmented symbol address. */
#define KLDRSYMKIND_REQ_SEGMENTED           0x40000000
/** Request type mask. */
#define KLDRSYMKIND_REQ_TYPE_MASK           0x40000000
/** @} */

/** @name kLdrModEnumSymbols flags.
 * @{ */
/** Returns ALL kinds of symbols. The default is to only return public/exported symbols. */
#define KLDRMOD_ENUM_SYMS_FLAGS_ALL         0x00000001
/** @} */


/**
 * Callback for resolving imported symbols when applying fixups.
 *
 * @returns 0 on success and *pValue and *pfKind filled.
 * @returns Non-zero OS specific or kLdr status code on failure.
 *
 * @param   pMod        The module which fixups are begin applied.
 * @param   iImport     The import module ordinal number or NIL_KLDRMOD_IMPORT.
 * @param   iSymbol     The symbol ordinal number or NIL_KLDRMOD_SYM_ORDINAL.
 * @param   pchSymbol   The symbol name. Can be NULL if iSymbol isn't nil. Doesn't have to be null-terminated.
 * @param   cchSymbol   The length of the symbol.
 * @param   pszVersion  The symbol version. NULL if not versioned.
 * @param   puValue     Where to store the symbol value.
 * @param   pfKind      Where to store the symbol kind flags.
 * @param   pvUser      The user parameter specified to the relocation function.
 */
typedef int FNKLDRMODGETIMPORT(PKLDRMOD pMod, KU32 iImport, KU32 iSymbol, const char *pchSymbol, KSIZE cchSymbol,
                               const char *pszVersion, PKLDRADDR puValue, KU32 *pfKind, void *pvUser);
/** Pointer to a import callback. */
typedef FNKLDRMODGETIMPORT *PFNKLDRMODGETIMPORT;

/**
 * Symbol enumerator callback.
 *
 * @returns 0 if enumeration should continue.
 * @returns non-zero if the enumeration should stop. This status code will then be returned by kLdrModEnumSymbols().
 *
 * @param   pMod        The module which symbols are being enumerated.s
 * @param   iSymbol     The symbol ordinal number or NIL_KLDRMOD_SYM_ORDINAL.
 * @param   pchSymbol   The symbol name. This can be NULL if there is a symbol ordinal.
 *                      This can also be an empty string if the symbol doesn't have a name
 *                      or it's name has been stripped.
 *                      Important, this doesn't have to be a null-terminated string.
 * @param   cchSymbol   The length of the symbol.
 * @param   pszVersion  The symbol version. NULL if not versioned.
 * @param   uValue      The symbol value.
 * @param   fKind       The symbol kind flags.
 * @param   pvUser      The user parameter specified to kLdrModEnumSymbols().
 */
typedef int FNKLDRMODENUMSYMS(PKLDRMOD pMod, KU32 iSymbol, const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                              KLDRADDR uValue, KU32 fKind, void *pvUser);
/** Pointer to a symbol enumerator callback. */
typedef FNKLDRMODENUMSYMS *PFNKLDRMODENUMSYMS;

/**
 * Debug info enumerator callback.
 *
 * @returns 0 to continue the enumeration.
 * @returns non-zero if the enumeration should stop. This status code will then be returned by kLdrModEnumDbgInfo().
 *
 * @param   pMod        The module.
 * @param   iDbgInfo    The debug info ordinal number / id.
 * @param   enmType     The debug info type.
 * @param   iMajorVer   The major version number of the debug info format. -1 if unknow - implies invalid iMinorVer.
 * @param   iMinorVer   The minor version number of the debug info format. -1 when iMajorVer is -1.
 * @param   pszPartNm   The name of the debug info part, NULL if not applicable.
 * @param   offFile     The file offset *if* this type has one specific location in the executable image file.
 *                      This is -1 if there isn't any specific file location.
 * @param   LinkAddress The link address of the debug info if it's loadable. NIL_KLDRADDR if not loadable.
 * @param   cb          The size of the debug information. -1 is used if this isn't applicable.
 * @param   pszExtFile  This points to the name of an external file containing the debug info.
 *                      This is NULL if there isn't any external file.
 * @param   pvUser      The user parameter specified to kLdrModEnumDbgInfo.
 */
typedef int FNKLDRENUMDBG(PKLDRMOD pMod, KU32 iDbgInfo, KLDRDBGINFOTYPE enmType, KI16 iMajorVer, KI16 iMinorVer,
                          const char *pszPartNm, KLDRFOFF offFile, KLDRADDR LinkAddress, KLDRSIZE cb,
                          const char *pszExtFile, void *pvUser);
/** Pointer to a debug info enumerator callback. */
typedef FNKLDRENUMDBG *PFNKLDRENUMDBG;

/**
 * Resource enumerator callback.
 *
 * @returns 0 to continue the enumeration.
 * @returns non-zero if the enumeration should stop. This status code will then be returned by kLdrModEnumResources().
 *
 * @param   pMod        The module.
 * @param   idType      The resource type id. NIL_KLDRMOD_RSRC_TYPE_ID if no type id.
 * @param   pszType     The resource type name. NULL if no type name.
 * @param   idName      The resource id. NIL_KLDRMOD_RSRC_NAME_ID if no id.
 * @param   pszName     The resource name. NULL if no name.
 * @param   idLang      The language id.
 * @param   AddrRsrc    The address value for the resource.
 * @param   cbRsrc      The size of the resource.
 * @param   pvUser      The user parameter specified to kLdrModEnumDbgInfo.
 */
typedef int FNKLDRENUMRSRC(PKLDRMOD pMod, KU32 idType, const char *pszType, KU32 idName, const char *pszName,
                           KU32 idLang, KLDRADDR AddrRsrc, KLDRSIZE cbRsrc, void *pvUser);
/** Pointer to a resource enumerator callback. */
typedef FNKLDRENUMRSRC *PFNKLDRENUMRSRC;

/** NIL resource name ID. */
#define NIL_KLDRMOD_RSRC_NAME_ID    ( ~(KU32)0 )
/** NIL resource type ID. */
#define NIL_KLDRMOD_RSRC_TYPE_ID    ( ~(KU32)0 )
/** @name Language ID
 *
 * Except for the special IDs #defined here, the values are considered
 * format specific for now since it's only used by the PE resources.
 *
 * @{ */
/** NIL language ID. */
#define NIL_KLDR_LANG_ID                ( ~(KU32)0 )
/** Special language id value for matching any language. */
#define KLDR_LANG_ID_ANY                ( ~(KU32)1 )
/** Special language id value indicating language neutral. */
#define KLDR_LANG_ID_NEUTRAL            ( ~(KU32)2 )
/** Special language id value indicating user default language. */
#define KLDR_LANG_ID_USER_DEFAULT       ( ~(KU32)3 )
/** Special language id value indicating system default language. */
#define KLDR_LANG_ID_SYS_DEFAULT        ( ~(KU32)4 )
/** Special language id value indicating default custom locale. */
#define KLDR_LANG_ID_CUSTOM_DEFAULT     ( ~(KU32)5 )
/** Special language id value indicating unspecified custom locale. */
#define KLDR_LANG_ID_CUSTOM_UNSPECIFIED ( ~(KU32)6 )
/** Special language id value indicating default custom MUI locale. */
#define KLDR_LANG_ID_UI_CUSTOM_DEFAULT  ( ~(KU32)7 )
/** @} */

/** @name Module Open Flags
 * @{ */
/** Indicates that we won't be loading the module, we're just getting
 *  information (like symbols and line numbers) out of it. */
#define KLDRMOD_OPEN_FLAGS_FOR_INFO     K_BIT32(0)
/** Mask of valid flags.    */
#define KLDRMOD_OPEN_FLAGS_VALID_MASK   KU32_C(0x00000001)
/** @} */

int     kLdrModOpen(const char *pszFilename, KU32 fFlags, KCPUARCH enmCpuArch, PPKLDRMOD ppMod);
int     kLdrModOpenFromRdr(PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch, PPKLDRMOD ppMod);
int     kLdrModOpenNative(const char *pszFilename, PPKLDRMOD ppMod);
int     kLdrModOpenNativeByHandle(KUPTR uHandle, PPKLDRMOD ppMod);
int     kLdrModClose(PKLDRMOD pMod);

int     kLdrModQuerySymbol(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 iSymbol,
                           const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                           PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind);
int     kLdrModEnumSymbols(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress,
                           KU32 fFlags, PFNKLDRMODENUMSYMS pfnCallback, void *pvUser);
int     kLdrModGetImport(PKLDRMOD pMod, const void *pvBits, KU32 iImport, char *pszName, KSIZE cchName);
KI32    kLdrModNumberOfImports(PKLDRMOD pMod, const void *pvBits);
int     kLdrModCanExecuteOn(PKLDRMOD pMod, const void *pvBits, KCPUARCH enmArch, KCPU enmCpu);
int     kLdrModGetStackInfo(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRSTACKINFO pStackInfo);
int     kLdrModQueryMainEntrypoint(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRADDR pMainEPAddress);
int     kLdrModQueryImageUuid(PKLDRMOD pMod, const void *pvBits, void *pvUuid, KSIZE cbUuid);
int     kLdrModQueryResource(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 idType, const char *pszType,
                             KU32 idName, const char *pszName, KU32 idLang, PKLDRADDR pAddrRsrc, KSIZE *pcbRsrc);
int     kLdrModEnumResources(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 idType, const char *pszType,
                             KU32 idName, const char *pszName, KU32 idLang, PFNKLDRENUMRSRC pfnCallback, void *pvUser);
int     kLdrModEnumDbgInfo(PKLDRMOD pMod, const void *pvBits, PFNKLDRENUMDBG pfnCallback, void *pvUser);
int     kLdrModHasDbgInfo(PKLDRMOD pMod, const void *pvBits);
int     kLdrModMostlyDone(PKLDRMOD pMod);


/** @name Operations On The Internally Managed Mapping
 * @{ */
int     kLdrModMap(PKLDRMOD pMod);
int     kLdrModUnmap(PKLDRMOD pMod);
int     kLdrModReload(PKLDRMOD pMod);
int     kLdrModFixupMapping(PKLDRMOD pMod, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
/** @} */

/** @name Operations On The Externally Managed Mappings
 * @{ */
KLDRADDR kLdrModSize(PKLDRMOD pMod);
int     kLdrModGetBits(PKLDRMOD pMod, void *pvBits, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
int     kLdrModRelocateBits(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                            PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
/** @} */

/** @name Operations on both internally and externally managed mappings.
 * @{ */
/** Special pvMapping value to pass to kLdrModAllocTLS,
 * kLdrModFreeTLS, kLdrModCallInit, kLdrModCallTerm, and kLdrModCallThread that
 * specifies the internal mapping (kLdrModMap). */
#define KLDRMOD_INT_MAP    ((void *)~(KUPTR)0)
int     kLdrModAllocTLS(PKLDRMOD pMod, void *pvMapping);
void    kLdrModFreeTLS(PKLDRMOD pMod, void *pvMapping);
int     kLdrModCallInit(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle);
int     kLdrModCallTerm(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle);
int     kLdrModCallThread(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle, unsigned fAttachingOrDetaching);
/** @} */


/**
 * The loader module operation.
 */
typedef struct KLDRMODOPS
{
    /** The name of this module interpreter. */
    const char         *pszName;
    /** Pointer to the next module interpreter. */
    PCKLDRMODOPS        pNext;

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
    int (* pfnCreate)(PCKLDRMODOPS pOps, PKRDR pRdr, KU32 fFlags, KCPUARCH enmCpuArch, KLDRFOFF offNewHdr, PPKLDRMOD ppMod);
    /**
     * Destroys an loader module instance.
     *
     * The caller is responsible for calling kLdrModUnmap() and kLdrFreeTLS() first.
     *
     * @returns 0 on success, non-zero on failure. The module instance state
     *          is unknown on failure, it's best not to touch it.
     * @param   pMod    The module.
     */
    int (* pfnDestroy)(PKLDRMOD pMod);

    /** @copydoc kLdrModQuerySymbol */
    int (* pfnQuerySymbol)(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 iSymbol,
                           const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                           PFNKLDRMODGETIMPORT pfnGetForwarder, void *pvUser, PKLDRADDR puValue, KU32 *pfKind);
    /** @copydoc kLdrModEnumSymbols */
    int (* pfnEnumSymbols)(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 fFlags,
                           PFNKLDRMODENUMSYMS pfnCallback, void *pvUser);
    /** @copydoc kLdrModGetImport */
    int (* pfnGetImport)(PKLDRMOD pMod, const void *pvBits, KU32 iImport, char *pszName, KSIZE cchName);
    /** @copydoc kLdrModNumberOfImports */
    KI32 (* pfnNumberOfImports)(PKLDRMOD pMod, const void *pvBits);
    /** @copydoc kLdrModCanExecuteOn */
    int (* pfnCanExecuteOn)(PKLDRMOD pMod, const void *pvBits, KCPUARCH enmArch, KCPU enmCpu);
    /** @copydoc kLdrModGetStackInfo */
    int (* pfnGetStackInfo)(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRSTACKINFO pStackInfo);
    /** @copydoc kLdrModQueryMainEntrypoint */
    int (* pfnQueryMainEntrypoint)(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, PKLDRADDR pMainEPAddress);
    /** @copydoc kLdrModQueryImageUuid  */
    int (* pfnQueryImageUuid)(PKLDRMOD pMod, const void *pvBits, void *pvUuid, KSIZE pcbUuid);
    /** @copydoc kLdrModQueryResource */
    int (* pfnQueryResource)(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 idType, const char *pszType,
                             KU32 idName, const char *pszName, KU32 idLang, PKLDRADDR pAddrRsrc, KSIZE *pcbRsrc);
    /** @copydoc kLdrModEnumResources */
    int (* pfnEnumResources)(PKLDRMOD pMod, const void *pvBits, KLDRADDR BaseAddress, KU32 idType, const char *pszType,
                             KU32 idName, const char *pszName, KU32 idLang, PFNKLDRENUMRSRC pfnCallback, void *pvUser);
    /** @copydoc kLdrModEnumDbgInfo */
    int (* pfnEnumDbgInfo)(PKLDRMOD pMod, const void *pvBits, PFNKLDRENUMDBG pfnCallback, void *pvUser);
    /** @copydoc kLdrModHasDbgInfo */
    int (* pfnHasDbgInfo)(PKLDRMOD pMod, const void *pvBits);
    /** @copydoc kLdrModMap */
    int (* pfnMap)(PKLDRMOD pMod);
    /** @copydoc kLdrModUnmap */
    int (* pfnUnmap)(PKLDRMOD pMod);
    /** @copydoc kLdrModAllocTLS */
    int (* pfnAllocTLS)(PKLDRMOD pMod, void *pvMapping);
    /** @copydoc kLdrModFreeTLS */
    void (*pfnFreeTLS)(PKLDRMOD pMod, void *pvMapping);
    /** @copydoc kLdrModReload */
    int (* pfnReload)(PKLDRMOD pMod);
    /** @copydoc kLdrModFixupMapping */
    int (* pfnFixupMapping)(PKLDRMOD pMod, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
    /** @copydoc kLdrModCallInit */
    int (* pfnCallInit)(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle);
    /** @copydoc kLdrModCallTerm */
    int (* pfnCallTerm)(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle);
    /** @copydoc kLdrModCallThread */
    int (* pfnCallThread)(PKLDRMOD pMod, void *pvMapping, KUPTR uHandle, unsigned fAttachingOrDetaching);
    /** @copydoc kLdrModSize */
    KLDRADDR (* pfnSize)(PKLDRMOD pMod);
    /** @copydoc kLdrModGetBits */
    int (* pfnGetBits)(PKLDRMOD pMod, void *pvBits, KLDRADDR BaseAddress, PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
    /** @copydoc kLdrModRelocateBits */
    int (* pfnRelocateBits)(PKLDRMOD pMod, void *pvBits, KLDRADDR NewBaseAddress, KLDRADDR OldBaseAddress,
                            PFNKLDRMODGETIMPORT pfnGetImport, void *pvUser);
    /** @copydoc kLdrModMostlyDone */
    int (* pfnMostlyDone)(PKLDRMOD pMod);
    /** Dummy which should be assigned a non-zero value. */
    KU32 uEndOfStructure;
} KLDRMODOPS;


/** @} */




/** @defgroup grp_kLdrDyld   kLdrDyld - The dynamic loader
 * @{ */

/** The handle to a dynamic loader module. */
typedef struct KLDRDYLDMOD *HKLDRMOD;
/** Pointer to the handle to a dynamic loader module. */
typedef HKLDRMOD *PHKLDRMOD;
/** NIL handle value. */
#define NIL_HKLDRMOD    ((HKLDRMOD)0)


/**
 * File search method.
 *
 * In addition to it's own way of finding files, kLdr emulates
 * the methods employed by the most popular systems.
 */
typedef enum KLDRDYLDSEARCH
{
    /** The usual invalid file search method. */
    KLDRDYLD_SEARCH_INVALID = 0,
    /** Uses the kLdr file search method.
     * @todo invent me. */
    KLDRDYLD_SEARCH_KLDR,
    /** Use the emulation closest to the host system. */
    KLDRDYLD_SEARCH_HOST,
    /** Emulate the OS/2 file search method.
     * On non-OS/2 systems, BEGINLIBPATH, LIBPATH, ENDLIBPATH and LIBPATHSTRICT are
     * taken form the environment. */
    KLDRDYLD_SEARCH_OS2,
    /** Emulate the standard window file search method. */
    KLDRDYLD_SEARCH_WINDOWS,
    /** Emulate the alternative window file search method. */
    KLDRDYLD_SEARCH_WINDOWS_ALTERED,
    /** Emulate the most common UNIX file search method. */
    KLDRDYLD_SEARCH_UNIX_COMMON,
    /** End of the valid file search method values. */
    KLDRDYLD_SEARCH_END,
    /** Hack to blow the type up to 32-bit. */
    KLDRDYLD_SEARCH_32BIT_HACK = 0x7fffffff
} KLDRDYLDSEARCH;

/** @name kLdrDyldLoad and kLdrDyldFindByName flags.
 * @{ */
/** The symbols in the module should be loaded into the global unix namespace.
 * If not specified, the symbols are local and can only be referenced directly. */
#define KLDRYDLD_LOAD_FLAGS_GLOBAL_SYMBOLS      0x00000001
/** The symbols in the module should be loaded into the global unix namespace and
 * it's symbols should take precedence over all currently loaded modules.
 * This implies KLDRYDLD_LOAD_FLAGS_GLOBAL_SYMBOLS. */
#define KLDRYDLD_LOAD_FLAGS_DEEP_SYMBOLS        0x00000002
/** The module shouldn't be found by a global module search.
 * If not specified, the module can be found by unspecified module searches,
 * typical used when loading import/dep modules. */
#define KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE     0x00000004
/** Do a recursive initialization calls instead of defering them to the outermost call. */
#define KLDRDYLD_LOAD_FLAGS_RECURSIVE_INIT      0x00000008
/** We're loading the executable module.
 * @internal */
#define KLDRDYLD_LOAD_FLAGS_EXECUTABLE          0x40000000
/** @} */


int     kLdrDyldLoad(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                     unsigned fFlags, PHKLDRMOD phMod, char *pszErr, KSIZE cchErr);
int     kLdrDyldUnload(HKLDRMOD hMod);
int     kLdrDyldFindByName(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                           unsigned fFlags, PHKLDRMOD phMod);
int     kLdrDyldFindByAddress(KUPTR Address, PHKLDRMOD phMod, KU32 *piSegment, KUPTR *poffSegment);
int     kLdrDyldGetName(HKLDRMOD hMod, char *pszName, KSIZE cchName);
int     kLdrDyldGetFilename(HKLDRMOD hMod, char *pszFilename, KSIZE cchFilename);
int     kLdrDyldQuerySymbol(HKLDRMOD hMod, KU32 uSymbolOrdinal, const char *pszSymbolName,
                            const char *pszSymbolVersion, KUPTR *pValue, KU32 *pfKind);
int     kLdrDyldQueryResource(HKLDRMOD hMod, KU32 idType, const char *pszType, KU32 idName,
                              const char *pszName, KU32 idLang, void **pvRsrc, KSIZE *pcbRsrc);
int     kLdrDyldEnumResources(HKLDRMOD hMod, KU32 idType, const char *pszType, KU32 idName,
                              const char *pszName, KU32 idLang, PFNKLDRENUMRSRC pfnCallback, void *pvUser);


/** @name OS/2 like API
 * @{ */
#if defined(__OS2__)
# define KLDROS2API _System
#else
# define KLDROS2API
#endif
int     kLdrDosLoadModule(char *pszObject, KSIZE cbObject, const char *pszModule, PHKLDRMOD phMod);
int     kLdrDosFreeModule(HKLDRMOD hMod);
int     kLdrDosQueryModuleHandle(const char *pszModname, PHKLDRMOD phMod);
int     kLdrDosQueryModuleName(HKLDRMOD hMod, KSIZE cchName, char *pszName);
int     kLdrDosQueryProcAddr(HKLDRMOD hMod, KU32 iOrdinal, const char *pszProcName, void **ppvProcAddr);
int     kLdrDosQueryProcType(HKLDRMOD hMod, KU32 iOrdinal, const char *pszProcName, KU32 *pfProcType);
int     kLdrDosQueryModFromEIP(PHKLDRMOD phMod, KU32 *piObject, KSIZE cbName, char *pszName, KUPTR *poffObject, KUPTR ulEIP);
int     kLdrDosReplaceModule(const char *pszOldModule, const char *pszNewModule, const char *pszBackupModule);
int     kLdrDosGetResource(HKLDRMOD hMod, KU32 idType, KU32 idName, void **pvResAddr);
int     kLdrDosQueryResourceSize(HKLDRMOD hMod, KU32 idType, KU32 idName, KU32 *pcb);
int     kLdrDosFreeResource(void *pvResAddr);
/** @} */

/** @name POSIX like API
 * @{ */
HKLDRMOD    kLdrDlOpen(const char *pszLibrary, int fFlags);
const char *kLdrDlError(void);
void *      kLdrDlSym(HKLDRMOD hMod, const char *pszSymbol);
int         kLdrDlClose(HKLDRMOD hMod);
/** @todo GNU extensions */
/** @} */

/** @name Win32 like API
 * @{ */
#if defined(_MSC_VER)
# define KLDRWINAPI __stdcall
#else
# define KLDRWINAPI
#endif
HKLDRMOD KLDRWINAPI kLdrWLoadLibrary(const char *pszFilename);
HKLDRMOD KLDRWINAPI kLdrWLoadLibraryEx(const char *pszFilename, void *hFileReserved, KU32 fFlags);
KU32     KLDRWINAPI kLdrWGetModuleFileName(HKLDRMOD hMod, char *pszModName, KSIZE cchModName);
HKLDRMOD KLDRWINAPI kLdrWGetModuleHandle(const char *pszFilename);
int      KLDRWINAPI kLdrWGetModuleHandleEx(KU32 fFlags, const char *pszFilename, HKLDRMOD hMod);
void *   KLDRWINAPI kLdrWGetProcAddress(HKLDRMOD hMod, const char *pszProcName);
KU32     KLDRWINAPI kLdrWGetDllDirectory(KSIZE cchDir, char *pszDir);
int      KLDRWINAPI kLdrWSetDllDirectory(const char *pszDir);
int      KLDRWINAPI kLdrWFreeLibrary(HKLDRMOD hMod);
int      KLDRWINAPI kLdrWDisableThreadLibraryCalls(HKLDRMOD hMod);

/** The handle to a resource that's been found. */
typedef struct KLDRWRSRCFOUND *HKLDRWRSRCFOUND;
/** The handle to a loaded resource. */
typedef struct KLDRWRSRCLOADED *HKLDRWRSRCLOADED;
HKLDRWRSRCFOUND  KLDRWINAPI kLdrWFindResource(HKLDRMOD hMod, const char *pszType, const char *pszName);
HKLDRWRSRCFOUND  KLDRWINAPI kLdrWFindResourceEx(HKLDRMOD hMod, const char *pszType, const char *pszName, KU16 idLang);
KU32             KLDRWINAPI kLdrWSizeofResource(HKLDRMOD hMod, HKLDRWRSRCFOUND hFoundRsrc);
HKLDRWRSRCLOADED KLDRWINAPI kLdrWLoadResource(HKLDRMOD hMod, HKLDRWRSRCFOUND hFoundRsrc);
void    *KLDRWINAPI kLdrWLockResource(HKLDRMOD hMod, HKLDRWRSRCLOADED hLoadedRsrc);
int      KLDRWINAPI kLdrWFreeResource(HKLDRMOD hMod, HKLDRWRSRCLOADED hLoadedRsrc);

typedef int (KLDRWINAPI *PFNKLDRWENUMRESTYPE)(HKLDRMOD hMod, const char *pszType, KUPTR uUser);
int      KLDRWINAPI kLdrWEnumResourceTypes(HKLDRMOD hMod, PFNKLDRWENUMRESTYPE pfnEnum, KUPTR uUser);
int      KLDRWINAPI kLdrWEnumResourceTypesEx(HKLDRMOD hMod, PFNKLDRWENUMRESTYPE pfnEnum, KUPTR uUser, KU32 fFlags, KU16 idLang);

typedef int (KLDRWINAPI *PFNKLDRWENUMRESNAME)(HKLDRMOD hMod, const char *pszType, char *pszName, KUPTR uUser);
int      KLDRWINAPI kLdrWEnumResourceNames(HKLDRMOD hMod, const char *pszType, PFNKLDRWENUMRESNAME pfnEnum, KUPTR uUser);
int      KLDRWINAPI kLdrWEnumResourceNamesEx(HKLDRMOD hMod, const char *pszType, PFNKLDRWENUMRESNAME pfnEnum, KUPTR uUser, KU32 fFlags, KU16 idLang);

typedef int (KLDRWINAPI *PFNKLDRWENUMRESLANG)(HKLDRMOD hMod, const char *pszType, const char *pszName, KU16 idLang, KUPTR uUser);
int      KLDRWINAPI kLdrWEnumResourceLanguages(HKLDRMOD hMod, const char *pszType, const char *pszName, PFNKLDRWENUMRESLANG pfnEnum, KUPTR uUser);
int      KLDRWINAPI kLdrWEnumResourceLanguagesEx(HKLDRMOD hMod, const char *pszType, const char *pszName,
                                                 PFNKLDRWENUMRESLANG pfnEnum, KUPTR uUser, KU32 fFlags, KU16 idLang);
/** @} */


/** @name Process Bootstrapping
 * @{ */

/**
 * Argument package from the stub.
 */
typedef struct KLDREXEARGS
{
    /** Load & search flags, some which will become defaults. */
    KU32            fFlags;
    /** The default search method. */
    KLDRDYLDSEARCH  enmSearch;
    /** The executable file that the stub is supposed to load. */
    char            szExecutable[260];
    /** The default prefix used when searching for DLLs. */
    char            szDefPrefix[16];
    /** The default suffix used when searching for DLLs. */
    char            szDefSuffix[16];
    /** The LD_LIBRARY_PATH prefix for the process.. */
    char            szLibPath[4096 - sizeof(KU32) - sizeof(KLDRDYLDSEARCH) - 16 - 16 - 260];
} KLDREXEARGS, *PKLDREXEARGS;
/** Pointer to a const argument package from the stub. */
typedef const KLDREXEARGS *PCKLDREXEARGS;

void kLdrLoadExe(PCKLDREXEARGS pArgs, void *pvOS); /** @todo fix this mess... */
void kLdrDyldLoadExe(PCKLDREXEARGS pArgs, void *pvOS);
/** @} */

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif


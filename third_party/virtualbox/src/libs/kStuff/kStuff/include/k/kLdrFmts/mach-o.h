/* $Id: mach-o.h 63 2013-10-30 02:00:14Z bird $ */
/** @file
 * Mach-0 structures, types and defines.
 */

/*
 * Copyright (c) 2006-2012 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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

#ifndef ___k_kLdrFmts_mach_o_h___
#define ___k_kLdrFmts_mach_o_h___

#include <k/kDefs.h>
#include <k/kTypes.h>


/** @defgroup grp_mach_o    The Mach-O Structures, Types, and Defines.
 * @{
 */


#ifndef IMAGE_FAT_SIGNATURE
/** The FAT signature (universal binaries). */
# define IMAGE_FAT_SIGNATURE            KU32_C(0xcafebabe)
#endif
#ifndef IMAGE_FAT_SIGNATURE_OE
/** The FAT signature (universal binaries), other endian. */
# define IMAGE_FAT_SIGNATURE_OE         KU32_C(0xbebafeca)
#endif

/**
 * The fat header found at the start of universal binaries.
 * It is followed by \a nfat_arch numbers of \a fat_arch structures.
 */
typedef struct fat_header
{
    KU32            magic;
    KU32            nfat_arch;
} fat_header_t;

/**
 * Description of fat file item.
 */
typedef struct fat_arch
{
    KI32            cputype;
    KI32            cpusubtype;
    KU32            offset;
    KU32            size;
    KU32            align;          /**< Power of 2. */
} fat_arch_t;



#ifndef IMAGE_MACHO32_SIGNATURE
/** The 32-bit Mach-O signature. */
# define IMAGE_MACHO32_SIGNATURE        KU32_C(0xfeedface)
#endif
#ifndef IMAGE_MACHO32_SIGNATURE_OE
/** The 32-bit Mach-O signature, other endian. */
# define IMAGE_MACHO32_SIGNATURE_OE     KU32_C(0xcefaedfe)
#endif
#define MH_MAGIC    IMAGE_MACHO32_SIGNATURE
#define MH_CIGAM    IMAGE_MACHO32_SIGNATURE_OE

/**
 * 32-bit Mach-O header.
 * This is followed by \a ncmds number of load commands.
 * @see mach_header_64
 */
typedef struct mach_header_32
{
    KU32            magic;
    KI32            cputype;
    KI32            cpusubtype;
    KU32            filetype;
    KU32            ncmds;
    KU32            sizeofcmds;
    KU32            flags;
} mach_header_32_t;



#ifndef IMAGE_MACHO64_SIGNATURE
/** The 64-bit Mach-O signature. */
# define IMAGE_MACHO64_SIGNATURE        KU32_C(0xfeedfacf)
#endif
#ifndef IMAGE_MACHO64_SIGNATURE_OE
/** The 64-bit Mach-O signature, other endian. */
# define IMAGE_MACHO64_SIGNATURE_OE     KU32_C(0xfefaedfe)
#endif
#define MH_MAGIC_64 IMAGE_MACHO64_SIGNATURE
#define MH_CIGAM_64 IMAGE_MACHO64_SIGNATURE_OE

/**
 * 64-bit Mach-O header.
 * This is followed by \a ncmds number of load commands.
 * @see mach_header
 */
typedef struct mach_header_64
{
    KU32            magic;
    KI32            cputype;
    KI32            cpusubtype;
    KU32            filetype;
    KU32            ncmds;
    KU32            sizeofcmds;
    KU32            flags;
    KU32            reserved;       /**< (for proper struct and command alignment I guess) */
} mach_header_64_t;


/** @name File types (mach_header_64::filetype, mach_header_32::filetype)
 * @{
 */
#define MH_OBJECT           KU32_C(1) /**< Object (relocatable). */
#define MH_EXECUTE          KU32_C(2) /**< Executable (demand paged). */
#define MH_FVMLIB           KU32_C(3) /**< Fixed VM shared library. */
#define MH_CORE             KU32_C(4) /**< Core file. */
#define MH_PRELOAD          KU32_C(5) /**< Preloaded executable. */
#define MH_DYLIB            KU32_C(6) /**< Dynamically bound shared library. */
#define MH_DYLINKER         KU32_C(7) /**< Dynamic linker. */
#define MH_BUNDLE           KU32_C(8) /**< Dymamically bound bundle. */
#define MH_DYLIB_STUB       KU32_C(9) /**< Shared library stub for static linking. */
#define MH_DSYM             KU32_C(10)/**< Debug symbols. */
#define MH_KEXT_BUNDLE      KU32_C(11)/**< Kernel extension (introduced with the AMD64 kernel). */

/** @} */


/** @name Mach-O Header flags (mach_header_64::flags, mach_header_32::flags)
 * @{
 */
#define MH_NOUNDEFS                 KU32_C(0x00000001) /**< No undefined symbols. */
#define MH_INCRLINK                 KU32_C(0x00000002) /**< Partial increment link output. */
#define MH_DYLDLINK                 KU32_C(0x00000004) /**< Food for the dynamic linker, not for ld. */
#define MH_BINDATLOAD               KU32_C(0x00000008) /**< Bind all undefined symbols at load time. */
#define MH_PREBOUND                 KU32_C(0x00000010) /**< Contains prebound undefined symbols. */
#define MH_SPLIT_SEGS               KU32_C(0x00000020) /**< Read-only and read-write segments are split. */
#define MH_LAZY_INIT                KU32_C(0x00000040) /**< Obsolete flag for doing lazy init when data is written. */
#define MH_TWOLEVEL                 KU32_C(0x00000080) /**< Uses two-level name space bindings. */
#define MH_FORCE_FLAT               KU32_C(0x00000100) /**< Task: The executable forces all images to use flat name space bindings. */
#define MH_NOMULTIDEFS              KU32_C(0x00000200) /**< No multiple symbol definitions, safe to use two-level namespace hints. */
#define MH_NOFIXPREBINDING          KU32_C(0x00000400) /**< The dynamic linker should not notify the prebinding agent about this executable. */
#define MH_PREBINDABLE              KU32_C(0x00000800) /**< Not prebound, but it can be. Invalid if MH_PREBOUND is set. */
#define MH_ALLMODSBOUND             KU32_C(0x00001000) /**< Binds to all two-level namespace modules of preqs. Requires MH_PREBINDABLE and MH_TWOLEVEL to be set. */
#define MH_SUBSECTIONS_VIA_SYMBOLS  KU32_C(0x00002000) /**< Safe to divide sections into sub-sections via symbols for dead code stripping. */
#define MH_CANONICAL                KU32_C(0x00004000) /**< Canonicalized via unprebind. */
#define MH_WEAK_DEFINES             KU32_C(0x00008000) /**< The (finally) linked image has weak symbols. */
#define MH_BINDS_TO_WEAK            KU32_C(0x00010000) /**< The (finally) linked image uses weak symbols. */
#define MH_ALLOW_STACK_EXECUTION    KU32_C(0x00020000) /**< Task: allow stack execution. (MH_EXECUTE only) */
#define MH_ROOT_SAFE                KU32_C(0x00040000) /**< Binary safe for root execution. */
#define MH_SETUID_SAFE              KU32_C(0x00080000) /**< Binary safe for set-uid execution. */
#define MH_NO_REEXPORTED_DYLIBS     KU32_C(0x00100000) /**< No reexported dylibs. */
#define MH_PIE                      KU32_C(0x00200000) /**< Address space randomization. (MH_EXECUTE only) */
#define MH_DEAD_STRIPPABLE_DYLIB    KU32_C(0x00400000) /**< Drop dylib dependency if not used. (MH_DYLIB only) */
#define MH_HAS_TLV_DESCRIPTORS      KU32_C(0x00800000) /**< Has a S_TRHEAD_LOCAL_VARIABLES section.  TLS support. */
#define MH_NO_HEAP_EXECUTION        KU32_C(0x01000000) /**< Task: no heap execution. (MH_EXECUTE only) */
#define MH_VALID_FLAGS              KU32_C(0x01ffffff) /**< Mask containing the defined flags. */
/** @} */


/** @name CPU types / bits (mach_header_64::cputype, mach_header_32::cputype, fat_arch::cputype)
 * @{
 */
#define CPU_ARCH_MASK               KI32_C(0xff000000)
#define CPU_ARCH_ABI64              KI32_C(0x01000000)
#define CPU_TYPE_ANY                KI32_C(-1)
#define CPU_TYPE_VAX                KI32_C(1)
#define CPU_TYPE_MC680x0            KI32_C(6)
#define CPU_TYPE_X86                KI32_C(7)
#define CPU_TYPE_I386               CPU_TYPE_X86
#define CPU_TYPE_X86_64             (CPU_TYPE_X86 | CPU_ARCH_ABI64)
#define CPU_TYPE_MC98000            KI32_C(10)
#define CPU_TYPE_HPPA               KI32_C(11)
#define CPU_TYPE_MC88000            KI32_C(13)
#define CPU_TYPE_SPARC              KI32_C(14)
#define CPU_TYPE_I860               KI32_C(15)
#define CPU_TYPE_POWERPC            KI32_C(18)
#define CPU_TYPE_POWERPC64          (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)
/** @} */


/** @name CPU subtypes (mach_header_64::cpusubtype, mach_header_32::cpusubtype, fat_arch::cpusubtype)
 * @{ */
#define CPU_SUBTYPE_MULTIPLE        KI32_C(-1)
#define CPU_SUBTYPE_LITTLE_ENDIAN   KI32_C(0)  /**< figure this one out. */
#define CPU_SUBTYPE_BIG_ENDIAN      KI32_C(1)  /**< ditto */

/* VAX */
#define CPU_SUBTYPE_VAX_ALL         KI32_C(0)
#define CPU_SUBTYPE_VAX780          KI32_C(1)
#define CPU_SUBTYPE_VAX785          KI32_C(2)
#define CPU_SUBTYPE_VAX750          KI32_C(3)
#define CPU_SUBTYPE_VAX730          KI32_C(4)
#define CPU_SUBTYPE_UVAXI           KI32_C(5)
#define CPU_SUBTYPE_UVAXII          KI32_C(6)
#define CPU_SUBTYPE_VAX8200         KI32_C(7)
#define CPU_SUBTYPE_VAX8500         KI32_C(8)
#define CPU_SUBTYPE_VAX8600         KI32_C(9)
#define CPU_SUBTYPE_VAX8650         KI32_C(10)
#define CPU_SUBTYPE_VAX8800         KI32_C(11)
#define CPU_SUBTYPE_UVAXIII         KI32_C(12)

/* MC680xx */
#define CPU_SUBTYPE_MC680x0_ALL     KI32_C(1)
#define CPU_SUBTYPE_MC68030         KI32_C(1)
#define CPU_SUBTYPE_MC68040         KI32_C(2)
#define CPU_SUBTYPE_MC68030_ONLY    KI32_C(3)

/* I386 */
#define CPU_SUBTYPE_INTEL(fam, model)       ( (KI32)(((model) << 4) | (fam)) )
#define CPU_SUBTYPE_INTEL_FAMILY(subtype)   ( (subtype) & 0xf )
#define CPU_SUBTYPE_INTEL_MODEL(subtype)    ( (subtype) >> 4 )
#define CPU_SUBTYPE_INTEL_FAMILY_MAX        0xf
#define CPU_SUBTYPE_INTEL_MODEL_ALL         0

#define CPU_SUBTYPE_I386_ALL        CPU_SUBTYPE_INTEL(3, 0)
#define CPU_SUBTYPE_386             CPU_SUBTYPE_INTEL(3, 0)
#define CPU_SUBTYPE_486             CPU_SUBTYPE_INTEL(4, 0)
#define CPU_SUBTYPE_486SX           CPU_SUBTYPE_INTEL(4, 8)
#define CPU_SUBTYPE_586             CPU_SUBTYPE_INTEL(5, 0)
#define CPU_SUBTYPE_PENT            CPU_SUBTYPE_INTEL(5, 0)
#define CPU_SUBTYPE_PENTPRO         CPU_SUBTYPE_INTEL(6, 1)
#define CPU_SUBTYPE_PENTII_M3       CPU_SUBTYPE_INTEL(6, 3)
#define CPU_SUBTYPE_PENTII_M5       CPU_SUBTYPE_INTEL(6, 5)
#define CPU_SUBTYPE_CELERON         CPU_SUBTYPE_INTEL(7, 6)
#define CPU_SUBTYPE_CELERON_MOBILE  CPU_SUBTYPE_INTEL(7, 7)
#define CPU_SUBTYPE_PENTIUM_3       CPU_SUBTYPE_INTEL(8, 0)
#define CPU_SUBTYPE_PENTIUM_3_M     CPU_SUBTYPE_INTEL(8, 1)
#define CPU_SUBTYPE_PENTIUM_3_XEON  CPU_SUBTYPE_INTEL(8, 2)
#define CPU_SUBTYPE_PENTIUM_M       CPU_SUBTYPE_INTEL(9, 0)
#define CPU_SUBTYPE_PENTIUM_4       CPU_SUBTYPE_INTEL(10, 0)
#define CPU_SUBTYPE_PENTIUM_4_M     CPU_SUBTYPE_INTEL(10, 1)
#define CPU_SUBTYPE_ITANIUM         CPU_SUBTYPE_INTEL(11, 0)
#define CPU_SUBTYPE_ITANIUM_2       CPU_SUBTYPE_INTEL(11, 1)
#define CPU_SUBTYPE_XEON            CPU_SUBTYPE_INTEL(12, 0)
#define CPU_SUBTYPE_XEON_MP         CPU_SUBTYPE_INTEL(12, 1)

/* X86 */
#define CPU_SUBTYPE_X86_ALL         KI32_C(3) /* CPU_SUBTYPE_I386_ALL */
#define CPU_SUBTYPE_X86_64_ALL      KI32_C(3) /* CPU_SUBTYPE_I386_ALL */
#define CPU_SUBTYPE_X86_ARCH1       KI32_C(4) /* CPU_SUBTYPE_I486_ALL */

/* MIPS */
#define CPU_SUBTYPE_MIPS_ALL        KI32_C(0)
#define CPU_SUBTYPE_MIPS_R2300      KI32_C(1)
#define CPU_SUBTYPE_MIPS_R2600      KI32_C(2)
#define CPU_SUBTYPE_MIPS_R2800      KI32_C(3)
#define CPU_SUBTYPE_MIPS_R2000a     KI32_C(4)
#define CPU_SUBTYPE_MIPS_R2000      KI32_C(5)
#define CPU_SUBTYPE_MIPS_R3000a     KI32_C(6)
#define CPU_SUBTYPE_MIPS_R3000      KI32_C(7)

/* MC98000 (PowerPC) */
#define CPU_SUBTYPE_MC98000_ALL     KI32_C(0)
#define CPU_SUBTYPE_MC98601         KI32_C(1)

/* HP-PA */
#define CPU_SUBTYPE_HPPA_ALL        KI32_C(0)
#define CPU_SUBTYPE_HPPA_7100       KI32_C(0)
#define CPU_SUBTYPE_HPPA_7100LC     KI32_C(1)

/* MC88000 */
#define CPU_SUBTYPE_MC88000_ALL     KI32_C(0)
#define CPU_SUBTYPE_MC88100         KI32_C(1)
#define CPU_SUBTYPE_MC88110         KI32_C(2)

/* SPARC */
#define CPU_SUBTYPE_SPARC_ALL       KI32_C(0)

/* I860 */
#define CPU_SUBTYPE_I860_ALL        KI32_C(0)
#define CPU_SUBTYPE_I860_860        KI32_C(1)

/* PowerPC */
#define CPU_SUBTYPE_POWERPC_ALL     KI32_C(0)
#define CPU_SUBTYPE_POWERPC_601     KI32_C(1)
#define CPU_SUBTYPE_POWERPC_602     KI32_C(2)
#define CPU_SUBTYPE_POWERPC_603     KI32_C(3)
#define CPU_SUBTYPE_POWERPC_603e    KI32_C(4)
#define CPU_SUBTYPE_POWERPC_603ev   KI32_C(5)
#define CPU_SUBTYPE_POWERPC_604     KI32_C(6)
#define CPU_SUBTYPE_POWERPC_604e    KI32_C(7)
#define CPU_SUBTYPE_POWERPC_620     KI32_C(8)
#define CPU_SUBTYPE_POWERPC_750     KI32_C(9)
#define CPU_SUBTYPE_POWERPC_7400    KI32_C(10)
#define CPU_SUBTYPE_POWERPC_7450    KI32_C(11)
#define CPU_SUBTYPE_POWERPC_Max     KI32_C(10)
#define CPU_SUBTYPE_POWERPC_SCVger  KI32_C(11)
#define CPU_SUBTYPE_POWERPC_970     KI32_C(100)

/* Subtype capability / feature bits, added in 10.5. X86 only? */
#define CPU_SUBTYPE_MASK            KU32_C(0xff000000)
#define CPU_SUBTYPE_LIB64           KU32_C(0x8000000)

/** @} */



/** @defgroup grp_macho_o_lc        Load Commands
 * @{ */

/**
 * The load command common core structure.
 *
 * After the Mach-O header follows an array of variable sized
 * load command which all has this header in common.
 */
typedef struct load_command
{
    KU32            cmd;            /**< The load command id. */
    KU32            cmdsize;        /**< The size of the command (including this header). */
} load_command_t;

/** @name Load Command IDs (load_command::cmd)
 * @{
 */
/** Flag that when set requires the dynamic linker to fail if it doesn't
 * grok the command. The dynamic linker will otherwise ignore commands it
 * doesn't understand. Introduced with Mac OS X 10.1. */
#define LC_REQ_DYLD         KU32_C(0x80000000)

#define LC_SEGMENT_32       KU32_C(0x01)  /**< Segment to be mapped (32-bit). See segment_command_32. */
#define LC_SYMTAB           KU32_C(0x02)  /**< 'stab' symbol table. See symtab_command. */
#define LC_SYMSEG           KU32_C(0x03)  /**< Obsoleted gdb symbol table. */
#define LC_THREAD           KU32_C(0x04)  /**< Thread. See thread_command. */
#define LC_UNIXTHREAD       KU32_C(0x05)  /**< Unix thread (includes stack and stuff). See thread_command. */
#define LC_LOADFVMLIB       KU32_C(0x06)  /**< Load a specified fixed VM shared library  (obsolete?). See fvmlib_command. */
#define LC_IDFVMLIB         KU32_C(0x07)  /**< Fixed VM shared library id (obsolete?). See fvmlib_command. */
#define LC_IDENT            KU32_C(0x08)  /**< Identification info (obsolete). See ident_command. */
#define LC_FVMFILE          KU32_C(0x09)  /**< Fixed VM file inclusion (internal). See fvmfile_command. */
#define LC_PREPAGE          KU32_C(0x0a)  /**< Prepage command (internal). See ?? */
#define LC_DYSYMTAB         KU32_C(0x0b)  /**< Symbol table for dynamic linking. See dysymtab_command. */
#define LC_LOAD_DYLIB       KU32_C(0x0c)  /**< Load a dynamically linked shared library. See dylib_command. */
#define LC_ID_DYLIB         KU32_C(0x0d)  /**< Dynamically linked share library ident. See dylib_command. */
#define LC_LOAD_DYLINKER    KU32_C(0x0e)  /**< Load a dynamical link editor. See dylinker_command. */
#define LC_ID_DYLINKER      KU32_C(0x0f)  /**< Dynamic link editor ident. See dylinker_command. */
#define LC_PREBOUND_DYLIB   KU32_C(0x10)  /**< Prebound modules for dynamically linking of a shared lib. See prebound_dylib_command. */
#define LC_ROUTINES         KU32_C(0x11)  /**< Image routines. See routines_command_32. */
#define LC_SUB_FRAMEWORK    KU32_C(0x12)  /**< Sub framework. See sub_framework_command. */
#define LC_SUB_UMBRELLA     KU32_C(0x13)  /**< Sub umbrella. See sub_umbrella_command. */
#define LC_SUB_CLIENT       KU32_C(0x14)  /**< Sub client. See sub_client_command. */
#define LC_SUB_LIBRARY      KU32_C(0x15)  /**< Sub library. See sub_library_command. */
#define LC_TWOLEVEL_HINTS   KU32_C(0x16)  /**< Two-level namespace lookup hints. See twolevel_hints_command. */
#define LC_PREBIND_CKSUM    KU32_C(0x17)  /**< Prebind checksum. See prebind_cksum_command. */
#define LC_LOAD_WEAK_DYLIB (KU32_C(0x18) | LC_REQ_DYLD) /**< Dylib that can be missing, all symbols weak. See dylib_command. */
#define LC_SEGMENT_64       KU32_C(0x19)  /**< segment to be mapped (64-bit). See segment_command_32. */
#define LC_ROUTINES_64      KU32_C(0x1a)  /**< Image routines (64-bit). See routines_command_32. */
#define LC_UUID             KU32_C(0x1b)  /**< The UUID of the object module. See uuid_command.  */
#define LC_RPATH           (KU32_C(0x1c) | LC_REQ_DYLD) /**< Runpth additions. See rpath_command. */
#define LC_CODE_SIGNATURE   KU32_C(0x1d)  /**< Code signature location. See linkedit_data_command. */
#define LC_SEGMENT_SPLIT_INFO KU32_C(0x1e)/**< Segment split info location. See linkedit_data_command. */
#define LC_REEXPORT_DYLIB  (KU32_C(0x1f) | LC_REQ_DYLD)/**< Load and re-export the given dylib - DLL forwarding. See dylib_command. */
#define LC_LAZY_LOAD_DYLIB  KU32_C(0x20)  /**< Delays loading of the given dylib until used. See dylib_command? */
#define LC_ENCRYPTION_INFO  KU32_C(0x21)  /**< Segment encryption information. See encryption_info_command. */
#define LC_DYLD_INFO        KU32_C(0x22)  /**< Compressed dylib relocation information, alternative present. See dyld_info_command. */
#define LC_DYLD_INFO_ONLY  (KU32_C(0x22) | LC_REQ_DYLD) /**< Compressed dylib relocation information, no alternative. See dyld_info_command. */
#define LC_LOAD_UPWARD_DYLIB KU32_C(0x23) /**< ???? */
#define LC_VERSION_MIN_MACOSX KU32_C(0x24)   /**< The image requires the given Mac OS X version. See version_min_command. */
#define LC_VERSION_MIN_IPHONEOS KU32_C(0x25) /**< The image requires the given iOS version. See version_min_command. */
#define LC_FUNCTION_STARTS  KU32_C(0x26)  /**< Where to find the compress function start addresses. See linkedit_data_command. */
#define LC_DYLD_ENVIRONMENT KU32_C(0x27)  /**< Environment variable for the dynamic linker. See dylinker_command. */
#define LC_MAIN            (KU32_C(0x28) | LC_REQ_DYLD) /**< Simpler alternative to LC_UNIXTHREAD. */
#define LC_DATA_IN_CODE     KU32_C(0x29)  /**< Table of data in the the text section. */
#define LC_SOURCE_VERSION   KU32_C(0x2a)  /**< Source code revision / version hint. */
#define LC_DYLIB_CODE_SIGN_DRS KU32_C(0x2b) /**< Code signing designated requirements copied from dylibs prequisites. */
/** @} */


/**
 * Load Command String.
 */
typedef struct lc_str
{
    /** Offset of the string relative to the load_command structure.
     * The string is zero-terminated. the size of the load command
     * is zero padded up to a multiple of 4 bytes. */
    KU32            offset;
} lc_str_t;


/**
 * Segment load command (32-bit).
 */
typedef struct segment_command_32
{
    KU32            cmd;            /**< LC_SEGMENT */
    KU32            cmdsize;        /**< sizeof(self) + sections. */
    char            segname[16];    /**< The segment name. */
    KU32            vmaddr;         /**< Memory address of this segment. */
    KU32            vmsize;         /**< Size of this segment. */
    KU32            fileoff;        /**< The file location of the segment. */
    KU32            filesize;       /**< The file size of the segment. */
    KU32            maxprot;        /**< Maximum VM protection. */
    KU32            initprot;       /**< Initial VM protection. */
    KU32            nsects;         /**< Number of section desciptors following this structure. */
    KU32            flags;          /**< Flags (SG_*). */
} segment_command_32_t;


/**
 * Segment load command (64-bit).
 * Same as segment_command_32 except 4 members has been blown up to 64-bit.
 */
typedef struct segment_command_64
{
    KU32            cmd;            /**< LC_SEGMENT */
    KU32            cmdsize;        /**< sizeof(self) + sections. */
    char            segname[16];    /**< The segment name. */
    KU64            vmaddr;         /**< Memory address of this segment. */
    KU64            vmsize;         /**< Size of this segment. */
    KU64            fileoff;        /**< The file location of the segment. */
    KU64            filesize;       /**< The file size of the segment. */
    KU32            maxprot;        /**< Maximum VM protection. */
    KU32            initprot;       /**< Initial VM protection. */
    KU32            nsects;         /**< Number of section desciptors following this structure. */
    KU32            flags;          /**< Flags (SG_*). */
} segment_command_64_t;

/** @name Segment flags (segment_command_64::flags, segment_command_32::flags)
 * @{ */
/** Map the file bits in the top end of the memory area for the segment
 * instead of the low end. Intended for stacks in core dumps.
 * The part of the segment memory not covered by file bits will be zeroed. */
#define SG_HIGHVM           KU32_C(0x00000001)
/** This segment is the virtual memory allocated by a fixed VM library.
 * (Used for overlap checking in the linker.) */
#define SG_FVMLIB           KU32_C(0x00000002)
/** No relocations for or symbols that's relocated to in this segment.
 * The segment can therefore safely be replaced. */
#define SG_NORELOC          KU32_C(0x00000004)
/** The segment is protected.
 * The first page isn't protected if it starts at file offset 0
 * (so that the mach header and this load command can be easily mapped). */
#define SG_PROTECTED_VERSION_1 KU32_C(0x00000008)
/** @} */


/**
 * 32-bit section (part of a segment load command).
 */
typedef struct section_32
{
    char            sectname[16];   /**< The section name. */
    char            segname[16];    /**< The name of the segment this section goes into. */
    KU32            addr;           /**< The memory address of this section. */
    KU32            size;           /**< The size of this section. */
    KU32            offset;         /**< The file offset of this section. */
    KU32            align;          /**< The section alignment (**2). */
    KU32            reloff;         /**< The file offset of the relocations. */
    KU32            nreloc;         /**< The number of relocations. */
    KU32            flags;          /**< The section flags; section type and attribs */
    KU32            reserved1;      /**< Reserved / offset / index. */
    KU32            reserved2;      /**< Reserved / count / sizeof. */
} section_32_t;

/**
 * 64-bit section (part of a segment load command).
 */
typedef struct section_64
{
    char            sectname[16];   /**< The section name. */
    char            segname[16];    /**< The name of the segment this section goes into. */
    KU64            addr;           /**< The memory address of this section. */
    KU64            size;           /**< The size of this section. */
    KU32            offset;         /**< The file offset of this section. */
    KU32            align;          /**< The section alignment (**2). */
    KU32            reloff;         /**< The file offset of the relocations. */
    KU32            nreloc;         /**< The number of relocations. */
    KU32            flags;          /**< The section flags; section type and attribs */
    KU32            reserved1;      /**< Reserved / offset / index. */
    KU32            reserved2;      /**< Reserved / count / sizeof. */
    KU32            reserved3;      /**< (Just) Reserved. */
} section_64_t;

/** @name Section flags (section_64::flags, section_32::flags)
 * @{
 */
/** Section type mask. */
#define SECTION_TYPE                    KU32_C(0x000000ff)
/** Regular section. */
#define S_REGULAR                       0x00
/** Zero filled section. */
#define S_ZEROFILL                      0x01
/** C literals. */
#define S_CSTRING_LITERALS              0x02
/** 4 byte literals. */
#define S_4BYTE_LITERALS                0x03
/** 8 byte literals. */
#define S_8BYTE_LITERALS                0x04
/** Pointer to literals. */
#define S_LITERAL_POINTERS              0x05
/** Section containing non-lazy symbol pointers.
 * Reserved1 == start index in the indirect symbol table. */
#define S_NON_LAZY_SYMBOL_POINTERS      0x06
/** Section containing lazy symbol pointers.
 * Reserved1 == start index in the indirect symbol table. */
#define S_LAZY_SYMBOL_POINTERS          0x07
/** Section containing symbol stubs.
 * Reserved2 == stub size. */
#define S_SYMBOL_STUBS                  0x08
/** Section containing function pointers for module initialization. . */
#define S_MOD_INIT_FUNC_POINTERS        0x09
/** Section containing function pointers for module termination. . */
#define S_MOD_TERM_FUNC_POINTERS        0x0a
/** Section containing symbols that are to be coalesced. */
#define S_COALESCED                     0x0b
/** Zero filled section that be larger than 4GB. */
#define S_GB_ZEROFILL                   0x0c
/** Section containing pairs of function pointers for interposing. */
#define S_INTERPOSING                   0x0d
/** 16 byte literals. */
#define S_16BYTE_LITERALS               0x0e
/** DTrace byte code / definitions (DOF = DTrace object format). */
#define S_DTRACE_DOF                    0x0f
/** Section containing pointers to symbols in lazily loaded dylibs. */
#define S_LAZY_DYLIB_SYMBOL_POINTERS    0x10

/** Section attribute mask. */
#define SECTION_ATTRIBUTES          KU32_C(0xffffff00)

/** User settable attribute mask. */
#define SECTION_ATTRIBUTES_USR      KU32_C(0xff000000)
/** Pure instruction (code). */
#define S_ATTR_PURE_INSTRUCTIONS    KU32_C(0x80000000)
/** ranlib, ignore my symbols... */
#define S_ATTR_NO_TOC               KU32_C(0x40000000)
/** May strip static symbols when linking int a MH_DYLDLINK file. */
#define S_ATTR_STRIP_STATIC_SYMS    KU32_C(0x20000000)
/** No dead stripping. */
#define S_ATTR_NO_DEAD_STRIP        KU32_C(0x10000000)
/** Live support. */
#define S_ATTR_LIVE_SUPPORT         KU32_C(0x08000000)
/** Contains self modifying code (generally i386 code stub for dyld). */
#define S_ATTR_SELF_MODIFYING_CODE  KU32_C(0x04000000)
/** Debug info (DWARF usually). */
#define S_ATTR_DEBUG                KU32_C(0x02000000)

/** System settable attribute mask. */
#define SECTION_ATTRIBUTES_SYS      KU32_C(0x00ffff00)
/** Contains some instructions (code). */
#define S_ATTR_SOME_INSTRUCTIONS    KU32_C(0x00000400)
/** Has external relocations. */
#define S_ATTR_EXT_RELOC            KU32_C(0x00000200)
/** Has internal (local) relocations. */
#define S_ATTR_LOC_RELOC            KU32_C(0x00000100)
/** @} */

/** @name Known Segment and Section Names.
 * Some of these implies special linker behaviour.
 * @{
 */
/** Page zero - not-present page for catching invalid access. (MH_EXECUTE typically) */
#define SEG_PAGEZERO        "__PAGEZERO"
/** Traditional UNIX text segment.
 * Defaults to R-X. */
#define SEG_TEXT            "__TEXT"
/** The text part of SEG_TEXT. */
#define SECT_TEXT               "__text"
/** The fvmlib initialization. */
#define SECT_FVMLIB_INIT0       "__fvmlib_init0"
/** The section following the fvmlib initialization. */
#define SECT_FVMLIB_INIT1       "__fvmlib_init1"
/** The traditional UNIX data segment. (DGROUP to DOS and OS/2 people.) */
#define SEG_DATA            "__DATA"
/** The initialized data section. */
#define SECT_DATA               "__data"
/** The uninitialized data section. */
#define SECT_BSS                "__bss"
/** The common symbol section. */
#define SECT_COMMON             "__common"
/** Objective-C runtime segment. */
#define SEG_OBJC            "__OBJC"
/** Objective-C symbol table section. */
#define SECT_OBJC_SYMBOLS       "__symbol_table"
/** Objective-C module information section. */
#define SECT_OBJC_MODULES       "__module_info"
/** Objective-C string table section. */
#define SECT_OBJC_STRINGS       "__selector_strs"
/** Objective-C string table section. */
#define SECT_OBJC_REFS          "__selector_refs"
/** Icon segment. */
#define SEG_ICON            "__ICON"
/** The icon headers. */
#define SECT_ICON_HEADER        "__header"
/** The icons in the TIFF format. */
#define SECT_ICON_TIFF          "__tiff"
/** ld -seglinkedit segment containing all the structs create and maintained
 * by the linker. MH_EXECUTE and MH_FVMLIB only. */
#define SEG_LINKEDIT        "__LINKEDIT"
/** The unix stack segment. */
#define SEG_UNIXSTACK       "__UNIXSTACK"
/** The segment for the self modifying code for dynamic linking.
 * Implies RWX permissions. */
#define SEG_IMPORT          "__IMPORT"
/** @} */


/** @todo fvmlib */
/** @todo fvmlib_command (LC_IDFVMLIB or LC_LOADFVMLIB) */
/** @todo dylib */
/** @todo dylib_command (LC_ID_DYLIB, LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB,
 *        LC_REEXPORT_DYLIB, LC_LAZY_LOAD_DYLIB) */
/** @todo sub_framework_command (LC_SUB_FRAMEWORK) */
/** @todo sub_client_command (LC_SUB_CLIENT) */
/** @todo sub_umbrella_command (LC_SUB_UMBRELLA) */
/** @todo sub_library_command (LC_SUB_LIBRARY) */
/** @todo prebound_dylib_command (LC_PREBOUND_DYLIB) */
/** @todo dylinker_command (LC_ID_DYLINKER or LC_LOAD_DYLINKER,
 *        LC_DYLD_ENVIRONMENT) */

/**
 * Thread command.
 *
 * State description of a thread that is to be created. The description
 * is made up of a number of state structures preceded by a 32-bit flavor
 * and 32-bit count field stating the kind of stat structure and it's size
 * in KU32 items respecitvly.
 *
 * LC_UNIXTHREAD differs from LC_THREAD in that it implies stack creation
 * and that it's started with the typical main(int, char **, char **) frame
 * on the stack.
 */
typedef struct thread_command
{
    KU32            cmd;        /**< LC_UNIXTHREAD or LC_THREAD. */
    KU32            cmdsize;    /**< The size of the command (including this header). */
} thread_command_t;


/** @todo routines_command (LC_ROUTINES) */
/** @todo routines_command_64 (LC_ROUTINES_64) */


/**
 * Symbol table command.
 * Contains a.out style symbol table with some tricks.
 */
typedef struct symtab_command
{
    KU32            cmd;        /**< LC_SYMTAB */
    KU32            cmdsize;    /** sizeof(symtab_command_t) */
    KU32            symoff;     /** The file offset of the symbol table. */
    KU32            nsyms;      /** The number of symbols in the symbol table. */
    KU32            stroff;     /** The file offset of the string table. */
    KU32            strsize;    /** The size of the string table. */
} symtab_command_t;


/** @todo dysymtab_command (LC_DYSYMTAB)  */
/** @todo dylib_table_of_contents */
/** @todo dylib_module_32 */
/** @todo dylib_module_64 */
/** @todo dylib_reference */
/** @todo twolevel_hints_command (LC_TWOLEVEL_HINTS) */
/** @todo twolevel_hint */
/** @todo prebind_cksum_command (LC_PREBIND_CKSUM) */


/**
 * UUID generated by ld.
 */
typedef struct uuid_command
{
    KU32            cmd;        /**< LC_UUID */
    KU32            cmdsize;    /**< sizeof(uuid_command_t) */
    KU8             uuid[16];   /** The UUID bytes. */
} uuid_command_t;


/** @todo symseg_command (LC_SYMSEG) */
/** @todo ident_command (LC_IDENT) */
/** @todo fvmfile_command (LC_FVMFILE) */
/** @todo rpath_command (LC_RPATH) */

typedef struct linkedit_data_command 
{
    KU32            cmd;        /**< LC_CODE_SIGNATURE, LC_SEGMENT_SPLIT_INFO, LC_FUNCTION_STARTS */
    KU32            cmdsize;    /**< size of this structure. */
    KU32            dataoff;    /**< Offset into the file of the data. */
    KU32            datasize;   /**< The size of the data. */
} linkedit_data_command_t;

/** @todo encryption_info_command (LC_ENCRYPTION_INFO) */
/** @todo dyld_info_command (LC_DYLD_INFO, LC_DYLD_INFO_ONLY) */

typedef struct version_min_command
{
    KU32            cmd;        /**< LC_VERSION_MIN_MACOSX, LC_VERSION_MIN_IPHONEOS */
    KU32            cmdsize;    /**< size of this structure. */
    KU32            version;    /**< 31..16=major, 15..8=minor, 7..0=patch. */
    KU32            reserved;   /**< MBZ. */
} version_min_command_t;

/** @} */



/** @defgroup grp_macho_o_syms  Symbol Table
 * @{ */

/**
 * The 32-bit Mach-O version of the nlist structure.
 *
 * This differs from the a.out nlist struct in that the unused n_other field
 * was renamed to n_sect and used for keeping the relevant section number.
 * @remark  This structure is not name mach_nlist_32 in the Apple headers, but nlist.
 */
typedef struct macho_nlist_32
{
    union
    {
        KI32    n_strx;         /**< Offset (index) into the string table. 0 means "". */
    }           n_un;
    KU8         n_type;         /**< Symbol type. */
    KU8         n_sect;         /**< Section number of NO_SECT. */
    KI16        n_desc;         /**< Type specific, debug info details mostly.*/
    KU32        n_value;        /**< The symbol value or stab offset. */
} macho_nlist_32_t;


/**
 * The 64-bit Mach-O version of the nlist structure.
 * @see macho_nlist_32
 */
typedef struct macho_nlist_64
{
    union
    {
        KU32    n_strx;         /**< Offset (index) into the string table. 0 means "". */
    }           n_un;
    KU8         n_type;         /**< Symbol type. */
    KU8         n_sect;         /**< Section number of NO_SECT. */
    KI16        n_desc;         /**< Type specific, debug info details mostly.*/
    KU64        n_value;        /**< The symbol value or stab offset. */
} macho_nlist_64_t;


/** @name Symbol Type Constants (macho_nlist_32_t::n_type, macho_nlist_64_t::n_type)
 *
 * In the Mach-O world n_type is somewhat similar to a.out, meaning N_EXT, N_UNDF, N_ABS
 * and the debug symbols are essentially the same, but the remaining stuff is different.
 * The main reason for this is that the encoding of section has been moved to n_sect
 * to permit up to 255 sections instead of the fixed 3 a.out sections (not counting
 * the abs symbols and set vectors).
 *
 * To avoid confusion with a.out the Mach-O constants has been fitted with a MACHO_
 * prefix here.
 *
 * Common symbols (aka communal symbols and comdefs) are represented by
 * n_type = MACHO_N_EXT | MACHO_N_UNDF, n_sect = NO_SECT and n_value giving
 * the size.
 *
 *
 * Symbol table entries can be inserted directly in the assembly code using
 * this notation:
 * @code
 *      .stabs "n_name", n_type, n_sect, n_desc, n_value
 * @endcode
 *
 * (1) The line number is optional, GCC doesn't set it.
 * (2) The type is optional, GCC doesn't set it.
 * (3) The binutil header is "skeptical" about the line. I'm skeptical about the whole thing... :-)
 * (M) Mach-O specific?
 * (S) Sun specific?
 * @{
 */

/* Base masks. */
#define MACHO_N_EXT     KU8_C(0x01)   /**< External symbol (when set) (N_EXT). */
#define MACHO_N_TYPE    KU8_C(0x0e)   /**< Symbol type (N_TYPE without the 8th bit). */
#define MACHO_N_PEXT    KU8_C(0x10)   /**< Private extern symbol (when set). (M) */
#define MACHO_N_STAB    KU8_C(0xe0)   /**< Debug symbol mask (N_STAB). */

/* MACHO_N_TYPE values. */
#define MACHO_N_UNDF    KU8_C(0x00)   /**< MACHO_N_TYPE: Undefined symbol (N_UNDF). n_sect = NO_SECT. */
#define MACHO_N_ABS     KU8_C(0x02)   /**< MACHO_N_TYPE: Absolute symbol (N_UNDF). n_sect = NO_SECT. */
#define MACHO_N_INDR    KU8_C(0x0a)   /**< MACHO_N_TYPE: Indirect symbol, n_value is the index of the symbol. (M) */
#define MACHO_N_PBUD    KU8_C(0x0c)   /**< MACHO_N_TYPE: Prebound undefined symbo (defined in a dylib). (M) */
#define MACHO_N_SECT    KU8_C(0x0e)   /**< MACHO_N_TYPE: Defined in the section given by n_sects. (M) */

/* Debug symbols. */
#define MACHO_N_GSYM    KU8_C(0x20)   /**< Global variable.       "name",, NO_SECT, type, 0       (2) */
#define MACHO_N_FNAME   KU8_C(0x22)   /**< Function name (F77).   "name",, NO_SECT, 0, 0 */
#define MACHO_N_FUN     KU8_C(0x24)   /**< Function / text var.   "name",, section, line, address (1) */
#define MACHO_N_STSYM   KU8_C(0x26)   /**< Static data symbol.    "name",, section, type, address (2) */
#define MACHO_N_LCSYM   KU8_C(0x28)   /**< static bss symbol.     "name",, section, type, address (2) */
    /* omits N_MAIN and N_ROSYM. */
#define MACHO_N_BNSYM   KU8_C(0x2e)   /**< Begin nsect symbol.         0,, section, 0, address (M) */
#define MACHO_N_PC      KU8_C(0x30)   /**< Global pascal symbol.  "name",, NO_SECT, subtype?, line (3) */
    /* omits N_NSYMS, N_NOMAP and N_OBJ. */
#define MACHO_N_OPT     KU8_C(0x3c)   /**< Options for the debugger related to the language of the
                                             source file.           "options?",,,, */
#define MACHO_N_RSYM    KU8_C(0x40)   /**< Register variable.     "name",, NO_SECT, type, register */
    /* omits N_M2C */
#define MACHO_N_SLINE   KU8_C(0x44)   /**< Source line.                0,, section, line, address */
    /* omits N_DSLINE, N_BSLINE / N_BROWS, N_DEFD and N_FLINE. */
#define MACHO_N_ENSYM   KU8_C(0x4e)   /**< End nsect symbol.           0,, section, 0, address (M) */
    /* omits N_EHDECL / N_MOD2 and N_CATCH. */
#define MACHO_N_SSYM    KU8_C(0x60)   /**< Struct/union element.  "name",, NO_SECT, type, offset */
    /* omits N_ENDM */
#define MACHO_N_SO      KU8_C(0x64)   /**< Source file name.      "fname",, section, 0, address */
#define MACHO_N_OSO     KU8_C(0x66)   /**< Object file name.      "fname",, 0, 0, st_mtime (M?) */
    /* omits N_ALIAS */
#define MACHO_N_LSYM    KU8_C(0x80)   /**< Stack variable.        "name",, NO_SECT, type, frame_offset */
#define MACHO_N_BINCL   KU8_C(0x82)   /**< Begin #include.        "fname",, NO_SECT, 0, sum? */
#define MACHO_N_SOL     KU8_C(0x84)   /**< #included file.        "fname",, section, 0, start_address (S) */
#define MACHO_N_PARAMS  KU8_C(0x86)   /**< Compiler params.       "params",, NO_SECT, 0, 0 */
#define MACHO_N_VERSION KU8_C(0x88)   /**< Compiler version.      "version",, NO_SECT, 0, 0 */
#define MACHO_N_OLEVEL  KU8_C(0x8A)   /**< Compiler -O level.     "level",, NO_SECT, 0, 0 */
#define MACHO_N_PSYM    KU8_C(0xa0)   /**< Parameter variable.    "name",, NO_SECT, type, frame_offset */
#define MACHO_N_EINCL   KU8_C(0xa2)   /**< End #include.          "fname",, NO_SECT, 0, 0 (S) */
#define MACHO_N_ENTRY   KU8_C(0xa4)   /**< Alternate entry point. "name",, section, line, address */
#define MACHO_N_LBRAC   KU8_C(0xc0)   /**< Left bracket.               0,, NO_SECT, nesting_level, address */
#define MACHO_N_EXCL    KU8_C(0xc2)   /**< Deleted include file.  "fname",, NO_SECT, 0, sum?  (S) */
    /* omits N_SCOPE */
#define MACHO_N_RBRAC   KU8_C(0xe0)   /**< Right bracket.              0,, NO_SECT, nesting_level, address */
#define MACHO_N_BCOMM   KU8_C(0xe2)   /**< Begin common.          "name",, NO_SECT?, 0, 0 */
#define MACHO_N_ECOMM   KU8_C(0xe4)   /**< End common.            "name",, section, 0, 0 */
#define MACHO_N_ECOML   KU8_C(0xe8)   /**< End local common.           0,, section, 0, address */
#define MACHO_N_LENG    KU8_C(0xfe)   /**< Length-value of the preceding entry.
                                                                    "name",, NO_SECT, 0, length */

/** @} */

/** @name Symbol Description Bits (macho_nlist_32_t::n_desc, macho_nlist_64_t::n_desc)
 *
 * Mach-O puts the n_desc field to a number of uses, like lazy binding , library
 * ordinal numbers for -twolevel_namespace, stripping and weak symbol handling.
 *
 * @remark The REFERENCE_FLAGS_* are really not flags in the normal sense (bit),
 *         they are more like enum values.
 * @{
 */

#define REFERENCE_TYPE                  KU16_C(0x000f)    /**< The reference type mask. */
#define REFERENCE_FLAG_UNDEFINED_NON_LAZY             0     /**< Normal undefined symbol. */
#define REFERENCE_FLAG_UNDEFINED_LAZY                 1     /**< Lazy undefined symbol. */
#define REFERENCE_FLAG_DEFINED                        2     /**< Defined symbol (dynamic linking). */
#define REFERENCE_FLAG_PRIVATE_DEFINED                3     /**< Defined private symbol (dynamic linking). */
#define REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY     4     /**< Normal undefined private symbol. */
#define REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY         5     /**< Lazy undefined private symbol. */

#define REFERENCED_DYNAMICALLY          KU16_C(0x0010)    /**< Don't strip. */


/** Get the dynamic library ordinal. */
#define GET_LIBRARY_ORDINAL(n_desc)     \
    (((n_desc) >> 8) & 0xff)
/** Set the dynamic library ordinal. */
#define SET_LIBRARY_ORDINAL(n_desc, ordinal) \
    (n_desc) = (((n_desc) & 0xff) | (((ordinal) & 0xff) << 8))
#define SELF_LIBRARY_ORDINAL            0x00                /**< Special ordinal for refering to onself. */
#define MAX_LIBRARY_ORDINAL             0xfd                /**< Maximum ordinal number. */
#define DYNAMIC_LOOKUP_ORDINAL          0xfe                /**< Special ordinal number for dynamic lookup. (Mac OS X 10.3 and later) */
#define EXECUTABLE_ORDINAL              0xff                /**< Special ordinal number for the executable.  */


/** Only MH_OBJECT: Never dead strip me! */
#define N_NO_DEAD_STRIP                 KU16_C(0x0020)
/** Not MH_OBJECT: Discarded symbol. */
#define N_DESC_DISCARDED                KU16_C(0x0020)
/** Weak external symbol. Symbol can be missing, in which case it's will have the value 0. */
#define N_WEAK_REF                      KU16_C(0x0040)
/** Weak symbol definition. The symbol can be overridden by another weak
 * symbol already present or by a non-weak (strong) symbol definition.
 * Currently only supported for coalesed symbols.
 * @remark This bit means something differently for undefined symbols, see N_REF_TO_WEAK.
 */
#define N_WEAK_DEF                      KU16_C(0x0080)
/** Reference to a weak symbol, resolve using flat namespace searching.
 * @remark This bit means something differently for defined symbols, see N_WEAK_DEF. */
#define N_REF_TO_WEAK                   KU16_C(0x0080)

/** @} */

/** @} */


/** @defgroup grp_macho_o_relocs    Relocations
 * @{ */

/**
 * Relocation entry.
 *
 * Differs from a.out in the meaning of r_symbolnum when r_extern=0 and
 * that r_pad is made into r_type.
 *
 * @remark  This structure and type has been prefixed with macho_ to avoid
 *          confusion with the original a.out type.
 */
typedef struct macho_relocation_info
{
    KI32        r_address;          /**< Section relative address of the fixup.
                                         The top bit (signed) indicates that this is a scattered
                                         relocation if set, see scattered_relocation_info_t. */
    KU32        r_symbolnum : 24,   /**< r_extern=1: Symbol table index, relocate with the address of this symbol.
                                         r_extern=0: Section ordinal, relocate with the address of this section. */
                r_pcrel : 1,        /**< PC (program counter) relative fixup; subtract the fixup address. */
                r_length : 2,       /**< Fixup length: 0=KU8, 1=KU16, 2=KU32, 3=KU64. */
                r_extern : 1,       /**< External or internal fixup, decides the r_symbolnum interpretation.. */
                r_type : 4;         /**< Relocation type; 0 is standard, non-zero are machine specific. */
} macho_relocation_info_t;

/** Special section ordinal value for absolute relocations. */
#define R_ABS           0

/** Flag in r_address indicating that the relocation is of the
 * scattered_relocation_info_t kind and not macho_relocation_info_t. */
#define R_SCATTERED     KU32_C(0x80000000)

/**
 * Scattered relocation.
 *
 * This is a hack mainly for RISC machines which restricts section size
 * to 16MB among other things.
 *
 * The reason for the big/little endian differences here is of course because
 * of the R_SCATTERED mask and the way bitfields are implemented by the
 * C/C++ compilers.
 */
typedef struct scattered_relocation_info
{
#if K_ENDIAN == K_ENDIAN_LITTLE
    KU32        r_address : 24,     /**< Section relative address of the fixup. (macho_relocation_info_t::r_address) */
                r_type : 4,         /**< Relocation type; 0 is standard, non-zero are machine specific. (macho_relocation_info_t::r_type) */
                r_length : 2,       /**< Fixup length: 0=KU8, 1=KU16, 2=KU32, 3=KU64. (macho_relocation_info_t::r_length) */
                r_pcrel : 1,        /**< PC (program counter) relative fixup; subtract the fixup address. (macho_relocation_info_t::r_pcrel) */
                r_scattered : 1;    /**< Set if scattered relocation, clear if normal relocation. */
#elif K_ENDIAN == K_ENDIAN_BIG
    KU32        r_scattered : 1,    /**< Set if scattered relocation, clear if normal relocation. */
                r_pcrel : 1,        /**< PC (program counter) relative fixup; subtract the fixup address. (macho_relocation_info_t::r_pcrel) */
                r_length : 2,       /**< Fixup length: 0=KU8, 1=KU16, 2=KU32, 3=KU64. (macho_relocation_info_t::r_length) */
                r_type : 4,         /**< Relocation type; 0 is standard, non-zero are machine specific. (macho_relocation_info_t::r_type) */
                r_address : 24;     /**< Section relative address of the fixup. (macho_relocation_info_t::r_address) */
#else
# error "Neither K_ENDIAN isn't LITTLE or BIG!"
#endif
    KI32        r_value;            /**< The value the fixup is refering to (without offset added). */
} scattered_relocation_info_t;

/**
 * Relocation type values for a generic implementation (for r_type).
 */
typedef enum reloc_type_generic
{
    GENERIC_RELOC_VANILLA = 0,      /**< Standard relocation. */
    GENERIC_RELOC_PAIR,             /**< Follows GENERIC_RELOC_SECTDIFF. */
    GENERIC_RELOC_SECTDIFF,         /**< ??? */
    GENERIC_RELOC_PB_LA_PTR,        /**< Prebound lazy pointer whatever that. */
    GENERIC_RELOC_LOCAL_SECTDIFF    /**< ??? */
} reloc_type_generic_t;

/**
 * Relocation type values for AMD64 (for r_type).
 */
typedef enum reloc_type_x86_64
{
    X86_64_RELOC_UNSIGNED = 0,      /**< Absolute address. */
    X86_64_RELOC_SIGNED,            /**< Signed displacement. */
    X86_64_RELOC_BRANCH,            /**< Branch displacement (jmp/call, adj by size). */
    X86_64_RELOC_GOT_LOAD,          /**< GOT entry load. */
    X86_64_RELOC_GOT,               /**< GOT reference. */
    X86_64_RELOC_SUBTRACTOR,        /**< ??. */
    X86_64_RELOC_SIGNED_1,          /**< Signed displacement with a -1 added. */
    X86_64_RELOC_SIGNED_2,          /**< Signed displacement with a -2 added. */
    X86_64_RELOC_SIGNED_4           /**< Signed displacement with a -4 added. */
} reloc_type_x86_64_t;

/** @} */


/** @} */
#endif


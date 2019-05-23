/* $Id $ */
/** @file
 * LX structures, types and defines.
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

#ifndef ___k_kLdrFmts_lx_h___
#define ___k_kLdrFmts_lx_h___

#include <k/kDefs.h>
#include <k/kTypes.h>


#ifndef IMAGE_OS2_SIGNATURE_LX
/** LX signature ("LX") */
# define IMAGE_LX_SIGNATURE  K_LE2H_U16('L' | ('X' << 8))
#endif

#pragma pack(1)

/**
 * Linear eXecutable header.
 * This structure is exactly 196 bytes long.
 */
struct e32_exe
{
    KU8                 e32_magic[2];
    KU8                 e32_border;
    KU8                 e32_worder;
    KU32                e32_level;
    KU16                e32_cpu;
    KU16                e32_os;
    KU32                e32_ver;
    KU32                e32_mflags;
    KU32                e32_mpages;
    KU32                e32_startobj;
    KU32                e32_eip;
    KU32                e32_stackobj;
    KU32                e32_esp;
    KU32                e32_pagesize;
    KU32                e32_pageshift;
    /** The size of the fixup section.
     * The fixup section consists of the fixup page table, the fixup record table,
     * the import module table, and the import procedure name table.
     */
    KU32                e32_fixupsize;
    KU32                e32_fixupsum;
    /** The size of the resident loader section.
     * This includes the object table, the object page map table, the resource table, the resident name table,
     * the entry table, the module format directives table, and the page checksum table (?). */
    KU32                e32_ldrsize;
    /** The checksum of the loader section. 0 if not calculated. */
    KU32                e32_ldrsum;
    /** The offset of the object table relative to this structure. */
    KU32                e32_objtab;
    /** Count of objects. */
    KU32                e32_objcnt;
    /** The offset of the object page map table relative to this structure. */
    KU32                e32_objmap;
    /** The offset of the object iterated pages (whatever this is used for) relative to the start of the file. */
    KU32                e32_itermap;
    /** The offset of the resource table relative to this structure. */
    KU32                e32_rsrctab;
    /** The number of entries in the resource table. */
    KU32                e32_rsrccnt;
    /** The offset of the resident name table relative to this structure. */
    KU32                e32_restab;
    /** The offset of the entry (export) table relative to this structure. */
    KU32                e32_enttab;
    /** The offset of the module format directives table relative to this structure. */
    KU32                e32_dirtab;
    /** The number of entries in the module format directives table. */
    KU32                e32_dircnt;
    /** The offset of the fixup page table relative to this structure. */
    KU32                e32_fpagetab;
    /** The offset of the fixup record table relative to this structure. */
    KU32                e32_frectab;
    /** The offset of the import module name table relative to this structure. */
    KU32                e32_impmod;
    /** The number of entries in the import module name table. */
    KU32                e32_impmodcnt;
    /** The offset of the import procedure name table relative to this structure. */
    KU32                e32_impproc;
    /** The offset of the page checksum table relative to this structure. */
    KU32                e32_pagesum;
    /** The offset of the data pages relative to the start of the file. */
    KU32                e32_datapage;
    /** The number of preload pages (ignored). */
    KU32                e32_preload;
    /** The offset of the non-resident name table relative to the start of the file. */
    KU32                e32_nrestab;
    /** The size of the non-resident name table. */
    KU32                e32_cbnrestab;
    KU32                e32_nressum;
    KU32                e32_autodata;
    KU32                e32_debuginfo;
    KU32                e32_debuglen;
    KU32                e32_instpreload;
    KU32                e32_instdemand;
    KU32                e32_heapsize;
    KU32                e32_stacksize;
    KU8                 e32_res3[20];
};

/** e32_magic[0] */
#define E32MAGIC1       'L'
/** e32_magic[1] */
#define E32MAGIC2       'X'
/** MAKEWORD(e32_magic[0], e32_magic[1]) */
#define E32MAGIC        0x584c
/** e32_border - little endian */
#define E32LEBO         0
/** e32_border - big endian */
#define E32BEBO         1
/** e32_worder - little endian */
#define E32LEWO         0
/** e32_worder - big endian */
#define E32BEWO         1
/** e32_level */
#define E32LEVEL        KU32_C(0)
/** e32_cpu - 80286 */
#define E32CPU286       1
/** e32_cpu - 80386 */
#define E32CPU386       2
/** e32_cpu - 80486 */
#define E32CPU486       3
/** e32_pagesize */
#define OBJPAGELEN      KU32_C(0x1000)


/** @name e32_mflags
 * @{ */
/** App Type: Fullscreen only. */
#define E32NOPMW         KU32_C(0x00000100)
/** App Type: PM API. */
#define E32PMAPI         KU32_C(0x00000300)
/** App Type: PM VIO compatible. */
#define E32PMW           KU32_C(0x00000200)
/** Application type mask. */
#define E32APPMASK       KU32_C(0x00000300)
/** Executable module. */
#define E32MODEXE        KU32_C(0x00000000)
/** Dynamic link library (DLL / library) module. */
#define E32MODDLL        KU32_C(0x00008000)
/** Protected memory DLL. */
#define E32PROTDLL       KU32_C(0x00010000)
/** Physical Device Driver. */
#define E32MODPDEV       KU32_C(0x00020000)
/** Virtual Device Driver. */
#define E32MODVDEV       KU32_C(0x00028000)
/** Device driver */
#define E32DEVICE        E32MODPDEV
/** Dynamic link library (DLL / library) module. */
#define E32NOTP          E32MODDLL
/** Protected memory DLL. */
#define E32MODPROTDLL    (E32MODDLL | E32PROTDLL)
/** Module Type mask. */
#define E32MODMASK       KU32_C(0x00038000)
/** Not loadable (linker error). */
#define E32NOLOAD        KU32_C(0x00002000)
/** No internal fixups. */
#define E32NOINTFIX      KU32_C(0x00000010)
/** No external fixups (i.e. imports). */
#define E32NOEXTFIX      KU32_C(0x00000020)
/** System DLL, no internal fixups. */
#define E32SYSDLL        KU32_C(0x00000008)
/** Global (set) or per instance (cleared) library initialization. */
#define E32LIBINIT       KU32_C(0x00000004)
/** Global (set) or per instance (cleared) library termination. */
#define E32LIBTERM       KU32_C(0x40000000)
/** Indicates when set in an executable that the process isn't SMP safe. */
#define E32NOTMPSAFE     KU32_C(0x00080000)
/** @} */

/** @name Relocations (aka Fixups).
 * @{ */
typedef union _offset
{
    KU16                offset16;
    KU32                offset32;
} offset;

/** A relocation.
 * @remark this structure isn't very usable since LX relocations comes in too many size variations.
 */
struct r32_rlc
{
    KU8                 nr_stype;
    KU8                 nr_flags;
    KI16                r32_soff;
    KU16                r32_objmod;

    union targetid
    {
        offset          intref;
        union extfixup
        {
            offset      proc;
            KU32        ord;
        } extref;
        struct addfixup
        {
            KU16        entry;
            offset      addval;
        } addfix;
    } r32_target;
    KU16                r32_srccount;
    KU16                r32_chain;
};

/** @name Some attempt at size constanstants.
 * @{
 */
#define RINTSIZE16      8
#define RINTSIZE32      10
#define RORDSIZE        8
#define RNAMSIZE16      8
#define RNAMSIZE32      10
#define RADDSIZE16      10
#define RADDSIZE32      12
/** @} */

/** @name nr_stype (source flags)
 * @{ */
#define NRSBYT          0x00
#define NRSSEG          0x02
#define NRSPTR          0x03
#define NRSOFF          0x05
#define NRPTR48         0x06
#define NROFF32         0x07
#define NRSOFF32        0x08
#define NRSTYP          0x0f
#define NRSRCMASK       0x0f
#define NRALIAS         0x10
#define NRCHAIN         0x20
/** @} */

/** @name nr_flags (target flags)
 * @{ */
#define NRRINT          0x00
#define NRRORD          0x01
#define NRRNAM          0x02
#define NRRENT          0x03
#define NRRTYP          0x03
#define NRADD           0x04
#define NRICHAIN        0x08
#define NR32BITOFF      0x10
#define NR32BITADD      0x20
#define NR16OBJMOD      0x40
#define NR8BITORD       0x80
/** @} */

/** @} */


/** @name The Object Table (aka segment table)
 * @{ */

/** The Object Table Entry. */
struct o32_obj
{
    /** The size of the object. */
    KU32                o32_size;
    /** The base address of the object. */
    KU32                o32_base;
    /** Object flags. */
    KU32                o32_flags;
    /** Page map index. */
    KU32                o32_pagemap;
    /** Page map size. (doesn't need to be o32_size >> page shift). */
    KU32                o32_mapsize;
    /** Reserved */
    KU32                o32_reserved;
};

/** @name o32_flags
 * @{ */
/** Read access. */
#define OBJREAD         KU32_C(0x00000001)
/** Write access. */
#define OBJWRITE        KU32_C(0x00000002)
/** Execute access. */
#define OBJEXEC         KU32_C(0x00000004)
/** Resource object. */
#define OBJRSRC         KU32_C(0x00000008)
/** The object is discarable (i.e. don't swap, just load in pages from the executable).
 * This overlaps a bit with object type. */
#define OBJDISCARD      KU32_C(0x00000010)
/** The object is shared. */
#define OBJSHARED       KU32_C(0x00000020)
/** The object has preload pages. */
#define OBJPRELOAD      KU32_C(0x00000040)
/** The object has invalid pages. */
#define OBJINVALID      KU32_C(0x00000080)
/** Non-permanent, link386 bug. */
#define LNKNONPERM      KU32_C(0x00000600)
/** Non-permanent, correct 'value'. */
#define OBJNONPERM      KU32_C(0x00000000)
/** Obj Type: The object is permanent and swappable. */
#define OBJPERM         KU32_C(0x00000100)
/** Obj Type: The object is permanent and resident (i.e. not swappable). */
#define OBJRESIDENT     KU32_C(0x00000200)
/** Obj Type: The object is resident and contigious. */
#define OBJCONTIG       KU32_C(0x00000300)
/** Obj Type: The object is permanent and long locable. */
#define OBJDYNAMIC      KU32_C(0x00000400)
/** Object type mask. */
#define OBJTYPEMASK     KU32_C(0x00000700)
/** x86: The object require an 16:16 alias. */
#define OBJALIAS16      KU32_C(0x00001000)
/** x86: Big/Default selector setting, i.e. toggle 32-bit or 16-bit. */
#define OBJBIGDEF       KU32_C(0x00002000)
/** x86: conforming selector setting (weird stuff). */
#define OBJCONFORM      KU32_C(0x00004000)
/** x86: IOPL. */
#define OBJIOPL         KU32_C(0x00008000)
/** @} */

/** A Object Page Map Entry. */
struct o32_map
{
    /** The file offset of the page. */
    KU32                o32_pagedataoffset;
    /** The number of bytes of raw page data. */
    KU16                o32_pagesize;
    /** Per page flags describing how the page is encoded in the file. */
    KU16                o32_pageflags;
};

/** @name o32 o32_pageflags
 * @{
 */
/** Raw page (uncompressed) in the file. */
#define VALID           KU16_C(0x0000)
/** RLE encoded page in file. */
#define ITERDATA        KU16_C(0x0001)
/** Invalid page, nothing in the file. */
#define INVALID         KU16_C(0x0002)
/** Zero page, nothing in file. */
#define ZEROED          KU16_C(0x0003)
/** range of pages (what is this?) */
#define RANGE           KU16_C(0x0004)
/** Compressed page in file. */
#define ITERDATA2       KU16_C(0x0005)
/** @} */


/** Iteration Record format (RLE compressed page). */
struct LX_Iter
{
    /** Number of iterations. */
    KU16                LX_nIter;
    /** The number of bytes that's being iterated. */
    KU16                LX_nBytes;
    /** The bytes. */
    KU8                 LX_Iterdata;
};

/** @} */


/** A Resource Table Entry */
struct rsrc32
{
    /** Resource Type. */
    KU16                type;
    /** Resource ID. */
    KU16                name;
    /** Resource size in bytes. */
    KU32                cb;
    /** The index of the object containing the resource. */
    KU16                obj;
    /** Offset of the resource that within the object. */
    KU32                offset;
};


/** @name The Entry Table (aka Export Table)
 * @{ */

/** Entry bundle.
 * Header descripting up to 255 entries that follows immediatly after this structure. */
struct b32_bundle
{
    /** The number of entries. */
    KU8                 b32_cnt;
    /** The type of bundle. */
    KU8                 b32_type;
    /** The index of the object containing these entry points. */
    KU16                b32_obj;
};

/** @name b32_type
 * @{ */
/** Empty bundle, filling up unused ranges of ordinals. */
#define EMPTY           0x00
/** 16-bit offset entry point. */
#define ENTRY16         0x01
/** 16-bit callgate entry point. */
#define GATE16          0x02
/** 32-bit offset entry point. */
#define ENTRY32         0x03
/** Forwarder entry point. */
#define ENTRYFWD        0x04
/** Typing information present indicator. */
#define TYPEINFO        0x80
/** @} */


/** Entry point. */
struct e32_entry
{
    /** Entry point flags */
    KU8                 e32_flags;      /* Entry point flags */
    union entrykind
    {
        /** ENTRY16 or ENTRY32. */
        offset          e32_offset;
        /** GATE16 */
        struct callgate
        {
            /** Offset into segment. */
            KU16        offset;
            /** The callgate selector */
            KU16        callgate;
        } e32_callgate;
        /** ENTRYFWD */
        struct fwd
        {
            /** Module ordinal number (i.e. into the import module table). */
            KU16        modord;
            /** Procedure name or ordinal number. */
            KU32        value;
        } e32_fwd;
    } e32_variant;
};

/** @name e32_flags
 * @{ */
/** Exported entry (set) or private entry (clear). */
#define E32EXPORT       0x01
/** Uses shared data. */
#define E32SHARED       0x02
/** Parameter word count mask. */
#define E32PARAMS       0xf8
/** ENTRYFWD: Imported by ordinal (set) or by name (clear). */
#define FWD_ORDINAL     0x01
/** @} */

/** @name dunno
 * @{ */
#define FIXENT16        3
#define FIXENT32        5
#define GATEENT16       5
#define FWDENT          7
/** @} */

#pragma pack()

#endif


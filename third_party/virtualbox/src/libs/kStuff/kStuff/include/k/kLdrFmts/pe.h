/* $Id: pe.h 92 2016-09-08 15:31:37Z bird $ */
/** @file
 * PE structures, types and defines.
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

#ifndef ___k_kLdrFmts_pe_h___
#define ___k_kLdrFmts_pe_h___


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kTypes.h>
#include <k/kDefs.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#ifndef IMAGE_NT_SIGNATURE
# define  IMAGE_NT_SIGNATURE K_LE2H_U32('P' | ('E' << 8))
#endif

/* file header */
#define  IMAGE_FILE_MACHINE_UNKNOWN  0x0000
#define  IMAGE_FILE_MACHINE_I386  0x014c
#define  IMAGE_FILE_MACHINE_AMD64  0x8664
#define  IMAGE_FILE_MACHINE_ARM  0x01c0
#define  IMAGE_FILE_MACHINE_ARMNT 0x01c4
#define  IMAGE_FILE_MACHINE_ARM64  0xaa64
#define  IMAGE_FILE_MACHINE_EBC  0x0ebc

#define  IMAGE_FILE_RELOCS_STRIPPED  0x0001
#define  IMAGE_FILE_EXECUTABLE_IMAGE  0x0002
#define  IMAGE_FILE_LINE_NUMS_STRIPPED  0x0004
#define  IMAGE_FILE_LOCAL_SYMS_STRIPPED  0x0008
#define  IMAGE_FILE_AGGRESIVE_WS_TRIM  0x0010
#define  IMAGE_FILE_LARGE_ADDRESS_AWARE  0x0020
#define  IMAGE_FILE_16BIT_MACHINE  0x0040
#define  IMAGE_FILE_BYTES_REVERSED_LO  0x0080
#define  IMAGE_FILE_32BIT_MACHINE  0x0100
#define  IMAGE_FILE_DEBUG_STRIPPED  0x0200
#define  IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP  0x0400
#define  IMAGE_FILE_NET_RUN_FROM_SWAP  0x0800
#define  IMAGE_FILE_SYSTEM  0x1000
#define  IMAGE_FILE_DLL  0x2000
#define  IMAGE_FILE_UP_SYSTEM_ONLY  0x4000
#define  IMAGE_FILE_BYTES_REVERSED_HI  0x8000

/** Raw UUID byte for the ANON_OBJECT_HEADER_BIGOBJ::ClassID value.
 * These make out {d1baa1c7-baee-4ba9-af20-faf66aa4dcb8}. */
#define  ANON_OBJECT_HEADER_BIGOBJ_CLS_ID_BYTES \
        0xc7, 0xa1, 0xba, 0xd1,/*-*/ 0xee, 0xba,/*-*/ 0xa9, 0x4b,/*-*/ 0xaf, 0x20,/*-*/ 0xfa, 0xf6, 0x6a, 0xa4, 0xdc, 0xb8

/* optional header */
#define  IMAGE_NT_OPTIONAL_HDR32_MAGIC  0x10B
#define  IMAGE_NT_OPTIONAL_HDR64_MAGIC  0x20B

#define  IMAGE_SUBSYSTEM_UNKNOWN  0x0
#define  IMAGE_SUBSYSTEM_NATIVE  0x1
#define  IMAGE_SUBSYSTEM_WINDOWS_GUI  0x2
#define  IMAGE_SUBSYSTEM_WINDOWS_CUI  0x3
#define  IMAGE_SUBSYSTEM_OS2_GUI  0x4
#define  IMAGE_SUBSYSTEM_OS2_CUI  0x5
#define  IMAGE_SUBSYSTEM_POSIX_CUI  0x7

#define  IMAGE_LIBRARY_PROCESS_INIT  0x0001
#define  IMAGE_LIBRARY_PROCESS_TERM  0x0002
#define  IMAGE_LIBRARY_THREAD_INIT  0x0004
#define  IMAGE_LIBRARY_THREAD_TERM  0x0008
#define  IMAGE_DLLCHARACTERISTICS_NO_ISOLATION  0x0200
#define  IMAGE_DLLCHARACTERISTICS_NO_SEH  0x0400
#define  IMAGE_DLLCHARACTERISTICS_NO_BIND  0x0800
#define  IMAGE_DLLCHARACTERISTICS_WDM_DRIVER  0x2000
#define  IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE  0x8000

#define  IMAGE_NUMBEROF_DIRECTORY_ENTRIES  0x10

#define  IMAGE_DIRECTORY_ENTRY_EXPORT  0x0
#define  IMAGE_DIRECTORY_ENTRY_IMPORT  0x1
#define  IMAGE_DIRECTORY_ENTRY_RESOURCE  0x2
#define  IMAGE_DIRECTORY_ENTRY_EXCEPTION  0x3
#define  IMAGE_DIRECTORY_ENTRY_SECURITY  0x4
#define  IMAGE_DIRECTORY_ENTRY_BASERELOC  0x5
#define  IMAGE_DIRECTORY_ENTRY_DEBUG  0x6
#define  IMAGE_DIRECTORY_ENTRY_ARCHITECTURE  0x7
#define  IMAGE_DIRECTORY_ENTRY_COPYRIGHT IMAGE_DIRECTORY_ENTRY_ARCHITECTURE
#define  IMAGE_DIRECTORY_ENTRY_GLOBALPTR  0x8
#define  IMAGE_DIRECTORY_ENTRY_TLS  0x9
#define  IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG  0xa
#define  IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT  0xb
#define  IMAGE_DIRECTORY_ENTRY_IAT  0xc
#define  IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT  0xd
#define  IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR  0xe


/* section header */
#define  IMAGE_SIZEOF_SHORT_NAME  0x8

#define  IMAGE_SCN_TYPE_REG  0x00000000
#define  IMAGE_SCN_TYPE_DSECT  0x00000001
#define  IMAGE_SCN_TYPE_NOLOAD  0x00000002
#define  IMAGE_SCN_TYPE_GROUP  0x00000004
#define  IMAGE_SCN_TYPE_NO_PAD  0x00000008
#define  IMAGE_SCN_TYPE_COPY  0x00000010

#define  IMAGE_SCN_CNT_CODE  0x00000020
#define  IMAGE_SCN_CNT_INITIALIZED_DATA  0x00000040
#define  IMAGE_SCN_CNT_UNINITIALIZED_DATA  0x00000080

#define  IMAGE_SCN_LNK_OTHER  0x00000100
#define  IMAGE_SCN_LNK_INFO  0x00000200
#define  IMAGE_SCN_TYPE_OVER  0x00000400
#define  IMAGE_SCN_LNK_REMOVE  0x00000800
#define  IMAGE_SCN_LNK_COMDAT  0x00001000
#define  IMAGE_SCN_MEM_PROTECTED  0x00004000
#define  IMAGE_SCN_NO_DEFER_SPEC_EXC  0x00004000
#define  IMAGE_SCN_GPREL  0x00008000
#define  IMAGE_SCN_MEM_FARDATA  0x00008000
#define  IMAGE_SCN_MEM_SYSHEAP  0x00010000
#define  IMAGE_SCN_MEM_PURGEABLE  0x00020000
#define  IMAGE_SCN_MEM_16BIT  0x00020000
#define  IMAGE_SCN_MEM_LOCKED  0x00040000
#define  IMAGE_SCN_MEM_PRELOAD  0x00080000

#define  IMAGE_SCN_ALIGN_1BYTES  0x00100000
#define  IMAGE_SCN_ALIGN_2BYTES  0x00200000
#define  IMAGE_SCN_ALIGN_4BYTES  0x00300000
#define  IMAGE_SCN_ALIGN_8BYTES  0x00400000
#define  IMAGE_SCN_ALIGN_16BYTES  0x00500000
#define  IMAGE_SCN_ALIGN_32BYTES  0x00600000
#define  IMAGE_SCN_ALIGN_64BYTES  0x00700000
#define  IMAGE_SCN_ALIGN_128BYTES  0x00800000
#define  IMAGE_SCN_ALIGN_256BYTES  0x00900000
#define  IMAGE_SCN_ALIGN_512BYTES  0x00A00000
#define  IMAGE_SCN_ALIGN_1024BYTES  0x00B00000
#define  IMAGE_SCN_ALIGN_2048BYTES  0x00C00000
#define  IMAGE_SCN_ALIGN_4096BYTES  0x00D00000
#define  IMAGE_SCN_ALIGN_8192BYTES  0x00E00000
#define  IMAGE_SCN_ALIGN_MASK  0x00F00000

#define  IMAGE_SCN_LNK_NRELOC_OVFL  0x01000000
#define  IMAGE_SCN_MEM_DISCARDABLE  0x02000000
#define  IMAGE_SCN_MEM_NOT_CACHED  0x04000000
#define  IMAGE_SCN_MEM_NOT_PAGED  0x08000000
#define  IMAGE_SCN_MEM_SHARED  0x10000000
#define  IMAGE_SCN_MEM_EXECUTE  0x20000000
#define  IMAGE_SCN_MEM_READ  0x40000000
#define  IMAGE_SCN_MEM_WRITE  0x80000000


/* relocations */
#define  IMAGE_REL_BASED_ABSOLUTE  0x0
#define  IMAGE_REL_BASED_HIGH  0x1
#define  IMAGE_REL_BASED_LOW  0x2
#define  IMAGE_REL_BASED_HIGHLOW  0x3
#define  IMAGE_REL_BASED_HIGHADJ  0x4
#define  IMAGE_REL_BASED_MIPS_JMPADDR  0x5
#define  IMAGE_REL_BASED_SECTION  0x6
#define  IMAGE_REL_BASED_REL32  0x7
/*#define  IMAGE_REL_BASED_RESERVED1 0x8 */
#define  IMAGE_REL_BASED_MIPS_JMPADDR16 0x9
#define  IMAGE_REL_BASED_IA64_IMM64  0x9
#define  IMAGE_REL_BASED_DIR64  0xa
#define  IMAGE_REL_BASED_HIGH3ADJ 0xb

/* imports */
#define  IMAGE_ORDINAL_FLAG32  0x80000000
#define  IMAGE_ORDINAL32(ord)  ((ord) &  0xffff)
#define  IMAGE_SNAP_BY_ORDINAL32(ord)  (!!((ord) & IMAGE_ORDINAL_FLAG32))

#define  IMAGE_ORDINAL_FLAG64  0x8000000000000000ULL
#define  IMAGE_ORDINAL64(ord)  ((ord) &  0xffff)
#define  IMAGE_SNAP_BY_ORDINAL64(ord)  (!!((ord) & IMAGE_ORDINAL_FLAG64))


/* dll/tls entry points argument */
#define DLL_PROCESS_DETACH      0
#define DLL_PROCESS_ATTACH      1
#define DLL_THREAD_ATTACH       2
#define DLL_THREAD_DETACH       3


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
#pragma pack(4)

typedef struct _IMAGE_FILE_HEADER
{
    KU16      Machine;
    KU16      NumberOfSections;
    KU32      TimeDateStamp;
    KU32      PointerToSymbolTable;
    KU32      NumberOfSymbols;
    KU16      SizeOfOptionalHeader;
    KU16      Characteristics;
} IMAGE_FILE_HEADER;
typedef IMAGE_FILE_HEADER *PIMAGE_FILE_HEADER;


typedef struct _ANON_OBJECT_HEADER
{
    KU16        Sig1;
    KU16        Sig2;
    KU16        Version;                /**< >= 1 */
    KU16        Machine;
    KU32        TimeDataStamp;
    KU8         ClassID[16];
    KU32        SizeOfData;
} ANON_OBJECT_HEADER;
typedef ANON_OBJECT_HEADER *PANON_OBJECT_HEADER;


typedef struct _ANON_OBJECT_HEADER_V2
{
    KU16        Sig1;
    KU16        Sig2;
    KU16        Version;                /**< >= 2 */
    KU16        Machine;
    KU32        TimeDataStamp;
    KU8         ClassID[16];
    KU32        SizeOfData;
    /* New fields for Version >= 2: */
    KU32        Flags;
    KU32        MetaDataSize;           /**< CLR metadata  */
    KU32        MetaDataOffset;
} ANON_OBJECT_HEADER_V2;
typedef ANON_OBJECT_HEADER_V2 *PANON_OBJECT_HEADER_V2;


typedef struct _ANON_OBJECT_HEADER_BIGOBJ
{
    KU16        Sig1;
    KU16        Sig2;
    KU16        Version;                /**< >= 2 */
    KU16        Machine;
    KU32        TimeDataStamp;
    KU8         ClassID[16];            /**< ANON_OBJECT_HEADER_BIGOBJ_CLS_ID_BYTES */
    KU32        SizeOfData;
    /* New fields for Version >= 2: */
    KU32        Flags;
    KU32        MetaDataSize;           /**< CLR metadata  */
    KU32        MetaDataOffset;
    /* Specific for bigobj: */
    KU32        NumberOfSections;
    KU32        PointerToSymbolTable;
    KU32        NumberOfSymbols;
} ANON_OBJECT_HEADER_BIGOBJ;
typedef ANON_OBJECT_HEADER_BIGOBJ *PANON_OBJECT_HEADER_BIGOBJ;


typedef struct _IMAGE_DATA_DIRECTORY
{
    KU32      VirtualAddress;
    KU32      Size;
} IMAGE_DATA_DIRECTORY;
typedef IMAGE_DATA_DIRECTORY *PIMAGE_DATA_DIRECTORY;


typedef struct _IMAGE_OPTIONAL_HEADER32
{
    KU16    Magic;
    KU8     MajorLinkerVersion;
    KU8     MinorLinkerVersion;
    KU32    SizeOfCode;
    KU32    SizeOfInitializedData;
    KU32    SizeOfUninitializedData;
    KU32    AddressOfEntryPoint;
    KU32    BaseOfCode;
    KU32    BaseOfData;
    KU32    ImageBase;
    KU32    SectionAlignment;
    KU32    FileAlignment;
    KU16    MajorOperatingSystemVersion;
    KU16    MinorOperatingSystemVersion;
    KU16    MajorImageVersion;
    KU16    MinorImageVersion;
    KU16    MajorSubsystemVersion;
    KU16    MinorSubsystemVersion;
    KU32    Win32VersionValue;
    KU32    SizeOfImage;
    KU32    SizeOfHeaders;
    KU32    CheckSum;
    KU16    Subsystem;
    KU16    DllCharacteristics;
    KU32    SizeOfStackReserve;
    KU32    SizeOfStackCommit;
    KU32    SizeOfHeapReserve;
    KU32    SizeOfHeapCommit;
    KU32    LoaderFlags;
    KU32    NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32;
typedef IMAGE_OPTIONAL_HEADER32 *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_OPTIONAL_HEADER64
{
    KU16    Magic;
    KU8     MajorLinkerVersion;
    KU8     MinorLinkerVersion;
    KU32    SizeOfCode;
    KU32    SizeOfInitializedData;
    KU32    SizeOfUninitializedData;
    KU32    AddressOfEntryPoint;
    KU32    BaseOfCode;
    KU64    ImageBase;
    KU32    SectionAlignment;
    KU32    FileAlignment;
    KU16    MajorOperatingSystemVersion;
    KU16    MinorOperatingSystemVersion;
    KU16    MajorImageVersion;
    KU16    MinorImageVersion;
    KU16    MajorSubsystemVersion;
    KU16    MinorSubsystemVersion;
    KU32    Win32VersionValue;
    KU32    SizeOfImage;
    KU32    SizeOfHeaders;
    KU32    CheckSum;
    KU16    Subsystem;
    KU16    DllCharacteristics;
    KU64    SizeOfStackReserve;
    KU64    SizeOfStackCommit;
    KU64    SizeOfHeapReserve;
    KU64    SizeOfHeapCommit;
    KU32    LoaderFlags;
    KU32    NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;
typedef IMAGE_OPTIONAL_HEADER64 *PIMAGE_OPTIONAL_HEADER64;


typedef struct _IMAGE_NT_HEADERS
{
    KU32 Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;
typedef IMAGE_NT_HEADERS32 *PIMAGE_NT_HEADERS32;

typedef struct _IMAGE_NT_HEADERS64
{
    KU32 Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;
typedef IMAGE_NT_HEADERS64 *PIMAGE_NT_HEADERS64;


typedef struct _IMAGE_SECTION_HEADER
{
    KU8      Name[IMAGE_SIZEOF_SHORT_NAME];
    union
    {
        KU32      PhysicalAddress;
        KU32      VirtualSize;
    } Misc;
    KU32      VirtualAddress;
    KU32      SizeOfRawData;
    KU32      PointerToRawData;
    KU32      PointerToRelocations;
    KU32      PointerToLinenumbers;
    KU16      NumberOfRelocations;
    KU16      NumberOfLinenumbers;
    KU32      Characteristics;
} IMAGE_SECTION_HEADER;
typedef IMAGE_SECTION_HEADER *PIMAGE_SECTION_HEADER;


typedef struct _IMAGE_BASE_RELOCATION
{
    KU32      VirtualAddress;
    KU32      SizeOfBlock;
} IMAGE_BASE_RELOCATION;
typedef IMAGE_BASE_RELOCATION *PIMAGE_BASE_RELOCATION;


typedef struct _IMAGE_EXPORT_DIRECTORY
{
    KU32      Characteristics;
    KU32      TimeDateStamp;
    KU16      MajorVersion;
    KU16      MinorVersion;
    KU32      Name;
    KU32      Base;
    KU32      NumberOfFunctions;
    KU32      NumberOfNames;
    KU32      AddressOfFunctions;
    KU32      AddressOfNames;
    KU32      AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;


typedef struct _IMAGE_IMPORT_DESCRIPTOR
{
    union
    {
        KU32      Characteristics;
        KU32      OriginalFirstThunk;
    } u;
    KU32      TimeDateStamp;
    KU32      ForwarderChain;
    KU32      Name;
    KU32      FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR *PIMAGE_IMPORT_DESCRIPTOR;


typedef struct _IMAGE_IMPORT_BY_NAME
{
    KU16      Hint;
    KU8       Name[1];
} IMAGE_IMPORT_BY_NAME;
typedef IMAGE_IMPORT_BY_NAME *PIMAGE_IMPORT_BY_NAME;


/* The image_thunk_data32/64 structures are not very helpful except for getting RSI. keep them around till all the code has been converted. */
typedef struct _IMAGE_THUNK_DATA64
{
    union
    {
        KU64      ForwarderString;
        KU64      Function;
        KU64      Ordinal;
        KU64      AddressOfData;
    } u1;
} IMAGE_THUNK_DATA64;
typedef IMAGE_THUNK_DATA64 *PIMAGE_THUNK_DATA64;

typedef struct _IMAGE_THUNK_DATA32
{
    union
    {
        KU32      ForwarderString;
        KU32      Function;
        KU32      Ordinal;
        KU32      AddressOfData;
    } u1;
} IMAGE_THUNK_DATA32;
typedef IMAGE_THUNK_DATA32 *PIMAGE_THUNK_DATA32;


typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY32
{
    KU32      Size;
    KU32      TimeDateStamp;
    KU16      MajorVersion;
    KU16      MinorVersion;
    KU32      GlobalFlagsClear;
    KU32      GlobalFlagsSet;
    KU32      CriticalSectionDefaultTimeout;
    KU32      DeCommitFreeBlockThreshold;
    KU32      DeCommitTotalFreeThreshold;
    KU32      LockPrefixTable;
    KU32      MaximumAllocationSize;
    KU32      VirtualMemoryThreshold;
    KU32      ProcessHeapFlags;
    KU32      ProcessAffinityMask;
    KU16      CSDVersion;
    KU16      Reserved1;
    KU32      EditList;
    KU32      SecurityCookie;
    KU32      SEHandlerTable;
    KU32      SEHandlerCount;
} IMAGE_LOAD_CONFIG_DIRECTORY32;
typedef IMAGE_LOAD_CONFIG_DIRECTORY32 PIMAGE_LOAD_CONFIG_DIRECTORY32;

typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY64
{
    KU32      Size;
    KU32      TimeDateStamp;
    KU16      MajorVersion;
    KU16      MinorVersion;
    KU32      GlobalFlagsClear;
    KU32      GlobalFlagsSet;
    KU32      CriticalSectionDefaultTimeout;
    KU64      DeCommitFreeBlockThreshold;
    KU64      DeCommitTotalFreeThreshold;
    KU64      LockPrefixTable;
    KU64      MaximumAllocationSize;
    KU64      VirtualMemoryThreshold;
    KU64      ProcessAffinityMask;
    KU32      ProcessHeapFlags;
    KU16      CSDVersion;
    KU16      Reserved1;
    KU64      EditList;
    KU64      SecurityCookie;
    KU64      SEHandlerTable;
    KU64      SEHandlerCount;
} IMAGE_LOAD_CONFIG_DIRECTORY64;
typedef IMAGE_LOAD_CONFIG_DIRECTORY64 *PIMAGE_LOAD_CONFIG_DIRECTORY64;

typedef struct _IMAGE_DEBUG_DIRECTORY
{
    KU32      Characteristics;
    KU32      TimeDateStamp;
    KU16      MajorVersion;
    KU16      MinorVersion;
    KU32      Type;
    KU32      SizeOfData;
    KU32      AddressOfRawData;
    KU32      PointerToRawData;
} IMAGE_DEBUG_DIRECTORY;
typedef IMAGE_DEBUG_DIRECTORY *PIMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_UNKNOWN  0
#define IMAGE_DEBUG_TYPE_COFF 1
#define IMAGE_DEBUG_TYPE_CODEVIEW 2 /* 4.0 */
#define IMAGE_DEBUG_TYPE_FPO 3 /* FPO = frame pointer omission */
#define IMAGE_DEBUG_TYPE_MISC 4
#define IMAGE_DEBUG_TYPE_EXCEPTION 5
#define IMAGE_DEBUG_TYPE_FIXUP 6
#define IMAGE_DEBUG_TYPE_BORLAND 9

typedef struct _IMAGE_TLS_DIRECTORY32
{
    KU32      StartAddressOfRawData;
    KU32      EndAddressOfRawData;
    KU32      AddressOfIndex;
    KU32      AddressOfCallBacks;
    KU32      SizeOfZeroFill;
    KU32      Characteristics;
} IMAGE_TLS_DIRECTORY32;
typedef IMAGE_TLS_DIRECTORY32 *PIMAGE_TLS_DIRECTORY32;

typedef struct _IMAGE_TLS_DIRECTORY64
{
    KU64      StartAddressOfRawData;
    KU64      EndAddressOfRawData;
    KU64      AddressOfIndex;
    KU64      AddressOfCallBacks;
    KU32      SizeOfZeroFill;
    KU32      Characteristics;
} IMAGE_TLS_DIRECTORY64;
typedef IMAGE_TLS_DIRECTORY64 *PIMAGE_TLS_DIRECTORY64;


#pragma pack()

#endif


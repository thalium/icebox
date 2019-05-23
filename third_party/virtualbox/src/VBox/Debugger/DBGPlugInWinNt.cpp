/* $Id: DBGPlugInWinNt.cpp $ */
/** @file
 * DBGPlugInWindows - Debugger and Guest OS Digger Plugin For Windows NT.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGF /// @todo add new log group.
#include "DBGPlugIns.h"
#include <VBox/vmm/dbgf.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <iprt/ldr.h>
#include <iprt/mem.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/formats/pecoff.h>
#include <iprt/formats/mz.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/** @name Internal WinNT structures
 * @{ */
/**
 * PsLoadedModuleList entry for 32-bit NT aka LDR_DATA_TABLE_ENTRY.
 * Tested with XP.
 */
typedef struct NTMTE32
{
    struct
    {
        uint32_t    Flink;
        uint32_t    Blink;
    }               InLoadOrderLinks,
                    InMemoryOrderModuleList,
                    InInitializationOrderModuleList;
    uint32_t        DllBase;
    uint32_t        EntryPoint;
    uint32_t        SizeOfImage;
    struct
    {
        uint16_t    Length;
        uint16_t    MaximumLength;
        uint32_t    Buffer;
    }               FullDllName,
                    BaseDllName;
    uint32_t        Flags;
    uint16_t        LoadCount;
    uint16_t        TlsIndex;
    /* ... there is more ... */
} NTMTE32;
typedef NTMTE32 *PNTMTE32;

/**
 * PsLoadedModuleList entry for 64-bit NT aka LDR_DATA_TABLE_ENTRY.
 */
typedef struct NTMTE64
{
    struct
    {
        uint64_t    Flink;
        uint64_t    Blink;
    }               InLoadOrderLinks,                  /**< 0x00 */
                    InMemoryOrderModuleList,           /**< 0x10 */
                    InInitializationOrderModuleList;   /**< 0x20 */
    uint64_t        DllBase;                           /**< 0x30 */
    uint64_t        EntryPoint;                        /**< 0x38 */
    uint32_t        SizeOfImage;                       /**< 0x40 */
    uint32_t        Alignment;                         /**< 0x44 */
    struct
    {
        uint16_t    Length;                            /**< 0x48,0x58 */
        uint16_t    MaximumLength;                     /**< 0x4a,0x5a */
        uint32_t    Alignment;                         /**< 0x4c,0x5c */
        uint64_t    Buffer;                            /**< 0x50,0x60 */
    }               FullDllName,                       /**< 0x48 */
                    BaseDllName;                       /**< 0x58 */
    uint32_t        Flags;                             /**< 0x68 */
    uint16_t        LoadCount;                         /**< 0x6c */
    uint16_t        TlsIndex;                          /**< 0x6e */
    /* ... there is more ... */
} NTMTE64;
typedef NTMTE64 *PNTMTE64;

/** MTE union. */
typedef union NTMTE
{
    NTMTE32         vX_32;
    NTMTE64         vX_64;
} NTMTE;
typedef NTMTE *PNTMTE;


/**
 * The essential bits of the KUSER_SHARED_DATA structure.
 */
typedef struct NTKUSERSHAREDDATA
{
    uint32_t        TickCountLowDeprecated;
    uint32_t        TickCountMultiplier;
    struct
    {
        uint32_t    LowPart;
        int32_t     High1Time;
        int32_t     High2Time;

    }               InterruptTime,
                    SystemTime,
                    TimeZoneBias;
    uint16_t        ImageNumberLow;
    uint16_t        ImageNumberHigh;
    RTUTF16         NtSystemRoot[260];
    uint32_t        MaxStackTraceDepth;
    uint32_t        CryptoExponent;
    uint32_t        TimeZoneId;
    uint32_t        LargePageMinimum;
    uint32_t        Reserved2[7];
    uint32_t        NtProductType;
    uint8_t         ProductTypeIsValid;
    uint8_t         abPadding[3];
    uint32_t        NtMajorVersion;
    uint32_t        NtMinorVersion;
    /* uint8_t         ProcessorFeatures[64];
    ...
    */
} NTKUSERSHAREDDATA;
typedef NTKUSERSHAREDDATA *PNTKUSERSHAREDDATA;

/** KI_USER_SHARED_DATA for i386 */
#define NTKUSERSHAREDDATA_WINNT32   UINT32_C(0xffdf0000)
/** KI_USER_SHARED_DATA for AMD64 */
#define NTKUSERSHAREDDATA_WINNT64   UINT64_C(0xfffff78000000000)

/** NTKUSERSHAREDDATA::NtProductType */
typedef enum NTPRODUCTTYPE
{
    kNtProductType_Invalid = 0,
    kNtProductType_WinNt = 1,
    kNtProductType_LanManNt,
    kNtProductType_Server
} NTPRODUCTTYPE;


/** NT image header union. */
typedef union NTHDRSU
{
    IMAGE_NT_HEADERS32  vX_32;
    IMAGE_NT_HEADERS64  vX_64;
} NTHDRS;
/** Pointer to NT image header union. */
typedef NTHDRS *PNTHDRS;
/** Pointer to const NT image header union. */
typedef NTHDRS const *PCNTHDRS;

/** @} */



typedef enum DBGDIGGERWINNTVER
{
    DBGDIGGERWINNTVER_UNKNOWN,
    DBGDIGGERWINNTVER_3_1,
    DBGDIGGERWINNTVER_3_5,
    DBGDIGGERWINNTVER_4_0,
    DBGDIGGERWINNTVER_5_0,
    DBGDIGGERWINNTVER_5_1,
    DBGDIGGERWINNTVER_6_0
} DBGDIGGERWINNTVER;

/**
 * WinNT guest OS digger instance data.
 */
typedef struct DBGDIGGERWINNT
{
    /** Whether the information is valid or not.
     * (For fending off illegal interface method calls.) */
    bool                fValid;
    /** 32-bit (true) or 64-bit (false) */
    bool                f32Bit;

    /** The NT version. */
    DBGDIGGERWINNTVER   enmVer;
    /** NTKUSERSHAREDDATA::NtProductType */
    NTPRODUCTTYPE       NtProductType;
    /** NTKUSERSHAREDDATA::NtMajorVersion */
    uint32_t            NtMajorVersion;
    /** NTKUSERSHAREDDATA::NtMinorVersion */
    uint32_t            NtMinorVersion;

    /** The address of the ntoskrnl.exe image. */
    DBGFADDRESS         KernelAddr;
    /** The address of the ntoskrnl.exe module table entry. */
    DBGFADDRESS         KernelMteAddr;
    /** The address of PsLoadedModuleList. */
    DBGFADDRESS         PsLoadedModuleListAddr;
} DBGDIGGERWINNT;
/** Pointer to the linux guest OS digger instance data. */
typedef DBGDIGGERWINNT *PDBGDIGGERWINNT;


/**
 * The WinNT digger's loader reader instance data.
 */
typedef struct DBGDIGGERWINNTRDR
{
    /** The VM handle (referenced). */
    PUVM                pUVM;
    /** The image base. */
    DBGFADDRESS         ImageAddr;
    /** The image size. */
    uint32_t            cbImage;
    /** The file offset of the SizeOfImage field in the optional header if it
     *  needs patching, otherwise set to UINT32_MAX. */
    uint32_t            offSizeOfImage;
    /** The correct image size. */
    uint32_t            cbCorrectImageSize;
    /** Number of entries in the aMappings table. */
    uint32_t            cMappings;
    /** Mapping hint. */
    uint32_t            iHint;
    /** Mapping file offset to memory offsets, ordered by file offset. */
    struct
    {
        /** The file offset. */
        uint32_t        offFile;
        /** The size of this mapping. */
        uint32_t        cbMem;
        /** The offset to the memory from the start of the image.  */
        uint32_t        offMem;
    } aMappings[1];
} DBGDIGGERWINNTRDR;
/** Pointer a WinNT loader reader instance data. */
typedef DBGDIGGERWINNTRDR *PDBGDIGGERWINNTRDR;


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Validates a 32-bit Windows NT kernel address */
#define WINNT32_VALID_ADDRESS(Addr)         ((Addr) >         UINT32_C(0x80000000) && (Addr) <         UINT32_C(0xfffff000))
/** Validates a 64-bit Windows NT kernel address */
 #define WINNT64_VALID_ADDRESS(Addr)         ((Addr) > UINT64_C(0xffff800000000000) && (Addr) < UINT64_C(0xfffffffffffff000))
/** Validates a kernel address. */
#define WINNT_VALID_ADDRESS(pThis, Addr)    ((pThis)->f32Bit ? WINNT32_VALID_ADDRESS(Addr) : WINNT64_VALID_ADDRESS(Addr))
/** Versioned and bitness wrapper. */
#define WINNT_UNION(pThis, pUnion, Member)  ((pThis)->f32Bit ? (pUnion)->vX_32. Member : (pUnion)->vX_64. Member )

/** The length (in chars) of the kernel file name (no path). */
#define WINNT_KERNEL_BASE_NAME_LEN          12

/** WindowsNT on little endian ASCII systems. */
#define DIG_WINNT_MOD_TAG                   UINT64_C(0x54696e646f774e54)


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int)  dbgDiggerWinNtInit(PUVM pUVM, void *pvData);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Kernel names. */
static const RTUTF16 g_wszKernelNames[][WINNT_KERNEL_BASE_NAME_LEN + 1] =
{
    { 'n', 't', 'o', 's', 'k', 'r', 'n', 'l', '.', 'e', 'x', 'e' }
};



/** @callback_method_impl{PFNRTLDRRDRMEMREAD} */
static DECLCALLBACK(int) dbgDiggerWinNtRdr_Read(void *pvBuf, size_t cb, size_t off, void *pvUser)
{
    PDBGDIGGERWINNTRDR pThis = (PDBGDIGGERWINNTRDR)pvUser;
    uint32_t           offFile = (uint32_t)off;
    AssertReturn(offFile == off, VERR_INVALID_PARAMETER);

    uint32_t i = pThis->iHint;
    if (pThis->aMappings[i].offFile > offFile)
    {
        i = pThis->cMappings;
        while (i-- > 0)
            if (offFile >= pThis->aMappings[i].offFile)
                break;
        pThis->iHint = i;
    }

    while (cb > 0)
    {
        uint32_t offNextMap =  i + 1 < pThis->cMappings ? pThis->aMappings[i + 1].offFile : pThis->cbImage;
        uint32_t offMap     = offFile - pThis->aMappings[i].offFile;

        /* Read file bits backed by memory. */
        if (offMap < pThis->aMappings[i].cbMem)
        {
            uint32_t cbToRead = pThis->aMappings[i].cbMem - offMap;
            if (cbToRead > cb)
                cbToRead = (uint32_t)cb;

            DBGFADDRESS Addr = pThis->ImageAddr;
            DBGFR3AddrAdd(&Addr, pThis->aMappings[i].offMem + offMap);

            int rc = DBGFR3MemRead(pThis->pUVM, 0 /*idCpu*/, &Addr, pvBuf, cbToRead);
            if (RT_FAILURE(rc))
                return rc;

            /* Apply SizeOfImage patch? */
            if (   pThis->offSizeOfImage != UINT32_MAX
                && offFile            < pThis->offSizeOfImage + 4
                && offFile + cbToRead > pThis->offSizeOfImage)
            {
                uint32_t SizeOfImage = pThis->cbCorrectImageSize;
                uint32_t cbPatch     = sizeof(SizeOfImage);
                int32_t  offPatch    = pThis->offSizeOfImage - offFile;
                uint8_t *pbPatch     = (uint8_t *)pvBuf + offPatch;
                if (offFile + cbToRead < pThis->offSizeOfImage + cbPatch)
                    cbPatch = offFile + cbToRead - pThis->offSizeOfImage;
                while (cbPatch-- > 0)
                {
                    if (offPatch >= 0)
                        *pbPatch = (uint8_t)SizeOfImage;
                    offPatch++;
                    pbPatch++;
                    SizeOfImage >>= 8;
                }
            }

            /* Done? */
            if (cbToRead == cb)
                break;

            offFile += cbToRead;
            cb      -= cbToRead;
            pvBuf    = (char *)pvBuf + cbToRead;
        }

        /* Mind the gap. */
        if (offNextMap > offFile)
        {
            uint32_t cbZero = offNextMap - offFile;
            if (cbZero > cb)
            {
                RT_BZERO(pvBuf, cb);
                break;
            }

            RT_BZERO(pvBuf, cbZero);
            offFile += cbZero;
            cb      -= cbZero;
            pvBuf   = (char *)pvBuf + cbZero;
        }

        pThis->iHint = ++i;
    }

    return VINF_SUCCESS;
}


/** @callback_method_impl{PFNRTLDRRDRMEMDTOR} */
static DECLCALLBACK(void) dbgDiggerWinNtRdr_Dtor(void *pvUser)
{
    PDBGDIGGERWINNTRDR pThis = (PDBGDIGGERWINNTRDR)pvUser;

    VMR3ReleaseUVM(pThis->pUVM);
    pThis->pUVM = NULL;
    RTMemFree(pvUser);
}


/**
 * Checks if the section headers look okay.
 *
 * @returns true / false.
 * @param   paShs               Pointer to the section headers.
 * @param   cShs                Number of headers.
 * @param   cbImage             The image size reported by NT.
 * @param   uRvaRsrc            The RVA of the resource directory. UINT32_MAX if
 *                              no resource directory.
 * @param   cbSectAlign         The section alignment specified in the header.
 * @param   pcbImageCorrect     The corrected image size.  This is derived from
 *                              cbImage and virtual range of the section tables.
 *
 *                              The problem is that NT may choose to drop the
 *                              last pages in images it loads early, starting at
 *                              the resource directory.  These images will have
 *                              a page aligned cbImage.
 */
static bool dbgDiggerWinNtCheckSectHdrsAndImgSize(PCIMAGE_SECTION_HEADER paShs, uint32_t cShs, uint32_t cbImage,
                                                  uint32_t uRvaRsrc, uint32_t cbSectAlign, uint32_t *pcbImageCorrect)
{
    *pcbImageCorrect = cbImage;

    for (uint32_t i = 0; i < cShs; i++)
    {
        if (!paShs[i].Name[0])
        {
            Log(("DigWinNt: Section header #%u has no name\n", i));
            return false;
        }

        if (paShs[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD)
            continue;

        /* Check that sizes are within the same range and that both sizes and
           addresses are within reasonable limits. */
        if (   RT_ALIGN(paShs[i].Misc.VirtualSize, _64K) < RT_ALIGN(paShs[i].SizeOfRawData, _64K)
            || paShs[i].Misc.VirtualSize >= _1G
            || paShs[i].SizeOfRawData    >= _1G)
        {
            Log(("DigWinNt: Section header #%u has a VirtualSize=%#x and SizeOfRawData=%#x, that's too much data!\n",
                 i, paShs[i].Misc.VirtualSize, paShs[i].SizeOfRawData));
            return false;
        }
        uint32_t uRvaEnd = paShs[i].VirtualAddress + paShs[i].Misc.VirtualSize;
        if (uRvaEnd >= _1G || uRvaEnd < paShs[i].VirtualAddress)
        {
            Log(("DigWinNt: Section header #%u has a VirtualSize=%#x and VirtualAddr=%#x, %#x in total, that's too much!\n",
                 i, paShs[i].Misc.VirtualSize, paShs[i].VirtualAddress, uRvaEnd));
            return false;
        }

        /* Check for images chopped off around '.rsrc'. */
        if (    cbImage < uRvaEnd
            &&  uRvaEnd >= uRvaRsrc)
            cbImage = RT_ALIGN(uRvaEnd, cbSectAlign);

        /* Check that the section is within the image. */
        if (uRvaEnd > cbImage)
        {
            Log(("DigWinNt: Section header #%u has a virtual address range beyond the image: %#x TO %#x cbImage=%#x\n",
                 i, paShs[i].VirtualAddress, uRvaEnd, cbImage));
            return false;
        }
    }

    Assert(*pcbImageCorrect == cbImage || !(*pcbImageCorrect & 0xfff));
    *pcbImageCorrect = cbImage;
    return true;
}


/**
 * Create a loader module for the in-guest-memory PE module.
 */
static int dbgDiggerWinNtCreateLdrMod(PDBGDIGGERWINNT pThis, PUVM pUVM, const char *pszName, PCDBGFADDRESS pImageAddr,
                                      uint32_t cbImage, uint8_t *pbBuf, size_t cbBuf,
                                      uint32_t offHdrs, PCNTHDRS pHdrs, PRTLDRMOD phLdrMod)
{
    /*
     * Allocate and create a reader instance.
     */
    uint32_t const      cShs = WINNT_UNION(pThis, pHdrs, FileHeader.NumberOfSections);
    PDBGDIGGERWINNTRDR  pRdr = (PDBGDIGGERWINNTRDR)RTMemAlloc(RT_OFFSETOF(DBGDIGGERWINNTRDR, aMappings[cShs + 2]));
    if (!pRdr)
        return VERR_NO_MEMORY;

    VMR3RetainUVM(pUVM);
    pRdr->pUVM               = pUVM;
    pRdr->ImageAddr          = *pImageAddr;
    pRdr->cbImage            = cbImage;
    pRdr->cbCorrectImageSize = cbImage;
    pRdr->offSizeOfImage     = UINT32_MAX;
    pRdr->iHint              = 0;

    /*
     * Use the section table to construct a more accurate view of the file/
     * image if it's in the buffer (it should be).
     */
    uint32_t uRvaRsrc = UINT32_MAX;
    if (WINNT_UNION(pThis, pHdrs, OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]).Size > 0)
        uRvaRsrc = WINNT_UNION(pThis, pHdrs, OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]).VirtualAddress;
    uint32_t offShs = offHdrs
                    + (  pThis->f32Bit
                       ? pHdrs->vX_32.FileHeader.SizeOfOptionalHeader + RT_OFFSETOF(IMAGE_NT_HEADERS32, OptionalHeader)
                       : pHdrs->vX_64.FileHeader.SizeOfOptionalHeader + RT_OFFSETOF(IMAGE_NT_HEADERS64, OptionalHeader));
    uint32_t cbShs  = cShs * sizeof(IMAGE_SECTION_HEADER);
    PCIMAGE_SECTION_HEADER paShs = (PCIMAGE_SECTION_HEADER)(pbBuf + offShs);
    if (   offShs + cbShs <= RT_MIN(cbImage, cbBuf)
        && dbgDiggerWinNtCheckSectHdrsAndImgSize(paShs, cShs, cbImage, uRvaRsrc,
                                                 WINNT_UNION(pThis, pHdrs, OptionalHeader.SectionAlignment),
                                                 &pRdr->cbCorrectImageSize))
    {
        pRdr->cMappings = 0;

        for (uint32_t i = 0; i < cShs; i++)
            if (   paShs[i].SizeOfRawData    > 0
                && paShs[i].PointerToRawData > 0)
            {
                uint32_t j = 1;
                if (!pRdr->cMappings)
                    pRdr->cMappings++;
                else
                {
                    while (j < pRdr->cMappings && pRdr->aMappings[j].offFile < paShs[i].PointerToRawData)
                        j++;
                    if (j < pRdr->cMappings)
                        memmove(&pRdr->aMappings[j + 1], &pRdr->aMappings[j], (pRdr->cMappings - j) * sizeof(pRdr->aMappings));
                }
                pRdr->aMappings[j].offFile = paShs[i].PointerToRawData;
                pRdr->aMappings[j].offMem  = paShs[i].VirtualAddress;
                pRdr->aMappings[j].cbMem   = i + 1 < cShs
                                           ? paShs[i + 1].VirtualAddress - paShs[i].VirtualAddress
                                           : paShs[i].Misc.VirtualSize;
                if (j == pRdr->cMappings)
                    pRdr->cbImage = paShs[i].PointerToRawData + paShs[i].SizeOfRawData;
                pRdr->cMappings++;
            }

        /* Insert the mapping of the headers that isn't covered by the section table. */
        pRdr->aMappings[0].offFile = 0;
        pRdr->aMappings[0].offMem  = 0;
        pRdr->aMappings[0].cbMem   = pRdr->cMappings ? pRdr->aMappings[1].offFile : pRdr->cbImage;

        int j = pRdr->cMappings - 1;
        while (j-- > 0)
        {
            uint32_t cbFile = pRdr->aMappings[j + 1].offFile - pRdr->aMappings[j].offFile;
            if (pRdr->aMappings[j].cbMem > cbFile)
                pRdr->aMappings[j].cbMem = cbFile;
        }
    }
    else
    {
        /*
         * Fallback, fake identity mapped file data.
         */
        pRdr->cMappings = 1;
        pRdr->aMappings[0].offFile = 0;
        pRdr->aMappings[0].offMem  = 0;
        pRdr->aMappings[0].cbMem   = pRdr->cbImage;
    }

    /* Enable the SizeOfImage patching if necessary. */
    if (pRdr->cbCorrectImageSize != cbImage)
    {
        Log(("DigWinNT: The image is really %#x bytes long, not %#x as mapped by NT!\n", pRdr->cbCorrectImageSize, cbImage));
        pRdr->offSizeOfImage = pThis->f32Bit
                             ? offHdrs + RT_OFFSETOF(IMAGE_NT_HEADERS32, OptionalHeader.SizeOfImage)
                             : offHdrs + RT_OFFSETOF(IMAGE_NT_HEADERS64, OptionalHeader.SizeOfImage);
    }

    /*
     * Call the loader to open the PE image for debugging.
     * Note! It always calls pfnDtor.
     */
    RTLDRMOD hLdrMod;
    int rc = RTLdrOpenInMemory(pszName, RTLDR_O_FOR_DEBUG, RTLDRARCH_WHATEVER, pRdr->cbImage,
                               dbgDiggerWinNtRdr_Read, dbgDiggerWinNtRdr_Dtor, pRdr,
                               &hLdrMod);
    if (RT_SUCCESS(rc))
        *phLdrMod = hLdrMod;
    else
        *phLdrMod = NIL_RTLDRMOD;
    return rc;
}


/**
 * Process a PE image found in guest memory.
 *
 * @param   pThis           The instance data.
 * @param   pUVM            The user mode VM handle.
 * @param   pszName         The image name.
 * @param   pImageAddr      The image address.
 * @param   cbImage         The size of the image.
 * @param   pbBuf           Scratch buffer containing the first
 *                          RT_MIN(cbBuf, cbImage) bytes of the image.
 * @param   cbBuf           The scratch buffer size.
 */
static void dbgDiggerWinNtProcessImage(PDBGDIGGERWINNT pThis, PUVM pUVM, const char *pszName,
                                       PCDBGFADDRESS pImageAddr, uint32_t cbImage,
                                       uint8_t *pbBuf, size_t cbBuf)
{
    LogFlow(("DigWinNt: %RGp %#x %s\n", pImageAddr->FlatPtr, cbImage, pszName));

    /*
     * Do some basic validation first.
     * This is the usual exteremely verbose and messy code...
     */
    Assert(cbBuf >= sizeof(IMAGE_NT_HEADERS64));
    if (   cbImage < sizeof(IMAGE_NT_HEADERS64)
        || cbImage >= _1M * 256)
    {
        Log(("DigWinNt: %s: Bad image size: %#x\n", pszName, cbImage));
        return;
    }

    /* Dig out the NT/PE headers. */
    IMAGE_DOS_HEADER const *pMzHdr = (IMAGE_DOS_HEADER const *)pbBuf;
    PCNTHDRS        pHdrs;
    uint32_t        offHdrs;
    if (pMzHdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        offHdrs = 0;
        pHdrs   = (PCNTHDRS)pbBuf;
    }
    else if (   pMzHdr->e_lfanew >= cbImage
             || pMzHdr->e_lfanew < sizeof(*pMzHdr)
             || pMzHdr->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > cbImage)
    {
        Log(("DigWinNt: %s: PE header to far into image: %#x  cbImage=%#x\n", pszName, pMzHdr->e_lfanew, cbImage));
        return;
    }
    else if (   pMzHdr->e_lfanew < cbBuf
             && pMzHdr->e_lfanew + sizeof(IMAGE_NT_HEADERS64) <= cbBuf)
    {
        offHdrs = pMzHdr->e_lfanew;
        pHdrs = (NTHDRS const *)(pbBuf + offHdrs);
    }
    else
    {
        Log(("DigWinNt: %s: PE header to far into image (lazy bird): %#x\n", pszName, pMzHdr->e_lfanew));
        return;
    }
    if (pHdrs->vX_32.Signature != IMAGE_NT_SIGNATURE)
    {
        Log(("DigWinNt: %s: Bad PE signature: %#x\n", pszName, pHdrs->vX_32.Signature));
        return;
    }

    /* The file header is the same on both archs */
    if (pHdrs->vX_32.FileHeader.Machine != (pThis->f32Bit ? IMAGE_FILE_MACHINE_I386 : IMAGE_FILE_MACHINE_AMD64))
    {
        Log(("DigWinNt: %s: Invalid FH.Machine: %#x\n", pszName, pHdrs->vX_32.FileHeader.Machine));
        return;
    }
    if (pHdrs->vX_32.FileHeader.SizeOfOptionalHeader != (pThis->f32Bit ? sizeof(IMAGE_OPTIONAL_HEADER32) : sizeof(IMAGE_OPTIONAL_HEADER64)))
    {
        Log(("DigWinNt: %s: Invalid FH.SizeOfOptionalHeader: %#x\n", pszName, pHdrs->vX_32.FileHeader.SizeOfOptionalHeader));
        return;
    }
    if (WINNT_UNION(pThis, pHdrs, FileHeader.NumberOfSections) > 64)
    {
        Log(("DigWinNt: %s: Too many sections: %#x\n", pszName, WINNT_UNION(pThis, pHdrs, FileHeader.NumberOfSections)));
        return;
    }

    const uint32_t TimeDateStamp = pHdrs->vX_32.FileHeader.TimeDateStamp;

    /* The optional header is not... */
    if (WINNT_UNION(pThis, pHdrs, OptionalHeader.Magic) != (pThis->f32Bit ? IMAGE_NT_OPTIONAL_HDR32_MAGIC : IMAGE_NT_OPTIONAL_HDR64_MAGIC))
    {
        Log(("DigWinNt: %s: Invalid OH.Magic: %#x\n", pszName, WINNT_UNION(pThis, pHdrs, OptionalHeader.Magic)));
        return;
    }
    uint32_t cbImageFromHdr = WINNT_UNION(pThis, pHdrs, OptionalHeader.SizeOfImage);
    if (RT_ALIGN(cbImageFromHdr, _4K) != RT_ALIGN(cbImage, _4K))
    {
        Log(("DigWinNt: %s: Invalid OH.SizeOfImage: %#x, expected %#x\n", pszName, cbImageFromHdr, cbImage));
        return;
    }
    if (WINNT_UNION(pThis, pHdrs, OptionalHeader.NumberOfRvaAndSizes) != IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
    {
        Log(("DigWinNt: %s: Invalid OH.NumberOfRvaAndSizes: %#x\n", pszName, WINNT_UNION(pThis, pHdrs, OptionalHeader.NumberOfRvaAndSizes)));
        return;
    }

    /*
     * Create the module using the in memory image first, falling back
     * on cached image.
     */
    RTLDRMOD hLdrMod;
    int rc = dbgDiggerWinNtCreateLdrMod(pThis, pUVM, pszName, pImageAddr, cbImage, pbBuf, cbBuf, offHdrs, pHdrs,
                                        &hLdrMod);
    if (RT_FAILURE(rc))
        hLdrMod = NIL_RTLDRMOD;

    RTDBGMOD hMod;
    rc = RTDbgModCreateFromPeImage(&hMod, pszName, NULL, hLdrMod,
                                   cbImageFromHdr, TimeDateStamp, DBGFR3AsGetConfig(pUVM));
    if (RT_FAILURE(rc))
    {
        /*
         * Final fallback is a container module.
         */
        rc = RTDbgModCreate(&hMod, pszName, cbImage, 0);
        if (RT_FAILURE(rc))
            return;

        rc = RTDbgModSymbolAdd(hMod, "Headers", 0 /*iSeg*/, 0, cbImage, 0 /*fFlags*/, NULL);
        AssertRC(rc);
    }

    /* Tag the module. */
    rc = RTDbgModSetTag(hMod, DIG_WINNT_MOD_TAG);
    AssertRC(rc);

    /*
     * Link the module.
     */
    RTDBGAS hAs = DBGFR3AsResolveAndRetain(pUVM, DBGF_AS_KERNEL);
    if (hAs != NIL_RTDBGAS)
        rc = RTDbgAsModuleLink(hAs, hMod, pImageAddr->FlatPtr, RTDBGASLINK_FLAGS_REPLACE /*fFlags*/);
    else
        rc = VERR_INTERNAL_ERROR;
    RTDbgModRelease(hMod);
    RTDbgAsRelease(hAs);
}


/**
 * @copydoc DBGFOSREG::pfnQueryInterface
 */
static DECLCALLBACK(void *) dbgDiggerWinNtQueryInterface(PUVM pUVM, void *pvData, DBGFOSINTERFACE enmIf)
{
    RT_NOREF3(pUVM, pvData, enmIf);
    return NULL;
}


/**
 * @copydoc DBGFOSREG::pfnQueryVersion
 */
static DECLCALLBACK(int)  dbgDiggerWinNtQueryVersion(PUVM pUVM, void *pvData, char *pszVersion, size_t cchVersion)
{
    RT_NOREF1(pUVM);
    PDBGDIGGERWINNT pThis = (PDBGDIGGERWINNT)pvData;
    Assert(pThis->fValid);
    const char *pszNtProductType;
    switch (pThis->NtProductType)
    {
        case kNtProductType_WinNt:      pszNtProductType = "-WinNT";        break;
        case kNtProductType_LanManNt:   pszNtProductType = "-LanManNT";     break;
        case kNtProductType_Server:     pszNtProductType = "-Server";       break;
        default:                        pszNtProductType = "";              break;
    }
    RTStrPrintf(pszVersion, cchVersion, "%u.%u-%s%s", pThis->NtMajorVersion, pThis->NtMinorVersion,
                pThis->f32Bit ? "x86" : "AMD64", pszNtProductType);
    return VINF_SUCCESS;
}


/**
 * @copydoc DBGFOSREG::pfnTerm
 */
static DECLCALLBACK(void)  dbgDiggerWinNtTerm(PUVM pUVM, void *pvData)
{
    RT_NOREF1(pUVM);
    PDBGDIGGERWINNT pThis = (PDBGDIGGERWINNT)pvData;
    Assert(pThis->fValid);

    pThis->fValid = false;
}


/**
 * @copydoc DBGFOSREG::pfnRefresh
 */
static DECLCALLBACK(int)  dbgDiggerWinNtRefresh(PUVM pUVM, void *pvData)
{
    PDBGDIGGERWINNT pThis = (PDBGDIGGERWINNT)pvData;
    NOREF(pThis);
    Assert(pThis->fValid);

    /*
     * For now we'll flush and reload everything.
     */
    RTDBGAS hDbgAs = DBGFR3AsResolveAndRetain(pUVM, DBGF_AS_KERNEL);
    if (hDbgAs != NIL_RTDBGAS)
    {
        uint32_t iMod = RTDbgAsModuleCount(hDbgAs);
        while (iMod-- > 0)
        {
            RTDBGMOD hMod = RTDbgAsModuleByIndex(hDbgAs, iMod);
            if (hMod != NIL_RTDBGMOD)
            {
                if (RTDbgModGetTag(hMod) == DIG_WINNT_MOD_TAG)
                {
                    int rc = RTDbgAsModuleUnlink(hDbgAs, hMod);
                    AssertRC(rc);
                }
                RTDbgModRelease(hMod);
            }
        }
        RTDbgAsRelease(hDbgAs);
    }

    dbgDiggerWinNtTerm(pUVM, pvData);
    return dbgDiggerWinNtInit(pUVM, pvData);
}


/**
 * @copydoc DBGFOSREG::pfnInit
 */
static DECLCALLBACK(int)  dbgDiggerWinNtInit(PUVM pUVM, void *pvData)
{
    PDBGDIGGERWINNT pThis = (PDBGDIGGERWINNT)pvData;
    Assert(!pThis->fValid);

    union
    {
        uint8_t             au8[0x2000];
        RTUTF16             wsz[0x2000/2];
        NTKUSERSHAREDDATA   UserSharedData;
    }               u;
    DBGFADDRESS     Addr;
    int             rc;

    /*
     * Figure the NT version.
     */
    DBGFR3AddrFromFlat(pUVM, &Addr, pThis->f32Bit ? NTKUSERSHAREDDATA_WINNT32 : NTKUSERSHAREDDATA_WINNT64);
    rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &Addr, &u, PAGE_SIZE);
    if (RT_FAILURE(rc))
        return rc;
    pThis->NtProductType  = u.UserSharedData.ProductTypeIsValid && u.UserSharedData.NtProductType <= kNtProductType_Server
                          ? (NTPRODUCTTYPE)u.UserSharedData.NtProductType
                          : kNtProductType_Invalid;
    pThis->NtMajorVersion = u.UserSharedData.NtMajorVersion;
    pThis->NtMinorVersion = u.UserSharedData.NtMinorVersion;

    /*
     * Dig out the module chain.
     */
    DBGFADDRESS AddrPrev = pThis->PsLoadedModuleListAddr;
    Addr                 = pThis->KernelMteAddr;
    do
    {
        /* Read the validate the MTE. */
        NTMTE Mte;
        rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &Addr, &Mte, pThis->f32Bit ? sizeof(Mte.vX_32) : sizeof(Mte.vX_64));
        if (RT_FAILURE(rc))
            break;
        if (WINNT_UNION(pThis, &Mte, InLoadOrderLinks.Blink) != AddrPrev.FlatPtr)
        {
            Log(("DigWinNt: Bad Mte At %RGv - backpointer\n", Addr.FlatPtr));
            break;
        }
        if (!WINNT_VALID_ADDRESS(pThis, WINNT_UNION(pThis, &Mte, InLoadOrderLinks.Flink)) )
        {
            Log(("DigWinNt: Bad Mte at %RGv - forward pointer\n", Addr.FlatPtr));
            break;
        }
        if (!WINNT_VALID_ADDRESS(pThis, WINNT_UNION(pThis, &Mte, BaseDllName.Buffer)))
        {
            Log(("DigWinNt: Bad Mte at %RGv - BaseDllName=%llx\n", Addr.FlatPtr, WINNT_UNION(pThis, &Mte, BaseDllName.Buffer)));
            break;
        }
        if (!WINNT_VALID_ADDRESS(pThis, WINNT_UNION(pThis, &Mte, FullDllName.Buffer)))
        {
            Log(("DigWinNt: Bad Mte at %RGv - FullDllName=%llx\n", Addr.FlatPtr, WINNT_UNION(pThis, &Mte, FullDllName.Buffer)));
            break;
        }
        if (    !WINNT_VALID_ADDRESS(pThis, WINNT_UNION(pThis, &Mte, DllBase))
            ||  WINNT_UNION(pThis, &Mte, SizeOfImage) > _1M*256
            ||  WINNT_UNION(pThis, &Mte, EntryPoint) - WINNT_UNION(pThis, &Mte, DllBase) > WINNT_UNION(pThis, &Mte, SizeOfImage) )
        {
            Log(("DigWinNt: Bad Mte at %RGv - EntryPoint=%llx SizeOfImage=%x DllBase=%llx\n",
                 Addr.FlatPtr, WINNT_UNION(pThis, &Mte, EntryPoint), WINNT_UNION(pThis, &Mte, SizeOfImage), WINNT_UNION(pThis, &Mte, DllBase)));
            break;
        }

        /* Read the full name. */
        DBGFADDRESS AddrName;
        DBGFR3AddrFromFlat(pUVM, &AddrName, WINNT_UNION(pThis, &Mte, FullDllName.Buffer));
        uint16_t    cbName = WINNT_UNION(pThis, &Mte, FullDllName.Length);
        if (cbName < sizeof(u))
            rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &AddrName, &u, cbName);
        else
            rc = VERR_OUT_OF_RANGE;
        if (RT_FAILURE(rc))
        {
            DBGFR3AddrFromFlat(pUVM, &AddrName, WINNT_UNION(pThis, &Mte, BaseDllName.Buffer));
            cbName = WINNT_UNION(pThis, &Mte, BaseDllName.Length);
            if (cbName < sizeof(u))
                rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &AddrName, &u, cbName);
            else
                rc = VERR_OUT_OF_RANGE;
        }
        if (RT_SUCCESS(rc))
        {
            u.wsz[cbName/2] = '\0';
            char *pszName;
            rc = RTUtf16ToUtf8(u.wsz, &pszName);
            if (RT_SUCCESS(rc))
            {
                /* Read the start of the PE image and pass it along to a worker. */
                DBGFADDRESS ImageAddr;
                DBGFR3AddrFromFlat(pUVM, &ImageAddr, WINNT_UNION(pThis, &Mte, DllBase));
                uint32_t    cbImageBuf = RT_MIN(sizeof(u), WINNT_UNION(pThis, &Mte, SizeOfImage));
                rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &ImageAddr, &u, cbImageBuf);
                if (RT_SUCCESS(rc))
                    dbgDiggerWinNtProcessImage(pThis,
                                               pUVM,
                                               pszName,
                                               &ImageAddr,
                                               WINNT_UNION(pThis, &Mte, SizeOfImage),
                                               &u.au8[0],
                                               sizeof(u));
                RTStrFree(pszName);
            }
        }

        /* next */
        AddrPrev = Addr;
        DBGFR3AddrFromFlat(pUVM, &Addr, WINNT_UNION(pThis, &Mte, InLoadOrderLinks.Flink));
    } while (   Addr.FlatPtr != pThis->KernelMteAddr.FlatPtr
             && Addr.FlatPtr != pThis->PsLoadedModuleListAddr.FlatPtr);

    pThis->fValid = true;
    return VINF_SUCCESS;
}


/**
 * @copydoc DBGFOSREG::pfnProbe
 */
static DECLCALLBACK(bool)  dbgDiggerWinNtProbe(PUVM pUVM, void *pvData)
{
    PDBGDIGGERWINNT pThis = (PDBGDIGGERWINNT)pvData;
    DBGFADDRESS     Addr;
    union
    {
        uint8_t             au8[8192];
        uint16_t            au16[8192/2];
        uint32_t            au32[8192/4];
        IMAGE_DOS_HEADER    MzHdr;
        RTUTF16             wsz[8192/2];
    } u;

    union
    {
        NTMTE32 v32;
        NTMTE64 v64;
    } uMte, uMte2, uMte3;

    /*
     * Look for the PAGELK section name that seems to be a part of all kernels.
     * Then try find the module table entry for it.  Since it's the first entry
     * in the PsLoadedModuleList we can easily validate the list head and report
     * success.
     */
    CPUMMODE        enmMode = DBGFR3CpuGetMode(pUVM, 0 /*idCpu*/);
    uint64_t const  uStart  = enmMode == CPUMMODE_LONG ? UINT64_C(0xffff080000000000) : UINT32_C(0x80001000);
    uint64_t const  uEnd    = enmMode == CPUMMODE_LONG ? UINT64_C(0xffffffffffff0000) : UINT32_C(0xffff0000);
    DBGFADDRESS     KernelAddr;
    for (DBGFR3AddrFromFlat(pUVM, &KernelAddr, uStart);
         KernelAddr.FlatPtr < uEnd;
         KernelAddr.FlatPtr += PAGE_SIZE)
    {
        int rc = DBGFR3MemScan(pUVM, 0 /*idCpu*/, &KernelAddr, uEnd - KernelAddr.FlatPtr,
                               1, "PAGELK\0", sizeof("PAGELK\0"), &KernelAddr);
        if (RT_FAILURE(rc))
            break;
        DBGFR3AddrSub(&KernelAddr, KernelAddr.FlatPtr & PAGE_OFFSET_MASK);

        /* MZ + PE header. */
        rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &KernelAddr, &u, sizeof(u));
        if (    RT_SUCCESS(rc)
            &&  u.MzHdr.e_magic == IMAGE_DOS_SIGNATURE
            &&  !(u.MzHdr.e_lfanew & 0x7)
            &&  u.MzHdr.e_lfanew >= 0x080
            &&  u.MzHdr.e_lfanew <= 0x400) /* W8 is at 0x288*/
        {
            if (enmMode != CPUMMODE_LONG)
            {
                IMAGE_NT_HEADERS32 const *pHdrs = (IMAGE_NT_HEADERS32 const *)&u.au8[u.MzHdr.e_lfanew];
                if (    pHdrs->Signature                            == IMAGE_NT_SIGNATURE
                    &&  pHdrs->FileHeader.Machine                   == IMAGE_FILE_MACHINE_I386
                    &&  pHdrs->FileHeader.SizeOfOptionalHeader      == sizeof(pHdrs->OptionalHeader)
                    &&  pHdrs->FileHeader.NumberOfSections          >= 10 /* the kernel has lots */
                    &&  (pHdrs->FileHeader.Characteristics & (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL)) == IMAGE_FILE_EXECUTABLE_IMAGE
                    &&  pHdrs->OptionalHeader.Magic                 == IMAGE_NT_OPTIONAL_HDR32_MAGIC
                    &&  pHdrs->OptionalHeader.NumberOfRvaAndSizes   == IMAGE_NUMBEROF_DIRECTORY_ENTRIES
                    )
                {
                    /* Find the MTE. */
                    RT_ZERO(uMte);
                    uMte.v32.DllBase     = KernelAddr.FlatPtr;
                    uMte.v32.EntryPoint  = KernelAddr.FlatPtr + pHdrs->OptionalHeader.AddressOfEntryPoint;
                    uMte.v32.SizeOfImage = pHdrs->OptionalHeader.SizeOfImage;
                    DBGFADDRESS HitAddr;
                    rc = DBGFR3MemScan(pUVM, 0 /*idCpu*/, &KernelAddr, uEnd - KernelAddr.FlatPtr,
                                       4 /*align*/, &uMte.v32.DllBase, 3 * sizeof(uint32_t), &HitAddr);
                    while (RT_SUCCESS(rc))
                    {
                        /* check the name. */
                        DBGFADDRESS MteAddr = HitAddr;
                        rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, DBGFR3AddrSub(&MteAddr, RT_OFFSETOF(NTMTE32, DllBase)),
                                           &uMte2.v32, sizeof(uMte2.v32));
                        if (    RT_SUCCESS(rc)
                            &&  uMte2.v32.DllBase     == uMte.v32.DllBase
                            &&  uMte2.v32.EntryPoint  == uMte.v32.EntryPoint
                            &&  uMte2.v32.SizeOfImage == uMte.v32.SizeOfImage
                            &&  WINNT32_VALID_ADDRESS(uMte2.v32.InLoadOrderLinks.Flink)
                            &&  WINNT32_VALID_ADDRESS(uMte2.v32.BaseDllName.Buffer)
                            &&  WINNT32_VALID_ADDRESS(uMte2.v32.FullDllName.Buffer)
                            &&  uMte2.v32.BaseDllName.Length <= 128
                            &&  uMte2.v32.FullDllName.Length <= 260
                           )
                        {
                            rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, DBGFR3AddrFromFlat(pUVM, &Addr, uMte2.v32.BaseDllName.Buffer),
                                               u.wsz, uMte2.v32.BaseDllName.Length);
                            u.wsz[uMte2.v32.BaseDllName.Length / 2] = '\0';
                            if (    RT_SUCCESS(rc)
                                &&  (   !RTUtf16ICmp(u.wsz, g_wszKernelNames[0])
                                  /* || !RTUtf16ICmp(u.wsz, g_wszKernelNames[1]) */
                                    )
                               )
                            {
                                rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/,
                                                   DBGFR3AddrFromFlat(pUVM, &Addr, uMte2.v32.InLoadOrderLinks.Blink),
                                                   &uMte3.v32, RT_SIZEOFMEMB(NTMTE32, InLoadOrderLinks));
                                if (   RT_SUCCESS(rc)
                                    && uMte3.v32.InLoadOrderLinks.Flink == MteAddr.FlatPtr
                                    && WINNT32_VALID_ADDRESS(uMte3.v32.InLoadOrderLinks.Blink) )
                                {
                                    Log(("DigWinNt: MteAddr=%RGv KernelAddr=%RGv SizeOfImage=%x &PsLoadedModuleList=%RGv (32-bit)\n",
                                         MteAddr.FlatPtr, KernelAddr.FlatPtr, uMte2.v32.SizeOfImage, Addr.FlatPtr));
                                    pThis->KernelAddr               = KernelAddr;
                                    pThis->KernelMteAddr            = MteAddr;
                                    pThis->PsLoadedModuleListAddr   = Addr;
                                    pThis->f32Bit                   = true;
                                    return true;
                                }
                            }
                            else if (RT_SUCCESS(rc))
                            {
                                Log2(("DigWinNt: Wrong module: MteAddr=%RGv ImageAddr=%RGv SizeOfImage=%#x '%ls'\n",
                                      MteAddr.FlatPtr, KernelAddr.FlatPtr, uMte2.v32.SizeOfImage, u.wsz));
                                break; /* Not NT kernel */
                            }
                        }

                        /* next */
                        DBGFR3AddrAdd(&HitAddr, 4);
                        if (HitAddr.FlatPtr < uEnd)
                            rc = DBGFR3MemScan(pUVM, 0 /*idCpu*/, &HitAddr, uEnd - HitAddr.FlatPtr,
                                               4 /*align*/, &uMte.v32.DllBase, 3 * sizeof(uint32_t), &HitAddr);
                        else
                            rc = VERR_DBGF_MEM_NOT_FOUND;
                    }
                }
            }
            else
            {
                IMAGE_NT_HEADERS64 const *pHdrs = (IMAGE_NT_HEADERS64 const *)&u.au8[u.MzHdr.e_lfanew];
                if (    pHdrs->Signature                            == IMAGE_NT_SIGNATURE
                    &&  pHdrs->FileHeader.Machine                   == IMAGE_FILE_MACHINE_AMD64
                    &&  pHdrs->FileHeader.SizeOfOptionalHeader      == sizeof(pHdrs->OptionalHeader)
                    &&  pHdrs->FileHeader.NumberOfSections          >= 10 /* the kernel has lots */
                    &&      (pHdrs->FileHeader.Characteristics & (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL))
                         == IMAGE_FILE_EXECUTABLE_IMAGE
                    &&  pHdrs->OptionalHeader.Magic                 == IMAGE_NT_OPTIONAL_HDR64_MAGIC
                    &&  pHdrs->OptionalHeader.NumberOfRvaAndSizes   == IMAGE_NUMBEROF_DIRECTORY_ENTRIES
                    )
                {
                    /* Find the MTE. */
                    RT_ZERO(uMte.v64);
                    uMte.v64.DllBase     = KernelAddr.FlatPtr;
                    uMte.v64.EntryPoint  = KernelAddr.FlatPtr + pHdrs->OptionalHeader.AddressOfEntryPoint;
                    uMte.v64.SizeOfImage = pHdrs->OptionalHeader.SizeOfImage;
                    DBGFADDRESS ScanAddr;
                    DBGFADDRESS HitAddr;
                    rc = DBGFR3MemScan(pUVM, 0 /*idCpu*/, DBGFR3AddrFromFlat(pUVM, &ScanAddr, uStart),
                                       uEnd - uStart, 8 /*align*/, &uMte.v64.DllBase, 5 * sizeof(uint32_t), &HitAddr);
                    while (RT_SUCCESS(rc))
                    {
                        /* Read the start of the MTE and check some basic members. */
                        DBGFADDRESS MteAddr = HitAddr;
                        rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, DBGFR3AddrSub(&MteAddr, RT_OFFSETOF(NTMTE64, DllBase)),
                                           &uMte2.v64, sizeof(uMte2.v64));
                        if (    RT_SUCCESS(rc)
                            &&  uMte2.v64.DllBase     == uMte.v64.DllBase
                            &&  uMte2.v64.EntryPoint  == uMte.v64.EntryPoint
                            &&  uMte2.v64.SizeOfImage == uMte.v64.SizeOfImage
                            &&  WINNT64_VALID_ADDRESS(uMte2.v64.InLoadOrderLinks.Flink)
                            &&  WINNT64_VALID_ADDRESS(uMte2.v64.BaseDllName.Buffer)
                            &&  WINNT64_VALID_ADDRESS(uMte2.v64.FullDllName.Buffer)
                            &&  uMte2.v64.BaseDllName.Length <= 128
                            &&  uMte2.v64.FullDllName.Length <= 260
                            )
                        {
                            /* Try read the base name and compare with known NT kernel names. */
                            rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, DBGFR3AddrFromFlat(pUVM, &Addr, uMte2.v64.BaseDllName.Buffer),
                                               u.wsz, uMte2.v64.BaseDllName.Length);
                            u.wsz[uMte2.v64.BaseDllName.Length / 2] = '\0';
                            if (    RT_SUCCESS(rc)
                                &&  (   !RTUtf16ICmp(u.wsz, g_wszKernelNames[0])
                                  /* || !RTUtf16ICmp(u.wsz, g_wszKernelNames[1]) */
                                    )
                               )
                            {
                                /* Read the link entry of the previous entry in the list and check that its
                                   forward pointer points at the MTE we've found. */
                                rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/,
                                                   DBGFR3AddrFromFlat(pUVM, &Addr, uMte2.v64.InLoadOrderLinks.Blink),
                                                   &uMte3.v64, RT_SIZEOFMEMB(NTMTE64, InLoadOrderLinks));
                                if (   RT_SUCCESS(rc)
                                    && uMte3.v64.InLoadOrderLinks.Flink == MteAddr.FlatPtr
                                    && WINNT64_VALID_ADDRESS(uMte3.v64.InLoadOrderLinks.Blink) )
                                {
                                    Log(("DigWinNt: MteAddr=%RGv KernelAddr=%RGv SizeOfImage=%x &PsLoadedModuleList=%RGv (32-bit)\n",
                                         MteAddr.FlatPtr, KernelAddr.FlatPtr, uMte2.v64.SizeOfImage, Addr.FlatPtr));
                                    pThis->KernelAddr               = KernelAddr;
                                    pThis->KernelMteAddr            = MteAddr;
                                    pThis->PsLoadedModuleListAddr   = Addr;
                                    pThis->f32Bit                   = false;
                                    return true;
                                }
                            }
                            else if (RT_SUCCESS(rc))
                            {
                                Log2(("DigWinNt: Wrong module: MteAddr=%RGv ImageAddr=%RGv SizeOfImage=%#x '%ls'\n",
                                      MteAddr.FlatPtr, KernelAddr.FlatPtr, uMte2.v64.SizeOfImage, u.wsz));
                                break; /* Not NT kernel */
                            }
                        }

                        /* next */
                        DBGFR3AddrAdd(&HitAddr, 8);
                        if (HitAddr.FlatPtr < uEnd)
                            rc = DBGFR3MemScan(pUVM, 0 /*idCpu*/, &HitAddr, uEnd - HitAddr.FlatPtr,
                                               8 /*align*/, &uMte.v64.DllBase, 3 * sizeof(uint32_t), &HitAddr);
                        else
                            rc = VERR_DBGF_MEM_NOT_FOUND;
                    }
                }
            }
        }
    }
    return false;
}


/**
 * @copydoc DBGFOSREG::pfnDestruct
 */
static DECLCALLBACK(void)  dbgDiggerWinNtDestruct(PUVM pUVM, void *pvData)
{
    RT_NOREF2(pUVM, pvData);
}


/**
 * @copydoc DBGFOSREG::pfnConstruct
 */
static DECLCALLBACK(int)  dbgDiggerWinNtConstruct(PUVM pUVM, void *pvData)
{
    RT_NOREF1(pUVM);
    PDBGDIGGERWINNT pThis = (PDBGDIGGERWINNT)pvData;
    pThis->fValid = false;
    pThis->f32Bit = false;
    pThis->enmVer = DBGDIGGERWINNTVER_UNKNOWN;
    return VINF_SUCCESS;
}


const DBGFOSREG g_DBGDiggerWinNt =
{
    /* .u32Magic = */           DBGFOSREG_MAGIC,
    /* .fFlags = */             0,
    /* .cbData = */             sizeof(DBGDIGGERWINNT),
    /* .szName = */             "WinNT",
    /* .pfnConstruct = */       dbgDiggerWinNtConstruct,
    /* .pfnDestruct = */        dbgDiggerWinNtDestruct,
    /* .pfnProbe = */           dbgDiggerWinNtProbe,
    /* .pfnInit = */            dbgDiggerWinNtInit,
    /* .pfnRefresh = */         dbgDiggerWinNtRefresh,
    /* .pfnTerm = */            dbgDiggerWinNtTerm,
    /* .pfnQueryVersion = */    dbgDiggerWinNtQueryVersion,
    /* .pfnQueryInterface = */  dbgDiggerWinNtQueryInterface,
    /* .u32EndMagic = */        DBGFOSREG_MAGIC
};


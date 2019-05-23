/* $Id: DBGPlugInOS2.cpp $ */
/** @file
 * DBGPlugInOS2 - Debugger and Guest OS Digger Plugin For OS/2.
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
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/stream.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/** @name Internal OS/2 structures */

/** @} */


typedef enum DBGDIGGEROS2VER
{
    DBGDIGGEROS2VER_UNKNOWN,
    DBGDIGGEROS2VER_1_x,
    DBGDIGGEROS2VER_2_x,
    DBGDIGGEROS2VER_3_0,
    DBGDIGGEROS2VER_4_0,
    DBGDIGGEROS2VER_4_5
} DBGDIGGEROS2VER;

/**
 * OS/2 guest OS digger instance data.
 */
typedef struct DBGDIGGEROS2
{
    /** Whether the information is valid or not.
     * (For fending off illegal interface method calls.) */
    bool                fValid;
    /** 32-bit (true) or 16-bit (false) */
    bool                f32Bit;

    /** The OS/2 guest version. */
    DBGDIGGEROS2VER     enmVer;
    uint8_t             OS2MajorVersion;
    uint8_t             OS2MinorVersion;

    /** Guest's Global Info Segment selector. */
    uint16_t            selGIS;

} DBGDIGGEROS2;
/** Pointer to the OS/2 guest OS digger instance data. */
typedef DBGDIGGEROS2 *PDBGDIGGEROS2;


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The 'SAS ' signature. */
#define DIG_OS2_SAS_SIG     RT_MAKE_U32_FROM_U8('S','A','S',' ')

/** OS/2Warp on little endian ASCII systems. */
#define DIG_OS2_MOD_TAG     UINT64_C(0x43532f3257617270)


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int)  dbgDiggerOS2Init(PUVM pUVM, void *pvData);



#if 0 /* unused */
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
static void dbgDiggerOS2ProcessImage(PDBGDIGGEROS2 pThis, PUVM pUVM, const char *pszName,
                                     PCDBGFADDRESS pImageAddr, uint32_t cbImage,
                                     uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF7(pThis, pUVM, pszName, pImageAddr, cbImage, pbBuf, cbBuf);
    LogFlow(("DigOS2: %RGp %#x %s\n", pImageAddr->FlatPtr, cbImage, pszName));

    /* To be implemented.*/
}
#endif


/**
 * @copydoc DBGFOSREG::pfnQueryInterface
 */
static DECLCALLBACK(void *) dbgDiggerOS2QueryInterface(PUVM pUVM, void *pvData, DBGFOSINTERFACE enmIf)
{
    RT_NOREF3(pUVM, pvData, enmIf);
    return NULL;
}


/**
 * @copydoc DBGFOSREG::pfnQueryVersion
 */
static DECLCALLBACK(int)  dbgDiggerOS2QueryVersion(PUVM pUVM, void *pvData, char *pszVersion, size_t cchVersion)
{
    RT_NOREF1(pUVM);
    PDBGDIGGEROS2 pThis = (PDBGDIGGEROS2)pvData;
    Assert(pThis->fValid);
    char *achOS2ProductType[32];
    char *pszOS2ProductType = (char *)achOS2ProductType;

    if (pThis->OS2MajorVersion == 10)
    {
        RTStrPrintf(pszOS2ProductType, sizeof(achOS2ProductType), "OS/2 1.%02d", pThis->OS2MinorVersion);
        pThis->enmVer = DBGDIGGEROS2VER_1_x;
    }
    else if (pThis->OS2MajorVersion == 20)
    {
        if (pThis->OS2MinorVersion < 30)
        {
            RTStrPrintf(pszOS2ProductType, sizeof(achOS2ProductType), "OS/2 2.%02d", pThis->OS2MinorVersion);
            pThis->enmVer = DBGDIGGEROS2VER_2_x;
        }
        else if (pThis->OS2MinorVersion < 40)
        {
            RTStrPrintf(pszOS2ProductType, sizeof(achOS2ProductType), "OS/2 Warp");
            pThis->enmVer = DBGDIGGEROS2VER_3_0;
        }
        else if (pThis->OS2MinorVersion == 40)
        {
            RTStrPrintf(pszOS2ProductType, sizeof(achOS2ProductType), "OS/2 Warp 4");
            pThis->enmVer = DBGDIGGEROS2VER_4_0;
        }
        else
        {
            RTStrPrintf(pszOS2ProductType, sizeof(achOS2ProductType), "OS/2 Warp %d.%d",
                        pThis->OS2MinorVersion / 10, pThis->OS2MinorVersion % 10);
            pThis->enmVer = DBGDIGGEROS2VER_4_5;
        }
    }
    RTStrPrintf(pszVersion, cchVersion, "%u.%u (%s)", pThis->OS2MajorVersion, pThis->OS2MinorVersion, pszOS2ProductType);
    return VINF_SUCCESS;
}


/**
 * @copydoc DBGFOSREG::pfnTerm
 */
static DECLCALLBACK(void)  dbgDiggerOS2Term(PUVM pUVM, void *pvData)
{
    RT_NOREF1(pUVM);
    PDBGDIGGEROS2 pThis = (PDBGDIGGEROS2)pvData;
    Assert(pThis->fValid);

    pThis->fValid = false;
}


/**
 * @copydoc DBGFOSREG::pfnRefresh
 */
static DECLCALLBACK(int)  dbgDiggerOS2Refresh(PUVM pUVM, void *pvData)
{
    PDBGDIGGEROS2 pThis = (PDBGDIGGEROS2)pvData;
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
                if (RTDbgModGetTag(hMod) == DIG_OS2_MOD_TAG)
                {
                    int rc = RTDbgAsModuleUnlink(hDbgAs, hMod);
                    AssertRC(rc);
                }
                RTDbgModRelease(hMod);
            }
        }
        RTDbgAsRelease(hDbgAs);
    }

    dbgDiggerOS2Term(pUVM, pvData);
    return dbgDiggerOS2Init(pUVM, pvData);
}


/**
 * @copydoc DBGFOSREG::pfnInit
 */
static DECLCALLBACK(int)  dbgDiggerOS2Init(PUVM pUVM, void *pvData)
{
    PDBGDIGGEROS2 pThis = (PDBGDIGGEROS2)pvData;
    Assert(!pThis->fValid);

    union
    {
        uint8_t             au8[0x2000];
        uint16_t            au16[0x2000/2];
        uint32_t            au32[0x2000/4];
        RTUTF16             wsz[0x2000/2];
    }               u;
    DBGFADDRESS     Addr;
    int             rc;

    /*
     * Determine the OS/2 version.
     */
    do {
        /* Version info is at GIS:15h (major/minor/revision). */
        rc = DBGFR3AddrFromSelOff(pUVM, 0 /*idCpu*/, &Addr, pThis->selGIS, 0x15);
        if (RT_FAILURE(rc))
            break;
        rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &Addr, u.au32, sizeof(uint32_t));
        if (RT_FAILURE(rc))
            break;

        pThis->OS2MajorVersion = u.au8[0];
        pThis->OS2MinorVersion = u.au8[1];

        pThis->fValid = true;
        return VINF_SUCCESS;
    } while (0);
    return VERR_NOT_SUPPORTED;
}


/**
 * @copydoc DBGFOSREG::pfnProbe
 */
static DECLCALLBACK(bool)  dbgDiggerOS2Probe(PUVM pUVM, void *pvData)
{
    PDBGDIGGEROS2   pThis = (PDBGDIGGEROS2)pvData;
    DBGFADDRESS     Addr;
    int             rc;
    uint16_t        offInfo;
    union
    {
        uint8_t             au8[8192];
        uint16_t            au16[8192/2];
        uint32_t            au32[8192/4];
        RTUTF16             wsz[8192/2];
    } u;

    /*
     * If the DWORD at 70:0 contains 'SAS ' it's quite unlikely that this wouldn't be OS/2.
     * Note: The SAS layout is similar between 16-bit and 32-bit OS/2, but not identical.
     * 32-bit OS/2 will have the flat kernel data selector at SAS:06. The selector is 168h
     * or similar. For 16-bit OS/2 the field contains a table offset into the SAS which will
     * be much smaller. Fun fact: The global infoseg selector in the SAS is bimodal in 16-bit
     * OS/2 and will work in real mode as well.
     */
    do {
        rc = DBGFR3AddrFromSelOff(pUVM, 0 /*idCpu*/, &Addr, 0x70, 0x00);
        if (RT_FAILURE(rc))
            break;
        rc = DBGFR3MemRead(pUVM, 0 /*idCpu*/, &Addr, u.au32, 256);
        if (RT_FAILURE(rc))
            break;
        if (u.au32[0] != DIG_OS2_SAS_SIG)
            break;

        /* This sure looks like OS/2, but a bit of paranoia won't hurt. */
        if (u.au16[2] >= u.au16[4])
            break;

        /* If 4th word is bigger than 5th, it's the flat kernel mode selector. */
        if (u.au16[3] > u.au16[4])
            pThis->f32Bit = true;

        /* Offset into info table is either at SAS:14h or SAS:16h. */
        if (pThis->f32Bit)
            offInfo = u.au16[0x14/2];
        else
            offInfo = u.au16[0x16/2];

        /* The global infoseg selector is the first entry in the info table. */
        pThis->selGIS = u.au16[offInfo/2];
        return true;
    } while (0);

    return false;
}


/**
 * @copydoc DBGFOSREG::pfnDestruct
 */
static DECLCALLBACK(void)  dbgDiggerOS2Destruct(PUVM pUVM, void *pvData)
{
    RT_NOREF2(pUVM, pvData);
}


/**
 * @copydoc DBGFOSREG::pfnConstruct
 */
static DECLCALLBACK(int)  dbgDiggerOS2Construct(PUVM pUVM, void *pvData)
{
    RT_NOREF1(pUVM);
    PDBGDIGGEROS2 pThis = (PDBGDIGGEROS2)pvData;
    pThis->fValid = false;
    pThis->f32Bit = false;
    pThis->enmVer = DBGDIGGEROS2VER_UNKNOWN;
    return VINF_SUCCESS;
}


const DBGFOSREG g_DBGDiggerOS2 =
{
    /* .u32Magic = */           DBGFOSREG_MAGIC,
    /* .fFlags = */             0,
    /* .cbData = */             sizeof(DBGDIGGEROS2),
    /* .szName = */             "OS/2",
    /* .pfnConstruct = */       dbgDiggerOS2Construct,
    /* .pfnDestruct = */        dbgDiggerOS2Destruct,
    /* .pfnProbe = */           dbgDiggerOS2Probe,
    /* .pfnInit = */            dbgDiggerOS2Init,
    /* .pfnRefresh = */         dbgDiggerOS2Refresh,
    /* .pfnTerm = */            dbgDiggerOS2Term,
    /* .pfnQueryVersion = */    dbgDiggerOS2QueryVersion,
    /* .pfnQueryInterface = */  dbgDiggerOS2QueryInterface,
    /* .u32EndMagic = */        DBGFOSREG_MAGIC
};

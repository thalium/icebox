/* $Id: DevFlash.cpp $ */
/** @file
 * DevFlash - A simple Flash device
 *
 * A simple non-volatile byte-wide (x8) memory device modeled after Intel 28F008
 * FlashFile. See 28F008SA datasheet, Intel order number 290429-007.
 *
 * Implemented as an MMIO device attached directly to the CPU, not behind any
 * bus. Typically mapped as part of the firmware image.
 */

/*
 * Copyright (C) 2018 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_FLASH
#include <VBox/vmm/pdmdev.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/file.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The current version of the saved state. */
#define FLASH_SAVED_STATE_VERSION           1


/** @name CUI (Command User Interface) Commands.
 *  @{ */
#define FLASH_CMD_ALT_WRITE             0x10
#define FLASH_CMD_ERASE_SETUP           0x20
#define FLASH_CMD_WRITE                 0x40
#define FLASH_CMD_STS_CLEAR             0x50
#define FLASH_CMD_STS_READ              0x70
#define FLASH_CMD_READ_ID               0x90
#define FLASH_CMD_ERASE_SUS_RES         0xB0
#define FLASH_CMD_ERASE_CONFIRM         0xD0
#define FLASH_CMD_ARRAY_READ            0xFF
/** @} */

/** @name Status register bits.
 *  @{ */
#define FLASH_STATUS_WSMS               0x80    /* Write State Machine Status, 1=Ready */
#define FLASH_STATUS_ESS                0x40    /* Erase Suspend Status, 1=Suspended */
#define FLASH_STATUS_ES                 0x20    /* Erase Status, 1=Error */
#define FLASH_STATUS_BWS                0x10    /* Byte Write Status, 1=Error */
#define FLASH_STATUS_VPPS               0x08    /* Vpp Status, 1=Low Vpp */
/* The remaining bits 0-2 are reserved/unused */
/** @} */


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/


/**
 * The flash device
 */
typedef struct DEVFLASH
{
    /** The current command. */
    uint8_t             bCmd;
    /** The status register. */
    uint8_t             bStatus;
    /** Current bus cycle. */
    uint8_t             cBusCycle;

    uint8_t             uPadding0;

    /* The following state does not change at runtime.*/
    /** Manufacturer (high byte) and device (low byte) ID. */
    uint16_t            u16FlashId;
    /** The configured block size of the device. */
    uint16_t            cbBlockSize;
    /** The guest physical memory base address. */
    RTGCPHYS            GCPhysFlashBase;
    /** The flash memory region size.  */
    uint32_t            cbFlashSize;
    /** The actual flash memory data.  */
    uint8_t             *pbFlash;
    /** When set, indicates the state was saved. */
    bool                fStateSaved;
    /** The backing file. */
    RTFILE              hFlashFile;
    char                *pszFlashFile;
} DEVFLASH;

/** Pointer to the Flash device state. */
typedef DEVFLASH *PDEVFLASH;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

#ifdef IN_RING3 /* for now */

static int flashMemWriteByte(PDEVFLASH pThis, RTGCPHYS GCPhysAddr, uint8_t bCmd)
{
    int rc = VINF_SUCCESS;
    unsigned uOffset;

    /* NB: Older datasheets (e.g. 28F008SA) suggest that for two-cycle commands like byte write or
     * erase setup, the address is significant in both cycles, but do not explain what happens
     * should the addresses not match. Newer datasheets (e.g. 28F008B3) clearly say that the address
     * in the first byte cycle never matters. We prefer the latter interpretation.
     */

    if (pThis->cBusCycle == 0)
    {
        /* First bus write cycle, start processing a new command. Address is ignored. */
        switch (bCmd)
        {
        case FLASH_CMD_ARRAY_READ:
        case FLASH_CMD_STS_READ:
        case FLASH_CMD_ERASE_SUS_RES:
        case FLASH_CMD_READ_ID:
            /* Single-cycle write commands, only change the current command. */
            pThis->bCmd = bCmd;
            break;
        case FLASH_CMD_STS_CLEAR:
            /* Status clear continues in read mode. */
            pThis->bStatus = 0;
            pThis->bCmd = FLASH_CMD_ARRAY_READ;
            break;
        case FLASH_CMD_WRITE:
        case FLASH_CMD_ALT_WRITE:
        case FLASH_CMD_ERASE_SETUP:
            /* Two-cycle commands, advance the bus write cycle. */
            pThis->bCmd = bCmd;
            pThis->cBusCycle++;
            break;
        default:
            LogFunc(("1st cycle command %02X, current cmd %02X\n", bCmd, pThis->bCmd));
            break;
        }
    }
    else
    {
        /* Second write of a two-cycle command. */
        Assert(pThis->cBusCycle == 1);
        switch (pThis->bCmd)
        {
        case FLASH_CMD_WRITE:
        case FLASH_CMD_ALT_WRITE:
            uOffset = GCPhysAddr & (pThis->cbFlashSize - 1);
            if (uOffset < pThis->cbFlashSize)
            {
                pThis->pbFlash[uOffset] = bCmd;
                /* NB: Writes are instant and never fail. */
                LogFunc(("wrote byte to flash at %08RGp: %02X\n", GCPhysAddr, bCmd));
            }
            else
                LogFunc(("ignoring write at %08RGp: %02X\n", GCPhysAddr, bCmd));
            break;
        case FLASH_CMD_ERASE_SETUP:
            if (bCmd == FLASH_CMD_ERASE_CONFIRM)
            {
                /* The current address determines the block to erase. */
                uOffset = GCPhysAddr & (pThis->cbFlashSize - 1);
                uOffset = uOffset & ~(pThis->cbBlockSize - 1);
                memset(pThis->pbFlash + uOffset, 0xff, pThis->cbBlockSize);
                LogFunc(("Erasing block at offset %u\n", uOffset));
            }
            else
            {
                /* Anything else is a command erorr. Transition to status read mode. */
                LogFunc(("2st cycle erase command is %02X, should be confirm (%02X)\n", bCmd, FLASH_CMD_ERASE_CONFIRM));
                pThis->bCmd = FLASH_CMD_STS_READ;
                pThis->bStatus |= FLASH_STATUS_BWS | FLASH_STATUS_ES;
            }
            break;
        default:
            LogFunc(("2st cycle bad command %02X, current cmd %02X\n", bCmd, pThis->bCmd));
            break;
        }
        pThis->cBusCycle = 0;
    }
    LogFlow(("flashMemWriteByte: write access at %08RGp: %#x rc=%Rrc\n", GCPhysAddr, bCmd, rc));
//LogRel(("flashMemWriteByte: write access at %08RGp: %#x (cmd=%02X) rc=%Rrc\n", GCPhysAddr, bCmd, pThis->bCmd, rc));
    return rc;
}


static int flashMemReadByte(PDEVFLASH pThis, RTGCPHYS GCPhysAddr, uint8_t *pbData)
{
    uint8_t bValue;
    unsigned uOffset;
    int rc = VINF_SUCCESS;

    /* Reads are only defined in three states: Array read, status register read,
     * and ID read.
     */
    switch (pThis->bCmd)
    {
    case FLASH_CMD_ARRAY_READ:
        uOffset = GCPhysAddr & (pThis->cbFlashSize - 1);
        bValue = pThis->pbFlash[uOffset];
        LogFunc(("read byte at %08RGp: %02X\n", GCPhysAddr, bValue));
        break;
    case FLASH_CMD_STS_READ:
        bValue = pThis->bStatus;
        break;
    case FLASH_CMD_READ_ID:
        bValue = GCPhysAddr & 1 ?  RT_HI_U8(pThis->u16FlashId) : RT_LO_U8(pThis->u16FlashId);
        break;
    default:
        bValue = 0xff;
        break;
    }
    *pbData = bValue;

    LogFlow(("flashMemReadByte: read access at %08RGp: %02X (cmd=%02X) rc=%Rrc\n", GCPhysAddr, bValue, pThis->bCmd, rc));
//LogRel(("flashMemReadByte: read access at %08RGp: %02X (cmd=%02X) rc=%Rrc\n", GCPhysAddr, bValue, pThis->bCmd, rc));
    return rc;
}

/** @callback_method_impl{FNIOMMIWRITE, Flash memory write} */
PDMBOTHCBDECL(int) flashMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
    PDEVFLASH pThis = PDMINS_2_DATA(pDevIns, PDEVFLASH);
    int rc = VINF_SUCCESS;
    const uint8_t *pu8Mem = (const uint8_t *)pv;
    unsigned uOffset;
    RT_NOREF1(pvUser);

    /* Writes may need to go back to R3. If more than one byte is being written (not likely!),
     * just suck it up and take the trip to R3 immediately.
     */
    /** @todo Idea: We could buffer all writes in R0 and flush them out on a
     *        timer. Probably not worth it.
     */
#ifndef IN_RING3
    if (cb > 1)
        return VINF_IOM_R3_IOPORT_WRITE;
#endif

    for (uOffset = 0; uOffset < cb; ++uOffset)
    {
        rc = flashMemWriteByte(pThis, GCPhysAddr + uOffset, pu8Mem[uOffset]);
        if (!RT_SUCCESS(rc))
            break;
    }

    LogFlow(("FlashMMIOWrite: completed write at %08RGp (LB %u): rc=%Rrc\n", GCPhysAddr, cb, rc));
    return rc;
}


/** @callback_method_impl{FNIOMMIOREAD, Flash memory read} */
PDMBOTHCBDECL(int) flashMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    PDEVFLASH pThis = PDMINS_2_DATA(pDevIns, PDEVFLASH);
    int rc = VINF_SUCCESS;
    unsigned uOffset;
    uint8_t *pu8Mem;
    RT_NOREF1(pvUser);

    /* Reading can always be done witout going back to R3. Reads do not
     * change the device state and we always have the data.
     */
    pu8Mem = (uint8_t *)pv;
    for (uOffset = 0; uOffset < cb; ++uOffset, ++pu8Mem)
    {
        rc = flashMemReadByte(pThis, GCPhysAddr + uOffset, pu8Mem);
        if (!RT_SUCCESS(rc))
            break;
    }

    LogFlow(("flashMMIORead: completed read at %08RGp (LB %u): rc=%Rrc\n", GCPhysAddr, cb, rc));
    return rc;
}

#endif /* IN_RING3 for now */

#ifdef IN_RING3

/** @callback_method_impl{FNSSMDEVSAVEEXEC} */
static DECLCALLBACK(int) flashSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PDEVFLASH pThis = PDMINS_2_DATA(pDevIns, PDEVFLASH);

    /* Save the device state. */
    SSMR3PutU8(pSSM, pThis->bCmd);
    SSMR3PutU8(pSSM, pThis->bStatus);
    SSMR3PutU8(pSSM, pThis->cBusCycle);

    /* Save the current configuration for validation purposes. */
    SSMR3PutU16(pSSM, pThis->cbBlockSize);
    SSMR3PutU16(pSSM, pThis->u16FlashId);

    /* Save the current flash contents. */
    SSMR3PutU32(pSSM, pThis->cbFlashSize);
    SSMR3PutMem(pSSM, pThis->pbFlash, pThis->cbFlashSize);

    pThis->fStateSaved = true;

    return VINF_SUCCESS;
}


/** @callback_method_impl{FNSSMDEVLOADEXEC} */
static DECLCALLBACK(int) flashLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PDEVFLASH pThis = PDMINS_2_DATA(pDevIns, PDEVFLASH);
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /* Fend off unsupported versions. */
    if (uVersion != FLASH_SAVED_STATE_VERSION)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;

    /*
     * Do the actual restoring.
     */
    if (uVersion == FLASH_SAVED_STATE_VERSION)
    {
        uint16_t    u16Val;
        uint32_t    u32Val;

        SSMR3GetU8(pSSM, &pThis->bCmd);
        SSMR3GetU8(pSSM, &pThis->bStatus);
        SSMR3GetU8(pSSM, &pThis->cBusCycle);

        /* Make sure configuration didn't change behind our back. */
        SSMR3GetU16(pSSM, &u16Val);
        if (u16Val != pThis->cbBlockSize)
            return VERR_SSM_LOAD_CONFIG_MISMATCH;
        SSMR3GetU16(pSSM, &u16Val);
        if (u16Val != pThis->u16FlashId)
            return VERR_SSM_LOAD_CONFIG_MISMATCH;
        SSMR3GetU32(pSSM, &u32Val);
        if (u16Val != pThis->cbFlashSize)
            return VERR_SSM_LOAD_CONFIG_MISMATCH;

        /* Suck in the flash contents. */
        SSMR3GetMem(pSSM, pThis->pbFlash, pThis->cbFlashSize);
    }

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) flashReset(PPDMDEVINS pDevIns)
{
    PDEVFLASH pThis = PDMINS_2_DATA(pDevIns, PDEVFLASH);

    /*
     * Initialize the device state.
     */
    pThis->bCmd      = FLASH_CMD_ARRAY_READ;
    pThis->bStatus   = 0;
    pThis->cBusCycle = 0;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) flashDestruct(PPDMDEVINS pDevIns)
{
    PDEVFLASH pThis = PDMINS_2_DATA(pDevIns, PDEVFLASH);
    int rc;

    if (!pThis->fStateSaved)
    {
        rc = RTFileSeek(pThis->hFlashFile, 0, RTFILE_SEEK_BEGIN, NULL);
        rc = RTFileWrite(pThis->hFlashFile, pThis->pbFlash, pThis->cbFlashSize, NULL);
        if (RT_FAILURE(rc))
            LogRel(("flash: Failed to save flash file"));
    }

    if (pThis->pbFlash)
    {
        PDMDevHlpMMHeapFree(pDevIns, pThis->pbFlash);
        pThis->pbFlash = NULL;
    }

    if (pThis->pszFlashFile)
    {
        PDMDevHlpMMHeapFree(pDevIns, pThis->pszFlashFile);
        pThis->pszFlashFile = NULL;
    }

    return VINF_SUCCESS;
}

/** @todo this does not really belong here; workaround for EFI failing to init empty flash. */
static const uint8_t aHdrBegin[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x8D, 0x2B, 0xF1, 0xFF, 0x96, 0x76, 0x8B, 0x4C, 0xA9, 0x85, 0x27, 0x47, 0x07, 0x5B, 0x4F, 0x50,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x46, 0x56, 0x48, 0xFF, 0xFE, 0x04, 0x00,
    0x48, 0x00, 0x19, 0xF9, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x36, 0xCF, 0xDD, 0x75, 0x32, 0x64, 0x41,
    0x98, 0xB6, 0xFE, 0x85, 0x70, 0x7F, 0xFE, 0x7D, 0xB8, 0xDF, 0x00, 0x00, 0x5A, 0xFE, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) flashConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF1(iInstance);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    PDEVFLASH pThis = PDMINS_2_DATA(pDevIns, PDEVFLASH);
    Assert(iInstance == 0);

    /*
     * Validate configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, "DeviceId|BaseAddress|Size|BlockSize|FlashFile", "");

    /*
     * Read configuration.
     */

    /* The default device ID is Intel 28F800SA. */
    int rc = CFGMR3QueryU16Def(pCfg, "DeviceId", &pThis->u16FlashId, 0xA289);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"DeviceId\" as an integer failed"));

    /* The default base address is 2MB below 4GB. */
    rc = CFGMR3QueryU64Def(pCfg, "BaseAddress", &pThis->GCPhysFlashBase, 0xFFE00000);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"BaseAddress\" as an integer failed"));

    /* The default flash device size is 128K. */
    rc = CFGMR3QueryU32Def(pCfg, "Size", &pThis->cbFlashSize, 128 * _1K);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"Size\" as an integer failed"));

    /* The default flash device block size is 4K. */
    rc = CFGMR3QueryU16Def(pCfg, "BlockSize", &pThis->cbBlockSize, 4 * _1K);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"BlockSize\" as an integer failed"));

    /* The default flash device block size is 4K. */
    rc = CFGMR3QueryU16Def(pCfg, "BlockSize", &pThis->cbBlockSize, 4 * _1K);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"BlockSize\" as an integer failed"));

    rc = CFGMR3QueryStringAlloc(pCfg, "FlashFile", &pThis->pszFlashFile);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"FlashFile\" as a string failed"));

    /* Try opening the backing file. */
    rc = RTFileOpen(&pThis->hFlashFile, pThis->pszFlashFile, RTFILE_O_READWRITE | RTFILE_O_OPEN_CREATE | RTFILE_O_DENY_WRITE);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to open flash file"));

    /* Set up the static state, immutable at run-time. */
    pThis->pbFlash = (uint8_t *)PDMDevHlpMMHeapAlloc(pDevIns, pThis->cbFlashSize);
    if (!pThis->pbFlash)
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to allocate heap memory"));

    size_t cbRead = 0;
    rc = RTFileRead(pThis->hFlashFile, pThis->pbFlash, pThis->cbFlashSize, &cbRead);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to read flash file"));
    Log(("Read %zu bytes from file (asked for %u)\n.", cbRead, pThis->cbFlashSize));

    /* If the file didn't exist, or someone truncated it, we'll initialize
     * the storage with default contents.
     */
    if (cbRead != pThis->cbFlashSize)
    {
        memset(pThis->pbFlash, 0xff, pThis->cbFlashSize);
        memcpy(pThis->pbFlash, aHdrBegin, sizeof(aHdrBegin));
        LogRel(("Only read %zu bytes from flash file (asked for %u). Initializing with defaults.\n", cbRead, pThis->cbFlashSize));
    }

    /* Reset the dynamic state.*/
    flashReset(pDevIns);

    /*
     * Register MMIO region.
     */
    rc = PDMDevHlpMMIORegister(pDevIns, pThis->GCPhysFlashBase, pThis->cbFlashSize, NULL /*pvUser*/,
                               IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                               flashMMIOWrite, flashMMIORead,
                               "Flash Memory");
    AssertRCReturn(rc, rc);
    LogRel(("Registered %uKB flash at %RGp\n", pThis->cbFlashSize / _1K, pThis->GCPhysFlashBase));

    /*
     * Register saved state.
     */
    rc = PDMDevHlpSSMRegister(pDevIns, FLASH_SAVED_STATE_VERSION, sizeof(*pThis), flashSaveExec, flashLoadExec);
    if (RT_FAILURE(rc))
        return rc;

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceFlash =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "flash",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "Flash Memory Device",
    /* fFlags */
    PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT | PDM_DEVREG_FLAGS_GUEST_BITS_DEFAULT | PDM_DEVREG_FLAGS_R0 | PDM_DEVREG_FLAGS_RC,
    /* fClass */
    PDM_DEVREG_CLASS_ARCH,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(DEVFLASH),
    /* pfnConstruct */
    flashConstruct,
    /* pfnDestruct */
    flashDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    flashReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete. */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

#endif /* IN_RING3 */
#endif /* VBOX_DEVICE_STRUCT_TESTCASE */

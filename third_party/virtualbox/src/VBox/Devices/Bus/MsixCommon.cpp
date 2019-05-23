/* $Id: MsixCommon.cpp $ */
/** @file
 * MSI-X support routines
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#define LOG_GROUP LOG_GROUP_DEV_PCI
#define PDMPCIDEV_INCLUDE_PRIVATE  /* Hack to get pdmpcidevint.h included at the right point. */
#include <VBox/pci.h>
#include <VBox/msi.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/log.h>
#include <VBox/vmm/mm.h>

#include <iprt/assert.h>

#include "MsiCommon.h"
#include "PciInline.h"

#pragma pack(1)
typedef struct
{
    uint32_t  u32MsgAddressLo;
    uint32_t  u32MsgAddressHi;
    uint32_t  u32MsgData;
    uint32_t  u32VectorControl;
} MsixTableRecord;
AssertCompileSize(MsixTableRecord, VBOX_MSIX_ENTRY_SIZE);
#pragma pack()

/** @todo use accessors so that raw PCI devices work correctly with MSI-X. */
DECLINLINE(uint16_t)  msixGetMessageControl(PPDMPCIDEV pDev)
{
    return PCIDevGetWord(pDev, pDev->Int.s.u8MsixCapOffset + VBOX_MSIX_CAP_MESSAGE_CONTROL);
}

DECLINLINE(bool)      msixIsEnabled(PPDMPCIDEV pDev)
{
    return (msixGetMessageControl(pDev) & VBOX_PCI_MSIX_FLAGS_ENABLE) != 0;
}

DECLINLINE(bool)      msixIsMasked(PPDMPCIDEV pDev)
{
    return (msixGetMessageControl(pDev) & VBOX_PCI_MSIX_FLAGS_FUNCMASK) != 0;
}

DECLINLINE(uint16_t)  msixTableSize(PPDMPCIDEV pDev)
{
    return (msixGetMessageControl(pDev) & 0x3ff) + 1;
}

DECLINLINE(uint8_t*)  msixGetPageOffset(PPDMPCIDEV pDev, uint32_t off)
{
    return (uint8_t*)pDev->Int.s.CTX_SUFF(pMsixPage) + off;
}

DECLINLINE(MsixTableRecord*) msixGetVectorRecord(PPDMPCIDEV pDev, uint32_t iVector)
{
    return (MsixTableRecord*)msixGetPageOffset(pDev, iVector * VBOX_MSIX_ENTRY_SIZE);
}

DECLINLINE(RTGCPHYS)  msixGetMsiAddress(PPDMPCIDEV pDev, uint32_t iVector)
{
    MsixTableRecord* pRec = msixGetVectorRecord(pDev, iVector);
    return RT_MAKE_U64(pRec->u32MsgAddressLo & ~UINT32_C(0x3), pRec->u32MsgAddressHi);
}

DECLINLINE(uint32_t)  msixGetMsiData(PPDMPCIDEV pDev, uint32_t iVector)
{
    return msixGetVectorRecord(pDev, iVector)->u32MsgData;
}

DECLINLINE(uint32_t)  msixIsVectorMasked(PPDMPCIDEV pDev, uint32_t iVector)
{
    return (msixGetVectorRecord(pDev, iVector)->u32VectorControl & 0x1) != 0;
}

DECLINLINE(uint8_t*)  msixPendingByte(PPDMPCIDEV pDev, uint32_t iVector)
{
    return msixGetPageOffset(pDev, pDev->Int.s.offMsixPba + iVector / 8);
}

DECLINLINE(void)      msixSetPending(PPDMPCIDEV pDev, uint32_t iVector)
{
    *msixPendingByte(pDev, iVector) |= (1 << (iVector & 0x7));
}

DECLINLINE(void)      msixClearPending(PPDMPCIDEV pDev, uint32_t iVector)
{
    *msixPendingByte(pDev, iVector) &= ~(1 << (iVector & 0x7));
}

DECLINLINE(bool)      msixIsPending(PPDMPCIDEV pDev, uint32_t iVector)
{
    return (*msixPendingByte(pDev, iVector) & (1 << (iVector & 0x7))) != 0;
}

static void msixCheckPendingVector(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, uint32_t iVector)
{
    if (msixIsPending(pDev, iVector) && !msixIsVectorMasked(pDev, iVector))
        MsixNotify(pDevIns, pPciHlp, pDev, iVector, 1 /* iLevel */, 0 /*uTagSrc*/);
}

#ifdef IN_RING3

PDMBOTHCBDECL(int) msixMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    LogFlowFunc(("\n"));

    uint32_t off = (uint32_t)(GCPhysAddr & 0xffff);
    PPDMPCIDEV pPciDev = (PPDMPCIDEV)pvUser;

    /// @todo qword accesses?
    RT_NOREF(pDevIns);
    AssertMsgReturn(cb == 4,
                    ("MSI-X must be accessed with 4-byte reads"),
                    VERR_INTERNAL_ERROR);
    AssertMsgReturn(off < pPciDev->Int.s.cbMsixRegion,
                    ("Out of bounds access for the MSI-X region\n"),
                    VINF_IOM_MMIO_UNUSED_FF);

    *(uint32_t*)pv = *(uint32_t*)msixGetPageOffset(pPciDev, off);
    return VINF_SUCCESS;
}

PDMBOTHCBDECL(int) msixMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
    LogFlowFunc(("\n"));

    PPDMPCIDEV pPciDev = (PPDMPCIDEV)pvUser;
    uint32_t off = (uint32_t)(GCPhysAddr & 0xffff);

    /// @todo qword accesses?
    AssertMsgReturn(cb == 4,
                    ("MSI-X must be accessed with 4-byte reads"),
                    VERR_INTERNAL_ERROR);
    AssertMsgReturn(off < pPciDev->Int.s.offMsixPba,
                    ("Trying to write to PBA\n"),
                    VINF_IOM_MMIO_UNUSED_FF);

    *(uint32_t*)msixGetPageOffset(pPciDev, off) = *(uint32_t*)pv;

    msixCheckPendingVector(pDevIns, (PCPDMPCIHLP)pPciDev->Int.s.pPciBusPtrR3, pPciDev, off / VBOX_MSIX_ENTRY_SIZE);
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) msixMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                 RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    Assert(enmType == PCI_ADDRESS_SPACE_MEM);
    NOREF(iRegion); NOREF(enmType);

    int rc = PDMDevHlpMMIORegister(pDevIns, GCPhysAddress, cb, pPciDev,
                                   IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                                   msixMMIOWrite, msixMMIORead, "MSI-X tables");

    if (RT_FAILURE(rc))
        return rc;

    return VINF_SUCCESS;
}

int MsixInit(PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, PPDMMSIREG pMsiReg)
{
    if (pMsiReg->cMsixVectors == 0)
         return VINF_SUCCESS;

     /* We cannot init MSI-X on raw devices yet. */
    Assert(!pciDevIsPassthrough(pDev));

    uint16_t   cVectors    = pMsiReg->cMsixVectors;
    uint8_t    iCapOffset  = pMsiReg->iMsixCapOffset;
    uint8_t    iNextOffset = pMsiReg->iMsixNextOffset;
    uint8_t    iBar        = pMsiReg->iMsixBar;

    AssertMsgReturn(cVectors <= VBOX_MSIX_MAX_ENTRIES,
                    ("Too many MSI-X vectors: %d\n", cVectors),
                    VERR_TOO_MUCH_DATA);
    AssertMsgReturn(iBar <= 5,
                    ("Using wrong BAR for MSI-X: %d\n", iBar),
                    VERR_INVALID_PARAMETER);

    Assert(iCapOffset != 0 && iCapOffset < 0xff && iNextOffset < 0xff);

    int rc = VINF_SUCCESS;
    uint16_t cbPba = cVectors / 8;
    if (cVectors % 8)
        cbPba++;
    uint16_t cbMsixRegion = RT_ALIGN_T(cVectors * sizeof(MsixTableRecord) + cbPba, _4K, uint16_t);

    /* If device is passthrough, BAR is registered using common mechanism. */
    if (!pciDevIsPassthrough(pDev))
    {
        rc = PDMDevHlpPCIIORegionRegister(pDev->Int.s.CTX_SUFF(pDevIns), iBar, cbMsixRegion, PCI_ADDRESS_SPACE_MEM, msixMap);
        if (RT_FAILURE (rc))
            return rc;
    }

    uint16_t offTable = 0;
    uint16_t offPBA   = cVectors * sizeof(MsixTableRecord);

    pDev->Int.s.u8MsixCapOffset = iCapOffset;
    pDev->Int.s.u8MsixCapSize   = VBOX_MSIX_CAP_SIZE;
    pDev->Int.s.cbMsixRegion    = cbMsixRegion;
    pDev->Int.s.offMsixPba      = offPBA;
    PVM pVM = PDMDevHlpGetVM(pDev->Int.s.CTX_SUFF(pDevIns));

    pDev->Int.s.pMsixPageR3     = NULL;

    rc = MMHyperAlloc(pVM, cbMsixRegion, 1, MM_TAG_PDM_DEVICE_USER, (void **)&pDev->Int.s.pMsixPageR3);
    if (RT_FAILURE(rc) || (pDev->Int.s.pMsixPageR3 == NULL))
        return VERR_NO_VM_MEMORY;
    RT_BZERO(pDev->Int.s.pMsixPageR3, cbMsixRegion);
    pDev->Int.s.pMsixPageR0     = MMHyperR3ToR0(pVM, pDev->Int.s.pMsixPageR3);
    pDev->Int.s.pMsixPageRC     = MMHyperR3ToRC(pVM, pDev->Int.s.pMsixPageR3);

    /* R3 PCI helper */
    pDev->Int.s.pPciBusPtrR3    = pPciHlp;

    PCIDevSetByte(pDev,  iCapOffset + 0, VBOX_PCI_CAP_ID_MSIX);
    PCIDevSetByte(pDev,  iCapOffset + 1, iNextOffset); /* next */
    PCIDevSetWord(pDev,  iCapOffset + VBOX_MSIX_CAP_MESSAGE_CONTROL, cVectors - 1);

    PCIDevSetDWord(pDev,  iCapOffset + VBOX_MSIX_TABLE_BIROFFSET, offTable | iBar);
    PCIDevSetDWord(pDev,  iCapOffset + VBOX_MSIX_PBA_BIROFFSET,   offPBA   | iBar);

    pciDevSetMsixCapable(pDev);

    return VINF_SUCCESS;
}
#endif

bool     MsixIsEnabled(PPDMPCIDEV pDev)
{
    return pciDevIsMsixCapable(pDev) && msixIsEnabled(pDev);
}

void MsixNotify(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, int iVector, int iLevel, uint32_t uTagSrc)
{
    AssertMsg(msixIsEnabled(pDev), ("Must be enabled to use that"));

    Assert(pPciHlp->pfnIoApicSendMsi != NULL);

    /* We only trigger MSI-X on level up */
    if ((iLevel & PDM_IRQ_LEVEL_HIGH) == 0)
    {
        return;
    }

    // if this vector is somehow disabled
    if (msixIsMasked(pDev) || msixIsVectorMasked(pDev, iVector))
    {
        // mark pending bit
        msixSetPending(pDev, iVector);
        return;
    }

    // clear pending bit
    msixClearPending(pDev, iVector);

    RTGCPHYS   GCAddr = msixGetMsiAddress(pDev, iVector);
    uint32_t   u32Value = msixGetMsiData(pDev, iVector);

    pPciHlp->pfnIoApicSendMsi(pDevIns, GCAddr, u32Value, uTagSrc);
}

DECLINLINE(bool) msixBitJustCleared(uint32_t uOldValue,
                                    uint32_t uNewValue,
                                    uint32_t uMask)
{
    return (!!(uOldValue & uMask) && !(uNewValue & uMask));
}

static void msixCheckPendingVectors(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev)
{
    for (uint32_t i = 0; i < msixTableSize(pDev); i++)
        msixCheckPendingVector(pDevIns, pPciHlp, pDev, i);
}


void MsixPciConfigWrite(PPDMDEVINS pDevIns, PCPDMPCIHLP pPciHlp, PPDMPCIDEV pDev, uint32_t u32Address, uint32_t val, unsigned len)
{
    int32_t iOff = u32Address - pDev->Int.s.u8MsixCapOffset;
    Assert(iOff >= 0 && (pciDevIsMsixCapable(pDev) && iOff < pDev->Int.s.u8MsixCapSize));

    Log2(("MsixPciConfigWrite: %d <- %x (%d)\n", iOff, val, len));

    uint32_t uAddr = u32Address;
    uint8_t u8NewVal;
    bool fJustEnabled = false;

    for (uint32_t i = 0; i < len; i++)
    {
        uint32_t reg = i + iOff;
        uint8_t u8Val = (uint8_t)val;
        switch (reg)
        {
            case 0: /* Capability ID, ro */
            case 1: /* Next pointer,  ro */
                break;
            case VBOX_MSIX_CAP_MESSAGE_CONTROL:
                /* don't change read-only bits: 0-7 */
                break;
            case VBOX_MSIX_CAP_MESSAGE_CONTROL + 1:
            {
                /* don't change read-only bits 8-13 */
                u8NewVal = (u8Val & UINT8_C(~0x3f)) | (pDev->abConfig[uAddr] & UINT8_C(0x3f));
                /* If just enabled globally - check pending vectors */
                fJustEnabled |= msixBitJustCleared(pDev->abConfig[uAddr], u8NewVal, VBOX_PCI_MSIX_FLAGS_ENABLE >> 8);
                fJustEnabled |= msixBitJustCleared(pDev->abConfig[uAddr], u8NewVal, VBOX_PCI_MSIX_FLAGS_FUNCMASK >> 8);
                pDev->abConfig[uAddr] = u8NewVal;
                break;
        }
            default:
                /* other fields read-only too */
                break;
        }
        uAddr++;
        val >>= 8;
    }

    if (fJustEnabled)
        msixCheckPendingVectors(pDevIns, pPciHlp, pDev);
}


/* $Id: VBoxSCSI.cpp $ */
/** @file
 * VBox storage devices - Simple SCSI interface for BIOS access.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
//#define DEBUG
#define LOG_GROUP LOG_GROUP_DEV_BUSLOGIC /** @todo Create extra group. */

#if defined(IN_R0) || defined(IN_RC)
# error This device has no R0 or RC components
#endif

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pgm.h>
#include <VBox/version.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/thread.h>
#include <iprt/string.h>

#include "VBoxSCSI.h"


/**
 * Resets the state.
 */
static void vboxscsiReset(PVBOXSCSI pVBoxSCSI, bool fEverything)
{
    if (fEverything)
    {
        pVBoxSCSI->regIdentify = 0;
        pVBoxSCSI->fBusy       = false;
    }
    pVBoxSCSI->cbCDB         = 0;
    RT_ZERO(pVBoxSCSI->abCDB);
    pVBoxSCSI->iCDB          = 0;
    pVBoxSCSI->rcCompletion  = 0;
    pVBoxSCSI->uTargetDevice = 0;
    pVBoxSCSI->cbBuf         = 0;
    pVBoxSCSI->cbBufLeft     = 0;
    pVBoxSCSI->iBuf          = 0;
    if (pVBoxSCSI->pbBuf)
        RTMemFree(pVBoxSCSI->pbBuf);
    pVBoxSCSI->pbBuf         = NULL;
    pVBoxSCSI->enmState      = VBOXSCSISTATE_NO_COMMAND;
}

/**
 * Initializes the state for the SCSI interface.
 *
 * @returns VBox status code.
 * @param   pVBoxSCSI    Pointer to the unitialized SCSI state.
 */
int vboxscsiInitialize(PVBOXSCSI pVBoxSCSI)
{
    pVBoxSCSI->pbBuf = NULL;
    vboxscsiReset(pVBoxSCSI, true /*fEverything*/);

    return VINF_SUCCESS;
}

/**
 * Reads a register value.
 *
 * @returns VBox status code.
 * @param   pVBoxSCSI    Pointer to the SCSI state.
 * @param   iRegister    Index of the register to read.
 * @param   pu32Value    Where to store the content of the register.
 */
int vboxscsiReadRegister(PVBOXSCSI pVBoxSCSI, uint8_t iRegister, uint32_t *pu32Value)
{
    uint8_t uVal = 0;

    switch (iRegister)
    {
        case 0:
        {
            if (ASMAtomicReadBool(&pVBoxSCSI->fBusy) == true)
            {
                uVal |= VBOX_SCSI_BUSY;
                /* There is an I/O operation in progress.
                 * Yield the execution thread to let the I/O thread make progress.
                 */
                RTThreadYield();
            }
            if (pVBoxSCSI->rcCompletion)
                uVal |= VBOX_SCSI_ERROR;
            break;
        }
        case 1:
        {
            /* If we're not in the 'command ready' state, there may not even be a buffer yet. */
            if (   pVBoxSCSI->enmState == VBOXSCSISTATE_COMMAND_READY
                && pVBoxSCSI->cbBufLeft > 0)
            {
                AssertMsg(pVBoxSCSI->pbBuf, ("pBuf is NULL\n"));
                Assert(!pVBoxSCSI->fBusy);
                uVal = pVBoxSCSI->pbBuf[pVBoxSCSI->iBuf];
                pVBoxSCSI->iBuf++;
                pVBoxSCSI->cbBufLeft--;

                /* When the guest reads the last byte from the data in buffer, clear
                   everything and reset command buffer. */
                if (pVBoxSCSI->cbBufLeft == 0)
                    vboxscsiReset(pVBoxSCSI, false /*fEverything*/);
            }
            break;
        }
        case 2:
        {
            uVal = pVBoxSCSI->regIdentify;
            break;
        }
        case 3:
        {
            uVal = pVBoxSCSI->rcCompletion;
            break;
        }
        default:
            AssertMsgFailed(("Invalid register to read from %u\n", iRegister));
    }

    *pu32Value = uVal;

    return VINF_SUCCESS;
}

/**
 * Writes to a register.
 *
 * @returns VBox status code.
 * @retval  VERR_MORE_DATA if a command is ready to be sent to the SCSI driver.
 * @param   pVBoxSCSI    Pointer to the SCSI state.
 * @param   iRegister    Index of the register to write to.
 * @param   uVal         Value to write.
 */
int vboxscsiWriteRegister(PVBOXSCSI pVBoxSCSI, uint8_t iRegister, uint8_t uVal)
{
    int rc = VINF_SUCCESS;

    switch (iRegister)
    {
        case 0:
        {
            if (pVBoxSCSI->enmState == VBOXSCSISTATE_NO_COMMAND)
            {
                pVBoxSCSI->enmState = VBOXSCSISTATE_READ_TXDIR;
                pVBoxSCSI->uTargetDevice = uVal;
            }
            else if (pVBoxSCSI->enmState == VBOXSCSISTATE_READ_TXDIR)
            {
                if (uVal != VBOXSCSI_TXDIR_FROM_DEVICE && uVal != VBOXSCSI_TXDIR_TO_DEVICE)
                    vboxscsiReset(pVBoxSCSI, true /*fEverything*/);
                else
                {
                    pVBoxSCSI->enmState = VBOXSCSISTATE_READ_CDB_SIZE_BUFHI;
                    pVBoxSCSI->uTxDir = uVal;
                }
            }
            else if (pVBoxSCSI->enmState == VBOXSCSISTATE_READ_CDB_SIZE_BUFHI)
            {
                uint8_t cbCDB = uVal & 0x0F;

                if (cbCDB == 0)
                    cbCDB = 16;
                if (cbCDB > VBOXSCSI_CDB_SIZE_MAX)
                    vboxscsiReset(pVBoxSCSI, true /*fEverything*/);
                else
                {
                    pVBoxSCSI->enmState = VBOXSCSISTATE_READ_BUFFER_SIZE_LSB;
                    pVBoxSCSI->cbCDB = cbCDB;
                    pVBoxSCSI->cbBuf = (uVal & 0xF0) << 12;     /* Bits 16-19 of buffer size. */
                }
            }
            else if (pVBoxSCSI->enmState == VBOXSCSISTATE_READ_BUFFER_SIZE_LSB)
            {
                pVBoxSCSI->enmState = VBOXSCSISTATE_READ_BUFFER_SIZE_MID;
                pVBoxSCSI->cbBuf |= uVal;                       /* Bits 0-7 of buffer size. */
            }
            else if (pVBoxSCSI->enmState == VBOXSCSISTATE_READ_BUFFER_SIZE_MID)
            {
                pVBoxSCSI->enmState = VBOXSCSISTATE_READ_COMMAND;
                pVBoxSCSI->cbBuf |= (((uint16_t)uVal) << 8);    /* Bits 8-15 of buffer size. */
            }
            else if (pVBoxSCSI->enmState == VBOXSCSISTATE_READ_COMMAND)
            {
                pVBoxSCSI->abCDB[pVBoxSCSI->iCDB] = uVal;
                pVBoxSCSI->iCDB++;

                /* Check if we have all necessary command data. */
                if (pVBoxSCSI->iCDB == pVBoxSCSI->cbCDB)
                {
                    Log(("%s: Command ready for processing\n", __FUNCTION__));
                    pVBoxSCSI->enmState = VBOXSCSISTATE_COMMAND_READY;
                    pVBoxSCSI->cbBufLeft = pVBoxSCSI->cbBuf;
                    if (pVBoxSCSI->uTxDir == VBOXSCSI_TXDIR_TO_DEVICE)
                    {
                        /* This is a write allocate buffer. */
                        pVBoxSCSI->pbBuf = (uint8_t *)RTMemAllocZ(pVBoxSCSI->cbBuf);
                        if (!pVBoxSCSI->pbBuf)
                            return VERR_NO_MEMORY;
                    }
                    else
                    {
                        /* This is a read from the device. */
                        ASMAtomicXchgBool(&pVBoxSCSI->fBusy, true);
                        rc = VERR_MORE_DATA; /** @todo Better return value to indicate ready command? */
                    }
                }
            }
            else
                AssertMsgFailed(("Invalid state %d\n", pVBoxSCSI->enmState));
            break;
        }

        case 1:
        {
            if (   pVBoxSCSI->enmState != VBOXSCSISTATE_COMMAND_READY
                || pVBoxSCSI->uTxDir != VBOXSCSI_TXDIR_TO_DEVICE)
            {
                /* Reset the state */
                vboxscsiReset(pVBoxSCSI, true /*fEverything*/);
            }
            else if (pVBoxSCSI->cbBufLeft > 0)
            {
                pVBoxSCSI->pbBuf[pVBoxSCSI->iBuf++] = uVal;
                pVBoxSCSI->cbBufLeft--;
                if (pVBoxSCSI->cbBufLeft == 0)
                {
                    rc = VERR_MORE_DATA;
                    ASMAtomicXchgBool(&pVBoxSCSI->fBusy, true);
                }
            }
            /* else: Ignore extra data, request pending or something. */
            break;
        }

        case 2:
        {
            pVBoxSCSI->regIdentify = uVal;
            break;
        }

        case 3:
        {
            /* Reset */
            vboxscsiReset(pVBoxSCSI, true /*fEverything*/);
            break;
        }

        default:
            AssertMsgFailed(("Invalid register to write to %u\n", iRegister));
    }

    return rc;
}

/**
 * Sets up a SCSI request which the owning SCSI device can process.
 *
 * @returns VBox status code.
 * @param   pVBoxSCSI      Pointer to the SCSI state.
 * @param   puLun          Where to store the LUN on success.
 * @param   ppbCdb         Where to store the pointer to the CDB on success.
 * @param   pcbCdb         Where to store the size of the CDB on success.
 * @param   pcbBuf         Where to store th size of the data buffer on success.
 * @param   puTargetDevice Where to store the target device ID.
 */
int vboxscsiSetupRequest(PVBOXSCSI pVBoxSCSI, uint32_t *puLun, uint8_t **ppbCdb,
                         size_t *pcbCdb, size_t *pcbBuf, uint32_t *puTargetDevice)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("pVBoxSCSI=%#p puTargetDevice=%#p\n", pVBoxSCSI, puTargetDevice));

    AssertMsg(pVBoxSCSI->enmState == VBOXSCSISTATE_COMMAND_READY, ("Invalid state %u\n", pVBoxSCSI->enmState));

    /* Clear any errors from a previous request. */
    pVBoxSCSI->rcCompletion = 0;

    if (pVBoxSCSI->uTxDir == VBOXSCSI_TXDIR_FROM_DEVICE)
    {
        if (pVBoxSCSI->pbBuf)
            RTMemFree(pVBoxSCSI->pbBuf);

        pVBoxSCSI->pbBuf = (uint8_t *)RTMemAllocZ(pVBoxSCSI->cbBuf);
        if (!pVBoxSCSI->pbBuf)
            return VERR_NO_MEMORY;
    }

    *puLun = 0;
    *ppbCdb = &pVBoxSCSI->abCDB[0];
    *pcbCdb = pVBoxSCSI->cbCDB;
    *pcbBuf = pVBoxSCSI->cbBuf;
    *puTargetDevice = pVBoxSCSI->uTargetDevice;

    return rc;
}

/**
 * Notifies the device that a request finished and the incoming data
 * is ready at the incoming data port.
 */
int vboxscsiRequestFinished(PVBOXSCSI pVBoxSCSI, int rcCompletion)
{
    LogFlowFunc(("pVBoxSCSI=%#p\n", pVBoxSCSI));

    if (pVBoxSCSI->uTxDir == VBOXSCSI_TXDIR_TO_DEVICE)
        vboxscsiReset(pVBoxSCSI, false /*fEverything*/);

    pVBoxSCSI->rcCompletion = rcCompletion;

    ASMAtomicXchgBool(&pVBoxSCSI->fBusy, false);

    return VINF_SUCCESS;
}

size_t vboxscsiCopyToBuf(PVBOXSCSI pVBoxSCSI, PRTSGBUF pSgBuf, size_t cbSkip, size_t cbCopy)
{
    AssertPtrReturn(pVBoxSCSI->pbBuf, 0);
    AssertReturn(cbSkip + cbCopy <= pVBoxSCSI->cbBuf, 0);

    void *pvBuf = pVBoxSCSI->pbBuf + cbSkip;
    return RTSgBufCopyToBuf(pSgBuf, pvBuf, cbCopy);
}

size_t vboxscsiCopyFromBuf(PVBOXSCSI pVBoxSCSI, PRTSGBUF pSgBuf, size_t cbSkip, size_t cbCopy)
{
    AssertPtrReturn(pVBoxSCSI->pbBuf, 0);
    AssertReturn(cbSkip + cbCopy <= pVBoxSCSI->cbBuf, 0);

    void *pvBuf = pVBoxSCSI->pbBuf + cbSkip;
    return RTSgBufCopyFromBuf(pSgBuf, pvBuf, cbCopy);
}

int vboxscsiReadString(PPDMDEVINS pDevIns, PVBOXSCSI pVBoxSCSI, uint8_t iRegister,
                       uint8_t *pbDst, uint32_t *pcTransfers, unsigned cb)
{
    RT_NOREF(pDevIns);
    LogFlowFunc(("pDevIns=%#p pVBoxSCSI=%#p iRegister=%d cTransfers=%u cb=%u\n",
                 pDevIns, pVBoxSCSI, iRegister, *pcTransfers, cb));

    /*
     * Check preconditions, fall back to non-string I/O handler.
     */
    Assert(*pcTransfers > 0);

    /* Read string only valid for data in register. */
    AssertMsgReturn(iRegister == 1, ("Hey! Only register 1 can be read from with string!\n"), VINF_SUCCESS);

    /* Accesses without a valid buffer will be ignored. */
    AssertReturn(pVBoxSCSI->pbBuf, VINF_SUCCESS);

    /* Check state. */
    AssertReturn(pVBoxSCSI->enmState == VBOXSCSISTATE_COMMAND_READY, VINF_SUCCESS);
    Assert(!pVBoxSCSI->fBusy);

    /*
     * Also ignore attempts to read more data than is available.
     */
    int rc = VINF_SUCCESS;
    uint32_t cbTransfer = *pcTransfers * cb;
    if (pVBoxSCSI->cbBufLeft > 0)
    {
        Assert(cbTransfer <= pVBoxSCSI->cbBuf);
        if (cbTransfer > pVBoxSCSI->cbBuf)
        {
            memset(pbDst + pVBoxSCSI->cbBuf, 0xff, cbTransfer - pVBoxSCSI->cbBuf);
            cbTransfer = pVBoxSCSI->cbBuf;  /* Ignore excess data (not supposed to happen). */
        }

        /* Copy the data and adance the buffer position. */
        memcpy(pbDst, pVBoxSCSI->pbBuf + pVBoxSCSI->iBuf, cbTransfer);

        /* Advance current buffer position. */
        pVBoxSCSI->iBuf      += cbTransfer;
        pVBoxSCSI->cbBufLeft -= cbTransfer;

        /* When the guest reads the last byte from the data in buffer, clear
           everything and reset command buffer. */
        if (pVBoxSCSI->cbBufLeft == 0)
            vboxscsiReset(pVBoxSCSI, false /*fEverything*/);
    }
    else
    {
        AssertFailed();
        memset(pbDst, 0, cbTransfer);
    }
    *pcTransfers = 0;

    return rc;
}

int vboxscsiWriteString(PPDMDEVINS pDevIns, PVBOXSCSI pVBoxSCSI, uint8_t iRegister,
                        uint8_t const *pbSrc, uint32_t *pcTransfers, unsigned cb)
{
    RT_NOREF(pDevIns);

    /*
     * Check preconditions, fall back to non-string I/O handler.
     */
    Assert(*pcTransfers > 0);
    /* Write string only valid for data in/out register. */
    AssertMsgReturn(iRegister == 1, ("Hey! Only register 1 can be written to with string!\n"), VINF_SUCCESS);

    /* Accesses without a valid buffer will be ignored. */
    AssertReturn(pVBoxSCSI->pbBuf, VINF_SUCCESS);

    /* State machine assumptions. */
    AssertReturn(pVBoxSCSI->enmState == VBOXSCSISTATE_COMMAND_READY, VINF_SUCCESS);
    AssertReturn(pVBoxSCSI->uTxDir == VBOXSCSI_TXDIR_TO_DEVICE, VINF_SUCCESS);

    /*
     * Ignore excess data (not supposed to happen).
     */
    int rc = VINF_SUCCESS;
    if (pVBoxSCSI->cbBufLeft > 0)
    {
        uint32_t cbTransfer = RT_MIN(*pcTransfers * cb, pVBoxSCSI->cbBufLeft);

        /* Copy the data and adance the buffer position. */
        memcpy(pVBoxSCSI->pbBuf + pVBoxSCSI->iBuf, pbSrc, cbTransfer);
        pVBoxSCSI->iBuf      += cbTransfer;
        pVBoxSCSI->cbBufLeft -= cbTransfer;

        /* If we've reached the end, tell the caller to submit the command. */
        if (pVBoxSCSI->cbBufLeft == 0)
        {
            ASMAtomicXchgBool(&pVBoxSCSI->fBusy, true);
            rc = VERR_MORE_DATA;
        }
    }
    else
        AssertFailed();
    *pcTransfers = 0;

    return rc;
}

void vboxscsiSetRequestRedo(PVBOXSCSI pVBoxSCSI)
{
    AssertMsg(pVBoxSCSI->fBusy, ("No request to redo\n"));

    if (pVBoxSCSI->uTxDir == VBOXSCSI_TXDIR_FROM_DEVICE)
    {
        AssertPtr(pVBoxSCSI->pbBuf);
    }
}

DECLHIDDEN(int) vboxscsiR3LoadExec(PVBOXSCSI pVBoxSCSI, PSSMHANDLE pSSM)
{
    SSMR3GetU8  (pSSM, &pVBoxSCSI->regIdentify);
    SSMR3GetU8  (pSSM, &pVBoxSCSI->uTargetDevice);
    SSMR3GetU8  (pSSM, &pVBoxSCSI->uTxDir);
    SSMR3GetU8  (pSSM, &pVBoxSCSI->cbCDB);

    /*
     * The CDB buffer was increased with r104155 in trunk (backported to 5.0
     * in r104311) without bumping the SSM state versions which leaves us
     * with broken saved state restoring for older VirtualBox releases
     * (up to 5.0.10).
     */
    if (   (   SSMR3HandleRevision(pSSM) < 104311
            && SSMR3HandleVersion(pSSM)  < VBOX_FULL_VERSION_MAKE(5, 0, 12))
        || (   SSMR3HandleRevision(pSSM) < 104155
            && SSMR3HandleVersion(pSSM)  >= VBOX_FULL_VERSION_MAKE(5, 0, 51)))
    {
        memset(&pVBoxSCSI->abCDB[0], 0, sizeof(pVBoxSCSI->abCDB));
        SSMR3GetMem (pSSM, &pVBoxSCSI->abCDB[0], 12);
    }
    else
        SSMR3GetMem (pSSM, &pVBoxSCSI->abCDB[0], sizeof(pVBoxSCSI->abCDB));

    SSMR3GetU8  (pSSM, &pVBoxSCSI->iCDB);
    SSMR3GetU32 (pSSM, &pVBoxSCSI->cbBufLeft);
    SSMR3GetU32 (pSSM, &pVBoxSCSI->iBuf);
    SSMR3GetBool(pSSM, (bool *)&pVBoxSCSI->fBusy);
    SSMR3GetU8  (pSSM, (uint8_t *)&pVBoxSCSI->enmState);

    /*
     * Old saved states only save the size of the buffer left to read/write.
     * To avoid changing the saved state version we can just calculate the original
     * buffer size from the offset and remaining size.
     */
    pVBoxSCSI->cbBuf = pVBoxSCSI->cbBufLeft + pVBoxSCSI->iBuf;

    if (pVBoxSCSI->cbBuf)
    {
        pVBoxSCSI->pbBuf = (uint8_t *)RTMemAllocZ(pVBoxSCSI->cbBuf);
        if (!pVBoxSCSI->pbBuf)
            return VERR_NO_MEMORY;

        SSMR3GetMem(pSSM, pVBoxSCSI->pbBuf, pVBoxSCSI->cbBuf);
    }

    return VINF_SUCCESS;
}

DECLHIDDEN(int) vboxscsiR3SaveExec(PVBOXSCSI pVBoxSCSI, PSSMHANDLE pSSM)
{
    SSMR3PutU8    (pSSM, pVBoxSCSI->regIdentify);
    SSMR3PutU8    (pSSM, pVBoxSCSI->uTargetDevice);
    SSMR3PutU8    (pSSM, pVBoxSCSI->uTxDir);
    SSMR3PutU8    (pSSM, pVBoxSCSI->cbCDB);
    SSMR3PutMem   (pSSM, pVBoxSCSI->abCDB, sizeof(pVBoxSCSI->abCDB));
    SSMR3PutU8    (pSSM, pVBoxSCSI->iCDB);
    SSMR3PutU32   (pSSM, pVBoxSCSI->cbBufLeft);
    SSMR3PutU32   (pSSM, pVBoxSCSI->iBuf);
    SSMR3PutBool  (pSSM, pVBoxSCSI->fBusy);
    SSMR3PutU8    (pSSM, pVBoxSCSI->enmState);

    if (pVBoxSCSI->cbBuf)
        SSMR3PutMem(pSSM, pVBoxSCSI->pbBuf, pVBoxSCSI->cbBuf);

    return VINF_SUCCESS;
}

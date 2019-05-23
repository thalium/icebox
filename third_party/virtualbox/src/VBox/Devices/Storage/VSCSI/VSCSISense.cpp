/* $Id: VSCSISense.cpp $ */
/** @file
 * Virtual SCSI driver: Sense handling
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
#define LOG_GROUP LOG_GROUP_VSCSI
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/string.h>

#include "VSCSIInternal.h"

void vscsiSenseInit(PVSCSISENSE pVScsiSense)
{
    memset(pVScsiSense->abSenseBuf, 0, sizeof(pVScsiSense->abSenseBuf));

    /* Fill in valid sense information (can't be just zeros). */
    pVScsiSense->abSenseBuf[0]  = (1 << 7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED; /* Fixed format */
    pVScsiSense->abSenseBuf[2]  = SCSI_SENSE_NONE;
    pVScsiSense->abSenseBuf[7]  = 10;
    pVScsiSense->abSenseBuf[12] = SCSI_ASC_NONE;
}

int vscsiReqSenseOkSet(PVSCSISENSE pVScsiSense, PVSCSIREQINT pVScsiReq)
{
    memset(pVScsiSense->abSenseBuf, 0, sizeof(pVScsiSense->abSenseBuf));

    pVScsiSense->abSenseBuf[0]  = (1 << 7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED; /* Fixed format */
    pVScsiSense->abSenseBuf[2]  = SCSI_SENSE_NONE;
    pVScsiSense->abSenseBuf[7]  = 10;
    pVScsiSense->abSenseBuf[12] = SCSI_ASC_NONE;
    pVScsiSense->abSenseBuf[13] = SCSI_ASC_NONE; /* Should be ASCQ but it has the same value for success. */

    if (pVScsiReq->pbSense && pVScsiReq->cbSense)
        memcpy(pVScsiReq->pbSense, pVScsiSense->abSenseBuf, RT_MIN(sizeof(pVScsiSense->abSenseBuf), pVScsiReq->cbSense));

    return SCSI_STATUS_OK;
}

int vscsiReqSenseErrorSet(PVSCSISENSE pVScsiSense, PVSCSIREQINT pVScsiReq, uint8_t uSCSISenseKey, uint8_t uSCSIASC, uint8_t uSCSIASCQ)
{
    memset(pVScsiSense->abSenseBuf, 0, sizeof(pVScsiSense->abSenseBuf));
    pVScsiSense->abSenseBuf[0] = (1 << 7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED; /* Fixed format */
    pVScsiSense->abSenseBuf[2] = uSCSISenseKey;
    pVScsiSense->abSenseBuf[7]  = 10;
    pVScsiSense->abSenseBuf[12] = uSCSIASC;
    pVScsiSense->abSenseBuf[13] = uSCSIASCQ;

    if (pVScsiReq->pbSense && pVScsiReq->cbSense)
        memcpy(pVScsiReq->pbSense, pVScsiSense->abSenseBuf, RT_MIN(sizeof(pVScsiSense->abSenseBuf), pVScsiReq->cbSense));

    return SCSI_STATUS_CHECK_CONDITION;
}

int vscsiReqSenseErrorInfoSet(PVSCSISENSE pVScsiSense, PVSCSIREQINT pVScsiReq, uint8_t uSCSISenseKey, uint8_t uSCSIASC, uint8_t uSCSIASCQ, uint32_t uInfo)
{
    memset(pVScsiSense->abSenseBuf, 0, sizeof(pVScsiSense->abSenseBuf));
    pVScsiSense->abSenseBuf[0] = RT_BIT(7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED; /* Fixed format */
    pVScsiSense->abSenseBuf[2] = uSCSISenseKey;
    scsiH2BE_U32(&pVScsiSense->abSenseBuf[3], uInfo);
    pVScsiSense->abSenseBuf[7]  = 10;
    pVScsiSense->abSenseBuf[12] = uSCSIASC;
    pVScsiSense->abSenseBuf[13] = uSCSIASCQ;

    if (pVScsiReq->pbSense && pVScsiReq->cbSense)
        memcpy(pVScsiReq->pbSense, pVScsiSense->abSenseBuf, RT_MIN(sizeof(pVScsiSense->abSenseBuf), pVScsiReq->cbSense));

    return SCSI_STATUS_CHECK_CONDITION;
}

int vscsiReqSenseCmd(PVSCSISENSE pVScsiSense, PVSCSIREQINT pVScsiReq)
{
    /* Copy the current sense data to the buffer. */
    RTSgBufCopyFromBuf(&pVScsiReq->SgBuf, pVScsiSense->abSenseBuf, sizeof(pVScsiSense->abSenseBuf));
    return vscsiReqSenseOkSet(pVScsiSense, pVScsiReq);
}


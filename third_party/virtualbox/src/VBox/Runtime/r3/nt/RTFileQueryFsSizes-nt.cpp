/* $Id: RTFileQueryFsSizes-nt.cpp $ */
/** @file
 * IPRT - RTFileQueryFsSizes, Native NT.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_FILE
#include "internal-r3-nt.h"

#include <iprt/file.h>
#include <iprt/err.h>



RTR3DECL(int) RTFileQueryFsSizes(RTFILE hFile, PRTFOFF pcbTotal, RTFOFF *pcbFree,
                                 uint32_t *pcbBlock, uint32_t *pcbSector)
{
    int rc;

    /*
     * Get the volume information.
     */
    FILE_FS_SIZE_INFORMATION FsSizeInfo;
    IO_STATUS_BLOCK Ios = RTNT_IO_STATUS_BLOCK_INITIALIZER;
    NTSTATUS rcNt = NtQueryVolumeInformationFile((HANDLE)RTFileToNative(hFile), &Ios,
                                                 &FsSizeInfo, sizeof(FsSizeInfo), FileFsSizeInformation);
    if (NT_SUCCESS(rcNt))
    {
        /*
         * Calculate the return values.
         */
        if (pcbTotal)
        {
            *pcbTotal = FsSizeInfo.TotalAllocationUnits.QuadPart
                      * FsSizeInfo.SectorsPerAllocationUnit
                      * FsSizeInfo.BytesPerSector;
            if (   *pcbTotal / FsSizeInfo.SectorsPerAllocationUnit / FsSizeInfo.BytesPerSector
                != FsSizeInfo.TotalAllocationUnits.QuadPart)
                *pcbTotal = UINT64_MAX;
        }

        if (pcbFree)
        {
            *pcbFree = FsSizeInfo.AvailableAllocationUnits.QuadPart
                     * FsSizeInfo.SectorsPerAllocationUnit
                     * FsSizeInfo.BytesPerSector;
            if (   *pcbFree / FsSizeInfo.SectorsPerAllocationUnit / FsSizeInfo.BytesPerSector
                != FsSizeInfo.AvailableAllocationUnits.QuadPart)
                *pcbFree = UINT64_MAX;
        }

        rc = VINF_SUCCESS;
        if (pcbBlock)
        {
            *pcbBlock  = FsSizeInfo.SectorsPerAllocationUnit * FsSizeInfo.BytesPerSector;
            if (*pcbBlock / FsSizeInfo.BytesPerSector != FsSizeInfo.SectorsPerAllocationUnit)
                rc = VERR_OUT_OF_RANGE;
        }

        if (pcbSector)
            *pcbSector = FsSizeInfo.BytesPerSector;
    }
    else
        rc = RTErrConvertFromNtStatus(rcNt);

    return rc;
}


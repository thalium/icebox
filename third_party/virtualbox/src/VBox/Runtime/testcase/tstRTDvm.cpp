/* $Id: tstRTDvm.cpp $ */
/** @file
 * IPRT Testcase - IPRT Disk Volume Management (DVM)
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
#include <iprt/dvm.h>

#include <iprt/err.h>
#include <iprt/test.h>
#include <iprt/file.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Disk structure.
 */
typedef struct TSTRTDVMDISK
{
    /** Flag whether this disk uses the image file or a volume. */
    bool            fUseImage;
    /** Data dependent on the flag. */
    union
    {
        /** File handle of the image. */
        RTVFSFILE   hImage;
        /** Handle of the volume. */
        RTDVMVOLUME hVol;
    };
} TSTRTDVMDISK, *PTSTRTDVMDISK;


#if 0
static DECLCALLBACK(int) dvmDiskRead(void *pvUser, uint64_t off, void *pvBuf, size_t cbRead)
{
    PTSTRTDVMDISK pDisk = (PTSTRTDVMDISK)pvUser;

    if (pDisk->fUseImage)
        return RTFileReadAt(pDisk->hImage, off, pvBuf, cbRead, NULL);
    return RTDvmVolumeRead(pDisk->hVol, off, pvBuf, cbRead);
}

static DECLCALLBACK(int) dvmDiskWrite(void *pvUser, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    PTSTRTDVMDISK pDisk = (PTSTRTDVMDISK)pvUser;

    if (pDisk->fUseImage)
        return RTFileWriteAt(pDisk->hImage, off, pvBuf, cbWrite, NULL);
    return RTDvmVolumeWrite(pDisk->hVol, off, pvBuf, cbWrite);
}

static int tstRTDvmVolume(RTTEST hTest, PTSTRTDVMDISK pDisk, uint64_t cb, unsigned cNesting)
{
    char szPrefix[100];
    int rc = VINF_SUCCESS;

    RT_ZERO(szPrefix);

    if (cNesting < sizeof(szPrefix) - 1)
    {
        for (unsigned i = 0; i < cNesting; i++)
            szPrefix[i] = '\t';
    }

    RTTestSubF(hTest, "Create DVM");
    RTDVM hVolMgr;
    rc = RTDvmCreate(&hVolMgr, dvmDiskRead, dvmDiskWrite, cb, 512, 0, pDisk);
    if (RT_FAILURE(rc))
    {
        RTTestIFailed("RTDvmCreate -> %Rrc", rc);
        return RTTestSummaryAndDestroy(hTest);
    }

    RTTestSubF(hTest, "Open volume map");
    rc = RTDvmMapOpen(hVolMgr);
    if (   RT_FAILURE(rc)
        && rc != VERR_NOT_SUPPORTED)
    {
        RTTestIFailed("RTDvmOpen -> %Rrc", rc);
        return RTTestSummaryAndDestroy(hTest);
    }
    if (rc == VERR_NOT_SUPPORTED)
        return VINF_SUCCESS;

    RTTestIPrintf(RTTESTLVL_ALWAYS, "%s Successfully opened map with format: %s.\n", szPrefix, RTDvmMapGetFormat(hVolMgr));

    /* Dump all volumes. */
    RTTestSubF(hTest, "Dump volumes");
    uint32_t cVolume = 0;
    RTDVMVOLUME hVol;

    rc = RTDvmMapQueryFirstVolume(hVolMgr, &hVol);

    while (RT_SUCCESS(rc))
    {
        char *pszVolName = NULL;
        RTDVMVOLTYPE enmVolType = RTDvmVolumeGetType(hVol);
        uint64_t fVolFlags = RTDvmVolumeGetFlags(hVol);

        RTTestIPrintf(RTTESTLVL_ALWAYS, "%s Volume %u:\n", szPrefix, cVolume);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%s Volume type  %s\n", szPrefix, RTDvmVolumeTypeGetDescr(enmVolType));
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%s Volume size  %llu\n", szPrefix, RTDvmVolumeGetSize(hVol));
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%s Volume flags %s %s\n\n", szPrefix,
                      fVolFlags & DVMVOLUME_FLAGS_BOOTABLE ? "Bootable" : "",
                      fVolFlags & DVMVOLUME_FLAGS_ACTIVE ? "Active" : "");

        rc = RTDvmVolumeQueryName(hVol, &pszVolName);
        if (RT_SUCCESS(rc))
        {
            RTTestIPrintf(RTTESTLVL_ALWAYS, "%s Volume name %s.\n", szPrefix, pszVolName);
            RTStrFree(pszVolName);
        }
        else if (rc != VERR_NOT_SUPPORTED)
            RTTestIFailed("RTDvmVolumeQueryName -> %Rrc", rc);
        else
            rc = VINF_SUCCESS;

        /*
         * Query all volumes which might be inside this.
         * (think of MBR partitions with a bsdlabel inside)
         */
        TSTRTDVMDISK Disk;
        Disk.fUseImage = false;
        Disk.hVol      = hVol;
        rc = tstRTDvmVolume(hTest, &Disk, RTDvmVolumeGetSize(hVol), cNesting + 1);

        RTDVMVOLUME hVolNext;
        rc = RTDvmMapQueryNextVolume(hVolMgr, hVol, &hVolNext);
        RTDvmVolumeRelease(hVol);
        hVol = hVolNext;
        cVolume++;
    }

    RTTestIPrintf(RTTESTLVL_ALWAYS, "%s Dumped %u volumes\n", szPrefix, cVolume);

    if (   rc == VERR_DVM_MAP_EMPTY
        || rc == VERR_DVM_MAP_NO_VOLUME)
        rc = VINF_SUCCESS;

    RTTESTI_CHECK(rc == VINF_SUCCESS);

    RTDvmRelease(hVolMgr);

    return rc;
}
#endif

int main(int argc, char **argv)
{
    /*
     * Initialize IPRT and create the test.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTDvm", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    /*
     * If no args, display usage.
     */
    if (argc < 2)
    {
        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "Syntax: %s <image>\n", argv[0]);
        return RTTestSkipAndDestroy(hTest, "Missing required arguments\n");
    }

#if 1
    RTTestFailed(hTest, "Needs updating to RTDvm API changes!");
#else
    /* Open image. */
    RTFILE hFile;
    uint64_t cb = 0;
    rc = RTFileOpen(&hFile, argv[1], RTFILE_O_OPEN | RTFILE_O_DENY_NONE | RTFILE_O_READWRITE);
    if (RT_FAILURE(rc))
    {
        RTTestIFailed("RTFileOpen -> %Rrc", rc);
        return RTTestSummaryAndDestroy(hTest);
    }

    rc = RTFileGetSize(hFile, &cb);
    if (   RT_FAILURE(rc)
        || cb % 512 != 0) /* Assume 512 byte sector size. */
    {
        RTTestIFailed("RTFileGetSize -> %Rrc", rc);
        return RTTestSummaryAndDestroy(hTest);
    }

    TSTRTDVMDISK Disk;

    Disk.fUseImage = true;
    Disk.hImage    = hFile;
    rc = tstRTDvmVolume(hTest, &Disk, cb, 0);

    RTTESTI_CHECK(rc == VINF_SUCCESS);

    /*
     * Summary
     */
#endif
    return RTTestSummaryAndDestroy(hTest);
}


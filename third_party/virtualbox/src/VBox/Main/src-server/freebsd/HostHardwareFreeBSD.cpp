/* $Id: HostHardwareFreeBSD.cpp $ */
/** @file
 * Classes for handling hardware detection under FreeBSD.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_MAIN


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <HostHardwareLinux.h>

#include <VBox/log.h>

#include <iprt/dir.h>
#include <iprt/env.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>

#ifdef RT_OS_FREEBSD
# include <sys/param.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <stdio.h>
# include <sys/ioctl.h>
# include <fcntl.h>
# include <cam/cam.h>
# include <cam/cam_ccb.h>
# include <cam/scsi/scsi_pass.h>
#endif /* RT_OS_FREEBSD */
#include <vector>


/*********************************************************************************************************************************
*   Typedefs and Defines                                                                                                         *
*********************************************************************************************************************************/

static int getDriveInfoFromEnv(const char *pcszVar, DriveInfoList *pList,
                               bool isDVD, bool *pfSuccess);
static int getDVDInfoFromCAM(DriveInfoList *pList, bool *pfSuccess);

/** Find the length of a string, ignoring trailing non-ascii or control
 * characters */
static size_t strLenStripped(const char *pcsz)
{
    size_t cch = 0;
    for (size_t i = 0; pcsz[i] != '\0'; ++i)
        if (pcsz[i] > 32 && pcsz[i] < 127)
            cch = i;
    return cch + 1;
}

static void strLenRemoveTrailingWhiteSpace(char *psz, size_t cchStr)
{
    while (   (cchStr > 0)
           && (psz[cchStr -1] == ' '))
        psz[--cchStr] = '\0';
}

/**
 * Initialise the device description for a DVD drive based on
 * vendor and model name strings.
 * @param pcszVendor  the vendor ID string
 * @param pcszModel   the product ID string
 * @param pszDesc    where to store the description string (optional)
 * @param cchDesc    the size of the buffer in @pszDesc
 */
/* static */
void dvdCreateDeviceString(const char *pcszVendor, const char *pcszModel,
                            char *pszDesc, size_t cchDesc)
{
    AssertPtrReturnVoid(pcszVendor);
    AssertPtrReturnVoid(pcszModel);
    AssertPtrNullReturnVoid(pszDesc);
    AssertReturnVoid(!pszDesc || cchDesc > 0);
    size_t cchVendor = strLenStripped(pcszVendor);
    size_t cchModel = strLenStripped(pcszModel);

    /* Construct the description string as "Vendor Product" */
    if (pszDesc)
    {
        if (cchVendor > 0)
            RTStrPrintf(pszDesc, cchDesc, "%.*s %s", cchVendor, pcszVendor,
                        cchModel > 0 ? pcszModel : "(unknown drive model)");
        else
            RTStrPrintf(pszDesc, cchDesc, "%s", pcszModel);
    }
}


int VBoxMainDriveInfo::updateDVDs ()
{
    LogFlowThisFunc(("entered\n"));
    int rc = VINF_SUCCESS;
    bool fSuccess = false;  /* Have we succeeded in finding anything yet? */

    try
    {
        mDVDList.clear ();
        /* Always allow the user to override our auto-detection using an
         * environment variable. */
        if (RT_SUCCESS(rc) && !fSuccess)
            rc = getDriveInfoFromEnv("VBOX_CDROM", &mDVDList, true /* isDVD */,
                                     &fSuccess);
        if (RT_SUCCESS(rc) && !fSuccess)
            rc = getDVDInfoFromCAM(&mDVDList, &fSuccess);
    }
    catch(std::bad_alloc &e)
    {
        rc = VERR_NO_MEMORY;
    }
    LogFlowThisFunc(("rc=%Rrc\n", rc));
    return rc;
}

int VBoxMainDriveInfo::updateFloppies ()
{
    LogFlowThisFunc(("entered\n"));
    int rc = VINF_SUCCESS;
    bool fSuccess = false;  /* Have we succeeded in finding anything yet? */

    try
    {
        mFloppyList.clear ();
        /* Always allow the user to override our auto-detection using an
         * environment variable. */
        if (RT_SUCCESS(rc) && !fSuccess)
            rc = getDriveInfoFromEnv("VBOX_FLOPPY", &mFloppyList, false /* isDVD */,
                                     &fSuccess);
    }
    catch(std::bad_alloc &e)
    {
        rc = VERR_NO_MEMORY;
    }
    LogFlowThisFunc(("rc=%Rrc\n", rc));
    return rc;
}

/**
 * Search for available CD/DVD drives using the CAM layer.
 *
 * @returns iprt status code
 * @param   pList      the list to append the drives found to
 * @param   pfSuccess  this will be set to true if we found at least one drive
 *                     and to false otherwise.  Optional.
 */
static int getDVDInfoFromCAM(DriveInfoList *pList, bool *pfSuccess)
{
    int rc = VINF_SUCCESS;
    RTFILE FileXpt;

    rc = RTFileOpen(&FileXpt, "/dev/xpt0", RTFILE_O_READWRITE | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
    if (RT_SUCCESS(rc))
    {
        union ccb DeviceCCB;
        struct dev_match_pattern DeviceMatchPattern;
        struct dev_match_result *paMatches = NULL;

        RT_ZERO(DeviceCCB);
        RT_ZERO(DeviceMatchPattern);

        /* We want to get all devices. */
        DeviceCCB.ccb_h.func_code  = XPT_DEV_MATCH;
        DeviceCCB.ccb_h.path_id    = CAM_XPT_PATH_ID;
        DeviceCCB.ccb_h.target_id  = CAM_TARGET_WILDCARD;
        DeviceCCB.ccb_h.target_lun = CAM_LUN_WILDCARD;

        /* Setup the pattern */
        DeviceMatchPattern.type = DEV_MATCH_DEVICE;
        DeviceMatchPattern.pattern.device_pattern.path_id    = CAM_XPT_PATH_ID;
        DeviceMatchPattern.pattern.device_pattern.target_id  = CAM_TARGET_WILDCARD;
        DeviceMatchPattern.pattern.device_pattern.target_lun = CAM_LUN_WILDCARD;
        DeviceMatchPattern.pattern.device_pattern.flags      = DEV_MATCH_INQUIRY;

#if __FreeBSD_version >= 900000
# define INQ_PAT data.inq_pat
#else
 #define INQ_PAT inq_pat
#endif
        DeviceMatchPattern.pattern.device_pattern.INQ_PAT.type = T_CDROM;
        DeviceMatchPattern.pattern.device_pattern.INQ_PAT.media_type  = SIP_MEDIA_REMOVABLE | SIP_MEDIA_FIXED;
        DeviceMatchPattern.pattern.device_pattern.INQ_PAT.vendor[0]   = '*'; /* Matches anything */
        DeviceMatchPattern.pattern.device_pattern.INQ_PAT.product[0]  = '*'; /* Matches anything */
        DeviceMatchPattern.pattern.device_pattern.INQ_PAT.revision[0] = '*'; /* Matches anything */
#undef INQ_PAT
        DeviceCCB.cdm.num_patterns    = 1;
        DeviceCCB.cdm.pattern_buf_len = sizeof(struct dev_match_result);
        DeviceCCB.cdm.patterns        = &DeviceMatchPattern;

        /*
         * Allocate the buffer holding the matches.
         * We will allocate for 10 results and call
         * CAM multiple times if we have more results.
         */
        paMatches = (struct dev_match_result *)RTMemAllocZ(10 * sizeof(struct dev_match_result));
        if (paMatches)
        {
            DeviceCCB.cdm.num_matches   = 0;
            DeviceCCB.cdm.match_buf_len = 10 * sizeof(struct dev_match_result);
            DeviceCCB.cdm.matches       = paMatches;

            do
            {
                rc = RTFileIoCtl(FileXpt, CAMIOCOMMAND, &DeviceCCB, sizeof(union ccb), NULL);
                if (RT_FAILURE(rc))
                {
                    Log(("Error while querying available CD/DVD devices rc=%Rrc\n", rc));
                    break;
                }

                for (unsigned i = 0; i < DeviceCCB.cdm.num_matches; i++)
                {
                    if (paMatches[i].type == DEV_MATCH_DEVICE)
                    {
                        /* We have the drive now but need the appropriate device node */
                        struct device_match_result *pDevResult = &paMatches[i].result.device_result;
                        union ccb PeriphCCB;
                        struct dev_match_pattern PeriphMatchPattern;
                        struct dev_match_result aPeriphMatches[2];
                        struct periph_match_result *pPeriphResult = NULL;
                        unsigned iPeriphMatch = 0;

                        RT_ZERO(PeriphCCB);
                        RT_ZERO(PeriphMatchPattern);
                        RT_ZERO(aPeriphMatches);

                        /* This time we only want the specific nodes for the device. */
                        PeriphCCB.ccb_h.func_code  = XPT_DEV_MATCH;
                        PeriphCCB.ccb_h.path_id    = paMatches[i].result.device_result.path_id;
                        PeriphCCB.ccb_h.target_id  = paMatches[i].result.device_result.target_id;
                        PeriphCCB.ccb_h.target_lun = paMatches[i].result.device_result.target_lun;

                        /* Setup the pattern */
                        PeriphMatchPattern.type = DEV_MATCH_PERIPH;
                        PeriphMatchPattern.pattern.periph_pattern.path_id    = paMatches[i].result.device_result.path_id;
                        PeriphMatchPattern.pattern.periph_pattern.target_id  = paMatches[i].result.device_result.target_id;
                        PeriphMatchPattern.pattern.periph_pattern.target_lun = paMatches[i].result.device_result.target_lun;
                        PeriphMatchPattern.pattern.periph_pattern.flags      = PERIPH_MATCH_PATH | PERIPH_MATCH_TARGET |
                                                                               PERIPH_MATCH_LUN;
                        PeriphCCB.cdm.num_patterns    = 1;
                        PeriphCCB.cdm.pattern_buf_len = sizeof(struct dev_match_result);
                        PeriphCCB.cdm.patterns        = &PeriphMatchPattern;
                        PeriphCCB.cdm.num_matches   = 0;
                        PeriphCCB.cdm.match_buf_len = sizeof(aPeriphMatches);
                        PeriphCCB.cdm.matches       = aPeriphMatches;

                        do
                        {
                            rc = RTFileIoCtl(FileXpt, CAMIOCOMMAND, &PeriphCCB, sizeof(union ccb), NULL);
                            if (RT_FAILURE(rc))
                            {
                                Log(("Error while querying available periph devices rc=%Rrc\n", rc));
                                break;
                            }

                            for (iPeriphMatch = 0; iPeriphMatch < PeriphCCB.cdm.num_matches; iPeriphMatch++)
                            {
                                if (   (aPeriphMatches[iPeriphMatch].type == DEV_MATCH_PERIPH)
                                    && (!strcmp(aPeriphMatches[iPeriphMatch].result.periph_result.periph_name, "cd")))
                                {
                                    pPeriphResult = &aPeriphMatches[iPeriphMatch].result.periph_result;
                                    break; /* We found the periph device */
                                }
                            }

                            if (iPeriphMatch < PeriphCCB.cdm.num_matches)
                                break;

                        } while (   (DeviceCCB.ccb_h.status == CAM_REQ_CMP)
                                 && (DeviceCCB.cdm.status == CAM_DEV_MATCH_MORE));

                        if (pPeriphResult)
                        {
                            char szPath[RTPATH_MAX];
                            char szDesc[256];

                            RTStrPrintf(szPath, sizeof(szPath), "/dev/%s%d",
                                        pPeriphResult->periph_name, pPeriphResult->unit_number);

                            /* Remove trailing white space. */
                            strLenRemoveTrailingWhiteSpace(pDevResult->inq_data.vendor,
                                                            sizeof(pDevResult->inq_data.vendor));
                            strLenRemoveTrailingWhiteSpace(pDevResult->inq_data.product,
                                                            sizeof(pDevResult->inq_data.product));

                            dvdCreateDeviceString(pDevResult->inq_data.vendor,
                                                    pDevResult->inq_data.product,
                                                    szDesc, sizeof(szDesc));

                            pList->push_back(DriveInfo(szPath, "", szDesc));
                            if (pfSuccess)
                                *pfSuccess = true;
                        }
                    }
                }
            } while (   (DeviceCCB.ccb_h.status == CAM_REQ_CMP)
                     && (DeviceCCB.cdm.status == CAM_DEV_MATCH_MORE));

            RTMemFree(paMatches);
        }
        else
            rc = VERR_NO_MEMORY;

        RTFileClose(FileXpt);
    }

    return rc;
}

/**
 * Extract the names of drives from an environment variable and add them to a
 * list if they are valid.
 * @returns iprt status code
 * @param   pcszVar     the name of the environment variable.  The variable
 *                     value should be a list of device node names, separated
 *                     by ':' characters.
 * @param   pList      the list to append the drives found to
 * @param   isDVD      are we looking for DVD drives or for floppies?
 * @param   pfSuccess  this will be set to true if we found at least one drive
 *                     and to false otherwise.  Optional.
 */
static int getDriveInfoFromEnv(const char *pcszVar, DriveInfoList *pList,
                               bool isDVD, bool *pfSuccess)
{
    AssertPtrReturn(pcszVar, VERR_INVALID_POINTER);
    AssertPtrReturn(pList, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pfSuccess, VERR_INVALID_POINTER);
    LogFlowFunc(("pcszVar=%s, pList=%p, isDVD=%d, pfSuccess=%p\n", pcszVar,
                 pList, isDVD, pfSuccess));
    int rc = VINF_SUCCESS;
    bool success = false;
    char *pszFreeMe = RTEnvDupEx(RTENV_DEFAULT, pcszVar);

    try
    {
        const char *pcszCurrent = pszFreeMe;
        while (pcszCurrent && *pcszCurrent != '\0')
        {
            const char *pcszNext = strchr(pcszCurrent, ':');
            char szPath[RTPATH_MAX], szReal[RTPATH_MAX];
            char szDesc[256], szUdi[256];
            if (pcszNext)
                RTStrPrintf(szPath, sizeof(szPath), "%.*s",
                            pcszNext - pcszCurrent - 1, pcszCurrent);
            else
                RTStrPrintf(szPath, sizeof(szPath), "%s", pcszCurrent);
            if (RT_SUCCESS(RTPathReal(szPath, szReal, sizeof(szReal))))
            {
                szUdi[0] = '\0'; /** @todo r=bird: missing a call to devValidateDevice() here and szUdi wasn't
                                  *        initialized because of that.  Need proper fixing. */
                pList->push_back(DriveInfo(szReal, szUdi, szDesc));
                success = true;
            }
            pcszCurrent = pcszNext ? pcszNext + 1 : NULL;
        }
        if (pfSuccess != NULL)
            *pfSuccess = success;
    }
    catch(std::bad_alloc &e)
    {
        rc = VERR_NO_MEMORY;
    }
    RTStrFree(pszFreeMe);
    LogFlowFunc(("rc=%Rrc, success=%d\n", rc, success));
    return rc;
}

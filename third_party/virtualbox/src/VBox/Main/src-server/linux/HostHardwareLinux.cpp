/* $Id: HostHardwareLinux.cpp $ */
/** @file
 * Classes for handling hardware detection under Linux.  Please feel free to
 * expand these to work for other systems (Solaris!) or to add new ones for
 * other systems.
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
#include <vector.h>

#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/asm.h>
#include <iprt/dir.h>
#include <iprt/env.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>

#include <linux/cdrom.h>
#include <linux/fd.h>
#include <linux/major.h>
#include <scsi/scsi.h>

#include <iprt/linux/sysfs.h>

#ifdef VBOX_USB_WITH_SYSFS
# ifdef VBOX_USB_WITH_INOTIFY
#  include <dlfcn.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <signal.h>
#  include <unistd.h>
# endif
#endif

#include <vector>

#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysmacros.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

#ifdef TESTCASE
static bool testing() { return true; }
static bool fNoProbe = false;
static bool noProbe() { return fNoProbe; }
static void setNoProbe(bool val) { fNoProbe = val; }
#else
static bool testing() { return false; }
static bool noProbe() { return false; }
static void setNoProbe(bool val) { (void)val; }
#endif


/*********************************************************************************************************************************
*   Typedefs and Defines                                                                                                         *
*********************************************************************************************************************************/

static int getDriveInfoFromEnv(const char *pcszVar, DriveInfoList *pList,
                               bool isDVD, bool *pfSuccess);
static int getDriveInfoFromSysfs(DriveInfoList *pList, bool isDVD,
                                 bool *pfSuccess);

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


/**
 * Get the name of a floppy drive according to the Linux floppy driver.
 * @returns true on success, false if the name was not available (i.e. the
 *          device was not readable, or the file name wasn't a PC floppy
 *          device)
 * @param  pcszNode  the path to the device node for the device
 * @param  Number    the Linux floppy driver number for the drive.  Required.
 * @param  pszName   where to store the name retrieved
 */
static bool floppyGetName(const char *pcszNode, unsigned Number,
                          floppy_drive_name pszName)
{
    AssertPtrReturn(pcszNode, false);
    AssertPtrReturn(pszName, false);
    AssertReturn(Number <= 7, false);
    RTFILE File;
    int rc = RTFileOpen(&File, pcszNode, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE | RTFILE_O_NON_BLOCK);
    if (RT_SUCCESS(rc))
    {
        int rcIoCtl;
        rc = RTFileIoCtl(File, FDGETDRVTYP, pszName, 0, &rcIoCtl);
        RTFileClose(File);
        if (RT_SUCCESS(rc) && rcIoCtl >= 0)
            return true;
    }
    return false;
}


/**
 * Create a UDI and a description for a floppy drive based on a number and the
 * driver's name for it.  We deliberately return an ugly sequence of
 * characters as the description rather than an English language string to
 * avoid translation issues.
 *
 * @returns true if we know the device to be valid, false otherwise
 * @param   pcszName     the floppy driver name for the device (optional)
 * @param   Number       the number of the floppy (0 to 3 on FDC 0, 4 to 7 on
 *                       FDC 1)
 * @param   pszDesc      where to store the device description (optional)
 * @param   cchDesc      the size of the buffer in @a pszDesc
 * @param   pszUdi       where to store the device UDI (optional)
 * @param   cchUdi       the size of the buffer in @a pszUdi
 */
static void floppyCreateDeviceStrings(const floppy_drive_name pcszName,
                                      unsigned Number, char *pszDesc,
                                      size_t cchDesc, char *pszUdi,
                                      size_t cchUdi)
{
    AssertPtrNullReturnVoid(pcszName);
    AssertPtrNullReturnVoid(pszDesc);
    AssertReturnVoid(!pszDesc || cchDesc > 0);
    AssertPtrNullReturnVoid(pszUdi);
    AssertReturnVoid(!pszUdi || cchUdi > 0);
    AssertReturnVoid(Number <= 7);
    if (pcszName)
    {
        const char *pcszSize;
        switch(pcszName[0])
        {
            case 'd': case 'q': case 'h':
                pcszSize = "5.25\"";
                break;
            case 'D': case 'H': case 'E': case 'u':
                pcszSize = "3.5\"";
                break;
            default:
                pcszSize = "(unknown)";
        }
        if (pszDesc)
            RTStrPrintf(pszDesc, cchDesc, "%s %s K%s", pcszSize, &pcszName[1],
                        Number > 3 ? ", FDC 2" : "");
    }
    else
    {
        if (pszDesc)
            RTStrPrintf(pszDesc, cchDesc, "FDD %d%s", (Number & 4) + 1,
                        Number > 3 ? ", FDC 2" : "");
    }
    if (pszUdi)
        RTStrPrintf(pszUdi, cchUdi,
                    "/org/freedesktop/Hal/devices/platform_floppy_%u_storage",
                    Number);
}


/**
 * Check whether a device number might correspond to a CD-ROM device according
 * to Documentation/devices.txt in the Linux kernel source.
 * @returns true if it might, false otherwise
 * @param   Number  the device number (major and minor combination)
 */
static bool isCdromDevNum(dev_t Number)
{
    int major = major(Number);
    int minor = minor(Number);
    if ((major == IDE0_MAJOR) && !(minor & 0x3f))
        return true;
    if (major == SCSI_CDROM_MAJOR)
        return true;
    if (major == CDU31A_CDROM_MAJOR)
        return true;
    if (major == GOLDSTAR_CDROM_MAJOR)
        return true;
    if (major == OPTICS_CDROM_MAJOR)
        return true;
    if (major == SANYO_CDROM_MAJOR)
        return true;
    if (major == MITSUMI_X_CDROM_MAJOR)
        return true;
    if ((major == IDE1_MAJOR) && !(minor & 0x3f))
        return true;
    if (major == MITSUMI_CDROM_MAJOR)
        return true;
    if (major == CDU535_CDROM_MAJOR)
        return true;
    if (major == MATSUSHITA_CDROM_MAJOR)
        return true;
    if (major == MATSUSHITA_CDROM2_MAJOR)
        return true;
    if (major == MATSUSHITA_CDROM3_MAJOR)
        return true;
    if (major == MATSUSHITA_CDROM4_MAJOR)
        return true;
    if (major == AZTECH_CDROM_MAJOR)
        return true;
    if (major == 30 /* CM205_CDROM_MAJOR */)  /* no #define for some reason */
        return true;
    if (major == CM206_CDROM_MAJOR)
        return true;
    if ((major == IDE3_MAJOR) && !(minor & 0x3f))
        return true;
    if (major == 46 /* Parallel port ATAPI CD-ROM */)  /* no #define */
        return true;
    if ((major == IDE4_MAJOR) && !(minor & 0x3f))
        return true;
    if ((major == IDE5_MAJOR) && !(minor & 0x3f))
        return true;
    if ((major == IDE6_MAJOR) && !(minor & 0x3f))
        return true;
    if ((major == IDE7_MAJOR) && !(minor & 0x3f))
        return true;
    if ((major == IDE8_MAJOR) && !(minor & 0x3f))
        return true;
    if ((major == IDE9_MAJOR) && !(minor & 0x3f))
        return true;
    if (major == 113 /* VIOCD_MAJOR */)
        return true;
    return false;
}


/**
 * Send an SCSI INQUIRY command to a device and return selected information.
 * @returns  iprt status code
 * @returns  VERR_TRY_AGAIN if the query failed but might succeed next time
 * @param pcszNode    the full path to the device node
 * @param pu8Type    where to store the SCSI device type on success (optional)
 * @param pchVendor  where to store the vendor id string on success (optional)
 * @param cchVendor  the size of the @a pchVendor buffer
 * @param pchModel   where to store the product id string on success (optional)
 * @param cchModel   the size of the @a pchModel buffer
 * @note check documentation on the SCSI INQUIRY command and the Linux kernel
 *       SCSI headers included above if you want to understand what is going
 *       on in this method.
 */
static int cdromDoInquiry(const char *pcszNode, uint8_t *pu8Type,
                          char *pchVendor, size_t cchVendor, char *pchModel,
                          size_t cchModel)
{
    LogRelFlowFunc(("pcszNode=%s, pu8Type=%p, pchVendor=%p, cchVendor=%llu, pchModel=%p, cchModel=%llu\n",
                    pcszNode, pu8Type, pchVendor, cchVendor, pchModel,
                    cchModel));
    AssertPtrReturn(pcszNode, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pu8Type, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pchVendor, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pchModel, VERR_INVALID_POINTER);

    RTFILE hFile;
    int rc = RTFileOpen(&hFile, pcszNode, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE | RTFILE_O_NON_BLOCK);
    if (RT_SUCCESS(rc))
    {
        int                             rcIoCtl        = 0;
        unsigned char                   u8Response[96] = { 0 };
        struct cdrom_generic_command    CdromCommandReq;
        RT_ZERO(CdromCommandReq);
        CdromCommandReq.cmd[0]         = INQUIRY;
        CdromCommandReq.cmd[4]         = sizeof(u8Response);
        CdromCommandReq.buffer         = u8Response;
        CdromCommandReq.buflen         = sizeof(u8Response);
        CdromCommandReq.data_direction = CGC_DATA_READ;
        CdromCommandReq.timeout        = 5000;  /* ms */
        rc = RTFileIoCtl(hFile, CDROM_SEND_PACKET, &CdromCommandReq, 0, &rcIoCtl);
        if (RT_SUCCESS(rc) && rcIoCtl < 0)
            rc = RTErrConvertFromErrno(-CdromCommandReq.stat);
        RTFileClose(hFile);

        if (RT_SUCCESS(rc))
        {
            if (pu8Type)
                *pu8Type = u8Response[0] & 0x1f;
            if (pchVendor)
                RTStrPrintf(pchVendor, cchVendor, "%.8s",
                            &u8Response[8] /* vendor id string */);
            if (pchModel)
                RTStrPrintf(pchModel, cchModel, "%.16s",
                            &u8Response[16] /* product id string */);
            LogRelFlowFunc(("returning success: type=%u, vendor=%.8s, product=%.16s\n",
                            u8Response[0] & 0x1f, &u8Response[8], &u8Response[16]));
            return VINF_SUCCESS;
        }
    }
    LogRelFlowFunc(("returning %Rrc\n", rc));
    return rc;
}


/**
 * Initialise the device strings (description and UDI) for a DVD drive based on
 * vendor and model name strings.
 * @param pcszVendor  the vendor ID string
 * @param pcszModel   the product ID string
 * @param pszDesc    where to store the description string (optional)
 * @param cchDesc    the size of the buffer in @a pszDesc
 * @param pszUdi     where to store the UDI string (optional)
 * @param cchUdi     the size of the buffer in @a pszUdi
 */
/* static */
void dvdCreateDeviceStrings(const char *pcszVendor, const char *pcszModel,
                            char *pszDesc, size_t cchDesc, char *pszUdi,
                            size_t cchUdi)
{
    AssertPtrReturnVoid(pcszVendor);
    AssertPtrReturnVoid(pcszModel);
    AssertPtrNullReturnVoid(pszDesc);
    AssertReturnVoid(!pszDesc || cchDesc > 0);
    AssertPtrNullReturnVoid(pszUdi);
    AssertReturnVoid(!pszUdi || cchUdi > 0);
    char szCleaned[128];
    size_t cchVendor = strLenStripped(pcszVendor);
    size_t cchModel = strLenStripped(pcszModel);

    /* Create a cleaned version of the model string for the UDI string. */
    for (unsigned i = 0; i < sizeof(szCleaned) && pcszModel[i] != '\0'; ++i)
        if (   (pcszModel[i] >= '0' && pcszModel[i] <= '9')
            || (pcszModel[i] >= 'A' && pcszModel[i] <= 'z'))
            szCleaned[i] = pcszModel[i];
        else
            szCleaned[i] = '_';
    szCleaned[RT_MIN(cchModel, sizeof(szCleaned) - 1)] = '\0';

    /* Construct the description string as "Vendor Product" */
    if (pszDesc)
    {
        if (cchVendor > 0)
            RTStrPrintf(pszDesc, cchDesc, "%.*s %s", cchVendor, pcszVendor,
                        cchModel > 0 ? pcszModel : "(unknown drive model)");
        else
            RTStrPrintf(pszDesc, cchDesc, "%s", pcszModel);
    }
    /* Construct the UDI string */
    if (pszUdi)
    {
        if (cchModel > 0)
            RTStrPrintf(pszUdi, cchUdi,
                        "/org/freedesktop/Hal/devices/storage_model_%s",
                        szCleaned);
        else
            pszUdi[0] = '\0';
    }
}


/**
 * Check whether a device node points to a valid device and create a UDI and
 * a description for it, and store the device number, if it does.
 * @returns true if the device is valid, false otherwise
 * @param   pcszNode   the path to the device node
 * @param   isDVD     are we looking for a DVD device (or a floppy device)?
 * @param   pDevice   where to store the device node (optional)
 * @param   pszDesc   where to store the device description (optional)
 * @param   cchDesc   the size of the buffer in @a pszDesc
 * @param   pszUdi    where to store the device UDI (optional)
 * @param   cchUdi    the size of the buffer in @a pszUdi
 */
static bool devValidateDevice(const char *pcszNode, bool isDVD, dev_t *pDevice,
                              char *pszDesc, size_t cchDesc, char *pszUdi,
                              size_t cchUdi)
{
    AssertPtrReturn(pcszNode, false);
    AssertPtrNullReturn(pDevice, false);
    AssertPtrNullReturn(pszDesc, false);
    AssertReturn(!pszDesc || cchDesc > 0, false);
    AssertPtrNullReturn(pszUdi, false);
    AssertReturn(!pszUdi || cchUdi > 0, false);
    RTFSOBJINFO ObjInfo;
    if (RT_FAILURE(RTPathQueryInfo(pcszNode, &ObjInfo, RTFSOBJATTRADD_UNIX)))
        return false;
    if (!RTFS_IS_DEV_BLOCK(ObjInfo.Attr.fMode))
        return false;
    if (pDevice)
        *pDevice = ObjInfo.Attr.u.Unix.Device;
    if (isDVD)
    {
        char szVendor[128], szModel[128];
        uint8_t u8Type;
        if (!isCdromDevNum(ObjInfo.Attr.u.Unix.Device))
            return false;
        if (RT_FAILURE(cdromDoInquiry(pcszNode, &u8Type,
                                      szVendor, sizeof(szVendor),
                                      szModel, sizeof(szModel))))
            return false;
        if (u8Type != TYPE_ROM)
            return false;
        dvdCreateDeviceStrings(szVendor, szModel, pszDesc, cchDesc,
                               pszUdi, cchUdi);
    }
    else
    {
        /* Floppies on Linux are legacy devices with hardcoded majors and
         * minors */
        unsigned Number;
        floppy_drive_name szName;
        if (major(ObjInfo.Attr.u.Unix.Device) != FLOPPY_MAJOR)
            return false;
        switch (minor(ObjInfo.Attr.u.Unix.Device))
        {
            case 0: case 1: case 2: case 3:
                Number = minor(ObjInfo.Attr.u.Unix.Device);
                break;
            case 128: case 129: case 130: case 131:
                Number = minor(ObjInfo.Attr.u.Unix.Device) - 128 + 4;
                break;
            default:
                return false;
        }
        if (!floppyGetName(pcszNode, Number, szName))
            return false;
        floppyCreateDeviceStrings(szName, Number, pszDesc, cchDesc, pszUdi,
                                  cchUdi);
    }
    return true;
}


int VBoxMainDriveInfo::updateDVDs ()
{
    LogFlowThisFunc(("entered\n"));
    int rc = VINF_SUCCESS;
    bool success = false;  /* Have we succeeded in finding anything yet? */
    try
    {
        mDVDList.clear ();
        /* Always allow the user to override our auto-detection using an
         * environment variable. */
        if (RT_SUCCESS(rc) && (!success || testing()))
            rc = getDriveInfoFromEnv ("VBOX_CDROM", &mDVDList, true /* isDVD */,
                                      &success);
        setNoProbe(false);
        if (RT_SUCCESS(rc) && (!success || testing()))
            rc = getDriveInfoFromSysfs(&mDVDList, true /* isDVD */, &success);
        if (RT_SUCCESS(rc) && testing())
        {
            setNoProbe(true);
            rc = getDriveInfoFromSysfs(&mDVDList, true /* isDVD */, &success);
        }
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
    bool success = false;  /* Have we succeeded in finding anything yet? */
    try
    {
        mFloppyList.clear ();
        if (RT_SUCCESS(rc) && (!success || testing()))
            rc = getDriveInfoFromEnv("VBOX_FLOPPY", &mFloppyList,
                                     false /* isDVD */, &success);
        setNoProbe(false);
        if (   RT_SUCCESS(rc) && (!success || testing()))
            rc = getDriveInfoFromSysfs(&mFloppyList, false /* isDVD */,
                                       &success);
        if (RT_SUCCESS(rc) && testing())
        {
            setNoProbe(true);
            rc = getDriveInfoFromSysfs(&mFloppyList, false /* isDVD */, &success);
        }
    }
    catch(std::bad_alloc &e)
    {
        rc = VERR_NO_MEMORY;
    }
    LogFlowThisFunc(("rc=%Rrc\n", rc));
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
/* static */
int getDriveInfoFromEnv(const char *pcszVar, DriveInfoList *pList,
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
            if (   RT_SUCCESS(RTPathReal(szPath, szReal, sizeof(szReal)))
                && devValidateDevice(szReal, isDVD, NULL, szDesc,
                                     sizeof(szDesc), szUdi, sizeof(szUdi)))
            {
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


class sysfsBlockDev
{
public:
    sysfsBlockDev(const char *pcszName, bool wantDVD)
            : mpcszName(pcszName), mwantDVD(wantDVD), misConsistent(true),
              misValid(false)
    {
        if (findDeviceNode())
        {
            if (mwantDVD)
                validateAndInitForDVD();
            else
                validateAndInitForFloppy();
        }
    }
private:
    /** The name of the subdirectory of /sys/block for this device */
    const char *mpcszName;
    /** Are we looking for a floppy or a DVD device? */
    bool mwantDVD;
    /** The device node for the device */
    char mszNode[RTPATH_MAX];
    /** Does the sysfs entry look like we expect it too?  This is a canary
     * for future sysfs ABI changes. */
    bool misConsistent;
    /** Is this entry a valid specimen of what we are looking for? */
    bool misValid;
    /** Human readable drive description string */
    char mszDesc[256];
    /** Unique identifier for the drive.  Should be identical to hal's UDI for
     * the device.  May not be unique for two identical drives. */
    char mszUdi[256];
private:
    /* Private methods */

    /**
     * Fill in the device node member based on the /sys/block subdirectory.
     * @returns boolean success value
     */
    bool findDeviceNode()
    {
        dev_t dev = 0;
        int rc = RTLinuxSysFsReadDevNumFile(&dev, "block/%s/dev", mpcszName);
        if (RT_FAILURE(rc) || dev == 0)
        {
            misConsistent = false;
            return false;
        }
        rc = RTLinuxCheckDevicePath(dev, RTFS_TYPE_DEV_BLOCK, mszNode,
                                    sizeof(mszNode), "%s", mpcszName);
        if (RT_FAILURE(rc))
            return false;
        return true;
    }

    /** Check whether the sysfs block entry is valid for a DVD device and
     * initialise the string data members for the object.  We try to get all
     * the information we need from sysfs if possible, to avoid unnecessarily
     * poking the device, and if that fails we fall back to an SCSI INQUIRY
     * command. */
    void validateAndInitForDVD()
    {
        char szVendor[128], szModel[128];
        int64_t type = 0;
        int rc = RTLinuxSysFsReadIntFile(10, &type, "block/%s/device/type", mpcszName);
        if (RT_SUCCESS(rc) && type != TYPE_ROM)
            return;
        if (type == TYPE_ROM)
        {
            rc = RTLinuxSysFsReadStrFile(szVendor, sizeof(szVendor), NULL,
                                         "block/%s/device/vendor", mpcszName);
            if (RT_SUCCESS(rc))
            {
                rc = RTLinuxSysFsReadStrFile(szModel, sizeof(szModel), NULL,
                                             "block/%s/device/model", mpcszName);
                if (RT_SUCCESS(rc))
                {
                    misValid = true;
                    dvdCreateDeviceStrings(szVendor, szModel,
                                           mszDesc, sizeof(mszDesc),
                                           mszUdi, sizeof(mszUdi));
                    return;
                }
            }
        }
        if (!noProbe())
            probeAndInitForDVD();
    }

    /** Try to find out whether a device is a DVD drive by sending it an
     * SCSI INQUIRY command.  If it is, initialise the string and validity
     * data members for the object based on the returned data.
     */
    void probeAndInitForDVD()
    {
        AssertReturnVoid(mszNode[0] != '\0');
        uint8_t u8Type = 0;
        char szVendor[128] = "";
        char szModel[128] = "";
        int rc = cdromDoInquiry(mszNode, &u8Type, szVendor,
                                sizeof(szVendor), szModel,
                                sizeof(szModel));
        if (RT_SUCCESS(rc) && (u8Type == TYPE_ROM))
        {
            misValid = true;
            dvdCreateDeviceStrings(szVendor, szModel, mszDesc, sizeof(mszDesc),
                                   mszUdi, sizeof(mszUdi));
        }
    }

    /** Check whether the sysfs block entry is valid for a floppy device and
     * initialise the string data members for the object.  Since we only
     * support floppies using the basic "floppy" driver, we check the driver
     * using the entry name and a driver-specific ioctl. */
    void validateAndInitForFloppy()
    {
        bool haveName = false;
        floppy_drive_name szName;
        char szDriver[8];
        if (   mpcszName[0] != 'f'
            || mpcszName[1] != 'd'
            || mpcszName[2] < '0'
            || mpcszName[2] > '7'
            || mpcszName[3] != '\0')
            return;
        if (!noProbe())
            haveName = floppyGetName(mszNode, mpcszName[2] - '0', szName);
        int rc = RTLinuxSysFsGetLinkDest(szDriver, sizeof(szDriver), NULL, "block/%s/%s",
                                         mpcszName, "device/driver");
        if (RT_SUCCESS(rc))
        {
            if (RTStrCmp(szDriver, "floppy"))
                return;
        }
        else if (!haveName)
            return;
        floppyCreateDeviceStrings(haveName ? szName : NULL,
                                  mpcszName[2] - '0', mszDesc,
                                  sizeof(mszDesc), mszUdi, sizeof(mszUdi));
        misValid = true;
    }

public:
    bool isConsistent()
    {
        return misConsistent;
    }
    bool isValid()
    {
        return misValid;
    }
    const char *getDesc()
    {
        return mszDesc;
    }
    const char *getUdi()
    {
        return mszUdi;
    }
    const char *getNode()
    {
        return mszNode;
    }
};

/**
 * Helper function to query the sysfs subsystem for information about DVD
 * drives attached to the system.
 * @returns iprt status code
 * @param   pList      where to add information about the drives detected
 * @param   isDVD      are we looking for DVDs or floppies?
 * @param   pfSuccess  Did we find anything?
 *
 * @returns IPRT status code
 */
/* static */
int getDriveInfoFromSysfs(DriveInfoList *pList, bool isDVD, bool *pfSuccess)
{
    AssertPtrReturn(pList, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pfSuccess, VERR_INVALID_POINTER); /* Valid or Null */
    LogFlowFunc (("pList=%p, isDVD=%u, pfSuccess=%p\n",
                  pList, (unsigned) isDVD, pfSuccess));
    RTDIR hDir;
    int rc;
    bool fSuccess = false;
    unsigned cFound = 0;

    if (!RTPathExists("/sys"))
        return VINF_SUCCESS;
    rc = RTDirOpen(&hDir, "/sys/block");
    /* This might mean that sysfs semantics have changed */
    AssertReturn(rc != VERR_FILE_NOT_FOUND, VINF_SUCCESS);
    fSuccess = true;
    if (RT_SUCCESS(rc))
    {
        for (;;)
        {
            RTDIRENTRY entry;
            rc = RTDirRead(hDir, &entry, NULL);
            Assert(rc != VERR_BUFFER_OVERFLOW);  /* Should never happen... */
            if (RT_FAILURE(rc))  /* Including overflow and no more files */
                break;
            if (entry.szName[0] == '.')
                continue;
            sysfsBlockDev dev(entry.szName, isDVD);
            /* This might mean that sysfs semantics have changed */
            AssertBreakStmt(dev.isConsistent(), fSuccess = false);
            if (!dev.isValid())
                continue;
            try
            {
                pList->push_back(DriveInfo(dev.getNode(), dev.getUdi(), dev.getDesc()));
            }
            catch(std::bad_alloc &e)
            {
                rc = VERR_NO_MEMORY;
                break;
            }
            ++cFound;
        }
        RTDirClose(hDir);
    }
    if (rc == VERR_NO_MORE_FILES)
        rc = VINF_SUCCESS;
    if (RT_FAILURE(rc))
        /* Clean up again */
        for (unsigned i = 0; i < cFound; ++i)
            pList->pop_back();
    if (pfSuccess)
        *pfSuccess = fSuccess;
    LogFlow (("rc=%Rrc, fSuccess=%u\n", rc, (unsigned) fSuccess));
    return rc;
}


/** Helper for readFilePathsFromDir().  Adds a path to the vector if it is not
 * NULL and not a dotfile (".", "..", ".*"). */
static int maybeAddPathToVector(const char *pcszPath, const char *pcszEntry,
                                VECTOR_PTR(char *) *pvecpchDevs)
{
    char *pszPath;

    if (!pcszPath)
        return 0;
    if (pcszEntry[0] == '.')
        return 0;
    pszPath = RTStrDup(pcszPath);
    if (!pszPath)
        return ENOMEM;
    if (RT_FAILURE(VEC_PUSH_BACK_PTR(pvecpchDevs, char *, pszPath)))
        return ENOMEM;
    return 0;
}

/** Helper for readFilePaths().  Adds the entries from the open directory
 * @a pDir to the vector @a pvecpchDevs using either the full path or the
 * realpath() and skipping hidden files and files on which realpath() fails. */
static int readFilePathsFromDir(const char *pcszPath, DIR *pDir,
                                VECTOR_PTR(char *) *pvecpchDevs, int withRealPath)
{
    struct dirent entry, *pResult;
    int err;

#if RT_GNUC_PREREQ(4, 6)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    for (err = readdir_r(pDir, &entry, &pResult); pResult;
         err = readdir_r(pDir, &entry, &pResult))
#if RT_GNUC_PREREQ(4, 6)
# pragma GCC diagnostic pop
#endif
    {
        /* We (implicitly) require that PATH_MAX be defined */
        char szPath[PATH_MAX + 1], szRealPath[PATH_MAX + 1], *pszPath;
        if (snprintf(szPath, sizeof(szPath), "%s/%s", pcszPath,
                     entry.d_name) < 0)
            return errno;
        if (withRealPath)
            pszPath = realpath(szPath, szRealPath);
        else
            pszPath = szPath;
        if ((err = maybeAddPathToVector(pszPath, entry.d_name, pvecpchDevs)))
            return err;
    }
    return err;
}


/**
 * Helper for walkDirectory to dump the names of a directory's entries into a
 * vector of char pointers.
 *
 * @returns zero on success or (positive) posix error value.
 * @param   pcszPath      the path to dump.
 * @param   pvecpchDevs   an empty vector of char pointers - must be cleaned up
 *                        by the caller even on failure.
 * @param   withRealPath  whether to canonicalise the filename with realpath
 */
static int readFilePaths(const char *pcszPath, VECTOR_PTR(char *) *pvecpchDevs,
                         int withRealPath)
{
    DIR *pDir;
    int err;

    AssertPtrReturn(pvecpchDevs, EINVAL);
    AssertReturn(VEC_SIZE_PTR(pvecpchDevs) == 0, EINVAL);
    AssertPtrReturn(pcszPath, EINVAL);

    pDir = opendir(pcszPath);
    if (!pDir)
        return RTErrConvertFromErrno(errno);
    err = readFilePathsFromDir(pcszPath, pDir, pvecpchDevs, withRealPath);
    if (closedir(pDir) < 0 && !err)
        err = errno;
    return RTErrConvertFromErrno(err);
}


class hotplugNullImpl : public VBoxMainHotplugWaiterImpl
{
public:
    hotplugNullImpl(const char *) {}
    virtual ~hotplugNullImpl (void) {}
    /** @copydoc VBoxMainHotplugWaiter::Wait */
    virtual int Wait (RTMSINTERVAL cMillies)
    {
        NOREF(cMillies);
        return VERR_NOT_SUPPORTED;
    }
    /** @copydoc VBoxMainHotplugWaiter::Interrupt */
    virtual void Interrupt (void) {}
    virtual int getStatus(void)
    {
        return VERR_NOT_SUPPORTED;
    }

};

#ifdef VBOX_USB_WITH_SYSFS
# ifdef VBOX_USB_WITH_INOTIFY
/** Class wrapper around an inotify watch (or a group of them to be precise).
 */
typedef struct inotifyWatch
{
    /** Pointer to the inotify_add_watch() glibc function/Linux API */
    int (*inotify_add_watch)(int, const char *, uint32_t);
    /** The native handle of the inotify fd. */
    int mhInotify;
} inotifyWatch;

/** The flags we pass to inotify - modify, create, delete, change permissions
 */
#define IN_FLAGS 0x306

static int iwAddWatch(inotifyWatch *pSelf, const char *pcszPath)
{
    errno = 0;
    if (  pSelf->inotify_add_watch(pSelf->mhInotify, pcszPath, IN_FLAGS) >= 0
        || (errno == EACCES))
        return VINF_SUCCESS;
    /* Other errors listed in the manpage can be treated as fatal */
    return RTErrConvertFromErrno(errno);
}

/** Object initialisation */
static int iwInit(inotifyWatch *pSelf)
{
    int (*inotify_init)(void);
    int fd, flags;
    int rc = VINF_SUCCESS;

    AssertPtr(pSelf);
    pSelf->mhInotify = -1;
    errno = 0;
    *(void **)(&inotify_init) = dlsym(RTLD_DEFAULT, "inotify_init");
    if (!inotify_init)
        return VERR_LDR_IMPORTED_SYMBOL_NOT_FOUND;
    *(void **)(&pSelf->inotify_add_watch)
                    = dlsym(RTLD_DEFAULT, "inotify_add_watch");
    if (!pSelf->inotify_add_watch)
        return VERR_LDR_IMPORTED_SYMBOL_NOT_FOUND;
    fd = inotify_init();
    if (fd < 0)
    {
        Assert(errno > 0);
        return RTErrConvertFromErrno(errno);
    }
    Assert(errno == 0);

    flags = fcntl(fd, F_GETFL, NULL);
    if (   flags < 0
        || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0
        || fcntl(fd, F_SETFD, FD_CLOEXEC) < 0 /* race here */)
    {
        Assert(errno > 0);
        rc = RTErrConvertFromErrno(errno);
    }
    if (RT_FAILURE(rc))
        close(fd);
    else
    {
        Assert(errno == 0);
        pSelf->mhInotify = fd;
    }
    return rc;
}

static void iwTerm(inotifyWatch *pSelf)
{
    AssertPtrReturnVoid(pSelf);
    if (pSelf->mhInotify != -1)
    {
        close(pSelf->mhInotify);
        pSelf->mhInotify = -1;
    }
}

static int iwGetFD(inotifyWatch *pSelf)
{
    AssertPtrReturn(pSelf, -1);
    return pSelf->mhInotify;
}

# define SYSFS_WAKEUP_STRING "Wake up!"

class hotplugInotifyImpl : public VBoxMainHotplugWaiterImpl
{
    /** Pipe used to interrupt wait(), the read end. */
    int mhWakeupPipeR;
    /** Pipe used to interrupt wait(), the write end. */
    int mhWakeupPipeW;
    /** The inotify watch set */
    inotifyWatch mWatches;
    /** Flag to mark that the Wait() method is currently being called, and to
     * ensure that it isn't called multiple times in parallel. */
    volatile uint32_t mfWaiting;
    /** The root of the USB devices tree. */
    const char *mpcszDevicesRoot;
    /** iprt result code from object initialisation.  Should be AssertReturn-ed
     * on at the start of all methods.  I went this way because I didn't want
     * to deal with exceptions. */
    int mStatus;
    /** ID values associates with the wakeup pipe and the FAM socket for polling
     */
    enum
    {
        RPIPE_ID = 0,
        INOTIFY_ID,
        MAX_POLLID
    };

    /** Clean up any resources in use, gracefully skipping over any which have
     * not yet been allocated or already cleaned up.  Intended to be called
     * from the destructor or after a failed initialisation. */
    void term(void);

    int drainInotify();

    /** Read the wakeup string from the wakeup pipe */
    int drainWakeupPipe(void);
public:
    hotplugInotifyImpl(const char *pcszDevicesRoot);
    virtual ~hotplugInotifyImpl(void)
    {
        term();
#ifdef DEBUG
        /** The first call to term should mark all resources as freed, so this
         * should be a semantic no-op. */
        term();
#endif
    }
    /** Is inotify available and working on this system?  If so we expect that
     * this implementation will be usable. */
    /** @todo test the "inotify in glibc but not in the kernel" case. */
    static bool Available(void)
    {
        int (*inotify_init)(void);

        *(void **)(&inotify_init) = dlsym(RTLD_DEFAULT, "inotify_init");
        if (!inotify_init)
            return false;
        int fd = inotify_init();
        if (fd == -1)
            return false;
        close(fd);
        return true;
    }

    virtual int getStatus(void)
    {
        return mStatus;
    }

    /** @copydoc VBoxMainHotplugWaiter::Wait */
    virtual int Wait(RTMSINTERVAL);
    /** @copydoc VBoxMainHotplugWaiter::Interrupt */
    virtual void Interrupt(void);
};

/** Simplified version of RTPipeCreate */
static int pipeCreateSimple(int *phPipeRead, int *phPipeWrite)
{
    AssertPtrReturn(phPipeRead, VERR_INVALID_POINTER);
    AssertPtrReturn(phPipeWrite, VERR_INVALID_POINTER);

    /*
     * Create the pipe and set the close-on-exec flag.
     */
    int aFds[2] = {-1, -1};
    if (pipe(aFds))
        return RTErrConvertFromErrno(errno);
    if (   fcntl(aFds[0], F_SETFD, FD_CLOEXEC) < 0
        || fcntl(aFds[1], F_SETFD, FD_CLOEXEC) < 0)
    {
        int rc = RTErrConvertFromErrno(errno);
        close(aFds[0]);
        close(aFds[1]);
        return rc;
    }

    *phPipeRead  = aFds[0];
    *phPipeWrite = aFds[1];

    /*
     * Before we leave, make sure to shut up SIGPIPE.
     */
    signal(SIGPIPE, SIG_IGN);
    return VINF_SUCCESS;
}

hotplugInotifyImpl::hotplugInotifyImpl(const char *pcszDevicesRoot) :
    mhWakeupPipeR(-1), mhWakeupPipeW(-1), mfWaiting(0),
    mpcszDevicesRoot(pcszDevicesRoot), mStatus(VERR_WRONG_ORDER)
{
#  ifdef DEBUG
    /* Excercise the code path (term() on a not-fully-initialised object) as
     * well as we can.  On an uninitialised object this method is a semantic
     * no-op. */
    mWatches.mhInotify = -1; /* term will access this variable */
    term();
    /* For now this probing method should only be used if nothing else is
     * available */
#  endif
    int rc;
    do {
        if (RT_FAILURE(rc = iwInit(&mWatches)))
            break;
        if (RT_FAILURE(rc = iwAddWatch(&mWatches, mpcszDevicesRoot)))
            break;
        if (RT_FAILURE(rc = pipeCreateSimple(&mhWakeupPipeR, &mhWakeupPipeW)))
            break;
    } while (0);
    mStatus = rc;
    if (RT_FAILURE(rc))
        term();
}

void hotplugInotifyImpl::term(void)
{
    /** This would probably be a pending segfault, so die cleanly */
    AssertRelease(!mfWaiting);
    if (mhWakeupPipeR != -1)
    {
        close(mhWakeupPipeR);
        mhWakeupPipeR = -1;
    }
    if (mhWakeupPipeW != -1)
    {
        close(mhWakeupPipeW);
        mhWakeupPipeW = -1;
    }
    iwTerm(&mWatches);
}

int hotplugInotifyImpl::drainInotify()
{
    char chBuf[RTPATH_MAX + 256];  /* Should always be big enough */
    ssize_t cchRead;

    AssertRCReturn(mStatus, VERR_WRONG_ORDER);
    errno = 0;
    do {
        cchRead = read(iwGetFD(&mWatches), chBuf, sizeof(chBuf));
    } while (cchRead > 0);
    if (cchRead == 0)
        return VINF_SUCCESS;
    if (   cchRead < 0
        && (   errno == EAGAIN
#if EAGAIN != EWOULDBLOCK
            || errno == EWOULDBLOCK
#endif
            ))
        return VINF_SUCCESS;
    Assert(errno > 0);
    return RTErrConvertFromErrno(errno);
}

int hotplugInotifyImpl::drainWakeupPipe(void)
{
    char szBuf[sizeof(SYSFS_WAKEUP_STRING)];
    ssize_t cbRead;

    AssertRCReturn(mStatus, VERR_WRONG_ORDER);
    cbRead = read(mhWakeupPipeR, szBuf, sizeof(szBuf));
    Assert(cbRead > 0);
    NOREF(cbRead);
    return VINF_SUCCESS;
}

int hotplugInotifyImpl::Wait(RTMSINTERVAL aMillies)
{
    int rc;
    char **ppszEntry;
    VECTOR_PTR(char *) vecpchDevs;

    AssertRCReturn(mStatus, VERR_WRONG_ORDER);
    bool fEntered = ASMAtomicCmpXchgU32(&mfWaiting, 1, 0);
    AssertReturn(fEntered, VERR_WRONG_ORDER);
    VEC_INIT_PTR(&vecpchDevs, char *, RTStrFree);
    do {
        struct pollfd pollFD[MAX_POLLID];

        rc = readFilePaths(mpcszDevicesRoot, &vecpchDevs, false);
        if (RT_SUCCESS(rc))
            VEC_FOR_EACH(&vecpchDevs, char *, ppszEntry)
                if (RT_FAILURE(rc = iwAddWatch(&mWatches, *ppszEntry)))
                    break;
        if (RT_FAILURE(rc))
            break;
        pollFD[RPIPE_ID].fd = mhWakeupPipeR;
        pollFD[RPIPE_ID].events = POLLIN;
        pollFD[INOTIFY_ID].fd = iwGetFD(&mWatches);
        pollFD[INOTIFY_ID].events = POLLIN | POLLERR | POLLHUP;
        errno = 0;
        int cPolled = poll(pollFD, RT_ELEMENTS(pollFD), aMillies);
        if (cPolled < 0)
        {
            Assert(errno > 0);
            rc = RTErrConvertFromErrno(errno);
        }
        else if (pollFD[RPIPE_ID].revents)
        {
            rc = drainWakeupPipe();
            if (RT_SUCCESS(rc))
                rc = VERR_INTERRUPTED;
            break;
        }
        else if (!(pollFD[INOTIFY_ID].revents))
        {
            AssertBreakStmt(cPolled == 0, rc = VERR_INTERNAL_ERROR);
            rc = VERR_TIMEOUT;
        }
        Assert(errno == 0 || (RT_FAILURE(rc) && rc != VERR_TIMEOUT));
        if (RT_FAILURE(rc))
            break;
        AssertBreakStmt(cPolled == 1, rc = VERR_INTERNAL_ERROR);
        if (RT_FAILURE(rc = drainInotify()))
            break;
    } while (false);
    mfWaiting = 0;
    VEC_CLEANUP_PTR(&vecpchDevs);
    return rc;
}

void hotplugInotifyImpl::Interrupt(void)
{
    AssertRCReturnVoid(mStatus);
    ssize_t cbWritten = write(mhWakeupPipeW, SYSFS_WAKEUP_STRING,
                         sizeof(SYSFS_WAKEUP_STRING));
    if (cbWritten > 0)
        fsync(mhWakeupPipeW);
}

# endif /* VBOX_USB_WITH_INOTIFY */
#endif  /* VBOX_USB_WTH_SYSFS */

VBoxMainHotplugWaiter::VBoxMainHotplugWaiter(const char *pcszDevicesRoot)
{
    try
    {
#ifdef VBOX_USB_WITH_SYSFS
# ifdef VBOX_USB_WITH_INOTIFY
        if (hotplugInotifyImpl::Available())
        {
            mImpl = new hotplugInotifyImpl(pcszDevicesRoot);
            return;
        }
# endif /* VBOX_USB_WITH_INOTIFY */
#endif  /* VBOX_USB_WITH_SYSFS */
        mImpl = new hotplugNullImpl(pcszDevicesRoot);
    }
    catch(std::bad_alloc &e)
    { }
}

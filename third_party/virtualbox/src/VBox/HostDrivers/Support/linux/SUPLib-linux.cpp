/* $Id: SUPLib-linux.cpp $ */
/** @file
 * VirtualBox Support Library - GNU/Linux specific parts.
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
#define LOG_GROUP LOG_GROUP_SUP
#ifdef IN_SUP_HARDENED_R3
# undef DEBUG /* Warning: disables RT_STRICT */
# undef RT_STRICT
# ifndef LOG_DISABLED
#  define LOG_DISABLED
# endif
# define RTLOG_REL_DISABLED
# include <iprt/log.h>
#endif

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>

#include <VBox/log.h>
#include <VBox/sup.h>
#include <iprt/path.h>
#include <iprt/assert.h>
#include <VBox/types.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include "../SUPLibInternal.h"
#include "../SUPDrvIOC.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** System device name. */
#define DEVICE_NAME_SYS     "/dev/vboxdrv"
/** User device name. */
#define DEVICE_NAME_USR     "/dev/vboxdrvu"

/* define MADV_DONTFORK if it's missing from the system headers. */
#ifndef MADV_DONTFORK
# define MADV_DONTFORK  10
#endif



int suplibOsInit(PSUPLIBDATA pThis, bool fPreInited, bool fUnrestricted, SUPINITOP *penmWhat, PRTERRINFO pErrInfo)
{
    RT_NOREF2(penmWhat, pErrInfo);

    /*
     * Nothing to do if pre-inited.
     */
    if (fPreInited)
        return VINF_SUCCESS;
    Assert(pThis->hDevice == (intptr_t)NIL_RTFILE);

    /*
     * Check if madvise works.
     */
    void *pv = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pv == MAP_FAILED)
        return VERR_NO_MEMORY;
    pThis->fSysMadviseWorks = (0 == madvise(pv, PAGE_SIZE, MADV_DONTFORK));
    munmap(pv, PAGE_SIZE);

    /*
     * Try open the device.
     */
    const char *pszDeviceNm = fUnrestricted ? DEVICE_NAME_SYS : DEVICE_NAME_USR;
    int hDevice = open(pszDeviceNm, O_RDWR, 0);
    if (hDevice < 0)
    {
        /*
         * Try load the device.
         */
        hDevice = open(pszDeviceNm, O_RDWR, 0);
        if (hDevice < 0)
        {
            int rc;
            switch (errno)
            {
                case ENXIO: /* see man 2 open, ENODEV is actually a kernel bug */
                case ENODEV:    rc = VERR_VM_DRIVER_LOAD_ERROR; break;
                case EPERM:
                case EACCES:    rc = VERR_VM_DRIVER_NOT_ACCESSIBLE; break;
                case ENOENT:    rc = VERR_VM_DRIVER_NOT_INSTALLED; break;
                default:        rc = VERR_VM_DRIVER_OPEN_ERROR; break;
            }
            LogRel(("Failed to open \"%s\", errno=%d, rc=%Rrc\n", pszDeviceNm, errno, rc));
            return rc;
        }
    }

    /*
     * Mark the file handle close on exec.
     */
    if (fcntl(hDevice, F_SETFD, FD_CLOEXEC) == -1)
    {
        close(hDevice);
#ifdef IN_SUP_HARDENED_R3
        return VERR_INTERNAL_ERROR;
#else
        return RTErrConvertFromErrno(errno);
#endif
    }

    /*
     * We're done.
     */
    pThis->hDevice       = hDevice;
    pThis->fUnrestricted = fUnrestricted;
    return VINF_SUCCESS;
}


int suplibOsTerm(PSUPLIBDATA pThis)
{
    /*
     * Close the device if it's actually open.
     */
    if (pThis->hDevice != (intptr_t)NIL_RTFILE)
    {
        if (close(pThis->hDevice))
            AssertFailed();
        pThis->hDevice = (intptr_t)NIL_RTFILE;
    }

    return 0;
}


#ifndef IN_SUP_HARDENED_R3

int suplibOsInstall(void)
{
    // nothing to do on Linux
    return VERR_NOT_IMPLEMENTED;
}


int suplibOsUninstall(void)
{
    // nothing to do on Linux
    return VERR_NOT_IMPLEMENTED;
}


int suplibOsIOCtl(PSUPLIBDATA pThis, uintptr_t uFunction, void *pvReq, size_t cbReq)
{
    AssertMsg(pThis->hDevice != (intptr_t)NIL_RTFILE, ("SUPLIB not initiated successfully!\n"));
    NOREF(cbReq);

    /*
     * Issue device iocontrol.
     */
    if (RT_LIKELY(ioctl(pThis->hDevice, uFunction, pvReq) >= 0))
        return VINF_SUCCESS;

    /* This is the reverse operation of the one found in SUPDrv-linux.c */
    switch (errno)
    {
        case EACCES: return VERR_GENERAL_FAILURE;
        case EINVAL: return VERR_INVALID_PARAMETER;
        case EILSEQ: return VERR_INVALID_MAGIC;
        case ENXIO:  return VERR_INVALID_HANDLE;
        case EFAULT: return VERR_INVALID_POINTER;
        case ENOLCK: return VERR_LOCK_FAILED;
        case EEXIST: return VERR_ALREADY_LOADED;
        case EPERM:  return VERR_PERMISSION_DENIED;
        case ENOSYS: return VERR_VERSION_MISMATCH;
        case 1000:   return VERR_IDT_FAILED;
    }

    return RTErrConvertFromErrno(errno);
}


int suplibOsIOCtlFast(PSUPLIBDATA pThis, uintptr_t uFunction, uintptr_t idCpu)
{
    int rc = ioctl(pThis->hDevice, uFunction, idCpu);
    if (rc == -1)
        rc = -errno;
    return rc;
}


int suplibOsPageAlloc(PSUPLIBDATA pThis, size_t cPages, void **ppvPages)
{
    size_t cbMmap = (pThis->fSysMadviseWorks ? cPages : cPages + 2) << PAGE_SHIFT;
    char *pvPages = (char *)mmap(NULL, cbMmap, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pvPages == MAP_FAILED)
        return VERR_NO_MEMORY;

    if (pThis->fSysMadviseWorks)
    {
        /*
         * It is not fatal if we fail here but a forked child (e.g. the ALSA sound server)
         * could crash. Linux < 2.6.16 does not implement madvise(MADV_DONTFORK) but the
         * kernel seems to split bigger VMAs and that is all that we want -- later we set the
         * VM_DONTCOPY attribute in supdrvOSLockMemOne().
         */
        if (madvise (pvPages, cbMmap, MADV_DONTFORK))
            LogRel(("SUPLib: madvise %p-%p failed\n", pvPages, cbMmap));
        *ppvPages = pvPages;
    }
    else
    {
        /*
         * madvise(MADV_DONTFORK) is not available (most probably Linux 2.4). Enclose any
         * mmapped region by two unmapped pages to guarantee that there is exactly one VM
         * area struct of the very same size as the mmap area.
         */
        mprotect(pvPages,                      PAGE_SIZE, PROT_NONE);
        mprotect(pvPages + cbMmap - PAGE_SIZE, PAGE_SIZE, PROT_NONE);
        *ppvPages = pvPages + PAGE_SIZE;
    }
    memset(*ppvPages, 0, cPages << PAGE_SHIFT);
    return VINF_SUCCESS;
}


int suplibOsPageFree(PSUPLIBDATA pThis, void *pvPages, size_t cPages)
{
    NOREF(pThis);
    munmap(pvPages, cPages << PAGE_SHIFT);
    return VINF_SUCCESS;
}


/** Check if the host kernel supports VT-x or not.
 *
 * Older Linux kernels clear the VMXE bit in the CR4 register (function
 * tlb_flush_all()) leading to a host kernel panic.
 */
int suplibOsQueryVTxSupported(void)
{
    char szBuf[256];
    int rc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE, szBuf, sizeof(szBuf));

    if (RT_SUCCESS(rc))
    {
        char *pszNext;
        uint32_t uA, uB, uC;

        rc = RTStrToUInt32Ex(szBuf, &pszNext, 10, &uA);
        if (   RT_SUCCESS(rc)
            && *pszNext == '.')
        {
            /*
             * new version number scheme starting with Linux 3.0
             */
            if (uA >= 3)
                return VINF_SUCCESS;
            rc = RTStrToUInt32Ex(pszNext+1, &pszNext, 10, &uB);
            if (   RT_SUCCESS(rc)
                && *pszNext == '.')
            {
                rc = RTStrToUInt32Ex(pszNext+1, &pszNext, 10, &uC);
                if (RT_SUCCESS(rc))
                {
                    uint32_t uLinuxVersion = (uA << 16) + (uB << 8) + uC;
                    if (uLinuxVersion >= (2 << 16) + (6 << 8) + 13)
                        return VINF_SUCCESS;
                }
            }
        }
    }

    return VERR_SUPDRV_KERNEL_TOO_OLD_FOR_VTX;
}

#endif /* !IN_SUP_HARDENED_R3 */


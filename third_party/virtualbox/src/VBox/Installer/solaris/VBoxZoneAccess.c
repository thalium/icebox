/* $Id: VBoxZoneAccess.c $ */
/** @file
 * VBoxZoneAccess - Hack that keeps vboxdrv referenced for granting zone access, Solaris hosts.
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
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <iprt/process.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define DEVICE_NAME     "/devices/pseudo/vboxdrv@0:vboxdrv"
#define DEVICE_NAME_USR "/devices/pseudo/vboxdrv@0:vboxdrvu"

int main(int argc, char *argv[])
{
    int hDevice = -1;
    int hDeviceUsr = -1;

    /* Check root permissions. */
    if (geteuid() != 0)
    {
        fprintf(stderr, "This program needs administrator privileges.\n");
        return -1;
    }

    /* Daemonize... */
    RTProcDaemonizeUsingFork(false /* fNoChDir */,
                             false /* fNoClose */,
                             NULL /* pszPidfile */);

    /* Open the device */
    hDevice = open(DEVICE_NAME, O_RDWR, 0);
    if (hDevice < 0)
    {
        fprintf(stderr, "Failed to open '%s'. errno=%d\n", DEVICE_NAME, errno);
        return errno;
    }

    /* Open the user device. */
    hDeviceUsr = open(DEVICE_NAME_USR, O_RDWR, 0);
    if (hDeviceUsr < 0)
    {
        fprintf(stderr, "Failed to open '%s'. errno=%d\n", DEVICE_NAME_USR, errno);
        close(hDevice);
        return errno;
    }

    /* Mark the file handle close on exec. */
    if (   fcntl(hDevice,    F_SETFD, FD_CLOEXEC) != 0
        || fcntl(hDeviceUsr, F_SETFD, FD_CLOEXEC) != 0)
    {
        fprintf(stderr, "Failed to set close on exec. errno=%d\n", errno);
        close(hDevice);
        close(hDeviceUsr);
        return errno;
    }

    /* Go to interruptible sleep for ~15 years... */
    /* avoid > 2^31 for Year 2038 32-bit overflow (Solaris 10) */
    sleep(500000000U);

    close(hDevice);
    close(hDeviceUsr);

    return 0;
}


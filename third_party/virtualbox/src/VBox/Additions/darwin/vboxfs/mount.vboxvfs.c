/* $Id: mount.vboxvfs.c $ */
/** @file
 * VBoxVFS - mount tool.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <errno.h>
#include <string.h>

#include "vboxvfs.h"

static char *progname;

static void
usage(void)
{
    fprintf(stderr, "usage: %s [OPTIONS] <shared folder name> "
                    "<mount point>\n", progname);
    exit(1);
}

int
main(int argc, char *argv[])
{
    int                         rc;
    int                         c;
    char                       *sShareName;
    char                       *sMountPoint;
    struct vboxvfs_mount_info   mnt_info;

    /* Set program name */
    progname = argv[0];

    /* Parse command line */
    while((c = getopt(argc, argv, "o:")) != -1)
    {
        switch(c)
        {
            case 'o': break;
            default : usage();
        }
    }

    /* Two arguments are rquired: <share name> and <mount point> */
    if ((argc - optind) != 2)
        usage();

    sShareName = argv[optind++];
    sMountPoint = argv[optind];

    if (strlen(sShareName) > MAXPATHLEN)
    {
        fprintf(stderr, "Specified Shared Folder name too long\n");
        return EINVAL;
    }

    mnt_info.magic = VBOXVFS_MOUNTINFO_MAGIC;
    strcpy(mnt_info.name, sShareName);

    rc = mount(VBOXVBFS_NAME, sMountPoint, 0, &mnt_info);
    if (rc)
    {
        fprintf(stderr,
                "Unable to mount shared folder (%s) '%s' to '%s': %s\n",
                VBOXVBFS_NAME,
                mnt_info.name,
                sMountPoint,
                strerror(errno));
        return 1;
    }

    return 0;
}

/* $Id: vbsfmount.c $ */
/** @file
 * vbsfmount - Commonly used code to mount shared folders on Linux-based
 *             systems.  Currently used by mount.vboxsf and VBoxService.
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

#include "vbsfmount.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ctype.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>


/** @todo Use defines for return values! */
int vbsfmount_complete(const char *host_name, const char *mount_point,
                       unsigned long flags, struct vbsf_mount_opts *opts)
{
    FILE *f, *m;
    char *buf;
    size_t size;
    struct mntent e;
    int rc = 0;

    m = open_memstream(&buf, &size);
    if (!m)
        return 1; /* Could not update mount table (failed to create memstream). */

    if (opts->uid)
        fprintf(m, "uid=%d,", opts->uid);
    if (opts->gid)
        fprintf(m, "gid=%d,", opts->gid);
    if (opts->ttl)
        fprintf(m, "ttl=%d,", opts->ttl);
    if (*opts->nls_name)
        fprintf(m, "iocharset=%s,", opts->nls_name);
    if (flags & MS_NOSUID)
        fprintf(m, "%s,", MNTOPT_NOSUID);
    if (flags & MS_RDONLY)
        fprintf(m, "%s,", MNTOPT_RO);
    else
        fprintf(m, "%s,", MNTOPT_RW);

    fclose(m);

    if (size > 0)
        buf[size - 1] = 0;
    else
        buf = "defaults";

    f = setmntent(MOUNTED, "a+");
    if (!f)
    {
        rc = 2; /* Could not open mount table for update. */
    }
    else
    {
        e.mnt_fsname = (char*)host_name;
        e.mnt_dir = (char*)mount_point;
        e.mnt_type = "vboxsf";
        e.mnt_opts = buf;
        e.mnt_freq = 0;
        e.mnt_passno = 0;

        if (addmntent(f, &e))
            rc = 3;  /* Could not add an entry to the mount table. */

        endmntent(f);
    }

    if (size > 0)
    {
        memset(buf, 0, size);
        free(buf);
    }

    return rc;
}

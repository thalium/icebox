/* $Id: vboxvfs.h $ */
/** @file
 * Description.
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

#ifndef ___VBOXVFS_H___
#define ___VBOXVFS_H___

#define VBOXVFS_VFSNAME "vboxvfs"
#define VBOXVFS_VERSION 1

#define MAX_HOST_NAME 256
#define MAX_NLS_NAME 32

struct vboxvfs_mount_info {
    char name[MAX_HOST_NAME];
    char nls_name[MAX_NLS_NAME];
    int uid;
    int gid;
    int ttl;
};

#ifdef _KERNEL

#include <VBox/VBoxGuestLibSharedFolders.h>
#include <sys/mount.h>
#include <sys/vnode.h>

struct vboxvfsmount {
    uid_t           uid;
    gid_t           gid;
    mode_t          file_mode;
    mode_t          dir_mode;
    struct mount   *mp;
    struct ucred   *owner;
    u_int           flags;
    long            nextino;
    int             caseopt;
    int             didrele;
};

/* structs - stolen from the linux shared module code */
struct sf_glob_info {
    VBGLSFMAP map;
/*    struct nls_table *nls;*/
    int ttl;
    int uid;
    int gid;
    struct vnode *vnode_root;
};

struct sf_inode_info {
    SHFLSTRING *path;
    int force_restat;
};

#if 0
struct sf_dir_info {
    struct list_head info_list;
};
#endif

struct sf_dir_buf {
    size_t nb_entries;
    size_t free_bytes;
    size_t used_bytes;
    void *buf;
#if 0
   struct list_head head;
#endif
};

struct sf_reg_info {
    SHFLHANDLE handle;
};

#endif  /* KERNEL */

#endif /* !___VBOXVFS_H___ */


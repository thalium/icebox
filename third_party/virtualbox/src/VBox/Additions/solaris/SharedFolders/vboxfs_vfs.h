/* $Id: vboxfs_vfs.h $ */
/** @file
 * VirtualBox File System for Solaris Guests, VFS header.
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

#ifndef	___VBoxFS_vfs_Solaris_h
#define	___VBoxFS_vfs_Solaris_h

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Shared Folders filesystem per-mount data structure.
 */
typedef struct sffs_data {
	vfs_t		*sf_vfsp;	/* filesystem's vfs struct */
	vnode_t		*sf_rootnode;	/* of vnode of the root directory */
	int  		sf_stat_ttl;	/* ttl for stat caches (in ms) */
	int  		sf_fsync;	/* whether to honor fsync or not */
	char		*sf_share_name;
	char 		*sf_mntpath;	/* name of mount point */
	sfp_mount_t	*sf_handle;
	uint64_t	sf_ino;		/* per FS ino generator */
} sffs_data_t;


#ifdef	__cplusplus
}
#endif

#endif	/* !___VBoxFS_vfs_Solaris_h */


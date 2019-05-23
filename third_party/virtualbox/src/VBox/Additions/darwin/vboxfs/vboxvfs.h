/* $Id: vboxvfs.h $ */
/** @file
 * VBoxVFS - common header used across all the driver source files.
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

#define MODULE_NAME "VBoxVFS"

#ifdef KERNEL
# include <libkern/libkern.h>
# include <sys/lock.h>
#endif

#define PINFO(fmt, args...) \
    printf(MODULE_NAME ": INFO: " fmt "\n", ## args)
#define PDEBUG(fmt, args...) \
    printf(MODULE_NAME ": %s(): DEBUG: " fmt "\n", __FUNCTION__, ## args)
#define PERROR(fmt, args...) \
    printf(MODULE_NAME ": ERROR: " fmt "\n", ## args)

#define VBOXVBFS_NAME               "vboxvfs"
#define VBOXVFS_MOUNTINFO_MAGIC     (0xCAFE)

#ifdef KERNEL

#include <iprt/types.h>
#undef PVM
#include <sys/vnode.h>

#include <VBox/VBoxGuestLibSharedFolders.h>


/** Global refernce to host service connection */
extern VBGLSFCLIENT g_vboxSFClient;

/** Private data assigned to each mounted shared folder. Assigned to mp structure. */
typedef struct vboxvfs_mount_data
{
    VBSFMAP             pMap;               /** Shared folder mapping */
    SHFLSTRING         *pShareName;         /** VBoxVFS share name */
    uint64_t            cFileIdCounter;     /** Counter that used in order to assign unique ID to each vnode within mounted share */
    vnode_t             pRootVnode;         /** VFS object: vnode that corresponds shared folder root */
    uint8_t volatile    fRootVnodeState;    /** Sync flag that used in order to safely allocate pRootVnode */
    uid_t               owner;              /** User ID tha mounted shared folder */
    lck_grp_t          *pLockGroup;         /** BSD locking stuff */
    lck_grp_attr_t     *pLockGroupAttr;     /** BSD locking stuff */
} vboxvfs_mount_t;

/** Private data assigned to each vnode object. */
typedef struct vboxvfs_vnode_data
{
    SHFLHANDLE      pHandle;                /** VBoxVFS object handle. */
    PSHFLSTRING     pPath;                  /** Path within shared folder */
    lck_attr_t     *pLockAttr;              /** BSD locking stuff */
    lck_rw_t       *pLock;                  /** BSD locking stuff */
} vboxvfs_vnode_t;

/**
 * Helper function to create XNU VFS vnode object.
 *
 * @param mp        Mount data structure
 * @param type      vnode type (directory, regular file, etc)
 * @param pParent   Parent vnode object (NULL for VBoxVFS root vnode)
 * @param fIsRoot   Flag that indicates if created vnode object is
 *                  VBoxVFS root vnode (TRUE for VBoxVFS root vnode, FALSE
 *                  for all aother vnodes)
 * @param           Path within Shared Folder
 * @param ret       Returned newly created vnode
 *
 * @return 0 on success, error code otherwise
 */
extern int vboxvfs_create_vnode_internal(struct mount *mp, enum vtype type, vnode_t pParent, int fIsRoot, PSHFLSTRING Path, vnode_t *ret);

/**
 * Convert guest absolute VFS path (starting from VFS root) to a host path
 * within mounted shared folder (returning it as a char *).
 *
 * @param mp            Mount data structure
 * @param pszGuestPath  Guest absolute VFS path (starting from VFS root)
 * @param cbGuestPath   Size of pszGuestPath
 * @param pszHostPath   Returned char * wich contains host path
 * @param cbHostPath    Returned pszHostPath size
 *
 * @return 0 on success, error code otherwise
 */
extern int vboxvfs_guest_path_to_char_path_internal(mount_t mp, char *pszGuestPath, int cbGuestPath, char **pszHostPath, int *cbHostPath);

/**
 * Convert guest absolute VFS path (starting from VFS root) to a host path
 * within mounted shared folder.
 *
 * @param mp            Mount data structure
 * @param pszGuestPath  Guest absolute VFS path (starting from VFS root)
 * @param cbGuestPath   Size of pszGuestPath
 * @param ppResult      Returned PSHFLSTRING object wich contains host path
 *
 * @return 0 on success, error code otherwise
 */
extern int vboxvfs_guest_path_to_shflstring_path_internal(mount_t mp, char *pszGuestPath, int cbGuestPath, PSHFLSTRING *ppResult);

/**
 * Wrapper function for vboxvfs_guest_path_to_char_path_internal() which
 * converts guest path to host path using vnode object information.
 *
 * @param vnode     Guest's VFS object
 * @param ppPath    Allocated  char * which contain a path
 * @param pcbPath   Size of ppPath
 *
 * @return 0 on success, error code otherwise.
 */
extern int vboxvfs_guest_vnode_to_char_path_internal(vnode_t vnode, char **ppHostPath, int *pcbHostPath);

/**
 * Wrapper function for vboxvfs_guest_path_to_shflstring_path_internal() which
 * converts guest path to host path using vnode object information.
 *
 * @param vnode     Guest's VFS object
 * @param ppResult  Allocated  PSHFLSTRING object which contain a path
 *
 * @return 0 on success, error code otherwise.
 */
extern int vboxvfs_guest_vnode_to_shflstring_path_internal(vnode_t vnode, PSHFLSTRING *ppResult);

/**
 * Free resources allocated by vboxvfs_path_internal() and vboxvfs_guest_vnode_to_shflstring_path_internal().
 *
 * @param ppHandle  Reference to object to be freed.
 */
extern void vboxvfs_put_path_internal(void **ppHandle);

/**
 * Open existing VBoxVFS object and return its handle.
 *
 * @param pMount            Mount session data.
 * @param pPath             VFS path to the object relative to mount point.
 * @param fFlags            For directory object it should be
 *                          SHFL_CF_DIRECTORY and 0 for any other object.
 * @param pHandle           Returned handle.
 *
 * @return 0 on success, error code otherwise.
 */
extern int vboxvfs_open_internal(vboxvfs_mount_t *pMount, PSHFLSTRING pPath, uint32_t fFlags, SHFLHANDLE *pHandle);

/**
 * Release VBoxVFS object handle openned by vboxvfs_open_internal().
 *
 * @param pMount            Mount session data.
 * @param pHandle           Handle to close.
 *
 * @return 0 on success, IPRT error code otherwise.
 */
extern int vboxvfs_close_internal(vboxvfs_mount_t *pMount, SHFLHANDLE pHandle);

/**
 * Get information about host VFS object.
 *
 * @param mp           Mount point data
 * @param pSHFLDPath   Path to VFS object within mounted shared folder
 * @param Info         Returned info
 *
 * @return  0 on success, error code otherwise.
 */
extern int vboxvfs_get_info_internal(mount_t mp, PSHFLSTRING pSHFLDPath, PSHFLFSOBJINFO Info);

/**
 * Check if VFS object exists on a host side.
 *
 * @param vnode     Guest VFS vnode that corresponds to host VFS object
 *
 * @return 1 if exists, 0 otherwise.
 */
extern int vboxvfs_exist_internal(vnode_t vnode);

/**
 * Convert host VFS object mode flags into guest ones.
 *
 * @param fHostMode     Host flags
 *
 * @return Guest flags
 */
extern mode_t vboxvfs_h2g_mode_inernal(RTFMODE fHostMode);

/**
 * Convert guest VFS object mode flags into host ones.
 *
 * @param fGuestMode     Host flags
 *
 * @return Host flags
 */
extern uint32_t vboxvfs_g2h_mode_inernal(mode_t fGuestMode);

extern SHFLSTRING *vboxvfs_construct_shflstring(char *szName, size_t cbName);

#endif /* KERNEL */

extern int vboxvfs_register_filesystem(void);
extern int vboxvfs_unregister_filesystem(void);

/* VFS options */
extern struct vfsops g_oVBoxVFSOpts;

extern int (**g_VBoxVFSVnodeDirOpsVector)(void *);
extern int g_cVBoxVFSVnodeOpvDescListSize;
extern struct vnodeopv_desc *g_VBoxVFSVnodeOpvDescList[];

/* Mount info */
struct vboxvfs_mount_info
{
    uint32_t    magic;
    char        name[MAXPATHLEN];   /* share name */
};


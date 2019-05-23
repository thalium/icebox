/* $Id: VBoxVFS-VNODEOps.cpp $ */
/** @file
 * VBoxVFS - vnode operations.
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

#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/dirent.h>

#include <iprt/mem.h>
#include <iprt/assert.h>

#include "vboxvfs.h"

#define VNODEOPFUNC int(*)(void *)

static int vboxvfs_dfl_error()
{
    PDEBUG("vboxvfs_dfl_error is called");

    return ENOTSUP;
}

static int
vboxvfs_vnode_getattr(struct vnop_getattr_args *args)
{
    vboxvfs_mount_t   *pMount;
    struct vnode_attr *vnode_args;
    vboxvfs_vnode_t   *pVnodeData;

    struct timespec timespec;

    SHFLFSOBJINFO Info;
    mount_t       mp;
    vnode_t       vnode;
    int           rc;

    PDEBUG("Getting vnode attribute...");

    AssertReturn(args, EINVAL);

    vnode                = args->a_vp;                                   AssertReturn(vnode,           EINVAL);
    vnode_args           = args->a_vap;                                  AssertReturn(vnode_args,      EINVAL);
    mp                   = vnode_mount(vnode);                           AssertReturn(mp,              EINVAL);
    pMount      = (vboxvfs_mount_t *)vfs_fsprivate(mp);     AssertReturn(pMount, EINVAL);
    pVnodeData           = (vboxvfs_vnode_t *)vnode_fsnode(vnode);  AssertReturn(pVnodeData,      EINVAL);

    lck_rw_lock_shared(pVnodeData->pLock);

    rc = vboxvfs_get_info_internal(mp, pVnodeData->pPath, &Info);
    if (rc == 0)
    {
        /* Set timestamps */
        RTTimeSpecGetTimespec(&Info.BirthTime,        &timespec); VATTR_RETURN(vnode_args, va_create_time, timespec);
        RTTimeSpecGetTimespec(&Info.AccessTime,       &timespec); VATTR_RETURN(vnode_args, va_access_time, timespec);
        RTTimeSpecGetTimespec(&Info.ModificationTime, &timespec); VATTR_RETURN(vnode_args, va_modify_time, timespec);
        RTTimeSpecGetTimespec(&Info.ChangeTime,       &timespec); VATTR_RETURN(vnode_args, va_change_time, timespec);
        VATTR_CLEAR_ACTIVE(vnode_args, va_backup_time);

        /* Set owner info. */
        VATTR_RETURN(vnode_args, va_uid, pMount->owner);
        VATTR_CLEAR_ACTIVE(vnode_args, va_gid);

        /* Access mode and flags */
        VATTR_RETURN(vnode_args, va_mode,  vboxvfs_h2g_mode_inernal(Info.Attr.fMode));
        VATTR_RETURN(vnode_args, va_flags, Info.Attr.u.Unix.fFlags);

        /* The current generation number (0 if this information is not available) */
        VATTR_RETURN(vnode_args, va_gen, Info.Attr.u.Unix.GenerationId);

        VATTR_RETURN(vnode_args, va_rdev,  0);
        VATTR_RETURN(vnode_args, va_nlink, 2);

        VATTR_RETURN(vnode_args, va_data_size, sizeof(struct dirent)); /* Size of data returned per each readdir() request */

        /* Hope, when it overflows nothing catastrophical will heppen! If we will not assign
         * a uniq va_fileid to each vnode, `ls`, 'find' (and simmilar tools that uses fts_read() calls) will think that
         * each sub-directory is self-cycled. */
        VATTR_RETURN(vnode_args, va_fileid, (pMount->cFileIdCounter++));

        /* Not supported */
        VATTR_CLEAR_ACTIVE(vnode_args, va_linkid);
        VATTR_CLEAR_ACTIVE(vnode_args, va_parentid);
        VATTR_CLEAR_ACTIVE(vnode_args, va_fsid);
        VATTR_CLEAR_ACTIVE(vnode_args, va_filerev);

        /* Not present on 10.6 */
        //VATTR_CLEAR_ACTIVE(vnode_args, va_addedtime);

        /** @todo take care about va_encoding (file name encoding) */
        VATTR_CLEAR_ACTIVE(vnode_args, va_encoding);
        /** @todo take care about: va_acl */
        VATTR_CLEAR_ACTIVE(vnode_args, va_acl);

        VATTR_CLEAR_ACTIVE(vnode_args, va_name);
        VATTR_CLEAR_ACTIVE(vnode_args, va_uuuid);
        VATTR_CLEAR_ACTIVE(vnode_args, va_guuid);

        VATTR_CLEAR_ACTIVE(vnode_args, va_total_size);
        VATTR_CLEAR_ACTIVE(vnode_args, va_total_alloc);
        VATTR_CLEAR_ACTIVE(vnode_args, va_data_alloc);
        VATTR_CLEAR_ACTIVE(vnode_args, va_iosize);

        VATTR_CLEAR_ACTIVE(vnode_args, va_nchildren);
        VATTR_CLEAR_ACTIVE(vnode_args, va_dirlinkcount);
    }
    else
    {
        PDEBUG("getattr: unable to get VBoxVFS object info");
    }

    lck_rw_unlock_shared(pVnodeData->pLock);

    return rc;
}

/**
 * Helper function for vboxvfs_vnode_lookup(): create new vnode.
 */
static int
vboxvfs_vnode_lookup_instantinate_vnode(vnode_t parent_vnode, char *entry_name, vnode_t *result_vnode)
{
    /* We need to construct full path to vnode in order to get
     * vboxvfs_get_info_internal() to understand us! */

    char *pszCurDirPath;
    int   cbCurDirPath = MAXPATHLEN;

    mount_t mp = vnode_mount(parent_vnode); AssertReturn(mp,  EINVAL);
    vnode_t vnode;

    int rc;

    pszCurDirPath = (char *)RTMemAllocZ(cbCurDirPath);
    if (pszCurDirPath)
    {
        rc = vn_getpath(parent_vnode, pszCurDirPath, &cbCurDirPath);
        if (rc == 0 && cbCurDirPath < MAXPATHLEN)
        {
            SHFLFSOBJINFO Info;
            PSHFLSTRING   pSHFLPath;

            /* Add '/' between path parts and truncate name if it is too long */
            strncat(pszCurDirPath, "/", 1); strncat(pszCurDirPath, entry_name, MAXPATHLEN - cbCurDirPath - 1);

            rc = vboxvfs_guest_path_to_shflstring_path_internal(mp, pszCurDirPath, strlen(pszCurDirPath) + 1, &pSHFLPath);
            if (rc == 0)
            {
                rc = vboxvfs_get_info_internal(mp, pSHFLPath, (PSHFLFSOBJINFO)&Info);
                if (rc == 0)
                {
                    enum vtype type;

                    if      (RTFS_IS_DIRECTORY(Info.Attr.fMode)) type = VDIR;
                    else if (RTFS_IS_FILE     (Info.Attr.fMode)) type = VREG;
                    else
                    {
                        PDEBUG("Not supported VFS object (%s) type: mode 0x%X",
                               entry_name,
                               Info.Attr.fMode);

                        RTMemFree(pszCurDirPath);
                        vboxvfs_put_path_internal((void **)&pSHFLPath);
                        return ENOENT;
                    }
                    /* Create new vnode */
                    rc = vboxvfs_create_vnode_internal(mp, type, parent_vnode, FALSE, pSHFLPath, &vnode);
                    if (rc == 0)
                    {
                        PDEBUG("new vnode object '%s' has been created", entry_name);

                        *result_vnode = vnode;
                        RTMemFree(pszCurDirPath);

                        return 0;
                    }
                    else
                        PDEBUG("Unable to create vnode: %d", rc);
                }
                else
                    PDEBUG("Unable to get host object info: %d", rc);

                vboxvfs_put_path_internal((void **)&pSHFLPath);
            }
            else
                PDEBUG("Unable to convert guest<->host path");
        }
        else
            PDEBUG("Unable to construct vnode path: %d", rc);

        RTMemFree(pszCurDirPath);
    }
    else
    {
        PDEBUG("Unable to allocate memory for path buffer");
        rc = ENOMEM;
    }

    return rc;
}

/**
 * Helper function for vboxvfs_vnode_lookup(): take care
 * about '.' and '..' directory entries.
 */
static int
vboxvfs_vnode_lookup_dot_handler(struct vnop_lookup_args *args, vnode_t *result_vnode)
{
    vnode_t vnode = NULL;

    if (args->a_cnp->cn_flags & ISDOTDOT)
    {
        vnode = vnode_getparent(args->a_dvp);
        if (vnode)
        {
            PDEBUG("return parent directory");
            *result_vnode = vnode;
            return 0;
        }
        else
        {
            PDEBUG("return parent directory not found, return current directory");
            *result_vnode = args->a_dvp;
            return 0;
        }
    }
    else if ((strncmp(args->a_cnp->cn_nameptr, ".", 1) == 0) &&
             args->a_cnp->cn_namelen == 1)
    {
        PDEBUG("return current directory");
        *result_vnode = args->a_dvp;
        return 0;
    }

    return ENOENT;
}

static int
vboxvfs_vnode_lookup(struct vnop_lookup_args *args)
{
    int rc;

    vnode_t          vnode;
    vboxvfs_vnode_t *pVnodeData;

    PDEBUG("Looking up for vnode...");

    AssertReturn(args,                      EINVAL);
    AssertReturn(args->a_dvp,               EINVAL);
    AssertReturn(vnode_isdir(args->a_dvp),  EINVAL);
    AssertReturn(args->a_cnp,               EINVAL);
    AssertReturn(args->a_cnp->cn_nameptr,   EINVAL);
    AssertReturn(args->a_vpp,               EINVAL);

    pVnodeData = (vboxvfs_vnode_t *)vnode_fsnode(args->a_dvp);
    AssertReturn(pVnodeData, EINVAL);
    AssertReturn(pVnodeData->pLock, EINVAL);

    /*
    todo: take care about args->a_cnp->cn_nameiop
    */

    if      (args->a_cnp->cn_nameiop == LOOKUP) PDEBUG("LOOKUP");
    else if (args->a_cnp->cn_nameiop == CREATE) PDEBUG("CREATE");
    else if (args->a_cnp->cn_nameiop == RENAME) PDEBUG("RENAME");
    else if (args->a_cnp->cn_nameiop == DELETE) PDEBUG("DELETE");
    else PDEBUG("Unknown cn_nameiop: 0x%X", (int)args->a_cnp->cn_nameiop);

    lck_rw_lock_exclusive(pVnodeData->pLock);

    /* Take care about '.' and '..' entries */
    if (vboxvfs_vnode_lookup_dot_handler(args, &vnode) == 0)
    {
        vnode_get(vnode);
        *args->a_vpp = vnode;

        lck_rw_unlock_exclusive(pVnodeData->pLock);

        return 0;
    }

    /* Look into VFS cache and attempt to find previously allocated vnode there. */
    rc = cache_lookup(args->a_dvp, &vnode, args->a_cnp);
    if (rc == -1) /* Record found */
    {
        PDEBUG("Found record in VFS cache");

        /* Check if VFS object still exist on a host side */
        if (vboxvfs_exist_internal(vnode))
        {
            /* Prepare & return cached vnode */
            vnode_get(vnode);
            *args->a_vpp = vnode;

            rc = 0;
        }
        else
        {
            /* If vnode exist in guets VFS cache, but not exist on a host -- just forget it. */
            cache_purge(vnode);
            /** @todo free vnode data here */
            rc = ENOENT;
        }
    }
    else
    {
        PDEBUG("cache_lookup() returned %d, create new VFS vnode", rc);

        rc = vboxvfs_vnode_lookup_instantinate_vnode(args->a_dvp, args->a_cnp->cn_nameptr, &vnode);
        if (rc == 0)
        {
            cache_enter(args->a_dvp, vnode, args->a_cnp);
            *args->a_vpp = vnode;
        }
        else
        {
            rc = ENOENT;
        }
    }

    lck_rw_unlock_exclusive(pVnodeData->pLock);

    return rc;
}

static int
vboxvfs_vnode_open(struct vnop_open_args *args)
{
    vnode_t           vnode;
    vboxvfs_vnode_t  *pVnodeData;
    uint32_t          fHostFlags;
    mount_t           mp;
    vboxvfs_mount_t  *pMount;

    int rc;

    PDEBUG("Opening vnode...");

    AssertReturn(args, EINVAL);

    vnode           = args->a_vp;                              AssertReturn(vnode,      EINVAL);
    pVnodeData      = (vboxvfs_vnode_t *)vnode_fsnode(vnode);  AssertReturn(pVnodeData, EINVAL);
    mp              = vnode_mount(vnode);                      AssertReturn(mp,         EINVAL);
    pMount = (vboxvfs_mount_t *)vfs_fsprivate(mp);             AssertReturn(pMount,     EINVAL);

    lck_rw_lock_exclusive(pVnodeData->pLock);

    if (vnode_isinuse(vnode, 0))
    {
        PDEBUG("vnode '%s' (handle 0x%X) already has VBoxVFS object handle assigned, just return ok",
               (char *)pVnodeData->pPath->String.utf8,
               (int)pVnodeData->pHandle);

        lck_rw_unlock_exclusive(pVnodeData->pLock);
        return 0;
    }

    /* At this point we must make sure that nobody is using VBoxVFS object handle */
    //if (pVnodeData->Handle != SHFL_HANDLE_NIL)
    //{
    //    PDEBUG("vnode has active VBoxVFS object handle set, aborting");
    //    lck_rw_unlock_exclusive(pVnodeData->pLock);
    //    return EINVAL;
    //}

    fHostFlags  = vboxvfs_g2h_mode_inernal(args->a_mode);
    fHostFlags |= (vnode_isdir(vnode) ? SHFL_CF_DIRECTORY : 0);

    SHFLHANDLE Handle;
    rc = vboxvfs_open_internal(pMount, pVnodeData->pPath, fHostFlags, &Handle);
    if (rc == 0)
    {
        PDEBUG("Open success: '%s' (handle 0x%X)",
               (char *)pVnodeData->pPath->String.utf8,
               (int)Handle);

        pVnodeData->pHandle = Handle;
    }
    else
    {
        PDEBUG("Unable to open: '%s': %d",
               (char *)pVnodeData->pPath->String.utf8,
               rc);
    }

    lck_rw_unlock_exclusive(pVnodeData->pLock);

    return rc;
}

static int
vboxvfs_vnode_close(struct vnop_close_args *args)
{
    vnode_t          vnode;
    mount_t          mp;
    vboxvfs_vnode_t *pVnodeData;
    vboxvfs_mount_t *pMount;

    int rc;

    PDEBUG("Closing vnode...");

    AssertReturn(args, EINVAL);

    vnode           = args->a_vp;                              AssertReturn(vnode,      EINVAL);
    pVnodeData      = (vboxvfs_vnode_t *)vnode_fsnode(vnode);  AssertReturn(pVnodeData, EINVAL);
    mp              = vnode_mount(vnode);                      AssertReturn(mp,         EINVAL);
    pMount = (vboxvfs_mount_t *)vfs_fsprivate(mp);             AssertReturn(pMount,     EINVAL);

    lck_rw_lock_exclusive(pVnodeData->pLock);

    if (vnode_isinuse(vnode, 0))
    {
        PDEBUG("vnode '%s' (handle 0x%X) is still in use, just return ok",
               (char *)pVnodeData->pPath->String.utf8,
               (int)pVnodeData->pHandle);

        lck_rw_unlock_exclusive(pVnodeData->pLock);
        return 0;
    }

    /* At this point we must make sure that vnode has VBoxVFS object handle assigned */
    if (pVnodeData->pHandle == SHFL_HANDLE_NIL)
    {
        PDEBUG("vnode has no active VBoxVFS object handle set, aborting");
        lck_rw_unlock_exclusive(pVnodeData->pLock);
        return EINVAL;
    }

    rc = vboxvfs_close_internal(pMount, pVnodeData->pHandle);
    if (rc == 0)
    {
        PDEBUG("Close success: '%s' (handle 0x%X)",
               (char *)pVnodeData->pPath->String.utf8,
               (int)pVnodeData->pHandle);

        /* Forget about previously assigned VBoxVFS object handle */
        pVnodeData->pHandle = SHFL_HANDLE_NIL;
    }
    else
    {
        PDEBUG("Unable to close: '%s' (handle 0x%X): %d",
               (char *)pVnodeData->pPath->String.utf8,
               (int)pVnodeData->pHandle, rc);
    }

    lck_rw_unlock_exclusive(pVnodeData->pLock);

    return rc;
}

/**
 * Convert SHFLDIRINFO to struct dirent and copy it back to user.
 */
static int
vboxvfs_vnode_readdir_copy_data(ino_t index, SHFLDIRINFO *Info, struct uio *uio, int *numdirent)
{
    struct dirent entry;

    int rc;

    entry.d_ino = index;
    entry.d_reclen = (__uint16_t)sizeof(entry);

    /* Detect dir entry type */
    if (RTFS_IS_DIRECTORY(Info->Info.Attr.fMode))
        entry.d_type = DT_DIR;
    else if (RTFS_IS_FILE(Info->Info.Attr.fMode))
        entry.d_type = DT_REG;
    else
    {
        PDEBUG("Unknown type of host file: mode 0x%X", (int)Info->Info.Attr.fMode);
        return ENOTSUP;
    }

    entry.d_namlen = (__uint8_t)min(sizeof(entry.d_name), Info->name.u16Size);
    memcpy(entry.d_name, Info->name.String.utf8, entry.d_namlen);

    rc = uiomove((char *)&entry, sizeof(entry), uio);
    if (rc == 0)
    {
        uio_setoffset(uio, index * sizeof(struct dirent));
        *numdirent = (int)index;

        PDEBUG("discovered entry: '%s' (%d bytes), item #%d", entry.d_name, (int)entry.d_namlen, (int)index);
    }
    else
    {
        PDEBUG("Failed to return dirent data item #%d (%d)", (int)index, rc);
    }

    return rc;
}

static int
vboxvfs_vnode_readdir(struct vnop_readdir_args *args)
{
    vboxvfs_mount_t *pMount;
    vboxvfs_vnode_t *pVnodeData;
    SHFLDIRINFO     *Info;
    uint32_t         cbInfo;
    mount_t          mp;
    vnode_t          vnode;
    struct uio      *uio;

    int rc = 0, rc2;

    PDEBUG("Reading directory...");

    AssertReturn(args,              EINVAL);
    AssertReturn(args->a_eofflag,   EINVAL);
    AssertReturn(args->a_numdirent, EINVAL);

    uio             = args->a_uio;                             AssertReturn(uio,        EINVAL);
    vnode           = args->a_vp;                              AssertReturn(vnode,      EINVAL); AssertReturn(vnode_isdir(vnode), EINVAL);
    pVnodeData      = (vboxvfs_vnode_t *)vnode_fsnode(vnode);  AssertReturn(pVnodeData, EINVAL);
    mp              = vnode_mount(vnode);                      AssertReturn(mp,         EINVAL);
    pMount = (vboxvfs_mount_t *)vfs_fsprivate(mp);             AssertReturn(pMount,     EINVAL);

    lck_rw_lock_shared(pVnodeData->pLock);

    cbInfo = sizeof(Info) + MAXPATHLEN;
    Info   = (SHFLDIRINFO *)RTMemAllocZ(cbInfo);
    if (!Info)
    {
        PDEBUG("No memory to allocate internal data");
        lck_rw_unlock_shared(pVnodeData->pLock);
        return ENOMEM;
    }

    uint32_t index = (uint32_t)uio_offset(uio) / (uint32_t)sizeof(struct dirent);
    uint32_t cFiles = 0;

    PDEBUG("Exploring VBoxVFS directory (%s), handle (0x%.8X), offset (0x%X), count (%d)", (char *)pVnodeData->pPath->String.utf8, (int)pVnodeData->pHandle, index, uio_iovcnt(uio));

    /* Currently, there is a problem when VbglR0SfDirInfo() is not able to
     * continue retrieve directory content if the same VBoxVFS handle is used.
     * This macro forces to use a new handle in readdir() callback. If enabled,
     * the original handle (obtained in open() callback is ignored). */

    SHFLHANDLE Handle;
    rc = vboxvfs_open_internal(pMount,
                               pVnodeData->pPath,
                               SHFL_CF_DIRECTORY | SHFL_CF_ACCESS_READ | SHFL_CF_ACT_OPEN_IF_EXISTS | SHFL_CF_ACT_FAIL_IF_NEW,
                               &Handle);
    if (rc != 0)
    {
        PDEBUG("Unable to open dir: %d", rc);
        RTMemFree(Info);
        lck_rw_unlock_shared(pVnodeData->pLock);
        return rc;
    }

#if 0
    rc = VbglR0SfDirInfo(&g_vboxSFClient, &pMount->pMap, Handle, 0, 0, index, &cbInfo, (PSHFLDIRINFO)Info, &cFiles);
#else
    SHFLSTRING *pMask = vboxvfs_construct_shflstring("*", strlen("*"));
    if (pMask)
    {
        for (uint32_t cSkip = 0; (cSkip < index + 1) && (rc == VINF_SUCCESS); cSkip++)
        {
            //rc = VbglR0SfDirInfo(&g_vboxSFClient, &pMount->pMap, Handle, 0 /* pMask */, 0 /* SHFL_LIST_RETURN_ONE */, 0, &cbInfo, (PSHFLDIRINFO)Info, &cFiles);

            uint32_t cbReturned = cbInfo;
            //rc = VbglR0SfDirInfo(&g_vboxSFClient, &pMount->pMap, Handle, pMask, SHFL_LIST_RETURN_ONE, 0, &cbReturned, (PSHFLDIRINFO)Info, &cFiles);
            rc = VbglR0SfDirInfo(&g_vboxSFClient, &pMount->pMap, Handle, 0, SHFL_LIST_RETURN_ONE, 0,
                                 &cbReturned, (PSHFLDIRINFO)Info, &cFiles);

        }

        PDEBUG("read %d files", cFiles);
        RTMemFree(pMask);
    }
    else
    {
        PDEBUG("Can't alloc mask");
        rc = ENOMEM;
    }
#endif
    rc2 = vboxvfs_close_internal(pMount, Handle);
    if (rc2 != 0)
    {
        PDEBUG("Unable to close directory: %s: %d",
               pVnodeData->pPath->String.utf8,
               rc2);
    }

    switch (rc)
    {
        case VINF_SUCCESS:
        {
            rc = vboxvfs_vnode_readdir_copy_data((ino_t)(index + 1), Info, uio, args->a_numdirent);
            break;
        }

        case VERR_NO_MORE_FILES:
        {
            PDEBUG("No more entries in directory");
            *(args->a_eofflag) = 1;
            break;
        }

        default:
        {
            PDEBUG("VbglR0SfDirInfo() for item #%d has failed: %d", (int)index, (int)rc);
            rc = EINVAL;
            break;
        }
    }

    RTMemFree(Info);
    lck_rw_unlock_shared(pVnodeData->pLock);

    return rc;
}

static int
vboxvfs_vnode_access(struct vnop_access_args *args)
{
    PDEBUG("here");
    return 0;
}


static int
vboxvfs_vnode_readdirattr(struct vnop_readdirattr_args *args)
{
    PDEBUG("here");
    return 0;
}

static int
vboxvfs_vnode_pathconf(struct vnop_pathconf_args *args)
{
    PDEBUG("here");
    return 0;
}

/**
 * VBoxVFS reclaim callback.
 * Called when vnode is going to be deallocated. Should release
 * all the VBoxVFS resources that correspond to current vnode object.
 *
 * @param pArgs     Operation arguments passed from VFS layer.
 *
 * @return 0 on success, BSD error code otherwise.
 */
static int
vboxvfs_vnode_reclaim(struct vnop_reclaim_args *pArgs)
{
    PDEBUG("Releasing vnode resources...");

    AssertReturn(pArgs, EINVAL);

    vnode_t          pVnode;
    vboxvfs_vnode_t *pVnodeData;
    vboxvfs_mount_t *pMount;
    mount_t          mp;

    pVnode = pArgs->a_vp;
    AssertReturn(pVnode, EINVAL);

    mp = vnode_mount(pVnode);
    AssertReturn(mp, EINVAL);

    pMount = (vboxvfs_mount_t *)vfs_fsprivate(mp);
    AssertReturn(pMount, EINVAL);

    pVnodeData = (vboxvfs_vnode_t *)vnode_fsnode(pVnode);
    AssertReturn(pVnodeData, EINVAL);
    AssertReturn(pVnodeData->pPath, EINVAL);
    AssertReturn(pVnodeData->pLockAttr, EINVAL);
    AssertReturn(pVnodeData->pLock, EINVAL);

    RTMemFree(pVnodeData->pPath);
    pVnodeData->pPath = NULL;

    lck_rw_free(pVnodeData->pLock, pMount->pLockGroup);
    pVnodeData->pLock = NULL;

    lck_attr_free(pVnodeData->pLockAttr);
    pVnodeData->pLockAttr = NULL;

    return 0;
}

/* Directory vnode operations */
static struct vnodeopv_entry_desc oVBoxVFSDirOpsDescList[] = {
    { &vnop_default_desc,     (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_lookup_desc,      (VNODEOPFUNC)vboxvfs_vnode_lookup },
    { &vnop_create_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_whiteout_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_mknod_desc,       (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_open_desc,        (VNODEOPFUNC)vboxvfs_vnode_open },
    { &vnop_close_desc,       (VNODEOPFUNC)vboxvfs_vnode_close },
    { &vnop_access_desc,      (VNODEOPFUNC)vboxvfs_vnode_access },
    { &vnop_getattr_desc,     (VNODEOPFUNC)vboxvfs_vnode_getattr },
    { &vnop_setattr_desc,     (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_read_desc,        (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_write_desc,       (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_ioctl_desc,       (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_select_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_exchange_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_revoke_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_mmap_desc,        (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_mnomap_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_fsync_desc,       (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_remove_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_link_desc,        (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_rename_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_mkdir_desc,       (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_rmdir_desc,       (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_symlink_desc,     (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_readdir_desc,     (VNODEOPFUNC)vboxvfs_vnode_readdir },
    { &vnop_readdirattr_desc, (VNODEOPFUNC)vboxvfs_vnode_readdirattr },
    { &vnop_readlink_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_inactive_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_reclaim_desc,     (VNODEOPFUNC)vboxvfs_vnode_reclaim },
    /* { &vnop_print_desc,       (VNODEOPFUNC)vboxvfs_dfl_error }, undefined in ML */
    { &vnop_pathconf_desc,    (VNODEOPFUNC)vboxvfs_vnode_pathconf },
    { &vnop_advlock_desc,     (VNODEOPFUNC)vboxvfs_dfl_error },
    /* { &vnop_truncate_desc,    (VNODEOPFUNC)vboxvfs_dfl_error }, undefined in ML */
    { &vnop_allocate_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_pagein_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_pageout_desc,     (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_searchfs_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_copyfile_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_blktooff_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_offtoblk_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_blockmap_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_strategy_desc,    (VNODEOPFUNC)vboxvfs_dfl_error },
    { &vnop_bwrite_desc,      (VNODEOPFUNC)vboxvfs_dfl_error },
    { NULL,                   (VNODEOPFUNC)NULL              },
};

int (**g_VBoxVFSVnodeDirOpsVector)(void *);

static struct vnodeopv_desc oVBoxVFSVnodeDirOps = {
    &g_VBoxVFSVnodeDirOpsVector,
    oVBoxVFSDirOpsDescList
};

struct vnodeopv_desc *g_VBoxVFSVnodeOpvDescList[] = {
    &oVBoxVFSVnodeDirOps,
};

int g_cVBoxVFSVnodeOpvDescListSize =
    sizeof(**g_VBoxVFSVnodeOpvDescList) / sizeof(struct vnodeopv_desc);

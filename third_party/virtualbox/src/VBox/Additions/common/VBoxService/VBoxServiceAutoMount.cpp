/* $Id: VBoxServiceAutoMount.cpp $ */
/** @file
 * VBoxService - Auto-mounting for Shared Folders, only Linux & Solaris atm.
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


/** @page pg_vgsvc_automount VBoxService - Shared Folder Automounter
 *
 * The Shared Folder Automounter subservice mounts shared folders upon request
 * from the host.
 *
 * This retrieves shared folder automount requests from Main via the VMMDev.
 * The current implemention only does this once, for some inexplicable reason,
 * so the run-time addition of automounted shared folders are not heeded.
 *
 * This subservice is only used on linux and solaris.  On Windows the current
 * thinking is this is better of done from VBoxTray, some one argue that for
 * drive letter assigned shared folders it would be better to do some magic here
 * (obviously not involving NDAddConnection).
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/assert.h>
#include <iprt/dir.h>
#include <iprt/mem.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <VBox/VBoxGuestLib.h>
#include "VBoxServiceInternal.h"
#include "VBoxServiceUtils.h"

#include <errno.h>
#include <grp.h>
#include <sys/mount.h>
#ifdef RT_OS_SOLARIS
# include <sys/mntent.h>
# include <sys/mnttab.h>
# include <sys/vfs.h>
#else
# include <mntent.h>
# include <paths.h>
#endif
#include <unistd.h>

RT_C_DECLS_BEGIN
#include "../../linux/sharedfolders/vbsfmount.h"
RT_C_DECLS_END

#ifdef RT_OS_SOLARIS
# define VBOXSERVICE_AUTOMOUNT_DEFAULT_DIR       "/mnt"
#else
# define VBOXSERVICE_AUTOMOUNT_DEFAULT_DIR       "/media"
#endif

#ifndef _PATH_MOUNTED
# ifdef RT_OS_SOLARIS
#  define _PATH_MOUNTED                          "/etc/mnttab"
# else
#  define _PATH_MOUNTED                          "/etc/mtab"
# endif
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The semaphore we're blocking on. */
static RTSEMEVENTMULTI  g_AutoMountEvent = NIL_RTSEMEVENTMULTI;
/** The Shared Folders service client ID. */
static uint32_t         g_SharedFoldersSvcClientID = 0;


/**
 * @interface_method_impl{VBOXSERVICE,pfnInit}
 */
static DECLCALLBACK(int) vbsvcAutoMountInit(void)
{
    VGSvcVerbose(3, "vbsvcAutoMountInit\n");

    int rc = RTSemEventMultiCreate(&g_AutoMountEvent);
    AssertRCReturn(rc, rc);

    rc = VbglR3SharedFolderConnect(&g_SharedFoldersSvcClientID);
    if (RT_SUCCESS(rc))
    {
        VGSvcVerbose(3, "vbsvcAutoMountInit: Service Client ID: %#x\n", g_SharedFoldersSvcClientID);
    }
    else
    {
        /* If the service was not found, we disable this service without
           causing VBoxService to fail. */
        if (rc == VERR_HGCM_SERVICE_NOT_FOUND) /* Host service is not available. */
        {
            VGSvcVerbose(0, "vbsvcAutoMountInit: Shared Folders service is not available\n");
            rc = VERR_SERVICE_DISABLED;
        }
        else
            VGSvcError("Control: Failed to connect to the Shared Folders service! Error: %Rrc\n", rc);
        RTSemEventMultiDestroy(g_AutoMountEvent);
        g_AutoMountEvent = NIL_RTSEMEVENTMULTI;
    }

    return rc;
}


/**
 * @todo Integrate into RTFsQueryMountpoint()?
 */
static bool vbsvcAutoMountShareIsMounted(const char *pszShare, char *pszMountPoint, size_t cbMountPoint)
{
    AssertPtrReturn(pszShare, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszMountPoint, VERR_INVALID_PARAMETER);
    AssertReturn(cbMountPoint, VERR_INVALID_PARAMETER);

    bool fMounted = false;
    /** @todo What to do if we have a relative path in mtab instead
     *       of an absolute one ("temp" vs. "/media/temp")?
     * procfs contains the full path but not the actual share name ...
     * FILE *pFh = setmntent("/proc/mounts", "r+t"); */
#ifdef RT_OS_SOLARIS
    FILE *pFh = fopen(_PATH_MOUNTED, "r");
    if (!pFh)
        VGSvcError("vbsvcAutoMountShareIsMounted: Could not open mount tab '%s'!\n", _PATH_MOUNTED);
    else
    {
        mnttab mntTab;
        while ((getmntent(pFh, &mntTab)))
        {
            if (!RTStrICmp(mntTab.mnt_special, pszShare))
            {
                fMounted = RTStrPrintf(pszMountPoint, cbMountPoint, "%s", mntTab.mnt_mountp)
                         ? true : false;
                break;
            }
        }
        fclose(pFh);
    }
#else
    FILE *pFh = setmntent(_PATH_MOUNTED, "r+t"); /** @todo r=bird: why open it for writing? (the '+') */
    if (pFh == NULL)
        VGSvcError("vbsvcAutoMountShareIsMounted: Could not open mount tab '%s'!\n", _PATH_MOUNTED);
    else
    {
        mntent *pMntEnt;
        while ((pMntEnt = getmntent(pFh)))
        {
            if (!RTStrICmp(pMntEnt->mnt_fsname, pszShare))
            {
                fMounted = RTStrPrintf(pszMountPoint, cbMountPoint, "%s", pMntEnt->mnt_dir)
                         ? true : false;
                break;
            }
        }
        endmntent(pFh);
    }
#endif

    VGSvcVerbose(4, "vbsvcAutoMountShareIsMounted: Share '%s' at mount point '%s' = %s\n",
                       pszShare, fMounted ? pszMountPoint : "<None>", fMounted ? "Yes" : "No");
    return fMounted;
}


/**
 * Unmounts a shared folder.
 *
 * @returns VBox status code
 * @param   pszMountPoint   The shared folder mount point.
 */
static int vbsvcAutoMountUnmount(const char *pszMountPoint)
{
    AssertPtrReturn(pszMountPoint, VERR_INVALID_PARAMETER);

    int rc = VINF_SUCCESS;
    uint8_t uTries = 0;
    int r;
    while (uTries++ < 3)
    {
        r = umount(pszMountPoint);
        if (r == 0)
            break;
/** @todo r=bird: Why do sleep 5 seconds after the final retry?
 *  May also be a good idea to check for EINVAL or other signs that someone
 *  else have already unmounted the share. */
        RTThreadSleep(5000); /* Wait a while ... */
    }
    if (r == -1)  /** @todo r=bird: RTThreadSleep set errno.  */
        rc = RTErrConvertFromErrno(errno);
    return rc;
}


/**
 * Prepares a mount point (create it, set group and mode).
 *
 * @returns VBox status code
 * @param   pszMountPoint   The mount point.
 * @param   pszShareName    Unused.
 * @param   pOpts           For getting the group ID.
 */
static int vbsvcAutoMountPrepareMountPoint(const char *pszMountPoint, const char *pszShareName, vbsf_mount_opts *pOpts)
{
    AssertPtrReturn(pOpts, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszMountPoint, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszShareName, VERR_INVALID_PARAMETER);

    RTFMODE fMode = RTFS_UNIX_IRWXU | RTFS_UNIX_IRWXG; /* Owner (=root) and the group (=vboxsf) have full access. */
    int rc = RTDirCreateFullPath(pszMountPoint, fMode);
    if (RT_SUCCESS(rc))
    {
        rc = RTPathSetOwnerEx(pszMountPoint, NIL_RTUID /* Owner, unchanged */, pOpts->gid, RTPATH_F_ON_LINK);
        if (RT_SUCCESS(rc))
        {
            rc = RTPathSetMode(pszMountPoint, fMode);
            if (RT_FAILURE(rc))
            {
                if (rc == VERR_WRITE_PROTECT)
                {
                    VGSvcVerbose(3, "vbsvcAutoMountPrepareMountPoint: Mount directory '%s' already is used/mounted\n",
                                       pszMountPoint);
                    rc = VINF_SUCCESS;
                }
                else
                    VGSvcError("vbsvcAutoMountPrepareMountPoint: Could not set mode %RTfmode for mount directory '%s', rc = %Rrc\n",
                                     fMode, pszMountPoint, rc);
            }
        }
        else
            VGSvcError("vbsvcAutoMountPrepareMountPoint: Could not set permissions for mount directory '%s', rc = %Rrc\n",
                             pszMountPoint, rc);
    }
    else
        VGSvcError("vbsvcAutoMountPrepareMountPoint: Could not create mount directory '%s' with mode %RTfmode, rc = %Rrc\n",
                         pszMountPoint, fMode, rc);
    return rc;
}


/**
 * Mounts a shared folder.
 *
 * @returns VBox status code reflecting unmount and mount point preparation
 *          results, but not actual mounting
 *
 * @param   pszShareName    The shared folder name.
 * @param   pszMountPoint   The mount point.
 * @param   pOpts           The mount options.
 */
static int vbsvcAutoMountSharedFolder(const char *pszShareName, const char *pszMountPoint, struct vbsf_mount_opts *pOpts)
{
    AssertPtr(pOpts);

    int rc = VINF_SUCCESS;
    bool fSkip = false;

    /* Already mounted? */
    char szAlreadyMountedTo[RTPATH_MAX];
    if (vbsvcAutoMountShareIsMounted(pszShareName, szAlreadyMountedTo, sizeof(szAlreadyMountedTo)))
    {
        fSkip = true;
        /* Do if it not mounted to our desired mount point */
        if (RTStrICmp(pszMountPoint, szAlreadyMountedTo))
        {
            VGSvcVerbose(3, "vbsvcAutoMountWorker: Shared folder '%s' already mounted to '%s', unmounting ...\n",
                               pszShareName, szAlreadyMountedTo);
            rc = vbsvcAutoMountUnmount(szAlreadyMountedTo);
            if (RT_SUCCESS(rc))
                fSkip = false;
            else
                VGSvcError("vbsvcAutoMountWorker: Failed to unmount '%s', %s (%d)! (rc=%Rrc)\n",
                                 szAlreadyMountedTo, strerror(errno), errno, rc); /** @todo errno isn't reliable at this point */
        }
        if (fSkip)
            VGSvcVerbose(3, "vbsvcAutoMountWorker: Shared folder '%s' already mounted to '%s', skipping\n",
                               pszShareName, szAlreadyMountedTo);
    }

    if (!fSkip && RT_SUCCESS(rc))
        rc = vbsvcAutoMountPrepareMountPoint(pszMountPoint, pszShareName, pOpts);
    if (!fSkip && RT_SUCCESS(rc))
    {
#ifdef RT_OS_SOLARIS
        char szOptBuf[MAX_MNTOPT_STR] = { '\0', };
        int fFlags = 0;
        if (pOpts->ronly)
            fFlags |= MS_RDONLY;
        RTStrPrintf(szOptBuf, sizeof(szOptBuf), "uid=%d,gid=%d,dmode=%0o,fmode=%0o,dmask=%0o,fmask=%0o",
                    pOpts->uid, pOpts->gid, pOpts->dmode, pOpts->fmode, pOpts->dmask, pOpts->fmask);
        int r = mount(pszShareName,
                      pszMountPoint,
                      fFlags | MS_OPTIONSTR,
                      "vboxfs",
                      NULL,                     /* char *dataptr */
                      0,                        /* int datalen */
                      szOptBuf,
                      sizeof(szOptBuf));
        if (r == 0)
            VGSvcVerbose(0, "vbsvcAutoMountWorker: Shared folder '%s' was mounted to '%s'\n", pszShareName, pszMountPoint);
        else if (errno != EBUSY) /* Share is already mounted? Then skip error msg. */
            VGSvcError("vbsvcAutoMountWorker: Could not mount shared folder '%s' to '%s', error = %s\n",
                             pszShareName, pszMountPoint, strerror(errno));

#elif defined(RT_OS_LINUX)
        unsigned long fFlags = MS_NODEV;

        /*const char *szOptions = { "rw" }; - ??? */
        struct vbsf_mount_info_new mntinf;

        mntinf.nullchar     = '\0';
        mntinf.signature[0] = VBSF_MOUNT_SIGNATURE_BYTE_0;
        mntinf.signature[1] = VBSF_MOUNT_SIGNATURE_BYTE_1;
        mntinf.signature[2] = VBSF_MOUNT_SIGNATURE_BYTE_2;
        mntinf.length       = sizeof(mntinf);

        mntinf.uid   = pOpts->uid;
        mntinf.gid   = pOpts->gid;
        mntinf.ttl   = pOpts->ttl;
        mntinf.dmode = pOpts->dmode;
        mntinf.fmode = pOpts->fmode;
        mntinf.dmask = pOpts->dmask;
        mntinf.fmask = pOpts->fmask;

        strcpy(mntinf.name, pszShareName);
        strcpy(mntinf.nls_name, "\0");

        int r = mount(pszShareName,
                      pszMountPoint,
                      "vboxsf",
                      fFlags,
                      &mntinf);
        if (r == 0)
        {
            VGSvcVerbose(0, "vbsvcAutoMountWorker: Shared folder '%s' was mounted to '%s'\n", pszShareName, pszMountPoint);

            r = vbsfmount_complete(pszShareName, pszMountPoint, fFlags, pOpts);
            switch (r)
            {
                case 0: /* Success. */
                    errno = 0; /* Clear all errors/warnings. */
                    break;

                case 1:
                    VGSvcError("vbsvcAutoMountWorker: Could not update mount table (failed to create memstream): %s\n",
                                     strerror(errno));
                    break;

                case 2:
                    VGSvcError("vbsvcAutoMountWorker: Could not open mount table for update: %s\n", strerror(errno));
                    break;

                case 3:
                    /* VGSvcError("vbsvcAutoMountWorker: Could not add an entry to the mount table: %s\n", strerror(errno)); */
                    errno = 0;
                    break;

                default:
                    VGSvcError("vbsvcAutoMountWorker: Unknown error while completing mount operation: %d\n", r);
                    break;
            }
        }
        else /* r == -1, we got some error in errno.  */
        {
            if (errno == EPROTO)
            {
                VGSvcVerbose(3, "vbsvcAutoMountWorker: Messed up share name, re-trying ...\n");

                /** @todo r=bird: What on earth is going on here?????  Why can't you
                 *        strcpy(mntinf.name, pszShareName) to fix it again? */

                /* Sometimes the mount utility messes up the share name.  Try to
                 * un-mangle it again. */
                char szCWD[RTPATH_MAX];
                size_t cchCWD;
                if (!getcwd(szCWD, sizeof(szCWD)))
                {
                    VGSvcError("vbsvcAutoMountWorker: Failed to get the current working directory\n");
                    szCWD[0] = '\0';
                }
                cchCWD = strlen(szCWD);
                if (!strncmp(pszMountPoint, szCWD, cchCWD))
                {
                    while (pszMountPoint[cchCWD] == '/')
                        ++cchCWD;
                    /* We checked before that we have enough space */
                    strcpy(mntinf.name, pszMountPoint + cchCWD);
                }
                r = mount(mntinf.name, pszMountPoint, "vboxsf", fFlags, &mntinf);
            }
            if (r == -1) /* Was there some error from one of the tries above? */
            {
                switch (errno)
                {
                    /* If we get EINVAL here, the system already has mounted the Shared Folder to another
                     * mount point. */
                    case EINVAL:
                        VGSvcVerbose(0, "vbsvcAutoMountWorker: Shared folder '%s' already is mounted!\n", pszShareName);
                        /* Ignore this error! */
                        break;
                    case EBUSY:
                        /* Ignore these errors! */
                        break;

                    default:
                        VGSvcError("vbsvcAutoMountWorker: Could not mount shared folder '%s' to '%s': %s (%d)\n",
                                         pszShareName, pszMountPoint, strerror(errno), errno);
                        rc = RTErrConvertFromErrno(errno);
                        break;
                }
            }
        }
#else
# error "PORTME"
#endif
    }
    VGSvcVerbose(3, "vbsvcAutoMountWorker: Mounting returned with rc=%Rrc\n", rc);
    return rc;
}


/**
 * Processes shared folder mappings retrieved from the host.
 *
 * @returns VBox status code.
 * @param   paMappings      The mappings.
 * @param   cMappings       The number of mappings.
 * @param   pszMountDir     The mount directory.
 * @param   pszSharePrefix  The share prefix.
 * @param   uClientID       The shared folder service (HGCM) client ID.
 */
static int vbsvcAutoMountProcessMappings(PCVBGLR3SHAREDFOLDERMAPPING paMappings, uint32_t cMappings,
                                         const char *pszMountDir, const char *pszSharePrefix, uint32_t uClientID)
{
    if (cMappings == 0)
        return VINF_SUCCESS;
    AssertPtrReturn(paMappings, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszMountDir, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszSharePrefix, VERR_INVALID_PARAMETER);
    AssertReturn(uClientID > 0, VERR_INVALID_PARAMETER);

    /** @todo r=bird: Why is this loop schitzoid about status codes? It quits if
     * RTPathJoin fails (i.e. if the user specifies a very long name), but happily
     * continues if RTStrAPrintf failes (mem alloc).
     *
     * It also happily continues if the 'vboxsf' group is missing, which is a waste
     * of effort... In fact, retrieving the group ID could probably be done up
     * front, outside the loop. */
    int rc = VINF_SUCCESS;
    for (uint32_t i = 0; i < cMappings && RT_SUCCESS(rc); i++)
    {
        char *pszShareName = NULL;
        rc = VbglR3SharedFolderGetName(uClientID, paMappings[i].u32Root, &pszShareName);
        if (   RT_SUCCESS(rc)
            && *pszShareName)
        {
            VGSvcVerbose(3, "vbsvcAutoMountWorker: Connecting share %u (%s) ...\n", i+1, pszShareName);

            /** @todo r=bird: why do you copy things twice here and waste heap space?
             * szMountPoint has a fixed size.
             * @code
             * char szMountPoint[RTPATH_MAX];
             * rc = RTPathJoin(szMountPoint, sizeof(szMountPoint), pszMountDir, *pszSharePrefix ? pszSharePrefix : pszShareName);
             * if (RT_SUCCESS(rc) && *pszSharePrefix)
             *     rc = RTStrCat(szMountPoint, sizeof(szMountPoint), pszShareName);
             * @endcode */
            char *pszShareNameFull = NULL;
            if (RTStrAPrintf(&pszShareNameFull, "%s%s", pszSharePrefix, pszShareName) > 0)
            {
                char szMountPoint[RTPATH_MAX];
                rc = RTPathJoin(szMountPoint, sizeof(szMountPoint), pszMountDir, pszShareNameFull);
                if (RT_SUCCESS(rc))
                {
                    VGSvcVerbose(4, "vbsvcAutoMountWorker: Processing mount point '%s'\n", szMountPoint);

                    struct group *grp_vboxsf = getgrnam("vboxsf");
                    if (grp_vboxsf)
                    {
                        struct vbsf_mount_opts mount_opts =
                        {
                            0,                     /* uid */
                            (int)grp_vboxsf->gr_gid, /* gid */
                            0,                     /* ttl */
                            0770,                  /* dmode, owner and group "vboxsf" have full access */
                            0770,                  /* fmode, owner and group "vboxsf" have full access */
                            0,                     /* dmask */
                            0,                     /* fmask */
                            0,                     /* ronly */
                            0,                     /* sloppy */
                            0,                     /* noexec */
                            0,                     /* nodev */
                            0,                     /* nosuid */
                            0,                     /* remount */
                            "\0",                  /* nls_name */
                            NULL,                  /* convertcp */
                        };

                        rc = vbsvcAutoMountSharedFolder(pszShareName, szMountPoint, &mount_opts);
                    }
                    else
                        VGSvcError("vbsvcAutoMountWorker: Group 'vboxsf' does not exist\n");
                }
                else
                    VGSvcError("vbsvcAutoMountWorker: Unable to join mount point/prefix/shrae, rc = %Rrc\n", rc);
                RTStrFree(pszShareNameFull);
            }
            else
                VGSvcError("vbsvcAutoMountWorker: Unable to allocate full share name\n");
            RTStrFree(pszShareName);
        }
        else
            VGSvcError("vbsvcAutoMountWorker: Error while getting the shared folder name for root node = %u, rc = %Rrc\n",
                             paMappings[i].u32Root, rc);
    } /* for cMappings. */
    return rc;
}


/**
 * @interface_method_impl{VBOXSERVICE,pfnWorker}
 */
static DECLCALLBACK(int) vbsvcAutoMountWorker(bool volatile *pfShutdown)
{
    /*
     * Tell the control thread that it can continue
     * spawning services.
     */
    RTThreadUserSignal(RTThreadSelf());

    uint32_t cMappings;
    PVBGLR3SHAREDFOLDERMAPPING paMappings;
    int rc = VbglR3SharedFolderGetMappings(g_SharedFoldersSvcClientID, true /* Only process auto-mounted folders */,
                                           &paMappings, &cMappings);
    if (   RT_SUCCESS(rc)
        && cMappings)
    {
        char *pszMountDir;
        rc = VbglR3SharedFolderGetMountDir(&pszMountDir);
        if (rc == VERR_NOT_FOUND)
            rc = RTStrDupEx(&pszMountDir, VBOXSERVICE_AUTOMOUNT_DEFAULT_DIR);
        if (RT_SUCCESS(rc))
        {
            VGSvcVerbose(3, "vbsvcAutoMountWorker: Shared folder mount dir set to '%s'\n", pszMountDir);

            char *pszSharePrefix;
            rc = VbglR3SharedFolderGetMountPrefix(&pszSharePrefix);
            if (RT_SUCCESS(rc))
            {
                VGSvcVerbose(3, "vbsvcAutoMountWorker: Shared folder mount prefix set to '%s'\n", pszSharePrefix);
#ifdef USE_VIRTUAL_SHARES
                /* Check for a fixed/virtual auto-mount share. */
                if (VbglR3SharedFolderExists(g_SharedFoldersSvcClientID, "vbsfAutoMount"))
                {
                    VGSvcVerbose(3, "vbsvcAutoMountWorker: Host supports auto-mount root\n");
                }
                else
                {
#endif
                    VGSvcVerbose(3, "vbsvcAutoMountWorker: Got %u shared folder mappings\n", cMappings);
                    rc = vbsvcAutoMountProcessMappings(paMappings, cMappings, pszMountDir, pszSharePrefix, g_SharedFoldersSvcClientID);
#ifdef USE_VIRTUAL_SHARES
                }
#endif
                RTStrFree(pszSharePrefix);
            } /* Mount share prefix. */
            else
                VGSvcError("vbsvcAutoMountWorker: Error while getting the shared folder mount prefix, rc = %Rrc\n", rc);
            RTStrFree(pszMountDir);
        }
        else
            VGSvcError("vbsvcAutoMountWorker: Error while getting the shared folder directory, rc = %Rrc\n", rc);
        VbglR3SharedFolderFreeMappings(paMappings);
    }
    else if (RT_FAILURE(rc))
        VGSvcError("vbsvcAutoMountWorker: Error while getting the shared folder mappings, rc = %Rrc\n", rc);
    else
        VGSvcVerbose(3, "vbsvcAutoMountWorker: No shared folder mappings found\n");

    /*
     * Because this thread is a one-timer at the moment we don't want to break/change
     * the semantics of the main thread's start/stop sub-threads handling.
     *
     * This thread exits so fast while doing its own startup in VGSvcStartServices()
     * that this->fShutdown flag is set to true in VGSvcThread() before we have the
     * chance to check for a service failure in VGSvcStartServices() to indicate
     * a VBoxService startup error.
     *
     * Therefore *no* service threads are allowed to quit themselves and need to wait
     * for the pfShutdown flag to be set by the main thread.
     */
/** @todo r=bird: Shared folders have always been configurable at run time, so
 * this service must be changed to check for changes and execute those changes!
 *
 * The 0.5sec sleep here is just soo crude and must go!
 */
    for (;;)
    {
        /* Do we need to shutdown? */
        if (*pfShutdown)
            break;

        /* Let's sleep for a bit and let others run ... */
        RTThreadSleep(500);
    }

    VGSvcVerbose(3, "vbsvcAutoMountWorker: Finished with rc=%Rrc\n", rc);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{VBOXSERVICE,pfnTerm}
 */
static DECLCALLBACK(void) vbsvcAutoMountTerm(void)
{
    VGSvcVerbose(3, "vbsvcAutoMountTerm\n");

    VbglR3SharedFolderDisconnect(g_SharedFoldersSvcClientID);
    g_SharedFoldersSvcClientID = 0;

    if (g_AutoMountEvent != NIL_RTSEMEVENTMULTI)
    {
        RTSemEventMultiDestroy(g_AutoMountEvent);
        g_AutoMountEvent = NIL_RTSEMEVENTMULTI;
    }
    return;
}


/**
 * @interface_method_impl{VBOXSERVICE,pfnStop}
 */
static DECLCALLBACK(void) vbsvcAutoMountStop(void)
{
    RTSemEventMultiSignal(g_AutoMountEvent);
}


/**
 * The 'automount' service description.
 */
VBOXSERVICE g_AutoMount =
{
    /* pszName. */
    "automount",
    /* pszDescription. */
    "Auto-mount for Shared Folders",
    /* pszUsage. */
    NULL,
    /* pszOptions. */
    NULL,
    /* methods */
    VGSvcDefaultPreInit,
    VGSvcDefaultOption,
    vbsvcAutoMountInit,
    vbsvcAutoMountWorker,
    vbsvcAutoMountStop,
    vbsvcAutoMountTerm
};

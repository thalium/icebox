/* $Id: process-creation-posix.cpp $ */
/** @file
 * IPRT - Process Creation, POSIX.
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
#define LOG_GROUP RTLOGGROUP_PROCESS
#include <iprt/cdefs.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>
#if defined(RT_OS_LINUX) || defined(RT_OS_SOLARIS)
# include <crypt.h>
# include <shadow.h>
#endif

#if defined(RT_OS_LINUX) || defined(RT_OS_OS2)
/* While Solaris has posix_spawn() of course we don't want to use it as
 * we need to have the child in a different process contract, no matter
 * whether it is started detached or not. */
# define HAVE_POSIX_SPAWN 1
#endif
#if defined(RT_OS_DARWIN) && defined(MAC_OS_X_VERSION_MIN_REQUIRED)
# if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
#  define HAVE_POSIX_SPAWN 1
# endif
#endif
#ifdef HAVE_POSIX_SPAWN
# include <spawn.h>
#endif

#ifdef RT_OS_DARWIN
# include <mach-o/dyld.h>
#endif
#ifdef RT_OS_SOLARIS
# include <limits.h>
# include <sys/ctfs.h>
# include <sys/contract/process.h>
# include <libcontract.h>
#endif

#ifndef RT_OS_SOLARIS
# include <paths.h>
#else
# define _PATH_MAILDIR "/var/mail"
# define _PATH_DEFPATH "/usr/bin:/bin"
# define _PATH_STDPATH "/sbin:/usr/sbin:/bin:/usr/bin"
#endif


#include <iprt/process.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/pipe.h>
#include <iprt/socket.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include "internal/process.h"


/**
 * Check the credentials and return the gid/uid of user.
 *
 * @param    pszUser     username
 * @param    pszPasswd   password
 * @param    gid         where to store the GID of the user
 * @param    uid         where to store the UID of the user
 * @returns IPRT status code
 */
static int rtCheckCredentials(const char *pszUser, const char *pszPasswd, gid_t *pGid, uid_t *pUid)
{
#if defined(RT_OS_LINUX)
    struct passwd *pw;

    pw = getpwnam(pszUser);
    if (!pw)
        return VERR_AUTHENTICATION_FAILURE;

    if (!pszPasswd)
        pszPasswd = "";

    struct spwd *spwd;
    /* works only if /etc/shadow is accessible */
    spwd = getspnam(pszUser);
    if (spwd)
        pw->pw_passwd = spwd->sp_pwdp;

    /* Default fCorrect=true if no password specified. In that case, pw->pw_passwd
     * must be NULL (no password set for this user). Fail if a password is specified
     * but the user does not have one assigned. */
    int fCorrect = !pszPasswd || !*pszPasswd;
    if (pw->pw_passwd && *pw->pw_passwd)
    {
        struct crypt_data *data = (struct crypt_data*)RTMemTmpAllocZ(sizeof(*data));
        /* be reentrant */
        char *pszEncPasswd = crypt_r(pszPasswd, pw->pw_passwd, data);
        fCorrect = pszEncPasswd && !strcmp(pszEncPasswd, pw->pw_passwd);
        RTMemTmpFree(data);
    }
    if (!fCorrect)
        return VERR_AUTHENTICATION_FAILURE;

    *pGid = pw->pw_gid;
    *pUid = pw->pw_uid;
    return VINF_SUCCESS;

#elif defined(RT_OS_SOLARIS)
    struct passwd *ppw, pw;
    char szBuf[1024];

    if (getpwnam_r(pszUser, &pw, szBuf, sizeof(szBuf), &ppw) != 0 || ppw == NULL)
        return VERR_AUTHENTICATION_FAILURE;

    if (!pszPasswd)
        pszPasswd = "";

    struct spwd spwd;
    char szPwdBuf[1024];
    /* works only if /etc/shadow is accessible */
    if (getspnam_r(pszUser, &spwd, szPwdBuf, sizeof(szPwdBuf)) != NULL)
        ppw->pw_passwd = spwd.sp_pwdp;

    char *pszEncPasswd = crypt(pszPasswd, ppw->pw_passwd);
    if (strcmp(pszEncPasswd, ppw->pw_passwd))
        return VERR_AUTHENTICATION_FAILURE;

    *pGid = ppw->pw_gid;
    *pUid = ppw->pw_uid;
    return VINF_SUCCESS;

#else
    NOREF(pszUser); NOREF(pszPasswd); NOREF(pGid); NOREF(pUid);
    return VERR_AUTHENTICATION_FAILURE;
#endif
}


#ifdef RT_OS_SOLARIS
/** @todo the error reporting of the Solaris process contract code could be
 * a lot better, but essentially it is not meant to run into errors after
 * the debugging phase. */
static int rtSolarisContractPreFork(void)
{
    int templateFd = open64(CTFS_ROOT "/process/template", O_RDWR);
    if (templateFd < 0)
        return -1;

    /* Set template parameters and event sets. */
    if (ct_pr_tmpl_set_param(templateFd, CT_PR_PGRPONLY))
    {
        close(templateFd);
        return -1;
    }
    if (ct_pr_tmpl_set_fatal(templateFd, CT_PR_EV_HWERR))
    {
        close(templateFd);
        return -1;
    }
    if (ct_tmpl_set_critical(templateFd, 0))
    {
        close(templateFd);
        return -1;
    }
    if (ct_tmpl_set_informative(templateFd, CT_PR_EV_HWERR))
    {
        close(templateFd);
        return -1;
    }

    /* Make this the active template for the process. */
    if (ct_tmpl_activate(templateFd))
    {
        close(templateFd);
        return -1;
    }

    return templateFd;
}

static void rtSolarisContractPostForkChild(int templateFd)
{
    if (templateFd == -1)
        return;

    /* Clear the active template. */
    ct_tmpl_clear(templateFd);
    close(templateFd);
}

static void rtSolarisContractPostForkParent(int templateFd, pid_t pid)
{
    if (templateFd == -1)
        return;

    /* Clear the active template. */
    int cleared = ct_tmpl_clear(templateFd);
    close(templateFd);

    /* If the clearing failed or the fork failed there's nothing more to do. */
    if (cleared || pid <= 0)
        return;

    /* Look up the contract which was created by this thread. */
    int statFd = open64(CTFS_ROOT "/process/latest", O_RDONLY);
    if (statFd == -1)
        return;
    ct_stathdl_t statHdl;
    if (ct_status_read(statFd, CTD_COMMON, &statHdl))
    {
        close(statFd);
        return;
    }
    ctid_t ctId = ct_status_get_id(statHdl);
    ct_status_free(statHdl);
    close(statFd);
    if (ctId < 0)
        return;

    /* Abandon this contract we just created. */
    char ctlPath[PATH_MAX];
    size_t len = snprintf(ctlPath, sizeof(ctlPath),
                          CTFS_ROOT "/process/%ld/ctl", (long)ctId);
    if (len >= sizeof(ctlPath))
        return;
    int ctlFd = open64(ctlPath, O_WRONLY);
    if (statFd == -1)
        return;
    if (ct_ctl_abandon(ctlFd) < 0)
    {
        close(ctlFd);
        return;
    }
    close(ctlFd);
}

#endif /* RT_OS_SOLARIS */


RTR3DECL(int)   RTProcCreate(const char *pszExec, const char * const *papszArgs, RTENV Env, unsigned fFlags, PRTPROCESS pProcess)
{
    return RTProcCreateEx(pszExec, papszArgs, Env, fFlags,
                          NULL, NULL, NULL,  /* standard handles */
                          NULL /*pszAsUser*/, NULL /* pszPassword*/,
                          pProcess);
}


/**
 * Adjust the profile environment after forking the child process and changing
 * the UID.
 *
 * @returns IRPT status code.
 * @param   hEnvToUse       The environment we're going to use with execve.
 * @param   fFlags          The process creation flags.
 * @param   hEnv            The environment passed in by the user.
 */
static int rtProcPosixAdjustProfileEnvFromChild(RTENV hEnvToUse, uint32_t fFlags, RTENV hEnv)
{
    int rc = VINF_SUCCESS;
#ifdef RT_OS_DARWIN
    if (   RT_SUCCESS(rc)
        && (!(fFlags & RTPROC_FLAGS_ENV_CHANGE_RECORD) || RTEnvExistEx(hEnv, "TMPDIR")) )
    {
        char szValue[_4K];
        size_t cbNeeded = confstr(_CS_DARWIN_USER_TEMP_DIR, szValue, sizeof(szValue));
        if (cbNeeded > 0 && cbNeeded < sizeof(szValue))
        {
            char *pszTmp;
            rc = RTStrCurrentCPToUtf8(&pszTmp, szValue);
            if (RT_SUCCESS(rc))
            {
                rc = RTEnvSetEx(hEnvToUse, "TMPDIR", pszTmp);
                RTStrFree(pszTmp);
            }
        }
        else
            rc = VERR_BUFFER_OVERFLOW;
    }
#else
    RT_NOREF_PV(hEnvToUse); RT_NOREF_PV(fFlags); RT_NOREF_PV(hEnv);
#endif
    return rc;
}


/**
 * Create a very very basic environment for a user.
 *
 * @returns IPRT status code.
 * @param   phEnvToUse  Where to return the created environment.
 * @param   pszUser     The user name for the profile.
 */
static int rtProcPosixCreateProfileEnv(PRTENV phEnvToUse, const char *pszUser)
{
    struct passwd   Pwd;
    struct passwd  *pPwd = NULL;
    char            achBuf[_4K];
    int             rc;
    errno = 0;
    if (pszUser)
        rc = getpwnam_r(pszUser, &Pwd, achBuf, sizeof(achBuf), &pPwd);
    else
        rc = getpwuid_r(getuid(), &Pwd, achBuf, sizeof(achBuf), &pPwd);
    if (rc == 0 && pPwd)
    {
        char *pszDir;
        rc = RTStrCurrentCPToUtf8(&pszDir, pPwd->pw_dir);
        if (RT_SUCCESS(rc))
        {
            char *pszShell;
            rc = RTStrCurrentCPToUtf8(&pszShell, pPwd->pw_shell);
            if (RT_SUCCESS(rc))
            {
                char *pszUserFree = NULL;
                if (!pszUser)
                {
                    rc = RTStrCurrentCPToUtf8(&pszUserFree, pPwd->pw_name);
                    if (RT_SUCCESS(rc))
                        pszUser = pszUserFree;
                }
                if (RT_SUCCESS(rc))
                {
                    rc = RTEnvCreate(phEnvToUse);
                    if (RT_SUCCESS(rc))
                    {
                        RTENV hEnvToUse = *phEnvToUse;

                        rc = RTEnvSetEx(hEnvToUse, "HOME", pszDir);
                        if (RT_SUCCESS(rc))
                            rc = RTEnvSetEx(hEnvToUse, "SHELL", pszShell);
                        if (RT_SUCCESS(rc))
                            rc = RTEnvSetEx(hEnvToUse, "USER", pszUser);
                        if (RT_SUCCESS(rc))
                            rc = RTEnvSetEx(hEnvToUse, "LOGNAME", pszUser);

                        if (RT_SUCCESS(rc))
                            rc = RTEnvSetEx(hEnvToUse, "PATH", pPwd->pw_uid == 0 ? _PATH_STDPATH : _PATH_DEFPATH);

                        if (RT_SUCCESS(rc))
                        {
                            RTStrPrintf(achBuf, sizeof(achBuf), "%s/%s", _PATH_MAILDIR, pszUser);
                            rc = RTEnvSetEx(hEnvToUse, "MAIL", achBuf);
                        }

#ifdef RT_OS_DARWIN
                        if (RT_SUCCESS(rc) && !pszUserFree)
                        {
                            size_t cbNeeded = confstr(_CS_DARWIN_USER_TEMP_DIR, achBuf, sizeof(achBuf));
                            if (cbNeeded > 0 && cbNeeded < sizeof(achBuf))
                            {
                                char *pszTmp;
                                rc = RTStrCurrentCPToUtf8(&pszTmp, achBuf);
                                if (RT_SUCCESS(rc))
                                {
                                    rc = RTEnvSetEx(hEnvToUse, "TMPDIR", pszTmp);
                                    RTStrFree(pszTmp);
                                }
                            }
                            else
                                rc = VERR_BUFFER_OVERFLOW;
                        }
#endif

                        /** @todo load /etc/environment, /etc/profile.env and ~/.pam_environment? */

                        if (RT_FAILURE(rc))
                            RTEnvDestroy(hEnvToUse);
                    }
                    RTStrFree(pszUserFree);
                }
                RTStrFree(pszShell);
            }
            RTStrFree(pszDir);
        }
    }
    else
        rc = errno ? RTErrConvertFromErrno(errno) : VERR_ACCESS_DENIED;
    return rc;
}


/**
 * RTPathTraverseList callback used by RTProcCreateEx to locate the executable.
 */
static DECLCALLBACK(int) rtPathFindExec(char const *pchPath, size_t cchPath, void *pvUser1, void *pvUser2)
{
    const char *pszExec     = (const char *)pvUser1;
    char       *pszRealExec = (char *)pvUser2;
    int rc = RTPathJoinEx(pszRealExec, RTPATH_MAX, pchPath, cchPath, pszExec, RTSTR_MAX);
    if (RT_FAILURE(rc))
        return rc;
    if (!access(pszRealExec, X_OK))
        return VINF_SUCCESS;
    if (   errno == EACCES
        || errno == EPERM)
        return RTErrConvertFromErrno(errno);
    return VERR_TRY_AGAIN;
}

/**
 * Cleans up the environment on the way out.
 */
static int rtProcPosixCreateReturn(int rc, RTENV hEnvToUse, RTENV hEnv)
{
    if (hEnvToUse != hEnv)
        RTEnvDestroy(hEnvToUse);
    return rc;
}


RTR3DECL(int)   RTProcCreateEx(const char *pszExec, const char * const *papszArgs, RTENV hEnv, uint32_t fFlags,
                               PCRTHANDLE phStdIn, PCRTHANDLE phStdOut, PCRTHANDLE phStdErr, const char *pszAsUser,
                               const char *pszPassword, PRTPROCESS phProcess)
{
    int rc;

    /*
     * Input validation
     */
    AssertPtrReturn(pszExec, VERR_INVALID_POINTER);
    AssertReturn(*pszExec, VERR_INVALID_PARAMETER);
    AssertReturn(!(fFlags & ~RTPROC_FLAGS_VALID_MASK), VERR_INVALID_PARAMETER);
    AssertReturn(!(fFlags & RTPROC_FLAGS_DETACHED) || !phProcess, VERR_INVALID_PARAMETER);
    AssertReturn(hEnv != NIL_RTENV, VERR_INVALID_PARAMETER);
    AssertPtrReturn(papszArgs, VERR_INVALID_PARAMETER);
    AssertPtrNullReturn(pszAsUser, VERR_INVALID_POINTER);
    AssertReturn(!pszAsUser || *pszAsUser, VERR_INVALID_PARAMETER);
    AssertReturn(!pszPassword || pszAsUser, VERR_INVALID_PARAMETER);
    AssertPtrNullReturn(pszPassword, VERR_INVALID_POINTER);
#if defined(RT_OS_OS2)
    if (fFlags & RTPROC_FLAGS_DETACHED)
        return VERR_PROC_DETACH_NOT_SUPPORTED;
#endif

    /*
     * Get the file descriptors for the handles we've been passed.
     */
    PCRTHANDLE  paHandles[3] = { phStdIn, phStdOut, phStdErr };
    int         aStdFds[3]   = {      -1,       -1,       -1 };
    for (int i = 0; i < 3; i++)
    {
        if (paHandles[i])
        {
            AssertPtrReturn(paHandles[i], VERR_INVALID_POINTER);
            switch (paHandles[i]->enmType)
            {
                case RTHANDLETYPE_FILE:
                    aStdFds[i] = paHandles[i]->u.hFile != NIL_RTFILE
                               ? (int)RTFileToNative(paHandles[i]->u.hFile)
                               : -2 /* close it */;
                    break;

                case RTHANDLETYPE_PIPE:
                    aStdFds[i] = paHandles[i]->u.hPipe != NIL_RTPIPE
                               ? (int)RTPipeToNative(paHandles[i]->u.hPipe)
                               : -2 /* close it */;
                    break;

                case RTHANDLETYPE_SOCKET:
                    aStdFds[i] = paHandles[i]->u.hSocket != NIL_RTSOCKET
                               ? (int)RTSocketToNative(paHandles[i]->u.hSocket)
                               : -2 /* close it */;
                    break;

                default:
                    AssertMsgFailedReturn(("%d: %d\n", i, paHandles[i]->enmType), VERR_INVALID_PARAMETER);
            }
            /** @todo check the close-on-execness of these handles?  */
        }
    }

    for (int i = 0; i < 3; i++)
        if (aStdFds[i] == i)
            aStdFds[i] = -1;

    for (int i = 0; i < 3; i++)
        AssertMsgReturn(aStdFds[i] < 0 || aStdFds[i] > i,
                        ("%i := %i not possible because we're lazy\n", i, aStdFds[i]),
                        VERR_NOT_SUPPORTED);

    /*
     * Resolve the user id if specified.
     */
    uid_t uid = ~(uid_t)0;
    gid_t gid = ~(gid_t)0;
    if (pszAsUser)
    {
        rc = rtCheckCredentials(pszAsUser, pszPassword, &gid, &uid);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Create the child environment if either RTPROC_FLAGS_PROFILE or
     * RTPROC_FLAGS_ENV_CHANGE_RECORD are in effect.
     */
    RTENV hEnvToUse = hEnv;
    if (   (fFlags & (RTPROC_FLAGS_ENV_CHANGE_RECORD | RTPROC_FLAGS_PROFILE))
        && (   (fFlags & RTPROC_FLAGS_ENV_CHANGE_RECORD)
            || hEnv == RTENV_DEFAULT) )
    {
        if (fFlags & RTPROC_FLAGS_PROFILE)
            rc = rtProcPosixCreateProfileEnv(&hEnvToUse, pszAsUser);
        else
            rc = RTEnvClone(&hEnvToUse, RTENV_DEFAULT);
        if (RT_SUCCESS(rc))
        {
            if ((fFlags & RTPROC_FLAGS_ENV_CHANGE_RECORD) && hEnv != RTENV_DEFAULT)
                rc = RTEnvApplyChanges(hEnvToUse, hEnv);
            if (RT_FAILURE(rc))
                RTEnvDestroy(hEnvToUse);
        }
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Check for execute access to the file.
     */
    char szRealExec[RTPATH_MAX];
    if (access(pszExec, X_OK))
    {
        rc = errno;
        if (   !(fFlags & RTPROC_FLAGS_SEARCH_PATH)
            || rc != ENOENT
            || RTPathHavePath(pszExec) )
            rc = RTErrConvertFromErrno(rc);
        else
        {
            /* search */
            char *pszPath = RTEnvDupEx(hEnvToUse, "PATH");
            rc = RTPathTraverseList(pszPath, ':', rtPathFindExec, (void *)pszExec, &szRealExec[0]);
            RTStrFree(pszPath);
            if (RT_SUCCESS(rc))
                pszExec = szRealExec;
            else
                rc = rc == VERR_END_OF_STRING ? VERR_FILE_NOT_FOUND : rc;
        }

        if (RT_FAILURE(rc))
            return rtProcPosixCreateReturn(rc, hEnvToUse, hEnv);
    }

    pid_t pid = -1;
    const char * const *papszEnv = RTEnvGetExecEnvP(hEnvToUse);
    AssertPtrReturn(papszEnv, rtProcPosixCreateReturn(VERR_INVALID_HANDLE, hEnvToUse, hEnv));


    /*
     * Take care of detaching the process.
     *
     * HACK ALERT! Put the process into a new process group with pgid = pid
     * to make sure it differs from that of the parent process to ensure that
     * the IPRT waitpid call doesn't race anyone (read XPCOM) doing group wide
     * waits. setsid() includes the setpgid() functionality.
     * 2010-10-11 XPCOM no longer waits for anything, but it cannot hurt.
     */
#ifndef RT_OS_OS2
    if (fFlags & RTPROC_FLAGS_DETACHED)
    {
# ifdef RT_OS_SOLARIS
        int templateFd = -1;
        if (!(fFlags & RTPROC_FLAGS_SAME_CONTRACT))
        {
            templateFd = rtSolarisContractPreFork();
            if (templateFd == -1)
                return rtProcPosixCreateReturn(VERR_OPEN_FAILED, hEnvToUse, hEnv);
        }
# endif /* RT_OS_SOLARIS */
        pid = fork();
        if (!pid)
        {
# ifdef RT_OS_SOLARIS
            if (!(fFlags & RTPROC_FLAGS_SAME_CONTRACT))
                rtSolarisContractPostForkChild(templateFd);
# endif
            setsid(); /* see comment above */

            pid = -1;
            /* Child falls through to the actual spawn code below. */
        }
        else
        {
# ifdef RT_OS_SOLARIS
            if (!(fFlags & RTPROC_FLAGS_SAME_CONTRACT))
                rtSolarisContractPostForkParent(templateFd, pid);
# endif
            if (pid > 0)
            {
                /* Must wait for the temporary process to avoid a zombie. */
                int status = 0;
                pid_t pidChild = 0;

                /* Restart if we get interrupted. */
                do
                {
                    pidChild = waitpid(pid, &status, 0);
                } while (   pidChild == -1
                         && errno == EINTR);

                /* Assume that something wasn't found. No detailed info. */
                if (status)
                    return rtProcPosixCreateReturn(VERR_PROCESS_NOT_FOUND, hEnvToUse, hEnv);
                if (phProcess)
                    *phProcess = 0;
                return rtProcPosixCreateReturn(VINF_SUCCESS, hEnvToUse, hEnv);
            }
            return rtProcPosixCreateReturn(RTErrConvertFromErrno(errno), hEnvToUse, hEnv);
        }
    }
#endif

    /*
     * Spawn the child.
     *
     * Any spawn code MUST not execute any atexit functions if it is for a
     * detached process. It would lead to running the atexit functions which
     * make only sense for the parent. libORBit e.g. gets confused by multiple
     * execution. Remember, there was only a fork() so far, and until exec()
     * is successfully run there is nothing which would prevent doing anything
     * silly with the (duplicated) file descriptors.
     */
#ifdef HAVE_POSIX_SPAWN
    /** @todo OS/2: implement DETACHED (BACKGROUND stuff), see VbglR3Daemonize.  */
    if (   uid == ~(uid_t)0
        && gid == ~(gid_t)0)
    {
        /* Spawn attributes. */
        posix_spawnattr_t Attr;
        rc = posix_spawnattr_init(&Attr);
        if (!rc)
        {
            /* Indicate that process group and signal mask are to be changed,
               and that the child should use default signal actions. */
            rc = posix_spawnattr_setflags(&Attr, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);
            Assert(rc == 0);

            /* The child starts in its own process group. */
            if (!rc)
            {
                rc = posix_spawnattr_setpgroup(&Attr, 0 /* pg == child pid */);
                Assert(rc == 0);
            }

            /* Unmask all signals. */
            if (!rc)
            {
                sigset_t SigMask;
                sigemptyset(&SigMask);
                rc = posix_spawnattr_setsigmask(&Attr, &SigMask); Assert(rc == 0);
            }

            /* File changes. */
            posix_spawn_file_actions_t  FileActions;
            posix_spawn_file_actions_t *pFileActions = NULL;
            if ((aStdFds[0] != -1 || aStdFds[1] != -1 || aStdFds[2] != -1) && !rc)
            {
                rc = posix_spawn_file_actions_init(&FileActions);
                if (!rc)
                {
                    pFileActions = &FileActions;
                    for (int i = 0; i < 3; i++)
                    {
                        int fd = aStdFds[i];
                        if (fd == -2)
                            rc = posix_spawn_file_actions_addclose(&FileActions, i);
                        else if (fd >= 0 && fd != i)
                        {
                            rc = posix_spawn_file_actions_adddup2(&FileActions, fd, i);
                            if (!rc)
                            {
                                for (int j = i + 1; j < 3; j++)
                                    if (aStdFds[j] == fd)
                                    {
                                        fd = -1;
                                        break;
                                    }
                                if (fd >= 0)
                                    rc = posix_spawn_file_actions_addclose(&FileActions, fd);
                            }
                        }
                        if (rc)
                            break;
                    }
                }
            }

            if (!rc)
                rc = posix_spawn(&pid, pszExec, pFileActions, &Attr, (char * const *)papszArgs,
                                 (char * const *)papszEnv);

            /* cleanup */
            int rc2 = posix_spawnattr_destroy(&Attr); Assert(rc2 == 0); NOREF(rc2);
            if (pFileActions)
            {
                rc2 = posix_spawn_file_actions_destroy(pFileActions);
                Assert(rc2 == 0);
            }

            /* return on success.*/
            if (!rc)
            {
                /* For a detached process this happens in the temp process, so
                 * it's not worth doing anything as this process must exit. */
                if (fFlags & RTPROC_FLAGS_DETACHED)
                    _Exit(0);
                if (phProcess)
                    *phProcess = pid;
                return rtProcPosixCreateReturn(VINF_SUCCESS, hEnvToUse, hEnv);
            }
        }
        /* For a detached process this happens in the temp process, so
         * it's not worth doing anything as this process must exit. */
        if (fFlags & RTPROC_FLAGS_DETACHED)
            _Exit(124);
    }
    else
#endif
    {
#ifdef RT_OS_SOLARIS
        int templateFd = -1;
        if (!(fFlags & RTPROC_FLAGS_SAME_CONTRACT))
        {
            templateFd = rtSolarisContractPreFork();
            if (templateFd == -1)
                return rtProcPosixCreateReturn(VERR_OPEN_FAILED, hEnvToUse, hEnv);
        }
#endif /* RT_OS_SOLARIS */
        pid = fork();
        if (!pid)
        {
#ifdef RT_OS_SOLARIS
            if (!(fFlags & RTPROC_FLAGS_SAME_CONTRACT))
                rtSolarisContractPostForkChild(templateFd);
#endif /* RT_OS_SOLARIS */
            if (!(fFlags & RTPROC_FLAGS_DETACHED))
                setpgid(0, 0); /* see comment above */

            /*
             * Change group and user if requested.
             */
#if 1 /** @todo This needs more work, see suplib/hardening. */
            if (pszAsUser)
            {
                int ret = initgroups(pszAsUser, gid);
                if (ret)
                {
                    if (fFlags & RTPROC_FLAGS_DETACHED)
                        _Exit(126);
                    else
                        exit(126);
                }
            }
            if (gid != ~(gid_t)0)
            {
                if (setgid(gid))
                {
                    if (fFlags & RTPROC_FLAGS_DETACHED)
                        _Exit(126);
                    else
                        exit(126);
                }
            }

            if (uid != ~(uid_t)0)
            {
                if (setuid(uid))
                {
                    if (fFlags & RTPROC_FLAGS_DETACHED)
                        _Exit(126);
                    else
                        exit(126);
                }
            }
#endif

            /*
             * Some final profile environment tweaks, if running as user.
             */
            if (   (fFlags & RTPROC_FLAGS_PROFILE)
                && pszAsUser
                && (   (fFlags & RTPROC_FLAGS_ENV_CHANGE_RECORD)
                    || hEnv == RTENV_DEFAULT) )
            {
                rc = rtProcPosixAdjustProfileEnvFromChild(hEnvToUse, fFlags, hEnv);
                papszEnv = RTEnvGetExecEnvP(hEnvToUse);
                if (RT_FAILURE(rc) || !papszEnv)
                {
                    if (fFlags & RTPROC_FLAGS_DETACHED)
                        _Exit(126);
                    else
                        exit(126);
                }
            }

            /*
             * Unset the signal mask.
             */
            sigset_t SigMask;
            sigemptyset(&SigMask);
            rc = sigprocmask(SIG_SETMASK, &SigMask, NULL);
            Assert(rc == 0);

            /*
             * Apply changes to the standard file descriptor and stuff.
             */
            for (int i = 0; i < 3; i++)
            {
                int fd = aStdFds[i];
                if (fd == -2)
                    close(i);
                else if (fd >= 0)
                {
                    int rc2 = dup2(fd, i);
                    if (rc2 != i)
                    {
                        if (fFlags & RTPROC_FLAGS_DETACHED)
                            _Exit(125);
                        else
                            exit(125);
                    }
                    for (int j = i + 1; j < 3; j++)
                        if (aStdFds[j] == fd)
                        {
                            fd = -1;
                            break;
                        }
                    if (fd >= 0)
                        close(fd);
                }
            }

            /*
             * Finally, execute the requested program.
             */
            rc = execve(pszExec, (char * const *)papszArgs, (char * const *)papszEnv);
            if (errno == ENOEXEC)
            {
                /* This can happen when trying to start a shell script without the magic #!/bin/sh */
                RTAssertMsg2Weak("Cannot execute this binary format!\n");
            }
            else
                RTAssertMsg2Weak("execve returns %d errno=%d\n", rc, errno);
            RTAssertReleasePanic();
            if (fFlags & RTPROC_FLAGS_DETACHED)
                _Exit(127);
            else
                exit(127);
        }
#ifdef RT_OS_SOLARIS
        if (!(fFlags & RTPROC_FLAGS_SAME_CONTRACT))
            rtSolarisContractPostForkParent(templateFd, pid);
#endif /* RT_OS_SOLARIS */
        if (pid > 0)
        {
            /* For a detached process this happens in the temp process, so
             * it's not worth doing anything as this process must exit. */
            if (fFlags & RTPROC_FLAGS_DETACHED)
                _Exit(0);
            if (phProcess)
                *phProcess = pid;
            return rtProcPosixCreateReturn(VINF_SUCCESS, hEnvToUse, hEnv);
        }
        /* For a detached process this happens in the temp process, so
         * it's not worth doing anything as this process must exit. */
        if (fFlags & RTPROC_FLAGS_DETACHED)
            _Exit(124);
        return rtProcPosixCreateReturn(RTErrConvertFromErrno(errno), hEnvToUse, hEnv);
    }

    return rtProcPosixCreateReturn(VERR_NOT_IMPLEMENTED, hEnvToUse, hEnv);
}


RTR3DECL(int)   RTProcDaemonizeUsingFork(bool fNoChDir, bool fNoClose, const char *pszPidfile)
{
    /*
     * Fork the child process in a new session and quit the parent.
     *
     * - fork once and create a new session (setsid). This will detach us
     *   from the controlling tty meaning that we won't receive the SIGHUP
     *   (or any other signal) sent to that session.
     * - The SIGHUP signal is ignored because the session/parent may throw
     *   us one before we get to the setsid.
     * - When the parent exit(0) we will become an orphan and re-parented to
     *   the init process.
     * - Because of the sometimes unexpected semantics of assigning the
     *   controlling tty automagically when a session leader first opens a tty,
     *   we will fork() once more to get rid of the session leadership role.
     */

    /* We start off by opening the pidfile, so that we can fail straight away
     * if it already exists. */
    int fdPidfile = -1;
    if (pszPidfile != NULL)
    {
        /* @note the exclusive create is not guaranteed on all file
         * systems (e.g. NFSv2) */
        if ((fdPidfile = open(pszPidfile, O_RDWR | O_CREAT | O_EXCL, 0644)) == -1)
            return RTErrConvertFromErrno(errno);
    }

    /* Ignore SIGHUP straight away. */
    struct sigaction OldSigAct;
    struct sigaction SigAct;
    memset(&SigAct, 0, sizeof(SigAct));
    SigAct.sa_handler = SIG_IGN;
    int rcSigAct = sigaction(SIGHUP, &SigAct, &OldSigAct);

    /* First fork, to become independent process. */
    pid_t pid = fork();
    if (pid == -1)
    {
        if (fdPidfile != -1)
            close(fdPidfile);
        return RTErrConvertFromErrno(errno);
    }
    if (pid != 0)
    {
        /* Parent exits, no longer necessary. The child gets reparented
         * to the init process. */
        exit(0);
    }

    /* Create new session, fix up the standard file descriptors and the
     * current working directory. */
    /** @todo r=klaus the webservice uses this function and assumes that the
     * contract id of the daemon is the same as that of the original process.
     * Whenever this code is changed this must still remain possible. */
    pid_t newpgid = setsid();
    int SavedErrno = errno;
    if (rcSigAct != -1)
        sigaction(SIGHUP, &OldSigAct, NULL);
    if (newpgid == -1)
    {
        if (fdPidfile != -1)
            close(fdPidfile);
        return RTErrConvertFromErrno(SavedErrno);
    }

    if (!fNoClose)
    {
        /* Open stdin(0), stdout(1) and stderr(2) as /dev/null. */
        int fd = open("/dev/null", O_RDWR);
        if (fd == -1) /* paranoia */
        {
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            fd = open("/dev/null", O_RDWR);
        }
        if (fd != -1)
        {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > 2)
                close(fd);
        }
    }

    if (!fNoChDir)
    {
        int rcIgnored = chdir("/");
        NOREF(rcIgnored);
    }

    /* Second fork to lose session leader status. */
    pid = fork();
    if (pid == -1)
    {
        if (fdPidfile != -1)
            close(fdPidfile);
        return RTErrConvertFromErrno(errno);
    }

    if (pid != 0)
    {
        /* Write the pid file, this is done in the parent, before exiting. */
        if (fdPidfile != -1)
        {
            char szBuf[256];
            size_t cbPid = RTStrPrintf(szBuf, sizeof(szBuf), "%d\n", pid);
            ssize_t cbIgnored = write(fdPidfile, szBuf, cbPid); NOREF(cbIgnored);
            close(fdPidfile);
        }
        exit(0);
    }

    if (fdPidfile != -1)
        close(fdPidfile);

    return VINF_SUCCESS;
}


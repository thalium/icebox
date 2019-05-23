/* $Id: path.cpp $ */
/** @file
 * IPRT - Path Manipulation.
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
#include "internal/iprt.h"
#include <iprt/assert.h>
#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include "internal/path.h"
#include "internal/process.h"


#ifdef RT_OS_SOLARIS
/**
 * Hack to strip of the architecture subdirectory from the exec dir.
 *
 * @returns See RTPathExecDir.
 * @param   pszPath             See RTPathExecDir.
 * @param   cchPath             See RTPathExecDir.
 */
DECLINLINE(int) rtPathSolarisArchHack(char *pszPath, size_t cchPath)
{
    int rc = RTPathExecDir(pszPath, cchPath);
    if (RT_SUCCESS(rc))
    {
        const char *pszLast = RTPathFilename(pszPath);
        if (   !strcmp(pszLast, "amd64")
            || !strcmp(pszLast, "i386"))
            RTPathStripFilename(pszPath);
    }
    return rc;
}
#endif


RTDECL(int) RTPathExecDir(char *pszPath, size_t cchPath)
{
    AssertReturn(g_szrtProcExePath[0], VERR_WRONG_ORDER);

    /*
     * Calc the length and check if there is space before copying.
     */
    size_t cch = g_cchrtProcDir;
    if (cch < cchPath)
    {
        memcpy(pszPath, g_szrtProcExePath, cch);
        pszPath[cch] = '\0';
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("Buffer too small (%zu <= %zu)\n", cchPath, cch));
    return VERR_BUFFER_OVERFLOW;
}


RTDECL(int) RTPathAppPrivateNoArch(char *pszPath, size_t cchPath)
{
#if !defined(RT_OS_WINDOWS) && defined(RTPATH_APP_PRIVATE)
    return RTStrCopy(pszPath, cchPath, RTPATH_APP_PRIVATE);
#elif defined(RT_OS_SOLARIS)
    return rtPathSolarisArchHack(pszPath, cchPath);
#else
    return RTPathExecDir(pszPath, cchPath);
#endif
}


RTDECL(int) RTPathAppPrivateArch(char *pszPath, size_t cchPath)
{
#if !defined(RT_OS_WINDOWS) && defined(RTPATH_APP_PRIVATE_ARCH)
    return RTStrCopy(pszPath, cchPath, RTPATH_APP_PRIVATE_ARCH);
#else
    return RTPathExecDir(pszPath, cchPath);
#endif
}


RTDECL(int) RTPathAppPrivateArchTop(char *pszPath, size_t cchPath)
{
#if !defined(RT_OS_WINDOWS) && defined(RTPATH_APP_PRIVATE_ARCH_TOP)
    return RTStrCopy(pszPath, cchPath, RTPATH_APP_PRIVATE_ARCH_TOP);
#elif !defined(RT_OS_WINDOWS) && defined(RTPATH_APP_PRIVATE_ARCH)
    return RTStrCopy(pszPath, cchPath, RTPATH_APP_PRIVATE_ARCH);
#elif defined(RT_OS_SOLARIS)
    return rtPathSolarisArchHack(pszPath, cchPath);
#else
    int rc = RTPathExecDir(pszPath, cchPath);
    return rc;
#endif
}


RTDECL(int) RTPathSharedLibs(char *pszPath, size_t cchPath)
{
#if !defined(RT_OS_WINDOWS) && defined(RTPATH_SHARED_LIBS)
    return RTStrCopy(pszPath, cchPath, RTPATH_SHARED_LIBS);
#else
    return RTPathExecDir(pszPath, cchPath);
#endif
}


RTDECL(int) RTPathAppDocs(char *pszPath, size_t cchPath)
{
#if !defined(RT_OS_WINDOWS) && defined(RTPATH_APP_DOCS)
    return RTStrCopy(pszPath, cchPath, RTPATH_APP_DOCS);
#elif defined(RT_OS_SOLARIS)
    return rtPathSolarisArchHack(pszPath, cchPath);
#else
    return RTPathExecDir(pszPath, cchPath);
#endif
}


RTDECL(int) RTPathTemp(char *pszPath, size_t cchPath)
{
    /*
     * Try get it from the environment first.
     */
    static const char * const s_apszVars[] =
    {
        "IPRT_TMPDIR"
#if defined(RT_OS_WINDOWS)
        , "TMP", "TEMP", "USERPROFILE"
#elif defined(RT_OS_OS2)
        , "TMP", "TEMP", "TMPDIR"
#else
        , "TMPDIR"
#endif
    };
    for (size_t iVar = 0; iVar < RT_ELEMENTS(s_apszVars); iVar++)
    {
        int rc = RTEnvGetEx(RTENV_DEFAULT, s_apszVars[iVar], pszPath, cchPath, NULL);
        if (rc != VERR_ENV_VAR_NOT_FOUND)
            return rc;
    }

    /*
     * Here we should use some sane system default, instead we just use
     * the typical unix temp dir for now.
     */
    /** @todo Windows should default to the windows directory, see GetTempPath.
     * Some unixes has path.h and _PATH_TMP. There is also a question about
     * whether /var/tmp wouldn't be a better place...  */
    if (cchPath < sizeof("/tmp") )
        return VERR_BUFFER_OVERFLOW;
    memcpy(pszPath, "/tmp", sizeof("/tmp"));
    return VINF_SUCCESS;
}


RTR3DECL(int) RTPathGetMode(const char *pszPath, PRTFMODE pfMode)
{
    AssertPtrReturn(pfMode, VERR_INVALID_POINTER);

    RTFSOBJINFO ObjInfo;
    int rc = RTPathQueryInfoEx(pszPath, &ObjInfo, RTFSOBJATTRADD_NOTHING, RTPATH_F_FOLLOW_LINK);
    if (RT_SUCCESS(rc))
        *pfMode = ObjInfo.Attr.fMode;

    return rc;
}

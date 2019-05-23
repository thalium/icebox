/* $Id: GetVBoxUserHomeDirectory.cpp $ */
/** @file
 * MS COM / XPCOM Abstraction Layer - GetVBoxUserHomeDirectory.
 */

/*
 * Copyright (C) 2005-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_MAIN
#include <VBox/com/com.h>

#include <iprt/env.h>
#include <iprt/dir.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>

#include <VBox/err.h>
#include <VBox/log.h>



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if !defined(RT_OS_DARWIN) && !defined(RT_OS_WINDOWS)
char g_szXdgConfigHome[RTPATH_MAX] = "";
#endif

/**
 * Possible locations for the VirtualBox user configuration folder,
 * listed from oldest (as in legacy) to newest.  These can be either
 * absolute or relative to the home directory.  We use the first entry
 * of the list which corresponds to a real folder on storage, or
 * create a folder corresponding to the last in the list (the least
 * legacy) if none do.
 */
const char * const g_apcszUserHome[] =
{
#ifdef RT_OS_DARWIN
    "Library/VirtualBox",
#elif defined RT_OS_WINDOWS
    ".VirtualBox",
#else
    ".VirtualBox", g_szXdgConfigHome,
#endif
};


namespace com
{

static int composeHomePath(char *aDir, size_t aDirLen, const char *pcszBase)
{
    int vrc;
    if (RTPathStartsWithRoot(pcszBase))
        vrc = RTStrCopy(aDir, aDirLen, pcszBase);
    else
    {
        /* compose the config directory (full path) */
        /** @todo r=bird: RTPathUserHome doesn't necessarily return a
         * full (abs) path like the comment above seems to indicate. */
        vrc = RTPathUserHome(aDir, aDirLen);
        if (RT_SUCCESS(vrc))
            vrc = RTPathAppend(aDir, aDirLen, pcszBase);
    }
    return vrc;
}

int GetVBoxUserHomeDirectory(char *aDir, size_t aDirLen, bool fCreateDir)
{
    AssertReturn(aDir, VERR_INVALID_POINTER);
    AssertReturn(aDirLen > 0, VERR_BUFFER_OVERFLOW);

    /* start with null */
    *aDir = 0;

    char szTmp[RTPATH_MAX];
    int vrc = RTEnvGetEx(RTENV_DEFAULT, "VBOX_USER_HOME", szTmp, sizeof(szTmp), NULL);
    if (RT_SUCCESS(vrc) || vrc == VERR_ENV_VAR_NOT_FOUND)
    {
        bool fFound = false;
        if (RT_SUCCESS(vrc))
        {
            /* get the full path name */
            vrc = RTPathAbs(szTmp, aDir, aDirLen);
        }
        else
        {
#if !defined(RT_OS_WINDOWS) && !defined(RT_OS_DARWIN)
            vrc = RTEnvGetEx(RTENV_DEFAULT, "XDG_CONFIG_HOME", g_szXdgConfigHome, sizeof(g_szXdgConfigHome), NULL);
            if (RT_SUCCESS(vrc))
                vrc = RTPathAppend(g_szXdgConfigHome, sizeof(g_szXdgConfigHome), "VirtualBox");
            AssertMsg(vrc == VINF_SUCCESS || vrc == VERR_ENV_VAR_NOT_FOUND, ("%Rrc\n", vrc));
            if (RT_FAILURE_NP(vrc))
                vrc = RTStrCopy(g_szXdgConfigHome, sizeof(g_szXdgConfigHome), ".config/VirtualBox");
#endif
            for (unsigned i = 0; i < RT_ELEMENTS(g_apcszUserHome); ++i)
            {
                vrc = composeHomePath(aDir, aDirLen, g_apcszUserHome[i]);
                if (   RT_SUCCESS(vrc)
                    && RTDirExists(aDir))
                {
                    fFound = true;
                    break;
                }
            }
        }

        /* ensure the home directory exists */
        if (RT_SUCCESS(vrc))
            if (!fFound && fCreateDir)
                vrc = RTDirCreateFullPath(aDir, 0700);
    }

    return vrc;
}

} /* namespace com */

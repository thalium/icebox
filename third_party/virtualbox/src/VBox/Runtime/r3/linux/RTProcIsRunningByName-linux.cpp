/* $Id: RTProcIsRunningByName-linux.cpp $ */
/** @file
 * IPRT - RTProcIsRunningByName, Linux implementation.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_PROCESS
#include <iprt/process.h>
#include <iprt/string.h>
#include <iprt/dir.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/param.h>
#include <iprt/assert.h>

#include <unistd.h>


RTR3DECL(bool) RTProcIsRunningByName(const char *pszName)
{
    /*
     * Quick validation.
     */
    if (!pszName)
        return false;

    bool const fWithPath = RTPathHavePath(pszName);

    /*
     * Enumerate /proc.
     */
    RTDIR hDir;
    int rc = RTDirOpen(&hDir, "/proc");
    AssertMsgRCReturn(rc, ("RTDirOpen on /proc failed: rc=%Rrc\n", rc), false);
    if (RT_SUCCESS(rc))
    {
        RTDIRENTRY DirEntry;
        while (RT_SUCCESS(RTDirRead(hDir, &DirEntry, NULL)))
        {
            /*
             * Filter numeric directory entries only.
             */
            if (   (   DirEntry.enmType == RTDIRENTRYTYPE_DIRECTORY
                    || DirEntry.enmType == RTDIRENTRYTYPE_UNKNOWN)
                && RTStrToUInt32(DirEntry.szName) > 0)
            {
                /*
                 * Try readlink on exe first since it's more faster and reliable.
                 * Fall back on reading the first line in cmdline if that fails
                 * (access errors typically). cmdline is unreliable as it might
                 * contain whatever the execv caller passes as argv[0].
                 */
                char szName[RTPATH_MAX];
                RTStrPrintf(szName, sizeof(szName), "/proc/%s/exe", &DirEntry.szName[0]);
                char szExe[RTPATH_MAX];
                int cchLink = readlink(szName, szExe, sizeof(szExe) - 1);
                if (    cchLink > 0
                    &&  (size_t)cchLink < sizeof(szExe))
                {
                    szExe[cchLink] = '\0';
                    rc = VINF_SUCCESS;
                }
                else
                {
                    RTStrPrintf(szName, sizeof(szName), "/proc/%s/cmdline", &DirEntry.szName[0]);
                    PRTSTREAM pStream;
                    rc = RTStrmOpen(szName, "r", &pStream);
                    if (RT_SUCCESS(rc))
                    {
                        rc = RTStrmGetLine(pStream, szExe, sizeof(szExe));
                        RTStrmClose(pStream);
                    }
                }
                if (RT_SUCCESS(rc))
                {
                    /*
                     * We are interested on the file name part only.
                     */
                    char const *pszProcName = fWithPath ? szExe : RTPathFilename(szExe);
                    if (RTStrCmp(pszProcName, pszName) == 0)
                    {
                        /* Found it! */
                        RTDirClose(hDir);
                        return true;
                    }
                }
            }
        }
        RTDirClose(hDir);
    }

    return false;
}


/* $Id: AutostartDb-generic.cpp $ */
/** @file
 * VirtualBox Main - Autostart implementation.
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
 */

#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/process.h>
#include <iprt/path.h>
#include <iprt/mem.h>
#include <iprt/file.h>
#include <iprt/string.h>

#include "AutostartDb.h"

#if defined(RT_OS_LINUX)
/**
 * Modifies the autostart database.
 *
 * @returns VBox status code.
 * @param   fAutostart    Flag whether the autostart or autostop database is modified.
 * @param   fAddVM        Flag whether a VM is added or removed from the database.
 */
int AutostartDb::autostartModifyDb(bool fAutostart, bool fAddVM)
{
    int rc = VINF_SUCCESS;
    char *pszUser = NULL;

    /* Check if the path is set. */
    if (!m_pszAutostartDbPath)
        return VERR_PATH_NOT_FOUND;

    rc = RTProcQueryUsernameA(RTProcSelf(), &pszUser);
    if (RT_SUCCESS(rc))
    {
        char *pszFile;
        uint64_t fOpen = RTFILE_O_DENY_ALL | RTFILE_O_READWRITE;
        RTFILE hAutostartFile;

        AssertPtr(pszUser);

        if (fAddVM)
            fOpen |= RTFILE_O_OPEN_CREATE;
        else
            fOpen |= RTFILE_O_OPEN;

        rc = RTStrAPrintf(&pszFile, "%s/%s.%s",
                          m_pszAutostartDbPath, pszUser, fAutostart ? "start" : "stop");
        if (RT_SUCCESS(rc))
        {
            rc = RTFileOpen(&hAutostartFile, pszFile, fOpen);
            if (RT_SUCCESS(rc))
            {
                uint64_t cbFile;

                /*
                 * Files with more than 16 bytes are rejected because they just contain
                 * a number of the amount of VMs with autostart configured, so they
                 * should be really really small. Anything else is bogus.
                 */
                rc = RTFileGetSize(hAutostartFile, &cbFile);
                if (   RT_SUCCESS(rc)
                    && cbFile <= 16)
                {
                    char abBuf[16 + 1]; /* trailing \0 */
                    uint32_t cAutostartVms = 0;

                    RT_ZERO(abBuf);

                    /* Check if the file was just created. */
                    if (cbFile)
                    {
                        rc = RTFileRead(hAutostartFile, abBuf, (size_t)cbFile, NULL);
                        if (RT_SUCCESS(rc))
                        {
                            rc = RTStrToUInt32Ex(abBuf, NULL, 10 /* uBase */, &cAutostartVms);
                            if (   rc == VWRN_TRAILING_CHARS
                                || rc == VWRN_TRAILING_SPACES)
                                rc = VINF_SUCCESS;
                        }
                    }

                    if (RT_SUCCESS(rc))
                    {
                        size_t cbBuf;

                        /* Modify VM counter and write back. */
                        if (fAddVM)
                            cAutostartVms++;
                        else
                            cAutostartVms--;

                        if (cAutostartVms > 0)
                        {
                            cbBuf = RTStrPrintf(abBuf, sizeof(abBuf), "%u", cAutostartVms);
                            rc = RTFileSetSize(hAutostartFile, cbBuf);
                            if (RT_SUCCESS(rc))
                                rc = RTFileWriteAt(hAutostartFile, 0, abBuf, cbBuf, NULL);
                        }
                        else
                        {
                            /* Just delete the file if there are no VMs left. */
                            RTFileClose(hAutostartFile);
                            RTFileDelete(pszFile);
                            hAutostartFile = NIL_RTFILE;
                        }
                    }
                }
                else if (RT_SUCCESS(rc))
                    rc = VERR_FILE_TOO_BIG;

                if (hAutostartFile != NIL_RTFILE)
                    RTFileClose(hAutostartFile);
            }
            RTStrFree(pszFile);
        }

        RTStrFree(pszUser);
    }

    return rc;
}

#endif

AutostartDb::AutostartDb()
{
#ifdef RT_OS_LINUX
    int rc = RTCritSectInit(&this->CritSect);
    NOREF(rc);
    m_pszAutostartDbPath = NULL;
#endif
}

AutostartDb::~AutostartDb()
{
#ifdef RT_OS_LINUX
    RTCritSectDelete(&this->CritSect);
    if (m_pszAutostartDbPath)
        RTStrFree(m_pszAutostartDbPath);
#endif
}

int AutostartDb::setAutostartDbPath(const char *pszAutostartDbPathNew)
{
#if defined(RT_OS_LINUX)
    char *pszAutostartDbPathTmp = NULL;

    if (pszAutostartDbPathNew)
    {
        pszAutostartDbPathTmp = RTStrDup(pszAutostartDbPathNew);
        if (!pszAutostartDbPathTmp)
            return VERR_NO_MEMORY;
    }

    RTCritSectEnter(&this->CritSect);
    if (m_pszAutostartDbPath)
        RTStrFree(m_pszAutostartDbPath);

    m_pszAutostartDbPath = pszAutostartDbPathTmp;
    RTCritSectLeave(&this->CritSect);
    return VINF_SUCCESS;
#else
    NOREF(pszAutostartDbPathNew);
    return VERR_NOT_SUPPORTED;
#endif
}

int AutostartDb::addAutostartVM(const char *pszVMId)
{
    int rc = VERR_NOT_SUPPORTED;

#if defined(RT_OS_LINUX)
    NOREF(pszVMId); /* Not needed */

    RTCritSectEnter(&this->CritSect);
    rc = autostartModifyDb(true /* fAutostart */, true /* fAddVM */);
    RTCritSectLeave(&this->CritSect);
#elif defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS)
    NOREF(pszVMId); /* Not needed */
    rc = VINF_SUCCESS;
#else
    NOREF(pszVMId);
    rc = VERR_NOT_SUPPORTED;
#endif

    return rc;
}

int AutostartDb::removeAutostartVM(const char *pszVMId)
{
    int rc = VINF_SUCCESS;

#if defined(RT_OS_LINUX)
    NOREF(pszVMId); /* Not needed */
    RTCritSectEnter(&this->CritSect);
    rc = autostartModifyDb(true /* fAutostart */, false /* fAddVM */);
    RTCritSectLeave(&this->CritSect);
#elif defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS)
    NOREF(pszVMId); /* Not needed */
    rc = VINF_SUCCESS;
#else
    NOREF(pszVMId);
    rc = VERR_NOT_SUPPORTED;
#endif

    return rc;
}

int AutostartDb::addAutostopVM(const char *pszVMId)
{
    int rc = VINF_SUCCESS;

#if defined(RT_OS_LINUX)
    NOREF(pszVMId); /* Not needed */
    RTCritSectEnter(&this->CritSect);
    rc = autostartModifyDb(false /* fAutostart */, true /* fAddVM */);
    RTCritSectLeave(&this->CritSect);
#elif defined(RT_OS_DARWIN)
    NOREF(pszVMId); /* Not needed */
    rc = VINF_SUCCESS;
#else
    NOREF(pszVMId);
    rc = VERR_NOT_SUPPORTED;
#endif

    return rc;
}

int AutostartDb::removeAutostopVM(const char *pszVMId)
{
    int rc = VINF_SUCCESS;

#if defined(RT_OS_LINUX)
    NOREF(pszVMId); /* Not needed */
    RTCritSectEnter(&this->CritSect);
    rc = autostartModifyDb(false /* fAutostart */, false /* fAddVM */);
    RTCritSectLeave(&this->CritSect);
#elif defined(RT_OS_DARWIN)
    NOREF(pszVMId); /* Not needed */
    rc = VINF_SUCCESS;
#else
    NOREF(pszVMId);
    rc = VERR_NOT_SUPPORTED;
#endif

    return rc;
}


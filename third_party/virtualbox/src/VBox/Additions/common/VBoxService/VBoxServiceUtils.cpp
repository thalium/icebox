/* $Id: VBoxServiceUtils.cpp $ */
/** @file
 * VBoxServiceUtils - Some utility functions.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
# include <iprt/param.h>
# include <iprt/path.h>
#endif
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/string.h>

#include <VBox/VBoxGuestLib.h>
#include "VBoxServiceInternal.h"


#ifdef VBOX_WITH_GUEST_PROPS

/**
 * Reads a guest property.
 *
 * @returns VBox status code, fully bitched.
 *
 * @param   u32ClientId         The HGCM client ID for the guest property session.
 * @param   pszPropName         The property name.
 * @param   ppszValue           Where to return the value.  This is always set
 *                              to NULL.  Free it using RTStrFree().  Optional.
 * @param   ppszFlags           Where to return the value flags. Free it
 *                              using RTStrFree().  Optional.
 * @param   puTimestamp         Where to return the timestamp.  This is only set
 *                              on success.  Optional.
 */
int VGSvcReadProp(uint32_t u32ClientId, const char *pszPropName, char **ppszValue, char **ppszFlags, uint64_t *puTimestamp)
{
    AssertPtrReturn(pszPropName, VERR_INVALID_POINTER);

    uint32_t    cbBuf = _1K;
    void       *pvBuf = NULL;
    int         rc    = VINF_SUCCESS;  /* MSC can't figure out the loop */

    if (ppszValue)
        *ppszValue = NULL;

    for (unsigned cTries = 0; cTries < 10; cTries++)
    {
        /*
         * (Re-)Allocate the buffer and try read the property.
         */
        RTMemFree(pvBuf);
        pvBuf = RTMemAlloc(cbBuf);
        if (!pvBuf)
        {
            VGSvcError("Guest Property: Failed to allocate %zu bytes\n", cbBuf);
            rc = VERR_NO_MEMORY;
            break;
        }
        char    *pszValue;
        char    *pszFlags;
        uint64_t uTimestamp;
        rc = VbglR3GuestPropRead(u32ClientId, pszPropName, pvBuf, cbBuf, &pszValue, &uTimestamp, &pszFlags, NULL);
        if (RT_FAILURE(rc))
        {
            if (rc == VERR_BUFFER_OVERFLOW)
            {
                /* try again with a bigger buffer. */
                cbBuf *= 2;
                continue;
            }
            if (rc == VERR_NOT_FOUND)
                VGSvcVerbose(2, "Guest Property: %s not found\n", pszPropName);
            else
                VGSvcError("Guest Property: Failed to query '%s': %Rrc\n", pszPropName, rc);
            break;
        }

        VGSvcVerbose(2, "Guest Property: Read '%s' = '%s', timestamp %RU64n\n", pszPropName, pszValue, uTimestamp);
        if (ppszValue)
        {
            *ppszValue = RTStrDup(pszValue);
            if (!*ppszValue)
            {
                VGSvcError("Guest Property: RTStrDup failed for '%s'\n", pszValue);
                rc = VERR_NO_MEMORY;
                break;
            }
        }

        if (puTimestamp)
            *puTimestamp = uTimestamp;
        if (ppszFlags)
            *ppszFlags = RTStrDup(pszFlags);
        break; /* done */
    }

    if (pvBuf)
        RTMemFree(pvBuf);
    return rc;
}


/**
 * Reads a guest property as a 32-bit value.
 *
 * @returns VBox status code, fully bitched.
 *
 * @param   u32ClientId         The HGCM client ID for the guest property session.
 * @param   pszPropName         The property name.
 * @param   pu32                Where to store the 32-bit value.
 *
 */
int VGSvcReadPropUInt32(uint32_t u32ClientId, const char *pszPropName, uint32_t *pu32, uint32_t u32Min, uint32_t u32Max)
{
    char *pszValue;
    int rc = VGSvcReadProp(u32ClientId, pszPropName, &pszValue, NULL /* ppszFlags */, NULL /* puTimestamp */);
    if (RT_SUCCESS(rc))
    {
        char *pszNext;
        rc = RTStrToUInt32Ex(pszValue, &pszNext, 0, pu32);
        if (   RT_SUCCESS(rc)
            && (*pu32 < u32Min || *pu32 > u32Max))
            rc = VGSvcError("The guest property value %s = %RU32 is out of range [%RU32..%RU32].\n",
                            pszPropName, *pu32, u32Min, u32Max);
        RTStrFree(pszValue);
    }
    return rc;
}

/**
 * Checks if @a pszPropName exists.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS if it exists.
 * @retval  VERR_NOT_FOUND if not found.
 *
 * @param   u32ClientId         The HGCM client ID for the guest property session.
 * @param   pszPropName         The property name.
 */
int VGSvcCheckPropExist(uint32_t u32ClientId, const char *pszPropName)
{
    return VGSvcReadProp(u32ClientId, pszPropName, NULL /*ppszValue*/, NULL /* ppszFlags */, NULL /* puTimestamp */);
}


/**
 * Reads a guest property from the host side.
 *
 * @returns IPRT status code, fully bitched.
 * @param   u32ClientId         The HGCM client ID for the guest property session.
 * @param   pszPropName         The property name.
 * @param   fReadOnly           Whether or not this property needs to be read only
 *                              by the guest side. Otherwise VERR_ACCESS_DENIED will
 *                              be returned.
 * @param   ppszValue           Where to return the value.  This is always set
 *                              to NULL.  Free it using RTStrFree().
 * @param   ppszFlags           Where to return the value flags. Free it
 *                              using RTStrFree().  Optional.
 * @param   puTimestamp         Where to return the timestamp.  This is only set
 *                              on success.  Optional.
 */
int VGSvcReadHostProp(uint32_t u32ClientId, const char *pszPropName, bool fReadOnly,
                      char **ppszValue, char **ppszFlags, uint64_t *puTimestamp)
{
    AssertPtrReturn(ppszValue, VERR_INVALID_PARAMETER);

    char *pszValue = NULL;
    char *pszFlags = NULL;
    int rc = VGSvcReadProp(u32ClientId, pszPropName, &pszValue, &pszFlags, puTimestamp);
    if (RT_SUCCESS(rc))
    {
        /* Check security bits. */
        if (   fReadOnly /* Do we except a guest read-only property */
            && !RTStrStr(pszFlags, "RDONLYGUEST"))
        {
            /* If we want a property which is read-only on the guest
             * and it is *not* marked as such, deny access! */
            rc = VERR_ACCESS_DENIED;
        }

        if (RT_SUCCESS(rc))
        {
            *ppszValue = pszValue;

            if (ppszFlags)
                *ppszFlags = pszFlags;
            else if (pszFlags)
                RTStrFree(pszFlags);
        }
        else
        {
            if (pszValue)
                RTStrFree(pszValue);
            if (pszFlags)
                RTStrFree(pszFlags);
        }
    }

    return rc;
}


/**
 * Wrapper around VbglR3GuestPropWriteValue that does value formatting and
 * logging.
 *
 * @returns VBox status code. Errors will be logged.
 *
 * @param   u32ClientId     The HGCM client ID for the guest property session.
 * @param   pszName         The property name.
 * @param   pszValueFormat  The property format string.  If this is NULL then
 *                          the property will be deleted (if possible).
 * @param   ...             Format arguments.
 */
int VGSvcWritePropF(uint32_t u32ClientId, const char *pszName, const char *pszValueFormat, ...)
{
    AssertPtr(pszName);
    int rc;
    if (pszValueFormat != NULL)
    {
        va_list va;
        va_start(va, pszValueFormat);
        VGSvcVerbose(3, "Writing guest property '%s' = '%N'\n", pszName, pszValueFormat, &va);
        va_end(va);

        va_start(va, pszValueFormat);
        rc = VbglR3GuestPropWriteValueV(u32ClientId, pszName, pszValueFormat, va);
        va_end(va);

        if (RT_FAILURE(rc))
             VGSvcError("Error writing guest property '%s' (rc=%Rrc)\n", pszName, rc);
    }
    else
    {
        VGSvcVerbose(3, "Deleting guest property '%s'\n", pszName);
        rc = VbglR3GuestPropWriteValue(u32ClientId, pszName, NULL);
        if (RT_FAILURE(rc))
            VGSvcError("Error deleting guest property '%s' (rc=%Rrc)\n", pszName, rc);
    }
    return rc;
}

#endif /* VBOX_WITH_GUEST_PROPS */
#ifdef RT_OS_WINDOWS

/**
 * Helper for vgsvcUtilGetFileVersion and attempts to read and parse
 * FileVersion.
 *
 * @returns Success indicator.
 */
static bool vgsvcUtilGetFileVersionOwn(LPSTR pVerData, PDWORD pdwMajor, PDWORD pdwMinor, PDWORD pdwBuildNumber,
                                       PDWORD pdwRevisionNumber)
{
    UINT    cchStrValue = 0;
    LPTSTR  pStrValue   = NULL;
    if (!VerQueryValueA(pVerData, "\\StringFileInfo\\040904b0\\FileVersion", (LPVOID *)&pStrValue, &cchStrValue))
        return false;

    /** @todo r=bird: get rid of this. Avoid sscanf like the plague! */
    if (sscanf(pStrValue, "%ld.%ld.%ld.%ld", pdwMajor, pdwMinor, pdwBuildNumber, pdwRevisionNumber) != 4)
        return false;

    return true;
}


/**
 * Worker for VGSvcUtilWinGetFileVersionString.
 *
 * @returns VBox status code.
 * @param   pszFilename         ASCII & ANSI & UTF-8 compliant name.
 * @param   pdwMajor            Where to return the major version number.
 * @param   pdwMinor            Where to return the minor version number.
 * @param   pdwBuildNumber      Where to return the build number.
 * @param   pdwRevisionNumber   Where to return the revision number.
 */
static int vgsvcUtilGetFileVersion(const char *pszFilename, PDWORD pdwMajor, PDWORD pdwMinor, PDWORD pdwBuildNumber,
                                   PDWORD pdwRevisionNumber)
{
    int rc;

    *pdwMajor = *pdwMinor = *pdwBuildNumber = *pdwRevisionNumber = 0;

    /*
     * Get the file version info.
     */
    DWORD dwHandleIgnored;
    DWORD cbVerData = GetFileVersionInfoSizeA(pszFilename, &dwHandleIgnored);
    if (cbVerData)
    {
        LPTSTR pVerData = (LPTSTR)RTMemTmpAllocZ(cbVerData);
        if (pVerData)
        {
            if (GetFileVersionInfoA(pszFilename, dwHandleIgnored, cbVerData, pVerData))
            {
                /*
                 * Try query and parse the FileVersion string our selves first
                 * since this will give us the correct revision number when
                 * it goes beyond the range of an uint16_t / WORD.
                 */
                if (vgsvcUtilGetFileVersionOwn(pVerData, pdwMajor, pdwMinor, pdwBuildNumber, pdwRevisionNumber))
                    rc = VINF_SUCCESS;
                else
                {
                    /* Fall back on VS_FIXEDFILEINFO */
                    UINT                 cbFileInfoIgnored = 0;
                    VS_FIXEDFILEINFO    *pFileInfo = NULL;
                    if (VerQueryValue(pVerData, "\\", (LPVOID *)&pFileInfo, &cbFileInfoIgnored))
                    {
                        *pdwMajor          = HIWORD(pFileInfo->dwFileVersionMS);
                        *pdwMinor          = LOWORD(pFileInfo->dwFileVersionMS);
                        *pdwBuildNumber    = HIWORD(pFileInfo->dwFileVersionLS);
                        *pdwRevisionNumber = LOWORD(pFileInfo->dwFileVersionLS);
                        rc = VINF_SUCCESS;
                    }
                    else
                    {
                        rc = RTErrConvertFromWin32(GetLastError());
                        VGSvcVerbose(3, "No file version value for file '%s' available! (%d / rc=%Rrc)\n",
                                     pszFilename,  GetLastError(), rc);
                    }
                }
            }
            else
            {
                rc = RTErrConvertFromWin32(GetLastError());
                VGSvcVerbose(0, "GetFileVersionInfo(%s) -> %u / %Rrc\n", pszFilename, GetLastError(), rc);
            }

            RTMemTmpFree(pVerData);
        }
        else
        {
            VGSvcVerbose(0, "Failed to allocate %u byte for file version info for '%s'\n", cbVerData, pszFilename);
            rc = VERR_NO_TMP_MEMORY;
        }
    }
    else
    {
        rc = RTErrConvertFromWin32(GetLastError());
        VGSvcVerbose(3, "GetFileVersionInfoSize(%s) -> %u / %Rrc\n", pszFilename, GetLastError(), rc);
    }
    return rc;
}


/**
 * Gets a re-formatted version string from the VS_FIXEDFILEINFO table.
 *
 * @returns VBox status code.  The output buffer is always valid and the status
 *          code can safely be ignored.
 *
 * @param   pszPath         The base path.
 * @param   pszFilename     The filename.
 * @param   pszVersion      Where to return the version string.
 * @param   cbVersion       The size of the version string buffer. This MUST be
 *                          at least 2 bytes!
 */
int VGSvcUtilWinGetFileVersionString(const char *pszPath, const char *pszFilename, char *pszVersion, size_t cbVersion)
{
    /*
     * We will ALWAYS return with a valid output buffer.
     */
    AssertReturn(cbVersion >= 2, VERR_BUFFER_OVERFLOW);
    pszVersion[0] = '-';
    pszVersion[1] = '\0';

    /*
     * Create the path and query the bits.
     */
    char szFullPath[RTPATH_MAX];
    int rc = RTPathJoin(szFullPath, sizeof(szFullPath), pszPath, pszFilename);
    if (RT_SUCCESS(rc))
    {
        DWORD dwMajor, dwMinor, dwBuild, dwRev;
        rc = vgsvcUtilGetFileVersion(szFullPath, &dwMajor, &dwMinor, &dwBuild, &dwRev);
        if (RT_SUCCESS(rc))
            RTStrPrintf(pszVersion, cbVersion, "%u.%u.%ur%u", dwMajor, dwMinor, dwBuild, dwRev);
    }
    return rc;
}

#endif /* RT_OS_WINDOWS */


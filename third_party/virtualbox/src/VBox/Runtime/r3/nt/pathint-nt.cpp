/* $Id: pathint-nt.cpp $ */
/** @file
 * IPRT - Native NT, Internal Path stuff.
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
#define LOG_GROUP RTLOGGROUP_FS
#include "internal-r3-nt.h"

#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/err.h>
#include <iprt/assert.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static char const g_szPrefixUnc[] = "\\??\\UNC\\";
static char const g_szPrefix[]    = "\\??\\";


/**
 * Handles the pass thru case for UTF-8 input.
 * Win32 path uses "\\?\" prefix which is converted to "\??\" NT prefix.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.
 * @param   phRootDir           Where to return the root handle, if applicable.
 * @param   pszPath             The UTF-8 path.
 */
static int rtNtPathFromWinUtf8PassThru(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir, const char *pszPath)
{
    PRTUTF16 pwszPath = NULL;
    size_t   cwcLen;
    int rc = RTStrToUtf16Ex(pszPath, RTSTR_MAX, &pwszPath, 0, &cwcLen);
    if (RT_SUCCESS(rc))
    {
        if (cwcLen < _32K - 1)
        {
            pwszPath[0] = '\\';
            pwszPath[1] = '?';
            pwszPath[2] = '?';
            pwszPath[3] = '\\';

            pNtName->Buffer = pwszPath;
            pNtName->Length = (uint16_t)(cwcLen * sizeof(RTUTF16));
            pNtName->MaximumLength = pNtName->Length + sizeof(RTUTF16);
            *phRootDir = NULL;
            return VINF_SUCCESS;
        }

        RTUtf16Free(pwszPath);
        rc = VERR_FILENAME_TOO_LONG;
    }
    return rc;
}


/**
 * Handles the pass thru case for UTF-16 input.
 * Win32 path uses "\\?\" prefix which is converted to "\??\" NT prefix.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.
 * @param   phRootDir           Stores NULL here, as we don't use it.
 * @param   pwszWinPath         The UTF-16 windows-style path.
 * @param   cwcWinPath          The length of the windows-style input path.
 */
static int rtNtPathFromWinUtf16PassThru(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir,
                                        PCRTUTF16 pwszWinPath, size_t cwcWinPath)
{
    /* Check length and allocate memory for it. */
    int rc;
    if (cwcWinPath < _32K - 1)
    {
        PRTUTF16 pwszNtPath = (PRTUTF16)RTUtf16Alloc((cwcWinPath + 1) * sizeof(RTUTF16));
        if (pwszNtPath)
        {
            /* Intialize the path. */
            pwszNtPath[0] = '\\';
            pwszNtPath[1] = '?';
            pwszNtPath[2] = '?';
            pwszNtPath[3] = '\\';
            memcpy(pwszNtPath + 4, pwszWinPath + 4, (cwcWinPath - 4) * sizeof(RTUTF16));
            pwszNtPath[cwcWinPath] = '\0';

            /* Initialize the return values. */
            pNtName->Buffer = pwszNtPath;
            pNtName->Length = (uint16_t)(cwcWinPath * sizeof(RTUTF16));
            pNtName->MaximumLength = pNtName->Length + sizeof(RTUTF16);
            *phRootDir = NULL;

            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_NO_UTF16_MEMORY;
    }
    else
        rc = VERR_FILENAME_TOO_LONG;
    return rc;
}





/**
 * Converts the path to UTF-16 and sets all the return values.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.
 * @param   phRootDir           Where to return the root handle, if applicable.
 * @param   pszPath             The UTF-8 path.
 */
static int rtNtPathUtf8ToUniStr(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir, const char *pszPath)
{
    PRTUTF16 pwszPath = NULL;
    size_t   cwcLen;
    int rc = RTStrToUtf16Ex(pszPath, RTSTR_MAX, &pwszPath, 0, &cwcLen);
    if (RT_SUCCESS(rc))
    {
        if (cwcLen < _32K - 1)
        {
            pNtName->Buffer = pwszPath;
            pNtName->Length = (uint16_t)(cwcLen * sizeof(RTUTF16));
            pNtName->MaximumLength = pNtName->Length + sizeof(RTUTF16);
            *phRootDir = NULL;
            return VINF_SUCCESS;
        }

        RTUtf16Free(pwszPath);
        rc = VERR_FILENAME_TOO_LONG;
    }
    return rc;
}


/**
 * Converts a windows-style path to NT format and encoding.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.  Free using
 *                              rtTNtPathToNative.
 * @param   phRootDir           Where to return the root handle, if applicable.
 * @param   pszPath             The UTF-8 path.
 */
static int rtNtPathToNative(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir, const char *pszPath)
{
/** @todo This code sucks a bit performance wise, esp. calling
 *        generic RTPathAbs.  Too many buffers involved, I think. */

    /*
     * Very simple conversion of a win32-like path into an NT path.
     */
    const char *pszPrefix = g_szPrefix;
    size_t      cchPrefix = sizeof(g_szPrefix) - 1;
    size_t      cchSkip   = 0;

    if (   RTPATH_IS_SLASH(pszPath[0])
        && RTPATH_IS_SLASH(pszPath[1])
        && !RTPATH_IS_SLASH(pszPath[2])
        && pszPath[2])
    {
        if (   pszPath[2] == '?'
            && RTPATH_IS_SLASH(pszPath[3]))
            return rtNtPathFromWinUtf8PassThru(pNtName, phRootDir, pszPath);

#ifdef IPRT_WITH_NT_PATH_PASSTHRU
        /* Special hack: The path starts with "\\\\!\\", we will skip past the bang and pass it thru. */
        if (   pszPath[2] == '!'
            && RTPATH_IS_SLASH(pszPath[3]))
            return rtNtPathUtf8ToUniStr(pNtName, phRootDir, pszPath + 3);
#endif

        if (   pszPath[2] == '.'
            && RTPATH_IS_SLASH(pszPath[3]))
        {
            /*
             * Device path.
             * Note! I suspect \\.\stuff\..\otherstuff may be handled differently by windows.
             */
            cchSkip   = 4;
        }
        else
        {
            /* UNC */
            pszPrefix = g_szPrefixUnc;
            cchPrefix = sizeof(g_szPrefixUnc) - 1;
            cchSkip   = 2;
        }
    }

    /*
     * Straighten out all .. and uncessary . references and convert slashes.
     */
    char szPath[RTPATH_MAX];
    int rc = RTPathAbs(pszPath, &szPath[cchPrefix - cchSkip], sizeof(szPath) - (cchPrefix - cchSkip));
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Add prefix and convert it to UTF16.
     */
    memcpy(szPath, pszPrefix, cchPrefix);
    return rtNtPathUtf8ToUniStr(pNtName, phRootDir, szPath);
}


/**
 * Converts a windows-style path to NT format and encoding.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.  Free using
 *                              RTNtPathToNative.
 * @param   phRootDir           Where to return the root handle, if applicable.
 * @param   pszPath             The UTF-8 path.
 */
RTDECL(int) RTNtPathFromWinUtf8(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir, const char *pszPath)
{
    return rtNtPathToNative(pNtName, phRootDir, pszPath);
}


/**
 * Converts a UTF-16 windows-style path to NT format.
 *
 * @returns IPRT status code.
 * @param   pNtName             Where to return the NT name.  Free using
 *                              RTNtPathFree.
 * @param   phRootDir           Where to return the root handle, if applicable.
 * @param   pwszWinPath         The UTF-16 windows-style path.
 * @param   cwcWinPath          The max length of the windows-style path in
 *                              RTUTF16 units.  Use RTSTR_MAX if unknown and @a
 *                              pwszWinPath is correctly terminated.
 */
RTDECL(int) RTNtPathFromWinUtf16Ex(struct _UNICODE_STRING *pNtName, HANDLE *phRootDir, PCRTUTF16 pwszWinPath, size_t cwcWinPath)
{
    /*
     * Validate the input, calculating the correct length.
     */
    if (cwcWinPath == 0 || *pwszWinPath == '\0')
        return VERR_INVALID_NAME;

    RTUtf16NLenEx(pwszWinPath, cwcWinPath, &cwcWinPath);
    int rc = RTUtf16ValidateEncodingEx(pwszWinPath, cwcWinPath, 0);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Very simple conversion of a win32-like path into an NT path.
     */
    const char *pszPrefix = g_szPrefix;
    size_t      cchPrefix = sizeof(g_szPrefix) - 1;
    size_t      cchSkip   = 0;

    if (   RTPATH_IS_SLASH(pwszWinPath[0])
        && cwcWinPath >= 3
        && RTPATH_IS_SLASH(pwszWinPath[1])
        && !RTPATH_IS_SLASH(pwszWinPath[2]) )
    {
        if (   pwszWinPath[2] == '?'
            && cwcWinPath >= 4
            && RTPATH_IS_SLASH(pwszWinPath[3]))
            return rtNtPathFromWinUtf16PassThru(pNtName, phRootDir, pwszWinPath, cwcWinPath);

#ifdef IPRT_WITH_NT_PATH_PASSTHRU
        /* Special hack: The path starts with "\\\\!\\", we will skip past the bang and pass it thru. */
        if (   pwszWinPath[2] == '!'
            && cwcWinPath >= 4
            && RTPATH_IS_SLASH(pwszWinPath[3]))
        {
            pwszWinPath += 3;
            cwcWinPath  -= 3;
            if (cwcWinPath < _32K - 1)
            {
                PRTUTF16 pwszNtPath = RTUtf16Alloc((cwcWinPath + 1) * sizeof(RTUTF16));
                if (pwszNtPath)
                {
                    memcpy(pwszNtPath, pwszWinPath, cwcWinPath * sizeof(RTUTF16));
                    pwszNtPath[cwcWinPath] = '\0';
                    pNtName->Buffer = pwszNtPath;
                    pNtName->Length = (uint16_t)(cwcWinPath * sizeof(RTUTF16));
                    pNtName->MaximumLength = pNtName->Length + sizeof(RTUTF16);
                    *phRootDir = NULL;
                    return VINF_SUCCESS;
                }
                rc = VERR_NO_UTF16_MEMORY;
            }
            else
                rc = VERR_FILENAME_TOO_LONG;
            return rc;
        }
#endif

        if (   pwszWinPath[2] == '.'
            && cwcWinPath >= 4
            && RTPATH_IS_SLASH(pwszWinPath[3]))
        {
            /*
             * Device path.
             * Note! I suspect \\.\stuff\..\otherstuff may be handled differently by windows.
             */
            cchSkip   = 4;
        }
        else
        {
            /* UNC */
            pszPrefix = g_szPrefixUnc;
            cchPrefix = sizeof(g_szPrefixUnc) - 1;
            cchSkip   = 2;
        }
    }

    /*
     * Straighten out all .. and unnecessary . references and convert slashes.
     */
    char   szAbsPath[RTPATH_MAX];
    char   szRelPath[RTPATH_MAX];
    char  *pszRelPath = szRelPath;
    size_t cchRelPath;
    rc = RTUtf16ToUtf8Ex(pwszWinPath, cwcWinPath, &pszRelPath, sizeof(szRelPath), &cchRelPath);
    if (RT_SUCCESS(rc))
        rc = RTPathAbs(szRelPath, &szAbsPath[cchPrefix - cchSkip], sizeof(szAbsPath) - (cchPrefix - cchSkip));
    if (RT_SUCCESS(rc))
    {
        /*
         * Add prefix
         */
        memcpy(szAbsPath, pszPrefix, cchPrefix);

        /*
         * Remove trailing '.' that is used to specify no extension in the Win32/DOS world.
         */
        size_t cchAbsPath = strlen(szAbsPath);
        if (   cchAbsPath > 2
            && szAbsPath[cchAbsPath - 1] == '.')
        {
            char const ch = szAbsPath[cchAbsPath - 2];
            if (   ch != '/'
                && ch != '\\'
                && ch != ':'
                && ch != '.')
                szAbsPath[--cchAbsPath] = '\0';
        }

        /*
         * Finally convert to UNICODE_STRING.
         */
        return rtNtPathUtf8ToUniStr(pNtName, phRootDir, szAbsPath);
    }
    return rc;
}


/**
 * Ensures that the NT string has sufficient storage to hold @a cwcMin RTUTF16
 * chars plus a terminator.
 *
 * The NT string must have been returned by RTNtPathFromWinUtf8 or
 * RTNtPathFromWinUtf16Ex.
 *
 * @returns IPRT status code.
 * @param   pNtName             The NT path string.
 * @param   cwcMin              The minimum number of RTUTF16 chars. Max 32767.
 * @sa      RTNtPathFree
 */
RTDECL(int) RTNtPathEnsureSpace(struct _UNICODE_STRING *pNtName, size_t cwcMin)
{
    if (pNtName->MaximumLength / sizeof(RTUTF16) > cwcMin)
        return VINF_SUCCESS;

    AssertReturn(cwcMin < _64K / sizeof(RTUTF16), VERR_OUT_OF_RANGE);

    size_t const cbMin = (cwcMin + 1) * sizeof(RTUTF16);
    int rc = RTUtf16Realloc(&pNtName->Buffer, cbMin);
    if (RT_SUCCESS(rc))
        pNtName->MaximumLength = (uint16_t)cbMin;
    return rc;
}


/**
 * Frees the native path and root handle.
 *
 * @param   pNtName             The NT path after a successful rtNtPathToNative
 *                              call.
 * @param   phRootDir           The root handle variable from the same call.
 */
static void rtNtPathFreeNative(struct _UNICODE_STRING *pNtName, PHANDLE phRootDir)
{
    RTUtf16Free(pNtName->Buffer);
    pNtName->Buffer = NULL;

    RT_NOREF_PV(phRootDir); /* never returned by rtNtPathToNative */
}


/**
 * Frees the native path and root handle.
 *
 * @param   pNtName             The NT path from a successful RTNtPathToNative
 *                              or RTNtPathFromWinUtf16Ex call.
 * @param   phRootDir           The root handle variable from the same call.
 */
RTDECL(void) RTNtPathFree(struct _UNICODE_STRING *pNtName, HANDLE *phRootDir)
{
    rtNtPathFreeNative(pNtName, phRootDir);
}


/**
 * Wrapper around NtCreateFile.
 *
 * @returns IPRT status code.
 * @param   pszPath             The UTF-8 path.
 * @param   fDesiredAccess      See NtCreateFile.
 * @param   fFileAttribs        See NtCreateFile.
 * @param   fShareAccess        See NtCreateFile.
 * @param   fCreateDisposition  See NtCreateFile.
 * @param   fCreateOptions      See NtCreateFile.
 * @param   fObjAttribs         The OBJECT_ATTRIBUTES::Attributes value, see
 *                              NtCreateFile and InitializeObjectAttributes.
 * @param   phHandle            Where to return the handle.
 * @param   puAction            Where to return the action taken. Optional.
 */
RTDECL(int) RTNtPathOpen(const char *pszPath, ACCESS_MASK fDesiredAccess, ULONG fFileAttribs, ULONG fShareAccess,
                         ULONG fCreateDisposition, ULONG fCreateOptions, ULONG fObjAttribs,
                         PHANDLE phHandle, PULONG_PTR puAction)
{
    *phHandle = RTNT_INVALID_HANDLE_VALUE;

    HANDLE         hRootDir;
    UNICODE_STRING NtName;
    int rc = rtNtPathToNative(&NtName, &hRootDir, pszPath);
    if (RT_SUCCESS(rc))
    {
        HANDLE              hFile = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios   = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, fObjAttribs, hRootDir, NULL);

        NTSTATUS rcNt = NtCreateFile(&hFile,
                                     fDesiredAccess,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /* AllocationSize*/,
                                     fFileAttribs,
                                     fShareAccess,
                                     fCreateDisposition,
                                     fCreateOptions,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);
        if (NT_SUCCESS(rcNt))
        {
            if (puAction)
                *puAction = Ios.Information;
            *phHandle = hFile;
            rc = VINF_SUCCESS;
        }
        else
            rc = RTErrConvertFromNtStatus(rcNt);
        rtNtPathFreeNative(&NtName, &hRootDir);
    }
    return rc;
}


/**
 * Wrapper around NtCreateFile.
 *
 * @returns IPRT status code.
 * @param   pszPath             The UTF-8 path.
 * @param   fDesiredAccess      See NtCreateFile.
 * @param   fShareAccess        See NtCreateFile.
 * @param   fCreateOptions      See NtCreateFile.
 * @param   fObjAttribs         The OBJECT_ATTRIBUTES::Attributes value, see
 *                              NtCreateFile and InitializeObjectAttributes.
 * @param   phHandle            Where to return the handle.
 * @param   pfObjDir            If not NULL, the variable pointed to will be set
 *                              to @c true if we opened an object directory and
 *                              @c false if we opened an directory file (normal
 *                              directory).
 */
RTDECL(int) RTNtPathOpenDir(const char *pszPath, ACCESS_MASK fDesiredAccess, ULONG fShareAccess, ULONG fCreateOptions,
                            ULONG fObjAttribs, PHANDLE phHandle, bool *pfObjDir)
{
    *phHandle = RTNT_INVALID_HANDLE_VALUE;

    HANDLE         hRootDir;
    UNICODE_STRING NtName;
    int rc = rtNtPathToNative(&NtName, &hRootDir, pszPath);
    if (RT_SUCCESS(rc))
    {
        HANDLE              hFile = RTNT_INVALID_HANDLE_VALUE;
        IO_STATUS_BLOCK     Ios   = RTNT_IO_STATUS_BLOCK_INITIALIZER;
        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, fObjAttribs, hRootDir, NULL);

        NTSTATUS rcNt = NtCreateFile(&hFile,
                                     fDesiredAccess,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /* AllocationSize*/,
                                     FILE_ATTRIBUTE_NORMAL,
                                     fShareAccess,
                                     FILE_OPEN,
                                     fCreateOptions,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);
        if (NT_SUCCESS(rcNt))
        {
            if (pfObjDir)
                *pfObjDir = false;
            *phHandle = hFile;
            rc = VINF_SUCCESS;
        }
#ifdef IPRT_WITH_NT_PATH_PASSTHRU
        else if (   pfObjDir
                 && (rcNt == STATUS_OBJECT_NAME_INVALID || rcNt == STATUS_OBJECT_TYPE_MISMATCH)
                 && RTPATH_IS_SLASH(pszPath[0])
                 && RTPATH_IS_SLASH(pszPath[1])
                 && pszPath[2] == '!'
                 && RTPATH_IS_SLASH(pszPath[3]))
        {
            /* Strip trailing slash. */
            if (   NtName.Length > 2
                && RTPATH_IS_SLASH(NtName.Buffer[(NtName.Length / 2) - 1]))
                NtName.Length -= 2;

            /* Rought conversion of the access flags. */
            ULONG fObjDesiredAccess = 0;
            if (fDesiredAccess & (GENERIC_ALL | STANDARD_RIGHTS_ALL))
                fObjDesiredAccess = DIRECTORY_ALL_ACCESS;
            else
            {
                if (fDesiredAccess & (FILE_GENERIC_WRITE | GENERIC_WRITE | STANDARD_RIGHTS_WRITE))
                    fObjDesiredAccess |= DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_OBJECT;
                if (   (fDesiredAccess & (FILE_LIST_DIRECTORY | FILE_GENERIC_READ | GENERIC_READ | STANDARD_RIGHTS_READ))
                    || !fObjDesiredAccess)
                    fObjDesiredAccess |= DIRECTORY_QUERY | FILE_LIST_DIRECTORY;
            }

            rcNt = NtOpenDirectoryObject(&hFile, fObjDesiredAccess, &ObjAttr);
            if (NT_SUCCESS(rcNt))
            {
                *pfObjDir = true;
                *phHandle = hFile;
                rc = VINF_SUCCESS;
            }
            else
                rc = RTErrConvertFromNtStatus(rcNt);
        }
#endif
        else
            rc = RTErrConvertFromNtStatus(rcNt);
        rtNtPathFreeNative(&NtName, &hRootDir);
    }
    return rc;
}


/**
 * Closes an handled open by rtNtPathOpen.
 *
 * @returns IPRT status code
 * @param   hHandle             The handle value.
 */
RTDECL(int) RTNtPathClose(HANDLE hHandle)
{
    NTSTATUS rcNt = NtClose(hHandle);
    if (NT_SUCCESS(rcNt))
        return VINF_SUCCESS;
    return RTErrConvertFromNtStatus(rcNt);
}


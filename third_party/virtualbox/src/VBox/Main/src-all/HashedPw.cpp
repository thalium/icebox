/* $Id: HashedPw.cpp $ */
/** @file
 * Main - Password Hashing
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
#include "HashedPw.h"

#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/sha.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * The prefix of a hashed password.
 */
static const char s_szHashedPwPrefix[] = "#SHA-512#";


/**
 * Checks if the password is a hashed one or not.
 *
 * Empty password are not considered hashed.
 *
 * @returns true if hashed, false if not.
 * @param   a_pstrPassword  Password to inspect.
 */
bool VBoxIsPasswordHashed(RTCString const *a_pstrPassword)
{
    /* prefix */
    if (!a_pstrPassword->startsWith(s_szHashedPwPrefix))
        return false;

    /* salt (optional) */
    const char *pszSalt    = a_pstrPassword->c_str() + sizeof(s_szHashedPwPrefix) - 1;
    const char *pszSaltEnd = strchr(pszSalt, '#');
    if (!pszSaltEnd)
        return false;
    while (pszSalt != pszSaltEnd)
    {
        if (!RT_C_IS_XDIGIT(*pszSalt))
            return false;
        pszSalt++;
    }

    /* hash */
    uint8_t abHash[RTSHA512_HASH_SIZE];
    int rc = RTSha512FromString(pszSaltEnd + 1, abHash);
    return RT_SUCCESS(rc);
}


/**
 * Hashes a plain text password.
 *
 * @param   a_pstrPassword      Plain text password to hash.  This is both
 *                              input and output.
 */
void VBoxHashPassword(RTCString *a_pstrPassword)
{
    AssertReturnVoid(!VBoxIsPasswordHashed(a_pstrPassword));

    char szHashedPw[sizeof(s_szHashedPwPrefix) + 1 + RTSHA512_DIGEST_LEN];
    if (a_pstrPassword->isEmpty())
        szHashedPw[0] = '\0';
    else
    {
        /* prefix */
        char *pszHashedPw = szHashedPw;
        strcpy(pszHashedPw, s_szHashedPwPrefix);
        pszHashedPw += sizeof(s_szHashedPwPrefix) - 1;

        /* salt */
        *pszHashedPw++ = '#'; /* no salt yet */

        /* hash */
        uint8_t abHash[RTSHA512_HASH_SIZE];
        RTSha512(a_pstrPassword->c_str(), a_pstrPassword->length(), abHash);
        int rc = RTSha512ToString(abHash, pszHashedPw, sizeof(szHashedPw) - (pszHashedPw - &szHashedPw[0]));
        AssertReleaseRC(rc);
    }

    *a_pstrPassword = szHashedPw;
}


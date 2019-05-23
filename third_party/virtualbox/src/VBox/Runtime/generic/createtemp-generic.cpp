/* $Id: createtemp-generic.cpp $ */
/** @file
 * IPRT - temporary file and directory creation, generic implementation.
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
#include <iprt/dir.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/rand.h>
#include <iprt/string.h>


static int rtCreateTempValidateTemplate(char *pszTemplate, char **ppszX,
                                        unsigned *pcXes)
{
    /*
     * Validate input and count X'es.
     *
     * The X'es may be trailing, or they may be a cluster of 3 or more inside
     * the file name.
     */
    AssertPtr(pszTemplate);
    AssertPtr(ppszX);
    AssertPtr(pcXes);
    unsigned    cXes = 0;
    char       *pszX = strchr(pszTemplate, '\0');
    if (   pszX != pszTemplate
        && pszX[-1] != 'X')
    {
        /* look inside the file name. */
        char *pszFilename = RTPathFilename(pszTemplate);
        if (   pszFilename
            && (size_t)(pszX - pszFilename) > 3)
        {
            char *pszXEnd = pszX - 1;
            pszFilename += 3;
            do
            {
                if (    pszXEnd[-1] == 'X'
                    &&  pszXEnd[-2] == 'X'
                    &&  pszXEnd[-3] == 'X')
                {
                    pszX = pszXEnd - 3;
                    cXes = 3;
                    break;
                }
            } while (pszXEnd-- != pszFilename);
        }
    }

    /* count them */
    while (   pszX != pszTemplate
           && pszX[-1] == 'X')
    {
        pszX--;
        cXes++;
    }

    /* fail if none found. */
    if (!cXes)
    {
        AssertFailed();
        return VERR_INVALID_PARAMETER;
    }
    *ppszX = pszX;
    *pcXes = cXes;
    return VINF_SUCCESS;
}


static void rtCreateTempFillTemplate(char *pszX, unsigned cXes)
{
    static char const s_sz[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    unsigned j = cXes;
    while (j-- > 0)
        pszX[j] = s_sz[RTRandU32Ex(0, RT_ELEMENTS(s_sz) - 2)];
}


RTDECL(int) RTDirCreateTemp(char *pszTemplate, RTFMODE fMode)
{
    char       *pszX = NULL;
    unsigned    cXes = 0;
    int rc = rtCreateTempValidateTemplate(pszTemplate, &pszX, &cXes);
    if (RT_FAILURE(rc))
    {
        *pszTemplate = '\0';
        return rc;
    }
    /*
     * Try ten thousand times.
     */
    int i = 10000;
    while (i-- > 0)
    {
        rtCreateTempFillTemplate(pszX, cXes);
        rc = RTDirCreate(pszTemplate, fMode, 0);
        if (RT_SUCCESS(rc))
            return rc;
        if (rc != VERR_ALREADY_EXISTS)
        {
            *pszTemplate = '\0';
            return rc;
        }
    }

    /* we've given up. */
    *pszTemplate = '\0';
    return VERR_ALREADY_EXISTS;
}
RT_EXPORT_SYMBOL(RTDirCreateTemp);


/** @todo Test case for this once it is implemented. */
RTDECL(int) RTDirCreateTempSecure(char *pszTemplate)
{
    size_t cchDir;
    char chOld;
    int rc;
    /* bool fSafe; */

    /* Temporarily convert pszTemplate to a path. */
    RTPathParseSimple(pszTemplate, &cchDir, NULL, NULL);
    chOld = pszTemplate[cchDir];
    pszTemplate[cchDir] = '\0';
    /** @todo Implement this. */
    rc = /* RTPathIsSecure(pszTemplate, &fSafe) */ VERR_NOT_SUPPORTED;
    pszTemplate[cchDir] = chOld;
    if (RT_SUCCESS(rc) /* && fSafe */)
        return RTDirCreateTemp(pszTemplate, 0700);
    else
    {
        *pszTemplate = '\0';
        /** @todo Replace VERR_PERMISSION_DENIED.  VERR_INSECURE? */
        return RT_FAILURE(rc) ? rc : VERR_PERMISSION_DENIED;
    }
}
RT_EXPORT_SYMBOL(RTDirCreateTempSecure);


RTDECL(int) RTFileCreateTemp(char *pszTemplate, RTFMODE fMode)
{
    char       *pszX = NULL;
    unsigned    cXes = 0;
    RTFILE      hFile;
    int rc = rtCreateTempValidateTemplate(pszTemplate, &pszX, &cXes);
    if (RT_FAILURE(rc))
    {
        *pszTemplate = '\0';
        return rc;
    }
    /*
     * Try ten thousand times.
     */
    int i = 10000;
    while (i-- > 0)
    {
        uint64_t fOpen =   RTFILE_O_WRITE | RTFILE_O_DENY_ALL
                         | RTFILE_O_CREATE | RTFILE_O_NOT_CONTENT_INDEXED
                         | fMode << RTFILE_O_CREATE_MODE_SHIFT;
        rtCreateTempFillTemplate(pszX, cXes);
        rc = RTFileOpen(&hFile, pszTemplate, fOpen);
        if (RT_SUCCESS(rc))
        {
            RTFileClose(hFile);
            return rc;
        }
        /** @todo Anything else to consider? */
        if (rc != VERR_ALREADY_EXISTS)
        {
            *pszTemplate = '\0';
            return rc;
        }
    }

    /* we've given up. */
    *pszTemplate = '\0';
    return VERR_ALREADY_EXISTS;
}
RT_EXPORT_SYMBOL(RTFileCreateTemp);


/** @todo Test case for this once it is implemented. */
RTDECL(int) RTFileCreateTempSecure(char *pszTemplate)
{
    size_t cchDir;
    char chOld;
    int rc;
    /* bool fSafe; */

    /* Temporarily convert pszTemplate to a path. */
    RTPathParseSimple(pszTemplate, &cchDir, NULL, NULL);
    chOld = pszTemplate[cchDir];
    pszTemplate[cchDir] = '\0';
    /** @todo Implement this. */
    rc = /* RTPathIsSecure(pszTemplate, &fSafe) */ VERR_NOT_SUPPORTED;
    pszTemplate[cchDir] = chOld;
    if (RT_SUCCESS(rc) /* && fSafe */)
        return RTFileCreateTemp(pszTemplate, 0600);
    else
    {
        *pszTemplate = '\0';
        /** @todo Replace VERR_PERMISSION_DENIED.  VERR_INSECURE? */
        return RT_FAILURE(rc) ? rc : VERR_PERMISSION_DENIED;
    }
}
RT_EXPORT_SYMBOL(RTFileCreateTempSecure);


RTDECL(int) RTFileOpenTemp(PRTFILE phFile, char *pszFilename, size_t cbFilename, uint64_t fOpen)
{
    AssertReturn((fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_CREATE, VERR_INVALID_FLAGS);
    AssertReturn(fOpen & RTFILE_O_WRITE, VERR_INVALID_FLAGS);

    /*
     * Start by obtaining the path to the temporary directory.
     */
    int rc = RTPathTemp(pszFilename, cbFilename);
    if (RT_SUCCESS(rc))
    {
        /*
         * Add a filename pattern.
         */
        static char const s_szTemplate[] = "IPRT-XXXXXXXXXXXX.tmp";
        rc = RTPathAppend(pszFilename, cbFilename, s_szTemplate);
        if (RT_SUCCESS(rc))
        {
            char * const pszX = RTStrEnd(pszFilename, cbFilename) - (sizeof(s_szTemplate) - 1) + 5;
            unsigned     cXes = sizeof(s_szTemplate) - 1 - 4 - 5;
            Assert(pszX[0] == 'X'); Assert(pszX[-1] == '-'); Assert(pszX[cXes] == '.');

            /*
             * Try 10000 times with random names.
             */
            unsigned cTriesLeft = 10000;
            while (cTriesLeft-- > 0)
            {
                rtCreateTempFillTemplate(pszX, cXes);
                rc = RTFileOpen(phFile, pszFilename, fOpen);
                if (RT_SUCCESS(rc))
                    return rc;
            }
        }
    }

    if (cbFilename)
        *pszFilename = '\0';
    *phFile = NIL_RTFILE;
    return rc;
}
RT_EXPORT_SYMBOL(RTFileOpenTemp);


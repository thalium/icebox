/* $Id: RTPathParse.cpp.h $ */
/** @file
 * IPRT - RTPathParse - Code Template.
 *
 * This file included multiple times with different path style macros.
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



/**
 * @copydoc RTPathParse
 */
static int RTPATH_STYLE_FN(rtPathParse)(const char *pszPath, PRTPATHPARSED pParsed, size_t cbParsed, uint32_t fFlags)
{
    /*
     * Parse the root specification if present and initialize the parser state
     * (keep it on the stack for speed).
     */
    uint32_t const  cMaxComps =  cbParsed < RT_UOFFSETOF(RTPATHPARSED, aComps[0xfff0])
                              ? (uint32_t)((cbParsed - RT_UOFFSETOF(RTPATHPARSED, aComps)) / sizeof(pParsed->aComps[0]))
                              : 0xfff0;
    uint32_t        idxComp   = 0;
    uint32_t        cchPath;
    uint32_t        offCur;
    uint16_t        fProps;

    if (RTPATH_IS_SLASH(pszPath[0]))
    {
        if (fFlags & RTPATH_STR_F_NO_START)
        {
            offCur = 1;
            while (RTPATH_IS_SLASH(pszPath[offCur]))
                offCur++;
            if (!pszPath[offCur])
                return VERR_PATH_ZERO_LENGTH;
            fProps = RTPATH_PROP_RELATIVE | RTPATH_PROP_EXTRA_SLASHES;
            cchPath = 0;
        }
#if RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS
        else if (   RTPATH_IS_SLASH(pszPath[1])
                 && !RTPATH_IS_SLASH(pszPath[2])
                 && pszPath[2])
        {
            /* UNC - skip to the end of the potential namespace or computer name. */
            offCur = 2;
            while (!RTPATH_IS_SLASH(pszPath[offCur]) && pszPath[offCur])
                offCur++;

            /* If there is another slash, we considered it a valid UNC path, if
               not it's just a root path with an extra slash thrown in. */
            if (RTPATH_IS_SLASH(pszPath[offCur]))
            {
                fProps = RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_UNC | RTPATH_PROP_ABSOLUTE;
                offCur++;
                cchPath = offCur;
            }
            else
            {
                fProps = RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_RELATIVE;
                offCur = 1;
                cchPath = 1;
            }
        }
#endif
        else
        {
#if RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS
            fProps = RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_RELATIVE;
#else
            fProps = RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE;
#endif
            offCur = 1;
            cchPath = 1;
        }
    }
#if RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS
    else if (RT_C_IS_ALPHA(pszPath[0]) && pszPath[1] == ':')
    {
        if (!RTPATH_IS_SLASH(pszPath[2]))
        {
            fProps = RTPATH_PROP_VOLUME | RTPATH_PROP_RELATIVE;
            offCur = 2;
        }
        else
        {
            fProps = RTPATH_PROP_VOLUME | RTPATH_PROP_ROOT_SLASH | RTPATH_PROP_ABSOLUTE;
            offCur = 3;
        }
        cchPath = offCur;
    }
#endif
    else
    {
        fProps  = RTPATH_PROP_RELATIVE;
        offCur  = 0;
        cchPath = 0;
    }

    /* Add it to the component array . */
    if (offCur && !(fFlags & RTPATH_STR_F_NO_START))
    {
        cchPath = offCur;
        if (idxComp < cMaxComps)
        {
            pParsed->aComps[idxComp].off = 0;
            pParsed->aComps[idxComp].cch = offCur;
        }
        idxComp++;

        /* Skip unnecessary slashes following the root-spec. */
        if (RTPATH_IS_SLASH(pszPath[offCur]))
        {
            fProps |= RTPATH_PROP_EXTRA_SLASHES;
            do
                offCur++;
            while (RTPATH_IS_SLASH(pszPath[offCur]));
        }
    }

    /*
     * Parse the rest.
     */
    if (pszPath[offCur])
    {
        for (;;)
        {
            Assert(!RTPATH_IS_SLASH(pszPath[offCur]));

            /* Find the end of the component. */
            uint32_t    offStart = offCur;
            char        ch;
            while ((ch = pszPath[offCur]) != '\0' && !RTPATH_IS_SLASH(ch))
                offCur++;
            if (offCur >= _64K)
                return VERR_FILENAME_TOO_LONG;

            /* Add it. */
            uint32_t cchComp = offCur - offStart;
            if (idxComp < cMaxComps)
            {
                pParsed->aComps[idxComp].off = offStart;
                pParsed->aComps[idxComp].cch = cchComp;
            }
            idxComp++;
            cchPath += cchComp;

            /* Look for '.' and '..' references. */
            if (cchComp == 1 && pszPath[offCur - 1] == '.')
                fProps |= RTPATH_PROP_DOT_REFS;
            else if (cchComp == 2 && pszPath[offCur - 1] == '.' && pszPath[offCur - 2] == '.')
            {
                fProps &= ~RTPATH_PROP_ABSOLUTE;
                fProps |= RTPATH_PROP_DOTDOT_REFS | RTPATH_PROP_RELATIVE;
            }

            /* Skip unnecessary slashes. Leave ch unchanged! */
            char ch2 = ch;
            if (ch2)
            {
                ch2 = pszPath[++offCur];
                if (RTPATH_IS_SLASH(ch2))
                {
                    fProps |= RTPATH_PROP_EXTRA_SLASHES;
                    do
                        ch2 = pszPath[++offCur];
                    while (RTPATH_IS_SLASH(ch2));
                }
            }

            /* The end? */
            if (ch2 == '\0')
            {
                pParsed->offSuffix = offCur;
                pParsed->cchSuffix = 0;
                if (ch)
                {
                    if (!(fFlags & RTPATH_STR_F_NO_END))
                    {
                        fProps |= RTPATH_PROP_DIR_SLASH; /* (not counted in component, but in cchPath) */
                        cchPath++;
                    }
                    else
                        fProps |= RTPATH_PROP_EXTRA_SLASHES;
                }
                else if (!(fFlags & RTPATH_STR_F_NO_END))
                {
                    fProps |= RTPATH_PROP_FILENAME;

                    /* look for an ? */
                    uint32_t offSuffix = offStart + cchComp;
                    while (offSuffix-- > offStart)
                        if (pszPath[offSuffix] == '.')
                        {
                            uint32_t cchSuffix = offStart + cchComp - offSuffix;
                            if (cchSuffix > 1 && offStart != offSuffix)
                            {
                                pParsed->cchSuffix = cchSuffix;
                                pParsed->offSuffix = offSuffix;
                                fProps |= RTPATH_PROP_SUFFIX;
                            }
                            break;
                        }
                }
                break;
            }

            /* No, not the end. Account for an separator before we restart the loop. */
            cchPath += sizeof(RTPATH_SLASH_STR) - 1;
        }
    }
    else
    {
        pParsed->offSuffix = offCur;
        pParsed->cchSuffix = 0;
    }
    if (offCur >= _64K)
        return VERR_FILENAME_TOO_LONG;

    /*
     * Store the remainder of the state and we're done.
     */
    pParsed->fProps  = fProps;
    pParsed->cchPath = cchPath;
    pParsed->cComps  = idxComp;

    return idxComp <= cMaxComps ? VINF_SUCCESS : VERR_BUFFER_OVERFLOW;
}


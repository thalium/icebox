/* $Id: utf8-posix.cpp $ */
/** @file
 * IPRT - UTF-8 helpers, POSIX.
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
#include <iprt/string.h>
#include "internal/iprt.h"

#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>

#include <errno.h>
#include <locale.h>

/* iconv prototype changed with 165+ (thanks to PSARC/2010/160 Bugster 7037400) */
#if defined(RT_OS_SOLARIS)
# if !defined(_XPG6)
#  define VBOX_XPG6_TMP_DEF
#  define _XPG6
# endif
# if defined(__USE_LEGACY_PROTOTYPES__)
#  define VBOX_LEGACY_PROTO_TMP_DEF
#  undef __USE_LEGACY_PROTOTYPES__
# endif
#endif /* RT_OS_SOLARIS */

# include <iconv.h>

#if defined(RT_OS_SOLARIS)
# if defined(VBOX_XPG6_TMP_DEF)
#  undef _XPG6
#  undef VBOX_XPG6_TMP_DEF
# endif
# if defined(VBOX_LEGACY_PROTO_TMP_DEF)
#  define __USE_LEGACY_PROTOTYPES__
#  undef VBOX_LEGACY_PROTO_TMP_DEF
# endif
#endif /* RT_OS_SOLARIS */

#include <wctype.h>

#include <langinfo.h>

#include "internal/alignmentchecks.h"
#include "internal/string.h"
#ifdef RT_WITH_ICONV_CACHE
# include "internal/thread.h"
AssertCompile(sizeof(iconv_t) <= sizeof(void *));
#endif


/* There are different opinions about the constness of the input buffer. */
#if defined(RT_OS_LINUX) || defined(RT_OS_HAIKU) || defined(RT_OS_SOLARIS) \
  || (defined(RT_OS_DARWIN) && defined(_DARWIN_FEATURE_UNIX_CONFORMANCE))
# define NON_CONST_ICONV_INPUT
#endif
#ifdef RT_OS_FREEBSD
# include <sys/param.h>
# if __FreeBSD_version >= 1002000 /* Changed around 10.2.2 (https://svnweb.freebsd.org/base?view=revision&revision=281550) */
#  define NON_CONST_ICONV_INPUT
# else
# error __FreeBSD_version__
# endif
#endif


/**
 * Gets the codeset of the current locale (LC_CTYPE).
 *
 * @returns Pointer to read-only string with the codeset name.
 */
DECLHIDDEN(const char *) rtStrGetLocaleCodeset(void)
{
    return nl_langinfo(CODESET);
}


#ifdef RT_WITH_ICONV_CACHE

/**
 * Initializes the iconv handle cache associated with a thread.
 *
 * @param   pThread             The thread in question.
 */
DECLHIDDEN(void) rtStrIconvCacheInit(PRTTHREADINT pThread)
{
    for (size_t i = 0; i < RT_ELEMENTS(pThread->ahIconvs); i++)
        pThread->ahIconvs[i] = (iconv_t)-1;
}

/**
 * Destroys the iconv handle cache associated with a thread.
 *
 * @param   pThread             The thread in question.
 */
DECLHIDDEN(void) rtStrIconvCacheDestroy(PRTTHREADINT pThread)
{
    for (size_t i = 0; i < RT_ELEMENTS(pThread->ahIconvs); i++)
    {
        iconv_t hIconv = (iconv_t)pThread->ahIconvs[i];
        pThread->ahIconvs[i] = (iconv_t)-1;
        if (hIconv != (iconv_t)-1)
            iconv_close(hIconv);
    }
}


/**
 * Converts a string from one charset to another.
 *
 * @returns iprt status code.
 * @param   pvInput         Pointer to intput string.
 * @param   cbInput         Size (in bytes) of input string. Excludes any terminators.
 * @param   pszInputCS      Codeset of the input string.
 * @param   ppvOutput       Pointer to pointer to output buffer if cbOutput > 0.
 *                          If cbOutput is 0 this is where the pointer to the allocated
 *                          buffer is stored.
 * @param   cbOutput        Size of the passed in buffer.
 * @param   pszOutputCS     Codeset of the input string.
 * @param   cFactor         Input vs. output size factor.
 * @param   phIconv         Pointer to the cache entry.
 */
static int rtstrConvertCached(const void *pvInput, size_t cbInput, const char *pszInputCS,
                              void **ppvOutput, size_t cbOutput, const char *pszOutputCS,
                              unsigned cFactor, iconv_t *phIconv)
{
    /*
     * Allocate buffer
     */
    bool    fUcs2Term;
    void   *pvOutput;
    size_t  cbOutput2;
    if (!cbOutput)
    {
        cbOutput2 = cbInput * cFactor;
        pvOutput = RTMemTmpAlloc(cbOutput2 + sizeof(RTUTF16));
        if (!pvOutput)
            return VERR_NO_TMP_MEMORY;
        fUcs2Term = true;
    }
    else
    {
        pvOutput = *ppvOutput;
        fUcs2Term = !strcmp(pszOutputCS, "UCS-2")
                 || !strcmp(pszOutputCS, "UTF-16")
                 || !strcmp(pszOutputCS, "ucs-2")
                 || !strcmp(pszOutputCS, "utf-16");
        cbOutput2 = cbOutput - (fUcs2Term ? sizeof(RTUTF16) : 1);
        if (cbOutput2 > cbOutput)
            return VERR_BUFFER_OVERFLOW;
    }

    /*
     * Use a loop here to retry with bigger buffers.
     */
    for (unsigned cTries = 10; cTries > 0; cTries--)
    {
        /*
         * Create conversion object if necessary.
         */
        iconv_t hIconv = (iconv_t)*phIconv;
        if (hIconv == (iconv_t)-1)
        {
#if defined(RT_OS_SOLARIS) || defined(RT_OS_NETBSD)
            /* Some systems don't grok empty codeset strings, so help them find the current codeset. */
            if (!*pszInputCS)
                pszInputCS = rtStrGetLocaleCodeset();
            if (!*pszOutputCS)
                pszOutputCS = rtStrGetLocaleCodeset();
#endif
            IPRT_ALIGNMENT_CHECKS_DISABLE(); /* glibc causes trouble */
            *phIconv = hIconv = iconv_open(pszOutputCS, pszInputCS);
            IPRT_ALIGNMENT_CHECKS_ENABLE();
        }
        if (hIconv != (iconv_t)-1)
        {
            /*
             * Do the conversion.
             */
            size_t      cbInLeft = cbInput;
            size_t      cbOutLeft = cbOutput2;
            const void *pvInputLeft = pvInput;
            void       *pvOutputLeft = pvOutput;
            size_t      cchNonRev;
#ifdef NON_CONST_ICONV_INPUT
            cchNonRev = iconv(hIconv, (char **)&pvInputLeft, &cbInLeft, (char **)&pvOutputLeft, &cbOutLeft);
#else
            cchNonRev = iconv(hIconv, (const char **)&pvInputLeft, &cbInLeft, (char **)&pvOutputLeft, &cbOutLeft);
#endif
            if (cchNonRev != (size_t)-1)
            {
                if (!cbInLeft)
                {
                    /*
                     * We're done, just add the terminator and return.
                     * (Two terminators to support UCS-2 output, too.)
                     */
                    ((char *)pvOutputLeft)[0] = '\0';
                    if (fUcs2Term)
                        ((char *)pvOutputLeft)[1] = '\0';
                    *ppvOutput = pvOutput;
                    if (cchNonRev == 0)
                        return VINF_SUCCESS;
                    return VWRN_NO_TRANSLATION;
                }
                errno = E2BIG;
            }

            /*
             * If we failed because of output buffer space we'll
             * increase the output buffer size and retry.
             */
            if (errno == E2BIG)
            {
                if (!cbOutput)
                {
                    RTMemTmpFree(pvOutput);
                    cbOutput2 *= 2;
                    pvOutput = RTMemTmpAlloc(cbOutput2 + sizeof(RTUTF16));
                    if (!pvOutput)
                        return VERR_NO_TMP_MEMORY;
                    continue;
                }
                return VERR_BUFFER_OVERFLOW;
            }

            /*
             * Close the handle on all other errors to make sure we won't carry
             * any bad state with us.
             */
            *phIconv = (iconv_t)-1;
            iconv_close(hIconv);
        }
        break;
    }

    /* failure */
    if (!cbOutput)
        RTMemTmpFree(pvOutput);
    return VERR_NO_TRANSLATION;
}

#endif /* RT_WITH_ICONV_CACHE */

/**
 * Converts a string from one charset to another without using the handle cache.
 *
 * @returns IPRT status code.
 *
 * @param   pvInput         Pointer to intput string.
 * @param   cbInput         Size (in bytes) of input string. Excludes any terminators.
 * @param   pszInputCS      Codeset of the input string.
 * @param   ppvOutput       Pointer to pointer to output buffer if cbOutput > 0.
 *                          If cbOutput is 0 this is where the pointer to the allocated
 *                          buffer is stored.
 * @param   cbOutput        Size of the passed in buffer.
 * @param   pszOutputCS     Codeset of the input string.
 * @param   cFactor         Input vs. output size factor.
 */
static int rtStrConvertUncached(const void *pvInput, size_t cbInput, const char *pszInputCS,
                                void **ppvOutput, size_t cbOutput, const char *pszOutputCS,
                                unsigned cFactor)
{
    /*
     * Allocate buffer
     */
    bool    fUcs2Term;
    void   *pvOutput;
    size_t  cbOutput2;
    if (!cbOutput)
    {
        cbOutput2 = cbInput * cFactor;
        pvOutput = RTMemTmpAlloc(cbOutput2 + sizeof(RTUTF16));
        if (!pvOutput)
            return VERR_NO_TMP_MEMORY;
        fUcs2Term = true;
    }
    else
    {
        pvOutput = *ppvOutput;
        fUcs2Term = !strcmp(pszOutputCS, "UCS-2");
        cbOutput2 = cbOutput - (fUcs2Term ? sizeof(RTUTF16) : 1);
        if (cbOutput2 > cbOutput)
            return VERR_BUFFER_OVERFLOW;
    }

    /*
     * Use a loop here to retry with bigger buffers.
     */
    for (unsigned cTries = 10; cTries > 0; cTries--)
    {
        /*
         * Create conversion object.
         */
#if defined(RT_OS_SOLARIS) || defined(RT_OS_NETBSD)
        /* Some systems don't grok empty codeset strings, so help them find the current codeset. */
        if (!*pszInputCS)
            pszInputCS = rtStrGetLocaleCodeset();
        if (!*pszOutputCS)
            pszOutputCS = rtStrGetLocaleCodeset();
#endif
        IPRT_ALIGNMENT_CHECKS_DISABLE(); /* glibc causes trouble */
        iconv_t icHandle = iconv_open(pszOutputCS, pszInputCS);
        IPRT_ALIGNMENT_CHECKS_ENABLE();
        if (icHandle != (iconv_t)-1)
        {
            /*
             * Do the conversion.
             */
            size_t      cbInLeft = cbInput;
            size_t      cbOutLeft = cbOutput2;
            const void *pvInputLeft = pvInput;
            void       *pvOutputLeft = pvOutput;
            size_t      cchNonRev;
#ifdef NON_CONST_ICONV_INPUT
            cchNonRev = iconv(icHandle, (char **)&pvInputLeft, &cbInLeft, (char **)&pvOutputLeft, &cbOutLeft);
#else
            cchNonRev = iconv(icHandle, (const char **)&pvInputLeft, &cbInLeft, (char **)&pvOutputLeft, &cbOutLeft);
#endif
            if (cchNonRev != (size_t)-1)
            {
                if (!cbInLeft)
                {
                    /*
                     * We're done, just add the terminator and return.
                     * (Two terminators to support UCS-2 output, too.)
                     */
                    iconv_close(icHandle);
                    ((char *)pvOutputLeft)[0] = '\0';
                    if (fUcs2Term)
                        ((char *)pvOutputLeft)[1] = '\0';
                    *ppvOutput = pvOutput;
                    if (cchNonRev == 0)
                        return VINF_SUCCESS;
                    return VWRN_NO_TRANSLATION;
                }
                errno = E2BIG;
            }
            iconv_close(icHandle);

            /*
             * If we failed because of output buffer space we'll
             * increase the output buffer size and retry.
             */
            if (errno == E2BIG)
            {
                if (!cbOutput)
                {
                    RTMemTmpFree(pvOutput);
                    cbOutput2 *= 2;
                    pvOutput = RTMemTmpAlloc(cbOutput2 + sizeof(RTUTF16));
                    if (!pvOutput)
                        return VERR_NO_TMP_MEMORY;
                    continue;
                }
                return VERR_BUFFER_OVERFLOW;
            }
        }
        break;
    }

    /* failure */
    if (!cbOutput)
        RTMemTmpFree(pvOutput);
    return VERR_NO_TRANSLATION;
}


/**
 * Wrapper that selects rtStrConvertCached or rtStrConvertUncached.
 *
 * @returns IPRT status code.
 *
 * @param   pszInput        Pointer to intput string.
 * @param   cchInput        Size (in bytes) of input string. Excludes any
 *                          terminators.
 * @param   pszInputCS      Codeset of the input string.
 * @param   ppszOutput      Pointer to pointer to output buffer if cbOutput > 0.
 *                          If cbOutput is 0 this is where the pointer to the
 *                          allocated buffer is stored.
 * @param   cbOutput        Size of the passed in buffer.
 * @param   pszOutputCS     Codeset of the input string.
 * @param   cFactor         Input vs. output size factor.
 * @param   enmCacheIdx     The iconv cache index.
 */
DECLINLINE(int) rtStrConvertWrapper(const char *pchInput, size_t cchInput, const char *pszInputCS,
                                    char **ppszOutput, size_t cbOutput, const char *pszOutputCS,
                                    unsigned cFactor, RTSTRICONV enmCacheIdx)
{
#ifdef RT_WITH_ICONV_CACHE
    RTTHREAD hSelf = RTThreadSelf();
    if (hSelf != NIL_RTTHREAD)
    {
        PRTTHREADINT pThread = rtThreadGet(hSelf);
        if (pThread)
        {
            if ((pThread->fIntFlags & (RTTHREADINT_FLAGS_ALIEN | RTTHREADINT_FLAGS_MAIN)) != RTTHREADINT_FLAGS_ALIEN)
            {
                int rc = rtstrConvertCached(pchInput, cchInput, pszInputCS,
                                            (void **)ppszOutput, cbOutput, pszOutputCS,
                                            cFactor, (iconv_t *)&pThread->ahIconvs[enmCacheIdx]);
                rtThreadRelease(pThread);
                return rc;
            }
            rtThreadRelease(pThread);
        }
    }
#endif
    return rtStrConvertUncached(pchInput, cchInput, pszInputCS,
                                (void **)ppszOutput, cbOutput, pszOutputCS,
                                cFactor);
}


/**
 * Internal API for use by the path conversion code.
 *
 * @returns IPRT status code.
 *
 * @param   pszInput        Pointer to intput string.
 * @param   cchInput        Size (in bytes) of input string. Excludes any
 *                          terminators.
 * @param   pszInputCS      Codeset of the input string.
 * @param   ppszOutput      Pointer to pointer to output buffer if cbOutput > 0.
 *                          If cbOutput is 0 this is where the pointer to the
 *                          allocated buffer is stored.
 * @param   cbOutput        Size of the passed in buffer.
 * @param   pszOutputCS     Codeset of the input string.
 * @param   cFactor         Input vs. output size factor.
 * @param   enmCacheIdx     The iconv cache index.
 */
DECLHIDDEN(int) rtStrConvert(const char *pchInput, size_t cchInput, const char *pszInputCS,
                             char **ppszOutput, size_t cbOutput, const char *pszOutputCS,
                             unsigned cFactor, RTSTRICONV enmCacheIdx)
{
    Assert(enmCacheIdx >= 0 && enmCacheIdx < RTSTRICONV_END);
    return rtStrConvertWrapper(pchInput, cchInput, pszInputCS,
                               ppszOutput, cbOutput, pszOutputCS,
                               cFactor, enmCacheIdx);
}


RTR3DECL(int)  RTStrUtf8ToCurrentCPTag(char **ppszString, const char *pszString, const char *pszTag)
{
    Assert(ppszString);
    Assert(pszString);
    *ppszString = NULL;

    /*
     * Assume result string length is not longer than UTF-8 string.
     */
    size_t cch = strlen(pszString);
    if (cch <= 0)
    {
        /* zero length string passed. */
        *ppszString = (char *)RTMemTmpAllocZTag(sizeof(char), pszTag);
        if (*ppszString)
            return VINF_SUCCESS;
        return VERR_NO_TMP_MEMORY;
    }
    return rtStrConvertWrapper(pszString, cch, "UTF-8", ppszString, 0, "", 1, RTSTRICONV_UTF8_TO_LOCALE);
}


RTR3DECL(int)  RTStrCurrentCPToUtf8Tag(char **ppszString, const char *pszString, const char *pszTag)
{
    Assert(ppszString);
    Assert(pszString);
    *ppszString = NULL;

    /*
     * Attempt with UTF-8 length of 2x the native length.
     */
    size_t cch = strlen(pszString);
    if (cch <= 0)
    {
        /* zero length string passed. */
        *ppszString = (char *)RTMemTmpAllocZTag(sizeof(char), pszTag);
        if (*ppszString)
            return VINF_SUCCESS;
        return VERR_NO_TMP_MEMORY;
    }
    return rtStrConvertWrapper(pszString, cch, "", ppszString, 0, "UTF-8", 2, RTSTRICONV_LOCALE_TO_UTF8);
}


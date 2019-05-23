/* $Id: http-curl.cpp $ */
/** @file
 * IPRT - HTTP client API, cURL based.
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
#define LOG_GROUP RTLOGGROUP_HTTP
#include <iprt/http.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/cidr.h>
#include <iprt/crypto/store.h>
#include <iprt/ctype.h>
#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/ldr.h>
#include <iprt/log.h>
#include <iprt/mem.h>
#include <iprt/net.h>
#include <iprt/once.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/uni.h>
#include <iprt/uri.h>

#include "internal/magics.h"

#ifdef RT_OS_WINDOWS /* curl.h drags in windows.h which isn't necessarily -Wall clean. */
# include <iprt/win/windows.h>
#endif
#include <curl/curl.h>

#ifdef RT_OS_DARWIN
# include <CoreFoundation/CoreFoundation.h>
# include <SystemConfiguration/SystemConfiguration.h>
# include <CoreServices/CoreServices.h>
#endif
#ifdef RT_OS_WINDOWS
# include <Winhttp.h>
# include "../r3/win/internal-r3-win.h"
#endif

#ifdef RT_OS_LINUX
//# define IPRT_USE_LIBPROXY
#endif
#ifdef IPRT_USE_LIBPROXY
# include <stdlib.h> /* free */
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Internal HTTP client instance.
 */
typedef struct RTHTTPINTERNAL
{
    /** Magic value. */
    uint32_t            u32Magic;
    /** cURL handle. */
    CURL               *pCurl;
    /** The last response code. */
    long                lLastResp;
    /** Custom headers/ */
    struct curl_slist  *pHeaders;
    /** CA certificate file for HTTPS authentication. */
    char               *pszCaFile;
    /** Whether to delete the CA on destruction. */
    bool                fDeleteCaFile;

    /** Set if we've applied a CURLOTP_USERAGENT already.  */
    bool                fHaveSetUserAgent;
    /** Set if we've got a user agent header, otherwise clear.  */
    bool                fHaveUserAgentHeader;

    /** @name Proxy settings.
     * When fUseSystemProxySettings is set, the other members will be updated each
     * time we're presented with a new URL.  The members reflect the cURL
     * configuration.
     *
     * @{ */
    /** Set if we should use the system proxy settings for a URL.
     * This means reconfiguring cURL for each request.  */
    bool                fUseSystemProxySettings;
    /** Set if we've detected no proxy necessary. */
    bool                fNoProxy;
    /** Proxy host name (RTStrFree). */
    char               *pszProxyHost;
    /** Proxy port number (UINT32_MAX if not specified). */
    uint32_t            uProxyPort;
    /** The proxy type (CURLPROXY_HTTP, CURLPROXY_SOCKS5, ++). */
    curl_proxytype      enmProxyType;
    /** Proxy username (RTStrFree). */
    char               *pszProxyUsername;
    /** Proxy password (RTStrFree). */
    char               *pszProxyPassword;
    /** @} */

    /** Abort the current HTTP request if true. */
    bool volatile       fAbort;
    /** Set if someone is preforming an HTTP operation. */
    bool volatile       fBusy;
    /** The location field for 301 responses. */
    char               *pszRedirLocation;

    /** Output callback data. */
    union
    {
        /** For file destination.  */
        RTFILE          hFile;
        /** For memory destination. */
        struct
        {
            /** The current size (sans terminator char). */
            size_t      cb;
            /** The currently allocated size. */
            size_t      cbAllocated;
            /** Pointer to the buffer. */
            uint8_t    *pb;
        } Mem;
    } Output;
    /** Output callback status. */
    int                 rcOutput;
    /** Download size hint set by the progress callback. */
    uint64_t            cbDownloadHint;
    /** Callback called during download. */
    PRTHTTPDOWNLDPROGRCALLBACK pfnDownloadProgress;
    /** User pointer parameter for pfnDownloadProgress. */
    void               *pvDownloadProgressUser;
} RTHTTPINTERNAL;
/** Pointer to an internal HTTP client instance. */
typedef RTHTTPINTERNAL *PRTHTTPINTERNAL;


#ifdef RT_OS_WINDOWS
/** @name Windows: Types for dynamically resolved APIs
 * @{ */
typedef HINTERNET (WINAPI * PFNWINHTTPOPEN)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
typedef BOOL      (WINAPI * PFNWINHTTPCLOSEHANDLE)(HINTERNET);
typedef BOOL      (WINAPI * PFNWINHTTPGETPROXYFORURL)(HINTERNET, LPCWSTR, WINHTTP_AUTOPROXY_OPTIONS *, WINHTTP_PROXY_INFO *);
typedef BOOL      (WINAPI * PFNWINHTTPGETDEFAULTPROXYCONFIGURATION)(WINHTTP_PROXY_INFO *);
typedef BOOL      (WINAPI * PFNWINHTTPGETIEPROXYCONFIGFORCURRENTUSER)(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *);
/** @} */
#endif

#ifdef IPRT_USE_LIBPROXY
typedef struct px_proxy_factory *PLIBPROXYFACTORY;
typedef PLIBPROXYFACTORY (* PFNLIBPROXYFACTORYCTOR)(void);
typedef void             (* PFNLIBPROXYFACTORYDTOR)(PLIBPROXYFACTORY);
typedef char          ** (* PFNLIBPROXYFACTORYGETPROXIES)(PLIBPROXYFACTORY, const char *);
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @def RTHTTP_MAX_MEM_DOWNLOAD_SIZE
 * The max size we are allowed to download to a memory buffer.
 *
 * @remarks The minus 1 is for the trailing zero terminator we always add.
 */
#if ARCH_BITS == 64
# define RTHTTP_MAX_MEM_DOWNLOAD_SIZE       (UINT32_C(64)*_1M - 1)
#else
# define RTHTTP_MAX_MEM_DOWNLOAD_SIZE       (UINT32_C(32)*_1M - 1)
#endif

/** Checks whether a cURL return code indicates success. */
#define CURL_SUCCESS(rcCurl)    RT_LIKELY(rcCurl == CURLE_OK)
/** Checks whether a cURL return code indicates failure. */
#define CURL_FAILURE(rcCurl)    RT_UNLIKELY(rcCurl != CURLE_OK)

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTHTTP_VALID_RETURN_RC(hHttp, rcCurl) \
    do { \
        AssertPtrReturn((hHttp), (rcCurl)); \
        AssertReturn((hHttp)->u32Magic == RTHTTP_MAGIC, (rcCurl)); \
    } while (0)

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTHTTP_VALID_RETURN(hHTTP) RTHTTP_VALID_RETURN_RC((hHttp), VERR_INVALID_HANDLE)

/** Validates a handle and returns (void) if not valid. */
#define RTHTTP_VALID_RETURN_VOID(hHttp) \
    do { \
        AssertPtrReturnVoid(hHttp); \
        AssertReturnVoid((hHttp)->u32Magic == RTHTTP_MAGIC); \
    } while (0)


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef RT_OS_WINDOWS
/** @name Windows: Dynamically resolved APIs
 * @{ */
static RTONCE                                   g_WinResolveImportsOnce = RTONCE_INITIALIZER;
static PFNWINHTTPOPEN                           g_pfnWinHttpOpen = NULL;
static PFNWINHTTPCLOSEHANDLE                    g_pfnWinHttpCloseHandle = NULL;
static PFNWINHTTPGETPROXYFORURL                 g_pfnWinHttpGetProxyForUrl = NULL;
static PFNWINHTTPGETDEFAULTPROXYCONFIGURATION   g_pfnWinHttpGetDefaultProxyConfiguration = NULL;
static PFNWINHTTPGETIEPROXYCONFIGFORCURRENTUSER g_pfnWinHttpGetIEProxyConfigForCurrentUser = NULL;
/** @} */
#endif

#ifdef IPRT_USE_LIBPROXY
/** @name Dynamaically resolved libproxy APIs.
 * @{ */
static RTONCE                                   g_LibProxyResolveImportsOnce = RTONCE_INITIALIZER;
static RTLDRMOD                                 g_hLdrLibProxy = NIL_RTLDRMOD;
static PFNLIBPROXYFACTORYCTOR                   g_pfnLibProxyFactoryCtor = NULL;
static PFNLIBPROXYFACTORYDTOR                   g_pfnLibProxyFactoryDtor = NULL;
static PFNLIBPROXYFACTORYGETPROXIES             g_pfnLibProxyFactoryGetProxies = NULL;
/** @} */
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void rtHttpUnsetCaFile(PRTHTTPINTERNAL pThis);
#ifdef RT_OS_DARWIN
static int rtHttpDarwinTryConfigProxies(PRTHTTPINTERNAL pThis, CFArrayRef hArrayProxies, CFURLRef hUrlTarget, bool fIgnorePacType);
#endif


RTR3DECL(int) RTHttpCreate(PRTHTTP phHttp)
{
    AssertPtrReturn(phHttp, VERR_INVALID_PARAMETER);

    /** @todo r=bird: rainy day: curl_global_init is not thread safe, only a
     *        problem if multiple threads get here at the same time. */
    int rc = VERR_HTTP_INIT_FAILED;
    CURLcode rcCurl = curl_global_init(CURL_GLOBAL_ALL);
    if (!CURL_FAILURE(rcCurl))
    {
        CURL *pCurl = curl_easy_init();
        if (pCurl)
        {
            PRTHTTPINTERNAL pThis = (PRTHTTPINTERNAL)RTMemAllocZ(sizeof(RTHTTPINTERNAL));
            if (pThis)
            {
                pThis->u32Magic                 = RTHTTP_MAGIC;
                pThis->pCurl                    = pCurl;
                pThis->fUseSystemProxySettings  = true;

                *phHttp = (RTHTTP)pThis;

                return VINF_SUCCESS;
            }
            rc = VERR_NO_MEMORY;
        }
        else
            rc = VERR_HTTP_INIT_FAILED;
    }
    curl_global_cleanup();
    return rc;
}


RTR3DECL(void) RTHttpDestroy(RTHTTP hHttp)
{
    if (hHttp == NIL_RTHTTP)
        return;

    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN_VOID(pThis);

    Assert(!pThis->fBusy);

    pThis->u32Magic = RTHTTP_MAGIC_DEAD;

    curl_easy_cleanup(pThis->pCurl);
    pThis->pCurl = NULL;

    if (pThis->pHeaders)
        curl_slist_free_all(pThis->pHeaders);

    rtHttpUnsetCaFile(pThis);
    Assert(!pThis->pszCaFile);

    if (pThis->pszRedirLocation)
        RTStrFree(pThis->pszRedirLocation);

    RTStrFree(pThis->pszProxyHost);
    RTStrFree(pThis->pszProxyUsername);
    if (pThis->pszProxyPassword)
    {
        RTMemWipeThoroughly(pThis->pszProxyPassword, strlen(pThis->pszProxyPassword), 2);
        RTStrFree(pThis->pszProxyPassword);
    }

    RTMemFree(pThis);

    curl_global_cleanup();
}


RTR3DECL(int) RTHttpAbort(RTHTTP hHttp)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);

    pThis->fAbort = true;

    return VINF_SUCCESS;
}


RTR3DECL(int) RTHttpGetRedirLocation(RTHTTP hHttp, char **ppszRedirLocation)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);
    Assert(!pThis->fBusy);

    if (!pThis->pszRedirLocation)
        return VERR_HTTP_NOT_FOUND;

    return RTStrDupEx(ppszRedirLocation, pThis->pszRedirLocation);
}


RTR3DECL(int) RTHttpUseSystemProxySettings(RTHTTP hHttp)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);
    AssertReturn(!pThis->fBusy, VERR_WRONG_ORDER);

    /*
     * Change the settings.
     */
    pThis->fUseSystemProxySettings = true;
    return VINF_SUCCESS;
}


/**
 * rtHttpConfigureProxyForUrl: Update cURL proxy settings as needed.
 *
 * @returns IPRT status code.
 * @param   pThis           The HTTP client instance.
 * @param   enmProxyType    The proxy type.
 * @param   pszHost         The proxy host name.
 * @param   uPort           The proxy port number.
 * @param   pszUsername     The proxy username, or NULL if none.
 * @param   pszPassword     The proxy password, or NULL if none.
 */
static int rtHttpUpdateProxyConfig(PRTHTTPINTERNAL pThis, curl_proxytype enmProxyType, const char *pszHost,
                                   uint32_t uPort, const char *pszUsername, const char *pszPassword)
{
    int rcCurl;
    AssertReturn(pszHost, VERR_INVALID_PARAMETER);
    Log(("rtHttpUpdateProxyConfig: pThis=%p type=%d host='%s' port=%u user='%s'%s\n",
         pThis, enmProxyType, pszHost, uPort, pszUsername, pszPassword ? " with password" : " without password"));

#ifdef CURLOPT_NOPROXY
    if (pThis->fNoProxy)
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_NOPROXY, (const char *)NULL);
        AssertMsgReturn(rcCurl == CURLE_OK, ("CURLOPT_NOPROXY=NULL: %d (%#x)\n", rcCurl, rcCurl),
                        VERR_HTTP_CURL_PROXY_CONFIG);
        pThis->fNoProxy = false;
    }
#endif

    if (enmProxyType != pThis->enmProxyType)
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYTYPE, (long)enmProxyType);
        AssertMsgReturn(rcCurl == CURLE_OK, ("CURLOPT_PROXYTYPE=%d: %d (%#x)\n", enmProxyType, rcCurl, rcCurl),
                        VERR_HTTP_CURL_PROXY_CONFIG);
        pThis->enmProxyType = CURLPROXY_HTTP;
    }

    if (uPort != pThis->uProxyPort)
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYPORT, (long)uPort);
        AssertMsgReturn(rcCurl == CURLE_OK, ("CURLOPT_PROXYPORT=%d: %d (%#x)\n", uPort, rcCurl, rcCurl),
                        VERR_HTTP_CURL_PROXY_CONFIG);
        pThis->uProxyPort = uPort;
    }

    if (   pszUsername != pThis->pszProxyUsername
        || RTStrCmp(pszUsername, pThis->pszProxyUsername))
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYUSERNAME, pszUsername);
        AssertMsgReturn(rcCurl == CURLE_OK, ("CURLOPT_PROXYUSERNAME=%s: %d (%#x)\n", pszUsername, rcCurl, rcCurl),
                        VERR_HTTP_CURL_PROXY_CONFIG);
        if (pThis->pszProxyUsername)
        {
            RTStrFree(pThis->pszProxyUsername);
            pThis->pszProxyUsername = NULL;
        }
        if (pszUsername)
        {
            pThis->pszProxyUsername = RTStrDup(pszUsername);
            AssertReturn(pThis->pszProxyUsername, VERR_NO_STR_MEMORY);
        }
    }

    if (   pszPassword != pThis->pszProxyPassword
        || RTStrCmp(pszPassword, pThis->pszProxyPassword))
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYPASSWORD, pszPassword);
        AssertMsgReturn(rcCurl == CURLE_OK, ("CURLOPT_PROXYPASSWORD=%s: %d (%#x)\n", pszPassword ? "xxx" : NULL, rcCurl, rcCurl),
                        VERR_HTTP_CURL_PROXY_CONFIG);
        if (pThis->pszProxyPassword)
        {
            RTMemWipeThoroughly(pThis->pszProxyPassword, strlen(pThis->pszProxyPassword), 2);
            RTStrFree(pThis->pszProxyPassword);
            pThis->pszProxyPassword = NULL;
        }
        if (pszPassword)
        {
            pThis->pszProxyPassword = RTStrDup(pszPassword);
            AssertReturn(pThis->pszProxyPassword, VERR_NO_STR_MEMORY);
        }
    }

    if (   pszHost != pThis->pszProxyHost
        || RTStrCmp(pszHost, pThis->pszProxyHost))
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_PROXY, pszHost);
        AssertMsgReturn(rcCurl == CURLE_OK, ("CURLOPT_PROXY=%s: %d (%#x)\n", pszHost, rcCurl, rcCurl),
                        VERR_HTTP_CURL_PROXY_CONFIG);
        if (pThis->pszProxyHost)
        {
            RTStrFree(pThis->pszProxyHost);
            pThis->pszProxyHost = NULL;
        }
        if (pszHost)
        {
            pThis->pszProxyHost = RTStrDup(pszHost);
            AssertReturn(pThis->pszProxyHost, VERR_NO_STR_MEMORY);
        }
    }

    return VINF_SUCCESS;
}


/**
 * rtHttpConfigureProxyForUrl: Disables proxying.
 *
 * @returns IPRT status code.
 * @param   pThis               The HTTP client instance.
 */
static int rtHttpUpdateAutomaticProxyDisable(PRTHTTPINTERNAL pThis)
{
    Log(("rtHttpUpdateAutomaticProxyDisable: pThis=%p\n", pThis));

    AssertReturn(curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYTYPE,   (long)CURLPROXY_HTTP) == CURLE_OK, VERR_INTERNAL_ERROR_2);
    pThis->enmProxyType = CURLPROXY_HTTP;

    AssertReturn(curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYPORT,             (long)1080) == CURLE_OK, VERR_INTERNAL_ERROR_2);
    pThis->uProxyPort = 1080;

    AssertReturn(curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYUSERNAME, (const char *)NULL) == CURLE_OK, VERR_INTERNAL_ERROR_2);
    if (pThis->pszProxyUsername)
    {
        RTStrFree(pThis->pszProxyUsername);
        pThis->pszProxyUsername = NULL;
    }

    AssertReturn(curl_easy_setopt(pThis->pCurl, CURLOPT_PROXYPASSWORD, (const char *)NULL) == CURLE_OK, VERR_INTERNAL_ERROR_2);
    if (pThis->pszProxyPassword)
    {
        RTStrFree(pThis->pszProxyPassword);
        pThis->pszProxyPassword = NULL;
    }

    AssertReturn(curl_easy_setopt(pThis->pCurl, CURLOPT_PROXY,         (const char *)NULL) == CURLE_OK, VERR_INTERNAL_ERROR_2);
    if (pThis->pszProxyHost)
    {
        RTStrFree(pThis->pszProxyHost);
        pThis->pszProxyHost = NULL;
    }

#ifdef CURLOPT_NOPROXY
    /* No proxy for everything! */
    AssertReturn(curl_easy_setopt(pThis->pCurl, CURLOPT_NOPROXY, "*") == CURLE_OK, CURLOPT_PROXY);
    pThis->fNoProxy = true;
#endif

    return VINF_SUCCESS;
}


/**
 * See if the host name of the URL is included in the stripped no_proxy list.
 *
 * The no_proxy list is a colon or space separated list of domain names for
 * which there should be no proxying.  Given "no_proxy=oracle.com" neither the
 * URL "http://www.oracle.com" nor "http://oracle.com" will not be proxied, but
 * "http://notoracle.com" will be.
 *
 * @returns true if the URL is in the no_proxy list, otherwise false.
 * @param   pszUrl          The URL.
 * @param   pszNoProxyList  The stripped no_proxy list.
 */
static bool rtHttpUrlInNoProxyList(const char *pszUrl, const char *pszNoProxyList)
{
    /*
     * Check for just '*', diabling proxying for everything.
     * (Caller stripped pszNoProxyList.)
     */
    if (*pszNoProxyList == '*' && pszNoProxyList[1] == '\0')
        return true;

    /*
     * Empty list? (Caller stripped it, remember).
     */
    if (!*pszNoProxyList)
        return false;

    /*
     * We now need to parse the URL and extract the host name.
     */
    RTURIPARSED Parsed;
    int rc = RTUriParse(pszUrl, &Parsed);
    AssertRCReturn(rc, false);
    char *pszHost = RTUriParsedAuthorityHost(pszUrl, &Parsed);
    if (!pszHost) /* Don't assert, in case of file:///xxx or similar blunder. */
        return false;

    bool fRet = false;
    size_t const cchHost = strlen(pszHost);
    if (cchHost)
    {
        /*
         * The list is comma or space separated, walk it and match host names.
         */
        while (*pszNoProxyList != '\0')
        {
            /* Strip leading slashes, commas and dots. */
            char ch;
            while (   (ch = *pszNoProxyList) == ','
                   || ch == '.'
                   || RT_C_IS_SPACE(ch))
                pszNoProxyList++;

            /* Find the end. */
            size_t cch     = RTStrOffCharOrTerm(pszNoProxyList, ',');
            size_t offNext = RTStrOffCharOrTerm(pszNoProxyList, ' ');
            cch = RT_MIN(cch, offNext);
            offNext = cch;

            /* Trip trailing spaces, well tabs and stuff. */
            while (cch > 0 && RT_C_IS_SPACE(pszNoProxyList[cch - 1]))
                cch--;

            /* Do the matching, if we have anything to work with. */
            if (cch > 0)
            {
                if (   (   cch == cchHost
                        && RTStrNICmp(pszNoProxyList, pszHost, cch) == 0)
                    || (   cch <  cchHost
                        && pszHost[cchHost - cch - 1] == '.'
                        && RTStrNICmp(pszNoProxyList, &pszHost[cchHost - cch], cch) == 0) )
                {
                    fRet = true;
                    break;
                }
            }

            /* Next. */
            pszNoProxyList += offNext;
        }
    }

    RTStrFree(pszHost);
    return fRet;
}


/**
 * Configures a proxy given a "URL" like specification.
 *
 * The format is:
 * @verbatim
 *      [<scheme>"://"][<userid>[@<password>]:]<server>[":"<port>]
 * @endverbatim
 *
 * Where the scheme gives the type of proxy server we're dealing with rather
 * than the protocol of the external server we wish to talk to.
 *
 * @returns IPRT status code.
 * @param   pThis               The HTTP client instance.
 * @param   pszProxyUrl         The proxy server "URL".
 */
static int rtHttpConfigureProxyFromUrl(PRTHTTPINTERNAL pThis, const char *pszProxyUrl)
{
    /*
     * Make sure it can be parsed as an URL.
     */
    char *pszFreeMe = NULL;
    if (!strstr(pszProxyUrl, "://"))
    {
        static const char s_szPrefix[] = "http://";
        size_t cchProxyUrl = strlen(pszProxyUrl);
        pszFreeMe = (char *)RTMemTmpAlloc(sizeof(s_szPrefix) + cchProxyUrl);
        if (pszFreeMe)
        {
            memcpy(pszFreeMe, s_szPrefix, sizeof(s_szPrefix) - 1);
            memcpy(&pszFreeMe[sizeof(s_szPrefix) - 1], pszProxyUrl, cchProxyUrl);
            pszFreeMe[sizeof(s_szPrefix) - 1 + cchProxyUrl] = '\0';
            pszProxyUrl = pszFreeMe;
        }
        else
            return VERR_NO_TMP_MEMORY;
    }

    RTURIPARSED Parsed;
    int rc = RTUriParse(pszProxyUrl, &Parsed);
    if (RT_SUCCESS(rc))
    {
        char *pszHost = RTUriParsedAuthorityHost(pszProxyUrl, &Parsed);
        if (pszHost)
        {
            /*
             * We've got a host name, try get the rest.
             */
            char    *pszUsername = RTUriParsedAuthorityUsername(pszProxyUrl, &Parsed);
            char    *pszPassword = RTUriParsedAuthorityPassword(pszProxyUrl, &Parsed);
            uint32_t uProxyPort  = RTUriParsedAuthorityPort(pszProxyUrl, &Parsed);
            curl_proxytype enmProxyType;
            if (RTUriIsSchemeMatch(pszProxyUrl, "http"))
            {
                enmProxyType  = CURLPROXY_HTTP;
                if (uProxyPort == UINT32_MAX)
                    uProxyPort = 80;
            }
            else if (   RTUriIsSchemeMatch(pszProxyUrl, "socks4")
                     || RTUriIsSchemeMatch(pszProxyUrl, "socks"))
                enmProxyType = CURLPROXY_SOCKS4;
            else if (RTUriIsSchemeMatch(pszProxyUrl, "socks4a"))
                enmProxyType = CURLPROXY_SOCKS4A;
            else if (RTUriIsSchemeMatch(pszProxyUrl, "socks5"))
                enmProxyType = CURLPROXY_SOCKS5;
            else if (RTUriIsSchemeMatch(pszProxyUrl, "socks5h"))
                enmProxyType = CURLPROXY_SOCKS5_HOSTNAME;
            else
            {
                enmProxyType = CURLPROXY_HTTP;
                if (uProxyPort == UINT32_MAX)
                    uProxyPort = 8080;
            }

            /* Guess the port from the proxy type if not given. */
            if (uProxyPort == UINT32_MAX)
                uProxyPort = 1080; /* CURL_DEFAULT_PROXY_PORT */

            rc = rtHttpUpdateProxyConfig(pThis, enmProxyType, pszHost, uProxyPort, pszUsername, pszPassword);

            RTStrFree(pszUsername);
            RTStrFree(pszPassword);
            RTStrFree(pszHost);
        }
        else
            AssertMsgFailed(("RTUriParsedAuthorityHost('%s',) -> NULL\n", pszProxyUrl));
    }
    else
        AssertMsgFailed(("RTUriParse('%s',) -> %Rrc\n", pszProxyUrl, rc));

    if (pszFreeMe)
        RTMemTmpFree(pszFreeMe);
    return rc;
}


/**
 * Consults enviornment variables that cURL/lynx/wget/lynx uses for figuring out
 * the proxy config.
 *
 * @returns IPRT status code.
 * @param   pThis               The HTTP client instance.
 * @param   pszUrl              The URL to configure a proxy for.
 */
static int rtHttpConfigureProxyForUrlFromEnv(PRTHTTPINTERNAL pThis, const char *pszUrl)
{
    char szTmp[_1K];

    /*
     * First we consult the "no_proxy" / "NO_PROXY" environment variable.
     */
    const char *pszNoProxyVar;
    size_t cchActual;
    char  *pszNoProxyFree = NULL;
    char  *pszNoProxy = szTmp;
    int rc = RTEnvGetEx(RTENV_DEFAULT, pszNoProxyVar = "no_proxy", szTmp, sizeof(szTmp), &cchActual);
    if (rc == VERR_ENV_VAR_NOT_FOUND)
        rc = RTEnvGetEx(RTENV_DEFAULT, pszNoProxyVar = "NO_PROXY", szTmp, sizeof(szTmp), &cchActual);
    if (rc == VERR_BUFFER_OVERFLOW)
    {
        pszNoProxyFree = pszNoProxy = (char *)RTMemTmpAlloc(cchActual + _1K);
        AssertReturn(pszNoProxy, VERR_NO_TMP_MEMORY);
        rc = RTEnvGetEx(RTENV_DEFAULT, pszNoProxyVar, pszNoProxy, cchActual + _1K, NULL);
    }
    AssertMsg(rc == VINF_SUCCESS || rc == VERR_ENV_VAR_NOT_FOUND, ("rc=%Rrc\n", rc));
    bool fNoProxy = false;
    if (RT_SUCCESS(rc))
        fNoProxy = rtHttpUrlInNoProxyList(pszUrl, RTStrStrip(pszNoProxy));
    RTMemTmpFree(pszNoProxyFree);
    if (!fNoProxy)
    {
        /*
         * Get the schema specific specific env var, falling back on the
         * generic all_proxy if not found.
         */
        const char *apszEnvVars[4];
        unsigned    cEnvVars = 0;
        if (!RTStrNICmp(pszUrl, RT_STR_TUPLE("http:")))
            apszEnvVars[cEnvVars++] = "http_proxy"; /* Skip HTTP_PROXY because of cgi paranoia */
        else if (!RTStrNICmp(pszUrl, RT_STR_TUPLE("https:")))
        {
            apszEnvVars[cEnvVars++] = "https_proxy";
            apszEnvVars[cEnvVars++] = "HTTPS_PROXY";
        }
        else if (!RTStrNICmp(pszUrl, RT_STR_TUPLE("ftp:")))
        {
            apszEnvVars[cEnvVars++] = "ftp_proxy";
            apszEnvVars[cEnvVars++] = "FTP_PROXY";
        }
        else
            AssertMsgFailedReturn(("Unknown/unsupported schema in URL: '%s'\n", pszUrl), VERR_NOT_SUPPORTED);
        apszEnvVars[cEnvVars++] = "all_proxy";
        apszEnvVars[cEnvVars++] = "ALL_PROXY";

        /*
         * We try the env vars out and goes with the first one we can make sense out of.
         * If we cannot make sense of any, we return the first unexpected rc we got.
         */
        rc = VINF_SUCCESS;
        for (uint32_t i = 0; i < cEnvVars; i++)
        {
            size_t cchValue;
            int rc2 = RTEnvGetEx(RTENV_DEFAULT, apszEnvVars[i], szTmp, sizeof(szTmp) - sizeof("http://"), &cchValue);
            if (RT_SUCCESS(rc2))
            {
                if (cchValue != 0)
                {
                    /* Add a http:// prefix so RTUriParse groks it (cheaper to do it here). */
                    if (!strstr(szTmp, "://"))
                    {
                        memmove(&szTmp[sizeof("http://") - 1], szTmp, cchValue + 1);
                        memcpy(szTmp, RT_STR_TUPLE("http://"));
                    }

                    rc2 = rtHttpConfigureProxyFromUrl(pThis, szTmp);
                    if (RT_SUCCESS(rc2))
                        rc = rc2;
                }
                /*
                 * The variable is empty.  Guess that means no proxying wanted.
                 */
                else
                {
                    rc = rtHttpUpdateAutomaticProxyDisable(pThis);
                    break;
                }
            }
            else
                AssertMsgStmt(rc2 == VERR_ENV_VAR_NOT_FOUND, ("%Rrc\n", rc2), if (RT_SUCCESS(rc)) rc = rc2);
        }
    }
    /*
     * The host is the no-proxy list, it seems.
     */
    else
        rc = rtHttpUpdateAutomaticProxyDisable(pThis);

    return rc;
}

#ifdef IPRT_USE_LIBPROXY

/**
 * @callback_method_impl{FNRTONCE,
 *      Attempts to load libproxy.so.1 and resolves APIs}
 */
static DECLCALLBACK(int) rtHttpLibProxyResolveImports(void *pvUser)
{
    RTLDRMOD hMod;
    int rc = RTLdrLoad("/usr/lib/libproxy.so.1", &hMod);
    if (RT_SUCCESS(rc))
    {
        rc = RTLdrGetSymbol(hMod, "px_proxy_factory_new", (void **)&g_pfnLibProxyFactoryCtor);
        if (RT_SUCCESS(rc))
            rc = RTLdrGetSymbol(hMod, "px_proxy_factory_free", (void **)&g_pfnLibProxyFactoryDtor);
        if (RT_SUCCESS(rc))
            rc = RTLdrGetSymbol(hMod, "px_proxy_factory_get_proxies", (void **)&g_pfnLibProxyFactoryGetProxies);
        if (RT_SUCCESS(rc))
            g_hLdrLibProxy = hMod;
        else
            RTLdrClose(hMod);
        AssertRC(rc);
    }

    NOREF(pvUser);
    return rc;
}

/**
 * Reconfigures the cURL proxy settings for the given URL, libproxy style.
 *
 * @returns IPRT status code. VINF_NOT_SUPPORTED if we should try fallback.
 * @param   pThis       The HTTP client instance.
 * @param   pszUrl      The URL.
 */
static int rtHttpLibProxyConfigureProxyForUrl(PRTHTTPINTERNAL pThis, const char *pszUrl)
{
    int rcRet = VINF_NOT_SUPPORTED;

    int rc = RTOnce(&g_LibProxyResolveImportsOnce, rtHttpLibProxyResolveImports, NULL);
    if (RT_SUCCESS(rc))
    {
        /*
         * Instance the factory and ask for a list of proxies.
         */
        PLIBPROXYFACTORY pFactory = g_pfnLibProxyFactoryCtor();
        if (pFactory)
        {
            char **papszProxies = g_pfnLibProxyFactoryGetProxies(pFactory, pszUrl);
            g_pfnLibProxyFactoryDtor(pFactory);
            if (papszProxies)
            {
                /*
                 * Look for something we can use.
                 */
                for (unsigned i = 0; papszProxies[i]; i++)
                {
                    if (strncmp(papszProxies[i], RT_STR_TUPLE("direct://")) == 0)
                        rcRet = rtHttpUpdateAutomaticProxyDisable(pThis);
                    else if (   strncmp(papszProxies[i], RT_STR_TUPLE("http://")) == 0
                             || strncmp(papszProxies[i], RT_STR_TUPLE("socks5://")) == 0
                             || strncmp(papszProxies[i], RT_STR_TUPLE("socks4://")) == 0
                             || strncmp(papszProxies[i], RT_STR_TUPLE("socks://")) == 0 /** @todo same problem as on OS X. */
                            )
                        rcRet = rtHttpConfigureProxyFromUrl(pThis, papszProxies[i]);
                    else
                        continue;
                    if (rcRet != VINF_NOT_SUPPORTED)
                        break;
                }

                /* free the result. */
                for (unsigned i = 0; papszProxies[i]; i++)
                    free(papszProxies[i]);
                free(papszProxies);
            }
        }
    }

    return rcRet;
}

#endif /* IPRT_USE_LIBPROXY */

#ifdef RT_OS_DARWIN

/**
 * Get a boolean like integer value from a dictionary.
 *
 * @returns true / false.
 * @param   hDict       The dictionary.
 * @param   pvKey       The dictionary value key.
 */
static bool rtHttpDarwinGetBooleanFromDict(CFDictionaryRef hDict, void const *pvKey, bool fDefault)
{
    CFNumberRef hNum = (CFNumberRef)CFDictionaryGetValue(hDict, pvKey);
    if (hNum)
    {
        int fEnabled;
        if (!CFNumberGetValue(hNum, kCFNumberIntType, &fEnabled))
            return fDefault;
        return fEnabled != 0;
    }
    return fDefault;
}


/**
 * Creates a CFURL object for an URL.
 *
 * @returns CFURL object reference.
 * @param   pszUrl              The URL.
 */
static CFURLRef rtHttpDarwinUrlToCFURL(const char *pszUrl)
{
    CFURLRef    hUrl = NULL;
    CFStringRef hStrUrl = CFStringCreateWithCString(kCFAllocatorDefault, pszUrl, kCFStringEncodingUTF8);
    if (hStrUrl)
    {
        CFStringRef hStrUrlEscaped = CFURLCreateStringByAddingPercentEscapes(kCFAllocatorDefault, hStrUrl,
                                                                             NULL /*charactersToLeaveUnescaped*/,
                                                                             NULL /*legalURLCharactersToBeEscaped*/,
                                                                             kCFStringEncodingUTF8);
        if (hStrUrlEscaped)
        {
            hUrl = CFURLCreateWithString(kCFAllocatorDefault, hStrUrlEscaped, NULL /*baseURL*/);
            Assert(hUrl);
            CFRelease(hStrUrlEscaped);
        }
        else
            AssertFailed();
        CFRelease(hStrUrl);
    }
    else
        AssertFailed();
    return hUrl;
}


/**
 * For passing results from rtHttpDarwinPacCallback to
 * rtHttpDarwinExecuteProxyAutoConfigurationUrl.
 */
typedef struct RTHTTPDARWINPACRESULT
{
    CFArrayRef  hArrayProxies;
    CFErrorRef  hError;
} RTHTTPDARWINPACRESULT;
typedef RTHTTPDARWINPACRESULT *PRTHTTPDARWINPACRESULT;

/**
 * Stupid callback for getting the result from
 * CFNetworkExecuteProxyAutoConfigurationURL.
 *
 * @param   pvUser          Pointer to a RTHTTPDARWINPACRESULT on the stack of
 *                          rtHttpDarwinExecuteProxyAutoConfigurationUrl.
 * @param   hArrayProxies   The result array.
 * @param   hError          Errors, if any.
 */
static void rtHttpDarwinPacCallback(void *pvUser, CFArrayRef hArrayProxies, CFErrorRef hError)
{
    PRTHTTPDARWINPACRESULT pResult = (PRTHTTPDARWINPACRESULT)pvUser;

    Assert(pResult->hArrayProxies == NULL);
    if (hArrayProxies)
        pResult->hArrayProxies  = (CFArrayRef)CFRetain(hArrayProxies);

    Assert(pResult->hError == NULL);
    if (hError)
        pResult->hError         = (CFErrorRef)CFRetain(hError);

    CFRunLoopStop(CFRunLoopGetCurrent());
}


/**
 * Executes a PAC script and returning the proxies it suggests.
 *
 * @returns Array of proxy configs (CFProxySupport.h style).
 * @param   hUrlTarget      The URL we're about to use.
 * @param   hUrlScript      The PAC script URL.
 */
static CFArrayRef rtHttpDarwinExecuteProxyAutoConfigurationUrl(CFURLRef hUrlTarget, CFURLRef hUrlScript)
{
    char szTmp[256];
    if (LogIsFlowEnabled())
    {
        szTmp[0] = '\0';
        CFStringGetCString(CFURLGetString(hUrlScript), szTmp, sizeof(szTmp), kCFStringEncodingUTF8);
        LogFlow(("rtHttpDarwinExecuteProxyAutoConfigurationUrl: hUrlScript=%p:%s\n", hUrlScript, szTmp));
    }

    /*
     * Use CFNetworkExecuteProxyAutoConfigurationURL here so we don't have to
     * download the script ourselves and mess around with too many CF APIs.
     */
    CFRunLoopRef hRunLoop = CFRunLoopGetCurrent();
    AssertReturn(hRunLoop, NULL);

    RTHTTPDARWINPACRESULT Result = { NULL, NULL };
    CFStreamClientContext Ctx    = { 0, &Result, NULL, NULL, NULL };
    CFRunLoopSourceRef hRunLoopSrc = CFNetworkExecuteProxyAutoConfigurationURL(hUrlScript, hUrlTarget,
                                                                               rtHttpDarwinPacCallback, &Ctx);
    AssertReturn(hRunLoopSrc, NULL);

    CFStringRef kMode = CFSTR("com.apple.dts.CFProxySupportTool");
    CFRunLoopAddSource(hRunLoop, hRunLoopSrc, kMode);
    CFRunLoopRunInMode(kMode, 1.0e10, false); /* callback will force a return. */
    CFRunLoopRemoveSource(hRunLoop, hRunLoopSrc, kMode);

    /** @todo convert errors, maybe even fail. */

    /*
     * Autoconfig (or missing wpad server) typically results in:
     *      domain:kCFErrorDomainCFNetwork; code=kCFHostErrorUnknown (2).
     *
     * In the autoconfig case, it looks like we're getting two entries, first
     * one that's http://wpad/wpad.dat and a noproxy entry.  So, no reason to
     * be very upset if this fails, just continue trying alternatives.
     */
    if (Result.hError)
    {
        if (LogIsEnabled())
        {
            szTmp[0] = '\0';
            CFStringGetCString(CFErrorCopyDescription(Result.hError), szTmp, sizeof(szTmp), kCFStringEncodingUTF8);
            Log(("rtHttpDarwinExecuteProxyAutoConfigurationUrl: error! code=%ld desc='%s'\n", (long)CFErrorGetCode(Result.hError), szTmp));
        }
        CFRelease(Result.hError);
    }
    return Result.hArrayProxies;
}


/**
 * Attempt to configure the proxy according to @a hDictProxy.
 *
 * @returns IPRT status code. VINF_NOT_SUPPORTED if not able to configure it and
 *          the caller should try out alternative proxy configs and fallbacks.
 * @param   pThis           The HTTP client instance.
 * @param   hDictProxy      The proxy configuration (see CFProxySupport.h).
 * @param   hUrlTarget      The URL we're about to use.
 * @param   fIgnorePacType  Whether to ignore PAC type proxy entries (i.e.
 *                          javascript URL).  This is set when we're processing
 *                          the output from a PAC script.
 */
static int rtHttpDarwinTryConfigProxy(PRTHTTPINTERNAL pThis, CFDictionaryRef hDictProxy, CFURLRef hUrlTarget, bool fIgnorePacType)
{
    CFStringRef hStrProxyType = (CFStringRef)CFDictionaryGetValue(hDictProxy, kCFProxyTypeKey);
    AssertReturn(hStrProxyType, VINF_NOT_SUPPORTED);

    /*
     * No proxy is fairly simple and common.
     */
    if (CFEqual(hStrProxyType, kCFProxyTypeNone))
        return rtHttpUpdateAutomaticProxyDisable(pThis);

    /*
     * PAC URL means recursion, however we only do one level.
     */
    if (CFEqual(hStrProxyType, kCFProxyTypeAutoConfigurationURL))
    {
        AssertReturn(!fIgnorePacType, VINF_NOT_SUPPORTED);

        CFURLRef hUrlScript = (CFURLRef)CFDictionaryGetValue(hDictProxy, kCFProxyAutoConfigurationURLKey);
        AssertReturn(hUrlScript, VINF_NOT_SUPPORTED);

        int rcRet = VINF_NOT_SUPPORTED;
        CFArrayRef hArray = rtHttpDarwinExecuteProxyAutoConfigurationUrl(hUrlTarget, hUrlScript);
        if (hArray)
        {
            rcRet = rtHttpDarwinTryConfigProxies(pThis, hArray, hUrlTarget, true /*fIgnorePacType*/);
            CFRelease(hArray);
        }
        return rcRet;
    }

    /*
     * Determine the proxy type (not entirely sure about type == proxy type and
     * not scheme/protocol)...
     */
    curl_proxytype  enmProxyType      = CURLPROXY_HTTP;
    uint32_t        uDefaultProxyPort = 8080;
    if (   CFEqual(hStrProxyType, kCFProxyTypeHTTP)
        || CFEqual(hStrProxyType, kCFProxyTypeHTTPS))
    {  /* defaults */ }
    else if (CFEqual(hStrProxyType, kCFProxyTypeSOCKS))
    {
        /** @todo All we get from darwin is 'SOCKS', no idea whether it's SOCK4 or
         *        SOCK5 on the other side... Selecting SOCKS5 for now. */
        enmProxyType = CURLPROXY_SOCKS5;
        uDefaultProxyPort = 1080;
    }
    /* Unknown proxy type. */
    else
        return VINF_NOT_SUPPORTED;

    /*
     * Extract the proxy configuration.
     */
    /* The proxy host name. */
    char szHostname[_1K];
    CFStringRef hStr = (CFStringRef)CFDictionaryGetValue(hDictProxy, kCFProxyHostNameKey);
    AssertReturn(hStr, VINF_NOT_SUPPORTED);
    AssertReturn(CFStringGetCString(hStr, szHostname, sizeof(szHostname), kCFStringEncodingUTF8), VINF_NOT_SUPPORTED);

    /* Get the port number (optional). */
    SInt32      iProxyPort;
    CFNumberRef hNum = (CFNumberRef)CFDictionaryGetValue(hDictProxy, kCFProxyPortNumberKey);
    if (hNum && CFNumberGetValue(hNum, kCFNumberSInt32Type, &iProxyPort))
        AssertMsgStmt(iProxyPort > 0 && iProxyPort < _64K, ("%d\n", iProxyPort), iProxyPort = uDefaultProxyPort);
    else
        iProxyPort = uDefaultProxyPort;

    /* The proxy username. */
    char szUsername[256];
    hStr = (CFStringRef)CFDictionaryGetValue(hDictProxy, kCFProxyUsernameKey);
    if (hStr)
        AssertReturn(CFStringGetCString(hStr, szUsername, sizeof(szUsername), kCFStringEncodingUTF8), VINF_NOT_SUPPORTED);
    else
        szUsername[0] = '\0';

    /* The proxy password. */
    char szPassword[384];
    hStr = (CFStringRef)CFDictionaryGetValue(hDictProxy, kCFProxyPasswordKey);
    if (hStr)
        AssertReturn(CFStringGetCString(hStr, szPassword, sizeof(szPassword), kCFStringEncodingUTF8), VINF_NOT_SUPPORTED);
    else
        szPassword[0] = '\0';

    /*
     * Apply the proxy config.
     */
    return rtHttpUpdateProxyConfig(pThis, enmProxyType, szHostname, iProxyPort,
                                   szUsername[0] ? szUsername : NULL, szPassword[0] ? szPassword : NULL);
}


/**
 * Try do proxy config for our HTTP client instance given an array of proxies.
 *
 * This is used with the output from a CFProxySupport.h API.
 *
 * @returns IPRT status code. VINF_NOT_SUPPORTED if not able to configure it and
 *          we might want to try out fallbacks.
 * @param   pThis           The HTTP client instance.
 * @param   hArrayProxies   The proxies CFPRoxySupport have given us.
 * @param   hUrlTarget      The URL we're about to use.
 * @param   fIgnorePacType  Whether to ignore PAC type proxy entries (i.e.
 *                          javascript URL).  This is set when we're processing
 *                          the output from a PAC script.
 */
static int rtHttpDarwinTryConfigProxies(PRTHTTPINTERNAL pThis, CFArrayRef hArrayProxies, CFURLRef hUrlTarget, bool fIgnorePacType)
{
    int rcRet = VINF_NOT_SUPPORTED;
    CFIndex const cEntries = CFArrayGetCount(hArrayProxies);
    LogFlow(("rtHttpDarwinTryConfigProxies: cEntries=%d\n", cEntries));
    for (CFIndex i = 0; i < cEntries; i++)
    {
        CFDictionaryRef hDictProxy = (CFDictionaryRef)CFArrayGetValueAtIndex(hArrayProxies, i);
        AssertContinue(hDictProxy);

        rcRet = rtHttpDarwinTryConfigProxy(pThis, hDictProxy, hUrlTarget, fIgnorePacType);
        if (rcRet != VINF_NOT_SUPPORTED)
            break;
    }
    return rcRet;
}


/**
 * Inner worker for rtHttpWinConfigureProxyForUrl.
 *
 * @returns IPRT status code. VINF_NOT_SUPPORTED if we should try fallback.
 * @param   pThis       The HTTP client instance.
 * @param   pszUrl      The URL.
 */
static int rtHttpDarwinConfigureProxyForUrlWorker(PRTHTTPINTERNAL pThis, CFDictionaryRef hDictProxies,
                                                  const char *pszUrl, const char *pszHost)
{
    CFArrayRef  hArray;

    /*
     * From what I can tell, the CFNetworkCopyProxiesForURL API doesn't apply
     * proxy exclusion rules (tested on 10.9).  So, do that manually.
     */
    RTNETADDRU  HostAddr;
    int         fIsHostIpv4Address = -1;
    char        szTmp[_4K];

    /* If we've got a simple hostname, something containing no dots, we must check
       whether such simple hostnames are excluded from proxying by default or not. */
    if (strchr(pszHost, '.') == NULL)
    {
        if (rtHttpDarwinGetBooleanFromDict(hDictProxies, kSCPropNetProxiesExcludeSimpleHostnames, false))
            return rtHttpUpdateAutomaticProxyDisable(pThis);
        fIsHostIpv4Address = false;
    }

    /* Consult the exclusion list.  This is an array of strings.
       This is very similar to what we do on windows. */
    hArray = (CFArrayRef)CFDictionaryGetValue(hDictProxies, kSCPropNetProxiesExceptionsList);
    if (hArray)
    {
        CFIndex const cEntries = CFArrayGetCount(hArray);
        for (CFIndex i = 0; i < cEntries; i++)
        {
            CFStringRef hStr = (CFStringRef)CFArrayGetValueAtIndex(hArray, i);
            AssertContinue(hStr);
            AssertContinue(CFStringGetCString(hStr, szTmp, sizeof(szTmp), kCFStringEncodingUTF8));
            RTStrToLower(szTmp);

            bool fRet;
            if (   strchr(szTmp, '*')
                || strchr(szTmp, '?'))
                fRet = RTStrSimplePatternMatch(szTmp, pszHost);
            else
            {
                if (fIsHostIpv4Address == -1)
                    fIsHostIpv4Address = RT_SUCCESS(RTNetStrToIPv4Addr(pszHost, &HostAddr.IPv4));
                RTNETADDRIPV4 Network, Netmask;
                if (   fIsHostIpv4Address
                    && RT_SUCCESS(RTCidrStrToIPv4(szTmp, &Network, &Netmask)) )
                    fRet = (HostAddr.IPv4.u & Netmask.u) == Network.u;
                else
                    fRet = strcmp(szTmp, pszHost) == 0;
            }
            if (fRet)
                return rtHttpUpdateAutomaticProxyDisable(pThis);
        }
    }

#if 0 /* The start of a manual alternative to CFNetworkCopyProxiesForURL below, hopefully we won't need this. */
    /*
     * Is proxy auto config (PAC) enabled?  If so, we must consult it first.
     */
    if (rtHttpDarwinGetBooleanFromDict(hDictProxies, kSCPropNetProxiesProxyAutoConfigEnable, false))
    {
        /* Convert the auto config url string to a CFURL object. */
        CFStringRef hStrAutoConfigUrl = (CFStringRef)CFDictionaryGetValue(hDictProxies, kSCPropNetProxiesProxyAutoConfigURLString);
        if (hStrAutoConfigUrl)
        {
            if (CFStringGetCString(hStrAutoConfigUrl, szTmp, sizeof(szTmp), kCFStringEncodingUTF8))
            {
                CFURLRef hUrlScript = rtHttpDarwinUrlToCFURL(szTmp);
                if (hUrlScript)
                {
                    int      rcRet      = VINF_NOT_SUPPORTED;
                    CFURLRef hUrlTarget = rtHttpDarwinUrlToCFURL(pszUrl);
                    if (hUrlTarget)
                    {
                        /* Work around for <rdar://problem/5530166>, whatever that is.  Initializes
                           some internal CFNetwork state, they say.  See CFPRoxySupportTool example. */
                        hArray = CFNetworkCopyProxiesForURL(hUrlTarget, NULL);
                        if (hArray)
                            CFRelease(hArray);

                        hArray = rtHttpDarwinExecuteProxyAutoConfigurationUrl(hUrlTarget, hUrlScript);
                        if (hArray)
                        {
                            rcRet = rtHttpDarwinTryConfigProxies(pThis, hArray, hUrlTarget, true /*fIgnorePacType*/);
                            CFRelease(hArray);
                        }
                    }
                    CFRelease(hUrlScript);
                    if (rcRet != VINF_NOT_SUPPORTED)
                        return rcRet;
                }
            }
        }
    }

    /*
     * Try static proxy configs.
     */
    /** @todo later if needed. */
    return VERR_NOT_SUPPORTED;

#else
    /*
     * Simple solution - "just" use CFNetworkCopyProxiesForURL.
     */
    CFURLRef hUrlTarget = rtHttpDarwinUrlToCFURL(pszUrl);
    AssertReturn(hUrlTarget, VERR_INTERNAL_ERROR);
    int rcRet = VINF_NOT_SUPPORTED;

    /* Work around for <rdar://problem/5530166>, whatever that is.  Initializes
       some internal CFNetwork state, they say.  See CFPRoxySupportTool example. */
    hArray = CFNetworkCopyProxiesForURL(hUrlTarget, NULL);
    if (hArray)
        CFRelease(hArray);

    /* The actual run. */
    hArray = CFNetworkCopyProxiesForURL(hUrlTarget, hDictProxies);
    if (hArray)
    {
        rcRet = rtHttpDarwinTryConfigProxies(pThis, hArray, hUrlTarget, false /*fIgnorePacType*/);
        CFRelease(hArray);
    }
    CFRelease(hUrlTarget);

    return rcRet;
#endif
}

/**
 * Reconfigures the cURL proxy settings for the given URL, OS X style.
 *
 * @returns IPRT status code. VINF_NOT_SUPPORTED if we should try fallback.
 * @param   pThis       The HTTP client instance.
 * @param   pszUrl      The URL.
 */
static int rtHttpDarwinConfigureProxyForUrl(PRTHTTPINTERNAL pThis, const char *pszUrl)
{
    /*
     * Parse the URL, if there isn't any host name (like for file:///xxx.txt)
     * we don't need to run thru proxy settings to know what to do.
     */
    RTURIPARSED Parsed;
    int rc = RTUriParse(pszUrl, &Parsed);
    AssertRCReturn(rc, false);
    if (Parsed.cchAuthorityHost == 0)
        return rtHttpUpdateAutomaticProxyDisable(pThis);
    char *pszHost = RTUriParsedAuthorityHost(pszUrl, &Parsed);
    AssertReturn(pszHost, VERR_NO_STR_MEMORY);
    RTStrToLower(pszHost);

    /*
     * Get a copy of the proxy settings (10.6 API).
     */
    CFDictionaryRef hDictProxies = CFNetworkCopySystemProxySettings(); /* Alt for 10.5: SCDynamicStoreCopyProxies(NULL); */
    if (hDictProxies)
        rc = rtHttpDarwinConfigureProxyForUrlWorker(pThis, hDictProxies, pszUrl, pszHost);
    else
        rc = VINF_NOT_SUPPORTED;
    CFRelease(hDictProxies);

    RTStrFree(pszHost);
    return rc;
}

#endif /* RT_OS_DARWIN */

#ifdef RT_OS_WINDOWS

/**
 * @callback_method_impl{FNRTONCE, Loads WinHttp.dll and resolves APIs}
 */
static DECLCALLBACK(int) rtHttpWinResolveImports(void *pvUser)
{
    /*
     * winhttp.dll is not present on NT4 and probably was first introduced with XP.
     */
    RTLDRMOD hMod;
    int rc = RTLdrLoadSystem("winhttp.dll", true /*fNoUnload*/, &hMod);
    if (RT_SUCCESS(rc))
    {
        rc = RTLdrGetSymbol(hMod, "WinHttpOpen", (void **)&g_pfnWinHttpOpen);
        if (RT_SUCCESS(rc))
            rc = RTLdrGetSymbol(hMod, "WinHttpCloseHandle", (void **)&g_pfnWinHttpCloseHandle);
        if (RT_SUCCESS(rc))
            rc = RTLdrGetSymbol(hMod, "WinHttpGetProxyForUrl", (void **)&g_pfnWinHttpGetProxyForUrl);
        if (RT_SUCCESS(rc))
            rc = RTLdrGetSymbol(hMod, "WinHttpGetDefaultProxyConfiguration", (void **)&g_pfnWinHttpGetDefaultProxyConfiguration);
        if (RT_SUCCESS(rc))
            rc = RTLdrGetSymbol(hMod, "WinHttpGetIEProxyConfigForCurrentUser", (void **)&g_pfnWinHttpGetIEProxyConfigForCurrentUser);
        RTLdrClose(hMod);
        AssertRC(rc);
    }
    else
        AssertMsg(g_enmWinVer < kRTWinOSType_XP, ("%Rrc\n", rc));

    NOREF(pvUser);
    return rc;
}


/**
 * Matches the URL against the given Windows by-pass list.
 *
 * @returns true if we should by-pass the proxy for this URL, false if not.
 * @param   pszUrl              The URL.
 * @param   pwszBypass          The Windows by-pass list.
 */
static bool rtHttpWinIsUrlInBypassList(const char *pszUrl, PCRTUTF16 pwszBypass)
{
    /*
     * Don't bother parsing the URL if we've actually got nothing to work with
     * in the by-pass list.
     */
    if (!pwszBypass)
        return false;

    RTUTF16 wc;
    while (   (wc = *pwszBypass) != '\0'
           && (   RTUniCpIsSpace(wc)
               || wc == ';') )
        pwszBypass++;
    if (wc == '\0')
        return false;

    /*
     * We now need to parse the URL and extract the host name.
     */
    RTURIPARSED Parsed;
    int rc = RTUriParse(pszUrl, &Parsed);
    AssertRCReturn(rc, false);
    char *pszHost = RTUriParsedAuthorityHost(pszUrl, &Parsed);
    if (!pszHost) /* Don't assert, in case of file:///xxx or similar blunder. */
        return false;
    RTStrToLower(pszHost);

    bool  fRet = false;
    char *pszBypassFree;
    rc = RTUtf16ToUtf8(pwszBypass, &pszBypassFree);
    if (RT_SUCCESS(rc))
    {
        /*
         * Walk the by-pass list.
         *
         * According to https://msdn.microsoft.com/en-us/library/aa384098(v=vs.85).aspx
         * a by-pass list is semicolon delimited list.  The entries are either host
         * names or IP addresses, and may use wildcard ('*', '?', I guess).  There
         * special "<local>" entry matches anything without a dot.
         */
        RTNETADDRU  HostAddr = { 0, 0 };
        int         fIsHostIpv4Address = -1;
        char *pszEntry = pszBypassFree;
        while (*pszEntry != '\0')
        {
            /*
             * Find end of entry.
             */
            char   ch;
            size_t cchEntry = 1;
            while (   (ch = pszEntry[cchEntry]) != '\0'
                   && ch != ';'
                   && !RT_C_IS_SPACE(ch))
                cchEntry++;

            char chSaved = pszEntry[cchEntry];
            pszEntry[cchEntry] = '\0';
            RTStrToLower(pszEntry);

            if (   cchEntry == sizeof("<local>")  - 1
                && memcmp(pszEntry, RT_STR_TUPLE("<local>")) == 0)
                fRet = strchr(pszHost, '.') == NULL;
            else if (   memchr(pszEntry, '*', cchEntry) != NULL
                     || memchr(pszEntry, '?', cchEntry) != NULL)
                fRet = RTStrSimplePatternMatch(pszEntry, pszHost);
            else
            {
                if (fIsHostIpv4Address == -1)
                    fIsHostIpv4Address = RT_SUCCESS(RTNetStrToIPv4Addr(pszHost, &HostAddr.IPv4));
                RTNETADDRIPV4 Network, Netmask;
                if (   fIsHostIpv4Address
                    && RT_SUCCESS(RTCidrStrToIPv4(pszEntry, &Network, &Netmask)) )
                    fRet = (HostAddr.IPv4.u & Netmask.u) == Network.u;
                else
                    fRet = strcmp(pszEntry, pszHost) == 0;
            }

            pszEntry[cchEntry] = chSaved;
            if (fRet)
                break;

            /*
             * Next entry.
             */
            pszEntry += cchEntry;
            while (   (ch = *pszEntry) != '\0'
                   && (   ch == ';'
                       || RT_C_IS_SPACE(ch)) )
                pszEntry++;
        }

        RTStrFree(pszBypassFree);
    }

    RTStrFree(pszHost);
    return false;
}


/**
 * Searches a Windows proxy server list for the best fitting proxy to use, then
 * reconfigures the HTTP client instance to use it.
 *
 * @returns IPRT status code, VINF_NOT_SUPPORTED if we need to consult fallback.
 * @param   pThis               The HTTP client instance.
 * @param   pszUrl              The URL needing proxying.
 * @param   pwszProxies         The list of proxy servers to choose from.
 */
static int rtHttpWinSelectProxyFromList(PRTHTTPINTERNAL pThis, const char *pszUrl, PCRTUTF16 pwszProxies)
{
    /*
     * Fend off empty strings (very unlikely, but just in case).
     */
    if (!pwszProxies)
        return VINF_NOT_SUPPORTED;

    RTUTF16 wc;
    while (   (wc = *pwszProxies) != '\0'
           && (   RTUniCpIsSpace(wc)
               || wc == ';') )
        pwszProxies++;
    if (wc == '\0')
        return VINF_NOT_SUPPORTED;

    /*
     * We now need to parse the URL and extract the scheme.
     */
    RTURIPARSED Parsed;
    int rc = RTUriParse(pszUrl, &Parsed);
    AssertRCReturn(rc, false);
    char *pszUrlScheme = RTUriParsedScheme(pszUrl, &Parsed);
    AssertReturn(pszUrlScheme, VERR_NO_STR_MEMORY);
    size_t const cchUrlScheme = strlen(pszUrlScheme);

    int rcRet = VINF_NOT_SUPPORTED;
    char *pszProxiesFree;
    rc = RTUtf16ToUtf8(pwszProxies, &pszProxiesFree);
    if (RT_SUCCESS(rc))
    {
        /*
         * Walk the server list.
         *
         * According to https://msdn.microsoft.com/en-us/library/aa383912(v=vs.85).aspx
         * this is also a semicolon delimited list.  The entries are on the form:
         *      [<scheme>=][<scheme>"://"]<server>[":"<port>]
         */
        bool        fBestEntryHasSameScheme = false;
        const char *pszBestEntry = NULL;
        char       *pszEntry = pszProxiesFree;
        while (*pszEntry != '\0')
        {
            /*
             * Find end of entry.  We include spaces here in addition to ';'.
             */
            char   ch;
            size_t cchEntry = 1;
            while (   (ch = pszEntry[cchEntry]) != '\0'
                   && ch != ';'
                   && !RT_C_IS_SPACE(ch))
                cchEntry++;

            char const chSaved = pszEntry[cchEntry];
            pszEntry[cchEntry] = '\0';

            /* Parse the entry. */
            const char *pszEndOfScheme = strstr(pszEntry, "://");
            const char *pszEqual       = (const char *)memchr(pszEntry, '=',
                                                              pszEndOfScheme ? pszEndOfScheme - pszEntry : cchEntry);
            if (pszEqual)
            {
                if (   (uintptr_t)(pszEqual - pszEntry) == cchUrlScheme
                    && RTStrNICmp(pszEntry, pszUrlScheme, cchUrlScheme) == 0)
                {
                    pszBestEntry = pszEqual + 1;
                    break;
                }
            }
            else
            {
                bool fSchemeMatch = pszEndOfScheme
                                 && (uintptr_t)(pszEndOfScheme - pszEntry) == cchUrlScheme
                                 && RTStrNICmp(pszEntry, pszUrlScheme, cchUrlScheme) == 0;
                if (   !pszBestEntry
                    || (   !fBestEntryHasSameScheme
                        && fSchemeMatch) )
                {
                    pszBestEntry = pszEntry;
                    fBestEntryHasSameScheme = fSchemeMatch;
                }
            }

            /*
             * Next entry.
             */
            if (!chSaved)
                break;
            pszEntry += cchEntry + 1;
            while (   (ch = *pszEntry) != '\0'
                   && (   ch == ';'
                       || RT_C_IS_SPACE(ch)) )
                pszEntry++;
        }

        /*
         * If we found something, try use it.
         */
        if (pszBestEntry)
            rcRet = rtHttpConfigureProxyFromUrl(pThis, pszBestEntry);

        RTStrFree(pszProxiesFree);
    }

    RTStrFree(pszUrlScheme);
    return rc;
}


/**
 * Reconfigures the cURL proxy settings for the given URL, Windows style.
 *
 * @returns IPRT status code. VINF_NOT_SUPPORTED if we should try fallback.
 * @param   pThis       The HTTP client instance.
 * @param   pszUrl      The URL.
 */
static int rtHttpWinConfigureProxyForUrl(PRTHTTPINTERNAL pThis, const char *pszUrl)
{
    int rcRet = VINF_NOT_SUPPORTED;

    int rc = RTOnce(&g_WinResolveImportsOnce, rtHttpWinResolveImports, NULL);
    if (RT_SUCCESS(rc))
    {
        /*
         * Try get some proxy info for the URL.  We first try getting the IE
         * config and seeing if we can use WinHttpGetIEProxyConfigForCurrentUser
         * in some way, if we can we prepare ProxyOptions with a non-zero dwFlags.
         */
        WINHTTP_PROXY_INFO          ProxyInfo;
        WINHTTP_AUTOPROXY_OPTIONS   AutoProxyOptions;
        RT_ZERO(AutoProxyOptions);
        RT_ZERO(ProxyInfo);

        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG IeProxyConfig;
        if (g_pfnWinHttpGetIEProxyConfigForCurrentUser(&IeProxyConfig))
        {
            AutoProxyOptions.fAutoLogonIfChallenged = FALSE;
            AutoProxyOptions.lpszAutoConfigUrl      = IeProxyConfig.lpszAutoConfigUrl;
            if (IeProxyConfig.fAutoDetect)
            {
                AutoProxyOptions.dwFlags            = WINHTTP_AUTOPROXY_AUTO_DETECT | WINHTTP_AUTOPROXY_RUN_INPROCESS;
                AutoProxyOptions.dwAutoDetectFlags  = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            }
            else if (AutoProxyOptions.lpszAutoConfigUrl)
                AutoProxyOptions.dwFlags            = WINHTTP_AUTOPROXY_CONFIG_URL;
            else if (ProxyInfo.lpszProxy)
                ProxyInfo.dwAccessType              = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            ProxyInfo.lpszProxy       = IeProxyConfig.lpszProxy;
            ProxyInfo.lpszProxyBypass = IeProxyConfig.lpszProxyBypass;
        }
        else
        {
            AssertMsgFailed(("WinHttpGetIEProxyConfigForCurrentUser -> %u\n", GetLastError()));
            if (!g_pfnWinHttpGetDefaultProxyConfiguration(&ProxyInfo))
            {
                AssertMsgFailed(("WinHttpGetDefaultProxyConfiguration -> %u\n", GetLastError()));
                RT_ZERO(ProxyInfo);
            }
        }

        /*
         * Should we try WinHttGetProxyForUrl?
         */
        if (AutoProxyOptions.dwFlags != 0)
        {
            HINTERNET hSession = g_pfnWinHttpOpen(NULL /*pwszUserAgent*/, WINHTTP_ACCESS_TYPE_NO_PROXY,
                                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 /*dwFlags*/ );
            if (hSession != NULL)
            {
                PRTUTF16 pwszUrl;
                rc = RTStrToUtf16(pszUrl, &pwszUrl);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Try autodetect first, then fall back on the config URL if there is one.
                     *
                     * Also, we first try without auto authentication, then with.  This will according
                     * to http://msdn.microsoft.com/en-us/library/aa383153%28v=VS.85%29.aspx help with
                     * caching the result when it's processed out-of-process (seems default here on W10).
                     */
                    WINHTTP_PROXY_INFO TmpProxyInfo;
                    BOOL fRc = g_pfnWinHttpGetProxyForUrl(hSession, pwszUrl, &AutoProxyOptions, &TmpProxyInfo);
                    if (   !fRc
                        && GetLastError() == ERROR_WINHTTP_LOGIN_FAILURE)
                    {
                        AutoProxyOptions.fAutoLogonIfChallenged = TRUE;
                        fRc = g_pfnWinHttpGetProxyForUrl(hSession, pwszUrl, &AutoProxyOptions, &TmpProxyInfo);
                    }

                    if (   !fRc
                        && AutoProxyOptions.dwFlags != WINHTTP_AUTOPROXY_CONFIG_URL
                        && AutoProxyOptions.lpszAutoConfigUrl)
                    {
                        AutoProxyOptions.fAutoLogonIfChallenged = FALSE;
                        AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
                        AutoProxyOptions.dwAutoDetectFlags = 0;
                        fRc = g_pfnWinHttpGetProxyForUrl(hSession, pwszUrl, &AutoProxyOptions, &TmpProxyInfo);
                        if (   !fRc
                            && GetLastError() == ERROR_WINHTTP_LOGIN_FAILURE)
                        {
                            AutoProxyOptions.fAutoLogonIfChallenged = TRUE;
                            fRc = g_pfnWinHttpGetProxyForUrl(hSession, pwszUrl, &AutoProxyOptions, &TmpProxyInfo);
                        }
                    }

                    if (fRc)
                    {
                        if (ProxyInfo.lpszProxy)
                            GlobalFree(ProxyInfo.lpszProxy);
                        if (ProxyInfo.lpszProxyBypass)
                            GlobalFree(ProxyInfo.lpszProxyBypass);
                        ProxyInfo = TmpProxyInfo;
                    }
                    /*
                     * If the autodetection failed, assume no proxy.
                     */
                    else
                    {
                        DWORD dwErr = GetLastError();
                        if (   dwErr == ERROR_WINHTTP_AUTODETECTION_FAILED
                            || (   dwErr == ERROR_WINHTTP_UNRECOGNIZED_SCHEME
                                && (   RTStrNICmp(pszUrl, RT_STR_TUPLE("https://")) == 0
                                    || RTStrNICmp(pszUrl, RT_STR_TUPLE("http://")) == 0) ) )
                            rcRet = rtHttpUpdateAutomaticProxyDisable(pThis);
                        else
                            AssertMsgFailed(("g_pfnWinHttpGetProxyForUrl(%s) -> %u; lpszAutoConfigUrl=%sx\n",
                                             pszUrl, dwErr, AutoProxyOptions.lpszAutoConfigUrl));
                    }
                    RTUtf16Free(pwszUrl);
                }
                else
                {
                    AssertMsgFailed(("RTStrToUtf16(%s,) -> %Rrc\n", pszUrl, rc));
                    rcRet = rc;
                }
                 g_pfnWinHttpCloseHandle(hSession);
            }
            else
                AssertMsgFailed(("g_pfnWinHttpOpen -> %u\n", GetLastError()));
        }

        /*
         * Try use the proxy info we've found.
         */
        switch (ProxyInfo.dwAccessType)
        {
            case WINHTTP_ACCESS_TYPE_NO_PROXY:
                rcRet = rtHttpUpdateAutomaticProxyDisable(pThis);
                break;

            case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
                if (!rtHttpWinIsUrlInBypassList(pszUrl, ProxyInfo.lpszProxyBypass))
                    rcRet = rtHttpWinSelectProxyFromList(pThis, pszUrl, ProxyInfo.lpszProxy);
                else
                    rcRet = rtHttpUpdateAutomaticProxyDisable(pThis);
                break;

            case 0:
                break;

            default:
                AssertMsgFailed(("%#x\n", ProxyInfo.dwAccessType));
        }

        /*
         * Cleanup.
         */
        if (ProxyInfo.lpszProxy)
            GlobalFree(ProxyInfo.lpszProxy);
        if (ProxyInfo.lpszProxyBypass)
            GlobalFree(ProxyInfo.lpszProxyBypass);
        if (AutoProxyOptions.lpszAutoConfigUrl)
            GlobalFree((PRTUTF16)AutoProxyOptions.lpszAutoConfigUrl);
    }

    return rcRet;
}

#endif /* RT_OS_WINDOWS */


static int rtHttpConfigureProxyForUrl(PRTHTTPINTERNAL pThis, const char *pszUrl)
{
    if (pThis->fUseSystemProxySettings)
    {
#ifdef IPRT_USE_LIBPROXY
        int rc = rtHttpLibProxyConfigureProxyForUrl(pThis, pszUrl);
        if (rc == VINF_SUCCESS || RT_FAILURE(rc))
            return rc;
        Assert(rc == VINF_NOT_SUPPORTED);
#endif
#ifdef RT_OS_DARWIN
        int rc = rtHttpDarwinConfigureProxyForUrl(pThis, pszUrl);
        if (rc == VINF_SUCCESS || RT_FAILURE(rc))
            return rc;
        Assert(rc == VINF_NOT_SUPPORTED);
#endif
#ifdef RT_OS_WINDOWS
        int rc = rtHttpWinConfigureProxyForUrl(pThis, pszUrl);
        if (rc == VINF_SUCCESS || RT_FAILURE(rc))
            return rc;
        Assert(rc == VINF_NOT_SUPPORTED);
#endif
/** @todo system specific class here, fall back on env vars if necessary. */
        return rtHttpConfigureProxyForUrlFromEnv(pThis, pszUrl);
    }

    return VINF_SUCCESS;
}


RTR3DECL(int) RTHttpSetProxy(RTHTTP hHttp, const char *pcszProxy, uint32_t uPort,
                             const char *pcszProxyUser, const char *pcszProxyPwd)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);
    AssertPtrReturn(pcszProxy, VERR_INVALID_PARAMETER);
    AssertReturn(!pThis->fBusy, VERR_WRONG_ORDER);

    /*
     * Update the settings.
     *
     * Currently, we don't make alot of effort parsing or checking the input, we
     * leave that to cURL.  (A bit afraid of breaking user settings.)
     */
    pThis->fUseSystemProxySettings = false;
    return rtHttpUpdateProxyConfig(pThis, CURLPROXY_HTTP, pcszProxy, uPort ? uPort : 1080, pcszProxyUser, pcszProxyPwd);
}


RTR3DECL(int) RTHttpSetHeaders(RTHTTP hHttp, size_t cHeaders, const char * const *papszHeaders)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);

    pThis->fHaveUserAgentHeader = false;
    if (!cHeaders)
    {
        if (pThis->pHeaders)
            curl_slist_free_all(pThis->pHeaders);
        pThis->pHeaders = 0;
        return VINF_SUCCESS;
    }

    struct curl_slist *pHeaders = NULL;
    for (size_t i = 0; i < cHeaders; i++)
    {
        pHeaders = curl_slist_append(pHeaders, papszHeaders[i]);
        if (strncmp(papszHeaders[i], RT_STR_TUPLE("User-Agent:")) == 0)
            pThis->fHaveUserAgentHeader = true;
    }

    pThis->pHeaders = pHeaders;
    int rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_HTTPHEADER, pHeaders);
    if (CURL_FAILURE(rcCurl))
        return VERR_INVALID_PARAMETER;

    /*
     * Unset the user agent if it's in one of the headers.
     */
    if (   pThis->fHaveUserAgentHeader
        && pThis->fHaveSetUserAgent)
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_USERAGENT, (char *)NULL);
        Assert(CURL_SUCCESS(rcCurl));
        pThis->fHaveSetUserAgent = false;
    }

    return VINF_SUCCESS;
}


/**
 * Set the CA file to NULL, deleting any temporary file if necessary.
 *
 * @param   pThis           The HTTP/HTTPS client instance.
 */
static void rtHttpUnsetCaFile(PRTHTTPINTERNAL pThis)
{
    if (pThis->pszCaFile)
    {
        if (pThis->fDeleteCaFile)
        {
            int rc2 = RTFileDelete(pThis->pszCaFile); RT_NOREF_PV(rc2);
            AssertMsg(RT_SUCCESS(rc2) || !RTFileExists(pThis->pszCaFile), ("rc=%Rrc '%s'\n", rc2, pThis->pszCaFile));
        }
        RTStrFree(pThis->pszCaFile);
        pThis->pszCaFile = NULL;
    }
}


RTR3DECL(int) RTHttpSetCAFile(RTHTTP hHttp, const char *pszCaFile)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);

    rtHttpUnsetCaFile(pThis);

    pThis->fDeleteCaFile = false;
    if (pszCaFile)
        return RTStrDupEx(&pThis->pszCaFile, pszCaFile);
    return VINF_SUCCESS;
}


RTR3DECL(int) RTHttpUseTemporaryCaFile(RTHTTP hHttp, PRTERRINFO pErrInfo)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);

    /*
     * Create a temporary file.
     */
    int rc = VERR_NO_STR_MEMORY;
    char *pszCaFile = RTStrAlloc(RTPATH_MAX);
    if (pszCaFile)
    {
        RTFILE hFile;
        rc = RTFileOpenTemp(&hFile, pszCaFile, RTPATH_MAX,
                            RTFILE_O_CREATE | RTFILE_O_WRITE | RTFILE_O_DENY_NONE | (0600 << RTFILE_O_CREATE_MODE_SHIFT));
        if (RT_SUCCESS(rc))
        {
            /*
             * Gather certificates into a temporary store and export them to the temporary file.
             */
            RTCRSTORE hStore;
            rc = RTCrStoreCreateInMem(&hStore, 256);
            if (RT_SUCCESS(rc))
            {
                rc = RTHttpGatherCaCertsInStore(hStore, 0 /*fFlags*/, pErrInfo);
                if (RT_SUCCESS(rc))
                    /** @todo Consider adding an API for exporting to a RTFILE... */
                    rc = RTCrStoreCertExportAsPem(hStore, 0 /*fFlags*/, pszCaFile);
                RTCrStoreRelease(hStore);
            }
            RTFileClose(hFile);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Set the CA file for the instance.
                 */
                rtHttpUnsetCaFile(pThis);

                pThis->fDeleteCaFile = true;
                pThis->pszCaFile = pszCaFile;
                return VINF_SUCCESS;
            }

            int rc2 = RTFileDelete(pszCaFile);
            AssertRC(rc2);
        }
        else
            RTErrInfoAddF(pErrInfo, rc, "Error creating temorary file: %Rrc", rc);

        RTStrFree(pszCaFile);
    }
    return rc;
}


RTR3DECL(int) RTHttpGatherCaCertsInStore(RTCRSTORE hStore, uint32_t fFlags, PRTERRINFO pErrInfo)
{
    uint32_t const cBefore = RTCrStoreCertCount(hStore);
    AssertReturn(cBefore != UINT32_MAX, VERR_INVALID_HANDLE);
    RT_NOREF_PV(fFlags);


    /*
     * Add the user store, quitely ignoring any errors.
     */
    RTCRSTORE hSrcStore;
    int rcUser = RTCrStoreCreateSnapshotById(&hSrcStore, RTCRSTOREID_USER_TRUSTED_CAS_AND_CERTIFICATES, pErrInfo);
    if (RT_SUCCESS(rcUser))
    {
        rcUser = RTCrStoreCertAddFromStore(hStore, RTCRCERTCTX_F_ADD_IF_NOT_FOUND | RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR,
                                           hSrcStore);
        RTCrStoreRelease(hSrcStore);
    }

    /*
     * Ditto for the system store.
     */
    int rcSystem = RTCrStoreCreateSnapshotById(&hSrcStore, RTCRSTOREID_SYSTEM_TRUSTED_CAS_AND_CERTIFICATES, pErrInfo);
    if (RT_SUCCESS(rcSystem))
    {
        rcSystem = RTCrStoreCertAddFromStore(hStore, RTCRCERTCTX_F_ADD_IF_NOT_FOUND | RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR,
                                             hSrcStore);
        RTCrStoreRelease(hSrcStore);
    }

    /*
     * If the number of certificates increased, we consider it a success.
     */
    if (RTCrStoreCertCount(hStore) > cBefore)
    {
        if (RT_FAILURE(rcSystem))
            return -rcSystem;
        if (RT_FAILURE(rcUser))
            return -rcUser;
        return rcSystem != VINF_SUCCESS ? rcSystem : rcUser;
    }

    if (RT_FAILURE(rcSystem))
        return rcSystem;
    if (RT_FAILURE(rcUser))
        return rcUser;
    return VERR_NOT_FOUND;
}


RTR3DECL(int) RTHttpGatherCaCertsInFile(const char *pszCaFile, uint32_t fFlags, PRTERRINFO pErrInfo)
{
    RTCRSTORE hStore;
    int rc = RTCrStoreCreateInMem(&hStore, 256);
    if (RT_SUCCESS(rc))
    {
        rc = RTHttpGatherCaCertsInStore(hStore, fFlags, pErrInfo);
        if (RT_SUCCESS(rc))
            rc = RTCrStoreCertExportAsPem(hStore, 0 /*fFlags*/, pszCaFile);
        RTCrStoreRelease(hStore);
    }
    return rc;
}



/**
 * Figures out the IPRT status code for a GET.
 *
 * @returns IPRT status code.
 * @param   pThis           The HTTP/HTTPS client instance.
 * @param   rcCurl          What curl returned.
 */
static int rtHttpGetCalcStatus(PRTHTTPINTERNAL pThis, int rcCurl)
{
    int rc = VERR_HTTP_CURL_ERROR;

    if (pThis->pszRedirLocation)
    {
        RTStrFree(pThis->pszRedirLocation);
        pThis->pszRedirLocation = NULL;
    }
    if (rcCurl == CURLE_OK)
    {
        curl_easy_getinfo(pThis->pCurl, CURLINFO_RESPONSE_CODE, &pThis->lLastResp);
        switch (pThis->lLastResp)
        {
            case 200:
                /* OK, request was fulfilled */
            case 204:
                /* empty response */
                rc = VINF_SUCCESS;
                break;
            case 301:
            {
                const char *pszRedirect;
                curl_easy_getinfo(pThis->pCurl, CURLINFO_REDIRECT_URL, &pszRedirect);
                size_t cb = strlen(pszRedirect);
                if (cb > 0 && cb < 2048)
                    pThis->pszRedirLocation = RTStrDup(pszRedirect);
                rc = VERR_HTTP_REDIRECTED;
                break;
            }
            case 400:
                /* bad request */
                rc = VERR_HTTP_BAD_REQUEST;
                break;
            case 403:
                /* forbidden, authorization will not help */
                rc = VERR_HTTP_ACCESS_DENIED;
                break;
            case 404:
                /* URL not found */
                rc = VERR_HTTP_NOT_FOUND;
                break;
        }

        if (pThis->pszRedirLocation)
            Log(("rtHttpGetCalcStatus: rc=%Rrc lastResp=%lu redir='%s'\n", rc, pThis->lLastResp, pThis->pszRedirLocation));
        else
            Log(("rtHttpGetCalcStatus: rc=%Rrc lastResp=%lu\n", rc, pThis->lLastResp));
    }
    else
    {
        switch (rcCurl)
        {
            case CURLE_URL_MALFORMAT:
            case CURLE_COULDNT_RESOLVE_HOST:
                rc = VERR_HTTP_HOST_NOT_FOUND;
                break;
            case CURLE_COULDNT_CONNECT:
                rc = VERR_HTTP_COULDNT_CONNECT;
                break;
            case CURLE_SSL_CONNECT_ERROR:
                rc = VERR_HTTP_SSL_CONNECT_ERROR;
                break;
            case CURLE_SSL_CACERT:
                /* The peer certificate cannot be authenticated with the CA certificates
                 * set by RTHttpSetCAFile(). We need other or additional CA certificates. */
                rc = VERR_HTTP_CACERT_CANNOT_AUTHENTICATE;
                break;
            case CURLE_SSL_CACERT_BADFILE:
                /* CAcert file (see RTHttpSetCAFile()) has wrong format */
                rc = VERR_HTTP_CACERT_WRONG_FORMAT;
                break;
            case CURLE_ABORTED_BY_CALLBACK:
                /* forcefully aborted */
                rc = VERR_HTTP_ABORTED;
                break;
            case CURLE_COULDNT_RESOLVE_PROXY:
                rc = VERR_HTTP_PROXY_NOT_FOUND;
                break;
            case CURLE_WRITE_ERROR:
                rc = RT_FAILURE_NP(pThis->rcOutput) ? pThis->rcOutput : VERR_WRITE_ERROR;
                break;
            //case CURLE_READ_ERROR

            default:
                break;
        }
        Log(("rtHttpGetCalcStatus: rc=%Rrc rcCurl=%u\n", rc, rcCurl));
    }

    return rc;
}


/**
 * cURL callback for reporting progress, we use it for checking for abort.
 */
static int rtHttpProgress(void *pData, double rdTotalDownload, double rdDownloaded, double rdTotalUpload, double rdUploaded)
{
    PRTHTTPINTERNAL pThis = (PRTHTTPINTERNAL)pData;
    AssertReturn(pThis->u32Magic == RTHTTP_MAGIC, 1);
    RT_NOREF_PV(rdTotalUpload);
    RT_NOREF_PV(rdUploaded);

    pThis->cbDownloadHint = (uint64_t)rdTotalDownload;

    if (pThis->pfnDownloadProgress)
        pThis->pfnDownloadProgress(pThis, pThis->pvDownloadProgressUser, (uint64_t)rdTotalDownload, (uint64_t)rdDownloaded);

    return pThis->fAbort ? 1 : 0;
}


/**
 * Whether we're likely to need SSL to handle the give URL.
 *
 * @returns true if we need, false if we probably don't.
 * @param   pszUrl              The URL.
 */
static bool rtHttpNeedSsl(const char *pszUrl)
{
    return RTStrNICmp(pszUrl, RT_STR_TUPLE("https:")) == 0;
}


/**
 * Applies recoded settings to the cURL instance before doing work.
 *
 * @returns IPRT status code.
 * @param   pThis           The HTTP/HTTPS client instance.
 * @param   pszUrl          The URL.
 */
static int rtHttpApplySettings(PRTHTTPINTERNAL pThis, const char *pszUrl)
{
    /*
     * The URL.
     */
    int rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_URL, pszUrl);
    if (CURL_FAILURE(rcCurl))
        return VERR_INVALID_PARAMETER;

    /*
     * Proxy config.
     */
    int rc = rtHttpConfigureProxyForUrl(pThis, pszUrl);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Setup SSL.  Can be a bit of work.
     */
    rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1);
    if (CURL_FAILURE(rcCurl))
        return VERR_INVALID_PARAMETER;

    const char *pszCaFile = pThis->pszCaFile;
    if (   !pszCaFile
        && rtHttpNeedSsl(pszUrl))
    {
        rc = RTHttpUseTemporaryCaFile(pThis, NULL);
        if (RT_SUCCESS(rc))
            pszCaFile = pThis->pszCaFile;
        else
            return rc; /* Non-portable alternative: pszCaFile = "/etc/ssl/certs/ca-certificates.crt"; */
    }
    if (pszCaFile)
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_CAINFO, pszCaFile);
        if (CURL_FAILURE(rcCurl))
            return VERR_HTTP_CURL_ERROR;
    }

    /*
     * Progress/abort.
     */
    rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_PROGRESSFUNCTION, &rtHttpProgress);
    if (CURL_FAILURE(rcCurl))
        return VERR_HTTP_CURL_ERROR;
    rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_PROGRESSDATA, (void *)pThis);
    if (CURL_FAILURE(rcCurl))
        return VERR_HTTP_CURL_ERROR;
    rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_NOPROGRESS, (long)0);
    if (CURL_FAILURE(rcCurl))
        return VERR_HTTP_CURL_ERROR;

    /*
     * Set default user agent string if necessary.  Some websites take offence
     * if we don't set it.
     */
    if (   !pThis->fHaveSetUserAgent
        && !pThis->fHaveUserAgentHeader)
    {
        rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_USERAGENT, "Mozilla/5.0 (AgnosticOS; Blend) IPRT/64.42");
        if (CURL_FAILURE(rcCurl))
            return VERR_HTTP_CURL_ERROR;
        pThis->fHaveSetUserAgent = true;
    }

    /*
     * Use GET by default.
     */
    rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_NOBODY, 0L);
    if (CURL_FAILURE(rcCurl))
        return VERR_HTTP_CURL_ERROR;
    rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_HEADER, 0L);
    if (CURL_FAILURE(rcCurl))
        return VERR_HTTP_CURL_ERROR;

    return VINF_SUCCESS;
}


/**
 * cURL callback for writing data.
 */
static size_t rtHttpWriteData(void *pvBuf, size_t cbUnit, size_t cUnits, void *pvUser)
{
    PRTHTTPINTERNAL pThis = (PRTHTTPINTERNAL)pvUser;

    /*
     * Do max size and overflow checks.
     */
    size_t const cbToAppend = cbUnit * cUnits;
    size_t const cbCurSize  = pThis->Output.Mem.cb;
    size_t const cbNewSize  = cbCurSize + cbToAppend;
    if (   cbToAppend < RTHTTP_MAX_MEM_DOWNLOAD_SIZE
        && cbNewSize  < RTHTTP_MAX_MEM_DOWNLOAD_SIZE)
    {
        if (cbNewSize + 1 <= pThis->Output.Mem.cbAllocated)
        {
            memcpy(&pThis->Output.Mem.pb[cbCurSize], pvBuf, cbToAppend);
            pThis->Output.Mem.cb = cbNewSize;
            pThis->Output.Mem.pb[cbNewSize] = '\0';
            return cbToAppend;
        }

        /*
         * We need to reallocate the output buffer.
         */
        /** @todo this could do with a better strategy wrt growth. */
        size_t cbAlloc = RT_ALIGN_Z(cbNewSize + 1, 64);
        if (   cbAlloc <= pThis->cbDownloadHint
            && pThis->cbDownloadHint < RTHTTP_MAX_MEM_DOWNLOAD_SIZE)
            cbAlloc = RT_ALIGN_Z(pThis->cbDownloadHint + 1, 64);

        uint8_t *pbNew = (uint8_t *)RTMemRealloc(pThis->Output.Mem.pb, cbAlloc);
        if (pbNew)
        {
            memcpy(&pbNew[cbCurSize], pvBuf, cbToAppend);
            pbNew[cbNewSize] = '\0';

            pThis->Output.Mem.cbAllocated = cbAlloc;
            pThis->Output.Mem.pb = pbNew;
            pThis->Output.Mem.cb = cbNewSize;
            return cbToAppend;
        }

        pThis->rcOutput = VERR_NO_MEMORY;
    }
    else
        pThis->rcOutput = VERR_TOO_MUCH_DATA;

    /*
     * Failure - abort.
     */
    RTMemFree(pThis->Output.Mem.pb);
    pThis->Output.Mem.pb = NULL;
    pThis->Output.Mem.cb = RTHTTP_MAX_MEM_DOWNLOAD_SIZE;
    pThis->fAbort        = true;
    return 0;
}


/**
 * Internal worker that performs a HTTP GET.
 *
 * @returns IPRT status code.
 * @param   hHttp               The HTTP/HTTPS client instance.
 * @param   pszUrl              The URL.
 * @param   fNoBody             Set to suppress the body.
 * @param   ppvResponse         Where to return the pointer to the allocated
 *                              response data (RTMemFree).  There will always be
 *                              an zero terminator char after the response, that
 *                              is not part of the size returned via @a pcb.
 * @param   pcb                 The size of the response data.
 *
 * @remarks We ASSUME the API user doesn't do concurrent GETs in different
 *          threads, because that will probably blow up!
 */
static int rtHttpGetToMem(RTHTTP hHttp, const char *pszUrl, bool fNoBody, uint8_t **ppvResponse, size_t *pcb)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);

    /*
     * Reset the return values in case of more "GUI programming" on the client
     * side (i.e. a programming style not bothering checking return codes).
     */
    *ppvResponse = NULL;
    *pcb         = 0;

    /*
     * Set the busy flag (paranoia).
     */
    bool fBusy = ASMAtomicXchgBool(&pThis->fBusy, true);
    AssertReturn(!fBusy, VERR_WRONG_ORDER);

    /*
     * Reset the state and apply settings.
     */
    pThis->fAbort = false;
    pThis->rcOutput = VINF_SUCCESS;
    pThis->cbDownloadHint = 0;

    int rc = rtHttpApplySettings(hHttp, pszUrl);
    if (RT_SUCCESS(rc))
    {
        RT_ZERO(pThis->Output.Mem);
        int rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_WRITEFUNCTION, &rtHttpWriteData);
        if (!CURL_FAILURE(rcCurl))
            rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_WRITEDATA, (void *)pThis);
        if (fNoBody)
        {
            if (!CURL_FAILURE(rcCurl))
                rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_NOBODY, 1L);
            if (!CURL_FAILURE(rcCurl))
                rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_HEADER, 1L);
        }
        if (!CURL_FAILURE(rcCurl))
        {
            /*
             * Perform the HTTP operation.
             */
            rcCurl = curl_easy_perform(pThis->pCurl);
            rc = rtHttpGetCalcStatus(pThis, rcCurl);
            if (RT_SUCCESS(rc))
                rc = pThis->rcOutput;
            if (RT_SUCCESS(rc))
            {
                *ppvResponse = pThis->Output.Mem.pb;
                *pcb         = pThis->Output.Mem.cb;
                Log(("rtHttpGetToMem: %zx bytes (allocated %zx)\n", pThis->Output.Mem.cb, pThis->Output.Mem.cbAllocated));
            }
            else if (pThis->Output.Mem.pb)
                RTMemFree(pThis->Output.Mem.pb);
            RT_ZERO(pThis->Output.Mem);
        }
        else
            rc = VERR_HTTP_CURL_ERROR;
    }

    ASMAtomicWriteBool(&pThis->fBusy, false);
    return rc;
}


RTR3DECL(int) RTHttpGetText(RTHTTP hHttp, const char *pszUrl, char **ppszNotUtf8)
{
    Log(("RTHttpGetText: hHttp=%p pszUrl=%s\n", hHttp, pszUrl));
    uint8_t *pv;
    size_t   cb;
    int rc = rtHttpGetToMem(hHttp, pszUrl, false /*fNoBody*/, &pv, &cb);
    if (RT_SUCCESS(rc))
    {
        if (pv) /* paranoia */
            *ppszNotUtf8 = (char *)pv;
        else
            *ppszNotUtf8 = (char *)RTMemDup("", 1);
    }
    else
        *ppszNotUtf8 = NULL;
    return rc;
}


RTR3DECL(int) RTHttpGetHeaderText(RTHTTP hHttp, const char *pszUrl, char **ppszNotUtf8)
{
    Log(("RTHttpGetText: hHttp=%p pszUrl=%s\n", hHttp, pszUrl));
    uint8_t *pv;
    size_t   cb;
    int rc = rtHttpGetToMem(hHttp, pszUrl, true /*fNoBody*/, &pv, &cb);
    if (RT_SUCCESS(rc))
    {
        if (pv) /* paranoia */
            *ppszNotUtf8 = (char *)pv;
        else
            *ppszNotUtf8 = (char *)RTMemDup("", 1);
    }
    else
        *ppszNotUtf8 = NULL;
    return rc;

}


RTR3DECL(void) RTHttpFreeResponseText(char *pszNotUtf8)
{
    RTMemFree(pszNotUtf8);
}


RTR3DECL(int) RTHttpGetBinary(RTHTTP hHttp, const char *pszUrl, void **ppvResponse, size_t *pcb)
{
    Log(("RTHttpGetBinary: hHttp=%p pszUrl=%s\n", hHttp, pszUrl));
    return rtHttpGetToMem(hHttp, pszUrl, false /*fNoBody*/, (uint8_t **)ppvResponse, pcb);
}


RTR3DECL(int) RTHttpGetHeaderBinary(RTHTTP hHttp, const char *pszUrl, void **ppvResponse, size_t *pcb)
{
    Log(("RTHttpGetBinary: hHttp=%p pszUrl=%s\n", hHttp, pszUrl));
    return rtHttpGetToMem(hHttp, pszUrl, true /*fNoBody*/, (uint8_t **)ppvResponse, pcb);
}


RTR3DECL(void) RTHttpFreeResponse(void *pvResponse)
{
    RTMemFree(pvResponse);
}


/**
 * cURL callback for writing data to a file.
 */
static size_t rtHttpWriteDataToFile(void *pvBuf, size_t cbUnit, size_t cUnits, void *pvUser)
{
    PRTHTTPINTERNAL pThis = (PRTHTTPINTERNAL)pvUser;
    size_t cbWritten = 0;
    int rc = RTFileWrite(pThis->Output.hFile, pvBuf, cbUnit * cUnits, &cbWritten);
    if (RT_SUCCESS(rc))
        return cbWritten;
    Log(("rtHttpWriteDataToFile: rc=%Rrc cbUnit=%zd cUnits=%zu\n", rc, cbUnit, cUnits));
    pThis->rcOutput = rc;
    return 0;
}


RTR3DECL(int) RTHttpGetFile(RTHTTP hHttp, const char *pszUrl, const char *pszDstFile)
{
    Log(("RTHttpGetBinary: hHttp=%p pszUrl=%s pszDstFile=%s\n", hHttp, pszUrl, pszDstFile));
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);

    /*
     * Set the busy flag (paranoia).
     */
    bool fBusy = ASMAtomicXchgBool(&pThis->fBusy, true);
    AssertReturn(!fBusy, VERR_WRONG_ORDER);

    /*
     * Reset the state and apply settings.
     */
    pThis->fAbort = false;
    pThis->rcOutput = VINF_SUCCESS;
    pThis->cbDownloadHint = 0;

    int rc = rtHttpApplySettings(hHttp, pszUrl);
    if (RT_SUCCESS(rc))
    {
        pThis->Output.hFile = NIL_RTFILE;
        int rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_WRITEFUNCTION, &rtHttpWriteDataToFile);
        if (!CURL_FAILURE(rcCurl))
            rcCurl = curl_easy_setopt(pThis->pCurl, CURLOPT_WRITEDATA, (void *)pThis);
        if (!CURL_FAILURE(rcCurl))
        {
            /*
             * Open the output file.
             */
            rc = RTFileOpen(&pThis->Output.hFile, pszDstFile, RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_READWRITE);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Perform the HTTP operation.
                 */
                rcCurl = curl_easy_perform(pThis->pCurl);
                rc = rtHttpGetCalcStatus(pThis, rcCurl);
                if (RT_SUCCESS(rc))
                     rc = pThis->rcOutput;

                int rc2 = RTFileClose(pThis->Output.hFile);
                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                    rc = rc2;
            }
            pThis->Output.hFile = NIL_RTFILE;
        }
        else
            rc = VERR_HTTP_CURL_ERROR;
    }

    ASMAtomicWriteBool(&pThis->fBusy, false);
    return rc;
}


RTR3DECL(int) RTHttpSetDownloadProgressCallback(RTHTTP hHttp, PRTHTTPDOWNLDPROGRCALLBACK pfnDownloadProgress, void *pvUser)
{
    PRTHTTPINTERNAL pThis = hHttp;
    RTHTTP_VALID_RETURN(pThis);

    pThis->pfnDownloadProgress = pfnDownloadProgress;
    pThis->pvDownloadProgressUser = pvUser;
    return VINF_SUCCESS;
}

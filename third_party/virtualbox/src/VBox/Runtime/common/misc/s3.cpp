/* $Id: s3.cpp $ */
/** @file
 * IPRT - S3 communication API.
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
#include <iprt/s3.h>
#include "internal/iprt.h"

#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/base64.h>
#include <iprt/file.h>
#include <iprt/stream.h>

#ifdef RT_OS_WINDOWS /* OpenSSL drags in Windows.h, which isn't compatible with -Wall.  */
# include <iprt/win/windows.h>
#endif
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <libxml/parser.h>

#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct RTS3INTERNAL
{
    uint32_t u32Magic;
    CURL *pCurl;
    char *pszAccessKey;
    char *pszSecretKey;
    char *pszBaseUrl;
    char *pszUserAgent;

    PFNRTS3PROGRESS pfnProgressCallback;
    void *pvUser;

    long lLastResp;
} RTS3INTERNAL;
typedef RTS3INTERNAL* PRTS3INTERNAL;

typedef struct RTS3TMPMEMCHUNK
{
    char *pszMem;
    size_t cSize;
} RTS3TMPMEMCHUNK;
typedef RTS3TMPMEMCHUNK *PRTS3TMPMEMCHUNK;


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTS3_VALID_RETURN_RC(hS3, rc) \
    do { \
        AssertPtrReturn((hS3), (rc)); \
        AssertReturn((hS3)->u32Magic == RTS3_MAGIC, (rc)); \
    } while (0)

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTS3_VALID_RETURN(hS3) RTS3_VALID_RETURN_RC((hS3), VERR_INVALID_HANDLE)

/** Validates a handle and returns (void) if not valid. */
#define RTS3_VALID_RETURN_VOID(hS3) \
    do { \
        AssertPtrReturnVoid(hS3); \
        AssertReturnVoid((hS3)->u32Magic == RTS3_MAGIC); \
    } while (0)


/*********************************************************************************************************************************
*   Private RTS3 helper                                                                                                          *
*********************************************************************************************************************************/

static char* rtS3Host(const char* pszBucket, const char* pszKey, const char* pszBaseUrl)
{
    char* pszUrl;
    /* Host header entry */
    if (pszBucket[0] == 0)
        RTStrAPrintf(&pszUrl, "%s", pszBaseUrl);
    else if (pszKey[0] == 0)
        RTStrAPrintf(&pszUrl, "%s.%s", pszBucket, pszBaseUrl);
    else
        RTStrAPrintf(&pszUrl, "%s.%s/%s", pszBucket, pszBaseUrl, pszKey);
    return pszUrl;
}

static char* rtS3HostHeader(const char* pszBucket, const char* pszBaseUrl)
{
    char* pszUrl;
    /* Host header entry */
    if (pszBucket[0] != 0)
        RTStrAPrintf(&pszUrl, "Host: %s.%s", pszBucket, pszBaseUrl);
    else
        RTStrAPrintf(&pszUrl, "Host: %s", pszBaseUrl);
    return pszUrl;
}

static char* rtS3DateHeader()
{
    /* Date header entry */
    RTTIMESPEC TimeSpec;
    RTTIME Time;
    RTTimeExplode(&Time, RTTimeNow(&TimeSpec));

    static const char s_apszDayNms[7][4] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    static const char s_apszMonthNms[1+12][4] =
    { "???", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    char *pszDate;
    RTStrAPrintf(&pszDate, "Date: %s, %02u %s %04d %02u:%02u:%02u UTC",
                 s_apszDayNms[Time.u8WeekDay],
                 Time.u8MonthDay,
                 s_apszMonthNms[Time.u8Month],
                 Time.i32Year,
                 Time.u8Hour,
                 Time.u8Minute,
                 Time.u8Second);

    return pszDate;
}

static char* rtS3ParseHeaders(char** ppHeaders, size_t cHeadEnts)
{
    char pszEmpty[] = "";
    char *pszRes = NULL;
    char *pszDate = pszEmpty;
    char *pszType = pszEmpty;
    for(size_t i=0; i < cHeadEnts; ++i)
    {
        if(ppHeaders[i] != NULL)
        {
            if (RTStrStr(ppHeaders[i], "Date: ") == ppHeaders[i])
            {
                pszDate = &(ppHeaders[i][6]);
            }
            else if(RTStrStr(ppHeaders[i], "Content-Type: ") == ppHeaders[i])
            {
                pszType = &(ppHeaders[i][14]);
//                char *pszTmp = RTStrDup (&(ppHeaders[i][14]));
//                if (pszRes)
//                {
//                    char *pszTmp1 = pszRes;
//                    RTStrAPrintf(&pszRes, "%s\n%s", pszRes, pszTmp);
//                    RTStrFree(pszTmp);
//                    RTStrFree(pszTmp1);
//                }
//                else
//                    pszRes = pszTmp;
            }
        }
    }
    RTStrAPrintf(&pszRes, "\n%s\n%s", pszType, pszDate);
    return pszRes;
}

static char* rtS3Canonicalize(const char* pszAction, const char* pszBucket, const char* pszKey, char** papszHeadEnts, size_t cHeadEnts)
{
    char* pszRes;
    /* Grep the necessary info out of the headers & put them in a string */
    char* pszHead = rtS3ParseHeaders(papszHeadEnts, cHeadEnts);
    /* Create the string which will be used as signature */
    RTStrAPrintf(&pszRes, "%s\n%s\n/",
                 pszAction,
                 pszHead);
    RTStrFree(pszHead);
    /* Add the bucket if the bucket isn't empty */
    if (pszBucket[0] != 0)
    {
        char* pszTmp = pszRes;
        RTStrAPrintf(&pszRes, "%s%s/", pszRes, pszBucket);
        RTStrFree(pszTmp);
    }
    /* Add the key if the key isn't empty. */
    if (pszKey[0] != 0)
    {
        char* pszTmp = pszRes;
        RTStrAPrintf(&pszRes, "%s%s", pszRes, pszKey);
        RTStrFree(pszTmp);
    }

    return pszRes;
}

static char* rtS3CreateSignature(PRTS3INTERNAL pS3Int, const char* pszAction, const char* pszBucket, const char* pszKey,
                                 char** papszHeadEnts, size_t cHeadEnts)
{
    /* Create a string we can sign */
    char* pszSig = rtS3Canonicalize(pszAction, pszBucket, pszKey, papszHeadEnts, cHeadEnts);
//    printf ("Sig %s\n", pszSig);
    /* Sign the string by creating a SHA1 finger print */
    char pszSigEnc[1024];
    unsigned int cSigEnc = sizeof(pszSigEnc);
    HMAC(EVP_sha1(), pS3Int->pszSecretKey, (int)strlen(pS3Int->pszSecretKey),
         (const unsigned char*)pszSig, strlen(pszSig),
         (unsigned char*)pszSigEnc, &cSigEnc);
    RTStrFree(pszSig);
    /* Convert the signature to Base64 */
    size_t cSigBase64Enc = RTBase64EncodedLength(cSigEnc) + 1; /* +1 for the 0 */
    char *pszSigBase64Enc = (char*)RTMemAlloc(cSigBase64Enc);
    size_t cRes;
    RTBase64Encode(pszSigEnc, cSigEnc, pszSigBase64Enc, cSigBase64Enc, &cRes);

    return pszSigBase64Enc;
}

static char* rtS3CreateAuthHeader(PRTS3INTERNAL pS3Int, const char* pszAction, const char* pszBucket, const char* pszKey,
                                  char** papszHeadEnts, size_t cHeadEnts)
{
    char *pszAuth;
    /* Create a signature out of the header & the bucket/key info */
    char *pszSigBase64Enc = rtS3CreateSignature(pS3Int, pszAction, pszBucket, pszKey, papszHeadEnts, cHeadEnts);
    /* Create the authorization header entry */
    RTStrAPrintf(&pszAuth, "Authorization: AWS %s:%s",
                 pS3Int->pszAccessKey,
                 pszSigBase64Enc);
    RTStrFree(pszSigBase64Enc);
    return pszAuth;
}

static int rtS3Perform(PRTS3INTERNAL pS3Int)
{
    int rc = VERR_INTERNAL_ERROR;
    CURLcode code = curl_easy_perform(pS3Int->pCurl);
    if (code == CURLE_OK)
    {
        curl_easy_getinfo(pS3Int->pCurl, CURLINFO_RESPONSE_CODE, &pS3Int->lLastResp);
        switch (pS3Int->lLastResp)
        {
            case 200:
            case 204: rc = VINF_SUCCESS; break; /* No content */
            case 403: rc = VERR_S3_ACCESS_DENIED; break; /* Access denied */
            case 404: rc = VERR_S3_NOT_FOUND; break; /* Site not found */
        }
    }
    else
    {
        switch(code)
        {
            case CURLE_URL_MALFORMAT:
            case CURLE_COULDNT_RESOLVE_HOST:
#if defined(CURLE_REMOTE_FILE_NOT_FOUND)
            case CURLE_REMOTE_FILE_NOT_FOUND: rc = VERR_S3_NOT_FOUND; break;
#elif defined(CURLE_FILE_COULDNT_READ_FILE)
            case CURLE_FILE_COULDNT_READ_FILE: rc = VERR_S3_NOT_FOUND; break;
#endif
#if defined(CURLE_REMOTE_ACCESS_DENIED)
            case CURLE_REMOTE_ACCESS_DENIED: rc = VERR_S3_ACCESS_DENIED; break;
#elif defined(CURLE_FTP_ACCESS_DENIED)
            case CURLE_FTP_ACCESS_DENIED: rc = VERR_S3_ACCESS_DENIED; break;
#endif
            case CURLE_ABORTED_BY_CALLBACK: rc = VERR_S3_CANCELED; break;
            default: break;
        }
    }
    return rc;
}

static size_t rtS3WriteNothingCallback(void *pvBuf, size_t cbItem, size_t cItems, void *pvUser)
{
    NOREF(pvBuf); NOREF(pvUser);
    return cbItem * cItems;
}

static size_t rtS3WriteMemoryCallback(void *pvBuf, size_t cSize, size_t cBSize, void *pvUser)
{
    PRTS3TMPMEMCHUNK pTmpMem = (PRTS3TMPMEMCHUNK)pvUser;
    size_t cRSize = cSize * cBSize;

    pTmpMem->pszMem = (char*)RTMemRealloc(pTmpMem->pszMem, pTmpMem->cSize + cRSize + 1);
    if (pTmpMem->pszMem)
    {
        memcpy(&(pTmpMem->pszMem[pTmpMem->cSize]), pvBuf, cRSize);
        pTmpMem->cSize += cRSize;
        pTmpMem->pszMem[pTmpMem->cSize] = 0;
    }
    return cRSize;
}

static size_t rtS3WriteFileCallback(void *pvBuf, size_t cSize, size_t cBSize, void *pvUser)
{
    size_t cWritten;
    RTFileWrite(*(RTFILE*)pvUser, pvBuf, cSize * cBSize, &cWritten);
    return cWritten;
}

static size_t rtS3ReadFileCallback(void *pvBuf, size_t cSize, size_t cBSize, void *pvUser)
{
  size_t cRead;
  RTFileRead(*(RTFILE*)pvUser, pvBuf, cSize * cBSize, &cRead);

  return cRead;
}

static int rtS3ProgressCallback(void *pvUser, double dDlTotal, double dDlNow, double dUlTotal, double dUlNow)
{
    if (pvUser)
    {
        PRTS3INTERNAL pS3Int = (PRTS3INTERNAL)pvUser;
        if (pS3Int->pfnProgressCallback)
        {
            int rc = VINF_SUCCESS;
            if (dDlTotal > 0)
                rc = pS3Int->pfnProgressCallback((unsigned)(100.0/dDlTotal*dDlNow), pS3Int->pvUser);
            else if (dUlTotal > 0)
                rc = pS3Int->pfnProgressCallback((unsigned)(100.0/dUlTotal*dUlNow), pS3Int->pvUser);
            if (rc != VINF_SUCCESS)
                return -1;
        }
    }
    return CURLE_OK;
}

static void rtS3ReinitCurl(PRTS3INTERNAL pS3Int)
{
    if (pS3Int &&
        pS3Int->pCurl)
    {
        /* Reset the CURL object to an defined state */
        curl_easy_reset(pS3Int->pCurl);
        /* Make sure HTTP 1.1 is used */
        curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
        /* We are cool we are a user agent now */
        if (pS3Int->pszUserAgent)
            curl_easy_setopt(pS3Int->pCurl, CURLOPT_USERAGENT, pS3Int->pszUserAgent);
        /* Check if the user has a progress callback requested */
        if (pS3Int->pfnProgressCallback)
        {
            /* Yes, we are willing to receive progress info */
            curl_easy_setopt(pS3Int->pCurl, CURLOPT_NOPROGRESS, 0);
            /* Callback for the progress info */
            curl_easy_setopt(pS3Int->pCurl, CURLOPT_PROGRESSFUNCTION, rtS3ProgressCallback);
            curl_easy_setopt(pS3Int->pCurl, CURLOPT_PROGRESSDATA, pS3Int);
        }
        /* Disable the internal cURL write function by providing one which does
         * nothing */
        curl_easy_setopt(pS3Int->pCurl, CURLOPT_WRITEFUNCTION, rtS3WriteNothingCallback);
        /* Set this do get some verbose info what CURL is doing */
//        curl_easy_setopt(pS3Int->pCurl, CURLOPT_VERBOSE, 1);
    }
}


/*********************************************************************************************************************************
*   Private XML helper                                                                                                           *
*********************************************************************************************************************************/

static xmlNodePtr rtS3FindNode(xmlNodePtr pNode, const char *pszName)
{
    pNode = pNode->xmlChildrenNode;
    while (pNode != NULL)
    {
        /* Check this level. */
        if (!xmlStrcmp(pNode->name, (const xmlChar *)pszName))
            return pNode;

        /* Recursively check the children of this node. */
        xmlNodePtr pChildNode = rtS3FindNode(pNode, pszName);
        if (pChildNode != NULL)
            return pChildNode;

        /* Next node. */
        pNode = pNode->next;
    }
    return pNode;
}

static int rtS3ReadXmlFromMemory(PRTS3TMPMEMCHUNK pChunk, const char* pszRootElement, xmlDocPtr *ppDoc, xmlNodePtr *ppCur)
{
    *ppDoc = xmlReadMemory(pChunk->pszMem, (int)pChunk->cSize, "", "ISO-8859-1", XML_PARSE_NOBLANKS | XML_PARSE_NONET);
    if (*ppDoc == NULL)
        return VERR_PARSE_ERROR;

    *ppCur = xmlDocGetRootElement(*ppDoc);
    if (*ppCur == NULL)
    {
        xmlFreeDoc(*ppDoc);
        return VERR_PARSE_ERROR;
    }
    if (xmlStrcmp((*ppCur)->name, (const xmlChar *) pszRootElement))
    {
        xmlFreeDoc(*ppDoc);
        return VERR_PARSE_ERROR;
    }
    return VINF_SUCCESS;
}

static void rtS3ExtractAllBuckets(xmlDocPtr pDoc, xmlNodePtr pNode, PCRTS3BUCKETENTRY *ppBuckets)
{
    pNode = rtS3FindNode(pNode, "Buckets");
    if (pNode != NULL)
    {
        PRTS3BUCKETENTRY pPrevBucket = NULL;
        xmlNodePtr pCurBucket = pNode->xmlChildrenNode;
        while (pCurBucket != NULL)
        {
            if ((!xmlStrcmp(pCurBucket->name, (const xmlChar *)"Bucket")))
            {
                PRTS3BUCKETENTRY pBucket = (PRTS3BUCKETENTRY)RTMemAllocZ(sizeof(RTS3BUCKETENTRY));
                pBucket->pPrev = pPrevBucket;
                if (pPrevBucket)
                    pPrevBucket->pNext = pBucket;
                else
                    (*ppBuckets) = pBucket;
                pPrevBucket = pBucket;
                xmlNodePtr pCurCont = pCurBucket->xmlChildrenNode;
                while (pCurCont != NULL)
                {
                    if ((!xmlStrcmp(pCurCont->name, (const xmlChar *)"Name")))
                    {
                        xmlChar *pszKey = xmlNodeListGetString(pDoc, pCurCont->xmlChildrenNode, 1);
                        pBucket->pszName = RTStrDup((const char*)pszKey);
                        xmlFree(pszKey);
                    }
                    if ((!xmlStrcmp(pCurCont->name, (const xmlChar*)"CreationDate")))
                    {
                        xmlChar *pszKey = xmlNodeListGetString(pDoc, pCurCont->xmlChildrenNode, 1);
                        pBucket->pszCreationDate = RTStrDup((const char*)pszKey);
                        xmlFree(pszKey);
                    }
                    pCurCont = pCurCont->next;
                }
            }
            pCurBucket = pCurBucket->next;
        }
    }
}

static void rtS3ExtractAllKeys(xmlDocPtr pDoc, xmlNodePtr pNode, PCRTS3KEYENTRY *ppKeys)
{
    if (pNode != NULL)
    {
        PRTS3KEYENTRY pPrevKey = NULL;
        xmlNodePtr pCurKey = pNode->xmlChildrenNode;
        while (pCurKey != NULL)
        {
            if ((!xmlStrcmp(pCurKey->name, (const xmlChar *)"Contents")))
            {
                PRTS3KEYENTRY pKey = (PRTS3KEYENTRY)RTMemAllocZ(sizeof(RTS3KEYENTRY));
                pKey->pPrev = pPrevKey;
                if (pPrevKey)
                    pPrevKey->pNext = pKey;
                else
                    (*ppKeys) = pKey;
                pPrevKey = pKey;
                xmlNodePtr pCurCont = pCurKey->xmlChildrenNode;
                while (pCurCont != NULL)
                {
                    if ((!xmlStrcmp(pCurCont->name, (const xmlChar *)"Key")))
                    {
                        xmlChar *pszKey = xmlNodeListGetString(pDoc, pCurCont->xmlChildrenNode, 1);
                        pKey->pszName = RTStrDup((const char*)pszKey);
                        xmlFree(pszKey);
                    }
                    if ((!xmlStrcmp(pCurCont->name, (const xmlChar*)"LastModified")))
                    {
                        xmlChar *pszKey = xmlNodeListGetString(pDoc, pCurCont->xmlChildrenNode, 1);
                        pKey->pszLastModified = RTStrDup((const char*)pszKey);
                        xmlFree(pszKey);
                    }
                    if ((!xmlStrcmp(pCurCont->name, (const xmlChar*)"Size")))
                    {
                        xmlChar *pszKey = xmlNodeListGetString(pDoc, pCurCont->xmlChildrenNode, 1);
                        pKey->cbFile = RTStrToUInt64((const char*)pszKey);
                        xmlFree(pszKey);
                    }
                    pCurCont = pCurCont->next;
                }
            }
            pCurKey = pCurKey->next;
        }
    }
}


/*********************************************************************************************************************************
*   Public RTS3 interface                                                                                                        *
*********************************************************************************************************************************/

RTR3DECL(int) RTS3Create(PRTS3 ppS3, const char* pszAccessKey, const char* pszSecretKey, const char* pszBaseUrl, const char* pszUserAgent /* = NULL */)
{
    AssertPtrReturn(ppS3, VERR_INVALID_POINTER);

    /* We need at least an URL to connect with */
    if (pszBaseUrl == NULL ||
        pszBaseUrl[0] == 0)
        return VERR_INVALID_PARAMETER;

    /* In windows, this will init the winsock stuff */
    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        return VERR_INTERNAL_ERROR;

    CURL* pCurl = curl_easy_init();
    if (!pCurl)
        return VERR_INTERNAL_ERROR;

    PRTS3INTERNAL pS3Int = (PRTS3INTERNAL)RTMemAllocZ(sizeof(RTS3INTERNAL));
    if (pS3Int == NULL)
        return VERR_NO_MEMORY;

    pS3Int->u32Magic = RTS3_MAGIC;
    pS3Int->pCurl = pCurl;
    pS3Int->pszAccessKey = RTStrDup(pszAccessKey);
    pS3Int->pszSecretKey = RTStrDup(pszSecretKey);
    pS3Int->pszBaseUrl = RTStrDup(pszBaseUrl);
    if (pszUserAgent)
        pS3Int->pszUserAgent = RTStrDup(pszUserAgent);

    *ppS3 = (RTS3)pS3Int;

    return VINF_SUCCESS;
}

RTR3DECL(void) RTS3Destroy(RTS3 hS3)
{
    if (hS3 == NIL_RTS3)
        return;

    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN_VOID(pS3Int);

    curl_easy_cleanup(pS3Int->pCurl);

    pS3Int->u32Magic = RTS3_MAGIC_DEAD;

    if (pS3Int->pszUserAgent)
        RTStrFree(pS3Int->pszUserAgent);
    RTStrFree(pS3Int->pszBaseUrl);
    RTStrFree(pS3Int->pszSecretKey);
    RTStrFree(pS3Int->pszAccessKey);

    RTMemFree(pS3Int);

    curl_global_cleanup();
}

RTR3DECL(void) RTS3SetProgressCallback(RTS3 hS3, PFNRTS3PROGRESS pfnProgressCallback, void *pvUser /* = NULL */)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN_VOID(pS3Int);

    pS3Int->pfnProgressCallback = pfnProgressCallback;
    pS3Int->pvUser = pvUser;
}

RTR3DECL(int) RTS3GetBuckets(RTS3 hS3, PCRTS3BUCKETENTRY *ppBuckets)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN(pS3Int);

    /* Properly initialize this */
    *ppBuckets = NULL;

    /* Reset the CURL object to an defined state */
    rtS3ReinitCurl(pS3Int);
    /* Create the CURL object to operate on */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_URL, pS3Int->pszBaseUrl);

    /* Create the three basic header entries */
    char *apszHead[3] =
    {
        rtS3HostHeader("", pS3Int->pszBaseUrl), /* Host entry */
        rtS3DateHeader(),                       /* Date entry */
        NULL                                    /* Authorization entry */
    };
    /* Create the authorization header entry */
    apszHead[RT_ELEMENTS(apszHead)-1] = rtS3CreateAuthHeader(pS3Int, "GET", "", "", apszHead, RT_ELEMENTS(apszHead));

    /* Add all headers to curl */
    struct curl_slist* pHeaders = NULL; /* Init to NULL is important */
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        pHeaders = curl_slist_append(pHeaders, apszHead[i]);

    /* Pass our list of custom made headers */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTPHEADER, pHeaders);

    RTS3TMPMEMCHUNK chunk = { NULL, 0 };
    /* Set the callback which receive the content */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_WRITEFUNCTION, rtS3WriteMemoryCallback);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_WRITEDATA, (void *)&chunk);
    /* Start the request */
    int rc = rtS3Perform(pS3Int);

    /* Regardless of the result, free all used resources first*/
    curl_slist_free_all(pHeaders);
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        RTStrFree(apszHead[i]);

    /* On success parse the result */
    if (RT_SUCCESS(rc))
    {
        xmlDocPtr pDoc;
        xmlNodePtr pCur;
        /* Parse the xml memory for "ListAllMyBucketsResult" */
        rc = rtS3ReadXmlFromMemory(&chunk, "ListAllMyBucketsResult", &pDoc, &pCur);
        if (RT_SUCCESS(rc))
        {
            /* Now extract all buckets */
            rtS3ExtractAllBuckets(pDoc, pCur, ppBuckets);
            /* Free the xml stuff */
            xmlFreeDoc(pDoc);
        }
    }
    /* Free the temporary memory */
    RTMemFree(chunk.pszMem);

    return rc;
}

RTR3DECL(int) RTS3BucketsDestroy(PCRTS3BUCKETENTRY pBuckets)
{
    if (!pBuckets)
        return VINF_SUCCESS;

    while (pBuckets)
    {
        PCRTS3BUCKETENTRY pTemp = pBuckets;
        RTStrFree((char*)pBuckets->pszName);
        RTStrFree((char*)pBuckets->pszCreationDate);
        pBuckets = pBuckets->pNext;
        RTMemFree((PRTS3BUCKETENTRY )pTemp);
    }
    return VINF_SUCCESS;
}

RTR3DECL(int) RTS3CreateBucket(RTS3 hS3, const char* pszBucketName)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN(pS3Int);

    /* Reset the CURL object to an defined state */
    rtS3ReinitCurl(pS3Int);

    char* pszUrl = rtS3Host(pszBucketName, "", pS3Int->pszBaseUrl);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_URL, pszUrl);
    RTStrFree(pszUrl);

    /* Create the basic header entries */
    char *apszHead[4] =
    {
        RTStrDup("Content-Length: 0"),                     /* Content length entry */
        rtS3HostHeader(pszBucketName, pS3Int->pszBaseUrl), /* Host entry */
        rtS3DateHeader(),                                  /* Date entry */
        NULL                                               /* Authorization entry */
    };
    /* Create the authorization header entry */
    apszHead[RT_ELEMENTS(apszHead)-1] = rtS3CreateAuthHeader(pS3Int, "PUT", pszBucketName, "", apszHead, RT_ELEMENTS(apszHead));

    /* Add all headers to curl */
    struct curl_slist* pHeaders = NULL; /* Init to NULL is important */
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        pHeaders = curl_slist_append(pHeaders, apszHead[i]);

    /* Pass our list of custom made headers */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTPHEADER, pHeaders);

    /* Set CURL in upload mode */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_PUT, 1);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_UPLOAD, 1);

    /* Set the size of the file we like to transfer */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_INFILESIZE_LARGE, 0);

    /* Start the request */
    int rc = rtS3Perform(pS3Int);
    if (RT_FAILURE(rc))
    {
        /* Handle special failures */
        if (pS3Int->lLastResp == 409)
            rc = VERR_S3_BUCKET_ALREADY_EXISTS;
    }

    /* Regardless of the result, free all used resources first*/
    curl_slist_free_all(pHeaders);
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        RTStrFree(apszHead[i]);

    return rc;
}

RTR3DECL(int) RTS3DeleteBucket(RTS3 hS3, const char* pszBucketName)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN(pS3Int);

    /* Reset the CURL object to an defined state */
    rtS3ReinitCurl(pS3Int);

    char* pszUrl = rtS3Host(pszBucketName, "", pS3Int->pszBaseUrl);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_URL, pszUrl);
    RTStrFree(pszUrl);

    /* Create the three basic header entries */
    char *apszHead[3] =
    {
        rtS3HostHeader(pszBucketName, pS3Int->pszBaseUrl), /* Host entry */
        rtS3DateHeader(),                                  /* Date entry */
        NULL                                               /* Authorization entry */
    };
    /* Create the authorization header entry */
    apszHead[RT_ELEMENTS(apszHead)-1] = rtS3CreateAuthHeader(pS3Int, "DELETE", pszBucketName, "", apszHead, RT_ELEMENTS(apszHead));

    /* Add all headers to curl */
    struct curl_slist* pHeaders = NULL; /* Init to NULL is important */
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        pHeaders = curl_slist_append(pHeaders, apszHead[i]);

    /* Pass our list of custom made headers */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTPHEADER, pHeaders);

    /* Set CURL in delete mode */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_CUSTOMREQUEST, "DELETE");

    /* Start the request */
    int rc = rtS3Perform(pS3Int);
    if (RT_FAILURE(rc))
    {
        /* Handle special failures */
        if (pS3Int->lLastResp == 409)
            rc = VERR_S3_BUCKET_NOT_EMPTY;
    }

    /* Regardless of the result, free all used resources first*/
    curl_slist_free_all(pHeaders);
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        RTStrFree(apszHead[i]);

    return rc;
}

RTR3DECL(int) RTS3GetBucketKeys(RTS3 hS3, const char* pszBucketName, PCRTS3KEYENTRY *ppKeys)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN(pS3Int);

    *ppKeys = NULL;

    /* Reset the CURL object to an defined state */
    rtS3ReinitCurl(pS3Int);

    char* pszUrl = rtS3Host(pszBucketName, "", pS3Int->pszBaseUrl);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_URL, pszUrl);
    RTStrFree(pszUrl);

    /* Create the three basic header entries */
    char *apszHead[3] =
    {
        rtS3HostHeader(pszBucketName, pS3Int->pszBaseUrl), /* Host entry */
        rtS3DateHeader(),                                  /* Date entry */
        NULL                                               /* Authorization entry */
    };
    /* Create the authorization header entry */
    apszHead[RT_ELEMENTS(apszHead)-1] = rtS3CreateAuthHeader(pS3Int, "GET", pszBucketName, "", apszHead, RT_ELEMENTS(apszHead));

    /* Add all headers to curl */
    struct curl_slist* pHeaders = NULL; /* Init to NULL is important */
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        pHeaders = curl_slist_append(pHeaders, apszHead[i]);

    /* Pass our list of custom made headers */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTPHEADER, pHeaders);

    RTS3TMPMEMCHUNK chunk = { NULL, 0 };
    /* Set the callback which receive the content */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_WRITEFUNCTION, rtS3WriteMemoryCallback);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* Start the request */
    int rc = rtS3Perform(pS3Int);

    /* Regardless of the result, free all used resources first*/
    curl_slist_free_all(pHeaders);
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        RTStrFree(apszHead[i]);

    /* On success parse the result */
    if (RT_SUCCESS(rc))
    {
        xmlDocPtr pDoc;
        xmlNodePtr pCur;
        /* Parse the xml memory for "ListBucketResult" */
        rc = rtS3ReadXmlFromMemory(&chunk, "ListBucketResult", &pDoc, &pCur);
        if (RT_SUCCESS(rc))
        {
            /* Now extract all buckets */
            rtS3ExtractAllKeys(pDoc, pCur, ppKeys);
            /* Free the xml stuff */
            xmlFreeDoc(pDoc);
        }
    }
    /* Free the temporary memory */
    RTMemFree(chunk.pszMem);

    return rc;
}

RTR3DECL(int) RTS3KeysDestroy(PCRTS3KEYENTRY pKeys)
{
    if (!pKeys)
        return VINF_SUCCESS;

    while (pKeys)
    {
        PCRTS3KEYENTRY pTemp = pKeys;
        RTStrFree((char*)pKeys->pszName);
        RTStrFree((char*)pKeys->pszLastModified);
        pKeys = pKeys->pNext;
        RTMemFree((PRTS3KEYENTRY)pTemp);
    }
    return VINF_SUCCESS;
}

RTR3DECL(int) RTS3DeleteKey(RTS3 hS3, const char* pszBucketName, const char* pszKeyName)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN(pS3Int);

    /* Reset the CURL object to an defined state */
    rtS3ReinitCurl(pS3Int);

    char* pszUrl = rtS3Host(pszBucketName, pszKeyName, pS3Int->pszBaseUrl);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_URL, pszUrl);
    RTStrFree(pszUrl);

    /* Create the three basic header entries */
    char *apszHead[3] =
    {
        rtS3HostHeader(pszBucketName, pS3Int->pszBaseUrl), /* Host entry */
        rtS3DateHeader(),                                  /* Date entry */
        NULL                                               /* Authorization entry */
    };
    /* Create the authorization header entry */
    apszHead[RT_ELEMENTS(apszHead)-1] = rtS3CreateAuthHeader(pS3Int, "DELETE", pszBucketName, pszKeyName, apszHead, RT_ELEMENTS(apszHead));

    /* Add all headers to curl */
    struct curl_slist* pHeaders = NULL; /* Init to NULL is important */
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        pHeaders = curl_slist_append(pHeaders, apszHead[i]);

    /* Pass our list of custom made headers */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTPHEADER, pHeaders);

    /* Set CURL in delete mode */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_CUSTOMREQUEST, "DELETE");

    /* Start the request */
    int rc = rtS3Perform(pS3Int);

    /* Regardless of the result, free all used resources first*/
    curl_slist_free_all(pHeaders);
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        RTStrFree(apszHead[i]);

    return rc;
}

RTR3DECL(int) RTS3GetKey(RTS3 hS3, const char *pszBucketName, const char *pszKeyName, const char *pszFilename)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN(pS3Int);

    /* Reset the CURL object to an defined state */
    rtS3ReinitCurl(pS3Int);

    /* Open the file */
    RTFILE hFile;
    int rc = RTFileOpen(&hFile, pszFilename, RTFILE_O_CREATE | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
    if (RT_FAILURE(rc))
        return rc;

    char* pszUrl = rtS3Host(pszBucketName, pszKeyName, pS3Int->pszBaseUrl);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_URL, pszUrl);
    RTStrFree(pszUrl);

    /* Create the three basic header entries */
    char *apszHead[3] =
    {
        rtS3HostHeader(pszBucketName, pS3Int->pszBaseUrl), /* Host entry */
        rtS3DateHeader(),                                  /* Date entry */
        NULL                                               /* Authorization entry */
    };
    /* Create the authorization header entry */
    apszHead[RT_ELEMENTS(apszHead)-1] = rtS3CreateAuthHeader(pS3Int, "GET", pszBucketName, pszKeyName, apszHead, RT_ELEMENTS(apszHead));

    /* Add all headers to curl */
    struct curl_slist* pHeaders = NULL; /* Init to NULL is important */
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        pHeaders = curl_slist_append(pHeaders, apszHead[i]);

    /* Pass our list of custom made headers */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTPHEADER, pHeaders);

    /* Set the callback which receive the content */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_WRITEFUNCTION, rtS3WriteFileCallback);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_WRITEDATA, &hFile);

    /* Start the request */
    rc = rtS3Perform(pS3Int);

    /* Regardless of the result, free all used resources first*/
    curl_slist_free_all(pHeaders);
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        RTStrFree(apszHead[i]);

    /* Close the open file */
    RTFileClose(hFile);

    /* If there was an error delete the newly created file */
    if (RT_FAILURE(rc))
        RTFileDelete(pszFilename);

    return rc;
}

RTR3DECL(int) RTS3PutKey(RTS3 hS3, const char *pszBucketName, const char *pszKeyName, const char *pszFilename)
{
    PRTS3INTERNAL pS3Int = hS3;
    RTS3_VALID_RETURN(pS3Int);

    /* Reset the CURL object to an defined state */
    rtS3ReinitCurl(pS3Int);

    /* Open the file */
    RTFILE hFile;
    int rc = RTFileOpen(&hFile, pszFilename, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_NONE);
    if (RT_FAILURE(rc))
        return rc;

    uint64_t cbFileSize;
    rc = RTFileGetSize(hFile, &cbFileSize);
    if (RT_FAILURE(rc))
    {
        RTFileClose(hFile);
        return rc;
    }

    char* pszUrl = rtS3Host(pszBucketName, pszKeyName, pS3Int->pszBaseUrl);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_URL, pszUrl);
    RTStrFree(pszUrl);

    char* pszContentLength;
    RTStrAPrintf(&pszContentLength, "Content-Length: %lu", cbFileSize);
    /* Create the three basic header entries */
    char *apszHead[5] =
    {
        /** @todo For now we use octet-stream for all types. Later we should try
         * to set the right one (libmagic from the file packet could be a
         * candidate for finding the right type). */
        RTStrDup("Content-Type: octet-stream"),            /* Content type entry */
        pszContentLength,                                  /* Content length entry */
        rtS3HostHeader(pszBucketName, pS3Int->pszBaseUrl), /* Host entry */
        rtS3DateHeader(),                                  /* Date entry */
        NULL                                               /* Authorization entry */
    };
    /* Create the authorization header entry */
    apszHead[RT_ELEMENTS(apszHead)-1] = rtS3CreateAuthHeader(pS3Int, "PUT", pszBucketName, pszKeyName, apszHead, RT_ELEMENTS(apszHead));

    /* Add all headers to curl */
    struct curl_slist* pHeaders = NULL; /* Init to NULL is important */
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        pHeaders = curl_slist_append(pHeaders, apszHead[i]);

    /* Pass our list of custom made headers */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_HTTPHEADER, pHeaders);

    /* Set CURL in upload mode */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_PUT, 1);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_UPLOAD, 1);

    /* Set the size of the file we like to transfer */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_INFILESIZE_LARGE, cbFileSize);

    /* Set the callback which send the content */
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_READFUNCTION, rtS3ReadFileCallback);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_READDATA, &hFile);
    curl_easy_setopt(pS3Int->pCurl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1);

    /* Start the request */
    rc = rtS3Perform(pS3Int);

    /* Regardless of the result, free all used resources first*/
    curl_slist_free_all(pHeaders);
    for(size_t i=0; i < RT_ELEMENTS(apszHead); ++i)
        RTStrFree(apszHead[i]);

    /* Close the open file */
    RTFileClose(hFile);

    return rc;
}


/* $Id: s3.h $ */
/** @file
 * IPRT - Simple Storage Service (S3) Communication API.
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

#ifndef ___iprt_s3_h
#define ___iprt_s3_h

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_s3    RTS3 - Simple Storage Service (S3) Communication API
 * @ingroup grp_rt
 * @{
 */

/** @todo the following three definitions may move the iprt/types.h later. */
/** RTS3 interface handle. */
typedef R3PTRTYPE(struct RTS3INTERNAL *)    RTS3;
/** Pointer to a RTS3 interface handle. */
typedef RTS3                               *PRTS3;
/** Nil RTS3 interface handle. */
#define NIL_RTS3                            ((RTS3)0)


/**
 * S3 progress callback.
 *
 * @returns Reserved, must be 0.
 *
 * @param   uPercent    The process completion percentage.
 * @param   pvUser      The user parameter given to RTS3SetProgressCallback.
 */
typedef DECLCALLBACK(int) FNRTS3PROGRESS(unsigned uPercent, void *pvUser);
/** Pointer to a S3 progress callback. */
typedef FNRTS3PROGRESS *PFNRTS3PROGRESS;


/** Pointer to an S3 bucket entry. */
typedef struct RTS3BUCKETENTRY *PRTS3BUCKETENTRY;
/** Pointer to a const S3 bucket entry. */
typedef struct RTS3BUCKETENTRY const *PCRTS3BUCKETENTRY;
/**
 * RTS3 bucket entry.
 *
 * Represent a bucket of the S3 storage server. Bucket entries are chained as a
 * doubly linked list using the pPrev & pNext member.
 *
 * @todo    Consider making the entire list const unless there are plans for
 *          more APIs using this structure which requires the caller to create
 *          or modify it.
 */
typedef struct RTS3BUCKETENTRY
{
    /** The previous element. */
    PRTS3BUCKETENTRY       pPrev;
    /** The next element. */
    PRTS3BUCKETENTRY       pNext;

    /** The name of the bucket. */
    char const             *pszName;
    /** The creation date of the bucket as string. */
    char const             *pszCreationDate;
} RTS3BUCKETENTRY;


/** Pointer to an S3 key entry. */
typedef struct RTS3KEYENTRY *PRTS3KEYENTRY;
/** Pointer to a const S3 key entry. */
typedef struct RTS3KEYENTRY const *PCRTS3KEYENTRY;
/**
 * RTS3 key entry.
 *
 * Represent a key of the S3 storage server. Key entries are chained as a doubly
 * linked list using the pPrev & pNext member.
 *
 * @todo    Consider making the entire list const unless there are plans for
 *          more APIs using this structure which requires the caller to create
 *          or modify it.
 */
typedef struct RTS3KEYENTRY
{
    /** The previous element. */
    PRTS3KEYENTRY          pPrev;
    /** The next element. */
    PRTS3KEYENTRY          pNext;

    /** The name of the key. */
    char const             *pszName;
    /** The date this key was last modified as string. */
    char const             *pszLastModified;
    /** The size of the file behind this key in bytes. */
    uint64_t                cbFile;
} RTS3KEYENTRY;


/**
 * Creates a RTS3 interface handle.
 *
 * @returns iprt status code.
 *
 * @param   phS3           Where to store the RTS3 handle.
 * @param   pszAccessKey   The access key for the S3 storage server.
 * @param   pszSecretKey   The secret access key for the S3 storage server.
 * @param   pszBaseUrl     The base URL of the S3 storage server.
 * @param   pszUserAgent   An optional user agent string used in the HTTP
 *                         communication.
 */
RTR3DECL(int) RTS3Create(PRTS3 phS3, const char *pszAccessKey, const char *pszSecretKey, const char *pszBaseUrl, const char *pszUserAgent);

/**
 * Destroys a RTS3 interface handle.
 *
 * @returns iprt status code.
 *
 * @param   hS3            Handle to the RTS3 interface.
 */
RTR3DECL(void) RTS3Destroy(RTS3 hS3);

/**
 * Sets an optional progress callback.
 *
 * This callback function will be called when the completion percentage of an S3
 * operation changes.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   pfnProgressCB   The pointer to the progress function.
 * @param   pvUser          The pvUser arg of FNRTS3PROGRESS.
 */
RTR3DECL(void) RTS3SetProgressCallback(RTS3 hS3, PFNRTS3PROGRESS pfnProgressCB, void *pvUser);

/**
 * Gets a list of all available buckets on the S3 storage server.
 *
 * You have to delete ppBuckets after usage with RTS3BucketsDestroy.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   ppBuckets       Where to store the pointer to the head of the
 *                          returned bucket list. Consider the entire list
 *                          read-only.
 */
RTR3DECL(int) RTS3GetBuckets(RTS3 hS3, PCRTS3BUCKETENTRY *ppBuckets);

/**
 * Destroys the bucket list returned by RTS3GetBuckets.
 *
 * @returns iprt status code.
 *
 * @param   pBuckets        Pointer to the first bucket entry.
 */
RTR3DECL(int) RTS3BucketsDestroy(PCRTS3BUCKETENTRY pBuckets);

/**
 * Creates a new bucket on the S3 storage server.
 *
 * This name have to be unique over all accounts on the S3 storage server.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   pszBucketName   Name of the new bucket.
 */
RTR3DECL(int) RTS3CreateBucket(RTS3 hS3, const char *pszBucketName);

/**
 * Deletes a bucket on the S3 storage server.
 *
 * The bucket must be empty.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   pszBucketName   Name of the bucket to delete.
 */
RTR3DECL(int) RTS3DeleteBucket(RTS3 hS3, const char *pszBucketName);

/**
 * Gets a list of all available keys in a bucket on the S3 storage server.
 *
 * You have to delete ppKeys after usage with RTS3KeysDestroy.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   pszBucketName   Name of the bucket to delete.
 * @param   ppKeys          Where to store the pointer to the head of the
 *                          returned key list. Consider the entire list
 *                          read-only.
 */
RTR3DECL(int) RTS3GetBucketKeys(RTS3 hS3, const char *pszBucketName, PCRTS3KEYENTRY *ppKeys);

/**
 * Delete the key list returned by RTS3GetBucketKeys.
 *
 * @returns iprt status code.
 *
 * @param   pKeys           Pointer to the first key entry.
 */
RTR3DECL(int) RTS3KeysDestroy(PCRTS3KEYENTRY pKeys);

/**
 * Deletes a key in a bucket on the S3 storage server.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   pszBucketName   Name of the bucket contains pszKeyName.
 * @param   pszKeyName      Name of the key to delete.
 */
RTR3DECL(int) RTS3DeleteKey(RTS3 hS3, const char *pszBucketName, const char *pszKeyName);

/**
 * Downloads a key from a bucket into a file.
 *
 * The file must not exists.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   pszBucketName   Name of the bucket that contains pszKeyName.
 * @param   pszKeyName      Name of the key to download.
 * @param   pszFilename     Name of the file to store the downloaded key as.
 */
RTR3DECL(int) RTS3GetKey(RTS3 hS3, const char *pszBucketName, const char *pszKeyName, const char *pszFilename);

/**
 * Uploads the content of a file into a key in the specified bucked.
 *
 * @returns iprt status code.
 *
 * @param   hS3             Handle to the RTS3 interface.
 * @param   pszBucketName   Name of the bucket where the new key should be
 *                          created.
 * @param   pszKeyName      Name of the new key.
 * @param   pszFilename     Name of the file to upload the content of.
 */
RTR3DECL(int) RTS3PutKey(RTS3 hS3, const char *pszBucketName, const char *pszKeyName, const char *pszFilename);

/** @} */

RT_C_DECLS_END

#endif


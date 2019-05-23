/* $Id: digest-vfs.cpp $ */
/** @file
 * IPRT - Crypto - Cryptographic Hash / Message Digest API, VFS related interfaces
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
#include "internal/iprt.h"
#include <iprt/crypto/digest.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/vfs.h>


RTDECL(int) RTCrDigestUpdateFromVfsFile(RTCRDIGEST hDigest, RTVFSFILE hVfsFile, bool fRewindFile)
{
    int rc;
    if (fRewindFile)
        rc = RTVfsFileSeek(hVfsFile, 0, RTFILE_SEEK_BEGIN, NULL);
    else
        rc = VINF_SUCCESS;
    if (RT_SUCCESS(rc))
    {
        for (;;)
        {
            char   abBuf[_16K];
            size_t cbRead;
            rc = RTVfsFileRead(hVfsFile, abBuf, sizeof(abBuf), &cbRead);
            if (RT_SUCCESS(rc))
            {
                bool const fEof = rc == VINF_EOF;
                rc = RTCrDigestUpdate(hDigest, abBuf, cbRead);
                if (fEof || RT_FAILURE(rc))
                    break;
            }
            else
            {
                Assert(rc != VERR_EOF);
                break;
            }
        }
    }
    return rc;
}


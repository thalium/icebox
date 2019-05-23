/* $Id: RTFileReadAllByHandleEx-generic.cpp $ */
/** @file
 * IPRT - RTFileReadAllByHandleEx, generic implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <iprt/file.h>
#include "internal/iprt.h"

#include <iprt/mem.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/err.h>


RTDECL(int) RTFileReadAllByHandleEx(RTFILE File, RTFOFF off, RTFOFF cbMax, uint32_t fFlags, void **ppvFile, size_t *pcbFile)
{
    AssertReturn(!(fFlags & ~RTFILE_RDALL_VALID_MASK), VERR_INVALID_PARAMETER);

    /*
     * Save the current offset first.
     */
    RTFOFF offOrg;
    int rc = RTFileSeek(File, 0, RTFILE_SEEK_CURRENT, (uint64_t *)&offOrg);
    if (RT_SUCCESS(rc))
    {
        /*
         * Get the file size, adjust it and check that it might fit into memory.
         */
        RTFOFF cbFile;
        rc = RTFileSeek(File, 0,RTFILE_SEEK_END, (uint64_t *)&cbFile);
        if (RT_SUCCESS(rc))
        {
            RTFOFF cbAllocFile = cbFile > off ? cbFile - off : 0;
            if (cbAllocFile > cbMax)
                cbAllocFile = cbMax;
            size_t cbAllocMem = (size_t)cbAllocFile;
            if ((RTFOFF)cbAllocMem == cbAllocFile)
            {
                /*
                 * Try allocate the required memory and initialize the header (hardcoded fun).
                 */
                void *pvHdr = RTMemAlloc(cbAllocMem + 32 + (fFlags & RTFILE_RDALL_F_TRAILING_ZERO_BYTE ? 1 : 0));
                if (pvHdr)
                {
                    memset(pvHdr, 0xff, 32);
                    *(size_t *)pvHdr = cbAllocMem;

                    /*
                     * Seek and read.
                     */
                    rc = RTFileSeek(File, off, RTFILE_SEEK_BEGIN, NULL);
                    if (RT_SUCCESS(rc))
                    {
                        void *pvFile = (uint8_t *)pvHdr + 32;
                        rc = RTFileRead(File, pvFile, cbAllocMem, NULL);
                        if (RT_SUCCESS(rc))
                        {
                            if (fFlags & RTFILE_RDALL_F_TRAILING_ZERO_BYTE)
                                ((uint8_t *)pvFile)[cbAllocFile] = '\0';

                            /*
                             * Success - fill in the return values.
                             */
                            *ppvFile = pvFile;
                            *pcbFile = cbAllocMem;
                        }
                    }

                    if (RT_FAILURE(rc))
                        RTMemFree(pvHdr);
                }
                else
                    rc = VERR_NO_MEMORY;
            }
            else
                rc = VERR_TOO_MUCH_DATA;
        }
        /* restore the position. */
        RTFileSeek(File, offOrg, RTFILE_SEEK_BEGIN, NULL);
    }
    return rc;
}
RT_EXPORT_SYMBOL(RTFileReadAllByHandleEx);


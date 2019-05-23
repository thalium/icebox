/* $Id: RTKrnlModInfo.cpp $ */
/** @file
 * IPRT - Utility for getting information about loaded kernel modules.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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
#include <iprt/krnlmod.h>

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>



int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    uint32_t cKrnlMods = RTKrnlModLoadedGetCount();
    if (cKrnlMods)
    {
        PRTKRNLMODINFO pahKrnlModInfo = (PRTKRNLMODINFO)RTMemAllocZ(cKrnlMods * sizeof(RTKRNLMODINFO));
        if (pahKrnlModInfo)
        {
            rc = RTKrnlModLoadedQueryInfoAll(pahKrnlModInfo, cKrnlMods, &cKrnlMods);
            if (RT_SUCCESS(rc))
            {
                RTPrintf("Index Load address        Size       Ref count  Name \n");
                for (unsigned i = 0; i < cKrnlMods; i++)
                {
                    RTKRNLMODINFO hKrnlModInfo = pahKrnlModInfo[i];
                    RTPrintf("%5u %#-18RHv  %-10u %-10u %s\n", i,
                             RTKrnlModInfoGetLoadAddr(hKrnlModInfo),
                             RTKrnlModInfoGetSize(hKrnlModInfo),
                             RTKrnlModInfoGetRefCnt(hKrnlModInfo),
                             RTKrnlModInfoGetName(hKrnlModInfo));
                    RTKrnlModInfoRelease(hKrnlModInfo);
                }
            }
            else
                rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Error %Rrc querying kernel modules", rc);

            RTMemFree(pahKrnlModInfo);
        }
        else
            rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Error %Rrc allocating memory", VERR_NO_MEMORY);
    }

    return rcExit;
}


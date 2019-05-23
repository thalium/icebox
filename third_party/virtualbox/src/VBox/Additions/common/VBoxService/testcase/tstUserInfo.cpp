/* $Id: tstUserInfo.cpp $ */
/** @file
 * Test case for correct user environment.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
# include <iprt/win/shlobj.h>
#endif

#include <iprt/initterm.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <VBox/log.h>
#include <VBox/version.h>
#include <VBox/VBoxGuestLib.h>


int main()
{
    /*
     * Init globals and such.
     */
    RTR3InitExeNoArguments(0);

    int rc = VbglR3Init();
    if (RT_FAILURE(rc))
    {
        RTPrintf("VbglR3Init failed with rc=%Rrc.\n", rc);
        return -1;
    }
#ifdef RT_OS_WINDOWS
    WCHAR   wszPath[MAX_PATH];
    HRESULT hRes = SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, wszPath);

    if (SUCCEEDED(hRes))
    {
        RTPrintf("SHGetFolderPathW (CSIDL_APPDATA) = %ls\n", wszPath);
        hRes = SHGetFolderPathW(0, CSIDL_PERSONAL, 0, 0, wszPath);
        if (SUCCEEDED(hRes))
        {
            RTPrintf("SHGetFolderPathW (CSIDL_PERSONAL) = %ls\n", wszPath);
        }
        else
            RTPrintf("SHGetFolderPathW (CSIDL_PERSONAL) returned error: 0x%x\n", hRes);
    }
    else
        RTPrintf("SHGetFolderPathW (CSIDL_APPDATA) returned error: 0x%x\n", hRes);

    if (FAILED(hRes))
        rc = RTErrConvertFromWin32(hRes);

    /* Dump env bits. */
    RTPrintf("Environment:\n\n");
    RTPrintf("APPDATA = %s\n", getenv("APPDATA"));
#endif
    return RT_SUCCESS(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}


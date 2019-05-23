/* $Id: SUPInstall.cpp $ */
/** @file
 * SUPInstall - Driver Install
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
#include <VBox/sup.h>
#include <VBox/err.h>
#include <iprt/initterm.h>
#include <iprt/message.h>


int main(int argc, char **argv)
{
    RTR3InitExeNoArguments(0);
    if (argc != 1)
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "This utility takes no arguments");
    NOREF(argv);

    int rc = SUPR3Install();
    if (RT_SUCCESS(rc))
    {
        if (rc == VINF_SUCCESS)
            RTMsgInfo("Installed successfully!");
        else if (rc == VINF_ALREADY_INITIALIZED)
            RTMsgInfo("Already loaded.");
        else if (rc == VWRN_ALREADY_EXISTS)
            RTMsgInfo("Service already existed; started successfully.");
        else
            RTMsgInfo("Unexpected status: %Rrc", rc);
        return RTEXITCODE_SUCCESS;
    }
    return RTMsgErrorExit(RTEXITCODE_FAILURE, "installation failed. rc=%Rrc", rc);
}


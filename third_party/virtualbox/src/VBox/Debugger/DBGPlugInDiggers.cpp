/* $Id: DBGPlugInDiggers.cpp $ */
/** @file
 * DbfPlugInDiggers - Debugger and Guest OS Digger Plug-in.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGC
#include <VBox/dbg.h>
#include <VBox/vmm/dbgf.h>
#include "DBGPlugIns.h"
#include <VBox/version.h>
#include <VBox/err.h>


DECLEXPORT(int) DbgPlugInEntry(DBGFPLUGINOP enmOperation, PUVM pUVM, uintptr_t uArg)
{
    static PCDBGFOSREG s_aPlugIns[] =
    {
        &g_DBGDiggerDarwin,
        &g_DBGDiggerFreeBsd,
        &g_DBGDiggerLinux,
        &g_DBGDiggerOS2,
        &g_DBGDiggerSolaris,
        &g_DBGDiggerWinNt
    };

    switch (enmOperation)
    {
        case DBGFPLUGINOP_INIT:
        {
            if (uArg != VBOX_VERSION)
                return VERR_VERSION_MISMATCH;

            for (unsigned i = 0; i < RT_ELEMENTS(s_aPlugIns); i++)
            {
                int rc = DBGFR3OSRegister(pUVM, s_aPlugIns[i]);
                if (RT_FAILURE(rc))
                {
                    AssertRC(rc);
                    while (i-- > 0)
                        DBGFR3OSDeregister(pUVM, s_aPlugIns[i]);
                    return rc;
                }
            }
            return VINF_SUCCESS;
        }

        case DBGFPLUGINOP_TERM:
        {
            for (unsigned i = 0; i < RT_ELEMENTS(s_aPlugIns); i++)
            {
                int rc = DBGFR3OSDeregister(pUVM, s_aPlugIns[i]);
                AssertRC(rc);
            }
            return VINF_SUCCESS;
        }

        default:
            return VERR_NOT_SUPPORTED;
    }
}


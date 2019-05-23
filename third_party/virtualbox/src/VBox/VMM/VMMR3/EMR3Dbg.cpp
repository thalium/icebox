/* $Id: EMR3Dbg.cpp $ */
/** @file
 * EM - Execution Monitor / Manager, Debugger Related Bits.
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
#define LOG_GROUP LOG_GROUP_EM
#include <VBox/vmm/em.h>
#include <VBox/dbg.h>
#include "EMInternal.h"


/** @callback_method_impl{FNDBGCCMD,
 * Implements the '.alliem' command. }
 */
static DECLCALLBACK(int) enmR3DbgCmdAllIem(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PUVM pUVM, PCDBGCVAR paArgs, unsigned cArgs)
{
    int  rc;
    bool f;

    if (cArgs == 0)
    {
        rc = EMR3QueryExecutionPolicy(pUVM, EMEXECPOLICY_IEM_ALL, &f);
        if (RT_FAILURE(rc))
            return DBGCCmdHlpFailRc(pCmdHlp, pCmd, rc, "EMR3QueryExecutionPolicy(,EMEXECPOLICY_IEM_ALL,");
        DBGCCmdHlpPrintf(pCmdHlp, f ? "alliem: enabled\n" : "alliem: disabled\n");
    }
    else
    {
        rc = DBGCCmdHlpVarToBool(pCmdHlp, &paArgs[0], &f);
        if (RT_FAILURE(rc))
            return DBGCCmdHlpFailRc(pCmdHlp, pCmd, rc, "DBGCCmdHlpVarToBool");
        rc = EMR3SetExecutionPolicy(pUVM, EMEXECPOLICY_IEM_ALL, f);
        if (RT_FAILURE(rc))
            return DBGCCmdHlpFailRc(pCmdHlp, pCmd, rc, "EMR3SetExecutionPolicy(,EMEXECPOLICY_IEM_ALL,%RTbool)", f);
    }
    return VINF_SUCCESS;
}


/** Describes a optional boolean argument. */
static DBGCVARDESC const g_BoolArg = { 0, 1, DBGCVAR_CAT_ANY, 0, "boolean", "Boolean value." };

/** Commands.  */
static DBGCCMD const g_aCmds[] =
{
    {
        "alliem", 0, 1, &g_BoolArg, 1, 0, enmR3DbgCmdAllIem, "[boolean]",
        "Enables or disabled executing ALL code in IEM, if no arguments are given it displays the current status."
    },
};


int emR3InitDbg(PVM pVM)
{
    RT_NOREF_PV(pVM);
    int rc = VINF_SUCCESS;
#ifdef VBOX_WITH_DEBUGGER
    rc = DBGCRegisterCommands(&g_aCmds[0], RT_ELEMENTS(g_aCmds));
    AssertLogRelRC(rc);
#endif
    return rc;
}


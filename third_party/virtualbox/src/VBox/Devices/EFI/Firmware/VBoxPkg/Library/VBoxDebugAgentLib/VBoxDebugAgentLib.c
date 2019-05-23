/* $Id: VBoxDebugAgentLib.c $ */
/** @file
 * VBox implementation of DebugAgentLib that reports EFI state transitions
 * to DevEFI for debugging purposes.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
#include <Base.h>
#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>
#include "VBoxPkg.h"
#include "../../../../DevEFI.h"


VOID
EFIAPI
InitializeDebugAgent(
  IN UINT32                InitFlag,
  IN VOID                 *Context OPTIONAL,
  IN DEBUG_AGENT_CONTINUE  Function OPTIONAL
  )
{
    /*
     * Do the reporting.
     */
    EFIDBGPOINT enmDbgPoint;
    switch (InitFlag)
    {
        case DEBUG_AGENT_INIT_PREMEM_SEC:   enmDbgPoint = EFIDBGPOINT_SEC_PREMEM;  break;
        case DEBUG_AGENT_INIT_POSTMEM_SEC:  enmDbgPoint = EFIDBGPOINT_SEC_POSTMEM; break;
        case DEBUG_AGENT_INIT_DXE_CORE:     enmDbgPoint = EFIDBGPOINT_DXE_CORE;    break;
        case DEBUG_AGENT_INIT_SMM:          enmDbgPoint = EFIDBGPOINT_SMM;         break;
        case DEBUG_AGENT_INIT_ENTER_SMI:    enmDbgPoint = EFIDBGPOINT_SMI_ENTER;   break;
        case DEBUG_AGENT_INIT_EXIT_SMI:     enmDbgPoint = EFIDBGPOINT_SMI_EXIT;    break;
        case DEBUG_AGENT_INIT_S3:           enmDbgPoint = EFIDBGPOINT_GRAPHICS;    break;
        case DEBUG_AGENT_INIT_DXE_AP:       enmDbgPoint = EFIDBGPOINT_DXE_AP;      break;
        default:
            ASSERT(false);
            enmDbgPoint = EFIDBGPOINT_INVALID;
            break;
    }
    if (enmDbgPoint != EFIDBGPOINT_INVALID)
        ASMOutU32(EFI_PORT_DEBUG_POINT, enmDbgPoint);


    /*
     * Call resume function if supplied.
     */
    if (Function)
        Function(Context);
}


BOOLEAN
EFIAPI
SaveAndSetDebugTimerInterrupt(
  IN BOOLEAN                EnableStatus
  )
{
    NOREF(EnableStatus);
    return FALSE;
}


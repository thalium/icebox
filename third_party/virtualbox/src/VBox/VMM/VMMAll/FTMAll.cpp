/* $Id: FTMAll.cpp $ */
/** @file
 * FTM - Fault Tolerance Manager - All contexts
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
#define LOG_GROUP LOG_GROUP_FTM
#include "FTMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/log.h>

#include <iprt/assert.h>


/**
 * Sets a checkpoint for syncing the state with the standby node
 *
 * @returns VBox status code.
 *
 * @param   pVM         The cross context VM structure.
 * @param   enmType     Checkpoint type
 */
VMM_INT_DECL(int) FTMSetCheckpoint(PVM pVM, FTMCHECKPOINTTYPE enmType)
{
    if (!pVM->fFaultTolerantMaster)
        return VINF_SUCCESS;

#ifdef IN_RING3
    return FTMR3SetCheckpoint(pVM, enmType);
#else
    return VMMRZCallRing3(pVM, VMMGetCpu(pVM), VMMCALLRING3_FTM_SET_CHECKPOINT, enmType);
#endif
}


/**
 * Checks if the delta save/load is enabled
 *
 * @returns true/false
 *
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(bool) FTMIsDeltaLoadSaveActive(PVM pVM)
{
    return pVM->ftm.s.fDeltaLoadSaveActive;
}


/* $Id: TMAllReal.cpp $ */
/** @file
 * TM - Timeout Manager, Real Time, All Contexts.
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
#define LOG_GROUP LOG_GROUP_TM
#include <VBox/vmm/tm.h>
#include "TMInternal.h"
#include <VBox/vmm/vm.h>
#include <iprt/time.h>


/**
 * Gets the current TMCLOCK_REAL time.
 *
 * @returns Real time.
 * @param   pVM             The cross context VM structure.
 */
VMM_INT_DECL(uint64_t) TMRealGet(PVM pVM)
{
    NOREF(pVM);
    return RTTimeMilliTS();
}


/**
 * Gets the frequency of the TMCLOCK_REAL clock.
 *
 * @returns frequency.
 * @param   pVM             The cross context VM structure.
 */
VMM_INT_DECL(uint64_t) TMRealGetFreq(PVM pVM)
{
    NOREF(pVM);
    return TMCLOCK_FREQ_REAL;
}


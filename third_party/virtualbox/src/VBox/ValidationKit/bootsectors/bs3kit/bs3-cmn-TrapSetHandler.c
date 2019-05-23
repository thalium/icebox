/* $Id: bs3-cmn-TrapSetHandler.c $ */
/** @file
 * BS3Kit - Bs3Trap32SetHandler
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#include "bs3kit-template-header.h"


/*********************************************************************************************************************************
*   External Symbols                                                                                                             *
*********************************************************************************************************************************/
extern PFNBS3TRAPHANDLER BS3_DATA_NM(BS3_CMN_NM(g_apfnBs3TrapHandlers))[256];


#undef Bs3TrapSetHandler
BS3_CMN_DEF(PFNBS3TRAPHANDLER, Bs3TrapSetHandler,(uint8_t iIdt, PFNBS3TRAPHANDLER pfnHandler))
{
    PFNBS3TRAPHANDLER pfnOld = BS3_DATA_NM(BS3_CMN_NM(g_apfnBs3TrapHandlers))[iIdt];
    BS3_DATA_NM(BS3_CMN_NM(g_apfnBs3TrapHandlers))[iIdt] = pfnHandler;
    return pfnOld;
}


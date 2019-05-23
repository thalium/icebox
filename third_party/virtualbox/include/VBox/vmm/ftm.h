/** @file
 * FTM - Fault Tolerance Manager.
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

#ifndef ___VBox_vmm_ftm_h
#define ___VBox_vmm_ftm_h

#include <VBox/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_ftm        The Fault Tolerance Monitor / Manager API
 * @ingroup grp_vmm
 * @{
 */

/**
 * Fault tolerance checkpoint type.
 */
typedef enum FTMCHECKPOINTTYPE
{
    FTMCHECKPOINTTYPE_INVALID = 0,
    FTMCHECKPOINTTYPE_NETWORK,
    FTMCHECKPOINTTYPE_STORAGE,
    FTMCHECKPOINTTYPE_END,
    FTMCHECKPOINTTYPE_32BIT_HACK = 0x7fffffff
} FTMCHECKPOINTTYPE;

VMM_INT_DECL(bool)  FTMIsDeltaLoadSaveActive(PVM pVM);
VMM_INT_DECL(int)   FTMSetCheckpoint(PVM pVM, FTMCHECKPOINTTYPE enmType);

#ifdef IN_RING3
/** @defgroup grp_ftm_r3     The FTM Host Context Ring-3 API
 * @{
 */
VMMR3DECL(int)      FTMR3PowerOn(PUVM pUVM, bool fMaster, unsigned uInterval, const char *pszAddress, unsigned uPort, const char *pszPassword);
VMMR3DECL(int)      FTMR3CancelStandby(PUVM pUVM);

VMMR3_INT_DECL(int) FTMR3Init(PVM pVM);
VMMR3_INT_DECL(int) FTMR3Term(PVM pVM);
VMMR3_INT_DECL(int) FTMR3SetCheckpoint(PVM pVM, FTMCHECKPOINTTYPE enmType);

#endif /* IN_RING3 */

/** @} */

/** @} */

RT_C_DECLS_END

#endif


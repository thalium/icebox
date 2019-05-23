/* $Id: symdb.h $ */
/** @file
 * IPRT - Internal Header for the NT Ring-0 Driver Symbol DB.
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

#ifndef ___internal_r0drv_nt_symdb_h
#define ___internal_r0drv_nt_symdb_h

#include <iprt/types.h>


/**
 * NT Version info.
 */
typedef struct RTNTSDBOSVER
{
    /** The major version number. */
    uint8_t     uMajorVer;
    /** The minor version number. */
    uint8_t     uMinorVer;
    /** Set if checked build, clear if free (retail) build. */
    uint8_t     fChecked : 1;
    /** Set if multi processor kernel. */
    uint8_t     fSmp : 1;
    /** The service pack number. */
    uint8_t     uCsdNo : 6;
    /** The build number. */
    uint32_t    uBuildNo;
} RTNTSDBOSVER;
/** Pointer to NT version info. */
typedef RTNTSDBOSVER *PRTNTSDBOSVER;
/** Pointer to const NT version info. */
typedef RTNTSDBOSVER const *PCRTNTSDBOSVER;


/**
 * Compare NT OS version structures.
 *
 * @retval  0 if equal
 * @retval  1 if @a pInfo1 is newer/greater than @a pInfo2
 * @retval  -1 if @a pInfo1 is older/less than @a pInfo2
 *
 * @param   pInfo1              The first version info structure.
 * @param   pInfo2              The second version info structure.
 */
DECLINLINE(int) rtNtOsVerInfoCompare(PCRTNTSDBOSVER pInfo1, PCRTNTSDBOSVER pInfo2)
{
    if (pInfo1->uMajorVer != pInfo2->uMajorVer)
        return pInfo1->uMajorVer > pInfo2->uMajorVer ? 1 : -1;
    if (pInfo1->uMinorVer != pInfo2->uMinorVer)
        return pInfo1->uMinorVer > pInfo2->uMinorVer ? 1 : -1;
    if (pInfo1->uBuildNo  != pInfo2->uBuildNo)
        return pInfo1->uBuildNo  > pInfo2->uBuildNo  ? 1 : -1;
    if (pInfo1->uCsdNo    != pInfo2->uCsdNo)
        return pInfo1->uCsdNo    > pInfo2->uCsdNo    ? 1 : -1;
    if (pInfo1->fSmp      != pInfo2->fSmp)
        return pInfo1->fSmp      > pInfo2->fSmp      ? 1 : -1;
    if (pInfo1->fChecked  != pInfo2->fChecked)
        return pInfo1->fChecked  > pInfo2->fChecked  ? 1 : -1;
    return 0;
}

#endif


/* $Id: interceptor.h $ */
/** @file
 * interceptor.h - universal interceptor builder.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#define TBL_ENTRY(a,b, ign, return_type, params_num, params) static return_type EFIAPI CONCAT(VBOXINTERCEPTOR,b)(PARAMETER(params_num)params);
#include SERVICE_H
#undef TBL_ENTRY


#define TBL_ENTRY(a, b, voidness, return_type, nparams, params) \
    FUNCTION(voidness)(return_type, b, nparams, params)
#include SERVICE_H
#undef TBL_ENTRY




EFI_STATUS INSTALLER(SERVICE)()
{
#define TBL_ENTRY(a,b, ign0, ign1, ign2, ign3)\
do {                                    \
    gThis->CONCAT(SERVICE, Orig).b = ORIG_SERVICE->b;           \
    ORIG_SERVICE->b = CONCAT(VBOXINTERCEPTOR, b);      \
}while (0);
#include SERVICE_H
#undef TBL_ENTRY
    return EFI_SUCCESS;
}

EFI_STATUS UNINSTALLER(SERVICE)()
{
#define TBL_ENTRY(a,b, ign0, ign1, ign2, ign3)\
do {                                    \
    ORIG_SERVICE->b = gThis->CONCAT(SERVICE,Orig).b;           \
}while (0);
#include SERVICE_H
#undef TBL_ENTRY
    return EFI_SUCCESS;
}

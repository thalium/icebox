/* $Id: VBoxGuestR3LibInternal.h $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 support library for the guest additions, Internal header.
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

#ifndef ___VBoxGuestR3LibInternal_h
#define ___VBoxGuestR3LibInternal_h

#include <VBox/VMMDev.h>
#include <VBox/VBoxGuest.h>
#include <VBox/VBoxGuestLib.h>

#ifdef VBOX_VBGLR3_XFREE86
/* Rather than try to resolve all the header file conflicts, I will just
   prototype what we need here. */
typedef unsigned long xf86size_t;
extern "C" xf86size_t xf86strlen(const char*);
# undef strlen
# define strlen xf86strlen
#endif /* VBOX_VBGLR3_XFREE86 */

RT_C_DECLS_BEGIN

int     vbglR3DoIOCtl(uintptr_t uFunction, PVBGLREQHDR pReq, size_t cbReq);
int     vbglR3DoIOCtlRaw(uintptr_t uFunction, PVBGLREQHDR pReq, size_t cbReq);
int     vbglR3GRAlloc(VMMDevRequestHeader **ppReq, size_t cb, VMMDevRequestType enmReqType);
int     vbglR3GRPerform(VMMDevRequestHeader *pReq);
void    vbglR3GRFree(VMMDevRequestHeader *pReq);



DECLINLINE(void) VbglHGCMParmUInt32Set(HGCMFunctionParameter *pParm, uint32_t u32)
{
    pParm->type = VMMDevHGCMParmType_32bit;
    pParm->u.value64 = 0; /* init unused bits to 0 */
    pParm->u.value32 = u32;
}


DECLINLINE(int) VbglHGCMParmUInt32Get(HGCMFunctionParameter *pParm, uint32_t *pu32)
{
    if (pParm->type == VMMDevHGCMParmType_32bit)
    {
        *pu32 = pParm->u.value32;
        return VINF_SUCCESS;
    }
    return VERR_INVALID_PARAMETER;
}


DECLINLINE(void) VbglHGCMParmUInt64Set(HGCMFunctionParameter *pParm, uint64_t u64)
{
    pParm->type      = VMMDevHGCMParmType_64bit;
    pParm->u.value64 = u64;
}


DECLINLINE(int) VbglHGCMParmUInt64Get(HGCMFunctionParameter *pParm, uint64_t *pu64)
{
    if (pParm->type == VMMDevHGCMParmType_64bit)
    {
        *pu64 = pParm->u.value64;
        return VINF_SUCCESS;
    }
    return VERR_INVALID_PARAMETER;
}


DECLINLINE(void) VbglHGCMParmPtrSet(HGCMFunctionParameter *pParm, void *pv, uint32_t cb)
{
    pParm->type                    = VMMDevHGCMParmType_LinAddr;
    pParm->u.Pointer.size          = cb;
    pParm->u.Pointer.u.linearAddr  = (uintptr_t)pv;
}


#ifdef ___iprt_string_h

DECLINLINE(void) VbglHGCMParmPtrSetString(HGCMFunctionParameter *pParm, const char *psz)
{
    pParm->type                    = VMMDevHGCMParmType_LinAddr_In;
    pParm->u.Pointer.size          = (uint32_t)strlen(psz) + 1;
    pParm->u.Pointer.u.linearAddr  = (uintptr_t)psz;
}

#endif /* ___iprt_string_h */

#ifdef VBOX_VBGLR3_XFREE86
# undef strlen
#endif /* VBOX_VBGLR3_XFREE86 */

RT_C_DECLS_END

#endif


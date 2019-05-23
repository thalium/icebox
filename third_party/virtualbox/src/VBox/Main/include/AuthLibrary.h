/* $Id: AuthLibrary.h $ */
/** @file
 * Main - external authentication library interface.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_AUTHLIBRARY
#define ____H_AUTHLIBRARY

#include <VBox/VBoxAuth.h>
#include <iprt/types.h>

typedef struct AUTHLIBRARYCONTEXT
{
    RTLDRMOD hAuthLibrary;
    PAUTHENTRY pfnAuthEntry;
    PAUTHENTRY2 pfnAuthEntry2;
    PAUTHENTRY3 pfnAuthEntry3;
} AUTHLIBRARYCONTEXT;

int AuthLibLoad(AUTHLIBRARYCONTEXT *pAuthLibCtx, const char *pszLibrary);
void AuthLibUnload(AUTHLIBRARYCONTEXT *pAuthLibCtx);

AuthResult AuthLibAuthenticate(const AUTHLIBRARYCONTEXT *pAuthLibCtx,
                               PCRTUUID pUuid, AuthGuestJudgement guestJudgement,
                               const char *pszUser, const char *pszPassword, const char *pszDomain,
                               uint32_t u32ClientId);
void AuthLibDisconnect(const AUTHLIBRARYCONTEXT *pAuthLibCtx, PCRTUUID pUuid, uint32_t u32ClientId);

#endif  /* !____H_AUTHLIBRARY */

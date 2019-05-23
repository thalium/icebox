/* $Id: VBoxUSBFilterMgr.h $ */
/** @file
 * VirtualBox Ring-0 USB Filter Manager.
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

#ifndef ___VBoxUSBFilterMgr_h
#define ___VBoxUSBFilterMgr_h

#include <VBox/usbfilter.h>

RT_C_DECLS_BEGIN

/** @todo r=bird: VBOXUSBFILTER_CONTEXT isn't following the coding
 *        guildlines. Don't know which clueless dude did this...  */
#if defined(RT_OS_WINDOWS)
typedef struct VBOXUSBFLTCTX *VBOXUSBFILTER_CONTEXT;
#define VBOXUSBFILTER_CONTEXT_NIL NULL
#else
typedef RTPROCESS VBOXUSBFILTER_CONTEXT;
#define VBOXUSBFILTER_CONTEXT_NIL NIL_RTPROCESS
#endif

int     VBoxUSBFilterInit(void);
void    VBoxUSBFilterTerm(void);
void    VBoxUSBFilterRemoveOwner(VBOXUSBFILTER_CONTEXT Owner);
int     VBoxUSBFilterAdd(PCUSBFILTER pFilter, VBOXUSBFILTER_CONTEXT Owner, uintptr_t *puId);
int     VBoxUSBFilterRemove(VBOXUSBFILTER_CONTEXT Owner, uintptr_t uId);
VBOXUSBFILTER_CONTEXT VBoxUSBFilterMatch(PCUSBFILTER pDevice, uintptr_t *puId);
VBOXUSBFILTER_CONTEXT VBoxUSBFilterMatchEx(PCUSBFILTER pDevice, uintptr_t *puId, bool fRemoveFltIfOneShot, bool *pfFilter, bool *pfIsOneShot);
VBOXUSBFILTER_CONTEXT VBoxUSBFilterGetOwner(uintptr_t uId);

RT_C_DECLS_END

#endif

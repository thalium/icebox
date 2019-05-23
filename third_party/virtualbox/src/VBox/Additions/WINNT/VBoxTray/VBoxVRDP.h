/* $Id: VBoxVRDP.h $ */
/** @file
 * VBoxVRDP - VRDP notification
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

#ifndef __VBOXSERVICEVRDP__H
#define __VBOXSERVICEVRDP__H

/* The restore service prototypes. */
int                VBoxVRDPInit    (const VBOXSERVICEENV *pEnv, void **ppInstance, bool *pfStartThread);
unsigned __stdcall VBoxVRDPThread  (void *pInstance);
void               VBoxVRDPDestroy (const VBOXSERVICEENV *pEnv, void *pInstance);

#endif /* !__VBOXSERVICEVRDP__H */

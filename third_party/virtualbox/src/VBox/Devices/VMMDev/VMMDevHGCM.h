/* $Id: */
/** @file
 * VBoxDev - HGCM - Host-Guest Communication Manager, internal header.
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

#ifndef ___VMMDev_VMMDevHGCM_h
#define ___VMMDev_VMMDevHGCM_h

#include "VMMDevState.h"

RT_C_DECLS_BEGIN
int vmmdevHGCMConnect(VMMDevState *pVMMDevState, const VMMDevHGCMConnect *pHGCMConnect, RTGCPHYS GCPtr);
int vmmdevHGCMDisconnect(VMMDevState *pVMMDevState, const VMMDevHGCMDisconnect *pHGCMDisconnect, RTGCPHYS GCPtr);
int vmmdevHGCMCall(VMMDevState *pVMMDevState, const VMMDevHGCMCall *pHGCMCall, uint32_t cbHGCMCall, RTGCPHYS GCPtr, VMMDevRequestType enmRequestType);
int vmmdevHGCMCancel(VMMDevState *pVMMDevState, const VMMDevHGCMCancel *pHGCMCancel, RTGCPHYS GCPtr);
int vmmdevHGCMCancel2(VMMDevState *pVMMDevState, RTGCPHYS GCPtr);

DECLCALLBACK(void) hgcmCompleted(PPDMIHGCMPORT pInterface, int32_t result, PVBOXHGCMCMD pCmdPtr);

int vmmdevHGCMSaveState(VMMDevState *pVMMDevState, PSSMHANDLE pSSM);
int vmmdevHGCMLoadState(VMMDevState *pVMMDevState, PSSMHANDLE pSSM, uint32_t u32Version);
int vmmdevHGCMLoadStateDone(VMMDevState *pVMMDevState);

void vmmdevHGCMDestroy(PVMMDEV pThis);
RT_C_DECLS_END

#endif /* !___VMMDev_VMMDevHGCM_h */


/* $Id: VBoxMPCommon.h $ */
/** @file
 * VBox Miniport common functions used by XPDM/WDDM drivers
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOXMPCOMMON_H
#define VBOXMPCOMMON_H

#include "VBoxMPDevExt.h"

RT_C_DECLS_BEGIN

int VBoxMPCmnMapAdapterMemory(PVBOXMP_COMMON pCommon, void **ppv, uint32_t ulOffset, uint32_t ulSize);
void VBoxMPCmnUnmapAdapterMemory(PVBOXMP_COMMON pCommon, void **ppv);

typedef bool(*PFNVIDEOIRQSYNC)(void *);
bool VBoxMPCmnSyncToVideoIRQ(PVBOXMP_COMMON pCommon, PFNVIDEOIRQSYNC pfnSync, void *pvUser);

/* Video modes related */
#ifdef VBOX_XPDM_MINIPORT
void VBoxMPCmnInitCustomVideoModes(PVBOXMP_DEVEXT pExt);
VIDEO_MODE_INFORMATION* VBoxMPCmnGetCustomVideoModeInfo(ULONG ulIndex);
VIDEO_MODE_INFORMATION* VBoxMPCmnGetVideoModeInfo(PVBOXMP_DEVEXT pExt, ULONG ulIndex);
VIDEO_MODE_INFORMATION* VBoxMPXpdmCurrentVideoMode(PVBOXMP_DEVEXT pExt);
ULONG VBoxMPXpdmGetVideoModesCount(PVBOXMP_DEVEXT pExt);
void VBoxMPXpdmBuildVideoModesTable(PVBOXMP_DEVEXT pExt);
#endif

/* Registry access */
#ifdef VBOX_XPDM_MINIPORT
typedef PVBOXMP_DEVEXT VBOXMPCMNREGISTRY;
#else
typedef HANDLE VBOXMPCMNREGISTRY;
#endif

VP_STATUS VBoxMPCmnRegInit(IN PVBOXMP_DEVEXT pExt, OUT VBOXMPCMNREGISTRY *pReg);
VP_STATUS VBoxMPCmnRegFini(IN VBOXMPCMNREGISTRY Reg);
VP_STATUS VBoxMPCmnRegSetDword(IN VBOXMPCMNREGISTRY Reg, PWSTR pName, uint32_t Val);
VP_STATUS VBoxMPCmnRegQueryDword(IN VBOXMPCMNREGISTRY Reg, PWSTR pName, uint32_t *pVal);

/* Pointer related */
bool VBoxMPCmnUpdatePointerShape(PVBOXMP_COMMON pCommon, PVIDEO_POINTER_ATTRIBUTES pAttrs, uint32_t cbLength);

RT_C_DECLS_END

#endif /*VBOXMPCOMMON_H*/

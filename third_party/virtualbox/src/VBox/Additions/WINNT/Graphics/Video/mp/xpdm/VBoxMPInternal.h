/* $Id: VBoxMPInternal.h $ */
/** @file
 * VBox XPDM Miniport internal header
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

#ifndef VBOXMPINTERNAL_H
#define VBOXMPINTERNAL_H

#include "common/VBoxMPUtils.h"
#include "common/VBoxMPDevExt.h"
#include "common/xpdm/VBoxVideoIOCTL.h"

RT_C_DECLS_BEGIN
ULONG DriverEntry(IN PVOID Context1, IN PVOID Context2);
RT_C_DECLS_END

/* ==================== Misc ==================== */
void VBoxSetupVideoPortAPI(PVBOXMP_DEVEXT pExt, PVIDEO_PORT_CONFIG_INFO pConfigInfo);
void VBoxCreateDisplays(PVBOXMP_DEVEXT pExt, PVIDEO_PORT_CONFIG_INFO pConfigInfo);
int VBoxVbvaEnable(PVBOXMP_DEVEXT pExt, BOOLEAN bEnable, VBVAENABLERESULT *pResult);
DECLCALLBACK(void) VBoxMPHGSMIHostCmdCompleteCB(HVBOXVIDEOHGSMI hHGSMI, struct VBVAHOSTCMD RT_UNTRUSTED_VOLATILE_HOST *pCmd);
DECLCALLBACK(int) VBoxMPHGSMIHostCmdRequestCB(HVBOXVIDEOHGSMI hHGSMI, uint8_t u8Channel, uint32_t iDisplay,
                                              struct VBVAHOSTCMD RT_UNTRUSTED_VOLATILE_HOST **ppCmd);
int VBoxVbvaChannelDisplayEnable(PVBOXMP_COMMON pCommon, int iDisplay, uint8_t u8Channel);

/* ==================== System VRP's handlers ==================== */
BOOLEAN VBoxMPSetCurrentMode(PVBOXMP_DEVEXT pExt, PVIDEO_MODE pMode, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPResetDevice(PVBOXMP_DEVEXT pExt, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPMapVideoMemory(PVBOXMP_DEVEXT pExt, PVIDEO_MEMORY pRequestedAddress,
                             PVIDEO_MEMORY_INFORMATION pMapInfo, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPUnmapVideoMemory(PVBOXMP_DEVEXT pExt, PVIDEO_MEMORY VideoMemory, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPShareVideoMemory(PVBOXMP_DEVEXT pExt, PVIDEO_SHARE_MEMORY pShareMem,
                               PVIDEO_SHARE_MEMORY_INFORMATION pShareMemInfo, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPUnshareVideoMemory(PVBOXMP_DEVEXT pExt, PVIDEO_SHARE_MEMORY pMem, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPQueryCurrentMode(PVBOXMP_DEVEXT pExt, PVIDEO_MODE_INFORMATION pModeInfo, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPQueryNumAvailModes(PVBOXMP_DEVEXT pExt, PVIDEO_NUM_MODES pNumModes, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPQueryAvailModes(PVBOXMP_DEVEXT pExt, PVIDEO_MODE_INFORMATION pModes, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPSetColorRegisters(PVBOXMP_DEVEXT pExt, PVIDEO_CLUT pClut, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPSetPointerAttr(PVBOXMP_DEVEXT pExt, PVIDEO_POINTER_ATTRIBUTES pPointerAttrs, uint32_t cbLen, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPEnablePointer(PVBOXMP_DEVEXT pExt, BOOLEAN bEnable, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPQueryPointerPosition(PVBOXMP_DEVEXT pExt, PVIDEO_POINTER_POSITION pPos, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPQueryPointerCapabilities(PVBOXMP_DEVEXT pExt, PVIDEO_POINTER_CAPABILITIES pCaps, PSTATUS_BLOCK pStatus);

/* ==================== Virtual Box VRP's handlers ==================== */
BOOLEAN VBoxMPVBVAEnable(PVBOXMP_DEVEXT pExt, BOOLEAN bEnable, VBVAENABLERESULT *pResult, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPSetVisibleRegion(uint32_t cRects, RTRECT *pRects, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPHGSMIQueryPortProcs(PVBOXMP_DEVEXT pExt, HGSMIQUERYCPORTPROCS *pProcs, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPHGSMIQueryCallbacks(PVBOXMP_DEVEXT pExt, HGSMIQUERYCALLBACKS *pCallbacks, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPQueryHgsmiInfo(PVBOXMP_DEVEXT pExt, QUERYHGSMIRESULT *pResult, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPHgsmiHandlerEnable(PVBOXMP_DEVEXT pExt, HGSMIHANDLERENABLE *pChannel, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPVhwaQueryInfo(PVBOXMP_DEVEXT pExt, VHWAQUERYINFO *pInfo, PSTATUS_BLOCK pStatus);
BOOLEAN VBoxMPQueryRegistryFlags(PVBOXMP_DEVEXT pExt, ULONG *pulFlags, PSTATUS_BLOCK pStatus);

#endif /*VBOXMPINTERNAL_H*/

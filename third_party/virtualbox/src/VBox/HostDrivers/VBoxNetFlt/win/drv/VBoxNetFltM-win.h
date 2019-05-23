/* $Id: VBoxNetFltM-win.h $ */
/** @file
 * VBoxNetFltM-win.h - Bridged Networking Driver, Windows Specific Code - Miniport edge API
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

#ifndef ___VBoxNetFltM_win_h___
#define ___VBoxNetFltM_win_h___

DECLHIDDEN(NDIS_STATUS) vboxNetFltWinMpRegister(PVBOXNETFLTGLOBALS_MP pGlobalsMp, PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPathStr);
DECLHIDDEN(VOID) vboxNetFltWinMpDeregister(PVBOXNETFLTGLOBALS_MP pGlobalsMp);
DECLHIDDEN(VOID) vboxNetFltWinMpReturnPacket(IN NDIS_HANDLE hMiniportAdapterContext, IN PNDIS_PACKET pPacket);

#ifdef VBOXNETADP
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinMpDoInitialization(PVBOXNETFLTINS pThis, NDIS_HANDLE hMiniportAdapter, NDIS_HANDLE hWrapperConfigurationContext);
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinMpDoDeinitialization(PVBOXNETFLTINS pThis);

#else

DECLHIDDEN(NDIS_STATUS) vboxNetFltWinMpInitializeDevideInstance(PVBOXNETFLTINS pThis);
DECLHIDDEN(bool) vboxNetFltWinMpDeInitializeDeviceInstance(PVBOXNETFLTINS pThis, PNDIS_STATUS pStatus);

DECLINLINE(VOID) vboxNetFltWinMpRequestStateComplete(PVBOXNETFLTINS pNetFlt)
{
    RTSpinlockAcquire(pNetFlt->hSpinlock);
    pNetFlt->u.s.WinIf.StateFlags.fRequestInfo = 0;
    RTSpinlockRelease(pNetFlt->hSpinlock);
}

DECLHIDDEN(NDIS_STATUS) vboxNetFltWinMpRequestPost(PVBOXNETFLTINS pNetFlt);
#endif

#endif /* !___VBoxNetFltM_win_h___ */


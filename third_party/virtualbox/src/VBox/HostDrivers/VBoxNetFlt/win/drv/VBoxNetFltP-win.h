/* $Id: VBoxNetFltP-win.h $ */
/** @file
 * VBoxNetFltP-win.h - Bridged Networking Driver, Windows Specific Code.
 * Protocol edge API
 */
/*
 * Copyright (C) 2011-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef ___VBoxNetFltP_win_h___
#define ___VBoxNetFltP_win_h___

#ifdef VBOXNETADP
# error "No protocol edge"
#endif
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtRegister(PVBOXNETFLTGLOBALS_PT pGlobalsPt, PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPathStr);
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtDeregister(PVBOXNETFLTGLOBALS_PT pGlobalsPt);
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtDoUnbinding(PVBOXNETFLTINS pNetFlt, bool bOnUnbind);
DECLHIDDEN(VOID) vboxNetFltWinPtRequestComplete(NDIS_HANDLE hContext, PNDIS_REQUEST pNdisRequest, NDIS_STATUS Status);
DECLHIDDEN(bool) vboxNetFltWinPtCloseInterface(PVBOXNETFLTINS pNetFlt, PNDIS_STATUS pStatus);
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtDoBinding(PVBOXNETFLTINS pThis, PNDIS_STRING pOurDeviceName, PNDIS_STRING pBindToDeviceName);
#endif /* #ifndef ___VBoxNetFltP_win_h___ */

/* $Id: HGCM.h $ */
/** @file
 * HGCM - Host-Guest Communication Manager.
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

#ifndef __HGCM_h__
#define __HGCM_h__

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/pdmifs.h>

#include <VBox/VMMDev.h>
#include <VBox/hgcmsvc.h>

/* HGCM saved state version */
#define HGCM_SSM_VERSION    2

/* Handle of a HGCM service extension. */
struct _HGCMSVCEXTHANDLEDATA;
typedef struct _HGCMSVCEXTHANDLEDATA *HGCMSVCEXTHANDLE;

RT_C_DECLS_BEGIN
int HGCMHostInit (void);
int HGCMHostShutdown (void);

int HGCMHostReset (void);

int HGCMHostLoad (const char *pszServiceLibrary, const char *pszServiceName);

int HGCMHostRegisterServiceExtension (HGCMSVCEXTHANDLE *pHandle, const char *pszServiceName, PFNHGCMSVCEXT pfnExtension, void *pvExtension);
void HGCMHostUnregisterServiceExtension (HGCMSVCEXTHANDLE handle);

int HGCMGuestConnect (PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmdPtr, const char *pszServiceName, uint32_t *pClientID);
int HGCMGuestDisconnect (PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmdPtr, uint32_t clientID);
int HGCMGuestCall (PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmdPtr, uint32_t clientID, uint32_t function, uint32_t cParms, VBOXHGCMSVCPARM *paParms);

int HGCMHostCall (const char *pszServiceName, uint32_t function, uint32_t cParms, VBOXHGCMSVCPARM aParms[]);

#ifdef VBOX_WITH_CRHGSMI
int HGCMHostSvcHandleCreate (const char *pszServiceName, HGCMCVSHANDLE * phSvc);
int HGCMHostSvcHandleDestroy (HGCMCVSHANDLE hSvc);
int HGCMHostFastCallAsync (HGCMCVSHANDLE hSvc, uint32_t function, PVBOXHGCMSVCPARM pParm, PHGCMHOSTFASTCALLCB pfnCompletion, void *pvCompletion);
#endif

int HGCMHostSaveState (PSSMHANDLE pSSM);
int HGCMHostLoadState (PSSMHANDLE pSSM);

RT_C_DECLS_END

#endif /* __HGCM_h__ */

/* $Id: VBoxNetCfg-win.h $ */
/** @file
 * Network Configuration API for Windows platforms.
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

#ifndef ___VBox_VBoxNetCfg_win_h
#define ___VBox_VBoxNetCfg_win_h

/*
 * Defining VBOXNETCFG_DELAYEDRENAME postpones renaming of host-only adapter
 * connection during adapter creation after it has been assigned with an
 * IP address. This hopefully prevents collisions that may happen when we
 * attempt to rename a connection too early, while its configuration is
 * still being 'committed' by the network setup engine.
 */
#define VBOXNETCFG_DELAYEDRENAME

#include <iprt/win/winsock2.h>
#include <iprt/win/windows.h>
#include <Netcfgn.h>
#include <iprt/win/Setupapi.h>
#include <VBox/cdefs.h>
#include <iprt/types.h>

/** @defgroup grp_vboxnetcfgwin     The Windows Network Configration Library
 * @{ */

/** @def VBOXNETCFGWIN_DECL
 * The usual declaration wrapper.
 */
#if 0
/* enable this in case we include this in a dll*/
# ifdef IN_VBOXDDU
#  define VBOXNETCFGWIN_DECL(a_Type) DECLEXPORT(a_Type)
# else
#  define VBOXNETCFGWIN_DECL(a_Type) DECLIMPORT(a_Type)
# endif
#else
/*enable this in case we include this in a static lib*/
# define VBOXNETCFGWIN_DECL(a_Type) a_Type VBOXCALL
#endif

RT_C_DECLS_BEGIN

VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinQueryINetCfg(OUT INetCfg **ppNetCfg,
                          IN BOOL fGetWriteLock,
                          IN LPCWSTR pszwClientDescription,
                          IN DWORD cmsTimeout,
                          OUT LPWSTR *ppszwClientDescription);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinReleaseINetCfg(IN INetCfg *pNetCfg, IN BOOL fHasWriteLock);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinGetComponentByGuid(IN INetCfg *pNc, IN const GUID *pguidClass,
                                                            IN const GUID * pComponentGuid, OUT INetCfgComponent **ppncc);

VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinNetFltInstall(IN INetCfg *pNc, IN LPCWSTR const * apInfFullPaths, IN UINT cInfFullPaths);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinNetFltUninstall(IN INetCfg *pNc);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinNetLwfInstall(IN INetCfg *pNc, IN LPCWSTR const pInfFullPath);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinNetLwfUninstall(IN INetCfg *pNc);

VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinNetAdpUninstall(IN INetCfg *pNc, IN LPCWSTR pwszId);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinNetAdpInstall(IN INetCfg *pNc,IN LPCWSTR const pInfFullPath);

#ifndef VBOXNETCFG_DELAYEDRENAME
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinCreateHostOnlyNetworkInterface(IN LPCWSTR pInfPath, IN bool bIsInfPathFile,
                                                                        OUT GUID *pGuid, OUT BSTR *lppszName,
                                                                        OUT BSTR *pErrMsg);
#else /* VBOXNETCFG_DELAYEDRENAME */
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinCreateHostOnlyNetworkInterface(IN LPCWSTR pInfPath, IN bool bIsInfPathFile,
                                                                        OUT GUID *pGuid, OUT BSTR *lppszId,
                                                                        OUT BSTR *pErrMsg);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinRenameHostOnlyConnection(IN const GUID *pGuid, IN LPCWSTR pszId,  OUT BSTR *pDevName);
#endif /* VBOXNETCFG_DELAYEDRENAME */
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinUpdateHostOnlyNetworkInterface(LPCWSTR pcsxwInf, BOOL *pbRebootRequired, LPCWSTR pcsxwId);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinRemoveHostOnlyNetworkInterface(IN const GUID *pGUID, OUT BSTR *pErrMsg);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinRemoveAllNetDevicesOfId(IN LPCWSTR lpszPnPId);

typedef enum
{
    VBOXNECTFGWINPROPCHANGE_TYPE_UNDEFINED = 0,
    VBOXNECTFGWINPROPCHANGE_TYPE_DISABLE,
    VBOXNECTFGWINPROPCHANGE_TYPE_ENABLE
} VBOXNECTFGWINPROPCHANGE_TYPE;

VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinPropChangeAllNetDevicesOfId(IN LPCWSTR lpszPnPId, VBOXNECTFGWINPROPCHANGE_TYPE enmPcType);

VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinGenHostOnlyNetworkNetworkIp(OUT PULONG pNetIp, OUT PULONG pNetMask);

typedef struct ADAPTER_SETTINGS
{
    ULONG ip;
    ULONG mask;
    BOOL bDhcp;
} ADAPTER_SETTINGS, *PADAPTER_SETTINGS; /**< I'm not prefixed */

VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinEnableStaticIpConfig(IN const GUID *pGuid, IN ULONG ip, IN ULONG mask);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinGetAdapterSettings(IN const GUID * pGuid, OUT PADAPTER_SETTINGS pSettings);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinEnableDynamicIpConfig(IN const GUID *pGuid);
VBOXNETCFGWIN_DECL(HRESULT) VBoxNetCfgWinDhcpRediscover(IN const GUID *pGuid);


typedef VOID (*LOG_ROUTINE)(LPCSTR szString); /**< I'm not prefixed. */
VBOXNETCFGWIN_DECL(VOID) VBoxNetCfgWinSetLogging(IN LOG_ROUTINE pfnLog);

RT_C_DECLS_END

/** @} */

#endif


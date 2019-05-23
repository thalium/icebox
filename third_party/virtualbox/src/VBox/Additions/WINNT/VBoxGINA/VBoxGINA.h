/* $Id: VBoxGINA.h $ */
/** @file
 * VBoxGINA - Windows Logon DLL for VirtualBox.
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

#ifndef __VBOXGINA_H__
#define __VBOXGINA_H__

/** Handle to Winlogon service */
extern HANDLE hGinaWlx;
/** Winlog function dispatch table */
extern PWLX_DISPATCH_VERSION_1_1 pWlxFuncs;


/** @name GINA entry point calls
 * @{
 */
typedef BOOL (WINAPI *PGWLXNEGOTIATE)(DWORD, DWORD*);
typedef BOOL (WINAPI *PGWLXINITIALIZE)(LPWSTR, HANDLE, PVOID, PVOID, PVOID*);
typedef VOID (WINAPI *PGWLXDISPLAYSASNOTICE)(PVOID);
typedef int  (WINAPI *PGWLXLOGGEDOUTSAS)(PVOID, DWORD, PLUID, PSID, PDWORD,
                                        PHANDLE, PWLX_MPR_NOTIFY_INFO, PVOID*);
typedef BOOL (WINAPI *PGWLXACTIVATEUSERSHELL)(PVOID, PWSTR, PWSTR, PVOID);
typedef int  (WINAPI *PGWLXLOGGEDONSAS)(PVOID, DWORD, PVOID);
typedef VOID (WINAPI *PGWLXDISPLAYLOCKEDNOTICE)(PVOID);
typedef int  (WINAPI *PGWLXWKSTALOCKEDSAS)(PVOID, DWORD);
typedef BOOL (WINAPI *PGWLXISLOCKOK)(PVOID);
typedef BOOL (WINAPI *PGWLXISLOGOFFOK)(PVOID);
typedef VOID (WINAPI *PGWLXLOGOFF)(PVOID);
typedef VOID (WINAPI *PGWLXSHUTDOWN)(PVOID, DWORD);
/* 1.1 calls */
typedef BOOL (WINAPI *PGWLXSCREENSAVERNOTIFY)(PVOID, BOOL*);
typedef BOOL (WINAPI *PGWLXSTARTAPPLICATION)(PVOID, PWSTR, PVOID, PWSTR);
/* 1.3 calls */
typedef BOOL (WINAPI *PGWLXNETWORKPROVIDERLOAD)(PVOID, PWLX_MPR_NOTIFY_INFO);
typedef BOOL (WINAPI *PGWLXDISPLAYSTATUSMESSAGE)(PVOID, HDESK, DWORD, PWSTR, PWSTR);
typedef BOOL (WINAPI *PGWLXGETSTATUSMESSAGE)(PVOID, DWORD*, PWSTR, DWORD);
typedef BOOL (WINAPI *PGWLXREMOVESTATUSMESSAGE)(PVOID);
/* 1.4 calls */
typedef BOOL (WINAPI *PGWLXGETCONSOLESWITCHCREDENTIALS)(PVOID, PVOID);
typedef VOID (WINAPI *PGWLXRECONNECTNOTIFY)(PVOID);
typedef VOID (WINAPI *PGWLXDISCONNECTNOTIFY)(PVOID);
/** @}  */

#endif /* !__VBOXGINA_H__ */


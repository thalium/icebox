/* $Id: com.cpp $ */
/** @file
 * MS COM / XPCOM Abstraction Layer
 */

/*
 * Copyright (C) 2005-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_MAIN
#if !defined(VBOX_WITH_XPCOM)

# include <iprt/win/objbase.h>

#else /* !defined (VBOX_WITH_XPCOM) */
# include <stdlib.h>
# include <nsCOMPtr.h>
# include <nsIServiceManagerUtils.h>
# include <nsIComponentManager.h>
# include <ipcIService.h>
# include <ipcCID.h>
# include <ipcIDConnectService.h>
# include <nsIInterfaceInfo.h>
# include <nsIInterfaceInfoManager.h>
# define IPC_DCONNECTSERVICE_CONTRACTID "@mozilla.org/ipc/dconnect-service;1" // official XPCOM headers don't define it yet
#endif /* !defined (VBOX_WITH_XPCOM) */

#include "VBox/com/com.h"
#include "VBox/com/assert.h"

#include "VBox/com/Guid.h"
#include "VBox/com/array.h"

#include <iprt/string.h>

#include <VBox/err.h>
#include <VBox/log.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
namespace com
{
/* static */
const Guid Guid::Empty; /* default ctor is OK */

#if defined (VBOX_WITH_XPCOM)

/* static */
const nsID *SafeGUIDArray::nsIDRef::Empty = (const nsID *)Guid::Empty.raw();

#endif /* (VBOX_WITH_XPCOM) */



void GetInterfaceNameByIID(const GUID &aIID, BSTR *aName)
{
    AssertPtrReturnVoid(aName);
    *aName = NULL;

#if !defined(VBOX_WITH_XPCOM)

    LONG rc;
    LPOLESTR iidStr = NULL;
    if (StringFromIID(aIID, &iidStr) == S_OK)
    {
        HKEY ifaceKey;
        rc = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Interface",
                           0, KEY_QUERY_VALUE, &ifaceKey);
        if (rc == ERROR_SUCCESS)
        {
            HKEY iidKey;
            rc = RegOpenKeyExW(ifaceKey, iidStr, 0, KEY_QUERY_VALUE, &iidKey);
            if (rc == ERROR_SUCCESS)
            {
                /* determine the size and type */
                DWORD sz, type;
                rc = RegQueryValueExW(iidKey, NULL, NULL, &type, NULL, &sz);
                if (rc == ERROR_SUCCESS && type == REG_SZ)
                {
                    /* query the value to BSTR */
                    *aName = SysAllocStringLen(NULL, (sz + 1) / sizeof(TCHAR) + 1);
                    rc = RegQueryValueExW(iidKey, NULL, NULL, NULL, (LPBYTE) *aName, &sz);
                    if (rc != ERROR_SUCCESS)
                    {
                        SysFreeString(*aName);
                        *aName = NULL;
                    }
                }
                RegCloseKey(iidKey);
            }
            RegCloseKey(ifaceKey);
        }
        CoTaskMemFree(iidStr);
    }

#else /* !defined (VBOX_WITH_XPCOM) */

    nsresult rv;
    nsCOMPtr<nsIInterfaceInfoManager> iim =
        do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsIInterfaceInfo> iinfo;
        rv = iim->GetInfoForIID(&aIID, getter_AddRefs(iinfo));
        if (NS_SUCCEEDED(rv))
        {
            const char *iname = NULL;
            iinfo->GetNameShared(&iname);
            char *utf8IName = NULL;
            if (RT_SUCCESS(RTStrCurrentCPToUtf8(&utf8IName, iname)))
            {
                PRTUTF16 utf16IName = NULL;
                if (RT_SUCCESS(RTStrToUtf16(utf8IName, &utf16IName)))
                {
                    *aName = SysAllocString((OLECHAR *) utf16IName);
                    RTUtf16Free(utf16IName);
                }
                RTStrFree(utf8IName);
            }
        }
    }

#endif /* !defined (VBOX_WITH_XPCOM) */
}

#ifdef VBOX_WITH_XPCOM

HRESULT GlueCreateObjectOnServer(const CLSID &clsid,
                                 const char *serverName,
                                 const nsIID &id,
                                 void** ppobj)
{
    HRESULT rc;
    nsCOMPtr<ipcIService> ipcServ = do_GetService(IPC_SERVICE_CONTRACTID, &rc);
    if (SUCCEEDED(rc))
    {
        PRUint32 serverID = 0;
        rc = ipcServ->ResolveClientName(serverName, &serverID);
        if (SUCCEEDED (rc))
        {
            nsCOMPtr<ipcIDConnectService> dconServ = do_GetService(IPC_DCONNECTSERVICE_CONTRACTID, &rc);
            if (SUCCEEDED(rc))
                rc = dconServ->CreateInstance(serverID,
                                              clsid,
                                              id,
                                              ppobj);
        }
    }
    return rc;
}

HRESULT GlueCreateInstance(const CLSID &clsid,
                           const nsIID &id,
                           void** ppobj)
{
    nsCOMPtr<nsIComponentManager> manager;
    HRESULT rc = NS_GetComponentManager(getter_AddRefs(manager));
    if (SUCCEEDED(rc))
        rc = manager->CreateInstance(clsid,
                                     nsnull,
                                     id,
                                     ppobj);
    return rc;
}

#endif // VBOX_WITH_XPCOM

} /* namespace com */


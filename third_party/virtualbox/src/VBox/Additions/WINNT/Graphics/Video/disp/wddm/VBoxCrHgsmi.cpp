/* $Id: VBoxCrHgsmi.cpp $ */
/** @file
 * VBoxVideo Display D3D User mode dll
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

#include <VBoxCrHgsmi.h>
#include <iprt/err.h>

#include "VBoxUhgsmiKmt.h"

static VBOXDISPKMT_CALLBACKS g_VBoxCrHgsmiKmtCallbacks;
static int g_bVBoxKmtCallbacksInited = 0;

static int vboxCrHgsmiInitPerform(VBOXDISPKMT_CALLBACKS *pCallbacks)
{
    HRESULT hr = vboxDispKmtCallbacksInit(pCallbacks);
    /*Assert(hr == S_OK);*/
    if (hr == S_OK)
    {
        /* check if we can create the hgsmi */
        PVBOXUHGSMI pHgsmi = VBoxCrHgsmiCreate();
        if (pHgsmi)
        {
            /* yes, we can, so this is wddm mode */
            VBoxCrHgsmiDestroy(pHgsmi);
            Log(("CrHgsmi: WDDM mode supported\n"));
            return 1;
        }
        vboxDispKmtCallbacksTerm(pCallbacks);
    }
    Log(("CrHgsmi: unsupported\n"));
    return -1;
}

VBOXCRHGSMI_DECL(int) VBoxCrHgsmiInit(void)
{
    if (!g_bVBoxKmtCallbacksInited)
    {
        g_bVBoxKmtCallbacksInited = vboxCrHgsmiInitPerform(&g_VBoxCrHgsmiKmtCallbacks);
        Assert(g_bVBoxKmtCallbacksInited);
    }

    return g_bVBoxKmtCallbacksInited > 0 ? VINF_SUCCESS : VERR_NOT_SUPPORTED;
}

VBOXCRHGSMI_DECL(PVBOXUHGSMI) VBoxCrHgsmiCreate()
{
    PVBOXUHGSMI_PRIVATE_KMT pHgsmiGL = (PVBOXUHGSMI_PRIVATE_KMT)RTMemAllocZ(sizeof (*pHgsmiGL));
    if (pHgsmiGL)
    {
        HRESULT hr = vboxUhgsmiKmtCreate(pHgsmiGL, TRUE /* bD3D tmp for injection thread*/);
        Log(("CrHgsmi: faled to create KmtEsc VBOXUHGSMI instance, hr (0x%x)\n", hr));
        if (hr == S_OK)
        {
            return &pHgsmiGL->BasePrivate.Base;
        }
        RTMemFree(pHgsmiGL);
    }

    return NULL;
}

VBOXCRHGSMI_DECL(void) VBoxCrHgsmiDestroy(PVBOXUHGSMI pHgsmi)
{
    PVBOXUHGSMI_PRIVATE_KMT pHgsmiGL = VBOXUHGSMIKMT_GET(pHgsmi);
    HRESULT hr = vboxUhgsmiKmtDestroy(pHgsmiGL);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        RTMemFree(pHgsmiGL);
    }
}

VBOXCRHGSMI_DECL(int) VBoxCrHgsmiTerm()
{
#if 0
    PVBOXUHGSMI_PRIVATE_KMT pHgsmiGL = gt_pHgsmiGL;
    if (pHgsmiGL)
    {
        g_VBoxCrHgsmiCallbacks.pfnClientDestroy(pHgsmiGL->BasePrivate.hClient);
        vboxUhgsmiKmtDestroy(pHgsmiGL);
        gt_pHgsmiGL = NULL;
    }

    if (g_pfnVBoxDispCrHgsmiTerm)
        g_pfnVBoxDispCrHgsmiTerm();
#endif
    if (g_bVBoxKmtCallbacksInited > 0)
    {
        vboxDispKmtCallbacksTerm(&g_VBoxCrHgsmiKmtCallbacks);
    }
    return VINF_SUCCESS;
}

VBOXCRHGSMI_DECL(int) VBoxCrHgsmiCtlConGetClientID(PVBOXUHGSMI pHgsmi, uint32_t *pu32ClientID)
{
    PVBOXUHGSMI_PRIVATE_BASE pHgsmiPrivate = (PVBOXUHGSMI_PRIVATE_BASE)pHgsmi;
    int rc = vboxCrHgsmiPrivateCtlConGetClientID(pHgsmiPrivate, pu32ClientID);
    if (!RT_SUCCESS(rc))
    {
        WARN(("vboxCrHgsmiPrivateCtlConGetClientID failed with rc (%d)", rc));
    }
    return rc;
}

VBOXCRHGSMI_DECL(int) VBoxCrHgsmiCtlConGetHostCaps(PVBOXUHGSMI pHgsmi, uint32_t *pu32HostCaps)
{
    PVBOXUHGSMI_PRIVATE_BASE pHgsmiPrivate = (PVBOXUHGSMI_PRIVATE_BASE)pHgsmi;
    int rc = vboxCrHgsmiPrivateCtlConGetHostCaps(pHgsmiPrivate, pu32HostCaps);
    if (!RT_SUCCESS(rc))
    {
        WARN(("vboxCrHgsmiPrivateCtlConGetHostCaps failed with rc (%d)", rc));
    }
    return rc;
}

VBOXCRHGSMI_DECL(int) VBoxCrHgsmiCtlConCall(PVBOXUHGSMI pHgsmi, struct VBGLIOCHGCMCALL *pCallInfo, int cbCallInfo)
{
    PVBOXUHGSMI_PRIVATE_BASE pHgsmiPrivate = (PVBOXUHGSMI_PRIVATE_BASE)pHgsmi;
    int rc = vboxCrHgsmiPrivateCtlConCall(pHgsmiPrivate, pCallInfo, cbCallInfo);
    if (!RT_SUCCESS(rc))
    {
        WARN(("VBoxCrHgsmiPrivateCtlConCall failed with rc (%d)", rc));
    }
    return rc;
}

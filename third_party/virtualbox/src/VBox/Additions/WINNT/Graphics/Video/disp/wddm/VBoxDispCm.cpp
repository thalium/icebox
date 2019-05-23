/* $Id: VBoxDispCm.cpp $ */
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

#include "VBoxDispD3DCmn.h"
#include "VBoxDispD3D.h"
#include <VBoxDisplay.h>

#include <iprt/list.h>

#ifdef VBOX_WITH_CROGL
#include <cr_protocol.h>
#endif

typedef struct VBOXDISPCM_SESSION
{
    HANDLE hEvent;
    CRITICAL_SECTION CritSect;
    /** List of VBOXWDDMDISP_CONTEXT nodes. */
    RTLISTANCHOR CtxList;
    bool bQueryMp;
} VBOXDISPCM_SESSION, *PVBOXDISPCM_SESSION;

typedef struct VBOXDISPCM_MGR
{
    VBOXDISPCM_SESSION Session;
} VBOXDISPCM_MGR, *PVBOXDISPCM_MGR;

/* the cm is one per process */
static VBOXDISPCM_MGR g_pVBoxCmMgr;

HRESULT vboxDispCmSessionTerm(PVBOXDISPCM_SESSION pSession)
{
#ifdef DEBUG_misha
    Assert(RTListIsEmpty(&pSession->CtxList));
#endif
    BOOL bRc = CloseHandle(pSession->hEvent);
    Assert(bRc);
    if (bRc)
    {
        DeleteCriticalSection(&pSession->CritSect);
        return S_OK;
    }
    DWORD winEr = GetLastError();
    HRESULT hr = HRESULT_FROM_WIN32(winEr);
    return hr;
}

HRESULT vboxDispCmSessionInit(PVBOXDISPCM_SESSION pSession)
{
    HANDLE hEvent = CreateEvent(NULL,
            FALSE, /* BOOL bManualReset */
            FALSE, /* BOOL bInitialState */
            NULL /* LPCTSTR lpName */
            );
    Assert(hEvent);
    if (hEvent)
    {
        pSession->hEvent = hEvent;
        InitializeCriticalSection(&pSession->CritSect);
        RTListInit(&pSession->CtxList);
        pSession->bQueryMp = false;
        return S_OK;
    }
    DWORD winEr = GetLastError();
    HRESULT hr = HRESULT_FROM_WIN32(winEr);
    return hr;
}

void vboxDispCmSessionCtxAdd(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_CONTEXT pContext)
{
    EnterCriticalSection(&pSession->CritSect);
    RTListAppend(&pSession->CtxList, &pContext->ListNode);
    LeaveCriticalSection(&pSession->CritSect);
}

void vboxDispCmSessionCtxRemoveLocked(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_CONTEXT pContext)
{
    RT_NOREF(pSession);
    RTListNodeRemove(&pContext->ListNode);
}

void vboxDispCmSessionCtxRemove(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_CONTEXT pContext)
{
    EnterCriticalSection(&pSession->CritSect);
    vboxDispCmSessionCtxRemoveLocked(pSession, pContext);
    LeaveCriticalSection(&pSession->CritSect);
}

HRESULT vboxDispCmInit()
{
    HRESULT hr = vboxDispCmSessionInit(&g_pVBoxCmMgr.Session);
    Assert(hr == S_OK);
    return hr;
}

HRESULT vboxDispCmTerm()
{
    HRESULT hr = vboxDispCmSessionTerm(&g_pVBoxCmMgr.Session);
    Assert(hr == S_OK);
    return hr;
}

HRESULT vboxDispCmCtxCreate(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_CONTEXT pContext)
{
    BOOL fIsCrContext;
    VBOXWDDM_CREATECONTEXT_INFO Info = {0};
    Info.u32IfVersion = pDevice->u32IfVersion;
    if (VBOXDISPMODE_IS_3D(pDevice->pAdapter))
    {
        Info.enmType = VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D;
#ifdef VBOX_WITH_CROGL
        Info.crVersionMajor = CR_PROTOCOL_VERSION_MAJOR;
        Info.crVersionMinor = CR_PROTOCOL_VERSION_MINOR;
#else
        WARN(("not expected"));
        Info.crVersionMajor = 0;
        Info.crVersionMinor = 0;
#endif
        fIsCrContext = TRUE;
    }
    else
    {
        Info.enmType = VBOXWDDM_CONTEXT_TYPE_CUSTOM_2D;
        fIsCrContext = FALSE;
    }
    Info.hUmEvent = (uintptr_t)g_pVBoxCmMgr.Session.hEvent;
    Info.u64UmInfo = (uintptr_t)pContext;

    if (VBOXDISPMODE_IS_3D(pDevice->pAdapter))
    {
        pContext->ContextInfo.NodeOrdinal = VBOXWDDM_NODE_ID_3D;
        pContext->ContextInfo.EngineAffinity = VBOXWDDM_ENGINE_ID_3D;
    }
    else
    {
        pContext->ContextInfo.NodeOrdinal = VBOXWDDM_NODE_ID_2D_VIDEO;
        pContext->ContextInfo.EngineAffinity = VBOXWDDM_ENGINE_ID_2D_VIDEO;
    }
    pContext->ContextInfo.Flags.Value = 0;
    pContext->ContextInfo.pPrivateDriverData = &Info;
    pContext->ContextInfo.PrivateDriverDataSize = sizeof (Info);
    pContext->ContextInfo.hContext = 0;
    pContext->ContextInfo.pCommandBuffer = NULL;
    pContext->ContextInfo.CommandBufferSize = 0;
    pContext->ContextInfo.pAllocationList = NULL;
    pContext->ContextInfo.AllocationListSize = 0;
    pContext->ContextInfo.pPatchLocationList = NULL;
    pContext->ContextInfo.PatchLocationListSize = 0;

    HRESULT hr = S_OK;
    hr = pDevice->RtCallbacks.pfnCreateContextCb(pDevice->hDevice, &pContext->ContextInfo);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        Assert(pContext->ContextInfo.hContext);
        pContext->ContextInfo.pPrivateDriverData = NULL;
        pContext->ContextInfo.PrivateDriverDataSize = 0;
        vboxDispCmSessionCtxAdd(&g_pVBoxCmMgr.Session, pContext);
        pContext->pDevice = pDevice;
        if (fIsCrContext)
        {
#ifdef VBOX_WITH_CRHGSMI
            if (pDevice->pAdapter->u32VBox3DCaps & CR_VBOX_CAP_CMDVBVA)
                vboxUhgsmiD3DInit(&pDevice->Uhgsmi, pDevice);
            else
                vboxUhgsmiD3DEscInit(&pDevice->Uhgsmi, pDevice);
#endif
        }
    }
    else
    {
        exit(1);
    }

    return hr;
}

HRESULT vboxDispCmSessionCtxDestroy(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_CONTEXT pContext)
{
    EnterCriticalSection(&pSession->CritSect);
    Assert(pContext->ContextInfo.hContext);
    D3DDDICB_DESTROYCONTEXT DestroyContext;
    Assert(pDevice->DefaultContext.ContextInfo.hContext);
    DestroyContext.hContext = pDevice->DefaultContext.ContextInfo.hContext;
    HRESULT hr = pDevice->RtCallbacks.pfnDestroyContextCb(pDevice->hDevice, &DestroyContext);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        vboxDispCmSessionCtxRemoveLocked(pSession, pContext);
    }
    LeaveCriticalSection(&pSession->CritSect);
    return hr;
}

HRESULT vboxDispCmCtxDestroy(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_CONTEXT pContext)
{
    return vboxDispCmSessionCtxDestroy(&g_pVBoxCmMgr.Session, pDevice, pContext);
}

static HRESULT vboxDispCmSessionCmdQueryData(PVBOXDISPCM_SESSION pSession, PVBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD pCmd, uint32_t cbCmd)
{

    HRESULT hr = S_OK;
    D3DDDICB_ESCAPE DdiEscape;
    DdiEscape.Flags.Value = 0;
    DdiEscape.pPrivateDriverData = pCmd;
    DdiEscape.PrivateDriverDataSize = cbCmd;

    pCmd->EscapeHdr.escapeCode = VBOXESC_GETVBOXVIDEOCMCMD;

    PVBOXWDDMDISP_CONTEXT pContext = NULL, pCurCtx;

    /* lock to ensure the context is not destroyed */
    EnterCriticalSection(&pSession->CritSect);
    /* use any context for identifying the kernel CmSession. We're using the first one */
    RTListForEach(&pSession->CtxList, pCurCtx, VBOXWDDMDISP_CONTEXT, ListNode)
    {
        PVBOXWDDMDISP_DEVICE pDevice = pCurCtx->pDevice;
        if (VBOXDISPMODE_IS_3D(pDevice->pAdapter))
        {
            pContext = pCurCtx;
            break;
        }
    }
    if (pContext)
    {
        PVBOXWDDMDISP_DEVICE pDevice = pContext->pDevice;
        DdiEscape.hDevice = pDevice->hDevice;
        DdiEscape.hContext = pContext->ContextInfo.hContext;
        Assert (DdiEscape.hContext);
        Assert (DdiEscape.hDevice);
        hr = pDevice->RtCallbacks.pfnEscapeCb(pDevice->pAdapter->hAdapter, &DdiEscape);
        LeaveCriticalSection(&pSession->CritSect);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            if (!pCmd->Hdr.cbCmdsReturned && !pCmd->Hdr.cbRemainingFirstCmd)
                hr = S_FALSE;
        }
        else
        {
            vboxVDbgPrint(("DispD3D: vboxDispCmSessionCmdQueryData, pfnEscapeCb failed hr (0x%x)\n", hr));
            exit(1);
        }
    }
    else
    {
        LeaveCriticalSection(&pSession->CritSect);
        hr = S_FALSE;
    }

    return hr;
}

HRESULT vboxDispCmCmdSessionInterruptWait(PVBOXDISPCM_SESSION pSession)
{
    SetEvent(pSession->hEvent);
    return S_OK;
}

HRESULT vboxDispCmSessionCmdGet(PVBOXDISPCM_SESSION pSession, PVBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD pCmd, uint32_t cbCmd, DWORD dwMilliseconds)
{
    Assert(cbCmd >= sizeof (VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD));
    if (cbCmd < sizeof (VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD))
        return E_INVALIDARG;

    do
    {

        if (pSession->bQueryMp)
        {
            HRESULT hr = vboxDispCmSessionCmdQueryData(pSession, pCmd, cbCmd);
            Assert(hr == S_OK || hr == S_FALSE);
            if (hr == S_OK || hr != S_FALSE)
            {
                return hr;
            }

            pSession->bQueryMp = false;
        }

        DWORD dwResult = WaitForSingleObject(pSession->hEvent, dwMilliseconds);
        switch(dwResult)
        {
            case WAIT_OBJECT_0:
            {
                pSession->bQueryMp = true;
                break; /* <- query commands */
            }
            case WAIT_TIMEOUT:
            {
                Assert(!pSession->bQueryMp);
                return WAIT_TIMEOUT;
            }
            default:
                Assert(0);
                return E_FAIL;
        }
    } while (1);

    /* should never be here */
    Assert(0);
    return E_FAIL;
}

HRESULT vboxDispCmCmdGet(PVBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD pCmd, uint32_t cbCmd, DWORD dwMilliseconds)
{
    return vboxDispCmSessionCmdGet(&g_pVBoxCmMgr.Session, pCmd, cbCmd, dwMilliseconds);
}

HRESULT vboxDispCmCmdInterruptWait()
{
    return vboxDispCmCmdSessionInterruptWait(&g_pVBoxCmMgr.Session);
}

void vboxDispCmLog(LPCSTR pszMsg)
{
    RT_NOREF(pszMsg);
    PVBOXDISPCM_SESSION pSession = &g_pVBoxCmMgr.Session;

    EnterCriticalSection(&pSession->CritSect);
    /* use any context for identifying the kernel CmSession. We're using the first one */
    PVBOXWDDMDISP_CONTEXT pContext = RTListGetFirst(&pSession->CtxList, VBOXWDDMDISP_CONTEXT, ListNode);
    Assert(pContext);
    if (pContext)
    {
        Assert(pContext->pDevice);
        vboxVDbgPrint(("%s", pszMsg));
    }
    LeaveCriticalSection(&pSession->CritSect);
}

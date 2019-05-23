/* $Id: VBoxDispDbg.cpp $ */
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

#include <stdio.h>
#include <stdarg.h>

#include <iprt/asm.h>
#include <iprt/assert.h>

static DWORD g_VBoxVDbgFIsModuleNameInited = 0;
static char g_VBoxVDbgModuleName[MAX_PATH];

char *vboxVDbgDoGetModuleName()
{
    if (!g_VBoxVDbgFIsModuleNameInited)
    {
        DWORD cName = GetModuleFileNameA(NULL, g_VBoxVDbgModuleName, RT_ELEMENTS(g_VBoxVDbgModuleName));
        if (!cName)
        {
#ifdef LOG_ENABLED
            DWORD winEr = GetLastError();
#endif
            WARN(("GetModuleFileNameA failed, winEr %d", winEr));
            return NULL;
        }
        g_VBoxVDbgFIsModuleNameInited = TRUE;
    }
    return g_VBoxVDbgModuleName;
}

static void vboxDispLogDbgFormatStringV(char * szBuffer, uint32_t cbBuffer, const char * szString, va_list pArgList)
{
    uint32_t cbWritten = sprintf(szBuffer, "['%s' 0x%x.0x%x] Disp: ", vboxVDbgDoGetModuleName(), GetCurrentProcessId(), GetCurrentThreadId());
    if (cbWritten > cbBuffer)
    {
        AssertReleaseFailed();
        return;
    }

    _vsnprintf(szBuffer + cbWritten, cbBuffer - cbWritten, szString, pArgList);
}

#if defined(VBOXWDDMDISP_DEBUG) || defined(VBOX_WDDMDISP_WITH_PROFILE)
LONG g_VBoxVDbgFIsDwm = -1;

DWORD g_VBoxVDbgPid = 0;

DWORD g_VBoxVDbgFLogRel = 1;
# if !defined(VBOXWDDMDISP_DEBUG)
DWORD g_VBoxVDbgFLog = 0;
# else
DWORD g_VBoxVDbgFLog = 1;
# endif
DWORD g_VBoxVDbgFLogFlow = 0;

#endif

#ifdef VBOXWDDMDISP_DEBUG

# ifndef IN_VBOXCRHGSMI
#define VBOXWDDMDISP_DEBUG_DUMP_DEFAULT 0
DWORD g_VBoxVDbgFDumpSetTexture = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpDrawPrim = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpTexBlt = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpBlt = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpRtSynch = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpFlush = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpShared = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpLock = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpUnlock = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpPresentEnter = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpPresentLeave = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFDumpScSync = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;

DWORD g_VBoxVDbgFBreakShared = VBOXWDDMDISP_DEBUG_DUMP_DEFAULT;
DWORD g_VBoxVDbgFBreakDdi = 0;

DWORD g_VBoxVDbgFCheckSysMemSync = 0;
DWORD g_VBoxVDbgFCheckBlt = 0;
DWORD g_VBoxVDbgFCheckTexBlt = 0;
DWORD g_VBoxVDbgFCheckScSync = 0;

DWORD g_VBoxVDbgFSkipCheckTexBltDwmWndUpdate = 1;

DWORD g_VBoxVDbgCfgMaxDirectRts = 3;
DWORD g_VBoxVDbgCfgForceDummyDevCreate = 0;

PVBOXWDDMDISP_DEVICE g_VBoxVDbgInternalDevice = NULL;
PVBOXWDDMDISP_RESOURCE g_VBoxVDbgInternalRc = NULL;

DWORD g_VBoxVDbgCfgCreateSwapchainOnDdiOnce = 0;

void vboxDispLogDbgPrintF(char * szString, ...)
{
    char szBuffer[4096] = {0};
    va_list pArgList;
    va_start(pArgList, szString);
    vboxDispLogDbgFormatStringV(szBuffer, sizeof (szBuffer), szString, pArgList);
    va_end(pArgList);

    OutputDebugStringA(szBuffer);
}

VOID vboxVDbgDoPrintDmlCmd(const char* pszDesc, const char* pszCmd)
{
    vboxVDbgPrint(("<?dml?><exec cmd=\"%s\">%s</exec>, ( %s )\n", pszCmd, pszDesc, pszCmd));
}

VOID vboxVDbgDoPrintDumpCmd(const char* pszDesc, const void *pvData, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch)
{
    char Cmd[1024];
    sprintf(Cmd, "!vbvdbg.ms 0x%p 0n%d 0n%d 0n%d 0n%d", pvData, width, height, bpp, pitch);
    vboxVDbgDoPrintDmlCmd(pszDesc, Cmd);
}

VOID vboxVDbgDoPrintLopLastCmd(const char* pszDesc)
{
    vboxVDbgDoPrintDmlCmd(pszDesc, "ed @@(&vboxVDbgLoop) 0");
}

typedef struct VBOXVDBG_DUMP_INFO
{
    DWORD fFlags;
    const VBOXWDDMDISP_ALLOCATION *pAlloc;
    IDirect3DResource9 *pD3DRc;
    const RECT *pRect;
} VBOXVDBG_DUMP_INFO, *PVBOXVDBG_DUMP_INFO;

typedef DECLCALLBACK(void) FNVBOXVDBG_CONTENTS_DUMPER(PVBOXVDBG_DUMP_INFO pInfo, BOOLEAN fBreak, void *pvDumper);
typedef FNVBOXVDBG_CONTENTS_DUMPER *PFNVBOXVDBG_CONTENTS_DUMPER;

static VOID vboxVDbgDoDumpSummary(const char * pPrefix, PVBOXVDBG_DUMP_INFO pInfo, const char * pSuffix)
{
    const VBOXWDDMDISP_ALLOCATION *pAlloc = pInfo->pAlloc;
    IDirect3DResource9 *pD3DRc = pInfo->pD3DRc;
    char rectBuf[24];
    if (pInfo->pRect)
        _snprintf(rectBuf, sizeof(rectBuf) / sizeof(rectBuf[0]), "(%d:%d);(%d:%d)",
                pInfo->pRect->left, pInfo->pRect->top,
                pInfo->pRect->right, pInfo->pRect->bottom);
    else
        strcpy(rectBuf, "n/a");

    vboxVDbgPrint(("%s Sh(0x%p), Rc(0x%p), pAlloc(0x%x), pD3DIf(0x%p), Type(%s), Rect(%s), Locks(%d) %s",
                    pPrefix ? pPrefix : "",
                    pAlloc ? pAlloc->pRc->aAllocations[0].hSharedHandle : NULL,
                    pAlloc ? pAlloc->pRc : NULL,
                    pAlloc,
                    pD3DRc,
                    pD3DRc ? vboxDispLogD3DRcType(pD3DRc->GetType()) : "n/a",
                    rectBuf,
                    pAlloc ? pAlloc->LockInfo.cLocks : 0,
                    pSuffix ? pSuffix : ""));
}

VOID vboxVDbgDoDumpPerform(const char * pPrefix, PVBOXVDBG_DUMP_INFO pInfo, const char * pSuffix,
        PFNVBOXVDBG_CONTENTS_DUMPER pfnCd, void *pvCd)
{
    DWORD fFlags = pInfo->fFlags;

    if (!VBOXVDBG_DUMP_TYPE_ENABLED_FOR_INFO(pInfo, fFlags))
        return;

    if (!pInfo->pD3DRc && pInfo->pAlloc)
        pInfo->pD3DRc = (IDirect3DResource9*)pInfo->pAlloc->pD3DIf;

    BOOLEAN bLogOnly = VBOXVDBG_DUMP_TYPE_FLOW_ONLY(fFlags);
    if (bLogOnly || !pfnCd)
    {
        vboxVDbgDoDumpSummary(pPrefix, pInfo, pSuffix);
        if (VBOXVDBG_DUMP_FLAGS_IS_SET(fFlags, VBOXVDBG_DUMP_TYPEF_BREAK_ON_FLOW)
                || (!bLogOnly && VBOXVDBG_DUMP_FLAGS_IS_CLEARED(fFlags, VBOXVDBG_DUMP_TYPEF_DONT_BREAK_ON_CONTENTS)))
            Assert(0);
        return;
    }

    vboxVDbgDoDumpSummary(pPrefix, pInfo, NULL);

    pfnCd(pInfo, VBOXVDBG_DUMP_FLAGS_IS_CLEARED(fFlags, VBOXVDBG_DUMP_TYPEF_DONT_BREAK_ON_CONTENTS), pvCd);

    if (pSuffix && pSuffix[0] != '\0')
        vboxVDbgPrint(("%s", pSuffix));
}

static DECLCALLBACK(void) vboxVDbgAllocRectContentsDumperCb(PVBOXVDBG_DUMP_INFO pInfo, BOOLEAN fBreak, void *pvDumper)
{
    RT_NOREF(fBreak, pvDumper);
    const VBOXWDDMDISP_ALLOCATION *pAlloc = pInfo->pAlloc;
    const RECT *pRect = pInfo->pRect;

    Assert(pAlloc->hAllocation);

    D3DDDICB_LOCK LockData;
    LockData.hAllocation = pAlloc->hAllocation;
    LockData.PrivateDriverData = 0;
    LockData.NumPages = 0;
    LockData.pPages = NULL;
    LockData.pData = NULL; /* out */
    LockData.Flags.Value = 0;
    LockData.Flags.LockEntire =1;
    LockData.Flags.ReadOnly = 1;

    PVBOXWDDMDISP_DEVICE pDevice = pAlloc->pRc->pDevice;

    HRESULT hr = pDevice->RtCallbacks.pfnLockCb(pDevice->hDevice, &LockData);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        UINT bpp = vboxWddmCalcBitsPerPixel(pAlloc->SurfDesc.format);
        vboxVDbgDoPrintDumpCmd("Surf Info", LockData.pData, pAlloc->SurfDesc.d3dWidth, pAlloc->SurfDesc.height, bpp, pAlloc->SurfDesc.pitch);
        if (pRect)
        {
            Assert(pRect->right > pRect->left);
            Assert(pRect->bottom > pRect->top);
            vboxVDbgDoPrintRect("rect: ", pRect, "\n");
            vboxVDbgDoPrintDumpCmd("Rect Info", ((uint8_t*)LockData.pData) + (pRect->top * pAlloc->SurfDesc.pitch) + ((pRect->left * bpp) >> 3),
                    pRect->right - pRect->left, pRect->bottom - pRect->top, bpp, pAlloc->SurfDesc.pitch);
        }
        Assert(0);

        D3DDDICB_UNLOCK DdiUnlock;

        DdiUnlock.NumAllocations = 1;
        DdiUnlock.phAllocations = &pAlloc->hAllocation;

        hr = pDevice->RtCallbacks.pfnUnlockCb(pDevice->hDevice, &DdiUnlock);
        Assert(hr == S_OK);
    }
}

VOID vboxVDbgDoDumpAllocRect(const char * pPrefix, PVBOXWDDMDISP_ALLOCATION pAlloc, RECT *pRect, const char* pSuffix, DWORD fFlags)
{
    VBOXVDBG_DUMP_INFO Info;
    Info.fFlags = fFlags;
    Info.pAlloc = pAlloc;
    Info.pD3DRc = NULL;
    Info.pRect = pRect;
    vboxVDbgDoDumpPerform(pPrefix, &Info, pSuffix, vboxVDbgAllocRectContentsDumperCb, NULL);
}

static DECLCALLBACK(void) vboxVDbgRcRectContentsDumperCb(PVBOXVDBG_DUMP_INFO pInfo, BOOLEAN fBreak, void *pvDumper)
{
    RT_NOREF(pvDumper);
    const VBOXWDDMDISP_ALLOCATION *pAlloc = pInfo->pAlloc;
    const RECT *pRect = pInfo->pRect;
    IDirect3DSurface9 *pSurf;
    HRESULT hr = VBoxD3DIfSurfGet(pAlloc->pRc, pAlloc->iAlloc, &pSurf);
    if (hr != S_OK)
    {
        WARN(("VBoxD3DIfSurfGet failed, hr 0x%x", hr));
        return;
    }

    D3DSURFACE_DESC Desc;
    hr = pSurf->GetDesc(&Desc);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        D3DLOCKED_RECT Lr;
        hr = pSurf->LockRect(&Lr, NULL, D3DLOCK_READONLY);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            UINT bpp = vboxWddmCalcBitsPerPixel((D3DDDIFORMAT)Desc.Format);
            vboxVDbgDoPrintDumpCmd("Surf Info", Lr.pBits, Desc.Width, Desc.Height, bpp, Lr.Pitch);
            if (pRect)
            {
                Assert(pRect->right > pRect->left);
                Assert(pRect->bottom > pRect->top);
                vboxVDbgDoPrintRect("rect: ", pRect, "\n");
                vboxVDbgDoPrintDumpCmd("Rect Info", ((uint8_t*)Lr.pBits) + (pRect->top * Lr.Pitch) + ((pRect->left * bpp) >> 3),
                        pRect->right - pRect->left, pRect->bottom - pRect->top, bpp, Lr.Pitch);
            }

            if (fBreak)
            {
                Assert(0);
            }
            hr = pSurf->UnlockRect();
            Assert(hr == S_OK);
        }
    }

    pSurf->Release();
}

VOID vboxVDbgDoDumpRcRect(const char * pPrefix, PVBOXWDDMDISP_ALLOCATION pAlloc,
        IDirect3DResource9 *pD3DRc, RECT *pRect, const char * pSuffix, DWORD fFlags)
{
    VBOXVDBG_DUMP_INFO Info;
    Info.fFlags = fFlags;
    Info.pAlloc = pAlloc;
    Info.pD3DRc = pD3DRc;
    Info.pRect = pRect;
    vboxVDbgDoDumpPerform(pPrefix, &Info, pSuffix, vboxVDbgRcRectContentsDumperCb, NULL);
}

VOID vboxVDbgDoDumpBb(const char * pPrefix, IDirect3DSwapChain9 *pSwapchainIf, const char * pSuffix, DWORD fFlags)
{
    IDirect3DSurface9 *pBb = NULL;
    HRESULT hr = pSwapchainIf->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBb);
    Assert(hr == S_OK);
    if (FAILED(hr))
    {
        return;
    }

    Assert(pBb);
    vboxVDbgDoDumpRcRect(pPrefix, NULL, pBb, NULL, pSuffix, fFlags);
    pBb->Release();
}

VOID vboxVDbgDoDumpFb(const char * pPrefix, IDirect3DSwapChain9 *pSwapchainIf, const char * pSuffix, DWORD fFlags)
{
    IDirect3DSurface9 *pBb = NULL;
    HRESULT hr = pSwapchainIf->GetBackBuffer(~(UINT)0, D3DBACKBUFFER_TYPE_MONO, &pBb);
    Assert(hr == S_OK);
    if (FAILED(hr))
    {
        return;
    }

    Assert(pBb);
    vboxVDbgDoDumpRcRect(pPrefix, NULL, pBb, NULL, pSuffix, fFlags);
    pBb->Release();
}


#define VBOXVDBG_STRCASE(_t) \
        case _t: return #_t;
#define VBOXVDBG_STRCASE_UNKNOWN() \
        default: Assert(0); return "Unknown";

const char* vboxVDbgStrCubeFaceType(D3DCUBEMAP_FACES enmFace)
{
    switch (enmFace)
    {
    VBOXVDBG_STRCASE(D3DCUBEMAP_FACE_POSITIVE_X);
    VBOXVDBG_STRCASE(D3DCUBEMAP_FACE_NEGATIVE_X);
    VBOXVDBG_STRCASE(D3DCUBEMAP_FACE_POSITIVE_Y);
    VBOXVDBG_STRCASE(D3DCUBEMAP_FACE_NEGATIVE_Y);
    VBOXVDBG_STRCASE(D3DCUBEMAP_FACE_POSITIVE_Z);
    VBOXVDBG_STRCASE(D3DCUBEMAP_FACE_NEGATIVE_Z);
    VBOXVDBG_STRCASE_UNKNOWN();
    }
}

VOID vboxVDbgDoDumpRt(const char * pPrefix, PVBOXWDDMDISP_DEVICE pDevice, const char * pSuffix, DWORD fFlags)
{
    for (UINT i = 0; i < pDevice->cRTs; ++i)
    {
        IDirect3DSurface9 *pRt;
        PVBOXWDDMDISP_ALLOCATION pAlloc = pDevice->apRTs[i];
        if (!pAlloc) continue;
        IDirect3DDevice9 *pDeviceIf = pDevice->pDevice9If;
        HRESULT hr = pDeviceIf->GetRenderTarget(i, &pRt);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
//            Assert(pAlloc->pD3DIf == pRt);
            vboxVDbgDoDumpRcRect(pPrefix, pAlloc, NULL, NULL, pSuffix, fFlags);
            pRt->Release();
        }
        else
        {
            vboxVDbgPrint((__FUNCTION__": ERROR getting rt: 0x%x", hr));
        }
    }
}

VOID vboxVDbgDoDumpSamplers(const char * pPrefix, PVBOXWDDMDISP_DEVICE pDevice, const char * pSuffix, DWORD fFlags)
{
    for (UINT i = 0, iSampler = 0; iSampler < pDevice->cSamplerTextures; ++i)
    {
        Assert(i < RT_ELEMENTS(pDevice->aSamplerTextures));
        if (!pDevice->aSamplerTextures[i]) continue;
        PVBOXWDDMDISP_RESOURCE pRc = pDevice->aSamplerTextures[i];
        for (UINT j = 0; j < pRc->cAllocations; ++j)
        {
            PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[j];
            vboxVDbgDoDumpRcRect(pPrefix, pAlloc, NULL, NULL, pSuffix, fFlags);
        }
        ++iSampler;
    }
}

static DECLCALLBACK(void) vboxVDbgLockUnlockSurfTexContentsDumperCb(PVBOXVDBG_DUMP_INFO pInfo, BOOLEAN fBreak, void *pvDumper)
{
    RT_NOREF(pvDumper);
    const VBOXWDDMDISP_ALLOCATION *pAlloc = pInfo->pAlloc;
    const RECT *pRect = pInfo->pRect;
    UINT bpp = vboxWddmCalcBitsPerPixel(pAlloc->SurfDesc.format);
    uint32_t width, height, pitch = 0;
    void *pvData;
    if (pAlloc->LockInfo.fFlags.AreaValid)
    {
        width = pAlloc->LockInfo.Area.left - pAlloc->LockInfo.Area.right;
        height = pAlloc->LockInfo.Area.bottom - pAlloc->LockInfo.Area.top;
    }
    else
    {
        width = pAlloc->SurfDesc.width;
        height = pAlloc->SurfDesc.height;
    }

    if (pAlloc->LockInfo.fFlags.NotifyOnly)
    {
        pitch = pAlloc->SurfDesc.pitch;
        pvData = ((uint8_t*)pAlloc->pvMem) + pitch*pRect->top + ((bpp*pRect->left) >> 3);
    }
    else
    {
        pvData = pAlloc->LockInfo.pvData;
    }

    vboxVDbgDoPrintDumpCmd("Surf Info", pvData, width, height, bpp, pitch);

    if (fBreak)
    {
        Assert(0);
    }
}

VOID vboxVDbgDoDumpLockUnlockSurfTex(const char * pPrefix, const VBOXWDDMDISP_ALLOCATION *pAlloc, const char * pSuffix, DWORD fFlags)
{
    Assert(!pAlloc->hSharedHandle);

    RECT Rect;
    const RECT *pRect;
    Assert(!pAlloc->LockInfo.fFlags.RangeValid);
    Assert(!pAlloc->LockInfo.fFlags.BoxValid);
    if (pAlloc->LockInfo.fFlags.AreaValid)
    {
        pRect = &pAlloc->LockInfo.Area;
    }
    else
    {
        Rect.top = 0;
        Rect.bottom = pAlloc->SurfDesc.height;
        Rect.left = 0;
        Rect.right = pAlloc->SurfDesc.width;
        pRect = &Rect;
    }

    VBOXVDBG_DUMP_INFO Info;
    Info.fFlags = fFlags;
    Info.pAlloc = pAlloc;
    Info.pD3DRc = NULL;
    Info.pRect = pRect;
    vboxVDbgDoDumpPerform(pPrefix, &Info, pSuffix, vboxVDbgLockUnlockSurfTexContentsDumperCb, NULL);
}

VOID vboxVDbgDoDumpLockSurfTex(const char * pPrefix, const D3DDDIARG_LOCK* pData, const char * pSuffix, DWORD fFlags)
{
    const VBOXWDDMDISP_RESOURCE *pRc = (const VBOXWDDMDISP_RESOURCE*)pData->hResource;
    const VBOXWDDMDISP_ALLOCATION *pAlloc = &pRc->aAllocations[pData->SubResourceIndex];
#ifdef VBOXWDDMDISP_DEBUG
    VBOXWDDMDISP_ALLOCATION *pUnconstpAlloc = (VBOXWDDMDISP_ALLOCATION *)pAlloc;
    pUnconstpAlloc->LockInfo.pvData = pData->pSurfData;
#endif
    vboxVDbgDoDumpLockUnlockSurfTex(pPrefix, pAlloc, pSuffix, fFlags);
}

VOID vboxVDbgDoDumpUnlockSurfTex(const char * pPrefix, const D3DDDIARG_UNLOCK* pData, const char * pSuffix, DWORD fFlags)
{
    const VBOXWDDMDISP_RESOURCE *pRc = (const VBOXWDDMDISP_RESOURCE*)pData->hResource;
    const VBOXWDDMDISP_ALLOCATION *pAlloc = &pRc->aAllocations[pData->SubResourceIndex];
    vboxVDbgDoDumpLockUnlockSurfTex(pPrefix, pAlloc, pSuffix, fFlags);
}

BOOL vboxVDbgDoCheckLRects(D3DLOCKED_RECT *pDstLRect, const RECT *pDstRect, D3DLOCKED_RECT *pSrcLRect, const RECT *pSrcRect, DWORD bpp, BOOL fBreakOnMismatch)
{
    LONG DstH, DstW, SrcH, SrcW, DstWBytes;
    BOOL fMatch = FALSE;
    DstH = pDstRect->bottom - pDstRect->top;
    DstW = pDstRect->right - pDstRect->left;
    SrcH = pSrcRect->bottom - pSrcRect->top;
    SrcW = pSrcRect->right - pSrcRect->left;

    DstWBytes = ((DstW * bpp + 7) >> 3);

    if(DstW != SrcW && DstH != SrcH)
    {
        WARN(("stretched comparison not supported!!"));
        return FALSE;
    }

    uint8_t *pDst = (uint8_t*)pDstLRect->pBits;
    uint8_t *pSrc = (uint8_t*)pSrcLRect->pBits;
    for (LONG i = 0; i < DstH; ++i)
    {
        if (!(fMatch = !memcmp(pDst, pSrc, DstWBytes)))
        {
            vboxVDbgPrint(("not match!\n"));
            if (fBreakOnMismatch)
                Assert(0);
            break;
        }
        pDst += pDstLRect->Pitch;
        pSrc += pSrcLRect->Pitch;
    }
    return fMatch;
}

BOOL vboxVDbgDoCheckRectsMatch(const VBOXWDDMDISP_RESOURCE *pDstRc, uint32_t iDstAlloc,
                            const VBOXWDDMDISP_RESOURCE *pSrcRc, uint32_t iSrcAlloc,
                            const RECT *pDstRect,
                            const RECT *pSrcRect,
                            BOOL fBreakOnMismatch)
{
    BOOL fMatch = FALSE;
    RECT DstRect = {0}, SrcRect = {0};
    if (!pDstRect)
    {
        DstRect.left = 0;
        DstRect.right = pDstRc->aAllocations[iDstAlloc].SurfDesc.width;
        DstRect.top = 0;
        DstRect.bottom = pDstRc->aAllocations[iDstAlloc].SurfDesc.height;
        pDstRect = &DstRect;
    }

    if (!pSrcRect)
    {
        SrcRect.left = 0;
        SrcRect.right = pSrcRc->aAllocations[iSrcAlloc].SurfDesc.width;
        SrcRect.top = 0;
        SrcRect.bottom = pSrcRc->aAllocations[iSrcAlloc].SurfDesc.height;
        pSrcRect = &SrcRect;
    }

    if (pDstRc == pSrcRc
            && iDstAlloc == iSrcAlloc)
    {
        if (!memcmp(pDstRect, pSrcRect, sizeof (*pDstRect)))
        {
            vboxVDbgPrint(("matching same rect of one allocation, skipping..\n"));
            return TRUE;
        }
        WARN(("matching different rects of the same allocation, unsupported!"));
        return FALSE;
    }

    if (pDstRc->RcDesc.enmFormat != pSrcRc->RcDesc.enmFormat)
    {
        WARN(("matching different formats, unsupported!"));
        return FALSE;
    }

    DWORD bpp = pDstRc->aAllocations[iDstAlloc].SurfDesc.bpp;
    if (!bpp)
    {
        WARN(("uninited bpp! unsupported!"));
        return FALSE;
    }

    LONG DstH, DstW, SrcH, SrcW;
    DstH = pDstRect->bottom - pDstRect->top;
    DstW = pDstRect->right - pDstRect->left;
    SrcH = pSrcRect->bottom - pSrcRect->top;
    SrcW = pSrcRect->right - pSrcRect->left;

    if(DstW != SrcW && DstH != SrcH)
    {
        WARN(("stretched comparison not supported!!"));
        return FALSE;
    }

    D3DLOCKED_RECT SrcLRect, DstLRect;
    HRESULT hr = VBoxD3DIfLockRect((VBOXWDDMDISP_RESOURCE *)pDstRc, iDstAlloc, &DstLRect, pDstRect, D3DLOCK_READONLY);
    if (FAILED(hr))
    {
        WARN(("VBoxD3DIfLockRect failed, hr(0x%x)", hr));
        return FALSE;
    }

    hr = VBoxD3DIfLockRect((VBOXWDDMDISP_RESOURCE *)pSrcRc, iSrcAlloc, &SrcLRect, pSrcRect, D3DLOCK_READONLY);
    if (FAILED(hr))
    {
        WARN(("VBoxD3DIfLockRect failed, hr(0x%x)", hr));
        hr = VBoxD3DIfUnlockRect((VBOXWDDMDISP_RESOURCE *)pDstRc, iDstAlloc);
        return FALSE;
    }

    fMatch = vboxVDbgDoCheckLRects(&DstLRect, pDstRect, &SrcLRect, pSrcRect, bpp, fBreakOnMismatch);

    hr = VBoxD3DIfUnlockRect((VBOXWDDMDISP_RESOURCE *)pDstRc, iDstAlloc);
    Assert(hr == S_OK);

    hr = VBoxD3DIfUnlockRect((VBOXWDDMDISP_RESOURCE *)pSrcRc, iSrcAlloc);
    Assert(hr == S_OK);

    return fMatch;
}

void vboxVDbgDoPrintAlloc(const char * pPrefix, const VBOXWDDMDISP_RESOURCE *pRc, uint32_t iAlloc, const char * pSuffix)
{
    Assert(pRc->cAllocations > iAlloc);
    const VBOXWDDMDISP_ALLOCATION *pAlloc = &pRc->aAllocations[iAlloc];
    BOOL bPrimary = pRc->RcDesc.fFlags.Primary;
    BOOL bFrontBuf = FALSE;
    if (bPrimary)
    {
        PVBOXWDDMDISP_SWAPCHAIN pSwapchain = vboxWddmSwapchainForAlloc((VBOXWDDMDISP_ALLOCATION *)pAlloc);
        Assert(pSwapchain);
        bFrontBuf = (vboxWddmSwapchainGetFb(pSwapchain)->pAlloc == pAlloc);
    }
    vboxVDbgPrint(("%s d3dWidth(%d), width(%d), height(%d), format(%d), usage(%s), %s", pPrefix,
            pAlloc->SurfDesc.d3dWidth, pAlloc->SurfDesc.width, pAlloc->SurfDesc.height, pAlloc->SurfDesc.format,
            bPrimary ?
                    (bFrontBuf ? "Front Buffer" : "Back Buffer")
                    : "?Everage? Alloc",
            pSuffix));
}

void vboxVDbgDoPrintRect(const char * pPrefix, const RECT *pRect, const char * pSuffix)
{
    vboxVDbgPrint(("%s left(%d), top(%d), right(%d), bottom(%d) %s", pPrefix, pRect->left, pRect->top, pRect->right, pRect->bottom, pSuffix));
}

# endif

static VOID CALLBACK vboxVDbgTimerCb(__in PVOID lpParameter, __in BOOLEAN TimerOrWaitFired)
{
    RT_NOREF(lpParameter, TimerOrWaitFired);
    Assert(0);
}

HRESULT vboxVDbgTimerStart(HANDLE hTimerQueue, HANDLE *phTimer, DWORD msTimeout)
{
    if (!CreateTimerQueueTimer(phTimer, hTimerQueue,
                               vboxVDbgTimerCb,
                               NULL,
                               msTimeout, /* ms*/
                               0,
                               WT_EXECUTEONLYONCE))
    {
        DWORD winEr = GetLastError();
        AssertMsgFailed(("CreateTimerQueueTimer failed, winEr (%d)\n", winEr));
        return E_FAIL;
    }
    return S_OK;
}

HRESULT vboxVDbgTimerStop(HANDLE hTimerQueue, HANDLE hTimer)
{
    if (!DeleteTimerQueueTimer(hTimerQueue, hTimer, NULL))
    {
        DWORD winEr = GetLastError();
        AssertMsg(winEr == ERROR_IO_PENDING, ("DeleteTimerQueueTimer failed, winEr (%d)\n", winEr));
    }
    return S_OK;
}
#endif

#if defined(VBOXWDDMDISP_DEBUG) || defined(VBOX_WDDMDISP_WITH_PROFILE)
BOOL vboxVDbgDoCheckExe(const char * pszName)
{
    char *pszModule = vboxVDbgDoGetModuleName();
    if (!pszModule)
        return FALSE;
    size_t cbModule, cbName;
    cbModule = strlen(pszModule);
    cbName = strlen(pszName);
    if (cbName > cbModule)
        return FALSE;
    if (_stricmp(pszName, pszModule + (cbModule - cbName)))
        return FALSE;
    return TRUE;
}
#endif

#ifdef VBOXWDDMDISP_DEBUG_VEHANDLER

static PVOID g_VBoxWDbgVEHandler = NULL;
LONG WINAPI vboxVDbgVectoredHandler(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;
    PCONTEXT pContextRecord = pExceptionInfo->ContextRecord;
    switch (pExceptionRecord->ExceptionCode)
    {
        case EXCEPTION_BREAKPOINT:
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_INVALID_OPERATION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            AssertRelease(0);
            break;
        default:
            break;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void vboxVDbgVEHandlerRegister()
{
    Assert(!g_VBoxWDbgVEHandler);
    g_VBoxWDbgVEHandler = AddVectoredExceptionHandler(1,vboxVDbgVectoredHandler);
    Assert(g_VBoxWDbgVEHandler);
}

void vboxVDbgVEHandlerUnregister()
{
    Assert(g_VBoxWDbgVEHandler);
    ULONG uResult = RemoveVectoredExceptionHandler(g_VBoxWDbgVEHandler);
    Assert(uResult);
    g_VBoxWDbgVEHandler = NULL;
}

#endif

#if defined(VBOXWDDMDISP_DEBUG) || defined(LOG_TO_BACKDOOR_DRV)
void vboxDispLogDrvF(char * szString, ...)
{
    char szBuffer[4096] = {0};
    va_list pArgList;
    va_start(pArgList, szString);
    vboxDispLogDbgFormatStringV(szBuffer, sizeof (szBuffer), szString, pArgList);
    va_end(pArgList);

    VBoxDispMpLoggerLog(szBuffer);
}
#endif

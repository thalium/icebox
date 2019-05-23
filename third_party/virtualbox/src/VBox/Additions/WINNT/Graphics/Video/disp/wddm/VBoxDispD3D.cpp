/* $Id: VBoxDispD3D.cpp $ */
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

#define INITGUID

#include <iprt/initterm.h>
#include <iprt/log.h>
#include <iprt/mem.h>

#include <VBox/Log.h>

#include <VBox/VBoxGuestLib.h>

#include "VBoxDispD3DCmn.h"
#include "VBoxDispD3D.h"
#include <VBoxCrHgsmi.h>

#include <Psapi.h>

#ifdef VBOX_WDDMDISP_WITH_PROFILE

volatile uint32_t g_u32VBoxDispProfileFunctionLoggerIndex = 0;

/* the number of frames to collect data before doing dump/reset */
#define VBOXDISPPROFILE_DDI_DUMP_FRAME_COUNT 0x20

struct VBOXDISPPROFILE_GLOBAL {
    VBoxDispProfileFpsCounter ProfileDdiFps;
    VBoxDispProfileSet ProfileDdiFunc;
} g_VBoxDispProfile;

/* uncomment to enable particular logging */
/* allows dumping fps + how much time is spent in ddi functions in comparison with the rest time */
//# define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_ENABLE
/* allows dumping time spent in each function and the number of calls made for any given function */
# define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_ENABLE

# ifdef VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_ENABLE

class VBoxDispProfileDevicePostProcess
{
public:
    VBoxDispProfileDevicePostProcess(PVBOXWDDMDISP_DEVICE pDevice) :
        m_pDevice(pDevice)
    {}

    void postProcess()
    {
        if (m_pDevice->pDevice9If)
            m_pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9Finish((IDirect3DDevice9Ex *)m_pDevice->pDevice9If);
    }
private:
    PVBOXWDDMDISP_DEVICE m_pDevice;
};

//static VBoxDispProfileSet g_VBoxDispProfileDDI("D3D_DDI");
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_PROLOGUE_DEV(_pObj) VBOXDISPPROFILE_FUNCTION_LOGGER_DEFINE((_pObj)->ProfileDdiFunc, VBoxDispProfileDevicePostProcess, VBoxDispProfileDevicePostProcess(_pObj))
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_PROLOGUE_BASE(_pObj) VBOXDISPPROFILE_FUNCTION_LOGGER_DEFINE((_pObj)->ProfileDdiFunc, VBoxDispProfileDummyPostProcess, VBoxDispProfileDummyPostProcess())
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_DUMP(_pObj) do {\
        (_pObj)->ProfileDdiFunc.dump(_pObj); \
    } while (0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_RESET(_pObj) do {\
        (_pObj)->ProfileDdiFunc.resetEntries();\
    } while (0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_DISABLE_CURRENT() do {\
        VBOXDISPPROFILE_FUNCTION_LOGGER_DISABLE_CURRENT();\
    } while (0)

#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_LOG_AND_DISABLE_CURRENT() VBOXDISPPROFILE_FUNCTION_LOGGER_LOG_AND_DISABLE_CURRENT()

#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_REPORT_FRAME(_pObj) do { \
        if (!((_pObj)->ProfileDdiFunc.reportIteration() % VBOXDISPPROFILE_DDI_DUMP_FRAME_COUNT) /*&& !VBOXVDBG_IS_DWM()*/) {\
            VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_DUMP(_pObj); \
            VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_RESET(_pObj); \
        } \
    } while (0)

# else
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_PROLOGUE_DEV(_pObj) do {} while(0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_PROLOGUE_BASE(_pObj) do {} while(0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_DUMP(_pObj) do {} while(0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_RESET(_pObj) do {} while(0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_DISABLE_CURRENT() do {} while (0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_LOG_AND_DISABLE_CURRENT() do {} while (0)
#  define VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_REPORT_FRAME(_pDev) do {} while (0)
# endif

# ifdef VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_ENABLE
//static VBoxDispProfileFpsCounter g_VBoxDispFpsDDI(64);
#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_PROLOGUE(_pObj) VBOXDISPPROFILE_STATISTIC_LOGGER_DEFINE(&(_pObj)->ProfileDdiFps, VBoxDispProfileDummyPostProcess, VBoxDispProfileDummyPostProcess())
#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_DISABLE_CURRENT() do {\
        VBOXDISPPROFILE_STATISTIC_LOGGER_DISABLE_CURRENT();\
    } while (0)

#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_DUMP(_pObj) do { \
            double fps = (_pObj)->ProfileDdiFps.GetFps(); \
            double cps = (_pObj)->ProfileDdiFps.GetCps(); \
            double tup = (_pObj)->ProfileDdiFps.GetTimeProcPercent(); \
            VBOXDISPPROFILE_DUMP(("[0x%p]: fps: %f, cps: %.1f, host %.1f%%", (_pObj), fps, cps, tup)); \
    } while (0)

#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_REPORT_FRAME(_pObj) do { \
        (_pObj)->ProfileDdiFps.ReportFrame(); \
        if(!((_pObj)->ProfileDdiFps.GetNumFrames() % VBOXDISPPROFILE_DDI_DUMP_FRAME_COUNT)) \
        { \
            VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_DUMP(_pObj); \
        } \
    } while (0)

#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_LOG_AND_DISABLE_CURRENT() VBOXDISPPROFILE_STATISTIC_LOGGER_LOG_AND_DISABLE_CURRENT()
# else
#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_PROLOGUE(_pObj) do {} while(0)
#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_DISABLE_CURRENT() do {} while (0)
#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_LOG_AND_DISABLE_CURRENT() do {} while (0)
#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_REPORT_FRAME(_pDev) do {} while (0)
#  define VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_DUMP(_pObj) do {} while (0)
# endif

# define VBOXDISPPROFILE_FUNCTION_DDI_PROLOGUE_DEV(_pObj) \
        VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_PROLOGUE_DEV(_pObj); \
        VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_PROLOGUE(_pObj);

# define VBOXDISPPROFILE_FUNCTION_DDI_PROLOGUE_BASE(_pObj) \
        VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_PROLOGUE_BASE(_pObj); \
        VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_PROLOGUE(_pObj);

# define VBOXDISPPROFILE_DDI_LOG_AND_DISABLE_CURRENT() \
        VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_LOG_AND_DISABLE_CURRENT(); \
        VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_LOG_AND_DISABLE_CURRENT();

# define VBOXDISPPROFILE_DDI_REPORT_FRAME(_pDev) do {\
        VBOXDISPPROFILE_DDI_LOG_AND_DISABLE_CURRENT(); \
        VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_REPORT_FRAME(_pDev); \
        VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_REPORT_FRAME(_pDev); \
    } while (0)

#if 0
# define VBOXDISPPROFILE_DDI_REPORT_FLUSH(_pDev) do {\
        VBOXDISPPROFILE_DDI_LOG_AND_DISABLE_CURRENT(); \
        VBOXDISPPROFILE_DDI_STATISTIC_LOGGER_REPORT_FRAME(_pDev); \
        VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_REPORT_FRAME(_pDev); \
    } while (0)
#else
# define VBOXDISPPROFILE_DDI_REPORT_FLUSH(_pDev) do {} while (0)
#endif

# define VBOXDISPPROFILE_DDI_INIT_CMN(_pObj, _name, _cEntries) do { \
        (_pObj)->ProfileDdiFps = VBoxDispProfileFpsCounter(); \
        (_pObj)->ProfileDdiFps.init(_cEntries); \
        (_pObj)->ProfileDdiFunc = VBoxDispProfileSet(_name); \
    } while (0)

# define VBOXDISPPROFILE_DDI_TERM_CMN(_pObj) do { \
        (_pObj)->ProfileDdiFps.term(); \
    } while (0)

# define VBOXDISPPROFILE_DDI_TERM(_pObj) do {\
        VBOXDISPPROFILE_DDI_LOG_AND_DISABLE_CURRENT(); \
        VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_DUMP(_pObj); \
        VBOXDISPPROFILE_DDI_FUNCTION_LOGGER_RESET(_pObj); \
        VBOXDISPPROFILE_DDI_TERM_CMN(_pObj); \
    } while (0)

# define VBOXDISPPROFILE_DDI_PRINT(_m) VBOXDISPPROFILE_DUMP(_m)

# define VBOXDISPPROFILE_DDI_INIT_GLBL() VBOXDISPPROFILE_DDI_INIT_CMN(&g_VBoxDispProfile, "DDI_Adp", 64)
# define VBOXDISPPROFILE_DDI_INIT_ADP(_pAdp) VBOXDISPPROFILE_DDI_INIT_CMN(_pAdp, "DDI_Adp", 64)
# define VBOXDISPPROFILE_DDI_INIT_DEV(_pDev) VBOXDISPPROFILE_DDI_INIT_CMN(_pDev, "DDI_Dev", 64)
#else
# define VBOXDISPPROFILE_FUNCTION_DDI_PROLOGUE_DEV(_pObj) do {} while (0)
# define VBOXDISPPROFILE_FUNCTION_DDI_PROLOGUE_BASE(_pObj) do {} while (0)
# define VBOXDISPPROFILE_DDI_REPORT_FRAME(_pDev) do {} while (0)
# define VBOXDISPPROFILE_DDI_REPORT_FLUSH(_pDev) do {} while (0)
# define VBOXDISPPROFILE_DDI_INIT_GLBL() do {} while (0)
# define VBOXDISPPROFILE_DDI_INIT_ADP(_pAdp) do {} while (0)
# define VBOXDISPPROFILE_DDI_INIT_DEV(_pDev) do {} while (0)
# define VBOXDISPPROFILE_DDI_TERM(_pObj) do {} while (0)
# define VBOXDISPPROFILE_DDI_PRINT(_m) do {} while (0)
#endif

/* debugging/profiling stuff could go here.
 * NOP in release */
#define VBOXDISP_DDI_PROLOGUE_CMN() \
    VBOXVDBG_BREAK_DDI(); \
    VBOXVDBG_CREATE_CHECK_SWAPCHAIN();

#define VBOXDISP_DDI_PROLOGUE_DEV(_hDevice) \
    NOREF(_hDevice); \
    VBOXDISP_DDI_PROLOGUE_CMN(); \
    VBOXDISPPROFILE_FUNCTION_DDI_PROLOGUE_DEV((PVBOXWDDMDISP_DEVICE)(_hDevice));

#define VBOXDISP_DDI_PROLOGUE_ADP(_hAdapter) \
    VBOXDISP_DDI_PROLOGUE_CMN(); \
    VBOXDISPPROFILE_FUNCTION_DDI_PROLOGUE_BASE((PVBOXWDDMDISP_ADAPTER)(_hAdapter));

#define VBOXDISP_DDI_PROLOGUE_GLBL() \
    VBOXDISP_DDI_PROLOGUE_CMN(); \
    VBOXDISPPROFILE_FUNCTION_DDI_PROLOGUE_BASE(&g_VBoxDispProfile);

#ifdef VBOXDISPMP_TEST
HRESULT vboxDispMpTstStart();
HRESULT vboxDispMpTstStop();
#endif

#define VBOXDISP_WITH_WINE_BB_WORKAROUND

//#define VBOXWDDMOVERLAY_TEST

static D3DDDIQUERYTYPE gVBoxQueryTypes[] = {
        D3DDDIQUERYTYPE_EVENT,
        D3DDDIQUERYTYPE_OCCLUSION
};

#define VBOX_QUERYTYPE_COUNT() RT_ELEMENTS(gVBoxQueryTypes)

static CRITICAL_SECTION g_VBoxCritSect;

void vboxDispLock()
{
    EnterCriticalSection(&g_VBoxCritSect);
}

void vboxDispUnlock()
{
    LeaveCriticalSection(&g_VBoxCritSect);
}

void vboxDispLockInit()
{
    InitializeCriticalSection(&g_VBoxCritSect);
}


#define VBOXDISPCRHGSMI_SCOPE_SET_DEV(_pDev) do {} while(0)
#define VBOXDISPCRHGSMI_SCOPE_SET_GLOBAL() do {} while(0)


typedef struct VBOXWDDMDISP_NSCADD
{
    VOID* pvCommandBuffer;
    UINT cbCommandBuffer;
    D3DDDI_ALLOCATIONLIST* pAllocationList;
    UINT cAllocationList;
    D3DDDI_PATCHLOCATIONLIST* pPatchLocationList;
    UINT cPatchLocationList;
    UINT cAllocations;
}VBOXWDDMDISP_NSCADD, *PVBOXWDDMDISP_NSCADD;

static HRESULT vboxWddmNSCAddAlloc(PVBOXWDDMDISP_NSCADD pData, PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    HRESULT hr = S_OK;
    Assert(pAlloc->fEverWritten || pAlloc->pRc->RcDesc.fFlags.SharedResource);
    if (pData->cAllocationList && pData->cPatchLocationList && pData->cbCommandBuffer >= 4)
    {
        memset(pData->pAllocationList, 0, sizeof (D3DDDI_ALLOCATIONLIST));
        pData->pAllocationList[0].hAllocation = pAlloc->hAllocation;
        if (pAlloc->fDirtyWrite)
            pData->pAllocationList[0].WriteOperation = 1;

        memset(pData->pPatchLocationList, 0, sizeof (D3DDDI_PATCHLOCATIONLIST));
        pData->pPatchLocationList[0].PatchOffset = pData->cAllocations*4;
        pData->pPatchLocationList[0].AllocationIndex = pData->cAllocations;

        pData->cbCommandBuffer -= 4;
        --pData->cAllocationList;
        --pData->cPatchLocationList;
        ++pData->cAllocations;

        ++pData->pAllocationList;
        ++pData->pPatchLocationList;
        pData->pvCommandBuffer = (VOID*)(((uint8_t*)pData->pvCommandBuffer) + 4);

    }
    else
        hr = S_FALSE;

    return hr;
}

static VOID vboxWddmDalRemove(PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    RTListNodeRemove(&pAlloc->DirtyAllocListEntry);
    pAlloc->fDirtyWrite = FALSE;
}

#ifdef DEBUG_misha
typedef struct VBOXWDDM_DBG_ALLOC
{
    BOOLEAN fWrite;
    PVBOXWDDMDISP_ALLOCATION pAlloc;
} VBOXWDDM_DBG_ALLOC;
#endif

static HRESULT vboxWddmDalNotifyChange(PVBOXWDDMDISP_DEVICE pDevice)
{
    VBOXWDDMDISP_NSCADD NscAdd = { NULL }; /* Shuts up MSC. */
    BOOL bReinitRenderData = TRUE;
#ifdef DEBUG_misha
    uint32_t cDbgAllocs = 0;
    VBOXWDDM_DBG_ALLOC aDbgAllocs[128];
#endif

    do
    {
        if (bReinitRenderData)
        {
            NscAdd.pvCommandBuffer = pDevice->DefaultContext.ContextInfo.pCommandBuffer;
            NscAdd.cbCommandBuffer = pDevice->DefaultContext.ContextInfo.CommandBufferSize;
            NscAdd.pAllocationList = pDevice->DefaultContext.ContextInfo.pAllocationList;
            NscAdd.cAllocationList = pDevice->DefaultContext.ContextInfo.AllocationListSize;
            NscAdd.pPatchLocationList = pDevice->DefaultContext.ContextInfo.pPatchLocationList;
            NscAdd.cPatchLocationList = pDevice->DefaultContext.ContextInfo.PatchLocationListSize;
            NscAdd.cAllocations = 0;
            Assert(NscAdd.cbCommandBuffer >= sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR));
            if (NscAdd.cbCommandBuffer < sizeof (VBOXWDDM_DMA_PRIVATEDATA_BASEHDR))
                return E_FAIL;

            PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR pHdr = (PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR)NscAdd.pvCommandBuffer;
            pHdr->enmCmd = VBOXVDMACMD_TYPE_DMA_NOP;
            NscAdd.pvCommandBuffer = (VOID*)(((uint8_t*)NscAdd.pvCommandBuffer) + sizeof (*pHdr));
            NscAdd.cbCommandBuffer -= sizeof (*pHdr);
            bReinitRenderData = FALSE;

#ifdef DEBUG_misha
            {
                memset(aDbgAllocs, 0, sizeof (aDbgAllocs));
                uint32_t cAllocs = 0;
                PVBOXWDDMDISP_ALLOCATION pAlloc;
                RTListForEach(&pDevice->DirtyAllocList, pAlloc, VBOXWDDMDISP_ALLOCATION, DirtyAllocListEntry)
                {
                    Assert(pAlloc->fEverWritten || pAlloc->pRc->RcDesc.fFlags.SharedResource);
                    if (cAllocs < RT_ELEMENTS(aDbgAllocs))
                    {
                        aDbgAllocs[cAllocs].pAlloc = pAlloc;
                        aDbgAllocs[cAllocs].fWrite = pAlloc->fDirtyWrite;
                        ++cDbgAllocs;
                    }
                    ++cAllocs;
                }
            }
#endif
        }

        PVBOXWDDMDISP_ALLOCATION pAlloc = RTListGetFirst(&pDevice->DirtyAllocList, VBOXWDDMDISP_ALLOCATION, DirtyAllocListEntry);
        if (pAlloc)
        {
            HRESULT tmpHr = vboxWddmNSCAddAlloc(&NscAdd, pAlloc);
#ifdef DEBUG_misha
            Assert(tmpHr == S_OK);
#endif
            Assert(tmpHr == S_OK || tmpHr == S_FALSE);
            if (tmpHr == S_OK)
            {
                vboxWddmDalRemove(pAlloc);
                continue;
            }
        }
        else
        {
            if (!NscAdd.cAllocations)
                break;
        }

        D3DDDICB_RENDER RenderData = {0};
        RenderData.CommandLength = pDevice->DefaultContext.ContextInfo.CommandBufferSize - NscAdd.cbCommandBuffer;
        Assert(RenderData.CommandLength);
        Assert(RenderData.CommandLength < UINT32_MAX/2);
        RenderData.CommandOffset = 0;
        RenderData.NumAllocations = pDevice->DefaultContext.ContextInfo.AllocationListSize - NscAdd.cAllocationList;
        Assert(RenderData.NumAllocations == NscAdd.cAllocations);
        RenderData.NumPatchLocations = pDevice->DefaultContext.ContextInfo.PatchLocationListSize - NscAdd.cPatchLocationList;
        Assert(RenderData.NumPatchLocations == NscAdd.cAllocations);
//        RenderData.NewCommandBufferSize = sizeof (VBOXVDMACMD) + 4 * (100);
//        RenderData.NewAllocationListSize = 100;
//        RenderData.NewPatchLocationListSize = 100;
        RenderData.hContext = pDevice->DefaultContext.ContextInfo.hContext;

        HRESULT hr = pDevice->RtCallbacks.pfnRenderCb(pDevice->hDevice, &RenderData);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            pDevice->DefaultContext.ContextInfo.CommandBufferSize = RenderData.NewCommandBufferSize;
            pDevice->DefaultContext.ContextInfo.pCommandBuffer = RenderData.pNewCommandBuffer;
            pDevice->DefaultContext.ContextInfo.AllocationListSize = RenderData.NewAllocationListSize;
            pDevice->DefaultContext.ContextInfo.pAllocationList = RenderData.pNewAllocationList;
            pDevice->DefaultContext.ContextInfo.PatchLocationListSize = RenderData.NewPatchLocationListSize;
            pDevice->DefaultContext.ContextInfo.pPatchLocationList = RenderData.pNewPatchLocationList;
            bReinitRenderData = TRUE;
        }
        else
            break;
    } while (1);

    return S_OK;
}

#ifdef VBOXWDDMDISP_DAL_CHECK_LOCK
static HRESULT vboxWddmDalCheckUnlock(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    if (!pAlloc->fAllocLocked || pAlloc->LockInfo.cLocks)
        return S_OK;

    Assert(pAlloc->hAllocation);

    D3DDDICB_UNLOCK Unlock;

    Unlock.NumAllocations = 1;
    Unlock.phAllocations = &pAlloc->hAllocation;

    HRESULT hr = pDevice->RtCallbacks.pfnUnlockCb(pDevice->hDevice, &Unlock);
    if(hr != S_OK)
    {
        WARN(("pfnUnlockCb failed, hr %#x", hr));
    }

    return hr;
}

static HRESULT vboxWddmDalCheckLock(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_ALLOCATION pAlloc, D3DDDI_LOCKFLAGS Flags)
{
    if (!pAlloc->hAllocation || pAlloc->fAllocLocked)
        return S_OK;

    HRESULT hr;

    if (pAlloc->fDirtyWrite)
    {
        Assert(pAlloc->DirtyAllocListEntry.pNext);
        hr = vboxWddmDalNotifyChange(pDevice);
        if (hr == S_OK)
        {
            Assert(!pAlloc->DirtyAllocListEntry.pNext);
        }
        else
        {
            WARN(("vboxWddmDalNotifyChange failed %#x, ignoring", hr));
        }
    }

    D3DDDICB_LOCK LockData;
    LockData.hAllocation = pAlloc->hAllocation;
    LockData.PrivateDriverData = 0;
    LockData.NumPages = 0;
    LockData.pPages = NULL;
    LockData.pData = NULL; /* out */
    LockData.Flags.Value = 0;
    LockData.Flags.Discard = Flags.Discard;
    LockData.Flags.DonotWait = Flags.DoNotWait;

    hr = pDevice->RtCallbacks.pfnLockCb(pDevice->hDevice, &LockData);
    if (hr == S_OK)
    {
        if (!Flags.ReadOnly)
            pAlloc->fEverWritten = TRUE;
        pAlloc->fAllocLocked = TRUE;
        return S_OK;
    }

    WARN(("pfnLockCb failed %#x, Flags %#x", hr, Flags.Value));

    return hr;
}
#endif

static BOOLEAN vboxWddmDalCheckNotifyRemove(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    if (pAlloc->DirtyAllocListEntry.pNext)
    {
        HRESULT hr = vboxWddmDalNotifyChange(pDevice);
        if (hr == S_OK)
        {
            Assert(!pAlloc->DirtyAllocListEntry.pNext);
        }
        else
        {
            WARN(("vboxWddmDalNotifyChange failed %#x", hr));
            if (pAlloc->DirtyAllocListEntry.pNext)
                vboxWddmDalRemove(pAlloc);
        }

        return TRUE;
    }

    return FALSE;
}

static BOOLEAN vboxWddmDalCheckAdd(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_ALLOCATION pAlloc, BOOLEAN fWrite)
{
    if (!pAlloc->hAllocation /* only shared resources matter */
            || (/*!fWrite &&*/ !pAlloc->hSharedHandle)
            )
    {
        Assert(!pAlloc->DirtyAllocListEntry.pNext || pAlloc->hSharedHandle /*|| pAlloc->fDirtyWrite*/);

        Assert(!pAlloc->hSharedHandle);

        return FALSE;
    }

    Assert(fWrite || pAlloc->fEverWritten || pAlloc->pRc->RcDesc.fFlags.SharedResource);

    if (!pAlloc->DirtyAllocListEntry.pNext)
    {
        Assert(!pAlloc->fDirtyWrite);
        RTListAppend(&pDevice->DirtyAllocList, &pAlloc->DirtyAllocListEntry);
    }
    else
    {
        Assert(pAlloc->fDirtyWrite == fWrite || pAlloc->pRc->RcDesc.fFlags.SharedResource);
    }
    pAlloc->fDirtyWrite |= fWrite;
    pAlloc->fEverWritten |= fWrite;

    return TRUE;
}

DECLINLINE(BOOLEAN) vboxWddmDalCheckAddRc(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_RESOURCE pRc, BOOLEAN fWrite)
{
    BOOLEAN fChanged = FALSE;
    for (UINT i = 0; i < pRc->cAllocations; ++i)
    {
        PVBOXWDDMDISP_ALLOCATION pDAlloc = &pRc->aAllocations[i];
        fChanged |= vboxWddmDalCheckAdd(pDevice, pDAlloc, fWrite);
    }
    return fChanged;
}

static VOID vboxWddmDalCheckAddDepthStencil(PVBOXWDDMDISP_DEVICE pDevice)
{
    if (pDevice->pDepthStencilRc)
        vboxWddmDalCheckAddRc(pDevice, pDevice->pDepthStencilRc, TRUE);
}

static VOID vboxWddmDalCheckAddRTs(PVBOXWDDMDISP_DEVICE pDevice)
{
    for (UINT i = 0; i < pDevice->cRTs; ++i)
    {
        if (pDevice->apRTs[i])
        {
            vboxWddmDalCheckAdd(pDevice, pDevice->apRTs[i], TRUE);
        }
    }
}

static VOID vboxWddmDalCheckAddSamplers(PVBOXWDDMDISP_DEVICE pDevice)
{
    for (UINT i = 0, iSampler = 0; iSampler < pDevice->cSamplerTextures; ++i)
    {
        Assert(i < RT_ELEMENTS(pDevice->aSamplerTextures));
        if (!pDevice->aSamplerTextures[i]) continue;
        vboxWddmDalCheckAddRc(pDevice, pDevice->aSamplerTextures[i], FALSE);
        ++iSampler;
    }
}

static VOID vboxWddmDalCheckAddOnDraw(PVBOXWDDMDISP_DEVICE pDevice)
{
    vboxWddmDalCheckAddRTs(pDevice);

    vboxWddmDalCheckAddDepthStencil(pDevice);

    vboxWddmDalCheckAddSamplers(pDevice);
}

static BOOLEAN vboxWddmDalIsEmpty(PVBOXWDDMDISP_DEVICE pDevice)
{
    return RTListIsEmpty(&pDevice->DirtyAllocList);
}

#ifdef VBOX_WITH_VIDEOHWACCEL

static bool vboxVhwaIsEnabled(PVBOXWDDMDISP_ADAPTER pAdapter)
{
    for (uint32_t i = 0; i < pAdapter->cHeads; ++i)
    {
        if (pAdapter->aHeads[i].Vhwa.Settings.fFlags & VBOXVHWA_F_ENABLED)
            return true;
    }
    return false;
}

static bool vboxVhwaHasCKeying(PVBOXWDDMDISP_ADAPTER pAdapter)
{
    for (uint32_t i = 0; i < pAdapter->cHeads; ++i)
    {
        VBOXVHWA_INFO* pSettings = &pAdapter->aHeads[i].Vhwa.Settings;
        if ((pSettings->fFlags & VBOXVHWA_F_ENABLED)
                && ((pSettings->fFlags & VBOXVHWA_F_CKEY_DST)
                        || (pSettings->fFlags & VBOXVHWA_F_CKEY_SRC))
               )
            return true;
    }
    return false;
}

#endif

static void vboxResourceFree(PVBOXWDDMDISP_RESOURCE pRc)
{
    RTMemFree(pRc);
}

void vboxWddmResourceInit(PVBOXWDDMDISP_RESOURCE pRc, UINT cAllocs)
{
    memset(pRc, 0, RT_OFFSETOF(VBOXWDDMDISP_RESOURCE, aAllocations[cAllocs]));
    pRc->cAllocations = cAllocs;
    for (UINT i = 0; i < cAllocs; ++i)
    {
        pRc->aAllocations[i].iAlloc = i;
        pRc->aAllocations[i].pRc = pRc;
    }
}

static PVBOXWDDMDISP_RESOURCE vboxResourceAlloc(UINT cAllocs)
{
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)RTMemAlloc(RT_OFFSETOF(VBOXWDDMDISP_RESOURCE, aAllocations[cAllocs]));
    Assert(pRc);
    if (pRc)
    {
        vboxWddmResourceInit(pRc, cAllocs);
        return pRc;
    }
    return NULL;
}

#ifdef VBOXWDDMDISP_DEBUG
static void vboxWddmDbgSynchMemCheck(PVBOXWDDMDISP_ALLOCATION pAlloc, D3DLOCKED_RECT *pLockInfo)
{
    Assert(pAlloc->SurfDesc.pitch);
    Assert(pAlloc->pvMem);
    int iRc = 0;

    if (pAlloc->SurfDesc.pitch == (UINT)pLockInfo->Pitch)
    {
        Assert(pAlloc->SurfDesc.cbSize);
        iRc = memcmp(pLockInfo->pBits, pAlloc->pvMem, pAlloc->SurfDesc.cbSize);
        Assert(!iRc);
    }
    else
    {
        uint8_t *pvSrc, *pvDst;
        uint32_t srcPitch, dstPitch;
        if (1)
        {
            pvSrc = (uint8_t *)pAlloc->pvMem;
            pvDst = (uint8_t *)pLockInfo->pBits;
            srcPitch = pAlloc->SurfDesc.pitch;
            dstPitch = pLockInfo->Pitch;
        }
        else
        {
            pvDst = (uint8_t *)pAlloc->pvMem;
            pvSrc = (uint8_t *)pLockInfo->pBits;
            dstPitch = pAlloc->SurfDesc.pitch;
            srcPitch = (uint32_t)pLockInfo->Pitch;
        }

        Assert(pAlloc->SurfDesc.pitch <= (UINT)pLockInfo->Pitch);
        uint32_t pitch = RT_MIN(srcPitch, dstPitch);
        Assert(pitch);
        uint32_t cRows = vboxWddmCalcNumRows(0, pAlloc->SurfDesc.height, pAlloc->SurfDesc.format);
        for (UINT j = 0; j < cRows; ++j)
        {
            iRc = memcmp(pvDst, pvSrc, pitch);
            Assert(!iRc);
            pvSrc += srcPitch;
            pvDst += dstPitch;
        }
    }
}

static VOID vboxWddmDbgRcSynchMemCheck(PVBOXWDDMDISP_RESOURCE pRc)
{
    if (!pRc)
    {
        return;
    }

    if (pRc->RcDesc.enmPool != D3DDDIPOOL_SYSTEMMEM)
    {
        return;
    }

    for (UINT i = 0; i < pRc->cAllocations; ++i)
    {
        D3DLOCKED_RECT Rect;
        HRESULT hr = VBoxD3DIfLockRect(pRc, i, &Rect, NULL, D3DLOCK_READONLY);
        if (FAILED(hr))
        {
            WARN(("VBoxD3DIfLockRect failed, hr(0x%x)", hr));
            return;
        }

        PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[i];
        Assert(pAlloc->pvMem);

        vboxWddmDbgSynchMemCheck(pAlloc, &Rect);

        hr = VBoxD3DIfUnlockRect(pRc, i);
        Assert(SUCCEEDED(hr));
    }
}
#endif


/******/
static HRESULT vboxWddmRenderTargetSet(PVBOXWDDMDISP_DEVICE pDevice, UINT iRt, PVBOXWDDMDISP_ALLOCATION pAlloc,
                                       BOOL bOnSwapchainSynch);

DECLINLINE(VOID) vboxWddmSwapchainInit(PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    RTLISTNODE ListEntry = pSwapchain->ListEntry;
    memset(pSwapchain, 0, sizeof (VBOXWDDMDISP_SWAPCHAIN));
    pSwapchain->ListEntry = ListEntry;
    pSwapchain->iBB = VBOXWDDMDISP_INDEX_UNDEFINED;
}

static HRESULT vboxWddmSwapchainKmSynch(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    struct
    {
        VBOXDISPIFESCAPE_SWAPCHAININFO SwapchainInfo;
        D3DKMT_HANDLE ahAllocs[VBOXWDDMDISP_MAX_SWAPCHAIN_SIZE];
    } Buf;

    memset(&Buf.SwapchainInfo, 0, sizeof (Buf.SwapchainInfo));
    Buf.SwapchainInfo.EscapeHdr.escapeCode = VBOXESC_SWAPCHAININFO;
    Buf.SwapchainInfo.SwapchainInfo.hSwapchainKm = pSwapchain->hSwapchainKm;
#pragma warning(suppress:4826) /** @todo warning C4826: Conversion from 'PVBOXWDDMDISP_SWAPCHAIN' to 'VBOXDISP_UMHANDLE' is sign-extended. This may cause unexpected runtime behavior. */
    Buf.SwapchainInfo.SwapchainInfo.hSwapchainUm = (VBOXDISP_UMHANDLE)pSwapchain;
    HRESULT hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DSwapchain9GetHostWinID(pSwapchain->pSwapChainIf,
                                                                                   &Buf.SwapchainInfo.SwapchainInfo.winHostID);
    if (FAILED(hr))
    {
        WARN(("pfnVBoxWineExD3DSwapchain9GetHostWinID failed, hr 0x%x", hr));
        return hr;
    }
    Assert(Buf.SwapchainInfo.SwapchainInfo.winHostID);
//    Buf.SwapchainInfo.SwapchainInfo.Rect;
//    Buf.SwapchainInfo.SwapchainInfo.u32Reserved;
    Buf.SwapchainInfo.SwapchainInfo.cAllocs = pSwapchain->cRTs;
    UINT cAllocsKm = 0;
    for (UINT i = 0; i < Buf.SwapchainInfo.SwapchainInfo.cAllocs; ++i)
    {
//        Assert(pSwapchain->aRTs[i].pAlloc->hAllocation);
        Buf.SwapchainInfo.SwapchainInfo.ahAllocs[i] = pSwapchain->aRTs[i].pAlloc->hAllocation;
        if (Buf.SwapchainInfo.SwapchainInfo.ahAllocs[i])
            ++cAllocsKm;
    }

    Assert(cAllocsKm == Buf.SwapchainInfo.SwapchainInfo.cAllocs || !cAllocsKm);
    if (cAllocsKm == Buf.SwapchainInfo.SwapchainInfo.cAllocs)
    {
        D3DDDICB_ESCAPE DdiEscape = {0};
        DdiEscape.hContext = pDevice->DefaultContext.ContextInfo.hContext;
        DdiEscape.hDevice = pDevice->hDevice;
    //    DdiEscape.Flags.Value = 0;
        DdiEscape.pPrivateDriverData = &Buf.SwapchainInfo;
        DdiEscape.PrivateDriverDataSize = RT_OFFSETOF(VBOXDISPIFESCAPE_SWAPCHAININFO,
                                                      SwapchainInfo.ahAllocs[Buf.SwapchainInfo.SwapchainInfo.cAllocs]);
        hr = pDevice->RtCallbacks.pfnEscapeCb(pDevice->pAdapter->hAdapter, &DdiEscape);
#ifdef DEBUG_misha
        Assert(hr == S_OK);
#endif
        if (hr == S_OK)
        {
            pSwapchain->hSwapchainKm = Buf.SwapchainInfo.SwapchainInfo.hSwapchainKm;
        }
    }

    return S_OK;
}

static HRESULT vboxWddmSwapchainKmDestroy(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    HRESULT hr = S_OK;
    if (pSwapchain->hSwapchainKm)
    {
        /* submit empty swapchain to destroy the KM one */
        UINT cOldRTc = pSwapchain->cRTs;
        pSwapchain->cRTs = 0;
        hr = vboxWddmSwapchainKmSynch(pDevice, pSwapchain);
        Assert(hr == S_OK);
        Assert(!pSwapchain->hSwapchainKm);
        pSwapchain->cRTs = cOldRTc;
    }
    return hr;
}
static HRESULT vboxWddmSwapchainDestroyIf(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    RT_NOREF(pDevice);
    if (pSwapchain->pSwapChainIf)
    {
#ifndef VBOXWDDM_WITH_VISIBLE_FB
        if (pSwapchain->pRenderTargetFbCopy)
        {
            pSwapchain->pRenderTargetFbCopy->Release();
            pSwapchain->pRenderTargetFbCopy = NULL;
            pSwapchain->bRTFbCopyUpToDate = FALSE;
        }
#endif
        pSwapchain->pSwapChainIf->Release();
        Assert(pSwapchain->hWnd);
        pSwapchain->pSwapChainIf = NULL;
        pSwapchain->hWnd = NULL;
        return S_OK;
    }

    Assert(!pSwapchain->hWnd);
    return S_OK;
}

DECLINLINE(VOID) vboxWddmSwapchainClear(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    for (UINT i = 0; i < pSwapchain->cRTs; ++i)
    {
        pSwapchain->aRTs[i].pAlloc->pSwapchain = NULL;
    }

    /* first do a Km destroy to ensure all km->um region submissions are completed */
    vboxWddmSwapchainKmDestroy(pDevice, pSwapchain);
    vboxWddmSwapchainDestroyIf(pDevice, pSwapchain);
    vboxWddmSwapchainInit(pSwapchain);
}

VOID vboxWddmSwapchainDestroy(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    vboxWddmSwapchainClear(pDevice, pSwapchain);
    RTListNodeRemove(&pSwapchain->ListEntry);
    RTMemFree(pSwapchain);
}

static VOID vboxWddmSwapchainDestroyAll(PVBOXWDDMDISP_DEVICE pDevice)
{
    PVBOXWDDMDISP_SWAPCHAIN pCur = RTListGetFirst(&pDevice->SwapchainList, VBOXWDDMDISP_SWAPCHAIN, ListEntry);
    while (pCur)
    {
        PVBOXWDDMDISP_SWAPCHAIN pNext = NULL;
        if (!RTListNodeIsLast(&pDevice->SwapchainList, &pCur->ListEntry))
        {
            pNext = RTListNodeGetNext(&pCur->ListEntry, VBOXWDDMDISP_SWAPCHAIN, ListEntry);
        }

        vboxWddmSwapchainDestroy(pDevice, pCur);

        pCur = pNext;
    }
}

static PVBOXWDDMDISP_SWAPCHAIN vboxWddmSwapchainAlloc(PVBOXWDDMDISP_DEVICE pDevice)
{
    PVBOXWDDMDISP_SWAPCHAIN pSwapchain = (PVBOXWDDMDISP_SWAPCHAIN)RTMemAllocZ(sizeof (VBOXWDDMDISP_SWAPCHAIN));
    Assert(pSwapchain);
    if (pSwapchain)
    {
        RTListAppend(&pDevice->SwapchainList, &pSwapchain->ListEntry);
        vboxWddmSwapchainInit(pSwapchain);
        return pSwapchain;
    }
    return NULL;
}

DECLINLINE(VOID) vboxWddmSwapchainRtInit(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, PVBOXWDDMDISP_RENDERTGT pRt,
                                         PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    pSwapchain->fFlags.bChanged = 1;
    pRt->pAlloc = pAlloc;
    pRt->cNumFlips = 0;
    pRt->fFlags.Value = 0;
    pRt->fFlags.bAdded = 1;
}

DECLINLINE(VOID) vboxWddmSwapchainBbAddTail(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, PVBOXWDDMDISP_ALLOCATION pAlloc, BOOL bAssignAsBb)
{
    pAlloc->pSwapchain = pSwapchain;
    /*VBOXWDDMDISP_SWAPCHAIN_FLAGS fOldFlags = pSwapchain->fFlags; - unused */
    PVBOXWDDMDISP_RENDERTGT pRt = &pSwapchain->aRTs[pSwapchain->cRTs];
    ++pSwapchain->cRTs;
    vboxWddmSwapchainRtInit(pSwapchain, pRt, pAlloc);
    if (pSwapchain->cRTs == 1)
    {
        Assert(pSwapchain->iBB == VBOXWDDMDISP_INDEX_UNDEFINED);
        pSwapchain->iBB = 0;
    }
    else if (bAssignAsBb)
    {
        pSwapchain->iBB = pSwapchain->cRTs - 1;
    }
    else if (pSwapchain->cRTs == 2) /* the first one is a frontbuffer */
    {
        pSwapchain->iBB = 1;
    }
}

DECLINLINE(VOID) vboxWddmSwapchainFlip(PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    pSwapchain->iBB = (pSwapchain->iBB + 1) % pSwapchain->cRTs;
}

DECLINLINE(UINT) vboxWddmSwapchainNumRTs(PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    return pSwapchain->cRTs;
}


DECLINLINE(PVBOXWDDMDISP_RENDERTGT) vboxWddmSwapchainRtForAlloc(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    if (pAlloc->pSwapchain != pSwapchain)
        return NULL;

    for (UINT i = 0; i < pSwapchain->cRTs; ++i)
    {
        Assert(pSwapchain->aRTs[i].pAlloc->pSwapchain = pSwapchain);
        if (pSwapchain->aRTs[i].pAlloc == pAlloc)
            return &pSwapchain->aRTs[i];
    }

    /* should never happen */
    Assert(0);
    return NULL;
}

DECLINLINE(UINT) vboxWddmSwapchainRtIndex(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, PVBOXWDDMDISP_RENDERTGT pRT)
{
    UINT offFirst = RT_OFFSETOF(VBOXWDDMDISP_SWAPCHAIN, aRTs);
    UINT offRT = UINT((uintptr_t)pRT - (uintptr_t)pSwapchain);
    Assert(offRT < sizeof (VBOXWDDMDISP_SWAPCHAIN));
    Assert(offRT >= offFirst);
    Assert(!((offRT - offFirst) % sizeof (VBOXWDDMDISP_RENDERTGT)));
    UINT iRt = (offRT - offFirst) / sizeof (VBOXWDDMDISP_RENDERTGT);
    Assert(iRt < pSwapchain->cRTs);
    return iRt;
}

DECLINLINE(VOID) vboxWddmSwapchainRtRemove(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, PVBOXWDDMDISP_RENDERTGT pRT)
{
    UINT iRt = vboxWddmSwapchainRtIndex(pSwapchain, pRT);
    Assert(iRt < pSwapchain->cRTs);
    pRT->pAlloc->pSwapchain = NULL;
    for (UINT i = iRt; i < pSwapchain->cRTs - 1; ++i)
    {
        pSwapchain->aRTs[i] = pSwapchain->aRTs[i + 1];
    }

    --pSwapchain->cRTs;
    if (pSwapchain->cRTs)
    {
        if (pSwapchain->iBB > iRt)
        {
            --pSwapchain->iBB;
        }
        else if (pSwapchain->iBB == iRt)
        {
            pSwapchain->iBB = 0;
        }
    }
    else
    {
        pSwapchain->iBB = VBOXWDDMDISP_INDEX_UNDEFINED;
    }
    pSwapchain->fFlags.bChanged = TRUE;
    pSwapchain->fFlags.bSwitchReportingPresent = TRUE;
}

DECLINLINE(VOID) vboxWddmSwapchainSetBb(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, PVBOXWDDMDISP_RENDERTGT pRT)
{
    UINT iRt = vboxWddmSwapchainRtIndex(pSwapchain, pRT);
    Assert(iRt < pSwapchain->cRTs);
    pSwapchain->iBB = iRt;
    pSwapchain->fFlags.bChanged = TRUE;
}

PVBOXWDDMDISP_SWAPCHAIN vboxWddmSwapchainFindCreate(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_ALLOCATION pBbAlloc,
                                                    BOOL *pbNeedPresent)
{
    PVBOXWDDMDISP_SWAPCHAIN pSwapchain = pBbAlloc->pSwapchain;
    if (pSwapchain)
    {
        /* check if this is what we expect */
        PVBOXWDDMDISP_RENDERTGT pRt = vboxWddmSwapchainGetBb(pSwapchain);
        if (pRt->pAlloc != pBbAlloc)
        {
            if (pBbAlloc == vboxWddmSwapchainGetFb(pSwapchain)->pAlloc)
            {
                /* the current front-buffer present is requested, don't do anything */
                *pbNeedPresent = FALSE;
                return pSwapchain;
            }
            /* bad, @todo: correct the swapchain by either removing the Rt and adding it to another swapchain
             * or by removing the pBbAlloc out of it */
/// @todo            Assert(0);

            PVBOXWDDMDISP_RENDERTGT pRt = vboxWddmSwapchainRtForAlloc(pSwapchain, pBbAlloc);
            Assert(pRt);
            vboxWddmSwapchainSetBb(pSwapchain, pRt);
            pSwapchain->fFlags.bSwitchReportingPresent = TRUE;
        }
    }

    *pbNeedPresent = TRUE;

    if (!pSwapchain)
    {
        /* first search for the swapchain the alloc might be added to */
        PVBOXWDDMDISP_SWAPCHAIN pCur = RTListGetFirst(&pDevice->SwapchainList, VBOXWDDMDISP_SWAPCHAIN, ListEntry);
        while (pCur)
        {
            PVBOXWDDMDISP_RENDERTGT pRt = vboxWddmSwapchainGetBb(pCur);
            Assert(pRt);
            if (pRt->cNumFlips < 2
                    && vboxWddmSwapchainRtIndex(pCur, pRt) == 0) /* <- in case we add a rt to the swapchain on present this would mean
                                                            * that the last RT in the swapchain array is now a frontbuffer and
                                                            * thus the aRTs[0] is a backbuffer */
            {
                if (pBbAlloc->SurfDesc.width == pRt->pAlloc->SurfDesc.width
                            && pBbAlloc->SurfDesc.height == pRt->pAlloc->SurfDesc.height
                            && vboxWddmFmtNoAlphaFormat(pBbAlloc->SurfDesc.format) == vboxWddmFmtNoAlphaFormat(pRt->pAlloc->SurfDesc.format)
                            && pBbAlloc->SurfDesc.VidPnSourceId == pRt->pAlloc->SurfDesc.VidPnSourceId
                            )
                {
                    vboxWddmSwapchainBbAddTail(pCur, pBbAlloc, TRUE);
                    pSwapchain = pCur;
                    break;
                }
            }
            if (RTListNodeIsLast(&pDevice->SwapchainList, &pCur->ListEntry))
                break;
            pCur = RTListNodeGetNext(&pCur->ListEntry, VBOXWDDMDISP_SWAPCHAIN, ListEntry);
        }

//        if (!pSwapchain) need to create a new one (see below)
    }

    if (!pSwapchain)
    {
        pSwapchain = vboxWddmSwapchainAlloc(pDevice);
        Assert(pSwapchain);
        if (pSwapchain)
        {
            vboxWddmSwapchainBbAddTail(pSwapchain, pBbAlloc, FALSE);
        }
    }

    return pSwapchain;
}

static PVBOXWDDMDISP_SWAPCHAIN vboxWddmSwapchainCreateForRc(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_RESOURCE pRc)
{
    PVBOXWDDMDISP_SWAPCHAIN pSwapchain = vboxWddmSwapchainAlloc(pDevice);
    Assert(pSwapchain);
    if (pSwapchain)
    {
        for (UINT i = 0; i < pRc->cAllocations; ++i)
        {
            vboxWddmSwapchainBbAddTail(pSwapchain, &pRc->aAllocations[i], FALSE);
        }
        return pSwapchain;
    }
    return NULL;
}

DECLINLINE(UINT) vboxWddmSwapchainIdxBb2Rt(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, uint32_t iBb)
{
    return iBb != (~0) ? (iBb + pSwapchain->iBB) % pSwapchain->cRTs : vboxWddmSwapchainIdxFb(pSwapchain);
}

static HRESULT vboxWddmSwapchainRtSynch(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain, uint32_t iBb)
{
    if (pSwapchain->fFlags.bRtReportingPresent)
        return S_OK;

    IDirect3DSurface9 *pD3D9Surf;
#ifdef VBOXDISP_WITH_WINE_BB_WORKAROUND
    if (pSwapchain->cRTs == 1)
    {
        iBb = 0;
    }
#endif
    UINT iRt = vboxWddmSwapchainIdxBb2Rt(pSwapchain, iBb);
    Assert(iRt < pSwapchain->cRTs);
    PVBOXWDDMDISP_RENDERTGT pRt = &pSwapchain->aRTs[iRt];
    HRESULT hr = pSwapchain->pSwapChainIf->GetBackBuffer(iBb, D3DBACKBUFFER_TYPE_MONO, &pD3D9Surf);
    if (FAILED(hr))
    {
        WARN(("GetBackBuffer failed, hr (0x%x)",hr));
        return hr;
    }

    PVBOXWDDMDISP_ALLOCATION pAlloc = pRt->pAlloc;
    Assert(pD3D9Surf);
    Assert(pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE);
    if (pAlloc->pD3DIf)
    {
        if (pSwapchain->fFlags.bChanged)
        {
            IDirect3DSurface9 *pD3D9OldSurf = NULL;
            if (pAlloc->pD3DIf)
            {
                /* since this can be texture, need to do the VBoxD3DIfSurfGet magic */
                hr = VBoxD3DIfSurfGet(pAlloc->pRc, pAlloc->iAlloc, &pD3D9OldSurf);
                if (FAILED(hr))
                {
                    WARN(("VBoxD3DIfSurfGet failed, hr (0x%x)",hr));
                    pD3D9Surf->Release();
                    return hr;
                }
            }

            if (pD3D9OldSurf && pD3D9OldSurf != pD3D9Surf)
            {
                VOID *pvSwapchain = NULL;
                /* get the old surface's swapchain */
                HRESULT tmpHr = pD3D9OldSurf->GetContainer(IID_IDirect3DSwapChain9, &pvSwapchain);
                if (tmpHr == S_OK)
                {
                    Assert(pvSwapchain);
                    ((IDirect3DSwapChain9 *)pvSwapchain)->Release();
                }
                else
                {
                    Assert(!pvSwapchain);
                }

                if (pvSwapchain != pSwapchain->pSwapChainIf)
                {
                    /* the swapchain has changed, copy data to the new surface */
#ifdef DEBUG_misha
                    /** @todo we can not generally update the render target directly, implement */
                    Assert(iBb != (~0));
#endif
                    VBOXVDBG_CHECK_SWAPCHAIN_SYNC(hr = pDevice->pDevice9If->StretchRect(pD3D9OldSurf, NULL, pD3D9Surf, NULL,
                                                                                        D3DTEXF_NONE); Assert(hr == S_OK),
                                                  pAlloc, pD3D9OldSurf, NULL, pAlloc, pD3D9Surf, NULL);
                }
            }

            if (pD3D9OldSurf)
            {
                pD3D9OldSurf->Release();
            }
        }
        else
        {
            Assert(pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE);
        }
        pAlloc->pD3DIf->Release();
    }

    pAlloc->enmD3DIfType = VBOXDISP_D3DIFTYPE_SURFACE;
    pAlloc->pD3DIf = pD3D9Surf;
    pRt->fFlags.Value = 0;

    if (pSwapchain->fFlags.bChanged)
    {
        for (UINT i = 0; i < pDevice->cRTs; ++i)
        {
            if (pDevice->apRTs[i] == pAlloc)
            {
                hr = vboxWddmRenderTargetSet(pDevice, i, pAlloc, TRUE);
                Assert(hr == S_OK);
            }
        }
    }

#ifdef VBOXDISP_WITH_WINE_BB_WORKAROUND
    if (pSwapchain->cRTs == 1)
    {
        IDirect3DSurface9 *pD3D9Bb;
        /* only use direct bb if wine is able to handle quick blits bewteen surfaces in one swapchain,
         * this is FALSE by now :( */
# ifdef VBOX_WINE_WITH_FAST_INTERSWAPCHAIN_BLT
        /* here we sync the front-buffer with a backbuffer data*/
        pD3D9Bb = (IDirect3DSurface9*)vboxWddmSwapchainGetBb(pSwapchain)->pAlloc->pD3DIf;
        Assert(pD3D9Bb);
        pD3D9Bb->AddRef();
        /* we use backbuffer as a rt frontbuffer copy, so release the old one and assign the current bb */
        if (pSwapchain->pRenderTargetFbCopy)
        {
            pSwapchain->pRenderTargetFbCopy->Release();
        }
        pSwapchain->pRenderTargetFbCopy = pD3D9Bb;
# else
        pD3D9Bb = pSwapchain->pRenderTargetFbCopy;
# endif
        HRESULT tmpHr = pSwapchain->pSwapChainIf->GetFrontBufferData(pD3D9Bb);
        if (SUCCEEDED(tmpHr))
        {
            VBOXVDBG_DUMP_SYNC_RT(pD3D9Bb);
            pSwapchain->bRTFbCopyUpToDate = TRUE;
# ifndef VBOX_WINE_WITH_FAST_INTERSWAPCHAIN_BLT
            VBOXVDBG_CHECK_SWAPCHAIN_SYNC(tmpHr = pDevice->pDevice9If->StretchRect(pD3D9Bb, NULL,
                                                                                   (IDirect3DSurface9*)vboxWddmSwapchainGetBb(pSwapchain)->pAlloc->pD3DIf,
                                                                                   NULL, D3DTEXF_NONE); Assert(tmpHr == S_OK),
                                         pAlloc, pD3D9Bb, NULL, pAlloc,
                                         (IDirect3DSurface9*)vboxWddmSwapchainGetBb(pSwapchain)->pAlloc->pD3DIf, NULL);

            if (FAILED(tmpHr))
            {
                WARN(("StretchRect failed, hr (0x%x)", tmpHr));
            }
# endif
        }
        else
        {
            WARN(("GetFrontBufferData failed, hr (0x%x)", tmpHr));
        }
    }
#endif
    return hr;
}

static HRESULT vboxWddmSwapchainSynch(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    HRESULT hr = S_OK;
    for (int iBb = -1; iBb < int(pSwapchain->cRTs - 1); ++iBb)
    {
        hr = vboxWddmSwapchainRtSynch(pDevice, pSwapchain, (UINT)iBb);
        Assert(hr == S_OK);
    }
    if (pSwapchain->fFlags.bChanged)
    {
        hr = vboxWddmSwapchainKmSynch(pDevice, pSwapchain);
        if (hr == S_OK)
        {
            pSwapchain->fFlags.bChanged = 0;
        }
    }
    return hr;
}

static VOID vboxWddmSwapchainFillPresentParams(PVBOXWDDMDISP_SWAPCHAIN pSwapchain, D3DPRESENT_PARAMETERS *pParams)
{
    Assert(pSwapchain->cRTs);
#ifdef DEBUG_misha
    /* not supported by wine properly, need to use offscreen render targets and blit their data to swapchain RTs*/
    Assert(pSwapchain->cRTs <= 2);
#endif
    PVBOXWDDMDISP_RENDERTGT pRt = vboxWddmSwapchainGetBb(pSwapchain);
    PVBOXWDDMDISP_RESOURCE pRc = pRt->pAlloc->pRc;
    VBoxD3DIfFillPresentParams(pParams, pRc, pSwapchain->cRTs);
}

/* copy current rt data to offscreen render targets */
static HRESULT vboxWddmSwapchainSwtichOffscreenRt(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain,
                                                  BOOL fForceCreate)
{
    D3DPRESENT_PARAMETERS Params;
    vboxWddmSwapchainFillPresentParams(pSwapchain, &Params);
    IDirect3DSurface9* pD3D9OldFb = NULL;
    IDirect3DSwapChain9 * pOldIf = pSwapchain->pSwapChainIf;
    HRESULT hr = S_OK;
    if (pOldIf)
    {
        hr = pOldIf->GetBackBuffer(~0U, D3DBACKBUFFER_TYPE_MONO, &pD3D9OldFb);
        if (FAILED(hr))
        {
            WARN(("GetBackBuffer ~0U failed, hr (%d)", hr));
            return hr;
        }
        /* just need a pointer to match */
        pD3D9OldFb->Release();
    }

    for (UINT i = 0; i < pSwapchain->cRTs; ++i)
    {
        PVBOXWDDMDISP_RENDERTGT pRT = &pSwapchain->aRTs[i];
        if (pRT->pAlloc->enmD3DIfType != VBOXDISP_D3DIFTYPE_SURFACE)
            continue;
        BOOL fHasSurf = pRT->pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE ?
                !!pRT->pAlloc->pD3DIf
                :
                !!pRT->pAlloc->pRc->aAllocations[0].pD3DIf;
        if (!fForceCreate && !fHasSurf)
            continue;

        IDirect3DSurface9* pD3D9OldSurf = NULL;
        if (fHasSurf)
        {
            VOID *pvSwapchain = NULL;
            /* since this can be texture, need to do the VBoxD3DIfSurfGet magic */
            hr = VBoxD3DIfSurfGet(pRT->pAlloc->pRc, pRT->pAlloc->iAlloc, &pD3D9OldSurf);
            Assert(hr == S_OK);
            hr = pD3D9OldSurf->GetContainer(IID_IDirect3DSwapChain9, &pvSwapchain);
            if (hr == S_OK)
            {
                Assert(pvSwapchain);
                ((IDirect3DSwapChain9 *)pvSwapchain)->Release();
            }
            else
            {
                hr = S_OK;
                Assert(!pvSwapchain);
            }

            if (!pvSwapchain) /* no swapchain, it is already offscreen */
            {
                pD3D9OldSurf->Release();
                continue;
            }
            Assert (pvSwapchain == pOldIf);
        }

        IDirect3DSurface9* pD3D9NewSurf;
        IDirect3DDevice9 *pDevice9If = pDevice->pDevice9If;
        hr = pDevice9If->CreateRenderTarget(
                                Params.BackBufferWidth, Params.BackBufferHeight,
                                Params.BackBufferFormat,
                                Params.MultiSampleType,
                                Params.MultiSampleQuality,
                                TRUE, /*bLockable*/
                                &pD3D9NewSurf,
                                pRT->pAlloc->hSharedHandle ? &pRT->pAlloc->hSharedHandle :  NULL
                                );
        Assert(hr == S_OK);
        if (FAILED(hr))
        {
            if (pD3D9OldSurf)
                pD3D9OldSurf->Release();
            break;
        }

        if (pD3D9OldSurf)
        {
            if (pD3D9OldSurf != pD3D9OldFb)
            {
                VBOXVDBG_CHECK_SWAPCHAIN_SYNC(hr = pDevice9If->StretchRect(pD3D9OldSurf, NULL, pD3D9NewSurf, NULL, D3DTEXF_NONE);
                                              Assert(hr == S_OK),
                                              pRT->pAlloc, pD3D9OldSurf, NULL, pRT->pAlloc, pD3D9NewSurf, NULL);
            }
            else
            {
                hr = pOldIf->GetFrontBufferData(pD3D9NewSurf);
                Assert(hr == S_OK);
            }
        }
        if (FAILED(hr))
        {
            if (pD3D9OldSurf)
                pD3D9OldSurf->Release();
            break;
        }

        Assert(pRT->pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE);

        if (pRT->pAlloc->pD3DIf)
            pRT->pAlloc->pD3DIf->Release();
        pRT->pAlloc->pD3DIf = pD3D9NewSurf;
        if (pD3D9OldSurf)
            pD3D9OldSurf->Release();
    }

    return hr;
}


/**
 * @return old RtReportingPresent state
 */
static HRESULT vboxWddmSwapchainSwtichRtPresent(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    if (pSwapchain->fFlags.bRtReportingPresent)
        return S_OK;

    HRESULT hr;
    pSwapchain->bRTFbCopyUpToDate = FALSE;
    if (pSwapchain->pRenderTargetFbCopy)
    {
        pSwapchain->pRenderTargetFbCopy->Release();
        pSwapchain->pRenderTargetFbCopy = NULL;
    }

    hr = vboxWddmSwapchainSwtichOffscreenRt(pDevice, pSwapchain,
                TRUE /* force offscreen surface creation right away. This way we ensure the swapchain data
                      * is always uptodate which allows making the vboxWddmSwapchainRtSynch behave as a nop */
                );
    Assert(hr == S_OK);
    if (FAILED(hr))
        return hr;

    /* ensure we update device RTs to offscreen ones we just created */
    for (UINT i = 0; i < pDevice->cRTs; ++i)
    {
        PVBOXWDDMDISP_ALLOCATION pRtAlloc = pDevice->apRTs[i];
        if (!pRtAlloc) continue;
        for (UINT j = 0; j < pSwapchain->cRTs; ++j)
        {
            PVBOXWDDMDISP_ALLOCATION pAlloc = pSwapchain->aRTs[j].pAlloc;
            if (pRtAlloc == pAlloc)
            {
                hr = vboxWddmRenderTargetSet(pDevice, i, pAlloc, TRUE);
                Assert(hr == S_OK);
            }
        }
    }

    pSwapchain->fFlags.bRtReportingPresent = TRUE;
    return hr;
}

HRESULT vboxWddmSwapchainChkCreateIf(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    if (!pSwapchain->fFlags.bChanged && pSwapchain->pSwapChainIf)
        return S_OK;
    /* preserve the old one */
    IDirect3DSwapChain9 * pOldIf = pSwapchain->pSwapChainIf;
    HRESULT hr = S_OK;
    BOOL bReuseSwapchain = FALSE;
    BOOL fNeedRtPresentSwitch = FALSE;

    if (pSwapchain->fFlags.bSwitchReportingPresent || pSwapchain->cRTs > VBOXWDDMDISP_MAX_DIRECT_RTS)
    {
        pSwapchain->fFlags.bSwitchReportingPresent = FALSE;
        /* indicae switch to Render Target Reporting Present mode is needed */
        fNeedRtPresentSwitch = TRUE;
//        vboxWddmSwapchainSwtichRtPresent(pDevice, pSwapchain);
    }
    else
    {
        for (UINT i = 0; i < pSwapchain->cRTs; ++i)
        {
            if (pSwapchain->aRTs[i].pAlloc->enmD3DIfType != VBOXDISP_D3DIFTYPE_SURFACE
                    && (pSwapchain->aRTs[i].pAlloc->enmD3DIfType != VBOXDISP_D3DIFTYPE_UNDEFINED
                            || pSwapchain->aRTs[i].pAlloc->enmType != VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE
                            ))
            {
                fNeedRtPresentSwitch = TRUE;
                break;
            }
        }
    }

    /* check if we need to re-create the swapchain */
    if (pOldIf)
    {
        if (fNeedRtPresentSwitch)
        {
            /* the number of swapchain backbuffers does not matter */
            bReuseSwapchain = TRUE;
        }
        else
        {
            D3DPRESENT_PARAMETERS OldParams;
            hr = pOldIf->GetPresentParameters(&OldParams);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                if (OldParams.BackBufferCount == pSwapchain->cRTs-1)
                {
                    bReuseSwapchain = TRUE;
                }
            }
        }
    }

  /* first create the new one */
    IDirect3DSwapChain9 * pNewIf = NULL; /* Shut up MSC. */
    ///
    PVBOXWDDMDISP_ADAPTER pAdapter = pDevice->pAdapter;
#ifdef DEBUG
    UINT cSurfs = pSwapchain->cRTs;
#endif
    IDirect3DDevice9 *pDevice9If = NULL;
    HWND hOldWnd = pSwapchain->hWnd;
    if (!bReuseSwapchain)
    {
        VBOXWINEEX_D3DPRESENT_PARAMETERS Params;
        vboxWddmSwapchainFillPresentParams(pSwapchain, &Params.Base);
        Params.pHgsmi = NULL;

        if (hr == S_OK)
        {
            DWORD fFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

            Params.Base.hDeviceWindow = NULL;
                        /** @todo it seems there should be a way to detect this correctly since
                         * our vboxWddmDDevSetDisplayMode will be called in case we are using full-screen */
            Params.Base.Windowed = TRUE;
            //            params.EnableAutoDepthStencil = FALSE;
            //            params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
            //            params.Flags;
            //            params.FullScreen_RefreshRateInHz;
            //            params.FullScreen_PresentationInterval;
            if (!pDevice->pDevice9If)
            {
#ifdef VBOX_WITH_CRHGSMI
                Params.pHgsmi = &pDevice->Uhgsmi.BasePrivate.Base;
#else
                Params.pHgsmi = NULL;
#endif
                hr = pAdapter->D3D.pD3D9If->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, fFlags,
                                                         &Params.Base, &pDevice9If);
                Assert(hr == S_OK);
                if (hr == S_OK)
                {
                    Assert(Params.Base.hDeviceWindow);
                    pSwapchain->hWnd = Params.Base.hDeviceWindow;
                    pDevice->pDevice9If = pDevice9If;
                    hr = pDevice9If->GetSwapChain(0, &pNewIf);
                    Assert(hr == S_OK);
                    if (hr == S_OK)
                    {
                        Assert(pNewIf);
                    }
                    else
                    {
                        pDevice9If->Release();
                    }
                }
            }
            else
            {
                pDevice9If = pDevice->pDevice9If;

                if (pOldIf)
                {
                    /* need to copy data to offscreen rt to ensure the data is preserved
                     * since the swapchain data may become invalid once we create a new swapchain
                     * and pass the current swapchain's window to it
                     * thus vboxWddmSwapchainSynch will not be able to do synchronization */
                    hr = vboxWddmSwapchainSwtichOffscreenRt(pDevice, pSwapchain, FALSE);
                    Assert(hr == S_OK);
                }

                /* re-use swapchain window
                 * this will invalidate the previusly used swapchain */
                Params.Base.hDeviceWindow = pSwapchain->hWnd;

                hr = pDevice->pDevice9If->CreateAdditionalSwapChain(&Params.Base, &pNewIf);
                Assert(hr == S_OK);
                if (hr == S_OK)
                {
                    Assert(Params.Base.hDeviceWindow);
                    pSwapchain->hWnd = Params.Base.hDeviceWindow;
                    Assert(pNewIf);
                }
            }
        }
    }
    else
    {
        Assert(pOldIf);
        Assert(hOldWnd); NOREF(hOldWnd);
        pNewIf = pOldIf;
        /* to ensure the swapchain is not deleted once we release the pOldIf */
        pNewIf->AddRef();
    }

    if (FAILED(hr))
        return hr;

    Assert(pNewIf);
    pSwapchain->pSwapChainIf = pNewIf;

    if (fNeedRtPresentSwitch)
    {
        hr = vboxWddmSwapchainSwtichRtPresent(pDevice, pSwapchain);
    }
    else
    {
#ifndef VBOXWDDM_WITH_VISIBLE_FB
        if (!pSwapchain->fFlags.bRtReportingPresent)
        {
            pSwapchain->bRTFbCopyUpToDate = FALSE;
# if defined(VBOXDISP_WITH_WINE_BB_WORKAROUND) && defined(VBOX_WINE_WITH_FAST_INTERSWAPCHAIN_BLT)
            /* if wine is able to do fast fb->bb blits, we will use backbuffer directly,
             * this is NOT possible currently */
            if (pSwapchain->cRTs == 1)
            {
                /* we will assign it to wine backbuffer on a swapchain synch */
                if (pSwapchain->pRenderTargetFbCopy)
                {
                    pSwapchain->pRenderTargetFbCopy->Release();
                    pSwapchain->pRenderTargetFbCopy = NULL;
                }
            }
            else
# endif
            if (!pSwapchain->pRenderTargetFbCopy)
            {
                D3DPRESENT_PARAMETERS Params;
                vboxWddmSwapchainFillPresentParams(pSwapchain, &Params);
                IDirect3DSurface9* pD3D9Surf;
                hr = pDevice9If->CreateRenderTarget(
                                        Params.BackBufferWidth, Params.BackBufferHeight,
                                        Params.BackBufferFormat,
                                        Params.MultiSampleType,
                                        Params.MultiSampleQuality,
                                        TRUE, /*bLockable*/
                                        &pD3D9Surf,
                                        NULL /* HANDLE* pSharedHandle */
                                        );
                Assert(hr == S_OK);
                if (hr == S_OK)
                {
                    Assert(pD3D9Surf);
                    pSwapchain->pRenderTargetFbCopy = pD3D9Surf;
                }
            }
        }
#endif
    }

    /* ignore any subsequen failures */
    Assert(hr == S_OK);


#ifdef DEBUG
    for (UINT i = 0; i < cSurfs; ++i)
    {
        PVBOXWDDMDISP_RENDERTGT pRt = &pSwapchain->aRTs[i];
        Assert(pRt->pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE
                || pRt->pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_TEXTURE);
        Assert(pRt->pAlloc->pRc->RcDesc.enmPool != D3DDDIPOOL_SYSTEMMEM);
    }
#endif

    hr = vboxWddmSwapchainSynch(pDevice, pSwapchain);
    Assert(hr == S_OK);

    Assert(!pSwapchain->fFlags.bChanged);
    Assert(!pSwapchain->fFlags.bSwitchReportingPresent);
    if (pOldIf)
    {
        Assert(hOldWnd);
        pOldIf->Release();
    }
    else
    {
        Assert(!hOldWnd);
    }
    return S_OK;
}

static HRESULT vboxWddmSwapchainCreateIfForRc(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_RESOURCE pRc,
                                              PVBOXWDDMDISP_SWAPCHAIN *ppSwapchain)
{
    PVBOXWDDMDISP_SWAPCHAIN pSwapchain = vboxWddmSwapchainCreateForRc(pDevice, pRc);
    Assert(pSwapchain);
    *ppSwapchain = NULL;
    if (pSwapchain)
    {
        HRESULT hr = vboxWddmSwapchainChkCreateIf(pDevice, pSwapchain);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            *ppSwapchain = pSwapchain;
        }
        return hr;
    }
    return E_OUTOFMEMORY;
}

static HRESULT vboxWddmSwapchainPresentPerform(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain);

static HRESULT vboxWddmSwapchainBbUpdate(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain,
                                         PVBOXWDDMDISP_ALLOCATION pBbAlloc)
{
    Assert(!pSwapchain->fFlags.bRtReportingPresent);
    for (UINT i = 0; i < pSwapchain->cRTs; ++i)
    {
        PVBOXWDDMDISP_ALLOCATION pCurBb = vboxWddmSwapchainGetBb(pSwapchain)->pAlloc;
        if (pCurBb == pBbAlloc)
            return S_OK;

        HRESULT hr = vboxWddmSwapchainPresentPerform(pDevice, pSwapchain);
        if (FAILED(hr))
        {
            WARN(("vboxWddmSwapchainPresentPerform failed, hr (0x%x)", hr));
            return hr;
        }
    }

    AssertMsgFailed(("the given allocation not par of the swapchain\n"));
    return E_FAIL;
}

/* get the surface for the specified allocation in the swapchain */
static HRESULT vboxWddmSwapchainSurfGet(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain,
                                        PVBOXWDDMDISP_ALLOCATION pAlloc, IDirect3DSurface9 **ppSurf)
{
    RT_NOREF(pDevice);
    Assert(pAlloc->pSwapchain == pSwapchain);

#ifndef VBOXWDDM_WITH_VISIBLE_FB
    if (!pSwapchain->fFlags.bRtReportingPresent
            && vboxWddmSwapchainGetFb(pSwapchain)->pAlloc == pAlloc
# ifdef VBOXDISP_WITH_WINE_BB_WORKAROUND

            && vboxWddmSwapchainNumRTs(pSwapchain) != 1 /* for swapchains w/o a backbuffer the alloc will contain the back-buffer actually */
            )
    {
        /* this is a front-buffer */
        Assert(vboxWddmSwapchainNumRTs(pSwapchain) > 1);
        IDirect3DSurface9 *pSurf = pSwapchain->pRenderTargetFbCopy;
        Assert(pSurf);
        pSurf->AddRef();
        if (!pSwapchain->bRTFbCopyUpToDate)
        {
            HRESULT hr = pSwapchain->pSwapChainIf->GetFrontBufferData(pSurf);
            if (FAILED(hr))
            {
                WARN(("GetFrontBufferData failed, hr (0x%x)", hr));
                pSurf->Release();
                return hr;
            }
            pSwapchain->bRTFbCopyUpToDate = TRUE;
        }

        *ppSurf = pSurf;
        return S_OK;
    }
# endif
#endif

    /* if this is not a front-buffer - just return the surface associated with the allocation */
    return VBoxD3DIfSurfGet(pAlloc->pRc, pAlloc->iAlloc, ppSurf);
}

static HRESULT vboxWddmSwapchainRtSurfGet(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain, UINT iRt,
                                          PVBOXWDDMDISP_ALLOCATION pAlloc, BOOL bOnSwapchainSynch, IDirect3DSurface9 **ppSurf)
{
    Assert(pAlloc->pSwapchain == pSwapchain);
    HRESULT hr = S_OK;

    /* do the necessary swapchain synchronization first,
     * not needed on swapchain synch since it is done already and we're called here just to set RTs */
    if (!bOnSwapchainSynch)
    {

        if (!pSwapchain->fFlags.bRtReportingPresent)
        {
            /* iRt != 0 is untested here !! */
            Assert(iRt == 0);
            if (iRt == 0)
            {
                hr = vboxWddmSwapchainBbUpdate(pDevice, pSwapchain, pAlloc);
                if (FAILED(hr))
                {
                    WARN(("vboxWddmSwapchainBbUpdate failed, hr(0x%x)",hr));
                    return hr;
                }
            }
        }

/// @todo        Assert(!pSwapchain->fFlags.bChanged);
        Assert(pSwapchain->pSwapChainIf);
        hr = vboxWddmSwapchainChkCreateIf(pDevice, pSwapchain);
        if (FAILED(hr))
        {
            WARN(("vboxWddmSwapchainChkCreateIf failed, hr(0x%x)",hr));
            return hr;
        }
    }

/// @todo    Assert(vboxWddmSwapchainGetBb(pSwapchain)->pAlloc == pAlloc || iRt != 0);
    IDirect3DSurface9 *pSurf;
    hr = vboxWddmSwapchainSurfGet(pDevice, pSwapchain, pAlloc, &pSurf);
    if (FAILED(hr))
    {
        WARN(("vboxWddmSwapchainSurfGet failed, hr(0x%x)", hr));
        return hr;
    }

    *ppSurf = pSurf;
    return S_OK;

}

static HRESULT vboxWddmSwapchainPresentPerform(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_SWAPCHAIN pSwapchain)
{
    HRESULT hr;

    VBOXVDBG_DUMP_PRESENT_ENTER(pDevice, pSwapchain);

    if (!pSwapchain->fFlags.bRtReportingPresent)
    {
        hr = pSwapchain->pSwapChainIf->Present(NULL, NULL, NULL, NULL, 0);
        Assert(hr == S_OK);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        PVBOXWDDMDISP_ALLOCATION pCurBb = vboxWddmSwapchainGetBb(pSwapchain)->pAlloc;
        IDirect3DSurface9 *pSurf;
        hr = vboxWddmSwapchainSurfGet(pDevice, pSwapchain, pCurBb, &pSurf);
        Assert(hr == S_OK);
        if (FAILED(hr))
            return hr;
        hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DSwapchain9Present(pSwapchain->pSwapChainIf, pSurf);
        Assert(hr == S_OK);
        pSurf->Release();
        if (FAILED(hr))
            return hr;
    }

    VBOXVDBG_DUMP_PRESENT_LEAVE(pDevice, pSwapchain);

    pSwapchain->bRTFbCopyUpToDate = FALSE;
    vboxWddmSwapchainFlip(pSwapchain);
    Assert(!pSwapchain->fFlags.bChanged);
    Assert(!pSwapchain->fFlags.bSwitchReportingPresent);
    hr = vboxWddmSwapchainSynch(pDevice, pSwapchain);
    Assert(hr == S_OK);
    return hr;
}

static HRESULT vboxWddmSwapchainPresent(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_ALLOCATION pBbAlloc)
{
    /* we currently *assume* that presenting shared resource is only possible when 3d app is rendering with composited desktop on,
     * no need to do anything else since dwm will present everything for us */
    if (pBbAlloc->hSharedHandle)
    {
        VBOXVDBG_ASSERT_IS_DWM(FALSE);

        HRESULT hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9Flush((IDirect3DDevice9Ex*)pDevice->pDevice9If);
        Assert(hr == S_OK); NOREF(hr);

        vboxWddmDalNotifyChange(pDevice);

        return S_OK;
    }

    VBOXVDBG_ASSERT_IS_DWM(TRUE);

    BOOL bNeedPresent;
    PVBOXWDDMDISP_SWAPCHAIN pSwapchain = vboxWddmSwapchainFindCreate(pDevice, pBbAlloc, &bNeedPresent);
    Assert(pSwapchain);
    if (!bNeedPresent)
        return S_OK;
    if (pSwapchain)
    {
        HRESULT hr = vboxWddmSwapchainChkCreateIf(pDevice, pSwapchain);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            hr = vboxWddmSwapchainPresentPerform(pDevice, pSwapchain);
            Assert(hr == S_OK);
        }
        return hr;
    }
    return E_OUTOFMEMORY;
}

#if 0 //def DEBUG
static void vboxWddmDbgRenderTargetUpdateCheckSurface(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_ALLOCATION pAlloc, uint32_t iBBuf)
{
    IDirect3DSurface9 *pD3D9Surf;
    Assert(pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE);
    IDirect3DDevice9 * pDevice9If = pDevice->aScreens[pDevice->iPrimaryScreen].pDevice9If;
    HRESULT hr = pDevice9If->GetBackBuffer(0 /*UINT iSwapChain*/,
            iBBuf, D3DBACKBUFFER_TYPE_MONO, &pD3D9Surf);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        Assert(pD3D9Surf);
        Assert(pD3D9Surf == pAlloc->pD3DIf);
        pD3D9Surf->Release();
    }
}

static void vboxWddmDbgRenderTargetCheck(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_RESOURCE pRc, uint32_t iNewRTFB)
{
    PVBOXWDDMDISP_ALLOCATION pAlloc;
    UINT iBBuf = 0;
    Assert(iNewRTFB < pRc->cAllocations);

    for (UINT i = 1; i < pRc->cAllocations; ++i, ++iBBuf)
    {
        UINT iAlloc = (iNewRTFB + i) % pRc->cAllocations;
        Assert(iAlloc != iNewRTFB);
        pAlloc = &pRc->aAllocations[iAlloc];
        vboxWddmDbgRenderTargetUpdateCheckSurface(pDevice, pAlloc, iBBuf);
    }

    pAlloc = &pRc->aAllocations[iNewRTFB];
#ifdef VBOXWDDM_WITH_VISIBLE_FB
    vboxWddmDbgRenderTargetUpdateCheckSurface(pDevice, pAlloc, ~0UL /* <- for the frontbuffer */);
#else
    Assert((!pAlloc->pD3DIf) == (pRc->cAllocations > 1));
#endif

    for (UINT i = 0; i < pRc->cAllocations; ++i)
    {
        pAlloc = &pRc->aAllocations[i];
        if (iNewRTFB == i)
        {
            Assert((!pAlloc->pD3DIf) == (pRc->cAllocations > 1));
        }

        for (UINT j = i+1; j < pRc->cAllocations; ++j)
        {
            PVBOXWDDMDISP_ALLOCATION pAllocJ = &pRc->aAllocations[j];
            Assert(pAlloc->pD3DIf != pAllocJ->pD3DIf);
        }
    }
}

# define VBOXVDBG_RTGT_STATECHECK(_pDev) (vboxWddmDbgRenderTargetCheck((_pDev), (_pDev)->aScreens[(_pDev)->iPrimaryScreen].pRenderTargetRc, (_pDev)->aScreens[(_pDev)->iPrimaryScreen].iRenderTargetFrontBuf))
#else
# define VBOXVDBG_RTGT_STATECHECK(_pDev) do{}while(0)
#endif

/******/

static HRESULT vboxWddmRenderTargetSet(PVBOXWDDMDISP_DEVICE pDevice, UINT iRt, PVBOXWDDMDISP_ALLOCATION pAlloc,
                                       BOOL bOnSwapchainSynch)
{
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = S_OK;
    IDirect3DSurface9 *pD3D9Surf = NULL;
    if (pAlloc)
    {
        PVBOXWDDMDISP_SWAPCHAIN pSwapchain = vboxWddmSwapchainForAlloc(pAlloc);
        if (pSwapchain)
        {
            hr = vboxWddmSwapchainRtSurfGet(pDevice, pSwapchain, iRt, pAlloc, bOnSwapchainSynch, &pD3D9Surf);
            if (FAILED(hr))
            {
                WARN(("vboxWddmSwapchainRtSurfGet failed, hr(0x%x)",hr));
                return hr;
            }
        }
        else
        {
            hr = VBoxD3DIfSurfGet(pAlloc->pRc, pAlloc->iAlloc, &pD3D9Surf);
            if (FAILED(hr))
            {
                WARN(("VBoxD3DIfSurfGet failed, hr(0x%x)",hr));
                return hr;
            }
        }

        Assert(pD3D9Surf);
    }

    hr = pDevice9If->SetRenderTarget(iRt, pD3D9Surf);
    if (hr == S_OK)
    {
        Assert(iRt < pDevice->cRTs);
        pDevice->apRTs[iRt] = pAlloc;
    }
    else
    {
        /** @todo This is workaround for wine 1 render target. */
        if (!pAlloc)
        {
            pDevice->apRTs[iRt] = NULL;
            hr = S_OK;
        }
        else
        {
            AssertFailed();
        }
    }

    if (pD3D9Surf)
    {
        pD3D9Surf->Release();
    }

    return hr;
}

/**
 * DLL entry point.
 */
BOOL WINAPI DllMain(HINSTANCE hInstance,
                    DWORD     dwReason,
                    LPVOID    lpReserved)
{
    RT_NOREF(hInstance, lpReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            vboxDispLockInit();

            vboxVDbgPrint(("VBoxDispD3D: DLL loaded.\n"));
#ifdef VBOXWDDMDISP_DEBUG_VEHANDLER
            vboxVDbgVEHandlerRegister();
#endif
            int rc = RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
//                rc = VbglR3Init();
//                AssertRC(rc);
//                if (RT_SUCCESS(rc))
                {
                    HRESULT hr = vboxDispCmInit();
                    Assert(hr == S_OK);
                    if (hr == S_OK)
                    {
                        VBoxDispD3DGlobalInit();
                        vboxVDbgPrint(("VBoxDispD3D: DLL loaded OK\n"));
                        return TRUE;
                    }
//                    VbglR3Term();
                }
            }

#ifdef VBOXWDDMDISP_DEBUG_VEHANDLER
            vboxVDbgVEHandlerUnregister();
#endif
            break;
        }

        case DLL_PROCESS_DETACH:
        {
#ifdef VBOXWDDMDISP_DEBUG_VEHANDLER
            vboxVDbgVEHandlerUnregister();
#endif
            HRESULT hr = vboxDispCmTerm();
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
//                    VbglR3Term();
                /// @todo RTR3Term();
                VBoxDispD3DGlobalTerm();
                return TRUE;
            }

            break;
        }

        default:
            return TRUE;
    }
    return FALSE;
}

static HRESULT APIENTRY vboxWddmDispGetCaps (HANDLE hAdapter, CONST D3DDDIARG_GETCAPS* pData)
{
        VBOXDISP_DDI_PROLOGUE_ADP(hAdapter);

    vboxVDbgPrint(("==> "__FUNCTION__", hAdapter(0x%p), caps type(%d)\n", hAdapter, pData->Type));

    VBOXDISPCRHGSMI_SCOPE_SET_GLOBAL();

    HRESULT hr = S_OK;
    PVBOXWDDMDISP_ADAPTER pAdapter = (PVBOXWDDMDISP_ADAPTER)hAdapter;

    switch (pData->Type)
    {
        case D3DDDICAPS_DDRAW:
        {
            Assert(!VBOXDISPMODE_IS_3D(pAdapter));
            Assert(pData->DataSize == sizeof (DDRAW_CAPS));
            if (pData->DataSize >= sizeof (DDRAW_CAPS))
            {
                memset(pData->pData, 0, sizeof (DDRAW_CAPS));
#ifdef VBOX_WITH_VIDEOHWACCEL
                if (!VBOXDISPMODE_IS_3D(pAdapter))
                {
                    if (vboxVhwaHasCKeying(pAdapter))
                    {
                        DDRAW_CAPS *pCaps = (DDRAW_CAPS*)pData->pData;
                        pCaps->Caps |= DDRAW_CAPS_COLORKEY;
    //                    pCaps->Caps2 |= DDRAW_CAPS2_FLIPNOVSYNC;
                    }
                }
                else
                {
                    WARN(("D3DDDICAPS_DDRAW query for D3D mode!"));
                }
#endif
            }
            else
                hr = E_INVALIDARG;
            break;
        }
        case D3DDDICAPS_DDRAW_MODE_SPECIFIC:
        {
            Assert(!VBOXDISPMODE_IS_3D(pAdapter));
            Assert(pData->DataSize == sizeof (DDRAW_MODE_SPECIFIC_CAPS));
            if (pData->DataSize >= sizeof (DDRAW_MODE_SPECIFIC_CAPS))
            {
                DDRAW_MODE_SPECIFIC_CAPS * pCaps = (DDRAW_MODE_SPECIFIC_CAPS*)pData->pData;
                memset(&pCaps->Caps /* do not cleanup the first "Head" field,
                                    zero starting with the one following "Head", i.e. Caps */,
                        0, sizeof (DDRAW_MODE_SPECIFIC_CAPS) - RT_OFFSETOF(DDRAW_MODE_SPECIFIC_CAPS, Caps));
#ifdef VBOX_WITH_VIDEOHWACCEL
                if (!VBOXDISPMODE_IS_3D(pAdapter))
                {
                    VBOXVHWA_INFO *pSettings = &pAdapter->aHeads[pCaps->Head].Vhwa.Settings;
                    if (pSettings->fFlags & VBOXVHWA_F_ENABLED)
                    {
                        pCaps->Caps |= MODE_CAPS_OVERLAY | MODE_CAPS_OVERLAYSTRETCH;

                        if (pSettings->fFlags & VBOXVHWA_F_CKEY_DST)
                        {
                            pCaps->CKeyCaps |= MODE_CKEYCAPS_DESTOVERLAY
                                    | MODE_CKEYCAPS_DESTOVERLAYYUV /* ?? */
                                    ;
                        }

                        if (pSettings->fFlags & VBOXVHWA_F_CKEY_SRC)
                        {
                            pCaps->CKeyCaps |= MODE_CKEYCAPS_SRCOVERLAY
                                    | MODE_CKEYCAPS_SRCOVERLAYCLRSPACE /* ?? */
                                    | MODE_CKEYCAPS_SRCOVERLAYCLRSPACEYUV /* ?? */
                                    | MODE_CKEYCAPS_SRCOVERLAYYUV /* ?? */
                                    ;
                        }

                        pCaps->FxCaps = MODE_FXCAPS_OVERLAYSHRINKX
                                | MODE_FXCAPS_OVERLAYSHRINKY
                                | MODE_FXCAPS_OVERLAYSTRETCHX
                                | MODE_FXCAPS_OVERLAYSTRETCHY;


                        pCaps->MaxVisibleOverlays = pSettings->cOverlaysSupported;
                        pCaps->MinOverlayStretch = 1;
                        pCaps->MaxOverlayStretch = 32000;
                    }
                }
                else
                {
                    WARN(("D3DDDICAPS_DDRAW_MODE_SPECIFIC query for D3D mode!"));
                }
#endif
            }
            else
                hr = E_INVALIDARG;
            break;
        }
        case D3DDDICAPS_GETFORMATCOUNT:
            *((uint32_t*)pData->pData) = pAdapter->Formats.cFormstOps;
            break;
        case D3DDDICAPS_GETFORMATDATA:
            Assert(pData->DataSize == pAdapter->Formats.cFormstOps * sizeof (FORMATOP));
            memcpy(pData->pData, pAdapter->Formats.paFormstOps, pAdapter->Formats.cFormstOps * sizeof (FORMATOP));
            break;
        case D3DDDICAPS_GETD3DQUERYCOUNT:
#if 1
            *((uint32_t*)pData->pData) = VBOX_QUERYTYPE_COUNT();
#else
            *((uint32_t*)pData->pData) = 0;
#endif
            break;
        case D3DDDICAPS_GETD3DQUERYDATA:
#if 1
            Assert(pData->DataSize == VBOX_QUERYTYPE_COUNT() * sizeof (D3DDDIQUERYTYPE));
            memcpy(pData->pData, gVBoxQueryTypes, VBOX_QUERYTYPE_COUNT() * sizeof (D3DDDIQUERYTYPE));
#else
            Assert(0);
            memset(pData->pData, 0, pData->DataSize);
#endif
            break;
        case D3DDDICAPS_GETD3D3CAPS:
            Assert(!VBOXDISPMODE_IS_3D(pAdapter));
            Assert(pData->DataSize == sizeof (D3DHAL_GLOBALDRIVERDATA));
            if (pData->DataSize >= sizeof (D3DHAL_GLOBALDRIVERDATA))
            {
                D3DHAL_GLOBALDRIVERDATA *pCaps = (D3DHAL_GLOBALDRIVERDATA *)pData->pData;
                memset (pCaps, 0, sizeof (D3DHAL_GLOBALDRIVERDATA));
                pCaps->dwSize = sizeof (D3DHAL_GLOBALDRIVERDATA);
                pCaps->hwCaps.dwSize = sizeof (D3DDEVICEDESC_V1);
                pCaps->hwCaps.dwFlags = D3DDD_COLORMODEL
                        | D3DDD_DEVCAPS
                        | D3DDD_DEVICERENDERBITDEPTH;

                pCaps->hwCaps.dcmColorModel = D3DCOLOR_RGB;
                pCaps->hwCaps.dwDevCaps = D3DDEVCAPS_CANRENDERAFTERFLIP
//                        | D3DDEVCAPS_DRAWPRIMTLVERTEX
                        | D3DDEVCAPS_EXECUTESYSTEMMEMORY
                        | D3DDEVCAPS_EXECUTEVIDEOMEMORY
//                        | D3DDEVCAPS_FLOATTLVERTEX
                        | D3DDEVCAPS_HWRASTERIZATION
//                        | D3DDEVCAPS_HWTRANSFORMANDLIGHT
//                        | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY
//                        | D3DDEVCAPS_TEXTUREVIDEOMEMORY
                        ;
                pCaps->hwCaps.dtcTransformCaps.dwSize = sizeof (D3DTRANSFORMCAPS);
                pCaps->hwCaps.dtcTransformCaps.dwCaps = 0;
                pCaps->hwCaps.bClipping = FALSE;
                pCaps->hwCaps.dlcLightingCaps.dwSize = sizeof (D3DLIGHTINGCAPS);
                pCaps->hwCaps.dlcLightingCaps.dwCaps = 0;
                pCaps->hwCaps.dlcLightingCaps.dwLightingModel = 0;
                pCaps->hwCaps.dlcLightingCaps.dwNumLights = 0;
                pCaps->hwCaps.dpcLineCaps.dwSize = sizeof (D3DPRIMCAPS);
                pCaps->hwCaps.dpcLineCaps.dwMiscCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwRasterCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwZCmpCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwSrcBlendCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwDestBlendCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwAlphaCmpCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwShadeCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwTextureCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwTextureFilterCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwTextureBlendCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwTextureAddressCaps = 0;
                pCaps->hwCaps.dpcLineCaps.dwStippleWidth = 0;
                pCaps->hwCaps.dpcLineCaps.dwStippleHeight = 0;

                pCaps->hwCaps.dpcTriCaps.dwSize = sizeof (D3DPRIMCAPS);
                pCaps->hwCaps.dpcTriCaps.dwMiscCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwRasterCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwZCmpCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwSrcBlendCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwDestBlendCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwAlphaCmpCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwShadeCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwTextureCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwTextureFilterCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwTextureBlendCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwTextureAddressCaps = 0;
                pCaps->hwCaps.dpcTriCaps.dwStippleWidth = 0;
                pCaps->hwCaps.dpcTriCaps.dwStippleHeight = 0;
                pCaps->hwCaps.dwDeviceRenderBitDepth = DDBD_8 | DDBD_16 | DDBD_24 | DDBD_32;
                pCaps->hwCaps.dwDeviceZBufferBitDepth = 0;
                pCaps->hwCaps.dwMaxBufferSize = 0;
                pCaps->hwCaps.dwMaxVertexCount = 0;


                pCaps->dwNumVertices = 0;
                pCaps->dwNumClipVertices = 0;
                pCaps->dwNumTextureFormats = 0;//pAdapter->cSurfDescs;
                pCaps->lpTextureFormats = NULL;//pAdapter->paSurfDescs;
            }
            else
                hr = E_INVALIDARG;
            break;
        case D3DDDICAPS_GETD3D7CAPS:
            Assert(!VBOXDISPMODE_IS_3D(pAdapter));
            Assert(pData->DataSize == sizeof (D3DHAL_D3DEXTENDEDCAPS));
            if (pData->DataSize >= sizeof (D3DHAL_D3DEXTENDEDCAPS))
            {
                memset(pData->pData, 0, sizeof (D3DHAL_D3DEXTENDEDCAPS));
                D3DHAL_D3DEXTENDEDCAPS *pCaps = (D3DHAL_D3DEXTENDEDCAPS*)pData->pData;
                pCaps->dwSize = sizeof (D3DHAL_D3DEXTENDEDCAPS);
            }
            else
                hr = E_INVALIDARG;
            break;
        case D3DDDICAPS_GETD3D9CAPS:
        {
            Assert(pData->DataSize == sizeof (D3DCAPS9));
            if (pData->DataSize >= sizeof (D3DCAPS9))
            {
                Assert(VBOXDISPMODE_IS_3D(pAdapter));
                if (VBOXDISPMODE_IS_3D(pAdapter))
                {
                    memcpy(pData->pData, &pAdapter->D3D.Caps, sizeof (D3DCAPS9));
                    hr = S_OK;
                    break;
                }

                memset(pData->pData, 0, sizeof (D3DCAPS9));
            }
            else
                hr = E_INVALIDARG;
            break;
        }
        case D3DDDICAPS_GETD3D8CAPS:
        {
            Assert(pData->DataSize == RT_OFFSETOF(D3DCAPS9, DevCaps2));
            if (pData->DataSize == RT_OFFSETOF(D3DCAPS9, DevCaps2))
            {
                Assert(VBOXDISPMODE_IS_3D(pAdapter));
                if (VBOXDISPMODE_IS_3D(pAdapter))
                {
                    memcpy(pData->pData, &pAdapter->D3D.Caps, RT_OFFSETOF(D3DCAPS9, DevCaps2));
                    hr = S_OK;
                    break;
                }
            }
            else
                hr = E_INVALIDARG;
            break;
        }
        case D3DDDICAPS_GETGAMMARAMPCAPS:
            *((uint32_t*)pData->pData) = 0;
            break;
        case D3DDDICAPS_GETVIDEOPROCESSORCAPS:
        case D3DDDICAPS_GETEXTENSIONGUIDCOUNT:
        case D3DDDICAPS_GETDECODEGUIDCOUNT:
        case D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDCOUNT:
        case D3DDDICAPS_GETVIDEOPROCESSORRTFORMATCOUNT:
            if (pData->pData && pData->DataSize)
                memset(pData->pData, 0, pData->DataSize);
            break;
        case D3DDDICAPS_GETMULTISAMPLEQUALITYLEVELS:
        case D3DDDICAPS_GETD3D5CAPS:
        case D3DDDICAPS_GETD3D6CAPS:
        case D3DDDICAPS_GETDECODEGUIDS:
        case D3DDDICAPS_GETDECODERTFORMATCOUNT:
        case D3DDDICAPS_GETDECODERTFORMATS:
        case D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFOCOUNT:
        case D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFO:
        case D3DDDICAPS_GETDECODECONFIGURATIONCOUNT:
        case D3DDDICAPS_GETDECODECONFIGURATIONS:
        case D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDS:
        case D3DDDICAPS_GETVIDEOPROCESSORRTFORMATS:
        case D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATCOUNT:
        case D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATS:
        case D3DDDICAPS_GETPROCAMPRANGE:
        case D3DDDICAPS_FILTERPROPERTYRANGE:
        case D3DDDICAPS_GETEXTENSIONGUIDS:
        case D3DDDICAPS_GETEXTENSIONCAPS:
            vboxVDbgPrint((__FUNCTION__": unimplemented caps type(%d)\n", pData->Type));
            Assert(0);
            if (pData->pData && pData->DataSize)
                memset(pData->pData, 0, pData->DataSize);
            break;
        default:
            vboxVDbgPrint((__FUNCTION__": unknown caps type(%d)\n", pData->Type));
            Assert(0);
    }

    vboxVDbgPrint(("<== "__FUNCTION__", hAdapter(0x%p), caps type(%d)\n", hAdapter, pData->Type));

    return S_OK;
}

static HRESULT APIENTRY vboxWddmDDevSetRenderState(HANDLE hDevice, CONST D3DDDIARG_RENDERSTATE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetRenderState(vboxDDI2D3DRenderStateType(pData->State), pData->Value);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevUpdateWInfo(HANDLE hDevice, CONST D3DDDIARG_WINFO* pData)
{
    RT_NOREF(hDevice, pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
//    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
//    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return S_OK;
}

static HRESULT APIENTRY vboxWddmDDevValidateDevice(HANDLE hDevice, D3DDDIARG_VALIDATETEXTURESTAGESTATE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
//    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
//    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
#ifdef DEBUG_misha
    /** @todo check if it's ok to always return success */
    vboxVDbgPrint((__FUNCTION__": @todo: check if it's ok to always return success\n"));
    Assert(0);
#endif
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return S_OK;
}

static HRESULT APIENTRY vboxWddmDDevSetTextureStageState(HANDLE hDevice, CONST D3DDDIARG_TEXTURESTAGESTATE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);

    VBOXWDDMDISP_TSS_LOOKUP lookup = vboxDDI2D3DTestureStageStateType(pData->State);
    HRESULT hr;

    if (!lookup.bSamplerState)
    {
        hr = pDevice9If->SetTextureStageState(pData->Stage, D3DTEXTURESTAGESTATETYPE(lookup.dType), pData->Value);
    }
    else
    {
        hr = pDevice9If->SetSamplerState(pData->Stage, D3DSAMPLERSTATETYPE(lookup.dType), pData->Value);
    }

    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevSetTexture(HANDLE hDevice, UINT Stage, HANDLE hTexture)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)hTexture;
//    Assert(pRc);
    IDirect3DBaseTexture9 *pD3DIfTex;
    if (pRc)
    {
        VBOXVDBG_CHECK_SMSYNC(pRc);
        if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_TEXTURE)
        {
            pD3DIfTex = (IDirect3DTexture9*)pRc->aAllocations[0].pD3DIf;

            VBOXVDBG_BREAK_SHARED(pRc);
            VBOXVDBG_DUMP_SETTEXTURE(pRc);
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_CUBE_TEXTURE)
        {
            pD3DIfTex = (IDirect3DCubeTexture9*)pRc->aAllocations[0].pD3DIf;

            VBOXVDBG_BREAK_SHARED(pRc);
            VBOXVDBG_DUMP_SETTEXTURE(pRc);
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE)
        {
            pD3DIfTex = (IDirect3DVolumeTexture9*)pRc->aAllocations[0].pD3DIf;

            VBOXVDBG_BREAK_SHARED(pRc);
            VBOXVDBG_DUMP_SETTEXTURE(pRc);
        }
        else
        {
            Assert(0);
            pD3DIfTex = NULL;
        }

        Assert(pDevice->cSamplerTextures < RT_ELEMENTS(pDevice->aSamplerTextures));
        int idx = VBOXWDDMDISP_SAMPLER_IDX(Stage);
        if (idx >= 0)
        {
            Assert(idx < RT_ELEMENTS(pDevice->aSamplerTextures));
#ifdef DEBUG_misha
            if (VBOXWDDMDISP_SAMPLER_IDX_IS_SPECIAL(Stage))
            {
                WARN(("non-zero special sampler index not tested!\n"));
            }
#endif
            if (!pDevice->aSamplerTextures[idx])
            {
                ++pDevice->cSamplerTextures;
            }
            Assert(pDevice->cSamplerTextures < RT_ELEMENTS(pDevice->aSamplerTextures));
            pDevice->aSamplerTextures[idx] = pRc;
        }
        else
        {
            WARN(("incorrect dampler index1! (%d)\n", Stage));
        }
    }
    else
    {
        pD3DIfTex = NULL;
        Assert(pDevice->cSamplerTextures < RT_ELEMENTS(pDevice->aSamplerTextures));
        int idx = VBOXWDDMDISP_SAMPLER_IDX(Stage);
        if (idx >= 0)
        {
            Assert(idx < RT_ELEMENTS(pDevice->aSamplerTextures));
            if (pDevice->aSamplerTextures[idx])
            {
                Assert(pDevice->cSamplerTextures);
                --pDevice->cSamplerTextures;
            }
            Assert(pDevice->cSamplerTextures < RT_ELEMENTS(pDevice->aSamplerTextures));
            pDevice->aSamplerTextures[idx] = NULL;
        }
        else
        {
            WARN(("incorrect dampler index2! (%d)\n", Stage));
        }
    }

    HRESULT hr = pDevice9If->SetTexture(Stage, pD3DIfTex);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevSetPixelShader(HANDLE hDevice, HANDLE hShaderHandle)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    IDirect3DPixelShader9 *pShader = (IDirect3DPixelShader9*)hShaderHandle;
    HRESULT hr = pDevice9If->SetPixelShader(pShader);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevSetPixelShaderConst(HANDLE hDevice, CONST D3DDDIARG_SETPIXELSHADERCONST* pData,
                                                        CONST FLOAT* pRegisters)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetPixelShaderConstantF(pData->Register, pRegisters, pData->Count);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevSetStreamSourceUm(HANDLE hDevice, CONST D3DDDIARG_SETSTREAMSOURCEUM* pData,
                                                      CONST VOID* pUMBuffer )
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    HRESULT hr = S_OK;

    Assert(pData->Stream < RT_ELEMENTS(pDevice->aStreamSourceUm));
    PVBOXWDDMDISP_STREAMSOURCEUM pStrSrcUm = &pDevice->aStreamSourceUm[pData->Stream];
    if (pStrSrcUm->pvBuffer && !pUMBuffer)
    {
        --pDevice->cStreamSourcesUm;
        Assert(pDevice->cStreamSourcesUm < UINT32_MAX/2);
    }
    else if (!pStrSrcUm->pvBuffer && pUMBuffer)
    {
        ++pDevice->cStreamSourcesUm;
        Assert(pDevice->cStreamSourcesUm <= RT_ELEMENTS(pDevice->aStreamSourceUm));
    }

    pStrSrcUm->pvBuffer = pUMBuffer;
    pStrSrcUm->cbStride = pData->Stride;

    if (pDevice->aStreamSource[pData->Stream])
    {
        hr = pDevice->pDevice9If->SetStreamSource(pData->Stream, NULL, 0, 0);
        --pDevice->cStreamSources;
        Assert(pDevice->cStreamSources < UINT32_MAX/2);
        pDevice->aStreamSource[pData->Stream] = NULL;
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevSetIndices(HANDLE hDevice, CONST D3DDDIARG_SETINDICES* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hIndexBuffer;
    PVBOXWDDMDISP_ALLOCATION pAlloc = NULL;
    IDirect3DIndexBuffer9 *pIndexBuffer = NULL;
    if (pRc)
    {
        VBOXVDBG_CHECK_SMSYNC(pRc);
        Assert(pRc->cAllocations == 1);
        pAlloc = &pRc->aAllocations[0];
        Assert(pAlloc->pD3DIf);
        pIndexBuffer = (IDirect3DIndexBuffer9*)pAlloc->pD3DIf;
    }
    HRESULT hr = pDevice9If->SetIndices(pIndexBuffer);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        pDevice->IndiciesInfo.pIndicesAlloc = pAlloc;
        pDevice->IndiciesInfo.uiStride = pData->Stride;
        pDevice->IndiciesInfo.pvIndicesUm = NULL;
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevSetIndicesUm(HANDLE hDevice, UINT IndexSize, CONST VOID* pUMBuffer)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);

    HRESULT hr = S_OK;
    if (pDevice->IndiciesInfo.pIndicesAlloc)
    {
        hr = pDevice9If->SetIndices(NULL);
    }

    if (SUCCEEDED(hr))
    {
        pDevice->IndiciesInfo.pvIndicesUm = pUMBuffer;
        pDevice->IndiciesInfo.uiStride = IndexSize;
        pDevice->IndiciesInfo.pIndicesAlloc = NULL;
        hr = S_OK;
    }
    else
    {
        WARN(("SetIndices failed hr 0x%x", hr));
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevDrawPrimitive(HANDLE hDevice, CONST D3DDDIARG_DRAWPRIMITIVE* pData, CONST UINT* pFlagBuffer)
{
    RT_NOREF(pFlagBuffer);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    Assert(!pFlagBuffer);
    HRESULT hr = S_OK;

    VBOXVDBG_BREAK_SHARED_DEV(pDevice);

    VBOXVDBG_DUMP_DRAWPRIM_ENTER(pDevice);

    if (pDevice->cStreamSourcesUm)
    {
#ifdef DEBUG
        uint32_t cStreams = 0;
        for (UINT i = 0; i < RT_ELEMENTS(pDevice->aStreamSourceUm); ++i)
        {
            if(pDevice->aStreamSourceUm[i].pvBuffer)
            {
                ++cStreams;
            }
        }

        Assert(cStreams);
        Assert(cStreams == pDevice->cStreamSourcesUm);
#endif
        if (pDevice->cStreamSourcesUm == 1)
        {
            for (UINT i = 0; i < RT_ELEMENTS(pDevice->aStreamSourceUm); ++i)
            {
                if(pDevice->aStreamSourceUm[i].pvBuffer)
                {
                    hr = pDevice9If->DrawPrimitiveUP(pData->PrimitiveType,
                                                     pData->PrimitiveCount,
                                                       ((uint8_t*)pDevice->aStreamSourceUm[i].pvBuffer)
                                                     + pData->VStart * pDevice->aStreamSourceUm[i].cbStride,
                                                     pDevice->aStreamSourceUm[i].cbStride);
                    Assert(hr == S_OK);
                    break;
                }
            }
        }
        else
        {
            /** @todo impl */
            WARN(("multiple user stream sources (%d) not implemented!!", pDevice->cStreamSourcesUm));
        }
    }
    else
    {

#ifdef DEBUG
        Assert(!pDevice->cStreamSourcesUm);
            for (UINT i = 0; i < RT_ELEMENTS(pDevice->aStreamSourceUm); ++i)
            {
                Assert(!pDevice->aStreamSourceUm[i].pvBuffer);
            }

            uint32_t cStreams = 0;
            for (UINT i = 0; i < RT_ELEMENTS(pDevice->aStreamSource); ++i)
            {
                if (pDevice->aStreamSource[i])
                {
                    ++cStreams;
                    Assert(!pDevice->aStreamSource[i]->LockInfo.cLocks);
                }
            }

            Assert(cStreams);
            Assert(cStreams == pDevice->cStreamSources);
#endif
        hr = pDevice9If->DrawPrimitive(pData->PrimitiveType,
                                                pData->VStart,
                                                pData->PrimitiveCount);
        Assert(hr == S_OK);

//        vboxVDbgMpPrintF((pDevice, __FUNCTION__": DrawPrimitive\n"));
    }

    vboxWddmDalCheckAddOnDraw(pDevice);

    VBOXVDBG_DUMP_DRAWPRIM_LEAVE(pDevice);

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevDrawIndexedPrimitive(HANDLE hDevice, CONST D3DDDIARG_DRAWINDEXEDPRIMITIVE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);

    VBOXVDBG_BREAK_SHARED_DEV(pDevice);

    VBOXVDBG_DUMP_DRAWPRIM_ENTER(pDevice);


#ifdef DEBUG
    uint32_t cStreams = 0;
    for (UINT i = 0; i < RT_ELEMENTS(pDevice->aStreamSourceUm); ++i)
    {
        if(pDevice->aStreamSourceUm[i].pvBuffer)
            ++cStreams;
    }

    Assert(cStreams == pDevice->cStreamSourcesUm);

    cStreams = 0;

    for (UINT i = 0; i < RT_ELEMENTS(pDevice->aStreamSource); ++i)
    {
        if (pDevice->aStreamSource[i])
        {
            ++cStreams;
            Assert(!pDevice->aStreamSource[i]->LockInfo.cLocks);
        }
    }

    Assert(cStreams == pDevice->cStreamSources);
#endif

    HRESULT hr;

    if (pDevice->cStreamSources)
    {
        Assert(pDevice->IndiciesInfo.pIndicesAlloc);
        Assert(!pDevice->IndiciesInfo.pvIndicesUm);
        Assert(!pDevice->IndiciesInfo.pIndicesAlloc->LockInfo.cLocks);
        Assert(!pDevice->cStreamSourcesUm);

        hr = pDevice9If->DrawIndexedPrimitive(
                pData->PrimitiveType,
                pData->BaseVertexIndex,
                pData->MinIndex,
                pData->NumVertices,
                pData->StartIndex,
                pData->PrimitiveCount);

        if(SUCCEEDED(hr))
            hr = S_OK;
        else
            WARN(("DrawIndexedPrimitive failed hr = 0x%x", hr));
    }
    else
    {
        Assert(pDevice->cStreamSourcesUm == 1);
        Assert(pDevice->IndiciesInfo.uiStride == 2 || pDevice->IndiciesInfo.uiStride == 4);
        const uint8_t * pvIndexBuffer;
        hr = S_OK;

        if (pDevice->IndiciesInfo.pIndicesAlloc)
        {
            Assert(!pDevice->IndiciesInfo.pvIndicesUm);
            if (pDevice->IndiciesInfo.pIndicesAlloc->pvMem)
                pvIndexBuffer = (const uint8_t*)pDevice->IndiciesInfo.pIndicesAlloc->pvMem;
            else
            {
                WARN(("not expected!"));
                hr = E_FAIL;
                pvIndexBuffer = NULL;
            }
        }
        else
        {
            pvIndexBuffer = (const uint8_t*)pDevice->IndiciesInfo.pvIndicesUm;
            if (!pvIndexBuffer)
            {
                WARN(("not expected!"));
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            for (UINT i = 0; i < RT_ELEMENTS(pDevice->aStreamSourceUm); ++i)
            {
                if(pDevice->aStreamSourceUm[i].pvBuffer)
                {
                    hr = pDevice9If->DrawIndexedPrimitiveUP(pData->PrimitiveType,
                                    pData->MinIndex,
                                    pData->NumVertices,
                                    pData->PrimitiveCount,
                                    pvIndexBuffer + pDevice->IndiciesInfo.uiStride * pData->StartIndex,
                                    pDevice->IndiciesInfo.uiStride == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
                                    pDevice->aStreamSourceUm[i].pvBuffer,
                                    pDevice->aStreamSourceUm[i].cbStride);
                    if(SUCCEEDED(hr))
                    {
                        if (pDevice->IndiciesInfo.pIndicesAlloc)
                        {
                            HRESULT hr2 = pDevice9If->SetIndices((IDirect3DIndexBuffer9*)pDevice->IndiciesInfo.pIndicesAlloc->pD3DIf);
                            if(!SUCCEEDED(hr2))
                                WARN(("SetIndices failed hr = 0x%x", hr2));
                        }

                        hr = S_OK;
                    }
                    else
                        WARN(("DrawIndexedPrimitiveUP failed hr = 0x%x", hr));
                    break;
                }
            }
        }
    }

    vboxWddmDalCheckAddOnDraw(pDevice);

    VBOXVDBG_DUMP_DRAWPRIM_LEAVE(pDevice);

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevDrawRectPatch(HANDLE hDevice, CONST D3DDDIARG_DRAWRECTPATCH* pData,
                                                  CONST D3DDDIRECTPATCH_INFO* pInfo, CONST FLOAT* pPatch)
{
    RT_NOREF(pData, pInfo, pPatch);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxWddmDalCheckAddOnDraw(pDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDDevDrawTriPatch(HANDLE hDevice, CONST D3DDDIARG_DRAWTRIPATCH* pData,
                                                 CONST D3DDDITRIPATCH_INFO* pInfo, CONST FLOAT* pPatch)
{
    RT_NOREF(pData, pInfo, pPatch);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxWddmDalCheckAddOnDraw(pDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDDevDrawPrimitive2(HANDLE hDevice, CONST D3DDDIARG_DRAWPRIMITIVE2* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = S_OK;
#pragma message("TODO: figure initial value of hr in vboxWddmDDevDrawPrimitive2, was being used uninitalized!")

#if 0
    int stream;
    for (stream=0; stream<VBOXWDDMDISP_MAX_VERTEX_STREAMS; ++stream)
    {
        if (pDevice->aStreamSource[stream] && pDevice->aStreamSource[stream]->LockInfo.cLocks)
        {
            VBOXWDDMDISP_LOCKINFO *pLock = &pDevice->aStreamSource[stream]->LockInfo;
            if (pLock->fFlags.MightDrawFromLocked && (pLock->fFlags.Discard || pLock->fFlags.NoOverwrite))
            {
                IDirect3DVertexBuffer9 *pD3D9VBuf = (IDirect3DVertexBuffer9*)pDevice->aStreamSource[stream]->pD3DIf;
                Assert(pLock->fFlags.RangeValid);
                pD3D9VBuf->Lock(pLock->Range.Offset, pLock->Range.Size,
                                &pLock->LockedRect.pBits,
                                vboxDDI2D3DLockFlags(pLock->fFlags));
                RECT r;
                r.top = 0;
                r.left = pLock->Range.Offset;
                r.bottom = 1;
                r.right = pLock->Range.Offset + pLock->Range.Size;

                VBoxD3DIfLockUnlockMemSynch(pDevice->aStreamSource[stream], &pLock->LockedRect, &r, true /*bool bToLockInfo*/);

                pD3D9VBuf->Unlock();
            }
        }
    }

    hr = pDevice9If->DrawPrimitive(pData->PrimitiveType, pData->FirstVertexOffset, pData->PrimitiveCount);
#else
    VBOXVDBG_BREAK_SHARED_DEV(pDevice);

    VBOXVDBG_DUMP_DRAWPRIM_ENTER(pDevice);

#ifdef DEBUG
    uint32_t cStreams = 0;
#endif

    int stream;
    for (stream=0; stream<VBOXWDDMDISP_MAX_VERTEX_STREAMS; ++stream)
    {
        if (pDevice->aStreamSource[stream])
        {
#ifdef DEBUG
            ++cStreams;
#endif
            Assert(stream==0); /*only stream 0 should be accessed here*/
            Assert(pDevice->StreamSourceInfo[stream].uiStride!=0);
            VBOXWDDMDISP_LOCKINFO *pLock = &pDevice->aStreamSource[stream]->LockInfo;

            if (pLock->cLocks)
            {
//                vboxVDbgMpPrintF((pDevice, __FUNCTION__": DrawPrimitiveUP\n"));

                Assert(pLock->fFlags.MightDrawFromLocked && (pLock->fFlags.Discard || pLock->fFlags.NoOverwrite));
                hr = pDevice9If->DrawPrimitiveUP(pData->PrimitiveType, pData->PrimitiveCount,
                                                 (void *)(  (uintptr_t)pDevice->aStreamSource[stream]->pvMem
                                                          + pDevice->StreamSourceInfo[stream].uiOffset + pData->FirstVertexOffset),
                                                 pDevice->StreamSourceInfo[stream].uiStride);
                Assert(hr == S_OK);
                hr = pDevice9If->SetStreamSource(stream, (IDirect3DVertexBuffer9*)pDevice->aStreamSource[stream]->pD3DIf,
                                                 pDevice->StreamSourceInfo[stream].uiOffset,
                                                 pDevice->StreamSourceInfo[stream].uiStride);
                Assert(hr == S_OK);
            }
            else
            {
//                vboxVDbgMpPrintF((pDevice, __FUNCTION__": DrawPrimitive\n"));

                hr = pDevice9If->DrawPrimitive(pData->PrimitiveType,
                                               pData->FirstVertexOffset / pDevice->StreamSourceInfo[stream].uiStride,
                                               pData->PrimitiveCount);
                Assert(hr == S_OK);
            }
        }
    }

#ifdef DEBUG
    Assert(cStreams);
    Assert(cStreams == pDevice->cStreamSources);
#endif
#endif

    vboxWddmDalCheckAddOnDraw(pDevice);

    VBOXVDBG_DUMP_DRAWPRIM_LEAVE(pDevice);

    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static UINT vboxWddmVertexCountFromPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
{
    Assert(PrimitiveCount > 0); /* Callers ensure this. */

    UINT cVertices;
    switch (PrimitiveType)
    {
        case D3DPT_POINTLIST:
            cVertices = PrimitiveCount;     /* Vertex per point. */
            break;

        case D3DPT_LINELIST:
            cVertices = PrimitiveCount * 2; /* Two vertices for each line. */
            break;

        case D3DPT_LINESTRIP:
            cVertices = PrimitiveCount + 1; /* Two vertices for the first line and one vertex for each subsequent. */
            break;

        case D3DPT_TRIANGLELIST:
            cVertices = PrimitiveCount * 3; /* Three vertices for each triangle. */
            break;

        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
            cVertices = PrimitiveCount + 2; /* Three vertices for the first triangle and one vertex for each subsequent. */
            break;

        default:
            cVertices = 0; /* No such primitive in d3d9types.h. */
            break;
    }

    return cVertices;
}

static HRESULT APIENTRY vboxWddmDDevDrawIndexedPrimitive2(HANDLE hDevice, CONST D3DDDIARG_DRAWINDEXEDPRIMITIVE2* pData,
                                                          UINT dwIndicesSize, CONST VOID* pIndexBuffer, CONST UINT* pFlagBuffer)
{
    RT_NOREF(pFlagBuffer);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = S_OK;
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    const uint8_t *pvVertexBuffer = NULL;
    DWORD cbVertexStride = 0;

    LOGF(("\n  PrimitiveType %d, BaseVertexOffset %d, MinIndex %d, NumVertices %d, StartIndexOffset %d, PrimitiveCount %d,\n"
          "  dwIndicesSize %d, pIndexBuffer %p, pFlagBuffer %p\n",
          pData->PrimitiveType,
          pData->BaseVertexOffset,
          pData->MinIndex,
          pData->NumVertices,
          pData->StartIndexOffset,
          pData->PrimitiveCount,
          dwIndicesSize,
          pIndexBuffer,
          pFlagBuffer));

    if (dwIndicesSize != 2 && dwIndicesSize != 4)
    {
        WARN(("unsupported dwIndicesSize %d", dwIndicesSize));
        return E_INVALIDARG;
    }

    if (pData->PrimitiveCount == 0)
    {
        /* Nothing to draw. */
        return S_OK;
    }

    /* Fetch the appropriate stream source:
     * "Stream zero contains transform indices and is the only stream that should be accessed."
     */
    if (pDevice->aStreamSourceUm[0].pvBuffer)
    {
        Assert(pDevice->aStreamSourceUm[0].cbStride);

        pvVertexBuffer = (const uint8_t *)pDevice->aStreamSourceUm[0].pvBuffer;
        cbVertexStride = pDevice->aStreamSourceUm[0].cbStride;
        LOGF(("aStreamSourceUm %p, stride %d\n",
              pvVertexBuffer, cbVertexStride));
    }
    else if (pDevice->aStreamSource[0])
    {
        PVBOXWDDMDISP_ALLOCATION pAlloc = pDevice->aStreamSource[0];
        if (pAlloc->pvMem)
        {
            Assert(pDevice->StreamSourceInfo[0].uiStride);
            pvVertexBuffer = ((const uint8_t *)pAlloc->pvMem) + pDevice->StreamSourceInfo[0].uiOffset;
            cbVertexStride = pDevice->StreamSourceInfo[0].uiStride;
            LOGF(("aStreamSource %p, cbSize %d, stride %d, uiOffset %d (elements %d)\n",
                  pvVertexBuffer, pAlloc->SurfDesc.cbSize, cbVertexStride, pDevice->StreamSourceInfo[0].uiOffset,
                  cbVertexStride? pAlloc->SurfDesc.cbSize / cbVertexStride: 0));
        }
        else
        {
            WARN(("unsupported!!"));
            hr = E_FAIL;
        }
    }
    else
    {
        WARN(("not expected!"));
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        /* Convert input data to appropriate DrawIndexedPrimitiveUP parameters.
         * In particular prepare zero based vertex array becuase wine does not
         * handle MinVertexIndex correctly.
         */

        /* Take the offset, which corresponds to the index == 0, into account. */
        const uint8_t *pu8VertexStart = pvVertexBuffer + pData->BaseVertexOffset;

        /* Where the pData->MinIndex starts. */
        pu8VertexStart += pData->MinIndex * cbVertexStride;

        /* Convert indexes to zero based relative to pData->MinIndex. */
        const uint8_t *pu8IndicesStartSrc = (uint8_t *)pIndexBuffer + pData->StartIndexOffset;
        UINT cIndices = vboxWddmVertexCountFromPrimitive(pData->PrimitiveType, pData->PrimitiveCount);

        /* Allocate memory for converted indices. */
        uint8_t *pu8IndicesStartConv = (uint8_t *)RTMemAlloc(cIndices * dwIndicesSize);
        if (pu8IndicesStartConv != NULL)
        {
            UINT i;
            if (dwIndicesSize == 2)
            {
                uint16_t *pu16Src = (uint16_t *)pu8IndicesStartSrc;
                uint16_t *pu16Dst = (uint16_t *)pu8IndicesStartConv;
                for (i = 0; i < cIndices; ++i, ++pu16Dst, ++pu16Src)
                {
                    *pu16Dst = *pu16Src - pData->MinIndex;
                }
            }
            else
            {
                uint32_t *pu32Src = (uint32_t *)pu8IndicesStartSrc;
                uint32_t *pu32Dst = (uint32_t *)pu8IndicesStartConv;
                for (i = 0; i < cIndices; ++i, ++pu32Dst, ++pu32Src)
                {
                    *pu32Dst = *pu32Src - pData->MinIndex;
                }
            }

            hr = pDevice9If->DrawIndexedPrimitiveUP(pData->PrimitiveType,
                                                    0,
                                                    pData->NumVertices,
                                                    pData->PrimitiveCount,
                                                    pu8IndicesStartConv,
                                                    dwIndicesSize == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
                                                    pu8VertexStart,
                                                    cbVertexStride);

            if (SUCCEEDED(hr))
                hr = S_OK;
            else
                WARN(("DrawIndexedPrimitiveUP failed hr = 0x%x", hr));

            RTMemFree(pu8IndicesStartConv);

            /* Following any IDirect3DDevice9::DrawIndexedPrimitiveUP call, the stream 0 settings,
             * referenced by IDirect3DDevice9::GetStreamSource, are set to NULL. Also, the index
             * buffer setting for IDirect3DDevice9::SetIndices is set to NULL.
             */
            if (pDevice->aStreamSource[0])
            {
                HRESULT tmpHr = pDevice9If->SetStreamSource(0, (IDirect3DVertexBuffer9*)pDevice->aStreamSource[0]->pD3DIf,
                                                            pDevice->StreamSourceInfo[0].uiOffset,
                                                            pDevice->StreamSourceInfo[0].uiStride);
                if(!SUCCEEDED(tmpHr))
                    WARN(("SetStreamSource failed hr = 0x%x", tmpHr));
            }

            if (pDevice->IndiciesInfo.pIndicesAlloc)
            {
                HRESULT tmpHr = pDevice9If->SetIndices((IDirect3DIndexBuffer9*)pDevice->IndiciesInfo.pIndicesAlloc->pD3DIf);
                if(!SUCCEEDED(tmpHr))
                    WARN(("SetIndices failed hr = 0x%x", tmpHr));
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    vboxWddmDalCheckAddOnDraw(pDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}

AssertCompile(sizeof (D3DDDIBOX) == sizeof (VBOXBOX3D));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Left) == RT_SIZEOFMEMB(VBOXBOX3D, Left));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Top) == RT_SIZEOFMEMB(VBOXBOX3D, Top));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Right) == RT_SIZEOFMEMB(VBOXBOX3D, Right));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Bottom) == RT_SIZEOFMEMB(VBOXBOX3D, Bottom));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Front) == RT_SIZEOFMEMB(VBOXBOX3D, Front));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Back) == RT_SIZEOFMEMB(VBOXBOX3D, Back));

AssertCompile(RT_OFFSETOF(D3DDDIBOX, Left) == RT_OFFSETOF(VBOXBOX3D, Left));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Top) == RT_OFFSETOF(VBOXBOX3D, Top));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Right) == RT_OFFSETOF(VBOXBOX3D, Right));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Bottom) == RT_OFFSETOF(VBOXBOX3D, Bottom));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Front) == RT_OFFSETOF(VBOXBOX3D, Front));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Back) == RT_OFFSETOF(VBOXBOX3D, Back));

static HRESULT APIENTRY vboxWddmDDevVolBlt(HANDLE hDevice, CONST D3DDDIARG_VOLUMEBLT* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pDstRc = (PVBOXWDDMDISP_RESOURCE)pData->hDstResource;
    PVBOXWDDMDISP_RESOURCE pSrcRc = (PVBOXWDDMDISP_RESOURCE)pData->hSrcResource;
    /* requirements for D3DDevice9::UpdateTexture */
    Assert(pDstRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE);
    Assert(pSrcRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE);
    IDirect3DVolumeTexture9 * pSrcTex = (IDirect3DVolumeTexture9*)pSrcRc->aAllocations[0].pD3DIf;
    IDirect3DVolumeTexture9 * pDstTex = (IDirect3DVolumeTexture9*)pDstRc->aAllocations[0].pD3DIf;
    VBOXPOINT3D Point;
    Point.x = pData->DstX;
    Point.y = pData->DstY;
    Point.z = pData->DstZ;

    HRESULT hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9VolTexBlt((IDirect3DDevice9Ex*)pDevice9If, pSrcTex, pDstTex,
                                                                          (VBOXBOX3D*)&pData->SrcBox, &Point);
    if (FAILED(hr))
        WARN(("pfnVBoxWineExD3DDev9VolTexBlt failed hr 0x%x", hr));
    else
        hr = S_OK;
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;;
}

static HRESULT APIENTRY vboxWddmDDevBufBlt(HANDLE hDevice, CONST D3DDDIARG_BUFFERBLT* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
/// @todo vboxWddmDalCheckAdd(pDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDDevTexBlt(HANDLE hDevice, CONST D3DDDIARG_TEXBLT* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pDstRc = (PVBOXWDDMDISP_RESOURCE)pData->hDstResource;
    PVBOXWDDMDISP_RESOURCE pSrcRc = (PVBOXWDDMDISP_RESOURCE)pData->hSrcResource;
    /* requirements for D3DDevice9::UpdateTexture */
    Assert(pDstRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_TEXTURE
            || pDstRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_CUBE_TEXTURE
            || pDstRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE);
    Assert(pSrcRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_TEXTURE
            || pSrcRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_CUBE_TEXTURE
            || pDstRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE);
    Assert(pSrcRc->aAllocations[0].enmD3DIfType == pDstRc->aAllocations[0].enmD3DIfType);
    Assert(pSrcRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM);
    Assert(pDstRc->RcDesc.enmPool != D3DDDIPOOL_SYSTEMMEM);
    HRESULT hr = S_OK;
    VBOXVDBG_CHECK_SMSYNC(pDstRc);
    VBOXVDBG_CHECK_SMSYNC(pSrcRc);

    if (   pSrcRc->aAllocations[0].SurfDesc.d3dWidth == pDstRc->aAllocations[0].SurfDesc.d3dWidth
        && pSrcRc->aAllocations[0].SurfDesc.height   == pDstRc->aAllocations[0].SurfDesc.height
        && pSrcRc->RcDesc.enmFormat == pDstRc->RcDesc.enmFormat
        && pData->DstPoint.x   == 0
        && pData->DstPoint.y   == 0
        && pData->SrcRect.left == 0
        && pData->SrcRect.top  == 0
        && pData->SrcRect.right - pData->SrcRect.left == (LONG)pSrcRc->aAllocations[0].SurfDesc.width
        && pData->SrcRect.bottom - pData->SrcRect.top == (LONG)pSrcRc->aAllocations[0].SurfDesc.height)
    {
        IDirect3DBaseTexture9 *pD3DIfSrcTex = (IDirect3DBaseTexture9*)pSrcRc->aAllocations[0].pD3DIf;
        IDirect3DBaseTexture9 *pD3DIfDstTex = (IDirect3DBaseTexture9*)pDstRc->aAllocations[0].pD3DIf;
        Assert(pD3DIfSrcTex);
        Assert(pD3DIfDstTex);
        VBOXVDBG_CHECK_TEXBLT(
                hr = pDevice9If->UpdateTexture(pD3DIfSrcTex, pD3DIfDstTex); Assert(hr == S_OK),
                pSrcRc,
                &pData->SrcRect,
                pDstRc,
                &pData->DstPoint);
    }
    else
    {
        Assert(pDstRc->aAllocations[0].enmD3DIfType != VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE);
        Assert(pSrcRc->aAllocations[0].enmD3DIfType != VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE);

        IDirect3DSurface9 *pSrcSurfIf = NULL;
        IDirect3DSurface9 *pDstSurfIf = NULL;
        hr = VBoxD3DIfSurfGet(pDstRc, 0, &pDstSurfIf);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            hr = VBoxD3DIfSurfGet(pSrcRc, 0, &pSrcSurfIf);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                RECT DstRect;
                vboxWddmRectMoved(&DstRect, &pData->SrcRect, pData->DstPoint.x, pData->DstPoint.y);
#ifdef DEBUG
                RECT tstRect = {0,0, pDstRc->aAllocations[0].SurfDesc.width, pDstRc->aAllocations[0].SurfDesc.height};
                Assert(vboxWddmRectIsCoveres(&tstRect, &DstRect));
#endif
                VBOXVDBG_CHECK_TEXBLT(hr = pDevice9If->StretchRect(pSrcSurfIf, &pData->SrcRect, pDstSurfIf, &DstRect, D3DTEXF_NONE);
                                      Assert(hr == S_OK),
                                      pSrcRc, &pData->SrcRect,
                                      pDstRc, &pData->DstPoint);
                pSrcSurfIf->Release();
            }
            pDstSurfIf->Release();
        }
    }

    vboxWddmDalCheckAddRc(pDevice, pDstRc, TRUE);
    vboxWddmDalCheckAddRc(pDevice, pSrcRc, FALSE);

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevStateSet(HANDLE hDevice, D3DDDIARG_STATESET* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetPriority(HANDLE hDevice, CONST D3DDDIARG_SETPRIORITY* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
//    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
//    Assert(pDevice);
//    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return S_OK;
}
AssertCompile(sizeof (RECT) == sizeof (D3DRECT));
AssertCompile(RT_SIZEOFMEMB(RECT, left) == RT_SIZEOFMEMB(D3DRECT, x1));
AssertCompile(RT_SIZEOFMEMB(RECT, right) == RT_SIZEOFMEMB(D3DRECT, x2));
AssertCompile(RT_SIZEOFMEMB(RECT, top) == RT_SIZEOFMEMB(D3DRECT, y1));
AssertCompile(RT_SIZEOFMEMB(RECT, bottom) == RT_SIZEOFMEMB(D3DRECT, y2));
AssertCompile(RT_OFFSETOF(RECT, left) == RT_OFFSETOF(D3DRECT, x1));
AssertCompile(RT_OFFSETOF(RECT, right) == RT_OFFSETOF(D3DRECT, x2));
AssertCompile(RT_OFFSETOF(RECT, top) == RT_OFFSETOF(D3DRECT, y1));
AssertCompile(RT_OFFSETOF(RECT, bottom) == RT_OFFSETOF(D3DRECT, y2));

static HRESULT APIENTRY vboxWddmDDevClear(HANDLE hDevice, CONST D3DDDIARG_CLEAR* pData, UINT NumRect, CONST RECT* pRect)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->Clear(NumRect, (D3DRECT*)pRect /* see AssertCompile above */,
            pData->Flags,
            pData->FillColor,
            pData->FillDepth,
            pData->FillStencil);
    if (SUCCEEDED(hr))
    {
        if (pData->Flags & D3DCLEAR_TARGET)
            vboxWddmDalCheckAddRTs(pDevice);
        if ((pData->Flags & D3DCLEAR_STENCIL)
                || (pData->Flags & D3DCLEAR_ZBUFFER))
            vboxWddmDalCheckAddDepthStencil(pDevice);
    }
    else
        WARN(("Clear failed %#x", hr));

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevUpdatePalette(HANDLE hDevice, CONST D3DDDIARG_UPDATEPALETTE* pData,
                                                  CONST PALETTEENTRY* pPaletteData)
{
    RT_NOREF(pData, pPaletteData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDDevSetPalette(HANDLE hDevice, CONST D3DDDIARG_SETPALETTE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDDevSetVertexShaderConst(HANDLE hDevice, CONST D3DDDIARG_SETVERTEXSHADERCONST* pData ,
                                                         CONST VOID* pRegisters)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetVertexShaderConstantF(
            pData->Register,
            (CONST float*)pRegisters,
            pData->Count);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevMultiplyTransform(HANDLE hDevice, CONST D3DDDIARG_MULTIPLYTRANSFORM* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetTransform(HANDLE hDevice, CONST D3DDDIARG_SETTRANSFORM* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetViewport(HANDLE hDevice, CONST D3DDDIARG_VIEWPORTINFO* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    pDevice->ViewPort.X = pData->X;
    pDevice->ViewPort.Y = pData->Y;
    pDevice->ViewPort.Width = pData->Width;
    pDevice->ViewPort.Height = pData->Height;
    HRESULT hr = pDevice9If->SetViewport(&pDevice->ViewPort);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetZRange(HANDLE hDevice, CONST D3DDDIARG_ZRANGE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    pDevice->ViewPort.MinZ = pData->MinZ;
    pDevice->ViewPort.MaxZ = pData->MaxZ;
    HRESULT hr = pDevice9If->SetViewport(&pDevice->ViewPort);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetMaterial(HANDLE hDevice, CONST D3DDDIARG_SETMATERIAL* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetLight(HANDLE hDevice, CONST D3DDDIARG_SETLIGHT* pData, CONST D3DDDI_LIGHT* pLightProperties)
{
    RT_NOREF(pData, pLightProperties);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevCreateLight(HANDLE hDevice, CONST D3DDDIARG_CREATELIGHT* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDestroyLight(HANDLE hDevice, CONST D3DDDIARG_DESTROYLIGHT* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetClipPlane(HANDLE hDevice, CONST D3DDDIARG_SETCLIPPLANE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetClipPlane(pData->Index, pData->Plane);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevGetInfo(HANDLE hDevice, UINT DevInfoID, VOID* pDevInfoStruct, UINT DevInfoSize)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
//    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
//    Assert(pDevice);
//    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    HRESULT hr = S_OK;
    switch (DevInfoID)
    {
        case D3DDDIDEVINFOID_VCACHE:
        {
            Assert(DevInfoSize == sizeof (D3DDDIDEVINFO_VCACHE));
            if (DevInfoSize == sizeof (D3DDDIDEVINFO_VCACHE))
            {
                D3DDDIDEVINFO_VCACHE *pVCache = (D3DDDIDEVINFO_VCACHE*)pDevInfoStruct;
                pVCache->Pattern = MAKEFOURCC('C', 'A', 'C', 'H');
                pVCache->OptMethod = 0 /* D3DXMESHOPT_STRIPREORDER */;
                pVCache->CacheSize = 0;
                pVCache->MagicNumber = 0;
            }
            else
                hr = E_INVALIDARG;
            break;
        }
        default:
            Assert(0);
            hr = E_NOTIMPL;
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}

AssertCompile(sizeof (D3DDDIBOX) == sizeof (D3DBOX));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Left) == RT_SIZEOFMEMB(D3DBOX, Left));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Top) == RT_SIZEOFMEMB(D3DBOX, Top));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Right) == RT_SIZEOFMEMB(D3DBOX, Right));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Bottom) == RT_SIZEOFMEMB(D3DBOX, Bottom));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Front) == RT_SIZEOFMEMB(D3DBOX, Front));
AssertCompile(RT_SIZEOFMEMB(D3DDDIBOX, Back) == RT_SIZEOFMEMB(D3DBOX, Back));

AssertCompile(RT_OFFSETOF(D3DDDIBOX, Left) == RT_OFFSETOF(D3DBOX, Left));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Top) == RT_OFFSETOF(D3DBOX, Top));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Right) == RT_OFFSETOF(D3DBOX, Right));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Bottom) == RT_OFFSETOF(D3DBOX, Bottom));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Front) == RT_OFFSETOF(D3DBOX, Front));
AssertCompile(RT_OFFSETOF(D3DDDIBOX, Back) == RT_OFFSETOF(D3DBOX, Back));

static HRESULT APIENTRY vboxWddmDDevLock(HANDLE hDevice, D3DDDIARG_LOCK* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hResource;
    if (pData->SubResourceIndex >= pRc->cAllocations)
        return E_INVALIDARG;
    PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[pData->SubResourceIndex];
    Assert(pData->SubResourceIndex < pRc->cAllocations);

    HRESULT hr = S_OK;

    if (VBOXDISPMODE_IS_3D(pDevice->pAdapter))
    {
//        Assert(pRc != pScreen->pRenderTargetRc || pScreen->iRenderTargetFrontBuf != pData->SubResourceIndex);
#ifdef VBOXWDDMDISP_DAL_CHECK_LOCK
        hr = vboxWddmDalCheckLock(pDevice, pAlloc, pData->Flags);
        if (!SUCCEEDED(hr))
        {
            WARN(("vboxWddmDalCheckLock failed %#x", hr));
            return hr;
        }
#endif

        if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_TEXTURE
            || pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_CUBE_TEXTURE
            || pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE)
        {
            PVBOXWDDMDISP_ALLOCATION pTexAlloc = &pRc->aAllocations[0];
            Assert(pData->SubResourceIndex < pRc->cAllocations);
            IDirect3DTexture9 *pD3DIfTex = (IDirect3DTexture9*)pTexAlloc->pD3DIf;
            IDirect3DCubeTexture9 *pD3DIfCubeTex = (IDirect3DCubeTexture9*)pTexAlloc->pD3DIf;
            IDirect3DSurface9 *pD3DIfSurface = (IDirect3DSurface9*)pTexAlloc->pD3DIf;
            Assert(pTexAlloc->pD3DIf);
            RECT *pRect = NULL;
            BOOL fNeedLock = TRUE;
            Assert(!pData->Flags.RangeValid);
            Assert(!pData->Flags.BoxValid);
            if (pData->Flags.AreaValid)
            {
                pRect = &pData->Area;
            }

            /* else - we lock the entire texture, pRect == NULL */

            if (pAlloc->LockInfo.cLocks)
            {
                Assert(pAlloc->LockInfo.fFlags.AreaValid == pData->Flags.AreaValid);
                if (pAlloc->LockInfo.fFlags.AreaValid && pData->Flags.AreaValid)
                {
                    Assert(pAlloc->LockInfo.Area.left == pData->Area.left);
                    Assert(pAlloc->LockInfo.Area.top == pData->Area.top);
                    Assert(pAlloc->LockInfo.Area.right == pData->Area.right);
                    Assert(pAlloc->LockInfo.Area.bottom == pData->Area.bottom);
                }
                Assert(pAlloc->LockInfo.LockedRect.pBits);
                Assert((pAlloc->LockInfo.fFlags.Value & ~1) == (pData->Flags.Value & ~1)); /* <- 1 is "ReadOnly" flag */

                if (pAlloc->LockInfo.fFlags.ReadOnly && !pData->Flags.ReadOnly)
                {
                    switch (pTexAlloc->enmD3DIfType)
                    {
                        case VBOXDISP_D3DIFTYPE_TEXTURE:
                            hr = pD3DIfTex->UnlockRect(pData->SubResourceIndex);
                            break;
                        case VBOXDISP_D3DIFTYPE_CUBE_TEXTURE:
                            hr = pD3DIfCubeTex->UnlockRect(VBOXDISP_CUBEMAP_INDEX_TO_FACE(pRc, pData->SubResourceIndex),
                                    VBOXDISP_CUBEMAP_INDEX_TO_LEVEL(pRc, pData->SubResourceIndex));
                            break;
                        case VBOXDISP_D3DIFTYPE_SURFACE:
                            hr = pD3DIfSurface->UnlockRect();
                            break;
                        default:
                            Assert(0);
                            break;
                    }
                    Assert(hr == S_OK);
                }
                else
                {
                    fNeedLock = FALSE;
                }
            }

            if (fNeedLock && SUCCEEDED(hr))
            {
                VBOXVDBG_CHECK_SMSYNC(pRc);

                pAlloc->LockInfo.fFlags = pData->Flags;
                if (pRect)
                {
                    pAlloc->LockInfo.Area = *pRect;
                    Assert(pAlloc->LockInfo.fFlags.AreaValid == 1);
                }
                else
                {
                    Assert(pAlloc->LockInfo.fFlags.AreaValid == 0);
                }

                switch (pTexAlloc->enmD3DIfType)
                {
                    case VBOXDISP_D3DIFTYPE_TEXTURE:
                        hr = pD3DIfTex->LockRect(pData->SubResourceIndex,
                                &pAlloc->LockInfo.LockedRect,
                                pRect,
                                vboxDDI2D3DLockFlags(pData->Flags));
                        break;
                    case VBOXDISP_D3DIFTYPE_CUBE_TEXTURE:
                        hr = pD3DIfCubeTex->LockRect(VBOXDISP_CUBEMAP_INDEX_TO_FACE(pRc, pData->SubResourceIndex),
                                VBOXDISP_CUBEMAP_INDEX_TO_LEVEL(pRc, pData->SubResourceIndex),
                                &pAlloc->LockInfo.LockedRect,
                                pRect,
                                vboxDDI2D3DLockFlags(pData->Flags));
                        break;
                    case VBOXDISP_D3DIFTYPE_SURFACE:
                        hr = pD3DIfSurface->LockRect(&pAlloc->LockInfo.LockedRect,
                                pRect,
                                vboxDDI2D3DLockFlags(pData->Flags));
                        break;
                    default:
                        Assert(0);
                        break;
                }

                if (FAILED(hr))
                {
                    WARN(("LockRect failed, hr", hr));
                }
            }

            if (SUCCEEDED(hr))
            {
                ++pAlloc->LockInfo.cLocks;

                if (!pData->Flags.NotifyOnly)
                {
                    pData->pSurfData = pAlloc->LockInfo.LockedRect.pBits;
                    pData->Pitch = pAlloc->LockInfo.LockedRect.Pitch;
                    pData->SlicePitch = 0;
                    Assert(pAlloc->SurfDesc.slicePitch == 0);
                    Assert(!pAlloc->pvMem);
                }
                else
                {
                    Assert(pAlloc->pvMem);
                    Assert(pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM);
                }

                VBOXVDBG_DUMP_LOCK_ST(pData);

                hr = S_OK;
            }
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE)
        {
            PVBOXWDDMDISP_ALLOCATION pTexAlloc = &pRc->aAllocations[0];
            Assert(pData->SubResourceIndex < pRc->cAllocations);
            IDirect3DVolumeTexture9 *pD3DIfTex = (IDirect3DVolumeTexture9*)pTexAlloc->pD3DIf;
            Assert(pTexAlloc->pD3DIf);
            D3DDDIBOX *pBox = NULL;
            BOOL fNeedLock = TRUE;
            Assert(!pData->Flags.AreaValid);
            Assert(!pData->Flags.BoxValid);
            if (pData->Flags.BoxValid)
            {
                pBox = &pData->Box;
            }

            /* else - we lock the entire texture, pBox == NULL */

            if (pAlloc->LockInfo.cLocks)
            {
                Assert(pAlloc->LockInfo.fFlags.BoxValid == pData->Flags.BoxValid);
                if (pAlloc->LockInfo.fFlags.BoxValid && pData->Flags.BoxValid)
                {
                    Assert(pAlloc->LockInfo.Box.Left == pData->Box.Left);
                    Assert(pAlloc->LockInfo.Box.Top == pData->Box.Top);
                    Assert(pAlloc->LockInfo.Box.Right == pData->Box.Right);
                    Assert(pAlloc->LockInfo.Box.Bottom == pData->Box.Bottom);
                    Assert(pAlloc->LockInfo.Box.Front == pData->Box.Front);
                    Assert(pAlloc->LockInfo.Box.Back == pData->Box.Back);
                }
                Assert(pAlloc->LockInfo.LockedBox.pBits);
                Assert((pAlloc->LockInfo.fFlags.Value & ~1) == (pData->Flags.Value & ~1)); /* <- 1 is "ReadOnly" flag */

                if (pAlloc->LockInfo.fFlags.ReadOnly && !pData->Flags.ReadOnly)
                {
                    hr = pD3DIfTex->UnlockBox(pData->SubResourceIndex);
                    Assert(hr == S_OK);
                }
                else
                {
                    fNeedLock = FALSE;
                }
            }

            if (fNeedLock && SUCCEEDED(hr))
            {
                VBOXVDBG_CHECK_SMSYNC(pRc);

                pAlloc->LockInfo.fFlags = pData->Flags;
                if (pBox)
                {
                    pAlloc->LockInfo.Box = *pBox;
                    Assert(pAlloc->LockInfo.fFlags.BoxValid == 1);
                }
                else
                {
                    Assert(pAlloc->LockInfo.fFlags.BoxValid == 0);
                }

                hr = pD3DIfTex->LockBox(pData->SubResourceIndex,
                                &pAlloc->LockInfo.LockedBox,
                                (D3DBOX*)pBox,
                                vboxDDI2D3DLockFlags(pData->Flags));
                if (FAILED(hr))
                {
                    WARN(("LockRect failed, hr", hr));
                }
            }

            if (SUCCEEDED(hr))
            {
                ++pAlloc->LockInfo.cLocks;

                if (!pData->Flags.NotifyOnly)
                {
                    pData->pSurfData = pAlloc->LockInfo.LockedBox.pBits;
                    pData->Pitch = pAlloc->LockInfo.LockedBox.RowPitch;
                    pData->SlicePitch = pAlloc->LockInfo.LockedBox.SlicePitch;
                    Assert(!pAlloc->pvMem);
                }
                else
                {
                    Assert(pAlloc->pvMem);
                    Assert(pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM);
                }

                VBOXVDBG_DUMP_LOCK_ST(pData);

                hr = S_OK;
            }
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VERTEXBUFFER)
        {
            Assert(pData->SubResourceIndex < pRc->cAllocations);
            IDirect3DVertexBuffer9 *pD3D9VBuf = (IDirect3DVertexBuffer9*)pAlloc->pD3DIf;
            BOOL bLocked = false;
            Assert(pD3D9VBuf);
            Assert(!pData->Flags.AreaValid);
            Assert(!pData->Flags.BoxValid);
            D3DDDIRANGE *pRange = NULL;
            if (pData->Flags.RangeValid)
            {
                pRange = &pData->Range;
            }

            /* else - we lock the entire vertex buffer, pRect == NULL */

            if (!pAlloc->LockInfo.cLocks)
            {
                VBOXVDBG_CHECK_SMSYNC(pRc);
                if (!pData->Flags.MightDrawFromLocked || (!pData->Flags.Discard && !pData->Flags.NoOverwrite))
                {
                    hr = pD3D9VBuf->Lock(pRange ? pRange->Offset : 0,
                                          pRange ? pRange->Size : 0,
                                          &pAlloc->LockInfo.LockedRect.pBits,
                                          vboxDDI2D3DLockFlags(pData->Flags));
                    bLocked = true;
                }

                Assert(hr == S_OK);
                if (hr == S_OK)
                {
                    Assert(pAlloc->SurfDesc.pitch == pAlloc->SurfDesc.width);
                    pAlloc->LockInfo.LockedRect.Pitch = pAlloc->SurfDesc.pitch;
//                    Assert(pAlloc->LockInfo.fFlags.Value == 0);
                    pAlloc->LockInfo.fFlags = pData->Flags;
                    if (pRange)
                    {
                        pAlloc->LockInfo.Range = *pRange;
                        Assert(pAlloc->LockInfo.fFlags.RangeValid == 1);
//                        pAlloc->LockInfo.fFlags.RangeValid = 1;
                    }
                    else
                    {
                        Assert(pAlloc->LockInfo.fFlags.RangeValid == 0);
//                        pAlloc->LockInfo.fFlags.RangeValid = 0;
                    }
                }
            }
            else
            {
                Assert(pAlloc->LockInfo.fFlags.RangeValid == pData->Flags.RangeValid);
                if (pAlloc->LockInfo.fFlags.RangeValid && pData->Flags.RangeValid)
                {
                    Assert(pAlloc->LockInfo.Range.Offset == pData->Range.Offset);
                    Assert(pAlloc->LockInfo.Range.Size == pData->Range.Size);
                }
                Assert(pAlloc->LockInfo.LockedRect.pBits);
            }

            if (hr == S_OK)
            {
                ++pAlloc->LockInfo.cLocks;

                if (!pData->Flags.NotifyOnly)
                {
                    pData->pSurfData = pAlloc->LockInfo.LockedRect.pBits;
                    pData->Pitch = pAlloc->LockInfo.LockedRect.Pitch;
                    pData->SlicePitch = 0;
                    Assert(pAlloc->SurfDesc.slicePitch == 0);
                    Assert(!pAlloc->pvMem);
                }
                else
                {
                    Assert(pAlloc->pvMem);
                    Assert(pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM);
                    if (bLocked && !pData->Flags.Discard)
                    {
                        RECT r, *pr;
                        if (pRange)
                        {
                            r.top = 0;
                            r.left = pRange->Offset;
                            r.bottom = 1;
                            r.right = pRange->Offset + pRange->Size;
                            pr = &r;
                        }
                        else
                            pr = NULL;
                        VBoxD3DIfLockUnlockMemSynch(pAlloc, &pAlloc->LockInfo.LockedRect, pr, false /*bool bToLockInfo*/);
                    }
                }
            }
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_INDEXBUFFER)
        {
            Assert(pData->SubResourceIndex < pRc->cAllocations);
            IDirect3DIndexBuffer9 *pD3D9IBuf = (IDirect3DIndexBuffer9*)pAlloc->pD3DIf;
            BOOL bLocked = false;
            Assert(pD3D9IBuf);
            Assert(!pData->Flags.AreaValid);
            Assert(!pData->Flags.BoxValid);
            D3DDDIRANGE *pRange = NULL;
            if (pData->Flags.RangeValid)
            {
                pRange = &pData->Range;
            }

            /* else - we lock the entire vertex buffer, pRect == NULL */

            if (!pAlloc->LockInfo.cLocks)
            {
                VBOXVDBG_CHECK_SMSYNC(pRc);
                if (!pData->Flags.MightDrawFromLocked || (!pData->Flags.Discard && !pData->Flags.NoOverwrite))
                {
                    hr = pD3D9IBuf->Lock(pRange ? pRange->Offset : 0,
                                          pRange ? pRange->Size : 0,
                                          &pAlloc->LockInfo.LockedRect.pBits,
                                          vboxDDI2D3DLockFlags(pData->Flags));
                    bLocked = true;
                }

                Assert(hr == S_OK);
                if (hr == S_OK)
                {
                    Assert(pAlloc->SurfDesc.pitch == pAlloc->SurfDesc.width);
                    pAlloc->LockInfo.LockedRect.Pitch = pAlloc->SurfDesc.pitch;
//                    Assert(pAlloc->LockInfo.fFlags.Value == 0);
                    pAlloc->LockInfo.fFlags = pData->Flags;
                    if (pRange)
                    {
                        pAlloc->LockInfo.Range = *pRange;
                        Assert(pAlloc->LockInfo.fFlags.RangeValid == 1);
//                        pAlloc->LockInfo.fFlags.RangeValid = 1;
                    }
                    else
                    {
                        Assert(pAlloc->LockInfo.fFlags.RangeValid == 0);
//                        pAlloc->LockInfo.fFlags.RangeValid = 0;
                    }
                }
            }
            else
            {
                Assert(pAlloc->LockInfo.fFlags.RangeValid == pData->Flags.RangeValid);
                if (pAlloc->LockInfo.fFlags.RangeValid && pData->Flags.RangeValid)
                {
                    Assert(pAlloc->LockInfo.Range.Offset == pData->Range.Offset);
                    Assert(pAlloc->LockInfo.Range.Size == pData->Range.Size);
                }
                Assert(pAlloc->LockInfo.LockedRect.pBits);
            }

            if (hr == S_OK)
            {
                ++pAlloc->LockInfo.cLocks;

                if (!pData->Flags.NotifyOnly)
                {
                    pData->pSurfData = pAlloc->LockInfo.LockedRect.pBits;
                    pData->Pitch = pAlloc->LockInfo.LockedRect.Pitch;
                    pData->SlicePitch = 0;
                    Assert(pAlloc->SurfDesc.slicePitch == 0);
                    Assert(!pAlloc->pvMem);
                }
                else
                {
                    Assert(pAlloc->pvMem);
                    Assert(pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM);
                    if (bLocked && !pData->Flags.Discard)
                    {
                        RECT r, *pr;
                        if (pRange)
                        {
                            r.top = 0;
                            r.left = pRange->Offset;
                            r.bottom = 1;
                            r.right = pRange->Offset + pRange->Size;
                            pr = &r;
                        }
                        else
                            pr = NULL;
                        VBoxD3DIfLockUnlockMemSynch(pAlloc, &pAlloc->LockInfo.LockedRect, pr, false /*bool bToLockInfo*/);
                    }
                }
            }
        }
        else
        {
            WARN(("not implemented %d", pRc->aAllocations[0].enmD3DIfType));
        }

#ifdef VBOXWDDMDISP_DAL_CHECK_LOCK
        if (!SUCCEEDED(hr))
        {
            WARN(("lock failed %#x", hr));
            vboxWddmDalCheckUnlock(pDevice, pAlloc);
        }
#endif
    }
    else /* if !VBOXDISPMODE_IS_3D(pDevice->pAdapter) */
    {
        if (pAlloc->hAllocation)
        {
            if (pRc->RcDesc.enmPool != D3DDDIPOOL_SYSTEMMEM)
            {
                D3DDDICB_LOCK LockData;
                LockData.hAllocation = pAlloc->hAllocation;
                LockData.PrivateDriverData = 0;
                LockData.NumPages = 0;
                LockData.pPages = NULL;
                LockData.pData = NULL; /* out */
                LockData.Flags.Value = 0;
                LockData.Flags.Discard = pData->Flags.Discard;
                LockData.Flags.DonotWait = pData->Flags.DoNotWait;

                uintptr_t offset;
                if (pData->Flags.AreaValid)
                {
                    offset = vboxWddmCalcOffXYrd(pData->Area.left, pData->Area.top, pAlloc->SurfDesc.pitch,
                                                 pAlloc->SurfDesc.format);
                }
                else if (pData->Flags.RangeValid)
                {
                    offset = pData->Range.Offset;
                }
                else if (pData->Flags.BoxValid)
                {
                    vboxVDbgPrintF((__FUNCTION__": Implement Box area"));
                    Assert(0);
                    offset = 0;
                }
                else
                {
                    offset = 0;
                }

                hr = pDevice->RtCallbacks.pfnLockCb(pDevice->hDevice, &LockData);
                Assert(hr == S_OK || (hr == D3DERR_WASSTILLDRAWING && pData->Flags.DoNotWait));
                if (hr == S_OK)
                {
                    pData->pSurfData = ((uint8_t*)LockData.pData) + offset;
                    pData->Pitch = pAlloc->SurfDesc.pitch;
                    pData->SlicePitch = pAlloc->SurfDesc.slicePitch;

                    if (pData->Flags.Discard)
                    {
                        /* check if the surface was renamed */
                        if (LockData.hAllocation)
                            pAlloc->hAllocation = LockData.hAllocation;
                    }
                }
            }
            /* else - d3d may create sysmem render targets and call our Present callbacks for those
             * to make it work properly we need to create a VRAM surface corresponding to sysmem one
             * and copy stuff to VRAM on lock/unlock
             *
             * so we don't do any locking here, but still track the lock info here
             * and do lock-memcopy-unlock to VRAM surface on sysmem surface unlock
             * */

            if (hr == S_OK)
            {
                Assert(!pAlloc->LockInfo.cLocks);

                if (!pData->Flags.ReadOnly)
                {
                    if (pData->Flags.AreaValid)
                        vboxWddmDirtyRegionAddRect(&pAlloc->DirtyRegion, &pData->Area);
                    else
                    {
                        Assert(!pData->Flags.RangeValid);
                        Assert(!pData->Flags.BoxValid);
                        vboxWddmDirtyRegionAddRect(&pAlloc->DirtyRegion, NULL); /* <- NULL means the entire surface */
                    }
                }

                ++pAlloc->LockInfo.cLocks;
            }
        }
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(%d)\n", hDevice, hr));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevUnlock(HANDLE hDevice, CONST D3DDDIARG_UNLOCK* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hResource;
    HRESULT hr = S_OK;

    Assert(pData->SubResourceIndex < pRc->cAllocations);
    if (pData->SubResourceIndex >= pRc->cAllocations)
        return E_INVALIDARG;

    PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[pData->SubResourceIndex];

    if (VBOXDISPMODE_IS_3D(pDevice->pAdapter))
    {
        if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_TEXTURE
            || pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_CUBE_TEXTURE
            || pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE)
        {
            Assert(pData->SubResourceIndex < pRc->cAllocations);

            VBOXVDBG_DUMP_UNLOCK_ST(pData);

            --pAlloc->LockInfo.cLocks;
            Assert(pAlloc->LockInfo.cLocks < UINT32_MAX);
            if (!pAlloc->LockInfo.cLocks)
            {
                PVBOXWDDMDISP_ALLOCATION pTexAlloc = &pRc->aAllocations[0];
                Assert(pTexAlloc->pD3DIf);
                switch (pRc->aAllocations[0].enmD3DIfType)
                {
                    case VBOXDISP_D3DIFTYPE_TEXTURE:
                    {
                        IDirect3DTexture9 *pD3DIfTex = (IDirect3DTexture9*)pTexAlloc->pD3DIf;
                        hr = pD3DIfTex->UnlockRect(pData->SubResourceIndex);
                        break;
                    }
                    case VBOXDISP_D3DIFTYPE_CUBE_TEXTURE:
                    {
                        IDirect3DCubeTexture9 *pD3DIfCubeTex = (IDirect3DCubeTexture9*)pTexAlloc->pD3DIf;
                        hr = pD3DIfCubeTex->UnlockRect(VBOXDISP_CUBEMAP_INDEX_TO_FACE(pRc, pData->SubResourceIndex),
                                VBOXDISP_CUBEMAP_INDEX_TO_LEVEL(pRc, pData->SubResourceIndex));
                        break;
                    }
                    case VBOXDISP_D3DIFTYPE_SURFACE:
                    {
                        IDirect3DSurface9 *pD3DIfSurf = (IDirect3DSurface9*)pTexAlloc->pD3DIf;
                        hr = pD3DIfSurf->UnlockRect();
                        break;
                    }
                    default:
                        Assert(0);
                        break;
                }
                if (FAILED(hr))
                    WARN(("UnlockRect failed, hr 0x%x", hr));
                VBOXVDBG_CHECK_SMSYNC(pRc);
            }
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE)
        {
            Assert(pData->SubResourceIndex < pRc->cAllocations);

            VBOXVDBG_DUMP_UNLOCK_ST(pData);

            --pAlloc->LockInfo.cLocks;
            Assert(pAlloc->LockInfo.cLocks < UINT32_MAX);
            if (!pAlloc->LockInfo.cLocks)
            {
                PVBOXWDDMDISP_ALLOCATION pTexAlloc = &pRc->aAllocations[0];
                Assert(pTexAlloc->pD3DIf);
                IDirect3DVolumeTexture9 *pD3DIfTex = (IDirect3DVolumeTexture9*)pTexAlloc->pD3DIf;
                hr = pD3DIfTex->UnlockBox(pData->SubResourceIndex);
                if (FAILED(hr))
                    WARN(("UnlockBox failed, hr 0x%x", hr));
                VBOXVDBG_CHECK_SMSYNC(pRc);
            }
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_VERTEXBUFFER)
        {
            Assert(pData->SubResourceIndex < pRc->cAllocations);

            --pAlloc->LockInfo.cLocks;
            Assert(pAlloc->LockInfo.cLocks < UINT32_MAX);
            if (!pAlloc->LockInfo.cLocks
                && (!pAlloc->LockInfo.fFlags.MightDrawFromLocked
                    || (!pAlloc->LockInfo.fFlags.Discard && !pAlloc->LockInfo.fFlags.NoOverwrite)))
            {
//                Assert(!pAlloc->LockInfo.cLocks);
                IDirect3DVertexBuffer9 *pD3D9VBuf = (IDirect3DVertexBuffer9*)pAlloc->pD3DIf;
                Assert(pD3D9VBuf);
                /* this is a sysmem texture, update  */
                if (pAlloc->pvMem && !pAlloc->LockInfo.fFlags.ReadOnly)
                {
                    RECT r, *pr;
                    if (pAlloc->LockInfo.fFlags.RangeValid)
                    {
                        r.top = 0;
                        r.left = pAlloc->LockInfo.Range.Offset;
                        r.bottom = 1;
                        r.right = pAlloc->LockInfo.Range.Offset + pAlloc->LockInfo.Range.Size;
                        pr = &r;
                    }
                    else
                        pr = NULL;
                    VBoxD3DIfLockUnlockMemSynch(pAlloc, &pAlloc->LockInfo.LockedRect,
                            pr,
                            true /*bool bToLockInfo*/);
                }
                hr = pD3D9VBuf->Unlock();
                Assert(hr == S_OK);
                VBOXVDBG_CHECK_SMSYNC(pRc);
            }
            else
            {
                Assert(pAlloc->LockInfo.cLocks < UINT32_MAX);
            }
        }
        else if (pRc->aAllocations[0].enmD3DIfType == VBOXDISP_D3DIFTYPE_INDEXBUFFER)
        {
            Assert(pData->SubResourceIndex < pRc->cAllocations);

            --pAlloc->LockInfo.cLocks;
            Assert(pAlloc->LockInfo.cLocks < UINT32_MAX);
            if (!pAlloc->LockInfo.cLocks
                && (!pAlloc->LockInfo.fFlags.MightDrawFromLocked
                    || (!pAlloc->LockInfo.fFlags.Discard && !pAlloc->LockInfo.fFlags.NoOverwrite)))
            {
//                Assert(!pAlloc->LockInfo.cLocks);
                IDirect3DIndexBuffer9 *pD3D9IBuf = (IDirect3DIndexBuffer9*)pAlloc->pD3DIf;
                Assert(pD3D9IBuf);
                /* this is a sysmem texture, update  */
                if (pAlloc->pvMem && !pAlloc->LockInfo.fFlags.ReadOnly)
                {
                    RECT r, *pr;
                    if (pAlloc->LockInfo.fFlags.RangeValid)
                    {
                        r.top = 0;
                        r.left = pAlloc->LockInfo.Range.Offset;
                        r.bottom = 1;
                        r.right = pAlloc->LockInfo.Range.Offset + pAlloc->LockInfo.Range.Size;
                        pr = &r;
                    }
                    else
                        pr = NULL;
                    VBoxD3DIfLockUnlockMemSynch(pAlloc, &pAlloc->LockInfo.LockedRect,
                            pr,
                            true /*bool bToLockInfo*/);
                }
                hr = pD3D9IBuf->Unlock();
                Assert(hr == S_OK);
                VBOXVDBG_CHECK_SMSYNC(pRc);
            }
            else
            {
                Assert(pAlloc->LockInfo.cLocks < UINT32_MAX);
            }
        }
        else
        {
            WARN(("Unlock unsupported %d", pRc->aAllocations[0].enmD3DIfType));
        }

#ifdef VBOXWDDMDISP_DAL_CHECK_LOCK
        if (SUCCEEDED(hr))
        {
            hr = vboxWddmDalCheckUnlock(pDevice, pAlloc);
            if (!SUCCEEDED(hr))
                WARN(("vboxWddmDalCheckUnlock failed %#x", hr));
        }
        else
            WARN(("unlock failed %#x", hr));
#endif
    }
    else
    {
        if (pAlloc->hAllocation)
        {
            BOOL fDoUnlock = FALSE;

            Assert(pAlloc->LockInfo.cLocks);
            --pAlloc->LockInfo.cLocks;
            Assert(pAlloc->LockInfo.cLocks < UINT32_MAX);

            if (pRc->RcDesc.enmPool != D3DDDIPOOL_SYSTEMMEM)
            {
                fDoUnlock = TRUE;
            }
            else
            {
                if (!pAlloc->LockInfo.cLocks)
                {
                    D3DDDICB_LOCK LockData;
                    LockData.hAllocation = pAlloc->hAllocation;
                    LockData.PrivateDriverData = 0;
                    LockData.NumPages = 0;
                    LockData.pPages = NULL;
                    LockData.pData = NULL; /* out */
                    LockData.Flags.Value = 0;

                    hr = pDevice->RtCallbacks.pfnLockCb(pDevice->hDevice, &LockData);
                    if (hr == S_OK)
                    {
                        D3DLOCKED_RECT LRect;
                        LRect.pBits = LockData.pData;
                        LRect.Pitch = pAlloc->SurfDesc.pitch;
                        Assert(pAlloc->DirtyRegion.fFlags & VBOXWDDM_DIRTYREGION_F_VALID);
                        VBoxD3DIfLockUnlockMemSynch(pAlloc, &LRect, &pAlloc->DirtyRegion.Rect, TRUE /* bool bToLockInfo*/);
                        vboxWddmDirtyRegionClear(&pAlloc->DirtyRegion);
                        fDoUnlock = TRUE;
                    }
                    else
                    {
                        WARN(("pfnLockCb failed, hr 0x%x", hr));
                    }
                }
            }

            if (fDoUnlock)
            {
                D3DDDICB_UNLOCK Unlock;

                Unlock.NumAllocations = 1;
                Unlock.phAllocations = &pAlloc->hAllocation;

                hr = pDevice->RtCallbacks.pfnUnlockCb(pDevice->hDevice, &Unlock);
                if(hr != S_OK)
                {
                    WARN(("pfnUnlockCb failed, hr 0x%x", hr));
                }
            }

            if (!SUCCEEDED(hr))
            {
                WARN(("unlock failure!"));
                ++pAlloc->LockInfo.cLocks;
            }
        }
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevLockAsync(HANDLE hDevice, D3DDDIARG_LOCKASYNC* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevUnlockAsync(HANDLE hDevice, CONST D3DDDIARG_UNLOCKASYNC* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevRename(HANDLE hDevice, CONST D3DDDIARG_RENAME* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static void vboxWddmRequestAllocFree(D3DDDICB_ALLOCATE* pAlloc)
{
    RTMemFree(pAlloc);
}

static D3DDDICB_ALLOCATE* vboxWddmRequestAllocAlloc(D3DDDIARG_CREATERESOURCE* pResource)
{
    /* allocate buffer for D3DDDICB_ALLOCATE + D3DDDI_ALLOCATIONINFO * numAllocs + PVBOXWDDM_RCINFO with aAllocInfos[numAllocs] */
    uint32_t cbBuf = sizeof (D3DDDICB_ALLOCATE);
    uint32_t offDdiAllocInfos = (cbBuf + 7) & ~3;
    uint32_t cbDdiAllocInfos = sizeof (D3DDDI_ALLOCATIONINFO) * pResource->SurfCount;
    cbBuf = offDdiAllocInfos + cbDdiAllocInfos;
    uint32_t offRcInfo = (cbBuf + 7) & ~3;
    uint32_t cbRcInfo = sizeof (VBOXWDDM_RCINFO);
    cbBuf = offRcInfo + cbRcInfo;
    uint32_t offAllocInfos = (cbBuf + 7) & ~3;
    uint32_t cbAllocInfos = sizeof (VBOXWDDM_ALLOCINFO) * pResource->SurfCount;
    cbBuf = offAllocInfos + cbAllocInfos;
    uint8_t *pvBuf = (uint8_t*)RTMemAllocZ(cbBuf);
    Assert(pvBuf);
    if (pvBuf)
    {
        D3DDDICB_ALLOCATE* pAlloc = (D3DDDICB_ALLOCATE*)pvBuf;
        pAlloc->NumAllocations = pResource->SurfCount;
        pAlloc->pAllocationInfo = (D3DDDI_ALLOCATIONINFO*)(pvBuf + offDdiAllocInfos);
        PVBOXWDDM_RCINFO pRcInfo = (PVBOXWDDM_RCINFO)(pvBuf + offRcInfo);
        pAlloc->PrivateDriverDataSize = cbRcInfo;
        pAlloc->pPrivateDriverData = pRcInfo;
        pAlloc->hResource = pResource->hResource;
        PVBOXWDDM_ALLOCINFO pAllocInfos = (PVBOXWDDM_ALLOCINFO)(pvBuf + offAllocInfos);
        for (UINT i = 0; i < pResource->SurfCount; ++i)
        {
            D3DDDI_ALLOCATIONINFO* pDdiAllocInfo = &pAlloc->pAllocationInfo[i];
            PVBOXWDDM_ALLOCINFO pAllocInfo = &pAllocInfos[i];
            pDdiAllocInfo->pPrivateDriverData = pAllocInfo;
            pDdiAllocInfo->PrivateDriverDataSize = sizeof (VBOXWDDM_ALLOCINFO);
        }
        return pAlloc;
    }
    return NULL;
}

static HRESULT APIENTRY vboxWddmDDevCreateResource(HANDLE hDevice, D3DDDIARG_CREATERESOURCE* pResource)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(pResource);
    PVBOXWDDMDISP_ADAPTER pAdapter = pDevice->pAdapter;

    PVBOXWDDMDISP_RESOURCE pRc = vboxResourceAlloc(pResource->SurfCount);
    if (!pRc)
    {
        WARN(("vboxResourceAlloc failed"));
        return E_OUTOFMEMORY;
    }
    bool bIssueCreateResource = false;
    bool bCreateKMResource = false;
    bool bSetHostID = false;

    pRc->hResource = pResource->hResource;
    pRc->hKMResource = NULL;
    pRc->pDevice = pDevice;
    pRc->fFlags.Value = 0;
    pRc->fFlags.Generic = 1;
    pRc->RcDesc.fFlags = pResource->Flags;
    pRc->RcDesc.enmFormat = pResource->Format;
    pRc->RcDesc.enmPool = pResource->Pool;
    pRc->RcDesc.enmMultisampleType = pResource->MultisampleType;
    pRc->RcDesc.MultisampleQuality = pResource->MultisampleQuality;
    pRc->RcDesc.MipLevels = pResource->MipLevels;
    pRc->RcDesc.Fvf = pResource->Fvf;
    pRc->RcDesc.VidPnSourceId = pResource->VidPnSourceId;
    pRc->RcDesc.RefreshRate = pResource->RefreshRate;
    pRc->RcDesc.enmRotation = pResource->Rotation;
    pRc->cAllocations = pResource->SurfCount;
    for (UINT i = 0; i < pResource->SurfCount; ++i)
    {
        PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
        CONST D3DDDI_SURFACEINFO* pSurf = &pResource->pSurfList[i];
        pAllocation->hAllocation = NULL;
        pAllocation->enmType = VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC;
        pAllocation->iAlloc = i;
        pAllocation->pRc = pRc;
        pAllocation->SurfDesc.d3dWidth = pSurf->Width;
        pAllocation->pvMem = (void*)pSurf->pSysMem;
        pAllocation->SurfDesc.slicePitch = pSurf->SysMemSlicePitch;
        pAllocation->SurfDesc.depth = pSurf->Depth;
        pAllocation->SurfDesc.width = pSurf->Width;
        pAllocation->SurfDesc.height = pSurf->Height;
        pAllocation->SurfDesc.format = pResource->Format;
        if (!vboxWddmFormatToFourcc(pResource->Format))
            pAllocation->SurfDesc.bpp = vboxWddmCalcBitsPerPixel(pResource->Format);
        else
            pAllocation->SurfDesc.bpp = 0;

        if (pSurf->SysMemPitch)
            pAllocation->SurfDesc.pitch = pSurf->SysMemPitch;
        else
            pAllocation->SurfDesc.pitch = vboxWddmCalcPitch(pSurf->Width, pResource->Format);

        pAllocation->SurfDesc.cbSize = vboxWddmCalcSize(pAllocation->SurfDesc.pitch, pAllocation->SurfDesc.height,
                                                        pAllocation->SurfDesc.format);

        pAllocation->SurfDesc.VidPnSourceId = pResource->VidPnSourceId;

        if (pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM)
        {
            Assert(pAllocation->pvMem);
            Assert(pAllocation->SurfDesc.pitch);
            UINT minPitch = vboxWddmCalcPitch(pAllocation->SurfDesc.width, pAllocation->SurfDesc.format);
            Assert(minPitch);
            if (minPitch)
            {
                if (pAllocation->SurfDesc.pitch != minPitch)
                {
                    Assert(pAllocation->SurfDesc.pitch > minPitch);
                    pAllocation->SurfDesc.d3dWidth = vboxWddmCalcWidthForPitch(pAllocation->SurfDesc.pitch,
                                                                               pAllocation->SurfDesc.format);
                    Assert(VBOXWDDMDISP_IS_TEXTURE(pRc->RcDesc.fFlags) && !pRc->RcDesc.fFlags.CubeMap); /* <- tested for textures only! */
                }
                Assert(pAllocation->SurfDesc.d3dWidth >= pAllocation->SurfDesc.width);
            }
            else
            {
                Assert(pAllocation->SurfDesc.d3dWidth == pAllocation->SurfDesc.width);
            }
        }

    }

    if (VBOXDISPMODE_IS_3D(pAdapter))
    {
        if (pRc->RcDesc.fFlags.SharedResource)
        {
            bIssueCreateResource = true;
            bCreateKMResource = true;
        }

        if (pRc->RcDesc.fFlags.RenderTarget || pRc->RcDesc.fFlags.Primary)
        {
            bIssueCreateResource = true;
            bSetHostID = true;
        }

        hr = VBoxD3DIfCreateForRc(pRc);
        if (!SUCCEEDED(hr))
        {
            WARN(("VBoxD3DIfCreateForRc failed, hr 0x%x", hr));
        }
    }
    else
    {
        bIssueCreateResource = (pResource->Pool != D3DDDIPOOL_SYSTEMMEM) || pResource->Flags.RenderTarget;
        bCreateKMResource = bIssueCreateResource;
    }

    if (SUCCEEDED(hr) && bIssueCreateResource)
    {
        pRc->fFlags.KmResource = bCreateKMResource;
        D3DDDICB_ALLOCATE *pDdiAllocate = vboxWddmRequestAllocAlloc(pResource);
        Assert(pDdiAllocate);
        if (pDdiAllocate)
        {
            Assert(pDdiAllocate->pPrivateDriverData);
            Assert(pDdiAllocate->PrivateDriverDataSize == sizeof (VBOXWDDM_RCINFO));
            PVBOXWDDM_RCINFO pRcInfo = (PVBOXWDDM_RCINFO)pDdiAllocate->pPrivateDriverData;
            pRcInfo->fFlags = pRc->fFlags;
            pRcInfo->RcDesc = pRc->RcDesc;
            pRcInfo->cAllocInfos = pResource->SurfCount;

            for (UINT i = 0; i < pResource->SurfCount; ++i)
            {
                D3DDDI_ALLOCATIONINFO *pDdiAllocI = &pDdiAllocate->pAllocationInfo[i];
                PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
                Assert(pDdiAllocI->pPrivateDriverData);
                Assert(pDdiAllocI->PrivateDriverDataSize == sizeof (VBOXWDDM_ALLOCINFO));
                PVBOXWDDM_ALLOCINFO pAllocInfo = (PVBOXWDDM_ALLOCINFO)pDdiAllocI->pPrivateDriverData;
                CONST D3DDDI_SURFACEINFO* pSurf = &pResource->pSurfList[i];
                pDdiAllocI->hAllocation = NULL;
                pDdiAllocI->pSystemMem = pSurf->pSysMem;
                Assert((!!(pSurf->pSysMem)) == (pResource->Pool == D3DDDIPOOL_SYSTEMMEM));
                pDdiAllocI->VidPnSourceId = pResource->VidPnSourceId;
                pDdiAllocI->Flags.Value = 0;
                if (pResource->Flags.Primary)
                {
                    Assert(pResource->Flags.RenderTarget);
                    pDdiAllocI->Flags.Primary = 1;
                }

                pAllocInfo->enmType = VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC;
                pAllocInfo->fFlags = pResource->Flags;
                pAllocInfo->hSharedHandle = (uintptr_t)pAllocation->hSharedHandle;
                pAllocInfo->SurfDesc = pAllocation->SurfDesc;
                if (bSetHostID)
                {
                    IDirect3DSurface9 *pSurfIf = NULL;
                    hr = VBoxD3DIfSurfGet(pRc, i, &pSurfIf);
                    if (SUCCEEDED(hr))
                    {
                        hr = pAdapter->D3D.D3D.pfnVBoxWineExD3DSurf9GetHostId(pSurfIf, &pAllocInfo->hostID);
                        if (SUCCEEDED(hr))
                        {
                            Assert(pAllocInfo->hostID);
                        }
                        else
                        {
                            WARN(("pfnVBoxWineExD3DSurf9GetHostId failed, hr 0x%x", hr));
                            break;
                        }
                        pSurfIf->Release();
                    }
                    else
                    {
                        WARN(("VBoxD3DIfSurfGet failed, hr 0x%x", hr));
                        break;
                    }
                }
                else
                    pAllocInfo->hostID = 0;
            }

            Assert(!pRc->fFlags.Opened);
//                Assert(!pRc->fFlags.KmResource);
            Assert(pRc->fFlags.Generic);

            if (SUCCEEDED(hr))
            {
                if (bCreateKMResource)
                {
                    Assert(pRc->fFlags.KmResource);

                    hr = pDevice->RtCallbacks.pfnAllocateCb(pDevice->hDevice, pDdiAllocate);
                    Assert(hr == S_OK);
                    Assert(pDdiAllocate->hKMResource
                            || pResource->Flags.SharedResource /* for some reason shared resources
                                                                * are created with zero km resource handle on Win7+ */
                            );
                }
                else
                {
                    Assert(!pRc->fFlags.KmResource);

                    pDdiAllocate->hResource = NULL;
                    pDdiAllocate->NumAllocations = 1;
                    pDdiAllocate->PrivateDriverDataSize = 0;
                    pDdiAllocate->pPrivateDriverData = NULL;
                    D3DDDI_ALLOCATIONINFO *pDdiAllocIBase = pDdiAllocate->pAllocationInfo;

                    for (UINT i = 0; i < pResource->SurfCount; ++i)
                    {
                        pDdiAllocate->pAllocationInfo = &pDdiAllocIBase[i];
                        hr = pDevice->RtCallbacks.pfnAllocateCb(pDevice->hDevice, pDdiAllocate);
                        Assert(hr == S_OK);
                        Assert(!pDdiAllocate->hKMResource);
                        if (SUCCEEDED(hr))
                        {
                            Assert(pDdiAllocate->pAllocationInfo->hAllocation);
                        }
                        else
                        {
                            for (UINT j = 0; j < i; ++j)
                            {
                                D3DDDI_ALLOCATIONINFO * pCur = &pDdiAllocIBase[i];
                                D3DDDICB_DEALLOCATE Dealloc;
                                Dealloc.hResource = 0;
                                Dealloc.NumAllocations = 1;
                                Dealloc.HandleList = &pCur->hAllocation;
                                HRESULT hr2 = pDevice->RtCallbacks.pfnDeallocateCb(pDevice->hDevice, &Dealloc);
                                Assert(hr2 == S_OK); NOREF(hr2);
                            }
                            break;
                        }
                    }

                    pDdiAllocate->pAllocationInfo = pDdiAllocIBase;
                }

                if (SUCCEEDED(hr))
                {
                    pRc->hKMResource = pDdiAllocate->hKMResource;

                    for (UINT i = 0; i < pResource->SurfCount; ++i)
                    {
                        PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
                        D3DDDI_ALLOCATIONINFO *pDdiAllocI = &pDdiAllocate->pAllocationInfo[i];
                        PVBOXWDDM_ALLOCINFO pAllocInfo = (PVBOXWDDM_ALLOCINFO)pDdiAllocI->pPrivateDriverData;
                        CONST D3DDDI_SURFACEINFO* pSurf = &pResource->pSurfList[i];
                        pAllocation->hAllocation = pDdiAllocI->hAllocation;
                        pAllocation->enmType = VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC;
                        pAllocation->pvMem = (void*)pSurf->pSysMem;
                        pAllocation->SurfDesc = pAllocInfo->SurfDesc;

                        if (pResource->Flags.SharedResource)
                        {
#ifdef VBOXWDDMDISP_DEBUG_PRINT_SHARED_CREATE
                            Assert(VBOXWDDMDISP_IS_TEXTURE(pResource->Flags));
                            vboxVDbgPrint(("\n\n********\n(0x%x:0n%d)Shared CREATED pAlloc(0x%p), hRc(0x%p), hAl(0x%p), "
                                            "Handle(0x%x), (0n%d) \n***********\n\n",
                                           GetCurrentProcessId(), GetCurrentProcessId(),
                                           pAllocation, pRc->hKMResource, pAllocation->hAllocation,
                                           pAllocation->hSharedHandle, pAllocation->hSharedHandle ));
#endif
                        }
                    }

                    VBOXVDBG_CREATE_CHECK_SWAPCHAIN();
                }
            }

            vboxWddmRequestAllocFree(pDdiAllocate);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    VBOXVDBG_BREAK_SHARED(pRc);

    if (SUCCEEDED(hr))
    {
        pResource->hResource = pRc;
        hr = S_OK;
    }
    else
        vboxResourceFree(pRc);

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevDestroyResource(HANDLE hDevice, HANDLE hResource)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    PVBOXWDDMDISP_ADAPTER pAdapter = pDevice->pAdapter;
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)hResource;

    HRESULT hr = S_OK;

    Assert(pDevice);
    Assert(hResource);

    if (VBOXDISPMODE_IS_3D(pAdapter))
    {
        for (UINT i = 0; i < pRc->cAllocations; ++i)
        {
            PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[i];
            if (pAlloc->hSharedHandle)
            {
#ifdef VBOXWDDMDISP_DEBUG_PRINT_SHARED_CREATE
                vboxVDbgPrint(("\n\n********\n(0x%x:0n%d)Shared DESTROYED pAlloc(0x%p), hRc(0x%p), hAl(0x%p), "
                                "Handle(0x%x), (0n%d) \n***********\n\n",
                            GetCurrentProcessId(), GetCurrentProcessId(),
                            pAlloc, pRc->hKMResource, pAlloc->hAllocation,
                            pAlloc->hSharedHandle, pAlloc->hSharedHandle
                            ));
#endif
            }

            if (pAlloc->pD3DIf)
                pAlloc->pD3DIf->Release();

            PVBOXWDDMDISP_SWAPCHAIN pSwapchain = vboxWddmSwapchainForAlloc(pAlloc);
            if (pSwapchain)
            {
                PVBOXWDDMDISP_RENDERTGT pRt = vboxWddmSwapchainRtForAlloc(pSwapchain, pAlloc);
                vboxWddmSwapchainRtRemove(pSwapchain, pRt);
                Assert(!vboxWddmSwapchainForAlloc(pAlloc));
                if (!vboxWddmSwapchainNumRTs(pSwapchain))
                    vboxWddmSwapchainDestroy(pDevice, pSwapchain);
            }

            vboxWddmDalCheckNotifyRemove(pDevice, pAlloc);
        }
    }

    if (pRc->fFlags.KmResource)
    {
        D3DDDICB_DEALLOCATE Dealloc;
        Assert(pRc->hResource);
        Dealloc.hResource = pRc->hResource;
        /* according to the docs the below two are ignored in case we set the hResource */
        Dealloc.NumAllocations = 0;
        Dealloc.HandleList = NULL;
        hr = pDevice->RtCallbacks.pfnDeallocateCb(pDevice->hDevice, &Dealloc);
        Assert(hr == S_OK);
    }
    else
    {
        Assert(!(pRc->fFlags.Opened));
        for (UINT j = 0; j < pRc->cAllocations; ++j)
        {
            if (pRc->aAllocations[j].hAllocation)
            {
                D3DDDICB_DEALLOCATE Dealloc;
                Dealloc.hResource = NULL;
                Dealloc.NumAllocations = 1;
                Dealloc.HandleList = &pRc->aAllocations[j].hAllocation;
                HRESULT hr2 = pDevice->RtCallbacks.pfnDeallocateCb(pDevice->hDevice, &Dealloc);
                Assert(hr2 == S_OK); NOREF(hr2);
            }
        }
    }

    vboxResourceFree(pRc);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetDisplayMode(HANDLE hDevice, CONST D3DDDIARG_SETDISPLAYMODE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(VBOXDISPMODE_IS_3D(pDevice->pAdapter));
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hResource;
    Assert(pRc);
    Assert(pRc->cAllocations > pData->SubResourceIndex);
    PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[pData->SubResourceIndex];
    Assert(pRc->RcDesc.fFlags.RenderTarget);
    Assert(pRc->RcDesc.fFlags.Primary);
    Assert(pAlloc->hAllocation);
    D3DDDICB_SETDISPLAYMODE DdiDm = {0};
    DdiDm.hPrimaryAllocation = pAlloc->hAllocation;

    {
        hr = pDevice->RtCallbacks.pfnSetDisplayModeCb(pDevice->hDevice, &DdiDm);
        Assert(hr == S_OK);
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}

#ifdef VBOXWDDM_TEST_UHGSMI
int vboxUhgsmiTst(PVBOXUHGSMI pUhgsmi, uint32_t cbBuf, uint32_t cNumCals, uint64_t * pTimeMs);
#endif

static HRESULT APIENTRY vboxWddmDDevPresent(HANDLE hDevice, CONST D3DDDIARG_PRESENT* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
//    VBOXDISPPROFILE_DDI_CHKDUMPRESET(pDevice);
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_ADAPTER pAdapter = pDevice->pAdapter;
    PVBOXWDDMDISP_RESOURCE pSrcRc = NULL, pDstRc = NULL;
    PVBOXWDDMDISP_ALLOCATION pSrcAlloc = NULL, pDstAlloc = NULL;

    Assert(vboxWddmDalIsEmpty(pDevice));

    if (pData->hSrcResource)
    {
        pSrcRc = (PVBOXWDDMDISP_RESOURCE)pData->hSrcResource;
        Assert(pSrcRc->cAllocations > pData->SrcSubResourceIndex);
        pSrcAlloc = &pSrcRc->aAllocations[pData->SrcSubResourceIndex];
        Assert(pSrcAlloc->hAllocation);
    }

    if (pData->hDstResource)
    {
        pDstRc = (PVBOXWDDMDISP_RESOURCE)pData->hDstResource;
        Assert(pDstRc->cAllocations > pData->DstSubResourceIndex);
        pDstAlloc = &pDstRc->aAllocations[pData->DstSubResourceIndex];
        Assert(pDstAlloc->hAllocation);
    }

    if (VBOXDISPMODE_IS_3D(pAdapter))
    {
#ifdef VBOXWDDM_TEST_UHGSMI
        {
            static uint32_t cCals = 100000;
            static uint32_t cbData = 8 * 1024 * 1024;
            uint64_t TimeMs;
            int rc = vboxUhgsmiTst(&pDevice->Uhgsmi.Base, cbData, cCals, &TimeMs);
            uint32_t cCPS = (((uint64_t)cCals) * 1000ULL)/TimeMs;
        }
#endif
#ifdef VBOX_WITH_CROGL
        if (pAdapter->u32VBox3DCaps & CR_VBOX_CAP_TEX_PRESENT)
        {
            IDirect3DSurface9 *pSrcSurfIf = NULL;
            hr = VBoxD3DIfSurfGet(pSrcRc, pData->SrcSubResourceIndex, &pSrcSurfIf);
            if (SUCCEEDED(hr))
            {
                pAdapter->D3D.D3D.pfnVBoxWineExD3DSurf9SyncToHost(pSrcSurfIf);
                pSrcSurfIf->Release();
            }
            else
            {
                WARN(("VBoxD3DIfSurfGet failed, hr = 0x%x", hr));
                return hr;
            }

            pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9FlushToHost((IDirect3DDevice9Ex*)pDevice->pDevice9If);
        }
        else
#endif
        {
            pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9FlushToHost((IDirect3DDevice9Ex*)pDevice->pDevice9If);
            PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hSrcResource;
            Assert(pRc);
            Assert(pRc->cAllocations > pData->SrcSubResourceIndex);
            PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[pData->SrcSubResourceIndex];
            hr = vboxWddmSwapchainPresent(pDevice, pAlloc);
            Assert(hr == S_OK);
        }
    }

    D3DDDICB_PRESENT DdiPresent = {0};
    if (pSrcAlloc)
        DdiPresent.hSrcAllocation = pSrcAlloc->hAllocation;

    if (pDstAlloc)
        DdiPresent.hDstAllocation = pDstAlloc->hAllocation;

    DdiPresent.hContext = pDevice->DefaultContext.ContextInfo.hContext;

#if 0 //def VBOX_WDDMDISP_WITH_PROFILE
    VBoxDispProfileScopeLogger<VBoxDispProfileEntry> profilePresentCbLogger(pDevice->ProfileDdiPresentCb.alloc("pfnPresentCb"));
#endif

#ifdef VBOXWDDMDISP_DEBUG_TIMER
    HANDLE hTimer = NULL;
    vboxVDbgTimerStart(pDevice->hTimerQueue, &hTimer, 1000);
#endif
    hr = pDevice->RtCallbacks.pfnPresentCb(pDevice->hDevice, &DdiPresent);
#ifdef VBOXWDDMDISP_DEBUG_TIMER
    vboxVDbgTimerStop(pDevice->hTimerQueue, hTimer);
#endif
#if 0 //def VBOX_WDDMDISP_WITH_PROFILE
    profilePresentCbLogger.logAndDisable();
    if (pDevice->ProfileDdiPresentCb.getNumEntries() == 64)
    {
        pDevice->ProfileDdiPresentCb.dump(pDevice);
        pDevice->ProfileDdiPresentCb.reset();
    }
#endif
    Assert(hr == S_OK);

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));

    VBOXDISPPROFILE_DDI_REPORT_FRAME(pDevice);

    return hr;
}

static HRESULT APIENTRY vboxWddmDDevFlush(HANDLE hDevice)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    HRESULT hr = S_OK;
    if (   VBOXDISPMODE_IS_3D(pDevice->pAdapter)
        && pDevice->pDevice9If) /* Windows 10 can call the Flush when pDevice9If is not yet initialized. */
    {

        hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9Flush((IDirect3DDevice9Ex*)pDevice->pDevice9If);
        Assert(hr == S_OK);

        vboxWddmDalNotifyChange(pDevice);

        VBOXVDBG_DUMP_FLUSH(pDevice);
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));

    VBOXDISPPROFILE_DDI_REPORT_FLUSH(pDevice);

    return hr;
}

AssertCompile(sizeof (D3DDDIVERTEXELEMENT) == sizeof (D3DVERTEXELEMENT9));
AssertCompile(RT_SIZEOFMEMB(D3DDDIVERTEXELEMENT, Stream) == RT_SIZEOFMEMB(D3DVERTEXELEMENT9, Stream));
AssertCompile(RT_SIZEOFMEMB(D3DDDIVERTEXELEMENT, Offset) == RT_SIZEOFMEMB(D3DVERTEXELEMENT9, Offset));
AssertCompile(RT_SIZEOFMEMB(D3DDDIVERTEXELEMENT, Type) == RT_SIZEOFMEMB(D3DVERTEXELEMENT9, Type));
AssertCompile(RT_SIZEOFMEMB(D3DDDIVERTEXELEMENT, Method) == RT_SIZEOFMEMB(D3DVERTEXELEMENT9, Method));
AssertCompile(RT_SIZEOFMEMB(D3DDDIVERTEXELEMENT, Usage) == RT_SIZEOFMEMB(D3DVERTEXELEMENT9, Usage));
AssertCompile(RT_SIZEOFMEMB(D3DDDIVERTEXELEMENT, UsageIndex) == RT_SIZEOFMEMB(D3DVERTEXELEMENT9, UsageIndex));

AssertCompile(RT_OFFSETOF(D3DDDIVERTEXELEMENT, Stream) == RT_OFFSETOF(D3DVERTEXELEMENT9, Stream));
AssertCompile(RT_OFFSETOF(D3DDDIVERTEXELEMENT, Offset) == RT_OFFSETOF(D3DVERTEXELEMENT9, Offset));
AssertCompile(RT_OFFSETOF(D3DDDIVERTEXELEMENT, Type) == RT_OFFSETOF(D3DVERTEXELEMENT9, Type));
AssertCompile(RT_OFFSETOF(D3DDDIVERTEXELEMENT, Method) == RT_OFFSETOF(D3DVERTEXELEMENT9, Method));
AssertCompile(RT_OFFSETOF(D3DDDIVERTEXELEMENT, Usage) == RT_OFFSETOF(D3DVERTEXELEMENT9, Usage));
AssertCompile(RT_OFFSETOF(D3DDDIVERTEXELEMENT, UsageIndex) == RT_OFFSETOF(D3DVERTEXELEMENT9, UsageIndex));

static HRESULT APIENTRY vboxWddmDDevCreateVertexShaderDecl(HANDLE hDevice, D3DDDIARG_CREATEVERTEXSHADERDECL* pData,
                                                           CONST D3DDDIVERTEXELEMENT* pVertexElements)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    IDirect3DVertexDeclaration9 *pDecl;
    static D3DVERTEXELEMENT9 DeclEnd = D3DDECL_END();
    D3DVERTEXELEMENT9* pVe;
    HRESULT hr = S_OK;
    bool bFreeVe = false;
    if(memcmp(&DeclEnd, &pVertexElements[pData->NumVertexElements], sizeof (DeclEnd)))
    {
        pVe = (D3DVERTEXELEMENT9*)RTMemAlloc(sizeof (D3DVERTEXELEMENT9) * (pData->NumVertexElements + 1));
        if (pVe)
        {
            memcpy(pVe, pVertexElements, sizeof (D3DVERTEXELEMENT9) * pData->NumVertexElements);
            pVe[pData->NumVertexElements] = DeclEnd;
            bFreeVe = true;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        pVe = (D3DVERTEXELEMENT9*)pVertexElements;

    if (hr == S_OK)
    {
        hr = pDevice9If->CreateVertexDeclaration(
                pVe,
                &pDecl
              );
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            Assert(pDecl);
            pData->ShaderHandle = pDecl;
        }
    }

    if (bFreeVe)
        RTMemFree((void*)pVe);

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetVertexShaderDecl(HANDLE hDevice, HANDLE hShaderHandle)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    IDirect3DVertexDeclaration9 *pDecl = (IDirect3DVertexDeclaration9*)hShaderHandle;
    Assert(pDecl);
    HRESULT hr = pDevice9If->SetVertexDeclaration(pDecl);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevDeleteVertexShaderDecl(HANDLE hDevice, HANDLE hShaderHandle)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DVertexDeclaration9 *pDecl = (IDirect3DVertexDeclaration9*)hShaderHandle;
    HRESULT hr = S_OK;
    pDecl->Release();
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevCreateVertexShaderFunc(HANDLE hDevice, D3DDDIARG_CREATEVERTEXSHADERFUNC* pData,
                                                           CONST UINT* pCode)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    IDirect3DVertexShader9 *pShader;
    Assert(*((UINT*)((uint8_t*)pCode + pData->Size-4)) == 0x0000FFFF /* end token */);
    HRESULT hr = pDevice9If->CreateVertexShader((const DWORD *)pCode, &pShader);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        Assert(pShader);
        pData->ShaderHandle = pShader;
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetVertexShaderFunc(HANDLE hDevice, HANDLE hShaderHandle)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    IDirect3DVertexShader9 *pShader = (IDirect3DVertexShader9*)hShaderHandle;
    HRESULT hr = pDevice9If->SetVertexShader(pShader);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevDeleteVertexShaderFunc(HANDLE hDevice, HANDLE hShaderHandle)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DVertexShader9 *pShader = (IDirect3DVertexShader9*)hShaderHandle;
    HRESULT hr = S_OK;
    pShader->Release();
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetVertexShaderConstI(HANDLE hDevice, CONST D3DDDIARG_SETVERTEXSHADERCONSTI* pData,
                                                          CONST INT* pRegisters)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetVertexShaderConstantI(pData->Register, pRegisters, pData->Count);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetVertexShaderConstB(HANDLE hDevice, CONST D3DDDIARG_SETVERTEXSHADERCONSTB* pData,
                                                          CONST BOOL* pRegisters)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetVertexShaderConstantB(pData->Register, pRegisters, pData->Count);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetScissorRect(HANDLE hDevice, CONST RECT* pRect)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetScissorRect(pRect);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetStreamSource(HANDLE hDevice, CONST D3DDDIARG_SETSTREAMSOURCE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hVertexBuffer;
    PVBOXWDDMDISP_ALLOCATION pAlloc = NULL;
    IDirect3DVertexBuffer9 *pStreamData = NULL;
    if (pRc)
    {
        VBOXVDBG_CHECK_SMSYNC(pRc);
        Assert(pRc->cAllocations == 1);
        pAlloc = &pRc->aAllocations[0];
        Assert(pAlloc->pD3DIf);
        pStreamData = (IDirect3DVertexBuffer9*)pAlloc->pD3DIf;
    }
    HRESULT hr = pDevice9If->SetStreamSource(pData->Stream, pStreamData, pData->Offset, pData->Stride);
    Assert(hr == S_OK);
    Assert(pData->Stream<VBOXWDDMDISP_MAX_VERTEX_STREAMS);
    if (hr == S_OK)
    {
        if (pDevice->aStreamSource[pData->Stream] && !pAlloc)
        {
            --pDevice->cStreamSources;
            Assert(pDevice->cStreamSources < UINT32_MAX/2);
        }
        else if (!pDevice->aStreamSource[pData->Stream] && pAlloc)
        {
            ++pDevice->cStreamSources;
            Assert(pDevice->cStreamSources <= RT_ELEMENTS(pDevice->aStreamSource));
        }
        pDevice->aStreamSource[pData->Stream] = pAlloc;
        pDevice->StreamSourceInfo[pData->Stream].uiOffset = pData->Offset;
        pDevice->StreamSourceInfo[pData->Stream].uiStride = pData->Stride;

        PVBOXWDDMDISP_STREAMSOURCEUM pStrSrcUm = &pDevice->aStreamSourceUm[pData->Stream];
        if (pStrSrcUm->pvBuffer)
        {
            --pDevice->cStreamSourcesUm;
            Assert(pDevice->cStreamSourcesUm < UINT32_MAX/2);
            pStrSrcUm->pvBuffer = NULL;
            pStrSrcUm->cbStride = 0;
        }
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetStreamSourceFreq(HANDLE hDevice, CONST D3DDDIARG_SETSTREAMSOURCEFREQ* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetStreamSourceFreq(pData->Stream, pData->Divider);
    if (SUCCEEDED(hr))
        hr = S_OK;
    else
        WARN(("SetStreamSourceFreq failed hr 0x%x", hr));

#ifdef DEBUG_misha
    /* test it more */
    Assert(0);
#endif
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetConvolutionKernelMono(HANDLE hDevice, CONST D3DDDIARG_SETCONVOLUTIONKERNELMONO* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevComposeRects(HANDLE hDevice, CONST D3DDDIARG_COMPOSERECTS* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDDevBlt(HANDLE hDevice, CONST D3DDDIARG_BLT* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
//    PVBOXWDDMDISP_SCREEN pScreen = &pDevice->aScreens[pDevice->iPrimaryScreen];
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pDstRc = (PVBOXWDDMDISP_RESOURCE)pData->hDstResource;
    PVBOXWDDMDISP_RESOURCE pSrcRc = (PVBOXWDDMDISP_RESOURCE)pData->hSrcResource;
    VBOXVDBG_CHECK_SMSYNC(pDstRc);
    VBOXVDBG_CHECK_SMSYNC(pSrcRc);
    Assert(pDstRc->cAllocations > pData->DstSubResourceIndex);
    Assert(pSrcRc->cAllocations > pData->SrcSubResourceIndex);
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_ALLOCATION pSrcAlloc = &pSrcRc->aAllocations[pData->SrcSubResourceIndex];
    PVBOXWDDMDISP_ALLOCATION pDstAlloc = &pDstRc->aAllocations[pData->DstSubResourceIndex];
    PVBOXWDDMDISP_SWAPCHAIN pSrcSwapchain = vboxWddmSwapchainForAlloc(pSrcAlloc);
    PVBOXWDDMDISP_SWAPCHAIN pDstSwapchain = vboxWddmSwapchainForAlloc(pDstAlloc);
    /* try StretchRect */
    IDirect3DSurface9 *pSrcSurfIf = NULL;
    IDirect3DSurface9 *pDstSurfIf = NULL;
    Assert(   !pDstSwapchain
           || vboxWddmSwapchainGetFb(pDstSwapchain)->pAlloc != pDstAlloc
           || vboxWddmSwapchainNumRTs(pDstSwapchain) == 1); NOREF(pDstSwapchain);

    VBOXVDBG_BREAK_SHARED(pSrcRc);
    VBOXVDBG_BREAK_SHARED(pDstRc);

    hr = VBoxD3DIfSurfGet(pDstRc, pData->DstSubResourceIndex, &pDstSurfIf);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        Assert(pDstSurfIf);
        do
        {
            if (pSrcSwapchain)
            {
                hr = vboxWddmSwapchainSurfGet(pDevice, pSrcSwapchain, pSrcAlloc, &pSrcSurfIf);
                Assert(hr == S_OK);
            }
            else
            {
                hr = VBoxD3DIfSurfGet(pSrcRc, pData->SrcSubResourceIndex, &pSrcSurfIf);
                Assert(hr == S_OK);
            }

            if (hr == S_OK)
            {
                Assert(pSrcSurfIf);

                VBOXVDBG_BREAK_SHARED(pSrcRc);
                VBOXVDBG_BREAK_SHARED(pDstRc);

                /* we support only Point & Linear, we ignore [Begin|Continue|End]PresentToDwm */
                Assert((pData->Flags.Value & (~(0x00000100 | 0x00000200 | 0x00000400 | 0x00000001  | 0x00000002))) == 0);
                VBOXVDBG_CHECK_BLT(hr = pDevice9If->StretchRect(pSrcSurfIf, &pData->SrcRect, pDstSurfIf, &pData->DstRect,
                                                                vboxDDI2D3DBltFlags(pData->Flags)); Assert(hr == S_OK),
                                   pSrcAlloc, pSrcSurfIf, &pData->SrcRect, pDstAlloc, pDstSurfIf, &pData->DstRect);

                pSrcSurfIf->Release();
            }
        } while (0);

        pDstSurfIf->Release();
    }

    if (hr != S_OK)
    {
        /** @todo fallback to memcpy or whatever ? */
        Assert(0);
    }

    PVBOXWDDMDISP_ALLOCATION pDAlloc = &pDstRc->aAllocations[pData->DstSubResourceIndex];
    vboxWddmDalCheckAdd(pDevice, pDAlloc, TRUE);
    pDAlloc = &pSrcRc->aAllocations[pData->SrcSubResourceIndex];
    vboxWddmDalCheckAdd(pDevice, pDAlloc, FALSE);

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevColorFill(HANDLE hDevice, CONST D3DDDIARG_COLORFILL* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hResource;
    Assert(pRc);
    IDirect3DSurface9 *pSurfIf = NULL;
    HRESULT hr = VBoxD3DIfSurfGet(pRc, pData->SubResourceIndex, &pSurfIf);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        VBOXVDBG_CHECK_SMSYNC(pRc);
        Assert(pSurfIf);
        hr = pDevice9If->ColorFill(pSurfIf, &pData->DstRect, pData->Color);
        Assert(hr == S_OK);
        /** @todo check what need to do when PresentToDwm flag is set */
        Assert(pData->Flags.Value == 0);

        pSurfIf->Release();

        PVBOXWDDMDISP_ALLOCATION pDAlloc = &pRc->aAllocations[pData->SubResourceIndex];
        vboxWddmDalCheckAdd(pDevice, pDAlloc, TRUE);
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevDepthFill(HANDLE hDevice, CONST D3DDDIARG_DEPTHFILL* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);
    Assert(0);
/// @todo    vboxWddmDalCheckAdd(pDevice, pDAlloc, TRUE);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevCreateQuery(HANDLE hDevice, D3DDDIARG_CREATEQUERY* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_QUERY pQuery = (PVBOXWDDMDISP_QUERY)RTMemAllocZ(sizeof (VBOXWDDMDISP_QUERY));
    if (!pQuery)
    {
        WARN(("RTMemAllocZ failed"));
        return E_OUTOFMEMORY;
    }

    hr = pDevice9If->CreateQuery(vboxDDI2D3DQueryType(pData->QueryType), &pQuery->pQueryIf);
    if (FAILED(hr))
    {
        WARN(("CreateQuery failed, hr 0x%x", hr));
        RTMemFree(pQuery);
        return hr;
    }

    Assert(hr == S_OK);

    pQuery->enmType = pData->QueryType;
    pData->hQuery = pQuery;

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevDestroyQuery(HANDLE hDevice, HANDLE hQuery)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    PVBOXWDDMDISP_QUERY pQuery = (PVBOXWDDMDISP_QUERY)hQuery;
    Assert(pQuery);
    pQuery->pQueryIf->Release();
    RTMemFree(pQuery);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevIssueQuery(HANDLE hDevice, CONST D3DDDIARG_ISSUEQUERY* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    PVBOXWDDMDISP_QUERY pQuery = (PVBOXWDDMDISP_QUERY)pData->hQuery;
    Assert(pQuery);
    pQuery->fQueryState.Value |= pData->Flags.Value;
    hr = pQuery->pQueryIf->Issue(vboxDDI2D3DIssueQueryFlags(pData->Flags));
    if (hr != S_OK)
        WARN(("Issue failed, hr = 0x%x", hr));

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevGetQueryData(HANDLE hDevice, CONST D3DDDIARG_GETQUERYDATA* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    PVBOXWDDMDISP_QUERY pQuery = (PVBOXWDDMDISP_QUERY)pData->hQuery;
    Assert(pQuery);
    DWORD cbData = pQuery->pQueryIf->GetDataSize();
#ifdef DEBUG
    switch (pQuery->enmType)
    {
        case D3DDDIQUERYTYPE_EVENT:
            Assert(cbData == sizeof (BOOL));
            break;
        case D3DDDIQUERYTYPE_OCCLUSION:
            Assert(cbData == sizeof (UINT));
            break;
        default:
            Assert(0);
            break;
    }
#endif
    hr = pQuery->pQueryIf->GetData(pData->pData, cbData, 0);
    if (hr != S_OK)
        WARN(("GetData failed, hr = 0x%x", hr));

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetRenderTarget(HANDLE hDevice, CONST D3DDDIARG_SETRENDERTARGET* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    /*IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice); - unused */
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hRenderTarget;
    PVBOXWDDMDISP_ALLOCATION pAlloc = NULL;
    if (pRc)
    {
        VBOXVDBG_CHECK_SMSYNC(pRc);
        Assert(pRc);
        Assert(pData->SubResourceIndex < pRc->cAllocations);
        pAlloc = &pRc->aAllocations[pData->SubResourceIndex];
    }
    HRESULT hr = vboxWddmRenderTargetSet(pDevice, pData->RenderTargetIndex, pAlloc, FALSE);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetDepthStencil(HANDLE hDevice, CONST D3DDDIARG_SETDEPTHSTENCIL* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hZBuffer;
    IDirect3DSurface9 *pD3D9Surf = NULL;
    HRESULT hr = S_OK;
    if (pRc)
    {
        VBOXVDBG_CHECK_SMSYNC(pRc);
        Assert(pRc->cAllocations == 1);
        hr = VBoxD3DIfSurfGet(pRc, 0, &pD3D9Surf);
        if (FAILED(hr))
            WARN(("VBoxD3DIfSurfGet failed, hr (0x%x)",hr));
        else
            Assert(pD3D9Surf);
    }

    if (SUCCEEDED(hr))
    {
        hr = pDevice9If->SetDepthStencilSurface(pD3D9Surf);
        if (SUCCEEDED(hr))
        {
            pDevice->pDepthStencilRc = pRc;
            hr = S_OK;
        }
        else
            WARN(("VBoxD3DIfSurfGet failed, hr (0x%x)",hr));

        if (pD3D9Surf)
            pD3D9Surf->Release();
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevGenerateMipSubLevels(HANDLE hDevice, CONST D3DDDIARG_GENERATEMIPSUBLEVELS* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetPixelShaderConstI(HANDLE hDevice, CONST D3DDDIARG_SETPIXELSHADERCONSTI* pData,
                                                         CONST INT* pRegisters)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetPixelShaderConstantI(pData->Register, pRegisters, pData->Count);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevSetPixelShaderConstB(HANDLE hDevice, CONST D3DDDIARG_SETPIXELSHADERCONSTB* pData,
                                                         CONST BOOL* pRegisters)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    HRESULT hr = pDevice9If->SetPixelShaderConstantB(pData->Register, pRegisters, pData->Count);
    Assert(hr == S_OK);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevCreatePixelShader(HANDLE hDevice, D3DDDIARG_CREATEPIXELSHADER* pData, CONST UINT* pCode)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);
    IDirect3DPixelShader9 *pShader;
    Assert(*((UINT*)((uint8_t*)pCode + pData->CodeSize-4)) == 0x0000FFFF /* end token */);
    HRESULT hr = pDevice9If->CreatePixelShader((const DWORD *)pCode, &pShader);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        Assert(pShader);
        pData->ShaderHandle = pShader;
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevDeletePixelShader(HANDLE hDevice, HANDLE hShaderHandle)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    IDirect3DPixelShader9 *pShader = (IDirect3DPixelShader9*)hShaderHandle;
    HRESULT hr = S_OK;
    pShader->Release();
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p), hr(0x%x)\n", hDevice, hr));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevCreateDecodeDevice(HANDLE hDevice, D3DDDIARG_CREATEDECODEDEVICE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDestroyDecodeDevice(HANDLE hDevice, HANDLE hDecodeDevice)
{
    RT_NOREF(hDecodeDevice);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetDecodeRenderTarget(HANDLE hDevice, CONST D3DDDIARG_SETDECODERENDERTARGET* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDecodeBeginFrame(HANDLE hDevice, D3DDDIARG_DECODEBEGINFRAME* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDecodeEndFrame(HANDLE hDevice, D3DDDIARG_DECODEENDFRAME* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDecodeExecute(HANDLE hDevice, CONST D3DDDIARG_DECODEEXECUTE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDecodeExtensionExecute(HANDLE hDevice, CONST D3DDDIARG_DECODEEXTENSIONEXECUTE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevCreateVideoProcessDevice(HANDLE hDevice, D3DDDIARG_CREATEVIDEOPROCESSDEVICE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDestroyVideoProcessDevice(HANDLE hDevice, HANDLE hVideoProcessor)
{
    RT_NOREF(hVideoProcessor);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevVideoProcessBeginFrame(HANDLE hDevice, HANDLE hVideoProcessor)
{
    RT_NOREF(hVideoProcessor);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevVideoProcessEndFrame(HANDLE hDevice, D3DDDIARG_VIDEOPROCESSENDFRAME* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetVideoProcessRenderTarget(HANDLE hDevice,
                                                                CONST D3DDDIARG_SETVIDEOPROCESSRENDERTARGET* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevVideoProcessBlt(HANDLE hDevice, CONST D3DDDIARG_VIDEOPROCESSBLT* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevCreateExtensionDevice(HANDLE hDevice, D3DDDIARG_CREATEEXTENSIONDEVICE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDestroyExtensionDevice(HANDLE hDevice, HANDLE hExtension)
{
    RT_NOREF(hExtension);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevExtensionExecute(HANDLE hDevice, CONST D3DDDIARG_EXTENSIONEXECUTE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDestroyDevice(IN HANDLE hDevice)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));

    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    VBOXDISPPROFILE_DDI_PRINT(("Dumping on DestroyDevice: 0x%p", pDevice));
    VBOXDISPPROFILE_DDI_TERM(pDevice);

#ifdef VBOXWDDMDISP_DEBUG_TIMER
        DeleteTimerQueueEx(pDevice->hTimerQueue, INVALID_HANDLE_VALUE /* see term */);
        pDevice->hTimerQueue = NULL;
#endif

    PVBOXWDDMDISP_ADAPTER pAdapter = pDevice->pAdapter;
    if (VBOXDISPMODE_IS_3D(pAdapter))
    {
        vboxWddmSwapchainDestroyAll(pDevice);
        /* ensure the device is destroyed in any way.
         * Release may not work in case of some leaking, which will leave the crOgl context refering the destroyed VBOXUHGSMI */
        if (pDevice->pDevice9If)
            pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9Term((IDirect3DDevice9Ex *)pDevice->pDevice9If);
    }

    HRESULT hr = vboxDispCmCtxDestroy(pDevice, &pDevice->DefaultContext);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        if (pDevice->hHgsmiTransportModule)
            FreeLibrary(pDevice->hHgsmiTransportModule);
        RTMemFree(pDevice);
    }
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}

AssertCompile(sizeof (RECT) == sizeof (D3DDDIRECT));
AssertCompile(RT_SIZEOFMEMB(RECT, left) == RT_SIZEOFMEMB(D3DDDIRECT, left));
AssertCompile(RT_SIZEOFMEMB(RECT, right) == RT_SIZEOFMEMB(D3DDDIRECT, right));
AssertCompile(RT_SIZEOFMEMB(RECT, top) == RT_SIZEOFMEMB(D3DDDIRECT, top));
AssertCompile(RT_SIZEOFMEMB(RECT, bottom) == RT_SIZEOFMEMB(D3DDDIRECT, bottom));
AssertCompile(RT_OFFSETOF(RECT, left) == RT_OFFSETOF(D3DDDIRECT, left));
AssertCompile(RT_OFFSETOF(RECT, right) == RT_OFFSETOF(D3DDDIRECT, right));
AssertCompile(RT_OFFSETOF(RECT, top) == RT_OFFSETOF(D3DDDIRECT, top));
AssertCompile(RT_OFFSETOF(RECT, bottom) == RT_OFFSETOF(D3DDDIRECT, bottom));

static HRESULT APIENTRY vboxWddmDDevCreateOverlay(HANDLE hDevice, D3DDDIARG_CREATEOVERLAY* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->OverlayInfo.hResource;
    Assert(pRc);
    Assert(pRc->cAllocations > pData->OverlayInfo.SubResourceIndex);
    PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[pData->OverlayInfo.SubResourceIndex];
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_OVERLAY pOverlay = (PVBOXWDDMDISP_OVERLAY)RTMemAllocZ(sizeof (VBOXWDDMDISP_OVERLAY));
    Assert(pOverlay);
    if (pOverlay)
    {
        VBOXWDDM_OVERLAY_INFO OurInfo;
        OurInfo.OverlayDesc.DstColorKeyLow = pData->OverlayInfo.DstColorKeyLow;
        OurInfo.OverlayDesc.DstColorKeyHigh = pData->OverlayInfo.DstColorKeyHigh;
        OurInfo.OverlayDesc.SrcColorKeyLow = pData->OverlayInfo.SrcColorKeyLow;
        OurInfo.OverlayDesc.SrcColorKeyHigh = pData->OverlayInfo.SrcColorKeyHigh;
        OurInfo.OverlayDesc.fFlags = pData->OverlayInfo.Flags.Value;
        vboxWddmDirtyRegionClear(&OurInfo.DirtyRegion);
        Assert(!pAlloc->LockInfo.cLocks);
        vboxWddmDirtyRegionUnite(&OurInfo.DirtyRegion, &pAlloc->DirtyRegion);
        D3DDDICB_CREATEOVERLAY OverInfo;
        OverInfo.VidPnSourceId = pData->VidPnSourceId;
        OverInfo.OverlayInfo.hAllocation = pAlloc->hAllocation;
        Assert(pAlloc->hAllocation);
        OverInfo.OverlayInfo.DstRect = *(D3DDDIRECT*)((void*)&pData->OverlayInfo.DstRect);
        OverInfo.OverlayInfo.SrcRect = *(D3DDDIRECT*)((void*)&pData->OverlayInfo.SrcRect);
        OverInfo.OverlayInfo.pPrivateDriverData = &OurInfo;
        OverInfo.OverlayInfo.PrivateDriverDataSize = sizeof (OurInfo);
        OverInfo.hKernelOverlay = NULL; /* <-- out */
#ifndef VBOXWDDMOVERLAY_TEST
        hr = pDevice->RtCallbacks.pfnCreateOverlayCb(pDevice->hDevice, &OverInfo);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            Assert(OverInfo.hKernelOverlay);
            pOverlay->hOverlay = OverInfo.hKernelOverlay;
            pOverlay->VidPnSourceId = pData->VidPnSourceId;

            Assert(!pAlloc->LockInfo.cLocks);
            if (!pAlloc->LockInfo.cLocks)
            {
                /* we have reported the dirty rect, may clear it if no locks are pending currently */
                vboxWddmDirtyRegionClear(&pAlloc->DirtyRegion);
            }

            pData->hOverlay = pOverlay;
        }
        else
        {
            RTMemFree(pOverlay);
        }
#else
        pData->hOverlay = pOverlay;
#endif
    }
    else
        hr = E_OUTOFMEMORY;

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevUpdateOverlay(HANDLE hDevice, CONST D3DDDIARG_UPDATEOVERLAY* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->OverlayInfo.hResource;
    Assert(pRc);
    Assert(pRc->cAllocations > pData->OverlayInfo.SubResourceIndex);
    PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[pData->OverlayInfo.SubResourceIndex];
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_OVERLAY pOverlay = (PVBOXWDDMDISP_OVERLAY)pData->hOverlay;
    VBOXWDDM_OVERLAY_INFO OurInfo;
    OurInfo.OverlayDesc.DstColorKeyLow = pData->OverlayInfo.DstColorKeyLow;
    OurInfo.OverlayDesc.DstColorKeyHigh = pData->OverlayInfo.DstColorKeyHigh;
    OurInfo.OverlayDesc.SrcColorKeyLow = pData->OverlayInfo.SrcColorKeyLow;
    OurInfo.OverlayDesc.SrcColorKeyHigh = pData->OverlayInfo.SrcColorKeyHigh;
    OurInfo.OverlayDesc.fFlags = pData->OverlayInfo.Flags.Value;
    vboxWddmDirtyRegionClear(&OurInfo.DirtyRegion);
    Assert(!pAlloc->LockInfo.cLocks);
    vboxWddmDirtyRegionUnite(&OurInfo.DirtyRegion, &pAlloc->DirtyRegion);
    D3DDDICB_UPDATEOVERLAY OverInfo;
    OverInfo.hKernelOverlay = pOverlay->hOverlay;
    OverInfo.OverlayInfo.hAllocation = pAlloc->hAllocation;
    OverInfo.OverlayInfo.DstRect = *(D3DDDIRECT*)((void*)&pData->OverlayInfo.DstRect);
    OverInfo.OverlayInfo.SrcRect = *(D3DDDIRECT*)((void*)&pData->OverlayInfo.SrcRect);
    OverInfo.OverlayInfo.pPrivateDriverData = &OurInfo;
    OverInfo.OverlayInfo.PrivateDriverDataSize = sizeof (OurInfo);
#ifndef VBOXWDDMOVERLAY_TEST
    hr = pDevice->RtCallbacks.pfnUpdateOverlayCb(pDevice->hDevice, &OverInfo);
    Assert(hr == S_OK);
    if (hr == S_OK)
#endif
    {
        Assert(!pAlloc->LockInfo.cLocks);
        if (!pAlloc->LockInfo.cLocks)
        {
            /* we have reported the dirty rect, may clear it if no locks are pending currently */
            vboxWddmDirtyRegionClear(&pAlloc->DirtyRegion);
        }
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevFlipOverlay(HANDLE hDevice, CONST D3DDDIARG_FLIPOVERLAY* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->hSource;
    Assert(pRc);
    Assert(pRc->cAllocations > pData->SourceIndex);
    PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[pData->SourceIndex];
    HRESULT hr = S_OK;
    PVBOXWDDMDISP_OVERLAY pOverlay = (PVBOXWDDMDISP_OVERLAY)pData->hOverlay;
    VBOXWDDM_OVERLAYFLIP_INFO OurInfo;
    vboxWddmDirtyRegionClear(&OurInfo.DirtyRegion);
    Assert(!pAlloc->LockInfo.cLocks);
    vboxWddmDirtyRegionUnite(&OurInfo.DirtyRegion, &pAlloc->DirtyRegion);
    D3DDDICB_FLIPOVERLAY OverInfo;
    OverInfo.hKernelOverlay = pOverlay->hOverlay;
    OverInfo.hSource = pAlloc->hAllocation;
    OverInfo.pPrivateDriverData = &OurInfo;
    OverInfo.PrivateDriverDataSize = sizeof (OurInfo);
#ifndef VBOXWDDMOVERLAY_TEST
    hr = pDevice->RtCallbacks.pfnFlipOverlayCb(pDevice->hDevice, &OverInfo);
    Assert(hr == S_OK);
    if (hr == S_OK)
#endif
    {
        Assert(!pAlloc->LockInfo.cLocks);
        if (!pAlloc->LockInfo.cLocks)
        {
            /* we have reported the dirty rect, may clear it if no locks are pending currently */
            vboxWddmDirtyRegionClear(&pAlloc->DirtyRegion);
        }
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevGetOverlayColorControls(HANDLE hDevice, D3DDDIARG_GETOVERLAYCOLORCONTROLS* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevSetOverlayColorControls(HANDLE hDevice, CONST D3DDDIARG_SETOVERLAYCOLORCONTROLS* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}
static HRESULT APIENTRY vboxWddmDDevDestroyOverlay(HANDLE hDevice, CONST D3DDDIARG_DESTROYOVERLAY* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    PVBOXWDDMDISP_OVERLAY pOverlay = (PVBOXWDDMDISP_OVERLAY)pData->hOverlay;
    D3DDDICB_DESTROYOVERLAY OverInfo;
    OverInfo.hKernelOverlay = pOverlay->hOverlay;
#ifndef VBOXWDDMOVERLAY_TEST
    HRESULT hr = pDevice->RtCallbacks.pfnDestroyOverlayCb(pDevice->hDevice, &OverInfo);
    Assert(hr == S_OK);
    if (hr == S_OK)
#else
    HRESULT hr = S_OK;
#endif
    {
        RTMemFree(pOverlay);
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevQueryResourceResidency(HANDLE hDevice, CONST D3DDDIARG_QUERYRESOURCERESIDENCY* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
#if 0
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
#endif

    HRESULT hr = S_OK;
    /** @todo check residency for the "real" allocations */
#if 0
    for (UINT i = 0; i < pData->NumResources; ++i)
    {
        PVBOXWDDMDISP_RESOURCE pRc = (PVBOXWDDMDISP_RESOURCE)pData->pHandleList[i];
        Assert(pRc->pDevice == pDevice);
        if (pRc->hKMResource)
        {

        }
    }
#endif
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}

static HRESULT APIENTRY vboxWddmDDevOpenResource(HANDLE hDevice, D3DDDIARG_OPENRESOURCE* pData)
{
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    HRESULT hr = S_OK;

    Assert(pData->hKMResource);

    Assert(pData->NumAllocations);
    PVBOXWDDMDISP_RESOURCE pRc = vboxResourceAlloc(pData->NumAllocations);
    Assert(pRc);
    if (pRc)
    {
        pRc->hResource = pData->hResource;
        pRc->hKMResource = pData->hKMResource;
        pRc->pDevice = pDevice;
        pRc->RcDesc.enmRotation = pData->Rotation;
        pRc->fFlags.Value = 0;
        pRc->fFlags.Opened = 1;
        pRc->fFlags.KmResource = 1;

        for (UINT i = 0; i < pData->NumAllocations; ++i)
        {
            PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
            D3DDDI_OPENALLOCATIONINFO* pOAI = pData->pOpenAllocationInfo;
            Assert(pOAI->PrivateDriverDataSize == sizeof (VBOXWDDM_ALLOCINFO));
            if (pOAI->PrivateDriverDataSize != sizeof (VBOXWDDM_ALLOCINFO))
            {
                hr = E_INVALIDARG;
                break;
            }
            Assert(pOAI->pPrivateDriverData);
            PVBOXWDDM_ALLOCINFO pAllocInfo = (PVBOXWDDM_ALLOCINFO)pOAI->pPrivateDriverData;
            pAllocation->hAllocation = pOAI->hAllocation;
            pAllocation->enmType = pAllocInfo->enmType;
            pAllocation->hSharedHandle = (HANDLE)pAllocInfo->hSharedHandle;
            pAllocation->SurfDesc = pAllocInfo->SurfDesc;
            pAllocation->pvMem = NULL;
#ifndef VBOXWDDMDISP_DEBUG_NOSHARED
            Assert(!pAllocation->hSharedHandle == (pAllocation->enmType == VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE));
#endif
#ifdef VBOXWDDMDISP_DEBUG_PRINT_SHARED_CREATE
            vboxVDbgPrint(("\n\n********\n(0x%x:0n%d)Shared OPENNED pAlloc(0x%p), hRc(0x%p), hAl(0x%p), "
                            "Handle(0x%x), (0n%d) \n***********\n\n",
                        GetCurrentProcessId(), GetCurrentProcessId(),
                        pAllocation, pRc->hKMResource, pAllocation->hAllocation,
                        pAllocation->hSharedHandle, pAllocation->hSharedHandle
                        ));
#endif
        }
        if (!pData->pPrivateDriverData || !pData->PrivateDriverDataSize)
        {
            /* this is a "standard" allocation resource */

            /* both should be actually zero */
            Assert(!pData->pPrivateDriverData && !pData->PrivateDriverDataSize);
            pRc->RcDesc.enmPool = D3DDDIPOOL_LOCALVIDMEM;
            pRc->RcDesc.enmMultisampleType = D3DDDIMULTISAMPLE_NONE;
            pRc->RcDesc.MultisampleQuality = 0;
            pRc->RcDesc.MipLevels = 0;
            /*pRc->RcDesc.Fvf;*/
            pRc->RcDesc.fFlags.SharedResource = 1;

            if (pData->NumAllocations != 1)
            {
                WARN(("NumAllocations is expected to be 1, but was %d", pData->NumAllocations));
            }

            for (UINT i = 0; i < pData->NumAllocations; ++i)
            {
                PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[i];
                pAlloc->enmD3DIfType = VBOXDISP_D3DIFTYPE_SURFACE;
                pAlloc->pD3DIf = NULL;
            }

            D3DDDI_OPENALLOCATIONINFO* pDdiAllocInfo = &pData->pOpenAllocationInfo[0];
            Assert(pDdiAllocInfo->pPrivateDriverData);
            Assert(pDdiAllocInfo->PrivateDriverDataSize >= sizeof (VBOXWDDM_ALLOCINFO));
            if (pDdiAllocInfo->pPrivateDriverData && pDdiAllocInfo->PrivateDriverDataSize >= sizeof (VBOXWDDM_ALLOCINFO))
            {
                PVBOXWDDM_ALLOCINFO pAllocInfo = (PVBOXWDDM_ALLOCINFO)pDdiAllocInfo->pPrivateDriverData;
                switch(pAllocInfo->enmType)
                {
                    case VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE:
                        pRc->RcDesc.fFlags.Primary = 1;
                    case VBOXWDDM_ALLOC_TYPE_STD_SHADOWSURFACE:
                    case VBOXWDDM_ALLOC_TYPE_STD_STAGINGSURFACE:
                        pRc->RcDesc.enmFormat = pAllocInfo->SurfDesc.format;
                        pRc->RcDesc.VidPnSourceId = pAllocInfo->SurfDesc.VidPnSourceId;
                        pRc->RcDesc.RefreshRate = pAllocInfo->SurfDesc.RefreshRate;
                        break;
                    default:
                        Assert(0);
                        hr = E_INVALIDARG;
                }
            }
            else
                hr = E_INVALIDARG;
        }
        else
        {
            /* this is a "generic" resource whose creation is initiated by the UMD */
            Assert(pData->PrivateDriverDataSize == sizeof (VBOXWDDM_RCINFO));
            if (pData->PrivateDriverDataSize == sizeof (VBOXWDDM_RCINFO))
            {
                VBOXWDDM_RCINFO *pRcInfo = (VBOXWDDM_RCINFO*)pData->pPrivateDriverData;
                Assert(pRcInfo->fFlags.Generic);
                Assert(!pRcInfo->fFlags.Opened);
                Assert(pRcInfo->cAllocInfos == pData->NumAllocations);
                pRc->fFlags.Value |= pRcInfo->fFlags.Value;
                pRc->fFlags.Generic = 1;
                pRc->RcDesc = pRcInfo->RcDesc;
                pRc->cAllocations = pData->NumAllocations;
                Assert(pRc->RcDesc.fFlags.SharedResource);

                hr = VBoxD3DIfCreateForRc(pRc);
                if (!SUCCEEDED(hr))
                {
                    WARN(("VBoxD3DIfCreateForRc failed, hr %d", hr));
                }
            }
            else
                hr = E_INVALIDARG;
        }

        if (hr == S_OK)
            pData->hResource = pRc;
        else
            vboxResourceFree(pRc);
    }
    else
    {
        vboxVDbgPrintR((__FUNCTION__": vboxResourceAlloc failed for hDevice(0x%p), NumAllocations(%d)\n", hDevice, pData->NumAllocations));
        hr = E_OUTOFMEMORY;
    }

    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return hr;
}
static HRESULT APIENTRY vboxWddmDDevGetCaptureAllocationHandle(HANDLE hDevice, D3DDDIARG_GETCAPTUREALLOCATIONHANDLE* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDDevCaptureToSysMem(HANDLE hDevice, CONST D3DDDIARG_CAPTURETOSYSMEM* pData)
{
    RT_NOREF(pData);
    VBOXDISP_DDI_PROLOGUE_DEV(hDevice);
    vboxVDbgPrintF(("<== "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)hDevice;
    Assert(pDevice); NOREF(pDevice);
    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

    Assert(0);
    vboxVDbgPrintF(("==> "__FUNCTION__", hDevice(0x%p)\n", hDevice));
    return E_FAIL;
}

static HRESULT APIENTRY vboxWddmDispCreateDevice (IN HANDLE hAdapter, IN D3DDDIARG_CREATEDEVICE* pCreateData)
{
    VBOXDISP_DDI_PROLOGUE_ADP(hAdapter);
    HRESULT hr = S_OK;
    vboxVDbgPrint(("==> "__FUNCTION__", hAdapter(0x%p), Interface(%d), Version(%d)\n", hAdapter, pCreateData->Interface, pCreateData->Version));

//    Assert(0);
    PVBOXWDDMDISP_ADAPTER pAdapter = (PVBOXWDDMDISP_ADAPTER)hAdapter;

    PVBOXWDDMDISP_DEVICE pDevice = (PVBOXWDDMDISP_DEVICE)RTMemAllocZ(RT_OFFSETOF(VBOXWDDMDISP_DEVICE,
                                                                                 apRTs[pAdapter->D3D.cMaxSimRTs]));
    if (pDevice)
    {
        pDevice->cRTs = pAdapter->D3D.cMaxSimRTs;
        pDevice->hDevice = pCreateData->hDevice;
        pDevice->pAdapter = pAdapter;
        pDevice->u32IfVersion = pCreateData->Interface;
        pDevice->uRtVersion = pCreateData->Version;
        pDevice->RtCallbacks = *pCreateData->pCallbacks;
        pDevice->pvCmdBuffer = pCreateData->pCommandBuffer;
        pDevice->cbCmdBuffer = pCreateData->CommandBufferSize;
        pDevice->fFlags = pCreateData->Flags;
        /* Set Viewport to some default values */
        pDevice->ViewPort.X = 0;
        pDevice->ViewPort.Y = 0;
        pDevice->ViewPort.Width = 1;
        pDevice->ViewPort.Height = 1;
        pDevice->ViewPort.MinZ = 0.;
        pDevice->ViewPort.MaxZ = 1.;

        RTListInit(&pDevice->DirtyAllocList);

        Assert(!pCreateData->AllocationListSize);
        Assert(!pCreateData->PatchLocationListSize);

        pCreateData->hDevice = pDevice;

        pCreateData->pDeviceFuncs->pfnSetRenderState = vboxWddmDDevSetRenderState;
        pCreateData->pDeviceFuncs->pfnUpdateWInfo = vboxWddmDDevUpdateWInfo;
        pCreateData->pDeviceFuncs->pfnValidateDevice = vboxWddmDDevValidateDevice;
        pCreateData->pDeviceFuncs->pfnSetTextureStageState = vboxWddmDDevSetTextureStageState;
        pCreateData->pDeviceFuncs->pfnSetTexture = vboxWddmDDevSetTexture;
        pCreateData->pDeviceFuncs->pfnSetPixelShader = vboxWddmDDevSetPixelShader;
        pCreateData->pDeviceFuncs->pfnSetPixelShaderConst = vboxWddmDDevSetPixelShaderConst;
        pCreateData->pDeviceFuncs->pfnSetStreamSourceUm = vboxWddmDDevSetStreamSourceUm;
        pCreateData->pDeviceFuncs->pfnSetIndices = vboxWddmDDevSetIndices;
        pCreateData->pDeviceFuncs->pfnSetIndicesUm = vboxWddmDDevSetIndicesUm;
        pCreateData->pDeviceFuncs->pfnDrawPrimitive = vboxWddmDDevDrawPrimitive;
        pCreateData->pDeviceFuncs->pfnDrawIndexedPrimitive = vboxWddmDDevDrawIndexedPrimitive;
        pCreateData->pDeviceFuncs->pfnDrawRectPatch = vboxWddmDDevDrawRectPatch;
        pCreateData->pDeviceFuncs->pfnDrawTriPatch = vboxWddmDDevDrawTriPatch;
        pCreateData->pDeviceFuncs->pfnDrawPrimitive2 = vboxWddmDDevDrawPrimitive2;
        pCreateData->pDeviceFuncs->pfnDrawIndexedPrimitive2 = vboxWddmDDevDrawIndexedPrimitive2;
        pCreateData->pDeviceFuncs->pfnVolBlt = vboxWddmDDevVolBlt;
        pCreateData->pDeviceFuncs->pfnBufBlt = vboxWddmDDevBufBlt;
        pCreateData->pDeviceFuncs->pfnTexBlt = vboxWddmDDevTexBlt;
        pCreateData->pDeviceFuncs->pfnStateSet = vboxWddmDDevStateSet;
        pCreateData->pDeviceFuncs->pfnSetPriority = vboxWddmDDevSetPriority;
        pCreateData->pDeviceFuncs->pfnClear = vboxWddmDDevClear;
        pCreateData->pDeviceFuncs->pfnUpdatePalette = vboxWddmDDevUpdatePalette;
        pCreateData->pDeviceFuncs->pfnSetPalette = vboxWddmDDevSetPalette;
        pCreateData->pDeviceFuncs->pfnSetVertexShaderConst = vboxWddmDDevSetVertexShaderConst;
        pCreateData->pDeviceFuncs->pfnMultiplyTransform = vboxWddmDDevMultiplyTransform;
        pCreateData->pDeviceFuncs->pfnSetTransform = vboxWddmDDevSetTransform;
        pCreateData->pDeviceFuncs->pfnSetViewport = vboxWddmDDevSetViewport;
        pCreateData->pDeviceFuncs->pfnSetZRange = vboxWddmDDevSetZRange;
        pCreateData->pDeviceFuncs->pfnSetMaterial = vboxWddmDDevSetMaterial;
        pCreateData->pDeviceFuncs->pfnSetLight = vboxWddmDDevSetLight;
        pCreateData->pDeviceFuncs->pfnCreateLight = vboxWddmDDevCreateLight;
        pCreateData->pDeviceFuncs->pfnDestroyLight = vboxWddmDDevDestroyLight;
        pCreateData->pDeviceFuncs->pfnSetClipPlane = vboxWddmDDevSetClipPlane;
        pCreateData->pDeviceFuncs->pfnGetInfo = vboxWddmDDevGetInfo;
        pCreateData->pDeviceFuncs->pfnLock = vboxWddmDDevLock;
        pCreateData->pDeviceFuncs->pfnUnlock = vboxWddmDDevUnlock;
        pCreateData->pDeviceFuncs->pfnCreateResource = vboxWddmDDevCreateResource;
        pCreateData->pDeviceFuncs->pfnDestroyResource = vboxWddmDDevDestroyResource;
        pCreateData->pDeviceFuncs->pfnSetDisplayMode = vboxWddmDDevSetDisplayMode;
        pCreateData->pDeviceFuncs->pfnPresent = vboxWddmDDevPresent;
        pCreateData->pDeviceFuncs->pfnFlush = vboxWddmDDevFlush;
        pCreateData->pDeviceFuncs->pfnCreateVertexShaderFunc = vboxWddmDDevCreateVertexShaderFunc;
        pCreateData->pDeviceFuncs->pfnDeleteVertexShaderFunc = vboxWddmDDevDeleteVertexShaderFunc;
        pCreateData->pDeviceFuncs->pfnSetVertexShaderFunc = vboxWddmDDevSetVertexShaderFunc;
        pCreateData->pDeviceFuncs->pfnCreateVertexShaderDecl = vboxWddmDDevCreateVertexShaderDecl;
        pCreateData->pDeviceFuncs->pfnDeleteVertexShaderDecl = vboxWddmDDevDeleteVertexShaderDecl;
        pCreateData->pDeviceFuncs->pfnSetVertexShaderDecl = vboxWddmDDevSetVertexShaderDecl;
        pCreateData->pDeviceFuncs->pfnSetVertexShaderConstI = vboxWddmDDevSetVertexShaderConstI;
        pCreateData->pDeviceFuncs->pfnSetVertexShaderConstB = vboxWddmDDevSetVertexShaderConstB;
        pCreateData->pDeviceFuncs->pfnSetScissorRect = vboxWddmDDevSetScissorRect;
        pCreateData->pDeviceFuncs->pfnSetStreamSource = vboxWddmDDevSetStreamSource;
        pCreateData->pDeviceFuncs->pfnSetStreamSourceFreq = vboxWddmDDevSetStreamSourceFreq;
        pCreateData->pDeviceFuncs->pfnSetConvolutionKernelMono = vboxWddmDDevSetConvolutionKernelMono;
        pCreateData->pDeviceFuncs->pfnComposeRects = vboxWddmDDevComposeRects;
        pCreateData->pDeviceFuncs->pfnBlt = vboxWddmDDevBlt;
        pCreateData->pDeviceFuncs->pfnColorFill = vboxWddmDDevColorFill;
        pCreateData->pDeviceFuncs->pfnDepthFill = vboxWddmDDevDepthFill;
        pCreateData->pDeviceFuncs->pfnCreateQuery = vboxWddmDDevCreateQuery;
        pCreateData->pDeviceFuncs->pfnDestroyQuery = vboxWddmDDevDestroyQuery;
        pCreateData->pDeviceFuncs->pfnIssueQuery = vboxWddmDDevIssueQuery;
        pCreateData->pDeviceFuncs->pfnGetQueryData = vboxWddmDDevGetQueryData;
        pCreateData->pDeviceFuncs->pfnSetRenderTarget = vboxWddmDDevSetRenderTarget;
        pCreateData->pDeviceFuncs->pfnSetDepthStencil = vboxWddmDDevSetDepthStencil;
        pCreateData->pDeviceFuncs->pfnGenerateMipSubLevels = vboxWddmDDevGenerateMipSubLevels;
        pCreateData->pDeviceFuncs->pfnSetPixelShaderConstI = vboxWddmDDevSetPixelShaderConstI;
        pCreateData->pDeviceFuncs->pfnSetPixelShaderConstB = vboxWddmDDevSetPixelShaderConstB;
        pCreateData->pDeviceFuncs->pfnCreatePixelShader = vboxWddmDDevCreatePixelShader;
        pCreateData->pDeviceFuncs->pfnDeletePixelShader = vboxWddmDDevDeletePixelShader;
        pCreateData->pDeviceFuncs->pfnCreateDecodeDevice = vboxWddmDDevCreateDecodeDevice;
        pCreateData->pDeviceFuncs->pfnDestroyDecodeDevice = vboxWddmDDevDestroyDecodeDevice;
        pCreateData->pDeviceFuncs->pfnSetDecodeRenderTarget = vboxWddmDDevSetDecodeRenderTarget;
        pCreateData->pDeviceFuncs->pfnDecodeBeginFrame = vboxWddmDDevDecodeBeginFrame;
        pCreateData->pDeviceFuncs->pfnDecodeEndFrame = vboxWddmDDevDecodeEndFrame;
        pCreateData->pDeviceFuncs->pfnDecodeExecute = vboxWddmDDevDecodeExecute;
        pCreateData->pDeviceFuncs->pfnDecodeExtensionExecute = vboxWddmDDevDecodeExtensionExecute;
        pCreateData->pDeviceFuncs->pfnCreateVideoProcessDevice = vboxWddmDDevCreateVideoProcessDevice;
        pCreateData->pDeviceFuncs->pfnDestroyVideoProcessDevice = vboxWddmDDevDestroyVideoProcessDevice;
        pCreateData->pDeviceFuncs->pfnVideoProcessBeginFrame = vboxWddmDDevVideoProcessBeginFrame;
        pCreateData->pDeviceFuncs->pfnVideoProcessEndFrame = vboxWddmDDevVideoProcessEndFrame;
        pCreateData->pDeviceFuncs->pfnSetVideoProcessRenderTarget = vboxWddmDDevSetVideoProcessRenderTarget;
        pCreateData->pDeviceFuncs->pfnVideoProcessBlt = vboxWddmDDevVideoProcessBlt;
        pCreateData->pDeviceFuncs->pfnCreateExtensionDevice = vboxWddmDDevCreateExtensionDevice;
        pCreateData->pDeviceFuncs->pfnDestroyExtensionDevice = vboxWddmDDevDestroyExtensionDevice;
        pCreateData->pDeviceFuncs->pfnExtensionExecute = vboxWddmDDevExtensionExecute;
        pCreateData->pDeviceFuncs->pfnCreateOverlay = vboxWddmDDevCreateOverlay;
        pCreateData->pDeviceFuncs->pfnUpdateOverlay = vboxWddmDDevUpdateOverlay;
        pCreateData->pDeviceFuncs->pfnFlipOverlay = vboxWddmDDevFlipOverlay;
        pCreateData->pDeviceFuncs->pfnGetOverlayColorControls = vboxWddmDDevGetOverlayColorControls;
        pCreateData->pDeviceFuncs->pfnSetOverlayColorControls = vboxWddmDDevSetOverlayColorControls;
        pCreateData->pDeviceFuncs->pfnDestroyOverlay = vboxWddmDDevDestroyOverlay;
        pCreateData->pDeviceFuncs->pfnDestroyDevice = vboxWddmDDevDestroyDevice;
        pCreateData->pDeviceFuncs->pfnQueryResourceResidency = vboxWddmDDevQueryResourceResidency;
        pCreateData->pDeviceFuncs->pfnOpenResource = vboxWddmDDevOpenResource;
        pCreateData->pDeviceFuncs->pfnGetCaptureAllocationHandle = vboxWddmDDevGetCaptureAllocationHandle;
        pCreateData->pDeviceFuncs->pfnCaptureToSysMem = vboxWddmDDevCaptureToSysMem;
        pCreateData->pDeviceFuncs->pfnLockAsync = NULL; //vboxWddmDDevLockAsync;
        pCreateData->pDeviceFuncs->pfnUnlockAsync = NULL; //vboxWddmDDevUnlockAsync;
        pCreateData->pDeviceFuncs->pfnRename = NULL; //vboxWddmDDevRename;

        VBOXDISPPROFILE_DDI_INIT_DEV(pDevice);
#ifdef VBOX_WDDMDISP_WITH_PROFILE
        pDevice->ProfileDdiPresentCb = VBoxDispProfileSet("pfnPresentCb");
#endif

#ifdef VBOXWDDMDISP_DEBUG_TIMER
        pDevice->hTimerQueue = CreateTimerQueue();
        Assert(pDevice->hTimerQueue);
#endif

        do
        {
            RTListInit(&pDevice->SwapchainList);
            Assert(!pCreateData->AllocationListSize
                    && !pCreateData->PatchLocationListSize);
            if (!pCreateData->AllocationListSize
                    && !pCreateData->PatchLocationListSize)
            {
                {
                    VBOXDISPCRHGSMI_SCOPE_SET_DEV(pDevice);

                    hr = vboxDispCmCtxCreate(pDevice, &pDevice->DefaultContext);
                    Assert(hr == S_OK);
                    if (hr == S_OK)
                    {
#ifdef VBOXDISP_EARLYCREATEDEVICE
                        PVBOXWDDMDISP_RESOURCE pRc = vboxResourceAlloc(2);
                        Assert(pRc);
                        if (pRc)
                        {
                            D3DPRESENT_PARAMETERS params;
                            memset(&params, 0, sizeof (params));
    //                        params.BackBufferWidth = 640;
    //                        params.BackBufferHeight = 480;
                            params.BackBufferWidth = 0x400;
                            params.BackBufferHeight = 0x300;
                            params.BackBufferFormat = D3DFMT_A8R8G8B8;
    //                        params.BackBufferCount = 0;
                            params.BackBufferCount = 1;
                            params.MultiSampleType = D3DMULTISAMPLE_NONE;
                            params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        //                    params.hDeviceWindow = hWnd;
                                        /** @todo it seems there should be a way to detect this correctly since
                                         * our vboxWddmDDevSetDisplayMode will be called in case we are using full-screen */
                            params.Windowed = TRUE;
                            //            params.EnableAutoDepthStencil = FALSE;
                            //            params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
                            //            params.Flags;
                            //            params.FullScreen_RefreshRateInHz;
                            //            params.FullScreen_PresentationInterval;

                            hr = vboxWddmD3DDeviceCreate(pDevice, 0, pRc, &params, TRUE /*BOOL bLockable*/);
                            Assert(hr == S_OK);
                            if (hr == S_OK)
                                break;
                            vboxResourceFree(pRc);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }

                        HRESULT hr2 = vboxDispCmCtxDestroy(pDevice, &pDevice->DefaultContext);
                        Assert(hr2 == S_OK); NOREF(hr2);
#else
//# define VBOXDISP_TEST_SWAPCHAIN
# ifdef VBOXDISP_TEST_SWAPCHAIN
                        VBOXDISP_D3DEV(pDevice);
# endif
                        break;
#endif
                    }
                }
            }
            else
            {
                vboxVDbgPrintR((__FUNCTION__": Not implemented: PatchLocationListSize(%d), AllocationListSize(%d)\n",
                        pCreateData->PatchLocationListSize, pCreateData->AllocationListSize));
                //pCreateData->pAllocationList = ??
                hr = E_FAIL;
            }

            RTMemFree(pDevice);
        } while (0);
    }
    else
    {
        vboxVDbgPrintR((__FUNCTION__": RTMemAllocZ returned NULL\n"));
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        /* Get system D3D DLL handle and prevent its unloading until entire process termination (GET_MODULE_HANDLE_EX_FLAG_PIN).
         * This is important because even after guest App issued CloseAdatper() call, we still use pointers provided by DLL. */
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                                (LPCWSTR)pDevice->RtCallbacks.pfnAllocateCb,
                                &pDevice->hHgsmiTransportModule))
        {
            Assert(pDevice->hHgsmiTransportModule);
            vboxVDbgPrintR((__FUNCTION__": system D3D DLL referenced with GET_MODULE_HANDLE_EX_FLAG_PIN flag\n"));
        }
        else
        {
            DWORD winEr = GetLastError(); NOREF(winEr);
            WARN(("GetModuleHandleEx failed winEr %d, ignoring", winEr));
            pDevice->hHgsmiTransportModule = 0;
        }
    }

    vboxVDbgPrint(("<== "__FUNCTION__", hAdapter(0x%p)\n", hAdapter));

    return hr;
}

static HRESULT APIENTRY vboxWddmDispCloseAdapter (IN HANDLE hAdapter)
{
    VBOXDISP_DDI_PROLOGUE_ADP(hAdapter);
    vboxVDbgPrint(("==> "__FUNCTION__", hAdapter(0x%p)\n", hAdapter));

    PVBOXWDDMDISP_ADAPTER pAdapter = (PVBOXWDDMDISP_ADAPTER)hAdapter;
    if (VBOXDISPMODE_IS_3D(pAdapter))
    {
        VBOXDISPCRHGSMI_SCOPE_SET_GLOBAL();
        VBoxDispD3DGlobalClose(&pAdapter->D3D, &pAdapter->Formats);
    }
#ifdef VBOX_WITH_VIDEOHWACCEL
    else
    {
        VBoxDispD3DGlobal2DFormatsTerm(pAdapter);
    }
#endif

    VBOXDISPPROFILE_DDI_TERM(pAdapter);

    RTMemFree(pAdapter);

    vboxVDbgPrint(("<== "__FUNCTION__", hAdapter(0x%p)\n", hAdapter));

#ifdef DEBUG
    VbglR3Term();
#endif
    return S_OK;
}

#define VBOXDISP_IS_MODULE_FUNC(_pvModule, _cbModule, _pfn) ( \
           (((uintptr_t)(_pfn)) >= ((uintptr_t)(_pvModule))) \
        && (((uintptr_t)(_pfn)) < (((uintptr_t)(_pvModule)) + ((DWORD)(_cbModule)))) \
        )

static BOOL vboxDispIsDDraw(__inout D3DDDIARG_OPENADAPTER*  pOpenData)
{
    /*if we are loaded by ddraw module, the Interface version should be 7
     * and pAdapterCallbacks should be ddraw-supplied, i.e. reside in ddraw module */
    if (pOpenData->Interface != 7)
        return FALSE;

    HMODULE hDDraw = GetModuleHandleA("ddraw.dll");
    if (!hDDraw)
        return FALSE;

    HANDLE hProcess = GetCurrentProcess();
    MODULEINFO ModuleInfo = {0};

    if (!GetModuleInformation(hProcess, hDDraw, &ModuleInfo, sizeof (ModuleInfo)))
    {
        DWORD winEr = GetLastError(); NOREF(winEr);
        WARN(("GetModuleInformation failed, %d", winEr));
        return FALSE;
    }

    if (VBOXDISP_IS_MODULE_FUNC(ModuleInfo.lpBaseOfDll, ModuleInfo.SizeOfImage,
                                pOpenData->pAdapterCallbacks->pfnQueryAdapterInfoCb))
        return TRUE;
    if (VBOXDISP_IS_MODULE_FUNC(ModuleInfo.lpBaseOfDll, ModuleInfo.SizeOfImage,
                                pOpenData->pAdapterCallbacks->pfnGetMultisampleMethodListCb))
        return TRUE;

    return FALSE;
}

HRESULT APIENTRY OpenAdapter(__inout D3DDDIARG_OPENADAPTER*  pOpenData)
{
#ifdef DEBUG
    VbglR3Init();
#endif

    VBOXDISP_DDI_PROLOGUE_GLBL();

    vboxVDbgPrint(("==> "__FUNCTION__"\n"));

#if 0 //def DEBUG_misha
    DWORD dwVersion = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;
    dwVersion = GetVersion();
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    if (dwMajorVersion == 6 && dwMinorVersion <= 1 && VBOXVDBG_IS_DWM())
    {
        exit(0);
        return E_FAIL;
    }
#endif

//    vboxDispLock();

    HRESULT hr = E_FAIL;

    do
    {

    LOGREL(("Built %s %s", __DATE__, __TIME__));

    VBOXWDDM_QI Query;
    D3DDDICB_QUERYADAPTERINFO DdiQuery;
    DdiQuery.PrivateDriverDataSize = sizeof(Query);
    DdiQuery.pPrivateDriverData = &Query;
    hr = pOpenData->pAdapterCallbacks->pfnQueryAdapterInfoCb(pOpenData->hAdapter, &DdiQuery);
    Assert(hr == S_OK);
    if (hr != S_OK)
    {
        vboxVDbgPrintR((__FUNCTION__": pfnQueryAdapterInfoCb failed, hr (%d)\n", hr));
        hr = E_FAIL;
        break;
    }

    /* check the miniport version match display version */
    if (Query.u32Version != VBOXVIDEOIF_VERSION)
    {
        vboxVDbgPrintR((__FUNCTION__": miniport version mismatch, expected (%d), but was (%d)\n",
                VBOXVIDEOIF_VERSION,
                Query.u32Version));
        hr = E_FAIL;
        break;
    }

#ifdef VBOX_WITH_VIDEOHWACCEL
    Assert(Query.cInfos >= 1);
    PVBOXWDDMDISP_ADAPTER pAdapter = (PVBOXWDDMDISP_ADAPTER)RTMemAllocZ(RT_OFFSETOF(VBOXWDDMDISP_ADAPTER, aHeads[Query.cInfos]));
#else
    PVBOXWDDMDISP_ADAPTER pAdapter = (PVBOXWDDMDISP_ADAPTER)RTMemAllocZ(sizeof (VBOXWDDMDISP_ADAPTER));
#endif
    Assert(pAdapter);
    if (pAdapter)
    {
        pAdapter->hAdapter = pOpenData->hAdapter;
        pAdapter->uIfVersion = pOpenData->Interface;
        pAdapter->uRtVersion= pOpenData->Version;
        pAdapter->RtCallbacks = *pOpenData->pAdapterCallbacks;

        pAdapter->u32VBox3DCaps = Query.u32VBox3DCaps;

        pAdapter->cHeads = Query.cInfos;

        pOpenData->hAdapter = pAdapter;
        pOpenData->pAdapterFuncs->pfnGetCaps = vboxWddmDispGetCaps;
        pOpenData->pAdapterFuncs->pfnCreateDevice = vboxWddmDispCreateDevice;
        pOpenData->pAdapterFuncs->pfnCloseAdapter = vboxWddmDispCloseAdapter;
        pOpenData->DriverVersion = D3D_UMD_INTERFACE_VERSION;
        if (!vboxDispIsDDraw(pOpenData))
        {
            do
            {
                {
                    VBOXDISPCRHGSMI_SCOPE_SET_GLOBAL();
                    /* try enable the 3D */
                    hr = VBoxDispD3DGlobalOpen(&pAdapter->D3D, &pAdapter->Formats);
                    if (hr == S_OK)
                    {
                        LOG(("SUCCESS 3D Enabled, pAdapter (0x%p)", pAdapter));
                        break;
                    }
                    else
                        WARN(("VBoxDispD3DOpen failed, hr (%d)", hr));

                }
            } while (0);
        }
#ifdef VBOX_WITH_VIDEOHWACCEL
        if (!VBOXDISPMODE_IS_3D(pAdapter))
        {
            for (uint32_t i = 0; i < pAdapter->cHeads; ++i)
            {
                pAdapter->aHeads[i].Vhwa.Settings = Query.aInfos[i];
            }
            hr = VBoxDispD3DGlobal2DFormatsInit(pAdapter);
            if (!SUCCEEDED(hr))
            {
                WARN(("VBoxDispD3DGlobal2DFormatsInit failed hr 0x%x", hr));
            }
        }
#endif

        if (SUCCEEDED(hr))
        {
            VBOXDISPPROFILE_DDI_INIT_ADP(pAdapter);
            hr = S_OK;
            break;
        }
        else
        {
            WARN(("OpenAdapter failed hr 0x%x", hr));
        }

        RTMemFree(pAdapter);
    }
    else
    {
        vboxVDbgPrintR((__FUNCTION__": RTMemAllocZ returned NULL\n"));
        hr = E_OUTOFMEMORY;
    }

    } while (0);

//    vboxDispUnlock();

    vboxVDbgPrint(("<== "__FUNCTION__", hr (%d)\n", hr));

    return hr;
}

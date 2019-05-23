/* $Id: thread-win.cpp $ */
/** @file
 * IPRT - Threads, Windows.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_THREAD
#include <iprt/nt/nt-and-windows.h>

#include <errno.h>
#include <process.h>

#include <iprt/thread.h>
#include "internal/iprt.h"

#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#include <iprt/cpuset.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/mem.h>
#include "internal/thread.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The TLS index allocated for storing the RTTHREADINT pointer. */
static DWORD g_dwSelfTLS = TLS_OUT_OF_INDEXES;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static unsigned __stdcall rtThreadNativeMain(void *pvArgs);
static void rtThreadWinTellDebuggerThreadName(uint32_t idThread, const char *pszName);


DECLHIDDEN(int) rtThreadNativeInit(void)
{
    g_dwSelfTLS = TlsAlloc();
    if (g_dwSelfTLS == TLS_OUT_OF_INDEXES)
        return VERR_NO_TLS_FOR_SELF;
    return VINF_SUCCESS;
}


DECLHIDDEN(void) rtThreadNativeReInitObtrusive(void)
{
    /* nothing to do here. */
}


DECLHIDDEN(void) rtThreadNativeDetach(void)
{
    /*
     * Deal with alien threads.
     */
    PRTTHREADINT pThread = (PRTTHREADINT)TlsGetValue(g_dwSelfTLS);
    if (    pThread
        &&  (pThread->fIntFlags & RTTHREADINT_FLAGS_ALIEN))
    {
        rtThreadTerminate(pThread, 0);
        TlsSetValue(g_dwSelfTLS, NULL);
    }
}


DECLHIDDEN(void) rtThreadNativeDestroy(PRTTHREADINT pThread)
{
    if (pThread == (PRTTHREADINT)TlsGetValue(g_dwSelfTLS))
        TlsSetValue(g_dwSelfTLS, NULL);

    if ((HANDLE)pThread->hThread != INVALID_HANDLE_VALUE)
    {
        CloseHandle((HANDLE)pThread->hThread);
        pThread->hThread = (uintptr_t)INVALID_HANDLE_VALUE;
    }
}


DECLHIDDEN(int) rtThreadNativeAdopt(PRTTHREADINT pThread)
{
    if (!TlsSetValue(g_dwSelfTLS, pThread))
        return VERR_FAILED_TO_SET_SELF_TLS;
    if (IsDebuggerPresent())
        rtThreadWinTellDebuggerThreadName(GetCurrentThreadId(), pThread->szName);
    return VINF_SUCCESS;
}


DECLHIDDEN(void) rtThreadNativeInformDebugger(PRTTHREADINT pThread)
{
    rtThreadWinTellDebuggerThreadName((uint32_t)(uintptr_t)pThread->Core.Key, pThread->szName);
}


/**
 * Communicates the thread name to the debugger, if we're begin debugged that
 * is.
 *
 * See http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx for debugger
 * interface details.
 *
 * @param   idThread        The thread ID. UINT32_MAX for current thread.
 * @param   pszName         The name.
 */
static void rtThreadWinTellDebuggerThreadName(uint32_t idThread, const char *pszName)
{
    struct
    {
        uint32_t    uType;
        const char *pszName;
        uint32_t    idThread;
        uint32_t    fFlags;
    } Pkg = { 0x1000, pszName, idThread, 0 };
    __try
    {
        RaiseException(0x406d1388, 0, sizeof(Pkg)/sizeof(ULONG_PTR), (ULONG_PTR *)&Pkg);
    }
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {

    }
}


/**
 * Bitch about dangling COM and OLE references, dispose of them
 * afterwards so we don't end up deadlocked somewhere below
 * OLE32!DllMain.
 */
static void rtThreadNativeUninitComAndOle(void)
{
#if 1 /* experimental code */
    /*
     * Read the counters.
     */
    struct MySOleTlsData
    {
        void       *apvReserved0[2];    /**< x86=0x00  W7/64=0x00 */
        DWORD       adwReserved0[3];    /**< x86=0x08  W7/64=0x10 */
        void       *apvReserved1[1];    /**< x86=0x14  W7/64=0x20 */
        DWORD       cComInits;          /**< x86=0x18  W7/64=0x28 */
        DWORD       cOleInits;          /**< x86=0x1c  W7/64=0x2c */
        DWORD       dwReserved1;        /**< x86=0x20  W7/64=0x30 */
        void       *apvReserved2[4];    /**< x86=0x24  W7/64=0x38 */
        DWORD       adwReserved2[1];    /**< x86=0x34  W7/64=0x58 */
        void       *pvCurrentCtx;       /**< x86=0x38  W7/64=0x60 */
        IUnknown   *pCallState;         /**< x86=0x3c  W7/64=0x68 */
    }      *pOleTlsData = NULL;         /* outside the try/except for debugging */
    DWORD   cComInits   = 0;
    DWORD   cOleInits   = 0;
    __try
    {
        void *pvTeb = NtCurrentTeb();
# ifdef RT_ARCH_AMD64
        pOleTlsData = *(struct MySOleTlsData **)((uintptr_t)pvTeb + 0x1758); /*TEB.ReservedForOle*/
# elif RT_ARCH_X86
        pOleTlsData = *(struct MySOleTlsData **)((uintptr_t)pvTeb + 0x0f80); /*TEB.ReservedForOle*/
# else
#  error "Port me!"
# endif
        if (pOleTlsData)
        {
            cComInits = pOleTlsData->cComInits;
            cOleInits = pOleTlsData->cOleInits;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        AssertFailedReturnVoid();
    }

    /*
     * Assert sanity. If any of these breaks, the structure layout above is
     * probably not correct any longer.
     */
    AssertMsgReturnVoid(cComInits < 1000, ("%u (%#x)\n", cComInits, cComInits));
    AssertMsgReturnVoid(cOleInits < 1000, ("%u (%#x)\n", cOleInits, cOleInits));
    AssertMsgReturnVoid(cComInits >= cOleInits, ("cComInits=%#x cOleInits=%#x\n", cComInits, cOleInits));

    /*
     * Do the uninitializing.
     */
    if (cComInits)
    {
        AssertMsgFailed(("cComInits=%u (%#x) cOleInits=%u (%#x) - dangling COM/OLE inits!\n",
                         cComInits, cComInits, cOleInits, cOleInits));

        HMODULE hOle32 = GetModuleHandle("ole32.dll");
        AssertReturnVoid(hOle32 != NULL);

        typedef void (WINAPI *PFNOLEUNINITIALIZE)(void);
        PFNOLEUNINITIALIZE  pfnOleUninitialize = (PFNOLEUNINITIALIZE)GetProcAddress(hOle32, "OleUninitialize");
        AssertReturnVoid(pfnOleUninitialize);

        typedef void (WINAPI *PFNCOUNINITIALIZE)(void);
        PFNCOUNINITIALIZE   pfnCoUninitialize  = (PFNCOUNINITIALIZE)GetProcAddress(hOle32, "CoUninitialize");
        AssertReturnVoid(pfnCoUninitialize);

        while (cOleInits-- > 0)
        {
            pfnOleUninitialize();
            cComInits--;
        }

        while (cComInits-- > 0)
            pfnCoUninitialize();
    }
#endif
}


/**
 * Wrapper which unpacks the param stuff and calls thread function.
 */
static unsigned __stdcall rtThreadNativeMain(void *pvArgs)
{
    DWORD           dwThreadId = GetCurrentThreadId();
    PRTTHREADINT    pThread = (PRTTHREADINT)pvArgs;

    if (!TlsSetValue(g_dwSelfTLS, pThread))
        AssertReleaseMsgFailed(("failed to set self TLS. lasterr=%d thread '%s'\n", GetLastError(), pThread->szName));
    if (IsDebuggerPresent())
        rtThreadWinTellDebuggerThreadName(dwThreadId, &pThread->szName[0]);

    int rc = rtThreadMain(pThread, dwThreadId, &pThread->szName[0]);

    TlsSetValue(g_dwSelfTLS, NULL);
    rtThreadNativeUninitComAndOle();
    _endthreadex(rc);
    return rc;
}


DECLHIDDEN(int) rtThreadNativeCreate(PRTTHREADINT pThread, PRTNATIVETHREAD pNativeThread)
{
    AssertReturn(pThread->cbStack < ~(unsigned)0, VERR_INVALID_PARAMETER);

    /*
     * Create the thread.
     */
    pThread->hThread = (uintptr_t)INVALID_HANDLE_VALUE;
    unsigned    uThreadId = 0;
    uintptr_t   hThread = _beginthreadex(NULL, (unsigned)pThread->cbStack, rtThreadNativeMain, pThread, 0, &uThreadId);
    if (hThread != 0 && hThread != ~0U)
    {
        pThread->hThread = hThread;
        *pNativeThread = uThreadId;
        return VINF_SUCCESS;
    }
    return RTErrConvertFromErrno(errno);
}


DECLHIDDEN(bool) rtThreadNativeIsAliveKludge(PRTTHREADINT pThread)
{
    PPEB_COMMON pPeb = NtCurrentPeb();
    if (!pPeb || !pPeb->Ldr || !pPeb->Ldr->ShutdownInProgress)
        return true;
    DWORD rcWait = WaitForSingleObject((HANDLE)pThread->hThread, 0);
    return rcWait != WAIT_OBJECT_0;
}


RTDECL(RTTHREAD) RTThreadSelf(void)
{
    PRTTHREADINT pThread = (PRTTHREADINT)TlsGetValue(g_dwSelfTLS);
    /** @todo import alien threads ? */
    return pThread;
}


#if 0 /* noone is using this ... */
/**
 * Returns the processor number the current thread was running on during this call
 *
 * @returns processor nr
 */
static int rtThreadGetCurrentProcessorNumber(void)
{
    static bool           fInitialized = false;
    static DWORD (WINAPI *pfnGetCurrentProcessorNumber)(void) = NULL;
    if (!fInitialized)
    {
        HMODULE hmodKernel32 = GetModuleHandle("kernel32.dll");
        if (hmodKernel32)
            pfnGetCurrentProcessorNumber = (DWORD (WINAPI*)(void))GetProcAddress(hmodKernel32, "GetCurrentProcessorNumber");
        fInitialized = true;
    }
    if (pfnGetCurrentProcessorNumber)
        return pfnGetCurrentProcessorNumber();
    return -1;
}
#endif


RTR3DECL(int) RTThreadSetAffinity(PCRTCPUSET pCpuSet)
{
    DWORD_PTR fNewMask = pCpuSet ? RTCpuSetToU64(pCpuSet) : ~(DWORD_PTR)0;
    DWORD_PTR dwRet = SetThreadAffinityMask(GetCurrentThread(), fNewMask);
    if (dwRet)
        return VINF_SUCCESS;

    int iLastError = GetLastError();
    AssertMsgFailed(("SetThreadAffinityMask failed, LastError=%d\n", iLastError));
    return RTErrConvertFromWin32(iLastError);
}


RTR3DECL(int) RTThreadGetAffinity(PRTCPUSET pCpuSet)
{
    /*
     * Haven't found no query api, but the set api returns the old mask, so let's use that.
     */
    DWORD_PTR dwIgnored;
    DWORD_PTR dwProcAff = 0;
    if (GetProcessAffinityMask(GetCurrentProcess(), &dwProcAff, &dwIgnored))
    {
        HANDLE hThread = GetCurrentThread();
        DWORD_PTR dwRet = SetThreadAffinityMask(hThread, dwProcAff);
        if (dwRet)
        {
            DWORD_PTR dwSet = SetThreadAffinityMask(hThread, dwRet);
            Assert(dwSet == dwProcAff); NOREF(dwRet);

            RTCpuSetFromU64(pCpuSet, (uint64_t)dwSet);
            return VINF_SUCCESS;
        }
    }

    int iLastError = GetLastError();
    AssertMsgFailed(("SetThreadAffinityMask or GetProcessAffinityMask failed, LastError=%d\n", iLastError));
    return RTErrConvertFromWin32(iLastError);
}


RTR3DECL(int) RTThreadGetExecutionTimeMilli(uint64_t *pKernelTime, uint64_t *pUserTime)
{
    uint64_t u64CreationTime, u64ExitTime, u64KernelTime, u64UserTime;

    if (GetThreadTimes(GetCurrentThread(), (LPFILETIME)&u64CreationTime, (LPFILETIME)&u64ExitTime, (LPFILETIME)&u64KernelTime, (LPFILETIME)&u64UserTime))
    {
        *pKernelTime = u64KernelTime / 10000;    /* GetThreadTimes returns time in 100 ns units */
        *pUserTime   = u64UserTime / 10000;    /* GetThreadTimes returns time in 100 ns units */
        return VINF_SUCCESS;
    }

    int iLastError = GetLastError();
    AssertMsgFailed(("GetThreadTimes failed, LastError=%d\n", iLastError));
    return RTErrConvertFromWin32(iLastError);
}


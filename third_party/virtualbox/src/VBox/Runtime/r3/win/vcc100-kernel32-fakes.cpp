/* $Id: vcc100-kernel32-fakes.cpp $ */
/** @file
 * IPRT - Tricks to make the Visual C++ 2010 CRT work on NT4, W2K and XP.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/asm.h>
#include <iprt/string.h>

#ifndef RT_ARCH_X86
# error "This code is X86 only"
#endif

#define DecodePointer                           Ignore_DecodePointer
#define EncodePointer                           Ignore_EncodePointer
#define InitializeCriticalSectionAndSpinCount   Ignore_InitializeCriticalSectionAndSpinCount
#define HeapSetInformation                      Ignore_HeapSetInformation
#define HeapQueryInformation                    Ignore_HeapQueryInformation
#define CreateTimerQueue                        Ignore_CreateTimerQueue
#define CreateTimerQueueTimer                   Ignore_CreateTimerQueueTimer
#define DeleteTimerQueueTimer                   Ignore_DeleteTimerQueueTimer
#define InitializeSListHead                     Ignore_InitializeSListHead
#define InterlockedFlushSList                   Ignore_InterlockedFlushSList
#define InterlockedPopEntrySList                Ignore_InterlockedPopEntrySList
#define InterlockedPushEntrySList               Ignore_InterlockedPushEntrySList
#define QueryDepthSList                         Ignore_QueryDepthSList
#define VerifyVersionInfoA                      Ignore_VerifyVersionInfoA
#define VerSetConditionMask                     Ignore_VerSetConditionMask

#include <iprt/win/windows.h>

#undef DecodePointer
#undef EncodePointer
#undef InitializeCriticalSectionAndSpinCount
#undef HeapSetInformation
#undef HeapQueryInformation
#undef CreateTimerQueue
#undef CreateTimerQueueTimer
#undef DeleteTimerQueueTimer
#undef InitializeSListHead
#undef InterlockedFlushSList
#undef InterlockedPopEntrySList
#undef InterlockedPushEntrySList
#undef QueryDepthSList
#undef VerifyVersionInfoA
#undef VerSetConditionMask


#ifndef HEAP_STANDARD
# define HEAP_STANDARD 0
#endif


/** Dynamically resolves an kernel32 API.   */
#define RESOLVE_ME(ApiNm) \
    static bool volatile    s_fInitialized = false; \
    static decltype(ApiNm) *s_pfnApi = NULL; \
    decltype(ApiNm)        *pfnApi; \
    if (!s_fInitialized) \
        pfnApi = s_pfnApi; \
    else \
    { \
        pfnApi = (decltype(pfnApi))GetProcAddress(GetModuleHandleW(L"kernel32"), #ApiNm); \
        s_pfnApi = pfnApi; \
        s_fInitialized = true; \
    } do {} while (0) \


extern "C"
__declspec(dllexport) PVOID WINAPI
DecodePointer(PVOID pvEncoded)
{
    RESOLVE_ME(DecodePointer);
    if (pfnApi)
        return pfnApi(pvEncoded);

    /*
     * Fallback code.
     */
    return pvEncoded;
}


extern "C"
__declspec(dllexport) PVOID WINAPI
EncodePointer(PVOID pvNative)
{
    RESOLVE_ME(EncodePointer);
    if (pfnApi)
        return pfnApi(pvNative);

    /*
     * Fallback code.
     */
    return pvNative;
}


extern "C"
__declspec(dllexport) BOOL WINAPI
InitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION pCritSect, DWORD cSpin)
{
    RESOLVE_ME(InitializeCriticalSectionAndSpinCount);
    if (pfnApi)
        return pfnApi(pCritSect, cSpin);

    /*
     * Fallback code.
     */
    InitializeCriticalSection(pCritSect);
    return TRUE;
}


extern "C"
__declspec(dllexport) BOOL WINAPI
HeapSetInformation(HANDLE hHeap, HEAP_INFORMATION_CLASS enmInfoClass, PVOID pvBuf, SIZE_T cbBuf)
{
    RESOLVE_ME(HeapSetInformation);
    if (pfnApi)
        return pfnApi(hHeap, enmInfoClass, pvBuf, cbBuf);

    /*
     * Fallback code.
     */
    if (enmInfoClass == HeapCompatibilityInformation)
    {
        if (   cbBuf != sizeof(ULONG)
            || !pvBuf
            || *(PULONG)pvBuf == HEAP_STANDARD
           )
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        return TRUE;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


extern "C"
__declspec(dllexport) BOOL WINAPI
HeapQueryInformation(HANDLE hHeap, HEAP_INFORMATION_CLASS enmInfoClass, PVOID pvBuf, SIZE_T cbBuf, PSIZE_T pcbRet)
{
    RESOLVE_ME(HeapQueryInformation);
    if (pfnApi)
        return pfnApi(hHeap, enmInfoClass, pvBuf, cbBuf, pcbRet);

    /*
     * Fallback code.
     */
    if (enmInfoClass == HeapCompatibilityInformation)
    {
        *pcbRet = sizeof(ULONG);
        if (cbBuf < sizeof(ULONG) || !pvBuf)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        *(PULONG)pvBuf = HEAP_STANDARD;
        return TRUE;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/* These are used by INTEL\mt_obj\Timer.obj: */

extern "C"
__declspec(dllexport)
HANDLE WINAPI CreateTimerQueue(void)
{
    RESOLVE_ME(CreateTimerQueue);
    if (pfnApi)
        return pfnApi();
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

extern "C"
__declspec(dllexport)
BOOL WINAPI CreateTimerQueueTimer(PHANDLE phTimer, HANDLE hTimerQueue, WAITORTIMERCALLBACK pfnCallback, PVOID pvUser,
                                  DWORD msDueTime, DWORD msPeriod, ULONG fFlags)
{
    RESOLVE_ME(CreateTimerQueueTimer);
    if (pfnApi)
        return pfnApi(phTimer, hTimerQueue, pfnCallback, pvUser, msDueTime, msPeriod, fFlags);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C"
__declspec(dllexport)
BOOL WINAPI DeleteTimerQueueTimer(HANDLE hTimerQueue, HANDLE hTimer, HANDLE hEvtCompletion)
{
    RESOLVE_ME(DeleteTimerQueueTimer);
    if (pfnApi)
        return pfnApi(hTimerQueue, hTimer, hEvtCompletion);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/* This is used by several APIs. */

extern "C"
__declspec(dllexport)
VOID WINAPI InitializeSListHead(PSLIST_HEADER pHead)
{
    RESOLVE_ME(InitializeSListHead);
    if (pfnApi)
        pfnApi(pHead);
    else /* fallback: */
        pHead->Alignment = 0;
}


extern "C"
__declspec(dllexport)
PSLIST_ENTRY WINAPI InterlockedFlushSList(PSLIST_HEADER pHead)
{
    RESOLVE_ME(InterlockedFlushSList);
    if (pfnApi)
        return pfnApi(pHead);

    /* fallback: */
    PSLIST_ENTRY pRet = NULL;
    if (pHead->Next.Next)
    {
        for (;;)
        {
            SLIST_HEADER OldHead = *pHead;
            SLIST_HEADER NewHead;
            NewHead.Alignment = 0;
            NewHead.Sequence  = OldHead.Sequence + 1;
            if (ASMAtomicCmpXchgU64(&pHead->Alignment, NewHead.Alignment, OldHead.Alignment))
            {
                pRet = OldHead.Next.Next;
                break;
            }
        }
    }
    return pRet;
}

extern "C"
__declspec(dllexport)
PSLIST_ENTRY WINAPI InterlockedPopEntrySList(PSLIST_HEADER pHead)
{
    RESOLVE_ME(InterlockedPopEntrySList);
    if (pfnApi)
        return pfnApi(pHead);

    /* fallback: */
    PSLIST_ENTRY pRet = NULL;
    for (;;)
    {
        SLIST_HEADER OldHead = *pHead;
        pRet = OldHead.Next.Next;
        if (pRet)
        {
            SLIST_HEADER NewHead;
            __try
            {
                NewHead.Next.Next = pRet->Next;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                continue;
            }
            NewHead.Depth     = OldHead.Depth - 1;
            NewHead.Sequence  = OldHead.Sequence + 1;
            if (ASMAtomicCmpXchgU64(&pHead->Alignment, NewHead.Alignment, OldHead.Alignment))
                break;
        }
        else
            break;
    }
    return pRet;
}

extern "C"
__declspec(dllexport)
PSLIST_ENTRY WINAPI InterlockedPushEntrySList(PSLIST_HEADER pHead, PSLIST_ENTRY pEntry)
{
    RESOLVE_ME(InterlockedPushEntrySList);
    if (pfnApi)
        return pfnApi(pHead, pEntry);

    /* fallback: */
    PSLIST_ENTRY pRet = NULL;
    for (;;)
    {
        SLIST_HEADER OldHead = *pHead;
        pRet = OldHead.Next.Next;
        pEntry->Next = pRet;
        SLIST_HEADER NewHead;
        NewHead.Next.Next = pEntry;
        NewHead.Depth     = OldHead.Depth + 1;
        NewHead.Sequence  = OldHead.Sequence + 1;
        if (ASMAtomicCmpXchgU64(&pHead->Alignment, NewHead.Alignment, OldHead.Alignment))
            break;
    }
    return pRet;
}

extern "C"
__declspec(dllexport)
WORD WINAPI QueryDepthSList(PSLIST_HEADER pHead)
{
    RESOLVE_ME(QueryDepthSList);
    if (pfnApi)
        return pfnApi(pHead);
    return pHead->Depth;
}


/* curl drags these in: */
extern "C"
__declspec(dllexport)
BOOL WINAPI VerifyVersionInfoA(LPOSVERSIONINFOEXA pInfo, DWORD fTypeMask, DWORDLONG fConditionMask)
{
    RESOLVE_ME(VerifyVersionInfoA);
    if (pfnApi)
        return pfnApi(pInfo, fTypeMask, fConditionMask);

    /* fallback to make curl happy: */
    OSVERSIONINFOEXA VerInfo;
    RT_ZERO(VerInfo);
    VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);
    if (!GetVersionEx((OSVERSIONINFO *)&VerInfo))
    {
        RT_ZERO(VerInfo);
        VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        AssertReturn(GetVersionEx((OSVERSIONINFO *)&VerInfo), FALSE);
    }

    BOOL fRet = TRUE;
    for (unsigned i = 0; i < 8 && fRet; i++)
        if (fTypeMask & RT_BIT_32(i))
        {
            uint32_t uLeft, uRight;
            switch (RT_BIT_32(i))
            {
#define MY_CASE(a_Num, a_Member) case a_Num: uLeft = VerInfo.a_Member; uRight = pInfo->a_Member; break
                MY_CASE(VER_MINORVERSION,       dwMinorVersion);
                MY_CASE(VER_MAJORVERSION,       dwMajorVersion);
                MY_CASE(VER_BUILDNUMBER,        dwBuildNumber);
                MY_CASE(VER_PLATFORMID,         dwPlatformId);
                MY_CASE(VER_SERVICEPACKMINOR,   wServicePackMinor);
                MY_CASE(VER_SERVICEPACKMAJOR,   wServicePackMinor);
                MY_CASE(VER_SUITENAME,          wSuiteMask);
                MY_CASE(VER_PRODUCT_TYPE,       wProductType);
#undef  MY_CASE
                default: uLeft = uRight = 0; AssertFailed();
            }
            switch ((uint8_t)(fConditionMask >> (i*8)))
            {
                case VER_EQUAL:             fRet = uLeft == uRight; break;
                case VER_GREATER:           fRet = uLeft >  uRight; break;
                case VER_GREATER_EQUAL:     fRet = uLeft >= uRight; break;
                case VER_LESS:              fRet = uLeft <  uRight; break;
                case VER_LESS_EQUAL:        fRet = uLeft <= uRight; break;
                case VER_AND:               fRet = (uLeft & uRight) == uRight; break;
                case VER_OR:                fRet = (uLeft & uRight) != 0; break;
                default:                    fRet = FALSE; AssertFailed(); break;
            }
        }

    return fRet;
}

extern "C"
__declspec(dllexport)
ULONGLONG WINAPI VerSetConditionMask(ULONGLONG fConditionMask, DWORD fTypeMask, BYTE bOperator)
{
    RESOLVE_ME(VerSetConditionMask);
    if (pfnApi)
        return pfnApi(fConditionMask, fTypeMask, bOperator);

    /* fallback: */
    for (unsigned i = 0; i < 8; i++)
        if (fTypeMask & RT_BIT_32(i))
        {
            uint64_t fMask  = UINT64_C(0xff) << (i*8);
            fConditionMask &= ~fMask;
            fConditionMask |= (uint64_t)bOperator << (i*8);

        }
    return fConditionMask;
}



/* Dummy to force dragging in this object in the link, so the linker
   won't accidentally use the symbols from kernel32. */
extern "C" int vcc100_kernel32_fakes_cpp(void)
{
    return 42;
}


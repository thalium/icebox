/* $Id: init-win.cpp $ */
/** @file
 * IPRT - Init Ring-3, Windows Specific Code.
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
#define LOG_GROUP RTLOGGROUP_DEFAULT
#include <iprt/nt/nt-and-windows.h>
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
# define LOAD_LIBRARY_SEARCH_APPLICATION_DIR    0x200
# define LOAD_LIBRARY_SEARCH_SYSTEM32           0x800
#endif

#include "internal-r3-win.h"
#include <iprt/initterm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/param.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include "../init.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef VOID (WINAPI *PFNGETCURRENTTHREADSTACKLIMITS)(PULONG_PTR puLow, PULONG_PTR puHigh);
typedef LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI * PFNSETUNHANDLEDEXCEPTIONFILTER)(LPTOP_LEVEL_EXCEPTION_FILTER);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Windows DLL loader protection level. */
DECLHIDDEN(RTR3WINLDRPROT)      g_enmWinLdrProt = RTR3WINLDRPROT_NONE;
/** Our simplified windows version.    */
DECLHIDDEN(RTWINOSTYPE)         g_enmWinVer = kRTWinOSType_UNKNOWN;
/** Extended windows version information. */
DECLHIDDEN(OSVERSIONINFOEXW)    g_WinOsInfoEx;
/** The native kernel32.dll handle. */
DECLHIDDEN(HMODULE)             g_hModKernel32 = NULL;
/** The native ntdll.dll handle. */
DECLHIDDEN(HMODULE)             g_hModNtDll = NULL;
/** GetSystemWindowsDirectoryW or GetWindowsDirectoryW (NT4). */
DECLHIDDEN(PFNGETWINSYSDIR)     g_pfnGetSystemWindowsDirectoryW = NULL;
/** The GetCurrentThreadStackLimits API. */
static PFNGETCURRENTTHREADSTACKLIMITS g_pfnGetCurrentThreadStackLimits = NULL;
/** SetUnhandledExceptionFilter. */
static PFNSETUNHANDLEDEXCEPTIONFILTER g_pfnSetUnhandledExceptionFilter = NULL;
/** The previous unhandled exception filter. */
static LPTOP_LEVEL_EXCEPTION_FILTER   g_pfnUnhandledXcptFilter = NULL;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static LONG CALLBACK rtR3WinUnhandledXcptFilter(PEXCEPTION_POINTERS);


/**
 * Translates OSVERSIONINOFEX into a Windows OS type.
 *
 * @returns The Windows OS type.
 * @param   pOSInfoEx       The OS info returned by Windows.
 *
 * @remarks This table has been assembled from Usenet postings, personal
 *          observations, and reading other people's code.  Please feel
 *          free to add to it or correct it.
 * <pre>
         dwPlatFormID  dwMajorVersion  dwMinorVersion  dwBuildNumber
95             1              4               0             950
95 SP1         1              4               0        >950 && <=1080
95 OSR2        1              4             <10           >1080
98             1              4              10            1998
98 SP1         1              4              10       >1998 && <2183
98 SE          1              4              10          >=2183
ME             1              4              90            3000

NT 3.51        2              3              51            1057
NT 4           2              4               0            1381
2000           2              5               0            2195
XP             2              5               1            2600
2003           2              5               2            3790
Vista          2              6               0

CE 1.0         3              1               0
CE 2.0         3              2               0
CE 2.1         3              2               1
CE 3.0         3              3               0
</pre>
 */
static RTWINOSTYPE rtR3InitWinSimplifiedVersion(OSVERSIONINFOEXW const *pOSInfoEx)
{
    RTWINOSTYPE enmVer         = kRTWinOSType_UNKNOWN;
    BYTE  const bProductType   = pOSInfoEx->wProductType;
    DWORD const dwPlatformId   = pOSInfoEx->dwPlatformId;
    DWORD const dwMinorVersion = pOSInfoEx->dwMinorVersion;
    DWORD const dwMajorVersion = pOSInfoEx->dwMajorVersion;
    DWORD const dwBuildNumber  = pOSInfoEx->dwBuildNumber & 0xFFFF;   /* Win 9x needs this. */

    if (    dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
        &&  dwMajorVersion == 4)
    {
        if (        dwMinorVersion < 10
                 && dwBuildNumber == 950)
            enmVer = kRTWinOSType_95;
        else if (   dwMinorVersion < 10
                 && dwBuildNumber > 950
                 && dwBuildNumber <= 1080)
            enmVer = kRTWinOSType_95SP1;
        else if (   dwMinorVersion < 10
                 && dwBuildNumber > 1080)
            enmVer = kRTWinOSType_95OSR2;
        else if (   dwMinorVersion == 10
                 && dwBuildNumber == 1998)
            enmVer = kRTWinOSType_98;
        else if (   dwMinorVersion == 10
                 && dwBuildNumber > 1998
                 && dwBuildNumber < 2183)
            enmVer = kRTWinOSType_98SP1;
        else if (   dwMinorVersion == 10
                 && dwBuildNumber >= 2183)
            enmVer = kRTWinOSType_98SE;
        else if (dwMinorVersion == 90)
            enmVer = kRTWinOSType_ME;
    }
    else if (dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (        dwMajorVersion == 3
                 && dwMinorVersion == 51)
            enmVer = kRTWinOSType_NT351;
        else if (   dwMajorVersion == 4
                 && dwMinorVersion == 0)
            enmVer = kRTWinOSType_NT4;
        else if (   dwMajorVersion == 5
                 && dwMinorVersion == 0)
            enmVer = kRTWinOSType_2K;
        else if (   dwMajorVersion == 5
                 && dwMinorVersion == 1)
            enmVer = kRTWinOSType_XP;
        else if (   dwMajorVersion == 5
                 && dwMinorVersion == 2)
            enmVer = kRTWinOSType_2003;
        else if (   dwMajorVersion == 6
                 && dwMinorVersion == 0)
        {
            if (bProductType != VER_NT_WORKSTATION)
                enmVer = kRTWinOSType_2008;
            else
                enmVer = kRTWinOSType_VISTA;
        }
        else if (   dwMajorVersion == 6
                 && dwMinorVersion == 1)
        {
            if (bProductType != VER_NT_WORKSTATION)
                enmVer = kRTWinOSType_2008R2;
            else
                enmVer = kRTWinOSType_7;
        }
        else if (   dwMajorVersion == 6
                 && dwMinorVersion == 2)
        {
            if (bProductType != VER_NT_WORKSTATION)
                enmVer = kRTWinOSType_2012;
            else
                enmVer = kRTWinOSType_8;
        }
        else if (   dwMajorVersion == 6
                 && dwMinorVersion == 3)
        {
            if (bProductType != VER_NT_WORKSTATION)
                enmVer = kRTWinOSType_2012R2;
            else
                enmVer = kRTWinOSType_81;
        }
        else if (   (   dwMajorVersion == 6
                     && dwMinorVersion == 4)
                 || (   dwMajorVersion == 10
                     && dwMinorVersion == 0))
        {
            if (bProductType != VER_NT_WORKSTATION)
                enmVer = kRTWinOSType_2016;
            else
                enmVer = kRTWinOSType_10;
        }
        else
            enmVer = kRTWinOSType_NT_UNKNOWN;
    }

    return enmVer;
}


/**
 * Initializes the global variables related to windows version.
 */
static void rtR3InitWindowsVersion(void)
{
    Assert(g_hModNtDll != NULL);

    /*
     * ASSUMES OSVERSIONINFOEX starts with the exact same layout as OSVERSIONINFO (safe).
     */
    AssertCompileMembersSameSizeAndOffset(OSVERSIONINFOEX, szCSDVersion, OSVERSIONINFO, szCSDVersion);
    AssertCompileMemberOffset(OSVERSIONINFOEX, wServicePackMajor, sizeof(OSVERSIONINFO));

    /*
     * Use the NT version of GetVersionExW so we don't get fooled by
     * compatability shims.
     */
    RT_ZERO(g_WinOsInfoEx);
    g_WinOsInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    LONG (__stdcall *pfnRtlGetVersion)(OSVERSIONINFOEXW *);
    *(FARPROC *)&pfnRtlGetVersion = GetProcAddress(g_hModNtDll, "RtlGetVersion");
    LONG rcNt = -1;
    if (pfnRtlGetVersion)
        rcNt = pfnRtlGetVersion(&g_WinOsInfoEx);
    if (rcNt != 0)
    {
        /*
         * Couldn't find it or it failed, try the windows version of the API.
         */
        RT_ZERO(g_WinOsInfoEx);
        g_WinOsInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
        if (!GetVersionExW((POSVERSIONINFOW)&g_WinOsInfoEx))
        {
            /*
             * If that didn't work either, just get the basic version bits.
             */
            RT_ZERO(g_WinOsInfoEx);
            g_WinOsInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
            if (GetVersionExW((POSVERSIONINFOW)&g_WinOsInfoEx))
                Assert(g_WinOsInfoEx.dwPlatformId != VER_PLATFORM_WIN32_NT || g_WinOsInfoEx.dwMajorVersion < 5);
            else
            {
                AssertBreakpoint();
                RT_ZERO(g_WinOsInfoEx);
            }
        }
    }

    if (g_WinOsInfoEx.dwOSVersionInfoSize)
        g_enmWinVer = rtR3InitWinSimplifiedVersion(&g_WinOsInfoEx);
}


static int rtR3InitNativeObtrusiveWorker(uint32_t fFlags)
{
    /*
     * Disable error popups.
     */
    UINT fOldErrMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX | fOldErrMode);

    /*
     * Restrict DLL searching for the process on windows versions which allow
     * us to do so.
     *  - The first trick works on XP SP1+ and disables the searching of the
     *    current directory.
     *  - The second trick is W7 w/ KB2533623 and W8+, it restrict the DLL
     *    searching to the application directory (except when
     *    RTR3INIT_FLAGS_STANDALONE_APP is given) and the System32 directory.
     */
    int rc = VINF_SUCCESS;

    typedef BOOL (WINAPI *PFNSETDLLDIRECTORY)(LPCWSTR);
    PFNSETDLLDIRECTORY pfnSetDllDir = (PFNSETDLLDIRECTORY)GetProcAddress(g_hModKernel32, "SetDllDirectoryW");
    if (pfnSetDllDir)
    {
        if (pfnSetDllDir(L""))
            g_enmWinLdrProt = RTR3WINLDRPROT_NO_CWD;
        else
            rc = VERR_INTERNAL_ERROR_3;
    }

    /** @bugref{6861} Observed GUI issues on Vista (32-bit and 64-bit) when using
     *                SetDefaultDllDirectories.
     *  @bugref{8194} Try use SetDefaultDllDirectories on Vista for standalone apps
     *                despite potential GUI issues. */
    if (   g_enmWinVer > kRTWinOSType_VISTA
        || (fFlags & RTR3INIT_FLAGS_STANDALONE_APP))
    {
        typedef BOOL(WINAPI *PFNSETDEFAULTDLLDIRECTORIES)(DWORD);
        PFNSETDEFAULTDLLDIRECTORIES pfnSetDefDllDirs;
        pfnSetDefDllDirs = (PFNSETDEFAULTDLLDIRECTORIES)GetProcAddress(g_hModKernel32, "SetDefaultDllDirectories");
        if (pfnSetDefDllDirs)
        {
            DWORD fDllDirs = LOAD_LIBRARY_SEARCH_SYSTEM32;
            if (!(fFlags & RTR3INIT_FLAGS_STANDALONE_APP))
                fDllDirs |= LOAD_LIBRARY_SEARCH_APPLICATION_DIR;
            if (pfnSetDefDllDirs(fDllDirs))
                g_enmWinLdrProt = fDllDirs & LOAD_LIBRARY_SEARCH_APPLICATION_DIR ? RTR3WINLDRPROT_SAFE : RTR3WINLDRPROT_SAFER;
            else if (RT_SUCCESS(rc))
                rc = VERR_INTERNAL_ERROR_4;
        }
    }

    /*
     * Register an unhandled exception callback if we can.
     */
    g_pfnGetCurrentThreadStackLimits = (PFNGETCURRENTTHREADSTACKLIMITS)GetProcAddress(g_hModKernel32, "GetCurrentThreadStackLimits");
    g_pfnSetUnhandledExceptionFilter = (PFNSETUNHANDLEDEXCEPTIONFILTER)GetProcAddress(g_hModKernel32, "SetUnhandledExceptionFilter");
    if (g_pfnSetUnhandledExceptionFilter && !g_pfnUnhandledXcptFilter)
    {
        g_pfnUnhandledXcptFilter = g_pfnSetUnhandledExceptionFilter(rtR3WinUnhandledXcptFilter);
        AssertStmt(g_pfnUnhandledXcptFilter != rtR3WinUnhandledXcptFilter, g_pfnUnhandledXcptFilter = NULL);
    }

    return rc;
}


DECLHIDDEN(int) rtR3InitNativeFirst(uint32_t fFlags)
{
    /*
     * Make sure we've got the handles of the two main Windows NT dlls.
     */
    g_hModKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (g_hModKernel32 == NULL)
        return VERR_INTERNAL_ERROR_2;
    g_hModNtDll    = GetModuleHandleW(L"ntdll.dll");
    if (g_hModNtDll == NULL)
        return VERR_INTERNAL_ERROR_2;

    rtR3InitWindowsVersion();

    int rc = VINF_SUCCESS;
    if (!(fFlags & RTR3INIT_FLAGS_UNOBTRUSIVE))
        rc = rtR3InitNativeObtrusiveWorker(fFlags);

    /*
     * Resolve some kernel32.dll APIs we may need but aren't necessarily
     * present in older windows versions.
     */
    g_pfnGetSystemWindowsDirectoryW = (PFNGETWINSYSDIR)GetProcAddress(g_hModKernel32, "GetSystemWindowsDirectoryW");
    if (g_pfnGetSystemWindowsDirectoryW)
        g_pfnGetSystemWindowsDirectoryW = (PFNGETWINSYSDIR)GetProcAddress(g_hModKernel32, "GetWindowsDirectoryW");

    return rc;
}


DECLHIDDEN(void) rtR3InitNativeObtrusive(uint32_t fFlags)
{
    rtR3InitNativeObtrusiveWorker(fFlags);
}


DECLHIDDEN(int) rtR3InitNativeFinal(uint32_t fFlags)
{
    /* Nothing to do here. */
    RT_NOREF_PV(fFlags);
    return VINF_SUCCESS;
}


/**
 * Unhandled exception filter callback.
 *
 * Will try log stuff.
 */
static LONG CALLBACK rtR3WinUnhandledXcptFilter(PEXCEPTION_POINTERS pPtrs)
{
    /*
     * Try get the logger and log exception details.
     *
     * Note! We'll be using RTLogLogger for now, though we should probably add
     *       a less deadlock prone API here and gives up pretty fast if it
     *       cannot get the lock...
     */
    PRTLOGGER pLogger = RTLogRelGetDefaultInstance();
    if (!pLogger)
        pLogger = RTLogGetDefaultInstance();
    if (pLogger)
    {
        RTLogLogger(pLogger, NULL, "\n!!! rtR3WinUnhandledXcptFilter caught an exception on thread %p!!!\n", RTThreadNativeSelf());

        /*
         * Dump the exception record.
         */
        uintptr_t         uXcptPC  = 0;
        PEXCEPTION_RECORD pXcptRec = RT_VALID_PTR(pPtrs) && RT_VALID_PTR(pPtrs->ExceptionRecord) ? pPtrs->ExceptionRecord : NULL;
        if (pXcptRec)
        {
            RTLogLogger(pLogger, NULL, "\nExceptionCode=%#010x ExceptionFlags=%#010x ExceptionAddress=%p\n",
                        pXcptRec->ExceptionCode, pXcptRec->ExceptionFlags, pXcptRec->ExceptionAddress);
            for (uint32_t i = 0; i < RT_MIN(pXcptRec->NumberParameters, EXCEPTION_MAXIMUM_PARAMETERS); i++)
                RTLogLogger(pLogger, NULL, "ExceptionInformation[%d]=%p\n", i, pXcptRec->ExceptionInformation[i]);
            uXcptPC = (uintptr_t)pXcptRec->ExceptionAddress;

            /* Nested? Display one level only. */
            PEXCEPTION_RECORD pNestedRec = pXcptRec->ExceptionRecord;
            if (RT_VALID_PTR(pNestedRec))
            {
                RTLogLogger(pLogger, NULL, "Nested: ExceptionCode=%#010x ExceptionFlags=%#010x ExceptionAddress=%p (nested %p)\n",
                            pNestedRec->ExceptionCode, pNestedRec->ExceptionFlags, pNestedRec->ExceptionAddress,
                            pNestedRec->ExceptionRecord);
                for (uint32_t i = 0; i < RT_MIN(pNestedRec->NumberParameters, EXCEPTION_MAXIMUM_PARAMETERS); i++)
                    RTLogLogger(pLogger, NULL, "Nested: ExceptionInformation[%d]=%p\n", i, pNestedRec->ExceptionInformation[i]);
                uXcptPC = (uintptr_t)pNestedRec->ExceptionAddress;
            }
        }

        /*
         * Dump the context record.
         */
        volatile char   szMarker[] = "stackmarker";
        uintptr_t       uXcptSP = (uintptr_t)&szMarker[0];
        PCONTEXT pXcptCtx = RT_VALID_PTR(pPtrs) && RT_VALID_PTR(pPtrs->ContextRecord)   ? pPtrs->ContextRecord   : NULL;
        if (pXcptCtx)
        {
#ifdef RT_ARCH_AMD64
            RTLogLogger(pLogger, NULL, "\ncs:rip=%04x:%016RX64\n", pXcptCtx->SegCs, pXcptCtx->Rip);
            RTLogLogger(pLogger, NULL, "ss:rsp=%04x:%016RX64 rbp=%016RX64\n", pXcptCtx->SegSs, pXcptCtx->Rsp, pXcptCtx->Rbp);
            RTLogLogger(pLogger, NULL, "rax=%016RX64 rcx=%016RX64 rdx=%016RX64 rbx=%016RX64\n",
                        pXcptCtx->Rax, pXcptCtx->Rcx, pXcptCtx->Rdx, pXcptCtx->Rbx);
            RTLogLogger(pLogger, NULL, "rsi=%016RX64 rdi=%016RX64 rsp=%016RX64 rbp=%016RX64\n",
                        pXcptCtx->Rsi, pXcptCtx->Rdi, pXcptCtx->Rsp, pXcptCtx->Rbp);
            RTLogLogger(pLogger, NULL, "r8 =%016RX64 r9 =%016RX64 r10=%016RX64 r11=%016RX64\n",
                        pXcptCtx->R8,  pXcptCtx->R9,  pXcptCtx->R10, pXcptCtx->R11);
            RTLogLogger(pLogger, NULL, "r12=%016RX64 r13=%016RX64 r14=%016RX64 r15=%016RX64\n",
                        pXcptCtx->R12,  pXcptCtx->R13,  pXcptCtx->R14, pXcptCtx->R15);
            RTLogLogger(pLogger, NULL, "ds=%04x es=%04x fs=%04x gs=%04x eflags=%08x\n",
                        pXcptCtx->SegDs, pXcptCtx->SegEs, pXcptCtx->SegFs, pXcptCtx->SegGs, pXcptCtx->EFlags);
            RTLogLogger(pLogger, NULL, "p1home=%016RX64 p2home=%016RX64 pe3home=%016RX64\n",
                        pXcptCtx->P1Home, pXcptCtx->P2Home, pXcptCtx->P3Home);
            RTLogLogger(pLogger, NULL, "p4home=%016RX64 p5home=%016RX64 pe6home=%016RX64\n",
                        pXcptCtx->P4Home, pXcptCtx->P5Home, pXcptCtx->P6Home);
            RTLogLogger(pLogger, NULL, "   LastBranchToRip=%016RX64    LastBranchFromRip=%016RX64\n",
                        pXcptCtx->LastBranchToRip, pXcptCtx->LastBranchFromRip);
            RTLogLogger(pLogger, NULL, "LastExceptionToRip=%016RX64 LastExceptionFromRip=%016RX64\n",
                        pXcptCtx->LastExceptionToRip, pXcptCtx->LastExceptionFromRip);
            uXcptSP = pXcptCtx->Rsp;
            uXcptPC = pXcptCtx->Rip;

#elif defined(RT_ARCH_X86)
            RTLogLogger(pLogger, NULL, "\ncs:eip=%04x:%08RX32\n", pXcptCtx->SegCs, pXcptCtx->Eip);
            RTLogLogger(pLogger, NULL, "ss:esp=%04x:%08RX32 ebp=%08RX32\n", pXcptCtx->SegSs, pXcptCtx->Esp, pXcptCtx->Ebp);
            RTLogLogger(pLogger, NULL, "eax=%08RX32 ecx=%08RX32 edx=%08RX32 ebx=%08RX32\n",
                        pXcptCtx->Eax, pXcptCtx->Ecx,  pXcptCtx->Edx,  pXcptCtx->Ebx);
            RTLogLogger(pLogger, NULL, "esi=%08RX32 edi=%08RX32 esp=%08RX32 ebp=%08RX32\n",
                        pXcptCtx->Esi, pXcptCtx->Edi,  pXcptCtx->Esp,  pXcptCtx->Ebp);
            RTLogLogger(pLogger, NULL, "ds=%04x es=%04x fs=%04x gs=%04x eflags=%08x\n",
                        pXcptCtx->SegDs, pXcptCtx->SegEs, pXcptCtx->SegFs, pXcptCtx->SegGs, pXcptCtx->EFlags);
            uXcptSP = pXcptCtx->Esp;
            uXcptPC = pXcptCtx->Eip;
#endif
        }

        /*
         * Dump stack.
         */
        uintptr_t uStack = (uintptr_t)(void *)&szMarker[0];
        uStack -= uStack & 15;

        size_t cbToDump = PAGE_SIZE - (uStack & PAGE_OFFSET_MASK);
        if (cbToDump < 512)
            cbToDump += PAGE_SIZE;
        size_t cbToXcpt = uXcptSP - uStack;
        while (cbToXcpt > cbToDump && cbToXcpt <= _16K)
            cbToDump += PAGE_SIZE;
        ULONG_PTR uLow  = (uintptr_t)&szMarker[0];
        ULONG_PTR uHigh = (uintptr_t)&szMarker[0];
        if (g_pfnGetCurrentThreadStackLimits)
        {
            g_pfnGetCurrentThreadStackLimits(&uLow, &uHigh);
            size_t cbToTop = RT_MAX(uLow, uHigh) - uStack;
            if (cbToTop < _1M)
                cbToDump = cbToTop;
        }

        RTLogLogger(pLogger, NULL, "\nStack %p, dumping %#x bytes (low=%p, high=%p)\n", uStack, cbToDump, uLow, uHigh);
        RTLogLogger(pLogger, NULL, "%.*RhxD\n", cbToDump, uStack);

        /*
         * Try figure the thread name.
         *
         * Note! This involves the thread db lock, so it may deadlock, which
         *       is why it's at the end.
         */
        RTLogLogger(pLogger, NULL,  "Thread ID:   %p\n", RTThreadNativeSelf());
        RTLogLogger(pLogger, NULL,  "Thread name: %s\n", RTThreadSelfName());
        RTLogLogger(pLogger, NULL,  "Thread IPRT: %p\n", RTThreadSelf());

        /*
         * Try dump the load information.
         */
        PPEB pPeb = RTNtCurrentPeb();
        if (RT_VALID_PTR(pPeb))
        {
            PPEB_LDR_DATA pLdrData = pPeb->Ldr;
            if (RT_VALID_PTR(pLdrData))
            {
                PLDR_DATA_TABLE_ENTRY pFound     = NULL;
                LIST_ENTRY * const    pList      = &pLdrData->InMemoryOrderModuleList;
                LIST_ENTRY           *pListEntry = pList->Flink;
                uint32_t              cLoops     = 0;
                RTLogLogger(pLogger, NULL,
                            "\nLoaded Modules:\n"
                            "%-*s[*] Timestamp Path\n", sizeof(void *) * 4 + 2 - 1, "Address range"
                            );
                while (pListEntry != pList && RT_VALID_PTR(pListEntry) && cLoops < 1024)
                {
                    PLDR_DATA_TABLE_ENTRY pLdrEntry = RT_FROM_MEMBER(pListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
                    uint32_t const        cbLength  = (uint32_t)(uintptr_t)pLdrEntry->Reserved3[1];
                    char                  chInd     = ' ';
                    if (uXcptPC - (uintptr_t)pLdrEntry->DllBase < cbLength)
                    {
                        chInd = '*';
                        pFound = pLdrEntry;
                    }

                    if (   RT_VALID_PTR(pLdrEntry->FullDllName.Buffer)
                        && pLdrEntry->FullDllName.Length > 0
                        && pLdrEntry->FullDllName.Length < _8K
                        && (pLdrEntry->FullDllName.Length & 1) == 0
                        && pLdrEntry->FullDllName.Length <= pLdrEntry->FullDllName.MaximumLength)
                        RTLogLogger(pLogger, NULL, "%p..%p%c  %08RX32  %.*ls\n",
                                    pLdrEntry->DllBase, (uintptr_t)pLdrEntry->DllBase + cbLength - 1, chInd,
                                    pLdrEntry->TimeDateStamp, pLdrEntry->FullDllName.Length / sizeof(RTUTF16),
                                    pLdrEntry->FullDllName.Buffer);
                    else
                        RTLogLogger(pLogger, NULL, "%p..%p%c  %08RX32  <bad or missing: %p LB %#x max %#x\n",
                                    pLdrEntry->DllBase, (uintptr_t)pLdrEntry->DllBase + cbLength - 1, chInd,
                                    pLdrEntry->TimeDateStamp, pLdrEntry->FullDllName.Buffer, pLdrEntry->FullDllName.Length,
                                    pLdrEntry->FullDllName.MaximumLength);

                    /* advance */
                    pListEntry = pListEntry->Flink;
                    cLoops++;
                }

                /*
                 * Use the above to pick out code addresses on the stack.
                 */
                if (   cLoops < 1024
                    && uXcptSP - uStack < cbToDump)
                {
                    RTLogLogger(pLogger, NULL, "\nPotential code addresses on the stack:\n");
                    if (pFound)
                    {
                        if (   RT_VALID_PTR(pFound->FullDllName.Buffer)
                            && pFound->FullDllName.Length > 0
                            && pFound->FullDllName.Length < _8K
                            && (pFound->FullDllName.Length & 1) == 0
                            && pFound->FullDllName.Length <= pFound->FullDllName.MaximumLength)
                            RTLogLogger(pLogger, NULL, "%-*s: %p - %#010RX32 bytes into %.*ls\n",
                                        sizeof(void *) * 2, "Xcpt PC", uXcptPC, (uint32_t)(uXcptPC - (uintptr_t)pFound->DllBase),
                                        pFound->FullDllName.Length / sizeof(RTUTF16), pFound->FullDllName.Buffer);
                        else
                            RTLogLogger(pLogger, NULL, "%-*s: %p - %08RX32 into module at %p\n",
                                        sizeof(void *) * 2, "Xcpt PC", uXcptPC, (uint32_t)(uXcptPC - (uintptr_t)pFound->DllBase),
                                        pFound->DllBase);
                    }

                    uintptr_t const *puStack = (uintptr_t const *)uXcptSP;
                    uintptr_t        cLeft   = (cbToDump - (uXcptSP - uStack)) / sizeof(uintptr_t);
                    while (cLeft-- > 0)
                    {
                        uintptr_t uPtr = *puStack;
                        if (RT_VALID_PTR(uPtr))
                        {
                            /* Search the module table. */
                            pFound     = NULL;
                            cLoops     = 0;
                            pListEntry = pList->Flink;
                            while (pListEntry != pList && RT_VALID_PTR(pListEntry) && cLoops < 1024)
                            {
                                PLDR_DATA_TABLE_ENTRY pLdrEntry = RT_FROM_MEMBER(pListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
                                uint32_t const        cbLength  = (uint32_t)(uintptr_t)pLdrEntry->Reserved3[1];
                                if (uPtr - (uintptr_t)pLdrEntry->DllBase < cbLength)
                                {
                                    pFound = pLdrEntry;
                                    break;
                                }

                                /* advance */
                                pListEntry = pListEntry->Flink;
                                cLoops++;
                            }

                            if (pFound)
                            {
                                if (   RT_VALID_PTR(pFound->FullDllName.Buffer)
                                    && pFound->FullDllName.Length > 0
                                    && pFound->FullDllName.Length < _8K
                                    && (pFound->FullDllName.Length & 1) == 0
                                    && pFound->FullDllName.Length <= pFound->FullDllName.MaximumLength)
                                    RTLogLogger(pLogger, NULL, "%p: %p - %#010RX32 bytes into %.*ls\n",
                                                puStack, uPtr, (uint32_t)(uPtr - (uintptr_t)pFound->DllBase),
                                                pFound->FullDllName.Length / sizeof(RTUTF16), pFound->FullDllName.Buffer);
                                else
                                    RTLogLogger(pLogger, NULL, "%p: %p - %08RX32 into module at %p\n",
                                                puStack, uPtr, (uint32_t)(uPtr - (uintptr_t)pFound->DllBase), pFound->DllBase);
                            }
                        }

                        puStack++;
                    }
                }
            }
        }
    }

    /*
     * Do the default stuff, never mind us.
     */
    if (g_pfnUnhandledXcptFilter)
        return g_pfnUnhandledXcptFilter(pPtrs);
    return EXCEPTION_CONTINUE_SEARCH;
}


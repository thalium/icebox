/* $Id: VBoxServicePageSharing.cpp $ */
/** @file
 * VBoxService - Guest page sharing.
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


/** @page pg_vgsvc_pagesharing VBoxService - Page Sharing
 *
 * The Page Sharing subservice is responsible for finding memory mappings
 * suitable page fusions.
 *
 * It is the driving force behind the Page Fusion feature in VirtualBox.
 * Working with PGM and GMM (ring-0) thru the VMMDev interface.  Every so often
 * it reenumerates the memory mappings (executables and shared libraries) of the
 * guest OS and reports additions and removals to GMM.  For each mapping there
 * is a filename and version as well as and address range and subsections.  GMM
 * will match the mapping with mapping with the same name and version from other
 * VMs and see if there are any identical pages between the two.
 *
 * To increase the hit rate and reduce the volatility, the service launches a
 * child process which loads all the Windows system DLLs it can.  The child
 * process is necessary as the DLLs are loaded without running the init code,
 * and therefore not actually callable for other VBoxService code (may crash).
 *
 * This is currently only implemented on Windows.  There is no technical reason
 * for it not to be doable for all the other guests too, it's just a matter of
 * customer demand and engineering time.
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/assert.h>
#include <iprt/avl.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/ldr.h>
#include <iprt/process.h>
#include <iprt/env.h>
#include <iprt/stream.h>
#include <iprt/file.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <iprt/thread.h>
#include <iprt/time.h>
#include <VBox/VMMDev.h>
#include <VBox/VBoxGuestLib.h>
#include "VBoxServiceInternal.h"
#include "VBoxServiceUtils.h"

#if defined(RT_OS_WINDOWS) && !defined(TARGET_NT4)
# include <tlhelp32.h>
# include <psapi.h>
# include <winternl.h>
#endif


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
typedef struct
{
    AVLPVNODECORE   Core;
#ifdef RT_OS_WINDOWS
    HMODULE         hModule;
    char            szFileVersion[16];
# ifndef TARGET_NT4
    MODULEENTRY32   Info;
# endif
#endif /* RT_OS_WINDOWS */
} VGSVCPGSHKNOWNMOD, *PVGSVCPGSHKNOWNMOD;


#ifdef RT_OS_WINDOWS
/* NTDLL API we want to use: */
# define SystemModuleInformation 11

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    ULONG Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    CHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

typedef NTSTATUS (WINAPI *PFNZWQUERYSYSTEMINFORMATION)(ULONG, PVOID, ULONG, PULONG);

#endif /* RT_OS_WINDOWS */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef RT_OS_WINDOWS
static PFNZWQUERYSYSTEMINFORMATION g_pfnZwQuerySystemInformation = NULL;
#endif

/** The semaphore we're blocking on. */
static RTSEMEVENTMULTI  g_PageSharingEvent = NIL_RTSEMEVENTMULTI;

static PAVLPVNODECORE   g_pKnownModuleTree = NULL;
static uint64_t         g_idSession = 0;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int) vgsvcPageSharingEmptyTreeCallback(PAVLPVNODECORE pNode, void *pvUser);


#if defined(RT_OS_WINDOWS) && !defined(TARGET_NT4)

/**
 * Registers a new module with the VMM
 * @param   pModule         Module ptr
 * @param   fValidateMemory Validate/touch memory pages or not
 */
static void vgsvcPageSharingRegisterModule(PVGSVCPGSHKNOWNMOD pModule, bool fValidateMemory)
{
    VMMDEVSHAREDREGIONDESC   aRegions[VMMDEVSHAREDREGIONDESC_MAX];
    DWORD                    dwModuleSize = pModule->Info.modBaseSize;
    BYTE                    *pBaseAddress = pModule->Info.modBaseAddr;
    DWORD                    cbVersionSize, dummy;
    BYTE                    *pVersionInfo;

    VGSvcVerbose(3, "vgsvcPageSharingRegisterModule\n");

    cbVersionSize = GetFileVersionInfoSize(pModule->Info.szExePath, &dummy);
    if (!cbVersionSize)
    {
        VGSvcVerbose(3, "vgsvcPageSharingRegisterModule: GetFileVersionInfoSize failed with %d\n", GetLastError());
        return;
    }
    pVersionInfo = (BYTE *)RTMemAlloc(cbVersionSize);
    if (!pVersionInfo)
        return;

    if (!GetFileVersionInfo(pModule->Info.szExePath, 0, cbVersionSize, pVersionInfo))
    {
        VGSvcVerbose(3, "vgsvcPageSharingRegisterModule: GetFileVersionInfo failed with %d\n", GetLastError());
        goto end;
    }

    /* Fetch default code page. */
    struct LANGANDCODEPAGE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;

    UINT   cbTranslate;
    BOOL fRet = VerQueryValue(pVersionInfo, TEXT("\\VarFileInfo\\Translation"), (LPVOID *)&lpTranslate, &cbTranslate);
    if (   !fRet
        || cbTranslate < 4)
    {
        VGSvcVerbose(3, "vgsvcPageSharingRegisterModule: VerQueryValue failed with %d (cb=%d)\n", GetLastError(), cbTranslate);
        goto end;
    }

    unsigned i;
    UINT     cbFileVersion;
    char    *pszFileVersion = NULL; /* Shut up MSC */
    unsigned cTranslationBlocks = cbTranslate/sizeof(struct LANGANDCODEPAGE);

    pModule->szFileVersion[0] = '\0';
    for (i = 0; i < cTranslationBlocks; i++)
    {
        /* Fetch file version string. */
        char   szFileVersionLocation[256];

/** @todo r=bird: Mixing ANSI and TCHAR crap again.  This code is a mess.  We
 * always use the wide version of the API and convert to UTF-8/whatever. */

        sprintf(szFileVersionLocation, TEXT("\\StringFileInfo\\%04x%04x\\FileVersion"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
        fRet = VerQueryValue(pVersionInfo, szFileVersionLocation, (LPVOID *)&pszFileVersion, &cbFileVersion);
        if (fRet)
        {
            RTStrCopy(pModule->szFileVersion, sizeof(pModule->szFileVersion), pszFileVersion);
            break;
        }
    }
    if (i == cTranslationBlocks)
    {
        VGSvcVerbose(3, "vgsvcPageSharingRegisterModule: no file version found!\n");
        goto end;
    }

    unsigned idxRegion = 0;

    if (fValidateMemory)
    {
        do
        {
            MEMORY_BASIC_INFORMATION MemInfo;
            SIZE_T cbRet = VirtualQuery(pBaseAddress, &MemInfo, sizeof(MemInfo));
            Assert(cbRet);
            if (!cbRet)
            {
                VGSvcVerbose(3, "vgsvcPageSharingRegisterModule: VirtualQueryEx failed with %d\n", GetLastError());
                break;
            }

            if (   MemInfo.State == MEM_COMMIT
                && MemInfo.Type  == MEM_IMAGE)
            {
                switch (MemInfo.Protect)
                {
                    case PAGE_EXECUTE:
                    case PAGE_EXECUTE_READ:
                    case PAGE_READONLY:
                    {
                        char *pRegion = (char *)MemInfo.BaseAddress;

                        /* Skip the first region as it only contains the image file header. */
                        if (pRegion != (char *)pModule->Info.modBaseAddr)
                        {
                            /* Touch all pages. */
                            while ((uintptr_t)pRegion < (uintptr_t)MemInfo.BaseAddress + MemInfo.RegionSize)
                            {
                                /* Try to trick the optimizer to leave the page touching code in place. */
                                ASMProbeReadByte(pRegion);
                                pRegion += PAGE_SIZE;
                            }
                        }
#ifdef RT_ARCH_X86
                        aRegions[idxRegion].GCRegionAddr = (RTGCPTR32)MemInfo.BaseAddress;
#else
                        aRegions[idxRegion].GCRegionAddr = (RTGCPTR64)MemInfo.BaseAddress;
#endif
                        aRegions[idxRegion].cbRegion     = MemInfo.RegionSize;
                        idxRegion++;

                        break;
                    }

                    default:
                        break; /* ignore */
                }
            }

            pBaseAddress = (BYTE *)MemInfo.BaseAddress + MemInfo.RegionSize;
            if (dwModuleSize > MemInfo.RegionSize)
                dwModuleSize -= MemInfo.RegionSize;
            else
            {
                dwModuleSize = 0;
                break;
            }

            if (idxRegion >= RT_ELEMENTS(aRegions))
                break;  /* out of room */
        }
        while (dwModuleSize);
    }
    else
    {
        /* We can't probe kernel memory ranges, so pretend it's one big region. */
#ifdef RT_ARCH_X86
        aRegions[idxRegion].GCRegionAddr = (RTGCPTR32)pBaseAddress;
#else
        aRegions[idxRegion].GCRegionAddr = (RTGCPTR64)pBaseAddress;
#endif
        aRegions[idxRegion].cbRegion     = dwModuleSize;
        idxRegion++;
    }
    VGSvcVerbose(3, "vgsvcPageSharingRegisterModule: VbglR3RegisterSharedModule %s %s base=%p size=%x cregions=%d\n", pModule->Info.szModule, pModule->szFileVersion, pModule->Info.modBaseAddr, pModule->Info.modBaseSize, idxRegion);
    int rc = VbglR3RegisterSharedModule(pModule->Info.szModule, pModule->szFileVersion, (uintptr_t)pModule->Info.modBaseAddr,
                                        pModule->Info.modBaseSize, idxRegion, aRegions);
    if (RT_FAILURE(rc))
        VGSvcVerbose(3, "vgsvcPageSharingRegisterModule: VbglR3RegisterSharedModule failed with %Rrc\n", rc);

end:
    RTMemFree(pVersionInfo);
    return;
}


/**
 * Inspect all loaded modules for the specified process
 *
 * @param   dwProcessId     Process id
 * @param   ppNewTree       The module tree we're assembling from modules found
 *                          in this process.  Modules found are moved from
 *                          g_pKnownModuleTree or created new.
 */
static void vgsvcPageSharingInspectModules(DWORD dwProcessId, PAVLPVNODECORE *ppNewTree)
{
    /* Get a list of all the modules in this process. */
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE /* no child process handle inheritance */, dwProcessId);
    if (hProcess == NULL)
    {
        VGSvcVerbose(3, "vgsvcPageSharingInspectModules: OpenProcess %x failed with %d\n", dwProcessId, GetLastError());
        return;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        VGSvcVerbose(3, "vgsvcPageSharingInspectModules: CreateToolhelp32Snapshot failed with %d\n", GetLastError());
        CloseHandle(hProcess);
        return;
    }

    VGSvcVerbose(3, "vgsvcPageSharingInspectModules\n");

    MODULEENTRY32 ModuleInfo;
    BOOL          bRet;

    ModuleInfo.dwSize = sizeof(ModuleInfo);
    bRet = Module32First(hSnapshot, &ModuleInfo);
    do
    {
        /** @todo when changing this make sure VBoxService.exe is excluded! */
        char *pszDot = strrchr(ModuleInfo.szModule, '.');
        if (    pszDot
            &&  (pszDot[1] == 'e' || pszDot[1] == 'E'))
            continue;   /* ignore executables for now. */

        /* Found it before? */
        PAVLPVNODECORE pRec = RTAvlPVGet(ppNewTree, ModuleInfo.modBaseAddr);
        if (!pRec)
        {
            pRec = RTAvlPVRemove(&g_pKnownModuleTree, ModuleInfo.modBaseAddr);
            if (!pRec)
            {
                /* New module; register it. */
                PVGSVCPGSHKNOWNMOD pModule = (PVGSVCPGSHKNOWNMOD)RTMemAllocZ(sizeof(*pModule));
                Assert(pModule);
                if (!pModule)
                    break;

                pModule->Info     = ModuleInfo;
                pModule->Core.Key = ModuleInfo.modBaseAddr;
                pModule->hModule  = LoadLibraryEx(ModuleInfo.szExePath, 0, DONT_RESOLVE_DLL_REFERENCES);
                if (pModule->hModule)
                    vgsvcPageSharingRegisterModule(pModule, true /* validate pages */);

                VGSvcVerbose(3, "\n\n     MODULE NAME:     %s",           ModuleInfo.szModule );
                VGSvcVerbose(3, "\n     executable     = %s",             ModuleInfo.szExePath );
                VGSvcVerbose(3, "\n     process ID     = 0x%08X",         ModuleInfo.th32ProcessID );
                VGSvcVerbose(3, "\n     base address   = %#010p", (uintptr_t) ModuleInfo.modBaseAddr );
                VGSvcVerbose(3, "\n     base size      = %d",             ModuleInfo.modBaseSize );

                pRec = &pModule->Core;
            }
            bool ret = RTAvlPVInsert(ppNewTree, pRec);
            Assert(ret); NOREF(ret);
        }
    } while (Module32Next(hSnapshot, &ModuleInfo));

    CloseHandle(hSnapshot);
    CloseHandle(hProcess);
}


/**
 * Inspect all running processes for executables and dlls that might be worth sharing
 * with other VMs.
 *
 */
static void vgsvcPageSharingInspectGuest(void)
{
    HANDLE hSnapshot;
    PAVLPVNODECORE pNewTree = NULL;
    DWORD dwProcessId = GetCurrentProcessId();

    VGSvcVerbose(3, "vgsvcPageSharingInspectGuest\n");

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        VGSvcVerbose(3, "vgsvcPageSharingInspectGuest: CreateToolhelp32Snapshot failed with %d\n", GetLastError());
        return;
    }

    /* Check loaded modules for all running processes. */
    PROCESSENTRY32 ProcessInfo;

    ProcessInfo.dwSize = sizeof(ProcessInfo);
    Process32First(hSnapshot, &ProcessInfo);

    do
    {
        /* Skip our own process. */
        if (ProcessInfo.th32ProcessID != dwProcessId)
            vgsvcPageSharingInspectModules(ProcessInfo.th32ProcessID, &pNewTree);
    }
    while (Process32Next(hSnapshot, &ProcessInfo));

    CloseHandle(hSnapshot);

    /* Check all loaded kernel modules. */
    if (g_pfnZwQuerySystemInformation)
    {
        ULONG                cbBuffer = 0;
        PVOID                pBuffer = NULL;
        PRTL_PROCESS_MODULES pSystemModules;

        NTSTATUS ret = g_pfnZwQuerySystemInformation(SystemModuleInformation, (PVOID)&cbBuffer, 0, &cbBuffer);
        if (!cbBuffer)
        {
            VGSvcVerbose(1, "ZwQuerySystemInformation returned length 0\n");
            goto skipkernelmodules;
        }

        pBuffer = RTMemAllocZ(cbBuffer);
        if (!pBuffer)
            goto skipkernelmodules;

        ret = g_pfnZwQuerySystemInformation(SystemModuleInformation, pBuffer, cbBuffer, &cbBuffer);
        if (ret != STATUS_SUCCESS)
        {
            VGSvcVerbose(1, "ZwQuerySystemInformation returned %x (1)\n", ret);
            goto skipkernelmodules;
        }

        pSystemModules = (PRTL_PROCESS_MODULES)pBuffer;
        for (unsigned i = 0; i < pSystemModules->NumberOfModules; i++)
        {
            VGSvcVerbose(4, "\n\n   KERNEL  MODULE NAME:     %s", pSystemModules->Modules[i].FullPathName[pSystemModules->Modules[i].OffsetToFileName] );
            VGSvcVerbose(4, "\n     executable     = %s",         pSystemModules->Modules[i].FullPathName );
            VGSvcVerbose(4, "\n     flags          = 0x%08X\n",   pSystemModules->Modules[i].Flags);

            /* User-mode modules seem to have no flags set; skip them as we detected them above. */
            if (pSystemModules->Modules[i].Flags == 0)
                continue;

            /* Found it before? */
            PAVLPVNODECORE pRec = RTAvlPVGet(&pNewTree, pSystemModules->Modules[i].ImageBase);
            if (!pRec)
            {
                pRec = RTAvlPVRemove(&g_pKnownModuleTree, pSystemModules->Modules[i].ImageBase);
                if (!pRec)
                {
                    /* New module; register it. */
                    char szFullFilePath[512];
                    PVGSVCPGSHKNOWNMOD pModule = (PVGSVCPGSHKNOWNMOD)RTMemAllocZ(sizeof(*pModule));
                    Assert(pModule);
                    if (!pModule)
                        break;

/** @todo Use unicode APIs! */
                    strcpy(pModule->Info.szModule, &pSystemModules->Modules[i].FullPathName[pSystemModules->Modules[i].OffsetToFileName]);
                    GetSystemDirectoryA(szFullFilePath, sizeof(szFullFilePath));

                    /* skip \Systemroot\system32 */
                    char *lpPath = strchr(&pSystemModules->Modules[i].FullPathName[1], '\\');
                    if (!lpPath)
                    {
                        /* Seen just file names in XP; try to locate the file in the system32 and system32\drivers directories. */
                        strcat(szFullFilePath, "\\");
                        strcat(szFullFilePath, pSystemModules->Modules[i].FullPathName);
                        VGSvcVerbose(3, "Unexpected kernel module name try %s\n", szFullFilePath);
                        if (RTFileExists(szFullFilePath) == false)
                        {
                            GetSystemDirectoryA(szFullFilePath, sizeof(szFullFilePath));
                            strcat(szFullFilePath, "\\drivers\\");
                            strcat(szFullFilePath, pSystemModules->Modules[i].FullPathName);
                            VGSvcVerbose(3, "Unexpected kernel module name try %s\n", szFullFilePath);
                            if (RTFileExists(szFullFilePath) == false)
                            {
                                VGSvcVerbose(1, "Unexpected kernel module name %s\n", pSystemModules->Modules[i].FullPathName);
                                RTMemFree(pModule);
                                continue;
                            }
                        }
                    }
                    else
                    {
                        lpPath = strchr(lpPath+1, '\\');
                        if (!lpPath)
                        {
                            VGSvcVerbose(1, "Unexpected kernel module name %s (2)\n", pSystemModules->Modules[i].FullPathName);
                            RTMemFree(pModule);
                            continue;
                        }

                        strcat(szFullFilePath, lpPath);
                    }

                    strcpy(pModule->Info.szExePath, szFullFilePath);
                    pModule->Info.modBaseAddr = (BYTE *)pSystemModules->Modules[i].ImageBase;
                    pModule->Info.modBaseSize = pSystemModules->Modules[i].ImageSize;

                    pModule->Core.Key = pSystemModules->Modules[i].ImageBase;
                    vgsvcPageSharingRegisterModule(pModule, false /* don't check memory pages */);

                    VGSvcVerbose(3, "\n\n   KERNEL  MODULE NAME:     %s",     pModule->Info.szModule );
                    VGSvcVerbose(3, "\n     executable     = %s",             pModule->Info.szExePath );
                    VGSvcVerbose(3, "\n     base address   = %#010p", (uintptr_t)pModule->Info.modBaseAddr );
                    VGSvcVerbose(3, "\n     flags          = 0x%08X",         pSystemModules->Modules[i].Flags);
                    VGSvcVerbose(3, "\n     base size      = %d",             pModule->Info.modBaseSize );

                    pRec = &pModule->Core;
                }
                bool ret = RTAvlPVInsert(&pNewTree, pRec);
                Assert(ret); NOREF(ret);
            }
        }
skipkernelmodules:
        if (pBuffer)
            RTMemFree(pBuffer);
    }

    /* Delete leftover modules in the old tree. */
    RTAvlPVDestroy(&g_pKnownModuleTree, vgsvcPageSharingEmptyTreeCallback, NULL);

    /* Check all registered modules. */
    VbglR3CheckSharedModules();

    /* Activate new module tree. */
    g_pKnownModuleTree = pNewTree;
}


/**
 * RTAvlPVDestroy callback.
 */
static DECLCALLBACK(int) vgsvcPageSharingEmptyTreeCallback(PAVLPVNODECORE pNode, void *pvUser)
{
    PVGSVCPGSHKNOWNMOD pModule = (PVGSVCPGSHKNOWNMOD)pNode;
    bool *pfUnregister = (bool *)pvUser;

    VGSvcVerbose(3, "vgsvcPageSharingEmptyTreeCallback %s %s\n", pModule->Info.szModule, pModule->szFileVersion);

    /* Dereference module in the hypervisor. */
    if (   !pfUnregister
        || *pfUnregister)
    {
        int rc = VbglR3UnregisterSharedModule(pModule->Info.szModule, pModule->szFileVersion,
                                              (uintptr_t)pModule->Info.modBaseAddr, pModule->Info.modBaseSize);
        AssertRC(rc);
    }

    if (pModule->hModule)
        FreeLibrary(pModule->hModule);
    RTMemFree(pNode);
    return 0;
}


#elif TARGET_NT4

static void vgsvcPageSharingInspectGuest(void)
{
    /* not implemented */
}

#else

static void vgsvcPageSharingInspectGuest(void)
{
    /** @todo other platforms */
}

#endif

/** @interface_method_impl{VBOXSERVICE,pfnInit} */
static DECLCALLBACK(int) vgsvcPageSharingInit(void)
{
    VGSvcVerbose(3, "vgsvcPageSharingInit\n");

    int rc = RTSemEventMultiCreate(&g_PageSharingEvent);
    AssertRCReturn(rc, rc);

#if defined(RT_OS_WINDOWS) && !defined(TARGET_NT4)
    g_pfnZwQuerySystemInformation = (PFNZWQUERYSYSTEMINFORMATION)RTLdrGetSystemSymbol("ntdll.dll", "ZwQuerySystemInformation");

    rc = VbglR3GetSessionId(&g_idSession);
    if (RT_FAILURE(rc))
    {
        if (rc == VERR_IO_GEN_FAILURE)
            VGSvcVerbose(0, "PageSharing: Page sharing support is not available by the host\n");
        else
            VGSvcError("vgsvcPageSharingInit: Failed with rc=%Rrc\n", rc);

        rc = VERR_SERVICE_DISABLED;

        RTSemEventMultiDestroy(g_PageSharingEvent);
        g_PageSharingEvent = NIL_RTSEMEVENTMULTI;

    }
#endif

    return rc;
}


/**
 * @interface_method_impl{VBOXSERVICE,pfnWorker}
 */
static DECLCALLBACK(int) vgsvcPageSharingWorker(bool volatile *pfShutdown)
{
    /*
     * Tell the control thread that it can continue
     * spawning services.
     */
    RTThreadUserSignal(RTThreadSelf());

    /*
     * Now enter the loop retrieving runtime data continuously.
     */
    for (;;)
    {
        bool fEnabled = VbglR3PageSharingIsEnabled();
        VGSvcVerbose(3, "vgsvcPageSharingWorker: enabled=%d\n", fEnabled);

        if (fEnabled)
            vgsvcPageSharingInspectGuest();

        /*
         * Block for a minute.
         *
         * The event semaphore takes care of ignoring interruptions and it
         * allows us to implement service wakeup later.
         */
        if (*pfShutdown)
            break;
        int rc = RTSemEventMultiWait(g_PageSharingEvent, 60000);
        if (*pfShutdown)
            break;
        if (rc != VERR_TIMEOUT && RT_FAILURE(rc))
        {
            VGSvcError("vgsvcPageSharingWorker: RTSemEventMultiWait failed; rc=%Rrc\n", rc);
            break;
        }
#if defined(RT_OS_WINDOWS) && !defined(TARGET_NT4)
        uint64_t idNewSession = g_idSession;
        rc =  VbglR3GetSessionId(&idNewSession);
        AssertRC(rc);

        if (idNewSession != g_idSession)
        {
            bool fUnregister = false;

            VGSvcVerbose(3, "vgsvcPageSharingWorker: VM was restored!!\n");
            /* The VM was restored, so reregister all modules the next time. */
            RTAvlPVDestroy(&g_pKnownModuleTree, vgsvcPageSharingEmptyTreeCallback, &fUnregister);
            g_pKnownModuleTree = NULL;

            g_idSession = idNewSession;
        }
#endif
    }

    RTSemEventMultiDestroy(g_PageSharingEvent);
    g_PageSharingEvent = NIL_RTSEMEVENTMULTI;

    VGSvcVerbose(3, "vgsvcPageSharingWorker: finished thread\n");
    return 0;
}

#ifdef RT_OS_WINDOWS

/**
 * This gets control when VBoxService is launched with "pagefusion" by
 * vgsvcPageSharingWorkerProcess().
 *
 * @returns RTEXITCODE_SUCCESS.
 *
 * @remarks It won't normally return since the parent drops the shutdown hint
 *          via RTProcTerminate().
 */
RTEXITCODE VGSvcPageSharingWorkerChild(void)
{
    VGSvcVerbose(3, "vgsvcPageSharingInitFork\n");

    bool fShutdown = false;
    vgsvcPageSharingInit();
    vgsvcPageSharingWorker(&fShutdown);

    return RTEXITCODE_SUCCESS;
}


/**
 * @interface_method_impl{VBOXSERVICE,pfnWorker}
 */
static DECLCALLBACK(int) vgsvcPageSharingWorkerProcess(bool volatile *pfShutdown)
{
    RTPROCESS hProcess = NIL_RTPROCESS;
    int rc;

    /*
     * Tell the control thread that it can continue
     * spawning services.
     */
    RTThreadUserSignal(RTThreadSelf());

    /*
     * Now enter the loop retrieving runtime data continuously.
     */
    for (;;)
    {
        bool fEnabled = VbglR3PageSharingIsEnabled();
        VGSvcVerbose(3, "vgsvcPageSharingWorkerProcess: enabled=%d\n", fEnabled);

        /*
         * Start a 2nd VBoxService process to deal with page fusion as we do
         * not wish to dummy load dlls into this process.  (First load with
         * DONT_RESOLVE_DLL_REFERENCES, 2nd normal -> dll init routines not called!)
         */
        if (    fEnabled
            &&  hProcess == NIL_RTPROCESS)
        {
            char szExeName[256];
            char *pszExeName = RTProcGetExecutablePath(szExeName, sizeof(szExeName));
            if (pszExeName)
            {
                char const *papszArgs[3];
                papszArgs[0] = pszExeName;
                papszArgs[1] = "pagefusion";
                papszArgs[2] = NULL;
                rc = RTProcCreate(pszExeName, papszArgs, RTENV_DEFAULT, 0 /* normal child */, &hProcess);
                if (RT_FAILURE(rc))
                    VGSvcError("vgsvcPageSharingWorkerProcess: RTProcCreate %s failed; rc=%Rrc\n", pszExeName, rc);
            }
        }

        /*
         * Block for a minute.
         *
         * The event semaphore takes care of ignoring interruptions and it
         * allows us to implement service wakeup later.
         */
        if (*pfShutdown)
            break;
        rc = RTSemEventMultiWait(g_PageSharingEvent, 60000);
        if (*pfShutdown)
            break;
        if (rc != VERR_TIMEOUT && RT_FAILURE(rc))
        {
            VGSvcError("vgsvcPageSharingWorkerProcess: RTSemEventMultiWait failed; rc=%Rrc\n", rc);
            break;
        }
    }

    if (hProcess != NIL_RTPROCESS)
        RTProcTerminate(hProcess);

    VGSvcVerbose(3, "vgsvcPageSharingWorkerProcess: finished thread\n");
    return 0;
}

#endif /* RT_OS_WINDOWS */

/**
 * @interface_method_impl{VBOXSERVICE,pfnStop}
 */
static DECLCALLBACK(void) vgsvcPageSharingStop(void)
{
    RTSemEventMultiSignal(g_PageSharingEvent);
}


/**
 * @interface_method_impl{VBOXSERVICE,pfnTerm}
 */
static DECLCALLBACK(void) vgsvcPageSharingTerm(void)
{
    if (g_PageSharingEvent != NIL_RTSEMEVENTMULTI)
    {
        RTSemEventMultiDestroy(g_PageSharingEvent);
        g_PageSharingEvent = NIL_RTSEMEVENTMULTI;
    }
}


/**
 * The 'pagesharing' service description.
 */
VBOXSERVICE g_PageSharing =
{
    /* pszName. */
    "pagesharing",
    /* pszDescription. */
    "Page Sharing",
    /* pszUsage. */
    NULL,
    /* pszOptions. */
    NULL,
    /* methods */
    VGSvcDefaultPreInit,
    VGSvcDefaultOption,
    vgsvcPageSharingInit,
#ifdef RT_OS_WINDOWS
    vgsvcPageSharingWorkerProcess,
#else
    vgsvcPageSharingWorker,
#endif
    vgsvcPageSharingStop,
    vgsvcPageSharingTerm
};


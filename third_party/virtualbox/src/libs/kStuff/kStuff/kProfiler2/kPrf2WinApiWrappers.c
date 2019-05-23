/* $Id: kPrf2WinApiWrappers.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * Wrappers for a number of common Windows APIs.
 */

/*
 * Copyright (c) 2008 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define _ADVAPI32_
#define _KERNEL32_
#define _WIN32_WINNT 0x0600
#define UNICODE
#include <Windows.h>
#include <TLHelp32.h>
#include <k/kDefs.h>
#include "kPrf2WinApiWrapperHlp.h"

#if K_ARCH == K_ARCH_X86_32
typedef PVOID PRUNTIME_FUNCTION;
typedef FARPROC PGET_RUNTIME_FUNCTION_CALLBACK;
#endif

/* RtlUnwindEx is used by msvcrt on amd64, but winnt.h only defines it for IA64... */
typedef struct _FRAME_POINTERS {
    ULONGLONG MemoryStackFp;
    ULONGLONG BackingStoreFp;
} FRAME_POINTERS, *PFRAME_POINTERS;
typedef PVOID PUNWIND_HISTORY_TABLE;
typedef PVOID PKNONVOLATILE_CONTEXT_POINTERS;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
KPRF2WRAPDLL g_Kernel32 =
{
    INVALID_HANDLE_VALUE, "KERNEL32"
};


/*
 * Include the generated code.
 */
#include "kPrf2WinApiWrappers-kernel32.h"

/* TODO (amd64):

AddLocalAlternateComputerNameA
AddLocalAlternateComputerNameW
EnumerateLocalComputerNamesA
EnumerateLocalComputerNamesW
RemoveLocalAlternateComputerNameA
RemoveLocalAlternateComputerNameW

RtlLookupFunctionEntry
RtlPcToFileHeader
RtlRaiseException
RtlVirtualUnwind

SetConsoleCursor
SetLocalPrimaryComputerNameA
SetLocalPrimaryComputerNameW
__C_specific_handler
__misaligned_access
_local_unwind

*/


/**
 * The DLL Main for the Windows API wrapper DLL.
 *
 * @returns Success indicator.
 * @param   hInstDll        The instance handle of the DLL. (i.e. the module handle)
 * @param   fdwReason       The reason why we're here. This is a 'flag' for reasons of
 *                          tradition, it's really a kind of enum.
 * @param   pReserved       Reserved / undocumented something.
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, PVOID pReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}


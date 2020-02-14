/*
    MIT License

    Copyright (c) 2015 Nicolas Couffin ncouffin@gmail.com

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "FDP.h"

#include <DbgEng.h>

#pragma comment(lib, "dbgeng.lib")

#include "FDP.h"

// Redefine the API by adding the export attribute.
#undef DECLARE_API
#define DECLARE_API(s)                             \
    CPPMOD __declspec(dllexport) VOID              \
    s(                                             \
        HANDLE                 hCurrentProcess,    \
        HANDLE                 hCurrentThread,     \
        ULONG                  dwCurrentPc,        \
        ULONG                  dwProcessor,        \
        PCSTR                  args                \
     )

#undef DECLARE_API32
#define DECLARE_API32(s)                           \
    CPPMOD __declspec(dllexport) VOID              \
    s(                                             \
        HANDLE                 hCurrentProcess,    \
        HANDLE                 hCurrentThread,     \
        ULONG                  dwCurrentPc,        \
        ULONG                  dwProcessor,        \
        PCSTR                  args                \
     )

#undef DECLARE_API64
#define DECLARE_API64(s)                           \
    CPPMOD __declspec(dllexport) VOID              \
    s(                                             \
        HANDLE                 hCurrentProcess,    \
        HANDLE                 hCurrentThread,     \
        ULONG64                dwCurrentPc,        \
        ULONG                  dwProcessor,        \
        PCSTR                  args                \
     )

#include <ntverp.h>

//
// globals
//
EXT_API_VERSION ApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS ExtensionApis;
ULONG SavedMajorVersion;
ULONG SavedMinorVersion;

bool ExtensionInit();


VOID
__declspec(dllexport)
WinDbgExtensionDllInit(
PWINDBG_EXTENSION_APIS lpExtensionApis,
USHORT MajorVersion,
USHORT MinorVersion
) {
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    ExtensionInit();
    return;
}

LPEXT_API_VERSION
__declspec(dllexport)
ExtensionApiVersion(
VOID
) {
    return &ApiVersion;
}

VOID
__declspec(dllexport)
CheckVersion(
VOID
) {
    return;
}

/*
*
*
*
*
*
*
*
*/

#if DEBUG
#define ddprintf(fmt, ...) dprintf(fmt, __VA_ARGS__)
#else
#define ddprintf(fmt, ...)
#endif


FDP_SHM*                pFDP;
IDebugClient*            pDebugClient;
IDebugControl*            pDebugControl;
IDebugEventCallbacks*    pDebugEventCallbacks;

bool InitializeIDebug()
{
    HRESULT Hr;
    char Buffer[1024];
    IID *piidIDebugClient = (IID*)Buffer;
    Hr = IIDFromString(L"{27fe5639-8407-4f47-8364-ee118fb08ac8}", piidIDebugClient);
    if (Hr != S_OK){
        dprintf("Failed to IIDFromString 1\n");
        return false;
    }
    Hr = DebugCreate(piidIDebugClient, (void**)&pDebugClient);
    if (Hr != S_OK){
        dprintf("Failed to DebugCreate\n");
        return false;
    }

    IID *piidIDebugControl = (IID*)Buffer;
    Hr = IIDFromString(L"{5182e668-105e-416e-ad92-24ef800424ba}", piidIDebugControl);
    if (Hr != S_OK){
        dprintf("Failed to IIDFromString 2\n");
        return false;
    }
    Hr = pDebugClient->lpVtbl->QueryInterface(pDebugClient, piidIDebugControl, (void**)&pDebugControl);
    if (Hr != S_OK){
        dprintf("Failed to QueryInterface\n");
        return false;
    }

    /*
    IID *piidIDebugEventCallbacks = (IID*)Buffer;
    Hr = IIDFromString(L"{337be28b-5036-4d72-b6bf-c45fbb9f2eaa}", piidIDebugEventCallbacks);
    if (Hr != S_OK){
        dprintf("Failed to IIDFromString 3\n");
        return false;
    }
    Hr = pDebugClient->lpVtbl->QueryInterface(pDebugClient, piidIDebugEventCallbacks, (void**)&g_pDebugEventCallbacks);
    if (Hr != S_OK){
        dprintf("Failed to QueryInterface 3\n");
        return false;
    }
    */

    return true;
}

bool ExtensionInit()
{
    char FDP_SHMNameBuffer[256];
    ULONG ulMemoryRead = 0;

    dprintf("Extension initialization...\n");
    if (InitializeIDebug() == false){
        dprintf("Failed to InitializeIDebug\n");
        return false;
    }

    dprintf("Getting FDP SHM from Winbagility...\n");
    ReadMemory(0xDEADCACABABECAFE, FDP_SHMNameBuffer, 256, &ulMemoryRead);

    dprintf("Trying to connect to FDP %s\n", FDP_SHMNameBuffer);
    pFDP = FDP_OpenSHM(FDP_SHMNameBuffer);
    if (pFDP != NULL){
        dprintf("Connected to %s\n", FDP_SHMNameBuffer);
        return true;
    }
    else{
        dprintf("Failed to connect to FDP %s\n", FDP_SHMNameBuffer);
        return false;
    }
    return false;
}


void usage()
{
    dprintf(
        "!hba <Access type> <Address Type> <Address> <Breakpoint Length>\n"
        "   <Access Type>\n"
        "       r to break on read access\n"
        "       w to break on write access\n"
        "       e to break on execute access\n"
         "  <Address Type>\n"
        "       v for virtual address\n"
        "       p for physical address\n"
        "   <Address>\n"
        "       Virtual / Physical address to break on\n"
        "   <Breakpoint Length>\n"
        "       Length of the breakpoint\n\n");
    dprintf(
        "!hbint <Interrupt Number> <Error Code> <Cr2 Value>\n");
    dprintf("!hbc BreakpointId\n");
    dprintf(
        "!hbmsr <Access type> <Msr Id>\n"
        "   <Access Type>\n"
        "       r to break on read access\n"
        "       w to break on write access\n");
    dprintf(
        "!save\n");
    dprintf(
        "!restore\n");
    //dprintf("!hbl");
}

FDP_AddressType getAdressType(char *flags)
{
    if (strlen(flags) != 1){
        return FDP_WRONG_ADDRESS;
    }

    FDP_AddressType addressType = FDP_WRONG_ADDRESS;
    for (size_t i = 0; i < strlen(flags); i++){
        switch (flags[i]){
        case 'v': addressType = FDP_VIRTUAL_ADDRESS; break;
        case 'p': addressType = FDP_PHYSICAL_ADDRESS; break;
        default: addressType = FDP_WRONG_ADDRESS; break;
        }
    }
    return addressType;
}

FDP_Access getAccessType(char *flags)
{
    if (strlen(flags) < 1 || strlen(flags) > 3){
        return FDP_WRONG_BP;
    }

    FDP_Access ReturnFlags = FDP_WRONG_BP;
    for (size_t i = 0; i < strlen(flags); i++){
        switch (flags[i]){
        case 'e': ReturnFlags |= (int)FDP_EXECUTE_BP; break;
        case 'r': ReturnFlags |= (int)FDP_READ_BP; break;
        case 'w': ReturnFlags |= (int)FDP_WRITE_BP; break;
        default: return FDP_WRONG_BP;
        }
    }
    return ReturnFlags;
}

//!hbmsr r <value>
//Set an Execute breakpoint
DECLARE_API(hbmsr)
{
    ULONG64 BreakpointAddress = 0;
    char myArgs[1024];
    char *argList[10];
    int argc = 0;
    char lastChar = '\0';

    if (strlen(args) > 1024){
        usage();
        goto Fail;
    }

    strcpy_s(myArgs, sizeof(myArgs), args);

    //Argument string parsing
    size_t len = strlen(myArgs);
    for (size_t i = 0; i < len; i++){
        if (myArgs[i] == ' ')
            myArgs[i] = '\0';

        if (lastChar == '\0' && myArgs[i] != '\0'){
            argList[argc++] = &myArgs[i];
            if (argc >= 10){
                usage();
                goto Fail;
            }
        }
        lastChar = myArgs[i];
    }

    if (argc != 2){
        usage();
        goto Fail;
    }


    FDP_Access BreakpointAccessType = getAccessType(argList[0]);
    if (BreakpointAccessType != FDP_READ_BP
        && BreakpointAccessType != FDP_WRITE_BP){
        dprintf("Invalid breakpoint flags ! \n");
        usage();
        goto Fail;
    }

    //Symbol resolution, get address in hex !
    //GetExpressionEx(argList[1], &BreakpointAddress, NULL);
    BOOL ret = GetExpressionEx(argList[1], &BreakpointAddress, NULL);
    BreakpointAddress = BreakpointAddress & 0x00000000FFFFFFFF;
    if (ret == false){
        dprintf("Invalid breakpoint address ! \n");
        usage();
        goto Fail;
    }

    dprintf("AccessType %p\n", BreakpointAccessType);
    dprintf("Address %p\n", BreakpointAddress);

    int iBreakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_MSRHBP, -1, BreakpointAccessType, FDP_VIRTUAL_ADDRESS, BreakpointAddress, 1, FDP_NO_CR3);
    if (iBreakpointId >= 0){
        dprintf("Breakpoint installed : BreakpointId %d\n", iBreakpointId);
    }
    else{
        dprintf("Failed to FDP_SetBreakpoint\n");
    }

Fail:
    return;
}

//Set an Execute breakpoint
DECLARE_API(hba)
{
    ULONG64 BreakpointAddress = 0;
    char myArgs[1024];
    char *argList[10];
    int argc = 0;
    char lastChar = '\0';

    if (strlen(args) > 1024){
        usage();
        goto Fail;
    }
    
    strcpy_s(myArgs, sizeof(myArgs), args);

    //Argument string parsing
    size_t len = strlen(myArgs);
    for (size_t i = 0; i < len; i++){
        if (myArgs[i] == ' ')
            myArgs[i] = '\0';

        if (lastChar == '\0' && myArgs[i] != '\0'){
            argList[argc++] = &myArgs[i];
            if (argc >= 10){
                usage();
                goto Fail;
            }
        }
        lastChar = myArgs[i];
    }

    if (argc != 4){
        usage();
        goto Fail;
    }

    FDP_Access BreakpointAccessType = getAccessType(argList[0]);
    if (BreakpointAccessType == 0x0){
        dprintf("Invalid breakpoint flags ! \n");
        usage();
        goto Fail;
    }

    FDP_AddressType BreakpointAddressType = getAdressType(argList[1]);
    if (BreakpointAddressType == 0x0){
        dprintf("Invalid breakpoint adress type ! \n");
        usage();
        goto Fail;
    }

    //Symbol resolution, get address in hex !
    GetExpressionEx(argList[2], &BreakpointAddress, NULL);
    BOOL ret = GetExpressionEx(argList[2], &BreakpointAddress, NULL);
    if (ret == false){
        dprintf("Invalid breakpoint address ! \n");
        usage();
        goto Fail;
    }

    uint64_t BreakpointLength = atoi(argList[3]);
    if (BreakpointLength < 1){
        dprintf("Invalid breakpoint length ! \n");
        usage();
        goto Fail;
    }
    
    dprintf("AccessType:    %p\n", BreakpointAccessType);
    dprintf("Type:          %p\n", BreakpointAddressType);
    dprintf("Addres:        %p\n", BreakpointAddress);
    dprintf("Length:        %d\n", BreakpointLength);

    int BreakpointId = FDP_SetBreakpoint(pFDP, 0, FDP_PAGEHBP, -1, BreakpointAccessType, BreakpointAddressType, BreakpointAddress, BreakpointLength, FDP_NO_CR3);
    if (BreakpointId == -1){
        dprintf("Failed to install breakpoint\n");
    }
    else{
        dprintf("Breakpoint installed : BreakpointId %d\n", BreakpointId);
    }

Fail:
    return;
}


DECLARE_API(hbl)
{
    dprintf("TODO !\n");
}

DECLARE_API(saveinternal)
{
    if (!pFDP){
        dprintf("Not connected to debuggee !\n");
        goto Fail;
    }
    dprintf("Saving...");
    if(FDP_Save(pFDP) == true){
        FDP_SetStateChanged(pFDP);
        dprintf("[OK]\n");
    }
    else{
        dprintf("[FAIL]\n");
    }
Fail:
    return;
}

DECLARE_API(save)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    char TempPath[256];
    char Command[256];
    char ScriptData[] =
        //Clear all breakpoint
        ".if(1==1){bc *}            \r\n"
        //Commit change Register change before restore !
        ".if (1 == 1){!refresh}     \r\n"
        //Effective save
        ".if (1 == 1){!saveinternal}  \r\n"
        //Commit change Register change before restore !
        ".if (1 == 1){!refresh}     \r\n";

    if (!pFDP){
        dprintf("Not connected to debuggee !\n");
        goto Fail;
    }

    dprintf("WARNING, All your breakpoints are lost !\n");

    GetTempPathA(128, TempPath);
    strcat_s(TempPath, sizeof(TempPath), "save.wdbg");
    hFile = CreateFileA(TempPath, FILE_ALL_ACCESS, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE){
        goto Fail;
    }
    //Write command content
    if (WriteFile(hFile, ScriptData, sizeof(ScriptData), NULL, NULL) == FALSE){
        goto Fail;
    }
    //Generate command 
    sprintf_s(Command, sizeof(Command), "$><%s", TempPath);

    //Close file before call !
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
    HRESULT Hr = pDebugControl->lpVtbl->Execute(pDebugControl, DEBUG_OUTCTL_ALL_CLIENTS, Command, DEBUG_EXECUTE_NOT_LOGGED);
    (void) Hr;

Fail:
    if (hFile != INVALID_HANDLE_VALUE){
        CloseHandle(hFile);
    }
    return;
}

DECLARE_API(refresh)
{
    if (!pFDP){
        dprintf("Not connected to debuggee !\n");
        goto Fail;
    }

    //Enable Fake Single-Step
    ULONG ulMemoryRead;
    char Fake[1];
    ReadMemory(0xDEADCACABABEFACE, Fake, 1, &ulMemoryRead);
    HRESULT Hr = pDebugControl->lpVtbl->Execute(pDebugControl, DEBUG_OUTCTL_ALL_CLIENTS, "t", DEBUG_EXECUTE_NOT_LOGGED);
    (void) Hr;

Fail:
    return;
}

DECLARE_API(restoreinternal)
{
    if (!pFDP){
        dprintf("Not connected to debuggee !\n");
        goto Fail;
    }

    dprintf("Restoring...");
    if (FDP_Restore(pFDP) == true){
        FDP_SetStateChanged(pFDP);
        dprintf("[OK]\n");
    }
    else{
        dprintf("[FAIL]\n");
    }

Fail:
    return;
}

DECLARE_API(restore)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    char TempPath[256];
    char Command[256];
    char ScriptData[] =
        //Clear all breakpoint
        ".if(1==1){bc *}            \r\n"
        //Commit change Register change before restore !
        ".if (1 == 1){!refresh}     \r\n"
        //Effective restore
        ".if (1 == 1){!restoreinternal}  \r\n"
        //Commit change Register change before restore !
        ".if (1 == 1){!refresh}     \r\n";

    if (!pFDP){
        dprintf("Not connected to debuggee !\n");
        goto Fail;
    }

    dprintf("WARNING, All your breakpoints are lost !\n");

    GetTempPathA(128, TempPath);
    strcat_s(TempPath, sizeof(TempPath), "restore.wdbg");
    hFile = CreateFileA(TempPath, FILE_ALL_ACCESS, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE){
        goto Fail;
    }
    //Write command content
    if (WriteFile(hFile, ScriptData, sizeof(ScriptData), NULL, NULL) == FALSE){
        goto Fail;
    }
    //Generate command 
    sprintf_s(Command, sizeof(Command), "$><%s", TempPath);

    //Close file before call !
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
    HRESULT Hr = pDebugControl->lpVtbl->Execute(pDebugControl, DEBUG_OUTCTL_ALL_CLIENTS, Command, DEBUG_EXECUTE_NOT_LOGGED);
    (void) Hr;
    
Fail:
    if (hFile != INVALID_HANDLE_VALUE){
        CloseHandle(hFile);
    }
    return;
}



DECLARE_API(hbdr)
{
    if (!pFDP){
        dprintf("Not connected to debuggee !\n");
        goto Fail;
    }

    uint64_t uTempValue = 0;
    FDP_ReadRegister(pFDP, 0, FDP_VDR0_REGISTER, &uTempValue);
    dprintf("VDR0 : %p\n", uTempValue);
    FDP_ReadRegister(pFDP, 0, FDP_VDR1_REGISTER, &uTempValue);
    dprintf("VDR1 : %p\n", uTempValue);
    FDP_ReadRegister(pFDP, 0, FDP_VDR2_REGISTER, &uTempValue);
    dprintf("VDR2 : %p\n", uTempValue);
    FDP_ReadRegister(pFDP, 0, FDP_VDR3_REGISTER, &uTempValue);
    dprintf("VDR3 : %p\n", uTempValue);
    FDP_ReadRegister(pFDP, 0, FDP_VDR6_REGISTER, &uTempValue);
    dprintf("VDR6 : %p\n", uTempValue);
    FDP_ReadRegister(pFDP, 0, FDP_VDR7_REGISTER, &uTempValue);
    dprintf("VDR7 : %p\n", uTempValue);

Fail:
    return;
}

//Clear a breakpoint !
DECLARE_API(hbc)
{
    ULONG64 Value;
    ULONG64 BreakpointId;
    PCSTR Remainder;

    if (!pFDP){
        dprintf("Not connected to debuggee !\n");
        goto Fail;
    }

    BOOL ret = GetExpressionEx(args, &Value, &Remainder);
    if (ret == TRUE) {
        BreakpointId = Value;
    }
    else{
        usage();
        goto Fail;
    }

    if (FDP_UnsetBreakpoint(pFDP, (uint8_t)BreakpointId) == true){
        dprintf("Breakpoint removed\n");
    }
    else{
        dprintf("Failed to FDP_UnsetBreakpoint\n");
    }

Fail:
    return;
}

//!hbint 0x0E 0x00 Cr2
DECLARE_API(hbint)
{
    PCSTR Remainder = NULL;
    char myArgs[1024];
    char *argList[10];
    int argc = 0;
    char lastChar = '\0';

    if (strlen(args) > 1024){
        usage();
        goto Fail;
    }

    strcpy_s(myArgs, sizeof(myArgs), args);

    //Argument string parsing
    size_t len = strlen(myArgs);
    for (size_t i = 0; i < len; i++){
        if (myArgs[i] == ' ')
            myArgs[i] = '\0';

        if (lastChar == '\0' && myArgs[i] != '\0'){
            argList[argc++] = &myArgs[i];
            if (argc >= 10){
                usage();
                goto Fail;
            }
        }
        lastChar = myArgs[i];
    }
    if (argc != 3){
        usage();
        goto Fail;
    }

    ULONG64 uInterruptionCode;
    ULONG64 uErrorceCode;
    ULONG64 Cr2;
    GetExpressionEx(argList[0], &uInterruptionCode, &Remainder);
    GetExpressionEx(argList[1], &uErrorceCode, &Remainder);
    GetExpressionEx(argList[2], &Cr2, &Remainder);

    dprintf("FDP_InjectInterrupt %02x %08x %p \n", uInterruptionCode, uErrorceCode, Cr2);
    if (FDP_InjectInterrupt(pFDP, 0, (uint32_t)uInterruptionCode, (uint32_t)uErrorceCode, Cr2) == true){
        dprintf("Interrupt injected.\n");
    }
    else{
        dprintf("Failed to FDP_InjectInterrupt\n");
    }

Fail:
    return;
}

//DllMain
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

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
#include "Windows.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <dbghelp.h>
#include <stdlib.h>


#include "symbols.h"

#define FAKE_BASE_ADDRESS 0x1

bool PdbOpenPdb(PDB_PARSER_T *pPdbParserHandle, const char *pPDBPath)
{
    bool bOk = false;

    pPdbParserHandle->hProcess = GetCurrentProcess();
    if (SymInitialize(pPdbParserHandle->hProcess, NULL, false) == false){
        printf("Failed to SymInitialize\n");
        return false;
    }

    DWORD BaseAddr = FAKE_BASE_ADDRESS;
    DWORD FileSize = 0;
    HANDLE hFile = CreateFile(pPDBPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE){
        if (GetFileSize(hFile, &FileSize) != INVALID_FILE_SIZE){
            if (SymLoadModule(pPdbParserHandle->hProcess, NULL, pPDBPath, NULL, BaseAddr, FileSize) > 0){
                bOk = true;
            }
            else{
                printf("Failed to SymLoadModuleEx (%d)\n", GetLastError());
            }
        }
        else{
            printf("Failed to GetFileSize (%d)\n", GetLastError());
        }
    }
    else{
        printf("Failed to CreateFile\n");
    }

    if (hFile != INVALID_HANDLE_VALUE){
        CloseHandle(hFile);
    }
    if (bOk == false){
        SymCleanup(pPdbParserHandle->hProcess);
    }

    return bOk;
}

bool PdbGetSymbolsRVA(PDB_PARSER_T  *pPdbParserHandle, const char *pSymbolName, uint64_t *pRVA)
{
    char Buffer[1024];
    memset(Buffer, 0, sizeof(Buffer));
    SYMBOL_INFO *pInfo = (SYMBOL_INFO *)Buffer;
    *pRVA = 0;

    pInfo->SizeOfStruct = sizeof(*pInfo);
    pInfo->MaxNameLen = MAX_PATH;
    pInfo->ModBase = 0;

    if (SymFromName(pPdbParserHandle->hProcess, pSymbolName, pInfo) == false) {
        printf("Failed to SymFromName (%d)\n", GetLastError());
        return false;
    }
    *pRVA = pInfo->Address - FAKE_BASE_ADDRESS;

    return true;
}

bool PdbClose(PDB_PARSER_T  *pPdbParserHandle)
{
    SymCleanup(pPdbParserHandle->hProcess);
    return true;
}
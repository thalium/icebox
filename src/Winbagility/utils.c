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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "utils.h"

//TODO: macro
void printTime()
{
    SYSTEMTIME tmp;
    GetSystemTime(&tmp);

    printf("%02d:%02d:%02d:%03d\n", tmp.wHour, tmp.wMinute, tmp.wSecond, tmp.wMilliseconds);
}

int roundup16(int value)
{
    return (value + 15) & ~15;
}

void dumpHexData(char *tmp, int len)
{
    printf("char pkt[] = {\n");
    int i;
    for (i = 0; i<len; i++){
        printf("0x%02x, ", tmp[i] & 0xFF);
        if (i % 16 == 15){
            printf("\n");
        }
    }
    if (i % 16 != 0){
        printf("\n");
    }
    printf("};\n");
}

void printHexData(char *tmp, int len)
{
    int i;
    for (i = 0; i<len; i++){
        printf("%02x ", tmp[i] & 0xFF);
        if (i % 16 == 15){
            printf("\n");
        }
    }
    if (i % 16 != 0){
        printf("\n");
    }
}




bool CreateConnectNamedPipe(HANDLE *hPipe, char *pipeName)
{
    *hPipe = CreateNamedPipeA(
        pipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE,
        100,
        1 * 1024,
        1 * 1024,
        1000,
        NULL
        );
    if (*hPipe == NULL || *hPipe == INVALID_HANDLE_VALUE) {
        printf("Failed to create outbound pipe instance.\n");
        return false;
    }
    printf("[Main] NamedPipe %s created ! Waiting connection...\n", pipeName);
    BOOL result = ConnectNamedPipe(*hPipe, NULL);
    if (!result) {
        printf("[Main] Failed to make connection on named pipe.\n");
        CloseHandle(*hPipe);
        return false;
    }
    printf("[Main] Client connected !\n");
    return true;
}


bool OpenNamedPipe(HANDLE *hPipe, char *pipeName)
{
    while (1){
        *hPipe = CreateFileA(
            pipeName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (*hPipe != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_PIPE_BUSY){
            printf("[Main] Waiting for NamedPipe... \n");
            Sleep(1000);
        }
        else{
            if (!WaitNamedPipeA(pipeName, 1000)){
                printf("[Main] Error when wait NamedPipe\n");
            }
        }
    }
    DWORD dwMode = PIPE_TYPE_BYTE;
    BOOL result = SetNamedPipeHandleState(
        *hPipe,
        &dwMode,
        NULL,
        NULL);
    if (!result){
        return false;
    }
    printf("[Main] Connected to server NamedPipe !\n");
    return true;
}

bool GetPipeTry(HANDLE hPipe, uint8_t* data, uint64_t size, bool timeout)
{
    DWORD avalaibleBytes;
    uint32_t TryCount = 0;
    do{
        PeekNamedPipe(hPipe, NULL, 0, NULL, &avalaibleBytes, NULL);
        if (avalaibleBytes >= size){
            DWORD numBytesRead = 0;
            BOOL result = ReadFile(hPipe, data, (uint32_t)size, &numBytesRead, NULL);
            return true;
        }
        else{
            if ((TryCount & 0xFFFF) == 0xFFFF){
                Sleep(0); //5
            }
            else{
                TryCount++;
            }
        }
    } while (timeout == false);
    return false;
}

bool GetPipe(HANDLE hPipe, uint8_t* data, uint64_t size)
{
    return GetPipeTry(hPipe, data, size, false);
}

uint8_t Get8Pipe(HANDLE hPipe)
{
    uint8_t tmp;
    GetPipe(hPipe, &tmp, sizeof(tmp));
    return tmp;
}

uint16_t Get16Pipe(HANDLE hPipe)
{
    uint16_t tmp;
    GetPipe(hPipe, (uint8_t*)&tmp, sizeof(tmp));
    return tmp;
}

uint32_t Get32Pipe(HANDLE hPipe)
{
    uint32_t tmp;
    GetPipe(hPipe, (uint8_t*)&tmp, sizeof(tmp));
    return tmp;
}

uint64_t Get64Pipe(HANDLE hPipe)
{
    uint64_t tmp;
    GetPipe(hPipe, (uint8_t*)&tmp, sizeof(tmp));
    return tmp;
}

DWORD PutPipe(HANDLE hPipe, uint8_t *data, uint64_t size)
{
    DWORD numBytesWritten = 0;
    BOOL result = WriteFile(hPipe, data, (uint32_t)size, &numBytesWritten, NULL);
    return numBytesWritten;
}

DWORD Put8Pipe(HANDLE hPipe, uint8_t data)
{
    return PutPipe(hPipe, &data, sizeof(data));
}

DWORD Put32Pipe(HANDLE hPipe, uint32_t data)
{
    return PutPipe(hPipe, (uint8_t*)&data, sizeof(data));
}

DWORD Put64Pipe(HANDLE hPipe, uint64_t data)
{
    return PutPipe(hPipe, (uint8_t*)&data, sizeof(data));
}

bool IsLetter(char c)
{
    if (c >= 'a' && c <= 'z')
        return true;
    if (c >= 'A' && c <= 'Z')
        return true;
    return false;
}

char* GetSymbolsDirectory()
{
    char *pSymbolsDirectoryReturned = NULL;
    char *pSymbolsDirectory = NULL;
    char Buffer[512];
    if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", Buffer, sizeof(Buffer)) == 0){
        return NULL;
    }
    //Extract Directory
    for (size_t i = 0; i < strlen(Buffer) - 4; i++){
        if (Buffer[i] == '*'
            && IsLetter(Buffer[i + 1])
            && (Buffer[i + 2] == '\\' || Buffer[i + 3] == '\\')){

            pSymbolsDirectory = &Buffer[i + 1];

            for (size_t j = i + 1; j < strlen(Buffer); j++){
                if (Buffer[j] == '*'){
                    Buffer[j] = '\0';
                }
            }
            break;
        }
    }

    if (pSymbolsDirectory != NULL){
        size_t szSymbolsDirectoryReturnedSize = strlen(pSymbolsDirectory) + 1 + 1;
        char *pSymbolsDirectoryReturned = (char*)malloc(szSymbolsDirectoryReturnedSize);
        if (pSymbolsDirectoryReturned != NULL){
            memcpy(pSymbolsDirectoryReturned, pSymbolsDirectory, strlen(pSymbolsDirectory) + 1);
            strncat_s(pSymbolsDirectoryReturned, szSymbolsDirectoryReturnedSize, "\\", 1);
        }
        return pSymbolsDirectoryReturned;
    }

    return NULL;
}

bool ReccurseFile(char *pDirectoryPath, bool(*pfnUserCallBack)(void *pUserHandle, char *pFilePath), void *pUserHandle, bool bBreakOnTrue)
{
    WIN32_FIND_DATAA Win32FindData;
    char DirectoryPathStar[MAX_PATH];
    sprintf_s(DirectoryPathStar, sizeof(DirectoryPathStar), "%s\\*", pDirectoryPath);
    HANDLE hFind = FindFirstFileA(DirectoryPathStar, &Win32FindData);
    if (INVALID_HANDLE_VALUE == hFind){
        return false;
    }

    do  {
        if (Win32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(Win32FindData.cFileName, ".") != 0 && strcmp(Win32FindData.cFileName, "..") != 0){
                char InnerDirectoryPath[MAX_PATH];
                sprintf_s(InnerDirectoryPath, sizeof(InnerDirectoryPath), "%s\\%s\\", pDirectoryPath, Win32FindData.cFileName);

                bool bResult = ReccurseFile(InnerDirectoryPath, pfnUserCallBack, pUserHandle, bBreakOnTrue);
                if (bResult == true && bBreakOnTrue == true){
                    return true;
                }
            }
        }
        else{
            char FilePath[MAX_PATH];
            sprintf_s(FilePath, sizeof(FilePath), "%s\\%s", pDirectoryPath, Win32FindData.cFileName);
            bool bResult = pfnUserCallBack(pUserHandle, FilePath);
            if (bResult == true && bBreakOnTrue == true){
                return true;
            }
        }
    } while (FindNextFileA(hFind, &Win32FindData) != 0);
    return false;
}
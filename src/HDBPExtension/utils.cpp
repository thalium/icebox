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
#include "stdafx.h"
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

//TODO: macro
int roundup16(int value){
    return (value + 15) & ~15;
}

void dumpHexData(char *tmp, int len){
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

void printHexData(char *tmp, int len){
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




BOOL CreateNamedPipe(HANDLE *hPipe, char *pipeName){
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
        //system("pause");
        return false;
    }
    printf("[Main] NamedPipe %s created ! Waiting connection...\n", pipeName);
    BOOL result = ConnectNamedPipe(*hPipe, NULL);
    if (!result) {
        printf("[Main] Failed to make connection on named pipe.\n");
        CloseHandle(*hPipe);
        //system("pause");
        return false;
    }
    printf("[Main] Client connected !\n");
    return true;
}


BOOL OpenNamedPipe(HANDLE *hPipe, char *pipeName){
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
        //system("pause");
        return false;
    }
    printf("[Main] Connected to server NamedPipe !\n");
    return true;
}

bool GetPipe(HANDLE hPipe, uint8_t* data, uint64_t size){
    DWORD avalaibleBytes;
    uint8_t trycount = 0;
    while (trycount < 0x8F){
        PeekNamedPipe(hPipe, NULL, 0, NULL, &avalaibleBytes, NULL);
        if (avalaibleBytes >= size){
            DWORD numBytesRead = 0;
            BOOL result = ReadFile(hPipe, data, (uint32_t)size, &numBytesRead, NULL);
            return true;
        }
        else{
            trycount++;
            Sleep(10);
        }
    }
    return false;
}

uint8_t Get8Pipe(HANDLE hPipe){
    uint8_t tmp;
    GetPipe(hPipe, &tmp, sizeof(tmp));
    return tmp;
}

uint16_t Get16Pipe(HANDLE hPipe){
    uint16_t tmp;
    GetPipe(hPipe, (uint8_t*)&tmp, sizeof(tmp));
    return tmp;
}

uint32_t Get32Pipe(HANDLE hPipe){
    uint32_t tmp;
    GetPipe(hPipe, (uint8_t*)&tmp, sizeof(tmp));
    return tmp;
}

uint64_t Get64Pipe(HANDLE hPipe){
    uint64_t tmp;
    GetPipe(hPipe, (uint8_t*)&tmp, sizeof(tmp));
    return tmp;
}

DWORD PutPipe(HANDLE hPipe, uint8_t *data, uint64_t size){
    DWORD numBytesWritten = 0;
    BOOL result = WriteFile(hPipe, data, (uint32_t)size, &numBytesWritten, NULL);
    return numBytesWritten;
}

DWORD Put8Pipe(HANDLE hPipe, uint8_t data){
    return PutPipe(hPipe, &data, sizeof(data));
}

DWORD Put32Pipe(HANDLE hPipe, uint32_t data){
    return PutPipe(hPipe, (uint8_t*)&data, sizeof(data));
}

DWORD Put64Pipe(HANDLE hPipe, uint64_t data){
    return PutPipe(hPipe, (uint8_t*)&data, sizeof(data));
}
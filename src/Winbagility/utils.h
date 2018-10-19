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
#ifndef __UTILS_H__
#define __UTILS_H__

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

//define if you don't want 100% CPU consomption, performance will be lower...
#define WINBAGILITY_POWER_SAVE 1

//define verbose log level
#define DEBUG_LEVEL 0
#define DEBUG_FLOW 0

#if DEBUG_LEVEL > 0
#define Log1(fmt,...) printf(fmt, ##__VA_ARGS__)
#else
#define Log1(fmt,...)
#endif

#if DEBUG_FLOW > 0
#define LogFlow() printf("%s\n", __FUNCTION__);
#else
#define LogFlow()
#endif

#define MSR_EFER            0xC0000080
#define MSR_STAR            0xC0000081
#define MSR_LSTAR           0xC0000082
#define MSR_CSTAR           0xC0000084
#define MSR_SYSCALL_MASK    0xC0000084
#define MSR_GS_BASE         0xC0000101
#define MSR_KERNEL_GS_BASE  0xC0000102

#define _1G 1*1024*1024*1024
#ifndef _1M
#define _1M 1024*1024
#endif
#define _2M 2*_1M
#define _4K 4096

#define KERNEL_SPACE_START          0xFFFFF80000000000
#define KERNEL_SPACE_START_X86          0x80000000

void         printTime();
int          roundup16(int value);
void         dumpHexData(char *tmp, int len);
void         printHexData(char *tmp, int len);

#ifdef _MSC_VER
//todo: NamedPipe utilities file
bool        CreateConnectNamedPipe(HANDLE* hPipe, char* pipeName);
bool        OpenNamedPipe(HANDLE* hPipe, char* pipeName);
bool        GetPipeTry(HANDLE hPipe, uint8_t* data, uint64_t size, bool timeout);
bool        GetPipe(HANDLE hPipe, uint8_t* data, uint64_t size);
uint8_t     Get8Pipe(HANDLE hPipe);
uint16_t    Get16Pipe(HANDLE hPipe);
uint32_t    Get32Pipe(HANDLE hPipe);
uint64_t    Get64Pipe(HANDLE hPipe);
DWORD       PutPipe(HANDLE hPipe, uint8_t* data, uint64_t size);
DWORD       Put8Pipe(HANDLE hPipe, uint8_t data);
DWORD       Put32Pipe(HANDLE hPipe, uint32_t data);
DWORD       Put64Pipe(HANDLE hPipe, uint64_t data);
#endif


//TODO: ...
__inline uint64_t LeftShift(uint64_t value, uint64_t count)
{
    for (int i = 0; i < count; i++){
        value = value << 1;
    }
    return value;
}


bool    IsLetter(char c);
char*   GetSymbolsDirectory();
bool    ReccurseFile(char *pDirectoryPath, bool(*pfnUserCallBack)(void *pUserHandle, char *pFilePath), void *pUserHandle, bool bBreakOnTrue);

#endif //__UTILS_H__

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

#include <intrin.h>
#include <Windows.h>
#include <stdint.h>

int roundup16(int value);
void dumpHexData(char *tmp, int len);
void printHexData(char *tmp, int len);

BOOL CreateNamedPipe(HANDLE *hPipe, char *pipeName);
BOOL OpenNamedPipe(HANDLE *hPipe, char *pipeName);
bool GetPipe(HANDLE hPipe, uint8_t* data, uint64_t size);
uint8_t Get8Pipe(HANDLE hPipe);
uint16_t Get16Pipe(HANDLE hPipe);
uint32_t Get32Pipe(HANDLE hPipe);
uint64_t Get64Pipe(HANDLE hPipe);
DWORD PutPipe(HANDLE hPipe, uint8_t *data, uint64_t size);
DWORD Put8Pipe(HANDLE hPipe, uint8_t data);
DWORD Put32Pipe(HANDLE hPipe, uint8_t data);
DWORD Put64Pipe(HANDLE hPipe, uint64_t data);

inline void ttas_spinlock_lock(volatile bool *lock){
    uint16_t test_counter = 0;
    while (true){
        if (*lock == 0){
            test_counter = 0;
            if (_InterlockedCompareExchange8((volatile char*)lock, 1, 0) == 0){
                MemoryBarrier();
                return;
            }
        }
        else{
            if ((test_counter & 0xFFFF) == 0xFFFF){
                Sleep(0);
            }
            else{
                test_counter++;
            }
        }
    }
}

inline void ttas_spinlock_unlock(volatile bool *lock){
    *lock = 0;
    MemoryBarrier();
    return;
}

inline uint64_t _rol64(uint64_t v, uint64_t s){
    return (v << s) | (v >> (64 - s));
}

#endif //__UTILS_H__
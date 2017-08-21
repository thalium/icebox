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
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <Windows.h>


#include "FDP.h"
#include "WindowsProfils.h"
#include "kdserver.h"
#include "mmu.h"
#include "utils.h"


bool WDBG_SearchVirtualMemory(WINBAGILITY_CONTEXT_T *pWinbagilityCtx, uint8_t *pPatternData, uint32_t PatternSize, uint64_t StartVirtualAddress, uint64_t EndOffset, uint64_t *pFoundVirtualAddress)
{
    uint64_t curOffset = 0;
    uint8_t TempBuffer[PAGE_SIZE];
    uint64_t leftToLook = EndOffset - curOffset;
    while (leftToLook){
        if (pWinbagilityCtx->pfnReadVirtualMemory(pWinbagilityCtx->pUserHandle, 0, TempBuffer, PAGE_SIZE, StartVirtualAddress + curOffset)){
            for (int i = 0; i < MIN(PAGE_SIZE - PatternSize, leftToLook); i++){
                if (memcmp(TempBuffer + i, pPatternData, PatternSize) == 0){
                    *pFoundVirtualAddress = StartVirtualAddress + curOffset + i;
                    return true;
                }
            }
        }
        curOffset = curOffset + MIN(leftToLook, PAGE_SIZE - PatternSize);
        leftToLook = EndOffset - curOffset;
    }
    *pFoundVirtualAddress = 0;
    return false;
}
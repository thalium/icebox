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


typedef struct CRASH_TYPE_T_
{
    HANDLE      hFile;
    uint64_t    uFileSize;
    uint8_t*    pFileData;
    uint64_t    Cr3Value;
    uint64_t    LSTARValue;
    uint64_t    v_KPCR;
    uint64_t    GDTRB;
    uint64_t    IDTRB;
    KTRAP_FRAME TrapFrame;
}CRASH_TYPE_T;


CRASH_TYPE_T* CRASH_Open(char *pFileName);
bool CRASH_Init(CRASH_TYPE_T *pUserHandle);
bool CRASH_GetPhysicalMemorySize(CRASH_TYPE_T *pUserHandle, uint64_t *pPhysicalMemorySize);
bool CRASH_ReadRegister(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, uint16_t RegisterId, uint64_t *pRegisterValue);
bool CRASH_ReadMsr(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, uint32_t MsrId, uint64_t *pMSRValue);
bool CRASH_ReadPhysicalMemory(CRASH_TYPE_T* pUserHandle, uint8_t *pDstBuffer, uint32_t ReadSize, uint64_t PhysicalAddress);
bool CRASH_ReadVirtualMemory(CRASH_TYPE_T* pUserHandle,
    uint32_t CpuId,
    uint8_t *pDstBuffer,
    uint32_t ReadSize,
    uint64_t VirtualAddress);
bool CRASH_Pause(CRASH_TYPE_T *pUserHandle);
bool CRASH_Resume(CRASH_TYPE_T *pUserHandle);
bool CRASH_SingleStep(CRASH_TYPE_T *pUserHandle, uint32_t CpuId);
bool CRASH_Pause(CRASH_TYPE_T *pUserHandle);
bool CRASH_WriteRegister(CRASH_TYPE_T *, uint32_t CpuId, uint16_t RegisterId, uint64_t RegisterValue);
bool CRASH_UnsetBreakpoint(CRASH_TYPE_T *, uint8_t BreakPointId);
bool CRASH_GetFxState64(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64);
bool CRASH_SetFxState64(CRASH_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64);
bool CRASH_WriteRegister(CRASH_TYPE_T *, uint32_t CpuId, uint16_t RegisterId, uint64_t RegisterValue);
bool CRASH_SingleStep(CRASH_TYPE_T *, uint32_t CpuId);
bool CRASH_WritePhysicalMemory(CRASH_TYPE_T* pUserHandle, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t PhysicalAddress);
bool CRASH_WriteVirtuallMemory(CRASH_TYPE_T* pUserHandle, uint32_t CpuId, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t VirtualAddr);
int  CRASH_SetBreakpoint(CRASH_TYPE_T* pUserHandle, uint32_t CpuId, FDP_BreakpointType BreakpointType, uint8_t BreakpointId, FDP_Access BreakpointAccessType, FDP_AddressType BreakpointAddressType, uint64_t BreakpointAddress, uint64_t BreakpointLength);

bool CRASH_GetCpuState(CRASH_TYPE_T* pUserHandle, uint32_t CpuId, uint16_t *pState);
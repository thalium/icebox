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


typedef struct LIVEKADAY_TYPE_T_
{
    HANDLE      hDevice;
    uint64_t    Cr3Value;
    uint64_t    uFileSize;
    uint64_t    v_KPCR;
    uint64_t    Idtr;
    uint64_t    Gdtr;
    KTRAP_FRAME TrapFrame;
}
LIVEKADAY_TYPE_T;


LIVEKADAY_TYPE_T* LIVEKADAY_Open(char *pFileName);
bool LIVEKADAY_Init(LIVEKADAY_TYPE_T *pUserHandle);
bool LIVEKADAY_GetPhysicalMemorySize(LIVEKADAY_TYPE_T *pUserHandle, uint64_t *pPhysicalMemorySize);
bool LIVEKADAY_ReadRegister(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, uint16_t RegisterId, uint64_t *pRegisterValue);
bool LIVEKADAY_ReadMsr(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, uint32_t MsrId, uint64_t *pMSRValue);
bool LIVEKADAY_ReadPhysicalMemory(LIVEKADAY_TYPE_T* pUserHandle, uint8_t *pDstBuffer, uint32_t ReadSize, uint64_t PhysicalAddress);
bool LIVEKADAY_ReadVirtualMemory(LIVEKADAY_TYPE_T* pUserHandle,
    uint32_t CpuId,
    uint8_t *pDstBuffer,
    uint32_t ReadSize,
    uint64_t VirtualAddress);
bool LIVEKADAY_Pause(LIVEKADAY_TYPE_T *pUserHandle);
bool LIVEKADAY_Resume(LIVEKADAY_TYPE_T *pUserHandle);
bool LIVEKADAY_SingleStep(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId);
bool LIVEKADAY_Pause(LIVEKADAY_TYPE_T *pUserHandle);
bool LIVEKADAY_WriteRegister(LIVEKADAY_TYPE_T *, uint32_t CpuId, uint16_t RegisterId, uint64_t RegisterValue);
bool LIVEKADAY_UnsetBreakpoint(LIVEKADAY_TYPE_T *, uint8_t BreakPointId);
bool LIVEKADAY_GetFxState64(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64);
bool LIVEKADAY_SetFxState64(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, void *pFxState64);
bool LIVEKADAY_WriteRegister(LIVEKADAY_TYPE_T *, uint32_t CpuId, uint16_t RegisterId, uint64_t RegisterValue);
bool LIVEKADAY_SingleStep(LIVEKADAY_TYPE_T *, uint32_t CpuId);
bool LIVEKADAY_WritePhysicalMemory(LIVEKADAY_TYPE_T* pUserHandle, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t PhysicalAddress);
bool LIVEKADAY_WriteVirtuallMemory(LIVEKADAY_TYPE_T* pUserHandle, uint32_t CpuId, uint8_t *pSrcBuffer, uint32_t WriteSize, uint64_t VirtualAddr);
int  LIVEKADAY_SetBreakpoint(LIVEKADAY_TYPE_T* pUserHandle, uint32_t CpuId, FDP_BreakpointType BreakpointType, uint8_t BreakpointId, FDP_Access BreakpointAccessType, FDP_AddressType BreakpointAddressType, uint64_t BreakpointAddress, uint64_t BreakpointLength);
bool LIVEKADAY_GetCpuState(LIVEKADAY_TYPE_T *pUserHandle, uint32_t CpuId, FDP_State *pState);
from ctypes import *
import struct

FDP_MAX_BREAKPOINT = 256

FDP_NO_CR3      = 0x0

FDP_SOFTHBP     = 0x1
FDP_HARDHBP     = 0x2
FDP_PAGEHBP     = 0x3
FDP_MSRHBP      = 0x4
FDP_CRHBP       = 0x5

FDP_WRONG_ADDRESS       = 0x0
FDP_VIRTUAL_ADDRESS     = 0x01
FDP_PHYSICAL_ADDRESS    = 0x02

FDP_WRONG_BP                 = 0x00
FDP_EXECUTE_BP                 = 0x01
FDP_WRITE_BP                 = 0x02
FDP_READ_BP                 = 0x04
FDP_INSTRUCTION_FETCH_BP     = 0x08

FDP_STATE_PAUSED = 0x1
FDP_STATE_BREAKPOINT_HIT = 0x2
FDP_STATE_DEBUGGER_ALERTED = 0x4
FDP_STATE_HARD_BREAKPOINT_HIT = 0x8


FDP_RAX_REGISTER = 0x0
FDP_RBX_REGISTER = 0x1
FDP_RCX_REGISTER = 0x2
FDP_RDX_REGISTER = 0x3
FDP_R8_REGISTER = 0x4
FDP_R9_REGISTER = 0x5
FDP_R10_REGISTER = 0x6
FDP_R11_REGISTER = 0x7
FDP_R12_REGISTER = 0x8
FDP_R13_REGISTER = 0x9
FDP_R14_REGISTER = 0xa
FDP_R15_REGISTER = 0xb
FDP_RSP_REGISTER = 0xc
FDP_RBP_REGISTER = 0xd
FDP_RSI_REGISTER = 0xe
FDP_RDI_REGISTER = 0xf
FDP_RIP_REGISTER = 0x10
FDP_DR0_REGISTER = 0x11
FDP_DR1_REGISTER = 0x12
FDP_DR2_REGISTER = 0x13
FDP_DR3_REGISTER = 0x14
FDP_DR6_REGISTER = 0x15
FDP_DR7_REGISTER = 0x16
FDP_VDR0_REGISTER = 0x17
FDP_VDR1_REGISTER = 0x18
FDP_VDR2_REGISTER = 0x19
FDP_VDR3_REGISTER = 0x1a
FDP_VDR6_REGISTER = 0x1b
FDP_VDR7_REGISTER = 0x1c
FDP_CS_REGISTER = 0x1d
FDP_DS_REGISTER = 0x1e
FDP_ES_REGISTER = 0x1f
FDP_FS_REGISTER = 0x20
FDP_GS_REGISTER = 0x21
FDP_SS_REGISTER = 0x22
FDP_RFLAGS_REGISTER = 0x23
FDP_MXCSR_REGISTER = 0x24
FDP_GDTRB_REGISTER = 0x25
FDP_GDTRL_REGISTER = 0x26
FDP_IDTRB_REGISTER = 0x27
FDP_IDTRL_REGISTER = 0x28
FDP_CR0_REGISTER = 0x29
FDP_CR2_REGISTER = 0x2a
FDP_CR3_REGISTER = 0x2b
FDP_CR4_REGISTER = 0x2c
FDP_CR8_REGISTER = 0x2d


FDP_4K = 4096
FDP_1M = 1*1024*1024

FDP_CPU0 = 0
FDP_MSR_LSTAR = 0xc0000082

class FDP:
    def __init__(self, Name):
        self.pFDP = 0
        self.fdpdll = cdll.LoadLibrary("FDP_x86.dll")
        self.fdpdll.FDP_SingleStep.restype = c_bool
        self.fdpdll.FDP_Pause.restype = c_bool
        self.fdpdll.FDP_Resume.restype = c_bool
        self.fdpdll.FDP_Save.restype = c_bool
        self.fdpdll.FDP_Restore.restype = c_bool
        self.fdpdll.FDP_GetStateChanged.restype = c_bool
        self.fdpdll.FDP_Reboot.restype = c_bool
        self.fdpdll.FDP_ReadRegister.restype = c_bool
        self.fdpdll.FDP_WriteRegister.restype = c_bool
        self.fdpdll.FDP_ReadMsr.restype = c_bool
        self.fdpdll.FDP_WriteMsr.restype = c_bool
        self.fdpdll.FDP_ReadVirtualMemory.restype = c_bool
        self.fdpdll.FDP_ReadPhysicalMemory.restype = c_bool
        self.fdpdll.FDP_WriteVirtualMemory.restype = c_bool
        self.fdpdll.FDP_WritePhysicalMemory.restype = c_bool
        self.fdpdll.FDP_UnsetBreakpoint.restype = c_bool
        self.fdpdll.FDP_GetState.restype = c_bool
        self.fdpdll.FDP_GetCpuState.restype = c_bool
        self.fdpdll.FDP_GetCpuCount.restype = c_bool
        self.fdpdll.FDP_InjectInterrupt.restype = c_bool
        self.fdpdll.FDP_GetPhysicalMemorySize.restype = c_bool
        
        pName = pointer(create_string_buffer(Name))
        self.pFDP = self.fdpdll.FDP_OpenSHM(pName)
        if self.pFDP == 0:
            return None
        self.fdpdll.FDP_Init(self.pFDP)

    def ReadRegister(self, CpuId, RegisterId):
        pRegisterValue = pointer(c_uint64(0))
        if self.fdpdll.FDP_ReadRegister(self.pFDP, CpuId, RegisterId, pRegisterValue) == True:
            return pRegisterValue[0]
        return None

    def WriteRegister(self, CpuId, RegisterId, RegisterValue):
        return self.fdpdll.FDP_WriteRegister(self.pFDP, CpuId, RegisterId, c_uint64(RegisterValue))
        
    def ReadMsr(self, CpuId, MsrId):
        pMsrValue = pointer(c_uint64(0))
        if self.fdpdll.FDP_ReadMsr(self.pFDP, CpuId, c_uint64(MsrId), pMsrValue) == True:
            return pMsrValue[0]
        return None

    def WriteMsr(self, CpuId, MsrId, MsrValue):
        return self.fdpdll.FDP_WriteMsr(self.pFDP, CpuId, c_uint64(MsrId), c_uint64(MsrValue))
        
    def Pause(self):
        return self.fdpdll.FDP_Pause(self.pFDP)
        
    def Resume(self):
        return self.fdpdll.FDP_Resume(self.pFDP)
        
    def Save(self):
        return self.fdpdll.FDP_Save(self.pFDP)
        
    def Restore(self, ):
        return self.fdpdll.FDP_Restore(self.pFDP)

    def Reboot(self, ):
        return self.fdpdll.FDP_Reboot(self.pFDP)

    def SingleStep(self, CpuId):
        return self.fdpdll.FDP_SingleStep(self.pFDP, CpuId)
        
    def ReadVirtualMemory(self, CpuId, VirtualAddress, ReadSize):
        Buffer = create_string_buffer(int(ReadSize))
        pBuffer = pointer(Buffer)
        if self.fdpdll.FDP_ReadVirtualMemory(self.pFDP, CpuId, pBuffer, ReadSize, c_uint64(VirtualAddress)) == True:
            return Buffer.raw
        return None
        
    def ReadVirtualMemory32(self, CpuId, VirtualAddress):
        Value32 = self.ReadVirtualMemory(FDP_CPU0, VirtualAddress, 4)
        if Value32 == None:
            return None
        return struct.unpack('<L', Value32)[0]
        
    def ReadVirtualMemory64(self, CpuId, VirtualAddress):
        Value64 = self.ReadVirtualMemory(FDP_CPU0, VirtualAddress, 8)
        if Value64 == None:
            return None
        return struct.unpack('<Q', Value64)[0]
        
    def ReadPhysicalMemory(self, PhysicalAddress, ReadSize):
        Buffer = create_string_buffer(int(ReadSize))
        pBuffer = pointer(Buffer)
        if self.fdpdll.FDP_ReadPhysicalMemory(self.pFDP, pBuffer, ReadSize, c_uint64(PhysicalAddress)) == True:
            return Buffer.raw
        return None
        
    def WritePhysicalMemory(self, PhysicalAddress, WriteBuffer):
        Buffer = create_string_buffer(int(WriteBuffer))
        pBuffer = pointer(Buffer)
        return self.fdpdll.FDP_WritePhysicalMemory(self.pFDP, pBuffer, len(WriteBuffer), c_uint64(PhysicalAddress))
        
    def WriteVirtualMemory(self, CpuId, VirtualAddress, WriteBuffer):
        #print "%x => %x" % (VirtualAddress, len(WriteBuffer))
        Buffer = create_string_buffer(WriteBuffer)
        pBuffer = pointer(Buffer)
        return self.fdpdll.FDP_WriteVirtualMemory(self.pFDP, CpuId, pBuffer, len(WriteBuffer), c_uint64(VirtualAddress))
        
    def SetBreakpoint(self, CpuId, BreakpointType, BreakpointId, BreakpointAccessType, BreakpointAddressType, BreakpointAddress, BreakpointLength, BreakpointCr3):
        BreakpointId = self.fdpdll.FDP_SetBreakpoint(self.pFDP, c_uint32(CpuId), c_uint16(BreakpointType), c_uint8(BreakpointId), c_uint16(BreakpointAccessType), c_uint16(BreakpointAddressType), c_uint64(BreakpointAddress), c_uint64(BreakpointLength), c_uint64(BreakpointCr3))
        if BreakpointId >= 0:
            return BreakpointId
        return None
        
    def UnsetBreakpoint(self, BreakpointId):
        return self.fdpdll.FDP_UnsetBreakpoint(self.pFDP, c_uint8(BreakpointId))
        
    def GetState(self):
        pState = pointer(c_uint8(0))
        if self.fdpdll.FDP_GetState(self.pFDP, pState) == True:
            return pState[0]
        return None

    def GetCpuState(self, CpuId):
        pState = pointer(c_uint8(0))
        if self.fdpdll.FDP_GetCpuState(self.pFDP, CpuId, pState) == True:
            return pState[0]
        return None

    def GetPhysicalMemorySize(self):
        pPhysicalMemorySize = pointer(c_uint64(0))
        if self.fdpdll.FDP_GetPhysicalMemorySize(self.pFDP, pPhysicalMemorySize) == True:
            return pPhysicalMemorySize[0]
        return None
        
    def GetCpuCount(self):
        pCpuCount = pointer(c_uint32(0))
        if self.fdpdll.FDP_GetCpuCount(self.pFDP, pCpuCount) == True:
            return pCpuCount[0]
        return None
        
    def GetStateChanged(self):
        return self.fdpdll.FDP_GetStateChanged(self.pFDP)

    def InjectInterrupt(self, CpuId, InterruptionCode, ErrorCode, Cr2Value):
        return self.fdpdll.FDP_InjectInterrupt(self.pFDP, CpuId, InterruptionCode, ErrorCode, c_uint64(Cr2Value))
        
    def UnsetAllBreakpoint(self):
        for i in range(0,FDP_MAX_BREAKPOINT):
            self.UnsetBreakpoint(i)
        return True
    
    def WaitForStateChanged(self):
        while True:
            if self.GetStateChanged() == True:
                return self.GetState()
        return 0
        
    def DumpPhysicalMemory(self, FilePath):
        f = open(FilePath, 'wb')
        PhysicalMemorySize = self.GetPhysicalMemorySize()
        for i in range(0, PhysicalMemorySize/_4K):
            PageContent = fdp.ReadPhysicalMemory(i*_4K, _4K)
            if PageContent == None:
                PageContent = "\x00"*_4K
            f.write(PageContent)
        f.close()

from multiprocessing import Process
import time
import sys


if __name__ == '__main__':
    fdp = FDP("10_RS1_64")
    
    fdp.Pause()
    fdp.UnsetAllBreakpoint()
    KiSystemCall64 = fdp.ReadMsr(FDP_CPU0, FDP_MSR_LSTAR)
    fdp.WriteRegister(FDP_CPU0, FDP_DR7_REGISTER, 0x400)

    #fdp.SetBreakpoint(FDP_CPU0, FDP_PAGEHBP, 0, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, KiSystemCall64, 1, FDP_NO_CR3)
    #fdp.SetBreakpoint(FDP_CPU0, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, KiSystemCall64, 1, FDP_NO_CR3)
    fdp.WriteRegister(FDP_CPU0, FDP_DR0_REGISTER, KiSystemCall64)
    fdp.WriteRegister(FDP_CPU0, FDP_DR7_REGISTER, 0x403)
    fdp.Resume()
    
    SyscallCount = 0
    OldTime = time.time()
    while True:
         if fdp.WaitForStateChanged() & FDP_STATE_BREAKPOINT_HIT:
            #DO STUFF
            SyscallCount = SyscallCount + 1
            if SyscallCount > 100000:
                NewTime = time.time()
                print int(SyscallCount / (NewTime-OldTime))
                OldTime = NewTime
                SyscallCount = 0
            fdp.SingleStep(FDP_CPU0)
            fdp.Resume()

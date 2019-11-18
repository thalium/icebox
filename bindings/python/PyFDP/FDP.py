from ctypes import *
import os
import re

import PyFDP

FDP_REGISTER = [
    {'name':    "FDP_RAX_REGISTER" ,'value': 0x0},
    {'name':    "FDP_RBX_REGISTER" ,'value': 0x1},
    {'name':    "FDP_RCX_REGISTER" ,'value': 0x2},
    {'name':    "FDP_RDX_REGISTER" ,'value': 0x3},
    {'name':    "FDP_R8_REGISTER" ,'value': 0x4},
    {'name':    "FDP_R9_REGISTER" ,'value': 0x5},
    {'name':    "FDP_R10_REGISTER" ,'value': 0x6},
    {'name':    "FDP_R11_REGISTER" ,'value': 0x7},
    {'name':    "FDP_R12_REGISTER" ,'value': 0x8},
    {'name':    "FDP_R13_REGISTER" ,'value': 0x9},
    {'name':    "FDP_R14_REGISTER" ,'value': 0xa},
    {'name':    "FDP_R15_REGISTER" ,'value': 0xb},
    {'name':    "FDP_RSP_REGISTER" ,'value': 0xc},
    {'name':    "FDP_RBP_REGISTER" ,'value': 0xd},
    {'name':    "FDP_RSI_REGISTER" ,'value': 0xe},
    {'name':    "FDP_RDI_REGISTER" ,'value': 0xf},
    {'name':    "FDP_RIP_REGISTER" ,'value': 0x10},
    {'name':    "FDP_DR0_REGISTER" ,'value': 0x11},
    {'name':    "FDP_DR1_REGISTER" ,'value': 0x12},
    {'name':    "FDP_DR2_REGISTER" ,'value': 0x13},
    {'name':    "FDP_DR3_REGISTER" ,'value': 0x14},
    {'name':    "FDP_DR6_REGISTER" ,'value': 0x15},
    {'name':    "FDP_DR7_REGISTER" ,'value': 0x16},
    {'name':    "FDP_VDR0_REGISTER" ,'value': 0x17},
    {'name':    "FDP_VDR1_REGISTER" ,'value': 0x18},
    {'name':    "FDP_VDR2_REGISTER" ,'value': 0x19},
    {'name':    "FDP_VDR3_REGISTER" ,'value': 0x1a},
    {'name':    "FDP_VDR6_REGISTER" ,'value': 0x1b},
    {'name':    "FDP_VDR7_REGISTER" ,'value': 0x1c},
    {'name':    "FDP_CS_REGISTER" ,'value': 0x1d},
    {'name':    "FDP_DS_REGISTER" ,'value': 0x1e},
    {'name':    "FDP_ES_REGISTER" ,'value': 0x1f},
    {'name':    "FDP_FS_REGISTER" ,'value': 0x20},
    {'name':    "FDP_GS_REGISTER" ,'value': 0x21},
    {'name':    "FDP_SS_REGISTER" ,'value': 0x22},
    {'name':    "FDP_RFLAGS_REGISTER" ,'value': 0x23},
    {'name':    "FDP_MXCSR_REGISTER" ,'value': 0x24},
    {'name':    "FDP_GDTRB_REGISTER" ,'value': 0x25},
    {'name':    "FDP_GDTRL_REGISTER" ,'value': 0x26},
    {'name':    "FDP_IDTRB_REGISTER" ,'value': 0x27},
    {'name':    "FDP_IDTRL_REGISTER" ,'value': 0x28},
    {'name':    "FDP_CR0_REGISTER" ,'value': 0x29},
    {'name':    "FDP_CR2_REGISTER" ,'value': 0x2a},
    {'name':    "FDP_CR3_REGISTER" ,'value': 0x2b},
    {'name':    "FDP_CR4_REGISTER" ,'value': 0x2c},
    {'name':    "FDP_CR8_REGISTER" ,'value': 0x2d}
]

class FDP(object):
    """ Fast Debug Protocol client object.

    Send requests to a FDP server located in an instrumented VirtualBox implementation.
    """

    FDP_MAX_BREAKPOINT = 1024

    FDP_NO_CR3      = 0x0

    # FDP_BreakpointType
    FDP_SOFTHBP     = 0x1
    FDP_HARDHBP     = 0x2
    FDP_PAGEHBP     = 0x3
    FDP_MSRHBP      = 0x4
    FDP_CRHBP       = 0x5

    # FDP_AddressType
    FDP_WRONG_ADDRESS       = 0x0
    FDP_VIRTUAL_ADDRESS     = 0x01
    FDP_PHYSICAL_ADDRESS    = 0x02

    # FDP_Access
    FDP_WRONG_BP                 = 0x00
    FDP_EXECUTE_BP                 = 0x01
    FDP_WRITE_BP                 = 0x02
    FDP_READ_BP                 = 0x04
    FDP_INSTRUCTION_FETCH_BP     = 0x08

    # FDP_State
    FDP_STATE_PAUSED = 0x1
    FDP_STATE_BREAKPOINT_HIT = 0x2
    FDP_STATE_DEBUGGER_ALERTED = 0x4
    FDP_STATE_HARD_BREAKPOINT_HIT = 0x8

    FDP_CPU0 = 0

    def __init__(self, Name):

        self.pFDP = 0
        self.fdpdll = PyFDP.FDP_DLL_HANDLE

        FDP_Register = c_uint32
        FDP_BreakpointType = c_uint16
        FDP_Access = c_uint16
        FDP_AddressType = c_uint16
        FDP_State = c_uint8

        self.pRegisterValue = pointer(c_uint64(0))
        self.pMsrValue = pointer(c_uint64(0))
        self.pState = pointer(c_uint8(0))

        self.fdpdll.FDP_CreateSHM.restype = c_void_p
        self.fdpdll.FDP_CreateSHM.argtypes = [c_char_p]
        self.fdpdll.FDP_OpenSHM.restype = c_void_p
        self.fdpdll.FDP_OpenSHM.argtypes = [c_char_p]
        self.fdpdll.FDP_Init.restype = c_bool
        self.fdpdll.FDP_Init.argtypes = [c_void_p]
        self.fdpdll.FDP_Pause.restype = c_bool
        self.fdpdll.FDP_Pause.argtypes = [c_void_p]
        self.fdpdll.FDP_Resume.restype = c_bool
        self.fdpdll.FDP_Resume.argtypes = [c_void_p]
        self.fdpdll.FDP_ReadPhysicalMemory.restype = c_bool
        self.fdpdll.FDP_ReadPhysicalMemory.argtypes = [c_void_p, POINTER(c_uint8), c_uint32, c_uint64]
        self.fdpdll.FDP_WritePhysicalMemory.restype = c_bool
        self.fdpdll.FDP_WritePhysicalMemory.argtypes = [c_void_p, POINTER(c_uint8), c_uint32, c_uint64]
        self.fdpdll.FDP_ReadVirtualMemory.restype = c_bool
        self.fdpdll.FDP_ReadVirtualMemory.argtypes = [c_void_p, c_uint32, POINTER(c_uint8), c_uint32, c_uint64]
        self.fdpdll.FDP_WriteVirtualMemory.restype = c_bool
        self.fdpdll.FDP_WriteVirtualMemory.argtypes = [c_void_p, c_uint32, POINTER(c_uint8), c_uint32, c_uint64]
        self.fdpdll.FDP_SearchPhysicalMemory.restype = c_uint64
        self.fdpdll.FDP_SearchPhysicalMemory.argtypes = [c_void_p, c_void_p, c_uint32, c_uint64]
        self.fdpdll.FDP_SearchVirtualMemory.restype = c_bool
        self.fdpdll.FDP_SearchVirtualMemory.argtypes = [c_void_p, c_uint32, c_void_p, c_uint32, c_uint64]
        self.fdpdll.FDP_ReadRegister.restype = c_bool
        self.fdpdll.FDP_ReadRegister.argtypes = [c_void_p, c_uint32, FDP_Register, POINTER(c_uint64)]
        self.fdpdll.FDP_WriteRegister.restype = c_bool
        self.fdpdll.FDP_WriteRegister.argtypes = [c_void_p, c_uint32, FDP_Register, c_uint64]
        self.fdpdll.FDP_ReadMsr.restype = c_bool
        self.fdpdll.FDP_ReadMsr.argtypes = [c_void_p, c_uint32, c_uint64, POINTER(c_uint64)]
        self.fdpdll.FDP_WriteMsr.restype = c_bool
        self.fdpdll.FDP_WriteMsr.argtypes = [c_void_p, c_uint32, c_uint64, c_uint64]
        self.fdpdll.FDP_SetBreakpoint.restype = c_int
        self.fdpdll.FDP_SetBreakpoint.argtypes = [c_void_p, c_uint32, FDP_BreakpointType, c_uint8, FDP_Access, FDP_AddressType, c_uint64, c_uint64, c_uint64]
        self.fdpdll.FDP_UnsetBreakpoint.restype = c_bool
        self.fdpdll.FDP_UnsetBreakpoint.argtypes = [c_void_p, c_uint8]
        self.fdpdll.FDP_VirtualToPhysical.restype = c_bool
        self.fdpdll.FDP_VirtualToPhysical.argtypes = [c_void_p, c_uint32, c_uint64, POINTER(c_uint64)]
        self.fdpdll.FDP_GetState.restype = c_bool
        self.fdpdll.FDP_GetState.argtypes = [c_void_p, POINTER(FDP_State)]
        self.fdpdll.FDP_GetFxState64.restype = c_bool
        self.fdpdll.FDP_GetFxState64.argtypes = [c_void_p, c_uint32, c_void_p]
        self.fdpdll.FDP_SetFxState64.restype = c_bool
        self.fdpdll.FDP_SetFxState64.argtypes = [c_void_p, c_uint32, c_void_p]
        self.fdpdll.FDP_SingleStep.restype = c_bool
        self.fdpdll.FDP_SingleStep.argtypes = [c_void_p, c_uint32]
        self.fdpdll.FDP_GetPhysicalMemorySize.restype = c_bool
        self.fdpdll.FDP_GetPhysicalMemorySize.argtypes = [c_void_p, POINTER(c_uint64)]
        self.fdpdll.FDP_GetCpuCount.restype = c_bool
        self.fdpdll.FDP_GetCpuCount.argtypes = [c_void_p, POINTER(c_uint32)]
        self.fdpdll.FDP_GetCpuState.restype = c_bool
        self.fdpdll.FDP_GetCpuState.argtypes = [c_void_p, c_uint32, POINTER(FDP_State)]
        self.fdpdll.FDP_Reboot.restype = c_bool
        self.fdpdll.FDP_Reboot.argtypes = [c_void_p]
        self.fdpdll.FDP_Save.restype = c_bool
        self.fdpdll.FDP_Save.argtypes = [c_void_p]
        self.fdpdll.FDP_Restore.restype = c_bool
        self.fdpdll.FDP_Restore.argtypes = [c_void_p]
        self.fdpdll.FDP_GetStateChanged.restype = c_bool
        self.fdpdll.FDP_GetStateChanged.argtypes = [c_void_p]
        self.fdpdll.FDP_SetStateChanged.restype = c_void_p
        self.fdpdll.FDP_SetStateChanged.argtypes = [c_void_p]
        self.fdpdll.FDP_InjectInterrupt.restype = c_bool
        self.fdpdll.FDP_InjectInterrupt.argtypes = [c_void_p, c_uint32, c_uint32, c_uint32, c_uint64]

        pName = cast(pointer(create_string_buffer(Name.encode())), c_char_p)
        self.pFDP = self.fdpdll.FDP_OpenSHM(pName)
        if self.pFDP == 0 or self.pFDP is None:
            raise Exception("SHMError: impossible to connect to {}".format(Name))

        self.fdpdll.FDP_Init(self.pFDP)

        # create registers attributes

        for reg in FDP_REGISTER:
            enum = reg["value"]
            name = self.__fix_names__(reg['name'])
            def get_property(enum):
                def read(self): return self.ReadRegister(enum)
                def write(self, value) : self.WriteRegister(enum, value)
                return property(read, write)
            setattr(FDP, name, get_property(enum))

    def __fix_names__(self, name):
        regx= re.compile(r"FDP_(.*)_REGISTER")
        return re.findall(regx, name)[0].lower()

    def ReadRegister(self, RegisterId, CpuId=FDP_CPU0 ):
        """ Return the value stored in the specified register
        RegisterId must be a member of FDP.FDP_REGISTER
        """
        if self.fdpdll.FDP_ReadRegister(self.pFDP, CpuId, RegisterId, self.pRegisterValue) == True:
            return self.pRegisterValue[0]
        return None

    def WriteRegister(self, RegisterId, RegisterValue, CpuId=FDP_CPU0):
        """ Store the given value into the specified register
        RegisterId must be a member of FDP.FDP_REGISTER
        """
        return self.fdpdll.FDP_WriteRegister(self.pFDP, CpuId, RegisterId, c_uint64(RegisterValue))

    def ReadMsr(self, MsrId, CpuId=FDP_CPU0):
        """ Return the value stored in the Model-specific register (MSR) indexed by MsrId
        MSR typically don't have an enum Id since there are vendor specific.
        """
        if self.fdpdll.FDP_ReadMsr(self.pFDP, CpuId, c_uint64(MsrId), self.pMsrValue) == True:
            return self.pMsrValue[0]
        return None

    def WriteMsr(self, MsrId, MsrValue, CpuId=FDP_CPU0):
        """ Store the value into the Model-specific register (MSR) indexed by MsrId
        MSR typically don't have an enum Id since there are vendor specific.
        """
        return self.fdpdll.FDP_WriteMsr(self.pFDP, CpuId, c_uint64(MsrId), c_uint64(MsrValue))

    def Pause(self):
        """ Suspend the target virtual machine """
        return self.fdpdll.FDP_Pause(self.pFDP)

    def Resume(self):
        """ Resume the target virtual machine execution """
        return self.fdpdll.FDP_Resume(self.pFDP)

    def Save(self):
        """Save the virtual machine state (CPU+memory).
        Only one save state allowed.
        """
        return self.fdpdll.FDP_Save(self.pFDP)

    def Restore(self):
        """ Restore the previously stored virtual machine state (CPU+memory). """
        return self.fdpdll.FDP_Restore(self.pFDP)

    def Reboot(self):
        """ Reboot the target virtual machine """
        return self.fdpdll.FDP_Reboot(self.pFDP)

    def SingleStep(self, CpuId=FDP_CPU0):
        """ Single step a paused execution """
        return self.fdpdll.FDP_SingleStep(self.pFDP, CpuId)

    def ReadVirtualMemory(self, VirtualAddress, ReadSize, CpuId=FDP_CPU0):
        """ Attempt to read a VM virtual memory buffer. 
        Check CR3 to know which process's memory your're reading
        """
        try: 
            Buffer = create_string_buffer(int(ReadSize))
        except(OverflowError): 
            return None

        pBuffer = cast(pointer(Buffer), POINTER(c_uint8))
        if self.fdpdll.FDP_ReadVirtualMemory(self.pFDP, CpuId, pBuffer, ReadSize, c_uint64(VirtualAddress)) == True:
            return Buffer.raw
        return None

    def ReadPhysicalMemory(self, PhysicalAddress, ReadSize):
        """ Attempt to read a VM physical memory buffer. """
        Buffer = create_string_buffer(int(ReadSize))
        pBuffer = pointer(Buffer)
        if self.fdpdll.FDP_ReadPhysicalMemory(self.pFDP, pBuffer, ReadSize, c_uint64(PhysicalAddress)) == True:
            return Buffer.raw
        return None

    def WritePhysicalMemory(self, PhysicalAddress, WriteBuffer):
        """ Attempt to write a buffer at a VM physical memory address. """
        Buffer = create_string_buffer(int(WriteBuffer))
        pBuffer = pointer(Buffer)
        return self.fdpdll.FDP_WritePhysicalMemory(self.pFDP, pBuffer, len(WriteBuffer), c_uint64(PhysicalAddress))

    def WriteVirtualMemory(self, VirtualAddress, WriteBuffer, CpuId=FDP_CPU0):
        """ Attempt to write a buffer at a VM virtual memory address. 
        Check CR3 to know which process's memory your're writing into.
        """
        Buffer = create_string_buffer(WriteBuffer)
        pBuffer = cast(pointer(Buffer), POINTER(c_uint8))
        return self.fdpdll.FDP_WriteVirtualMemory(self.pFDP, CpuId, pBuffer, len(WriteBuffer), c_uint64(VirtualAddress))

    def SetBreakpoint(self, BreakpointType, BreakpointId, BreakpointAccessType, BreakpointAddressType, BreakpointAddress, BreakpointLength, BreakpointCr3, CpuId=FDP_CPU0):
        """ Place a breakpoint. 
        
        * BreakpointType :
            - FDP.FDP_SOFTHBP : "soft" hyperbreakpoint, backed by a shadow "0xcc" isntruction in the VM physical memory page.
            - FDP.FDP_HARDHBP : "hard" hyperbreakpoint, backed by a shadow debug register (only 4)
            - FDP.FDP_PAGEHBP : "page" hyperbreakpoint relying on Extended Page Table (EPT) page guard faults.
            - FDP.FDP_MSRHBP  : "msr" hyperbreakpoint, specifically to read a VM's MSR
            - FDP.FDP_CRHBP  : "cr" hyperbreakpoint, specifically to read a VM's Context Register
        
        * BreakpointId: Currently unused

        * BreakpointAccessType:
            - FDP.FDP_EXECUTE_BP : break on execution
            - FDP.FDP_WRITE_BP : break on write
            - FDP.FDP_READ_BP : break on read
            - FDP.FDP_INSTRUCTION_FETCH_BP : break when fetching instructions before executing

        * BreakpointAddressType:
            - FDP.FDP_VIRTUAL_ADDRESS : VM's virtual addressing
            - FDP.FDP_PHYSICAL_ADDRESS  : VM's physical addressing
    
        * BreakpointAddress: address (virtual or physical) to break execution
        
        * BreakpointLength: Length of the data pointed by BreakpointAddress which trigger the breakpoint (think "ba e 8" style of breakpoint)

        * BreakpointCr3: Filter breakpoint on a specific CR3 value. Mandatory if you want to break on a particular process.
        """

        BreakpointId = self.fdpdll.FDP_SetBreakpoint(self.pFDP, c_uint32(CpuId), c_uint16(BreakpointType), c_uint8(BreakpointId), c_uint16(BreakpointAccessType), c_uint16(BreakpointAddressType), c_uint64(BreakpointAddress), c_uint64(BreakpointLength), c_uint64(BreakpointCr3))
        if BreakpointId >= 0:
            return BreakpointId
        return None

    def UnsetBreakpoint(self, BreakpointId):
        """ Remove the selected breakoint. Return True on success """
        return self.fdpdll.FDP_UnsetBreakpoint(self.pFDP, c_uint8(BreakpointId))

    def GetState(self):
        """ Return the bitfield state of an system execution break (all CPUs considered):

        - FDP.FDP_STATE_PAUSED : the VM is paused.
        - FDP.FDP_STATE_BREAKPOINT_HIT : the execution has hit a soft or page breakpoint
        - FDP.FDP_STATE_DEBUGGER_ALERTED : the VM is in a debuggable state
        - FDP.FDP_STATE_HARD_BREAKPOINT_HIT : the execution has hit a hard breakpoint
        """
        if self.fdpdll.FDP_GetState(self.pFDP, self.pState) == True:
            return self.pState[0]
        return None

    def GetCpuState(self, CpuId=FDP_CPU0):
        """ Return the bitfield state of an execution break for the sprecified CpuId:

        - FDP.FDP_STATE_PAUSED : the VM is paused.
        - FDP.FDP_STATE_BREAKPOINT_HIT : the execution has hit a soft or page breakpoint
        - FDP.FDP_STATE_DEBUGGER_ALERTED : the VM is in a debuggable state
        - FDP.FDP_STATE_HARD_BREAKPOINT_HIT : the execution has hit a hard breakpoint
        """
        if self.fdpdll.FDP_GetCpuState(self.pFDP, CpuId, self.pState) == True:
            return self.pState[0]
        return None

    def GetPhysicalMemorySize(self):
        """ return the target VM physical memory size, or None on failure """
        pPhysicalMemorySize = pointer(c_uint64(0))
        if self.fdpdll.FDP_GetPhysicalMemorySize(self.pFDP, pPhysicalMemorySize) == True:
            return pPhysicalMemorySize[0]
        return None

    def GetCpuCount(self):
        """ return the target VM CPU count, or None on failure """
        pCpuCount = pointer(c_uint32(0))
        if self.fdpdll.FDP_GetCpuCount(self.pFDP, pCpuCount) == True:
            return pCpuCount[0]
        return None

    def GetStateChanged(self):
        """ check if the VM execution state has changed. Useful on resume."""
        return self.fdpdll.FDP_GetStateChanged(self.pFDP)

    def WaitForStateChanged(self):
        """ wait for the VM execution state has change. Useful on when waiting for a breakpoint to hit."""
        if self.fdpdll.FDP_WaitForStateChanged(self.pFDP, self.pState) == True:
            return self.pState[0]
        return None

    def InjectInterrupt(self, InterruptionCode, ErrorCode, Cr2Value, CpuId=FDP_CPU0):
        """ Inject an interruption in the VM execution state.
        
        * InterruptionCode (int) : interrupt code (e.g. 0x0E for a #PF)
        * ErrorCode : the error code for the interruption (e.g. 0x02 for a Write error on a #PF)
        * Cr2Value : typically the address associated with the interruption
        """
        return self.fdpdll.FDP_InjectInterrupt(self.pFDP, CpuId, InterruptionCode, ErrorCode, c_uint64(Cr2Value))

    def UnsetAllBreakpoint(self):
        """ Remove every set breakpionts """
        for i in range(0,FDP.FDP_MAX_BREAKPOINT):
            self.UnsetBreakpoint(i)
        return True

    def WaitForStateChanged(self):
        """ wait for the VM state to change """
        while True:
            if self.GetStateChanged() == True:
                return self.GetState()
        return 0

    def DumpPhysicalMemory(self, FilePath):
        """ Write the whole VM physicai memory to the host disk. Useful for Volatility-like tools."""
        _4K = 4096
        PhysicalMemorySize = self.GetPhysicalMemorySize()

        with open(FilePath, 'wb') as dumped_memory_file:
            for physical_page in range(0, PhysicalMemorySize, _4K):
                PageContent = fdp.ReadPhysicalMemory(physical_page, _4K)
                if PageContent == None:
                    PageContent = b"??"*_4K

                dumped_memory_file.write(PageContent)

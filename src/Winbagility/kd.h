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
#ifndef __KD_H__
#define __KD_H__

#define DBGKD_QUERY_MEMORY_VIRTUAL  0
#define DBGKD_QUERY_MEMORY_PROCESS  0
#define DBGKD_QUERY_MEMORY_SESSION  1
#define DBGKD_QUERY_MEMORY_KERNEL   2

#define DBGKD_QUERY_MEMORY_READ     0x01
#define DBGKD_QUERY_MEMORY_WRITE    0x02
#define DBGKD_QUERY_MEMORY_EXECUTE  0x04
#define DBGKD_QUERY_MEMORY_FIXED    0x08

#define KD_DATA_PACKET                        0x30303030
#define KD_CONTROL_PACKET                    0x69696969
//
// Wait State Change Types
//
#define DbgKdMinimumStateChange             0x00003030
#define DbgKdExceptionStateChange           0x00003030
#define DbgKdLoadSymbolsStateChange         0x00003031
#define DbgKdCommandStringStateChange       0x00003032
#define DbgKdMaximumStateChange             0x00003033

#define DbgKdMinimumManipulate              0x00003130
#define DbgKdReadVirtualMemoryApi           0x00003130
#define DbgKdWriteVirtualMemoryApi          0x00003131
#define DbgKdGetContextApi                  0x00003132
#define DbgKdSetContextApi                  0x00003133
#define DbgKdWriteBreakPointApi             0x00003134
#define DbgKdRestoreBreakPointApi           0x00003135
#define DbgKdContinueApi                    0x00003136
#define DbgKdReadControlSpaceApi            0x00003137
#define DbgKdWriteControlSpaceApi           0x00003138
#define DbgKdReadIoSpaceApi                 0x00003139
#define DbgKdWriteIoSpaceApi                0x0000313A
#define DbgKdRebootApi                      0x0000313B
#define DbgKdContinueApi2                   0x0000313C
#define DbgKdReadPhysicalMemoryApi          0x0000313D
#define DbgKdWritePhysicalMemoryApi         0x0000313E
#define DbgKdQuerySpecialCallsApi           0x0000313F
#define DbgKdSetSpecialCallApi              0x00003140
#define DbgKdClearSpecialCallsApi           0x00003141
#define DbgKdSetInternalBreakPointApi       0x00003142
#define DbgKdGetInternalBreakPointApi       0x00003143
#define DbgKdReadIoSpaceExtendedApi         0x00003144
#define DbgKdWriteIoSpaceExtendedApi        0x00003145
#define DbgKdGetVersionApi                  0x00003146
#define DbgKdWriteBreakPointExApi           0x00003147
#define DbgKdRestoreBreakPointExApi         0x00003148
#define DbgKdCauseBugCheckApi               0x00003149
#define DbgKdSwitchProcessor                0x00003150
#define DbgKdPageInApi                      0x00003151
#define DbgKdReadMachineSpecificRegister    0x00003152
#define DbgKdWriteMachineSpecificRegister   0x00003153
#define OldVlm1                             0x00003154
#define OldVlm2                             0x00003155
#define DbgKdSearchMemoryApi                0x00003156
#define DbgKdGetBusDataApi                  0x00003157
#define DbgKdSetBusDataApi                  0x00003158
#define DbgKdCheckLowMemoryApi              0x00003159
#define DbgKdClearAllInternalBreakpointsApi 0x0000315A
#define DbgKdFillMemoryApi                  0x0000315B
#define DbgKdQueryMemoryApi                 0x0000315C
#define DbgKdSwitchPartition                0x0000315D
#define DbgKdMaximumManipulate              0x0000315E
//New in v8
#define DbgKdGetRegisterApi                 0x0000315F

#define DbgKdPrintStringApi                    0x00003230
#define DbgKdGetStringApi                    0x00003231

#define KD_PACKET_TYPE_MANIP        2
#define KD_PACKET_TYPE_ACK            4
#define KD_PACKET_TYPE_RESEND        5
#define KD_PACKET_TYPE_RESET        6
#define KD_PACKET_TYPE_STATE_CHANGE 7
#define KD_PACKET_TYPE_IO            11

#pragma pack(push)
#pragma warning( disable : 4200 )

/*typedef struct uint128_t_ {
    uint64_t a;
    uint64_t b;
}uint128_t;*/

typedef struct DECLSPEC_ALIGN(16) _DBGKD_CONTINUE2
{
    uint32_t ContinueStatus;
    uint32_t TraceFlag;
    uint64_t Dr7;
    uint64_t CurrentSymbolStart;
    uint64_t CurrentSymbolEnd;
} DBGKD_CONTINUE2, *PDBGKD_CONTINUE2;

//
// Format of data for fnsave/frstor instructions.
//
// This structure is used to store the legacy floating point state.
//
typedef struct _AMD64_LEGACY_SAVE_AREA {
    USHORT ControlWord;
    USHORT Reserved0;
    USHORT StatusWord;
    USHORT Reserved1;
    USHORT TagWord;
    USHORT Reserved2;
    ULONG ErrorOffset;
    USHORT ErrorSelector;
    USHORT ErrorOpcode;
    ULONG DataOffset;
    USHORT DataSelector;
    USHORT Reserved3;
    UCHAR FloatRegisters[8 * 10];
} AMD64_LEGACY_SAVE_AREA, *PAMD64_LEGACY_SAVE_AREA;

typedef struct _AMD64_M128 {
    ULONGLONG Low;
    LONGLONG High;
} AMD64_M128, *PAMD64_M128;


typedef struct _declspec(align(16)) XSAVE_FORMAT64_
{
    uint16_t    ControlWord;
    uint16_t    StatusWord;
    uint8_t     TagWord;
    uint8_t     Reserved1;
    uint16_t    ErrorOpcode;
    uint32_t    ErrorOffset;
    uint16_t    ErrorSelector;
    uint16_t    Reserved2;
    uint32_t    DataOffset;
    uint16_t    DataSelector;
    uint16_t    Reserved3;
    uint32_t    MxCsr;
    uint32_t    MxCsr_Mask;
    uint128_t   FloatRegisters[8];

#if defined(_WIN64)
    uint128_t   XmmRegisters[16];
    uint8_t     Reserved4[96];
#else
    uint128_t   XmmRegisters[8];
    uint8_t     Reserved4[224];
#endif
} XSAVE_FORMAT64;

//TODO: rename
//http://gate.upm.ro/os/LABs/Windows_OS_Internals_Curriculum_Resource_Kit-ACADEMIC/WindowsResearchKernel-WRK/WRK-v1.2/public/internal/sdktools/inc/ntdbg.h
typedef struct DECLSPEC_ALIGN(16) _DBGKD_GET_REGISTER64{
    ULONG64 u[12];


    //
    // Control flags.
    //

    //ULONG ContextFlags;
    //ULONG MxCsr;

    //
    // Segment Registers and processor flags.
    //

    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
    USHORT SegSs;
    ULONG EFlags;

    //
    // Debug registers
    //

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

    //
    // Integer registers.
    //

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

    //
    // Program counter.
    //
    ULONG64 Rip;

    //
    // Floating point state.
    //
    XSAVE_FORMAT64 FltSave;

    //
    // Vector registers.
    //
    uint128_t VectorRegister[26];
    uint64_t VectorControl;
    //
    // Special debug control registers.
    //
    uint64_t DebugControl;
    uint64_t LastBranchToRip;
    uint64_t LastBranchFromRip;
    uint64_t LastExceptionToRip;
    uint64_t LastExceptionFromRip;
    //uint8_t DATA[122*8];

}DBGKD_GET_REGISTER64, *PBGKD_GET_REGISTER64;

typedef struct _DBGKD_RESTORE_BREAKPOINT
{
    ULONG BreakPointHandle;
    UINT8 Unknown[36];
} DBGKD_RESTORE_BREAKPOINT, *PDBGKD_RESTORE_BREAKPOINT;

typedef struct _DBGKD_GET_VERSION_API64
{
    uint16_t    MajorVersion;
    uint16_t    MinorVersion;
    uint16_t    ProtocolVersion;
    uint16_t    Flags;
    uint16_t    MachineType;
    uint8_t        MaxPacketType;
    uint8_t        MaxStateChange;
    uint8_t        MaxManipulate;
    uint8_t        Simulation;
    uint16_t    Unknown1; //0x0000
    uint64_t    KernelImageBase;
    uint64_t    PsLoadedModuleList;
    uint64_t    DebuggerDataList;
    uint64_t    Unknown2; //0xABABABABFDFDFDFD
    uint64_t    Unknown3; //0xABABABABABABABAB
} DBGKD_GET_VERSION_API64, *PDBGKD_GET_VERSION_API64;

typedef struct _DBGKD_WRITE_MEMORY64
{
    UINT64    TargetBaseAddress;
    ULONG    TransferCount;
    ULONG    ActualBytesWritten;
    UINT64    Unknown1; //Don't know... But Useless Windbg is OK, if setted 0x00
    UINT64    Unknown2; //Don't know... But Useless Windbg is OK, if setted 0x00
    UINT64    Unknown3; //Don't know... But Useless Windbg is OK, if setted 0x00
    UINT8    Data[0];
} DBGKD_WRITE_MEMORY64, *PDBGKD_WRITE_MEMORY64;

typedef struct _DBGKD_READ_MEMORY64
{
    UINT64    TargetBaseAddress;
    UINT32    TransferCount;
    UINT32    ActualBytesRead;
    UINT64    Unknown1; //Don't know... But Useless Windbg is OK, if setted 0x00
    UINT64    Unknown2; //Don't know... But Useless Windbg is OK, if setted 0x00
    UINT64    Unknown3; //Don't know... But Useless Windbg is OK, if setted 0x00
    UINT8    Data[0];
} DBGKD_READ_MEMORY64, *PDBGKD_READ_MEMORY64;

//
// query memory
//
typedef struct _DBGKD_QUERY_MEMORY{
    uint64_t    Address;
    uint64_t    Reserved;
    uint32_t    AddressSpace;
    uint32_t    Flags;
}DBGKD_QUERY_MEMORY;


typedef struct DECLSPEC_ALIGN(16) _CONTEXT64_T_{
    uint64_t u;
    //
    // Register parameter home addresses.
    //
    // N.B. These fields are for convience - they could be used to extend the
    // context record in the future.
    //
    uint64_t P1Home;
    uint64_t P2Home;
    uint64_t P3Home;
    uint64_t P4Home;
    uint64_t P5Home;
    uint64_t P6Home;

    //
    // Control flags.
    //
    uint32_t ContextFlags;
    uint32_t MxCsr;
    //
    // Segment Registers and processor flags.
    //
    uint16_t SegCs;
    uint16_t SegDs;
    uint16_t SegEs;
    uint16_t SegFs;
    uint16_t SegGs;
    uint16_t SegSs;
    uint32_t EFlags;
    //
    // Debug registers
    //
    uint64_t Dr0;
    uint64_t Dr1;
    uint64_t Dr2;
    uint64_t Dr3;
    uint64_t Dr6;
    uint64_t Dr7;
    //
    // Integer registers.
    //
    uint64_t Rax;
    uint64_t Rcx;
    uint64_t Rdx;
    uint64_t Rbx;
    uint64_t Rsp;
    uint64_t Rbp;
    uint64_t Rsi;
    uint64_t Rdi;
    uint64_t R8;
    uint64_t R9;
    uint64_t R10;
    uint64_t R11;
    uint64_t R12;
    uint64_t R13;
    uint64_t R14;
    uint64_t R15;

    //
    // Program counter.
    //
    uint64_t Rip;

    //
    // Floating point state.
    //
    XSAVE_FORMAT64 FltSave;

    //
    // Vector registers.
    //
    uint128_t VectorRegister[26];
    uint64_t VectorControl;
    //
    // Special debug control registers.
    //
    uint64_t DebugControl;
    uint64_t LastBranchToRip;
    uint64_t LastBranchFromRip;
    uint64_t LastExceptionToRip;
    uint64_t LastExceptionFromRip;
} CONTEXT64_T;

typedef struct _DBGKD_GET_CONTEXT{
    uint64_t u[4]; //Aligment
    CONTEXT64_T Context;
}DBGKD_GET_CONTEXT;

typedef struct _DBGKD_SEARCH_MEMORY{
    union{
        uint64_t SearchAddress;
        uint64_t FoundAddress;
    };
    uint64_t SearchLength;
    uint32_t PatternLength;
    uint32_t u[5];
    uint8_t Data[0];
} DBGKD_SEARCH_MEMORY, *PDBGKD_SEARCH_MEMORY;

typedef struct _DBGKD_WRITE_BREAKPOINT64{
    uint64_t BreakPointAddress;
    uint32_t BreakPointHandle;
    uint8_t u[32];
}DBGKD_WRITE_BREAKPOINT64;


typedef struct DBGKD_READ_WRITE_MSR{
    uint32_t Msr;
    uint32_t DataValueLow;
    uint32_t DataValueHigh;
}DBGKD_READ_WRITE_MSR;

typedef struct _DBGKD_MANIPULATE_STATE64
{
    UINT32 ApiNumber;
    UINT16 ProcessorLevel;
    UINT16 Processor;
    NTSTATUS ReturnStatus;
    union
    {
        DBGKD_READ_MEMORY64                ReadMemory;
        DBGKD_WRITE_MEMORY64            WriteMemory;
        DBGKD_GET_CONTEXT                GetContext;
        //DBGKD_SET_CONTEXT                SetContext;
        DBGKD_WRITE_BREAKPOINT64        WriteBreakPoint;
        DBGKD_RESTORE_BREAKPOINT        RestoreBreakPoint;
        /*DBGKD_CONTINUE                Continue;*/
        DBGKD_CONTINUE2                    Continue2;
        /*DBGKD_READ_WRITE_IO64            ReadWriteIo;
        DBGKD_READ_WRITE_IO_EXTENDED64    ReadWriteIoExtended;
        DBGKD_QUERY_SPECIAL_CALLS        QuerySpecialCalls;
        DBGKD_SET_SPECIAL_CALL64        SetSpecialCall;
        DBGKD_SET_INTERNAL_BREAKPOINT64 SetInternalBreakpoint;
        DBGKD_GET_INTERNAL_BREAKPOINT64 GetInternalBreakpoint;
        DBGKD_GET_VERSION64                GetVersion64;
        DBGKD_BREAKPOINTEX                BreakPointEx;*/
        DBGKD_READ_WRITE_MSR            ReadWriteMsr;
        DBGKD_SEARCH_MEMORY                SearchMemory;
        /*DBGKD_GET_SET_BUS_DATA        GetSetBusData;
        DBGKD_FILL_MEMORY                FillMemory;*/
        DBGKD_QUERY_MEMORY                QueryMemory;
        /*DBGKD_SWITCH_PARTITION        SwitchPartition;*/
        DBGKD_GET_REGISTER64            GetRegisters;
        DBGKD_GET_VERSION_API64            DbgGetVersion;
        uint8_t                            data[0]; //XXX: for testing
    };
} DBGKD_MANIPULATE_STATE64, *PDBGKD_MANIPULATE_STATE64;

#define EXCEPTION_MAXIMUM_PARAMETERS 15
typedef struct _EXCEPTION_RECORD644
{
    UINT32 ExceptionCode;
    UINT32 ExceptionFlags;
    UINT64 ExceptionRecord;
    UINT64 ExceptionAddress;
    UINT32 NumberParameters;
    UINT32 u1;
    UINT64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD644, *PEXCEPTION_RECORD644;

typedef struct _DBGKM_EXCEPTION64
{
    EXCEPTION_RECORD644 ExceptionRecord;
    UINT32 FirstChance;
} DBGKM_EXCEPTION64, *PDBGKM_EXCEPTION64;

#define DBGKD_MAXSTREAM 16 
typedef struct _AMD64_DBGKD_CONTROL_REPORT
{
    ULONG64 Dr6;
    ULONG64 Dr7;
    ULONG EFlags;
    USHORT InstructionCount;
    USHORT ReportFlags;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegSs;
} AMD64_DBGKD_CONTROL_REPORT, *PAMD64_DBGKD_CONTROL_REPORT;

typedef struct _X86_DBGKD_CONTROL_REPORT
{
    uint32_t Dr6;
    uint32_t Dr7;
    //ULONG EFlags;
    USHORT InstructionCount;
    USHORT ReportFlags;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegSs;
} X86_DBGKD_CONTROL_REPORT, *PX86_DBGKD_CONTROL_REPORT;

typedef struct _DBGKD_WAIT_STATE_CHANGE64_{
    uint32_t    ApiNumber;
    uint16_t    NewState;
    uint16_t    ProcessorId;
    USHORT        ProcessorLevel;
    USHORT        Processor;
    ULONG        NumberProcessors;
    ULONG64        Thread;
    ULONG64        ProgramCounter;
    union{
        DBGKM_EXCEPTION64 Exception;
    };
    AMD64_DBGKD_CONTROL_REPORT ControlReport;
} DBGKD_WAIT_STATE_CHANGE64, *PDBGKD_WAIT_STATE_CHANGE64;

typedef struct KD_PACKET_T_{
    uint32_t Leader;
    uint16_t Type;
    uint16_t Length;
    uint32_t Id;
    uint32_t Checksum;
    union{
        UINT32 ApiNumber;
        DBGKD_MANIPULATE_STATE64 ManipulateState64;
        DBGKD_WAIT_STATE_CHANGE64 StateChange;
        uint8_t data[0];
    };
}KD_PACKET_T;

typedef struct _KDESCRIPTOR_T_{
    uint16_t Pad[3];
    uint16_t Limit;
    uint64_t Base;
}KDESCRIPTOR_T;

typedef struct _KSPECIAL_REGISTERS64_T_{
    uint64_t Cr0;
    uint64_t Cr2;
    uint64_t Cr3;
    uint64_t Cr4;
    uint64_t KernelDr0;
    uint64_t KernelDr1;
    uint64_t KernelDr2;
    uint64_t KernelDr3;
    uint64_t KernelDr6;
    uint64_t KernelDr7;
    KDESCRIPTOR_T Gdtr;
    KDESCRIPTOR_T Idtr;
    uint16_t Tr;
    uint16_t Ldtr;
    uint32_t MxCsr;
    uint64_t DebugControl;
    uint64_t LastBranchToRip;
    uint64_t LastBranchFromRip;
    uint64_t LastExceptionToRip;
    uint64_t LastExceptionFromRip;
    uint64_t Cr8;
    uint64_t MsrGsBase;
    uint64_t MsrGsSwap;
    uint64_t MsrStar;
    uint64_t MsrLStar;
    uint64_t MsrCStar;
    uint64_t MsrSyscallMask;
    uint64_t Xcr0;
} KSPECIAL_REGISTERS64_T;

typedef struct _KPROCESSOR_STATE64
{
    KSPECIAL_REGISTERS64_T SpecialRegisters;
    CONTEXT64_T ContextFrame;
}_KPROCESSOR_STATE64, *P_KPROCESSOR_STATE64;
#pragma pack(pop)

typedef enum KdPacketType{
    KdBreakinPacket,
    KdKdPacket,
    KdErrorPacket,
    KdNoPacket,
}KdPacketType;

typedef struct _KD_HEADER{
    uint16_t Type;
    uint16_t Length;
    uint32_t Id;
    uint32_t Checksum;
} KD_HEADER;

//functions
uint32_t            ChecksumKD(KD_PACKET_T *pkt);
bool                SendKDPacket(HANDLE hPipe, KD_PACKET_T* toSendKDPkt);
DWORD               WriteKDPipe(HANDLE hPipe, KD_PACKET_T *pkt);
KdPacketType        ReadKDPipe(HANDLE hPipe, KD_PACKET_T *pktBuffer);
bool                ParseKDPkt(KD_PACKET_T* pkt);

#endif //__KD_H__
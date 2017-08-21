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
#ifndef _FDP_ENUM_H_
#define _FDP_ENUM_H_

enum FDP_BreakpointType_
{
    FDP_INVHBP = 0x0,
    FDP_SOFTHBP,
    FDP_HARDHBP,
    FDP_PAGEHBP,
    FDP_MSRHBP,
    FDP_CRHBP,
    FDP_BREAKPOINT_ID_HACK = 0xFFFF
};
typedef uint16_t FDP_BreakpointType;


enum FDP_AddressType_
{
    FDP_WRONG_ADDRESS = 0x0,
    FDP_VIRTUAL_ADDRESS = 0x01,
    FDP_PHYSICAL_ADDRESS = 0x02,
    FDP_ADRESSTYPE_HACK = 0xFFFF
};
typedef uint16_t FDP_AddressType;

enum FDP_Access_
{
    FDP_WRONG_BP = 0x0,
    FDP_EXECUTE_BP = 0x01,
    FDP_WRITE_BP = 0x02,
    FDP_READ_BP = 0x04,
    FDP_INSTRUCTION_FETCH_BP = 0x08,
    FDBP_ACCESS_HACK = 0xFFFF,
};
typedef uint16_t FDP_Access;

enum FDP_State_
{
    FDP_STATE_NULL = 0x0,
    FDP_STATE_PAUSED = 0x1,
    FDP_STATE_BREAKPOINT_HIT = 0x2,
    FDP_STATE_DEBUGGER_ALERTED = 0x4,
    FDP_STATE_HARD_BREAKPOINT_HIT = 0x8,
    FDP_STATE_HACK = 0xFFFF
};
typedef uint16_t FDP_State;

enum FDP_Register_ {
    FDP_RAX_REGISTER = 0x0,
    FDP_RBX_REGISTER,
    FDP_RCX_REGISTER,
    FDP_RDX_REGISTER,
    FDP_R8_REGISTER,
    FDP_R9_REGISTER,
    FDP_R10_REGISTER,
    FDP_R11_REGISTER,
    FDP_R12_REGISTER,
    FDP_R13_REGISTER,
    FDP_R14_REGISTER,
    FDP_R15_REGISTER,
    FDP_RSP_REGISTER,
    FDP_RBP_REGISTER,
    FDP_RSI_REGISTER,
    FDP_RDI_REGISTER,
    FDP_RIP_REGISTER,
    FDP_DR0_REGISTER,
    FDP_DR1_REGISTER,
    FDP_DR2_REGISTER,
    FDP_DR3_REGISTER,
    FDP_DR6_REGISTER,
    FDP_DR7_REGISTER,
    FDP_VDR0_REGISTER,
    FDP_VDR1_REGISTER,
    FDP_VDR2_REGISTER,
    FDP_VDR3_REGISTER,
    FDP_VDR6_REGISTER,
    FDP_VDR7_REGISTER,
    FDP_CS_REGISTER,
    FDP_DS_REGISTER,
    FDP_ES_REGISTER,
    FDP_FS_REGISTER,
    FDP_GS_REGISTER,
    FDP_SS_REGISTER,
    FDP_RFLAGS_REGISTER,
    FDP_MXCSR_REGISTER,
    FDP_GDTRB_REGISTER,
    FDP_GDTRL_REGISTER,
    FDP_IDTRB_REGISTER,
    FDP_IDTRL_REGISTER,
    FDP_CR0_REGISTER,
    FDP_CR2_REGISTER,
    FDP_CR3_REGISTER,
    FDP_CR4_REGISTER,
    FDP_CR8_REGISTER,
    FDP_LDTR_REGISTER,
    FDP_LDTRB_REGISTER,
    FDP_LDTRL_REGISTER,
    FDP_TR_REGISTER,
    FDP_REGISTER_UINT16_TRICK = 0xFFFF
};
typedef uint16_t FDP_Register;

#endif

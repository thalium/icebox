/* $Id: IOMRC.cpp $ */
/** @file
 * IOM - Input / Output Monitor - Raw-Mode Context.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_IOM
#include <VBox/vmm/iom.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/trpm.h>
#include "IOMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/string.h>


/**
 * Converts disassembler mode to IEM mode.
 * @return IEM CPU mode.
 * @param  enmDisMode   Disassembler CPU mode.
 */
DECLINLINE(IEMMODE) iomDisModeToIemMode(DISCPUMODE enmDisMode)
{
    switch (enmDisMode)
    {
        case DISCPUMODE_16BIT: return IEMMODE_16BIT;
        case DISCPUMODE_32BIT: return IEMMODE_32BIT;
        case DISCPUMODE_64BIT: return IEMMODE_64BIT;
        default:
            AssertFailed();
            return IEMMODE_32BIT;
    }
}


/**
 * IN <AL|AX|EAX>, <DX|imm16>
 *
 * @returns Strict VBox status code. Informational status codes other than the one documented
 *          here are to be treated as internal failure. Use IOM_SUCCESS() to check for success.
 * @retval  VINF_SUCCESS                Success.
 * @retval  VINF_EM_FIRST-VINF_EM_LAST  Success with some exceptions (see IOM_SUCCESS()), the
 *                                      status code must be passed on to EM.
 * @retval  VINF_IOM_R3_IOPORT_READ     Defer the read to ring-3. (R0/GC only)
 * @retval  VINF_EM_RAW_GUEST_TRAP      The exception was left pending. (TRPMRaiseXcptErr)
 * @retval  VINF_TRPM_XCPT_DISPATCHED   The exception was raised and dispatched for raw-mode execution. (TRPMRaiseXcptErr)
 * @retval  VINF_EM_RESCHEDULE_REM      The exception was dispatched and cannot be executed in raw-mode. (TRPMRaiseXcptErr)
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pRegFrame   Pointer to CPUMCTXCORE guest registers structure.
 * @param   pCpu        Disassembler CPU state.
 */
static VBOXSTRICTRC iomRCInterpretIN(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pCpu)
{
    STAM_COUNTER_INC(&pVM->iom.s.StatInstIn); RT_NOREF_PV(pVM);
    Assert(pCpu->Param2.fUse & (DISUSE_IMMEDIATE8 | DISUSE_REG_GEN16));
    uint16_t u16Port = pCpu->Param2.fUse & DISUSE_REG_GEN16 ? pRegFrame->dx : (uint16_t)pCpu->Param2.uValue;

    Assert(pCpu->Param1.fUse & (DISUSE_REG_GEN32 | DISUSE_REG_GEN16 | DISUSE_REG_GEN8));
    uint8_t cbValue = pCpu->Param1.fUse & DISUSE_REG_GEN32 ? 4 : pCpu->Param1.fUse & DISUSE_REG_GEN16 ? 2 : 1;

    return IEMExecDecodedIn(pVCpu, pCpu->cbInstr, u16Port, cbValue);
}


/**
 * OUT <DX|imm16>, <AL|AX|EAX>
 *
 * @returns Strict VBox status code. Informational status codes other than the one documented
 *          here are to be treated as internal failure. Use IOM_SUCCESS() to check for success.
 * @retval  VINF_SUCCESS                Success.
 * @retval  VINF_EM_FIRST-VINF_EM_LAST  Success with some exceptions (see IOM_SUCCESS()), the
 *                                      status code must be passed on to EM.
 * @retval  VINF_IOM_R3_IOPORT_WRITE    Defer the write to ring-3. (R0/GC only)
 * @retval  VINF_IOM_R3_IOPORT_COMMIT_WRITE Defer the write to ring-3. (R0/GC only)
 * @retval  VINF_EM_RAW_GUEST_TRAP      The exception was left pending. (TRPMRaiseXcptErr)
 * @retval  VINF_TRPM_XCPT_DISPATCHED   The exception was raised and dispatched for raw-mode execution. (TRPMRaiseXcptErr)
 * @retval  VINF_EM_RESCHEDULE_REM      The exception was dispatched and cannot be executed in raw-mode. (TRPMRaiseXcptErr)
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pRegFrame   Pointer to CPUMCTXCORE guest registers structure.
 * @param   pCpu        Disassembler CPU state.
 */
static VBOXSTRICTRC iomRCInterpretOUT(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pCpu)
{
    STAM_COUNTER_INC(&pVM->iom.s.StatInstOut); RT_NOREF_PV(pVM);
    Assert(pCpu->Param1.fUse & (DISUSE_IMMEDIATE8 | DISUSE_REG_GEN16));
    uint16_t const u16Port = pCpu->Param1.fUse & DISUSE_REG_GEN16 ? pRegFrame->dx : (uint16_t)pCpu->Param1.uValue;

    Assert(pCpu->Param2.fUse & (DISUSE_REG_GEN32 | DISUSE_REG_GEN16 | DISUSE_REG_GEN8));
    uint8_t const cbValue = pCpu->Param2.fUse & DISUSE_REG_GEN32 ? 4 : pCpu->Param2.fUse & DISUSE_REG_GEN16 ? 2 : 1;

    return IEMExecDecodedOut(pVCpu, pCpu->cbInstr, u16Port, cbValue);
}


/**
 * [REP*] INSB/INSW/INSD
 * ES:EDI,DX[,ECX]
 *
 * @returns Strict VBox status code. Informational status codes other than the one documented
 *          here are to be treated as internal failure. Use IOM_SUCCESS() to check for success.
 * @retval  VINF_SUCCESS                Success.
 * @retval  VINF_EM_FIRST-VINF_EM_LAST  Success with some exceptions (see IOM_SUCCESS()), the
 *                                      status code must be passed on to EM.
 * @retval  VINF_IOM_R3_IOPORT_READ     Defer the read to ring-3. (R0/GC only)
 * @retval  VINF_EM_RAW_EMULATE_INSTR   Defer the read to the REM.
 * @retval  VINF_EM_RAW_GUEST_TRAP      The exception was left pending. (TRPMRaiseXcptErr)
 * @retval  VINF_TRPM_XCPT_DISPATCHED   The exception was raised and dispatched for raw-mode execution. (TRPMRaiseXcptErr)
 * @retval  VINF_EM_RESCHEDULE_REM      The exception was dispatched and cannot be executed in raw-mode. (TRPMRaiseXcptErr)
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCpu        Disassembler CPU state.
 */
static VBOXSTRICTRC iomRCInterpretINS(PVMCPU pVCpu, PDISCPUSTATE pCpu)
{
    uint8_t cbValue = pCpu->pCurInstr->uOpcode == OP_INSB ? 1
                    : pCpu->uOpMode == DISCPUMODE_16BIT ? 2 : 4;       /* dword in both 32 & 64 bits mode */
    return IEMExecStringIoRead(pVCpu,
                               cbValue,
                               iomDisModeToIemMode((DISCPUMODE)pCpu->uCpuMode),
                               RT_BOOL(pCpu->fPrefix & (DISPREFIX_REPNE | DISPREFIX_REP)),
                               pCpu->cbInstr,
                               false /*fIoChecked*/);
}


/**
 * [REP*] OUTSB/OUTSW/OUTSD
 * DS:ESI,DX[,ECX]
 *
 * @returns Strict VBox status code. Informational status codes other than the one documented
 *          here are to be treated as internal failure. Use IOM_SUCCESS() to check for success.
 * @retval  VINF_SUCCESS                Success.
 * @retval  VINF_EM_FIRST-VINF_EM_LAST  Success with some exceptions (see IOM_SUCCESS()), the
 *                                      status code must be passed on to EM.
 * @retval  VINF_IOM_R3_IOPORT_WRITE    Defer the write to ring-3. (R0/GC only)
 * @retval  VINF_IOM_R3_IOPORT_COMMIT_WRITE    Defer the write to ring-3. (R0/GC only)
 * @retval  VINF_EM_RAW_EMULATE_INSTR   Defer the write to the REM.
 * @retval  VINF_EM_RAW_GUEST_TRAP      The exception was left pending. (TRPMRaiseXcptErr)
 * @retval  VINF_TRPM_XCPT_DISPATCHED   The exception was raised and dispatched for raw-mode execution. (TRPMRaiseXcptErr)
 * @retval  VINF_EM_RESCHEDULE_REM      The exception was dispatched and cannot be executed in raw-mode. (TRPMRaiseXcptErr)
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCpu        Disassembler CPU state.
 */
static VBOXSTRICTRC iomRCInterpretOUTS(PVMCPU pVCpu, PDISCPUSTATE pCpu)
{
    uint8_t cbValue = pCpu->pCurInstr->uOpcode == OP_OUTSB ? 1
                    : pCpu->uOpMode == DISCPUMODE_16BIT ? 2 : 4;       /* dword in both 32 & 64 bits mode */
    return IEMExecStringIoWrite(pVCpu,
                                cbValue,
                                iomDisModeToIemMode((DISCPUMODE)pCpu->uCpuMode),
                                RT_BOOL(pCpu->fPrefix & (DISPREFIX_REPNE | DISPREFIX_REP)),
                                pCpu->cbInstr,
                                pCpu->fPrefix & DISPREFIX_SEG ? pCpu->idxSegPrefix : X86_SREG_DS,
                                false /*fIoChecked*/);
}



/**
 * Attempts to service an IN/OUT instruction.
 *
 * The \#GP trap handler in RC will call this function if the opcode causing
 * the trap is a in or out type instruction. (Call it indirectly via EM that
 * is.)
 *
 * @returns Strict VBox status code. Informational status codes other than the one documented
 *          here are to be treated as internal failure. Use IOM_SUCCESS() to check for success.
 * @retval  VINF_SUCCESS                Success.
 * @retval  VINF_EM_FIRST-VINF_EM_LAST  Success with some exceptions (see IOM_SUCCESS()), the
 *                                      status code must be passed on to EM.
 * @retval  VINF_EM_RESCHEDULE_REM      The exception was dispatched and cannot be executed in raw-mode. (TRPMRaiseXcptErr)
 * @retval  VINF_EM_RAW_EMULATE_INSTR   Defer the read to the REM.
 * @retval  VINF_IOM_R3_IOPORT_READ     Defer the read to ring-3.
 * @retval  VINF_EM_RAW_GUEST_TRAP      The exception was left pending. (TRPMRaiseXcptErr)
 * @retval  VINF_TRPM_XCPT_DISPATCHED   The exception was raised and dispatched for raw-mode execution. (TRPMRaiseXcptErr)
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pRegFrame   Pointer to CPUMCTXCORE guest registers structure.
 * @param   pCpu        Disassembler CPU state.
 */
VMMRCDECL(VBOXSTRICTRC) IOMRCIOPortHandler(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pCpu)
{
    switch (pCpu->pCurInstr->uOpcode)
    {
        case OP_IN:
            return iomRCInterpretIN(pVM, pVCpu, pRegFrame, pCpu);

        case OP_OUT:
            return iomRCInterpretOUT(pVM, pVCpu, pRegFrame, pCpu);

        case OP_INSB:
        case OP_INSWD:
            return iomRCInterpretINS(pVCpu, pCpu);

        case OP_OUTSB:
        case OP_OUTSWD:
            return iomRCInterpretOUTS(pVCpu, pCpu);

        /*
         * The opcode wasn't know to us, freak out.
         */
        default:
            AssertMsgFailed(("Unknown I/O port access opcode %d.\n", pCpu->pCurInstr->uOpcode));
            return VERR_IOM_IOPORT_UNKNOWN_OPCODE;
    }
}


/* $Id: IEMAllInstructions.cpp.h $ */
/** @file
 * IEM - Instruction Decoding and Emulation.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
extern const PFNIEMOP g_apfnOneByteMap[256]; /* not static since we need to forward declare it. */

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4702) /* Unreachable code like return in iemOp_Grp6_lldt. */
#endif


/**
 * Common worker for instructions like ADD, AND, OR, ++ with a byte
 * memory/register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rm_r8, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);

        IEM_MC_FETCH_GREG_U8(u8Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
        IEM_MC_REF_GREG_U8(pu8Dst, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're accessing memory.
         * Note! We're putting the eflags on the stack here so we can commit them
         *       after the memory.
         */
        uint32_t const fAccess = pImpl->pfnLockedU8 ? IEM_ACCESS_DATA_RW : IEM_ACCESS_DATA_R; /* CMP,TEST */
        IEM_MC_BEGIN(3, 2);
        IEM_MC_ARG(uint8_t *,  pu8Dst,           0);
        IEM_MC_ARG(uint8_t,    u8Src,            1);
        IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
        IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        if (!pImpl->pfnLockedU8)
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_MEM_MAP(pu8Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
        IEM_MC_FETCH_GREG_U8(u8Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
        IEM_MC_FETCH_EFLAGS(EFlags);
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);
        else
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_MEM_COMMIT_AND_UNMAP(pu8Dst, fAccess);
        IEM_MC_COMMIT_EFLAGS(EFlags);
        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for word/dword/qword instructions like ADD, AND, OR, ++ with
 * memory/register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rm_rv, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U16(u16Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_GREG_U16(pu16Dst, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U32(u32Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_GREG_U32(pu32Dst, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                if (pImpl != &g_iemAImpl_test)
                    IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U64(u64Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_GREG_U64(pu64Dst, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    else
    {
        /*
         * We're accessing memory.
         * Note! We're putting the eflags on the stack here so we can commit them
         *       after the memory.
         */
        uint32_t const fAccess = pImpl->pfnLockedU8 ? IEM_ACCESS_DATA_RW : IEM_ACCESS_DATA_R /* CMP,TEST */;
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint16_t *, pu16Dst,          0);
                IEM_MC_ARG(uint16_t,   u16Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                if (!pImpl->pfnLockedU16)
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu16Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U16(u16Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu16Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint32_t *, pu32Dst,          0);
                IEM_MC_ARG(uint32_t,   u32Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                if (!pImpl->pfnLockedU32)
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu32Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U32(u32Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu32Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 2);
                IEM_MC_ARG(uint64_t *, pu64Dst,          0);
                IEM_MC_ARG(uint64_t,   u64Src,           1);
                IEM_MC_ARG_LOCAL_EFLAGS(pEFlags, EFlags, 2);
                IEM_MC_LOCAL(RTGCPTR, GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                if (!pImpl->pfnLockedU64)
                    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_MEM_MAP(pu64Dst, fAccess, pVCpu->iem.s.iEffSeg, GCPtrEffDst, 0 /*arg*/);
                IEM_MC_FETCH_GREG_U64(u64Src, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_FETCH_EFLAGS(EFlags);
                if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);
                else
                    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnLockedU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_MEM_COMMIT_AND_UNMAP(pu64Dst, fAccess);
                IEM_MC_COMMIT_EFLAGS(EFlags);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for byte instructions like ADD, AND, OR, ++ with a register as
 * the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_r8_rm, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_BEGIN(3, 0);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);

        IEM_MC_FETCH_GREG_U8(u8Src, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
        IEM_MC_REF_GREG_U8(pu8Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    else
    {
        /*
         * We're accessing memory.
         */
        IEM_MC_BEGIN(3, 1);
        IEM_MC_ARG(uint8_t *,  pu8Dst,  0);
        IEM_MC_ARG(uint8_t,    u8Src,   1);
        IEM_MC_ARG(uint32_t *, pEFlags, 2);
        IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_FETCH_MEM_U8(u8Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
        IEM_MC_REF_GREG_U8(pu8Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
        IEM_MC_REF_EFLAGS(pEFlags);
        IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for word/dword/qword instructions like ADD, AND, OR, ++ with a
 * register as the destination.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rv_rm, PCIEMOPBINSIZES, pImpl)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);

    /*
     * If rm is denoting a register, no more instruction bytes.
     */
    if ((bRm & X86_MODRM_MOD_MASK) == (3 << X86_MODRM_MOD_SHIFT))
    {
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U16(u16Src, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
                IEM_MC_REF_GREG_U16(pu16Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U32(u32Src, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
                IEM_MC_REF_GREG_U32(pu32Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 0);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);

                IEM_MC_FETCH_GREG_U64(u64Src, (bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB);
                IEM_MC_REF_GREG_U64(pu64Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    else
    {
        /*
         * We're accessing memory.
         */
        switch (pVCpu->iem.s.enmEffOpSize)
        {
            case IEMMODE_16BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint16_t *, pu16Dst, 0);
                IEM_MC_ARG(uint16_t,   u16Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U16(u16Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U16(pu16Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_32BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint32_t *, pu32Dst, 0);
                IEM_MC_ARG(uint32_t,   u32Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U32(u32Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U32(pu32Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;

            case IEMMODE_64BIT:
                IEM_MC_BEGIN(3, 1);
                IEM_MC_ARG(uint64_t *, pu64Dst, 0);
                IEM_MC_ARG(uint64_t,   u64Src,  1);
                IEM_MC_ARG(uint32_t *, pEFlags, 2);
                IEM_MC_LOCAL(RTGCPTR,  GCPtrEffDst);

                IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffDst, bRm, 0);
                IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
                IEM_MC_FETCH_MEM_U64(u64Src, pVCpu->iem.s.iEffSeg, GCPtrEffDst);
                IEM_MC_REF_GREG_U64(pu64Dst, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
                IEM_MC_REF_EFLAGS(pEFlags);
                IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

                IEM_MC_ADVANCE_RIP();
                IEM_MC_END();
                break;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Common worker for instructions like ADD, AND, OR, ++ with working on AL with
 * a byte immediate.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_AL_Ib, PCIEMOPBINSIZES, pImpl)
{
    uint8_t u8Imm; IEM_OPCODE_GET_NEXT_U8(&u8Imm);
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

    IEM_MC_BEGIN(3, 0);
    IEM_MC_ARG(uint8_t *,       pu8Dst,             0);
    IEM_MC_ARG_CONST(uint8_t,   u8Src,/*=*/ u8Imm,  1);
    IEM_MC_ARG(uint32_t *,      pEFlags,            2);

    IEM_MC_REF_GREG_U8(pu8Dst, X86_GREG_xAX);
    IEM_MC_REF_EFLAGS(pEFlags);
    IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU8, pu8Dst, u8Src, pEFlags);

    IEM_MC_ADVANCE_RIP();
    IEM_MC_END();
    return VINF_SUCCESS;
}


/**
 * Common worker for instructions like ADD, AND, OR, ++ with working on
 * AX/EAX/RAX with a word/dword immediate.
 *
 * @param   pImpl       Pointer to the instruction implementation (assembly).
 */
FNIEMOP_DEF_1(iemOpHlpBinaryOperator_rAX_Iz, PCIEMOPBINSIZES, pImpl)
{
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t u16Imm; IEM_OPCODE_GET_NEXT_U16(&u16Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint16_t *,      pu16Dst,                0);
            IEM_MC_ARG_CONST(uint16_t,  u16Src,/*=*/ u16Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U16(pu16Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU16, pu16Dst, u16Src, pEFlags);

            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_32BIT:
        {
            uint32_t u32Imm; IEM_OPCODE_GET_NEXT_U32(&u32Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint32_t *,      pu32Dst,                0);
            IEM_MC_ARG_CONST(uint32_t,  u32Src,/*=*/ u32Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U32(pu32Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU32, pu32Dst, u32Src, pEFlags);

            if (pImpl != &g_iemAImpl_test)
                IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(pu32Dst);
            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            uint64_t u64Imm; IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64Imm);
            IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();

            IEM_MC_BEGIN(3, 0);
            IEM_MC_ARG(uint64_t *,      pu64Dst,                0);
            IEM_MC_ARG_CONST(uint64_t,  u64Src,/*=*/ u64Imm,    1);
            IEM_MC_ARG(uint32_t *,      pEFlags,                2);

            IEM_MC_REF_GREG_U64(pu64Dst, X86_GREG_xAX);
            IEM_MC_REF_EFLAGS(pEFlags);
            IEM_MC_CALL_VOID_AIMPL_3(pImpl->pfnNormalU64, pu64Dst, u64Src, pEFlags);

            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
            return VINF_SUCCESS;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }
}


/** Opcodes 0xf1, 0xd6. */
FNIEMOP_DEF(iemOp_Invalid)
{
    IEMOP_MNEMONIC(Invalid, "Invalid");
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte . */
FNIEMOPRM_DEF(iemOp_InvalidWithRM)
{
    RT_NOREF_PV(bRm);
    IEMOP_MNEMONIC(InvalidWithRm, "InvalidWithRM");
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where intel decodes any additional address encoding
 *  bytes. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMNeedDecode)
{
    IEMOP_MNEMONIC(InvalidWithRMNeedDecode, "InvalidWithRMNeedDecode");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
#ifndef TST_IEM_CHECK_MC
        if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where both AMD and Intel decodes any additional
 *  address encoding bytes. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMAllNeeded)
{
    IEMOP_MNEMONIC(InvalidWithRMAllNeeded, "InvalidWithRMAllNeeded");
#ifndef TST_IEM_CHECK_MC
    if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
    {
        RTGCPTR      GCPtrEff;
        VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
#endif
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where intel requires 8-byte immediate.
 * Intel will also need SIB and displacement if bRm indicates memory. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMNeedImm8)
{
    IEMOP_MNEMONIC(InvalidWithRMNeedImm8, "InvalidWithRMNeedImm8");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
#ifndef TST_IEM_CHECK_MC
        if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
        uint8_t bImm8;  IEM_OPCODE_GET_NEXT_U8(&bImm8);  RT_NOREF(bRm);
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid with RM byte where intel requires 8-byte immediate.
 * Both AMD and Intel also needs SIB and displacement according to bRm. */
FNIEMOPRM_DEF(iemOp_InvalidWithRMAllNeedImm8)
{
    IEMOP_MNEMONIC(InvalidWithRMAllNeedImm8, "InvalidWithRMAllNeedImm8");
#ifndef TST_IEM_CHECK_MC
    if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
    {
        RTGCPTR      GCPtrEff;
        VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
#endif
    uint8_t bImm8;  IEM_OPCODE_GET_NEXT_U8(&bImm8);  RT_NOREF(bRm);
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires Mod R/M sequence. */
FNIEMOP_DEF(iemOp_InvalidNeedRM)
{
    IEMOP_MNEMONIC(InvalidNeedRM, "InvalidNeedRM");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm); RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where both AMD and Intel requires Mod R/M sequence. */
FNIEMOP_DEF(iemOp_InvalidAllNeedRM)
{
    IEMOP_MNEMONIC(InvalidAllNeedRM, "InvalidAllNeedRM");
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm); RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
    if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
    {
        RTGCPTR      GCPtrEff;
        VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
#endif
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires Mod R/M sequence and 8-byte
 *  immediate. */
FNIEMOP_DEF(iemOp_InvalidNeedRMImm8)
{
    IEMOP_MNEMONIC(InvalidNeedRMImm8, "InvalidNeedRMImm8");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t bRm;  IEM_OPCODE_GET_NEXT_U8(&bRm);  RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
        uint8_t bImm; IEM_OPCODE_GET_NEXT_U8(&bImm); RT_NOREF(bImm);
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires a 3rd escape byte and a Mod R/M
 *  sequence. */
FNIEMOP_DEF(iemOp_InvalidNeed3ByteEscRM)
{
    IEMOP_MNEMONIC(InvalidNeed3ByteEscRM, "InvalidNeed3ByteEscRM");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t b3rd; IEM_OPCODE_GET_NEXT_U8(&b3rd); RT_NOREF(b3rd);
        uint8_t bRm;  IEM_OPCODE_GET_NEXT_U8(&bRm);  RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 0, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
    }
    IEMOP_HLP_DONE_DECODING();
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Invalid opcode where intel requires a 3rd escape byte, Mod R/M sequence, and
 *  a 8-byte immediate. */
FNIEMOP_DEF(iemOp_InvalidNeed3ByteEscRMImm8)
{
    IEMOP_MNEMONIC(InvalidNeed3ByteEscRMImm8, "InvalidNeed3ByteEscRMImm8");
    if (pVCpu->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL)
    {
        uint8_t b3rd; IEM_OPCODE_GET_NEXT_U8(&b3rd); RT_NOREF(b3rd);
        uint8_t bRm;  IEM_OPCODE_GET_NEXT_U8(&bRm);  RT_NOREF(bRm);
#ifndef TST_IEM_CHECK_MC
        if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
        {
            RTGCPTR      GCPtrEff;
            VBOXSTRICTRC rcStrict = iemOpHlpCalcRmEffAddr(pVCpu, bRm, 1, &GCPtrEff);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
        }
#endif
        uint8_t bImm; IEM_OPCODE_GET_NEXT_U8(&bImm); RT_NOREF(bImm);
        IEMOP_HLP_DONE_DECODING();
    }
    return IEMOP_RAISE_INVALID_OPCODE();
}


/** Repeats a_fn four times.  For decoding tables. */
#define IEMOP_X4(a_fn) a_fn, a_fn, a_fn, a_fn

/*
 * Include the tables.
 */
#ifdef IEM_WITH_3DNOW
# include "IEMAllInstructions3DNow.cpp.h"
#endif
#ifdef IEM_WITH_THREE_0F_38
# include "IEMAllInstructionsThree0f38.cpp.h"
#endif
#ifdef IEM_WITH_THREE_0F_3A
# include "IEMAllInstructionsThree0f3a.cpp.h"
#endif
#include "IEMAllInstructionsTwoByte0f.cpp.h"
#ifdef IEM_WITH_VEX
# include "IEMAllInstructionsVexMap1.cpp.h"
# include "IEMAllInstructionsVexMap2.cpp.h"
# include "IEMAllInstructionsVexMap3.cpp.h"
#endif
#include "IEMAllInstructionsOneByte.cpp.h"


#ifdef _MSC_VER
# pragma warning(pop)
#endif


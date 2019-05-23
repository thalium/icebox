/* $Id: IEMAllAImplC.cpp $ */
/** @file
 * IEM - Instruction Implementation in Assembly, portable C variant.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "IEMInternal.h"
#include <VBox/vmm/vm.h>
#include <iprt/x86.h>
#include <iprt/uint128.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef RT_ARCH_X86
/**
 * Parity calculation table.
 *
 * The generator code:
 * @code
 * #include <stdio.h>
 *
 * int main()
 * {
 *     unsigned b;
 *     for (b = 0; b < 256; b++)
 *     {
 *         int cOnes = ( b       & 1)
 *                   + ((b >> 1) & 1)
 *                   + ((b >> 2) & 1)
 *                   + ((b >> 3) & 1)
 *                   + ((b >> 4) & 1)
 *                   + ((b >> 5) & 1)
 *                   + ((b >> 6) & 1)
 *                   + ((b >> 7) & 1);
 *         printf("    /" "* %#04x = %u%u%u%u%u%u%u%ub *" "/ %s,\n",
 *                b,
 *                (b >> 7) & 1,
 *                (b >> 6) & 1,
 *                (b >> 5) & 1,
 *                (b >> 4) & 1,
 *                (b >> 3) & 1,
 *                (b >> 2) & 1,
 *                (b >> 1) & 1,
 *                 b       & 1,
 *                cOnes & 1 ? "0" : "X86_EFL_PF");
 *     }
 *     return 0;
 * }
 * @endcode
 */
static uint8_t const g_afParity[256] =
{
    /* 0000 = 00000000b */ X86_EFL_PF,
    /* 0x01 = 00000001b */ 0,
    /* 0x02 = 00000010b */ 0,
    /* 0x03 = 00000011b */ X86_EFL_PF,
    /* 0x04 = 00000100b */ 0,
    /* 0x05 = 00000101b */ X86_EFL_PF,
    /* 0x06 = 00000110b */ X86_EFL_PF,
    /* 0x07 = 00000111b */ 0,
    /* 0x08 = 00001000b */ 0,
    /* 0x09 = 00001001b */ X86_EFL_PF,
    /* 0x0a = 00001010b */ X86_EFL_PF,
    /* 0x0b = 00001011b */ 0,
    /* 0x0c = 00001100b */ X86_EFL_PF,
    /* 0x0d = 00001101b */ 0,
    /* 0x0e = 00001110b */ 0,
    /* 0x0f = 00001111b */ X86_EFL_PF,
    /* 0x10 = 00010000b */ 0,
    /* 0x11 = 00010001b */ X86_EFL_PF,
    /* 0x12 = 00010010b */ X86_EFL_PF,
    /* 0x13 = 00010011b */ 0,
    /* 0x14 = 00010100b */ X86_EFL_PF,
    /* 0x15 = 00010101b */ 0,
    /* 0x16 = 00010110b */ 0,
    /* 0x17 = 00010111b */ X86_EFL_PF,
    /* 0x18 = 00011000b */ X86_EFL_PF,
    /* 0x19 = 00011001b */ 0,
    /* 0x1a = 00011010b */ 0,
    /* 0x1b = 00011011b */ X86_EFL_PF,
    /* 0x1c = 00011100b */ 0,
    /* 0x1d = 00011101b */ X86_EFL_PF,
    /* 0x1e = 00011110b */ X86_EFL_PF,
    /* 0x1f = 00011111b */ 0,
    /* 0x20 = 00100000b */ 0,
    /* 0x21 = 00100001b */ X86_EFL_PF,
    /* 0x22 = 00100010b */ X86_EFL_PF,
    /* 0x23 = 00100011b */ 0,
    /* 0x24 = 00100100b */ X86_EFL_PF,
    /* 0x25 = 00100101b */ 0,
    /* 0x26 = 00100110b */ 0,
    /* 0x27 = 00100111b */ X86_EFL_PF,
    /* 0x28 = 00101000b */ X86_EFL_PF,
    /* 0x29 = 00101001b */ 0,
    /* 0x2a = 00101010b */ 0,
    /* 0x2b = 00101011b */ X86_EFL_PF,
    /* 0x2c = 00101100b */ 0,
    /* 0x2d = 00101101b */ X86_EFL_PF,
    /* 0x2e = 00101110b */ X86_EFL_PF,
    /* 0x2f = 00101111b */ 0,
    /* 0x30 = 00110000b */ X86_EFL_PF,
    /* 0x31 = 00110001b */ 0,
    /* 0x32 = 00110010b */ 0,
    /* 0x33 = 00110011b */ X86_EFL_PF,
    /* 0x34 = 00110100b */ 0,
    /* 0x35 = 00110101b */ X86_EFL_PF,
    /* 0x36 = 00110110b */ X86_EFL_PF,
    /* 0x37 = 00110111b */ 0,
    /* 0x38 = 00111000b */ 0,
    /* 0x39 = 00111001b */ X86_EFL_PF,
    /* 0x3a = 00111010b */ X86_EFL_PF,
    /* 0x3b = 00111011b */ 0,
    /* 0x3c = 00111100b */ X86_EFL_PF,
    /* 0x3d = 00111101b */ 0,
    /* 0x3e = 00111110b */ 0,
    /* 0x3f = 00111111b */ X86_EFL_PF,
    /* 0x40 = 01000000b */ 0,
    /* 0x41 = 01000001b */ X86_EFL_PF,
    /* 0x42 = 01000010b */ X86_EFL_PF,
    /* 0x43 = 01000011b */ 0,
    /* 0x44 = 01000100b */ X86_EFL_PF,
    /* 0x45 = 01000101b */ 0,
    /* 0x46 = 01000110b */ 0,
    /* 0x47 = 01000111b */ X86_EFL_PF,
    /* 0x48 = 01001000b */ X86_EFL_PF,
    /* 0x49 = 01001001b */ 0,
    /* 0x4a = 01001010b */ 0,
    /* 0x4b = 01001011b */ X86_EFL_PF,
    /* 0x4c = 01001100b */ 0,
    /* 0x4d = 01001101b */ X86_EFL_PF,
    /* 0x4e = 01001110b */ X86_EFL_PF,
    /* 0x4f = 01001111b */ 0,
    /* 0x50 = 01010000b */ X86_EFL_PF,
    /* 0x51 = 01010001b */ 0,
    /* 0x52 = 01010010b */ 0,
    /* 0x53 = 01010011b */ X86_EFL_PF,
    /* 0x54 = 01010100b */ 0,
    /* 0x55 = 01010101b */ X86_EFL_PF,
    /* 0x56 = 01010110b */ X86_EFL_PF,
    /* 0x57 = 01010111b */ 0,
    /* 0x58 = 01011000b */ 0,
    /* 0x59 = 01011001b */ X86_EFL_PF,
    /* 0x5a = 01011010b */ X86_EFL_PF,
    /* 0x5b = 01011011b */ 0,
    /* 0x5c = 01011100b */ X86_EFL_PF,
    /* 0x5d = 01011101b */ 0,
    /* 0x5e = 01011110b */ 0,
    /* 0x5f = 01011111b */ X86_EFL_PF,
    /* 0x60 = 01100000b */ X86_EFL_PF,
    /* 0x61 = 01100001b */ 0,
    /* 0x62 = 01100010b */ 0,
    /* 0x63 = 01100011b */ X86_EFL_PF,
    /* 0x64 = 01100100b */ 0,
    /* 0x65 = 01100101b */ X86_EFL_PF,
    /* 0x66 = 01100110b */ X86_EFL_PF,
    /* 0x67 = 01100111b */ 0,
    /* 0x68 = 01101000b */ 0,
    /* 0x69 = 01101001b */ X86_EFL_PF,
    /* 0x6a = 01101010b */ X86_EFL_PF,
    /* 0x6b = 01101011b */ 0,
    /* 0x6c = 01101100b */ X86_EFL_PF,
    /* 0x6d = 01101101b */ 0,
    /* 0x6e = 01101110b */ 0,
    /* 0x6f = 01101111b */ X86_EFL_PF,
    /* 0x70 = 01110000b */ 0,
    /* 0x71 = 01110001b */ X86_EFL_PF,
    /* 0x72 = 01110010b */ X86_EFL_PF,
    /* 0x73 = 01110011b */ 0,
    /* 0x74 = 01110100b */ X86_EFL_PF,
    /* 0x75 = 01110101b */ 0,
    /* 0x76 = 01110110b */ 0,
    /* 0x77 = 01110111b */ X86_EFL_PF,
    /* 0x78 = 01111000b */ X86_EFL_PF,
    /* 0x79 = 01111001b */ 0,
    /* 0x7a = 01111010b */ 0,
    /* 0x7b = 01111011b */ X86_EFL_PF,
    /* 0x7c = 01111100b */ 0,
    /* 0x7d = 01111101b */ X86_EFL_PF,
    /* 0x7e = 01111110b */ X86_EFL_PF,
    /* 0x7f = 01111111b */ 0,
    /* 0x80 = 10000000b */ 0,
    /* 0x81 = 10000001b */ X86_EFL_PF,
    /* 0x82 = 10000010b */ X86_EFL_PF,
    /* 0x83 = 10000011b */ 0,
    /* 0x84 = 10000100b */ X86_EFL_PF,
    /* 0x85 = 10000101b */ 0,
    /* 0x86 = 10000110b */ 0,
    /* 0x87 = 10000111b */ X86_EFL_PF,
    /* 0x88 = 10001000b */ X86_EFL_PF,
    /* 0x89 = 10001001b */ 0,
    /* 0x8a = 10001010b */ 0,
    /* 0x8b = 10001011b */ X86_EFL_PF,
    /* 0x8c = 10001100b */ 0,
    /* 0x8d = 10001101b */ X86_EFL_PF,
    /* 0x8e = 10001110b */ X86_EFL_PF,
    /* 0x8f = 10001111b */ 0,
    /* 0x90 = 10010000b */ X86_EFL_PF,
    /* 0x91 = 10010001b */ 0,
    /* 0x92 = 10010010b */ 0,
    /* 0x93 = 10010011b */ X86_EFL_PF,
    /* 0x94 = 10010100b */ 0,
    /* 0x95 = 10010101b */ X86_EFL_PF,
    /* 0x96 = 10010110b */ X86_EFL_PF,
    /* 0x97 = 10010111b */ 0,
    /* 0x98 = 10011000b */ 0,
    /* 0x99 = 10011001b */ X86_EFL_PF,
    /* 0x9a = 10011010b */ X86_EFL_PF,
    /* 0x9b = 10011011b */ 0,
    /* 0x9c = 10011100b */ X86_EFL_PF,
    /* 0x9d = 10011101b */ 0,
    /* 0x9e = 10011110b */ 0,
    /* 0x9f = 10011111b */ X86_EFL_PF,
    /* 0xa0 = 10100000b */ X86_EFL_PF,
    /* 0xa1 = 10100001b */ 0,
    /* 0xa2 = 10100010b */ 0,
    /* 0xa3 = 10100011b */ X86_EFL_PF,
    /* 0xa4 = 10100100b */ 0,
    /* 0xa5 = 10100101b */ X86_EFL_PF,
    /* 0xa6 = 10100110b */ X86_EFL_PF,
    /* 0xa7 = 10100111b */ 0,
    /* 0xa8 = 10101000b */ 0,
    /* 0xa9 = 10101001b */ X86_EFL_PF,
    /* 0xaa = 10101010b */ X86_EFL_PF,
    /* 0xab = 10101011b */ 0,
    /* 0xac = 10101100b */ X86_EFL_PF,
    /* 0xad = 10101101b */ 0,
    /* 0xae = 10101110b */ 0,
    /* 0xaf = 10101111b */ X86_EFL_PF,
    /* 0xb0 = 10110000b */ 0,
    /* 0xb1 = 10110001b */ X86_EFL_PF,
    /* 0xb2 = 10110010b */ X86_EFL_PF,
    /* 0xb3 = 10110011b */ 0,
    /* 0xb4 = 10110100b */ X86_EFL_PF,
    /* 0xb5 = 10110101b */ 0,
    /* 0xb6 = 10110110b */ 0,
    /* 0xb7 = 10110111b */ X86_EFL_PF,
    /* 0xb8 = 10111000b */ X86_EFL_PF,
    /* 0xb9 = 10111001b */ 0,
    /* 0xba = 10111010b */ 0,
    /* 0xbb = 10111011b */ X86_EFL_PF,
    /* 0xbc = 10111100b */ 0,
    /* 0xbd = 10111101b */ X86_EFL_PF,
    /* 0xbe = 10111110b */ X86_EFL_PF,
    /* 0xbf = 10111111b */ 0,
    /* 0xc0 = 11000000b */ X86_EFL_PF,
    /* 0xc1 = 11000001b */ 0,
    /* 0xc2 = 11000010b */ 0,
    /* 0xc3 = 11000011b */ X86_EFL_PF,
    /* 0xc4 = 11000100b */ 0,
    /* 0xc5 = 11000101b */ X86_EFL_PF,
    /* 0xc6 = 11000110b */ X86_EFL_PF,
    /* 0xc7 = 11000111b */ 0,
    /* 0xc8 = 11001000b */ 0,
    /* 0xc9 = 11001001b */ X86_EFL_PF,
    /* 0xca = 11001010b */ X86_EFL_PF,
    /* 0xcb = 11001011b */ 0,
    /* 0xcc = 11001100b */ X86_EFL_PF,
    /* 0xcd = 11001101b */ 0,
    /* 0xce = 11001110b */ 0,
    /* 0xcf = 11001111b */ X86_EFL_PF,
    /* 0xd0 = 11010000b */ 0,
    /* 0xd1 = 11010001b */ X86_EFL_PF,
    /* 0xd2 = 11010010b */ X86_EFL_PF,
    /* 0xd3 = 11010011b */ 0,
    /* 0xd4 = 11010100b */ X86_EFL_PF,
    /* 0xd5 = 11010101b */ 0,
    /* 0xd6 = 11010110b */ 0,
    /* 0xd7 = 11010111b */ X86_EFL_PF,
    /* 0xd8 = 11011000b */ X86_EFL_PF,
    /* 0xd9 = 11011001b */ 0,
    /* 0xda = 11011010b */ 0,
    /* 0xdb = 11011011b */ X86_EFL_PF,
    /* 0xdc = 11011100b */ 0,
    /* 0xdd = 11011101b */ X86_EFL_PF,
    /* 0xde = 11011110b */ X86_EFL_PF,
    /* 0xdf = 11011111b */ 0,
    /* 0xe0 = 11100000b */ 0,
    /* 0xe1 = 11100001b */ X86_EFL_PF,
    /* 0xe2 = 11100010b */ X86_EFL_PF,
    /* 0xe3 = 11100011b */ 0,
    /* 0xe4 = 11100100b */ X86_EFL_PF,
    /* 0xe5 = 11100101b */ 0,
    /* 0xe6 = 11100110b */ 0,
    /* 0xe7 = 11100111b */ X86_EFL_PF,
    /* 0xe8 = 11101000b */ X86_EFL_PF,
    /* 0xe9 = 11101001b */ 0,
    /* 0xea = 11101010b */ 0,
    /* 0xeb = 11101011b */ X86_EFL_PF,
    /* 0xec = 11101100b */ 0,
    /* 0xed = 11101101b */ X86_EFL_PF,
    /* 0xee = 11101110b */ X86_EFL_PF,
    /* 0xef = 11101111b */ 0,
    /* 0xf0 = 11110000b */ X86_EFL_PF,
    /* 0xf1 = 11110001b */ 0,
    /* 0xf2 = 11110010b */ 0,
    /* 0xf3 = 11110011b */ X86_EFL_PF,
    /* 0xf4 = 11110100b */ 0,
    /* 0xf5 = 11110101b */ X86_EFL_PF,
    /* 0xf6 = 11110110b */ X86_EFL_PF,
    /* 0xf7 = 11110111b */ 0,
    /* 0xf8 = 11111000b */ 0,
    /* 0xf9 = 11111001b */ X86_EFL_PF,
    /* 0xfa = 11111010b */ X86_EFL_PF,
    /* 0xfb = 11111011b */ 0,
    /* 0xfc = 11111100b */ X86_EFL_PF,
    /* 0xfd = 11111101b */ 0,
    /* 0xfe = 11111110b */ 0,
    /* 0xff = 11111111b */ X86_EFL_PF,
};
#endif /* RT_ARCH_X86 */


/**
 * Calculates the signed flag value given a result and it's bit width.
 *
 * The signed flag (SF) is a duplication of the most significant bit in the
 * result.
 *
 * @returns X86_EFL_SF or 0.
 * @param   a_uResult       Unsigned result value.
 * @param   a_cBitsWidth    The width of the result (8, 16, 32, 64).
 */
#define X86_EFL_CALC_SF(a_uResult, a_cBitsWidth) \
    ( (uint32_t)((a_uResult) >> ((a_cBitsWidth) - X86_EFL_SF_BIT - 1)) & X86_EFL_SF )

/**
 * Calculates the zero flag value given a result.
 *
 * The zero flag (ZF) indicates whether the result is zero or not.
 *
 * @returns X86_EFL_ZF or 0.
 * @param   a_uResult       Unsigned result value.
 */
#define X86_EFL_CALC_ZF(a_uResult) \
    ( (uint32_t)((a_uResult) == 0) << X86_EFL_ZF_BIT )

/**
 * Updates the status bits (CF, PF, AF, ZF, SF, and OF) after a logical op.
 *
 * CF and OF are defined to be 0 by logical operations.  AF on the other hand is
 * undefined.  We do not set AF, as that seems to make the most sense (which
 * probably makes it the most wrong in real life).
 *
 * @returns Status bits.
 * @param   a_pfEFlags      Pointer to the 32-bit EFLAGS value to update.
 * @param   a_uResult       Unsigned result value.
 * @param   a_cBitsWidth    The width of the result (8, 16, 32, 64).
 * @param   a_fExtra        Additional bits to set.
 */
#define IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(a_pfEFlags, a_uResult, a_cBitsWidth, a_fExtra) \
    do { \
        uint32_t fEflTmp = *(a_pfEFlags); \
        fEflTmp &= ~X86_EFL_STATUS_BITS; \
        fEflTmp |= g_afParity[(a_uResult) & 0xff]; \
        fEflTmp |= X86_EFL_CALC_ZF(a_uResult); \
        fEflTmp |= X86_EFL_CALC_SF(a_uResult, a_cBitsWidth); \
        fEflTmp |= (a_fExtra); \
        *(a_pfEFlags) = fEflTmp; \
    } while (0)


#ifdef RT_ARCH_X86
/*
 * There are a few 64-bit on 32-bit things we'd rather do in C.  Actually, doing
 * it all in C is probably safer atm., optimize what's necessary later, maybe.
 */


/* Binary ops */

IEM_DECL_IMPL_DEF(void, iemAImpl_add_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    uint64_t uDst    = *puDst;
    uint64_t uResult = uDst + uSrc;
    *puDst = uResult;

    /* Calc EFLAGS. */
    uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
    fEfl |= (uResult < uDst) << X86_EFL_CF_BIT;
    fEfl |= g_afParity[uResult & 0xff];
    fEfl |= ((uint32_t)uResult ^ (uint32_t)uSrc ^ (uint32_t)uDst) & X86_EFL_AF;
    fEfl |= X86_EFL_CALC_ZF(uResult);
    fEfl |= X86_EFL_CALC_SF(uResult, 64);
    fEfl |= (((uDst ^ uSrc ^ RT_BIT_64(63)) & (uResult ^ uDst)) >> (64 - X86_EFL_OF_BIT)) & X86_EFL_OF;
    *pfEFlags = fEfl;
}


IEM_DECL_IMPL_DEF(void, iemAImpl_adc_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    if (!(*pfEFlags & X86_EFL_CF))
        iemAImpl_add_u64(puDst, uSrc, pfEFlags);
    else
    {
        uint64_t uDst    = *puDst;
        uint64_t uResult = uDst + uSrc + 1;
        *puDst = uResult;

        /* Calc EFLAGS. */
        /** @todo verify AF and OF calculations. */
        uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
        fEfl |= (uResult <= uDst) << X86_EFL_CF_BIT;
        fEfl |= g_afParity[uResult & 0xff];
        fEfl |= ((uint32_t)uResult ^ (uint32_t)uSrc ^ (uint32_t)uDst) & X86_EFL_AF;
        fEfl |= X86_EFL_CALC_ZF(uResult);
        fEfl |= X86_EFL_CALC_SF(uResult, 64);
        fEfl |= (((uDst ^ uSrc ^ RT_BIT_64(63)) & (uResult ^ uDst)) >> (64 - X86_EFL_OF_BIT)) & X86_EFL_OF;
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_sub_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    uint64_t uDst    = *puDst;
    uint64_t uResult = uDst - uSrc;
    *puDst = uResult;

    /* Calc EFLAGS. */
    uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
    fEfl |= (uDst < uSrc) << X86_EFL_CF_BIT;
    fEfl |= g_afParity[uResult & 0xff];
    fEfl |= ((uint32_t)uResult ^ (uint32_t)uSrc ^ (uint32_t)uDst) & X86_EFL_AF;
    fEfl |= X86_EFL_CALC_ZF(uResult);
    fEfl |= X86_EFL_CALC_SF(uResult, 64);
    fEfl |= (((uDst ^ uSrc) & (uResult ^ uDst)) >> (64 - X86_EFL_OF_BIT)) & X86_EFL_OF;
    *pfEFlags = fEfl;
}


IEM_DECL_IMPL_DEF(void, iemAImpl_sbb_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    if (!(*pfEFlags & X86_EFL_CF))
        iemAImpl_sub_u64(puDst, uSrc, pfEFlags);
    else
    {
        uint64_t uDst    = *puDst;
        uint64_t uResult = uDst - uSrc - 1;
        *puDst = uResult;

        /* Calc EFLAGS. */
        /** @todo verify AF and OF calculations. */
        uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
        fEfl |= (uDst <= uSrc) << X86_EFL_CF_BIT;
        fEfl |= g_afParity[uResult & 0xff];
        fEfl |= ((uint32_t)uResult ^ (uint32_t)uSrc ^ (uint32_t)uDst) & X86_EFL_AF;
        fEfl |= X86_EFL_CALC_ZF(uResult);
        fEfl |= X86_EFL_CALC_SF(uResult, 64);
        fEfl |= (((uDst ^ uSrc) & (uResult ^ uDst)) >> (64 - X86_EFL_OF_BIT)) & X86_EFL_OF;
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_or_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    uint64_t uResult = *puDst | uSrc;
    *puDst = uResult;
    IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uResult, 64, 0);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_xor_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    uint64_t uResult = *puDst ^ uSrc;
    *puDst = uResult;
    IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uResult, 64, 0);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_and_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    uint64_t uResult = *puDst & uSrc;
    *puDst = uResult;
    IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uResult, 64, 0);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_cmp_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    uint64_t uDstTmp = *puDst;
    iemAImpl_sub_u64(&uDstTmp, uSrc, pfEFlags);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_test_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    uint64_t uResult = *puDst & uSrc;
    IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uResult, 64, 0);
}


/** 64-bit locked binary operand operation. */
# define DO_LOCKED_BIN_OP_U64(a_Mnemonic) \
    do { \
        uint64_t uOld = ASMAtomicReadU64(puDst); \
        uint64_t uTmp; \
        uint32_t fEflTmp; \
        do \
        { \
            uTmp = uOld; \
            fEflTmp = *pfEFlags; \
            iemAImpl_ ## a_Mnemonic ## _u64(&uTmp, uSrc, &fEflTmp); \
        } while (!ASMAtomicCmpXchgExU64(puDst, uTmp, uOld, &uOld)); \
        *pfEFlags = fEflTmp; \
    } while (0)


IEM_DECL_IMPL_DEF(void, iemAImpl_add_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(add);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_adc_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(adc);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_sub_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(sub);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_sbb_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(sbb);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_or_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(or);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_xor_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(xor);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_and_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(and);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u64,(uint64_t *puDst, uint64_t *puReg, uint32_t *pfEFlags))
{
    uint64_t uDst    = *puDst;
    uint64_t uResult = uDst;
    iemAImpl_add_u64(&uResult, *puReg, pfEFlags);
    *puDst = uResult;
    *puReg = uDst;
}


IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u64_locked,(uint64_t *puDst, uint64_t *puReg, uint32_t *pfEFlags))
{
    uint64_t uOld = ASMAtomicReadU64(puDst);
    uint64_t uTmpDst;
    uint32_t fEflTmp;
    do
    {
        uTmpDst = uOld;
        fEflTmp = *pfEFlags;
        iemAImpl_add_u64(&uTmpDst, *puReg, pfEFlags);
    } while (!ASMAtomicCmpXchgExU64(puDst, uTmpDst, uOld, &uOld));
    *puReg    = uOld;
    *pfEFlags = fEflTmp;
}


/* Bit operations (same signature as above). */

IEM_DECL_IMPL_DEF(void, iemAImpl_bt_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    /* Note! "undefined" flags: OF, SF, ZF, AF, PF.  We set them as after an
       logical operation (AND/OR/whatever). */
    Assert(uSrc < 64);
    uint64_t uDst = *puDst;
    if (uDst & RT_BIT_64(uSrc))
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, X86_EFL_CF);
    else
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, 0);
}

IEM_DECL_IMPL_DEF(void, iemAImpl_btc_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    /* Note! "undefined" flags: OF, SF, ZF, AF, PF.  We set them as after an
       logical operation (AND/OR/whatever). */
    Assert(uSrc < 64);
    uint64_t fMask = RT_BIT_64(uSrc);
    uint64_t uDst = *puDst;
    if (uDst & fMask)
    {
        uDst &= ~fMask;
        *puDst = uDst;
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, X86_EFL_CF);
    }
    else
    {
        uDst |= fMask;
        *puDst = uDst;
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, 0);
    }
}

IEM_DECL_IMPL_DEF(void, iemAImpl_btr_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    /* Note! "undefined" flags: OF, SF, ZF, AF, PF.  We set them as after an
       logical operation (AND/OR/whatever). */
    Assert(uSrc < 64);
    uint64_t fMask = RT_BIT_64(uSrc);
    uint64_t uDst = *puDst;
    if (uDst & fMask)
    {
        uDst &= ~fMask;
        *puDst = uDst;
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, X86_EFL_CF);
    }
    else
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, 0);
}

IEM_DECL_IMPL_DEF(void, iemAImpl_bts_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    /* Note! "undefined" flags: OF, SF, ZF, AF, PF.  We set them as after an
       logical operation (AND/OR/whatever). */
    Assert(uSrc < 64);
    uint64_t fMask = RT_BIT_64(uSrc);
    uint64_t uDst = *puDst;
    if (uDst & fMask)
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, X86_EFL_CF);
    else
    {
        uDst |= fMask;
        *puDst = uDst;
        IEM_EFL_UPDATE_STATUS_BITS_FOR_LOGIC(pfEFlags, uDst, 64, 0);
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_btc_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(btc);
}

IEM_DECL_IMPL_DEF(void, iemAImpl_btr_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(btr);
}

IEM_DECL_IMPL_DEF(void, iemAImpl_bts_u64_locked,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    DO_LOCKED_BIN_OP_U64(bts);
}


/* bit scan */

IEM_DECL_IMPL_DEF(void, iemAImpl_bsf_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    /* Note! "undefined" flags: OF, SF, AF, PF, CF. */
    /** @todo check what real CPUs do. */
    if (uSrc)
    {
        uint8_t  iBit;
        uint32_t u32Src;
        if (uSrc & UINT32_MAX)
        {
            iBit = 0;
            u32Src = uSrc;
        }
        else
        {
            iBit = 32;
            u32Src = uSrc >> 32;
        }
        if (!(u32Src & UINT16_MAX))
        {
            iBit += 16;
            u32Src >>= 16;
        }
        if (!(u32Src & UINT8_MAX))
        {
            iBit += 8;
            u32Src >>= 8;
        }
        if (!(u32Src & 0xf))
        {
            iBit += 4;
            u32Src >>= 4;
        }
        if (!(u32Src & 0x3))
        {
            iBit += 2;
            u32Src >>= 2;
        }
        if (!(u32Src & 1))
        {
            iBit += 1;
            Assert(u32Src & 2);
        }

        *puDst     = iBit;
        *pfEFlags &= ~X86_EFL_ZF;
    }
    else
        *pfEFlags |= X86_EFL_ZF;
}

IEM_DECL_IMPL_DEF(void, iemAImpl_bsr_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
    /* Note! "undefined" flags: OF, SF, AF, PF, CF. */
    /** @todo check what real CPUs do. */
    if (uSrc)
    {
        uint8_t  iBit;
        uint32_t u32Src;
        if (uSrc & UINT64_C(0xffffffff00000000))
        {
            iBit = 63;
            u32Src = uSrc >> 32;
        }
        else
        {
            iBit = 31;
            u32Src = uSrc;
        }
        if (!(u32Src & UINT32_C(0xffff0000)))
        {
            iBit -= 16;
            u32Src <<= 16;
        }
        if (!(u32Src & UINT32_C(0xff000000)))
        {
            iBit -= 8;
            u32Src <<= 8;
        }
        if (!(u32Src & UINT32_C(0xf0000000)))
        {
            iBit -= 4;
            u32Src <<= 4;
        }
        if (!(u32Src & UINT32_C(0xc0000000)))
        {
            iBit -= 2;
            u32Src <<= 2;
        }
        if (!(u32Src & UINT32_C(0x80000000)))
        {
            iBit -= 1;
            Assert(u32Src & RT_BIT(30));
        }

        *puDst     = iBit;
        *pfEFlags &= ~X86_EFL_ZF;
    }
    else
        *pfEFlags |= X86_EFL_ZF;
}


/* Unary operands. */

IEM_DECL_IMPL_DEF(void, iemAImpl_inc_u64,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    uint64_t uDst    = *puDst;
    uint64_t uResult = uDst + 1;
    *puDst = uResult;

    /*
     * Calc EFLAGS.
     * CF is NOT modified for hysterical raisins (allegedly for carrying and
     * borrowing in arithmetic loops on intel 8008).
     */
    uint32_t fEfl = *pfEFlags & ~(X86_EFL_STATUS_BITS & ~X86_EFL_CF);
    fEfl |= g_afParity[uResult & 0xff];
    fEfl |= ((uint32_t)uResult ^ (uint32_t)uDst) & X86_EFL_AF;
    fEfl |= X86_EFL_CALC_ZF(uResult);
    fEfl |= X86_EFL_CALC_SF(uResult, 64);
    fEfl |= (((uDst ^ RT_BIT_64(63)) & uResult) >> (64 - X86_EFL_OF_BIT)) & X86_EFL_OF;
    *pfEFlags = fEfl;
}


IEM_DECL_IMPL_DEF(void, iemAImpl_dec_u64,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    uint64_t uDst    = *puDst;
    uint64_t uResult = uDst - 1;
    *puDst = uResult;

    /*
     * Calc EFLAGS.
     * CF is NOT modified for hysterical raisins (allegedly for carrying and
     * borrowing in arithmetic loops on intel 8008).
     */
    uint32_t fEfl = *pfEFlags & ~(X86_EFL_STATUS_BITS & ~X86_EFL_CF);
    fEfl |= g_afParity[uResult & 0xff];
    fEfl |= ((uint32_t)uResult ^ (uint32_t)uDst) & X86_EFL_AF;
    fEfl |= X86_EFL_CALC_ZF(uResult);
    fEfl |= X86_EFL_CALC_SF(uResult, 64);
    fEfl |= ((uDst & (uResult ^ RT_BIT_64(63))) >> (64 - X86_EFL_OF_BIT)) & X86_EFL_OF;
    *pfEFlags = fEfl;
}


IEM_DECL_IMPL_DEF(void, iemAImpl_not_u64,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    uint64_t uDst    = *puDst;
    uint64_t uResult = ~uDst;
    *puDst = uResult;
    /* EFLAGS are not modified. */
    RT_NOREF_PV(pfEFlags);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_neg_u64,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    uint64_t uDst    = 0;
    uint64_t uSrc    = *puDst;
    uint64_t uResult = uDst - uSrc;
    *puDst = uResult;

    /* Calc EFLAGS. */
    uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
    fEfl |= (uSrc != 0) << X86_EFL_CF_BIT;
    fEfl |= g_afParity[uResult & 0xff];
    fEfl |= ((uint32_t)uResult ^ (uint32_t)uDst) & X86_EFL_AF;
    fEfl |= X86_EFL_CALC_ZF(uResult);
    fEfl |= X86_EFL_CALC_SF(uResult, 64);
    fEfl |= ((uSrc & uResult) >> (64 - X86_EFL_OF_BIT)) & X86_EFL_OF;
    *pfEFlags = fEfl;
}


/** 64-bit locked unary operand operation. */
# define DO_LOCKED_UNARY_OP_U64(a_Mnemonic) \
    do { \
        uint64_t uOld = ASMAtomicReadU64(puDst); \
        uint64_t uTmp; \
        uint32_t fEflTmp; \
        do \
        { \
            uTmp = uOld; \
            fEflTmp = *pfEFlags; \
            iemAImpl_ ## a_Mnemonic ## _u64(&uTmp, &fEflTmp); \
        } while (!ASMAtomicCmpXchgExU64(puDst, uTmp, uOld, &uOld)); \
        *pfEFlags = fEflTmp; \
    } while (0)

IEM_DECL_IMPL_DEF(void, iemAImpl_inc_u64_locked,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    DO_LOCKED_UNARY_OP_U64(inc);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_dec_u64_locked,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    DO_LOCKED_UNARY_OP_U64(dec);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_not_u64_locked,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    DO_LOCKED_UNARY_OP_U64(not);
}


IEM_DECL_IMPL_DEF(void, iemAImpl_neg_u64_locked,(uint64_t  *puDst,  uint32_t *pfEFlags))
{
    DO_LOCKED_UNARY_OP_U64(neg);
}


/* Shift and rotate. */

IEM_DECL_IMPL_DEF(void, iemAImpl_rol_u64,(uint64_t *puDst, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint64_t uDst = *puDst;
        uint64_t uResult;
        uResult  = uDst << cShift;
        uResult |= uDst >> (64 - cShift);
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts. */
        AssertCompile(X86_EFL_CF_BIT == 0);
        uint32_t fEfl   = *pfEFlags & ~(X86_EFL_CF | X86_EFL_OF);
        uint32_t fCarry = (uResult & 1);
        fEfl |= fCarry;
        fEfl |= ((uResult >> 63) ^ fCarry) << X86_EFL_OF_BIT;
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_ror_u64,(uint64_t *puDst, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint64_t uDst = *puDst;
        uint64_t uResult;
        uResult  = uDst >> cShift;
        uResult |= uDst << (64 - cShift);
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts (OF = OF XOR New-CF). */
        AssertCompile(X86_EFL_CF_BIT == 0);
        uint32_t fEfl   = *pfEFlags & ~(X86_EFL_CF | X86_EFL_OF);
        uint32_t fCarry = (uResult >> 63) & X86_EFL_CF;
        fEfl |= fCarry;
        fEfl |= (((uResult >> 62) ^ fCarry) << X86_EFL_OF_BIT) & X86_EFL_OF;
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_rcl_u64,(uint64_t *puDst, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint32_t fEfl = *pfEFlags;
        uint64_t uDst = *puDst;
        uint64_t uResult;
        uResult = uDst << cShift;
        AssertCompile(X86_EFL_CF_BIT == 0);
        if (cShift > 1)
            uResult |= uDst >> (65 - cShift);
        uResult |= (uint64_t)(fEfl & X86_EFL_CF) << (cShift - 1);
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts. */
        uint32_t fCarry = (uDst >> (64 - cShift)) & X86_EFL_CF;
        fEfl &= ~(X86_EFL_CF | X86_EFL_OF);
        fEfl |= fCarry;
        fEfl |= ((uResult >> 63) ^ fCarry) << X86_EFL_OF_BIT;
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_rcr_u64,(uint64_t *puDst, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint32_t fEfl = *pfEFlags;
        uint64_t uDst = *puDst;
        uint64_t uResult;
        uResult  = uDst >> cShift;
        AssertCompile(X86_EFL_CF_BIT == 0);
        if (cShift > 1)
            uResult |= uDst << (65 - cShift);
        uResult |= (uint64_t)(fEfl & X86_EFL_CF) << (64 - cShift);
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts. */
        uint32_t fCarry = (uDst >> (cShift - 1)) & X86_EFL_CF;
        fEfl &= ~(X86_EFL_CF | X86_EFL_OF);
        fEfl |= fCarry;
        fEfl |= ((uResult >> 63) ^ fCarry) << X86_EFL_OF_BIT;
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_shl_u64,(uint64_t *puDst, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint64_t uDst = *puDst;
        uint64_t uResult = uDst << cShift;
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts.  The AF bit is undefined, we
           always set it to zero atm. */
        AssertCompile(X86_EFL_CF_BIT == 0);
        uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
        uint32_t fCarry = (uDst >> (64 - cShift)) & X86_EFL_CF;
        fEfl |= fCarry;
        fEfl |= ((uResult >> 63) ^ fCarry) << X86_EFL_OF_BIT;
        fEfl |= X86_EFL_CALC_SF(uResult, 64);
        fEfl |= X86_EFL_CALC_ZF(uResult);
        fEfl |= g_afParity[uResult & 0xff];
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_shr_u64,(uint64_t *puDst, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint64_t uDst = *puDst;
        uint64_t uResult = uDst >> cShift;
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts.  The AF bit is undefined, we
           always set it to zero atm. */
        AssertCompile(X86_EFL_CF_BIT == 0);
        uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
        fEfl |= (uDst >> (cShift - 1)) & X86_EFL_CF;
        fEfl |= (uDst >> 63) << X86_EFL_OF_BIT;
        fEfl |= X86_EFL_CALC_SF(uResult, 64);
        fEfl |= X86_EFL_CALC_ZF(uResult);
        fEfl |= g_afParity[uResult & 0xff];
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_sar_u64,(uint64_t *puDst, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint64_t uDst = *puDst;
        uint64_t uResult = (int64_t)uDst >> cShift;
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts (0).  The AF bit is undefined,
           we always set it to zero atm. */
        AssertCompile(X86_EFL_CF_BIT == 0);
        uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
        fEfl |= (uDst >> (cShift - 1)) & X86_EFL_CF;
        fEfl |= X86_EFL_CALC_SF(uResult, 64);
        fEfl |= X86_EFL_CALC_ZF(uResult);
        fEfl |= g_afParity[uResult & 0xff];
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_shld_u64,(uint64_t *puDst, uint64_t uSrc, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint64_t uDst = *puDst;
        uint64_t uResult;
        uResult  = uDst << cShift;
        uResult |= uSrc >> (64 - cShift);
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts.  The AF bit is undefined,
           we always set it to zero atm. */
        AssertCompile(X86_EFL_CF_BIT == 0);
        uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
        fEfl |= (uDst >> (64 - cShift)) & X86_EFL_CF;
        fEfl |= (uint32_t)((uDst >> 63) ^ (uint32_t)(uResult >> 63)) << X86_EFL_OF_BIT;
        fEfl |= X86_EFL_CALC_SF(uResult, 64);
        fEfl |= X86_EFL_CALC_ZF(uResult);
        fEfl |= g_afParity[uResult & 0xff];
        *pfEFlags = fEfl;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_shrd_u64,(uint64_t *puDst, uint64_t uSrc, uint8_t cShift, uint32_t *pfEFlags))
{
    cShift &= 63;
    if (cShift)
    {
        uint64_t uDst = *puDst;
        uint64_t uResult;
        uResult  = uDst >> cShift;
        uResult |= uSrc << (64 - cShift);
        *puDst = uResult;

        /* Calc EFLAGS.  The OF bit is undefined if cShift > 1, we implement
           it the same way as for 1 bit shifts.  The AF bit is undefined,
           we always set it to zero atm. */
        AssertCompile(X86_EFL_CF_BIT == 0);
        uint32_t fEfl = *pfEFlags & ~X86_EFL_STATUS_BITS;
        fEfl |= (uDst >> (cShift - 1)) & X86_EFL_CF;
        fEfl |= (uint32_t)((uDst >> 63) ^ (uint32_t)(uResult >> 63)) << X86_EFL_OF_BIT;
        fEfl |= X86_EFL_CALC_SF(uResult, 64);
        fEfl |= X86_EFL_CALC_ZF(uResult);
        fEfl |= g_afParity[uResult & 0xff];
        *pfEFlags = fEfl;
    }
}


/* misc */

IEM_DECL_IMPL_DEF(void, iemAImpl_xchg_u64,(uint64_t *puMem, uint64_t *puReg))
{
    /* XCHG implies LOCK. */
    uint64_t uOldMem = *puMem;
    while (!ASMAtomicCmpXchgExU64(puMem, *puReg, uOldMem, &uOldMem))
        ASMNopPause();
    *puReg = uOldMem;
}


#endif /* RT_ARCH_X86 */
#ifdef RT_ARCH_X86

/* multiplication and division */


IEM_DECL_IMPL_DEF(int, iemAImpl_mul_u64,(uint64_t *pu64RAX, uint64_t *pu64RDX, uint64_t u64Factor, uint32_t *pfEFlags))
{
    RTUINT128U Result;
    RTUInt128MulU64ByU64(&Result, *pu64RAX, u64Factor);
    *pu64RAX = Result.s.Lo;
    *pu64RDX = Result.s.Hi;

    /* MUL EFLAGS according to Skylake (similar to IMUL). */
    *pfEFlags &= ~(X86_EFL_SF | X86_EFL_CF | X86_EFL_OF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_PF);
    if (Result.s.Lo & RT_BIT_64(63))
        *pfEFlags |= X86_EFL_SF;
    *pfEFlags |= g_afParity[Result.s.Lo & 0xff]; /* (Skylake behaviour) */
    if (Result.s.Hi != 0)
        *pfEFlags |= X86_EFL_CF | X86_EFL_OF;
    return 0;
}


IEM_DECL_IMPL_DEF(int, iemAImpl_imul_u64,(uint64_t *pu64RAX, uint64_t *pu64RDX, uint64_t u64Factor, uint32_t *pfEFlags))
{
    RTUINT128U Result;
    *pfEFlags &= ~( X86_EFL_SF | X86_EFL_CF | X86_EFL_OF
                   /* Skylake always clears: */ | X86_EFL_AF | X86_EFL_ZF
                   /* Skylake may set: */       | X86_EFL_PF);

    if ((int64_t)*pu64RAX >= 0)
    {
        if ((int64_t)u64Factor >= 0)
        {
            RTUInt128MulU64ByU64(&Result, *pu64RAX, u64Factor);
            if (Result.s.Hi != 0 || Result.s.Lo >= UINT64_C(0x8000000000000000))
                *pfEFlags |= X86_EFL_CF | X86_EFL_OF;
        }
        else
        {
            RTUInt128MulU64ByU64(&Result, *pu64RAX, UINT64_C(0) - u64Factor);
            if (Result.s.Hi != 0 || Result.s.Lo > UINT64_C(0x8000000000000000))
                *pfEFlags |= X86_EFL_CF | X86_EFL_OF;
            RTUInt128AssignNeg(&Result);
        }
    }
    else
    {
        if ((int64_t)u64Factor >= 0)
        {
            RTUInt128MulU64ByU64(&Result, UINT64_C(0) - *pu64RAX, u64Factor);
            if (Result.s.Hi != 0 || Result.s.Lo > UINT64_C(0x8000000000000000))
                *pfEFlags |= X86_EFL_CF | X86_EFL_OF;
            RTUInt128AssignNeg(&Result);
        }
        else
        {
            RTUInt128MulU64ByU64(&Result, UINT64_C(0) - *pu64RAX, UINT64_C(0) - u64Factor);
            if (Result.s.Hi != 0 || Result.s.Lo >= UINT64_C(0x8000000000000000))
                *pfEFlags |= X86_EFL_CF | X86_EFL_OF;
        }
    }
    *pu64RAX = Result.s.Lo;
    if (Result.s.Lo & RT_BIT_64(63))
        *pfEFlags |= X86_EFL_SF;
    *pfEFlags |= g_afParity[Result.s.Lo & 0xff]; /* (Skylake behaviour) */
    *pu64RDX = Result.s.Hi;

    return 0;
}


IEM_DECL_IMPL_DEF(void, iemAImpl_imul_two_u64,(uint64_t *puDst, uint64_t uSrc, uint32_t *pfEFlags))
{
/** @todo Testcase: IMUL 2 and 3 operands. */
    uint64_t u64Ign;
    iemAImpl_imul_u64(puDst, &u64Ign, uSrc, pfEFlags);
}



IEM_DECL_IMPL_DEF(int, iemAImpl_div_u64,(uint64_t *pu64RAX, uint64_t *pu64RDX, uint64_t u64Divisor, uint32_t *pfEFlags))
{
    /* Note! Skylake leaves all flags alone. */
    RT_NOREF_PV(pfEFlags);

    if (   u64Divisor != 0
        && *pu64RDX < u64Divisor)
    {
        RTUINT128U Dividend;
        Dividend.s.Lo = *pu64RAX;
        Dividend.s.Hi = *pu64RDX;

        RTUINT128U Divisor;
        Divisor.s.Lo = u64Divisor;
        Divisor.s.Hi = 0;

        RTUINT128U Remainder;
        RTUINT128U Quotient;
# ifdef __GNUC__ /* GCC maybe really annoying in function. */
        Quotient.s.Lo = 0;
        Quotient.s.Hi = 0;
# endif
        RTUInt128DivRem(&Quotient, &Remainder, &Dividend, &Divisor);
        Assert(Quotient.s.Hi == 0);
        Assert(Remainder.s.Hi == 0);

        *pu64RAX = Quotient.s.Lo;
        *pu64RDX = Remainder.s.Lo;
        /** @todo research the undefined DIV flags. */
        return 0;

    }
    /* #DE */
    return VERR_IEM_ASPECT_NOT_IMPLEMENTED;
}


IEM_DECL_IMPL_DEF(int, iemAImpl_idiv_u64,(uint64_t *pu64RAX, uint64_t *pu64RDX, uint64_t u64Divisor, uint32_t *pfEFlags))
{
    /* Note! Skylake leaves all flags alone. */
    RT_NOREF_PV(pfEFlags);

    if (u64Divisor != 0)
    {
        /*
         * Convert to unsigned division.
         */
        RTUINT128U Dividend;
        Dividend.s.Lo = *pu64RAX;
        Dividend.s.Hi = *pu64RDX;
        if ((int64_t)*pu64RDX < 0)
            RTUInt128AssignNeg(&Dividend);

        RTUINT128U Divisor;
        Divisor.s.Hi = 0;
        if ((int64_t)u64Divisor >= 0)
            Divisor.s.Lo = u64Divisor;
        else
            Divisor.s.Lo = UINT64_C(0) - u64Divisor;

        RTUINT128U Remainder;
        RTUINT128U Quotient;
# ifdef __GNUC__ /* GCC maybe really annoying in function. */
        Quotient.s.Lo = 0;
        Quotient.s.Hi = 0;
# endif
        RTUInt128DivRem(&Quotient, &Remainder, &Dividend, &Divisor);

        /*
         * Setup the result, checking for overflows.
         */
        if ((int64_t)u64Divisor >= 0)
        {
            if ((int64_t)*pu64RDX >= 0)
            {
                /* Positive divisor, positive dividend => result positive. */
                if (Quotient.s.Hi == 0 && Quotient.s.Lo <= (uint64_t)INT64_MAX)
                {
                    *pu64RAX = Quotient.s.Lo;
                    *pu64RDX = Remainder.s.Lo;
                    return 0;
                }
            }
            else
            {
                /* Positive divisor, positive dividend => result negative. */
                if (Quotient.s.Hi == 0 && Quotient.s.Lo <= UINT64_C(0x8000000000000000))
                {
                    *pu64RAX = UINT64_C(0) - Quotient.s.Lo;
                    *pu64RDX = UINT64_C(0) - Remainder.s.Lo;
                    return 0;
                }
            }
        }
        else
        {
            if ((int64_t)*pu64RDX >= 0)
            {
                /* Negative divisor, positive dividend => negative quotient, positive remainder. */
                if (Quotient.s.Hi == 0 && Quotient.s.Lo <= UINT64_C(0x8000000000000000))
                {
                    *pu64RAX = UINT64_C(0) - Quotient.s.Lo;
                    *pu64RDX = Remainder.s.Lo;
                    return 0;
                }
            }
            else
            {
                /* Negative divisor, negative dividend => positive quotient, negative remainder. */
                if (Quotient.s.Hi == 0 && Quotient.s.Lo <= (uint64_t)INT64_MAX)
                {
                    *pu64RAX = Quotient.s.Lo;
                    *pu64RDX = UINT64_C(0) - Remainder.s.Lo;
                    return 0;
                }
            }
        }
    }
    /* #DE */
    return VERR_IEM_ASPECT_NOT_IMPLEMENTED;
}


#endif /* RT_ARCH_X86 */


IEM_DECL_IMPL_DEF(void, iemAImpl_arpl,(uint16_t *pu16Dst, uint16_t u16Src, uint32_t *pfEFlags))
{
    if ((*pu16Dst & X86_SEL_RPL) < (u16Src & X86_SEL_RPL))
    {
        *pu16Dst &= X86_SEL_MASK_OFF_RPL;
        *pu16Dst |= u16Src & X86_SEL_RPL;

        *pfEFlags |= X86_EFL_ZF;
    }
    else
        *pfEFlags &= ~X86_EFL_ZF;
}



IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg16b_fallback,(PRTUINT128U pu128Dst, PRTUINT128U pu128RaxRdx,
                                                      PRTUINT128U pu128RbxRcx, uint32_t *pEFlags))
{
    RTUINT128U u128Tmp = *pu128Dst;
    if (   u128Tmp.s.Lo == pu128RaxRdx->s.Lo
        && u128Tmp.s.Hi == pu128RaxRdx->s.Hi)
    {
        *pu128Dst = *pu128RbxRcx;
        *pEFlags |= X86_EFL_ZF;
    }
    else
    {
        *pu128RaxRdx = u128Tmp;
        *pEFlags &= ~X86_EFL_ZF;
    }
}


IEM_DECL_IMPL_DEF(void, iemAImpl_movsldup,(PCX86FXSTATE pFpuState, PRTUINT128U puDst, PCRTUINT128U puSrc))
{
    RT_NOREF(pFpuState);
    puDst->au32[0] = puSrc->au32[0];
    puDst->au32[1] = puSrc->au32[0];
    puDst->au32[2] = puSrc->au32[2];
    puDst->au32[3] = puSrc->au32[2];
}

#ifdef IEM_WITH_VEX

IEM_DECL_IMPL_DEF(void, iemAImpl_vmovsldup_256_rr,(PX86XSAVEAREA pXState, uint8_t iYRegDst, uint8_t iYRegSrc))
{
    pXState->x87.aXMM[iYRegDst].au32[0] = pXState->x87.aXMM[iYRegSrc].au32[0];
    pXState->x87.aXMM[iYRegDst].au32[1] = pXState->x87.aXMM[iYRegSrc].au32[0];
    pXState->x87.aXMM[iYRegDst].au32[2] = pXState->x87.aXMM[iYRegSrc].au32[2];
    pXState->x87.aXMM[iYRegDst].au32[3] = pXState->x87.aXMM[iYRegSrc].au32[2];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[0] = pXState->u.YmmHi.aYmmHi[iYRegSrc].au32[0];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[1] = pXState->u.YmmHi.aYmmHi[iYRegSrc].au32[0];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[2] = pXState->u.YmmHi.aYmmHi[iYRegSrc].au32[2];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[3] = pXState->u.YmmHi.aYmmHi[iYRegSrc].au32[2];
}


IEM_DECL_IMPL_DEF(void, iemAImpl_vmovsldup_256_rm,(PX86XSAVEAREA pXState, uint8_t iYRegDst, PCRTUINT256U pSrc))
{
    pXState->x87.aXMM[iYRegDst].au32[0]       = pSrc->au32[0];
    pXState->x87.aXMM[iYRegDst].au32[1]       = pSrc->au32[0];
    pXState->x87.aXMM[iYRegDst].au32[2]       = pSrc->au32[2];
    pXState->x87.aXMM[iYRegDst].au32[3]       = pSrc->au32[2];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[0] = pSrc->au32[4];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[1] = pSrc->au32[4];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[2] = pSrc->au32[6];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au32[3] = pSrc->au32[6];
}

#endif /* IEM_WITH_VEX */


IEM_DECL_IMPL_DEF(void, iemAImpl_movshdup,(PCX86FXSTATE pFpuState, PRTUINT128U puDst, PCRTUINT128U puSrc))
{
    RT_NOREF(pFpuState);
    puDst->au32[0] = puSrc->au32[1];
    puDst->au32[1] = puSrc->au32[1];
    puDst->au32[2] = puSrc->au32[3];
    puDst->au32[3] = puSrc->au32[3];
}


IEM_DECL_IMPL_DEF(void, iemAImpl_movddup,(PCX86FXSTATE pFpuState, PRTUINT128U puDst, uint64_t uSrc))
{
    RT_NOREF(pFpuState);
    puDst->au64[0] = uSrc;
    puDst->au64[1] = uSrc;
}

#ifdef IEM_WITH_VEX

IEM_DECL_IMPL_DEF(void, iemAImpl_vmovddup_256_rr,(PX86XSAVEAREA pXState, uint8_t iYRegDst, uint8_t iYRegSrc))
{
    pXState->x87.aXMM[iYRegDst].au64[0] = pXState->x87.aXMM[iYRegSrc].au64[0];
    pXState->x87.aXMM[iYRegDst].au64[1] = pXState->x87.aXMM[iYRegSrc].au64[0];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au64[0] = pXState->u.YmmHi.aYmmHi[iYRegSrc].au64[0];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au64[1] = pXState->u.YmmHi.aYmmHi[iYRegSrc].au64[0];
}

IEM_DECL_IMPL_DEF(void, iemAImpl_vmovddup_256_rm,(PX86XSAVEAREA pXState, uint8_t iYRegDst, PCRTUINT256U pSrc))
{
    pXState->x87.aXMM[iYRegDst].au64[0]       = pSrc->au64[0];
    pXState->x87.aXMM[iYRegDst].au64[1]       = pSrc->au64[0];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au64[0] = pSrc->au64[2];
    pXState->u.YmmHi.aYmmHi[iYRegDst].au64[1] = pSrc->au64[2];
}

#endif /* IEM_WITH_VEX */


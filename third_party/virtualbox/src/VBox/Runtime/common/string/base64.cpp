/* $Id: base64.cpp $ */
/** @file
 * IPRT - Base64, MIME content transfer encoding.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/base64.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/ctype.h>
#include <iprt/string.h>
#ifdef RT_STRICT
# include <iprt/asm.h>
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The line length used for encoding. */
#define RTBASE64_LINE_LEN   64

/** @name Special g_au8CharToVal values
 * @{ */
#define BASE64_SPACE        0xc0
#define BASE64_PAD          0xe0
#define BASE64_INVALID      0xff
/** @} */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Base64 character to value. (RFC 2045)
 * ASSUMES ASCII / UTF-8. */
static const uint8_t    g_au8CharToVal[256] =
{
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xc0, 0xc0, 0xc0,   0xc0, 0xc0, 0xff, 0xff, /* 0x00..0x0f */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0x10..0x1f */
    0xc0, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff,   62,   0xff, 0xff, 0xff,   63, /* 0x20..0x2f */
      52,   53,   54,   55,     56,   57,   58,   59,     60,   61, 0xff, 0xff,   0xff, 0xe0, 0xff, 0xff, /* 0x30..0x3f */
    0xff,    0,    1,    2,      3,    4,    5,    6,      7,    8,    9,   10,     11,   12,   13,   14, /* 0x40..0x4f */
      15,   16,   17,   18,     19,   20,   21,   22,     23,   24,   25, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0x50..0x5f */
    0xff,   26,   27,   28,     29,   30,   31,   32,     33,   34,   35,   36,     37,   38,   39,   40, /* 0x60..0x6f */
      41,   42,   43,   44,     45,   46,   47,   48,     49,   50,   51, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0x70..0x7f */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0x80..0x8f */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0x90..0x9f */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0xa0..0xaf */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0xb0..0xbf */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0xc0..0xcf */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0xd0..0xdf */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff, /* 0xe0..0xef */
    0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff  /* 0xf0..0xff */
};

/** Value to Base64 character. (RFC 2045) */
static const char       g_szValToChar[64+1] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


#ifdef RT_STRICT
/**
 * Perform table sanity checks on the first call.
 */
static void rtBase64Sanity(void)
{
    static bool s_fSane = false;
    if (RT_UNLIKELY(!s_fSane))
    {
        for (unsigned i = 0; i < 64; i++)
        {
            unsigned ch = g_szValToChar[i];
            Assert(ch);
            Assert(g_au8CharToVal[ch] == i);
        }

        for (unsigned i = 0; i < 256; i++)
        {
            uint8_t u8 = g_au8CharToVal[i];
            Assert(   (     u8 == BASE64_INVALID
                       &&   !RT_C_IS_ALNUM(i)
                       &&   !RT_C_IS_SPACE(i))
                   || (     u8 == BASE64_PAD
                       &&   i  == '=')
                   || (     u8 == BASE64_SPACE
                       &&   RT_C_IS_SPACE(i))
                   || (     u8 < 64
                       &&   (unsigned)g_szValToChar[u8] == i));
        }
        ASMAtomicWriteBool(&s_fSane, true);
    }
}
#endif /* RT_STRICT */


RTDECL(ssize_t) RTBase64DecodedSizeEx(const char *pszString, size_t cchStringMax, char **ppszEnd)
{
#ifdef RT_STRICT
    rtBase64Sanity();
#endif

    /*
     * Walk the string until a non-encoded or non-space character is encountered.
     */
    uint32_t    c6Bits = 0;
    uint8_t     u8     = BASE64_INVALID;
    unsigned    ch     = 0;
    AssertCompile(sizeof(char) == sizeof(uint8_t));

    while (cchStringMax > 0 && (ch = *pszString))
    {
        u8 = g_au8CharToVal[ch];
        if (u8 < 64)
            c6Bits++;
        else if (RT_UNLIKELY(u8 != BASE64_SPACE))
            break;

        /* advance */
        pszString++;
        cchStringMax--;
    }

    /*
     * Padding can only be found at the end and there is
     * only 1 or 2 padding chars. Deal with it first.
     */
    unsigned    cbPad = 0;
    if (u8 == BASE64_PAD)
    {
        cbPad = 1;
        c6Bits++;
        pszString++;
        cchStringMax--;
        while (cchStringMax > 0 && (ch = *pszString))
        {
            u8 = g_au8CharToVal[ch];
            if (u8 != BASE64_SPACE)
            {
                if (u8 != BASE64_PAD)
                    break;
                c6Bits++;
                cbPad++;
            }
            pszString++;
            cchStringMax--;
        }
        if (cbPad >= 3)
            return -1;
    }

    /*
     * Invalid char and no where to indicate where the
     * Base64 text ends? Return failure.
     */
    if (    u8 == BASE64_INVALID
        &&  !ppszEnd
        &&  ch)
        return -1;

    /*
     * Recalc 6-bit to 8-bit and adjust for padding.
     */
    size_t cb;
    if (c6Bits * 3 / 3 == c6Bits)
    {
        if ((c6Bits * 3 % 4) != 0)
            return -1;
        cb = c6Bits * 3 / 4;
    }
    else
    {
        if ((c6Bits * (uint64_t)3 % 4) != 0)
            return -1;
        cb = c6Bits * (uint64_t)3 / 4;
    }

    if (cb < cbPad)
        return -1;
    cb -= cbPad;

    if (ppszEnd)
        *ppszEnd = (char *)pszString;
    return cb;
}
RT_EXPORT_SYMBOL(RTBase64DecodedSizeEx);


RTDECL(ssize_t) RTBase64DecodedSize(const char *pszString, char **ppszEnd)
{
    return RTBase64DecodedSizeEx(pszString, RTSTR_MAX, ppszEnd);
}
RT_EXPORT_SYMBOL(RTBase64DecodedSize);


RTDECL(int) RTBase64DecodeEx(const char *pszString, size_t cchStringMax, void *pvData, size_t cbData,
                             size_t *pcbActual, char **ppszEnd)
{
#ifdef RT_STRICT
    rtBase64Sanity();
#endif

    /*
     * Process input in groups of 4 input / 3 output chars.
     */
    uint8_t     u8Trio[3] = { 0, 0, 0 }; /* shuts up gcc */
    uint8_t    *pbData    = (uint8_t *)pvData;
    unsigned    ch;
    uint8_t     u8;
    unsigned    c6Bits    = 0;
    AssertCompile(sizeof(char) == sizeof(uint8_t));

    for (;;)
    {
        /* The first 6-bit group. */
        while ((u8 = g_au8CharToVal[ch = cchStringMax > 0 ? (uint8_t)*pszString : 0]) == BASE64_SPACE)
            pszString++, cchStringMax--;
        if (u8 >= 64)
        {
            c6Bits = 0;
            break;
        }
        u8Trio[0] = u8 << 2;
        pszString++;
        cchStringMax--;

        /* The second 6-bit group. */
        while ((u8 = g_au8CharToVal[ch = cchStringMax > 0 ? (uint8_t)*pszString : 0]) == BASE64_SPACE)
            pszString++, cchStringMax--;
        if (u8 >= 64)
        {
            c6Bits = 1;
            break;
        }
        u8Trio[0] |= u8 >> 4;
        u8Trio[1]  = u8 << 4;
        pszString++;
        cchStringMax--;

        /* The third 6-bit group. */
        u8 = BASE64_INVALID;
        while ((u8 = g_au8CharToVal[ch = cchStringMax > 0 ? (uint8_t)*pszString : 0]) == BASE64_SPACE)
            pszString++, cchStringMax--;
        if (u8 >= 64)
        {
            c6Bits = 2;
            break;
        }
        u8Trio[1] |= u8 >> 2;
        u8Trio[2]  = u8 << 6;
        pszString++;
        cchStringMax--;

        /* The fourth 6-bit group. */
        u8 = BASE64_INVALID;
        while ((u8 = g_au8CharToVal[ch = cchStringMax > 0 ? (uint8_t)*pszString : 0]) == BASE64_SPACE)
            pszString++, cchStringMax--;
        if (u8 >= 64)
        {
            c6Bits = 3;
            break;
        }
        u8Trio[2] |= u8;
        pszString++;
        cchStringMax--;

        /* flush the trio */
        if (cbData < 3)
            return VERR_BUFFER_OVERFLOW;
        cbData -= 3;
        pbData[0] = u8Trio[0];
        pbData[1] = u8Trio[1];
        pbData[2] = u8Trio[2];
        pbData += 3;
    }

    /*
     * Padding can only be found at the end and there is
     * only 1 or 2 padding chars. Deal with it first.
     */
    unsigned cbPad = 0;
    if (u8 == BASE64_PAD)
    {
        cbPad = 1;
        pszString++;
        cchStringMax--;
        while (cchStringMax > 0 && (ch = (uint8_t)*pszString))
        {
            u8 = g_au8CharToVal[ch];
            if (u8 != BASE64_SPACE)
            {
                if (u8 != BASE64_PAD)
                    break;
                cbPad++;
            }
            pszString++;
            cchStringMax--;
        }
        if (cbPad >= 3)
            return VERR_INVALID_BASE64_ENCODING;
    }

    /*
     * Invalid char and no where to indicate where the
     * Base64 text ends? Return failure.
     */
    if (    u8 == BASE64_INVALID
        &&  !ppszEnd
        &&  ch != '\0')
        return VERR_INVALID_BASE64_ENCODING;

    /*
     * Check padding vs. pending sextets, if anything left to do finish it off.
     */
    if (c6Bits || cbPad)
    {
        if (c6Bits + cbPad != 4)
            return VERR_INVALID_BASE64_ENCODING;

        switch (c6Bits)
        {
            case 1:
                u8Trio[1] = u8Trio[2] = 0;
                break;
            case 2:
                u8Trio[2] = 0;
                break;
            case 3:
            default:
                break;
        }
        switch (3 - cbPad)
        {
            case 1:
                if (cbData < 1)
                    return VERR_BUFFER_OVERFLOW;
                cbData--;
                pbData[0] = u8Trio[0];
                pbData++;
                break;

            case 2:
                if (cbData < 2)
                    return VERR_BUFFER_OVERFLOW;
                cbData -= 2;
                pbData[0] = u8Trio[0];
                pbData[1] = u8Trio[1];
                pbData += 2;
                break;

            default:
                break;
        }
    }

    /*
     * Set optional return values and return successfully.
     */
    if (ppszEnd)
        *ppszEnd = (char *)pszString;
    if (pcbActual)
        *pcbActual = pbData - (uint8_t *)pvData;
    return VINF_SUCCESS;
}
RT_EXPORT_SYMBOL(RTBase64DecodeEx);


RTDECL(int) RTBase64Decode(const char *pszString, void *pvData, size_t cbData, size_t *pcbActual, char **ppszEnd)
{
    return RTBase64DecodeEx(pszString, RTSTR_MAX, pvData, cbData, pcbActual, ppszEnd);
}
RT_EXPORT_SYMBOL(RTBase64Decode);


/**
 * Calculates the length of the Base64 encoding of a given number of bytes of
 * data.
 *
 * This will assume line breaks every 64 chars. A RTBase64EncodedLengthEx
 * function can be added if closer control over the output is found to be
 * required.
 *
 * @returns The Base64 string length.
 * @param   cbData      The number of bytes to encode.
 */
RTDECL(size_t) RTBase64EncodedLength(size_t cbData)
{
    if (cbData * 8 / 8 != cbData)
    {
        AssertReturn(sizeof(size_t) == sizeof(uint64_t), ~(size_t)0);
        uint64_t cch = cbData * (uint64_t)8;
        while (cch % 24)
            cch += 8;
        cch /= 6;

        cch += ((cch - 1) / RTBASE64_LINE_LEN) * RTBASE64_EOL_SIZE;
        return cch;
    }

    size_t cch = cbData * 8;
    while (cch % 24)
        cch += 8;
    cch /= 6;

    cch += ((cch - 1) / RTBASE64_LINE_LEN) * RTBASE64_EOL_SIZE;
    return cch;
}
RT_EXPORT_SYMBOL(RTBase64EncodedLength);


/**
 * Encodes the specifed data into a Base64 string, the caller supplies the
 * output buffer.
 *
 * This will make the same assumptions about line breaks and EOL size as
 * RTBase64EncodedLength() does. A RTBase64EncodeEx function can be added if
 * more strict control over the output formatting is found necessary.
 *
 * @returns IRPT status code.
 * @retval  VERR_BUFFER_OVERFLOW if the output buffer is too small. The buffer
 *          may contain an invalid Base64 string.
 *
 * @param   pvData      The data to encode.
 * @param   cbData      The number of bytes to encode.
 * @param   pszBuf      Where to put the Base64 string.
 * @param   cbBuf       The size of the output buffer, including the terminator.
 * @param   pcchActual  The actual number of characters returned.
 */
RTDECL(int) RTBase64Encode(const void *pvData, size_t cbData, char *pszBuf, size_t cbBuf, size_t *pcchActual)
{
    /*
     * Process whole "trios" of input data.
     */
    uint8_t         u8A;
    uint8_t         u8B;
    uint8_t         u8C;
    size_t          cbLineFeed = cbBuf - RTBASE64_LINE_LEN;
    const uint8_t  *pbSrc      = (const uint8_t *)pvData;
    char           *pchDst     = pszBuf;
    while (cbData >= 3)
    {
        if (cbBuf < 4 + 1)
            return VERR_BUFFER_OVERFLOW;

        /* encode */
        u8A = pbSrc[0];
        pchDst[0] = g_szValToChar[u8A >> 2];
        u8B = pbSrc[1];
        pchDst[1] = g_szValToChar[((u8A << 4) & 0x3f) | (u8B >> 4)];
        u8C = pbSrc[2];
        pchDst[2] = g_szValToChar[((u8B << 2) & 0x3f) | (u8C >> 6)];
        pchDst[3] = g_szValToChar[u8C & 0x3f];

        /* advance */
        cbBuf  -= 4;
        pchDst += 4;
        cbData -= 3;
        pbSrc  += 3;

        /* deal out linefeeds */
        if (cbBuf == cbLineFeed && cbData)
        {
            if (cbBuf < RTBASE64_EOL_SIZE + 1)
                return VERR_BUFFER_OVERFLOW;
            cbBuf -= RTBASE64_EOL_SIZE;
            if (RTBASE64_EOL_SIZE == 2)
                *pchDst++ = '\r';
            *pchDst++ = '\n';
            cbLineFeed = cbBuf - RTBASE64_LINE_LEN;
        }
    }

    /*
     * Deal with the odd bytes and string termination.
     */
    if (cbData)
    {
        if (cbBuf < 4 + 1)
            return VERR_BUFFER_OVERFLOW;
        switch (cbData)
        {
            case 1:
                u8A = pbSrc[0];
                pchDst[0] = g_szValToChar[u8A >> 2];
                pchDst[1] = g_szValToChar[(u8A << 4) & 0x3f];
                pchDst[2] = '=';
                pchDst[3] = '=';
                break;
            case 2:
                u8A = pbSrc[0];
                pchDst[0] = g_szValToChar[u8A >> 2];
                u8B = pbSrc[1];
                pchDst[1] = g_szValToChar[((u8A << 4) & 0x3f) | (u8B >> 4)];
                pchDst[2] = g_szValToChar[(u8B << 2) & 0x3f];
                pchDst[3] = '=';
                break;
        }
        pchDst += 4;
    }

    *pchDst = '\0';

    if (pcchActual)
        *pcchActual = pchDst - pszBuf;
    return VINF_SUCCESS;
}
RT_EXPORT_SYMBOL(RTBase64Encode);


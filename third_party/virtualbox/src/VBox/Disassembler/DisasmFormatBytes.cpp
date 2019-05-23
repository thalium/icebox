/* $Id: DisasmFormatBytes.cpp $ */
/** @file
 * VBox Disassembler - Helper for formatting the opcode bytes.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include "DisasmInternal.h"
#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/err.h>


/**
 * Helper function for formatting the opcode bytes.
 *
 * @returns The number of output bytes.
 *
 * @param   pDis    Pointer to the disassembler state.
 * @param   pszDst  The output buffer.
 * @param   cchDst  The size of the output buffer.
 * @param   fFlags  The flags passed to the formatter.
 */
size_t disFormatBytes(PCDISSTATE pDis, char *pszDst, size_t cchDst, uint32_t fFlags)
{
    size_t      cchOutput = 0;
    uint32_t    cb        = pDis->cbInstr;
    AssertStmt(cb <= 16, cb = 16);

#define PUT_C(ch) \
            do { \
                cchOutput++; \
                if (cchDst > 1) \
                { \
                    cchDst--; \
                    *pszDst++ = (ch); \
                } \
            } while (0)
#define PUT_NUM(cch, fmt, num) \
            do { \
                 cchOutput += (cch); \
                 if (cchDst > 1) \
                 { \
                    const size_t cchTmp = RTStrPrintf(pszDst, cchDst, fmt, (num)); \
                    pszDst += cchTmp; \
                    cchDst -= cchTmp; \
                 } \
            } while (0)


    if (fFlags & DIS_FMT_FLAGS_BYTES_BRACKETS)
        PUT_C('[');

    for (uint32_t i = 0; i < cb; i++)
    {
        if (i != 0 && (fFlags & DIS_FMT_FLAGS_BYTES_SPACED))
            PUT_NUM(3, " %02x", pDis->abInstr[i]);
        else
            PUT_NUM(2, "%02x", pDis->abInstr[i]);
    }

    if (fFlags & DIS_FMT_FLAGS_BYTES_BRACKETS)
        PUT_C(']');

    /* Terminate it just in case. */
    if (cchDst >= 1)
        *pszDst = '\0';

#undef PUT_C
#undef PUT_NUM
    return cchOutput;
}


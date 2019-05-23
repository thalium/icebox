/* $Id: DBGCCmdHlp.cpp $ */
/** @file
 * DBGC - Debugger Console, Command Helpers.
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
#define LOG_GROUP LOG_GROUP_DBGC
#include <VBox/dbg.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/pgm.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/mem.h>
#include <iprt/string.h>

#include "DBGCInternal.h"



/**
 * @interface_method_impl{DBGCCMDHLP,pfnPrintf}
 */
static DECLCALLBACK(int) dbgcHlpPrintf(PDBGCCMDHLP pCmdHlp, size_t *pcbWritten, const char *pszFormat, ...)
{
    /*
     * Do the formatting and output.
     */
    va_list args;
    va_start(args, pszFormat);
    int rc = pCmdHlp->pfnPrintfV(pCmdHlp, pcbWritten, pszFormat, args);
    va_end(args);

    return rc;
}


/**
 * Outputs a string in quotes.
 *
 * @returns The number of bytes formatted.
 * @param   pfnOutput       Pointer to output function.
 * @param   pvArgOutput     Argument for the output function.
 * @param   chQuote         The quote character.
 * @param   psz             The string to quote.
 * @param   cch             The string length.
 */
static size_t dbgcStringOutputInQuotes(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput, char chQuote, const char *psz, size_t cch)
{
    size_t cchOutput = pfnOutput(pvArgOutput, &chQuote, 1);

    while (cch > 0)
    {
        char *pchQuote = (char *)memchr(psz, chQuote, cch);
        if (!pchQuote)
        {
            cchOutput += pfnOutput(pvArgOutput, psz, cch);
            break;
        }
        size_t cchSub = pchQuote - psz + 1;
        cchOutput += pfnOutput(pvArgOutput, psz, cchSub);
        cchOutput += pfnOutput(pvArgOutput, &chQuote, 1);
        cch    -= cchSub;
        psz    += cchSub;
    }

    cchOutput += pfnOutput(pvArgOutput, &chQuote, 1);
    return cchOutput;
}


/**
 * Callback to format non-standard format specifiers, employed by dbgcPrintfV
 * and others.
 *
 * @returns The number of bytes formatted.
 * @param   pvArg           Formatter argument.
 * @param   pfnOutput       Pointer to output function.
 * @param   pvArgOutput     Argument for the output function.
 * @param   ppszFormat      Pointer to the format string pointer. Advance this till the char
 *                          after the format specifier.
 * @param   pArgs           Pointer to the argument list. Use this to fetch the arguments.
 * @param   cchWidth        Format Width. -1 if not specified.
 * @param   cchPrecision    Format Precision. -1 if not specified.
 * @param   fFlags          Flags (RTSTR_NTFS_*).
 * @param   chArgSize       The argument size specifier, 'l' or 'L'.
 */
static DECLCALLBACK(size_t) dbgcStringFormatter(void *pvArg, PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                                const char **ppszFormat, va_list *pArgs, int cchWidth,
                                                int cchPrecision, unsigned fFlags, char chArgSize)
{
    NOREF(cchWidth); NOREF(cchPrecision); NOREF(fFlags); NOREF(chArgSize); NOREF(pvArg);
    if (**ppszFormat != 'D')
    {
        (*ppszFormat)++;
        return 0;
    }

    (*ppszFormat)++;
    switch (**ppszFormat)
    {
        /*
         * Print variable without range.
         * The argument is a const pointer to the variable.
         */
        case 'V':
        {
            (*ppszFormat)++;
            PCDBGCVAR   pVar = va_arg(*pArgs, PCDBGCVAR);
            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%RGv", pVar->u.GCFlat);
                case DBGCVAR_TYPE_GC_FAR:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%04x:%08x", pVar->u.GCFar.sel, pVar->u.GCFar.off);
                case DBGCVAR_TYPE_GC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%%%RGp", pVar->u.GCPhys);
                case DBGCVAR_TYPE_HC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%#%RHv", (uintptr_t)pVar->u.pvHCFlat);
                case DBGCVAR_TYPE_HC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "#%%%%%RHp", pVar->u.HCPhys);
                case DBGCVAR_TYPE_NUMBER:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%llx", pVar->u.u64Number);
                case DBGCVAR_TYPE_STRING:
                    return dbgcStringOutputInQuotes(pfnOutput, pvArgOutput, '"', pVar->u.pszString, (size_t)pVar->u64Range);
                case DBGCVAR_TYPE_SYMBOL:
                    return dbgcStringOutputInQuotes(pfnOutput, pvArgOutput, '\'', pVar->u.pszString, (size_t)pVar->u64Range);

                case DBGCVAR_TYPE_UNKNOWN:
                default:
                    return pfnOutput(pvArgOutput, "??", 2);
            }
        }

        /*
         * Print variable with range.
         * The argument is a const pointer to the variable.
         */
        case 'v':
        {
            (*ppszFormat)++;
            PCDBGCVAR   pVar = va_arg(*pArgs, PCDBGCVAR);

            char szRange[32];
            switch (pVar->enmRangeType)
            {
                case DBGCVAR_RANGE_NONE:
                    szRange[0] = '\0';
                    break;
                case DBGCVAR_RANGE_ELEMENTS:
                    RTStrPrintf(szRange, sizeof(szRange), " L %llx", pVar->u64Range);
                    break;
                case DBGCVAR_RANGE_BYTES:
                    RTStrPrintf(szRange, sizeof(szRange), " LB %llx", pVar->u64Range);
                    break;
            }

            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%RGv%s", pVar->u.GCFlat, szRange);
                case DBGCVAR_TYPE_GC_FAR:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%04x:%08x%s", pVar->u.GCFar.sel, pVar->u.GCFar.off, szRange);
                case DBGCVAR_TYPE_GC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%%%RGp%s", pVar->u.GCPhys, szRange);
                case DBGCVAR_TYPE_HC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%#%RHv%s", (uintptr_t)pVar->u.pvHCFlat, szRange);
                case DBGCVAR_TYPE_HC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "#%%%%%RHp%s", pVar->u.HCPhys, szRange);
                case DBGCVAR_TYPE_NUMBER:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%llx%s", pVar->u.u64Number, szRange);
                case DBGCVAR_TYPE_STRING:
                    return dbgcStringOutputInQuotes(pfnOutput, pvArgOutput, '"', pVar->u.pszString, (size_t)pVar->u64Range);
                case DBGCVAR_TYPE_SYMBOL:
                    return dbgcStringOutputInQuotes(pfnOutput, pvArgOutput, '\'', pVar->u.pszString, (size_t)pVar->u64Range);

                case DBGCVAR_TYPE_UNKNOWN:
                default:
                    return pfnOutput(pvArgOutput, "??", 2);
            }
        }

        default:
            AssertMsgFailed(("Invalid format type '%s'!\n", **ppszFormat));
            return 0;
    }
}


/**
 * Output callback employed by dbgcHlpPrintfV.
 *
 * @returns number of bytes written.
 * @param   pvArg       User argument.
 * @param   pachChars   Pointer to an array of utf-8 characters.
 * @param   cbChars     Number of bytes in the character array pointed to by pachChars.
 */
static DECLCALLBACK(size_t) dbgcFormatOutput(void *pvArg, const char *pachChars, size_t cbChars)
{
    PDBGC pDbgc = (PDBGC)pvArg;
    if (cbChars)
    {
        int rc = pDbgc->pBack->pfnWrite(pDbgc->pBack, pachChars, cbChars, NULL);
        if (RT_SUCCESS(rc))
            pDbgc->chLastOutput = pachChars[cbChars - 1];
        else
        {
            pDbgc->rcOutput = rc;
            cbChars = 0;
        }
    }

    return cbChars;
}



/**
 * @interface_method_impl{DBGCCMDHLP,pfnPrintfV}
 */
static DECLCALLBACK(int) dbgcHlpPrintfV(PDBGCCMDHLP pCmdHlp, size_t *pcbWritten, const char *pszFormat, va_list args)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Do the formatting and output.
     */
    pDbgc->rcOutput = 0;
    size_t cb = RTStrFormatV(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, pszFormat, args);

    if (pcbWritten)
        *pcbWritten = cb;

    return pDbgc->rcOutput;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnStrPrintfV}
 */
static DECLCALLBACK(size_t) dbgcHlpStrPrintfV(PDBGCCMDHLP pCmdHlp, char *pszBuf, size_t cbBuf,
                                              const char *pszFormat, va_list va)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    return RTStrPrintfExV(dbgcStringFormatter, pDbgc, pszBuf, cbBuf, pszFormat, va);
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnStrPrintf}
 */
static DECLCALLBACK(size_t) dbgcHlpStrPrintf(PDBGCCMDHLP pCmdHlp, char *pszBuf, size_t cbBuf, const char *pszFormat, ...)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    va_list va;
    va_start(va, pszFormat);
    size_t cch = RTStrPrintfExV(dbgcStringFormatter, pDbgc, pszBuf, cbBuf, pszFormat, va);
    va_end(va);
    return cch;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVBoxErrorV}
 */
static DECLCALLBACK(int) dbgcHlpVBoxErrorV(PDBGCCMDHLP pCmdHlp, int rc, const char *pszFormat, va_list args)
{
    switch (rc)
    {
        case VINF_SUCCESS:
            break;

        default:
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, "error: %Rrc: %s", rc, pszFormat ? " " : "\n");
            if (RT_SUCCESS(rc) && pszFormat)
                rc = pCmdHlp->pfnPrintfV(pCmdHlp, NULL, pszFormat, args);
            if (RT_SUCCESS(rc))
                rc = VERR_DBGC_COMMAND_FAILED;
            break;
    }
    return rc;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVBoxError}
 */
static DECLCALLBACK(int) dbgcHlpVBoxError(PDBGCCMDHLP pCmdHlp, int rc, const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    int rcRet = pCmdHlp->pfnVBoxErrorV(pCmdHlp, rc, pszFormat, args);
    va_end(args);
    return rcRet;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnMemRead}
 */
static DECLCALLBACK(int) dbgcHlpMemRead(PDBGCCMDHLP pCmdHlp, void *pvBuffer, size_t cbRead, PCDBGCVAR pVarPointer, size_t *pcbRead)
{
    PDBGC       pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    DBGFADDRESS Address;
    int         rc;

    /*
     * Dummy check.
     */
    if (cbRead == 0)
    {
        if (*pcbRead)
            *pcbRead = 0;
        return VINF_SUCCESS;
    }

    /*
     * Convert Far addresses getting size and the correct base address.
     * Getting and checking the size is what makes this messy and slow.
     */
    DBGCVAR Var = *pVarPointer;
    switch (pVarPointer->enmType)
    {
        case DBGCVAR_TYPE_GC_FAR:
            /* Use DBGFR3AddrFromSelOff for the conversion. */
            Assert(pDbgc->pUVM);
            rc = DBGFR3AddrFromSelOff(pDbgc->pUVM, pDbgc->idCpu, &Address, Var.u.GCFar.sel, Var.u.GCFar.off);
            if (RT_FAILURE(rc))
                return rc;

            /* don't bother with flat selectors (for now). */
            if (!DBGFADDRESS_IS_FLAT(&Address))
            {
                DBGFSELINFO SelInfo;
                rc = DBGFR3SelQueryInfo(pDbgc->pUVM, pDbgc->idCpu, Address.Sel,
                                        DBGFSELQI_FLAGS_DT_GUEST | DBGFSELQI_FLAGS_DT_ADJ_64BIT_MODE, &SelInfo);
                if (RT_SUCCESS(rc))
                {
                    RTGCUINTPTR cb; /* -1 byte */
                    if (DBGFSelInfoIsExpandDown(&SelInfo))
                    {
                        if (    !SelInfo.u.Raw.Gen.u1Granularity
                            &&  Address.off > UINT16_C(0xffff))
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        if (Address.off <= SelInfo.cbLimit)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cb = (SelInfo.u.Raw.Gen.u1Granularity ? UINT32_C(0xffffffff) : UINT32_C(0xffff)) - Address.off;
                    }
                    else
                    {
                        if (Address.off > SelInfo.cbLimit)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cb = SelInfo.cbLimit - Address.off;
                    }
                    if (cbRead - 1 > cb)
                    {
                        if (!pcbRead)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cbRead = cb + 1;
                    }
                }
            }
            Var.enmType = DBGCVAR_TYPE_GC_FLAT;
            Var.u.GCFlat = Address.FlatPtr;
            break;

        case DBGCVAR_TYPE_GC_FLAT:
        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_PHYS:
            break;

        default:
            return VERR_NOT_IMPLEMENTED;
    }



    /*
     * Copy page by page.
     */
    size_t cbLeft = cbRead;
    for (;;)
    {
        /*
         * Calc read size.
         */
        size_t cb = RT_MIN(PAGE_SIZE, cbLeft);
        switch (pVarPointer->enmType)
        {
            case DBGCVAR_TYPE_GC_FLAT: cb = RT_MIN(cb, PAGE_SIZE - (Var.u.GCFlat & PAGE_OFFSET_MASK)); break;
            case DBGCVAR_TYPE_GC_PHYS: cb = RT_MIN(cb, PAGE_SIZE - (Var.u.GCPhys & PAGE_OFFSET_MASK)); break;
            case DBGCVAR_TYPE_HC_FLAT: cb = RT_MIN(cb, PAGE_SIZE - ((uintptr_t)Var.u.pvHCFlat & PAGE_OFFSET_MASK)); break;
            case DBGCVAR_TYPE_HC_PHYS: cb = RT_MIN(cb, PAGE_SIZE - ((size_t)Var.u.HCPhys & PAGE_OFFSET_MASK)); break; /* size_t: MSC has braindead loss of data warnings! */
            default: break;
        }

        /*
         * Perform read.
         */
        switch (Var.enmType)
        {
            case DBGCVAR_TYPE_GC_FLAT:
                rc = DBGFR3MemRead(pDbgc->pUVM, pDbgc->idCpu,
                                   DBGFR3AddrFromFlat(pDbgc->pUVM, &Address, Var.u.GCFlat),
                                   pvBuffer, cb);
                break;

            case DBGCVAR_TYPE_GC_PHYS:
                rc = DBGFR3MemRead(pDbgc->pUVM, pDbgc->idCpu,
                                   DBGFR3AddrFromPhys(pDbgc->pUVM, &Address, Var.u.GCPhys),
                                   pvBuffer, cb);
                break;

            case DBGCVAR_TYPE_HC_PHYS:
            case DBGCVAR_TYPE_HC_FLAT:
            {
                DBGCVAR Var2;
                rc = dbgcOpAddrFlat(pDbgc, &Var, DBGCVAR_CAT_ANY, &Var2);
                if (RT_SUCCESS(rc))
                {
                    /** @todo protect this!!! */
                    memcpy(pvBuffer, Var2.u.pvHCFlat, cb);
                    rc = 0;
                }
                else
                    rc = VERR_INVALID_POINTER;
                break;
            }

            default:
                rc = VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;
        }

        /*
         * Check for failure.
         */
        if (RT_FAILURE(rc))
        {
            if (pcbRead && (*pcbRead = cbRead - cbLeft) > 0)
                return VINF_SUCCESS;
            return rc;
        }

        /*
         * Next.
         */
        cbLeft -= cb;
        if (!cbLeft)
            break;
        pvBuffer = (char *)pvBuffer + cb;
        rc = DBGCCmdHlpEval(pCmdHlp, &Var, "%DV + %#zx", &Var, cb);
        if (RT_FAILURE(rc))
        {
            if (pcbRead && (*pcbRead = cbRead - cbLeft) > 0)
                return VINF_SUCCESS;
            return rc;
        }
    }

    /*
     * Done
     */
    if (pcbRead)
        *pcbRead = cbRead;
    return 0;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnMemWrite}
 */
static DECLCALLBACK(int) dbgcHlpMemWrite(PDBGCCMDHLP pCmdHlp, const void *pvBuffer, size_t cbWrite, PCDBGCVAR pVarPointer, size_t *pcbWritten)
{
    PDBGC       pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    DBGFADDRESS Address;
    int         rc;

    /*
     * Dummy check.
     */
    if (cbWrite == 0)
    {
        if (*pcbWritten)
            *pcbWritten = 0;
        return VINF_SUCCESS;
    }

    /*
     * Convert Far addresses getting size and the correct base address.
     * Getting and checking the size is what makes this messy and slow.
     */
    DBGCVAR Var = *pVarPointer;
    switch (pVarPointer->enmType)
    {
        case DBGCVAR_TYPE_GC_FAR:
        {
            /* Use DBGFR3AddrFromSelOff for the conversion. */
            Assert(pDbgc->pUVM);
            rc = DBGFR3AddrFromSelOff(pDbgc->pUVM, pDbgc->idCpu, &Address, Var.u.GCFar.sel, Var.u.GCFar.off);
            if (RT_FAILURE(rc))
                return rc;

            /* don't bother with flat selectors (for now). */
            if (!DBGFADDRESS_IS_FLAT(&Address))
            {
                DBGFSELINFO SelInfo;
                rc = DBGFR3SelQueryInfo(pDbgc->pUVM, pDbgc->idCpu, Address.Sel,
                                        DBGFSELQI_FLAGS_DT_GUEST | DBGFSELQI_FLAGS_DT_ADJ_64BIT_MODE, &SelInfo);
                if (RT_SUCCESS(rc))
                {
                    RTGCUINTPTR cb; /* -1 byte */
                    if (DBGFSelInfoIsExpandDown(&SelInfo))
                    {
                        if (    !SelInfo.u.Raw.Gen.u1Granularity
                            &&  Address.off > UINT16_C(0xffff))
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        if (Address.off <= SelInfo.cbLimit)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cb = (SelInfo.u.Raw.Gen.u1Granularity ? UINT32_C(0xffffffff) : UINT32_C(0xffff)) - Address.off;
                    }
                    else
                    {
                        if (Address.off > SelInfo.cbLimit)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cb = SelInfo.cbLimit - Address.off;
                    }
                    if (cbWrite - 1 > cb)
                    {
                        if (!pcbWritten)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cbWrite = cb + 1;
                    }
                }
            }
            Var.enmType = DBGCVAR_TYPE_GC_FLAT;
            Var.u.GCFlat = Address.FlatPtr;
        }
        RT_FALL_THRU();
        case DBGCVAR_TYPE_GC_FLAT:
            rc = DBGFR3MemWrite(pDbgc->pUVM, pDbgc->idCpu,
                                DBGFR3AddrFromFlat(pDbgc->pUVM, &Address, Var.u.GCFlat),
                                pvBuffer, cbWrite);
            if (pcbWritten && RT_SUCCESS(rc))
                *pcbWritten = cbWrite;
            return rc;

        case DBGCVAR_TYPE_GC_PHYS:
            rc = DBGFR3MemWrite(pDbgc->pUVM, pDbgc->idCpu,
                                DBGFR3AddrFromPhys(pDbgc->pUVM, &Address, Var.u.GCPhys),
                                pvBuffer, cbWrite);
            if (pcbWritten && RT_SUCCESS(rc))
                *pcbWritten = cbWrite;
            return rc;

        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_PHYS:
        {
            /*
             * Copy HC memory page by page.
             */
            if (pcbWritten)
                *pcbWritten = 0;
            while (cbWrite > 0)
            {
                /* convert to flat address */
                DBGCVAR Var2;
                rc = dbgcOpAddrFlat(pDbgc, &Var, DBGCVAR_CAT_ANY, &Var2);
                if (RT_FAILURE(rc))
                {
                    if (pcbWritten && *pcbWritten)
                        return -VERR_INVALID_POINTER;
                    return VERR_INVALID_POINTER;
                }

                /* calc size. */
                size_t cbChunk = PAGE_SIZE;
                cbChunk -= (uintptr_t)Var.u.pvHCFlat & PAGE_OFFSET_MASK;
                if (cbChunk > cbWrite)
                    cbChunk = cbWrite;

                /** @todo protect this!!! */
                memcpy(Var2.u.pvHCFlat, pvBuffer, cbChunk);

                /* advance */
                if (Var.enmType == DBGCVAR_TYPE_HC_FLAT)
                    Var.u.pvHCFlat = (uint8_t *)Var.u.pvHCFlat + cbChunk;
                else
                    Var.u.HCPhys += cbChunk;
                pvBuffer = (uint8_t const *)pvBuffer + cbChunk;
                if (pcbWritten)
                    *pcbWritten += cbChunk;
                cbWrite -= cbChunk;
            }

            return VINF_SUCCESS;
        }

        default:
            return VERR_NOT_IMPLEMENTED;
    }
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnExec}
 */
static DECLCALLBACK(int) dbgcHlpExec(PDBGCCMDHLP pCmdHlp, const char *pszExpr, ...)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    /* Save the scratch state. */
    char       *pszScratch  = pDbgc->pszScratch;
    unsigned    iArg        = pDbgc->iArg;

    /*
     * Format the expression.
     */
    va_list args;
    va_start(args, pszExpr);
    size_t cbScratch = sizeof(pDbgc->achScratch) - (pDbgc->pszScratch - &pDbgc->achScratch[0]);
    size_t cb = RTStrPrintfExV(dbgcStringFormatter, pDbgc, pDbgc->pszScratch, cbScratch, pszExpr, args);
    va_end(args);
    if (cb >= cbScratch)
        return VERR_BUFFER_OVERFLOW;

    /*
     * Execute the command.
     * We save and restore the arg index and scratch buffer pointer.
     */
    pDbgc->pszScratch = pDbgc->pszScratch + cb + 1;
    int rc = dbgcEvalCommand(pDbgc, pszScratch, cb, false /* fNoExecute */);

    /* Restore the scratch state. */
    pDbgc->iArg         = iArg;
    pDbgc->pszScratch   = pszScratch;

    return rc;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnEvalV}
 */
static DECLCALLBACK(int) dbgcHlpEvalV(PDBGCCMDHLP pCmdHlp, PDBGCVAR pResult, const char *pszExpr, va_list va)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Format the expression.
     */
    char szExprFormatted[2048];
    size_t cb = RTStrPrintfExV(dbgcStringFormatter, pDbgc, szExprFormatted, sizeof(szExprFormatted), pszExpr, va);
    /* ignore overflows. */

    return dbgcEvalSub(pDbgc, &szExprFormatted[0], cb, DBGCVAR_CAT_ANY, pResult);
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnFailV}
 */
static DECLCALLBACK(int) dbgcHlpFailV(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, const char *pszFormat, va_list va)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Do the formatting and output.
     */
    pDbgc->rcOutput = VINF_SUCCESS;
    RTStrFormat(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, "%s: error: ", pCmd->pszCmd);
    if (RT_FAILURE(pDbgc->rcOutput))
        return pDbgc->rcOutput;
    RTStrFormatV(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, pszFormat, va);
    if (RT_FAILURE(pDbgc->rcOutput))
        return pDbgc->rcOutput;
    if (pDbgc->chLastOutput != '\n')
        dbgcFormatOutput(pDbgc, "\n", 1);
    return VERR_DBGC_COMMAND_FAILED;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnFailRcV}
 */
static DECLCALLBACK(int) dbgcHlpFailRcV(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, int rc, const char *pszFormat, va_list va)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Do the formatting and output.
     */
    pDbgc->rcOutput = VINF_SUCCESS;
    RTStrFormat(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, "%s: error: ", pCmd->pszCmd);
    if (RT_FAILURE(pDbgc->rcOutput))
        return pDbgc->rcOutput;
    RTStrFormatV(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, pszFormat, va);
    if (RT_FAILURE(pDbgc->rcOutput))
        return pDbgc->rcOutput;
    RTStrFormat(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, ": %Rrc\n", rc);
    if (RT_FAILURE(pDbgc->rcOutput))
        return pDbgc->rcOutput;

    return VERR_DBGC_COMMAND_FAILED;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnParserError}
 */
static DECLCALLBACK(int) dbgcHlpParserError(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, int iArg, const char *pszExpr, unsigned iLine)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Do the formatting and output.
     */
    pDbgc->rcOutput = VINF_SUCCESS;
    RTStrFormat(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, "%s: parser error: iArg=%d iLine=%u pszExpr=%s\n",
                pCmd->pszCmd, iArg, iLine, pszExpr);
    if (RT_FAILURE(pDbgc->rcOutput))
        return pDbgc->rcOutput;
    return VERR_DBGC_COMMAND_FAILED;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVarToDbgfAddr}
 */
static DECLCALLBACK(int) dbgcHlpVarToDbgfAddr(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, PDBGFADDRESS pAddress)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    AssertPtr(pVar);
    AssertPtr(pAddress);

    switch (pVar->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            DBGFR3AddrFromFlat(pDbgc->pUVM, pAddress, pVar->u.GCFlat);
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_NUMBER:
            DBGFR3AddrFromFlat(pDbgc->pUVM, pAddress, (RTGCUINTPTR)pVar->u.u64Number);
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_GC_FAR:
            return DBGFR3AddrFromSelOff(pDbgc->pUVM, pDbgc->idCpu, pAddress, pVar->u.GCFar.sel, pVar->u.GCFar.off);

        case DBGCVAR_TYPE_GC_PHYS:
            DBGFR3AddrFromPhys(pDbgc->pUVM, pAddress, pVar->u.GCPhys);
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_SYMBOL:
        {
            DBGCVAR Var;
            int rc = DBGCCmdHlpEval(&pDbgc->CmdHlp, &Var, "%%(%DV)", pVar);
            if (RT_FAILURE(rc))
                return rc;
            return dbgcHlpVarToDbgfAddr(pCmdHlp, &Var, pAddress);
        }

        case DBGCVAR_TYPE_STRING:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_PHYS:
        default:
            return VERR_DBGC_PARSE_CONVERSION_FAILED;
    }
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVarFromDbgfAddr}
 */
static DECLCALLBACK(int) dbgcHlpVarFromDbgfAddr(PDBGCCMDHLP pCmdHlp, PCDBGFADDRESS pAddress, PDBGCVAR pResult)
{
    RT_NOREF1(pCmdHlp);
    AssertPtrReturn(pAddress, VERR_INVALID_POINTER);
    AssertReturn(DBGFADDRESS_IS_VALID(pAddress), VERR_INVALID_PARAMETER);
    AssertPtrReturn(pResult,  VERR_INVALID_POINTER);

    switch (pAddress->fFlags & DBGFADDRESS_FLAGS_TYPE_MASK)
    {
        case DBGFADDRESS_FLAGS_FAR16:
        case DBGFADDRESS_FLAGS_FAR32:
        case DBGFADDRESS_FLAGS_FAR64:
            DBGCVAR_INIT_GC_FAR(pResult, pAddress->Sel, pAddress->off);
            break;

        case DBGFADDRESS_FLAGS_FLAT:
            DBGCVAR_INIT_GC_FLAT(pResult, pAddress->FlatPtr);
            break;

        case DBGFADDRESS_FLAGS_PHYS:
            DBGCVAR_INIT_GC_PHYS(pResult, pAddress->FlatPtr);
            break;

        default:
            DBGCVAR_INIT(pResult);
            AssertMsgFailedReturn(("%#x\n", pAddress->fFlags), VERR_INVALID_PARAMETER);
            break;
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVarToNumber}
 */
static DECLCALLBACK(int) dbgcHlpVarToNumber(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, uint64_t *pu64Number)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    NOREF(pDbgc);

    uint64_t u64Number;
    switch (pVar->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            u64Number = pVar->u.GCFlat;
            break;
        case DBGCVAR_TYPE_GC_PHYS:
            u64Number = pVar->u.GCPhys;
            break;
        case DBGCVAR_TYPE_HC_FLAT:
            u64Number = (uintptr_t)pVar->u.pvHCFlat;
            break;
        case DBGCVAR_TYPE_HC_PHYS:
            u64Number = (uintptr_t)pVar->u.HCPhys;
            break;
        case DBGCVAR_TYPE_NUMBER:
            u64Number = (uintptr_t)pVar->u.u64Number;
            break;
        case DBGCVAR_TYPE_GC_FAR:
            u64Number = (uintptr_t)pVar->u.GCFar.off;
            break;
        case DBGCVAR_TYPE_SYMBOL:
            /** @todo try convert as symbol? */
        case DBGCVAR_TYPE_STRING:
            return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE; /** @todo better error code! */
        default:
            return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;
    }
    *pu64Number = u64Number;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVarToBool}
 */
static DECLCALLBACK(int) dbgcHlpVarToBool(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, bool *pf)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    NOREF(pDbgc);

    switch (pVar->enmType)
    {
        case DBGCVAR_TYPE_SYMBOL:
        case DBGCVAR_TYPE_STRING:
            if (    !RTStrICmp(pVar->u.pszString, "true")
                ||  !RTStrICmp(pVar->u.pszString, "on")
                ||  !RTStrICmp(pVar->u.pszString, "no")
                ||  !RTStrICmp(pVar->u.pszString, "enabled"))
            {
                *pf = true;
                return VINF_SUCCESS;
            }
            if (    !RTStrICmp(pVar->u.pszString, "false")
                ||  !RTStrICmp(pVar->u.pszString, "off")
                ||  !RTStrICmp(pVar->u.pszString, "yes")
                ||  !RTStrICmp(pVar->u.pszString, "disabled"))
            {
                *pf = false;
                return VINF_SUCCESS;
            }
            return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE; /** @todo better error code! */

        case DBGCVAR_TYPE_GC_FLAT:
        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_PHYS:
        case DBGCVAR_TYPE_NUMBER:
            *pf = pVar->u.u64Number != 0;
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_GC_FAR:
        default:
            return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;
    }
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVarGetRange}
 */
static DECLCALLBACK(int) dbgcHlpVarGetRange(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, uint64_t cbElement, uint64_t cbDefault,
                                            uint64_t *pcbRange)
{
    RT_NOREF1(pCmdHlp);
/** @todo implement this properly, strings/symbols are not resolved now. */
    switch (pVar->enmRangeType)
    {
        default:
        case DBGCVAR_RANGE_NONE:
            *pcbRange = cbDefault;
            break;
        case DBGCVAR_RANGE_BYTES:
            *pcbRange = pVar->u64Range;
            break;
        case DBGCVAR_RANGE_ELEMENTS:
            *pcbRange = pVar->u64Range * cbElement;
            break;
    }
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnVarConvert}
 */
static DECLCALLBACK(int) dbgcHlpVarConvert(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, DBGCVARTYPE enmToType, bool fConvSyms,
                                           PDBGCVAR pResult)
{
    PDBGC           pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    DBGCVAR const   InVar = *pVar;      /* if pVar == pResult  */
    PCDBGCVAR       pArg = &InVar;      /* lazy bird, clean up later */
    DBGFADDRESS     Address;
    int             rc;

    Assert(pDbgc->pUVM);

    *pResult = InVar;
    switch (InVar.enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            switch (enmToType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_GC_FAR:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_PHYS:
                    pResult->enmType = DBGCVAR_TYPE_GC_PHYS;
                    rc = DBGFR3AddrToPhys(pDbgc->pUVM, pDbgc->idCpu,
                                          DBGFR3AddrFromFlat(pDbgc->pUVM, &Address, pArg->u.GCFlat),
                                          &pResult->u.GCPhys);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_HC_FLAT:
                    pResult->enmType = DBGCVAR_TYPE_HC_FLAT;
                    rc = DBGFR3AddrToVolatileR3Ptr(pDbgc->pUVM, pDbgc->idCpu,
                                                   DBGFR3AddrFromFlat(pDbgc->pUVM, &Address, pArg->u.GCFlat),
                                                   false /*fReadOnly */,
                                                   &pResult->u.pvHCFlat);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_HC_PHYS:
                    pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
                    rc = DBGFR3AddrToHostPhys(pDbgc->pUVM, pDbgc->idCpu,
                                              DBGFR3AddrFromFlat(pDbgc->pUVM, &Address, pArg->u.GCFlat),
                                              &pResult->u.GCPhys);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_NUMBER:
                    pResult->enmType     = enmToType;
                    pResult->u.u64Number = InVar.u.GCFlat;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_STRING:
                case DBGCVAR_TYPE_SYMBOL:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_UNKNOWN:
                case DBGCVAR_TYPE_ANY:
                    break;
            }
            break;

        case DBGCVAR_TYPE_GC_FAR:
            switch (enmToType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    rc = DBGFR3AddrFromSelOff(pDbgc->pUVM, pDbgc->idCpu, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
                    if (RT_SUCCESS(rc))
                    {
                        pResult->enmType  = DBGCVAR_TYPE_GC_FLAT;
                        pResult->u.GCFlat = Address.FlatPtr;
                        return VINF_SUCCESS;
                    }
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_GC_FAR:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_GC_PHYS:
                    rc = DBGFR3AddrFromSelOff(pDbgc->pUVM, pDbgc->idCpu, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
                    if (RT_SUCCESS(rc))
                    {
                        pResult->enmType = DBGCVAR_TYPE_GC_PHYS;
                        rc = DBGFR3AddrToPhys(pDbgc->pUVM, pDbgc->idCpu, &Address, &pResult->u.GCPhys);
                        if (RT_SUCCESS(rc))
                            return VINF_SUCCESS;
                    }
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_HC_FLAT:
                    rc = DBGFR3AddrFromSelOff(pDbgc->pUVM, pDbgc->idCpu, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
                    if (RT_SUCCESS(rc))
                    {
                        pResult->enmType = DBGCVAR_TYPE_HC_FLAT;
                        rc = DBGFR3AddrToVolatileR3Ptr(pDbgc->pUVM, pDbgc->idCpu, &Address,
                                                       false /*fReadOnly*/, &pResult->u.pvHCFlat);
                        if (RT_SUCCESS(rc))
                            return VINF_SUCCESS;
                    }
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_HC_PHYS:
                    rc = DBGFR3AddrFromSelOff(pDbgc->pUVM, pDbgc->idCpu, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
                    if (RT_SUCCESS(rc))
                    {
                        pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
                        rc = DBGFR3AddrToHostPhys(pDbgc->pUVM, pDbgc->idCpu, &Address, &pResult->u.GCPhys);
                        if (RT_SUCCESS(rc))
                            return VINF_SUCCESS;
                    }
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_NUMBER:
                    pResult->enmType     = enmToType;
                    pResult->u.u64Number = InVar.u.GCFar.off;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_STRING:
                case DBGCVAR_TYPE_SYMBOL:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_UNKNOWN:
                case DBGCVAR_TYPE_ANY:
                    break;
            }
            break;

        case DBGCVAR_TYPE_GC_PHYS:
            switch (enmToType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    //rc = MMR3PhysGCPhys2GCVirtEx(pDbgc->pVM, pResult->u.GCPhys, ..., &pResult->u.GCFlat); - yea, sure.
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_FAR:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_PHYS:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_HC_FLAT:
                    pResult->enmType = DBGCVAR_TYPE_HC_FLAT;
                    rc = DBGFR3AddrToVolatileR3Ptr(pDbgc->pUVM, pDbgc->idCpu,
                                                   DBGFR3AddrFromPhys(pDbgc->pUVM, &Address, pArg->u.GCPhys),
                                                   false /*fReadOnly */,
                                                   &pResult->u.pvHCFlat);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_HC_PHYS:
                    pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
                    rc = DBGFR3AddrToHostPhys(pDbgc->pUVM, pDbgc->idCpu,
                                              DBGFR3AddrFromPhys(pDbgc->pUVM, &Address, pArg->u.GCPhys),
                                              &pResult->u.HCPhys);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_NUMBER:
                    pResult->enmType     = enmToType;
                    pResult->u.u64Number = InVar.u.GCPhys;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_STRING:
                case DBGCVAR_TYPE_SYMBOL:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_UNKNOWN:
                case DBGCVAR_TYPE_ANY:
                    break;
            }
            break;

        case DBGCVAR_TYPE_HC_FLAT:
            switch (enmToType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_FAR:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_PHYS:
                    pResult->enmType = DBGCVAR_TYPE_GC_PHYS;
                    rc = PGMR3DbgR3Ptr2GCPhys(pDbgc->pUVM, pArg->u.pvHCFlat, &pResult->u.GCPhys);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    /** @todo more memory types! */
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_HC_FLAT:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_HC_PHYS:
                    pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
                    rc = PGMR3DbgR3Ptr2HCPhys(pDbgc->pUVM, pArg->u.pvHCFlat, &pResult->u.HCPhys);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    /** @todo more memory types! */
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_NUMBER:
                    pResult->enmType     = enmToType;
                    pResult->u.u64Number = (uintptr_t)InVar.u.pvHCFlat;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_STRING:
                case DBGCVAR_TYPE_SYMBOL:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_UNKNOWN:
                case DBGCVAR_TYPE_ANY:
                    break;
            }
            break;

        case DBGCVAR_TYPE_HC_PHYS:
            switch (enmToType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_FAR:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_PHYS:
                    pResult->enmType = DBGCVAR_TYPE_GC_PHYS;
                    rc = PGMR3DbgHCPhys2GCPhys(pDbgc->pUVM, pArg->u.HCPhys, &pResult->u.GCPhys);
                    if (RT_SUCCESS(rc))
                        return VINF_SUCCESS;
                    return VERR_DBGC_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_HC_FLAT:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_HC_PHYS:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_NUMBER:
                    pResult->enmType     = enmToType;
                    pResult->u.u64Number = InVar.u.HCPhys;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_STRING:
                case DBGCVAR_TYPE_SYMBOL:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_UNKNOWN:
                case DBGCVAR_TYPE_ANY:
                    break;
            }
            break;

        case DBGCVAR_TYPE_NUMBER:
            switch (enmToType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    pResult->enmType  = DBGCVAR_TYPE_GC_FLAT;
                    pResult->u.GCFlat = (RTGCPTR)InVar.u.u64Number;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_GC_FAR:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_GC_PHYS:
                    pResult->enmType  = DBGCVAR_TYPE_GC_PHYS;
                    pResult->u.GCPhys = (RTGCPHYS)InVar.u.u64Number;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_HC_FLAT:
                    pResult->enmType    = DBGCVAR_TYPE_HC_FLAT;
                    pResult->u.pvHCFlat = (void *)(uintptr_t)InVar.u.u64Number;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_HC_PHYS:
                    pResult->enmType  = DBGCVAR_TYPE_HC_PHYS;
                    pResult->u.HCPhys = (RTHCPHYS)InVar.u.u64Number;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_NUMBER:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_STRING:
                case DBGCVAR_TYPE_SYMBOL:
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_UNKNOWN:
                case DBGCVAR_TYPE_ANY:
                    break;
            }
            break;

        case DBGCVAR_TYPE_SYMBOL:
        case DBGCVAR_TYPE_STRING:
            switch (enmToType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                case DBGCVAR_TYPE_GC_FAR:
                case DBGCVAR_TYPE_GC_PHYS:
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_PHYS:
                case DBGCVAR_TYPE_NUMBER:
                    if (fConvSyms)
                    {
                        rc = dbgcSymbolGet(pDbgc, InVar.u.pszString, enmToType, pResult);
                        if (RT_SUCCESS(rc))
                            return VINF_SUCCESS;
                    }
                    return VERR_DBGC_PARSE_INCORRECT_ARG_TYPE;

                case DBGCVAR_TYPE_STRING:
                case DBGCVAR_TYPE_SYMBOL:
                    pResult->enmType = enmToType;
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_UNKNOWN:
                case DBGCVAR_TYPE_ANY:
                    break;
            }
            break;

        case DBGCVAR_TYPE_UNKNOWN:
        case DBGCVAR_TYPE_ANY:
            break;
    }

    AssertMsgFailed(("f=%d t=%d\n", InVar.enmType, enmToType));
    return VERR_INVALID_PARAMETER;
}


/**
 * @interface_method_impl{DBGFINFOHLP,pfnPrintf}
 */
static DECLCALLBACK(void) dbgcHlpGetDbgfOutputHlp_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...)
{
    PDBGC pDbgc = RT_FROM_MEMBER(pHlp, DBGC, DbgfOutputHlp);
    va_list va;
    va_start(va, pszFormat);
    pDbgc->CmdHlp.pfnPrintfV(&pDbgc->CmdHlp, NULL, pszFormat, va);
    va_end(va);
}


/**
 * @interface_method_impl{DBGFINFOHLP,pfnPrintfV}
 */
static DECLCALLBACK(void) dbgcHlpGetDbgfOutputHlp_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args)
{
    PDBGC pDbgc = RT_FROM_MEMBER(pHlp, DBGC, DbgfOutputHlp);
    pDbgc->CmdHlp.pfnPrintfV(&pDbgc->CmdHlp, NULL, pszFormat, args);
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnGetDbgfOutputHlp}
 */
static DECLCALLBACK(PCDBGFINFOHLP) dbgcHlpGetDbgfOutputHlp(PDBGCCMDHLP pCmdHlp)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /* Lazy init */
    if (!pDbgc->DbgfOutputHlp.pfnPrintf)
    {
        pDbgc->DbgfOutputHlp.pfnPrintf  = dbgcHlpGetDbgfOutputHlp_Printf;
        pDbgc->DbgfOutputHlp.pfnPrintfV = dbgcHlpGetDbgfOutputHlp_PrintfV;
    }

    return &pDbgc->DbgfOutputHlp;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnGetCurrentCpu}
 */
static DECLCALLBACK(VMCPUID) dbgcHlpGetCurrentCpu(PDBGCCMDHLP pCmdHlp)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    return pDbgc->idCpu;
}


/**
 * @interface_method_impl{DBGCCMDHLP,pfnGetCpuMode}
 */
static DECLCALLBACK(CPUMMODE) dbgcHlpGetCpuMode(PDBGCCMDHLP pCmdHlp)
{
    PDBGC    pDbgc   = DBGC_CMDHLP2DBGC(pCmdHlp);
    CPUMMODE enmMode = CPUMMODE_INVALID;
    if (pDbgc->fRegCtxGuest)
    {
        if (pDbgc->pUVM)
            enmMode = DBGFR3CpuGetMode(pDbgc->pUVM, DBGCCmdHlpGetCurrentCpu(pCmdHlp));
        if (enmMode == CPUMMODE_INVALID)
#if HC_ARCH_BITS == 64
            enmMode = CPUMMODE_LONG;
#else
            enmMode = CPUMMODE_PROTECTED;
#endif
    }
    else
        enmMode = CPUMMODE_PROTECTED;
    return enmMode;
}


/**
 * Initializes the Command Helpers for a DBGC instance.
 *
 * @param   pDbgc   Pointer to the DBGC instance.
 */
void dbgcInitCmdHlp(PDBGC pDbgc)
{
    pDbgc->CmdHlp.u32Magic              = DBGCCMDHLP_MAGIC;
    pDbgc->CmdHlp.pfnPrintfV            = dbgcHlpPrintfV;
    pDbgc->CmdHlp.pfnPrintf             = dbgcHlpPrintf;
    pDbgc->CmdHlp.pfnStrPrintf          = dbgcHlpStrPrintf;
    pDbgc->CmdHlp.pfnStrPrintfV         = dbgcHlpStrPrintfV;
    pDbgc->CmdHlp.pfnVBoxErrorV         = dbgcHlpVBoxErrorV;
    pDbgc->CmdHlp.pfnVBoxError          = dbgcHlpVBoxError;
    pDbgc->CmdHlp.pfnMemRead            = dbgcHlpMemRead;
    pDbgc->CmdHlp.pfnMemWrite           = dbgcHlpMemWrite;
    pDbgc->CmdHlp.pfnEvalV              = dbgcHlpEvalV;
    pDbgc->CmdHlp.pfnExec               = dbgcHlpExec;
    pDbgc->CmdHlp.pfnFailV              = dbgcHlpFailV;
    pDbgc->CmdHlp.pfnFailRcV            = dbgcHlpFailRcV;
    pDbgc->CmdHlp.pfnParserError        = dbgcHlpParserError;
    pDbgc->CmdHlp.pfnVarToDbgfAddr      = dbgcHlpVarToDbgfAddr;
    pDbgc->CmdHlp.pfnVarFromDbgfAddr    = dbgcHlpVarFromDbgfAddr;
    pDbgc->CmdHlp.pfnVarToNumber        = dbgcHlpVarToNumber;
    pDbgc->CmdHlp.pfnVarToBool          = dbgcHlpVarToBool;
    pDbgc->CmdHlp.pfnVarGetRange        = dbgcHlpVarGetRange;
    pDbgc->CmdHlp.pfnVarConvert         = dbgcHlpVarConvert;
    pDbgc->CmdHlp.pfnGetDbgfOutputHlp   = dbgcHlpGetDbgfOutputHlp;
    pDbgc->CmdHlp.pfnGetCurrentCpu      = dbgcHlpGetCurrentCpu;
    pDbgc->CmdHlp.pfnGetCpuMode         = dbgcHlpGetCpuMode;
    pDbgc->CmdHlp.u32EndMarker          = DBGCCMDHLP_MAGIC;
}


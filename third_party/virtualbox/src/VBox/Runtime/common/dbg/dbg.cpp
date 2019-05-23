/* $Id: dbg.cpp $ */
/** @file
 * IPRT - Debug Misc.
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
#include <iprt/dbg.h>
#include "internal/iprt.h"

#include <iprt/mem.h>



/**
 * Allocate a new symbol structure.
 *
 * @returns Pointer to a new structure on success, NULL on failure.
 */
RTDECL(PRTDBGSYMBOL) RTDbgSymbolAlloc(void)
{
    return (PRTDBGSYMBOL)RTMemAllocZ(sizeof(RTDBGSYMBOL));
}
RT_EXPORT_SYMBOL(RTDbgSymbolAlloc);


/**
 * Duplicates a symbol structure.
 *
 * @returns Pointer to duplicate on success, NULL on failure.
 *
 * @param   pSymInfo        The symbol info to duplicate.
 */
RTDECL(PRTDBGSYMBOL) RTDbgSymbolDup(PCRTDBGSYMBOL pSymInfo)
{
    return (PRTDBGSYMBOL)RTMemDup(pSymInfo, sizeof(*pSymInfo));
}
RT_EXPORT_SYMBOL(RTDbgSymbolDup);


/**
 * Free a symbol structure previously allocated by a RTDbg method.
 *
 * @param   pSymInfo        The symbol info to free. NULL is ignored.
 */
RTDECL(void) RTDbgSymbolFree(PRTDBGSYMBOL pSymInfo)
{
    RTMemFree(pSymInfo);
}
RT_EXPORT_SYMBOL(RTDbgSymbolFree);


/**
 * Allocate a new line number structure.
 *
 * @returns Pointer to a new structure on success, NULL on failure.
 */
RTDECL(PRTDBGLINE) RTDbgLineAlloc(void)
{
    return (PRTDBGLINE)RTMemAllocZ(sizeof(RTDBGLINE));
}
RT_EXPORT_SYMBOL(RTDbgLineAlloc);


/**
 * Duplicates a line number structure.
 *
 * @returns Pointer to duplicate on success, NULL on failure.
 *
 * @param   pLine           The line number to duplicate.
 */
RTDECL(PRTDBGLINE) RTDbgLineDup(PCRTDBGLINE pLine)
{
    return (PRTDBGLINE)RTMemDup(pLine, sizeof(*pLine));
}
RT_EXPORT_SYMBOL(RTDbgLineDup);


/**
 * Free a line number structure previously allocated by a RTDbg method.
 *
 * @param   pLine           The line number to free. NULL is ignored.
 */
RTDECL(void) RTDbgLineFree(PRTDBGLINE pLine)
{
    RTMemFree(pLine);
}
RT_EXPORT_SYMBOL(RTDbgLineFree);


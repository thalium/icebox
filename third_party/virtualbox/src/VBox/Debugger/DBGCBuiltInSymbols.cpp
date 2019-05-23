/* $Id: DBGCBuiltInSymbols.cpp $ */
/** @file
 * DBGC - Debugger Console, Built-In Symbols.
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
#include "DBGCInternal.h"



/**
 * Finds a builtin symbol.
 *
 * @returns Pointer to symbol descriptor on success.
 * @returns NULL on failure.
 * @param   pDbgc       The debug console instance.
 * @param   pszSymbol   The symbol name.
 */
PCDBGCSYM dbgcLookupRegisterSymbol(PDBGC pDbgc, const char *pszSymbol)
{
    /** @todo externally registered symbols. */
    RT_NOREF2(pDbgc, pszSymbol);
    return NULL;
}


/* $Id: VBoxPrintString.c $ */
/** @file
 * VBoxPrintString.c - Implementation of the VBoxPrintString() debug logging routine.
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
#include "VBoxDebugLib.h"
#include "DevEFI.h"
#include "iprt/asm.h"


/**
 * Prints a string to the EFI debug port.
 *
 * @returns The string length.
 * @param   pszString       The string to print.
 */
size_t VBoxPrintString(const char *pszString)
{
    const char *pszEnd = pszString;
    while (*pszEnd)
        pszEnd++;
    ASMOutStrU8(EFI_DEBUG_PORT, (uint8_t const *)pszString, pszEnd - pszString);
    return pszEnd - pszString;
}


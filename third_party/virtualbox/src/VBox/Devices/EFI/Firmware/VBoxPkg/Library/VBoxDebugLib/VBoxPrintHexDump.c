/* $Id: VBoxPrintHexDump.c $ */
/** @file
 * VBoxPrintHex.c - Implementation of the VBoxPrintHex() debug logging routine.
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
#include <ctype.h>
#include "VBoxDebugLib.h"
#include "DevEFI.h"
#include "iprt/asm.h"


/**
 * Prints a char.
 * @returns 1.
 * @param   ch              The char to print.
 */
DECLINLINE(int) vboxPrintHexDumpChar(int ch)
{
    ASMOutU8(EFI_DEBUG_PORT, (uint8_t)ch);
    return 1;
}


/**
 * Prints a hex dump the specified memory block.
 *
 * @returns Number of bytes printed.
 *
 * @param   pv      The memory to dump.
 * @param   cb      Number of bytes to dump.
 */
size_t VBoxPrintHexDump(const void *pv, size_t cb)
{
    size_t          cchPrinted = 0;
    uint8_t const  *pb         = (uint8_t const *)pv;
    while (cb > 0)
    {
        unsigned i;

        /* the offset */
        cchPrinted += VBoxPrintHex((uintptr_t)pb, sizeof(pb));
        cchPrinted += VBoxPrintString("  ");

        /* the hex bytes value. */
        for (i = 0; i < 16; i++)
        {
            cchPrinted += vboxPrintHexDumpChar(i == 7 ? '-' : ' ');
            if (i < cb)
                cchPrinted += VBoxPrintHex(pb[i], 1);
            else
                cchPrinted += VBoxPrintString("  ");
        }

        /* the printable chars */
        cchPrinted += VBoxPrintString("  ");
        for (i = 0; i < 16 && i < cb; i++)
            cchPrinted += vboxPrintHexDumpChar(isprint(pb[i])
                                               ? pb[i]
                                               : '.');

        /* finally, the new line. */
        cchPrinted += vboxPrintHexDumpChar('\n');

        /*
         * Advance.
         */
        if (cb <= 16)
            break;
        cb -= 16;
        pb += 16;
    }

    return cchPrinted;
}


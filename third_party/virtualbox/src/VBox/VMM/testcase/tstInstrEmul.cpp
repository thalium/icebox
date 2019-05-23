/* $Id: tstInstrEmul.cpp $ */
/** @file
 * Micro Testcase, checking emulation of certain instructions
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
#include <stdio.h>
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/vmm/em.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>


int main(int argc, char **argv)
{
    int     rcRet = 0;                  /* error count. */

    RTR3InitExe(argc, &argv, 0);
    RTPrintf("tstInstrEmul: TESTING...\n");

    uint32_t eax, edx, ebx, ecx, eflags;
    uint64_t val;

    val = UINT64_C(0xffffffffffff);
    eax = 0xffffffff;
    edx = 0xffff;
    ebx = 0x1;
    ecx = 0x2;
    eflags = EMEmulateLockCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (    !(eflags & X86_EFL_ZF)
        ||  val != UINT64_C(0x200000001))
    {
        RTPrintf("tstInstrEmul: FAILURE - Lock cmpxchg8b failed the equal case! (val=%RX64)\n", val);
        return 1;
    }
    val = UINT64_C(0x123456789);
    eflags = EMEmulateLockCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (    (eflags & X86_EFL_ZF)
        ||  eax != 0x23456789
        ||  edx != 0x1)
    {
        RTPrintf("tstInstrEmul: FAILURE - Lock cmpxchg8b failed the non-equal case! (val=%RX64)\n", val);
        return 1;
    }
    RTPrintf("tstInstrEmul: Testing lock cmpxchg instruction emulation - SUCCESS\n");

    val = UINT64_C(0xffffffffffff);
    eax = 0xffffffff;
    edx = 0xffff;
    ebx = 0x1;
    ecx = 0x2;
    eflags = EMEmulateCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (    !(eflags & X86_EFL_ZF)
        ||  val != UINT64_C(0x200000001))
    {
        RTPrintf("tstInstrEmul: FAILURE - Cmpxchg8b failed the equal case! (val=%RX64)\n", val);
        return 1;
    }
    val = UINT64_C(0x123456789);
    eflags = EMEmulateCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (    (eflags & X86_EFL_ZF)
        ||  eax != 0x23456789
        ||  edx != 0x1)
    {
        RTPrintf("tstInstrEmul: FAILURE - Cmpxchg8b failed the non-equal case! (val=%RX64)\n", val);
        return 1;
    }
    RTPrintf("tstInstrEmul: Testing cmpxchg instruction emulation - SUCCESS\n");


    /*
     * Summary.
     */
    if (!rcRet)
        RTPrintf("tstInstrEmul: SUCCESS\n");
    else
        RTPrintf("tstInstrEmul: FAILURE - %d errors\n", rcRet);
    return rcRet ? 1 : 0;
}

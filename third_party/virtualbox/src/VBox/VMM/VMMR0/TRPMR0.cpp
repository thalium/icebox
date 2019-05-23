/* $Id: TRPMR0.cpp $ */
/** @file
 * TRPM - The Trap Monitor - HC Ring 0
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
#define LOG_GROUP LOG_GROUP_TRPM
#include <VBox/vmm/trpm.h>
#include "TRPMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm-amd64-x86.h>


#if defined(RT_OS_DARWIN) && ARCH_BITS == 32
# error "32-bit darwin is no longer supported. Go back to 4.3 or earlier!"
#endif


/**
 * Dispatches an interrupt that arrived while we were in the guest context.
 *
 * @param   pVM     The cross context VM structure.
 * @remark  Must be called with interrupts disabled.
 */
VMMR0DECL(void) TRPMR0DispatchHostInterrupt(PVM pVM)
{
    /*
     * Get the active interrupt vector number.
     */
    PVMCPU pVCpu = VMMGetCpu0(pVM);
    RTUINT uActiveVector = pVCpu->trpm.s.uActiveVector;
    pVCpu->trpm.s.uActiveVector = UINT32_MAX;
    AssertMsgReturnVoid(uActiveVector < 256, ("uActiveVector=%#x is invalid! (More assertions to come, please enjoy!)\n", uActiveVector));

#if HC_ARCH_BITS == 64 && defined(RT_OS_DARWIN)
    /*
     * Do it the simple and safe way.
     *
     * This is a workaround for an optimization bug in the code below
     * or a gcc 4.2 on mac (snow leopard seed 314).
     */
    trpmR0DispatchHostInterruptSimple(uActiveVector);

#else  /* The complicated way: */

    /*
     * Get the handler pointer (16:32 ptr) / (16:48 ptr).
     */
    RTIDTR      Idtr;
    ASMGetIDTR(&Idtr);
# if HC_ARCH_BITS == 32
    PVBOXIDTE   pIdte = &((PVBOXIDTE)Idtr.pIdt)[uActiveVector];
# else
    PVBOXIDTE64 pIdte = &((PVBOXIDTE64)Idtr.pIdt)[uActiveVector];
# endif
    AssertMsgReturnVoid(pIdte->Gen.u1Present, ("The IDT entry (%d) is not present!\n", uActiveVector));
    AssertMsgReturnVoid(    pIdte->Gen.u3Type1 == VBOX_IDTE_TYPE1
                        ||  pIdte->Gen.u5Type2 == VBOX_IDTE_TYPE2_INT_32,
                        ("The IDT entry (%d) is not 32-bit int gate! type1=%#x type2=%#x\n",
                         uActiveVector, pIdte->Gen.u3Type1, pIdte->Gen.u5Type2));
# if HC_ARCH_BITS == 32
    RTFAR32   pfnHandler;
    pfnHandler.off = VBOXIDTE_OFFSET(*pIdte);
    pfnHandler.sel = pIdte->Gen.u16SegSel;

    const RTR0UINTREG uRSP = ~(RTR0UINTREG)0;

# else /* 64-bit: */
    RTFAR64   pfnHandler;
    pfnHandler.off = VBOXIDTE64_OFFSET(*pIdte);
    pfnHandler.sel = pIdte->Gen.u16SegSel;

    const RTR0UINTREG uRSP = ~(RTR0UINTREG)0;
    if (pIdte->Gen.u3Ist)
    {
        trpmR0DispatchHostInterruptSimple(uActiveVector);
        return;
    }

# endif

    /*
     * Dispatch it.
     */
    trpmR0DispatchHostInterrupt(pfnHandler.off, pfnHandler.sel, uRSP);
#endif
}


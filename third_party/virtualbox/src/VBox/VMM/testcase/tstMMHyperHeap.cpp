/* $Id: tstMMHyperHeap.cpp $ */
/** @file
 * MM Hypervisor Heap testcase.
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
#include <VBox/vmm/mm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/sup.h>
#include <VBox/param.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/assert.h>
#include <iprt/stream.h>
#include <iprt/string.h>

/* does not work for more CPUs as SUPR3LowAlloc() would refuse to allocate more pages */
#define NUM_CPUS  16


/**
 *  Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char **envp)
{
    RT_NOREF1(envp);

    /*
     * Init runtime.
     */
    RTR3InitExe(argc, &argv, 0);

    /*
     * Create empty VM structure and call MMR3Init().
     */
    PVM         pVM = NULL;
    RTR0PTR     pvR0 = NIL_RTR0PTR;
    SUPPAGE     aPages[RT_ALIGN_Z(sizeof(*pVM) + NUM_CPUS * sizeof(VMCPU), PAGE_SIZE) >> PAGE_SHIFT];
    int rc = SUPR3Init(NULL);
    if (RT_SUCCESS(rc))
        rc = SUPR3LowAlloc(RT_ELEMENTS(aPages), (void **)&pVM, &pvR0, &aPages[0]);
    if (RT_FAILURE(rc))
    {
        RTPrintf("Fatal error: SUP Failure! rc=%Rrc\n", rc);
        return 1;
    }
    memset(pVM, 0, sizeof(*pVM)); /* wtf? */
    pVM->paVMPagesR3 = aPages;
    pVM->pVMR0 = pvR0;

    static UVM s_UVM;
    PUVM pUVM = &s_UVM;
    pUVM->pVM = pVM;
    pVM->pUVM = pUVM;

    pVM->cCpus = NUM_CPUS;
    pVM->cbSelf = RT_UOFFSETOF(VM, aCpus[pVM->cCpus]);

    rc = STAMR3InitUVM(pUVM);
    if (RT_FAILURE(rc))
    {
        RTPrintf("FAILURE: STAMR3Init failed. rc=%Rrc\n", rc);
        return 1;
    }

    rc = MMR3InitUVM(pUVM);
    if (RT_FAILURE(rc))
    {
        RTPrintf("FAILURE: STAMR3Init failed. rc=%Rrc\n", rc);
        return 1;
    }

    rc = CFGMR3Init(pVM, NULL, NULL);
    if (RT_FAILURE(rc))
    {
        RTPrintf("FAILURE: CFGMR3Init failed. rc=%Rrc\n", rc);
        return 1;
    }

    rc = MMR3Init(pVM);
    if (RT_FAILURE(rc))
    {
        RTPrintf("Fatal error: MMR3Init failed! rc=%Rrc\n", rc);
        return 1;
    }

    /*
     * Try allocate.
     */
    static struct
    {
        size_t      cb;
        unsigned    uAlignment;
        void       *pvAlloc;
        unsigned    iFreeOrder;
    } aOps[] =
    {
        {        16,          0,    NULL,  0 },
        {        16,          4,    NULL,  1 },
        {        16,          8,    NULL,  2 },
        {        16,         16,    NULL,  5 },
        {        16,         32,    NULL,  4 },
        {        32,          0,    NULL,  3 },
        {        31,          0,    NULL,  6 },
        {      1024,          0,    NULL,  8 },
        {      1024,         32,    NULL, 10 },
        {      1024,         32,    NULL, 12 },
        { PAGE_SIZE,  PAGE_SIZE,    NULL, 13 },
        {      1024,         32,    NULL,  9 },
        { PAGE_SIZE,         32,    NULL, 11 },
        { PAGE_SIZE,  PAGE_SIZE,    NULL, 14 },
        {        16,          0,    NULL, 15 },
        {        9,           0,    NULL,  7 },
        {        16,          0,    NULL,  7 },
        {        36,          0,    NULL,  7 },
        {        16,          0,    NULL,  7 },
        {     12344,          0,    NULL,  7 },
        {        50,          0,    NULL,  7 },
        {        16,          0,    NULL,  7 },
    };
    unsigned i;
#ifdef DEBUG
    MMHyperHeapDump(pVM);
#endif
    size_t cbBefore = MMHyperHeapGetFreeSize(pVM);
    static char szFill[] = "01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    /* allocate */
    for (i = 0; i < RT_ELEMENTS(aOps); i++)
    {
        rc = MMHyperAlloc(pVM, aOps[i].cb, aOps[i].uAlignment, MM_TAG_VM, &aOps[i].pvAlloc);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failure: MMHyperAlloc(, %#x, %#x,) -> %d i=%d\n", aOps[i].cb, aOps[i].uAlignment, rc, i);
            return 1;
        }
        memset(aOps[i].pvAlloc, szFill[i], aOps[i].cb);
        if (RT_ALIGN_P(aOps[i].pvAlloc, (aOps[i].uAlignment ? aOps[i].uAlignment : 8)) != aOps[i].pvAlloc)
        {
            RTPrintf("Failure: MMHyperAlloc(, %#x, %#x,) -> %p, invalid alignment!\n", aOps[i].cb, aOps[i].uAlignment, aOps[i].pvAlloc);
            return 1;
        }
    }

    /* free and allocate the same node again. */
    for (i = 0; i < RT_ELEMENTS(aOps); i++)
    {
        if (    !aOps[i].pvAlloc
            ||  aOps[i].uAlignment == PAGE_SIZE)
            continue;
        //size_t cbBeforeSub = MMHyperHeapGetFreeSize(pVM);
        rc = MMHyperFree(pVM, aOps[i].pvAlloc);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failure: MMHyperFree(, %p,) -> %d i=%d\n", aOps[i].pvAlloc, rc, i);
            return 1;
        }
        //RTPrintf("debug: i=%d cbBeforeSub=%d now=%d\n", i, cbBeforeSub, MMHyperHeapGetFreeSize(pVM));
        void *pv;
        rc = MMHyperAlloc(pVM, aOps[i].cb, aOps[i].uAlignment, MM_TAG_VM_REQ, &pv);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failure: MMHyperAlloc(, %#x, %#x,) -> %d i=%d\n", aOps[i].cb, aOps[i].uAlignment, rc, i);
            return 1;
        }
        if (pv != aOps[i].pvAlloc)
        {
            RTPrintf("Failure: Free+Alloc returned different address. new=%p old=%p i=%d (doesn't work with delayed free)\n", pv, aOps[i].pvAlloc, i);
            //return 1;
        }
        aOps[i].pvAlloc = pv;
        #if 0 /* won't work :/ */
        size_t cbAfterSub = MMHyperHeapGetFreeSize(pVM);
        if (cbBeforeSub != cbAfterSub)
        {
            RTPrintf("Failure: cbBeforeSub=%d cbAfterSub=%d. i=%d\n", cbBeforeSub, cbAfterSub, i);
            return 1;
        }
        #endif
    }

    /* free it in a specific order. */
    int cFreed = 0;
    for (i = 0; i < RT_ELEMENTS(aOps); i++)
    {
        unsigned j;
        for (j = 0; j < RT_ELEMENTS(aOps); j++)
        {
            if (    aOps[j].iFreeOrder != i
                ||  !aOps[j].pvAlloc)
                continue;
            RTPrintf("j=%d i=%d free=%d cb=%d pv=%p\n", j, i, MMHyperHeapGetFreeSize(pVM), aOps[j].cb, aOps[j].pvAlloc);
            if (aOps[j].uAlignment == PAGE_SIZE)
                cbBefore -= aOps[j].cb;
            else
            {
                rc = MMHyperFree(pVM, aOps[j].pvAlloc);
                if (RT_FAILURE(rc))
                {
                    RTPrintf("Failure: MMHyperFree(, %p,) -> %d j=%d i=%d\n", aOps[j].pvAlloc, rc, i, j);
                    return 1;
                }
            }
            aOps[j].pvAlloc = NULL;
            cFreed++;
        }
    }
    Assert(cFreed == RT_ELEMENTS(aOps));
    RTPrintf("i=done free=%d\n", MMHyperHeapGetFreeSize(pVM));

    /* check that we're back at the right amount of free memory. */
    size_t cbAfter = MMHyperHeapGetFreeSize(pVM);
    if (cbBefore != cbAfter)
    {
        RTPrintf("Warning: Either we've split out an alignment chunk at the start, or we've got\n"
                 "         an alloc/free accounting bug: cbBefore=%d cbAfter=%d\n", cbBefore, cbAfter);
#ifdef DEBUG
        MMHyperHeapDump(pVM);
#endif
    }

    RTPrintf("tstMMHyperHeap: Success\n");
#ifdef LOG_ENABLED
    RTLogFlush(NULL);
#endif
    return 0;
}


#if !defined(VBOX_WITH_HARDENING) || !defined(RT_OS_WINDOWS)
/**
 * Main entry point.
 */
int main(int argc, char **argv, char **envp)
{
    return TrustedMain(argc, argv, envp);
}
#endif


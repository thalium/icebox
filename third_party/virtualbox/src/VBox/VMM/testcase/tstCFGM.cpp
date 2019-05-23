/* $Id: tstCFGM.cpp $ */
/** @file
 * Testcase for CFGM.
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
#include <VBox/sup.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>

#include <VBox/err.h>
#include <VBox/param.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/mem.h>
#include <iprt/string.h>

#include <iprt/test.h>


static void doGeneralTests(PCFGMNODE pRoot)
{
    /* test multilevel node creation */
    PCFGMNODE pChild = NULL;
    RTTESTI_CHECK_RC_RETV(CFGMR3InsertNode(pRoot, "First/Second/Third//Final", &pChild), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(RT_VALID_PTR(pChild));
    RTTESTI_CHECK(CFGMR3GetChild(pRoot, "First/Second/Third/Final") == pChild);

    /*
     * Boolean queries.
     */
    RTTESTI_CHECK_RC(CFGMR3InsertInteger(pChild, "BoolValue", 1), VINF_SUCCESS);
    bool f = false;
    RTTESTI_CHECK_RC(CFGMR3QueryBool(pChild, "BoolValue", &f), VINF_SUCCESS);
    RTTESTI_CHECK(f == true);

    RTTESTI_CHECK_RC(CFGMR3QueryBool(pRoot, "BoolValue", &f), VERR_CFGM_VALUE_NOT_FOUND);
    RTTESTI_CHECK_RC(CFGMR3QueryBool(NULL, "BoolValue", &f), VERR_CFGM_NO_PARENT);

    RTTESTI_CHECK_RC(CFGMR3QueryBoolDef(pChild, "ValueNotFound", &f, true), VINF_SUCCESS);
    RTTESTI_CHECK(f == true);
    RTTESTI_CHECK_RC(CFGMR3QueryBoolDef(pChild, "ValueNotFound", &f, false), VINF_SUCCESS);
    RTTESTI_CHECK(f == false);

    RTTESTI_CHECK_RC(CFGMR3QueryBoolDef(NULL, "BoolValue", &f, true), VINF_SUCCESS);
    RTTESTI_CHECK(f == true);
    RTTESTI_CHECK_RC(CFGMR3QueryBoolDef(NULL, "BoolValue", &f, false), VINF_SUCCESS);
    RTTESTI_CHECK(f == false);

}



static void doTestsOnDefaultValues(PCFGMNODE pRoot)
{
    /* integer */
    uint64_t u64;
    RTTESTI_CHECK_RC(CFGMR3QueryU64(pRoot, "RamSize", &u64), VINF_SUCCESS);

    size_t cb = 0;
    RTTESTI_CHECK_RC(CFGMR3QuerySize(pRoot, "RamSize", &cb), VINF_SUCCESS);
    RTTESTI_CHECK(cb == sizeof(uint64_t));

    /* string */
    char *pszName = NULL;
    RTTESTI_CHECK_RC(CFGMR3QueryStringAlloc(pRoot, "Name", &pszName), VINF_SUCCESS);
    RTTESTI_CHECK_RC(CFGMR3QuerySize(pRoot, "Name", &cb), VINF_SUCCESS);
    RTTESTI_CHECK(cb == strlen(pszName) + 1);
    MMR3HeapFree(pszName);
}


static void doInVmmTests(RTTEST hTest)
{
    /*
     * Create empty VM structure and init SSM.
     */
    int rc = SUPR3Init(NULL);
    if (RT_FAILURE(rc))
    {
        RTTestSkipped(hTest, "SUPR3Init failed with rc=%Rrc",  rc);
        return;
    }

    PVM pVM;
    RTTESTI_CHECK_RC_RETV(SUPR3PageAlloc(RT_ALIGN_Z(sizeof(*pVM), PAGE_SIZE) >> PAGE_SHIFT, (void **)&pVM), VINF_SUCCESS);


    PUVM pUVM = (PUVM)RTMemPageAlloc(sizeof(*pUVM));
    pUVM->u32Magic = UVM_MAGIC;
    pUVM->pVM = pVM;
    pVM->pUVM = pUVM;

    /*
     * Do the testing.
     */
    RTTESTI_CHECK_RC_RETV(STAMR3InitUVM(pUVM), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(MMR3InitUVM(pUVM), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(CFGMR3Init(pVM, NULL, NULL), VINF_SUCCESS);
    RTTESTI_CHECK_RETV(CFGMR3GetRoot(pVM) != NULL);

    doTestsOnDefaultValues(CFGMR3GetRoot(pVM));
    doGeneralTests(CFGMR3GetRoot(pVM));


    /* done */
    RTTESTI_CHECK_RC_RETV(CFGMR3Term(pVM), VINF_SUCCESS);
}


static void doStandaloneTests(void)
{
    RTTestISub("Standalone");
    PCFGMNODE pRoot;;
    RTTESTI_CHECK_RETV((pRoot = CFGMR3CreateTree(NULL)) != NULL);
    doGeneralTests(pRoot);
    CFGMR3DestroyTree(pRoot);
}


/**
 *  Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char **envp)
{
    RT_NOREF3(argc, argv, envp);

    /*
     * Init runtime.
     */
    RTTEST hTest;
    RTR3InitExeNoArguments(RTR3INIT_FLAGS_SUPLIB);
    RTEXITCODE rcExit = RTTestInitAndCreate("tstCFGM", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;

    doInVmmTests(hTest);
    doStandaloneTests();

    return RTTestSummaryAndDestroy(hTest);
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


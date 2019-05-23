/* $Id: tstVMStructDTrace.cpp $ */
/** @file
 * tstVMMStructDTrace - Generates the DTrace test scripts for check that C/C++
 *                      and DTrace has the same understand of the VM, VMCPU and
 *                      other structures.
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
#define IN_TSTVMSTRUCT 1
#define IN_TSTVMSTRUCTGC 1
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/stam.h>
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include "CFGMInternal.h"
#include "CPUMInternal.h"
#include "MMInternal.h"
#include "PGMInternal.h"
#include "SELMInternal.h"
#include "TRPMInternal.h"
#include "TMInternal.h"
#include "IOMInternal.h"
#include "REMInternal.h"
#include "HMInternal.h"
#include "APICInternal.h"
#include "VMMInternal.h"
#include "DBGFInternal.h"
#include "GIMInternal.h"
#include "STAMInternal.h"
#include "EMInternal.h"
#include "IEMInternal.h"
#include "REMInternal.h"
#ifdef VBOX_WITH_RAW_MODE
# include "CSAMInternal.h"
# include "PATMInternal.h"
#endif
#include <VBox/vmm/vm.h>
#include <VBox/param.h>
#include <iprt/x86.h>
#include <iprt/assert.h>

/* we don't use iprt here because we wish to run without trouble. */
#include <stdio.h>


int main()
{
    /*
     * File header and pragmas.
     */
    printf("#pragma D option quiet\n");
//    printf("#pragma D depends_on library x86.d\n");
//    printf("#pragma D depends_on library cpumctx.d\n");
//    printf("#pragma D depends_on library CPUMInternal.d\n");
//    printf("#pragma D depends_on library vm.d\n");

    printf("int g_cErrors;\n"
           "\n"
           "dtrace:::BEGIN\n"
           "{\n"
           "    g_cErrors = 0;\n"
           "}\n"
           "\n"
           );

    /*
     * Test generator macros.
     */
#define GEN_CHECK_SIZE(s) \
    printf("dtrace:::BEGIN\n" \
           "/sizeof(" #s ") != %u/\n" \
           "{\n" \
           "    printf(\"error: sizeof(" #s ") should be %u, not %%u\\n\", sizeof(" #s "));\n" \
           "    g_cErrors++;\n" \
           "}\n" \
           "\n", \
           (unsigned)sizeof(s), (unsigned)sizeof(s))

#if 1
# define GEN_CHECK_OFF(s, m) \
   printf("dtrace:::BEGIN\n" \
          "/offsetof(" #s ", " #m ") != %u/\n" \
          "{\n" \
          "    printf(\"error: offsetof(" #s ", " #m ") should be %u, not %%u\\n\", offsetof(" #s ", " #m "));\n" \
          "    g_cErrors++;\n" \
          "}\n" \
          "\n", \
          (unsigned)RT_OFFSETOF(s, m), (unsigned)RT_OFFSETOF(s, m))

#else
# define GEN_CHECK_OFF(s, m) do { } while (0)
#endif

#define GEN_CHECK_OFF_DOT(s, m)  do { } while (0)


    /*
     *  Body.
     */
#define VBOX_FOR_DTRACE_LIB
#include "tstVMStruct.h"

    /*
     * Footer.
     */
    printf("dtrace:::BEGIN\n"
           "/g_cErrors != 0/\n"
           "{\n"
           "    printf(\"%%u errors!\\n\", g_cErrors);\n"
           "    exit(1);\n"
           "}\n"
           "\n"
           "dtrace:::BEGIN\n"
           "{\n"
           "    printf(\"Success!\\n\");\n"
           "    exit(0);\n"
           "}\n"
           "\n"
           );


    return (0);
}

